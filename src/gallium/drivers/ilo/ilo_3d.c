/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2012-2013 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#include "intel_winsys.h"

#include "ilo_3d_pipeline.h"
#include "ilo_context.h"
#include "ilo_cp.h"
#include "ilo_query.h"
#include "ilo_shader.h"
#include "ilo_state.h"
#include "ilo_3d.h"

/**
 * Hook for CP new-batch.
 */
void
ilo_3d_new_cp_batch(struct ilo_3d *hw3d)
{
   hw3d->new_batch = true;

   /* invalidate the pipeline */
   ilo_3d_pipeline_invalidate(hw3d->pipeline,
         ILO_3D_PIPELINE_INVALIDATE_BATCH_BO |
         ILO_3D_PIPELINE_INVALIDATE_STATE_BO);
   if (!hw3d->cp->hw_ctx) {
      ilo_3d_pipeline_invalidate(hw3d->pipeline,
            ILO_3D_PIPELINE_INVALIDATE_HW);
   }
}

/**
 * Hook for CP pre-flush.
 */
void
ilo_3d_pre_cp_flush(struct ilo_3d *hw3d)
{
}

/**
 * Hook for CP post-flush
 */
void
ilo_3d_post_cp_flush(struct ilo_3d *hw3d)
{
   if (ilo_debug & ILO_DEBUG_3D)
      ilo_3d_pipeline_dump(hw3d->pipeline);
}

/**
 * Create a 3D context.
 */
struct ilo_3d *
ilo_3d_create(struct ilo_cp *cp, int gen, int gt)
{
   struct ilo_3d *hw3d;

   hw3d = CALLOC_STRUCT(ilo_3d);
   if (!hw3d)
      return NULL;

   hw3d->cp = cp;
   hw3d->new_batch = true;

   hw3d->pipeline = ilo_3d_pipeline_create(cp, gen, gt);
   if (!hw3d->pipeline) {
      FREE(hw3d);
      return NULL;
   }

   return hw3d;
}

/**
 * Destroy a 3D context.
 */
void
ilo_3d_destroy(struct ilo_3d *hw3d)
{
   ilo_3d_pipeline_destroy(hw3d->pipeline);
   FREE(hw3d);
}

static bool
draw_vbo(struct ilo_3d *hw3d, const struct ilo_context *ilo,
         const struct pipe_draw_info *info,
         int *prim_generated, int *prim_emitted)
{
   bool need_flush;
   int max_len;

   ilo_cp_set_ring(hw3d->cp, ILO_CP_RING_RENDER);

   /*
    * Without a better tracking mechanism, when the framebuffer changes, we
    * have to assume that the old framebuffer may be sampled from.  If that
    * happens in the middle of a batch buffer, we need to insert manual
    * flushes.
    */
   need_flush = (!hw3d->new_batch && (ilo->dirty & ILO_DIRTY_FRAMEBUFFER));

   /* make sure there is enough room first */
   max_len = ilo_3d_pipeline_estimate_size(hw3d->pipeline,
         ILO_3D_PIPELINE_DRAW, ilo);
   if (need_flush) {
      max_len += ilo_3d_pipeline_estimate_size(hw3d->pipeline,
            ILO_3D_PIPELINE_FLUSH, NULL);
   }

   if (max_len > ilo_cp_space(hw3d->cp)) {
      ilo_cp_flush(hw3d->cp);
      need_flush = false;
      assert(max_len <= ilo_cp_space(hw3d->cp));
   }

   if (need_flush)
      ilo_3d_pipeline_emit_flush(hw3d->pipeline);

   return ilo_3d_pipeline_emit_draw(hw3d->pipeline, ilo, info,
         prim_generated, prim_emitted);
}

static bool
pass_render_condition(struct ilo_3d *hw3d, struct pipe_context *pipe)
{
   uint64_t result;
   bool wait;

   if (!hw3d->render_condition.query)
      return true;

   switch (hw3d->render_condition.mode) {
   case PIPE_RENDER_COND_WAIT:
   case PIPE_RENDER_COND_BY_REGION_WAIT:
      wait = true;
      break;
   case PIPE_RENDER_COND_NO_WAIT:
   case PIPE_RENDER_COND_BY_REGION_NO_WAIT:
   default:
      wait = false;
      break;
   }

   if (pipe->get_query_result(pipe, hw3d->render_condition.query,
            wait, (union pipe_query_result *) &result)) {
      return (result > 0);
   }
   else {
      return true;
   }
}

static void
ilo_draw_vbo(struct pipe_context *pipe, const struct pipe_draw_info *info)
{
   struct ilo_context *ilo = ilo_context(pipe);
   struct ilo_3d *hw3d = ilo->hw3d;
   int prim_generated, prim_emitted;

   if (!pass_render_condition(hw3d, pipe))
      return;

   /* assume the cache is still in use by the previous batch */
   if (hw3d->new_batch)
      ilo_shader_cache_mark_busy(ilo->shader_cache);

   ilo_finalize_states(ilo);

   /* the shaders may be uploaded to a new shader cache */
   if (hw3d->shader_cache_seqno != ilo->shader_cache->seqno) {
      ilo_3d_pipeline_invalidate(hw3d->pipeline,
            ILO_3D_PIPELINE_INVALIDATE_KERNEL_BO);
   }

   /*
    * The VBs and/or IB may have different BOs due to being mapped with
    * PIPE_TRANSFER_DISCARD_x.  We should track that instead of setting the
    * dirty flags for the performance reason.
    */
   ilo->dirty |= ILO_DIRTY_VERTEX_BUFFERS | ILO_DIRTY_INDEX_BUFFER;

   if (!draw_vbo(hw3d, ilo, info, &prim_generated, &prim_emitted))
      return;

   /* clear dirty status */
   ilo->dirty = 0x0;
   hw3d->new_batch = false;
   hw3d->shader_cache_seqno = ilo->shader_cache->seqno;

   if (ilo_debug & ILO_DEBUG_NOCACHE)
      ilo_3d_pipeline_emit_flush(hw3d->pipeline);
}

static void
ilo_render_condition(struct pipe_context *pipe,
                     struct pipe_query *query,
                     uint mode)
{
   struct ilo_context *ilo = ilo_context(pipe);
   struct ilo_3d *hw3d = ilo->hw3d;

   /* reference count? */
   hw3d->render_condition.query = query;
   hw3d->render_condition.mode = mode;
}

static void
ilo_texture_barrier(struct pipe_context *pipe)
{
   struct ilo_context *ilo = ilo_context(pipe);
   struct ilo_3d *hw3d = ilo->hw3d;

   if (ilo->cp->ring != ILO_CP_RING_RENDER)
      return;

   ilo_3d_pipeline_emit_flush(hw3d->pipeline);

   /* don't know why */
   if (ilo->gen >= ILO_GEN(7))
      ilo_cp_flush(hw3d->cp);
}

static void
ilo_get_sample_position(struct pipe_context *pipe,
                        unsigned sample_count,
                        unsigned sample_index,
                        float *out_value)
{
   struct ilo_context *ilo = ilo_context(pipe);
   struct ilo_3d *hw3d = ilo->hw3d;

   ilo_3d_pipeline_get_sample_position(hw3d->pipeline,
         sample_count, sample_index,
         &out_value[0], &out_value[1]);
}

/**
 * Initialize 3D-related functions.
 */
void
ilo_init_3d_functions(struct ilo_context *ilo)
{
   ilo->base.draw_vbo = ilo_draw_vbo;
   ilo->base.render_condition = ilo_render_condition;
   ilo->base.texture_barrier = ilo_texture_barrier;
   ilo->base.get_sample_position = ilo_get_sample_position;
}

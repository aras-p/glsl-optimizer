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
 * Begin a query.
 */
void
ilo_3d_begin_query(struct ilo_context *ilo, struct ilo_query *q)
{
   struct ilo_3d *hw3d = ilo->hw3d;

   ilo_cp_set_ring(hw3d->cp, ILO_CP_RING_RENDER);

   switch (q->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
      /* reserve some space for pausing the query */
      q->reg_cmd_size = ilo_3d_pipeline_estimate_size(hw3d->pipeline,
            ILO_3D_PIPELINE_WRITE_DEPTH_COUNT, NULL);
      ilo_cp_reserve_for_pre_flush(hw3d->cp, q->reg_cmd_size);

      q->data.u64 = 0;

      if (ilo_query_alloc_bo(q, 2, -1, hw3d->cp->winsys)) {
         /* XXX we should check the aperture size */
         ilo_3d_pipeline_emit_write_depth_count(hw3d->pipeline,
               q->bo, q->reg_read++);

         list_add(&q->list, &hw3d->occlusion_queries);
      }
      break;
   case PIPE_QUERY_TIMESTAMP:
      /* nop */
      break;
   case PIPE_QUERY_TIME_ELAPSED:
      /* reserve some space for pausing the query */
      q->reg_cmd_size = ilo_3d_pipeline_estimate_size(hw3d->pipeline,
            ILO_3D_PIPELINE_WRITE_TIMESTAMP, NULL);
      ilo_cp_reserve_for_pre_flush(hw3d->cp, q->reg_cmd_size);

      q->data.u64 = 0;

      if (ilo_query_alloc_bo(q, 2, -1, hw3d->cp->winsys)) {
         /* XXX we should check the aperture size */
         ilo_3d_pipeline_emit_write_timestamp(hw3d->pipeline,
               q->bo, q->reg_read++);

         list_add(&q->list, &hw3d->time_elapsed_queries);
      }
      break;
   case PIPE_QUERY_PRIMITIVES_GENERATED:
      q->data.u64 = 0;
      list_add(&q->list, &hw3d->prim_generated_queries);
      break;
   case PIPE_QUERY_PRIMITIVES_EMITTED:
      q->data.u64 = 0;
      list_add(&q->list, &hw3d->prim_emitted_queries);
      break;
   default:
      assert(!"unknown query type");
      break;
   }
}

/**
 * End a query.
 */
void
ilo_3d_end_query(struct ilo_context *ilo, struct ilo_query *q)
{
   struct ilo_3d *hw3d = ilo->hw3d;

   ilo_cp_set_ring(hw3d->cp, ILO_CP_RING_RENDER);

   switch (q->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
      list_del(&q->list);

      assert(q->reg_read < q->reg_total);
      ilo_cp_reserve_for_pre_flush(hw3d->cp, -q->reg_cmd_size);
      ilo_3d_pipeline_emit_write_depth_count(hw3d->pipeline,
            q->bo, q->reg_read++);
      break;
   case PIPE_QUERY_TIMESTAMP:
      q->data.u64 = 0;

      if (ilo_query_alloc_bo(q, 1, 1, hw3d->cp->winsys)) {
         ilo_3d_pipeline_emit_write_timestamp(hw3d->pipeline,
               q->bo, q->reg_read++);
      }
      break;
   case PIPE_QUERY_TIME_ELAPSED:
      list_del(&q->list);

      assert(q->reg_read < q->reg_total);
      ilo_cp_reserve_for_pre_flush(hw3d->cp, -q->reg_cmd_size);
      ilo_3d_pipeline_emit_write_timestamp(hw3d->pipeline,
            q->bo, q->reg_read++);
      break;
   case PIPE_QUERY_PRIMITIVES_GENERATED:
   case PIPE_QUERY_PRIMITIVES_EMITTED:
      list_del(&q->list);
      break;
   default:
      assert(!"unknown query type");
      break;
   }
}

static void
process_query_for_occlusion_counter(struct ilo_3d *hw3d,
                                    struct ilo_query *q)
{
   uint64_t *vals, depth_count = 0;
   int i;

   /* in pairs */
   assert(q->reg_read % 2 == 0);

   q->bo->map(q->bo, false);
   vals = q->bo->get_virtual(q->bo);
   for (i = 1; i < q->reg_read; i += 2)
      depth_count += vals[i] - vals[i - 1];
   q->bo->unmap(q->bo);

   /* accumulate so that the query can be resumed if wanted */
   q->data.u64 += depth_count;
   q->reg_read = 0;
}

static uint64_t
timestamp_to_ns(uint64_t timestamp)
{
   /* see ilo_get_timestamp() */
   return (timestamp & 0xffffffff) * 80;
}

static void
process_query_for_timestamp(struct ilo_3d *hw3d, struct ilo_query *q)
{
   uint64_t *vals, timestamp;

   assert(q->reg_read == 1);

   q->bo->map(q->bo, false);
   vals = q->bo->get_virtual(q->bo);
   timestamp = vals[0];
   q->bo->unmap(q->bo);

   q->data.u64 = timestamp_to_ns(timestamp);
   q->reg_read = 0;
}

static void
process_query_for_time_elapsed(struct ilo_3d *hw3d, struct ilo_query *q)
{
   uint64_t *vals, elapsed = 0;
   int i;

   /* in pairs */
   assert(q->reg_read % 2 == 0);

   q->bo->map(q->bo, false);
   vals = q->bo->get_virtual(q->bo);

   for (i = 1; i < q->reg_read; i += 2)
      elapsed += vals[i] - vals[i - 1];

   q->bo->unmap(q->bo);

   /* accumulate so that the query can be resumed if wanted */
   q->data.u64 += timestamp_to_ns(elapsed);
   q->reg_read = 0;
}

/**
 * Process the raw query data.
 */
void
ilo_3d_process_query(struct ilo_context *ilo, struct ilo_query *q)
{
   struct ilo_3d *hw3d = ilo->hw3d;

   switch (q->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
      if (q->bo)
         process_query_for_occlusion_counter(hw3d, q);
      break;
   case PIPE_QUERY_TIMESTAMP:
      if (q->bo)
         process_query_for_timestamp(hw3d, q);
      break;
   case PIPE_QUERY_TIME_ELAPSED:
      if (q->bo)
         process_query_for_time_elapsed(hw3d, q);
      break;
   case PIPE_QUERY_PRIMITIVES_GENERATED:
   case PIPE_QUERY_PRIMITIVES_EMITTED:
      break;
   default:
      assert(!"unknown query type");
      break;
   }
}

/**
 * Hook for CP new-batch.
 */
void
ilo_3d_new_cp_batch(struct ilo_3d *hw3d)
{
   struct ilo_query *q;

   hw3d->new_batch = true;

   /* invalidate the pipeline */
   ilo_3d_pipeline_invalidate(hw3d->pipeline,
         ILO_3D_PIPELINE_INVALIDATE_BATCH_BO |
         ILO_3D_PIPELINE_INVALIDATE_STATE_BO);
   if (!hw3d->cp->hw_ctx) {
      ilo_3d_pipeline_invalidate(hw3d->pipeline,
            ILO_3D_PIPELINE_INVALIDATE_HW);
   }

   /* resume occlusion queries */
   LIST_FOR_EACH_ENTRY(q, &hw3d->occlusion_queries, list) {
      /* accumulate the result if the bo is alreay full */
      if (q->reg_read >= q->reg_total)
         process_query_for_occlusion_counter(hw3d, q);

      ilo_3d_pipeline_emit_write_depth_count(hw3d->pipeline,
            q->bo, q->reg_read++);
   }

   /* resume timer queries */
   LIST_FOR_EACH_ENTRY(q, &hw3d->time_elapsed_queries, list) {
      /* accumulate the result if the bo is alreay full */
      if (q->reg_read >= q->reg_total)
         process_query_for_time_elapsed(hw3d, q);

      ilo_3d_pipeline_emit_write_timestamp(hw3d->pipeline,
            q->bo, q->reg_read++);
   }
}

/**
 * Hook for CP pre-flush.
 */
void
ilo_3d_pre_cp_flush(struct ilo_3d *hw3d)
{
   struct ilo_query *q;

   /* pause occlusion queries */
   LIST_FOR_EACH_ENTRY(q, &hw3d->occlusion_queries, list) {
      assert(q->reg_read < q->reg_total);
      ilo_3d_pipeline_emit_write_depth_count(hw3d->pipeline,
            q->bo, q->reg_read++);
   }

   /* pause timer queries */
   LIST_FOR_EACH_ENTRY(q, &hw3d->time_elapsed_queries, list) {
      assert(q->reg_read < q->reg_total);
      ilo_3d_pipeline_emit_write_timestamp(hw3d->pipeline,
            q->bo, q->reg_read++);
   }
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

   list_inithead(&hw3d->occlusion_queries);
   list_inithead(&hw3d->time_elapsed_queries);
   list_inithead(&hw3d->prim_generated_queries);
   list_inithead(&hw3d->prim_emitted_queries);

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

static void
update_prim_count(struct ilo_3d *hw3d, int generated, int emitted)
{
   struct ilo_query *q;

   LIST_FOR_EACH_ENTRY(q, &hw3d->prim_generated_queries, list)
      q->data.u64 += generated;

   LIST_FOR_EACH_ENTRY(q, &hw3d->prim_emitted_queries, list)
      q->data.u64 += emitted;
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

   update_prim_count(hw3d, prim_generated, prim_emitted);

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
   if (ilo->dev->gen >= ILO_GEN(7))
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

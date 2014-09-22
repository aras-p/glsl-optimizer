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

#include "util/u_prim.h"
#include "intel_winsys.h"

#include "ilo_3d_pipeline.h"
#include "ilo_blit.h"
#include "ilo_context.h"
#include "ilo_cp.h"
#include "ilo_query.h"
#include "ilo_shader.h"
#include "ilo_state.h"
#include "ilo_3d.h"

static void
ilo_3d_own_render_ring(struct ilo_3d *hw3d)
{
   ilo_cp_set_owner(hw3d->cp, INTEL_RING_RENDER, &hw3d->owner);
}

static uint64_t
query_timestamp_to_ns(const struct ilo_3d *hw3d, uint64_t timestamp)
{
   /* see ilo_get_timestamp() */
   return (timestamp & 0xffffffff) * 80;
}

/**
 * Process the bo and accumulate the result.  The bo is emptied.
 */
static void
query_process_bo(const struct ilo_3d *hw3d, struct ilo_query *q)
{
   const uint64_t *vals;
   uint64_t tmp;
   int i;

   if (!q->used)
      return;

   vals = intel_bo_map(q->bo, false);
   if (!vals) {
      q->used = 0;
      return;
   }

   switch (q->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_TIME_ELAPSED:
   case PIPE_QUERY_PRIMITIVES_GENERATED:
   case PIPE_QUERY_PRIMITIVES_EMITTED:
      assert(q->stride == sizeof(*vals) * 2);

      tmp = 0;
      for (i = 0; i < q->used; i++)
         tmp += vals[2 * i + 1] - vals[2 * i];

      if (q->type == PIPE_QUERY_TIME_ELAPSED)
         tmp = query_timestamp_to_ns(hw3d, tmp);

      q->result.u64 += tmp;
      break;
   case PIPE_QUERY_TIMESTAMP:
      assert(q->stride == sizeof(*vals));

      q->result.u64 = query_timestamp_to_ns(hw3d, vals[q->used - 1]);
      break;
   case PIPE_QUERY_PIPELINE_STATISTICS:
      assert(q->stride == sizeof(*vals) * 22);

      for (i = 0; i < q->used; i++) {
         struct pipe_query_data_pipeline_statistics *stats =
            &q->result.pipeline_statistics;
         const uint64_t *begin = vals + 22 * i;
         const uint64_t *end = begin + 11;

         stats->ia_vertices    += end[0] - begin[0];
         stats->ia_primitives  += end[1] - begin[1];
         stats->vs_invocations += end[2] - begin[2];
         stats->gs_invocations += end[3] - begin[3];
         stats->gs_primitives  += end[4] - begin[4];
         stats->c_invocations  += end[5] - begin[5];
         stats->c_primitives   += end[6] - begin[6];
         stats->ps_invocations += end[7] - begin[7];
         stats->hs_invocations += end[8] - begin[8];
         stats->ds_invocations += end[9] - begin[9];
         stats->cs_invocations += end[10] - begin[10];
      }
      break;
   default:
      break;
   }

   intel_bo_unmap(q->bo);

   q->used = 0;
}

static void
query_begin_bo(struct ilo_3d *hw3d, struct ilo_query *q)
{
   /* bo is full */
   if (q->used >= q->count)
      query_process_bo(hw3d, q);

   /* write the beginning value to the bo */
   if (q->in_pairs)
      ilo_3d_pipeline_emit_query(hw3d->pipeline, q, q->stride * q->used);
}

static void
query_end_bo(struct ilo_3d *hw3d, struct ilo_query *q)
{
   uint32_t offset;

   assert(q->used < q->count);

   offset = q->stride * q->used;
   if (q->in_pairs)
      offset += q->stride >> 1;

   q->used++;

   /* write the ending value to the bo */
   ilo_3d_pipeline_emit_query(hw3d->pipeline, q, offset);
}

bool
ilo_3d_init_query(struct pipe_context *pipe, struct ilo_query *q)
{
   struct ilo_context *ilo = ilo_context(pipe);
   unsigned bo_size;

   switch (q->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_TIME_ELAPSED:
   case PIPE_QUERY_PRIMITIVES_GENERATED:
   case PIPE_QUERY_PRIMITIVES_EMITTED:
      q->stride = sizeof(uint64_t);
      q->in_pairs = true;
      break;
   case PIPE_QUERY_TIMESTAMP:
      q->stride = sizeof(uint64_t);
      q->in_pairs = false;
      break;
   case PIPE_QUERY_PIPELINE_STATISTICS:
      q->stride = sizeof(uint64_t) * 11;
      q->in_pairs = true;
      break;
   default:
      return false;
      break;
   }

   q->cmd_len = ilo_3d_pipeline_estimate_size(ilo->hw3d->pipeline,
         ILO_3D_PIPELINE_QUERY, q);

   /* double cmd_len and stride if in pairs */
   q->cmd_len <<= q->in_pairs;
   q->stride <<= q->in_pairs;

   bo_size = (q->stride > 4096) ? q->stride : 4096;
   q->bo = intel_winsys_alloc_buffer(ilo->winsys, "query", bo_size, false);
   if (!q->bo)
      return false;

   q->count = bo_size / q->stride;

   return true;
}

void
ilo_3d_begin_query(struct pipe_context *pipe, struct ilo_query *q)
{
   struct ilo_3d *hw3d = ilo_context(pipe)->hw3d;

   ilo_3d_own_render_ring(hw3d);

   /* need to submit first */
   if (!ilo_builder_validate(&hw3d->cp->builder, 1, &q->bo) ||
         ilo_cp_space(hw3d->cp) < q->cmd_len) {
      ilo_cp_submit(hw3d->cp, "out of aperture or space");

      assert(ilo_builder_validate(&hw3d->cp->builder, 1, &q->bo));
      assert(ilo_cp_space(hw3d->cp) >= q->cmd_len);

      ilo_3d_own_render_ring(hw3d);
   }

   /* reserve the space for ending/pausing the query */
   hw3d->owner.reserve += q->cmd_len >> q->in_pairs;

   query_begin_bo(hw3d, q);

   if (q->in_pairs)
      list_add(&q->list, &hw3d->queries);
}

void
ilo_3d_end_query(struct pipe_context *pipe, struct ilo_query *q)
{
   struct ilo_3d *hw3d = ilo_context(pipe)->hw3d;

   ilo_3d_own_render_ring(hw3d);

   /* reclaim the reserved space */
   hw3d->owner.reserve -= q->cmd_len >> q->in_pairs;
   assert(hw3d->owner.reserve >= 0);

   query_end_bo(hw3d, q);

   list_delinit(&q->list);
}

/**
 * Process the raw query data.
 */
void
ilo_3d_process_query(struct pipe_context *pipe, struct ilo_query *q)
{
   struct ilo_3d *hw3d = ilo_context(pipe)->hw3d;

   query_process_bo(hw3d, q);
}

static void
ilo_3d_own_cp(struct ilo_cp *cp, void *data)
{
   struct ilo_3d *hw3d = data;

   /* multiply by 2 for both resuming and pausing */
   if (ilo_cp_space(hw3d->cp) < hw3d->owner.reserve * 2) {
      ilo_cp_submit(hw3d->cp, "out of space");
      assert(ilo_cp_space(hw3d->cp) >= hw3d->owner.reserve * 2);
   }

   while (true) {
      struct ilo_builder_snapshot snapshot;
      struct ilo_query *q;

      ilo_builder_batch_snapshot(&hw3d->cp->builder, &snapshot);

      /* resume queries */
      LIST_FOR_EACH_ENTRY(q, &hw3d->queries, list)
         query_begin_bo(hw3d, q);

      if (!ilo_builder_validate(&hw3d->cp->builder, 0, NULL)) {
         ilo_builder_batch_restore(&hw3d->cp->builder, &snapshot);

         if (ilo_builder_batch_used(&hw3d->cp->builder)) {
            ilo_cp_submit(hw3d->cp, "out of aperture");
            continue;
         }
      }

      break;
   }

   assert(ilo_cp_space(hw3d->cp) >= hw3d->owner.reserve);
}

static void
ilo_3d_release_cp(struct ilo_cp *cp, void *data)
{
   struct ilo_3d *hw3d = data;
   struct ilo_query *q;

   assert(ilo_cp_space(hw3d->cp) >= hw3d->owner.reserve);

   /* pause queries */
   LIST_FOR_EACH_ENTRY(q, &hw3d->queries, list)
      query_end_bo(hw3d, q);
}

/**
 * Hook for CP new-batch.
 */
void
ilo_3d_cp_submitted(struct ilo_3d *hw3d)
{
   /* invalidate the pipeline */
   ilo_3d_pipeline_invalidate(hw3d->pipeline,
         ILO_3D_PIPELINE_INVALIDATE_BATCH_BO |
         ILO_3D_PIPELINE_INVALIDATE_STATE_BO |
         ILO_3D_PIPELINE_INVALIDATE_KERNEL_BO);

   hw3d->new_batch = true;
}

/**
 * Create a 3D context.
 */
struct ilo_3d *
ilo_3d_create(struct ilo_cp *cp, const struct ilo_dev_info *dev)
{
   struct ilo_3d *hw3d;

   hw3d = CALLOC_STRUCT(ilo_3d);
   if (!hw3d)
      return NULL;

   hw3d->cp = cp;
   hw3d->owner.own = ilo_3d_own_cp;
   hw3d->owner.release = ilo_3d_release_cp;
   hw3d->owner.data = hw3d;
   hw3d->owner.reserve = 0;

   hw3d->new_batch = true;

   list_inithead(&hw3d->queries);

   hw3d->pipeline = ilo_3d_pipeline_create(cp, dev);
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
draw_vbo(struct ilo_3d *hw3d, const struct ilo_state_vector *vec)
{
   bool need_flush = false;
   bool success = true;
   int max_len, before_space;

   /* on GEN7+, we need SOL_RESET to reset the SO write offsets */
   if (ilo_dev_gen(hw3d->pipeline->dev) >= ILO_GEN(7) &&
       (vec->dirty & ILO_DIRTY_SO) && vec->so.enabled &&
       !vec->so.append_bitmask) {
      ilo_cp_submit(hw3d->cp, "SOL_RESET");
      ilo_cp_set_one_off_flags(hw3d->cp, INTEL_EXEC_GEN7_SOL_RESET);
   }

   ilo_3d_own_render_ring(hw3d);

   if (!hw3d->new_batch) {
      /*
       * Without a better tracking mechanism, when the framebuffer changes, we
       * have to assume that the old framebuffer may be sampled from.  If that
       * happens in the middle of a batch buffer, we need to insert manual
       * flushes.
       */
      need_flush = (vec->dirty & ILO_DIRTY_FB);

      /* same to SO target changes */
      need_flush |= (vec->dirty & ILO_DIRTY_SO);
   }

   /* make sure there is enough room first */
   max_len = ilo_3d_pipeline_estimate_size(hw3d->pipeline,
         ILO_3D_PIPELINE_DRAW, vec);
   if (need_flush) {
      max_len += ilo_3d_pipeline_estimate_size(hw3d->pipeline,
            ILO_3D_PIPELINE_FLUSH, NULL);
   }

   if (max_len > ilo_cp_space(hw3d->cp)) {
      ilo_cp_submit(hw3d->cp, "out of space");
      need_flush = false;
      assert(max_len <= ilo_cp_space(hw3d->cp));
   }

   /* space available before emission */
   before_space = ilo_cp_space(hw3d->cp);

   if (need_flush)
      ilo_3d_pipeline_emit_flush(hw3d->pipeline);

   while (true) {
      struct ilo_builder_snapshot snapshot;

      ilo_builder_batch_snapshot(&hw3d->cp->builder, &snapshot);

      ilo_3d_pipeline_emit_draw(hw3d->pipeline, vec);

      if (!ilo_builder_validate(&hw3d->cp->builder, 0, NULL)) {
         ilo_builder_batch_restore(&hw3d->cp->builder, &snapshot);

         /* flush and try again */
         if (ilo_builder_batch_used(&hw3d->cp->builder)) {
            ilo_cp_submit(hw3d->cp, "out of aperture");
            continue;
         }

         success = false;
      }

      break;
   }

   hw3d->pipeline->invalidate_flags = 0x0;

   /* sanity check size estimation */
   assert(before_space - ilo_cp_space(hw3d->cp) <= max_len);

   return success;
}

bool
ilo_3d_pass_render_condition(struct ilo_context *ilo)
{
   struct ilo_3d *hw3d = ilo->hw3d;
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

   if (ilo->base.get_query_result(&ilo->base, hw3d->render_condition.query,
            wait, (union pipe_query_result *) &result))
      return (!result == hw3d->render_condition.cond);
   else
      return true;
}

void
ilo_3d_draw_rectlist(struct ilo_3d *hw3d, const struct ilo_blitter *blitter)
{
   int max_len, before_space;

   ilo_3d_own_render_ring(hw3d);

   max_len = ilo_3d_pipeline_estimate_size(hw3d->pipeline,
         ILO_3D_PIPELINE_RECTLIST, blitter);
   max_len += ilo_3d_pipeline_estimate_size(hw3d->pipeline,
         ILO_3D_PIPELINE_FLUSH, NULL) * 2;

   if (max_len > ilo_cp_space(hw3d->cp)) {
      ilo_cp_submit(hw3d->cp, "out of space");
      assert(max_len <= ilo_cp_space(hw3d->cp));
   }

   before_space = ilo_cp_space(hw3d->cp);

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 313:
    *
    *     "If other rendering operations have preceded this clear, a
    *      PIPE_CONTROL with write cache flush enabled and Z-inhibit
    *      disabled must be issued before the rectangle primitive used for
    *      the depth buffer clear operation."
    *
    * From the Sandy Bridge PRM, volume 2 part 1, page 314:
    *
    *     "Depth buffer clear pass must be followed by a PIPE_CONTROL
    *      command with DEPTH_STALL bit set and Then followed by Depth
    *      FLUSH"
    *
    * But the pipeline has to be flushed both before and after not only
    * because of these workarounds.  We need them for reasons such as
    *
    *  - we may sample from a texture that was rendered to
    *  - we may sample from the fb shortly after
    *
    * Skip checking blitter->op and do the flushes.
    */
   if (!hw3d->new_batch)
      ilo_3d_pipeline_emit_flush(hw3d->pipeline);

   while (true) {
      struct ilo_builder_snapshot snapshot;

      ilo_builder_batch_snapshot(&hw3d->cp->builder, &snapshot);

      ilo_3d_pipeline_emit_rectlist(hw3d->pipeline, blitter);

      if (!ilo_builder_validate(&hw3d->cp->builder, 0, NULL)) {
         ilo_builder_batch_restore(&hw3d->cp->builder, &snapshot);

         /* flush and try again */
         if (ilo_builder_batch_used(&hw3d->cp->builder)) {
            ilo_cp_submit(hw3d->cp, "out of aperture");
            continue;
         }
      }

      break;
   }

   ilo_3d_pipeline_invalidate(hw3d->pipeline, ILO_3D_PIPELINE_INVALIDATE_HW);

   ilo_3d_pipeline_emit_flush(hw3d->pipeline);

   /* sanity check size estimation */
   assert(before_space - ilo_cp_space(hw3d->cp) <= max_len);

   hw3d->new_batch = false;
}

#define UPDATE_MIN2(a, b) (a) = MIN2((a), (b))
#define UPDATE_MAX2(a, b) (a) = MAX2((a), (b))

/**
 * \see find_sub_primitives() from core mesa
 */
static int
ilo_find_sub_primitives(const void *elements, unsigned element_size,
                    const struct pipe_draw_info *orig_info,
                    struct pipe_draw_info *info)
{
   const unsigned max_prims = orig_info->count - orig_info->start;
   unsigned i, cur_start, cur_count;
   int scan_index;
   unsigned scan_num;

   cur_start = orig_info->start;
   cur_count = 0;
   scan_num = 0;

#define IB_INDEX_READ(TYPE, INDEX) (((const TYPE *) elements)[INDEX])

#define SCAN_ELEMENTS(TYPE) \
   info[scan_num] = *orig_info; \
   info[scan_num].primitive_restart = false; \
   for (i = orig_info->start; i < orig_info->count; i++) { \
      scan_index = IB_INDEX_READ(TYPE, i); \
      if (scan_index == orig_info->restart_index) { \
         if (cur_count > 0) { \
            assert(scan_num < max_prims); \
            info[scan_num].start = cur_start; \
            info[scan_num].count = cur_count; \
            scan_num++; \
            info[scan_num] = *orig_info; \
            info[scan_num].primitive_restart = false; \
         } \
         cur_start = i + 1; \
         cur_count = 0; \
      } \
      else { \
         UPDATE_MIN2(info[scan_num].min_index, scan_index); \
         UPDATE_MAX2(info[scan_num].max_index, scan_index); \
         cur_count++; \
      } \
   } \
   if (cur_count > 0) { \
      assert(scan_num < max_prims); \
      info[scan_num].start = cur_start; \
      info[scan_num].count = cur_count; \
      scan_num++; \
   }

   switch (element_size) {
   case 1:
      SCAN_ELEMENTS(uint8_t);
      break;
   case 2:
      SCAN_ELEMENTS(uint16_t);
      break;
   case 4:
      SCAN_ELEMENTS(uint32_t);
      break;
   default:
      assert(0 && "bad index_size in find_sub_primitives()");
   }

#undef SCAN_ELEMENTS

   return scan_num;
}

static inline bool
ilo_check_restart_index(const struct ilo_context *ilo, unsigned restart_index)
{
   /*
    * Haswell (GEN(7.5)) supports an arbitrary cut index, check everything
    * older.
    */
   if (ilo_dev_gen(ilo->dev) >= ILO_GEN(7.5))
      return true;

   /* Note: indices must be unsigned byte, unsigned short or unsigned int */
   switch (ilo->state_vector.ib.index_size) {
   case 1:
      return ((restart_index & 0xff) == 0xff);
      break;
   case 2:
      return ((restart_index & 0xffff) == 0xffff);
      break;
   case 4:
      return (restart_index == 0xffffffff);
      break;
   }
   return false;
}

static inline bool
ilo_check_restart_prim_type(const struct ilo_context *ilo, unsigned prim)
{
   switch (prim) {
   case PIPE_PRIM_POINTS:
   case PIPE_PRIM_LINES:
   case PIPE_PRIM_LINE_STRIP:
   case PIPE_PRIM_TRIANGLES:
   case PIPE_PRIM_TRIANGLE_STRIP:
      /* All 965 GEN graphics support a cut index for these primitive types */
      return true;
      break;

   case PIPE_PRIM_LINE_LOOP:
   case PIPE_PRIM_POLYGON:
   case PIPE_PRIM_QUAD_STRIP:
   case PIPE_PRIM_QUADS:
   case PIPE_PRIM_TRIANGLE_FAN:
      if (ilo_dev_gen(ilo->dev) >= ILO_GEN(7.5)) {
         /* Haswell and newer parts can handle these prim types. */
         return true;
      }
      break;
   }

   return false;
}

/*
 * Handle VBOs using primitive restart.
 * Verify that restart index and primitive type can be handled by the HW.
 * Return true if this routine did the rendering
 * Return false if this routine did NOT render because restart can be handled
 * in HW.
 */
static void
ilo_draw_vbo_with_sw_restart(struct pipe_context *pipe,
                             const struct pipe_draw_info *info)
{
   struct ilo_state_vector *vec = &ilo_context(pipe)->state_vector;
   struct pipe_draw_info *restart_info = NULL;
   int sub_prim_count = 1;

   /*
    * We have to break up the primitive into chunks manually
    * Worst case, every other index could be a restart index so
    * need to have space for that many primitives
    */
   restart_info = MALLOC(((info->count + 1) / 2) * sizeof(*info));
   if (NULL == restart_info) {
      /* If we can't get memory for this, bail out */
      ilo_err("%s:%d - Out of memory", __FILE__, __LINE__);
      return;
   }

   if (vec->ib.buffer) {
      struct pipe_transfer *transfer;
      const void *map;

      map = pipe_buffer_map(pipe, vec->ib.buffer,
            PIPE_TRANSFER_READ, &transfer);

      sub_prim_count = ilo_find_sub_primitives(map + vec->ib.offset,
            vec->ib.index_size, info, restart_info);

      pipe_buffer_unmap(pipe, transfer);
   }
   else {
      sub_prim_count =
         ilo_find_sub_primitives(vec->ib.user_buffer,
               vec->ib.index_size, info, restart_info);
   }

   info = restart_info;

   while (sub_prim_count > 0) {
      pipe->draw_vbo(pipe, info);

      sub_prim_count--;
      info++;
   }

   FREE(restart_info);
}

static void
ilo_draw_vbo(struct pipe_context *pipe, const struct pipe_draw_info *info)
{
   struct ilo_context *ilo = ilo_context(pipe);
   struct ilo_3d *hw3d = ilo->hw3d;

   if (ilo_debug & ILO_DEBUG_DRAW) {
      if (info->indexed) {
         ilo_printf("indexed draw %s: "
               "index start %d, count %d, vertex range [%d, %d]\n",
               u_prim_name(info->mode), info->start, info->count,
               info->min_index, info->max_index);
      }
      else {
         ilo_printf("draw %s: vertex start %d, count %d\n",
               u_prim_name(info->mode), info->start, info->count);
      }

      ilo_state_vector_dump_dirty(&ilo->state_vector);
   }

   if (!ilo_3d_pass_render_condition(ilo))
      return;

   if (info->primitive_restart && info->indexed) {
      /*
       * Want to draw an indexed primitive using primitive restart
       * Check that HW can handle the request and fall to SW if not.
       */
      if (!ilo_check_restart_index(ilo, info->restart_index) ||
          !ilo_check_restart_prim_type(ilo, info->mode)) {
         ilo_draw_vbo_with_sw_restart(pipe, info);
         return;
      }
   }

   ilo_finalize_3d_states(ilo, info);

   ilo_shader_cache_upload(ilo->shader_cache, &hw3d->cp->builder);

   ilo_blit_resolve_framebuffer(ilo);

   /* If draw_vbo ever fails, return immediately. */
   if (!draw_vbo(hw3d, &ilo->state_vector))
      return;

   /* clear dirty status */
   ilo->state_vector.dirty = 0x0;
   hw3d->new_batch = false;

   /* avoid dangling pointer reference */
   ilo->state_vector.draw = NULL;

   if (ilo_debug & ILO_DEBUG_NOCACHE)
      ilo_3d_pipeline_emit_flush(hw3d->pipeline);
}

static void
ilo_render_condition(struct pipe_context *pipe,
                     struct pipe_query *query,
                     boolean condition,
                     uint mode)
{
   struct ilo_context *ilo = ilo_context(pipe);
   struct ilo_3d *hw3d = ilo->hw3d;

   /* reference count? */
   hw3d->render_condition.query = query;
   hw3d->render_condition.mode = mode;
   hw3d->render_condition.cond = condition;
}

static void
ilo_texture_barrier(struct pipe_context *pipe)
{
   struct ilo_context *ilo = ilo_context(pipe);
   struct ilo_3d *hw3d = ilo->hw3d;

   if (ilo->cp->ring != INTEL_RING_RENDER)
      return;

   ilo_3d_pipeline_emit_flush(hw3d->pipeline);

   /* don't know why */
   if (ilo_dev_gen(ilo->dev) >= ILO_GEN(7))
      ilo_cp_submit(hw3d->cp, "texture barrier");
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

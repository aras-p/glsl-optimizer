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
process_query_for_occlusion_counter(struct ilo_3d *hw3d,
                                    struct ilo_query *q)
{
   uint64_t *vals, depth_count = 0;
   int i;

   /* in pairs */
   assert(q->reg_read % 2 == 0);

   vals = intel_bo_map(q->bo, false);
   for (i = 1; i < q->reg_read; i += 2)
      depth_count += vals[i] - vals[i - 1];
   intel_bo_unmap(q->bo);

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

   vals = intel_bo_map(q->bo, false);
   timestamp = vals[0];
   intel_bo_unmap(q->bo);

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

   vals = intel_bo_map(q->bo, false);

   for (i = 1; i < q->reg_read; i += 2)
      elapsed += vals[i] - vals[i - 1];

   intel_bo_unmap(q->bo);

   /* accumulate so that the query can be resumed if wanted */
   q->data.u64 += timestamp_to_ns(elapsed);
   q->reg_read = 0;
}

static void
process_query_for_pipeline_statistics(struct ilo_3d *hw3d,
                                      struct ilo_query *q)
{
   const uint64_t *vals;
   int i;

   assert(q->reg_read % 22 == 0);

   vals = intel_bo_map(q->bo, false);

   for (i = 0; i < q->reg_read; i += 22) {
      struct pipe_query_data_pipeline_statistics *stats =
         &q->data.pipeline_statistics;
      const uint64_t *begin = vals + i;
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

   intel_bo_unmap(q->bo);

   q->reg_read = 0;
}

static void
ilo_3d_resume_queries(struct ilo_3d *hw3d)
{
   struct ilo_query *q;

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

   /* resume pipeline statistics queries */
   LIST_FOR_EACH_ENTRY(q, &hw3d->pipeline_statistics_queries, list) {
      /* accumulate the result if the bo is alreay full */
      if (q->reg_read >= q->reg_total)
         process_query_for_pipeline_statistics(hw3d, q);

      ilo_3d_pipeline_emit_write_statistics(hw3d->pipeline,
            q->bo, q->reg_read);
      q->reg_read += 11;
   }
}

static void
ilo_3d_pause_queries(struct ilo_3d *hw3d)
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

   /* pause pipeline statistics queries */
   LIST_FOR_EACH_ENTRY(q, &hw3d->pipeline_statistics_queries, list) {
      assert(q->reg_read < q->reg_total);
      ilo_3d_pipeline_emit_write_statistics(hw3d->pipeline,
            q->bo, q->reg_read);
      q->reg_read += 11;
   }
}

static void
ilo_3d_release_render_ring(struct ilo_cp *cp, void *data)
{
   struct ilo_3d *hw3d = data;

   ilo_3d_pause_queries(hw3d);
}

void
ilo_3d_own_render_ring(struct ilo_3d *hw3d)
{
   ilo_cp_set_ring(hw3d->cp, INTEL_RING_RENDER);

   if (ilo_cp_set_owner(hw3d->cp, &hw3d->owner, hw3d->owner_reserve))
      ilo_3d_resume_queries(hw3d);
}

/**
 * Begin a query.
 */
void
ilo_3d_begin_query(struct ilo_context *ilo, struct ilo_query *q)
{
   struct ilo_3d *hw3d = ilo->hw3d;

   ilo_3d_own_render_ring(hw3d);

   switch (q->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
      /* reserve some space for pausing the query */
      q->reg_cmd_size = ilo_3d_pipeline_estimate_size(hw3d->pipeline,
            ILO_3D_PIPELINE_WRITE_DEPTH_COUNT, NULL);
      hw3d->owner_reserve += q->reg_cmd_size;
      ilo_cp_set_owner(hw3d->cp, &hw3d->owner, hw3d->owner_reserve);

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
      hw3d->owner_reserve += q->reg_cmd_size;
      ilo_cp_set_owner(hw3d->cp, &hw3d->owner, hw3d->owner_reserve);

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
   case PIPE_QUERY_PIPELINE_STATISTICS:
      /* reserve some space for pausing the query */
      q->reg_cmd_size = ilo_3d_pipeline_estimate_size(hw3d->pipeline,
            ILO_3D_PIPELINE_WRITE_STATISTICS, NULL);
      hw3d->owner_reserve += q->reg_cmd_size;
      ilo_cp_set_owner(hw3d->cp, &hw3d->owner, hw3d->owner_reserve);

      memset(&q->data.pipeline_statistics, 0,
            sizeof(q->data.pipeline_statistics));

      if (ilo_query_alloc_bo(q, 11 * 2, -1, hw3d->cp->winsys)) {
         /* XXX we should check the aperture size */
         ilo_3d_pipeline_emit_write_statistics(hw3d->pipeline,
               q->bo, q->reg_read);
         q->reg_read += 11;

         list_add(&q->list, &hw3d->pipeline_statistics_queries);
      }
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

   ilo_3d_own_render_ring(hw3d);

   switch (q->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
      list_del(&q->list);

      assert(q->reg_read < q->reg_total);
      hw3d->owner_reserve -= q->reg_cmd_size;
      ilo_cp_set_owner(hw3d->cp, &hw3d->owner, hw3d->owner_reserve);
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
      hw3d->owner_reserve -= q->reg_cmd_size;
      ilo_cp_set_owner(hw3d->cp, &hw3d->owner, hw3d->owner_reserve);
      ilo_3d_pipeline_emit_write_timestamp(hw3d->pipeline,
            q->bo, q->reg_read++);
      break;
   case PIPE_QUERY_PRIMITIVES_GENERATED:
   case PIPE_QUERY_PRIMITIVES_EMITTED:
      list_del(&q->list);
      break;
   case PIPE_QUERY_PIPELINE_STATISTICS:
      list_del(&q->list);

      assert(q->reg_read + 11 <= q->reg_total);
      hw3d->owner_reserve -= q->reg_cmd_size;
      ilo_cp_set_owner(hw3d->cp, &hw3d->owner, hw3d->owner_reserve);
      ilo_3d_pipeline_emit_write_statistics(hw3d->pipeline,
            q->bo, q->reg_read);
      q->reg_read += 11;
      break;
   default:
      assert(!"unknown query type");
      break;
   }
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
   case PIPE_QUERY_PIPELINE_STATISTICS:
      if (q->bo)
         process_query_for_pipeline_statistics(hw3d, q);
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
ilo_3d_cp_flushed(struct ilo_3d *hw3d)
{
   if (ilo_debug & ILO_DEBUG_3D)
      ilo_3d_pipeline_dump(hw3d->pipeline);

   /* invalidate the pipeline */
   ilo_3d_pipeline_invalidate(hw3d->pipeline,
         ILO_3D_PIPELINE_INVALIDATE_BATCH_BO |
         ILO_3D_PIPELINE_INVALIDATE_STATE_BO);

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
   hw3d->owner.release_callback = ilo_3d_release_render_ring;
   hw3d->owner.release_data = hw3d;

   hw3d->new_batch = true;

   list_inithead(&hw3d->occlusion_queries);
   list_inithead(&hw3d->time_elapsed_queries);
   list_inithead(&hw3d->prim_generated_queries);
   list_inithead(&hw3d->prim_emitted_queries);
   list_inithead(&hw3d->pipeline_statistics_queries);

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

   if (hw3d->kernel.bo)
      intel_bo_unreference(hw3d->kernel.bo);

   FREE(hw3d);
}

static bool
draw_vbo(struct ilo_3d *hw3d, const struct ilo_context *ilo,
         int *prim_generated, int *prim_emitted)
{
   bool need_flush = false;
   int max_len;

   ilo_3d_own_render_ring(hw3d);

   if (!hw3d->new_batch) {
      /*
       * Without a better tracking mechanism, when the framebuffer changes, we
       * have to assume that the old framebuffer may be sampled from.  If that
       * happens in the middle of a batch buffer, we need to insert manual
       * flushes.
       */
      need_flush = (ilo->dirty & ILO_DIRTY_FB);

      /* same to SO target changes */
      need_flush |= (ilo->dirty & ILO_DIRTY_SO);
   }

   /* make sure there is enough room first */
   max_len = ilo_3d_pipeline_estimate_size(hw3d->pipeline,
         ILO_3D_PIPELINE_DRAW, ilo);
   if (need_flush) {
      max_len += ilo_3d_pipeline_estimate_size(hw3d->pipeline,
            ILO_3D_PIPELINE_FLUSH, NULL);
   }

   if (max_len > ilo_cp_space(hw3d->cp)) {
      ilo_cp_flush(hw3d->cp, "out of space");
      need_flush = false;
      assert(max_len <= ilo_cp_space(hw3d->cp));
   }

   if (need_flush)
      ilo_3d_pipeline_emit_flush(hw3d->pipeline);

   return ilo_3d_pipeline_emit_draw(hw3d->pipeline, ilo,
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
   if (ilo->dev->gen >= ILO_GEN(7.5))
      return true;

   /* Note: indices must be unsigned byte, unsigned short or unsigned int */
   switch (ilo->ib.index_size) {
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
      if (ilo->dev->gen >= ILO_GEN(7.5)) {
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
   struct ilo_context *ilo = ilo_context(pipe);
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

   if (ilo->ib.buffer) {
      struct pipe_transfer *transfer;
      const void *map;

      map = pipe_buffer_map(pipe, ilo->ib.buffer,
            PIPE_TRANSFER_READ, &transfer);

      sub_prim_count = ilo_find_sub_primitives(map + ilo->ib.offset,
            ilo->ib.index_size, info, restart_info);

      pipe_buffer_unmap(pipe, transfer);
   }
   else {
      sub_prim_count = ilo_find_sub_primitives(ilo->ib.user_buffer,
               ilo->ib.index_size, info, restart_info);
   }

   info = restart_info;

   while (sub_prim_count > 0) {
      pipe->draw_vbo(pipe, info);

      sub_prim_count--;
      info++;
   }

   FREE(restart_info);
}

static bool
upload_shaders(struct ilo_3d *hw3d, struct ilo_shader_cache *shc)
{
   bool incremental = true;
   int upload;

   upload = ilo_shader_cache_upload(shc,
         NULL, hw3d->kernel.used, incremental);
   if (!upload)
      return true;

   /*
    * Allocate a new bo.  When this is a new batch, assume the bo is still in
    * use by the previous batch and force allocation.
    *
    * Does it help to make shader cache upload with unsynchronized mapping,
    * and remove the check for new batch here?
    */
   if (hw3d->kernel.used + upload > hw3d->kernel.size || hw3d->new_batch) {
      unsigned new_size = (hw3d->kernel.size) ?
         hw3d->kernel.size : (8 * 1024);

      while (hw3d->kernel.used + upload > new_size)
         new_size *= 2;

      if (hw3d->kernel.bo)
         intel_bo_unreference(hw3d->kernel.bo);

      hw3d->kernel.bo = intel_winsys_alloc_buffer(hw3d->cp->winsys,
            "kernel bo", new_size, INTEL_DOMAIN_CPU);
      if (!hw3d->kernel.bo) {
         ilo_err("failed to allocate kernel bo\n");
         return false;
      }

      hw3d->kernel.used = 0;
      hw3d->kernel.size = new_size;
      incremental = false;

      assert(new_size >= ilo_shader_cache_upload(shc,
            NULL, hw3d->kernel.used, incremental));

      ilo_3d_pipeline_invalidate(hw3d->pipeline,
            ILO_3D_PIPELINE_INVALIDATE_KERNEL_BO);
   }

   upload = ilo_shader_cache_upload(shc,
         hw3d->kernel.bo, hw3d->kernel.used, incremental);
   if (upload < 0) {
      ilo_err("failed to upload shaders\n");
      return false;
   }

   hw3d->kernel.used += upload;

   assert(hw3d->kernel.used <= hw3d->kernel.size);

   return true;
}

static void
ilo_draw_vbo(struct pipe_context *pipe, const struct pipe_draw_info *info)
{
   struct ilo_context *ilo = ilo_context(pipe);
   struct ilo_3d *hw3d = ilo->hw3d;
   int prim_generated, prim_emitted;

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

      ilo_dump_dirty_flags(ilo->dirty);
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

   if (!upload_shaders(hw3d, ilo->shader_cache))
      return;

   ilo_blit_resolve_framebuffer(ilo);

   /* If draw_vbo ever fails, return immediately. */
   if (!draw_vbo(hw3d, ilo, &prim_generated, &prim_emitted))
      return;

   /* clear dirty status */
   ilo->dirty = 0x0;
   hw3d->new_batch = false;

   /* avoid dangling pointer reference */
   ilo->draw = NULL;

   update_prim_count(hw3d, prim_generated, prim_emitted);

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
   if (ilo->dev->gen >= ILO_GEN(7))
      ilo_cp_flush(hw3d->cp, "texture barrier");
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

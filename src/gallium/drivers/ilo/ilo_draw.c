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

#include "ilo_render.h"
#include "ilo_blit.h"
#include "ilo_context.h"
#include "ilo_cp.h"
#include "ilo_query.h"
#include "ilo_shader.h"
#include "ilo_state.h"
#include "ilo_draw.h"

static void
ilo_draw_set_owner(struct ilo_context *ilo)
{
   ilo_cp_set_owner(ilo->cp, INTEL_RING_RENDER, &ilo->draw.cp_owner);
}

static uint64_t
query_timestamp_to_ns(const struct ilo_context *ilo, uint64_t timestamp)
{
   /* see ilo_get_timestamp() */
   return (timestamp & 0xffffffff) * 80;
}

/**
 * Process the bo and accumulate the result.  The bo is emptied.
 */
static void
query_process_bo(const struct ilo_context *ilo, struct ilo_query *q)
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
         tmp = query_timestamp_to_ns(ilo, tmp);

      q->result.u64 += tmp;
      break;
   case PIPE_QUERY_TIMESTAMP:
      assert(q->stride == sizeof(*vals));

      q->result.u64 = query_timestamp_to_ns(ilo, vals[q->used - 1]);
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
query_begin_bo(struct ilo_context *ilo, struct ilo_query *q)
{
   /* bo is full */
   if (q->used >= q->count)
      query_process_bo(ilo, q);

   /* write the beginning value to the bo */
   if (q->in_pairs)
      ilo_render_emit_query(ilo->render, q, q->stride * q->used);
}

static void
query_end_bo(struct ilo_context *ilo, struct ilo_query *q)
{
   uint32_t offset;

   assert(q->used < q->count);

   offset = q->stride * q->used;
   if (q->in_pairs)
      offset += q->stride >> 1;

   q->used++;

   /* write the ending value to the bo */
   ilo_render_emit_query(ilo->render, q, offset);
}

bool
ilo_init_draw_query(struct ilo_context *ilo, struct ilo_query *q)
{
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

   q->cmd_len = ilo_render_get_query_len(ilo->render, q->type);

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
ilo_begin_draw_query(struct ilo_context *ilo, struct ilo_query *q)
{
   ilo_draw_set_owner(ilo);

   /* need to submit first */
   if (!ilo_builder_validate(&ilo->cp->builder, 1, &q->bo) ||
         ilo_cp_space(ilo->cp) < q->cmd_len) {
      ilo_cp_submit(ilo->cp, "out of aperture or space");

      assert(ilo_builder_validate(&ilo->cp->builder, 1, &q->bo));
      assert(ilo_cp_space(ilo->cp) >= q->cmd_len);

      ilo_draw_set_owner(ilo);
   }

   /* reserve the space for ending/pausing the query */
   ilo->draw.cp_owner.reserve += q->cmd_len >> q->in_pairs;

   query_begin_bo(ilo, q);

   if (q->in_pairs)
      list_add(&q->list, &ilo->draw.queries);
}

void
ilo_end_draw_query(struct ilo_context *ilo, struct ilo_query *q)
{
   ilo_draw_set_owner(ilo);

   /* reclaim the reserved space */
   ilo->draw.cp_owner.reserve -= q->cmd_len >> q->in_pairs;
   assert(ilo->draw.cp_owner.reserve >= 0);

   query_end_bo(ilo, q);

   list_delinit(&q->list);
}

/**
 * Process the raw query data.
 */
void
ilo_process_draw_query(struct ilo_context *ilo, struct ilo_query *q)
{
   query_process_bo(ilo, q);
}

static void
ilo_draw_own_cp(struct ilo_cp *cp, void *data)
{
   struct ilo_context *ilo = data;

   /* multiply by 2 for both resuming and pausing */
   if (ilo_cp_space(ilo->cp) < ilo->draw.cp_owner.reserve * 2) {
      ilo_cp_submit(ilo->cp, "out of space");
      assert(ilo_cp_space(ilo->cp) >= ilo->draw.cp_owner.reserve * 2);
   }

   while (true) {
      struct ilo_builder_snapshot snapshot;
      struct ilo_query *q;

      ilo_builder_batch_snapshot(&ilo->cp->builder, &snapshot);

      /* resume queries */
      LIST_FOR_EACH_ENTRY(q, &ilo->draw.queries, list)
         query_begin_bo(ilo, q);

      if (!ilo_builder_validate(&ilo->cp->builder, 0, NULL)) {
         ilo_builder_batch_restore(&ilo->cp->builder, &snapshot);

         if (ilo_builder_batch_used(&ilo->cp->builder)) {
            ilo_cp_submit(ilo->cp, "out of aperture");
            continue;
         }
      }

      break;
   }

   assert(ilo_cp_space(ilo->cp) >= ilo->draw.cp_owner.reserve);
}

static void
ilo_draw_release_cp(struct ilo_cp *cp, void *data)
{
   struct ilo_context *ilo = data;
   struct ilo_query *q;

   assert(ilo_cp_space(ilo->cp) >= ilo->draw.cp_owner.reserve);

   /* pause queries */
   LIST_FOR_EACH_ENTRY(q, &ilo->draw.queries, list)
      query_end_bo(ilo, q);
}

static bool
draw_vbo(struct ilo_context *ilo, const struct ilo_state_vector *vec)
{
   bool need_flush = false;
   bool success = true;
   int max_len, before_space;

   /* on GEN7+, we need SOL_RESET to reset the SO write offsets */
   if (ilo_dev_gen(ilo->dev) >= ILO_GEN(7) &&
       (vec->dirty & ILO_DIRTY_SO) && vec->so.enabled &&
       !vec->so.append_bitmask) {
      ilo_cp_submit(ilo->cp, "SOL_RESET");
      ilo_cp_set_one_off_flags(ilo->cp, INTEL_EXEC_GEN7_SOL_RESET);
   }

   if (ilo_builder_batch_used(&ilo->cp->builder)) {
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

   ilo_draw_set_owner(ilo);

   /* make sure there is enough room first */
   max_len = ilo_render_get_draw_len(ilo->render, vec);
   if (need_flush)
      max_len += ilo_render_get_flush_len(ilo->render);

   if (max_len > ilo_cp_space(ilo->cp)) {
      ilo_cp_submit(ilo->cp, "out of space");
      need_flush = false;
      assert(max_len <= ilo_cp_space(ilo->cp));
   }

   /* space available before emission */
   before_space = ilo_cp_space(ilo->cp);

   if (need_flush)
      ilo_render_emit_flush(ilo->render);

   while (true) {
      struct ilo_builder_snapshot snapshot;

      ilo_builder_batch_snapshot(&ilo->cp->builder, &snapshot);

      ilo_render_emit_draw(ilo->render, vec);

      if (!ilo_builder_validate(&ilo->cp->builder, 0, NULL)) {
         ilo_builder_batch_restore(&ilo->cp->builder, &snapshot);

         /* flush and try again */
         if (ilo_builder_batch_used(&ilo->cp->builder)) {
            ilo_cp_submit(ilo->cp, "out of aperture");
            continue;
         }

         success = false;
      }

      break;
   }

   /* sanity check size estimation */
   assert(before_space - ilo_cp_space(ilo->cp) <= max_len);

   return success;
}

void
ilo_draw_rectlist(struct ilo_context *ilo)
{
   int max_len, before_space;
   bool need_flush;

   need_flush = ilo_builder_batch_used(&ilo->cp->builder);

   ilo_draw_set_owner(ilo);

   max_len = ilo_render_get_rectlist_len(ilo->render, ilo->blitter);
   max_len += ilo_render_get_flush_len(ilo->render) * 2;

   if (max_len > ilo_cp_space(ilo->cp)) {
      ilo_cp_submit(ilo->cp, "out of space");
      need_flush = false;
      assert(max_len <= ilo_cp_space(ilo->cp));
   }

   before_space = ilo_cp_space(ilo->cp);

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
   if (need_flush)
      ilo_render_emit_flush(ilo->render);

   while (true) {
      struct ilo_builder_snapshot snapshot;

      ilo_builder_batch_snapshot(&ilo->cp->builder, &snapshot);

      ilo_render_emit_rectlist(ilo->render, ilo->blitter);

      if (!ilo_builder_validate(&ilo->cp->builder, 0, NULL)) {
         ilo_builder_batch_restore(&ilo->cp->builder, &snapshot);

         /* flush and try again */
         if (ilo_builder_batch_used(&ilo->cp->builder)) {
            ilo_cp_submit(ilo->cp, "out of aperture");
            continue;
         }
      }

      break;
   }

   ilo_render_invalidate_hw(ilo->render);

   ilo_render_emit_flush(ilo->render);

   /* sanity check size estimation */
   assert(before_space - ilo_cp_space(ilo->cp) <= max_len);
}

static void
draw_vbo_with_sw_restart(struct ilo_context *ilo,
                         const struct pipe_draw_info *info)
{
   const struct ilo_ib_state *ib = &ilo->state_vector.ib;
   union {
      const void *ptr;
      const uint8_t *u8;
      const uint16_t *u16;
      const uint32_t *u32;
   } u;

   /* we will draw with IB mapped */
   if (ib->buffer) {
      u.ptr = intel_bo_map(ilo_buffer(ib->buffer)->bo, false);
      if (u.ptr)
         u.u8 += ib->offset;
   } else {
      u.ptr = ib->user_buffer;
   }

   if (!u.ptr)
      return;

#define DRAW_VBO_WITH_SW_RESTART(pipe, info, ptr) do {   \
   const unsigned end = (info)->start + (info)->count;   \
   struct pipe_draw_info subinfo;                        \
   unsigned i;                                           \
                                                         \
   subinfo = *(info);                                    \
   subinfo.primitive_restart = false;                    \
   for (i = (info)->start; i < end; i++) {               \
      if ((ptr)[i] == (info)->restart_index) {           \
         subinfo.count = i - subinfo.start;              \
         if (subinfo.count)                              \
            (pipe)->draw_vbo(pipe, &subinfo);            \
         subinfo.start = i + 1;                          \
      }                                                  \
   }                                                     \
   subinfo.count = i - subinfo.start;                    \
   if (subinfo.count)                                    \
      (pipe)->draw_vbo(pipe, &subinfo);                  \
} while (0)

   switch (ib->index_size) {
   case 1:
      DRAW_VBO_WITH_SW_RESTART(&ilo->base, info, u.u8);
      break;
   case 2:
      DRAW_VBO_WITH_SW_RESTART(&ilo->base, info, u.u16);
      break;
   case 4:
      DRAW_VBO_WITH_SW_RESTART(&ilo->base, info, u.u32);
      break;
   default:
      assert(!"unsupported index size");
      break;
   }

#undef DRAW_VBO_WITH_SW_RESTART

   if (ib->buffer)
      intel_bo_unmap(ilo_buffer(ib->buffer)->bo);
}

static bool
draw_vbo_need_sw_restart(const struct ilo_context *ilo,
                         const struct pipe_draw_info *info)
{
   /* the restart index is fixed prior to GEN7.5 */
   if (ilo_dev_gen(ilo->dev) < ILO_GEN(7.5)) {
      const unsigned cut_index =
         (ilo->state_vector.ib.index_size == 1) ? 0xff :
         (ilo->state_vector.ib.index_size == 2) ? 0xffff :
         (ilo->state_vector.ib.index_size == 4) ? 0xffffffff : 0;

      if (info->restart_index < cut_index)
         return true;
   }

   switch (info->mode) {
   case PIPE_PRIM_POINTS:
   case PIPE_PRIM_LINES:
   case PIPE_PRIM_LINE_STRIP:
   case PIPE_PRIM_TRIANGLES:
   case PIPE_PRIM_TRIANGLE_STRIP:
      /* these never need software fallback */
      return false;
   case PIPE_PRIM_LINE_LOOP:
   case PIPE_PRIM_POLYGON:
   case PIPE_PRIM_QUAD_STRIP:
   case PIPE_PRIM_QUADS:
   case PIPE_PRIM_TRIANGLE_FAN:
      /* these need software fallback prior to GEN7.5 */
      return (ilo_dev_gen(ilo->dev) < ILO_GEN(7.5));
   default:
      /* the rest always needs software fallback */
      return true;
   }
}

static void
ilo_draw_vbo(struct pipe_context *pipe, const struct pipe_draw_info *info)
{
   struct ilo_context *ilo = ilo_context(pipe);

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

   if (ilo_skip_rendering(ilo))
      return;

   if (info->primitive_restart && info->indexed &&
       draw_vbo_need_sw_restart(ilo, info)) {
      draw_vbo_with_sw_restart(ilo, info);
      return;
   }

   ilo_finalize_3d_states(ilo, info);

   ilo_shader_cache_upload(ilo->shader_cache, &ilo->cp->builder);

   ilo_blit_resolve_framebuffer(ilo);

   /* If draw_vbo ever fails, return immediately. */
   if (!draw_vbo(ilo, &ilo->state_vector))
      return;

   /* clear dirty status */
   ilo->state_vector.dirty = 0x0;

   /* avoid dangling pointer reference */
   ilo->state_vector.draw = NULL;

   if (ilo_debug & ILO_DEBUG_NOCACHE)
      ilo_render_emit_flush(ilo->render);
}

static void
ilo_texture_barrier(struct pipe_context *pipe)
{
   struct ilo_context *ilo = ilo_context(pipe);

   if (ilo->cp->ring != INTEL_RING_RENDER)
      return;

   ilo_render_emit_flush(ilo->render);

   /* don't know why */
   if (ilo_dev_gen(ilo->dev) >= ILO_GEN(7))
      ilo_cp_submit(ilo->cp, "texture barrier");
}

static void
ilo_get_sample_position(struct pipe_context *pipe,
                        unsigned sample_count,
                        unsigned sample_index,
                        float *out_value)
{
   struct ilo_context *ilo = ilo_context(pipe);

   ilo_render_get_sample_position(ilo->render,
         sample_count, sample_index,
         &out_value[0], &out_value[1]);
}

void
ilo_init_draw(struct ilo_context *ilo)
{
   ilo->draw.cp_owner.own = ilo_draw_own_cp;
   ilo->draw.cp_owner.release = ilo_draw_release_cp;
   ilo->draw.cp_owner.data = (void *) ilo;
   ilo->draw.cp_owner.reserve = 0;

   list_inithead(&ilo->draw.queries);
}

/**
 * Initialize 3D-related functions.
 */
void
ilo_init_draw_functions(struct ilo_context *ilo)
{
   ilo->base.draw_vbo = ilo_draw_vbo;
   ilo->base.texture_barrier = ilo_texture_barrier;
   ilo->base.get_sample_position = ilo_get_sample_position;
}

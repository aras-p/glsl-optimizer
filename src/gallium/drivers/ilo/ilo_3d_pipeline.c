/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2013 LunarG, Inc.
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

#include "ilo_blitter.h"
#include "ilo_context.h"
#include "ilo_cp.h"
#include "ilo_state.h"
#include "ilo_3d_pipeline_gen6.h"
#include "ilo_3d_pipeline_gen7.h"
#include "ilo_3d_pipeline.h"

/* in U0.4 */
struct sample_position {
   uint8_t x, y;
};

/* \see gen6_get_sample_position() */
static const struct sample_position sample_position_1x[1] = {
   {  8,  8 },
};

static const struct sample_position sample_position_4x[4] = {
   {  6,  2 }, /* distance from the center is sqrt(40) */
   { 14,  6 }, /* distance from the center is sqrt(40) */
   {  2, 10 }, /* distance from the center is sqrt(40) */
   { 10, 14 }, /* distance from the center is sqrt(40) */
};

static const struct sample_position sample_position_8x[8] = {
   {  7,  9 }, /* distance from the center is sqrt(2) */
   {  9, 13 }, /* distance from the center is sqrt(26) */
   { 11,  3 }, /* distance from the center is sqrt(34) */
   { 13, 11 }, /* distance from the center is sqrt(34) */
   {  1,  7 }, /* distance from the center is sqrt(50) */
   {  5,  1 }, /* distance from the center is sqrt(58) */
   { 15,  5 }, /* distance from the center is sqrt(58) */
   {  3, 15 }, /* distance from the center is sqrt(74) */
};

struct ilo_3d_pipeline *
ilo_3d_pipeline_create(struct ilo_cp *cp, const struct ilo_dev_info *dev)
{
   struct ilo_3d_pipeline *p;
   int i;

   p = CALLOC_STRUCT(ilo_3d_pipeline);
   if (!p)
      return NULL;

   p->cp = cp;
   p->dev = dev;

   switch (p->dev->gen) {
   case ILO_GEN(6):
      ilo_3d_pipeline_init_gen6(p);
      break;
   case ILO_GEN(7):
   case ILO_GEN(7.5):
      ilo_3d_pipeline_init_gen7(p);
      break;
   default:
      assert(!"unsupported GEN");
      FREE(p);
      return NULL;
      break;
   }

   p->invalidate_flags = ILO_3D_PIPELINE_INVALIDATE_ALL;

   p->workaround_bo = intel_winsys_alloc_buffer(p->cp->winsys,
         "PIPE_CONTROL workaround", 4096, INTEL_DOMAIN_INSTRUCTION);
   if (!p->workaround_bo) {
      ilo_warn("failed to allocate PIPE_CONTROL workaround bo\n");
      FREE(p);
      return NULL;
   }

   p->packed_sample_position_1x =
      sample_position_1x[0].x << 4 |
      sample_position_1x[0].y;

   /* pack into dwords */
   for (i = 0; i < 4; i++) {
      p->packed_sample_position_4x |=
         sample_position_4x[i].x << (8 * i + 4) |
         sample_position_4x[i].y << (8 * i);

      p->packed_sample_position_8x[0] |=
         sample_position_8x[i].x << (8 * i + 4) |
         sample_position_8x[i].y << (8 * i);

      p->packed_sample_position_8x[1] |=
         sample_position_8x[4 + i].x << (8 * i + 4) |
         sample_position_8x[4 + i].y << (8 * i);
   }

   return p;
}

void
ilo_3d_pipeline_destroy(struct ilo_3d_pipeline *p)
{
   if (p->workaround_bo)
      intel_bo_unreference(p->workaround_bo);

   FREE(p);
}

static void
handle_invalid_batch_bo(struct ilo_3d_pipeline *p, bool unset)
{
   if (p->invalidate_flags & ILO_3D_PIPELINE_INVALIDATE_BATCH_BO) {
      if (p->dev->gen == ILO_GEN(6))
         p->state.has_gen6_wa_pipe_control = false;

      if (unset)
         p->invalidate_flags &= ~ILO_3D_PIPELINE_INVALIDATE_BATCH_BO;
   }
}

/**
 * Emit context states and 3DPRIMITIVE.
 */
bool
ilo_3d_pipeline_emit_draw(struct ilo_3d_pipeline *p,
                          const struct ilo_context *ilo,
                          int *prim_generated, int *prim_emitted)
{
   bool success;

   if (ilo->dirty & ILO_DIRTY_SO &&
       ilo->so.enabled && !ilo->so.append_bitmask) {
      /*
       * We keep track of the SVBI in the driver, so that we can restore it
       * when the HW context is invalidated (by another process).  The value
       * needs to be reset when stream output is enabled and the targets are
       * changed.
       */
      p->state.so_num_vertices = 0;

      /* on GEN7+, we need SOL_RESET to reset the SO write offsets */
      if (p->dev->gen >= ILO_GEN(7))
         ilo_cp_set_one_off_flags(p->cp, INTEL_EXEC_GEN7_SOL_RESET);
   }


   while (true) {
      struct ilo_cp_jmp_buf jmp;

      /* we will rewind if aperture check below fails */
      ilo_cp_setjmp(p->cp, &jmp);

      handle_invalid_batch_bo(p, false);

      /* draw! */
      ilo_cp_assert_no_implicit_flush(p->cp, true);
      p->emit_draw(p, ilo);
      ilo_cp_assert_no_implicit_flush(p->cp, false);

      if (intel_winsys_can_submit_bo(ilo->winsys, &p->cp->bo, 1)) {
         success = true;
         break;
      }

      /* rewind */
      ilo_cp_longjmp(p->cp, &jmp);

      if (ilo_cp_empty(p->cp)) {
         success = false;
         break;
      }
      else {
         /* flush and try again */
         ilo_cp_flush(p->cp, "out of aperture");
      }
   }

   if (success) {
      const int num_verts =
         u_vertices_per_prim(u_reduced_prim(ilo->draw->mode));
      const int max_emit =
         (p->state.so_max_vertices - p->state.so_num_vertices) / num_verts;
      const int generated =
         u_reduced_prims_for_vertices(ilo->draw->mode, ilo->draw->count);
      const int emitted = MIN2(generated, max_emit);

      p->state.so_num_vertices += emitted * num_verts;

      if (prim_generated)
         *prim_generated = generated;

      if (prim_emitted)
         *prim_emitted = emitted;
   }

   p->invalidate_flags = 0x0;

   return success;
}

/**
 * Emit PIPE_CONTROL to flush all caches.
 */
void
ilo_3d_pipeline_emit_flush(struct ilo_3d_pipeline *p)
{
   handle_invalid_batch_bo(p, true);
   p->emit_flush(p);
}

/**
 * Emit PIPE_CONTROL with GEN6_PIPE_CONTROL_WRITE_TIMESTAMP post-sync op.
 */
void
ilo_3d_pipeline_emit_write_timestamp(struct ilo_3d_pipeline *p,
                                     struct intel_bo *bo, int index)
{
   handle_invalid_batch_bo(p, true);
   p->emit_write_timestamp(p, bo, index);
}

/**
 * Emit PIPE_CONTROL with GEN6_PIPE_CONTROL_WRITE_PS_DEPTH_COUNT post-sync op.
 */
void
ilo_3d_pipeline_emit_write_depth_count(struct ilo_3d_pipeline *p,
                                       struct intel_bo *bo, int index)
{
   handle_invalid_batch_bo(p, true);
   p->emit_write_depth_count(p, bo, index);
}

/**
 * Emit MI_STORE_REGISTER_MEM to store statistics registers.
 */
void
ilo_3d_pipeline_emit_write_statistics(struct ilo_3d_pipeline *p,
                                      struct intel_bo *bo, int index)
{
   handle_invalid_batch_bo(p, true);
   p->emit_write_statistics(p, bo, index);
}

void
ilo_3d_pipeline_emit_rectlist(struct ilo_3d_pipeline *p,
                              const struct ilo_blitter *blitter)
{
   const int max_len = ilo_3d_pipeline_estimate_size(p,
         ILO_3D_PIPELINE_RECTLIST, blitter);

   if (max_len > ilo_cp_space(p->cp))
      ilo_cp_flush(p->cp, "out of space");

   while (true) {
      struct ilo_cp_jmp_buf jmp;

      /* we will rewind if aperture check below fails */
      ilo_cp_setjmp(p->cp, &jmp);

      handle_invalid_batch_bo(p, false);

      ilo_cp_assert_no_implicit_flush(p->cp, true);
      p->emit_rectlist(p, blitter);
      ilo_cp_assert_no_implicit_flush(p->cp, false);

      if (!intel_winsys_can_submit_bo(blitter->ilo->winsys, &p->cp->bo, 1)) {
         /* rewind */
         ilo_cp_longjmp(p->cp, &jmp);

         /* flush and try again */
         if (!ilo_cp_empty(p->cp)) {
            ilo_cp_flush(p->cp, "out of aperture");
            continue;
         }
      }

      break;
   }

   ilo_3d_pipeline_invalidate(p, ILO_3D_PIPELINE_INVALIDATE_HW);
}

void
ilo_3d_pipeline_get_sample_position(struct ilo_3d_pipeline *p,
                                    unsigned sample_count,
                                    unsigned sample_index,
                                    float *x, float *y)
{
   const struct sample_position *pos;

   switch (sample_count) {
   case 1:
      assert(sample_index < Elements(sample_position_1x));
      pos = sample_position_1x;
      break;
   case 4:
      assert(sample_index < Elements(sample_position_4x));
      pos = sample_position_4x;
      break;
   case 8:
      assert(sample_index < Elements(sample_position_8x));
      pos = sample_position_8x;
      break;
   default:
      assert(!"unknown sample count");
      *x = 0.5f;
      *y = 0.5f;
      return;
      break;
   }

   *x = (float) pos[sample_index].x / 16.0f;
   *y = (float) pos[sample_index].y / 16.0f;
}

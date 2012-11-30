/*
 * Copyright © 2012 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <stdbool.h>
#include "brw_state.h"
#include "brw_defines.h"
#include "intel_batchbuffer.h"

static void
upload_ps_extra(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   /* BRW_NEW_FRAGMENT_PROGRAM */
   const struct brw_fragment_program *fp =
      brw_fragment_program_const(brw->fragment_program);
   uint32_t dw1 = 0;

   dw1 |= GEN8_PSX_PIXEL_SHADER_VALID;

   if (fp->program.UsesKill)
      dw1 |= GEN8_PSX_KILL_ENABLE;

   /* BRW_NEW_FRAGMENT_PROGRAM */
   if (brw->wm.prog_data->num_varying_inputs != 0)
      dw1 |= GEN8_PSX_ATTRIBUTE_ENABLE;

   if (fp->program.Base.OutputsWritten & BITFIELD64_BIT(FRAG_RESULT_DEPTH)) {
      switch (fp->program.FragDepthLayout) {
         case FRAG_DEPTH_LAYOUT_NONE:
         case FRAG_DEPTH_LAYOUT_ANY:
            dw1 |= GEN8_PSX_PSCDEPTH_ON;
            break;
         case FRAG_DEPTH_LAYOUT_GREATER:
            dw1 |= GEN8_PSX_PSCDEPTH_ON_GE;
            break;
         case FRAG_DEPTH_LAYOUT_LESS:
            dw1 |= GEN8_PSX_PSCDEPTH_ON_LE;
            break;
         case FRAG_DEPTH_LAYOUT_UNCHANGED:
            break;
      }
   }

   if (fp->program.Base.InputsRead & VARYING_BIT_POS)
      dw1 |= GEN8_PSX_USES_SOURCE_DEPTH | GEN8_PSX_USES_SOURCE_W;

   BEGIN_BATCH(2);
   OUT_BATCH(_3DSTATE_PS_EXTRA << 16 | (2 - 2));
   OUT_BATCH(dw1);
   ADVANCE_BATCH();
}

const struct brw_tracked_state gen8_ps_extra = {
   .dirty = {
      .mesa  = 0,
      .brw   = BRW_NEW_CONTEXT | BRW_NEW_FRAGMENT_PROGRAM,
      .cache = 0,
   },
   .emit = upload_ps_extra,
};

static void
upload_wm_state(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   uint32_t dw1 = 0;

   dw1 |= GEN7_WM_STATISTICS_ENABLE;
   dw1 |= GEN7_WM_LINE_AA_WIDTH_1_0;
   dw1 |= GEN7_WM_LINE_END_CAP_AA_WIDTH_0_5;
   dw1 |= GEN7_WM_POINT_RASTRULE_UPPER_RIGHT;

   /* _NEW_LINE */
   if (ctx->Line.StippleFlag)
      dw1 |= GEN7_WM_LINE_STIPPLE_ENABLE;

   /* _NEW_POLYGON */
   if (ctx->Polygon.StippleFlag)
      dw1 |= GEN7_WM_POLYGON_STIPPLE_ENABLE;

   /* CACHE_NEW_WM_PROG */
   dw1 |= brw->wm.prog_data->barycentric_interp_modes <<
      GEN7_WM_BARYCENTRIC_INTERPOLATION_MODE_SHIFT;

   BEGIN_BATCH(2);
   OUT_BATCH(_3DSTATE_WM << 16 | (2 - 2));
   OUT_BATCH(dw1);
   ADVANCE_BATCH();
}

const struct brw_tracked_state gen8_wm_state = {
   .dirty = {
      .mesa  = _NEW_LINE | _NEW_POLYGON,
      .brw   = BRW_NEW_CONTEXT,
      .cache = CACHE_NEW_WM_PROG,
   },
   .emit = upload_wm_state,
};

static void
upload_ps_state(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   uint32_t dw3 = 0, dw6 = 0, dw7 = 0;

   /* BRW_NEW_PS_BINDING_TABLE */
   BEGIN_BATCH(2);
   OUT_BATCH(_3DSTATE_BINDING_TABLE_POINTERS_PS << 16 | (2 - 2));
   OUT_BATCH(brw->wm.base.bind_bo_offset);
   ADVANCE_BATCH();

   /* CACHE_NEW_SAMPLER */
   BEGIN_BATCH(2);
   OUT_BATCH(_3DSTATE_SAMPLER_STATE_POINTERS_PS << 16 | (2 - 2));
   OUT_BATCH(brw->wm.base.sampler_offset);
   ADVANCE_BATCH();

   /* CACHE_NEW_WM_PROG */
   gen8_upload_constant_state(brw, &brw->wm.base, true, _3DSTATE_CONSTANT_PS);

   /* Initialize the execution mask with VMask.  Otherwise, derivatives are
    * incorrect for subspans where some of the pixels are unlit.  We believe
    * the bit just didn't take effect in previous generations.
    */
   dw3 |= GEN7_PS_VECTOR_MASK_ENABLE;

   /* CACHE_NEW_SAMPLER */
   dw3 |=
      (ALIGN(brw->wm.base.sampler_count, 4) / 4) << GEN7_PS_SAMPLER_COUNT_SHIFT;

   /* CACHE_NEW_WM_PROG */
   dw3 |=
      ((brw->wm.prog_data->base.binding_table.size_bytes / 4) <<
       GEN7_PS_BINDING_TABLE_ENTRY_COUNT_SHIFT);

   /* Use ALT floating point mode for ARB fragment programs, because they
    * require 0^0 == 1.  Even though _CurrentFragmentProgram is used for
    * rendering, CurrentFragmentProgram is used for this check to
    * differentiate between the GLSL and non-GLSL cases.
    */
   if (ctx->Shader.CurrentProgram[MESA_SHADER_FRAGMENT] == NULL)
      dw3 |= GEN7_PS_FLOATING_POINT_MODE_ALT;

   dw6 |= (brw->max_wm_threads - 2) << HSW_PS_MAX_THREADS_SHIFT;

   /* CACHE_NEW_WM_PROG */
   if (brw->wm.prog_data->nr_params > 0)
      dw6 |= GEN7_PS_PUSH_CONSTANT_ENABLE;

   dw6 |= GEN7_PS_8_DISPATCH_ENABLE;
   if (brw->wm.prog_data->prog_offset_16)
      dw6 |= GEN7_PS_16_DISPATCH_ENABLE;

   dw7 |=
      brw->wm.prog_data->first_curbe_grf << GEN7_PS_DISPATCH_START_GRF_SHIFT_0 |
      brw->wm.prog_data->first_curbe_grf_16<< GEN7_PS_DISPATCH_START_GRF_SHIFT_2;

   BEGIN_BATCH(12);
   OUT_BATCH(_3DSTATE_PS << 16 | (12 - 2));
   OUT_BATCH(brw->wm.base.prog_offset);
   OUT_BATCH(0);
   OUT_BATCH(dw3);
   if (brw->wm.prog_data->total_scratch) {
      OUT_RELOC64(brw->wm.base.scratch_bo,
                  I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
                  ffs(brw->wm.prog_data->total_scratch) - 11);
   } else {
      OUT_BATCH(0);
      OUT_BATCH(0);
   }
   OUT_BATCH(dw6);
   OUT_BATCH(dw7);
   OUT_BATCH(0); /* kernel 1 pointer */
   OUT_BATCH(0);
   OUT_BATCH(brw->wm.base.prog_offset + brw->wm.prog_data->prog_offset_16);
   OUT_BATCH(0);
   ADVANCE_BATCH();
}

const struct brw_tracked_state gen8_ps_state = {
   .dirty = {
      .mesa  = _NEW_PROGRAM_CONSTANTS,
      .brw   = BRW_NEW_FRAGMENT_PROGRAM |
               BRW_NEW_PS_BINDING_TABLE |
               BRW_NEW_BATCH |
               BRW_NEW_PUSH_CONSTANT_ALLOCATION,
      .cache = CACHE_NEW_SAMPLER | CACHE_NEW_WM_PROG
   },
   .emit = upload_ps_state,
};

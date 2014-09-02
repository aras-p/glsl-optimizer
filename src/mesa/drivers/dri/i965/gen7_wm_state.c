/*
 * Copyright © 2011 Intel Corporation
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
#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"
#include "brw_util.h"
#include "brw_wm.h"
#include "program/program.h"
#include "program/prog_parameter.h"
#include "program/prog_statevars.h"
#include "intel_batchbuffer.h"

static void
upload_wm_state(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   const struct brw_fragment_program *fp =
      brw_fragment_program_const(brw->fragment_program);
   bool writes_depth = false;
   uint32_t dw1, dw2;

   /* _NEW_BUFFERS */
   bool multisampled_fbo = ctx->DrawBuffer->Visual.samples > 1;

   dw1 = dw2 = 0;
   dw1 |= GEN7_WM_STATISTICS_ENABLE;
   dw1 |= GEN7_WM_LINE_AA_WIDTH_1_0;
   dw1 |= GEN7_WM_LINE_END_CAP_AA_WIDTH_0_5;

   /* _NEW_LINE */
   if (ctx->Line.StippleFlag)
      dw1 |= GEN7_WM_LINE_STIPPLE_ENABLE;

   /* _NEW_POLYGON */
   if (ctx->Polygon.StippleFlag)
      dw1 |= GEN7_WM_POLYGON_STIPPLE_ENABLE;

   /* BRW_NEW_FRAGMENT_PROGRAM */
   if (fp->program.Base.InputsRead & VARYING_BIT_POS)
      dw1 |= GEN7_WM_USES_SOURCE_DEPTH | GEN7_WM_USES_SOURCE_W;
   if (fp->program.Base.OutputsWritten & BITFIELD64_BIT(FRAG_RESULT_DEPTH)) {
      writes_depth = fp->program.FragDepthLayout != FRAG_DEPTH_LAYOUT_UNCHANGED;

      switch (fp->program.FragDepthLayout) {
         case FRAG_DEPTH_LAYOUT_NONE:
         case FRAG_DEPTH_LAYOUT_ANY:
            dw1 |= GEN7_WM_PSCDEPTH_ON;
            break;
         case FRAG_DEPTH_LAYOUT_GREATER:
            dw1 |= GEN7_WM_PSCDEPTH_ON_GE;
            break;
         case FRAG_DEPTH_LAYOUT_LESS:
            dw1 |= GEN7_WM_PSCDEPTH_ON_LE;
            break;
         case FRAG_DEPTH_LAYOUT_UNCHANGED:
            break;
      }
   }
   /* CACHE_NEW_WM_PROG */
   dw1 |= brw->wm.prog_data->barycentric_interp_modes <<
      GEN7_WM_BARYCENTRIC_INTERPOLATION_MODE_SHIFT;

   /* _NEW_COLOR, _NEW_MULTISAMPLE */
   /* Enable if the pixel shader kernel generates and outputs oMask.
    */
   if (fp->program.UsesKill || ctx->Color.AlphaEnabled ||
       ctx->Multisample.SampleAlphaToCoverage ||
       brw->wm.prog_data->uses_omask) {
      dw1 |= GEN7_WM_KILL_ENABLE;
   }

   /* _NEW_BUFFERS | _NEW_COLOR */
   if (brw_color_buffer_write_enabled(brw) || writes_depth ||
       dw1 & GEN7_WM_KILL_ENABLE) {
      dw1 |= GEN7_WM_DISPATCH_ENABLE;
   }
   if (multisampled_fbo) {
      /* _NEW_MULTISAMPLE */
      if (ctx->Multisample.Enabled)
         dw1 |= GEN7_WM_MSRAST_ON_PATTERN;
      else
         dw1 |= GEN7_WM_MSRAST_OFF_PIXEL;

      if (_mesa_get_min_invocations_per_fragment(ctx, brw->fragment_program, false) > 1)
         dw2 |= GEN7_WM_MSDISPMODE_PERSAMPLE;
      else
         dw2 |= GEN7_WM_MSDISPMODE_PERPIXEL;
   } else {
      dw1 |= GEN7_WM_MSRAST_OFF_PIXEL;
      dw2 |= GEN7_WM_MSDISPMODE_PERSAMPLE;
   }

   if (fp->program.Base.SystemValuesRead & SYSTEM_BIT_SAMPLE_MASK_IN) {
      dw1 |= GEN7_WM_USES_INPUT_COVERAGE_MASK;
   }

   BEGIN_BATCH(3);
   OUT_BATCH(_3DSTATE_WM << 16 | (3 - 2));
   OUT_BATCH(dw1);
   OUT_BATCH(dw2);
   ADVANCE_BATCH();
}

const struct brw_tracked_state gen7_wm_state = {
   .dirty = {
      .mesa  = (_NEW_LINE | _NEW_POLYGON |
	        _NEW_COLOR | _NEW_BUFFERS |
                _NEW_MULTISAMPLE),
      .brw   = (BRW_NEW_FRAGMENT_PROGRAM |
		BRW_NEW_BATCH),
      .cache = CACHE_NEW_WM_PROG,
   },
   .emit = upload_wm_state,
};

static void
upload_ps_state(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   uint32_t dw2, dw4, dw5, ksp0, ksp2;
   const int max_threads_shift = brw->is_haswell ?
      HSW_PS_MAX_THREADS_SHIFT : IVB_PS_MAX_THREADS_SHIFT;

   dw2 = dw4 = dw5 = ksp2 = 0;

   dw2 |=
      (ALIGN(brw->wm.base.sampler_count, 4) / 4) << GEN7_PS_SAMPLER_COUNT_SHIFT;

   /* CACHE_NEW_WM_PROG */
   dw2 |= ((brw->wm.prog_data->base.binding_table.size_bytes / 4) <<
           GEN7_PS_BINDING_TABLE_ENTRY_COUNT_SHIFT);

   /* Use ALT floating point mode for ARB fragment programs, because they
    * require 0^0 == 1.  Even though _CurrentFragmentProgram is used for
    * rendering, CurrentProgram[MESA_SHADER_FRAGMENT] is used for this check
    * to differentiate between the GLSL and non-GLSL cases.
    */
   /* BRW_NEW_FRAGMENT_PROGRAM */
   if (ctx->_Shader->CurrentProgram[MESA_SHADER_FRAGMENT] == NULL)
      dw2 |= GEN7_PS_FLOATING_POINT_MODE_ALT;

   /* Haswell requires the sample mask to be set in this packet as well as
    * in 3DSTATE_SAMPLE_MASK; the values should match. */
   /* _NEW_BUFFERS, _NEW_MULTISAMPLE */
   if (brw->is_haswell)
      dw4 |= SET_FIELD(gen6_determine_sample_mask(brw), HSW_PS_SAMPLE_MASK);

   dw4 |= (brw->max_wm_threads - 1) << max_threads_shift;

   /* CACHE_NEW_WM_PROG */
   if (brw->wm.prog_data->base.nr_params > 0)
      dw4 |= GEN7_PS_PUSH_CONSTANT_ENABLE;

   /* From the IVB PRM, volume 2 part 1, page 287:
    * "This bit is inserted in the PS payload header and made available to
    * the DataPort (either via the message header or via header bypass) to
    * indicate that oMask data (one or two phases) is included in Render
    * Target Write messages. If present, the oMask data is used to mask off
    * samples."
    */
   if (brw->wm.prog_data->uses_omask)
      dw4 |= GEN7_PS_OMASK_TO_RENDER_TARGET;

   /* From the IVB PRM, volume 2 part 1, page 287:
    * "If the PS kernel does not need the Position XY Offsets to
    * compute a Position Value, then this field should be programmed
    * to POSOFFSET_NONE."
    * "SW Recommendation: If the PS kernel needs the Position Offsets
    * to compute a Position XY value, this field should match Position
    * ZW Interpolation Mode to ensure a consistent position.xyzw
    * computation."
    * We only require XY sample offsets. So, this recommendation doesn't
    * look useful at the moment. We might need this in future.
    */
   if (brw->wm.prog_data->uses_pos_offset)
      dw4 |= GEN7_PS_POSOFFSET_SAMPLE;
   else
      dw4 |= GEN7_PS_POSOFFSET_NONE;

   /* CACHE_NEW_WM_PROG | _NEW_COLOR
    *
    * The hardware wedges if you have this bit set but don't turn on any dual
    * source blend factors.
    */
   if (brw->wm.prog_data->dual_src_blend &&
       (ctx->Color.BlendEnabled & 1) &&
       ctx->Color.Blend[0]._UsesDualSrc) {
      dw4 |= GEN7_PS_DUAL_SOURCE_BLEND_ENABLE;
   }

   /* CACHE_NEW_WM_PROG */
   if (brw->wm.prog_data->num_varying_inputs != 0)
      dw4 |= GEN7_PS_ATTRIBUTE_ENABLE;

   /* In case of non 1x per sample shading, only one of SIMD8 and SIMD16
    * should be enabled. We do 'SIMD16 only' dispatch if a SIMD16 shader
    * is successfully compiled. In majority of the cases that bring us
    * better performance than 'SIMD8 only' dispatch.
    */
   int min_inv_per_frag =
      _mesa_get_min_invocations_per_fragment(ctx, brw->fragment_program, false);
   assert(min_inv_per_frag >= 1);

   if (brw->wm.prog_data->prog_offset_16 || brw->wm.prog_data->no_8) {
      dw4 |= GEN7_PS_16_DISPATCH_ENABLE;
      if (!brw->wm.prog_data->no_8 && min_inv_per_frag == 1) {
         dw4 |= GEN7_PS_8_DISPATCH_ENABLE;
         dw5 |= (brw->wm.prog_data->base.dispatch_grf_start_reg <<
                 GEN7_PS_DISPATCH_START_GRF_SHIFT_0);
         dw5 |= (brw->wm.prog_data->dispatch_grf_start_reg_16 <<
                 GEN7_PS_DISPATCH_START_GRF_SHIFT_2);
         ksp0 = brw->wm.base.prog_offset;
         ksp2 = brw->wm.base.prog_offset + brw->wm.prog_data->prog_offset_16;
      } else {
         dw5 |= (brw->wm.prog_data->dispatch_grf_start_reg_16 <<
                 GEN7_PS_DISPATCH_START_GRF_SHIFT_0);
         ksp0 = brw->wm.base.prog_offset + brw->wm.prog_data->prog_offset_16;
      }
   }
   else {
      dw4 |= GEN7_PS_8_DISPATCH_ENABLE;
      dw5 |= (brw->wm.prog_data->base.dispatch_grf_start_reg <<
              GEN7_PS_DISPATCH_START_GRF_SHIFT_0);
      ksp0 = brw->wm.base.prog_offset;
   }

   dw4 |= brw->wm.fast_clear_op;

   BEGIN_BATCH(8);
   OUT_BATCH(_3DSTATE_PS << 16 | (8 - 2));
   OUT_BATCH(ksp0);
   OUT_BATCH(dw2);
   if (brw->wm.prog_data->base.total_scratch) {
      OUT_RELOC(brw->wm.base.scratch_bo,
		I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
		ffs(brw->wm.prog_data->base.total_scratch) - 11);
   } else {
      OUT_BATCH(0);
   }
   OUT_BATCH(dw4);
   OUT_BATCH(dw5);
   OUT_BATCH(0); /* kernel 1 pointer */
   OUT_BATCH(ksp2);
   ADVANCE_BATCH();
}

const struct brw_tracked_state gen7_ps_state = {
   .dirty = {
      .mesa  = (_NEW_COLOR |
                _NEW_BUFFERS |
                _NEW_MULTISAMPLE),
      .brw   = (BRW_NEW_FRAGMENT_PROGRAM |
                BRW_NEW_BATCH),
      .cache = (CACHE_NEW_WM_PROG)
   },
   .emit = upload_ps_state,
};

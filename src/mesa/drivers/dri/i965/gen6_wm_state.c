/*
 * Copyright © 2009 Intel Corporation
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
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

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
gen6_upload_wm_push_constants(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   /* BRW_NEW_FRAGMENT_PROGRAM */
   const struct brw_fragment_program *fp =
      brw_fragment_program_const(brw->fragment_program);
   /* CACHE_NEW_WM_PROG */
   const struct brw_wm_prog_data *prog_data = brw->wm.prog_data;

   /* Updates the ParameterValues[i] pointers for all parameters of the
    * basic type of PROGRAM_STATE_VAR.
    */
   /* XXX: Should this happen somewhere before to get our state flag set? */
   _mesa_load_state_parameters(ctx, fp->program.Base.Parameters);

   if (prog_data->base.nr_params == 0) {
      brw->wm.base.push_const_size = 0;
   } else {
      float *constants;
      unsigned int i;

      constants = brw_state_batch(brw, AUB_TRACE_WM_CONSTANTS,
				  prog_data->base.nr_params * sizeof(float),
				  32, &brw->wm.base.push_const_offset);

      for (i = 0; i < prog_data->base.nr_params; i++) {
	 constants[i] = *prog_data->base.param[i];
      }

      if (0) {
	 fprintf(stderr, "WM constants:\n");
	 for (i = 0; i < prog_data->base.nr_params; i++) {
	    if ((i & 7) == 0)
	       fprintf(stderr, "g%d: ", prog_data->first_curbe_grf + i / 8);
	    fprintf(stderr, "%8f ", constants[i]);
	    if ((i & 7) == 7)
	       fprintf(stderr, "\n");
	 }
	 if ((i & 7) != 0)
	    fprintf(stderr, "\n");
	 fprintf(stderr, "\n");
      }

      brw->wm.base.push_const_size = ALIGN(prog_data->base.nr_params, 8) / 8;
   }

   if (brw->gen >= 7) {
      gen7_upload_constant_state(brw, &brw->wm.base, true,
                                 _3DSTATE_CONSTANT_PS);
   }
}

const struct brw_tracked_state gen6_wm_push_constants = {
   .dirty = {
      .mesa  = _NEW_PROGRAM_CONSTANTS,
      .brw   = (BRW_NEW_BATCH |
                BRW_NEW_FRAGMENT_PROGRAM |
                BRW_NEW_PUSH_CONSTANT_ALLOCATION),
      .cache = CACHE_NEW_WM_PROG,
   },
   .emit = gen6_upload_wm_push_constants,
};

static void
upload_wm_state(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   const struct brw_fragment_program *fp =
      brw_fragment_program_const(brw->fragment_program);
   uint32_t dw2, dw4, dw5, dw6;

   /* _NEW_BUFFERS */
   bool multisampled_fbo = ctx->DrawBuffer->Visual.samples > 1;

    /* CACHE_NEW_WM_PROG */
   if (brw->wm.prog_data->base.nr_params == 0) {
      /* Disable the push constant buffers. */
      BEGIN_BATCH(5);
      OUT_BATCH(_3DSTATE_CONSTANT_PS << 16 | (5 - 2));
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   } else {
      BEGIN_BATCH(5);
      OUT_BATCH(_3DSTATE_CONSTANT_PS << 16 |
		GEN6_CONSTANT_BUFFER_0_ENABLE |
		(5 - 2));
      /* Pointer to the WM constant buffer.  Covered by the set of
       * state flags from gen6_upload_wm_push_constants.
       */
      OUT_BATCH(brw->wm.base.push_const_offset +
		brw->wm.base.push_const_size - 1);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }

   dw2 = dw4 = dw5 = dw6 = 0;
   dw4 |= GEN6_WM_STATISTICS_ENABLE;
   dw5 |= GEN6_WM_LINE_AA_WIDTH_1_0;
   dw5 |= GEN6_WM_LINE_END_CAP_AA_WIDTH_0_5;

   /* Use ALT floating point mode for ARB fragment programs, because they
    * require 0^0 == 1.  Even though _CurrentFragmentProgram is used for
    * rendering, CurrentProgram[MESA_SHADER_FRAGMENT] is used for this check
    * to differentiate between the GLSL and non-GLSL cases.
    */
   if (ctx->_Shader->CurrentProgram[MESA_SHADER_FRAGMENT] == NULL)
      dw2 |= GEN6_WM_FLOATING_POINT_MODE_ALT;

   dw2 |= (ALIGN(brw->wm.base.sampler_count, 4) / 4) <<
           GEN6_WM_SAMPLER_COUNT_SHIFT;

   /* CACHE_NEW_WM_PROG */
   dw2 |= ((brw->wm.prog_data->base.binding_table.size_bytes / 4) <<
           GEN6_WM_BINDING_TABLE_ENTRY_COUNT_SHIFT);

   dw5 |= (brw->max_wm_threads - 1) << GEN6_WM_MAX_THREADS_SHIFT;

   /* CACHE_NEW_WM_PROG */

   /* In case of non 1x per sample shading, only one of SIMD8 and SIMD16
    * should be enabled. We do 'SIMD16 only' dispatch if a SIMD16 shader
    * is successfully compiled. In majority of the cases that bring us
    * better performance than 'SIMD8 only' dispatch.
    */
   int min_inv_per_frag =
      _mesa_get_min_invocations_per_fragment(ctx, brw->fragment_program, false);
   assert(min_inv_per_frag >= 1);

   if (brw->wm.prog_data->prog_offset_16) {
      dw5 |= GEN6_WM_16_DISPATCH_ENABLE;

      if (min_inv_per_frag == 1) {
         dw5 |= GEN6_WM_8_DISPATCH_ENABLE;
         dw4 |= (brw->wm.prog_data->first_curbe_grf <<
                 GEN6_WM_DISPATCH_START_GRF_SHIFT_0);
         dw4 |= (brw->wm.prog_data->first_curbe_grf_16 <<
                 GEN6_WM_DISPATCH_START_GRF_SHIFT_2);
      } else
         dw4 |= (brw->wm.prog_data->first_curbe_grf_16 <<
                GEN6_WM_DISPATCH_START_GRF_SHIFT_0);
   }
   else {
      dw5 |= GEN6_WM_8_DISPATCH_ENABLE;
      dw4 |= (brw->wm.prog_data->first_curbe_grf <<
              GEN6_WM_DISPATCH_START_GRF_SHIFT_0);
   }

   /* CACHE_NEW_WM_PROG | _NEW_COLOR */
   if (brw->wm.prog_data->dual_src_blend &&
       (ctx->Color.BlendEnabled & 1) &&
       ctx->Color.Blend[0]._UsesDualSrc) {
      dw5 |= GEN6_WM_DUAL_SOURCE_BLEND_ENABLE;
   }

   /* _NEW_LINE */
   if (ctx->Line.StippleFlag)
      dw5 |= GEN6_WM_LINE_STIPPLE_ENABLE;

   /* _NEW_POLYGON */
   if (ctx->Polygon.StippleFlag)
      dw5 |= GEN6_WM_POLYGON_STIPPLE_ENABLE;

   /* BRW_NEW_FRAGMENT_PROGRAM */
   if (fp->program.Base.InputsRead & VARYING_BIT_POS)
      dw5 |= GEN6_WM_USES_SOURCE_DEPTH | GEN6_WM_USES_SOURCE_W;
   if (fp->program.Base.OutputsWritten & BITFIELD64_BIT(FRAG_RESULT_DEPTH))
      dw5 |= GEN6_WM_COMPUTED_DEPTH;
   /* CACHE_NEW_WM_PROG */
   dw6 |= brw->wm.prog_data->barycentric_interp_modes <<
      GEN6_WM_BARYCENTRIC_INTERPOLATION_MODE_SHIFT;

   /* _NEW_COLOR, _NEW_MULTISAMPLE */
   if (fp->program.UsesKill || ctx->Color.AlphaEnabled ||
       ctx->Multisample.SampleAlphaToCoverage ||
       brw->wm.prog_data->uses_omask)
      dw5 |= GEN6_WM_KILL_ENABLE;

   /* _NEW_BUFFERS | _NEW_COLOR */
   if (brw_color_buffer_write_enabled(brw) ||
       dw5 & (GEN6_WM_KILL_ENABLE | GEN6_WM_COMPUTED_DEPTH)) {
      dw5 |= GEN6_WM_DISPATCH_ENABLE;
   }

   /* From the SNB PRM, volume 2 part 1, page 278:
    * "This bit is inserted in the PS payload header and made available to
    * the DataPort (either via the message header or via header bypass) to
    * indicate that oMask data (one or two phases) is included in Render
    * Target Write messages. If present, the oMask data is used to mask off
    * samples."
    */
    if(brw->wm.prog_data->uses_omask)
      dw5 |= GEN6_WM_OMASK_TO_RENDER_TARGET;

   /* CACHE_NEW_WM_PROG */
   dw6 |= brw->wm.prog_data->num_varying_inputs <<
      GEN6_WM_NUM_SF_OUTPUTS_SHIFT;
   if (multisampled_fbo) {
      /* _NEW_MULTISAMPLE */
      if (ctx->Multisample.Enabled)
         dw6 |= GEN6_WM_MSRAST_ON_PATTERN;
      else
         dw6 |= GEN6_WM_MSRAST_OFF_PIXEL;

      if (min_inv_per_frag > 1)
         dw6 |= GEN6_WM_MSDISPMODE_PERSAMPLE;
      else {
         dw6 |= GEN6_WM_MSDISPMODE_PERPIXEL;

         /* From the Sandy Bridge PRM, Vol 2 part 1, 7.7.1 ("Pixel Grouping
          * (Dispatch Size) Control"), p.334:
          *
          *     Note: in the table below, the Valid column indicates which
          *     products that combination is supported on. Combinations of
          *     dispatch enables not listed in the table are not available on
          *     any product.
          *
          *     A: Valid on all products
          *
          *     B: Not valid on [DevSNB] if 4x PERPIXEL mode with pixel shader
          *     computed depth.
          *
          *     D: Valid on all products, except when in non-1x PERSAMPLE mode
          *     (applies to [DevSNB+] only). Not valid on [DevSNB] if 4x
          *     PERPIXEL mode with pixel shader computed depth.
          *
          *     E: Not valid on [DevSNB] if 4x PERPIXEL mode with pixel shader
          *     computed depth.
          *
          *     F: Valid on all products, except not valid on [DevSNB] if 4x
          *     PERPIXEL mode with pixel shader computed depth.
          *
          * In the table that follows, the only entry with "A" in the Valid
          * column is the entry where only 8 pixel dispatch is enabled.
          * Therefore, when we are in PERPIXEL mode with pixel shader computed
          * depth, we need to disable SIMD16 dispatch.
          */
         if (dw5 & GEN6_WM_COMPUTED_DEPTH)
            dw5 &= ~GEN6_WM_16_DISPATCH_ENABLE;
      }
   } else {
      dw6 |= GEN6_WM_MSRAST_OFF_PIXEL;
      dw6 |= GEN6_WM_MSDISPMODE_PERSAMPLE;
   }

   /* From the SNB PRM, volume 2 part 1, page 281:
    * "If the PS kernel does not need the Position XY Offsets
    * to compute a Position XY value, then this field should be
    * programmed to POSOFFSET_NONE."
    *
    * "SW Recommendation: If the PS kernel needs the Position Offsets
    * to compute a Position XY value, this field should match Position
    * ZW Interpolation Mode to ensure a consistent position.xyzw
    * computation."
    * We only require XY sample offsets. So, this recommendation doesn't
    * look useful at the moment. We might need this in future.
    */
   if (brw->wm.prog_data->uses_pos_offset)
      dw6 |= GEN6_WM_POSOFFSET_SAMPLE;
   else
      dw6 |= GEN6_WM_POSOFFSET_NONE;

   BEGIN_BATCH(9);
   OUT_BATCH(_3DSTATE_WM << 16 | (9 - 2));
   if (brw->wm.prog_data->prog_offset_16 && min_inv_per_frag > 1)
      OUT_BATCH(brw->wm.base.prog_offset + brw->wm.prog_data->prog_offset_16);
   else
      OUT_BATCH(brw->wm.base.prog_offset);
   OUT_BATCH(dw2);
   if (brw->wm.prog_data->total_scratch) {
      OUT_RELOC(brw->wm.base.scratch_bo,
                I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
		ffs(brw->wm.prog_data->total_scratch) - 11);
   } else {
      OUT_BATCH(0);
   }
   OUT_BATCH(dw4);
   OUT_BATCH(dw5);
   OUT_BATCH(dw6);
   OUT_BATCH(0); /* kernel 1 pointer */
   /* kernel 2 pointer */
   OUT_BATCH(brw->wm.base.prog_offset + brw->wm.prog_data->prog_offset_16);
   ADVANCE_BATCH();
}

const struct brw_tracked_state gen6_wm_state = {
   .dirty = {
      .mesa  = (_NEW_LINE |
		_NEW_COLOR |
		_NEW_BUFFERS |
		_NEW_PROGRAM_CONSTANTS |
		_NEW_POLYGON |
                _NEW_MULTISAMPLE),
      .brw   = (BRW_NEW_FRAGMENT_PROGRAM |
		BRW_NEW_BATCH |
                BRW_NEW_PUSH_CONSTANT_ALLOCATION),
      .cache = (CACHE_NEW_WM_PROG)
   },
   .emit = upload_wm_state,
};

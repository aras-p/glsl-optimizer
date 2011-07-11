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
#include "program/prog_parameter.h"
#include "program/prog_statevars.h"
#include "intel_batchbuffer.h"

static void
gen7_prepare_wm_constants(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;
   /* BRW_NEW_FRAGMENT_PROGRAM */
   const struct brw_fragment_program *fp =
      brw_fragment_program_const(brw->fragment_program);

   /* Updates the ParameterValues[i] pointers for all parameters of the
    * basic type of PROGRAM_STATE_VAR.
    */
   /* XXX: Should this happen somewhere before to get our state flag set? */
   _mesa_load_state_parameters(ctx, fp->program.Base.Parameters);

   /* CACHE_NEW_WM_PROG */
   if (brw->wm.prog_data->nr_params != 0) {
      float *constants;
      unsigned int i;

      constants = brw_state_batch(brw,
				  brw->wm.prog_data->nr_params *
				  sizeof(float),
				  32, &brw->wm.push_const_offset);

      for (i = 0; i < brw->wm.prog_data->nr_params; i++) {
	 constants[i] = convert_param(brw->wm.prog_data->param_convert[i],
				      *brw->wm.prog_data->param[i]);
      }

      if (0) {
	 printf("WM constants:\n");
	 for (i = 0; i < brw->wm.prog_data->nr_params; i++) {
	    if ((i & 7) == 0)
	       printf("g%d: ", brw->wm.prog_data->first_curbe_grf + i / 8);
	    printf("%8f ", constants[i]);
	    if ((i & 7) == 7)
	       printf("\n");
	 }
	 if ((i & 7) != 0)
	    printf("\n");
	 printf("\n");
      }
   }
}

const struct brw_tracked_state gen7_wm_constants = {
   .dirty = {
      .mesa  = _NEW_PROGRAM_CONSTANTS,
      .brw   = (BRW_NEW_BATCH | BRW_NEW_FRAGMENT_PROGRAM),
      .cache = CACHE_NEW_WM_PROG,
   },
   .prepare = gen7_prepare_wm_constants,
};

static void
upload_wm_state(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;
   const struct brw_fragment_program *fp =
      brw_fragment_program_const(brw->fragment_program);
   bool writes_depth = false;
   uint32_t dw1;

   dw1 = 0;
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
   if (fp->program.Base.InputsRead & (1 << FRAG_ATTRIB_WPOS))
      dw1 |= GEN7_WM_USES_SOURCE_DEPTH | GEN7_WM_USES_SOURCE_W;
   if (fp->program.Base.OutputsWritten & BITFIELD64_BIT(FRAG_RESULT_DEPTH)) {
      writes_depth = true;
      dw1 |= GEN7_WM_PSCDEPTH_ON;
   }

   /* _NEW_COLOR */
   if (fp->program.UsesKill || ctx->Color.AlphaEnabled)
      dw1 |= GEN7_WM_KILL_ENABLE;

   /* _NEW_BUFFERS */
   if (brw_color_buffer_write_enabled(brw) || writes_depth ||
       dw1 & GEN7_WM_KILL_ENABLE) {
      dw1 |= GEN7_WM_DISPATCH_ENABLE;
   }

   dw1 |= GEN7_WM_PERSPECTIVE_PIXEL_BARYCENTRIC;

   BEGIN_BATCH(3);
   OUT_BATCH(_3DSTATE_WM << 16 | (3 - 2));
   OUT_BATCH(dw1);
   OUT_BATCH(0);
   ADVANCE_BATCH();
}

const struct brw_tracked_state gen7_wm_state = {
   .dirty = {
      .mesa  = (_NEW_LINE | _NEW_POLYGON |
	        _NEW_COLOR | _NEW_BUFFERS),
      .brw   = (BRW_NEW_FRAGMENT_PROGRAM |
		BRW_NEW_URB_FENCE |
		BRW_NEW_BATCH),
      .cache = 0,
   },
   .emit = upload_wm_state,
};

static void
upload_ps_state(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   uint32_t dw2, dw4, dw5;

   BEGIN_BATCH(2);
   OUT_BATCH(_3DSTATE_BINDING_TABLE_POINTERS_PS << 16 | (2 - 2));
   OUT_BATCH(brw->wm.bind_bo_offset);
   ADVANCE_BATCH();

   /* CACHE_NEW_SAMPLER */
   BEGIN_BATCH(2);
   OUT_BATCH(_3DSTATE_SAMPLER_STATE_POINTERS_PS << 16 | (2 - 2));
   OUT_BATCH(brw->wm.sampler_offset);
   ADVANCE_BATCH();

   /* CACHE_NEW_WM_PROG */
   if (brw->wm.prog_data->nr_params == 0) {
      /* Disable the push constant buffers. */
      BEGIN_BATCH(7);
      OUT_BATCH(_3DSTATE_CONSTANT_PS << 16 | (7 - 2));
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   } else {
      BEGIN_BATCH(7);
      OUT_BATCH(_3DSTATE_CONSTANT_PS << 16 | (7 - 2));

      OUT_BATCH(ALIGN(brw->wm.prog_data->nr_params,
		      brw->wm.prog_data->dispatch_width) / 8);
      OUT_BATCH(0);
      /* Pointer to the WM constant buffer.  Covered by the set of
       * state flags from gen7_prepare_wm_constants
       */
      OUT_BATCH(brw->wm.push_const_offset);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }

   dw2 = dw4 = dw5 = 0;

   dw2 |= (ALIGN(brw->wm.sampler_count, 4) / 4) << GEN7_PS_SAMPLER_COUNT_SHIFT;

   /* BRW_NEW_NR_WM_SURFACES */
   dw2 |= brw->wm.nr_surfaces << GEN7_PS_BINDING_TABLE_ENTRY_COUNT_SHIFT;

   /* OpenGL non-ieee floating point mode */
   dw2 |= GEN7_PS_FLOATING_POINT_MODE_ALT;

   /* CACHE_NEW_SAMPLER */
   dw4 |= (brw->wm_max_threads - 1) << GEN7_PS_MAX_THREADS_SHIFT;

   /* CACHE_NEW_WM_PROG */
   if (brw->wm.prog_data->nr_params > 0)
      dw4 |= GEN7_PS_PUSH_CONSTANT_ENABLE;

   /* BRW_NEW_FRAGMENT_PROGRAM */
   if (brw->fragment_program->Base.InputsRead != 0)
      dw4 |= GEN7_PS_ATTRIBUTE_ENABLE;

   if (brw->wm.prog_data->dispatch_width == 8)
      dw4 |= GEN7_PS_8_DISPATCH_ENABLE;
   else
      dw4 |= GEN7_PS_16_DISPATCH_ENABLE;

   /* BRW_NEW_CURBE_OFFSETS */
   dw5 |= (brw->wm.prog_data->first_curbe_grf <<
	   GEN7_PS_DISPATCH_START_GRF_SHIFT_0);

   BEGIN_BATCH(8);
   OUT_BATCH(_3DSTATE_PS << 16 | (8 - 2));
   OUT_BATCH(brw->wm.prog_offset);
   OUT_BATCH(dw2);
   OUT_BATCH(0); /* scratch space base offset */
   OUT_BATCH(dw4);
   OUT_BATCH(dw5);
   OUT_BATCH(0); /* kernel 1 pointer */
   OUT_BATCH(brw->wm.prog_offset + brw->wm.prog_data->prog_offset_16);
   ADVANCE_BATCH();
}

const struct brw_tracked_state gen7_ps_state = {
   .dirty = {
      .mesa  = _NEW_PROGRAM_CONSTANTS,
      .brw   = (BRW_NEW_CURBE_OFFSETS |
		BRW_NEW_FRAGMENT_PROGRAM |
                BRW_NEW_NR_WM_SURFACES |
		BRW_NEW_PS_BINDING_TABLE |
		BRW_NEW_URB_FENCE |
		BRW_NEW_BATCH),
      .cache = (CACHE_NEW_SAMPLER |
		CACHE_NEW_WM_PROG)
   },
   .emit = upload_ps_state,
};

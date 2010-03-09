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
#include "shader/prog_parameter.h"
#include "shader/prog_statevars.h"
#include "intel_batchbuffer.h"

static void
upload_wm_state(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   GLcontext *ctx = &intel->ctx;
   const struct brw_fragment_program *fp =
      brw_fragment_program_const(brw->fragment_program);
   unsigned int nr_params = fp->program.Base.Parameters->NumParameters;
   drm_intel_bo *constant_bo;
   int i;
   uint32_t dw2, dw4, dw5, dw6;

   if (fp->use_const_buffer || nr_params == 0) {
      /* Disable the push constant buffers. */
      BEGIN_BATCH(5);
      OUT_BATCH(CMD_3D_CONSTANT_PS_STATE << 16 | (5 - 2));
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   } else {
      /* Updates the ParamaterValues[i] pointers for all parameters of the
       * basic type of PROGRAM_STATE_VAR.
       */
      _mesa_load_state_parameters(ctx, fp->program.Base.Parameters);

      constant_bo = drm_intel_bo_alloc(intel->bufmgr, "WM constant_bo",
				       nr_params * 4 * sizeof(float),
				       4096);
      drm_intel_gem_bo_map_gtt(constant_bo);
      for (i = 0; i < nr_params; i++) {
	 memcpy((char *)constant_bo->virtual + i * 4 * sizeof(float),
		fp->program.Base.Parameters->ParameterValues[i],
		4 * sizeof(float));
      }
      drm_intel_gem_bo_unmap_gtt(constant_bo);

      BEGIN_BATCH(5);
      OUT_BATCH(CMD_3D_CONSTANT_PS_STATE << 16 |
		GEN6_CONSTANT_BUFFER_0_ENABLE |
		(5 - 2));
      OUT_RELOC(constant_bo,
		I915_GEM_DOMAIN_RENDER, 0, /* XXX: bad domain */
		ALIGN(nr_params, 2) / 2 - 1);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();

      drm_intel_bo_unreference(constant_bo);
   }

   intel_batchbuffer_emit_mi_flush(intel->batch);

   dw2 = dw4 = dw5 = dw6 = 0;
   dw4 |= GEN6_WM_STATISTICS_ENABLE;
   dw5 |= GEN6_WM_LINE_AA_WIDTH_1_0;
   dw5 |= GEN6_WM_LINE_END_CAP_AA_WIDTH_0_5;

   /* BRW_NEW_NR_SURFACES */
   dw2 |= brw->wm.nr_surfaces << GEN6_WM_BINDING_TABLE_ENTRY_COUNT_SHIFT;

   /* CACHE_NEW_SAMPLER */
   dw2 |= (ALIGN(brw->wm.sampler_count, 4) / 4) << GEN6_WM_SAMPLER_COUNT_SHIFT;
   dw4 |= (1 << GEN6_WM_DISPATCH_START_GRF_SHIFT_0);

   dw5 |= (40 - 1) << GEN6_WM_MAX_THREADS_SHIFT;
   dw5 |= GEN6_WM_DISPATCH_ENABLE;

   /* BRW_NEW_FRAGMENT_PROGRAM */
   if (fp->isGLSL)
      dw5 |= GEN6_WM_8_DISPATCH_ENABLE;
   else
      dw5 |= GEN6_WM_16_DISPATCH_ENABLE;

   /* _NEW_LINE */
   if (ctx->Line.StippleFlag)
      dw5 |= GEN6_WM_LINE_STIPPLE_ENABLE;

   /* _NEW_POLYGONSTIPPLE */
   if (ctx->Polygon.StippleFlag)
      dw5 |= GEN6_WM_POLYGON_STIPPLE_ENABLE;

   /* BRW_NEW_FRAGMENT_PROGRAM */
   if (fp->program.Base.InputsRead & (1 << FRAG_ATTRIB_WPOS))
      dw5 |= GEN6_WM_USES_SOURCE_DEPTH | GEN6_WM_USES_SOURCE_W;
   if (fp->program.Base.OutputsWritten & BITFIELD64_BIT(FRAG_RESULT_DEPTH))
      dw5 |= GEN6_WM_COMPUTED_DEPTH;

   /* _NEW_COLOR */
   if (fp->program.UsesKill || ctx->Color.AlphaEnabled)
      dw5 |= GEN6_WM_KILL_ENABLE;

   /* This should probably be FS inputs read */
   dw6 |= brw_count_bits(brw->vs.prog_data->outputs_written) <<
      GEN6_WM_NUM_SF_OUTPUTS_SHIFT;

   BEGIN_BATCH(9);
   OUT_BATCH(CMD_3D_WM_STATE << 16 | (9 - 2));
   OUT_RELOC(brw->wm.prog_bo, I915_GEM_DOMAIN_INSTRUCTION, 0, 0);
   OUT_BATCH(dw2);
   OUT_BATCH(0); /* scratch space base offset */
   OUT_BATCH(dw4);
   OUT_BATCH(dw5);
   OUT_BATCH(dw6);
   OUT_BATCH(0); /* kernel 1 pointer */
   OUT_BATCH(0); /* kernel 2 pointer */
   ADVANCE_BATCH();

   intel_batchbuffer_emit_mi_flush(intel->batch);
}

const struct brw_tracked_state gen6_wm_state = {
   .dirty = {
      .mesa  = _NEW_LINE | _NEW_POLYGONSTIPPLE | _NEW_COLOR,
      .brw   = (BRW_NEW_CURBE_OFFSETS |
		BRW_NEW_FRAGMENT_PROGRAM |
                BRW_NEW_NR_WM_SURFACES |
		BRW_NEW_URB_FENCE |
		BRW_NEW_BATCH),
      .cache = CACHE_NEW_SAMPLER
   },
   .emit = upload_wm_state,
};

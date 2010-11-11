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
#include "program/prog_parameter.h"
#include "program/prog_statevars.h"
#include "intel_batchbuffer.h"

static void
upload_vs_state(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;
   const struct brw_vertex_program *vp =
      brw_vertex_program_const(brw->vertex_program);
   unsigned int nr_params = brw->vs.prog_data->nr_params / 4;
   drm_intel_bo *constant_bo;
   int i;

   if (brw->vs.prog_data->nr_params == 0 && !ctx->Transform.ClipPlanesEnabled) {
      /* Disable the push constant buffers. */
      BEGIN_BATCH(5);
      OUT_BATCH(CMD_3D_CONSTANT_VS_STATE << 16 | (5 - 2));
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   } else {
      int params_uploaded = 0;
      float *param;

      if (brw->vertex_program->IsNVProgram)
	 _mesa_load_tracked_matrices(ctx);

      /* Updates the ParamaterValues[i] pointers for all parameters of the
       * basic type of PROGRAM_STATE_VAR.
       */
      _mesa_load_state_parameters(ctx, vp->program.Base.Parameters);

      constant_bo = drm_intel_bo_alloc(intel->bufmgr, "VS constant_bo",
				       (MAX_CLIP_PLANES + nr_params) *
				       4 * sizeof(float),
				       4096);
      drm_intel_gem_bo_map_gtt(constant_bo);
      param = constant_bo->virtual;

      /* This should be loaded like any other param, but it's ad-hoc
       * until we redo the VS backend.
       */
      for (i = 0; i < MAX_CLIP_PLANES; i++) {
	 if (ctx->Transform.ClipPlanesEnabled & (1 << i)) {
	    memcpy(param, ctx->Transform._ClipUserPlane[i], 4 * sizeof(float));
	    param += 4;
	    params_uploaded++;
	 }
      }
      /* Align to a reg for convenience for brw_vs_emit.c */
      if (params_uploaded & 1) {
	 param += 4;
	 params_uploaded++;
      }

      if (vp->use_const_buffer) {
	 for (i = 0; i < vp->program.Base.Parameters->NumParameters; i++) {
	    if (brw->vs.constant_map[i] != -1) {
	       memcpy(param + brw->vs.constant_map[i] * 4,
		      vp->program.Base.Parameters->ParameterValues[i],
		      4 * sizeof(float));
	       params_uploaded++;
	    }
	 }
      } else {
	 for (i = 0; i < nr_params; i++) {
	    memcpy(param, vp->program.Base.Parameters->ParameterValues[i],
		   4 * sizeof(float));
	    param += 4;
	    params_uploaded++;
	 }
      }

      if (0) {
	 printf("VS constant buffer:\n");
	 for (i = 0; i < params_uploaded; i++) {
	    float *buf = (float *)constant_bo->virtual + i * 4;
	    printf("%d: %f %f %f %f\n",
		   i, buf[0], buf[1], buf[2], buf[3]);
	 }
      }

      drm_intel_gem_bo_unmap_gtt(constant_bo);

      BEGIN_BATCH(5);
      OUT_BATCH(CMD_3D_CONSTANT_VS_STATE << 16 |
		GEN6_CONSTANT_BUFFER_0_ENABLE |
		(5 - 2));
      OUT_RELOC(constant_bo,
		I915_GEM_DOMAIN_RENDER, 0, /* XXX: bad domain */
		ALIGN(params_uploaded, 2) / 2 - 1);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();

      drm_intel_bo_unreference(constant_bo);
   }

   BEGIN_BATCH(6);
   OUT_BATCH(CMD_3D_VS_STATE << 16 | (6 - 2));
   OUT_RELOC(brw->vs.prog_bo, I915_GEM_DOMAIN_INSTRUCTION, 0, 0);
   OUT_BATCH((0 << GEN6_VS_SAMPLER_COUNT_SHIFT) |
	     (brw->vs.nr_surfaces << GEN6_VS_BINDING_TABLE_ENTRY_COUNT_SHIFT));
   OUT_BATCH(0); /* scratch space base offset */
   OUT_BATCH((1 << GEN6_VS_DISPATCH_START_GRF_SHIFT) |
	     (brw->vs.prog_data->urb_read_length << GEN6_VS_URB_READ_LENGTH_SHIFT) |
	     (0 << GEN6_VS_URB_ENTRY_READ_OFFSET_SHIFT));
   OUT_BATCH((0 << GEN6_VS_MAX_THREADS_SHIFT) |
	     GEN6_VS_STATISTICS_ENABLE |
	     GEN6_VS_ENABLE);
   ADVANCE_BATCH();
}

const struct brw_tracked_state gen6_vs_state = {
   .dirty = {
      .mesa  = _NEW_TRANSFORM | _NEW_PROGRAM_CONSTANTS,
      .brw   = (BRW_NEW_CURBE_OFFSETS |
                BRW_NEW_NR_VS_SURFACES |
		BRW_NEW_URB_FENCE |
		BRW_NEW_CONTEXT),
      .cache = CACHE_NEW_VS_PROG
   },
   .emit = upload_vs_state,
};

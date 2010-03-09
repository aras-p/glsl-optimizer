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
upload_vs_state(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   GLcontext *ctx = &intel->ctx;
   const struct brw_vertex_program *vp =
      brw_vertex_program_const(brw->vertex_program);
   unsigned int nr_params = vp->program.Base.Parameters->NumParameters;
   drm_intel_bo *constant_bo;
   int i;

   if (vp->use_const_buffer || nr_params == 0) {
      /* Disable the push constant buffers. */
      BEGIN_BATCH(5);
      OUT_BATCH(CMD_3D_CONSTANT_VS_STATE << 16 | (5 - 2));
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   } else {
      if (brw->vertex_program->IsNVProgram)
	 _mesa_load_tracked_matrices(ctx);

      /* Updates the ParamaterValues[i] pointers for all parameters of the
       * basic type of PROGRAM_STATE_VAR.
       */
      _mesa_load_state_parameters(ctx, vp->program.Base.Parameters);

      constant_bo = drm_intel_bo_alloc(intel->bufmgr, "VS constant_bo",
				       nr_params * 4 * sizeof(float),
				       4096);
      drm_intel_gem_bo_map_gtt(constant_bo);
      for (i = 0; i < nr_params; i++) {
	 memcpy((char *)constant_bo->virtual + i * 4 * sizeof(float),
		vp->program.Base.Parameters->ParameterValues[i],
		4 * sizeof(float));
      }
      drm_intel_gem_bo_unmap_gtt(constant_bo);

      BEGIN_BATCH(5);
      OUT_BATCH(CMD_3D_CONSTANT_VS_STATE << 16 |
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
	     GEN6_VS_STATISTICS_ENABLE);
   ADVANCE_BATCH();

   intel_batchbuffer_emit_mi_flush(intel->batch);
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

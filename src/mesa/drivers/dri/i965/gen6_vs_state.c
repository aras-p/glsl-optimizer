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

void
gen6_upload_vec4_push_constants(struct brw_context *brw,
                                const struct gl_program *prog,
                                const struct brw_vec4_prog_data *prog_data,
                                struct brw_stage_state *stage_state,
                                enum state_struct_type type)
{
   struct gl_context *ctx = &brw->ctx;

   /* Updates the ParamaterValues[i] pointers for all parameters of the
    * basic type of PROGRAM_STATE_VAR.
    */
   /* XXX: Should this happen somewhere before to get our state flag set? */
   _mesa_load_state_parameters(ctx, prog->Parameters);

   if (prog_data->base.nr_params == 0) {
      stage_state->push_const_size = 0;
   } else {
      int params_uploaded;
      float *param;
      int i;

      param = brw_state_batch(brw, type,
			      prog_data->base.nr_params * sizeof(float),
			      32, &stage_state->push_const_offset);

      /* _NEW_PROGRAM_CONSTANTS
       *
       * Also _NEW_TRANSFORM -- we may reference clip planes other than as a
       * side effect of dereferencing uniforms, so _NEW_PROGRAM_CONSTANTS
       * wouldn't be set for them.
      */
      for (i = 0; i < prog_data->base.nr_params; i++) {
         param[i] = *prog_data->base.param[i];
      }
      params_uploaded = prog_data->base.nr_params / 4;

      if (0) {
	 fprintf(stderr, "Constant buffer:\n");
	 for (i = 0; i < params_uploaded; i++) {
	    float *buf = param + i * 4;
	    fprintf(stderr, "%d: %f %f %f %f\n",
                    i, buf[0], buf[1], buf[2], buf[3]);
	 }
      }

      stage_state->push_const_size = (params_uploaded + 1) / 2;
      /* We can only push 32 registers of constants at a time. */
      assert(stage_state->push_const_size <= 32);
   }
}

static void
gen6_upload_vs_push_constants(struct brw_context *brw)
{
   struct brw_stage_state *stage_state = &brw->vs.base;

   /* _BRW_NEW_VERTEX_PROGRAM */
   const struct brw_vertex_program *vp =
      brw_vertex_program_const(brw->vertex_program);
   /* CACHE_NEW_VS_PROG */
   const struct brw_vec4_prog_data *prog_data = &brw->vs.prog_data->base;

   gen6_upload_vec4_push_constants(brw, &vp->program.Base, prog_data,
                                   stage_state, AUB_TRACE_VS_CONSTANTS);

   if (brw->gen >= 7) {
      if (brw->gen == 7 && !brw->is_haswell)
         gen7_emit_vs_workaround_flush(brw);

      gen7_upload_constant_state(brw, stage_state, true /* active */,
                                 _3DSTATE_CONSTANT_VS);
   }
}

const struct brw_tracked_state gen6_vs_push_constants = {
   .dirty = {
      .mesa  = _NEW_TRANSFORM | _NEW_PROGRAM_CONSTANTS,
      .brw   = (BRW_NEW_BATCH |
                BRW_NEW_VERTEX_PROGRAM |
                BRW_NEW_PUSH_CONSTANT_ALLOCATION),
      .cache = CACHE_NEW_VS_PROG,
   },
   .emit = gen6_upload_vs_push_constants,
};

static void
upload_vs_state(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   const struct brw_stage_state *stage_state = &brw->vs.base;
   uint32_t floating_point_mode = 0;

   /* From the BSpec, 3D Pipeline > Geometry > Vertex Shader > State,
    * 3DSTATE_VS, Dword 5.0 "VS Function Enable":
    *
    *   [DevSNB] A pipeline flush must be programmed prior to a 3DSTATE_VS
    *   command that causes the VS Function Enable to toggle. Pipeline
    *   flush can be executed by sending a PIPE_CONTROL command with CS
    *   stall bit set and a post sync operation.
    *
    * Although we don't disable the VS during normal drawing, BLORP sometimes
    * disables it.  To be safe, do the flush here just in case.
    */
   intel_emit_post_sync_nonzero_flush(brw);

   if (stage_state->push_const_size == 0) {
      /* Disable the push constant buffers. */
      BEGIN_BATCH(5);
      OUT_BATCH(_3DSTATE_CONSTANT_VS << 16 | (5 - 2));
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   } else {
      BEGIN_BATCH(5);
      OUT_BATCH(_3DSTATE_CONSTANT_VS << 16 |
		GEN6_CONSTANT_BUFFER_0_ENABLE |
		(5 - 2));
      /* Pointer to the VS constant buffer.  Covered by the set of
       * state flags from gen6_upload_vs_constants
       */
      OUT_BATCH(stage_state->push_const_offset +
		stage_state->push_const_size - 1);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }

   /* Use ALT floating point mode for ARB vertex programs, because they
    * require 0^0 == 1.
    */
   if (ctx->_Shader->CurrentProgram[MESA_SHADER_VERTEX] == NULL)
      floating_point_mode = GEN6_VS_FLOATING_POINT_MODE_ALT;

   BEGIN_BATCH(6);
   OUT_BATCH(_3DSTATE_VS << 16 | (6 - 2));
   OUT_BATCH(stage_state->prog_offset);
   OUT_BATCH(floating_point_mode |
	     ((ALIGN(stage_state->sampler_count, 4)/4) << GEN6_VS_SAMPLER_COUNT_SHIFT) |
             ((brw->vs.prog_data->base.base.binding_table.size_bytes / 4) <<
              GEN6_VS_BINDING_TABLE_ENTRY_COUNT_SHIFT));

   if (brw->vs.prog_data->base.total_scratch) {
      OUT_RELOC(stage_state->scratch_bo,
		I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
		ffs(brw->vs.prog_data->base.total_scratch) - 11);
   } else {
      OUT_BATCH(0);
   }

   OUT_BATCH((brw->vs.prog_data->base.dispatch_grf_start_reg <<
              GEN6_VS_DISPATCH_START_GRF_SHIFT) |
	     (brw->vs.prog_data->base.urb_read_length << GEN6_VS_URB_READ_LENGTH_SHIFT) |
	     (0 << GEN6_VS_URB_ENTRY_READ_OFFSET_SHIFT));

   OUT_BATCH(((brw->max_vs_threads - 1) << GEN6_VS_MAX_THREADS_SHIFT) |
	     GEN6_VS_STATISTICS_ENABLE |
	     GEN6_VS_ENABLE);
   ADVANCE_BATCH();

   /* Based on my reading of the simulator, the VS constants don't get
    * pulled into the VS FF unit until an appropriate pipeline flush
    * happens, and instead the 3DSTATE_CONSTANT_VS packet just adds
    * references to them into a little FIFO.  The flushes are common,
    * but don't reliably happen between this and a 3DPRIMITIVE, causing
    * the primitive to use the wrong constants.  Then the FIFO
    * containing the constant setup gets added to again on the next
    * constants change, and eventually when a flush does happen the
    * unit is overwhelmed by constant changes and dies.
    *
    * To avoid this, send a PIPE_CONTROL down the line that will
    * update the unit immediately loading the constants.  The flush
    * type bits here were those set by the STATE_BASE_ADDRESS whose
    * move in a82a43e8d99e1715dd11c9c091b5ab734079b6a6 triggered the
    * bug reports that led to this workaround, and may be more than
    * what is strictly required to avoid the issue.
    */
   intel_emit_post_sync_nonzero_flush(brw);
   brw_emit_pipe_control_flush(brw,
                               PIPE_CONTROL_DEPTH_STALL |
                               PIPE_CONTROL_INSTRUCTION_FLUSH |
                               PIPE_CONTROL_STATE_CACHE_INVALIDATE);
}

const struct brw_tracked_state gen6_vs_state = {
   .dirty = {
      .mesa  = _NEW_TRANSFORM | _NEW_PROGRAM_CONSTANTS,
      .brw   = (BRW_NEW_CONTEXT |
		BRW_NEW_VERTEX_PROGRAM |
		BRW_NEW_BATCH |
                BRW_NEW_PUSH_CONSTANT_ALLOCATION),
      .cache = CACHE_NEW_VS_PROG
   },
   .emit = upload_vs_state,
};

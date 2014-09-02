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

#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"
#include "brw_util.h"
#include "program/prog_parameter.h"
#include "program/prog_statevars.h"
#include "intel_batchbuffer.h"


void
gen7_upload_constant_state(struct brw_context *brw,
                           const struct brw_stage_state *stage_state,
                           bool active, unsigned opcode)
{
   uint32_t mocs = brw->gen < 8 ? GEN7_MOCS_L3 : 0;

   /* Disable if the shader stage is inactive or there are no push constants. */
   active = active && stage_state->push_const_size != 0;

   int dwords = brw->gen >= 8 ? 11 : 7;
   BEGIN_BATCH(dwords);
   OUT_BATCH(opcode << 16 | (dwords - 2));
   OUT_BATCH(active ? stage_state->push_const_size : 0);
   OUT_BATCH(0);
   /* Pointer to the constant buffer.  Covered by the set of state flags
    * from gen6_prepare_wm_contants
    */
   OUT_BATCH(active ? (stage_state->push_const_offset | mocs) : 0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   if (brw->gen >= 8) {
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
   }

   ADVANCE_BATCH();
}


static void
upload_vs_state(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   const struct brw_stage_state *stage_state = &brw->vs.base;
   uint32_t floating_point_mode = 0;
   const int max_threads_shift = brw->is_haswell ?
      HSW_VS_MAX_THREADS_SHIFT : GEN6_VS_MAX_THREADS_SHIFT;

   if (!brw->is_haswell && !brw->is_baytrail)
      gen7_emit_vs_workaround_flush(brw);

   /* Use ALT floating point mode for ARB vertex programs, because they
    * require 0^0 == 1.
    */
   if (ctx->_Shader->CurrentProgram[MESA_SHADER_VERTEX] == NULL)
      floating_point_mode = GEN6_VS_FLOATING_POINT_MODE_ALT;

   BEGIN_BATCH(6);
   OUT_BATCH(_3DSTATE_VS << 16 | (6 - 2));
   OUT_BATCH(stage_state->prog_offset);
   OUT_BATCH(floating_point_mode |
	     ((ALIGN(stage_state->sampler_count, 4)/4) <<
              GEN6_VS_SAMPLER_COUNT_SHIFT) |
             ((brw->vs.prog_data->base.base.binding_table.size_bytes / 4) <<
              GEN6_VS_BINDING_TABLE_ENTRY_COUNT_SHIFT));

   if (brw->vs.prog_data->base.base.total_scratch) {
      OUT_RELOC(stage_state->scratch_bo,
		I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
		ffs(brw->vs.prog_data->base.base.total_scratch) - 11);
   } else {
      OUT_BATCH(0);
   }

   OUT_BATCH((brw->vs.prog_data->base.base.dispatch_grf_start_reg <<
              GEN6_VS_DISPATCH_START_GRF_SHIFT) |
	     (brw->vs.prog_data->base.urb_read_length << GEN6_VS_URB_READ_LENGTH_SHIFT) |
	     (0 << GEN6_VS_URB_ENTRY_READ_OFFSET_SHIFT));

   OUT_BATCH(((brw->max_vs_threads - 1) << max_threads_shift) |
	     GEN6_VS_STATISTICS_ENABLE |
	     GEN6_VS_ENABLE);
   ADVANCE_BATCH();
}

const struct brw_tracked_state gen7_vs_state = {
   .dirty = {
      .mesa  = _NEW_TRANSFORM,
      .brw   = (BRW_NEW_CONTEXT |
		BRW_NEW_VERTEX_PROGRAM |
		BRW_NEW_BATCH),
      .cache = CACHE_NEW_VS_PROG
   },
   .emit = upload_vs_state,
};

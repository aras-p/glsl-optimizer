/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics to
 develop this 3D driver.

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:

 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keithw@vmware.com>
  */



#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"
#include "main/macros.h"

static void
brw_upload_vs_unit(struct brw_context *brw)
{
   struct brw_stage_state *stage_state = &brw->vs.base;

   struct brw_vs_unit_state *vs;

   vs = brw_state_batch(brw, AUB_TRACE_VS_STATE,
			sizeof(*vs), 32, &stage_state->state_offset);
   memset(vs, 0, sizeof(*vs));

   /* BRW_NEW_PROGRAM_CACHE | CACHE_NEW_VS_PROG */
   vs->thread0.grf_reg_count =
      ALIGN(brw->vs.prog_data->base.total_grf, 16) / 16 - 1;
   vs->thread0.kernel_start_pointer =
      brw_program_reloc(brw,
			stage_state->state_offset +
			offsetof(struct brw_vs_unit_state, thread0),
			stage_state->prog_offset +
			(vs->thread0.grf_reg_count << 1)) >> 6;

   /* Use ALT floating point mode for ARB vertex programs, because they
    * require 0^0 == 1.
    */
   if (brw->ctx.Shader.CurrentProgram[MESA_SHADER_VERTEX] == NULL)
      vs->thread1.floating_point_mode = BRW_FLOATING_POINT_NON_IEEE_754;
   else
      vs->thread1.floating_point_mode = BRW_FLOATING_POINT_IEEE_754;

   /* Choosing multiple program flow means that we may get 2-vertex threads,
    * which will have the channel mask for dwords 4-7 enabled in the thread,
    * and those dwords will be written to the second URB handle when we
    * brw_urb_WRITE() results.
    */
   /* Force single program flow on Ironlake.  We cannot reliably get
    * all applications working without it.  See:
    * https://bugs.freedesktop.org/show_bug.cgi?id=29172
    *
    * The most notable and reliably failing application is the Humus
    * demo "CelShading"
   */
   vs->thread1.single_program_flow = (brw->gen == 5);

   vs->thread1.binding_table_entry_count =
      brw->vs.prog_data->base.base.binding_table.size_bytes / 4;

   if (brw->vs.prog_data->base.base.total_scratch != 0) {
      vs->thread2.scratch_space_base_pointer =
	 stage_state->scratch_bo->offset64 >> 10; /* reloc */
      vs->thread2.per_thread_scratch_space =
	 ffs(brw->vs.prog_data->base.base.total_scratch) - 11;
   } else {
      vs->thread2.scratch_space_base_pointer = 0;
      vs->thread2.per_thread_scratch_space = 0;
   }

   vs->thread3.urb_entry_read_length = brw->vs.prog_data->base.urb_read_length;
   vs->thread3.const_urb_entry_read_length
      = brw->vs.prog_data->base.base.curb_read_length;
   vs->thread3.dispatch_grf_start_reg =
      brw->vs.prog_data->base.base.dispatch_grf_start_reg;
   vs->thread3.urb_entry_read_offset = 0;

   /* BRW_NEW_CURBE_OFFSETS, _NEW_TRANSFORM, BRW_NEW_VERTEX_PROGRAM */
   vs->thread3.const_urb_entry_read_offset = brw->curbe.vs_start * 2;

   /* BRW_NEW_URB_FENCE */
   if (brw->gen == 5) {
      switch (brw->urb.nr_vs_entries) {
      case 8:
      case 12:
      case 16:
      case 32:
      case 64:
      case 96:
      case 128:
      case 168:
      case 192:
      case 224:
      case 256:
	 vs->thread4.nr_urb_entries = brw->urb.nr_vs_entries >> 2;
	 break;
      default:
         unreachable("not reached");
      }
   } else {
      switch (brw->urb.nr_vs_entries) {
      case 8:
      case 12:
      case 16:
      case 32:
	 break;
      case 64:
	 assert(brw->is_g4x);
	 break;
      default:
         unreachable("not reached");
      }
      vs->thread4.nr_urb_entries = brw->urb.nr_vs_entries;
   }

   vs->thread4.urb_entry_allocation_size = brw->urb.vsize - 1;

   vs->thread4.max_threads = CLAMP(brw->urb.nr_vs_entries / 2,
				   1, brw->max_vs_threads) - 1;

   if (brw->gen == 5)
      vs->vs5.sampler_count = 0; /* hardware requirement */
   else {
      /* CACHE_NEW_SAMPLER */
      vs->vs5.sampler_count = (stage_state->sampler_count + 3) / 4;
   }


   if (unlikely(INTEL_DEBUG & DEBUG_STATS))
      vs->thread4.stats_enable = 1;

   /* Vertex program always enabled:
    */
   vs->vs6.vs_enable = 1;

   /* Set the sampler state pointer, and its reloc
    */
   if (stage_state->sampler_count) {
      vs->vs5.sampler_state_pointer =
         (brw->batch.bo->offset64 + stage_state->sampler_offset) >> 5;
      drm_intel_bo_emit_reloc(brw->batch.bo,
                              stage_state->state_offset +
                              offsetof(struct brw_vs_unit_state, vs5),
                              brw->batch.bo,
                              (stage_state->sampler_offset |
                               vs->vs5.sampler_count),
                              I915_GEM_DOMAIN_INSTRUCTION, 0);
   }

   /* Emit scratch space relocation */
   if (brw->vs.prog_data->base.base.total_scratch != 0) {
      drm_intel_bo_emit_reloc(brw->batch.bo,
			      stage_state->state_offset +
			      offsetof(struct brw_vs_unit_state, thread2),
			      stage_state->scratch_bo,
			      vs->thread2.per_thread_scratch_space,
			      I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER);
   }

   brw->state.dirty.cache |= CACHE_NEW_VS_UNIT;
}

const struct brw_tracked_state brw_vs_unit = {
   .dirty = {
      .mesa  = _NEW_TRANSFORM,
      .brw   = (BRW_NEW_BATCH |
		BRW_NEW_PROGRAM_CACHE |
		BRW_NEW_CURBE_OFFSETS |
		BRW_NEW_URB_FENCE |
                BRW_NEW_VERTEX_PROGRAM),
      .cache = CACHE_NEW_VS_PROG | CACHE_NEW_SAMPLER
   },
   .emit = brw_upload_vs_unit,
};

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
#include "intel_batchbuffer.h"

static void
gen6_upload_gs_push_constants(struct brw_context *brw)
{
   struct brw_stage_state *stage_state = &brw->gs.base;

   /* BRW_NEW_GEOMETRY_PROGRAM */
   const struct brw_geometry_program *gp =
      (struct brw_geometry_program *) brw->geometry_program;

   if (gp) {
      /* CACHE_NEW_GS_PROG */
      struct brw_stage_prog_data *prog_data = &brw->gs.prog_data->base.base;

      gen6_upload_push_constants(brw, &gp->program.Base, prog_data,
                                 stage_state, AUB_TRACE_VS_CONSTANTS);
   }

   if (brw->gen >= 7)
      gen7_upload_constant_state(brw, stage_state, gp, _3DSTATE_CONSTANT_GS);
}

const struct brw_tracked_state gen6_gs_push_constants = {
   .dirty = {
      .mesa  = _NEW_TRANSFORM | _NEW_PROGRAM_CONSTANTS,
      .brw   = (BRW_NEW_BATCH |
                BRW_NEW_GEOMETRY_PROGRAM |
                BRW_NEW_PUSH_CONSTANT_ALLOCATION),
      .cache = CACHE_NEW_GS_PROG,
   },
   .emit = gen6_upload_gs_push_constants,
};

static void
upload_gs_state_for_tf(struct brw_context *brw)
{
   BEGIN_BATCH(7);
   OUT_BATCH(_3DSTATE_GS << 16 | (7 - 2));
   OUT_BATCH(brw->ff_gs.prog_offset);
   OUT_BATCH(GEN6_GS_SPF_MODE | GEN6_GS_VECTOR_MASK_ENABLE);
   OUT_BATCH(0); /* no scratch space */
   OUT_BATCH((2 << GEN6_GS_DISPATCH_START_GRF_SHIFT) |
             (brw->ff_gs.prog_data->urb_read_length << GEN6_GS_URB_READ_LENGTH_SHIFT));
   OUT_BATCH(((brw->max_gs_threads - 1) << GEN6_GS_MAX_THREADS_SHIFT) |
             GEN6_GS_STATISTICS_ENABLE |
             GEN6_GS_SO_STATISTICS_ENABLE |
             GEN6_GS_RENDERING_ENABLE);
   OUT_BATCH(GEN6_GS_SVBI_PAYLOAD_ENABLE |
             GEN6_GS_SVBI_POSTINCREMENT_ENABLE |
             (brw->ff_gs.prog_data->svbi_postincrement_value <<
              GEN6_GS_SVBI_POSTINCREMENT_VALUE_SHIFT) |
             GEN6_GS_ENABLE);
   ADVANCE_BATCH();
}

static void
upload_gs_state(struct brw_context *brw)
{
   /* BRW_NEW_GEOMETRY_PROGRAM */
   bool active = brw->geometry_program;
   /* CACHE_NEW_GS_PROG */
   const struct brw_vec4_prog_data *prog_data = &brw->gs.prog_data->base;
   const struct brw_stage_state *stage_state = &brw->gs.base;

   if (!active || stage_state->push_const_size == 0) {
      /* Disable the push constant buffers. */
      BEGIN_BATCH(5);
      OUT_BATCH(_3DSTATE_CONSTANT_GS << 16 | (5 - 2));
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   } else {
      BEGIN_BATCH(5);
      OUT_BATCH(_3DSTATE_CONSTANT_GS << 16 |
		GEN6_CONSTANT_BUFFER_0_ENABLE |
		(5 - 2));
      /* Pointer to the GS constant buffer.  Covered by the set of
       * state flags from gen6_upload_vs_constants
       */
      OUT_BATCH(stage_state->push_const_offset +
                stage_state->push_const_size - 1);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }

   if (active) {
      BEGIN_BATCH(7);
      OUT_BATCH(_3DSTATE_GS << 16 | (7 - 2));
      OUT_BATCH(stage_state->prog_offset);

      /* GEN6_GS_SPF_MODE and GEN6_GS_VECTOR_MASK_ENABLE are enabled as it
       * was previously done for gen6.
       *
       * TODO: test with both disabled to see if the HW is behaving
       * as expected, like in gen7.
       */
      OUT_BATCH(GEN6_GS_SPF_MODE | GEN6_GS_VECTOR_MASK_ENABLE |
                ((ALIGN(stage_state->sampler_count, 4)/4) <<
                 GEN6_GS_SAMPLER_COUNT_SHIFT) |
                ((prog_data->base.binding_table.size_bytes / 4) <<
                 GEN6_GS_BINDING_TABLE_ENTRY_COUNT_SHIFT));

      if (prog_data->base.total_scratch) {
         OUT_RELOC(stage_state->scratch_bo,
                   I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
                   ffs(prog_data->base.total_scratch) - 11);
      } else {
         OUT_BATCH(0); /* no scratch space */
      }

      OUT_BATCH((prog_data->urb_read_length <<
                 GEN6_GS_URB_READ_LENGTH_SHIFT) |
                (0 << GEN6_GS_URB_ENTRY_READ_OFFSET_SHIFT) |
                (prog_data->base.dispatch_grf_start_reg <<
                 GEN6_GS_DISPATCH_START_GRF_SHIFT));

      OUT_BATCH(((brw->max_gs_threads - 1) << GEN6_GS_MAX_THREADS_SHIFT) |
                GEN6_GS_STATISTICS_ENABLE |
                GEN6_GS_SO_STATISTICS_ENABLE |
                GEN6_GS_RENDERING_ENABLE);

      if (brw->gs.prog_data->gen6_xfb_enabled) {
         /* GEN6_GS_REORDER is equivalent to GEN7_GS_REORDER_TRAILING
          * in gen7. SNB and IVB specs are the same regarding the reordering of
          * TRISTRIP/TRISTRIP_REV vertices and triangle orientation, so we do
          * the same thing in both generations. For more details, see the
          * comment in gen7_gs_state.c
          */
         OUT_BATCH(GEN6_GS_REORDER |
                   GEN6_GS_SVBI_PAYLOAD_ENABLE |
                   GEN6_GS_ENABLE);
      } else {
         OUT_BATCH(GEN6_GS_REORDER | GEN6_GS_ENABLE);
      }
      ADVANCE_BATCH();
   } else if (brw->ff_gs.prog_active) {
      /* In gen6, transform feedback for the VS stage is done with an ad-hoc GS
       * program. This function provides the needed 3DSTATE_GS for this.
       */
      upload_gs_state_for_tf(brw);
   } else {
      /* No GS function required */
      BEGIN_BATCH(7);
      OUT_BATCH(_3DSTATE_GS << 16 | (7 - 2));
      OUT_BATCH(0); /* prog_bo */
      OUT_BATCH((0 << GEN6_GS_SAMPLER_COUNT_SHIFT) |
                (0 << GEN6_GS_BINDING_TABLE_ENTRY_COUNT_SHIFT));
      OUT_BATCH(0); /* scratch space base offset */
      OUT_BATCH((1 << GEN6_GS_DISPATCH_START_GRF_SHIFT) |
                (0 << GEN6_GS_URB_READ_LENGTH_SHIFT) |
                (0 << GEN6_GS_URB_ENTRY_READ_OFFSET_SHIFT));
      OUT_BATCH((0 << GEN6_GS_MAX_THREADS_SHIFT) |
                GEN6_GS_STATISTICS_ENABLE |
                GEN6_GS_RENDERING_ENABLE);
                OUT_BATCH(0);
      ADVANCE_BATCH();
   }
   brw->gs.enabled = active;
}

const struct brw_tracked_state gen6_gs_state = {
   .dirty = {
      .mesa  = _NEW_TRANSFORM | _NEW_PROGRAM_CONSTANTS,
      .brw   = (BRW_NEW_CONTEXT |
                BRW_NEW_PUSH_CONSTANT_ALLOCATION |
                BRW_NEW_GEOMETRY_PROGRAM |
                BRW_NEW_BATCH),
      .cache = (CACHE_NEW_GS_PROG | CACHE_NEW_FF_GS_PROG)
   },
   .emit = upload_gs_state,
};

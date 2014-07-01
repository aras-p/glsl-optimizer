/*
 * Copyright © 2013 Intel Corporation
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"
#include "intel_batchbuffer.h"

static void
gen8_upload_gs_state(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   const struct brw_stage_state *stage_state = &brw->gs.base;
   /* BRW_NEW_GEOMETRY_PROGRAM */
   bool active = brw->geometry_program;
   /* CACHE_NEW_GS_PROG */
   const struct brw_vec4_prog_data *prog_data = &brw->gs.prog_data->base;

   if (active) {
      int urb_entry_write_offset = 1;
      uint32_t urb_entry_output_length =
         ((prog_data->vue_map.num_slots + 1) / 2 - urb_entry_write_offset);

      if (urb_entry_output_length == 0)
         urb_entry_output_length = 1;

      BEGIN_BATCH(10);
      OUT_BATCH(_3DSTATE_GS << 16 | (10 - 2));
      OUT_BATCH(stage_state->prog_offset);
      OUT_BATCH(0);
      OUT_BATCH(GEN6_GS_VECTOR_MASK_ENABLE |
                brw->geometry_program->VerticesIn |
                ((ALIGN(stage_state->sampler_count, 4)/4) <<
                 GEN6_GS_SAMPLER_COUNT_SHIFT) |
                ((prog_data->base.binding_table.size_bytes / 4) <<
                 GEN6_GS_BINDING_TABLE_ENTRY_COUNT_SHIFT));

      if (brw->gs.prog_data->base.base.total_scratch) {
         OUT_RELOC64(stage_state->scratch_bo,
                     I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
                     ffs(brw->gs.prog_data->base.base.total_scratch) - 11);
         WARN_ONCE(true,
                   "May need to implement a temporary workaround: GS Number of "
                   "URB Entries must be less than or equal to the GS Maximum "
                   "Number of Threads.\n");
      } else {
         OUT_BATCH(0);
         OUT_BATCH(0);
      }

      /* DW6 */
      OUT_BATCH(((brw->gs.prog_data->output_vertex_size_hwords * 2 - 1) <<
                 GEN7_GS_OUTPUT_VERTEX_SIZE_SHIFT) |
                (brw->gs.prog_data->output_topology <<
                 GEN7_GS_OUTPUT_TOPOLOGY_SHIFT) |
                (prog_data->urb_read_length <<
                 GEN6_GS_URB_READ_LENGTH_SHIFT) |
                (0 << GEN6_GS_URB_ENTRY_READ_OFFSET_SHIFT) |
                (prog_data->base.dispatch_grf_start_reg <<
                 GEN6_GS_DISPATCH_START_GRF_SHIFT));

      /* DW7 */
      OUT_BATCH(((brw->max_gs_threads / 2 - 1) << HSW_GS_MAX_THREADS_SHIFT) |
                (brw->gs.prog_data->control_data_header_size_hwords <<
                 GEN7_GS_CONTROL_DATA_HEADER_SIZE_SHIFT) |
                brw->gs.prog_data->dispatch_mode |
                GEN6_GS_STATISTICS_ENABLE |
                (brw->gs.prog_data->include_primitive_id ?
                 GEN7_GS_INCLUDE_PRIMITIVE_ID : 0) |
                GEN7_GS_REORDER_TRAILING |
                GEN7_GS_ENABLE);

      /* DW8 */
      OUT_BATCH(brw->gs.prog_data->control_data_format <<
                HSW_GS_CONTROL_DATA_FORMAT_SHIFT);

      /* DW9 / _NEW_TRANSFORM */
      OUT_BATCH((ctx->Transform.ClipPlanesEnabled <<
                 GEN8_GS_USER_CLIP_DISTANCE_SHIFT) |
                (urb_entry_output_length << GEN8_GS_URB_OUTPUT_LENGTH_SHIFT) |
                (urb_entry_write_offset <<
                 GEN8_GS_URB_ENTRY_OUTPUT_OFFSET_SHIFT));
      ADVANCE_BATCH();
   } else {
      BEGIN_BATCH(10);
      OUT_BATCH(_3DSTATE_GS << 16 | (10 - 2));
      OUT_BATCH(0); /* prog_bo */
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0); /* scratch space base offset */
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(GEN6_GS_STATISTICS_ENABLE);
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }
}

const struct brw_tracked_state gen8_gs_state = {
   .dirty = {
      .mesa  = _NEW_TRANSFORM,
      .brw   = (BRW_NEW_CONTEXT |
                BRW_NEW_GEOMETRY_PROGRAM |
                BRW_NEW_BATCH),
      .cache = CACHE_NEW_GS_PROG
   },
   .emit = gen8_upload_gs_state,
};

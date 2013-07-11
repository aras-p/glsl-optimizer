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

#include "main/macros.h"
#include "intel_batchbuffer.h"
#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"

/**
 * The following diagram shows how we partition the URB:
 *
 *      8kB         8kB              Rest of the URB space
 *   ____-____   ____-____   _________________-_________________
 *  /         \ /         \ /                                   \
 * +-------------------------------------------------------------+
 * | VS Push   | FS Push   | VS                                  |
 * | Constants | Constants | Handles                             |
 * +-------------------------------------------------------------+
 *
 * Notably, push constants must be stored at the beginning of the URB
 * space, while entries can be stored anywhere.  Ivybridge and Haswell
 * GT1/GT2 have a maximum constant buffer size of 16kB, while Haswell GT3
 * doubles this (32kB).
 *
 * Currently we split the constant buffer space evenly between VS and FS.
 * This is probably not ideal, but simple.
 *
 * Ivybridge GT1 and Haswell GT1 have 128kB of URB space.
 * Ivybridge GT2 and Haswell GT2 have 256kB of URB space.
 * Haswell GT3 has 512kB of URB space.
 *
 * See "Volume 2a: 3D Pipeline," section 1.8, "Volume 1b: Configurations",
 * and the documentation for 3DSTATE_PUSH_CONSTANT_ALLOC_xS.
 */
void
gen7_allocate_push_constants(struct brw_context *brw)
{
   unsigned size = 8;
   if (brw->is_haswell && brw->gt == 3)
      size = 16;

   BEGIN_BATCH(4);
   OUT_BATCH(_3DSTATE_PUSH_CONSTANT_ALLOC_VS << 16 | (2 - 2));
   OUT_BATCH(size);

   OUT_BATCH(_3DSTATE_PUSH_CONSTANT_ALLOC_PS << 16 | (2 - 2));
   OUT_BATCH(size | size << GEN7_PUSH_CONSTANT_BUFFER_OFFSET_SHIFT);
   ADVANCE_BATCH();
}

static void
gen7_upload_urb(struct brw_context *brw)
{
   const int push_size_kB = brw->is_haswell && brw->gt == 3 ? 32 : 16;

   /* Total space for entries is URB size - 16kB for push constants */
   int handle_region_size = (brw->urb.size - push_size_kB) * 1024; /* bytes */

   /* CACHE_NEW_VS_PROG */
   unsigned vs_size = MAX2(brw->vs.prog_data->base.urb_entry_size, 1);

   int nr_vs_entries = handle_region_size / (vs_size * 64);
   if (nr_vs_entries > brw->urb.max_vs_entries)
      nr_vs_entries = brw->urb.max_vs_entries;

   /* According to volume 2a, nr_vs_entries must be a multiple of 8. */
   brw->urb.nr_vs_entries = ROUND_DOWN_TO(nr_vs_entries, 8);

   /* URB Starting Addresses are specified in multiples of 8kB. */
   brw->urb.vs_start = push_size_kB / 8; /* skip over push constants */

   assert(brw->urb.nr_vs_entries % 8 == 0);
   assert(brw->urb.nr_gs_entries % 8 == 0);
   /* GS requirement */
   assert(!brw->gs.prog_active);

   gen7_emit_vs_workaround_flush(brw);
   gen7_emit_urb_state(brw, brw->urb.nr_vs_entries, vs_size, brw->urb.vs_start);
}

void
gen7_emit_urb_state(struct brw_context *brw, GLuint nr_vs_entries,
                    GLuint vs_size, GLuint vs_start)
{
   BEGIN_BATCH(8);
   OUT_BATCH(_3DSTATE_URB_VS << 16 | (2 - 2));
   OUT_BATCH(nr_vs_entries |
             ((vs_size - 1) << GEN7_URB_ENTRY_SIZE_SHIFT) |
             (vs_start << GEN7_URB_STARTING_ADDRESS_SHIFT));

   /* Allocate the GS, HS, and DS zero space - we don't use them. */
   OUT_BATCH(_3DSTATE_URB_GS << 16 | (2 - 2));
   OUT_BATCH((0 << GEN7_URB_ENTRY_SIZE_SHIFT) |
             (vs_start << GEN7_URB_STARTING_ADDRESS_SHIFT));

   OUT_BATCH(_3DSTATE_URB_HS << 16 | (2 - 2));
   OUT_BATCH((0 << GEN7_URB_ENTRY_SIZE_SHIFT) |
             (vs_start << GEN7_URB_STARTING_ADDRESS_SHIFT));

   OUT_BATCH(_3DSTATE_URB_DS << 16 | (2 - 2));
   OUT_BATCH((0 << GEN7_URB_ENTRY_SIZE_SHIFT) |
             (vs_start << GEN7_URB_STARTING_ADDRESS_SHIFT));
   ADVANCE_BATCH();
}

const struct brw_tracked_state gen7_urb = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_CONTEXT,
      .cache = (CACHE_NEW_VS_PROG | CACHE_NEW_GS_PROG),
   },
   .emit = gen7_upload_urb,
};

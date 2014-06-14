/*
 * Copyright © 2012 Intel Corporation
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

#include "intel_batchbuffer.h"
#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"

/**
 * Define the base addresses which some state is referenced from.
 */
static void upload_state_base_address(struct brw_context *brw)
{
   BEGIN_BATCH(16);
   OUT_BATCH(CMD_STATE_BASE_ADDRESS << 16 | (16 - 2));
   /* General state base address: stateless DP read/write requests */
   OUT_BATCH(BDW_MOCS_WB << 4 | 1);
   OUT_BATCH(0);
   OUT_BATCH(BDW_MOCS_WB << 16);
   /* Surface state base address: */
   OUT_RELOC64(brw->batch.bo, I915_GEM_DOMAIN_SAMPLER, 0,
               BDW_MOCS_WB << 4 | 1);
   /* Dynamic state base address: */
   OUT_RELOC64(brw->batch.bo,
               I915_GEM_DOMAIN_RENDER | I915_GEM_DOMAIN_INSTRUCTION, 0,
               BDW_MOCS_WB << 4 | 1);
   /* Indirect object base address: MEDIA_OBJECT data */
   OUT_BATCH(BDW_MOCS_WB << 4 | 1);
   OUT_BATCH(0);
   /* Instruction base address: shader kernels (incl. SIP) */
   OUT_RELOC64(brw->cache.bo, I915_GEM_DOMAIN_INSTRUCTION, 0,
               BDW_MOCS_WB << 4 | 1);

   /* General state buffer size */
   OUT_BATCH(0xfffff001);
   /* Dynamic state buffer size */
   OUT_BATCH(ALIGN(brw->batch.bo->size, 4096) | 1);
   /* Indirect object upper bound */
   OUT_BATCH(0xfffff001);
   /* Instruction access upper bound */
   OUT_BATCH(ALIGN(brw->cache.bo->size, 4096) | 1);
   ADVANCE_BATCH();

   brw->state.dirty.brw |= BRW_NEW_STATE_BASE_ADDRESS;
}

const struct brw_tracked_state gen8_state_base_address = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_BATCH | BRW_NEW_PROGRAM_CACHE,
      .cache = 0,
   },
   .emit = upload_state_base_address
};

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
#include "brw_defines.h"
#include "brw_multisample_state.h"

/**
 * 3DSTATE_MULTISAMPLE
 */
void
gen8_emit_3dstate_multisample(struct brw_context *brw, unsigned num_samples)
{
   assert(num_samples <= 16);

   unsigned log2_samples = ffs(MAX2(num_samples, 1)) - 1;

   BEGIN_BATCH(2);
   OUT_BATCH(GEN8_3DSTATE_MULTISAMPLE << 16 | (2 - 2));
   OUT_BATCH(MS_PIXEL_LOCATION_CENTER | log2_samples << 1);
   ADVANCE_BATCH();
}

/**
 * 3DSTATE_SAMPLE_PATTERN
 */
void
gen8_emit_3dstate_sample_pattern(struct brw_context *brw)
{
   BEGIN_BATCH(9);
   OUT_BATCH(_3DSTATE_SAMPLE_PATTERN << 16 | (9 - 2));

   /* 16x MSAA
    * XXX: Need to program these.
    */
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);

   /* 8x MSAA */
   OUT_BATCH(brw_multisample_positions_8x[1]); /* sample positions 7654 */
   OUT_BATCH(brw_multisample_positions_8x[0]); /* sample positions 3210 */

   /* 4x MSAA */
   OUT_BATCH(brw_multisample_positions_4x);

   /* 1x and 2x MSAA */
   OUT_BATCH(brw_multisample_positions_1x_2x);
   ADVANCE_BATCH();
}


static void
upload_multisample_state(struct brw_context *brw)
{
   gen8_emit_3dstate_multisample(brw, brw->num_samples);
   gen6_emit_3dstate_sample_mask(brw, gen6_determine_sample_mask(brw));
}

const struct brw_tracked_state gen8_multisample_state = {
   .dirty = {
      .mesa = _NEW_MULTISAMPLE,
      .brw = (BRW_NEW_CONTEXT |
              BRW_NEW_NUM_SAMPLES),
      .cache = 0
   },
   .emit = upload_multisample_state
};

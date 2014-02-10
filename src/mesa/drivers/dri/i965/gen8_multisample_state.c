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
   uint32_t number_of_multisamples = 0;

   switch (num_samples) {
   case 0:
   case 1:
      number_of_multisamples = MS_NUMSAMPLES_1;
      break;
   case 2:
      number_of_multisamples = MS_NUMSAMPLES_2;
      break;
   case 4:
      number_of_multisamples = MS_NUMSAMPLES_4;
      break;
   case 8:
      number_of_multisamples = MS_NUMSAMPLES_8;
      break;
   case 16:
      number_of_multisamples = MS_NUMSAMPLES_16;
      break;
   default:
      assert(!"Unrecognized num_samples in gen8_emit_3dstate_multisample");
      break;
   }

   BEGIN_BATCH(2);
   OUT_BATCH(GEN8_3DSTATE_MULTISAMPLE << 16 | (2 - 2));
   OUT_BATCH(MS_PIXEL_LOCATION_CENTER | number_of_multisamples);
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
   struct gl_context *ctx = &brw->ctx;

   /* _NEW_BUFFERS, _NEW_MULTISAMPLE */
   unsigned num_samples = ctx->DrawBuffer->Visual.samples;

   gen8_emit_3dstate_multisample(brw, num_samples);
   gen6_emit_3dstate_sample_mask(brw, gen6_determine_sample_mask(brw));
}

const struct brw_tracked_state gen8_multisample_state = {
   .dirty = {
      .mesa = _NEW_BUFFERS | _NEW_MULTISAMPLE,
      .brw = BRW_NEW_CONTEXT,
      .cache = 0
   },
   .emit = upload_multisample_state
};

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


/**
 * 3DSTATE_MULTISAMPLE
 */
void
gen6_emit_3dstate_multisample(struct brw_context *brw,
                              unsigned num_samples)
{
   struct intel_context *intel = &brw->intel;

   /* TODO: 8x MSAA not implemented */
   assert(num_samples <= 4);

   int len = intel->gen >= 7 ? 4 : 3;
   BEGIN_BATCH(len);
   OUT_BATCH(_3DSTATE_MULTISAMPLE << 16 | (len - 2));
   OUT_BATCH(MS_PIXEL_LOCATION_CENTER |
             (num_samples > 0 ? MS_NUMSAMPLES_4 : MS_NUMSAMPLES_1));
   OUT_BATCH(num_samples > 0 ? 0xae2ae662 : 0); /* positions for 4/8-sample */
   if (intel->gen >= 7)
      OUT_BATCH(0);
   ADVANCE_BATCH();
}


/**
 * 3DSTATE_SAMPLE_MASK
 */
void
gen6_emit_3dstate_sample_mask(struct brw_context *brw,
                              unsigned num_samples)
{
   struct intel_context *intel = &brw->intel;

   /* TODO: 8x MSAA not implemented */
   assert(num_samples <= 4);

   BEGIN_BATCH(2);
   OUT_BATCH(_3DSTATE_SAMPLE_MASK << 16 | (2 - 2));
   OUT_BATCH(num_samples > 0 ? 15 : 1);
   ADVANCE_BATCH();
}


static void upload_multisample_state(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;
   unsigned num_samples = 0;

   /* _NEW_BUFFERS */
   if (ctx->DrawBuffer->_ColorDrawBuffers[0])
      num_samples = ctx->DrawBuffer->_ColorDrawBuffers[0]->NumSamples;

   /* 3DSTATE_MULTISAMPLE is nonpipelined. */
   intel_emit_post_sync_nonzero_flush(intel);

   gen6_emit_3dstate_multisample(brw, num_samples);
   gen6_emit_3dstate_sample_mask(brw, num_samples);
}


const struct brw_tracked_state gen6_multisample_state = {
   .dirty = {
      .mesa = _NEW_BUFFERS,
      .brw = BRW_NEW_CONTEXT,
      .cache = 0
   },
   .emit = upload_multisample_state
};

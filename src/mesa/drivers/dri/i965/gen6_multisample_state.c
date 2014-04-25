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

void
gen6_get_sample_position(struct gl_context *ctx,
                         struct gl_framebuffer *fb,
                         GLuint index, GLfloat *result)
{
   uint8_t bits;

   switch (fb->Visual.samples) {
   case 1:
      result[0] = result[1] = 0.5f;
      return;
   case 2:
      bits = brw_multisample_positions_1x_2x >> (8 * index);
      break;
   case 4:
      bits = brw_multisample_positions_4x >> (8 * index);
      break;
   case 8:
      bits = brw_multisample_positions_8x[index >> 2] >> (8 * (index & 3));
      break;
   default:
      assert(!"Not implemented");
      return;
   }

   /* Convert from U0.4 back to a floating point coordinate. */
   result[0] = ((bits >> 4) & 0xf) / 16.0f;
   result[1] = (bits & 0xf) / 16.0f;
}

/**
 * 3DSTATE_MULTISAMPLE
 */
void
gen6_emit_3dstate_multisample(struct brw_context *brw,
                              unsigned num_samples)
{
   uint32_t number_of_multisamples = 0;
   uint32_t sample_positions_3210 = 0;
   uint32_t sample_positions_7654 = 0;

   assert(brw->gen < 8);

   switch (num_samples) {
   case 0:
   case 1:
      number_of_multisamples = MS_NUMSAMPLES_1;
      break;
   case 4:
      number_of_multisamples = MS_NUMSAMPLES_4;
      sample_positions_3210 = brw_multisample_positions_4x;
      break;
   case 8:
      number_of_multisamples = MS_NUMSAMPLES_8;
      sample_positions_3210 = brw_multisample_positions_8x[0];
      sample_positions_7654 = brw_multisample_positions_8x[1];
      break;
   default:
      assert(!"Unrecognized num_samples in gen6_emit_3dstate_multisample");
      break;
   }

   /* 3DSTATE_MULTISAMPLE is nonpipelined. */
   intel_emit_post_sync_nonzero_flush(brw);

   int len = brw->gen >= 7 ? 4 : 3;
   BEGIN_BATCH(len);
   OUT_BATCH(_3DSTATE_MULTISAMPLE << 16 | (len - 2));
   OUT_BATCH(MS_PIXEL_LOCATION_CENTER | number_of_multisamples);
   OUT_BATCH(sample_positions_3210);
   if (brw->gen >= 7)
      OUT_BATCH(sample_positions_7654);
   ADVANCE_BATCH();
}


unsigned
gen6_determine_sample_mask(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   float coverage = 1.0;
   float coverage_invert = false;
   unsigned sample_mask = ~0u;

   /* BRW_NEW_NUM_SAMPLES */
   unsigned num_samples = brw->num_samples;

   if (ctx->Multisample._Enabled) {
      if (ctx->Multisample.SampleCoverage) {
         coverage = ctx->Multisample.SampleCoverageValue;
         coverage_invert = ctx->Multisample.SampleCoverageInvert;
      }
      if (ctx->Multisample.SampleMask) {
         sample_mask = ctx->Multisample.SampleMaskValue;
      }
   }

   if (num_samples > 1) {
      int coverage_int = (int) (num_samples * coverage + 0.5);
      uint32_t coverage_bits = (1 << coverage_int) - 1;
      if (coverage_invert)
         coverage_bits ^= (1 << num_samples) - 1;
      return coverage_bits & sample_mask;
   } else {
      return 1;
   }
}


/**
 * 3DSTATE_SAMPLE_MASK
 */
void
gen6_emit_3dstate_sample_mask(struct brw_context *brw, unsigned mask)
{
   BEGIN_BATCH(2);
   OUT_BATCH(_3DSTATE_SAMPLE_MASK << 16 | (2 - 2));
   OUT_BATCH(mask);
   ADVANCE_BATCH();
}


static void upload_multisample_state(struct brw_context *brw)
{
   /* BRW_NEW_NUM_SAMPLES */
   gen6_emit_3dstate_multisample(brw, brw->num_samples);
   gen6_emit_3dstate_sample_mask(brw, gen6_determine_sample_mask(brw));
}


const struct brw_tracked_state gen6_multisample_state = {
   .dirty = {
      .mesa = _NEW_MULTISAMPLE,
      .brw = (BRW_NEW_CONTEXT |
              BRW_NEW_NUM_SAMPLES),
      .cache = 0
   },
   .emit = upload_multisample_state
};

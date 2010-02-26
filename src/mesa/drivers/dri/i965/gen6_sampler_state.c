/*
 * Copyright © 2010 Intel Corporation
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
upload_sampler_state_pointers(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(4);
   OUT_BATCH(CMD_3D_SAMPLER_STATE_POINTERS << 16 |
	     VS_SAMPLER_STATE_CHANGE |
	     GS_SAMPLER_STATE_CHANGE |
	     PS_SAMPLER_STATE_CHANGE |
	     (4 - 2));
   OUT_BATCH(0); /* VS */
   OUT_BATCH(0); /* GS */
   if (brw->wm.sampler_bo)
      OUT_RELOC(brw->wm.sampler_bo, I915_GEM_DOMAIN_INSTRUCTION, 0, 0);
   else
      OUT_BATCH(0);

   ADVANCE_BATCH();

   intel_batchbuffer_emit_mi_flush(intel->batch);
}


static void
prepare_sampler_state_pointers(struct brw_context *brw)
{
   brw_add_validated_bo(brw, brw->wm.sampler_bo);
}

const struct brw_tracked_state gen6_sampler_state = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_BATCH,
      .cache = CACHE_NEW_SAMPLER
   },
   .prepare = prepare_sampler_state_pointers,
   .emit = upload_sampler_state_pointers,
};

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

/**
 * @file gen7_sol_state.c
 *
 * Controls the stream output logic (SOL) stage of the gen7 hardware, which is
 * used to implement GL_EXT_transform_feedback.
 */

#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"
#include "intel_batchbuffer.h"

static void
upload_sol_state(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;

   /* Disable the SOL stage */
   BEGIN_BATCH(3);
   OUT_BATCH(_3DSTATE_STREAMOUT << 16 | (3 - 2));
   OUT_BATCH(0);
   OUT_BATCH(0);
   ADVANCE_BATCH();
}

const struct brw_tracked_state gen7_sol_state = {
   .dirty = {
      .mesa  = 0,
      .brw   = BRW_NEW_BATCH,
      .cache = 0,
   },
   .emit = upload_sol_state,
};

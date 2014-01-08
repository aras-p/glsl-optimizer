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
#include "main/fbobject.h"
#include "main/framebuffer.h"

static void
gen6_upload_scissor_state(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   const bool render_to_fbo = _mesa_is_user_fbo(ctx->DrawBuffer);
   struct gen6_scissor_rect *scissor;
   uint32_t scissor_state_offset;

   scissor = brw_state_batch(brw, AUB_TRACE_SCISSOR_STATE,
			     sizeof(*scissor) * ctx->Const.MaxViewports, 32,
                             &scissor_state_offset);

   /* _NEW_SCISSOR | _NEW_BUFFERS | _NEW_VIEWPORT */

   /* The scissor only needs to handle the intersection of drawable and
    * scissor rect.  Clipping to the boundaries of static shared buffers
    * for front/back/depth is covered by looping over cliprects in brw_draw.c.
    *
    * Note that the hardware's coordinates are inclusive, while Mesa's min is
    * inclusive but max is exclusive.
    */
   for (unsigned i = 0; i < ctx->Const.MaxViewports; i++) {
      int bbox[4];

      _mesa_scissor_bounding_box(ctx, ctx->DrawBuffer, i, bbox);

      if (bbox[0] == bbox[1] || bbox[2] == bbox[3]) {
         /* If the scissor was out of bounds and got clamped to 0 width/height
          * at the bounds, the subtraction of 1 from maximums could produce a
          * negative number and thus not clip anything.  Instead, just provide
          * a min > max scissor inside the bounds, which produces the expected
          * no rendering.
          */
         scissor[i].xmin = 1;
         scissor[i].xmax = 0;
         scissor[i].ymin = 1;
         scissor[i].ymax = 0;
      } else if (render_to_fbo) {
         /* texmemory: Y=0=bottom */
         scissor[i].xmin = bbox[0];
         scissor[i].xmax = bbox[1] - 1;
         scissor[i].ymin = bbox[2];
         scissor[i].ymax = bbox[3] - 1;
      }
      else {
         /* memory: Y=0=top */
         scissor[i].xmin = bbox[0];
         scissor[i].xmax = bbox[1] - 1;
         scissor[i].ymin = ctx->DrawBuffer->Height - bbox[3];
         scissor[i].ymax = ctx->DrawBuffer->Height - bbox[2] - 1;
      }
   }
   BEGIN_BATCH(2);
   OUT_BATCH(_3DSTATE_SCISSOR_STATE_POINTERS << 16 | (2 - 2));
   OUT_BATCH(scissor_state_offset);
   ADVANCE_BATCH();
}

const struct brw_tracked_state gen6_scissor_state = {
   .dirty = {
      .mesa = _NEW_SCISSOR | _NEW_BUFFERS | _NEW_VIEWPORT,
      .brw = BRW_NEW_BATCH,
      .cache = 0,
   },
   .emit = gen6_upload_scissor_state,
};

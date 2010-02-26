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

static void
prepare_scissor_state(struct brw_context *brw)
{
   GLcontext *ctx = &brw->intel.ctx;
   const GLboolean render_to_fbo = (ctx->DrawBuffer->Name != 0);
   struct gen6_scissor_state scissor;

   /* _NEW_SCISSOR | _NEW_BUFFERS | _NEW_VIEWPORT */

   /* The scissor only needs to handle the intersection of drawable and
    * scissor rect.  Clipping to the boundaries of static shared buffers
    * for front/back/depth is covered by looping over cliprects in brw_draw.c.
    *
    * Note that the hardware's coordinates are inclusive, while Mesa's min is
    * inclusive but max is exclusive.
    */
   if (render_to_fbo) {
      /* texmemory: Y=0=bottom */
      scissor.xmin = ctx->DrawBuffer->_Xmin;
      scissor.xmax = ctx->DrawBuffer->_Xmax - 1;
      scissor.ymin = ctx->DrawBuffer->_Ymin;
      scissor.ymax = ctx->DrawBuffer->_Ymax - 1;
   }
   else {
      /* memory: Y=0=top */
      scissor.xmin = ctx->DrawBuffer->_Xmin;
      scissor.xmax = ctx->DrawBuffer->_Xmax - 1;
      scissor.ymin = ctx->DrawBuffer->Height - ctx->DrawBuffer->_Ymax;
      scissor.ymax = ctx->DrawBuffer->Height - ctx->DrawBuffer->_Ymin - 1;
   }

   drm_intel_bo_unreference(brw->sf.state_bo);
   brw->sf.state_bo = brw_cache_data(&brw->cache, BRW_SF_UNIT,
				     &scissor, sizeof(scissor),
				     NULL, 0);
}

const struct brw_tracked_state gen6_scissor_state = {
   .dirty = {
      .mesa = _NEW_SCISSOR | _NEW_BUFFERS | _NEW_VIEWPORT,
      .brw = 0,
      .cache = 0,
   },
   .prepare = prepare_scissor_state,
};

static void upload_scissor_state_pointers(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(2);
   OUT_BATCH(CMD_3D_SCISSOR_STATE_POINTERS << 16 | (2 - 2));
   OUT_RELOC(brw->sf.state_bo, I915_GEM_DOMAIN_INSTRUCTION, 0, 0);
   ADVANCE_BATCH();

   intel_batchbuffer_emit_mi_flush(intel->batch);
}


static void prepare_scissor_state_pointers(struct brw_context *brw)
{
   brw_add_validated_bo(brw, brw->sf.state_bo);
}

const struct brw_tracked_state gen6_scissor_state_pointers = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_BATCH,
      .cache = CACHE_NEW_SF_UNIT
   },
   .prepare = prepare_scissor_state_pointers,
   .emit = upload_scissor_state_pointers,
};

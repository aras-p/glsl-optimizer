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

#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"
#include "intel_batchbuffer.h"
#include "main/fbobject.h"

static void
gen7_upload_sf_clip_viewport(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;
   const GLfloat depth_scale = 1.0F / ctx->DrawBuffer->_DepthMaxF;
   GLfloat y_scale, y_bias;
   const bool render_to_fbo = _mesa_is_user_fbo(ctx->DrawBuffer);
   const GLfloat *v = ctx->Viewport._WindowMap.m;
   struct gen7_sf_clip_viewport *vp;

   vp = brw_state_batch(brw, AUB_TRACE_SF_VP_STATE,
			sizeof(*vp), 64, &brw->sf.vp_offset);
   /* Also assign to clip.vp_offset in case something uses it. */
   brw->clip.vp_offset = brw->sf.vp_offset;

   /* According to the "Vertex X,Y Clamping and Quantization" section of the
    * Strips and Fans documentation, objects must not have a screen-space
    * extents of over 8192 pixels, or they may be mis-rasterized.  The maximum
    * screen space coordinates of a small object may larger, but we have no
    * way to enforce the object size other than through clipping.
    *
    * If you're surprised that we set clip to -gbx to +gbx and it seems like
    * we'll end up with 16384 wide, note that for a 8192-wide render target,
    * we'll end up with a normal (-1, 1) clip volume that just covers the
    * drawable.
    */
   const float maximum_guardband_extent = 8192;
   float gbx = maximum_guardband_extent / (float) ctx->Viewport.Width;
   float gby = maximum_guardband_extent / (float) ctx->Viewport.Height;

   vp->guardband.xmin = -gbx;
   vp->guardband.xmax = gbx;
   vp->guardband.ymin = -gby;
   vp->guardband.ymax = gby;

   /* _NEW_BUFFERS */
   if (render_to_fbo) {
      y_scale = 1.0;
      y_bias = 0;
   } else {
      y_scale = -1.0;
      y_bias = ctx->DrawBuffer->Height;
   }

   /* _NEW_VIEWPORT */
   vp->viewport.m00 = v[MAT_SX];
   vp->viewport.m11 = v[MAT_SY] * y_scale;
   vp->viewport.m22 = v[MAT_SZ] * depth_scale;
   vp->viewport.m30 = v[MAT_TX];
   vp->viewport.m31 = v[MAT_TY] * y_scale + y_bias;
   vp->viewport.m32 = v[MAT_TZ] * depth_scale;

   BEGIN_BATCH(2);
   OUT_BATCH(_3DSTATE_VIEWPORT_STATE_POINTERS_SF_CL << 16 | (2 - 2));
   OUT_BATCH(brw->sf.vp_offset);
   ADVANCE_BATCH();
}

const struct brw_tracked_state gen7_sf_clip_viewport = {
   .dirty = {
      .mesa = _NEW_VIEWPORT | _NEW_BUFFERS,
      .brw = BRW_NEW_BATCH,
      .cache = 0,
   },
   .emit = gen7_upload_sf_clip_viewport,
};

/* ----------------------------------------------------- */

static void upload_cc_viewport_state_pointer(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(2);
   OUT_BATCH(_3DSTATE_VIEWPORT_STATE_POINTERS_CC << 16 | (2 - 2));
   OUT_BATCH(brw->cc.vp_offset);
   ADVANCE_BATCH();
}

const struct brw_tracked_state gen7_cc_viewport_state_pointer = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_BATCH,
      .cache = CACHE_NEW_CC_VP
   },
   .emit = upload_cc_viewport_state_pointer,
};

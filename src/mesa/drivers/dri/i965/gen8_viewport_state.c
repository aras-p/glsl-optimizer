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
gen8_upload_sf_clip_viewport(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   const GLfloat depth_scale = 1.0F / ctx->DrawBuffer->_DepthMaxF;
   float y_scale, y_bias;
   const bool render_to_fbo = _mesa_is_user_fbo(ctx->DrawBuffer);

   float *vp = brw_state_batch(brw, AUB_TRACE_SF_VP_STATE,
                               16 * 4 * ctx->Const.MaxViewports,
                               64, &brw->sf.vp_offset);
   /* Also assign to clip.vp_offset in case something uses it. */
   brw->clip.vp_offset = brw->sf.vp_offset;

   /* _NEW_BUFFERS */
   if (render_to_fbo) {
      y_scale = 1.0;
      y_bias = 0;
   } else {
      y_scale = -1.0;
      y_bias = ctx->DrawBuffer->Height;
   }

   for (unsigned i = 0; i < ctx->Const.MaxViewports; i++) {
      const GLfloat *const v = ctx->ViewportArray[i]._WindowMap.m;

      /* _NEW_VIEWPORT: Viewport Matrix Elements */
      vp[0] = v[MAT_SX];                    /* m00 */
      vp[1] = v[MAT_SY] * y_scale;          /* m11 */
      vp[2] = v[MAT_SZ] * depth_scale;      /* m22 */
      vp[3] = v[MAT_TX];                    /* m30 */
      vp[4] = v[MAT_TY] * y_scale + y_bias; /* m31 */
      vp[5] = v[MAT_TZ] * depth_scale;      /* m32 */

      /* Reserved */
      vp[6] = 0;
      vp[7] = 0;

      /* According to the "Vertex X,Y Clamping and Quantization" section of the
       * Strips and Fans documentation, objects must not have a screen-space
       * extents of over 8192 pixels, or they may be mis-rasterized.  The
       * maximum screen space coordinates of a small object may larger, but we
       * have no way to enforce the object size other than through clipping.
       *
       * The goal is to create the maximum sized guardband (8K x 8K) with the
       * viewport rectangle in the center of the guardband. This looks weird
       * because the hardware wants coordinates that are scaled to the viewport
       * in NDC. In other words, an 8K x 8K viewport would have [-1,1] for X and Y.
       * A 4K viewport would be [-2,2], 2K := [-4,4] etc.
       *
       * --------------------------------
       * |Guardband                     |
       * |                              |
       * |         ------------         |
       * |         |viewport  |         |
       * |         |          |         |
       * |         |          |         |
       * |         |__________|         |
       * |                              |
       * |                              |
       * |______________________________|
       *
       */
      const float maximum_guardband_extent = 8192;
      float gbx = maximum_guardband_extent / ctx->ViewportArray[i].Width;
      float gby = maximum_guardband_extent / ctx->ViewportArray[i].Height;

      /* _NEW_VIEWPORT: Guardband Clipping */
      vp[8]  = -gbx; /* x-min */
      vp[9]  =  gbx; /* x-max */
      vp[10] = -gby; /* y-min */
      vp[11] =  gby; /* y-max */

      /* _NEW_VIEWPORT | _NEW_BUFFERS: Screen Space Viewport
       * The hardware will take the intersection of the drawing rectangle,
       * scissor rectangle, and the viewport extents. We don't need to be
       * smart, and can therefore just program the viewport extents.
       */
      float viewport_Xmax = ctx->ViewportArray[i].X + ctx->ViewportArray[i].Width;
      float viewport_Ymax = ctx->ViewportArray[i].Y + ctx->ViewportArray[i].Height;
      if (render_to_fbo) {
         vp[12] = ctx->ViewportArray[i].X;
         vp[13] = viewport_Xmax - 1;
         vp[14] = ctx->ViewportArray[i].Y;
         vp[15] = viewport_Ymax - 1;
      } else {
         vp[12] = ctx->ViewportArray[i].X;
         vp[13] = viewport_Xmax - 1;
         vp[14] = ctx->DrawBuffer->Height - viewport_Ymax;
         vp[15] = ctx->DrawBuffer->Height - ctx->ViewportArray[i].Y - 1;
      }

      vp += 16;
   }

   BEGIN_BATCH(2);
   OUT_BATCH(_3DSTATE_VIEWPORT_STATE_POINTERS_SF_CL << 16 | (2 - 2));
   OUT_BATCH(brw->sf.vp_offset);
   ADVANCE_BATCH();
}

const struct brw_tracked_state gen8_sf_clip_viewport = {
   .dirty = {
      .mesa = _NEW_BUFFERS | _NEW_VIEWPORT,
      .brw = BRW_NEW_BATCH,
      .cache = 0,
   },
   .emit = gen8_upload_sf_clip_viewport,
};

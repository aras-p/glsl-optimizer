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
#include "brw_util.h"
#include "intel_batchbuffer.h"
#include "main/fbobject.h"

static void
upload_clip_state(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   /* BRW_NEW_META_IN_PROGRESS */
   uint32_t dw1 = brw->meta_in_progress ? 0 : GEN6_CLIP_STATISTICS_ENABLE;
   uint32_t dw2 = 0;

   /* _NEW_BUFFERS */
   struct gl_framebuffer *fb = ctx->DrawBuffer;

   /* CACHE_NEW_WM_PROG */
   if (brw->wm.prog_data->barycentric_interp_modes &
       BRW_WM_NONPERSPECTIVE_BARYCENTRIC_BITS) {
      dw2 |= GEN6_CLIP_NON_PERSPECTIVE_BARYCENTRIC_ENABLE;
   }

   if (brw->gen >= 7)
      dw1 |= GEN7_CLIP_EARLY_CULL;

   if (brw->gen == 7) {
      /* _NEW_POLYGON */
      if ((ctx->Polygon.FrontFace == GL_CCW) ^ _mesa_is_user_fbo(fb))
         dw1 |= GEN7_CLIP_WINDING_CCW;

      if (ctx->Polygon.CullFlag) {
         switch (ctx->Polygon.CullFaceMode) {
         case GL_FRONT:
            dw1 |= GEN7_CLIP_CULLMODE_FRONT;
            break;
         case GL_BACK:
            dw1 |= GEN7_CLIP_CULLMODE_BACK;
            break;
         case GL_FRONT_AND_BACK:
            dw1 |= GEN7_CLIP_CULLMODE_BOTH;
            break;
         default:
            unreachable("Should not get here: invalid CullFlag");
         }
      } else {
         dw1 |= GEN7_CLIP_CULLMODE_NONE;
      }
   }

   if (brw->gen < 8 && !ctx->Transform.DepthClamp)
      dw2 |= GEN6_CLIP_Z_TEST;

   /* _NEW_LIGHT */
   if (ctx->Light.ProvokingVertex == GL_FIRST_VERTEX_CONVENTION) {
      dw2 |=
	 (0 << GEN6_CLIP_TRI_PROVOKE_SHIFT) |
	 (1 << GEN6_CLIP_TRIFAN_PROVOKE_SHIFT) |
	 (0 << GEN6_CLIP_LINE_PROVOKE_SHIFT);
   } else {
      dw2 |=
	 (2 << GEN6_CLIP_TRI_PROVOKE_SHIFT) |
	 (2 << GEN6_CLIP_TRIFAN_PROVOKE_SHIFT) |
	 (1 << GEN6_CLIP_LINE_PROVOKE_SHIFT);
   }

   /* _NEW_TRANSFORM */
   dw2 |= (ctx->Transform.ClipPlanesEnabled <<
           GEN6_USER_CLIP_CLIP_DISTANCES_SHIFT);

   dw2 |= GEN6_CLIP_GB_TEST;

   /* We need to disable guardband clipping if the guardband (which we always
    * program to the maximum screen-space bounding box of 8K x 8K) will be
    * smaller than the viewport.
    *
    * Closely examining the clip determination formulas in the documentation
    * reveals that objects will be discarded entirely if they're outside the
    * (small) guardband, even if they're within the (large) viewport:
    *
    *     TR = TR_GB || TR_VPXY || TR_VPZ || TR_UC || TR_NEGW
    *     TA   = !TR && TA_GB && TA_VPZ && TA_NEGW
    *     MC = !(TA || TR)
    *
    * (TA is "Trivial Accept", TR is "Trivial Reject", MC is "Must Clip".)
    *
    * Disabling guardband clipping removes the TR_GB condition, which means
    * they'll be considered MC ("Must Clip") unless they're rejected for
    * some other reason.
    *
    * Note that there is no TA_VPXY condition.  If there were, objects entirely
    * inside a 16384x16384 viewport would be trivially accepted, breaking the
    * "objects must have a screenspace bounding box not exceeding 8K in the X
    * or Y direction" restriction.  Instead, they're clipped.
    */
   for (unsigned i = 0; i < ctx->Const.MaxViewports; i++) {
      if (ctx->ViewportArray[i].Width > 8192 ||
          ctx->ViewportArray[i].Height > 8192) {
         dw2 &= ~GEN6_CLIP_GB_TEST;
         break;
      }
   }

   /* If the viewport dimensions are smaller than the drawable dimensions,
    * we have to disable guardband clipping prior to Gen8.  We always program
    * the guardband to a fixed size, which is almost always larger than the
    * viewport.  Any geometry which intersects the viewport but lies within
    * the guardband would bypass the 3D clipping stage, so it wouldn't be
    * clipped to the viewport.  Rendering would happen beyond the viewport,
    * but still inside the drawable.
    *
    * Gen8+ introduces a viewport extents test which restricts rendering to
    * the viewport, so we can ignore this restriction.
    */
   if (brw->gen < 8) {
      for (unsigned i = 0; i < ctx->Const.MaxViewports; i++) {
         if (ctx->ViewportArray[i].X != 0 ||
             ctx->ViewportArray[i].Y != 0 ||
             ctx->ViewportArray[i].Width != (float) fb->Width ||
             ctx->ViewportArray[i].Height != (float) fb->Height) {
            dw2 &= ~GEN6_CLIP_GB_TEST;
            break;
         }
      }
   }

   /* BRW_NEW_RASTERIZER_DISCARD */
   if (ctx->RasterDiscard) {
      dw2 |= GEN6_CLIP_MODE_REJECT_ALL;
      perf_debug("Rasterizer discard is currently implemented via the clipper; "
                 "%s be faster.\n", brw->gen >= 7 ? "using the SOL unit may" :
                 "having the GS not write primitives would likely");
   }

   uint32_t enable;
   if (brw->primitive == _3DPRIM_RECTLIST)
      enable = 0;
   else
      enable = GEN6_CLIP_ENABLE;

   BEGIN_BATCH(4);
   OUT_BATCH(_3DSTATE_CLIP << 16 | (4 - 2));
   OUT_BATCH(dw1);
   OUT_BATCH(enable |
	     GEN6_CLIP_API_OGL |
	     GEN6_CLIP_MODE_NORMAL |
	     GEN6_CLIP_XY_TEST |
	     dw2);
   OUT_BATCH(U_FIXED(0.125, 3) << GEN6_CLIP_MIN_POINT_WIDTH_SHIFT |
             U_FIXED(255.875, 3) << GEN6_CLIP_MAX_POINT_WIDTH_SHIFT |
             (fb->MaxNumLayers > 0 ? 0 : GEN6_CLIP_FORCE_ZERO_RTAINDEX) |
             ((ctx->Const.MaxViewports - 1) & GEN6_CLIP_MAX_VP_INDEX_MASK));
   ADVANCE_BATCH();
}

const struct brw_tracked_state gen6_clip_state = {
   .dirty = {
      .mesa  = _NEW_TRANSFORM | _NEW_LIGHT | _NEW_BUFFERS,
      .brw   = BRW_NEW_CONTEXT |
               BRW_NEW_META_IN_PROGRESS |
               BRW_NEW_RASTERIZER_DISCARD,
      .cache = CACHE_NEW_WM_PROG
   },
   .emit = upload_clip_state,
};

const struct brw_tracked_state gen7_clip_state = {
   .dirty = {
      .mesa  = _NEW_BUFFERS | _NEW_LIGHT | _NEW_POLYGON | _NEW_TRANSFORM,
      .brw   = BRW_NEW_CONTEXT |
               BRW_NEW_META_IN_PROGRESS |
               BRW_NEW_RASTERIZER_DISCARD,
      .cache = CACHE_NEW_WM_PROG
   },
   .emit = upload_clip_state,
};

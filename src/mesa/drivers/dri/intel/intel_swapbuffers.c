/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include "intel_blit.h"
#include "intel_buffers.h"
#include "intel_swapbuffers.h"
#include "intel_fbo.h"
#include "intel_batchbuffer.h"
#include "drirenderbuffer.h"
#include "vblank.h"
#include "i915_drm.h"



/*
 * Correct a drawablePrivate's set of vblank flags WRT the current context.
 * When considering multiple crtcs.
 */
GLuint
intelFixupVblank(struct intel_context *intel, __DRIdrawablePrivate *dPriv)
{

   if (!intel->intelScreen->driScrnPriv->dri2.enabled &&
       intel->intelScreen->driScrnPriv->ddx_version.minor >= 7) {
      volatile drm_i915_sarea_t *sarea = intel->sarea;
      drm_clip_rect_t drw_rect = { .x1 = dPriv->x, .x2 = dPriv->x + dPriv->w,
				   .y1 = dPriv->y, .y2 = dPriv->y + dPriv->h };
      drm_clip_rect_t planeA_rect = { .x1 = sarea->planeA_x, .y1 = sarea->planeA_y,
				     .x2 = sarea->planeA_x + sarea->planeA_w,
				     .y2 = sarea->planeA_y + sarea->planeA_h };
      drm_clip_rect_t planeB_rect = { .x1 = sarea->planeB_x, .y1 = sarea->planeB_y,
				     .x2 = sarea->planeB_x + sarea->planeB_w,
				     .y2 = sarea->planeB_y + sarea->planeB_h };
      GLint areaA = driIntersectArea( drw_rect, planeA_rect );
      GLint areaB = driIntersectArea( drw_rect, planeB_rect );
      GLuint flags = dPriv->vblFlags;

      /* Update vblank info
       */
      if (areaB > areaA || (areaA == areaB && areaB > 0)) {
	 flags = dPriv->vblFlags | VBLANK_FLAG_SECONDARY;
      } else {
	 flags = dPriv->vblFlags & ~VBLANK_FLAG_SECONDARY;
      }

      /* Do the stupid test: Is one of them actually disabled?
       */
      if (sarea->planeA_w == 0 || sarea->planeA_h == 0) {
	 flags = dPriv->vblFlags | VBLANK_FLAG_SECONDARY;
      } else if (sarea->planeB_w == 0 || sarea->planeB_h == 0) {
	 flags = dPriv->vblFlags & ~VBLANK_FLAG_SECONDARY;
      }

      return flags;
   } else {
	return dPriv->vblFlags & ~VBLANK_FLAG_SECONDARY;
   }
}


void
intelSwapBuffers(__DRIdrawablePrivate * dPriv)
{
   __DRIscreenPrivate *psp = dPriv->driScreenPriv;

   if (dPriv->driContextPriv && dPriv->driContextPriv->driverPrivate) {
      GET_CURRENT_CONTEXT(ctx);
      struct intel_context *intel;

      if (ctx == NULL)
	 return;

      intel = intel_context(ctx);

      if (ctx->Visual.doubleBufferMode) {
	 GLboolean missed_target;
	 struct intel_framebuffer *intel_fb = dPriv->driverPrivate;
	 int64_t ust;
         
	 _mesa_notifySwapBuffers(ctx);  /* flush pending rendering comands */

	/*
	 * The old swapping ioctl was incredibly racy, just wait for vblank
	 * and do the swap ourselves.
	 */
	 driWaitForVBlank(dPriv, &missed_target);

	 /*
	  * Update each buffer's vbl_pending so we don't get too out of
	  * sync
	  */
	 intel_get_renderbuffer(&intel_fb->Base,
		   		BUFFER_BACK_LEFT)->vbl_pending = dPriv->vblSeq;
         intel_get_renderbuffer(&intel_fb->Base,
		   		BUFFER_FRONT_LEFT)->vbl_pending = dPriv->vblSeq;

	 intelCopyBuffer(dPriv, NULL);

	 intel_fb->swap_count++;
	 (*psp->systemTime->getUST) (&ust);
	 if (missed_target) {
	    intel_fb->swap_missed_count++;
	    intel_fb->swap_missed_ust = ust - intel_fb->swap_ust;
	 }

	 intel_fb->swap_ust = ust;
      }
      drmCommandNone(intel->driFd, DRM_I915_GEM_THROTTLE);

   }
   else {
      /* XXX this shouldn't be an error but we can't handle it for now */
      fprintf(stderr, "%s: drawable has no context!\n", __FUNCTION__);
   }
}

void
intelCopySubBuffer(__DRIdrawablePrivate * dPriv, int x, int y, int w, int h)
{
   if (dPriv->driContextPriv && dPriv->driContextPriv->driverPrivate) {
      struct intel_context *intel =
         (struct intel_context *) dPriv->driContextPriv->driverPrivate;
      GLcontext *ctx = &intel->ctx;

      if (ctx->Visual.doubleBufferMode) {
         drm_clip_rect_t rect;
         rect.x1 = x + dPriv->x;
         rect.y1 = (dPriv->h - y - h) + dPriv->y;
         rect.x2 = rect.x1 + w;
         rect.y2 = rect.y1 + h;
         _mesa_notifySwapBuffers(ctx);  /* flush pending rendering comands */
         intelCopyBuffer(dPriv, &rect);
      }
   }
   else {
      /* XXX this shouldn't be an error but we can't handle it for now */
      fprintf(stderr, "%s: drawable has no context!\n", __FUNCTION__);
   }
}


/**
 * This will be called whenever the currently bound window is moved/resized.
 * XXX: actually, it seems to NOT be called when the window is only moved (BP).
 */
void
intelWindowMoved(struct intel_context *intel)
{
   GLcontext *ctx = &intel->ctx;
   __DRIdrawablePrivate *dPriv = intel->driDrawable;
   struct intel_framebuffer *intel_fb = dPriv->driverPrivate;

   if (!intel->intelScreen->driScrnPriv->dri2.enabled &&
       intel->intelScreen->driScrnPriv->ddx_version.minor >= 7) {
      GLuint flags = intelFixupVblank(intel, dPriv);

      /* Check to see if we changed pipes */
      if (flags != dPriv->vblFlags && dPriv->vblFlags &&
	  !(dPriv->vblFlags & VBLANK_FLAG_NO_IRQ)) {
	 int64_t count;
	 drmVBlank vbl;
	 int i;

	 /*
	  * Deal with page flipping
	  */
	 vbl.request.type = DRM_VBLANK_ABSOLUTE;

	 if ( dPriv->vblFlags & VBLANK_FLAG_SECONDARY ) {
	    vbl.request.type |= DRM_VBLANK_SECONDARY;
	 }

	 for (i = 0; i < 2; i++) {
	    if (!intel_fb->color_rb[i] ||
		(intel_fb->vbl_waited - intel_fb->color_rb[i]->vbl_pending) <=
		(1<<23))
	       continue;

	    vbl.request.sequence = intel_fb->color_rb[i]->vbl_pending;
	    drmWaitVBlank(intel->driFd, &vbl);
	 }

	 /*
	  * Update msc_base from old pipe
	  */
	 driDrawableGetMSC32(dPriv->driScreenPriv, dPriv, &count);
	 dPriv->msc_base = count;
	 /*
	  * Then get new vblank_base and vblSeq values
	  */
	 dPriv->vblFlags = flags;
	 driGetCurrentVBlank(dPriv);
	 dPriv->vblank_base = dPriv->vblSeq;

	 intel_fb->vbl_waited = dPriv->vblSeq;

	 for (i = 0; i < 2; i++) {
	    if (intel_fb->color_rb[i])
	       intel_fb->color_rb[i]->vbl_pending = intel_fb->vbl_waited;
	 }
      }
   } else {
      dPriv->vblFlags &= ~VBLANK_FLAG_SECONDARY;
   }

   /* Update Mesa's notion of window size */
   driUpdateFramebufferSize(ctx, dPriv);
   intel_fb->Base.Initialized = GL_TRUE; /* XXX remove someday */

   /* Update hardware scissor */
   if (ctx->Driver.Scissor != NULL) {
      ctx->Driver.Scissor(ctx, ctx->Scissor.X, ctx->Scissor.Y,
			  ctx->Scissor.Width, ctx->Scissor.Height);
   }

   /* Re-calculate viewport related state */
   if (ctx->Driver.DepthRange != NULL)
      ctx->Driver.DepthRange( ctx, ctx->Viewport.Near, ctx->Viewport.Far );
}

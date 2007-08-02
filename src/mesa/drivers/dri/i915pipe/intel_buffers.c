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

#include "intel_screen.h"
#include "intel_context.h"
#include "intel_blit.h"
#include "intel_buffers.h"
#include "intel_depthstencil.h"
#include "intel_fbo.h"
#include "intel_batchbuffer.h"
#include "intel_reg.h"
#include "context.h"
#include "utils.h"
#include "drirenderbuffer.h"
#include "framebuffer.h"
#include "swrast/swrast.h"
#include "vblank.h"

#include "pipe/p_context.h"

/* This block can be removed when libdrm >= 2.3.1 is required */

#ifndef DRM_VBLANK_FLIP

#define DRM_VBLANK_FLIP 0x8000000

typedef struct drm_i915_flip {
   int pipes;
} drm_i915_flip_t;

#undef DRM_IOCTL_I915_FLIP
#define DRM_IOCTL_I915_FLIP DRM_IOW(DRM_COMMAND_BASE + DRM_I915_FLIP, \
				    drm_i915_flip_t)

#endif


/**
 * XXX move this into a new dri/common/cliprects.c file.
 */
GLboolean
intel_intersect_cliprects(drm_clip_rect_t * dst,
                          const drm_clip_rect_t * a,
                          const drm_clip_rect_t * b)
{
   GLint bx = b->x1;
   GLint by = b->y1;
   GLint bw = b->x2 - bx;
   GLint bh = b->y2 - by;

   if (bx < a->x1)
      bw -= a->x1 - bx, bx = a->x1;
   if (by < a->y1)
      bh -= a->y1 - by, by = a->y1;
   if (bx + bw > a->x2)
      bw = a->x2 - bx;
   if (by + bh > a->y2)
      bh = a->y2 - by;
   if (bw <= 0)
      return GL_FALSE;
   if (bh <= 0)
      return GL_FALSE;

   dst->x1 = bx;
   dst->y1 = by;
   dst->x2 = bx + bw;
   dst->y2 = by + bh;

   return GL_TRUE;
}

/**
 * Return pointer to current color drawing region, or NULL.
 */
struct pipe_region *
intel_drawbuf_region(struct intel_context *intel)
{
   struct intel_renderbuffer *irbColor =
      intel_renderbuffer(intel->ctx.DrawBuffer->_ColorDrawBuffers[0][0]);
   if (irbColor)
      return irbColor->region;
   else
      return NULL;
}

/**
 * Return pointer to current color reading region, or NULL.
 */
struct pipe_region *
intel_readbuf_region(struct intel_context *intel)
{
   struct intel_renderbuffer *irb
      = intel_renderbuffer(intel->ctx.ReadBuffer->_ColorReadBuffer);
   if (irb)
      return irb->region;
   else
      return NULL;
}



/**
 * Update the following fields for rendering:
 *   intel->numClipRects
 *   intel->pClipRects
 */
static void
intelSetRenderbufferClipRects(struct intel_context *intel)
{
   /* zero-sized buffers might be legal? */
   assert(intel->ctx.DrawBuffer->Width > 0);
   assert(intel->ctx.DrawBuffer->Height > 0);
   intel->fboRect.x1 = 0;
   intel->fboRect.y1 = 0;
   intel->fboRect.x2 = intel->ctx.DrawBuffer->Width;
   intel->fboRect.y2 = intel->ctx.DrawBuffer->Height;
   intel->numClipRects = 1;
   intel->pClipRects = &intel->fboRect;
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

   if (!intel->ctx.DrawBuffer) {
      /* when would this happen? -BP */
      assert(0);
      intel->numClipRects = 0;
   }

   /* Update Mesa's notion of window size */
   driUpdateFramebufferSize(ctx, dPriv);
   intel_fb->Base.Initialized = GL_TRUE; /* XXX remove someday */

   {
      drmI830Sarea *sarea = intel->sarea;
      drm_clip_rect_t drw_rect = { .x1 = dPriv->x, .x2 = dPriv->x + dPriv->w,
				   .y1 = dPriv->y, .y2 = dPriv->y + dPriv->h };
      drm_clip_rect_t pipeA_rect = { .x1 = sarea->pipeA_x, .y1 = sarea->pipeA_y,
				     .x2 = sarea->pipeA_x + sarea->pipeA_w,
				     .y2 = sarea->pipeA_y + sarea->pipeA_h };
      drm_clip_rect_t pipeB_rect = { .x1 = sarea->pipeB_x, .y1 = sarea->pipeB_y,
				     .x2 = sarea->pipeB_x + sarea->pipeB_w,
				     .y2 = sarea->pipeB_y + sarea->pipeB_h };
      GLint areaA = driIntersectArea( drw_rect, pipeA_rect );
      GLint areaB = driIntersectArea( drw_rect, pipeB_rect );
      GLuint flags = intel_fb->vblank_flags;
      GLboolean pf_active;
      GLint pf_pipes;

      /* Update page flipping info
       */
      pf_pipes = 0;

      if (areaA > 0)
	 pf_pipes |= 1;

      if (areaB > 0)
	 pf_pipes |= 2;

      intel_fb->pf_current_page = (intel->sarea->pf_current_page >>
				   (intel_fb->pf_pipes & 0x2)) & 0x3;

      intel_fb->pf_num_pages = 2 /*intel->intelScreen->third.handle ? 3 : 2*/;

      pf_active = pf_pipes && (pf_pipes & intel->sarea->pf_active) == pf_pipes;

      if (INTEL_DEBUG & DEBUG_LOCK)
	 if (pf_active != intel_fb->pf_active)
	    _mesa_printf("%s - Page flipping %sactive\n", __progname,
			 pf_active ? "" : "in");

      if (pf_active) {
	 /* Sync pages between pipes if we're flipping on both at the same time */
	 if (pf_pipes == 0x3 &&	pf_pipes != intel_fb->pf_pipes &&
	     (intel->sarea->pf_current_page & 0x3) !=
	     (((intel->sarea->pf_current_page) >> 2) & 0x3)) {
	    drm_i915_flip_t flip;

	    if (intel_fb->pf_current_page ==
		(intel->sarea->pf_current_page & 0x3)) {
	       /* XXX: This is ugly, but emitting two flips 'in a row' can cause
		* lockups for unknown reasons.
		*/
               intel->sarea->pf_current_page =
		  intel->sarea->pf_current_page & 0x3;
	       intel->sarea->pf_current_page |=
		  ((intel_fb->pf_current_page + intel_fb->pf_num_pages - 1) %
		   intel_fb->pf_num_pages) << 2;

	       flip.pipes = 0x2;
	    } else {
               intel->sarea->pf_current_page =
		  intel->sarea->pf_current_page & (0x3 << 2);
	       intel->sarea->pf_current_page |=
		  (intel_fb->pf_current_page + intel_fb->pf_num_pages - 1) %
		  intel_fb->pf_num_pages;

	       flip.pipes = 0x1;
	    }

	    drmCommandWrite(intel->driFd, DRM_I915_FLIP, &flip, sizeof(flip));
	 }

	 intel_fb->pf_pipes = pf_pipes;
      }

      intel_fb->pf_active = pf_active;
      intel_flip_renderbuffers(intel_fb);
      intel_draw_buffer(&intel->ctx, intel->ctx.DrawBuffer);

      /* Update vblank info
       */
      if (areaB > areaA || (areaA == areaB && areaB > 0)) {
	 flags = intel_fb->vblank_flags | VBLANK_FLAG_SECONDARY;
      } else {
	 flags = intel_fb->vblank_flags & ~VBLANK_FLAG_SECONDARY;
      }

      if (flags != intel_fb->vblank_flags && intel_fb->vblank_flags &&
	  !(intel_fb->vblank_flags & VBLANK_FLAG_NO_IRQ)) {
	 drmVBlank vbl;
	 int i;

	 vbl.request.type = DRM_VBLANK_ABSOLUTE;

	 if ( intel_fb->vblank_flags & VBLANK_FLAG_SECONDARY ) {
	    vbl.request.type |= DRM_VBLANK_SECONDARY;
	 }

	 for (i = 0; i < intel_fb->pf_num_pages; i++) {
	    if (!intel_fb->color_rb[i] ||
		(intel_fb->vbl_waited - intel_fb->color_rb[i]->vbl_pending) <=
		(1<<23))
	       continue;

	    vbl.request.sequence = intel_fb->color_rb[i]->vbl_pending;
	    drmWaitVBlank(intel->driFd, &vbl);
	 }

	 intel_fb->vblank_flags = flags;
	 driGetCurrentVBlank(dPriv, intel_fb->vblank_flags, &intel_fb->vbl_seq);
	 intel_fb->vbl_waited = intel_fb->vbl_seq;

	 for (i = 0; i < intel_fb->pf_num_pages; i++) {
	    if (intel_fb->color_rb[i])
	       intel_fb->color_rb[i]->vbl_pending = intel_fb->vbl_waited;
	 }
      }
   }

   /* This will be picked up by looking at the dirty state flags:
    */

   /* Update hardware scissor */
//   ctx->Driver.Scissor(ctx, ctx->Scissor.X, ctx->Scissor.Y,
//                       ctx->Scissor.Width, ctx->Scissor.Height);

   /* Re-calculate viewport related state */
//   ctx->Driver.DepthRange( ctx, ctx->Viewport.Near, ctx->Viewport.Far );
}





/* Emit wait for pending flips */
void
intel_wait_flips(struct intel_context *intel, GLuint batch_flags)
{
   struct intel_framebuffer *intel_fb =
      (struct intel_framebuffer *) intel->ctx.DrawBuffer;
   struct intel_renderbuffer *intel_rb =
      intel_get_renderbuffer(&intel_fb->Base,
			     intel_fb->Base._ColorDrawBufferMask[0] ==
			     BUFFER_BIT_FRONT_LEFT ? BUFFER_FRONT_LEFT :
			     BUFFER_BACK_LEFT);

   if (intel_fb->Base.Name == 0 && intel_rb->pf_pending == intel_fb->pf_seq) {
      GLint pf_pipes = intel_fb->pf_pipes;
      BATCH_LOCALS;

      /* Wait for pending flips to take effect */
      BEGIN_BATCH(2, batch_flags);
      OUT_BATCH(pf_pipes & 0x1 ? (MI_WAIT_FOR_EVENT | MI_WAIT_FOR_PLANE_A_FLIP)
		: 0);
      OUT_BATCH(pf_pipes & 0x2 ? (MI_WAIT_FOR_EVENT | MI_WAIT_FOR_PLANE_B_FLIP)
		: 0);
      ADVANCE_BATCH();

      intel_rb->pf_pending--;
   }
}

#if 0
/* Flip the front & back buffers
 */
static GLboolean
intelPageFlip(const __DRIdrawablePrivate * dPriv)
{
   struct intel_context *intel;
   int ret;
   struct intel_framebuffer *intel_fb = dPriv->driverPrivate;

   if (INTEL_DEBUG & DEBUG_IOCTL)
      fprintf(stderr, "%s\n", __FUNCTION__);

   assert(dPriv);
   assert(dPriv->driContextPriv);
   assert(dPriv->driContextPriv->driverPrivate);

   intel = (struct intel_context *) dPriv->driContextPriv->driverPrivate;

   if (intel->intelScreen->drmMinor < 9)
      return GL_FALSE;

   intelFlush(&intel->ctx);

   ret = 0;

   LOCK_HARDWARE(intel);

   if (dPriv->numClipRects && intel_fb->pf_active) {
      drm_i915_flip_t flip;

      flip.pipes = intel_fb->pf_pipes;

      ret = drmCommandWrite(intel->driFd, DRM_I915_FLIP, &flip, sizeof(flip));
   }

   UNLOCK_HARDWARE(intel);

   if (ret || !intel_fb->pf_active)
      return GL_FALSE;

   if (!dPriv->numClipRects) {
      usleep(10000);	/* throttle invisible client 10ms */
   }

   intel_fb->pf_current_page = (intel->sarea->pf_current_page >>
				(intel_fb->pf_pipes & 0x2)) & 0x3;

   if (dPriv->numClipRects != 0) {
      intel_get_renderbuffer(&intel_fb->Base, BUFFER_FRONT_LEFT)->pf_pending =
      intel_get_renderbuffer(&intel_fb->Base, BUFFER_BACK_LEFT)->pf_pending =
	 ++intel_fb->pf_seq;
   }

   intel_flip_renderbuffers(intel_fb);
   intel_draw_buffer(&intel->ctx, &intel_fb->Base);

   if (INTEL_DEBUG & DEBUG_IOCTL)
      fprintf(stderr, "%s: success\n", __FUNCTION__);

   return GL_TRUE;
}
#endif


static GLboolean
intelScheduleSwap(const __DRIdrawablePrivate * dPriv, GLboolean *missed_target)
{
   struct intel_framebuffer *intel_fb = dPriv->driverPrivate;
   unsigned int interval = driGetVBlankInterval(dPriv, intel_fb->vblank_flags);
   struct intel_context *intel =
      intelScreenContext(dPriv->driScreenPriv->private);
   const intelScreenPrivate *intelScreen = intel->intelScreen;
   unsigned int target;
   drm_i915_vblank_swap_t swap;
   GLboolean ret;

   if (!intel_fb->vblank_flags ||
       (intel_fb->vblank_flags & VBLANK_FLAG_NO_IRQ) ||
       intelScreen->drmMinor < (intel_fb->pf_active ? 9 : 6))
      return GL_FALSE;

   swap.seqtype = DRM_VBLANK_ABSOLUTE;

   if (intel_fb->vblank_flags & VBLANK_FLAG_SYNC) {
      swap.seqtype |= DRM_VBLANK_NEXTONMISS;
   } else if (interval == 0) {
      return GL_FALSE;
   }

   swap.drawable = dPriv->hHWDrawable;
   target = swap.sequence = intel_fb->vbl_seq + interval;

   if ( intel_fb->vblank_flags & VBLANK_FLAG_SECONDARY ) {
      swap.seqtype |= DRM_VBLANK_SECONDARY;
   }

   LOCK_HARDWARE(intel);

   intel_batchbuffer_flush(intel->batch);

   if ( intel_fb->pf_active ) {
      swap.seqtype |= DRM_VBLANK_FLIP;

      intel_fb->pf_current_page = (((intel->sarea->pf_current_page >>
				     (intel_fb->pf_pipes & 0x2)) & 0x3) + 1) %
				  intel_fb->pf_num_pages;
   }

   if (!drmCommandWriteRead(intel->driFd, DRM_I915_VBLANK_SWAP, &swap,
			    sizeof(swap))) {
      intel_fb->vbl_seq = swap.sequence;
      swap.sequence -= target;
      *missed_target = swap.sequence > 0 && swap.sequence <= (1 << 23);

      intel_get_renderbuffer(&intel_fb->Base, BUFFER_BACK_LEFT)->vbl_pending =
	 intel_get_renderbuffer(&intel_fb->Base,
				BUFFER_FRONT_LEFT)->vbl_pending =
	 intel_fb->vbl_seq;

      if (swap.seqtype & DRM_VBLANK_FLIP) {
	 intel_flip_renderbuffers(intel_fb);
	 intel_draw_buffer(&intel->ctx, intel->ctx.DrawBuffer);
      }

      ret = GL_TRUE;
   } else {
      if (swap.seqtype & DRM_VBLANK_FLIP) {
	 intel_fb->pf_current_page = ((intel->sarea->pf_current_page >>
					(intel_fb->pf_pipes & 0x2)) & 0x3) %
				     intel_fb->pf_num_pages;
      }

      ret = GL_FALSE;
   }

   UNLOCK_HARDWARE(intel);

   return ret;
}

void
intelSwapBuffers(__DRIdrawablePrivate * dPriv)
{
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

         if (!intelScheduleSwap(dPriv, &missed_target)) {
	    driWaitForVBlank(dPriv, &intel_fb->vbl_seq, intel_fb->vblank_flags,
			     &missed_target);

            intelCopyBuffer(dPriv, NULL);
	 }

	 intel_fb->swap_count++;
	 (*dri_interface->getUST) (&ust);
	 if (missed_target) {
	    intel_fb->swap_missed_count++;
	    intel_fb->swap_missed_ust = ust - intel_fb->swap_ust;
	 }

	 intel_fb->swap_ust = ust;
      }
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
	 /* fixup cliprect (driDrawable may have changed?) later */
         rect.x1 = x;
         rect.y1 = y;
         rect.x2 = w;
         rect.y2 = h;
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
 * Update the hardware state for drawing into a window or framebuffer object.
 *
 * Called by glDrawBuffer, glBindFramebufferEXT, MakeCurrent, and other
 * places within the driver.
 *
 * Basically, this needs to be called any time the current framebuffer
 * changes, the renderbuffers change, or we need to draw into different
 * color buffers.
 */
void
intel_draw_buffer(GLcontext * ctx, struct gl_framebuffer *fb)
{
   struct intel_context *intel = intel_context(ctx);
   struct pipe_region *colorRegion, *depthRegion = NULL;
   struct intel_renderbuffer *irbDepth = NULL, *irbStencil = NULL;

   if (!fb) {
      /* this can happen during the initial context initialization */
      return;
   }

   /* Do this here, not core Mesa, since this function is called from
    * many places within the driver.
    */
   if (ctx->NewState & (_NEW_BUFFERS | _NEW_COLOR | _NEW_PIXEL)) {
      /* this updates the DrawBuffer->_NumColorDrawBuffers fields, etc */
      _mesa_update_framebuffer(ctx);
      /* this updates the DrawBuffer's Width/Height if it's a FBO */
      _mesa_update_draw_buffer_bounds(ctx);
   }

   if (fb->_Status != GL_FRAMEBUFFER_COMPLETE_EXT) {
      /* this may occur when we're called by glBindFrameBuffer() during
       * the process of someone setting up renderbuffers, etc.
       */
      /*_mesa_debug(ctx, "DrawBuffer: incomplete user FBO\n");*/
      return;
   }

   if (fb->Name)
      intel_validate_paired_depth_stencil(ctx, fb);

   /*
    * How many color buffers are we drawing into?
    */
   if (fb->_NumColorDrawBuffers[0] != 1) {
      /* writing to 0 or 2 or 4 color buffers */
      /*_mesa_debug(ctx, "Software rendering\n");*/
      FALLBACK(intel, INTEL_FALLBACK_DRAW_BUFFER, GL_TRUE);
   }
   else {
      /* draw to exactly one color buffer */
      /*_mesa_debug(ctx, "Hardware rendering\n");*/
      FALLBACK(intel, INTEL_FALLBACK_DRAW_BUFFER, GL_FALSE);
   }

   /*
    * Get the intel_renderbuffer for the colorbuffer we're drawing into.
    * And set up cliprects.
    */
   {
      struct intel_renderbuffer *irb;
      intelSetRenderbufferClipRects(intel);
      irb = intel_renderbuffer(fb->_ColorDrawBuffers[0][0]);
      colorRegion = (irb && irb->region) ? irb->region : NULL;
   }

   /* Update culling direction which changes depending on the
    * orientation of the buffer:
    */
   if (ctx->Driver.FrontFace)
      ctx->Driver.FrontFace(ctx, ctx->Polygon.FrontFace);
   else
      ctx->NewState |= _NEW_POLYGON;

   if (!colorRegion) {
      FALLBACK(intel, INTEL_FALLBACK_DRAW_BUFFER, GL_TRUE);
   }
   else {
      FALLBACK(intel, INTEL_FALLBACK_DRAW_BUFFER, GL_FALSE);
   }

   /***
    *** Get depth buffer region and check if we need a software fallback.
    *** Note that the depth buffer is usually a DEPTH_STENCIL buffer.
    ***/
   if (fb->_DepthBuffer && fb->_DepthBuffer->Wrapped) {
      irbDepth = intel_renderbuffer(fb->_DepthBuffer->Wrapped);
      if (irbDepth && irbDepth->region) {
         FALLBACK(intel, INTEL_FALLBACK_DEPTH_BUFFER, GL_FALSE);
         depthRegion = irbDepth->region;
      }
      else {
         FALLBACK(intel, INTEL_FALLBACK_DEPTH_BUFFER, GL_TRUE);
         depthRegion = NULL;
      }
   }
   else {
      /* not using depth buffer */
      FALLBACK(intel, INTEL_FALLBACK_DEPTH_BUFFER, GL_FALSE);
      depthRegion = NULL;
   }

   /***
    *** Stencil buffer
    *** This can only be hardware accelerated if we're using a
    *** combined DEPTH_STENCIL buffer (for now anyway).
    ***/
   if (fb->_StencilBuffer && fb->_StencilBuffer->Wrapped) {
      irbStencil = intel_renderbuffer(fb->_StencilBuffer->Wrapped);
      if (irbStencil && irbStencil->region) {
         ASSERT(irbStencil->Base._ActualFormat == GL_DEPTH24_STENCIL8_EXT);
         FALLBACK(intel, INTEL_FALLBACK_STENCIL_BUFFER, GL_FALSE);
         /* need to re-compute stencil hw state */
//         ctx->Driver.Enable(ctx, GL_STENCIL_TEST, ctx->Stencil.Enabled);
         if (!depthRegion)
            depthRegion = irbStencil->region;
      }
      else {
         FALLBACK(intel, INTEL_FALLBACK_STENCIL_BUFFER, GL_TRUE);
      }
   }
   else {
      /* XXX FBO: instead of FALSE, pass ctx->Stencil.Enabled ??? */
      FALLBACK(intel, INTEL_FALLBACK_STENCIL_BUFFER, GL_FALSE);
      /* need to re-compute stencil hw state */
//      ctx->Driver.Enable(ctx, GL_STENCIL_TEST, ctx->Stencil.Enabled);
   }


   /**
    ** Release old regions, reference new regions
    **/

   //  intel->vtbl.set_draw_region(intel, colorRegion, depthRegion);

   /* update viewport since it depends on window size */
//   ctx->Driver.Viewport(ctx, ctx->Viewport.X, ctx->Viewport.Y,
//                        ctx->Viewport.Width, ctx->Viewport.Height);

   /* Update hardware scissor */
//   ctx->Driver.Scissor(ctx, ctx->Scissor.X, ctx->Scissor.Y,
//                       ctx->Scissor.Width, ctx->Scissor.Height);
}


static void
intelDrawBuffer(GLcontext * ctx, GLenum mode)
{
   intel_draw_buffer(ctx, ctx->DrawBuffer);
}


static void
intelReadBuffer(GLcontext * ctx, GLenum mode)
{
   if (ctx->ReadBuffer == ctx->DrawBuffer) {
      /* This will update FBO completeness status.
       * A framebuffer will be incomplete if the GL_READ_BUFFER setting
       * refers to a missing renderbuffer.  Calling glReadBuffer can set
       * that straight and can make the drawing buffer complete.
       */
      intel_draw_buffer(ctx, ctx->DrawBuffer);
   }
   /* Generally, functions which read pixels (glReadPixels, glCopyPixels, etc)
    * reference ctx->ReadBuffer and do appropriate state checks.
    */
}


void
intelInitBufferFuncs(struct dd_function_table *functions)
{
   functions->DrawBuffer = intelDrawBuffer;
   functions->ReadBuffer = intelReadBuffer;
}

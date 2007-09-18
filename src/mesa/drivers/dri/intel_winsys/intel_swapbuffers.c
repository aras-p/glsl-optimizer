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
#include "intel_swapbuffers.h"
#include "intel_batchbuffer.h"
#include "intel_reg.h"
#include "context.h"
#include "utils.h"
#include "drirenderbuffer.h"
#include "vblank.h"

#include "pipe/p_context.h"
#include "state_tracker/st_cb_fbo.h"


/* This block can be removed when libdrm >= 2.3.1 is required */

#ifndef DRM_VBLANK_FLIP

#define DRM_VBLANK_FLIP 0x8000000

typedef struct drm_i915_flip {
   int planes;
} drm_i915_flip_t;

#undef DRM_IOCTL_I915_FLIP
#define DRM_IOCTL_I915_FLIP DRM_IOW(DRM_COMMAND_BASE + DRM_I915_FLIP, \
				    drm_i915_flip_t)

#endif




/**
 * Copy the back color buffer to the front color buffer. 
 * Used for SwapBuffers().
 */
void
intelCopyBuffer(__DRIdrawablePrivate * dPriv,
                const drm_clip_rect_t * rect)
{

   struct intel_context *intel;
   const intelScreenPrivate *intelScreen;

   DBG(SWAP, "%s\n", __FUNCTION__);

   assert(dPriv);

   intel = intelScreenContext(dPriv->driScreenPriv->private);
   if (!intel)
      return;

   intelScreen = intel->intelScreen;

   if (intel->last_swap_fence) {
      driFenceFinish(intel->last_swap_fence, DRM_FENCE_TYPE_EXE, GL_TRUE);
      driFenceUnReference(intel->last_swap_fence);
      intel->last_swap_fence = NULL;
   }
   intel->last_swap_fence = intel->first_swap_fence;
   intel->first_swap_fence = NULL;

   /* The LOCK_HARDWARE is required for the cliprects.  Buffer offsets
    * should work regardless.
    */
   LOCK_HARDWARE(intel);
   /* if this drawable isn't currently bound the LOCK_HARDWARE done on the
      current context (which is what intelScreenContext should return) might
      not get a contended lock and thus cliprects not updated (tests/manywin) */
      if ((struct intel_context *)dPriv->driContextPriv->driverPrivate != intel)
         DRI_VALIDATE_DRAWABLE_INFO(intel->driScreen, dPriv);


   if (dPriv && dPriv->numClipRects) {
      struct intel_framebuffer *intel_fb = dPriv->driverPrivate;
#if 0
      const struct pipe_region *backRegion
	 = intel_fb->Base._ColorDrawBufferMask[0] == BUFFER_BIT_FRONT_LEFT ?
	   intel_get_rb_region(&intel_fb->Base, BUFFER_FRONT_LEFT) :
	   intel_get_rb_region(&intel_fb->Base, BUFFER_BACK_LEFT);
#endif
      const int backWidth = intel_fb->Base.Width;
      const int backHeight = intel_fb->Base.Height;
      const int nbox = dPriv->numClipRects;
      const drm_clip_rect_t *pbox = dPriv->pClipRects;
      const int pitch = intelScreen->front.pitch / intelScreen->front.cpp;
#if 0
      const int srcpitch = backRegion->pitch;
#endif
      const int cpp = intelScreen->front.cpp;
      int BR13, CMD;
      int i;

      const struct pipe_surface *backSurf;
      const struct pipe_region *backRegion;
      int srcpitch;
      struct st_renderbuffer *strb;

      /* blit from back color buffer if it exists, else front buffer */
      strb = st_renderbuffer(intel_fb->Base.Attachment[BUFFER_BACK_LEFT].Renderbuffer);
      if (strb) {
         backSurf = strb->surface;
      }
      else {
         strb = st_renderbuffer(intel_fb->Base.Attachment[BUFFER_FRONT_LEFT].Renderbuffer);
         backSurf = strb->surface;
      }

      backRegion = backSurf->region;
      srcpitch = backRegion->pitch;

      ASSERT(intel_fb);
      ASSERT(intel_fb->Base.Name == 0);    /* Not a user-created FBO */
      ASSERT(backRegion);
      ASSERT(backRegion->cpp == cpp);

      DBG(SWAP, "front pitch %d back pitch %d\n",
	  pitch, backRegion->pitch);

      if (cpp == 2) {
	 BR13 = (pitch * cpp) | (0xCC << 16) | (1 << 24);
	 CMD = XY_SRC_COPY_BLT_CMD;
      }
      else {
	 BR13 = (pitch * cpp) | (0xCC << 16) | (1 << 24) | (1 << 25);
	 CMD = (XY_SRC_COPY_BLT_CMD | XY_SRC_COPY_BLT_WRITE_ALPHA |
		XY_SRC_COPY_BLT_WRITE_RGB);
      }

      for (i = 0; i < nbox; i++, pbox++) {
	 drm_clip_rect_t box;
	 drm_clip_rect_t sbox;

	 if (pbox->x1 > pbox->x2 ||
	     pbox->y1 > pbox->y2 ||
	     pbox->x2 > intelScreen->front.width || 
	     pbox->y2 > intelScreen->front.height)
	    continue;

	 box = *pbox;

	 if (rect) {
	    drm_clip_rect_t rrect;

	    rrect.x1 = dPriv->x + rect->x1;
	    rrect.y1 = (dPriv->h - rect->y1 - rect->y2) + dPriv->y;
	    rrect.x2 = rect->x2 + rrect.x1;
	    rrect.y2 = rect->y2 + rrect.y1;
	    if (rrect.x1 > box.x1)
	       box.x1 = rrect.x1;
	    if (rrect.y1 > box.y1)
	       box.y1 = rrect.y1;
	    if (rrect.x2 < box.x2)
	       box.x2 = rrect.x2;
	    if (rrect.y2 < box.y2)
	       box.y2 = rrect.y2;

	    if (box.x1 > box.x2 || box.y1 > box.y2)
	       continue;
	 }

	 /* restrict blit to size of actually rendered area */
	 if (box.x2 - box.x1 > backWidth)
	    box.x2 = backWidth + box.x1;
	 if (box.y2 - box.y1 > backHeight)
	    box.y2 = backHeight + box.y1;

	 DBG(SWAP, "box x1 x2 y1 y2 %d %d %d %d\n",
	     box.x1, box.x2, box.y1, box.y2);

	 sbox.x1 = box.x1 - dPriv->x;
	 sbox.y1 = box.y1 - dPriv->y;

	 BEGIN_BATCH(8, INTEL_BATCH_NO_CLIPRECTS);
	 OUT_BATCH(CMD);
	 OUT_BATCH(BR13);
	 OUT_BATCH((box.y1 << 16) | box.x1);
	 OUT_BATCH((box.y2 << 16) | box.x2);

	 OUT_RELOC(intelScreen->front.buffer, 
		   DRM_BO_FLAG_MEM_TT | DRM_BO_FLAG_WRITE,
		   DRM_BO_MASK_MEM | DRM_BO_FLAG_WRITE, 0);
	 OUT_BATCH((sbox.y1 << 16) | sbox.x1);
	 OUT_BATCH((srcpitch * cpp) & 0xffff);
	 OUT_RELOC(backRegion->buffer, DRM_BO_FLAG_MEM_TT | DRM_BO_FLAG_READ,
		   DRM_BO_MASK_MEM | DRM_BO_FLAG_READ, 0);

	 ADVANCE_BATCH();
      }

      if (intel->first_swap_fence)
	 driFenceUnReference(intel->first_swap_fence);
      intel->first_swap_fence = intel_batchbuffer_flush(intel->batch);
      driFenceReference(intel->first_swap_fence);
   }

   UNLOCK_HARDWARE(intel);

   /* XXX this is bogus. The context here may not even be bound to this drawable! */
   if (intel->lastStamp != dPriv->lastStamp) {
      GET_CURRENT_CONTEXT(currctx);
      struct intel_context *intelcurrent = intel_context(currctx);
      if (intelcurrent == intel && intelcurrent->driDrawable == dPriv) {
         intelWindowMoved(intel);
         intel->lastStamp = dPriv->lastStamp;
      }
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
      GLint pf_planes;

      /* Update page flipping info
       */
      pf_planes = 0;

      if (areaA > 0)
	 pf_planes |= 1;

      if (areaB > 0)
	 pf_planes |= 2;

      intel_fb->pf_current_page = (intel->sarea->pf_current_page >>
				   (intel_fb->pf_planes & 0x2)) & 0x3;

      intel_fb->pf_num_pages = 2 /*intel->intelScreen->third.handle ? 3 : 2*/;

      pf_active = pf_planes && (pf_planes & intel->sarea->pf_active) == pf_planes;

      if (pf_active != intel_fb->pf_active)
	 DBG(LOCK, "%s - Page flipping %sactive\n",
	     __progname, pf_active ? "" : "in");

      if (pf_active) {
	 /* Sync pages between planes if we're flipping on both at the same time */
	 if (pf_planes == 0x3 && pf_planes != intel_fb->pf_planes &&
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

	       flip.planes = 0x2;
	    } else {
               intel->sarea->pf_current_page =
		  intel->sarea->pf_current_page & (0x3 << 2);
	       intel->sarea->pf_current_page |=
		  (intel_fb->pf_current_page + intel_fb->pf_num_pages - 1) %
		  intel_fb->pf_num_pages;

	       flip.planes = 0x1;
	    }

	    drmCommandWrite(intel->driFd, DRM_I915_FLIP, &flip, sizeof(flip));
	 }

	 intel_fb->pf_planes = pf_planes;
      }

      intel_fb->pf_active = pf_active;
#if 0
      intel_flip_renderbuffers(intel_fb);
      intel_draw_buffer(&intel->ctx, intel->ctx.DrawBuffer);
#endif

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
	    if ((intel_fb->vbl_waited - intel_fb->vbl_pending[i]) <= (1<<23))
	       continue;

	    vbl.request.sequence = intel_fb->vbl_pending[i];
	    drmWaitVBlank(intel->driFd, &vbl);
	 }

	 intel_fb->vblank_flags = flags;
	 driGetCurrentVBlank(dPriv, intel_fb->vblank_flags, &intel_fb->vbl_seq);
	 intel_fb->vbl_waited = intel_fb->vbl_seq;

	 for (i = 0; i < intel_fb->pf_num_pages; i++) {
            intel_fb->vbl_pending[i] = intel_fb->vbl_waited;
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
#if 0
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
      GLint pf_planes = intel_fb->pf_planes;
      BATCH_LOCALS;

      /* Wait for pending flips to take effect */
      BEGIN_BATCH(2, batch_flags);
      OUT_BATCH(pf_planes & 0x1 ? (MI_WAIT_FOR_EVENT | MI_WAIT_FOR_PLANE_A_FLIP)
		: 0);
      OUT_BATCH(pf_planes & 0x2 ? (MI_WAIT_FOR_EVENT | MI_WAIT_FOR_PLANE_B_FLIP)
		: 0);
      ADVANCE_BATCH();

      intel_rb->pf_pending--;
   }
}
#endif

#if 0
/* Flip the front & back buffers
 */
static GLboolean
intelPageFlip(const __DRIdrawablePrivate * dPriv)
{
   struct intel_context *intel;
   int ret;
   struct intel_framebuffer *intel_fb = dPriv->driverPrivate;

   DBG(SWAP, "%s\n", __FUNCTION__);

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

      flip.planes = intel_fb->pf_planes;

      ret = drmCommandWrite(intel->driFd, DRM_I915_FLIP, &flip, sizeof(flip));
   }

   UNLOCK_HARDWARE(intel);

   if (ret || !intel_fb->pf_active)
      return GL_FALSE;

   if (!dPriv->numClipRects) {
      usleep(10000);	/* throttle invisible client 10ms */
   }

   intel_fb->pf_current_page = (intel->sarea->pf_current_page >>
				(intel_fb->pf_planes & 0x2)) & 0x3;

   if (dPriv->numClipRects != 0) {
      intel_get_renderbuffer(&intel_fb->Base, BUFFER_FRONT_LEFT)->pf_pending =
      intel_get_renderbuffer(&intel_fb->Base, BUFFER_BACK_LEFT)->pf_pending =
	 ++intel_fb->pf_seq;
   }

#if 0
   intel_flip_renderbuffers(intel_fb);
   intel_draw_buffer(&intel->ctx, &intel_fb->Base);
#endif

   DBG(SWAP, "%s: success\n", __FUNCTION__);

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

   /* XXX: Scheduled buffer swaps don't work with private back buffers yet */
   if (1 || !intel_fb->vblank_flags ||
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
				     (intel_fb->pf_planes & 0x2)) & 0x3) + 1) %
				  intel_fb->pf_num_pages;
   }

   if (!drmCommandWriteRead(intel->driFd, DRM_I915_VBLANK_SWAP, &swap,
			    sizeof(swap))) {
      intel_fb->vbl_seq = swap.sequence;
      swap.sequence -= target;
      *missed_target = swap.sequence > 0 && swap.sequence <= (1 << 23);

#if 1
      intel_fb->vbl_pending[1] = intel_fb->vbl_pending[0] = intel_fb->vbl_seq;
#else
      intel_get_renderbuffer(&intel_fb->Base, BUFFER_BACK_LEFT)->vbl_pending =
	 intel_get_renderbuffer(&intel_fb->Base,
				BUFFER_FRONT_LEFT)->vbl_pending =
	 intel_fb->vbl_seq;
#endif

      if (swap.seqtype & DRM_VBLANK_FLIP) {
#if 0
	 intel_flip_renderbuffers(intel_fb);
	 intel_draw_buffer(&intel->ctx, intel->ctx.DrawBuffer);
#endif
      }

      ret = GL_TRUE;
   } else {
      if (swap.seqtype & DRM_VBLANK_FLIP) {
	 intel_fb->pf_current_page = ((intel->sarea->pf_current_page >>
					(intel_fb->pf_planes & 0x2)) & 0x3) %
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

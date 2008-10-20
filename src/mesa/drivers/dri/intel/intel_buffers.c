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
#include "intel_chipset.h"
#include "intel_depthstencil.h"
#include "intel_fbo.h"
#include "intel_regions.h"
#include "intel_batchbuffer.h"
#include "intel_reg.h"
#include "main/context.h"
#include "main/framebuffer.h"
#include "swrast/swrast.h"
#include "utils.h"
#include "drirenderbuffer.h"
#include "vblank.h"
#include "i915_drm.h"

/* This block can be removed when libdrm >= 2.3.1 is required */

#ifndef DRM_IOCTL_I915_FLIP

#define DRM_VBLANK_FLIP 0x8000000

typedef struct drm_i915_flip {
   int pipes;
} drm_i915_flip_t;

#undef DRM_IOCTL_I915_FLIP
#define DRM_IOCTL_I915_FLIP DRM_IOW(DRM_COMMAND_BASE + DRM_I915_FLIP, \
				    drm_i915_flip_t)

#endif

#define FILE_DEBUG_FLAG DEBUG_BLIT

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
struct intel_region *
intel_drawbuf_region(struct intel_context *intel)
{
   struct intel_renderbuffer *irbColor =
      intel_renderbuffer(intel->ctx.DrawBuffer->_ColorDrawBuffers[0]);
   if (irbColor)
      return irbColor->region;
   else
      return NULL;
}

/**
 * Return pointer to current color reading region, or NULL.
 */
struct intel_region *
intel_readbuf_region(struct intel_context *intel)
{
   struct intel_renderbuffer *irb
      = intel_renderbuffer(intel->ctx.ReadBuffer->_ColorReadBuffer);
   if (irb)
      return irb->region;
   else
      return NULL;
}

void
intel_get_cliprects(struct intel_context *intel,
		    struct drm_clip_rect **cliprects,
		    unsigned int *num_cliprects,
		    int *x_off, int *y_off)
{
   __DRIdrawablePrivate *dPriv = intel->driDrawable;
   struct intel_framebuffer *intel_fb = dPriv->driverPrivate;

   if (intel->constant_cliprect) {
      /* FBO or DRI2 rendering, which can just use the fb's size. */
      intel->fboRect.x1 = 0;
      intel->fboRect.y1 = 0;
      intel->fboRect.x2 = intel->ctx.DrawBuffer->Width;
      intel->fboRect.y2 = intel->ctx.DrawBuffer->Height;

      *cliprects = &intel->fboRect;
      *num_cliprects = 1;
      *x_off = 0;
      *y_off = 0;
   } else if (intel->front_cliprects ||
       intel_fb->pf_active || dPriv->numBackClipRects == 0) {
      /* use the front clip rects */
      *cliprects = dPriv->pClipRects;
      *num_cliprects = dPriv->numClipRects;
      *x_off = dPriv->x;
      *y_off = dPriv->y;
   }
   else {
      /* use the back clip rects */
      *num_cliprects = dPriv->numBackClipRects;
      *cliprects = dPriv->pBackClipRects;
      *x_off = dPriv->backX;
      *y_off = dPriv->backY;
   }
}

static void
intelUpdatePageFlipping(struct intel_context *intel,
			GLint areaA, GLint areaB)
{
   __DRIdrawablePrivate *dPriv = intel->driDrawable;
   struct intel_framebuffer *intel_fb = dPriv->driverPrivate;
   GLboolean pf_active;
   GLint pf_planes;

   /* Update page flipping info */
   pf_planes = 0;

   if (areaA > 0)
      pf_planes |= 1;

   if (areaB > 0)
      pf_planes |= 2;

   intel_fb->pf_current_page = (intel->sarea->pf_current_page >>
				(intel_fb->pf_planes & 0x2)) & 0x3;

   intel_fb->pf_num_pages = intel->intelScreen->third.handle ? 3 : 2;

   pf_active = pf_planes && (pf_planes & intel->sarea->pf_active) == pf_planes;

   if (INTEL_DEBUG & DEBUG_LOCK)
      if (pf_active != intel_fb->pf_active)
	 _mesa_printf("%s - Page flipping %sactive\n", __progname,
		      pf_active ? "" : "in");

   if (pf_active) {
      /* Sync pages between planes if flipping on both at the same time */
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

      intel_fb->pf_planes = pf_planes;
   }

   intel_fb->pf_active = pf_active;
   intel_flip_renderbuffers(intel_fb);
   intel_draw_buffer(&intel->ctx, intel->ctx.DrawBuffer);
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
      volatile struct drm_i915_sarea *sarea = intel->sarea;
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

      intelUpdatePageFlipping(intel, areaA, areaB);

      /* Update vblank info
       */
      if (areaB > areaA || (areaA == areaB && areaB > 0)) {
	 flags = dPriv->vblFlags | VBLANK_FLAG_SECONDARY;
      } else {
	 flags = dPriv->vblFlags & ~VBLANK_FLAG_SECONDARY;
      }

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

	 for (i = 0; i < intel_fb->pf_num_pages; i++) {
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

	 for (i = 0; i < intel_fb->pf_num_pages; i++) {
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



/* A true meta version of this would be very simple and additionally
 * machine independent.  Maybe we'll get there one day.
 */
static void
intelClearWithTris(struct intel_context *intel, GLbitfield mask)
{
   GLcontext *ctx = &intel->ctx;
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   GLuint buf;

   intel->vtbl.install_meta_state(intel);

   /* Back and stencil cliprects are the same.  Try and do both
    * buffers at once:
    */
   if (mask & (BUFFER_BIT_BACK_LEFT | BUFFER_BIT_STENCIL | BUFFER_BIT_DEPTH)) {
      struct intel_region *backRegion =
	 intel_get_rb_region(fb, BUFFER_BACK_LEFT);
      struct intel_region *depthRegion =
	 intel_get_rb_region(fb, BUFFER_DEPTH);

      intel->vtbl.meta_draw_region(intel, backRegion, depthRegion);

      if (mask & BUFFER_BIT_BACK_LEFT)
	 intel->vtbl.meta_color_mask(intel, GL_TRUE);
      else
	 intel->vtbl.meta_color_mask(intel, GL_FALSE);

      if (mask & BUFFER_BIT_STENCIL)
	 intel->vtbl.meta_stencil_replace(intel,
					  intel->ctx.Stencil.WriteMask[0],
					  intel->ctx.Stencil.Clear);
      else
	 intel->vtbl.meta_no_stencil_write(intel);

      if (mask & BUFFER_BIT_DEPTH)
	 intel->vtbl.meta_depth_replace(intel);
      else
	 intel->vtbl.meta_no_depth_write(intel);

      intel->vtbl.meta_draw_quad(intel,
				 fb->_Xmin,
				 fb->_Xmax,
				 fb->_Ymin,
				 fb->_Ymax,
				 intel->ctx.Depth.Clear,
				 intel->ClearColor8888,
				 0, 0, 0, 0);   /* texcoords */

      mask &= ~(BUFFER_BIT_BACK_LEFT | BUFFER_BIT_STENCIL | BUFFER_BIT_DEPTH);
   }

   /* clear the remaining (color) renderbuffers */
   for (buf = 0; buf < BUFFER_COUNT && mask; buf++) {
      const GLuint bufBit = 1 << buf;
      if (mask & bufBit) {
	 struct intel_renderbuffer *irbColor =
	    intel_renderbuffer(fb->Attachment[buf].Renderbuffer);

	 ASSERT(irbColor);

	 intel->vtbl.meta_no_depth_write(intel);
	 intel->vtbl.meta_no_stencil_write(intel);
	 intel->vtbl.meta_color_mask(intel, GL_TRUE);
	 intel->vtbl.meta_draw_region(intel, irbColor->region, NULL);

	 intel->vtbl.meta_draw_quad(intel,
				    fb->_Xmin,
				    fb->_Xmax,
				    fb->_Ymin,
				    fb->_Ymax,
				    0, intel->ClearColor8888,
				    0, 0, 0, 0);   /* texcoords */

	 mask &= ~bufBit;
      }
   }

   intel->vtbl.leave_meta_state(intel);
}

static const char *buffer_names[] = {
   [BUFFER_FRONT_LEFT] = "front",
   [BUFFER_BACK_LEFT] = "back",
   [BUFFER_FRONT_RIGHT] = "front right",
   [BUFFER_BACK_RIGHT] = "back right",
   [BUFFER_AUX0] = "aux0",
   [BUFFER_AUX1] = "aux1",
   [BUFFER_AUX2] = "aux2",
   [BUFFER_AUX3] = "aux3",
   [BUFFER_DEPTH] = "depth",
   [BUFFER_STENCIL] = "stencil",
   [BUFFER_ACCUM] = "accum",
   [BUFFER_COLOR0] = "color0",
   [BUFFER_COLOR1] = "color1",
   [BUFFER_COLOR2] = "color2",
   [BUFFER_COLOR3] = "color3",
   [BUFFER_COLOR4] = "color4",
   [BUFFER_COLOR5] = "color5",
   [BUFFER_COLOR6] = "color6",
   [BUFFER_COLOR7] = "color7",
};

/**
 * Called by ctx->Driver.Clear.
 */
static void
intelClear(GLcontext *ctx, GLbitfield mask)
{
   struct intel_context *intel = intel_context(ctx);
   const GLuint colorMask = *((GLuint *) & ctx->Color.ColorMask);
   GLbitfield tri_mask = 0;
   GLbitfield blit_mask = 0;
   GLbitfield swrast_mask = 0;
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   GLuint i;

   if (0)
      fprintf(stderr, "%s\n", __FUNCTION__);

   /* HW color buffers (front, back, aux, generic FBO, etc) */
   if (colorMask == ~0) {
      /* clear all R,G,B,A */
      /* XXX FBO: need to check if colorbuffers are software RBOs! */
      blit_mask |= (mask & BUFFER_BITS_COLOR);
   }
   else {
      /* glColorMask in effect */
      tri_mask |= (mask & BUFFER_BITS_COLOR);
   }

   /* HW stencil */
   if (mask & BUFFER_BIT_STENCIL) {
      const struct intel_region *stencilRegion
         = intel_get_rb_region(fb, BUFFER_STENCIL);
      if (stencilRegion) {
         /* have hw stencil */
         if (IS_965(intel->intelScreen->deviceID) ||
	     (ctx->Stencil.WriteMask[0] & 0xff) != 0xff) {
	    /* We have to use the 3D engine if we're clearing a partial mask
	     * of the stencil buffer, or if we're on a 965 which has a tiled
	     * depth/stencil buffer in a layout we can't blit to.
	     */
            tri_mask |= BUFFER_BIT_STENCIL;
         }
         else {
            /* clearing all stencil bits, use blitting */
            blit_mask |= BUFFER_BIT_STENCIL;
         }
      }
   }

   /* HW depth */
   if (mask & BUFFER_BIT_DEPTH) {
      /* clear depth with whatever method is used for stencil (see above) */
      if (IS_965(intel->intelScreen->deviceID) ||
	  tri_mask & BUFFER_BIT_STENCIL)
         tri_mask |= BUFFER_BIT_DEPTH;
      else
         blit_mask |= BUFFER_BIT_DEPTH;
   }

   /* SW fallback clearing */
   swrast_mask = mask & ~tri_mask & ~blit_mask;

   for (i = 0; i < BUFFER_COUNT; i++) {
      GLuint bufBit = 1 << i;
      if ((blit_mask | tri_mask) & bufBit) {
         if (!fb->Attachment[i].Renderbuffer->ClassID) {
            blit_mask &= ~bufBit;
            tri_mask &= ~bufBit;
            swrast_mask |= bufBit;
         }
      }
   }

   if (blit_mask) {
      if (INTEL_DEBUG & DEBUG_BLIT) {
	 DBG("blit clear:");
	 for (i = 0; i < BUFFER_COUNT; i++) {
	    if (blit_mask & (1 << i))
	       DBG(" %s", buffer_names[i]);
	 }
	 DBG("\n");
      }
      intelClearWithBlit(ctx, blit_mask);
   }

   if (tri_mask) {
      if (INTEL_DEBUG & DEBUG_BLIT) {
	 DBG("tri clear:");
	 for (i = 0; i < BUFFER_COUNT; i++) {
	    if (tri_mask & (1 << i))
	       DBG(" %s", buffer_names[i]);
	 }
	 DBG("\n");
      }
      intelClearWithTris(intel, tri_mask);
   }

   if (swrast_mask) {
      if (INTEL_DEBUG & DEBUG_BLIT) {
	 DBG("swrast clear:");
	 for (i = 0; i < BUFFER_COUNT; i++) {
	    if (swrast_mask & (1 << i))
	       DBG(" %s", buffer_names[i]);
	 }
	 DBG("\n");
      }
      _swrast_Clear(ctx, swrast_mask);
   }
}


/* Emit wait for pending flips */
void
intel_wait_flips(struct intel_context *intel)
{
   struct intel_framebuffer *intel_fb =
      (struct intel_framebuffer *) intel->ctx.DrawBuffer;
   struct intel_renderbuffer *intel_rb =
      intel_get_renderbuffer(&intel_fb->Base,
			     intel_fb->Base._ColorDrawBufferIndexes[0] ==
			     BUFFER_FRONT_LEFT ? BUFFER_FRONT_LEFT :
			     BUFFER_BACK_LEFT);

   if (intel->intelScreen->driScrnPriv->dri2.enabled)
      return;

   if (intel_fb->Base.Name == 0 && intel_rb &&
       intel_rb->pf_pending == intel_fb->pf_seq) {
      GLint pf_planes = intel_fb->pf_planes;
      BATCH_LOCALS;

      /* Wait for pending flips to take effect */
      BEGIN_BATCH(2, NO_LOOP_CLIPRECTS);
      OUT_BATCH(pf_planes & 0x1 ? (MI_WAIT_FOR_EVENT | MI_WAIT_FOR_PLANE_A_FLIP)
		: 0);
      OUT_BATCH(pf_planes & 0x2 ? (MI_WAIT_FOR_EVENT | MI_WAIT_FOR_PLANE_B_FLIP)
		: 0);
      ADVANCE_BATCH();

      intel_rb->pf_pending--;
   }
}


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

      flip.pipes = intel_fb->pf_planes;

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

   intel_flip_renderbuffers(intel_fb);
   intel_draw_buffer(&intel->ctx, &intel_fb->Base);

   return GL_TRUE;
}

static GLboolean
intelScheduleSwap(__DRIdrawablePrivate * dPriv, GLboolean *missed_target)
{
   struct intel_framebuffer *intel_fb = dPriv->driverPrivate;
   unsigned int interval;
   struct intel_context *intel =
      intelScreenContext(dPriv->driScreenPriv->private);
   const intelScreenPrivate *intelScreen = intel->intelScreen;
   unsigned int target;
   drm_i915_vblank_swap_t swap;
   GLboolean ret;

   if (!dPriv->vblFlags ||
       (dPriv->vblFlags & VBLANK_FLAG_NO_IRQ) ||
       intelScreen->drmMinor < (intel_fb->pf_active ? 9 : 6))
      return GL_FALSE;

   interval = driGetVBlankInterval(dPriv);

   swap.seqtype = DRM_VBLANK_ABSOLUTE;

   if (dPriv->vblFlags & VBLANK_FLAG_SYNC) {
      swap.seqtype |= DRM_VBLANK_NEXTONMISS;
   } else if (interval == 0)
      return GL_FALSE;

   swap.drawable = dPriv->hHWDrawable;
   target = swap.sequence = dPriv->vblSeq + interval;

   if ( dPriv->vblFlags & VBLANK_FLAG_SECONDARY ) {
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
      dPriv->vblSeq = swap.sequence;
      swap.sequence -= target;
      *missed_target = swap.sequence > 0 && swap.sequence <= (1 << 23);

      intel_get_renderbuffer(&intel_fb->Base, BUFFER_BACK_LEFT)->vbl_pending =
	 intel_get_renderbuffer(&intel_fb->Base,
				BUFFER_FRONT_LEFT)->vbl_pending =
	 dPriv->vblSeq;

      if (swap.seqtype & DRM_VBLANK_FLIP) {
	 intel_flip_renderbuffers(intel_fb);
	 intel_draw_buffer(&intel->ctx, intel->ctx.DrawBuffer);
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

         if (!intelScheduleSwap(dPriv, &missed_target)) {
	    driWaitForVBlank(dPriv, &missed_target);

	    /*
	     * Update each buffer's vbl_pending so we don't get too out of
	     * sync
	     */
	    intel_get_renderbuffer(&intel_fb->Base,
				   BUFFER_BACK_LEFT)->vbl_pending = 
		    intel_get_renderbuffer(&intel_fb->Base,
					   BUFFER_FRONT_LEFT)->vbl_pending =
		    dPriv->vblSeq;
	    if (!intelPageFlip(dPriv)) {
	       intelCopyBuffer(dPriv, NULL);
	    }
	 }

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
   struct intel_region *colorRegions[MAX_DRAW_BUFFERS], *depthRegion = NULL;
   struct intel_renderbuffer *irbDepth = NULL, *irbStencil = NULL;

   if (!fb) {
      /* this can happen during the initial context initialization */
      return;
   }

   /* Do this here, note core Mesa, since this function is called from
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
   if (fb->_NumColorDrawBuffers == 0) {
      /* writing to 0  */
      colorRegions[0] = NULL;
      intel->constant_cliprect = GL_TRUE;
   } else if (fb->_NumColorDrawBuffers > 1) {
       int i;
       struct intel_renderbuffer *irb;

       for (i = 0; i < fb->_NumColorDrawBuffers; i++) {
           irb = intel_renderbuffer(fb->_ColorDrawBuffers[i]);
           colorRegions[i] = irb ? irb->region : NULL;
       }
       intel->constant_cliprect = GL_TRUE;
   }
   else {
      /* Get the intel_renderbuffer for the single colorbuffer we're drawing
       * into, and set up cliprects if it's .
       */
      if (fb->Name == 0) {
	 intel->constant_cliprect = intel->driScreen->dri2.enabled;
	 /* drawing to window system buffer */
	 if (fb->_ColorDrawBufferIndexes[0] == BUFFER_FRONT_LEFT) {
	    if (!intel->constant_cliprect && !intel->front_cliprects)
	       intel_batchbuffer_flush(intel->batch);
	    intel->front_cliprects = GL_TRUE;
	    colorRegions[0] = intel_get_rb_region(fb, BUFFER_FRONT_LEFT);
	 }
	 else {
	    if (!intel->constant_cliprect && intel->front_cliprects)
	       intel_batchbuffer_flush(intel->batch);
	    intel->front_cliprects = GL_FALSE;
	    colorRegions[0]= intel_get_rb_region(fb, BUFFER_BACK_LEFT);
	 }
      }
      else {
	 /* drawing to user-created FBO */
	 struct intel_renderbuffer *irb;
	 irb = intel_renderbuffer(fb->_ColorDrawBuffers[0]);
	 colorRegions[0] = (irb && irb->region) ? irb->region : NULL;
	 intel->constant_cliprect = GL_TRUE;
      }
   }

   /* Update culling direction which changes depending on the
    * orientation of the buffer:
    */
   if (ctx->Driver.FrontFace)
      ctx->Driver.FrontFace(ctx, ctx->Polygon.FrontFace);
   else
      ctx->NewState |= _NEW_POLYGON;

   if (!colorRegions[0]) {
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
	 if (ctx->Driver.Enable != NULL)
	    ctx->Driver.Enable(ctx, GL_STENCIL_TEST, ctx->Stencil.Enabled);
	 else
	    ctx->NewState |= _NEW_STENCIL;
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
      if (ctx->Driver.Enable != NULL)
	 ctx->Driver.Enable(ctx, GL_STENCIL_TEST, ctx->Stencil.Enabled);
      else
	 ctx->NewState |= _NEW_STENCIL;
   }

   /*
    * Update depth test state
    */
   if (ctx->Driver.Enable) {
      if (ctx->Depth.Test && fb->Visual.depthBits > 0) {
	 ctx->Driver.Enable(ctx, GL_DEPTH_TEST, GL_TRUE);
      } else {
	 ctx->Driver.Enable(ctx, GL_DEPTH_TEST, GL_FALSE);
      }
   } else {
      ctx->NewState |= _NEW_DEPTH;
   }

   intel->vtbl.set_draw_region(intel, colorRegions, depthRegion, 
	fb->_NumColorDrawBuffers);

   /* update viewport since it depends on window size */
   if (ctx->Driver.Viewport) {
      ctx->Driver.Viewport(ctx, ctx->Viewport.X, ctx->Viewport.Y,
			   ctx->Viewport.Width, ctx->Viewport.Height);
   } else {
      ctx->NewState |= _NEW_VIEWPORT;
   }

   /* Set state we know depends on drawable parameters:
    */
   if (ctx->Driver.Scissor)
      ctx->Driver.Scissor(ctx, ctx->Scissor.X, ctx->Scissor.Y,
			  ctx->Scissor.Width, ctx->Scissor.Height);
   intel->NewGLState |= _NEW_SCISSOR;

   if (ctx->Driver.DepthRange)
      ctx->Driver.DepthRange(ctx,
			     ctx->Viewport.Near,
			     ctx->Viewport.Far);
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
   functions->Clear = intelClear;
   functions->DrawBuffer = intelDrawBuffer;
   functions->ReadBuffer = intelReadBuffer;
}

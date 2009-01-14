/**************************************************************************

Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#include <errno.h>
#include "main/glheader.h"
#include "main/imports.h"
#include "main/api_arrayelt.h"
#include "main/enums.h"
#include "main/colormac.h"
#include "main/light.h"
#include "main/framebuffer.h"

#include "swrast/swrast.h"
#include "vbo/vbo.h"
#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"
#include "swrast_setup/swrast_setup.h"

#include "drirenderbuffer.h"
#include "vblank.h"

#include "radeon_bo.h"
#include "radeon_cs.h"
#include "radeon_bo_legacy.h"
#include "radeon_cs_legacy.h"
#include "radeon_bo_gem.h"
#include "radeon_cs_gem.h"
#include "dri_util.h"
#include "radeon_drm.h"
#include "radeon_buffer.h"
#include "radeon_screen.h"
#include "common_context.h"
#include "common_misc.h"
#include "common_lock.h"

#ifndef RADEON_DEBUG
int RADEON_DEBUG = (0);
#endif

/* =============================================================
 * Scissoring
 */

static GLboolean intersect_rect(drm_clip_rect_t * out,
				drm_clip_rect_t * a, drm_clip_rect_t * b)
{
	*out = *a;
	if (b->x1 > out->x1)
		out->x1 = b->x1;
	if (b->y1 > out->y1)
		out->y1 = b->y1;
	if (b->x2 < out->x2)
		out->x2 = b->x2;
	if (b->y2 < out->y2)
		out->y2 = b->y2;
	if (out->x1 >= out->x2)
		return GL_FALSE;
	if (out->y1 >= out->y2)
		return GL_FALSE;
	return GL_TRUE;
}

void radeonRecalcScissorRects(radeonContextPtr radeon)
{
	drm_clip_rect_t *out;
	int i;

	/* Grow cliprect store?
	 */
	if (radeon->state.scissor.numAllocedClipRects < radeon->numClipRects) {
		while (radeon->state.scissor.numAllocedClipRects <
		       radeon->numClipRects) {
			radeon->state.scissor.numAllocedClipRects += 1;	/* zero case */
			radeon->state.scissor.numAllocedClipRects *= 2;
		}

		if (radeon->state.scissor.pClipRects)
			FREE(radeon->state.scissor.pClipRects);

		radeon->state.scissor.pClipRects =
		    MALLOC(radeon->state.scissor.numAllocedClipRects *
			   sizeof(drm_clip_rect_t));

		if (radeon->state.scissor.pClipRects == NULL) {
			radeon->state.scissor.numAllocedClipRects = 0;
			return;
		}
	}

	out = radeon->state.scissor.pClipRects;
	radeon->state.scissor.numClipRects = 0;

	for (i = 0; i < radeon->numClipRects; i++) {
		if (intersect_rect(out,
				   &radeon->pClipRects[i],
				   &radeon->state.scissor.rect)) {
			radeon->state.scissor.numClipRects++;
			out++;
		}
	}
}

/**
 * Update cliprects and scissors.
 */
void radeonSetCliprects(radeonContextPtr radeon)
{
	__DRIdrawablePrivate *const drawable = radeon->dri.drawable;
	__DRIdrawablePrivate *const readable = radeon->dri.readable;
	GLframebuffer *const draw_fb = (GLframebuffer*)drawable->driverPrivate;
	GLframebuffer *const read_fb = (GLframebuffer*)readable->driverPrivate;

	if (!radeon->radeonScreen->driScreen->dri2.enabled) {
		if (draw_fb->_ColorDrawBufferIndexes[0] == BUFFER_BACK_LEFT) {
			/* Can't ignore 2d windows if we are page flipping. */
			if (drawable->numBackClipRects == 0 || radeon->doPageFlip ||
			    radeon->sarea->pfCurrentPage == 1) {
				radeon->numClipRects = drawable->numClipRects;
				radeon->pClipRects = drawable->pClipRects;
			} else {
				radeon->numClipRects = drawable->numBackClipRects;
				radeon->pClipRects = drawable->pBackClipRects;
			}
		} else {
			/* front buffer (or none, or multiple buffers */
			radeon->numClipRects = drawable->numClipRects;
			radeon->pClipRects = drawable->pClipRects;
		}
	}
	
	if ((draw_fb->Width != drawable->w) ||
	    (draw_fb->Height != drawable->h)) {
		_mesa_resize_framebuffer(radeon->glCtx, draw_fb,
					 drawable->w, drawable->h);
		draw_fb->Initialized = GL_TRUE;
	}

	if (drawable != readable) {
		if ((read_fb->Width != readable->w) ||
		    (read_fb->Height != readable->h)) {
			_mesa_resize_framebuffer(radeon->glCtx, read_fb,
						 readable->w, readable->h);
			read_fb->Initialized = GL_TRUE;
		}
	}

	if (radeon->state.scissor.enabled)
		radeonRecalcScissorRects(radeon);

	radeon->lastStamp = drawable->lastStamp;
}

void radeonUpdateScissor( GLcontext *ctx )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);

   if ( rmesa->dri.drawable ) {
      __DRIdrawablePrivate *dPriv = rmesa->dri.drawable;

      int x = ctx->Scissor.X;
      int y = dPriv->h - ctx->Scissor.Y - ctx->Scissor.Height;
      int w = ctx->Scissor.X + ctx->Scissor.Width - 1;
      int h = dPriv->h - ctx->Scissor.Y - 1;

      rmesa->state.scissor.rect.x1 = x + dPriv->x;
      rmesa->state.scissor.rect.y1 = y + dPriv->y;
      rmesa->state.scissor.rect.x2 = w + dPriv->x + 1;
      rmesa->state.scissor.rect.y2 = h + dPriv->y + 1;

      radeonRecalcScissorRects( rmesa );
   }
}

/* ================================================================
 * SwapBuffers with client-side throttling
 */

static uint32_t radeonGetLastFrame(radeonContextPtr radeon)
{
	drm_radeon_getparam_t gp;
	int ret;
	uint32_t frame;

	gp.param = RADEON_PARAM_LAST_FRAME;
	gp.value = (int *)&frame;
	ret = drmCommandWriteRead(radeon->dri.fd, DRM_RADEON_GETPARAM,
				  &gp, sizeof(gp));
	if (ret) {
		fprintf(stderr, "%s: drmRadeonGetParam: %d\n", __FUNCTION__,
			ret);
		exit(1);
	}

	return frame;
}

uint32_t radeonGetAge(radeonContextPtr radeon)
{
	drm_radeon_getparam_t gp;
	int ret;
	uint32_t age;

	gp.param = RADEON_PARAM_LAST_CLEAR;
	gp.value = (int *)&age;
	ret = drmCommandWriteRead(radeon->dri.fd, DRM_RADEON_GETPARAM,
				  &gp, sizeof(gp));
	if (ret) {
		fprintf(stderr, "%s: drmRadeonGetParam: %d\n", __FUNCTION__,
			ret);
		exit(1);
	}

	return age;
}

static void radeonEmitIrqLocked(radeonContextPtr radeon)
{
	drm_radeon_irq_emit_t ie;
	int ret;

	ie.irq_seq = &radeon->iw.irq_seq;
	ret = drmCommandWriteRead(radeon->dri.fd, DRM_RADEON_IRQ_EMIT,
				  &ie, sizeof(ie));
	if (ret) {
		fprintf(stderr, "%s: drmRadeonIrqEmit: %d\n", __FUNCTION__,
			ret);
		exit(1);
	}
}

static void radeonWaitIrq(radeonContextPtr radeon)
{
	int ret;

	do {
		ret = drmCommandWrite(radeon->dri.fd, DRM_RADEON_IRQ_WAIT,
				      &radeon->iw, sizeof(radeon->iw));
	} while (ret && (errno == EINTR || errno == EBUSY));

	if (ret) {
		fprintf(stderr, "%s: drmRadeonIrqWait: %d\n", __FUNCTION__,
			ret);
		exit(1);
	}
}

static void radeonWaitForFrameCompletion(radeonContextPtr radeon)
{
	drm_radeon_sarea_t *sarea = radeon->sarea;

	if (radeon->do_irqs) {
		if (radeonGetLastFrame(radeon) < sarea->last_frame) {
			if (!radeon->irqsEmitted) {
				while (radeonGetLastFrame(radeon) <
				       sarea->last_frame) ;
			} else {
				UNLOCK_HARDWARE(radeon);
				radeonWaitIrq(radeon);
				LOCK_HARDWARE(radeon);
			}
			radeon->irqsEmitted = 10;
		}

		if (radeon->irqsEmitted) {
			radeonEmitIrqLocked(radeon);
			radeon->irqsEmitted--;
		}
	} else {
		while (radeonGetLastFrame(radeon) < sarea->last_frame) {
			UNLOCK_HARDWARE(radeon);
			if (radeon->do_usleeps)
				DO_USLEEP(1);
			LOCK_HARDWARE(radeon);
		}
	}
}

/* wait for idle */
void radeonWaitForIdleLocked(radeonContextPtr radeon)
{
	int ret;
	int i = 0;

	do {
		ret = drmCommandNone(radeon->dri.fd, DRM_RADEON_CP_IDLE);
		if (ret)
			DO_USLEEP(1);
	} while (ret && ++i < 100);

	if (ret < 0) {
		UNLOCK_HARDWARE(radeon);
		fprintf(stderr, "Error: R300 timed out... exiting\n");
		exit(-1);
	}
}

static void radeonWaitForIdle(radeonContextPtr radeon)
{
	LOCK_HARDWARE(radeon);
	radeonWaitForIdleLocked(radeon);
	UNLOCK_HARDWARE(radeon);
}


/* Copy the back color buffer to the front color buffer.
 */
void radeonCopyBuffer( __DRIdrawablePrivate *dPriv,
		       const drm_clip_rect_t	  *rect)
{
   radeonContextPtr rmesa;
   GLint nbox, i, ret;
   GLboolean   missed_target;
   int64_t ust;
   __DRIscreenPrivate *psp;

   assert(dPriv);
   assert(dPriv->driContextPriv);
   assert(dPriv->driContextPriv->driverPrivate);
   
   rmesa = (radeonContextPtr) dPriv->driContextPriv->driverPrivate;

   if ( RADEON_DEBUG & DEBUG_IOCTL ) {
      fprintf( stderr, "\n%s( %p )\n\n", __FUNCTION__, (void *) rmesa->glCtx );
   }

   rmesa->vtbl.flush(rmesa->glCtx);
   LOCK_HARDWARE( rmesa );

   /* Throttle the frame rate -- only allow one pending swap buffers
    * request at a time.
    */
   radeonWaitForFrameCompletion( rmesa );
   if (!rect)
   {
       UNLOCK_HARDWARE( rmesa );
       driWaitForVBlank( dPriv, & missed_target );
       LOCK_HARDWARE( rmesa );
   }

   nbox = dPriv->numClipRects; /* must be in locked region */

   for ( i = 0 ; i < nbox ; ) {
      GLint nr = MIN2( i + RADEON_NR_SAREA_CLIPRECTS , nbox );
      drm_clip_rect_t *box = dPriv->pClipRects;
      drm_clip_rect_t *b = rmesa->sarea->boxes;
      GLint n = 0;

      for ( ; i < nr ; i++ ) {

	  *b = box[i];

	  if (rect)
	  {
	      if (rect->x1 > b->x1)
		  b->x1 = rect->x1;
	      if (rect->y1 > b->y1)
		  b->y1 = rect->y1;
	      if (rect->x2 < b->x2)
		  b->x2 = rect->x2;
	      if (rect->y2 < b->y2)
		  b->y2 = rect->y2;

	      if (b->x1 >= b->x2 || b->y1 >= b->y2)
		  continue;
	  }

	  b++;
	  n++;
      }
      rmesa->sarea->nbox = n;

      if (!n)
	 continue;

      ret = drmCommandNone( rmesa->dri.fd, DRM_RADEON_SWAP );

      if ( ret ) {
	 fprintf( stderr, "DRM_RADEON_SWAP_BUFFERS: return = %d\n", ret );
	 UNLOCK_HARDWARE( rmesa );
	 exit( 1 );
      }
   }

   UNLOCK_HARDWARE( rmesa );
   if (!rect)
   {
       psp = dPriv->driScreenPriv;
       rmesa->swap_count++;
       (*psp->systemTime->getUST)( & ust );
       if ( missed_target ) {
	   rmesa->swap_missed_count++;
	   rmesa->swap_missed_ust = ust - rmesa->swap_ust;
       }

       rmesa->swap_ust = ust;
       rmesa->vtbl.set_all_dirty(rmesa->glCtx);

   }
}

void radeonPageFlip( __DRIdrawablePrivate *dPriv )
{
   radeonContextPtr rmesa;
   GLint ret;
   GLboolean   missed_target;
   __DRIscreenPrivate *psp;
   struct radeon_renderbuffer *rrb;
   GLframebuffer *fb = dPriv->driverPrivate;

   assert(dPriv);
   assert(dPriv->driContextPriv);
   assert(dPriv->driContextPriv->driverPrivate);

   rmesa = (radeonContextPtr) dPriv->driContextPriv->driverPrivate;
   rrb = (void *)fb->Attachment[BUFFER_FRONT_LEFT].Renderbuffer;

   psp = dPriv->driScreenPriv;

   if ( RADEON_DEBUG & DEBUG_IOCTL ) {
      fprintf(stderr, "%s: pfCurrentPage: %d\n", __FUNCTION__,
	      rmesa->sarea->pfCurrentPage);
   }

   rmesa->vtbl.flush(rmesa->glCtx);

   LOCK_HARDWARE( rmesa );

   if (!dPriv->numClipRects) {
	   UNLOCK_HARDWARE(rmesa);
	   usleep(10000);	/* throttle invisible client 10ms */
	   return;
   }

   drm_clip_rect_t *box = dPriv->pClipRects;
   drm_clip_rect_t *b = rmesa->sarea->boxes;
   b[0] = box[0];
   rmesa->sarea->nbox = 1;

   /* Throttle the frame rate -- only allow a few pending swap buffers
    * request at a time.
    */
   radeonWaitForFrameCompletion( rmesa );
   UNLOCK_HARDWARE( rmesa );
   driWaitForVBlank( dPriv, & missed_target );
   if ( missed_target ) {
      rmesa->swap_missed_count++;
      (void) (*psp->systemTime->getUST)( & rmesa->swap_missed_ust );
   }
   LOCK_HARDWARE( rmesa );

   ret = drmCommandNone( rmesa->dri.fd, DRM_RADEON_FLIP );

   UNLOCK_HARDWARE( rmesa );

   if ( ret ) {
      fprintf( stderr, "DRM_RADEON_FLIP: return = %d\n", ret );
      exit( 1 );
   }

   rmesa->swap_count++;
   (void) (*psp->systemTime->getUST)( & rmesa->swap_ust );

   /* Get ready for drawing next frame.  Update the renderbuffers'
    * flippedOffset/Pitch fields so we draw into the right place.
    */
   driFlipRenderbuffers(rmesa->glCtx->WinSysDrawBuffer,
                        rmesa->sarea->pfCurrentPage);

   rmesa->state.color.rrb = rrb;

   if (rmesa->vtbl.update_draw_buffer)
	   rmesa->vtbl.update_draw_buffer(rmesa->glCtx);
}


/* Make sure all commands have been sent to the hardware and have
 * completed processing.
 */
void radeon_common_finish(GLcontext * ctx)
{
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);
	struct gl_framebuffer *fb = ctx->DrawBuffer;
	int i;

	if (radeon->radeonScreen->kernel_mm) {
		for (i = 0; i < fb->_NumColorDrawBuffers; i++) {
			struct radeon_renderbuffer *rrb;
			rrb = (struct radeon_renderbuffer *)fb->_ColorDrawBuffers[i];
			if (rrb->bo)
			    radeon_bo_wait(rrb->bo);
		}
	} else if (radeon->do_irqs) {
		LOCK_HARDWARE(radeon);
		radeonEmitIrqLocked(radeon);
		UNLOCK_HARDWARE(radeon);
		radeonWaitIrq(radeon);
	} else {
		radeonWaitForIdle(radeon);
	}
}

/**
 * Swap front and back buffer.
 */
void radeonSwapBuffers(__DRIdrawablePrivate * dPriv)
{
	if (dPriv->driContextPriv && dPriv->driContextPriv->driverPrivate) {
		radeonContextPtr radeon;
		GLcontext *ctx;

		radeon = (radeonContextPtr) dPriv->driContextPriv->driverPrivate;
		ctx = radeon->glCtx;

		if (ctx->Visual.doubleBufferMode) {
			_mesa_notifySwapBuffers(ctx);/* flush pending rendering comands */
			if (radeon->doPageFlip) {
				radeonPageFlip(dPriv);
			} else {
				radeonCopyBuffer(dPriv, NULL);
			}
		}
	} else {
		/* XXX this shouldn't be an error but we can't handle it for now */
		_mesa_problem(NULL, "%s: drawable has no context!",
			      __FUNCTION__);
	}
}

void radeonCopySubBuffer(__DRIdrawablePrivate * dPriv,
			 int x, int y, int w, int h )
{
    if (dPriv->driContextPriv && dPriv->driContextPriv->driverPrivate) {
	radeonContextPtr radeon;
	GLcontext *ctx;

	radeon = (radeonContextPtr) dPriv->driContextPriv->driverPrivate;
	ctx = radeon->glCtx;

	if (ctx->Visual.doubleBufferMode) {
	    drm_clip_rect_t rect;
	    rect.x1 = x + dPriv->x;
	    rect.y1 = (dPriv->h - y - h) + dPriv->y;
	    rect.x2 = rect.x1 + w;
	    rect.y2 = rect.y1 + h;
	    _mesa_notifySwapBuffers(ctx);	/* flush pending rendering comands */
	    radeonCopyBuffer(dPriv, &rect);
	}
    } else {
	/* XXX this shouldn't be an error but we can't handle it for now */
	_mesa_problem(NULL, "%s: drawable has no context!",
		      __FUNCTION__);
    }
}

/* cmdbuffer */
/**
 * Send the current command buffer via ioctl to the hardware.
 */
int rcommonFlushCmdBufLocked(radeonContextPtr rmesa, const char *caller)
{
	int ret = 0;

	if (rmesa->cmdbuf.flushing) {
		fprintf(stderr, "Recursive call into r300FlushCmdBufLocked!\n");
		exit(-1);
	}
	rmesa->cmdbuf.flushing = 1;
	if (rmesa->cmdbuf.cs->cdw) {
		ret = radeon_cs_emit(rmesa->cmdbuf.cs);
		rmesa->vtbl.set_all_dirty(rmesa->glCtx);
	}
	radeon_cs_erase(rmesa->cmdbuf.cs);
	rmesa->cmdbuf.flushing = 0;
	return ret;
}

int rcommonFlushCmdBuf(radeonContextPtr rmesa, const char *caller)
{
	int ret;

	LOCK_HARDWARE(rmesa);
	ret = rcommonFlushCmdBufLocked(rmesa, caller);
	UNLOCK_HARDWARE(rmesa);

	if (ret) {
		fprintf(stderr, "drmRadeonCmdBuffer: %d\n", ret);
		_mesa_exit(ret);
	}

	return ret;
}

/**
 * Make sure that enough space is available in the command buffer
 * by flushing if necessary.
 *
 * \param dwords The number of dwords we need to be free on the command buffer
 */
void rcommonEnsureCmdBufSpace(radeonContextPtr rmesa, int dwords, const char *caller)
{
	if ((rmesa->cmdbuf.cs->cdw + dwords + 128) > rmesa->cmdbuf.size ||
	    radeon_cs_need_flush(rmesa->cmdbuf.cs)) {
		rcommonFlushCmdBuf(rmesa, caller);
    }
}

void rcommonInitCmdBuf(radeonContextPtr rmesa, int max_state_size)
{
	GLuint size;
	/* Initialize command buffer */
	size = 256 * driQueryOptioni(&rmesa->optionCache,
				     "command_buffer_size");
	if (size < 2 * max_state_size) {
		size = 2 * max_state_size + 65535;
	}
	if (size > 64 * 256)
		size = 64 * 256;

	size = 64 * 1024 / 4;

	if (RADEON_DEBUG & (DEBUG_IOCTL | DEBUG_DMA)) {
		fprintf(stderr, "sizeof(drm_r300_cmd_header_t)=%zd\n",
			sizeof(drm_r300_cmd_header_t));
		fprintf(stderr, "sizeof(drm_radeon_cmd_buffer_t)=%zd\n",
			sizeof(drm_radeon_cmd_buffer_t));
		fprintf(stderr,
			"Allocating %d bytes command buffer (max state is %d bytes)\n",
			size * 4, max_state_size * 4);
	}

	if (rmesa->radeonScreen->kernel_mm) {
		int fd = rmesa->radeonScreen->driScreen->fd;
		rmesa->cmdbuf.csm = radeon_cs_manager_gem_ctor(fd);
	} else {
		rmesa->cmdbuf.csm = radeon_cs_manager_legacy_ctor(rmesa);
	}
	if (rmesa->cmdbuf.csm == NULL) {
		/* FIXME: fatal error */
		return;
	}
	rmesa->cmdbuf.cs = radeon_cs_create(rmesa->cmdbuf.csm, size);
	assert(rmesa->cmdbuf.cs != NULL);
	rmesa->cmdbuf.size = size;

}
/**
 * Destroy the command buffer
 */
void rcommonDestroyCmdBuf(radeonContextPtr rmesa)
{
	radeon_cs_destroy(rmesa->cmdbuf.cs);
	if (rmesa->radeonScreen->driScreen->dri2.enabled || rmesa->radeonScreen->kernel_mm) {
		radeon_cs_manager_gem_dtor(rmesa->cmdbuf.csm);
	} else {
		radeon_cs_manager_legacy_dtor(rmesa->cmdbuf.csm);
	}
}

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
#include "xmlpool.h"		/* for symbolic values of enum-type options */

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
#include "common_cmdbuf.h"

#define DRIVER_DATE "20090101"

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

void rcommonBeginBatch(radeonContextPtr rmesa, int n,
		       int dostate,
		       const char *file,
		       const char *function,
		       int line)
{
	rcommonEnsureCmdBufSpace(rmesa, n, function);
	if (!rmesa->cmdbuf.cs->cdw && dostate) {
		if (RADEON_DEBUG & DEBUG_IOCTL)
		  fprintf(stderr, "Reemit state after flush (from %s)\n", function);
		rmesa->vtbl.emit_state(rmesa);
	}
	radeon_cs_begin(rmesa->cmdbuf.cs, n, file, function, line);
}



/* Return various strings for glGetString().
 */
static const GLubyte *radeonGetString(GLcontext * ctx, GLenum name)
{
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);
	static char buffer[128];

	switch (name) {
	case GL_VENDOR:
		if (IS_R300_CLASS(radeon->radeonScreen))
			return (GLubyte *) "DRI R300 Project";
		else
			return (GLubyte *) "Tungsten Graphics, Inc.";

	case GL_RENDERER:
	{
		unsigned offset;
		GLuint agp_mode = (radeon->radeonScreen->card_type==RADEON_CARD_PCI) ? 0 :
			radeon->radeonScreen->AGPMode;
		const char* chipname;

			

		if (IS_R300_CLASS(radeon->radeonScreen))
			chipname = "R300";
		else
			chipname = "R200";

		offset = driGetRendererString(buffer, chipname, DRIVER_DATE,
					      agp_mode);

		if (IS_R300_CLASS(radeon->radeonScreen)) {
			sprintf(&buffer[offset], " %sTCL",
				(radeon->radeonScreen->chip_flags & RADEON_CHIPSET_TCL)
				? "" : "NO-");
		} else {
			sprintf(&buffer[offset], " %sTCL",
				!(radeon->TclFallback & RADEON_TCL_FALLBACK_TCL_DISABLE)
				? "" : "NO-");
		}

		if (radeon->radeonScreen->driScreen->dri2.enabled)
			strcat(buffer, " DRI2");

		return (GLubyte *) buffer;
	}

	default:
		return NULL;
	}
}

/* Initialize the driver's misc functions.
 */
static void radeonInitDriverFuncs(struct dd_function_table *functions)
{
	functions->GetString = radeonGetString;
}

/**
 * Create and initialize all common fields of the context,
 * including the Mesa context itself.
 */
GLboolean radeonInitContext(radeonContextPtr radeon,
			    struct dd_function_table* functions,
			    const __GLcontextModes * glVisual,
			    __DRIcontextPrivate * driContextPriv,
			    void *sharedContextPrivate)
{
	__DRIscreenPrivate *sPriv = driContextPriv->driScreenPriv;
	radeonScreenPtr screen = (radeonScreenPtr) (sPriv->private);
	GLcontext* ctx;
	GLcontext* shareCtx;
	int fthrottle_mode;

	/* Fill in additional standard functions. */
	radeonInitDriverFuncs(functions);

	radeon->radeonScreen = screen;
	/* Allocate and initialize the Mesa context */
	if (sharedContextPrivate)
		shareCtx = ((radeonContextPtr)sharedContextPrivate)->glCtx;
	else
		shareCtx = NULL;
	radeon->glCtx = _mesa_create_context(glVisual, shareCtx,
					    functions, (void *)radeon);
	if (!radeon->glCtx)
		return GL_FALSE;

	ctx = radeon->glCtx;
	driContextPriv->driverPrivate = radeon;

	/* DRI fields */
	radeon->dri.context = driContextPriv;
	radeon->dri.screen = sPriv;
	radeon->dri.drawable = NULL;
	radeon->dri.readable = NULL;
	radeon->dri.hwContext = driContextPriv->hHWContext;
	radeon->dri.hwLock = &sPriv->pSAREA->lock;
	radeon->dri.fd = sPriv->fd;
	radeon->dri.drmMinor = sPriv->drm_version.minor;

	radeon->sarea = (drm_radeon_sarea_t *) ((GLubyte *) sPriv->pSAREA +
					       screen->sarea_priv_offset);

	/* Setup IRQs */
	fthrottle_mode = driQueryOptioni(&radeon->optionCache, "fthrottle_mode");
	radeon->iw.irq_seq = -1;
	radeon->irqsEmitted = 0;
	radeon->do_irqs = (fthrottle_mode == DRI_CONF_FTHROTTLE_IRQS &&
			  radeon->radeonScreen->irq);

	radeon->do_usleeps = (fthrottle_mode == DRI_CONF_FTHROTTLE_USLEEPS);

	if (!radeon->do_irqs)
		fprintf(stderr,
			"IRQ's not enabled, falling back to %s: %d %d\n",
			radeon->do_usleeps ? "usleeps" : "busy waits",
			fthrottle_mode, radeon->radeonScreen->irq);

	(*sPriv->systemTime->getUST) (&radeon->swap_ust);

	return GL_TRUE;
}

/**
 * Cleanup common context fields.
 * Called by r200DestroyContext/r300DestroyContext
 */
void radeonCleanupContext(radeonContextPtr radeon)
{
	FILE *track;
	struct radeon_renderbuffer *rb;
	GLframebuffer *fb;
	
	fb = (void*)radeon->dri.drawable->driverPrivate;
	rb = (void *)fb->Attachment[BUFFER_FRONT_LEFT].Renderbuffer;
	if (rb && rb->bo) {
		radeon_bo_unref(rb->bo);
		rb->bo = NULL;
	}
	rb = (void *)fb->Attachment[BUFFER_BACK_LEFT].Renderbuffer;
	if (rb && rb->bo) {
		radeon_bo_unref(rb->bo);
		rb->bo = NULL;
	}
	rb = (void *)fb->Attachment[BUFFER_DEPTH].Renderbuffer;
	if (rb && rb->bo) {
		radeon_bo_unref(rb->bo);
		rb->bo = NULL;
	}
	fb = (void*)radeon->dri.readable->driverPrivate;
	rb = (void *)fb->Attachment[BUFFER_FRONT_LEFT].Renderbuffer;
	if (rb && rb->bo) {
		radeon_bo_unref(rb->bo);
		rb->bo = NULL;
	}
	rb = (void *)fb->Attachment[BUFFER_BACK_LEFT].Renderbuffer;
	if (rb && rb->bo) {
		radeon_bo_unref(rb->bo);
		rb->bo = NULL;
	}
	rb = (void *)fb->Attachment[BUFFER_DEPTH].Renderbuffer;
	if (rb && rb->bo) {
		radeon_bo_unref(rb->bo);
		rb->bo = NULL;
	}
	
	/* _mesa_destroy_context() might result in calls to functions that
	 * depend on the DriverCtx, so don't set it to NULL before.
	 *
	 * radeon->glCtx->DriverCtx = NULL;
	 */

	/* free the Mesa context */
	_mesa_destroy_context(radeon->glCtx);

	/* free the option cache */
	driDestroyOptionCache(&radeon->optionCache);

	if (radeon->state.scissor.pClipRects) {
		FREE(radeon->state.scissor.pClipRects);
		radeon->state.scissor.pClipRects = 0;
	}
	track = fopen("/tmp/tracklog", "w");
	if (track) {
		radeon_tracker_print(&radeon->radeonScreen->bom->tracker, track);
		fclose(track);
	}
}

void
radeon_make_kernel_renderbuffer_current(radeonContextPtr radeon,
					GLframebuffer *draw)
{
	/* if radeon->fake */
	struct radeon_renderbuffer *rb;

	if ((rb = (void *)draw->Attachment[BUFFER_FRONT_LEFT].Renderbuffer)) {
		
		if (!rb->bo) {
			rb->bo = radeon_bo_open(radeon->radeonScreen->bom,
						radeon->radeonScreen->frontOffset,
						0,
						0,
						RADEON_GEM_DOMAIN_VRAM,
						0);
		}
		rb->cpp = radeon->radeonScreen->cpp;
		rb->pitch = radeon->radeonScreen->frontPitch * rb->cpp;
	}
	if ((rb = (void *)draw->Attachment[BUFFER_BACK_LEFT].Renderbuffer)) {
		if (!rb->bo) {
			rb->bo = radeon_bo_open(radeon->radeonScreen->bom,
						radeon->radeonScreen->backOffset,
						0,
						0,
						RADEON_GEM_DOMAIN_VRAM,
						0);
		}
		rb->cpp = radeon->radeonScreen->cpp;
		rb->pitch = radeon->radeonScreen->backPitch * rb->cpp;
	}
	if ((rb = (void *)draw->Attachment[BUFFER_DEPTH].Renderbuffer)) {
		if (!rb->bo) {
			rb->bo = radeon_bo_open(radeon->radeonScreen->bom,
						radeon->radeonScreen->depthOffset,
						0,
						0,
						RADEON_GEM_DOMAIN_VRAM,
						0);
		}
		rb->cpp = radeon->radeonScreen->cpp;
		rb->pitch = radeon->radeonScreen->depthPitch * rb->cpp;
	}
}

static void
radeon_make_renderbuffer_current(radeonContextPtr radeon,
					GLframebuffer *draw)
{
	int size = 4096*4096*4;
	/* if radeon->fake */
	struct radeon_renderbuffer *rb;
	
	if (radeon->radeonScreen->kernel_mm) {
		radeon_make_kernel_renderbuffer_current(radeon, draw);
		return;
	}
			

	if ((rb = (void *)draw->Attachment[BUFFER_FRONT_LEFT].Renderbuffer)) {
		if (!rb->bo) {
			rb->bo = radeon_bo_open(radeon->radeonScreen->bom,
						radeon->radeonScreen->frontOffset +
						radeon->radeonScreen->fbLocation,
						size,
						4096,
						RADEON_GEM_DOMAIN_VRAM,
						0);
		}
		rb->cpp = radeon->radeonScreen->cpp;
		rb->pitch = radeon->radeonScreen->frontPitch * rb->cpp;
	}
	if ((rb = (void *)draw->Attachment[BUFFER_BACK_LEFT].Renderbuffer)) {
		if (!rb->bo) {
			rb->bo = radeon_bo_open(radeon->radeonScreen->bom,
						radeon->radeonScreen->backOffset +
						radeon->radeonScreen->fbLocation,
						size,
						4096,
						RADEON_GEM_DOMAIN_VRAM,
						0);
		}
		rb->cpp = radeon->radeonScreen->cpp;
		rb->pitch = radeon->radeonScreen->backPitch * rb->cpp;
	}
	if ((rb = (void *)draw->Attachment[BUFFER_DEPTH].Renderbuffer)) {
		if (!rb->bo) {
			rb->bo = radeon_bo_open(radeon->radeonScreen->bom,
						radeon->radeonScreen->depthOffset +
						radeon->radeonScreen->fbLocation,
						size,
						4096,
						RADEON_GEM_DOMAIN_VRAM,
						0);
		}
		rb->cpp = radeon->radeonScreen->cpp;
		rb->pitch = radeon->radeonScreen->depthPitch * rb->cpp;
	}
}


void
radeon_update_renderbuffers(__DRIcontext *context, __DRIdrawable *drawable)
{
	unsigned int attachments[10];
	__DRIbuffer *buffers;
	__DRIscreen *screen;
	struct radeon_renderbuffer *rb;
	int i, count;
	GLframebuffer *draw;
	radeonContextPtr radeon;
	
	draw = drawable->driverPrivate;
	screen = context->driScreenPriv;
	radeon = (radeonContextPtr) context->driverPrivate;
	i = 0;
	if ((rb = (void *)draw->Attachment[BUFFER_FRONT_LEFT].Renderbuffer)) {
		attachments[i++] = __DRI_BUFFER_FRONT_LEFT;
	}
	if ((rb = (void *)draw->Attachment[BUFFER_BACK_LEFT].Renderbuffer)) {
		attachments[i++] = __DRI_BUFFER_BACK_LEFT;
	}
	if ((rb = (void *)draw->Attachment[BUFFER_DEPTH].Renderbuffer)) {
		attachments[i++] = __DRI_BUFFER_DEPTH;
	}
	
	buffers = (*screen->dri2.loader->getBuffers)(drawable,
						     &drawable->w,
						     &drawable->h,
						     attachments, i,
						     &count,
						     drawable->loaderPrivate);
	if (buffers == NULL)
		return;

	/* set one cliprect to cover the whole drawable */
	drawable->x = 0;
	drawable->y = 0;
	drawable->backX = 0;
	drawable->backY = 0;
	drawable->numClipRects = 1;
	drawable->pClipRects[0].x1 = 0;
	drawable->pClipRects[0].y1 = 0;
	drawable->pClipRects[0].x2 = drawable->w;
	drawable->pClipRects[0].y2 = drawable->h;
	drawable->numBackClipRects = 1;
	drawable->pBackClipRects[0].x1 = 0;
	drawable->pBackClipRects[0].y1 = 0;
	drawable->pBackClipRects[0].x2 = drawable->w;
	drawable->pBackClipRects[0].y2 = drawable->h;
	for (i = 0; i < count; i++) {
		switch (buffers[i].attachment) {
		case __DRI_BUFFER_FRONT_LEFT:
			rb = (void *)draw->Attachment[BUFFER_FRONT_LEFT].Renderbuffer;
			if (rb->bo) {
				radeon_bo_unref(rb->bo);
				rb->bo = NULL;
			}
			rb->cpp = buffers[i].cpp;
			rb->pitch = buffers[i].pitch;
			rb->width = drawable->w;
			rb->height = drawable->h;
			rb->has_surface = 0;
			rb->bo = radeon_bo_open(radeon->radeonScreen->bom,
						buffers[i].name,
						0,
						0,
						RADEON_GEM_DOMAIN_VRAM,
						buffers[i].flags);
			if (rb->bo == NULL) {
				fprintf(stderr, "failled to attach front %d\n",
					buffers[i].name);
			}
			break;
		case __DRI_BUFFER_BACK_LEFT:
			rb = (void *)draw->Attachment[BUFFER_BACK_LEFT].Renderbuffer;
			if (rb->bo) {
				radeon_bo_unref(rb->bo);
				rb->bo = NULL;
			}
			rb->cpp = buffers[i].cpp;
			rb->pitch = buffers[i].pitch;
			rb->width = drawable->w;
			rb->height = drawable->h;
			rb->has_surface = 0;
			rb->bo = radeon_bo_open(radeon->radeonScreen->bom,
						buffers[i].name,
						0,
						0,
						RADEON_GEM_DOMAIN_VRAM,
						buffers[i].flags);
			break;
		case __DRI_BUFFER_DEPTH:
			rb = (void *)draw->Attachment[BUFFER_DEPTH].Renderbuffer;
			if (rb->bo) {
				radeon_bo_unref(rb->bo);
				rb->bo = NULL;
			}
			rb->cpp = buffers[i].cpp;
			rb->pitch = buffers[i].pitch;
			rb->width = drawable->w;
			rb->height = drawable->h;
			rb->has_surface = 0;
			rb->bo = radeon_bo_open(radeon->radeonScreen->bom,
						buffers[i].name,
						0,
						0,
						RADEON_GEM_DOMAIN_VRAM,
						buffers[i].flags);
			break;
		case __DRI_BUFFER_STENCIL:
			break;
		case __DRI_BUFFER_ACCUM:
		default:
			fprintf(stderr,
				"unhandled buffer attach event, attacment type %d\n",
				buffers[i].attachment);
			return;
		}
	}
	radeon = (radeonContextPtr) context->driverPrivate;
	driUpdateFramebufferSize(radeon->glCtx, drawable);
}

/* Force the context `c' to be the current context and associate with it
 * buffer `b'.
 */
GLboolean radeonMakeCurrent(__DRIcontextPrivate * driContextPriv,
			    __DRIdrawablePrivate * driDrawPriv,
			    __DRIdrawablePrivate * driReadPriv)
{
	radeonContextPtr radeon;
	GLframebuffer *dfb, *rfb;

	if (!driContextPriv) {
		if (RADEON_DEBUG & DEBUG_DRI)
			fprintf(stderr, "%s ctx is null\n", __FUNCTION__);
		_mesa_make_current(NULL, NULL, NULL);
		return GL_TRUE;
	}
	radeon = (radeonContextPtr) driContextPriv->driverPrivate;
	dfb = driDrawPriv->driverPrivate;
	rfb = driReadPriv->driverPrivate;

	if (driContextPriv->driScreenPriv->dri2.enabled) {    
		radeon_update_renderbuffers(driContextPriv, driDrawPriv);
		if (driDrawPriv != driReadPriv)
			radeon_update_renderbuffers(driContextPriv, driReadPriv);
		radeon->state.color.rrb =
			(void *)dfb->Attachment[BUFFER_BACK_LEFT].Renderbuffer;
		radeon->state.depth.rrb =
			(void *)dfb->Attachment[BUFFER_DEPTH].Renderbuffer;
	}


	if (RADEON_DEBUG & DEBUG_DRI)
		fprintf(stderr, "%s ctx %p\n", __FUNCTION__, radeon->glCtx);

	driUpdateFramebufferSize(radeon->glCtx, driDrawPriv);
	if (driReadPriv != driDrawPriv)
		driUpdateFramebufferSize(radeon->glCtx, driReadPriv);

	if (!driContextPriv->driScreenPriv->dri2.enabled) {
		radeon_make_renderbuffer_current(radeon, dfb);
	}
	
	_mesa_make_current(radeon->glCtx, dfb, rfb);

	if (radeon->dri.drawable != driDrawPriv) {
		if (driDrawPriv->swap_interval == (unsigned)-1) {
			driDrawPriv->vblFlags =
				(radeon->radeonScreen->irq != 0)
				? driGetDefaultVBlankFlags(&radeon->
							   optionCache)
					: VBLANK_FLAG_NO_IRQ;

			driDrawableInitVBlank(driDrawPriv);
		}
	}

	radeon->dri.readable = driReadPriv;

	if (radeon->dri.drawable != driDrawPriv ||
	    radeon->lastStamp != driDrawPriv->lastStamp) {
		radeon->dri.drawable = driDrawPriv;

		radeonSetCliprects(radeon);
		radeon->vtbl.update_viewport_offset(radeon->glCtx);
	}

	_mesa_update_state(radeon->glCtx);

	if (!driContextPriv->driScreenPriv->dri2.enabled) {    
		radeonUpdatePageFlipping(radeon);
	}

	if (RADEON_DEBUG & DEBUG_DRI)
		fprintf(stderr, "End %s\n", __FUNCTION__);
	return GL_TRUE;
}


#if defined(USE_X86_ASM)
#define COPY_DWORDS( dst, src, nr )					\
do {									\
	int __tmp;							\
	__asm__ __volatile__( "rep ; movsl"				\
			      : "=%c" (__tmp), "=D" (dst), "=S" (__tmp)	\
			      : "0" (nr),				\
			        "D" ((long)dst),			\
			        "S" ((long)src) );			\
} while (0)
#else
#define COPY_DWORDS( dst, src, nr )		\
do {						\
   int j;					\
   for ( j = 0 ; j < nr ; j++ )			\
      dst[j] = ((int *)src)[j];			\
   dst += nr;					\
} while (0)
#endif

static void radeonEmitVec4(uint32_t *out, GLvoid * data, int stride, int count)
{
	int i;

	if (RADEON_DEBUG & DEBUG_VERTS)
		fprintf(stderr, "%s count %d stride %d out %p data %p\n",
			__FUNCTION__, count, stride, (void *)out, (void *)data);

	if (stride == 4)
		COPY_DWORDS(out, data, count);
	else
		for (i = 0; i < count; i++) {
			out[0] = *(int *)data;
			out++;
			data += stride;
		}
}

static void radeonEmitVec8(uint32_t *out, GLvoid * data, int stride, int count)
{
	int i;

	if (RADEON_DEBUG & DEBUG_VERTS)
		fprintf(stderr, "%s count %d stride %d out %p data %p\n",
			__FUNCTION__, count, stride, (void *)out, (void *)data);

	if (stride == 8)
		COPY_DWORDS(out, data, count * 2);
	else
		for (i = 0; i < count; i++) {
			out[0] = *(int *)data;
			out[1] = *(int *)(data + 4);
			out += 2;
			data += stride;
		}
}

static void radeonEmitVec12(uint32_t *out, GLvoid * data, int stride, int count)
{
	int i;

	if (RADEON_DEBUG & DEBUG_VERTS)
		fprintf(stderr, "%s count %d stride %d out %p data %p\n",
			__FUNCTION__, count, stride, (void *)out, (void *)data);

	if (stride == 12) {
		COPY_DWORDS(out, data, count * 3);
    }
	else
		for (i = 0; i < count; i++) {
			out[0] = *(int *)data;
			out[1] = *(int *)(data + 4);
			out[2] = *(int *)(data + 8);
			out += 3;
			data += stride;
		}
}

static void radeonEmitVec16(uint32_t *out, GLvoid * data, int stride, int count)
{
	int i;

	if (RADEON_DEBUG & DEBUG_VERTS)
		fprintf(stderr, "%s count %d stride %d out %p data %p\n",
			__FUNCTION__, count, stride, (void *)out, (void *)data);

	if (stride == 16)
		COPY_DWORDS(out, data, count * 4);
	else
		for (i = 0; i < count; i++) {
			out[0] = *(int *)data;
			out[1] = *(int *)(data + 4);
			out[2] = *(int *)(data + 8);
			out[3] = *(int *)(data + 12);
			out += 4;
			data += stride;
		}
}

void rcommon_emit_vector(GLcontext * ctx, struct radeon_aos *aos,
			 GLvoid * data, int size, int stride, int count)
{
	radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
	uint32_t *out;
	uint32_t bo_size;

	memset(aos, 0, sizeof(struct radeon_aos));
	if (stride == 0) {
		bo_size = size * 4;
		count = 1;
		aos->stride = 0;
	} else {
		bo_size = size * count * 4;
		aos->stride = size;
	}
	aos->bo = radeon_bo_open(rmesa->radeonScreen->bom,
				 0, bo_size, 32, RADEON_GEM_DOMAIN_GTT, 0);
	aos->offset = 0;
	aos->components = size;
	aos->count = count;

	radeon_bo_map(aos->bo, 1);
	out = (uint32_t*)((char*)aos->bo->ptr + aos->offset);
	switch (size) {
	case 1: radeonEmitVec4(out, data, stride, count); break;
	case 2: radeonEmitVec8(out, data, stride, count); break;
	case 3: radeonEmitVec12(out, data, stride, count); break;
	case 4: radeonEmitVec16(out, data, stride, count); break;
	default:
		assert(0);
		break;
	}
	radeon_bo_unmap(aos->bo);
}

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
#include "main/context.h"
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
#include "main/mipmap.h"
#include "main/texformat.h"
#include "main/texstore.h"
#include "main/teximage.h"
#include "main/texobj.h"

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
#include "radeon_mipmap_tree.h"

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


void radeon_print_state_atom( struct radeon_state_atom *state )
{
   int i;

   fprintf(stderr, "emit %s/%d\n", state->name, state->cmd_size);

   if (RADEON_DEBUG & DEBUG_VERBOSE) 
      for (i = 0 ; i < state->cmd_size ; i++) 
	 fprintf(stderr, "\t%s[%d]: %x\n", state->name, i, state->cmd[i]);

}

/* textures */
/**
 * Allocate an empty texture image object.
 */
struct gl_texture_image *radeonNewTextureImage(GLcontext *ctx)
{
	return CALLOC(sizeof(radeon_texture_image));
}

/**
 * Free memory associated with this texture image.
 */
void radeonFreeTexImageData(GLcontext *ctx, struct gl_texture_image *timage)
{
	radeon_texture_image* image = get_radeon_texture_image(timage);

	if (image->mt) {
		radeon_miptree_unreference(image->mt);
		image->mt = 0;
		assert(!image->base.Data);
	} else {
		_mesa_free_texture_image_data(ctx, timage);
	}
	if (image->bo) {
		radeon_bo_unref(image->bo);
		image->bo = NULL;
	}
}

/* Set Data pointer and additional data for mapped texture image */
static void teximage_set_map_data(radeon_texture_image *image)
{
	radeon_mipmap_level *lvl = &image->mt->levels[image->mtlevel];
	image->base.Data = image->mt->bo->ptr + lvl->faces[image->mtface].offset;
	image->base.RowStride = lvl->rowstride / image->mt->bpp;
}


/**
 * Map a single texture image for glTexImage and friends.
 */
void radeon_teximage_map(radeon_texture_image *image, GLboolean write_enable)
{
	if (image->mt) {
		assert(!image->base.Data);

		radeon_bo_map(image->mt->bo, write_enable);
		teximage_set_map_data(image);
	}
}


void radeon_teximage_unmap(radeon_texture_image *image)
{
	if (image->mt) {
		assert(image->base.Data);

		image->base.Data = 0;
		radeon_bo_unmap(image->mt->bo);
	}
}

/**
 * Map a validated texture for reading during software rendering.
 */
void radeonMapTexture(GLcontext *ctx, struct gl_texture_object *texObj)
{
	radeonTexObj* t = radeon_tex_obj(texObj);
	int face, level;

	assert(texObj->_Complete);
	assert(t->mt);

	radeon_bo_map(t->mt->bo, GL_FALSE);
	for(face = 0; face < t->mt->faces; ++face) {
		for(level = t->mt->firstLevel; level <= t->mt->lastLevel; ++level)
			teximage_set_map_data(get_radeon_texture_image(texObj->Image[face][level]));
	}
}

void radeonUnmapTexture(GLcontext *ctx, struct gl_texture_object *texObj)
{
	radeonTexObj* t = radeon_tex_obj(texObj);
	int face, level;

	assert(texObj->_Complete);
	assert(t->mt);

	for(face = 0; face < t->mt->faces; ++face) {
		for(level = t->mt->firstLevel; level <= t->mt->lastLevel; ++level)
			texObj->Image[face][level]->Data = 0;
	}
	radeon_bo_unmap(t->mt->bo);
}

GLuint radeon_face_for_target(GLenum target)
{
	switch (target) {
	case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
		return (GLuint) target - (GLuint) GL_TEXTURE_CUBE_MAP_POSITIVE_X;
	default:
		return 0;
	}
}

/**
 * Wraps Mesa's implementation to ensure that the base level image is mapped.
 *
 * This relies on internal details of _mesa_generate_mipmap, in particular
 * the fact that the memory for recreated texture images is always freed.
 */
void radeon_generate_mipmap(GLcontext* ctx, GLenum target, struct gl_texture_object *texObj)
{
	GLuint face = radeon_face_for_target(target);
	radeon_texture_image *baseimage = get_radeon_texture_image(texObj->Image[face][texObj->BaseLevel]);

	radeon_teximage_map(baseimage, GL_FALSE);
	_mesa_generate_mipmap(ctx, target, texObj);
	radeon_teximage_unmap(baseimage);
}


/* try to find a format which will only need a memcopy */
static const struct gl_texture_format *radeonChoose8888TexFormat(GLenum srcFormat,
							       GLenum srcType)
{
	const GLuint ui = 1;
	const GLubyte littleEndian = *((const GLubyte *)&ui);

	if ((srcFormat == GL_RGBA && srcType == GL_UNSIGNED_INT_8_8_8_8) ||
	    (srcFormat == GL_RGBA && srcType == GL_UNSIGNED_BYTE && !littleEndian) ||
	    (srcFormat == GL_ABGR_EXT && srcType == GL_UNSIGNED_INT_8_8_8_8_REV) ||
	    (srcFormat == GL_ABGR_EXT && srcType == GL_UNSIGNED_BYTE && littleEndian)) {
		return &_mesa_texformat_rgba8888;
	} else if ((srcFormat == GL_RGBA && srcType == GL_UNSIGNED_INT_8_8_8_8_REV) ||
		   (srcFormat == GL_RGBA && srcType == GL_UNSIGNED_BYTE && littleEndian) ||
		   (srcFormat == GL_ABGR_EXT && srcType == GL_UNSIGNED_INT_8_8_8_8) ||
		   (srcFormat == GL_ABGR_EXT && srcType == GL_UNSIGNED_BYTE && !littleEndian)) {
		return &_mesa_texformat_rgba8888_rev;
	} else if (srcFormat == GL_BGRA && ((srcType == GL_UNSIGNED_BYTE && !littleEndian) ||
					    srcType == GL_UNSIGNED_INT_8_8_8_8)) {
		return &_mesa_texformat_argb8888_rev;
	} else if (srcFormat == GL_BGRA && ((srcType == GL_UNSIGNED_BYTE && littleEndian) ||
					    srcType == GL_UNSIGNED_INT_8_8_8_8_REV)) {
		return &_mesa_texformat_argb8888;
	} else
		return _dri_texformat_argb8888;
}

const struct gl_texture_format *radeonChooseTextureFormat(GLcontext * ctx,
							  GLint internalFormat,
							  GLenum format,
							  GLenum type)
{
	radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
	const GLboolean do32bpt =
	    (rmesa->texture_depth == DRI_CONF_TEXTURE_DEPTH_32);
	const GLboolean force16bpt =
	    (rmesa->texture_depth == DRI_CONF_TEXTURE_DEPTH_FORCE_16);
	(void)format;

#if 0
	fprintf(stderr, "InternalFormat=%s(%d) type=%s format=%s\n",
		_mesa_lookup_enum_by_nr(internalFormat), internalFormat,
		_mesa_lookup_enum_by_nr(type), _mesa_lookup_enum_by_nr(format));
	fprintf(stderr, "do32bpt=%d force16bpt=%d\n", do32bpt, force16bpt);
#endif

	switch (internalFormat) {
	case 4:
	case GL_RGBA:
	case GL_COMPRESSED_RGBA:
		switch (type) {
		case GL_UNSIGNED_INT_10_10_10_2:
		case GL_UNSIGNED_INT_2_10_10_10_REV:
			return do32bpt ? _dri_texformat_argb8888 :
			    _dri_texformat_argb1555;
		case GL_UNSIGNED_SHORT_4_4_4_4:
		case GL_UNSIGNED_SHORT_4_4_4_4_REV:
			return _dri_texformat_argb4444;
		case GL_UNSIGNED_SHORT_5_5_5_1:
		case GL_UNSIGNED_SHORT_1_5_5_5_REV:
			return _dri_texformat_argb1555;
		default:
			return do32bpt ? radeonChoose8888TexFormat(format, type) :
			    _dri_texformat_argb4444;
		}

	case 3:
	case GL_RGB:
	case GL_COMPRESSED_RGB:
		switch (type) {
		case GL_UNSIGNED_SHORT_4_4_4_4:
		case GL_UNSIGNED_SHORT_4_4_4_4_REV:
			return _dri_texformat_argb4444;
		case GL_UNSIGNED_SHORT_5_5_5_1:
		case GL_UNSIGNED_SHORT_1_5_5_5_REV:
			return _dri_texformat_argb1555;
		case GL_UNSIGNED_SHORT_5_6_5:
		case GL_UNSIGNED_SHORT_5_6_5_REV:
			return _dri_texformat_rgb565;
		default:
			return do32bpt ? _dri_texformat_argb8888 :
			    _dri_texformat_rgb565;
		}

	case GL_RGBA8:
	case GL_RGB10_A2:
	case GL_RGBA12:
	case GL_RGBA16:
		return !force16bpt ?
		    radeonChoose8888TexFormat(format,
					    type) : _dri_texformat_argb4444;

	case GL_RGBA4:
	case GL_RGBA2:
		return _dri_texformat_argb4444;

	case GL_RGB5_A1:
		return _dri_texformat_argb1555;

	case GL_RGB8:
	case GL_RGB10:
	case GL_RGB12:
	case GL_RGB16:
		return !force16bpt ? _dri_texformat_argb8888 :
		    _dri_texformat_rgb565;

	case GL_RGB5:
	case GL_RGB4:
	case GL_R3_G3_B2:
		return _dri_texformat_rgb565;

	case GL_ALPHA:
	case GL_ALPHA4:
	case GL_ALPHA8:
	case GL_ALPHA12:
	case GL_ALPHA16:
	case GL_COMPRESSED_ALPHA:
		return _dri_texformat_a8;

	case 1:
	case GL_LUMINANCE:
	case GL_LUMINANCE4:
	case GL_LUMINANCE8:
	case GL_LUMINANCE12:
	case GL_LUMINANCE16:
	case GL_COMPRESSED_LUMINANCE:
		return _dri_texformat_l8;

	case 2:
	case GL_LUMINANCE_ALPHA:
	case GL_LUMINANCE4_ALPHA4:
	case GL_LUMINANCE6_ALPHA2:
	case GL_LUMINANCE8_ALPHA8:
	case GL_LUMINANCE12_ALPHA4:
	case GL_LUMINANCE12_ALPHA12:
	case GL_LUMINANCE16_ALPHA16:
	case GL_COMPRESSED_LUMINANCE_ALPHA:
		return _dri_texformat_al88;

	case GL_INTENSITY:
	case GL_INTENSITY4:
	case GL_INTENSITY8:
	case GL_INTENSITY12:
	case GL_INTENSITY16:
	case GL_COMPRESSED_INTENSITY:
		return _dri_texformat_i8;

	case GL_YCBCR_MESA:
		if (type == GL_UNSIGNED_SHORT_8_8_APPLE ||
		    type == GL_UNSIGNED_BYTE)
			return &_mesa_texformat_ycbcr;
		else
			return &_mesa_texformat_ycbcr_rev;

	case GL_RGB_S3TC:
	case GL_RGB4_S3TC:
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
		return &_mesa_texformat_rgb_dxt1;

	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		return &_mesa_texformat_rgba_dxt1;

	case GL_RGBA_S3TC:
	case GL_RGBA4_S3TC:
	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
		return &_mesa_texformat_rgba_dxt3;

	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		return &_mesa_texformat_rgba_dxt5;

	case GL_ALPHA16F_ARB:
		return &_mesa_texformat_alpha_float16;
	case GL_ALPHA32F_ARB:
		return &_mesa_texformat_alpha_float32;
	case GL_LUMINANCE16F_ARB:
		return &_mesa_texformat_luminance_float16;
	case GL_LUMINANCE32F_ARB:
		return &_mesa_texformat_luminance_float32;
	case GL_LUMINANCE_ALPHA16F_ARB:
		return &_mesa_texformat_luminance_alpha_float16;
	case GL_LUMINANCE_ALPHA32F_ARB:
		return &_mesa_texformat_luminance_alpha_float32;
	case GL_INTENSITY16F_ARB:
		return &_mesa_texformat_intensity_float16;
	case GL_INTENSITY32F_ARB:
		return &_mesa_texformat_intensity_float32;
	case GL_RGB16F_ARB:
		return &_mesa_texformat_rgba_float16;
	case GL_RGB32F_ARB:
		return &_mesa_texformat_rgba_float32;
	case GL_RGBA16F_ARB:
		return &_mesa_texformat_rgba_float16;
	case GL_RGBA32F_ARB:
		return &_mesa_texformat_rgba_float32;

	case GL_DEPTH_COMPONENT:
	case GL_DEPTH_COMPONENT16:
	case GL_DEPTH_COMPONENT24:
	case GL_DEPTH_COMPONENT32:
#if 0
		switch (type) {
		case GL_UNSIGNED_BYTE:
		case GL_UNSIGNED_SHORT:
			return &_mesa_texformat_z16;
		case GL_UNSIGNED_INT:
			return &_mesa_texformat_z32;
		case GL_UNSIGNED_INT_24_8_EXT:
		default:
			return &_mesa_texformat_z24_s8;
		}
#else
		return &_mesa_texformat_z16;
#endif

	default:
		_mesa_problem(ctx,
			      "unexpected internalFormat 0x%x in r300ChooseTextureFormat",
			      (int)internalFormat);
		return NULL;
	}

	return NULL;		/* never get here */
}

/**
 * All glTexImage calls go through this function.
 */
static void radeon_teximage(
	GLcontext *ctx, int dims,
	GLint face, GLint level,
	GLint internalFormat,
	GLint width, GLint height, GLint depth,
	GLsizei imageSize,
	GLenum format, GLenum type, const GLvoid * pixels,
	const struct gl_pixelstore_attrib *packing,
	struct gl_texture_object *texObj,
	struct gl_texture_image *texImage,
	int compressed)
{
	radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
	radeonTexObj* t = radeon_tex_obj(texObj);
	radeon_texture_image* image = get_radeon_texture_image(texImage);

	rmesa->vtbl.flush_vertices(rmesa);

	t->validated = GL_FALSE;

	/* Choose and fill in the texture format for this image */
	texImage->TexFormat = radeonChooseTextureFormat(ctx, internalFormat, format, type);
	_mesa_set_fetch_functions(texImage, dims);

	if (texImage->TexFormat->TexelBytes == 0) {
		texImage->IsCompressed = GL_TRUE;
		texImage->CompressedSize =
			ctx->Driver.CompressedTextureSize(ctx, texImage->Width,
					   texImage->Height, texImage->Depth,
					   texImage->TexFormat->MesaFormat);
	} else {
		texImage->IsCompressed = GL_FALSE;
		texImage->CompressedSize = 0;
	}

	/* Allocate memory for image */
	radeonFreeTexImageData(ctx, texImage); /* Mesa core only clears texImage->Data but not image->mt */

	if (!t->mt)
		radeon_try_alloc_miptree(rmesa, t, texImage, face, level);
	if (t->mt && radeon_miptree_matches_image(t->mt, texImage, face, level)) {
		image->mt = t->mt;
		image->mtlevel = level - t->mt->firstLevel;
		image->mtface = face;
		radeon_miptree_reference(t->mt);
	} else {
		int size;
		if (texImage->IsCompressed) {
			size = texImage->CompressedSize;
		} else {
			size = texImage->Width * texImage->Height * texImage->Depth * texImage->TexFormat->TexelBytes;
		}
		texImage->Data = _mesa_alloc_texmemory(size);
	}

	/* Upload texture image; note that the spec allows pixels to be NULL */
	if (compressed) {
		pixels = _mesa_validate_pbo_compressed_teximage(
			ctx, imageSize, pixels, packing, "glCompressedTexImage");
	} else {
		pixels = _mesa_validate_pbo_teximage(
			ctx, dims, width, height, depth,
			format, type, pixels, packing, "glTexImage");
	}

	if (pixels) {
		radeon_teximage_map(image, GL_TRUE);

		if (compressed) {
			memcpy(texImage->Data, pixels, imageSize);
		} else {
			GLuint dstRowStride;
			if (image->mt) {
				radeon_mipmap_level *lvl = &image->mt->levels[image->mtlevel];
				dstRowStride = lvl->rowstride;
			} else {
				dstRowStride = texImage->Width * texImage->TexFormat->TexelBytes;
			}
			if (!texImage->TexFormat->StoreImage(ctx, dims,
						texImage->_BaseFormat,
						texImage->TexFormat,
						texImage->Data, 0, 0, 0, /* dstX/Y/Zoffset */
						dstRowStride,
						texImage->ImageOffsets,
						width, height, depth,
						format, type, pixels, packing))
				_mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage");
		}

		radeon_teximage_unmap(image);
	}

	_mesa_unmap_teximage_pbo(ctx, packing);

	/* SGIS_generate_mipmap */
	if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
		ctx->Driver.GenerateMipmap(ctx, texObj->Target, texObj);
	}
}

void radeonTexImage1D(GLcontext * ctx, GLenum target, GLint level,
		      GLint internalFormat,
		      GLint width, GLint border,
		      GLenum format, GLenum type, const GLvoid * pixels,
		      const struct gl_pixelstore_attrib *packing,
		      struct gl_texture_object *texObj,
		      struct gl_texture_image *texImage)
{
	radeon_teximage(ctx, 1, 0, level, internalFormat, width, 1, 1,
		0, format, type, pixels, packing, texObj, texImage, 0);
}

void radeonTexImage2D(GLcontext * ctx, GLenum target, GLint level,
			   GLint internalFormat,
			   GLint width, GLint height, GLint border,
			   GLenum format, GLenum type, const GLvoid * pixels,
			   const struct gl_pixelstore_attrib *packing,
			   struct gl_texture_object *texObj,
			   struct gl_texture_image *texImage)

{
	GLuint face = radeon_face_for_target(target);

	radeon_teximage(ctx, 2, face, level, internalFormat, width, height, 1,
		0, format, type, pixels, packing, texObj, texImage, 0);
}

void radeonCompressedTexImage2D(GLcontext * ctx, GLenum target,
				     GLint level, GLint internalFormat,
				     GLint width, GLint height, GLint border,
				     GLsizei imageSize, const GLvoid * data,
				     struct gl_texture_object *texObj,
				     struct gl_texture_image *texImage)
{
	GLuint face = radeon_face_for_target(target);

	radeon_teximage(ctx, 2, face, level, internalFormat, width, height, 1,
		imageSize, 0, 0, data, 0, texObj, texImage, 1);
}

void radeonTexImage3D(GLcontext * ctx, GLenum target, GLint level,
		      GLint internalFormat,
		      GLint width, GLint height, GLint depth,
		      GLint border,
		      GLenum format, GLenum type, const GLvoid * pixels,
		      const struct gl_pixelstore_attrib *packing,
		      struct gl_texture_object *texObj,
		      struct gl_texture_image *texImage)
{
	radeon_teximage(ctx, 3, 0, level, internalFormat, width, height, depth,
		0, format, type, pixels, packing, texObj, texImage, 0);
}

/**
 * Update a subregion of the given texture image.
 */
static void radeon_texsubimage(GLcontext* ctx, int dims, int level,
		GLint xoffset, GLint yoffset, GLint zoffset,
		GLsizei width, GLsizei height, GLsizei depth,
		GLenum format, GLenum type,
		const GLvoid * pixels,
		const struct gl_pixelstore_attrib *packing,
		struct gl_texture_object *texObj,
		struct gl_texture_image *texImage,
		int compressed)
{
	radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
	radeon_texture_image* image = get_radeon_texture_image(texImage);

	rmesa->vtbl.flush_vertices(rmesa);

	pixels = _mesa_validate_pbo_teximage(ctx, dims,
		width, height, depth, format, type, pixels, packing, "glTexSubImage1D");

	if (pixels) {
		GLint dstRowStride;
		radeon_teximage_map(image, GL_TRUE);

		if (image->mt) {
			radeon_mipmap_level *lvl = &image->mt->levels[image->mtlevel];
			dstRowStride = lvl->rowstride;
		} else {
			dstRowStride = texImage->Width * texImage->TexFormat->TexelBytes;
		}

		if (!texImage->TexFormat->StoreImage(ctx, dims, texImage->_BaseFormat,
				texImage->TexFormat, texImage->Data,
				xoffset, yoffset, zoffset,
				dstRowStride,
				texImage->ImageOffsets,
				width, height, depth,
				format, type, pixels, packing))
			_mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexSubImage");

		radeon_teximage_unmap(image);
	}

	_mesa_unmap_teximage_pbo(ctx, packing);

	/* GL_SGIS_generate_mipmap */
	if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
		ctx->Driver.GenerateMipmap(ctx, texObj->Target, texObj);
	}
}

void radeonTexSubImage1D(GLcontext * ctx, GLenum target, GLint level,
			 GLint xoffset,
			 GLsizei width,
			 GLenum format, GLenum type,
			 const GLvoid * pixels,
			 const struct gl_pixelstore_attrib *packing,
			 struct gl_texture_object *texObj,
			 struct gl_texture_image *texImage)
{
	radeon_texsubimage(ctx, 1, level, xoffset, 0, 0, width, 1, 1,
		format, type, pixels, packing, texObj, texImage, 0);
}

void radeonTexSubImage2D(GLcontext * ctx, GLenum target, GLint level,
			 GLint xoffset, GLint yoffset,
			 GLsizei width, GLsizei height,
			 GLenum format, GLenum type,
			 const GLvoid * pixels,
			 const struct gl_pixelstore_attrib *packing,
			 struct gl_texture_object *texObj,
			 struct gl_texture_image *texImage)
{
	radeon_texsubimage(ctx, 2, level, xoffset, yoffset, 0, width, height,
			   1, format, type, pixels, packing, texObj, texImage,
			   0);
}

void radeonCompressedTexSubImage2D(GLcontext * ctx, GLenum target,
				   GLint level, GLint xoffset,
				   GLint yoffset, GLsizei width,
				   GLsizei height, GLenum format,
				   GLsizei imageSize, const GLvoid * data,
				   struct gl_texture_object *texObj,
				   struct gl_texture_image *texImage)
{
	radeon_texsubimage(ctx, 2, level, xoffset, yoffset, 0, width, height, 1,
		format, 0, data, 0, texObj, texImage, 1);
}


void radeonTexSubImage3D(GLcontext * ctx, GLenum target, GLint level,
			 GLint xoffset, GLint yoffset, GLint zoffset,
			 GLsizei width, GLsizei height, GLsizei depth,
			 GLenum format, GLenum type,
			 const GLvoid * pixels,
			 const struct gl_pixelstore_attrib *packing,
			 struct gl_texture_object *texObj,
			 struct gl_texture_image *texImage)
{
	radeon_texsubimage(ctx, 3, level, xoffset, yoffset, zoffset, width, height, depth,
		format, type, pixels, packing, texObj, texImage, 0);
}

static void copy_rows(void* dst, GLuint dststride, const void* src, GLuint srcstride,
	GLuint numrows, GLuint rowsize)
{
	assert(rowsize <= dststride);
	assert(rowsize <= srcstride);

	if (rowsize == srcstride && rowsize == dststride) {
		memcpy(dst, src, numrows*rowsize);
	} else {
		GLuint i;
		for(i = 0; i < numrows; ++i) {
			memcpy(dst, src, rowsize);
			dst += dststride;
			src += srcstride;
		}
	}
}


/**
 * Ensure that the given image is stored in the given miptree from now on.
 */
static void migrate_image_to_miptree(radeon_mipmap_tree *mt, radeon_texture_image *image, int face, int level)
{
	radeon_mipmap_level *dstlvl = &mt->levels[level - mt->firstLevel];
	unsigned char *dest;

	assert(image->mt != mt);
	assert(dstlvl->width == image->base.Width);
	assert(dstlvl->height == image->base.Height);
	assert(dstlvl->depth == image->base.Depth);

	radeon_bo_map(mt->bo, GL_TRUE);
	dest = mt->bo->ptr + dstlvl->faces[face].offset;

	if (image->mt) {
		/* Format etc. should match, so we really just need a memcpy().
		 * In fact, that memcpy() could be done by the hardware in many
		 * cases, provided that we have a proper memory manager.
		 */
		radeon_mipmap_level *srclvl = &image->mt->levels[image->mtlevel];

		assert(srclvl->size == dstlvl->size);
		assert(srclvl->rowstride == dstlvl->rowstride);

		radeon_bo_map(image->mt->bo, GL_FALSE);
		memcpy(dest,
			image->mt->bo->ptr + srclvl->faces[face].offset,
			dstlvl->size);
		radeon_bo_unmap(image->mt->bo);

		radeon_miptree_unreference(image->mt);
	} else {
		uint srcrowstride = image->base.Width * image->base.TexFormat->TexelBytes;

//		if (mt->tilebits)
//			WARN_ONCE("%s: tiling not supported yet", __FUNCTION__);

		copy_rows(dest, dstlvl->rowstride, image->base.Data, srcrowstride,
			image->base.Height * image->base.Depth, srcrowstride);

		_mesa_free_texmemory(image->base.Data);
		image->base.Data = 0;
	}

	radeon_bo_unmap(mt->bo);

	image->mt = mt;
	image->mtface = face;
	image->mtlevel = level;
	radeon_miptree_reference(image->mt);
}

int radeon_validate_texture_miptree(GLcontext * ctx, struct gl_texture_object *texObj)
{
	radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
	radeonTexObj *t = radeon_tex_obj(texObj);
	radeon_texture_image *baseimage = get_radeon_texture_image(texObj->Image[0][texObj->BaseLevel]);
	int face, level;

	if (t->validated || t->image_override)
		return GL_TRUE;

	if (RADEON_DEBUG & DEBUG_TEXTURE)
		fprintf(stderr, "%s: Validating texture %p now\n", __FUNCTION__, texObj);

	if (baseimage->base.Border > 0)
		return GL_FALSE;

	/* Ensure a matching miptree exists.
	 *
	 * Differing mipmap trees can result when the app uses TexImage to
	 * change texture dimensions.
	 *
	 * Prefer to use base image's miptree if it
	 * exists, since that most likely contains more valid data (remember
	 * that the base level is usually significantly larger than the rest
	 * of the miptree, so cubemaps are the only possible exception).
	 */
	if (baseimage->mt &&
	    baseimage->mt != t->mt &&
	    radeon_miptree_matches_texture(baseimage->mt, &t->base)) {
		radeon_miptree_unreference(t->mt);
		t->mt = baseimage->mt;
		radeon_miptree_reference(t->mt);
	} else if (t->mt && !radeon_miptree_matches_texture(t->mt, &t->base)) {
		radeon_miptree_unreference(t->mt);
		t->mt = 0;
	}

	if (!t->mt) {
		if (RADEON_DEBUG & DEBUG_TEXTURE)
			fprintf(stderr, " Allocate new miptree\n");
		radeon_try_alloc_miptree(rmesa, t, &baseimage->base, 0, texObj->BaseLevel);
		if (!t->mt) {
			_mesa_problem(ctx, "r300_validate_texture failed to alloc miptree");
			return GL_FALSE;
		}
	}

	/* Ensure all images are stored in the single main miptree */
	for(face = 0; face < t->mt->faces; ++face) {
		for(level = t->mt->firstLevel; level <= t->mt->lastLevel; ++level) {
			radeon_texture_image *image = get_radeon_texture_image(texObj->Image[face][level]);
			if (RADEON_DEBUG & DEBUG_TEXTURE)
				fprintf(stderr, " face %i, level %i... ", face, level);
			if (t->mt == image->mt) {
				if (RADEON_DEBUG & DEBUG_TEXTURE)
					fprintf(stderr, "OK\n");
				continue;
			}

			if (RADEON_DEBUG & DEBUG_TEXTURE)
				fprintf(stderr, "migrating\n");
			migrate_image_to_miptree(t->mt, image, face, level);
		}
	}

	return GL_TRUE;
}


GLubyte *radeon_ptr32(const struct radeon_renderbuffer * rrb,
		      GLint x, GLint y)
{
    GLubyte *ptr = rrb->bo->ptr;
    uint32_t mask = RADEON_BO_FLAGS_MACRO_TILE | RADEON_BO_FLAGS_MICRO_TILE;
    GLint offset;
    GLint nmacroblkpl;
    GLint nmicroblkpl;

    if (rrb->has_surface || !(rrb->bo->flags & mask)) {
        offset = x * rrb->cpp + y * rrb->pitch;
    } else {
        offset = 0;
        if (rrb->bo->flags & RADEON_BO_FLAGS_MACRO_TILE) {
            if (rrb->bo->flags & RADEON_BO_FLAGS_MICRO_TILE) {
                nmacroblkpl = rrb->pitch >> 5;
                offset += ((y >> 4) * nmacroblkpl) << 11;
                offset += ((y & 15) >> 1) << 8;
                offset += (y & 1) << 4;
                offset += (x >> 5) << 11;
                offset += ((x & 31) >> 2) << 5;
                offset += (x & 3) << 2;
            } else {
                nmacroblkpl = rrb->pitch >> 6;
                offset += ((y >> 3) * nmacroblkpl) << 11;
                offset += (y & 7) << 8;
                offset += (x >> 6) << 11;
                offset += ((x & 63) >> 3) << 5;
                offset += (x & 7) << 2;
            }
        } else {
            nmicroblkpl = ((rrb->pitch + 31) & ~31) >> 5;
            offset += (y * nmicroblkpl) << 5;
            offset += (x >> 3) << 5;
            offset += (x & 7) << 2;
        }
    }
    return &ptr[offset];
}

GLubyte *radeon_ptr16(const struct radeon_renderbuffer * rrb,
		      GLint x, GLint y)
{
    GLubyte *ptr = rrb->bo->ptr;
    uint32_t mask = RADEON_BO_FLAGS_MACRO_TILE | RADEON_BO_FLAGS_MICRO_TILE;
    GLint offset;
    GLint nmacroblkpl;
    GLint nmicroblkpl;

    if (rrb->has_surface || !(rrb->bo->flags & mask)) {
        offset = x * rrb->cpp + y * rrb->pitch;
    } else {
        offset = 0;
        if (rrb->bo->flags & RADEON_BO_FLAGS_MACRO_TILE) {
            if (rrb->bo->flags & RADEON_BO_FLAGS_MICRO_TILE) {
                nmacroblkpl = rrb->pitch >> 6;
                offset += ((y >> 4) * nmacroblkpl) << 11;
                offset += ((y & 15) >> 1) << 8;
                offset += (y & 1) << 4;
                offset += (x >> 6) << 11;
                offset += ((x & 63) >> 3) << 5;
                offset += (x & 7) << 1;
            } else {
                nmacroblkpl = rrb->pitch >> 7;
                offset += ((y >> 3) * nmacroblkpl) << 11;
                offset += (y & 7) << 8;
                offset += (x >> 7) << 11;
                offset += ((x & 127) >> 4) << 5;
                offset += (x & 15) << 2;
            }
        } else {
            nmicroblkpl = ((rrb->pitch + 31) & ~31) >> 5;
            offset += (y * nmicroblkpl) << 5;
            offset += (x >> 4) << 5;
            offset += (x & 15) << 2;
        }
    }
    return &ptr[offset];
}

GLubyte *radeon_ptr(const struct radeon_renderbuffer * rrb,
		    GLint x, GLint y)
{
    GLubyte *ptr = rrb->bo->ptr;
    uint32_t mask = RADEON_BO_FLAGS_MACRO_TILE | RADEON_BO_FLAGS_MICRO_TILE;
    GLint offset;
    GLint microblkxs;
    GLint macroblkxs;
    GLint nmacroblkpl;
    GLint nmicroblkpl;

    if (rrb->has_surface || !(rrb->bo->flags & mask)) {
        offset = x * rrb->cpp + y * rrb->pitch;
    } else {
        offset = 0;
        if (rrb->bo->flags & RADEON_BO_FLAGS_MACRO_TILE) {
            if (rrb->bo->flags & RADEON_BO_FLAGS_MICRO_TILE) {
                microblkxs = 16 / rrb->cpp;
                macroblkxs = 128 / rrb->cpp;
                nmacroblkpl = rrb->pitch / macroblkxs;
                offset += ((y >> 4) * nmacroblkpl) << 11;
                offset += ((y & 15) >> 1) << 8;
                offset += (y & 1) << 4;
                offset += (x / macroblkxs) << 11;
                offset += ((x & (macroblkxs - 1)) / microblkxs) << 5;
                offset += (x & (microblkxs - 1)) * rrb->cpp;
            } else {
                microblkxs = 32 / rrb->cpp;
                macroblkxs = 256 / rrb->cpp;
                nmacroblkpl = rrb->pitch / macroblkxs;
                offset += ((y >> 3) * nmacroblkpl) << 11;
                offset += (y & 7) << 8;
                offset += (x / macroblkxs) << 11;
                offset += ((x & (macroblkxs - 1)) / microblkxs) << 5;
                offset += (x & (microblkxs - 1)) * rrb->cpp;
            }
        } else {
            microblkxs = 32 / rrb->cpp;
            nmicroblkpl = ((rrb->pitch + 31) & ~31) >> 5;
            offset += (y * nmicroblkpl) << 5;
            offset += (x / microblkxs) << 5;
            offset += (x & (microblkxs - 1)) * rrb->cpp;
        }
    }
    return &ptr[offset];
}


static void map_buffer(struct gl_renderbuffer *rb, GLboolean write)
{
	struct radeon_renderbuffer *rrb = (void*)rb;
	int r;
	
	if (rrb->bo) {
		r = radeon_bo_map(rrb->bo, write);
		if (r) {
			fprintf(stderr, "(%s) error(%d) mapping buffer.\n",
				__FUNCTION__, r);
		}
	}
}

static void unmap_buffer(struct gl_renderbuffer *rb)
{
	struct radeon_renderbuffer *rrb = (void*)rb;

	if (rrb->bo) {
		radeon_bo_unmap(rrb->bo);
	}
}

void radeonSpanRenderStart(GLcontext * ctx)
{
	radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
	int i;

	rmesa->vtbl.flush_vertices(rmesa);

	for (i = 0; i < ctx->Const.MaxTextureImageUnits; i++) {
		if (ctx->Texture.Unit[i]._ReallyEnabled)
			ctx->Driver.MapTexture(ctx, ctx->Texture.Unit[i]._Current);
	}

	/* color draw buffers */
	for (i = 0; i < ctx->DrawBuffer->_NumColorDrawBuffers; i++) {
		map_buffer(ctx->DrawBuffer->_ColorDrawBuffers[i], GL_TRUE);
	}

	map_buffer(ctx->ReadBuffer->_ColorReadBuffer, GL_FALSE);

	if (ctx->DrawBuffer->_DepthBuffer) {
		map_buffer(ctx->DrawBuffer->_DepthBuffer->Wrapped, GL_TRUE);
	}
	if (ctx->DrawBuffer->_StencilBuffer)
		map_buffer(ctx->DrawBuffer->_StencilBuffer->Wrapped, GL_TRUE);

	/* The locking and wait for idle should really only be needed in classic mode.
	 * In a future memory manager based implementation, this should become
	 * unnecessary due to the fact that mapping our buffers, textures, etc.
	 * should implicitly wait for any previous rendering commands that must
	 * be waited on. */
	LOCK_HARDWARE(rmesa);
	radeonWaitForIdleLocked(rmesa);
}

void radeonSpanRenderFinish(GLcontext * ctx)
{
	radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
	int i;
	_swrast_flush(ctx);
	UNLOCK_HARDWARE(rmesa);

	for (i = 0; i < ctx->Const.MaxTextureImageUnits; i++) {
		if (ctx->Texture.Unit[i]._ReallyEnabled)
			ctx->Driver.UnmapTexture(ctx, ctx->Texture.Unit[i]._Current);
	}

	/* color draw buffers */
	for (i = 0; i < ctx->DrawBuffer->_NumColorDrawBuffers; i++)
		unmap_buffer(ctx->DrawBuffer->_ColorDrawBuffers[i]);

	unmap_buffer(ctx->ReadBuffer->_ColorReadBuffer);

	if (ctx->DrawBuffer->_DepthBuffer)
		unmap_buffer(ctx->DrawBuffer->_DepthBuffer->Wrapped);
	if (ctx->DrawBuffer->_StencilBuffer)
		unmap_buffer(ctx->DrawBuffer->_StencilBuffer->Wrapped);
}


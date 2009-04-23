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

/*
   - Scissor implementation
   - buffer swap/copy ioctls
   - finish/flush
   - state emission
   - cmdbuffer management
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
#include "main/simple_list.h"
#include "main/renderbuffer.h"
#include "swrast/swrast.h"
#include "vbo/vbo.h"
#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"
#include "swrast_setup/swrast_setup.h"

#include "main/blend.h"
#include "main/bufferobj.h"
#include "main/buffers.h"
#include "main/depth.h"
#include "main/shaders.h"
#include "main/texstate.h"
#include "main/varray.h"
#include "glapi/dispatch.h"
#include "swrast/swrast.h"
#include "main/stencil.h"
#include "main/matrix.h"
#include "main/attrib.h"
#include "main/enable.h"
#include "main/viewport.h"

#include "dri_util.h"
#include "vblank.h"

#include "radeon_common.h"
#include "radeon_bocs_wrapper.h"
#include "radeon_lock.h"
#include "radeon_drm.h"
#include "radeon_mipmap_tree.h"

#define DEBUG_CMDBUF         0

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

void radeon_get_cliprects(radeonContextPtr radeon,
			  struct drm_clip_rect **cliprects,
			  unsigned int *num_cliprects,
			  int *x_off, int *y_off)
{
	__DRIdrawablePrivate *dPriv = radeon->dri.drawable;
	struct radeon_framebuffer *rfb = dPriv->driverPrivate;

	if (radeon->constant_cliprect) {
		radeon->fboRect.x1 = 0;
		radeon->fboRect.y1 = 0;
		radeon->fboRect.x2 = radeon->glCtx->DrawBuffer->Width;
		radeon->fboRect.y2 = radeon->glCtx->DrawBuffer->Height;

		*cliprects = &radeon->fboRect;
		*num_cliprects = 1;
		*x_off = 0;
		*y_off = 0;
	} else if (radeon->front_cliprects ||
		   rfb->pf_active || dPriv->numBackClipRects == 0) {
		*cliprects = dPriv->pClipRects;
		*num_cliprects = dPriv->numClipRects;
		*x_off = dPriv->x;
		*y_off = dPriv->y;
	} else {
		*num_cliprects = dPriv->numBackClipRects;
		*cliprects = dPriv->pBackClipRects;
		*x_off = dPriv->backX;
		*y_off = dPriv->backY;
	}
}

/**
 * Update cliprects and scissors.
 */
void radeonSetCliprects(radeonContextPtr radeon)
{
	__DRIdrawablePrivate *const drawable = radeon->dri.drawable;
	__DRIdrawablePrivate *const readable = radeon->dri.readable;
	struct radeon_framebuffer *const draw_rfb = drawable->driverPrivate;
	struct radeon_framebuffer *const read_rfb = readable->driverPrivate;
	int x_off, y_off;

	radeon_get_cliprects(radeon, &radeon->pClipRects,
			     &radeon->numClipRects, &x_off, &y_off);
	
	if ((draw_rfb->base.Width != drawable->w) ||
	    (draw_rfb->base.Height != drawable->h)) {
		_mesa_resize_framebuffer(radeon->glCtx, &draw_rfb->base,
					 drawable->w, drawable->h);
		draw_rfb->base.Initialized = GL_TRUE;
	}

	if (drawable != readable) {
		if ((read_rfb->base.Width != readable->w) ||
		    (read_rfb->base.Height != readable->h)) {
			_mesa_resize_framebuffer(radeon->glCtx, &read_rfb->base,
						 readable->w, readable->h);
			read_rfb->base.Initialized = GL_TRUE;
		}
	}

	if (radeon->state.scissor.enabled)
		radeonRecalcScissorRects(radeon);

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

/* =============================================================
 * Scissoring
 */

void radeonScissor(GLcontext* ctx, GLint x, GLint y, GLsizei w, GLsizei h)
{
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);
	if (ctx->Scissor.Enabled) {
		/* We don't pipeline cliprect changes */
		radeon_firevertices(radeon);
		radeonUpdateScissor(ctx);
	}
}


/* ================================================================
 * SwapBuffers with client-side throttling
 */

static uint32_t radeonGetLastFrame(radeonContextPtr radeon)
{
	drm_radeon_getparam_t gp;
	int ret;
	uint32_t frame = 0;

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
	if (!radeon->radeonScreen->driScreen->dri2.enabled) {
        LOCK_HARDWARE(radeon);
	    radeonWaitForIdleLocked(radeon);
	    UNLOCK_HARDWARE(radeon);
    }
}

static void radeon_flip_renderbuffers(struct radeon_framebuffer *rfb)
{
	int current_page = rfb->pf_current_page;
	int next_page = (current_page + 1) % rfb->pf_num_pages;
	struct gl_renderbuffer *tmp_rb;

	/* Exchange renderbuffers if necessary but make sure their
	 * reference counts are preserved.
	 */
	if (rfb->color_rb[current_page] &&
	    rfb->base.Attachment[BUFFER_FRONT_LEFT].Renderbuffer !=
	    &rfb->color_rb[current_page]->base) {
		tmp_rb = NULL;
		_mesa_reference_renderbuffer(&tmp_rb,
					     rfb->base.Attachment[BUFFER_FRONT_LEFT].Renderbuffer);
		tmp_rb = &rfb->color_rb[current_page]->base;
		_mesa_reference_renderbuffer(&rfb->base.Attachment[BUFFER_FRONT_LEFT].Renderbuffer, tmp_rb);
		_mesa_reference_renderbuffer(&tmp_rb, NULL);
	}

	if (rfb->color_rb[next_page] &&
	    rfb->base.Attachment[BUFFER_BACK_LEFT].Renderbuffer !=
	    &rfb->color_rb[next_page]->base) {
		tmp_rb = NULL;
		_mesa_reference_renderbuffer(&tmp_rb,
					     rfb->base.Attachment[BUFFER_BACK_LEFT].Renderbuffer);
		tmp_rb = &rfb->color_rb[next_page]->base;
		_mesa_reference_renderbuffer(&rfb->base.Attachment[BUFFER_BACK_LEFT].Renderbuffer, tmp_rb);
		_mesa_reference_renderbuffer(&tmp_rb, NULL);
	}
}

/* Copy the back color buffer to the front color buffer.
 */
void radeonCopyBuffer( __DRIdrawablePrivate *dPriv,
		       const drm_clip_rect_t	  *rect)
{
	radeonContextPtr rmesa;
	struct radeon_framebuffer *rfb;
	GLint nbox, i, ret;
   
	assert(dPriv);
	assert(dPriv->driContextPriv);
	assert(dPriv->driContextPriv->driverPrivate);
   
	rmesa = (radeonContextPtr) dPriv->driContextPriv->driverPrivate;

	LOCK_HARDWARE(rmesa);

	rfb = dPriv->driverPrivate;

	if ( RADEON_DEBUG & DEBUG_IOCTL ) {
		fprintf( stderr, "\n%s( %p )\n\n", __FUNCTION__, (void *) rmesa->glCtx );
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
}

static int radeonScheduleSwap(__DRIdrawablePrivate *dPriv, GLboolean *missed_target)
{
	radeonContextPtr rmesa;

	rmesa = (radeonContextPtr) dPriv->driContextPriv->driverPrivate;
	radeon_firevertices(rmesa);

	LOCK_HARDWARE( rmesa );

	if (!dPriv->numClipRects) {
		UNLOCK_HARDWARE(rmesa);
		usleep(10000);	/* throttle invisible client 10ms */
		return 0;
	}

	radeonWaitForFrameCompletion(rmesa);

	UNLOCK_HARDWARE(rmesa);
	driWaitForVBlank(dPriv, missed_target);
	
	return 0;
}

static GLboolean radeonPageFlip( __DRIdrawablePrivate *dPriv )
{
	radeonContextPtr radeon;
	GLint ret;
	__DRIscreenPrivate *psp;
	struct radeon_renderbuffer *rrb;
	struct radeon_framebuffer *rfb;

	assert(dPriv);
	assert(dPriv->driContextPriv);
	assert(dPriv->driContextPriv->driverPrivate);

	radeon = (radeonContextPtr) dPriv->driContextPriv->driverPrivate;
	rfb = dPriv->driverPrivate;
	rrb = (void *)rfb->base.Attachment[BUFFER_FRONT_LEFT].Renderbuffer;

	psp = dPriv->driScreenPriv;

	LOCK_HARDWARE(radeon);

	if ( RADEON_DEBUG & DEBUG_IOCTL ) {
		fprintf(stderr, "%s: pfCurrentPage: %d %d\n", __FUNCTION__,
			radeon->sarea->pfCurrentPage, radeon->sarea->pfState);
	}
	drm_clip_rect_t *box = dPriv->pClipRects;
	drm_clip_rect_t *b = radeon->sarea->boxes;
	b[0] = box[0];
	radeon->sarea->nbox = 1;

	ret = drmCommandNone( radeon->dri.fd, DRM_RADEON_FLIP );
	
	UNLOCK_HARDWARE(radeon);

	if ( ret ) {
		fprintf( stderr, "DRM_RADEON_FLIP: return = %d\n", ret );
		return GL_FALSE;
	}

	if (!rfb->pf_active)
		return GL_FALSE;

	rfb->pf_current_page = radeon->sarea->pfCurrentPage;
	radeon_flip_renderbuffers(rfb);
	radeon_draw_buffer(radeon->glCtx, &rfb->base);

	return GL_TRUE;
}


/**
 * Swap front and back buffer.
 */
void radeonSwapBuffers(__DRIdrawablePrivate * dPriv)
{
	int64_t ust;
	__DRIscreenPrivate *psp;

	if (dPriv->driContextPriv && dPriv->driContextPriv->driverPrivate) {
		radeonContextPtr radeon;
		GLcontext *ctx;

		radeon = (radeonContextPtr) dPriv->driContextPriv->driverPrivate;
		ctx = radeon->glCtx;

		if (ctx->Visual.doubleBufferMode) {
			GLboolean missed_target;
			struct radeon_framebuffer *rfb = dPriv->driverPrivate;
			_mesa_notifySwapBuffers(ctx);/* flush pending rendering comands */

			radeonScheduleSwap(dPriv, &missed_target);

			if (rfb->pf_active) {
				radeonPageFlip(dPriv);
			} else {
				radeonCopyBuffer(dPriv, NULL);
			}

			psp = dPriv->driScreenPriv;

			rfb->swap_count++;
			(*psp->systemTime->getUST)( & ust );
			if ( missed_target ) {
				rfb->swap_missed_count++;
				rfb->swap_missed_ust = ust - rfb->swap_ust;
			}

			rfb->swap_ust = ust;
			radeon->hw.all_dirty = GL_TRUE;
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

void radeon_draw_buffer(GLcontext *ctx, struct gl_framebuffer *fb)
{
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);
	struct radeon_renderbuffer *rrbDepth = NULL, *rrbStencil = NULL,
		*rrbColor = NULL;
	uint32_t offset = 0;
       

	if (!fb) {
		/* this can happen during the initial context initialization */
		return;
	}

	/* radeons only handle 1 color draw so far */
	if (fb->_NumColorDrawBuffers != 1) {
		radeon->vtbl.fallback(ctx, RADEON_FALLBACK_DRAW_BUFFER, GL_TRUE);
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
		;/* do something depthy/stencily TODO */


		/* none */
	if (fb->Name == 0) {
		if (fb->_ColorDrawBufferIndexes[0] == BUFFER_FRONT_LEFT) {
			rrbColor = radeon_renderbuffer(fb->Attachment[BUFFER_FRONT_LEFT].Renderbuffer);
			radeon->front_cliprects = GL_TRUE;
		} else {
			rrbColor = radeon_renderbuffer(fb->Attachment[BUFFER_BACK_LEFT].Renderbuffer);
			radeon->front_cliprects = GL_FALSE;
		}
	} else {
		/* user FBO in theory */
		struct radeon_renderbuffer *rrb;
		rrb = radeon_renderbuffer(fb->_ColorDrawBuffers[0]);
		if (rrb) {
			offset = rrb->draw_offset;
			rrbColor = rrb;
		}
		radeon->constant_cliprect = GL_TRUE;
	}

	if (rrbColor == NULL)
		radeon->vtbl.fallback(ctx, RADEON_FALLBACK_DRAW_BUFFER, GL_TRUE);
	else
		radeon->vtbl.fallback(ctx, RADEON_FALLBACK_DRAW_BUFFER, GL_FALSE);


	if (fb->_DepthBuffer && fb->_DepthBuffer->Wrapped) {
		rrbDepth = radeon_renderbuffer(fb->_DepthBuffer->Wrapped);
		if (rrbDepth && rrbDepth->bo) {
			radeon->vtbl.fallback(ctx, RADEON_FALLBACK_DEPTH_BUFFER, GL_FALSE);
		} else {
			radeon->vtbl.fallback(ctx, RADEON_FALLBACK_DEPTH_BUFFER, GL_TRUE);
		}
	} else {
		radeon->vtbl.fallback(ctx, RADEON_FALLBACK_DEPTH_BUFFER, GL_FALSE);
		rrbDepth = NULL;
	}

	if (fb->_StencilBuffer && fb->_StencilBuffer->Wrapped) {
		rrbStencil = radeon_renderbuffer(fb->_DepthBuffer->Wrapped);
		if (rrbStencil && rrbStencil->bo) {
			radeon->vtbl.fallback(ctx, RADEON_FALLBACK_STENCIL_BUFFER, GL_FALSE);
			/* need to re-compute stencil hw state */
			if (!rrbDepth)
				rrbDepth = rrbStencil;
		} else {
			radeon->vtbl.fallback(ctx, RADEON_FALLBACK_STENCIL_BUFFER, GL_TRUE);
		}
	} else {
		radeon->vtbl.fallback(ctx, RADEON_FALLBACK_STENCIL_BUFFER, GL_FALSE);
		if (ctx->Driver.Enable != NULL)
			ctx->Driver.Enable(ctx, GL_STENCIL_TEST, ctx->Stencil.Enabled);
		else
			ctx->NewState |= _NEW_STENCIL;
	}

	/* Update culling direction which changes depending on the
	 * orientation of the buffer:
	 */
	if (ctx->Driver.FrontFace)
		ctx->Driver.FrontFace(ctx, ctx->Polygon.FrontFace);
	else
		ctx->NewState |= _NEW_POLYGON;
	
	/*
	 * Update depth test state
	 */
	if (ctx->Driver.Enable) {
		ctx->Driver.Enable(ctx, GL_DEPTH_TEST,
				   (ctx->Depth.Test && fb->Visual.depthBits > 0));
		ctx->Driver.Enable(ctx, GL_STENCIL_TEST,
				   (ctx->Stencil._Enabled && fb->Visual.stencilBits > 0));
	} else {
		ctx->NewState |= (_NEW_DEPTH | _NEW_STENCIL);
	}
	
	radeon->state.depth.rrb = rrbDepth;
	radeon->state.color.rrb = rrbColor;
	radeon->state.color.draw_offset = offset;

#if 0
	/* update viewport since it depends on window size */
	if (ctx->Driver.Viewport) {
		ctx->Driver.Viewport(ctx, ctx->Viewport.X, ctx->Viewport.Y,
				     ctx->Viewport.Width, ctx->Viewport.Height);
	} else {
	
	}
#endif
	ctx->NewState |= _NEW_VIEWPORT;

	/* Set state we know depends on drawable parameters:
	 */
	if (ctx->Driver.Scissor)
		ctx->Driver.Scissor(ctx, ctx->Scissor.X, ctx->Scissor.Y,
				    ctx->Scissor.Width, ctx->Scissor.Height);
	radeon->NewGLState |= _NEW_SCISSOR;

	if (ctx->Driver.DepthRange)
		ctx->Driver.DepthRange(ctx,
				       ctx->Viewport.Near,
				       ctx->Viewport.Far);

	/* Update culling direction which changes depending on the
	 * orientation of the buffer:
	 */
	if (ctx->Driver.FrontFace)
		ctx->Driver.FrontFace(ctx, ctx->Polygon.FrontFace);
	else
		ctx->NewState |= _NEW_POLYGON;
}

/**
 * Called via glDrawBuffer.
 */
void radeonDrawBuffer( GLcontext *ctx, GLenum mode )
{
	if (RADEON_DEBUG & DEBUG_DRI)
		fprintf(stderr, "%s %s\n", __FUNCTION__,
			_mesa_lookup_enum_by_nr( mode ));
	
	radeon_draw_buffer(ctx, ctx->DrawBuffer);
}

void radeonReadBuffer( GLcontext *ctx, GLenum mode )
{
	/* nothing, until we implement h/w glRead/CopyPixels or CopyTexImage */
	if (ctx->ReadBuffer == ctx->DrawBuffer) {
		/* This will update FBO completeness status.
		 * A framebuffer will be incomplete if the GL_READ_BUFFER setting
		 * refers to a missing renderbuffer.  Calling glReadBuffer can set
		 * that straight and can make the drawing buffer complete.
		 */
		radeon_draw_buffer(ctx, ctx->DrawBuffer);
	}
}


/* Turn on/off page flipping according to the flags in the sarea:
 */
void radeonUpdatePageFlipping(radeonContextPtr radeon)
{
	struct radeon_framebuffer *rfb = radeon->dri.drawable->driverPrivate;

	rfb->pf_active = radeon->sarea->pfState;
	rfb->pf_current_page = radeon->sarea->pfCurrentPage;
	rfb->pf_num_pages = 2;
	radeon_flip_renderbuffers(rfb);
	radeon_draw_buffer(radeon->glCtx, radeon->glCtx->DrawBuffer);
}

void radeon_window_moved(radeonContextPtr radeon)
{
	if (!radeon->radeonScreen->driScreen->dri2.enabled) {
		radeonUpdatePageFlipping(radeon);
	}
	radeonSetCliprects(radeon);
}

void radeon_viewport(GLcontext *ctx, GLint x, GLint y, GLsizei width, GLsizei height)
{
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);
	__DRIcontext *driContext = radeon->dri.context;
	void (*old_viewport)(GLcontext *ctx, GLint x, GLint y,
			     GLsizei w, GLsizei h);

	if (!driContext->driScreenPriv->dri2.enabled)
		return;

	radeon_update_renderbuffers(driContext, driContext->driDrawablePriv);
	if (driContext->driDrawablePriv != driContext->driReadablePriv)
		radeon_update_renderbuffers(driContext, driContext->driReadablePriv);

	old_viewport = ctx->Driver.Viewport;
	ctx->Driver.Viewport = NULL;
	radeon->dri.drawable = driContext->driDrawablePriv;
	radeon_window_moved(radeon);
	radeon_draw_buffer(ctx, radeon->glCtx->DrawBuffer);
	ctx->Driver.Viewport = old_viewport;
}

static void radeon_print_state_atom(radeonContextPtr radeon, struct radeon_state_atom *state)
{
	int i, j, reg;
	int dwords = (*state->check) (radeon->glCtx, state);
	drm_r300_cmd_header_t cmd;

	fprintf(stderr, "  emit %s %d/%d\n", state->name, dwords, state->cmd_size);

	if (RADEON_DEBUG & DEBUG_VERBOSE) {
		for (i = 0; i < dwords;) {
			cmd = *((drm_r300_cmd_header_t *) &state->cmd[i]);
			reg = (cmd.packet0.reghi << 8) | cmd.packet0.reglo;
			fprintf(stderr, "      %s[%d]: cmdpacket0 (first reg=0x%04x, count=%d)\n",
					state->name, i, reg, cmd.packet0.count);
			++i;
			for (j = 0; j < cmd.packet0.count && i < dwords; j++) {
				fprintf(stderr, "      %s[%d]: 0x%04x = %08x\n",
						state->name, i, reg, state->cmd[i]);
				reg += 4;
				++i;
			}
		}
	}
}

static void radeon_print_state_atom_kmm(radeonContextPtr radeon, struct radeon_state_atom *state)
{
	int i, j, reg, count;
	int dwords = (*state->check) (radeon->glCtx, state);
	uint32_t packet0;

	fprintf(stderr, "  emit %s %d/%d\n", state->name, dwords, state->cmd_size);

	if (RADEON_DEBUG & DEBUG_VERBOSE) {
		for (i = 0; i < dwords;) {
			packet0 = state->cmd[i];
			reg = (packet0 & 0x1FFF) << 2;
			count = ((packet0 & 0x3FFF0000) >> 16) + 1;
			fprintf(stderr, "      %s[%d]: cmdpacket0 (first reg=0x%04x, count=%d)\n",
					state->name, i, reg, count);
			++i;
			for (j = 0; j < count && i < dwords; j++) {
				fprintf(stderr, "      %s[%d]: 0x%04x = %08x\n",
						state->name, i, reg, state->cmd[i]);
				reg += 4;
				++i;
			}
		}
	}
}

static INLINE void radeonEmitAtoms(radeonContextPtr radeon, GLboolean dirty)
{
	BATCH_LOCALS(radeon);
	struct radeon_state_atom *atom;
	int dwords;

	if (radeon->vtbl.pre_emit_atoms)
		radeon->vtbl.pre_emit_atoms(radeon);

	/* Emit actual atoms */
	foreach(atom, &radeon->hw.atomlist) {
		if ((atom->dirty || radeon->hw.all_dirty) == dirty) {
			dwords = (*atom->check) (radeon->glCtx, atom);
			if (dwords) {
				if (DEBUG_CMDBUF && RADEON_DEBUG & DEBUG_STATE) {
					if (radeon->radeonScreen->kernel_mm)
						radeon_print_state_atom_kmm(radeon, atom);
					else
						radeon_print_state_atom(radeon, atom);
				}
				if (atom->emit) {
					(*atom->emit)(radeon->glCtx, atom);
				} else {
					BEGIN_BATCH_NO_AUTOSTATE(dwords);
					OUT_BATCH_TABLE(atom->cmd, dwords);
					END_BATCH();
				}
				atom->dirty = GL_FALSE;
			} else {
				if (DEBUG_CMDBUF && RADEON_DEBUG & DEBUG_STATE) {
					fprintf(stderr, "  skip state %s\n",
						atom->name);
				}
			}
		}
	}
   
	COMMIT_BATCH();
}

GLboolean radeon_revalidate_bos(GLcontext *ctx)
{
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);
	int flushed = 0;
	int ret;
again:
	ret = radeon_cs_space_check(radeon->cmdbuf.cs, radeon->state.bos, radeon->state.validated_bo_count);
	if (ret == RADEON_CS_SPACE_OP_TO_BIG)
		return GL_FALSE;
	if (ret == RADEON_CS_SPACE_FLUSH) {
		radeonFlush(ctx);
		if (flushed)
			return GL_FALSE;
		flushed = 1;
		goto again;
	}
	return GL_TRUE;
}

void radeon_validate_reset_bos(radeonContextPtr radeon)
{
	int i;

	for (i = 0; i < radeon->state.validated_bo_count; i++) {
		radeon_bo_unref(radeon->state.bos[i].bo);
		radeon->state.bos[i].bo = NULL;
		radeon->state.bos[i].read_domains = 0;
		radeon->state.bos[i].write_domain = 0;
		radeon->state.bos[i].new_accounted = 0;
	}
	radeon->state.validated_bo_count = 0;
}

void radeon_validate_bo(radeonContextPtr radeon, struct radeon_bo *bo, uint32_t read_domains, uint32_t write_domain)
{
	radeon_bo_ref(bo);
	radeon->state.bos[radeon->state.validated_bo_count].bo = bo;
	radeon->state.bos[radeon->state.validated_bo_count].read_domains = read_domains;
	radeon->state.bos[radeon->state.validated_bo_count].write_domain = write_domain;
	radeon->state.bos[radeon->state.validated_bo_count].new_accounted = 0;
	radeon->state.validated_bo_count++;

	assert(radeon->state.validated_bo_count < RADEON_MAX_BOS);
}

void radeonEmitState(radeonContextPtr radeon)
{
	if (RADEON_DEBUG & (DEBUG_STATE|DEBUG_PRIMS))
		fprintf(stderr, "%s\n", __FUNCTION__);

	if (radeon->vtbl.pre_emit_state)
		radeon->vtbl.pre_emit_state(radeon);

	/* this code used to return here but now it emits zbs */
	if (radeon->cmdbuf.cs->cdw && !radeon->hw.is_dirty && !radeon->hw.all_dirty)
		return;

	/* To avoid going across the entire set of states multiple times, just check
	 * for enough space for the case of emitting all state, and inline the
	 * radeonAllocCmdBuf code here without all the checks.
	 */
	rcommonEnsureCmdBufSpace(radeon, radeon->hw.max_state_size, __FUNCTION__);

	if (!radeon->cmdbuf.cs->cdw) {
		if (RADEON_DEBUG & DEBUG_STATE)
			fprintf(stderr, "Begin reemit state\n");
		
		radeonEmitAtoms(radeon, GL_FALSE);
	}

	if (RADEON_DEBUG & DEBUG_STATE)
		fprintf(stderr, "Begin dirty state\n");

	radeonEmitAtoms(radeon, GL_TRUE);
	radeon->hw.is_dirty = GL_FALSE;
	radeon->hw.all_dirty = GL_FALSE;

}


void radeonFlush(GLcontext *ctx)
{
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);
	if (RADEON_DEBUG & DEBUG_IOCTL)
		fprintf(stderr, "%s %d\n", __FUNCTION__, radeon->cmdbuf.cs->cdw);

	/* okay if we have no cmds in the buffer &&
	   we have no DMA flush &&
	   we have no DMA buffer allocated.
	   then no point flushing anything at all.
	*/
	if (!radeon->dma.flush && !radeon->cmdbuf.cs->cdw && !radeon->dma.current)
		return;

	if (radeon->dma.flush)
		radeon->dma.flush( ctx );

	radeonEmitState(radeon);
   
	if (radeon->cmdbuf.cs->cdw)
		rcommonFlushCmdBuf(radeon, __FUNCTION__);
}

/* Make sure all commands have been sent to the hardware and have
 * completed processing.
 */
void radeonFinish(GLcontext * ctx)
{
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);
	struct gl_framebuffer *fb = ctx->DrawBuffer;
	int i;

	radeonFlush(ctx);

	if (radeon->radeonScreen->kernel_mm) {
		for (i = 0; i < fb->_NumColorDrawBuffers; i++) {
			struct radeon_renderbuffer *rrb;
			rrb = radeon_renderbuffer(fb->_ColorDrawBuffers[i]);
			if (rrb && rrb->bo)
				radeon_bo_wait(rrb->bo);
		}
		{
			struct radeon_renderbuffer *rrb;
			rrb = radeon_get_depthbuffer(radeon);
			if (rrb && rrb->bo)
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

	if (RADEON_DEBUG & DEBUG_IOCTL) {
		fprintf(stderr, "%s from %s - %i cliprects\n",
			__FUNCTION__, caller, rmesa->numClipRects);
	}

	if (rmesa->cmdbuf.cs->cdw) {
		ret = radeon_cs_emit(rmesa->cmdbuf.cs);
		rmesa->hw.all_dirty = GL_TRUE;
	}
	radeon_cs_erase(rmesa->cmdbuf.cs);
	rmesa->cmdbuf.flushing = 0;

	if (radeon_revalidate_bos(rmesa->glCtx) == GL_FALSE) {
		fprintf(stderr,"failed to revalidate buffers\n");
	}

	return ret;
}

int rcommonFlushCmdBuf(radeonContextPtr rmesa, const char *caller)
{
	int ret;

	radeonReleaseDmaRegion(rmesa);
	
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

void rcommonInitCmdBuf(radeonContextPtr rmesa)
{
	GLuint size;
	/* Initialize command buffer */
	size = 256 * driQueryOptioni(&rmesa->optionCache,
				     "command_buffer_size");
	if (size < 2 * rmesa->hw.max_state_size) {
		size = 2 * rmesa->hw.max_state_size + 65535;
	}
	if (size > 64 * 256)
		size = 64 * 256;

	if (RADEON_DEBUG & (DEBUG_IOCTL | DEBUG_DMA)) {
		fprintf(stderr, "sizeof(drm_r300_cmd_header_t)=%zd\n",
			sizeof(drm_r300_cmd_header_t));
		fprintf(stderr, "sizeof(drm_radeon_cmd_buffer_t)=%zd\n",
			sizeof(drm_radeon_cmd_buffer_t));
		fprintf(stderr,
			"Allocating %d bytes command buffer (max state is %d bytes)\n",
			size * 4, rmesa->hw.max_state_size * 4);
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
	
	if (!rmesa->radeonScreen->kernel_mm) {
		radeon_cs_set_limit(rmesa->cmdbuf.cs, RADEON_GEM_DOMAIN_VRAM, rmesa->radeonScreen->texSize[0]);
		radeon_cs_set_limit(rmesa->cmdbuf.cs, RADEON_GEM_DOMAIN_GTT, rmesa->radeonScreen->gartTextures.size);
	} else {
		struct drm_radeon_gem_info mminfo;

		if (!drmCommandWriteRead(rmesa->dri.fd, DRM_RADEON_GEM_INFO, &mminfo, sizeof(mminfo)))
		{
			radeon_cs_set_limit(rmesa->cmdbuf.cs, RADEON_GEM_DOMAIN_VRAM, mminfo.vram_visible);
			radeon_cs_set_limit(rmesa->cmdbuf.cs, RADEON_GEM_DOMAIN_GTT, mminfo.gart_size);
		}
	}

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
		radeonEmitState(rmesa);
	}
	radeon_cs_begin(rmesa->cmdbuf.cs, n, file, function, line);

        if (DEBUG_CMDBUF && RADEON_DEBUG & DEBUG_IOCTL)
                fprintf(stderr, "BEGIN_BATCH(%d) at %d, from %s:%i\n",
                        n, rmesa->cmdbuf.cs->cdw, function, line);

}



static void
radeon_meta_set_passthrough_transform(radeonContextPtr radeon)
{
   GLcontext *ctx = radeon->glCtx;

   radeon->meta.saved_vp_x = ctx->Viewport.X;
   radeon->meta.saved_vp_y = ctx->Viewport.Y;
   radeon->meta.saved_vp_width = ctx->Viewport.Width;
   radeon->meta.saved_vp_height = ctx->Viewport.Height;
   radeon->meta.saved_matrix_mode = ctx->Transform.MatrixMode;

   _mesa_Viewport(0, 0, ctx->DrawBuffer->Width, ctx->DrawBuffer->Height);

   _mesa_MatrixMode(GL_PROJECTION);
   _mesa_PushMatrix();
   _mesa_LoadIdentity();
   _mesa_Ortho(0, ctx->DrawBuffer->Width, 0, ctx->DrawBuffer->Height, 1, -1);

   _mesa_MatrixMode(GL_MODELVIEW);
   _mesa_PushMatrix();
   _mesa_LoadIdentity();
}

static void
radeon_meta_restore_transform(radeonContextPtr radeon)
{
   _mesa_MatrixMode(GL_PROJECTION);
   _mesa_PopMatrix();
   _mesa_MatrixMode(GL_MODELVIEW);
   _mesa_PopMatrix();

   _mesa_MatrixMode(radeon->meta.saved_matrix_mode);

   _mesa_Viewport(radeon->meta.saved_vp_x, radeon->meta.saved_vp_y,
		  radeon->meta.saved_vp_width, radeon->meta.saved_vp_height);
}


/**
 * Perform glClear where mask contains only color, depth, and/or stencil.
 *
 * The implementation is based on calling into Mesa to set GL state and
 * performing normal triangle rendering.  The intent of this path is to
 * have as generic a path as possible, so that any driver could make use of
 * it.
 */


void radeon_clear_tris(GLcontext *ctx, GLbitfield mask)
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   GLfloat vertices[4][3];
   GLfloat color[4][4];
   GLfloat dst_z;
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   int i;
   GLboolean saved_fp_enable = GL_FALSE, saved_vp_enable = GL_FALSE;
   GLboolean saved_shader_program = 0;
   unsigned int saved_active_texture;

   assert((mask & ~(TRI_CLEAR_COLOR_BITS | BUFFER_BIT_DEPTH |
		    BUFFER_BIT_STENCIL)) == 0);   

   _mesa_PushAttrib(GL_COLOR_BUFFER_BIT |
		    GL_CURRENT_BIT |
		    GL_DEPTH_BUFFER_BIT |
		    GL_ENABLE_BIT |
		    GL_POLYGON_BIT |
		    GL_STENCIL_BUFFER_BIT |
		    GL_TRANSFORM_BIT |
		    GL_CURRENT_BIT);
   _mesa_PushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
   saved_active_texture = ctx->Texture.CurrentUnit;
  
  /* Disable existing GL state we don't want to apply to a clear. */
   _mesa_Disable(GL_ALPHA_TEST);
   _mesa_Disable(GL_BLEND);
   _mesa_Disable(GL_CULL_FACE);
   _mesa_Disable(GL_FOG);
   _mesa_Disable(GL_POLYGON_SMOOTH);
   _mesa_Disable(GL_POLYGON_STIPPLE);
   _mesa_Disable(GL_POLYGON_OFFSET_FILL);
   _mesa_Disable(GL_LIGHTING);
   _mesa_Disable(GL_CLIP_PLANE0);
   _mesa_Disable(GL_CLIP_PLANE1);
   _mesa_Disable(GL_CLIP_PLANE2);
   _mesa_Disable(GL_CLIP_PLANE3);
   _mesa_Disable(GL_CLIP_PLANE4);
   _mesa_Disable(GL_CLIP_PLANE5);
   _mesa_PolygonMode(GL_FRONT_AND_BACK, GL_FILL);
   if (ctx->Extensions.ARB_fragment_program && ctx->FragmentProgram.Enabled) {
      saved_fp_enable = GL_TRUE;
      _mesa_Disable(GL_FRAGMENT_PROGRAM_ARB);
   }
   if (ctx->Extensions.ARB_vertex_program && ctx->VertexProgram.Enabled) {
      saved_vp_enable = GL_TRUE;
      _mesa_Disable(GL_VERTEX_PROGRAM_ARB);
   }
   if (ctx->Extensions.ARB_shader_objects && ctx->Shader.CurrentProgram) {
      saved_shader_program = ctx->Shader.CurrentProgram->Name;
      _mesa_UseProgramObjectARB(0);
   }
   
   if (ctx->Texture._EnabledUnits != 0) {
      int i;
      
      for (i = 0; i < ctx->Const.MaxTextureUnits; i++) {
	 _mesa_ActiveTextureARB(GL_TEXTURE0 + i);
	 _mesa_Disable(GL_TEXTURE_1D);
	 _mesa_Disable(GL_TEXTURE_2D);
	 _mesa_Disable(GL_TEXTURE_3D);
	 if (ctx->Extensions.ARB_texture_cube_map)
	    _mesa_Disable(GL_TEXTURE_CUBE_MAP_ARB);
	 if (ctx->Extensions.NV_texture_rectangle)
	    _mesa_Disable(GL_TEXTURE_RECTANGLE_NV);
	 if (ctx->Extensions.MESA_texture_array) {
	    _mesa_Disable(GL_TEXTURE_1D_ARRAY_EXT);
	    _mesa_Disable(GL_TEXTURE_2D_ARRAY_EXT);
	 }
      }
   }
  
#if FEATURE_ARB_vertex_buffer_object
   _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
   _mesa_BindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
#endif

   radeon_meta_set_passthrough_transform(rmesa);
   
   for (i = 0; i < 4; i++) {
      color[i][0] = ctx->Color.ClearColor[0];
      color[i][1] = ctx->Color.ClearColor[1];
      color[i][2] = ctx->Color.ClearColor[2];
      color[i][3] = ctx->Color.ClearColor[3];
   }

   /* convert clear Z from [0,1] to NDC coord in [-1,1] */

   dst_z = -1.0 + 2.0 * ctx->Depth.Clear;
   /* Prepare the vertices, which are the same regardless of which buffer we're
    * drawing to.
    */
   vertices[0][0] = fb->_Xmin;
   vertices[0][1] = fb->_Ymin;
   vertices[0][2] = dst_z;
   vertices[1][0] = fb->_Xmax;
   vertices[1][1] = fb->_Ymin;
   vertices[1][2] = dst_z;
   vertices[2][0] = fb->_Xmax;
   vertices[2][1] = fb->_Ymax;
   vertices[2][2] = dst_z;
   vertices[3][0] = fb->_Xmin;
   vertices[3][1] = fb->_Ymax;
   vertices[3][2] = dst_z;

   _mesa_ColorPointer(4, GL_FLOAT, 4 * sizeof(GLfloat), &color);
   _mesa_VertexPointer(3, GL_FLOAT, 3 * sizeof(GLfloat), &vertices);
   _mesa_Enable(GL_COLOR_ARRAY);
   _mesa_Enable(GL_VERTEX_ARRAY);

   while (mask != 0) {
      GLuint this_mask = 0;
      GLuint color_bit;

      color_bit = _mesa_ffs(mask & TRI_CLEAR_COLOR_BITS);
      if (color_bit != 0)
	 this_mask |= (1 << (color_bit - 1));

      /* Clear depth/stencil in the same pass as color. */
      this_mask |= (mask & (BUFFER_BIT_DEPTH | BUFFER_BIT_STENCIL));

      /* Select the current color buffer and use the color write mask if
       * we have one, otherwise don't write any color channels.
       */
      if (this_mask & BUFFER_BIT_FRONT_LEFT)
	 _mesa_DrawBuffer(GL_FRONT_LEFT);
      else if (this_mask & BUFFER_BIT_BACK_LEFT)
	 _mesa_DrawBuffer(GL_BACK_LEFT);
      else if (color_bit != 0)
	 _mesa_DrawBuffer(GL_COLOR_ATTACHMENT0 +
			  (color_bit - BUFFER_COLOR0 - 1));
      else
	 _mesa_ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

      /* Control writing of the depth clear value to depth. */
      if (this_mask & BUFFER_BIT_DEPTH) {
	 _mesa_DepthFunc(GL_ALWAYS);
	 _mesa_DepthMask(GL_TRUE);
	 _mesa_Enable(GL_DEPTH_TEST);
      } else {
	 _mesa_Disable(GL_DEPTH_TEST);
	 _mesa_DepthMask(GL_FALSE);
      }

      /* Control writing of the stencil clear value to stencil. */
      if (this_mask & BUFFER_BIT_STENCIL) {
	 _mesa_Enable(GL_STENCIL_TEST);
	 _mesa_StencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
	 _mesa_StencilFuncSeparate(GL_FRONT, GL_ALWAYS, ctx->Stencil.Clear,
				   ctx->Stencil.WriteMask[0]);
      } else {
	 _mesa_Disable(GL_STENCIL_TEST);
      }

      CALL_DrawArrays(ctx->Exec, (GL_TRIANGLE_FAN, 0, 4));

      mask &= ~this_mask;
   }

   radeon_meta_restore_transform(rmesa);

   _mesa_ActiveTextureARB(GL_TEXTURE0 + saved_active_texture);
   if (saved_fp_enable)
      _mesa_Enable(GL_FRAGMENT_PROGRAM_ARB);
   if (saved_vp_enable)
      _mesa_Enable(GL_VERTEX_PROGRAM_ARB);

   if (saved_shader_program)
      _mesa_UseProgramObjectARB(saved_shader_program);

   _mesa_PopClientAttrib();
   _mesa_PopAttrib();
}

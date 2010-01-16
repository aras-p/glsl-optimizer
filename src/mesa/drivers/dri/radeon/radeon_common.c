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
#include "main/enums.h"
#include "main/framebuffer.h"
#include "main/renderbuffer.h"
#include "drivers/common/meta.h"

#include "vblank.h"

#include "radeon_common.h"
#include "radeon_bocs_wrapper.h"
#include "radeon_lock.h"
#include "radeon_drm.h"
#include "radeon_queryobj.h"

/**
 * Enable verbose debug output for emit code.
 * 0 no output
 * 1 most output
 * 2 also print state alues
 */
#define RADEON_CMDBUF         0

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

	if (radeon->vtbl.update_scissor)
	   radeon->vtbl.update_scissor(radeon->glCtx);
}

void radeon_get_cliprects(radeonContextPtr radeon,
			  struct drm_clip_rect **cliprects,
			  unsigned int *num_cliprects,
			  int *x_off, int *y_off)
{
	__DRIdrawable *dPriv = radeon_get_drawable(radeon);
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
	__DRIdrawable *const drawable = radeon_get_drawable(radeon);
	__DRIdrawable *const readable = radeon_get_readable(radeon);
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
	GLint x = ctx->Scissor.X, y = ctx->Scissor.Y;
	GLsizei w = ctx->Scissor.Width, h = ctx->Scissor.Height;
	int x1, y1, x2, y2;
	int min_x, min_y, max_x, max_y;

	if (!ctx->DrawBuffer)
	    return;
	min_x = min_y = 0;
	max_x = ctx->DrawBuffer->Width - 1;
	max_y = ctx->DrawBuffer->Height - 1;

	if ( !ctx->DrawBuffer->Name ) {
		x1 = x;
		y1 = ctx->DrawBuffer->Height - (y + h);
		x2 = x + w - 1;
		y2 = y1 + h - 1;
	} else {
		x1 = x;
		y1 = y;
		x2 = x + w - 1;
		y2 = y + h - 1;

	}
	if (!rmesa->radeonScreen->kernel_mm) {
	   /* Fix scissors for dri 1 */
	   __DRIdrawable *dPriv = radeon_get_drawable(rmesa);
	   x1 += dPriv->x;
	   x2 += dPriv->x + 1;
	   min_x += dPriv->x;
	   max_x += dPriv->x + 1;
	   y1 += dPriv->y;
	   y2 += dPriv->y + 1;
	   min_y += dPriv->y;
	   max_y += dPriv->y + 1;
	}

	rmesa->state.scissor.rect.x1 = CLAMP(x1,  min_x, max_x);
	rmesa->state.scissor.rect.y1 = CLAMP(y1,  min_y, max_y);
	rmesa->state.scissor.rect.x2 = CLAMP(x2,  min_x, max_x);
	rmesa->state.scissor.rect.y2 = CLAMP(y2,  min_y, max_y);

	radeonRecalcScissorRects( rmesa );
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
void radeonCopyBuffer( __DRIdrawable *dPriv,
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

	if ( RADEON_DEBUG & RADEON_IOCTL ) {
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

static int radeonScheduleSwap(__DRIdrawable *dPriv, GLboolean *missed_target)
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

static GLboolean radeonPageFlip( __DRIdrawable *dPriv )
{
	radeonContextPtr radeon;
	GLint ret;
	__DRIscreen *psp;
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

	if ( RADEON_DEBUG & RADEON_IOCTL ) {
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
void radeonSwapBuffers(__DRIdrawable * dPriv)
{
	int64_t ust;
	__DRIscreen *psp;

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

void radeonCopySubBuffer(__DRIdrawable * dPriv,
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

/**
 * Check if we're about to draw into the front color buffer.
 * If so, set the intel->front_buffer_dirty field to true.
 */
void
radeon_check_front_buffer_rendering(GLcontext *ctx)
{
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);
	const struct gl_framebuffer *fb = ctx->DrawBuffer;

	if (fb->Name == 0) {
		/* drawing to window system buffer */
		if (fb->_NumColorDrawBuffers > 0) {
			if (fb->_ColorDrawBufferIndexes[0] == BUFFER_FRONT_LEFT) {
				radeon->front_buffer_dirty = GL_TRUE;
			}
		}
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
			radeon->front_buffer_dirty = GL_TRUE;
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
		rrbStencil = radeon_renderbuffer(fb->_StencilBuffer->Wrapped);
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
		/* Need to update the derived ctx->Stencil._Enabled first */
		ctx->Driver.Enable(ctx, GL_STENCIL_TEST,
				   (ctx->Stencil.Enabled && fb->Visual.stencilBits > 0));
	} else {
		ctx->NewState |= (_NEW_DEPTH | _NEW_STENCIL);
	}

	_mesa_reference_renderbuffer(&radeon->state.depth.rb, &rrbDepth->base);
	_mesa_reference_renderbuffer(&radeon->state.color.rb, &rrbColor->base);
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
	radeonUpdateScissor(ctx);
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
	if (RADEON_DEBUG & RADEON_DRI)
		fprintf(stderr, "%s %s\n", __FUNCTION__,
			_mesa_lookup_enum_by_nr( mode ));

	if (ctx->DrawBuffer->Name == 0) {
		radeonContextPtr radeon = RADEON_CONTEXT(ctx);

		const GLboolean was_front_buffer_rendering =
			radeon->is_front_buffer_rendering;

		radeon->is_front_buffer_rendering = (mode == GL_FRONT_LEFT) ||
                                            (mode == GL_FRONT);

      /* If we weren't front-buffer rendering before but we are now, make sure
       * that the front-buffer has actually been allocated.
       */
		if (!was_front_buffer_rendering && radeon->is_front_buffer_rendering) {
			radeon_update_renderbuffers(radeon->dri.context,
				radeon->dri.context->driDrawablePriv, GL_FALSE);
      }
	}

	radeon_draw_buffer(ctx, ctx->DrawBuffer);
}

void radeonReadBuffer( GLcontext *ctx, GLenum mode )
{
	if ((ctx->DrawBuffer != NULL) && (ctx->DrawBuffer->Name == 0)) {
		struct radeon_context *const rmesa = RADEON_CONTEXT(ctx);
		const GLboolean was_front_buffer_reading = rmesa->is_front_buffer_reading;
		rmesa->is_front_buffer_reading = (mode == GL_FRONT_LEFT)
					|| (mode == GL_FRONT);

		if (!was_front_buffer_reading && rmesa->is_front_buffer_reading) {
			radeon_update_renderbuffers(rmesa->dri.context,
						    rmesa->dri.context->driReadablePriv, GL_FALSE);
	 	}
	}
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
	struct radeon_framebuffer *rfb = radeon_get_drawable(radeon)->driverPrivate;

	rfb->pf_active = radeon->sarea->pfState;
	rfb->pf_current_page = radeon->sarea->pfCurrentPage;
	rfb->pf_num_pages = 2;
	radeon_flip_renderbuffers(rfb);
	radeon_draw_buffer(radeon->glCtx, radeon->glCtx->DrawBuffer);
}

void radeon_window_moved(radeonContextPtr radeon)
{
	/* Cliprects has to be updated before doing anything else */
	radeonSetCliprects(radeon);
	if (!radeon->radeonScreen->driScreen->dri2.enabled) {
		radeonUpdatePageFlipping(radeon);
	}
}

void radeon_viewport(GLcontext *ctx, GLint x, GLint y, GLsizei width, GLsizei height)
{
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);
	__DRIcontext *driContext = radeon->dri.context;
	void (*old_viewport)(GLcontext *ctx, GLint x, GLint y,
			     GLsizei w, GLsizei h);

	if (!driContext->driScreenPriv->dri2.enabled)
		return;

	if (!radeon->meta.internal_viewport_call && ctx->DrawBuffer->Name == 0) {
		if (radeon->is_front_buffer_rendering) {
			ctx->Driver.Flush(ctx);
		}
		radeon_update_renderbuffers(driContext, driContext->driDrawablePriv, GL_FALSE);
		if (driContext->driDrawablePriv != driContext->driReadablePriv)
			radeon_update_renderbuffers(driContext, driContext->driReadablePriv, GL_FALSE);
	}

	old_viewport = ctx->Driver.Viewport;
	ctx->Driver.Viewport = NULL;
	radeon_window_moved(radeon);
	radeon_draw_buffer(ctx, radeon->glCtx->DrawBuffer);
	ctx->Driver.Viewport = old_viewport;
}

static void radeon_print_state_atom_prekmm(radeonContextPtr radeon, struct radeon_state_atom *state)
{
	int i, j, reg;
	int dwords = (*state->check) (radeon->glCtx, state);
	drm_r300_cmd_header_t cmd;

	fprintf(stderr, "  emit %s %d/%d\n", state->name, dwords, state->cmd_size);

	if (radeon_is_debug_enabled(RADEON_STATE, RADEON_TRACE)) {
		if (dwords > state->cmd_size)
			dwords = state->cmd_size;

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

static void radeon_print_state_atom(radeonContextPtr radeon, struct radeon_state_atom *state)
{
	int i, j, reg, count;
	int dwords;
	uint32_t packet0;
	if (!radeon_is_debug_enabled(RADEON_STATE, RADEON_VERBOSE) )
		return;

	if (!radeon->radeonScreen->kernel_mm) {
		radeon_print_state_atom_prekmm(radeon, state);
		return;
	}

	dwords = (*state->check) (radeon->glCtx, state);

	fprintf(stderr, "  emit %s %d/%d\n", state->name, dwords, state->cmd_size);

	if (radeon_is_debug_enabled(RADEON_STATE, RADEON_TRACE)) {
		if (dwords > state->cmd_size)
			dwords = state->cmd_size;
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

/**
 * Count total size for next state emit.
 **/
GLuint radeonCountStateEmitSize(radeonContextPtr radeon)
{
	struct radeon_state_atom *atom;
	GLuint dwords = 0;
	/* check if we are going to emit full state */

	if (radeon->cmdbuf.cs->cdw && !radeon->hw.all_dirty) {
		if (!radeon->hw.is_dirty)
			goto out;
		foreach(atom, &radeon->hw.atomlist) {
			if (atom->dirty) {
				const GLuint atom_size = atom->check(radeon->glCtx, atom);
				dwords += atom_size;
				if (RADEON_CMDBUF && atom_size) {
					radeon_print_state_atom(radeon, atom);
				}
			}
		}
	} else {
		foreach(atom, &radeon->hw.atomlist) {
			const GLuint atom_size = atom->check(radeon->glCtx, atom);
			dwords += atom_size;
			if (RADEON_CMDBUF && atom_size) {
				radeon_print_state_atom(radeon, atom);
			}

		}
	}
out:
	radeon_print(RADEON_STATE, RADEON_NORMAL, "%s %u\n", __func__, dwords);
	return dwords;
}

static INLINE void radeon_emit_atom(radeonContextPtr radeon, struct radeon_state_atom *atom)
{
	BATCH_LOCALS(radeon);
	int dwords;

	dwords = (*atom->check) (radeon->glCtx, atom);
	if (dwords) {

		radeon_print_state_atom(radeon, atom);

		if (atom->emit) {
			(*atom->emit)(radeon->glCtx, atom);
		} else {
			BEGIN_BATCH_NO_AUTOSTATE(dwords);
			OUT_BATCH_TABLE(atom->cmd, dwords);
			END_BATCH();
		}
		atom->dirty = GL_FALSE;

	} else {
		radeon_print(RADEON_STATE, RADEON_VERBOSE, "  skip state %s\n", atom->name);
	}

}

static INLINE void radeonEmitAtoms(radeonContextPtr radeon, GLboolean emitAll)
{
	struct radeon_state_atom *atom;

	if (radeon->vtbl.pre_emit_atoms)
		radeon->vtbl.pre_emit_atoms(radeon);

	/* Emit actual atoms */
	if (radeon->hw.all_dirty || emitAll) {
		foreach(atom, &radeon->hw.atomlist)
			radeon_emit_atom( radeon, atom );
	} else {
		foreach(atom, &radeon->hw.atomlist) {
			if ( atom->dirty )
				radeon_emit_atom( radeon, atom );
		}
	}

	COMMIT_BATCH();
}

static GLboolean radeon_revalidate_bos(GLcontext *ctx)
{
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);
	int ret;

	ret = radeon_cs_space_check(radeon->cmdbuf.cs);
	if (ret == RADEON_CS_SPACE_FLUSH)
		return GL_FALSE;
	return GL_TRUE;
}

void radeonEmitState(radeonContextPtr radeon)
{
	radeon_print(RADEON_STATE, RADEON_NORMAL, "%s\n", __FUNCTION__);

	if (radeon->vtbl.pre_emit_state)
		radeon->vtbl.pre_emit_state(radeon);

	/* this code used to return here but now it emits zbs */
	if (radeon->cmdbuf.cs->cdw && !radeon->hw.is_dirty && !radeon->hw.all_dirty)
		return;

	if (!radeon->cmdbuf.cs->cdw) {
		if (RADEON_DEBUG & RADEON_STATE)
			fprintf(stderr, "Begin reemit state\n");

		radeonEmitAtoms(radeon, GL_TRUE);
	} else {

		if (RADEON_DEBUG & RADEON_STATE)
			fprintf(stderr, "Begin dirty state\n");

		radeonEmitAtoms(radeon, GL_FALSE);
	}

	radeon->hw.is_dirty = GL_FALSE;
	radeon->hw.all_dirty = GL_FALSE;
}


void radeonFlush(GLcontext *ctx)
{
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);
	if (RADEON_DEBUG & RADEON_IOCTL)
		fprintf(stderr, "%s %d\n", __FUNCTION__, radeon->cmdbuf.cs->cdw);

	/* okay if we have no cmds in the buffer &&
	   we have no DMA flush &&
	   we have no DMA buffer allocated.
	   then no point flushing anything at all.
	*/
	if (!radeon->dma.flush && !radeon->cmdbuf.cs->cdw && is_empty_list(&radeon->dma.reserved))
		goto flush_front;

	if (radeon->dma.flush)
		radeon->dma.flush( ctx );

	if (radeon->cmdbuf.cs->cdw)
		rcommonFlushCmdBuf(radeon, __FUNCTION__);

flush_front:
	if ((ctx->DrawBuffer->Name == 0) && radeon->front_buffer_dirty) {
		__DRIscreen *const screen = radeon->radeonScreen->driScreen;

		if (screen->dri2.loader && (screen->dri2.loader->base.version >= 2)
			&& (screen->dri2.loader->flushFrontBuffer != NULL)) {
			__DRIdrawable * drawable = radeon_get_drawable(radeon);
			(*screen->dri2.loader->flushFrontBuffer)(drawable, drawable->loaderPrivate);

			/* Only clear the dirty bit if front-buffer rendering is no longer
			 * enabled.  This is done so that the dirty bit can only be set in
			 * glDrawBuffer.  Otherwise the dirty bit would have to be set at
			 * each of N places that do rendering.  This has worse performances,
			 * but it is much easier to get correct.
			 */
			if (!radeon->is_front_buffer_rendering) {
				radeon->front_buffer_dirty = GL_FALSE;
			}
		}
	}
}

/* Make sure all commands have been sent to the hardware and have
 * completed processing.
 */
void radeonFinish(GLcontext * ctx)
{
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);
	struct gl_framebuffer *fb = ctx->DrawBuffer;
	int i;

	if (ctx->Driver.Flush)
		ctx->Driver.Flush(ctx); /* +r6/r7 */

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

	if (RADEON_DEBUG & RADEON_IOCTL) {
		fprintf(stderr, "%s from %s - %i cliprects\n",
			__FUNCTION__, caller, rmesa->numClipRects);
	}

	radeonEmitQueryEnd(rmesa->glCtx);

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

	radeonReleaseDmaRegions(rmesa);

	LOCK_HARDWARE(rmesa);
	ret = rcommonFlushCmdBufLocked(rmesa, caller);
	UNLOCK_HARDWARE(rmesa);

	if (ret) {
		fprintf(stderr, "drmRadeonCmdBuffer: %d. Kernel failed to "
				"parse or rejected command stream. See dmesg "
				"for more info.\n", ret);
		exit(ret);
	}

	return ret;
}

/**
 * Make sure that enough space is available in the command buffer
 * by flushing if necessary.
 *
 * \param dwords The number of dwords we need to be free on the command buffer
 */
GLboolean rcommonEnsureCmdBufSpace(radeonContextPtr rmesa, int dwords, const char *caller)
{
   if ((rmesa->cmdbuf.cs->cdw + dwords + 128) > rmesa->cmdbuf.size
	 || radeon_cs_need_flush(rmesa->cmdbuf.cs)) {
      /* If we try to flush empty buffer there is too big rendering operation. */
      assert(rmesa->cmdbuf.cs->cdw);
      rcommonFlushCmdBuf(rmesa, caller);
      return GL_TRUE;
   }
   return GL_FALSE;
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

	radeon_print(RADEON_CS, RADEON_VERBOSE,
			"sizeof(drm_r300_cmd_header_t)=%zd\n", sizeof(drm_r300_cmd_header_t));
	radeon_print(RADEON_CS, RADEON_VERBOSE,
			"sizeof(drm_radeon_cmd_buffer_t)=%zd\n", sizeof(drm_radeon_cmd_buffer_t));
	radeon_print(RADEON_CS, RADEON_VERBOSE,
			"Allocating %d bytes command buffer (max state is %d bytes)\n",
			size * 4, rmesa->hw.max_state_size * 4);

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

	radeon_cs_space_set_flush(rmesa->cmdbuf.cs,
				  (void (*)(void *))rmesa->glCtx->Driver.Flush, rmesa->glCtx);

	if (!rmesa->radeonScreen->kernel_mm) {
		radeon_cs_set_limit(rmesa->cmdbuf.cs, RADEON_GEM_DOMAIN_VRAM, rmesa->radeonScreen->texSize[0]);
		radeon_cs_set_limit(rmesa->cmdbuf.cs, RADEON_GEM_DOMAIN_GTT, rmesa->radeonScreen->gartTextures.size);
	} else {
		struct drm_radeon_gem_info mminfo = { 0 };

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
	radeon_cs_begin(rmesa->cmdbuf.cs, n, file, function, line);

    radeon_print(RADEON_CS, RADEON_VERBOSE, "BEGIN_BATCH(%d) at %d, from %s:%i\n",
                        n, rmesa->cmdbuf.cs->cdw, function, line);

}

void radeonUserClear(GLcontext *ctx, GLuint mask)
{
   _mesa_meta_Clear(ctx, mask);
}

/*
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 *
 * Author: Alan Hourihane <alanh@tungstengraphics.com>
 * Author: Jakob Bornecrantz <wallbraker@gmail.com>
 *
 */

#include "xorg-server.h"
#include "xf86.h"
#include "xf86_OSproc.h"

#include "xorg_tracker.h"

#include "dri2.h"

#include "pipe/p_state.h"
#include "pipe/p_inlines.h"

#include "util/u_rect.h"

typedef struct {
    PixmapPtr pPixmap;
    struct pipe_texture *tex;
    struct pipe_buffer *buf;
    struct pipe_fence_handle *fence;
} *BufferPrivatePtr;

static DRI2BufferPtr
driCreateBuffers(DrawablePtr pDraw, unsigned int *attachments, int count)
{
    struct pipe_texture *depth, *tex;
    struct pipe_buffer *buf;
    ScreenPtr pScreen = pDraw->pScreen;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    BufferPrivatePtr privates;
    DRI2BufferPtr buffers;
    PixmapPtr pPixmap;
    unsigned stride, handle;
    int i;

    buffers = xcalloc(count, sizeof *buffers);
    if (!buffers)
	goto fail_buffers;

    privates = xcalloc(count, sizeof *privates);
    if (!privates)
	goto fail_privates;

    depth = NULL;
    for (i = 0; i < count; i++) {
	pPixmap = NULL;
	tex = NULL;
	buf = NULL;
	if (attachments[i] == DRI2BufferFrontLeft) {
	    if (pDraw->type == DRAWABLE_PIXMAP)
		pPixmap = (PixmapPtr) pDraw;
	    else
		pPixmap = (*pScreen->GetWindowPixmap)((WindowPtr) pDraw);
	    pPixmap->refcnt++;
	    tex = xorg_exa_get_texture(pPixmap);
	} else if (attachments[i] == DRI2BufferStencil) {
	    pipe_texture_reference(&tex, depth);
	} else if (attachments[i] == DRI2BufferDepth) {
	    struct pipe_texture template;
	    memset(&template, 0, sizeof(template));
	    template.target = PIPE_TEXTURE_2D;
	    template.format = PIPE_FORMAT_S8Z24_UNORM;
	    pf_get_block(template.format, &template.block);
	    template.width[0] = pDraw->width;
	    template.height[0] = pDraw->height;
	    template.depth[0] = 1;
	    template.last_level = 0;
	    template.tex_usage = PIPE_TEXTURE_USAGE_DEPTH_STENCIL;
	    tex = ms->screen->texture_create(ms->screen, &template);
	    depth = tex;
	} else {
	    pPixmap = (*pScreen->CreatePixmap)(pScreen, pDraw->width,
					       pDraw->height,
					       pDraw->depth,
					       0);
	    tex = xorg_exa_get_texture(pPixmap);
	}

	if (!tex)
		FatalError("NO TEXTURE IN DRI2\n");

	ms->api->buffer_from_texture(ms->api, tex, &buf, &stride);
	ms->api->global_handle_from_buffer(ms->api, ms->screen, buf, &handle);

	buffers[i].name = handle;
	buffers[i].attachment = attachments[i];
	buffers[i].pitch = stride;
	buffers[i].cpp = 4;
	buffers[i].driverPrivate = &privates[i];
	buffers[i].flags = 0; /* not tiled */
	privates[i].pPixmap = pPixmap;
	privates[i].buf = buf;
	privates[i].tex = tex;
    }

    return buffers;

fail_privates:
    xfree(buffers);
fail_buffers:
    return NULL;
}

static void
driDestroyBuffers(DrawablePtr pDraw, DRI2BufferPtr buffers, int count)
{
    ScreenPtr pScreen = pDraw->pScreen;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    BufferPrivatePtr private;
    int i;
    (void)ms;

    for (i = 0; i < count; i++) {
	private = buffers[i].driverPrivate;

	pipe_texture_reference(&private->tex, NULL);
	pipe_buffer_reference(&private->buf, NULL);
        ms->screen->fence_reference(ms->screen, &private->fence, NULL);

	if (private->pPixmap)
	    (*pScreen->DestroyPixmap)(private->pPixmap);
    }

    if (buffers) {
	xfree(buffers[0].driverPrivate);
	xfree(buffers);
    }
}

static void
driCopyRegion(DrawablePtr pDraw, RegionPtr pRegion,
              DRI2BufferPtr pDestBuffer, DRI2BufferPtr pSrcBuffer)
{
    ScreenPtr pScreen = pDraw->pScreen;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    BufferPrivatePtr dst_priv = pDestBuffer->driverPrivate;
    BufferPrivatePtr src_priv = pSrcBuffer->driverPrivate;
    PixmapPtr src_pixmap;
    PixmapPtr dst_pixmap;
    GCPtr gc;
    RegionPtr copy_clip;

    src_pixmap = src_priv->pPixmap;
    dst_pixmap = dst_priv->pPixmap;
    if (pSrcBuffer->attachment == DRI2BufferFrontLeft)
	src_pixmap = (PixmapPtr)pDraw;
    if (pDestBuffer->attachment == DRI2BufferFrontLeft)
	dst_pixmap = (PixmapPtr)pDraw;
    gc = GetScratchGC(pDraw->depth, pScreen);
    copy_clip = REGION_CREATE(pScreen, NULL, 0);
    REGION_COPY(pScreen, copy_clip, pRegion);
    (*gc->funcs->ChangeClip) (gc, CT_REGION, copy_clip, 0);
    ValidateGC(&dst_pixmap->drawable, gc);

    /* If this is a full buffer swap, throttle on the previous one */
    if (dst_priv->fence && REGION_NUM_RECTS(pRegion) == 1) {
	BoxPtr extents = REGION_EXTENTS(pScreen, pRegion);

	if (extents->x1 == 0 && extents->y1 == 0 &&
	    extents->x2 == pDraw->width && extents->y2 == pDraw->height) {
	    ms->screen->fence_finish(ms->screen, dst_priv->fence, 0);
	    ms->screen->fence_reference(ms->screen, &dst_priv->fence, NULL);
	}
    }

    (*gc->ops->CopyArea)(&src_pixmap->drawable, &dst_pixmap->drawable, gc,
			 0, 0, pDraw->width, pDraw->height, 0, 0);

    FreeScratchGC(gc);

    ms->ctx->flush(ms->ctx, PIPE_FLUSH_SWAPBUFFERS,
		   pDestBuffer->attachment == DRI2BufferFrontLeft ?
		   &dst_priv->fence : NULL);
}

Bool
driScreenInit(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    DRI2InfoRec dri2info;

    dri2info.version = 1;
    dri2info.fd = ms->fd;
#if 0
    dri2info.driverName = pScrn->name;
#else
    dri2info.driverName = "i915"; /* FIXME */
#endif
    dri2info.deviceName = "/dev/dri/card0"; /* FIXME */

    dri2info.CreateBuffers = driCreateBuffers;
    dri2info.DestroyBuffers = driDestroyBuffers;
    dri2info.CopyRegion = driCopyRegion;

    return DRI2ScreenInit(pScreen, &dri2info);
}

void
driCloseScreen(ScreenPtr pScreen)
{
    DRI2CloseScreen(pScreen);
}

/* vim: set sw=4 ts=8 sts=4: */

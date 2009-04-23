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

typedef struct {
    PixmapPtr pPixmap;
    struct pipe_texture *tex;
    struct pipe_buffer *buf;
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
	    template.tex_usage = PIPE_TEXTURE_USAGE_RENDER_TARGET;
	    tex = ms->screen->texture_create(ms->screen, &template);
	} else {
	    struct pipe_texture template;
	    memset(&template, 0, sizeof(template));
	    template.target = PIPE_TEXTURE_2D;
	    template.format = PIPE_FORMAT_A8R8G8B8_UNORM;
	    pf_get_block(template.format, &template.block);
	    template.width[0] = pDraw->width;
	    template.height[0] = pDraw->height;
	    template.depth[0] = 1;
	    template.last_level = 0;
	    template.tex_usage = PIPE_TEXTURE_USAGE_RENDER_TARGET;
	    tex = ms->screen->texture_create(ms->screen, &template);
	}

	drm_api_hooks.buffer_from_texture(tex, &buf, &stride);
	drm_api_hooks.global_handle_from_buffer(ms->screen, buf, &handle);

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

    for (i = 0; i < count; i++) {
	private = buffers[i].driverPrivate;

	if (private->pPixmap)
	    (*pScreen->DestroyPixmap)(private->pPixmap);

	pipe_texture_reference(&private->tex, NULL);
	pipe_buffer_reference(&private->buf, NULL);
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

    struct pipe_surface *dst_surf =
	ms->screen->get_tex_surface(ms->screen, dst_priv->tex, 0, 0, 0,
				    PIPE_BUFFER_USAGE_GPU_WRITE);
    struct pipe_surface *src_surf =
	ms->screen->get_tex_surface(ms->screen, src_priv->tex, 0, 0, 0,
				    PIPE_BUFFER_USAGE_GPU_READ);

    ms->ctx->surface_copy(ms->ctx, dst_surf, 0, 0, src_surf,
			  0, 0, pDraw->width, pDraw->height);

    pipe_surface_reference(&dst_surf, NULL);
    pipe_surface_reference(&src_surf, NULL);
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

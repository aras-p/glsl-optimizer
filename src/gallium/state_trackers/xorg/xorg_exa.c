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
#include "xorg_tracker.h"

#include "pipe/p_format.h"
#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "pipe/p_inlines.h"

#include "util/u_rect.h"

struct exa_entity
{
    ExaDriverPtr pExa;
    struct pipe_context *ctx;
    struct pipe_screen *scrn;
};

struct PixmapPriv
{
    int flags;
    struct pipe_texture *tex;
    unsigned int color;
    struct pipe_surface *src_surf; /* for copies */

    struct pipe_transfer *map_transfer;
    unsigned map_count;
};

/*
 * Helper functions
 */

static void
exa_get_pipe_format(int depth, enum pipe_format *format, int *bbp)
{
    switch (depth) {
    case 32:
	*format = PIPE_FORMAT_A8R8G8B8_UNORM;
	assert(*bbp == 32);
	break;
    case 24:
	*format = PIPE_FORMAT_X8R8G8B8_UNORM;
	assert(*bbp == 32);
	break;
    case 16:
	*format = PIPE_FORMAT_R5G6B5_UNORM;
	assert(*bbp == 16);
	break;
    case 15:
	*format = PIPE_FORMAT_A1R5G5B5_UNORM;
	assert(*bbp == 16);
	break;
    case 8:
    case 4:
    case 1:
	*format = PIPE_FORMAT_A8R8G8B8_UNORM; /* bad bad bad */
	break;
    default:
	assert(0);
	break;
    }
}

/*
 * Static exported EXA functions
 */

static void
ExaWaitMarker(ScreenPtr pScreen, int marker)
{
}

static int
ExaMarkSync(ScreenPtr pScreen)
{
    return 1;
}

static Bool
ExaPrepareAccess(PixmapPtr pPix, int index)
{
    ScreenPtr pScreen = pPix->drawable.pScreen;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    struct exa_entity *exa = ms->exa;
    struct PixmapPriv *priv;

    priv = exaGetPixmapDriverPrivate(pPix);

    if (!priv)
	return FALSE;

    if (!priv->tex)
	return FALSE;

    if (priv->map_count++ == 0)
    {
	priv->map_transfer =
	    exa->scrn->get_tex_transfer(exa->scrn, priv->tex, 0, 0, 0,
					PIPE_TRANSFER_READ_WRITE,
					0, 0, priv->tex->width[0], priv->tex->height[0]);

	pPix->devPrivate.ptr =
	    exa->scrn->transfer_map(exa->scrn, priv->map_transfer);
	pPix->devKind = priv->map_transfer->stride;
    }

    return TRUE;
}

static void
ExaFinishAccess(PixmapPtr pPix, int index)
{
    ScreenPtr pScreen = pPix->drawable.pScreen;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    struct exa_entity *exa = ms->exa;
    struct PixmapPriv *priv;
    priv = exaGetPixmapDriverPrivate(pPix);

    if (!priv)
	return;

    if (!priv->map_transfer)
	return;

    if (--priv->map_count == 0) {
	assert(priv->map_transfer);
	exa->scrn->transfer_unmap(exa->scrn, priv->map_transfer);
	exa->scrn->tex_transfer_destroy(priv->map_transfer);
	priv->map_transfer = NULL;
    }
}

static void
ExaDone(PixmapPtr pPixmap)
{
    ScrnInfoPtr pScrn = xf86Screens[pPixmap->drawable.pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    struct PixmapPriv *priv = exaGetPixmapDriverPrivate(pPixmap);
    struct exa_entity *exa = ms->exa;

    if (!priv)
	return;

    if (priv->src_surf)
	exa->scrn->tex_surface_destroy(priv->src_surf);
    priv->src_surf = NULL;
}

static void
ExaDoneComposite(PixmapPtr pPixmap)
{

}

static Bool
ExaPrepareSolid(PixmapPtr pPixmap, int alu, Pixel planeMask, Pixel fg)
{
    ScrnInfoPtr pScrn = xf86Screens[pPixmap->drawable.pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    struct PixmapPriv *priv = exaGetPixmapDriverPrivate(pPixmap);
    struct exa_entity *exa = ms->exa;

    if (1)
        return FALSE;

    if (pPixmap->drawable.depth < 15)
	return FALSE;

    if (!EXA_PM_IS_SOLID(&pPixmap->drawable, planeMask))
	return FALSE;

    if (!priv || !priv->tex)
	return FALSE;

    if (alu != GXcopy)
	return FALSE;

    if (!exa->ctx || !exa->ctx->surface_fill)
	return FALSE;

    priv->color = fg;

    return TRUE;
}

static void
ExaSolid(PixmapPtr pPixmap, int x0, int y0, int x1, int y1)
{
    ScrnInfoPtr pScrn = xf86Screens[pPixmap->drawable.pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    struct exa_entity *exa = ms->exa;
    struct PixmapPriv *priv = exaGetPixmapDriverPrivate(pPixmap);
    struct pipe_surface *surf =
	exa->scrn->get_tex_surface(exa->scrn, priv->tex, 0, 0, 0,
				   PIPE_BUFFER_USAGE_GPU_READ |
				   PIPE_BUFFER_USAGE_GPU_WRITE);

    exa->ctx->surface_fill(exa->ctx, surf, x0, y0, x1 - x0, y1 - y0,
			   priv->color);

    exa->scrn->tex_surface_destroy(surf);
}

static Bool
ExaPrepareCopy(PixmapPtr pSrcPixmap, PixmapPtr pDstPixmap, int xdir,
	       int ydir, int alu, Pixel planeMask)
{
    ScrnInfoPtr pScrn = xf86Screens[pDstPixmap->drawable.pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    struct exa_entity *exa = ms->exa;
    struct PixmapPriv *priv = exaGetPixmapDriverPrivate(pDstPixmap);
    struct PixmapPriv *src_priv = exaGetPixmapDriverPrivate(pSrcPixmap);

    if (1)
        return FALSE;

    if (alu != GXcopy)
	return FALSE;

    if (pSrcPixmap->drawable.depth < 15 || pDstPixmap->drawable.depth < 15)
	return FALSE;

    if (!EXA_PM_IS_SOLID(&pSrcPixmap->drawable, planeMask))
	return FALSE;

    if (!priv || !src_priv)
	return FALSE;

    if (!priv->tex || !src_priv->tex)
	return FALSE;

    if (!exa->ctx || !exa->ctx->surface_copy)
	return FALSE;

    priv->src_surf =
	exa->scrn->get_tex_surface(exa->scrn, src_priv->tex, 0, 0, 0,
				   PIPE_BUFFER_USAGE_GPU_READ |
				   PIPE_BUFFER_USAGE_GPU_WRITE);

    return TRUE;
}

static void
ExaCopy(PixmapPtr pDstPixmap, int srcX, int srcY, int dstX, int dstY,
	int width, int height)
{
    ScrnInfoPtr pScrn = xf86Screens[pDstPixmap->drawable.pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    struct exa_entity *exa = ms->exa;
    struct PixmapPriv *priv = exaGetPixmapDriverPrivate(pDstPixmap);
    struct pipe_surface *surf =
	exa->scrn->get_tex_surface(exa->scrn, priv->tex, 0, 0, 0,
				   PIPE_BUFFER_USAGE_GPU_READ |
				   PIPE_BUFFER_USAGE_GPU_WRITE);

    exa->ctx->surface_copy(exa->ctx, surf, dstX, dstY, priv->src_surf,
			   srcX, srcY, width, height);
    exa->scrn->tex_surface_destroy(surf);
}

static Bool
ExaPrepareComposite(int op, PicturePtr pSrcPicture,
		    PicturePtr pMaskPicture, PicturePtr pDstPicture,
		    PixmapPtr pSrc, PixmapPtr pMask, PixmapPtr pDst)
{
    return FALSE;
}

static void
ExaComposite(PixmapPtr pDst, int srcX, int srcY, int maskX, int maskY,
	     int dstX, int dstY, int width, int height)
{
}

static Bool
ExaCheckComposite(int op,
		  PicturePtr pSrcPicture, PicturePtr pMaskPicture,
		  PicturePtr pDstPicture)
{
    return FALSE;
}

static void *
ExaCreatePixmap(ScreenPtr pScreen, int size, int align)
{
    struct PixmapPriv *priv;

    priv = xcalloc(1, sizeof(struct PixmapPriv));
    if (!priv)
	return NULL;

    return priv;
}

static void
ExaDestroyPixmap(ScreenPtr pScreen, void *dPriv)
{
    struct PixmapPriv *priv = (struct PixmapPriv *)dPriv;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);

    if (!priv)
	return;

    if (priv->tex)
	ms->screen->texture_destroy(priv->tex);

    xfree(priv);
}

static Bool
ExaPixmapIsOffscreen(PixmapPtr pPixmap)
{
    struct PixmapPriv *priv;

    priv = exaGetPixmapDriverPrivate(pPixmap);

    if (!priv)
	return FALSE;

    if (priv->tex)
	return TRUE;

    return FALSE;
}

unsigned
xorg_exa_get_pixmap_handle(PixmapPtr pPixmap)
{
    ScreenPtr pScreen = pPixmap->drawable.pScreen;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    struct PixmapPriv *priv;
    struct pipe_buffer *buffer = NULL;
    unsigned handle;
    unsigned stride;

    if (!ms->exa) {
	FatalError("NO MS->EXA\n");
	return 0;
    }

    priv = exaGetPixmapDriverPrivate(pPixmap);

    if (!priv) {
	FatalError("NO PIXMAP PRIVATE\n");
	return 0;
    }

    drm_api_hooks.buffer_from_texture(priv->tex, &buffer, &stride);
    drm_api_hooks.handle_from_buffer(ms->screen, buffer, &handle);
    pipe_buffer_reference(&buffer, NULL);
    return handle;
}

static Bool
ExaModifyPixmapHeader(PixmapPtr pPixmap, int width, int height,
		      int depth, int bitsPerPixel, int devKind,
		      pointer pPixData)
{
    ScreenPtr pScreen = pPixmap->drawable.pScreen;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    struct PixmapPriv *priv = exaGetPixmapDriverPrivate(pPixmap);
    modesettingPtr ms = modesettingPTR(pScrn);
    struct exa_entity *exa = ms->exa;

    if (!priv)
	return FALSE;

    if (depth <= 0)
	depth = pPixmap->drawable.depth;

    if (bitsPerPixel <= 0)
	bitsPerPixel = pPixmap->drawable.bitsPerPixel;

    if (width <= 0)
	width = pPixmap->drawable.width;

    if (height <= 0)
	height = pPixmap->drawable.height;

    if (width <= 0 || height <= 0 || depth <= 0)
	return FALSE;

    miModifyPixmapHeader(pPixmap, width, height, depth,
			     bitsPerPixel, devKind, NULL);

    /* Deal with screen resize */
    if (priv->tex && (priv->tex->width[0] != width || priv->tex->height[0] != height)) {
	pipe_texture_reference(&priv->tex, NULL);
    }

    if (!priv->tex) {
	struct pipe_texture template;

	memset(&template, 0, sizeof(template));
	template.target = PIPE_TEXTURE_2D;
	exa_get_pipe_format(depth, &template.format, &bitsPerPixel);
	pf_get_block(template.format, &template.block);
	template.width[0] = width;
	template.height[0] = height;
	template.depth[0] = 1;
	template.last_level = 0;
	template.tex_usage = PIPE_TEXTURE_USAGE_RENDER_TARGET;
	priv->tex = exa->scrn->texture_create(exa->scrn, &template);
    }

    if (pPixData) {
	struct pipe_transfer *transfer =
	    exa->scrn->get_tex_transfer(exa->scrn, priv->tex, 0, 0, 0,
					PIPE_TRANSFER_WRITE,
					0, 0, width, height);
        pipe_copy_rect(exa->scrn->transfer_map(exa->scrn, transfer),
                       &priv->tex->block, transfer->stride, 0, 0,
                       width, height, pPixData, pPixmap->devKind, 0, 0);
        exa->scrn->transfer_unmap(exa->scrn, transfer);
        exa->scrn->tex_transfer_destroy(transfer);
    }

    return TRUE;
}

struct pipe_texture *
xorg_exa_get_texture(PixmapPtr pPixmap)
{
    struct PixmapPriv *priv = exaGetPixmapDriverPrivate(pPixmap);
    struct pipe_texture *tex = NULL;
    pipe_texture_reference(&tex, priv->tex);
    return tex;
}

void
xorg_exa_close(ScrnInfoPtr pScrn)
{
    modesettingPtr ms = modesettingPTR(pScrn);
    struct exa_entity *exa = ms->exa;

    if (exa->ctx)
	exa->ctx->destroy(exa->ctx);

    exaDriverFini(pScrn->pScreen);
    xfree(exa);
    ms->exa = NULL;
}

void *
xorg_exa_init(ScrnInfoPtr pScrn)
{
    modesettingPtr ms = modesettingPTR(pScrn);
    struct exa_entity *exa;
    ExaDriverPtr pExa;

    exa = xcalloc(1, sizeof(struct exa_entity));
    if (!exa)
	return NULL;

    pExa = exaDriverAlloc();
    if (!pExa) {
	goto out_err;
    }

    memset(pExa, 0, sizeof(*pExa));
    pExa->exa_major = 2;
    pExa->exa_minor = 2;
    pExa->memoryBase = 0;
    pExa->memorySize = 0;
    pExa->offScreenBase = 0;
    pExa->pixmapOffsetAlign = 0;
    pExa->pixmapPitchAlign = 1;
    pExa->flags = EXA_OFFSCREEN_PIXMAPS | EXA_HANDLES_PIXMAPS;
    pExa->maxX = 8191;		       /* FIXME */
    pExa->maxY = 8191;		       /* FIXME */
    pExa->WaitMarker = ExaWaitMarker;
    pExa->MarkSync = ExaMarkSync;
    pExa->PrepareSolid = ExaPrepareSolid;
    pExa->Solid = ExaSolid;
    pExa->DoneSolid = ExaDone;
    pExa->PrepareCopy = ExaPrepareCopy;
    pExa->Copy = ExaCopy;
    pExa->DoneCopy = ExaDone;
    pExa->CheckComposite = ExaCheckComposite;
    pExa->PrepareComposite = ExaPrepareComposite;
    pExa->Composite = ExaComposite;
    pExa->DoneComposite = ExaDoneComposite;
    pExa->PixmapIsOffscreen = ExaPixmapIsOffscreen;
    pExa->PrepareAccess = ExaPrepareAccess;
    pExa->FinishAccess = ExaFinishAccess;
    pExa->CreatePixmap = ExaCreatePixmap;
    pExa->DestroyPixmap = ExaDestroyPixmap;
    pExa->ModifyPixmapHeader = ExaModifyPixmapHeader;

    if (!exaDriverInit(pScrn->pScreen, pExa)) {
	goto out_err;
    }

    exa->scrn = ms->screen;
    exa->ctx = drm_api_hooks.create_context(exa->scrn);
    /* Share context with DRI */
    ms->ctx = exa->ctx;

    return (void *)exa;

  out_err:
    xorg_exa_close(pScrn);

    return NULL;
}

/* vim: set sw=4 ts=8 sts=4: */

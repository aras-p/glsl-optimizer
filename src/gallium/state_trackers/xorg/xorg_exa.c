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

#include "xorg_exa.h"
#include "xorg_tracker.h"
#include "xorg_composite.h"
#include "xorg_exa_tgsi.h"

#include <xorg-server.h>
#include <xf86.h>
#include <picturestr.h>
#include <picture.h>

#include "pipe/p_format.h"
#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "pipe/p_inlines.h"

#include "cso_cache/cso_context.h"

#include "util/u_rect.h"

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
ExaDownloadFromScreen(PixmapPtr pPix, int x,  int y, int w,  int h, char *dst,
		      int dst_pitch)
{
    ScreenPtr pScreen = pPix->drawable.pScreen;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    struct exa_context *exa = ms->exa;
    struct exa_pixmap_priv *priv = exaGetPixmapDriverPrivate(pPix);
    struct pipe_transfer *transfer;

    if (!priv || !priv->tex)
	return FALSE;

    if (exa->ctx->is_texture_referenced(exa->ctx, priv->tex, 0, 0) &
	PIPE_REFERENCED_FOR_WRITE)
	exa->ctx->flush(exa->ctx, 0, NULL);

    transfer = exa->scrn->get_tex_transfer(exa->scrn, priv->tex, 0, 0, 0,
					   PIPE_TRANSFER_READ, x, y, w, h);
    if (!transfer)
	return FALSE;

    util_copy_rect((unsigned char*)dst, &priv->tex->block, dst_pitch, 0, 0,
		   w, h, exa->scrn->transfer_map(exa->scrn, transfer),
		   transfer->stride, 0, 0);

    exa->scrn->transfer_unmap(exa->scrn, transfer);
    exa->scrn->tex_transfer_destroy(transfer);

    return TRUE;
}

static Bool
ExaUploadToScreen(PixmapPtr pPix, int x, int y, int w, int h, char *src,
		  int src_pitch)
{
    ScreenPtr pScreen = pPix->drawable.pScreen;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    struct exa_context *exa = ms->exa;
    struct exa_pixmap_priv *priv = exaGetPixmapDriverPrivate(pPix);
    struct pipe_transfer *transfer;

    if (!priv || !priv->tex)
	return FALSE;

    transfer = exa->scrn->get_tex_transfer(exa->scrn, priv->tex, 0, 0, 0,
					   PIPE_TRANSFER_WRITE, x, y, w, h);
    if (!transfer)
	return FALSE;

    util_copy_rect(exa->scrn->transfer_map(exa->scrn, transfer),
		   &priv->tex->block, transfer->stride, 0, 0, w, h,
		   (unsigned char*)src, src_pitch, 0, 0);

    exa->scrn->transfer_unmap(exa->scrn, transfer);
    exa->scrn->tex_transfer_destroy(transfer);

    return TRUE;
}

static Bool
ExaPrepareAccess(PixmapPtr pPix, int index)
{
    ScreenPtr pScreen = pPix->drawable.pScreen;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    struct exa_context *exa = ms->exa;
    struct exa_pixmap_priv *priv;

    priv = exaGetPixmapDriverPrivate(pPix);

    if (!priv)
	return FALSE;

    if (!priv->tex)
	return FALSE;

    if (priv->map_count++ == 0)
    {
	if (exa->ctx->is_texture_referenced(exa->ctx, priv->tex, 0, 0) &
	    PIPE_REFERENCED_FOR_WRITE)
	    exa->ctx->flush(exa->ctx, 0, NULL);

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
    struct exa_context *exa = ms->exa;
    struct exa_pixmap_priv *priv;
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
	pPix->devPrivate.ptr = NULL;
    }
}

static void
ExaDone(PixmapPtr pPixmap)
{
    ScrnInfoPtr pScrn = xf86Screens[pPixmap->drawable.pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    struct exa_pixmap_priv *priv = exaGetPixmapDriverPrivate(pPixmap);
    struct exa_context *exa = ms->exa;

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
    struct exa_pixmap_priv *priv = exaGetPixmapDriverPrivate(pPixmap);
    struct exa_context *exa = ms->exa;

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
    struct exa_context *exa = ms->exa;
    struct exa_pixmap_priv *priv = exaGetPixmapDriverPrivate(pPixmap);
    struct pipe_surface *surf = exa_gpu_surface(exa, priv);

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
    struct exa_context *exa = ms->exa;
    struct exa_pixmap_priv *priv = exaGetPixmapDriverPrivate(pDstPixmap);
    struct exa_pixmap_priv *src_priv = exaGetPixmapDriverPrivate(pSrcPixmap);

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

    priv->src_surf = exa_gpu_surface(exa, src_priv);

    return TRUE;
}

static void
ExaCopy(PixmapPtr pDstPixmap, int srcX, int srcY, int dstX, int dstY,
	int width, int height)
{
    ScrnInfoPtr pScrn = xf86Screens[pDstPixmap->drawable.pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    struct exa_context *exa = ms->exa;
    struct exa_pixmap_priv *priv = exaGetPixmapDriverPrivate(pDstPixmap);
    struct pipe_surface *surf = exa_gpu_surface(exa, priv);

    exa->ctx->surface_copy(exa->ctx, surf, dstX, dstY, priv->src_surf,
			   srcX, srcY, width, height);
    exa->scrn->tex_surface_destroy(surf);
}

static Bool
ExaPrepareComposite(int op, PicturePtr pSrcPicture,
		    PicturePtr pMaskPicture, PicturePtr pDstPicture,
		    PixmapPtr pSrc, PixmapPtr pMask, PixmapPtr pDst)
{
   ScrnInfoPtr pScrn = xf86Screens[pDst->drawable.pScreen->myNum];
   modesettingPtr ms = modesettingPTR(pScrn);
   struct exa_context *exa = ms->exa;

   return xorg_composite_bind_state(exa, op, pSrcPicture, pMaskPicture,
                                    pDstPicture,
                                    exaGetPixmapDriverPrivate(pSrc),
                                    exaGetPixmapDriverPrivate(pMask),
                                    exaGetPixmapDriverPrivate(pDst));
}

static void
ExaComposite(PixmapPtr pDst, int srcX, int srcY, int maskX, int maskY,
	     int dstX, int dstY, int width, int height)
{
   ScrnInfoPtr pScrn = xf86Screens[pDst->drawable.pScreen->myNum];
   modesettingPtr ms = modesettingPTR(pScrn);
   struct exa_context *exa = ms->exa;
   struct exa_pixmap_priv *priv = exaGetPixmapDriverPrivate(pDst);

   xorg_composite(exa, priv, srcX, srcY, maskX, maskY,
                  dstX, dstY, width, height);
}

static Bool
ExaCheckComposite(int op,
		  PicturePtr pSrcPicture, PicturePtr pMaskPicture,
		  PicturePtr pDstPicture)
{
   return xorg_composite_accelerated(op,
                                     pSrcPicture,
                                     pMaskPicture,
                                     pDstPicture);
}

static void *
ExaCreatePixmap(ScreenPtr pScreen, int size, int align)
{
    struct exa_pixmap_priv *priv;

    priv = xcalloc(1, sizeof(struct exa_pixmap_priv));
    if (!priv)
	return NULL;

    return priv;
}

static void
ExaDestroyPixmap(ScreenPtr pScreen, void *dPriv)
{
    struct exa_pixmap_priv *priv = (struct exa_pixmap_priv *)dPriv;
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
    struct exa_pixmap_priv *priv;

    priv = exaGetPixmapDriverPrivate(pPixmap);

    if (!priv)
	return FALSE;

    if (priv->tex)
	return TRUE;

    return FALSE;
}

int
xorg_exa_set_displayed_usage(PixmapPtr pPixmap)
{
    struct exa_pixmap_priv *priv;
    priv = exaGetPixmapDriverPrivate(pPixmap);

    if (!priv) {
	FatalError("NO PIXMAP PRIVATE\n");
	return 0;
    }

    priv->flags |= PIPE_TEXTURE_USAGE_PRIMARY;

    return 0;
}

int
xorg_exa_set_shared_usage(PixmapPtr pPixmap)
{
    struct exa_pixmap_priv *priv;
    priv = exaGetPixmapDriverPrivate(pPixmap);

    if (!priv) {
	FatalError("NO PIXMAP PRIVATE\n");
	return 0;
    }

    priv->flags |= PIPE_TEXTURE_USAGE_DISPLAY_TARGET;

    return 0;
}

unsigned
xorg_exa_get_pixmap_handle(PixmapPtr pPixmap, unsigned *stride_out)
{
    ScreenPtr pScreen = pPixmap->drawable.pScreen;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    struct exa_pixmap_priv *priv;
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

    ms->api->local_handle_from_texture(ms->api, ms->screen, priv->tex, &stride, &handle);
    if (stride_out)
	*stride_out = stride;

    return handle;
}

static Bool
ExaModifyPixmapHeader(PixmapPtr pPixmap, int width, int height,
		      int depth, int bitsPerPixel, int devKind,
		      pointer pPixData)
{
    ScreenPtr pScreen = pPixmap->drawable.pScreen;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    struct exa_pixmap_priv *priv = exaGetPixmapDriverPrivate(pPixmap);
    modesettingPtr ms = modesettingPTR(pScrn);
    struct exa_context *exa = ms->exa;

    if (!priv || pPixData)
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
    if (!priv->tex ||
        (priv->tex->width[0] != width ||
         priv->tex->height[0] != height ||
         priv->tex_flags != priv->flags)) {
	struct pipe_texture *texture = NULL;

#ifdef DRM_MODE_FEATURE_DIRTYFB
	if (priv->flags)
#endif
	{
	    struct pipe_texture template;

	    memset(&template, 0, sizeof(template));
	    template.target = PIPE_TEXTURE_2D;
	    exa_get_pipe_format(depth, &template.format, &bitsPerPixel);
	    pf_get_block(template.format, &template.block);
	    template.width[0] = width;
	    template.height[0] = height;
	    template.depth[0] = 1;
	    template.last_level = 0;
	    template.tex_usage = PIPE_TEXTURE_USAGE_RENDER_TARGET | priv->flags;
	    priv->tex_flags = priv->flags;
	    texture = exa->scrn->texture_create(exa->scrn, &template);

	    if (priv->tex) {
		struct pipe_surface *dst_surf;

		dst_surf = exa->scrn->get_tex_surface(exa->scrn, texture, 0, 0, 0,
						      PIPE_BUFFER_USAGE_GPU_WRITE);
		priv->src_surf = exa_gpu_surface(exa, priv);
		exa->ctx->surface_copy(exa->ctx, dst_surf, 0, 0, priv->src_surf,
				       0, 0, min(width, texture->width[0]),
				       min(height, texture->height[0]));
		exa->scrn->tex_surface_destroy(dst_surf);
		exa->scrn->tex_surface_destroy(priv->src_surf);
		priv->src_surf = NULL;
	    } else if (pPixmap->devPrivate.ptr) {
		struct pipe_transfer *transfer;

		if (priv->map_count != 0)
		     FatalError("doing ExaModifyPixmapHeader on mapped buffer\n");

		transfer =
		    exa->scrn->get_tex_transfer(exa->scrn, texture, 0, 0, 0,
						PIPE_TRANSFER_WRITE,
						0, 0, width, height);
		util_copy_rect(exa->scrn->transfer_map(exa->scrn, transfer),
			       &texture->block, transfer->stride, 0, 0,
			       width, height, pPixmap->devPrivate.ptr,
			       pPixmap->devKind, 0, 0);
		exa->scrn->transfer_unmap(exa->scrn, transfer);
		exa->scrn->tex_transfer_destroy(transfer);
	    }
	}
#ifdef DRM_MODE_FEATURE_DIRTYFB
	else {
	    xfree(pPixmap->devPrivate.ptr);
	    pPixmap->devPrivate.ptr = xalloc(pPixmap->drawable.height *
					     pPixmap->devKind);
	}
#endif

	pipe_texture_reference(&priv->tex, texture);
    }

    return TRUE;
}

struct pipe_texture *
xorg_exa_get_texture(PixmapPtr pPixmap)
{
   struct exa_pixmap_priv *priv = exaGetPixmapDriverPrivate(pPixmap);
   struct pipe_texture *tex = NULL;
   pipe_texture_reference(&tex, priv->tex);
   return tex;
}

void
xorg_exa_close(ScrnInfoPtr pScrn)
{
   modesettingPtr ms = modesettingPTR(pScrn);
   struct exa_context *exa = ms->exa;
   struct pipe_constant_buffer *vsbuf = &exa->vs_const_buffer;
   struct pipe_constant_buffer *fsbuf = &exa->fs_const_buffer;

   if (exa->shaders) {
      xorg_shaders_destroy(exa->shaders);
   }

   if (vsbuf && vsbuf->buffer)
      pipe_buffer_reference(&vsbuf->buffer, NULL);

   if (fsbuf && fsbuf->buffer)
      pipe_buffer_reference(&fsbuf->buffer, NULL);

   if (exa->cso) {
      cso_release_all(exa->cso);
      cso_destroy_context(exa->cso);
   }

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
   struct exa_context *exa;
   ExaDriverPtr pExa;

   exa = xcalloc(1, sizeof(struct exa_context));
   if (!exa)
      return NULL;

   pExa = exaDriverAlloc();
   if (!pExa) {
      goto out_err;
   }

   memset(pExa, 0, sizeof(*pExa));

   pExa->exa_major         = 2;
   pExa->exa_minor         = 2;
   pExa->memoryBase        = 0;
   pExa->memorySize        = 0;
   pExa->offScreenBase     = 0;
   pExa->pixmapOffsetAlign = 0;
   pExa->pixmapPitchAlign  = 1;
   pExa->flags             = EXA_OFFSCREEN_PIXMAPS | EXA_HANDLES_PIXMAPS;
   pExa->maxX              = 8191; /* FIXME */
   pExa->maxY              = 8191; /* FIXME */

   pExa->WaitMarker         = ExaWaitMarker;
   pExa->MarkSync           = ExaMarkSync;
   pExa->PrepareSolid       = ExaPrepareSolid;
   pExa->Solid              = ExaSolid;
   pExa->DoneSolid          = ExaDone;
   pExa->PrepareCopy        = ExaPrepareCopy;
   pExa->Copy               = ExaCopy;
   pExa->DoneCopy           = ExaDone;
   pExa->CheckComposite     = ExaCheckComposite;
   pExa->PrepareComposite   = ExaPrepareComposite;
   pExa->Composite          = ExaComposite;
   pExa->DoneComposite      = ExaDoneComposite;
   pExa->PixmapIsOffscreen  = ExaPixmapIsOffscreen;
   pExa->DownloadFromScreen = ExaDownloadFromScreen;
   pExa->UploadToScreen     = ExaUploadToScreen;
   pExa->PrepareAccess      = ExaPrepareAccess;
   pExa->FinishAccess       = ExaFinishAccess;
   pExa->CreatePixmap       = ExaCreatePixmap;
   pExa->DestroyPixmap      = ExaDestroyPixmap;
   pExa->ModifyPixmapHeader = ExaModifyPixmapHeader;

   if (!exaDriverInit(pScrn->pScreen, pExa)) {
      goto out_err;
   }

   exa->scrn = ms->screen;
   exa->ctx = ms->api->create_context(ms->api, exa->scrn);
   /* Share context with DRI */
   ms->ctx = exa->ctx;

   exa->cso = cso_create_context(exa->ctx);
   exa->shaders = xorg_shaders_create(exa);

   return (void *)exa;

out_err:
   xorg_exa_close(pScrn);

   return NULL;
}

struct pipe_surface *
exa_gpu_surface(struct exa_context *exa, struct exa_pixmap_priv *priv)
{
   return exa->scrn->get_tex_surface(exa->scrn, priv->tex, 0, 0, 0,
                                     PIPE_BUFFER_USAGE_GPU_READ |
                                     PIPE_BUFFER_USAGE_GPU_WRITE);

}


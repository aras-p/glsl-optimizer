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

#define DEBUG_PRINT 0
#define DEBUG_SOLID 0
#define DISABLE_ACCEL 0

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
	*format = PIPE_FORMAT_I8_UNORM;
	assert(*bbp == 8);
	break;
    case 4:
    case 1:
	*format = PIPE_FORMAT_A8R8G8B8_UNORM; /* bad bad bad */
	break;
    default:
	assert(0);
	break;
    }
}

static void
xorg_exa_init_state(struct exa_context *exa)
{
   struct pipe_depth_stencil_alpha_state dsa;

   /* set common initial clip state */
   memset(&dsa, 0, sizeof(struct pipe_depth_stencil_alpha_state));
   cso_set_depth_stencil_alpha(exa->cso, &dsa);
}

static void
xorg_exa_common_done(struct exa_context *exa)
{
   exa->copy.src = NULL;
   exa->copy.dst = NULL;
   exa->has_solid_color = FALSE;
   exa->num_bound_samplers = 0;
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

    if (exa->pipe->is_texture_referenced(exa->pipe, priv->tex, 0, 0) &
	PIPE_REFERENCED_FOR_WRITE)
	exa->pipe->flush(exa->pipe, 0, NULL);

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
	if (exa->pipe->is_texture_referenced(exa->pipe, priv->tex, 0, 0) &
	    PIPE_REFERENCED_FOR_WRITE)
	    exa->pipe->flush(exa->pipe, 0, NULL);

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

#if 1
    xorg_exa_flush(exa, PIPE_FLUSH_RENDER_CACHE, NULL);
#else
    xorg_exa_finish(exa);
#endif
    xorg_exa_common_done(exa);
}

static void
ExaDoneComposite(PixmapPtr pPixmap)
{
   ScrnInfoPtr pScrn = xf86Screens[pPixmap->drawable.pScreen->myNum];
   modesettingPtr ms = modesettingPTR(pScrn);
   struct exa_context *exa = ms->exa;

   xorg_exa_common_done(exa);
}

static Bool
ExaPrepareSolid(PixmapPtr pPixmap, int alu, Pixel planeMask, Pixel fg)
{
    ScrnInfoPtr pScrn = xf86Screens[pPixmap->drawable.pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    struct exa_pixmap_priv *priv = exaGetPixmapDriverPrivate(pPixmap);
    struct exa_context *exa = ms->exa;

#if DEBUG_PRINT
    debug_printf("ExaPrepareSolid(0x%x)\n", fg);
#endif
    if (!exa->pipe)
	XORG_FALLBACK("solid accle not enabled");

    if (!priv || !priv->tex)
	XORG_FALLBACK("solid !priv || !priv->tex");

    if (!EXA_PM_IS_SOLID(&pPixmap->drawable, planeMask))
	XORG_FALLBACK("solid planeMask is not solid");

    if (alu != GXcopy)
	XORG_FALLBACK("solid not GXcopy");

    if (!exa->scrn->is_format_supported(exa->scrn, priv->tex->format,
                                        priv->tex->target,
                                        PIPE_TEXTURE_USAGE_RENDER_TARGET, 0)) {
	XORG_FALLBACK("solid bad format %s", pf_name(priv->tex->format));
    }

#if DEBUG_SOLID
    fg = 0xffff0000;
#endif

#if DISABLE_ACCEL
    return FALSE;
#else
    return xorg_solid_bind_state(exa, priv, fg);
#endif
}

static void
ExaSolid(PixmapPtr pPixmap, int x0, int y0, int x1, int y1)
{
    ScrnInfoPtr pScrn = xf86Screens[pPixmap->drawable.pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    struct exa_context *exa = ms->exa;
    struct exa_pixmap_priv *priv = exaGetPixmapDriverPrivate(pPixmap);

#if DEBUG_PRINT
    debug_printf("\tExaSolid(%d, %d, %d, %d)\n", x0, y0, x1, y1);
#endif

#if 0
    if (x0 == 0 && y0 == 0 &&
        x1 == priv->tex->width[0] &&
        y1 == priv->tex->height[0]) {
       exa->ctx->clear(exa->pipe, PIPE_CLEAR_COLOR,
                       exa->solid_color, 1., 0);
    } else
#endif

#if DEBUG_SOLID
       exa->solid_color[0] = 0.f;
       exa->solid_color[1] = 1.f;
       exa->solid_color[2] = 0.f;
       exa->solid_color[3] = 1.f;
    xorg_solid(exa, priv, 0, 0, 1024, 768);
       exa->solid_color[0] = 1.f;
       exa->solid_color[1] = 0.f;
       exa->solid_color[2] = 0.f;
       exa->solid_color[3] = 1.f;
       xorg_solid(exa, priv, 0, 0, 300, 300);
       xorg_solid(exa, priv, 300, 300, 350, 350);
       xorg_solid(exa, priv, 350, 350, 500, 500);

       xorg_solid(exa, priv,
               priv->tex->width[0] - 10,
               priv->tex->height[0] - 10,
               priv->tex->width[0],
               priv->tex->height[0]);

    exa->solid_color[0] = 0.f;
    exa->solid_color[1] = 0.f;
    exa->solid_color[2] = 1.f;
    exa->solid_color[3] = 1.f;

    exa->has_solid_color = FALSE;
    ExaPrepareCopy(pPixmap, pPixmap, 0, 0, GXcopy, 0xffffffff);
    ExaCopy(pPixmap, 0, 0, 50, 50, 500, 500);
#else
    xorg_solid(exa, priv, x0, y0, x1, y1) ;
#endif
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

#if DEBUG_PRINT
    debug_printf("ExaPrepareCopy\n");
#endif
    if (!exa->pipe)
	XORG_FALLBACK("copy accle not enabled");

    if (!priv || !src_priv)
	XORG_FALLBACK("copy !priv || !src_priv");

    if (!priv->tex || !src_priv->tex)
	XORG_FALLBACK("copy !priv->tex || !src_priv->tex");

    if (!EXA_PM_IS_SOLID(&pSrcPixmap->drawable, planeMask))
	XORG_FALLBACK("copy planeMask is not solid");

    if (alu != GXcopy)
	XORG_FALLBACK("copy alu not GXcopy");

    if (!exa->scrn->is_format_supported(exa->scrn, priv->tex->format,
                                        priv->tex->target,
                                        PIPE_TEXTURE_USAGE_RENDER_TARGET, 0))
	XORG_FALLBACK("copy pDst format %s", pf_name(priv->tex->format));

    if (!exa->scrn->is_format_supported(exa->scrn, src_priv->tex->format,
                                        src_priv->tex->target,
                                        PIPE_TEXTURE_USAGE_SAMPLER, 0))
	XORG_FALLBACK("copy pSrc format %s", pf_name(src_priv->tex->format));

    exa->copy.src = src_priv;
    exa->copy.dst = priv;

#if DISABLE_ACCEL
    return FALSE;
#else
    return TRUE;
#endif
}

static void
ExaCopy(PixmapPtr pDstPixmap, int srcX, int srcY, int dstX, int dstY,
	int width, int height)
{
   ScrnInfoPtr pScrn = xf86Screens[pDstPixmap->drawable.pScreen->myNum];
   modesettingPtr ms = modesettingPTR(pScrn);
   struct exa_context *exa = ms->exa;
   struct exa_pixmap_priv *priv = exaGetPixmapDriverPrivate(pDstPixmap);

#if DEBUG_PRINT
   debug_printf("\tExaCopy(srcx=%d, srcy=%d, dstX=%d, dstY=%d, w=%d, h=%d)\n",
                srcX, srcY, dstX, dstY, width, height);
#endif

   debug_assert(priv == exa->copy.dst);

   xorg_copy_pixmap(exa, exa->copy.dst, dstX, dstY,
                    exa->copy.src, srcX, srcY,
                    width, height);
}

static Bool
ExaPrepareComposite(int op, PicturePtr pSrcPicture,
		    PicturePtr pMaskPicture, PicturePtr pDstPicture,
		    PixmapPtr pSrc, PixmapPtr pMask, PixmapPtr pDst)
{
   ScrnInfoPtr pScrn = xf86Screens[pDst->drawable.pScreen->myNum];
   modesettingPtr ms = modesettingPTR(pScrn);
   struct exa_context *exa = ms->exa;
   struct exa_pixmap_priv *priv;

#if DEBUG_PRINT
   debug_printf("ExaPrepareComposite\n");
#endif
   if (!exa->pipe)
      XORG_FALLBACK("comp accle not enabled");

   priv = exaGetPixmapDriverPrivate(pDst);
   if (!priv || !priv->tex)
      XORG_FALLBACK("comp pDst %s", !priv ? "!priv" : "!priv->tex");

   if (!exa->scrn->is_format_supported(exa->scrn, priv->tex->format,
                                       priv->tex->target,
                                       PIPE_TEXTURE_USAGE_RENDER_TARGET, 0))
      XORG_FALLBACK("copy pDst format: %s", pf_name(priv->tex->format));

   if (pSrc) {
      priv = exaGetPixmapDriverPrivate(pSrc);
      if (!priv || !priv->tex)
         XORG_FALLBACK("comp pSrc %s", !priv ? "!priv" : "!priv->tex");

      if (!exa->scrn->is_format_supported(exa->scrn, priv->tex->format,
                                          priv->tex->target,
                                          PIPE_TEXTURE_USAGE_SAMPLER, 0))
         XORG_FALLBACK("copy pSrc format: %s", pf_name(priv->tex->format));
   }

   if (pMask) {
      priv = exaGetPixmapDriverPrivate(pMask);
      if (!priv || !priv->tex)
         XORG_FALLBACK("comp pMask %s", !priv ? "!priv" : "!priv->tex");

      if (!exa->scrn->is_format_supported(exa->scrn, priv->tex->format,
                                          priv->tex->target,
                                          PIPE_TEXTURE_USAGE_SAMPLER, 0))
         XORG_FALLBACK("copy pMask format: %s", pf_name(priv->tex->format));
   }

#if DISABLE_ACCEL
   (void) exa;
   return FALSE;
#else
   return xorg_composite_bind_state(exa, op, pSrcPicture, pMaskPicture,
                                    pDstPicture,
                                    pSrc ? exaGetPixmapDriverPrivate(pSrc) : NULL,
                                    pMask ? exaGetPixmapDriverPrivate(pMask) : NULL,
                                    exaGetPixmapDriverPrivate(pDst));
#endif
}

static void
ExaComposite(PixmapPtr pDst, int srcX, int srcY, int maskX, int maskY,
	     int dstX, int dstY, int width, int height)
{
   ScrnInfoPtr pScrn = xf86Screens[pDst->drawable.pScreen->myNum];
   modesettingPtr ms = modesettingPTR(pScrn);
   struct exa_context *exa = ms->exa;
   struct exa_pixmap_priv *priv = exaGetPixmapDriverPrivate(pDst);

#if DEBUG_PRINT
   debug_printf("\tExaComposite\n");
#endif

   xorg_composite(exa, priv, srcX, srcY, maskX, maskY,
                  dstX, dstY, width, height);
}

static Bool
ExaCheckComposite(int op,
		  PicturePtr pSrcPicture, PicturePtr pMaskPicture,
		  PicturePtr pDstPicture)
{
   boolean accelerated = xorg_composite_accelerated(op,
                                                    pSrcPicture,
                                                    pMaskPicture,
                                                    pDstPicture);
#if DEBUG_PRINT
   debug_printf("ExaCheckComposite(%d, %p, %p, %p) = %d\n",
                op, pSrcPicture, pMaskPicture, pDstPicture, accelerated);
#endif
   return accelerated;
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

    if (!priv)
	return;

    pipe_texture_reference(&priv->tex, NULL);

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
                struct pipe_surface *src_surf;

		dst_surf = exa->scrn->get_tex_surface(
                   exa->scrn, texture, 0, 0, 0, PIPE_BUFFER_USAGE_GPU_WRITE);
		src_surf = exa_gpu_surface(exa, priv);
		exa->pipe->surface_copy(exa->pipe, dst_surf, 0, 0, src_surf,
                                        0, 0, min(width, texture->width[0]),
                                        min(height, texture->height[0]));
		exa->scrn->tex_surface_destroy(dst_surf);
		exa->scrn->tex_surface_destroy(src_surf);
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

		xfree(pPixmap->devPrivate.ptr);
		pPixmap->devPrivate.ptr = NULL;
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
	/* the texture we create has one reference */
	pipe_texture_reference(&texture, NULL);
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

   if (exa->pipe)
      exa->pipe->destroy(exa->pipe);

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
#ifdef EXA_SUPPORTS_PREPARE_AUX
   pExa->flags            |= EXA_SUPPORTS_PREPARE_AUX;
#endif
#ifdef EXA_MIXED_PIXMAPS
   pExa->flags            |= EXA_MIXED_PIXMAPS;
#endif
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
   exa->pipe = ms->api->create_context(ms->api, exa->scrn);
   /* Share context with DRI */
   ms->ctx = exa->pipe;

   exa->cso = cso_create_context(exa->pipe);
   exa->shaders = xorg_shaders_create(exa);

   xorg_exa_init_state(exa);

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

void xorg_exa_flush(struct exa_context *exa, uint pipeFlushFlags,
                    struct pipe_fence_handle **fence)
{
   exa->pipe->flush(exa->pipe, pipeFlushFlags, fence);
}

void xorg_exa_finish(struct exa_context *exa)
{
   struct pipe_fence_handle *fence = NULL;

   xorg_exa_flush(exa, PIPE_FLUSH_RENDER_CACHE, &fence);

   exa->pipe->screen->fence_finish(exa->pipe->screen, fence, 0);
   exa->pipe->screen->fence_reference(exa->pipe->screen, &fence, NULL);
}


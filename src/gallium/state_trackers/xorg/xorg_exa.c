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

#include "util/u_rect.h"
#include "util/u_math.h"
#include "util/u_debug.h"
#include "util/u_format.h"

#define DEBUG_PRINT 0
#define ROUND_UP_TEXTURES 1

/*
 * Helper functions
 */
struct render_format_str {
   int format;
   const char *name;
};
static const struct render_format_str formats_info[] =
{
   {PICT_a8r8g8b8, "PICT_a8r8g8b8"},
   {PICT_x8r8g8b8, "PICT_x8r8g8b8"},
   {PICT_a8b8g8r8, "PICT_a8b8g8r8"},
   {PICT_x8b8g8r8, "PICT_x8b8g8r8"},
#ifdef PICT_TYPE_BGRA
   {PICT_b8g8r8a8, "PICT_b8g8r8a8"},
   {PICT_b8g8r8x8, "PICT_b8g8r8x8"},
   {PICT_a2r10g10b10, "PICT_a2r10g10b10"},
   {PICT_x2r10g10b10, "PICT_x2r10g10b10"},
   {PICT_a2b10g10r10, "PICT_a2b10g10r10"},
   {PICT_x2b10g10r10, "PICT_x2b10g10r10"},
#endif
   {PICT_r8g8b8, "PICT_r8g8b8"},
   {PICT_b8g8r8, "PICT_b8g8r8"},
   {PICT_r5g6b5, "PICT_r5g6b5"},
   {PICT_b5g6r5, "PICT_b5g6r5"},
   {PICT_a1r5g5b5, "PICT_a1r5g5b5"},
   {PICT_x1r5g5b5, "PICT_x1r5g5b5"},
   {PICT_a1b5g5r5, "PICT_a1b5g5r5"},
   {PICT_x1b5g5r5, "PICT_x1b5g5r5"},
   {PICT_a4r4g4b4, "PICT_a4r4g4b4"},
   {PICT_x4r4g4b4, "PICT_x4r4g4b4"},
   {PICT_a4b4g4r4, "PICT_a4b4g4r4"},
   {PICT_x4b4g4r4, "PICT_x4b4g4r4"},
   {PICT_a8, "PICT_a8"},
   {PICT_r3g3b2, "PICT_r3g3b2"},
   {PICT_b2g3r3, "PICT_b2g3r3"},
   {PICT_a2r2g2b2, "PICT_a2r2g2b2"},
   {PICT_a2b2g2r2, "PICT_a2b2g2r2"},
   {PICT_c8, "PICT_c8"},
   {PICT_g8, "PICT_g8"},
   {PICT_x4a4, "PICT_x4a4"},
   {PICT_x4c4, "PICT_x4c4"},
   {PICT_x4g4, "PICT_x4g4"},
   {PICT_a4, "PICT_a4"},
   {PICT_r1g2b1, "PICT_r1g2b1"},
   {PICT_b1g2r1, "PICT_b1g2r1"},
   {PICT_a1r1g1b1, "PICT_a1r1g1b1"},
   {PICT_a1b1g1r1, "PICT_a1b1g1r1"},
   {PICT_c4, "PICT_c4"},
   {PICT_g4, "PICT_g4"},
   {PICT_a1, "PICT_a1"},
   {PICT_g1, "PICT_g1"}
};
static const char *render_format_name(int format)
{
   int i = 0;
   for (i = 0; i < sizeof(formats_info)/sizeof(formats_info[0]); ++i) {
      if (formats_info[i].format == format)
         return formats_info[i].name;
   }
   return NULL;
}

static void
exa_get_pipe_format(int depth, enum pipe_format *format, int *bbp, int *picture_format)
{
    switch (depth) {
    case 32:
	*format = PIPE_FORMAT_B8G8R8A8_UNORM;
	*picture_format = PICT_a8r8g8b8;
	assert(*bbp == 32);
	break;
    case 24:
	*format = PIPE_FORMAT_B8G8R8X8_UNORM;
	*picture_format = PICT_x8r8g8b8;
	assert(*bbp == 32);
	break;
    case 16:
	*format = PIPE_FORMAT_B5G6R5_UNORM;
	*picture_format = PICT_r5g6b5;
	assert(*bbp == 16);
	break;
    case 15:
	*format = PIPE_FORMAT_B5G5R5A1_UNORM;
	*picture_format = PICT_x1r5g5b5;
	assert(*bbp == 16);
	break;
    case 8:
	*format = PIPE_FORMAT_L8_UNORM;
	*picture_format = PICT_a8;
	assert(*bbp == 8);
	break;
    case 4:
    case 1:
	*format = PIPE_FORMAT_B8G8R8A8_UNORM; /* bad bad bad */
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
   /* Nothing to do, handled in the PrepareAccess hook */
}

static int
ExaMarkSync(ScreenPtr pScreen)
{
   return 1;
}


/***********************************************************************
 * Screen upload/download
 */

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

#if DEBUG_PRINT
    debug_printf("------ ExaDownloadFromScreen(%d, %d, %d, %d, %d)\n",
                 x, y, w, h, dst_pitch);
#endif

    util_copy_rect((unsigned char*)dst, priv->tex->format, dst_pitch, 0, 0,
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

    /* make sure that any pending operations are flushed to hardware */
    if (exa->pipe->is_texture_referenced(exa->pipe, priv->tex, 0, 0) &
	(PIPE_REFERENCED_FOR_READ | PIPE_REFERENCED_FOR_WRITE))
	xorg_exa_flush(exa, 0, NULL);

    transfer = exa->scrn->get_tex_transfer(exa->scrn, priv->tex, 0, 0, 0,
					   PIPE_TRANSFER_WRITE, x, y, w, h);
    if (!transfer)
	return FALSE;

#if DEBUG_PRINT
    debug_printf("++++++ ExaUploadToScreen(%d, %d, %d, %d, %d)\n",
                 x, y, w, h, src_pitch);
#endif

    util_copy_rect(exa->scrn->transfer_map(exa->scrn, transfer),
		   priv->tex->format, transfer->stride, 0, 0, w, h,
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

    if (priv->map_count == 0)
    {
	if (exa->pipe->is_texture_referenced(exa->pipe, priv->tex, 0, 0) &
	    PIPE_REFERENCED_FOR_WRITE)
	    exa->pipe->flush(exa->pipe, 0, NULL);

        assert(pPix->drawable.width <= priv->tex->width0);
        assert(pPix->drawable.height <= priv->tex->height0);

	priv->map_transfer =
	    exa->scrn->get_tex_transfer(exa->scrn, priv->tex, 0, 0, 0,
#ifdef EXA_MIXED_PIXMAPS
					PIPE_TRANSFER_MAP_DIRECTLY |
#endif
					PIPE_TRANSFER_READ_WRITE,
					0, 0, 
                                        pPix->drawable.width,
                                        pPix->drawable.height );
	if (!priv->map_transfer)
#ifdef EXA_MIXED_PIXMAPS
	    return FALSE;
#else
	    FatalError("failed to create transfer\n");
#endif

	pPix->devPrivate.ptr =
	    exa->scrn->transfer_map(exa->scrn, priv->map_transfer);
	pPix->devKind = priv->map_transfer->stride;
    }

    priv->map_count++;

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

/***********************************************************************
 * Solid Fills
 */

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
    if (!exa->accel)
	return FALSE;

    if (!exa->pipe)
	XORG_FALLBACK("accle not enabled");

    if (!priv || !priv->tex)
	XORG_FALLBACK("%s", !priv ? "!priv" : "!priv->tex");

    if (!EXA_PM_IS_SOLID(&pPixmap->drawable, planeMask))
	XORG_FALLBACK("planeMask is not solid");

    if (alu != GXcopy)
	XORG_FALLBACK("not GXcopy");

    if (!exa->scrn->is_format_supported(exa->scrn, priv->tex->format,
                                        priv->tex->target,
                                        PIPE_TEXTURE_USAGE_RENDER_TARGET, 0)) {
	XORG_FALLBACK("format %s", util_format_name(priv->tex->format));
    }

    return xorg_solid_bind_state(exa, priv, fg);
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

    if (x0 == 0 && y0 == 0 &&
        x1 == pPixmap->drawable.width && y1 == pPixmap->drawable.height) {
       exa->pipe->clear(exa->pipe, PIPE_CLEAR_COLOR, exa->solid_color, 0.0, 0);
       return;
    }

    xorg_solid(exa, priv, x0, y0, x1, y1) ;
}


static void
ExaDoneSolid(PixmapPtr pPixmap)
{
    ScrnInfoPtr pScrn = xf86Screens[pPixmap->drawable.pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    struct exa_pixmap_priv *priv = exaGetPixmapDriverPrivate(pPixmap);
    struct exa_context *exa = ms->exa;

    if (!priv)
	return;
   
    xorg_composite_done(exa);
}

/***********************************************************************
 * Copy Blits
 */

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

    if (!exa->accel)
	return FALSE;

    if (!exa->pipe)
	XORG_FALLBACK("accle not enabled");

    if (!priv || !priv->tex)
	XORG_FALLBACK("pDst %s", !priv ? "!priv" : "!priv->tex");

    if (!src_priv || !src_priv->tex)
	XORG_FALLBACK("pSrc %s", !src_priv ? "!priv" : "!priv->tex");

    if (!EXA_PM_IS_SOLID(&pSrcPixmap->drawable, planeMask))
	XORG_FALLBACK("planeMask is not solid");

    if (alu != GXcopy)
	XORG_FALLBACK("alu not GXcopy");

    if (!exa->scrn->is_format_supported(exa->scrn, priv->tex->format,
                                        priv->tex->target,
                                        PIPE_TEXTURE_USAGE_RENDER_TARGET, 0))
	XORG_FALLBACK("pDst format %s", util_format_name(priv->tex->format));

    if (!exa->scrn->is_format_supported(exa->scrn, src_priv->tex->format,
                                        src_priv->tex->target,
                                        PIPE_TEXTURE_USAGE_SAMPLER, 0))
	XORG_FALLBACK("pSrc format %s", util_format_name(src_priv->tex->format));

    exa->copy.src = src_priv;
    exa->copy.dst = priv;

    /* For same-surface copies, the pipe->surface_copy path is clearly
     * superior, providing it is implemented.  In other cases it's not
     * clear what the better path would be, and eventually we'd
     * probably want to gather timings and choose dynamically.
     */
    if (exa->pipe->surface_copy &&
        exa->copy.src == exa->copy.dst) {

       exa->copy.use_surface_copy = TRUE;
       
       exa->copy.src_surface =
          exa->scrn->get_tex_surface( exa->scrn,
                                      exa->copy.src->tex,
                                      0, 0, 0,
                                      PIPE_BUFFER_USAGE_GPU_READ);

       exa->copy.dst_surface =
          exa->scrn->get_tex_surface( exa->scrn, 
                                      exa->copy.dst->tex,
                                      0, 0, 0, 
                                      PIPE_BUFFER_USAGE_GPU_WRITE );
    }
    else {
       exa->copy.use_surface_copy = FALSE;

       if (exa->copy.dst == exa->copy.src)
          exa->copy.src_texture = renderer_clone_texture( exa->renderer,
                                                          exa->copy.src->tex );
       else
          pipe_texture_reference(&exa->copy.src_texture,
                                 exa->copy.src->tex);

       exa->copy.dst_surface =
          exa->scrn->get_tex_surface(exa->scrn,
                                     exa->copy.dst->tex,
                                     0, 0, 0,
                                     PIPE_BUFFER_USAGE_GPU_WRITE);


       renderer_copy_prepare(exa->renderer, 
                             exa->copy.dst_surface,
                             exa->copy.src_texture );
    }


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

#if DEBUG_PRINT
   debug_printf("\tExaCopy(srcx=%d, srcy=%d, dstX=%d, dstY=%d, w=%d, h=%d)\n",
                srcX, srcY, dstX, dstY, width, height);
#endif

   debug_assert(priv == exa->copy.dst);
   (void) priv;

   if (exa->copy.use_surface_copy) {
      /* XXX: consider exposing >1 box in surface_copy interface.
       */
      exa->pipe->surface_copy( exa->pipe,
                             exa->copy.dst_surface,
                             dstX, dstY,
                             exa->copy.src_surface,
                             srcX, srcY,
                             width, height );
   }
   else {
      renderer_copy_pixmap(exa->renderer, 
                           dstX, dstY,
                           srcX, srcY,
                           width, height,
                           exa->copy.src_texture->width0,
                           exa->copy.src_texture->height0);
   }
}

static void
ExaDoneCopy(PixmapPtr pPixmap)
{
    ScrnInfoPtr pScrn = xf86Screens[pPixmap->drawable.pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    struct exa_pixmap_priv *priv = exaGetPixmapDriverPrivate(pPixmap);
    struct exa_context *exa = ms->exa;

    if (!priv)
	return;

   renderer_draw_flush(exa->renderer);

   exa->copy.src = NULL;
   exa->copy.dst = NULL;
   pipe_surface_reference(&exa->copy.src_surface, NULL);
   pipe_surface_reference(&exa->copy.dst_surface, NULL);
   pipe_texture_reference(&exa->copy.src_texture, NULL);
}



static Bool
picture_check_formats(struct exa_pixmap_priv *pSrc, PicturePtr pSrcPicture)
{
   if (pSrc->picture_format == pSrcPicture->format)
      return TRUE;

   if (pSrc->picture_format != PICT_a8r8g8b8)
      return FALSE;

   /* pSrc->picture_format == PICT_a8r8g8b8 */
   switch (pSrcPicture->format) {
   case PICT_a8r8g8b8:
   case PICT_x8r8g8b8:
   case PICT_a8b8g8r8:
   case PICT_x8b8g8r8:
   /* just treat these two as x8... */
   case PICT_r8g8b8:
   case PICT_b8g8r8:
      return TRUE;
#ifdef PICT_TYPE_BGRA
   case PICT_b8g8r8a8:
   case PICT_b8g8r8x8:
      return FALSE; /* does not support swizzleing the alpha channel yet */
   case PICT_a2r10g10b10:
   case PICT_x2r10g10b10:
   case PICT_a2b10g10r10:
   case PICT_x2b10g10r10:
      return FALSE;
#endif
   default:
      return FALSE;
   }
   return FALSE;
}

/***********************************************************************
 * Composite entrypoints
 */

static Bool
ExaCheckComposite(int op,
		  PicturePtr pSrcPicture, PicturePtr pMaskPicture,
		  PicturePtr pDstPicture)
{
   ScrnInfoPtr pScrn = xf86Screens[pDstPicture->pDrawable->pScreen->myNum];
   modesettingPtr ms = modesettingPTR(pScrn);
   struct exa_context *exa = ms->exa;

#if DEBUG_PRINT
   debug_printf("ExaCheckComposite(%d, %p, %p, %p) = %d\n",
                op, pSrcPicture, pMaskPicture, pDstPicture, accelerated);
#endif

   if (!exa->accel)
       return FALSE;

   return xorg_composite_accelerated(op,
				     pSrcPicture,
				     pMaskPicture,
				     pDstPicture);
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

   if (!exa->accel)
       return FALSE;

#if DEBUG_PRINT
   debug_printf("ExaPrepareComposite(%d, src=0x%p, mask=0x%p, dst=0x%p)\n",
                op, pSrcPicture, pMaskPicture, pDstPicture);
   debug_printf("\tFormats: src(%s), mask(%s), dst(%s)\n",
                pSrcPicture ? render_format_name(pSrcPicture->format) : "none",
                pMaskPicture ? render_format_name(pMaskPicture->format) : "none",
                pDstPicture ? render_format_name(pDstPicture->format) : "none");
#endif
   if (!exa->pipe)
      XORG_FALLBACK("accle not enabled");

   priv = exaGetPixmapDriverPrivate(pDst);
   if (!priv || !priv->tex)
      XORG_FALLBACK("pDst %s", !priv ? "!priv" : "!priv->tex");

   if (!exa->scrn->is_format_supported(exa->scrn, priv->tex->format,
                                       priv->tex->target,
                                       PIPE_TEXTURE_USAGE_RENDER_TARGET, 0))
      XORG_FALLBACK("pDst format: %s", util_format_name(priv->tex->format));

   if (priv->picture_format != pDstPicture->format)
      XORG_FALLBACK("pDst pic_format: %s != %s",
                    render_format_name(priv->picture_format),
                    render_format_name(pDstPicture->format));

   if (pSrc) {
      priv = exaGetPixmapDriverPrivate(pSrc);
      if (!priv || !priv->tex)
         XORG_FALLBACK("pSrc %s", !priv ? "!priv" : "!priv->tex");

      if (!exa->scrn->is_format_supported(exa->scrn, priv->tex->format,
                                          priv->tex->target,
                                          PIPE_TEXTURE_USAGE_SAMPLER, 0))
         XORG_FALLBACK("pSrc format: %s", util_format_name(priv->tex->format));

      if (!picture_check_formats(priv, pSrcPicture))
         XORG_FALLBACK("pSrc pic_format: %s != %s",
                       render_format_name(priv->picture_format),
                       render_format_name(pSrcPicture->format));

   }

   if (pMask) {
      priv = exaGetPixmapDriverPrivate(pMask);
      if (!priv || !priv->tex)
         XORG_FALLBACK("pMask %s", !priv ? "!priv" : "!priv->tex");

      if (!exa->scrn->is_format_supported(exa->scrn, priv->tex->format,
                                          priv->tex->target,
                                          PIPE_TEXTURE_USAGE_SAMPLER, 0))
         XORG_FALLBACK("pMask format: %s", util_format_name(priv->tex->format));

      if (!picture_check_formats(priv, pMaskPicture))
         XORG_FALLBACK("pMask pic_format: %s != %s",
                       render_format_name(priv->picture_format),
                       render_format_name(pMaskPicture->format));
   }

   return xorg_composite_bind_state(exa, op, pSrcPicture, pMaskPicture,
                                    pDstPicture,
                                    pSrc ? exaGetPixmapDriverPrivate(pSrc) : NULL,
                                    pMask ? exaGetPixmapDriverPrivate(pMask) : NULL,
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

#if DEBUG_PRINT
   debug_printf("\tExaComposite(src[%d,%d], mask=[%d, %d], dst=[%d, %d], dim=[%d, %d])\n",
                srcX, srcY, maskX, maskY, dstX, dstY, width, height);
   debug_printf("\t   Num bound samplers = %d\n",
                exa->num_bound_samplers);
#endif

   xorg_composite(exa, priv, srcX, srcY, maskX, maskY,
                  dstX, dstY, width, height);
}



static void
ExaDoneComposite(PixmapPtr pPixmap)
{
   ScrnInfoPtr pScrn = xf86Screens[pPixmap->drawable.pScreen->myNum];
   modesettingPtr ms = modesettingPTR(pScrn);
   struct exa_context *exa = ms->exa;

   xorg_composite_done(exa);
}


/***********************************************************************
 * Pixmaps
 */

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



static Bool
size_match( int width, int tex_width )
{
#if ROUND_UP_TEXTURES
   if (width > tex_width)
      return FALSE;

   if (width * 2 < tex_width)
      return FALSE;

   return TRUE;
#else
   return width == tex_width;
#endif
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

    if (0) {
       debug_printf("%s pixmap %p sz %dx%dx%d devKind %d\n",
                    __FUNCTION__, pPixmap, width, height, bitsPerPixel, devKind);
       
       if (priv->tex)
          debug_printf("  ==> old texture %dx%d\n",
                       priv->tex->width0, 
                       priv->tex->height0);
    }


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

    priv->width = width;
    priv->height = height;

    /* Deal with screen resize */
    if ((exa->accel || priv->flags) &&
        (!priv->tex ||
         !size_match(width, priv->tex->width0) ||
         !size_match(height, priv->tex->height0) ||
         priv->tex_flags != priv->flags)) {
	struct pipe_texture *texture = NULL;
	struct pipe_texture template;

	memset(&template, 0, sizeof(template));
	template.target = PIPE_TEXTURE_2D;
	exa_get_pipe_format(depth, &template.format, &bitsPerPixel, &priv->picture_format);
        if (ROUND_UP_TEXTURES && priv->flags == 0) {
           template.width0 = util_next_power_of_two(width);
           template.height0 = util_next_power_of_two(height);
        }
        else {
           template.width0 = width;
           template.height0 = height;
        }

	template.depth0 = 1;
	template.last_level = 0;
	template.tex_usage = PIPE_TEXTURE_USAGE_RENDER_TARGET | priv->flags;
	priv->tex_flags = priv->flags;
	texture = exa->scrn->texture_create(exa->scrn, &template);

	if (priv->tex) {
	    struct pipe_surface *dst_surf;
	    struct pipe_surface *src_surf;

	    dst_surf = exa->scrn->get_tex_surface(
		exa->scrn, texture, 0, 0, 0, PIPE_BUFFER_USAGE_GPU_WRITE);
	    src_surf = xorg_gpu_surface(exa->pipe->screen, priv);
            if (exa->pipe->surface_copy) {
               exa->pipe->surface_copy(exa->pipe, dst_surf, 0, 0, src_surf,
                                       0, 0, min(width, texture->width0),
                                       min(height, texture->height0));
            } else {
               util_surface_copy(exa->pipe, FALSE, dst_surf, 0, 0, src_surf,
                                 0, 0, min(width, texture->width0),
                                 min(height, texture->height0));
            }
	    exa->scrn->tex_surface_destroy(dst_surf);
	    exa->scrn->tex_surface_destroy(src_surf);
	}

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

Bool
xorg_exa_set_texture(PixmapPtr pPixmap, struct  pipe_texture *tex)
{
    struct exa_pixmap_priv *priv = exaGetPixmapDriverPrivate(pPixmap);

    int mask = PIPE_TEXTURE_USAGE_PRIMARY | PIPE_TEXTURE_USAGE_DISPLAY_TARGET;

    if (!priv)
	return FALSE;

    if (pPixmap->drawable.width != tex->width0 ||
	pPixmap->drawable.height != tex->height0)
	return FALSE;

    pipe_texture_reference(&priv->tex, tex);
    priv->tex_flags = tex->tex_usage & mask;

    return TRUE;
}

struct pipe_texture *
xorg_exa_create_root_texture(ScrnInfoPtr pScrn,
			     int width, int height,
			     int depth, int bitsPerPixel)
{
    modesettingPtr ms = modesettingPTR(pScrn);
    struct exa_context *exa = ms->exa;
    struct pipe_texture template;
    int dummy;

    memset(&template, 0, sizeof(template));
    template.target = PIPE_TEXTURE_2D;
    exa_get_pipe_format(depth, &template.format, &bitsPerPixel, &dummy);
    template.width0 = width;
    template.height0 = height;
    template.depth0 = 1;
    template.last_level = 0;
    template.tex_usage |= PIPE_TEXTURE_USAGE_RENDER_TARGET;
    template.tex_usage |= PIPE_TEXTURE_USAGE_PRIMARY;
    template.tex_usage |= PIPE_TEXTURE_USAGE_DISPLAY_TARGET;

    return exa->scrn->texture_create(exa->scrn, &template);
}

void
xorg_exa_close(ScrnInfoPtr pScrn)
{
   modesettingPtr ms = modesettingPTR(pScrn);
   struct exa_context *exa = ms->exa;

   renderer_destroy(exa->renderer);

   if (exa->pipe)
      exa->pipe->destroy(exa->pipe);
   exa->pipe = NULL;
   /* Since this was shared be proper with the pointer */
   ms->ctx = NULL;

   exaDriverFini(pScrn->pScreen);
   xfree(exa);
   ms->exa = NULL;
}

void *
xorg_exa_init(ScrnInfoPtr pScrn, Bool accel)
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
   pExa->DoneSolid          = ExaDoneSolid;
   pExa->PrepareCopy        = ExaPrepareCopy;
   pExa->Copy               = ExaCopy;
   pExa->DoneCopy           = ExaDoneCopy;
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
   exa->pipe = exa->scrn->context_create(exa->scrn, NULL);
   if (exa->pipe == NULL)
      goto out_err;

   /* Share context with DRI */
   ms->ctx = exa->pipe;

   exa->renderer = renderer_create(exa->pipe);
   exa->accel = accel;

   return (void *)exa;

out_err:
   xorg_exa_close(pScrn);

   return NULL;
}

struct pipe_surface *
xorg_gpu_surface(struct pipe_screen *scrn, struct exa_pixmap_priv *priv)
{
   return scrn->get_tex_surface(scrn, priv->tex, 0, 0, 0,
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


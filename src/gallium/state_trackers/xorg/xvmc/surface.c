/**************************************************************************
 *
 * Copyright 2009 Younes Manton.
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
 **************************************************************************/

#include <assert.h>
#include <X11/Xlibint.h>
#include <pipe/p_video_context.h>
#include <pipe/p_video_state.h>
#include <pipe/p_state.h>
#include <util/u_memory.h>
#include "xvmc_private.h"

static enum pipe_mpeg12_macroblock_type TypeToPipe(int xvmc_mb_type)
{
   if (xvmc_mb_type & XVMC_MB_TYPE_INTRA)
      return PIPE_MPEG12_MACROBLOCK_TYPE_INTRA;
   if ((xvmc_mb_type & (XVMC_MB_TYPE_MOTION_FORWARD | XVMC_MB_TYPE_MOTION_BACKWARD)) == XVMC_MB_TYPE_MOTION_FORWARD)
      return PIPE_MPEG12_MACROBLOCK_TYPE_FWD;
   if ((xvmc_mb_type & (XVMC_MB_TYPE_MOTION_FORWARD | XVMC_MB_TYPE_MOTION_BACKWARD)) == XVMC_MB_TYPE_MOTION_BACKWARD)
      return PIPE_MPEG12_MACROBLOCK_TYPE_BKWD;
   if ((xvmc_mb_type & (XVMC_MB_TYPE_MOTION_FORWARD | XVMC_MB_TYPE_MOTION_BACKWARD)) == (XVMC_MB_TYPE_MOTION_FORWARD | XVMC_MB_TYPE_MOTION_BACKWARD))
      return PIPE_MPEG12_MACROBLOCK_TYPE_BI;

   assert(0);

   return -1;
}

static enum pipe_mpeg12_picture_type PictureToPipe(int xvmc_pic)
{
   switch (xvmc_pic) {
      case XVMC_TOP_FIELD:
         return PIPE_MPEG12_PICTURE_TYPE_FIELD_TOP;
      case XVMC_BOTTOM_FIELD:
         return PIPE_MPEG12_PICTURE_TYPE_FIELD_BOTTOM;
      case XVMC_FRAME_PICTURE:
         return PIPE_MPEG12_PICTURE_TYPE_FRAME;
      default:
         assert(0);
   }

   return -1;
}

static enum pipe_mpeg12_motion_type MotionToPipe(int xvmc_motion_type, int xvmc_dct_type)
{
   switch (xvmc_motion_type) {
      case XVMC_PREDICTION_FRAME:
         return xvmc_dct_type == XVMC_DCT_TYPE_FIELD ?
            PIPE_MPEG12_MOTION_TYPE_16x8 : PIPE_MPEG12_MOTION_TYPE_FRAME;
      case XVMC_PREDICTION_FIELD:
         return PIPE_MPEG12_MOTION_TYPE_FIELD;
      case XVMC_PREDICTION_DUAL_PRIME:
         return PIPE_MPEG12_MOTION_TYPE_DUALPRIME;
      default:
         assert(0);
   }

   return -1;
}

static bool
CreateOrResizeBackBuffer(struct pipe_video_context *vpipe, unsigned int width, unsigned int height,
                         struct pipe_surface **backbuffer)
{
   struct pipe_texture template;
   struct pipe_texture *tex;

   assert(vpipe);

   if (*backbuffer) {
      if ((*backbuffer)->width != width || (*backbuffer)->height != height)
         pipe_surface_reference(backbuffer, NULL);
      else
         return true;
   }

   memset(&template, 0, sizeof(struct pipe_texture));
   template.target = PIPE_TEXTURE_2D;
   /* XXX: Needs to match the drawable's format? */
   template.format = PIPE_FORMAT_B8G8R8X8_UNORM;
   template.last_level = 0;
   template.width0 = width;
   template.height0 = height;
   template.depth0 = 1;
   template.tex_usage = PIPE_TEXTURE_USAGE_DISPLAY_TARGET;

   tex = vpipe->screen->texture_create(vpipe->screen, &template);
   if (!tex)
      return false;

   *backbuffer = vpipe->screen->get_tex_surface(vpipe->screen, tex, 0, 0, 0,
                                                PIPE_BUFFER_USAGE_GPU_READ |
                                                PIPE_BUFFER_USAGE_GPU_WRITE);
   pipe_texture_reference(&tex, NULL);

   if (!*backbuffer)
      return false;

   /* Clear the backbuffer in case the video doesn't cover the whole window */
   /* FIXME: Need to clear every time a frame moves and leaves dirty rects */
   vpipe->clear_surface(vpipe, 0, 0, width, height, 0, *backbuffer);

   return true;
}

static void
MacroBlocksToPipe(const XvMCMacroBlockArray *xvmc_macroblocks,
                  const XvMCBlockArray *xvmc_blocks,
                  unsigned int first_macroblock,
                  unsigned int num_macroblocks,
                  struct pipe_mpeg12_macroblock *pipe_macroblocks)
{
   unsigned int i, j, k, l;
   XvMCMacroBlock *xvmc_mb;

   assert(xvmc_macroblocks);
   assert(xvmc_blocks);
   assert(pipe_macroblocks);
   assert(num_macroblocks);

   xvmc_mb = xvmc_macroblocks->macro_blocks + first_macroblock;

   for (i = 0; i < num_macroblocks; ++i) {
      pipe_macroblocks->base.codec = PIPE_VIDEO_CODEC_MPEG12;
      pipe_macroblocks->mbx = xvmc_mb->x;
      pipe_macroblocks->mby = xvmc_mb->y;
      pipe_macroblocks->mb_type = TypeToPipe(xvmc_mb->macroblock_type);
      if (pipe_macroblocks->mb_type != PIPE_MPEG12_MACROBLOCK_TYPE_INTRA)
         pipe_macroblocks->mo_type = MotionToPipe(xvmc_mb->motion_type, xvmc_mb->dct_type);
      /* Get rid of Valgrind 'undefined' warnings */
      else
         pipe_macroblocks->mo_type = -1;
      pipe_macroblocks->dct_type = xvmc_mb->dct_type == XVMC_DCT_TYPE_FIELD ?
         PIPE_MPEG12_DCT_TYPE_FIELD : PIPE_MPEG12_DCT_TYPE_FRAME;

      for (j = 0; j < 2; ++j)
         for (k = 0; k < 2; ++k)
            for (l = 0; l < 2; ++l)
               pipe_macroblocks->pmv[j][k][l] = xvmc_mb->PMV[j][k][l];

      pipe_macroblocks->cbp = xvmc_mb->coded_block_pattern;
      pipe_macroblocks->blocks = xvmc_blocks->blocks + xvmc_mb->index * BLOCK_SIZE_SAMPLES;

      ++pipe_macroblocks;
      ++xvmc_mb;
   }
}

Status XvMCCreateSurface(Display *dpy, XvMCContext *context, XvMCSurface *surface)
{
   XvMCContextPrivate *context_priv;
   struct pipe_video_context *vpipe;
   XvMCSurfacePrivate *surface_priv;
   struct pipe_video_surface *vsfc;

   assert(dpy);

   if (!context)
      return XvMCBadContext;
   if (!surface)
      return XvMCBadSurface;

   context_priv = context->privData;
   vpipe = context_priv->vpipe;

   surface_priv = CALLOC(1, sizeof(XvMCSurfacePrivate));
   if (!surface_priv)
      return BadAlloc;

   vsfc = vpipe->screen->video_surface_create(vpipe->screen, vpipe->chroma_format,
                                              vpipe->width, vpipe->height);
   if (!vsfc) {
      FREE(surface_priv);
      return BadAlloc;
   }

   surface_priv->pipe_vsfc = vsfc;
   surface_priv->context = context;

   surface->surface_id = XAllocID(dpy);
   surface->context_id = context->context_id;
   surface->surface_type_id = context->surface_type_id;
   surface->width = context->width;
   surface->height = context->height;
   surface->privData = surface_priv;

   SyncHandle();

   return Success;
}

Status XvMCRenderSurface(Display *dpy, XvMCContext *context, unsigned int picture_structure,
                         XvMCSurface *target_surface, XvMCSurface *past_surface, XvMCSurface *future_surface,
                         unsigned int flags, unsigned int num_macroblocks, unsigned int first_macroblock,
                         XvMCMacroBlockArray *macroblocks, XvMCBlockArray *blocks
)
{
   struct pipe_video_context *vpipe;
   struct pipe_surface *t_vsfc;
   struct pipe_surface *p_vsfc;
   struct pipe_surface *f_vsfc;
   XvMCContextPrivate *context_priv;
   XvMCSurfacePrivate *target_surface_priv;
   XvMCSurfacePrivate *past_surface_priv;
   XvMCSurfacePrivate *future_surface_priv;
   struct pipe_mpeg12_macroblock pipe_macroblocks[num_macroblocks];

   assert(dpy);

   if (!context || !context->privData)
      return XvMCBadContext;
   if (!target_surface || !target_surface->privData)
      return XvMCBadSurface;

   if (picture_structure != XVMC_TOP_FIELD &&
       picture_structure != XVMC_BOTTOM_FIELD &&
       picture_structure != XVMC_FRAME_PICTURE)
      return BadValue;
   /* Bkwd pred equivalent to fwd (past && !future) */
   if (future_surface && !past_surface)
      return BadMatch;

   assert(context->context_id == target_surface->context_id);
   assert(!past_surface || context->context_id == past_surface->context_id);
   assert(!future_surface || context->context_id == future_surface->context_id);

   assert(macroblocks);
   assert(blocks);

   assert(macroblocks->context_id == context->context_id);
   assert(blocks->context_id == context->context_id);

   assert(flags == 0 || flags == XVMC_SECOND_FIELD);

   target_surface_priv = target_surface->privData;
   past_surface_priv = past_surface ? past_surface->privData : NULL;
   future_surface_priv = future_surface ? future_surface->privData : NULL;

   assert(target_surface_priv->context == context);
   assert(!past_surface || past_surface_priv->context == context);
   assert(!future_surface || future_surface_priv->context == context);

   context_priv = context->privData;
   vpipe = context_priv->vpipe;

   t_vsfc = target_surface_priv->pipe_vsfc;
   p_vsfc = past_surface ? past_surface_priv->pipe_vsfc : NULL;
   f_vsfc = future_surface ? future_surface_priv->pipe_vsfc : NULL;

   MacroBlocksToPipe(macroblocks, blocks, first_macroblock,
                     num_macroblocks, pipe_macroblocks);

   vpipe->set_decode_target(vpipe, t_vsfc);
   vpipe->decode_macroblocks(vpipe, p_vsfc, f_vsfc, num_macroblocks,
                             &pipe_macroblocks->base, target_surface_priv->render_fence);

   return Success;
}

Status XvMCFlushSurface(Display *dpy, XvMCSurface *surface)
{
   assert(dpy);

   if (!surface)
      return XvMCBadSurface;

   return Success;
}

Status XvMCSyncSurface(Display *dpy, XvMCSurface *surface)
{
   assert(dpy);

   if (!surface)
      return XvMCBadSurface;

   return Success;
}

Status XvMCPutSurface(Display *dpy, XvMCSurface *surface, Drawable drawable,
                      short srcx, short srcy, unsigned short srcw, unsigned short srch,
                      short destx, short desty, unsigned short destw, unsigned short desth,
                      int flags)
{
   Window root;
   int x, y;
   unsigned int width, height;
   unsigned int border_width;
   unsigned int depth;
   struct pipe_video_context *vpipe;
   XvMCSurfacePrivate *surface_priv;
   XvMCContextPrivate *context_priv;
   XvMCContext *context;
   struct pipe_video_rect src_rect = {srcx, srcy, srcw, srch};
   struct pipe_video_rect dst_rect = {destx, desty, destw, desth};

   assert(dpy);

   if (!surface || !surface->privData)
      return XvMCBadSurface;

   if (XGetGeometry(dpy, drawable, &root, &x, &y, &width, &height, &border_width, &depth) == BadDrawable)
      return BadDrawable;

   assert(flags == XVMC_TOP_FIELD || flags == XVMC_BOTTOM_FIELD || flags == XVMC_FRAME_PICTURE);
   assert(srcx + srcw - 1 < surface->width);
   assert(srcy + srch - 1 < surface->height);
   /*
    * Some apps (mplayer) hit these asserts because they call
    * this function after the window has been resized by the WM
    * but before they've handled the corresponding XEvent and
    * know about the new dimensions. The output should be clipped
    * until the app updates destw and desth.
    */
   /*
   assert(destx + destw - 1 < width);
   assert(desty + desth - 1 < height);
    */

   surface_priv = surface->privData;
   context = surface_priv->context;
   context_priv = context->privData;
   vpipe = context_priv->vpipe;

   if (!CreateOrResizeBackBuffer(vpipe, width, height, &context_priv->backbuffer))
      return BadAlloc;

   vpipe->render_picture(vpipe, surface_priv->pipe_vsfc, PictureToPipe(flags), &src_rect,
                         context_priv->backbuffer, &dst_rect, surface_priv->disp_fence);

   vl_video_bind_drawable(vpipe, drawable);
	
   vpipe->screen->flush_frontbuffer
   (
      vpipe->screen,
      context_priv->backbuffer,
      vpipe->priv
   );

   return Success;
}

Status XvMCGetSurfaceStatus(Display *dpy, XvMCSurface *surface, int *status)
{
   assert(dpy);

   if (!surface)
      return XvMCBadSurface;

   assert(status);

   *status = 0;

   return Success;
}

Status XvMCDestroySurface(Display *dpy, XvMCSurface *surface)
{
   XvMCSurfacePrivate *surface_priv;

   assert(dpy);

   if (!surface || !surface->privData)
      return XvMCBadSurface;

   surface_priv = surface->privData;
   pipe_video_surface_reference(&surface_priv->pipe_vsfc, NULL);
   FREE(surface_priv);
   surface->privData = NULL;

   return Success;
}

Status XvMCHideSurface(Display *dpy, XvMCSurface *surface)
{
   assert(dpy);

   if (!surface || !surface->privData)
      return XvMCBadSurface;

   /* No op, only for overlaid rendering */

   return Success;
}

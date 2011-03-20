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
#include <stdio.h>
#include <X11/Xlibint.h>
#include <vl_winsys.h>
#include <pipe/p_video_context.h>
#include <pipe/p_video_state.h>
#include <pipe/p_state.h>
#include <util/u_inlines.h>
#include <util/u_memory.h>
#include <util/u_math.h>
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

   XVMC_MSG(XVMC_ERR, "[XvMC] Unrecognized mb type 0x%08X.\n", xvmc_mb_type);

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

   XVMC_MSG(XVMC_ERR, "[XvMC] Unrecognized picture type 0x%08X.\n", xvmc_pic);

   return -1;
}

static enum pipe_mpeg12_motion_type MotionToPipe(int xvmc_motion_type, unsigned int xvmc_picture_structure)
{
   switch (xvmc_motion_type) {
      case XVMC_PREDICTION_FRAME:
         if (xvmc_picture_structure == XVMC_FRAME_PICTURE)
            return PIPE_MPEG12_MOTION_TYPE_FRAME;
         else
            return PIPE_MPEG12_MOTION_TYPE_16x8;
         break;
      case XVMC_PREDICTION_FIELD:
         return PIPE_MPEG12_MOTION_TYPE_FIELD;
      case XVMC_PREDICTION_DUAL_PRIME:
         return PIPE_MPEG12_MOTION_TYPE_DUALPRIME;
      default:
         assert(0);
   }

   XVMC_MSG(XVMC_ERR, "[XvMC] Unrecognized motion type 0x%08X (with picture structure 0x%08X).\n", xvmc_motion_type, xvmc_picture_structure);

   return -1;
}

#if 0
static bool
CreateOrResizeBackBuffer(struct vl_context *vctx, unsigned int width, unsigned int height,
                         struct pipe_surface **backbuffer)
{
   struct pipe_video_context *vpipe;
   struct pipe_resource template;
   struct pipe_resource *tex;

   assert(vctx);

   vpipe = vctx->vpipe;

   if (*backbuffer) {
      if ((*backbuffer)->width != width || (*backbuffer)->height != height)
         pipe_surface_reference(backbuffer, NULL);
      else
         return true;
   }

   memset(&template, 0, sizeof(struct pipe_resource));
   template.target = PIPE_TEXTURE_2D;
   template.format = vctx->vscreen->format;
   template.last_level = 0;
   template.width0 = width;
   template.height0 = height;
   template.depth0 = 1;
   template.array_size = 1;
   template.usage = PIPE_USAGE_DEFAULT;
   template.bind = PIPE_BIND_RENDER_TARGET | PIPE_BIND_DISPLAY_TARGET | PIPE_BIND_BLIT_SOURCE;
   template.flags = 0;

   tex = vpipe->screen->resource_create(vpipe->screen, &template);
   if (!tex)
      return false;

   *backbuffer = vpipe->screen->get_tex_surface(vpipe->screen, tex, 0, 0, 0,
                                                template.bind);
   pipe_resource_reference(&tex, NULL);

   if (!*backbuffer)
      return false;

   /* Clear the backbuffer in case the video doesn't cover the whole window */
   /* FIXME: Need to clear every time a frame moves and leaves dirty rects */
   vpipe->surface_fill(vpipe, *backbuffer, 0, 0, width, height, 0);

   return true;
}
#endif

static void
MacroBlocksToPipe(struct pipe_screen *screen,
                  unsigned int xvmc_picture_structure,
                  const XvMCMacroBlockArray *xvmc_macroblocks,
                  const XvMCBlockArray *xvmc_blocks,
                  unsigned int first_macroblock,
                  unsigned int num_macroblocks,
                  struct pipe_mpeg12_macroblock *mb)
{
   unsigned int i, j, k, l;
   XvMCMacroBlock *xvmc_mb;

   assert(xvmc_macroblocks);
   assert(xvmc_blocks);
   assert(mb);
   assert(num_macroblocks);

   xvmc_mb = xvmc_macroblocks->macro_blocks + first_macroblock;

   for (i = 0; i < num_macroblocks; ++i) {
      mb->base.codec = PIPE_VIDEO_CODEC_MPEG12;
      mb->mbx = xvmc_mb->x;
      mb->mby = xvmc_mb->y;
      mb->mb_type = TypeToPipe(xvmc_mb->macroblock_type);
      if (mb->mb_type != PIPE_MPEG12_MACROBLOCK_TYPE_INTRA)
         mb->mo_type = MotionToPipe(xvmc_mb->motion_type, xvmc_picture_structure);
      /* Get rid of Valgrind 'undefined' warnings */
      else
         mb->mo_type = -1;
      mb->dct_type = xvmc_mb->dct_type == XVMC_DCT_TYPE_FIELD ?
         PIPE_MPEG12_DCT_TYPE_FIELD : PIPE_MPEG12_DCT_TYPE_FRAME;

      for (j = 0; j < 2; ++j) {
         mb->mv[j].top.x = xvmc_mb->PMV[0][j][0];
         mb->mv[j].top.y = xvmc_mb->PMV[0][j][1];
         mb->mv[j].bottom.x = xvmc_mb->PMV[1][j][0];
         mb->mv[j].bottom.y = xvmc_mb->PMV[1][j][1];
      }

      mb->mv[0].top.field_select = xvmc_mb->motion_vertical_field_select & XVMC_SELECT_FIRST_FORWARD;
      mb->mv[1].top.field_select = xvmc_mb->motion_vertical_field_select & XVMC_SELECT_FIRST_BACKWARD;
      mb->mv[0].bottom.field_select = xvmc_mb->motion_vertical_field_select & XVMC_SELECT_SECOND_FORWARD;
      mb->mv[1].bottom.field_select = xvmc_mb->motion_vertical_field_select & XVMC_SELECT_SECOND_BACKWARD;

      mb->cbp = xvmc_mb->coded_block_pattern;
      mb->blocks = xvmc_blocks->blocks + xvmc_mb->index * BLOCK_SIZE_SAMPLES;

      ++mb;
      ++xvmc_mb;
   }
}

PUBLIC
Status XvMCCreateSurface(Display *dpy, XvMCContext *context, XvMCSurface *surface)
{
   XvMCContextPrivate *context_priv;
   struct pipe_video_context *vpipe;
   XvMCSurfacePrivate *surface_priv;
   struct pipe_resource template;
   struct pipe_resource *vsfc_tex;
   struct pipe_surface surf_template;
   struct pipe_surface *vsfc;

   XVMC_MSG(XVMC_TRACE, "[XvMC] Creating surface %p.\n", surface);

   assert(dpy);

   if (!context)
      return XvMCBadContext;
   if (!surface)
      return XvMCBadSurface;

   context_priv = context->privData;
   vpipe = context_priv->vctx->vpipe;

   surface_priv = CALLOC(1, sizeof(XvMCSurfacePrivate));
   if (!surface_priv)
      return BadAlloc;

   memset(&template, 0, sizeof(struct pipe_resource));
   template.target = PIPE_TEXTURE_2D;
   template.format = (enum pipe_format)vpipe->get_param(vpipe, PIPE_CAP_DECODE_TARGET_PREFERRED_FORMAT);
   if (!vpipe->is_format_supported(vpipe, template.format,
                                   PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET)) {
      FREE(surface_priv);
      return BadAlloc;
   }
   template.last_level = 0;
   if (vpipe->get_param(vpipe, PIPE_CAP_NPOT_TEXTURES)) {
      template.width0 = context->width;
      template.height0 = context->height;
   }
   else {
      template.width0 = util_next_power_of_two(context->width);
      template.height0 = util_next_power_of_two(context->height);
   }
   template.depth0 = 1;
   template.array_size = 1;
   template.usage = PIPE_USAGE_DEFAULT;
   template.bind = PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET;
   template.flags = 0;
   vsfc_tex = vpipe->screen->resource_create(vpipe->screen, &template);
   if (!vsfc_tex) {
      FREE(surface_priv);
      return BadAlloc;
   }

   memset(&surf_template, 0, sizeof(surf_template));
   surf_template.format = vsfc_tex->format;
   surf_template.usage = PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET;
   vsfc = vpipe->create_surface(vpipe, vsfc_tex, &surf_template);
   pipe_resource_reference(&vsfc_tex, NULL);
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

   XVMC_MSG(XVMC_TRACE, "[XvMC] Surface %p created.\n", surface);

   return Success;
}

PUBLIC
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

   XVMC_MSG(XVMC_TRACE, "[XvMC] Rendering to surface %p.\n", target_surface);

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
   vpipe = context_priv->vctx->vpipe;

   t_vsfc = target_surface_priv->pipe_vsfc;
   p_vsfc = past_surface ? past_surface_priv->pipe_vsfc : NULL;
   f_vsfc = future_surface ? future_surface_priv->pipe_vsfc : NULL;

   MacroBlocksToPipe(vpipe->screen, picture_structure, macroblocks, blocks, first_macroblock,
                     num_macroblocks, pipe_macroblocks);

   vpipe->set_decode_target(vpipe, t_vsfc);
   vpipe->decode_macroblocks(vpipe, p_vsfc, f_vsfc, num_macroblocks,
                             &pipe_macroblocks->base, &target_surface_priv->render_fence);

   XVMC_MSG(XVMC_TRACE, "[XvMC] Submitted surface %p for rendering.\n", target_surface);

   return Success;
}

PUBLIC
Status XvMCFlushSurface(Display *dpy, XvMCSurface *surface)
{
   assert(dpy);

   if (!surface)
      return XvMCBadSurface;

   return Success;
}

PUBLIC
Status XvMCSyncSurface(Display *dpy, XvMCSurface *surface)
{
   assert(dpy);

   if (!surface)
      return XvMCBadSurface;

   return Success;
}

PUBLIC
Status XvMCPutSurface(Display *dpy, XvMCSurface *surface, Drawable drawable,
                      short srcx, short srcy, unsigned short srcw, unsigned short srch,
                      short destx, short desty, unsigned short destw, unsigned short desth,
                      int flags)
{
   static int dump_window = -1;

   struct pipe_video_context *vpipe;
   XvMCSurfacePrivate *surface_priv;
   XvMCContextPrivate *context_priv;
   XvMCSubpicturePrivate *subpicture_priv;
   XvMCContext *context;
   struct pipe_video_rect src_rect = {srcx, srcy, srcw, srch};
   struct pipe_video_rect dst_rect = {destx, desty, destw, desth};
   struct pipe_surface *drawable_surface;

   XVMC_MSG(XVMC_TRACE, "[XvMC] Displaying surface %p.\n", surface);

   assert(dpy);

   if (!surface || !surface->privData)
      return XvMCBadSurface;

   surface_priv = surface->privData;
   context = surface_priv->context;
   context_priv = context->privData;

   drawable_surface = vl_drawable_surface_get(context_priv->vctx, drawable);
   if (!drawable_surface)
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
   assert(destx + destw - 1 < drawable_surface->width);
   assert(desty + desth - 1 < drawable_surface->height);
    */

   subpicture_priv = surface_priv->subpicture ? surface_priv->subpicture->privData : NULL;
   vpipe = context_priv->vctx->vpipe;

#if 0
   if (!CreateOrResizeBackBuffer(context_priv->vctx, width, height, &context_priv->backbuffer))
      return BadAlloc;
#endif

   if (subpicture_priv) {
      struct pipe_video_rect src_rect = {surface_priv->subx, surface_priv->suby, surface_priv->subw, surface_priv->subh};
      struct pipe_video_rect dst_rect = {surface_priv->surfx, surface_priv->surfy, surface_priv->surfw, surface_priv->surfh};
      struct pipe_video_rect *src_rects[1] = {&src_rect};
      struct pipe_video_rect *dst_rects[1] = {&dst_rect};

      XVMC_MSG(XVMC_TRACE, "[XvMC] Surface %p has subpicture %p.\n", surface, surface_priv->subpicture);

      assert(subpicture_priv->surface == surface);
      vpipe->set_picture_layers(vpipe, &subpicture_priv->sfc, src_rects, dst_rects, 1);

      surface_priv->subpicture = NULL;
      subpicture_priv->surface = NULL;
   }
   else
      vpipe->set_picture_layers(vpipe, NULL, NULL, NULL, 0);

   vpipe->render_picture(vpipe, surface_priv->pipe_vsfc, PictureToPipe(flags), &src_rect,
                         drawable_surface, &dst_rect, &surface_priv->disp_fence);

   XVMC_MSG(XVMC_TRACE, "[XvMC] Submitted surface %p for display. Pushing to front buffer.\n", surface);

   vpipe->screen->flush_frontbuffer
   (
      vpipe->screen,
      drawable_surface->texture,
      0, 0,
      vl_contextprivate_get(context_priv->vctx, drawable_surface)
   );

   pipe_surface_reference(&drawable_surface, NULL);

   if(dump_window == -1) {
      dump_window = debug_get_num_option("XVMC_DUMP", 0);
   }

   if(dump_window) {
      static unsigned int framenum = 0;
      char cmd[256];
      sprintf(cmd, "xwd -id %d -out xvmc_frame_%08d.xwd", (int)drawable, ++framenum);
      system(cmd);
   }

   XVMC_MSG(XVMC_TRACE, "[XvMC] Pushed surface %p to front buffer.\n", surface);

   return Success;
}

PUBLIC
Status XvMCGetSurfaceStatus(Display *dpy, XvMCSurface *surface, int *status)
{
   assert(dpy);

   if (!surface)
      return XvMCBadSurface;

   assert(status);

   *status = 0;

   return Success;
}

PUBLIC
Status XvMCDestroySurface(Display *dpy, XvMCSurface *surface)
{
   XvMCSurfacePrivate *surface_priv;

   XVMC_MSG(XVMC_TRACE, "[XvMC] Destroying surface %p.\n", surface);

   assert(dpy);

   if (!surface || !surface->privData)
      return XvMCBadSurface;

   surface_priv = surface->privData;
   pipe_surface_reference(&surface_priv->pipe_vsfc, NULL);
   FREE(surface_priv);
   surface->privData = NULL;

   XVMC_MSG(XVMC_TRACE, "[XvMC] Surface %p destroyed.\n", surface);

   return Success;
}

PUBLIC
Status XvMCHideSurface(Display *dpy, XvMCSurface *surface)
{
   assert(dpy);

   if (!surface || !surface->privData)
      return XvMCBadSurface;

   /* No op, only for overlaid rendering */

   return Success;
}

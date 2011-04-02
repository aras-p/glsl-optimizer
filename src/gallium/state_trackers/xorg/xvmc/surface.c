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

static void
MacroBlocksToPipe(struct pipe_screen *screen,
                  unsigned int xvmc_picture_structure,
                  const XvMCMacroBlockArray *xvmc_macroblocks,
                  const XvMCBlockArray *xvmc_blocks,
                  unsigned int first_macroblock,
                  unsigned int num_macroblocks,
                  struct pipe_mpeg12_macroblock *mb)
{
   unsigned int i, j;
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

static void
unmap_and_flush_surface(XvMCSurfacePrivate *surface)
{
   struct pipe_video_buffer *ref_frames[2];
   unsigned i;

   assert(surface);

   for ( i = 0; i < 2; ++i ) {
      if (surface->ref_surfaces[i]) {
         XvMCSurfacePrivate *ref = surface->ref_surfaces[i]->privData;

         assert(ref);

         unmap_and_flush_surface(ref);
         surface->ref_surfaces[i] = NULL;
         ref_frames[i] = ref->pipe_buffer;
      } else {
         ref_frames[i] = NULL;
      }
   }

   if (surface->mapped) {
      surface->pipe_buffer->unmap(surface->pipe_buffer);
      surface->pipe_buffer->flush(surface->pipe_buffer,
                                  ref_frames,
                                  &surface->flush_fence);
      surface->mapped = 0;
   }
}

PUBLIC
Status XvMCCreateSurface(Display *dpy, XvMCContext *context, XvMCSurface *surface)
{
   XvMCContextPrivate *context_priv;
   struct pipe_video_context *vpipe;
   XvMCSurfacePrivate *surface_priv;

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

   surface_priv->pipe_buffer = vpipe->create_buffer(vpipe);
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
   struct pipe_video_buffer *t_buffer;
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

   t_buffer = target_surface_priv->pipe_buffer;

   // enshure that all reference frames are flushed
   // not really nessasary, but speeds ups rendering
   if (past_surface)
      unmap_and_flush_surface(past_surface->privData);

   if (future_surface)
      unmap_and_flush_surface(future_surface->privData);

   /* If the surface we're rendering hasn't changed the ref frames shouldn't change. */
   if (target_surface_priv->mapped && (
       target_surface_priv->ref_surfaces[0] != past_surface ||
       target_surface_priv->ref_surfaces[1] != future_surface)) {

      // If they change anyway we need to flush our surface
      unmap_and_flush_surface(target_surface_priv);
   }

   MacroBlocksToPipe(vpipe->screen, picture_structure, macroblocks, blocks, first_macroblock,
                     num_macroblocks, pipe_macroblocks);

   if (!target_surface_priv->mapped) {
      t_buffer->map(t_buffer);
      target_surface_priv->ref_surfaces[0] = past_surface;
      target_surface_priv->ref_surfaces[1] = future_surface;
      target_surface_priv->mapped = 1;
   }

   t_buffer->add_macroblocks(t_buffer, num_macroblocks, &pipe_macroblocks->base);

   XVMC_MSG(XVMC_TRACE, "[XvMC] Submitted surface %p for rendering.\n", target_surface);

   return Success;
}

PUBLIC
Status XvMCFlushSurface(Display *dpy, XvMCSurface *surface)
{
   assert(dpy);

   if (!surface)
      return XvMCBadSurface;

   // don't call flush here, because this is usually
   // called once for every slice instead of every frame

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

   if (subpicture_priv) {
      struct pipe_video_rect src_rect = {surface_priv->subx, surface_priv->suby, surface_priv->subw, surface_priv->subh};
      struct pipe_video_rect dst_rect = {surface_priv->surfx, surface_priv->surfy, surface_priv->surfw, surface_priv->surfh};
      struct pipe_video_rect *src_rects[1] = {&src_rect};
      struct pipe_video_rect *dst_rects[1] = {&dst_rect};

      XVMC_MSG(XVMC_TRACE, "[XvMC] Surface %p has subpicture %p.\n", surface, surface_priv->subpicture);

      assert(subpicture_priv->surface == surface);
      vpipe->set_picture_layers(vpipe, &subpicture_priv->sampler, &subpicture_priv->palette, src_rects, dst_rects, 1);

      surface_priv->subpicture = NULL;
      subpicture_priv->surface = NULL;
   }
   else
      vpipe->set_picture_layers(vpipe, NULL, NULL, NULL, NULL, 0);

   unmap_and_flush_surface(surface_priv);
   vpipe->render_picture(vpipe, surface_priv->pipe_buffer, &src_rect, PictureToPipe(flags),
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
   surface_priv->pipe_buffer->destroy(surface_priv->pipe_buffer);
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

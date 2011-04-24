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

#include <pipe/p_video_context.h>
#include <pipe/p_video_state.h>
#include <pipe/p_state.h>

#include <util/u_inlines.h>
#include <util/u_memory.h>
#include <util/u_math.h>

#include <vl_winsys.h>

#include "xvmc_private.h"

static const unsigned const_empty_block_mask_420[3][2][2] = {
   { { 0x20, 0x10 },  { 0x08, 0x04 } },
   { { 0x02, 0x02 },  { 0x02, 0x02 } },
   { { 0x01, 0x01 },  { 0x01, 0x01 } }
};

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

static inline void
MacroBlockTypeToPipeWeights(const XvMCMacroBlock *xvmc_mb, unsigned weights[2])
{
   assert(xvmc_mb);

   switch (xvmc_mb->macroblock_type & (XVMC_MB_TYPE_MOTION_FORWARD | XVMC_MB_TYPE_MOTION_BACKWARD)) {
   case XVMC_MB_TYPE_MOTION_FORWARD:
      weights[0] = PIPE_VIDEO_MV_WEIGHT_MAX;
      weights[1] = PIPE_VIDEO_MV_WEIGHT_MIN;
      break;

   case (XVMC_MB_TYPE_MOTION_FORWARD | XVMC_MB_TYPE_MOTION_BACKWARD):
      weights[0] = PIPE_VIDEO_MV_WEIGHT_HALF;
      weights[1] = PIPE_VIDEO_MV_WEIGHT_HALF;
      break;

   case XVMC_MB_TYPE_MOTION_BACKWARD:
      weights[0] = PIPE_VIDEO_MV_WEIGHT_MIN;
      weights[1] = PIPE_VIDEO_MV_WEIGHT_MAX;
      break;

   default:
      /* workaround for xines xxmc video out plugin */
      if (!(xvmc_mb->macroblock_type & ~XVMC_MB_TYPE_PATTERN)) {
         weights[0] = PIPE_VIDEO_MV_WEIGHT_MAX;
         weights[1] = PIPE_VIDEO_MV_WEIGHT_MIN;
      } else {
         weights[0] = PIPE_VIDEO_MV_WEIGHT_MIN;
         weights[1] = PIPE_VIDEO_MV_WEIGHT_MIN;
      }
      break;
   }
}

static inline struct pipe_motionvector
MotionVectorToPipe(const XvMCMacroBlock *xvmc_mb, unsigned vector,
                   unsigned field_select_mask, unsigned weight)
{
   struct pipe_motionvector mv;

   assert(xvmc_mb);

   switch (xvmc_mb->motion_type) {
   case XVMC_PREDICTION_FRAME:
      mv.top.x = xvmc_mb->PMV[0][vector][0];
      mv.top.y = xvmc_mb->PMV[0][vector][1];
      mv.top.field_select = PIPE_VIDEO_FRAME;
      mv.top.weight = weight;

      mv.bottom.x = xvmc_mb->PMV[0][vector][0];
      mv.bottom.y = xvmc_mb->PMV[0][vector][1];
      mv.bottom.weight = weight;
      mv.bottom.field_select = PIPE_VIDEO_FRAME;
      break;

   case XVMC_PREDICTION_FIELD:
      mv.top.x = xvmc_mb->PMV[0][vector][0];
      mv.top.y = xvmc_mb->PMV[0][vector][1];
      mv.top.field_select = (xvmc_mb->motion_vertical_field_select & field_select_mask) ?
         PIPE_VIDEO_BOTTOM_FIELD : PIPE_VIDEO_TOP_FIELD;
      mv.top.weight = weight;

      mv.bottom.x = xvmc_mb->PMV[1][vector][0];
      mv.bottom.y = xvmc_mb->PMV[1][vector][1];
      mv.bottom.field_select = (xvmc_mb->motion_vertical_field_select & (field_select_mask << 2)) ?
         PIPE_VIDEO_BOTTOM_FIELD : PIPE_VIDEO_TOP_FIELD;
      mv.bottom.weight = weight;
      break;

   default: // TODO: Support DUALPRIME and 16x8
      break;
   }

   return mv;
}

static inline void
UploadYcbcrBlocks(XvMCSurfacePrivate *surface,
                  const XvMCMacroBlock *xvmc_mb,
                  const XvMCBlockArray *xvmc_blocks)
{
   enum pipe_mpeg12_dct_intra intra;
   enum pipe_mpeg12_dct_type coding;

   unsigned tb, x, y;
   short *blocks;

   assert(surface);
   assert(xvmc_mb);

   intra = xvmc_mb->macroblock_type & XVMC_MB_TYPE_INTRA ?
           PIPE_MPEG12_DCT_INTRA : PIPE_MPEG12_DCT_DELTA;

   coding = xvmc_mb->dct_type == XVMC_DCT_TYPE_FIELD ?
            PIPE_MPEG12_DCT_TYPE_FIELD : PIPE_MPEG12_DCT_TYPE_FRAME;

   blocks = xvmc_blocks->blocks + xvmc_mb->index * BLOCK_SIZE_SAMPLES;

   for (y = 0; y < 2; ++y) {
      for (x = 0; x < 2; ++x, ++tb) {
         if (xvmc_mb->coded_block_pattern & const_empty_block_mask_420[0][y][x]) {

            struct pipe_ycbcr_block *stream = surface->ycbcr[0].stream;
            stream->x = xvmc_mb->x * 2 + x;
            stream->y = xvmc_mb->y * 2 + y;
            stream->intra = intra;
            stream->coding = coding;

            memcpy(surface->ycbcr[0].buffer, blocks, BLOCK_SIZE_BYTES);

            surface->ycbcr[0].num_blocks_added++;
            surface->ycbcr[0].stream++;
            surface->ycbcr[0].buffer += BLOCK_SIZE_SAMPLES;
            blocks += BLOCK_SIZE_SAMPLES;
         }
      }
   }

   /* TODO: Implement 422, 444 */
   //assert(ctx->base.chroma_format == PIPE_VIDEO_CHROMA_FORMAT_420);

   for (tb = 1; tb < 3; ++tb) {
      if (xvmc_mb->coded_block_pattern & const_empty_block_mask_420[tb][0][0]) {

         struct pipe_ycbcr_block *stream = surface->ycbcr[tb].stream;
         stream->x = xvmc_mb->x;
         stream->y = xvmc_mb->y;
         stream->intra = intra;
         stream->coding = PIPE_MPEG12_DCT_TYPE_FRAME;

         memcpy(surface->ycbcr[tb].buffer, blocks, BLOCK_SIZE_BYTES);

         surface->ycbcr[tb].num_blocks_added++;
         surface->ycbcr[tb].stream++;
         surface->ycbcr[tb].buffer += BLOCK_SIZE_SAMPLES;
         blocks += BLOCK_SIZE_SAMPLES;
      }
   }

}

static void
MacroBlocksToPipe(XvMCSurfacePrivate *surface,
                  unsigned int xvmc_picture_structure,
                  const XvMCMacroBlock *xvmc_mb,
                  const XvMCBlockArray *xvmc_blocks,
                  unsigned int num_macroblocks)
{
   unsigned int i, j;

   assert(xvmc_mb);
   assert(xvmc_blocks);
   assert(num_macroblocks);

   for (i = 0; i < num_macroblocks; ++i) {
      unsigned mv_pos = xvmc_mb->x + surface->mv_stride * xvmc_mb->y;
      unsigned mv_weights[2];

      UploadYcbcrBlocks(surface, xvmc_mb, xvmc_blocks);

      MacroBlockTypeToPipeWeights(xvmc_mb, mv_weights);

      for (j = 0; j < 2; ++j) {
         if (!surface->ref[j].mv) continue;

         surface->ref[j].mv[mv_pos] = MotionVectorToPipe
         (
            xvmc_mb, j,
            j ? XVMC_SELECT_FIRST_BACKWARD : XVMC_SELECT_FIRST_FORWARD,
            mv_weights[j]
         );
      }

      ++xvmc_mb;
   }
}

static void
unmap_and_flush_surface(XvMCSurfacePrivate *surface)
{
   struct pipe_video_buffer *ref_frames[2];
   XvMCContextPrivate *context_priv;
   unsigned i, num_ycbcr_blocks[3];

   assert(surface);

   context_priv = surface->context->privData;

   for ( i = 0; i < 2; ++i ) {
      if (surface->ref[i].surface) {
         XvMCSurfacePrivate *ref = surface->ref[i].surface->privData;

         assert(ref);

         unmap_and_flush_surface(ref);
         surface->ref[i].surface = NULL;
         ref_frames[i] = ref->video_buffer;
      } else {
         ref_frames[i] = NULL;
      }
   }

   if (surface->mapped) {
      surface->decode_buffer->unmap(surface->decode_buffer);
      for (i = 0; i < 3; ++i)
         num_ycbcr_blocks[i] = surface->ycbcr[i].num_blocks_added;
      context_priv->decoder->flush_buffer(surface->decode_buffer,
                                          num_ycbcr_blocks,
                                          ref_frames,
                                          surface->video_buffer,
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

   surface_priv->decode_buffer = context_priv->decoder->create_buffer(context_priv->decoder);
   surface_priv->mv_stride = surface_priv->decode_buffer->get_mv_stream_stride(surface_priv->decode_buffer);
   surface_priv->video_buffer = vpipe->create_buffer(vpipe, PIPE_FORMAT_YV12, //TODO
                                                     context_priv->decoder->chroma_format,
                                                     context_priv->decoder->width,
                                                     context_priv->decoder->height);
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
   struct pipe_video_decode_buffer *t_buffer;

   XvMCContextPrivate *context_priv;
   XvMCSurfacePrivate *target_surface_priv;
   XvMCSurfacePrivate *past_surface_priv;
   XvMCSurfacePrivate *future_surface_priv;
   XvMCMacroBlock *xvmc_mb;

   unsigned i;

   XVMC_MSG(XVMC_TRACE, "[XvMC] Rendering to surface %p, with past %p and future %p\n",
            target_surface, past_surface, future_surface);

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

   t_buffer = target_surface_priv->decode_buffer;

   // enshure that all reference frames are flushed
   // not really nessasary, but speeds ups rendering
   if (past_surface)
      unmap_and_flush_surface(past_surface->privData);

   if (future_surface)
      unmap_and_flush_surface(future_surface->privData);

   xvmc_mb = macroblocks->macro_blocks + first_macroblock;

   /* If the surface we're rendering hasn't changed the ref frames shouldn't change. */
   if (target_surface_priv->mapped && (
       target_surface_priv->ref[0].surface != past_surface ||
       target_surface_priv->ref[1].surface != future_surface ||
       (xvmc_mb->x == 0 && xvmc_mb->y == 0))) {

      // If they change anyway we need to clear our surface
      unmap_and_flush_surface(target_surface_priv);
   }

   if (!target_surface_priv->mapped) {
      t_buffer->map(t_buffer);

      for (i = 0; i < 3; ++i) {
         target_surface_priv->ycbcr[i].num_blocks_added = 0;
         target_surface_priv->ycbcr[i].stream = t_buffer->get_ycbcr_stream(t_buffer, i);
         target_surface_priv->ycbcr[i].buffer = t_buffer->get_ycbcr_buffer(t_buffer, i);
      }

      for (i = 0; i < 2; ++i) {
         target_surface_priv->ref[i].surface = i == 0 ? past_surface : future_surface;

         if (target_surface_priv->ref[i].surface)
            target_surface_priv->ref[i].mv = t_buffer->get_mv_stream(t_buffer, i);
         else
            target_surface_priv->ref[i].mv = NULL;
      }

      target_surface_priv->mapped = 1;
   }

   MacroBlocksToPipe(target_surface_priv, picture_structure, xvmc_mb, blocks, num_macroblocks);

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

   XVMC_MSG(XVMC_TRACE, "[XvMC] Flushing surface %p\n", surface);

   return Success;
}

PUBLIC
Status XvMCSyncSurface(Display *dpy, XvMCSurface *surface)
{
   assert(dpy);

   if (!surface)
      return XvMCBadSurface;

   XVMC_MSG(XVMC_TRACE, "[XvMC] Syncing surface %p\n", surface);

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
   struct pipe_video_compositor *compositor;

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
   compositor = context_priv->compositor;

   unmap_and_flush_surface(surface_priv);

   compositor->clear_layers(compositor);
   compositor->set_buffer_layer(compositor, 0, surface_priv->video_buffer, &src_rect, NULL);

   if (subpicture_priv) {
      XVMC_MSG(XVMC_TRACE, "[XvMC] Surface %p has subpicture %p.\n", surface, surface_priv->subpicture);

      assert(subpicture_priv->surface == surface);

      if (subpicture_priv->palette)
         compositor->set_palette_layer(compositor, 1, subpicture_priv->sampler, subpicture_priv->palette,
                                       &subpicture_priv->src_rect, &subpicture_priv->dst_rect);
      else
         compositor->set_rgba_layer(compositor, 1, subpicture_priv->sampler, &src_rect, &dst_rect);

      surface_priv->subpicture = NULL;
      subpicture_priv->surface = NULL;
   }

   compositor->render_picture(compositor, PictureToPipe(flags), drawable_surface, &dst_rect, &surface_priv->disp_fence);

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
      if (system(cmd) != 0)
         XVMC_MSG(XVMC_ERR, "[XvMC] Dumping surface %p failed.\n", surface);
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
   surface_priv->decode_buffer->destroy(surface_priv->decode_buffer);
   surface_priv->video_buffer->destroy(surface_priv->video_buffer);
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

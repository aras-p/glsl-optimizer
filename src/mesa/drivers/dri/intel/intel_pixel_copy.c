/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "main/glheader.h"
#include "main/enums.h"
#include "main/image.h"
#include "main/state.h"
#include "main/mtypes.h"
#include "main/macros.h"
#include "swrast/swrast.h"

#include "intel_screen.h"
#include "intel_context.h"
#include "intel_batchbuffer.h"
#include "intel_buffers.h"
#include "intel_blit.h"
#include "intel_regions.h"
#include "intel_pixel.h"

#define FILE_DEBUG_FLAG DEBUG_PIXEL

static struct intel_region *
copypix_src_region(struct intel_context *intel, GLenum type)
{
   switch (type) {
   case GL_COLOR:
      return intel_readbuf_region(intel);
   case GL_DEPTH:
      /* Don't think this is really possible execpt at 16bpp, when we have no stencil.
       */
      if (intel->depth_region && intel->depth_region->cpp == 2)
         return intel->depth_region;
   case GL_STENCIL:
      /* Don't think this is really possible. 
       */
      break;
   case GL_DEPTH_STENCIL_EXT:
      /* Does it matter whether it is stencil/depth or depth/stencil?
       */
      return intel->depth_region;
   default:
      break;
   }

   return NULL;
}


/**
 * Check if any fragment operations are in effect which might effect
 * glCopyPixels.  Differs from intel_check_blit_fragment_ops in that
 * we allow Scissor.
 */
static GLboolean
intel_check_copypixel_blit_fragment_ops(GLcontext * ctx)
{
   if (ctx->NewState)
      _mesa_update_state(ctx);

   /* Could do logicop with the blitter: 
    */
   return !(ctx->_ImageTransferState ||
            ctx->Color.AlphaEnabled ||
            ctx->Depth.Test ||
            ctx->Fog.Enabled ||
            ctx->Stencil.Enabled ||
            !ctx->Color.ColorMask[0] ||
            !ctx->Color.ColorMask[1] ||
            !ctx->Color.ColorMask[2] ||
            !ctx->Color.ColorMask[3] ||
            ctx->Texture._EnabledUnits ||
	    ctx->FragmentProgram._Enabled ||
	    ctx->Color.BlendEnabled);
}

#ifdef I915
/* Doesn't work for overlapping regions.  Could do a double copy or
 * just fallback.
 */
static GLboolean
do_texture_copypixels(GLcontext * ctx,
                      GLint srcx, GLint srcy,
                      GLsizei width, GLsizei height,
                      GLint dstx, GLint dsty, GLenum type)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_region *dst = intel_drawbuf_region(intel);
   struct intel_region *src = copypix_src_region(intel, type);
   GLenum src_format;
   GLenum src_type;

   DBG("%s %d,%d %dx%d --> %d,%d\n", __FUNCTION__, 
       srcx, srcy, width, height, dstx, dsty);

   if (!src || !dst || type != GL_COLOR)
      return GL_FALSE;

   /* Can't handle overlapping regions.  Don't have sufficient control
    * over rasterization to pull it off in-place.  Punt on these for
    * now.
    * 
    * XXX: do a copy to a temporary. 
    */
   if (src->buffer == dst->buffer) {
      drm_clip_rect_t srcbox;
      drm_clip_rect_t dstbox;
      drm_clip_rect_t tmp;

      srcbox.x1 = srcx;
      srcbox.y1 = srcy;
      srcbox.x2 = srcx + width;
      srcbox.y2 = srcy + height;

      if (ctx->Pixel.ZoomX > 0) {
	 dstbox.x1 = dstx;
	 dstbox.x2 = dstx + width * ctx->Pixel.ZoomX;
      } else {
	 dstbox.x1 = dstx + width * ctx->Pixel.ZoomX;
	 dstbox.x2 = dstx;
      }
      if (ctx->Pixel.ZoomY > 0) {
	 dstbox.y1 = dsty;
	 dstbox.y2 = dsty + height * ctx->Pixel.ZoomY;
      } else {
	 dstbox.y1 = dsty + height * ctx->Pixel.ZoomY;
	 dstbox.y2 = dsty;
      }

      DBG("src %d,%d %d,%d\n", srcbox.x1, srcbox.y1, srcbox.x2, srcbox.y2);
      DBG("dst %d,%d %d,%d (%dx%d) (%f,%f)\n", dstbox.x1, dstbox.y1, dstbox.x2, dstbox.y2,
	  width, height, ctx->Pixel.ZoomX, ctx->Pixel.ZoomY);

      if (intel_intersect_cliprects(&tmp, &srcbox, &dstbox)) {
         DBG("%s: regions overlap\n", __FUNCTION__);
         return GL_FALSE;
      }
   }

   intelFlush(&intel->ctx);

   intel->vtbl.install_meta_state(intel);

   /* Is this true?  Also will need to turn depth testing on according
    * to state:
    */
   intel->vtbl.meta_no_stencil_write(intel);
   intel->vtbl.meta_no_depth_write(intel);

   /* Set the 3d engine to draw into the destination region:
    */
   intel->vtbl.meta_draw_region(intel, dst, intel->depth_region);

   intel->vtbl.meta_import_pixel_state(intel);

   if (src->cpp == 2) {
      src_format = GL_RGB;
      src_type = GL_UNSIGNED_SHORT_5_6_5;
   }
   else {
      src_format = GL_BGRA;
      src_type = GL_UNSIGNED_BYTE;
   }

   /* Set the frontbuffer up as a large rectangular texture.
    */
   if (!intel->vtbl.meta_tex_rect_source(intel, src->buffer, 0,
                                         src->pitch,
                                         src->height, src_format, src_type)) {
      intel->vtbl.leave_meta_state(intel);
      return GL_FALSE;
   }


   intel->vtbl.meta_texture_blend_replace(intel);

   LOCK_HARDWARE(intel);

   if (intel->driDrawable->numClipRects) {
      __DRIdrawablePrivate *dPriv = intel->driDrawable;


      srcy = dPriv->h - srcy - height;  /* convert from gl to hardware coords */

      srcx += dPriv->x;
      srcy += dPriv->y;

      /* Clip against the source region.  This is the only source
       * clipping we do.  XXX: Just set the texcord wrap mode to clamp
       * or similar.
       *
       */
      if (0) {
         GLint orig_x = srcx;
         GLint orig_y = srcy;

         if (!_mesa_clip_to_region(0, 0, src->pitch, src->height,
                                   &srcx, &srcy, &width, &height))
            goto out;

         dstx += srcx - orig_x;
         dsty += (srcy - orig_y) * ctx->Pixel.ZoomY;
      }

      /* Just use the regular cliprect mechanism...  Does this need to
       * even hold the lock???
       */
      intel->vtbl.meta_draw_quad(intel,
				 dstx,
				 dstx + width * ctx->Pixel.ZoomX,
				 dPriv->h - (dsty + height * ctx->Pixel.ZoomY),
				 dPriv->h - (dsty), 0, /* XXX: what z value? */
				 0x00ff00ff,
				 srcx, srcx + width, srcy, srcy + height);

    out:
      intel->vtbl.leave_meta_state(intel);
      intel_batchbuffer_emit_mi_flush(intel->batch);
   }
   UNLOCK_HARDWARE(intel);

   DBG("%s: success\n", __FUNCTION__);
   return GL_TRUE;
}
#endif /* I915 */


/**
 * CopyPixels with the blitter.  Don't support zooming, pixel transfer, etc.
 */
static GLboolean
do_blit_copypixels(GLcontext * ctx,
                   GLint srcx, GLint srcy,
                   GLsizei width, GLsizei height,
                   GLint dstx, GLint dsty, GLenum type)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_region *dst = intel_drawbuf_region(intel);
   struct intel_region *src = copypix_src_region(intel, type);
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   struct gl_framebuffer *read_fb = ctx->ReadBuffer;
   unsigned int num_cliprects;
   drm_clip_rect_t *cliprects;
   int x_off, y_off;

   /* Copypixels can be more than a straight copy.  Ensure all the
    * extra operations are disabled:
    */
   if (!intel_check_copypixel_blit_fragment_ops(ctx) ||
       ctx->Pixel.ZoomX != 1.0F || ctx->Pixel.ZoomY != 1.0F)
      return GL_FALSE;

   if (!src || !dst)
      return GL_FALSE;



   intelFlush(&intel->ctx);

   LOCK_HARDWARE(intel);

   intel_get_cliprects(intel, &cliprects, &num_cliprects, &x_off, &y_off);
   if (num_cliprects != 0) {
      GLint delta_x;
      GLint delta_y;
      GLint orig_dstx;
      GLint orig_dsty;
      GLint orig_srcx;
      GLint orig_srcy;
      GLuint i;

      /* XXX: We fail to handle different inversion between read and draw framebuffer. */

      /* Clip to destination buffer. */
      orig_dstx = dstx;
      orig_dsty = dsty;
      if (!_mesa_clip_to_region(fb->_Xmin, fb->_Ymin,
				fb->_Xmax, fb->_Ymax,
				&dstx, &dsty, &width, &height))
	 goto out;
      /* Adjust src coords for our post-clipped destination origin */
      srcx += dstx - orig_dstx;
      srcy += dsty - orig_dsty;

      /* Clip to source buffer. */
      orig_srcx = srcx;
      orig_srcy = srcy;
      if (!_mesa_clip_to_region(read_fb->_Xmin, read_fb->_Ymin,
				read_fb->_Xmax, read_fb->_Ymax,
				&srcx, &srcy, &width, &height))
	 goto out;
      /* Adjust dst coords for our post-clipped source origin */
      dstx += srcx - orig_srcx;
      dsty += srcy - orig_srcy;

      /* Convert from GL to hardware coordinates:
       */
      if (fb->Name == 0) {
	 /* copypixels to a system framebuffer */
	 dstx = x_off + dstx;
	 dsty = y_off + (fb->Height - dsty - height);
      } else {
	 /* copypixels to a user framebuffer object */
	 dstx = x_off + dstx;
	 dsty = y_off + dsty;
      }

      /* Flip source Y if it's a system framebuffer. */
      if (read_fb->Name == 0) {
	 srcx = intel->driReadDrawable->x + srcx;
	 srcy = intel->driReadDrawable->y + (fb->Height - srcy - height);
      }

      delta_x = srcx - dstx;
      delta_y = srcy - dsty;
      /* Could do slightly more clipping: Eg, take the intersection of
       * the destination cliprects and the read drawable cliprects
       *
       * This code will not overwrite other windows, but will
       * introduce garbage when copying from obscured window regions.
       */
      for (i = 0; i < num_cliprects; i++) {
	 GLint clip_x = dstx;
	 GLint clip_y = dsty;
	 GLint clip_w = width;
	 GLint clip_h = height;

         if (!_mesa_clip_to_region(cliprects[i].x1, cliprects[i].y1,
				   cliprects[i].x2, cliprects[i].y2,
				   &clip_x, &clip_y, &clip_w, &clip_h))
            continue;

         intelEmitCopyBlit(intel, dst->cpp,
			   src->pitch, src->buffer, 0, src->tiling,
			   dst->pitch, dst->buffer, 0, dst->tiling,
			   clip_x + delta_x, clip_y + delta_y, /* srcx, srcy */
			   clip_x, clip_y, /* dstx, dsty */
			   clip_w, clip_h,
			   ctx->Color.ColorLogicOpEnabled ?
			   ctx->Color.LogicOp : GL_COPY);
      }
   }
out:
   UNLOCK_HARDWARE(intel);

   DBG("%s: success\n", __FUNCTION__);
   return GL_TRUE;
}


void
intelCopyPixels(GLcontext * ctx,
                GLint srcx, GLint srcy,
                GLsizei width, GLsizei height,
                GLint destx, GLint desty, GLenum type)
{
   if (INTEL_DEBUG & DEBUG_PIXEL)
      fprintf(stderr, "%s\n", __FUNCTION__);

   if (do_blit_copypixels(ctx, srcx, srcy, width, height, destx, desty, type))
      return;

#ifdef I915
   if (do_texture_copypixels(ctx, srcx, srcy, width, height, destx, desty, type))
      return;
#endif

   DBG("fallback to _swrast_CopyPixels\n");

   _swrast_CopyPixels(ctx, srcx, srcy, width, height, destx, desty, type);
}

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
#include "main/image.h"
#include "main/state.h"
#include "main/mtypes.h"
#include "drivers/common/meta.h"

#include "intel_context.h"
#include "intel_buffers.h"
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
            ctx->Stencil._Enabled ||
            !ctx->Color.ColorMask[0] ||
            !ctx->Color.ColorMask[1] ||
            !ctx->Color.ColorMask[2] ||
            !ctx->Color.ColorMask[3] ||
            ctx->Texture._EnabledUnits ||
	    ctx->FragmentProgram._Enabled ||
	    ctx->Color.BlendEnabled);
}


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

   if (type == GL_DEPTH || type == GL_STENCIL) {
      if (INTEL_DEBUG & DEBUG_FALLBACKS)
	 fprintf(stderr, "glCopyPixels() fallback: GL_DEPTH || GL_STENCIL\n");
      return GL_FALSE;
   }

   /* Update draw buffer bounds */
   _mesa_update_state(ctx);

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
      if (!_mesa_clip_to_region(0, 0,
				read_fb->Width, read_fb->Height,
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

	 if (!intel_region_copy(intel,
				dst, 0, clip_x, clip_y,
				src, 0, clip_x + delta_x, clip_y + delta_y,
				clip_w, clip_h,
				ctx->Color.ColorLogicOpEnabled ?
				ctx->Color.LogicOp : GL_COPY)) {
	    DBG("%s: blit failure\n", __FUNCTION__);
	    UNLOCK_HARDWARE(intel);
	    return GL_FALSE;
	 }
      }
   }
out:
   UNLOCK_HARDWARE(intel);

   intel_check_front_buffer_rendering(intel);

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

   /* this will use swrast if needed */
   _mesa_meta_CopyPixels(ctx, srcx, srcy, width, height, destx, desty, type);
}

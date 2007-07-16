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

#include "glheader.h"
#include "enums.h"
#include "image.h"
#include "state.h"
#include "mtypes.h"
#include "macros.h"
#include "swrast/swrast.h"

#include "intel_screen.h"
#include "intel_context.h"
#include "intel_ioctl.h"
#include "intel_batchbuffer.h"
#include "intel_buffers.h"
#include "intel_blit.h"
#include "intel_regions.h"
#include "intel_tris.h"
#include "intel_pixel.h"
#include "intel_fbo.h"

#define FILE_DEBUG_FLAG DEBUG_PIXEL

static struct intel_region *
copypix_src_region(struct intel_context *intel, GLenum type)
{
   struct intel_renderbuffer *irb;

   switch (type) {
   case GL_COLOR:
      return intel_readbuf_region(intel);
   case GL_DEPTH:
      /* Don't think this is really possible execpt at 16bpp, when we have no stencil.
       */
      irb = intel_renderbuffer(intel->ctx.ReadBuffer->_DepthBuffer->Wrapped);
         if (irb && irb->region && irb->region->cpp == 2)
            return irb->region;
      break;
   case GL_STENCIL:
      /* Don't think this is really possible. 
       */
      break;
   case GL_DEPTH_STENCIL_EXT:
      /* Does it matter whether it is stencil/depth or depth/stencil?
       */
      irb = intel_renderbuffer(intel->ctx.ReadBuffer->_DepthBuffer->Wrapped);
         if (irb && irb->region)
            return irb->region;
      break;
   default:
      break;
   }

   return NULL;
}


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
   struct intel_region *depthreg = NULL;
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

      dstbox.x1 = dstx;
      dstbox.y1 = dsty;
      dstbox.x2 = dstx + width * ctx->Pixel.ZoomX;
      dstbox.y2 = dsty + height * ctx->Pixel.ZoomY;

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
   if (ctx->DrawBuffer->_DepthBuffer &&
       ctx->DrawBuffer->_DepthBuffer->Wrapped)
      depthreg = (intel_renderbuffer(ctx->DrawBuffer->_DepthBuffer->Wrapped))->region;

   intel->vtbl.meta_draw_region(intel, dst, depthreg);

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

   {
      int dstbufHeight = ctx->DrawBuffer->Height;
      /* convert from gl to hardware coords */
      srcy = ctx->ReadBuffer->Height - srcy - height;

      /* Clip against the source region.  This is the only source
       * clipping we do.  XXX: Just set the texcord wrap mode to clamp
       * or similar.
       *
       */
      if (0) {
         GLint orig_x = srcx;
         GLint orig_y = srcy;

         if (!_mesa_clip_to_region(0, 0, ctx->ReadBuffer->Width - 1,
				   ctx->ReadBuffer->Height - 1,
                                   &srcx, &srcy, &width, &height))
            goto out;

         dstx += srcx - orig_x;
         dsty += (srcy - orig_y) * ctx->Pixel.ZoomY;
      }

      /* Just use the regular cliprect mechanism...  Does this need to
       * even hold the lock???
       */
      intel_meta_draw_quad(intel,
			   dstx,
			   dstx + width * ctx->Pixel.ZoomX,
			   dstbufHeight - (dsty + height * ctx->Pixel.ZoomY),
			   dstbufHeight - (dsty), 0,   /* XXX: what z value? */
                           0x00ff00ff,
                           srcx, srcx + width, srcy, srcy + height);

    out:
      intel->vtbl.leave_meta_state(intel);
      intel_batchbuffer_flush(intel->batch);
   }
   UNLOCK_HARDWARE(intel);

   DBG("%s: success\n", __FUNCTION__);
   return GL_TRUE;
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
   struct intel_renderbuffer *irbread;
   struct intel_renderbuffer *irbdraw;
   struct intel_region *dst;
   struct intel_region *src;

   /* Copypixels can be more than a straight copy.  Ensure all the
    * extra operations are disabled:
    */
   if (!intel_check_blit_fragment_ops(ctx) ||
       ctx->Pixel.ZoomX != 1.0F || ctx->Pixel.ZoomY != 1.0F)
      return GL_FALSE;

   intelFlush(&intel->ctx);

   if (type == GL_COLOR) {
      irbread = intel_renderbuffer(ctx->ReadBuffer->_ColorReadBuffer);
      irbdraw = intel_renderbuffer(ctx->DrawBuffer->_ColorDrawBuffers[0][0]);
      if (!irbread || !irbread->region || !irbdraw || !irbdraw->region)
	 return GL_FALSE;
   }
   else if (type == GL_DEPTH) {
      /* Don't think this is really possible execpt at 16bpp, when we have no stencil.
       */
      irbread = intel_renderbuffer(ctx->ReadBuffer->_DepthBuffer->Wrapped);
      irbdraw = intel_renderbuffer(ctx->DrawBuffer->_DepthBuffer->Wrapped);
      if (!irbread || !irbread->region || !irbdraw || !irbdraw->region
	 || !(irbread->region->cpp == 2))
	 return GL_FALSE;
   }
   else if (type == GL_DEPTH_STENCIL_EXT) {
      /* Does it matter whether it is stencil/depth or depth/stencil?
       */
      irbread = intel_renderbuffer(ctx->ReadBuffer->_DepthBuffer->Wrapped);
      irbdraw = intel_renderbuffer(ctx->DrawBuffer->_DepthBuffer->Wrapped);
      if (!irbread || !irbread->region || !irbdraw || !irbdraw->region)
	 return GL_FALSE;
   }
   else if (type == GL_STENCIL) {
      /* Don't think this is really possible. 
       */
      return GL_FALSE;
   }

   src = irbread->region;
   dst = irbdraw->region;

   {
      GLint delta_x = 0;
      GLint delta_y = 0;

      /* Do scissoring in GL coordinates:
       */
      if (ctx->Scissor.Enabled)
      {
	 GLint x = ctx->Scissor.X;
	 GLint y = ctx->Scissor.Y;
	 GLuint w = ctx->Scissor.Width;
	 GLuint h = ctx->Scissor.Height;
	 GLint dx = dstx - srcx;
         GLint dy = dsty - srcy;

         if (!_mesa_clip_to_region(x, y, x+w-1, y+h-1, &dstx, &dsty, &width, &height))
            goto out;

         srcx = dstx - dx;
         srcy = dsty - dy;
      }

      /* Convert from GL to hardware coordinates:
       */
      dsty = irbdraw->Base.Height - dsty - height;
      srcy = irbread->Base.Height - srcy - height;

      /* Clip against the source region:
       */
      {
         delta_x = srcx - dstx;
         delta_y = srcy - dsty;

         if (!_mesa_clip_to_region(0, 0, irbread->Base.Width - 1,
				   irbread->Base.Height - 1,
				   &srcx, &srcy, &width, &height))
            goto out;

         dstx = srcx - delta_x;
         dsty = srcy - delta_y;
      }

      /* Clip against the dest region:
       */
      {
         delta_x = dstx - srcx;
         delta_y = dsty - srcy;

         if (!_mesa_clip_to_region(0, 0, irbdraw->Base.Width - 1,
				   irbdraw->Base.Height - 1,
				   &dstx, &dsty, &width, &height))
            goto out;

         srcx = dstx - delta_x;
         srcy = dsty - delta_y;
      }

      {

         intelEmitCopyBlit(intel, dst->cpp,
			   src->pitch, src->buffer, 0,
			   dst->pitch, dst->buffer, 0,
			   srcx, srcy,
			   dstx, dsty,
			   width, height,
			   ctx->Color.ColorLogicOpEnabled ?
			   ctx->Color.LogicOp : GL_COPY);
      }

    out:
      intel_batchbuffer_flush(intel->batch);
   }

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

   if (do_texture_copypixels(ctx, srcx, srcy, width, height, destx, desty, type))
      return;

   DBG("fallback to _swrast_CopyPixels\n");

   _swrast_CopyPixels(ctx, srcx, srcy, width, height, destx, desty, type);
}

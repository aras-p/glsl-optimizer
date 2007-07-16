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
#include "mtypes.h"
#include "macros.h"
#include "image.h"
#include "bufferobj.h"
#include "swrast/swrast.h"

#include "intel_screen.h"
#include "intel_context.h"
#include "intel_ioctl.h"
#include "intel_batchbuffer.h"
#include "intel_blit.h"
#include "intel_buffers.h"
#include "intel_regions.h"
#include "intel_pixel.h"
#include "intel_buffer_objects.h"
#include "intel_fbo.h"
#include "intel_tris.h"

/* For many applications, the new ability to pull the source buffers
 * back out of the GTT and then do the packing/conversion operations
 * in software will be as much of an improvement as trying to get the
 * blitter and/or texture engine to do the work. 
 *
 * This step is gated on private backbuffers.
 * 
 * Obviously the frontbuffer can't be pulled back, so that is either
 * an argument for blit/texture readpixels, or for blitting to a
 * temporary and then pulling that back.
 *
 * XXX this is not true for fake frontbuffers...

 * When the destination is a pbo, however, it's not clear if it is
 * ever going to be pulled to main memory (though the access param
 * will be a good hint).  So it sounds like we do want to be able to
 * choose between blit/texture implementation on the gpu and pullback
 * and cpu-based copying.
 *
 * Unless you can magically turn client memory into a PBO for the
 * duration of this call, there will be a cpu-based copying step in
 * any case.
 */


static GLboolean
do_texture_readpixels(GLcontext * ctx,
                      GLint srcx, GLint srcy, GLsizei width, GLsizei height,
                      GLenum format, GLenum type,
                      const struct gl_pixelstore_attrib *pack,
                      struct intel_region *dest_region)
{
#if 0
   struct intel_context *intel = intel_context(ctx);
   struct intel_region *depthreg = NULL;
   struct intel_region *src = intel_readbuf_region(intel);
// struct intel_region *src = copypix_src_region(intel, type);
   GLenum src_format;
   GLenum src_type;

   if (INTEL_DEBUG & DEBUG_PIXEL)
      fprintf(stderr, "%s\n", __FUNCTION__);

   if (!src || type != GL_COLOR)
      return GL_FALSE;

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

   intel->vtbl.meta_draw_region(intel, dest_region, depthreg);

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
      int dstbufHeight = height * ctx->Pixel.ZoomY; /* ? */
      /* convert from gl to hardware coords */
      srcy = ctx->ReadBuffer->Height - srcy - height;

      /* Clip against the source region.  This is the only source
       * clipping we do.  XXX: Just set the texcord wrap mode to clamp
       * or similar.
       *
       */
      if (0) {
         if (!_mesa_clip_to_region(0, 0, ctx->ReadBuffer->Width - 1,
				   ctx->ReadBuffer->Height - 1,
                                   &srcx, &srcy, &width, &height))
            goto out;

      }

      /* Just use the regular cliprect mechanism...  Does this need to
       * even hold the lock???
       */
      intel_meta_draw_quad(intel,
			   0,
			   width * ctx->Pixel.ZoomX,
			   dstbufHeight - (height * ctx->Pixel.ZoomY),
			   dstbufHeight, 0,   /* XXX: what z value? */
                           0x00ff00ff,
                           srcx, srcx + width, srcy, srcy + height);

    out:
      intel->vtbl.leave_meta_state(intel);
      intel_batchbuffer_flush(intel->batch);
   }
   UNLOCK_HARDWARE(intel);

   if (INTEL_DEBUG & DEBUG_PIXEL)
      _mesa_printf("%s: success\n", __FUNCTION__);
   return GL_TRUE;
#endif

   return GL_FALSE;
}




static GLboolean
do_blit_readpixels(GLcontext * ctx,
                   GLint x, GLint y, GLsizei width, GLsizei height,
                   GLenum format, GLenum type,
                   const struct gl_pixelstore_attrib *pack, GLvoid * pixels)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_renderbuffer *irbread;
   struct intel_region *src;
   struct intel_buffer_object *dst = intel_buffer_object(pack->BufferObj);
   GLuint dst_offset;
   GLuint rowLength;
   struct _DriFenceObject *fence = NULL;

   if (INTEL_DEBUG & DEBUG_PIXEL)
      _mesa_printf("%s\n", __FUNCTION__);

   irbread = intel_renderbuffer(ctx->ReadBuffer->_ColorReadBuffer);
   if (!irbread || !irbread->region)
      return GL_FALSE;
   src = irbread->region;

   if (dst) {
      /* XXX This validation should be done by core mesa:
       */
      if (!_mesa_validate_pbo_access(2, pack, width, height, 1,
                                     format, type, pixels)) {
         _mesa_error(ctx, GL_INVALID_OPERATION, "glReadPixels");
         return GL_TRUE;
      }
   }
   else {
      /* PBO only for now:
       */
      if (INTEL_DEBUG & DEBUG_PIXEL)
         _mesa_printf("%s - not PBO\n", __FUNCTION__);
      return GL_FALSE;
   }


   if (ctx->_ImageTransferState ||
       !intel_check_blit_format(src, format, type)) {
      if (INTEL_DEBUG & DEBUG_PIXEL)
         _mesa_printf("%s - bad format for blit\n", __FUNCTION__);
      return GL_FALSE;
   }

   if (pack->RowLength > 0)
      rowLength = pack->RowLength;
   else
      rowLength = width;

   if (((rowLength * src->cpp) % pack->Alignment) ||
      pack->SwapBytes || pack->LsbFirst) {
      if (INTEL_DEBUG & DEBUG_PIXEL)
         _mesa_printf("%s: bad packing params\n", __FUNCTION__);
      return GL_FALSE;
   }

   if (pack->Invert) {
      if (INTEL_DEBUG & DEBUG_PIXEL)
         _mesa_printf("%s: MESA_PACK_INVERT not done yet\n", __FUNCTION__);
      return GL_FALSE;
   }
   else {
      rowLength = -rowLength;
   }

   /* XXX 64-bit cast? */
   dst_offset = (GLuint) _mesa_image_address(2, pack, pixels, width, height,
                                             format, type, 0, 0, 0);

   intelFlush(&intel->ctx);

   {
      GLint srcx = x;
      GLint srcy = irbread->Base.Height - (y + height);
      GLint srcwidth = width;
      GLint srcheight = height;

      GLboolean all = (width * height * src->cpp == dst->Base.Size &&
                       x == 0 && dst_offset == 0);

      struct _DriBufferObject *dst_buffer =
         intel_bufferobj_buffer(intel, dst, all ? INTEL_WRITE_FULL :
                                INTEL_WRITE_PART);

      /* clip to src region */
      if (_mesa_clip_to_region(0, 0, irbread->Base.Width - 1,
				   irbread->Base.Height - 1,
				   &srcx, &srcy, &srcwidth, &srcheight));

      {
         intelEmitCopyBlit(intel,
                           src->cpp,
                           src->pitch, src->buffer, 0,
                           rowLength,
                           dst_buffer, dst_offset,
                           srcx, srcy,
                           0, 0,
                           width, height,
			   GL_COPY);
      }

      fence = intel_batchbuffer_flush(intel->batch);
      driFenceReference(fence);

   }

   if (fence) {
      driFenceFinish(fence, DRM_FENCE_TYPE_EXE | DRM_I915_FENCE_TYPE_RW, 
		     GL_FALSE);
      driFenceUnReference(fence);
   }

   if (INTEL_DEBUG & DEBUG_PIXEL)
      _mesa_printf("%s - DONE\n", __FUNCTION__);

   return GL_TRUE;
}

void
intelReadPixels(GLcontext * ctx,
                GLint x, GLint y, GLsizei width, GLsizei height,
                GLenum format, GLenum type,
                const struct gl_pixelstore_attrib *pack, GLvoid * pixels)
{
   if (INTEL_DEBUG & DEBUG_PIXEL)
      fprintf(stderr, "%s\n", __FUNCTION__);

   intelFlush(ctx);

   if (do_blit_readpixels
       (ctx, x, y, width, height, format, type, pack, pixels))
      return;

   if (do_texture_readpixels
       (ctx, x, y, width, height, format, type, pack, pixels))
      return;

   if (INTEL_DEBUG & DEBUG_PIXEL)
      _mesa_printf("%s: fallback to swrast\n", __FUNCTION__);

   _swrast_ReadPixels(ctx, x, y, width, height, format, type, pack, pixels);
}

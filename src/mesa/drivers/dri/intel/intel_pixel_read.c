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
#include "main/mtypes.h"
#include "main/macros.h"
#include "main/image.h"
#include "main/bufferobj.h"
#include "main/state.h"
#include "swrast/swrast.h"

#include "intel_screen.h"
#include "intel_context.h"
#include "intel_blit.h"
#include "intel_buffers.h"
#include "intel_regions.h"
#include "intel_pixel.h"
#include "intel_buffer_objects.h"

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
do_blit_readpixels(GLcontext * ctx,
                   GLint x, GLint y, GLsizei width, GLsizei height,
                   GLenum format, GLenum type,
                   const struct gl_pixelstore_attrib *pack, GLvoid * pixels)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_region *src = intel_readbuf_region(intel);
   struct intel_buffer_object *dst = intel_buffer_object(pack->BufferObj);
   GLuint dst_offset;
   GLuint rowLength;
   drm_intel_bo *dst_buffer;
   GLboolean all;
   GLint dst_x, dst_y;

   if (INTEL_DEBUG & DEBUG_PIXEL)
      printf("%s\n", __FUNCTION__);

   if (!src)
      return GL_FALSE;

   if (!_mesa_is_bufferobj(pack->BufferObj)) {
      /* PBO only for now:
       */
      if (INTEL_DEBUG & DEBUG_PIXEL)
         printf("%s - not PBO\n", __FUNCTION__);
      return GL_FALSE;
   }


   if (ctx->_ImageTransferState ||
       !intel_check_blit_format(src, format, type)) {
      if (INTEL_DEBUG & DEBUG_PIXEL)
         printf("%s - bad format for blit\n", __FUNCTION__);
      return GL_FALSE;
   }

   if (pack->Alignment != 1 || pack->SwapBytes || pack->LsbFirst) {
      if (INTEL_DEBUG & DEBUG_PIXEL)
         printf("%s: bad packing params\n", __FUNCTION__);
      return GL_FALSE;
   }

   if (pack->RowLength > 0)
      rowLength = pack->RowLength;
   else
      rowLength = width;

   if (pack->Invert) {
      if (INTEL_DEBUG & DEBUG_PIXEL)
         printf("%s: MESA_PACK_INVERT not done yet\n", __FUNCTION__);
      return GL_FALSE;
   }
   else {
      if (ctx->ReadBuffer->Name == 0)
	 rowLength = -rowLength;
   }

   dst_offset = (GLintptr) _mesa_image_address(2, pack, pixels, width, height,
					       format, type, 0, 0, 0);

   if (!_mesa_clip_copytexsubimage(ctx,
				   &dst_x, &dst_y,
				   &x, &y,
				   &width, &height)) {
      return GL_TRUE;
   }

   intel_prepare_render(intel);

   all = (width * height * src->cpp == dst->Base.Size &&
	  x == 0 && dst_offset == 0);

   dst_x = 0;
   dst_y = 0;

   dst_buffer = intel_bufferobj_buffer(intel, dst,
					       all ? INTEL_WRITE_FULL :
					       INTEL_WRITE_PART);

   if (ctx->ReadBuffer->Name == 0)
      y = ctx->ReadBuffer->Height - (y + height);

   if (!intelEmitCopyBlit(intel,
			  src->cpp,
			  src->pitch, src->buffer, 0, src->tiling,
			  rowLength, dst_buffer, dst_offset, GL_FALSE,
			  x, y,
			  dst_x, dst_y,
			  width, height,
			  GL_COPY)) {
      return GL_FALSE;
   }

   if (INTEL_DEBUG & DEBUG_PIXEL)
      printf("%s - DONE\n", __FUNCTION__);

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
   intel_prepare_render(intel_context(ctx));

   if (do_blit_readpixels
       (ctx, x, y, width, height, format, type, pack, pixels))
      return;

   if (INTEL_DEBUG & DEBUG_PIXEL)
      printf("%s: fallback to swrast\n", __FUNCTION__);

   /* Update Mesa state before calling down into _swrast_ReadPixels, as
    * the spans code requires the computed buffer states to be up to date,
    * but _swrast_ReadPixels only updates Mesa state after setting up
    * the spans code.
    */

   if (ctx->NewState)
      _mesa_update_state(ctx);

   _swrast_ReadPixels(ctx, x, y, width, height, format, type, pack, pixels);
}

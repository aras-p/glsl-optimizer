/**************************************************************************
 *
 * Copyright 2003 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "main/glheader.h"
#include "main/enums.h"
#include "main/mtypes.h"
#include "main/macros.h"
#include "main/fbobject.h"
#include "main/image.h"
#include "main/bufferobj.h"
#include "main/readpix.h"
#include "main/state.h"
#include "main/glformats.h"

#include "brw_context.h"
#include "intel_screen.h"
#include "intel_blit.h"
#include "intel_buffers.h"
#include "intel_fbo.h"
#include "intel_mipmap_tree.h"
#include "intel_pixel.h"
#include "intel_buffer_objects.h"

#define FILE_DEBUG_FLAG DEBUG_PIXEL

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

static bool
do_blit_readpixels(struct gl_context * ctx,
                   GLint x, GLint y, GLsizei width, GLsizei height,
                   GLenum format, GLenum type,
                   const struct gl_pixelstore_attrib *pack, GLvoid * pixels)
{
   struct brw_context *brw = brw_context(ctx);
   struct intel_buffer_object *dst = intel_buffer_object(pack->BufferObj);
   GLuint dst_offset;
   drm_intel_bo *dst_buffer;
   GLint dst_x, dst_y;
   GLuint dirty;

   DBG("%s\n", __FUNCTION__);

   assert(_mesa_is_bufferobj(pack->BufferObj));

   struct gl_renderbuffer *rb = ctx->ReadBuffer->_ColorReadBuffer;
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);

   /* Currently this function only supports reading from color buffers. */
   if (!_mesa_is_color_format(format))
      return false;

   assert(irb != NULL);

   if (ctx->_ImageTransferState ||
       !_mesa_format_matches_format_and_type(irb->mt->format, format, type,
                                             false)) {
      DBG("%s - bad format for blit\n", __FUNCTION__);
      return false;
   }

   if (pack->SwapBytes || pack->LsbFirst) {
      DBG("%s: bad packing params\n", __FUNCTION__);
      return false;
   }

   int dst_stride = _mesa_image_row_stride(pack, width, format, type);
   bool dst_flip = false;
   /* Mesa flips the dst_stride for pack->Invert, but we want our mt to have a
    * normal dst_stride.
    */
   struct gl_pixelstore_attrib uninverted_pack = *pack;
   if (pack->Invert) {
      dst_stride = -dst_stride;
      dst_flip = true;
      uninverted_pack.Invert = false;
   }

   dst_offset = (GLintptr)pixels;
   dst_offset += _mesa_image_offset(2, &uninverted_pack, width, height,
				    format, type, 0, 0, 0);

   if (!_mesa_clip_copytexsubimage(ctx,
				   &dst_x, &dst_y,
				   &x, &y,
				   &width, &height)) {
      return true;
   }

   dirty = brw->front_buffer_dirty;
   intel_prepare_render(brw);
   brw->front_buffer_dirty = dirty;

   dst_buffer = intel_bufferobj_buffer(brw, dst,
				       dst_offset, height * dst_stride);

   struct intel_mipmap_tree *pbo_mt =
      intel_miptree_create_for_bo(brw,
                                  dst_buffer,
                                  irb->mt->format,
                                  dst_offset,
                                  width, height,
                                  dst_stride);

   if (!intel_miptree_blit(brw,
                           irb->mt, irb->mt_level, irb->mt_layer,
                           x, y, _mesa_is_winsys_fbo(ctx->ReadBuffer),
                           pbo_mt, 0, 0,
                           0, 0, dst_flip,
                           width, height, GL_COPY)) {
      return false;
   }

   intel_miptree_release(&pbo_mt);

   DBG("%s - DONE\n", __FUNCTION__);

   return true;
}

void
intelReadPixels(struct gl_context * ctx,
                GLint x, GLint y, GLsizei width, GLsizei height,
                GLenum format, GLenum type,
                const struct gl_pixelstore_attrib *pack, GLvoid * pixels)
{
   struct brw_context *brw = brw_context(ctx);
   bool dirty;

   DBG("%s\n", __FUNCTION__);

   if (_mesa_is_bufferobj(pack->BufferObj)) {
      /* Using PBOs, so try the BLT based path. */
      if (do_blit_readpixels(ctx, x, y, width, height, format, type, pack,
                             pixels)) {
         return;
      }

      perf_debug("%s: fallback to CPU mapping in PBO case\n", __FUNCTION__);
   }

   /* glReadPixels() wont dirty the front buffer, so reset the dirty
    * flag after calling intel_prepare_render(). */
   dirty = brw->front_buffer_dirty;
   intel_prepare_render(brw);
   brw->front_buffer_dirty = dirty;

   /* Update Mesa state before calling _mesa_readpixels().
    * XXX this may not be needed since ReadPixels no longer uses the
    * span code.
    */

   if (ctx->NewState)
      _mesa_update_state(ctx);

   _mesa_readpixels(ctx, x, y, width, height, format, type, pack, pixels);

   /* There's an intel_prepare_render() call in intelSpanRenderStart(). */
   brw->front_buffer_dirty = dirty;
}

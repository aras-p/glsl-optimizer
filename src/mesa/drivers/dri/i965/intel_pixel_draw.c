/**************************************************************************
 *
 * Copyright 2006 VMware, Inc.
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
 * next paragraph) shall be included in all copies or substantial portionsalloc
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
#include "main/image.h"
#include "main/mtypes.h"
#include "main/condrender.h"
#include "main/fbobject.h"
#include "main/teximage.h"
#include "main/texobj.h"
#include "main/texstate.h"
#include "main/bufferobj.h"
#include "swrast/swrast.h"
#include "drivers/common/meta.h"

#include "brw_context.h"
#include "intel_screen.h"
#include "intel_blit.h"
#include "intel_buffers.h"
#include "intel_fbo.h"
#include "intel_mipmap_tree.h"
#include "intel_pixel.h"
#include "intel_buffer_objects.h"

#define FILE_DEBUG_FLAG DEBUG_PIXEL

static bool
do_blit_drawpixels(struct gl_context * ctx,
		   GLint x, GLint y, GLsizei width, GLsizei height,
		   GLenum format, GLenum type,
		   const struct gl_pixelstore_attrib *unpack,
		   const GLvoid * pixels)
{
   struct brw_context *brw = brw_context(ctx);
   struct intel_buffer_object *src = intel_buffer_object(unpack->BufferObj);
   GLuint src_offset;
   drm_intel_bo *src_buffer;

   DBG("%s\n", __FUNCTION__);

   if (!intel_check_blit_fragment_ops(ctx, false))
      return false;

   if (ctx->DrawBuffer->_NumColorDrawBuffers != 1) {
      DBG("%s: fallback due to MRT\n", __FUNCTION__);
      return false;
   }

   intel_prepare_render(brw);

   struct gl_renderbuffer *rb = ctx->DrawBuffer->_ColorDrawBuffers[0];
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);

   if (!_mesa_format_matches_format_and_type(irb->mt->format, format, type,
                                             false)) {
      DBG("%s: bad format for blit\n", __FUNCTION__);
      return false;
   }

   if (unpack->SwapBytes || unpack->LsbFirst ||
       unpack->SkipPixels || unpack->SkipRows) {
      DBG("%s: bad packing params\n", __FUNCTION__);
      return false;
   }

   int src_stride = _mesa_image_row_stride(unpack, width, format, type);
   bool src_flip = false;
   /* Mesa flips the src_stride for unpack->Invert, but we want our mt to have
    * a normal src_stride.
    */
   if (unpack->Invert) {
      src_stride = -src_stride;
      src_flip = true;
   }

   src_offset = (GLintptr)pixels;
   src_offset += _mesa_image_offset(2, unpack, width, height,
				    format, type, 0, 0, 0);

   src_buffer = intel_bufferobj_buffer(brw, src, src_offset,
                                       height * src_stride);

   struct intel_mipmap_tree *pbo_mt =
      intel_miptree_create_for_bo(brw,
                                  src_buffer,
                                  irb->mt->format,
                                  src_offset,
                                  width, height,
                                  src_stride);
   if (!pbo_mt)
      return false;

   if (!intel_miptree_blit(brw,
                           pbo_mt, 0, 0,
                           0, 0, src_flip,
                           irb->mt, irb->mt_level, irb->mt_layer,
                           x, y, _mesa_is_winsys_fbo(ctx->DrawBuffer),
                           width, height, GL_COPY)) {
      DBG("%s: blit failed\n", __FUNCTION__);
      intel_miptree_release(&pbo_mt);
      return false;
   }

   intel_miptree_release(&pbo_mt);

   if (ctx->Query.CurrentOcclusionObject)
      ctx->Query.CurrentOcclusionObject->Result += width * height;

   DBG("%s: success\n", __FUNCTION__);
   return true;
}

void
intelDrawPixels(struct gl_context * ctx,
                GLint x, GLint y,
                GLsizei width, GLsizei height,
                GLenum format,
                GLenum type,
                const struct gl_pixelstore_attrib *unpack,
                const GLvoid * pixels)
{
   struct brw_context *brw = brw_context(ctx);

   if (!_mesa_check_conditional_render(ctx))
      return;

   if (format == GL_STENCIL_INDEX) {
      _swrast_DrawPixels(ctx, x, y, width, height, format, type,
                         unpack, pixels);
      return;
   }

   if (_mesa_is_bufferobj(unpack->BufferObj)) {
      if (do_blit_drawpixels(ctx, x, y, width, height, format, type, unpack,
			     pixels)) {
	 return;
      }

      perf_debug("%s: fallback to generic code in PBO case\n", __FUNCTION__);
   }

   _mesa_meta_DrawPixels(ctx, x, y, width, height, format, type,
                         unpack, pixels);
}

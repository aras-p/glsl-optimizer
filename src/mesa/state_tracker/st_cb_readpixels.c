/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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


/**
 * glReadPixels interface to pipe
 *
 * \author Brian Paul
 */


#include "main/imports.h"
#include "main/context.h"
#include "main/image.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "st_context.h"
#include "st_cb_readpixels.h"
#include "st_cb_fbo.h"
#include "st_format.h"
#include "st_public.h"


/**
 * Do glReadPixels by getting rows from the framebuffer surface with
 * get_tile().  Convert to requested format/type with Mesa image routines.
 * Image transfer ops are done in software too.
 */
static void
st_readpixels(GLcontext *ctx, GLint x, GLint y, GLsizei width, GLsizei height,
              GLenum format, GLenum type,
              const struct gl_pixelstore_attrib *pack,
              GLvoid *dest)
{
   struct pipe_context *pipe = ctx->st->pipe;
   GLfloat temp[MAX_WIDTH][4];
   const GLbitfield transferOps = ctx->_ImageTransferState;
   GLint i, yStep, dfStride;
   GLfloat *df;
   struct st_renderbuffer *strb;
   struct gl_pixelstore_attrib clippedPacking = *pack;

   /* XXX convolution not done yet */
   assert((transferOps & IMAGE_CONVOLUTION_BIT) == 0);

   /* Do all needed clipping here, so that we can forget about it later */
   if (!_mesa_clip_readpixels(ctx, &x, &y, &width, &height, &clippedPacking)) {
      /* The ReadPixels region is totally outside the window bounds */
      return;
   }

   /* make sure rendering has completed */
   pipe->flush(pipe, 0x0);

   if (pack->BufferObj && pack->BufferObj->Name) {
      /* reading into a PBO */

   }
   else {
      /* reading into user memory/buffer */

   }

   if (format == GL_DEPTH_COMPONENT) {
      strb = st_renderbuffer(ctx->ReadBuffer->_DepthBuffer);
   }
   else {
      strb = st_renderbuffer(ctx->ReadBuffer->_ColorReadBuffer);
   }
   if (!strb)
      return;

   pipe->region_map(pipe, strb->surface->region);

   if (format == GL_RGBA && type == GL_FLOAT) {
      /* write tile(row) directly into user's buffer */
      df = (GLfloat *) _mesa_image_address2d(&clippedPacking, dest, width,
                                             height, format, type, 0, 0);
      dfStride = width * 4;
   }
#if 0
   else if (format == GL_DEPTH_COMPONENT && type == GL_FLOAT) {
      /* write tile(row) directly into user's buffer */
      df = (GLfloat *) _mesa_image_address2d(&clippedPacking, dest, width,
                                             height, format, type, 0, 0);
      dfStride = width;
   }
#endif
   else {
      /* write tile(row) into temp row buffer */
      df = (GLfloat *) temp;
      dfStride = 0;
   }

   /* determine bottom-to-top vs. top-to-bottom order */
   if (st_fb_orientation(ctx->ReadBuffer) == Y_0_TOP) {
      y = strb->Base.Height - 1 - y;
      yStep = -1;
   }
   else {
      yStep = 1;
   }

   /* Do a row at a time to flip image data vertically */
   for (i = 0; i < height; i++) {
      strb->surface->get_tile(strb->surface, x, y, width, 1, df);
      y += yStep;
      df += dfStride;
      if (!dfStride) {
         /* convert GLfloat to user's format/type */
         GLvoid *dst = _mesa_image_address2d(&clippedPacking, dest, width,
                                             height, format, type, i, 0);
         if (format == GL_DEPTH_COMPONENT) {
            _mesa_pack_depth_span(ctx, width, dst, type,
                                  (GLfloat *) temp, &clippedPacking);
         }
         else {
            _mesa_pack_rgba_span_float(ctx, width, temp, format, type, dst,
                                       &clippedPacking, transferOps);
         }
      }
   }

   pipe->region_unmap(pipe, strb->surface->region);
}


void st_init_readpixels_functions(struct dd_function_table *functions)
{
   functions->ReadPixels = st_readpixels;
}

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
#include "main/bufferobj.h"
#include "main/context.h"
#include "main/image.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_inlines.h"
#include "util/p_tile.h"
#include "st_context.h"
#include "st_cb_bitmap.h"
#include "st_cb_readpixels.h"
#include "st_cb_fbo.h"
#include "st_format.h"
#include "st_public.h"


/**
 * Special case for reading stencil buffer.
 * For color/depth we use get_tile().  For stencil, map the stencil buffer.
 */
void
st_read_stencil_pixels(GLcontext *ctx, GLint x, GLint y,
                       GLsizei width, GLsizei height, GLenum type,
                       const struct gl_pixelstore_attrib *packing,
                       GLvoid *pixels)
{
   struct gl_framebuffer *fb = ctx->ReadBuffer;
   struct st_renderbuffer *strb = st_renderbuffer(fb->_StencilBuffer);
   struct pipe_surface *ps = strb->surface;
   ubyte *stmap;
   GLint j;

   /* map the stencil buffer */
   stmap = pipe_surface_map(ps);

   /* width should never be > MAX_WIDTH since we did clipping earlier */
   ASSERT(width <= MAX_WIDTH);

   /* process image row by row */
   for (j = 0; j < height; j++, y++) {
      GLvoid *dest;
      GLstencil values[MAX_WIDTH];
      GLint srcY;

      if (st_fb_orientation(ctx->DrawBuffer) == Y_0_TOP) {
         srcY = ctx->DrawBuffer->Height - y - 1;
      }
      else {
         srcY = y;
      }

      /* get stencil values */
      switch (ps->format) {
      case PIPE_FORMAT_S8_UNORM:
         {
            const ubyte *src = stmap + srcY * ps->pitch + x;
            memcpy(values, src, width);
         }
         break;
      case PIPE_FORMAT_S8Z24_UNORM:
         {
            const uint *src = (uint *) stmap + srcY * ps->pitch + x;
            GLint k;
            for (k = 0; k < width; k++) {
               values[k] = src[k] >> 24;
            }
         }
         break;
      case PIPE_FORMAT_Z24S8_UNORM:
         {
            const uint *src = (uint *) stmap + srcY * ps->pitch + x;
            GLint k;
            for (k = 0; k < width; k++) {
               values[k] = src[k] & 0xff;
            }
         }
         break;
      default:
         assert(0);
      }

      /* store */
      dest = _mesa_image_address2d(packing, pixels, width, height,
                                   GL_STENCIL_INDEX, type, j, 0);

      _mesa_pack_stencil_span(ctx, width, type, dest, values, packing);
   }


   /* unmap the stencil buffer */
   pipe_surface_unmap(ps);
}


/**
 * Return renderbuffer to use for reading color pixels for glRead/CopyPixel
 * commands.
 * Special care is needed for the front buffer.
 */
struct st_renderbuffer *
st_get_color_read_renderbuffer(GLcontext *ctx)
{
   struct gl_framebuffer *fb = ctx->ReadBuffer;
   struct st_renderbuffer *strb =
      st_renderbuffer(fb->_ColorReadBuffer);
   struct st_renderbuffer *front = 
      st_renderbuffer(fb->Attachment[BUFFER_FRONT_LEFT].Renderbuffer);

   if (strb == front
       && ctx->st->frontbuffer_status == FRONT_STATUS_COPY_OF_BACK) {
      /* reading from front color buffer, which is a logical copy of the
       * back color buffer.
       */
      struct st_renderbuffer *back = 
         st_renderbuffer(fb->Attachment[BUFFER_BACK_LEFT].Renderbuffer);
      strb = back;
   }

   return strb;
}


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
      /* The ReadPixels surface is totally outside the window bounds */
      return;
   }

   dest = _mesa_map_readpix_pbo(ctx, &clippedPacking, dest);
   if (!dest)
      return;

   st_flush_bitmap_cache(ctx->st);

   /* make sure rendering has completed */
   pipe->flush(pipe, PIPE_FLUSH_RENDER_CACHE, NULL);

   if (format == GL_STENCIL_INDEX) {
      st_read_stencil_pixels(ctx, x, y, width, height, type, pack, dest);
      return;
   }
   else if (format == GL_DEPTH_COMPONENT) {
      strb = st_renderbuffer(ctx->ReadBuffer->_DepthBuffer);
   }
   else {
      /* Read color buffer */
      strb = st_get_color_read_renderbuffer(ctx);
   }

   if (!strb)
      return;

   if (format == GL_RGBA && type == GL_FLOAT) {
      /* write tile(row) directly into user's buffer */
      df = (GLfloat *) _mesa_image_address2d(&clippedPacking, dest, width,
                                             height, format, type, 0, 0);
      dfStride = width * 4;
   }
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

   /*
    * Copy pixels from pipe_surface to user memory
    */
   {
      /* dest of first pixel in client memory */
      GLubyte *dst = _mesa_image_address2d(&clippedPacking, dest, width,
                                           height, format, type, 0, 0);
      /* dest row stride */
      const GLint dstStride = _mesa_image_row_stride(&clippedPacking, width,
                                                     format, type);

      if (strb->surface->format == PIPE_FORMAT_S8Z24_UNORM ||
          strb->surface->format == PIPE_FORMAT_X8Z24_UNORM) {
         if (format == GL_DEPTH_COMPONENT) {
            for (i = 0; i < height; i++) {
               GLuint ztemp[MAX_WIDTH], j;
               GLfloat zfloat[MAX_WIDTH];
               const double scale = 1.0 / ((1 << 24) - 1);
               pipe_get_tile_raw(pipe, strb->surface, x, y,
                                 width, 1, ztemp, 0);
               y += yStep;
               for (j = 0; j < width; j++) {
                  zfloat[j] = (float) (scale * (ztemp[j] & 0xffffff));
               }
               _mesa_pack_depth_span(ctx, width, dst, type,
                                     zfloat, &clippedPacking);
               dst += dstStride;
            }
         }
         else {
            /* untested, but simple: */
            assert(format == GL_DEPTH_STENCIL_EXT);
            for (i = 0; i < height; i++) {
               pipe_get_tile_raw(pipe, strb->surface, x, y, width, 1, dst, 0);
               y += yStep;
               dst += dstStride;
            }
         }
      }
      else if (strb->surface->format == PIPE_FORMAT_Z16_UNORM) {
         for (i = 0; i < height; i++) {
            GLushort ztemp[MAX_WIDTH], j;
            GLfloat zfloat[MAX_WIDTH];
            const double scale = 1.0 / 0xffff;
            pipe_get_tile_raw(pipe, strb->surface, x, y, width, 1, ztemp, 0);
            y += yStep;
            for (j = 0; j < width; j++) {
               zfloat[j] = (float) (scale * ztemp[j]);
            }
            _mesa_pack_depth_span(ctx, width, dst, type,
                                  zfloat, &clippedPacking);
            dst += dstStride;
         }
      }
      else if (strb->surface->format == PIPE_FORMAT_Z32_UNORM) {
         for (i = 0; i < height; i++) {
            GLuint ztemp[MAX_WIDTH], j;
            GLfloat zfloat[MAX_WIDTH];
            const double scale = 1.0 / 0xffffffff;
            pipe_get_tile_raw(pipe, strb->surface, x, y, width, 1, ztemp, 0);
            y += yStep;
            for (j = 0; j < width; j++) {
               zfloat[j] = (float) (scale * ztemp[j]);
            }
            _mesa_pack_depth_span(ctx, width, dst, type,
                                  zfloat, &clippedPacking);
            dst += dstStride;
         }
      }
      else {
         /* RGBA format */
         /* Do a row at a time to flip image data vertically */
         for (i = 0; i < height; i++) {
            pipe_get_tile_rgba(pipe, strb->surface, x, y, width, 1, df);
            y += yStep;
            df += dfStride;
            if (!dfStride) {
               _mesa_pack_rgba_span_float(ctx, width, temp, format, type, dst,
                                          &clippedPacking, transferOps);
               dst += dstStride;
            }
         }
      }
   }

   _mesa_unmap_readpix_pbo(ctx, &clippedPacking);
}


void st_init_readpixels_functions(struct dd_function_table *functions)
{
   functions->ReadPixels = st_readpixels;
}

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
#include "util/u_inlines.h"
#include "util/u_tile.h"

#include "st_debug.h"
#include "st_context.h"
#include "st_cb_readpixels.h"
#include "st_cb_fbo.h"
#include "st_public.h"
#include "st_texture.h"
#include "st_inlines.h"

/**
 * Special case for reading stencil buffer.
 * For color/depth we use get_tile().  For stencil, map the stencil buffer.
 */
void
st_read_stencil_pixels(GLcontext *ctx, GLint x, GLint y,
                       GLsizei width, GLsizei height,
                       GLenum format, GLenum type,
                       const struct gl_pixelstore_attrib *packing,
                       GLvoid *pixels)
{
   struct gl_framebuffer *fb = ctx->ReadBuffer;
   struct pipe_screen *screen = ctx->st->pipe->screen;
   struct st_renderbuffer *strb = st_renderbuffer(fb->_StencilBuffer);
   struct pipe_transfer *pt;
   ubyte *stmap;
   GLint j;

   if (st_fb_orientation(ctx->DrawBuffer) == Y_0_TOP) {
      y = ctx->DrawBuffer->Height - y - height;
   }

   /* Create a read transfer from the renderbuffer's texture */

   pt = st_cond_flush_get_tex_transfer(st_context(ctx), strb->texture,
				       0, 0, 0,
				       PIPE_TRANSFER_READ, x, y,
				       width, height);

   /* map the stencil buffer */
   stmap = screen->transfer_map(screen, pt);

   /* width should never be > MAX_WIDTH since we did clipping earlier */
   ASSERT(width <= MAX_WIDTH);

   /* process image row by row */
   for (j = 0; j < height; j++) {
      GLvoid *dest;
      GLstencil sValues[MAX_WIDTH];
      GLfloat zValues[MAX_WIDTH];
      GLint srcY;

      if (st_fb_orientation(ctx->DrawBuffer) == Y_0_TOP) {
         srcY = height - j - 1;
      }
      else {
         srcY = j;
      }

      /* get stencil (and Z) values */
      switch (pt->texture->format) {
      case PIPE_FORMAT_S8_UNORM:
         {
            const ubyte *src = stmap + srcY * pt->stride;
            memcpy(sValues, src, width);
         }
         break;
      case PIPE_FORMAT_Z24S8_UNORM:
         if (format == GL_DEPTH_STENCIL) {
            const uint *src = (uint *) (stmap + srcY * pt->stride);
            const GLfloat scale = 1.0f / (0xffffff);
            GLint k;
            for (k = 0; k < width; k++) {
               sValues[k] = src[k] >> 24;
               zValues[k] = (src[k] & 0xffffff) * scale;
            }
         }
         else {
            const uint *src = (uint *) (stmap + srcY * pt->stride);
            GLint k;
            for (k = 0; k < width; k++) {
               sValues[k] = src[k] >> 24;
            }
         }
         break;
      case PIPE_FORMAT_S8Z24_UNORM:
         if (format == GL_DEPTH_STENCIL) {
            const uint *src = (uint *) (stmap + srcY * pt->stride);
            const GLfloat scale = 1.0f / (0xffffff);
            GLint k;
            for (k = 0; k < width; k++) {
               sValues[k] = src[k] & 0xff;
               zValues[k] = (src[k] >> 8) * scale;
            }
         }
         else {
            const uint *src = (uint *) (stmap + srcY * pt->stride);
            GLint k;
            for (k = 0; k < width; k++) {
               sValues[k] = src[k] & 0xff;
            }
         }
         break;
      default:
         assert(0);
      }

      /* store */
      dest = _mesa_image_address2d(packing, pixels, width, height,
                                   format, type, j, 0);
      if (format == GL_DEPTH_STENCIL) {
         _mesa_pack_depth_stencil_span(ctx, width, dest,
                                       zValues, sValues, packing);
      }
      else {
         _mesa_pack_stencil_span(ctx, width, type, dest, sValues, packing);
      }
   }

   /* unmap the stencil buffer */
   screen->transfer_unmap(screen, pt);
   screen->tex_transfer_destroy(pt);
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
 * Try to do glReadPixels in a fast manner for common cases.
 * \return GL_TRUE for success, GL_FALSE for failure
 */
static GLboolean
st_fast_readpixels(GLcontext *ctx, struct st_renderbuffer *strb,
                   GLint x, GLint y, GLsizei width, GLsizei height,
                   GLenum format, GLenum type,
                   const struct gl_pixelstore_attrib *pack,
                   GLvoid *dest)
{
   enum combination {
      A8R8G8B8_UNORM_TO_RGBA_UBYTE,
      A8R8G8B8_UNORM_TO_RGB_UBYTE,
      A8R8G8B8_UNORM_TO_BGRA_UINT
   } combo;

   if (ctx->_ImageTransferState)
      return GL_FALSE;

   if (strb->format == PIPE_FORMAT_B8G8R8A8_UNORM &&
       format == GL_RGBA && type == GL_UNSIGNED_BYTE) {
      combo = A8R8G8B8_UNORM_TO_RGBA_UBYTE;
   }
   else if (strb->format == PIPE_FORMAT_B8G8R8A8_UNORM &&
            format == GL_RGB && type == GL_UNSIGNED_BYTE) {
      combo = A8R8G8B8_UNORM_TO_RGB_UBYTE;
   }
   else if (strb->format == PIPE_FORMAT_B8G8R8A8_UNORM &&
            format == GL_BGRA && type == GL_UNSIGNED_INT_8_8_8_8_REV) {
      combo = A8R8G8B8_UNORM_TO_BGRA_UINT;
   }
   else {
      return GL_FALSE;
   }

   /*printf("st_fast_readpixels combo %d\n", (GLint) combo);*/

   {
      struct pipe_context *pipe = ctx->st->pipe;
      struct pipe_screen *screen = pipe->screen;
      struct pipe_transfer *trans;
      const GLubyte *map;
      GLubyte *dst;
      GLint row, col, dy, dstStride;

      if (st_fb_orientation(ctx->ReadBuffer) == Y_0_TOP) {
         y = strb->texture->height0 - y - height;
      }

      trans = st_cond_flush_get_tex_transfer(st_context(ctx), strb->texture,
					     0, 0, 0,
					     PIPE_TRANSFER_READ, x, y,
					     width, height);
      if (!trans) {
         return GL_FALSE;
      }

      map = screen->transfer_map(screen, trans);
      if (!map) {
         screen->tex_transfer_destroy(trans);
         return GL_FALSE;
      }

      if (st_fb_orientation(ctx->ReadBuffer) == Y_0_TOP) {
         y = height - 1;
         dy = -1;
      }
      else {
         y = 0;
         dy = 1;
      }

      dst = _mesa_image_address2d(pack, dest, width, height,
                                  format, type, 0, 0);
      dstStride = _mesa_image_row_stride(pack, width, format, type);

      switch (combo) {
      case A8R8G8B8_UNORM_TO_RGBA_UBYTE:
         for (row = 0; row < height; row++) {
            const GLubyte *src = map + y * trans->stride;
            for (col = 0; col < width; col++) {
               GLuint pixel = ((GLuint *) src)[col];
               dst[col*4+0] = (pixel >> 16) & 0xff;
               dst[col*4+1] = (pixel >>  8) & 0xff;
               dst[col*4+2] = (pixel >>  0) & 0xff;
               dst[col*4+3] = (pixel >> 24) & 0xff;
            }
            dst += dstStride;
            y += dy;
         }
         break;
      case A8R8G8B8_UNORM_TO_RGB_UBYTE:
         for (row = 0; row < height; row++) {
            const GLubyte *src = map + y * trans->stride;
            for (col = 0; col < width; col++) {
               GLuint pixel = ((GLuint *) src)[col];
               dst[col*3+0] = (pixel >> 16) & 0xff;
               dst[col*3+1] = (pixel >>  8) & 0xff;
               dst[col*3+2] = (pixel >>  0) & 0xff;
            }
            dst += dstStride;
            y += dy;
         }
         break;
      case A8R8G8B8_UNORM_TO_BGRA_UINT:
         for (row = 0; row < height; row++) {
            const GLubyte *src = map + y * trans->stride;
            memcpy(dst, src, 4 * width);
            dst += dstStride;
            y += dy;
         }
         break;
      default:
         ; /* nothing */
      }

      screen->transfer_unmap(screen, trans);
      screen->tex_transfer_destroy(trans);
   }

   return GL_TRUE;
}


/**
 * Do glReadPixels by getting rows from the framebuffer transfer with
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
   struct pipe_screen *screen = pipe->screen;
   GLfloat temp[MAX_WIDTH][4];
   const GLbitfield transferOps = ctx->_ImageTransferState;
   GLsizei i, j;
   GLint yStep, dfStride;
   GLfloat *df;
   struct st_renderbuffer *strb;
   struct gl_pixelstore_attrib clippedPacking = *pack;
   struct pipe_transfer *trans;

   assert(ctx->ReadBuffer->Width > 0);

   /* XXX convolution not done yet */
   assert((transferOps & IMAGE_CONVOLUTION_BIT) == 0);

   /* Do all needed clipping here, so that we can forget about it later */
   if (!_mesa_clip_readpixels(ctx, &x, &y, &width, &height, &clippedPacking)) {
      /* The ReadPixels transfer is totally outside the window bounds */
      return;
   }

   dest = _mesa_map_pbo_dest(ctx, &clippedPacking, dest);
   if (!dest)
      return;

   st_flush(ctx->st, PIPE_FLUSH_RENDER_CACHE, NULL);

   if (format == GL_STENCIL_INDEX ||
       format == GL_DEPTH_STENCIL) {
      st_read_stencil_pixels(ctx, x, y, width, height,
                             format, type, pack, dest);
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

   /* try a fast-path readpixels before anything else */
   if (st_fast_readpixels(ctx, strb, x, y, width, height,
                          format, type, pack, dest)) {
      /* success! */
      _mesa_unmap_pbo_dest(ctx, &clippedPacking);
      return;
   }

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

   if (st_fb_orientation(ctx->ReadBuffer) == Y_0_TOP) {
      y = strb->Base.Height - y - height;
   }

   /* Create a read transfer from the renderbuffer's texture */
   trans = st_cond_flush_get_tex_transfer(st_context(ctx), strb->texture,
					  0, 0, 0,
					  PIPE_TRANSFER_READ, x, y,
					  width, height);

   /* determine bottom-to-top vs. top-to-bottom order */
   if (st_fb_orientation(ctx->ReadBuffer) == Y_0_TOP) {
      y = height - 1;
      yStep = -1;
   }
   else {
      y = 0;
      yStep = 1;
   }

   if (ST_DEBUG & DEBUG_FALLBACK)
      debug_printf("%s: fallback processing\n", __FUNCTION__);

   /*
    * Copy pixels from pipe_transfer to user memory
    */
   {
      /* dest of first pixel in client memory */
      GLubyte *dst = _mesa_image_address2d(&clippedPacking, dest, width,
                                           height, format, type, 0, 0);
      /* dest row stride */
      const GLint dstStride = _mesa_image_row_stride(&clippedPacking, width,
                                                     format, type);

      if (trans->texture->format == PIPE_FORMAT_Z24S8_UNORM ||
          trans->texture->format == PIPE_FORMAT_Z24X8_UNORM) {
         if (format == GL_DEPTH_COMPONENT) {
            for (i = 0; i < height; i++) {
               GLuint ztemp[MAX_WIDTH];
               GLfloat zfloat[MAX_WIDTH];
               const double scale = 1.0 / ((1 << 24) - 1);
               pipe_get_tile_raw(trans, 0, y, width, 1, ztemp, 0);
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
            /* XXX: unreachable code -- should be before st_read_stencil_pixels */
            assert(format == GL_DEPTH_STENCIL_EXT);
            for (i = 0; i < height; i++) {
               GLuint *zshort = (GLuint *)dst;
               pipe_get_tile_raw(trans, 0, y, width, 1, dst, 0);
               y += yStep;
               /* Reverse into 24/8 */
               for (j = 0; j < width; j++) {
                  zshort[j] = (zshort[j] << 8) | (zshort[j] >> 24);
               }
               dst += dstStride;
            }
         }
      }
      else if (trans->texture->format == PIPE_FORMAT_S8Z24_UNORM ||
               trans->texture->format == PIPE_FORMAT_X8Z24_UNORM) {
         if (format == GL_DEPTH_COMPONENT) {
            for (i = 0; i < height; i++) {
               GLuint ztemp[MAX_WIDTH];
               GLfloat zfloat[MAX_WIDTH];
               const double scale = 1.0 / ((1 << 24) - 1);
               pipe_get_tile_raw(trans, 0, y, width, 1, ztemp, 0);
               y += yStep;
               for (j = 0; j < width; j++) {
                  zfloat[j] = (float) (scale * ((ztemp[j] >> 8) & 0xffffff));
               }
               _mesa_pack_depth_span(ctx, width, dst, type,
                                     zfloat, &clippedPacking);
               dst += dstStride;
            }
         }
         else {
            /* XXX: unreachable code -- should be before st_read_stencil_pixels */
            assert(format == GL_DEPTH_STENCIL_EXT);
            for (i = 0; i < height; i++) {
               pipe_get_tile_raw(trans, 0, y, width, 1, dst, 0);
               y += yStep;
               dst += dstStride;
            }
         }
      }
      else if (trans->texture->format == PIPE_FORMAT_Z16_UNORM) {
         for (i = 0; i < height; i++) {
            GLushort ztemp[MAX_WIDTH];
            GLfloat zfloat[MAX_WIDTH];
            const double scale = 1.0 / 0xffff;
            pipe_get_tile_raw(trans, 0, y, width, 1, ztemp, 0);
            y += yStep;
            for (j = 0; j < width; j++) {
               zfloat[j] = (float) (scale * ztemp[j]);
            }
            _mesa_pack_depth_span(ctx, width, dst, type,
                                  zfloat, &clippedPacking);
            dst += dstStride;
         }
      }
      else if (trans->texture->format == PIPE_FORMAT_Z32_UNORM) {
         for (i = 0; i < height; i++) {
            GLuint ztemp[MAX_WIDTH];
            GLfloat zfloat[MAX_WIDTH];
            const double scale = 1.0 / 0xffffffff;
            pipe_get_tile_raw(trans, 0, y, width, 1, ztemp, 0);
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
            pipe_get_tile_rgba(trans, 0, y, width, 1, df);
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

   screen->tex_transfer_destroy(trans);

   _mesa_unmap_pbo_dest(ctx, &clippedPacking);
}


void st_init_readpixels_functions(struct dd_function_table *functions)
{
   functions->ReadPixels = st_readpixels;
}

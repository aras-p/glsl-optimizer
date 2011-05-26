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
#include "main/pack.h"
#include "main/pbo.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "util/u_format.h"
#include "util/u_inlines.h"
#include "util/u_tile.h"

#include "st_debug.h"
#include "st_context.h"
#include "st_atom.h"
#include "st_cb_bitmap.h"
#include "st_cb_readpixels.h"
#include "st_cb_fbo.h"

/**
 * Special case for reading stencil buffer.
 * For color/depth we use get_tile().  For stencil, map the stencil buffer.
 */
void
st_read_stencil_pixels(struct gl_context *ctx, GLint x, GLint y,
                       GLsizei width, GLsizei height,
                       GLenum format, GLenum type,
                       const struct gl_pixelstore_attrib *packing,
                       GLvoid *pixels)
{
   struct gl_framebuffer *fb = ctx->ReadBuffer;
   struct pipe_context *pipe = st_context(ctx)->pipe;
   struct st_renderbuffer *strb = st_renderbuffer(fb->_StencilBuffer);
   struct pipe_transfer *pt;
   ubyte *stmap;
   GLint j;

   if (strb->Base.Wrapped) {
      strb = st_renderbuffer(strb->Base.Wrapped);
   }

   if (st_fb_orientation(ctx->DrawBuffer) == Y_0_TOP) {
      y = ctx->DrawBuffer->Height - y - height;
   }

   /* Create a read transfer from the renderbuffer's texture */

   pt = pipe_get_transfer(pipe, strb->texture,
                          strb->rtt_level,
                          strb->rtt_face + strb->rtt_slice,
                          PIPE_TRANSFER_READ,
                          x, y, width, height);

   /* map the stencil buffer */
   stmap = pipe_transfer_map(pipe, pt);

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
      switch (pt->resource->format) {
      case PIPE_FORMAT_S8_USCALED:
         {
            const ubyte *src = stmap + srcY * pt->stride;
            memcpy(sValues, src, width);
         }
         break;
      case PIPE_FORMAT_Z24_UNORM_S8_USCALED:
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
      case PIPE_FORMAT_S8_USCALED_Z24_UNORM:
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
   pipe_transfer_unmap(pipe, pt);
   pipe->transfer_destroy(pipe, pt);
}


/**
 * Return renderbuffer to use for reading color pixels for glRead/CopyPixel
 * commands.
 */
struct st_renderbuffer *
st_get_color_read_renderbuffer(struct gl_context *ctx)
{
   struct gl_framebuffer *fb = ctx->ReadBuffer;
   struct st_renderbuffer *strb =
      st_renderbuffer(fb->_ColorReadBuffer);

   return strb;
}


/**
 * Try to do glReadPixels in a fast manner for common cases.
 * \return GL_TRUE for success, GL_FALSE for failure
 */
static GLboolean
st_fast_readpixels(struct gl_context *ctx, struct st_renderbuffer *strb,
                   GLint x, GLint y, GLsizei width, GLsizei height,
                   GLenum format, GLenum type,
                   const struct gl_pixelstore_attrib *pack,
                   GLvoid *dest)
{
   GLubyte alphaORoperand;
   enum combination {
      A8R8G8B8_UNORM_TO_RGBA_UBYTE,
      A8R8G8B8_UNORM_TO_RGB_UBYTE,
      A8R8G8B8_UNORM_TO_BGRA_UINT,
      A8R8G8B8_UNORM_TO_RGBA_UINT
   } combo;

   if (ctx->_ImageTransferState)
      return GL_FALSE;

   if (strb->format == PIPE_FORMAT_B8G8R8A8_UNORM) {
      alphaORoperand = 0;
   }
   else if (strb->format == PIPE_FORMAT_B8G8R8X8_UNORM ) {
      alphaORoperand = 0xff;
   }
   else {
      return GL_FALSE;
   }

   if (format == GL_RGBA && type == GL_UNSIGNED_BYTE) {
      combo = A8R8G8B8_UNORM_TO_RGBA_UBYTE;
   }
   else if (format == GL_RGB && type == GL_UNSIGNED_BYTE) {
      combo = A8R8G8B8_UNORM_TO_RGB_UBYTE;
   }
   else if (format == GL_BGRA && type == GL_UNSIGNED_INT_8_8_8_8_REV) {
      combo = A8R8G8B8_UNORM_TO_BGRA_UINT;
   }
   else if (format == GL_RGBA && type == GL_UNSIGNED_INT_8_8_8_8) {
      combo = A8R8G8B8_UNORM_TO_RGBA_UINT;
   }
   else {
      return GL_FALSE;
   }

   /*printf("st_fast_readpixels combo %d\n", (GLint) combo);*/

   {
      struct pipe_context *pipe = st_context(ctx)->pipe;
      struct pipe_transfer *trans;
      const GLubyte *map;
      GLubyte *dst;
      GLint row, col, dy, dstStride;

      if (st_fb_orientation(ctx->ReadBuffer) == Y_0_TOP) {
         /* convert GL Y to Gallium Y */
         y = strb->texture->height0 - y - height;
      }

      trans = pipe_get_transfer(pipe, strb->texture,
                                strb->rtt_level,
                                strb->rtt_face + strb->rtt_slice,
                                PIPE_TRANSFER_READ,
                                x, y, width, height);
      if (!trans) {
         return GL_FALSE;
      }

      map = pipe_transfer_map(pipe, trans);
      if (!map) {
         pipe->transfer_destroy(pipe, trans);
         return GL_FALSE;
      }

      /* We always write to the user/dest buffer from low addr to high addr
       * but the read order depends on renderbuffer orientation
       */
      if (st_fb_orientation(ctx->ReadBuffer) == Y_0_TOP) {
         /* read source rows from bottom to top */
         y = height - 1;
         dy = -1;
      }
      else {
         /* read source rows from top to bottom */
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
               dst[col*4+3] = ((pixel >> 24) & 0xff) | alphaORoperand;
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
            if (alphaORoperand) {
               assert(alphaORoperand == 0xff);
               for (col = 0; col < width; col++) {
                  dst[col*4+3] = 0xff;
               }
            }
            dst += dstStride;
            y += dy;
         }
         break;
      case A8R8G8B8_UNORM_TO_RGBA_UINT:
         for (row = 0; row < height; row++) {
            const GLubyte *src = map + y * trans->stride;
            for (col = 0; col < width; col++) {
               GLuint pixel = ((GLuint *) src)[col];
               dst[col*4+0] = ((pixel >> 24) & 0xff) | alphaORoperand;
               dst[col*4+1] = (pixel >> 0) & 0xff;
               dst[col*4+2] = (pixel >> 8) & 0xff;
               dst[col*4+3] = (pixel >> 16) & 0xff;
            }
            dst += dstStride;
            y += dy;
         }
         break;
      default:
         ; /* nothing */
      }

      pipe_transfer_unmap(pipe, trans);
      pipe->transfer_destroy(pipe, trans);
   }

   return GL_TRUE;
}


/**
 * Do glReadPixels by getting rows from the framebuffer transfer with
 * get_tile().  Convert to requested format/type with Mesa image routines.
 * Image transfer ops are done in software too.
 */
static void
st_readpixels(struct gl_context *ctx, GLint x, GLint y, GLsizei width, GLsizei height,
              GLenum format, GLenum type,
              const struct gl_pixelstore_attrib *pack,
              GLvoid *dest)
{
   struct st_context *st = st_context(ctx);
   struct pipe_context *pipe = st->pipe;
   GLfloat (*temp)[4];
   GLbitfield transferOps = ctx->_ImageTransferState;
   GLsizei i, j;
   GLint yStep, dfStride;
   GLfloat *df;
   struct st_renderbuffer *strb;
   struct gl_pixelstore_attrib clippedPacking = *pack;
   struct pipe_transfer *trans;
   enum pipe_format pformat;

   assert(ctx->ReadBuffer->Width > 0);

   st_validate_state(st);

   /* Do all needed clipping here, so that we can forget about it later */
   if (!_mesa_clip_readpixels(ctx, &x, &y, &width, &height, &clippedPacking)) {
      /* The ReadPixels transfer is totally outside the window bounds */
      return;
   }

   st_flush_bitmap_cache(st);

   dest = _mesa_map_pbo_dest(ctx, &clippedPacking, dest);
   if (!dest)
      return;

   if (format == GL_STENCIL_INDEX ||
       format == GL_DEPTH_STENCIL) {
      st_read_stencil_pixels(ctx, x, y, width, height,
                             format, type, pack, dest);
      return;
   }
   else if (format == GL_DEPTH_COMPONENT) {
      strb = st_renderbuffer(ctx->ReadBuffer->_DepthBuffer);
      if (strb->Base.Wrapped) {
         strb = st_renderbuffer(strb->Base.Wrapped);
      }
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

   /* allocate temp pixel row buffer */
   temp = (GLfloat (*)[4]) malloc(4 * width * sizeof(GLfloat));
   if (!temp) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glReadPixels");
      return;
   }

   if(ctx->Color._ClampReadColor)
      transferOps |= IMAGE_CLAMP_BIT;

   if (format == GL_RGBA && type == GL_FLOAT && !transferOps) {
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
      /* convert GL Y to Gallium Y */
      y = strb->Base.Height - y - height;
   }

   /* Create a read transfer from the renderbuffer's texture */
   trans = pipe_get_transfer(pipe, strb->texture,
                             strb->rtt_level, /* level */
                             strb->rtt_face + strb->rtt_slice, /* layer */
                             PIPE_TRANSFER_READ,
                             x, y, width, height);

   /* determine bottom-to-top vs. top-to-bottom order */
   if (st_fb_orientation(ctx->ReadBuffer) == Y_0_TOP) {
      y = height - 1;
      yStep = -1;
   }
   else {
      y = 0;
      yStep = 1;
   }

   /* possibly convert sRGB format to linear RGB format */
   pformat = util_format_linear(trans->resource->format);

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

      if (pformat == PIPE_FORMAT_Z24_UNORM_S8_USCALED ||
          pformat == PIPE_FORMAT_Z24X8_UNORM) {
         if (format == GL_DEPTH_COMPONENT) {
            for (i = 0; i < height; i++) {
               GLuint ztemp[MAX_WIDTH];
               GLfloat zfloat[MAX_WIDTH];
               const double scale = 1.0 / ((1 << 24) - 1);
               pipe_get_tile_raw(pipe, trans, 0, y, width, 1, ztemp, 0);
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
               pipe_get_tile_raw(pipe, trans, 0, y, width, 1, dst, 0);
               y += yStep;
               /* Reverse into 24/8 */
               for (j = 0; j < width; j++) {
                  zshort[j] = (zshort[j] << 8) | (zshort[j] >> 24);
               }
               dst += dstStride;
            }
         }
      }
      else if (pformat == PIPE_FORMAT_S8_USCALED_Z24_UNORM ||
               pformat == PIPE_FORMAT_X8Z24_UNORM) {
         if (format == GL_DEPTH_COMPONENT) {
            for (i = 0; i < height; i++) {
               GLuint ztemp[MAX_WIDTH];
               GLfloat zfloat[MAX_WIDTH];
               const double scale = 1.0 / ((1 << 24) - 1);
               pipe_get_tile_raw(pipe, trans, 0, y, width, 1, ztemp, 0);
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
               pipe_get_tile_raw(pipe, trans, 0, y, width, 1, dst, 0);
               y += yStep;
               dst += dstStride;
            }
         }
      }
      else if (pformat == PIPE_FORMAT_Z16_UNORM) {
         for (i = 0; i < height; i++) {
            GLushort ztemp[MAX_WIDTH];
            GLfloat zfloat[MAX_WIDTH];
            const double scale = 1.0 / 0xffff;
            pipe_get_tile_raw(pipe, trans, 0, y, width, 1, ztemp, 0);
            y += yStep;
            for (j = 0; j < width; j++) {
               zfloat[j] = (float) (scale * ztemp[j]);
            }
            _mesa_pack_depth_span(ctx, width, dst, type,
                                  zfloat, &clippedPacking);
            dst += dstStride;
         }
      }
      else if (pformat == PIPE_FORMAT_Z32_UNORM) {
         for (i = 0; i < height; i++) {
            GLuint ztemp[MAX_WIDTH];
            GLfloat zfloat[MAX_WIDTH];
            const double scale = 1.0 / 0xffffffff;
            pipe_get_tile_raw(pipe, trans, 0, y, width, 1, ztemp, 0);
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
            pipe_get_tile_rgba_format(pipe, trans, 0, y, width, 1,
                                      pformat, df);
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

   free(temp);

   pipe->transfer_destroy(pipe, trans);

   _mesa_unmap_pbo_dest(ctx, &clippedPacking);
}


void st_init_readpixels_functions(struct dd_function_table *functions)
{
   functions->ReadPixels = st_readpixels;
}

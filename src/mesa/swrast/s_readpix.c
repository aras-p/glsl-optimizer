/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include "glheader.h"
#include "bufferobj.h"
#include "colormac.h"
#include "convolve.h"
#include "context.h"
#include "feedback.h"
#include "image.h"
#include "macros.h"
#include "imports.h"
#include "pixel.h"

#include "s_context.h"
#include "s_depth.h"
#include "s_span.h"
#include "s_stencil.h"


/*
 * Read a block of color index pixels.
 */
static void
read_index_pixels( GLcontext *ctx,
                   GLint x, GLint y,
                   GLsizei width, GLsizei height,
                   GLenum type, GLvoid *pixels,
                   const struct gl_pixelstore_attrib *packing )
{
   struct gl_renderbuffer *rb = ctx->ReadBuffer->_ColorReadBuffer;
   GLint i;

   ASSERT(rb);

   /* width should never be > MAX_WIDTH since we did clipping earlier */
   ASSERT(width <= MAX_WIDTH);

   /* process image row by row */
   for (i = 0; i < height; i++) {
      GLuint index[MAX_WIDTH];
      GLvoid *dest;
      ASSERT(rb->DataType == GL_UNSIGNED_INT);
      rb->GetRow(ctx, rb, width, x, y + i, index);

      dest = _mesa_image_address2d(packing, pixels, width, height,
                                   GL_COLOR_INDEX, type, i, 0);

      _mesa_pack_index_span(ctx, width, type, dest, index,
                            &ctx->Pack, ctx->_ImageTransferState);
   }
}



/**
 * Read pixels for format=GL_DEPTH_COMPONENT.
 */
static void
read_depth_pixels( GLcontext *ctx,
                   GLint x, GLint y,
                   GLsizei width, GLsizei height,
                   GLenum type, GLvoid *pixels,
                   const struct gl_pixelstore_attrib *packing )
{
   struct gl_framebuffer *fb = ctx->ReadBuffer;
   struct gl_renderbuffer *rb = fb->Attachment[BUFFER_DEPTH].Renderbuffer;
   GLboolean bias_or_scale;

   /* clipping should have been done already */
   ASSERT(x >= 0);
   ASSERT(y >= 0);
   ASSERT(x + width <= rb->Width);
   ASSERT(y + height <= rb->Height);
   /* width should never be > MAX_WIDTH since we did clipping earlier */
   ASSERT(width <= MAX_WIDTH);

   ASSERT(rb);

   bias_or_scale = ctx->Pixel.DepthBias != 0.0 || ctx->Pixel.DepthScale != 1.0;

   if (type == GL_UNSIGNED_SHORT && rb->DepthBits == 16
       && !bias_or_scale && !packing->SwapBytes) {
      /* Special case: directly read 16-bit unsigned depth values. */
      GLint j;
      ASSERT(rb->InternalFormat == GL_DEPTH_COMPONENT16);
      ASSERT(rb->DataType == GL_UNSIGNED_SHORT);
      for (j = 0; j < height; j++, y++) {
         void *dest =_mesa_image_address2d(packing, pixels, width, height,
                                           GL_DEPTH_COMPONENT, type, j, 0);
         rb->GetRow(ctx, rb, width, x, y, dest);
      }
   }
   else if (type == GL_UNSIGNED_INT && rb->DepthBits == 24
            && !bias_or_scale && !packing->SwapBytes) {
      /* Special case: directly read 24-bit unsigned depth values. */
      GLint j;
      ASSERT(rb->InternalFormat == GL_DEPTH_COMPONENT32);
      ASSERT(rb->DataType == GL_UNSIGNED_INT);
      for (j = 0; j < height; j++, y++) {
         GLuint *dest = (GLuint *)
            _mesa_image_address2d(packing, pixels, width, height,
                                  GL_DEPTH_COMPONENT, type, j, 0);
         GLint k;
         rb->GetRow(ctx, rb, width, x, y, dest);
         /* convert range from 24-bit to 32-bit */
         for (k = 0; k < width; k++) {
            dest[k] = (dest[k] << 8) | (dest[k] >> 24);
         }
      }
   }
   else if (type == GL_UNSIGNED_INT && rb->DepthBits == 32
            && !bias_or_scale && !packing->SwapBytes) {
      /* Special case: directly read 32-bit unsigned depth values. */
      GLint j;
      ASSERT(rb->InternalFormat == GL_DEPTH_COMPONENT32);
      ASSERT(rb->DataType == GL_UNSIGNED_INT);
      for (j = 0; j < height; j++, y++) {
         void *dest = _mesa_image_address2d(packing, pixels, width, height,
                                            GL_DEPTH_COMPONENT, type, j, 0);
         rb->GetRow(ctx, rb, width, x, y, dest);
      }
   }
   else {
      /* General case (slower) */
      GLint j;
      for (j = 0; j < height; j++, y++) {
         GLfloat depthValues[MAX_WIDTH];
         GLvoid *dest = _mesa_image_address2d(packing, pixels, width, height,
                                              GL_DEPTH_COMPONENT, type, j, 0);
         _swrast_read_depth_span_float(ctx, rb, width, x, y, depthValues);
         _mesa_pack_depth_span(ctx, width, dest, type, depthValues, packing);
      }
   }
}


/**
 * Read pixels for format=GL_STENCIL_INDEX.
 */
static void
read_stencil_pixels( GLcontext *ctx,
                     GLint x, GLint y,
                     GLsizei width, GLsizei height,
                     GLenum type, GLvoid *pixels,
                     const struct gl_pixelstore_attrib *packing )
{
   struct gl_framebuffer *fb = ctx->ReadBuffer;
   struct gl_renderbuffer *rb = fb->Attachment[BUFFER_STENCIL].Renderbuffer;
   GLint j;

   ASSERT(rb);

   /* width should never be > MAX_WIDTH since we did clipping earlier */
   ASSERT(width <= MAX_WIDTH);

   /* process image row by row */
   for (j=0;j<height;j++,y++) {
      GLvoid *dest;
      GLstencil stencil[MAX_WIDTH];

      _swrast_read_stencil_span(ctx, rb, width, x, y, stencil);

      dest = _mesa_image_address2d(packing, pixels, width, height,
                                   GL_STENCIL_INDEX, type, j, 0);

      _mesa_pack_stencil_span(ctx, width, type, dest, stencil, packing);
   }
}



/**
 * Optimized glReadPixels for particular pixel formats:
 *   GL_UNSIGNED_BYTE, GL_RGBA
 * when pixel scaling, biasing and mapping are disabled.
 */
static GLboolean
read_fast_rgba_pixels( GLcontext *ctx,
                       GLint x, GLint y,
                       GLsizei width, GLsizei height,
                       GLenum format, GLenum type,
                       GLvoid *pixels,
                       const struct gl_pixelstore_attrib *packing )
{
   struct gl_renderbuffer *rb = ctx->ReadBuffer->_ColorReadBuffer;

   /* can't do scale, bias, mapping, etc */
   if (ctx->_ImageTransferState)
       return GL_FALSE;

   /* can't do fancy pixel packing */
   if (packing->Alignment != 1 || packing->SwapBytes || packing->LsbFirst)
      return GL_FALSE;

   {
      GLint srcX = x;
      GLint srcY = y;
      GLint readWidth = width;           /* actual width read */
      GLint readHeight = height;         /* actual height read */
      GLint skipPixels = packing->SkipPixels;
      GLint skipRows = packing->SkipRows;
      GLint rowLength;

      if (packing->RowLength > 0)
         rowLength = packing->RowLength;
      else
         rowLength = width;

      /*
       * Ready to read!
       * The window region at (destX, destY) of size (readWidth, readHeight)
       * will be read back.
       * We'll write pixel data to buffer pointed to by "pixels" but we'll
       * skip "skipRows" rows and skip "skipPixels" pixels/row.
       */
#if CHAN_BITS == 8
      if (format == GL_RGBA && type == GL_UNSIGNED_BYTE)
#elif CHAN_BITS == 16
      if (format == GL_RGBA && type == GL_UNSIGNED_SHORT)
#else
      if (0)
#endif
      {
         GLchan *dest = (GLchan *) pixels
                      + (skipRows * rowLength + skipPixels) * 4;
         GLint row;

         if (packing->Invert) {
            /* start at top and go down */
            dest += (readHeight - 1) * rowLength * 4;
            rowLength = -rowLength;
         }

         ASSERT(rb->GetRow);
         for (row=0; row<readHeight; row++) {
            rb->GetRow(ctx, rb, readWidth, srcX, srcY, dest);
            dest += rowLength * 4;
            srcY++;
         }
         return GL_TRUE;
      }
      else {
         /* can't do this format/type combination */
         return GL_FALSE;
      }
   }
}



/*
 * Read R, G, B, A, RGB, L, or LA pixels.
 */
static void
read_rgba_pixels( GLcontext *ctx,
                  GLint x, GLint y,
                  GLsizei width, GLsizei height,
                  GLenum format, GLenum type, GLvoid *pixels,
                  const struct gl_pixelstore_attrib *packing )
{
   struct gl_framebuffer *fb = ctx->ReadBuffer;
   struct gl_renderbuffer *rb = fb->_ColorReadBuffer;

   ASSERT(rb);

   /* Try optimized path first */
   if (read_fast_rgba_pixels( ctx, x, y, width, height,
                              format, type, pixels, packing )) {
      return; /* done! */
   }

   /* width should never be > MAX_WIDTH since we did clipping earlier */
   ASSERT(width <= MAX_WIDTH);

   if (ctx->Pixel.Convolution2DEnabled || ctx->Pixel.Separable2DEnabled) {
      const GLuint transferOps = ctx->_ImageTransferState;
      GLfloat *dest, *src, *tmpImage, *convImage;
      GLint row;

      tmpImage = (GLfloat *) _mesa_malloc(width * height * 4 * sizeof(GLfloat));
      if (!tmpImage) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glReadPixels");
         return;
      }
      convImage = (GLfloat *) _mesa_malloc(width * height * 4 * sizeof(GLfloat));
      if (!convImage) {
         _mesa_free(tmpImage);
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glReadPixels");
         return;
      }

      /* read full RGBA, FLOAT image */
      dest = tmpImage;
      for (row = 0; row < height; row++, y++) {
         GLchan rgba[MAX_WIDTH][4];
         if (fb->Visual.rgbMode) {
            _swrast_read_rgba_span(ctx, rb, width, x, y, rgba);
         }
         else {
            GLuint index[MAX_WIDTH];
            ASSERT(rb->DataType == GL_UNSIGNED_INT);
            rb->GetRow(ctx, rb, width, x, y, index);
            if (ctx->Pixel.IndexShift != 0 || ctx->Pixel.IndexOffset !=0 ) {
               _mesa_map_ci(ctx, width, index);
            }
            _mesa_map_ci_to_rgba_chan(ctx, width, index, rgba);
         }
         _mesa_pack_rgba_span_chan(ctx, width, (const GLchan (*)[4]) rgba,
                              GL_RGBA, GL_FLOAT, dest, &ctx->DefaultPacking,
                              transferOps & IMAGE_PRE_CONVOLUTION_BITS);
         dest += width * 4;
      }

      /* do convolution */
      if (ctx->Pixel.Convolution2DEnabled) {
         _mesa_convolve_2d_image(ctx, &width, &height, tmpImage, convImage);
      }
      else {
         ASSERT(ctx->Pixel.Separable2DEnabled);
         _mesa_convolve_sep_image(ctx, &width, &height, tmpImage, convImage);
      }
      _mesa_free(tmpImage);

      /* finish transfer ops and pack the resulting image */
      src = convImage;
      for (row = 0; row < height; row++) {
         GLvoid *dest;
         dest = _mesa_image_address2d(packing, pixels, width, height,
                                      format, type, row, 0);
         _mesa_pack_rgba_span_float(ctx, width,
                                    (const GLfloat (*)[4]) src,
                                    format, type, dest, packing,
                                    transferOps & IMAGE_POST_CONVOLUTION_BITS);
         src += width * 4;
      }
   }
   else {
      /* no convolution */
      GLint row;
      for (row = 0; row < height; row++, y++) {
         GLchan rgba[MAX_WIDTH][4];
         GLvoid *dst;
         if (fb->Visual.rgbMode) {
            _swrast_read_rgba_span(ctx, rb, width, x, y, rgba);
         }
         else {
            GLuint index[MAX_WIDTH];
            ASSERT(rb->DataType == GL_UNSIGNED_INT);
            rb->GetRow(ctx, rb, width, x, y, index);
            if (ctx->Pixel.IndexShift != 0 || ctx->Pixel.IndexOffset != 0) {
               _mesa_map_ci(ctx, width, index);
            }
            _mesa_map_ci_to_rgba_chan(ctx, width, index, rgba);
         }
         dst = _mesa_image_address2d(packing, pixels, width, height,
                                     format, type, row, 0);
         if (fb->Visual.redBits < CHAN_BITS ||
             fb->Visual.greenBits < CHAN_BITS ||
             fb->Visual.blueBits < CHAN_BITS) {
            /* Requantize the color values into floating point and go from
             * there.  This fixes conformance failures with 16-bit color
             * buffers, for example.
             */
            GLfloat rgbaf[MAX_WIDTH][4];
            _mesa_chan_to_float_span(ctx, width,
                                     (CONST GLchan (*)[4]) rgba, rgbaf);
            _mesa_pack_rgba_span_float(ctx, width,
                                       (CONST GLfloat (*)[4]) rgbaf,
                                       format, type, dst, packing,
                                       ctx->_ImageTransferState);
         }
         else {
            /* GLubytes are fine */
            _mesa_pack_rgba_span_chan(ctx, width, (CONST GLchan (*)[4]) rgba,
                                 format, type, dst, packing,
                                 ctx->_ImageTransferState);
         }
      }
   }
}


/**
 * Read combined depth/stencil values.
 * We'll have already done error checking to be sure the expected
 * depth and stencil buffers really exist.
 */
static void
read_depth_stencil_pixels(GLcontext *ctx,
                          GLint x, GLint y,
                          GLsizei width, GLsizei height,
                          GLenum type, GLvoid *pixels,
                          const struct gl_pixelstore_attrib *packing )
{
   const GLboolean scaleOrBias
      = ctx->Pixel.DepthScale != 1.0 || ctx->Pixel.DepthBias != 0.0;
   const GLboolean stencilTransfer = ctx->Pixel.IndexShift
      || ctx->Pixel.IndexOffset || ctx->Pixel.MapStencilFlag;
   struct gl_renderbuffer *depthRb, *stencilRb;
   GLint i;

   depthRb = ctx->ReadBuffer->Attachment[BUFFER_DEPTH].Renderbuffer;
   stencilRb = ctx->ReadBuffer->Attachment[BUFFER_STENCIL].Renderbuffer;

   ASSERT(depthRb);
   ASSERT(stencilRb);

   for (i = 0; i < height; i++) {
      GLstencil stencilVals[MAX_WIDTH];

      GLuint *depthStencilDst = (GLuint *)
         _mesa_image_address2d(packing, pixels, width, height,
                               GL_DEPTH_STENCIL_EXT, type, i, 0);

      _swrast_read_stencil_span(ctx, stencilRb, width, x, y + i, stencilVals);

      if (!scaleOrBias && !stencilTransfer && depthRb->DepthBits == 24) {
         /* ideal case */
         GLuint zVals[MAX_WIDTH]; /* 24-bit values! */
         GLint j;
         ASSERT(depthRb->DataType == GL_UNSIGNED_INT);
         /* note, we've already been clipped */
         depthRb->GetRow(ctx, depthRb, width, x, y + i, zVals);
         for (j = 0; j < width; j++) {
            depthStencilDst[j] = (zVals[j] << 8) | (stencilVals[j] & 0xff);
         }
      }
      else {
         /* general case */
         GLfloat depthVals[MAX_WIDTH];
         _swrast_read_depth_span_float(ctx, depthRb, width, x, y + i,
                                       depthVals);
         _mesa_pack_depth_stencil_span(ctx, width, depthStencilDst,
                                       depthVals, stencilVals, packing);
      }
   }
}



/**
 * Software fallback routine for ctx->Driver.ReadPixels().
 * By time we get here, all error checking will have been done.
 */
void
_swrast_ReadPixels( GLcontext *ctx,
		    GLint x, GLint y, GLsizei width, GLsizei height,
		    GLenum format, GLenum type,
		    const struct gl_pixelstore_attrib *packing,
		    GLvoid *pixels )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   struct gl_pixelstore_attrib clippedPacking = *packing;

   if (swrast->NewState)
      _swrast_validate_derived( ctx );

   /* Do all needed clipping here, so that we can forget about it later */
   if (!_mesa_clip_readpixels(ctx, &x, &y, &width, &height, &clippedPacking)) {
      /* The ReadPixels region is totally outside the window bounds */
      return;
   }

   if (clippedPacking.BufferObj->Name) {
      /* pack into PBO */
      GLubyte *buf;
      if (!_mesa_validate_pbo_access(2, &clippedPacking, width, height, 1,
                                     format, type, pixels)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glReadPixels(invalid PBO access)");
         return;
      }
      buf = (GLubyte *) ctx->Driver.MapBuffer(ctx, GL_PIXEL_PACK_BUFFER_EXT,
                                              GL_WRITE_ONLY_ARB,
                                              clippedPacking.BufferObj);
      if (!buf) {
         /* buffer is already mapped - that's an error */
         _mesa_error(ctx, GL_INVALID_OPERATION, "glReadPixels(PBO is mapped)");
         return;
      }
      pixels = ADD_POINTERS(buf, pixels);
   }

   RENDER_START(swrast, ctx);

   switch (format) {
      case GL_COLOR_INDEX:
         read_index_pixels(ctx, x, y, width, height, type, pixels,
                           &clippedPacking);
	 break;
      case GL_STENCIL_INDEX:
	 read_stencil_pixels(ctx, x, y, width, height, type, pixels,
                             &clippedPacking);
         break;
      case GL_DEPTH_COMPONENT:
	 read_depth_pixels(ctx, x, y, width, height, type, pixels,
                           &clippedPacking);
	 break;
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
      case GL_RGB:
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
      case GL_RGBA:
      case GL_BGR:
      case GL_BGRA:
      case GL_ABGR_EXT:
         read_rgba_pixels(ctx, x, y, width, height,
                          format, type, pixels, &clippedPacking);
	 break;
      case GL_DEPTH_STENCIL_EXT:
         read_depth_stencil_pixels(ctx, x, y, width, height,
                                   type, pixels, &clippedPacking);
         break;
      default:
	 _mesa_problem(ctx, "unexpected format in _swrast_ReadPixels");
         /* don't return yet, clean-up */
   }

   RENDER_FINISH(swrast, ctx);

   if (clippedPacking.BufferObj->Name) {
      /* done with PBO so unmap it now */
      ctx->Driver.UnmapBuffer(ctx, GL_PIXEL_PACK_BUFFER_EXT,
                              clippedPacking.BufferObj);
   }
}

/* $Id: drawpix.c,v 1.30 2000/08/23 14:32:06 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 * 
 * Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
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


#ifdef PC_HEADER
#include "all.h"
#else
#include "glheader.h"
#include "context.h"
#include "convolve.h"
#include "drawpix.h"
#include "feedback.h"
#include "image.h"
#include "macros.h"
#include "mem.h"
#include "mmath.h"
#include "pixel.h"
#include "pixeltex.h"
#include "span.h"
#include "state.h"
#include "stencil.h"
#include "texture.h"
#include "types.h"
#include "zoom.h"
#endif



/*
 * Given the dest position, size and skipPixels and skipRows values
 * for a glDrawPixels command, perform clipping of the image bounds
 * so the result lies withing the context's buffer bounds.
 * Return:  GL_TRUE if image is ready for drawing
 *          GL_FALSE if image was completely clipped away (draw nothing)
 */
GLboolean
_mesa_clip_pixelrect(const GLcontext *ctx,
                     GLint *destX, GLint *destY,
                     GLsizei *width, GLsizei *height,
                     GLint *skipPixels, GLint *skipRows)
{
   const GLframebuffer *buffer = ctx->DrawBuffer;

   /* left clipping */
   if (*destX < buffer->Xmin) {
      *skipPixels += (buffer->Xmin - *destX);
      *width -= (buffer->Xmin - *destX);
      *destX = buffer->Xmin;
   }
   /* right clipping */
   if (*destX + *width > buffer->Xmax)
      *width -= (*destX + *width - buffer->Xmax - 1);

   if (*width <= 0)
      return GL_FALSE;

   /* bottom clipping */
   if (*destY < buffer->Ymin) {
      *skipRows += (buffer->Ymin - *destY);
      *height -= (buffer->Ymin - *destY);
      *destY = buffer->Ymin;
   }
   /* top clipping */
   if (*destY + *height > buffer->Ymax)
      *height -= (*destY + *height - buffer->Ymax - 1);

   if (*height <= 0)
      return GL_TRUE;

   return GL_TRUE;
}



/*
 * Try to do a fast and simple RGB(a) glDrawPixels.
 * Return:  GL_TRUE if success, GL_FALSE if slow path must be used instead
 */
static GLboolean
fast_draw_pixels(GLcontext *ctx, GLint x, GLint y,
                 GLsizei width, GLsizei height,
                 GLenum format, GLenum type, const GLvoid *pixels)
{
   const GLuint cantTransferBits =
      IMAGE_SCALE_BIAS_BIT |
      IMAGE_SHIFT_OFFSET_BIT |
      IMAGE_MAP_COLOR_BIT |
      IMAGE_COLOR_TABLE_BIT |
      IMAGE_CONVOLUTION_BIT |
      IMAGE_POST_CONVOLUTION_COLOR_TABLE_BIT |
      IMAGE_COLOR_MATRIX_BIT |
      IMAGE_POST_COLOR_MATRIX_COLOR_TABLE_BIT |
      IMAGE_HISTOGRAM_BIT |
      IMAGE_MIN_MAX_BIT;
   const struct gl_pixelstore_attrib *unpack = &ctx->Unpack;
   GLubyte rgb[MAX_WIDTH][3];
   GLubyte rgba[MAX_WIDTH][4];

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH_WITH_RETVAL(ctx, "glDrawPixels", 
						  GL_FALSE);


   if (!ctx->Current.RasterPosValid) {
      return GL_TRUE;      /* no-op */
   }

   if ((ctx->RasterMask&(~(SCISSOR_BIT|WINCLIP_BIT)))==0
       && (ctx->ImageTransferState & cantTransferBits) == 0
       && ctx->Texture.ReallyEnabled == 0
       && unpack->Alignment == 1
       && !unpack->SwapBytes
       && !unpack->LsbFirst) {

      GLint destX = x;
      GLint destY = y;
      GLint drawWidth = width;           /* actual width drawn */
      GLint drawHeight = height;         /* actual height drawn */
      GLint skipPixels = unpack->SkipPixels;
      GLint skipRows = unpack->SkipRows;
      GLint rowLength;
      GLdepth zSpan[MAX_WIDTH];  /* only used when zooming */
      GLint zoomY0;

      if (unpack->RowLength > 0)
         rowLength = unpack->RowLength;
      else
         rowLength = width;

      /* If we're not using pixel zoom then do all clipping calculations
       * now.  Otherwise, we'll let the gl_write_zoomed_*_span() functions
       * handle the clipping.
       */
      if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==1.0F) {
         /* horizontal clipping */
         if (destX < ctx->DrawBuffer->Xmin) {
            skipPixels += (ctx->DrawBuffer->Xmin - destX);
            drawWidth  -= (ctx->DrawBuffer->Xmin - destX);
            destX = ctx->DrawBuffer->Xmin;
         }
         if (destX + drawWidth > ctx->DrawBuffer->Xmax)
            drawWidth -= (destX + drawWidth - ctx->DrawBuffer->Xmax - 1);
         if (drawWidth <= 0)
            return GL_TRUE;

         /* vertical clipping */
         if (destY < ctx->DrawBuffer->Ymin) {
            skipRows   += (ctx->DrawBuffer->Ymin - destY);
            drawHeight -= (ctx->DrawBuffer->Ymin - destY);
            destY = ctx->DrawBuffer->Ymin;
         }
         if (destY + drawHeight > ctx->DrawBuffer->Ymax)
            drawHeight -= (destY + drawHeight - ctx->DrawBuffer->Ymax - 1);
         if (drawHeight <= 0)
            return GL_TRUE;

         zoomY0 = 0;  /* not used - silence compiler warning */
      }
      else if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==-1.0F) {
         /* upside-down image */
         /* horizontal clipping */
         if (destX < ctx->DrawBuffer->Xmin) {
            skipPixels += (ctx->DrawBuffer->Xmin - destX);
            drawWidth  -= (ctx->DrawBuffer->Xmin - destX);
            destX = ctx->DrawBuffer->Xmin;
         }
         if (destX + drawWidth > ctx->DrawBuffer->Xmax)
            drawWidth -= (destX + drawWidth - ctx->DrawBuffer->Xmax - 1);
         if (drawWidth <= 0)
            return GL_TRUE;

         /* vertical clipping */
         if (destY > ctx->DrawBuffer->Ymax) {
            skipRows   += (destY - ctx->DrawBuffer->Ymax - 1);
            drawHeight -= (destY - ctx->DrawBuffer->Ymax - 1);
            destY = ctx->DrawBuffer->Ymax + 1;
         }
         if (destY - drawHeight < ctx->DrawBuffer->Ymin)
            drawHeight -= (ctx->DrawBuffer->Ymin - (destY - drawHeight));
         if (drawHeight <= 0)
            return GL_TRUE;
      }
      else {
         /* setup array of fragment Z value to pass to zoom function */
         GLdepth z = (GLdepth) (ctx->Current.RasterPos[2] * ctx->Visual->DepthMaxF);
         GLint i;
         ASSERT(drawWidth < MAX_WIDTH);
         for (i=0; i<drawWidth; i++)
            zSpan[i] = z;

         /* save Y value of first row */
         zoomY0 = (GLint) (ctx->Current.RasterPos[1] + 0.5F);
      }


      /*
       * Ready to draw!
       * The window region at (destX, destY) of size (drawWidth, drawHeight)
       * will be written to.
       * We'll take pixel data from buffer pointed to by "pixels" but we'll
       * skip "skipRows" rows and skip "skipPixels" pixels/row.
       */

      if (format==GL_RGBA && type==GL_UNSIGNED_BYTE) {
         if (ctx->Visual->RGBAflag) {
            GLubyte *src = (GLubyte *) pixels
               + (skipRows * rowLength + skipPixels) * 4;
            if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==1.0F) {
               /* no zooming */
               GLint row;
               for (row=0; row<drawHeight; row++) {
                  (*ctx->Driver.WriteRGBASpan)(ctx, drawWidth, destX, destY,
                                               (void *) src, NULL);
                  src += rowLength * 4;
                  destY++;
               }
            }
            else if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==-1.0F) {
               /* upside-down */
               GLint row;
               for (row=0; row<drawHeight; row++) {
                  destY--;
                  (*ctx->Driver.WriteRGBASpan)(ctx, drawWidth, destX, destY,
                                              (void *) src, NULL);
                  src += rowLength * 4;
               }
            }
            else {
               /* with zooming */
               GLint row;
               for (row=0; row<drawHeight; row++) {
                  gl_write_zoomed_rgba_span(ctx, drawWidth, destX, destY,
                                            zSpan, (void *) src, zoomY0);
                  src += rowLength * 4;
                  destY++;
               }
            }
         }
         return GL_TRUE;
      }
      else if (format==GL_RGB && type==GL_UNSIGNED_BYTE) {
         if (ctx->Visual->RGBAflag) {
            GLubyte *src = (GLubyte *) pixels
               + (skipRows * rowLength + skipPixels) * 3;
            if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==1.0F) {
               GLint row;
               for (row=0; row<drawHeight; row++) {
                  (*ctx->Driver.WriteRGBSpan)(ctx, drawWidth, destX, destY,
                                              (void *) src, NULL);
                  src += rowLength * 3;
                  destY++;
               }
            }
            else if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==-1.0F) {
               /* upside-down */
               GLint row;
               for (row=0; row<drawHeight; row++) {
                  destY--;
                  (*ctx->Driver.WriteRGBSpan)(ctx, drawWidth, destX, destY,
                                              (void *) src, NULL);
                  src += rowLength * 3;
               }
            }
            else {
               /* with zooming */
               GLint row;
               for (row=0; row<drawHeight; row++) {
                  gl_write_zoomed_rgb_span(ctx, drawWidth, destX, destY,
                                           zSpan, (void *) src, zoomY0);
                  src += rowLength * 3;
                  destY++;
               }
            }
         }
         return GL_TRUE;
      }
      else if (format==GL_LUMINANCE && type==GL_UNSIGNED_BYTE) {
         if (ctx->Visual->RGBAflag) {
            GLubyte *src = (GLubyte *) pixels
               + (skipRows * rowLength + skipPixels);
            if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==1.0F) {
               /* no zooming */
               GLint row;
               ASSERT(drawWidth < MAX_WIDTH);
               for (row=0; row<drawHeight; row++) {
                  GLint i;
		  for (i=0;i<drawWidth;i++) {
                     rgb[i][0] = src[i];
                     rgb[i][1] = src[i];
                     rgb[i][2] = src[i];
		  }
                  (*ctx->Driver.WriteRGBSpan)(ctx, drawWidth, destX, destY,
                                              (void *) rgb, NULL);
                  src += rowLength;
                  destY++;
               }
            }
            else if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==-1.0F) {
               /* upside-down */
               GLint row;
               ASSERT(drawWidth < MAX_WIDTH);
               for (row=0; row<drawHeight; row++) {
                  GLint i;
                  for (i=0;i<drawWidth;i++) {
                     rgb[i][0] = src[i];
                     rgb[i][1] = src[i];
                     rgb[i][2] = src[i];
                  }
                  destY--;
                  (*ctx->Driver.WriteRGBSpan)(ctx, drawWidth, destX, destY,
                                              (void *) rgb, NULL);
                  src += rowLength;
               }
            }
            else {
               /* with zooming */
               GLint row;
               ASSERT(drawWidth < MAX_WIDTH);
               for (row=0; row<drawHeight; row++) {
                  GLint i;
		  for (i=0;i<drawWidth;i++) {
                     rgb[i][0] = src[i];
                     rgb[i][1] = src[i];
                     rgb[i][2] = src[i];
		  }
                  gl_write_zoomed_rgb_span(ctx, drawWidth, destX, destY,
                                           zSpan, (void *) rgb, zoomY0);
                  src += rowLength;
                  destY++;
               }
            }
         }
         return GL_TRUE;
      }
      else if (format==GL_LUMINANCE_ALPHA && type==GL_UNSIGNED_BYTE) {
         if (ctx->Visual->RGBAflag) {
            GLubyte *src = (GLubyte *) pixels
               + (skipRows * rowLength + skipPixels)*2;
            if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==1.0F) {
               /* no zooming */
               GLint row;
               ASSERT(drawWidth < MAX_WIDTH);
               for (row=0; row<drawHeight; row++) {
                  GLint i;
                  GLubyte *ptr = src;
		  for (i=0;i<drawWidth;i++) {
                     rgba[i][0] = *ptr;
                     rgba[i][1] = *ptr;
                     rgba[i][2] = *ptr++;
                     rgba[i][3] = *ptr++;
		  }
                  (*ctx->Driver.WriteRGBASpan)(ctx, drawWidth, destX, destY,
                                               (void *) rgba, NULL);
                  src += rowLength*2;
                  destY++;
               }
            }
            else if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==-1.0F) {
               /* upside-down */
               GLint row;
               ASSERT(drawWidth < MAX_WIDTH);
               for (row=0; row<drawHeight; row++) {
                  GLint i;
                  GLubyte *ptr = src;
                  for (i=0;i<drawWidth;i++) {
                     rgba[i][0] = *ptr;
                     rgba[i][1] = *ptr;
                     rgba[i][2] = *ptr++;
                     rgba[i][3] = *ptr++;
                  }
                  destY--;
                  (*ctx->Driver.WriteRGBASpan)(ctx, drawWidth, destX, destY,
                                               (void *) rgba, NULL);
                  src += rowLength*2;
               }
            }
            else {
               /* with zooming */
               GLint row;
               ASSERT(drawWidth < MAX_WIDTH);
               for (row=0; row<drawHeight; row++) {
                  GLubyte *ptr = src;
                  GLint i;
		  for (i=0;i<drawWidth;i++) {
                     rgba[i][0] = *ptr;
                     rgba[i][1] = *ptr;
                     rgba[i][2] = *ptr++;
                     rgba[i][3] = *ptr++;
		  }
                  gl_write_zoomed_rgba_span(ctx, drawWidth, destX, destY,
                                            zSpan, (void *) rgba, zoomY0);
                  src += rowLength*2;
                  destY++;
               }
            }
         }
         return GL_TRUE;
      }
      else if (format==GL_COLOR_INDEX && type==GL_UNSIGNED_BYTE) {
         GLubyte *src = (GLubyte *) pixels + skipRows * rowLength + skipPixels;
         if (ctx->Visual->RGBAflag) {
            /* convert CI data to RGBA */
            if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==1.0F) {
               /* no zooming */
               GLint row;
               for (row=0; row<drawHeight; row++) {
                  ASSERT(drawWidth < MAX_WIDTH);
                  _mesa_map_ci8_to_rgba(ctx, drawWidth, src, rgba);
                  (*ctx->Driver.WriteRGBASpan)(ctx, drawWidth, destX, destY,
                                               (const GLubyte (*)[4])rgba, 
					       NULL);
                  src += rowLength;
                  destY++;
               }
               return GL_TRUE;
            }
            else if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==-1.0F) {
               /* upside-down */
               GLint row;
               for (row=0; row<drawHeight; row++) {
                  ASSERT(drawWidth < MAX_WIDTH);
                  _mesa_map_ci8_to_rgba(ctx, drawWidth, src, rgba);
                  destY--;
                  (*ctx->Driver.WriteRGBASpan)(ctx, drawWidth, destX, destY,
                                               (const GLubyte (*)[4])rgba, 
                                               NULL);
                  src += rowLength;
               }
               return GL_TRUE;
            }
            else {
               /* with zooming */
               GLint row;
               for (row=0; row<drawHeight; row++) {
                  ASSERT(drawWidth < MAX_WIDTH);
                  _mesa_map_ci8_to_rgba(ctx, drawWidth, src, rgba);
                  gl_write_zoomed_rgba_span(ctx, drawWidth, destX, destY,
                                            zSpan, (void *) rgba, zoomY0);
                  src += rowLength;
                  destY++;
               }
               return GL_TRUE;
            }
         }
         else {
            /* write CI data to CI frame buffer */
            GLint row;
            if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==1.0F) {
               /* no zooming */
               for (row=0; row<drawHeight; row++) {
                  (*ctx->Driver.WriteCI8Span)(ctx, drawWidth, destX, destY,
                                              src, NULL);
                  src += rowLength;
                  destY++;
               }
               return GL_TRUE;
            }
            else {
               /* with zooming */
               return GL_FALSE;
            }
         }
      }
      else {
         /* can't handle this pixel format and/or data type here */
         return GL_FALSE;
      }
   }

   /* can't do a simple draw, have to use slow path */
   return GL_FALSE;
}



/*
 * Do glDrawPixels of index pixels.
 */
static void
draw_index_pixels( GLcontext *ctx, GLint x, GLint y,
                   GLsizei width, GLsizei height,
                   GLenum type, const GLvoid *pixels )
{
   const GLboolean zoom = ctx->Pixel.ZoomX!=1.0 || ctx->Pixel.ZoomY!=1.0;
   const GLint desty = y;
   GLint row, drawWidth;
   GLdepth zspan[MAX_WIDTH];

   drawWidth = (width > MAX_WIDTH) ? MAX_WIDTH : width;

   /* Fragment depth values */
   if (ctx->Depth.Test || ctx->Fog.Enabled) {
      GLdepth zval = (GLdepth) (ctx->Current.RasterPos[2] * ctx->Visual->DepthMaxF);
      GLint i;
      for (i = 0; i < drawWidth; i++) {
	 zspan[i] = zval;
      }
   }

   /*
    * General solution
    */
   for (row = 0; row < height; row++, y++) {
      GLuint indexes[MAX_WIDTH];
      const GLvoid *source = _mesa_image_address(&ctx->Unpack,
                    pixels, width, height, GL_COLOR_INDEX, type, 0, row, 0);
      _mesa_unpack_index_span(ctx, drawWidth, GL_UNSIGNED_INT, indexes,
                              type, source, &ctx->Unpack,
                              ctx->ImageTransferState);
      if (zoom) {
         gl_write_zoomed_index_span(ctx, drawWidth, x, y, zspan, indexes, desty);
      }
      else {
         gl_write_index_span(ctx, drawWidth, x, y, zspan, indexes, GL_BITMAP);
      }
   }
}



/*
 * Do glDrawPixels of stencil image.  The image datatype may either
 * be GLubyte or GLbitmap.
 */
static void 
draw_stencil_pixels( GLcontext *ctx, GLint x, GLint y,
                     GLsizei width, GLsizei height,
                     GLenum type, const GLvoid *pixels )
{
   const GLboolean zoom = ctx->Pixel.ZoomX!=1.0 || ctx->Pixel.ZoomY!=1.0;
   const GLint desty = y;
   GLint row, drawWidth;

   if (type != GL_BYTE &&
       type != GL_UNSIGNED_BYTE &&
       type != GL_SHORT &&
       type != GL_UNSIGNED_SHORT &&
       type != GL_INT &&
       type != GL_UNSIGNED_INT &&
       type != GL_FLOAT &&
       type != GL_BITMAP) {
      gl_error( ctx, GL_INVALID_ENUM, "glDrawPixels(stencil type)");
      return;
   }

   drawWidth = (width > MAX_WIDTH) ? MAX_WIDTH : width;

   for (row = 0; row < height; row++, y++) {
      GLstencil values[MAX_WIDTH];
      GLenum destType = (sizeof(GLstencil) == sizeof(GLubyte))
                      ? GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT;
      const GLvoid *source = _mesa_image_address(&ctx->Unpack,
                    pixels, width, height, GL_COLOR_INDEX, type, 0, row, 0);
      _mesa_unpack_index_span(ctx, drawWidth, destType, values,
                              type, source, &ctx->Unpack,
                              ctx->ImageTransferState);
      if (ctx->ImageTransferState & IMAGE_SHIFT_OFFSET_BIT) {
         _mesa_shift_and_offset_stencil( ctx, drawWidth, values );
      }
      if (ctx->Pixel.MapStencilFlag) {
         _mesa_map_stencil( ctx, drawWidth, values );
      }

      if (zoom) {
         gl_write_zoomed_stencil_span( ctx, (GLuint) drawWidth, x, y,
                                       values, desty );
      }
      else {
         _mesa_write_stencil_span( ctx, (GLuint) drawWidth, x, y, values );
      }
   }
}



/*
 * Do a glDrawPixels of depth values.
 */
static void
draw_depth_pixels( GLcontext *ctx, GLint x, GLint y,
                   GLsizei width, GLsizei height,
                   GLenum type, const GLvoid *pixels )
{
   const GLboolean bias_or_scale = ctx->Pixel.DepthBias!=0.0 || ctx->Pixel.DepthScale!=1.0;
   const GLboolean zoom = ctx->Pixel.ZoomX!=1.0 || ctx->Pixel.ZoomY!=1.0;
   const GLint desty = y;
   GLubyte rgba[MAX_WIDTH][4];
   GLuint ispan[MAX_WIDTH];
   GLint drawWidth = (width > MAX_WIDTH) ? MAX_WIDTH : width;

   if (type != GL_BYTE
       && type != GL_UNSIGNED_BYTE
       && type != GL_SHORT
       && type != GL_UNSIGNED_SHORT
       && type != GL_INT
       && type != GL_UNSIGNED_INT
       && type != GL_FLOAT) {
      gl_error(ctx, GL_INVALID_ENUM, "glDrawPixels(type)");
      return;
   }

   /* Colors or indexes */
   if (ctx->Visual->RGBAflag) {
      GLint r = (GLint) (ctx->Current.RasterColor[0] * 255.0F);
      GLint g = (GLint) (ctx->Current.RasterColor[1] * 255.0F);
      GLint b = (GLint) (ctx->Current.RasterColor[2] * 255.0F);
      GLint a = (GLint) (ctx->Current.RasterColor[3] * 255.0F);
      GLint i;
      for (i = 0; i < drawWidth; i++) {
         rgba[i][RCOMP] = r;
         rgba[i][GCOMP] = g;
         rgba[i][BCOMP] = b;
         rgba[i][ACOMP] = a;
      }
   }
   else {
      GLint i;
      for (i = 0; i < drawWidth; i++) {
	 ispan[i] = ctx->Current.RasterIndex;
      }
   }

   if (type==GL_UNSIGNED_SHORT && sizeof(GLdepth)==sizeof(GLushort)
       && !bias_or_scale && !zoom && ctx->Visual->RGBAflag) {
      /* Special case: directly write 16-bit depth values */
      GLint row;
      for (row = 0; row < height; row++, y++) {
         GLdepth zspan[MAX_WIDTH];
         const GLushort *zptr = _mesa_image_address(&ctx->Unpack,
                pixels, width, height, GL_DEPTH_COMPONENT, type, 0, row, 0);
         GLint i;
         for (i = 0; i < width; i++)
            zspan[i] = zptr[i];
         gl_write_rgba_span( ctx, width, x, y, zspan, rgba, GL_BITMAP );
      }
   }
   else if (type==GL_UNSIGNED_INT && ctx->Visual->DepthBits == 32
       && !bias_or_scale && !zoom && ctx->Visual->RGBAflag) {
      /* Special case: directly write 32-bit depth values */
      GLint row;
      for (row = 0; row < height; row++, y++) {
         const GLuint *zptr = _mesa_image_address(&ctx->Unpack,
                pixels, width, height, GL_DEPTH_COMPONENT, type, 0, row, 0);
         gl_write_rgba_span( ctx, width, x, y, zptr, rgba, GL_BITMAP );
      }
   }
   else {
      /* General case */
      GLint row;
      for (row = 0; row < height; row++, y++) {
         GLdepth zspan[MAX_WIDTH];
         const GLvoid *src = _mesa_image_address(&ctx->Unpack,
                pixels, width, height, GL_DEPTH_COMPONENT, type, 0, row, 0);
         _mesa_unpack_depth_span( ctx, drawWidth, zspan, type, src,
                                  &ctx->Unpack, ctx->ImageTransferState );
         if (ctx->Visual->RGBAflag) {
            if (zoom) {
               gl_write_zoomed_rgba_span(ctx, width, x, y, zspan,
                                         (const GLubyte (*)[4])rgba, desty);
            }
            else {
               gl_write_rgba_span(ctx, width, x, y, zspan, rgba, GL_BITMAP);
            }
         }
         else {
            if (zoom) {
               gl_write_zoomed_index_span(ctx, width, x, y, zspan,
                                          ispan, GL_BITMAP);
            }
            else {
               gl_write_index_span(ctx, width, x, y, zspan, ispan, GL_BITMAP);
            }
         }

      }
   }
}


/*
 * Do glDrawPixels of RGBA pixels.
 */
static void
draw_rgba_pixels( GLcontext *ctx, GLint x, GLint y,
                  GLsizei width, GLsizei height,
                  GLenum format, GLenum type, const GLvoid *pixels )
{
   const struct gl_pixelstore_attrib *unpack = &ctx->Unpack;
   const GLboolean zoom = ctx->Pixel.ZoomX!=1.0 || ctx->Pixel.ZoomY!=1.0;
   const GLint desty = y;
   GLdepth zspan[MAX_WIDTH];
   GLboolean quickDraw;
   GLfloat *convImage = NULL;
   GLuint transferOps = ctx->ImageTransferState;

   /* Try an optimized glDrawPixels first */
   if (fast_draw_pixels(ctx, x, y, width, height, format, type, pixels))
      return;

   /* Fragment depth values */
   if (ctx->Depth.Test || ctx->Fog.Enabled) {
      /* fill in array of z values */
      GLdepth z = (GLdepth) (ctx->Current.RasterPos[2] * ctx->Visual->DepthMaxF);
      GLint i;
      for (i=0;i<width;i++) {
	 zspan[i] = z;
      }
   }


   if (ctx->RasterMask == 0 && !zoom && x >= 0 && y >= 0
       && x + width <= ctx->DrawBuffer->Width
       && y + height <= ctx->DrawBuffer->Height) {
      quickDraw = GL_TRUE;
   }
   else {
      quickDraw = GL_FALSE;
   }

   if (ctx->Pixel.Convolution2DEnabled || ctx->Pixel.Separable2DEnabled) {
      /* Convolution has to be handled specially.  We'll create an
       * intermediate image, applying all pixel transfer operations
       * up to convolution.  Then we'll convolve the image.  Then
       * we'll proceed with the rest of the transfer operations and
       * rasterize the image.
       */
      const GLuint preConvTransferOps =
         IMAGE_SCALE_BIAS_BIT |
         IMAGE_SHIFT_OFFSET_BIT |
         IMAGE_MAP_COLOR_BIT |
         IMAGE_COLOR_TABLE_BIT;
      const GLuint postConvTransferOps =
         IMAGE_POST_CONVOLUTION_COLOR_TABLE_BIT |
         IMAGE_COLOR_MATRIX_BIT |
         IMAGE_POST_COLOR_MATRIX_COLOR_TABLE_BIT |
         IMAGE_HISTOGRAM_BIT |
         IMAGE_MIN_MAX_BIT;
      GLint row;
      GLfloat *dest, *tmpImage;

      tmpImage = (GLfloat *) MALLOC(width * height * 4 * sizeof(GLfloat));
      if (!tmpImage) {
         gl_error(ctx, GL_OUT_OF_MEMORY, "glDrawPixels");
         return;
      }
      convImage = (GLfloat *) MALLOC(width * height * 4 * sizeof(GLfloat));
      if (!convImage) {
         FREE(tmpImage);
         gl_error(ctx, GL_OUT_OF_MEMORY, "glDrawPixels");
         return;
      }

      /* Unpack the image and apply transfer ops up to convolution */
      dest = tmpImage;
      for (row = 0; row < height; row++) {
         const GLvoid *source = _mesa_image_address(unpack,
                  pixels, width, height, format, type, 0, row, 0);
         _mesa_unpack_float_color_span(ctx, width, GL_RGBA, (void *) dest,
                                       format, type, source, unpack,
                                       transferOps & preConvTransferOps,
                                       GL_FALSE);
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
      FREE(tmpImage);

      /* continue transfer ops and draw the convolved image */
      unpack = &_mesa_native_packing;
      pixels = convImage;
      format = GL_RGBA;
      type = GL_FLOAT;
      transferOps &= postConvTransferOps;
   }

   /*
    * General solution
    */
   {
      GLubyte rgba[MAX_WIDTH][4];
      GLint row;
      if (width > MAX_WIDTH)
         width = MAX_WIDTH;
      for (row = 0; row < height; row++, y++) {
         const GLvoid *source = _mesa_image_address(unpack,
                  pixels, width, height, format, type, 0, row, 0);
         _mesa_unpack_ubyte_color_span(ctx, width, GL_RGBA, (void*) rgba,
                                       format, type, source, unpack,
                                       transferOps);
         if ((ctx->Pixel.MinMaxEnabled && ctx->MinMax.Sink) ||
             (ctx->Pixel.HistogramEnabled && ctx->Histogram.Sink))
            continue;

         if (ctx->Texture.ReallyEnabled && ctx->Pixel.PixelTextureEnabled) {
            GLfloat s[MAX_WIDTH], t[MAX_WIDTH], r[MAX_WIDTH], q[MAX_WIDTH];
            GLubyte primary_rgba[MAX_WIDTH][4];
            GLuint unit;
            /* XXX not sure how multitexture is supposed to work here */

            MEMCPY(primary_rgba, rgba, 4 * width * sizeof(GLubyte));

            for (unit = 0; unit < MAX_TEXTURE_UNITS; unit++) {
               _mesa_pixeltexgen(ctx, width, (const GLubyte (*)[4]) rgba,
                                 s, t, r, q);
               gl_texture_pixels(ctx, unit, width, s, t, r, NULL,
                                 primary_rgba, rgba);
            }
         }

         if (quickDraw) {
            (*ctx->Driver.WriteRGBASpan)( ctx, width, x, y,
                                          (CONST GLubyte (*)[]) rgba, NULL);
         }
         else if (zoom) {
            gl_write_zoomed_rgba_span( ctx, width, x, y, zspan, 
				       (CONST GLubyte (*)[]) rgba, desty );
         }
         else {
            gl_write_rgba_span( ctx, (GLuint) width, x, y, zspan, rgba, GL_BITMAP);
         }
      }
   }

   if (convImage) {
      FREE(convImage);
   }
}



/*
 * Execute glDrawPixels
 */
void
_mesa_DrawPixels( GLsizei width, GLsizei height,
                  GLenum format, GLenum type, const GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glDrawPixels");

   if (ctx->RenderMode==GL_RENDER) {
      GLint x, y;
      if (!pixels || !ctx->Current.RasterPosValid) {
	 return;
      }

      if (ctx->NewState) {
         gl_update_state(ctx);
      }

      if (ctx->ImageTransferState == UPDATE_IMAGE_TRANSFER_STATE)
         _mesa_update_image_transfer_state(ctx);

      x = (GLint) (ctx->Current.RasterPos[0] + 0.5F);
      y = (GLint) (ctx->Current.RasterPos[1] + 0.5F);

      ctx->OcclusionResult = GL_TRUE;

      /* see if device driver can do the drawpix */
      if (ctx->Driver.DrawPixels
          && (*ctx->Driver.DrawPixels)(ctx, x, y, width, height, format, type,
                                       &ctx->Unpack, pixels)) {
         return;
      }

      switch (format) {
	 case GL_STENCIL_INDEX:
	    draw_stencil_pixels( ctx, x, y, width, height, type, pixels );
	    break;
	 case GL_DEPTH_COMPONENT:
	    draw_depth_pixels( ctx, x, y, width, height, type, pixels );
	    break;
	 case GL_COLOR_INDEX:
            if (ctx->Visual->RGBAflag)
               draw_rgba_pixels(ctx, x,y, width, height, format, type, pixels);
            else
               draw_index_pixels(ctx, x, y, width, height, type, pixels);
	    break;
	 case GL_RED:
	 case GL_GREEN:
	 case GL_BLUE:
	 case GL_ALPHA:
	 case GL_LUMINANCE:
	 case GL_LUMINANCE_ALPHA:
	 case GL_RGB:
         case GL_BGR:
	 case GL_RGBA:
	 case GL_BGRA:
	 case GL_ABGR_EXT:
            draw_rgba_pixels(ctx, x, y, width, height, format, type, pixels);
	    break;
	 default:
	    gl_error( ctx, GL_INVALID_ENUM, "glDrawPixels(format)" );
            return;
      }
   }
   else if (ctx->RenderMode==GL_FEEDBACK) {
      if (ctx->Current.RasterPosValid) {
         GLfloat color[4];
	 GLfloat texcoord[4], invq;
	 UBYTE_RGBA_TO_FLOAT_RGBA(color, ctx->Current.ByteColor);
         invq = 1.0F / ctx->Current.Texcoord[0][3];
         texcoord[0] = ctx->Current.Texcoord[0][0] * invq;
         texcoord[1] = ctx->Current.Texcoord[0][1] * invq;
         texcoord[2] = ctx->Current.Texcoord[0][2] * invq;
         texcoord[3] = ctx->Current.Texcoord[0][3];
         FEEDBACK_TOKEN( ctx, (GLfloat) (GLint) GL_DRAW_PIXEL_TOKEN );
         gl_feedback_vertex( ctx,
                             ctx->Current.RasterPos,
                             color, ctx->Current.Index, texcoord );
      }
   }
   else if (ctx->RenderMode==GL_SELECT) {
      if (ctx->Current.RasterPosValid) {
         gl_update_hitflag( ctx, ctx->Current.RasterPos[2] );
      }
   }
}


/* $Id: s_drawpix.c,v 1.21 2001/06/18 23:55:18 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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
#include "colormac.h"
#include "context.h"
#include "convolve.h"
#include "image.h"
#include "macros.h"
#include "mem.h"
#include "mmath.h"
#include "pixel.h"

#include "s_context.h"
#include "s_drawpix.h"
#include "s_fog.h"
#include "s_pixeltex.h"
#include "s_span.h"
#include "s_stencil.h"
#include "s_texture.h"
#include "s_zoom.h"



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
   if (*destX < buffer->_Xmin) {
      *skipPixels += (buffer->_Xmin - *destX);
      *width -= (buffer->_Xmin - *destX);
      *destX = buffer->_Xmin;
   }
   /* right clipping */
   if (*destX + *width > buffer->_Xmax)
      *width -= (*destX + *width - buffer->_Xmax);

   if (*width <= 0)
      return GL_FALSE;

   /* bottom clipping */
   if (*destY < buffer->_Ymin) {
      *skipRows += (buffer->_Ymin - *destY);
      *height -= (buffer->_Ymin - *destY);
      *destY = buffer->_Ymin;
   }
   /* top clipping */
   if (*destY + *height > buffer->_Ymax)
      *height -= (*destY + *height - buffer->_Ymax);

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
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const struct gl_pixelstore_attrib *unpack = &ctx->Unpack;
   GLchan rgb[MAX_WIDTH][3];
   GLchan rgba[MAX_WIDTH][4];

   if (!ctx->Current.RasterPosValid) {
      return GL_TRUE;      /* no-op */
   }

   if ((SWRAST_CONTEXT(ctx)->_RasterMask&(~(SCISSOR_BIT|WINCLIP_BIT)))==0
       && ctx->Texture._ReallyEnabled == 0
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
      GLint zoomY0 = 0;

      if (unpack->RowLength > 0)
         rowLength = unpack->RowLength;
      else
         rowLength = width;

      /* If we're not using pixel zoom then do all clipping calculations
       * now.  Otherwise, we'll let the _mesa_write_zoomed_*_span() functions
       * handle the clipping.
       */
      if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==1.0F) {
         /* horizontal clipping */
         if (destX < ctx->DrawBuffer->_Xmin) {
            skipPixels += (ctx->DrawBuffer->_Xmin - destX);
            drawWidth  -= (ctx->DrawBuffer->_Xmin - destX);
            destX = ctx->DrawBuffer->_Xmin;
         }
         if (destX + drawWidth > ctx->DrawBuffer->_Xmax)
            drawWidth -= (destX + drawWidth - ctx->DrawBuffer->_Xmax);
         if (drawWidth <= 0)
            return GL_TRUE;

         /* vertical clipping */
         if (destY < ctx->DrawBuffer->_Ymin) {
            skipRows   += (ctx->DrawBuffer->_Ymin - destY);
            drawHeight -= (ctx->DrawBuffer->_Ymin - destY);
            destY = ctx->DrawBuffer->_Ymin;
         }
         if (destY + drawHeight > ctx->DrawBuffer->_Ymax)
            drawHeight -= (destY + drawHeight - ctx->DrawBuffer->_Ymax);
         if (drawHeight <= 0)
            return GL_TRUE;
      }
      else if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==-1.0F) {
         /* upside-down image */
         /* horizontal clipping */
         if (destX < ctx->DrawBuffer->_Xmin) {
            skipPixels += (ctx->DrawBuffer->_Xmin - destX);
            drawWidth  -= (ctx->DrawBuffer->_Xmin - destX);
            destX = ctx->DrawBuffer->_Xmin;
         }
         if (destX + drawWidth > ctx->DrawBuffer->_Xmax)
            drawWidth -= (destX + drawWidth - ctx->DrawBuffer->_Xmax);
         if (drawWidth <= 0)
            return GL_TRUE;

         /* vertical clipping */
         if (destY > ctx->DrawBuffer->_Ymax) {
            skipRows   += (destY - ctx->DrawBuffer->_Ymax);
            drawHeight -= (destY - ctx->DrawBuffer->_Ymax);
            destY = ctx->DrawBuffer->_Ymax;
         }
         if (destY - drawHeight < ctx->DrawBuffer->_Ymin)
            drawHeight -= (ctx->DrawBuffer->_Ymin - (destY - drawHeight));
         if (drawHeight <= 0)
            return GL_TRUE;
      }
      else {
         /* setup array of fragment Z value to pass to zoom function */
         GLdepth z = (GLdepth) (ctx->Current.RasterPos[2] * ctx->DepthMaxF);
         GLint i;
         ASSERT(drawWidth < MAX_WIDTH);
         for (i=0; i<drawWidth; i++)
            zSpan[i] = z;

         /* save Y value of first row */
         zoomY0 = IROUND(ctx->Current.RasterPos[1]);
      }


      /*
       * Ready to draw!
       * The window region at (destX, destY) of size (drawWidth, drawHeight)
       * will be written to.
       * We'll take pixel data from buffer pointed to by "pixels" but we'll
       * skip "skipRows" rows and skip "skipPixels" pixels/row.
       */

      if (format == GL_RGBA && type == CHAN_TYPE
          && ctx->_ImageTransferState==0) {
         if (ctx->Visual.rgbMode) {
            GLchan *src = (GLchan *) pixels
               + (skipRows * rowLength + skipPixels) * 4;
            if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==1.0F) {
               /* no zooming */
               GLint row;
               for (row=0; row<drawHeight; row++) {
                  (*swrast->Driver.WriteRGBASpan)(ctx, drawWidth, destX, destY,
                                              (CONST GLchan (*)[4]) src, NULL);
                  src += rowLength * 4;
                  destY++;
               }
            }
            else if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==-1.0F) {
               /* upside-down */
               GLint row;
               for (row=0; row<drawHeight; row++) {
                  destY--;
                  (*swrast->Driver.WriteRGBASpan)(ctx, drawWidth, destX, destY,
                                              (CONST GLchan (*)[4]) src, NULL);
                  src += rowLength * 4;
               }
            }
            else {
               /* with zooming */
               GLint row;
               for (row=0; row<drawHeight; row++) {
                  _mesa_write_zoomed_rgba_span(ctx, drawWidth, destX, destY,
                                  zSpan, 0, (CONST GLchan (*)[4]) src, zoomY0);
                  src += rowLength * 4;
                  destY++;
               }
            }
         }
         return GL_TRUE;
      }
      else if (format == GL_RGB && type == CHAN_TYPE
               && ctx->_ImageTransferState == 0) {
         if (ctx->Visual.rgbMode) {
            GLchan *src = (GLchan *) pixels
               + (skipRows * rowLength + skipPixels) * 3;
            if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==1.0F) {
               GLint row;
               for (row=0; row<drawHeight; row++) {
                  (*swrast->Driver.WriteRGBSpan)(ctx, drawWidth, destX, destY,
                                              (CONST GLchan (*)[3]) src, NULL);
                  src += rowLength * 3;
                  destY++;
               }
            }
            else if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==-1.0F) {
               /* upside-down */
               GLint row;
               for (row=0; row<drawHeight; row++) {
                  destY--;
                  (*swrast->Driver.WriteRGBSpan)(ctx, drawWidth, destX, destY,
                                              (CONST GLchan (*)[3]) src, NULL);
                  src += rowLength * 3;
               }
            }
            else {
               /* with zooming */
               GLint row;
               for (row=0; row<drawHeight; row++) {
                  _mesa_write_zoomed_rgb_span(ctx, drawWidth, destX, destY,
                                  zSpan, 0, (CONST GLchan (*)[3]) src, zoomY0);
                  src += rowLength * 3;
                  destY++;
               }
            }
         }
         return GL_TRUE;
      }
      else if (format == GL_LUMINANCE && type == CHAN_TYPE
               && ctx->_ImageTransferState==0) {
         if (ctx->Visual.rgbMode) {
            GLchan *src = (GLchan *) pixels
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
                  (*swrast->Driver.WriteRGBSpan)(ctx, drawWidth, destX, destY,
                                              (CONST GLchan (*)[3]) rgb, NULL);
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
                  (*swrast->Driver.WriteRGBSpan)(ctx, drawWidth, destX, destY,
                                              (CONST GLchan (*)[3]) rgb, NULL);
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
                  _mesa_write_zoomed_rgb_span(ctx, drawWidth, destX, destY,
                                  zSpan, 0, (CONST GLchan (*)[3]) rgb, zoomY0);
                  src += rowLength;
                  destY++;
               }
            }
         }
         return GL_TRUE;
      }
      else if (format == GL_LUMINANCE_ALPHA && type == CHAN_TYPE
               && ctx->_ImageTransferState == 0) {
         if (ctx->Visual.rgbMode) {
            GLchan *src = (GLchan *) pixels
               + (skipRows * rowLength + skipPixels)*2;
            if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==1.0F) {
               /* no zooming */
               GLint row;
               ASSERT(drawWidth < MAX_WIDTH);
               for (row=0; row<drawHeight; row++) {
                  GLint i;
                  GLchan *ptr = src;
		  for (i=0;i<drawWidth;i++) {
                     rgba[i][0] = *ptr;
                     rgba[i][1] = *ptr;
                     rgba[i][2] = *ptr++;
                     rgba[i][3] = *ptr++;
		  }
                  (*swrast->Driver.WriteRGBASpan)(ctx, drawWidth, destX, destY,
                                             (CONST GLchan (*)[4]) rgba, NULL);
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
                  GLchan *ptr = src;
                  for (i=0;i<drawWidth;i++) {
                     rgba[i][0] = *ptr;
                     rgba[i][1] = *ptr;
                     rgba[i][2] = *ptr++;
                     rgba[i][3] = *ptr++;
                  }
                  destY--;
                  (*swrast->Driver.WriteRGBASpan)(ctx, drawWidth, destX, destY,
                                             (CONST GLchan (*)[4]) rgba, NULL);
                  src += rowLength*2;
               }
            }
            else {
               /* with zooming */
               GLint row;
               ASSERT(drawWidth < MAX_WIDTH);
               for (row=0; row<drawHeight; row++) {
                  GLchan *ptr = src;
                  GLint i;
		  for (i=0;i<drawWidth;i++) {
                     rgba[i][0] = *ptr;
                     rgba[i][1] = *ptr;
                     rgba[i][2] = *ptr++;
                     rgba[i][3] = *ptr++;
		  }
                  _mesa_write_zoomed_rgba_span(ctx, drawWidth, destX, destY,
                                 zSpan, 0, (CONST GLchan (*)[4]) rgba, zoomY0);
                  src += rowLength*2;
                  destY++;
               }
            }
         }
         return GL_TRUE;
      }
      else if (format==GL_COLOR_INDEX && type==GL_UNSIGNED_BYTE) {
         GLubyte *src = (GLubyte *) pixels + skipRows * rowLength + skipPixels;
         if (ctx->Visual.rgbMode) {
            /* convert CI data to RGBA */
            if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==1.0F) {
               /* no zooming */
               GLint row;
               for (row=0; row<drawHeight; row++) {
                  ASSERT(drawWidth < MAX_WIDTH);
                  _mesa_map_ci8_to_rgba(ctx, drawWidth, src, rgba);
                  (*swrast->Driver.WriteRGBASpan)(ctx, drawWidth, destX, destY,
                                               (const GLchan (*)[4]) rgba,
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
                  (*swrast->Driver.WriteRGBASpan)(ctx, drawWidth, destX, destY,
                                               (CONST GLchan (*)[4]) rgba,
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
                  _mesa_write_zoomed_rgba_span(ctx, drawWidth, destX, destY,
                                 zSpan, 0, (CONST GLchan (*)[4]) rgba, zoomY0);
                  src += rowLength;
                  destY++;
               }
               return GL_TRUE;
            }
         }
         else if (ctx->_ImageTransferState==0) {
            /* write CI data to CI frame buffer */
            GLint row;
            if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==1.0F) {
               /* no zooming */
               for (row=0; row<drawHeight; row++) {
                  (*swrast->Driver.WriteCI8Span)(ctx, drawWidth, destX, destY,
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
   GLfloat fogSpan[MAX_WIDTH];

   drawWidth = (width > MAX_WIDTH) ? MAX_WIDTH : width;

   /* Fragment depth values */
   if (ctx->Depth.Test || ctx->Fog.Enabled) {
      GLdepth zval = (GLdepth) (ctx->Current.RasterPos[2] * ctx->DepthMaxF);
      GLfloat fog;
      GLint i;

      if (ctx->Fog.FogCoordinateSource == GL_FOG_COORDINATE_EXT) {
         fog = ctx->Current.RasterFogCoord;
      }
      else {
         fog = _mesa_z_to_fogfactor(ctx, ctx->Current.RasterDistance);
      }

      for (i = 0; i < drawWidth; i++) {
	 zspan[i] = zval;
         fogSpan[i] = fog;
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
                              ctx->_ImageTransferState);
      if (zoom) {
         _mesa_write_zoomed_index_span(ctx, drawWidth, x, y, zspan, fogSpan,
                                       indexes, desty);
      }
      else {
         _mesa_write_index_span(ctx, drawWidth, x, y, zspan, fogSpan, indexes,
                                NULL, GL_BITMAP);
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
      _mesa_error( ctx, GL_INVALID_ENUM, "glDrawPixels(stencil type)");
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
                              ctx->_ImageTransferState);
      if (ctx->_ImageTransferState & IMAGE_SHIFT_OFFSET_BIT) {
         _mesa_shift_and_offset_stencil( ctx, drawWidth, values );
      }
      if (ctx->Pixel.MapStencilFlag) {
         _mesa_map_stencil( ctx, drawWidth, values );
      }

      if (zoom) {
         _mesa_write_zoomed_stencil_span( ctx, (GLuint) drawWidth, x, y,
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
   GLchan rgba[MAX_WIDTH][4];
   GLuint ispan[MAX_WIDTH];
   GLint drawWidth = (width > MAX_WIDTH) ? MAX_WIDTH : width;

   if (type != GL_BYTE
       && type != GL_UNSIGNED_BYTE
       && type != GL_SHORT
       && type != GL_UNSIGNED_SHORT
       && type != GL_INT
       && type != GL_UNSIGNED_INT
       && type != GL_FLOAT) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glDrawPixels(type)");
      return;
   }

   /* Colors or indexes */
   if (ctx->Visual.rgbMode) {
      GLint i;
      GLint r, g, b, a;
      UNCLAMPED_FLOAT_TO_CHAN(r, ctx->Current.RasterColor[0]);
      UNCLAMPED_FLOAT_TO_CHAN(g, ctx->Current.RasterColor[1]);
      UNCLAMPED_FLOAT_TO_CHAN(b, ctx->Current.RasterColor[2]);
      UNCLAMPED_FLOAT_TO_CHAN(a, ctx->Current.RasterColor[3]);
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
       && !bias_or_scale && !zoom && ctx->Visual.rgbMode) {
      /* Special case: directly write 16-bit depth values */
      GLint row;
      for (row = 0; row < height; row++, y++) {
         GLdepth zspan[MAX_WIDTH];
         const GLushort *zptr = (const GLushort *)
            _mesa_image_address(&ctx->Unpack, pixels, width, height,
                                GL_DEPTH_COMPONENT, type, 0, row, 0);
         GLint i;
         for (i = 0; i < width; i++)
            zspan[i] = zptr[i];
         _mesa_write_rgba_span(ctx, width, x, y, zspan, 0, rgba,
                               NULL, GL_BITMAP);
      }
   }
   else if (type==GL_UNSIGNED_INT && ctx->Visual.depthBits == 32
       && !bias_or_scale && !zoom && ctx->Visual.rgbMode) {
      /* Special case: directly write 32-bit depth values */
      GLint row;
      for (row = 0; row < height; row++, y++) {
         const GLuint *zptr = (const GLuint *)
            _mesa_image_address(&ctx->Unpack, pixels, width, height,
                                GL_DEPTH_COMPONENT, type, 0, row, 0);
         _mesa_write_rgba_span(ctx, width, x, y, zptr, 0, rgba,
                               NULL, GL_BITMAP);
      }
   }
   else {
      /* General case */
      GLint row;
      for (row = 0; row < height; row++, y++) {
         GLfloat fspan[MAX_WIDTH];
         GLdepth zspan[MAX_WIDTH];
         const GLvoid *src = _mesa_image_address(&ctx->Unpack,
                pixels, width, height, GL_DEPTH_COMPONENT, type, 0, row, 0);
         _mesa_unpack_depth_span( ctx, drawWidth, fspan, type, src,
                                  &ctx->Unpack );
         /* clamp depth values to [0,1] and convert from floats to integers */
         {
            const GLfloat zs = ctx->DepthMaxF;
            GLint i;
            for (i = 0; i < drawWidth; i++) {
               zspan[i] = (GLdepth) (fspan[i] * zs);
            }
         }

         if (ctx->Visual.rgbMode) {
            if (zoom) {
               _mesa_write_zoomed_rgba_span(ctx, width, x, y, zspan, 0,
                                            (const GLchan (*)[4]) rgba, desty);
            }
            else {
               _mesa_write_rgba_span(ctx, width, x, y, zspan, 0,
                                     rgba, NULL, GL_BITMAP);
            }
         }
         else {
            if (zoom) {
               _mesa_write_zoomed_index_span(ctx, width, x, y, zspan, 0,
                                             ispan, GL_BITMAP);
            }
            else {
               _mesa_write_index_span(ctx, width, x, y, zspan, 0,
                                      ispan, NULL, GL_BITMAP);
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
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const struct gl_pixelstore_attrib *unpack = &ctx->Unpack;
   const GLboolean zoom = ctx->Pixel.ZoomX!=1.0 || ctx->Pixel.ZoomY!=1.0;
   const GLint desty = y;
   GLdepth zspan[MAX_WIDTH];
   GLfloat fogSpan[MAX_WIDTH];
   GLboolean quickDraw;
   GLfloat *convImage = NULL;
   GLuint transferOps = ctx->_ImageTransferState;

   if (!_mesa_is_legal_format_and_type(format, type)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glDrawPixels(format or type)");
      return;
   }

   /* Try an optimized glDrawPixels first */
   if (fast_draw_pixels(ctx, x, y, width, height, format, type, pixels))
      return;

   /* Fragment depth values */
   if (ctx->Depth.Test || ctx->Fog.Enabled) {
      /* fill in array of z values */
      GLdepth z = (GLdepth) (ctx->Current.RasterPos[2] * ctx->DepthMaxF);
      GLfloat fog;
      GLint i;

      if (ctx->Fog.FogCoordinateSource == GL_FOG_COORDINATE_EXT) {
         fog = ctx->Current.RasterFogCoord;
      }
      else {
         fog = _mesa_z_to_fogfactor(ctx, ctx->Current.RasterDistance);
      }

      for (i=0;i<width;i++) {
	 zspan[i] = z;
         fogSpan[i] = fog;
      }
   }


   if (SWRAST_CONTEXT(ctx)->_RasterMask == 0 && !zoom && x >= 0 && y >= 0
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
      GLint row;
      GLfloat *dest, *tmpImage;

      tmpImage = (GLfloat *) MALLOC(width * height * 4 * sizeof(GLfloat));
      if (!tmpImage) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glDrawPixels");
         return;
      }
      convImage = (GLfloat *) MALLOC(width * height * 4 * sizeof(GLfloat));
      if (!convImage) {
         FREE(tmpImage);
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glDrawPixels");
         return;
      }

      /* Unpack the image and apply transfer ops up to convolution */
      dest = tmpImage;
      for (row = 0; row < height; row++) {
         const GLvoid *source = _mesa_image_address(unpack,
                  pixels, width, height, format, type, 0, row, 0);
         _mesa_unpack_float_color_span(ctx, width, GL_RGBA, (GLfloat *) dest,
                                      format, type, source, unpack,
                                      transferOps & IMAGE_PRE_CONVOLUTION_BITS,
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
      transferOps &= IMAGE_POST_CONVOLUTION_BITS;
   }

   /*
    * General solution
    */
   {
      GLchan rgba[MAX_WIDTH][4];
      GLint row;
      if (width > MAX_WIDTH)
         width = MAX_WIDTH;
      for (row = 0; row < height; row++, y++) {
         const GLvoid *source = _mesa_image_address(unpack,
                  pixels, width, height, format, type, 0, row, 0);
         _mesa_unpack_chan_color_span(ctx, width, GL_RGBA, (GLchan *) rgba,
                                      format, type, source, unpack,
                                      transferOps);
         if ((ctx->Pixel.MinMaxEnabled && ctx->MinMax.Sink) ||
             (ctx->Pixel.HistogramEnabled && ctx->Histogram.Sink))
            continue;

         if (ctx->Texture._ReallyEnabled && ctx->Pixel.PixelTextureEnabled) {
            GLchan primary_rgba[MAX_WIDTH][4];
            GLuint unit;
            DEFARRAY(GLfloat, s, MAX_WIDTH);  /* mac 32k limitation */
            DEFARRAY(GLfloat, t, MAX_WIDTH);
            DEFARRAY(GLfloat, r, MAX_WIDTH);
            DEFARRAY(GLfloat, q, MAX_WIDTH);
            CHECKARRAY(s, return);  /* mac 32k limitation */
            CHECKARRAY(t, return);
            CHECKARRAY(r, return);
            CHECKARRAY(q, return);

            /* XXX not sure how multitexture is supposed to work here */
            MEMCPY(primary_rgba, rgba, 4 * width * sizeof(GLchan));

            for (unit = 0; unit < ctx->Const.MaxTextureUnits; unit++) {
               if (ctx->Texture.Unit[unit]._ReallyEnabled) {
                  _mesa_pixeltexgen(ctx, width, (const GLchan (*)[4]) rgba,
                                    s, t, r, q);
                  _swrast_texture_fragments(ctx, unit, width, s, t, r, NULL,
                                            (CONST GLchan (*)[4]) primary_rgba,
                                            rgba);
               }
            }
            UNDEFARRAY(s);  /* mac 32k limitation */
            UNDEFARRAY(t);
            UNDEFARRAY(r);
            UNDEFARRAY(q);
         }

         if (quickDraw) {
            (*swrast->Driver.WriteRGBASpan)(ctx, width, x, y,
                                            (CONST GLchan (*)[4]) rgba, NULL);
         }
         else if (zoom) {
            _mesa_write_zoomed_rgba_span(ctx, width, x, y, zspan, fogSpan,
                                         (CONST GLchan (*)[4]) rgba, desty);
         }
         else {
            _mesa_write_rgba_span(ctx, (GLuint) width, x, y, zspan, fogSpan,
                                  rgba, NULL, GL_BITMAP);
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
_swrast_DrawPixels( GLcontext *ctx,
		    GLint x, GLint y,
		    GLsizei width, GLsizei height,
		    GLenum format, GLenum type,
		    const struct gl_pixelstore_attrib *unpack,
		    const GLvoid *pixels )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   (void) unpack;


   if (swrast->NewState)
      _swrast_validate_derived( ctx );

   RENDER_START(swrast,ctx);

   switch (format) {
   case GL_STENCIL_INDEX:
      draw_stencil_pixels( ctx, x, y, width, height, type, pixels );
      break;
   case GL_DEPTH_COMPONENT:
      draw_depth_pixels( ctx, x, y, width, height, type, pixels );
      break;
   case GL_COLOR_INDEX:
      if (ctx->Visual.rgbMode)
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
      _mesa_error( ctx, GL_INVALID_ENUM, "glDrawPixels(format)" );
   }

   RENDER_FINISH(swrast,ctx);
}

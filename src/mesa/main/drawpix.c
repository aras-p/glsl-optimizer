/* $Id: drawpix.c,v 1.4 1999/10/19 20:33:57 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.1
 * 
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
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
#ifndef XFree86Server
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#else
#include "GL/xf86glx.h"
#endif
#include "context.h"
#include "drawpix.h"
#include "feedback.h"
#include "image.h"
#include "macros.h"
#include "mmath.h"
#include "pixel.h"
#include "span.h"
#include "stencil.h"
#include "types.h"
#include "zoom.h"
#ifdef XFree86Server
#include "GL/xf86glx.h"
#endif
#endif



/* TODO:  apply texture mapping to fragments */


/*
 * Try to do a fast glDrawPixels.  Conditions include:
 *   not using a display list
 *   simple pixel unpacking
 *   no raster ops
 *   etc....
 * Return:  GL_TRUE if success, GL_FALSE if slow path must be used instead
 */
GLboolean gl_direct_DrawPixels( GLcontext *ctx,
                                const struct gl_pixelstore_attrib *unpack,
                                GLsizei width, GLsizei height,
                                GLenum format, GLenum type,
                                const GLvoid *pixels )
{
   GLubyte rgb[MAX_WIDTH][3];
   GLubyte rgba[MAX_WIDTH][4];

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH_WITH_RETVAL(ctx, "glDrawPixels", 
						  GL_FALSE);


   if (!ctx->Current.RasterPosValid) {
      /* no-op */
      return GL_TRUE;
   }

   if (ctx->NewState) {
      gl_update_state(ctx);
   }

   /* see if device driver can do the drawpix */
   if (ctx->Driver.DrawPixels) {
      GLint x = (GLint) (ctx->Current.RasterPos[0] + 0.5F);
      GLint y = (GLint) (ctx->Current.RasterPos[1] + 0.5F);
      if ((*ctx->Driver.DrawPixels)(ctx, x, y, width, height, format, type,
                                    unpack, pixels))
         return GL_TRUE;
   }

   if ((ctx->RasterMask&(~(SCISSOR_BIT|WINCLIP_BIT)))==0
       && ctx->Pixel.RedBias==0.0 && ctx->Pixel.RedScale==1.0
       && ctx->Pixel.GreenBias==0.0 && ctx->Pixel.GreenScale==1.0
       && ctx->Pixel.BlueBias==0.0 && ctx->Pixel.BlueScale==1.0
       && ctx->Pixel.AlphaBias==0.0 && ctx->Pixel.AlphaScale==1.0
       && ctx->Pixel.IndexShift==0 && ctx->Pixel.IndexOffset==0
       && ctx->Pixel.MapColorFlag==0
       && unpack->Alignment==1
       && !unpack->SwapBytes
       && !unpack->LsbFirst) {

      GLint destX = (GLint) (ctx->Current.RasterPos[0] + 0.5F);
      GLint destY = (GLint) (ctx->Current.RasterPos[1] + 0.5F);
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
         if (destX < ctx->Buffer->Xmin) {
            skipPixels += (ctx->Buffer->Xmin - destX);
            drawWidth  -= (ctx->Buffer->Xmin - destX);
            destX = ctx->Buffer->Xmin;
         }
         if (destX + drawWidth > ctx->Buffer->Xmax)
            drawWidth -= (destX + drawWidth - ctx->Buffer->Xmax - 1);
         if (drawWidth <= 0)
            return GL_TRUE;

         /* vertical clipping */
         if (destY < ctx->Buffer->Ymin) {
            skipRows   += (ctx->Buffer->Ymin - destY);
            drawHeight -= (ctx->Buffer->Ymin - destY);
            destY = ctx->Buffer->Ymin;
         }
         if (destY + drawHeight > ctx->Buffer->Ymax)
            drawHeight -= (destY + drawHeight - ctx->Buffer->Ymax - 1);
         if (drawHeight <= 0)
            return GL_TRUE;
      }
      else {
         /* setup array of fragment Z value to pass to zoom function */
         GLdepth z = (GLdepth) (ctx->Current.RasterPos[2] * DEPTH_SCALE);
         GLint i;
         assert(drawWidth < MAX_WIDTH);
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
               assert(drawWidth < MAX_WIDTH);
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
            else {
               /* with zooming */
               GLint row;
               assert(drawWidth < MAX_WIDTH);
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
               assert(drawWidth < MAX_WIDTH);
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
            else {
               /* with zooming */
               GLint row;
               assert(drawWidth < MAX_WIDTH);
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
                  assert(drawWidth < MAX_WIDTH);
                  gl_map_ci8_to_rgba(ctx, drawWidth, src, rgba);
                  (*ctx->Driver.WriteRGBASpan)(ctx, drawWidth, destX, destY,
                                               (const GLubyte (*)[4])rgba, 
					       NULL);
                  src += rowLength;
                  destY++;
               }
               return GL_TRUE;
            }
            else {
               /* with zooming */
               GLint row;
               for (row=0; row<drawHeight; row++) {
                  assert(drawWidth < MAX_WIDTH);
                  gl_map_ci8_to_rgba(ctx, drawWidth, src, rgba);
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
   else {
      /* can't do direct render, have to use slow path */
      return GL_FALSE;
   }
}



/*
 * Do glDrawPixels of index pixels.
 */
static void draw_index_pixels( GLcontext *ctx, GLint x, GLint y,
                               const struct gl_image *image )
{
   GLint width, height, widthInBytes;
   const GLint desty = y;
   GLint i, j;
   GLdepth zspan[MAX_WIDTH];
   const GLboolean zoom = ctx->Pixel.ZoomX!=1.0 || ctx->Pixel.ZoomY!=1.0;

   assert(image);
   assert(image->Format == GL_COLOR_INDEX);

   width = image->Width;
   height = image->Height;
   if (image->Type == GL_BITMAP)
      widthInBytes = (width + 7) / 8;
   else
      widthInBytes = width;

   /* Fragment depth values */
   if (ctx->Depth.Test) {
      GLdepth zval = (GLdepth) (ctx->Current.RasterPos[2] * DEPTH_SCALE);
      for (i=0;i<width;i++) {
	 zspan[i] = zval;
      }
   }

   /* process the image row by row */
   for (i=0;i<height;i++,y++) {
      GLuint ispan[MAX_WIDTH];

      /* convert to uints */
      switch (image->Type) {
	 case GL_UNSIGNED_BYTE:
	    {
	       GLubyte *src = (GLubyte *) image->Data + i * width;
	       for (j=0;j<width;j++) {
		  ispan[j] = (GLuint) *src++;
	       }
	    }
	    break;
	 case GL_FLOAT:
	    {
	       GLfloat *src = (GLfloat *) image->Data + i * width;
	       for (j=0;j<width;j++) {
		  ispan[j] = (GLuint) (GLint) *src++;
	       }
	    }
	    break;
         case GL_BITMAP:
            {
	       GLubyte *src = (GLubyte *) image->Data + i * widthInBytes;
	       for (j=0;j<width;j++) {
		  ispan[j] = ( src[j >> 3] >> (7 - (j & 0x7)) ) & 1;
	       }
            }
            break;
	 default:
	    gl_problem( ctx, "draw_index_pixels type" );
            return;
      }

      /* apply shift and offset */
      if (ctx->Pixel.IndexOffset || ctx->Pixel.IndexShift) {
         gl_shift_and_offset_ci( ctx, width, ispan );
      }

      if (ctx->Visual->RGBAflag) {
	 /* Convert index to RGBA and write to frame buffer */
	 GLubyte rgba[MAX_WIDTH][4];
         gl_map_ci_to_rgba( ctx, width, ispan, rgba );
         if (zoom) {
            gl_write_zoomed_rgba_span( ctx, width, x, y, zspan, 
				       (const GLubyte (*)[4])rgba, desty );
         }
         else {
            gl_write_rgba_span( ctx, width, x, y, zspan, rgba, GL_BITMAP );
         }
      }
      else {
	 /* optionally apply index map then write to frame buffer */
	 if (ctx->Pixel.MapColorFlag) {
            gl_map_ci(ctx, width, ispan);
	 }
         if (zoom) {
            gl_write_zoomed_index_span( ctx, width, x, y, zspan, ispan, desty );
         }
         else {
            gl_write_index_span( ctx, width, x, y, zspan, ispan, GL_BITMAP );
         }
      }
   }

}



/*
 * Do glDrawPixels of stencil image.  The image datatype may either
 * be GLubyte or GLbitmap.
 */
static void draw_stencil_pixels( GLcontext *ctx, GLint x, GLint y,
                                 const struct gl_image *image )
{
   GLint widthInBytes, width, height;
   const GLint desty = y;
   GLint i;
   const GLboolean zoom = ctx->Pixel.ZoomX!=1.0 || ctx->Pixel.ZoomY!=1.0;

   if (image->Type != GL_BYTE &&
       image->Type != GL_UNSIGNED_BYTE &&
       image->Type != GL_SHORT &&
       image->Type != GL_UNSIGNED_SHORT &&
       image->Type != GL_INT &&
       image->Type != GL_UNSIGNED_INT &&
       image->Type != GL_FLOAT &&
       image->Type != GL_BITMAP) {
      gl_error( ctx, GL_INVALID_OPERATION, "glDrawPixels(stencil type)");
      return;
   }

   assert(image);
   assert(image->Format == GL_STENCIL_INDEX);
   assert(image->Type == GL_UNSIGNED_BYTE || image->Type == GL_BITMAP);

   if (image->Type == GL_UNSIGNED_BYTE)
      widthInBytes = image->Width;
   else
      widthInBytes = (image->Width + 7) / 8;
   width = image->Width;
   height = image->Height;

   /* process the image row by row */
   for (i=0;i<height;i++,y++) {
      GLstencil *src = (GLstencil*)image->Data + i * widthInBytes;
      GLstencil *stencilValues;
      GLstencil stencilCopy[MAX_WIDTH];

      if (image->Type == GL_BITMAP) {
         /* convert bitmap data to GLubyte (0 or 1) data */
         GLint j;
         for (j = 0; j < width; j++) {
            stencilCopy[j] = ( src[j >> 3] >> (7 - (j & 0x7)) ) & 1;
         }
         src = stencilCopy;
      }

      if (ctx->Pixel.IndexOffset || ctx->Pixel.IndexShift
          || ctx->Pixel.MapStencilFlag) {

         /* make copy of stencil values */
         if (src != stencilCopy)
            MEMCPY( stencilCopy, src, width * sizeof(GLstencil));

         /* apply shift and offset */
         if (ctx->Pixel.IndexOffset || ctx->Pixel.IndexShift) {
            gl_shift_and_offset_stencil( ctx, width, stencilCopy );
         }

         /* mapping */
         if (ctx->Pixel.MapStencilFlag) {
            gl_map_stencil( ctx, width, stencilCopy );
         }

         stencilValues = stencilCopy;
      }
      else {
         /* use stencil values in-place */
         stencilValues = src;
      }

      /* write stencil values to stencil buffer */
      if (zoom) {
         gl_write_zoomed_stencil_span( ctx, (GLuint) width, x, y,
                                       stencilValues, desty );
      }
      else {
         gl_write_stencil_span( ctx, (GLuint) width, x, y, stencilValues );
      }
   }
}



/*
 * Do a glDrawPixels of depth values.
 */
static void draw_depth_pixels( GLcontext *ctx, GLint x, GLint y,
                               const struct gl_image *image )
{
   GLint width, height;
   const GLint desty = y;
   GLubyte rgba[MAX_WIDTH][4];
   GLuint ispan[MAX_WIDTH];
   const GLboolean bias_or_scale = ctx->Pixel.DepthBias!=0.0 || ctx->Pixel.DepthScale!=1.0;
   const GLboolean zoom = ctx->Pixel.ZoomX!=1.0 || ctx->Pixel.ZoomY!=1.0;

   assert(image);
   assert(image->Format == GL_DEPTH_COMPONENT);

   width = image->Width;
   height = image->Height;

   /* Color or index */
   if (ctx->Visual->RGBAflag) {
      GLint r = (GLint) (ctx->Current.RasterColor[0] * 255.0F);
      GLint g = (GLint) (ctx->Current.RasterColor[1] * 255.0F);
      GLint b = (GLint) (ctx->Current.RasterColor[2] * 255.0F);
      GLint a = (GLint) (ctx->Current.RasterColor[3] * 255.0F);
      GLint i;
      for (i=0; i<width; i++) {
         rgba[i][RCOMP] = r;
         rgba[i][GCOMP] = g;
         rgba[i][BCOMP] = b;
         rgba[i][ACOMP] = a;
      }
   }
   else {
      GLint i;
      for (i=0;i<width;i++) {
	 ispan[i] = ctx->Current.RasterIndex;
      }
   }

   if (image->Type==GL_UNSIGNED_SHORT && sizeof(GLdepth)==sizeof(GLushort)
       && !bias_or_scale && !zoom && ctx->Visual->RGBAflag) {
      /* Special case: directly write 16-bit depth values */
      GLint j;
      for (j=0;j<height;j++,y++) {
         GLdepth *zptr = (GLdepth *) image->Data + j * width;
         gl_write_rgba_span( ctx, width, x, y, zptr, rgba, GL_BITMAP );
      }
   }
   else if (image->Type==GL_UNSIGNED_INT && sizeof(GLdepth)==sizeof(GLuint)
       && !bias_or_scale && !zoom && ctx->Visual->RGBAflag) {
      /* Special case: directly write 32-bit depth values */
      GLint i, j;
      /* Compute shift value to scale 32-bit uints down to depth values. */
      GLuint shift = 0;
      GLuint max = MAX_DEPTH;
      while ((max&0x80000000)==0) {
         max = max << 1;
         shift++;
      }
      for (j=0;j<height;j++,y++) {
         GLdepth zspan[MAX_WIDTH];
         GLuint *zptr = (GLuint *) image->Data + j * width;
         for (i=0;i<width;i++) {
            zspan[i] = zptr[i] >> shift;
         }
         gl_write_rgba_span( ctx, width, x, y, zspan, rgba, GL_BITMAP );
      }
   }
   else {
      /* General case (slower) */
      GLint i, j;

      /* process image row by row */
      for (i=0;i<height;i++,y++) {
         GLfloat depth[MAX_WIDTH];
         GLdepth zspan[MAX_WIDTH];

         switch (image->Type) {
            case GL_UNSIGNED_SHORT:
               {
                  GLushort *src = (GLushort *) image->Data + i * width;
                  for (j=0;j<width;j++) {
                     depth[j] = USHORT_TO_FLOAT( *src++ );
                  }
               }
               break;
            case GL_UNSIGNED_INT:
               {
                  GLuint *src = (GLuint *) image->Data + i * width;
                  for (j=0;j<width;j++) {
                     depth[j] = UINT_TO_FLOAT( *src++ );
                  }
               }
               break;
            case GL_FLOAT:
               {
                  GLfloat *src = (GLfloat *) image->Data + i * width;
                  for (j=0;j<width;j++) {
                     depth[j] = *src++;
                  }
               }
               break;
            default:
               gl_problem(ctx, "Bad type in draw_depth_pixels");
               return;
         }

         /* apply depth scale and bias */
         if (ctx->Pixel.DepthScale!=1.0 || ctx->Pixel.DepthBias!=0.0) {
            for (j=0;j<width;j++) {
               depth[j] = depth[j] * ctx->Pixel.DepthScale + ctx->Pixel.DepthBias;
            }
         }

         /* clamp depth values to [0,1] and convert from floats to integers */
         for (j=0;j<width;j++) {
            zspan[j] = (GLdepth) (CLAMP( depth[j], 0.0F, 1.0F ) * DEPTH_SCALE);
         }

         if (ctx->Visual->RGBAflag) {
            if (zoom) {
               gl_write_zoomed_rgba_span( ctx, width, x, y, zspan,
                                          (const GLubyte (*)[4])rgba, desty );
            }
            else {
               gl_write_rgba_span( ctx, width, x, y, zspan, rgba, GL_BITMAP );
            }
         }
         else {
            if (zoom) {
               gl_write_zoomed_index_span( ctx, width, x, y, zspan,
                                           ispan, GL_BITMAP );
            }
            else {
               gl_write_index_span( ctx, width, x, y, zspan, ispan, GL_BITMAP );
            }
         }

      }
   }
}



/* Simple unpacking parameters: */
static struct gl_pixelstore_attrib NoUnpack = {
   1,            /* Alignment */
   0,            /* RowLength */
   0,            /* SkipPixels */
   0,            /* SkipRows */
   0,            /* ImageHeight */
   0,            /* SkipImages */
   GL_FALSE,     /* SwapBytes */
   GL_FALSE      /* LsbFirst */
};


/*
 * Do glDrawPixels of RGBA pixels.
 */
static void draw_rgba_pixels( GLcontext *ctx, GLint x, GLint y,
                              const struct gl_image *image )
{
   GLint width, height;
   GLint i, j;
   const GLint desty = y;
   GLdepth zspan[MAX_WIDTH];
   GLboolean quickDraw;
   const GLboolean zoom = ctx->Pixel.ZoomX!=1.0 || ctx->Pixel.ZoomY!=1.0;

   assert(image);

   /* Try an optimized glDrawPixels first */
   if (gl_direct_DrawPixels(ctx, &NoUnpack, image->Width, image->Height,
                            image->Format, image->Type, image->Data ))
      return;

   width = image->Width;
   height = image->Height;

   /* Fragment depth values */
   if (ctx->Depth.Test) {
      /* fill in array of z values */
      GLdepth z = (GLdepth) (ctx->Current.RasterPos[2] * DEPTH_SCALE);
      for (i=0;i<width;i++) {
	 zspan[i] = z;
      }
   }

   if (ctx->RasterMask==0 && !zoom && x>=0 && y>=0
       && x+width<=ctx->Buffer->Width && y+height<=ctx->Buffer->Height) {
      quickDraw = GL_TRUE;
   }
   else {
      quickDraw = GL_FALSE;
   }

   {
      /* General solution */
      GLboolean r_flag, g_flag, b_flag, a_flag, l_flag;
      GLuint components;
      GLubyte rgba[MAX_WIDTH][4];
      GLfloat  rf[MAX_WIDTH];
      GLfloat  gf[MAX_WIDTH];
      GLfloat  bf[MAX_WIDTH];
      DEFARRAY(GLfloat,af,MAX_WIDTH);
      CHECKARRAY(af,return);

      r_flag = g_flag = b_flag = a_flag = l_flag = GL_FALSE;
      switch (image->Format) {
	 case GL_RED:
	    r_flag = GL_TRUE;
	    components = 1;
	    break;
	 case GL_GREEN:
	    g_flag = GL_TRUE;
	    components = 1;
	    break;
	 case GL_BLUE:
	    b_flag = GL_TRUE;
	    components = 1;
	    break;
	 case GL_ALPHA:
	    a_flag = GL_TRUE;
	    components = 1;
	    break;
	 case GL_RGB:
	    r_flag = g_flag = b_flag = GL_TRUE;
	    components = 3;
	    break;
	 case GL_LUMINANCE:
	    l_flag = GL_TRUE;
	    components = 1;
	    break;
	 case GL_LUMINANCE_ALPHA:
	    l_flag = a_flag = GL_TRUE;
	    components = 2;
	    break;
	 case GL_RGBA:
	    r_flag = g_flag = b_flag = a_flag = GL_TRUE;
	    components = 4;
	    break;
         default:
            gl_problem(ctx, "Bad type in draw_rgba_pixels");
            goto cleanup;
      }

      /* process the image row by row */
      for (i=0;i<height;i++,y++) {
	 /* convert to floats */
	 switch (image->Type) {
	    case GL_UNSIGNED_BYTE:
	       {
		  GLubyte *src = (GLubyte *) image->Data + i * width * components;
		  for (j=0;j<width;j++) {
		     if (l_flag) {
			rf[j] = gf[j] = bf[j] = UBYTE_TO_FLOAT(*src++);
		     }
		     else {
			rf[j] = r_flag ? UBYTE_TO_FLOAT(*src++) : 0.0;
			gf[j] = g_flag ? UBYTE_TO_FLOAT(*src++) : 0.0;
			bf[j] = b_flag ? UBYTE_TO_FLOAT(*src++) : 0.0;
		     }
		     af[j] = a_flag ? UBYTE_TO_FLOAT(*src++) : 1.0;
		  }
	       }
	       break;
	    case GL_FLOAT:
	       {
		  GLfloat *src = (GLfloat *) image->Data + i * width * components;
		  for (j=0;j<width;j++) {
		     if (l_flag) {
			rf[j] = gf[j] = bf[j] = *src++;
		     }
		     else {
			rf[j] = r_flag ? *src++ : 0.0;
			gf[j] = g_flag ? *src++ : 0.0;
			bf[j] = b_flag ? *src++ : 0.0;
		     }
		     af[j] = a_flag ? *src++ : 1.0;
		  }
	       }
	       break;
	    default:
	       gl_problem( ctx, "draw_rgba_pixels type" );
               goto cleanup;
	 }

	 /* apply scale and bias */
	 if (ctx->Pixel.ScaleOrBiasRGBA) {
            gl_scale_and_bias_color(ctx, width, rf, gf, bf, af);
	 }

	 /* apply pixel mappings */
	 if (ctx->Pixel.MapColorFlag) {
            gl_map_color(ctx, width, rf, gf, bf, af);
	 }

	 /* convert to integers */
	 for (j=0;j<width;j++) {
	    rgba[j][RCOMP] = (GLint) (rf[j] * 255.0F);
	    rgba[j][GCOMP] = (GLint) (gf[j] * 255.0F);
	    rgba[j][BCOMP] = (GLint) (bf[j] * 255.0F);
	    rgba[j][ACOMP] = (GLint) (af[j] * 255.0F);
	 }

	 /* write to frame buffer */
         if (quickDraw) {
            (*ctx->Driver.WriteRGBASpan)( ctx, width, x, y, 
					  (const GLubyte (*)[4])rgba, NULL);
         }
         else if (zoom) {
            gl_write_zoomed_rgba_span( ctx, width, x, y, zspan, 
				       (const GLubyte (*)[4])rgba, desty );
         }
         else {
            gl_write_rgba_span( ctx, (GLuint) width, x, y, zspan, rgba, GL_BITMAP);
         }
      }
cleanup:
      UNDEFARRAY(af);
   }
}



/*
 * Execute glDrawPixels
 */
void gl_DrawPixels( GLcontext* ctx, struct gl_image *image )
{
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glDrawPixels");


   if (gl_image_error_test( ctx, image, "glDrawPixels" ))
      return;

   if (ctx->RenderMode==GL_RENDER) {
      GLint x, y;
      if (!ctx->Current.RasterPosValid) {
	 return;
      }

      x = (GLint) (ctx->Current.RasterPos[0] + 0.5F);
      y = (GLint) (ctx->Current.RasterPos[1] + 0.5F);

      switch (image->Format) {
	 case GL_COLOR_INDEX:
            draw_index_pixels( ctx, x, y, image );
	    break;
	 case GL_STENCIL_INDEX:
	    draw_stencil_pixels( ctx, x, y, image );
	    break;
	 case GL_DEPTH_COMPONENT:
	    draw_depth_pixels( ctx, x, y, image );
	    break;
	 case GL_RED:
	 case GL_GREEN:
	 case GL_BLUE:
	 case GL_ALPHA:
	 case GL_RGB:
	 case GL_LUMINANCE:
	 case GL_LUMINANCE_ALPHA:
	 case GL_RGBA:
            draw_rgba_pixels( ctx, x, y, image );
	    break;
	 default:
	    gl_error( ctx, GL_INVALID_ENUM, "glDrawPixels" );
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


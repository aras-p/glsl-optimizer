/* $Id: s_copypix.c,v 1.1 2000/10/31 18:00:04 keithw Exp $ */

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


#include "glheader.h"
#include "colormac.h"
#include "context.h"
#include "convolve.h"
#include "feedback.h"
#include "macros.h"
#include "mem.h"
#include "mmath.h"
#include "pixel.h"

#include "s_depth.h"
#include "s_imaging.h"
#include "s_pixeltex.h"
#include "s_span.h"
#include "s_stencil.h"
#include "s_texture.h"
#include "s_zoom.h"



/*
 * Determine if there's overlap in an image copy
 */
static GLboolean
regions_overlap(int srcx, int srcy, int dstx, int dsty, int width, int height,
                float zoomX, float zoomY)
{
   if ((srcx > dstx + (width * zoomX) + 1) || (srcx + width + 1 < dstx)) {
      return GL_FALSE;
   }
   else if ((srcy < dsty) && (srcy + height < dsty + (height * zoomY))) {
      return GL_FALSE;
   }
   else if ((srcy > dsty) && (srcy + height > dsty + (height * zoomY))) {
      return GL_FALSE;
   }
   else {
      return GL_TRUE;
   }
}



/*
 * RGBA copypixels with convolution.
 */
static void
copy_conv_rgba_pixels(GLcontext *ctx, GLint srcx, GLint srcy,
                      GLint width, GLint height, GLint destx, GLint desty)
{
   GLdepth zspan[MAX_WIDTH];
   GLboolean quick_draw;
   GLint row;
   GLboolean changeBuffer;
   GLchan *saveReadAlpha;
   const GLboolean zoom = ctx->Pixel.ZoomX != 1.0F || ctx->Pixel.ZoomY != 1.0F;
   const GLuint transferOps = ctx->ImageTransferState;
   GLfloat *dest, *tmpImage, *convImage;

   if (ctx->Depth.Test || ctx->Fog.Enabled) {
      /* fill in array of z values */
      GLdepth z = (GLdepth)
         (ctx->Current.RasterPos[2] * ctx->Visual.DepthMax);
      GLint i;
      for (i = 0; i < width; i++) {
         zspan[i] = z;
      }
   }

   if (ctx->RasterMask == 0
       && !zoom
       && destx >= 0
       && destx + width <= ctx->DrawBuffer->Width) {
      quick_draw = GL_TRUE;
   }
   else {
      quick_draw = GL_FALSE;
   }

   /* If read and draw buffer are different we must do buffer switching */
   saveReadAlpha = ctx->ReadBuffer->Alpha;
   changeBuffer = ctx->Pixel.ReadBuffer != ctx->Color.DrawBuffer
               || ctx->DrawBuffer != ctx->ReadBuffer;


   /* allocate space for GLfloat image */
   tmpImage = (GLfloat *) MALLOC(width * height * 4 * sizeof(GLfloat));
   if (!tmpImage) {
      gl_error(ctx, GL_OUT_OF_MEMORY, "glCopyPixels");
      return;
   }
   convImage = (GLfloat *) MALLOC(width * height * 4 * sizeof(GLfloat));
   if (!convImage) {
      FREE(tmpImage);
      gl_error(ctx, GL_OUT_OF_MEMORY, "glCopyPixels");
      return;
   }

   dest = tmpImage;

   if (changeBuffer) {
      (*ctx->Driver.SetReadBuffer)( ctx, ctx->ReadBuffer,
                                    ctx->Pixel.DriverReadBuffer );
      if (ctx->Pixel.DriverReadBuffer == GL_FRONT_LEFT)
         ctx->ReadBuffer->Alpha = ctx->ReadBuffer->FrontLeftAlpha;
      else if (ctx->Pixel.DriverReadBuffer == GL_BACK_LEFT)
         ctx->ReadBuffer->Alpha = ctx->ReadBuffer->BackLeftAlpha;
      else if (ctx->Pixel.DriverReadBuffer == GL_FRONT_RIGHT)
         ctx->ReadBuffer->Alpha = ctx->ReadBuffer->FrontRightAlpha;
      else
         ctx->ReadBuffer->Alpha = ctx->ReadBuffer->BackRightAlpha;
   }

   /* read source image */
   dest = tmpImage;
   for (row = 0; row < height; row++) {
      GLchan rgba[MAX_WIDTH][4];
      GLint i;
      gl_read_rgba_span(ctx, ctx->ReadBuffer, width, srcx, srcy + row, rgba);
      /* convert GLchan to GLfloat */
      for (i = 0; i < width; i++) {
         *dest++ = (GLfloat) rgba[i][RCOMP] * (1.0F / CHAN_MAXF);
         *dest++ = (GLfloat) rgba[i][GCOMP] * (1.0F / CHAN_MAXF);
         *dest++ = (GLfloat) rgba[i][BCOMP] * (1.0F / CHAN_MAXF);
         *dest++ = (GLfloat) rgba[i][ACOMP] * (1.0F / CHAN_MAXF);
      }
   }

   /* read from the draw buffer again (in case of blending) */
   if (changeBuffer) {
      (*ctx->Driver.SetReadBuffer)( ctx, ctx->DrawBuffer,
                                    ctx->Color.DriverDrawBuffer );
      ctx->ReadBuffer->Alpha = saveReadAlpha;
   }

   /* do image transfer ops up until convolution */
   for (row = 0; row < height; row++) {
      GLfloat (*rgba)[4] = (GLfloat (*)[4]) tmpImage + row * width * 4;

      /* scale & bias */
      if (transferOps & IMAGE_SCALE_BIAS_BIT) {
         _mesa_scale_and_bias_rgba(ctx, width, rgba);
      }
      /* color map lookup */
      if (transferOps & IMAGE_MAP_COLOR_BIT) {
         _mesa_map_rgba(ctx, width, rgba);
      }
      /* GL_COLOR_TABLE lookup */
      if (transferOps & IMAGE_COLOR_TABLE_BIT) {
         _mesa_lookup_rgba(&ctx->ColorTable, width, rgba);
      }
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

   /* do remaining image transfer ops */
   for (row = 0; row < height; row++) {
      GLfloat (*rgba)[4] = (GLfloat (*)[4]) convImage + row * width * 4;

      /* GL_POST_CONVOLUTION_COLOR_TABLE lookup */
      if (transferOps & IMAGE_POST_CONVOLUTION_COLOR_TABLE_BIT) {
         _mesa_lookup_rgba(&ctx->PostConvolutionColorTable, width, rgba);
      }
      /* color matrix */
      if (transferOps & IMAGE_COLOR_MATRIX_BIT) {
         _mesa_transform_rgba(ctx, width, rgba);
      }
      /* GL_POST_COLOR_MATRIX_COLOR_TABLE lookup */
      if (transferOps & IMAGE_POST_COLOR_MATRIX_COLOR_TABLE_BIT) {
         _mesa_lookup_rgba(&ctx->PostColorMatrixColorTable, width, rgba);
      }
      /* update histogram count */
      if (transferOps & IMAGE_HISTOGRAM_BIT) {
         _mesa_update_histogram(ctx, width, (CONST GLfloat (*)[4]) rgba);
      }
      /* update min/max */
      if (transferOps & IMAGE_MIN_MAX_BIT) {
         _mesa_update_minmax(ctx, width, (CONST GLfloat (*)[4]) rgba);
      }
   }

   for (row = 0; row < height; row++) {
      const GLfloat *src = convImage + row * width * 4;
      GLchan rgba[MAX_WIDTH][4];
      GLint i, dy;

      /* clamp to [0,1] and convert float back to chan */
      for (i = 0; i < width; i++) {
         GLint r = (GLint) (src[i * 4 + RCOMP] * CHAN_MAXF);
         GLint g = (GLint) (src[i * 4 + GCOMP] * CHAN_MAXF);
         GLint b = (GLint) (src[i * 4 + BCOMP] * CHAN_MAXF);
         GLint a = (GLint) (src[i * 4 + ACOMP] * CHAN_MAXF);
         rgba[i][RCOMP] = (GLchan) CLAMP(r, 0, CHAN_MAX);
         rgba[i][GCOMP] = (GLchan) CLAMP(g, 0, CHAN_MAX);
         rgba[i][BCOMP] = (GLchan) CLAMP(b, 0, CHAN_MAX);
         rgba[i][ACOMP] = (GLchan) CLAMP(a, 0, CHAN_MAX);
      }

      if (ctx->Texture.ReallyEnabled && ctx->Pixel.PixelTextureEnabled) {
         GLfloat s[MAX_WIDTH], t[MAX_WIDTH], r[MAX_WIDTH], q[MAX_WIDTH];
         GLchan primary_rgba[MAX_WIDTH][4];
         GLuint unit;
         /* XXX not sure how multitexture is supposed to work here */

         MEMCPY(primary_rgba, rgba, 4 * width * sizeof(GLchan));

         for (unit = 0; unit < MAX_TEXTURE_UNITS; unit++) {
            _mesa_pixeltexgen(ctx, width, (const GLchan (*)[4]) rgba,
                              s, t, r, q);
            gl_texture_pixels(ctx, unit, width, s, t, r, NULL,
                              primary_rgba, rgba);
         }
      }

      /* write row to framebuffer */

      dy = desty + row;
      if (quick_draw && dy >= 0 && dy < ctx->DrawBuffer->Height) {
         (*ctx->Driver.WriteRGBASpan)( ctx, width, destx, dy, 
				       (const GLchan (*)[4])rgba, NULL );
      }
      else if (zoom) {
         gl_write_zoomed_rgba_span( ctx, width, destx, dy, zspan, 0, 
				    (const GLchan (*)[4])rgba, desty);
      }
      else {
         gl_write_rgba_span( ctx, width, destx, dy, zspan, 0, rgba, GL_BITMAP );
      }
   }

   FREE(convImage);
}


/*
 * RGBA copypixels
 */
static void
copy_rgba_pixels(GLcontext *ctx, GLint srcx, GLint srcy,
                 GLint width, GLint height, GLint destx, GLint desty)
{
   GLdepth zspan[MAX_WIDTH];
   GLchan rgba[MAX_WIDTH][4];
   GLchan *tmpImage,*p;
   GLboolean quick_draw;
   GLint sy, dy, stepy;
   GLint i, j;
   GLboolean changeBuffer;
   GLchan *saveReadAlpha;
   const GLboolean zoom = ctx->Pixel.ZoomX != 1.0F || ctx->Pixel.ZoomY != 1.0F;
   GLint overlapping;
   const GLuint transferOps = ctx->ImageTransferState;

   if (ctx->Pixel.Convolution2DEnabled || ctx->Pixel.Separable2DEnabled) {
      copy_conv_rgba_pixels(ctx, srcx, srcy, width, height, destx, desty);
      return;
   }

   /* Determine if copy should be done bottom-to-top or top-to-bottom */
   if (srcy < desty) {
      /* top-down  max-to-min */
      sy = srcy + height - 1;
      dy = desty + height - 1;
      stepy = -1;
   }
   else {
      /* bottom-up  min-to-max */
      sy = srcy;
      dy = desty;
      stepy = 1;
   }

   overlapping = regions_overlap(srcx, srcy, destx, desty, width, height,
                                 ctx->Pixel.ZoomX, ctx->Pixel.ZoomY);

   if (ctx->Depth.Test || ctx->Fog.Enabled) {
      /* fill in array of z values */
      GLdepth z = (GLdepth) (ctx->Current.RasterPos[2] * ctx->Visual.DepthMax);
      for (i=0;i<width;i++) {
         zspan[i] = z;
      }
   }

   if (ctx->RasterMask == 0
       && !zoom
       && destx >= 0
       && destx + width <= ctx->DrawBuffer->Width) {
      quick_draw = GL_TRUE;
   }
   else {
      quick_draw = GL_FALSE;
   }

   /* If read and draw buffer are different we must do buffer switching */
   saveReadAlpha = ctx->ReadBuffer->Alpha;
   changeBuffer = ctx->Pixel.ReadBuffer != ctx->Color.DrawBuffer
               || ctx->DrawBuffer != ctx->ReadBuffer;

   if (overlapping) {
      GLint ssy = sy;
      tmpImage = (GLchan *) MALLOC(width * height * sizeof(GLchan) * 4);
      if (!tmpImage) {
         gl_error( ctx, GL_OUT_OF_MEMORY, "glCopyPixels" );
         return;
      }
      p = tmpImage;
      if (changeBuffer) {
         (*ctx->Driver.SetReadBuffer)( ctx, ctx->ReadBuffer,
                                       ctx->Pixel.DriverReadBuffer );
         if (ctx->Pixel.DriverReadBuffer == GL_FRONT_LEFT)
            ctx->ReadBuffer->Alpha = ctx->ReadBuffer->FrontLeftAlpha;
         else if (ctx->Pixel.DriverReadBuffer == GL_BACK_LEFT)
            ctx->ReadBuffer->Alpha = ctx->ReadBuffer->BackLeftAlpha;
         else if (ctx->Pixel.DriverReadBuffer == GL_FRONT_RIGHT)
            ctx->ReadBuffer->Alpha = ctx->ReadBuffer->FrontRightAlpha;
         else
            ctx->ReadBuffer->Alpha = ctx->ReadBuffer->BackRightAlpha;
      }
      for (j = 0; j < height; j++, ssy += stepy) {
         gl_read_rgba_span( ctx, ctx->ReadBuffer, width, srcx, ssy,
                            (GLchan (*)[4]) p );
         p += (width * sizeof(GLchan) * 4);
      }
      p = tmpImage;
   }
   else {
      tmpImage = NULL;  /* silence compiler warnings */
      p = NULL;
   }

   for (j = 0; j < height; j++, sy += stepy, dy += stepy) {
      /* Get source pixels */
      if (overlapping) {
         /* get from buffered image */
         MEMCPY(rgba, p, width * sizeof(GLchan) * 4);
         p += (width * sizeof(GLchan) * 4);
      }
      else {
         /* get from framebuffer */
         if (changeBuffer) {
            (*ctx->Driver.SetReadBuffer)( ctx, ctx->ReadBuffer,
                                          ctx->Pixel.DriverReadBuffer );
            if (ctx->Pixel.DriverReadBuffer == GL_FRONT_LEFT) {
               ctx->ReadBuffer->Alpha = ctx->ReadBuffer->FrontLeftAlpha;
            }
            else if (ctx->Pixel.DriverReadBuffer == GL_BACK_LEFT) {
               ctx->ReadBuffer->Alpha = ctx->ReadBuffer->BackLeftAlpha;
            }
            else if (ctx->Pixel.DriverReadBuffer == GL_FRONT_RIGHT) {
               ctx->ReadBuffer->Alpha = ctx->ReadBuffer->FrontRightAlpha;
            }
            else {
               ctx->ReadBuffer->Alpha = ctx->ReadBuffer->BackRightAlpha;
            }
         }
         gl_read_rgba_span( ctx, ctx->ReadBuffer, width, srcx, sy, rgba );
      }

      if (changeBuffer) {
         /* read from the draw buffer again (in case of blending) */
         (*ctx->Driver.SetReadBuffer)( ctx, ctx->DrawBuffer,
                                       ctx->Color.DriverDrawBuffer );
         ctx->ReadBuffer->Alpha = saveReadAlpha;
      }

      if (transferOps) {
         const GLfloat scale = (1.0F / CHAN_MAXF);
         GLfloat rgbaFloat[MAX_WIDTH][4];
         GLuint k;
         /* convert chan to float */
         for (k = 0; k < width; k++) {
            rgbaFloat[k][RCOMP] = (GLfloat) rgba[k][RCOMP] * scale;
            rgbaFloat[k][GCOMP] = (GLfloat) rgba[k][GCOMP] * scale;
            rgbaFloat[k][BCOMP] = (GLfloat) rgba[k][BCOMP] * scale;
            rgbaFloat[k][ACOMP] = (GLfloat) rgba[k][ACOMP] * scale;
         }
         /* scale & bias */
         if (transferOps & IMAGE_SCALE_BIAS_BIT) {
            _mesa_scale_and_bias_rgba(ctx, width, rgbaFloat);
         }
         /* color map lookup */
         if (transferOps & IMAGE_MAP_COLOR_BIT) {
            _mesa_map_rgba(ctx, width, rgbaFloat);
         }
         /* GL_COLOR_TABLE lookup */
         if (transferOps & IMAGE_COLOR_TABLE_BIT) {
            _mesa_lookup_rgba(&ctx->ColorTable, width, rgbaFloat);
         }
         /* convolution */
         if (transferOps & IMAGE_CONVOLUTION_BIT) {
            /* XXX to do */
         }
         /* GL_POST_CONVOLUTION_COLOR_TABLE lookup */
         if (transferOps & IMAGE_POST_CONVOLUTION_COLOR_TABLE_BIT) {
            _mesa_lookup_rgba(&ctx->PostConvolutionColorTable, width, rgbaFloat);
         }
         /* color matrix */
         if (transferOps & IMAGE_COLOR_MATRIX_BIT) {
            _mesa_transform_rgba(ctx, width, rgbaFloat);
         }
         /* GL_POST_COLOR_MATRIX_COLOR_TABLE lookup */
         if (transferOps & IMAGE_POST_COLOR_MATRIX_COLOR_TABLE_BIT) {
            _mesa_lookup_rgba(&ctx->PostColorMatrixColorTable, width, rgbaFloat);
         }
         /* update histogram count */
         if (transferOps & IMAGE_HISTOGRAM_BIT) {
            _mesa_update_histogram(ctx, width, (CONST GLfloat (*)[4]) rgbaFloat);
         }
         /* update min/max */
         if (transferOps & IMAGE_MIN_MAX_BIT) {
            _mesa_update_minmax(ctx, width, (CONST GLfloat (*)[4]) rgbaFloat);
         }
         /* clamp to [0,1] and convert float back to chan */
         for (k = 0; k < width; k++) {
            GLint r = (GLint) (rgbaFloat[k][RCOMP] * CHAN_MAXF);
            GLint g = (GLint) (rgbaFloat[k][GCOMP] * CHAN_MAXF);
            GLint b = (GLint) (rgbaFloat[k][BCOMP] * CHAN_MAXF);
            GLint a = (GLint) (rgbaFloat[k][ACOMP] * CHAN_MAXF);
            rgba[k][RCOMP] = (GLchan) CLAMP(r, 0, CHAN_MAX);
            rgba[k][GCOMP] = (GLchan) CLAMP(g, 0, CHAN_MAX);
            rgba[k][BCOMP] = (GLchan) CLAMP(b, 0, CHAN_MAX);
            rgba[k][ACOMP] = (GLchan) CLAMP(a, 0, CHAN_MAX);
         }
      }

      if (ctx->Texture.ReallyEnabled && ctx->Pixel.PixelTextureEnabled) {
         GLfloat s[MAX_WIDTH], t[MAX_WIDTH], r[MAX_WIDTH], q[MAX_WIDTH];
         GLchan primary_rgba[MAX_WIDTH][4];
         GLuint unit;
         /* XXX not sure how multitexture is supposed to work here */

         MEMCPY(primary_rgba, rgba, 4 * width * sizeof(GLchan));

         for (unit = 0; unit < MAX_TEXTURE_UNITS; unit++) {
            _mesa_pixeltexgen(ctx, width, (const GLchan (*)[4]) rgba,
                              s, t, r, q);
            gl_texture_pixels(ctx, unit, width, s, t, r, NULL,
                              primary_rgba, rgba);
         }
      }

      if (quick_draw && dy >= 0 && dy < ctx->DrawBuffer->Height) {
         (*ctx->Driver.WriteRGBASpan)( ctx, width, destx, dy, 
				       (const GLchan (*)[4])rgba, NULL );
      }
      else if (zoom) {
         gl_write_zoomed_rgba_span( ctx, width, destx, dy, zspan, 0,
				    (const GLchan (*)[4])rgba, desty);
      }
      else {
         gl_write_rgba_span( ctx, width, destx, dy, zspan, 0, rgba, GL_BITMAP );
      }
   }

   if (overlapping)
      FREE(tmpImage);
}


static void copy_ci_pixels( GLcontext *ctx,
                            GLint srcx, GLint srcy, GLint width, GLint height,
                            GLint destx, GLint desty )
{
   GLdepth zspan[MAX_WIDTH];
   GLuint *tmpImage,*p;
   GLint sy, dy, stepy;
   GLint i, j;
   GLboolean changeBuffer;
   const GLboolean zoom = ctx->Pixel.ZoomX != 1.0F || ctx->Pixel.ZoomY != 1.0F;
   const GLboolean shift_or_offset = ctx->Pixel.IndexShift || ctx->Pixel.IndexOffset;
   GLint overlapping;

   /* Determine if copy should be bottom-to-top or top-to-bottom */
   if (srcy<desty) {
      /* top-down  max-to-min */
      sy = srcy + height - 1;
      dy = desty + height - 1;
      stepy = -1;
   }
   else {
      /* bottom-up  min-to-max */
      sy = srcy;
      dy = desty;
      stepy = 1;
   }

   overlapping = regions_overlap(srcx, srcy, destx, desty, width, height,
                                 ctx->Pixel.ZoomX, ctx->Pixel.ZoomY);

   if (ctx->Depth.Test || ctx->Fog.Enabled) {
      /* fill in array of z values */
      GLdepth z = (GLdepth) (ctx->Current.RasterPos[2] * ctx->Visual.DepthMax);
      for (i=0;i<width;i++) {
         zspan[i] = z;
      }
   }

   /* If read and draw buffer are different we must do buffer switching */
   changeBuffer = ctx->Pixel.ReadBuffer != ctx->Color.DrawBuffer
               || ctx->DrawBuffer != ctx->ReadBuffer;

   if (overlapping) {
      GLint ssy = sy;
      tmpImage = (GLuint *) MALLOC(width * height * sizeof(GLuint));
      if (!tmpImage) {
         gl_error( ctx, GL_OUT_OF_MEMORY, "glCopyPixels" );
         return;
      }
      p = tmpImage;
      if (changeBuffer) {
         (*ctx->Driver.SetReadBuffer)( ctx, ctx->ReadBuffer,
                                       ctx->Pixel.DriverReadBuffer );
      }
      for (j = 0; j < height; j++, ssy += stepy) {
         gl_read_index_span( ctx, ctx->ReadBuffer, width, srcx, ssy, p );
         p += width;
      }
      p = tmpImage;
   }
   else {
      tmpImage = NULL;  /* silence compiler warning */
      p = NULL;
   }

   for (j = 0; j < height; j++, sy += stepy, dy += stepy) {
      GLuint indexes[MAX_WIDTH];
      if (overlapping) {
         MEMCPY(indexes, p, width * sizeof(GLuint));
         p += width;
      }
      else {
         if (changeBuffer) {
            (*ctx->Driver.SetReadBuffer)( ctx, ctx->ReadBuffer,
                                          ctx->Pixel.DriverReadBuffer );
         }
         gl_read_index_span( ctx, ctx->ReadBuffer, width, srcx, sy, indexes );
      }

      if (changeBuffer) {
         /* set read buffer back to draw buffer (in case of logicops) */
         (*ctx->Driver.SetReadBuffer)( ctx, ctx->DrawBuffer,
                                       ctx->Color.DriverDrawBuffer );
      }

      if (shift_or_offset) {
         _mesa_shift_and_offset_ci( ctx, width, indexes );
      }
      if (ctx->Pixel.MapColorFlag) {
         _mesa_map_ci( ctx, width, indexes );
      }

      if (zoom) {
         gl_write_zoomed_index_span( ctx, width, destx, dy, zspan, 0, indexes, desty );
      }
      else {
         gl_write_index_span(ctx, width, destx, dy, zspan, 0, indexes, GL_BITMAP);
      }
   }

   if (overlapping)
      FREE(tmpImage);
}



/*
 * TODO: Optimize!!!!
 */
static void copy_depth_pixels( GLcontext *ctx, GLint srcx, GLint srcy,
                               GLint width, GLint height,
                               GLint destx, GLint desty )
{
   GLfloat depth[MAX_WIDTH];
   GLdepth zspan[MAX_WIDTH];
   GLfloat *p, *tmpImage;
   GLuint indexes[MAX_WIDTH];
   GLchan rgba[MAX_WIDTH][4];
   GLint sy, dy, stepy;
   GLint i, j;
   const GLboolean zoom = ctx->Pixel.ZoomX != 1.0F || ctx->Pixel.ZoomY != 1.0F;
   GLint overlapping;

   if (!ctx->ReadBuffer->DepthBuffer || !ctx->DrawBuffer->DepthBuffer) {
      gl_error( ctx, GL_INVALID_OPERATION, "glCopyPixels" );
      return;
   }

   /* Determine if copy should be bottom-to-top or top-to-bottom */
   if (srcy<desty) {
      /* top-down  max-to-min */
      sy = srcy + height - 1;
      dy = desty + height - 1;
      stepy = -1;
   }
   else {
      /* bottom-up  min-to-max */
      sy = srcy;
      dy = desty;
      stepy = 1;
   }

   overlapping = regions_overlap(srcx, srcy, destx, desty, width, height,
                                 ctx->Pixel.ZoomX, ctx->Pixel.ZoomY);

   /* setup colors or indexes */
   if (ctx->Visual.RGBAflag) {
      GLuint *rgba32 = (GLuint *) rgba;
      GLuint color = *(GLuint*)( ctx->Current.Color );
      for (i = 0; i < width; i++) {
         rgba32[i] = color;
      }
   }
   else {
      for (i = 0; i < width; i++) {
         indexes[i] = ctx->Current.Index;
      }
   }

   if (overlapping) {
      GLint ssy = sy;
      tmpImage = (GLfloat *) MALLOC(width * height * sizeof(GLfloat));
      if (!tmpImage) {
         gl_error( ctx, GL_OUT_OF_MEMORY, "glCopyPixels" );
         return;
      }
      p = tmpImage;
      for (j = 0; j < height; j++, ssy += stepy) {
         _mesa_read_depth_span_float(ctx, width, srcx, ssy, p);
         p += width;
      }
      p = tmpImage;
   }
   else {
      tmpImage = NULL;  /* silence compiler warning */
      p = NULL;
   }

   for (j = 0; j < height; j++, sy += stepy, dy += stepy) {
      if (overlapping) {
         MEMCPY(depth, p, width * sizeof(GLfloat));
         p += width;
      }
      else {
         _mesa_read_depth_span_float(ctx, width, srcx, sy, depth);
      }

      for (i = 0; i < width; i++) {
         GLfloat d = depth[i] * ctx->Pixel.DepthScale + ctx->Pixel.DepthBias;
         zspan[i] = (GLdepth) (CLAMP(d, 0.0F, 1.0F) * ctx->Visual.DepthMax);
      }

      if (ctx->Visual.RGBAflag) {
         if (zoom) {
            gl_write_zoomed_rgba_span( ctx, width, destx, dy, zspan, 0,
				       (const GLchan (*)[4])rgba, desty );
         }
         else {
            gl_write_rgba_span( ctx, width, destx, dy, zspan, 0, 
				rgba, GL_BITMAP);
         }
      }
      else {
         if (zoom) {
            gl_write_zoomed_index_span( ctx, width, destx, dy,
                                        zspan, 0, indexes, desty );
         }
         else {
            gl_write_index_span( ctx, width, destx, dy,
                                 zspan, 0, indexes, GL_BITMAP );
         }
      }
   }

  if (overlapping)
     FREE(tmpImage);
}



static void copy_stencil_pixels( GLcontext *ctx, GLint srcx, GLint srcy,
                                 GLint width, GLint height,
                                 GLint destx, GLint desty )
{
   GLint sy, dy, stepy;
   GLint j;
   GLstencil *p, *tmpImage;
   const GLboolean zoom = ctx->Pixel.ZoomX != 1.0F || ctx->Pixel.ZoomY != 1.0F;
   const GLboolean shift_or_offset = ctx->Pixel.IndexShift || ctx->Pixel.IndexOffset;
   GLint overlapping;

   if (!ctx->DrawBuffer->Stencil || !ctx->ReadBuffer->Stencil) {
      gl_error( ctx, GL_INVALID_OPERATION, "glCopyPixels" );
      return;
   }

   /* Determine if copy should be bottom-to-top or top-to-bottom */
   if (srcy < desty) {
      /* top-down  max-to-min */
      sy = srcy + height - 1;
      dy = desty + height - 1;
      stepy = -1;
   }
   else {
      /* bottom-up  min-to-max */
      sy = srcy;
      dy = desty;
      stepy = 1;
   }

   overlapping = regions_overlap(srcx, srcy, destx, desty, width, height,
                                 ctx->Pixel.ZoomX, ctx->Pixel.ZoomY);

   if (overlapping) {
      GLint ssy = sy;
      tmpImage = (GLstencil *) MALLOC(width * height * sizeof(GLstencil));
      if (!tmpImage) {
         gl_error( ctx, GL_OUT_OF_MEMORY, "glCopyPixels" );
         return;
      }
      p = tmpImage;
      for (j = 0; j < height; j++, ssy += stepy) {
         _mesa_read_stencil_span( ctx, width, srcx, ssy, p );
         p += width;
      }
      p = tmpImage;
   }
   else {
      tmpImage = NULL;  /* silence compiler warning */
      p = NULL;
   }

   for (j = 0; j < height; j++, sy += stepy, dy += stepy) {
      GLstencil stencil[MAX_WIDTH];

      if (overlapping) {
         MEMCPY(stencil, p, width * sizeof(GLstencil));
         p += width;
      }
      else {
         _mesa_read_stencil_span( ctx, width, srcx, sy, stencil );
      }

      if (shift_or_offset) {
         _mesa_shift_and_offset_stencil( ctx, width, stencil );
      }
      if (ctx->Pixel.MapStencilFlag) {
         _mesa_map_stencil( ctx, width, stencil );
      }

      if (zoom) {
         gl_write_zoomed_stencil_span( ctx, width, destx, dy, stencil, desty );
      }
      else {
         _mesa_write_stencil_span( ctx, width, destx, dy, stencil );
      }
   }

   if (overlapping)
      FREE(tmpImage);
}




void
_swrast_CopyPixels( GLcontext *ctx,
		    GLint srcx, GLint srcy, GLsizei width, GLsizei height,
		    GLint destx, GLint desty,
		    GLenum type )
{
   if (type == GL_COLOR && ctx->Visual.RGBAflag) {
      copy_rgba_pixels( ctx, srcx, srcy, width, height, destx, desty );
   }
   else if (type == GL_COLOR && !ctx->Visual.RGBAflag) {
      copy_ci_pixels( ctx, srcx, srcy, width, height, destx, desty );
   }
   else if (type == GL_DEPTH) {
      copy_depth_pixels( ctx, srcx, srcy, width, height, destx, desty );
   }
   else if (type == GL_STENCIL) {
      copy_stencil_pixels( ctx, srcx, srcy, width, height, destx, desty );
   }
   else {
      gl_error( ctx, GL_INVALID_ENUM, "glCopyPixels" );
   }
}

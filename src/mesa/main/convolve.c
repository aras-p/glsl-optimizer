/* $Id: convolve.c,v 1.3 2000/08/22 18:54:25 brianp Exp $ */

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


/*
 * Image convolution functions.
 *
 * Notes: filter kernel elements are indexed by <n> and <m> as in
 * the GL spec.
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include "glheader.h"
#include "convolve.h"
#include "context.h"
#include "types.h"
#endif


static void
convolve_1d_reduce(GLint srcWidth, const GLfloat src[][4],
                   GLint filterWidth, const GLfloat filter[][4],
                   GLfloat dest[][4])
{
   GLint dstWidth;
   GLint i, n;

   if (filterWidth >= 1)
      dstWidth = srcWidth - (filterWidth - 1);
   else
      dstWidth = srcWidth;

   if (dstWidth <= 0)
      return;  /* null result */

   for (i = 0; i < dstWidth; i++) {
      GLfloat sumR = 0.0;
      GLfloat sumG = 0.0;
      GLfloat sumB = 0.0;
      GLfloat sumA = 0.0;
      for (n = 0; n < filterWidth; n++) {
         sumR += src[i + n][RCOMP] * filter[n][RCOMP];
         sumG += src[i + n][GCOMP] * filter[n][GCOMP];
         sumB += src[i + n][BCOMP] * filter[n][BCOMP];
         sumA += src[i + n][ACOMP] * filter[n][ACOMP];
      }
      dest[i][RCOMP] = sumR;
      dest[i][GCOMP] = sumG;
      dest[i][BCOMP] = sumB;
      dest[i][ACOMP] = sumA;
   }
}


static void
convolve_1d_constant(GLint srcWidth, const GLfloat src[][4],
                     GLint filterWidth, const GLfloat filter[][4],
                     GLfloat dest[][4],
                     const GLfloat borderColor[4])
{
   const GLint halfFilterWidth = filterWidth / 2;
   GLint i, n;

   for (i = 0; i < srcWidth; i++) {
      GLfloat sumR = 0.0;
      GLfloat sumG = 0.0;
      GLfloat sumB = 0.0;
      GLfloat sumA = 0.0;
      for (n = 0; n < filterWidth; n++) {
         if (i + n < halfFilterWidth || i + n - halfFilterWidth >= srcWidth) {
            sumR += borderColor[RCOMP] * filter[n][RCOMP];
            sumG += borderColor[GCOMP] * filter[n][GCOMP];
            sumB += borderColor[BCOMP] * filter[n][BCOMP];
            sumA += borderColor[ACOMP] * filter[n][ACOMP];
         }
         else {
            sumR += src[i + n - halfFilterWidth][RCOMP] * filter[n][RCOMP];
            sumG += src[i + n - halfFilterWidth][GCOMP] * filter[n][GCOMP];
            sumB += src[i + n - halfFilterWidth][BCOMP] * filter[n][BCOMP];
            sumA += src[i + n - halfFilterWidth][ACOMP] * filter[n][ACOMP];
         }
      }
      dest[i][RCOMP] = sumR;
      dest[i][GCOMP] = sumG;
      dest[i][BCOMP] = sumB;
      dest[i][ACOMP] = sumA;
   }
}


static void
convolve_1d_replicate(GLint srcWidth, const GLfloat src[][4],
                      GLint filterWidth, const GLfloat filter[][4],
                      GLfloat dest[][4])
{
   const GLint halfFilterWidth = filterWidth / 2;
   GLint i, n;

   for (i = 0; i < srcWidth; i++) {
      GLfloat sumR = 0.0;
      GLfloat sumG = 0.0;
      GLfloat sumB = 0.0;
      GLfloat sumA = 0.0;
      for (n = 0; n < filterWidth; n++) {
         if (i + n < halfFilterWidth) {
            sumR += src[0][RCOMP] * filter[n][RCOMP];
            sumG += src[0][GCOMP] * filter[n][GCOMP];
            sumB += src[0][BCOMP] * filter[n][BCOMP];
            sumA += src[0][ACOMP] * filter[n][ACOMP];
         }
         else if (i + n - halfFilterWidth >= srcWidth) {
            sumR += src[srcWidth - 1][RCOMP] * filter[n][RCOMP];
            sumG += src[srcWidth - 1][GCOMP] * filter[n][GCOMP];
            sumB += src[srcWidth - 1][BCOMP] * filter[n][BCOMP];
            sumA += src[srcWidth - 1][ACOMP] * filter[n][ACOMP];
         }
         else {
            sumR += src[i + n - halfFilterWidth][RCOMP] * filter[n][RCOMP];
            sumG += src[i + n - halfFilterWidth][GCOMP] * filter[n][GCOMP];
            sumB += src[i + n - halfFilterWidth][BCOMP] * filter[n][BCOMP];
            sumA += src[i + n - halfFilterWidth][ACOMP] * filter[n][ACOMP];
         }
      }
      dest[i][RCOMP] = sumR;
      dest[i][GCOMP] = sumG;
      dest[i][BCOMP] = sumB;
      dest[i][ACOMP] = sumA;
   }
}


static void
convolve_2d_reduce(GLint srcWidth, GLint srcHeight,
                   const GLfloat src[][4],
                   GLint filterWidth, GLint filterHeight,
                   const GLfloat filter[][4],
                   GLfloat dest[][4])
{
   GLint dstWidth, dstHeight;
   GLint i, j, n, m;

   if (filterWidth >= 1)
      dstWidth = srcWidth - (filterWidth - 1);
   else
      dstWidth = srcWidth;

   if (filterHeight >= 1)
      dstHeight = srcHeight - (filterHeight - 1);
   else
      dstHeight = srcHeight;

   if (dstWidth <= 0 || dstHeight <= 0)
      return;

   for (j = 0; j < dstHeight; j++) {
      for (i = 0; i < dstWidth; i++) {
         GLfloat sumR = 0.0;
         GLfloat sumG = 0.0;
         GLfloat sumB = 0.0;
         GLfloat sumA = 0.0;
         for (m = 0; m < filterHeight; m++) {
            for (n = 0; n < filterWidth; n++) {
               const GLint k = (j + m) * srcWidth + i + n;
               const GLint f = m * filterWidth + n;
               sumR += src[k][RCOMP] * filter[f][RCOMP];
               sumG += src[k][GCOMP] * filter[f][GCOMP];
               sumB += src[k][BCOMP] * filter[f][BCOMP];
               sumA += src[k][ACOMP] * filter[f][ACOMP];
            }
         }
         dest[j * dstWidth + i][RCOMP] = sumR;
         dest[j * dstWidth + i][GCOMP] = sumG;
         dest[j * dstWidth + i][BCOMP] = sumB;
         dest[j * dstWidth + i][ACOMP] = sumA;
      }
   }
}


static void
convolve_2d_constant(GLint srcWidth, GLint srcHeight,
                     const GLfloat src[][4],
                     GLint filterWidth, GLint filterHeight,
                     const GLfloat filter[][4],
                     GLfloat dest[][4],
                     const GLfloat borderColor[4])
{
   const GLint halfFilterWidth = filterWidth / 2;
   const GLint halfFilterHeight = filterHeight / 2;
   GLint i, j, n, m;

   for (j = 0; j < srcHeight; j++) {
      for (i = 0; i < srcWidth; i++) {
         GLfloat sumR = 0.0;
         GLfloat sumG = 0.0;
         GLfloat sumB = 0.0;
         GLfloat sumA = 0.0;
         for (m = 0; m < filterHeight; m++) {
            for (n = 0; n < filterWidth; n++) {
               const GLint f = m * filterWidth + n;
               const GLint is = i + n - halfFilterWidth;
               const GLint js = j + m - halfFilterHeight;
               if (is < 0 || is >= srcWidth ||
                   js < 0 || js >= srcHeight) {
                  sumR += borderColor[RCOMP] * filter[f][RCOMP];
                  sumG += borderColor[GCOMP] * filter[f][GCOMP];
                  sumB += borderColor[BCOMP] * filter[f][BCOMP];
                  sumA += borderColor[ACOMP] * filter[f][ACOMP];
               }
               else {
                  const GLint k = js * srcWidth + is;
                  sumR += src[k][RCOMP] * filter[f][RCOMP];
                  sumG += src[k][GCOMP] * filter[f][GCOMP];
                  sumB += src[k][BCOMP] * filter[f][BCOMP];
                  sumA += src[k][ACOMP] * filter[f][ACOMP];
               }
            }
         }
         dest[j * srcWidth + i][RCOMP] = sumR;
         dest[j * srcWidth + i][GCOMP] = sumG;
         dest[j * srcWidth + i][BCOMP] = sumB;
         dest[j * srcWidth + i][ACOMP] = sumA;
      }
   }
}


static void
convolve_2d_replicate(GLint srcWidth, GLint srcHeight,
                      const GLfloat src[][4],
                      GLint filterWidth, GLint filterHeight,
                      const GLfloat filter[][4],
                      GLfloat dest[][4])
{
   const GLint halfFilterWidth = filterWidth / 2;
   const GLint halfFilterHeight = filterHeight / 2;
   GLint i, j, n, m;

   for (j = 0; j < srcHeight; j++) {
      for (i = 0; i < srcWidth; i++) {
         GLfloat sumR = 0.0;
         GLfloat sumG = 0.0;
         GLfloat sumB = 0.0;
         GLfloat sumA = 0.0;
         for (m = 0; m < filterHeight; m++) {
            for (n = 0; n < filterWidth; n++) {
               const GLint f = m * filterWidth + n;
               GLint is = i + n - halfFilterWidth;
               GLint js = j + m - halfFilterHeight;
               GLint k;
               if (is < 0)
                  is = 0;
               else if (is >= srcWidth)
                  is = srcWidth - 1;
               if (js < 0)
                  js = 0;
               else if (js >= srcHeight)
                  js = srcHeight - 1;
               k = js * srcWidth + is;
               sumR += src[k][RCOMP] * filter[f][RCOMP];
               sumG += src[k][GCOMP] * filter[f][GCOMP];
               sumB += src[k][BCOMP] * filter[f][BCOMP];
               sumA += src[k][ACOMP] * filter[f][ACOMP];
            }
         }
         dest[j * srcWidth + i][RCOMP] = sumR;
         dest[j * srcWidth + i][GCOMP] = sumG;
         dest[j * srcWidth + i][BCOMP] = sumB;
         dest[j * srcWidth + i][ACOMP] = sumA;
      }
   }
}


static void
convolve_sep_reduce(GLint srcWidth, GLint srcHeight,
                    const GLfloat src[][4],
                    GLint filterWidth, GLint filterHeight,
                    const GLfloat rowFilt[][4],
                    const GLfloat colFilt[][4],
                    GLfloat dest[][4])
{
   GLint dstWidth, dstHeight;
   GLint i, j, n, m;

   if (filterWidth >= 1)
      dstWidth = srcWidth - (filterWidth - 1);
   else
      dstWidth = srcWidth;

   if (filterHeight >= 1)
      dstHeight = srcHeight - (filterHeight - 1);
   else
      dstHeight = srcHeight;

   if (dstWidth <= 0 || dstHeight <= 0)
      return;

   for (j = 0; j < dstHeight; j++) {
      for (i = 0; i < dstWidth; i++) {
         GLfloat sumR = 0.0;
         GLfloat sumG = 0.0;
         GLfloat sumB = 0.0;
         GLfloat sumA = 0.0;
         for (m = 0; m < filterHeight; m++) {
            for (n = 0; n < filterWidth; n++) {
               GLint k = (j + m) * srcWidth + i + n;
               sumR += src[k][RCOMP] * rowFilt[n][RCOMP] * colFilt[m][RCOMP];
               sumG += src[k][GCOMP] * rowFilt[n][GCOMP] * colFilt[m][GCOMP];
               sumB += src[k][BCOMP] * rowFilt[n][BCOMP] * colFilt[m][BCOMP];
               sumA += src[k][ACOMP] * rowFilt[n][ACOMP] * colFilt[m][ACOMP];
            }
         }
         dest[j * dstWidth + i][RCOMP] = sumR;
         dest[j * dstWidth + i][GCOMP] = sumG;
         dest[j * dstWidth + i][BCOMP] = sumB;
         dest[j * dstWidth + i][ACOMP] = sumA;
      }
   }
}


static void
convolve_sep_constant(GLint srcWidth, GLint srcHeight,
                      const GLfloat src[][4],
                      GLint filterWidth, GLint filterHeight,
                      const GLfloat rowFilt[][4],
                      const GLfloat colFilt[][4],
                      GLfloat dest[][4],
                      const GLfloat borderColor[4])
{
   const GLint halfFilterWidth = filterWidth / 2;
   const GLint halfFilterHeight = filterHeight / 2;
   GLint i, j, n, m;

   for (j = 0; j < srcHeight; j++) {
      for (i = 0; i < srcWidth; i++) {
         GLfloat sumR = 0.0;
         GLfloat sumG = 0.0;
         GLfloat sumB = 0.0;
         GLfloat sumA = 0.0;
         for (m = 0; m < filterHeight; m++) {
            for (n = 0; n < filterWidth; n++) {
               const GLint is = i + n - halfFilterWidth;
               const GLint js = j + m - halfFilterHeight;
               if (is < 0 || is >= srcWidth ||
                   js < 0 || js >= srcHeight) {
                  sumR += borderColor[RCOMP] * rowFilt[n][RCOMP] * colFilt[m][RCOMP];
                  sumG += borderColor[GCOMP] * rowFilt[n][GCOMP] * colFilt[m][GCOMP];
                  sumB += borderColor[BCOMP] * rowFilt[n][BCOMP] * colFilt[m][BCOMP];
                  sumA += borderColor[ACOMP] * rowFilt[n][ACOMP] * colFilt[m][ACOMP];
               }
               else {
                  GLint k = js * srcWidth + is;
                  sumR += src[k][RCOMP] * rowFilt[n][RCOMP] * colFilt[m][RCOMP];
                  sumG += src[k][GCOMP] * rowFilt[n][GCOMP] * colFilt[m][GCOMP];
                  sumB += src[k][BCOMP] * rowFilt[n][BCOMP] * colFilt[m][BCOMP];
                  sumA += src[k][ACOMP] * rowFilt[n][ACOMP] * colFilt[m][ACOMP];
               }

            }
         }
         dest[j * srcWidth + i][RCOMP] = sumR;
         dest[j * srcWidth + i][GCOMP] = sumG;
         dest[j * srcWidth + i][BCOMP] = sumB;
         dest[j * srcWidth + i][ACOMP] = sumA;
      }
   }
}


static void
convolve_sep_replicate(GLint srcWidth, GLint srcHeight,
                       const GLfloat src[][4],
                       GLint filterWidth, GLint filterHeight,
                       const GLfloat rowFilt[][4],
                       const GLfloat colFilt[][4],
                       GLfloat dest[][4])
{
   const GLint halfFilterWidth = filterWidth / 2;
   const GLint halfFilterHeight = filterHeight / 2;
   GLint i, j, n, m;

   for (j = 0; j < srcHeight; j++) {
      for (i = 0; i < srcWidth; i++) {
         GLfloat sumR = 0.0;
         GLfloat sumG = 0.0;
         GLfloat sumB = 0.0;
         GLfloat sumA = 0.0;
         for (m = 0; m < filterHeight; m++) {
            for (n = 0; n < filterWidth; n++) {
               GLint is = i + n - halfFilterWidth;
               GLint js = j + m - halfFilterHeight;
               GLint k;
               if (is < 0)
                  is = 0;
               else if (is >= srcWidth)
                  is = srcWidth - 1;
               if (js < 0)
                  js = 0;
               else if (js >= srcHeight)
                  js = srcHeight - 1;
               k = js * srcWidth + is;
               sumR += src[k][RCOMP] * rowFilt[n][RCOMP] * colFilt[m][RCOMP];
               sumG += src[k][GCOMP] * rowFilt[n][GCOMP] * colFilt[m][GCOMP];
               sumB += src[k][BCOMP] * rowFilt[n][BCOMP] * colFilt[m][BCOMP];
               sumA += src[k][ACOMP] * rowFilt[n][ACOMP] * colFilt[m][ACOMP];
            }
         }
         dest[j * srcWidth + i][RCOMP] = sumR;
         dest[j * srcWidth + i][GCOMP] = sumG;
         dest[j * srcWidth + i][BCOMP] = sumB;
         dest[j * srcWidth + i][ACOMP] = sumA;
      }
   }
}



void
_mesa_convolve_1d_image(const GLcontext *ctx, GLsizei *width,
                        const GLfloat *srcImage, GLfloat *dstImage)
{
   switch (ctx->Pixel.ConvolutionBorderMode[0]) {
      case GL_REDUCE:
         convolve_1d_reduce(*width, (const GLfloat (*)[4]) srcImage,
                            ctx->Convolution1D.Width,
                            (const GLfloat (*)[4]) ctx->Convolution1D.Filter,
                            (GLfloat (*)[4]) dstImage);
         *width = *width - (MAX2(ctx->Convolution1D.Width, 1) - 1);
         break;
      case GL_CONSTANT_BORDER:
         convolve_1d_constant(*width, (const GLfloat (*)[4]) srcImage,
                              ctx->Convolution1D.Width,
                              (const GLfloat (*)[4]) ctx->Convolution1D.Filter,
                              (GLfloat (*)[4]) dstImage,
                              ctx->Pixel.ConvolutionBorderColor[0]);
         break;
      case GL_REPLICATE_BORDER:
         convolve_1d_replicate(*width, (const GLfloat (*)[4]) srcImage,
                              ctx->Convolution1D.Width,
                              (const GLfloat (*)[4]) ctx->Convolution1D.Filter,
                              (GLfloat (*)[4]) dstImage);
         break;
      default:
         ;
   }
}


void
_mesa_convolve_2d_image(const GLcontext *ctx, GLsizei *width, GLsizei *height,
                        const GLfloat *srcImage, GLfloat *dstImage)
{
   switch (ctx->Pixel.ConvolutionBorderMode[1]) {
      case GL_REDUCE:
         convolve_2d_reduce(*width, *height,
                            (const GLfloat (*)[4]) srcImage,
                            ctx->Convolution2D.Width,
                            ctx->Convolution2D.Height,
                            (const GLfloat (*)[4]) ctx->Convolution2D.Filter,
                            (GLfloat (*)[4]) dstImage);
         *width = *width - (MAX2(ctx->Convolution2D.Width, 1) - 1);
         *height = *height - (MAX2(ctx->Convolution2D.Height, 1) - 1);
         break;
      case GL_CONSTANT_BORDER:
         convolve_2d_constant(*width, *height,
                              (const GLfloat (*)[4]) srcImage,
                              ctx->Convolution2D.Width,
                              ctx->Convolution2D.Height,
                              (const GLfloat (*)[4]) ctx->Convolution2D.Filter,
                              (GLfloat (*)[4]) dstImage,
                              ctx->Pixel.ConvolutionBorderColor[1]);
         break;
      case GL_REPLICATE_BORDER:
         convolve_2d_replicate(*width, *height,
                               (const GLfloat (*)[4]) srcImage,
                               ctx->Convolution2D.Width,
                               ctx->Convolution2D.Height,
                               (const GLfloat (*)[4])ctx->Convolution2D.Filter,
                               (GLfloat (*)[4]) dstImage);
         break;
      default:
         ;
      }
}


void
_mesa_convolve_sep_image(const GLcontext *ctx,
                         GLsizei *width, GLsizei *height,
                         const GLfloat *srcImage, GLfloat *dstImage)
{
   const GLfloat *rowFilter = ctx->Separable2D.Filter;
   const GLfloat *colFilter = rowFilter + 4 * MAX_CONVOLUTION_WIDTH;

   switch (ctx->Pixel.ConvolutionBorderMode[2]) {
      case GL_REDUCE:
         convolve_sep_reduce(*width, *height,
                             (const GLfloat (*)[4]) srcImage,
                             ctx->Separable2D.Width,
                             ctx->Separable2D.Height,
                             (const GLfloat (*)[4]) rowFilter,
                             (const GLfloat (*)[4]) colFilter,
                             (GLfloat (*)[4]) dstImage);
         *width = *width - (MAX2(ctx->Separable2D.Width, 1) - 1);
         *height = *height - (MAX2(ctx->Separable2D.Height, 1) - 1);
         break;
      case GL_CONSTANT_BORDER:
         convolve_sep_constant(*width, *height,
                               (const GLfloat (*)[4]) srcImage,
                               ctx->Separable2D.Width,
                               ctx->Separable2D.Height,
                               (const GLfloat (*)[4]) rowFilter,
                               (const GLfloat (*)[4]) colFilter,
                               (GLfloat (*)[4]) dstImage,
                               ctx->Pixel.ConvolutionBorderColor[2]);
         break;
      case GL_REPLICATE_BORDER:
         convolve_sep_replicate(*width, *height,
                                (const GLfloat (*)[4]) srcImage,
                                ctx->Separable2D.Width,
                                ctx->Separable2D.Height,
                                (const GLfloat (*)[4]) rowFilter,
                                (const GLfloat (*)[4]) colFilter,
                                (GLfloat (*)[4]) dstImage);
         break;
      default:
         ;
   }
}

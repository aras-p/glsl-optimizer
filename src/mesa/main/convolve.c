/* $Id: convolve.c,v 1.1 2000/07/12 13:00:09 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.3
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
#include "types.h"
#endif


void
_mesa_convolve_1d_reduce(GLint srcWidth, const GLfloat src[][4],
                         GLint filterWidth, const GLfloat filter[][4],
                         GLfloat dest[][4])
{
   const GLint dstWidth = srcWidth - (filterWidth - 1);
   GLint i, n;

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


void
_mesa_convolve_1d_constant(GLint srcWidth, const GLfloat src[][4],
                           GLint filterWidth, const GLfloat filter[][4],
                           const GLfloat borderColor[4], GLfloat dest[][4])
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


void
_mesa_convolve_1d_replicate(GLint srcWidth, const GLfloat src[][4],
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


/*
 * <src> is the source image width width = srcWidth, height = filterHeight.
 * <filter> has width <filterWidth> and height <filterHeight>.
 * <dst> is a 1-D image span of width <srcWidth> - (<filterWidth> - 1).
 */
void
_mesa_convolve_2d_reduce(GLint srcWidth, GLint srcHeight,
                         const GLfloat src[][4],
                         GLint filterWidth, GLint filterHeight,
                         const GLfloat filter[][4],
                         GLfloat dest[][4])
{
   const GLint dstWidth = srcWidth - (filterWidth - 1);
   GLint i, n, m;

   if (dstWidth <= 0)
      return;  /* null result */

   /* XXX todo */
   for (i = 0; i < dstWidth; i++) {
      GLfloat sumR = 0.0;
      GLfloat sumG = 0.0;
      GLfloat sumB = 0.0;
      GLfloat sumA = 0.0;
      for (n = 0; n < filterHeight; n++) {
         for (m = 0; m < filterWidth; m++) {
            const GLint k = n * srcWidth + i + m;
            sumR += src[k][RCOMP] * filter[n][RCOMP];
            sumG += src[k][GCOMP] * filter[n][GCOMP];
            sumB += src[k][BCOMP] * filter[n][BCOMP];
            sumA += src[k][ACOMP] * filter[n][ACOMP];
         }
      }
      dest[i][RCOMP] = sumR;
      dest[i][GCOMP] = sumG;
      dest[i][BCOMP] = sumB;
      dest[i][ACOMP] = sumA;
   }
}


void
_mesa_convolve_2d_constant(GLint srcWidth, GLint srcHeight,
                           const GLfloat src[][4],
                           GLint filterWidth, GLint filterHeight,
                           const GLfloat filter[][4],
                           GLfloat dest[][4],
                           const GLfloat borderColor[4])
{
   const GLint halfFilterWidth = filterWidth / 2;
   GLint i, n, m;

   for (i = 0; i < srcWidth; i++) {
      GLfloat sumR = 0.0;
      GLfloat sumG = 0.0;
      GLfloat sumB = 0.0;
      GLfloat sumA = 0.0;
      for (m = 0; m < filterHeight; m++) {
         const GLfloat (*filterRow)[4] = filter + m * filterWidth;
         for (n = 0; n < filterWidth; n++) {
            if (i + n < halfFilterWidth ||
                i + n - halfFilterWidth >= srcWidth) {
               sumR += borderColor[RCOMP] * filterRow[n][RCOMP];
               sumG += borderColor[GCOMP] * filterRow[n][GCOMP];
               sumB += borderColor[BCOMP] * filterRow[n][BCOMP];
               sumA += borderColor[ACOMP] * filterRow[n][ACOMP];
            }
            else {
               const GLint k = m * srcWidth + i + n - halfFilterWidth;
               sumR += src[k][RCOMP] * filterRow[n][RCOMP];
               sumG += src[k][GCOMP] * filterRow[n][GCOMP];
               sumB += src[k][BCOMP] * filterRow[n][BCOMP];
               sumA += src[k][ACOMP] * filterRow[n][ACOMP];
            }
         }
      }
      dest[i][RCOMP] = sumR;
      dest[i][GCOMP] = sumG;
      dest[i][BCOMP] = sumB;
      dest[i][ACOMP] = sumA;
   }
}


void
_mesa_convolve_2d_replicate(GLint srcWidth, GLint srcHeight,
                            const GLfloat src[][4],
                            GLint filterWidth, GLint filterHeight,
                            const GLfloat filter[][4],
                            GLfloat dest[][4])
{
   const GLint halfFilterWidth = filterWidth / 2;
   GLint i, n, m;

   for (i = 0; i < srcWidth; i++) {
      GLfloat sumR = 0.0;
      GLfloat sumG = 0.0;
      GLfloat sumB = 0.0;
      GLfloat sumA = 0.0;
      for (m = 0; m < filterHeight; m++) {
         const GLfloat (*filterRow)[4] = filter + m * filterWidth;
         for (n = 0; n < filterWidth; n++) {
            if (i + n < halfFilterWidth) {
               const GLint k = m * srcWidth + 0;
               sumR += src[k][RCOMP] * filterRow[n][RCOMP];
               sumG += src[k][GCOMP] * filterRow[n][GCOMP];
               sumB += src[k][BCOMP] * filterRow[n][BCOMP];
               sumA += src[k][ACOMP] * filterRow[n][ACOMP];
            }
            else if (i + n - halfFilterWidth >= srcWidth) {
               const GLint k = m * srcWidth + srcWidth - 1;
               sumR += src[k][RCOMP] * filterRow[n][RCOMP];
               sumG += src[k][GCOMP] * filterRow[n][GCOMP];
               sumB += src[k][BCOMP] * filterRow[n][BCOMP];
               sumA += src[k][ACOMP] * filterRow[n][ACOMP];
            }
            else {
               const GLint k = m * srcWidth + i + n - halfFilterWidth;
               sumR += src[k][RCOMP] * filterRow[n][RCOMP];
               sumG += src[k][GCOMP] * filterRow[n][GCOMP];
               sumB += src[k][BCOMP] * filterRow[n][BCOMP];
               sumA += src[k][ACOMP] * filterRow[n][ACOMP];
            }
         }
      }
      dest[i][RCOMP] = sumR;
      dest[i][GCOMP] = sumG;
      dest[i][BCOMP] = sumB;
      dest[i][ACOMP] = sumA;
   }
}


void
_mesa_convolve_sep_constant(GLint srcWidth, GLint srcHeight,
                            const GLfloat src[][4],
                            GLint filterWidth, GLint filterHeight,
                            const GLfloat rowFilt[][4],
                            const GLfloat colFilt[][4],
                            GLfloat dest[][4],
                            const GLfloat borderColor[4])
{
   const GLint halfFilterWidth = filterWidth / 2;
   GLint i, n, m;

   for (i = 0; i < srcWidth; i++) {
      GLfloat sumR = 0.0;
      GLfloat sumG = 0.0;
      GLfloat sumB = 0.0;
      GLfloat sumA = 0.0;
      for (m = 0; m < filterHeight; m++) {
         for (n = 0; n < filterWidth; n++) {
            if (i + n < halfFilterWidth ||
                i + n - halfFilterWidth >= srcWidth) {
               sumR += borderColor[RCOMP] * rowFilt[n][RCOMP] * colFilt[m][RCOMP];
               sumG += borderColor[GCOMP] * rowFilt[n][GCOMP] * colFilt[m][GCOMP];
               sumB += borderColor[BCOMP] * rowFilt[n][BCOMP] * colFilt[m][BCOMP];
               sumA += borderColor[ACOMP] * rowFilt[n][ACOMP] * colFilt[m][ACOMP];
            }
            else {
               const GLint k = m * srcWidth + i + n - halfFilterWidth;
               sumR += src[k][RCOMP] * rowFilt[n][RCOMP] * colFilt[m][RCOMP];
               sumG += src[k][GCOMP] * rowFilt[n][GCOMP] * colFilt[m][GCOMP];
               sumB += src[k][BCOMP] * rowFilt[n][BCOMP] * colFilt[m][BCOMP];
               sumA += src[k][ACOMP] * rowFilt[n][ACOMP] * colFilt[m][ACOMP];
            }
         }
      }
      dest[i][RCOMP] = sumR;
      dest[i][GCOMP] = sumG;
      dest[i][BCOMP] = sumB;
      dest[i][ACOMP] = sumA;
   }
}


void
_mesa_convolve_sep_reduce(GLint srcWidth, GLint srcHeight,
                          const GLfloat src[][4],
                          GLint filterWidth, GLint filterHeight,
                          const GLfloat rowFilt[][4],
                          const GLfloat colFilt[][4],
                          GLfloat dest[][4])
{
#if 00
   const GLint halfFilterWidth = filterWidth / 2;
   GLint i, n, m;

   for (i = 0; i < srcWidth; i++) {
      GLfloat sumR = 0.0;
      GLfloat sumG = 0.0;
      GLfloat sumB = 0.0;
      GLfloat sumA = 0.0;
      for (m = 0; m < filterHeight; m++) {
         for (n = 0; n < filterWidth; n++) {
            if (i + n < halfFilterWidth ||
                i + n - halfFilterWidth >= srcWidth) {
               sumR += borderColor[RCOMP] * rowFilt[n][RCOMP] * colFilt[m][RCOMP];
               sumG += borderColor[GCOMP] * rowFilt[n][GCOMP] * colFilt[m][GCOMP];
               sumB += borderColor[BCOMP] * rowFilt[n][BCOMP] * colFilt[m][BCOMP];
               sumA += borderColor[ACOMP] * rowFilt[n][ACOMP] * colFilt[m][ACOMP];
            }
            else {
               const GLint k = m * srcWidth + i + n - halfFilterWidth;
               sumR += src[k][RCOMP] * rowFilt[n][RCOMP] * colFilt[m][RCOMP];
               sumG += src[k][GCOMP] * rowFilt[n][GCOMP] * colFilt[m][GCOMP];
               sumB += src[k][BCOMP] * rowFilt[n][BCOMP] * colFilt[m][BCOMP];
               sumA += src[k][ACOMP] * rowFilt[n][ACOMP] * colFilt[m][ACOMP];
            }
         }
      }
      dest[i][RCOMP] = sumR;
      dest[i][GCOMP] = sumG;
      dest[i][BCOMP] = sumB;
      dest[i][ACOMP] = sumA;
   }
#endif
}


void
_mesa_convolve_sep_replicate(GLint srcWidth, GLint srcHeight,
                             const GLfloat src[][4],
                             GLint filterWidth, GLint filterHeight,
                             const GLfloat rowFilt[][4],
                             const GLfloat colFilt[][4],
                             GLfloat dest[][4])
{
   const GLint halfFilterWidth = filterWidth / 2;
   GLint i, n, m;

   for (i = 0; i < srcWidth; i++) {
      GLfloat sumR = 0.0;
      GLfloat sumG = 0.0;
      GLfloat sumB = 0.0;
      GLfloat sumA = 0.0;
      for (m = 0; m < filterHeight; m++) {
         for (n = 0; n < filterWidth; n++) {
            if (i + n < halfFilterWidth) {
               const GLint k = m * srcWidth + 0;
               sumR += src[k][RCOMP] * rowFilt[n][RCOMP] * colFilt[m][RCOMP];
               sumG += src[k][GCOMP] * rowFilt[n][GCOMP] * colFilt[m][GCOMP];
               sumB += src[k][BCOMP] * rowFilt[n][BCOMP] * colFilt[m][BCOMP];
               sumA += src[k][ACOMP] * rowFilt[n][ACOMP] * colFilt[m][ACOMP];
            }
            else if (i + n - halfFilterWidth >= srcWidth) {
               const GLint k = m * srcWidth + srcWidth - 1;
               sumR += src[k][RCOMP] * rowFilt[n][RCOMP] * colFilt[m][RCOMP];
               sumG += src[k][GCOMP] * rowFilt[n][GCOMP] * colFilt[m][GCOMP];
               sumB += src[k][BCOMP] * rowFilt[n][BCOMP] * colFilt[m][BCOMP];
               sumA += src[k][ACOMP] * rowFilt[n][ACOMP] * colFilt[m][ACOMP];
            }
            else {
               const GLint k = m * srcWidth + i + n - halfFilterWidth;
               sumR += src[k][RCOMP] * rowFilt[n][RCOMP] * colFilt[m][RCOMP];
               sumG += src[k][GCOMP] * rowFilt[n][GCOMP] * colFilt[m][GCOMP];
               sumB += src[k][BCOMP] * rowFilt[n][BCOMP] * colFilt[m][BCOMP];
               sumA += src[k][ACOMP] * rowFilt[n][ACOMP] * colFilt[m][ACOMP];
            }
         }
      }
      dest[i][RCOMP] = sumR;
      dest[i][GCOMP] = sumG;
      dest[i][BCOMP] = sumB;
      dest[i][ACOMP] = sumA;
   }
}

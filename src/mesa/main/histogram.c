/* $Id: histogram.c,v 1.2 2000/11/10 18:31:04 brianp Exp $ */

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
#include "colormac.h"
#include "context.h"
#include "image.h"
#include "histogram.h"
#include "mmath.h"
#endif



static void
pack_histogram( GLcontext *ctx,
                GLuint n, CONST GLuint rgba[][4],
                GLenum format, GLenum type, GLvoid *destination,
                const struct gl_pixelstore_attrib *packing )
{
   const GLint comps = _mesa_components_in_format(format);
   GLuint luminance[MAX_WIDTH];

   if (format == GL_LUMINANCE || format == GL_LUMINANCE_ALPHA) {
      GLuint i;
      for (i = 0; i < n; i++) {
         luminance[i] = rgba[i][RCOMP] + rgba[i][GCOMP] + rgba[i][BCOMP];
      }
   }

#define PACK_MACRO(TYPE)					\
   {								\
      GLuint i;							\
      switch (format) {						\
         case GL_RED:						\
            for (i=0;i<n;i++)					\
               dst[i] = (TYPE) rgba[i][RCOMP];			\
            break;						\
         case GL_GREEN:						\
            for (i=0;i<n;i++)					\
               dst[i] = (TYPE) rgba[i][GCOMP];			\
            break;						\
         case GL_BLUE:						\
            for (i=0;i<n;i++)					\
               dst[i] = (TYPE) rgba[i][BCOMP];			\
            break;						\
         case GL_ALPHA:						\
            for (i=0;i<n;i++)					\
               dst[i] = (TYPE) rgba[i][ACOMP];			\
            break;						\
         case GL_LUMINANCE:					\
            for (i=0;i<n;i++)					\
               dst[i] = (TYPE) luminance[i];			\
            break;						\
         case GL_LUMINANCE_ALPHA:				\
            for (i=0;i<n;i++) {					\
               dst[i*2+0] = (TYPE) luminance[i];		\
               dst[i*2+1] = (TYPE) rgba[i][ACOMP];		\
            }							\
            break;						\
         case GL_RGB:						\
            for (i=0;i<n;i++) {					\
               dst[i*3+0] = (TYPE) rgba[i][RCOMP];		\
               dst[i*3+1] = (TYPE) rgba[i][GCOMP];		\
               dst[i*3+2] = (TYPE) rgba[i][BCOMP];		\
            }							\
            break;						\
         case GL_RGBA:						\
            for (i=0;i<n;i++) {					\
               dst[i*4+0] = (TYPE) rgba[i][RCOMP];		\
               dst[i*4+1] = (TYPE) rgba[i][GCOMP];		\
               dst[i*4+2] = (TYPE) rgba[i][BCOMP];		\
               dst[i*4+3] = (TYPE) rgba[i][ACOMP];		\
            }							\
            break;						\
         case GL_BGR:						\
            for (i=0;i<n;i++) {					\
               dst[i*3+0] = (TYPE) rgba[i][BCOMP];		\
               dst[i*3+1] = (TYPE) rgba[i][GCOMP];		\
               dst[i*3+2] = (TYPE) rgba[i][RCOMP];		\
            }							\
            break;						\
         case GL_BGRA:						\
            for (i=0;i<n;i++) {					\
               dst[i*4+0] = (TYPE) rgba[i][BCOMP];		\
               dst[i*4+1] = (TYPE) rgba[i][GCOMP];		\
               dst[i*4+2] = (TYPE) rgba[i][RCOMP];		\
               dst[i*4+3] = (TYPE) rgba[i][ACOMP];		\
            }							\
            break;						\
         case GL_ABGR_EXT:					\
            for (i=0;i<n;i++) {					\
               dst[i*4+0] = (TYPE) rgba[i][ACOMP];		\
               dst[i*4+1] = (TYPE) rgba[i][BCOMP];		\
               dst[i*4+2] = (TYPE) rgba[i][GCOMP];		\
               dst[i*4+3] = (TYPE) rgba[i][RCOMP];		\
            }							\
            break;						\
         default:						\
            gl_problem(ctx, "bad format in pack_histogram");	\
      }								\
   }

   switch (type) {
      case GL_UNSIGNED_BYTE:
         {
            GLubyte *dst = (GLubyte *) destination;
            PACK_MACRO(GLubyte);
         }
         break;
      case GL_BYTE:
         {
            GLbyte *dst = (GLbyte *) destination;
            PACK_MACRO(GLbyte);
         }
         break;
      case GL_UNSIGNED_SHORT:
         {
            GLushort *dst = (GLushort *) destination;
            PACK_MACRO(GLushort);
            if (packing->SwapBytes) {
               _mesa_swap2(dst, n * comps);
            }
         }
         break;
      case GL_SHORT:
         {
            GLshort *dst = (GLshort *) destination;
            PACK_MACRO(GLshort);
            if (packing->SwapBytes) {
               _mesa_swap2((GLushort *) dst, n * comps);
            }
         }
         break;
      case GL_UNSIGNED_INT:
         {
            GLuint *dst = (GLuint *) destination;
            PACK_MACRO(GLuint);
            if (packing->SwapBytes) {
               _mesa_swap4(dst, n * comps);
            }
         }
         break;
      case GL_INT:
         {
            GLint *dst = (GLint *) destination;
            PACK_MACRO(GLint);
            if (packing->SwapBytes) {
               _mesa_swap4((GLuint *) dst, n * comps);
            }
         }
         break;
      case GL_FLOAT:
         {
            GLfloat *dst = (GLfloat *) destination;
            PACK_MACRO(GLfloat);
            if (packing->SwapBytes) {
               _mesa_swap4((GLuint *) dst, n * comps);
            }
         }
         break;
      default:
         gl_problem(ctx, "Bad type in pack_histogram");
   }

#undef PACK_MACRO
}



static void
pack_minmax( GLcontext *ctx, CONST GLfloat minmax[2][4],
             GLenum format, GLenum type, GLvoid *destination,
             const struct gl_pixelstore_attrib *packing )
{
   const GLint comps = _mesa_components_in_format(format);
   GLuint luminance[2];

   if (format == GL_LUMINANCE || format == GL_LUMINANCE_ALPHA) {
      luminance[0] = minmax[0][RCOMP] + minmax[0][GCOMP] + minmax[0][BCOMP];
      luminance[1] = minmax[1][RCOMP] + minmax[1][GCOMP] + minmax[1][BCOMP];
   }

#define PACK_MACRO(TYPE, CONVERSION)				\
   {								\
      GLuint i;							\
      switch (format) {						\
         case GL_RED:						\
            for (i=0;i<2;i++)					\
               dst[i] = CONVERSION (minmax[i][RCOMP]);		\
            break;						\
         case GL_GREEN:						\
            for (i=0;i<2;i++)					\
               dst[i] = CONVERSION (minmax[i][GCOMP]);		\
            break;						\
         case GL_BLUE:						\
            for (i=0;i<2;i++)					\
               dst[i] = CONVERSION (minmax[i][BCOMP]);		\
            break;						\
         case GL_ALPHA:						\
            for (i=0;i<2;i++)					\
               dst[i] = CONVERSION (minmax[i][ACOMP]);		\
            break;						\
         case GL_LUMINANCE:					\
            for (i=0;i<2;i++)					\
               dst[i] = CONVERSION (luminance[i]);		\
            break;						\
         case GL_LUMINANCE_ALPHA:				\
            for (i=0;i<2;i++) {					\
               dst[i*2+0] = CONVERSION (luminance[i]);		\
               dst[i*2+1] = CONVERSION (minmax[i][ACOMP]);	\
            }							\
            break;						\
         case GL_RGB:						\
            for (i=0;i<2;i++) {					\
               dst[i*3+0] = CONVERSION (minmax[i][RCOMP]);	\
               dst[i*3+1] = CONVERSION (minmax[i][GCOMP]);	\
               dst[i*3+2] = CONVERSION (minmax[i][BCOMP]);	\
            }							\
            break;						\
         case GL_RGBA:						\
            for (i=0;i<2;i++) {					\
               dst[i*4+0] = CONVERSION (minmax[i][RCOMP]);	\
               dst[i*4+1] = CONVERSION (minmax[i][GCOMP]);	\
               dst[i*4+2] = CONVERSION (minmax[i][BCOMP]);	\
               dst[i*4+3] = CONVERSION (minmax[i][ACOMP]);	\
            }							\
            break;						\
         case GL_BGR:						\
            for (i=0;i<2;i++) {					\
               dst[i*3+0] = CONVERSION (minmax[i][BCOMP]);	\
               dst[i*3+1] = CONVERSION (minmax[i][GCOMP]);	\
               dst[i*3+2] = CONVERSION (minmax[i][RCOMP]);	\
            }							\
            break;						\
         case GL_BGRA:						\
            for (i=0;i<2;i++) {					\
               dst[i*4+0] = CONVERSION (minmax[i][BCOMP]);	\
               dst[i*4+1] = CONVERSION (minmax[i][GCOMP]);	\
               dst[i*4+2] = CONVERSION (minmax[i][RCOMP]);	\
               dst[i*4+3] = CONVERSION (minmax[i][ACOMP]);	\
            }							\
            break;						\
         case GL_ABGR_EXT:					\
            for (i=0;i<2;i++) {					\
               dst[i*4+0] = CONVERSION (minmax[i][ACOMP]);	\
               dst[i*4+1] = CONVERSION (minmax[i][BCOMP]);	\
               dst[i*4+2] = CONVERSION (minmax[i][GCOMP]);	\
               dst[i*4+3] = CONVERSION (minmax[i][RCOMP]);	\
            }							\
            break;						\
         default:						\
            gl_problem(ctx, "bad format in pack_minmax");	\
      }								\
   }

   switch (type) {
      case GL_UNSIGNED_BYTE:
         {
            GLubyte *dst = (GLubyte *) destination;
            PACK_MACRO(GLubyte, FLOAT_TO_UBYTE);
         }
         break;
      case GL_BYTE:
         {
            GLbyte *dst = (GLbyte *) destination;
            PACK_MACRO(GLbyte, FLOAT_TO_BYTE);
         }
         break;
      case GL_UNSIGNED_SHORT:
         {
            GLushort *dst = (GLushort *) destination;
            PACK_MACRO(GLushort, FLOAT_TO_USHORT);
            if (packing->SwapBytes) {
               _mesa_swap2(dst, 2 * comps);
            }
         }
         break;
      case GL_SHORT:
         {
            GLshort *dst = (GLshort *) destination;
            PACK_MACRO(GLshort, FLOAT_TO_SHORT);
            if (packing->SwapBytes) {
               _mesa_swap2((GLushort *) dst, 2 * comps);
            }
         }
         break;
      case GL_UNSIGNED_INT:
         {
            GLuint *dst = (GLuint *) destination;
            PACK_MACRO(GLuint, FLOAT_TO_UINT);
            if (packing->SwapBytes) {
               _mesa_swap4(dst, 2 * comps);
            }
         }
         break;
      case GL_INT:
         {
            GLint *dst = (GLint *) destination;
            PACK_MACRO(GLint, FLOAT_TO_INT);
            if (packing->SwapBytes) {
               _mesa_swap4((GLuint *) dst, 2 * comps);
            }
         }
         break;
      case GL_FLOAT:
         {
            GLfloat *dst = (GLfloat *) destination;
            PACK_MACRO(GLfloat, (GLfloat));
            if (packing->SwapBytes) {
               _mesa_swap4((GLuint *) dst, 2 * comps);
            }
         }
         break;
      default:
         gl_problem(ctx, "Bad type in pack_minmax");
   }

#undef PACK_MACRO
}


/*
 * Given an internalFormat token passed to glHistogram or glMinMax,
 * return the corresponding base format.
 * Return -1 if invalid token.
 */
static GLint
base_histogram_format( GLenum format )
{
   switch (format) {
      case GL_ALPHA:
      case GL_ALPHA4:
      case GL_ALPHA8:
      case GL_ALPHA12:
      case GL_ALPHA16:
         return GL_ALPHA;
      case GL_LUMINANCE:
      case GL_LUMINANCE4:
      case GL_LUMINANCE8:
      case GL_LUMINANCE12:
      case GL_LUMINANCE16:
         return GL_LUMINANCE;
      case GL_LUMINANCE_ALPHA:
      case GL_LUMINANCE4_ALPHA4:
      case GL_LUMINANCE6_ALPHA2:
      case GL_LUMINANCE8_ALPHA8:
      case GL_LUMINANCE12_ALPHA4:
      case GL_LUMINANCE12_ALPHA12:
      case GL_LUMINANCE16_ALPHA16:
         return GL_LUMINANCE_ALPHA;
      case GL_RGB:
      case GL_R3_G3_B2:
      case GL_RGB4:
      case GL_RGB5:
      case GL_RGB8:
      case GL_RGB10:
      case GL_RGB12:
      case GL_RGB16:
         return GL_RGB;
      case GL_RGBA:
      case GL_RGBA2:
      case GL_RGBA4:
      case GL_RGB5_A1:
      case GL_RGBA8:
      case GL_RGB10_A2:
      case GL_RGBA12:
      case GL_RGBA16:
         return GL_RGBA;
      default:
         return -1;  /* error */
   }
}


void
_mesa_GetMinmax(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glGetMinmax");

   if (!ctx->Extensions.EXT_histogram) {
      gl_error(ctx, GL_INVALID_OPERATION, "glGetMinmax");
      return;
   }

   if (target != GL_MINMAX) {
      gl_error(ctx, GL_INVALID_ENUM, "glGetMinmax(target)");
      return;
   }

   if (format != GL_RED &&
       format != GL_GREEN &&
       format != GL_BLUE &&
       format != GL_ALPHA &&
       format != GL_RGB &&
       format != GL_RGBA &&
       format != GL_ABGR_EXT &&
       format != GL_LUMINANCE &&
       format != GL_LUMINANCE_ALPHA) {
      gl_error(ctx, GL_INVALID_ENUM, "glGetMinmax(format)");
      return;
   }

   if (type != GL_UNSIGNED_BYTE &&
       type != GL_BYTE &&
       type != GL_UNSIGNED_SHORT &&
       type != GL_SHORT &&
       type != GL_UNSIGNED_INT &&
       type != GL_INT &&
       type != GL_FLOAT) {
      gl_error(ctx, GL_INVALID_ENUM, "glGetMinmax(type)");
      return;
   }

   if (!values)
      return;

   {
      GLfloat minmax[2][4];
      minmax[0][RCOMP] = CLAMP(ctx->MinMax.Min[RCOMP], 0.0F, 1.0F);
      minmax[0][GCOMP] = CLAMP(ctx->MinMax.Min[GCOMP], 0.0F, 1.0F);
      minmax[0][BCOMP] = CLAMP(ctx->MinMax.Min[BCOMP], 0.0F, 1.0F);
      minmax[0][ACOMP] = CLAMP(ctx->MinMax.Min[ACOMP], 0.0F, 1.0F);
      minmax[1][RCOMP] = CLAMP(ctx->MinMax.Max[RCOMP], 0.0F, 1.0F);
      minmax[1][GCOMP] = CLAMP(ctx->MinMax.Max[GCOMP], 0.0F, 1.0F);
      minmax[1][BCOMP] = CLAMP(ctx->MinMax.Max[BCOMP], 0.0F, 1.0F);
      minmax[1][ACOMP] = CLAMP(ctx->MinMax.Max[ACOMP], 0.0F, 1.0F);
      pack_minmax(ctx, (CONST GLfloat (*)[4]) minmax,
                  format, type, values, &ctx->Pack);
   }

   if (reset) {
      _mesa_ResetMinmax(GL_MINMAX);
   }
}


void
_mesa_GetHistogram(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glGetHistogram");

   if (!ctx->Extensions.EXT_histogram) {
      gl_error(ctx, GL_INVALID_OPERATION, "glGetHistogram");
      return;
   }

   if (target != GL_HISTOGRAM) {
      gl_error(ctx, GL_INVALID_ENUM, "glGetHistogram(target)");
      return;
   }

   if (format != GL_RED &&
       format != GL_GREEN &&
       format != GL_BLUE &&
       format != GL_ALPHA &&
       format != GL_RGB &&
       format != GL_RGBA &&
       format != GL_ABGR_EXT &&
       format != GL_LUMINANCE &&
       format != GL_LUMINANCE_ALPHA) {
      gl_error(ctx, GL_INVALID_ENUM, "glGetHistogram(format)");
      return;
   }

   if (type != GL_UNSIGNED_BYTE &&
       type != GL_BYTE &&
       type != GL_UNSIGNED_SHORT &&
       type != GL_SHORT &&
       type != GL_UNSIGNED_INT &&
       type != GL_INT &&
       type != GL_FLOAT) {
      gl_error(ctx, GL_INVALID_ENUM, "glGetHistogram(type)");
      return;
   }

   if (!values)
      return;

   pack_histogram(ctx, ctx->Histogram.Width,
                  (CONST GLuint (*)[4]) ctx->Histogram.Count,
                  format, type, values, &ctx->Pack);

   if (reset) {
      GLuint i;
      for (i = 0; i < HISTOGRAM_TABLE_SIZE; i++) {
         ctx->Histogram.Count[i][0] = 0;
         ctx->Histogram.Count[i][1] = 0;
         ctx->Histogram.Count[i][2] = 0;
         ctx->Histogram.Count[i][3] = 0;
      }
   }
}


void
_mesa_GetHistogramParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glGetHistogramParameterfv");

   if (!ctx->Extensions.EXT_histogram) {
      gl_error(ctx, GL_INVALID_OPERATION, "glGetHistogramParameterfv");
      return;
   }

   if (target != GL_HISTOGRAM && target != GL_PROXY_HISTOGRAM) {
      gl_error(ctx, GL_INVALID_ENUM, "glGetHistogramParameterfv(target)");
      return;
   }

   switch (pname) {
      case GL_HISTOGRAM_WIDTH:
         *params = (GLfloat) ctx->Histogram.Width;
         break;
      case GL_HISTOGRAM_FORMAT:
         *params = (GLfloat) ctx->Histogram.Format;
         break;
      case GL_HISTOGRAM_RED_SIZE:
         *params = (GLfloat) ctx->Histogram.RedSize;
         break;
      case GL_HISTOGRAM_GREEN_SIZE:
         *params = (GLfloat) ctx->Histogram.GreenSize;
         break;
      case GL_HISTOGRAM_BLUE_SIZE:
         *params = (GLfloat) ctx->Histogram.BlueSize;
         break;
      case GL_HISTOGRAM_ALPHA_SIZE:
         *params = (GLfloat) ctx->Histogram.AlphaSize;
         break;
      case GL_HISTOGRAM_LUMINANCE_SIZE:
         *params = (GLfloat) ctx->Histogram.LuminanceSize;
         break;
      case GL_HISTOGRAM_SINK:
         *params = (GLfloat) ctx->Histogram.Sink;
         break;
      default:
         gl_error(ctx, GL_INVALID_ENUM, "glGetHistogramParameterfv(pname)");
   }
}


void
_mesa_GetHistogramParameteriv(GLenum target, GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glGetHistogramParameteriv");

   if (!ctx->Extensions.EXT_histogram) {
      gl_error(ctx, GL_INVALID_OPERATION, "glGetHistogramParameteriv");
      return;
   }

   if (target != GL_HISTOGRAM && target != GL_PROXY_HISTOGRAM) {
      gl_error(ctx, GL_INVALID_ENUM, "glGetHistogramParameteriv(target)");
      return;
   }

   switch (pname) {
      case GL_HISTOGRAM_WIDTH:
         *params = (GLint) ctx->Histogram.Width;
         break;
      case GL_HISTOGRAM_FORMAT:
         *params = (GLint) ctx->Histogram.Format;
         break;
      case GL_HISTOGRAM_RED_SIZE:
         *params = (GLint) ctx->Histogram.RedSize;
         break;
      case GL_HISTOGRAM_GREEN_SIZE:
         *params = (GLint) ctx->Histogram.GreenSize;
         break;
      case GL_HISTOGRAM_BLUE_SIZE:
         *params = (GLint) ctx->Histogram.BlueSize;
         break;
      case GL_HISTOGRAM_ALPHA_SIZE:
         *params = (GLint) ctx->Histogram.AlphaSize;
         break;
      case GL_HISTOGRAM_LUMINANCE_SIZE:
         *params = (GLint) ctx->Histogram.LuminanceSize;
         break;
      case GL_HISTOGRAM_SINK:
         *params = (GLint) ctx->Histogram.Sink;
         break;
      default:
         gl_error(ctx, GL_INVALID_ENUM, "glGetHistogramParameteriv(pname)");
   }
}


void
_mesa_GetMinmaxParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glGetMinmaxParameterfv");

   if (!ctx->Extensions.EXT_histogram) {
      gl_error(ctx, GL_INVALID_OPERATION, "glGetMinmaxParameterfv");
      return;
   }
   if (target != GL_MINMAX) {
      gl_error(ctx, GL_INVALID_ENUM, "glGetMinmaxParameterfv(target)");
      return;
   }
   if (pname == GL_MINMAX_FORMAT) {
      *params = (GLfloat) ctx->MinMax.Format;
   }
   else if (pname == GL_MINMAX_SINK) {
      *params = (GLfloat) ctx->MinMax.Sink;
   }
   else {
      gl_error(ctx, GL_INVALID_ENUM, "glGetMinMaxParameterfv(pname)");
   }
}


void
_mesa_GetMinmaxParameteriv(GLenum target, GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glGetMinmaxParameteriv");

   if (!ctx->Extensions.EXT_histogram) {
      gl_error(ctx, GL_INVALID_OPERATION, "glGetMinmaxParameteriv");
      return;
   }
   if (target != GL_MINMAX) {
      gl_error(ctx, GL_INVALID_ENUM, "glGetMinmaxParameteriv(target)");
      return;
   }
   if (pname == GL_MINMAX_FORMAT) {
      *params = (GLint) ctx->MinMax.Format;
   }
   else if (pname == GL_MINMAX_SINK) {
      *params = (GLint) ctx->MinMax.Sink;
   }
   else {
      gl_error(ctx, GL_INVALID_ENUM, "glGetMinMaxParameteriv(pname)");
   }
}


void
_mesa_Histogram(GLenum target, GLsizei width, GLenum internalFormat, GLboolean sink)
{
   GLuint i;
   GLboolean error = GL_FALSE;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glHistogram");

   if (!ctx->Extensions.EXT_histogram) {
      gl_error(ctx, GL_INVALID_OPERATION, "glHistogram");
      return;
   }

   if (target != GL_HISTOGRAM && target != GL_PROXY_HISTOGRAM) {
      gl_error(ctx, GL_INVALID_ENUM, "glHistogram(target)");
      return;
   }

   if (width < 0 || width > HISTOGRAM_TABLE_SIZE) {
      if (target == GL_PROXY_HISTOGRAM) {
         error = GL_TRUE;
      }
      else {
         if (width < 0)
            gl_error(ctx, GL_INVALID_VALUE, "glHistogram(width)");
         else
            gl_error(ctx, GL_TABLE_TOO_LARGE, "glHistogram(width)");
         return;
      }
   }

   if (width != 0 && _mesa_bitcount(width) != 1) {
      if (target == GL_PROXY_HISTOGRAM) {
         error = GL_TRUE;
      }
      else {
         gl_error(ctx, GL_INVALID_VALUE, "glHistogram(width)");
         return;
      }
   }

   if (base_histogram_format(internalFormat) < 0) {
      if (target == GL_PROXY_HISTOGRAM) {
         error = GL_TRUE;
      }
      else {
         gl_error(ctx, GL_INVALID_ENUM, "glHistogram(internalFormat)");
         return;
      }
   }

   /* reset histograms */
   for (i = 0; i < HISTOGRAM_TABLE_SIZE; i++) {
      ctx->Histogram.Count[i][0] = 0;
      ctx->Histogram.Count[i][1] = 0;
      ctx->Histogram.Count[i][2] = 0;
      ctx->Histogram.Count[i][3] = 0;
   }

   if (error) {
      ctx->Histogram.Width = 0;
      ctx->Histogram.Format = 0;
      ctx->Histogram.RedSize       = 0;
      ctx->Histogram.GreenSize     = 0;
      ctx->Histogram.BlueSize      = 0;
      ctx->Histogram.AlphaSize     = 0;
      ctx->Histogram.LuminanceSize = 0;
   }
   else {
      ctx->Histogram.Width = width;
      ctx->Histogram.Format = internalFormat;
      ctx->Histogram.Sink = sink;
      ctx->Histogram.RedSize       = 0xffffffff;
      ctx->Histogram.GreenSize     = 0xffffffff;
      ctx->Histogram.BlueSize      = 0xffffffff;
      ctx->Histogram.AlphaSize     = 0xffffffff;
      ctx->Histogram.LuminanceSize = 0xffffffff;
   }
   
   ctx->NewState |= _NEW_PIXEL;
}


void
_mesa_Minmax(GLenum target, GLenum internalFormat, GLboolean sink)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glMinmax");

   if (!ctx->Extensions.EXT_histogram) {
      gl_error(ctx, GL_INVALID_OPERATION, "glMinmax");
      return;
   }

   if (target != GL_MINMAX) {
      gl_error(ctx, GL_INVALID_ENUM, "glMinMax(target)");
      return;
   }

   if (base_histogram_format(internalFormat) < 0) {
      gl_error(ctx, GL_INVALID_ENUM, "glMinMax(internalFormat)");
      return;
   }

   ctx->MinMax.Sink = sink;
   ctx->NewState |= _NEW_PIXEL;
}


void
_mesa_ResetHistogram(GLenum target)
{
   GLuint i;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glResetHistogram");

   if (!ctx->Extensions.EXT_histogram) {
      gl_error(ctx, GL_INVALID_OPERATION, "glResetHistogram");
      return;
   }

   if (target != GL_HISTOGRAM) {
      gl_error(ctx, GL_INVALID_ENUM, "glResetHistogram(target)");
      return;
   }

   for (i = 0; i < HISTOGRAM_TABLE_SIZE; i++) {
      ctx->Histogram.Count[i][0] = 0;
      ctx->Histogram.Count[i][1] = 0;
      ctx->Histogram.Count[i][2] = 0;
      ctx->Histogram.Count[i][3] = 0;
   }

   ctx->NewState |= _NEW_PIXEL;
}


void
_mesa_ResetMinmax(GLenum target)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glResetMinmax");

   if (!ctx->Extensions.EXT_histogram) {
      gl_error(ctx, GL_INVALID_OPERATION, "glResetMinmax");
      return;
   }

   if (target != GL_MINMAX) {
      gl_error(ctx, GL_INVALID_ENUM, "glResetMinMax(target)");
      return;
   }

   ctx->MinMax.Min[RCOMP] = 1000;    ctx->MinMax.Max[RCOMP] = -1000;
   ctx->MinMax.Min[GCOMP] = 1000;    ctx->MinMax.Max[GCOMP] = -1000;
   ctx->MinMax.Min[BCOMP] = 1000;    ctx->MinMax.Max[BCOMP] = -1000;
   ctx->MinMax.Min[ACOMP] = 1000;    ctx->MinMax.Max[ACOMP] = -1000;
   ctx->NewState |= _NEW_PIXEL;
}

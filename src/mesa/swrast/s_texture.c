/* $Id: s_texture.c,v 1.32 2001/06/01 13:23:27 brianp Exp $ */

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
#include "context.h"
#include "colormac.h"
#include "macros.h"
#include "mmath.h"
#include "mem.h"
#include "texformat.h"
#include "teximage.h"

#include "s_context.h"
#include "s_pb.h"
#include "s_texture.h"


/*
 * These values are used in the fixed-point arithmetic used
 * for linear filtering.
 */
#define WEIGHT_SCALE 65536.0F
#define WEIGHT_SHIFT 16


/*
 * Used to compute texel locations for linear sampling.
 * Input:
 *    wrapMode = GL_REPEAT, GL_CLAMP, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER_ARB
 *    S = texcoord in [0,1]
 *    SIZE = width (or height or depth) of texture
 * Output:
 *    U = texcoord in [0, width]
 *    I0, I1 = two nearest texel indexes
 */
#define COMPUTE_LINEAR_TEXEL_LOCATIONS(wrapMode, S, U, SIZE, I0, I1)	\
{									\
   if (wrapMode == GL_REPEAT) {						\
      U = S * SIZE - 0.5F;						\
      I0 = IFLOOR(U) & (SIZE - 1);					\
      I1 = (I0 + 1) & (SIZE - 1);					\
   }									\
   else if (wrapMode == GL_CLAMP_TO_EDGE) {				\
      if (S <= 0.0F)							\
         U = 0.0F;							\
      else if (S >= 1.0F)						\
         U = SIZE;							\
      else								\
         U = S * SIZE;							\
      U -= 0.5F;							\
      I0 = IFLOOR(U);							\
      I1 = I0 + 1;							\
      if (I0 < 0)							\
         I0 = 0;							\
      if (I1 >= (GLint) SIZE)						\
         I1 = SIZE - 1;							\
   }									\
   else  if (wrapMode == GL_CLAMP_TO_BORDER_ARB) {			\
      const GLfloat min = -1.0F / (2.0F * SIZE);			\
      const GLfloat max = 1.0F - min;					\
      if (S <= min)							\
         U = min * SIZE;						\
      else if (S >= max)						\
         U = max * SIZE;						\
      else								\
         U = S * SIZE;							\
      U -= 0.5F;							\
      I0 = IFLOOR(U);							\
      I1 = I0 + 1;							\
   }									\
   else {								\
      ASSERT(wrapMode == GL_CLAMP);					\
      if (S <= 0.0F)							\
         U = 0.0F;							\
      else if (S >= 1.0F)						\
         U = SIZE;							\
      else								\
         U = S * SIZE;							\
      U -= 0.5F;							\
      I0 = IFLOOR(U);							\
      I1 = I0 + 1;							\
   }									\
}


/*
 * Used to compute texel location for nearest sampling.
 */
#define COMPUTE_NEAREST_TEXEL_LOCATION(wrapMode, S, SIZE, I)		\
{									\
   if (wrapMode == GL_REPEAT) {						\
      /* s limited to [0,1) */						\
      /* i limited to [0,size-1] */					\
      I = IFLOOR(S * SIZE);						\
      I &= (SIZE - 1);							\
   }									\
   else if (wrapMode == GL_CLAMP_TO_EDGE) {				\
      /* s limited to [min,max] */					\
      /* i limited to [0, size-1] */					\
      const GLfloat min = 1.0F / (2.0F * SIZE);				\
      const GLfloat max = 1.0F - min;					\
      if (S < min)							\
         I = 0;								\
      else if (S > max)							\
         I = SIZE - 1;							\
      else								\
         I = IFLOOR(S * SIZE);						\
   }									\
   else if (wrapMode == GL_CLAMP_TO_BORDER_ARB) {			\
      /* s limited to [min,max] */					\
      /* i limited to [-1, size] */					\
      const GLfloat min = -1.0F / (2.0F * SIZE);			\
      const GLfloat max = 1.0F - min;					\
      if (S <= min)							\
         I = -1;							\
      else if (S >= max)						\
         I = SIZE;							\
      else								\
         I = IFLOOR(S * SIZE);						\
   }									\
   else {								\
      ASSERT(wrapMode == GL_CLAMP);					\
      /* s limited to [0,1] */						\
      /* i limited to [0,size-1] */					\
      if (S <= 0.0F)							\
         I = 0;								\
      else if (S >= 1.0F)						\
         I = SIZE - 1;							\
      else								\
         I = IFLOOR(S * SIZE);						\
   }									\
}


/*
 * Compute linear mipmap levels for given lambda.
 */
#define COMPUTE_LINEAR_MIPMAP_LEVEL(tObj, lambda, level)	\
{								\
   if (lambda < 0.0F)						\
      lambda = 0.0F;						\
   else if (lambda > tObj->_MaxLambda)				\
      lambda = tObj->_MaxLambda;				\
   level = (GLint) (tObj->BaseLevel + lambda);			\
}


/*
 * Compute nearest mipmap level for given lambda.
 */
#define COMPUTE_NEAREST_MIPMAP_LEVEL(tObj, lambda, level)	\
{								\
   if (lambda <= 0.5F)						\
      lambda = 0.0F;						\
   else if (lambda > tObj->_MaxLambda + 0.4999F)		\
      lambda = tObj->_MaxLambda + 0.4999F;			\
   level = (GLint) (tObj->BaseLevel + lambda + 0.5F);		\
   if (level > tObj->_MaxLevel)					\
      level = tObj->_MaxLevel;					\
}



/*
 * Note, the FRAC macro has to work perfectly.  Otherwise you'll sometimes
 * see 1-pixel bands of improperly weighted linear-sampled texels.  The
 * tests/texwrap.c demo is a good test.
 * Also note, FRAC(x) doesn't truly return the fractional part of x for x < 0.
 * Instead, if x < 0 then FRAC(x) = 1 - true_frac(x).
 */
#define FRAC(f)  ((f) - IFLOOR(f))



/*
 * Bitflags for texture border color sampling.
 */
#define I0BIT   1
#define I1BIT   2
#define J0BIT   4
#define J1BIT   8
#define K0BIT  16
#define K1BIT  32



/*
 * Get texture palette entry.
 */
static void
palette_sample(const GLcontext *ctx,
               const struct gl_texture_object *tObj,
               GLint index, GLchan rgba[4] )
{
   const GLchan *palette;
   GLenum format;

   if (ctx->Texture.SharedPalette) {
      ASSERT(!ctx->Texture.Palette.FloatTable);
      palette = (const GLchan *) ctx->Texture.Palette.Table;
      format = ctx->Texture.Palette.Format;
   }
   else {
      ASSERT(!tObj->Palette.FloatTable);
      palette = (const GLchan *) tObj->Palette.Table;
      format = tObj->Palette.Format;
   }

   switch (format) {
      case GL_ALPHA:
         rgba[ACOMP] = palette[index];
         return;
      case GL_LUMINANCE:
      case GL_INTENSITY:
         rgba[RCOMP] = palette[index];
         return;
      case GL_LUMINANCE_ALPHA:
         rgba[RCOMP] = palette[(index << 1) + 0];
         rgba[ACOMP] = palette[(index << 1) + 1];
         return;
      case GL_RGB:
         rgba[RCOMP] = palette[index * 3 + 0];
         rgba[GCOMP] = palette[index * 3 + 1];
         rgba[BCOMP] = palette[index * 3 + 2];
         return;
      case GL_RGBA:
         rgba[RCOMP] = palette[(index << 2) + 0];
         rgba[GCOMP] = palette[(index << 2) + 1];
         rgba[BCOMP] = palette[(index << 2) + 2];
         rgba[ACOMP] = palette[(index << 2) + 3];
         return;
      default:
         _mesa_problem(ctx, "Bad palette format in palette_sample");
   }
}



/**********************************************************************/
/*                    1-D Texture Sampling Functions                  */
/**********************************************************************/

/*
 * Return the texture sample for coordinate (s) using GL_NEAREST filter.
 */
static void
sample_1d_nearest(GLcontext *ctx,
                  const struct gl_texture_object *tObj,
                  const struct gl_texture_image *img,
                  GLfloat s, GLchan rgba[4])
{
   const GLint width = img->Width2;  /* without border, power of two */
   GLint i;

   COMPUTE_NEAREST_TEXEL_LOCATION(tObj->WrapS, s, width, i);

   /* skip over the border, if any */
   i += img->Border;

   if (i < 0 || i >= (GLint) img->Width) {
      /* Need this test for GL_CLAMP_TO_BORDER_ARB mode */
      COPY_CHAN4(rgba, tObj->BorderColor);
   }
   else {
      (*img->FetchTexel)(img, i, 0, 0, (GLvoid *) rgba);
      if (img->Format == GL_COLOR_INDEX) {
         palette_sample(ctx, tObj, rgba[0], rgba);
      }
   }
}



/*
 * Return the texture sample for coordinate (s) using GL_LINEAR filter.
 */
static void
sample_1d_linear(GLcontext *ctx,
                 const struct gl_texture_object *tObj,
                 const struct gl_texture_image *img,
                 GLfloat s, GLchan rgba[4])
{
   const GLint width = img->Width2;
   GLint i0, i1;
   GLfloat u;
   GLuint useBorderColor;

   COMPUTE_LINEAR_TEXEL_LOCATIONS(tObj->WrapS, s, u, width, i0, i1);

   useBorderColor = 0;
   if (img->Border) {
      i0 += img->Border;
      i1 += img->Border;
   }
   else {
      if (i0 < 0 || i0 >= width)   useBorderColor |= I0BIT;
      if (i1 < 0 || i1 >= width)   useBorderColor |= I1BIT;
   }

   {
      const GLfloat a = FRAC(u);

#if CHAN_BITS == 32
      const GLfloat w0 = (1.0F-a);
      const GLfloat w1 =       a ;
#else /* CHAN_BITS == 8 || CHAN_BITS == 16 */
      /* compute sample weights in fixed point in [0,WEIGHT_SCALE] */
      const GLint w0 = IROUND_POS((1.0F - a) * WEIGHT_SCALE);
      const GLint w1 = IROUND_POS(        a  * WEIGHT_SCALE);
#endif

      GLchan t0[4], t1[4];  /* texels */

      if (useBorderColor & I0BIT) {
         COPY_CHAN4(t0, tObj->BorderColor);
      }
      else {
         (*img->FetchTexel)(img, i0, 0, 0, (GLvoid *) t0);
         if (img->Format == GL_COLOR_INDEX) {
            palette_sample(ctx, tObj, t0[0], t0);
         }
      }
      if (useBorderColor & I1BIT) {
         COPY_CHAN4(t1, tObj->BorderColor);
      }
      else {
         (*img->FetchTexel)(img, i1, 0, 0, (GLvoid *) t1);
         if (img->Format == GL_COLOR_INDEX) {
            palette_sample(ctx, tObj, t1[0], t1);
         }
      }

#if CHAN_BITS == 32
      rgba[0] = w0 * t0[0] + w1 * t1[0];
      rgba[1] = w0 * t0[1] + w1 * t1[1];
      rgba[2] = w0 * t0[2] + w1 * t1[2];
      rgba[3] = w0 * t0[3] + w1 * t1[3];
#else /* CHAN_BITS == 8 || CHAN_BITS == 16 */
      rgba[0] = (GLchan) ((w0 * t0[0] + w1 * t1[0]) >> WEIGHT_SHIFT);
      rgba[1] = (GLchan) ((w0 * t0[1] + w1 * t1[1]) >> WEIGHT_SHIFT);
      rgba[2] = (GLchan) ((w0 * t0[2] + w1 * t1[2]) >> WEIGHT_SHIFT);
      rgba[3] = (GLchan) ((w0 * t0[3] + w1 * t1[3]) >> WEIGHT_SHIFT);
#endif

   }
}


static void
sample_1d_nearest_mipmap_nearest(GLcontext *ctx,
                                 const struct gl_texture_object *tObj,
                                 GLfloat s, GLfloat lambda,
                                 GLchan rgba[4])
{
   GLint level;
   COMPUTE_NEAREST_MIPMAP_LEVEL(tObj, lambda, level);
   sample_1d_nearest(ctx, tObj, tObj->Image[level], s, rgba);
}


static void
sample_1d_linear_mipmap_nearest(GLcontext *ctx,
                                const struct gl_texture_object *tObj,
                                GLfloat s, GLfloat lambda,
                                GLchan rgba[4])
{
   GLint level;
   COMPUTE_NEAREST_MIPMAP_LEVEL(tObj, lambda, level);
   sample_1d_linear(ctx, tObj, tObj->Image[level], s, rgba);
}



static void
sample_1d_nearest_mipmap_linear(GLcontext *ctx,
                                const struct gl_texture_object *tObj,
                                GLfloat s, GLfloat lambda,
                                GLchan rgba[4])
{
   GLint level;

   COMPUTE_LINEAR_MIPMAP_LEVEL(tObj, lambda, level);

   if (level >= tObj->_MaxLevel) {
      sample_1d_nearest(ctx, tObj, tObj->Image[tObj->_MaxLevel], s, rgba);
   }
   else {
      GLchan t0[4], t1[4];
      const GLfloat f = FRAC(lambda);
      sample_1d_nearest(ctx, tObj, tObj->Image[level  ], s, t0);
      sample_1d_nearest(ctx, tObj, tObj->Image[level+1], s, t1);
      rgba[RCOMP] = (GLchan) (GLint) ((1.0F-f) * t0[RCOMP] + f * t1[RCOMP]);
      rgba[GCOMP] = (GLchan) (GLint) ((1.0F-f) * t0[GCOMP] + f * t1[GCOMP]);
      rgba[BCOMP] = (GLchan) (GLint) ((1.0F-f) * t0[BCOMP] + f * t1[BCOMP]);
      rgba[ACOMP] = (GLchan) (GLint) ((1.0F-f) * t0[ACOMP] + f * t1[ACOMP]);
   }
}



static void
sample_1d_linear_mipmap_linear(GLcontext *ctx,
                               const struct gl_texture_object *tObj,
                               GLfloat s, GLfloat lambda,
                               GLchan rgba[4])
{
   GLint level;

   COMPUTE_LINEAR_MIPMAP_LEVEL(tObj, lambda, level);

   if (level >= tObj->_MaxLevel) {
      sample_1d_linear(ctx, tObj, tObj->Image[tObj->_MaxLevel], s, rgba);
   }
   else {
      GLchan t0[4], t1[4];
      const GLfloat f = FRAC(lambda);
      sample_1d_linear(ctx, tObj, tObj->Image[level  ], s, t0);
      sample_1d_linear(ctx, tObj, tObj->Image[level+1], s, t1);
      rgba[RCOMP] = (GLchan) (GLint) ((1.0F-f) * t0[RCOMP] + f * t1[RCOMP]);
      rgba[GCOMP] = (GLchan) (GLint) ((1.0F-f) * t0[GCOMP] + f * t1[GCOMP]);
      rgba[BCOMP] = (GLchan) (GLint) ((1.0F-f) * t0[BCOMP] + f * t1[BCOMP]);
      rgba[ACOMP] = (GLchan) (GLint) ((1.0F-f) * t0[ACOMP] + f * t1[ACOMP]);
   }
}



static void
sample_nearest_1d( GLcontext *ctx, GLuint texUnit,
                   const struct gl_texture_object *tObj, GLuint n,
                   const GLfloat s[], const GLfloat t[],
                   const GLfloat u[], const GLfloat lambda[],
                   GLchan rgba[][4] )
{
   GLuint i;
   struct gl_texture_image *image = tObj->Image[tObj->BaseLevel];
   (void) t;
   (void) u;
   (void) lambda;
   for (i=0;i<n;i++) {
      sample_1d_nearest(ctx, tObj, image, s[i], rgba[i]);
   }
}



static void
sample_linear_1d( GLcontext *ctx, GLuint texUnit,
                  const struct gl_texture_object *tObj, GLuint n,
                  const GLfloat s[], const GLfloat t[],
                  const GLfloat u[], const GLfloat lambda[],
                  GLchan rgba[][4] )
{
   GLuint i;
   struct gl_texture_image *image = tObj->Image[tObj->BaseLevel];
   (void) t;
   (void) u;
   (void) lambda;
   for (i=0;i<n;i++) {
      sample_1d_linear(ctx, tObj, image, s[i], rgba[i]);
   }
}


/*
 * Given an (s) texture coordinate and lambda (level of detail) value,
 * return a texture sample.
 *
 */
static void
sample_lambda_1d( GLcontext *ctx, GLuint texUnit,
                  const struct gl_texture_object *tObj, GLuint n,
                  const GLfloat s[], const GLfloat t[],
                  const GLfloat u[], const GLfloat lambda[],
                  GLchan rgba[][4] )
{
   GLfloat MinMagThresh = SWRAST_CONTEXT(ctx)->_MinMagThresh[texUnit];
   GLuint i;

   (void) t;
   (void) u;

   for (i=0;i<n;i++) {
      if (lambda[i] > MinMagThresh) {
         /* minification */
         switch (tObj->MinFilter) {
            case GL_NEAREST:
               sample_1d_nearest(ctx, tObj, tObj->Image[tObj->BaseLevel],
                                 s[i], rgba[i]);
               break;
            case GL_LINEAR:
               sample_1d_linear(ctx, tObj, tObj->Image[tObj->BaseLevel],
                                s[i], rgba[i]);
               break;
            case GL_NEAREST_MIPMAP_NEAREST:
               sample_1d_nearest_mipmap_nearest(ctx, tObj, lambda[i], s[i],
                                                rgba[i]);
               break;
            case GL_LINEAR_MIPMAP_NEAREST:
               sample_1d_linear_mipmap_nearest(ctx, tObj, s[i], lambda[i],
                                               rgba[i]);
               break;
            case GL_NEAREST_MIPMAP_LINEAR:
               sample_1d_nearest_mipmap_linear(ctx, tObj, s[i], lambda[i],
                                               rgba[i]);
               break;
            case GL_LINEAR_MIPMAP_LINEAR:
               sample_1d_linear_mipmap_linear(ctx, tObj, s[i], lambda[i],
                                              rgba[i]);
               break;
            default:
               _mesa_problem(NULL, "Bad min filter in sample_1d_texture");
               return;
         }
      }
      else {
         /* magnification */
         switch (tObj->MagFilter) {
            case GL_NEAREST:
               sample_1d_nearest(ctx, tObj, tObj->Image[tObj->BaseLevel],
                                 s[i], rgba[i]);
               break;
            case GL_LINEAR:
               sample_1d_linear(ctx, tObj, tObj->Image[tObj->BaseLevel],
                                s[i], rgba[i]);
               break;
            default:
               _mesa_problem(NULL, "Bad mag filter in sample_1d_texture");
               return;
         }
      }
   }
}




/**********************************************************************/
/*                    2-D Texture Sampling Functions                  */
/**********************************************************************/


/*
 * Return the texture sample for coordinate (s,t) using GL_NEAREST filter.
 */
static void
sample_2d_nearest(GLcontext *ctx,
                  const struct gl_texture_object *tObj,
                  const struct gl_texture_image *img,
                  GLfloat s, GLfloat t,
                  GLchan rgba[])
{
   const GLint width = img->Width2;    /* without border, power of two */
   const GLint height = img->Height2;  /* without border, power of two */
   GLint i, j;

   COMPUTE_NEAREST_TEXEL_LOCATION(tObj->WrapS, s, width,  i);
   COMPUTE_NEAREST_TEXEL_LOCATION(tObj->WrapT, t, height, j);

   /* skip over the border, if any */
   i += img->Border;
   j += img->Border;

   if (i < 0 || i >= (GLint) img->Width || j < 0 || j >= (GLint) img->Height) {
      /* Need this test for GL_CLAMP_TO_BORDER_ARB mode */
      COPY_CHAN4(rgba, tObj->BorderColor);
   }
   else {
      (*img->FetchTexel)(img, i, j, 0, (GLvoid *) rgba);
      if (img->Format == GL_COLOR_INDEX) {
         palette_sample(ctx, tObj, rgba[0], rgba);
      }
   }
}



/*
 * Return the texture sample for coordinate (s,t) using GL_LINEAR filter.
 * New sampling code contributed by Lynn Quam <quam@ai.sri.com>.
 */
static void
sample_2d_linear(GLcontext *ctx,
                 const struct gl_texture_object *tObj,
                 const struct gl_texture_image *img,
                 GLfloat s, GLfloat t,
                 GLchan rgba[])
{
   const GLint width = img->Width2;
   const GLint height = img->Height2;
   GLint i0, j0, i1, j1;
   GLuint useBorderColor;
   GLfloat u, v;

   COMPUTE_LINEAR_TEXEL_LOCATIONS(tObj->WrapS, s, u, width,  i0, i1);
   COMPUTE_LINEAR_TEXEL_LOCATIONS(tObj->WrapT, t, v, height, j0, j1);

   useBorderColor = 0;
   if (img->Border) {
      i0 += img->Border;
      i1 += img->Border;
      j0 += img->Border;
      j1 += img->Border;
   }
   else {
      if (i0 < 0 || i0 >= width)   useBorderColor |= I0BIT;
      if (i1 < 0 || i1 >= width)   useBorderColor |= I1BIT;
      if (j0 < 0 || j0 >= height)  useBorderColor |= J0BIT;
      if (j1 < 0 || j1 >= height)  useBorderColor |= J1BIT;
   }

   {
      const GLfloat a = FRAC(u);
      const GLfloat b = FRAC(v);

#if CHAN_BITS == 32
      const GLfloat w00 = (1.0F-a) * (1.0F-b);
      const GLfloat w10 =       a  * (1.0F-b);
      const GLfloat w01 = (1.0F-a) *       b ;
      const GLfloat w11 =       a  *       b ;
#else /* CHAN_BITS == 8 || CHAN_BITS == 16 */
      /* compute sample weights in fixed point in [0,WEIGHT_SCALE] */
      const GLint w00 = IROUND_POS((1.0F-a) * (1.0F-b) * WEIGHT_SCALE);
      const GLint w10 = IROUND_POS(      a  * (1.0F-b) * WEIGHT_SCALE);
      const GLint w01 = IROUND_POS((1.0F-a) *       b  * WEIGHT_SCALE);
      const GLint w11 = IROUND_POS(      a  *       b  * WEIGHT_SCALE);
#endif

      GLchan t00[4];
      GLchan t10[4];
      GLchan t01[4];
      GLchan t11[4];

      if (useBorderColor & (I0BIT | J0BIT)) {
         COPY_CHAN4(t00, tObj->BorderColor);
      }
      else {
         (*img->FetchTexel)(img, i0, j0, 0, (GLvoid *) t00);
         if (img->Format == GL_COLOR_INDEX) {
            palette_sample(ctx, tObj, t00[0], t00);
         }
      }
      if (useBorderColor & (I1BIT | J0BIT)) {
         COPY_CHAN4(t10, tObj->BorderColor);
      }
      else {
         (*img->FetchTexel)(img, i1, j0, 0, (GLvoid *) t10);
         if (img->Format == GL_COLOR_INDEX) {
            palette_sample(ctx, tObj, t10[0], t10);
         }
      }
      if (useBorderColor & (I0BIT | J1BIT)) {
         COPY_CHAN4(t01, tObj->BorderColor);
      }
      else {
         (*img->FetchTexel)(img, i0, j1, 0, (GLvoid *) t01);
         if (img->Format == GL_COLOR_INDEX) {
            palette_sample(ctx, tObj, t01[0], t01);
         }
      }
      if (useBorderColor & (I1BIT | J1BIT)) {
         COPY_CHAN4(t11, tObj->BorderColor);
      }
      else {
         (*img->FetchTexel)(img, i1, j1, 0, (GLvoid *) t11);
         if (img->Format == GL_COLOR_INDEX) {
            palette_sample(ctx, tObj, t11[0], t11);
         }
      }
#if CHAN_BITS == 32
      rgba[0] = w00 * t00[0] + w10 * t10[0] + w01 * t01[0] + w11 * t11[0];
      rgba[1] = w00 * t00[1] + w10 * t10[1] + w01 * t01[1] + w11 * t11[1];
      rgba[2] = w00 * t00[2] + w10 * t10[2] + w01 * t01[2] + w11 * t11[2];
      rgba[3] = w00 * t00[3] + w10 * t10[3] + w01 * t01[3] + w11 * t11[3];
#else /* CHAN_BITS == 8 || CHAN_BITS == 16 */
      rgba[0] = (GLchan) ((w00 * t00[0] + w10 * t10[0] + w01 * t01[0] + w11 * t11[0]) >> WEIGHT_SHIFT);
      rgba[1] = (GLchan) ((w00 * t00[1] + w10 * t10[1] + w01 * t01[1] + w11 * t11[1]) >> WEIGHT_SHIFT);
      rgba[2] = (GLchan) ((w00 * t00[2] + w10 * t10[2] + w01 * t01[2] + w11 * t11[2]) >> WEIGHT_SHIFT);
      rgba[3] = (GLchan) ((w00 * t00[3] + w10 * t10[3] + w01 * t01[3] + w11 * t11[3]) >> WEIGHT_SHIFT);
#endif

   }

}



static void
sample_2d_nearest_mipmap_nearest(GLcontext *ctx,
                                 const struct gl_texture_object *tObj,
                                 GLfloat s, GLfloat t, GLfloat lambda,
                                 GLchan rgba[4])
{
   GLint level;
   COMPUTE_NEAREST_MIPMAP_LEVEL(tObj, lambda, level);
   sample_2d_nearest(ctx, tObj, tObj->Image[level], s, t, rgba);
}



static void
sample_2d_linear_mipmap_nearest(GLcontext *ctx,
                                const struct gl_texture_object *tObj,
                                GLfloat s, GLfloat t, GLfloat lambda,
                                GLchan rgba[4])
{
   GLint level;
   COMPUTE_NEAREST_MIPMAP_LEVEL(tObj, lambda, level);
   sample_2d_linear(ctx, tObj, tObj->Image[level], s, t, rgba);
}



static void
sample_2d_nearest_mipmap_linear(GLcontext *ctx,
                                const struct gl_texture_object *tObj,
                                GLfloat s, GLfloat t, GLfloat lambda,
                                GLchan rgba[4])
{
   GLint level;

   COMPUTE_LINEAR_MIPMAP_LEVEL(tObj, lambda, level);

   if (level >= tObj->_MaxLevel) {
      sample_2d_nearest(ctx, tObj, tObj->Image[tObj->_MaxLevel], s, t, rgba);
   }
   else {
      GLchan t0[4], t1[4];  /* texels */
      const GLfloat f = FRAC(lambda);
      sample_2d_nearest(ctx, tObj, tObj->Image[level  ], s, t, t0);
      sample_2d_nearest(ctx, tObj, tObj->Image[level+1], s, t, t1);
      rgba[RCOMP] = (GLchan) (GLint) ((1.0F-f) * t0[RCOMP] + f * t1[RCOMP]);
      rgba[GCOMP] = (GLchan) (GLint) ((1.0F-f) * t0[GCOMP] + f * t1[GCOMP]);
      rgba[BCOMP] = (GLchan) (GLint) ((1.0F-f) * t0[BCOMP] + f * t1[BCOMP]);
      rgba[ACOMP] = (GLchan) (GLint) ((1.0F-f) * t0[ACOMP] + f * t1[ACOMP]);
   }
}



static void
sample_2d_linear_mipmap_linear(GLcontext *ctx,
                               const struct gl_texture_object *tObj,
                               GLfloat s, GLfloat t, GLfloat lambda,
                               GLchan rgba[4])
{
   GLint level;

   COMPUTE_LINEAR_MIPMAP_LEVEL(tObj, lambda, level);

   if (level >= tObj->_MaxLevel) {
      sample_2d_linear(ctx, tObj, tObj->Image[tObj->_MaxLevel], s, t, rgba);
   }
   else {
      GLchan t0[4], t1[4];  /* texels */
      const GLfloat f = FRAC(lambda);
      sample_2d_linear(ctx, tObj, tObj->Image[level  ], s, t, t0);
      sample_2d_linear(ctx, tObj, tObj->Image[level+1], s, t, t1);
      rgba[RCOMP] = (GLchan) (GLint) ((1.0F-f) * t0[RCOMP] + f * t1[RCOMP]);
      rgba[GCOMP] = (GLchan) (GLint) ((1.0F-f) * t0[GCOMP] + f * t1[GCOMP]);
      rgba[BCOMP] = (GLchan) (GLint) ((1.0F-f) * t0[BCOMP] + f * t1[BCOMP]);
      rgba[ACOMP] = (GLchan) (GLint) ((1.0F-f) * t0[ACOMP] + f * t1[ACOMP]);
   }
}



static void
sample_nearest_2d( GLcontext *ctx, GLuint texUnit,
                   const struct gl_texture_object *tObj, GLuint n,
                   const GLfloat s[], const GLfloat t[],
                   const GLfloat u[], const GLfloat lambda[],
                   GLchan rgba[][4] )
{
   GLuint i;
   struct gl_texture_image *image = tObj->Image[tObj->BaseLevel];
   (void) u;
   (void) lambda;
   for (i=0;i<n;i++) {
      sample_2d_nearest(ctx, tObj, image, s[i], t[i], rgba[i]);
   }
}



static void
sample_linear_2d( GLcontext *ctx, GLuint texUnit,
                  const struct gl_texture_object *tObj, GLuint n,
                  const GLfloat s[], const GLfloat t[],
                  const GLfloat u[], const GLfloat lambda[],
                  GLchan rgba[][4] )
{
   GLuint i;
   struct gl_texture_image *image = tObj->Image[tObj->BaseLevel];
   (void) u;
   (void) lambda;
   for (i=0;i<n;i++) {
      sample_2d_linear(ctx, tObj, image, s[i], t[i], rgba[i]);
   }
}


/*
 * Given an array of (s,t) texture coordinate and lambda (level of detail)
 * values, return an array of texture sample.
 */
static void
sample_lambda_2d( GLcontext *ctx, GLuint texUnit,
                  const struct gl_texture_object *tObj,
                  GLuint n,
                  const GLfloat s[], const GLfloat t[],
                  const GLfloat u[], const GLfloat lambda[],
                  GLchan rgba[][4] )
{
   const GLfloat minMagThresh = SWRAST_CONTEXT(ctx)->_MinMagThresh[texUnit];
   GLuint i;
   (void) u;

   /* since lambda is monotonous-array use this check first */
   if (lambda[0] <= minMagThresh && lambda[n-1] <= minMagThresh) {
      /* magnification for whole span */
      switch (tObj->MagFilter) {
      case GL_NEAREST:
	 sample_nearest_2d(ctx, texUnit, tObj, n, s, t, u,
                           lambda, rgba);
         break;
      case GL_LINEAR:
	 sample_linear_2d(ctx, texUnit, tObj, n, s, t, u,
                         lambda, rgba);
         break;
      default:
         _mesa_problem(NULL, "Bad mag filter in sample_lambda_2d");
      }
   }
   else {
      for (i = 0; i < n; i++) {
         if (lambda[i] > minMagThresh) {
            /* minification */
            switch (tObj->MinFilter) {
               case GL_NEAREST:
                  sample_2d_nearest(ctx, tObj, tObj->Image[tObj->BaseLevel],
                                    s[i], t[i], rgba[i]);
                  break;
               case GL_LINEAR:
                  sample_2d_linear(ctx, tObj, tObj->Image[tObj->BaseLevel],
                                   s[i], t[i], rgba[i]);
                  break;
               case GL_NEAREST_MIPMAP_NEAREST:
                  sample_2d_nearest_mipmap_nearest(ctx, tObj, s[i], t[i],
                                                   lambda[i], rgba[i]);
                  break;
               case GL_LINEAR_MIPMAP_NEAREST:
                  sample_2d_linear_mipmap_nearest(ctx,tObj, s[i], t[i],
                                                  lambda[i], rgba[i]);
                  break;
               case GL_NEAREST_MIPMAP_LINEAR:
                  sample_2d_nearest_mipmap_linear(ctx,tObj, s[i], t[i],
                                                  lambda[i], rgba[i]);
                  break;
               case GL_LINEAR_MIPMAP_LINEAR:
                  sample_2d_linear_mipmap_linear(ctx,tObj, s[i], t[i],
                                                 lambda[i], rgba[i] );
                  break;
               default:
                  _mesa_problem(NULL, "Bad min filter in sample_2d_texture");
                  return;
            }
         }
         else {
            /* magnification */
            switch (tObj->MagFilter) {
               case GL_NEAREST:
                  sample_2d_nearest(ctx, tObj, tObj->Image[tObj->BaseLevel],
                                    s[i], t[i], rgba[i]);
                  break;
               case GL_LINEAR:
                  sample_2d_linear(ctx, tObj, tObj->Image[tObj->BaseLevel],
                                   s[i], t[i], rgba[i] );
                  break;
               default:
                  _mesa_problem(NULL, "Bad mag filter in sample_2d_texture");
            }
         }
      }
   }
}


/*
 * Optimized 2-D texture sampling:
 *    S and T wrap mode == GL_REPEAT
 *    GL_NEAREST min/mag filter
 *    No border
 *    Format = GL_RGB
 */
static void
opt_sample_rgb_2d( GLcontext *ctx, GLuint texUnit,
                   const struct gl_texture_object *tObj,
                   GLuint n, const GLfloat s[], const GLfloat t[],
                   const GLfloat u[], const GLfloat lambda[],
                   GLchan rgba[][4] )
{
   const struct gl_texture_image *img = tObj->Image[tObj->BaseLevel];
   const GLfloat width = (GLfloat) img->Width;
   const GLfloat height = (GLfloat) img->Height;
   const GLint colMask = img->Width - 1;
   const GLint rowMask = img->Height - 1;
   const GLint shift = img->WidthLog2;
   GLuint k;
   (void) u;
   (void) lambda;
   ASSERT(tObj->WrapS==GL_REPEAT);
   ASSERT(tObj->WrapT==GL_REPEAT);
   ASSERT(tObj->MinFilter==GL_NEAREST);
   ASSERT(tObj->MagFilter==GL_NEAREST);
   ASSERT(img->Border==0);
   ASSERT(img->Format==GL_RGB);

   /* NOTE: negative float->int doesn't floor, add 10000 as to work-around */
   for (k=0;k<n;k++) {
      GLint i = (GLint) ((s[k] + 10000.0) * width) & colMask;
      GLint j = (GLint) ((t[k] + 10000.0) * height) & rowMask;
      GLint pos = (j << shift) | i;
      GLchan *texel = ((GLchan *) img->Data) + pos + pos + pos;  /* pos*3 */
      rgba[k][RCOMP] = texel[0];
      rgba[k][GCOMP] = texel[1];
      rgba[k][BCOMP] = texel[2];
   }
}


/*
 * Optimized 2-D texture sampling:
 *    S and T wrap mode == GL_REPEAT
 *    GL_NEAREST min/mag filter
 *    No border
 *    Format = GL_RGBA
 */
static void
opt_sample_rgba_2d( GLcontext *ctx, GLuint texUnit,
                    const struct gl_texture_object *tObj,
                    GLuint n, const GLfloat s[], const GLfloat t[],
                    const GLfloat u[], const GLfloat lambda[],
                    GLchan rgba[][4] )
{
   const struct gl_texture_image *img = tObj->Image[tObj->BaseLevel];
   const GLfloat width = (GLfloat) img->Width;
   const GLfloat height = (GLfloat) img->Height;
   const GLint colMask = img->Width - 1;
   const GLint rowMask = img->Height - 1;
   const GLint shift = img->WidthLog2;
   GLuint k;
   (void) u;
   (void) lambda;
   ASSERT(tObj->WrapS==GL_REPEAT);
   ASSERT(tObj->WrapT==GL_REPEAT);
   ASSERT(tObj->MinFilter==GL_NEAREST);
   ASSERT(tObj->MagFilter==GL_NEAREST);
   ASSERT(img->Border==0);
   ASSERT(img->Format==GL_RGBA);

   /* NOTE: negative float->int doesn't floor, add 10000 as to work-around */
   for (k=0;k<n;k++) {
      GLint i = (GLint) ((s[k] + 10000.0) * width) & colMask;
      GLint j = (GLint) ((t[k] + 10000.0) * height) & rowMask;
      GLint pos = (j << shift) | i;
      GLchan *texel = ((GLchan *) img->Data) + (pos << 2);    /* pos*4 */
      rgba[k][RCOMP] = texel[0];
      rgba[k][GCOMP] = texel[1];
      rgba[k][BCOMP] = texel[2];
      rgba[k][ACOMP] = texel[3];
   }
}



/**********************************************************************/
/*                    3-D Texture Sampling Functions                  */
/**********************************************************************/

/*
 * Return the texture sample for coordinate (s,t,r) using GL_NEAREST filter.
 */
static void
sample_3d_nearest(GLcontext *ctx,
                  const struct gl_texture_object *tObj,
                  const struct gl_texture_image *img,
                  GLfloat s, GLfloat t, GLfloat r,
                  GLchan rgba[4])
{
   const GLint width = img->Width2;     /* without border, power of two */
   const GLint height = img->Height2;   /* without border, power of two */
   const GLint depth = img->Depth2;     /* without border, power of two */
   GLint i, j, k;

   COMPUTE_NEAREST_TEXEL_LOCATION(tObj->WrapS, s, width,  i);
   COMPUTE_NEAREST_TEXEL_LOCATION(tObj->WrapT, t, height, j);
   COMPUTE_NEAREST_TEXEL_LOCATION(tObj->WrapR, r, depth,  k);

   if (i < 0 || i >= (GLint) img->Width ||
       j < 0 || j >= (GLint) img->Height ||
       k < 0 || k >= (GLint) img->Depth) {
      /* Need this test for GL_CLAMP_TO_BORDER_ARB mode */
      COPY_CHAN4(rgba, tObj->BorderColor);
   }
   else {
      (*img->FetchTexel)(img, i, j, k, (GLvoid *) rgba);
      if (img->Format == GL_COLOR_INDEX) {
         palette_sample(ctx, tObj, rgba[0], rgba);
      }
   }
}



/*
 * Return the texture sample for coordinate (s,t,r) using GL_LINEAR filter.
 */
static void
sample_3d_linear(GLcontext *ctx,
                 const struct gl_texture_object *tObj,
                 const struct gl_texture_image *img,
                 GLfloat s, GLfloat t, GLfloat r,
                 GLchan rgba[4])
{
   const GLint width = img->Width2;
   const GLint height = img->Height2;
   const GLint depth = img->Depth2;
   GLint i0, j0, k0, i1, j1, k1;
   GLuint useBorderColor;
   GLfloat u, v, w;

   COMPUTE_LINEAR_TEXEL_LOCATIONS(tObj->WrapS, s, u, width,  i0, i1);
   COMPUTE_LINEAR_TEXEL_LOCATIONS(tObj->WrapT, t, v, height, j0, j1);
   COMPUTE_LINEAR_TEXEL_LOCATIONS(tObj->WrapR, r, w, depth,  k0, k1);

   useBorderColor = 0;
   if (img->Border) {
      i0 += img->Border;
      i1 += img->Border;
      j0 += img->Border;
      j1 += img->Border;
      k0 += img->Border;
      k1 += img->Border;
   }
   else {
      /* check if sampling texture border color */
      if (i0 < 0 || i0 >= width)   useBorderColor |= I0BIT;
      if (i1 < 0 || i1 >= width)   useBorderColor |= I1BIT;
      if (j0 < 0 || j0 >= height)  useBorderColor |= J0BIT;
      if (j1 < 0 || j1 >= height)  useBorderColor |= J1BIT;
      if (k0 < 0 || k0 >= depth)   useBorderColor |= K0BIT;
      if (k1 < 0 || k1 >= depth)   useBorderColor |= K1BIT;
   }

   {
      const GLfloat a = FRAC(u);
      const GLfloat b = FRAC(v);
      const GLfloat c = FRAC(w);

#if CHAN_BITS == 32
      /* compute sample weights in fixed point in [0,WEIGHT_SCALE] */
      GLfloat w000 = (1.0F-a) * (1.0F-b) * (1.0F-c);
      GLfloat w100 =       a  * (1.0F-b) * (1.0F-c);
      GLfloat w010 = (1.0F-a) *       b  * (1.0F-c);
      GLfloat w110 =       a  *       b  * (1.0F-c);
      GLfloat w001 = (1.0F-a) * (1.0F-b) *       c ;
      GLfloat w101 =       a  * (1.0F-b) *       c ;
      GLfloat w011 = (1.0F-a) *       b  *       c ;
      GLfloat w111 =       a  *       b  *       c ;
#else /* CHAN_BITS == 8 || CHAN_BITS == 16 */
      /* compute sample weights in fixed point in [0,WEIGHT_SCALE] */
      GLint w000 = IROUND_POS((1.0F-a) * (1.0F-b) * (1.0F-c) * WEIGHT_SCALE);
      GLint w100 = IROUND_POS(      a  * (1.0F-b) * (1.0F-c) * WEIGHT_SCALE);
      GLint w010 = IROUND_POS((1.0F-a) *       b  * (1.0F-c) * WEIGHT_SCALE);
      GLint w110 = IROUND_POS(      a  *       b  * (1.0F-c) * WEIGHT_SCALE);
      GLint w001 = IROUND_POS((1.0F-a) * (1.0F-b) *       c  * WEIGHT_SCALE);
      GLint w101 = IROUND_POS(      a  * (1.0F-b) *       c  * WEIGHT_SCALE);
      GLint w011 = IROUND_POS((1.0F-a) *       b  *       c  * WEIGHT_SCALE);
      GLint w111 = IROUND_POS(      a  *       b  *       c  * WEIGHT_SCALE);
#endif

      GLchan t000[4], t010[4], t001[4], t011[4];
      GLchan t100[4], t110[4], t101[4], t111[4];

      if (useBorderColor & (I0BIT | J0BIT | K0BIT)) {
         COPY_CHAN4(t000, tObj->BorderColor);
      }
      else {
         (*img->FetchTexel)(img, i0, j0, k0, (GLvoid *) t000);
         if (img->Format == GL_COLOR_INDEX) {
            palette_sample(ctx, tObj, t000[0], t000);
         }
      }
      if (useBorderColor & (I1BIT | J0BIT | K0BIT)) {
         COPY_CHAN4(t100, tObj->BorderColor);
      }
      else {
         (*img->FetchTexel)(img, i1, j0, k0, (GLvoid *) t100);
         if (img->Format == GL_COLOR_INDEX) {
            palette_sample(ctx, tObj, t100[0], t100);
         }
      }
      if (useBorderColor & (I0BIT | J1BIT | K0BIT)) {
         COPY_CHAN4(t010, tObj->BorderColor);
      }
      else {
         (*img->FetchTexel)(img, i0, j1, k0, (GLvoid *) t010);
         if (img->Format == GL_COLOR_INDEX) {
            palette_sample(ctx, tObj, t010[0], t010);
         }
      }
      if (useBorderColor & (I1BIT | J1BIT | K0BIT)) {
         COPY_CHAN4(t110, tObj->BorderColor);
      }
      else {
         (*img->FetchTexel)(img, i1, j1, k0, (GLvoid *) t110);
         if (img->Format == GL_COLOR_INDEX) {
            palette_sample(ctx, tObj, t110[0], t110);
         }
      }

      if (useBorderColor & (I0BIT | J0BIT | K1BIT)) {
         COPY_CHAN4(t001, tObj->BorderColor);
      }
      else {
         (*img->FetchTexel)(img, i0, j0, k1, (GLvoid *) t001);
         if (img->Format == GL_COLOR_INDEX) {
            palette_sample(ctx, tObj, t001[0], t001);
         }
      }
      if (useBorderColor & (I1BIT | J0BIT | K1BIT)) {
         COPY_CHAN4(t101, tObj->BorderColor);
      }
      else {
         (*img->FetchTexel)(img, i1, j0, k1, (GLvoid *) t101);
         if (img->Format == GL_COLOR_INDEX) {
            palette_sample(ctx, tObj, t101[0], t101);
         }
      }
      if (useBorderColor & (I0BIT | J1BIT | K1BIT)) {
         COPY_CHAN4(t011, tObj->BorderColor);
      }
      else {
         (*img->FetchTexel)(img, i0, j1, k1, (GLvoid *) t011);
         if (img->Format == GL_COLOR_INDEX) {
            palette_sample(ctx, tObj, t011[0], t011);
         }
      }
      if (useBorderColor & (I1BIT | J1BIT | K1BIT)) {
         COPY_CHAN4(t111, tObj->BorderColor);
      }
      else {
         (*img->FetchTexel)(img, i1, j1, k1, (GLvoid *) t111);
         if (img->Format == GL_COLOR_INDEX) {
            palette_sample(ctx, tObj, t111[0], t111);
         }
      }

#if CHAN_BITS == 32
      rgba[0] = w000*t000[0] + w010*t010[0] + w001*t001[0] + w011*t011[0] +
                w100*t100[0] + w110*t110[0] + w101*t101[0] + w111*t111[0];
      rgba[1] = w000*t000[1] + w010*t010[1] + w001*t001[1] + w011*t011[1] +
                w100*t100[1] + w110*t110[1] + w101*t101[1] + w111*t111[1];
      rgba[2] = w000*t000[2] + w010*t010[2] + w001*t001[2] + w011*t011[2] +
                w100*t100[2] + w110*t110[2] + w101*t101[2] + w111*t111[2];
      rgba[3] = w000*t000[3] + w010*t010[3] + w001*t001[3] + w011*t011[3] +
                w100*t100[3] + w110*t110[3] + w101*t101[3] + w111*t111[3];
#else /* CHAN_BITS == 8 || CHAN_BITS == 16 */
      rgba[0] = (GLchan) (
                 (w000*t000[0] + w010*t010[0] + w001*t001[0] + w011*t011[0] +
                  w100*t100[0] + w110*t110[0] + w101*t101[0] + w111*t111[0] )
                 >> WEIGHT_SHIFT);
      rgba[1] = (GLchan) (
                 (w000*t000[1] + w010*t010[1] + w001*t001[1] + w011*t011[1] +
                  w100*t100[1] + w110*t110[1] + w101*t101[1] + w111*t111[1] )
                 >> WEIGHT_SHIFT);
      rgba[2] = (GLchan) (
                 (w000*t000[2] + w010*t010[2] + w001*t001[2] + w011*t011[2] +
                  w100*t100[2] + w110*t110[2] + w101*t101[2] + w111*t111[2] )
                 >> WEIGHT_SHIFT);
      rgba[3] = (GLchan) (
                 (w000*t000[3] + w010*t010[3] + w001*t001[3] + w011*t011[3] +
                  w100*t100[3] + w110*t110[3] + w101*t101[3] + w111*t111[3] )
                 >> WEIGHT_SHIFT);
#endif

   }
}



static void
sample_3d_nearest_mipmap_nearest(GLcontext *ctx,
                                 const struct gl_texture_object *tObj,
                                 GLfloat s, GLfloat t, GLfloat r,
                                 GLfloat lambda, GLchan rgba[4] )
{
   GLint level;
   COMPUTE_NEAREST_MIPMAP_LEVEL(tObj, lambda, level);
   sample_3d_nearest(ctx, tObj, tObj->Image[level], s, t, r, rgba);
}


static void
sample_3d_linear_mipmap_nearest(GLcontext *ctx,
                                const struct gl_texture_object *tObj,
                                GLfloat s, GLfloat t, GLfloat r,
                                GLfloat lambda, GLchan rgba[4])
{
   GLint level;
   COMPUTE_NEAREST_MIPMAP_LEVEL(tObj, lambda, level);
   sample_3d_linear(ctx, tObj, tObj->Image[level], s, t, r, rgba);
}


static void
sample_3d_nearest_mipmap_linear(GLcontext *ctx,
                                const struct gl_texture_object *tObj,
                                GLfloat s, GLfloat t, GLfloat r,
                                GLfloat lambda, GLchan rgba[4])
{
   GLint level;

   COMPUTE_LINEAR_MIPMAP_LEVEL(tObj, lambda, level);

   if (level >= tObj->_MaxLevel) {
      sample_3d_nearest(ctx, tObj, tObj->Image[tObj->_MaxLevel],
                        s, t, r, rgba);
   }
   else {
      GLchan t0[4], t1[4];  /* texels */
      const GLfloat f = FRAC(lambda);
      sample_3d_nearest(ctx, tObj, tObj->Image[level  ], s, t, r, t0);
      sample_3d_nearest(ctx, tObj, tObj->Image[level+1], s, t, r, t1);
      rgba[RCOMP] = (GLchan) (GLint) ((1.0F-f) * t0[RCOMP] + f * t1[RCOMP]);
      rgba[GCOMP] = (GLchan) (GLint) ((1.0F-f) * t0[GCOMP] + f * t1[GCOMP]);
      rgba[BCOMP] = (GLchan) (GLint) ((1.0F-f) * t0[BCOMP] + f * t1[BCOMP]);
      rgba[ACOMP] = (GLchan) (GLint) ((1.0F-f) * t0[ACOMP] + f * t1[ACOMP]);
   }
}


static void
sample_3d_linear_mipmap_linear(GLcontext *ctx,
                               const struct gl_texture_object *tObj,
                               GLfloat s, GLfloat t, GLfloat r,
                               GLfloat lambda, GLchan rgba[4] )
{
   GLint level;

   COMPUTE_LINEAR_MIPMAP_LEVEL(tObj, lambda, level);

   if (level >= tObj->_MaxLevel) {
      sample_3d_linear(ctx, tObj, tObj->Image[tObj->_MaxLevel], s, t, r, rgba);
   }
   else {
      GLchan t0[4], t1[4];  /* texels */
      const GLfloat f = FRAC(lambda);
      sample_3d_linear(ctx, tObj, tObj->Image[level  ], s, t, r, t0);
      sample_3d_linear(ctx, tObj, tObj->Image[level+1], s, t, r, t1);
      rgba[RCOMP] = (GLchan) (GLint) ((1.0F-f) * t0[RCOMP] + f * t1[RCOMP]);
      rgba[GCOMP] = (GLchan) (GLint) ((1.0F-f) * t0[GCOMP] + f * t1[GCOMP]);
      rgba[BCOMP] = (GLchan) (GLint) ((1.0F-f) * t0[BCOMP] + f * t1[BCOMP]);
      rgba[ACOMP] = (GLchan) (GLint) ((1.0F-f) * t0[ACOMP] + f * t1[ACOMP]);
   }
}


static void
sample_nearest_3d(GLcontext *ctx, GLuint texUnit,
                  const struct gl_texture_object *tObj, GLuint n,
                  const GLfloat s[], const GLfloat t[],
                  const GLfloat u[], const GLfloat lambda[],
                  GLchan rgba[][4])
{
   GLuint i;
   struct gl_texture_image *image = tObj->Image[tObj->BaseLevel];
   (void) lambda;
   for (i=0;i<n;i++) {
      sample_3d_nearest(ctx, tObj, image, s[i], t[i], u[i], rgba[i]);
   }
}



static void
sample_linear_3d( GLcontext *ctx, GLuint texUnit,
                  const struct gl_texture_object *tObj, GLuint n,
                  const GLfloat s[], const GLfloat t[],
                  const GLfloat u[], const GLfloat lambda[],
                  GLchan rgba[][4] )
{
   GLuint i;
   struct gl_texture_image *image = tObj->Image[tObj->BaseLevel];
   (void) lambda;
   for (i=0;i<n;i++) {
      sample_3d_linear(ctx, tObj, image, s[i], t[i], u[i], rgba[i]);
   }
}


/*
 * Given an (s,t,r) texture coordinate and lambda (level of detail) value,
 * return a texture sample.
 */
static void
sample_lambda_3d( GLcontext *ctx, GLuint texUnit,
                  const struct gl_texture_object *tObj, GLuint n,
                  const GLfloat s[], const GLfloat t[],
                  const GLfloat u[], const GLfloat lambda[],
                  GLchan rgba[][4] )
{
   GLuint i;
   GLfloat MinMagThresh = SWRAST_CONTEXT(ctx)->_MinMagThresh[texUnit];

   for (i=0;i<n;i++) {

      if (lambda[i] > MinMagThresh) {
         /* minification */
         switch (tObj->MinFilter) {
            case GL_NEAREST:
               sample_3d_nearest(ctx, tObj, tObj->Image[tObj->BaseLevel],
                                 s[i], t[i], u[i], rgba[i]);
               break;
            case GL_LINEAR:
               sample_3d_linear(ctx, tObj, tObj->Image[tObj->BaseLevel],
                                s[i], t[i], u[i], rgba[i]);
               break;
            case GL_NEAREST_MIPMAP_NEAREST:
               sample_3d_nearest_mipmap_nearest(ctx, tObj, s[i], t[i], u[i],
                                                lambda[i], rgba[i]);
               break;
            case GL_LINEAR_MIPMAP_NEAREST:
               sample_3d_linear_mipmap_nearest(ctx, tObj, s[i], t[i], u[i],
                                               lambda[i], rgba[i]);
               break;
            case GL_NEAREST_MIPMAP_LINEAR:
               sample_3d_nearest_mipmap_linear(ctx, tObj, s[i], t[i], u[i],
                                               lambda[i], rgba[i]);
               break;
            case GL_LINEAR_MIPMAP_LINEAR:
               sample_3d_linear_mipmap_linear(ctx, tObj, s[i], t[i], u[i],
                                              lambda[i], rgba[i]);
               break;
            default:
               _mesa_problem(NULL, "Bad min filterin sample_3d_texture");
         }
      }
      else {
         /* magnification */
         switch (tObj->MagFilter) {
            case GL_NEAREST:
               sample_3d_nearest(ctx, tObj, tObj->Image[tObj->BaseLevel],
                                 s[i], t[i], u[i], rgba[i]);
               break;
            case GL_LINEAR:
               sample_3d_linear(ctx, tObj, tObj->Image[tObj->BaseLevel],
                                s[i], t[i], u[i], rgba[i]);
               break;
            default:
               _mesa_problem(NULL, "Bad mag filter in sample_3d_texture");
         }
      }
   }
}


/**********************************************************************/
/*                Texture Cube Map Sampling Functions                 */
/**********************************************************************/

/*
 * Choose one of six sides of a texture cube map given the texture
 * coord (rx,ry,rz).  Return pointer to corresponding array of texture
 * images.
 */
static const struct gl_texture_image **
choose_cube_face(const struct gl_texture_object *texObj,
                 GLfloat rx, GLfloat ry, GLfloat rz,
                 GLfloat *newS, GLfloat *newT)
{
/*
      major axis
      direction     target                             sc     tc    ma
      ----------    -------------------------------    ---    ---   ---
       +rx          TEXTURE_CUBE_MAP_POSITIVE_X_EXT    -rz    -ry   rx
       -rx          TEXTURE_CUBE_MAP_NEGATIVE_X_EXT    +rz    -ry   rx
       +ry          TEXTURE_CUBE_MAP_POSITIVE_Y_EXT    +rx    +rz   ry
       -ry          TEXTURE_CUBE_MAP_NEGATIVE_Y_EXT    +rx    -rz   ry
       +rz          TEXTURE_CUBE_MAP_POSITIVE_Z_EXT    +rx    -ry   rz
       -rz          TEXTURE_CUBE_MAP_NEGATIVE_Z_EXT    -rx    -ry   rz
*/
   const struct gl_texture_image **imgArray;
   const GLfloat arx = ABSF(rx),   ary = ABSF(ry),   arz = ABSF(rz);
   GLfloat sc, tc, ma;

   if (arx > ary && arx > arz) {
      if (rx >= 0.0F) {
         imgArray = (const struct gl_texture_image **) texObj->Image;
         sc = -rz;
         tc = -ry;
         ma = arx;
      }
      else {
         imgArray = (const struct gl_texture_image **) texObj->NegX;
         sc = rz;
         tc = -ry;
         ma = arx;
      }
   }
   else if (ary > arx && ary > arz) {
      if (ry >= 0.0F) {
         imgArray = (const struct gl_texture_image **) texObj->PosY;
         sc = rx;
         tc = rz;
         ma = ary;
      }
      else {
         imgArray = (const struct gl_texture_image **) texObj->NegY;
         sc = rx;
         tc = -rz;
         ma = ary;
      }
   }
   else {
      if (rz > 0.0F) {
         imgArray = (const struct gl_texture_image **) texObj->PosZ;
         sc = rx;
         tc = -ry;
         ma = arz;
      }
      else {
         imgArray = (const struct gl_texture_image **) texObj->NegZ;
         sc = -rx;
         tc = -ry;
         ma = arz;
      }
   }

   *newS = ( sc / ma + 1.0F ) * 0.5F;
   *newT = ( tc / ma + 1.0F ) * 0.5F;
   return imgArray;
}


static void
sample_nearest_cube(GLcontext *ctx, GLuint texUnit,
		    const struct gl_texture_object *tObj, GLuint n,
                    const GLfloat s[], const GLfloat t[],
                    const GLfloat u[], const GLfloat lambda[],
                    GLchan rgba[][4])
{
   GLuint i;
   (void) lambda;
   for (i = 0; i < n; i++) {
      const struct gl_texture_image **images;
      GLfloat newS, newT;
      images = choose_cube_face(tObj, s[i], t[i], u[i], &newS, &newT);
      sample_2d_nearest(ctx, tObj, images[tObj->BaseLevel],
                        newS, newT, rgba[i]);
   }
}


static void
sample_linear_cube(GLcontext *ctx, GLuint texUnit,
		   const struct gl_texture_object *tObj, GLuint n,
                   const GLfloat s[], const GLfloat t[],
                   const GLfloat u[], const GLfloat lambda[],
                   GLchan rgba[][4])
{
   GLuint i;
   (void) lambda;
   for (i = 0; i < n; i++) {
      const struct gl_texture_image **images;
      GLfloat newS, newT;
      images = choose_cube_face(tObj, s[i], t[i], u[i], &newS, &newT);
      sample_2d_linear(ctx, tObj, images[tObj->BaseLevel],
                       newS, newT, rgba[i]);
   }
}


static void
sample_cube_nearest_mipmap_nearest(GLcontext *ctx,
                                   const struct gl_texture_object *tObj,
                                   GLfloat s, GLfloat t, GLfloat u,
                                   GLfloat lambda, GLchan rgba[4])
{
   const struct gl_texture_image **images;
   GLfloat newS, newT;
   GLint level;

   COMPUTE_NEAREST_MIPMAP_LEVEL(tObj, lambda, level);

   images = choose_cube_face(tObj, s, t, u, &newS, &newT);
   sample_2d_nearest(ctx, tObj, images[level], newS, newT, rgba);
}


static void
sample_cube_linear_mipmap_nearest(GLcontext *ctx,
                                  const struct gl_texture_object *tObj,
                                  GLfloat s, GLfloat t, GLfloat u,
                                  GLfloat lambda, GLchan rgba[4])
{
   const struct gl_texture_image **images;
   GLfloat newS, newT;
   GLint level;

   COMPUTE_NEAREST_MIPMAP_LEVEL(tObj, lambda, level);

   images = choose_cube_face(tObj, s, t, u, &newS, &newT);
   sample_2d_linear(ctx, tObj, images[level], newS, newT, rgba);
}


static void
sample_cube_nearest_mipmap_linear(GLcontext *ctx,
                                  const struct gl_texture_object *tObj,
                                  GLfloat s, GLfloat t, GLfloat u,
                                  GLfloat lambda, GLchan rgba[4])
{
   const struct gl_texture_image **images;
   GLfloat newS, newT;
   GLint level;

   COMPUTE_LINEAR_MIPMAP_LEVEL(tObj, lambda, level);

   images = choose_cube_face(tObj, s, t, u, &newS, &newT);

   if (level >= tObj->_MaxLevel) {
      sample_2d_nearest(ctx, tObj, images[tObj->_MaxLevel], newS, newT, rgba);
   }
   else {
      GLchan t0[4], t1[4];  /* texels */
      const GLfloat f = FRAC(lambda);
      sample_2d_nearest(ctx, tObj, images[level  ], newS, newT, t0);
      sample_2d_nearest(ctx, tObj, images[level+1], newS, newT, t1);
      rgba[RCOMP] = (GLchan) (GLint) ((1.0F-f) * t0[RCOMP] + f * t1[RCOMP]);
      rgba[GCOMP] = (GLchan) (GLint) ((1.0F-f) * t0[GCOMP] + f * t1[GCOMP]);
      rgba[BCOMP] = (GLchan) (GLint) ((1.0F-f) * t0[BCOMP] + f * t1[BCOMP]);
      rgba[ACOMP] = (GLchan) (GLint) ((1.0F-f) * t0[ACOMP] + f * t1[ACOMP]);
   }
}


static void
sample_cube_linear_mipmap_linear(GLcontext *ctx,
                                 const struct gl_texture_object *tObj,
                                 GLfloat s, GLfloat t, GLfloat u,
                                 GLfloat lambda, GLchan rgba[4])
{
   const struct gl_texture_image **images;
   GLfloat newS, newT;
   GLint level;

   COMPUTE_LINEAR_MIPMAP_LEVEL(tObj, lambda, level);

   images = choose_cube_face(tObj, s, t, u, &newS, &newT);

   if (level >= tObj->_MaxLevel) {
      sample_2d_linear(ctx, tObj, images[tObj->_MaxLevel], newS, newT, rgba);
   }
   else {
      GLchan t0[4], t1[4];
      const GLfloat f = FRAC(lambda);
      sample_2d_linear(ctx, tObj, images[level  ], newS, newT, t0);
      sample_2d_linear(ctx, tObj, images[level+1], newS, newT, t1);
      rgba[RCOMP] = (GLchan) (GLint) ((1.0F-f) * t0[RCOMP] + f * t1[RCOMP]);
      rgba[GCOMP] = (GLchan) (GLint) ((1.0F-f) * t0[GCOMP] + f * t1[GCOMP]);
      rgba[BCOMP] = (GLchan) (GLint) ((1.0F-f) * t0[BCOMP] + f * t1[BCOMP]);
      rgba[ACOMP] = (GLchan) (GLint) ((1.0F-f) * t0[ACOMP] + f * t1[ACOMP]);
   }
}


static void
sample_lambda_cube( GLcontext *ctx, GLuint texUnit,
		    const struct gl_texture_object *tObj, GLuint n,
		    const GLfloat s[], const GLfloat t[],
		    const GLfloat u[], const GLfloat lambda[],
		    GLchan rgba[][4])
{
   GLfloat MinMagThresh = SWRAST_CONTEXT(ctx)->_MinMagThresh[texUnit];
   GLuint i;

   for (i = 0; i < n; i++) {
      if (lambda[i] > MinMagThresh) {
         /* minification */
         switch (tObj->MinFilter) {
            case GL_NEAREST:
               {
                  const struct gl_texture_image **images;
                  GLfloat newS, newT;
                  images = choose_cube_face(tObj, s[i], t[i], u[i],
                                            &newS, &newT);
                  sample_2d_nearest(ctx, tObj, images[tObj->BaseLevel],
                                    newS, newT, rgba[i]);
               }
               break;
            case GL_LINEAR:
               {
                  const struct gl_texture_image **images;
                  GLfloat newS, newT;
                  images = choose_cube_face(tObj, s[i], t[i], u[i],
                                            &newS, &newT);
                  sample_2d_linear(ctx, tObj, images[tObj->BaseLevel],
                                   newS, newT, rgba[i]);
               }
               break;
            case GL_NEAREST_MIPMAP_NEAREST:
               sample_cube_nearest_mipmap_nearest(ctx, tObj, s[i], t[i], u[i],
                                                  lambda[i], rgba[i]);
               break;
            case GL_LINEAR_MIPMAP_NEAREST:
               sample_cube_linear_mipmap_nearest(ctx, tObj, s[i], t[i], u[i],
                                                 lambda[i], rgba[i]);
               break;
            case GL_NEAREST_MIPMAP_LINEAR:
               sample_cube_nearest_mipmap_linear(ctx, tObj, s[i], t[i], u[i],
                                                 lambda[i], rgba[i]);
               break;
            case GL_LINEAR_MIPMAP_LINEAR:
               sample_cube_linear_mipmap_linear(ctx, tObj, s[i], t[i], u[i],
                                                lambda[i], rgba[i]);
               break;
            default:
               _mesa_problem(NULL, "Bad min filter in sample_lambda_cube");
         }
      }
      else {
         /* magnification */
         const struct gl_texture_image **images;
         GLfloat newS, newT;
         images = choose_cube_face(tObj, s[i], t[i], u[i],
                                   &newS, &newT);
         switch (tObj->MagFilter) {
            case GL_NEAREST:
               sample_2d_nearest(ctx, tObj, images[tObj->BaseLevel],
                                 newS, newT, rgba[i]);
               break;
            case GL_LINEAR:
               sample_2d_linear(ctx, tObj, images[tObj->BaseLevel],
                                newS, newT, rgba[i]);
               break;
            default:
               _mesa_problem(NULL, "Bad mag filter in sample_lambda_cube");
         }
      }
   }
}

static void
null_sample_func( GLcontext *ctx, GLuint texUnit,
		  const struct gl_texture_object *tObj, GLuint n,
		  const GLfloat s[], const GLfloat t[],
		  const GLfloat u[], const GLfloat lambda[],
		  GLchan rgba[][4])
{
}

/**********************************************************************/
/*                       Texture Sampling Setup                       */
/**********************************************************************/


/*
 * Setup the texture sampling function for this texture object.
 */
void
_swrast_choose_texture_sample_func( GLcontext *ctx, GLuint texUnit,
				    const struct gl_texture_object *t )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

  if (!t->Complete) {
     swrast->TextureSample[texUnit] = null_sample_func;
   }
   else {
      GLboolean needLambda = (GLboolean) (t->MinFilter != t->MagFilter);

      if (needLambda) {
         /* Compute min/mag filter threshold */
         if (t->MagFilter == GL_LINEAR
             && (t->MinFilter == GL_NEAREST_MIPMAP_NEAREST ||
                 t->MinFilter == GL_NEAREST_MIPMAP_LINEAR)) {
            swrast->_MinMagThresh[texUnit] = 0.5F;
         }
         else {
            swrast->_MinMagThresh[texUnit] = 0.0F;
         }
      }

      switch (t->Dimensions) {
         case 1:
            if (needLambda) {
               swrast->TextureSample[texUnit] = sample_lambda_1d;
            }
            else if (t->MinFilter==GL_LINEAR) {
               swrast->TextureSample[texUnit] = sample_linear_1d;
            }
            else {
               ASSERT(t->MinFilter==GL_NEAREST);
               swrast->TextureSample[texUnit] = sample_nearest_1d;
            }
            break;
         case 2:
            if (needLambda) {
               swrast->TextureSample[texUnit] = sample_lambda_2d;
            }
            else if (t->MinFilter==GL_LINEAR) {
               swrast->TextureSample[texUnit] = sample_linear_2d;
            }
            else {
               GLint baseLevel = t->BaseLevel;
               ASSERT(t->MinFilter==GL_NEAREST);
               if (t->WrapS == GL_REPEAT &&
                   t->WrapT == GL_REPEAT &&
                   t->Image[baseLevel]->Border == 0 &&
                   t->Image[baseLevel]->TexFormat->MesaFormat == MESA_FORMAT_RGB) {
                  swrast->TextureSample[texUnit] = opt_sample_rgb_2d;
               }
               else if (t->WrapS == GL_REPEAT &&
                        t->WrapT == GL_REPEAT &&
                        t->Image[baseLevel]->Border == 0 &&
                        t->Image[baseLevel]->TexFormat->MesaFormat == MESA_FORMAT_RGBA) {
                  swrast->TextureSample[texUnit] = opt_sample_rgba_2d;
               }
               else
                  swrast->TextureSample[texUnit] = sample_nearest_2d;
            }
            break;
         case 3:
            if (needLambda) {
               swrast->TextureSample[texUnit] = sample_lambda_3d;
            }
            else if (t->MinFilter==GL_LINEAR) {
               swrast->TextureSample[texUnit] = sample_linear_3d;
            }
            else {
               ASSERT(t->MinFilter==GL_NEAREST);
               swrast->TextureSample[texUnit] = sample_nearest_3d;
            }
            break;
         case 6: /* cube map */
            if (needLambda) {
               swrast->TextureSample[texUnit] = sample_lambda_cube;
            }
            else if (t->MinFilter==GL_LINEAR) {
               swrast->TextureSample[texUnit] = sample_linear_cube;
            }
            else {
               ASSERT(t->MinFilter==GL_NEAREST);
               swrast->TextureSample[texUnit] = sample_nearest_cube;
            }
            break;
         default:
            _mesa_problem(NULL, "invalid dimensions in _mesa_set_texture_sampler");
      }
   }
}


#define PROD(A,B)   ( (GLuint)(A) * ((GLuint)(B)+1) )
#define S_PROD(A,B) ( (GLint)(A) * ((GLint)(B)+1) )

static INLINE void
texture_combine(const GLcontext *ctx,
                const struct gl_texture_unit *textureUnit,
                GLuint n,
                CONST GLchan (*primary_rgba)[4],
                CONST GLchan (*texel)[4],
                GLchan (*rgba)[4])
{
   const GLchan (*argRGB [3])[4];
   const GLchan (*argA [3])[4];
   GLuint i, j;
   const GLuint RGBshift = textureUnit->CombineScaleShiftRGB;
   const GLuint Ashift   = textureUnit->CombineScaleShiftA;
   DEFMNARRAY(GLchan, ccolor, 3, 3 * MAX_WIDTH, 4);  /* mac 32k limitation */
   CHECKARRAY(ccolor, return);  /* mac 32k limitation */

   ASSERT(ctx->Extensions.EXT_texture_env_combine ||
          ctx->Extensions.ARB_texture_env_combine);

   /*
    * Do operand setup for up to 3 operands.  Loop over the terms.
    */
   for (j = 0; j < 3; j++) {
      switch (textureUnit->CombineSourceA[j]) {
         case GL_TEXTURE:
            argA[j] = texel;
            break;
         case GL_PRIMARY_COLOR_EXT:
            argA[j] = primary_rgba;
            break;
         case GL_PREVIOUS_EXT:
            argA[j] = (const GLchan (*)[4]) rgba;
            break;
         case GL_CONSTANT_EXT:
            {
               GLchan alpha, (*c)[4] = ccolor[j];
               UNCLAMPED_FLOAT_TO_CHAN(alpha, textureUnit->EnvColor[3]);
               for (i = 0; i < n; i++)
                  c[i][ACOMP] = alpha;
               argA[j] = (const GLchan (*)[4]) ccolor[j];
            }
            break;
         default:
            _mesa_problem(NULL, "invalid combine source");
      }

      switch (textureUnit->CombineSourceRGB[j]) {
         case GL_TEXTURE:
            argRGB[j] = texel;
            break;
         case GL_PRIMARY_COLOR_EXT:
            argRGB[j] = primary_rgba;
            break;
         case GL_PREVIOUS_EXT:
            argRGB[j] = (const GLchan (*)[4]) rgba;
            break;
         case GL_CONSTANT_EXT:
            {
               GLchan (*c)[4] = ccolor[j];
               GLchan red, green, blue, alpha;
               UNCLAMPED_FLOAT_TO_CHAN(red,   textureUnit->EnvColor[0]);
               UNCLAMPED_FLOAT_TO_CHAN(green, textureUnit->EnvColor[1]);
               UNCLAMPED_FLOAT_TO_CHAN(blue,  textureUnit->EnvColor[2]);
               UNCLAMPED_FLOAT_TO_CHAN(alpha, textureUnit->EnvColor[3]);
               for (i = 0; i < n; i++) {
                  c[i][RCOMP] = red;
                  c[i][GCOMP] = green;
                  c[i][BCOMP] = blue;
                  c[i][ACOMP] = alpha;
               }
               argRGB[j] = (const GLchan (*)[4]) ccolor[j];
            }
            break;
         default:
            _mesa_problem(NULL, "invalid combine source");
      }

      if (textureUnit->CombineOperandRGB[j] != GL_SRC_COLOR) {
         const GLchan (*src)[4] = argRGB[j];
         GLchan (*dst)[4] = ccolor[j];

         /* point to new arg[j] storage */
         argRGB[j] = (const GLchan (*)[4]) ccolor[j];

         if (textureUnit->CombineOperandRGB[j] == GL_ONE_MINUS_SRC_COLOR) {
            for (i = 0; i < n; i++) {
               dst[i][RCOMP] = CHAN_MAX - src[i][RCOMP];
               dst[i][GCOMP] = CHAN_MAX - src[i][GCOMP];
               dst[i][BCOMP] = CHAN_MAX - src[i][BCOMP];
            }
         }
         else if (textureUnit->CombineOperandRGB[j] == GL_SRC_ALPHA) {
            for (i = 0; i < n; i++) {
               dst[i][RCOMP] = src[i][ACOMP];
               dst[i][GCOMP] = src[i][ACOMP];
               dst[i][BCOMP] = src[i][ACOMP];
            }
         }
         else {
            ASSERT(textureUnit->CombineOperandRGB[j] ==GL_ONE_MINUS_SRC_ALPHA);
            for (i = 0; i < n; i++) {
               dst[i][RCOMP] = CHAN_MAX - src[i][ACOMP];
               dst[i][GCOMP] = CHAN_MAX - src[i][ACOMP];
               dst[i][BCOMP] = CHAN_MAX - src[i][ACOMP];
            }
         }
      }

      if (textureUnit->CombineOperandA[j] == GL_ONE_MINUS_SRC_ALPHA) {
         const GLchan (*src)[4] = argA[j];
         GLchan (*dst)[4] = ccolor[j];
         argA[j] = (const GLchan (*)[4]) ccolor[j];
         for (i = 0; i < n; i++) {
            dst[i][ACOMP] = CHAN_MAX - src[i][ACOMP];
         }
      }

      if (textureUnit->CombineModeRGB == GL_REPLACE &&
          textureUnit->CombineModeA == GL_REPLACE) {
         break;      /*  done, we need only arg0  */
      }

      if (j == 1 &&
          textureUnit->CombineModeRGB != GL_INTERPOLATE_EXT &&
          textureUnit->CombineModeA != GL_INTERPOLATE_EXT) {
         break;      /*  arg0 and arg1 are done. we don't need arg2. */
      }
   }

   /*
    * Do the texture combine.
    */
   switch (textureUnit->CombineModeRGB) {
      case GL_REPLACE:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argRGB[0];
            if (RGBshift) {
               for (i = 0; i < n; i++) {
                  GLuint r = (GLuint) arg0[i][RCOMP] << RGBshift;
                  GLuint g = (GLuint) arg0[i][GCOMP] << RGBshift;
                  GLuint b = (GLuint) arg0[i][BCOMP] << RGBshift;
                  rgba[i][RCOMP] = MIN2(r, CHAN_MAX);
                  rgba[i][GCOMP] = MIN2(g, CHAN_MAX);
                  rgba[i][BCOMP] = MIN2(b, CHAN_MAX);
               }
            }
            else {
               for (i = 0; i < n; i++) {
                  rgba[i][RCOMP] = arg0[i][RCOMP];
                  rgba[i][GCOMP] = arg0[i][GCOMP];
                  rgba[i][BCOMP] = arg0[i][BCOMP];
               }
            }
         }
         break;
      case GL_MODULATE:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argRGB[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argRGB[1];
            const GLint shift = 8 - RGBshift;
            for (i = 0; i < n; i++) {
               GLuint r = PROD(arg0[i][0], arg1[i][RCOMP]) >> shift;
               GLuint g = PROD(arg0[i][1], arg1[i][GCOMP]) >> shift;
               GLuint b = PROD(arg0[i][2], arg1[i][BCOMP]) >> shift;
               rgba[i][RCOMP] = (GLchan) MIN2(r, CHAN_MAX);
               rgba[i][GCOMP] = (GLchan) MIN2(g, CHAN_MAX);
               rgba[i][BCOMP] = (GLchan) MIN2(b, CHAN_MAX);
            }
         }
         break;
      case GL_ADD:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argRGB[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argRGB[1];
            for (i = 0; i < n; i++) {
               GLint r = ((GLint) arg0[i][RCOMP] + (GLint) arg1[i][RCOMP]) << RGBshift;
               GLint g = ((GLint) arg0[i][GCOMP] + (GLint) arg1[i][GCOMP]) << RGBshift;
               GLint b = ((GLint) arg0[i][BCOMP] + (GLint) arg1[i][BCOMP]) << RGBshift;
               rgba[i][RCOMP] = (GLchan) MIN2(r, CHAN_MAX);
               rgba[i][GCOMP] = (GLchan) MIN2(g, CHAN_MAX);
               rgba[i][BCOMP] = (GLchan) MIN2(b, CHAN_MAX);
            }
         }
         break;
      case GL_ADD_SIGNED_EXT:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argRGB[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argRGB[1];
            for (i = 0; i < n; i++) {
               GLint r = (GLint) arg0[i][RCOMP] + (GLint) arg1[i][RCOMP] - 128;
               GLint g = (GLint) arg0[i][GCOMP] + (GLint) arg1[i][GCOMP] - 128;
               GLint b = (GLint) arg0[i][BCOMP] + (GLint) arg1[i][BCOMP] - 128;
               r = (r < 0) ? 0 : r << RGBshift;
               g = (g < 0) ? 0 : g << RGBshift;
               b = (b < 0) ? 0 : b << RGBshift;
               rgba[i][RCOMP] = (GLchan) MIN2(r, CHAN_MAX);
               rgba[i][GCOMP] = (GLchan) MIN2(g, CHAN_MAX);
               rgba[i][BCOMP] = (GLchan) MIN2(b, CHAN_MAX);
            }
         }
         break;
      case GL_INTERPOLATE_EXT:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argRGB[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argRGB[1];
            const GLchan (*arg2)[4] = (const GLchan (*)[4]) argRGB[2];
            const GLint shift = 8 - RGBshift;
            for (i = 0; i < n; i++) {
               GLuint r = (PROD(arg0[i][RCOMP], arg2[i][RCOMP])
                           + PROD(arg1[i][RCOMP], CHAN_MAX - arg2[i][RCOMP]))
                              >> shift;
               GLuint g = (PROD(arg0[i][GCOMP], arg2[i][GCOMP])
                           + PROD(arg1[i][GCOMP], CHAN_MAX - arg2[i][GCOMP]))
                              >> shift;
               GLuint b = (PROD(arg0[i][BCOMP], arg2[i][BCOMP])
                           + PROD(arg1[i][BCOMP], CHAN_MAX - arg2[i][BCOMP]))
                              >> shift;
               rgba[i][RCOMP] = (GLchan) MIN2(r, CHAN_MAX);
               rgba[i][GCOMP] = (GLchan) MIN2(g, CHAN_MAX);
               rgba[i][BCOMP] = (GLchan) MIN2(b, CHAN_MAX);
            }
         }
         break;
      case GL_SUBTRACT_ARB:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argRGB[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argRGB[1];
            for (i = 0; i < n; i++) {
               GLint r = ((GLint) arg0[i][RCOMP] - (GLint) arg1[i][RCOMP]) << RGBshift;
               GLint g = ((GLint) arg0[i][GCOMP] - (GLint) arg1[i][GCOMP]) << RGBshift;
               GLint b = ((GLint) arg0[i][BCOMP] - (GLint) arg1[i][BCOMP]) << RGBshift;
               rgba[i][RCOMP] = (GLchan) CLAMP(r, 0, CHAN_MAX);
               rgba[i][GCOMP] = (GLchan) CLAMP(g, 0, CHAN_MAX);
               rgba[i][BCOMP] = (GLchan) CLAMP(b, 0, CHAN_MAX);
            }
         }
         break;
      case GL_DOT3_RGB_EXT:
      case GL_DOT3_RGBA_EXT:
      case GL_DOT3_RGB_ARB:
      case GL_DOT3_RGBA_ARB:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argRGB[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argRGB[1];
	    /* ATI's EXT extension has a constant scale by 4.  The ARB
	     * one will likely remove this restriction, and we should
	     * drop the EXT extension in favour of the ARB one.
	     */
            for (i = 0; i < n; i++) {
               GLint dot = (S_PROD((GLint)arg0[i][RCOMP] - 128,
				   (GLint)arg1[i][RCOMP] - 128) +
			    S_PROD((GLint)arg0[i][GCOMP] - 128,
				   (GLint)arg1[i][GCOMP] - 128) +
			    S_PROD((GLint)arg0[i][BCOMP] - 128,
				   (GLint)arg1[i][BCOMP] - 128)) >> 6;
               dot = CLAMP(dot, 0, 255);
               rgba[i][RCOMP] = rgba[i][GCOMP] = rgba[i][BCOMP] = (GLchan) dot;
            }
         }
         break;
      default:
         _mesa_problem(NULL, "invalid combine mode");
   }

   switch (textureUnit->CombineModeA) {
      case GL_REPLACE:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argA[0];
            if (Ashift) {
               for (i = 0; i < n; i++) {
                  GLuint a = (GLuint) arg0[i][ACOMP] << Ashift;
                  rgba[i][ACOMP] = (GLchan) MIN2(a, CHAN_MAX);
               }
            }
            else {
               for (i = 0; i < n; i++) {
                  rgba[i][ACOMP] = arg0[i][ACOMP];
               }
            }
         }
         break;
      case GL_MODULATE:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argA[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argA[1];
            const GLint shift = 8 - Ashift;
            for (i = 0; i < n; i++) {
               GLuint a = (PROD(arg0[i][ACOMP], arg1[i][ACOMP]) >> shift);
               rgba[i][ACOMP] = (GLchan) MIN2(a, CHAN_MAX);
            }
         }
         break;
      case GL_ADD:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argA[0];
            const GLchan  (*arg1)[4] = (const GLchan (*)[4]) argA[1];
            for (i = 0; i < n; i++) {
               GLint a = ((GLint) arg0[i][ACOMP] + arg1[i][ACOMP]) << Ashift;
               rgba[i][ACOMP] = (GLchan) MIN2(a, CHAN_MAX);
            }
         }
         break;
      case GL_ADD_SIGNED_EXT:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argA[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argA[1];
            for (i = 0; i < n; i++) {
               GLint a = (GLint) arg0[i][ACOMP] + (GLint) arg1[i][ACOMP] - 128;
               a = (a < 0) ? 0 : a << Ashift;
               rgba[i][ACOMP] = (GLchan) MIN2(a, CHAN_MAX);
            }
         }
         break;
      case GL_INTERPOLATE_EXT:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argA[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argA[1];
            const GLchan (*arg2)[4] = (const GLchan (*)[4]) argA[2];
            const GLint shift = 8 - Ashift;
            for (i=0; i<n; i++) {
               GLuint a = (PROD(arg0[i][ACOMP], arg2[i][ACOMP])
                           + PROD(arg1[i][ACOMP], CHAN_MAX - arg2[i][ACOMP]))
                              >> shift;
               rgba[i][ACOMP] = (GLchan) MIN2(a, CHAN_MAX);
            }
         }
         break;
      case GL_SUBTRACT_ARB:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argRGB[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argRGB[1];
            for (i = 0; i < n; i++) {
               GLint a = ((GLint) arg0[i][ACOMP] - (GLint) arg1[i][ACOMP]) << RGBshift;
               rgba[i][ACOMP] = (GLchan) CLAMP(a, 0, CHAN_MAX);
            }
         }
         break;

      default:
         _mesa_problem(NULL, "invalid combine mode");
   }

   /* Fix the alpha component for GL_DOT3_RGBA_EXT combining.
    */
   if (textureUnit->CombineModeRGB == GL_DOT3_RGBA_EXT ||
       textureUnit->CombineModeRGB == GL_DOT3_RGBA_ARB) {
      for (i = 0; i < n; i++) {
	 rgba[i][ACOMP] = rgba[i][RCOMP];
      }
   }
   UNDEFARRAY(ccolor);  /* mac 32k limitation */
}
#undef PROD



/**********************************************************************/
/*                      Texture Application                           */
/**********************************************************************/


/*
 * Combine incoming fragment color with texel color to produce output color.
 * Input:  textureUnit - pointer to texture unit to apply
 *         format - base internal texture format
 *         n - number of fragments
 *         primary_rgba - primary colors (may alias rgba for single texture)
 *         texels - array of texel colors
 * InOut:  rgba - incoming fragment colors modified by texel colors
 *                according to the texture environment mode.
 */
static void
apply_texture( const GLcontext *ctx,
               const struct gl_texture_unit *texUnit,
               GLuint n,
               CONST GLchan primary_rgba[][4], CONST GLchan texel[][4],
               GLchan rgba[][4] )
{
   GLint baseLevel;
   GLuint i;
   GLint Rc, Gc, Bc, Ac;
   GLenum format;

   ASSERT(texUnit);
   ASSERT(texUnit->_Current);

   baseLevel = texUnit->_Current->BaseLevel;
   ASSERT(texUnit->_Current->Image[baseLevel]);

   format = texUnit->_Current->Image[baseLevel]->Format;

   if (format==GL_COLOR_INDEX || format==GL_DEPTH_COMPONENT) {
      format = GL_RGBA;  /* XXXX a hack! */
   }

   switch (texUnit->EnvMode) {
      case GL_REPLACE:
	 switch (format) {
	    case GL_ALPHA:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf */
                  /* Av = At */
                  rgba[i][ACOMP] = texel[i][ACOMP];
	       }
	       break;
	    case GL_LUMINANCE:
	       for (i=0;i<n;i++) {
		  /* Cv = Lt */
                  GLchan Lt = texel[i][RCOMP];
                  rgba[i][RCOMP] = rgba[i][GCOMP] = rgba[i][BCOMP] = Lt;
                  /* Av = Af */
	       }
	       break;
	    case GL_LUMINANCE_ALPHA:
	       for (i=0;i<n;i++) {
                  GLchan Lt = texel[i][RCOMP];
		  /* Cv = Lt */
		  rgba[i][RCOMP] = rgba[i][GCOMP] = rgba[i][BCOMP] = Lt;
		  /* Av = At */
		  rgba[i][ACOMP] = texel[i][ACOMP];
	       }
	       break;
	    case GL_INTENSITY:
	       for (i=0;i<n;i++) {
		  /* Cv = It */
                  GLchan It = texel[i][RCOMP];
                  rgba[i][RCOMP] = rgba[i][GCOMP] = rgba[i][BCOMP] = It;
                  /* Av = It */
                  rgba[i][ACOMP] = It;
	       }
	       break;
	    case GL_RGB:
	       for (i=0;i<n;i++) {
		  /* Cv = Ct */
		  rgba[i][RCOMP] = texel[i][RCOMP];
		  rgba[i][GCOMP] = texel[i][GCOMP];
		  rgba[i][BCOMP] = texel[i][BCOMP];
		  /* Av = Af */
	       }
	       break;
	    case GL_RGBA:
	       for (i=0;i<n;i++) {
		  /* Cv = Ct */
		  rgba[i][RCOMP] = texel[i][RCOMP];
		  rgba[i][GCOMP] = texel[i][GCOMP];
		  rgba[i][BCOMP] = texel[i][BCOMP];
		  /* Av = At */
		  rgba[i][ACOMP] = texel[i][ACOMP];
	       }
	       break;
            default:
               _mesa_problem(ctx, "Bad format (GL_REPLACE) in apply_texture");
               return;
	 }
	 break;

      case GL_MODULATE:
         switch (format) {
	    case GL_ALPHA:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf */
		  /* Av = AfAt */
		  rgba[i][ACOMP] = CHAN_PRODUCT( rgba[i][ACOMP], texel[i][ACOMP] );
	       }
	       break;
	    case GL_LUMINANCE:
	       for (i=0;i<n;i++) {
		  /* Cv = LtCf */
                  GLchan Lt = texel[i][RCOMP];
		  rgba[i][RCOMP] = CHAN_PRODUCT( rgba[i][RCOMP], Lt );
		  rgba[i][GCOMP] = CHAN_PRODUCT( rgba[i][GCOMP], Lt );
		  rgba[i][BCOMP] = CHAN_PRODUCT( rgba[i][BCOMP], Lt );
		  /* Av = Af */
	       }
	       break;
	    case GL_LUMINANCE_ALPHA:
	       for (i=0;i<n;i++) {
		  /* Cv = CfLt */
                  GLchan Lt = texel[i][RCOMP];
		  rgba[i][RCOMP] = CHAN_PRODUCT( rgba[i][RCOMP], Lt );
		  rgba[i][GCOMP] = CHAN_PRODUCT( rgba[i][GCOMP], Lt );
		  rgba[i][BCOMP] = CHAN_PRODUCT( rgba[i][BCOMP], Lt );
		  /* Av = AfAt */
		  rgba[i][ACOMP] = CHAN_PRODUCT( rgba[i][ACOMP], texel[i][ACOMP] );
	       }
	       break;
	    case GL_INTENSITY:
	       for (i=0;i<n;i++) {
		  /* Cv = CfIt */
                  GLchan It = texel[i][RCOMP];
		  rgba[i][RCOMP] = CHAN_PRODUCT( rgba[i][RCOMP], It );
		  rgba[i][GCOMP] = CHAN_PRODUCT( rgba[i][GCOMP], It );
		  rgba[i][BCOMP] = CHAN_PRODUCT( rgba[i][BCOMP], It );
		  /* Av = AfIt */
		  rgba[i][ACOMP] = CHAN_PRODUCT( rgba[i][ACOMP], It );
	       }
	       break;
	    case GL_RGB:
	       for (i=0;i<n;i++) {
		  /* Cv = CfCt */
		  rgba[i][RCOMP] = CHAN_PRODUCT( rgba[i][RCOMP], texel[i][RCOMP] );
		  rgba[i][GCOMP] = CHAN_PRODUCT( rgba[i][GCOMP], texel[i][GCOMP] );
		  rgba[i][BCOMP] = CHAN_PRODUCT( rgba[i][BCOMP], texel[i][BCOMP] );
		  /* Av = Af */
	       }
	       break;
	    case GL_RGBA:
	       for (i=0;i<n;i++) {
		  /* Cv = CfCt */
		  rgba[i][RCOMP] = CHAN_PRODUCT( rgba[i][RCOMP], texel[i][RCOMP] );
		  rgba[i][GCOMP] = CHAN_PRODUCT( rgba[i][GCOMP], texel[i][GCOMP] );
		  rgba[i][BCOMP] = CHAN_PRODUCT( rgba[i][BCOMP], texel[i][BCOMP] );
		  /* Av = AfAt */
		  rgba[i][ACOMP] = CHAN_PRODUCT( rgba[i][ACOMP], texel[i][ACOMP] );
	       }
	       break;
            default:
               _mesa_problem(ctx, "Bad format (GL_MODULATE) in apply_texture");
               return;
	 }
	 break;

      case GL_DECAL:
         switch (format) {
            case GL_ALPHA:
            case GL_LUMINANCE:
            case GL_LUMINANCE_ALPHA:
            case GL_INTENSITY:
               /* undefined */
               break;
	    case GL_RGB:
	       for (i=0;i<n;i++) {
		  /* Cv = Ct */
		  rgba[i][RCOMP] = texel[i][RCOMP];
		  rgba[i][GCOMP] = texel[i][GCOMP];
		  rgba[i][BCOMP] = texel[i][BCOMP];
		  /* Av = Af */
	       }
	       break;
	    case GL_RGBA:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf(1-At) + CtAt */
		  GLint t = texel[i][ACOMP], s = CHAN_MAX - t;
		  rgba[i][RCOMP] = CHAN_PRODUCT(rgba[i][RCOMP], s) + CHAN_PRODUCT(texel[i][RCOMP],t);
		  rgba[i][GCOMP] = CHAN_PRODUCT(rgba[i][GCOMP], s) + CHAN_PRODUCT(texel[i][GCOMP],t);
		  rgba[i][BCOMP] = CHAN_PRODUCT(rgba[i][BCOMP], s) + CHAN_PRODUCT(texel[i][BCOMP],t);
		  /* Av = Af */
	       }
	       break;
            default:
               _mesa_problem(ctx, "Bad format (GL_DECAL) in apply_texture");
               return;
	 }
	 break;

      case GL_BLEND:
         Rc = (GLint) (texUnit->EnvColor[0] * CHAN_MAXF);
         Gc = (GLint) (texUnit->EnvColor[1] * CHAN_MAXF);
         Bc = (GLint) (texUnit->EnvColor[2] * CHAN_MAXF);
         Ac = (GLint) (texUnit->EnvColor[3] * CHAN_MAXF);
	 switch (format) {
	    case GL_ALPHA:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf */
		  /* Av = AfAt */
                  rgba[i][ACOMP] = CHAN_PRODUCT(rgba[i][ACOMP], texel[i][ACOMP]);
	       }
	       break;
            case GL_LUMINANCE:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf(1-Lt) + CcLt */
		  GLchan Lt = texel[i][RCOMP], s = CHAN_MAX - Lt;
		  rgba[i][RCOMP] = CHAN_PRODUCT(rgba[i][RCOMP], s) + CHAN_PRODUCT(Rc, Lt);
		  rgba[i][GCOMP] = CHAN_PRODUCT(rgba[i][GCOMP], s) + CHAN_PRODUCT(Gc, Lt);
		  rgba[i][BCOMP] = CHAN_PRODUCT(rgba[i][BCOMP], s) + CHAN_PRODUCT(Bc, Lt);
		  /* Av = Af */
	       }
	       break;
	    case GL_LUMINANCE_ALPHA:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf(1-Lt) + CcLt */
		  GLchan Lt = texel[i][RCOMP], s = CHAN_MAX - Lt;
		  rgba[i][RCOMP] = CHAN_PRODUCT(rgba[i][RCOMP], s) + CHAN_PRODUCT(Rc, Lt);
		  rgba[i][GCOMP] = CHAN_PRODUCT(rgba[i][GCOMP], s) + CHAN_PRODUCT(Gc, Lt);
		  rgba[i][BCOMP] = CHAN_PRODUCT(rgba[i][BCOMP], s) + CHAN_PRODUCT(Bc, Lt);
		  /* Av = AfAt */
		  rgba[i][ACOMP] = CHAN_PRODUCT(rgba[i][ACOMP],texel[i][ACOMP]);
	       }
	       break;
            case GL_INTENSITY:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf(1-It) + CcLt */
		  GLchan It = texel[i][RCOMP], s = CHAN_MAX - It;
		  rgba[i][RCOMP] = CHAN_PRODUCT(rgba[i][RCOMP], s) + CHAN_PRODUCT(Rc, It);
		  rgba[i][GCOMP] = CHAN_PRODUCT(rgba[i][GCOMP], s) + CHAN_PRODUCT(Gc, It);
		  rgba[i][BCOMP] = CHAN_PRODUCT(rgba[i][BCOMP], s) + CHAN_PRODUCT(Bc, It);
                  /* Av = Af(1-It) + Ac*It */
                  rgba[i][ACOMP] = CHAN_PRODUCT(rgba[i][ACOMP], s) + CHAN_PRODUCT(Ac, It);
               }
               break;
	    case GL_RGB:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf(1-Ct) + CcCt */
		  rgba[i][RCOMP] = CHAN_PRODUCT(rgba[i][RCOMP], (CHAN_MAX-texel[i][RCOMP])) + CHAN_PRODUCT(Rc,texel[i][RCOMP]);
		  rgba[i][GCOMP] = CHAN_PRODUCT(rgba[i][GCOMP], (CHAN_MAX-texel[i][GCOMP])) + CHAN_PRODUCT(Gc,texel[i][GCOMP]);
		  rgba[i][BCOMP] = CHAN_PRODUCT(rgba[i][BCOMP], (CHAN_MAX-texel[i][BCOMP])) + CHAN_PRODUCT(Bc,texel[i][BCOMP]);
		  /* Av = Af */
	       }
	       break;
	    case GL_RGBA:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf(1-Ct) + CcCt */
		  rgba[i][RCOMP] = CHAN_PRODUCT(rgba[i][RCOMP], (CHAN_MAX-texel[i][RCOMP])) + CHAN_PRODUCT(Rc,texel[i][RCOMP]);
		  rgba[i][GCOMP] = CHAN_PRODUCT(rgba[i][GCOMP], (CHAN_MAX-texel[i][GCOMP])) + CHAN_PRODUCT(Gc,texel[i][GCOMP]);
		  rgba[i][BCOMP] = CHAN_PRODUCT(rgba[i][BCOMP], (CHAN_MAX-texel[i][BCOMP])) + CHAN_PRODUCT(Bc,texel[i][BCOMP]);
		  /* Av = AfAt */
		  rgba[i][ACOMP] = CHAN_PRODUCT(rgba[i][ACOMP],texel[i][ACOMP]);
	       }
	       break;
            default:
               _mesa_problem(ctx, "Bad format (GL_BLEND) in apply_texture");
               return;
	 }
	 break;

      case GL_ADD:  /* GL_EXT_texture_add_env */
         switch (format) {
            case GL_ALPHA:
               for (i=0;i<n;i++) {
                  /* Rv = Rf */
                  /* Gv = Gf */
                  /* Bv = Bf */
                  rgba[i][ACOMP] = CHAN_PRODUCT(rgba[i][ACOMP], texel[i][ACOMP]);
               }
               break;
            case GL_LUMINANCE:
               for (i=0;i<n;i++) {
                  GLuint Lt = texel[i][RCOMP];
                  GLuint r = rgba[i][RCOMP] + Lt;
                  GLuint g = rgba[i][GCOMP] + Lt;
                  GLuint b = rgba[i][BCOMP] + Lt;
                  rgba[i][RCOMP] = MIN2(r, CHAN_MAX);
                  rgba[i][GCOMP] = MIN2(g, CHAN_MAX);
                  rgba[i][BCOMP] = MIN2(b, CHAN_MAX);
                  /* Av = Af */
               }
               break;
            case GL_LUMINANCE_ALPHA:
               for (i=0;i<n;i++) {
                  GLuint Lt = texel[i][RCOMP];
                  GLuint r = rgba[i][RCOMP] + Lt;
                  GLuint g = rgba[i][GCOMP] + Lt;
                  GLuint b = rgba[i][BCOMP] + Lt;
                  rgba[i][RCOMP] = MIN2(r, CHAN_MAX);
                  rgba[i][GCOMP] = MIN2(g, CHAN_MAX);
                  rgba[i][BCOMP] = MIN2(b, CHAN_MAX);
                  rgba[i][ACOMP] = CHAN_PRODUCT(rgba[i][ACOMP], texel[i][ACOMP]);
               }
               break;
            case GL_INTENSITY:
               for (i=0;i<n;i++) {
                  GLchan It = texel[i][RCOMP];
                  GLuint r = rgba[i][RCOMP] + It;
                  GLuint g = rgba[i][GCOMP] + It;
                  GLuint b = rgba[i][BCOMP] + It;
                  GLuint a = rgba[i][ACOMP] + It;
                  rgba[i][RCOMP] = MIN2(r, CHAN_MAX);
                  rgba[i][GCOMP] = MIN2(g, CHAN_MAX);
                  rgba[i][BCOMP] = MIN2(b, CHAN_MAX);
                  rgba[i][ACOMP] = MIN2(a, CHAN_MAX);
               }
               break;
	    case GL_RGB:
	       for (i=0;i<n;i++) {
                  GLuint r = rgba[i][RCOMP] + texel[i][RCOMP];
                  GLuint g = rgba[i][GCOMP] + texel[i][GCOMP];
                  GLuint b = rgba[i][BCOMP] + texel[i][BCOMP];
		  rgba[i][RCOMP] = MIN2(r, CHAN_MAX);
		  rgba[i][GCOMP] = MIN2(g, CHAN_MAX);
		  rgba[i][BCOMP] = MIN2(b, CHAN_MAX);
		  /* Av = Af */
	       }
	       break;
	    case GL_RGBA:
	       for (i=0;i<n;i++) {
                  GLuint r = rgba[i][RCOMP] + texel[i][RCOMP];
                  GLuint g = rgba[i][GCOMP] + texel[i][GCOMP];
                  GLuint b = rgba[i][BCOMP] + texel[i][BCOMP];
		  rgba[i][RCOMP] = MIN2(r, CHAN_MAX);
		  rgba[i][GCOMP] = MIN2(g, CHAN_MAX);
		  rgba[i][BCOMP] = MIN2(b, CHAN_MAX);
                  rgba[i][ACOMP] = CHAN_PRODUCT(rgba[i][ACOMP], texel[i][ACOMP]);
               }
               break;
            default:
               _mesa_problem(ctx, "Bad format (GL_ADD) in apply_texture");
               return;
	 }
	 break;

      case GL_COMBINE_EXT:
         texture_combine(ctx, texUnit, n, primary_rgba, texel, rgba);
         break;

      default:
         _mesa_problem(ctx, "Bad env mode in apply_texture");
         return;
   }
}



/*
 * Sample a shadow/depth texture.
 * Input:  ctx - context
 *         texUnit - the texture unit
 *         n - number of samples
 *         s,t,r - array [n] of texture coordinates
 * In/Out:  rgba - array [n] of texel colors.
 */
static void
sample_depth_texture(const GLcontext *ctx,
                     const struct gl_texture_unit *texUnit,
                     GLuint n,
                     const GLfloat s[], const GLfloat t[], const GLfloat r[],
                     GLchan texel[][4])
{
   const struct gl_texture_object *texObj = texUnit->_Current;
   const GLint baseLevel = texObj->BaseLevel;
   const struct gl_texture_image *texImage = texObj->Image[baseLevel];
   const GLuint width = texImage->Width;
   const GLuint height = texImage->Height;
   const GLchan ambient = texObj->ShadowAmbient;
   GLboolean lequal, gequal;

   if (texObj->Dimensions != 2) {
      _mesa_problem(ctx, "only 2-D depth textures supported at this time");
      return;
   }

   if (texObj->MinFilter != texObj->MagFilter) {
      _mesa_problem(ctx, "mipmapped depth textures not supported at this time");
      return;
   }

   /* XXX the GL_SGIX_shadow extension spec doesn't say what to do if
    * GL_TEXTURE_COMPARE_SGIX == GL_TRUE but the current texture object
    * isn't a depth texture.
    */
   if (texImage->Format != GL_DEPTH_COMPONENT) {
      _mesa_problem(ctx,"GL_TEXTURE_COMPARE_SGIX enabled with non-depth texture");
      return;
   }

   if (texObj->CompareOperator == GL_TEXTURE_LEQUAL_R_SGIX) {
      lequal = GL_TRUE;
      gequal = GL_FALSE;
   }
   else {
      lequal = GL_FALSE;
      gequal = GL_TRUE;
   }

   if (texObj->MagFilter == GL_NEAREST) {
      GLuint i;
      for (i = 0; i < n; i++) {
         GLfloat depthSample;
         GLint col, row;
         COMPUTE_NEAREST_TEXEL_LOCATION(texObj->WrapS, s[i], width, col);
         COMPUTE_NEAREST_TEXEL_LOCATION(texObj->WrapT, t[i], height, row);
         depthSample = *((const GLfloat *) texImage->Data + row * width + col);
         if ((r[i] <= depthSample && lequal) ||
             (r[i] >= depthSample && gequal)) {
            texel[i][RCOMP] = CHAN_MAX;
            texel[i][GCOMP] = CHAN_MAX;
            texel[i][BCOMP] = CHAN_MAX;
            texel[i][ACOMP] = CHAN_MAX;
         }
         else {
            texel[i][RCOMP] = ambient;
            texel[i][GCOMP] = ambient;
            texel[i][BCOMP] = ambient;
            texel[i][ACOMP] = CHAN_MAX;
         }
      }
   }
   else {
      GLuint i;
      ASSERT(texObj->MagFilter == GL_LINEAR);
      for (i = 0; i < n; i++) {
         GLfloat depth00, depth01, depth10, depth11;
         GLint i0, i1, j0, j1;
         GLfloat u, v;
         GLuint useBorderTexel;

         COMPUTE_LINEAR_TEXEL_LOCATIONS(texObj->WrapS, s[i], u, width, i0, i1);
         COMPUTE_LINEAR_TEXEL_LOCATIONS(texObj->WrapT, t[i], v, height,j0, j1);

         useBorderTexel = 0;
         if (texImage->Border) {
            i0 += texImage->Border;
            i1 += texImage->Border;
            j0 += texImage->Border;
            j1 += texImage->Border;
         }
         else {
            if (i0 < 0 || i0 >= (GLint) width)   useBorderTexel |= I0BIT;
            if (i1 < 0 || i1 >= (GLint) width)   useBorderTexel |= I1BIT;
            if (j0 < 0 || j0 >= (GLint) height)  useBorderTexel |= J0BIT;
            if (j1 < 0 || j1 >= (GLint) height)  useBorderTexel |= J1BIT;
         }

         /* get four depth samples from the texture */
         if (useBorderTexel & (I0BIT | J0BIT)) {
            depth00 = 1.0;
         }
         else {
            depth00 = *((const GLfloat *) texImage->Data + j0 * width + i0);
         }
         if (useBorderTexel & (I1BIT | J0BIT)) {
            depth10 = 1.0;
         }
         else {
            depth10 = *((const GLfloat *) texImage->Data + j0 * width + i1);
         }
         if (useBorderTexel & (I0BIT | J1BIT)) {
            depth01 = 1.0;
         }
         else {
            depth01 = *((const GLfloat *) texImage->Data + j1 * width + i0);
         }
         if (useBorderTexel & (I1BIT | J1BIT)) {
            depth11 = 1.0;
         }
         else {
            depth11 = *((const GLfloat *) texImage->Data + j1 * width + i1);
         }

         if (0) {
            /* compute a single weighted depth sample and do one comparison */
            const GLfloat a = FRAC(u + 1.0F);
            const GLfloat b = FRAC(v + 1.0F);
            const GLfloat w00 = (1.0F - a) * (1.0F - b);
            const GLfloat w10 = (       a) * (1.0F - b);
            const GLfloat w01 = (1.0F - a) * (       b);
            const GLfloat w11 = (       a) * (       b);
            const GLfloat depthSample = w00 * depth00 + w10 * depth10
                                      + w01 * depth01 + w11 * depth11;
            if ((depthSample <= r[i] && lequal) ||
                (depthSample >= r[i] && gequal)) {
               texel[i][RCOMP] = ambient;
               texel[i][GCOMP] = ambient;
               texel[i][BCOMP] = ambient;
               texel[i][ACOMP] = CHAN_MAX;
            }
            else {
               texel[i][RCOMP] = CHAN_MAX;
               texel[i][GCOMP] = CHAN_MAX;
               texel[i][BCOMP] = CHAN_MAX;
               texel[i][ACOMP] = CHAN_MAX;
            }
         }
         else {
            /* Do four depth/R comparisons and compute a weighted result.
             * If this touches on somebody's I.P., I'll remove this code
             * upon request.
             */
            const GLfloat d = (CHAN_MAXF - (GLfloat) ambient) * 0.25F;
            GLfloat luminance = CHAN_MAXF;
            GLchan lum;
            if (lequal) {
               if (depth00 <= r[i])   luminance -= d;
               if (depth01 <= r[i])   luminance -= d;
               if (depth10 <= r[i])   luminance -= d;
               if (depth11 <= r[i])   luminance -= d;
            }
            else {
               if (depth00 >= r[i])   luminance -= d;
               if (depth01 >= r[i])   luminance -= d;
               if (depth10 >= r[i])   luminance -= d;
               if (depth11 >= r[i])   luminance -= d;
            }
            lum = (GLchan) luminance;
            texel[i][RCOMP] = lum;
            texel[i][GCOMP] = lum;
            texel[i][BCOMP] = lum;
            texel[i][ACOMP] = CHAN_MAX;
         }
      }
   }
}


#if 0
/*
 * Experimental depth texture sampling function.
 */
static void
sample_depth_texture2(const GLcontext *ctx,
                     const struct gl_texture_unit *texUnit,
                     GLuint n,
                     const GLfloat s[], const GLfloat t[], const GLfloat r[],
                     GLchan texel[][4])
{
   const struct gl_texture_object *texObj = texUnit->_Current;
   const GLint baseLevel = texObj->BaseLevel;
   const struct gl_texture_image *texImage = texObj->Image[baseLevel];
   const GLuint width = texImage->Width;
   const GLuint height = texImage->Height;
   const GLchan ambient = texObj->ShadowAmbient;
   GLboolean lequal, gequal;

   if (texObj->Dimensions != 2) {
      _mesa_problem(ctx, "only 2-D depth textures supported at this time");
      return;
   }

   if (texObj->MinFilter != texObj->MagFilter) {
      _mesa_problem(ctx, "mipmapped depth textures not supported at this time");
      return;
   }

   /* XXX the GL_SGIX_shadow extension spec doesn't say what to do if
    * GL_TEXTURE_COMPARE_SGIX == GL_TRUE but the current texture object
    * isn't a depth texture.
    */
   if (texImage->Format != GL_DEPTH_COMPONENT) {
      _mesa_problem(ctx,"GL_TEXTURE_COMPARE_SGIX enabled with non-depth texture");
      return;
   }

   if (texObj->CompareOperator == GL_TEXTURE_LEQUAL_R_SGIX) {
      lequal = GL_TRUE;
      gequal = GL_FALSE;
   }
   else {
      lequal = GL_FALSE;
      gequal = GL_TRUE;
   }

   {
      GLuint i;
      for (i = 0; i < n; i++) {
         const GLint K = 3;
         GLint col, row, ii, jj, imin, imax, jmin, jmax, samples, count;
         GLfloat w;
         GLchan lum;
         COMPUTE_NEAREST_TEXEL_LOCATION(texObj->WrapS, s[i], width, col);
         COMPUTE_NEAREST_TEXEL_LOCATION(texObj->WrapT, t[i], height, row);

         imin = col - K;
         imax = col + K;
         jmin = row - K;
         jmax = row + K;

         if (imin < 0)  imin = 0;
         if (imax >= width)  imax = width - 1;
         if (jmin < 0)  jmin = 0;
         if (jmax >= height) jmax = height - 1;

         samples = (imax - imin + 1) * (jmax - jmin + 1);
         count = 0;
         for (jj = jmin; jj <= jmax; jj++) {
            for (ii = imin; ii <= imax; ii++) {
               GLfloat depthSample = *((const GLfloat *) texImage->Data
                                       + jj * width + ii);
               if ((depthSample <= r[i] && lequal) ||
                   (depthSample >= r[i] && gequal)) {
                  count++;
               }
            }
         }

         w = (GLfloat) count / (GLfloat) samples;
         w = CHAN_MAXF - w * (CHAN_MAXF - (GLfloat) ambient);
         lum = (GLint) w;

         texel[i][RCOMP] = lum;
         texel[i][GCOMP] = lum;
         texel[i][BCOMP] = lum;
         texel[i][ACOMP] = CHAN_MAX;
      }
   }
}
#endif


/*
 * Apply a unit of texture mapping to the incoming fragments.
 */
void
_swrast_texture_fragments( GLcontext *ctx, GLuint texUnit, GLuint n,
                           const GLfloat s[], const GLfloat t[],
                           const GLfloat r[], GLfloat lambda[],
                           CONST GLchan primary_rgba[][4], GLchan rgba[][4] )
{
   const GLuint mask = TEXTURE0_ANY << (texUnit * 4);

   if (ctx->Texture._ReallyEnabled & mask) {
      const struct gl_texture_unit *textureUnit = &ctx->Texture.Unit[texUnit];

      if (textureUnit->_Current) {   /* XXX need this? */
         GLchan texel[PB_SIZE][4];

	 if (textureUnit->LodBias != 0.0F) {
	    /* apply LOD bias, but don't clamp yet */
            GLuint i;
	    for (i=0;i<n;i++) {
	       lambda[i] += textureUnit->LodBias;
	    }
	 }

         if ((textureUnit->_Current->MinLod != -1000.0
              || textureUnit->_Current->MaxLod != 1000.0)
             && lambda) {
            /* apply LOD clamping to lambda */
            const GLfloat min = textureUnit->_Current->MinLod;
            const GLfloat max = textureUnit->_Current->MaxLod;
            GLuint i;
            for (i=0;i<n;i++) {
               GLfloat l = lambda[i];
               lambda[i] = CLAMP(l, min, max);
            }
         }

         /* Sample the texture. */
         if (textureUnit->_Current->CompareFlag) {
            /* depth texture */
            sample_depth_texture(ctx, textureUnit, n, s, t, r, texel);
         }
         else {
            /* color texture */
            SWRAST_CONTEXT(ctx)->TextureSample[texUnit]( ctx, texUnit,
                                                         textureUnit->_Current,
                                                         n, s, t, r,
                                                         lambda, texel );
         }
         apply_texture( ctx, textureUnit, n, primary_rgba,
                        (const GLchan (*)[4]) texel, rgba );
      }
   }
}

/* $Id: s_fog.c,v 1.13 2001/06/18 23:55:18 brianp Exp $ */

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
#include "macros.h"
#include "mmath.h"

#include "s_context.h"
#include "s_fog.h"
#include "s_pb.h"




/*
 * Used to convert current raster distance to a fog factor in [0,1].
 */
GLfloat
_mesa_z_to_fogfactor(GLcontext *ctx, GLfloat z)
{
   GLfloat d, f;

   switch (ctx->Fog.Mode) {
   case GL_LINEAR:
      if (ctx->Fog.Start == ctx->Fog.End)
         d = 1.0F;
      else
         d = 1.0F / (ctx->Fog.End - ctx->Fog.Start);
      f = (ctx->Fog.End - z) * d;
      return CLAMP(f, 0.0F, 1.0F);
   case GL_EXP:
      d = ctx->Fog.Density;
      f = exp(-d * z);
      return f;
   case GL_EXP2:
      d = ctx->Fog.Density;
      f = exp(-(d * d * z * z));
      return f;
   default:
      _mesa_problem(ctx, "Bad fog mode in make_fog_coord");
      return 0.0; 
   }
}



/*
 * Apply fog to an array of RGBA pixels.
 * Input:  n - number of pixels
 *         fog - array of fog factors in [0,1]
 *         red, green, blue, alpha - pixel colors
 * Output:  red, green, blue, alpha - fogged pixel colors
 */
void
_mesa_fog_rgba_pixels( const GLcontext *ctx,
                       GLuint n,
		       const GLfloat fog[],
		       GLchan rgba[][4] )
{
   GLuint i;
   GLchan rFog, gFog, bFog;

   UNCLAMPED_FLOAT_TO_CHAN(rFog, ctx->Fog.Color[RCOMP]);
   UNCLAMPED_FLOAT_TO_CHAN(gFog, ctx->Fog.Color[GCOMP]);
   UNCLAMPED_FLOAT_TO_CHAN(bFog, ctx->Fog.Color[BCOMP]);

   for (i = 0; i < n; i++) {
      const GLfloat f = fog[i];
      const GLfloat g = 1.0 - f;
      rgba[i][RCOMP] = f * rgba[i][RCOMP] + g * rFog;
      rgba[i][GCOMP] = f * rgba[i][GCOMP] + g * gFog;
      rgba[i][BCOMP] = f * rgba[i][BCOMP] + g * bFog;
   }
}



/*
 * Apply fog to an array of color index pixels.
 * Input:  n - number of pixels
 *         fog - array of fog factors in [0,1]
 *         index - pixel color indexes
 * Output:  index - fogged pixel color indexes
 */
void
_mesa_fog_ci_pixels( const GLcontext *ctx,
                     GLuint n, const GLfloat fog[], GLuint index[] )
{
   GLuint idx = (GLuint) ctx->Fog.Index;
   GLuint i;

   for (i = 0; i < n; i++) {
      const GLfloat f = CLAMP(fog[i], 0.0, 1.0);
      index[i] = (GLuint) ((GLfloat) index[i] + (1.0F - f) * idx);
   }
}



/*
 * Calculate fog factors (in [0,1]) from window z values
 * Input:  n - number of pixels
 *         z - array of integer depth values
 *         red, green, blue, alpha - pixel colors
 * Output:  red, green, blue, alpha - fogged pixel colors
 *
 * Use lookup table & interpolation?
 */
static void
compute_fog_factors_from_z( const GLcontext *ctx,
                            GLuint n,
                            const GLdepth z[],
                            GLfloat fogFact[] )
{
   const GLboolean ortho = (ctx->ProjectionMatrix.m[15] != 0.0F);
   const GLfloat p10 = ctx->ProjectionMatrix.m[10];
   const GLfloat p14 = ctx->ProjectionMatrix.m[14];
   const GLfloat tz = ctx->Viewport._WindowMap.m[MAT_TZ];
   GLfloat szInv;
   GLuint i;

   if (ctx->Viewport._WindowMap.m[MAT_SZ] == 0.0)
      szInv = 1.0F;
   else
      szInv = 1.0F / ctx->Viewport._WindowMap.m[MAT_SZ];

   /*
    * Note: to compute eyeZ from the ndcZ we have to solve the following:
    *
    *        p[10] * eyeZ + p[14] * eyeW
    * ndcZ = ---------------------------
    *        p[11] * eyeZ + p[15] * eyeW
    *
    * Thus:
    *
    *        p[14] * eyeW - p[15] * eyeW * ndcZ
    * eyeZ = ----------------------------------
    *             p[11] * ndcZ - p[10]
    *
    * If we note:
    *    a) if using an orthographic projection, p[11] = 0 and p[15] = 1.
    *    b) if using a perspective projection, p[11] = -1 and p[15] = 0.
    *    c) we assume eyeW = 1 (not always true- glVertex4)
    *
    * Then we can simplify the calculation of eyeZ quite a bit.  We do
    * separate calculations for the orthographic and perspective cases below.
    * Note that we drop a negative sign or two since they don't matter.
    */

   switch (ctx->Fog.Mode) {
      case GL_LINEAR:
         {
            GLfloat fogEnd = ctx->Fog.End;
            GLfloat fogScale;
            if (ctx->Fog.Start == ctx->Fog.End)
               fogScale = 1.0;
            else
               fogScale = 1.0F / (ctx->Fog.End - ctx->Fog.Start);
            if (ortho) {
               for (i=0;i<n;i++) {
                  GLfloat ndcz = ((GLfloat) z[i] - tz) * szInv;
                  GLfloat eyez = (ndcz - p14) / p10;
                  if (eyez < 0.0)
                     eyez = -eyez;
                  fogFact[i] = (fogEnd - eyez) * fogScale;
               }
            }
            else {
               /* perspective */
               for (i=0;i<n;i++) {
                  GLfloat ndcz = ((GLfloat) z[i] - tz) * szInv;
                  GLfloat eyez = p14 / (ndcz + p10);
                  if (eyez < 0.0)
                     eyez = -eyez;
                  fogFact[i] = (fogEnd - eyez) * fogScale;
               }
            }
         }
	 break;
      case GL_EXP:
         if (ortho) {
            for (i=0;i<n;i++) {
               GLfloat ndcz = ((GLfloat) z[i] - tz) * szInv;
               GLfloat eyez = (ndcz - p14) / p10;
               if (eyez < 0.0)
                  eyez = -eyez;
               fogFact[i] = exp( -ctx->Fog.Density * eyez );
            }
         }
         else {
            /* perspective */
            for (i=0;i<n;i++) {
               GLfloat ndcz = ((GLfloat) z[i] - tz) * szInv;
               GLfloat eyez = p14 / (ndcz + p10);
               if (eyez < 0.0)
                  eyez = -eyez;
               fogFact[i] = exp( -ctx->Fog.Density * eyez );
            }
         }
	 break;
      case GL_EXP2:
         {
            GLfloat negDensitySquared = -ctx->Fog.Density * ctx->Fog.Density;
            if (ortho) {
               for (i=0;i<n;i++) {
                  GLfloat ndcz = ((GLfloat) z[i] - tz) * szInv;
                  GLfloat eyez = (ndcz - p14) / p10;
                  GLfloat tmp = negDensitySquared * eyez * eyez;
#if defined(__alpha__) || defined(__alpha)
                  /* XXX this underflow check may be needed for other systems*/
                  if (tmp < FLT_MIN_10_EXP)
                     tmp = FLT_MIN_10_EXP;
#endif
                  fogFact[i] = exp( tmp );
               }
            }
            else {
               /* perspective */
               for (i=0;i<n;i++) {
                  GLfloat ndcz = ((GLfloat) z[i] - tz) * szInv;
                  GLfloat eyez = p14 / (ndcz + p10);
                  GLfloat tmp = negDensitySquared * eyez * eyez;
#if defined(__alpha__) || defined(__alpha)
                  /* XXX this underflow check may be needed for other systems*/
                  if (tmp < FLT_MIN_10_EXP)
                     tmp = FLT_MIN_10_EXP;
#endif
                  fogFact[i] = exp( tmp );
               }
            }
         }
	 break;
      default:
         _mesa_problem(ctx, "Bad fog mode in compute_fog_factors_from_z");
         return;
   }
}


/*
 * Apply fog to an array of RGBA pixels.
 * Input:  n - number of pixels
 *         z - array of integer depth values
 *         red, green, blue, alpha - pixel colors
 * Output:  red, green, blue, alpha - fogged pixel colors
 */
void
_mesa_depth_fog_rgba_pixels( const GLcontext *ctx,
			     GLuint n, const GLdepth z[], GLchan rgba[][4] )
{
   GLfloat fogFact[PB_SIZE];
   ASSERT(n <= PB_SIZE);
   compute_fog_factors_from_z( ctx, n, z, fogFact );
   _mesa_fog_rgba_pixels( ctx, n, fogFact, rgba );
}


/*
 * Apply fog to an array of color index pixels.
 * Input:  n - number of pixels
 *         z - array of integer depth values
 *         index - pixel color indexes
 * Output:  index - fogged pixel color indexes
 */
void
_mesa_depth_fog_ci_pixels( const GLcontext *ctx,
                     GLuint n, const GLdepth z[], GLuint index[] )
{
   GLfloat fogFact[PB_SIZE];
   ASSERT(n <= PB_SIZE);
   compute_fog_factors_from_z( ctx, n, z, fogFact );
   _mesa_fog_ci_pixels( ctx, n, fogFact, index );
}

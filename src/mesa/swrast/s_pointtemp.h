/* $Id: s_pointtemp.h,v 1.18 2002/08/07 00:45:07 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  4.1
 *
 * Copyright (C) 1999-2002  Brian Paul   All Rights Reserved.
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
 * Point rendering template code.
 *
 * Set FLAGS = bitwise-OR of the following tokens:
 *
 *   RGBA = do rgba instead of color index
 *   SMOOTH = do antialiasing
 *   TEXTURE = do texture coords
 *   SPECULAR = do separate specular color
 *   LARGE = do points with diameter > 1 pixel
 *   ATTENUATE = compute point size attenuation
 *   SPRITE = GL_NV_point_sprite
 *
 * Notes: LARGE and ATTENUATE are exclusive of each other.
 *        TEXTURE requires RGBA
 *        SPECULAR requires TEXTURE
 */


/*
 * NOTES on antialiased point rasterization:
 *
 * Let d = distance of fragment center from vertex.
 * if d < rmin2 then
 *    fragment has 100% coverage
 * else if d > rmax2 then
 *    fragment has 0% coverage
 * else
 *    fragment has % coverage = (d - rmin2) / (rmax2 - rmin2)
 */



static void
NAME ( GLcontext *ctx, const SWvertex *vert )
{
#if FLAGS & TEXTURE
   GLuint u;
#endif
#if FLAGS & (ATTENUATE | LARGE | SMOOTH | SPRITE)
   GLfloat size;
#endif
#if FLAGS & ATTENUATE
   GLfloat alphaAtten;
#endif
#if (FLAGS & RGBA) && (FLAGS & SMOOTH)
   const GLchan red   = vert->color[0];
   const GLchan green = vert->color[1];
   const GLchan blue  = vert->color[2];
   const GLchan alpha = vert->color[3];
#endif

   struct sw_span span;

   /* Cull primitives with malformed coordinates.
    */
   {
      float tmp = vert->win[0] + vert->win[1];
      if (IS_INF_OR_NAN(tmp))
	 return;
   }

   INIT_SPAN(span, GL_POINT, 0, SPAN_FOG, SPAN_XY | SPAN_Z);
   span.fog = vert->fog;
   span.fogStep = 0.0;

#if (FLAGS & RGBA)
#if (FLAGS & SMOOTH)
   /* because we need per-fragment alpha values */
   span.arrayMask |= SPAN_RGBA;
#else
   /* same RGBA for all fragments */
   span.interpMask |= SPAN_RGBA;
   span.red = ChanToFixed(vert->color[0]);
   span.green = ChanToFixed(vert->color[1]);
   span.blue = ChanToFixed(vert->color[2]);
   span.alpha = ChanToFixed(vert->color[3]);
   span.redStep = span.greenStep = span.blueStep = span.alphaStep = 0;
#endif /*SMOOTH*/
#endif /*RGBA*/
#if FLAGS & SPECULAR
   span.interpMask |= SPAN_SPEC;
   span.specRed = ChanToFixed(vert->specular[0]);
   span.specGreen = ChanToFixed(vert->specular[1]);
   span.specBlue = ChanToFixed(vert->specular[2]);
   span.specRedStep = span.specGreenStep = span.specBlueStep = 0;
#endif
#if FLAGS & INDEX
   span.interpMask |= SPAN_INDEX;
   span.index = IntToFixed(vert->index);
   span.indexStep = 0;
#endif
#if FLAGS & TEXTURE
   /* but not used for sprite mode */
   span.interpMask |= SPAN_TEXTURE;
   for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {
      if (ctx->Texture.Unit[u]._ReallyEnabled) {
         const GLfloat q = vert->texcoord[u][3];
         const GLfloat invQ = (q == 0.0 || q == 1.0) ? 1.0 : (1.0 / q);
         span.tex[u][0] = vert->texcoord[u][0] * invQ;
         span.tex[u][1] = vert->texcoord[u][1] * invQ;
         span.tex[u][2] = vert->texcoord[u][2] * invQ;
         span.tex[u][3] = q;
         span.texStepX[u][0] = span.texStepY[u][0] = 0.0;
         span.texStepX[u][1] = span.texStepY[u][1] = 0.0;
         span.texStepX[u][2] = span.texStepY[u][2] = 0.0;
         span.texStepX[u][3] = span.texStepY[u][3] = 0.0;
      }
   }
#endif
#if FLAGS & SMOOTH
   span.arrayMask |= SPAN_COVERAGE;
#endif
#if FLAGS & SPRITE
   span.arrayMask |= SPAN_TEXTURE;
#endif

#if FLAGS & ATTENUATE
   if (vert->pointSize >= ctx->Point.Threshold) {
      size = MIN2(vert->pointSize, ctx->Point.MaxSize);
      alphaAtten = 1.0F;
   }
   else {
      GLfloat dsize = vert->pointSize / ctx->Point.Threshold;
      size = MAX2(ctx->Point.Threshold, ctx->Point.MinSize);
      alphaAtten = dsize * dsize;
   }
#elif FLAGS & (LARGE | SMOOTH | SPRITE)
   size = ctx->Point._Size;
#endif

#if FLAGS & (LARGE | ATTENUATE | SMOOTH | SPRITE)
   {
      GLint x, y;
      const GLfloat radius = 0.5F * size;
      const GLint z = (GLint) (vert->win[2]);
      GLuint count = 0;
#if FLAGS & SMOOTH
      const GLfloat rmin = radius - 0.7071F;  /* 0.7071 = sqrt(2)/2 */
      const GLfloat rmax = radius + 0.7071F;
      const GLfloat rmin2 = MAX2(0.0F, rmin * rmin);
      const GLfloat rmax2 = rmax * rmax;
      const GLfloat cscale = 1.0F / (rmax2 - rmin2);
      const GLint xmin = (GLint) (vert->win[0] - radius);
      const GLint xmax = (GLint) (vert->win[0] + radius);
      const GLint ymin = (GLint) (vert->win[1] - radius);
      const GLint ymax = (GLint) (vert->win[1] + radius);
#else
      /* non-smooth */
      GLint xmin, xmax, ymin, ymax;
      GLint iSize = (GLint) (size + 0.5F);
      GLint iRadius;
      iSize = MAX2(1, iSize);
      iRadius = iSize / 2;
      if (iSize & 1) {
         /* odd size */
         xmin = (GLint) (vert->win[0] - iRadius);
         xmax = (GLint) (vert->win[0] + iRadius);
         ymin = (GLint) (vert->win[1] - iRadius);
         ymax = (GLint) (vert->win[1] + iRadius);
      }
      else {
         /* even size */
         xmin = (GLint) vert->win[0] - iRadius + 1;
         xmax = xmin + iSize - 1;
         ymin = (GLint) vert->win[1] - iRadius + 1;
         ymax = ymin + iSize - 1;
      }
#endif /*SMOOTH*/

      (void) radius;
      for (y = ymin; y <= ymax; y++) {
         for (x = xmin; x <= xmax; x++) {
#if FLAGS & SPRITE
            GLuint u;
#endif
#if FLAGS & SMOOTH
            /* compute coverage */
            const GLfloat dx = x - vert->win[0] + 0.5F;
            const GLfloat dy = y - vert->win[1] + 0.5F;
            const GLfloat dist2 = dx * dx + dy * dy;
            if (dist2 < rmax2) {
               if (dist2 >= rmin2) {
                  /* compute partial coverage */
                  span.array->coverage[count] = 1.0F - (dist2 - rmin2) * cscale;
#if FLAGS & INDEX
                  span.array->coverage[count] *= 15.0; /* coverage in [0,15] */
#endif
               }
               else {
                  /* full coverage */
                  span.array->coverage[count] = 1.0F;
               }

               span.array->x[count] = x;
               span.array->y[count] = y;
               span.array->z[count] = z;

#if FLAGS & RGBA
               span.array->rgba[count][RCOMP] = red;
               span.array->rgba[count][GCOMP] = green;
               span.array->rgba[count][BCOMP] = blue;
#if FLAGS & ATTENUATE
               span.array->rgba[count][ACOMP] = (GLchan) (alpha * alphaAtten);
#else
               span.array->rgba[count][ACOMP] = alpha;
#endif /*ATTENUATE*/
#endif /*RGBA*/
               count++;
            } /*if*/
#else /*SMOOTH*/
            /* not smooth (square points) */
            span.array->x[count] = x;
            span.array->y[count] = y;
            span.array->z[count] = z;
#if FLAGS & SPRITE
            for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {
               if (ctx->Texture.Unit[u]._ReallyEnabled) {
                  if (ctx->Point.CoordReplace[u]) {
                     GLfloat s = 0.5F + (x + 0.5F - vert->win[0]) / size;
                     GLfloat t = 0.5F - (y + 0.5F - vert->win[1]) / size;
                     span.array->texcoords[u][count][0] = s;
                     span.array->texcoords[u][count][1] = t;
                     span.array->texcoords[u][count][3] = 1.0F;
                     if (ctx->Point.SpriteRMode == GL_ZERO)
                        span.array->texcoords[u][count][2] = 0.0F;
                     else if (ctx->Point.SpriteRMode == GL_S)
                        span.array->texcoords[u][count][2] = vert->texcoord[u][0];
                     else /* GL_R */
                        span.array->texcoords[u][count][2] = vert->texcoord[u][2];
                  }
                  else {
                     COPY_4V(span.array->texcoords[u][count], vert->texcoord[u]);
                  }
               }
            }
#endif /*SPRITE*/
            count++;
#endif /*SMOOTH*/
	 } /*for x*/
      } /*for y*/
      span.end = count;
   }

#else /* LARGE | ATTENUATE | SMOOTH | SPRITE */

   {
      /* size == 1 */
      span.array->x[0] = (GLint) vert->win[0];
      span.array->y[0] = (GLint) vert->win[1];
      span.array->z[0] = (GLint) vert->win[2];
      span.end = 1;
   }

#endif /* LARGE || ATTENUATE || SMOOTH */

   ASSERT(span.end > 0);

#if FLAGS & (TEXTURE | SPRITE)
   if (ctx->Texture._EnabledUnits)
      _mesa_write_texture_span(ctx, &span);
   else
      _mesa_write_rgba_span(ctx, &span);
#elif FLAGS & RGBA
   _mesa_write_rgba_span(ctx, &span);
#else
   _mesa_write_index_span(ctx, &span);
#endif
}


#undef FLAGS
#undef NAME

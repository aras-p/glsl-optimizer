/* $Id: s_pointtemp.h,v 1.13 2002/03/16 18:02:08 brianp Exp $ */

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
 *   SPRITE = GL_MESA_sprite_point
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
#if FLAGS & (ATTENUATE | LARGE | SMOOTH)
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

   INIT_SPAN(span);

   span.arrayMask |= (SPAN_XY | SPAN_Z);
   span.interpMask |= SPAN_FOG;
   span.fog = vert->fog;
   span.fogStep = 0.0;

#if (FLAGS & RGBA)
#if (FLAGS & SMOOTH)
   span.arrayMask |= SPAN_RGBA;
#else
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
#elif FLAGS & (LARGE | SMOOTH)
   size = ctx->Point._Size;
#endif

#if FLAGS & SPRITE
   {
      SWcontext *swctx = SWRAST_CONTEXT(ctx);
      const GLfloat radius = 0.5F * vert->pointSize; /* XXX threshold, alpha */
      SWvertex v0, v1, v2, v3;
      GLuint unit;

#if (FLAGS & RGBA) && (FLAGS & SMOOTH)
      (void) red;
      (void) green;
      (void) blue;
      (void) alpha;
#endif

      /* lower left corner */
      v0 = *vert;
      v0.win[0] -= radius;
      v0.win[1] -= radius;

      /* lower right corner */
      v1 = *vert;
      v1.win[0] += radius;
      v1.win[1] -= radius;

      /* upper right corner */
      v2 = *vert;
      v2.win[0] += radius;
      v2.win[1] += radius;

      /* upper left corner */
      v3 = *vert;
      v3.win[0] -= radius;
      v3.win[1] += radius;

      for (unit = 0; unit < ctx->Const.MaxTextureUnits; unit++) {
         if (ctx->Texture.Unit[unit]._ReallyEnabled) {
            v0.texcoord[unit][0] = 0.0;
            v0.texcoord[unit][1] = 0.0;
            v1.texcoord[unit][0] = 1.0;
            v1.texcoord[unit][1] = 0.0;
            v2.texcoord[unit][0] = 1.0;
            v2.texcoord[unit][1] = 1.0;
            v3.texcoord[unit][0] = 0.0;
            v3.texcoord[unit][1] = 1.0;
         }
      }

      /* XXX if radius < threshold, attenuate alpha? */

      /* XXX need to implement clipping!!! */

      /* render */
      swctx->Triangle(ctx, &v0, &v1, &v2);
      swctx->Triangle(ctx, &v0, &v2, &v3);
   }

#elif FLAGS & (LARGE | ATTENUATE | SMOOTH)

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
#if FLAGS & SMOOTH
            /* compute coverage */
            const GLfloat dx = x - vert->win[0] + 0.5F;
            const GLfloat dy = y - vert->win[1] + 0.5F;
            const GLfloat dist2 = dx * dx + dy * dy;
            if (dist2 < rmax2) {
               if (dist2 >= rmin2) {
                  /* compute partial coverage */
                  span.coverage[count] = 1.0F - (dist2 - rmin2) * cscale;
#if FLAGS & INDEX
                  span.coverage[count] *= 15.0; /* coverage in [0,15] */
#endif
               }
               else {
                  /* full coverage */
                  span.coverage[count] = 1.0F;
               }

               span.xArray[count] = x;
               span.yArray[count] = y;
               span.zArray[count] = z;

#if FLAGS & RGBA
               span.color.rgba[count][RCOMP] = red;
               span.color.rgba[count][GCOMP] = green;
               span.color.rgba[count][BCOMP] = blue;
#if FLAGS & ATTENUATE
               span.color.rgba[count][ACOMP] = (GLchan) (alpha * alphaAtten);
#else
               span.color.rgba[count][ACOMP] = alpha;
#endif /*ATTENUATE*/
#endif /*RGBA*/
               count++;
            } /*if*/
#else /*SMOOTH*/
            /* not smooth (square points */
            span.xArray[count] = x;
            span.yArray[count] = y;
            span.zArray[count] = z;
            count++;
#endif /*SMOOTH*/
	 } /*for x*/
      } /*for y*/
      span.end = count;
   }

#else /* LARGE || ATTENUATE || SMOOTH*/

   {
      /* size == 1 */
      span.xArray[0] = (GLint) vert->win[0];
      span.yArray[0] = (GLint) vert->win[1];
      span.zArray[0] = (GLint) vert->win[2];
      span.end = 1;
   }

#endif /* LARGE || ATTENUATE || SMOOTH */

   ASSERT(span.end > 0);

#if FLAGS & TEXTURE
   if (ctx->Texture._ReallyEnabled)
      _mesa_write_texture_span(ctx, &span, GL_POINT);
   else
      _mesa_write_rgba_span(ctx, &span, GL_POINT);
#elif FLAGS & RGBA
   _mesa_write_rgba_span(ctx, &span, GL_POINT);
#else
   _mesa_write_index_span(ctx, &span, GL_POINT);
#endif
}


#undef FLAGS
#undef NAME

/* $Id: s_pointtemp.h,v 1.8 2001/05/17 09:32:17 keithw Exp $ */

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
 *    fragement has % coverage = (d - rmin2) / (rmax2 - rmin2)
 */



static void
NAME ( GLcontext *ctx, const SWvertex *vert )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   struct pixel_buffer *PB = swrast->PB;

   const GLint z = (GLint) (vert->win[2]);

#if FLAGS & RGBA
   const GLint red   = vert->color[0];
   const GLint green = vert->color[1];
   const GLint blue  = vert->color[2];
   GLint alpha = vert->color[3];
#if FLAGS & SPECULAR
   const GLint sRed   = vert->specular[0];
   const GLint sGreen = vert->specular[1];
   const GLint sBlue  = vert->specular[2];
#endif
#else
   GLint index = vert->index;
#endif
#if FLAGS & (ATTENUATE | LARGE | SMOOTH)
   GLfloat size;
#endif
#if FLAGS & ATTENUATE
   GLfloat alphaAtten;
#endif

#if FLAGS & TEXTURE
   GLfloat texcoord[MAX_TEXTURE_UNITS][4];
   GLuint u;
   for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {
      if (ctx->Texture.Unit[u]._ReallyEnabled) {
         if (vert->texcoord[u][3] != 1.0 && vert->texcoord[u][3] != 0.0) {
            texcoord[u][0] = vert->texcoord[u][0] / vert->texcoord[u][3];
            texcoord[u][1] = vert->texcoord[u][1] / vert->texcoord[u][3];
            texcoord[u][2] = vert->texcoord[u][2] / vert->texcoord[u][3];
         }
         else {
            texcoord[u][0] = vert->texcoord[u][0];
            texcoord[u][1] = vert->texcoord[u][1];
            texcoord[u][2] = vert->texcoord[u][2];
         }
      }
   }
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
      const GLfloat radius = 0.5 * vert->pointSize;  /* XXX threshold, alpha */
      SWvertex v0, v1, v2, v3;
      GLuint unit;

      (void) red;
      (void) green;
      (void) blue;
      (void) alpha;
      (void) z;

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
#if FLAGS & SMOOTH
      const GLfloat rmin = radius - 0.7071F;  /* 0.7071 = sqrt(2)/2 */
      const GLfloat rmax = radius + 0.7071F;
      const GLfloat rmin2 = MAX2(0.0, rmin * rmin);
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
#endif
      (void) radius;

      for (y = ymin; y <= ymax; y++) {
         for (x = xmin; x <= xmax; x++) {
#if FLAGS & SMOOTH
            /* compute coverage */
            const GLfloat dx = x - vert->win[0] + 0.5F;
            const GLfloat dy = y - vert->win[1] + 0.5F;
            const GLfloat dist2 = dx * dx + dy * dy;
            if (dist2 < rmax2) {
#if FLAGS & RGBA
               alpha = vert->color[3];
#endif
               if (dist2 >= rmin2) {
                  /* compute partial coverage */
                  PB_COVERAGE(PB, 1.0F - (dist2 - rmin2) * cscale);
	       }
               else {
                  /* full coverage */
                  PB_COVERAGE(PB, 1.0F);
               }

#endif /* SMOOTH */

#if ((FLAGS & (ATTENUATE | RGBA)) == (ATTENUATE | RGBA))
	       alpha = (GLint) (alpha * alphaAtten);
#endif

#if FLAGS & SPECULAR
               PB_WRITE_MULTITEX_SPEC_PIXEL(PB, x, y, z, vert->fog,
                                            red, green, blue, alpha,
                                            sRed, sGreen, sBlue,
                                            texcoord);
#elif FLAGS & TEXTURE
	       if (ctx->Texture._ReallyEnabled > TEXTURE0_ANY) {
		  PB_WRITE_MULTITEX_PIXEL(PB, x, y, z, vert->fog,
					  red, green, blue, alpha,
					  texcoord);
	       }
	       else if (ctx->Texture._ReallyEnabled) {
		  PB_WRITE_TEX_PIXEL(PB, x,y,z, vert->fog,
				     red, green, blue, alpha,
				     texcoord[0][0],
				     texcoord[0][1],
				     texcoord[0][2]);
	       }
               else {
                  PB_WRITE_RGBA_PIXEL(PB, x, y, z, vert->fog,
                                      red, green, blue, alpha);
               }
#elif FLAGS & RGBA
	       PB_WRITE_RGBA_PIXEL(PB, x, y, z, vert->fog,
                                   red, green, blue, alpha);
#else /* color index */
               PB_WRITE_CI_PIXEL(PB, x, y, z, vert->fog, index);
#endif
#if FLAGS & SMOOTH
	    }
#endif
	 }
      }

#if FLAGS & SMOOTH
      PB->haveCoverage = GL_TRUE;
#endif

      PB_CHECK_FLUSH(ctx,PB);
   }

#else /* LARGE || ATTENUATE || SMOOTH*/

   {
      /* size == 1 */
      GLint x = (GLint) vert->win[0];
      GLint y = (GLint) vert->win[1];
#if ((FLAGS & (SPECULAR | TEXTURE)) == (SPECULAR | TEXTURE))
      PB_WRITE_MULTITEX_SPEC_PIXEL(PB, x, y, z, vert->fog,
                                   red, green, blue, alpha,
                                   sRed, sGreen, sBlue,
                                   texcoord);
#elif FLAGS & TEXTURE
      if (ctx->Texture._ReallyEnabled > TEXTURE0_ANY) {
         PB_WRITE_MULTITEX_PIXEL(PB, x, y, z, vert->fog,
                                 red, green, blue, alpha, texcoord );
      }
      else {
         PB_WRITE_TEX_PIXEL(PB, x, y, z, vert->fog,
                            red, green, blue, alpha,
                            texcoord[0][0], texcoord[0][1], texcoord[0][2]);
      }
#elif FLAGS & RGBA
      /* rgba size 1 point */
      alpha = vert->color[3];
      PB_WRITE_RGBA_PIXEL(PB, x, y, z, vert->fog, red, green, blue, alpha);
#else
      /* color index size 1 point */
      PB_WRITE_CI_PIXEL(PB, x, y, z, vert->fog, index);
#endif
   }
#endif /* LARGE || ATTENUATE || SMOOTH */

   PB_CHECK_FLUSH(ctx, PB);
}


#undef FLAGS
#undef NAME

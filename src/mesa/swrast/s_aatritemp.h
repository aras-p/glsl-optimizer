/* $Id: s_aatritemp.h,v 1.19 2001/07/13 20:07:37 brianp Exp $ */

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
 * Antialiased Triangle Rasterizer Template
 *
 * This file is #include'd to generate custom AA triangle rasterizers.
 * NOTE: this code hasn't been optimized yet.  That'll come after it
 * works correctly.
 *
 * The following macros may be defined to indicate what auxillary information
 * must be copmuted across the triangle:
 *    DO_Z         - if defined, compute Z values
 *    DO_RGBA      - if defined, compute RGBA values
 *    DO_INDEX     - if defined, compute color index values
 *    DO_SPEC      - if defined, compute specular RGB values
 *    DO_TEX       - if defined, compute unit 0 STRQ texcoords
 *    DO_MULTITEX  - if defined, compute all unit's STRQ texcoords
 */

/*void triangle( GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2, GLuint pv )*/
{
   const GLfloat *p0 = v0->win;
   const GLfloat *p1 = v1->win;
   const GLfloat *p2 = v2->win;
   const SWvertex *vMin, *vMid, *vMax;
   GLfloat xMin, yMin, xMid, yMid, xMax, yMax;
   GLfloat majDx, majDy, botDx, botDy, topDx, topDy;
   GLfloat area;
   GLboolean majorOnLeft;
   GLfloat bf = SWRAST_CONTEXT(ctx)->_backface_sign;

#ifdef DO_Z
   GLfloat zPlane[4];
   GLdepth z[MAX_WIDTH];
#endif
#ifdef DO_FOG
   GLfloat fogPlane[4];
   GLfloat fog[MAX_WIDTH];
#else
   GLfloat *fog = NULL;
#endif
#ifdef DO_RGBA
   GLfloat rPlane[4], gPlane[4], bPlane[4], aPlane[4];
   DEFMARRAY(GLchan, rgba, MAX_WIDTH, 4);  /* mac 32k limitation */
#endif
#ifdef DO_INDEX
   GLfloat iPlane[4];
   GLuint index[MAX_WIDTH];
   GLint icoverageSpan[MAX_WIDTH];
   GLfloat coverageSpan[MAX_WIDTH];
#else
   GLfloat coverageSpan[MAX_WIDTH];
#endif
#ifdef DO_SPEC
   GLfloat srPlane[4], sgPlane[4], sbPlane[4];
   DEFMARRAY(GLchan, spec, MAX_WIDTH, 4);
#endif
#ifdef DO_TEX
   GLfloat sPlane[4], tPlane[4], uPlane[4], vPlane[4];
   GLfloat texWidth, texHeight;
   DEFARRAY(GLfloat, s, MAX_WIDTH);  /* mac 32k limitation */
   DEFARRAY(GLfloat, t, MAX_WIDTH);
   DEFARRAY(GLfloat, u, MAX_WIDTH);
   DEFARRAY(GLfloat, lambda, MAX_WIDTH);
#elif defined(DO_MULTITEX)
   GLfloat sPlane[MAX_TEXTURE_UNITS][4];
   GLfloat tPlane[MAX_TEXTURE_UNITS][4];
   GLfloat uPlane[MAX_TEXTURE_UNITS][4];
   GLfloat vPlane[MAX_TEXTURE_UNITS][4];
   GLfloat texWidth[MAX_TEXTURE_UNITS], texHeight[MAX_TEXTURE_UNITS];
   DEFMARRAY(GLfloat, s, MAX_TEXTURE_UNITS, MAX_WIDTH);  /* mac 32k limit */
   DEFMARRAY(GLfloat, t, MAX_TEXTURE_UNITS, MAX_WIDTH);
   DEFMARRAY(GLfloat, u, MAX_TEXTURE_UNITS, MAX_WIDTH);
   DEFMARRAY(GLfloat, lambda, MAX_TEXTURE_UNITS, MAX_WIDTH);
#endif

#ifdef DO_RGBA
   CHECKARRAY(rgba, return);  /* mac 32k limitation */
#endif
#ifdef DO_SPEC
   CHECKARRAY(spec, return);
#endif
#if defined(DO_TEX) || defined(DO_MULTITEX)
   CHECKARRAY(s, return);
   CHECKARRAY(t, return);
   CHECKARRAY(u, return);
   CHECKARRAY(lambda, return);
#endif

   /* determine bottom to top order of vertices */
   {
      GLfloat y0 = v0->win[1];
      GLfloat y1 = v1->win[1];
      GLfloat y2 = v2->win[1];
      if (y0 <= y1) {
	 if (y1 <= y2) {
	    vMin = v0;   vMid = v1;   vMax = v2;   /* y0<=y1<=y2 */
	 }
	 else if (y2 <= y0) {
	    vMin = v2;   vMid = v0;   vMax = v1;   /* y2<=y0<=y1 */
	 }
	 else {
	    vMin = v0;   vMid = v2;   vMax = v1;  bf = -bf; /* y0<=y2<=y1 */
	 }
      }
      else {
	 if (y0 <= y2) {
	    vMin = v1;   vMid = v0;   vMax = v2;  bf = -bf; /* y1<=y0<=y2 */
	 }
	 else if (y2 <= y1) {
	    vMin = v2;   vMid = v1;   vMax = v0;  bf = -bf; /* y2<=y1<=y0 */
	 }
	 else {
	    vMin = v1;   vMid = v2;   vMax = v0;   /* y1<=y2<=y0 */
	 }
      }
   }

   xMin = vMin->win[0];  yMin = vMin->win[1];
   xMid = vMid->win[0];  yMid = vMid->win[1];
   xMax = vMax->win[0];  yMax = vMax->win[1];

   /* the major edge is between the top and bottom vertices */
   majDx = xMax - xMin;
   majDy = yMax - yMin;
   /* the bottom edge is between the bottom and mid vertices */
   botDx = xMid - xMin;
   botDy = yMid - yMin;
   /* the top edge is between the top and mid vertices */
   topDx = xMax - xMid;
   topDy = yMax - yMid;

   /* compute clockwise / counter-clockwise orientation and do BF culling */
   area = majDx * botDy - botDx * majDy;
   /* Do backface culling */
   if (area * bf < 0 || area * area < .0025)
      return;
   majorOnLeft = (GLboolean) (area < 0.0F);

#ifndef DO_OCCLUSION_TEST
   ctx->OcclusionResult = GL_TRUE;
#endif

   assert(majDy > 0.0F);

   /* Plane equation setup:
    * We evaluate plane equations at window (x,y) coordinates in order
    * to compute color, Z, fog, texcoords, etc.  This isn't terribly
    * efficient but it's easy and reliable.  It also copes with computing
    * interpolated data just outside the triangle's edges.
    */
#ifdef DO_Z
   compute_plane(p0, p1, p2, p0[2], p1[2], p2[2], zPlane);
#endif
#ifdef DO_FOG
   compute_plane(p0, p1, p2, v0->fog, v1->fog, v2->fog, fogPlane);
#endif
#ifdef DO_RGBA
   if (ctx->Light.ShadeModel == GL_SMOOTH) {
      compute_plane(p0, p1, p2, v0->color[0], v1->color[0], v2->color[0], rPlane);
      compute_plane(p0, p1, p2, v0->color[1], v1->color[1], v2->color[1], gPlane);
      compute_plane(p0, p1, p2, v0->color[2], v1->color[2], v2->color[2], bPlane);
      compute_plane(p0, p1, p2, v0->color[3], v1->color[3], v2->color[3], aPlane);
   }
   else {
      constant_plane(v2->color[RCOMP], rPlane);
      constant_plane(v2->color[GCOMP], gPlane);
      constant_plane(v2->color[BCOMP], bPlane);
      constant_plane(v2->color[ACOMP], aPlane);
   }
#endif
#ifdef DO_INDEX
   if (ctx->Light.ShadeModel == GL_SMOOTH) {
      compute_plane(p0, p1, p2, v0->index,
                    v1->index, v2->index, iPlane);
   }
   else {
      constant_plane(v2->index, iPlane);
   }
#endif
#ifdef DO_SPEC
   if (ctx->Light.ShadeModel == GL_SMOOTH) {
      compute_plane(p0, p1, p2, v0->specular[0], v1->specular[0], v2->specular[0],srPlane);
      compute_plane(p0, p1, p2, v0->specular[1], v1->specular[1], v2->specular[1],sgPlane);
      compute_plane(p0, p1, p2, v0->specular[2], v1->specular[2], v2->specular[2],sbPlane);
   }
   else {
      constant_plane(v2->specular[RCOMP], srPlane);
      constant_plane(v2->specular[GCOMP], sgPlane);
      constant_plane(v2->specular[BCOMP], sbPlane);
   }
#endif
#ifdef DO_TEX
   {
      const struct gl_texture_object *obj = ctx->Texture.Unit[0]._Current;
      const struct gl_texture_image *texImage = obj->Image[obj->BaseLevel];
      const GLfloat invW0 = v0->win[3];
      const GLfloat invW1 = v1->win[3];
      const GLfloat invW2 = v2->win[3];
      const GLfloat s0 = v0->texcoord[0][0] * invW0;
      const GLfloat s1 = v1->texcoord[0][0] * invW1;
      const GLfloat s2 = v2->texcoord[0][0] * invW2;
      const GLfloat t0 = v0->texcoord[0][1] * invW0;
      const GLfloat t1 = v1->texcoord[0][1] * invW1;
      const GLfloat t2 = v2->texcoord[0][1] * invW2;
      const GLfloat r0 = v0->texcoord[0][2] * invW0;
      const GLfloat r1 = v1->texcoord[0][2] * invW1;
      const GLfloat r2 = v2->texcoord[0][2] * invW2;
      const GLfloat q0 = v0->texcoord[0][3] * invW0;
      const GLfloat q1 = v1->texcoord[0][3] * invW1;
      const GLfloat q2 = v2->texcoord[0][3] * invW2;
      compute_plane(p0, p1, p2, s0, s1, s2, sPlane);
      compute_plane(p0, p1, p2, t0, t1, t2, tPlane);
      compute_plane(p0, p1, p2, r0, r1, r2, uPlane);
      compute_plane(p0, p1, p2, q0, q1, q2, vPlane);
      texWidth = (GLfloat) texImage->Width;
      texHeight = (GLfloat) texImage->Height;
   }
#elif defined(DO_MULTITEX)
   {
      GLuint u;
      for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {
         if (ctx->Texture.Unit[u]._ReallyEnabled) {
            const struct gl_texture_object *obj = ctx->Texture.Unit[u]._Current;
            const struct gl_texture_image *texImage = obj->Image[obj->BaseLevel];
            const GLfloat invW0 = v0->win[3];
            const GLfloat invW1 = v1->win[3];
            const GLfloat invW2 = v2->win[3];
            const GLfloat s0 = v0->texcoord[u][0] * invW0;
            const GLfloat s1 = v1->texcoord[u][0] * invW1;
            const GLfloat s2 = v2->texcoord[u][0] * invW2;
            const GLfloat t0 = v0->texcoord[u][1] * invW0;
            const GLfloat t1 = v1->texcoord[u][1] * invW1;
            const GLfloat t2 = v2->texcoord[u][1] * invW2;
            const GLfloat r0 = v0->texcoord[u][2] * invW0;
            const GLfloat r1 = v1->texcoord[u][2] * invW1;
            const GLfloat r2 = v2->texcoord[u][2] * invW2;
            const GLfloat q0 = v0->texcoord[u][3] * invW0;
            const GLfloat q1 = v1->texcoord[u][3] * invW1;
            const GLfloat q2 = v2->texcoord[u][3] * invW2;
            compute_plane(p0, p1, p2, s0, s1, s2, sPlane[u]);
            compute_plane(p0, p1, p2, t0, t1, t2, tPlane[u]);
            compute_plane(p0, p1, p2, r0, r1, r2, uPlane[u]);
            compute_plane(p0, p1, p2, q0, q1, q2, vPlane[u]);
            texWidth[u]  = (GLfloat) texImage->Width;
            texHeight[u] = (GLfloat) texImage->Height;
         }
      }
   }
#endif

   /* Begin bottom-to-top scan over the triangle.
    * The long edge will either be on the left or right side of the
    * triangle.  We always scan from the long edge toward the shorter
    * edges, stopping when we find that coverage = 0.  If the long edge
    * is on the left we scan left-to-right.  Else, we scan right-to-left.
    */
   {
      const GLint iyMin = (GLint) yMin;
      const GLint iyMax = (GLint) yMax + 1;
      /* upper edge and lower edge derivatives */
      const GLfloat topDxDy = (topDy != 0.0F) ? topDx / topDy : 0.0F;
      const GLfloat botDxDy = (botDy != 0.0F) ? botDx / botDy : 0.0F;
      const GLfloat *pA, *pB, *pC;
      const GLfloat majDxDy = majDx / majDy;
      const GLfloat absMajDxDy = FABSF(majDxDy);
      const GLfloat absTopDxDy = FABSF(topDxDy);
      const GLfloat absBotDxDy = FABSF(botDxDy);
#if 0
      GLfloat xMaj = xMin - (yMin - (GLfloat) iyMin) * majDxDy;
      GLfloat xBot = xMaj;
      GLfloat xTop = xMid - (yMid - (GLint) yMid) * topDxDy;
#else
      GLfloat xMaj;
      GLfloat xBot;
      GLfloat xTop;
#endif
      GLint iy;
      GLint k;

      /* pA, pB, pC are the vertices in counter-clockwise order */
      if (majorOnLeft) {
         pA = vMin->win;
         pB = vMid->win;
         pC = vMax->win;
         xMaj = xMin - absMajDxDy - 1.0;
         xBot = xMin + absBotDxDy + 1.0;
         xTop = xMid + absTopDxDy + 1.0;
      }
      else {
         pA = vMin->win;
         pB = vMax->win;
         pC = vMid->win;
         xMaj = xMin + absMajDxDy + 1.0;
         xBot = xMin - absBotDxDy - 1.0;
         xTop = xMid - absTopDxDy - 1.0;
      }

      /* Scan from bottom to top */
      for (iy = iyMin; iy < iyMax; iy++, xMaj += majDxDy) {
         GLint ix, i, j, len;
         GLint iRight, iLeft;
         GLfloat coverage = 0.0F;

         if (majorOnLeft) {
            iLeft = (GLint) (xMaj + 0.0);
            /* compute right */
            if (iy <= yMid) {
               /* we're in the lower part */
               iRight = (GLint) (xBot + 0.0);
               xBot += botDxDy;
            }
            else {
               /* we're in the upper part */
               iRight = (GLint) (xTop + 0.0);
               xTop += topDxDy;
            }
         }
         else {
            iRight = (GLint) (xMaj + 0.0);
            /* compute left */
            if (iy <= yMid) {
               /* we're in the lower part */
               iLeft = (GLint) (xBot - 0.0);
               xBot += botDxDy;
            }
            else {
               /* we're in the upper part */
               iLeft = (GLint) (xTop - 0.0);
               xTop += topDxDy;
            }
         }

#ifdef DEBUG
         for (i = 0; i < MAX_WIDTH; i++) {
            coverageSpan[i] = -1.0;
         }
#endif

         if (iLeft < 0)
            iLeft = 0;
         if (iRight >= ctx->DrawBuffer->_Xmax)
            iRight = ctx->DrawBuffer->_Xmax - 1;

         /*printf("%d: iLeft = %d  iRight = %d\n", iy, iLeft, iRight);*/

         /* The pixels at y in [iLeft, iRight] (inclusive) are candidates */

         /* scan left to right until we hit 100% coverage or the right edge */
         i = iLeft;
         while (i < iRight) {
            coverage = compute_coveragef(pA, pB, pC, i, iy);
            if (coverage == 0.0F) {
               if (i == iLeft)
                  iLeft++;  /* skip zero coverage pixels */
               else {
                  iRight = i;
                  i--;
                  break;  /* went past right edge */
               }
            }
            else {
               coverageSpan[i - iLeft] = coverage;
               if (coverage == 1.0F)
                  break;
            }
            i++;
         }

         assert(coverageSpan[i-iLeft] > 0.0 || iLeft == iRight);

         assert(i == iRight || coverage == 1.0 || coverage == 0.0);

         /* scan right to left until we hit 100% coverage or the left edge */
         j = iRight;
         assert(j - iLeft >= 0);
         while (1) {
            coverage = compute_coveragef(pA, pB, pC, j, iy);
            if (coverage == 0.0F) {
               if (j == iRight && j > i)
                  iRight--;  /* skip zero coverage pixels */
               else
                  break;
            }
            else {
               if (j <= i)
                  break;
               assert(j - iLeft >= 0);
               coverageSpan[j - iLeft] = coverage;
               if (coverage == 1.0F)
                  break;
            }
            /*printf("%d: coverage[%d]' = %g\n", iy, j-iLeft, coverage);*/
            j--;
         }

         assert(coverageSpan[j-iLeft] > 0.0 || iRight <= iLeft);

         printf("iLeft=%d i=%d j=%d iRight=%d\n", iLeft, i, j, iRight);

         assert(iLeft >= 0);
         assert(iLeft < ctx->DrawBuffer->_Xmax);
         assert(iRight >= 0);
         assert(iRight < ctx->DrawBuffer->_Xmax);
         assert(iRight >= iLeft);


         /* any pixels left in between must have 100% coverage */
         k = i + 1;
         while (k < j) {
            coverageSpan[k - iLeft] = 1.0F;
            k++;
         }

         len = iRight - iLeft;
         /*printf("len = %d\n", len);*/
         assert(len >= 0);
         assert(len < MAX_WIDTH);

         if (len == 0)
            continue;

#ifdef DEBUG
         for (k = 0; k < len; k++) {
            assert(coverageSpan[k] > 0.0);
         }
#endif

         /*
          * Compute color, texcoords, etc for the span
          */
         {
            const GLfloat cx = iLeft + 0.5F, cy = iy + 0.5F;
#ifdef DO_Z
            GLfloat zFrag = solve_plane(cx, cy, zPlane);
            const GLfloat zStep = -zPlane[0] / zPlane[2];
#endif
#ifdef DO_FOG
            GLfloat fogFrag = solve_plane(cx, cy, fogPlane);
            const GLfloat fogStep = -fogPlane[0] / fogPlane[2];
#endif
#ifdef DO_RGBA
            /* to do */
#endif
#ifdef DO_INDEX
            /* to do */
#endif
#ifdef DO_SPEC
            /* to do */
#endif
#ifdef DO_TEX
            GLfloat sFrag = solve_plane(cx, cy, sPlane);
            GLfloat tFrag = solve_plane(cx, cy, tPlane);
            GLfloat uFrag = solve_plane(cx, cy, uPlane);
            GLfloat vFrag = solve_plane(cx, cy, vPlane);
            const GLfloat sStep = -sPlane[0] / sPlane[2];
            const GLfloat tStep = -tPlane[0] / tPlane[2];
            const GLfloat uStep = -uPlane[0] / uPlane[2];
            const GLfloat vStep = -vPlane[0] / vPlane[2];
#elif defined(DO_MULTITEX)
            /* to do */
#endif

            for (ix = iLeft; ix < iRight; ix++) {
               const GLint k = ix - iLeft;
               const GLfloat cx = ix + 0.5F, cy = iy + 0.5F;

#ifdef DO_Z
               z[k] = zFrag;  zFrag += zStep;
#endif
#ifdef DO_FOG
               fog[k] = fogFrag;  fogFrag += fogStep;
#endif
#ifdef DO_RGBA
               rgba[k][RCOMP] = solve_plane_chan(cx, cy, rPlane);
               rgba[k][GCOMP] = solve_plane_chan(cx, cy, gPlane);
               rgba[k][BCOMP] = solve_plane_chan(cx, cy, bPlane);
               rgba[k][ACOMP] = solve_plane_chan(cx, cy, aPlane);
#endif
#ifdef DO_INDEX
               index[k] = (GLint) solve_plane(cx, cy, iPlane);
#endif
#ifdef DO_SPEC
               spec[k][RCOMP] = solve_plane_chan(cx, cy, srPlane);
               spec[k][GCOMP] = solve_plane_chan(cx, cy, sgPlane);
               spec[k][BCOMP] = solve_plane_chan(cx, cy, sbPlane);
#endif
#ifdef DO_TEX
               s[k] = sFrag / vFrag;
               t[k] = tFrag / vFrag;
               u[k] = uFrag / vFrag;
               lambda[k] = compute_lambda(sPlane, tPlane, 1.0F / vFrag,
                                          texWidth, texHeight);
               sFrag += sStep;
               tFrag += tStep;
               uFrag += uStep;
               vFrag += vStep;
#elif defined(DO_MULTITEX)
               {
                  GLuint unit;
                  for (unit = 0; unit < ctx->Const.MaxTextureUnits; unit++) {
                     if (ctx->Texture.Unit[unit]._ReallyEnabled) {
                        GLfloat invQ = solve_plane_recip(cx, cy, vPlane[unit]);
                        s[unit][k] = solve_plane(cx, cy, sPlane[unit]) * invQ;
                        t[unit][k] = solve_plane(cx, cy, tPlane[unit]) * invQ;
                        u[unit][k] = solve_plane(cx, cy, uPlane[unit]) * invQ;
                        lambda[unit][k] = compute_lambda(sPlane[unit],
                          tPlane[unit], invQ, texWidth[unit], texHeight[unit]);
                     }
                  }
               }
#endif
            } /* for ix */
         }

         /*
          * Write/process the span of fragments.
          */
#ifdef DO_MULTITEX
         _mesa_write_multitexture_span(ctx, len, iLeft, iy, z, fog,
                                       (const GLfloat (*)[MAX_WIDTH]) s,
                                       (const GLfloat (*)[MAX_WIDTH]) t,
                                       (const GLfloat (*)[MAX_WIDTH]) u,
                                       (GLfloat (*)[MAX_WIDTH]) lambda,
                                       rgba,
#  ifdef DO_SPEC
                                       (const GLchan (*)[4]) spec,
#  else
                                       NULL,
#  endif
                                       coverageSpan,  GL_POLYGON);
#elif defined(DO_TEX)
         _mesa_write_texture_span(ctx, len, iLeft, iy, z, fog,
                                  s, t, u, lambda, rgba,
#  ifdef DO_SPEC
                                  (const GLchan (*)[4]) spec,
#  else
                                  NULL,
#  endif
                                  coverageSpan, GL_POLYGON);
#elif defined(DO_RGBA)
         _mesa_write_rgba_span(ctx, len, iLeft, iy, z, fog, rgba,
                               coverageSpan, GL_POLYGON);
#elif defined(DO_INDEX)
         _mesa_write_index_span(ctx, len, iLeft, iy, z, fog, index,
                                icoverageSpan, GL_POLYGON);
#endif

      } /* for iy */
   }


#ifdef DO_RGBA
   UNDEFARRAY(rgba);  /* mac 32k limitation */
#endif
#ifdef DO_SPEC
   UNDEFARRAY(spec);
#endif
#if defined(DO_TEX) || defined(DO_MULTITEX)
   UNDEFARRAY(s);
   UNDEFARRAY(t);
   UNDEFARRAY(u);
   UNDEFARRAY(lambda);
#endif
}


#ifdef DO_Z
#undef DO_Z
#endif

#ifdef DO_FOG
#undef DO_FOG
#endif

#ifdef DO_RGBA
#undef DO_RGBA
#endif

#ifdef DO_INDEX
#undef DO_INDEX
#endif

#ifdef DO_SPEC
#undef DO_SPEC
#endif

#ifdef DO_TEX
#undef DO_TEX
#endif

#ifdef DO_MULTITEX
#undef DO_MULTITEX
#endif

#ifdef DO_OCCLUSION_TEST
#undef DO_OCCLUSION_TEST
#endif

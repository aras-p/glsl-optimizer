/* $Id: s_aatritemp.h,v 1.18 2001/06/05 21:41:05 brianp Exp $ */

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
   GLint iyMin, iyMax;
   GLfloat yMin, yMax;
   GLboolean ltor;
   GLfloat majDx, majDy;  /* major (i.e. long) edge dx and dy */

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
   GLfloat bf = SWRAST_CONTEXT(ctx)->_backface_sign;

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

   majDx = vMax->win[0] - vMin->win[0];
   majDy = vMax->win[1] - vMin->win[1];

   {
      const GLfloat botDx = vMid->win[0] - vMin->win[0];
      const GLfloat botDy = vMid->win[1] - vMin->win[1];
      const GLfloat area = majDx * botDy - botDx * majDy;
      ltor = (GLboolean) (area < 0.0F);
      /* Do backface culling */
      if (area * bf < 0 || area * area < .0025)
	 return;
   }

#ifndef DO_OCCLUSION_TEST
   ctx->OcclusionResult = GL_TRUE;
#endif

   /* Plane equation setup:
    * We evaluate plane equations at window (x,y) coordinates in order
    * to compute color, Z, fog, texcoords, etc.  This isn't terribly
    * efficient but it's easy and reliable.
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
   yMin = vMin->win[1];
   yMax = vMax->win[1];
   iyMin = (GLint) yMin;
   iyMax = (GLint) yMax + 1;

   if (ltor) {
      /* scan left to right */
      const GLfloat *pMin = vMin->win;
      const GLfloat *pMid = vMid->win;
      const GLfloat *pMax = vMax->win;
      const GLfloat dxdy = majDx / majDy;
      const GLfloat xAdj = dxdy < 0.0F ? -dxdy : 0.0F;
      GLfloat x = pMin[0] - (yMin - iyMin) * dxdy;
      GLint iy;
      for (iy = iyMin; iy < iyMax; iy++, x += dxdy) {
         GLint ix, startX = (GLint) (x - xAdj);
         GLuint count, n;
         GLfloat coverage = 0.0F;
         /* skip over fragments with zero coverage */
         while (startX < MAX_WIDTH) {
            coverage = compute_coveragef(pMin, pMid, pMax, startX, iy);
            if (coverage > 0.0F)
               break;
            startX++;
         }

         /* enter interior of triangle */
         ix = startX;
         count = 0;
         while (coverage > 0.0F) {
            /* (cx,cy) = center of fragment */
            const GLfloat cx = ix + 0.5F, cy = iy + 0.5F;
#ifdef DO_INDEX
            icoverageSpan[count] = compute_coveragei(pMin, pMid, pMax, ix, iy);
#else
            coverageSpan[count] = coverage;
#endif
#ifdef DO_Z
            z[count] = (GLdepth) solve_plane(cx, cy, zPlane);
#endif
#ifdef DO_FOG
	    fog[count] = solve_plane(cx, cy, fogPlane);
#endif
#ifdef DO_RGBA
            rgba[count][RCOMP] = solve_plane_chan(cx, cy, rPlane);
            rgba[count][GCOMP] = solve_plane_chan(cx, cy, gPlane);
            rgba[count][BCOMP] = solve_plane_chan(cx, cy, bPlane);
            rgba[count][ACOMP] = solve_plane_chan(cx, cy, aPlane);
#endif
#ifdef DO_INDEX
            index[count] = (GLint) solve_plane(cx, cy, iPlane);
#endif
#ifdef DO_SPEC
            spec[count][RCOMP] = solve_plane_chan(cx, cy, srPlane);
            spec[count][GCOMP] = solve_plane_chan(cx, cy, sgPlane);
            spec[count][BCOMP] = solve_plane_chan(cx, cy, sbPlane);
#endif
#ifdef DO_TEX
            {
               const GLfloat invQ = solve_plane_recip(cx, cy, vPlane);
               s[count] = solve_plane(cx, cy, sPlane) * invQ;
               t[count] = solve_plane(cx, cy, tPlane) * invQ;
               u[count] = solve_plane(cx, cy, uPlane) * invQ;
               lambda[count] = compute_lambda(sPlane, tPlane, invQ,
                                                 texWidth, texHeight);
            }
#elif defined(DO_MULTITEX)
            {
               GLuint unit;
               for (unit = 0; unit < ctx->Const.MaxTextureUnits; unit++) {
                  if (ctx->Texture.Unit[unit]._ReallyEnabled) {
                     GLfloat invQ = solve_plane_recip(cx, cy, vPlane[unit]);
                     s[unit][count] = solve_plane(cx, cy, sPlane[unit]) * invQ;
                     t[unit][count] = solve_plane(cx, cy, tPlane[unit]) * invQ;
                     u[unit][count] = solve_plane(cx, cy, uPlane[unit]) * invQ;
                     lambda[unit][count] = compute_lambda(sPlane[unit],
                          tPlane[unit], invQ, texWidth[unit], texHeight[unit]);
                  }
               }
            }
#endif
            ix++;
            count++;
            coverage = compute_coveragef(pMin, pMid, pMax, ix, iy);
         }

         if (ix <= startX)
            continue;

         n = (GLuint) ix - (GLuint) startX;

#ifdef DO_MULTITEX
#  ifdef DO_SPEC
         _mesa_write_multitexture_span(ctx, n, startX, iy, z, fog,
                                       (const GLfloat (*)[MAX_WIDTH]) s,
                                       (const GLfloat (*)[MAX_WIDTH]) t,
                                       (const GLfloat (*)[MAX_WIDTH]) u,
                                       (GLfloat (*)[MAX_WIDTH]) lambda,
                                       rgba, (const GLchan (*)[4]) spec,
                                       coverageSpan,  GL_POLYGON);
#  else
         _mesa_write_multitexture_span(ctx, n, startX, iy, z, fog,
                                       (const GLfloat (*)[MAX_WIDTH]) s,
                                       (const GLfloat (*)[MAX_WIDTH]) t,
                                       (const GLfloat (*)[MAX_WIDTH]) u,
                                       lambda, rgba, NULL, coverageSpan,
                                       GL_POLYGON);
#  endif
#elif defined(DO_TEX)
#  ifdef DO_SPEC
         _mesa_write_texture_span(ctx, n, startX, iy, z, fog,
                                  s, t, u, lambda, rgba,
                                  (const GLchan (*)[4]) spec,
                                  coverageSpan, GL_POLYGON);
#  else
         _mesa_write_texture_span(ctx, n, startX, iy, z, fog,
                                  s, t, u, lambda,
                                  rgba, NULL, coverageSpan, GL_POLYGON);
#  endif
#elif defined(DO_RGBA)
         _mesa_write_rgba_span(ctx, n, startX, iy, z, fog, rgba,
                               coverageSpan, GL_POLYGON);
#elif defined(DO_INDEX)
         _mesa_write_index_span(ctx, n, startX, iy, z, fog, index,
                                icoverageSpan, GL_POLYGON);
#endif
      }
   }
   else {
      /* scan right to left */
      const GLfloat *pMin = vMin->win;
      const GLfloat *pMid = vMid->win;
      const GLfloat *pMax = vMax->win;
      const GLfloat dxdy = majDx / majDy;
      const GLfloat xAdj = dxdy > 0 ? dxdy : 0.0F;
      GLfloat x = pMin[0] - (yMin - iyMin) * dxdy;
      GLint iy;
      for (iy = iyMin; iy < iyMax; iy++, x += dxdy) {
         GLint ix, left, startX = (GLint) (x + xAdj);
         GLuint count, n;
         GLfloat coverage = 0.0F;

         /* make sure we're not past the window edge */
         if (startX >= ctx->DrawBuffer->_Xmax) {
            startX = ctx->DrawBuffer->_Xmax - 1;
         }

         /* skip fragments with zero coverage */
         while (startX >= 0) {
            coverage = compute_coveragef(pMin, pMax, pMid, startX, iy);
            if (coverage > 0.0F)
               break;
            startX--;
         }

         /* enter interior of triangle */
         ix = startX;
         count = 0;
         while (coverage > 0.0F) {
            /* (cx,cy) = center of fragment */
            const GLfloat cx = ix + 0.5F, cy = iy + 0.5F;
#ifdef DO_INDEX
            icoverageSpan[ix] = compute_coveragei(pMin, pMid, pMax, ix, iy);
#else
            coverageSpan[ix] = coverage;
#endif
#ifdef DO_Z
            z[ix] = (GLdepth) solve_plane(cx, cy, zPlane);
#endif
#ifdef DO_FOG
            fog[ix] = solve_plane(cx, cy, fogPlane);
#endif
#ifdef DO_RGBA
            rgba[ix][RCOMP] = solve_plane_chan(cx, cy, rPlane);
            rgba[ix][GCOMP] = solve_plane_chan(cx, cy, gPlane);
            rgba[ix][BCOMP] = solve_plane_chan(cx, cy, bPlane);
            rgba[ix][ACOMP] = solve_plane_chan(cx, cy, aPlane);
#endif
#ifdef DO_INDEX
            index[ix] = (GLint) solve_plane(cx, cy, iPlane);
#endif
#ifdef DO_SPEC
            spec[ix][RCOMP] = solve_plane_chan(cx, cy, srPlane);
            spec[ix][GCOMP] = solve_plane_chan(cx, cy, sgPlane);
            spec[ix][BCOMP] = solve_plane_chan(cx, cy, sbPlane);
#endif
#ifdef DO_TEX
            {
               const GLfloat invQ = solve_plane_recip(cx, cy, vPlane);
               s[ix] = solve_plane(cx, cy, sPlane) * invQ;
               t[ix] = solve_plane(cx, cy, tPlane) * invQ;
               u[ix] = solve_plane(cx, cy, uPlane) * invQ;
               lambda[ix] = compute_lambda(sPlane, tPlane, invQ,
                                              texWidth, texHeight);
            }
#elif defined(DO_MULTITEX)
            {
               GLuint unit;
               for (unit = 0; unit < ctx->Const.MaxTextureUnits; unit++) {
                  if (ctx->Texture.Unit[unit]._ReallyEnabled) {
                     GLfloat invQ = solve_plane_recip(cx, cy, vPlane[unit]);
                     s[unit][ix] = solve_plane(cx, cy, sPlane[unit]) * invQ;
                     t[unit][ix] = solve_plane(cx, cy, tPlane[unit]) * invQ;
                     u[unit][ix] = solve_plane(cx, cy, uPlane[unit]) * invQ;
                     lambda[unit][ix] = compute_lambda(sPlane[unit],
                         tPlane[unit], invQ, texWidth[unit], texHeight[unit]);
                  }
               }
            }
#endif
            ix--;
            count++;
            coverage = compute_coveragef(pMin, pMax, pMid, ix, iy);
         }

         if (startX <= ix)
            continue;

         n = (GLuint) startX - (GLuint) ix;

         left = ix + 1;
#ifdef DO_MULTITEX
         {
            GLuint unit;
            for (unit = 0; unit < ctx->Const.MaxTextureUnits; unit++) {
               if (ctx->Texture.Unit[unit]._ReallyEnabled) {
                  GLint j;
                  for (j = 0; j < (GLint) n; j++) {
                     s[unit][j] = s[unit][j + left];
                     t[unit][j] = t[unit][j + left];
                     u[unit][j] = u[unit][j + left];
                     lambda[unit][j] = lambda[unit][j + left];
                  }
               }
            }
         }
#  ifdef DO_SPEC
         _mesa_write_multitexture_span(ctx, n, left, iy, z + left, fog + left,
                                       (const GLfloat (*)[MAX_WIDTH]) s,
                                       (const GLfloat (*)[MAX_WIDTH]) t,
                                       (const GLfloat (*)[MAX_WIDTH]) u,
                                       lambda, rgba + left,
                                       (const GLchan (*)[4]) (spec + left),
                                       coverageSpan + left,
                                       GL_POLYGON);
#  else
         _mesa_write_multitexture_span(ctx, n, left, iy, z + left, fog + left,
                                       (const GLfloat (*)[MAX_WIDTH]) s,
                                       (const GLfloat (*)[MAX_WIDTH]) t,
                                       (const GLfloat (*)[MAX_WIDTH]) u,
                                       lambda,
                                       rgba + left, NULL, coverageSpan + left,
                                       GL_POLYGON);
#  endif
#elif defined(DO_TEX)
#  ifdef DO_SPEC
         _mesa_write_texture_span(ctx, n, left, iy, z + left, fog + left,
                                  s + left, t + left, u + left,
                                  lambda + left, rgba + left,
                                  (const GLchan (*)[4]) (spec + left),
                                  coverageSpan + left,
                                  GL_POLYGON);
#  else
         _mesa_write_texture_span(ctx, n, left, iy, z + left, fog + left,
                                  s + left, t + left,
                                  u + left, lambda + left,
                                  rgba + left, NULL,
                                  coverageSpan + left, GL_POLYGON);
#  endif
#elif defined(DO_RGBA)
         _mesa_write_rgba_span(ctx, n, left, iy, z + left, fog + left,
                               rgba + left, coverageSpan + left, GL_POLYGON);
#elif defined(DO_INDEX)
         _mesa_write_index_span(ctx, n, left, iy, z + left, fog + left,
                               index + left, icoverageSpan + left, GL_POLYGON);
#endif
      }
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

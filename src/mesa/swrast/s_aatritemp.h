/* $Id: s_aatritemp.h,v 1.1 2000/10/31 18:00:04 keithw Exp $ */

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
   const struct vertex_buffer *VB = ctx->VB;
   const GLfloat *p0 = VB->Win.data[v0];
   const GLfloat *p1 = VB->Win.data[v1];
   const GLfloat *p2 = VB->Win.data[v2];
   GLint vMin, vMid, vMax;
   GLint iyMin, iyMax;
   GLfloat yMin, yMax;
   GLboolean ltor;
   GLfloat majDx, majDy;
#ifdef DO_Z
   GLfloat zPlane[4];                                       /* Z (depth) */
   GLdepth z[MAX_WIDTH];
   GLfloat fogPlane[4];
   GLfixed fog[MAX_WIDTH];
#endif
#ifdef DO_RGBA
   GLfloat rPlane[4], gPlane[4], bPlane[4], aPlane[4];      /* color */
   GLchan rgba[MAX_WIDTH][4];
#endif
#ifdef DO_INDEX
   GLfloat iPlane[4];                                       /* color index */
   GLuint index[MAX_WIDTH];
#endif
#ifdef DO_SPEC
   GLfloat srPlane[4], sgPlane[4], sbPlane[4];              /* spec color */
   GLchan spec[MAX_WIDTH][4];
#endif
#ifdef DO_TEX
   GLfloat sPlane[4], tPlane[4], uPlane[4], vPlane[4];
   GLfloat texWidth, texHeight;
   GLfloat s[MAX_WIDTH], t[MAX_WIDTH], u[MAX_WIDTH];
   GLfloat lambda[MAX_WIDTH];
#elif defined(DO_MULTITEX)
   GLfloat sPlane[MAX_TEXTURE_UNITS][4];
   GLfloat tPlane[MAX_TEXTURE_UNITS][4];
   GLfloat uPlane[MAX_TEXTURE_UNITS][4];
   GLfloat vPlane[MAX_TEXTURE_UNITS][4];
   GLfloat texWidth[MAX_TEXTURE_UNITS], texHeight[MAX_TEXTURE_UNITS];
   GLfloat s[MAX_TEXTURE_UNITS][MAX_WIDTH];
   GLfloat t[MAX_TEXTURE_UNITS][MAX_WIDTH];
   GLfloat u[MAX_TEXTURE_UNITS][MAX_WIDTH];
   GLfloat lambda[MAX_TEXTURE_UNITS][MAX_WIDTH];
#endif
   GLfloat bf = ctx->backface_sign;

   /* determine bottom to top order of vertices */
   {
      GLfloat y0 = VB->Win.data[v0][1];
      GLfloat y1 = VB->Win.data[v1][1];
      GLfloat y2 = VB->Win.data[v2][1];
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

   majDx = VB->Win.data[vMax][0] - VB->Win.data[vMin][0];
   majDy = VB->Win.data[vMax][1] - VB->Win.data[vMin][1];

   {
      const GLfloat botDx = VB->Win.data[vMid][0] - VB->Win.data[vMin][0];
      const GLfloat botDy = VB->Win.data[vMid][1] - VB->Win.data[vMin][1];
      const GLfloat area = majDx * botDy - botDx * majDy;
      ltor = (GLboolean) (area < 0.0F);
      /* Do backface culling */
      if (area * bf < 0 || area * area < .0025)
	 return;
   }

#ifndef DO_OCCLUSION_TEST
   ctx->OcclusionResult = GL_TRUE;
#endif

   /* plane setup */
#ifdef DO_Z
   compute_plane(p0, p1, p2, p0[2], p1[2], p2[2], zPlane);
   compute_plane(p0, p1, p2, 
		 VB->FogCoordPtr->data[v0], 
		 VB->FogCoordPtr->data[v1], 
		 VB->FogCoordPtr->data[v2], 
		 fogPlane);
#endif
#ifdef DO_RGBA
   if (ctx->Light.ShadeModel == GL_SMOOTH) {
      GLchan (*rgba)[4] = VB->ColorPtr->data;
      compute_plane(p0, p1, p2, rgba[v0][0], rgba[v1][0], rgba[v2][0], rPlane);
      compute_plane(p0, p1, p2, rgba[v0][1], rgba[v1][1], rgba[v2][1], gPlane);
      compute_plane(p0, p1, p2, rgba[v0][2], rgba[v1][2], rgba[v2][2], bPlane);
      compute_plane(p0, p1, p2, rgba[v0][3], rgba[v1][3], rgba[v2][3], aPlane);
   }
   else {
      constant_plane(VB->ColorPtr->data[pv][RCOMP], rPlane);
      constant_plane(VB->ColorPtr->data[pv][GCOMP], gPlane);
      constant_plane(VB->ColorPtr->data[pv][BCOMP], bPlane);
      constant_plane(VB->ColorPtr->data[pv][ACOMP], aPlane);
   }
#endif
#ifdef DO_INDEX
   if (ctx->Light.ShadeModel == GL_SMOOTH) {
      compute_plane(p0, p1, p2, VB->IndexPtr->data[v0],
                    VB->IndexPtr->data[v1], VB->IndexPtr->data[v2], iPlane);
   }
   else {
      constant_plane(VB->IndexPtr->data[pv], iPlane);
   }
#endif
#ifdef DO_SPEC
   {
      GLchan (*spec)[4] = VB->SecondaryColorPtr->data;
      compute_plane(p0, p1, p2, spec[v0][0], spec[v1][0], spec[v2][0],srPlane);
      compute_plane(p0, p1, p2, spec[v0][1], spec[v1][1], spec[v2][1],sgPlane);
      compute_plane(p0, p1, p2, spec[v0][2], spec[v1][2], spec[v2][2],sbPlane);
   }
#endif
#ifdef DO_TEX
   {
      const struct gl_texture_object *obj = ctx->Texture.Unit[0].Current;
      const struct gl_texture_image *texImage = obj->Image[obj->BaseLevel];
      const GLint tSize = 3;
      const GLfloat invW0 = VB->Win.data[v0][3];
      const GLfloat invW1 = VB->Win.data[v1][3];
      const GLfloat invW2 = VB->Win.data[v2][3];
      GLfloat (*texCoord)[4] = VB->TexCoordPtr[0]->data;
      const GLfloat s0 = texCoord[v0][0] * invW0;
      const GLfloat s1 = texCoord[v1][0] * invW1;
      const GLfloat s2 = texCoord[v2][0] * invW2;
      const GLfloat t0 = (tSize > 1) ? texCoord[v0][1] * invW0 : 0.0F;
      const GLfloat t1 = (tSize > 1) ? texCoord[v1][1] * invW1 : 0.0F;
      const GLfloat t2 = (tSize > 1) ? texCoord[v2][1] * invW2 : 0.0F;
      const GLfloat r0 = (tSize > 2) ? texCoord[v0][2] * invW0 : 0.0F;
      const GLfloat r1 = (tSize > 2) ? texCoord[v1][2] * invW1 : 0.0F;
      const GLfloat r2 = (tSize > 2) ? texCoord[v2][2] * invW2 : 0.0F;
      const GLfloat q0 = (tSize > 3) ? texCoord[v0][3] * invW0 : invW0;
      const GLfloat q1 = (tSize > 3) ? texCoord[v1][3] * invW1 : invW1;
      const GLfloat q2 = (tSize > 3) ? texCoord[v2][3] * invW2 : invW2;
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
         if (ctx->Texture.Unit[u].ReallyEnabled) {
            const struct gl_texture_object *obj = ctx->Texture.Unit[u].Current;
            const struct gl_texture_image *texImage = obj->Image[obj->BaseLevel];
            const GLint tSize = VB->TexCoordPtr[u]->size;
            const GLfloat invW0 = VB->Win.data[v0][3];
            const GLfloat invW1 = VB->Win.data[v1][3];
            const GLfloat invW2 = VB->Win.data[v2][3];
            GLfloat (*texCoord)[4] = VB->TexCoordPtr[u]->data;
            const GLfloat s0 = texCoord[v0][0] * invW0;
            const GLfloat s1 = texCoord[v1][0] * invW1;
            const GLfloat s2 = texCoord[v2][0] * invW2;
            const GLfloat t0 = (tSize > 1) ? texCoord[v0][1] * invW0 : 0.0F;
            const GLfloat t1 = (tSize > 1) ? texCoord[v1][1] * invW1 : 0.0F;
            const GLfloat t2 = (tSize > 1) ? texCoord[v2][1] * invW2 : 0.0F;
            const GLfloat r0 = (tSize > 2) ? texCoord[v0][2] * invW0 : 0.0F;
            const GLfloat r1 = (tSize > 2) ? texCoord[v1][2] * invW1 : 0.0F;
            const GLfloat r2 = (tSize > 2) ? texCoord[v2][2] * invW2 : 0.0F;
            const GLfloat q0 = (tSize > 3) ? texCoord[v0][3] * invW0 : invW0;
            const GLfloat q1 = (tSize > 3) ? texCoord[v1][3] * invW1 : invW1;
            const GLfloat q2 = (tSize > 3) ? texCoord[v2][3] * invW2 : invW2;
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

   yMin = VB->Win.data[vMin][1];
   yMax = VB->Win.data[vMax][1];
   iyMin = (int) yMin;
   iyMax = (int) yMax + 1;

   if (ltor) {
      /* scan left to right */
      const float *pMin = VB->Win.data[vMin];
      const float *pMid = VB->Win.data[vMid];
      const float *pMax = VB->Win.data[vMax];
      const float dxdy = majDx / majDy;
      const float xAdj = dxdy < 0.0F ? -dxdy : 0.0F;
      float x = VB->Win.data[vMin][0] - (yMin - iyMin) * dxdy;
      int iy;
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
#ifdef DO_Z
            z[count] = (GLdepth) solve_plane(ix, iy, zPlane);
	    fog[count] = FloatToFixed(solve_plane(ix, iy, fogPlane));
#endif
#ifdef DO_RGBA
            rgba[count][RCOMP] = solve_plane_chan(ix, iy, rPlane);
            rgba[count][GCOMP] = solve_plane_chan(ix, iy, gPlane);
            rgba[count][BCOMP] = solve_plane_chan(ix, iy, bPlane);
            rgba[count][ACOMP] = (GLchan) (solve_plane_chan(ix, iy, aPlane) * coverage);
#endif
#ifdef DO_INDEX
            {
               GLint frac = compute_coveragei(pMin, pMid, pMax, ix, iy);
               GLint indx = (GLint) solve_plane(ix, iy, iPlane);
               index[count] = (indx & ~0xf) | frac;
            }
#endif
#ifdef DO_SPEC
            spec[count][RCOMP] = solve_plane_chan(ix, iy, srPlane);
            spec[count][GCOMP] = solve_plane_chan(ix, iy, sgPlane);
            spec[count][BCOMP] = solve_plane_chan(ix, iy, sbPlane);
#endif
#ifdef DO_TEX
            {
               GLfloat invQ = solve_plane_recip(ix, iy, vPlane);
               s[count] = solve_plane(ix, iy, sPlane) * invQ;
               t[count] = solve_plane(ix, iy, tPlane) * invQ;
               u[count] = solve_plane(ix, iy, uPlane) * invQ;
               lambda[count] = compute_lambda(sPlane, tPlane, invQ,
                                                 texWidth, texHeight);
            }
#elif defined(DO_MULTITEX)
            {
               GLuint unit;
               for (unit = 0; unit < ctx->Const.MaxTextureUnits; unit++) {
                  if (ctx->Texture.Unit[unit].ReallyEnabled) {
                     GLfloat invQ = solve_plane_recip(ix, iy, vPlane[unit]);
                     s[unit][count] = solve_plane(ix, iy, sPlane[unit]) * invQ;
                     t[unit][count] = solve_plane(ix, iy, tPlane[unit]) * invQ;
                     u[unit][count] = solve_plane(ix, iy, uPlane[unit]) * invQ;
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

         n = (GLuint) ix - (GLuint) startX;
#ifdef DO_MULTITEX
#  ifdef DO_SPEC
         gl_write_multitexture_span(ctx, n, startX, iy, z, fog,
                                    (const GLfloat (*)[MAX_WIDTH]) s,
                                    (const GLfloat (*)[MAX_WIDTH]) t,
                                    (const GLfloat (*)[MAX_WIDTH]) u,
                                    (GLfloat (*)[MAX_WIDTH]) lambda,
                                    rgba, (const GLchan (*)[4]) spec,
                                    GL_POLYGON);
#  else
         gl_write_multitexture_span(ctx, n, startX, iy, z, fog,
                                    (const GLfloat (*)[MAX_WIDTH]) s,
                                    (const GLfloat (*)[MAX_WIDTH]) t,
                                    (const GLfloat (*)[MAX_WIDTH]) u,
                                    lambda, rgba, NULL, GL_POLYGON);
#  endif
#elif defined(DO_TEX)
#  ifdef DO_SPEC
         gl_write_texture_span(ctx, n, startX, iy, z, fog,
                               s, t, u, lambda, rgba,
                               (const GLchan (*)[4]) spec, GL_POLYGON);
#  else
         gl_write_texture_span(ctx, n, startX, iy, z, fog,
                               s, t, u, lambda,
                               rgba, NULL, GL_POLYGON);
#  endif
#elif defined(DO_RGBA)
         gl_write_rgba_span(ctx, n, startX, iy, z, fog, rgba, GL_POLYGON);
#elif defined(DO_INDEX)
         gl_write_index_span(ctx, n, startX, iy, z, fog, index, GL_POLYGON);
#endif
      }
   }
   else {
      /* scan right to left */
      const GLfloat *pMin = VB->Win.data[vMin];
      const GLfloat *pMid = VB->Win.data[vMid];
      const GLfloat *pMax = VB->Win.data[vMax];
      const GLfloat dxdy = majDx / majDy;
      const GLfloat xAdj = dxdy > 0 ? dxdy : 0.0F;
      GLfloat x = VB->Win.data[vMin][0] - (yMin - iyMin) * dxdy;
      GLint iy;
      for (iy = iyMin; iy < iyMax; iy++, x += dxdy) {
         GLint ix, left, startX = (GLint) (x + xAdj);
         GLuint count, n;
         GLfloat coverage = 0.0F;
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
#ifdef DO_Z
            z[ix] = (GLdepth) solve_plane(ix, iy, zPlane);
            fog[ix] = FloatToFixed(solve_plane(ix, iy, fogPlane));
#endif
#ifdef DO_RGBA
            rgba[ix][RCOMP] = solve_plane_chan(ix, iy, rPlane);
            rgba[ix][GCOMP] = solve_plane_chan(ix, iy, gPlane);
            rgba[ix][BCOMP] = solve_plane_chan(ix, iy, bPlane);
            rgba[ix][ACOMP] = (GLchan) (solve_plane_chan(ix, iy, aPlane) * coverage);
#endif
#ifdef DO_INDEX
            {
               GLint frac = compute_coveragei(pMin, pMax, pMid, ix, iy);
               GLint indx = (GLint) solve_plane(ix, iy, iPlane);
               index[ix] = (indx & ~0xf) | frac;
            }
#endif
#ifdef DO_SPEC
            spec[ix][RCOMP] = solve_plane_chan(ix, iy, srPlane);
            spec[ix][GCOMP] = solve_plane_chan(ix, iy, sgPlane);
            spec[ix][BCOMP] = solve_plane_chan(ix, iy, sbPlane);
#endif
#ifdef DO_TEX
            {
               GLfloat invQ = solve_plane_recip(ix, iy, vPlane);
               s[ix] = solve_plane(ix, iy, sPlane) * invQ;
               t[ix] = solve_plane(ix, iy, tPlane) * invQ;
               u[ix] = solve_plane(ix, iy, uPlane) * invQ;
               lambda[ix] = compute_lambda(sPlane, tPlane, invQ,
                                              texWidth, texHeight);
            }
#elif defined(DO_MULTITEX)
            {
               GLuint unit;
               for (unit = 0; unit < ctx->Const.MaxTextureUnits; unit++) {
                  if (ctx->Texture.Unit[unit].ReallyEnabled) {
                     GLfloat invQ = solve_plane_recip(ix, iy, vPlane[unit]);
                     s[unit][ix] = solve_plane(ix, iy, sPlane[unit]) * invQ;
                     t[unit][ix] = solve_plane(ix, iy, tPlane[unit]) * invQ;
                     u[unit][ix] = solve_plane(ix, iy, uPlane[unit]) * invQ;
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

         n = (GLuint) startX - (GLuint) ix;
         left = ix + 1;
#ifdef DO_MULTITEX
         {
            GLuint unit;
            for (unit = 0; unit < ctx->Const.MaxTextureUnits; unit++) {
               if (ctx->Texture.Unit[unit].ReallyEnabled) {
                  GLint j;
                  for (j = 0; j < n; j++) {
                     s[unit][j] = s[unit][j + left];
                     t[unit][j] = t[unit][j + left];
                     u[unit][j] = u[unit][j + left];
                     lambda[unit][j] = lambda[unit][j + left];
                  }
               }
            }
         }
#  ifdef DO_SPEC
         gl_write_multitexture_span(ctx, n, left, iy, z + left, fog + left,
                                    (const GLfloat (*)[MAX_WIDTH]) s,
                                    (const GLfloat (*)[MAX_WIDTH]) t,
                                    (const GLfloat (*)[MAX_WIDTH]) u,
                                    lambda, rgba + left,
                                    (const GLchan (*)[4]) (spec + left),
                                    GL_POLYGON);
#  else
         gl_write_multitexture_span(ctx, n, left, iy, z + left, fog + left,
                                    (const GLfloat (*)[MAX_WIDTH]) s,
                                    (const GLfloat (*)[MAX_WIDTH]) t,
                                    (const GLfloat (*)[MAX_WIDTH]) u,
                                    lambda,
                                    rgba + left, NULL, GL_POLYGON);
#  endif
#elif defined(DO_TEX)
#  ifdef DO_SPEC
         gl_write_texture_span(ctx, n, left, iy, z + left, fog + left,
                               s + left, t + left, u + left,
                               lambda + left, rgba + left,
                               (const GLchan (*)[4]) (spec + left),
                               GL_POLYGON);
#  else
         gl_write_texture_span(ctx, n, left, iy, z + left, fog + left, 
                               s + left, t + left,
                               u + left, lambda + left,
                               rgba + left, NULL, GL_POLYGON);
#  endif
#elif defined(DO_RGBA)
         gl_write_rgba_span(ctx, n, left, iy, z + left, fog + left, 
                            rgba + left, GL_POLYGON);
#elif defined(DO_INDEX)
         gl_write_index_span(ctx, n, left, iy, z + left, fog + left, 
                             index + left, GL_POLYGON);
#endif
      }
   }
}


#ifdef DO_Z
#undef DO_Z
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

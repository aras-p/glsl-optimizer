/* $Id: s_tritemp.h,v 1.19 2001/06/13 14:53:52 brianp Exp $ */

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
 * Triangle Rasterizer Template
 *
 * This file is #include'd to generate custom triangle rasterizers.
 *
 * The following macros may be defined to indicate what auxillary information
 * must be interplated across the triangle:
 *    INTERP_Z        - if defined, interpolate Z values
 *    INTERP_FOG      - if defined, interpolate fog values
 *    INTERP_RGB      - if defined, interpolate RGB values
 *    INTERP_ALPHA    - if defined, interpolate Alpha values
 *    INTERP_SPEC     - if defined, interpolate specular RGB values
 *    INTERP_INDEX    - if defined, interpolate color index values
 *    INTERP_INT_TEX  - if defined, interpolate integer ST texcoords
 *                         (fast, simple 2-D texture mapping)
 *    INTERP_TEX      - if defined, interpolate set 0 float STRQ texcoords
 *                         NOTE:  OpenGL STRQ = Mesa STUV (R was taken for red)
 *    INTERP_MULTITEX - if defined, interpolate N units of STRQ texcoords
 *    INTERP_LAMBDA   - if defined, compute lambda value (for mipmapping)
 *                         a lambda value for every texture unit
 *
 * When one can directly address pixels in the color buffer the following
 * macros can be defined and used to compute pixel addresses during
 * rasterization (see pRow):
 *    PIXEL_TYPE          - the datatype of a pixel (GLubyte, GLushort, GLuint)
 *    BYTES_PER_ROW       - number of bytes per row in the color buffer
 *    PIXEL_ADDRESS(X,Y)  - returns the address of pixel at (X,Y) where
 *                          Y==0 at bottom of screen and increases upward.
 *
 * Similarly, for direct depth buffer access, this type is used for depth
 * buffer addressing:
 *    DEPTH_TYPE          - either GLushort or GLuint
 *
 * Optionally, one may provide one-time setup code per triangle:
 *    SETUP_CODE    - code which is to be executed once per triangle
 *    CLEANUP_CODE    - code to execute at end of triangle
 *
 * The following macro MUST be defined:
 *    RENDER_SPAN(span) - code to write a span of pixels.
 *
 * This code was designed for the origin to be in the lower-left corner.
 *
 * Inspired by triangle rasterizer code written by Allen Akin.  Thanks Allen!
 */

/*void triangle( GLcontext *ctx, SWvertex *v0, SWvertex *v1, SWvertex *v2 )*/
{
   typedef struct {
        const SWvertex *v0, *v1;   /* Y(v0) < Y(v1) */
	GLfloat dx;	/* X(v1) - X(v0) */
	GLfloat dy;	/* Y(v1) - Y(v0) */
	GLfixed fdxdy;	/* dx/dy in fixed-point */
	GLfixed fsx;	/* first sample point x coord */
	GLfixed fsy;
	GLfloat adjy;	/* adjust from v[0]->fy to fsy, scaled */
	GLint lines;	/* number of lines to be sampled on this edge */
	GLfixed fx0;	/* fixed pt X of lower endpoint */
   } EdgeT;

#ifdef INTERP_Z
   const GLint depthBits = ctx->Visual.depthBits;
   const GLint fixedToDepthShift = depthBits <= 16 ? FIXED_SHIFT : 0;
   const GLfloat maxDepth = ctx->DepthMaxF;
#define FixedToDepth(F)  ((F) >> fixedToDepthShift)
#endif
   EdgeT eMaj, eTop, eBot;
   GLfloat oneOverArea;
   const SWvertex *vMin, *vMid, *vMax;  /* Y(vMin)<=Y(vMid)<=Y(vMax) */
   float bf = SWRAST_CONTEXT(ctx)->_backface_sign;
   GLboolean tiny;
   const GLint snapMask = ~((FIXED_ONE / 16) - 1);  /* for x/y coord snapping */
   GLfixed vMin_fx, vMin_fy, vMid_fx, vMid_fy, vMax_fx, vMax_fy;

   struct triangle_span span;

#ifdef INTERP_Z
   (void) fixedToDepthShift;
#endif

   /* Compute fixed point x,y coords w/ half-pixel offsets and snapping.
    * And find the order of the 3 vertices along the Y axis.
    */
   {
      const GLfixed fy0 = FloatToFixed(v0->win[1] - 0.5F) & snapMask;
      const GLfixed fy1 = FloatToFixed(v1->win[1] - 0.5F) & snapMask;
      const GLfixed fy2 = FloatToFixed(v2->win[1] - 0.5F) & snapMask;

      if (fy0 <= fy1) {
	 if (fy1 <= fy2) {
            /* y0 <= y1 <= y2 */
	    vMin = v0;   vMid = v1;   vMax = v2;
            vMin_fy = fy0;  vMid_fy = fy1;  vMax_fy = fy2;
	 }
	 else if (fy2 <= fy0) {
            /* y2 <= y0 <= y1 */
	    vMin = v2;   vMid = v0;   vMax = v1;
            vMin_fy = fy2;  vMid_fy = fy0;  vMax_fy = fy1;
	 }
	 else {
            /* y0 <= y2 <= y1 */
	    vMin = v0;   vMid = v2;   vMax = v1;
            vMin_fy = fy0;  vMid_fy = fy2;  vMax_fy = fy1;
            bf = -bf;
	 }
      }
      else {
	 if (fy0 <= fy2) {
            /* y1 <= y0 <= y2 */
	    vMin = v1;   vMid = v0;   vMax = v2;
            vMin_fy = fy1;  vMid_fy = fy0;  vMax_fy = fy2;
            bf = -bf;
	 }
	 else if (fy2 <= fy1) {
            /* y2 <= y1 <= y0 */
	    vMin = v2;   vMid = v1;   vMax = v0;
            vMin_fy = fy2;  vMid_fy = fy1;  vMax_fy = fy0;
            bf = -bf;
	 }
	 else {
            /* y1 <= y2 <= y0 */
	    vMin = v1;   vMid = v2;   vMax = v0;
            vMin_fy = fy1;  vMid_fy = fy2;  vMax_fy = fy0;
	 }
      }

      /* fixed point X coords */
      vMin_fx = FloatToFixed(vMin->win[0] + 0.5F) & snapMask;
      vMid_fx = FloatToFixed(vMid->win[0] + 0.5F) & snapMask;
      vMax_fx = FloatToFixed(vMax->win[0] + 0.5F) & snapMask;
   }

   /* vertex/edge relationship */
   eMaj.v0 = vMin;   eMaj.v1 = vMax;   /*TODO: .v1's not needed */
   eTop.v0 = vMid;   eTop.v1 = vMax;
   eBot.v0 = vMin;   eBot.v1 = vMid;

   /* compute deltas for each edge:  vertex[upper] - vertex[lower] */
   eMaj.dx = FixedToFloat(vMax_fx - vMin_fx);
   eMaj.dy = FixedToFloat(vMax_fy - vMin_fy);
   eTop.dx = FixedToFloat(vMax_fx - vMid_fx);
   eTop.dy = FixedToFloat(vMax_fy - vMid_fy);
   eBot.dx = FixedToFloat(vMid_fx - vMin_fx);
   eBot.dy = FixedToFloat(vMid_fy - vMin_fy);

   /* compute area, oneOverArea and perform backface culling */
   {
      const GLfloat area = eMaj.dx * eBot.dy - eBot.dx * eMaj.dy;

      /* Do backface culling */
      if (area * bf < 0.0)
	 return;

      if (area == 0.0F)
         return;

      /* This may not be needed anymore.  Let's see if anyone reports a problem. */
#if 0
      /* check for very tiny triangle */
      if (area * area < (0.001F * 0.001F)) { /* square to ensure positive value */
         oneOverArea = 1.0F / 0.001F;  /* a close-enough value */
         tiny = GL_TRUE;
      }
      else
#endif
      {
         oneOverArea = 1.0F / area;
         tiny = GL_FALSE;
      }
   }

#ifndef DO_OCCLUSION_TEST
   ctx->OcclusionResult = GL_TRUE;
#endif

   /* Edge setup.  For a triangle strip these could be reused... */
   {
      eMaj.fsy = FixedCeil(vMin_fy);
      eMaj.lines = FixedToInt(FixedCeil(vMax_fy - eMaj.fsy));
      if (eMaj.lines > 0) {
         GLfloat dxdy = eMaj.dx / eMaj.dy;
         eMaj.fdxdy = SignedFloatToFixed(dxdy);
         eMaj.adjy = (GLfloat) (eMaj.fsy - vMin_fy);  /* SCALED! */
         eMaj.fx0 = vMin_fx;
         eMaj.fsx = eMaj.fx0 + (GLfixed) (eMaj.adjy * dxdy);
      }
      else {
         return;  /*CULLED*/
      }

      eTop.fsy = FixedCeil(vMid_fy);
      eTop.lines = FixedToInt(FixedCeil(vMax_fy - eTop.fsy));
      if (eTop.lines > 0) {
         GLfloat dxdy = eTop.dx / eTop.dy;
         eTop.fdxdy = SignedFloatToFixed(dxdy);
         eTop.adjy = (GLfloat) (eTop.fsy - vMid_fy); /* SCALED! */
         eTop.fx0 = vMid_fx;
         eTop.fsx = eTop.fx0 + (GLfixed) (eTop.adjy * dxdy);
      }

      eBot.fsy = FixedCeil(vMin_fy);
      eBot.lines = FixedToInt(FixedCeil(vMid_fy - eBot.fsy));
      if (eBot.lines > 0) {
         GLfloat dxdy = eBot.dx / eBot.dy;
         eBot.fdxdy = SignedFloatToFixed(dxdy);
         eBot.adjy = (GLfloat) (eBot.fsy - vMin_fy);  /* SCALED! */
         eBot.fx0 = vMin_fx;
         eBot.fsx = eBot.fx0 + (GLfixed) (eBot.adjy * dxdy);
      }
   }

   /*
    * Conceptually, we view a triangle as two subtriangles
    * separated by a perfectly horizontal line.  The edge that is
    * intersected by this line is one with maximal absolute dy; we
    * call it a ``major'' edge.  The other two edges are the
    * ``top'' edge (for the upper subtriangle) and the ``bottom''
    * edge (for the lower subtriangle).  If either of these two
    * edges is horizontal or very close to horizontal, the
    * corresponding subtriangle might cover zero sample points;
    * we take care to handle such cases, for performance as well
    * as correctness.
    *
    * By stepping rasterization parameters along the major edge,
    * we can avoid recomputing them at the discontinuity where
    * the top and bottom edges meet.  However, this forces us to
    * be able to scan both left-to-right and right-to-left.
    * Also, we must determine whether the major edge is at the
    * left or right side of the triangle.  We do this by
    * computing the magnitude of the cross-product of the major
    * and top edges.  Since this magnitude depends on the sine of
    * the angle between the two edges, its sign tells us whether
    * we turn to the left or to the right when travelling along
    * the major edge to the top edge, and from this we infer
    * whether the major edge is on the left or the right.
    *
    * Serendipitously, this cross-product magnitude is also a
    * value we need to compute the iteration parameter
    * derivatives for the triangle, and it can be used to perform
    * backface culling because its sign tells us whether the
    * triangle is clockwise or counterclockwise.  In this code we
    * refer to it as ``area'' because it's also proportional to
    * the pixel area of the triangle.
    */

   {
      GLint ltor;		/* true if scanning left-to-right */
#ifdef INTERP_Z
      GLfloat dzdx, dzdy;
#endif
#ifdef INTERP_FOG
      GLfloat dfogdy;
#endif
#ifdef INTERP_RGB
      GLfloat drdx, drdy;
      GLfloat dgdx, dgdy;
      GLfloat dbdx, dbdy;
#endif
#ifdef INTERP_ALPHA
      GLfloat dadx, dady;
#endif
#ifdef INTERP_SPEC
      GLfloat dsrdx, dsrdy;
      GLfloat dsgdx, dsgdy;
      GLfloat dsbdx, dsbdy;
#endif
#ifdef INTERP_INDEX
      GLfloat didx, didy;
#endif
#ifdef INTERP_INT_TEX
      GLfloat dsdx, dsdy;
      GLfloat dtdx, dtdy;
#endif
#ifdef INTERP_TEX
      GLfloat dsdy;
      GLfloat dtdy;
      GLfloat dudy;
      GLfloat dvdy;
#endif
#ifdef INTERP_MULTITEX
      GLfloat dsdy[MAX_TEXTURE_UNITS];
      GLfloat dtdy[MAX_TEXTURE_UNITS];
      GLfloat dudy[MAX_TEXTURE_UNITS];
      GLfloat dvdy[MAX_TEXTURE_UNITS];
#endif

#if defined(INTERP_LAMBDA) && !defined(INTERP_TEX) && !defined(INTERP_MULTITEX)
#error "Mipmapping without texturing doesn't make sense."
#endif

      /*
       * Execute user-supplied setup code
       */
#ifdef SETUP_CODE
      SETUP_CODE
#endif

      ltor = (oneOverArea < 0.0F);

      span.activeMask = 0;

      /* compute d?/dx and d?/dy derivatives */
#ifdef INTERP_Z
      span.activeMask |= SPAN_Z;
      {
         GLfloat eMaj_dz, eBot_dz;
         eMaj_dz = vMax->win[2] - vMin->win[2];
         eBot_dz = vMid->win[2] - vMin->win[2];
         dzdx = oneOverArea * (eMaj_dz * eBot.dy - eMaj.dy * eBot_dz);
         if (dzdx > maxDepth || dzdx < -maxDepth) {
            /* probably a sliver triangle */
            dzdx = 0.0;
            dzdy = 0.0;
         }
         else {
            dzdy = oneOverArea * (eMaj.dx * eBot_dz - eMaj_dz * eBot.dx);
         }
         if (depthBits <= 16)
            span.zStep = SignedFloatToFixed(dzdx);
         else
            span.zStep = (GLint) dzdx;
      }
#endif
#ifdef INTERP_FOG
      span.activeMask |= SPAN_FOG;
      {
         const GLfloat eMaj_dfog = vMax->fog - vMin->fog;
         const GLfloat eBot_dfog = vMid->fog - vMin->fog;
         span.fogStep = oneOverArea * (eMaj_dfog * eBot.dy - eMaj.dy * eBot_dfog);
         dfogdy = oneOverArea * (eMaj.dx * eBot_dfog - eMaj_dfog * eBot.dx);
      }
#endif
#ifdef INTERP_RGB
      span.activeMask |= SPAN_RGBA;
      if (tiny) {
         /* This is kind of a hack to eliminate RGB color over/underflow
          * problems when rendering very tiny triangles.  We're not doing
          * anything with alpha or specular color at this time.
          */
         drdx = drdy = 0.0;  span.redStep = 0;
         dgdx = dgdy = 0.0;  span.greenStep = 0;
         dbdx = dbdy = 0.0;  span.blueStep = 0;
      }
      else {
         GLfloat eMaj_dr, eBot_dr;
         GLfloat eMaj_dg, eBot_dg;
         GLfloat eMaj_db, eBot_db;
         eMaj_dr = (GLint) vMax->color[0] - (GLint) vMin->color[0];
         eBot_dr = (GLint) vMid->color[0] - (GLint) vMin->color[0];
         drdx = oneOverArea * (eMaj_dr * eBot.dy - eMaj.dy * eBot_dr);
         span.redStep = SignedFloatToFixed(drdx);
         drdy = oneOverArea * (eMaj.dx * eBot_dr - eMaj_dr * eBot.dx);
         eMaj_dg = (GLint) vMax->color[1] - (GLint) vMin->color[1];
	 eBot_dg = (GLint) vMid->color[1] - (GLint) vMin->color[1];
         dgdx = oneOverArea * (eMaj_dg * eBot.dy - eMaj.dy * eBot_dg);
         span.greenStep = SignedFloatToFixed(dgdx);
         dgdy = oneOverArea * (eMaj.dx * eBot_dg - eMaj_dg * eBot.dx);
         eMaj_db = (GLint) vMax->color[2] - (GLint) vMin->color[2];
         eBot_db = (GLint) vMid->color[2] - (GLint) vMin->color[2];
         dbdx = oneOverArea * (eMaj_db * eBot.dy - eMaj.dy * eBot_db);
         span.blueStep = SignedFloatToFixed(dbdx);
	 dbdy = oneOverArea * (eMaj.dx * eBot_db - eMaj_db * eBot.dx);
      }
#endif
#ifdef INTERP_ALPHA
      {
         GLfloat eMaj_da, eBot_da;
         eMaj_da = (GLint) vMax->color[3] - (GLint) vMin->color[3];
         eBot_da = (GLint) vMid->color[3] - (GLint) vMin->color[3];
         dadx = oneOverArea * (eMaj_da * eBot.dy - eMaj.dy * eBot_da);
         span.alphaStep = SignedFloatToFixed(dadx);
         dady = oneOverArea * (eMaj.dx * eBot_da - eMaj_da * eBot.dx);
      }
#endif
#ifdef INTERP_SPEC
      span.activeMask |= SPAN_SPEC;
      {
         GLfloat eMaj_dsr, eBot_dsr;
         eMaj_dsr = (GLint) vMax->specular[0] - (GLint) vMin->specular[0];
         eBot_dsr = (GLint) vMid->specular[0] - (GLint) vMin->specular[0];
         dsrdx = oneOverArea * (eMaj_dsr * eBot.dy - eMaj.dy * eBot_dsr);
         span.specRedStep = SignedFloatToFixed(dsrdx);
         dsrdy = oneOverArea * (eMaj.dx * eBot_dsr - eMaj_dsr * eBot.dx);
      }
      {
         GLfloat eMaj_dsg, eBot_dsg;
         eMaj_dsg = (GLint) vMax->specular[1] - (GLint) vMin->specular[1];
	 eBot_dsg = (GLint) vMid->specular[1] - (GLint) vMin->specular[1];
         dsgdx = oneOverArea * (eMaj_dsg * eBot.dy - eMaj.dy * eBot_dsg);
         span.specGreenStep = SignedFloatToFixed(dsgdx);
         dsgdy = oneOverArea * (eMaj.dx * eBot_dsg - eMaj_dsg * eBot.dx);
      }
      {
         GLfloat eMaj_dsb, eBot_dsb;
         eMaj_dsb = (GLint) vMax->specular[2] - (GLint) vMin->specular[2];
         eBot_dsb = (GLint) vMid->specular[2] - (GLint) vMin->specular[2];
         dsbdx = oneOverArea * (eMaj_dsb * eBot.dy - eMaj.dy * eBot_dsb);
         span.specBlueStep = SignedFloatToFixed(dsbdx);
	 dsbdy = oneOverArea * (eMaj.dx * eBot_dsb - eMaj_dsb * eBot.dx);
      }
#endif
#ifdef INTERP_INDEX
      span.activeMask |= SPAN_INDEX;
      {
         GLfloat eMaj_di, eBot_di;
         eMaj_di = (GLint) vMax->index - (GLint) vMin->index;
         eBot_di = (GLint) vMid->index - (GLint) vMin->index;
         didx = oneOverArea * (eMaj_di * eBot.dy - eMaj.dy * eBot_di);
         span.indexStep = SignedFloatToFixed(didx);
         didy = oneOverArea * (eMaj.dx * eBot_di - eMaj_di * eBot.dx);
      }
#endif
#ifdef INTERP_INT_TEX
      span.activeMask |= SPAN_INT_TEXTURE;
      {
         GLfloat eMaj_ds, eBot_ds;
         eMaj_ds = (vMax->texcoord[0][0] - vMin->texcoord[0][0]) * S_SCALE;
         eBot_ds = (vMid->texcoord[0][0] - vMin->texcoord[0][0]) * S_SCALE;
         dsdx = oneOverArea * (eMaj_ds * eBot.dy - eMaj.dy * eBot_ds);
         span.intTexStep[0] = SignedFloatToFixed(dsdx);
         dsdy = oneOverArea * (eMaj.dx * eBot_ds - eMaj_ds * eBot.dx);
      }
      {
         GLfloat eMaj_dt, eBot_dt;
         eMaj_dt = (vMax->texcoord[0][1] - vMin->texcoord[0][1]) * T_SCALE;
         eBot_dt = (vMid->texcoord[0][1] - vMin->texcoord[0][1]) * T_SCALE;
         dtdx = oneOverArea * (eMaj_dt * eBot.dy - eMaj.dy * eBot_dt);
         span.intTexStep[1] = SignedFloatToFixed(dtdx);
         dtdy = oneOverArea * (eMaj.dx * eBot_dt - eMaj_dt * eBot.dx);
      }

#endif

#ifdef INTERP_TEX
      span.activeMask |= SPAN_TEXTURE;
      {
         GLfloat wMax = vMax->win[3];
         GLfloat wMin = vMin->win[3];
         GLfloat wMid = vMid->win[3];
         GLfloat eMaj_ds, eBot_ds;
         GLfloat eMaj_dt, eBot_dt;
         GLfloat eMaj_du, eBot_du;
         GLfloat eMaj_dv, eBot_dv;

         eMaj_ds = vMax->texcoord[0][0] * wMax - vMin->texcoord[0][0] * wMin;
         eBot_ds = vMid->texcoord[0][0] * wMid - vMin->texcoord[0][0] * wMin;
         span.texStep[0][0] = oneOverArea * (eMaj_ds * eBot.dy
                                             - eMaj.dy * eBot_ds);
         dsdy = oneOverArea * (eMaj.dx * eBot_ds - eMaj_ds * eBot.dx);

	 eMaj_dt = vMax->texcoord[0][1] * wMax - vMin->texcoord[0][1] * wMin;
	 eBot_dt = vMid->texcoord[0][1] * wMid - vMin->texcoord[0][1] * wMin;
	 span.texStep[0][1] = oneOverArea * (eMaj_dt * eBot.dy
                                             - eMaj.dy * eBot_dt);
	 dtdy = oneOverArea * (eMaj.dx * eBot_dt - eMaj_dt * eBot.dx);

	 eMaj_du = vMax->texcoord[0][2] * wMax - vMin->texcoord[0][2] * wMin;
	 eBot_du = vMid->texcoord[0][2] * wMid - vMin->texcoord[0][2] * wMin;
	 span.texStep[0][2] = oneOverArea * (eMaj_du * eBot.dy
                                             - eMaj.dy * eBot_du);
	 dudy = oneOverArea * (eMaj.dx * eBot_du - eMaj_du * eBot.dx);

	 eMaj_dv = vMax->texcoord[0][3] * wMax - vMin->texcoord[0][3] * wMin;
	 eBot_dv = vMid->texcoord[0][3] * wMid - vMin->texcoord[0][3] * wMin;
	 span.texStep[0][3] = oneOverArea * (eMaj_dv * eBot.dy
                                             - eMaj.dy * eBot_dv);
	 dvdy = oneOverArea * (eMaj.dx * eBot_dv - eMaj_dv * eBot.dx);
      }
#  ifdef INTERP_LAMBDA
      {
         GLfloat dudx = span.texStep[0][0] * span.texWidth[0];
         GLfloat dudy = dsdy * span.texWidth[0];
         GLfloat dvdx = span.texStep[0][1] * span.texHeight[0];
         GLfloat dvdy = dtdy * span.texHeight[0];
         GLfloat r1 = dudx * dudx + dudy * dudy;
         GLfloat r2 = dvdx * dvdx + dvdy * dvdy;
         span.rho[0] = r1 + r2; /* was rho2 = MAX2(r1,r2) */
         span.activeMask |= SPAN_LAMBDA;
      }
#  endif
#endif

#ifdef INTERP_MULTITEX
      span.activeMask |= SPAN_TEXTURE;
#  ifdef INTERP_LAMBDA
      span.activeMask |= SPAN_LAMBDA;
#  endif
      {
         GLfloat wMax = vMax->win[3];
         GLfloat wMin = vMin->win[3];
         GLfloat wMid = vMid->win[3];
         GLuint u;
         for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {
            if (ctx->Texture.Unit[u]._ReallyEnabled) {
               GLfloat eMaj_ds, eBot_ds;
               GLfloat eMaj_dt, eBot_dt;
               GLfloat eMaj_du, eBot_du;
               GLfloat eMaj_dv, eBot_dv;
               eMaj_ds = vMax->texcoord[u][0] * wMax
                       - vMin->texcoord[u][0] * wMin;
               eBot_ds = vMid->texcoord[u][0] * wMid
                       - vMin->texcoord[u][0] * wMin;
               span.texStep[u][0] = oneOverArea * (eMaj_ds * eBot.dy
                                                   - eMaj.dy * eBot_ds);
               dsdy[u] = oneOverArea * (eMaj.dx * eBot_ds - eMaj_ds * eBot.dx);

	       eMaj_dt = vMax->texcoord[u][1] * wMax
		       - vMin->texcoord[u][1] * wMin;
	       eBot_dt = vMid->texcoord[u][1] * wMid
		       - vMin->texcoord[u][1] * wMin;
	       span.texStep[u][1] = oneOverArea * (eMaj_dt * eBot.dy
                                                   - eMaj.dy * eBot_dt);
	       dtdy[u] = oneOverArea * (eMaj.dx * eBot_dt - eMaj_dt * eBot.dx);

	       eMaj_du = vMax->texcoord[u][2] * wMax
                       - vMin->texcoord[u][2] * wMin;
	       eBot_du = vMid->texcoord[u][2] * wMid
                       - vMin->texcoord[u][2] * wMin;
	       span.texStep[u][2] = oneOverArea * (eMaj_du * eBot.dy
                                                   - eMaj.dy * eBot_du);
	       dudy[u] = oneOverArea * (eMaj.dx * eBot_du - eMaj_du * eBot.dx);

	       eMaj_dv = vMax->texcoord[u][3] * wMax
                       - vMin->texcoord[u][3] * wMin;
	       eBot_dv = vMid->texcoord[u][3] * wMid
                       - vMin->texcoord[u][3] * wMin;
	       span.texStep[u][3] = oneOverArea * (eMaj_dv * eBot.dy
                                                   - eMaj.dy * eBot_dv);
	       dvdy[u] = oneOverArea * (eMaj.dx * eBot_dv - eMaj_dv * eBot.dx);
#  ifdef INTERP_LAMBDA
               {
                  GLfloat dudx = span.texStep[u][0] * span.texWidth[u];
                  GLfloat dudy = dsdy[u] * span.texWidth[u];
                  GLfloat dvdx = span.texStep[u][1] * span.texHeight[u];
                  GLfloat dvdy = dtdy[u] * span.texHeight[u];
                  GLfloat r1 = dudx * dudx + dudy * dudy;
                  GLfloat r2 = dvdx * dvdx + dvdy * dvdy;
                  span.rho[u] = r1 + r2; /* was rho2 = MAX2(r1,r2) */
               }
#  endif
            }
         }
      }
#endif

      /*
       * We always sample at pixel centers.  However, we avoid
       * explicit half-pixel offsets in this code by incorporating
       * the proper offset in each of x and y during the
       * transformation to window coordinates.
       *
       * We also apply the usual rasterization rules to prevent
       * cracks and overlaps.  A pixel is considered inside a
       * subtriangle if it meets all of four conditions: it is on or
       * to the right of the left edge, strictly to the left of the
       * right edge, on or below the top edge, and strictly above
       * the bottom edge.  (Some edges may be degenerate.)
       *
       * The following discussion assumes left-to-right scanning
       * (that is, the major edge is on the left); the right-to-left
       * case is a straightforward variation.
       *
       * We start by finding the half-integral y coordinate that is
       * at or below the top of the triangle.  This gives us the
       * first scan line that could possibly contain pixels that are
       * inside the triangle.
       *
       * Next we creep down the major edge until we reach that y,
       * and compute the corresponding x coordinate on the edge.
       * Then we find the half-integral x that lies on or just
       * inside the edge.  This is the first pixel that might lie in
       * the interior of the triangle.  (We won't know for sure
       * until we check the other edges.)
       *
       * As we rasterize the triangle, we'll step down the major
       * edge.  For each step in y, we'll move an integer number
       * of steps in x.  There are two possible x step sizes, which
       * we'll call the ``inner'' step (guaranteed to land on the
       * edge or inside it) and the ``outer'' step (guaranteed to
       * land on the edge or outside it).  The inner and outer steps
       * differ by one.  During rasterization we maintain an error
       * term that indicates our distance from the true edge, and
       * select either the inner step or the outer step, whichever
       * gets us to the first pixel that falls inside the triangle.
       *
       * All parameters (z, red, etc.) as well as the buffer
       * addresses for color and z have inner and outer step values,
       * so that we can increment them appropriately.  This method
       * eliminates the need to adjust parameters by creeping a
       * sub-pixel amount into the triangle at each scanline.
       */

      {
         int subTriangle;
         GLfixed fx;
         GLfixed fxLeftEdge, fxRightEdge, fdxLeftEdge, fdxRightEdge;
         GLfixed fdxOuter;
         int idxOuter;
         float dxOuter;
         GLfixed fError, fdError;
         float adjx, adjy;
         GLfixed fy;
#ifdef PIXEL_ADDRESS
         PIXEL_TYPE *pRow;
         int dPRowOuter, dPRowInner;  /* offset in bytes */
#endif
#ifdef INTERP_Z
#  ifdef DEPTH_TYPE
         DEPTH_TYPE *zRow;
         int dZRowOuter, dZRowInner;  /* offset in bytes */
#  endif
         GLfixed fz, fdzOuter, fdzInner;
#endif
#ifdef INTERP_FOG
         GLfloat fogLeft, dfogOuter, dfogInner;
#endif
#ifdef INTERP_RGB
         GLfixed fr, fdrOuter, fdrInner;
         GLfixed fg, fdgOuter, fdgInner;
         GLfixed fb, fdbOuter, fdbInner;
#endif
#ifdef INTERP_ALPHA
         GLfixed fa=0, fdaOuter=0, fdaInner;
#endif
#ifdef INTERP_SPEC
         GLfixed fsr=0, fdsrOuter=0, fdsrInner;
         GLfixed fsg=0, fdsgOuter=0, fdsgInner;
         GLfixed fsb=0, fdsbOuter=0, fdsbInner;
#endif
#ifdef INTERP_INDEX
         GLfixed fi=0, fdiOuter=0, fdiInner;
#endif
#ifdef INTERP_INT_TEX
         GLfixed fs=0, fdsOuter=0, fdsInner;
         GLfixed ft=0, fdtOuter=0, fdtInner;
#endif
#ifdef INTERP_TEX
         GLfloat sLeft=0, dsOuter=0, dsInner;
         GLfloat tLeft=0, dtOuter=0, dtInner;
         GLfloat uLeft=0, duOuter=0, duInner;
         GLfloat vLeft=0, dvOuter=0, dvInner;
#endif
#ifdef INTERP_MULTITEX
         GLfloat sLeft[MAX_TEXTURE_UNITS];
         GLfloat tLeft[MAX_TEXTURE_UNITS];
         GLfloat uLeft[MAX_TEXTURE_UNITS];
         GLfloat vLeft[MAX_TEXTURE_UNITS];
         GLfloat dsOuter[MAX_TEXTURE_UNITS], dsInner[MAX_TEXTURE_UNITS];
         GLfloat dtOuter[MAX_TEXTURE_UNITS], dtInner[MAX_TEXTURE_UNITS];
         GLfloat duOuter[MAX_TEXTURE_UNITS], duInner[MAX_TEXTURE_UNITS];
         GLfloat dvOuter[MAX_TEXTURE_UNITS], dvInner[MAX_TEXTURE_UNITS];
#endif

         for (subTriangle=0; subTriangle<=1; subTriangle++) {
            EdgeT *eLeft, *eRight;
            int setupLeft, setupRight;
            int lines;

            if (subTriangle==0) {
               /* bottom half */
               if (ltor) {
                  eLeft = &eMaj;
                  eRight = &eBot;
                  lines = eRight->lines;
                  setupLeft = 1;
                  setupRight = 1;
               }
               else {
                  eLeft = &eBot;
                  eRight = &eMaj;
                  lines = eLeft->lines;
                  setupLeft = 1;
                  setupRight = 1;
               }
            }
            else {
               /* top half */
               if (ltor) {
                  eLeft = &eMaj;
                  eRight = &eTop;
                  lines = eRight->lines;
                  setupLeft = 0;
                  setupRight = 1;
               }
               else {
                  eLeft = &eTop;
                  eRight = &eMaj;
                  lines = eLeft->lines;
                  setupLeft = 1;
                  setupRight = 0;
               }
               if (lines == 0)
                  return;
            }

            if (setupLeft && eLeft->lines > 0) {
               const SWvertex *vLower;
               GLfixed fsx = eLeft->fsx;
               fx = FixedCeil(fsx);
               fError = fx - fsx - FIXED_ONE;
               fxLeftEdge = fsx - FIXED_EPSILON;
               fdxLeftEdge = eLeft->fdxdy;
               fdxOuter = FixedFloor(fdxLeftEdge - FIXED_EPSILON);
               fdError = fdxOuter - fdxLeftEdge + FIXED_ONE;
               idxOuter = FixedToInt(fdxOuter);
               dxOuter = (float) idxOuter;
               (void) dxOuter;

               fy = eLeft->fsy;
               span.y = FixedToInt(fy);

               adjx = (float)(fx - eLeft->fx0);  /* SCALED! */
               adjy = eLeft->adjy;		 /* SCALED! */
               (void) adjx;  /* silence compiler warnings */
               (void) adjy;  /* silence compiler warnings */

               vLower = eLeft->v0;
               (void) vLower;  /* silence compiler warnings */

#ifdef PIXEL_ADDRESS
               {
                  pRow = (PIXEL_TYPE *) PIXEL_ADDRESS(FixedToInt(fxLeftEdge), span.y);
                  dPRowOuter = -((int)BYTES_PER_ROW) + idxOuter * sizeof(PIXEL_TYPE);
                  /* negative because Y=0 at bottom and increases upward */
               }
#endif
               /*
                * Now we need the set of parameter (z, color, etc.) values at
                * the point (fx, fy).  This gives us properly-sampled parameter
                * values that we can step from pixel to pixel.  Furthermore,
                * although we might have intermediate results that overflow
                * the normal parameter range when we step temporarily outside
                * the triangle, we shouldn't overflow or underflow for any
                * pixel that's actually inside the triangle.
                */

#ifdef INTERP_Z
               {
                  GLfloat z0 = vLower->win[2];
                  if (depthBits <= 16) {
                     /* interpolate fixed-pt values */
                     GLfloat tmp = (z0 * FIXED_SCALE +
                                    dzdx * adjx + dzdy * adjy) + FIXED_HALF;
                     if (tmp < MAX_GLUINT / 2)
                        fz = (GLfixed) tmp;
                     else
                        fz = MAX_GLUINT / 2;
                     fdzOuter = SignedFloatToFixed(dzdy + dxOuter * dzdx);
                  }
                  else {
                     /* interpolate depth values exactly */
                     fz = (GLint) (z0 + dzdx * FixedToFloat(adjx)
                                   + dzdy * FixedToFloat(adjy));
                     fdzOuter = (GLint) (dzdy + dxOuter * dzdx);
                  }
#  ifdef DEPTH_TYPE
                  zRow = (DEPTH_TYPE *)
                    _mesa_zbuffer_address(ctx, FixedToInt(fxLeftEdge), span.y);
                  dZRowOuter = (ctx->DrawBuffer->Width + idxOuter) * sizeof(DEPTH_TYPE);
#  endif
               }
#endif
#ifdef INTERP_FOG
               fogLeft = vLower->fog + (span.fogStep * adjx + dfogdy * adjy)
                                       * (1.0F/FIXED_SCALE);
               dfogOuter = dfogdy + dxOuter * span.fogStep;
#endif
#ifdef INTERP_RGB
               fr = (GLfixed)(IntToFixed(vLower->color[0])
                              + drdx * adjx + drdy * adjy) + FIXED_HALF;
               fdrOuter = SignedFloatToFixed(drdy + dxOuter * drdx);

               fg = (GLfixed)(IntToFixed(vLower->color[1])
                              + dgdx * adjx + dgdy * adjy) + FIXED_HALF;
               fdgOuter = SignedFloatToFixed(dgdy + dxOuter * dgdx);

               fb = (GLfixed)(IntToFixed(vLower->color[2])
                              + dbdx * adjx + dbdy * adjy) + FIXED_HALF;
               fdbOuter = SignedFloatToFixed(dbdy + dxOuter * dbdx);
#endif
#ifdef INTERP_ALPHA
               fa = (GLfixed)(IntToFixed(vLower->color[3])
                              + dadx * adjx + dady * adjy) + FIXED_HALF;
               fdaOuter = SignedFloatToFixed(dady + dxOuter * dadx);
#endif
#ifdef INTERP_SPEC
               fsr = (GLfixed)(IntToFixed(vLower->specular[0])
                               + dsrdx * adjx + dsrdy * adjy) + FIXED_HALF;
               fdsrOuter = SignedFloatToFixed(dsrdy + dxOuter * dsrdx);

               fsg = (GLfixed)(IntToFixed(vLower->specular[1])
                               + dsgdx * adjx + dsgdy * adjy) + FIXED_HALF;
               fdsgOuter = SignedFloatToFixed(dsgdy + dxOuter * dsgdx);

               fsb = (GLfixed)(IntToFixed(vLower->specular[2])
                               + dsbdx * adjx + dsbdy * adjy) + FIXED_HALF;
               fdsbOuter = SignedFloatToFixed(dsbdy + dxOuter * dsbdx);
#endif
#ifdef INTERP_INDEX
               fi = (GLfixed)(vLower->index * FIXED_SCALE
                              + didx * adjx + didy * adjy) + FIXED_HALF;
               fdiOuter = SignedFloatToFixed(didy + dxOuter * didx);
#endif
#ifdef INTERP_INT_TEX
               {
                  GLfloat s0, t0;
                  s0 = vLower->texcoord[0][0] * S_SCALE;
                  fs = (GLfixed)(s0 * FIXED_SCALE + dsdx * adjx
                                 + dsdy * adjy) + FIXED_HALF;
                  fdsOuter = SignedFloatToFixed(dsdy + dxOuter * dsdx);

		  t0 = vLower->texcoord[0][1] * T_SCALE;
		  ft = (GLfixed)(t0 * FIXED_SCALE + dtdx * adjx
                                 + dtdy * adjy) + FIXED_HALF;
		  fdtOuter = SignedFloatToFixed(dtdy + dxOuter * dtdx);
	       }
#endif
#ifdef INTERP_TEX
               {
                  GLfloat invW = vLower->win[3];
                  GLfloat s0, t0, u0, v0;
                  s0 = vLower->texcoord[0][0] * invW;
                  sLeft = s0 + (span.texStep[0][0] * adjx + dsdy * adjy)
                     * (1.0F/FIXED_SCALE);
                  dsOuter = dsdy + dxOuter * span.texStep[0][0];
		  t0 = vLower->texcoord[0][1] * invW;
		  tLeft = t0 + (span.texStep[0][1] * adjx + dtdy * adjy)
                     * (1.0F/FIXED_SCALE);
		  dtOuter = dtdy + dxOuter * span.texStep[0][1];
		  u0 = vLower->texcoord[0][2] * invW;
		  uLeft = u0 + (span.texStep[0][2] * adjx + dudy * adjy)
                     * (1.0F/FIXED_SCALE);
		  duOuter = dudy + dxOuter * span.texStep[0][2];
		  v0 = vLower->texcoord[0][3] * invW;
		  vLeft = v0 + (span.texStep[0][3] * adjx + dvdy * adjy)
                     * (1.0F/FIXED_SCALE);
		  dvOuter = dvdy + dxOuter * span.texStep[0][3];
               }
#endif
#ifdef INTERP_MULTITEX
               {
                  GLuint u;
                  for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {
                     if (ctx->Texture.Unit[u]._ReallyEnabled) {
                        GLfloat invW = vLower->win[3];
                        GLfloat s0, t0, u0, v0;
                        s0 = vLower->texcoord[u][0] * invW;
                        sLeft[u] = s0 + (span.texStep[u][0] * adjx + dsdy[u]
                                         * adjy) * (1.0F/FIXED_SCALE);
                        dsOuter[u] = dsdy[u] + dxOuter * span.texStep[u][0];
			t0 = vLower->texcoord[u][1] * invW;
			tLeft[u] = t0 + (span.texStep[u][1] * adjx + dtdy[u]
                                         * adjy) * (1.0F/FIXED_SCALE);
			dtOuter[u] = dtdy[u] + dxOuter * span.texStep[u][1];
			u0 = vLower->texcoord[u][2] * invW;
			uLeft[u] = u0 + (span.texStep[u][2] * adjx + dudy[u]
                                         * adjy) * (1.0F/FIXED_SCALE);
			duOuter[u] = dudy[u] + dxOuter * span.texStep[u][2];
			v0 = vLower->texcoord[u][3] * invW;
                        vLeft[u] = v0 + (span.texStep[u][3] * adjx + dvdy[u]
                                         * adjy) * (1.0F/FIXED_SCALE);
                        dvOuter[u] = dvdy[u] + dxOuter * span.texStep[u][3];
                     }
                  }
               }
#endif

            } /*if setupLeft*/


            if (setupRight && eRight->lines>0) {
               fxRightEdge = eRight->fsx - FIXED_EPSILON;
               fdxRightEdge = eRight->fdxdy;
            }

            if (lines==0) {
               continue;
            }


            /* Rasterize setup */
#ifdef PIXEL_ADDRESS
            dPRowInner = dPRowOuter + sizeof(PIXEL_TYPE);
#endif
#ifdef INTERP_Z
#  ifdef DEPTH_TYPE
            dZRowInner = dZRowOuter + sizeof(DEPTH_TYPE);
#  endif
            fdzInner = fdzOuter + span.zStep;
#endif
#ifdef INTERP_FOG
            dfogInner = dfogOuter + span.fogStep;
#endif
#ifdef INTERP_RGB
            fdrInner = fdrOuter + span.redStep;
            fdgInner = fdgOuter + span.greenStep;
            fdbInner = fdbOuter + span.blueStep;
#endif
#ifdef INTERP_ALPHA
            fdaInner = fdaOuter + span.alphaStep;
#endif
#ifdef INTERP_SPEC
            fdsrInner = fdsrOuter + span.specRedStep;
            fdsgInner = fdsgOuter + span.specGreenStep;
            fdsbInner = fdsbOuter + span.specBlueStep;
#endif
#ifdef INTERP_INDEX
            fdiInner = fdiOuter + span.indexStep;
#endif
#ifdef INTERP_INT_TEX
            fdsInner = fdsOuter + span.intTexStep[0];
            fdtInner = fdtOuter + span.intTexStep[1];
#endif
#ifdef INTERP_TEX
	    dsInner = dsOuter + span.texStep[0][0];
	    dtInner = dtOuter + span.texStep[0][1];
	    duInner = duOuter + span.texStep[0][2];
	    dvInner = dvOuter + span.texStep[0][3];
#endif
#ifdef INTERP_MULTITEX
            {
               GLuint u;
               for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {
                  if (ctx->Texture.Unit[u]._ReallyEnabled) {
                     dsInner[u] = dsOuter[u] + span.texStep[u][0];
                     dtInner[u] = dtOuter[u] + span.texStep[u][1];
                     duInner[u] = duOuter[u] + span.texStep[u][2];
                     dvInner[u] = dvOuter[u] + span.texStep[u][3];
                  }
               }
            }
#endif

            while (lines > 0) {
               /* initialize the span interpolants to the leftmost value */
               /* ff = fixed-pt fragment */
               const GLint right = FixedToInt(fxRightEdge);
               span.x = FixedToInt(fxLeftEdge);
               if (right <= span.x)
                  span.count = 0;
               else
                  span.count = right - span.x;

#ifdef INTERP_Z
               span.z = fz;
#endif
#ifdef INTERP_FOG
               span.fog = fogLeft;
#endif
#ifdef INTERP_RGB
               span.red = fr;
               span.green = fg;
               span.blue = fb;
#endif
#ifdef INTERP_ALPHA
               span.alpha = fa;
#endif
#ifdef INTERP_SPEC
               span.specRed = fsr;
               span.specGreen = fsg;
               span.specBlue = fsb;
#endif
#ifdef INTERP_INDEX
               span.index = fi;
#endif
#ifdef INTERP_INT_TEX
               span.intTex[0] = fs;
               span.intTex[1] = ft;
#endif

#ifdef INTERP_TEX
               span.tex[0][0] = sLeft;
               span.tex[0][1] = tLeft;
               span.tex[0][2] = uLeft;
               span.tex[0][3] = vLeft;
#endif

#ifdef INTERP_MULTITEX
               {
                  GLuint u;
                  for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {
                     if (ctx->Texture.Unit[u]._ReallyEnabled) {
                        span.tex[u][0] = sLeft[u];
                        span.tex[u][1] = tLeft[u];
                        span.tex[u][2] = uLeft[u];
                        span.tex[u][3] = vLeft[u];
                     }
                  }
               }
#endif

#ifdef INTERP_RGB
               {
                  /* need this to accomodate round-off errors */
                  const GLint len = right - span.x - 1;
                  GLfixed ffrend = span.red + len * span.redStep;
                  GLfixed ffgend = span.green + len * span.greenStep;
                  GLfixed ffbend = span.blue + len * span.blueStep;
                  if (ffrend < 0) {
                     span.red -= ffrend;
                     if (span.red < 0)
                        span.red = 0;
                  }
                  if (ffgend < 0) {
                     span.green -= ffgend;
                     if (span.green < 0)
                        span.green = 0;
                  }
                  if (ffbend < 0) {
                     span.blue -= ffbend;
                     if (span.blue < 0)
                        span.blue = 0;
                  }
               }
#endif
#ifdef INTERP_ALPHA
               {
                  const GLint len = right - span.x - 1;
                  GLfixed ffaend = span.alpha + len * span.alphaStep;
                  if (ffaend < 0) {
                     span.alpha -= ffaend;
                     if (span.alpha < 0)
                        span.alpha = 0;
                  }
               }
#endif
#ifdef INTERP_SPEC
               {
                  /* need this to accomodate round-off errors */
                  const GLint len = right - span.x - 1;
                  GLfixed ffsrend = span.specRed + len * span.specRedStep;
                  GLfixed ffsgend = span.specGreen + len * span.specGreenStep;
                  GLfixed ffsbend = span.specBlue + len * span.specBlueStep;
                  if (ffsrend < 0) {
                     span.specRed -= ffsrend;
                     if (span.specRed < 0)
                        span.specRed = 0;
                  }
                  if (ffsgend < 0) {
                     span.specGreen -= ffsgend;
                     if (span.specGreen < 0)
                        span.specGreen = 0;
                  }
                  if (ffsbend < 0) {
                     span.specBlue -= ffsbend;
                     if (span.specBlue < 0)
                        span.specBlue = 0;
                  }
               }
#endif
#ifdef INTERP_INDEX
               if (span.index < 0)  span.index = 0;
#endif

               /* This is where we actually generate fragments */
               if (span.count > 0) {
                  RENDER_SPAN( span );
               }

               /*
                * Advance to the next scan line.  Compute the
                * new edge coordinates, and adjust the
                * pixel-center x coordinate so that it stays
                * on or inside the major edge.
                */
               span.y++;
               lines--;

               fxLeftEdge += fdxLeftEdge;
               fxRightEdge += fdxRightEdge;


               fError += fdError;
               if (fError >= 0) {
                  fError -= FIXED_ONE;
#ifdef PIXEL_ADDRESS
                  pRow = (PIXEL_TYPE *) ((GLubyte *) pRow + dPRowOuter);
#endif
#ifdef INTERP_Z
#  ifdef DEPTH_TYPE
                  zRow = (DEPTH_TYPE *) ((GLubyte *) zRow + dZRowOuter);
#  endif
                  fz += fdzOuter;
#endif
#ifdef INTERP_FOG
                  fogLeft += dfogOuter;
#endif
#ifdef INTERP_RGB
                  fr += fdrOuter;
                  fg += fdgOuter;
                  fb += fdbOuter;
#endif
#ifdef INTERP_ALPHA
                  fa += fdaOuter;
#endif
#ifdef INTERP_SPEC
                  fsr += fdsrOuter;
                  fsg += fdsgOuter;
                  fsb += fdsbOuter;
#endif
#ifdef INTERP_INDEX
                  fi += fdiOuter;
#endif
#ifdef INTERP_INT_TEX
                  fs += fdsOuter;
                  ft += fdtOuter;
#endif
#ifdef INTERP_TEX
		  sLeft += dsOuter;
		  tLeft += dtOuter;
		  uLeft += duOuter;
		  vLeft += dvOuter;
#endif
#ifdef INTERP_MULTITEX
                  {
                     GLuint u;
                     for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {
                        if (ctx->Texture.Unit[u]._ReallyEnabled) {
                           sLeft[u] += dsOuter[u];
                           tLeft[u] += dtOuter[u];
                           uLeft[u] += duOuter[u];
                           vLeft[u] += dvOuter[u];
                        }
                     }
                  }
#endif
               }
               else {
#ifdef PIXEL_ADDRESS
                  pRow = (PIXEL_TYPE *) ((GLubyte *) pRow + dPRowInner);
#endif
#ifdef INTERP_Z
#  ifdef DEPTH_TYPE
                  zRow = (DEPTH_TYPE *) ((GLubyte *) zRow + dZRowInner);
#  endif
                  fz += fdzInner;
#endif
#ifdef INTERP_FOG
                  fogLeft += dfogInner;
#endif
#ifdef INTERP_RGB
                  fr += fdrInner;
                  fg += fdgInner;
                  fb += fdbInner;
#endif
#ifdef INTERP_ALPHA
                  fa += fdaInner;
#endif
#ifdef INTERP_SPEC
                  fsr += fdsrInner;
                  fsg += fdsgInner;
                  fsb += fdsbInner;
#endif
#ifdef INTERP_INDEX
                  fi += fdiInner;
#endif
#ifdef INTERP_INT_TEX
                  fs += fdsInner;
                  ft += fdtInner;
#endif
#ifdef INTERP_TEX
		  sLeft += dsInner;
		  tLeft += dtInner;
		  uLeft += duInner;
		  vLeft += dvInner;
#endif
#ifdef INTERP_MULTITEX
                  {
                     GLuint u;
                     for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {
                        if (ctx->Texture.Unit[u]._ReallyEnabled) {
                           sLeft[u] += dsInner[u];
                           tLeft[u] += dtInner[u];
                           uLeft[u] += duInner[u];
                           vLeft[u] += dvInner[u];
                        }
                     }
                  }
#endif
               }
            } /*while lines>0*/

         } /* for subTriangle */

      }
#ifdef CLEANUP_CODE
      CLEANUP_CODE
#endif
   }
}

#undef SETUP_CODE
#undef CLEANUP_CODE
#undef RENDER_SPAN

#undef PIXEL_TYPE
#undef BYTES_PER_ROW
#undef PIXEL_ADDRESS

#undef INTERP_Z
#undef INTERP_FOG
#undef INTERP_RGB
#undef INTERP_ALPHA
#undef INTERP_SPEC
#undef INTERP_INDEX
#undef INTERP_INT_TEX
#undef INTERP_TEX
#undef INTERP_MULTITEX
#undef INTERP_LAMBDA

#undef S_SCALE
#undef T_SCALE

#undef FixedToDepth

#undef DO_OCCLUSION_TEST

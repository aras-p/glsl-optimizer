/* $Id: s_tritemp.h,v 1.8 2001/01/29 18:51:25 brianp Exp $ */

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
 *    INTERP_RGB      - if defined, interpolate RGB values
 *    INTERP_SPEC     - if defined, interpolate specular RGB values
 *    INTERP_ALPHA    - if defined, interpolate Alpha values
 *    INTERP_INDEX    - if defined, interpolate color index values
 *    INTERP_INT_TEX  - if defined, interpolate integer ST texcoords
 *                         (fast, simple 2-D texture mapping)
 *    INTERP_LAMBDA   - if defined, the lambda value is computed at every
 *                         pixel, to apply MIPMAPPING, and min/maxification
 *    INTERP_TEX      - if defined, interpolate set 0 float STRQ texcoords
 *                         NOTE:  OpenGL STRQ = Mesa STUV (R was taken for red)
 *    INTERP_MULTITEX - if defined, interpolate N units of STRQ texcoords
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
 * 
 * The following macro MUST be defined:
 *    INNER_LOOP(LEFT,RIGHT,Y) - code to write a span of pixels.
 *        Something like:
 *
 *                    for (x=LEFT; x<RIGHT;x++) {
 *                       put_pixel(x,Y);
 *                       // increment fixed point interpolants
 *                    }
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

   /* find the order of the 3 vertices along the Y axis */
   {
      GLfloat y0 = v0->win[1];
      GLfloat y1 = v1->win[1];
      GLfloat y2 = v2->win[1];

      if (y0<=y1) {
	 if (y1<=y2) {
	    vMin = v0;   vMid = v1;   vMax = v2;   /* y0<=y1<=y2 */
	 }
	 else if (y2<=y0) {
	    vMin = v2;   vMid = v0;   vMax = v1;   /* y2<=y0<=y1 */
	 }
	 else {
	    vMin = v0;   vMid = v2;   vMax = v1;  bf = -bf; /* y0<=y2<=y1 */
	 }
      }
      else {
	 if (y0<=y2) {
	    vMin = v1;   vMid = v0;   vMax = v2;  bf = -bf; /* y1<=y0<=y2 */
	 }
	 else if (y2<=y1) {
	    vMin = v2;   vMid = v1;   vMax = v0;  bf = -bf; /* y2<=y1<=y0 */
	 }
	 else {
	    vMin = v1;   vMid = v2;   vMax = v0;   /* y1<=y2<=y0 */
	 }
      }
   }

   /* vertex/edge relationship */
   eMaj.v0 = vMin;   eMaj.v1 = vMax;   /*TODO: .v1's not needed */
   eTop.v0 = vMid;   eTop.v1 = vMax;
   eBot.v0 = vMin;   eBot.v1 = vMid;

   /* compute deltas for each edge:  vertex[v1] - vertex[v0] */
   eMaj.dx = vMax->win[0] - vMin->win[0];
   eMaj.dy = vMax->win[1] - vMin->win[1];
   eTop.dx = vMax->win[0] - vMid->win[0];
   eTop.dy = vMax->win[1] - vMid->win[1];
   eBot.dx = vMid->win[0] - vMin->win[0];
   eBot.dy = vMid->win[1] - vMin->win[1];

   /* compute oneOverArea */
   {
      const GLfloat area = eMaj.dx * eBot.dy - eBot.dx * eMaj.dy;

      /* Do backface culling */
      if (area * bf < 0.0)
	 return;

      if (area == 0.0F)
         return;

      /* check for very tiny triangle */
      if (area * area < (0.05F * 0.05F))  /* square to ensure positive value */
         oneOverArea = 1.0F / 0.05F;  /* a close-enough value */
      else
         oneOverArea = 1.0F / area;
   }

#ifndef DO_OCCLUSION_TEST
   ctx->OcclusionResult = GL_TRUE;
#endif

   /* Edge setup.  For a triangle strip these could be reused... */
   {
      /* fixed point Y coordinates */
      GLfixed vMin_fx = FloatToFixed(vMin->win[0] + 0.5F);
      GLfixed vMin_fy = FloatToFixed(vMin->win[1] - 0.5F);
      GLfixed vMid_fx = FloatToFixed(vMid->win[0] + 0.5F);
      GLfixed vMid_fy = FloatToFixed(vMid->win[1] - 0.5F);
      GLfixed vMax_fy = FloatToFixed(vMax->win[1] - 0.5F);

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
      GLfloat dzdx, dzdy;      GLfixed fdzdx;
      GLfloat dfogdx, dfogdy;      GLfixed fdfogdx;
#endif
#ifdef INTERP_RGB
      GLfloat drdx, drdy;      GLfixed fdrdx;
      GLfloat dgdx, dgdy;      GLfixed fdgdx;
      GLfloat dbdx, dbdy;      GLfixed fdbdx;
#endif
#ifdef INTERP_SPEC
      GLfloat dsrdx, dsrdy;    GLfixed fdsrdx;
      GLfloat dsgdx, dsgdy;    GLfixed fdsgdx;
      GLfloat dsbdx, dsbdy;    GLfixed fdsbdx;
#endif
#ifdef INTERP_ALPHA
      GLfloat dadx, dady;      GLfixed fdadx;
#endif
#ifdef INTERP_INDEX
      GLfloat didx, didy;      GLfixed fdidx;
#endif
#ifdef INTERP_INT_TEX
      GLfloat dsdx, dsdy;      GLfixed fdsdx;
      GLfloat dtdx, dtdy;      GLfixed fdtdx;
#endif
#ifdef INTERP_TEX
      GLfloat dsdx, dsdy;
      GLfloat dtdx, dtdy;
      GLfloat dudx, dudy;
      GLfloat dvdx, dvdy;
#endif
#ifdef INTERP_MULTITEX
      GLfloat dsdx[MAX_TEXTURE_UNITS], dsdy[MAX_TEXTURE_UNITS];
      GLfloat dtdx[MAX_TEXTURE_UNITS], dtdy[MAX_TEXTURE_UNITS];
      GLfloat dudx[MAX_TEXTURE_UNITS], dudy[MAX_TEXTURE_UNITS];
      GLfloat dvdx[MAX_TEXTURE_UNITS], dvdy[MAX_TEXTURE_UNITS];
#endif
#ifdef INTERP_LAMBDA

#ifndef INTERP_TEX
#error "Mipmapping without texturing doesn't make sense."
#endif
      GLfloat lambda_nominator;
#endif


      /*
       * Execute user-supplied setup code
       */
#ifdef SETUP_CODE
      SETUP_CODE
#endif

      ltor = (oneOverArea < 0.0F);

      /* compute d?/dx and d?/dy derivatives */
#ifdef INTERP_Z
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
            fdzdx = SignedFloatToFixed(dzdx);
         else
            fdzdx = (GLint) dzdx;
      }
      {
         GLfloat eMaj_dfog, eBot_dfog;
         eMaj_dfog = (vMax->fog - vMin->fog) * 256;
         eBot_dfog = (vMid->fog - vMin->fog) * 256;
         dfogdx = oneOverArea * (eMaj_dfog * eBot.dy - eMaj.dy * eBot_dfog);
         fdfogdx = SignedFloatToFixed(dfogdx);
         dfogdy = oneOverArea * (eMaj.dx * eBot_dfog - eMaj_dfog * eBot.dx);
      }
#endif
#ifdef INTERP_RGB
      {
         GLfloat eMaj_dr, eBot_dr;
         eMaj_dr = (GLint) vMax->color[0]
                 - (GLint) vMin->color[0];
         eBot_dr = (GLint) vMid->color[0]
                 - (GLint) vMin->color[0];
         drdx = oneOverArea * (eMaj_dr * eBot.dy - eMaj.dy * eBot_dr);
         fdrdx = SignedFloatToFixed(drdx);
         drdy = oneOverArea * (eMaj.dx * eBot_dr - eMaj_dr * eBot.dx);
      }
      {
         GLfloat eMaj_dg, eBot_dg;
         eMaj_dg = (GLint) vMax->color[1]
                 - (GLint) vMin->color[1];
	 eBot_dg = (GLint) vMid->color[1]
                 - (GLint) vMin->color[1];
         dgdx = oneOverArea * (eMaj_dg * eBot.dy - eMaj.dy * eBot_dg);
         fdgdx = SignedFloatToFixed(dgdx);
         dgdy = oneOverArea * (eMaj.dx * eBot_dg - eMaj_dg * eBot.dx);
      }
      {
         GLfloat eMaj_db, eBot_db;
         eMaj_db = (GLint) vMax->color[2]
                 - (GLint) vMin->color[2];
         eBot_db = (GLint) vMid->color[2]
                 - (GLint) vMin->color[2];
         dbdx = oneOverArea * (eMaj_db * eBot.dy - eMaj.dy * eBot_db);
         fdbdx = SignedFloatToFixed(dbdx);
	 dbdy = oneOverArea * (eMaj.dx * eBot_db - eMaj_db * eBot.dx);
      }
#endif
#ifdef INTERP_SPEC
      {
         GLfloat eMaj_dsr, eBot_dsr;
         eMaj_dsr = (GLint) vMax->specular[0]
                  - (GLint) vMin->specular[0];
         eBot_dsr = (GLint) vMid->specular[0]
                  - (GLint) vMin->specular[0];
         dsrdx = oneOverArea * (eMaj_dsr * eBot.dy - eMaj.dy * eBot_dsr);
         fdsrdx = SignedFloatToFixed(dsrdx);
         dsrdy = oneOverArea * (eMaj.dx * eBot_dsr - eMaj_dsr * eBot.dx);
      }
      {
         GLfloat eMaj_dsg, eBot_dsg;
         eMaj_dsg = (GLint) vMax->specular[1]
                  - (GLint) vMin->specular[1];
	 eBot_dsg = (GLint) vMid->specular[1]
                  - (GLint) vMin->specular[1];
         dsgdx = oneOverArea * (eMaj_dsg * eBot.dy - eMaj.dy * eBot_dsg);
         fdsgdx = SignedFloatToFixed(dsgdx);
         dsgdy = oneOverArea * (eMaj.dx * eBot_dsg - eMaj_dsg * eBot.dx);
      }
      {
         GLfloat eMaj_dsb, eBot_dsb;
         eMaj_dsb = (GLint) vMax->specular[2]
                  - (GLint) vMin->specular[2];
         eBot_dsb = (GLint) vMid->specular[2]
                  - (GLint) vMin->specular[2];
         dsbdx = oneOverArea * (eMaj_dsb * eBot.dy - eMaj.dy * eBot_dsb);
         fdsbdx = SignedFloatToFixed(dsbdx);
	 dsbdy = oneOverArea * (eMaj.dx * eBot_dsb - eMaj_dsb * eBot.dx);
      }
#endif
#ifdef INTERP_ALPHA
      {
         GLfloat eMaj_da, eBot_da;
         eMaj_da = (GLint) vMax->color[3]
                 - (GLint) vMin->color[3];
         eBot_da = (GLint) vMid->color[3]
                 - (GLint) vMin->color[3];
         dadx = oneOverArea * (eMaj_da * eBot.dy - eMaj.dy * eBot_da);
         fdadx = SignedFloatToFixed(dadx);
         dady = oneOverArea * (eMaj.dx * eBot_da - eMaj_da * eBot.dx);
      }
#endif
#ifdef INTERP_INDEX
      {
         GLfloat eMaj_di, eBot_di;
         eMaj_di = (GLint) vMax->index
                 - (GLint) vMin->index;
         eBot_di = (GLint) vMid->index
                 - (GLint) vMin->index;
         didx = oneOverArea * (eMaj_di * eBot.dy - eMaj.dy * eBot_di);
         fdidx = SignedFloatToFixed(didx);
         didy = oneOverArea * (eMaj.dx * eBot_di - eMaj_di * eBot.dx);
      }
#endif
#ifdef INTERP_INT_TEX
      {
         GLfloat eMaj_ds, eBot_ds;
         eMaj_ds = (vMax->texcoord[0][0] - vMin->texcoord[0][0]) * S_SCALE;
         eBot_ds = (vMid->texcoord[0][0] - vMin->texcoord[0][0]) * S_SCALE;
         dsdx = oneOverArea * (eMaj_ds * eBot.dy - eMaj.dy * eBot_ds);
         fdsdx = SignedFloatToFixed(dsdx);
         dsdy = oneOverArea * (eMaj.dx * eBot_ds - eMaj_ds * eBot.dx);
      }
      {
         GLfloat eMaj_dt, eBot_dt;
         eMaj_dt = (vMax->texcoord[0][1] - vMin->texcoord[0][1]) * T_SCALE;
         eBot_dt = (vMid->texcoord[0][1] - vMin->texcoord[0][1]) * T_SCALE;
         dtdx = oneOverArea * (eMaj_dt * eBot.dy - eMaj.dy * eBot_dt);
         fdtdx = SignedFloatToFixed(dtdx);
         dtdy = oneOverArea * (eMaj.dx * eBot_dt - eMaj_dt * eBot.dx);
      }

#endif
#ifdef INTERP_TEX
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
         dsdx = oneOverArea * (eMaj_ds * eBot.dy - eMaj.dy * eBot_ds);
         dsdy = oneOverArea * (eMaj.dx * eBot_ds - eMaj_ds * eBot.dx);

	 eMaj_dt = vMax->texcoord[0][1] * wMax - vMin->texcoord[0][1] * wMin;
	 eBot_dt = vMid->texcoord[0][1] * wMid - vMin->texcoord[0][1] * wMin;
	 dtdx = oneOverArea * (eMaj_dt * eBot.dy - eMaj.dy * eBot_dt);
	 dtdy = oneOverArea * (eMaj.dx * eBot_dt - eMaj_dt * eBot.dx);
	 
	 eMaj_du = vMax->texcoord[0][2] * wMax - vMin->texcoord[0][2] * wMin;
	 eBot_du = vMid->texcoord[0][2] * wMid - vMin->texcoord[0][2] * wMin;
	 dudx = oneOverArea * (eMaj_du * eBot.dy - eMaj.dy * eBot_du);
	 dudy = oneOverArea * (eMaj.dx * eBot_du - eMaj_du * eBot.dx);


	 eMaj_dv = vMax->texcoord[0][3] * wMax - vMin->texcoord[0][3] * wMin;
	 eBot_dv = vMid->texcoord[0][3] * wMid - vMin->texcoord[0][3] * wMin;
	 dvdx = oneOverArea * (eMaj_dv * eBot.dy - eMaj.dy * eBot_dv);
	 dvdy = oneOverArea * (eMaj.dx * eBot_dv - eMaj_dv * eBot.dx);
      }
#endif
#ifdef INTERP_MULTITEX
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
               dsdx[u] = oneOverArea * (eMaj_ds * eBot.dy - eMaj.dy * eBot_ds);
               dsdy[u] = oneOverArea * (eMaj.dx * eBot_ds - eMaj_ds * eBot.dx);

	       eMaj_dt = vMax->texcoord[u][1] * wMax
		       - vMin->texcoord[u][1] * wMin;
	       eBot_dt = vMid->texcoord[u][1] * wMid
		       - vMin->texcoord[u][1] * wMin;
	       dtdx[u] = oneOverArea * (eMaj_dt * eBot.dy - eMaj.dy * eBot_dt);
	       dtdy[u] = oneOverArea * (eMaj.dx * eBot_dt - eMaj_dt * eBot.dx);
	       
	       eMaj_du = vMax->texcoord[u][2] * wMax
                       - vMin->texcoord[u][2] * wMin;
	       eBot_du = vMid->texcoord[u][2] * wMid
                       - vMin->texcoord[u][2] * wMin;
	       dudx[u] = oneOverArea * (eMaj_du * eBot.dy - eMaj.dy * eBot_du);
	       dudy[u] = oneOverArea * (eMaj.dx * eBot_du - eMaj_du * eBot.dx);
	       
	       eMaj_dv = vMax->texcoord[u][3] * wMax
                       - vMin->texcoord[u][3] * wMin;
	       eBot_dv = vMid->texcoord[u][3] * wMid
                       - vMin->texcoord[u][3] * wMin;
	       dvdx[u] = oneOverArea * (eMaj_dv * eBot.dy - eMaj.dy * eBot_dv);
	       dvdy[u] = oneOverArea * (eMaj.dx * eBot_dv - eMaj_dv * eBot.dx);
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
         GLfixed fx, fxLeftEdge, fxRightEdge, fdxLeftEdge, fdxRightEdge;
         GLfixed fdxOuter;
         int idxOuter;
         float dxOuter;
         GLfixed fError, fdError;
         float adjx, adjy;
         GLfixed fy;
         int iy;
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
         GLfixed ffog, fdfogOuter, fdfogInner;
#endif
#ifdef INTERP_RGB
         GLfixed fr, fdrOuter, fdrInner;
         GLfixed fg, fdgOuter, fdgInner;
         GLfixed fb, fdbOuter, fdbInner;
#endif
#ifdef INTERP_SPEC
         GLfixed fsr, fdsrOuter, fdsrInner;
         GLfixed fsg, fdsgOuter, fdsgInner;
         GLfixed fsb, fdsbOuter, fdsbInner;
#endif
#ifdef INTERP_ALPHA
         GLfixed fa, fdaOuter, fdaInner;
#endif
#ifdef INTERP_INDEX
         GLfixed fi, fdiOuter, fdiInner;
#endif
#ifdef INTERP_INT_TEX
         GLfixed fs, fdsOuter, fdsInner;
         GLfixed ft, fdtOuter, fdtInner;
#endif
#ifdef INTERP_TEX
         GLfloat sLeft, dsOuter, dsInner;
         GLfloat tLeft, dtOuter, dtInner;
         GLfloat uLeft, duOuter, duInner;
         GLfloat vLeft, dvOuter, dvInner;
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
               iy = FixedToInt(fy);

               adjx = (float)(fx - eLeft->fx0);  /* SCALED! */
               adjy = eLeft->adjy;		 /* SCALED! */
               (void) adjx;  /* silence compiler warnings */
               (void) adjy;  /* silence compiler warnings */

               vLower = eLeft->v0;
               (void) vLower;  /* silence compiler warnings */

#ifdef PIXEL_ADDRESS
               {
                  pRow = PIXEL_ADDRESS( FixedToInt(fxLeftEdge), iy );
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
                     fz = (GLint) (z0 + dzdx*FixedToFloat(adjx) + dzdy*FixedToFloat(adjy));
                     fdzOuter = (GLint) (dzdy + dxOuter * dzdx);
                  }
#  ifdef DEPTH_TYPE
                  zRow = (DEPTH_TYPE *) _mesa_zbuffer_address(ctx, FixedToInt(fxLeftEdge), iy);
                  dZRowOuter = (ctx->DrawBuffer->Width + idxOuter) * sizeof(DEPTH_TYPE);
#  endif
               }
	       {
		  ffog = FloatToFixed(vLower->fog) * 256 + dfogdx * adjx + dfogdy * adjy + FIXED_HALF;
		  fdfogOuter = SignedFloatToFixed(dfogdy + dxOuter * dfogdx);
	       }
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
#ifdef INTERP_ALPHA
               fa = (GLfixed)(IntToFixed(vLower->color[3])
                              + dadx * adjx + dady * adjy) + FIXED_HALF;
               fdaOuter = SignedFloatToFixed(dady + dxOuter * dadx);
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
                  fs = (GLfixed)(s0 * FIXED_SCALE + dsdx * adjx + dsdy * adjy) + FIXED_HALF;
                  fdsOuter = SignedFloatToFixed(dsdy + dxOuter * dsdx);

		  t0 = vLower->texcoord[0][1] * T_SCALE;
		  ft = (GLfixed)(t0 * FIXED_SCALE + dtdx * adjx + dtdy * adjy) + FIXED_HALF;
		  fdtOuter = SignedFloatToFixed(dtdy + dxOuter * dtdx);
	       }
#endif
#ifdef INTERP_TEX
               {
                  GLfloat invW = vLower->win[3];
                  GLfloat s0, t0, u0, v0;
                  s0 = vLower->texcoord[0][0] * invW;
                  sLeft = s0 + (dsdx * adjx + dsdy * adjy) * (1.0F/FIXED_SCALE);
                  dsOuter = dsdy + dxOuter * dsdx;
		  t0 = vLower->texcoord[0][1] * invW;
		  tLeft = t0 + (dtdx * adjx + dtdy * adjy) * (1.0F/FIXED_SCALE);
		  dtOuter = dtdy + dxOuter * dtdx;
		  u0 = vLower->texcoord[0][2] * invW;
		  uLeft = u0 + (dudx * adjx + dudy * adjy) * (1.0F/FIXED_SCALE);
		  duOuter = dudy + dxOuter * dudx;
		  v0 = vLower->texcoord[0][3] * invW;
		  vLeft = v0 + (dvdx * adjx + dvdy * adjy) * (1.0F/FIXED_SCALE);
		  dvOuter = dvdy + dxOuter * dvdx;
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
                        sLeft[u] = s0 + (dsdx[u] * adjx + dsdy[u] * adjy) * (1.0F/FIXED_SCALE);
                        dsOuter[u] = dsdy[u] + dxOuter * dsdx[u];
			t0 = vLower->texcoord[u][1] * invW;
			tLeft[u] = t0 + (dtdx[u] * adjx + dtdy[u] * adjy) * (1.0F/FIXED_SCALE);
			dtOuter[u] = dtdy[u] + dxOuter * dtdx[u];
			u0 = vLower->texcoord[u][2] * invW;
			uLeft[u] = u0 + (dudx[u] * adjx + dudy[u] * adjy) * (1.0F/FIXED_SCALE);
			duOuter[u] = dudy[u] + dxOuter * dudx[u];
			v0 = vLower->texcoord[u][3] * invW;
                        vLeft[u] = v0 + (dvdx[u] * adjx + dvdy[u] * adjy) * (1.0F/FIXED_SCALE);
                        dvOuter[u] = dvdy[u] + dxOuter * dvdx[u];
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
            fdzInner = fdzOuter + fdzdx;
            fdfogInner = fdfogOuter + fdfogdx;
#endif
#ifdef INTERP_RGB
            fdrInner = fdrOuter + fdrdx;
            fdgInner = fdgOuter + fdgdx;
            fdbInner = fdbOuter + fdbdx;
#endif
#ifdef INTERP_SPEC
            fdsrInner = fdsrOuter + fdsrdx;
            fdsgInner = fdsgOuter + fdsgdx;
            fdsbInner = fdsbOuter + fdsbdx;
#endif
#ifdef INTERP_ALPHA
            fdaInner = fdaOuter + fdadx;
#endif
#ifdef INTERP_INDEX
            fdiInner = fdiOuter + fdidx;
#endif
#ifdef INTERP_INT_TEX
            fdsInner = fdsOuter + fdsdx;
            fdtInner = fdtOuter + fdtdx;
#endif
#ifdef INTERP_TEX
	    dsInner = dsOuter + dsdx;
	    dtInner = dtOuter + dtdx;
	    duInner = duOuter + dudx;
	    dvInner = dvOuter + dvdx;
#endif
#ifdef INTERP_MULTITEX
            {
               GLuint u;
               for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {
                  if (ctx->Texture.Unit[u]._ReallyEnabled) {
                     dsInner[u] = dsOuter[u] + dsdx[u];
                     dtInner[u] = dtOuter[u] + dtdx[u];
                     duInner[u] = duOuter[u] + dudx[u];
                     dvInner[u] = dvOuter[u] + dvdx[u];
                  }
               }
            }
#endif

            while (lines>0) {
               /* initialize the span interpolants to the leftmost value */
               /* ff = fixed-pt fragment */
               GLint left = FixedToInt(fxLeftEdge);
               GLint right = FixedToInt(fxRightEdge);
#ifdef INTERP_Z
               GLfixed ffz = fz;
               GLfixed fffog = ffog;
#endif
#ifdef INTERP_RGB
               GLfixed ffr = fr,  ffg = fg,  ffb = fb;
#endif
#ifdef INTERP_SPEC
               GLfixed ffsr = fsr,  ffsg = fsg,  ffsb = fsb;
#endif
#ifdef INTERP_ALPHA
               GLfixed ffa = fa;
#endif
#ifdef INTERP_INDEX
               GLfixed ffi = fi;
#endif
#ifdef INTERP_INT_TEX
               GLfixed ffs = fs,  fft = ft;
#endif
#ifdef INTERP_TEX
               GLfloat ss = sLeft, tt = tLeft, uu = uLeft, vv = vLeft;
#endif
#ifdef INTERP_MULTITEX
               GLfloat ss[MAX_TEXTURE_UNITS];
               GLfloat tt[MAX_TEXTURE_UNITS];
               GLfloat uu[MAX_TEXTURE_UNITS];
               GLfloat vv[MAX_TEXTURE_UNITS];
               {
                  GLuint u;
                  for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {
                     if (ctx->Texture.Unit[u]._ReallyEnabled) {
                        ss[u] = sLeft[u];
                        tt[u] = tLeft[u];
                        uu[u] = uLeft[u];
                        vv[u] = vLeft[u];
                     }
                  }
               }
#endif

#ifdef INTERP_RGB
               {
                  /* need this to accomodate round-off errors */
                  GLfixed ffrend = ffr+(right-left-1)*fdrdx;
                  GLfixed ffgend = ffg+(right-left-1)*fdgdx;
                  GLfixed ffbend = ffb+(right-left-1)*fdbdx;
                  if (ffrend<0) ffr -= ffrend;
                  if (ffgend<0) ffg -= ffgend;
                  if (ffbend<0) ffb -= ffbend;
                  if (ffr<0) ffr = 0;
                  if (ffg<0) ffg = 0;
                  if (ffb<0) ffb = 0;
               }
#endif
#ifdef INTERP_SPEC
               {
                  /* need this to accomodate round-off errors */
                  GLfixed ffsrend = ffsr+(right-left-1)*fdsrdx;
                  GLfixed ffsgend = ffsg+(right-left-1)*fdsgdx;
                  GLfixed ffsbend = ffsb+(right-left-1)*fdsbdx;
                  if (ffsrend<0) ffsr -= ffsrend;
                  if (ffsgend<0) ffsg -= ffsgend;
                  if (ffsbend<0) ffsb -= ffsbend;
                  if (ffsr<0) ffsr = 0;
                  if (ffsg<0) ffsg = 0;
                  if (ffsb<0) ffsb = 0;
               }
#endif
#ifdef INTERP_ALPHA
               {
                  GLfixed ffaend = ffa+(right-left-1)*fdadx;
                  if (ffaend<0) ffa -= ffaend;
                  if (ffa<0) ffa = 0;
               }
#endif
#ifdef INTERP_INDEX
               if (ffi<0) ffi = 0;
#endif

#ifdef INTERP_LAMBDA
/*
 * The lambda value is:
 *        log_2(sqrt(f(n))) = 1/2*log_2(f(n)), where f(n) is a function
 *     defined by
 *        f(n):=  dudx * dudx + dudy * dudy  +  dvdx * dvdx + dvdy * dvdy;
 *     and each of this terms is resp.
 *        dudx = dsdx * invQ(n) * tex_width;
 *        dudy = dsdy * invQ(n) * tex_width;
 *        dvdx = dtdx * invQ(n) * tex_height;
 *        dvdy = dtdy * invQ(n) * tex_height;
 *     Therefore the function lambda can be represented (by factoring out) as:
 *        f(n) = lambda_nominator * invQ(n) * invQ(n),
 *     which saves some computation time.
 */
	       {
		 GLfloat dudx = dsdx /* * invQ*/ * twidth;
		 GLfloat dudy = dsdy /* * invQ*/ * twidth;
		 GLfloat dvdx = dtdx /* * invQ*/ * theight;
		 GLfloat dvdy = dtdy /* * invQ*/ * theight;
		 GLfloat r1 = dudx * dudx + dudy * dudy;
		 GLfloat r2 = dvdx * dvdx + dvdy * dvdy;
		 GLfloat rho2 = r1 + r2; /* used to be:  rho2 = MAX2(r1,r2); */
		 lambda_nominator = rho2;
	       }
	       
	       /* return log base 2 of sqrt(rho) */ 
#define COMPUTE_LAMBDA(X)  log( lambda_nominator * (X)*(X) ) * 1.442695F * 0.5F  /* 1.442695 = 1/log(2) */
#endif

               INNER_LOOP( left, right, iy );

               /*
                * Advance to the next scan line.  Compute the
                * new edge coordinates, and adjust the
                * pixel-center x coordinate so that it stays
                * on or inside the major edge.
                */
               iy++;
               lines--;

               fxLeftEdge += fdxLeftEdge;
               fxRightEdge += fdxRightEdge;


               fError += fdError;
               if (fError >= 0) {
                  fError -= FIXED_ONE;
#ifdef PIXEL_ADDRESS
                  pRow = (PIXEL_TYPE *) ((GLubyte*)pRow + dPRowOuter);
#endif
#ifdef INTERP_Z
#  ifdef DEPTH_TYPE
                  zRow = (DEPTH_TYPE *) ((GLubyte*)zRow + dZRowOuter);
#  endif
                  fz += fdzOuter;
                  ffog += fdfogOuter; 
#endif
#ifdef INTERP_RGB
                  fr += fdrOuter;   fg += fdgOuter;   fb += fdbOuter;
#endif
#ifdef INTERP_SPEC
                  fsr += fdsrOuter;   fsg += fdsgOuter;   fsb += fdsbOuter;
#endif
#ifdef INTERP_ALPHA
                  fa += fdaOuter;
#endif
#ifdef INTERP_INDEX
                  fi += fdiOuter;
#endif
#ifdef INTERP_INT_TEX
                  fs += fdsOuter;   ft += fdtOuter;
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
                  pRow = (PIXEL_TYPE *) ((GLubyte*)pRow + dPRowInner);
#endif
#ifdef INTERP_Z
#  ifdef DEPTH_TYPE
                  zRow = (DEPTH_TYPE *) ((GLubyte*)zRow + dZRowInner);
#  endif
                  fz += fdzInner;
                  ffog += fdfogInner;
#endif
#ifdef INTERP_RGB
                  fr += fdrInner;   fg += fdgInner;   fb += fdbInner;
#endif
#ifdef INTERP_SPEC
                  fsr += fdsrInner;   fsg += fdsgInner;   fsb += fdsbInner;
#endif
#ifdef INTERP_ALPHA
                  fa += fdaInner;
#endif
#ifdef INTERP_INDEX
                  fi += fdiInner;
#endif
#ifdef INTERP_INT_TEX
                  fs += fdsInner;   ft += fdtInner;
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
   }
}

#undef SETUP_CODE
#undef INNER_LOOP

#undef PIXEL_TYPE
#undef BYTES_PER_ROW
#undef PIXEL_ADDRESS

#undef INTERP_Z
#undef INTERP_RGB
#undef INTERP_SPEC
#undef INTERP_ALPHA
#undef INTERP_INDEX
#undef INTERP_LAMBDA
#undef COMPUTE_LAMBDA
#undef INTERP_INT_TEX
#undef INTERP_TEX
#undef INTERP_MULTITEX

#undef S_SCALE
#undef T_SCALE

#undef FixedToDepth

#undef DO_OCCLUSION_TEST

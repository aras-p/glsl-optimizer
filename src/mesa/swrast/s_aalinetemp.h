/* $Id: s_aalinetemp.h,v 1.12 2001/05/21 18:13:43 brianp Exp $ */

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
 * Antialiased line template.
 */


/*
 * Function to render each fragment in the AA line.
 */
static void
NAME(plot)(GLcontext *ctx, const struct LineInfo *line,
           struct pixel_buffer *pb, int ix, int iy)
{
   const GLfloat fx = (GLfloat) ix;
   const GLfloat fy = (GLfloat) iy;
   const GLfloat coverage = compute_coveragef(line, ix, iy);
   GLdepth z;
   GLfloat fog;
#ifdef DO_RGBA
   GLchan red, green, blue, alpha;
#else
   GLint index;
#endif
   GLchan specRed, specGreen, specBlue;
   GLfloat tex[MAX_TEXTURE_UNITS][4], lambda[MAX_TEXTURE_UNITS];

   if (coverage == 0.0)
      return;

   /*
    * Compute Z, color, texture coords, fog for the fragment by
    * solving the plane equations at (ix,iy).
    */
#ifdef DO_Z
   z = (GLdepth) solve_plane(fx, fy, line->zPlane);
#else
   z = 0.0;
#endif
#ifdef DO_FOG
   fog = solve_plane(fx, fy, line->fPlane);
#else
   fog = 0.0;
#endif
#ifdef DO_RGBA
   red   = solve_plane_chan(fx, fy, line->rPlane);
   green = solve_plane_chan(fx, fy, line->gPlane);
   blue  = solve_plane_chan(fx, fy, line->bPlane);
   alpha = solve_plane_chan(fx, fy, line->aPlane);
#endif
#ifdef DO_INDEX
   index = (GLint) solve_plane(fx, fy, line->iPlane);
#endif
#ifdef DO_SPEC
   specRed   = solve_plane_chan(fx, fy, line->srPlane);
   specGreen = solve_plane_chan(fx, fy, line->sgPlane);
   specBlue  = solve_plane_chan(fx, fy, line->sbPlane);
#else
   (void) specRed;
   (void) specGreen;
   (void) specBlue;
#endif
#ifdef DO_TEX
   {
      GLfloat invQ = solve_plane_recip(fx, fy, line->vPlane[0]);
      tex[0][0] = solve_plane(fx, fy, line->sPlane[0]) * invQ;
      tex[0][1] = solve_plane(fx, fy, line->tPlane[0]) * invQ;
      tex[0][2] = solve_plane(fx, fy, line->uPlane[0]) * invQ;
      lambda[0] = compute_lambda(line->sPlane[0], line->tPlane[0], invQ,
                                 line->texWidth[0], line->texHeight[0]);
   }
#elif defined(DO_MULTITEX)
   {
      GLuint unit;
      for (unit = 0; unit < ctx->Const.MaxTextureUnits; unit++) {
         if (ctx->Texture.Unit[unit]._ReallyEnabled) {
            GLfloat invQ = solve_plane_recip(fx, fy, line->vPlane[unit]);
            tex[unit][0] = solve_plane(fx, fy, line->sPlane[unit]) * invQ;
            tex[unit][1] = solve_plane(fx, fy, line->tPlane[unit]) * invQ;
            tex[unit][2] = solve_plane(fx, fy, line->uPlane[unit]) * invQ;
            lambda[unit] = compute_lambda(line->sPlane[unit],
                                          line->tPlane[unit], invQ,
                                          line->texWidth[unit], line->texHeight[unit]);
         }
      }
   }
#else
   (void) tex[0][0];
   (void) lambda[0];
#endif


   PB_COVERAGE(pb, coverage);

#if defined(DO_MULTITEX)
#if defined(DO_SPEC)
   PB_WRITE_MULTITEX_SPEC_PIXEL(pb, ix, iy, z, fog, red, green, blue, alpha,
                                specRed, specGreen, specBlue, tex);
#else
   PB_WRITE_MULTITEX_PIXEL(pb, ix, iy, z, fog, red, green, blue, alpha, tex);
#endif
#elif defined(DO_TEX)
   PB_WRITE_TEX_PIXEL(pb, ix, iy, z, fog, red, green, blue, alpha,
                      tex[0][0], tex[0][1], tex[0][2]);
#elif defined(DO_RGBA)
   PB_WRITE_RGBA_PIXEL(pb, ix, iy, z, fog, red, green, blue, alpha);
#elif defined(DO_INDEX)
   PB_WRITE_CI_PIXEL(pb, ix, iy, z, fog, index);
#endif

   pb->haveCoverage = GL_TRUE;
   PB_CHECK_FLUSH(ctx, pb);
}



/*
 * Line setup
 */
static void
NAME(line)(GLcontext *ctx, const SWvertex *v0, const SWvertex *v1)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   struct pixel_buffer *pb = SWRAST_CONTEXT(ctx)->PB;
   GLfloat tStart, tEnd;   /* segment start, end along line length */
   GLboolean inSegment;
   GLint iLen, i;

   /* Init the LineInfo struct */
   struct LineInfo line;
   line.x0 = v0->win[0];
   line.y0 = v0->win[1];
   line.x1 = v1->win[0];
   line.y1 = v1->win[1];
   line.dx = line.x1 - line.x0;
   line.dy = line.y1 - line.y0;
   line.len = sqrt(line.dx * line.dx + line.dy * line.dy);
   line.halfWidth = 0.5F * ctx->Line.Width;

   if (line.len == 0.0)
      return;

   line.xAdj = line.dx / line.len * line.halfWidth;
   line.yAdj = line.dy / line.len * line.halfWidth;

#ifdef DO_Z
   compute_plane(line.x0, line.y0, line.x1, line.y1,
                 v0->win[2], v1->win[2], line.zPlane);
#endif
#ifdef DO_FOG
   compute_plane(line.x0, line.y0, line.x1, line.y1,
                 v0->fog, v1->fog, line.fPlane);
#endif
#ifdef DO_RGBA
   if (ctx->Light.ShadeModel == GL_SMOOTH) {
      compute_plane(line.x0, line.y0, line.x1, line.y1,
                    v0->color[RCOMP], v1->color[RCOMP], line.rPlane);
      compute_plane(line.x0, line.y0, line.x1, line.y1,
                    v0->color[GCOMP], v1->color[GCOMP], line.gPlane);
      compute_plane(line.x0, line.y0, line.x1, line.y1,
                    v0->color[BCOMP], v1->color[BCOMP], line.bPlane);
      compute_plane(line.x0, line.y0, line.x1, line.y1,
                    v0->color[ACOMP], v1->color[ACOMP], line.aPlane);
   }
   else {
      constant_plane(v1->color[RCOMP], line.rPlane);
      constant_plane(v1->color[GCOMP], line.gPlane);
      constant_plane(v1->color[BCOMP], line.bPlane);
      constant_plane(v1->color[ACOMP], line.aPlane);
   }
#endif
#ifdef DO_SPEC
   if (ctx->Light.ShadeModel == GL_SMOOTH) {
      compute_plane(line.x0, line.y0, line.x1, line.y1,
                    v0->specular[RCOMP], v1->specular[RCOMP], line.srPlane);
      compute_plane(line.x0, line.y0, line.x1, line.y1,
                    v0->specular[GCOMP], v1->specular[GCOMP], line.sgPlane);
      compute_plane(line.x0, line.y0, line.x1, line.y1,
                    v0->specular[BCOMP], v1->specular[BCOMP], line.sbPlane);
   }
   else {
      constant_plane(v1->specular[RCOMP], line.srPlane);
      constant_plane(v1->specular[GCOMP], line.sgPlane);
      constant_plane(v1->specular[BCOMP], line.sbPlane);
   }
#endif
#ifdef DO_INDEX
   if (ctx->Light.ShadeModel == GL_SMOOTH) {
      compute_plane(line.x0, line.y0, line.x1, line.y1,
                    v0->index, v1->index, line.iPlane);
   }
   else {
      constant_plane(v1->index, line.iPlane);
   }
#endif
#ifdef DO_TEX
   {
      const struct gl_texture_object *obj = ctx->Texture.Unit[0]._Current;
      const struct gl_texture_image *texImage = obj->Image[obj->BaseLevel];
      const GLfloat invW0 = v0->win[3];
      const GLfloat invW1 = v1->win[3];
      const GLfloat s0 = v0->texcoord[0][0] * invW0;
      const GLfloat s1 = v1->texcoord[0][0] * invW1;
      const GLfloat t0 = v0->texcoord[0][1] * invW0;
      const GLfloat t1 = v1->texcoord[0][1] * invW0;
      const GLfloat r0 = v0->texcoord[0][2] * invW0;
      const GLfloat r1 = v1->texcoord[0][2] * invW0;
      const GLfloat q0 = v0->texcoord[0][3] * invW0;
      const GLfloat q1 = v1->texcoord[0][3] * invW0;
      compute_plane(line.x0, line.y0, line.x1, line.y1, s0, s1, line.sPlane[0]);
      compute_plane(line.x0, line.y0, line.x1, line.y1, t0, t1, line.tPlane[0]);
      compute_plane(line.x0, line.y0, line.x1, line.y1, r0, r1, line.uPlane[0]);
      compute_plane(line.x0, line.y0, line.x1, line.y1, q0, q1, line.vPlane[0]);
      line.texWidth[0] = (GLfloat) texImage->Width;
      line.texHeight[0] = (GLfloat) texImage->Height;
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
            const GLfloat s0 = v0->texcoord[u][0] * invW0;
            const GLfloat s1 = v1->texcoord[u][0] * invW1;
            const GLfloat t0 = v0->texcoord[u][1] * invW0;
            const GLfloat t1 = v1->texcoord[u][1] * invW0;
            const GLfloat r0 = v0->texcoord[u][2] * invW0;
            const GLfloat r1 = v1->texcoord[u][2] * invW0;
            const GLfloat q0 = v0->texcoord[u][3] * invW0;
            const GLfloat q1 = v1->texcoord[u][3] * invW0;
            compute_plane(line.x0, line.y0, line.x1, line.y1, s0, s1, line.sPlane[u]);
            compute_plane(line.x0, line.y0, line.x1, line.y1, t0, t1, line.tPlane[u]);
            compute_plane(line.x0, line.y0, line.x1, line.y1, r0, r1, line.uPlane[u]);
            compute_plane(line.x0, line.y0, line.x1, line.y1, q0, q1, line.vPlane[u]);
            line.texWidth[u]  = (GLfloat) texImage->Width;
            line.texHeight[u] = (GLfloat) texImage->Height;
         }
      }
   }
#endif

   tStart = tEnd = 0.0;
   inSegment = GL_FALSE;
   iLen = (GLint) line.len;

   if (ctx->Line.StippleFlag) {
      for (i = 0; i < iLen; i++) {
         const GLuint bit = (swrast->StippleCounter / ctx->Line.StippleFactor) & 0xf;
         if ((1 << bit) & ctx->Line.StipplePattern) {
            /* stipple bit is on */
            const GLfloat t = (GLfloat) i / (GLfloat) line.len;
            if (!inSegment) {
               /* start new segment */
               inSegment = GL_TRUE;
               tStart = t;
            }
            else {
               /* still in the segment, extend it */
               tEnd = t;
            }
         }
         else {
            /* stipple bit is off */
            if (inSegment && (tEnd > tStart)) {
               /* draw the segment */
               segment(ctx, &line, NAME(plot), pb, tStart, tEnd);
               inSegment = GL_FALSE;
            }
            else {
               /* still between segments, do nothing */
            }
         }
         swrast->StippleCounter++;
      }

      if (inSegment) {
         /* draw the final segment of the line */
         segment(ctx, &line, NAME(plot), pb, tStart, 1.0F);
      }
   }
   else {
      /* non-stippled */
      segment(ctx, &line, NAME(plot), pb, 0.0, 1.0);
   }
}




#undef DO_Z
#undef DO_FOG
#undef DO_RGBA
#undef DO_INDEX
#undef DO_SPEC
#undef DO_TEX
#undef DO_MULTITEX
#undef NAME

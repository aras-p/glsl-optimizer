/* $Id: s_linetemp.h,v 1.8 2001/05/03 22:13:32 brianp Exp $ */

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
 * Line Rasterizer Template
 *
 * This file is #include'd to generate custom line rasterizers.
 *
 * The following macros may be defined to indicate what auxillary information
 * must be interplated along the line:
 *    INTERP_Z        - if defined, interpolate Z values
 *    INTERP_FOG      - if defined, interpolate FOG values
 *    INTERP_RGB      - if defined, interpolate RGB values
 *    INTERP_SPEC     - if defined, interpolate specular RGB values
 *    INTERP_ALPHA    - if defined, interpolate Alpha values
 *    INTERP_INDEX    - if defined, interpolate color index values
 *    INTERP_TEX      - if defined, interpolate unit 0 texcoords
 *    INTERP_MULTITEX - if defined, interpolate multi-texcoords
 *
 * When one can directly address pixels in the color buffer the following
 * macros can be defined and used to directly compute pixel addresses during
 * rasterization (see pixelPtr):
 *    PIXEL_TYPE          - the datatype of a pixel (GLubyte, GLushort, GLuint)
 *    BYTES_PER_ROW       - number of bytes per row in the color buffer
 *    PIXEL_ADDRESS(X,Y)  - returns the address of pixel at (X,Y) where
 *                          Y==0 at bottom of screen and increases upward.
 *
 * Similarly, for direct depth buffer access, this type is used for depth
 * buffer addressing:
 *    DEPTH_TYPE          - either GLushort or GLuint
 *
 * Optionally, one may provide one-time setup code
 *    SETUP_CODE    - code which is to be executed once per line
 *
 * To enable line stippling define STIPPLE = 1
 * To enable wide lines define WIDE = 1
 *
 * To actually "plot" each pixel either the PLOT macro or
 * (XMAJOR_PLOT and YMAJOR_PLOT macros) must be defined...
 *    PLOT(X,Y) - code to plot a pixel.  Example:
 *                if (Z < *zPtr) {
 *                   *zPtr = Z;
 *                   color = pack_rgb( FixedToInt(r0), FixedToInt(g0),
 *                                     FixedToInt(b0) );
 *                   put_pixel( X, Y, color );
 *                }
 *
 * This code was designed for the origin to be in the lower-left corner.
 *
 */


/*void line( GLcontext *ctx, const SWvertex *vert0, const SWvertex *vert1 )*/
{
   GLint x0 = (GLint) vert0->win[0];
   GLint x1 = (GLint) vert1->win[0];
   GLint y0 = (GLint) vert0->win[1];
   GLint y1 = (GLint) vert1->win[1];
   GLint dx, dy;
#ifdef INTERP_XY
   GLint xstep, ystep;
#endif
#ifdef INTERP_Z
   GLint z0, z1, dz;
   const GLint depthBits = ctx->Visual.depthBits;
   const GLint fixedToDepthShift = depthBits <= 16 ? FIXED_SHIFT : 0;
#  define FixedToDepth(F)  ((F) >> fixedToDepthShift)
#  ifdef DEPTH_TYPE
   GLint zPtrXstep, zPtrYstep;
   DEPTH_TYPE *zPtr;
#  endif
#endif
#ifdef INTERP_FOG
   GLfloat fog0 = vert0->fog;
   GLfloat dfog = vert1->fog - fog0;
#endif
#ifdef INTERP_RGB
   GLfixed r0 = IntToFixed(vert0->color[0]);
   GLfixed dr = IntToFixed(vert1->color[0]) - r0;
   GLfixed g0 = IntToFixed(vert0->color[1]);
   GLfixed dg = IntToFixed(vert1->color[1]) - g0;
   GLfixed b0 = IntToFixed(vert0->color[2]);
   GLfixed db = IntToFixed(vert1->color[2]) - b0;
#endif
#ifdef INTERP_SPEC
   GLfixed sr0 = IntToFixed(vert0->specular[0]);
   GLfixed dsr = IntToFixed(vert1->specular[0]) - sr0;
   GLfixed sg0 = IntToFixed(vert0->specular[1]);
   GLfixed dsg = IntToFixed(vert1->specular[1]) - sg0;
   GLfixed sb0 = IntToFixed(vert0->specular[2]);
   GLfixed dsb = IntToFixed(vert1->specular[2]) - sb0;
#endif
#ifdef INTERP_ALPHA
   GLfixed a0 = IntToFixed(vert0->color[3]);
   GLfixed da = IntToFixed(vert1->color[3]) - a0;
#endif
#ifdef INTERP_INDEX
   GLint i0 = vert0->index << 8;
   GLint di = (GLint) (vert1->index << 8) - i0;
#endif
#ifdef INTERP_TEX
   const GLfloat invw0 = vert0->win[3];
   const GLfloat invw1 = vert1->win[3];
   GLfloat tex[4];
   GLfloat dtex[4];
   GLfloat fragTexcoord[4];
#endif
#ifdef INTERP_MULTITEX
   const GLfloat invw0 = vert0->win[3];
   const GLfloat invw1 = vert1->win[3];
   GLfloat tex[MAX_TEXTURE_UNITS][4];
   GLfloat dtex[MAX_TEXTURE_UNITS][4];
   GLfloat fragTexcoord[MAX_TEXTURE_UNITS][4];
#endif
#ifdef PIXEL_ADDRESS
   PIXEL_TYPE *pixelPtr;
   GLint pixelXstep, pixelYstep;
#endif
#ifdef STIPPLE
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
#endif
#ifdef WIDE
   /* for wide lines, draw all X in [x+min, x+max] or Y in [y+min, y+max] */
   GLint width, min, max;
   width = (GLint) CLAMP( ctx->Line.Width, MIN_LINE_WIDTH, MAX_LINE_WIDTH );
   min = (width-1) / -2;
   max = min + width - 1;
#endif
#ifdef INTERP_TEX
   {
      tex[0]  = invw0 * vert0->texcoord[0][0];
      dtex[0] = invw1 * vert1->texcoord[0][0] - tex[0];
      tex[1]  = invw0 * vert0->texcoord[0][1];
      dtex[1] = invw1 * vert1->texcoord[0][1] - tex[1];
      tex[2]  = invw0 * vert0->texcoord[0][2];
      dtex[2] = invw1 * vert1->texcoord[0][2] - tex[2];
      tex[3]  = invw0 * vert0->texcoord[0][3];
      dtex[3] = invw1 * vert1->texcoord[0][3] - tex[3];
   }
#endif
#ifdef INTERP_MULTITEX
   {
      GLuint u;
      for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {
         if (ctx->Texture.Unit[u]._ReallyEnabled) {
            tex[u][0]  = invw0 * vert0->texcoord[u][0];
            dtex[u][0] = invw1 * vert1->texcoord[u][0] - tex[u][0];
	    tex[u][1]  = invw0 * vert0->texcoord[u][1];
	    dtex[u][1] = invw1 * vert1->texcoord[u][1] - tex[u][1];
	    tex[u][2]  = invw0 * vert0->texcoord[u][2];
	    dtex[u][2] = invw1 * vert1->texcoord[u][2] - tex[u][2];
	    tex[u][3]  = invw0 * vert0->texcoord[u][3];
	    dtex[u][3] = invw1 * vert1->texcoord[u][3] - tex[u][3];
	 }
      }
   }
#endif


/*
 * Despite being clipped to the view volume, the line's window coordinates
 * may just lie outside the window bounds.  That is, if the legal window
 * coordinates are [0,W-1][0,H-1], it's possible for x==W and/or y==H.
 * This quick and dirty code nudges the endpoints inside the window if
 * necessary.
 */
#ifdef CLIP_HACK
   {
      GLint w = ctx->DrawBuffer->Width;
      GLint h = ctx->DrawBuffer->Height;
      if ((x0==w) | (x1==w)) {
         if ((x0==w) & (x1==w))
           return;
         x0 -= x0==w;
         x1 -= x1==w;
      }
      if ((y0==h) | (y1==h)) {
         if ((y0==h) & (y1==h))
           return;
         y0 -= y0==h;
         y1 -= y1==h;
      }
   }
#endif
   dx = x1 - x0;
   dy = y1 - y0;
   if (dx==0 && dy==0) {
      return;
   }

   /*
    * Setup
    */
#ifdef SETUP_CODE
   SETUP_CODE
#endif

#ifdef INTERP_Z
#  ifdef DEPTH_TYPE
     zPtr = (DEPTH_TYPE *) _mesa_zbuffer_address(ctx, x0, y0);
#  endif
   if (depthBits <= 16) {
      z0 = FloatToFixed(vert0->win[2]);
      z1 = FloatToFixed(vert1->win[2]);
   }
   else {
      z0 = (int) vert0->win[2];
      z1 = (int) vert1->win[2];
   }
#endif
#ifdef PIXEL_ADDRESS
   pixelPtr = (PIXEL_TYPE *) PIXEL_ADDRESS(x0,y0);
#endif

   if (dx<0) {
      dx = -dx;   /* make positive */
#ifdef INTERP_XY
      xstep = -1;
#endif
#if defined(INTERP_Z) && defined(DEPTH_TYPE)
      zPtrXstep = -((GLint)sizeof(DEPTH_TYPE));
#endif
#ifdef PIXEL_ADDRESS
      pixelXstep = -((GLint)sizeof(PIXEL_TYPE));
#endif
   }
   else {
#ifdef INTERP_XY
      xstep = 1;
#endif
#if defined(INTERP_Z) && defined(DEPTH_TYPE)
      zPtrXstep = ((GLint)sizeof(DEPTH_TYPE));
#endif
#ifdef PIXEL_ADDRESS
      pixelXstep = ((GLint)sizeof(PIXEL_TYPE));
#endif
   }

   if (dy<0) {
      dy = -dy;   /* make positive */
#ifdef INTERP_XY
      ystep = -1;
#endif
#if defined(INTERP_Z) && defined(DEPTH_TYPE)
      zPtrYstep = -ctx->DrawBuffer->Width * ((GLint)sizeof(DEPTH_TYPE));
#endif
#ifdef PIXEL_ADDRESS
      pixelYstep = BYTES_PER_ROW;
#endif
   }
   else {
#ifdef INTERP_XY
      ystep = 1;
#endif
#if defined(INTERP_Z) && defined(DEPTH_TYPE)
      zPtrYstep = ctx->DrawBuffer->Width * ((GLint)sizeof(DEPTH_TYPE));
#endif
#ifdef PIXEL_ADDRESS
      pixelYstep = -(BYTES_PER_ROW);
#endif
   }

   /*
    * Draw
    */

   if (dx>dy) {
      /*** X-major line ***/
      GLint i;
      GLint errorInc = dy+dy;
      GLint error = errorInc-dx;
      GLint errorDec = error-dx;
#ifdef INTERP_Z
      dz = (z1-z0) / dx;
#endif
#ifdef INTERP_FOG
      dfog /= dx;
#endif
#ifdef INTERP_RGB
      dr /= dx;   /* convert from whole line delta to per-pixel delta */
      dg /= dx;
      db /= dx;
#endif
#ifdef INTERP_SPEC
      dsr /= dx;   /* convert from whole line delta to per-pixel delta */
      dsg /= dx;
      dsb /= dx;
#endif
#ifdef INTERP_ALPHA
      da /= dx;
#endif
#ifdef INTERP_INDEX
      di /= dx;
#endif
#ifdef INTERP_TEX
      {
         const GLfloat invDx = 1.0F / (GLfloat) dx;
         dtex[0] *= invDx;
         dtex[1] *= invDx;
         dtex[2] *= invDx;
         dtex[3] *= invDx;
      }
#endif
#ifdef INTERP_MULTITEX
      {
         const GLfloat invDx = 1.0F / (GLfloat) dx;
         GLuint u;
         for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {
            if (ctx->Texture.Unit[u]._ReallyEnabled) {
               dtex[u][0] *= invDx;
               dtex[u][1] *= invDx;
               dtex[u][2] *= invDx;
               dtex[u][3] *= invDx;
            }
         }
      }
#endif

      for (i=0;i<dx;i++) {
#ifdef STIPPLE
         GLushort m;
         m = 1 << ((swrast->StippleCounter/ctx->Line.StippleFactor) & 0xf);
         if (ctx->Line.StipplePattern & m) {
#endif
#ifdef INTERP_Z
            GLdepth Z = FixedToDepth(z0);
#endif
#ifdef INTERP_INDEX
            GLint I = i0 >> 8;
#endif
#ifdef INTERP_TEX
            {
               const GLfloat invQ = tex[3] ? (1.0F / tex[3]) : 1.0F;
               fragTexcoord[0] = tex[0] * invQ;
               fragTexcoord[1] = tex[1] * invQ;
               fragTexcoord[2] = tex[2] * invQ;
               fragTexcoord[3] = tex[3];
            }
#endif
#ifdef INTERP_MULTITEX
            {
               GLuint u;
               for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {
                  if (ctx->Texture.Unit[u]._ReallyEnabled) {
                     const GLfloat invQ = 1.0F / tex[u][3];
                     fragTexcoord[u][0] = tex[u][0] * invQ;
                     fragTexcoord[u][1] = tex[u][1] * invQ;
                     fragTexcoord[u][2] = tex[u][2] * invQ;
                     fragTexcoord[u][3] = tex[u][3];
                  }
               }
            }
#endif
#ifdef WIDE
            {
               GLint yy;
               GLint ymin = y0 + min;
               GLint ymax = y0 + max;
               for (yy=ymin;yy<=ymax;yy++) {
                  PLOT( x0, yy );
               }
            }
#else
#  ifdef XMAJOR_PLOT
            XMAJOR_PLOT( x0, y0 );
#  else
            PLOT( x0, y0 );
#  endif
#endif /*WIDE*/
#ifdef STIPPLE
        }
	swrast->StippleCounter++;
#endif
#ifdef INTERP_XY
         x0 += xstep;
#endif
#ifdef INTERP_Z
#  ifdef DEPTH_TYPE
         zPtr = (DEPTH_TYPE *) ((GLubyte*) zPtr + zPtrXstep);
#  endif
         z0 += dz;
#endif
#ifdef INTERP_FOG
	 fog0 += dfog;
#endif
#ifdef INTERP_RGB
         r0 += dr;
         g0 += dg;
         b0 += db;
#endif
#ifdef INTERP_SPEC
         sr0 += dsr;
         sg0 += dsg;
         sb0 += dsb;
#endif
#ifdef INTERP_ALPHA
         a0 += da;
#endif
#ifdef INTERP_INDEX
         i0 += di;
#endif
#ifdef INTERP_TEX
         tex[0] += dtex[0];
         tex[1] += dtex[1];
         tex[2] += dtex[2];
         tex[3] += dtex[3];
#endif
#ifdef INTERP_MULTITEX
         {
            GLuint u;
            for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {
               if (ctx->Texture.Unit[u]._ReallyEnabled) {
                  tex[u][0] += dtex[u][0];
                  tex[u][1] += dtex[u][1];
                  tex[u][2] += dtex[u][2];
                  tex[u][3] += dtex[u][3];
               }
            }
         }
#endif

#ifdef PIXEL_ADDRESS
         pixelPtr = (PIXEL_TYPE*) ((GLubyte*) pixelPtr + pixelXstep);
#endif
         if (error<0) {
            error += errorInc;
         }
         else {
            error += errorDec;
#ifdef INTERP_XY
            y0 += ystep;
#endif
#if defined(INTERP_Z) && defined(DEPTH_TYPE)
            zPtr = (DEPTH_TYPE *) ((GLubyte*) zPtr + zPtrYstep);
#endif
#ifdef PIXEL_ADDRESS
            pixelPtr = (PIXEL_TYPE*) ((GLubyte*) pixelPtr + pixelYstep);
#endif
         }
      }
   }
   else {
      /*** Y-major line ***/
      GLint i;
      GLint errorInc = dx+dx;
      GLint error = errorInc-dy;
      GLint errorDec = error-dy;
#ifdef INTERP_Z
      dz = (z1-z0) / dy;
#endif
#ifdef INTERP_FOG
      dfog /= dy;
#endif
#ifdef INTERP_RGB
      dr /= dy;   /* convert from whole line delta to per-pixel delta */
      dg /= dy;
      db /= dy;
#endif
#ifdef INTERP_SPEC
      dsr /= dy;   /* convert from whole line delta to per-pixel delta */
      dsg /= dy;
      dsb /= dy;
#endif
#ifdef INTERP_ALPHA
      da /= dy;
#endif
#ifdef INTERP_INDEX
      di /= dy;
#endif
#ifdef INTERP_TEX
      {
         const GLfloat invDy = 1.0F / (GLfloat) dy;
         dtex[0] *= invDy;
         dtex[1] *= invDy;
         dtex[2] *= invDy;
         dtex[3] *= invDy;
      }
#endif
#ifdef INTERP_MULTITEX
      {
         const GLfloat invDy = 1.0F / (GLfloat) dy;
         GLuint u;
         for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {
            if (ctx->Texture.Unit[u]._ReallyEnabled) {
               dtex[u][0] *= invDy;
               dtex[u][1] *= invDy;
               dtex[u][2] *= invDy;
               dtex[u][3] *= invDy;
            }
         }
      }
#endif

      for (i=0;i<dy;i++) {
#ifdef STIPPLE
         GLushort m;
         m = 1 << ((swrast->StippleCounter/ctx->Line.StippleFactor) & 0xf);
         if (ctx->Line.StipplePattern & m) {
#endif
#ifdef INTERP_Z
            GLdepth Z = FixedToDepth(z0);
#endif
#ifdef INTERP_INDEX
            GLint I = i0 >> 8;
#endif
#ifdef INTERP_TEX
            {
               const GLfloat invQ = tex[3] ? (1.0F / tex[3]) : 1.0F;
               fragTexcoord[0] = tex[0] * invQ;
               fragTexcoord[1] = tex[1] * invQ;
               fragTexcoord[2] = tex[2] * invQ;
               fragTexcoord[3] = tex[3];
            }
#endif
#ifdef INTERP_MULTITEX
            {
               GLuint u;
               for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {
                  if (ctx->Texture.Unit[u]._ReallyEnabled) {
                     const GLfloat invQ = 1.0F / tex[u][3];
                     fragTexcoord[u][0] = tex[u][0] * invQ;
                     fragTexcoord[u][1] = tex[u][1] * invQ;
                     fragTexcoord[u][2] = tex[u][2] * invQ;
                     fragTexcoord[u][3] = tex[u][3];
                  }
               }
            }
#endif
#ifdef WIDE
            {
               GLint xx;
               GLint xmin = x0 + min;
               GLint xmax = x0 + max;
               for (xx=xmin;xx<=xmax;xx++) {
                  PLOT( xx, y0 );
               }
            }
#else
#  ifdef YMAJOR_PLOT
            YMAJOR_PLOT( x0, y0 );
#  else
            PLOT( x0, y0 );
#  endif
#endif /*WIDE*/
#ifdef STIPPLE
        }
	swrast->StippleCounter++;
#endif
#ifdef INTERP_XY
         y0 += ystep;
#endif
#ifdef INTERP_Z
#  ifdef DEPTH_TYPE
         zPtr = (DEPTH_TYPE *) ((GLubyte*) zPtr + zPtrYstep);
#  endif
         z0 += dz;
#endif
#ifdef INTERP_FOG
	 fog0 += dfog;
#endif
#ifdef INTERP_RGB
         r0 += dr;
         g0 += dg;
         b0 += db;
#endif
#ifdef INTERP_SPEC
         sr0 += dsr;
         sg0 += dsg;
         sb0 += dsb;
#endif
#ifdef INTERP_ALPHA
         a0 += da;
#endif
#ifdef INTERP_INDEX
         i0 += di;
#endif
#ifdef INTERP_TEX
         tex[0] += dtex[0];
         tex[1] += dtex[1];
         tex[2] += dtex[2];
         tex[3] += dtex[3];
#endif
#ifdef INTERP_MULTITEX
         {
            GLuint u;
            for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {
               if (ctx->Texture.Unit[u]._ReallyEnabled) {
                  tex[u][0] += dtex[u][0];
                  tex[u][1] += dtex[u][1];
                  tex[u][2] += dtex[u][2];
                  tex[u][3] += dtex[u][3];
               }
            }
         }
#endif
#ifdef PIXEL_ADDRESS
         pixelPtr = (PIXEL_TYPE*) ((GLubyte*) pixelPtr + pixelYstep);
#endif
         if (error<0) {
            error += errorInc;
         }
         else {
            error += errorDec;
#ifdef INTERP_XY
            x0 += xstep;
#endif
#if defined(INTERP_Z) && defined(DEPTH_TYPE)
            zPtr = (DEPTH_TYPE *) ((GLubyte*) zPtr + zPtrXstep);
#endif
#ifdef PIXEL_ADDRESS
            pixelPtr = (PIXEL_TYPE*) ((GLubyte*) pixelPtr + pixelXstep);
#endif
         }
      }
   }

}


#undef INTERP_XY
#undef INTERP_Z
#undef INTERP_FOG
#undef INTERP_RGB
#undef INTERP_SPEC
#undef INTERP_ALPHA
#undef INTERP_TEX
#undef INTERP_MULTITEX
#undef INTERP_INDEX
#undef PIXEL_ADDRESS
#undef PIXEL_TYPE
#undef DEPTH_TYPE
#undef BYTES_PER_ROW
#undef SETUP_CODE
#undef PLOT
#undef XMAJOR_PLOT
#undef YMAJOR_PLOT
#undef CLIP_HACK
#undef STIPPLE
#undef WIDE
#undef FixedToDepth

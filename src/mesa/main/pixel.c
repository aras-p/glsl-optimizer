/* $Id: pixel.c,v 1.4 1999/11/08 07:36:44 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.1
 * 
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
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
 * glPixelStore, glPixelTransfer, glPixelMap, glPixelZoom, etc.
 */


#ifdef PC_HEADER
#include "all.h"
#else
#ifndef XFree86Server
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#else
#include "GL/xf86glx.h"
#endif
#include "context.h"
#include "macros.h"
#include "pixel.h"
#include "types.h"
#endif



/**********************************************************************/
/*****                    glPixelZoom                             *****/
/**********************************************************************/



void gl_PixelZoom( GLcontext *ctx, GLfloat xfactor, GLfloat yfactor )
{
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glPixelZoom");

   ctx->Pixel.ZoomX = xfactor;
   ctx->Pixel.ZoomY = yfactor;
}



/**********************************************************************/
/*****                    glPixelStore                            *****/
/**********************************************************************/


void gl_PixelStorei( GLcontext *ctx, GLenum pname, GLint param )
{
   /* NOTE: this call can't be compiled into the display list */

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glPixelStore");


   switch (pname) {
      case GL_PACK_SWAP_BYTES:
         ctx->Pack.SwapBytes = param ? GL_TRUE : GL_FALSE;
	 break;
      case GL_PACK_LSB_FIRST:
         ctx->Pack.LsbFirst = param ? GL_TRUE : GL_FALSE;
	 break;
      case GL_PACK_ROW_LENGTH:
	 if (param<0) {
	    gl_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	 }
	 else {
	    ctx->Pack.RowLength = param;
	 }
	 break;
      case GL_PACK_IMAGE_HEIGHT:
         if (param<0)
            gl_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
         else
            ctx->Pack.ImageHeight = param;
         break;
      case GL_PACK_SKIP_PIXELS:
	 if (param<0) {
	    gl_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	 }
	 else {
	    ctx->Pack.SkipPixels = param;
	 }
	 break;
      case GL_PACK_SKIP_ROWS:
	 if (param<0) {
	    gl_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	 }
	 else {
	    ctx->Pack.SkipRows = param;
	 }
	 break;
      case GL_PACK_ALIGNMENT:
         if (param==1 || param==2 || param==4 || param==8) {
	    ctx->Pack.Alignment = param;
	 }
	 else {
	    gl_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	 }
	 break;
      case GL_UNPACK_SWAP_BYTES:
	 ctx->Unpack.SwapBytes = param ? GL_TRUE : GL_FALSE;
         break;
      case GL_UNPACK_LSB_FIRST:
	 ctx->Unpack.LsbFirst = param ? GL_TRUE : GL_FALSE;
	 break;
      case GL_UNPACK_ROW_LENGTH:
	 if (param<0) {
	    gl_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	 }
	 else {
	    ctx->Unpack.RowLength = param;
	 }
	 break;
      case GL_UNPACK_IMAGE_HEIGHT:
         if (param<0)
            gl_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
         else
            ctx->Unpack.ImageHeight = param;
         break;
      case GL_UNPACK_SKIP_PIXELS:
	 if (param<0) {
	    gl_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	 }
	 else {
	    ctx->Unpack.SkipPixels = param;
	 }
	 break;
      case GL_UNPACK_SKIP_ROWS:
	 if (param<0) {
	    gl_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	 }
	 else {
	    ctx->Unpack.SkipRows = param;
	 }
	 break;
      case GL_UNPACK_ALIGNMENT:
         if (param==1 || param==2 || param==4 || param==8) {
	    ctx->Unpack.Alignment = param;
	 }
	 else {
	    gl_error( ctx, GL_INVALID_VALUE, "glPixelStore" );
	 }
	 break;
      default:
	 gl_error( ctx, GL_INVALID_ENUM, "glPixelStore" );
   }
}





/**********************************************************************/
/*****                         glPixelMap                         *****/
/**********************************************************************/



void gl_PixelMapfv( GLcontext *ctx,
                    GLenum map, GLint mapsize, const GLfloat *values )
{
   GLint i;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glPixelMapfv");


   if (mapsize<0 || mapsize>MAX_PIXEL_MAP_TABLE) {
      gl_error( ctx, GL_INVALID_VALUE, "glPixelMapfv(mapsize)" );
      return;
   }

   if (map>=GL_PIXEL_MAP_S_TO_S && map<=GL_PIXEL_MAP_I_TO_A) {
      /* test that mapsize is a power of two */
      GLuint p;
      GLboolean ok = GL_FALSE;
      for (p=1; p<=MAX_PIXEL_MAP_TABLE; p=p<<1) {
	 if ( (p&mapsize) == p ) {
	    ok = GL_TRUE;
	    break;
	 }
      }
      if (!ok) {
	 gl_error( ctx, GL_INVALID_VALUE, "glPixelMapfv(mapsize)" );
         return;
      }
   }

   switch (map) {
      case GL_PIXEL_MAP_S_TO_S:
         ctx->Pixel.MapStoSsize = mapsize;
         for (i=0;i<mapsize;i++) {
	    ctx->Pixel.MapStoS[i] = (GLint) values[i];
	 }
	 break;
      case GL_PIXEL_MAP_I_TO_I:
         ctx->Pixel.MapItoIsize = mapsize;
         for (i=0;i<mapsize;i++) {
	    ctx->Pixel.MapItoI[i] = (GLint) values[i];
	 }
	 break;
      case GL_PIXEL_MAP_I_TO_R:
         ctx->Pixel.MapItoRsize = mapsize;
         for (i=0;i<mapsize;i++) {
            GLfloat val = CLAMP( values[i], 0.0, 1.0 );
	    ctx->Pixel.MapItoR[i] = val;
	    ctx->Pixel.MapItoR8[i] = (GLint) (val * 255.0F);
	 }
	 break;
      case GL_PIXEL_MAP_I_TO_G:
         ctx->Pixel.MapItoGsize = mapsize;
         for (i=0;i<mapsize;i++) {
            GLfloat val = CLAMP( values[i], 0.0, 1.0 );
	    ctx->Pixel.MapItoG[i] = val;
	    ctx->Pixel.MapItoG8[i] = (GLint) (val * 255.0F);
	 }
	 break;
      case GL_PIXEL_MAP_I_TO_B:
         ctx->Pixel.MapItoBsize = mapsize;
         for (i=0;i<mapsize;i++) {
            GLfloat val = CLAMP( values[i], 0.0, 1.0 );
	    ctx->Pixel.MapItoB[i] = val;
	    ctx->Pixel.MapItoB8[i] = (GLint) (val * 255.0F);
	 }
	 break;
      case GL_PIXEL_MAP_I_TO_A:
         ctx->Pixel.MapItoAsize = mapsize;
         for (i=0;i<mapsize;i++) {
            GLfloat val = CLAMP( values[i], 0.0, 1.0 );
	    ctx->Pixel.MapItoA[i] = val;
	    ctx->Pixel.MapItoA8[i] = (GLint) (val * 255.0F);
	 }
	 break;
      case GL_PIXEL_MAP_R_TO_R:
         ctx->Pixel.MapRtoRsize = mapsize;
         for (i=0;i<mapsize;i++) {
	    ctx->Pixel.MapRtoR[i] = CLAMP( values[i], 0.0, 1.0 );
	 }
	 break;
      case GL_PIXEL_MAP_G_TO_G:
         ctx->Pixel.MapGtoGsize = mapsize;
         for (i=0;i<mapsize;i++) {
	    ctx->Pixel.MapGtoG[i] = CLAMP( values[i], 0.0, 1.0 );
	 }
	 break;
      case GL_PIXEL_MAP_B_TO_B:
         ctx->Pixel.MapBtoBsize = mapsize;
         for (i=0;i<mapsize;i++) {
	    ctx->Pixel.MapBtoB[i] = CLAMP( values[i], 0.0, 1.0 );
	 }
	 break;
      case GL_PIXEL_MAP_A_TO_A:
         ctx->Pixel.MapAtoAsize = mapsize;
         for (i=0;i<mapsize;i++) {
	    ctx->Pixel.MapAtoA[i] = CLAMP( values[i], 0.0, 1.0 );
	 }
	 break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glPixelMapfv(map)" );
   }
}





void gl_GetPixelMapfv( GLcontext *ctx, GLenum map, GLfloat *values )
{
   GLint i;

   ASSERT_OUTSIDE_BEGIN_END(ctx, "glGetPixelMapfv");

   switch (map) {
      case GL_PIXEL_MAP_I_TO_I:
         for (i=0;i<ctx->Pixel.MapItoIsize;i++) {
	    values[i] = (GLfloat) ctx->Pixel.MapItoI[i];
	 }
	 break;
      case GL_PIXEL_MAP_S_TO_S:
         for (i=0;i<ctx->Pixel.MapStoSsize;i++) {
	    values[i] = (GLfloat) ctx->Pixel.MapStoS[i];
	 }
	 break;
      case GL_PIXEL_MAP_I_TO_R:
         MEMCPY(values,ctx->Pixel.MapItoR,ctx->Pixel.MapItoRsize*sizeof(GLfloat));
	 break;
      case GL_PIXEL_MAP_I_TO_G:
         MEMCPY(values,ctx->Pixel.MapItoG,ctx->Pixel.MapItoGsize*sizeof(GLfloat));
	 break;
      case GL_PIXEL_MAP_I_TO_B:
         MEMCPY(values,ctx->Pixel.MapItoB,ctx->Pixel.MapItoBsize*sizeof(GLfloat));
	 break;
      case GL_PIXEL_MAP_I_TO_A:
         MEMCPY(values,ctx->Pixel.MapItoA,ctx->Pixel.MapItoAsize*sizeof(GLfloat));
	 break;
      case GL_PIXEL_MAP_R_TO_R:
         MEMCPY(values,ctx->Pixel.MapRtoR,ctx->Pixel.MapRtoRsize*sizeof(GLfloat));
	 break;
      case GL_PIXEL_MAP_G_TO_G:
         MEMCPY(values,ctx->Pixel.MapGtoG,ctx->Pixel.MapGtoGsize*sizeof(GLfloat));
	 break;
      case GL_PIXEL_MAP_B_TO_B:
         MEMCPY(values,ctx->Pixel.MapBtoB,ctx->Pixel.MapBtoBsize*sizeof(GLfloat));
	 break;
      case GL_PIXEL_MAP_A_TO_A:
         MEMCPY(values,ctx->Pixel.MapAtoA,ctx->Pixel.MapAtoAsize*sizeof(GLfloat));
	 break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetPixelMapfv" );
   }
}


void gl_GetPixelMapuiv( GLcontext *ctx, GLenum map, GLuint *values )
{
   GLint i;

   ASSERT_OUTSIDE_BEGIN_END(ctx, "glGetPixelMapfv");

   switch (map) {
      case GL_PIXEL_MAP_I_TO_I:
         MEMCPY(values, ctx->Pixel.MapItoI, ctx->Pixel.MapItoIsize*sizeof(GLint));
	 break;
      case GL_PIXEL_MAP_S_TO_S:
         MEMCPY(values, ctx->Pixel.MapStoS, ctx->Pixel.MapStoSsize*sizeof(GLint));
	 break;
      case GL_PIXEL_MAP_I_TO_R:
	 for (i=0;i<ctx->Pixel.MapItoRsize;i++) {
	    values[i] = FLOAT_TO_UINT( ctx->Pixel.MapItoR[i] );
	 }
	 break;
      case GL_PIXEL_MAP_I_TO_G:
	 for (i=0;i<ctx->Pixel.MapItoGsize;i++) {
	    values[i] = FLOAT_TO_UINT( ctx->Pixel.MapItoG[i] );
	 }
	 break;
      case GL_PIXEL_MAP_I_TO_B:
	 for (i=0;i<ctx->Pixel.MapItoBsize;i++) {
	    values[i] = FLOAT_TO_UINT( ctx->Pixel.MapItoB[i] );
	 }
	 break;
      case GL_PIXEL_MAP_I_TO_A:
	 for (i=0;i<ctx->Pixel.MapItoAsize;i++) {
	    values[i] = FLOAT_TO_UINT( ctx->Pixel.MapItoA[i] );
	 }
	 break;
      case GL_PIXEL_MAP_R_TO_R:
	 for (i=0;i<ctx->Pixel.MapRtoRsize;i++) {
	    values[i] = FLOAT_TO_UINT( ctx->Pixel.MapRtoR[i] );
	 }
	 break;
      case GL_PIXEL_MAP_G_TO_G:
	 for (i=0;i<ctx->Pixel.MapGtoGsize;i++) {
	    values[i] = FLOAT_TO_UINT( ctx->Pixel.MapGtoG[i] );
	 }
	 break;
      case GL_PIXEL_MAP_B_TO_B:
	 for (i=0;i<ctx->Pixel.MapBtoBsize;i++) {
	    values[i] = FLOAT_TO_UINT( ctx->Pixel.MapBtoB[i] );
	 }
	 break;
      case GL_PIXEL_MAP_A_TO_A:
	 for (i=0;i<ctx->Pixel.MapAtoAsize;i++) {
	    values[i] = FLOAT_TO_UINT( ctx->Pixel.MapAtoA[i] );
	 }
	 break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetPixelMapfv" );
   }
}


void gl_GetPixelMapusv( GLcontext *ctx, GLenum map, GLushort *values )
{
   GLint i;

   ASSERT_OUTSIDE_BEGIN_END(ctx, "glGetPixelMapfv");

   switch (map) {
      case GL_PIXEL_MAP_I_TO_I:
	 for (i=0;i<ctx->Pixel.MapItoIsize;i++) {
	    values[i] = (GLushort) ctx->Pixel.MapItoI[i];
	 }
	 break;
      case GL_PIXEL_MAP_S_TO_S:
	 for (i=0;i<ctx->Pixel.MapStoSsize;i++) {
	    values[i] = (GLushort) ctx->Pixel.MapStoS[i];
	 }
	 break;
      case GL_PIXEL_MAP_I_TO_R:
	 for (i=0;i<ctx->Pixel.MapItoRsize;i++) {
	    values[i] = FLOAT_TO_USHORT( ctx->Pixel.MapItoR[i] );
	 }
	 break;
      case GL_PIXEL_MAP_I_TO_G:
	 for (i=0;i<ctx->Pixel.MapItoGsize;i++) {
	    values[i] = FLOAT_TO_USHORT( ctx->Pixel.MapItoG[i] );
	 }
	 break;
      case GL_PIXEL_MAP_I_TO_B:
	 for (i=0;i<ctx->Pixel.MapItoBsize;i++) {
	    values[i] = FLOAT_TO_USHORT( ctx->Pixel.MapItoB[i] );
	 }
	 break;
      case GL_PIXEL_MAP_I_TO_A:
	 for (i=0;i<ctx->Pixel.MapItoAsize;i++) {
	    values[i] = FLOAT_TO_USHORT( ctx->Pixel.MapItoA[i] );
	 }
	 break;
      case GL_PIXEL_MAP_R_TO_R:
	 for (i=0;i<ctx->Pixel.MapRtoRsize;i++) {
	    values[i] = FLOAT_TO_USHORT( ctx->Pixel.MapRtoR[i] );
	 }
	 break;
      case GL_PIXEL_MAP_G_TO_G:
	 for (i=0;i<ctx->Pixel.MapGtoGsize;i++) {
	    values[i] = FLOAT_TO_USHORT( ctx->Pixel.MapGtoG[i] );
	 }
	 break;
      case GL_PIXEL_MAP_B_TO_B:
	 for (i=0;i<ctx->Pixel.MapBtoBsize;i++) {
	    values[i] = FLOAT_TO_USHORT( ctx->Pixel.MapBtoB[i] );
	 }
	 break;
      case GL_PIXEL_MAP_A_TO_A:
	 for (i=0;i<ctx->Pixel.MapAtoAsize;i++) {
	    values[i] = FLOAT_TO_USHORT( ctx->Pixel.MapAtoA[i] );
	 }
	 break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetPixelMapfv" );
   }
}



/**********************************************************************/
/*****                       glPixelTransfer                      *****/
/**********************************************************************/


/*
 * Implements glPixelTransfer[fi] whether called immediately or from a
 * display list.
 */
void gl_PixelTransferf( GLcontext *ctx, GLenum pname, GLfloat param )
{
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glPixelTransfer");


   switch (pname) {
      case GL_MAP_COLOR:
         ctx->Pixel.MapColorFlag = param ? GL_TRUE : GL_FALSE;
	 break;
      case GL_MAP_STENCIL:
         ctx->Pixel.MapStencilFlag = param ? GL_TRUE : GL_FALSE;
	 break;
      case GL_INDEX_SHIFT:
         ctx->Pixel.IndexShift = (GLint) param;
	 break;
      case GL_INDEX_OFFSET:
         ctx->Pixel.IndexOffset = (GLint) param;
	 break;
      case GL_RED_SCALE:
         ctx->Pixel.RedScale = param;
	 break;
      case GL_RED_BIAS:
         ctx->Pixel.RedBias = param;
	 break;
      case GL_GREEN_SCALE:
         ctx->Pixel.GreenScale = param;
	 break;
      case GL_GREEN_BIAS:
         ctx->Pixel.GreenBias = param;
	 break;
      case GL_BLUE_SCALE:
         ctx->Pixel.BlueScale = param;
	 break;
      case GL_BLUE_BIAS:
         ctx->Pixel.BlueBias = param;
	 break;
      case GL_ALPHA_SCALE:
         ctx->Pixel.AlphaScale = param;
	 break;
      case GL_ALPHA_BIAS:
         ctx->Pixel.AlphaBias = param;
	 break;
      case GL_DEPTH_SCALE:
         ctx->Pixel.DepthScale = param;
	 break;
      case GL_DEPTH_BIAS:
         ctx->Pixel.DepthBias = param;
	 break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glPixelTransfer(pname)" );
         return;
   }

   if (ctx->Pixel.RedScale!=1.0F   || ctx->Pixel.RedBias!=0.0F ||
       ctx->Pixel.GreenScale!=1.0F || ctx->Pixel.GreenBias!=0.0F ||
       ctx->Pixel.BlueScale!=1.0F  || ctx->Pixel.BlueBias!=0.0F ||
       ctx->Pixel.AlphaScale!=1.0F || ctx->Pixel.AlphaBias!=0.0F) {
      ctx->Pixel.ScaleOrBiasRGBA = GL_TRUE;
   }
   else {
      ctx->Pixel.ScaleOrBiasRGBA = GL_FALSE;
   }
}




/*
 * Pixel processing functions
 */


/*
 * Apply scale and bias factors to an array of RGBA pixels.
 */
void gl_scale_and_bias_color( const GLcontext *ctx, GLuint n,
                              GLfloat red[], GLfloat green[],
                              GLfloat blue[], GLfloat alpha[] )
{
   GLuint i;
   for (i=0;i<n;i++) {
      GLfloat r = red[i]   * ctx->Pixel.RedScale   + ctx->Pixel.RedBias;
      GLfloat g = green[i] * ctx->Pixel.GreenScale + ctx->Pixel.GreenBias;
      GLfloat b = blue[i]  * ctx->Pixel.BlueScale  + ctx->Pixel.BlueBias;
      GLfloat a = alpha[i] * ctx->Pixel.AlphaScale + ctx->Pixel.AlphaBias;
      red[i]   = CLAMP( r, 0.0F, 1.0F );
      green[i] = CLAMP( g, 0.0F, 1.0F );
      blue[i]  = CLAMP( b, 0.0F, 1.0F );
      alpha[i] = CLAMP( a, 0.0F, 1.0F );
   }
}


/*
 * Apply scale and bias factors to an array of RGBA pixels.
 */
void gl_scale_and_bias_rgba( const GLcontext *ctx, GLuint n, GLubyte rgba[][4] )
{
   GLfloat rbias = ctx->Pixel.RedBias   * 255.0F;
   GLfloat gbias = ctx->Pixel.GreenBias * 255.0F;
   GLfloat bbias = ctx->Pixel.BlueBias  * 255.0F;
   GLfloat abias = ctx->Pixel.AlphaBias * 255.0F;
   GLuint i;
   for (i=0;i<n;i++) {
      GLint r = (GLint) (rgba[i][RCOMP] * ctx->Pixel.RedScale   + rbias);
      GLint g = (GLint) (rgba[i][GCOMP] * ctx->Pixel.GreenScale + gbias);
      GLint b = (GLint) (rgba[i][BCOMP] * ctx->Pixel.BlueScale  + bbias);
      GLint a = (GLint) (rgba[i][ACOMP] * ctx->Pixel.AlphaScale + abias);
      rgba[i][RCOMP] = CLAMP( r, 0, 255 );
      rgba[i][GCOMP] = CLAMP( g, 0, 255 );
      rgba[i][BCOMP] = CLAMP( b, 0, 255 );
      rgba[i][ACOMP] = CLAMP( a, 0, 255 );
   }
}


/*
 * Apply scale and bias factors to an array of RGBA pixels.
 */
void gl_scale_and_bias_rgba_float( const GLcontext *ctx, GLuint n, GLfloat rgba[][4] )
{
   if (ctx->Pixel.RedScale != 1.0 || ctx->Pixel.RedBias != 0.0) {
      const GLfloat scale = ctx->Pixel.RedScale;
      const GLfloat bias = ctx->Pixel.RedBias;
      GLuint i;
      for (i = 0; i < n; i++) {
         rgba[i][RCOMP] = rgba[i][RCOMP] * scale + bias;
      }
   }
   if (ctx->Pixel.GreenScale != 1.0 || ctx->Pixel.GreenBias != 0.0) {
      const GLfloat scale = ctx->Pixel.GreenScale;
      const GLfloat bias = ctx->Pixel.GreenBias;
      GLuint i;
      for (i = 0; i < n; i++) {
         rgba[i][GCOMP] = rgba[i][GCOMP] * scale + bias;
      }
   }
   if (ctx->Pixel.BlueScale != 1.0 || ctx->Pixel.BlueBias != 0.0) {
      const GLfloat scale = ctx->Pixel.BlueScale;
      const GLfloat bias = ctx->Pixel.BlueBias;
      GLuint i;
      for (i = 0; i < n; i++) {
         rgba[i][BCOMP] = rgba[i][BCOMP] * scale + bias;
      }
   }
   if (ctx->Pixel.AlphaScale != 1.0 || ctx->Pixel.AlphaBias != 0.0) {
      const GLfloat scale = ctx->Pixel.AlphaScale;
      const GLfloat bias = ctx->Pixel.AlphaBias;
      GLuint i;
      for (i = 0; i < n; i++) {
         rgba[i][ACOMP] = rgba[i][ACOMP] * scale + bias;
      }
   }
}


/*
 * Apply pixel mapping to an array of RGBA pixels.
 */
void gl_map_rgba( const GLcontext *ctx, GLuint n, GLubyte rgba[][4] )
{
   GLfloat rscale = (ctx->Pixel.MapRtoRsize - 1) / 255.0F;
   GLfloat gscale = (ctx->Pixel.MapGtoGsize - 1) / 255.0F;
   GLfloat bscale = (ctx->Pixel.MapBtoBsize - 1) / 255.0F;
   GLfloat ascale = (ctx->Pixel.MapAtoAsize - 1) / 255.0F;
   GLuint i;
   for (i=0;i<n;i++) {
      GLint ir = (GLint) (rgba[i][RCOMP] * rscale);
      GLint ig = (GLint) (rgba[i][GCOMP] * gscale);
      GLint ib = (GLint) (rgba[i][BCOMP] * bscale);
      GLint ia = (GLint) (rgba[i][ACOMP] * ascale);
      rgba[i][RCOMP] = (GLint) (ctx->Pixel.MapRtoR[ir] * 255.0F);
      rgba[i][GCOMP] = (GLint) (ctx->Pixel.MapGtoG[ig] * 255.0F);
      rgba[i][BCOMP] = (GLint) (ctx->Pixel.MapBtoB[ib] * 255.0F);
      rgba[i][ACOMP] = (GLint) (ctx->Pixel.MapAtoA[ia] * 255.0F);
   }
}


/*
 * Apply pixel mapping to an array of floating point RGBA pixels.
 */
void gl_map_rgba_float( const GLcontext *ctx, GLuint n, GLfloat rgba[][4] )
{
   const GLfloat rscale = ctx->Pixel.MapRtoRsize - 1;
   const GLfloat gscale = ctx->Pixel.MapGtoGsize - 1;
   const GLfloat bscale = ctx->Pixel.MapBtoBsize - 1;
   const GLfloat ascale = ctx->Pixel.MapAtoAsize - 1;
   const GLfloat *rMap = ctx->Pixel.MapRtoR;
   const GLfloat *gMap = ctx->Pixel.MapGtoG;
   const GLfloat *bMap = ctx->Pixel.MapBtoB;
   const GLfloat *aMap = ctx->Pixel.MapAtoA;
   GLuint i;
   for (i=0;i<n;i++) {
      rgba[i][RCOMP] = rMap[(GLint) (rgba[i][RCOMP] * rscale + 0.5F)];
      rgba[i][GCOMP] = gMap[(GLint) (rgba[i][GCOMP] * gscale + 0.5F)];
      rgba[i][BCOMP] = bMap[(GLint) (rgba[i][BCOMP] * bscale + 0.5F)];
      rgba[i][ACOMP] = aMap[(GLint) (rgba[i][ACOMP] * ascale + 0.5F)];
   }
}


/*
 * Apply pixel mapping to an array of RGBA pixels.
 */
void gl_map_color( const GLcontext *ctx, GLuint n,
                   GLfloat red[], GLfloat green[],
                   GLfloat blue[], GLfloat alpha[] )
{
   GLfloat rscale = ctx->Pixel.MapRtoRsize - 1;
   GLfloat gscale = ctx->Pixel.MapGtoGsize - 1;
   GLfloat bscale = ctx->Pixel.MapBtoBsize - 1;
   GLfloat ascale = ctx->Pixel.MapAtoAsize - 1;
   GLuint i;
   for (i=0;i<n;i++) {
      red[i]   = ctx->Pixel.MapRtoR[ (GLint) (red[i]   * rscale + 0.5F) ];
      green[i] = ctx->Pixel.MapGtoG[ (GLint) (green[i] * gscale + 0.5F) ];
      blue[i]  = ctx->Pixel.MapBtoB[ (GLint) (blue[i]  * bscale + 0.5F) ];
      alpha[i] = ctx->Pixel.MapAtoA[ (GLint) (alpha[i] * ascale + 0.5F) ];
   }
}



/*
 * Apply color index shift and offset to an array of pixels.
 */
void gl_shift_and_offset_ci( const GLcontext *ctx, GLuint n, GLuint indexes[] )
{
   GLint shift = ctx->Pixel.IndexShift;
   GLint offset = ctx->Pixel.IndexOffset;
   GLuint i;
   if (shift > 0) {
      for (i=0;i<n;i++) {
         indexes[i] = (indexes[i] << shift) + offset;
      }
   }
   else if (shift < 0) {
      shift = -shift;
      for (i=0;i<n;i++) {
         indexes[i] = (indexes[i] >> shift) + offset;
      }
   }
   else {
      for (i=0;i<n;i++) {
         indexes[i] = indexes[i] + offset;
      }
   }
}


/*
 * Apply color index mapping to color indexes.
 */
void gl_map_ci( const GLcontext *ctx, GLuint n, GLuint index[] )
{
   GLuint mask = ctx->Pixel.MapItoIsize - 1;
   GLuint i;
   for (i=0;i<n;i++) {
      index[i] = ctx->Pixel.MapItoI[ index[i] & mask ];
   }
}


/*
 * Map color indexes to rgba values.
 */
void gl_map_ci_to_rgba( const GLcontext *ctx, GLuint n, const GLuint index[],
                        GLubyte rgba[][4] )
{
   GLuint rmask = ctx->Pixel.MapItoRsize - 1;
   GLuint gmask = ctx->Pixel.MapItoGsize - 1;
   GLuint bmask = ctx->Pixel.MapItoBsize - 1;
   GLuint amask = ctx->Pixel.MapItoAsize - 1;
   const GLubyte *rMap = ctx->Pixel.MapItoR8;
   const GLubyte *gMap = ctx->Pixel.MapItoG8;
   const GLubyte *bMap = ctx->Pixel.MapItoB8;
   const GLubyte *aMap = ctx->Pixel.MapItoA8;
   GLuint i;
   for (i=0;i<n;i++) {
      rgba[i][RCOMP] = rMap[index[i] & rmask];
      rgba[i][GCOMP] = gMap[index[i] & gmask];
      rgba[i][BCOMP] = bMap[index[i] & bmask];
      rgba[i][ACOMP] = aMap[index[i] & amask];
   }
}


/*
 * Map color indexes to float rgba values.
 */
void gl_map_ci_to_rgba_float( const GLcontext *ctx, GLuint n, const GLuint index[],
                              GLfloat rgba[][4] )
{
   GLuint rmask = ctx->Pixel.MapItoRsize - 1;
   GLuint gmask = ctx->Pixel.MapItoGsize - 1;
   GLuint bmask = ctx->Pixel.MapItoBsize - 1;
   GLuint amask = ctx->Pixel.MapItoAsize - 1;
   const GLfloat *rMap = ctx->Pixel.MapItoR;
   const GLfloat *gMap = ctx->Pixel.MapItoG;
   const GLfloat *bMap = ctx->Pixel.MapItoB;
   const GLfloat *aMap = ctx->Pixel.MapItoA;
   GLuint i;
   for (i=0;i<n;i++) {
      rgba[i][RCOMP] = rMap[index[i] & rmask];
      rgba[i][GCOMP] = gMap[index[i] & gmask];
      rgba[i][BCOMP] = bMap[index[i] & bmask];
      rgba[i][ACOMP] = aMap[index[i] & amask];
   }
}


/*
 * Map 8-bit color indexes to rgb values.
 */
void gl_map_ci8_to_rgba( const GLcontext *ctx, GLuint n, const GLubyte index[],
                         GLubyte rgba[][4] )
{
   GLuint rmask = ctx->Pixel.MapItoRsize - 1;
   GLuint gmask = ctx->Pixel.MapItoGsize - 1;
   GLuint bmask = ctx->Pixel.MapItoBsize - 1;
   GLuint amask = ctx->Pixel.MapItoAsize - 1;
   const GLubyte *rMap = ctx->Pixel.MapItoR8;
   const GLubyte *gMap = ctx->Pixel.MapItoG8;
   const GLubyte *bMap = ctx->Pixel.MapItoB8;
   const GLubyte *aMap = ctx->Pixel.MapItoA8;
   GLuint i;
   for (i=0;i<n;i++) {
      rgba[i][RCOMP] = rMap[index[i] & rmask];
      rgba[i][GCOMP] = gMap[index[i] & gmask];
      rgba[i][BCOMP] = bMap[index[i] & bmask];
      rgba[i][ACOMP] = aMap[index[i] & amask];
   }
}


void gl_map_ci_to_color( const GLcontext *ctx, GLuint n, const GLuint index[],
                         GLfloat r[], GLfloat g[],
                         GLfloat b[], GLfloat a[] )
{
   GLuint rmask = ctx->Pixel.MapItoRsize - 1;
   GLuint gmask = ctx->Pixel.MapItoGsize - 1;
   GLuint bmask = ctx->Pixel.MapItoBsize - 1;
   GLuint amask = ctx->Pixel.MapItoAsize - 1;
   GLuint i;
   for (i=0;i<n;i++) {
      r[i] = ctx->Pixel.MapItoR[index[i] & rmask];
      g[i] = ctx->Pixel.MapItoG[index[i] & gmask];
      b[i] = ctx->Pixel.MapItoB[index[i] & bmask];
      a[i] = ctx->Pixel.MapItoA[index[i] & amask];
   }
}



void gl_shift_and_offset_stencil( const GLcontext *ctx, GLuint n,
                                  GLstencil stencil[] )
{
   GLuint i;
   GLint shift = ctx->Pixel.IndexShift;
   GLint offset = ctx->Pixel.IndexOffset;
   if (shift > 0) {
      for (i=0;i<n;i++) {
         stencil[i] = (stencil[i] << shift) + offset;
      }
   }
   else if (shift < 0) {
      shift = -shift;
      for (i=0;i<n;i++) {
         stencil[i] = (stencil[i] >> shift) + offset;
      }
   }
   else {
      for (i=0;i<n;i++) {
         stencil[i] = stencil[i] + offset;
      }
   }

}



void gl_map_stencil( const GLcontext *ctx, GLuint n, GLstencil stencil[] )
{
   GLuint mask = ctx->Pixel.MapStoSsize - 1;
   GLuint i;
   for (i=0;i<n;i++) {
      stencil[i] = ctx->Pixel.MapStoS[ stencil[i] & mask ];
   }
}


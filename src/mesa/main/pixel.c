/* $Id: pixel.c,v 1.25 2001/02/27 16:42:01 brianp Exp $ */

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


#ifdef PC_HEADER
#include "all.h"
#else
#include "glheader.h"
#include "colormac.h"
#include "context.h"
#include "macros.h"
#include "mem.h"
#include "pixel.h"
#include "mtypes.h"
#endif



/**********************************************************************/
/*****                    glPixelZoom                             *****/
/**********************************************************************/



void
_mesa_PixelZoom( GLfloat xfactor, GLfloat yfactor )
{
   GET_CURRENT_CONTEXT(ctx);

   if (ctx->Pixel.ZoomX == xfactor &&
       ctx->Pixel.ZoomY == yfactor)
      return;

   FLUSH_VERTICES(ctx, _NEW_PIXEL);
   ctx->Pixel.ZoomX = xfactor;
   ctx->Pixel.ZoomY = yfactor;
}



/**********************************************************************/
/*****                    glPixelStore                            *****/
/**********************************************************************/


void
_mesa_PixelStorei( GLenum pname, GLint param )
{
   /* NOTE: this call can't be compiled into the display list */
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (pname) {
      case GL_PACK_SWAP_BYTES:
	 if (param == (GLint)ctx->Pack.SwapBytes) 
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
         ctx->Pack.SwapBytes = param ? GL_TRUE : GL_FALSE;
	 break;
      case GL_PACK_LSB_FIRST:
	 if (param == (GLint)ctx->Pack.LsbFirst) 
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
         ctx->Pack.LsbFirst = param ? GL_TRUE : GL_FALSE;
	 break;
      case GL_PACK_ROW_LENGTH:
	 if (param<0) {
	    gl_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	    return;
	 }
	 if (ctx->Pack.RowLength == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
	 ctx->Pack.RowLength = param;
	 break;
      case GL_PACK_IMAGE_HEIGHT:
         if (param<0) {
            gl_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	    return;
	 }
	 if (ctx->Pack.ImageHeight == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
	 ctx->Pack.ImageHeight = param;
         break;
      case GL_PACK_SKIP_PIXELS:
	 if (param<0) {
	    gl_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	    return;
	 }
	 if (ctx->Pack.SkipPixels == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
	 ctx->Pack.SkipPixels = param;
	 break;
      case GL_PACK_SKIP_ROWS:
	 if (param<0) {
	    gl_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	    return;
	 }
	 if (ctx->Pack.SkipRows == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
	 ctx->Pack.SkipRows = param;
	 break;
      case GL_PACK_SKIP_IMAGES:
	 if (param<0) {
	    gl_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	    return;
	 }
	 if (ctx->Pack.SkipImages == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
	 ctx->Pack.SkipImages = param;
	 break;
      case GL_PACK_ALIGNMENT:
         if (param!=1 && param!=2 && param!=4 && param!=8) {
	    gl_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	    return;
	 }
	 if (ctx->Pack.Alignment == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
	 ctx->Pack.Alignment = param;
	 break;
      case GL_UNPACK_SWAP_BYTES:
	 if (param == (GLint)ctx->Unpack.SwapBytes) 
	    return;
	 if ((GLint)ctx->Unpack.SwapBytes == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
	 ctx->Unpack.SwapBytes = param ? GL_TRUE : GL_FALSE;
         break;
      case GL_UNPACK_LSB_FIRST:
	 if (param == (GLint)ctx->Unpack.LsbFirst) 
	    return;
	 if ((GLint)ctx->Unpack.LsbFirst == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
	 ctx->Unpack.LsbFirst = param ? GL_TRUE : GL_FALSE;
	 break;
      case GL_UNPACK_ROW_LENGTH:
	 if (param<0) {
	    gl_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	    return;
	 }
	 if (ctx->Unpack.RowLength == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
	 ctx->Unpack.RowLength = param;
	 break;
      case GL_UNPACK_IMAGE_HEIGHT:
         if (param<0) {
            gl_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	    return;
	 }
	 if (ctx->Unpack.ImageHeight == param)
	    return;

	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
	 ctx->Unpack.ImageHeight = param;
         break;
      case GL_UNPACK_SKIP_PIXELS:
	 if (param<0) {
	    gl_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	    return;
	 }
	 if (ctx->Unpack.SkipPixels == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
	 ctx->Unpack.SkipPixels = param;
	 break;
      case GL_UNPACK_SKIP_ROWS:
	 if (param<0) {
	    gl_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	    return;
	 }
	 if (ctx->Unpack.SkipRows == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
	 ctx->Unpack.SkipRows = param;
	 break;
      case GL_UNPACK_SKIP_IMAGES:
	 if (param < 0) {
	    gl_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	    return;
	 }
	 if (ctx->Unpack.SkipImages == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
	 ctx->Unpack.SkipImages = param;
	 break;
      case GL_UNPACK_ALIGNMENT:
         if (param!=1 && param!=2 && param!=4 && param!=8) {
	    gl_error( ctx, GL_INVALID_VALUE, "glPixelStore" );
	    return;
	 }
	 if (ctx->Unpack.Alignment == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
	 ctx->Unpack.Alignment = param;
	 break;
      default:
	 gl_error( ctx, GL_INVALID_ENUM, "glPixelStore" );
	 return;
   }
}


void
_mesa_PixelStoref( GLenum pname, GLfloat param )
{
   _mesa_PixelStorei( pname, (GLint) param );
}



/**********************************************************************/
/*****                         glPixelMap                         *****/
/**********************************************************************/



void
_mesa_PixelMapfv( GLenum map, GLint mapsize, const GLfloat *values )
{
   GLint i;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

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

   FLUSH_VERTICES(ctx, _NEW_PIXEL);

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



void
_mesa_PixelMapuiv(GLenum map, GLint mapsize, const GLuint *values )
{
   GLfloat fvalues[MAX_PIXEL_MAP_TABLE];
   GLint i;
   if (map==GL_PIXEL_MAP_I_TO_I || map==GL_PIXEL_MAP_S_TO_S) {
      for (i=0;i<mapsize;i++) {
         fvalues[i] = (GLfloat) values[i];
      }
   }
   else {
      for (i=0;i<mapsize;i++) {
         fvalues[i] = UINT_TO_FLOAT( values[i] );
      }
   }
   _mesa_PixelMapfv(map, mapsize, fvalues);
}



void
_mesa_PixelMapusv(GLenum map, GLint mapsize, const GLushort *values )
{
   GLfloat fvalues[MAX_PIXEL_MAP_TABLE];
   GLint i;
   if (map==GL_PIXEL_MAP_I_TO_I || map==GL_PIXEL_MAP_S_TO_S) {
      for (i=0;i<mapsize;i++) {
         fvalues[i] = (GLfloat) values[i];
      }
   }
   else {
      for (i=0;i<mapsize;i++) {
         fvalues[i] = USHORT_TO_FLOAT( values[i] );
      }
   }
   _mesa_PixelMapfv(map, mapsize, fvalues);
}



void
_mesa_GetPixelMapfv( GLenum map, GLfloat *values )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint i;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

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


void
_mesa_GetPixelMapuiv( GLenum map, GLuint *values )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint i;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

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


void
_mesa_GetPixelMapusv( GLenum map, GLushort *values )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint i;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

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
void
_mesa_PixelTransferf( GLenum pname, GLfloat param )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (pname) {
      case GL_MAP_COLOR:
         if (ctx->Pixel.MapColorFlag == param ? GL_TRUE : GL_FALSE)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.MapColorFlag = param ? GL_TRUE : GL_FALSE;
	 break;
      case GL_MAP_STENCIL:
         if (ctx->Pixel.MapStencilFlag == param ? GL_TRUE : GL_FALSE)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.MapStencilFlag = param ? GL_TRUE : GL_FALSE;
	 break;
      case GL_INDEX_SHIFT:
         if (ctx->Pixel.IndexShift == (GLint) param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.IndexShift = (GLint) param;
	 break;
      case GL_INDEX_OFFSET:
         if (ctx->Pixel.IndexOffset == (GLint) param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.IndexOffset = (GLint) param;
	 break;
      case GL_RED_SCALE:
         if (ctx->Pixel.RedScale == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.RedScale = param;
	 break;
      case GL_RED_BIAS:
         if (ctx->Pixel.RedBias == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.RedBias = param;
	 break;
      case GL_GREEN_SCALE:
         if (ctx->Pixel.GreenScale == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.GreenScale = param;
	 break;
      case GL_GREEN_BIAS:
         if (ctx->Pixel.GreenBias == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.GreenBias = param;
	 break;
      case GL_BLUE_SCALE:
         if (ctx->Pixel.BlueScale == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.BlueScale = param;
	 break;
      case GL_BLUE_BIAS:
         if (ctx->Pixel.BlueBias == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.BlueBias = param;
	 break;
      case GL_ALPHA_SCALE:
         if (ctx->Pixel.AlphaScale == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.AlphaScale = param;
	 break;
      case GL_ALPHA_BIAS:
         if (ctx->Pixel.AlphaBias == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.AlphaBias = param;
	 break;
      case GL_DEPTH_SCALE:
         if (ctx->Pixel.DepthScale == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.DepthScale = param;
	 break;
      case GL_DEPTH_BIAS:
         if (ctx->Pixel.DepthBias == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.DepthBias = param;
	 break;
      case GL_POST_COLOR_MATRIX_RED_SCALE:
         if (ctx->Pixel.PostColorMatrixScale[0] == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.PostColorMatrixScale[0] = param;
	 break;
      case GL_POST_COLOR_MATRIX_RED_BIAS:
         if (ctx->Pixel.PostColorMatrixBias[0] == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.PostColorMatrixBias[0] = param;
	 break;
      case GL_POST_COLOR_MATRIX_GREEN_SCALE:
         if (ctx->Pixel.PostColorMatrixScale[1] == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.PostColorMatrixScale[1] = param;
	 break;
      case GL_POST_COLOR_MATRIX_GREEN_BIAS:
         if (ctx->Pixel.PostColorMatrixBias[1] == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.PostColorMatrixBias[1] = param;
	 break;
      case GL_POST_COLOR_MATRIX_BLUE_SCALE:
         if (ctx->Pixel.PostColorMatrixScale[2] == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.PostColorMatrixScale[2] = param;
	 break;
      case GL_POST_COLOR_MATRIX_BLUE_BIAS:
         if (ctx->Pixel.PostColorMatrixBias[2] == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.PostColorMatrixBias[2] = param;
	 break;
      case GL_POST_COLOR_MATRIX_ALPHA_SCALE:
         if (ctx->Pixel.PostColorMatrixScale[3] == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.PostColorMatrixScale[3] = param;
	 break;
      case GL_POST_COLOR_MATRIX_ALPHA_BIAS:
         if (ctx->Pixel.PostColorMatrixBias[3] == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.PostColorMatrixBias[3] = param;
	 break;
      case GL_POST_CONVOLUTION_RED_SCALE:
         if (ctx->Pixel.PostConvolutionScale[0] == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.PostConvolutionScale[0] = param;
	 break;
      case GL_POST_CONVOLUTION_RED_BIAS:
         if (ctx->Pixel.PostConvolutionBias[0] == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.PostConvolutionBias[0] = param;
	 break;
      case GL_POST_CONVOLUTION_GREEN_SCALE:
         if (ctx->Pixel.PostConvolutionScale[1] == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.PostConvolutionScale[1] = param;
	 break;
      case GL_POST_CONVOLUTION_GREEN_BIAS:
         if (ctx->Pixel.PostConvolutionBias[1] == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.PostConvolutionBias[1] = param;
	 break;
      case GL_POST_CONVOLUTION_BLUE_SCALE:
         if (ctx->Pixel.PostConvolutionScale[2] == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.PostConvolutionScale[2] = param;
	 break;
      case GL_POST_CONVOLUTION_BLUE_BIAS:
         if (ctx->Pixel.PostConvolutionBias[2] == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.PostConvolutionBias[2] = param;
	 break;
      case GL_POST_CONVOLUTION_ALPHA_SCALE:
         if (ctx->Pixel.PostConvolutionScale[2] == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.PostConvolutionScale[2] = param;
	 break;
      case GL_POST_CONVOLUTION_ALPHA_BIAS:
         if (ctx->Pixel.PostConvolutionBias[2] == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.PostConvolutionBias[2] = param;
	 break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glPixelTransfer(pname)" );
         return;
   }
}


void
_mesa_PixelTransferi( GLenum pname, GLint param )
{
   _mesa_PixelTransferf( pname, (GLfloat) param );
}



/**********************************************************************/
/*****                  Pixel processing functions               ******/
/**********************************************************************/


/*
 * Apply scale and bias factors to an array of RGBA pixels.
 */
void
_mesa_scale_and_bias_rgba(const GLcontext *ctx, GLuint n, GLfloat rgba[][4],
                          GLfloat rScale, GLfloat gScale,
                          GLfloat bScale, GLfloat aScale,
                          GLfloat rBias, GLfloat gBias,
                          GLfloat bBias, GLfloat aBias)
{
   if (rScale != 1.0 || rBias != 0.0) {
      GLuint i;
      for (i = 0; i < n; i++) {
         rgba[i][RCOMP] = rgba[i][RCOMP] * rScale + rBias;
      }
   }
   if (gScale != 1.0 || gBias != 0.0) {
      GLuint i;
      for (i = 0; i < n; i++) {
         rgba[i][GCOMP] = rgba[i][GCOMP] * gScale + gBias;
      }
   }
   if (bScale != 1.0 || bBias != 0.0) {
      GLuint i;
      for (i = 0; i < n; i++) {
         rgba[i][BCOMP] = rgba[i][BCOMP] * bScale + bBias;
      }
   }
   if (aScale != 1.0 || aBias != 0.0) {
      GLuint i;
      for (i = 0; i < n; i++) {
         rgba[i][ACOMP] = rgba[i][ACOMP] * aScale + aBias;
      }
   }
}


/*
 * Apply pixel mapping to an array of floating point RGBA pixels.
 */
void
_mesa_map_rgba( const GLcontext *ctx, GLuint n, GLfloat rgba[][4] )
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
 * Apply the color matrix and post color matrix scaling and biasing.
 */
void
_mesa_transform_rgba(const GLcontext *ctx, GLuint n, GLfloat rgba[][4])
{
   const GLfloat rs = ctx->Pixel.PostColorMatrixScale[0];
   const GLfloat rb = ctx->Pixel.PostColorMatrixBias[0];
   const GLfloat gs = ctx->Pixel.PostColorMatrixScale[1];
   const GLfloat gb = ctx->Pixel.PostColorMatrixBias[1];
   const GLfloat bs = ctx->Pixel.PostColorMatrixScale[2];
   const GLfloat bb = ctx->Pixel.PostColorMatrixBias[2];
   const GLfloat as = ctx->Pixel.PostColorMatrixScale[3];
   const GLfloat ab = ctx->Pixel.PostColorMatrixBias[3];
   const GLfloat *m = ctx->ColorMatrix.m;
   GLuint i;
   for (i = 0; i < n; i++) {
      const GLfloat r = rgba[i][RCOMP];
      const GLfloat g = rgba[i][GCOMP];
      const GLfloat b = rgba[i][BCOMP];
      const GLfloat a = rgba[i][ACOMP];
      rgba[i][RCOMP] = (m[0] * r + m[4] * g + m[ 8] * b + m[12] * a) * rs + rb;
      rgba[i][GCOMP] = (m[1] * r + m[5] * g + m[ 9] * b + m[13] * a) * gs + gb;
      rgba[i][BCOMP] = (m[2] * r + m[6] * g + m[10] * b + m[14] * a) * bs + bb;
      rgba[i][ACOMP] = (m[3] * r + m[7] * g + m[11] * b + m[15] * a) * as + ab;
   }
}


/*
 * Apply a color table lookup to an array of colors.
 */
void
_mesa_lookup_rgba(const struct gl_color_table *table,
                  GLuint n, GLfloat rgba[][4])
{
   ASSERT(table->FloatTable);
   if (!table->Table || table->Size == 0)
      return;

   switch (table->Format) {
      case GL_INTENSITY:
         /* replace RGBA with I */
         if (!table->FloatTable) {
            const GLfloat scale = (GLfloat) (table->Size - 1);
            const GLchan *lut = (const GLchan *) table->Table;
            GLuint i;
            for (i = 0; i < n; i++) {
               GLint j = (GLint) (rgba[i][RCOMP] * scale + 0.5F);
               GLfloat c = CHAN_TO_FLOAT(lut[j]);
               rgba[i][RCOMP] = rgba[i][GCOMP] =
                  rgba[i][BCOMP] = rgba[i][ACOMP] = c;
            }

         }
         else {
            const GLfloat scale = (GLfloat) (table->Size - 1);
            const GLfloat *lut = (const GLfloat *) table->Table;
            GLuint i;
            for (i = 0; i < n; i++) {
               GLint j = (GLint) (rgba[i][RCOMP] * scale + 0.5F);
               GLfloat c = lut[j];
               rgba[i][RCOMP] = rgba[i][GCOMP] =
                  rgba[i][BCOMP] = rgba[i][ACOMP] = c;
            }
         }
         break;
      case GL_LUMINANCE:
         /* replace RGB with L */
         if (!table->FloatTable) {
            const GLfloat scale = (GLfloat) (table->Size - 1);
            const GLchan *lut = (const GLchan *) table->Table;
            GLuint i;
            for (i = 0; i < n; i++) {
               GLint j = (GLint) (rgba[i][RCOMP] * scale + 0.5F);
               GLfloat c = CHAN_TO_FLOAT(lut[j]);
               rgba[i][RCOMP] = rgba[i][GCOMP] = rgba[i][BCOMP] = c;
            }
         }
         else {
            const GLfloat scale = (GLfloat) (table->Size - 1);
            const GLfloat *lut = (const GLfloat *) table->Table;
            GLuint i;
            for (i = 0; i < n; i++) {
               GLint j = (GLint) (rgba[i][RCOMP] * scale + 0.5F);
               GLfloat c = lut[j];
               rgba[i][RCOMP] = rgba[i][GCOMP] = rgba[i][BCOMP] = c;
            }
         }
         break;
      case GL_ALPHA:
         /* replace A with A */
         if (!table->FloatTable) {
            const GLfloat scale = (GLfloat) (table->Size - 1);
            const GLchan *lut = (const GLchan *) table->Table;
            GLuint i;
            for (i = 0; i < n; i++) {
               GLint j = (GLint) (rgba[i][ACOMP] * scale + 0.5F);
               rgba[i][ACOMP] = CHAN_TO_FLOAT(lut[j]);
            }
         }
         else  {
            const GLfloat scale = (GLfloat) (table->Size - 1);
            const GLfloat *lut = (const GLfloat *) table->Table;
            GLuint i;
            for (i = 0; i < n; i++) {
               GLint j = (GLint) (rgba[i][ACOMP] * scale + 0.5F);
               rgba[i][ACOMP] = lut[j];
            }
         }
         break;
      case GL_LUMINANCE_ALPHA:
         /* replace RGBA with LLLA */
         if (!table->FloatTable) {
            const GLfloat scale = (GLfloat) (table->Size - 1);
            const GLchan *lut = (const GLchan *) table->Table;
            GLuint i;
            for (i = 0; i < n; i++) {
               GLint jL = (GLint) (rgba[i][RCOMP] * scale + 0.5F);
               GLint jA = (GLint) (rgba[i][ACOMP] * scale + 0.5F);
               GLfloat luminance = CHAN_TO_FLOAT(lut[jL * 2 + 0]);
               GLfloat alpha     = CHAN_TO_FLOAT(lut[jA * 2 + 1]);
               rgba[i][RCOMP] = rgba[i][GCOMP] = rgba[i][BCOMP] = luminance;
               rgba[i][ACOMP] = alpha;;
            }
         }
         else {
            const GLfloat scale = (GLfloat) (table->Size - 1);
            const GLfloat *lut = (const GLfloat *) table->Table;
            GLuint i;
            for (i = 0; i < n; i++) {
               GLint jL = (GLint) (rgba[i][RCOMP] * scale + 0.5F);
               GLint jA = (GLint) (rgba[i][ACOMP] * scale + 0.5F);
               GLfloat luminance = lut[jL * 2 + 0];
               GLfloat alpha     = lut[jA * 2 + 1];
               rgba[i][RCOMP] = rgba[i][GCOMP] = rgba[i][BCOMP] = luminance;
               rgba[i][ACOMP] = alpha;;
            }
         }
         break;
      case GL_RGB:
         /* replace RGB with RGB */
         if (!table->FloatTable) {
            const GLfloat scale = (GLfloat) (table->Size - 1);
            const GLchan *lut = (const GLchan *) table->Table;
            GLuint i;
            for (i = 0; i < n; i++) {
               GLint jR = (GLint) (rgba[i][RCOMP] * scale + 0.5F);
               GLint jG = (GLint) (rgba[i][GCOMP] * scale + 0.5F);
               GLint jB = (GLint) (rgba[i][BCOMP] * scale + 0.5F);
               rgba[i][RCOMP] = CHAN_TO_FLOAT(lut[jR * 3 + 0]);
               rgba[i][GCOMP] = CHAN_TO_FLOAT(lut[jG * 3 + 1]);
               rgba[i][BCOMP] = CHAN_TO_FLOAT(lut[jB * 3 + 2]);
            }
         }
         else {
            const GLfloat scale = (GLfloat) (table->Size - 1);
            const GLfloat *lut = (const GLfloat *) table->Table;
            GLuint i;
            for (i = 0; i < n; i++) {
               GLint jR = (GLint) (rgba[i][RCOMP] * scale + 0.5F);
               GLint jG = (GLint) (rgba[i][GCOMP] * scale + 0.5F);
               GLint jB = (GLint) (rgba[i][BCOMP] * scale + 0.5F);
               rgba[i][RCOMP] = lut[jR * 3 + 0];
               rgba[i][GCOMP] = lut[jG * 3 + 1];
               rgba[i][BCOMP] = lut[jB * 3 + 2];
            }
         }
         break;
      case GL_RGBA:
         /* replace RGBA with RGBA */
         if (!table->FloatTable) {
            const GLfloat scale = (GLfloat) (table->Size - 1);
            const GLchan *lut = (const GLchan *) table->Table;
            GLuint i;
            for (i = 0; i < n; i++) {
               GLint jR = (GLint) (rgba[i][RCOMP] * scale + 0.5F);
               GLint jG = (GLint) (rgba[i][GCOMP] * scale + 0.5F);
               GLint jB = (GLint) (rgba[i][BCOMP] * scale + 0.5F);
               GLint jA = (GLint) (rgba[i][ACOMP] * scale + 0.5F);
               rgba[i][RCOMP] = CHAN_TO_FLOAT(lut[jR * 4 + 0]);
               rgba[i][GCOMP] = CHAN_TO_FLOAT(lut[jG * 4 + 1]);
               rgba[i][BCOMP] = CHAN_TO_FLOAT(lut[jB * 4 + 2]);
               rgba[i][ACOMP] = CHAN_TO_FLOAT(lut[jA * 4 + 3]);
            }
         }
         else {
            const GLfloat scale = (GLfloat) (table->Size - 1);
            const GLfloat *lut = (const GLfloat *) table->Table;
            GLuint i;
            for (i = 0; i < n; i++) {
               GLint jR = (GLint) (rgba[i][RCOMP] * scale + 0.5F);
               GLint jG = (GLint) (rgba[i][GCOMP] * scale + 0.5F);
               GLint jB = (GLint) (rgba[i][BCOMP] * scale + 0.5F);
               GLint jA = (GLint) (rgba[i][ACOMP] * scale + 0.5F);
               rgba[i][RCOMP] = lut[jR * 4 + 0];
               rgba[i][GCOMP] = lut[jG * 4 + 1];
               rgba[i][BCOMP] = lut[jB * 4 + 2];
               rgba[i][ACOMP] = lut[jA * 4 + 3];
            }
         }
         break;
      default:
         gl_problem(NULL, "Bad format in _mesa_lookup_rgba");
         return;
   }
}



/*
 * Apply color index shift and offset to an array of pixels.
 */
void
_mesa_shift_and_offset_ci( const GLcontext *ctx, GLuint n, GLuint indexes[] )
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
void
_mesa_map_ci( const GLcontext *ctx, GLuint n, GLuint index[] )
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
void
_mesa_map_ci_to_rgba_chan( const GLcontext *ctx, GLuint n,
                           const GLuint index[], GLchan rgba[][4] )
{
#if CHAN_BITS == 8
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
#else
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
      CLAMPED_FLOAT_TO_CHAN(rgba[i][RCOMP], rMap[index[i] & rmask]);
      CLAMPED_FLOAT_TO_CHAN(rgba[i][GCOMP], gMap[index[i] & gmask]);
      CLAMPED_FLOAT_TO_CHAN(rgba[i][BCOMP], bMap[index[i] & bmask]);
      CLAMPED_FLOAT_TO_CHAN(rgba[i][ACOMP], aMap[index[i] & amask]);
   }
#endif
}


/*
 * Map color indexes to float rgba values.
 */
void
_mesa_map_ci_to_rgba( const GLcontext *ctx, GLuint n,
                      const GLuint index[], GLfloat rgba[][4] )
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
void
_mesa_map_ci8_to_rgba( const GLcontext *ctx, GLuint n, const GLubyte index[],
                       GLchan rgba[][4] )
{
#if CHAN_BITS == 8
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
#else
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
      CLAMPED_FLOAT_TO_CHAN(rgba[i][RCOMP], rMap[index[i] & rmask]);
      CLAMPED_FLOAT_TO_CHAN(rgba[i][GCOMP], gMap[index[i] & gmask]);
      CLAMPED_FLOAT_TO_CHAN(rgba[i][BCOMP], bMap[index[i] & bmask]);
      CLAMPED_FLOAT_TO_CHAN(rgba[i][ACOMP], aMap[index[i] & amask]);
   }
#endif
}


void
_mesa_shift_and_offset_stencil( const GLcontext *ctx, GLuint n,
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


void
_mesa_map_stencil( const GLcontext *ctx, GLuint n, GLstencil stencil[] )
{
   GLuint mask = ctx->Pixel.MapStoSsize - 1;
   GLuint i;
   for (i=0;i<n;i++) {
      stencil[i] = ctx->Pixel.MapStoS[ stencil[i] & mask ];
   }
}



/*
 * This function converts an array of GLchan colors to GLfloat colors.
 * Most importantly, it undoes the non-uniform quantization of pixel
 * values introduced when we convert shallow (< 8 bit) pixel values
 * to GLubytes in the ctx->Driver.ReadRGBASpan() functions.
 * This fixes a number of OpenGL conformance failures when running on
 * 16bpp displays, for example.
 */
void
_mesa_chan_to_float_span(const GLcontext *ctx, GLuint n,
                         CONST GLchan rgba[][4], GLfloat rgbaf[][4])
{
   const GLuint rShift = CHAN_BITS - ctx->Visual.redBits;
   const GLuint gShift = CHAN_BITS - ctx->Visual.greenBits;
   const GLuint bShift = CHAN_BITS - ctx->Visual.blueBits;
   GLuint aShift;
   const GLfloat rScale = 1.0 / (GLfloat) ((1 << ctx->Visual.redBits  ) - 1);
   const GLfloat gScale = 1.0 / (GLfloat) ((1 << ctx->Visual.greenBits) - 1);
   const GLfloat bScale = 1.0 / (GLfloat) ((1 << ctx->Visual.blueBits ) - 1);
   GLfloat aScale;
   GLuint i;

   if (ctx->Visual.alphaBits > 0) {
      aShift = CHAN_BITS - ctx->Visual.alphaBits;
      aScale = 1.0 / (GLfloat) ((1 << ctx->Visual.alphaBits) - 1);
   }
   else {
      aShift = 0;
      aScale = 1.0F / CHAN_MAXF;
   }

   for (i = 0; i < n; i++) {
      const GLint r = rgba[i][RCOMP] >> rShift;
      const GLint g = rgba[i][GCOMP] >> gShift;
      const GLint b = rgba[i][BCOMP] >> bShift;
      const GLint a = rgba[i][ACOMP] >> aShift;
      rgbaf[i][RCOMP] = (GLfloat) r * rScale;
      rgbaf[i][GCOMP] = (GLfloat) g * gScale;
      rgbaf[i][BCOMP] = (GLfloat) b * bScale;
      rgbaf[i][ACOMP] = (GLfloat) a * aScale;
   }
}

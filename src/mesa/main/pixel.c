/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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


/**
 * \file pixel.c
 * Pixel transfer functions (glPixelZoom, glPixelMap, glPixelTransfer)
 */

#include "glheader.h"
#include "bufferobj.h"
#include "colormac.h"
#include "context.h"
#include "macros.h"
#include "pixel.h"
#include "mtypes.h"
#include "main/dispatch.h"


#if FEATURE_pixel_transfer


/**********************************************************************/
/*****                    glPixelZoom                             *****/
/**********************************************************************/

static void GLAPIENTRY
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
/*****                         glPixelMap                         *****/
/**********************************************************************/

/**
 * Return pointer to a pixelmap by name.
 */
static struct gl_pixelmap *
get_pixelmap(GLcontext *ctx, GLenum map)
{
   switch (map) {
   case GL_PIXEL_MAP_I_TO_I:
      return &ctx->PixelMaps.ItoI;
   case GL_PIXEL_MAP_S_TO_S:
      return &ctx->PixelMaps.StoS;
   case GL_PIXEL_MAP_I_TO_R:
      return &ctx->PixelMaps.ItoR;
   case GL_PIXEL_MAP_I_TO_G:
      return &ctx->PixelMaps.ItoG;
   case GL_PIXEL_MAP_I_TO_B:
      return &ctx->PixelMaps.ItoB;
   case GL_PIXEL_MAP_I_TO_A:
      return &ctx->PixelMaps.ItoA;
   case GL_PIXEL_MAP_R_TO_R:
      return &ctx->PixelMaps.RtoR;
   case GL_PIXEL_MAP_G_TO_G:
      return &ctx->PixelMaps.GtoG;
   case GL_PIXEL_MAP_B_TO_B:
      return &ctx->PixelMaps.BtoB;
   case GL_PIXEL_MAP_A_TO_A:
      return &ctx->PixelMaps.AtoA;
   default:
      return NULL;
   }
}


/**
 * Helper routine used by the other _mesa_PixelMap() functions.
 */
static void
store_pixelmap(GLcontext *ctx, GLenum map, GLsizei mapsize,
               const GLfloat *values)
{
   GLint i;
   struct gl_pixelmap *pm = get_pixelmap(ctx, map);
   if (!pm) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glPixelMap(map)");
      return;
   }

   switch (map) {
   case GL_PIXEL_MAP_S_TO_S:
      /* special case */
      ctx->PixelMaps.StoS.Size = mapsize;
      for (i = 0; i < mapsize; i++) {
         ctx->PixelMaps.StoS.Map[i] = (GLfloat)IROUND(values[i]);
      }
      break;
   case GL_PIXEL_MAP_I_TO_I:
      /* special case */
      ctx->PixelMaps.ItoI.Size = mapsize;
      for (i = 0; i < mapsize; i++) {
         ctx->PixelMaps.ItoI.Map[i] = values[i];
      }
      break;
   default:
      /* general case */
      pm->Size = mapsize;
      for (i = 0; i < mapsize; i++) {
         GLfloat val = CLAMP(values[i], 0.0F, 1.0F);
         pm->Map[i] = val;
         pm->Map8[i] = (GLint) (val * 255.0F);
      }
   }
}


/**
 * Convenience wrapper for _mesa_validate_pbo_access() for gl[Get]PixelMap().
 */
static GLboolean
validate_pbo_access(GLcontext *ctx, struct gl_pixelstore_attrib *pack,
                    GLsizei mapsize, GLenum format, GLenum type,
                    const GLvoid *ptr)
{
   GLboolean ok;

   /* Note, need to use DefaultPacking and Unpack's buffer object */
   _mesa_reference_buffer_object(ctx,
                                 &ctx->DefaultPacking.BufferObj,
                                 pack->BufferObj);

   ok = _mesa_validate_pbo_access(1, &ctx->DefaultPacking, mapsize, 1, 1,
                                  format, type, ptr);

   /* restore */
   _mesa_reference_buffer_object(ctx,
                                 &ctx->DefaultPacking.BufferObj,
                                 ctx->Shared->NullBufferObj);

   if (!ok) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glPixelMap(invalid PBO access)");
   }
   return ok;
}


static void GLAPIENTRY
_mesa_PixelMapfv( GLenum map, GLsizei mapsize, const GLfloat *values )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   /* XXX someday, test against ctx->Const.MaxPixelMapTableSize */
   if (mapsize < 1 || mapsize > MAX_PIXEL_MAP_TABLE) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glPixelMapfv(mapsize)" );
      return;
   }

   if (map >= GL_PIXEL_MAP_S_TO_S && map <= GL_PIXEL_MAP_I_TO_A) {
      /* test that mapsize is a power of two */
      if (!_mesa_is_pow_two(mapsize)) {
	 _mesa_error( ctx, GL_INVALID_VALUE, "glPixelMapfv(mapsize)" );
         return;
      }
   }

   FLUSH_VERTICES(ctx, _NEW_PIXEL);

   if (!validate_pbo_access(ctx, &ctx->Unpack, mapsize,
                            GL_INTENSITY, GL_FLOAT, values)) {
      return;
   }

   values = (const GLfloat *) _mesa_map_pbo_source(ctx, &ctx->Unpack, values);
   if (!values) {
      if (_mesa_is_bufferobj(ctx->Unpack.BufferObj)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glPixelMapfv(PBO is mapped)");
      }
      return;
   }

   store_pixelmap(ctx, map, mapsize, values);

   _mesa_unmap_pbo_source(ctx, &ctx->Unpack);
}


static void GLAPIENTRY
_mesa_PixelMapuiv(GLenum map, GLsizei mapsize, const GLuint *values )
{
   GLfloat fvalues[MAX_PIXEL_MAP_TABLE];
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (mapsize < 1 || mapsize > MAX_PIXEL_MAP_TABLE) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glPixelMapuiv(mapsize)" );
      return;
   }

   if (map >= GL_PIXEL_MAP_S_TO_S && map <= GL_PIXEL_MAP_I_TO_A) {
      /* test that mapsize is a power of two */
      if (!_mesa_is_pow_two(mapsize)) {
	 _mesa_error( ctx, GL_INVALID_VALUE, "glPixelMapuiv(mapsize)" );
         return;
      }
   }

   FLUSH_VERTICES(ctx, _NEW_PIXEL);

   if (!validate_pbo_access(ctx, &ctx->Unpack, mapsize,
                            GL_INTENSITY, GL_UNSIGNED_INT, values)) {
      return;
   }

   values = (const GLuint *) _mesa_map_pbo_source(ctx, &ctx->Unpack, values);
   if (!values) {
      if (_mesa_is_bufferobj(ctx->Unpack.BufferObj)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glPixelMapuiv(PBO is mapped)");
      }
      return;
   }

   /* convert to floats */
   if (map == GL_PIXEL_MAP_I_TO_I || map == GL_PIXEL_MAP_S_TO_S) {
      GLint i;
      for (i = 0; i < mapsize; i++) {
         fvalues[i] = (GLfloat) values[i];
      }
   }
   else {
      GLint i;
      for (i = 0; i < mapsize; i++) {
         fvalues[i] = UINT_TO_FLOAT( values[i] );
      }
   }

   _mesa_unmap_pbo_source(ctx, &ctx->Unpack);

   store_pixelmap(ctx, map, mapsize, fvalues);
}


static void GLAPIENTRY
_mesa_PixelMapusv(GLenum map, GLsizei mapsize, const GLushort *values )
{
   GLfloat fvalues[MAX_PIXEL_MAP_TABLE];
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (mapsize < 1 || mapsize > MAX_PIXEL_MAP_TABLE) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glPixelMapusv(mapsize)" );
      return;
   }

   if (map >= GL_PIXEL_MAP_S_TO_S && map <= GL_PIXEL_MAP_I_TO_A) {
      /* test that mapsize is a power of two */
      if (!_mesa_is_pow_two(mapsize)) {
	 _mesa_error( ctx, GL_INVALID_VALUE, "glPixelMapuiv(mapsize)" );
         return;
      }
   }

   FLUSH_VERTICES(ctx, _NEW_PIXEL);

   if (!validate_pbo_access(ctx, &ctx->Unpack, mapsize,
                            GL_INTENSITY, GL_UNSIGNED_SHORT, values)) {
      return;
   }

   values = (const GLushort *) _mesa_map_pbo_source(ctx, &ctx->Unpack, values);
   if (!values) {
      if (_mesa_is_bufferobj(ctx->Unpack.BufferObj)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glPixelMapusv(PBO is mapped)");
      }
      return;
   }

   /* convert to floats */
   if (map == GL_PIXEL_MAP_I_TO_I || map == GL_PIXEL_MAP_S_TO_S) {
      GLint i;
      for (i = 0; i < mapsize; i++) {
         fvalues[i] = (GLfloat) values[i];
      }
   }
   else {
      GLint i;
      for (i = 0; i < mapsize; i++) {
         fvalues[i] = USHORT_TO_FLOAT( values[i] );
      }
   }

   _mesa_unmap_pbo_source(ctx, &ctx->Unpack);

   store_pixelmap(ctx, map, mapsize, fvalues);
}


static void GLAPIENTRY
_mesa_GetPixelMapfv( GLenum map, GLfloat *values )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint mapsize, i;
   const struct gl_pixelmap *pm;

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   pm = get_pixelmap(ctx, map);
   if (!pm) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetPixelMapfv(map)");
      return;
   }

   mapsize = pm->Size;

   if (!validate_pbo_access(ctx, &ctx->Pack, mapsize,
                            GL_INTENSITY, GL_FLOAT, values)) {
      return;
   }

   values = (GLfloat *) _mesa_map_pbo_dest(ctx, &ctx->Pack, values);
   if (!values) {
      if (_mesa_is_bufferobj(ctx->Pack.BufferObj)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glGetPixelMapfv(PBO is mapped)");
      }
      return;
   }

   if (map == GL_PIXEL_MAP_S_TO_S) {
      /* special case */
      for (i = 0; i < mapsize; i++) {
         values[i] = (GLfloat) ctx->PixelMaps.StoS.Map[i];
      }
   }
   else {
      memcpy(values, pm->Map, mapsize * sizeof(GLfloat));
   }

   _mesa_unmap_pbo_dest(ctx, &ctx->Pack);
}


static void GLAPIENTRY
_mesa_GetPixelMapuiv( GLenum map, GLuint *values )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint mapsize, i;
   const struct gl_pixelmap *pm;

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   pm = get_pixelmap(ctx, map);
   if (!pm) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetPixelMapuiv(map)");
      return;
   }
   mapsize = pm->Size;

   if (!validate_pbo_access(ctx, &ctx->Pack, mapsize,
                            GL_INTENSITY, GL_UNSIGNED_INT, values)) {
      return;
   }

   values = (GLuint *) _mesa_map_pbo_dest(ctx, &ctx->Pack, values);
   if (!values) {
      if (_mesa_is_bufferobj(ctx->Pack.BufferObj)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glGetPixelMapuiv(PBO is mapped)");
      }
      return;
   }

   if (map == GL_PIXEL_MAP_S_TO_S) {
      /* special case */
      memcpy(values, ctx->PixelMaps.StoS.Map, mapsize * sizeof(GLint));
   }
   else {
      for (i = 0; i < mapsize; i++) {
         values[i] = FLOAT_TO_UINT( pm->Map[i] );
      }
   }

   _mesa_unmap_pbo_dest(ctx, &ctx->Pack);
}


static void GLAPIENTRY
_mesa_GetPixelMapusv( GLenum map, GLushort *values )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint mapsize, i;
   const struct gl_pixelmap *pm;

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   pm = get_pixelmap(ctx, map);
   if (!pm) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetPixelMapusv(map)");
      return;
   }
   mapsize = pm->Size;

   if (!validate_pbo_access(ctx, &ctx->Pack, mapsize,
                            GL_INTENSITY, GL_UNSIGNED_SHORT, values)) {
      return;
   }

   values = (GLushort *) _mesa_map_pbo_dest(ctx, &ctx->Pack, values);
   if (!values) {
      if (_mesa_is_bufferobj(ctx->Pack.BufferObj)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glGetPixelMapusv(PBO is mapped)");
      }
      return;
   }

   switch (map) {
   /* special cases */
   case GL_PIXEL_MAP_I_TO_I:
      for (i = 0; i < mapsize; i++) {
         values[i] = (GLushort) CLAMP(ctx->PixelMaps.ItoI.Map[i], 0.0, 65535.);
      }
      break;
   case GL_PIXEL_MAP_S_TO_S:
      for (i = 0; i < mapsize; i++) {
         values[i] = (GLushort) CLAMP(ctx->PixelMaps.StoS.Map[i], 0.0, 65535.);
      }
      break;
   default:
      for (i = 0; i < mapsize; i++) {
         CLAMPED_FLOAT_TO_USHORT(values[i], pm->Map[i] );
      }
   }

   _mesa_unmap_pbo_dest(ctx, &ctx->Pack);
}



/**********************************************************************/
/*****                       glPixelTransfer                      *****/
/**********************************************************************/


/*
 * Implements glPixelTransfer[fi] whether called immediately or from a
 * display list.
 */
static void GLAPIENTRY
_mesa_PixelTransferf( GLenum pname, GLfloat param )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (pname) {
      case GL_MAP_COLOR:
         if (ctx->Pixel.MapColorFlag == (param ? GL_TRUE : GL_FALSE))
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.MapColorFlag = param ? GL_TRUE : GL_FALSE;
	 break;
      case GL_MAP_STENCIL:
         if (ctx->Pixel.MapStencilFlag == (param ? GL_TRUE : GL_FALSE))
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
         if (ctx->Pixel.PostConvolutionScale[3] == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.PostConvolutionScale[3] = param;
	 break;
      case GL_POST_CONVOLUTION_ALPHA_BIAS:
         if (ctx->Pixel.PostConvolutionBias[3] == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.PostConvolutionBias[3] = param;
	 break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glPixelTransfer(pname)" );
         return;
   }
}


static void GLAPIENTRY
_mesa_PixelTransferi( GLenum pname, GLint param )
{
   _mesa_PixelTransferf( pname, (GLfloat) param );
}



/**********************************************************************/
/*****                    State Management                        *****/
/**********************************************************************/

/*
 * Return a bitmask of IMAGE_*_BIT flags which to indicate which
 * pixel transfer operations are enabled.
 */
static void
update_image_transfer_state(GLcontext *ctx)
{
   GLuint mask = 0;

   if (ctx->Pixel.RedScale   != 1.0F || ctx->Pixel.RedBias   != 0.0F ||
       ctx->Pixel.GreenScale != 1.0F || ctx->Pixel.GreenBias != 0.0F ||
       ctx->Pixel.BlueScale  != 1.0F || ctx->Pixel.BlueBias  != 0.0F ||
       ctx->Pixel.AlphaScale != 1.0F || ctx->Pixel.AlphaBias != 0.0F)
      mask |= IMAGE_SCALE_BIAS_BIT;

   if (ctx->Pixel.IndexShift || ctx->Pixel.IndexOffset)
      mask |= IMAGE_SHIFT_OFFSET_BIT;

   if (ctx->Pixel.MapColorFlag)
      mask |= IMAGE_MAP_COLOR_BIT;

   if (ctx->Pixel.ColorTableEnabled[COLORTABLE_PRECONVOLUTION])
      mask |= IMAGE_COLOR_TABLE_BIT;

   if (ctx->Pixel.Convolution1DEnabled ||
       ctx->Pixel.Convolution2DEnabled ||
       ctx->Pixel.Separable2DEnabled) {
      mask |= IMAGE_CONVOLUTION_BIT;
      if (ctx->Pixel.PostConvolutionScale[0] != 1.0F ||
          ctx->Pixel.PostConvolutionScale[1] != 1.0F ||
          ctx->Pixel.PostConvolutionScale[2] != 1.0F ||
          ctx->Pixel.PostConvolutionScale[3] != 1.0F ||
          ctx->Pixel.PostConvolutionBias[0] != 0.0F ||
          ctx->Pixel.PostConvolutionBias[1] != 0.0F ||
          ctx->Pixel.PostConvolutionBias[2] != 0.0F ||
          ctx->Pixel.PostConvolutionBias[3] != 0.0F) {
         mask |= IMAGE_POST_CONVOLUTION_SCALE_BIAS;
      }
   }

   if (ctx->Pixel.ColorTableEnabled[COLORTABLE_POSTCONVOLUTION])
      mask |= IMAGE_POST_CONVOLUTION_COLOR_TABLE_BIT;

   if (ctx->ColorMatrixStack.Top->type != MATRIX_IDENTITY ||
       ctx->Pixel.PostColorMatrixScale[0] != 1.0F ||
       ctx->Pixel.PostColorMatrixBias[0]  != 0.0F ||
       ctx->Pixel.PostColorMatrixScale[1] != 1.0F ||
       ctx->Pixel.PostColorMatrixBias[1]  != 0.0F ||
       ctx->Pixel.PostColorMatrixScale[2] != 1.0F ||
       ctx->Pixel.PostColorMatrixBias[2]  != 0.0F ||
       ctx->Pixel.PostColorMatrixScale[3] != 1.0F ||
       ctx->Pixel.PostColorMatrixBias[3]  != 0.0F)
      mask |= IMAGE_COLOR_MATRIX_BIT;

   if (ctx->Pixel.ColorTableEnabled[COLORTABLE_POSTCOLORMATRIX])
      mask |= IMAGE_POST_COLOR_MATRIX_COLOR_TABLE_BIT;

   if (ctx->Pixel.HistogramEnabled)
      mask |= IMAGE_HISTOGRAM_BIT;

   if (ctx->Pixel.MinMaxEnabled)
      mask |= IMAGE_MIN_MAX_BIT;

   ctx->_ImageTransferState = mask;
}


/**
 * Update mesa pixel transfer derived state.
 */
void _mesa_update_pixel( GLcontext *ctx, GLuint new_state )
{
   if (new_state & _NEW_COLOR_MATRIX)
      _math_matrix_analyse( ctx->ColorMatrixStack.Top );

   /* References ColorMatrix.type (derived above).
    */
   if (new_state & _MESA_NEW_TRANSFER_STATE)
      update_image_transfer_state(ctx);
}


void
_mesa_init_pixel_dispatch(struct _glapi_table *disp)
{
   SET_GetPixelMapfv(disp, _mesa_GetPixelMapfv);
   SET_GetPixelMapuiv(disp, _mesa_GetPixelMapuiv);
   SET_GetPixelMapusv(disp, _mesa_GetPixelMapusv);
   SET_PixelMapfv(disp, _mesa_PixelMapfv);
   SET_PixelMapuiv(disp, _mesa_PixelMapuiv);
   SET_PixelMapusv(disp, _mesa_PixelMapusv);
   SET_PixelTransferf(disp, _mesa_PixelTransferf);
   SET_PixelTransferi(disp, _mesa_PixelTransferi);
   SET_PixelZoom(disp, _mesa_PixelZoom);
}


#endif /* FEATURE_pixel_transfer */


/**********************************************************************/
/*****                      Initialization                        *****/
/**********************************************************************/

static void
init_pixelmap(struct gl_pixelmap *map)
{
   map->Size = 1;
   map->Map[0] = 0.0;
   map->Map8[0] = 0;
}


/**
 * Initialize the context's PIXEL attribute group.
 */
void
_mesa_init_pixel( GLcontext *ctx )
{
   int i;

   /* Pixel group */
   ctx->Pixel.RedBias = 0.0;
   ctx->Pixel.RedScale = 1.0;
   ctx->Pixel.GreenBias = 0.0;
   ctx->Pixel.GreenScale = 1.0;
   ctx->Pixel.BlueBias = 0.0;
   ctx->Pixel.BlueScale = 1.0;
   ctx->Pixel.AlphaBias = 0.0;
   ctx->Pixel.AlphaScale = 1.0;
   ctx->Pixel.DepthBias = 0.0;
   ctx->Pixel.DepthScale = 1.0;
   ctx->Pixel.IndexOffset = 0;
   ctx->Pixel.IndexShift = 0;
   ctx->Pixel.ZoomX = 1.0;
   ctx->Pixel.ZoomY = 1.0;
   ctx->Pixel.MapColorFlag = GL_FALSE;
   ctx->Pixel.MapStencilFlag = GL_FALSE;
   init_pixelmap(&ctx->PixelMaps.StoS);
   init_pixelmap(&ctx->PixelMaps.ItoI);
   init_pixelmap(&ctx->PixelMaps.ItoR);
   init_pixelmap(&ctx->PixelMaps.ItoG);
   init_pixelmap(&ctx->PixelMaps.ItoB);
   init_pixelmap(&ctx->PixelMaps.ItoA);
   init_pixelmap(&ctx->PixelMaps.RtoR);
   init_pixelmap(&ctx->PixelMaps.GtoG);
   init_pixelmap(&ctx->PixelMaps.BtoB);
   init_pixelmap(&ctx->PixelMaps.AtoA);
   ctx->Pixel.HistogramEnabled = GL_FALSE;
   ctx->Pixel.MinMaxEnabled = GL_FALSE;
   ASSIGN_4V(ctx->Pixel.PostColorMatrixScale, 1.0, 1.0, 1.0, 1.0);
   ASSIGN_4V(ctx->Pixel.PostColorMatrixBias, 0.0, 0.0, 0.0, 0.0);
   for (i = 0; i < COLORTABLE_MAX; i++) {
      ASSIGN_4V(ctx->Pixel.ColorTableScale[i], 1.0, 1.0, 1.0, 1.0);
      ASSIGN_4V(ctx->Pixel.ColorTableBias[i], 0.0, 0.0, 0.0, 0.0);
      ctx->Pixel.ColorTableEnabled[i] = GL_FALSE;
   }
   ctx->Pixel.Convolution1DEnabled = GL_FALSE;
   ctx->Pixel.Convolution2DEnabled = GL_FALSE;
   ctx->Pixel.Separable2DEnabled = GL_FALSE;
   for (i = 0; i < 3; i++) {
      ASSIGN_4V(ctx->Pixel.ConvolutionBorderColor[i], 0.0, 0.0, 0.0, 0.0);
      ctx->Pixel.ConvolutionBorderMode[i] = GL_REDUCE;
      ASSIGN_4V(ctx->Pixel.ConvolutionFilterScale[i], 1.0, 1.0, 1.0, 1.0);
      ASSIGN_4V(ctx->Pixel.ConvolutionFilterBias[i], 0.0, 0.0, 0.0, 0.0);
   }
   for (i = 0; i < MAX_CONVOLUTION_WIDTH * MAX_CONVOLUTION_WIDTH * 4; i++) {
      ctx->Convolution1D.Filter[i] = 0.0;
      ctx->Convolution2D.Filter[i] = 0.0;
      ctx->Separable2D.Filter[i] = 0.0;
   }
   ASSIGN_4V(ctx->Pixel.PostConvolutionScale, 1.0, 1.0, 1.0, 1.0);
   ASSIGN_4V(ctx->Pixel.PostConvolutionBias, 0.0, 0.0, 0.0, 0.0);
   /* GL_SGI_texture_color_table */
   ASSIGN_4V(ctx->Pixel.TextureColorTableScale, 1.0, 1.0, 1.0, 1.0);
   ASSIGN_4V(ctx->Pixel.TextureColorTableBias, 0.0, 0.0, 0.0, 0.0);

   if (ctx->Visual.doubleBufferMode) {
      ctx->Pixel.ReadBuffer = GL_BACK;
   }
   else {
      ctx->Pixel.ReadBuffer = GL_FRONT;
   }

   /* Miscellaneous */
   ctx->_ImageTransferState = 0;
}

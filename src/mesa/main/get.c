/* $Id: get.c,v 1.2 1999/09/09 23:47:09 brianp Exp $ */

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


#ifdef PC_HEADER
#include "all.h"
#else
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "context.h"
#include "enable.h"
#include "enums.h"
#include "extensions.h"
#include "get.h"
#include "macros.h"
#include "mmath.h"
#include "types.h"
#include "vb.h"
#ifdef XFree86Server
#include "GL/xf86glx.h"
#endif
#endif



#define FLOAT_TO_BOOL(X)	( (X)==0.0F ? GL_FALSE : GL_TRUE )
#define INT_TO_BOOL(I)		( (I)==0 ? GL_FALSE : GL_TRUE )
#define ENUM_TO_BOOL(E)		( (E)==0 ? GL_FALSE : GL_TRUE )

#ifdef SPECIALCAST
/* Needed for an Amiga compiler */
#define ENUM_TO_FLOAT(X) ((GLfloat)(GLint)(X))
#define ENUM_TO_DOUBLE(X) ((GLdouble)(GLint)(X))
#else
/* all other compilers */
#define ENUM_TO_FLOAT(X) ((GLfloat)(X))
#define ENUM_TO_DOUBLE(X) ((GLdouble)(X))
#endif



void gl_GetBooleanv( GLcontext *ctx, GLenum pname, GLboolean *params )
{
   GLuint i;
   GLuint texUnit = ctx->Texture.CurrentUnit;
   GLuint texTransformUnit = ctx->Texture.CurrentTransformUnit;
   const struct gl_texture_unit *textureUnit = &ctx->Texture.Unit[texUnit];

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glGetBooleanv");

   if (MESA_VERBOSE & VERBOSE_API) 
      fprintf(stderr, "glGetBooleanv %s\n", gl_lookup_enum_by_nr(pname));

   switch (pname) {
      case GL_ACCUM_RED_BITS:
      case GL_ACCUM_GREEN_BITS:
      case GL_ACCUM_BLUE_BITS:
      case GL_ACCUM_ALPHA_BITS:
         *params = INT_TO_BOOL(ctx->Visual->AccumBits);
         break;
      case GL_ACCUM_CLEAR_VALUE:
         params[0] = FLOAT_TO_BOOL(ctx->Accum.ClearColor[0]);
         params[1] = FLOAT_TO_BOOL(ctx->Accum.ClearColor[1]);
         params[2] = FLOAT_TO_BOOL(ctx->Accum.ClearColor[2]);
         params[3] = FLOAT_TO_BOOL(ctx->Accum.ClearColor[3]);
         break;
      case GL_ALPHA_BIAS:
         *params = FLOAT_TO_BOOL(ctx->Pixel.AlphaBias);
         break;
      case GL_ALPHA_BITS:
         *params = INT_TO_BOOL(ctx->Visual->AlphaBits);
         break;
      case GL_ALPHA_SCALE:
         *params = FLOAT_TO_BOOL(ctx->Pixel.AlphaScale);
         break;
      case GL_ALPHA_TEST:
         *params = ctx->Color.AlphaEnabled;
         break;
      case GL_ALPHA_TEST_FUNC:
         *params = ENUM_TO_BOOL(ctx->Color.AlphaFunc);
         break;
      case GL_ALPHA_TEST_REF:
         *params = FLOAT_TO_BOOL((GLfloat) ctx->Color.AlphaRef / 255.0);
         break;
      case GL_ATTRIB_STACK_DEPTH:
         *params = INT_TO_BOOL(ctx->AttribStackDepth);
         break;
      case GL_AUTO_NORMAL:
         *params = ctx->Eval.AutoNormal;
         break;
      case GL_AUX_BUFFERS:
         *params = (NUM_AUX_BUFFERS) ? GL_TRUE : GL_FALSE;
         break;
      case GL_BLEND:
         *params = ctx->Color.BlendEnabled;
         break;
      case GL_BLEND_DST:
         *params = ENUM_TO_BOOL(ctx->Color.BlendDstRGB);
         break;
      case GL_BLEND_SRC:
         *params = ENUM_TO_BOOL(ctx->Color.BlendSrcRGB);
         break;
      case GL_BLEND_SRC_RGB_INGR:
         *params = ENUM_TO_BOOL(ctx->Color.BlendSrcRGB);
         break;
      case GL_BLEND_DST_RGB_INGR:
         *params = ENUM_TO_BOOL(ctx->Color.BlendDstRGB);
         break;
      case GL_BLEND_SRC_ALPHA_INGR:
         *params = ENUM_TO_BOOL(ctx->Color.BlendSrcA);
         break;
      case GL_BLEND_DST_ALPHA_INGR:
         *params = ENUM_TO_BOOL(ctx->Color.BlendDstA);
         break;
      case GL_BLEND_EQUATION_EXT:
	 *params = ENUM_TO_BOOL( ctx->Color.BlendEquation );
	 break;
      case GL_BLEND_COLOR_EXT:
	 params[0] = FLOAT_TO_BOOL( ctx->Color.BlendColor[0] );
	 params[1] = FLOAT_TO_BOOL( ctx->Color.BlendColor[1] );
	 params[2] = FLOAT_TO_BOOL( ctx->Color.BlendColor[2] );
	 params[3] = FLOAT_TO_BOOL( ctx->Color.BlendColor[3] );
	 break;
      case GL_BLUE_BIAS:
         *params = FLOAT_TO_BOOL(ctx->Pixel.BlueBias);
         break;
      case GL_BLUE_BITS:
         *params = INT_TO_BOOL( ctx->Visual->BlueBits );
         break;
      case GL_BLUE_SCALE:
         *params = FLOAT_TO_BOOL(ctx->Pixel.BlueScale);
         break;
      case GL_CLIENT_ATTRIB_STACK_DEPTH:
         *params = INT_TO_BOOL(ctx->ClientAttribStackDepth);
         break;
      case GL_CLIP_PLANE0:
      case GL_CLIP_PLANE1:
      case GL_CLIP_PLANE2:
      case GL_CLIP_PLANE3:
      case GL_CLIP_PLANE4:
      case GL_CLIP_PLANE5:
         *params = ctx->Transform.ClipEnabled[pname-GL_CLIP_PLANE0];
         break;
      case GL_COLOR_CLEAR_VALUE:
         params[0] = FLOAT_TO_BOOL(ctx->Color.ClearColor[0]);
         params[1] = FLOAT_TO_BOOL(ctx->Color.ClearColor[1]);
         params[2] = FLOAT_TO_BOOL(ctx->Color.ClearColor[2]);
         params[3] = FLOAT_TO_BOOL(ctx->Color.ClearColor[3]);
         break;
      case GL_COLOR_MATERIAL:
         *params = ctx->Light.ColorMaterialEnabled;
         break;
      case GL_COLOR_MATERIAL_FACE:
         *params = ENUM_TO_BOOL(ctx->Light.ColorMaterialFace);
         break;
      case GL_COLOR_MATERIAL_PARAMETER:
         *params = ENUM_TO_BOOL(ctx->Light.ColorMaterialMode);
         break;
      case GL_COLOR_WRITEMASK:
         params[0] = ctx->Color.ColorMask[RCOMP] ? GL_TRUE : GL_FALSE;
         params[1] = ctx->Color.ColorMask[GCOMP] ? GL_TRUE : GL_FALSE;
         params[2] = ctx->Color.ColorMask[BCOMP] ? GL_TRUE : GL_FALSE;
         params[3] = ctx->Color.ColorMask[ACOMP] ? GL_TRUE : GL_FALSE;
         break;
      case GL_CULL_FACE:
         *params = ctx->Polygon.CullFlag;
         break;
      case GL_CULL_FACE_MODE:
         *params = ENUM_TO_BOOL(ctx->Polygon.CullFaceMode);
         break;
      case GL_CURRENT_COLOR:
         params[0] = INT_TO_BOOL(ctx->Current.ByteColor[0]);
         params[1] = INT_TO_BOOL(ctx->Current.ByteColor[1]);
         params[2] = INT_TO_BOOL(ctx->Current.ByteColor[2]);
         params[3] = INT_TO_BOOL(ctx->Current.ByteColor[3]);
         break;
      case GL_CURRENT_INDEX:
         *params = INT_TO_BOOL(ctx->Current.Index);
         break;
      case GL_CURRENT_NORMAL:
         params[0] = FLOAT_TO_BOOL(ctx->Current.Normal[0]);
         params[1] = FLOAT_TO_BOOL(ctx->Current.Normal[1]);
         params[2] = FLOAT_TO_BOOL(ctx->Current.Normal[2]);
         break;
      case GL_CURRENT_RASTER_COLOR:
	 params[0] = FLOAT_TO_BOOL(ctx->Current.RasterColor[0]);
	 params[1] = FLOAT_TO_BOOL(ctx->Current.RasterColor[1]);
	 params[2] = FLOAT_TO_BOOL(ctx->Current.RasterColor[2]);
	 params[3] = FLOAT_TO_BOOL(ctx->Current.RasterColor[3]);
	 break;
      case GL_CURRENT_RASTER_DISTANCE:
	 *params = FLOAT_TO_BOOL(ctx->Current.RasterDistance);
	 break;
      case GL_CURRENT_RASTER_INDEX:
	 *params = FLOAT_TO_BOOL(ctx->Current.RasterIndex);
	 break;
      case GL_CURRENT_RASTER_POSITION:
	 params[0] = FLOAT_TO_BOOL(ctx->Current.RasterPos[0]);
	 params[1] = FLOAT_TO_BOOL(ctx->Current.RasterPos[1]);
	 params[2] = FLOAT_TO_BOOL(ctx->Current.RasterPos[2]);
	 params[3] = FLOAT_TO_BOOL(ctx->Current.RasterPos[3]);
	 break;
      case GL_CURRENT_RASTER_TEXTURE_COORDS:
         params[0] = FLOAT_TO_BOOL(ctx->Current.RasterMultiTexCoord[texTransformUnit][0]);
         params[1] = FLOAT_TO_BOOL(ctx->Current.RasterMultiTexCoord[texTransformUnit][1]);
         params[2] = FLOAT_TO_BOOL(ctx->Current.RasterMultiTexCoord[texTransformUnit][2]);
         params[3] = FLOAT_TO_BOOL(ctx->Current.RasterMultiTexCoord[texTransformUnit][3]);
	 break;
      case GL_CURRENT_RASTER_POSITION_VALID:
         *params = ctx->Current.RasterPosValid;
	 break;
      case GL_CURRENT_TEXTURE_COORDS:
         params[0] = FLOAT_TO_BOOL(ctx->Current.Texcoord[texTransformUnit][0]);
         params[1] = FLOAT_TO_BOOL(ctx->Current.Texcoord[texTransformUnit][1]);
         params[2] = FLOAT_TO_BOOL(ctx->Current.Texcoord[texTransformUnit][2]);
         params[3] = FLOAT_TO_BOOL(ctx->Current.Texcoord[texTransformUnit][3]);
	 break;
      case GL_DEPTH_BIAS:
         *params = FLOAT_TO_BOOL(ctx->Pixel.DepthBias);
	 break;
      case GL_DEPTH_BITS:
	 *params = INT_TO_BOOL(ctx->Visual->DepthBits);
	 break;
      case GL_DEPTH_CLEAR_VALUE:
         *params = FLOAT_TO_BOOL(ctx->Depth.Clear);
	 break;
      case GL_DEPTH_FUNC:
         *params = ENUM_TO_BOOL(ctx->Depth.Func);
	 break;
      case GL_DEPTH_RANGE:
         params[0] = FLOAT_TO_BOOL(ctx->Viewport.Near);
         params[1] = FLOAT_TO_BOOL(ctx->Viewport.Far);
	 break;
      case GL_DEPTH_SCALE:
         *params = FLOAT_TO_BOOL(ctx->Pixel.DepthScale);
	 break;
      case GL_DEPTH_TEST:
         *params = ctx->Depth.Test;
	 break;
      case GL_DEPTH_WRITEMASK:
	 *params = ctx->Depth.Mask;
	 break;
      case GL_DITHER:
	 *params = ctx->Color.DitherFlag;
	 break;
      case GL_DOUBLEBUFFER:
	 *params = ctx->Visual->DBflag;
	 break;
      case GL_DRAW_BUFFER:
	 *params = ENUM_TO_BOOL(ctx->Color.DrawBuffer);
	 break;
      case GL_EDGE_FLAG:
	 *params = ctx->Current.EdgeFlag;
	 break;
      case GL_FEEDBACK_BUFFER_SIZE:
         /* TODO: is this right?  Or, return number of entries in buffer? */
         *params = INT_TO_BOOL(ctx->Feedback.BufferSize);
         break;
      case GL_FEEDBACK_BUFFER_TYPE:
         *params = INT_TO_BOOL(ctx->Feedback.Type);
         break;
      case GL_FOG:
	 *params = ctx->Fog.Enabled;
	 break;
      case GL_FOG_COLOR:
         params[0] = FLOAT_TO_BOOL(ctx->Fog.Color[0]);
         params[1] = FLOAT_TO_BOOL(ctx->Fog.Color[1]);
         params[2] = FLOAT_TO_BOOL(ctx->Fog.Color[2]);
         params[3] = FLOAT_TO_BOOL(ctx->Fog.Color[3]);
	 break;
      case GL_FOG_DENSITY:
         *params = FLOAT_TO_BOOL(ctx->Fog.Density);
	 break;
      case GL_FOG_END:
         *params = FLOAT_TO_BOOL(ctx->Fog.End);
	 break;
      case GL_FOG_HINT:
	 *params = ENUM_TO_BOOL(ctx->Hint.Fog);
	 break;
      case GL_FOG_INDEX:
	 *params = FLOAT_TO_BOOL(ctx->Fog.Index);
	 break;
      case GL_FOG_MODE:
	 *params = ENUM_TO_BOOL(ctx->Fog.Mode);
	 break;
      case GL_FOG_START:
         *params = FLOAT_TO_BOOL(ctx->Fog.End);
	 break;
      case GL_FRONT_FACE:
	 *params = ENUM_TO_BOOL(ctx->Polygon.FrontFace);
	 break;
      case GL_GREEN_BIAS:
         *params = FLOAT_TO_BOOL(ctx->Pixel.GreenBias);
	 break;
      case GL_GREEN_BITS:
         *params = INT_TO_BOOL( ctx->Visual->GreenBits );
	 break;
      case GL_GREEN_SCALE:
         *params = FLOAT_TO_BOOL(ctx->Pixel.GreenScale);
	 break;
      case GL_INDEX_BITS:
         *params = INT_TO_BOOL( ctx->Visual->IndexBits );
	 break;
      case GL_INDEX_CLEAR_VALUE:
	 *params = INT_TO_BOOL(ctx->Color.ClearIndex);
	 break;
      case GL_INDEX_MODE:
	 *params = ctx->Visual->RGBAflag ? GL_FALSE : GL_TRUE;
	 break;
      case GL_INDEX_OFFSET:
	 *params = INT_TO_BOOL(ctx->Pixel.IndexOffset);
	 break;
      case GL_INDEX_SHIFT:
	 *params = INT_TO_BOOL(ctx->Pixel.IndexShift);
	 break;
      case GL_INDEX_WRITEMASK:
	 *params = INT_TO_BOOL(ctx->Color.IndexMask);
	 break;
      case GL_LIGHT0:
      case GL_LIGHT1:
      case GL_LIGHT2:
      case GL_LIGHT3:
      case GL_LIGHT4:
      case GL_LIGHT5:
      case GL_LIGHT6:
      case GL_LIGHT7:
	 *params = ctx->Light.Light[pname-GL_LIGHT0].Enabled;
	 break;
      case GL_LIGHTING:
	 *params = ctx->Light.Enabled;
	 break;
      case GL_LIGHT_MODEL_AMBIENT:
	 params[0] = FLOAT_TO_BOOL(ctx->Light.Model.Ambient[0]);
	 params[1] = FLOAT_TO_BOOL(ctx->Light.Model.Ambient[1]);
	 params[2] = FLOAT_TO_BOOL(ctx->Light.Model.Ambient[2]);
	 params[3] = FLOAT_TO_BOOL(ctx->Light.Model.Ambient[3]);
	 break;
      case GL_LIGHT_MODEL_COLOR_CONTROL:
         params[0] = ENUM_TO_BOOL(ctx->Light.Model.ColorControl);
         break;
      case GL_LIGHT_MODEL_LOCAL_VIEWER:
	 *params = ctx->Light.Model.LocalViewer;
	 break;
      case GL_LIGHT_MODEL_TWO_SIDE:
	 *params = ctx->Light.Model.TwoSide;
	 break;
      case GL_LINE_SMOOTH:
	 *params = ctx->Line.SmoothFlag;
	 break;
      case GL_LINE_SMOOTH_HINT:
	 *params = ENUM_TO_BOOL(ctx->Hint.LineSmooth);
	 break;
      case GL_LINE_STIPPLE:
	 *params = ctx->Line.StippleFlag;
	 break;
      case GL_LINE_STIPPLE_PATTERN:
	 *params = INT_TO_BOOL(ctx->Line.StipplePattern);
	 break;
      case GL_LINE_STIPPLE_REPEAT:
	 *params = INT_TO_BOOL(ctx->Line.StippleFactor);
	 break;
      case GL_LINE_WIDTH:
	 *params = FLOAT_TO_BOOL(ctx->Line.Width);
	 break;
      case GL_LINE_WIDTH_GRANULARITY:
	 *params = FLOAT_TO_BOOL(LINE_WIDTH_GRANULARITY);
	 break;
      case GL_LINE_WIDTH_RANGE:
	 params[0] = FLOAT_TO_BOOL(MIN_LINE_WIDTH);
	 params[1] = FLOAT_TO_BOOL(MAX_LINE_WIDTH);
	 break;
      case GL_LIST_BASE:
	 *params = INT_TO_BOOL(ctx->List.ListBase);
	 break;
      case GL_LIST_INDEX:
	 *params = INT_TO_BOOL( ctx->CurrentListNum );
	 break;
      case GL_LIST_MODE:
	 *params = ENUM_TO_BOOL( ctx->ExecuteFlag
				  ? GL_COMPILE_AND_EXECUTE : GL_COMPILE );
	 break;
      case GL_INDEX_LOGIC_OP:
	 *params = ctx->Color.IndexLogicOpEnabled;
	 break;
      case GL_COLOR_LOGIC_OP:
	 *params = ctx->Color.ColorLogicOpEnabled;
	 break;
      case GL_LOGIC_OP_MODE:
	 *params = ENUM_TO_BOOL(ctx->Color.LogicOp);
	 break;
      case GL_MAP1_COLOR_4:
	 *params = ctx->Eval.Map1Color4;
	 break;
      case GL_MAP1_GRID_DOMAIN:
	 params[0] = FLOAT_TO_BOOL(ctx->Eval.MapGrid1u1);
	 params[1] = FLOAT_TO_BOOL(ctx->Eval.MapGrid1u2);
	 break;
      case GL_MAP1_GRID_SEGMENTS:
	 *params = INT_TO_BOOL(ctx->Eval.MapGrid1un);
	 break;
      case GL_MAP1_INDEX:
	 *params = ctx->Eval.Map1Index;
	 break;
      case GL_MAP1_NORMAL:
	 *params = ctx->Eval.Map1Normal;
	 break;
      case GL_MAP1_TEXTURE_COORD_1:
	 *params = ctx->Eval.Map1TextureCoord1;
	 break;
      case GL_MAP1_TEXTURE_COORD_2:
	 *params = ctx->Eval.Map1TextureCoord2;
	 break;
      case GL_MAP1_TEXTURE_COORD_3:
	 *params = ctx->Eval.Map1TextureCoord3;
	 break;
      case GL_MAP1_TEXTURE_COORD_4:
	 *params = ctx->Eval.Map1TextureCoord4;
	 break;
      case GL_MAP1_VERTEX_3:
	 *params = ctx->Eval.Map1Vertex3;
	 break;
      case GL_MAP1_VERTEX_4:
	 *params = ctx->Eval.Map1Vertex4;
	 break;
      case GL_MAP2_COLOR_4:
	 *params = ctx->Eval.Map2Color4;
	 break;
      case GL_MAP2_GRID_DOMAIN:
	 params[0] = FLOAT_TO_BOOL(ctx->Eval.MapGrid2u1);
	 params[1] = FLOAT_TO_BOOL(ctx->Eval.MapGrid2u2);
	 params[2] = FLOAT_TO_BOOL(ctx->Eval.MapGrid2v1);
	 params[3] = FLOAT_TO_BOOL(ctx->Eval.MapGrid2v2);
	 break;
      case GL_MAP2_GRID_SEGMENTS:
	 params[0] = INT_TO_BOOL(ctx->Eval.MapGrid2un);
	 params[1] = INT_TO_BOOL(ctx->Eval.MapGrid2vn);
	 break;
      case GL_MAP2_INDEX:
	 *params = ctx->Eval.Map2Index;
	 break;
      case GL_MAP2_NORMAL:
	 *params = ctx->Eval.Map2Normal;
	 break;
      case GL_MAP2_TEXTURE_COORD_1:
	 *params = ctx->Eval.Map2TextureCoord1;
	 break;
      case GL_MAP2_TEXTURE_COORD_2:
	 *params = ctx->Eval.Map2TextureCoord2;
	 break;
      case GL_MAP2_TEXTURE_COORD_3:
	 *params = ctx->Eval.Map2TextureCoord3;
	 break;
      case GL_MAP2_TEXTURE_COORD_4:
	 *params = ctx->Eval.Map2TextureCoord4;
	 break;
      case GL_MAP2_VERTEX_3:
	 *params = ctx->Eval.Map2Vertex3;
	 break;
      case GL_MAP2_VERTEX_4:
	 *params = ctx->Eval.Map2Vertex4;
	 break;
      case GL_MAP_COLOR:
	 *params = ctx->Pixel.MapColorFlag;
	 break;
      case GL_MAP_STENCIL:
	 *params = ctx->Pixel.MapStencilFlag;
	 break;
      case GL_MATRIX_MODE:
	 *params = ENUM_TO_BOOL( ctx->Transform.MatrixMode );
	 break;
      case GL_MAX_ATTRIB_STACK_DEPTH:
	 *params = INT_TO_BOOL(MAX_ATTRIB_STACK_DEPTH);
	 break;
      case GL_MAX_CLIENT_ATTRIB_STACK_DEPTH:
         *params = INT_TO_BOOL( MAX_CLIENT_ATTRIB_STACK_DEPTH);
         break;
      case GL_MAX_CLIP_PLANES:
	 *params = INT_TO_BOOL(MAX_CLIP_PLANES);
	 break;
      case GL_MAX_ELEMENTS_VERTICES:  /* GL_VERSION_1_2 */
         *params = INT_TO_BOOL(VB_MAX);
         break;
      case GL_MAX_ELEMENTS_INDICES:   /* GL_VERSION_1_2 */
         *params = INT_TO_BOOL(VB_MAX);
         break;
      case GL_MAX_EVAL_ORDER:
	 *params = INT_TO_BOOL(MAX_EVAL_ORDER);
	 break;
      case GL_MAX_LIGHTS:
	 *params = INT_TO_BOOL(MAX_LIGHTS);
	 break;
      case GL_MAX_LIST_NESTING:
	 *params = INT_TO_BOOL(MAX_LIST_NESTING);
	 break;
      case GL_MAX_MODELVIEW_STACK_DEPTH:
	 *params = INT_TO_BOOL(MAX_MODELVIEW_STACK_DEPTH);
	 break;
      case GL_MAX_NAME_STACK_DEPTH:
	 *params = INT_TO_BOOL(MAX_NAME_STACK_DEPTH);
	 break;
      case GL_MAX_PIXEL_MAP_TABLE:
	 *params = INT_TO_BOOL(MAX_PIXEL_MAP_TABLE);
	 break;
      case GL_MAX_PROJECTION_STACK_DEPTH:
	 *params = INT_TO_BOOL(MAX_PROJECTION_STACK_DEPTH);
	 break;
      case GL_MAX_TEXTURE_SIZE:
      case GL_MAX_3D_TEXTURE_SIZE:
         *params = INT_TO_BOOL(ctx->Const.MaxTextureSize);
	 break;
      case GL_MAX_TEXTURE_STACK_DEPTH:
	 *params = INT_TO_BOOL(MAX_TEXTURE_STACK_DEPTH);
	 break;
      case GL_MAX_VIEWPORT_DIMS:
	 params[0] = INT_TO_BOOL(MAX_WIDTH);
	 params[1] = INT_TO_BOOL(MAX_HEIGHT);
	 break;
      case GL_MODELVIEW_MATRIX:
	 for (i=0;i<16;i++) {
	    params[i] = FLOAT_TO_BOOL(ctx->ModelView.m[i]);
	 }
	 break;
      case GL_MODELVIEW_STACK_DEPTH:
	 *params = INT_TO_BOOL(ctx->ModelViewStackDepth + 1);
	 break;
      case GL_NAME_STACK_DEPTH:
	 *params = INT_TO_BOOL(ctx->Select.NameStackDepth);
	 break;
      case GL_NORMALIZE:
	 *params = ctx->Transform.Normalize;
	 break;
      case GL_PACK_ALIGNMENT:
	 *params = INT_TO_BOOL(ctx->Pack.Alignment);
	 break;
      case GL_PACK_LSB_FIRST:
	 *params = ctx->Pack.LsbFirst;
	 break;
      case GL_PACK_ROW_LENGTH:
	 *params = INT_TO_BOOL(ctx->Pack.RowLength);
	 break;
      case GL_PACK_SKIP_PIXELS:
	 *params = INT_TO_BOOL(ctx->Pack.SkipPixels);
	 break;
      case GL_PACK_SKIP_ROWS:
	 *params = INT_TO_BOOL(ctx->Pack.SkipRows);
	 break;
      case GL_PACK_SWAP_BYTES:
	 *params = ctx->Pack.SwapBytes;
	 break;
      case GL_PACK_SKIP_IMAGES_EXT:
         *params = ctx->Pack.SkipImages;
         break;
      case GL_PACK_IMAGE_HEIGHT_EXT:
         *params = ctx->Pack.ImageHeight;
         break;
      case GL_PERSPECTIVE_CORRECTION_HINT:
	 *params = ENUM_TO_BOOL(ctx->Hint.PerspectiveCorrection);
	 break;
      case GL_PIXEL_MAP_A_TO_A_SIZE:
	 *params = INT_TO_BOOL(ctx->Pixel.MapAtoAsize);
	 break;
      case GL_PIXEL_MAP_B_TO_B_SIZE:
	 *params = INT_TO_BOOL(ctx->Pixel.MapBtoBsize);
	 break;
      case GL_PIXEL_MAP_G_TO_G_SIZE:
	 *params = INT_TO_BOOL(ctx->Pixel.MapGtoGsize);
	 break;
      case GL_PIXEL_MAP_I_TO_A_SIZE:
	 *params = INT_TO_BOOL(ctx->Pixel.MapItoAsize);
	 break;
      case GL_PIXEL_MAP_I_TO_B_SIZE:
	 *params = INT_TO_BOOL(ctx->Pixel.MapItoBsize);
	 break;
      case GL_PIXEL_MAP_I_TO_G_SIZE:
	 *params = INT_TO_BOOL(ctx->Pixel.MapItoGsize);
	 break;
      case GL_PIXEL_MAP_I_TO_I_SIZE:
	 *params = INT_TO_BOOL(ctx->Pixel.MapItoIsize);
	 break;
      case GL_PIXEL_MAP_I_TO_R_SIZE:
	 *params = INT_TO_BOOL(ctx->Pixel.MapItoRsize);
	 break;
      case GL_PIXEL_MAP_R_TO_R_SIZE:
	 *params = INT_TO_BOOL(ctx->Pixel.MapRtoRsize);
	 break;
      case GL_PIXEL_MAP_S_TO_S_SIZE:
	 *params = INT_TO_BOOL(ctx->Pixel.MapStoSsize);
	 break;
      case GL_POINT_SIZE:
	 *params = FLOAT_TO_BOOL(ctx->Point.Size );
	 break;
      case GL_POINT_SIZE_GRANULARITY:
	 *params = FLOAT_TO_BOOL(POINT_SIZE_GRANULARITY );
	 break;
      case GL_POINT_SIZE_RANGE:
	 params[0] = FLOAT_TO_BOOL(MIN_POINT_SIZE );
	 params[1] = FLOAT_TO_BOOL(MAX_POINT_SIZE );
	 break;
      case GL_POINT_SMOOTH:
	 *params = ctx->Point.SmoothFlag;
	 break;
      case GL_POINT_SMOOTH_HINT:
	 *params = ENUM_TO_BOOL(ctx->Hint.PointSmooth);
	 break;
      case GL_POINT_SIZE_MIN_EXT:
	 *params = FLOAT_TO_BOOL(ctx->Point.MinSize);
	 break;
      case GL_POINT_SIZE_MAX_EXT:
	 *params = FLOAT_TO_BOOL(ctx->Point.MaxSize);
	 break;
      case GL_POINT_FADE_THRESHOLD_SIZE_EXT:
	 *params = FLOAT_TO_BOOL(ctx->Point.Threshold);
	 break;
      case GL_DISTANCE_ATTENUATION_EXT:
	 params[0] = FLOAT_TO_BOOL(ctx->Point.Params[0]);
	 params[1] = FLOAT_TO_BOOL(ctx->Point.Params[1]);
	 params[2] = FLOAT_TO_BOOL(ctx->Point.Params[2]);
	 break;
      case GL_POLYGON_MODE:
	 params[0] = ENUM_TO_BOOL(ctx->Polygon.FrontMode);
	 params[1] = ENUM_TO_BOOL(ctx->Polygon.BackMode);
	 break;
#ifdef GL_EXT_polygon_offset
      case GL_POLYGON_OFFSET_BIAS_EXT:
         *params = FLOAT_TO_BOOL( ctx->Polygon.OffsetUnits );
         break;
#endif
      case GL_POLYGON_OFFSET_FACTOR:
         *params = FLOAT_TO_BOOL( ctx->Polygon.OffsetFactor );
         break;
      case GL_POLYGON_OFFSET_UNITS:
         *params = FLOAT_TO_BOOL( ctx->Polygon.OffsetUnits );
         break;
      case GL_POLYGON_SMOOTH:
	 *params = ctx->Polygon.SmoothFlag;
	 break;
      case GL_POLYGON_SMOOTH_HINT:
	 *params = ENUM_TO_BOOL(ctx->Hint.PolygonSmooth);
	 break;
      case GL_POLYGON_STIPPLE:
	 *params = ctx->Polygon.StippleFlag;
	 break;
      case GL_PROJECTION_MATRIX:
	 for (i=0;i<16;i++) {
	    params[i] = FLOAT_TO_BOOL(ctx->ProjectionMatrix.m[i]);
	 }
	 break;
      case GL_PROJECTION_STACK_DEPTH:
	 *params = INT_TO_BOOL(ctx->ProjectionStackDepth + 1);
	 break;
      case GL_READ_BUFFER:
	 *params = ENUM_TO_BOOL(ctx->Pixel.ReadBuffer);
	 break;
      case GL_RED_BIAS:
         *params = FLOAT_TO_BOOL(ctx->Pixel.RedBias);
	 break;
      case GL_RED_BITS:
         *params = INT_TO_BOOL( ctx->Visual->RedBits );
	 break;
      case GL_RED_SCALE:
         *params = FLOAT_TO_BOOL(ctx->Pixel.RedScale);
	 break;
      case GL_RENDER_MODE:
	 *params = ENUM_TO_BOOL(ctx->RenderMode);
	 break;
      case GL_RGBA_MODE:
         *params = ctx->Visual->RGBAflag;
	 break;
      case GL_SCISSOR_BOX:
	 params[0] = INT_TO_BOOL(ctx->Scissor.X);
	 params[1] = INT_TO_BOOL(ctx->Scissor.Y);
	 params[2] = INT_TO_BOOL(ctx->Scissor.Width);
	 params[3] = INT_TO_BOOL(ctx->Scissor.Height);
	 break;
      case GL_SCISSOR_TEST:
	 *params = ctx->Scissor.Enabled;
	 break;
      case GL_SELECTION_BUFFER_SIZE:
         *params = INT_TO_BOOL(ctx->Select.BufferSize);
         break;
      case GL_SHADE_MODEL:
	 *params = ENUM_TO_BOOL(ctx->Light.ShadeModel);
	 break;
      case GL_SHARED_TEXTURE_PALETTE_EXT:
         *params = ctx->Texture.SharedPalette;
         break;
      case GL_STENCIL_BITS:
	 *params = INT_TO_BOOL(ctx->Visual->StencilBits);
	 break;
      case GL_STENCIL_CLEAR_VALUE:
	 *params = INT_TO_BOOL(ctx->Stencil.Clear);
	 break;
      case GL_STENCIL_FAIL:
	 *params = ENUM_TO_BOOL(ctx->Stencil.FailFunc);
	 break;
      case GL_STENCIL_FUNC:
	 *params = ENUM_TO_BOOL(ctx->Stencil.Function);
	 break;
      case GL_STENCIL_PASS_DEPTH_FAIL:
	 *params = ENUM_TO_BOOL(ctx->Stencil.ZFailFunc);
	 break;
      case GL_STENCIL_PASS_DEPTH_PASS:
	 *params = ENUM_TO_BOOL(ctx->Stencil.ZPassFunc);
	 break;
      case GL_STENCIL_REF:
	 *params = INT_TO_BOOL(ctx->Stencil.Ref);
	 break;
      case GL_STENCIL_TEST:
	 *params = ctx->Stencil.Enabled;
	 break;
      case GL_STENCIL_VALUE_MASK:
	 *params = INT_TO_BOOL(ctx->Stencil.ValueMask);
	 break;
      case GL_STENCIL_WRITEMASK:
	 *params = INT_TO_BOOL(ctx->Stencil.WriteMask);
	 break;
      case GL_STEREO:
	 *params = ctx->Visual->StereoFlag;
	 break;
      case GL_SUBPIXEL_BITS:
	 *params = INT_TO_BOOL(0);  /* TODO */
	 break;
      case GL_TEXTURE_1D:
         *params = gl_IsEnabled( ctx, GL_TEXTURE_1D );
	 break;
      case GL_TEXTURE_2D:
         *params = gl_IsEnabled( ctx, GL_TEXTURE_2D );
	 break;
      case GL_TEXTURE_3D:
         *params = gl_IsEnabled( ctx, GL_TEXTURE_3D );
	 break;
      case GL_TEXTURE_BINDING_1D:
         *params = INT_TO_BOOL(textureUnit->CurrentD[1]->Name);
          break;
      case GL_TEXTURE_BINDING_2D:
         *params = INT_TO_BOOL(textureUnit->CurrentD[2]->Name);
          break;
      case GL_TEXTURE_BINDING_3D:
         *params = INT_TO_BOOL(textureUnit->CurrentD[3]->Name);
          break;
      case GL_TEXTURE_ENV_COLOR:
         {
            params[0] = FLOAT_TO_BOOL(textureUnit->EnvColor[0]);
            params[1] = FLOAT_TO_BOOL(textureUnit->EnvColor[1]);
            params[2] = FLOAT_TO_BOOL(textureUnit->EnvColor[2]);
            params[3] = FLOAT_TO_BOOL(textureUnit->EnvColor[3]);
         }
	 break;
      case GL_TEXTURE_ENV_MODE:
	 *params = ENUM_TO_BOOL(textureUnit->EnvMode);
	 break;
      case GL_TEXTURE_GEN_S:
	 *params = (textureUnit->TexGenEnabled & S_BIT) ? GL_TRUE : GL_FALSE;
	 break;
      case GL_TEXTURE_GEN_T:
	 *params = (textureUnit->TexGenEnabled & T_BIT) ? GL_TRUE : GL_FALSE;
	 break;
      case GL_TEXTURE_GEN_R:
	 *params = (textureUnit->TexGenEnabled & R_BIT) ? GL_TRUE : GL_FALSE;
	 break;
      case GL_TEXTURE_GEN_Q:
	 *params = (textureUnit->TexGenEnabled & Q_BIT) ? GL_TRUE : GL_FALSE;
	 break;
      case GL_TEXTURE_MATRIX:
	 for (i=0;i<16;i++) {
	    params[i] = 
	       FLOAT_TO_BOOL(ctx->TextureMatrix[texTransformUnit].m[i]);
	 }
	 break;
      case GL_TEXTURE_STACK_DEPTH:
	 *params = INT_TO_BOOL(ctx->TextureStackDepth[texTransformUnit] + 1);
	 break;
      case GL_UNPACK_ALIGNMENT:
	 *params = INT_TO_BOOL(ctx->Unpack.Alignment);
	 break;
      case GL_UNPACK_LSB_FIRST:
	 *params = ctx->Unpack.LsbFirst;
	 break;
      case GL_UNPACK_ROW_LENGTH:
	 *params = INT_TO_BOOL(ctx->Unpack.RowLength);
	 break;
      case GL_UNPACK_SKIP_PIXELS:
	 *params = INT_TO_BOOL(ctx->Unpack.SkipPixels);
	 break;
      case GL_UNPACK_SKIP_ROWS:
	 *params = INT_TO_BOOL(ctx->Unpack.SkipRows);
	 break;
      case GL_UNPACK_SWAP_BYTES:
	 *params = ctx->Unpack.SwapBytes;
	 break;
      case GL_UNPACK_SKIP_IMAGES_EXT:
         *params = ctx->Unpack.SkipImages;
         break;
      case GL_UNPACK_IMAGE_HEIGHT_EXT:
         *params = ctx->Unpack.ImageHeight;
         break;
      case GL_VIEWPORT:
	 params[0] = INT_TO_BOOL(ctx->Viewport.X);
	 params[1] = INT_TO_BOOL(ctx->Viewport.Y);
	 params[2] = INT_TO_BOOL(ctx->Viewport.Width);
	 params[3] = INT_TO_BOOL(ctx->Viewport.Height);
	 break;
      case GL_ZOOM_X:
	 *params = FLOAT_TO_BOOL(ctx->Pixel.ZoomX);
	 break;
      case GL_ZOOM_Y:
	 *params = FLOAT_TO_BOOL(ctx->Pixel.ZoomY);
	 break;
      case GL_VERTEX_ARRAY_SIZE:
         *params = INT_TO_BOOL(ctx->Array.Vertex.Size);
         break;
      case GL_VERTEX_ARRAY_TYPE:
         *params = ENUM_TO_BOOL(ctx->Array.Vertex.Type);
         break;
      case GL_VERTEX_ARRAY_STRIDE:
         *params = INT_TO_BOOL(ctx->Array.Vertex.Stride);
         break;
      case GL_VERTEX_ARRAY_COUNT_EXT:
         *params = INT_TO_BOOL(0);
         break;
      case GL_NORMAL_ARRAY_TYPE:
         *params = ENUM_TO_BOOL(ctx->Array.Normal.Type);
         break;
      case GL_NORMAL_ARRAY_STRIDE:
         *params = INT_TO_BOOL(ctx->Array.Normal.Stride);
         break;
      case GL_NORMAL_ARRAY_COUNT_EXT:
         *params = INT_TO_BOOL(0);
         break;
      case GL_COLOR_ARRAY_SIZE:
         *params = INT_TO_BOOL(ctx->Array.Color.Size);
         break;
      case GL_COLOR_ARRAY_TYPE:
         *params = ENUM_TO_BOOL(ctx->Array.Color.Type);
         break;
      case GL_COLOR_ARRAY_STRIDE:
         *params = INT_TO_BOOL(ctx->Array.Color.Stride);
         break;
      case GL_COLOR_ARRAY_COUNT_EXT:
         *params = INT_TO_BOOL(0);
         break;
      case GL_INDEX_ARRAY_TYPE:
         *params = ENUM_TO_BOOL(ctx->Array.Index.Type);
         break;
      case GL_INDEX_ARRAY_STRIDE:
         *params = INT_TO_BOOL(ctx->Array.Index.Stride);
         break;
      case GL_INDEX_ARRAY_COUNT_EXT:
         *params = INT_TO_BOOL(0);
         break;
      case GL_TEXTURE_COORD_ARRAY_SIZE:
         *params = INT_TO_BOOL(ctx->Array.TexCoord[texUnit].Size);
         break;
      case GL_TEXTURE_COORD_ARRAY_TYPE:
         *params = ENUM_TO_BOOL(ctx->Array.TexCoord[texUnit].Type);
         break;
      case GL_TEXTURE_COORD_ARRAY_STRIDE:
         *params = INT_TO_BOOL(ctx->Array.TexCoord[texUnit].Stride);
         break;
      case GL_TEXTURE_COORD_ARRAY_COUNT_EXT:
         *params = INT_TO_BOOL(0);
         break;
      case GL_EDGE_FLAG_ARRAY_STRIDE:
         *params = INT_TO_BOOL(ctx->Array.EdgeFlag.Stride);
         break;
      case GL_EDGE_FLAG_ARRAY_EXT:
         *params = INT_TO_BOOL(0);
         break;

      case GL_MAX_TEXTURE_UNITS_ARB:
         *params = ctx->Const.MaxTextureUnits;
         break;
      case GL_ACTIVE_TEXTURE_ARB:
         *params = INT_TO_BOOL(GL_TEXTURE0_ARB + ctx->Texture.CurrentUnit);
         break;
      case GL_CLIENT_ACTIVE_TEXTURE_ARB:
         *params = INT_TO_BOOL(GL_TEXTURE0_ARB + ctx->Array.ActiveTexture);
         break;


      /* GL_PGI_misc_hints */
      case GL_STRICT_DEPTHFUNC_HINT_PGI:
	 *params = ENUM_TO_BOOL(GL_NICEST);
         break;
      case GL_STRICT_LIGHTING_HINT_PGI:
	 *params = ENUM_TO_BOOL(ctx->Hint.StrictLighting);
	 break;
      case GL_STRICT_SCISSOR_HINT_PGI:
      case GL_FULL_STIPPLE_HINT_PGI:
	 *params = ENUM_TO_BOOL(GL_TRUE);
	 break;
      case GL_CONSERVE_MEMORY_HINT_PGI:
	 *params = ENUM_TO_BOOL(GL_FALSE);
	 break;
      case GL_ALWAYS_FAST_HINT_PGI:
	 *params = (GLboolean) (ctx->Hint.AllowDrawWin == GL_TRUE &&
			      ctx->Hint.AllowDrawSpn == GL_FALSE && 
			      ctx->Hint.AllowDrawMem == GL_FALSE);
	 break;
      case GL_ALWAYS_SOFT_HINT_PGI:
	 *params = (GLboolean) (ctx->Hint.AllowDrawWin == GL_TRUE &&
			      ctx->Hint.AllowDrawSpn == GL_TRUE && 
			      ctx->Hint.AllowDrawMem == GL_TRUE);
	 break;
      case GL_ALLOW_DRAW_OBJ_HINT_PGI:
	 *params = (GLboolean) GL_TRUE;
	 break;
      case GL_ALLOW_DRAW_WIN_HINT_PGI:
	 *params = (GLboolean) ctx->Hint.AllowDrawWin;
	 break;
      case GL_ALLOW_DRAW_SPN_HINT_PGI:
	 *params = (GLboolean) ctx->Hint.AllowDrawSpn;
	 break;
      case GL_ALLOW_DRAW_MEM_HINT_PGI:
	 *params = (GLboolean) ctx->Hint.AllowDrawMem;
	 break;
      case GL_CLIP_NEAR_HINT_PGI:
      case GL_CLIP_FAR_HINT_PGI:
	 *params = ENUM_TO_BOOL(GL_TRUE);
	 break;
      case GL_WIDE_LINE_HINT_PGI:
	 *params = ENUM_TO_BOOL(GL_DONT_CARE);
	 break;
      case GL_BACK_NORMALS_HINT_PGI:
	 *params = ENUM_TO_BOOL(GL_TRUE);
	 break;
      case GL_NATIVE_GRAPHICS_HANDLE_PGI:
	 *params = 0;
	 break;
      default:
	 printf("invalid enum: %x\n", pname);
         gl_error( ctx, GL_INVALID_ENUM, "glGetBooleanv" );
   }
}




void gl_GetDoublev( GLcontext *ctx, GLenum pname, GLdouble *params )
{
   GLuint i;
   GLuint texUnit = ctx->Texture.CurrentUnit;
   GLuint texTransformUnit = ctx->Texture.CurrentTransformUnit;
   const struct gl_texture_unit *textureUnit = &ctx->Texture.Unit[texUnit];

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glGetDoublev");

   if (MESA_VERBOSE & VERBOSE_API) 
      fprintf(stderr, "glGetDoublev %s\n", gl_lookup_enum_by_nr(pname));

   switch (pname) {
      case GL_ACCUM_RED_BITS:
      case GL_ACCUM_GREEN_BITS:
      case GL_ACCUM_BLUE_BITS:
      case GL_ACCUM_ALPHA_BITS:
         *params = (GLdouble) ctx->Visual->AccumBits;
         break;
      case GL_ACCUM_CLEAR_VALUE:
         params[0] = (GLdouble) ctx->Accum.ClearColor[0];
         params[1] = (GLdouble) ctx->Accum.ClearColor[1];
         params[2] = (GLdouble) ctx->Accum.ClearColor[2];
         params[3] = (GLdouble) ctx->Accum.ClearColor[3];
         break;
      case GL_ALPHA_BIAS:
         *params = (GLdouble) ctx->Pixel.AlphaBias;
         break;
      case GL_ALPHA_BITS:
         *params = (GLdouble) ctx->Visual->AlphaBits;
         break;
      case GL_ALPHA_SCALE:
         *params = (GLdouble) ctx->Pixel.AlphaScale;
         break;
      case GL_ALPHA_TEST:
         *params = (GLdouble) ctx->Color.AlphaEnabled;
         break;
      case GL_ALPHA_TEST_FUNC:
         *params = ENUM_TO_DOUBLE(ctx->Color.AlphaFunc);
         break;
      case GL_ALPHA_TEST_REF:
         *params = (GLdouble) ctx->Color.AlphaRef / 255.0;
         break;
      case GL_ATTRIB_STACK_DEPTH:
         *params = (GLdouble ) (ctx->AttribStackDepth);
         break;
      case GL_AUTO_NORMAL:
         *params = (GLdouble) ctx->Eval.AutoNormal;
         break;
      case GL_AUX_BUFFERS:
         *params = (GLdouble) NUM_AUX_BUFFERS;
         break;
      case GL_BLEND:
         *params = (GLdouble) ctx->Color.BlendEnabled;
         break;
      case GL_BLEND_DST:
         *params = ENUM_TO_DOUBLE(ctx->Color.BlendDstRGB);
         break;
      case GL_BLEND_SRC:
         *params = ENUM_TO_DOUBLE(ctx->Color.BlendSrcRGB);
         break;
      case GL_BLEND_SRC_RGB_INGR:
         *params = ENUM_TO_DOUBLE(ctx->Color.BlendSrcRGB);
         break;
      case GL_BLEND_DST_RGB_INGR:
         *params = ENUM_TO_DOUBLE(ctx->Color.BlendDstRGB);
         break;
      case GL_BLEND_SRC_ALPHA_INGR:
         *params = ENUM_TO_DOUBLE(ctx->Color.BlendSrcA);
         break;
      case GL_BLEND_DST_ALPHA_INGR:
         *params = ENUM_TO_DOUBLE(ctx->Color.BlendDstA);
         break;
      case GL_BLEND_EQUATION_EXT:
	 *params = ENUM_TO_DOUBLE(ctx->Color.BlendEquation);
	 break;
      case GL_BLEND_COLOR_EXT:
	 params[0] = (GLdouble) ctx->Color.BlendColor[0];
	 params[1] = (GLdouble) ctx->Color.BlendColor[1];
	 params[2] = (GLdouble) ctx->Color.BlendColor[2];
	 params[3] = (GLdouble) ctx->Color.BlendColor[3];
	 break;
      case GL_BLUE_BIAS:
         *params = (GLdouble) ctx->Pixel.BlueBias;
         break;
      case GL_BLUE_BITS:
         *params = (GLdouble) ctx->Visual->BlueBits;
         break;
      case GL_BLUE_SCALE:
         *params = (GLdouble) ctx->Pixel.BlueScale;
         break;
      case GL_CLIENT_ATTRIB_STACK_DEPTH:
         *params = (GLdouble) (ctx->ClientAttribStackDepth);
         break;
      case GL_CLIP_PLANE0:
      case GL_CLIP_PLANE1:
      case GL_CLIP_PLANE2:
      case GL_CLIP_PLANE3:
      case GL_CLIP_PLANE4:
      case GL_CLIP_PLANE5:
         *params = (GLdouble) ctx->Transform.ClipEnabled[pname-GL_CLIP_PLANE0];
         break;
      case GL_COLOR_CLEAR_VALUE:
         params[0] = (GLdouble) ctx->Color.ClearColor[0];
         params[1] = (GLdouble) ctx->Color.ClearColor[1];
         params[2] = (GLdouble) ctx->Color.ClearColor[2];
         params[3] = (GLdouble) ctx->Color.ClearColor[3];
         break;
      case GL_COLOR_MATERIAL:
         *params = (GLdouble) ctx->Light.ColorMaterialEnabled;
         break;
      case GL_COLOR_MATERIAL_FACE:
         *params = ENUM_TO_DOUBLE(ctx->Light.ColorMaterialFace);
         break;
      case GL_COLOR_MATERIAL_PARAMETER:
         *params = ENUM_TO_DOUBLE(ctx->Light.ColorMaterialMode);
         break;
      case GL_COLOR_WRITEMASK:
         params[0] = ctx->Color.ColorMask[RCOMP] ? 1.0 : 0.0;
         params[1] = ctx->Color.ColorMask[GCOMP] ? 1.0 : 0.0;
         params[2] = ctx->Color.ColorMask[BCOMP] ? 1.0 : 0.0;
         params[3] = ctx->Color.ColorMask[ACOMP] ? 1.0 : 0.0;
         break;
      case GL_CULL_FACE:
         *params = (GLdouble) ctx->Polygon.CullFlag;
         break;
      case GL_CULL_FACE_MODE:
         *params = ENUM_TO_DOUBLE(ctx->Polygon.CullFaceMode);
         break;
      case GL_CURRENT_COLOR:
         params[0] = UBYTE_COLOR_TO_FLOAT_COLOR(ctx->Current.ByteColor[0]);
         params[1] = UBYTE_COLOR_TO_FLOAT_COLOR(ctx->Current.ByteColor[1]);
         params[2] = UBYTE_COLOR_TO_FLOAT_COLOR(ctx->Current.ByteColor[2]);
         params[3] = UBYTE_COLOR_TO_FLOAT_COLOR(ctx->Current.ByteColor[3]);
         break;
      case GL_CURRENT_INDEX:
         *params = (GLdouble) ctx->Current.Index;
         break;
      case GL_CURRENT_NORMAL:
         params[0] = (GLdouble) ctx->Current.Normal[0];
         params[1] = (GLdouble) ctx->Current.Normal[1];
         params[2] = (GLdouble) ctx->Current.Normal[2];
         break;
      case GL_CURRENT_RASTER_COLOR:
	 params[0] = (GLdouble) ctx->Current.RasterColor[0];
	 params[1] = (GLdouble) ctx->Current.RasterColor[1];
	 params[2] = (GLdouble) ctx->Current.RasterColor[2];
	 params[3] = (GLdouble) ctx->Current.RasterColor[3];
	 break;
      case GL_CURRENT_RASTER_DISTANCE:
	 params[0] = (GLdouble) ctx->Current.RasterDistance;
	 break;
      case GL_CURRENT_RASTER_INDEX:
	 *params = (GLdouble) ctx->Current.RasterIndex;
	 break;
      case GL_CURRENT_RASTER_POSITION:
	 params[0] = (GLdouble) ctx->Current.RasterPos[0];
	 params[1] = (GLdouble) ctx->Current.RasterPos[1];
	 params[2] = (GLdouble) ctx->Current.RasterPos[2];
	 params[3] = (GLdouble) ctx->Current.RasterPos[3];
	 break;
      case GL_CURRENT_RASTER_TEXTURE_COORDS:
	 params[0] = (GLdouble) ctx->Current.RasterMultiTexCoord[texTransformUnit][0];
	 params[1] = (GLdouble) ctx->Current.RasterMultiTexCoord[texTransformUnit][1];
	 params[2] = (GLdouble) ctx->Current.RasterMultiTexCoord[texTransformUnit][2];
	 params[3] = (GLdouble) ctx->Current.RasterMultiTexCoord[texTransformUnit][3];
	 break;
      case GL_CURRENT_RASTER_POSITION_VALID:
	 *params = (GLdouble) ctx->Current.RasterPosValid;
	 break;
      case GL_CURRENT_TEXTURE_COORDS:
	 params[0] = (GLdouble) ctx->Current.Texcoord[texTransformUnit][0];
	 params[1] = (GLdouble) ctx->Current.Texcoord[texTransformUnit][1];
	 params[2] = (GLdouble) ctx->Current.Texcoord[texTransformUnit][2];
	 params[3] = (GLdouble) ctx->Current.Texcoord[texTransformUnit][3];
	 break;
      case GL_DEPTH_BIAS:
	 *params = (GLdouble) ctx->Pixel.DepthBias;
	 break;
      case GL_DEPTH_BITS:
	 *params = (GLdouble) ctx->Visual->DepthBits;
	 break;
      case GL_DEPTH_CLEAR_VALUE:
	 *params = (GLdouble) ctx->Depth.Clear;
	 break;
      case GL_DEPTH_FUNC:
	 *params = ENUM_TO_DOUBLE(ctx->Depth.Func);
	 break;
      case GL_DEPTH_RANGE:
         params[0] = (GLdouble) ctx->Viewport.Near;
         params[1] = (GLdouble) ctx->Viewport.Far;
	 break;
      case GL_DEPTH_SCALE:
	 *params = (GLdouble) ctx->Pixel.DepthScale;
	 break;
      case GL_DEPTH_TEST:
	 *params = (GLdouble) ctx->Depth.Test;
	 break;
      case GL_DEPTH_WRITEMASK:
	 *params = (GLdouble) ctx->Depth.Mask;
	 break;
      case GL_DITHER:
	 *params = (GLdouble) ctx->Color.DitherFlag;
	 break;
      case GL_DOUBLEBUFFER:
	 *params = (GLdouble) ctx->Visual->DBflag;
	 break;
      case GL_DRAW_BUFFER:
	 *params = ENUM_TO_DOUBLE(ctx->Color.DrawBuffer);
	 break;
      case GL_EDGE_FLAG:
	 *params = (GLdouble) ctx->Current.EdgeFlag;
	 break;
      case GL_FEEDBACK_BUFFER_SIZE:
         /* TODO: is this right?  Or, return number of entries in buffer? */
         *params = (GLdouble) ctx->Feedback.BufferSize;
         break;
      case GL_FEEDBACK_BUFFER_TYPE:
         *params = ENUM_TO_DOUBLE(ctx->Feedback.Type);
         break;
      case GL_FOG:
	 *params = (GLdouble) ctx->Fog.Enabled;
	 break;
      case GL_FOG_COLOR:
	 params[0] = (GLdouble) ctx->Fog.Color[0];
	 params[1] = (GLdouble) ctx->Fog.Color[1];
	 params[2] = (GLdouble) ctx->Fog.Color[2];
	 params[3] = (GLdouble) ctx->Fog.Color[3];
	 break;
      case GL_FOG_DENSITY:
	 *params = (GLdouble) ctx->Fog.Density;
	 break;
      case GL_FOG_END:
	 *params = (GLdouble) ctx->Fog.End;
	 break;
      case GL_FOG_HINT:
	 *params = ENUM_TO_DOUBLE(ctx->Hint.Fog);
	 break;
      case GL_FOG_INDEX:
	 *params = (GLdouble) ctx->Fog.Index;
	 break;
      case GL_FOG_MODE:
	 *params = ENUM_TO_DOUBLE(ctx->Fog.Mode);
	 break;
      case GL_FOG_START:
	 *params = (GLdouble) ctx->Fog.Start;
	 break;
      case GL_FRONT_FACE:
	 *params = ENUM_TO_DOUBLE(ctx->Polygon.FrontFace);
	 break;
      case GL_GREEN_BIAS:
         *params = (GLdouble) ctx->Pixel.GreenBias;
         break;
      case GL_GREEN_BITS:
         *params = (GLdouble) ctx->Visual->GreenBits;
         break;
      case GL_GREEN_SCALE:
         *params = (GLdouble) ctx->Pixel.GreenScale;
         break;
      case GL_INDEX_BITS:
         *params = (GLdouble) ctx->Visual->IndexBits;
	 break;
      case GL_INDEX_CLEAR_VALUE:
         *params = (GLdouble) ctx->Color.ClearIndex;
	 break;
      case GL_INDEX_MODE:
	 *params = ctx->Visual->RGBAflag ? 0.0 : 1.0;
	 break;
      case GL_INDEX_OFFSET:
	 *params = (GLdouble) ctx->Pixel.IndexOffset;
	 break;
      case GL_INDEX_SHIFT:
	 *params = (GLdouble) ctx->Pixel.IndexShift;
	 break;
      case GL_INDEX_WRITEMASK:
	 *params = (GLdouble) ctx->Color.IndexMask;
	 break;
      case GL_LIGHT0:
      case GL_LIGHT1:
      case GL_LIGHT2:
      case GL_LIGHT3:
      case GL_LIGHT4:
      case GL_LIGHT5:
      case GL_LIGHT6:
      case GL_LIGHT7:
	 *params = (GLdouble) ctx->Light.Light[pname-GL_LIGHT0].Enabled;
	 break;
      case GL_LIGHTING:
	 *params = (GLdouble) ctx->Light.Enabled;
	 break;
      case GL_LIGHT_MODEL_AMBIENT:
	 params[0] = (GLdouble) ctx->Light.Model.Ambient[0];
	 params[1] = (GLdouble) ctx->Light.Model.Ambient[1];
	 params[2] = (GLdouble) ctx->Light.Model.Ambient[2];
	 params[3] = (GLdouble) ctx->Light.Model.Ambient[3];
	 break;
      case GL_LIGHT_MODEL_COLOR_CONTROL:
         params[0] = (GLdouble) ctx->Light.Model.ColorControl;
         break;
      case GL_LIGHT_MODEL_LOCAL_VIEWER:
	 *params = (GLdouble) ctx->Light.Model.LocalViewer;
	 break;
      case GL_LIGHT_MODEL_TWO_SIDE:
	 *params = (GLdouble) ctx->Light.Model.TwoSide;
	 break;
      case GL_LINE_SMOOTH:
	 *params = (GLdouble) ctx->Line.SmoothFlag;
	 break;
      case GL_LINE_SMOOTH_HINT:
	 *params = ENUM_TO_DOUBLE(ctx->Hint.LineSmooth);
	 break;
      case GL_LINE_STIPPLE:
	 *params = (GLdouble) ctx->Line.StippleFlag;
	 break;
      case GL_LINE_STIPPLE_PATTERN:
         *params = (GLdouble) ctx->Line.StipplePattern;
         break;
      case GL_LINE_STIPPLE_REPEAT:
         *params = (GLdouble) ctx->Line.StippleFactor;
         break;
      case GL_LINE_WIDTH:
	 *params = (GLdouble) ctx->Line.Width;
	 break;
      case GL_LINE_WIDTH_GRANULARITY:
	 *params = (GLdouble) LINE_WIDTH_GRANULARITY;
	 break;
      case GL_LINE_WIDTH_RANGE:
	 params[0] = (GLdouble) MIN_LINE_WIDTH;
	 params[1] = (GLdouble) MAX_LINE_WIDTH;
	 break;
      case GL_LIST_BASE:
	 *params = (GLdouble) ctx->List.ListBase;
	 break;
      case GL_LIST_INDEX:
	 *params = (GLdouble) ctx->CurrentListNum;
	 break;
      case GL_LIST_MODE:
	 *params = ctx->ExecuteFlag ? ENUM_TO_DOUBLE(GL_COMPILE_AND_EXECUTE)
	   			  : ENUM_TO_DOUBLE(GL_COMPILE);
	 break;
      case GL_INDEX_LOGIC_OP:
	 *params = (GLdouble) ctx->Color.IndexLogicOpEnabled;
	 break;
      case GL_COLOR_LOGIC_OP:
	 *params = (GLdouble) ctx->Color.ColorLogicOpEnabled;
	 break;
      case GL_LOGIC_OP_MODE:
         *params = ENUM_TO_DOUBLE(ctx->Color.LogicOp);
	 break;
      case GL_MAP1_COLOR_4:
	 *params = (GLdouble) ctx->Eval.Map1Color4;
	 break;
      case GL_MAP1_GRID_DOMAIN:
	 params[0] = (GLdouble) ctx->Eval.MapGrid1u1;
	 params[1] = (GLdouble) ctx->Eval.MapGrid1u2;
	 break;
      case GL_MAP1_GRID_SEGMENTS:
	 *params = (GLdouble) ctx->Eval.MapGrid1un;
	 break;
      case GL_MAP1_INDEX:
	 *params = (GLdouble) ctx->Eval.Map1Index;
	 break;
      case GL_MAP1_NORMAL:
	 *params = (GLdouble) ctx->Eval.Map1Normal;
	 break;
      case GL_MAP1_TEXTURE_COORD_1:
	 *params = (GLdouble) ctx->Eval.Map1TextureCoord1;
	 break;
      case GL_MAP1_TEXTURE_COORD_2:
	 *params = (GLdouble) ctx->Eval.Map1TextureCoord2;
	 break;
      case GL_MAP1_TEXTURE_COORD_3:
	 *params = (GLdouble) ctx->Eval.Map1TextureCoord3;
	 break;
      case GL_MAP1_TEXTURE_COORD_4:
	 *params = (GLdouble) ctx->Eval.Map1TextureCoord4;
	 break;
      case GL_MAP1_VERTEX_3:
	 *params = (GLdouble) ctx->Eval.Map1Vertex3;
	 break;
      case GL_MAP1_VERTEX_4:
	 *params = (GLdouble) ctx->Eval.Map1Vertex4;
	 break;
      case GL_MAP2_COLOR_4:
	 *params = (GLdouble) ctx->Eval.Map2Color4;
	 break;
      case GL_MAP2_GRID_DOMAIN:
	 params[0] = (GLdouble) ctx->Eval.MapGrid2u1;
	 params[1] = (GLdouble) ctx->Eval.MapGrid2u2;
	 params[2] = (GLdouble) ctx->Eval.MapGrid2v1;
	 params[3] = (GLdouble) ctx->Eval.MapGrid2v2;
	 break;
      case GL_MAP2_GRID_SEGMENTS:
	 params[0] = (GLdouble) ctx->Eval.MapGrid2un;
	 params[1] = (GLdouble) ctx->Eval.MapGrid2vn;
	 break;
      case GL_MAP2_INDEX:
	 *params = (GLdouble) ctx->Eval.Map2Index;
	 break;
      case GL_MAP2_NORMAL:
	 *params = (GLdouble) ctx->Eval.Map2Normal;
	 break;
      case GL_MAP2_TEXTURE_COORD_1:
	 *params = (GLdouble) ctx->Eval.Map2TextureCoord1;
	 break;
      case GL_MAP2_TEXTURE_COORD_2:
	 *params = (GLdouble) ctx->Eval.Map2TextureCoord2;
	 break;
      case GL_MAP2_TEXTURE_COORD_3:
	 *params = (GLdouble) ctx->Eval.Map2TextureCoord3;
	 break;
      case GL_MAP2_TEXTURE_COORD_4:
	 *params = (GLdouble) ctx->Eval.Map2TextureCoord4;
	 break;
      case GL_MAP2_VERTEX_3:
	 *params = (GLdouble) ctx->Eval.Map2Vertex3;
	 break;
      case GL_MAP2_VERTEX_4:
	 *params = (GLdouble) ctx->Eval.Map2Vertex4;
	 break;
      case GL_MAP_COLOR:
	 *params = (GLdouble) ctx->Pixel.MapColorFlag;
	 break;
      case GL_MAP_STENCIL:
	 *params = (GLdouble) ctx->Pixel.MapStencilFlag;
	 break;
      case GL_MATRIX_MODE:
	 *params = ENUM_TO_DOUBLE(ctx->Transform.MatrixMode);
	 break;
      case GL_MAX_ATTRIB_STACK_DEPTH:
	 *params = (GLdouble) MAX_ATTRIB_STACK_DEPTH;
	 break;
      case GL_MAX_CLIENT_ATTRIB_STACK_DEPTH:
         *params = (GLdouble) MAX_CLIENT_ATTRIB_STACK_DEPTH;
         break;
      case GL_MAX_CLIP_PLANES:
	 *params = (GLdouble) MAX_CLIP_PLANES;
	 break;
      case GL_MAX_ELEMENTS_VERTICES:  /* GL_VERSION_1_2 */
         *params = (GLdouble) VB_MAX;
         break;
      case GL_MAX_ELEMENTS_INDICES:   /* GL_VERSION_1_2 */
         *params = (GLdouble) VB_MAX;
         break;
      case GL_MAX_EVAL_ORDER:
	 *params = (GLdouble) MAX_EVAL_ORDER;
	 break;
      case GL_MAX_LIGHTS:
	 *params = (GLdouble) MAX_LIGHTS;
	 break;
      case GL_MAX_LIST_NESTING:
	 *params = (GLdouble) MAX_LIST_NESTING;
	 break;
      case GL_MAX_MODELVIEW_STACK_DEPTH:
	 *params = (GLdouble) MAX_MODELVIEW_STACK_DEPTH;
	 break;
      case GL_MAX_NAME_STACK_DEPTH:
	 *params = (GLdouble) MAX_NAME_STACK_DEPTH;
	 break;
      case GL_MAX_PIXEL_MAP_TABLE:
	 *params = (GLdouble) MAX_PIXEL_MAP_TABLE;
	 break;
      case GL_MAX_PROJECTION_STACK_DEPTH:
	 *params = (GLdouble) MAX_PROJECTION_STACK_DEPTH;
	 break;
      case GL_MAX_TEXTURE_SIZE:
      case GL_MAX_3D_TEXTURE_SIZE:
         *params = (GLdouble) ctx->Const.MaxTextureSize;
	 break;
      case GL_MAX_TEXTURE_STACK_DEPTH:
	 *params = (GLdouble) MAX_TEXTURE_STACK_DEPTH;
	 break;
      case GL_MAX_VIEWPORT_DIMS:
         params[0] = (GLdouble) MAX_WIDTH;
         params[1] = (GLdouble) MAX_HEIGHT;
         break;
      case GL_MODELVIEW_MATRIX:
	 for (i=0;i<16;i++) {
	    params[i] = (GLdouble) ctx->ModelView.m[i];
	 }
	 break;
      case GL_MODELVIEW_STACK_DEPTH:
	 *params = (GLdouble) (ctx->ModelViewStackDepth + 1);
	 break;
      case GL_NAME_STACK_DEPTH:
	 *params = (GLdouble) ctx->Select.NameStackDepth;
	 break;
      case GL_NORMALIZE:
	 *params = (GLdouble) ctx->Transform.Normalize;
	 break;
      case GL_PACK_ALIGNMENT:
	 *params = (GLdouble) ctx->Pack.Alignment;
	 break;
      case GL_PACK_LSB_FIRST:
	 *params = (GLdouble) ctx->Pack.LsbFirst;
	 break;
      case GL_PACK_ROW_LENGTH:
	 *params = (GLdouble) ctx->Pack.RowLength;
	 break;
      case GL_PACK_SKIP_PIXELS:
	 *params = (GLdouble) ctx->Pack.SkipPixels;
	 break;
      case GL_PACK_SKIP_ROWS:
	 *params = (GLdouble) ctx->Pack.SkipRows;
	 break;
      case GL_PACK_SWAP_BYTES:
	 *params = (GLdouble) ctx->Pack.SwapBytes;
	 break;
      case GL_PACK_SKIP_IMAGES_EXT:
         *params = (GLdouble) ctx->Pack.SkipImages;
         break;
      case GL_PACK_IMAGE_HEIGHT_EXT:
         *params = (GLdouble) ctx->Pack.ImageHeight;
         break;
      case GL_PERSPECTIVE_CORRECTION_HINT:
	 *params = ENUM_TO_DOUBLE(ctx->Hint.PerspectiveCorrection);
	 break;
      case GL_PIXEL_MAP_A_TO_A_SIZE:
	 *params = (GLdouble) ctx->Pixel.MapAtoAsize;
	 break;
      case GL_PIXEL_MAP_B_TO_B_SIZE:
	 *params = (GLdouble) ctx->Pixel.MapBtoBsize;
	 break;
      case GL_PIXEL_MAP_G_TO_G_SIZE:
	 *params = (GLdouble) ctx->Pixel.MapGtoGsize;
	 break;
      case GL_PIXEL_MAP_I_TO_A_SIZE:
	 *params = (GLdouble) ctx->Pixel.MapItoAsize;
	 break;
      case GL_PIXEL_MAP_I_TO_B_SIZE:
	 *params = (GLdouble) ctx->Pixel.MapItoBsize;
	 break;
      case GL_PIXEL_MAP_I_TO_G_SIZE:
	 *params = (GLdouble) ctx->Pixel.MapItoGsize;
	 break;
      case GL_PIXEL_MAP_I_TO_I_SIZE:
	 *params = (GLdouble) ctx->Pixel.MapItoIsize;
	 break;
      case GL_PIXEL_MAP_I_TO_R_SIZE:
	 *params = (GLdouble) ctx->Pixel.MapItoRsize;
	 break;
      case GL_PIXEL_MAP_R_TO_R_SIZE:
	 *params = (GLdouble) ctx->Pixel.MapRtoRsize;
	 break;
      case GL_PIXEL_MAP_S_TO_S_SIZE:
	 *params = (GLdouble) ctx->Pixel.MapStoSsize;
	 break;
      case GL_POINT_SIZE:
         *params = (GLdouble) ctx->Point.Size;
         break;
      case GL_POINT_SIZE_GRANULARITY:
	 *params = (GLdouble) POINT_SIZE_GRANULARITY;
	 break;
      case GL_POINT_SIZE_RANGE:
	 params[0] = (GLdouble) MIN_POINT_SIZE;
	 params[1] = (GLdouble) MAX_POINT_SIZE;
	 break;
      case GL_POINT_SMOOTH:
	 *params = (GLdouble) ctx->Point.SmoothFlag;
	 break;
      case GL_POINT_SMOOTH_HINT:
	 *params = ENUM_TO_DOUBLE(ctx->Hint.PointSmooth);
	 break;
      case GL_POINT_SIZE_MIN_EXT:
	 *params = (GLdouble) (ctx->Point.MinSize);
	 break;
      case GL_POINT_SIZE_MAX_EXT:
	 *params = (GLdouble) (ctx->Point.MaxSize);
	 break;
      case GL_POINT_FADE_THRESHOLD_SIZE_EXT:
	 *params = (GLdouble) (ctx->Point.Threshold);
	 break;
      case GL_DISTANCE_ATTENUATION_EXT:
	 params[0] = (GLdouble) (ctx->Point.Params[0]);
	 params[1] = (GLdouble) (ctx->Point.Params[1]);
	 params[2] = (GLdouble) (ctx->Point.Params[2]);
	 break;
      case GL_POLYGON_MODE:
	 params[0] = ENUM_TO_DOUBLE(ctx->Polygon.FrontMode);
	 params[1] = ENUM_TO_DOUBLE(ctx->Polygon.BackMode);
	 break;
#ifdef GL_EXT_polygon_offset
      case GL_POLYGON_OFFSET_BIAS_EXT:
         *params = (GLdouble) ctx->Polygon.OffsetUnits;
         break;
#endif
      case GL_POLYGON_OFFSET_FACTOR:
         *params = (GLdouble) ctx->Polygon.OffsetFactor;
         break;
      case GL_POLYGON_OFFSET_UNITS:
         *params = (GLdouble) ctx->Polygon.OffsetUnits;
         break;
      case GL_POLYGON_SMOOTH:
	 *params = (GLdouble) ctx->Polygon.SmoothFlag;
	 break;
      case GL_POLYGON_SMOOTH_HINT:
	 *params = ENUM_TO_DOUBLE(ctx->Hint.PolygonSmooth);
	 break;
      case GL_POLYGON_STIPPLE:
         *params = (GLdouble) ctx->Polygon.StippleFlag;
	 break;
      case GL_PROJECTION_MATRIX:
	 for (i=0;i<16;i++) {
	    params[i] = (GLdouble) ctx->ProjectionMatrix.m[i];
	 }
	 break;
      case GL_PROJECTION_STACK_DEPTH:
	 *params = (GLdouble) (ctx->ProjectionStackDepth + 1);
	 break;
      case GL_READ_BUFFER:
	 *params = ENUM_TO_DOUBLE(ctx->Pixel.ReadBuffer);
	 break;
      case GL_RED_BIAS:
         *params = (GLdouble) ctx->Pixel.RedBias;
         break;
      case GL_RED_BITS:
         *params = (GLdouble) ctx->Visual->RedBits;
         break;
      case GL_RED_SCALE:
         *params = (GLdouble) ctx->Pixel.RedScale;
         break;
      case GL_RENDER_MODE:
	 *params = ENUM_TO_DOUBLE(ctx->RenderMode);
	 break;
      case GL_RGBA_MODE:
	 *params = (GLdouble) ctx->Visual->RGBAflag;
	 break;
      case GL_SCISSOR_BOX:
	 params[0] = (GLdouble) ctx->Scissor.X;
	 params[1] = (GLdouble) ctx->Scissor.Y;
	 params[2] = (GLdouble) ctx->Scissor.Width;
	 params[3] = (GLdouble) ctx->Scissor.Height;
	 break;
      case GL_SCISSOR_TEST:
	 *params = (GLdouble) ctx->Scissor.Enabled;
	 break;
      case GL_SELECTION_BUFFER_SIZE:
         *params = (GLdouble) ctx->Select.BufferSize;
         break;
      case GL_SHADE_MODEL:
	 *params = ENUM_TO_DOUBLE(ctx->Light.ShadeModel);
	 break;
      case GL_SHARED_TEXTURE_PALETTE_EXT:
         *params = (GLdouble) ctx->Texture.SharedPalette;
         break;
      case GL_STENCIL_BITS:
         *params = (GLdouble) ctx->Visual->StencilBits;
         break;
      case GL_STENCIL_CLEAR_VALUE:
	 *params = (GLdouble) ctx->Stencil.Clear;
	 break;
      case GL_STENCIL_FAIL:
	 *params = ENUM_TO_DOUBLE(ctx->Stencil.FailFunc);
	 break;
      case GL_STENCIL_FUNC:
	 *params = ENUM_TO_DOUBLE(ctx->Stencil.Function);
	 break;
      case GL_STENCIL_PASS_DEPTH_FAIL:
	 *params = ENUM_TO_DOUBLE(ctx->Stencil.ZFailFunc);
	 break;
      case GL_STENCIL_PASS_DEPTH_PASS:
	 *params = ENUM_TO_DOUBLE(ctx->Stencil.ZPassFunc);
	 break;
      case GL_STENCIL_REF:
	 *params = (GLdouble) ctx->Stencil.Ref;
	 break;
      case GL_STENCIL_TEST:
	 *params = (GLdouble) ctx->Stencil.Enabled;
	 break;
      case GL_STENCIL_VALUE_MASK:
	 *params = (GLdouble) ctx->Stencil.ValueMask;
	 break;
      case GL_STENCIL_WRITEMASK:
	 *params = (GLdouble) ctx->Stencil.WriteMask;
	 break;
      case GL_STEREO:
	 *params = (GLdouble) ctx->Visual->StereoFlag;
	 break;
      case GL_SUBPIXEL_BITS:
	 *params = 0.0;   /* TODO */
	 break;
      case GL_TEXTURE_1D:
         *params = gl_IsEnabled(ctx, GL_TEXTURE_1D) ? 1.0 : 0.0;
	 break;
      case GL_TEXTURE_2D:
         *params = gl_IsEnabled(ctx, GL_TEXTURE_2D) ? 1.0 : 0.0;
	 break;
      case GL_TEXTURE_3D:
         *params = gl_IsEnabled(ctx, GL_TEXTURE_3D) ? 1.0 : 0.0;
	 break;
      case GL_TEXTURE_BINDING_1D:
         *params = (GLdouble) textureUnit->CurrentD[1]->Name;
          break;
      case GL_TEXTURE_BINDING_2D:
         *params = (GLdouble) textureUnit->CurrentD[2]->Name;
          break;
      case GL_TEXTURE_BINDING_3D:
         *params = (GLdouble) textureUnit->CurrentD[3]->Name;
          break;
      case GL_TEXTURE_ENV_COLOR:
	 params[0] = (GLdouble) textureUnit->EnvColor[0];
	 params[1] = (GLdouble) textureUnit->EnvColor[1];
	 params[2] = (GLdouble) textureUnit->EnvColor[2];
	 params[3] = (GLdouble) textureUnit->EnvColor[3];
	 break;
      case GL_TEXTURE_ENV_MODE:
	 *params = ENUM_TO_DOUBLE(textureUnit->EnvMode);
	 break;
      case GL_TEXTURE_GEN_S:
	 *params = (textureUnit->TexGenEnabled & S_BIT) ? 1.0 : 0.0;
	 break;
      case GL_TEXTURE_GEN_T:
	 *params = (textureUnit->TexGenEnabled & T_BIT) ? 1.0 : 0.0;
	 break;
      case GL_TEXTURE_GEN_R:
	 *params = (textureUnit->TexGenEnabled & R_BIT) ? 1.0 : 0.0;
	 break;
      case GL_TEXTURE_GEN_Q:
	 *params = (textureUnit->TexGenEnabled & Q_BIT) ? 1.0 : 0.0;
	 break;
      case GL_TEXTURE_MATRIX:
         for (i=0;i<16;i++) {
	    params[i] = (GLdouble) ctx->TextureMatrix[texTransformUnit].m[i];
	 }
	 break;
      case GL_TEXTURE_STACK_DEPTH:
	 *params = (GLdouble) (ctx->TextureStackDepth[texTransformUnit] + 1);
	 break;
      case GL_UNPACK_ALIGNMENT:
	 *params = (GLdouble) ctx->Unpack.Alignment;
	 break;
      case GL_UNPACK_LSB_FIRST:
	 *params = (GLdouble) ctx->Unpack.LsbFirst;
	 break;
      case GL_UNPACK_ROW_LENGTH:
	 *params = (GLdouble) ctx->Unpack.RowLength;
	 break;
      case GL_UNPACK_SKIP_PIXELS:
	 *params = (GLdouble) ctx->Unpack.SkipPixels;
	 break;
      case GL_UNPACK_SKIP_ROWS:
	 *params = (GLdouble) ctx->Unpack.SkipRows;
	 break;
      case GL_UNPACK_SWAP_BYTES:
	 *params = (GLdouble) ctx->Unpack.SwapBytes;
	 break;
      case GL_UNPACK_SKIP_IMAGES_EXT:
         *params = (GLdouble) ctx->Unpack.SkipImages;
         break;
      case GL_UNPACK_IMAGE_HEIGHT_EXT:
         *params = (GLdouble) ctx->Unpack.ImageHeight;
         break;
      case GL_VIEWPORT:
	 params[0] = (GLdouble) ctx->Viewport.X;
	 params[1] = (GLdouble) ctx->Viewport.Y;
	 params[2] = (GLdouble) ctx->Viewport.Width;
	 params[3] = (GLdouble) ctx->Viewport.Height;
	 break;
      case GL_ZOOM_X:
	 *params = (GLdouble) ctx->Pixel.ZoomX;
	 break;
      case GL_ZOOM_Y:
	 *params = (GLdouble) ctx->Pixel.ZoomY;
	 break;
      case GL_VERTEX_ARRAY_SIZE:
         *params = (GLdouble) ctx->Array.Vertex.Size;
         break;
      case GL_VERTEX_ARRAY_TYPE:
         *params = ENUM_TO_DOUBLE(ctx->Array.Vertex.Type);
         break;
      case GL_VERTEX_ARRAY_STRIDE:
         *params = (GLdouble) ctx->Array.Vertex.Stride;
         break;
      case GL_VERTEX_ARRAY_COUNT_EXT:
         *params = 0.0;
         break;
      case GL_NORMAL_ARRAY_TYPE:
         *params = ENUM_TO_DOUBLE(ctx->Array.Normal.Type);
         break;
      case GL_NORMAL_ARRAY_STRIDE:
         *params = (GLdouble) ctx->Array.Normal.Stride;
         break;
      case GL_NORMAL_ARRAY_COUNT_EXT:
         *params = 0.0;
         break;
      case GL_COLOR_ARRAY_SIZE:
         *params = (GLdouble) ctx->Array.Color.Size;
         break;
      case GL_COLOR_ARRAY_TYPE:
         *params = ENUM_TO_DOUBLE(ctx->Array.Color.Type);
         break;
      case GL_COLOR_ARRAY_STRIDE:
         *params = (GLdouble) ctx->Array.Color.Stride;
         break;
      case GL_COLOR_ARRAY_COUNT_EXT:
         *params = 0.0;
         break;
      case GL_INDEX_ARRAY_TYPE:
         *params = ENUM_TO_DOUBLE(ctx->Array.Index.Type);
         break;
      case GL_INDEX_ARRAY_STRIDE:
         *params = (GLdouble) ctx->Array.Index.Stride;
         break;
      case GL_INDEX_ARRAY_COUNT_EXT:
         *params = 0.0;
         break;
      case GL_TEXTURE_COORD_ARRAY_SIZE:
         *params = (GLdouble) ctx->Array.TexCoord[texUnit].Size;
         break;
      case GL_TEXTURE_COORD_ARRAY_TYPE:
         *params = ENUM_TO_DOUBLE(ctx->Array.TexCoord[texUnit].Type);
         break;
      case GL_TEXTURE_COORD_ARRAY_STRIDE:
         *params = (GLdouble) ctx->Array.TexCoord[texUnit].Stride;
         break;
      case GL_TEXTURE_COORD_ARRAY_COUNT_EXT:
         *params = 0.0;
         break;
      case GL_EDGE_FLAG_ARRAY_STRIDE:
         *params = (GLdouble) ctx->Array.EdgeFlag.Stride;
         break;
      case GL_EDGE_FLAG_ARRAY_COUNT_EXT:
         *params = 0.0;
         break;

      case GL_MAX_TEXTURE_UNITS_ARB:
         *params = (GLdouble) ctx->Const.MaxTextureUnits;
         break;
      case GL_ACTIVE_TEXTURE_ARB:
         *params = (GLdouble) (GL_TEXTURE0_ARB + ctx->Texture.CurrentUnit);
         break;
      case GL_CLIENT_ACTIVE_TEXTURE_ARB:
         *params = (GLdouble) (GL_TEXTURE0_ARB + ctx->Array.ActiveTexture);
         break;


      /* GL_PGI_misc_hints */
      case GL_STRICT_DEPTHFUNC_HINT_PGI:
	 *params = ENUM_TO_DOUBLE(GL_NICEST);
         break;
      case GL_STRICT_LIGHTING_HINT_PGI:
	 *params = ENUM_TO_DOUBLE(ctx->Hint.StrictLighting);
	 break;
      case GL_STRICT_SCISSOR_HINT_PGI:
      case GL_FULL_STIPPLE_HINT_PGI:
	 *params = ENUM_TO_DOUBLE(GL_TRUE);
	 break;
      case GL_CONSERVE_MEMORY_HINT_PGI:
	 *params = ENUM_TO_DOUBLE(GL_FALSE);
	 break;
      case GL_ALWAYS_FAST_HINT_PGI:
	 *params = (GLdouble) (ctx->Hint.AllowDrawWin == GL_TRUE &&
			      ctx->Hint.AllowDrawSpn == GL_FALSE && 
			      ctx->Hint.AllowDrawMem == GL_FALSE);
	 break;
      case GL_ALWAYS_SOFT_HINT_PGI:
	 *params = (GLdouble) (ctx->Hint.AllowDrawWin == GL_TRUE &&
			      ctx->Hint.AllowDrawSpn == GL_TRUE && 
			      ctx->Hint.AllowDrawMem == GL_TRUE);
	 break;
      case GL_ALLOW_DRAW_OBJ_HINT_PGI:
	 *params = (GLdouble) GL_TRUE;
	 break;
      case GL_ALLOW_DRAW_WIN_HINT_PGI:
	 *params = (GLdouble) ctx->Hint.AllowDrawWin;
	 break;
      case GL_ALLOW_DRAW_SPN_HINT_PGI:
	 *params = (GLdouble) ctx->Hint.AllowDrawSpn;
	 break;
      case GL_ALLOW_DRAW_MEM_HINT_PGI:
	 *params = (GLdouble) ctx->Hint.AllowDrawMem;
	 break;
      case GL_CLIP_NEAR_HINT_PGI:
      case GL_CLIP_FAR_HINT_PGI:
	 *params = ENUM_TO_DOUBLE(GL_TRUE);
	 break;
      case GL_WIDE_LINE_HINT_PGI:
	 *params = ENUM_TO_DOUBLE(GL_DONT_CARE);
	 break;
      case GL_BACK_NORMALS_HINT_PGI:
	 *params = ENUM_TO_DOUBLE(GL_TRUE);
	 break;
      case GL_NATIVE_GRAPHICS_HANDLE_PGI:
	 *params = 0;
	 break;



      default:
	 printf("invalid enum: %x\n", pname);
         gl_error( ctx, GL_INVALID_ENUM, "glGetDoublev" );
   }
}




void gl_GetFloatv( GLcontext *ctx, GLenum pname, GLfloat *params )
{
   GLuint i;
   GLuint texUnit = ctx->Texture.CurrentUnit;
   GLuint texTransformUnit = ctx->Texture.CurrentTransformUnit;
   const struct gl_texture_unit *textureUnit = &ctx->Texture.Unit[texUnit];

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glGetFloatv");

   if (MESA_VERBOSE & VERBOSE_API) 
      fprintf(stderr, "glGetFloatv %s\n", gl_lookup_enum_by_nr(pname));

   switch (pname) {
      case GL_ACCUM_RED_BITS:
      case GL_ACCUM_GREEN_BITS:
      case GL_ACCUM_BLUE_BITS:
      case GL_ACCUM_ALPHA_BITS:
         *params = (GLfloat) ctx->Visual->AccumBits;
         break;
      case GL_ACCUM_CLEAR_VALUE:
         params[0] = ctx->Accum.ClearColor[0];
         params[1] = ctx->Accum.ClearColor[1];
         params[2] = ctx->Accum.ClearColor[2];
         params[3] = ctx->Accum.ClearColor[3];
         break;
      case GL_ALPHA_BIAS:
         *params = ctx->Pixel.AlphaBias;
         break;
      case GL_ALPHA_BITS:
         *params = (GLfloat) ctx->Visual->AlphaBits;
         break;
      case GL_ALPHA_SCALE:
         *params = ctx->Pixel.AlphaScale;
         break;
      case GL_ALPHA_TEST:
         *params = (GLfloat) ctx->Color.AlphaEnabled;
         break;
      case GL_ALPHA_TEST_FUNC:
         *params = ENUM_TO_FLOAT(ctx->Color.AlphaFunc);
         break;
      case GL_ALPHA_TEST_REF:
         *params = (GLfloat) ctx->Color.AlphaRef / 255.0;
         break;
      case GL_ATTRIB_STACK_DEPTH:
         *params = (GLfloat) (ctx->AttribStackDepth);
         break;
      case GL_AUTO_NORMAL:
         *params = (GLfloat) ctx->Eval.AutoNormal;
         break;
      case GL_AUX_BUFFERS:
         *params = (GLfloat) NUM_AUX_BUFFERS;
         break;
      case GL_BLEND:
         *params = (GLfloat) ctx->Color.BlendEnabled;
         break;
      case GL_BLEND_DST:
         *params = ENUM_TO_FLOAT(ctx->Color.BlendDstRGB);
         break;
      case GL_BLEND_SRC:
         *params = ENUM_TO_FLOAT(ctx->Color.BlendSrcRGB);
         break;
      case GL_BLEND_SRC_RGB_INGR:
         *params = ENUM_TO_FLOAT(ctx->Color.BlendSrcRGB);
         break;
      case GL_BLEND_DST_RGB_INGR:
         *params = ENUM_TO_FLOAT(ctx->Color.BlendDstRGB);
         break;
      case GL_BLEND_SRC_ALPHA_INGR:
         *params = ENUM_TO_FLOAT(ctx->Color.BlendSrcA);
         break;
      case GL_BLEND_DST_ALPHA_INGR:
         *params = ENUM_TO_FLOAT(ctx->Color.BlendDstA);
         break;
      case GL_BLEND_EQUATION_EXT:
	 *params = ENUM_TO_FLOAT(ctx->Color.BlendEquation);
	 break;
      case GL_BLEND_COLOR_EXT:
	 params[0] = ctx->Color.BlendColor[0];
	 params[1] = ctx->Color.BlendColor[1];
	 params[2] = ctx->Color.BlendColor[2];
	 params[3] = ctx->Color.BlendColor[3];
	 break;
      case GL_BLUE_BIAS:
         *params = ctx->Pixel.BlueBias;
         break;
      case GL_BLUE_BITS:
         *params = (GLfloat) ctx->Visual->BlueBits;
         break;
      case GL_BLUE_SCALE:
         *params = ctx->Pixel.BlueScale;
         break;
      case GL_CLIENT_ATTRIB_STACK_DEPTH:
         *params = (GLfloat) (ctx->ClientAttribStackDepth);
         break;
      case GL_CLIP_PLANE0:
      case GL_CLIP_PLANE1:
      case GL_CLIP_PLANE2:
      case GL_CLIP_PLANE3:
      case GL_CLIP_PLANE4:
      case GL_CLIP_PLANE5:
         *params = (GLfloat) ctx->Transform.ClipEnabled[pname-GL_CLIP_PLANE0];
         break;
      case GL_COLOR_CLEAR_VALUE:
         params[0] = (GLfloat) ctx->Color.ClearColor[0];
         params[1] = (GLfloat) ctx->Color.ClearColor[1];
         params[2] = (GLfloat) ctx->Color.ClearColor[2];
         params[3] = (GLfloat) ctx->Color.ClearColor[3];
         break;
      case GL_COLOR_MATERIAL:
         *params = (GLfloat) ctx->Light.ColorMaterialEnabled;
         break;
      case GL_COLOR_MATERIAL_FACE:
         *params = ENUM_TO_FLOAT(ctx->Light.ColorMaterialFace);
         break;
      case GL_COLOR_MATERIAL_PARAMETER:
         *params = ENUM_TO_FLOAT(ctx->Light.ColorMaterialMode);
         break;
      case GL_COLOR_WRITEMASK:
         params[0] = ctx->Color.ColorMask[RCOMP] ? 1.0F : 0.0F;
         params[1] = ctx->Color.ColorMask[GCOMP] ? 1.0F : 0.0F;
         params[2] = ctx->Color.ColorMask[BCOMP] ? 1.0F : 0.0F;
         params[3] = ctx->Color.ColorMask[ACOMP] ? 1.0F : 0.0F;
         break;
      case GL_CULL_FACE:
         *params = (GLfloat) ctx->Polygon.CullFlag;
         break;
      case GL_CULL_FACE_MODE:
         *params = ENUM_TO_FLOAT(ctx->Polygon.CullFaceMode);
         break;
      case GL_CURRENT_COLOR:
	 UBYTE_RGBA_TO_FLOAT_RGBA(params, ctx->Current.ByteColor);
         break;
      case GL_CURRENT_INDEX:
         *params = (GLfloat) ctx->Current.Index;
         break;
      case GL_CURRENT_NORMAL:
         params[0] = ctx->Current.Normal[0];
         params[1] = ctx->Current.Normal[1];
         params[2] = ctx->Current.Normal[2];
         break;
      case GL_CURRENT_RASTER_COLOR:
	 params[0] = ctx->Current.RasterColor[0];
	 params[1] = ctx->Current.RasterColor[1];
	 params[2] = ctx->Current.RasterColor[2];
	 params[3] = ctx->Current.RasterColor[3];
	 break;
      case GL_CURRENT_RASTER_DISTANCE:
	 params[0] = ctx->Current.RasterDistance;
	 break;
      case GL_CURRENT_RASTER_INDEX:
	 *params = (GLfloat) ctx->Current.RasterIndex;
	 break;
      case GL_CURRENT_RASTER_POSITION:
	 params[0] = ctx->Current.RasterPos[0];
	 params[1] = ctx->Current.RasterPos[1];
	 params[2] = ctx->Current.RasterPos[2];
	 params[3] = ctx->Current.RasterPos[3];
	 break;
      case GL_CURRENT_RASTER_TEXTURE_COORDS:
	 params[0] = ctx->Current.RasterMultiTexCoord[texTransformUnit][0];
	 params[1] = ctx->Current.RasterMultiTexCoord[texTransformUnit][1];
	 params[2] = ctx->Current.RasterMultiTexCoord[texTransformUnit][2];
	 params[3] = ctx->Current.RasterMultiTexCoord[texTransformUnit][3];
	 break;
      case GL_CURRENT_RASTER_POSITION_VALID:
	 *params = (GLfloat) ctx->Current.RasterPosValid;
	 break;
      case GL_CURRENT_TEXTURE_COORDS:
	 params[0] = (GLfloat) ctx->Current.Texcoord[texTransformUnit][0];
	 params[1] = (GLfloat) ctx->Current.Texcoord[texTransformUnit][1];
	 params[2] = (GLfloat) ctx->Current.Texcoord[texTransformUnit][2];
	 params[3] = (GLfloat) ctx->Current.Texcoord[texTransformUnit][3];
	 break;
      case GL_DEPTH_BIAS:
	 *params = (GLfloat) ctx->Pixel.DepthBias;
	 break;
      case GL_DEPTH_BITS:
	 *params = (GLfloat) ctx->Visual->DepthBits;
	 break;
      case GL_DEPTH_CLEAR_VALUE:
	 *params = (GLfloat) ctx->Depth.Clear;
	 break;
      case GL_DEPTH_FUNC:
	 *params = ENUM_TO_FLOAT(ctx->Depth.Func);
	 break;
      case GL_DEPTH_RANGE:
         params[0] = (GLfloat) ctx->Viewport.Near;
         params[1] = (GLfloat) ctx->Viewport.Far;
	 break;
      case GL_DEPTH_SCALE:
	 *params = (GLfloat) ctx->Pixel.DepthScale;
	 break;
      case GL_DEPTH_TEST:
	 *params = (GLfloat) ctx->Depth.Test;
	 break;
      case GL_DEPTH_WRITEMASK:
	 *params = (GLfloat) ctx->Depth.Mask;
	 break;
      case GL_DITHER:
	 *params = (GLfloat) ctx->Color.DitherFlag;
	 break;
      case GL_DOUBLEBUFFER:
	 *params = (GLfloat) ctx->Visual->DBflag;
	 break;
      case GL_DRAW_BUFFER:
	 *params = ENUM_TO_FLOAT(ctx->Color.DrawBuffer);
	 break;
      case GL_EDGE_FLAG:
	 *params = (GLfloat) ctx->Current.EdgeFlag;
	 break;
      case GL_FEEDBACK_BUFFER_SIZE:
         /* TODO: is this right?  Or, return number of entries in buffer? */
         *params = (GLfloat) ctx->Feedback.BufferSize;
         break;
      case GL_FEEDBACK_BUFFER_TYPE:
         *params = ENUM_TO_FLOAT(ctx->Feedback.Type);
         break;
      case GL_FOG:
	 *params = (GLfloat) ctx->Fog.Enabled;
	 break;
      case GL_FOG_COLOR:
	 params[0] = ctx->Fog.Color[0];
	 params[1] = ctx->Fog.Color[1];
	 params[2] = ctx->Fog.Color[2];
	 params[3] = ctx->Fog.Color[3];
	 break;
      case GL_FOG_DENSITY:
	 *params = ctx->Fog.Density;
	 break;
      case GL_FOG_END:
	 *params = ctx->Fog.End;
	 break;
      case GL_FOG_HINT:
	 *params = ENUM_TO_FLOAT(ctx->Hint.Fog);
	 break;
      case GL_FOG_INDEX:
	 *params = ctx->Fog.Index;
	 break;
      case GL_FOG_MODE:
	 *params = ENUM_TO_FLOAT(ctx->Fog.Mode);
	 break;
      case GL_FOG_START:
	 *params = ctx->Fog.Start;
	 break;
      case GL_FRONT_FACE:
	 *params = ENUM_TO_FLOAT(ctx->Polygon.FrontFace);
	 break;
      case GL_GREEN_BIAS:
         *params = (GLfloat) ctx->Pixel.GreenBias;
         break;
      case GL_GREEN_BITS:
         *params = (GLfloat) ctx->Visual->GreenBits;
         break;
      case GL_GREEN_SCALE:
         *params = (GLfloat) ctx->Pixel.GreenScale;
         break;
      case GL_INDEX_BITS:
         *params = (GLfloat) ctx->Visual->IndexBits;
	 break;
      case GL_INDEX_CLEAR_VALUE:
         *params = (GLfloat) ctx->Color.ClearIndex;
	 break;
      case GL_INDEX_MODE:
	 *params = ctx->Visual->RGBAflag ? 0.0F : 1.0F;
	 break;
      case GL_INDEX_OFFSET:
	 *params = (GLfloat) ctx->Pixel.IndexOffset;
	 break;
      case GL_INDEX_SHIFT:
	 *params = (GLfloat) ctx->Pixel.IndexShift;
	 break;
      case GL_INDEX_WRITEMASK:
	 *params = (GLfloat) ctx->Color.IndexMask;
	 break;
      case GL_LIGHT0:
      case GL_LIGHT1:
      case GL_LIGHT2:
      case GL_LIGHT3:
      case GL_LIGHT4:
      case GL_LIGHT5:
      case GL_LIGHT6:
      case GL_LIGHT7:
	 *params = (GLfloat) ctx->Light.Light[pname-GL_LIGHT0].Enabled;
	 break;
      case GL_LIGHTING:
	 *params = (GLfloat) ctx->Light.Enabled;
	 break;
      case GL_LIGHT_MODEL_AMBIENT:
	 params[0] = ctx->Light.Model.Ambient[0];
	 params[1] = ctx->Light.Model.Ambient[1];
	 params[2] = ctx->Light.Model.Ambient[2];
	 params[3] = ctx->Light.Model.Ambient[3];
	 break;
      case GL_LIGHT_MODEL_COLOR_CONTROL:
         params[0] = ENUM_TO_FLOAT(ctx->Light.Model.ColorControl);
         break;
      case GL_LIGHT_MODEL_LOCAL_VIEWER:
	 *params = (GLfloat) ctx->Light.Model.LocalViewer;
	 break;
      case GL_LIGHT_MODEL_TWO_SIDE:
	 *params = (GLfloat) ctx->Light.Model.TwoSide;
	 break;
      case GL_LINE_SMOOTH:
	 *params = (GLfloat) ctx->Line.SmoothFlag;
	 break;
      case GL_LINE_SMOOTH_HINT:
	 *params = ENUM_TO_FLOAT(ctx->Hint.LineSmooth);
	 break;
      case GL_LINE_STIPPLE:
	 *params = (GLfloat) ctx->Line.StippleFlag;
	 break;
      case GL_LINE_STIPPLE_PATTERN:
         *params = (GLfloat) ctx->Line.StipplePattern;
         break;
      case GL_LINE_STIPPLE_REPEAT:
         *params = (GLfloat) ctx->Line.StippleFactor;
         break;
      case GL_LINE_WIDTH:
	 *params = (GLfloat) ctx->Line.Width;
	 break;
      case GL_LINE_WIDTH_GRANULARITY:
	 *params = (GLfloat) LINE_WIDTH_GRANULARITY;
	 break;
      case GL_LINE_WIDTH_RANGE:
	 params[0] = (GLfloat) MIN_LINE_WIDTH;
	 params[1] = (GLfloat) MAX_LINE_WIDTH;
	 break;
      case GL_LIST_BASE:
	 *params = (GLfloat) ctx->List.ListBase;
	 break;
      case GL_LIST_INDEX:
	 *params = (GLfloat) ctx->CurrentListNum;
	 break;
      case GL_LIST_MODE:
	 *params = ctx->ExecuteFlag ? ENUM_TO_FLOAT(GL_COMPILE_AND_EXECUTE)
	   			  : ENUM_TO_FLOAT(GL_COMPILE);
	 break;
      case GL_INDEX_LOGIC_OP:
	 *params = (GLfloat) ctx->Color.IndexLogicOpEnabled;
	 break;
      case GL_COLOR_LOGIC_OP:
	 *params = (GLfloat) ctx->Color.ColorLogicOpEnabled;
	 break;
      case GL_LOGIC_OP_MODE:
         *params = ENUM_TO_FLOAT(ctx->Color.LogicOp);
	 break;
      case GL_MAP1_COLOR_4:
	 *params = (GLfloat) ctx->Eval.Map1Color4;
	 break;
      case GL_MAP1_GRID_DOMAIN:
	 params[0] = ctx->Eval.MapGrid1u1;
	 params[1] = ctx->Eval.MapGrid1u2;
	 break;
      case GL_MAP1_GRID_SEGMENTS:
	 *params = (GLfloat) ctx->Eval.MapGrid1un;
	 break;
      case GL_MAP1_INDEX:
	 *params = (GLfloat) ctx->Eval.Map1Index;
	 break;
      case GL_MAP1_NORMAL:
	 *params = (GLfloat) ctx->Eval.Map1Normal;
	 break;
      case GL_MAP1_TEXTURE_COORD_1:
	 *params = (GLfloat) ctx->Eval.Map1TextureCoord1;
	 break;
      case GL_MAP1_TEXTURE_COORD_2:
	 *params = (GLfloat) ctx->Eval.Map1TextureCoord2;
	 break;
      case GL_MAP1_TEXTURE_COORD_3:
	 *params = (GLfloat) ctx->Eval.Map1TextureCoord3;
	 break;
      case GL_MAP1_TEXTURE_COORD_4:
	 *params = (GLfloat) ctx->Eval.Map1TextureCoord4;
	 break;
      case GL_MAP1_VERTEX_3:
	 *params = (GLfloat) ctx->Eval.Map1Vertex3;
	 break;
      case GL_MAP1_VERTEX_4:
	 *params = (GLfloat) ctx->Eval.Map1Vertex4;
	 break;
      case GL_MAP2_COLOR_4:
	 *params = (GLfloat) ctx->Eval.Map2Color4;
	 break;
      case GL_MAP2_GRID_DOMAIN:
	 params[0] = ctx->Eval.MapGrid2u1;
	 params[1] = ctx->Eval.MapGrid2u2;
	 params[2] = ctx->Eval.MapGrid2v1;
	 params[3] = ctx->Eval.MapGrid2v2;
	 break;
      case GL_MAP2_GRID_SEGMENTS:
	 params[0] = (GLfloat) ctx->Eval.MapGrid2un;
	 params[1] = (GLfloat) ctx->Eval.MapGrid2vn;
	 break;
      case GL_MAP2_INDEX:
	 *params = (GLfloat) ctx->Eval.Map2Index;
	 break;
      case GL_MAP2_NORMAL:
	 *params = (GLfloat) ctx->Eval.Map2Normal;
	 break;
      case GL_MAP2_TEXTURE_COORD_1:
	 *params = ctx->Eval.Map2TextureCoord1;
	 break;
      case GL_MAP2_TEXTURE_COORD_2:
	 *params = ctx->Eval.Map2TextureCoord2;
	 break;
      case GL_MAP2_TEXTURE_COORD_3:
	 *params = ctx->Eval.Map2TextureCoord3;
	 break;
      case GL_MAP2_TEXTURE_COORD_4:
	 *params = ctx->Eval.Map2TextureCoord4;
	 break;
      case GL_MAP2_VERTEX_3:
	 *params = (GLfloat) ctx->Eval.Map2Vertex3;
	 break;
      case GL_MAP2_VERTEX_4:
	 *params = (GLfloat) ctx->Eval.Map2Vertex4;
	 break;
      case GL_MAP_COLOR:
	 *params = (GLfloat) ctx->Pixel.MapColorFlag;
	 break;
      case GL_MAP_STENCIL:
	 *params = (GLfloat) ctx->Pixel.MapStencilFlag;
	 break;
      case GL_MATRIX_MODE:
	 *params = ENUM_TO_FLOAT(ctx->Transform.MatrixMode);
	 break;
      case GL_MAX_ATTRIB_STACK_DEPTH:
	 *params = (GLfloat) MAX_ATTRIB_STACK_DEPTH;
	 break;
      case GL_MAX_CLIENT_ATTRIB_STACK_DEPTH:
         *params = (GLfloat) MAX_CLIENT_ATTRIB_STACK_DEPTH;
         break;
      case GL_MAX_CLIP_PLANES:
	 *params = (GLfloat) MAX_CLIP_PLANES;
	 break;
      case GL_MAX_ELEMENTS_VERTICES:  /* GL_VERSION_1_2 */
         *params = (GLfloat) VB_MAX;
         break;
      case GL_MAX_ELEMENTS_INDICES:   /* GL_VERSION_1_2 */
         *params = (GLfloat) VB_MAX;
         break;
      case GL_MAX_EVAL_ORDER:
	 *params = (GLfloat) MAX_EVAL_ORDER;
	 break;
      case GL_MAX_LIGHTS:
	 *params = (GLfloat) MAX_LIGHTS;
	 break;
      case GL_MAX_LIST_NESTING:
	 *params = (GLfloat) MAX_LIST_NESTING;
	 break;
      case GL_MAX_MODELVIEW_STACK_DEPTH:
	 *params = (GLfloat) MAX_MODELVIEW_STACK_DEPTH;
	 break;
      case GL_MAX_NAME_STACK_DEPTH:
	 *params = (GLfloat) MAX_NAME_STACK_DEPTH;
	 break;
      case GL_MAX_PIXEL_MAP_TABLE:
	 *params = (GLfloat) MAX_PIXEL_MAP_TABLE;
	 break;
      case GL_MAX_PROJECTION_STACK_DEPTH:
	 *params = (GLfloat) MAX_PROJECTION_STACK_DEPTH;
	 break;
      case GL_MAX_TEXTURE_SIZE:
      case GL_MAX_3D_TEXTURE_SIZE:
         *params = (GLfloat) ctx->Const.MaxTextureSize;
	 break;
      case GL_MAX_TEXTURE_STACK_DEPTH:
	 *params = (GLfloat) MAX_TEXTURE_STACK_DEPTH;
	 break;
      case GL_MAX_VIEWPORT_DIMS:
         params[0] = (GLfloat) MAX_WIDTH;
         params[1] = (GLfloat) MAX_HEIGHT;
         break;
      case GL_MODELVIEW_MATRIX:
	 for (i=0;i<16;i++) {
	    params[i] = ctx->ModelView.m[i];
	 }
	 break;
      case GL_MODELVIEW_STACK_DEPTH:
	 *params = (GLfloat) (ctx->ModelViewStackDepth + 1);
	 break;
      case GL_NAME_STACK_DEPTH:
	 *params = (GLfloat) ctx->Select.NameStackDepth;
	 break;
      case GL_NORMALIZE:
	 *params = (GLfloat) ctx->Transform.Normalize;
	 break;
      case GL_PACK_ALIGNMENT:
	 *params = (GLfloat) ctx->Pack.Alignment;
	 break;
      case GL_PACK_LSB_FIRST:
	 *params = (GLfloat) ctx->Pack.LsbFirst;
	 break;
      case GL_PACK_ROW_LENGTH:
	 *params = (GLfloat) ctx->Pack.RowLength;
	 break;
      case GL_PACK_SKIP_PIXELS:
	 *params = (GLfloat) ctx->Pack.SkipPixels;
	 break;
      case GL_PACK_SKIP_ROWS:
	 *params = (GLfloat) ctx->Pack.SkipRows;
	 break;
      case GL_PACK_SWAP_BYTES:
	 *params = (GLfloat) ctx->Pack.SwapBytes;
	 break;
      case GL_PACK_SKIP_IMAGES_EXT:
         *params = (GLfloat) ctx->Pack.SkipImages;
         break;
      case GL_PACK_IMAGE_HEIGHT_EXT:
         *params = (GLfloat) ctx->Pack.ImageHeight;
         break;
      case GL_PERSPECTIVE_CORRECTION_HINT:
	 *params = ENUM_TO_FLOAT(ctx->Hint.PerspectiveCorrection);
	 break;
      case GL_PIXEL_MAP_A_TO_A_SIZE:
	 *params = (GLfloat) ctx->Pixel.MapAtoAsize;
	 break;
      case GL_PIXEL_MAP_B_TO_B_SIZE:
	 *params = (GLfloat) ctx->Pixel.MapBtoBsize;
	 break;
      case GL_PIXEL_MAP_G_TO_G_SIZE:
	 *params = (GLfloat) ctx->Pixel.MapGtoGsize;
	 break;
      case GL_PIXEL_MAP_I_TO_A_SIZE:
	 *params = (GLfloat) ctx->Pixel.MapItoAsize;
	 break;
      case GL_PIXEL_MAP_I_TO_B_SIZE:
	 *params = (GLfloat) ctx->Pixel.MapItoBsize;
	 break;
      case GL_PIXEL_MAP_I_TO_G_SIZE:
	 *params = (GLfloat) ctx->Pixel.MapItoGsize;
	 break;
      case GL_PIXEL_MAP_I_TO_I_SIZE:
	 *params = (GLfloat) ctx->Pixel.MapItoIsize;
	 break;
      case GL_PIXEL_MAP_I_TO_R_SIZE:
	 *params = (GLfloat) ctx->Pixel.MapItoRsize;
	 break;
      case GL_PIXEL_MAP_R_TO_R_SIZE:
	 *params = (GLfloat) ctx->Pixel.MapRtoRsize;
	 break;
      case GL_PIXEL_MAP_S_TO_S_SIZE:
	 *params = (GLfloat) ctx->Pixel.MapStoSsize;
	 break;
      case GL_POINT_SIZE:
         *params = (GLfloat) ctx->Point.Size;
         break;
      case GL_POINT_SIZE_GRANULARITY:
	 *params = (GLfloat) POINT_SIZE_GRANULARITY;
	 break;
      case GL_POINT_SIZE_RANGE:
	 params[0] = (GLfloat) MIN_POINT_SIZE;
	 params[1] = (GLfloat) MAX_POINT_SIZE;
	 break;
      case GL_POINT_SMOOTH:
	 *params = (GLfloat) ctx->Point.SmoothFlag;
	 break;
      case GL_POINT_SMOOTH_HINT:
	 *params = ENUM_TO_FLOAT(ctx->Hint.PointSmooth);
	 break;
      case GL_POINT_SIZE_MIN_EXT:
	 *params = (GLfloat) (ctx->Point.MinSize);
	 break;
      case GL_POINT_SIZE_MAX_EXT:
	 *params = (GLfloat) (ctx->Point.MaxSize);
	 break;
      case GL_POINT_FADE_THRESHOLD_SIZE_EXT:
	 *params = (GLfloat) (ctx->Point.Threshold);
	 break;
      case GL_DISTANCE_ATTENUATION_EXT:
	 params[0] = (GLfloat) (ctx->Point.Params[0]);
	 params[1] = (GLfloat) (ctx->Point.Params[1]);
	 params[2] = (GLfloat) (ctx->Point.Params[2]);
	 break;
      case GL_POLYGON_MODE:
	 params[0] = ENUM_TO_FLOAT(ctx->Polygon.FrontMode);
	 params[1] = ENUM_TO_FLOAT(ctx->Polygon.BackMode);
	 break;
#ifdef GL_EXT_polygon_offset
      case GL_POLYGON_OFFSET_BIAS_EXT:
         *params = ctx->Polygon.OffsetUnits;
         break;
#endif
      case GL_POLYGON_OFFSET_FACTOR:
         *params = ctx->Polygon.OffsetFactor;
         break;
      case GL_POLYGON_OFFSET_UNITS:
         *params = ctx->Polygon.OffsetUnits;
         break;
      case GL_POLYGON_SMOOTH:
	 *params = (GLfloat) ctx->Polygon.SmoothFlag;
	 break;
      case GL_POLYGON_SMOOTH_HINT:
	 *params = ENUM_TO_FLOAT(ctx->Hint.PolygonSmooth);
	 break;
      case GL_POLYGON_STIPPLE:
         *params = (GLfloat) ctx->Polygon.StippleFlag;
	 break;
      case GL_PROJECTION_MATRIX:
	 for (i=0;i<16;i++) {
	    params[i] = ctx->ProjectionMatrix.m[i];
	 }
	 break;
      case GL_PROJECTION_STACK_DEPTH:
	 *params = (GLfloat) (ctx->ProjectionStackDepth + 1);
	 break;
      case GL_READ_BUFFER:
	 *params = ENUM_TO_FLOAT(ctx->Pixel.ReadBuffer);
	 break;
      case GL_RED_BIAS:
         *params = ctx->Pixel.RedBias;
         break;
      case GL_RED_BITS:
         *params = (GLfloat) ctx->Visual->RedBits;
         break;
      case GL_RED_SCALE:
         *params = ctx->Pixel.RedScale;
         break;
      case GL_RENDER_MODE:
	 *params = ENUM_TO_FLOAT(ctx->RenderMode);
	 break;
      case GL_RGBA_MODE:
	 *params = (GLfloat) ctx->Visual->RGBAflag;
	 break;
      case GL_SCISSOR_BOX:
	 params[0] = (GLfloat) ctx->Scissor.X;
	 params[1] = (GLfloat) ctx->Scissor.Y;
	 params[2] = (GLfloat) ctx->Scissor.Width;
	 params[3] = (GLfloat) ctx->Scissor.Height;
	 break;
      case GL_SCISSOR_TEST:
	 *params = (GLfloat) ctx->Scissor.Enabled;
	 break;
      case GL_SELECTION_BUFFER_SIZE:
         *params = (GLfloat) ctx->Select.BufferSize;
         break;
      case GL_SHADE_MODEL:
	 *params = ENUM_TO_FLOAT(ctx->Light.ShadeModel);
	 break;
      case GL_SHARED_TEXTURE_PALETTE_EXT:
         *params = (GLfloat) ctx->Texture.SharedPalette;
         break;
      case GL_STENCIL_BITS:
         *params = (GLfloat) ctx->Visual->StencilBits;
         break;
      case GL_STENCIL_CLEAR_VALUE:
	 *params = (GLfloat) ctx->Stencil.Clear;
	 break;
      case GL_STENCIL_FAIL:
	 *params = ENUM_TO_FLOAT(ctx->Stencil.FailFunc);
	 break;
      case GL_STENCIL_FUNC:
	 *params = ENUM_TO_FLOAT(ctx->Stencil.Function);
	 break;
      case GL_STENCIL_PASS_DEPTH_FAIL:
	 *params = ENUM_TO_FLOAT(ctx->Stencil.ZFailFunc);
	 break;
      case GL_STENCIL_PASS_DEPTH_PASS:
	 *params = ENUM_TO_FLOAT(ctx->Stencil.ZPassFunc);
	 break;
      case GL_STENCIL_REF:
	 *params = (GLfloat) ctx->Stencil.Ref;
	 break;
      case GL_STENCIL_TEST:
	 *params = (GLfloat) ctx->Stencil.Enabled;
	 break;
      case GL_STENCIL_VALUE_MASK:
	 *params = (GLfloat) ctx->Stencil.ValueMask;
	 break;
      case GL_STENCIL_WRITEMASK:
	 *params = (GLfloat) ctx->Stencil.WriteMask;
	 break;
      case GL_STEREO:
	 *params = (GLfloat) ctx->Visual->StereoFlag;
	 break;
      case GL_SUBPIXEL_BITS:
	 *params = 0.0F;  /* TODO */
	 break;
      case GL_TEXTURE_1D:
         *params = gl_IsEnabled(ctx, GL_TEXTURE_1D) ? 1.0 : 0.0;
	 break;
      case GL_TEXTURE_2D:
         *params = gl_IsEnabled(ctx, GL_TEXTURE_2D) ? 1.0 : 0.0;
	 break;
      case GL_TEXTURE_3D:
         *params = gl_IsEnabled(ctx, GL_TEXTURE_3D) ? 1.0 : 0.0;
	 break;
      case GL_TEXTURE_BINDING_1D:
         *params = (GLfloat) textureUnit->CurrentD[1]->Name;
          break;
      case GL_TEXTURE_BINDING_2D:
         *params = (GLfloat) textureUnit->CurrentD[2]->Name;
          break;
      case GL_TEXTURE_BINDING_3D:
         *params = (GLfloat) textureUnit->CurrentD[2]->Name;
          break;
      case GL_TEXTURE_ENV_COLOR:
	 params[0] = textureUnit->EnvColor[0];
	 params[1] = textureUnit->EnvColor[1];
	 params[2] = textureUnit->EnvColor[2];
	 params[3] = textureUnit->EnvColor[3];
	 break;
      case GL_TEXTURE_ENV_MODE:
	 *params = ENUM_TO_FLOAT(textureUnit->EnvMode);
	 break;
      case GL_TEXTURE_GEN_S:
	 *params = (textureUnit->TexGenEnabled & S_BIT) ? 1.0 : 0.0;
	 break;
      case GL_TEXTURE_GEN_T:
	 *params = (textureUnit->TexGenEnabled & T_BIT) ? 1.0 : 0.0;
	 break;
      case GL_TEXTURE_GEN_R:
	 *params = (textureUnit->TexGenEnabled & R_BIT) ? 1.0 : 0.0;
	 break;
      case GL_TEXTURE_GEN_Q:
	 *params = (textureUnit->TexGenEnabled & Q_BIT) ? 1.0 : 0.0;
	 break;
      case GL_TEXTURE_MATRIX:
         for (i=0;i<16;i++) {
	    params[i] = ctx->TextureMatrix[texTransformUnit].m[i];
	 }
	 break;
      case GL_TEXTURE_STACK_DEPTH:
	 *params = (GLfloat) (ctx->TextureStackDepth[texTransformUnit] + 1);
	 break;
      case GL_UNPACK_ALIGNMENT:
	 *params = (GLfloat) ctx->Unpack.Alignment;
	 break;
      case GL_UNPACK_LSB_FIRST:
	 *params = (GLfloat) ctx->Unpack.LsbFirst;
	 break;
      case GL_UNPACK_ROW_LENGTH:
	 *params = (GLfloat) ctx->Unpack.RowLength;
	 break;
      case GL_UNPACK_SKIP_PIXELS:
	 *params = (GLfloat) ctx->Unpack.SkipPixels;
	 break;
      case GL_UNPACK_SKIP_ROWS:
	 *params = (GLfloat) ctx->Unpack.SkipRows;
	 break;
      case GL_UNPACK_SWAP_BYTES:
	 *params = (GLfloat) ctx->Unpack.SwapBytes;
	 break;
      case GL_UNPACK_SKIP_IMAGES_EXT:
         *params = (GLfloat) ctx->Unpack.SkipImages;
         break;
      case GL_UNPACK_IMAGE_HEIGHT_EXT:
         *params = (GLfloat) ctx->Unpack.ImageHeight;
         break;
      case GL_VIEWPORT:
	 params[0] = (GLfloat) ctx->Viewport.X;
	 params[1] = (GLfloat) ctx->Viewport.Y;
	 params[2] = (GLfloat) ctx->Viewport.Width;
	 params[3] = (GLfloat) ctx->Viewport.Height;
	 break;
      case GL_ZOOM_X:
	 *params = (GLfloat) ctx->Pixel.ZoomX;
	 break;
      case GL_ZOOM_Y:
	 *params = (GLfloat) ctx->Pixel.ZoomY;
	 break;
      case GL_VERTEX_ARRAY_SIZE:
         *params = (GLfloat) ctx->Array.Vertex.Size;
         break;
      case GL_VERTEX_ARRAY_TYPE:
         *params = ENUM_TO_FLOAT(ctx->Array.Vertex.Type);
         break;
      case GL_VERTEX_ARRAY_STRIDE:
         *params = (GLfloat) ctx->Array.Vertex.Stride;
         break;
      case GL_VERTEX_ARRAY_COUNT_EXT:
         *params = 0.0;
         break;
      case GL_NORMAL_ARRAY_TYPE:
         *params = ENUM_TO_FLOAT(ctx->Array.Normal.Type);
         break;
      case GL_NORMAL_ARRAY_STRIDE:
         *params = (GLfloat) ctx->Array.Normal.Stride;
         break;
      case GL_NORMAL_ARRAY_COUNT_EXT:
         *params = 0.0;
         break;
      case GL_COLOR_ARRAY_SIZE:
         *params = (GLfloat) ctx->Array.Color.Size;
         break;
      case GL_COLOR_ARRAY_TYPE:
         *params = ENUM_TO_FLOAT(ctx->Array.Color.Type);
         break;
      case GL_COLOR_ARRAY_STRIDE:
         *params = (GLfloat) ctx->Array.Color.Stride;
         break;
      case GL_COLOR_ARRAY_COUNT_EXT:
         *params = 0.0;
         break;
      case GL_INDEX_ARRAY_TYPE:
         *params = ENUM_TO_FLOAT(ctx->Array.Index.Type);
         break;
      case GL_INDEX_ARRAY_STRIDE:
         *params = (GLfloat) ctx->Array.Index.Stride;
         break;
      case GL_INDEX_ARRAY_COUNT_EXT:
         *params = 0.0;
         break;
      case GL_TEXTURE_COORD_ARRAY_SIZE:
         *params = (GLfloat) ctx->Array.TexCoord[texUnit].Size;
         break;
      case GL_TEXTURE_COORD_ARRAY_TYPE:
         *params = ENUM_TO_FLOAT(ctx->Array.TexCoord[texUnit].Type);
         break;
      case GL_TEXTURE_COORD_ARRAY_STRIDE:
         *params = (GLfloat) ctx->Array.TexCoord[texUnit].Stride;
         break;
      case GL_TEXTURE_COORD_ARRAY_COUNT_EXT:
         *params = 0.0;
         break;
      case GL_EDGE_FLAG_ARRAY_STRIDE:
         *params = (GLfloat) ctx->Array.EdgeFlag.Stride;
         break;
      case GL_EDGE_FLAG_ARRAY_COUNT_EXT:
         *params = 0.0;
         break;

      case GL_MAX_TEXTURE_UNITS_ARB:
         *params = (GLfloat) ctx->Const.MaxTextureUnits;
         break;
      case GL_ACTIVE_TEXTURE_ARB:
         *params = (GLfloat) (GL_TEXTURE0_ARB + ctx->Texture.CurrentUnit);
         break;
      case GL_CLIENT_ACTIVE_TEXTURE_ARB:
         *params = (GLfloat) (GL_TEXTURE0_ARB + ctx->Array.ActiveTexture);
         break;

      /* GL_PGI_misc_hints */
      case GL_STRICT_DEPTHFUNC_HINT_PGI:
	 *params = ENUM_TO_FLOAT(GL_NICEST);
         break;
      case GL_STRICT_LIGHTING_HINT_PGI:
	 *params = ENUM_TO_FLOAT(ctx->Hint.StrictLighting);
	 break;
      case GL_STRICT_SCISSOR_HINT_PGI:
      case GL_FULL_STIPPLE_HINT_PGI:
	 *params = ENUM_TO_FLOAT(GL_TRUE);
	 break;
      case GL_CONSERVE_MEMORY_HINT_PGI:
	 *params = ENUM_TO_FLOAT(GL_FALSE);
	 break;
      case GL_ALWAYS_FAST_HINT_PGI:
	 *params = (GLfloat) (ctx->Hint.AllowDrawWin == GL_TRUE &&
			      ctx->Hint.AllowDrawSpn == GL_FALSE && 
			      ctx->Hint.AllowDrawMem == GL_FALSE);
	 break;
      case GL_ALWAYS_SOFT_HINT_PGI:
	 *params = (GLfloat) (ctx->Hint.AllowDrawWin == GL_TRUE &&
			      ctx->Hint.AllowDrawSpn == GL_TRUE && 
			      ctx->Hint.AllowDrawMem == GL_TRUE);
	 break;
      case GL_ALLOW_DRAW_OBJ_HINT_PGI:
	 *params = (GLfloat) GL_TRUE;
	 break;
      case GL_ALLOW_DRAW_WIN_HINT_PGI:
	 *params = (GLfloat) ctx->Hint.AllowDrawWin;
	 break;
      case GL_ALLOW_DRAW_SPN_HINT_PGI:
	 *params = (GLfloat) ctx->Hint.AllowDrawSpn;
	 break;
      case GL_ALLOW_DRAW_MEM_HINT_PGI:
	 *params = (GLfloat) ctx->Hint.AllowDrawMem;
	 break;
      case GL_CLIP_NEAR_HINT_PGI:
      case GL_CLIP_FAR_HINT_PGI:
	 *params = ENUM_TO_FLOAT(GL_TRUE);
	 break;
      case GL_WIDE_LINE_HINT_PGI:
	 *params = ENUM_TO_FLOAT(GL_DONT_CARE);
	 break;
      case GL_BACK_NORMALS_HINT_PGI:
	 *params = ENUM_TO_FLOAT(GL_TRUE);
	 break;
      case GL_NATIVE_GRAPHICS_HANDLE_PGI:
	 *params = 0;
	 break;

      default:
	 printf("invalid enum: %x\n", pname);
         gl_error( ctx, GL_INVALID_ENUM, "glGetFloatv" );
   }
}




void gl_GetIntegerv( GLcontext *ctx, GLenum pname, GLint *params )
{
   GLuint i;
   GLuint texUnit = ctx->Texture.CurrentUnit;
   GLuint texTransformUnit = ctx->Texture.CurrentTransformUnit;
   const struct gl_texture_unit *textureUnit = &ctx->Texture.Unit[texUnit];

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glGetIntegerv");

   if (MESA_VERBOSE & VERBOSE_API) 
      fprintf(stderr, "glGetIntegerv %s\n", gl_lookup_enum_by_nr(pname));

   switch (pname) {
      case GL_ACCUM_RED_BITS:
      case GL_ACCUM_GREEN_BITS:
      case GL_ACCUM_BLUE_BITS:
      case GL_ACCUM_ALPHA_BITS:
         *params = (GLint) ctx->Visual->AccumBits;
         break;
      case GL_ACCUM_CLEAR_VALUE:
         params[0] = FLOAT_TO_INT( ctx->Accum.ClearColor[0] );
         params[1] = FLOAT_TO_INT( ctx->Accum.ClearColor[1] );
         params[2] = FLOAT_TO_INT( ctx->Accum.ClearColor[2] );
         params[3] = FLOAT_TO_INT( ctx->Accum.ClearColor[3] );
         break;
      case GL_ALPHA_BIAS:
         *params = (GLint) ctx->Pixel.AlphaBias;
         break;
      case GL_ALPHA_BITS:
         *params = ctx->Visual->AlphaBits;
         break;
      case GL_ALPHA_SCALE:
         *params = (GLint) ctx->Pixel.AlphaScale;
         break;
      case GL_ALPHA_TEST:
         *params = (GLint) ctx->Color.AlphaEnabled;
         break;
      case GL_ALPHA_TEST_REF:
         *params = FLOAT_TO_INT( (GLfloat) ctx->Color.AlphaRef / 255.0 );
         break;
      case GL_ALPHA_TEST_FUNC:
         *params = (GLint) ctx->Color.AlphaFunc;
         break;
      case GL_ATTRIB_STACK_DEPTH:
         *params = (GLint) (ctx->AttribStackDepth);
         break;
      case GL_AUTO_NORMAL:
         *params = (GLint) ctx->Eval.AutoNormal;
         break;
      case GL_AUX_BUFFERS:
         *params = (GLint) NUM_AUX_BUFFERS;
         break;
      case GL_BLEND:
         *params = (GLint) ctx->Color.BlendEnabled;
         break;
      case GL_BLEND_DST:
         *params = (GLint) ctx->Color.BlendDstRGB;
         break;
      case GL_BLEND_SRC:
         *params = (GLint) ctx->Color.BlendSrcRGB;
         break;
      case GL_BLEND_SRC_RGB_INGR:
         *params = (GLint) ctx->Color.BlendSrcRGB;
         break;
      case GL_BLEND_DST_RGB_INGR:
         *params = (GLint) ctx->Color.BlendDstRGB;
         break;
      case GL_BLEND_SRC_ALPHA_INGR:
         *params = (GLint) ctx->Color.BlendSrcA;
         break;
      case GL_BLEND_DST_ALPHA_INGR:
         *params = (GLint) ctx->Color.BlendDstA;
         break;
      case GL_BLEND_EQUATION_EXT:
	 *params = (GLint) ctx->Color.BlendEquation;
	 break;
      case GL_BLEND_COLOR_EXT:
	 params[0] = FLOAT_TO_INT( ctx->Color.BlendColor[0] );
	 params[1] = FLOAT_TO_INT( ctx->Color.BlendColor[1] );
	 params[2] = FLOAT_TO_INT( ctx->Color.BlendColor[2] );
	 params[3] = FLOAT_TO_INT( ctx->Color.BlendColor[3] );
	 break;
      case GL_BLUE_BIAS:
         *params = (GLint) ctx->Pixel.BlueBias;
         break;
      case GL_BLUE_BITS:
         *params = (GLint) ctx->Visual->BlueBits;
         break;
      case GL_BLUE_SCALE:
         *params = (GLint) ctx->Pixel.BlueScale;
         break;
      case GL_CLIENT_ATTRIB_STACK_DEPTH:
         *params = (GLint) (ctx->ClientAttribStackDepth);
         break;
      case GL_CLIP_PLANE0:
      case GL_CLIP_PLANE1:
      case GL_CLIP_PLANE2:
      case GL_CLIP_PLANE3:
      case GL_CLIP_PLANE4:
      case GL_CLIP_PLANE5:
         i = (GLint) (pname - GL_CLIP_PLANE0);
         *params = (GLint) ctx->Transform.ClipEnabled[i];
         break;
      case GL_COLOR_CLEAR_VALUE:
         params[0] = FLOAT_TO_INT( ctx->Color.ClearColor[0] );
         params[1] = FLOAT_TO_INT( ctx->Color.ClearColor[1] );
         params[2] = FLOAT_TO_INT( ctx->Color.ClearColor[2] );
         params[3] = FLOAT_TO_INT( ctx->Color.ClearColor[3] );
         break;
      case GL_COLOR_MATERIAL:
         *params = (GLint) ctx->Light.ColorMaterialEnabled;
         break;
      case GL_COLOR_MATERIAL_FACE:
         *params = (GLint) ctx->Light.ColorMaterialFace;
         break;
      case GL_COLOR_MATERIAL_PARAMETER:
         *params = (GLint) ctx->Light.ColorMaterialMode;
         break;
      case GL_COLOR_WRITEMASK:
         params[0] = ctx->Color.ColorMask[RCOMP] ? 1 : 0;
         params[1] = ctx->Color.ColorMask[GCOMP] ? 1 : 0;
         params[2] = ctx->Color.ColorMask[BCOMP] ? 1 : 0;
         params[3] = ctx->Color.ColorMask[ACOMP] ? 1 : 0;
         break;
      case GL_CULL_FACE:
         *params = (GLint) ctx->Polygon.CullFlag;
         break;
      case GL_CULL_FACE_MODE:
         *params = (GLint) ctx->Polygon.CullFaceMode;
         break;
      case GL_CURRENT_COLOR:
         params[0] = FLOAT_TO_INT( UBYTE_COLOR_TO_FLOAT_COLOR( ctx->Current.ByteColor[0] ) );
         params[1] = FLOAT_TO_INT( UBYTE_COLOR_TO_FLOAT_COLOR( ctx->Current.ByteColor[1] ) );
         params[2] = FLOAT_TO_INT( UBYTE_COLOR_TO_FLOAT_COLOR( ctx->Current.ByteColor[2] ) );
         params[3] = FLOAT_TO_INT( UBYTE_COLOR_TO_FLOAT_COLOR( ctx->Current.ByteColor[3] ) );
         break;
      case GL_CURRENT_INDEX:
         *params = (GLint) ctx->Current.Index;
         break;
      case GL_CURRENT_NORMAL:
         params[0] = FLOAT_TO_INT( ctx->Current.Normal[0] );
         params[1] = FLOAT_TO_INT( ctx->Current.Normal[1] );
         params[2] = FLOAT_TO_INT( ctx->Current.Normal[2] );
         break;
      case GL_CURRENT_RASTER_COLOR:
	 params[0] = FLOAT_TO_INT( ctx->Current.RasterColor[0] );
	 params[1] = FLOAT_TO_INT( ctx->Current.RasterColor[1] );
	 params[2] = FLOAT_TO_INT( ctx->Current.RasterColor[2] );
	 params[3] = FLOAT_TO_INT( ctx->Current.RasterColor[3] );
	 break;
      case GL_CURRENT_RASTER_DISTANCE:
	 params[0] = (GLint) ctx->Current.RasterDistance;
	 break;
      case GL_CURRENT_RASTER_INDEX:
	 *params = (GLint) ctx->Current.RasterIndex;
	 break;
      case GL_CURRENT_RASTER_POSITION:
	 params[0] = (GLint) ctx->Current.RasterPos[0];
	 params[1] = (GLint) ctx->Current.RasterPos[1];
	 params[2] = (GLint) ctx->Current.RasterPos[2];
	 params[3] = (GLint) ctx->Current.RasterPos[3];
	 break;
      case GL_CURRENT_RASTER_TEXTURE_COORDS:
	 params[0] = (GLint) ctx->Current.RasterMultiTexCoord[texTransformUnit][0];
	 params[1] = (GLint) ctx->Current.RasterMultiTexCoord[texTransformUnit][1];
	 params[2] = (GLint) ctx->Current.RasterMultiTexCoord[texTransformUnit][2];
	 params[3] = (GLint) ctx->Current.RasterMultiTexCoord[texTransformUnit][3];
	 break;
      case GL_CURRENT_RASTER_POSITION_VALID:
	 *params = (GLint) ctx->Current.RasterPosValid;
	 break;
      case GL_CURRENT_TEXTURE_COORDS:
         params[0] = (GLint) ctx->Current.Texcoord[texTransformUnit][0];
         params[1] = (GLint) ctx->Current.Texcoord[texTransformUnit][1];
         params[2] = (GLint) ctx->Current.Texcoord[texTransformUnit][2];
         params[3] = (GLint) ctx->Current.Texcoord[texTransformUnit][3];
	 break;
      case GL_DEPTH_BIAS:
         *params = (GLint) ctx->Pixel.DepthBias;
	 break;
      case GL_DEPTH_BITS:
	 *params = ctx->Visual->DepthBits;
	 break;
      case GL_DEPTH_CLEAR_VALUE:
         *params = (GLint) ctx->Depth.Clear;
	 break;
      case GL_DEPTH_FUNC:
         *params = (GLint) ctx->Depth.Func;
	 break;
      case GL_DEPTH_RANGE:
         params[0] = (GLint) ctx->Viewport.Near;
         params[1] = (GLint) ctx->Viewport.Far;
	 break;
      case GL_DEPTH_SCALE:
         *params = (GLint) ctx->Pixel.DepthScale;
	 break;
      case GL_DEPTH_TEST:
         *params = (GLint) ctx->Depth.Test;
	 break;
      case GL_DEPTH_WRITEMASK:
	 *params = (GLint) ctx->Depth.Mask;
	 break;
      case GL_DITHER:
	 *params = (GLint) ctx->Color.DitherFlag;
	 break;
      case GL_DOUBLEBUFFER:
	 *params = (GLint) ctx->Visual->DBflag;
	 break;
      case GL_DRAW_BUFFER:
	 *params = (GLint) ctx->Color.DrawBuffer;
	 break;
      case GL_EDGE_FLAG:
	 *params = (GLint) ctx->Current.EdgeFlag;
	 break;
      case GL_FEEDBACK_BUFFER_SIZE:
         /* TODO: is this right?  Or, return number of entries in buffer? */
         *params = ctx->Feedback.BufferSize;
         break;
      case GL_FEEDBACK_BUFFER_TYPE:
         *params = ctx->Feedback.Type;
         break;
      case GL_FOG:
	 *params = (GLint) ctx->Fog.Enabled;
	 break;
      case GL_FOG_COLOR:
	 params[0] = FLOAT_TO_INT( ctx->Fog.Color[0] );
	 params[1] = FLOAT_TO_INT( ctx->Fog.Color[1] );
	 params[2] = FLOAT_TO_INT( ctx->Fog.Color[2] );
	 params[3] = FLOAT_TO_INT( ctx->Fog.Color[3] );
	 break;
      case GL_FOG_DENSITY:
	 *params = (GLint) ctx->Fog.Density;
	 break;
      case GL_FOG_END:
	 *params = (GLint) ctx->Fog.End;
	 break;
      case GL_FOG_HINT:
	 *params = (GLint) ctx->Hint.Fog;
	 break;
      case GL_FOG_INDEX:
	 *params = (GLint) ctx->Fog.Index;
	 break;
      case GL_FOG_MODE:
	 *params = (GLint) ctx->Fog.Mode;
	 break;
      case GL_FOG_START:
	 *params = (GLint) ctx->Fog.Start;
	 break;
      case GL_FRONT_FACE:
	 *params = (GLint) ctx->Polygon.FrontFace;
	 break;
      case GL_GREEN_BIAS:
         *params = (GLint) ctx->Pixel.GreenBias;
         break;
      case GL_GREEN_BITS:
         *params = (GLint) ctx->Visual->GreenBits;
         break;
      case GL_GREEN_SCALE:
         *params = (GLint) ctx->Pixel.GreenScale;
         break;
      case GL_INDEX_BITS:
         *params = (GLint) ctx->Visual->IndexBits;
         break;
      case GL_INDEX_CLEAR_VALUE:
         *params = (GLint) ctx->Color.ClearIndex;
         break;
      case GL_INDEX_MODE:
	 *params = ctx->Visual->RGBAflag ? 0 : 1;
	 break;
      case GL_INDEX_OFFSET:
	 *params = ctx->Pixel.IndexOffset;
	 break;
      case GL_INDEX_SHIFT:
	 *params = ctx->Pixel.IndexShift;
	 break;
      case GL_INDEX_WRITEMASK:
	 *params = (GLint) ctx->Color.IndexMask;
	 break;
      case GL_LIGHT0:
      case GL_LIGHT1:
      case GL_LIGHT2:
      case GL_LIGHT3:
      case GL_LIGHT4:
      case GL_LIGHT5:
      case GL_LIGHT6:
      case GL_LIGHT7:
	 *params = (GLint) ctx->Light.Light[pname-GL_LIGHT0].Enabled;
	 break;
      case GL_LIGHTING:
	 *params = (GLint) ctx->Light.Enabled;
	 break;
      case GL_LIGHT_MODEL_AMBIENT:
	 params[0] = FLOAT_TO_INT( ctx->Light.Model.Ambient[0] );
	 params[1] = FLOAT_TO_INT( ctx->Light.Model.Ambient[1] );
	 params[2] = FLOAT_TO_INT( ctx->Light.Model.Ambient[2] );
	 params[3] = FLOAT_TO_INT( ctx->Light.Model.Ambient[3] );
	 break;
      case GL_LIGHT_MODEL_COLOR_CONTROL:
         params[0] = (GLint) ctx->Light.Model.ColorControl;
         break;
      case GL_LIGHT_MODEL_LOCAL_VIEWER:
	 *params = (GLint) ctx->Light.Model.LocalViewer;
	 break;
      case GL_LIGHT_MODEL_TWO_SIDE:
	 *params = (GLint) ctx->Light.Model.TwoSide;
	 break;
      case GL_LINE_SMOOTH:
	 *params = (GLint) ctx->Line.SmoothFlag;
	 break;
      case GL_LINE_SMOOTH_HINT:
	 *params = (GLint) ctx->Hint.LineSmooth;
	 break;
      case GL_LINE_STIPPLE:
	 *params = (GLint) ctx->Line.StippleFlag;
	 break;
      case GL_LINE_STIPPLE_PATTERN:
         *params = (GLint) ctx->Line.StipplePattern;
         break;
      case GL_LINE_STIPPLE_REPEAT:
         *params = (GLint) ctx->Line.StippleFactor;
         break;
      case GL_LINE_WIDTH:
	 *params = (GLint) ctx->Line.Width;
	 break;
      case GL_LINE_WIDTH_GRANULARITY:
	 *params = (GLint) LINE_WIDTH_GRANULARITY;
	 break;
      case GL_LINE_WIDTH_RANGE:
	 params[0] = (GLint) MIN_LINE_WIDTH;
	 params[1] = (GLint) MAX_LINE_WIDTH;
	 break;
      case GL_LIST_BASE:
	 *params = (GLint) ctx->List.ListBase;
	 break;
      case GL_LIST_INDEX:
	 *params = (GLint) ctx->CurrentListNum;
	 break;
      case GL_LIST_MODE:
	 *params = ctx->ExecuteFlag ? (GLint) GL_COMPILE_AND_EXECUTE
	   			  : (GLint) GL_COMPILE;
	 break;
      case GL_INDEX_LOGIC_OP:
	 *params = (GLint) ctx->Color.IndexLogicOpEnabled;
	 break;
      case GL_COLOR_LOGIC_OP:
	 *params = (GLint) ctx->Color.ColorLogicOpEnabled;
	 break;
      case GL_LOGIC_OP_MODE:
         *params = (GLint) ctx->Color.LogicOp;
         break;
      case GL_MAP1_COLOR_4:
	 *params = (GLint) ctx->Eval.Map1Color4;
	 break;
      case GL_MAP1_GRID_DOMAIN:
	 params[0] = (GLint) ctx->Eval.MapGrid1u1;
	 params[1] = (GLint) ctx->Eval.MapGrid1u2;
	 break;
      case GL_MAP1_GRID_SEGMENTS:
	 *params = (GLint) ctx->Eval.MapGrid1un;
	 break;
      case GL_MAP1_INDEX:
	 *params = (GLint) ctx->Eval.Map1Index;
	 break;
      case GL_MAP1_NORMAL:
	 *params = (GLint) ctx->Eval.Map1Normal;
	 break;
      case GL_MAP1_TEXTURE_COORD_1:
	 *params = (GLint) ctx->Eval.Map1TextureCoord1;
	 break;
      case GL_MAP1_TEXTURE_COORD_2:
	 *params = (GLint) ctx->Eval.Map1TextureCoord2;
	 break;
      case GL_MAP1_TEXTURE_COORD_3:
	 *params = (GLint) ctx->Eval.Map1TextureCoord3;
	 break;
      case GL_MAP1_TEXTURE_COORD_4:
	 *params = (GLint) ctx->Eval.Map1TextureCoord4;
	 break;
      case GL_MAP1_VERTEX_3:
	 *params = (GLint) ctx->Eval.Map1Vertex3;
	 break;
      case GL_MAP1_VERTEX_4:
	 *params = (GLint) ctx->Eval.Map1Vertex4;
	 break;
      case GL_MAP2_COLOR_4:
	 *params = (GLint) ctx->Eval.Map2Color4;
	 break;
      case GL_MAP2_GRID_DOMAIN:
	 params[0] = (GLint) ctx->Eval.MapGrid2u1;
	 params[1] = (GLint) ctx->Eval.MapGrid2u2;
	 params[2] = (GLint) ctx->Eval.MapGrid2v1;
	 params[3] = (GLint) ctx->Eval.MapGrid2v2;
	 break;
      case GL_MAP2_GRID_SEGMENTS:
	 params[0] = (GLint) ctx->Eval.MapGrid2un;
	 params[1] = (GLint) ctx->Eval.MapGrid2vn;
	 break;
      case GL_MAP2_INDEX:
	 *params = (GLint) ctx->Eval.Map2Index;
	 break;
      case GL_MAP2_NORMAL:
	 *params = (GLint) ctx->Eval.Map2Normal;
	 break;
      case GL_MAP2_TEXTURE_COORD_1:
	 *params = (GLint) ctx->Eval.Map2TextureCoord1;
	 break;
      case GL_MAP2_TEXTURE_COORD_2:
	 *params = (GLint) ctx->Eval.Map2TextureCoord2;
	 break;
      case GL_MAP2_TEXTURE_COORD_3:
	 *params = (GLint) ctx->Eval.Map2TextureCoord3;
	 break;
      case GL_MAP2_TEXTURE_COORD_4:
	 *params = (GLint) ctx->Eval.Map2TextureCoord4;
	 break;
      case GL_MAP2_VERTEX_3:
	 *params = (GLint) ctx->Eval.Map2Vertex3;
	 break;
      case GL_MAP2_VERTEX_4:
	 *params = (GLint) ctx->Eval.Map2Vertex4;
	 break;
      case GL_MAP_COLOR:
	 *params = (GLint) ctx->Pixel.MapColorFlag;
	 break;
      case GL_MAP_STENCIL:
	 *params = (GLint) ctx->Pixel.MapStencilFlag;
	 break;
      case GL_MATRIX_MODE:
	 *params = (GLint) ctx->Transform.MatrixMode;
	 break;
      case GL_MAX_ATTRIB_STACK_DEPTH:
         *params = (GLint) MAX_ATTRIB_STACK_DEPTH;
         break;
      case GL_MAX_CLIENT_ATTRIB_STACK_DEPTH:
         *params = (GLint) MAX_CLIENT_ATTRIB_STACK_DEPTH;
         break;
      case GL_MAX_CLIP_PLANES:
         *params = (GLint) MAX_CLIP_PLANES;
         break;
      case GL_MAX_ELEMENTS_VERTICES:  /* GL_VERSION_1_2 */
         *params = VB_MAX;
         break;
      case GL_MAX_ELEMENTS_INDICES:   /* GL_VERSION_1_2 */
         *params = VB_MAX;
         break;
      case GL_MAX_EVAL_ORDER:
	 *params = (GLint) MAX_EVAL_ORDER;
	 break;
      case GL_MAX_LIGHTS:
         *params = (GLint) MAX_LIGHTS;
         break;
      case GL_MAX_LIST_NESTING:
         *params = (GLint) MAX_LIST_NESTING;
         break;
      case GL_MAX_MODELVIEW_STACK_DEPTH:
         *params = (GLint) MAX_MODELVIEW_STACK_DEPTH;
         break;
      case GL_MAX_NAME_STACK_DEPTH:
	 *params = (GLint) MAX_NAME_STACK_DEPTH;
	 break;
      case GL_MAX_PIXEL_MAP_TABLE:
	 *params = (GLint) MAX_PIXEL_MAP_TABLE;
	 break;
      case GL_MAX_PROJECTION_STACK_DEPTH:
         *params = (GLint) MAX_PROJECTION_STACK_DEPTH;
         break;
      case GL_MAX_TEXTURE_SIZE:
      case GL_MAX_3D_TEXTURE_SIZE:
         *params = ctx->Const.MaxTextureSize;
	 break;
      case GL_MAX_TEXTURE_STACK_DEPTH:
	 *params = (GLint) MAX_TEXTURE_STACK_DEPTH;
	 break;
      case GL_MAX_VIEWPORT_DIMS:
         params[0] = (GLint) MAX_WIDTH;
         params[1] = (GLint) MAX_HEIGHT;
         break;
      case GL_MODELVIEW_MATRIX:
	 for (i=0;i<16;i++) {
	    params[i] = (GLint) ctx->ModelView.m[i];
	 }
	 break;
      case GL_MODELVIEW_STACK_DEPTH:
	 *params = (GLint) (ctx->ModelViewStackDepth + 1);
	 break;
      case GL_NAME_STACK_DEPTH:
	 *params = (GLint) ctx->Select.NameStackDepth;
	 break;
      case GL_NORMALIZE:
	 *params = (GLint) ctx->Transform.Normalize;
	 break;
      case GL_PACK_ALIGNMENT:
	 *params = ctx->Pack.Alignment;
	 break;
      case GL_PACK_LSB_FIRST:
	 *params = (GLint) ctx->Pack.LsbFirst;
	 break;
      case GL_PACK_ROW_LENGTH:
	 *params = ctx->Pack.RowLength;
	 break;
      case GL_PACK_SKIP_PIXELS:
	 *params = ctx->Pack.SkipPixels;
	 break;
      case GL_PACK_SKIP_ROWS:
	 *params = ctx->Pack.SkipRows;
	 break;
      case GL_PACK_SWAP_BYTES:
	 *params = (GLint) ctx->Pack.SwapBytes;
	 break;
      case GL_PACK_SKIP_IMAGES_EXT:
         *params = ctx->Pack.SkipImages;
         break;
      case GL_PACK_IMAGE_HEIGHT_EXT:
         *params = ctx->Pack.ImageHeight;
         break;
      case GL_PERSPECTIVE_CORRECTION_HINT:
	 *params = (GLint) ctx->Hint.PerspectiveCorrection;
	 break;
      case GL_PIXEL_MAP_A_TO_A_SIZE:
	 *params = ctx->Pixel.MapAtoAsize;
	 break;
      case GL_PIXEL_MAP_B_TO_B_SIZE:
	 *params = ctx->Pixel.MapBtoBsize;
	 break;
      case GL_PIXEL_MAP_G_TO_G_SIZE:
	 *params = ctx->Pixel.MapGtoGsize;
	 break;
      case GL_PIXEL_MAP_I_TO_A_SIZE:
	 *params = ctx->Pixel.MapItoAsize;
	 break;
      case GL_PIXEL_MAP_I_TO_B_SIZE:
	 *params = ctx->Pixel.MapItoBsize;
	 break;
      case GL_PIXEL_MAP_I_TO_G_SIZE:
	 *params = ctx->Pixel.MapItoGsize;
	 break;
      case GL_PIXEL_MAP_I_TO_I_SIZE:
	 *params = ctx->Pixel.MapItoIsize;
	 break;
      case GL_PIXEL_MAP_I_TO_R_SIZE:
	 *params = ctx->Pixel.MapItoRsize;
	 break;
      case GL_PIXEL_MAP_R_TO_R_SIZE:
	 *params = ctx->Pixel.MapRtoRsize;
	 break;
      case GL_PIXEL_MAP_S_TO_S_SIZE:
	 *params = ctx->Pixel.MapStoSsize;
	 break;
      case GL_POINT_SIZE:
         *params = (GLint) ctx->Point.Size;
         break;
      case GL_POINT_SIZE_GRANULARITY:
	 *params = (GLint) POINT_SIZE_GRANULARITY;
	 break;
      case GL_POINT_SIZE_RANGE:
	 params[0] = (GLint) MIN_POINT_SIZE;
	 params[1] = (GLint) MAX_POINT_SIZE;
	 break;
      case GL_POINT_SMOOTH:
	 *params = (GLint) ctx->Point.SmoothFlag;
	 break;
      case GL_POINT_SMOOTH_HINT:
	 *params = (GLint) ctx->Hint.PointSmooth;
	 break;
      case GL_POINT_SIZE_MIN_EXT:
	 *params = (GLint) (ctx->Point.MinSize);
	 break;
      case GL_POINT_SIZE_MAX_EXT:
	 *params = (GLint) (ctx->Point.MaxSize);
	 break;
      case GL_POINT_FADE_THRESHOLD_SIZE_EXT:
	 *params = (GLint) (ctx->Point.Threshold);
	 break;
      case GL_DISTANCE_ATTENUATION_EXT:
	 params[0] = (GLint) (ctx->Point.Params[0]);
	 params[1] = (GLint) (ctx->Point.Params[1]);
	 params[2] = (GLint) (ctx->Point.Params[2]);
	 break;
      case GL_POLYGON_MODE:
	 params[0] = (GLint) ctx->Polygon.FrontMode;
	 params[1] = (GLint) ctx->Polygon.BackMode;
	 break;
#ifdef GL_EXT_polygon_offset
      case GL_POLYGON_OFFSET_BIAS_EXT:
         *params = (GLint) ctx->Polygon.OffsetUnits;
         break;
#endif
      case GL_POLYGON_OFFSET_FACTOR:
         *params = (GLint) ctx->Polygon.OffsetFactor;
         break;
      case GL_POLYGON_OFFSET_UNITS:
         *params = (GLint) ctx->Polygon.OffsetUnits;
         break;
      case GL_POLYGON_SMOOTH:
	 *params = (GLint) ctx->Polygon.SmoothFlag;
	 break;
      case GL_POLYGON_SMOOTH_HINT:
	 *params = (GLint) ctx->Hint.PolygonSmooth;
	 break;
      case GL_POLYGON_STIPPLE:
         *params = (GLint) ctx->Polygon.StippleFlag;
	 break;
      case GL_PROJECTION_MATRIX:
	 for (i=0;i<16;i++) {
	    params[i] = (GLint) ctx->ProjectionMatrix.m[i];
	 }
	 break;
      case GL_PROJECTION_STACK_DEPTH:
	 *params = (GLint) (ctx->ProjectionStackDepth + 1);
	 break;
      case GL_READ_BUFFER:
	 *params = (GLint) ctx->Pixel.ReadBuffer;
	 break;
      case GL_RED_BIAS:
         *params = (GLint) ctx->Pixel.RedBias;
         break;
      case GL_RED_BITS:
         *params = (GLint) ctx->Visual->RedBits;
         break;
      case GL_RED_SCALE:
         *params = (GLint) ctx->Pixel.RedScale;
         break;
      case GL_RENDER_MODE:
	 *params = (GLint) ctx->RenderMode;
	 break;
      case GL_RGBA_MODE:
	 *params = (GLint) ctx->Visual->RGBAflag;
	 break;
      case GL_SCISSOR_BOX:
	 params[0] = (GLint) ctx->Scissor.X;
	 params[1] = (GLint) ctx->Scissor.Y;
	 params[2] = (GLint) ctx->Scissor.Width;
	 params[3] = (GLint) ctx->Scissor.Height;
	 break;
      case GL_SCISSOR_TEST:
	 *params = (GLint) ctx->Scissor.Enabled;
	 break;
      case GL_SELECTION_BUFFER_SIZE:
         *params = (GLint) ctx->Select.BufferSize;
         break;
      case GL_SHADE_MODEL:
	 *params = (GLint) ctx->Light.ShadeModel;
	 break;
      case GL_SHARED_TEXTURE_PALETTE_EXT:
         *params = (GLint) ctx->Texture.SharedPalette;
         break;
      case GL_STENCIL_BITS:
         *params = ctx->Visual->StencilBits;
         break;
      case GL_STENCIL_CLEAR_VALUE:
	 *params = (GLint) ctx->Stencil.Clear;
	 break;
      case GL_STENCIL_FAIL:
	 *params = (GLint) ctx->Stencil.FailFunc;
	 break;
      case GL_STENCIL_FUNC:
	 *params = (GLint) ctx->Stencil.Function;
	 break;
      case GL_STENCIL_PASS_DEPTH_FAIL:
	 *params = (GLint) ctx->Stencil.ZFailFunc;
	 break;
      case GL_STENCIL_PASS_DEPTH_PASS:
	 *params = (GLint) ctx->Stencil.ZPassFunc;
	 break;
      case GL_STENCIL_REF:
	 *params = (GLint) ctx->Stencil.Ref;
	 break;
      case GL_STENCIL_TEST:
	 *params = (GLint) ctx->Stencil.Enabled;
	 break;
      case GL_STENCIL_VALUE_MASK:
	 *params = (GLint) ctx->Stencil.ValueMask;
	 break;
      case GL_STENCIL_WRITEMASK:
	 *params = (GLint) ctx->Stencil.WriteMask;
	 break;
      case GL_STEREO:
	 *params = (GLint) ctx->Visual->StereoFlag;
	 break;
      case GL_SUBPIXEL_BITS:
	 *params = 0;  /* TODO */
	 break;
      case GL_TEXTURE_1D:
         *params = gl_IsEnabled(ctx, GL_TEXTURE_1D) ? 1 : 0;
	 break;
      case GL_TEXTURE_2D:
         *params = gl_IsEnabled(ctx, GL_TEXTURE_2D) ? 1 : 0;
	 break;
      case GL_TEXTURE_3D:
         *params = gl_IsEnabled(ctx, GL_TEXTURE_3D) ? 1 : 0;
	 break;
      case GL_TEXTURE_BINDING_1D:
         *params = textureUnit->CurrentD[1]->Name;
          break;
      case GL_TEXTURE_BINDING_2D:
         *params = textureUnit->CurrentD[2]->Name;
          break;
      case GL_TEXTURE_BINDING_3D:
         *params = textureUnit->CurrentD[3]->Name;
          break;
      case GL_TEXTURE_ENV_COLOR:
	 params[0] = FLOAT_TO_INT( textureUnit->EnvColor[0] );
	 params[1] = FLOAT_TO_INT( textureUnit->EnvColor[1] );
	 params[2] = FLOAT_TO_INT( textureUnit->EnvColor[2] );
	 params[3] = FLOAT_TO_INT( textureUnit->EnvColor[3] );
	 break;
      case GL_TEXTURE_ENV_MODE:
	 *params = (GLint) textureUnit->EnvMode;
	 break;
      case GL_TEXTURE_GEN_S:
	 *params = (textureUnit->TexGenEnabled & S_BIT) ? 1 : 0;
	 break;
      case GL_TEXTURE_GEN_T:
	 *params = (textureUnit->TexGenEnabled & T_BIT) ? 1 : 0;
	 break;
      case GL_TEXTURE_GEN_R:
	 *params = (textureUnit->TexGenEnabled & R_BIT) ? 1 : 0;
	 break;
      case GL_TEXTURE_GEN_Q:
	 *params = (textureUnit->TexGenEnabled & Q_BIT) ? 1 : 0;
	 break;
      case GL_TEXTURE_MATRIX:
         for (i=0;i<16;i++) {
	    params[i] = (GLint) ctx->TextureMatrix[texTransformUnit].m[i];
	 }
	 break;
      case GL_TEXTURE_STACK_DEPTH:
	 *params = (GLint) (ctx->TextureStackDepth[texTransformUnit] + 1);
	 break;
      case GL_UNPACK_ALIGNMENT:
	 *params = ctx->Unpack.Alignment;
	 break;
      case GL_UNPACK_LSB_FIRST:
	 *params = (GLint) ctx->Unpack.LsbFirst;
	 break;
      case GL_UNPACK_ROW_LENGTH:
	 *params = ctx->Unpack.RowLength;
	 break;
      case GL_UNPACK_SKIP_PIXELS:
	 *params = ctx->Unpack.SkipPixels;
	 break;
      case GL_UNPACK_SKIP_ROWS:
	 *params = ctx->Unpack.SkipRows;
	 break;
      case GL_UNPACK_SWAP_BYTES:
	 *params = (GLint) ctx->Unpack.SwapBytes;
	 break;
      case GL_UNPACK_SKIP_IMAGES_EXT:
         *params = ctx->Unpack.SkipImages;
         break;
      case GL_UNPACK_IMAGE_HEIGHT_EXT:
         *params = ctx->Unpack.ImageHeight;
         break;
      case GL_VIEWPORT:
         params[0] = (GLint) ctx->Viewport.X;
         params[1] = (GLint) ctx->Viewport.Y;
         params[2] = (GLint) ctx->Viewport.Width;
         params[3] = (GLint) ctx->Viewport.Height;
         break;
      case GL_ZOOM_X:
	 *params = (GLint) ctx->Pixel.ZoomX;
	 break;
      case GL_ZOOM_Y:
	 *params = (GLint) ctx->Pixel.ZoomY;
	 break;
      case GL_VERTEX_ARRAY_SIZE:
         *params = ctx->Array.Vertex.Size;
         break;
      case GL_VERTEX_ARRAY_TYPE:
         *params = ctx->Array.Vertex.Type;
         break;
      case GL_VERTEX_ARRAY_STRIDE:
         *params = ctx->Array.Vertex.Stride;
         break;
      case GL_VERTEX_ARRAY_COUNT_EXT:
         *params = 0;
         break;
      case GL_NORMAL_ARRAY_TYPE:
         *params = ctx->Array.Normal.Type;
         break;
      case GL_NORMAL_ARRAY_STRIDE:
         *params = ctx->Array.Normal.Stride;
         break;
      case GL_NORMAL_ARRAY_COUNT_EXT:
         *params = 0;
         break;
      case GL_COLOR_ARRAY_SIZE:
         *params = ctx->Array.Color.Size;
         break;
      case GL_COLOR_ARRAY_TYPE:
         *params = ctx->Array.Color.Type;
         break;
      case GL_COLOR_ARRAY_STRIDE:
         *params = ctx->Array.Color.Stride;
         break;
      case GL_COLOR_ARRAY_COUNT_EXT:
         *params = 0;
         break;
      case GL_INDEX_ARRAY_TYPE:
         *params = ctx->Array.Index.Type;
         break;
      case GL_INDEX_ARRAY_STRIDE:
         *params = ctx->Array.Index.Stride;
         break;
      case GL_INDEX_ARRAY_COUNT_EXT:
         *params = 0;
         break;
      case GL_TEXTURE_COORD_ARRAY_SIZE:
         *params = ctx->Array.TexCoord[texUnit].Size;
         break;
      case GL_TEXTURE_COORD_ARRAY_TYPE:
         *params = ctx->Array.TexCoord[texUnit].Type;
         break;
      case GL_TEXTURE_COORD_ARRAY_STRIDE:
         *params = ctx->Array.TexCoord[texUnit].Stride;
         break;
      case GL_TEXTURE_COORD_ARRAY_COUNT_EXT:
         *params = 0;
         break;
      case GL_EDGE_FLAG_ARRAY_STRIDE:
         *params = ctx->Array.EdgeFlag.Stride;
         break;
      case GL_EDGE_FLAG_ARRAY_COUNT_EXT:
         *params = 0;
         break;

      case GL_MAX_TEXTURE_UNITS_ARB:
         *params = ctx->Const.MaxTextureUnits;
         break;
      case GL_ACTIVE_TEXTURE_ARB:
         *params = GL_TEXTURE0_ARB + ctx->Texture.CurrentUnit;
         break;
      case GL_CLIENT_ACTIVE_TEXTURE_ARB:
         *params = GL_TEXTURE0_ARB + ctx->Array.ActiveTexture;
         break;


      /* GL_PGI_misc_hints */
      case GL_STRICT_DEPTHFUNC_HINT_PGI:
	 *params = (GL_NICEST);
         break;
      case GL_STRICT_LIGHTING_HINT_PGI:
	 *params = (ctx->Hint.StrictLighting);
	 break;
      case GL_STRICT_SCISSOR_HINT_PGI:
      case GL_FULL_STIPPLE_HINT_PGI:
	 *params = (GL_TRUE);
	 break;
      case GL_CONSERVE_MEMORY_HINT_PGI:
	 *params = (GL_FALSE);
	 break;
      case GL_ALWAYS_FAST_HINT_PGI:
	 *params = (ctx->Hint.AllowDrawWin == GL_TRUE &&
		    ctx->Hint.AllowDrawSpn == GL_FALSE && 
		    ctx->Hint.AllowDrawMem == GL_FALSE);
	 break;
      case GL_ALWAYS_SOFT_HINT_PGI:
	 *params =  (ctx->Hint.AllowDrawWin == GL_TRUE &&
		     ctx->Hint.AllowDrawSpn == GL_TRUE && 
		     ctx->Hint.AllowDrawMem == GL_TRUE);
	 break;
      case GL_ALLOW_DRAW_OBJ_HINT_PGI:
	 *params = GL_TRUE;
	 break;
      case GL_ALLOW_DRAW_WIN_HINT_PGI:
	 *params = ctx->Hint.AllowDrawWin;
	 break;
      case GL_ALLOW_DRAW_SPN_HINT_PGI:
	 *params = ctx->Hint.AllowDrawSpn;
	 break;
      case GL_ALLOW_DRAW_MEM_HINT_PGI:
	 *params = ctx->Hint.AllowDrawMem;
	 break;
      case GL_CLIP_NEAR_HINT_PGI:
      case GL_CLIP_FAR_HINT_PGI:
	 *params = GL_TRUE;
	 break;
      case GL_WIDE_LINE_HINT_PGI:
	 *params = GL_DONT_CARE;
	 break;
      case GL_BACK_NORMALS_HINT_PGI:
	 *params = (GL_TRUE);
	 break;
      case GL_NATIVE_GRAPHICS_HANDLE_PGI:
	 *params = 0;
	 break;

      /* GL_EXT_compiled_vertex_array
       */
      case GL_ARRAY_ELEMENT_LOCK_FIRST_SGI:
	 *params = ctx->Array.LockFirst;
	 break;

      case GL_ARRAY_ELEMENT_LOCK_COUNT_SGI:
	 *params = ctx->Array.LockCount;
	 break;
	 
      default:
	 printf("invalid enum: %x\n", pname);
         gl_error( ctx, GL_INVALID_ENUM, "glGetIntegerv" );
   }
}



void gl_GetPointerv( GLcontext *ctx, GLenum pname, GLvoid **params )
{
   GLuint texUnit = ctx->Texture.CurrentUnit;
   /*GLuint texTransformUnit = ctx->Texture.CurrentTransformUnit;*/

   if (MESA_VERBOSE & VERBOSE_API) 
      fprintf(stderr, "glGetPointerv %s\n", gl_lookup_enum_by_nr(pname));

   switch (pname) {
      case GL_VERTEX_ARRAY_POINTER:
         *params = ctx->Array.Vertex.Ptr;
         break;
      case GL_NORMAL_ARRAY_POINTER:
         *params = ctx->Array.Normal.Ptr;
         break;
      case GL_COLOR_ARRAY_POINTER:
         *params = ctx->Array.Color.Ptr;
         break;
      case GL_INDEX_ARRAY_POINTER:
         *params = ctx->Array.Index.Ptr;
         break;
      case GL_TEXTURE_COORD_ARRAY_POINTER:
         *params = ctx->Array.TexCoord[texUnit].Ptr;
         break;
      case GL_EDGE_FLAG_ARRAY_POINTER:
         *params = ctx->Array.EdgeFlag.Ptr;
         break;
      case GL_FEEDBACK_BUFFER_POINTER:
         *params = ctx->Feedback.Buffer;
         break;
      case GL_SELECTION_BUFFER_POINTER:
         *params = ctx->Select.Buffer;
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetPointerv" );
         return;
   }
}



const GLubyte *gl_GetString( GLcontext *ctx, GLenum name )
{
   static char result[1000];
   static char *vendor = "Brian Paul";
   static char *version = "1.2.1 Mesa 3.1 beta";

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH_WITH_RETVAL(ctx, "glGetString", 0);

   /* First see if device driver can satisfy this call */
   switch (name) {
      case GL_VENDOR:
      case GL_RENDERER:
      case GL_VERSION:
         if (ctx->Driver.GetString) {
            const GLubyte *str = (*ctx->Driver.GetString)(ctx, name);
            if (str && str[0])
               return str;
         }
         break;
      /* Extensions always handled by extensions.c */
      case GL_EXTENSIONS:
	 return (GLubyte *) gl_extensions_get_string( ctx );
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetString" );
         return (GLubyte *) 0;
   }

   /* If we get here, device driver didn't return a string */
   switch (name) {
      case GL_VENDOR:
         return (GLubyte *) vendor;
      case GL_RENDERER:
         strcpy(result, "Mesa");
         if (ctx->Driver.RendererString) {
            strcat(result, " ");
            strcat(result, (*ctx->Driver.RendererString)());
         }
         return (GLubyte *) result;
      case GL_VERSION:
         return (GLubyte *) version;
      default:
         /* caught above */
         return NULL;
   }
}

#!/usr/bin/env python

# Mesa 3-D graphics library
#
# Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
# AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


# This script is used to generate the get.c file:
# python get_gen.py > get.c


import string
import sys


GLint = 1
GLenum = 2
GLfloat = 3
GLdouble = 4
GLboolean = 5
GLfloatN = 6    # A normalized value, such as a color or depth range
GLint64 = 7


TypeStrings = {
	GLint : "GLint",
	GLenum : "GLenum",
	GLfloat : "GLfloat",
	GLdouble : "GLdouble",
	GLboolean : "GLboolean",
	GLint64 : "GLint64"
}


NoState = None
NoExt = None
FlushCurrent = 1


# Each entry is a tuple of:
#  - the GL state name, such as GL_CURRENT_COLOR
#  - the state datatype, one of GLint, GLfloat, GLboolean or GLenum
#  - list of code fragments to get the state, such as ["ctx->Foo.Bar"]
#  - optional extra code or empty string.  If present, "CONVERSION" will be
#    replaced by ENUM_TO_FLOAT, INT_TO_FLOAT, etc.
#  - state flags: either NoExt, FlushCurrent or "_NEW_xxx"
#    if NoExt, do nothing special
#    if FlushCurrent, emit FLUSH_CURRENT() call
#    if "_NEW_xxx", call _mesa_update_state() if that dirty state flag is set
#  - optional extensions to check, or NoExt
#
StateVars = [
	( "GL_ACCUM_RED_BITS", GLint, ["ctx->DrawBuffer->Visual.accumRedBits"],
	  "", NoState, NoExt ),
	( "GL_ACCUM_GREEN_BITS", GLint, ["ctx->DrawBuffer->Visual.accumGreenBits"],
	  "", NoState, NoExt ),
	( "GL_ACCUM_BLUE_BITS", GLint, ["ctx->DrawBuffer->Visual.accumBlueBits"],
	  "", NoState, NoExt ),
	( "GL_ACCUM_ALPHA_BITS", GLint, ["ctx->DrawBuffer->Visual.accumAlphaBits"],
	  "", NoState, NoExt ),
	( "GL_ACCUM_CLEAR_VALUE", GLfloatN,
	  [ "ctx->Accum.ClearColor[0]",
		"ctx->Accum.ClearColor[1]",
		"ctx->Accum.ClearColor[2]",
		"ctx->Accum.ClearColor[3]" ],
	  "", NoState, NoExt ),
	( "GL_ALPHA_BIAS", GLfloat, ["ctx->Pixel.AlphaBias"], "", NoState, NoExt ),
	( "GL_ALPHA_BITS", GLint, ["ctx->DrawBuffer->Visual.alphaBits"],
	  "", "_NEW_BUFFERS", NoExt ),
	( "GL_ALPHA_SCALE", GLfloat, ["ctx->Pixel.AlphaScale"], "", NoState, NoExt ),
	( "GL_ALPHA_TEST", GLboolean, ["ctx->Color.AlphaEnabled"], "", NoState, NoExt ),
	( "GL_ALPHA_TEST_FUNC", GLenum, ["ctx->Color.AlphaFunc"], "", NoState, NoExt ),
	( "GL_ALPHA_TEST_REF", GLfloatN, ["ctx->Color.AlphaRef"], "", NoState, NoExt ),
	( "GL_ATTRIB_STACK_DEPTH", GLint, ["ctx->AttribStackDepth"], "", NoState, NoExt ),
	( "GL_AUTO_NORMAL", GLboolean, ["ctx->Eval.AutoNormal"], "", NoState, NoExt ),
	( "GL_AUX_BUFFERS", GLint, ["ctx->DrawBuffer->Visual.numAuxBuffers"],
	  "", NoState, NoExt ),
	( "GL_BLEND", GLboolean, ["(ctx->Color.BlendEnabled & 1)"], "", NoState, NoExt ),
	( "GL_BLEND_DST", GLenum, ["ctx->Color.BlendDstRGB"], "", NoState, NoExt ),
	( "GL_BLEND_SRC", GLenum, ["ctx->Color.BlendSrcRGB"], "", NoState, NoExt ),
	( "GL_BLEND_SRC_RGB_EXT", GLenum, ["ctx->Color.BlendSrcRGB"], "", NoState, NoExt ),
	( "GL_BLEND_DST_RGB_EXT", GLenum, ["ctx->Color.BlendDstRGB"], "", NoState, NoExt ),
	( "GL_BLEND_SRC_ALPHA_EXT", GLenum, ["ctx->Color.BlendSrcA"], "", NoState, NoExt ),
	( "GL_BLEND_DST_ALPHA_EXT", GLenum, ["ctx->Color.BlendDstA"], "", NoState, NoExt ),
	( "GL_BLEND_EQUATION", GLenum, ["ctx->Color.BlendEquationRGB "], "", NoState, NoExt),
	( "GL_BLEND_EQUATION_ALPHA_EXT", GLenum, ["ctx->Color.BlendEquationA "],
	  "", NoState, NoExt ),
	( "GL_BLEND_COLOR_EXT", GLfloatN,
	  [ "ctx->Color.BlendColor[0]",
		"ctx->Color.BlendColor[1]",
		"ctx->Color.BlendColor[2]",
		"ctx->Color.BlendColor[3]"], "", NoState, NoExt ),
	( "GL_BLUE_BIAS", GLfloat, ["ctx->Pixel.BlueBias"], "", NoState, NoExt ),
	( "GL_BLUE_BITS", GLint, ["ctx->DrawBuffer->Visual.blueBits"], "", "_NEW_BUFFERS", NoExt ),
	( "GL_BLUE_SCALE", GLfloat, ["ctx->Pixel.BlueScale"], "", NoState, NoExt ),
	( "GL_CLIENT_ATTRIB_STACK_DEPTH", GLint,
	  ["ctx->ClientAttribStackDepth"], "", NoState, NoExt ),
	( "GL_CLIP_PLANE0", GLboolean,
	  [ "(ctx->Transform.ClipPlanesEnabled >> 0) & 1" ], "", NoState, NoExt ),
	( "GL_CLIP_PLANE1", GLboolean,
	  [ "(ctx->Transform.ClipPlanesEnabled >> 1) & 1" ], "", NoState, NoExt ),
	( "GL_CLIP_PLANE2", GLboolean,
	  [ "(ctx->Transform.ClipPlanesEnabled >> 2) & 1" ], "", NoState, NoExt ),
	( "GL_CLIP_PLANE3", GLboolean,
	  [ "(ctx->Transform.ClipPlanesEnabled >> 3) & 1" ], "", NoState, NoExt ),
	( "GL_CLIP_PLANE4", GLboolean,
	  [ "(ctx->Transform.ClipPlanesEnabled >> 4) & 1" ], "", NoState, NoExt ),
	( "GL_CLIP_PLANE5", GLboolean,
	  [ "(ctx->Transform.ClipPlanesEnabled >> 5) & 1" ], "", NoState, NoExt ),
	( "GL_COLOR_CLEAR_VALUE", GLfloatN,
	  [ "ctx->Color.ClearColor[0]",
		"ctx->Color.ClearColor[1]",
		"ctx->Color.ClearColor[2]",
		"ctx->Color.ClearColor[3]" ], "", NoState, NoExt ),
	( "GL_COLOR_MATERIAL", GLboolean,
	  ["ctx->Light.ColorMaterialEnabled"], "", NoState, NoExt ),
	( "GL_COLOR_MATERIAL_FACE", GLenum,
	  ["ctx->Light.ColorMaterialFace"], "", NoState, NoExt ),
	( "GL_COLOR_MATERIAL_PARAMETER", GLenum,
	  ["ctx->Light.ColorMaterialMode"], "", NoState, NoExt ),
	( "GL_COLOR_WRITEMASK", GLint,
	  [ "ctx->Color.ColorMask[0][RCOMP] ? 1 : 0",
		"ctx->Color.ColorMask[0][GCOMP] ? 1 : 0",
		"ctx->Color.ColorMask[0][BCOMP] ? 1 : 0",
		"ctx->Color.ColorMask[0][ACOMP] ? 1 : 0" ], "", NoState, NoExt ),
	( "GL_CULL_FACE", GLboolean, ["ctx->Polygon.CullFlag"], "", NoState, NoExt ),
	( "GL_CULL_FACE_MODE", GLenum, ["ctx->Polygon.CullFaceMode"], "", NoState, NoExt ),
	( "GL_CURRENT_COLOR", GLfloatN,
	  [ "ctx->Current.Attrib[VERT_ATTRIB_COLOR0][0]",
		"ctx->Current.Attrib[VERT_ATTRIB_COLOR0][1]",
		"ctx->Current.Attrib[VERT_ATTRIB_COLOR0][2]",
		"ctx->Current.Attrib[VERT_ATTRIB_COLOR0][3]" ],
	  "", FlushCurrent, NoExt ),
	( "GL_CURRENT_INDEX", GLfloat,
	  [ "ctx->Current.Attrib[VERT_ATTRIB_COLOR_INDEX][0]" ],
	  "", FlushCurrent, NoExt ),
	( "GL_CURRENT_NORMAL", GLfloatN,
	  [ "ctx->Current.Attrib[VERT_ATTRIB_NORMAL][0]",
		"ctx->Current.Attrib[VERT_ATTRIB_NORMAL][1]",
		"ctx->Current.Attrib[VERT_ATTRIB_NORMAL][2]"],
	  "", FlushCurrent, NoExt ),
	( "GL_CURRENT_RASTER_COLOR", GLfloatN,
	  ["ctx->Current.RasterColor[0]",
	   "ctx->Current.RasterColor[1]",
	   "ctx->Current.RasterColor[2]",
	   "ctx->Current.RasterColor[3]"], "", NoState, NoExt ),
	( "GL_CURRENT_RASTER_DISTANCE", GLfloat,
	  ["ctx->Current.RasterDistance"], "", NoState, NoExt ),
	( "GL_CURRENT_RASTER_INDEX", GLfloat,
	  ["1.0"], "", NoState, NoExt ),
	( "GL_CURRENT_RASTER_POSITION", GLfloat,
	  ["ctx->Current.RasterPos[0]",
	   "ctx->Current.RasterPos[1]",
	   "ctx->Current.RasterPos[2]",
	   "ctx->Current.RasterPos[3]"], "", NoState, NoExt ),
	( "GL_CURRENT_RASTER_SECONDARY_COLOR", GLfloatN,
	  ["ctx->Current.RasterSecondaryColor[0]",
	   "ctx->Current.RasterSecondaryColor[1]",
	   "ctx->Current.RasterSecondaryColor[2]",
	   "ctx->Current.RasterSecondaryColor[3]"], "", NoState, NoExt ),
	( "GL_CURRENT_RASTER_TEXTURE_COORDS", GLfloat,
	  ["ctx->Current.RasterTexCoords[unit][0]",
	   "ctx->Current.RasterTexCoords[unit][1]",
	   "ctx->Current.RasterTexCoords[unit][2]",
	   "ctx->Current.RasterTexCoords[unit][3]"],
	  """const GLuint unit = ctx->Texture.CurrentUnit;
         if (unit >= ctx->Const.MaxTextureCoordUnits) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glGet(raster tex coords, unit %u)", unit);
            return;
         }""",
	  NoState, NoExt ),
	( "GL_CURRENT_RASTER_POSITION_VALID", GLboolean,
	  ["ctx->Current.RasterPosValid"], "", NoState, NoExt ),
	( "GL_CURRENT_TEXTURE_COORDS", GLfloat,
	  ["ctx->Current.Attrib[VERT_ATTRIB_TEX0 + unit][0]",
	   "ctx->Current.Attrib[VERT_ATTRIB_TEX0 + unit][1]",
	   "ctx->Current.Attrib[VERT_ATTRIB_TEX0 + unit][2]",
	   "ctx->Current.Attrib[VERT_ATTRIB_TEX0 + unit][3]"],
	  """const GLuint unit = ctx->Texture.CurrentUnit;
         if (unit >= ctx->Const.MaxTextureCoordUnits) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glGet(current tex coords, unit %u)", unit);
            return;
         }
         FLUSH_CURRENT(ctx, 0);""",
	  NoState, NoExt ),
	( "GL_DEPTH_BIAS", GLfloat, ["ctx->Pixel.DepthBias"], "", NoState, NoExt ),
	( "GL_DEPTH_BITS", GLint, ["ctx->DrawBuffer->Visual.depthBits"],
	  "", NoState, NoExt ),
	( "GL_DEPTH_CLEAR_VALUE", GLfloatN, ["((GLfloat) ctx->Depth.Clear)"], "", NoState, NoExt ),
	( "GL_DEPTH_FUNC", GLenum, ["ctx->Depth.Func"], "", NoState, NoExt ),
	( "GL_DEPTH_RANGE", GLfloatN,
	  [ "ctx->Viewport.Near", "ctx->Viewport.Far" ], "", NoState, NoExt ),
	( "GL_DEPTH_SCALE", GLfloat, ["ctx->Pixel.DepthScale"], "", NoState, NoExt ),
	( "GL_DEPTH_TEST", GLboolean, ["ctx->Depth.Test"], "", NoState, NoExt ),
	( "GL_DEPTH_WRITEMASK", GLboolean, ["ctx->Depth.Mask"], "", NoState, NoExt ),
	( "GL_DITHER", GLboolean, ["ctx->Color.DitherFlag"], "", NoState, NoExt ),
	( "GL_DOUBLEBUFFER", GLboolean,
	  ["ctx->DrawBuffer->Visual.doubleBufferMode"], "", NoState, NoExt ),
	( "GL_DRAW_BUFFER", GLenum, ["ctx->DrawBuffer->ColorDrawBuffer[0]"], "", NoState, NoExt ),
	( "GL_EDGE_FLAG", GLboolean, ["(ctx->Current.Attrib[VERT_ATTRIB_EDGEFLAG][0] == 1.0)"],
	  "", FlushCurrent, NoExt ),
	( "GL_FEEDBACK_BUFFER_SIZE", GLint, ["ctx->Feedback.BufferSize"], "", NoState, NoExt ),
	( "GL_FEEDBACK_BUFFER_TYPE", GLenum, ["ctx->Feedback.Type"], "", NoState, NoExt ),
	( "GL_FOG", GLboolean, ["ctx->Fog.Enabled"], "", NoState, NoExt ),
	( "GL_FOG_COLOR", GLfloatN,
	  [ "ctx->Fog.Color[0]",
		"ctx->Fog.Color[1]",
		"ctx->Fog.Color[2]",
		"ctx->Fog.Color[3]" ], "", NoState, NoExt ),
	( "GL_FOG_DENSITY", GLfloat, ["ctx->Fog.Density"], "", NoState, NoExt ),
	( "GL_FOG_END", GLfloat, ["ctx->Fog.End"], "", NoState, NoExt ),
	( "GL_FOG_HINT", GLenum, ["ctx->Hint.Fog"], "", NoState, NoExt ),
	( "GL_FOG_INDEX", GLfloat, ["ctx->Fog.Index"], "", NoState, NoExt ),
	( "GL_FOG_MODE", GLenum, ["ctx->Fog.Mode"], "", NoState, NoExt ),
	( "GL_FOG_START", GLfloat, ["ctx->Fog.Start"], "", NoState, NoExt ),
	( "GL_FRONT_FACE", GLenum, ["ctx->Polygon.FrontFace"], "", NoState, NoExt ),
	( "GL_GREEN_BIAS", GLfloat, ["ctx->Pixel.GreenBias"], "", NoState, NoExt ),
	( "GL_GREEN_BITS", GLint, ["ctx->DrawBuffer->Visual.greenBits"],
	  "", "_NEW_BUFFERS", NoExt ),
	( "GL_GREEN_SCALE", GLfloat, ["ctx->Pixel.GreenScale"], "", NoState, NoExt ),
	( "GL_INDEX_BITS", GLint, ["ctx->DrawBuffer->Visual.indexBits"],
	  "", "_NEW_BUFFERS", NoExt ),
	( "GL_INDEX_CLEAR_VALUE", GLint, ["ctx->Color.ClearIndex"], "", NoState, NoExt ),
	( "GL_INDEX_MODE", GLboolean, ["GL_FALSE"],
	  "", NoState, NoExt ),
	( "GL_INDEX_OFFSET", GLint, ["ctx->Pixel.IndexOffset"], "", NoState, NoExt ),
	( "GL_INDEX_SHIFT", GLint, ["ctx->Pixel.IndexShift"], "", NoState, NoExt ),
	( "GL_INDEX_WRITEMASK", GLint, ["ctx->Color.IndexMask"], "", NoState, NoExt ),
	( "GL_LIGHT0", GLboolean, ["ctx->Light.Light[0].Enabled"], "", NoState, NoExt ),
	( "GL_LIGHT1", GLboolean, ["ctx->Light.Light[1].Enabled"], "", NoState, NoExt ),
	( "GL_LIGHT2", GLboolean, ["ctx->Light.Light[2].Enabled"], "", NoState, NoExt ),
	( "GL_LIGHT3", GLboolean, ["ctx->Light.Light[3].Enabled"], "", NoState, NoExt ),
	( "GL_LIGHT4", GLboolean, ["ctx->Light.Light[4].Enabled"], "", NoState, NoExt ),
	( "GL_LIGHT5", GLboolean, ["ctx->Light.Light[5].Enabled"], "", NoState, NoExt ),
	( "GL_LIGHT6", GLboolean, ["ctx->Light.Light[6].Enabled"], "", NoState, NoExt ),
	( "GL_LIGHT7", GLboolean, ["ctx->Light.Light[7].Enabled"], "", NoState, NoExt ),
	( "GL_LIGHTING", GLboolean, ["ctx->Light.Enabled"], "", NoState, NoExt ),
	( "GL_LIGHT_MODEL_AMBIENT", GLfloatN,
	  ["ctx->Light.Model.Ambient[0]",
	   "ctx->Light.Model.Ambient[1]",
	   "ctx->Light.Model.Ambient[2]",
	   "ctx->Light.Model.Ambient[3]"], "", NoState, NoExt ),
	( "GL_LIGHT_MODEL_COLOR_CONTROL", GLenum,
	  ["ctx->Light.Model.ColorControl"], "", NoState, NoExt ),
	( "GL_LIGHT_MODEL_LOCAL_VIEWER", GLboolean,
	  ["ctx->Light.Model.LocalViewer"], "", NoState, NoExt ),
	( "GL_LIGHT_MODEL_TWO_SIDE", GLboolean, ["ctx->Light.Model.TwoSide"], "", NoState, NoExt ),
	( "GL_LINE_SMOOTH", GLboolean, ["ctx->Line.SmoothFlag"], "", NoState, NoExt ),
	( "GL_LINE_SMOOTH_HINT", GLenum, ["ctx->Hint.LineSmooth"], "", NoState, NoExt ),
	( "GL_LINE_STIPPLE", GLboolean, ["ctx->Line.StippleFlag"], "", NoState, NoExt ),
	( "GL_LINE_STIPPLE_PATTERN", GLint, ["ctx->Line.StipplePattern"], "", NoState, NoExt ),
	( "GL_LINE_STIPPLE_REPEAT", GLint, ["ctx->Line.StippleFactor"], "", NoState, NoExt ),
	( "GL_LINE_WIDTH", GLfloat, ["ctx->Line.Width"], "", NoState, NoExt ),
	( "GL_LINE_WIDTH_GRANULARITY", GLfloat,
	  ["ctx->Const.LineWidthGranularity"], "", NoState, NoExt ),
	( "GL_LINE_WIDTH_RANGE", GLfloat,
	  ["ctx->Const.MinLineWidthAA",
	   "ctx->Const.MaxLineWidthAA"], "", NoState, NoExt ),
	( "GL_ALIASED_LINE_WIDTH_RANGE", GLfloat,
	  ["ctx->Const.MinLineWidth",
	   "ctx->Const.MaxLineWidth"], "", NoState, NoExt ),
	( "GL_LIST_BASE", GLint, ["ctx->List.ListBase"], "", NoState, NoExt ),
	( "GL_LIST_INDEX", GLint, ["(ctx->ListState.CurrentList ? ctx->ListState.CurrentList->Name : 0)"], "", NoState, NoExt ),
	( "GL_LIST_MODE", GLenum, ["mode"],
	  """GLenum mode;
         if (!ctx->CompileFlag)
            mode = 0;
         else if (ctx->ExecuteFlag)
            mode = GL_COMPILE_AND_EXECUTE;
         else
            mode = GL_COMPILE;""", NoState, NoExt ),
	( "GL_INDEX_LOGIC_OP", GLboolean, ["ctx->Color.IndexLogicOpEnabled"], "", NoState, NoExt ),
	( "GL_COLOR_LOGIC_OP", GLboolean, ["ctx->Color.ColorLogicOpEnabled"], "", NoState, NoExt ),
	( "GL_LOGIC_OP_MODE", GLenum, ["ctx->Color.LogicOp"], "", NoState, NoExt ),
	( "GL_MAP1_COLOR_4", GLboolean, ["ctx->Eval.Map1Color4"], "", NoState, NoExt ),
	( "GL_MAP1_GRID_DOMAIN", GLfloat,
	  ["ctx->Eval.MapGrid1u1",
	   "ctx->Eval.MapGrid1u2"], "", NoState, NoExt ),
	( "GL_MAP1_GRID_SEGMENTS", GLint, ["ctx->Eval.MapGrid1un"], "", NoState, NoExt ),
	( "GL_MAP1_INDEX", GLboolean, ["ctx->Eval.Map1Index"], "", NoState, NoExt ),
	( "GL_MAP1_NORMAL", GLboolean, ["ctx->Eval.Map1Normal"], "", NoState, NoExt ),
	( "GL_MAP1_TEXTURE_COORD_1", GLboolean, ["ctx->Eval.Map1TextureCoord1"], "", NoState, NoExt ),
	( "GL_MAP1_TEXTURE_COORD_2", GLboolean, ["ctx->Eval.Map1TextureCoord2"], "", NoState, NoExt ),
	( "GL_MAP1_TEXTURE_COORD_3", GLboolean, ["ctx->Eval.Map1TextureCoord3"], "", NoState, NoExt ),
	( "GL_MAP1_TEXTURE_COORD_4", GLboolean, ["ctx->Eval.Map1TextureCoord4"], "", NoState, NoExt ),
	( "GL_MAP1_VERTEX_3", GLboolean, ["ctx->Eval.Map1Vertex3"], "", NoState, NoExt ),
	( "GL_MAP1_VERTEX_4", GLboolean, ["ctx->Eval.Map1Vertex4"], "", NoState, NoExt ),
	( "GL_MAP2_COLOR_4", GLboolean, ["ctx->Eval.Map2Color4"], "", NoState, NoExt ),
	( "GL_MAP2_GRID_DOMAIN", GLfloat,
	  ["ctx->Eval.MapGrid2u1",
	   "ctx->Eval.MapGrid2u2",
	   "ctx->Eval.MapGrid2v1",
	   "ctx->Eval.MapGrid2v2"], "", NoState, NoExt ),
	( "GL_MAP2_GRID_SEGMENTS", GLint,
	  ["ctx->Eval.MapGrid2un",
	   "ctx->Eval.MapGrid2vn"], "", NoState, NoExt ),
	( "GL_MAP2_INDEX", GLboolean, ["ctx->Eval.Map2Index"], "", NoState, NoExt ),
	( "GL_MAP2_NORMAL", GLboolean, ["ctx->Eval.Map2Normal"], "", NoState, NoExt ),
	( "GL_MAP2_TEXTURE_COORD_1", GLboolean, ["ctx->Eval.Map2TextureCoord1"], "", NoState, NoExt ),
	( "GL_MAP2_TEXTURE_COORD_2", GLboolean, ["ctx->Eval.Map2TextureCoord2"], "", NoState, NoExt ),
	( "GL_MAP2_TEXTURE_COORD_3", GLboolean, ["ctx->Eval.Map2TextureCoord3"], "", NoState, NoExt ),
	( "GL_MAP2_TEXTURE_COORD_4", GLboolean, ["ctx->Eval.Map2TextureCoord4"], "", NoState, NoExt ),
	( "GL_MAP2_VERTEX_3", GLboolean, ["ctx->Eval.Map2Vertex3"], "", NoState, NoExt ),
	( "GL_MAP2_VERTEX_4", GLboolean, ["ctx->Eval.Map2Vertex4"], "", NoState, NoExt ),
	( "GL_MAP_COLOR", GLboolean, ["ctx->Pixel.MapColorFlag"], "", NoState, NoExt ),
	( "GL_MAP_STENCIL", GLboolean, ["ctx->Pixel.MapStencilFlag"], "", NoState, NoExt ),
	( "GL_MATRIX_MODE", GLenum, ["ctx->Transform.MatrixMode"], "", NoState, NoExt ),

	( "GL_MAX_ATTRIB_STACK_DEPTH", GLint, ["MAX_ATTRIB_STACK_DEPTH"], "", NoState, NoExt ),
	( "GL_MAX_CLIENT_ATTRIB_STACK_DEPTH", GLint, ["MAX_CLIENT_ATTRIB_STACK_DEPTH"], "", NoState, NoExt ),
	( "GL_MAX_CLIP_PLANES", GLint, ["ctx->Const.MaxClipPlanes"], "", NoState, NoExt ),
	( "GL_MAX_ELEMENTS_VERTICES", GLint, ["ctx->Const.MaxArrayLockSize"], "", NoState, NoExt ),
	( "GL_MAX_ELEMENTS_INDICES", GLint, ["ctx->Const.MaxArrayLockSize"], "", NoState, NoExt ),
	( "GL_MAX_EVAL_ORDER", GLint, ["MAX_EVAL_ORDER"], "", NoState, NoExt ),
	( "GL_MAX_LIGHTS", GLint, ["ctx->Const.MaxLights"], "", NoState, NoExt ),
	( "GL_MAX_LIST_NESTING", GLint, ["MAX_LIST_NESTING"], "", NoState, NoExt ),
	( "GL_MAX_MODELVIEW_STACK_DEPTH", GLint, ["MAX_MODELVIEW_STACK_DEPTH"], "", NoState, NoExt ),
	( "GL_MAX_NAME_STACK_DEPTH", GLint, ["MAX_NAME_STACK_DEPTH"], "", NoState, NoExt ),
	( "GL_MAX_PIXEL_MAP_TABLE", GLint, ["MAX_PIXEL_MAP_TABLE"], "", NoState, NoExt ),
	( "GL_MAX_PROJECTION_STACK_DEPTH", GLint, ["MAX_PROJECTION_STACK_DEPTH"], "", NoState, NoExt ),
	( "GL_MAX_TEXTURE_SIZE", GLint, ["1 << (ctx->Const.MaxTextureLevels - 1)"], "", NoState, NoExt ),
	( "GL_MAX_3D_TEXTURE_SIZE", GLint, ["1 << (ctx->Const.Max3DTextureLevels - 1)"], "", NoState, NoExt ),
	( "GL_MAX_TEXTURE_STACK_DEPTH", GLint, ["MAX_TEXTURE_STACK_DEPTH"], "", NoState, NoExt ),
	( "GL_MAX_VIEWPORT_DIMS", GLint,
	  ["ctx->Const.MaxViewportWidth", "ctx->Const.MaxViewportHeight"],
	  "", NoState, NoExt ),
	( "GL_MODELVIEW_MATRIX", GLfloat,
	  [ "matrix[0]", "matrix[1]", "matrix[2]", "matrix[3]",
		"matrix[4]", "matrix[5]", "matrix[6]", "matrix[7]",
		"matrix[8]", "matrix[9]", "matrix[10]", "matrix[11]",
		"matrix[12]", "matrix[13]", "matrix[14]", "matrix[15]" ],
	  "const GLfloat *matrix = ctx->ModelviewMatrixStack.Top->m;", NoState, NoExt ),
	( "GL_MODELVIEW_STACK_DEPTH", GLint, ["ctx->ModelviewMatrixStack.Depth + 1"], "", NoState, NoExt ),
	( "GL_NAME_STACK_DEPTH", GLint, ["ctx->Select.NameStackDepth"], "", NoState, NoExt ),
	( "GL_NORMALIZE", GLboolean, ["ctx->Transform.Normalize"], "", NoState, NoExt ),
	( "GL_PACK_ALIGNMENT", GLint, ["ctx->Pack.Alignment"], "", NoState, NoExt ),
	( "GL_PACK_LSB_FIRST", GLboolean, ["ctx->Pack.LsbFirst"], "", NoState, NoExt ),
	( "GL_PACK_ROW_LENGTH", GLint, ["ctx->Pack.RowLength"], "", NoState, NoExt ),
	( "GL_PACK_SKIP_PIXELS", GLint, ["ctx->Pack.SkipPixels"], "", NoState, NoExt ),
	( "GL_PACK_SKIP_ROWS", GLint, ["ctx->Pack.SkipRows"], "", NoState, NoExt ),
	( "GL_PACK_SWAP_BYTES", GLboolean, ["ctx->Pack.SwapBytes"], "", NoState, NoExt ),
	( "GL_PACK_SKIP_IMAGES_EXT", GLint, ["ctx->Pack.SkipImages"], "", NoState, NoExt ),
	( "GL_PACK_IMAGE_HEIGHT_EXT", GLint, ["ctx->Pack.ImageHeight"], "", NoState, NoExt ),
	( "GL_PACK_INVERT_MESA", GLboolean, ["ctx->Pack.Invert"], "", NoState, NoExt ),
	( "GL_PERSPECTIVE_CORRECTION_HINT", GLenum,
	  ["ctx->Hint.PerspectiveCorrection"], "", NoState, NoExt ),
	( "GL_PIXEL_MAP_A_TO_A_SIZE", GLint, ["ctx->PixelMaps.AtoA.Size"], "", NoState, NoExt ),
	( "GL_PIXEL_MAP_B_TO_B_SIZE", GLint, ["ctx->PixelMaps.BtoB.Size"], "", NoState, NoExt ),
	( "GL_PIXEL_MAP_G_TO_G_SIZE", GLint, ["ctx->PixelMaps.GtoG.Size"], "", NoState, NoExt ),
	( "GL_PIXEL_MAP_I_TO_A_SIZE", GLint, ["ctx->PixelMaps.ItoA.Size"], "", NoState, NoExt ),
	( "GL_PIXEL_MAP_I_TO_B_SIZE", GLint, ["ctx->PixelMaps.ItoB.Size"], "", NoState, NoExt ),
	( "GL_PIXEL_MAP_I_TO_G_SIZE", GLint, ["ctx->PixelMaps.ItoG.Size"], "", NoState, NoExt ),
	( "GL_PIXEL_MAP_I_TO_I_SIZE", GLint, ["ctx->PixelMaps.ItoI.Size"], "", NoState, NoExt ),
	( "GL_PIXEL_MAP_I_TO_R_SIZE", GLint, ["ctx->PixelMaps.ItoR.Size"], "", NoState, NoExt ),
	( "GL_PIXEL_MAP_R_TO_R_SIZE", GLint, ["ctx->PixelMaps.RtoR.Size"], "", NoState, NoExt ),
	( "GL_PIXEL_MAP_S_TO_S_SIZE", GLint, ["ctx->PixelMaps.StoS.Size"], "", NoState, NoExt ),
	( "GL_POINT_SIZE", GLfloat, ["ctx->Point.Size"], "", NoState, NoExt ),
	( "GL_POINT_SIZE_GRANULARITY", GLfloat,
	  ["ctx->Const.PointSizeGranularity"], "", NoState, NoExt ),
	( "GL_POINT_SIZE_RANGE", GLfloat,
	  ["ctx->Const.MinPointSizeAA",
	   "ctx->Const.MaxPointSizeAA"], "", NoState, NoExt ),
	( "GL_ALIASED_POINT_SIZE_RANGE", GLfloat,
	  ["ctx->Const.MinPointSize",
	   "ctx->Const.MaxPointSize"], "", NoState, NoExt ),
	( "GL_POINT_SMOOTH", GLboolean, ["ctx->Point.SmoothFlag"], "", NoState, NoExt ),
	( "GL_POINT_SMOOTH_HINT", GLenum, ["ctx->Hint.PointSmooth"], "", NoState, NoExt ),
	( "GL_POINT_SIZE_MIN_EXT", GLfloat, ["ctx->Point.MinSize"], "", NoState, NoExt ),
	( "GL_POINT_SIZE_MAX_EXT", GLfloat, ["ctx->Point.MaxSize"], "", NoState, NoExt ),
	( "GL_POINT_FADE_THRESHOLD_SIZE_EXT", GLfloat,
	  ["ctx->Point.Threshold"], "", NoState, NoExt ),
	( "GL_DISTANCE_ATTENUATION_EXT", GLfloat,
	  ["ctx->Point.Params[0]",
	   "ctx->Point.Params[1]",
	   "ctx->Point.Params[2]"], "", NoState, NoExt ),
	( "GL_POLYGON_MODE", GLenum,
	  ["ctx->Polygon.FrontMode",
	   "ctx->Polygon.BackMode"], "", NoState, NoExt ),
	( "GL_POLYGON_OFFSET_BIAS_EXT", GLfloat, ["ctx->Polygon.OffsetUnits"], "", NoState, NoExt ),
	( "GL_POLYGON_OFFSET_FACTOR", GLfloat, ["ctx->Polygon.OffsetFactor "], "", NoState, NoExt ),
	( "GL_POLYGON_OFFSET_UNITS", GLfloat, ["ctx->Polygon.OffsetUnits "], "", NoState, NoExt ),
	( "GL_POLYGON_OFFSET_POINT", GLboolean, ["ctx->Polygon.OffsetPoint"], "", NoState, NoExt ),
	( "GL_POLYGON_OFFSET_LINE", GLboolean, ["ctx->Polygon.OffsetLine"], "", NoState, NoExt ),
	( "GL_POLYGON_OFFSET_FILL", GLboolean, ["ctx->Polygon.OffsetFill"], "", NoState, NoExt ),
	( "GL_POLYGON_SMOOTH", GLboolean, ["ctx->Polygon.SmoothFlag"], "", NoState, NoExt ),
	( "GL_POLYGON_SMOOTH_HINT", GLenum, ["ctx->Hint.PolygonSmooth"], "", NoState, NoExt ),
	( "GL_POLYGON_STIPPLE", GLboolean, ["ctx->Polygon.StippleFlag"], "", NoState, NoExt ),
	( "GL_PROJECTION_MATRIX", GLfloat,
	  [ "matrix[0]", "matrix[1]", "matrix[2]", "matrix[3]",
		"matrix[4]", "matrix[5]", "matrix[6]", "matrix[7]",
		"matrix[8]", "matrix[9]", "matrix[10]", "matrix[11]",
		"matrix[12]", "matrix[13]", "matrix[14]", "matrix[15]" ],
	  "const GLfloat *matrix = ctx->ProjectionMatrixStack.Top->m;", NoState, NoExt ),
	( "GL_PROJECTION_STACK_DEPTH", GLint,
	  ["ctx->ProjectionMatrixStack.Depth + 1"], "", NoState, NoExt ),
	( "GL_READ_BUFFER", GLenum, ["ctx->ReadBuffer->ColorReadBuffer"], "", NoState, NoExt ),
	( "GL_RED_BIAS", GLfloat, ["ctx->Pixel.RedBias"], "", NoState, NoExt ),
	( "GL_RED_BITS", GLint, ["ctx->DrawBuffer->Visual.redBits"], "", "_NEW_BUFFERS", NoExt ),
	( "GL_RED_SCALE", GLfloat, ["ctx->Pixel.RedScale"], "", NoState, NoExt ),
	( "GL_RENDER_MODE", GLenum, ["ctx->RenderMode"], "", NoState, NoExt ),
	( "GL_RESCALE_NORMAL", GLboolean,
	  ["ctx->Transform.RescaleNormals"], "", NoState, NoExt ),
	( "GL_RGBA_MODE", GLboolean, ["GL_TRUE"],
	  "", NoState, NoExt ),
	( "GL_SCISSOR_BOX", GLint,
	  ["ctx->Scissor.X",
	   "ctx->Scissor.Y",
	   "ctx->Scissor.Width",
	   "ctx->Scissor.Height"], "", NoState, NoExt ),
	( "GL_SCISSOR_TEST", GLboolean, ["ctx->Scissor.Enabled"], "", NoState, NoExt ),
	( "GL_SELECTION_BUFFER_SIZE", GLint, ["ctx->Select.BufferSize"], "", NoState, NoExt ),
	( "GL_SHADE_MODEL", GLenum, ["ctx->Light.ShadeModel"], "", NoState, NoExt ),
	( "GL_SHARED_TEXTURE_PALETTE_EXT", GLboolean,
	  ["ctx->Texture.SharedPalette"], "", NoState, NoExt ),
	( "GL_STENCIL_BITS", GLint, ["ctx->DrawBuffer->Visual.stencilBits"], "", NoState, NoExt ),
	( "GL_STENCIL_CLEAR_VALUE", GLint, ["ctx->Stencil.Clear"], "", NoState, NoExt ),
	( "GL_STENCIL_FAIL", GLenum,
	  ["ctx->Stencil.FailFunc[ctx->Stencil.ActiveFace]"], "", NoState, NoExt ),
	( "GL_STENCIL_FUNC", GLenum,
	  ["ctx->Stencil.Function[ctx->Stencil.ActiveFace]"], "", NoState, NoExt ),
	( "GL_STENCIL_PASS_DEPTH_FAIL", GLenum,
	  ["ctx->Stencil.ZFailFunc[ctx->Stencil.ActiveFace]"], "", NoState, NoExt ),
	( "GL_STENCIL_PASS_DEPTH_PASS", GLenum,
	  ["ctx->Stencil.ZPassFunc[ctx->Stencil.ActiveFace]"], "", NoState, NoExt ),
	( "GL_STENCIL_REF", GLint,
	  ["ctx->Stencil.Ref[ctx->Stencil.ActiveFace]"], "", NoState, NoExt ),
	( "GL_STENCIL_TEST", GLboolean, ["ctx->Stencil.Enabled"], "", NoState, NoExt ),
	( "GL_STENCIL_VALUE_MASK", GLint,
	  ["ctx->Stencil.ValueMask[ctx->Stencil.ActiveFace]"], "", NoState, NoExt ),
	( "GL_STENCIL_WRITEMASK", GLint,
	  ["ctx->Stencil.WriteMask[ctx->Stencil.ActiveFace]"], "", NoState, NoExt ),
	( "GL_STEREO", GLboolean, ["ctx->DrawBuffer->Visual.stereoMode"],
	  "", NoState, NoExt ),
	( "GL_SUBPIXEL_BITS", GLint, ["ctx->Const.SubPixelBits"], "", NoState, NoExt ),
	( "GL_TEXTURE_1D", GLboolean, ["_mesa_IsEnabled(GL_TEXTURE_1D)"], "", NoState, NoExt ),
	( "GL_TEXTURE_2D", GLboolean, ["_mesa_IsEnabled(GL_TEXTURE_2D)"], "", NoState, NoExt ),
	( "GL_TEXTURE_3D", GLboolean, ["_mesa_IsEnabled(GL_TEXTURE_3D)"], "", NoState, NoExt ),
	( "GL_TEXTURE_1D_ARRAY_EXT", GLboolean, ["_mesa_IsEnabled(GL_TEXTURE_1D_ARRAY_EXT)"], "", NoState, ["MESA_texture_array"] ),
	( "GL_TEXTURE_2D_ARRAY_EXT", GLboolean, ["_mesa_IsEnabled(GL_TEXTURE_2D_ARRAY_EXT)"], "", NoState, ["MESA_texture_array"] ),
	( "GL_TEXTURE_BINDING_1D", GLint,
	  ["ctx->Texture.Unit[ctx->Texture.CurrentUnit].CurrentTex[TEXTURE_1D_INDEX]->Name"], "", NoState, NoExt ),
	( "GL_TEXTURE_BINDING_2D", GLint,
	  ["ctx->Texture.Unit[ctx->Texture.CurrentUnit].CurrentTex[TEXTURE_2D_INDEX]->Name"], "", NoState, NoExt ),
	( "GL_TEXTURE_BINDING_3D", GLint,
	  ["ctx->Texture.Unit[ctx->Texture.CurrentUnit].CurrentTex[TEXTURE_3D_INDEX]->Name"], "", NoState, NoExt ),
	( "GL_TEXTURE_BINDING_1D_ARRAY_EXT", GLint,
	  ["ctx->Texture.Unit[ctx->Texture.CurrentUnit].CurrentTex[TEXTURE_1D_ARRAY_INDEX]->Name"], "", NoState, ["MESA_texture_array"] ),
	( "GL_TEXTURE_BINDING_2D_ARRAY_EXT", GLint,
	  ["ctx->Texture.Unit[ctx->Texture.CurrentUnit].CurrentTex[TEXTURE_2D_ARRAY_INDEX]->Name"], "", NoState, ["MESA_texture_array"] ),
	( "GL_MAX_ARRAY_TEXTURE_LAYERS_EXT", GLint,
	  ["ctx->Const.MaxArrayTextureLayers"], "", NoState, ["MESA_texture_array"] ),
	( "GL_TEXTURE_GEN_S", GLboolean,
	  ["((ctx->Texture.Unit[ctx->Texture.CurrentUnit].TexGenEnabled & S_BIT) ? 1 : 0)"], "", NoState, NoExt ),
	( "GL_TEXTURE_GEN_T", GLboolean,
	  ["((ctx->Texture.Unit[ctx->Texture.CurrentUnit].TexGenEnabled & T_BIT) ? 1 : 0)"], "", NoState, NoExt ),
	( "GL_TEXTURE_GEN_R", GLboolean,
	  ["((ctx->Texture.Unit[ctx->Texture.CurrentUnit].TexGenEnabled & R_BIT) ? 1 : 0)"], "", NoState, NoExt ),
	( "GL_TEXTURE_GEN_Q", GLboolean,
	  ["((ctx->Texture.Unit[ctx->Texture.CurrentUnit].TexGenEnabled & Q_BIT) ? 1 : 0)"], "", NoState, NoExt ),
	( "GL_TEXTURE_MATRIX", GLfloat,
	  ["matrix[0]", "matrix[1]", "matrix[2]", "matrix[3]",
	   "matrix[4]", "matrix[5]", "matrix[6]", "matrix[7]",
	   "matrix[8]", "matrix[9]", "matrix[10]", "matrix[11]",
	   "matrix[12]", "matrix[13]", "matrix[14]", "matrix[15]" ],
	  """const GLfloat *matrix;
         const GLuint unit = ctx->Texture.CurrentUnit;
         if (unit >= ctx->Const.MaxTextureCoordUnits) {
            _mesa_error(ctx, GL_INVALID_OPERATION, "glGet(texture matrix %u)",
                        unit);
            return;
         }
         matrix = ctx->TextureMatrixStack[unit].Top->m;""",
	  NoState, NoExt ),
	( "GL_TEXTURE_STACK_DEPTH", GLint,
	  ["ctx->TextureMatrixStack[unit].Depth + 1"],
	  """const GLuint unit = ctx->Texture.CurrentUnit;
         if (unit >= ctx->Const.MaxTextureCoordUnits) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glGet(texture stack depth, unit %u)", unit);
            return;
         }""",
	  NoState, NoExt ),
	( "GL_UNPACK_ALIGNMENT", GLint, ["ctx->Unpack.Alignment"], "", NoState, NoExt ),
	( "GL_UNPACK_LSB_FIRST", GLboolean, ["ctx->Unpack.LsbFirst"], "", NoState, NoExt ),
	( "GL_UNPACK_ROW_LENGTH", GLint, ["ctx->Unpack.RowLength"], "", NoState, NoExt ),
	( "GL_UNPACK_SKIP_PIXELS", GLint, ["ctx->Unpack.SkipPixels"], "", NoState, NoExt ),
	( "GL_UNPACK_SKIP_ROWS", GLint, ["ctx->Unpack.SkipRows"], "", NoState, NoExt ),
	( "GL_UNPACK_SWAP_BYTES", GLboolean, ["ctx->Unpack.SwapBytes"], "", NoState, NoExt ),
	( "GL_UNPACK_SKIP_IMAGES_EXT", GLint, ["ctx->Unpack.SkipImages"], "", NoState, NoExt ),
	( "GL_UNPACK_IMAGE_HEIGHT_EXT", GLint, ["ctx->Unpack.ImageHeight"], "", NoState, NoExt ),
	( "GL_UNPACK_CLIENT_STORAGE_APPLE", GLboolean, ["ctx->Unpack.ClientStorage"], "", NoState, NoExt ),
	( "GL_VIEWPORT", GLint, [ "ctx->Viewport.X", "ctx->Viewport.Y",
	  "ctx->Viewport.Width", "ctx->Viewport.Height" ], "", NoState, NoExt ),
	( "GL_ZOOM_X", GLfloat, ["ctx->Pixel.ZoomX"], "", NoState, NoExt ),
	( "GL_ZOOM_Y", GLfloat, ["ctx->Pixel.ZoomY"], "", NoState, NoExt ),

	# Vertex arrays
	( "GL_VERTEX_ARRAY", GLboolean, ["ctx->Array.ArrayObj->Vertex.Enabled"], "", NoState, NoExt ),
	( "GL_VERTEX_ARRAY_SIZE", GLint, ["ctx->Array.ArrayObj->Vertex.Size"], "", NoState, NoExt ),
	( "GL_VERTEX_ARRAY_TYPE", GLenum, ["ctx->Array.ArrayObj->Vertex.Type"], "", NoState, NoExt ),
	( "GL_VERTEX_ARRAY_STRIDE", GLint, ["ctx->Array.ArrayObj->Vertex.Stride"], "", NoState, NoExt ),
	( "GL_VERTEX_ARRAY_COUNT_EXT", GLint, ["0"], "", NoState, NoExt ),
	( "GL_NORMAL_ARRAY", GLenum, ["ctx->Array.ArrayObj->Normal.Enabled"], "", NoState, NoExt ),
	( "GL_NORMAL_ARRAY_TYPE", GLenum, ["ctx->Array.ArrayObj->Normal.Type"], "", NoState, NoExt ),
	( "GL_NORMAL_ARRAY_STRIDE", GLint, ["ctx->Array.ArrayObj->Normal.Stride"], "", NoState, NoExt ),
	( "GL_NORMAL_ARRAY_COUNT_EXT", GLint, ["0"], "", NoState, NoExt ),
	( "GL_COLOR_ARRAY", GLboolean, ["ctx->Array.ArrayObj->Color.Enabled"], "", NoState, NoExt ),
	( "GL_COLOR_ARRAY_SIZE", GLint, ["ctx->Array.ArrayObj->Color.Size"], "", NoState, NoExt ),
	( "GL_COLOR_ARRAY_TYPE", GLenum, ["ctx->Array.ArrayObj->Color.Type"], "", NoState, NoExt ),
	( "GL_COLOR_ARRAY_STRIDE", GLint, ["ctx->Array.ArrayObj->Color.Stride"], "", NoState, NoExt ),
	( "GL_COLOR_ARRAY_COUNT_EXT", GLint, ["0"], "", NoState, NoExt ),
	( "GL_INDEX_ARRAY", GLboolean, ["ctx->Array.ArrayObj->Index.Enabled"], "", NoState, NoExt ),
	( "GL_INDEX_ARRAY_TYPE", GLenum, ["ctx->Array.ArrayObj->Index.Type"], "", NoState, NoExt ),
	( "GL_INDEX_ARRAY_STRIDE", GLint, ["ctx->Array.ArrayObj->Index.Stride"], "", NoState, NoExt ),
	( "GL_INDEX_ARRAY_COUNT_EXT", GLint, ["0"], "", NoState, NoExt ),
	( "GL_TEXTURE_COORD_ARRAY", GLboolean,
	  ["ctx->Array.ArrayObj->TexCoord[ctx->Array.ActiveTexture].Enabled"], "", NoState, NoExt ),
	( "GL_TEXTURE_COORD_ARRAY_SIZE", GLint,
	  ["ctx->Array.ArrayObj->TexCoord[ctx->Array.ActiveTexture].Size"], "", NoState, NoExt ),
	( "GL_TEXTURE_COORD_ARRAY_TYPE", GLenum,
	  ["ctx->Array.ArrayObj->TexCoord[ctx->Array.ActiveTexture].Type"], "", NoState, NoExt ),
	( "GL_TEXTURE_COORD_ARRAY_STRIDE", GLint,
	  ["ctx->Array.ArrayObj->TexCoord[ctx->Array.ActiveTexture].Stride"], "", NoState, NoExt ),
	( "GL_TEXTURE_COORD_ARRAY_COUNT_EXT", GLint, ["0"], "", NoState, NoExt ),
	( "GL_EDGE_FLAG_ARRAY", GLboolean, ["ctx->Array.ArrayObj->EdgeFlag.Enabled"], "", NoState, NoExt ),
	( "GL_EDGE_FLAG_ARRAY_STRIDE", GLint, ["ctx->Array.ArrayObj->EdgeFlag.Stride"], "", NoState, NoExt ),
	( "GL_EDGE_FLAG_ARRAY_COUNT_EXT", GLint, ["0"], "", NoState, NoExt ),

	# GL_ARB_multitexture
	( "GL_MAX_TEXTURE_UNITS_ARB", GLint,
	  ["ctx->Const.MaxTextureUnits"], "", NoState, ["ARB_multitexture"] ),
	( "GL_ACTIVE_TEXTURE_ARB", GLint,
	  [ "GL_TEXTURE0_ARB + ctx->Texture.CurrentUnit"], "", NoState, ["ARB_multitexture"] ),
	( "GL_CLIENT_ACTIVE_TEXTURE_ARB", GLint,
	  ["GL_TEXTURE0_ARB + ctx->Array.ActiveTexture"], "", NoState, ["ARB_multitexture"] ),

	# GL_ARB_texture_cube_map
	( "GL_TEXTURE_CUBE_MAP_ARB", GLboolean,
	  ["_mesa_IsEnabled(GL_TEXTURE_CUBE_MAP_ARB)"], "", NoState, ["ARB_texture_cube_map"] ),
	( "GL_TEXTURE_BINDING_CUBE_MAP_ARB", GLint,
	  ["ctx->Texture.Unit[ctx->Texture.CurrentUnit].CurrentTex[TEXTURE_CUBE_INDEX]->Name"],
	  "", NoState, ["ARB_texture_cube_map"] ),
	( "GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB", GLint,
	  ["(1 << (ctx->Const.MaxCubeTextureLevels - 1))"],
	  "", NoState, ["ARB_texture_cube_map"]),

	# GL_ARB_texture_compression */
	( "GL_TEXTURE_COMPRESSION_HINT_ARB", GLint,
	  ["ctx->Hint.TextureCompression"], "", NoState, NoExt ),
	( "GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB", GLint,
	  ["_mesa_get_compressed_formats(ctx, NULL, GL_FALSE)"],
	  "", NoState, NoExt ),
	( "GL_COMPRESSED_TEXTURE_FORMATS_ARB", GLenum,
	  [],
	  """GLint formats[100];
         GLuint i, n = _mesa_get_compressed_formats(ctx, formats, GL_FALSE);
         ASSERT(n <= 100);
         for (i = 0; i < n; i++)
            params[i] = CONVERSION(formats[i]);""",
	  NoState, NoExt ),

	# GL_EXT_compiled_vertex_array
	( "GL_ARRAY_ELEMENT_LOCK_FIRST_EXT", GLint, ["ctx->Array.LockFirst"],
	  "", NoState, ["EXT_compiled_vertex_array"] ),
	( "GL_ARRAY_ELEMENT_LOCK_COUNT_EXT", GLint, ["ctx->Array.LockCount"],
	  "", NoState, ["EXT_compiled_vertex_array"] ),

	# GL_ARB_transpose_matrix
	( "GL_TRANSPOSE_COLOR_MATRIX_ARB", GLfloat,
	  ["matrix[0]", "matrix[4]", "matrix[8]", "matrix[12]",
	   "matrix[1]", "matrix[5]", "matrix[9]", "matrix[13]",
	   "matrix[2]", "matrix[6]", "matrix[10]", "matrix[14]",
	   "matrix[3]", "matrix[7]", "matrix[11]", "matrix[15]"],
	  "const GLfloat *matrix = ctx->ColorMatrixStack.Top->m;",
	  NoState, NoExt ),
	( "GL_TRANSPOSE_MODELVIEW_MATRIX_ARB", GLfloat,
	  ["matrix[0]", "matrix[4]", "matrix[8]", "matrix[12]",
	   "matrix[1]", "matrix[5]", "matrix[9]", "matrix[13]",
	   "matrix[2]", "matrix[6]", "matrix[10]", "matrix[14]",
	   "matrix[3]", "matrix[7]", "matrix[11]", "matrix[15]"],
	  "const GLfloat *matrix = ctx->ModelviewMatrixStack.Top->m;",
	  NoState, NoExt ),
	( "GL_TRANSPOSE_PROJECTION_MATRIX_ARB", GLfloat,
	  ["matrix[0]", "matrix[4]", "matrix[8]", "matrix[12]",
	   "matrix[1]", "matrix[5]", "matrix[9]", "matrix[13]",
	   "matrix[2]", "matrix[6]", "matrix[10]", "matrix[14]",
	   "matrix[3]", "matrix[7]", "matrix[11]", "matrix[15]"],
	  "const GLfloat *matrix = ctx->ProjectionMatrixStack.Top->m;",
	  NoState, NoExt ),
	( "GL_TRANSPOSE_TEXTURE_MATRIX_ARB", GLfloat,
	  ["matrix[0]", "matrix[4]", "matrix[8]", "matrix[12]",
	   "matrix[1]", "matrix[5]", "matrix[9]", "matrix[13]",
	   "matrix[2]", "matrix[6]", "matrix[10]", "matrix[14]",
	   "matrix[3]", "matrix[7]", "matrix[11]", "matrix[15]"],
	  "const GLfloat *matrix = ctx->TextureMatrixStack[ctx->Texture.CurrentUnit].Top->m;",
	  NoState, NoExt ),

	# GL_SGI_color_matrix (also in 1.2 imaging)
	( "GL_COLOR_MATRIX_SGI", GLfloat,
	  ["matrix[0]", "matrix[1]", "matrix[2]", "matrix[3]",
	   "matrix[4]", "matrix[5]", "matrix[6]", "matrix[7]",
	   "matrix[8]", "matrix[9]", "matrix[10]", "matrix[11]",
	   "matrix[12]", "matrix[13]", "matrix[14]", "matrix[15]" ],
	  "const GLfloat *matrix = ctx->ColorMatrixStack.Top->m;",
	  NoState, NoExt ),
	( "GL_COLOR_MATRIX_STACK_DEPTH_SGI", GLint,
	  ["ctx->ColorMatrixStack.Depth + 1"], "", NoState, NoExt ),
	( "GL_MAX_COLOR_MATRIX_STACK_DEPTH_SGI", GLint,
	  ["MAX_COLOR_STACK_DEPTH"], "", NoState, NoExt ),
	( "GL_POST_COLOR_MATRIX_RED_SCALE_SGI", GLfloat,
	  ["ctx->Pixel.PostColorMatrixScale[0]"], "", NoState, NoExt ),
	( "GL_POST_COLOR_MATRIX_GREEN_SCALE_SGI", GLfloat,
	  ["ctx->Pixel.PostColorMatrixScale[1]"], "", NoState, NoExt ),
	( "GL_POST_COLOR_MATRIX_BLUE_SCALE_SGI", GLfloat,
	  ["ctx->Pixel.PostColorMatrixScale[2]"], "", NoState, NoExt ),
	( "GL_POST_COLOR_MATRIX_ALPHA_SCALE_SGI", GLfloat,
	  ["ctx->Pixel.PostColorMatrixScale[3]"], "", NoState, NoExt ),
	( "GL_POST_COLOR_MATRIX_RED_BIAS_SGI", GLfloat,
	  ["ctx->Pixel.PostColorMatrixBias[0]"], "", NoState, NoExt ),
	( "GL_POST_COLOR_MATRIX_GREEN_BIAS_SGI", GLfloat,
	  ["ctx->Pixel.PostColorMatrixBias[1]"], "", NoState, NoExt ),
	( "GL_POST_COLOR_MATRIX_BLUE_BIAS_SGI", GLfloat,
	  ["ctx->Pixel.PostColorMatrixBias[2]"], "", NoState, NoExt ),
	( "GL_POST_COLOR_MATRIX_ALPHA_BIAS_SGI", GLfloat,
	  ["ctx->Pixel.PostColorMatrixBias[3]"], "", NoState, NoExt ),

	# GL_EXT_convolution (also in 1.2 imaging)
	( "GL_CONVOLUTION_1D_EXT", GLboolean,
	  ["ctx->Pixel.Convolution1DEnabled"], "", NoState, ["EXT_convolution"] ),
	( "GL_CONVOLUTION_2D_EXT", GLboolean,
	  ["ctx->Pixel.Convolution2DEnabled"], "", NoState, ["EXT_convolution"] ),
	( "GL_SEPARABLE_2D_EXT", GLboolean,
	  ["ctx->Pixel.Separable2DEnabled"], "", NoState, ["EXT_convolution"] ),
	( "GL_POST_CONVOLUTION_RED_SCALE_EXT", GLfloat,
	  ["ctx->Pixel.PostConvolutionScale[0]"], "", NoState, ["EXT_convolution"] ),
	( "GL_POST_CONVOLUTION_GREEN_SCALE_EXT", GLfloat,
	  ["ctx->Pixel.PostConvolutionScale[1]"], "", NoState, ["EXT_convolution"] ),
	( "GL_POST_CONVOLUTION_BLUE_SCALE_EXT", GLfloat,
	  ["ctx->Pixel.PostConvolutionScale[2]"], "", NoState, ["EXT_convolution"] ),
	( "GL_POST_CONVOLUTION_ALPHA_SCALE_EXT", GLfloat,
	  ["ctx->Pixel.PostConvolutionScale[3]"], "", NoState, ["EXT_convolution"] ),
	( "GL_POST_CONVOLUTION_RED_BIAS_EXT", GLfloat,
	  ["ctx->Pixel.PostConvolutionBias[0]"], "", NoState, ["EXT_convolution"] ),
	( "GL_POST_CONVOLUTION_GREEN_BIAS_EXT", GLfloat,
	  ["ctx->Pixel.PostConvolutionBias[1]"], "", NoState, ["EXT_convolution"] ),
	( "GL_POST_CONVOLUTION_BLUE_BIAS_EXT", GLfloat,
	  ["ctx->Pixel.PostConvolutionBias[2]"], "", NoState, ["EXT_convolution"] ),
	( "GL_POST_CONVOLUTION_ALPHA_BIAS_EXT", GLfloat,
	  ["ctx->Pixel.PostConvolutionBias[3]"], "", NoState, ["EXT_convolution"] ),

	# GL_EXT_histogram / GL_ARB_imaging
	( "GL_HISTOGRAM", GLboolean,
	  [ "ctx->Pixel.HistogramEnabled" ], "", NoState, ["EXT_histogram"] ),
	( "GL_MINMAX", GLboolean,
	  [ "ctx->Pixel.MinMaxEnabled" ], "", NoState, ["EXT_histogram"] ),

	# GL_SGI_color_table / GL_ARB_imaging
	( "GL_COLOR_TABLE_SGI", GLboolean,
	  ["ctx->Pixel.ColorTableEnabled[COLORTABLE_PRECONVOLUTION]"], "",
	  NoState, ["SGI_color_table"] ),
	( "GL_POST_CONVOLUTION_COLOR_TABLE_SGI", GLboolean,
	  ["ctx->Pixel.ColorTableEnabled[COLORTABLE_POSTCONVOLUTION]"], "",
	  NoState, ["SGI_color_table"] ),
	( "GL_POST_COLOR_MATRIX_COLOR_TABLE_SGI", GLboolean,
	  ["ctx->Pixel.ColorTableEnabled[COLORTABLE_POSTCOLORMATRIX]"], "",
	  NoState, ["SGI_color_table"] ),

	# GL_SGI_texture_color_table
	( "GL_TEXTURE_COLOR_TABLE_SGI", GLboolean,
	  ["ctx->Texture.Unit[ctx->Texture.CurrentUnit].ColorTableEnabled"],
	  "", NoState, ["SGI_texture_color_table"] ),

	# GL_EXT_secondary_color
	( "GL_COLOR_SUM_EXT", GLboolean,
	  ["ctx->Fog.ColorSumEnabled"], "", NoState,
	  ["EXT_secondary_color", "ARB_vertex_program"] ),
	( "GL_CURRENT_SECONDARY_COLOR_EXT", GLfloatN,
	  ["ctx->Current.Attrib[VERT_ATTRIB_COLOR1][0]",
	   "ctx->Current.Attrib[VERT_ATTRIB_COLOR1][1]",
	   "ctx->Current.Attrib[VERT_ATTRIB_COLOR1][2]",
	   "ctx->Current.Attrib[VERT_ATTRIB_COLOR1][3]"],
	  "", FlushCurrent, ["EXT_secondary_color"] ),
	( "GL_SECONDARY_COLOR_ARRAY_EXT", GLboolean,
	  ["ctx->Array.ArrayObj->SecondaryColor.Enabled"],
	  "", NoState, ["EXT_secondary_color"] ),
	( "GL_SECONDARY_COLOR_ARRAY_TYPE_EXT", GLenum,
	  ["ctx->Array.ArrayObj->SecondaryColor.Type"],
	  "", NoState,  ["EXT_secondary_color"] ),
	( "GL_SECONDARY_COLOR_ARRAY_STRIDE_EXT", GLint,
	  ["ctx->Array.ArrayObj->SecondaryColor.Stride"],
	  "", NoState, ["EXT_secondary_color"] ),
	( "GL_SECONDARY_COLOR_ARRAY_SIZE_EXT", GLint,
	  ["ctx->Array.ArrayObj->SecondaryColor.Size"],
	  "", NoState, ["EXT_secondary_color"] ),

	# GL_EXT_fog_coord
	( "GL_CURRENT_FOG_COORDINATE_EXT", GLfloat,
	  ["ctx->Current.Attrib[VERT_ATTRIB_FOG][0]"],
	  "", FlushCurrent, ["EXT_fog_coord"] ),
	( "GL_FOG_COORDINATE_ARRAY_EXT", GLboolean,
	  ["ctx->Array.ArrayObj->FogCoord.Enabled"],
	  "", NoState,  ["EXT_fog_coord"] ),
	( "GL_FOG_COORDINATE_ARRAY_TYPE_EXT", GLenum,
	  ["ctx->Array.ArrayObj->FogCoord.Type"],
	  "", NoState, ["EXT_fog_coord"] ),
	( "GL_FOG_COORDINATE_ARRAY_STRIDE_EXT", GLint,
	  ["ctx->Array.ArrayObj->FogCoord.Stride"],
	  "", NoState, ["EXT_fog_coord"] ),
	( "GL_FOG_COORDINATE_SOURCE_EXT", GLenum,
	  ["ctx->Fog.FogCoordinateSource"],
	  "", NoState, ["EXT_fog_coord"] ),

	# GL_EXT_texture_lod_bias
	( "GL_MAX_TEXTURE_LOD_BIAS_EXT", GLfloat,
	  ["ctx->Const.MaxTextureLodBias"], "", NoState, ["EXT_texture_lod_bias"]),

	# GL_EXT_texture_filter_anisotropic
	( "GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT", GLfloat,
	  ["ctx->Const.MaxTextureMaxAnisotropy"],
	  "", NoState, ["EXT_texture_filter_anisotropic"]),

	# GL_ARB_multisample
	( "GL_MULTISAMPLE_ARB", GLboolean,
	  ["ctx->Multisample.Enabled"], "", NoState, NoExt ),
	( "GL_SAMPLE_ALPHA_TO_COVERAGE_ARB", GLboolean,
	  ["ctx->Multisample.SampleAlphaToCoverage"], "", NoState, NoExt ),
	( "GL_SAMPLE_ALPHA_TO_ONE_ARB", GLboolean,
	  ["ctx->Multisample.SampleAlphaToOne"], "", NoState, NoExt ),
	( "GL_SAMPLE_COVERAGE_ARB", GLboolean,
	  ["ctx->Multisample.SampleCoverage"], "", NoState, NoExt ),
	( "GL_SAMPLE_COVERAGE_VALUE_ARB", GLfloat,
	  ["ctx->Multisample.SampleCoverageValue"], "", NoState, NoExt ),
	( "GL_SAMPLE_COVERAGE_INVERT_ARB", GLboolean,
	  ["ctx->Multisample.SampleCoverageInvert"], "", NoState, NoExt ),
	( "GL_SAMPLE_BUFFERS_ARB", GLint,
	  ["ctx->DrawBuffer->Visual.sampleBuffers"], "", NoState, NoExt ),
	( "GL_SAMPLES_ARB", GLint,
	  ["ctx->DrawBuffer->Visual.samples"], "", NoState, NoExt ),

	# GL_IBM_rasterpos_clip
	( "GL_RASTER_POSITION_UNCLIPPED_IBM", GLboolean,
	  ["ctx->Transform.RasterPositionUnclipped"],
	  "", NoState, ["IBM_rasterpos_clip"] ),

	# GL_NV_point_sprite
	( "GL_POINT_SPRITE_NV", GLboolean, ["ctx->Point.PointSprite"], # == GL_POINT_SPRITE_ARB
	  "", NoState, ["NV_point_sprite", "ARB_point_sprite"] ),
	( "GL_POINT_SPRITE_R_MODE_NV", GLenum, ["ctx->Point.SpriteRMode"],
	  "", NoState, ["NV_point_sprite"] ),
	( "GL_POINT_SPRITE_COORD_ORIGIN", GLenum, ["ctx->Point.SpriteOrigin"],
	  "", NoState, ["NV_point_sprite", "ARB_point_sprite"] ),

	# GL_SGIS_generate_mipmap
	( "GL_GENERATE_MIPMAP_HINT_SGIS", GLenum, ["ctx->Hint.GenerateMipmap"],
	  "", NoState, ["SGIS_generate_mipmap"] ),

	# GL_NV_vertex_program
	( "GL_VERTEX_PROGRAM_BINDING_NV", GLint,
	  ["(ctx->VertexProgram.Current ? ctx->VertexProgram.Current->Base.Id : 0)"],
	  "", NoState, ["NV_vertex_program"] ),
	( "GL_VERTEX_ATTRIB_ARRAY0_NV", GLboolean,
	  ["ctx->Array.ArrayObj->VertexAttrib[0].Enabled"],
	  "", NoState, ["NV_vertex_program"] ),
	( "GL_VERTEX_ATTRIB_ARRAY1_NV", GLboolean,
	  ["ctx->Array.ArrayObj->VertexAttrib[1].Enabled"],
	  "", NoState, ["NV_vertex_program"] ),
	( "GL_VERTEX_ATTRIB_ARRAY2_NV", GLboolean,
	  ["ctx->Array.ArrayObj->VertexAttrib[2].Enabled"],
	  "", NoState, ["NV_vertex_program"] ),
	( "GL_VERTEX_ATTRIB_ARRAY3_NV", GLboolean,
	  ["ctx->Array.ArrayObj->VertexAttrib[3].Enabled"],
	  "", NoState, ["NV_vertex_program"] ),
	( "GL_VERTEX_ATTRIB_ARRAY4_NV", GLboolean,
	  ["ctx->Array.ArrayObj->VertexAttrib[4].Enabled"],
	  "", NoState, ["NV_vertex_program"] ),
	( "GL_VERTEX_ATTRIB_ARRAY5_NV", GLboolean,
	  ["ctx->Array.ArrayObj->VertexAttrib[5].Enabled"],
	  "", NoState, ["NV_vertex_program"] ),
	( "GL_VERTEX_ATTRIB_ARRAY6_NV", GLboolean,
	  ["ctx->Array.ArrayObj->VertexAttrib[6].Enabled"],
	  "", NoState, ["NV_vertex_program"] ),
	( "GL_VERTEX_ATTRIB_ARRAY7_NV", GLboolean,
	  ["ctx->Array.ArrayObj->VertexAttrib[7].Enabled"],
	  "", NoState, ["NV_vertex_program"] ),
	( "GL_VERTEX_ATTRIB_ARRAY8_NV", GLboolean,
	  ["ctx->Array.ArrayObj->VertexAttrib[8].Enabled"],
	  "", NoState, ["NV_vertex_program"] ),
	( "GL_VERTEX_ATTRIB_ARRAY9_NV", GLboolean,
	  ["ctx->Array.ArrayObj->VertexAttrib[9].Enabled"],
	  "", NoState, ["NV_vertex_program"] ),
	( "GL_VERTEX_ATTRIB_ARRAY10_NV", GLboolean,
	  ["ctx->Array.ArrayObj->VertexAttrib[10].Enabled"],
	  "", NoState, ["NV_vertex_program"] ),
	( "GL_VERTEX_ATTRIB_ARRAY11_NV", GLboolean,
	  ["ctx->Array.ArrayObj->VertexAttrib[11].Enabled"],
	  "", NoState, ["NV_vertex_program"] ),
	( "GL_VERTEX_ATTRIB_ARRAY12_NV", GLboolean,
	  ["ctx->Array.ArrayObj->VertexAttrib[12].Enabled"],
	  "", NoState, ["NV_vertex_program"] ),
	( "GL_VERTEX_ATTRIB_ARRAY13_NV", GLboolean,
	  ["ctx->Array.ArrayObj->VertexAttrib[13].Enabled"],
	  "", NoState, ["NV_vertex_program"] ),
	( "GL_VERTEX_ATTRIB_ARRAY14_NV", GLboolean,
	  ["ctx->Array.ArrayObj->VertexAttrib[14].Enabled"],
	  "", NoState, ["NV_vertex_program"] ),
	( "GL_VERTEX_ATTRIB_ARRAY15_NV", GLboolean,
	  ["ctx->Array.ArrayObj->VertexAttrib[15].Enabled"],
	  "", NoState, ["NV_vertex_program"] ),
	( "GL_MAP1_VERTEX_ATTRIB0_4_NV", GLboolean,
	  ["ctx->Eval.Map1Attrib[0]"], "", NoState, ["NV_vertex_program"] ),
	( "GL_MAP1_VERTEX_ATTRIB1_4_NV", GLboolean,
	  ["ctx->Eval.Map1Attrib[1]"], "", NoState, ["NV_vertex_program"] ),
	( "GL_MAP1_VERTEX_ATTRIB2_4_NV", GLboolean,
	  ["ctx->Eval.Map1Attrib[2]"], "", NoState, ["NV_vertex_program"] ),
	( "GL_MAP1_VERTEX_ATTRIB3_4_NV", GLboolean,
	  ["ctx->Eval.Map1Attrib[3]"], "", NoState, ["NV_vertex_program"] ),
	( "GL_MAP1_VERTEX_ATTRIB4_4_NV", GLboolean,
	  ["ctx->Eval.Map1Attrib[4]"], "", NoState, ["NV_vertex_program"] ),
	( "GL_MAP1_VERTEX_ATTRIB5_4_NV", GLboolean,
	  ["ctx->Eval.Map1Attrib[5]"], "", NoState, ["NV_vertex_program"] ),
	( "GL_MAP1_VERTEX_ATTRIB6_4_NV", GLboolean,
	  ["ctx->Eval.Map1Attrib[6]"], "", NoState, ["NV_vertex_program"] ),
	( "GL_MAP1_VERTEX_ATTRIB7_4_NV", GLboolean,
	  ["ctx->Eval.Map1Attrib[7]"], "", NoState, ["NV_vertex_program"] ),
	( "GL_MAP1_VERTEX_ATTRIB8_4_NV", GLboolean,
	  ["ctx->Eval.Map1Attrib[8]"], "", NoState, ["NV_vertex_program"] ),
	( "GL_MAP1_VERTEX_ATTRIB9_4_NV", GLboolean,
	  ["ctx->Eval.Map1Attrib[9]"], "", NoState, ["NV_vertex_program"] ),
	( "GL_MAP1_VERTEX_ATTRIB10_4_NV", GLboolean,
	  ["ctx->Eval.Map1Attrib[10]"], "", NoState, ["NV_vertex_program"] ),
	( "GL_MAP1_VERTEX_ATTRIB11_4_NV", GLboolean,
	  ["ctx->Eval.Map1Attrib[11]"], "", NoState, ["NV_vertex_program"] ),
	( "GL_MAP1_VERTEX_ATTRIB12_4_NV", GLboolean,
	  ["ctx->Eval.Map1Attrib[12]"], "", NoState, ["NV_vertex_program"] ),
	( "GL_MAP1_VERTEX_ATTRIB13_4_NV", GLboolean,
	  ["ctx->Eval.Map1Attrib[13]"], "", NoState, ["NV_vertex_program"] ),
	( "GL_MAP1_VERTEX_ATTRIB14_4_NV", GLboolean,
	  ["ctx->Eval.Map1Attrib[14]"], "", NoState, ["NV_vertex_program"] ),
	( "GL_MAP1_VERTEX_ATTRIB15_4_NV", GLboolean,
	  ["ctx->Eval.Map1Attrib[15]"], "", NoState, ["NV_vertex_program"] ),

	# GL_NV_fragment_program
	( "GL_FRAGMENT_PROGRAM_NV", GLboolean,
	  ["ctx->FragmentProgram.Enabled"], "", NoState, ["NV_fragment_program"] ),
	( "GL_FRAGMENT_PROGRAM_BINDING_NV", GLint,
	  ["ctx->FragmentProgram.Current ? ctx->FragmentProgram.Current->Base.Id : 0"],
	  "", NoState, ["NV_fragment_program"] ),
	( "GL_MAX_FRAGMENT_PROGRAM_LOCAL_PARAMETERS_NV", GLint,
	  ["MAX_NV_FRAGMENT_PROGRAM_PARAMS"], "", NoState, ["NV_fragment_program"] ),

	# GL_NV_texture_rectangle
	( "GL_TEXTURE_RECTANGLE_NV", GLboolean,
	  ["_mesa_IsEnabled(GL_TEXTURE_RECTANGLE_NV)"], "", NoState, ["NV_texture_rectangle"] ),
	( "GL_TEXTURE_BINDING_RECTANGLE_NV", GLint,
	  ["ctx->Texture.Unit[ctx->Texture.CurrentUnit].CurrentTex[TEXTURE_RECT_INDEX]->Name"],
	  "", NoState, ["NV_texture_rectangle"] ),
	( "GL_MAX_RECTANGLE_TEXTURE_SIZE_NV", GLint,
	  ["ctx->Const.MaxTextureRectSize"], "", NoState, ["NV_texture_rectangle"] ),

	# GL_EXT_stencil_two_side
	( "GL_STENCIL_TEST_TWO_SIDE_EXT", GLboolean,
	  ["ctx->Stencil.TestTwoSide"], "", NoState, ["EXT_stencil_two_side"] ),
	( "GL_ACTIVE_STENCIL_FACE_EXT", GLenum,
	  ["ctx->Stencil.ActiveFace ? GL_BACK : GL_FRONT"],
	  "", NoState, ["EXT_stencil_two_side"] ),

	# GL_NV_light_max_exponent
	( "GL_MAX_SHININESS_NV", GLfloat,
	  ["ctx->Const.MaxShininess"], "", NoState, ["NV_light_max_exponent"] ),
	( "GL_MAX_SPOT_EXPONENT_NV", GLfloat,
	  ["ctx->Const.MaxSpotExponent"], "", NoState, ["NV_light_max_exponent"] ),

	# GL_ARB_vertex_buffer_object
	( "GL_ARRAY_BUFFER_BINDING_ARB", GLint,
	  ["ctx->Array.ArrayBufferObj->Name"], "", NoState, NoExt ),
	( "GL_VERTEX_ARRAY_BUFFER_BINDING_ARB", GLint,
	  ["ctx->Array.ArrayObj->Vertex.BufferObj->Name"], "", NoState, NoExt ),
	( "GL_NORMAL_ARRAY_BUFFER_BINDING_ARB", GLint,
	  ["ctx->Array.ArrayObj->Normal.BufferObj->Name"], "", NoState, NoExt ),
	( "GL_COLOR_ARRAY_BUFFER_BINDING_ARB", GLint,
	  ["ctx->Array.ArrayObj->Color.BufferObj->Name"], "", NoState, NoExt ),
	( "GL_INDEX_ARRAY_BUFFER_BINDING_ARB", GLint,
	  ["ctx->Array.ArrayObj->Index.BufferObj->Name"], "", NoState, NoExt ),
	( "GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING_ARB", GLint,
	  ["ctx->Array.ArrayObj->TexCoord[ctx->Array.ActiveTexture].BufferObj->Name"],
	  "", NoState, NoExt ),
	( "GL_EDGE_FLAG_ARRAY_BUFFER_BINDING_ARB", GLint,
	  ["ctx->Array.ArrayObj->EdgeFlag.BufferObj->Name"], "", NoState, NoExt ),
	( "GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING_ARB", GLint,
	  ["ctx->Array.ArrayObj->SecondaryColor.BufferObj->Name"],
	  "", NoState, NoExt ),
	( "GL_FOG_COORDINATE_ARRAY_BUFFER_BINDING_ARB", GLint,
	  ["ctx->Array.ArrayObj->FogCoord.BufferObj->Name"],
	  "", NoState, NoExt ),
	# GL_WEIGHT_ARRAY_BUFFER_BINDING_ARB - not supported
	( "GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB", GLint,
	  ["ctx->Array.ElementArrayBufferObj->Name"],
	  "", NoState, NoExt ),

	# GL_EXT_pixel_buffer_object
	( "GL_PIXEL_PACK_BUFFER_BINDING_EXT", GLint,
	  ["ctx->Pack.BufferObj->Name"], "", NoState, ["EXT_pixel_buffer_object"] ),
	( "GL_PIXEL_UNPACK_BUFFER_BINDING_EXT", GLint,
	  ["ctx->Unpack.BufferObj->Name"], "", NoState, ["EXT_pixel_buffer_object"] ),

	# GL_ARB_vertex_program
	( "GL_VERTEX_PROGRAM_ARB", GLboolean, # == GL_VERTEX_PROGRAM_NV
	  ["ctx->VertexProgram.Enabled"], "", NoState,
	  ["ARB_vertex_program", "NV_vertex_program"] ),
	( "GL_VERTEX_PROGRAM_POINT_SIZE_ARB", GLboolean, # == GL_VERTEX_PROGRAM_POINT_SIZE_NV
	  ["ctx->VertexProgram.PointSizeEnabled"], "", NoState,
	  ["ARB_vertex_program", "NV_vertex_program"] ),
	( "GL_VERTEX_PROGRAM_TWO_SIDE_ARB", GLboolean, # == GL_VERTEX_PROGRAM_TWO_SIDE_NV
	  ["ctx->VertexProgram.TwoSideEnabled"], "", NoState,
	  ["ARB_vertex_program", "NV_vertex_program"] ),
	( "GL_MAX_PROGRAM_MATRIX_STACK_DEPTH_ARB", GLint, # == GL_MAX_TRACK_MATRIX_STACK_DEPTH_NV
	  ["ctx->Const.MaxProgramMatrixStackDepth"], "", NoState,
	  ["ARB_vertex_program", "ARB_fragment_program", "NV_vertex_program"] ),
	( "GL_MAX_PROGRAM_MATRICES_ARB", GLint, # == GL_MAX_TRACK_MATRICES_NV
	  ["ctx->Const.MaxProgramMatrices"], "", NoState,
	  ["ARB_vertex_program", "ARB_fragment_program", "NV_vertex_program"] ),
	( "GL_CURRENT_MATRIX_STACK_DEPTH_ARB", GLboolean, # == GL_CURRENT_MATRIX_STACK_DEPTH_NV
	  ["ctx->CurrentStack->Depth + 1"], "", NoState,
	  ["ARB_vertex_program", "ARB_fragment_program", "NV_vertex_program"] ),
	( "GL_CURRENT_MATRIX_ARB", GLfloat, # == GL_CURRENT_MATRIX_NV
	  ["matrix[0]", "matrix[1]", "matrix[2]", "matrix[3]",
	   "matrix[4]", "matrix[5]", "matrix[6]", "matrix[7]",
	   "matrix[8]", "matrix[9]", "matrix[10]", "matrix[11]",
	   "matrix[12]", "matrix[13]", "matrix[14]", "matrix[15]" ],
	  "const GLfloat *matrix = ctx->CurrentStack->Top->m;", NoState,
	  ["ARB_vertex_program", "ARB_fragment_program", "NV_fragment_program"] ),
	( "GL_TRANSPOSE_CURRENT_MATRIX_ARB", GLfloat,
	  ["matrix[0]", "matrix[4]", "matrix[8]", "matrix[12]",
	   "matrix[1]", "matrix[5]", "matrix[9]", "matrix[13]",
	   "matrix[2]", "matrix[6]", "matrix[10]", "matrix[14]",
	   "matrix[3]", "matrix[7]", "matrix[11]", "matrix[15]"],
	  "const GLfloat *matrix = ctx->CurrentStack->Top->m;", NoState,
	  ["ARB_vertex_program", "ARB_fragment_program"] ),
	( "GL_MAX_VERTEX_ATTRIBS_ARB", GLint,
	  ["ctx->Const.VertexProgram.MaxAttribs"], "", NoState, ["ARB_vertex_program"] ),
	( "GL_PROGRAM_ERROR_POSITION_ARB", GLint, # == GL_PROGRAM_ERROR_POSITION_NV
	  ["ctx->Program.ErrorPos"], "", NoState, ["NV_vertex_program",
	   "ARB_vertex_program", "NV_fragment_program", "ARB_fragment_program"] ),

	# GL_ARB_fragment_program
	( "GL_FRAGMENT_PROGRAM_ARB", GLboolean,
	  ["ctx->FragmentProgram.Enabled"], "", NoState, ["ARB_fragment_program"] ),
	( "GL_MAX_TEXTURE_COORDS_ARB", GLint, # == GL_MAX_TEXTURE_COORDS_NV
	  ["ctx->Const.MaxTextureCoordUnits"], "", NoState,
	  ["ARB_fragment_program", "NV_fragment_program"] ),
	( "GL_MAX_TEXTURE_IMAGE_UNITS_ARB", GLint, # == GL_MAX_TEXTURE_IMAGE_UNITS_NV
	  ["ctx->Const.MaxTextureImageUnits"], "", NoState,
	  ["ARB_fragment_program", "NV_fragment_program"] ),

	# GL_EXT_depth_bounds_test
	( "GL_DEPTH_BOUNDS_TEST_EXT", GLboolean,
	  ["ctx->Depth.BoundsTest"], "", NoState, ["EXT_depth_bounds_test"] ),
	( "GL_DEPTH_BOUNDS_EXT", GLfloat,
	  ["ctx->Depth.BoundsMin", "ctx->Depth.BoundsMax"],
	  "", NoState, ["EXT_depth_bounds_test"] ),

	# GL_ARB_depth_clamp
	( "GL_DEPTH_CLAMP", GLboolean, ["ctx->Transform.DepthClamp"], "",
	  NoState, ["ARB_depth_clamp"] ),

	# GL_ARB_draw_buffers
	( "GL_MAX_DRAW_BUFFERS_ARB", GLint,
	  ["ctx->Const.MaxDrawBuffers"], "", NoState, NoExt ),
	( "GL_DRAW_BUFFER0_ARB", GLenum,
	  ["ctx->DrawBuffer->ColorDrawBuffer[0]"], "", NoState, NoExt ),
	( "GL_DRAW_BUFFER1_ARB", GLenum,
	  ["buffer"],
	  """GLenum buffer;
         if (pname - GL_DRAW_BUFFER0_ARB >= ctx->Const.MaxDrawBuffers) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGet(GL_DRAW_BUFFERx_ARB)");
            return;
         }
         buffer = ctx->DrawBuffer->ColorDrawBuffer[1];""", NoState, NoExt ),
	( "GL_DRAW_BUFFER2_ARB", GLenum,
	  ["buffer"],
	  """GLenum buffer;
         if (pname - GL_DRAW_BUFFER0_ARB >= ctx->Const.MaxDrawBuffers) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGet(GL_DRAW_BUFFERx_ARB)");
            return;
         }
         buffer = ctx->DrawBuffer->ColorDrawBuffer[2];""", NoState, NoExt ),
	( "GL_DRAW_BUFFER3_ARB", GLenum,
	  ["buffer"],
	  """GLenum buffer;
         if (pname - GL_DRAW_BUFFER0_ARB >= ctx->Const.MaxDrawBuffers) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGet(GL_DRAW_BUFFERx_ARB)");
            return;
         }
         buffer = ctx->DrawBuffer->ColorDrawBuffer[3];""", NoState, NoExt ),
	# XXX Add more GL_DRAW_BUFFERn_ARB entries as needed in the future

	# GL_OES_read_format
	( "GL_IMPLEMENTATION_COLOR_READ_TYPE_OES", GLint,
	  ["_mesa_get_color_read_type(ctx)"], "", "_NEW_BUFFERS", ["OES_read_format"] ),
	( "GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES", GLint,
	  ["_mesa_get_color_read_format(ctx)"], "", "_NEW_BUFFERS", ["OES_read_format"] ),

	# GL_ATI_fragment_shader
	( "GL_NUM_FRAGMENT_REGISTERS_ATI", GLint, ["6"],
	  "", NoState, ["ATI_fragment_shader"] ),
	( "GL_NUM_FRAGMENT_CONSTANTS_ATI", GLint, ["8"],
	  "", NoState, ["ATI_fragment_shader"] ),
	( "GL_NUM_PASSES_ATI", GLint, ["2"],
	  "", NoState, ["ATI_fragment_shader"] ),
	( "GL_NUM_INSTRUCTIONS_PER_PASS_ATI", GLint, ["8"],
	  "", NoState, ["ATI_fragment_shader"] ),
	( "GL_NUM_INSTRUCTIONS_TOTAL_ATI", GLint, ["16"],
	  "", NoState, ["ATI_fragment_shader"] ),
	( "GL_COLOR_ALPHA_PAIRING_ATI", GLboolean, ["GL_TRUE"],
	  "", NoState, ["ATI_fragment_shader"] ),
	( "GL_NUM_LOOPBACK_COMPONENTS_ATI", GLint, ["3"],
	  "", NoState, ["ATI_fragment_shader"] ),
	( "GL_NUM_INPUT_INTERPOLATOR_COMPONENTS_ATI", GLint, ["3"],
	  "", NoState, ["ATI_fragment_shader"] ),

	# OpenGL 2.0
	( "GL_STENCIL_BACK_FUNC", GLenum, ["ctx->Stencil.Function[1]"],
	  "", NoState, NoExt ),
	( "GL_STENCIL_BACK_VALUE_MASK", GLint, ["ctx->Stencil.ValueMask[1]"],
	  "", NoState, NoExt ),
	( "GL_STENCIL_BACK_WRITEMASK", GLint, ["ctx->Stencil.WriteMask[1]"],
	  "", NoState, NoExt ),
	( "GL_STENCIL_BACK_REF", GLint, ["ctx->Stencil.Ref[1]"],
	  "", NoState, NoExt ),
	( "GL_STENCIL_BACK_FAIL", GLenum, ["ctx->Stencil.FailFunc[1]"],
	  "", NoState, NoExt ),
	( "GL_STENCIL_BACK_PASS_DEPTH_FAIL", GLenum, ["ctx->Stencil.ZFailFunc[1]"],
	  "", NoState, NoExt ),
	( "GL_STENCIL_BACK_PASS_DEPTH_PASS", GLenum, ["ctx->Stencil.ZPassFunc[1]"],
	  "", NoState, NoExt ),

	# GL_EXT_framebuffer_object
	( "GL_FRAMEBUFFER_BINDING_EXT", GLint, ["ctx->DrawBuffer->Name"], "",
	  NoState, ["EXT_framebuffer_object"] ),
	( "GL_RENDERBUFFER_BINDING_EXT", GLint,
	  ["ctx->CurrentRenderbuffer ? ctx->CurrentRenderbuffer->Name : 0"], "",
	  NoState, ["EXT_framebuffer_object"] ),
	( "GL_MAX_COLOR_ATTACHMENTS_EXT", GLint,
	  ["ctx->Const.MaxColorAttachments"], "",
	  NoState, ["EXT_framebuffer_object"] ),
	( "GL_MAX_RENDERBUFFER_SIZE_EXT", GLint,
	  ["ctx->Const.MaxRenderbufferSize"], "",
	  NoState, ["EXT_framebuffer_object"] ),

	# GL_EXT_framebuffer_blit
	# NOTE: GL_DRAW_FRAMEBUFFER_BINDING_EXT == GL_FRAMEBUFFER_BINDING_EXT
	( "GL_READ_FRAMEBUFFER_BINDING_EXT", GLint, ["ctx->ReadBuffer->Name"], "",
	  NoState, ["EXT_framebuffer_blit"] ),

	# GL_EXT_provoking_vertex
	( "GL_PROVOKING_VERTEX_EXT", GLboolean,
	  ["ctx->Light.ProvokingVertex"], "", NoState, ["EXT_provoking_vertex"] ),
	( "GL_QUADS_FOLLOW_PROVOKING_VERTEX_CONVENTION_EXT", GLboolean,
	  ["ctx->Const.QuadsFollowProvokingVertexConvention"], "",
	  NoState, ["EXT_provoking_vertex"] ),

	# GL_ARB_fragment_shader
	( "GL_MAX_FRAGMENT_UNIFORM_COMPONENTS_ARB", GLint,
	  ["ctx->Const.FragmentProgram.MaxUniformComponents"], "",
	  NoState, ["ARB_fragment_shader"] ),
	( "GL_FRAGMENT_SHADER_DERIVATIVE_HINT_ARB", GLenum,
	  ["ctx->Hint.FragmentShaderDerivative"],
	  "", NoState, ["ARB_fragment_shader"] ),

	# GL_ARB_vertex_shader
	( "GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB", GLint,
	  ["ctx->Const.VertexProgram.MaxUniformComponents"], "",
	  NoState, ["ARB_vertex_shader"] ),
	( "GL_MAX_VARYING_FLOATS_ARB", GLint,
	  ["ctx->Const.MaxVarying * 4"], "", NoState, ["ARB_vertex_shader"] ),
	( "GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS_ARB", GLint,
	  ["ctx->Const.MaxVertexTextureImageUnits"],
	  "", NoState, ["ARB_vertex_shader"] ),
	( "GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS_ARB", GLint,
	  ["ctx->Const.MaxCombinedTextureImageUnits"],
	  "", NoState, ["ARB_vertex_shader"] ),

	# GL_ARB_shader_objects
	# Actually, this token isn't part of GL_ARB_shader_objects, but is
	# close enough for now.
	( "GL_CURRENT_PROGRAM", GLint,
	  ["ctx->Shader.CurrentProgram ? ctx->Shader.CurrentProgram->Name : 0"],
	  "", NoState, ["ARB_shader_objects"] ),

	# GL_ARB_framebuffer_object
	( "GL_MAX_SAMPLES", GLint, ["ctx->Const.MaxSamples"], "",
	  NoState, ["ARB_framebuffer_object"] ),

	# GL_APPLE_vertex_array_object
	( "GL_VERTEX_ARRAY_BINDING_APPLE", GLint, ["ctx->Array.ArrayObj->Name"], "",
	  NoState, ["APPLE_vertex_array_object"] ),

	# GL_ARB_seamless_cube_map
	( "GL_TEXTURE_CUBE_MAP_SEAMLESS", GLboolean, ["ctx->Texture.CubeMapSeamless"], "",
	  NoState, ["ARB_seamless_cube_map"] ),

	# GL_ARB_sync
	( "GL_MAX_SERVER_WAIT_TIMEOUT", GLint64, ["ctx->Const.MaxServerWaitTimeout"], "",
	  NoState, ["ARB_sync"] ),

	# GL_EXT_transform_feedback
	( "GL_TRANSFORM_FEEDBACK_BUFFER_BINDING", GLint,
	  ["ctx->TransformFeedback.CurrentBuffer->Name"], "",
	  NoState, ["EXT_transform_feedback"] ),
	( "GL_RASTERIZER_DISCARD", GLboolean,
	  ["ctx->TransformFeedback.RasterDiscard"], "",
	  NoState, ["EXT_transform_feedback"] ),
	( "GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS", GLint,
	  ["ctx->Const.MaxTransformFeedbackInterleavedComponents"], "",
	  NoState, ["EXT_transform_feedback"] ),
	( "GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS", GLint,
	  ["ctx->Const.MaxTransformFeedbackSeparateAttribs"], "",
	  NoState, ["EXT_transform_feedback"] ),
	( "GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS", GLint,
	  ["ctx->Const.MaxTransformFeedbackSeparateComponents"], "",
	  NoState, ["EXT_transform_feedback"] ),

	# GL 3.0
	( "GL_NUM_EXTENSIONS", GLint, ["_mesa_get_extension_count(ctx)"], "", NoState, NoExt ),
	( "GL_MAJOR_VERSION", GLint, ["ctx->VersionMajor"], "", NoState, NoExt ),
	( "GL_MINOR_VERSION", GLint, ["ctx->VersionMinor"], "", NoState, NoExt ),
	( "GL_CONTEXT_FLAGS", GLint, ["ctx->Const.ContextFlags"], "", NoState, NoExt ),

    # GL 3.1
    ( "GL_PRIMITIVE_RESTART", GLboolean,
      ["ctx->Array.PrimitiveRestart"], "", NoState, NoExt ),
    ( "GL_PRIMITIVE_RESTART_INDEX", GLint,
      ["ctx->Array.RestartIndex"], "", NoState, NoExt ),
 
	# GL 3.2
	( "GL_CONTEXT_PROFILE_MASK", GLint, ["ctx->Const.ProfileMask"], "",
	  NoState, NoExt )

]


# These are queried via glGetIntegetIndexdvEXT() or glGetIntegeri_v()
# The tuples are the same as above, with one exception: the "optional"
# code field is instead the max legal index value.
IndexedStateVars = [
	# GL_EXT_draw_buffers2 / GL3
	( "GL_BLEND", GLint, ["((ctx->Color.BlendEnabled >> index) & 1)"],
	  "ctx->Const.MaxDrawBuffers", NoState, ["EXT_draw_buffers2"] ),
	( "GL_COLOR_WRITEMASK", GLint,
	  [ "ctx->Color.ColorMask[index][RCOMP] ? 1 : 0",
		"ctx->Color.ColorMask[index][GCOMP] ? 1 : 0",
		"ctx->Color.ColorMask[index][BCOMP] ? 1 : 0",
		"ctx->Color.ColorMask[index][ACOMP] ? 1 : 0" ],
	  "ctx->Const.MaxDrawBuffers", NoState, ["EXT_draw_buffers2"] ),

	# GL_EXT_transform_feedback
	( "GL_TRANSFORM_FEEDBACK_BUFFER_START", GLint64,
	  ["ctx->TransformFeedback.Offset[index]"],
	  "ctx->Const.MaxTransformFeedbackSeparateAttribs",
	  NoState, ["EXT_transform_feedback"] ),
	( "GL_TRANSFORM_FEEDBACK_BUFFER_SIZE", GLint64,
	  ["ctx->TransformFeedback.Size[index]"],
	  "ctx->Const.MaxTransformFeedbackSeparateAttribs",
	  NoState, ["EXT_transform_feedback"] ),
	( "GL_TRANSFORM_FEEDBACK_BUFFER_BINDING", GLint,
	  ["ctx->TransformFeedback.Buffers[index]->Name"],
	  "ctx->Const.MaxTransformFeedbackSeparateAttribs",
	  NoState, ["EXT_transform_feedback"] ),

	# XXX more to come...
]



def ConversionFunc(fromType, toType):
	"""Return the name of the macro to convert between two data types."""
	if fromType == toType:
		return ""
	elif fromType == GLfloat and toType == GLint:
		return "IROUND"
	elif fromType == GLfloat and toType == GLint64:
		return "IROUND64"
	elif fromType == GLfloatN and toType == GLfloat:
		return ""
	elif fromType == GLint and toType == GLfloat: # but not GLfloatN!
		return "(GLfloat)"
	elif fromType == GLint and toType == GLint64:
		return "(GLint64)"
	elif fromType == GLint64 and toType == GLfloat: # but not GLfloatN!
		return "(GLfloat)"
	else:
		if fromType == GLfloatN:
			fromType = GLfloat
		fromStr = TypeStrings[fromType]
		fromStr = string.upper(fromStr[2:])
		toStr = TypeStrings[toType]
		toStr = string.upper(toStr[2:])
		return fromStr + "_TO_" + toStr


def EmitGetFunction(stateVars, returnType, indexed):
	"""Emit the code to implement glGetBooleanv, glGetIntegerv or glGetFloatv."""
	assert (returnType == GLboolean or
			returnType == GLint or
			returnType == GLint64 or
			returnType == GLfloat)

	strType = TypeStrings[returnType]
	# Capitalize first letter of return type
	if indexed:
		if returnType == GLint:
			function = "GetIntegerIndexedv"
		elif returnType == GLboolean:
			function = "GetBooleanIndexedv"
		elif returnType == GLint64:
			function = "GetInteger64Indexedv"
		else:
			function = "Foo"
	else:
		if returnType == GLint:
			function = "GetIntegerv"
		elif returnType == GLboolean:
			function = "GetBooleanv"
		elif returnType == GLfloat:
			function = "GetFloatv"
		elif returnType == GLint64:
			function = "GetInteger64v"
		else:
			sys.exit(1)

	if returnType == GLint64:
		print "#if FEATURE_ARB_sync"

	print "void GLAPIENTRY"
	if indexed:
		print "_mesa_%s( GLenum pname, GLuint index, %s *params )" % (function, strType)
	else:
		print "_mesa_%s( GLenum pname, %s *params )" % (function, strType)
	print "{"
	print "   GET_CURRENT_CONTEXT(ctx);"
	print "   ASSERT_OUTSIDE_BEGIN_END(ctx);"
	print ""
	print "   if (!params)"
	print "      return;"
	print ""
	if indexed == 0:
		print "   if (ctx->Driver.%s &&" % function
		print "       ctx->Driver.%s(ctx, pname, params))" % function
		print "      return;"
		print ""
	print "   switch (pname) {"

	for state in stateVars:
		if indexed:
			(name, varType, state, indexMax, dirtyFlags, extensions) = state
			optionalCode = 0
		else:
			(name, varType, state, optionalCode, dirtyFlags, extensions) = state
			indexMax = 0
		print "      case " + name + ":"

		# Do extension check
		if extensions:
			if len(extensions) == 1:
				print ('         CHECK_EXT1(%s);' % extensions[0])
			elif len(extensions) == 2:
				print ('         CHECK_EXT2(%s, %s);' % (extensions[0], extensions[1]))
			elif len(extensions) == 3:
				print ('         CHECK_EXT3(%s, %s, %s);' %
					   (extensions[0], extensions[1], extensions[2]))
			else:
				assert len(extensions) == 4
				print ('         CHECK_EXT4(%s, %s, %s, %s);' %
					   (extensions[0], extensions[1], extensions[2], extensions[3]))

		# Do dirty state check
		if dirtyFlags:
			if dirtyFlags == FlushCurrent:
				print ('         FLUSH_CURRENT(ctx, 0);')
			else:
				print ('         if (ctx->NewState & %s)' % dirtyFlags)
				print ('            _mesa_update_state(ctx);')

		# Do index validation for glGet*Indexed() calls
		if indexMax:
			print ('         if (index >= %s) {' % indexMax)
			print ('            _mesa_error(ctx, GL_INVALID_VALUE, "gl%s(index=%%u), index", pname);' % function)
			print ('            return;')
			print ('         }')

		conversion = ConversionFunc(varType, returnType)
		if optionalCode:
			optionalCode = string.replace(optionalCode, "CONVERSION", conversion);	
			print "         {"
			print "         " + optionalCode
		n = len(state)
		for i in range(n):
			if conversion:
				print "         params[%d] = %s(%s);" % (i, conversion, state[i])
			else:
				print "         params[%d] = %s;" % (i, state[i])
		if optionalCode:
			print "         }"
		print "         break;"

	print "      default:"
	print "         goto invalid_enum_error;"
	print "   }"
	print "   return;"
	print ""
	print "invalid_enum_error:"
	print '   _mesa_error(ctx, GL_INVALID_ENUM, "gl%s(pname=0x%%x)", pname);' % function
	print "}"
	if returnType == GLint64:
		print "#endif /* FEATURE_ARB_sync */"
	print ""
	return



def EmitHeader():
	"""Print the get.c file header."""
	print """
/***
 ***  NOTE!!!  DO NOT EDIT THIS FILE!!!  IT IS GENERATED BY get_gen.py
 ***/

#include "glheader.h"
#include "context.h"
#include "enable.h"
#include "extensions.h"
#include "get.h"
#include "macros.h"
#include "mtypes.h"
#include "state.h"
#include "texcompress.h"
#include "framebuffer.h"


#define FLOAT_TO_BOOLEAN(X)   ( (X) ? GL_TRUE : GL_FALSE )

#define INT_TO_BOOLEAN(I)     ( (I) ? GL_TRUE : GL_FALSE )

#define INT64_TO_BOOLEAN(I)   ( (I) ? GL_TRUE : GL_FALSE )
#define INT64_TO_INT(I)       ( (GLint)((I > INT_MAX) ? INT_MAX : ((I < INT_MIN) ? INT_MIN : (I))) )

#define BOOLEAN_TO_INT(B)     ( (GLint) (B) )
#define BOOLEAN_TO_INT64(B)   ( (GLint64) (B) )
#define BOOLEAN_TO_FLOAT(B)   ( (B) ? 1.0F : 0.0F )

#define ENUM_TO_INT64(E)      ( (GLint64) (E) )


/*
 * Check if named extension is enabled, if not generate error and return.
 */
#define CHECK_EXT1(EXT1)                                               \\
   if (!ctx->Extensions.EXT1) {                                        \\
      goto invalid_enum_error;                                         \\
   }

/*
 * Check if either of two extensions is enabled.
 */
#define CHECK_EXT2(EXT1, EXT2)                                         \\
   if (!ctx->Extensions.EXT1 && !ctx->Extensions.EXT2) {               \\
      goto invalid_enum_error;                                         \\
   }

/*
 * Check if either of three extensions is enabled.
 */
#define CHECK_EXT3(EXT1, EXT2, EXT3)                                   \\
   if (!ctx->Extensions.EXT1 && !ctx->Extensions.EXT2 &&               \\
       !ctx->Extensions.EXT3) {                                        \\
      goto invalid_enum_error;                                         \\
   }

/*
 * Check if either of four extensions is enabled.
 */
#define CHECK_EXT4(EXT1, EXT2, EXT3, EXT4)                             \\
   if (!ctx->Extensions.EXT1 && !ctx->Extensions.EXT2 &&               \\
       !ctx->Extensions.EXT3 && !ctx->Extensions.EXT4) {               \\
      goto invalid_enum_error;                                         \\
   }

"""
	return



def EmitGetDoublev():
	print """
void GLAPIENTRY
_mesa_GetDoublev( GLenum pname, GLdouble *params )
{
   const GLfloat magic = -1234.5F;
   GLfloat values[16];
   GLuint i;

   if (!params)
      return;

   /* Init temp array to magic numbers so we can figure out how many values
    * are returned by the GetFloatv() call.
    */
   for (i = 0; i < 16; i++)
      values[i] = magic;

   _mesa_GetFloatv(pname, values);
   
   for (i = 0; i < 16 && values[i] != magic; i++)
      params[i] = (GLdouble) values[i];
}
"""
	



EmitHeader()
# XXX Maybe sort the StateVars list
EmitGetFunction(StateVars, GLboolean, 0)
EmitGetFunction(StateVars, GLfloat, 0)
EmitGetFunction(StateVars, GLint, 0)
EmitGetFunction(StateVars, GLint64, 0)
EmitGetDoublev()

EmitGetFunction(IndexedStateVars, GLboolean, 1)
EmitGetFunction(IndexedStateVars, GLint, 1)
EmitGetFunction(IndexedStateVars, GLint64, 1)


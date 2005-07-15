#!/usr/bin/env python

# Mesa 3-D graphics library
#
# Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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


GLint = 1
GLenum = 2
GLfloat = 3
GLdouble = 4
GLboolean = 5
GLfloatN = 6    # A normalized value, such as a color or depth range


TypeStrings = {
	GLint : "GLint",
	GLenum : "GLenum",
	GLfloat : "GLfloat",
	GLdouble : "GLdouble",
	GLboolean : "GLboolean"
}


# Each entry is a tuple of:
#  - the GL state name, such as GL_CURRENT_COLOR
#  - the state datatype, one of GLint, GLfloat, GLboolean or GLenum
#  - list of code fragments to get the state, such as ["ctx->Foo.Bar"]
#  - optional extra code or empty string
#  - optional extension to check, or None  (XXX this should be a list!)
#
StateVars = [
	( "GL_ACCUM_RED_BITS", GLint, ["ctx->DrawBuffer->Visual.accumRedBits"],
	  "", None ),
	( "GL_ACCUM_GREEN_BITS", GLint, ["ctx->DrawBuffer->Visual.accumGreenBits"],
	  "", None ),
	( "GL_ACCUM_BLUE_BITS", GLint, ["ctx->DrawBuffer->Visual.accumBlueBits"],
	  "", None ),
	( "GL_ACCUM_ALPHA_BITS", GLint, ["ctx->DrawBuffer->Visual.accumAlphaBits"],
	  "", None ),
	( "GL_ACCUM_CLEAR_VALUE", GLfloatN,
	  [ "ctx->Accum.ClearColor[0]",
		"ctx->Accum.ClearColor[1]",
		"ctx->Accum.ClearColor[2]",
		"ctx->Accum.ClearColor[3]" ],
	  "", None ),
	( "GL_ALPHA_BIAS", GLfloat, ["ctx->Pixel.AlphaBias"], "", None ),
	( "GL_ALPHA_BITS", GLint, ["ctx->DrawBuffer->Visual.alphaBits"],
	  "", None ),
	( "GL_ALPHA_SCALE", GLfloat, ["ctx->Pixel.AlphaScale"], "", None ),
	( "GL_ALPHA_TEST", GLboolean, ["ctx->Color.AlphaEnabled"], "", None ),
	( "GL_ALPHA_TEST_FUNC", GLenum, ["ctx->Color.AlphaFunc"], "", None ),
	( "GL_ALPHA_TEST_REF", GLfloatN, ["ctx->Color.AlphaRef"], "", None ),
	( "GL_ATTRIB_STACK_DEPTH", GLint, ["ctx->AttribStackDepth"], "", None ),
	( "GL_AUTO_NORMAL", GLboolean, ["ctx->Eval.AutoNormal"], "", None ),
	( "GL_AUX_BUFFERS", GLint, ["ctx->DrawBuffer->Visual.numAuxBuffers"],
	  "", None ),
	( "GL_BLEND", GLboolean, ["ctx->Color.BlendEnabled"], "", None ),
	( "GL_BLEND_DST", GLenum, ["ctx->Color.BlendDstRGB"], "", None ),
	( "GL_BLEND_SRC", GLenum, ["ctx->Color.BlendSrcRGB"], "", None ),
	( "GL_BLEND_SRC_RGB_EXT", GLenum, ["ctx->Color.BlendSrcRGB"], "", None ),
	( "GL_BLEND_DST_RGB_EXT", GLenum, ["ctx->Color.BlendDstRGB"], "", None ),
	( "GL_BLEND_SRC_ALPHA_EXT", GLenum, ["ctx->Color.BlendSrcA"], "", None ),
	( "GL_BLEND_DST_ALPHA_EXT", GLenum, ["ctx->Color.BlendDstA"], "", None ),
	( "GL_BLEND_EQUATION", GLenum, ["ctx->Color.BlendEquationRGB "], "", None),
	( "GL_BLEND_EQUATION_ALPHA_EXT", GLenum, ["ctx->Color.BlendEquationA "],
	  "", None ),
	( "GL_BLEND_COLOR_EXT", GLfloatN,
	  [ "ctx->Color.BlendColor[0]",
		"ctx->Color.BlendColor[1]",
		"ctx->Color.BlendColor[2]",
		"ctx->Color.BlendColor[3]"], "", None ),
	( "GL_BLUE_BIAS", GLfloat, ["ctx->Pixel.BlueBias"], "", None ),
	( "GL_BLUE_BITS", GLint, ["ctx->DrawBuffer->Visual.blueBits"], "", None ),
	( "GL_BLUE_SCALE", GLfloat, ["ctx->Pixel.BlueScale"], "", None ),
	( "GL_CLIENT_ATTRIB_STACK_DEPTH", GLint,
	  ["ctx->ClientAttribStackDepth"], "", None ),
	( "GL_CLIP_PLANE0", GLboolean,
	  [ "(ctx->Transform.ClipPlanesEnabled >> 0) & 1" ], "", None ),
	( "GL_CLIP_PLANE1", GLboolean,
	  [ "(ctx->Transform.ClipPlanesEnabled >> 1) & 1" ], "", None ),
	( "GL_CLIP_PLANE2", GLboolean,
	  [ "(ctx->Transform.ClipPlanesEnabled >> 2) & 1" ], "", None ),
	( "GL_CLIP_PLANE3", GLboolean,
	  [ "(ctx->Transform.ClipPlanesEnabled >> 3) & 1" ], "", None ),
	( "GL_CLIP_PLANE4", GLboolean,
	  [ "(ctx->Transform.ClipPlanesEnabled >> 4) & 1" ], "", None ),
	( "GL_CLIP_PLANE5", GLboolean,
	  [ "(ctx->Transform.ClipPlanesEnabled >> 5) & 1" ], "", None ),
	( "GL_COLOR_CLEAR_VALUE", GLfloatN,
	  [ "ctx->Color.ClearColor[0]",
		"ctx->Color.ClearColor[1]",
		"ctx->Color.ClearColor[2]",
		"ctx->Color.ClearColor[3]" ], "", None ),
	( "GL_COLOR_MATERIAL", GLboolean,
	  ["ctx->Light.ColorMaterialEnabled"], "", None ),
	( "GL_COLOR_MATERIAL_FACE", GLenum,
	  ["ctx->Light.ColorMaterialFace"], "", None ),
	( "GL_COLOR_MATERIAL_PARAMETER", GLenum,
	  ["ctx->Light.ColorMaterialMode"], "", None ),
	( "GL_COLOR_WRITEMASK", GLint,
	  [ "ctx->Color.ColorMask[RCOMP] ? 1 : 0",
		"ctx->Color.ColorMask[GCOMP] ? 1 : 0",
		"ctx->Color.ColorMask[BCOMP] ? 1 : 0",
		"ctx->Color.ColorMask[ACOMP] ? 1 : 0" ], "", None ),
	( "GL_CULL_FACE", GLboolean, ["ctx->Polygon.CullFlag"], "", None ),
	( "GL_CULL_FACE_MODE", GLenum, ["ctx->Polygon.CullFaceMode"], "", None ),
	( "GL_CURRENT_COLOR", GLfloatN,
	  [ "ctx->Current.Attrib[VERT_ATTRIB_COLOR0][0]",
		"ctx->Current.Attrib[VERT_ATTRIB_COLOR0][1]",
		"ctx->Current.Attrib[VERT_ATTRIB_COLOR0][2]",
		"ctx->Current.Attrib[VERT_ATTRIB_COLOR0][3]" ],
	  "FLUSH_CURRENT(ctx, 0);", None ),
	( "GL_CURRENT_INDEX", GLfloat, ["ctx->Current.Index"],
	  "FLUSH_CURRENT(ctx, 0);", None ),
	( "GL_CURRENT_NORMAL", GLfloatN,
	  [ "ctx->Current.Attrib[VERT_ATTRIB_NORMAL][0]",
		"ctx->Current.Attrib[VERT_ATTRIB_NORMAL][1]",
		"ctx->Current.Attrib[VERT_ATTRIB_NORMAL][2]"],
	  "FLUSH_CURRENT(ctx, 0);", None ),
	( "GL_CURRENT_RASTER_COLOR", GLfloatN,
	  ["ctx->Current.RasterColor[0]",
	   "ctx->Current.RasterColor[1]",
	   "ctx->Current.RasterColor[2]",
	   "ctx->Current.RasterColor[3]"], "", None ),
	( "GL_CURRENT_RASTER_DISTANCE", GLfloat,
	  ["ctx->Current.RasterDistance"], "", None ),
	( "GL_CURRENT_RASTER_INDEX", GLfloat,
	  ["ctx->Current.RasterIndex"], "", None ),
	( "GL_CURRENT_RASTER_POSITION", GLfloat,
	  ["ctx->Current.RasterPos[0]",
	   "ctx->Current.RasterPos[1]",
	   "ctx->Current.RasterPos[2]",
	   "ctx->Current.RasterPos[3]"], "", None ),
	( "GL_CURRENT_RASTER_TEXTURE_COORDS", GLfloat,
	  ["ctx->Current.RasterTexCoords[texUnit][0]",
	   "ctx->Current.RasterTexCoords[texUnit][1]",
	   "ctx->Current.RasterTexCoords[texUnit][2]",
	   "ctx->Current.RasterTexCoords[texUnit][3]"],
	  "const GLuint texUnit = ctx->Texture.CurrentUnit;", None ),
	( "GL_CURRENT_RASTER_POSITION_VALID", GLboolean,
	  ["ctx->Current.RasterPosValid"], "", None ),
	( "GL_CURRENT_TEXTURE_COORDS", GLfloat,
	  ["ctx->Current.Attrib[VERT_ATTRIB_TEX0 + texUnit][0]",
	   "ctx->Current.Attrib[VERT_ATTRIB_TEX0 + texUnit][1]",
	   "ctx->Current.Attrib[VERT_ATTRIB_TEX0 + texUnit][2]",
	   "ctx->Current.Attrib[VERT_ATTRIB_TEX0 + texUnit][3]"],
	  "const GLuint texUnit = ctx->Texture.CurrentUnit;", None ),
	( "GL_DEPTH_BIAS", GLfloat, ["ctx->Pixel.DepthBias"], "", None ),
	( "GL_DEPTH_BITS", GLint, ["ctx->DrawBuffer->Visual.depthBits"],
	  "", None ),
	( "GL_DEPTH_CLEAR_VALUE", GLfloat, ["ctx->Depth.Clear"], "", None ),
	( "GL_DEPTH_FUNC", GLenum, ["ctx->Depth.Func"], "", None ),
	( "GL_DEPTH_RANGE", GLfloatN,
	  [ "ctx->Viewport.Near", "ctx->Viewport.Far" ], "", None ),
	( "GL_DEPTH_SCALE", GLfloat, ["ctx->Pixel.DepthScale"], "", None ),
	( "GL_DEPTH_TEST", GLboolean, ["ctx->Depth.Test"], "", None ),
	( "GL_DEPTH_WRITEMASK", GLboolean, ["ctx->Depth.Mask"], "", None ),
	( "GL_DITHER", GLboolean, ["ctx->Color.DitherFlag"], "", None ),
	( "GL_DOUBLEBUFFER", GLboolean,
	  ["ctx->DrawBuffer->Visual.doubleBufferMode"], "", None ),
	( "GL_DRAW_BUFFER", GLenum, ["ctx->Color.DrawBuffer[0]"], "", None ),
	( "GL_EDGE_FLAG", GLboolean, ["ctx->Current.EdgeFlag"],
	  "FLUSH_CURRENT(ctx, 0);", None ),
	( "GL_FEEDBACK_BUFFER_SIZE", GLint, ["ctx->Feedback.BufferSize"], "", None ),
	( "GL_FEEDBACK_BUFFER_TYPE", GLenum, ["ctx->Feedback.Type"], "", None ),
	( "GL_FOG", GLboolean, ["ctx->Fog.Enabled"], "", None ),
	( "GL_FOG_COLOR", GLfloatN,
	  [ "ctx->Fog.Color[0]",
		"ctx->Fog.Color[1]",
		"ctx->Fog.Color[2]",
		"ctx->Fog.Color[3]" ], "", None ),
	( "GL_FOG_DENSITY", GLfloat, ["ctx->Fog.Density"], "", None ),
	( "GL_FOG_END", GLfloat, ["ctx->Fog.End"], "", None ),
	( "GL_FOG_HINT", GLenum, ["ctx->Hint.Fog"], "", None ),
	( "GL_FOG_INDEX", GLfloat, ["ctx->Fog.Index"], "", None ),
	( "GL_FOG_MODE", GLenum, ["ctx->Fog.Mode"], "", None ),
	( "GL_FOG_START", GLfloat, ["ctx->Fog.Start"], "", None ),
	( "GL_FRONT_FACE", GLenum, ["ctx->Polygon.FrontFace"], "", None ),
	( "GL_GREEN_BIAS", GLfloat, ["ctx->Pixel.GreenBias"], "", None ),
	( "GL_GREEN_BITS", GLint, ["ctx->DrawBuffer->Visual.greenBits"],
	  "", None ),
	( "GL_GREEN_SCALE", GLfloat, ["ctx->Pixel.GreenScale"], "", None ),
	( "GL_INDEX_BITS", GLint, ["ctx->DrawBuffer->Visual.indexBits"],
	  "", None ),
	( "GL_INDEX_CLEAR_VALUE", GLint, ["ctx->Color.ClearIndex"], "", None ),
	( "GL_INDEX_MODE", GLboolean, ["!ctx->DrawBuffer->Visual.rgbMode"],
	  "", None ),
	( "GL_INDEX_OFFSET", GLint, ["ctx->Pixel.IndexOffset"], "", None ),
	( "GL_INDEX_SHIFT", GLint, ["ctx->Pixel.IndexShift"], "", None ),
	( "GL_INDEX_WRITEMASK", GLint, ["ctx->Color.IndexMask"], "", None ),
	( "GL_LIGHT0", GLboolean, ["ctx->Light.Light[0].Enabled"], "", None ),
	( "GL_LIGHT1", GLboolean, ["ctx->Light.Light[1].Enabled"], "", None ),
	( "GL_LIGHT2", GLboolean, ["ctx->Light.Light[2].Enabled"], "", None ),
	( "GL_LIGHT3", GLboolean, ["ctx->Light.Light[3].Enabled"], "", None ),
	( "GL_LIGHT4", GLboolean, ["ctx->Light.Light[4].Enabled"], "", None ),
	( "GL_LIGHT5", GLboolean, ["ctx->Light.Light[5].Enabled"], "", None ),
	( "GL_LIGHT6", GLboolean, ["ctx->Light.Light[6].Enabled"], "", None ),
	( "GL_LIGHT7", GLboolean, ["ctx->Light.Light[7].Enabled"], "", None ),
	( "GL_LIGHTING", GLboolean, ["ctx->Light.Enabled"], "", None ),
	( "GL_LIGHT_MODEL_AMBIENT", GLfloatN,
	  ["ctx->Light.Model.Ambient[0]",
	   "ctx->Light.Model.Ambient[1]",
	   "ctx->Light.Model.Ambient[2]",
	   "ctx->Light.Model.Ambient[3]"], "", None ),
	( "GL_LIGHT_MODEL_COLOR_CONTROL", GLenum,
	  ["ctx->Light.Model.ColorControl"], "", None ),
	( "GL_LIGHT_MODEL_LOCAL_VIEWER", GLboolean,
	  ["ctx->Light.Model.LocalViewer"], "", None ),
	( "GL_LIGHT_MODEL_TWO_SIDE", GLboolean, ["ctx->Light.Model.TwoSide"], "", None ),
	( "GL_LINE_SMOOTH", GLboolean, ["ctx->Line.SmoothFlag"], "", None ),
	( "GL_LINE_SMOOTH_HINT", GLenum, ["ctx->Hint.LineSmooth"], "", None ),
	( "GL_LINE_STIPPLE", GLboolean, ["ctx->Line.StippleFlag"], "", None ),
	( "GL_LINE_STIPPLE_PATTERN", GLint, ["ctx->Line.StipplePattern"], "", None ),
	( "GL_LINE_STIPPLE_REPEAT", GLint, ["ctx->Line.StippleFactor"], "", None ),
	( "GL_LINE_WIDTH", GLfloat, ["ctx->Line.Width"], "", None ),
	( "GL_LINE_WIDTH_GRANULARITY", GLfloat,
	  ["ctx->Const.LineWidthGranularity"], "", None ),
	( "GL_LINE_WIDTH_RANGE", GLfloat,
	  ["ctx->Const.MinLineWidthAA",
	   "ctx->Const.MaxLineWidthAA"], "", None ),
	( "GL_ALIASED_LINE_WIDTH_RANGE", GLfloat,
	  ["ctx->Const.MinLineWidth",
	   "ctx->Const.MaxLineWidth"], "", None ),
	( "GL_LIST_BASE", GLint, ["ctx->List.ListBase"], "", None ),
	( "GL_LIST_INDEX", GLint, ["ctx->ListState.CurrentListNum"], "", None ),
	( "GL_LIST_MODE", GLenum, ["mode"],
	  """GLenum mode;
         if (!ctx->CompileFlag)
            mode = 0;
         else if (ctx->ExecuteFlag)
            mode = GL_COMPILE_AND_EXECUTE;
         else
            mode = GL_COMPILE;""", None ),
	( "GL_INDEX_LOGIC_OP", GLboolean, ["ctx->Color.IndexLogicOpEnabled"], "", None ),
	( "GL_COLOR_LOGIC_OP", GLboolean, ["ctx->Color.ColorLogicOpEnabled"], "", None ),
	( "GL_LOGIC_OP_MODE", GLenum, ["ctx->Color.LogicOp"], "", None ),
	( "GL_MAP1_COLOR_4", GLboolean, ["ctx->Eval.Map1Color4"], "", None ),
	( "GL_MAP1_GRID_DOMAIN", GLfloat,
	  ["ctx->Eval.MapGrid1u1",
	   "ctx->Eval.MapGrid1u2"], "", None ),
	( "GL_MAP1_GRID_SEGMENTS", GLint, ["ctx->Eval.MapGrid1un"], "", None ),
	( "GL_MAP1_INDEX", GLboolean, ["ctx->Eval.Map1Index"], "", None ),
	( "GL_MAP1_NORMAL", GLboolean, ["ctx->Eval.Map1Normal"], "", None ),
	( "GL_MAP1_TEXTURE_COORD_1", GLboolean, ["ctx->Eval.Map1TextureCoord1"], "", None ),
	( "GL_MAP1_TEXTURE_COORD_2", GLboolean, ["ctx->Eval.Map1TextureCoord2"], "", None ),
	( "GL_MAP1_TEXTURE_COORD_3", GLboolean, ["ctx->Eval.Map1TextureCoord3"], "", None ),
	( "GL_MAP1_TEXTURE_COORD_4", GLboolean, ["ctx->Eval.Map1TextureCoord4"], "", None ),
	( "GL_MAP1_VERTEX_3", GLboolean, ["ctx->Eval.Map1Vertex3"], "", None ),
	( "GL_MAP1_VERTEX_4", GLboolean, ["ctx->Eval.Map1Vertex4"], "", None ),
	( "GL_MAP2_COLOR_4", GLboolean, ["ctx->Eval.Map2Color4"], "", None ),
	( "GL_MAP2_GRID_DOMAIN", GLfloat,
	  ["ctx->Eval.MapGrid2u1",
	   "ctx->Eval.MapGrid2u2",
	   "ctx->Eval.MapGrid2v1",
	   "ctx->Eval.MapGrid2v2"], "", None ),
	( "GL_MAP2_GRID_SEGMENTS", GLint,
	  ["ctx->Eval.MapGrid2un",
	   "ctx->Eval.MapGrid2vn"], "", None ),
	( "GL_MAP2_INDEX", GLboolean, ["ctx->Eval.Map2Index"], "", None ),
	( "GL_MAP2_NORMAL", GLboolean, ["ctx->Eval.Map2Normal"], "", None ),
	( "GL_MAP2_TEXTURE_COORD_1", GLboolean, ["ctx->Eval.Map2TextureCoord1"], "", None ),
	( "GL_MAP2_TEXTURE_COORD_2", GLboolean, ["ctx->Eval.Map2TextureCoord2"], "", None ),
	( "GL_MAP2_TEXTURE_COORD_3", GLboolean, ["ctx->Eval.Map2TextureCoord3"], "", None ),
	( "GL_MAP2_TEXTURE_COORD_4", GLboolean, ["ctx->Eval.Map2TextureCoord4"], "", None ),
	( "GL_MAP2_VERTEX_3", GLboolean, ["ctx->Eval.Map2Vertex3"], "", None ),
	( "GL_MAP2_VERTEX_4", GLboolean, ["ctx->Eval.Map2Vertex4"], "", None ),
	( "GL_MAP_COLOR", GLboolean, ["ctx->Pixel.MapColorFlag"], "", None ),
	( "GL_MAP_STENCIL", GLboolean, ["ctx->Pixel.MapStencilFlag"], "", None ),
	( "GL_MATRIX_MODE", GLenum, ["ctx->Transform.MatrixMode"], "", None ),

	( "GL_MAX_ATTRIB_STACK_DEPTH", GLint, ["MAX_ATTRIB_STACK_DEPTH"], "", None ),
	( "GL_MAX_CLIENT_ATTRIB_STACK_DEPTH", GLint, ["MAX_CLIENT_ATTRIB_STACK_DEPTH"], "", None ),
	( "GL_MAX_CLIP_PLANES", GLint, ["ctx->Const.MaxClipPlanes"], "", None ),
	( "GL_MAX_ELEMENTS_VERTICES", GLint, ["ctx->Const.MaxArrayLockSize"], "", None ),
	( "GL_MAX_ELEMENTS_INDICES", GLint, ["ctx->Const.MaxArrayLockSize"], "", None ),
	( "GL_MAX_EVAL_ORDER", GLint, ["MAX_EVAL_ORDER"], "", None ),
	( "GL_MAX_LIGHTS", GLint, ["ctx->Const.MaxLights"], "", None ),
	( "GL_MAX_LIST_NESTING", GLint, ["MAX_LIST_NESTING"], "", None ),
	( "GL_MAX_MODELVIEW_STACK_DEPTH", GLint, ["MAX_MODELVIEW_STACK_DEPTH"], "", None ),
	( "GL_MAX_NAME_STACK_DEPTH", GLint, ["MAX_NAME_STACK_DEPTH"], "", None ),
	( "GL_MAX_PIXEL_MAP_TABLE", GLint, ["MAX_PIXEL_MAP_TABLE"], "", None ),
	( "GL_MAX_PROJECTION_STACK_DEPTH", GLint, ["MAX_PROJECTION_STACK_DEPTH"], "", None ),
	( "GL_MAX_TEXTURE_SIZE", GLint, ["1 << (ctx->Const.MaxTextureLevels - 1)"], "", None ),
	( "GL_MAX_3D_TEXTURE_SIZE", GLint, ["1 << (ctx->Const.Max3DTextureLevels - 1)"], "", None ),
	( "GL_MAX_TEXTURE_STACK_DEPTH", GLint, ["MAX_TEXTURE_STACK_DEPTH"], "", None ),
	( "GL_MAX_VIEWPORT_DIMS", GLint,
	  ["ctx->Const.MaxViewportWidth", "ctx->Const.MaxViewportHeight"],
	  "", None ),
	( "GL_MODELVIEW_MATRIX", GLfloat,
	  [ "matrix[0]", "matrix[1]", "matrix[2]", "matrix[3]",
		"matrix[4]", "matrix[5]", "matrix[6]", "matrix[7]",
		"matrix[8]", "matrix[9]", "matrix[10]", "matrix[11]",
		"matrix[12]", "matrix[13]", "matrix[14]", "matrix[15]" ],
	  "const GLfloat *matrix = ctx->ModelviewMatrixStack.Top->m;", None ),
	( "GL_MODELVIEW_STACK_DEPTH", GLint, ["ctx->ModelviewMatrixStack.Depth + 1"], "", None ),
	( "GL_NAME_STACK_DEPTH", GLint, ["ctx->Select.NameStackDepth"], "", None ),
	( "GL_NORMALIZE", GLboolean, ["ctx->Transform.Normalize"], "", None ),
	( "GL_PACK_ALIGNMENT", GLint, ["ctx->Pack.Alignment"], "", None ),
	( "GL_PACK_LSB_FIRST", GLboolean, ["ctx->Pack.LsbFirst"], "", None ),
	( "GL_PACK_ROW_LENGTH", GLint, ["ctx->Pack.RowLength"], "", None ),
	( "GL_PACK_SKIP_PIXELS", GLint, ["ctx->Pack.SkipPixels"], "", None ),
	( "GL_PACK_SKIP_ROWS", GLint, ["ctx->Pack.SkipRows"], "", None ),
	( "GL_PACK_SWAP_BYTES", GLboolean, ["ctx->Pack.SwapBytes"], "", None ),
	( "GL_PACK_SKIP_IMAGES_EXT", GLint, ["ctx->Pack.SkipImages"], "", None ),
	( "GL_PACK_IMAGE_HEIGHT_EXT", GLint, ["ctx->Pack.ImageHeight"], "", None ),
	( "GL_PACK_INVERT_MESA", GLboolean, ["ctx->Pack.Invert"], "", None ),
	( "GL_PERSPECTIVE_CORRECTION_HINT", GLenum,
	  ["ctx->Hint.PerspectiveCorrection"], "", None ),
	( "GL_PIXEL_MAP_A_TO_A_SIZE", GLint, ["ctx->Pixel.MapAtoAsize"], "", None ),
	( "GL_PIXEL_MAP_B_TO_B_SIZE", GLint, ["ctx->Pixel.MapBtoBsize"], "", None ),
	( "GL_PIXEL_MAP_G_TO_G_SIZE", GLint, ["ctx->Pixel.MapGtoGsize"], "", None ),
	( "GL_PIXEL_MAP_I_TO_A_SIZE", GLint, ["ctx->Pixel.MapItoAsize"], "", None ),
	( "GL_PIXEL_MAP_I_TO_B_SIZE", GLint, ["ctx->Pixel.MapItoBsize"], "", None ),
	( "GL_PIXEL_MAP_I_TO_G_SIZE", GLint, ["ctx->Pixel.MapItoGsize"], "", None ),
	( "GL_PIXEL_MAP_I_TO_I_SIZE", GLint, ["ctx->Pixel.MapItoIsize"], "", None ),
	( "GL_PIXEL_MAP_I_TO_R_SIZE", GLint, ["ctx->Pixel.MapItoRsize"], "", None ),
	( "GL_PIXEL_MAP_R_TO_R_SIZE", GLint, ["ctx->Pixel.MapRtoRsize"], "", None ),
	( "GL_PIXEL_MAP_S_TO_S_SIZE", GLint, ["ctx->Pixel.MapStoSsize"], "", None ),
	( "GL_POINT_SIZE", GLfloat, ["ctx->Point.Size"], "", None ),
	( "GL_POINT_SIZE_GRANULARITY", GLfloat,
	  ["ctx->Const.PointSizeGranularity"], "", None ),
	( "GL_POINT_SIZE_RANGE", GLfloat,
	  ["ctx->Const.MinPointSizeAA",
	   "ctx->Const.MaxPointSizeAA"], "", None ),
	( "GL_ALIASED_POINT_SIZE_RANGE", GLfloat,
	  ["ctx->Const.MinPointSize",
	   "ctx->Const.MaxPointSize"], "", None ),
	( "GL_POINT_SMOOTH", GLboolean, ["ctx->Point.SmoothFlag"], "", None ),
	( "GL_POINT_SMOOTH_HINT", GLenum, ["ctx->Hint.PointSmooth"], "", None ),
	( "GL_POINT_SIZE_MIN_EXT", GLfloat, ["ctx->Point.MinSize"], "", None ),
	( "GL_POINT_SIZE_MAX_EXT", GLfloat, ["ctx->Point.MaxSize"], "", None ),
	( "GL_POINT_FADE_THRESHOLD_SIZE_EXT", GLfloat,
	  ["ctx->Point.Threshold"], "", None ),
	( "GL_DISTANCE_ATTENUATION_EXT", GLfloat,
	  ["ctx->Point.Params[0]",
	   "ctx->Point.Params[1]",
	   "ctx->Point.Params[2]"], "", None ),
	( "GL_POLYGON_MODE", GLenum,
	  ["ctx->Polygon.FrontMode",
	   "ctx->Polygon.BackMode"], "", None ),
	( "GL_POLYGON_OFFSET_BIAS_EXT", GLfloat, ["ctx->Polygon.OffsetUnits"], "", None ),
	( "GL_POLYGON_OFFSET_FACTOR", GLfloat, ["ctx->Polygon.OffsetFactor "], "", None ),
	( "GL_POLYGON_OFFSET_UNITS", GLfloat, ["ctx->Polygon.OffsetUnits "], "", None ),
	( "GL_POLYGON_SMOOTH", GLboolean, ["ctx->Polygon.SmoothFlag"], "", None ),
	( "GL_POLYGON_SMOOTH_HINT", GLenum, ["ctx->Hint.PolygonSmooth"], "", None ),
	( "GL_POLYGON_STIPPLE", GLboolean, ["ctx->Polygon.StippleFlag"], "", None ),
	( "GL_PROJECTION_MATRIX", GLfloat,
	  [ "matrix[0]", "matrix[1]", "matrix[2]", "matrix[3]",
		"matrix[4]", "matrix[5]", "matrix[6]", "matrix[7]",
		"matrix[8]", "matrix[9]", "matrix[10]", "matrix[11]",
		"matrix[12]", "matrix[13]", "matrix[14]", "matrix[15]" ],
	  "const GLfloat *matrix = ctx->ProjectionMatrixStack.Top->m;", None ),
	( "GL_PROJECTION_STACK_DEPTH", GLint,
	  ["ctx->ProjectionMatrixStack.Depth + 1"], "", None ),
	( "GL_READ_BUFFER", GLenum, ["ctx->Pixel.ReadBuffer"], "", None ),
	( "GL_RED_BIAS", GLfloat, ["ctx->Pixel.RedBias"], "", None ),
	( "GL_RED_BITS", GLint, [" ctx->DrawBuffer->Visual.redBits "], "", None ),
	( "GL_RED_SCALE", GLfloat, ["ctx->Pixel.RedScale"], "", None ),
	( "GL_RENDER_MODE", GLenum, ["ctx->RenderMode"], "", None ),
	( "GL_RESCALE_NORMAL", GLboolean,
	  ["ctx->Transform.RescaleNormals"], "", None ),
	( "GL_RGBA_MODE", GLboolean, ["ctx->DrawBuffer->Visual.rgbMode"],
	  "", None ),
	( "GL_SCISSOR_BOX", GLint,
	  ["ctx->Scissor.X",
	   "ctx->Scissor.Y",
	   "ctx->Scissor.Width",
	   "ctx->Scissor.Height"], "", None ),
	( "GL_SCISSOR_TEST", GLboolean, ["ctx->Scissor.Enabled"], "", None ),
	( "GL_SELECTION_BUFFER_SIZE", GLint, ["ctx->Select.BufferSize"], "", None ),
	( "GL_SHADE_MODEL", GLenum, ["ctx->Light.ShadeModel"], "", None ),
	( "GL_SHARED_TEXTURE_PALETTE_EXT", GLboolean,
	  ["ctx->Texture.SharedPalette"], "", None ),
	( "GL_STENCIL_BITS", GLint, ["ctx->DrawBuffer->Visual.stencilBits"], "", None ),
	( "GL_STENCIL_CLEAR_VALUE", GLint, ["ctx->Stencil.Clear"], "", None ),
	( "GL_STENCIL_FAIL", GLenum,
	  ["ctx->Stencil.FailFunc[ctx->Stencil.ActiveFace]"], "", None ),
	( "GL_STENCIL_FUNC", GLenum,
	  ["ctx->Stencil.Function[ctx->Stencil.ActiveFace]"], "", None ),
	( "GL_STENCIL_PASS_DEPTH_FAIL", GLenum,
	  ["ctx->Stencil.ZFailFunc[ctx->Stencil.ActiveFace]"], "", None ),
	( "GL_STENCIL_PASS_DEPTH_PASS", GLenum,
	  ["ctx->Stencil.ZPassFunc[ctx->Stencil.ActiveFace]"], "", None ),
	( "GL_STENCIL_REF", GLint,
	  ["ctx->Stencil.Ref[ctx->Stencil.ActiveFace]"], "", None ),
	( "GL_STENCIL_TEST", GLboolean, ["ctx->Stencil.Enabled"], "", None ),
	( "GL_STENCIL_VALUE_MASK", GLint,
	  ["ctx->Stencil.ValueMask[ctx->Stencil.ActiveFace]"], "", None ),
	( "GL_STENCIL_WRITEMASK", GLint,
	  ["ctx->Stencil.WriteMask[ctx->Stencil.ActiveFace]"], "", None ),
	( "GL_STEREO", GLboolean, ["ctx->DrawBuffer->Visual.stereoMode"],
	  "", None ),
	( "GL_SUBPIXEL_BITS", GLint, ["ctx->Const.SubPixelBits"], "", None ),
	( "GL_TEXTURE_1D", GLboolean, ["_mesa_IsEnabled(GL_TEXTURE_1D)"], "", None ),
	( "GL_TEXTURE_2D", GLboolean, ["_mesa_IsEnabled(GL_TEXTURE_2D)"], "", None ),
	( "GL_TEXTURE_3D", GLboolean, ["_mesa_IsEnabled(GL_TEXTURE_3D)"], "", None ),
	( "GL_TEXTURE_BINDING_1D", GLint,
	  ["ctx->Texture.Unit[ctx->Texture.CurrentUnit].Current1D->Name"], "", None ),
	( "GL_TEXTURE_BINDING_2D", GLint,
	  ["ctx->Texture.Unit[ctx->Texture.CurrentUnit].Current2D->Name"], "", None ),
	( "GL_TEXTURE_BINDING_3D", GLint,
	  ["ctx->Texture.Unit[ctx->Texture.CurrentUnit].Current3D->Name"], "", None ),
	( "GL_TEXTURE_ENV_COLOR", GLfloatN,
	  ["color[0]", "color[1]", "color[2]", "color[3]"],
	  "const GLfloat *color = ctx->Texture.Unit[ctx->Texture.CurrentUnit].EnvColor;",
	  None ),
	( "GL_TEXTURE_ENV_MODE", GLenum,
	  ["ctx->Texture.Unit[ctx->Texture.CurrentUnit].EnvMode"], "", None ),
	( "GL_TEXTURE_GEN_S", GLboolean,
	  ["((ctx->Texture.Unit[ctx->Texture.CurrentUnit].TexGenEnabled & S_BIT) ? 1 : 0)"], "", None ),
	( "GL_TEXTURE_GEN_T", GLboolean,
	  ["((ctx->Texture.Unit[ctx->Texture.CurrentUnit].TexGenEnabled & T_BIT) ? 1 : 0)"], "", None ),
	( "GL_TEXTURE_GEN_R", GLboolean,
	  ["((ctx->Texture.Unit[ctx->Texture.CurrentUnit].TexGenEnabled & R_BIT) ? 1 : 0)"], "", None ),
	( "GL_TEXTURE_GEN_Q", GLboolean,
	  ["((ctx->Texture.Unit[ctx->Texture.CurrentUnit].TexGenEnabled & Q_BIT) ? 1 : 0)"], "", None ),
	( "GL_TEXTURE_MATRIX", GLfloat,
	  ["matrix[0]", "matrix[1]", "matrix[2]", "matrix[3]",
	   "matrix[4]", "matrix[5]", "matrix[6]", "matrix[7]",
	   "matrix[8]", "matrix[9]", "matrix[10]", "matrix[11]",
	   "matrix[12]", "matrix[13]", "matrix[14]", "matrix[15]" ],
	  "const GLfloat *matrix = ctx->TextureMatrixStack[ctx->Texture.CurrentUnit].Top->m;", None ),
	( "GL_TEXTURE_STACK_DEPTH", GLint,
	  ["ctx->TextureMatrixStack[ctx->Texture.CurrentUnit].Depth + 1"], "", None ),
	( "GL_UNPACK_ALIGNMENT", GLint, ["ctx->Unpack.Alignment"], "", None ),
	( "GL_UNPACK_LSB_FIRST", GLboolean, ["ctx->Unpack.LsbFirst"], "", None ),
	( "GL_UNPACK_ROW_LENGTH", GLint, ["ctx->Unpack.RowLength"], "", None ),
	( "GL_UNPACK_SKIP_PIXELS", GLint, ["ctx->Unpack.SkipPixels"], "", None ),
	( "GL_UNPACK_SKIP_ROWS", GLint, ["ctx->Unpack.SkipRows"], "", None ),
	( "GL_UNPACK_SWAP_BYTES", GLboolean, ["ctx->Unpack.SwapBytes"], "", None ),
	( "GL_UNPACK_SKIP_IMAGES_EXT", GLint, ["ctx->Unpack.SkipImages"], "", None ),
	( "GL_UNPACK_IMAGE_HEIGHT_EXT", GLint, ["ctx->Unpack.ImageHeight"], "", None ),
	( "GL_UNPACK_CLIENT_STORAGE_APPLE", GLboolean, ["ctx->Unpack.ClientStorage"], "", None ),
	( "GL_VIEWPORT", GLint, [ "ctx->Viewport.X", "ctx->Viewport.Y",
	  "ctx->Viewport.Width", "ctx->Viewport.Height" ], "", None ),
	( "GL_ZOOM_X", GLfloat, ["ctx->Pixel.ZoomX"], "", None ),
	( "GL_ZOOM_Y", GLfloat, ["ctx->Pixel.ZoomY"], "", None ),

	# Vertex arrays
	( "GL_VERTEX_ARRAY", GLboolean, ["ctx->Array.Vertex.Enabled"], "", None ),
	( "GL_VERTEX_ARRAY_SIZE", GLint, ["ctx->Array.Vertex.Size"], "", None ),
	( "GL_VERTEX_ARRAY_TYPE", GLenum, ["ctx->Array.Vertex.Type"], "", None ),
	( "GL_VERTEX_ARRAY_STRIDE", GLint, ["ctx->Array.Vertex.Stride"], "", None ),
	( "GL_VERTEX_ARRAY_COUNT_EXT", GLint, ["0"], "", None ),
	( "GL_NORMAL_ARRAY", GLenum, ["ctx->Array.Normal.Enabled"], "", None ),
	( "GL_NORMAL_ARRAY_TYPE", GLenum, ["ctx->Array.Normal.Type"], "", None ),
	( "GL_NORMAL_ARRAY_STRIDE", GLint, ["ctx->Array.Normal.Stride"], "", None ),
	( "GL_NORMAL_ARRAY_COUNT_EXT", GLint, ["0"], "", None ),
	( "GL_COLOR_ARRAY", GLboolean, ["ctx->Array.Color.Enabled"], "", None ),
	( "GL_COLOR_ARRAY_SIZE", GLint, ["ctx->Array.Color.Size"], "", None ),
	( "GL_COLOR_ARRAY_TYPE", GLenum, ["ctx->Array.Color.Type"], "", None ),
	( "GL_COLOR_ARRAY_STRIDE", GLint, ["ctx->Array.Color.Stride"], "", None ),
	( "GL_COLOR_ARRAY_COUNT_EXT", GLint, ["0"], "", None ),
	( "GL_INDEX_ARRAY", GLboolean, ["ctx->Array.Index.Enabled"], "", None ),
	( "GL_INDEX_ARRAY_TYPE", GLenum, ["ctx->Array.Index.Type"], "", None ),
	( "GL_INDEX_ARRAY_STRIDE", GLint, ["ctx->Array.Index.Stride"], "", None ),
	( "GL_INDEX_ARRAY_COUNT_EXT", GLint, ["0"], "", None ),
	( "GL_TEXTURE_COORD_ARRAY", GLboolean,
	  ["ctx->Array.TexCoord[ctx->Array.ActiveTexture].Enabled"], "", None ),
	( "GL_TEXTURE_COORD_ARRAY_SIZE", GLint,
	  ["ctx->Array.TexCoord[ctx->Array.ActiveTexture].Size"], "", None ),
	( "GL_TEXTURE_COORD_ARRAY_TYPE", GLenum,
	  ["ctx->Array.TexCoord[ctx->Array.ActiveTexture].Type"], "", None ),
	( "GL_TEXTURE_COORD_ARRAY_STRIDE", GLint,
	  ["ctx->Array.TexCoord[ctx->Array.ActiveTexture].Stride"], "", None ),
	( "GL_TEXTURE_COORD_ARRAY_COUNT_EXT", GLint, ["0"], "", None ),
	( "GL_EDGE_FLAG_ARRAY", GLboolean, ["ctx->Array.EdgeFlag.Enabled"], "", None ),
	( "GL_EDGE_FLAG_ARRAY_STRIDE", GLint, ["ctx->Array.EdgeFlag.Stride"], "", None ),
	( "GL_EDGE_FLAG_ARRAY_COUNT_EXT", GLint, ["0"], "", None ),

	# GL_ARB_multitexture
	( "GL_MAX_TEXTURE_UNITS_ARB", GLint,
	  ["MIN2(ctx->Const.MaxTextureImageUnits, ctx->Const.MaxTextureCoordUnits)"],
	  "", "ARB_multitexture" ),
	( "GL_ACTIVE_TEXTURE_ARB", GLint,
	  [ "GL_TEXTURE0_ARB + ctx->Texture.CurrentUnit"], "", "ARB_multitexture" ),
	( "GL_CLIENT_ACTIVE_TEXTURE_ARB", GLint,
	  ["GL_TEXTURE0_ARB + ctx->Array.ActiveTexture"], "", "ARB_multitexture" ),

	# GL_ARB_texture_cube_map
	( "GL_TEXTURE_CUBE_MAP_ARB", GLboolean,
	  ["_mesa_IsEnabled(GL_TEXTURE_CUBE_MAP_ARB)"], "", "ARB_texture_cube_map" ),
	( "GL_TEXTURE_BINDING_CUBE_MAP_ARB", GLint,
	  ["ctx->Texture.Unit[ctx->Texture.CurrentUnit].CurrentCubeMap->Name"],
	  "", "ARB_texture_cube_map" ),
	( "GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB", GLint,
	  ["(1 << (ctx->Const.MaxCubeTextureLevels - 1))"],
	  "", "ARB_texture_cube_map"),

	# GL_ARB_texture_compression */
	( "GL_TEXTURE_COMPRESSION_HINT_ARB", GLint,
	  ["ctx->Hint.TextureCompression"], "", "ARB_texture_compression" ),
	( "GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB", GLint,
	  ["_mesa_get_compressed_formats(ctx, NULL)"], "", "ARB_texture_compression" ),
	( "GL_COMPRESSED_TEXTURE_FORMATS_ARB", GLenum,
	  [],
	  """GLint formats[100];
         GLuint i, n = _mesa_get_compressed_formats(ctx, formats);
         ASSERT(n <= 100);
         for (i = 0; i < n; i++)
            params[i] = ENUM_TO_INT(formats[i]);""",
	  "ARB_texture_compression" ),

	# GL_EXT_compiled_vertex_array
	( "GL_ARRAY_ELEMENT_LOCK_FIRST_EXT", GLint, ["ctx->Array.LockFirst"],
	  "", "EXT_compiled_vertex_array" ),
	( "GL_ARRAY_ELEMENT_LOCK_COUNT_EXT", GLint, ["ctx->Array.LockCount"],
	  "", "EXT_compiled_vertex_array" ),

	# GL_ARB_transpose_matrix
	( "GL_TRANSPOSE_COLOR_MATRIX_ARB", GLfloat,
	  ["matrix[0]", "matrix[4]", "matrix[8]", "matrix[12]",
	   "matrix[1]", "matrix[5]", "matrix[9]", "matrix[13]",
	   "matrix[2]", "matrix[6]", "matrix[10]", "matrix[14]",
	   "matrix[3]", "matrix[7]", "matrix[11]", "matrix[15]"],
	  "const GLfloat *matrix = ctx->ColorMatrixStack.Top->m;", None ),
	( "GL_TRANSPOSE_MODELVIEW_MATRIX_ARB", GLfloat,
	  ["matrix[0]", "matrix[4]", "matrix[8]", "matrix[12]",
	   "matrix[1]", "matrix[5]", "matrix[9]", "matrix[13]",
	   "matrix[2]", "matrix[6]", "matrix[10]", "matrix[14]",
	   "matrix[3]", "matrix[7]", "matrix[11]", "matrix[15]"],
	  "const GLfloat *matrix = ctx->ModelviewMatrixStack.Top->m;", None ),
	( "GL_TRANSPOSE_PROJECTION_MATRIX_ARB", GLfloat,
	  ["matrix[0]", "matrix[4]", "matrix[8]", "matrix[12]",
	   "matrix[1]", "matrix[5]", "matrix[9]", "matrix[13]",
	   "matrix[2]", "matrix[6]", "matrix[10]", "matrix[14]",
	   "matrix[3]", "matrix[7]", "matrix[11]", "matrix[15]"],
	  "const GLfloat *matrix = ctx->ProjectionMatrixStack.Top->m;", None ),
	( "GL_TRANSPOSE_TEXTURE_MATRIX_ARB", GLfloat,
	  ["matrix[0]", "matrix[4]", "matrix[8]", "matrix[12]",
	   "matrix[1]", "matrix[5]", "matrix[9]", "matrix[13]",
	   "matrix[2]", "matrix[6]", "matrix[10]", "matrix[14]",
	   "matrix[3]", "matrix[7]", "matrix[11]", "matrix[15]"],
	  "const GLfloat *matrix = ctx->TextureMatrixStack[ctx->Texture.CurrentUnit].Top->m;", None ),

	# GL_HP_occlusion_test
	( "GL_OCCLUSION_TEST_HP", GLboolean, ["ctx->Depth.OcclusionTest"], "",
	  "HP_occlusion_test" ),
	( "GL_OCCLUSION_TEST_RESULT_HP", GLboolean, [],
	  """FLUSH_VERTICES(ctx, _NEW_DEPTH);
         if (ctx->Depth.OcclusionTest)
            params[0] = ctx->OcclusionResult;
         else
            params[0] = ctx->OcclusionResultSaved;
         /* reset flag now */
         ctx->OcclusionResult = GL_FALSE;
         ctx->OcclusionResultSaved = GL_FALSE;
         return;""",
	  "HP_occlusion_test" ),

	# GL_SGIS_pixel_texture
	( "GL_PIXEL_TEXTURE_SGIS", GLboolean, ["ctx->Pixel.PixelTextureEnabled"],
	  "", "SGIS_pixel_texture" ),

	# GL_SGIX_pixel_texture
	( "GL_PIXEL_TEX_GEN_SGIX", GLboolean, ["ctx->Pixel.PixelTextureEnabled"],
	  "", "SGIX_pixel_texture" ),
	( "GL_PIXEL_TEX_GEN_MODE_SGIX", GLenum, ["pixel_texgen_mode(ctx)"],
	  "", "SGIX_pixel_texture" ),

	# GL_SGI_color_matrix (also in 1.2 imaging)
	( "GL_COLOR_MATRIX_SGI", GLfloat,
	  ["matrix[0]", "matrix[1]", "matrix[2]", "matrix[3]",
	   "matrix[4]", "matrix[5]", "matrix[6]", "matrix[7]",
	   "matrix[8]", "matrix[9]", "matrix[10]", "matrix[11]",
	   "matrix[12]", "matrix[13]", "matrix[14]", "matrix[15]" ],
	  "const GLfloat *matrix = ctx->ColorMatrixStack.Top->m;", None ),
	( "GL_COLOR_MATRIX_STACK_DEPTH_SGI", GLint,
	  ["ctx->ColorMatrixStack.Depth + 1"], "", None ),
	( "GL_MAX_COLOR_MATRIX_STACK_DEPTH_SGI", GLint,
	  ["MAX_COLOR_STACK_DEPTH"], "", None ),
	( "GL_POST_COLOR_MATRIX_RED_SCALE_SGI", GLfloat,
	  ["ctx->Pixel.PostColorMatrixScale[0]"], "", None ),
	( "GL_POST_COLOR_MATRIX_GREEN_SCALE_SGI", GLfloat,
	  ["ctx->Pixel.PostColorMatrixScale[1]"], "", None ),
	( "GL_POST_COLOR_MATRIX_BLUE_SCALE_SGI", GLfloat,
	  ["ctx->Pixel.PostColorMatrixScale[2]"], "", None ),
	( "GL_POST_COLOR_MATRIX_ALPHA_SCALE_SGI", GLfloat,
	  ["ctx->Pixel.PostColorMatrixScale[3]"], "", None ),
	( "GL_POST_COLOR_MATRIX_RED_BIAS_SGI", GLfloat,
	  ["ctx->Pixel.PostColorMatrixBias[0]"], "", None ),
	( "GL_POST_COLOR_MATRIX_GREEN_BIAS_SGI", GLfloat,
	  ["ctx->Pixel.PostColorMatrixBias[1]"], "", None ),
	( "GL_POST_COLOR_MATRIX_BLUE_BIAS_SGI", GLfloat,
	  ["ctx->Pixel.PostColorMatrixBias[2]"], "", None ),
	( "GL_POST_COLOR_MATRIX_ALPHA_BIAS_SGI", GLfloat,
	  ["ctx->Pixel.PostColorMatrixBias[3]"], "", None ),

	# GL_EXT_convolution (also in 1.2 imaging)
	( "GL_CONVOLUTION_1D_EXT", GLboolean,
	  ["ctx->Pixel.Convolution1DEnabled"], "", "EXT_convolution" ),
	( "GL_CONVOLUTION_2D_EXT", GLboolean,
	  ["ctx->Pixel.Convolution2DEnabled"], "", "EXT_convolution" ),
	( "GL_SEPARABLE_2D_EXT", GLboolean,
	  ["ctx->Pixel.Separable2DEnabled"], "", "EXT_convolution" ),
	( "GL_POST_CONVOLUTION_RED_SCALE_EXT", GLfloat,
	  ["ctx->Pixel.PostConvolutionScale[0]"], "", "EXT_convolution" ),
	( "GL_POST_CONVOLUTION_GREEN_SCALE_EXT", GLfloat,
	  ["ctx->Pixel.PostConvolutionScale[1]"], "", "EXT_convolution" ),
	( "GL_POST_CONVOLUTION_BLUE_SCALE_EXT", GLfloat,
	  ["ctx->Pixel.PostConvolutionScale[2]"], "", "EXT_convolution" ),
	( "GL_POST_CONVOLUTION_ALPHA_SCALE_EXT", GLfloat,
	  ["ctx->Pixel.PostConvolutionScale[3]"], "", "EXT_convolution" ),
	( "GL_POST_CONVOLUTION_RED_BIAS_EXT", GLfloat,
	  ["ctx->Pixel.PostConvolutionBias[0]"], "", "EXT_convolution" ),
	( "GL_POST_CONVOLUTION_GREEN_BIAS_EXT", GLfloat,
	  ["ctx->Pixel.PostConvolutionBias[1]"], "", "EXT_convolution" ),
	( "GL_POST_CONVOLUTION_BLUE_BIAS_EXT", GLfloat,
	  ["ctx->Pixel.PostConvolutionBias[2]"], "", "EXT_convolution" ),
	( "GL_POST_CONVOLUTION_ALPHA_BIAS_EXT", GLfloat,
	  ["ctx->Pixel.PostConvolutionBias[3]"], "", "EXT_convolution" ),

	# GL_EXT_histogram / GL_ARB_imaging
	( "GL_HISTOGRAM", GLboolean,
	  [ "ctx->Pixel.HistogramEnabled" ], "", "EXT_histogram" ),
	( "GL_MINMAX", GLboolean,
	  [ "ctx->Pixel.MinMaxEnabled" ], "", "EXT_histogram" ),

	# GL_SGI_color_table / GL_ARB_imaging
	( "GL_COLOR_TABLE_SGI", GLboolean,
	  ["ctx->Pixel.ColorTableEnabled"], "", "SGI_color_table" ),
	( "GL_POST_CONVOLUTION_COLOR_TABLE_SGI", GLboolean,
	  ["ctx->Pixel.PostConvolutionColorTableEnabled"], "", "SGI_color_table" ),
	( "GL_POST_COLOR_MATRIX_COLOR_TABLE_SGI", GLboolean,
	  ["ctx->Pixel.PostColorMatrixColorTableEnabled"], "", "SGI_color_table" ),

	# GL_SGI_texture_color_table
	( "GL_TEXTURE_COLOR_TABLE_SGI", GLboolean,
	  ["ctx->Texture.Unit[ctx->Texture.CurrentUnit].ColorTableEnabled"],
	  "", "SGI_texture_color_table" ),

	# GL_EXT_secondary_color
	( "GL_COLOR_SUM_EXT", GLboolean,
	  ["ctx->Fog.ColorSumEnabled"], "", "EXT_secondary_color" ),
	( "GL_CURRENT_SECONDARY_COLOR_EXT", GLfloatN,
	  ["ctx->Current.Attrib[VERT_ATTRIB_COLOR1][0]",
	   "ctx->Current.Attrib[VERT_ATTRIB_COLOR1][1]",
	   "ctx->Current.Attrib[VERT_ATTRIB_COLOR1][2]",
	   "ctx->Current.Attrib[VERT_ATTRIB_COLOR1][3]"],
	  "FLUSH_CURRENT(ctx, 0);", "EXT_secondary_color" ),
	( "GL_SECONDARY_COLOR_ARRAY_EXT", GLboolean,
	  ["ctx->Array.SecondaryColor.Enabled"], "", "EXT_secondary_color" ),
	( "GL_SECONDARY_COLOR_ARRAY_TYPE_EXT", GLenum,
	  ["ctx->Array.SecondaryColor.Type"], "",  "EXT_secondary_color" ),
	( "GL_SECONDARY_COLOR_ARRAY_STRIDE_EXT", GLint,
	  ["ctx->Array.SecondaryColor.Stride"], "", "EXT_secondary_color" ),
	( "GL_SECONDARY_COLOR_ARRAY_SIZE_EXT", GLint,
	  ["ctx->Array.SecondaryColor.Size"], "", "EXT_secondary_color" ),

	# GL_EXT_fog_coord
	( "GL_CURRENT_FOG_COORDINATE_EXT", GLfloat,
	  ["ctx->Current.Attrib[VERT_ATTRIB_FOG][0]"],
	  "FLUSH_CURRENT(ctx, 0);", "EXT_fog_coord" ),
	( "GL_FOG_COORDINATE_ARRAY_EXT", GLboolean, ["ctx->Array.FogCoord.Enabled"],
	  "",  "EXT_fog_coord" ),
	( "GL_FOG_COORDINATE_ARRAY_TYPE_EXT", GLenum, ["ctx->Array.FogCoord.Type"],
	  "", "EXT_fog_coord" ),
	( "GL_FOG_COORDINATE_ARRAY_STRIDE_EXT", GLint, ["ctx->Array.FogCoord.Stride"],
	  "", "EXT_fog_coord" ),
	( "GL_FOG_COORDINATE_SOURCE_EXT", GLenum, ["ctx->Fog.FogCoordinateSource"],
	  "", "EXT_fog_coord" ),

	# GL_EXT_texture_lod_bias
	( "GL_MAX_TEXTURE_LOD_BIAS_EXT", GLfloat,
	  ["ctx->Const.MaxTextureLodBias"], "", "EXT_texture_lod_bias"),

	# GL_EXT_texture_filter_anisotropic
	( "GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT", GLfloat,
	  ["ctx->Const.MaxTextureMaxAnisotropy"], "", "EXT_texture_filter_anisotropic"),

	# GL_ARB_multisample
	( "GL_MULTISAMPLE_ARB", GLboolean,
	  ["ctx->Multisample.Enabled"], "", "ARB_multisample" ),
	( "GL_SAMPLE_ALPHA_TO_COVERAGE_ARB", GLboolean,
	  ["ctx->Multisample.SampleAlphaToCoverage"], "", "ARB_multisample" ),
	( "GL_SAMPLE_ALPHA_TO_ONE_ARB", GLboolean,
	  ["ctx->Multisample.SampleAlphaToOne"], "", "ARB_multisample" ),
	( "GL_SAMPLE_COVERAGE_ARB", GLboolean,
	  ["ctx->Multisample.SampleCoverage"], "", "ARB_multisample" ),
	( "GL_SAMPLE_COVERAGE_VALUE_ARB", GLfloat,
	  ["ctx->Multisample.SampleCoverageValue"], "", "ARB_multisample" ),
	( "GL_SAMPLE_COVERAGE_INVERT_ARB", GLboolean,
	  ["ctx->Multisample.SampleCoverageInvert"], "", "ARB_multisample" ),
	( "GL_SAMPLE_BUFFERS_ARB", GLint,
	  ["ctx->DrawBuffer->Visual.sampleBuffers"], "", "ARB_multisample" ),
	( "GL_SAMPLES_ARB", GLint,
	  ["ctx->DrawBuffer->Visual.samples"], "", "ARB_multisample" ),

	# GL_IBM_rasterpos_clip
	( "GL_RASTER_POSITION_UNCLIPPED_IBM", GLboolean,
	  ["ctx->Transform.RasterPositionUnclipped"], "", "IBM_rasterpos_clip" ),

	# GL_NV_point_sprite
	( "GL_POINT_SPRITE_NV", GLboolean, ["ctx->Point.PointSprite"],
	  "", "NV_point_sprite" ), # OR ARB_point_sprite
	( "GL_POINT_SPRITE_R_MODE_NV", GLenum, ["ctx->Point.SpriteRMode"],
	  "", "NV_point_sprite" ), # OR ARB_point_sprite
	( "GL_POINT_SPRITE_COORD_ORIGIN", GLenum, ["ctx->Point.SpriteOrigin"],
	  "", "NV_point_sprite" ), # OR ARB_point_sprite

	# GL_SGIS_generate_mipmap
	( "GL_GENERATE_MIPMAP_HINT_SGIS", GLenum, ["ctx->Hint.GenerateMipmap"],
	  "", "SGIS_generate_mipmap" ),

	# GL_NV_vertex_program
	( "GL_VERTEX_PROGRAM_NV", GLboolean,
	  ["ctx->VertexProgram.Enabled"], "", "NV_vertex_program" ),
	( "GL_VERTEX_PROGRAM_POINT_SIZE_NV", GLboolean,
	  ["ctx->VertexProgram.PointSizeEnabled"], "", "NV_vertex_program" ),
	( "GL_VERTEX_PROGRAM_TWO_SIDE_NV", GLboolean,
	  ["ctx->VertexProgram.TwoSideEnabled"], "", "NV_vertex_program" ),
	( "GL_MAX_TRACK_MATRIX_STACK_DEPTH_NV", GLint,
	  ["ctx->Const.MaxProgramMatrixStackDepth"], "", "NV_vertex_program" ),
	( "GL_MAX_TRACK_MATRICES_NV", GLint,
	  ["ctx->Const.MaxProgramMatrices"], "", "NV_vertex_program" ),
	( "GL_CURRENT_MATRIX_STACK_DEPTH_NV", GLboolean,
	  ["ctx->CurrentStack->Depth + 1"], "", "NV_vertex_program" ),
	( "GL_CURRENT_MATRIX_NV", GLfloat,
	  ["matrix[0]", "matrix[1]", "matrix[2]", "matrix[3]",
	   "matrix[4]", "matrix[5]", "matrix[6]", "matrix[7]",
	   "matrix[8]", "matrix[9]", "matrix[10]", "matrix[11]",
	   "matrix[12]", "matrix[13]", "matrix[14]", "matrix[15]" ],
	  "const GLfloat *matrix = ctx->CurrentStack->Top->m;", "NV_vertex_program" ),
	( "GL_VERTEX_PROGRAM_BINDING_NV", GLint,
	  ["(ctx->VertexProgram.Current ? ctx->VertexProgram.Current->Base.Id : 0)"],
	  "", "NV_vertex_program" ),
	( "GL_PROGRAM_ERROR_POSITION_NV", GLint,
	  ["ctx->Program.ErrorPos"], "", "NV_vertex_program" ),
	( "GL_VERTEX_ATTRIB_ARRAY0_NV", GLboolean,
	  ["ctx->Array.VertexAttrib[0].Enabled"], "", "NV_vertex_program" ),
	( "GL_VERTEX_ATTRIB_ARRAY1_NV", GLboolean,
	  ["ctx->Array.VertexAttrib[1].Enabled"], "", "NV_vertex_program" ),
	( "GL_VERTEX_ATTRIB_ARRAY2_NV", GLboolean,
	  ["ctx->Array.VertexAttrib[2].Enabled"], "", "NV_vertex_program" ),
	( "GL_VERTEX_ATTRIB_ARRAY3_NV", GLboolean,
	  ["ctx->Array.VertexAttrib[3].Enabled"], "", "NV_vertex_program" ),
	( "GL_VERTEX_ATTRIB_ARRAY4_NV", GLboolean,
	  ["ctx->Array.VertexAttrib[4].Enabled"], "", "NV_vertex_program" ),
	( "GL_VERTEX_ATTRIB_ARRAY5_NV", GLboolean,
	  ["ctx->Array.VertexAttrib[5].Enabled"], "", "NV_vertex_program" ),
	( "GL_VERTEX_ATTRIB_ARRAY6_NV", GLboolean,
	  ["ctx->Array.VertexAttrib[6].Enabled"], "", "NV_vertex_program" ),
	( "GL_VERTEX_ATTRIB_ARRAY7_NV", GLboolean,
	  ["ctx->Array.VertexAttrib[7].Enabled"], "", "NV_vertex_program" ),
	( "GL_VERTEX_ATTRIB_ARRAY8_NV", GLboolean,
	  ["ctx->Array.VertexAttrib[8].Enabled"], "", "NV_vertex_program" ),
	( "GL_VERTEX_ATTRIB_ARRAY9_NV", GLboolean,
	  ["ctx->Array.VertexAttrib[9].Enabled"], "", "NV_vertex_program" ),
	( "GL_VERTEX_ATTRIB_ARRAY10_NV", GLboolean,
	  ["ctx->Array.VertexAttrib[10].Enabled"], "", "NV_vertex_program" ),
	( "GL_VERTEX_ATTRIB_ARRAY11_NV", GLboolean,
	  ["ctx->Array.VertexAttrib[11].Enabled"], "", "NV_vertex_program" ),
	( "GL_VERTEX_ATTRIB_ARRAY12_NV", GLboolean,
	  ["ctx->Array.VertexAttrib[12].Enabled"], "", "NV_vertex_program" ),
	( "GL_VERTEX_ATTRIB_ARRAY13_NV", GLboolean,
	  ["ctx->Array.VertexAttrib[13].Enabled"], "", "NV_vertex_program" ),
	( "GL_VERTEX_ATTRIB_ARRAY14_NV", GLboolean,
	  ["ctx->Array.VertexAttrib[14].Enabled"], "", "NV_vertex_program" ),
	( "GL_VERTEX_ATTRIB_ARRAY15_NV", GLboolean,
	  ["ctx->Array.VertexAttrib[15].Enabled"], "", "NV_vertex_program" ),
	( "GL_MAP1_VERTEX_ATTRIB0_4_NV", GLboolean,
	  ["ctx->Eval.Map1Attrib[0]"], "", "NV_vertex_program" ),
	( "GL_MAP1_VERTEX_ATTRIB1_4_NV", GLboolean,
	  ["ctx->Eval.Map1Attrib[1]"], "", "NV_vertex_program" ),
	( "GL_MAP1_VERTEX_ATTRIB2_4_NV", GLboolean,
	  ["ctx->Eval.Map1Attrib[2]"], "", "NV_vertex_program" ),
	( "GL_MAP1_VERTEX_ATTRIB3_4_NV", GLboolean,
	  ["ctx->Eval.Map1Attrib[3]"], "", "NV_vertex_program" ),
	( "GL_MAP1_VERTEX_ATTRIB4_4_NV", GLboolean,
	  ["ctx->Eval.Map1Attrib[4]"], "", "NV_vertex_program" ),
	( "GL_MAP1_VERTEX_ATTRIB5_4_NV", GLboolean,
	  ["ctx->Eval.Map1Attrib[5]"], "", "NV_vertex_program" ),
	( "GL_MAP1_VERTEX_ATTRIB6_4_NV", GLboolean,
	  ["ctx->Eval.Map1Attrib[6]"], "", "NV_vertex_program" ),
	( "GL_MAP1_VERTEX_ATTRIB7_4_NV", GLboolean,
	  ["ctx->Eval.Map1Attrib[7]"], "", "NV_vertex_program" ),
	( "GL_MAP1_VERTEX_ATTRIB8_4_NV", GLboolean,
	  ["ctx->Eval.Map1Attrib[8]"], "", "NV_vertex_program" ),
	( "GL_MAP1_VERTEX_ATTRIB9_4_NV", GLboolean,
	  ["ctx->Eval.Map1Attrib[9]"], "", "NV_vertex_program" ),
	( "GL_MAP1_VERTEX_ATTRIB10_4_NV", GLboolean,
	  ["ctx->Eval.Map1Attrib[10]"], "", "NV_vertex_program" ),
	( "GL_MAP1_VERTEX_ATTRIB11_4_NV", GLboolean,
	  ["ctx->Eval.Map1Attrib[11]"], "", "NV_vertex_program" ),
	( "GL_MAP1_VERTEX_ATTRIB12_4_NV", GLboolean,
	  ["ctx->Eval.Map1Attrib[12]"], "", "NV_vertex_program" ),
	( "GL_MAP1_VERTEX_ATTRIB13_4_NV", GLboolean,
	  ["ctx->Eval.Map1Attrib[13]"], "", "NV_vertex_program" ),
	( "GL_MAP1_VERTEX_ATTRIB14_4_NV", GLboolean,
	  ["ctx->Eval.Map1Attrib[14]"], "", "NV_vertex_program" ),
	( "GL_MAP1_VERTEX_ATTRIB15_4_NV", GLboolean,
	  ["ctx->Eval.Map1Attrib[15]"], "", "NV_vertex_program" ),

	# GL_NV_fragment_program
	( "GL_FRAGMENT_PROGRAM_NV", GLboolean,
	  ["ctx->FragmentProgram.Enabled"], "", "NV_fragment_program" ),
	( "GL_MAX_TEXTURE_COORDS_NV", GLint,
	  ["ctx->Const.MaxTextureCoordUnits"], "", "NV_fragment_program" ),
	( "GL_MAX_TEXTURE_IMAGE_UNITS_NV", GLint,
	  ["ctx->Const.MaxTextureImageUnits"], "", "NV_fragment_program" ),
	( "GL_FRAGMENT_PROGRAM_BINDING_NV", GLint,
	  ["ctx->FragmentProgram.Current ? ctx->FragmentProgram.Current->Base.Id : 0"],
	  "", "NV_fragment_program" ),
	( "GL_MAX_FRAGMENT_PROGRAM_LOCAL_PARAMETERS_NV", GLint,
	  ["MAX_NV_FRAGMENT_PROGRAM_PARAMS"], "", "NV_fragment_program" ),

	# GL_NV_texture_rectangle
	( "GL_TEXTURE_RECTANGLE_NV", GLboolean,
	  ["_mesa_IsEnabled(GL_TEXTURE_RECTANGLE_NV)"], "", "NV_texture_rectangle" ),
	( "GL_TEXTURE_BINDING_RECTANGLE_NV", GLint,
	  ["ctx->Texture.Unit[ctx->Texture.CurrentUnit].CurrentRect->Name"],
	  "", "NV_texture_rectangle" ),
	( "GL_MAX_RECTANGLE_TEXTURE_SIZE_NV", GLint,
	  ["ctx->Const.MaxTextureRectSize"], "", "NV_texture_rectangle" ),

	# GL_EXT_stencil_two_side
	( "GL_STENCIL_TEST_TWO_SIDE_EXT", GLboolean,
	  ["ctx->Stencil.TestTwoSide"], "", "EXT_stencil_two_side" ),
	( "GL_ACTIVE_STENCIL_FACE_EXT", GLenum,
	  ["ctx->Stencil.ActiveFace ? GL_BACK : GL_FRONT"],
	  "", "EXT_stencil_two_side" ),

	# GL_NV_light_max_exponent
	( "GL_MAX_SHININESS_NV", GLfloat,
	  ["ctx->Const.MaxShininess"], "", "NV_light_max_exponent" ),
	( "GL_MAX_SPOT_EXPONENT_NV", GLfloat,
	  ["ctx->Const.MaxSpotExponent"], "", "NV_light_max_exponent" ),

	# GL_ARB_vertex_buffer_object
	( "GL_ARRAY_BUFFER_BINDING_ARB", GLint,
	  ["ctx->Array.ArrayBufferObj->Name"], "", "ARB_vertex_buffer_object" ),
	( "GL_VERTEX_ARRAY_BUFFER_BINDING_ARB", GLint,
	  ["ctx->Array.Vertex.BufferObj->Name"], "", "ARB_vertex_buffer_object" ),
	( "GL_NORMAL_ARRAY_BUFFER_BINDING_ARB", GLint,
	  ["ctx->Array.Normal.BufferObj->Name"], "", "ARB_vertex_buffer_object" ),
	( "GL_COLOR_ARRAY_BUFFER_BINDING_ARB", GLint,
	  ["ctx->Array.Color.BufferObj->Name"], "", "ARB_vertex_buffer_object" ),
	( "GL_INDEX_ARRAY_BUFFER_BINDING_ARB", GLint,
	  ["ctx->Array.Index.BufferObj->Name"], "", "ARB_vertex_buffer_object" ),
	( "GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING_ARB", GLint,
	  ["ctx->Array.TexCoord[ctx->Array.ActiveTexture].BufferObj->Name"],
	  "", "ARB_vertex_buffer_object" ),
	( "GL_EDGE_FLAG_ARRAY_BUFFER_BINDING_ARB", GLint,
	  ["ctx->Array.EdgeFlag.BufferObj->Name"], "", "ARB_vertex_buffer_object" ),
	( "GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING_ARB", GLint,
	  ["ctx->Array.SecondaryColor.BufferObj->Name"],
	  "", "ARB_vertex_buffer_object" ),
	( "GL_FOG_COORDINATE_ARRAY_BUFFER_BINDING_ARB", GLint,
	  ["ctx->Array.FogCoord.BufferObj->Name"], "", "ARB_vertex_buffer_object" ),
	# GL_WEIGHT_ARRAY_BUFFER_BINDING_ARB - not supported
	( "GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB", GLint,
	  ["ctx->Array.ElementArrayBufferObj->Name"], "", "ARB_vertex_buffer_object" ),

	# GL_EXT_pixel_buffer_object
	( "GL_PIXEL_PACK_BUFFER_BINDING_EXT", GLint,
	  ["ctx->Pack.BufferObj->Name"], "", "EXT_pixel_buffer_object" ),
	( "GL_PIXEL_UNPACK_BUFFER_BINDING_EXT", GLint,
	  ["ctx->Unpack.BufferObj->Name"], "", "EXT_pixel_buffer_object" ),

	# GL_ARB_vertex_program
	( "GL_MAX_VERTEX_ATTRIBS_ARB", GLint,
	  ["ctx->Const.MaxVertexProgramAttribs"], "", "ARB_vertex_program" ),

	# GL_ARB_fragment_program
	( "GL_FRAGMENT_PROGRAM_ARB", GLboolean,
	  ["ctx->FragmentProgram.Enabled"], "", "ARB_fragment_program" ),
	( "GL_TRANSPOSE_CURRENT_MATRIX_ARB", GLfloat,
	  ["matrix[0]", "matrix[4]", "matrix[8]", "matrix[12]",
	   "matrix[1]", "matrix[5]", "matrix[9]", "matrix[13]",
	   "matrix[2]", "matrix[6]", "matrix[10]", "matrix[14]",
	   "matrix[3]", "matrix[7]", "matrix[11]", "matrix[15]"],
	  "const GLfloat *matrix = ctx->CurrentStack->Top->m;", "ARB_fragment_program" ),

	# GL_EXT_depth_bounds_test
	( "GL_DEPTH_BOUNDS_TEST_EXT", GLboolean,
	  ["ctx->Depth.BoundsTest"], "", "EXT_depth_bounds_test" ),
	( "GL_DEPTH_BOUNDS_EXT", GLfloat,
	  ["ctx->Depth.BoundsMin", "ctx->Depth.BoundsMax"],
	  "", "EXT_depth_bounds_test" ),

	# GL_MESA_program_debug
	( "GL_FRAGMENT_PROGRAM_CALLBACK_MESA", GLboolean,
	  ["ctx->FragmentProgram.CallbackEnabled"], "", "MESA_program_debug" ),
	( "GL_VERTEX_PROGRAM_CALLBACK_MESA", GLboolean,
	  ["ctx->VertexProgram.CallbackEnabled"], "", "MESA_program_debug" ),
	( "GL_FRAGMENT_PROGRAM_POSITION_MESA", GLint,
	  ["ctx->FragmentProgram.CurrentPosition"], "", "MESA_program_debug" ),
	( "GL_VERTEX_PROGRAM_POSITION_MESA", GLint,
	  ["ctx->VertexProgram.CurrentPosition"], "", "MESA_program_debug" ),

	# GL_ARB_draw_buffers
	( "GL_MAX_DRAW_BUFFERS_ARB", GLint,
	  ["ctx->Const.MaxDrawBuffers"], "", "ARB_draw_buffers" ),
	( "GL_DRAW_BUFFER0_ARB", GLenum,
	  ["ctx->Color.DrawBuffer[0]"], "", "ARB_draw_buffers" ),
	( "GL_DRAW_BUFFER1_ARB", GLenum,
	  ["buffer"],
	  """GLenum buffer;
         if (pname - GL_DRAW_BUFFER0_ARB >= ctx->Const.MaxDrawBuffers) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGet(GL_DRAW_BUFFERx_ARB)");
            return;
         }
         buffer = ctx->Color.DrawBuffer[1];""", "ARB_draw_buffers" ),
	( "GL_DRAW_BUFFER2_ARB", GLenum,
	  ["buffer"],
	  """GLenum buffer;
         if (pname - GL_DRAW_BUFFER0_ARB >= ctx->Const.MaxDrawBuffers) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGet(GL_DRAW_BUFFERx_ARB)");
            return;
         }
         buffer = ctx->Color.DrawBuffer[2];""", "ARB_draw_buffers" ),
	( "GL_DRAW_BUFFER3_ARB", GLenum,
	  ["buffer"],
	  """GLenum buffer;
         if (pname - GL_DRAW_BUFFER0_ARB >= ctx->Const.MaxDrawBuffers) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGet(GL_DRAW_BUFFERx_ARB)");
            return;
         }
         buffer = ctx->Color.DrawBuffer[3];""", "ARB_draw_buffers" ),
	# XXX Add more GL_DRAW_BUFFERn_ARB entries as needed in the future

	# GL_OES_read_format
	( "GL_IMPLEMENTATION_COLOR_READ_TYPE_OES", GLint,
	  ["ctx->Const.ColorReadType"], "", "OES_read_format" ),
	( "GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES", GLint,
	  ["ctx->Const.ColorReadFormat"], "", "OES_read_format" ),

	# GL_ATI_fragment_shader
	( "GL_NUM_FRAGMENT_REGISTERS_ATI", GLint, ["6"], "", "ATI_fragment_shader" ),
	( "GL_NUM_FRAGMENT_CONSTANTS_ATI", GLint, ["8"], "", "ATI_fragment_shader" ),
	( "GL_NUM_PASSES_ATI", GLint, ["2"], "", "ATI_fragment_shader" ),
	( "GL_NUM_INSTRUCTIONS_PER_PASS_ATI", GLint, ["8"], "", "ATI_fragment_shader" ),
	( "GL_NUM_INSTRUCTIONS_TOTAL_ATI", GLint, ["16"], "", "ATI_fragment_shader" ),
	( "GL_COLOR_ALPHA_PAIRING_ATI", GLboolean, ["GL_TRUE"], "", "ATI_fragment_shader" ),
	( "GL_NUM_LOOPBACK_COMPONENTS_ATI", GLint, ["3"], "", "ATI_fragment_shader" ),
	( "GL_NUM_INPUT_INTERPOLATOR_COMPONENTS_ATI", GLint, ["3"], "", "ATI_fragment_shader" ),

	# OpenGL 2.0
	( "GL_STENCIL_BACK_FUNC", GLenum, ["ctx->Stencil.Function[1]"], "", None ),
	( "GL_STENCIL_BACK_VALUE_MASK", GLint, ["ctx->Stencil.ValueMask[1]"], "", None ),
	( "GL_STENCIL_BACK_REF", GLint, ["ctx->Stencil.Ref[1]"], "", None ),
	( "GL_STENCIL_BACK_FAIL", GLenum, ["ctx->Stencil.FailFunc[1]"], "", None ),
	( "GL_STENCIL_BACK_PASS_DEPTH_FAIL", GLenum, ["ctx->Stencil.ZFailFunc[1]"], "", None ),
	( "GL_STENCIL_BACK_PASS_DEPTH_PASS", GLenum, ["ctx->Stencil.ZPassFunc[1]"], "", None ),

	# GL_EXT_framebuffer_object
	( "GL_FRAMEBUFFER_BINDING_EXT", GLint, ["ctx->DrawBuffer->Name"], "",
	  "EXT_framebuffer_object" ),
	( "GL_RENDERBUFFER_BINDING_EXT", GLint,
	  ["ctx->CurrentRenderbuffer ? ctx->CurrentRenderbuffer->Name : 0"], "",
	  "EXT_framebuffer_object" ),
	( "GL_MAX_COLOR_ATTACHMENTS_EXT", GLint,
	  ["ctx->Const.MaxColorAttachments"], "",
	  "EXT_framebuffer_object" ),
	( "GL_MAX_RENDERBUFFER_SIZE_EXT", GLint,
	  ["ctx->Const.MaxRenderbufferSize"], "",
	  "EXT_framebuffer_object" ),

	# GL_ARB_fragment_shader
	( "GL_MAX_FRAGMENT_UNIFORM_COMPONENTS_ARB", GLint, ["MAX_FRAGMENT_UNIFORM_COMPONENTS"], "",
	"ARB_fragment_shader" ),
	( "GL_FRAGMENT_SHADER_DERIVATIVE_HINT_ARB", GLenum, ["ctx->Hint.FragmentShaderDerivative"], "",
	"ARB_fragment_shader" ),

	# GL_ARB_vertex_shader
	( "GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB", GLint, ["MAX_VERTEX_UNIFORM_COMPONENTS"], "",
	"ARB_vertex_shader" ),
	( "GL_MAX_VARYING_FLOATS_ARB", GLint, ["MAX_VARYING_FLOATS"], "", "ARB_vertex_shader" ),
	( "GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS_ARB", GLint, ["MAX_VERTEX_TEXTURE_IMAGE_UNITS"], "",
	"ARB_vertex_shader" ),
	( "GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS_ARB", GLint, ["MAX_COMBINED_TEXTURE_IMAGE_UNITS"], "",
	"ARB_vertex_shader" )
]


def ConversionFunc(fromType, toType):
	"""Return the name of the macro to convert between two data types."""
	if fromType == toType:
		return ""
	elif fromType == GLfloat and toType == GLint:
		return "IROUND"
	elif fromType == GLfloatN and toType == GLfloat:
		return ""
	elif fromType == GLint and toType == GLfloat: # but not GLfloatN!
		return "(GLfloat)"
	else:
		if fromType == GLfloatN:
			fromType = GLfloat
		fromStr = TypeStrings[fromType]
		fromStr = string.upper(fromStr[2:])
		toStr = TypeStrings[toType]
		toStr = string.upper(toStr[2:])
		return fromStr + "_TO_" + toStr


def EmitGetFunction(stateVars, returnType):
	"""Emit the code to implement glGetBooleanv, glGetIntegerv or glGetFloatv."""
	assert (returnType == GLboolean or
			returnType == GLint or
			returnType == GLfloat)

	strType = TypeStrings[returnType]
	# Capitalize first letter of return type
	if returnType == GLint:
		function = "GetIntegerv"
	else:
		function = "Get" + string.upper(strType[2]) + strType[3:] + "v"
	
	print "void GLAPIENTRY"
	print "_mesa_%s( GLenum pname, %s *params )" % (function, TypeStrings[returnType])
	print "{"
	print "   GET_CURRENT_CONTEXT(ctx);"
	print "   ASSERT_OUTSIDE_BEGIN_END(ctx);"
	print ""
	print "   if (!params)"
	print "      return;"
	print ""
	print "   if (ctx->Driver.%s &&" % function
	print "       ctx->Driver.%s(ctx, pname, params))" % function
	print "      return;"
	print ""
	print "   switch (pname) {"

	extCheck = "CHECK_EXTENSION_%s" % string.upper(TypeStrings[returnType][2])

	for (name, varType, state, optionalCode, extension) in stateVars:
		print "      case " + name + ":"
		if extension:
			print "         %s(%s, pname);" % (extCheck, extension)
		if optionalCode:
			print "         {"
			print "         " + optionalCode
		conversion = ConversionFunc(varType, returnType)
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
	print '         _mesa_error(ctx, GL_INVALID_ENUM, "gl%s(pname=0x%%x)", pname);' % function
	print "   }"
	print "}"
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
#include "fbobject.h"
#include "get.h"
#include "macros.h"
#include "mtypes.h"
#include "texcompress.h"


#define FLOAT_TO_BOOLEAN(X)   ( (X) ? GL_TRUE : GL_FALSE )

#define INT_TO_BOOLEAN(I)     ( (I) ? GL_TRUE : GL_FALSE )

#define ENUM_TO_BOOLEAN(E)    ( (E) ? GL_TRUE : GL_FALSE )
#define ENUM_TO_INT(E)        ( (GLint) (E) )
#define ENUM_TO_FLOAT(E)      ( (GLfloat) (E) )

#define BOOLEAN_TO_INT(B)     ( (GLint) (B) )
#define BOOLEAN_TO_FLOAT(B)   ( (B) ? 1.0F : 0.0F )


/* Check if named extension is enabled, if not generate error and return */

#define CHECK1(E1, str, PNAME)                         \\
   if (!ctx->Extensions.E1) {                          \\
      _mesa_error(ctx, GL_INVALID_VALUE,               \\
                  "glGet" str "v(0x%x)", (int) PNAME); \\
      return;                                          \\
   }
    
#define CHECK2(E1, E2, str, PNAME)                     \\
   if (!ctx->Extensions.E1 && !ctx->Extensions.E2) {   \\
      _mesa_error(ctx, GL_INVALID_VALUE,               \\
                  "glGet" str "v(0x%x)", (int) PNAME); \\
      return;                                          \\
   }
    
#define CHECK_EXTENSION_B(EXTNAME, PNAME)      \\
   CHECK1(EXTNAME, "Boolean", PNAME )

#define CHECK_EXTENSION_I(EXTNAME, PNAME)      \\
   CHECK1(EXTNAME, "Integer", PNAME )

#define CHECK_EXTENSION_F(EXTNAME, PNAME)      \\
   CHECK1(EXTNAME, "Float", PNAME )


/**
 * Helper routine.
 */
static GLenum
pixel_texgen_mode(const GLcontext *ctx)
{
   if (ctx->Pixel.FragmentRgbSource == GL_CURRENT_RASTER_POSITION) {
      if (ctx->Pixel.FragmentAlphaSource == GL_CURRENT_RASTER_POSITION) {
         return GL_RGBA;
      }
      else {
         return GL_RGB;
      }
   }
   else {
      if (ctx->Pixel.FragmentAlphaSource == GL_CURRENT_RASTER_POSITION) {
         return GL_ALPHA;
      }
      else {
         return GL_NONE;
      }
   }
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
EmitGetFunction(StateVars, GLboolean)
EmitGetFunction(StateVars, GLfloat)
EmitGetFunction(StateVars, GLint)
EmitGetDoublev()


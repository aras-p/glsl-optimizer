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
GLfixed = 7


TypeStrings = {
	GLint : "GLint",
	GLenum : "GLenum",
	GLfloat : "GLfloat",
	GLdouble : "GLdouble",
	GLboolean : "GLboolean",
	GLfixed : "GLfixed"
}


# Each entry is a tuple of:
#  - the GL state name, such as GL_CURRENT_COLOR
#  - the state datatype, one of GLint, GLfloat, GLboolean or GLenum
#  - list of code fragments to get the state, such as ["ctx->Foo.Bar"]
#  - optional extra code or empty string
#  - optional extensions to check, or None
#

# Present in ES 1.x and 2.x:
StateVars_common = [
	( "GL_ALPHA_BITS", GLint, ["ctx->DrawBuffer->Visual.alphaBits"],
	  "", None ),
	( "GL_BLEND", GLboolean, ["ctx->Color.BlendEnabled"], "", None ),
	( "GL_BLEND_SRC", GLenum, ["ctx->Color.BlendSrcRGB"], "", None ),
	( "GL_BLUE_BITS", GLint, ["ctx->DrawBuffer->Visual.blueBits"], "", None ),
	( "GL_COLOR_CLEAR_VALUE", GLfloatN,
	  [ "ctx->Color.ClearColor[0]",
		"ctx->Color.ClearColor[1]",
		"ctx->Color.ClearColor[2]",
		"ctx->Color.ClearColor[3]" ], "", None ),
	( "GL_COLOR_WRITEMASK", GLint,
	  [ "ctx->Color.ColorMask[RCOMP] ? 1 : 0",
		"ctx->Color.ColorMask[GCOMP] ? 1 : 0",
		"ctx->Color.ColorMask[BCOMP] ? 1 : 0",
		"ctx->Color.ColorMask[ACOMP] ? 1 : 0" ], "", None ),
	( "GL_CULL_FACE", GLboolean, ["ctx->Polygon.CullFlag"], "", None ),
	( "GL_CULL_FACE_MODE", GLenum, ["ctx->Polygon.CullFaceMode"], "", None ),
	( "GL_DEPTH_BITS", GLint, ["ctx->DrawBuffer->Visual.depthBits"],
	  "", None ),
	( "GL_DEPTH_CLEAR_VALUE", GLfloatN, ["ctx->Depth.Clear"], "", None ),
	( "GL_DEPTH_FUNC", GLenum, ["ctx->Depth.Func"], "", None ),
	( "GL_DEPTH_RANGE", GLfloatN,
	  [ "ctx->Viewport.Near", "ctx->Viewport.Far" ], "", None ),
	( "GL_DEPTH_TEST", GLboolean, ["ctx->Depth.Test"], "", None ),
	( "GL_DEPTH_WRITEMASK", GLboolean, ["ctx->Depth.Mask"], "", None ),
	( "GL_DITHER", GLboolean, ["ctx->Color.DitherFlag"], "", None ),
	( "GL_FRONT_FACE", GLenum, ["ctx->Polygon.FrontFace"], "", None ),
	( "GL_GREEN_BITS", GLint, ["ctx->DrawBuffer->Visual.greenBits"],
	  "", None ),
	( "GL_LINE_WIDTH", GLfloat, ["ctx->Line.Width"], "", None ),
	( "GL_ALIASED_LINE_WIDTH_RANGE", GLfloat,
	  ["ctx->Const.MinLineWidth",
	   "ctx->Const.MaxLineWidth"], "", None ),
	( "GL_MAX_ELEMENTS_INDICES", GLint, ["ctx->Const.MaxArrayLockSize"], "", None ),
	( "GL_MAX_ELEMENTS_VERTICES", GLint, ["ctx->Const.MaxArrayLockSize"], "", None ),

	( "GL_MAX_TEXTURE_SIZE", GLint, ["1 << (ctx->Const.MaxTextureLevels - 1)"], "", None ),
	( "GL_MAX_VIEWPORT_DIMS", GLint,
	  ["ctx->Const.MaxViewportWidth", "ctx->Const.MaxViewportHeight"],
	  "", None ),
	( "GL_PACK_ALIGNMENT", GLint, ["ctx->Pack.Alignment"], "", None ),
	( "GL_ALIASED_POINT_SIZE_RANGE", GLfloat,
	  ["ctx->Const.MinPointSize",
	   "ctx->Const.MaxPointSize"], "", None ),
	( "GL_POLYGON_OFFSET_FACTOR", GLfloat, ["ctx->Polygon.OffsetFactor "], "", None ),
	( "GL_POLYGON_OFFSET_UNITS", GLfloat, ["ctx->Polygon.OffsetUnits "], "", None ),
	( "GL_RED_BITS", GLint, ["ctx->DrawBuffer->Visual.redBits"], "", None ),
	( "GL_SCISSOR_BOX", GLint,
	  ["ctx->Scissor.X",
	   "ctx->Scissor.Y",
	   "ctx->Scissor.Width",
	   "ctx->Scissor.Height"], "", None ),
	( "GL_SCISSOR_TEST", GLboolean, ["ctx->Scissor.Enabled"], "", None ),
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
	( "GL_SUBPIXEL_BITS", GLint, ["ctx->Const.SubPixelBits"], "", None ),
	( "GL_TEXTURE_BINDING_2D", GLint,
	  ["ctx->Texture.Unit[ctx->Texture.CurrentUnit].CurrentTex[TEXTURE_2D_INDEX]->Name"], "", None ),
	( "GL_UNPACK_ALIGNMENT", GLint, ["ctx->Unpack.Alignment"], "", None ),
	( "GL_VIEWPORT", GLint, [ "ctx->Viewport.X", "ctx->Viewport.Y",
	  "ctx->Viewport.Width", "ctx->Viewport.Height" ], "", None ),

	# GL_ARB_multitexture
	( "GL_ACTIVE_TEXTURE_ARB", GLint,
	  [ "GL_TEXTURE0_ARB + ctx->Texture.CurrentUnit"], "", ["ARB_multitexture"] ),

        # Note that all the OES_* extensions require that the Mesa
        # "struct gl_extensions" include a member with the name of
        # the extension.  That structure does not yet include OES
        # extensions (and we're not sure whether it will).  If
        # it does, all the OES_* extensions below should mark the
        # dependency.

	# OES_texture_cube_map
	( "GL_TEXTURE_BINDING_CUBE_MAP_ARB", GLint,
	  ["ctx->Texture.Unit[ctx->Texture.CurrentUnit].CurrentTex[TEXTURE_CUBE_INDEX]->Name"],
	  "", None),
	( "GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB", GLint,
	  ["(1 << (ctx->Const.MaxCubeTextureLevels - 1))"],
	  "", None),

        # OES_blend_subtract
	( "GL_BLEND_SRC_RGB_EXT", GLenum, ["ctx->Color.BlendSrcRGB"], "", None),
	( "GL_BLEND_DST_RGB_EXT", GLenum, ["ctx->Color.BlendDstRGB"], "", None),
	( "GL_BLEND_SRC_ALPHA_EXT", GLenum, ["ctx->Color.BlendSrcA"], "", None),
	( "GL_BLEND_DST_ALPHA_EXT", GLenum, ["ctx->Color.BlendDstA"], "", None),

        # GL_BLEND_EQUATION_RGB, which is what we're really after,
        # is defined identically to GL_BLEND_EQUATION.
	( "GL_BLEND_EQUATION", GLenum, ["ctx->Color.BlendEquationRGB "], "", None),
	( "GL_BLEND_EQUATION_ALPHA_EXT", GLenum, ["ctx->Color.BlendEquationA "],
	  "", None),

	# GL_ARB_texture_compression */
#	( "GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB", GLint,
#	  ["_mesa_get_compressed_formats(ctx, NULL, GL_FALSE)"],
#	  "", ["ARB_texture_compression"] ),
#	( "GL_COMPRESSED_TEXTURE_FORMATS_ARB", GLenum,
#	  [],
#	  """GLint formats[100];
#         GLuint i, n = _mesa_get_compressed_formats(ctx, formats, GL_FALSE);
#         ASSERT(n <= 100);
#         for (i = 0; i < n; i++)
#            params[i] = ENUM_TO_INT(formats[i]);""",
#	  ["ARB_texture_compression"] ),

	# GL_ARB_multisample
	( "GL_SAMPLE_ALPHA_TO_COVERAGE_ARB", GLboolean,
	  ["ctx->Multisample.SampleAlphaToCoverage"], "", ["ARB_multisample"] ),
	( "GL_SAMPLE_COVERAGE_ARB", GLboolean,
	  ["ctx->Multisample.SampleCoverage"], "", ["ARB_multisample"] ),
	( "GL_SAMPLE_COVERAGE_VALUE_ARB", GLfloat,
	  ["ctx->Multisample.SampleCoverageValue"], "", ["ARB_multisample"] ),
	( "GL_SAMPLE_COVERAGE_INVERT_ARB", GLboolean,
	  ["ctx->Multisample.SampleCoverageInvert"], "", ["ARB_multisample"] ),
	( "GL_SAMPLE_BUFFERS_ARB", GLint,
	  ["ctx->DrawBuffer->Visual.sampleBuffers"], "", ["ARB_multisample"] ),
	( "GL_SAMPLES_ARB", GLint,
	  ["ctx->DrawBuffer->Visual.samples"], "", ["ARB_multisample"] ),


	# GL_SGIS_generate_mipmap
	( "GL_GENERATE_MIPMAP_HINT_SGIS", GLenum, ["ctx->Hint.GenerateMipmap"],
	  "", ["SGIS_generate_mipmap"] ),

	# GL_ARB_vertex_buffer_object
	( "GL_ARRAY_BUFFER_BINDING_ARB", GLint,
	  ["ctx->Array.ArrayBufferObj->Name"], "", ["ARB_vertex_buffer_object"] ),
	# GL_WEIGHT_ARRAY_BUFFER_BINDING_ARB - not supported
	( "GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB", GLint,
	  ["ctx->Array.ElementArrayBufferObj->Name"],
	  "", ["ARB_vertex_buffer_object"] ),

	# GL_OES_read_format
	( "GL_IMPLEMENTATION_COLOR_READ_TYPE_OES", GLint,
	  ["_mesa_get_color_read_type(ctx)"], "", ["OES_read_format"] ),
	( "GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES", GLint,
	  ["_mesa_get_color_read_format(ctx)"], "", ["OES_read_format"] ),

	# GL_OES_framebuffer_object
	( "GL_FRAMEBUFFER_BINDING_EXT", GLint, ["ctx->DrawBuffer->Name"], "",
	  None),
	( "GL_RENDERBUFFER_BINDING_EXT", GLint,
	  ["ctx->CurrentRenderbuffer ? ctx->CurrentRenderbuffer->Name : 0"], "",
	  None),
	( "GL_MAX_RENDERBUFFER_SIZE_EXT", GLint,
	  ["ctx->Const.MaxRenderbufferSize"], "",
	  None),

	# OpenGL ES 1/2 special:
	( "GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB", GLint,
	  [ "ARRAY_SIZE(compressed_formats)" ],
	  "",
	  None ),

	("GL_COMPRESSED_TEXTURE_FORMATS_ARB", GLint,
	 [],
	 """
     int i;
     for (i = 0; i < ARRAY_SIZE(compressed_formats); i++) {
        params[i] = compressed_formats[i];
     }""",
	 None ),

	( "GL_POLYGON_OFFSET_FILL", GLboolean, ["ctx->Polygon.OffsetFill"], "", None ),

]

# Only present in ES 1.x:
StateVars_es1 = [
	( "GL_MAX_LIGHTS", GLint, ["ctx->Const.MaxLights"], "", None ),
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
	( "GL_LIGHT_MODEL_TWO_SIDE", GLboolean, ["ctx->Light.Model.TwoSide"], "", None ),
	( "GL_ALPHA_TEST", GLboolean, ["ctx->Color.AlphaEnabled"], "", None ),
	( "GL_ALPHA_TEST_FUNC", GLenum, ["ctx->Color.AlphaFunc"], "", None ),
	( "GL_ALPHA_TEST_REF", GLfloatN, ["ctx->Color.AlphaRef"], "", None ),
	( "GL_BLEND_DST", GLenum, ["ctx->Color.BlendDstRGB"], "", None ),
	( "GL_MAX_CLIP_PLANES", GLint, ["ctx->Const.MaxClipPlanes"], "", None ),
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
	( "GL_COLOR_MATERIAL", GLboolean,
	  ["ctx->Light.ColorMaterialEnabled"], "", None ),
	( "GL_CURRENT_COLOR", GLfloatN,
	  [ "ctx->Current.Attrib[VERT_ATTRIB_COLOR0][0]",
		"ctx->Current.Attrib[VERT_ATTRIB_COLOR0][1]",
		"ctx->Current.Attrib[VERT_ATTRIB_COLOR0][2]",
		"ctx->Current.Attrib[VERT_ATTRIB_COLOR0][3]" ],
	  "FLUSH_CURRENT(ctx, 0);", None ),
	( "GL_CURRENT_NORMAL", GLfloatN,
	  [ "ctx->Current.Attrib[VERT_ATTRIB_NORMAL][0]",
		"ctx->Current.Attrib[VERT_ATTRIB_NORMAL][1]",
		"ctx->Current.Attrib[VERT_ATTRIB_NORMAL][2]"],
	  "FLUSH_CURRENT(ctx, 0);", None ),
	( "GL_CURRENT_TEXTURE_COORDS", GLfloat,
	  ["ctx->Current.Attrib[VERT_ATTRIB_TEX0 + texUnit][0]",
	   "ctx->Current.Attrib[VERT_ATTRIB_TEX0 + texUnit][1]",
	   "ctx->Current.Attrib[VERT_ATTRIB_TEX0 + texUnit][2]",
	   "ctx->Current.Attrib[VERT_ATTRIB_TEX0 + texUnit][3]"],
	  "const GLuint texUnit = ctx->Texture.CurrentUnit;", None ),
	( "GL_DISTANCE_ATTENUATION_EXT", GLfloat,
	  ["ctx->Point.Params[0]",
	   "ctx->Point.Params[1]",
	   "ctx->Point.Params[2]"], "", None ),
	( "GL_FOG", GLboolean, ["ctx->Fog.Enabled"], "", None ),
	( "GL_FOG_COLOR", GLfloatN,
	  [ "ctx->Fog.Color[0]",
		"ctx->Fog.Color[1]",
		"ctx->Fog.Color[2]",
		"ctx->Fog.Color[3]" ], "", None ),
	( "GL_FOG_DENSITY", GLfloat, ["ctx->Fog.Density"], "", None ),
	( "GL_FOG_END", GLfloat, ["ctx->Fog.End"], "", None ),
	( "GL_FOG_HINT", GLenum, ["ctx->Hint.Fog"], "", None ),
	( "GL_FOG_MODE", GLenum, ["ctx->Fog.Mode"], "", None ),
	( "GL_FOG_START", GLfloat, ["ctx->Fog.Start"], "", None ),
	( "GL_LINE_SMOOTH", GLboolean, ["ctx->Line.SmoothFlag"], "", None ),
	( "GL_LINE_SMOOTH_HINT", GLenum, ["ctx->Hint.LineSmooth"], "", None ),
	( "GL_LINE_WIDTH_RANGE", GLfloat,
	  ["ctx->Const.MinLineWidthAA",
	   "ctx->Const.MaxLineWidthAA"], "", None ),
	( "GL_COLOR_LOGIC_OP", GLboolean, ["ctx->Color.ColorLogicOpEnabled"], "", None ),
	( "GL_LOGIC_OP_MODE", GLenum, ["ctx->Color.LogicOp"], "", None ),
	( "GL_MATRIX_MODE", GLenum, ["ctx->Transform.MatrixMode"], "", None ),

	( "GL_MAX_MODELVIEW_STACK_DEPTH", GLint, ["MAX_MODELVIEW_STACK_DEPTH"], "", None ),
	( "GL_MAX_PROJECTION_STACK_DEPTH", GLint, ["MAX_PROJECTION_STACK_DEPTH"], "", None ),
	( "GL_MAX_TEXTURE_STACK_DEPTH", GLint, ["MAX_TEXTURE_STACK_DEPTH"], "", None ),
	( "GL_MODELVIEW_MATRIX", GLfloat,
	  [ "matrix[0]", "matrix[1]", "matrix[2]", "matrix[3]",
		"matrix[4]", "matrix[5]", "matrix[6]", "matrix[7]",
		"matrix[8]", "matrix[9]", "matrix[10]", "matrix[11]",
		"matrix[12]", "matrix[13]", "matrix[14]", "matrix[15]" ],
	  "const GLfloat *matrix = ctx->ModelviewMatrixStack.Top->m;", None ),
	( "GL_MODELVIEW_STACK_DEPTH", GLint, ["ctx->ModelviewMatrixStack.Depth + 1"], "", None ),
	( "GL_NORMALIZE", GLboolean, ["ctx->Transform.Normalize"], "", None ),
	( "GL_PACK_SKIP_IMAGES_EXT", GLint, ["ctx->Pack.SkipImages"], "", None ),
	( "GL_PERSPECTIVE_CORRECTION_HINT", GLenum,
	  ["ctx->Hint.PerspectiveCorrection"], "", None ),
	( "GL_POINT_SIZE", GLfloat, ["ctx->Point.Size"], "", None ),
	( "GL_POINT_SIZE_RANGE", GLfloat,
	  ["ctx->Const.MinPointSizeAA",
	   "ctx->Const.MaxPointSizeAA"], "", None ),
	( "GL_POINT_SMOOTH", GLboolean, ["ctx->Point.SmoothFlag"], "", None ),
	( "GL_POINT_SMOOTH_HINT", GLenum, ["ctx->Hint.PointSmooth"], "", None ),
	( "GL_POINT_SIZE_MIN_EXT", GLfloat, ["ctx->Point.MinSize"], "", None ),
	( "GL_POINT_SIZE_MAX_EXT", GLfloat, ["ctx->Point.MaxSize"], "", None ),
	( "GL_POINT_FADE_THRESHOLD_SIZE_EXT", GLfloat,
	  ["ctx->Point.Threshold"], "", None ),
	( "GL_PROJECTION_MATRIX", GLfloat,
	  [ "matrix[0]", "matrix[1]", "matrix[2]", "matrix[3]",
		"matrix[4]", "matrix[5]", "matrix[6]", "matrix[7]",
		"matrix[8]", "matrix[9]", "matrix[10]", "matrix[11]",
		"matrix[12]", "matrix[13]", "matrix[14]", "matrix[15]" ],
	  "const GLfloat *matrix = ctx->ProjectionMatrixStack.Top->m;", None ),
	( "GL_PROJECTION_STACK_DEPTH", GLint,
	  ["ctx->ProjectionMatrixStack.Depth + 1"], "", None ),
	( "GL_RESCALE_NORMAL", GLboolean,
	  ["ctx->Transform.RescaleNormals"], "", None ),
	( "GL_SHADE_MODEL", GLenum, ["ctx->Light.ShadeModel"], "", None ),
	( "GL_TEXTURE_2D", GLboolean, ["_mesa_IsEnabled(GL_TEXTURE_2D)"], "", None ),
	( "GL_TEXTURE_MATRIX", GLfloat,
	  ["matrix[0]", "matrix[1]", "matrix[2]", "matrix[3]",
	   "matrix[4]", "matrix[5]", "matrix[6]", "matrix[7]",
	   "matrix[8]", "matrix[9]", "matrix[10]", "matrix[11]",
	   "matrix[12]", "matrix[13]", "matrix[14]", "matrix[15]" ],
	  "const GLfloat *matrix = ctx->TextureMatrixStack[ctx->Texture.CurrentUnit].Top->m;", None ),
	( "GL_TEXTURE_STACK_DEPTH", GLint,
	  ["ctx->TextureMatrixStack[ctx->Texture.CurrentUnit].Depth + 1"], "", None ),
	( "GL_VERTEX_ARRAY", GLboolean, ["ctx->Array.ArrayObj->Vertex.Enabled"], "", None ),
	( "GL_VERTEX_ARRAY_SIZE", GLint, ["ctx->Array.ArrayObj->Vertex.Size"], "", None ),
	( "GL_VERTEX_ARRAY_TYPE", GLenum, ["ctx->Array.ArrayObj->Vertex.Type"], "", None ),
	( "GL_VERTEX_ARRAY_STRIDE", GLint, ["ctx->Array.ArrayObj->Vertex.Stride"], "", None ),
	( "GL_NORMAL_ARRAY", GLenum, ["ctx->Array.ArrayObj->Normal.Enabled"], "", None ),
	( "GL_NORMAL_ARRAY_TYPE", GLenum, ["ctx->Array.ArrayObj->Normal.Type"], "", None ),
	( "GL_NORMAL_ARRAY_STRIDE", GLint, ["ctx->Array.ArrayObj->Normal.Stride"], "", None ),
	( "GL_COLOR_ARRAY", GLboolean, ["ctx->Array.ArrayObj->Color.Enabled"], "", None ),
	( "GL_COLOR_ARRAY_SIZE", GLint, ["ctx->Array.ArrayObj->Color.Size"], "", None ),
	( "GL_COLOR_ARRAY_TYPE", GLenum, ["ctx->Array.ArrayObj->Color.Type"], "", None ),
	( "GL_COLOR_ARRAY_STRIDE", GLint, ["ctx->Array.ArrayObj->Color.Stride"], "", None ),
	( "GL_TEXTURE_COORD_ARRAY", GLboolean,
	  ["ctx->Array.ArrayObj->TexCoord[ctx->Array.ActiveTexture].Enabled"], "", None ),
	( "GL_TEXTURE_COORD_ARRAY_SIZE", GLint,
	  ["ctx->Array.ArrayObj->TexCoord[ctx->Array.ActiveTexture].Size"], "", None ),
	( "GL_TEXTURE_COORD_ARRAY_TYPE", GLenum,
	  ["ctx->Array.ArrayObj->TexCoord[ctx->Array.ActiveTexture].Type"], "", None ),
	( "GL_TEXTURE_COORD_ARRAY_STRIDE", GLint,
	  ["ctx->Array.ArrayObj->TexCoord[ctx->Array.ActiveTexture].Stride"], "", None ),
	# GL_ARB_multitexture
	( "GL_MAX_TEXTURE_UNITS_ARB", GLint,
	  ["ctx->Const.MaxTextureUnits"], "", ["ARB_multitexture"] ),
	( "GL_CLIENT_ACTIVE_TEXTURE_ARB", GLint,
	  ["GL_TEXTURE0_ARB + ctx->Array.ActiveTexture"], "", ["ARB_multitexture"] ),
        # OES_texture_cube_map
	( "GL_TEXTURE_CUBE_MAP_ARB", GLboolean,
	  ["_mesa_IsEnabled(GL_TEXTURE_CUBE_MAP_ARB)"], "", None),
	( "GL_TEXTURE_GEN_STR_OES", GLboolean,
          # S, T, and R are always set at the same time
	  ["((ctx->Texture.Unit[ctx->Texture.CurrentUnit].TexGenEnabled & S_BIT) ? 1 : 0)"], "", None),
        # ARB_multisample
	( "GL_MULTISAMPLE_ARB", GLboolean,
	  ["ctx->Multisample.Enabled"], "", ["ARB_multisample"] ),
	( "GL_SAMPLE_ALPHA_TO_ONE_ARB", GLboolean,
	  ["ctx->Multisample.SampleAlphaToOne"], "", ["ARB_multisample"] ),

	( "GL_VERTEX_ARRAY_BUFFER_BINDING_ARB", GLint,
	  ["ctx->Array.ArrayObj->Vertex.BufferObj->Name"], "", ["ARB_vertex_buffer_object"] ),
	( "GL_NORMAL_ARRAY_BUFFER_BINDING_ARB", GLint,
	  ["ctx->Array.ArrayObj->Normal.BufferObj->Name"], "", ["ARB_vertex_buffer_object"] ),
	( "GL_COLOR_ARRAY_BUFFER_BINDING_ARB", GLint,
	  ["ctx->Array.ArrayObj->Color.BufferObj->Name"], "", ["ARB_vertex_buffer_object"] ),
	( "GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING_ARB", GLint,
	  ["ctx->Array.ArrayObj->TexCoord[ctx->Array.ActiveTexture].BufferObj->Name"],
	  "", ["ARB_vertex_buffer_object"] ),

	# OES_point_sprite
	( "GL_POINT_SPRITE_NV", GLboolean, ["ctx->Point.PointSprite"], # == GL_POINT_SPRITE_ARB
	  "", None),

	# GL_ARB_fragment_shader
	( "GL_MAX_FRAGMENT_UNIFORM_COMPONENTS_ARB", GLint,
	  ["ctx->Const.FragmentProgram.MaxUniformComponents"], "",
	  ["ARB_fragment_shader"] ),

        # GL_ARB_vertex_shader
	( "GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB", GLint,
	  ["ctx->Const.VertexProgram.MaxUniformComponents"], "",
	  ["ARB_vertex_shader"] ),
	( "GL_MAX_VARYING_FLOATS_ARB", GLint,
	  ["ctx->Const.MaxVarying * 4"], "", ["ARB_vertex_shader"] ),

        # OES_matrix_get
	( "GL_MODELVIEW_MATRIX_FLOAT_AS_INT_BITS_OES", GLint, [],
	  """
      /* See GL_OES_matrix_get */
      {
         const GLfloat *matrix = ctx->ModelviewMatrixStack.Top->m;
         memcpy(params, matrix, 16 * sizeof(GLint));
      }""",
	  None),

	( "GL_PROJECTION_MATRIX_FLOAT_AS_INT_BITS_OES", GLint, [],
	  """
      /* See GL_OES_matrix_get */
      {
         const GLfloat *matrix = ctx->ProjectionMatrixStack.Top->m;
         memcpy(params, matrix, 16 * sizeof(GLint));
      }""",
	  None),

	( "GL_TEXTURE_MATRIX_FLOAT_AS_INT_BITS_OES", GLint, [],
	  """
      /* See GL_OES_matrix_get */
      {
         const GLfloat *matrix =
            ctx->TextureMatrixStack[ctx->Texture.CurrentUnit].Top->m;
         memcpy(params, matrix, 16 * sizeof(GLint));
      }""",
	  None),

        # OES_point_size_array
	("GL_POINT_SIZE_ARRAY_OES", GLboolean,
	 ["ctx->Array.ArrayObj->PointSize.Enabled"], "", None),
	("GL_POINT_SIZE_ARRAY_TYPE_OES", GLenum,
      ["ctx->Array.ArrayObj->PointSize.Type"], "", None),
	("GL_POINT_SIZE_ARRAY_STRIDE_OES", GLint,
	 ["ctx->Array.ArrayObj->PointSize.Stride"], "", None),
	("GL_POINT_SIZE_ARRAY_BUFFER_BINDING_OES", GLint,
	 ["ctx->Array.ArrayObj->PointSize.BufferObj->Name"], "", None),

	# GL_EXT_texture_lod_bias
	( "GL_MAX_TEXTURE_LOD_BIAS_EXT", GLfloat,
	  ["ctx->Const.MaxTextureLodBias"], "", ["EXT_texture_lod_bias"]),

	# GL_EXT_texture_filter_anisotropic
	( "GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT", GLfloat,
	  ["ctx->Const.MaxTextureMaxAnisotropy"], "", ["EXT_texture_filter_anisotropic"]),

]

# Only present in ES 2.x:
StateVars_es2 = [
	# XXX These entries are not spec'ed for GLES 2, but are
        #     needed for Mesa's GLSL:
	( "GL_MAX_LIGHTS", GLint, ["ctx->Const.MaxLights"], "", None ),
	( "GL_MAX_CLIP_PLANES", GLint, ["ctx->Const.MaxClipPlanes"], "", None ),
        ( "GL_MAX_TEXTURE_COORDS_ARB", GLint, # == GL_MAX_TEXTURE_COORDS_NV
          ["ctx->Const.MaxTextureCoordUnits"], "",
          ["ARB_fragment_program", "NV_fragment_program"] ),
	( "GL_MAX_DRAW_BUFFERS_ARB", GLint,
	  ["ctx->Const.MaxDrawBuffers"], "", ["ARB_draw_buffers"] ),
	( "GL_BLEND_COLOR_EXT", GLfloatN,
	  [ "ctx->Color.BlendColor[0]",
		"ctx->Color.BlendColor[1]",
		"ctx->Color.BlendColor[2]",
		"ctx->Color.BlendColor[3]"], "", None ),

        # This is required for GLES2, but also needed for GLSL:
        ( "GL_MAX_TEXTURE_IMAGE_UNITS_ARB", GLint, # == GL_MAX_TEXTURE_IMAGE_UNI
          ["ctx->Const.MaxTextureImageUnits"], "",
          ["ARB_fragment_program", "NV_fragment_program"] ),

	( "GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS_ARB", GLint,
	  ["ctx->Const.MaxVertexTextureImageUnits"], "", ["ARB_vertex_shader"] ),
	( "GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS_ARB", GLint,
	  ["MAX_COMBINED_TEXTURE_IMAGE_UNITS"], "", ["ARB_vertex_shader"] ),

	# GL_ARB_shader_objects
	# Actually, this token isn't part of GL_ARB_shader_objects, but is
	# close enough for now.
	( "GL_CURRENT_PROGRAM", GLint,
	  ["ctx->Shader.CurrentProgram ? ctx->Shader.CurrentProgram->Name : 0"],
	  "", ["ARB_shader_objects"] ),

	# OpenGL 2.0
	( "GL_STENCIL_BACK_FUNC", GLenum, ["ctx->Stencil.Function[1]"], "", None ),
	( "GL_STENCIL_BACK_VALUE_MASK", GLint, ["ctx->Stencil.ValueMask[1]"], "", None ),
	( "GL_STENCIL_BACK_WRITEMASK", GLint, ["ctx->Stencil.WriteMask[1]"], "", None ),
	( "GL_STENCIL_BACK_REF", GLint, ["ctx->Stencil.Ref[1]"], "", None ),
	( "GL_STENCIL_BACK_FAIL", GLenum, ["ctx->Stencil.FailFunc[1]"], "", None ),
	( "GL_STENCIL_BACK_PASS_DEPTH_FAIL", GLenum, ["ctx->Stencil.ZFailFunc[1]"], "", None ),
	( "GL_STENCIL_BACK_PASS_DEPTH_PASS", GLenum, ["ctx->Stencil.ZPassFunc[1]"], "", None ),

	( "GL_MAX_VERTEX_ATTRIBS_ARB", GLint,
	  ["ctx->Const.VertexProgram.MaxAttribs"], "", ["ARB_vertex_program"] ),

        # OES_texture_3D 
	( "GL_TEXTURE_BINDING_3D", GLint,
	  ["ctx->Texture.Unit[ctx->Texture.CurrentUnit].CurrentTex[TEXTURE_3D_INDEX]->Name"], "", None),
	( "GL_MAX_3D_TEXTURE_SIZE", GLint, ["1 << (ctx->Const.Max3DTextureLevels - 1)"], "", None),

        # OES_standard_derivatives
	( "GL_FRAGMENT_SHADER_DERIVATIVE_HINT_ARB", GLenum,
	  ["ctx->Hint.FragmentShaderDerivative"], "", ["ARB_fragment_shader"] ),

	# Unique to ES 2 (not in full GL)
	( "GL_MAX_FRAGMENT_UNIFORM_VECTORS", GLint,
	  ["ctx->Const.FragmentProgram.MaxUniformComponents / 4"], "", None),
	( "GL_MAX_VARYING_VECTORS", GLint,
	  ["ctx->Const.MaxVarying"], "", None),
	( "GL_MAX_VERTEX_UNIFORM_VECTORS", GLint,
	  ["ctx->Const.VertexProgram.MaxUniformComponents / 4"], "", None),
	( "GL_SHADER_COMPILER", GLint, ["1"], "", None),
        # OES_get_program_binary
	( "GL_NUM_SHADER_BINARY_FORMATS", GLint, ["0"], "", None),
	( "GL_SHADER_BINARY_FORMATS", GLint, [], "", None),
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
			returnType == GLfloat or
			returnType == GLfixed)

	strType = TypeStrings[returnType]
	# Capitalize first letter of return type
	if returnType == GLint:
		function = "_mesa_GetIntegerv"
	elif returnType == GLboolean:
		function = "_mesa_GetBooleanv"
	elif returnType == GLfloat:
		function = "_mesa_GetFloatv"
	elif returnType == GLfixed:
		function = "_mesa_GetFixedv"
	else:
		abort()

	print "void GLAPIENTRY"
	print "%s( GLenum pname, %s *params )" % (function, strType)
	print "{"
	print "   GET_CURRENT_CONTEXT(ctx);"
	print "   ASSERT_OUTSIDE_BEGIN_END(ctx);"
	print ""
	print "   if (!params)"
	print "      return;"
	print ""
	print "   if (ctx->NewState)"
	print "      _mesa_update_state(ctx);"
	print ""
	print "   switch (pname) {"

	for (name, varType, state, optionalCode, extensions) in stateVars:
		print "      case " + name + ":"
		if extensions:
			if len(extensions) == 1:
				print ('         CHECK_EXT1(%s, "%s");' %
					   (extensions[0], function))
			elif len(extensions) == 2:
				print ('         CHECK_EXT2(%s, %s, "%s");' %
					   (extensions[0], extensions[1], function))
			elif len(extensions) == 3:
				print ('         CHECK_EXT3(%s, %s, %s, "%s");' %
					   (extensions[0], extensions[1], extensions[2], function))
			else:
				assert len(extensions) == 4
				print ('         CHECK_EXT4(%s, %s, %s, %s, "%s");' %
					   (extensions[0], extensions[1], extensions[2], extensions[3], function))
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

#include "main/glheader.h"
#include "main/context.h"
#include "main/enable.h"
#include "main/extensions.h"
#include "main/fbobject.h"
#include "main/get.h"
#include "main/macros.h"
#include "main/mtypes.h"
#include "main/state.h"
#include "main/texcompress.h"
#include "main/framebuffer.h"


/* ES1 tokens that should be in gl.h but aren't */
#define GL_MAX_ELEMENTS_INDICES             0x80E9
#define GL_MAX_ELEMENTS_VERTICES            0x80E8


/* ES2 special tokens */
#define GL_MAX_FRAGMENT_UNIFORM_VECTORS     0x8DFD
#define GL_MAX_VARYING_VECTORS              0x8DFC
#define GL_MAX_VARYING_VECTORS              0x8DFC
#define GL_MAX_VERTEX_UNIFORM_VECTORS       0x8DFB
#define GL_SHADER_COMPILER                  0x8DFA
#define GL_PLATFORM_BINARY                  0x8D63
#define GL_SHADER_BINARY_FORMATS            0x8DF8
#define GL_NUM_SHADER_BINARY_FORMATS        0x8DF9


#ifndef GL_OES_matrix_get
#define GL_MODELVIEW_MATRIX_FLOAT_AS_INT_BITS_OES               0x898D
#define GL_PROJECTION_MATRIX_FLOAT_AS_INT_BITS_OES              0x898E
#define GL_TEXTURE_MATRIX_FLOAT_AS_INT_BITS_OES                 0x898F
#endif

#ifndef GL_OES_compressed_paletted_texture
#define GL_PALETTE4_RGB8_OES                                    0x8B90
#define GL_PALETTE4_RGBA8_OES                                   0x8B91
#define GL_PALETTE4_R5_G6_B5_OES                                0x8B92
#define GL_PALETTE4_RGBA4_OES                                   0x8B93
#define GL_PALETTE4_RGB5_A1_OES                                 0x8B94
#define GL_PALETTE8_RGB8_OES                                    0x8B95
#define GL_PALETTE8_RGBA8_OES                                   0x8B96
#define GL_PALETTE8_R5_G6_B5_OES                                0x8B97
#define GL_PALETTE8_RGBA4_OES                                   0x8B98
#define GL_PALETTE8_RGB5_A1_OES                                 0x8B99
#endif

/* GL_OES_texture_cube_map */
#ifndef GL_OES_texture_cube_map
#define GL_TEXTURE_GEN_STR_OES                                  0x8D60
#endif

#define FLOAT_TO_BOOLEAN(X)   ( (X) ? GL_TRUE : GL_FALSE )
#define FLOAT_TO_FIXED(F)     ( ((F) * 65536.0f > INT_MAX) ? INT_MAX : \\
                                ((F) * 65536.0f < INT_MIN) ? INT_MIN : \\
                                (GLint) ((F) * 65536.0f) )

#define INT_TO_BOOLEAN(I)     ( (I) ? GL_TRUE : GL_FALSE )
#define INT_TO_FIXED(I)       ( ((I) > SHRT_MAX) ? INT_MAX : \\
                                ((I) < SHRT_MIN) ? INT_MIN : \\
                                (GLint) ((I) * 65536) )

#define BOOLEAN_TO_INT(B)     ( (GLint) (B) )
#define BOOLEAN_TO_FLOAT(B)   ( (B) ? 1.0F : 0.0F )
#define BOOLEAN_TO_FIXED(B)   ( (GLint) ((B) ? 1 : 0) << 16 )

#define ENUM_TO_FIXED(E)      (E)


/*
 * Check if named extension is enabled, if not generate error and return.
 */
#define CHECK_EXT1(EXT1, FUNC)                                         \\
   if (!ctx->Extensions.EXT1) {                                        \\
      _mesa_error(ctx, GL_INVALID_ENUM, FUNC "(0x%x)", (int) pname);  \\
      return;                                                          \\
   }

/*
 * Check if either of two extensions is enabled.
 */
#define CHECK_EXT2(EXT1, EXT2, FUNC)                                   \\
   if (!ctx->Extensions.EXT1 && !ctx->Extensions.EXT2) {               \\
      _mesa_error(ctx, GL_INVALID_ENUM, FUNC "(0x%x)", (int) pname);  \\
      return;                                                          \\
   }

/*
 * Check if either of three extensions is enabled.
 */
#define CHECK_EXT3(EXT1, EXT2, EXT3, FUNC)                             \\
   if (!ctx->Extensions.EXT1 && !ctx->Extensions.EXT2 &&               \\
       !ctx->Extensions.EXT3) {                                        \\
      _mesa_error(ctx, GL_INVALID_ENUM, FUNC "(0x%x)", (int) pname);  \\
      return;                                                          \\
   }

/*
 * Check if either of four extensions is enabled.
 */
#define CHECK_EXT4(EXT1, EXT2, EXT3, EXT4, FUNC)                       \\
   if (!ctx->Extensions.EXT1 && !ctx->Extensions.EXT2 &&               \\
       !ctx->Extensions.EXT3 && !ctx->Extensions.EXT4) {               \\
      _mesa_error(ctx, GL_INVALID_ENUM, FUNC "(0x%x)", (int) pname);  \\
      return;                                                          \\
   }



/**
 * List of compressed texture formats supported by ES.
 */
static GLenum compressed_formats[] = {
   GL_PALETTE4_RGB8_OES,
   GL_PALETTE4_RGBA8_OES,
   GL_PALETTE4_R5_G6_B5_OES,
   GL_PALETTE4_RGBA4_OES,
   GL_PALETTE4_RGB5_A1_OES,
   GL_PALETTE8_RGB8_OES,
   GL_PALETTE8_RGBA8_OES,
   GL_PALETTE8_R5_G6_B5_OES,
   GL_PALETTE8_RGBA4_OES,
   GL_PALETTE8_RGB5_A1_OES
};

#define ARRAY_SIZE(A)  (sizeof(A) / sizeof(A[0]))

void GLAPIENTRY
_mesa_GetFixedv( GLenum pname, GLfixed *params );

"""
	return


def EmitAll(stateVars, API):
	EmitHeader()
	EmitGetFunction(stateVars, GLboolean)
	EmitGetFunction(stateVars, GLfloat)
	EmitGetFunction(stateVars, GLint)
	if API == 1:
		EmitGetFunction(stateVars, GLfixed)


def main(args):
	# Determine whether to generate ES1 or ES2 queries
	if len(args) > 1 and args[1] == "1":
		API = 1
	elif len(args) > 1 and args[1] == "2":
		API = 2
	else:
		API = 1
	#print "len args = %d  API = %d" % (len(args), API)

	if API == 1:
		vars = StateVars_common + StateVars_es1
	else:
		vars = StateVars_common + StateVars_es2

	EmitAll(vars, API)


main(sys.argv)

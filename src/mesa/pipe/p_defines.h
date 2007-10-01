/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#ifndef PIPE_DEFINES_H
#define PIPE_DEFINES_H

#define PIPE_BLENDFACTOR_ONE                 0x1
#define PIPE_BLENDFACTOR_SRC_COLOR           0x2
#define PIPE_BLENDFACTOR_SRC_ALPHA           0x3
#define PIPE_BLENDFACTOR_DST_ALPHA           0x4
#define PIPE_BLENDFACTOR_DST_COLOR           0x5
#define PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE  0x6
#define PIPE_BLENDFACTOR_CONST_COLOR         0x7
#define PIPE_BLENDFACTOR_CONST_ALPHA         0x8
#define PIPE_BLENDFACTOR_SRC1_COLOR          0x9
#define PIPE_BLENDFACTOR_SRC1_ALPHA          0x0A
#define PIPE_BLENDFACTOR_ZERO                0x11
#define PIPE_BLENDFACTOR_INV_SRC_COLOR       0x12
#define PIPE_BLENDFACTOR_INV_SRC_ALPHA       0x13
#define PIPE_BLENDFACTOR_INV_DST_ALPHA       0x14
#define PIPE_BLENDFACTOR_INV_DST_COLOR       0x15
#define PIPE_BLENDFACTOR_INV_CONST_COLOR     0x17
#define PIPE_BLENDFACTOR_INV_CONST_ALPHA     0x18
#define PIPE_BLENDFACTOR_INV_SRC1_COLOR      0x19
#define PIPE_BLENDFACTOR_INV_SRC1_ALPHA      0x1A

#define PIPE_BLEND_ADD               0
#define PIPE_BLEND_SUBTRACT          1
#define PIPE_BLEND_REVERSE_SUBTRACT  2
#define PIPE_BLEND_MIN               3
#define PIPE_BLEND_MAX               4

#define PIPE_LOGICOP_CLEAR            0
#define PIPE_LOGICOP_NOR              1
#define PIPE_LOGICOP_AND_INVERTED     2
#define PIPE_LOGICOP_COPY_INVERTED    3
#define PIPE_LOGICOP_AND_REVERSE      4
#define PIPE_LOGICOP_INVERT           5
#define PIPE_LOGICOP_XOR              6
#define PIPE_LOGICOP_NAND             7
#define PIPE_LOGICOP_AND              8
#define PIPE_LOGICOP_EQUIV            9
#define PIPE_LOGICOP_NOOP             10
#define PIPE_LOGICOP_OR_INVERTED      11
#define PIPE_LOGICOP_COPY             12
#define PIPE_LOGICOP_OR_REVERSE       13
#define PIPE_LOGICOP_OR               14
#define PIPE_LOGICOP_SET              15  

#define PIPE_MASK_R  0x1
#define PIPE_MASK_G  0x2
#define PIPE_MASK_B  0x4
#define PIPE_MASK_A  0x8
#define PIPE_MASK_RGBA 0xf


/**
 * Inequality functions.  Used for depth test, stencil compare, alpha
 * test, shadow compare, etc.
 */
#define PIPE_FUNC_NEVER    0
#define PIPE_FUNC_LESS     1
#define PIPE_FUNC_EQUAL    2
#define PIPE_FUNC_LEQUAL   3
#define PIPE_FUNC_GREATER  4
#define PIPE_FUNC_NOTEQUAL 5
#define PIPE_FUNC_GEQUAL   6
#define PIPE_FUNC_ALWAYS   7

/** Polygon fill mode */
#define PIPE_POLYGON_MODE_FILL  0
#define PIPE_POLYGON_MODE_LINE  1
#define PIPE_POLYGON_MODE_POINT 2

/** Polygon front/back window, also for culling */
#define PIPE_WINDING_NONE 0
#define PIPE_WINDING_CW   1
#define PIPE_WINDING_CCW  2
#define PIPE_WINDING_BOTH (PIPE_WINDING_CW | PIPE_WINDING_CCW)

/** Stencil ops */
#define PIPE_STENCIL_OP_KEEP       0
#define PIPE_STENCIL_OP_ZERO       1
#define PIPE_STENCIL_OP_REPLACE    2
#define PIPE_STENCIL_OP_INCR       3
#define PIPE_STENCIL_OP_DECR       4
#define PIPE_STENCIL_OP_INCR_WRAP  5
#define PIPE_STENCIL_OP_DECR_WRAP  6
#define PIPE_STENCIL_OP_INVERT     7

/** Texture types */
#define PIPE_TEXTURE_1D   0
#define PIPE_TEXTURE_2D   1
#define PIPE_TEXTURE_3D   2
#define PIPE_TEXTURE_CUBE 3

#define PIPE_TEX_FACE_POS_X 0
#define PIPE_TEX_FACE_NEG_X 1
#define PIPE_TEX_FACE_POS_Y 2
#define PIPE_TEX_FACE_NEG_Y 3
#define PIPE_TEX_FACE_POS_Z 4
#define PIPE_TEX_FACE_NEG_Z 5

#define PIPE_TEX_WRAP_REPEAT                   0
#define PIPE_TEX_WRAP_CLAMP                    1
#define PIPE_TEX_WRAP_CLAMP_TO_EDGE            2
#define PIPE_TEX_WRAP_CLAMP_TO_BORDER          3
#define PIPE_TEX_WRAP_MIRROR_REPEAT            4
#define PIPE_TEX_WRAP_MIRROR_CLAMP             5
#define PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE     6
#define PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER   7

/* Between mipmaps, ie mipfilter
 */
#define PIPE_TEX_MIPFILTER_NEAREST  0
#define PIPE_TEX_MIPFILTER_LINEAR   1
#define PIPE_TEX_MIPFILTER_NONE     2

/* Within a mipmap, ie min/mag filter 
 */
#define PIPE_TEX_FILTER_NEAREST      0
#define PIPE_TEX_FILTER_LINEAR       1
//#define PIPE_TEX_FILTER_ANISO        2


#define PIPE_TEX_COMPARE_NONE          0
#define PIPE_TEX_COMPARE_R_TO_TEXTURE  1

#define PIPE_TEX_FACE_POS_X   0
#define PIPE_TEX_FACE_NEG_X   1
#define PIPE_TEX_FACE_POS_Y   2
#define PIPE_TEX_FACE_NEG_Y   3
#define PIPE_TEX_FACE_POS_Z   4
#define PIPE_TEX_FACE_NEG_Z   5
#define PIPE_TEX_FACE_MAX     6


/**
 * Texture/surface image formats (preliminary)
 */

/* KW: Added lots of surface formats to support vertex element layout
 * definitions, and eventually render-to-vertex-buffer.  Could
 * consider making float/int/uint/scaled/normalized a separate
 * parameter, but on the other hand there are special cases like
 * z24s8, compressed textures, ycbcr, etc that won't fit that model.
 */

#define PIPE_FORMAT_NONE               0  /**< unstructured */
#define PIPE_FORMAT_U_A8_R8_G8_B8      2  /**< ubyte[4] ARGB */
#define PIPE_FORMAT_U_A1_R5_G5_B5      3  /**< 16-bit packed RGBA */
#define PIPE_FORMAT_U_A4_R4_G4_B4      4  /**< 16-bit packed RGBA */
#define PIPE_FORMAT_U_R5_G6_B5         5  /**< 16-bit packed RGB */
#define PIPE_FORMAT_U_L8               6  /**< ubyte luminance */
#define PIPE_FORMAT_U_A8               7  /**< ubyte alpha */
#define PIPE_FORMAT_U_I8               8  /**< ubyte intensity */
#define PIPE_FORMAT_U_A8_L8            9  /**< ubyte alpha, luminance */
#define PIPE_FORMAT_S_R16_G16_B16_A16 10  /**< signed 16-bit RGBA (accum) */
#define PIPE_FORMAT_YCBCR             11
#define PIPE_FORMAT_YCBCR_REV         12
#define PIPE_FORMAT_U_Z16             13  /**< ushort Z/depth */
#define PIPE_FORMAT_U_Z32             14  /**< uint Z/depth */
#define PIPE_FORMAT_F_Z32             15  /**< float Z/depth */
#define PIPE_FORMAT_S8_Z24            16  /**< 8-bit stencil + 24-bit Z */
#define PIPE_FORMAT_U_S8              17  /**< 8-bit stencil */
#define PIPE_FORMAT_R64_FLOAT             0x20
#define PIPE_FORMAT_R64G64_FLOAT          0x21
#define PIPE_FORMAT_R64G64B64_FLOAT       0x22
#define PIPE_FORMAT_R64G64B64A64_FLOAT    0x23
#define PIPE_FORMAT_R32_FLOAT             0x24
#define PIPE_FORMAT_R32G32_FLOAT          0x25
#define PIPE_FORMAT_R32G32B32_FLOAT       0x26
#define PIPE_FORMAT_R32G32B32A32_FLOAT    0x27
#define PIPE_FORMAT_R32_UNORM             0x28
#define PIPE_FORMAT_R32G32_UNORM          0x29
#define PIPE_FORMAT_R32G32B32_UNORM       0x2a
#define PIPE_FORMAT_R32G32B32A32_UNORM    0x2b
#define PIPE_FORMAT_R32_USCALED           0x2c
#define PIPE_FORMAT_R32G32_USCALED        0x2d
#define PIPE_FORMAT_R32G32B32_USCALED     0x2e
#define PIPE_FORMAT_R32G32B32A32_USCALED  0x2f
#define PIPE_FORMAT_R32_SNORM             0x30
#define PIPE_FORMAT_R32G32_SNORM          0x31
#define PIPE_FORMAT_R32G32B32_SNORM       0x32
#define PIPE_FORMAT_R32G32B32A32_SNORM    0x33
#define PIPE_FORMAT_R32_SSCALED           0x34
#define PIPE_FORMAT_R32G32_SSCALED        0x35
#define PIPE_FORMAT_R32G32B32_SSCALED     0x36
#define PIPE_FORMAT_R32G32B32A32_SSCALED  0x37
#define PIPE_FORMAT_R16_UNORM             0x38
#define PIPE_FORMAT_R16G16_UNORM          0x39
#define PIPE_FORMAT_R16G16B16_UNORM       0x3a
#define PIPE_FORMAT_R16G16B16A16_UNORM    0x3b
#define PIPE_FORMAT_R16_USCALED           0x3c
#define PIPE_FORMAT_R16G16_USCALED        0x3d
#define PIPE_FORMAT_R16G16B16_USCALED     0x3e
#define PIPE_FORMAT_R16G16B16A16_USCALED  0x3f
#define PIPE_FORMAT_R16_SNORM             0x40
#define PIPE_FORMAT_R16G16_SNORM          0x41
#define PIPE_FORMAT_R16G16B16_SNORM       0x42
#define PIPE_FORMAT_R16G16B16A16_SNORM    0x43
#define PIPE_FORMAT_R16_SSCALED           0x44
#define PIPE_FORMAT_R16G16_SSCALED        0x45
#define PIPE_FORMAT_R16G16B16_SSCALED     0x46
#define PIPE_FORMAT_R16G16B16A16_SSCALED  0x47
#define PIPE_FORMAT_R8_UNORM              0x48
#define PIPE_FORMAT_R8G8_UNORM            0x49
#define PIPE_FORMAT_R8G8B8_UNORM          0x4a
#define PIPE_FORMAT_R8G8B8A8_UNORM        0x4b
#define PIPE_FORMAT_R8_USCALED            0x4c
#define PIPE_FORMAT_R8G8_USCALED          0x4d
#define PIPE_FORMAT_R8G8B8_USCALED        0x4e
#define PIPE_FORMAT_R8G8B8A8_USCALED      0x4f
#define PIPE_FORMAT_R8_SNORM              0x50
#define PIPE_FORMAT_R8G8_SNORM            0x51
#define PIPE_FORMAT_R8G8B8_SNORM          0x52
#define PIPE_FORMAT_R8G8B8A8_SNORM        0x53
#define PIPE_FORMAT_R8_SSCALED            0x54
#define PIPE_FORMAT_R8G8_SSCALED          0x55
#define PIPE_FORMAT_R8G8B8_SSCALED        0x56
#define PIPE_FORMAT_R8G8B8A8_SSCALED      0x57

#define PIPE_FORMAT_COUNT                 0x58  /**< number of formats */

/* Duplicated formats:
 */
#define PIPE_FORMAT_U_R8_G8_B8_A8      PIPE_FORMAT_R8G8B8A8_UNORM

/**
 * Surface flags
 */
#define PIPE_SURFACE_FLAG_TEXTURE 0x1
#define PIPE_SURFACE_FLAG_RENDER  0x2


/**
 * Buffer flags
 */
#define PIPE_BUFFER_FLAG_READ    0x1
#define PIPE_BUFFER_FLAG_WRITE   0x2

#define PIPE_BUFFER_USE_TEXTURE         0x1
#define PIPE_BUFFER_USE_VERTEX_BUFFER   0x2
#define PIPE_BUFFER_USE_INDEX_BUFFER    0x4
#define PIPE_BUFFER_USE_RENDER_TARGET   0x8


/** 
 * Flush types:
 */
#define PIPE_FLUSH_RENDER_CACHE  0x1
#define PIPE_FLUSH_TEXTURE_CACHE  0x2


/**
 * Shaders
 */
#define PIPE_SHADER_VERTEX   0
#define PIPE_SHADER_FRAGMENT 1
#define PIPE_SHADER_TYPES    2


/**
 * Primitive types:
 */
#define PIPE_PRIM_POINTS          0
#define PIPE_PRIM_LINES           1
#define PIPE_PRIM_LINE_LOOP       2
#define PIPE_PRIM_LINE_STRIP      3
#define PIPE_PRIM_TRIANGLES       4
#define PIPE_PRIM_TRIANGLE_STRIP  5
#define PIPE_PRIM_TRIANGLE_FAN    6
#define PIPE_PRIM_QUADS           7
#define PIPE_PRIM_QUAD_STRIP      8
#define PIPE_PRIM_POLYGON         9


/**
 * Query object types
 */
#define PIPE_QUERY_OCCLUSION_COUNTER     0
#define PIPE_QUERY_PRIMITIVES_GENERATED  1
#define PIPE_QUERY_PRIMITIVES_EMITTED    2
#define PIPE_QUERY_TYPES                 3


#endif

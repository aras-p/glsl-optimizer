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

#include "p_format.h"

#ifdef __cplusplus
extern "C" {
#endif

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
enum pipe_texture_target {
   PIPE_TEXTURE_1D   = 0,
   PIPE_TEXTURE_2D   = 1,
   PIPE_TEXTURE_3D   = 2,
   PIPE_TEXTURE_CUBE = 3
};

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
#define PIPE_TEX_FILTER_ANISO        2 


#define PIPE_TEX_COMPARE_NONE          0
#define PIPE_TEX_COMPARE_R_TO_TEXTURE  1

#define PIPE_TEX_FACE_POS_X   0
#define PIPE_TEX_FACE_NEG_X   1
#define PIPE_TEX_FACE_POS_Y   2
#define PIPE_TEX_FACE_NEG_Y   3
#define PIPE_TEX_FACE_POS_Z   4
#define PIPE_TEX_FACE_NEG_Z   5
#define PIPE_TEX_FACE_MAX     6

#define PIPE_TEXTURE_USAGE_RENDER_TARGET   0x1
#define PIPE_TEXTURE_USAGE_DISPLAY_TARGET  0x2 /* ie a backbuffer */
#define PIPE_TEXTURE_USAGE_PRIMARY         0x4 /* ie a frontbuffer */
#define PIPE_TEXTURE_USAGE_DEPTH_STENCIL   0x8
#define PIPE_TEXTURE_USAGE_SAMPLER         0x10

#define PIPE_TEXTURE_GEOM_NON_SQUARE       0x1
#define PIPE_TEXTURE_GEOM_NON_POWER_OF_TWO 0x2


/**
 * Surface layout
 */
#define PIPE_SURFACE_LAYOUT_LINEAR  0


/**
 * Surface status
 */
#define PIPE_SURFACE_STATUS_UNDEFINED  0
#define PIPE_SURFACE_STATUS_DEFINED    1
#define PIPE_SURFACE_STATUS_CLEAR      2


/**
 * Buffer usage flags
 */
#define PIPE_BUFFER_USAGE_CPU_READ  (1 << 0)
#define PIPE_BUFFER_USAGE_CPU_WRITE (1 << 1)
#define PIPE_BUFFER_USAGE_GPU_READ  (1 << 2)
#define PIPE_BUFFER_USAGE_GPU_WRITE (1 << 3)
#define PIPE_BUFFER_USAGE_PIXEL     (1 << 4)
#define PIPE_BUFFER_USAGE_VERTEX    (1 << 5)
#define PIPE_BUFFER_USAGE_INDEX     (1 << 6)
#define PIPE_BUFFER_USAGE_CONSTANT  (1 << 7)
/** Pipe driver custom usage flags should be greater or equal to this value */
#define PIPE_BUFFER_USAGE_CUSTOM    (1 << 16)


/** 
 * Flush types:
 */
#define PIPE_FLUSH_RENDER_CACHE   0x1
#define PIPE_FLUSH_TEXTURE_CACHE  0x2
#define PIPE_FLUSH_SWAPBUFFERS    0x4
#define PIPE_FLUSH_FRAME          0x8 /**< Mark the end of a frame */


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


/**
 * Point sprite coord modes
 */
#define PIPE_SPRITE_COORD_NONE       0
#define PIPE_SPRITE_COORD_UPPER_LEFT 1
#define PIPE_SPRITE_COORD_LOWER_LEFT 2


/**
 * Implementation capabilities/limits
 * Passed to pipe->get_param()
 * XXX this will need some fine tuning...
 */
#define PIPE_CAP_MAX_TEXTURE_IMAGE_UNITS 1
#define PIPE_CAP_NPOT_TEXTURES           2
#define PIPE_CAP_TWO_SIDED_STENCIL       3
#define PIPE_CAP_GLSL                    4  /* XXX need something better */
#define PIPE_CAP_S3TC                    5
#define PIPE_CAP_ANISOTROPIC_FILTER      6
#define PIPE_CAP_POINT_SPRITE            7
#define PIPE_CAP_MAX_RENDER_TARGETS      8
#define PIPE_CAP_OCCLUSION_QUERY         9
#define PIPE_CAP_TEXTURE_SHADOW_MAP      10
#define PIPE_CAP_MAX_TEXTURE_2D_LEVELS   11
#define PIPE_CAP_MAX_TEXTURE_3D_LEVELS   12
#define PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS 13
#define PIPE_CAP_MAX_LINE_WIDTH          14
#define PIPE_CAP_MAX_LINE_WIDTH_AA       15
#define PIPE_CAP_MAX_POINT_WIDTH         16
#define PIPE_CAP_MAX_POINT_WIDTH_AA      17
#define PIPE_CAP_MAX_TEXTURE_ANISOTROPY  18
#define PIPE_CAP_MAX_TEXTURE_LOD_BIAS    19
#define PIPE_CAP_GUARD_BAND_LEFT         20  /*< float */
#define PIPE_CAP_GUARD_BAND_TOP          21  /*< float */
#define PIPE_CAP_GUARD_BAND_RIGHT        22  /*< float */
#define PIPE_CAP_GUARD_BAND_BOTTOM       23  /*< float */
#define PIPE_CAP_TEXTURE_MIRROR_CLAMP    24
#define PIPE_CAP_TEXTURE_MIRROR_REPEAT   25



#ifdef __cplusplus
}
#endif

#endif

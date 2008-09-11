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

#ifndef PIPE_FORMAT_H
#define PIPE_FORMAT_H

#include "p_compiler.h"
#include "p_debug.h"

#include "util/u_string.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The pipe_format enum is a 32-bit wide bitfield that encodes all the
 * information needed to uniquely describe a pixel format.
 */

/**
 * Possible format layouts are encoded in the first 2 bits.
 * The interpretation of the remaining 30 bits depends on a particular
 * format layout.
 */
#define PIPE_FORMAT_LAYOUT_RGBAZS   0
#define PIPE_FORMAT_LAYOUT_YCBCR    1
#define PIPE_FORMAT_LAYOUT_DXT      2  /**< XXX temporary? */
#define PIPE_FORMAT_LAYOUT_MIXED    3

static INLINE uint pf_layout(uint f)  /**< PIPE_FORMAT_LAYOUT_ */
{
   return f & 0x3;
}

/**
 * RGBAZS Format Layout.
 */

/**
 * Format component selectors for RGBAZS & MIXED layout.
 */
#define PIPE_FORMAT_COMP_R    0
#define PIPE_FORMAT_COMP_G    1
#define PIPE_FORMAT_COMP_B    2
#define PIPE_FORMAT_COMP_A    3
#define PIPE_FORMAT_COMP_0    4
#define PIPE_FORMAT_COMP_1    5
#define PIPE_FORMAT_COMP_Z    6
#define PIPE_FORMAT_COMP_S    7

/**
 * Format types for RGBAZS layout.
 */
#define PIPE_FORMAT_TYPE_UNKNOWN 0
#define PIPE_FORMAT_TYPE_FLOAT   1  /**< 16/32/64-bit/channel formats */
#define PIPE_FORMAT_TYPE_UNORM   2  /**< uints, normalized to [0,1] */
#define PIPE_FORMAT_TYPE_SNORM   3  /**< ints, normalized to [-1,1] */
#define PIPE_FORMAT_TYPE_USCALED 4  /**< uints, not normalized */
#define PIPE_FORMAT_TYPE_SSCALED 5  /**< ints, not normalized */
#define PIPE_FORMAT_TYPE_SRGB    6  /**< sRGB colorspace */
#define PIPE_FORMAT_TYPE_FIXED   7  /**< 16.16 fixed point */


/**
 * Because the destination vector is assumed to be RGBA FLOAT, we
 * need to know how to swizzle and expand components from the source
 * vector.
 * Let's take U_A1_R5_G5_B5 as an example. X swizzle is A, X size
 * is 1 bit and type is UNORM. So we take the most significant bit
 * from source vector, convert 0 to 0.0 and 1 to 1.0 and save it
 * in the last component of the destination RGBA component.
 * Next, Y swizzle is R, Y size is 5 and type is UNORM. We normalize
 * those 5 bits into [0.0; 1.0] range and put it into second
 * component of the destination vector. Rinse and repeat for
 * components Z and W.
 * If any of size fields is zero, it means the source format contains
 * less than four components.
 * If any swizzle is 0 or 1, the corresponding destination component
 * should be filled with 0.0 and 1.0, respectively.
 */
typedef uint pipe_format_rgbazs_t;

static INLINE uint pf_get(pipe_format_rgbazs_t f, uint shift, uint mask)
{
   return (f >> shift) & mask;
}

#define pf_swizzle_x(f)       pf_get(f, 2, 0x7)  /**< PIPE_FORMAT_COMP_ */
#define pf_swizzle_y(f)       pf_get(f, 5, 0x7)  /**< PIPE_FORMAT_COMP_ */
#define pf_swizzle_z(f)       pf_get(f, 8, 0x7)  /**< PIPE_FORMAT_COMP_ */
#define pf_swizzle_w(f)       pf_get(f, 11, 0x7) /**< PIPE_FORMAT_COMP_ */
#define pf_swizzle_xyzw(f,i)  pf_get(f, 2+((i)*3), 0x7)
#define pf_size_x(f)          pf_get(f, 14, 0x7) /**< Size of X */
#define pf_size_y(f)          pf_get(f, 17, 0x7) /**< Size of Y */
#define pf_size_z(f)          pf_get(f, 20, 0x7) /**< Size of Z */
#define pf_size_w(f)          pf_get(f, 23, 0x7) /**< Size of W */
#define pf_size_xyzw(f,i)     pf_get(f, 14+((i)*3), 0x7)
#define pf_exp2(f)            pf_get(f, 26, 0x7) /**< Scale size by 2 ^ exp2 */
#define pf_type(f)            pf_get(f, 29, 0x7) /**< PIPE_FORMAT_TYPE_ */

/**
 * Helper macro to encode the above structure into a 32-bit value.
 */
#define _PIPE_FORMAT_RGBAZS( SWZ, SIZEX, SIZEY, SIZEZ, SIZEW, EXP2, TYPE ) (\
   (PIPE_FORMAT_LAYOUT_RGBAZS << 0) |\
   ((SWZ) << 2) |\
   ((SIZEX) << 14) |\
   ((SIZEY) << 17) |\
   ((SIZEZ) << 20) |\
   ((SIZEW) << 23) |\
   ((EXP2) << 26) |\
   ((TYPE) << 29) )

/**
 * Helper macro to encode the swizzle part of the structure above.
 */
#define _PIPE_FORMAT_SWZ( SWZX, SWZY, SWZZ, SWZW ) (((SWZX) << 0) | ((SWZY) << 3) | ((SWZZ) << 6) | ((SWZW) << 9))

/**
 * Shorthand macro for RGBAZS layout with component sizes in 1-bit units.
 */
#define _PIPE_FORMAT_RGBAZS_1( SWZ, SIZEX, SIZEY, SIZEZ, SIZEW, TYPE )\
   _PIPE_FORMAT_RGBAZS( SWZ, SIZEX, SIZEY, SIZEZ, SIZEW, 0, TYPE )

/**
 * Shorthand macro for RGBAZS layout with component sizes in 2-bit units.
 */
#define _PIPE_FORMAT_RGBAZS_2( SWZ, SIZEX, SIZEY, SIZEZ, SIZEW, TYPE )\
   _PIPE_FORMAT_RGBAZS( SWZ, SIZEX, SIZEY, SIZEZ, SIZEW, 1, TYPE )

/**
 * Shorthand macro for RGBAZS layout with component sizes in 8-bit units.
 */
#define _PIPE_FORMAT_RGBAZS_8( SWZ, SIZEX, SIZEY, SIZEZ, SIZEW, TYPE )\
   _PIPE_FORMAT_RGBAZS( SWZ, SIZEX, SIZEY, SIZEZ, SIZEW, 3, TYPE )

/**
 * Shorthand macro for RGBAZS layout with component sizes in 64-bit units.
 */
#define _PIPE_FORMAT_RGBAZS_64( SWZ, SIZEX, SIZEY, SIZEZ, SIZEW, TYPE )\
   _PIPE_FORMAT_RGBAZS( SWZ, SIZEX, SIZEY, SIZEZ, SIZEW, 6, TYPE )

typedef uint pipe_format_mixed_t;

/* NOTE: Use pf_swizzle_* and pf_size_* macros for swizzles and sizes.
 */

#define pf_mixed_sign_x(f)       pf_get( f, 26, 0x1 )       /*< Sign of X */
#define pf_mixed_sign_y(f)       pf_get( f, 27, 0x1 )       /*< Sign of Y */
#define pf_mixed_sign_z(f)       pf_get( f, 28, 0x1 )       /*< Sign of Z */
#define pf_mixed_sign_w(f)       pf_get( f, 29, 0x1 )       /*< Sign of W */
#define pf_mixed_sign_xyzw(f, i) pf_get( f, 26 + (i), 0x1 )
#define pf_mixed_normalized(f)   pf_get( f, 30, 0x1 )       /*< Type is NORM (1) or SCALED (0) */
#define pf_mixed_scale8(f)       pf_get( f, 31, 0x1 )       /*< Scale size by either one (0) or eight (1) */

/**
 * Helper macro to encode the above structure into a 32-bit value.
 */
#define _PIPE_FORMAT_MIXED( SWZ, SIZEX, SIZEY, SIZEZ, SIZEW, SIGNX, SIGNY, SIGNZ, SIGNW, NORMALIZED, SCALE8 ) (\
   (PIPE_FORMAT_LAYOUT_MIXED << 0) |\
   ((SWZ) << 2) |\
   ((SIZEX) << 14) |\
   ((SIZEY) << 17) |\
   ((SIZEZ) << 20) |\
   ((SIZEW) << 23) |\
   ((SIGNX) << 26) |\
   ((SIGNY) << 27) |\
   ((SIGNZ) << 28) |\
   ((SIGNW) << 29) |\
   ((NORMALIZED) << 30) |\
   ((SCALE8) << 31) )

/**
 * Shorthand macro for common format swizzles.
 */
#define _PIPE_FORMAT_R001 _PIPE_FORMAT_SWZ( PIPE_FORMAT_COMP_R, PIPE_FORMAT_COMP_0, PIPE_FORMAT_COMP_0, PIPE_FORMAT_COMP_1 )
#define _PIPE_FORMAT_RG01 _PIPE_FORMAT_SWZ( PIPE_FORMAT_COMP_R, PIPE_FORMAT_COMP_G, PIPE_FORMAT_COMP_0, PIPE_FORMAT_COMP_1 )
#define _PIPE_FORMAT_RGB1 _PIPE_FORMAT_SWZ( PIPE_FORMAT_COMP_R, PIPE_FORMAT_COMP_G, PIPE_FORMAT_COMP_B, PIPE_FORMAT_COMP_1 )
#define _PIPE_FORMAT_RGBA _PIPE_FORMAT_SWZ( PIPE_FORMAT_COMP_R, PIPE_FORMAT_COMP_G, PIPE_FORMAT_COMP_B, PIPE_FORMAT_COMP_A )
#define _PIPE_FORMAT_ARGB _PIPE_FORMAT_SWZ( PIPE_FORMAT_COMP_A, PIPE_FORMAT_COMP_R, PIPE_FORMAT_COMP_G, PIPE_FORMAT_COMP_B )
#define _PIPE_FORMAT_ABGR _PIPE_FORMAT_SWZ( PIPE_FORMAT_COMP_A, PIPE_FORMAT_COMP_B, PIPE_FORMAT_COMP_G, PIPE_FORMAT_COMP_R )
#define _PIPE_FORMAT_BGRA _PIPE_FORMAT_SWZ( PIPE_FORMAT_COMP_B, PIPE_FORMAT_COMP_G, PIPE_FORMAT_COMP_R, PIPE_FORMAT_COMP_A )
#define _PIPE_FORMAT_1RGB _PIPE_FORMAT_SWZ( PIPE_FORMAT_COMP_1, PIPE_FORMAT_COMP_R, PIPE_FORMAT_COMP_G, PIPE_FORMAT_COMP_B )
#define _PIPE_FORMAT_1BGR _PIPE_FORMAT_SWZ( PIPE_FORMAT_COMP_1, PIPE_FORMAT_COMP_B, PIPE_FORMAT_COMP_G, PIPE_FORMAT_COMP_R )
#define _PIPE_FORMAT_BGR1 _PIPE_FORMAT_SWZ( PIPE_FORMAT_COMP_B, PIPE_FORMAT_COMP_G, PIPE_FORMAT_COMP_R, PIPE_FORMAT_COMP_1 )
#define _PIPE_FORMAT_0000 _PIPE_FORMAT_SWZ( PIPE_FORMAT_COMP_0, PIPE_FORMAT_COMP_0, PIPE_FORMAT_COMP_0, PIPE_FORMAT_COMP_0 )
#define _PIPE_FORMAT_000R _PIPE_FORMAT_SWZ( PIPE_FORMAT_COMP_0, PIPE_FORMAT_COMP_0, PIPE_FORMAT_COMP_0, PIPE_FORMAT_COMP_R )
#define _PIPE_FORMAT_RRR1 _PIPE_FORMAT_SWZ( PIPE_FORMAT_COMP_R, PIPE_FORMAT_COMP_R, PIPE_FORMAT_COMP_R, PIPE_FORMAT_COMP_1 )
#define _PIPE_FORMAT_RRRR _PIPE_FORMAT_SWZ( PIPE_FORMAT_COMP_R, PIPE_FORMAT_COMP_R, PIPE_FORMAT_COMP_R, PIPE_FORMAT_COMP_R )
#define _PIPE_FORMAT_RRRG _PIPE_FORMAT_SWZ( PIPE_FORMAT_COMP_R, PIPE_FORMAT_COMP_R, PIPE_FORMAT_COMP_R, PIPE_FORMAT_COMP_G )
#define _PIPE_FORMAT_Z000 _PIPE_FORMAT_SWZ( PIPE_FORMAT_COMP_Z, PIPE_FORMAT_COMP_0, PIPE_FORMAT_COMP_0, PIPE_FORMAT_COMP_0 )
#define _PIPE_FORMAT_0Z00 _PIPE_FORMAT_SWZ( PIPE_FORMAT_COMP_0, PIPE_FORMAT_COMP_Z, PIPE_FORMAT_COMP_0, PIPE_FORMAT_COMP_0 )
#define _PIPE_FORMAT_SZ00 _PIPE_FORMAT_SWZ( PIPE_FORMAT_COMP_S, PIPE_FORMAT_COMP_Z, PIPE_FORMAT_COMP_0, PIPE_FORMAT_COMP_0 )
#define _PIPE_FORMAT_ZS00 _PIPE_FORMAT_SWZ( PIPE_FORMAT_COMP_Z, PIPE_FORMAT_COMP_S, PIPE_FORMAT_COMP_0, PIPE_FORMAT_COMP_0 )
#define _PIPE_FORMAT_S000 _PIPE_FORMAT_SWZ( PIPE_FORMAT_COMP_S, PIPE_FORMAT_COMP_0, PIPE_FORMAT_COMP_0, PIPE_FORMAT_COMP_0 )

/**
 * YCBCR Format Layout.
 */

/**
 * This only contains a flag that indicates whether the format is reversed or
 * not.
 */
typedef uint pipe_format_ycbcr_t;

/**
 * Helper macro to encode the above structure into a 32-bit value.
 */
#define _PIPE_FORMAT_YCBCR( REV ) (\
   (PIPE_FORMAT_LAYOUT_YCBCR << 0) |\
   ((REV) << 2) )

static INLINE uint pf_rev(pipe_format_ycbcr_t f)
{
   return (f >> 2) & 0x1;
}


/**
  * Compresssed format layouts (this will probably change)
  */
#define _PIPE_FORMAT_DXT( LEVEL, RSIZE, GSIZE, BSIZE, ASIZE ) \
   ((PIPE_FORMAT_LAYOUT_DXT << 0) | \
    ((LEVEL) << 2) | \
    ((RSIZE) << 5) | \
    ((GSIZE) << 8) | \
    ((BSIZE) << 11) | \
    ((ASIZE) << 14) )



/**
 * Texture/surface image formats (preliminary)
 */

/* KW: Added lots of surface formats to support vertex element layout
 * definitions, and eventually render-to-vertex-buffer.  Could
 * consider making float/int/uint/scaled/normalized a separate
 * parameter, but on the other hand there are special cases like
 * z24s8, compressed textures, ycbcr, etc that won't fit that model.
 */

enum pipe_format {
   PIPE_FORMAT_NONE                  = _PIPE_FORMAT_RGBAZS_1 ( _PIPE_FORMAT_0000, 0, 0, 0, 0, PIPE_FORMAT_TYPE_UNKNOWN ),
   PIPE_FORMAT_A8R8G8B8_UNORM        = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_ARGB, 1, 1, 1, 1, PIPE_FORMAT_TYPE_UNORM ),
   PIPE_FORMAT_X8R8G8B8_UNORM        = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_1RGB, 1, 1, 1, 1, PIPE_FORMAT_TYPE_UNORM ),
   PIPE_FORMAT_B8G8R8A8_UNORM        = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_BGRA, 1, 1, 1, 1, PIPE_FORMAT_TYPE_UNORM ),
   PIPE_FORMAT_B8G8R8X8_UNORM        = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_BGR1, 1, 1, 1, 1, PIPE_FORMAT_TYPE_UNORM ),
   PIPE_FORMAT_A1R5G5B5_UNORM        = _PIPE_FORMAT_RGBAZS_1 ( _PIPE_FORMAT_ARGB, 1, 5, 5, 5, PIPE_FORMAT_TYPE_UNORM ),
   PIPE_FORMAT_A4R4G4B4_UNORM        = _PIPE_FORMAT_RGBAZS_1 ( _PIPE_FORMAT_ARGB, 4, 4, 4, 4, PIPE_FORMAT_TYPE_UNORM ),
   PIPE_FORMAT_R5G6B5_UNORM          = _PIPE_FORMAT_RGBAZS_1 ( _PIPE_FORMAT_RGB1, 5, 6, 5, 0, PIPE_FORMAT_TYPE_UNORM ),
   PIPE_FORMAT_A2B10G10R10_UNORM     = _PIPE_FORMAT_RGBAZS_2 ( _PIPE_FORMAT_ABGR, 1, 5, 5, 5, PIPE_FORMAT_TYPE_UNORM ),
   PIPE_FORMAT_L8_UNORM              = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RRR1, 1, 1, 1, 0, PIPE_FORMAT_TYPE_UNORM ), /**< ubyte luminance */
   PIPE_FORMAT_A8_UNORM              = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_000R, 0, 0, 0, 1, PIPE_FORMAT_TYPE_UNORM ), /**< ubyte alpha */
   PIPE_FORMAT_I8_UNORM              = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RRRR, 1, 1, 1, 1, PIPE_FORMAT_TYPE_UNORM ), /**< ubyte intensity */
   PIPE_FORMAT_A8L8_UNORM            = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RRRG, 1, 1, 1, 1, PIPE_FORMAT_TYPE_UNORM ), /**< ubyte alpha, luminance */
   PIPE_FORMAT_L16_UNORM             = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RRR1, 2, 2, 2, 0, PIPE_FORMAT_TYPE_UNORM ), /**< ushort luminance */
   PIPE_FORMAT_YCBCR                 = _PIPE_FORMAT_YCBCR( 0 ),
   PIPE_FORMAT_YCBCR_REV             = _PIPE_FORMAT_YCBCR( 1 ),
   PIPE_FORMAT_Z16_UNORM             = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_Z000, 2, 0, 0, 0, PIPE_FORMAT_TYPE_UNORM ),
   PIPE_FORMAT_Z32_UNORM             = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_Z000, 4, 0, 0, 0, PIPE_FORMAT_TYPE_UNORM ),
   PIPE_FORMAT_Z32_FLOAT             = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_Z000, 4, 0, 0, 0, PIPE_FORMAT_TYPE_FLOAT ),
   PIPE_FORMAT_S8Z24_UNORM           = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_SZ00, 1, 3, 0, 0, PIPE_FORMAT_TYPE_UNORM ),
   PIPE_FORMAT_Z24S8_UNORM           = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_ZS00, 3, 1, 0, 0, PIPE_FORMAT_TYPE_UNORM ),
   PIPE_FORMAT_X8Z24_UNORM           = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_0Z00, 1, 3, 0, 0, PIPE_FORMAT_TYPE_UNORM ),
   PIPE_FORMAT_Z24X8_UNORM           = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_Z000, 3, 1, 0, 0, PIPE_FORMAT_TYPE_UNORM ),
   PIPE_FORMAT_S8_UNORM              = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_S000, 1, 0, 0, 0, PIPE_FORMAT_TYPE_UNORM ), /**< ubyte stencil */
   PIPE_FORMAT_R64_FLOAT             = _PIPE_FORMAT_RGBAZS_64( _PIPE_FORMAT_R001, 1, 0, 0, 0, PIPE_FORMAT_TYPE_FLOAT ),
   PIPE_FORMAT_R64G64_FLOAT          = _PIPE_FORMAT_RGBAZS_64( _PIPE_FORMAT_RG01, 1, 1, 0, 0, PIPE_FORMAT_TYPE_FLOAT ),
   PIPE_FORMAT_R64G64B64_FLOAT       = _PIPE_FORMAT_RGBAZS_64( _PIPE_FORMAT_RGB1, 1, 1, 1, 0, PIPE_FORMAT_TYPE_FLOAT ),
   PIPE_FORMAT_R64G64B64A64_FLOAT    = _PIPE_FORMAT_RGBAZS_64( _PIPE_FORMAT_RGBA, 1, 1, 1, 1, PIPE_FORMAT_TYPE_FLOAT ),
   PIPE_FORMAT_R32_FLOAT             = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_R001, 4, 0, 0, 0, PIPE_FORMAT_TYPE_FLOAT ),
   PIPE_FORMAT_R32G32_FLOAT          = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RG01, 4, 4, 0, 0, PIPE_FORMAT_TYPE_FLOAT ),
   PIPE_FORMAT_R32G32B32_FLOAT       = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RGB1, 4, 4, 4, 0, PIPE_FORMAT_TYPE_FLOAT ),
   PIPE_FORMAT_R32G32B32A32_FLOAT    = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RGBA, 4, 4, 4, 4, PIPE_FORMAT_TYPE_FLOAT ),
   PIPE_FORMAT_R32_UNORM             = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_R001, 4, 0, 0, 0, PIPE_FORMAT_TYPE_UNORM ),
   PIPE_FORMAT_R32G32_UNORM          = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RG01, 4, 4, 0, 0, PIPE_FORMAT_TYPE_UNORM ),
   PIPE_FORMAT_R32G32B32_UNORM       = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RGB1, 4, 4, 4, 0, PIPE_FORMAT_TYPE_UNORM ),
   PIPE_FORMAT_R32G32B32A32_UNORM    = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RGBA, 4, 4, 4, 4, PIPE_FORMAT_TYPE_UNORM ),
   PIPE_FORMAT_R32_USCALED           = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_R001, 4, 0, 0, 0, PIPE_FORMAT_TYPE_USCALED ),
   PIPE_FORMAT_R32G32_USCALED        = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RG01, 4, 4, 0, 0, PIPE_FORMAT_TYPE_USCALED ),
   PIPE_FORMAT_R32G32B32_USCALED     = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RGB1, 4, 4, 4, 0, PIPE_FORMAT_TYPE_USCALED ),
   PIPE_FORMAT_R32G32B32A32_USCALED  = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RGBA, 4, 4, 4, 4, PIPE_FORMAT_TYPE_USCALED ),
   PIPE_FORMAT_R32_SNORM             = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_R001, 4, 0, 0, 0, PIPE_FORMAT_TYPE_SNORM ),
   PIPE_FORMAT_R32G32_SNORM          = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RG01, 4, 4, 0, 0, PIPE_FORMAT_TYPE_SNORM ),
   PIPE_FORMAT_R32G32B32_SNORM       = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RGB1, 4, 4, 4, 0, PIPE_FORMAT_TYPE_SNORM ),
   PIPE_FORMAT_R32G32B32A32_SNORM    = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RGBA, 4, 4, 4, 4, PIPE_FORMAT_TYPE_SNORM ),
   PIPE_FORMAT_R32_SSCALED           = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_R001, 4, 0, 0, 0, PIPE_FORMAT_TYPE_SSCALED ),
   PIPE_FORMAT_R32G32_SSCALED        = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RG01, 4, 4, 0, 0, PIPE_FORMAT_TYPE_SSCALED ),
   PIPE_FORMAT_R32G32B32_SSCALED     = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RGB1, 4, 4, 4, 0, PIPE_FORMAT_TYPE_SSCALED ),
   PIPE_FORMAT_R32G32B32A32_SSCALED  = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RGBA, 4, 4, 4, 4, PIPE_FORMAT_TYPE_SSCALED ),
   PIPE_FORMAT_R16_UNORM             = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_R001, 2, 0, 0, 0, PIPE_FORMAT_TYPE_UNORM ),
   PIPE_FORMAT_R16G16_UNORM          = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RG01, 2, 2, 0, 0, PIPE_FORMAT_TYPE_UNORM ),
   PIPE_FORMAT_R16G16B16_UNORM       = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RGB1, 2, 2, 2, 0, PIPE_FORMAT_TYPE_UNORM ),
   PIPE_FORMAT_R16G16B16A16_UNORM    = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RGBA, 2, 2, 2, 2, PIPE_FORMAT_TYPE_UNORM ),
   PIPE_FORMAT_R16_USCALED           = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_R001, 2, 0, 0, 0, PIPE_FORMAT_TYPE_USCALED ),
   PIPE_FORMAT_R16G16_USCALED        = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RG01, 2, 2, 0, 0, PIPE_FORMAT_TYPE_USCALED ),
   PIPE_FORMAT_R16G16B16_USCALED     = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RGB1, 2, 2, 2, 0, PIPE_FORMAT_TYPE_USCALED ),
   PIPE_FORMAT_R16G16B16A16_USCALED  = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RGBA, 2, 2, 2, 2, PIPE_FORMAT_TYPE_USCALED ),
   PIPE_FORMAT_R16_SNORM             = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_R001, 2, 0, 0, 0, PIPE_FORMAT_TYPE_SNORM ),
   PIPE_FORMAT_R16G16_SNORM          = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RG01, 2, 2, 0, 0, PIPE_FORMAT_TYPE_SNORM ),
   PIPE_FORMAT_R16G16B16_SNORM       = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RGB1, 2, 2, 2, 0, PIPE_FORMAT_TYPE_SNORM ),
   PIPE_FORMAT_R16G16B16A16_SNORM    = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RGBA, 2, 2, 2, 2, PIPE_FORMAT_TYPE_SNORM ),
   PIPE_FORMAT_R16_SSCALED           = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_R001, 2, 0, 0, 0, PIPE_FORMAT_TYPE_SSCALED ),
   PIPE_FORMAT_R16G16_SSCALED        = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RG01, 2, 2, 0, 0, PIPE_FORMAT_TYPE_SSCALED ),
   PIPE_FORMAT_R16G16B16_SSCALED     = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RGB1, 2, 2, 2, 0, PIPE_FORMAT_TYPE_SSCALED ),
   PIPE_FORMAT_R16G16B16A16_SSCALED  = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RGBA, 2, 2, 2, 2, PIPE_FORMAT_TYPE_SSCALED ),
   PIPE_FORMAT_R8_UNORM              = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_R001, 1, 0, 0, 0, PIPE_FORMAT_TYPE_UNORM ),
   PIPE_FORMAT_R8G8_UNORM            = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RG01, 1, 1, 0, 0, PIPE_FORMAT_TYPE_UNORM ),
   PIPE_FORMAT_R8G8B8_UNORM          = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RGB1, 1, 1, 1, 0, PIPE_FORMAT_TYPE_UNORM ),
   PIPE_FORMAT_R8G8B8A8_UNORM        = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RGBA, 1, 1, 1, 1, PIPE_FORMAT_TYPE_UNORM ),
   PIPE_FORMAT_R8G8B8X8_UNORM        = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RGB1, 1, 1, 1, 1, PIPE_FORMAT_TYPE_UNORM ),
   PIPE_FORMAT_R8_USCALED            = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_R001, 1, 0, 0, 0, PIPE_FORMAT_TYPE_USCALED ),
   PIPE_FORMAT_R8G8_USCALED          = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RG01, 1, 1, 0, 0, PIPE_FORMAT_TYPE_USCALED ),
   PIPE_FORMAT_R8G8B8_USCALED        = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RGB1, 1, 1, 1, 0, PIPE_FORMAT_TYPE_USCALED ),
   PIPE_FORMAT_R8G8B8A8_USCALED      = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RGBA, 1, 1, 1, 1, PIPE_FORMAT_TYPE_USCALED ),
   PIPE_FORMAT_R8G8B8X8_USCALED      = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RGB1, 1, 1, 1, 1, PIPE_FORMAT_TYPE_USCALED ),
   PIPE_FORMAT_R8_SNORM              = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_R001, 1, 0, 0, 0, PIPE_FORMAT_TYPE_SNORM ),
   PIPE_FORMAT_R8G8_SNORM            = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RG01, 1, 1, 0, 0, PIPE_FORMAT_TYPE_SNORM ),
   PIPE_FORMAT_R8G8B8_SNORM          = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RGB1, 1, 1, 1, 0, PIPE_FORMAT_TYPE_SNORM ),
   PIPE_FORMAT_R8G8B8A8_SNORM        = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RGBA, 1, 1, 1, 1, PIPE_FORMAT_TYPE_SNORM ),
   PIPE_FORMAT_R8G8B8X8_SNORM        = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RGB1, 1, 1, 1, 1, PIPE_FORMAT_TYPE_SNORM ),
   PIPE_FORMAT_B6G5R5_SNORM          = _PIPE_FORMAT_RGBAZS_1 ( _PIPE_FORMAT_BGR1, 6, 5, 5, 0, PIPE_FORMAT_TYPE_SNORM ),
   PIPE_FORMAT_A8B8G8R8_SNORM        = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_BGRA, 1, 1, 1, 1, PIPE_FORMAT_TYPE_SNORM ),
   PIPE_FORMAT_X8B8G8R8_SNORM        = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RGB1, 1, 1, 1, 1, PIPE_FORMAT_TYPE_SNORM ),
   PIPE_FORMAT_R8_SSCALED            = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_R001, 1, 0, 0, 0, PIPE_FORMAT_TYPE_SSCALED ),
   PIPE_FORMAT_R8G8_SSCALED          = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RG01, 1, 1, 0, 0, PIPE_FORMAT_TYPE_SSCALED ),
   PIPE_FORMAT_R8G8B8_SSCALED        = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RGB1, 1, 1, 1, 0, PIPE_FORMAT_TYPE_SSCALED ),
   PIPE_FORMAT_R8G8B8A8_SSCALED      = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RGBA, 1, 1, 1, 1, PIPE_FORMAT_TYPE_SSCALED ),
   PIPE_FORMAT_R8G8B8X8_SSCALED      = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RGB1, 1, 1, 1, 1, PIPE_FORMAT_TYPE_SSCALED ),
   PIPE_FORMAT_R32_FIXED             = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_R001, 4, 0, 0, 0, PIPE_FORMAT_TYPE_FIXED ),
   PIPE_FORMAT_R32G32_FIXED          = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RG01, 4, 4, 0, 0, PIPE_FORMAT_TYPE_FIXED ),
   PIPE_FORMAT_R32G32B32_FIXED       = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RGB1, 4, 4, 4, 0, PIPE_FORMAT_TYPE_FIXED ),
   PIPE_FORMAT_R32G32B32A32_FIXED    = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RGBA, 4, 4, 4, 4, PIPE_FORMAT_TYPE_FIXED ),
   /* sRGB formats */
   PIPE_FORMAT_L8_SRGB               = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RRR1, 1, 1, 1, 0, PIPE_FORMAT_TYPE_SRGB ),
   PIPE_FORMAT_A8_L8_SRGB            = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RRRG, 1, 1, 1, 1, PIPE_FORMAT_TYPE_SRGB ),
   PIPE_FORMAT_R8G8B8_SRGB           = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RGB1, 1, 1, 1, 0, PIPE_FORMAT_TYPE_SRGB ),
   PIPE_FORMAT_R8G8B8A8_SRGB         = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RGBA, 1, 1, 1, 1, PIPE_FORMAT_TYPE_SRGB ),
   PIPE_FORMAT_R8G8B8X8_SRGB         = _PIPE_FORMAT_RGBAZS_8 ( _PIPE_FORMAT_RGB1, 1, 1, 1, 1, PIPE_FORMAT_TYPE_SRGB ),

   /* mixed formats */
   PIPE_FORMAT_X8UB8UG8SR8S_NORM     = _PIPE_FORMAT_MIXED( _PIPE_FORMAT_1BGR, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1 ),
   PIPE_FORMAT_B6UG5SR5S_NORM        = _PIPE_FORMAT_MIXED( _PIPE_FORMAT_BGR1, 6, 5, 5, 0, 0, 1, 1, 0, 1, 0 ),

   /* compressed formats */
   PIPE_FORMAT_DXT1_RGB              = _PIPE_FORMAT_DXT( 1, 8, 8, 8, 0 ),
   PIPE_FORMAT_DXT1_RGBA             = _PIPE_FORMAT_DXT( 1, 8, 8, 8, 8 ),
   PIPE_FORMAT_DXT3_RGBA             = _PIPE_FORMAT_DXT( 3, 8, 8, 8, 8 ),
   PIPE_FORMAT_DXT5_RGBA             = _PIPE_FORMAT_DXT( 5, 8, 8, 8, 8 )
};

/**
 * Builds pipe format name from format token.
 */
extern const char *pf_name( enum pipe_format format );

/**
 * Return bits for a particular component.
 * \param comp  component index, starting at 0
 */
static INLINE uint pf_get_component_bits( enum pipe_format format, uint comp )
{
   uint size;

   if (pf_swizzle_x(format) == comp) {
      size = pf_size_x(format);
   }
   else if (pf_swizzle_y(format) == comp) {
      size = pf_size_y(format);
   }
   else if (pf_swizzle_z(format) == comp) {
      size = pf_size_z(format);
   }
   else if (pf_swizzle_w(format) == comp) {
      size = pf_size_w(format);
   }
   else {
      size = 0;
   }
   if (pf_layout( format ) == PIPE_FORMAT_LAYOUT_RGBAZS)
      return size << pf_exp2( format );
   return size << (pf_mixed_scale8( format ) * 3);
}

/**
 * Return total bits needed for the pixel format.
 */
static INLINE uint pf_get_bits( enum pipe_format format )
{
   switch (pf_layout(format)) {
   case PIPE_FORMAT_LAYOUT_RGBAZS:
   case PIPE_FORMAT_LAYOUT_MIXED:
      return
         pf_get_component_bits( format, PIPE_FORMAT_COMP_0 ) +
         pf_get_component_bits( format, PIPE_FORMAT_COMP_1 ) +
         pf_get_component_bits( format, PIPE_FORMAT_COMP_R ) +
         pf_get_component_bits( format, PIPE_FORMAT_COMP_G ) +
         pf_get_component_bits( format, PIPE_FORMAT_COMP_B ) +
         pf_get_component_bits( format, PIPE_FORMAT_COMP_A ) +
         pf_get_component_bits( format, PIPE_FORMAT_COMP_Z ) +
         pf_get_component_bits( format, PIPE_FORMAT_COMP_S );
   case PIPE_FORMAT_LAYOUT_YCBCR:
      assert( format == PIPE_FORMAT_YCBCR || format == PIPE_FORMAT_YCBCR_REV );
      /* return effective bits per pixel */
      return 16; 
   default:
      assert( 0 );
      return 0;
   }
}

/**
 * Return bytes per pixel for the given format.
 */
static INLINE uint pf_get_size( enum pipe_format format )
{
   assert(pf_get_bits(format) % 8 == 0);
   return pf_get_bits(format) / 8;
}

/**
 * Describe accurately the pixel format.
 * 
 * The chars-per-pixel concept falls apart with compressed and yuv images, where
 * more than one pixel are coded in a single data block. This structure 
 * describes that block.
 * 
 * Simple pixel formats are effectively a 1x1xcpp block.
 */
struct pipe_format_block
{
   /** Block size in bytes */
   unsigned size;
   
   /** Block width in pixels */
   unsigned width;
   
   /** Block height in pixels */
   unsigned height;
};

/**
 * Describe pixel format's block.   
 * 
 * @sa http://msdn2.microsoft.com/en-us/library/ms796147.aspx
 */
static INLINE void 
pf_get_block(enum pipe_format format, struct pipe_format_block *block)
{
   switch(format) {
   case PIPE_FORMAT_DXT1_RGBA:
   case PIPE_FORMAT_DXT1_RGB:
      block->size = 8;
      block->width = 4;
      block->height = 4;
      break;
   case PIPE_FORMAT_DXT3_RGBA:
   case PIPE_FORMAT_DXT5_RGBA:
      block->size = 16;
      block->width = 4;
      block->height = 4;
      break;
   case PIPE_FORMAT_YCBCR:
   case PIPE_FORMAT_YCBCR_REV:
      block->size = 4; /* 2*cpp */
      block->width = 2;
      block->height = 1;
      break;
   default:
      block->size = pf_get_size(format);
      block->width = 1;
      block->height = 1;
      break;
   }
}

static INLINE unsigned
pf_get_nblocksx(const struct pipe_format_block *block, unsigned x)
{
   return (x + block->width - 1)/block->width;
}

static INLINE unsigned
pf_get_nblocksy(const struct pipe_format_block *block, unsigned y)
{
   return (y + block->height - 1)/block->height;
}

static INLINE unsigned
pf_get_nblocks(const struct pipe_format_block *block, unsigned width, unsigned height)
{
   return pf_get_nblocksx(block, width)*pf_get_nblocksy(block, height);
}

static INLINE boolean 
pf_is_compressed( enum pipe_format format )
{
   return pf_layout(format) == PIPE_FORMAT_LAYOUT_DXT ? TRUE : FALSE;
}

static INLINE boolean 
pf_is_ycbcr( enum pipe_format format )
{
   return pf_layout(format) == PIPE_FORMAT_LAYOUT_YCBCR ? TRUE : FALSE;
}

static INLINE boolean 
pf_has_alpha( enum pipe_format format )
{
   switch (pf_layout(format)) {
   case PIPE_FORMAT_LAYOUT_RGBAZS:
   case PIPE_FORMAT_LAYOUT_MIXED:
      /* FIXME: pf_get_component_bits( PIPE_FORMAT_A8L8_UNORM, PIPE_FORMAT_COMP_A ) should not return 0 right? */
      if(format == PIPE_FORMAT_A8_UNORM || 
         format == PIPE_FORMAT_A8L8_UNORM || 
         format == PIPE_FORMAT_A8_L8_SRGB)
         return TRUE;
      return pf_get_component_bits( format, PIPE_FORMAT_COMP_A ) ? TRUE : FALSE;
   case PIPE_FORMAT_LAYOUT_YCBCR:
      return FALSE; 
   case PIPE_FORMAT_LAYOUT_DXT:
      switch (format) {
      case PIPE_FORMAT_DXT1_RGBA:
      case PIPE_FORMAT_DXT3_RGBA:
      case PIPE_FORMAT_DXT5_RGBA:
         return TRUE;
      default:
         return FALSE;
      }
   default:
      assert( 0 );
      return FALSE;
   }
}

#ifdef __cplusplus
}
#endif

#endif

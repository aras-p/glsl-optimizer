/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * Copyright (c) 2008 VMware, Inc.
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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Texture/surface image formats (preliminary)
 */

/* KW: Added lots of surface formats to support vertex element layout
 * definitions, and eventually render-to-vertex-buffer.
 */

enum pipe_format {
   PIPE_FORMAT_NONE                  = 0,
   PIPE_FORMAT_B8G8R8A8_UNORM        = 1,
   PIPE_FORMAT_B8G8R8X8_UNORM        = 2,
   PIPE_FORMAT_A8R8G8B8_UNORM        = 3,
   PIPE_FORMAT_X8R8G8B8_UNORM        = 4,
   PIPE_FORMAT_B5G5R5A1_UNORM        = 5,
   PIPE_FORMAT_B4G4R4A4_UNORM        = 6,
   PIPE_FORMAT_B5G6R5_UNORM          = 7,
   PIPE_FORMAT_R10G10B10A2_UNORM     = 8,
   PIPE_FORMAT_L8_UNORM              = 9,    /**< ubyte luminance */
   PIPE_FORMAT_A8_UNORM              = 10,   /**< ubyte alpha */
   PIPE_FORMAT_I8_UNORM              = 11,   /**< ubyte intensity */
   PIPE_FORMAT_L8A8_UNORM            = 12,   /**< ubyte alpha, luminance */
   PIPE_FORMAT_L16_UNORM             = 13,   /**< ushort luminance */
   PIPE_FORMAT_UYVY                  = 14,
   PIPE_FORMAT_YUYV                  = 15,
   PIPE_FORMAT_Z16_UNORM             = 16,
   PIPE_FORMAT_Z32_UNORM             = 17,
   PIPE_FORMAT_Z32_FLOAT             = 18,
   PIPE_FORMAT_Z24S8_UNORM           = 19,
   PIPE_FORMAT_S8Z24_UNORM           = 20,
   PIPE_FORMAT_Z24X8_UNORM           = 21,
   PIPE_FORMAT_X8Z24_UNORM           = 22,
   PIPE_FORMAT_S8_UNORM              = 23,   /**< ubyte stencil */
   PIPE_FORMAT_R64_FLOAT             = 24,
   PIPE_FORMAT_R64G64_FLOAT          = 25,
   PIPE_FORMAT_R64G64B64_FLOAT       = 26,
   PIPE_FORMAT_R64G64B64A64_FLOAT    = 27,
   PIPE_FORMAT_R32_FLOAT             = 28,
   PIPE_FORMAT_R32G32_FLOAT          = 29,
   PIPE_FORMAT_R32G32B32_FLOAT       = 30,
   PIPE_FORMAT_R32G32B32A32_FLOAT    = 31,
   PIPE_FORMAT_R32_UNORM             = 32,
   PIPE_FORMAT_R32G32_UNORM          = 33,
   PIPE_FORMAT_R32G32B32_UNORM       = 34,
   PIPE_FORMAT_R32G32B32A32_UNORM    = 35,
   PIPE_FORMAT_R32_USCALED           = 36,
   PIPE_FORMAT_R32G32_USCALED        = 37,
   PIPE_FORMAT_R32G32B32_USCALED     = 38,
   PIPE_FORMAT_R32G32B32A32_USCALED  = 39,
   PIPE_FORMAT_R32_SNORM             = 40,
   PIPE_FORMAT_R32G32_SNORM          = 41,
   PIPE_FORMAT_R32G32B32_SNORM       = 42,
   PIPE_FORMAT_R32G32B32A32_SNORM    = 43,
   PIPE_FORMAT_R32_SSCALED           = 44,
   PIPE_FORMAT_R32G32_SSCALED        = 45,
   PIPE_FORMAT_R32G32B32_SSCALED     = 46,
   PIPE_FORMAT_R32G32B32A32_SSCALED  = 47,
   PIPE_FORMAT_R16_UNORM             = 48,
   PIPE_FORMAT_R16G16_UNORM          = 49,
   PIPE_FORMAT_R16G16B16_UNORM       = 50,
   PIPE_FORMAT_R16G16B16A16_UNORM    = 51,
   PIPE_FORMAT_R16_USCALED           = 52,
   PIPE_FORMAT_R16G16_USCALED        = 53,
   PIPE_FORMAT_R16G16B16_USCALED     = 54,
   PIPE_FORMAT_R16G16B16A16_USCALED  = 55,
   PIPE_FORMAT_R16_SNORM             = 56,
   PIPE_FORMAT_R16G16_SNORM          = 57,
   PIPE_FORMAT_R16G16B16_SNORM       = 58,
   PIPE_FORMAT_R16G16B16A16_SNORM    = 59,
   PIPE_FORMAT_R16_SSCALED           = 60,
   PIPE_FORMAT_R16G16_SSCALED        = 61,
   PIPE_FORMAT_R16G16B16_SSCALED     = 62,
   PIPE_FORMAT_R16G16B16A16_SSCALED  = 63,
   PIPE_FORMAT_R8_UNORM              = 64,
   PIPE_FORMAT_R8G8_UNORM            = 65,
   PIPE_FORMAT_R8G8B8_UNORM          = 66,
   PIPE_FORMAT_R8G8B8A8_UNORM        = 67,
   PIPE_FORMAT_X8B8G8R8_UNORM        = 68,
   PIPE_FORMAT_R8_USCALED            = 69,
   PIPE_FORMAT_R8G8_USCALED          = 70,
   PIPE_FORMAT_R8G8B8_USCALED        = 71,
   PIPE_FORMAT_R8G8B8A8_USCALED      = 72,
   PIPE_FORMAT_R8_SNORM              = 74,
   PIPE_FORMAT_R8G8_SNORM            = 75,
   PIPE_FORMAT_R8G8B8_SNORM          = 76,
   PIPE_FORMAT_R8G8B8A8_SNORM        = 77,
   PIPE_FORMAT_R8_SSCALED            = 82,
   PIPE_FORMAT_R8G8_SSCALED          = 83,
   PIPE_FORMAT_R8G8B8_SSCALED        = 84,
   PIPE_FORMAT_R8G8B8A8_SSCALED      = 85,
   PIPE_FORMAT_R32_FIXED             = 87,
   PIPE_FORMAT_R32G32_FIXED          = 88,
   PIPE_FORMAT_R32G32B32_FIXED       = 89,
   PIPE_FORMAT_R32G32B32A32_FIXED    = 90,
   /* sRGB formats */
   PIPE_FORMAT_L8_SRGB               = 91,
   PIPE_FORMAT_L8A8_SRGB             = 92,
   PIPE_FORMAT_R8G8B8_SRGB           = 93,
   PIPE_FORMAT_A8B8G8R8_SRGB         = 94,
   PIPE_FORMAT_X8B8G8R8_SRGB         = 95,
   PIPE_FORMAT_B8G8R8A8_SRGB         = 96,
   PIPE_FORMAT_B8G8R8X8_SRGB         = 97,
   PIPE_FORMAT_A8R8G8B8_SRGB         = 98,
   PIPE_FORMAT_X8R8G8B8_SRGB         = 99,

   /* mixed formats */
   PIPE_FORMAT_R8SG8SB8UX8U_NORM     = 100,
   PIPE_FORMAT_R5SG5SB6U_NORM        = 101,

   /* compressed formats */
   PIPE_FORMAT_DXT1_RGB              = 102,
   PIPE_FORMAT_DXT1_RGBA             = 103,
   PIPE_FORMAT_DXT3_RGBA             = 104,
   PIPE_FORMAT_DXT5_RGBA             = 105,

   /* sRGB, compressed */
   PIPE_FORMAT_DXT1_SRGB             = 106,
   PIPE_FORMAT_DXT1_SRGBA            = 107,
   PIPE_FORMAT_DXT3_SRGBA            = 108,
   PIPE_FORMAT_DXT5_SRGBA            = 109,

   PIPE_FORMAT_A8B8G8R8_UNORM        = 110,

   PIPE_FORMAT_COUNT
};


enum pipe_video_chroma_format
{
   PIPE_VIDEO_CHROMA_FORMAT_420,
   PIPE_VIDEO_CHROMA_FORMAT_422,
   PIPE_VIDEO_CHROMA_FORMAT_444
};

#if 0
enum pipe_video_surface_format
{
   PIPE_VIDEO_SURFACE_FORMAT_NV12,  /**< Planar; Y plane, UV plane */
   PIPE_VIDEO_SURFACE_FORMAT_YV12,  /**< Planar; Y plane, U plane, V plane */
   PIPE_VIDEO_SURFACE_FORMAT_YUYV,  /**< Interleaved; Y,U,Y,V,Y,U,Y,V */
   PIPE_VIDEO_SURFACE_FORMAT_UYVY,  /**< Interleaved; U,Y,V,Y,U,Y,V,Y */
   PIPE_VIDEO_SURFACE_FORMAT_VUYA   /**< Packed; A31-24|Y23-16|U15-8|V7-0 */
};
#endif

#ifdef __cplusplus
}
#endif

#endif

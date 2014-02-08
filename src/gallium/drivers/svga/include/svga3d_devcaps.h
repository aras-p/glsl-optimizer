/**********************************************************
 * Copyright 1998-2014 VMware, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************/

/*
 * svga3d_devcaps.h --
 *
 *       SVGA 3d caps definitions
 */

#ifndef _SVGA3D_DEVCAPS_H_
#define _SVGA3D_DEVCAPS_H_

#define INCLUDE_ALLOW_MODULE
#define INCLUDE_ALLOW_USERLEVEL
#define INCLUDE_ALLOW_VMCORE

#include "includeCheck.h"

/*
 * 3D Hardware Version
 *
 *   The hardware version is stored in the SVGA_FIFO_3D_HWVERSION fifo
 *   register.   Is set by the host and read by the guest.  This lets
 *   us make new guest drivers which are backwards-compatible with old
 *   SVGA hardware revisions.  It does not let us support old guest
 *   drivers.  Good enough for now.
 *
 */

#define SVGA3D_MAKE_HWVERSION(major, minor)      (((major) << 16) | ((minor) & 0xFF))
#define SVGA3D_MAJOR_HWVERSION(version)          ((version) >> 16)
#define SVGA3D_MINOR_HWVERSION(version)          ((version) & 0xFF)

typedef enum {
   SVGA3D_HWVERSION_WS5_RC1   = SVGA3D_MAKE_HWVERSION(0, 1),
   SVGA3D_HWVERSION_WS5_RC2   = SVGA3D_MAKE_HWVERSION(0, 2),
   SVGA3D_HWVERSION_WS51_RC1  = SVGA3D_MAKE_HWVERSION(0, 3),
   SVGA3D_HWVERSION_WS6_B1    = SVGA3D_MAKE_HWVERSION(1, 1),
   SVGA3D_HWVERSION_FUSION_11 = SVGA3D_MAKE_HWVERSION(1, 4),
   SVGA3D_HWVERSION_WS65_B1   = SVGA3D_MAKE_HWVERSION(2, 0),
   SVGA3D_HWVERSION_WS8_B1    = SVGA3D_MAKE_HWVERSION(2, 1),
   SVGA3D_HWVERSION_CURRENT   = SVGA3D_HWVERSION_WS8_B1,
} SVGA3dHardwareVersion;

/*
 * DevCap indexes.
 */

typedef enum {
   SVGA3D_DEVCAP_INVALID                           = ((uint32)-1),
   SVGA3D_DEVCAP_3D                                = 0,
   SVGA3D_DEVCAP_MAX_LIGHTS                        = 1,

   /*
    * SVGA3D_DEVCAP_MAX_TEXTURES reflects the maximum number of
    * fixed-function texture units available. Each of these units
    * work in both FFP and Shader modes, and they support texture
    * transforms and texture coordinates. The host may have additional
    * texture image units that are only usable with shaders.
    */
   SVGA3D_DEVCAP_MAX_TEXTURES                      = 2,
   SVGA3D_DEVCAP_MAX_CLIP_PLANES                   = 3,
   SVGA3D_DEVCAP_VERTEX_SHADER_VERSION             = 4,
   SVGA3D_DEVCAP_VERTEX_SHADER                     = 5,
   SVGA3D_DEVCAP_FRAGMENT_SHADER_VERSION           = 6,
   SVGA3D_DEVCAP_FRAGMENT_SHADER                   = 7,
   SVGA3D_DEVCAP_MAX_RENDER_TARGETS                = 8,
   SVGA3D_DEVCAP_S23E8_TEXTURES                    = 9,
   SVGA3D_DEVCAP_S10E5_TEXTURES                    = 10,
   SVGA3D_DEVCAP_MAX_FIXED_VERTEXBLEND             = 11,
   SVGA3D_DEVCAP_D16_BUFFER_FORMAT                 = 12,
   SVGA3D_DEVCAP_D24S8_BUFFER_FORMAT               = 13,
   SVGA3D_DEVCAP_D24X8_BUFFER_FORMAT               = 14,
   SVGA3D_DEVCAP_QUERY_TYPES                       = 15,
   SVGA3D_DEVCAP_TEXTURE_GRADIENT_SAMPLING         = 16,
   SVGA3D_DEVCAP_MAX_POINT_SIZE                    = 17,
   SVGA3D_DEVCAP_MAX_SHADER_TEXTURES               = 18,
   SVGA3D_DEVCAP_MAX_TEXTURE_WIDTH                 = 19,
   SVGA3D_DEVCAP_MAX_TEXTURE_HEIGHT                = 20,
   SVGA3D_DEVCAP_MAX_VOLUME_EXTENT                 = 21,
   SVGA3D_DEVCAP_MAX_TEXTURE_REPEAT                = 22,
   SVGA3D_DEVCAP_MAX_TEXTURE_ASPECT_RATIO          = 23,
   SVGA3D_DEVCAP_MAX_TEXTURE_ANISOTROPY            = 24,
   SVGA3D_DEVCAP_MAX_PRIMITIVE_COUNT               = 25,
   SVGA3D_DEVCAP_MAX_VERTEX_INDEX                  = 26,
   SVGA3D_DEVCAP_MAX_VERTEX_SHADER_INSTRUCTIONS    = 27,
   SVGA3D_DEVCAP_MAX_FRAGMENT_SHADER_INSTRUCTIONS  = 28,
   SVGA3D_DEVCAP_MAX_VERTEX_SHADER_TEMPS           = 29,
   SVGA3D_DEVCAP_MAX_FRAGMENT_SHADER_TEMPS         = 30,
   SVGA3D_DEVCAP_TEXTURE_OPS                       = 31,
   SVGA3D_DEVCAP_SURFACEFMT_X8R8G8B8               = 32,
   SVGA3D_DEVCAP_SURFACEFMT_A8R8G8B8               = 33,
   SVGA3D_DEVCAP_SURFACEFMT_A2R10G10B10            = 34,
   SVGA3D_DEVCAP_SURFACEFMT_X1R5G5B5               = 35,
   SVGA3D_DEVCAP_SURFACEFMT_A1R5G5B5               = 36,
   SVGA3D_DEVCAP_SURFACEFMT_A4R4G4B4               = 37,
   SVGA3D_DEVCAP_SURFACEFMT_R5G6B5                 = 38,
   SVGA3D_DEVCAP_SURFACEFMT_LUMINANCE16            = 39,
   SVGA3D_DEVCAP_SURFACEFMT_LUMINANCE8_ALPHA8      = 40,
   SVGA3D_DEVCAP_SURFACEFMT_ALPHA8                 = 41,
   SVGA3D_DEVCAP_SURFACEFMT_LUMINANCE8             = 42,
   SVGA3D_DEVCAP_SURFACEFMT_Z_D16                  = 43,
   SVGA3D_DEVCAP_SURFACEFMT_Z_D24S8                = 44,
   SVGA3D_DEVCAP_SURFACEFMT_Z_D24X8                = 45,
   SVGA3D_DEVCAP_SURFACEFMT_DXT1                   = 46,
   SVGA3D_DEVCAP_SURFACEFMT_DXT2                   = 47,
   SVGA3D_DEVCAP_SURFACEFMT_DXT3                   = 48,
   SVGA3D_DEVCAP_SURFACEFMT_DXT4                   = 49,
   SVGA3D_DEVCAP_SURFACEFMT_DXT5                   = 50,
   SVGA3D_DEVCAP_SURFACEFMT_BUMPX8L8V8U8           = 51,
   SVGA3D_DEVCAP_SURFACEFMT_A2W10V10U10            = 52,
   SVGA3D_DEVCAP_SURFACEFMT_BUMPU8V8               = 53,
   SVGA3D_DEVCAP_SURFACEFMT_Q8W8V8U8               = 54,
   SVGA3D_DEVCAP_SURFACEFMT_CxV8U8                 = 55,
   SVGA3D_DEVCAP_SURFACEFMT_R_S10E5                = 56,
   SVGA3D_DEVCAP_SURFACEFMT_R_S23E8                = 57,
   SVGA3D_DEVCAP_SURFACEFMT_RG_S10E5               = 58,
   SVGA3D_DEVCAP_SURFACEFMT_RG_S23E8               = 59,
   SVGA3D_DEVCAP_SURFACEFMT_ARGB_S10E5             = 60,
   SVGA3D_DEVCAP_SURFACEFMT_ARGB_S23E8             = 61,

   /*
    * There is a hole in our devcap definitions for
    * historical reasons.
    *
    * Define a constant just for completeness.
    */
   SVGA3D_DEVCAP_MISSING62                         = 62,

   SVGA3D_DEVCAP_MAX_VERTEX_SHADER_TEXTURES        = 63,

   /*
    * Note that MAX_SIMULTANEOUS_RENDER_TARGETS is a maximum count of color
    * render targets.  This does not include the depth or stencil targets.
    */
   SVGA3D_DEVCAP_MAX_SIMULTANEOUS_RENDER_TARGETS   = 64,

   SVGA3D_DEVCAP_SURFACEFMT_V16U16                 = 65,
   SVGA3D_DEVCAP_SURFACEFMT_G16R16                 = 66,
   SVGA3D_DEVCAP_SURFACEFMT_A16B16G16R16           = 67,
   SVGA3D_DEVCAP_SURFACEFMT_UYVY                   = 68,
   SVGA3D_DEVCAP_SURFACEFMT_YUY2                   = 69,
   SVGA3D_DEVCAP_MULTISAMPLE_NONMASKABLESAMPLES    = 70,
   SVGA3D_DEVCAP_MULTISAMPLE_MASKABLESAMPLES       = 71,
   SVGA3D_DEVCAP_ALPHATOCOVERAGE                   = 72,
   SVGA3D_DEVCAP_SUPERSAMPLE                       = 73,
   SVGA3D_DEVCAP_AUTOGENMIPMAPS                    = 74,
   SVGA3D_DEVCAP_SURFACEFMT_NV12                   = 75,
   SVGA3D_DEVCAP_SURFACEFMT_AYUV                   = 76,

   /*
    * This is the maximum number of SVGA context IDs that the guest
    * can define using SVGA_3D_CMD_CONTEXT_DEFINE.
    */
   SVGA3D_DEVCAP_MAX_CONTEXT_IDS                   = 77,

   /*
    * This is the maximum number of SVGA surface IDs that the guest
    * can define using SVGA_3D_CMD_SURFACE_DEFINE*.
    */
   SVGA3D_DEVCAP_MAX_SURFACE_IDS                   = 78,

   SVGA3D_DEVCAP_SURFACEFMT_Z_DF16                 = 79,
   SVGA3D_DEVCAP_SURFACEFMT_Z_DF24                 = 80,
   SVGA3D_DEVCAP_SURFACEFMT_Z_D24S8_INT            = 81,

   SVGA3D_DEVCAP_SURFACEFMT_ATI1                   = 82,
   SVGA3D_DEVCAP_SURFACEFMT_ATI2                   = 83,

   /*
    * Deprecated.
    */
   SVGA3D_DEVCAP_DEAD1                             = 84,

   /*
    * This contains several SVGA_3D_CAPS_VIDEO_DECODE elements
    * ored together, one for every type of video decoding supported.
    */
   SVGA3D_DEVCAP_VIDEO_DECODE                      = 85,

   /*
    * This contains several SVGA_3D_CAPS_VIDEO_PROCESS elements
    * ored together, one for every type of video processing supported.
    */
   SVGA3D_DEVCAP_VIDEO_PROCESS                     = 86,

   SVGA3D_DEVCAP_LINE_AA                           = 87,  /* boolean */
   SVGA3D_DEVCAP_LINE_STIPPLE                      = 88,  /* boolean */
   SVGA3D_DEVCAP_MAX_LINE_WIDTH                    = 89,  /* float */
   SVGA3D_DEVCAP_MAX_AA_LINE_WIDTH                 = 90,  /* float */

   SVGA3D_DEVCAP_SURFACEFMT_YV12                   = 91,

   /*
    * Does the host support the SVGA logic ops commands?
    */
   SVGA3D_DEVCAP_LOGICOPS                          = 92,

   /*
    * Are TS_CONSTANT, TS_COLOR_KEY, and TS_COLOR_KEY_ENABLE supported?
    */
   SVGA3D_DEVCAP_TS_COLOR_KEY                      = 93, /* boolean */

   SVGA3D_DEVCAP_MAX                       /* This must be the last index. */
} SVGA3dDevCapIndex;

typedef union {
   Bool   b;
   uint32 u;
   int32  i;
   float  f;
} SVGA3dDevCapResult;

#endif // _SVGA3D_DEVCAPS_H_

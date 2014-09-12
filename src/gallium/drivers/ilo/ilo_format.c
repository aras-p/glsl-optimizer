/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2012-2013 LunarG, Inc.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#include "genhw/genhw.h"
#include "vl/vl_video_buffer.h"

#include "ilo_screen.h"
#include "ilo_format.h"

struct ilo_vf_cap {
   int vertex_element;
};

struct ilo_sol_cap {
   int buffer;
};

struct ilo_sampler_cap {
   int sampling;
   int filtering;
   int shadow_map;
   int chroma_key;
};

struct ilo_dp_cap {
   int rt_write;
   int rt_write_blending;
   int typed_write;
   int media_color_processing;
};

/*
 * This table is based on:
 *
 *  - the Sandy Bridge PRM, volume 4 part 1, page 88-97
 *  - the Ivy Bridge PRM, volume 2 part 1, page 97-99
 *  - the Haswell PRM, volume 7, page 467-470
 */
static const struct ilo_vf_cap ilo_vf_caps[] = {
#define CAP(vertex_element) { ILO_GEN(vertex_element) }
   [GEN6_FORMAT_R32G32B32A32_FLOAT]       = CAP(  1),
   [GEN6_FORMAT_R32G32B32A32_SINT]        = CAP(  1),
   [GEN6_FORMAT_R32G32B32A32_UINT]        = CAP(  1),
   [GEN6_FORMAT_R32G32B32A32_UNORM]       = CAP(  1),
   [GEN6_FORMAT_R32G32B32A32_SNORM]       = CAP(  1),
   [GEN6_FORMAT_R64G64_FLOAT]             = CAP(  1),
   [GEN6_FORMAT_R32G32B32A32_SSCALED]     = CAP(  1),
   [GEN6_FORMAT_R32G32B32A32_USCALED]     = CAP(  1),
   [GEN6_FORMAT_R32G32B32A32_SFIXED]      = CAP(7.5),
   [GEN6_FORMAT_R32G32B32_FLOAT]          = CAP(  1),
   [GEN6_FORMAT_R32G32B32_SINT]           = CAP(  1),
   [GEN6_FORMAT_R32G32B32_UINT]           = CAP(  1),
   [GEN6_FORMAT_R32G32B32_UNORM]          = CAP(  1),
   [GEN6_FORMAT_R32G32B32_SNORM]          = CAP(  1),
   [GEN6_FORMAT_R32G32B32_SSCALED]        = CAP(  1),
   [GEN6_FORMAT_R32G32B32_USCALED]        = CAP(  1),
   [GEN6_FORMAT_R32G32B32_SFIXED]         = CAP(7.5),
   [GEN6_FORMAT_R16G16B16A16_UNORM]       = CAP(  1),
   [GEN6_FORMAT_R16G16B16A16_SNORM]       = CAP(  1),
   [GEN6_FORMAT_R16G16B16A16_SINT]        = CAP(  1),
   [GEN6_FORMAT_R16G16B16A16_UINT]        = CAP(  1),
   [GEN6_FORMAT_R16G16B16A16_FLOAT]       = CAP(  1),
   [GEN6_FORMAT_R32G32_FLOAT]             = CAP(  1),
   [GEN6_FORMAT_R32G32_SINT]              = CAP(  1),
   [GEN6_FORMAT_R32G32_UINT]              = CAP(  1),
   [GEN6_FORMAT_R32G32_UNORM]             = CAP(  1),
   [GEN6_FORMAT_R32G32_SNORM]             = CAP(  1),
   [GEN6_FORMAT_R64_FLOAT]                = CAP(  1),
   [GEN6_FORMAT_R16G16B16A16_SSCALED]     = CAP(  1),
   [GEN6_FORMAT_R16G16B16A16_USCALED]     = CAP(  1),
   [GEN6_FORMAT_R32G32_SSCALED]           = CAP(  1),
   [GEN6_FORMAT_R32G32_USCALED]           = CAP(  1),
   [GEN6_FORMAT_R32G32_SFIXED]            = CAP(7.5),
   [GEN6_FORMAT_B8G8R8A8_UNORM]           = CAP(  1),
   [GEN6_FORMAT_R10G10B10A2_UNORM]        = CAP(  1),
   [GEN6_FORMAT_R10G10B10A2_UINT]         = CAP(  1),
   [GEN6_FORMAT_R10G10B10_SNORM_A2_UNORM] = CAP(  1),
   [GEN6_FORMAT_R8G8B8A8_UNORM]           = CAP(  1),
   [GEN6_FORMAT_R8G8B8A8_SNORM]           = CAP(  1),
   [GEN6_FORMAT_R8G8B8A8_SINT]            = CAP(  1),
   [GEN6_FORMAT_R8G8B8A8_UINT]            = CAP(  1),
   [GEN6_FORMAT_R16G16_UNORM]             = CAP(  1),
   [GEN6_FORMAT_R16G16_SNORM]             = CAP(  1),
   [GEN6_FORMAT_R16G16_SINT]              = CAP(  1),
   [GEN6_FORMAT_R16G16_UINT]              = CAP(  1),
   [GEN6_FORMAT_R16G16_FLOAT]             = CAP(  1),
   [GEN6_FORMAT_B10G10R10A2_UNORM]        = CAP(7.5),
   [GEN6_FORMAT_R11G11B10_FLOAT]          = CAP(  1),
   [GEN6_FORMAT_R32_SINT]                 = CAP(  1),
   [GEN6_FORMAT_R32_UINT]                 = CAP(  1),
   [GEN6_FORMAT_R32_FLOAT]                = CAP(  1),
   [GEN6_FORMAT_R32_UNORM]                = CAP(  1),
   [GEN6_FORMAT_R32_SNORM]                = CAP(  1),
   [GEN6_FORMAT_R10G10B10X2_USCALED]      = CAP(  1),
   [GEN6_FORMAT_R8G8B8A8_SSCALED]         = CAP(  1),
   [GEN6_FORMAT_R8G8B8A8_USCALED]         = CAP(  1),
   [GEN6_FORMAT_R16G16_SSCALED]           = CAP(  1),
   [GEN6_FORMAT_R16G16_USCALED]           = CAP(  1),
   [GEN6_FORMAT_R32_SSCALED]              = CAP(  1),
   [GEN6_FORMAT_R32_USCALED]              = CAP(  1),
   [GEN6_FORMAT_R8G8_UNORM]               = CAP(  1),
   [GEN6_FORMAT_R8G8_SNORM]               = CAP(  1),
   [GEN6_FORMAT_R8G8_SINT]                = CAP(  1),
   [GEN6_FORMAT_R8G8_UINT]                = CAP(  1),
   [GEN6_FORMAT_R16_UNORM]                = CAP(  1),
   [GEN6_FORMAT_R16_SNORM]                = CAP(  1),
   [GEN6_FORMAT_R16_SINT]                 = CAP(  1),
   [GEN6_FORMAT_R16_UINT]                 = CAP(  1),
   [GEN6_FORMAT_R16_FLOAT]                = CAP(  1),
   [GEN6_FORMAT_R8G8_SSCALED]             = CAP(  1),
   [GEN6_FORMAT_R8G8_USCALED]             = CAP(  1),
   [GEN6_FORMAT_R16_SSCALED]              = CAP(  1),
   [GEN6_FORMAT_R16_USCALED]              = CAP(  1),
   [GEN6_FORMAT_R8_UNORM]                 = CAP(  1),
   [GEN6_FORMAT_R8_SNORM]                 = CAP(  1),
   [GEN6_FORMAT_R8_SINT]                  = CAP(  1),
   [GEN6_FORMAT_R8_UINT]                  = CAP(  1),
   [GEN6_FORMAT_R8_SSCALED]               = CAP(  1),
   [GEN6_FORMAT_R8_USCALED]               = CAP(  1),
   [GEN6_FORMAT_R8G8B8_UNORM]             = CAP(  1),
   [GEN6_FORMAT_R8G8B8_SNORM]             = CAP(  1),
   [GEN6_FORMAT_R8G8B8_SSCALED]           = CAP(  1),
   [GEN6_FORMAT_R8G8B8_USCALED]           = CAP(  1),
   [GEN6_FORMAT_R64G64B64A64_FLOAT]       = CAP(  1),
   [GEN6_FORMAT_R64G64B64_FLOAT]          = CAP(  1),
   [GEN6_FORMAT_R16G16B16_FLOAT]          = CAP(  6),
   [GEN6_FORMAT_R16G16B16_UNORM]          = CAP(  1),
   [GEN6_FORMAT_R16G16B16_SNORM]          = CAP(  1),
   [GEN6_FORMAT_R16G16B16_SSCALED]        = CAP(  1),
   [GEN6_FORMAT_R16G16B16_USCALED]        = CAP(  1),
   [GEN6_FORMAT_R16G16B16_UINT]           = CAP(7.5),
   [GEN6_FORMAT_R16G16B16_SINT]           = CAP(7.5),
   [GEN6_FORMAT_R32_SFIXED]               = CAP(7.5),
   [GEN6_FORMAT_R10G10B10A2_SNORM]        = CAP(7.5),
   [GEN6_FORMAT_R10G10B10A2_USCALED]      = CAP(7.5),
   [GEN6_FORMAT_R10G10B10A2_SSCALED]      = CAP(7.5),
   [GEN6_FORMAT_R10G10B10A2_SINT]         = CAP(7.5),
   [GEN6_FORMAT_B10G10R10A2_SNORM]        = CAP(7.5),
   [GEN6_FORMAT_B10G10R10A2_USCALED]      = CAP(7.5),
   [GEN6_FORMAT_B10G10R10A2_SSCALED]      = CAP(7.5),
   [GEN6_FORMAT_B10G10R10A2_UINT]         = CAP(7.5),
   [GEN6_FORMAT_B10G10R10A2_SINT]         = CAP(7.5),
   [GEN6_FORMAT_R8G8B8_UINT]              = CAP(7.5),
   [GEN6_FORMAT_R8G8B8_SINT]              = CAP(7.5),
#undef CAP
};

/*
 * This table is based on:
 *
 *  - the Sandy Bridge PRM, volume 4 part 1, page 88-97
 *  - the Ivy Bridge PRM, volume 2 part 1, page 195
 *  - the Haswell PRM, volume 7, page 535
 */
static const struct ilo_sol_cap ilo_sol_caps[] = {
#define CAP(buffer) { ILO_GEN(buffer) }
   [GEN6_FORMAT_R32G32B32A32_FLOAT]       = CAP(  1),
   [GEN6_FORMAT_R32G32B32A32_SINT]        = CAP(  1),
   [GEN6_FORMAT_R32G32B32A32_UINT]        = CAP(  1),
   [GEN6_FORMAT_R32G32B32_FLOAT]          = CAP(  1),
   [GEN6_FORMAT_R32G32B32_SINT]           = CAP(  1),
   [GEN6_FORMAT_R32G32B32_UINT]           = CAP(  1),
   [GEN6_FORMAT_R32G32_FLOAT]             = CAP(  1),
   [GEN6_FORMAT_R32G32_SINT]              = CAP(  1),
   [GEN6_FORMAT_R32G32_UINT]              = CAP(  1),
   [GEN6_FORMAT_R32_SINT]                 = CAP(  1),
   [GEN6_FORMAT_R32_UINT]                 = CAP(  1),
   [GEN6_FORMAT_R32_FLOAT]                = CAP(  1),
#undef CAP
};

/*
 * This table is based on:
 *
 *  - the Sandy Bridge PRM, volume 4 part 1, page 88-97
 *  - the Ivy Bridge PRM, volume 4 part 1, page 84-87
 */
static const struct ilo_sampler_cap ilo_sampler_caps[] = {
#define CAP(sampling, filtering, shadow_map, chroma_key) \
   { ILO_GEN(sampling), ILO_GEN(filtering), ILO_GEN(shadow_map), ILO_GEN(chroma_key) }
   [GEN6_FORMAT_R32G32B32A32_FLOAT]       = CAP(  1,   5,   0,   0),
   [GEN6_FORMAT_R32G32B32A32_SINT]        = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_R32G32B32A32_UINT]        = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_R32G32B32X32_FLOAT]       = CAP(  1,   5,   0,   0),
   [GEN6_FORMAT_R32G32B32_FLOAT]          = CAP(  1,   5,   0,   0),
   [GEN6_FORMAT_R32G32B32_SINT]           = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_R32G32B32_UINT]           = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_R16G16B16A16_UNORM]       = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R16G16B16A16_SNORM]       = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R16G16B16A16_SINT]        = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_R16G16B16A16_UINT]        = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_R16G16B16A16_FLOAT]       = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R32G32_FLOAT]             = CAP(  1,   5,   0,   0),
   [GEN6_FORMAT_R32G32_SINT]              = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_R32G32_UINT]              = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_R32_FLOAT_X8X24_TYPELESS] = CAP(  1,   5,   1,   0),
   [GEN6_FORMAT_X32_TYPELESS_G8X24_UINT]  = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_L32A32_FLOAT]             = CAP(  1,   5,   0,   0),
   [GEN6_FORMAT_R16G16B16X16_UNORM]       = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R16G16B16X16_FLOAT]       = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_A32X32_FLOAT]             = CAP(  1,   5,   0,   0),
   [GEN6_FORMAT_L32X32_FLOAT]             = CAP(  1,   5,   0,   0),
   [GEN6_FORMAT_I32X32_FLOAT]             = CAP(  1,   5,   0,   0),
   [GEN6_FORMAT_B8G8R8A8_UNORM]           = CAP(  1,   1,   0,   1),
   [GEN6_FORMAT_B8G8R8A8_UNORM_SRGB]      = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R10G10B10A2_UNORM]        = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R10G10B10A2_UNORM_SRGB]   = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R10G10B10A2_UINT]         = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_R10G10B10_SNORM_A2_UNORM] = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R8G8B8A8_UNORM]           = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R8G8B8A8_UNORM_SRGB]      = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R8G8B8A8_SNORM]           = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R8G8B8A8_SINT]            = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_R8G8B8A8_UINT]            = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_R16G16_UNORM]             = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R16G16_SNORM]             = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R16G16_SINT]              = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_R16G16_UINT]              = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_R16G16_FLOAT]             = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_B10G10R10A2_UNORM]        = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_B10G10R10A2_UNORM_SRGB]   = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R11G11B10_FLOAT]          = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R32_SINT]                 = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_R32_UINT]                 = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_R32_FLOAT]                = CAP(  1,   5,   1,   0),
   [GEN6_FORMAT_R24_UNORM_X8_TYPELESS]    = CAP(  1,   5,   1,   0),
   [GEN6_FORMAT_X24_TYPELESS_G8_UINT]     = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_L16A16_UNORM]             = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_I24X8_UNORM]              = CAP(  1,   5,   1,   0),
   [GEN6_FORMAT_L24X8_UNORM]              = CAP(  1,   5,   1,   0),
   [GEN6_FORMAT_A24X8_UNORM]              = CAP(  1,   5,   1,   0),
   [GEN6_FORMAT_I32_FLOAT]                = CAP(  1,   5,   1,   0),
   [GEN6_FORMAT_L32_FLOAT]                = CAP(  1,   5,   1,   0),
   [GEN6_FORMAT_A32_FLOAT]                = CAP(  1,   5,   1,   0),
   [GEN6_FORMAT_B8G8R8X8_UNORM]           = CAP(  1,   1,   0,   1),
   [GEN6_FORMAT_B8G8R8X8_UNORM_SRGB]      = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R8G8B8X8_UNORM]           = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R8G8B8X8_UNORM_SRGB]      = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R9G9B9E5_SHAREDEXP]       = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_B10G10R10X2_UNORM]        = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_L16A16_FLOAT]             = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_B5G6R5_UNORM]             = CAP(  1,   1,   0,   1),
   [GEN6_FORMAT_B5G6R5_UNORM_SRGB]        = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_B5G5R5A1_UNORM]           = CAP(  1,   1,   0,   1),
   [GEN6_FORMAT_B5G5R5A1_UNORM_SRGB]      = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_B4G4R4A4_UNORM]           = CAP(  1,   1,   0,   1),
   [GEN6_FORMAT_B4G4R4A4_UNORM_SRGB]      = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R8G8_UNORM]               = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R8G8_SNORM]               = CAP(  1,   1,   0,   1),
   [GEN6_FORMAT_R8G8_SINT]                = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_R8G8_UINT]                = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_R16_UNORM]                = CAP(  1,   1,   1,   0),
   [GEN6_FORMAT_R16_SNORM]                = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R16_SINT]                 = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_R16_UINT]                 = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_R16_FLOAT]                = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_A8P8_UNORM_PALETTE0]      = CAP(  5,   5,   0,   0),
   [GEN6_FORMAT_A8P8_UNORM_PALETTE1]      = CAP(  5,   5,   0,   0),
   [GEN6_FORMAT_I16_UNORM]                = CAP(  1,   1,   1,   0),
   [GEN6_FORMAT_L16_UNORM]                = CAP(  1,   1,   1,   0),
   [GEN6_FORMAT_A16_UNORM]                = CAP(  1,   1,   1,   0),
   [GEN6_FORMAT_L8A8_UNORM]               = CAP(  1,   1,   0,   1),
   [GEN6_FORMAT_I16_FLOAT]                = CAP(  1,   1,   1,   0),
   [GEN6_FORMAT_L16_FLOAT]                = CAP(  1,   1,   1,   0),
   [GEN6_FORMAT_A16_FLOAT]                = CAP(  1,   1,   1,   0),
   [GEN6_FORMAT_L8A8_UNORM_SRGB]          = CAP(4.5, 4.5,   0,   0),
   [GEN6_FORMAT_R5G5_SNORM_B6_UNORM]      = CAP(  1,   1,   0,   1),
   [GEN6_FORMAT_P8A8_UNORM_PALETTE0]      = CAP(  5,   5,   0,   0),
   [GEN6_FORMAT_P8A8_UNORM_PALETTE1]      = CAP(  5,   5,   0,   0),
   [GEN6_FORMAT_R8_UNORM]                 = CAP(  1,   1,   0, 4.5),
   [GEN6_FORMAT_R8_SNORM]                 = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R8_SINT]                  = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_R8_UINT]                  = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_A8_UNORM]                 = CAP(  1,   1,   0,   1),
   [GEN6_FORMAT_I8_UNORM]                 = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_L8_UNORM]                 = CAP(  1,   1,   0,   1),
   [GEN6_FORMAT_P4A4_UNORM_PALETTE0]      = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_A4P4_UNORM_PALETTE0]      = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_P8_UNORM_PALETTE0]        = CAP(4.5, 4.5,   0,   0),
   [GEN6_FORMAT_L8_UNORM_SRGB]            = CAP(4.5, 4.5,   0,   0),
   [GEN6_FORMAT_P8_UNORM_PALETTE1]        = CAP(4.5, 4.5,   0,   0),
   [GEN6_FORMAT_P4A4_UNORM_PALETTE1]      = CAP(4.5, 4.5,   0,   0),
   [GEN6_FORMAT_A4P4_UNORM_PALETTE1]      = CAP(4.5, 4.5,   0,   0),
   [GEN6_FORMAT_DXT1_RGB_SRGB]            = CAP(4.5, 4.5,   0,   0),
   [GEN6_FORMAT_R1_UNORM]                 = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_YCRCB_NORMAL]             = CAP(  1,   1,   0,   1),
   [GEN6_FORMAT_YCRCB_SWAPUVY]            = CAP(  1,   1,   0,   1),
   [GEN6_FORMAT_P2_UNORM_PALETTE0]        = CAP(4.5, 4.5,   0,   0),
   [GEN6_FORMAT_P2_UNORM_PALETTE1]        = CAP(4.5, 4.5,   0,   0),
   [GEN6_FORMAT_BC1_UNORM]                = CAP(  1,   1,   0,   1),
   [GEN6_FORMAT_BC2_UNORM]                = CAP(  1,   1,   0,   1),
   [GEN6_FORMAT_BC3_UNORM]                = CAP(  1,   1,   0,   1),
   [GEN6_FORMAT_BC4_UNORM]                = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_BC5_UNORM]                = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_BC1_UNORM_SRGB]           = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_BC2_UNORM_SRGB]           = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_BC3_UNORM_SRGB]           = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_MONO8]                    = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_YCRCB_SWAPUV]             = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_YCRCB_SWAPY]              = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_DXT1_RGB]                 = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_FXT1]                     = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_BC4_SNORM]                = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_BC5_SNORM]                = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R16G16B16_FLOAT]          = CAP(  5,   5,   0,   0),
   [GEN6_FORMAT_BC6H_SF16]                = CAP(  7,   7,   0,   0),
   [GEN6_FORMAT_BC7_UNORM]                = CAP(  7,   7,   0,   0),
   [GEN6_FORMAT_BC7_UNORM_SRGB]           = CAP(  7,   7,   0,   0),
   [GEN6_FORMAT_BC6H_UF16]                = CAP(  7,   7,   0,   0),
#undef CAP
};

/*
 * This table is based on:
 *
 *  - the Sandy Bridge PRM, volume 4 part 1, page 88-97
 *  - the Ivy Bridge PRM, volume 4 part 1, page 172, 252-253, and 277-278
 *  - the Haswell PRM, volume 7, page 262-264
 */
static const struct ilo_dp_cap ilo_dp_caps[] = {
#define CAP(rt_write, rt_write_blending, typed_write, media_color_processing) \
   { ILO_GEN(rt_write), ILO_GEN(rt_write_blending), ILO_GEN(typed_write), ILO_GEN(media_color_processing) }
   [GEN6_FORMAT_R32G32B32A32_FLOAT]       = CAP(  1,   1,   7,   0),
   [GEN6_FORMAT_R32G32B32A32_SINT]        = CAP(  1,   0,   7,   0),
   [GEN6_FORMAT_R32G32B32A32_UINT]        = CAP(  1,   0,   7,   0),
   [GEN6_FORMAT_R16G16B16A16_UNORM]       = CAP(  1, 4.5,   7,   6),
   [GEN6_FORMAT_R16G16B16A16_SNORM]       = CAP(  1,   6,   7,   0),
   [GEN6_FORMAT_R16G16B16A16_SINT]        = CAP(  1,   0,   7,   0),
   [GEN6_FORMAT_R16G16B16A16_UINT]        = CAP(  1,   0,   7,   0),
   [GEN6_FORMAT_R16G16B16A16_FLOAT]       = CAP(  1,   1,   7,   0),
   [GEN6_FORMAT_R32G32_FLOAT]             = CAP(  1,   1,   7,   0),
   [GEN6_FORMAT_R32G32_SINT]              = CAP(  1,   0,   7,   0),
   [GEN6_FORMAT_R32G32_UINT]              = CAP(  1,   0,   7,   0),
   [GEN6_FORMAT_B8G8R8A8_UNORM]           = CAP(  1,   1,   7,   6),
   [GEN6_FORMAT_B8G8R8A8_UNORM_SRGB]      = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R10G10B10A2_UNORM]        = CAP(  1,   1,   7,   6),
   [GEN6_FORMAT_R10G10B10A2_UNORM_SRGB]   = CAP(  0,   0,   0,   6),
   [GEN6_FORMAT_R10G10B10A2_UINT]         = CAP(  1,   0,   7,   0),
   [GEN6_FORMAT_R8G8B8A8_UNORM]           = CAP(  1,   1,   7,   6),
   [GEN6_FORMAT_R8G8B8A8_UNORM_SRGB]      = CAP(  1,   1,   0,   6),
   [GEN6_FORMAT_R8G8B8A8_SNORM]           = CAP(  1,   6,   7,   0),
   [GEN6_FORMAT_R8G8B8A8_SINT]            = CAP(  1,   0,   7,   0),
   [GEN6_FORMAT_R8G8B8A8_UINT]            = CAP(  1,   0,   7,   0),
   [GEN6_FORMAT_R16G16_UNORM]             = CAP(  1, 4.5,   7,   0),
   [GEN6_FORMAT_R16G16_SNORM]             = CAP(  1,   6,   7,   0),
   [GEN6_FORMAT_R16G16_SINT]              = CAP(  1,   0,   7,   0),
   [GEN6_FORMAT_R16G16_UINT]              = CAP(  1,   0,   7,   0),
   [GEN6_FORMAT_R16G16_FLOAT]             = CAP(  1,   1,   7,   0),
   [GEN6_FORMAT_B10G10R10A2_UNORM]        = CAP(  1,   1,   7,   6),
   [GEN6_FORMAT_B10G10R10A2_UNORM_SRGB]   = CAP(  1,   1,   0,   6),
   [GEN6_FORMAT_R11G11B10_FLOAT]          = CAP(  1,   1,   7,   0),
   [GEN6_FORMAT_R32_SINT]                 = CAP(  1,   0,   7,   0),
   [GEN6_FORMAT_R32_UINT]                 = CAP(  1,   0,   7,   0),
   [GEN6_FORMAT_R32_FLOAT]                = CAP(  1,   1,   7,   0),
   [GEN6_FORMAT_B8G8R8X8_UNORM]           = CAP(  0,   0,   0,   6),
   [GEN6_FORMAT_B5G6R5_UNORM]             = CAP(  1,   1,   7,   0),
   [GEN6_FORMAT_B5G6R5_UNORM_SRGB]        = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_B5G5R5A1_UNORM]           = CAP(  1,   1,   7,   0),
   [GEN6_FORMAT_B5G5R5A1_UNORM_SRGB]      = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_B4G4R4A4_UNORM]           = CAP(  1,   1,   7,   0),
   [GEN6_FORMAT_B4G4R4A4_UNORM_SRGB]      = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R8G8_UNORM]               = CAP(  1,   1,   7,   0),
   [GEN6_FORMAT_R8G8_SNORM]               = CAP(  1,   6,   7,   0),
   [GEN6_FORMAT_R8G8_SINT]                = CAP(  1,   0,   7,   0),
   [GEN6_FORMAT_R8G8_UINT]                = CAP(  1,   0,   7,   0),
   [GEN6_FORMAT_R16_UNORM]                = CAP(  1, 4.5,   7,   7),
   [GEN6_FORMAT_R16_SNORM]                = CAP(  1,   6,   7,   0),
   [GEN6_FORMAT_R16_SINT]                 = CAP(  1,   0,   7,   0),
   [GEN6_FORMAT_R16_UINT]                 = CAP(  1,   0,   7,   0),
   [GEN6_FORMAT_R16_FLOAT]                = CAP(  1,   1,   7,   0),
   [GEN6_FORMAT_B5G5R5X1_UNORM]           = CAP(  1,   1,   7,   0),
   [GEN6_FORMAT_B5G5R5X1_UNORM_SRGB]      = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R8_UNORM]                 = CAP(  1,   1,   7,   0),
   [GEN6_FORMAT_R8_SNORM]                 = CAP(  1,   6,   7,   0),
   [GEN6_FORMAT_R8_SINT]                  = CAP(  1,   0,   7,   0),
   [GEN6_FORMAT_R8_UINT]                  = CAP(  1,   0,   7,   0),
   [GEN6_FORMAT_A8_UNORM]                 = CAP(  1,   1,   7,   0),
   [GEN6_FORMAT_YCRCB_NORMAL]             = CAP(  1,   0,   0,   6),
   [GEN6_FORMAT_YCRCB_SWAPUVY]            = CAP(  1,   0,   0,   6),
   [GEN6_FORMAT_YCRCB_SWAPUV]             = CAP(  1,   0,   0,   6),
   [GEN6_FORMAT_YCRCB_SWAPY]              = CAP(  1,   0,   0,   6),
#undef CAP
};

/**
 * Translate a color (non-depth/stencil) pipe format to the matching hardware
 * format.  Return -1 on errors.
 */
int
ilo_translate_color_format(const struct ilo_dev_info *dev,
                           enum pipe_format format)
{
   static const int format_mapping[PIPE_FORMAT_COUNT] = {
      [PIPE_FORMAT_NONE]                  = 0,
      [PIPE_FORMAT_B8G8R8A8_UNORM]        = GEN6_FORMAT_B8G8R8A8_UNORM,
      [PIPE_FORMAT_B8G8R8X8_UNORM]        = GEN6_FORMAT_B8G8R8X8_UNORM,
      [PIPE_FORMAT_A8R8G8B8_UNORM]        = 0,
      [PIPE_FORMAT_X8R8G8B8_UNORM]        = 0,
      [PIPE_FORMAT_B5G5R5A1_UNORM]        = GEN6_FORMAT_B5G5R5A1_UNORM,
      [PIPE_FORMAT_B4G4R4A4_UNORM]        = GEN6_FORMAT_B4G4R4A4_UNORM,
      [PIPE_FORMAT_B5G6R5_UNORM]          = GEN6_FORMAT_B5G6R5_UNORM,
      [PIPE_FORMAT_R10G10B10A2_UNORM]     = GEN6_FORMAT_R10G10B10A2_UNORM,
      [PIPE_FORMAT_L8_UNORM]              = GEN6_FORMAT_L8_UNORM,
      [PIPE_FORMAT_A8_UNORM]              = GEN6_FORMAT_A8_UNORM,
      [PIPE_FORMAT_I8_UNORM]              = GEN6_FORMAT_I8_UNORM,
      [PIPE_FORMAT_L8A8_UNORM]            = GEN6_FORMAT_L8A8_UNORM,
      [PIPE_FORMAT_L16_UNORM]             = GEN6_FORMAT_L16_UNORM,
      [PIPE_FORMAT_UYVY]                  = GEN6_FORMAT_YCRCB_SWAPUVY,
      [PIPE_FORMAT_YUYV]                  = GEN6_FORMAT_YCRCB_NORMAL,
      [PIPE_FORMAT_Z16_UNORM]             = 0,
      [PIPE_FORMAT_Z32_UNORM]             = 0,
      [PIPE_FORMAT_Z32_FLOAT]             = 0,
      [PIPE_FORMAT_Z24_UNORM_S8_UINT]     = 0,
      [PIPE_FORMAT_S8_UINT_Z24_UNORM]     = 0,
      [PIPE_FORMAT_Z24X8_UNORM]           = 0,
      [PIPE_FORMAT_X8Z24_UNORM]           = 0,
      [PIPE_FORMAT_S8_UINT]               = 0,
      [PIPE_FORMAT_R64_FLOAT]             = GEN6_FORMAT_R64_FLOAT,
      [PIPE_FORMAT_R64G64_FLOAT]          = GEN6_FORMAT_R64G64_FLOAT,
      [PIPE_FORMAT_R64G64B64_FLOAT]       = GEN6_FORMAT_R64G64B64_FLOAT,
      [PIPE_FORMAT_R64G64B64A64_FLOAT]    = GEN6_FORMAT_R64G64B64A64_FLOAT,
      [PIPE_FORMAT_R32_FLOAT]             = GEN6_FORMAT_R32_FLOAT,
      [PIPE_FORMAT_R32G32_FLOAT]          = GEN6_FORMAT_R32G32_FLOAT,
      [PIPE_FORMAT_R32G32B32_FLOAT]       = GEN6_FORMAT_R32G32B32_FLOAT,
      [PIPE_FORMAT_R32G32B32A32_FLOAT]    = GEN6_FORMAT_R32G32B32A32_FLOAT,
      [PIPE_FORMAT_R32_UNORM]             = GEN6_FORMAT_R32_UNORM,
      [PIPE_FORMAT_R32G32_UNORM]          = GEN6_FORMAT_R32G32_UNORM,
      [PIPE_FORMAT_R32G32B32_UNORM]       = GEN6_FORMAT_R32G32B32_UNORM,
      [PIPE_FORMAT_R32G32B32A32_UNORM]    = GEN6_FORMAT_R32G32B32A32_UNORM,
      [PIPE_FORMAT_R32_USCALED]           = GEN6_FORMAT_R32_USCALED,
      [PIPE_FORMAT_R32G32_USCALED]        = GEN6_FORMAT_R32G32_USCALED,
      [PIPE_FORMAT_R32G32B32_USCALED]     = GEN6_FORMAT_R32G32B32_USCALED,
      [PIPE_FORMAT_R32G32B32A32_USCALED]  = GEN6_FORMAT_R32G32B32A32_USCALED,
      [PIPE_FORMAT_R32_SNORM]             = GEN6_FORMAT_R32_SNORM,
      [PIPE_FORMAT_R32G32_SNORM]          = GEN6_FORMAT_R32G32_SNORM,
      [PIPE_FORMAT_R32G32B32_SNORM]       = GEN6_FORMAT_R32G32B32_SNORM,
      [PIPE_FORMAT_R32G32B32A32_SNORM]    = GEN6_FORMAT_R32G32B32A32_SNORM,
      [PIPE_FORMAT_R32_SSCALED]           = GEN6_FORMAT_R32_SSCALED,
      [PIPE_FORMAT_R32G32_SSCALED]        = GEN6_FORMAT_R32G32_SSCALED,
      [PIPE_FORMAT_R32G32B32_SSCALED]     = GEN6_FORMAT_R32G32B32_SSCALED,
      [PIPE_FORMAT_R32G32B32A32_SSCALED]  = GEN6_FORMAT_R32G32B32A32_SSCALED,
      [PIPE_FORMAT_R16_UNORM]             = GEN6_FORMAT_R16_UNORM,
      [PIPE_FORMAT_R16G16_UNORM]          = GEN6_FORMAT_R16G16_UNORM,
      [PIPE_FORMAT_R16G16B16_UNORM]       = GEN6_FORMAT_R16G16B16_UNORM,
      [PIPE_FORMAT_R16G16B16A16_UNORM]    = GEN6_FORMAT_R16G16B16A16_UNORM,
      [PIPE_FORMAT_R16_USCALED]           = GEN6_FORMAT_R16_USCALED,
      [PIPE_FORMAT_R16G16_USCALED]        = GEN6_FORMAT_R16G16_USCALED,
      [PIPE_FORMAT_R16G16B16_USCALED]     = GEN6_FORMAT_R16G16B16_USCALED,
      [PIPE_FORMAT_R16G16B16A16_USCALED]  = GEN6_FORMAT_R16G16B16A16_USCALED,
      [PIPE_FORMAT_R16_SNORM]             = GEN6_FORMAT_R16_SNORM,
      [PIPE_FORMAT_R16G16_SNORM]          = GEN6_FORMAT_R16G16_SNORM,
      [PIPE_FORMAT_R16G16B16_SNORM]       = GEN6_FORMAT_R16G16B16_SNORM,
      [PIPE_FORMAT_R16G16B16A16_SNORM]    = GEN6_FORMAT_R16G16B16A16_SNORM,
      [PIPE_FORMAT_R16_SSCALED]           = GEN6_FORMAT_R16_SSCALED,
      [PIPE_FORMAT_R16G16_SSCALED]        = GEN6_FORMAT_R16G16_SSCALED,
      [PIPE_FORMAT_R16G16B16_SSCALED]     = GEN6_FORMAT_R16G16B16_SSCALED,
      [PIPE_FORMAT_R16G16B16A16_SSCALED]  = GEN6_FORMAT_R16G16B16A16_SSCALED,
      [PIPE_FORMAT_R8_UNORM]              = GEN6_FORMAT_R8_UNORM,
      [PIPE_FORMAT_R8G8_UNORM]            = GEN6_FORMAT_R8G8_UNORM,
      [PIPE_FORMAT_R8G8B8_UNORM]          = GEN6_FORMAT_R8G8B8_UNORM,
      [PIPE_FORMAT_R8G8B8A8_UNORM]        = GEN6_FORMAT_R8G8B8A8_UNORM,
      [PIPE_FORMAT_X8B8G8R8_UNORM]        = 0,
      [PIPE_FORMAT_R8_USCALED]            = GEN6_FORMAT_R8_USCALED,
      [PIPE_FORMAT_R8G8_USCALED]          = GEN6_FORMAT_R8G8_USCALED,
      [PIPE_FORMAT_R8G8B8_USCALED]        = GEN6_FORMAT_R8G8B8_USCALED,
      [PIPE_FORMAT_R8G8B8A8_USCALED]      = GEN6_FORMAT_R8G8B8A8_USCALED,
      [PIPE_FORMAT_R8_SNORM]              = GEN6_FORMAT_R8_SNORM,
      [PIPE_FORMAT_R8G8_SNORM]            = GEN6_FORMAT_R8G8_SNORM,
      [PIPE_FORMAT_R8G8B8_SNORM]          = GEN6_FORMAT_R8G8B8_SNORM,
      [PIPE_FORMAT_R8G8B8A8_SNORM]        = GEN6_FORMAT_R8G8B8A8_SNORM,
      [PIPE_FORMAT_R8_SSCALED]            = GEN6_FORMAT_R8_SSCALED,
      [PIPE_FORMAT_R8G8_SSCALED]          = GEN6_FORMAT_R8G8_SSCALED,
      [PIPE_FORMAT_R8G8B8_SSCALED]        = GEN6_FORMAT_R8G8B8_SSCALED,
      [PIPE_FORMAT_R8G8B8A8_SSCALED]      = GEN6_FORMAT_R8G8B8A8_SSCALED,
      [PIPE_FORMAT_R32_FIXED]             = GEN6_FORMAT_R32_SFIXED,
      [PIPE_FORMAT_R32G32_FIXED]          = GEN6_FORMAT_R32G32_SFIXED,
      [PIPE_FORMAT_R32G32B32_FIXED]       = GEN6_FORMAT_R32G32B32_SFIXED,
      [PIPE_FORMAT_R32G32B32A32_FIXED]    = GEN6_FORMAT_R32G32B32A32_SFIXED,
      [PIPE_FORMAT_R16_FLOAT]             = GEN6_FORMAT_R16_FLOAT,
      [PIPE_FORMAT_R16G16_FLOAT]          = GEN6_FORMAT_R16G16_FLOAT,
      [PIPE_FORMAT_R16G16B16_FLOAT]       = GEN6_FORMAT_R16G16B16_FLOAT,
      [PIPE_FORMAT_R16G16B16A16_FLOAT]    = GEN6_FORMAT_R16G16B16A16_FLOAT,
      [PIPE_FORMAT_L8_SRGB]               = GEN6_FORMAT_L8_UNORM_SRGB,
      [PIPE_FORMAT_L8A8_SRGB]             = GEN6_FORMAT_L8A8_UNORM_SRGB,
      [PIPE_FORMAT_R8G8B8_SRGB]           = GEN6_FORMAT_R8G8B8_UNORM_SRGB,
      [PIPE_FORMAT_A8B8G8R8_SRGB]         = 0,
      [PIPE_FORMAT_X8B8G8R8_SRGB]         = 0,
      [PIPE_FORMAT_B8G8R8A8_SRGB]         = GEN6_FORMAT_B8G8R8A8_UNORM_SRGB,
      [PIPE_FORMAT_B8G8R8X8_SRGB]         = GEN6_FORMAT_B8G8R8X8_UNORM_SRGB,
      [PIPE_FORMAT_A8R8G8B8_SRGB]         = 0,
      [PIPE_FORMAT_X8R8G8B8_SRGB]         = 0,
      [PIPE_FORMAT_R8G8B8A8_SRGB]         = GEN6_FORMAT_R8G8B8A8_UNORM_SRGB,
      [PIPE_FORMAT_DXT1_RGB]              = GEN6_FORMAT_DXT1_RGB,
      [PIPE_FORMAT_DXT1_RGBA]             = GEN6_FORMAT_BC1_UNORM,
      [PIPE_FORMAT_DXT3_RGBA]             = GEN6_FORMAT_BC2_UNORM,
      [PIPE_FORMAT_DXT5_RGBA]             = GEN6_FORMAT_BC3_UNORM,
      [PIPE_FORMAT_DXT1_SRGB]             = GEN6_FORMAT_DXT1_RGB_SRGB,
      [PIPE_FORMAT_DXT1_SRGBA]            = GEN6_FORMAT_BC1_UNORM_SRGB,
      [PIPE_FORMAT_DXT3_SRGBA]            = GEN6_FORMAT_BC2_UNORM_SRGB,
      [PIPE_FORMAT_DXT5_SRGBA]            = GEN6_FORMAT_BC3_UNORM_SRGB,
      [PIPE_FORMAT_RGTC1_UNORM]           = GEN6_FORMAT_BC4_UNORM,
      [PIPE_FORMAT_RGTC1_SNORM]           = GEN6_FORMAT_BC4_SNORM,
      [PIPE_FORMAT_RGTC2_UNORM]           = GEN6_FORMAT_BC5_UNORM,
      [PIPE_FORMAT_RGTC2_SNORM]           = GEN6_FORMAT_BC5_SNORM,
      [PIPE_FORMAT_R8G8_B8G8_UNORM]       = 0,
      [PIPE_FORMAT_G8R8_G8B8_UNORM]       = 0,
      [PIPE_FORMAT_R8SG8SB8UX8U_NORM]     = 0,
      [PIPE_FORMAT_R5SG5SB6U_NORM]        = 0,
      [PIPE_FORMAT_A8B8G8R8_UNORM]        = 0,
      [PIPE_FORMAT_B5G5R5X1_UNORM]        = GEN6_FORMAT_B5G5R5X1_UNORM,
      [PIPE_FORMAT_R10G10B10A2_USCALED]   = GEN6_FORMAT_R10G10B10A2_USCALED,
      [PIPE_FORMAT_R11G11B10_FLOAT]       = GEN6_FORMAT_R11G11B10_FLOAT,
      [PIPE_FORMAT_R9G9B9E5_FLOAT]        = GEN6_FORMAT_R9G9B9E5_SHAREDEXP,
      [PIPE_FORMAT_Z32_FLOAT_S8X24_UINT]  = 0,
      [PIPE_FORMAT_R1_UNORM]              = GEN6_FORMAT_R1_UNORM,
      [PIPE_FORMAT_R10G10B10X2_USCALED]   = GEN6_FORMAT_R10G10B10X2_USCALED,
      [PIPE_FORMAT_R10G10B10X2_SNORM]     = 0,
      [PIPE_FORMAT_L4A4_UNORM]            = 0,
      [PIPE_FORMAT_B10G10R10A2_UNORM]     = GEN6_FORMAT_B10G10R10A2_UNORM,
      [PIPE_FORMAT_R10SG10SB10SA2U_NORM]  = 0,
      [PIPE_FORMAT_R8G8Bx_SNORM]          = 0,
      [PIPE_FORMAT_R8G8B8X8_UNORM]        = GEN6_FORMAT_R8G8B8X8_UNORM,
      [PIPE_FORMAT_B4G4R4X4_UNORM]        = 0,
      [PIPE_FORMAT_X24S8_UINT]            = 0,
      [PIPE_FORMAT_S8X24_UINT]            = 0,
      [PIPE_FORMAT_X32_S8X24_UINT]        = 0,
      [PIPE_FORMAT_B2G3R3_UNORM]          = 0,
      [PIPE_FORMAT_L16A16_UNORM]          = GEN6_FORMAT_L16A16_UNORM,
      [PIPE_FORMAT_A16_UNORM]             = GEN6_FORMAT_A16_UNORM,
      [PIPE_FORMAT_I16_UNORM]             = GEN6_FORMAT_I16_UNORM,
      [PIPE_FORMAT_LATC1_UNORM]           = 0,
      [PIPE_FORMAT_LATC1_SNORM]           = 0,
      [PIPE_FORMAT_LATC2_UNORM]           = 0,
      [PIPE_FORMAT_LATC2_SNORM]           = 0,
      [PIPE_FORMAT_A8_SNORM]              = 0,
      [PIPE_FORMAT_L8_SNORM]              = 0,
      [PIPE_FORMAT_L8A8_SNORM]            = 0,
      [PIPE_FORMAT_I8_SNORM]              = 0,
      [PIPE_FORMAT_A16_SNORM]             = 0,
      [PIPE_FORMAT_L16_SNORM]             = 0,
      [PIPE_FORMAT_L16A16_SNORM]          = 0,
      [PIPE_FORMAT_I16_SNORM]             = 0,
      [PIPE_FORMAT_A16_FLOAT]             = GEN6_FORMAT_A16_FLOAT,
      [PIPE_FORMAT_L16_FLOAT]             = GEN6_FORMAT_L16_FLOAT,
      [PIPE_FORMAT_L16A16_FLOAT]          = GEN6_FORMAT_L16A16_FLOAT,
      [PIPE_FORMAT_I16_FLOAT]             = GEN6_FORMAT_I16_FLOAT,
      [PIPE_FORMAT_A32_FLOAT]             = GEN6_FORMAT_A32_FLOAT,
      [PIPE_FORMAT_L32_FLOAT]             = GEN6_FORMAT_L32_FLOAT,
      [PIPE_FORMAT_L32A32_FLOAT]          = GEN6_FORMAT_L32A32_FLOAT,
      [PIPE_FORMAT_I32_FLOAT]             = GEN6_FORMAT_I32_FLOAT,
      [PIPE_FORMAT_YV12]                  = 0,
      [PIPE_FORMAT_YV16]                  = 0,
      [PIPE_FORMAT_IYUV]                  = 0,
      [PIPE_FORMAT_NV12]                  = 0,
      [PIPE_FORMAT_NV21]                  = 0,
      [PIPE_FORMAT_A4R4_UNORM]            = 0,
      [PIPE_FORMAT_R4A4_UNORM]            = 0,
      [PIPE_FORMAT_R8A8_UNORM]            = 0,
      [PIPE_FORMAT_A8R8_UNORM]            = 0,
      [PIPE_FORMAT_R10G10B10A2_SSCALED]   = GEN6_FORMAT_R10G10B10A2_SSCALED,
      [PIPE_FORMAT_R10G10B10A2_SNORM]     = GEN6_FORMAT_R10G10B10A2_SNORM,
      [PIPE_FORMAT_B10G10R10A2_USCALED]   = GEN6_FORMAT_B10G10R10A2_USCALED,
      [PIPE_FORMAT_B10G10R10A2_SSCALED]   = GEN6_FORMAT_B10G10R10A2_SSCALED,
      [PIPE_FORMAT_B10G10R10A2_SNORM]     = GEN6_FORMAT_B10G10R10A2_SNORM,
      [PIPE_FORMAT_R8_UINT]               = GEN6_FORMAT_R8_UINT,
      [PIPE_FORMAT_R8G8_UINT]             = GEN6_FORMAT_R8G8_UINT,
      [PIPE_FORMAT_R8G8B8_UINT]           = GEN6_FORMAT_R8G8B8_UINT,
      [PIPE_FORMAT_R8G8B8A8_UINT]         = GEN6_FORMAT_R8G8B8A8_UINT,
      [PIPE_FORMAT_R8_SINT]               = GEN6_FORMAT_R8_SINT,
      [PIPE_FORMAT_R8G8_SINT]             = GEN6_FORMAT_R8G8_SINT,
      [PIPE_FORMAT_R8G8B8_SINT]           = GEN6_FORMAT_R8G8B8_SINT,
      [PIPE_FORMAT_R8G8B8A8_SINT]         = GEN6_FORMAT_R8G8B8A8_SINT,
      [PIPE_FORMAT_R16_UINT]              = GEN6_FORMAT_R16_UINT,
      [PIPE_FORMAT_R16G16_UINT]           = GEN6_FORMAT_R16G16_UINT,
      [PIPE_FORMAT_R16G16B16_UINT]        = GEN6_FORMAT_R16G16B16_UINT,
      [PIPE_FORMAT_R16G16B16A16_UINT]     = GEN6_FORMAT_R16G16B16A16_UINT,
      [PIPE_FORMAT_R16_SINT]              = GEN6_FORMAT_R16_SINT,
      [PIPE_FORMAT_R16G16_SINT]           = GEN6_FORMAT_R16G16_SINT,
      [PIPE_FORMAT_R16G16B16_SINT]        = GEN6_FORMAT_R16G16B16_SINT,
      [PIPE_FORMAT_R16G16B16A16_SINT]     = GEN6_FORMAT_R16G16B16A16_SINT,
      [PIPE_FORMAT_R32_UINT]              = GEN6_FORMAT_R32_UINT,
      [PIPE_FORMAT_R32G32_UINT]           = GEN6_FORMAT_R32G32_UINT,
      [PIPE_FORMAT_R32G32B32_UINT]        = GEN6_FORMAT_R32G32B32_UINT,
      [PIPE_FORMAT_R32G32B32A32_UINT]     = GEN6_FORMAT_R32G32B32A32_UINT,
      [PIPE_FORMAT_R32_SINT]              = GEN6_FORMAT_R32_SINT,
      [PIPE_FORMAT_R32G32_SINT]           = GEN6_FORMAT_R32G32_SINT,
      [PIPE_FORMAT_R32G32B32_SINT]        = GEN6_FORMAT_R32G32B32_SINT,
      [PIPE_FORMAT_R32G32B32A32_SINT]     = GEN6_FORMAT_R32G32B32A32_SINT,
      [PIPE_FORMAT_A8_UINT]               = 0,
      [PIPE_FORMAT_I8_UINT]               = GEN6_FORMAT_I8_UINT,
      [PIPE_FORMAT_L8_UINT]               = GEN6_FORMAT_L8_UINT,
      [PIPE_FORMAT_L8A8_UINT]             = GEN6_FORMAT_L8A8_UINT,
      [PIPE_FORMAT_A8_SINT]               = 0,
      [PIPE_FORMAT_I8_SINT]               = GEN6_FORMAT_I8_SINT,
      [PIPE_FORMAT_L8_SINT]               = GEN6_FORMAT_L8_SINT,
      [PIPE_FORMAT_L8A8_SINT]             = GEN6_FORMAT_L8A8_SINT,
      [PIPE_FORMAT_A16_UINT]              = 0,
      [PIPE_FORMAT_I16_UINT]              = 0,
      [PIPE_FORMAT_L16_UINT]              = 0,
      [PIPE_FORMAT_L16A16_UINT]           = 0,
      [PIPE_FORMAT_A16_SINT]              = 0,
      [PIPE_FORMAT_I16_SINT]              = 0,
      [PIPE_FORMAT_L16_SINT]              = 0,
      [PIPE_FORMAT_L16A16_SINT]           = 0,
      [PIPE_FORMAT_A32_UINT]              = 0,
      [PIPE_FORMAT_I32_UINT]              = 0,
      [PIPE_FORMAT_L32_UINT]              = 0,
      [PIPE_FORMAT_L32A32_UINT]           = 0,
      [PIPE_FORMAT_A32_SINT]              = 0,
      [PIPE_FORMAT_I32_SINT]              = 0,
      [PIPE_FORMAT_L32_SINT]              = 0,
      [PIPE_FORMAT_L32A32_SINT]           = 0,
      [PIPE_FORMAT_B10G10R10A2_UINT]      = GEN6_FORMAT_B10G10R10A2_UINT,
      [PIPE_FORMAT_ETC1_RGB8]             = GEN6_FORMAT_ETC1_RGB8,
      [PIPE_FORMAT_R8G8_R8B8_UNORM]       = 0,
      [PIPE_FORMAT_G8R8_B8R8_UNORM]       = 0,
      [PIPE_FORMAT_R8G8B8X8_SNORM]        = 0,
      [PIPE_FORMAT_R8G8B8X8_SRGB]         = 0,
      [PIPE_FORMAT_R8G8B8X8_UINT]         = 0,
      [PIPE_FORMAT_R8G8B8X8_SINT]         = 0,
      [PIPE_FORMAT_B10G10R10X2_UNORM]     = GEN6_FORMAT_B10G10R10X2_UNORM,
      [PIPE_FORMAT_R16G16B16X16_UNORM]    = GEN6_FORMAT_R16G16B16X16_UNORM,
      [PIPE_FORMAT_R16G16B16X16_SNORM]    = 0,
      [PIPE_FORMAT_R16G16B16X16_FLOAT]    = GEN6_FORMAT_R16G16B16X16_FLOAT,
      [PIPE_FORMAT_R16G16B16X16_UINT]     = 0,
      [PIPE_FORMAT_R16G16B16X16_SINT]     = 0,
      [PIPE_FORMAT_R32G32B32X32_FLOAT]    = GEN6_FORMAT_R32G32B32X32_FLOAT,
      [PIPE_FORMAT_R32G32B32X32_UINT]     = 0,
      [PIPE_FORMAT_R32G32B32X32_SINT]     = 0,
      [PIPE_FORMAT_R8A8_SNORM]            = 0,
      [PIPE_FORMAT_R16A16_UNORM]          = 0,
      [PIPE_FORMAT_R16A16_SNORM]          = 0,
      [PIPE_FORMAT_R16A16_FLOAT]          = 0,
      [PIPE_FORMAT_R32A32_FLOAT]          = 0,
      [PIPE_FORMAT_R8A8_UINT]             = 0,
      [PIPE_FORMAT_R8A8_SINT]             = 0,
      [PIPE_FORMAT_R16A16_UINT]           = 0,
      [PIPE_FORMAT_R16A16_SINT]           = 0,
      [PIPE_FORMAT_R32A32_UINT]           = 0,
      [PIPE_FORMAT_R32A32_SINT]           = 0,
      [PIPE_FORMAT_R10G10B10A2_UINT]      = GEN6_FORMAT_R10G10B10A2_UINT,
      [PIPE_FORMAT_B5G6R5_SRGB]           = GEN6_FORMAT_B5G6R5_UNORM_SRGB,
   };
   int sfmt = format_mapping[format];

   /* GEN6_FORMAT_R32G32B32A32_FLOAT happens to be 0 */
   if (!sfmt && format != PIPE_FORMAT_R32G32B32A32_FLOAT)
      sfmt = -1;

   return sfmt;
}

static bool
ilo_format_supports_zs(const struct ilo_dev_info *dev,
                       enum pipe_format format)
{
   switch (format) {
   case PIPE_FORMAT_Z16_UNORM:
   case PIPE_FORMAT_Z24X8_UNORM:
   case PIPE_FORMAT_Z32_FLOAT:
   case PIPE_FORMAT_Z24_UNORM_S8_UINT:
   case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
      return true;
   case PIPE_FORMAT_S8_UINT:
      /* TODO separate stencil */
   default:
      return false;
   }
}

static bool
ilo_format_supports_rt(const struct ilo_dev_info *dev,
                       enum pipe_format format)
{
   const int idx = ilo_translate_format(dev, format, PIPE_BIND_RENDER_TARGET);
   const struct ilo_dp_cap *cap = (idx >= 0 && idx < Elements(ilo_dp_caps)) ?
      &ilo_dp_caps[idx] : NULL;

   if (!cap || !cap->rt_write)
      return false;

   assert(!cap->rt_write_blending || cap->rt_write_blending >= cap->rt_write);

   if (util_format_is_pure_integer(format))
      return (ilo_dev_gen(dev) >= cap->rt_write);
   else if (cap->rt_write_blending)
      return (ilo_dev_gen(dev) >= cap->rt_write_blending);
   else
      return false;
}

static bool
ilo_format_supports_sampler(const struct ilo_dev_info *dev,
                            enum pipe_format format)
{
   const int idx = ilo_translate_format(dev, format, PIPE_BIND_SAMPLER_VIEW);
   const struct ilo_sampler_cap *cap = (idx >= 0 &&
         idx < Elements(ilo_sampler_caps)) ? &ilo_sampler_caps[idx] : NULL;

   if (!cap || !cap->sampling)
      return false;

   assert(!cap->filtering || cap->filtering >= cap->sampling);

   if (util_format_is_pure_integer(format))
      return (ilo_dev_gen(dev) >= cap->sampling);
   else if (cap->filtering)
      return (ilo_dev_gen(dev) >= cap->filtering);
   else
      return false;
}

static bool
ilo_format_supports_vb(const struct ilo_dev_info *dev,
                       enum pipe_format format)
{
   const int idx = ilo_translate_format(dev, format, PIPE_BIND_VERTEX_BUFFER);
   const struct ilo_vf_cap *cap = (idx >= 0 && idx < Elements(ilo_vf_caps)) ?
      &ilo_vf_caps[idx] : NULL;

   return (cap && cap->vertex_element &&
         ilo_dev_gen(dev) >= cap->vertex_element);
}

static boolean
ilo_is_format_supported(struct pipe_screen *screen,
                        enum pipe_format format,
                        enum pipe_texture_target target,
                        unsigned sample_count,
                        unsigned bindings)
{
   struct ilo_screen *is = ilo_screen(screen);
   const struct ilo_dev_info *dev = &is->dev;
   unsigned bind;

   if (!util_format_is_supported(format, bindings))
      return false;

   /* no MSAA support yet */
   if (sample_count > 1)
      return false;

   bind = (bindings & PIPE_BIND_DEPTH_STENCIL);
   if (bind && !ilo_format_supports_zs(dev, format))
      return false;

   bind = (bindings & PIPE_BIND_RENDER_TARGET);
   if (bind && !ilo_format_supports_rt(dev, format))
      return false;

   bind = (bindings & PIPE_BIND_SAMPLER_VIEW);
   if (bind && !ilo_format_supports_sampler(dev, format))
      return false;

   bind = (bindings & PIPE_BIND_VERTEX_BUFFER);
   if (bind && !ilo_format_supports_vb(dev, format))
      return false;

   (void) ilo_sol_caps;

   return true;
}

static boolean
ilo_is_video_format_supported(struct pipe_screen *screen,
                              enum pipe_format format,
                              enum pipe_video_profile profile,
                              enum pipe_video_entrypoint entrypoint)
{
   return vl_video_buffer_is_format_supported(screen, format, profile, entrypoint);
}

/**
 * Initialize format-related functions.
 */
void
ilo_init_format_functions(struct ilo_screen *is)
{
   is->base.is_format_supported = ilo_is_format_supported;
   is->base.is_video_format_supported = ilo_is_video_format_supported;
}

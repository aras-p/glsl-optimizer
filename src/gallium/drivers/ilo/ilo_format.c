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

/* this idea is taken from i965 */
struct ilo_format_info {
   bool exists;
   int sampling;
   int filtering;
   int shadow_compare;
   int chroma_key;
   int render_target;
   int alpha_blend;
   int input_vb;
   int streamed_output_vb;
   int color_processing;
};

#define FI_INITIALIZER(exist, sampl, filt, shad, ck, rt, ab, vb, so, color) \
   { exist, ILO_GEN(sampl), ILO_GEN(filt), ILO_GEN(shad), ILO_GEN(ck),      \
     ILO_GEN(rt), ILO_GEN(ab), ILO_GEN(vb), ILO_GEN(so), ILO_GEN(color) }

#define FI_ENTRY(sampl, filt, shad, ck, rt, ab, vb, so, color, sf)          \
   [GEN6_FORMAT_ ## sf] = FI_INITIALIZER(true,                              \
         sampl, filt, shad, ck, rt, ab, vb, so, color)

#define X 999

static const struct ilo_format_info ilo_format_nonexist =
   FI_INITIALIZER(false, X, X, X, X, X, X, X, X, X);

/*
 * This table is based on:
 *
 *  - the Sandy Bridge PRM, volume 4 part 1, page 88-97
 *  - the Ivy Bridge PRM, volume 2 part 1, page 97-99 and 195
 *  - the Ivy Bridge PRM, volume 4 part 1, page 84-87, 172, and 277-278
 *  - the Haswell PRM, volume 7, page 262-264, 467-470, and 535.
 *  - i965 surface_format_info (for BC6/BC7)
 */
static const struct ilo_format_info ilo_format_table[] = {
/*        sampl filt shad   ck   rt   ab   vb   so  color */
   FI_ENTRY(  1,   5,   X,   X,   1,   1,   1,   1,   X, R32G32B32A32_FLOAT),
   FI_ENTRY(  1,   X,   X,   X,   1,   X,   1,   1,   X, R32G32B32A32_SINT),
   FI_ENTRY(  1,   X,   X,   X,   1,   X,   1,   1,   X, R32G32B32A32_UINT),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   1,   X,   X, R32G32B32A32_UNORM),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   1,   X,   X, R32G32B32A32_SNORM),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   1,   X,   X, R64G64_FLOAT),
   FI_ENTRY(  1,   5,   X,   X,   X,   X,   X,   X,   X, R32G32B32X32_FLOAT),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   1,   X,   X, R32G32B32A32_SSCALED),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   1,   X,   X, R32G32B32A32_USCALED),
   FI_ENTRY(  X,   X,   X,   X,   X,   X, 7.5,   X,   X, R32G32B32A32_SFIXED),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   X,   X,   X, R64G64_PASSTHRU),
   FI_ENTRY(  1,   5,   X,   X,   X,   X,   1,   1,   X, R32G32B32_FLOAT),
   FI_ENTRY(  1,   X,   X,   X,   X,   X,   1,   1,   X, R32G32B32_SINT),
   FI_ENTRY(  1,   X,   X,   X,   X,   X,   1,   1,   X, R32G32B32_UINT),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   1,   X,   X, R32G32B32_UNORM),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   1,   X,   X, R32G32B32_SNORM),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   1,   X,   X, R32G32B32_SSCALED),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   1,   X,   X, R32G32B32_USCALED),
   FI_ENTRY(  X,   X,   X,   X,   X,   X, 7.5,   X,   X, R32G32B32_SFIXED),
   FI_ENTRY(  1,   1,   X,   X,   1, 4.5,   1,   X,   6, R16G16B16A16_UNORM),
   FI_ENTRY(  1,   1,   X,   X,   1,   6,   1,   X,   X, R16G16B16A16_SNORM),
   FI_ENTRY(  1,   X,   X,   X,   1,   X,   1,   X,   X, R16G16B16A16_SINT),
   FI_ENTRY(  1,   X,   X,   X,   1,   X,   1,   X,   X, R16G16B16A16_UINT),
   FI_ENTRY(  1,   1,   X,   X,   1,   1,   1,   X,   X, R16G16B16A16_FLOAT),
   FI_ENTRY(  1,   5,   X,   X,   1,   1,   1,   1,   X, R32G32_FLOAT),
   FI_ENTRY(  1,   X,   X,   X,   1,   X,   1,   1,   X, R32G32_SINT),
   FI_ENTRY(  1,   X,   X,   X,   1,   X,   1,   1,   X, R32G32_UINT),
   FI_ENTRY(  1,   5,   1,   X,   X,   X,   X,   X,   X, R32_FLOAT_X8X24_TYPELESS),
   FI_ENTRY(  1,   X,   X,   X,   X,   X,   X,   X,   X, X32_TYPELESS_G8X24_UINT),
   FI_ENTRY(  1,   5,   X,   X,   X,   X,   X,   X,   X, L32A32_FLOAT),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   1,   X,   X, R32G32_UNORM),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   1,   X,   X, R32G32_SNORM),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   1,   X,   X, R64_FLOAT),
   FI_ENTRY(  1,   1,   X,   X,   X,   X,   X,   X,   X, R16G16B16X16_UNORM),
   FI_ENTRY(  1,   1,   X,   X,   X,   X,   X,   X,   X, R16G16B16X16_FLOAT),
   FI_ENTRY(  1,   5,   X,   X,   X,   X,   X,   X,   X, A32X32_FLOAT),
   FI_ENTRY(  1,   5,   X,   X,   X,   X,   X,   X,   X, L32X32_FLOAT),
   FI_ENTRY(  1,   5,   X,   X,   X,   X,   X,   X,   X, I32X32_FLOAT),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   1,   X,   X, R16G16B16A16_SSCALED),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   1,   X,   X, R16G16B16A16_USCALED),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   1,   X,   X, R32G32_SSCALED),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   1,   X,   X, R32G32_USCALED),
   FI_ENTRY(  X,   X,   X,   X,   X,   X, 7.5,   X,   X, R32G32_SFIXED),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   X,   X,   X, R64_PASSTHRU),
   FI_ENTRY(  1,   1,   X,   1,   1,   1,   1,   X,   6, B8G8R8A8_UNORM),
   FI_ENTRY(  1,   1,   X,   X,   1,   1,   X,   X,   X, B8G8R8A8_UNORM_SRGB),
   FI_ENTRY(  1,   1,   X,   X,   1,   1,   1,   X,   6, R10G10B10A2_UNORM),
   FI_ENTRY(  1,   1,   X,   X,   X,   X,   X,   X,   6, R10G10B10A2_UNORM_SRGB),
   FI_ENTRY(  1,   X,   X,   X,   1,   X,   1,   X,   X, R10G10B10A2_UINT),
   FI_ENTRY(  1,   1,   X,   X,   X,   X,   1,   X,   X, R10G10B10_SNORM_A2_UNORM),
   FI_ENTRY(  1,   1,   X,   X,   1,   1,   1,   X,   6, R8G8B8A8_UNORM),
   FI_ENTRY(  1,   1,   X,   X,   1,   1,   X,   X,   6, R8G8B8A8_UNORM_SRGB),
   FI_ENTRY(  1,   1,   X,   X,   1,   6,   1,   X,   X, R8G8B8A8_SNORM),
   FI_ENTRY(  1,   X,   X,   X,   1,   X,   1,   X,   X, R8G8B8A8_SINT),
   FI_ENTRY(  1,   X,   X,   X,   1,   X,   1,   X,   X, R8G8B8A8_UINT),
   FI_ENTRY(  1,   1,   X,   X,   1, 4.5,   1,   X,   X, R16G16_UNORM),
   FI_ENTRY(  1,   1,   X,   X,   1,   6,   1,   X,   X, R16G16_SNORM),
   FI_ENTRY(  1,   X,   X,   X,   1,   X,   1,   X,   X, R16G16_SINT),
   FI_ENTRY(  1,   X,   X,   X,   1,   X,   1,   X,   X, R16G16_UINT),
   FI_ENTRY(  1,   1,   X,   X,   1,   1,   1,   X,   X, R16G16_FLOAT),
   FI_ENTRY(  1,   1,   X,   X,   1,   1, 7.5,   X,   6, B10G10R10A2_UNORM),
   FI_ENTRY(  1,   1,   X,   X,   1,   1,   X,   X,   6, B10G10R10A2_UNORM_SRGB),
   FI_ENTRY(  1,   1,   X,   X,   1,   1,   1,   X,   X, R11G11B10_FLOAT),
   FI_ENTRY(  1,   X,   X,   X,   1,   X,   1,   1,   X, R32_SINT),
   FI_ENTRY(  1,   X,   X,   X,   1,   X,   1,   1,   X, R32_UINT),
   FI_ENTRY(  1,   5,   1,   X,   1,   1,   1,   1,   X, R32_FLOAT),
   FI_ENTRY(  1,   5,   1,   X,   X,   X,   X,   X,   X, R24_UNORM_X8_TYPELESS),
   FI_ENTRY(  1,   X,   X,   X,   X,   X,   X,   X,   X, X24_TYPELESS_G8_UINT),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   X,   X,   X, L32_UNORM),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   X,   X,   X, A32_UNORM),
   FI_ENTRY(  1,   1,   X,   X,   X,   X,   X,   X,   X, L16A16_UNORM),
   FI_ENTRY(  1,   5,   1,   X,   X,   X,   X,   X,   X, I24X8_UNORM),
   FI_ENTRY(  1,   5,   1,   X,   X,   X,   X,   X,   X, L24X8_UNORM),
   FI_ENTRY(  1,   5,   1,   X,   X,   X,   X,   X,   X, A24X8_UNORM),
   FI_ENTRY(  1,   5,   1,   X,   X,   X,   X,   X,   X, I32_FLOAT),
   FI_ENTRY(  1,   5,   1,   X,   X,   X,   X,   X,   X, L32_FLOAT),
   FI_ENTRY(  1,   5,   1,   X,   X,   X,   X,   X,   X, A32_FLOAT),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   X,   X,   X, X8B8_UNORM_G8R8_SNORM),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   X,   X,   X, A8X8_UNORM_G8R8_SNORM),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   X,   X,   X, B8X8_UNORM_G8R8_SNORM),
   FI_ENTRY(  1,   1,   X,   1,   X,   X,   X,   X,   6, B8G8R8X8_UNORM),
   FI_ENTRY(  1,   1,   X,   X,   X,   X,   X,   X,   X, B8G8R8X8_UNORM_SRGB),
   FI_ENTRY(  1,   1,   X,   X,   X,   X,   X,   X,   X, R8G8B8X8_UNORM),
   FI_ENTRY(  1,   1,   X,   X,   X,   X,   X,   X,   X, R8G8B8X8_UNORM_SRGB),
   FI_ENTRY(  1,   1,   X,   X,   X,   X,   X,   X,   X, R9G9B9E5_SHAREDEXP),
   FI_ENTRY(  1,   1,   X,   X,   X,   X,   X,   X,   X, B10G10R10X2_UNORM),
   FI_ENTRY(  1,   1,   X,   X,   X,   X,   X,   X,   X, L16A16_FLOAT),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   1,   X,   X, R32_UNORM),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   1,   X,   X, R32_SNORM),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   1,   X,   X, R10G10B10X2_USCALED),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   1,   X,   X, R8G8B8A8_SSCALED),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   1,   X,   X, R8G8B8A8_USCALED),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   1,   X,   X, R16G16_SSCALED),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   1,   X,   X, R16G16_USCALED),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   1,   X,   X, R32_SSCALED),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   1,   X,   X, R32_USCALED),
   FI_ENTRY(  1,   1,   X,   1,   1,   1,   X,   X,   X, B5G6R5_UNORM),
   FI_ENTRY(  1,   1,   X,   X,   1,   1,   X,   X,   X, B5G6R5_UNORM_SRGB),
   FI_ENTRY(  1,   1,   X,   1,   1,   1,   X,   X,   X, B5G5R5A1_UNORM),
   FI_ENTRY(  1,   1,   X,   X,   1,   1,   X,   X,   X, B5G5R5A1_UNORM_SRGB),
   FI_ENTRY(  1,   1,   X,   1,   1,   1,   X,   X,   X, B4G4R4A4_UNORM),
   FI_ENTRY(  1,   1,   X,   X,   1,   1,   X,   X,   X, B4G4R4A4_UNORM_SRGB),
   FI_ENTRY(  1,   1,   X,   X,   1,   1,   1,   X,   X, R8G8_UNORM),
   FI_ENTRY(  1,   1,   X,   1,   1,   6,   1,   X,   X, R8G8_SNORM),
   FI_ENTRY(  1,   X,   X,   X,   1,   X,   1,   X,   X, R8G8_SINT),
   FI_ENTRY(  1,   X,   X,   X,   1,   X,   1,   X,   X, R8G8_UINT),
   FI_ENTRY(  1,   1,   1,   X,   1, 4.5,   1,   X,   7, R16_UNORM),
   FI_ENTRY(  1,   1,   X,   X,   1,   6,   1,   X,   X, R16_SNORM),
   FI_ENTRY(  1,   X,   X,   X,   1,   X,   1,   X,   X, R16_SINT),
   FI_ENTRY(  1,   X,   X,   X,   1,   X,   1,   X,   X, R16_UINT),
   FI_ENTRY(  1,   1,   X,   X,   1,   1,   1,   X,   X, R16_FLOAT),
   FI_ENTRY(  5,   5,   X,   X,   X,   X,   X,   X,   X, A8P8_UNORM_PALETTE0),
   FI_ENTRY(  5,   5,   X,   X,   X,   X,   X,   X,   X, A8P8_UNORM_PALETTE1),
   FI_ENTRY(  1,   1,   1,   X,   X,   X,   X,   X,   X, I16_UNORM),
   FI_ENTRY(  1,   1,   1,   X,   X,   X,   X,   X,   X, L16_UNORM),
   FI_ENTRY(  1,   1,   1,   X,   X,   X,   X,   X,   X, A16_UNORM),
   FI_ENTRY(  1,   1,   X,   1,   X,   X,   X,   X,   X, L8A8_UNORM),
   FI_ENTRY(  1,   1,   1,   X,   X,   X,   X,   X,   X, I16_FLOAT),
   FI_ENTRY(  1,   1,   1,   X,   X,   X,   X,   X,   X, L16_FLOAT),
   FI_ENTRY(  1,   1,   1,   X,   X,   X,   X,   X,   X, A16_FLOAT),
   FI_ENTRY(4.5, 4.5,   X,   X,   X,   X,   X,   X,   X, L8A8_UNORM_SRGB),
   FI_ENTRY(  1,   1,   X,   1,   X,   X,   X,   X,   X, R5G5_SNORM_B6_UNORM),
   FI_ENTRY(  X,   X,   X,   X,   1,   1,   X,   X,   X, B5G5R5X1_UNORM),
   FI_ENTRY(  X,   X,   X,   X,   1,   1,   X,   X,   X, B5G5R5X1_UNORM_SRGB),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   1,   X,   X, R8G8_SSCALED),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   1,   X,   X, R8G8_USCALED),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   1,   X,   X, R16_SSCALED),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   1,   X,   X, R16_USCALED),
   FI_ENTRY(  5,   5,   X,   X,   X,   X,   X,   X,   X, P8A8_UNORM_PALETTE0),
   FI_ENTRY(  5,   5,   X,   X,   X,   X,   X,   X,   X, P8A8_UNORM_PALETTE1),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   X,   X,   X, A1B5G5R5_UNORM),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   X,   X,   X, A4B4G4R4_UNORM),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   X,   X,   X, L8A8_UINT),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   X,   X,   X, L8A8_SINT),
   FI_ENTRY(  1,   1,   X, 4.5,   1,   1,   1,   X,   X, R8_UNORM),
   FI_ENTRY(  1,   1,   X,   X,   1,   6,   1,   X,   X, R8_SNORM),
   FI_ENTRY(  1,   X,   X,   X,   1,   X,   1,   X,   X, R8_SINT),
   FI_ENTRY(  1,   X,   X,   X,   1,   X,   1,   X,   X, R8_UINT),
   FI_ENTRY(  1,   1,   X,   1,   1,   1,   X,   X,   X, A8_UNORM),
   FI_ENTRY(  1,   1,   X,   X,   X,   X,   X,   X,   X, I8_UNORM),
   FI_ENTRY(  1,   1,   X,   1,   X,   X,   X,   X,   X, L8_UNORM),
   FI_ENTRY(  1,   1,   X,   X,   X,   X,   X,   X,   X, P4A4_UNORM_PALETTE0),
   FI_ENTRY(  1,   1,   X,   X,   X,   X,   X,   X,   X, A4P4_UNORM_PALETTE0),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   1,   X,   X, R8_SSCALED),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   1,   X,   X, R8_USCALED),
   FI_ENTRY(4.5, 4.5,   X,   X,   X,   X,   X,   X,   X, P8_UNORM_PALETTE0),
   FI_ENTRY(4.5, 4.5,   X,   X,   X,   X,   X,   X,   X, L8_UNORM_SRGB),
   FI_ENTRY(4.5, 4.5,   X,   X,   X,   X,   X,   X,   X, P8_UNORM_PALETTE1),
   FI_ENTRY(4.5, 4.5,   X,   X,   X,   X,   X,   X,   X, P4A4_UNORM_PALETTE1),
   FI_ENTRY(4.5, 4.5,   X,   X,   X,   X,   X,   X,   X, A4P4_UNORM_PALETTE1),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   X,   X,   X, Y8_UNORM),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   X,   X,   X, L8_UINT),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   X,   X,   X, L8_SINT),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   X,   X,   X, I8_UINT),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   X,   X,   X, I8_SINT),
   FI_ENTRY(4.5, 4.5,   X,   X,   X,   X,   X,   X,   X, DXT1_RGB_SRGB),
   FI_ENTRY(  1,   1,   X,   X,   X,   X,   X,   X,   X, R1_UNORM),
   FI_ENTRY(  1,   1,   X,   1,   1,   X,   X,   X,   6, YCRCB_NORMAL),
   FI_ENTRY(  1,   1,   X,   1,   1,   X,   X,   X,   6, YCRCB_SWAPUVY),
   FI_ENTRY(4.5, 4.5,   X,   X,   X,   X,   X,   X,   X, P2_UNORM_PALETTE0),
   FI_ENTRY(4.5, 4.5,   X,   X,   X,   X,   X,   X,   X, P2_UNORM_PALETTE1),
   FI_ENTRY(  1,   1,   X,   1,   X,   X,   X,   X,   X, BC1_UNORM),
   FI_ENTRY(  1,   1,   X,   1,   X,   X,   X,   X,   X, BC2_UNORM),
   FI_ENTRY(  1,   1,   X,   1,   X,   X,   X,   X,   X, BC3_UNORM),
   FI_ENTRY(  1,   1,   X,   X,   X,   X,   X,   X,   X, BC4_UNORM),
   FI_ENTRY(  1,   1,   X,   X,   X,   X,   X,   X,   X, BC5_UNORM),
   FI_ENTRY(  1,   1,   X,   X,   X,   X,   X,   X,   X, BC1_UNORM_SRGB),
   FI_ENTRY(  1,   1,   X,   X,   X,   X,   X,   X,   X, BC2_UNORM_SRGB),
   FI_ENTRY(  1,   1,   X,   X,   X,   X,   X,   X,   X, BC3_UNORM_SRGB),
   FI_ENTRY(  1,   X,   X,   X,   X,   X,   X,   X,   X, MONO8),
   FI_ENTRY(  1,   1,   X,   X,   1,   X,   X,   X,   6, YCRCB_SWAPUV),
   FI_ENTRY(  1,   1,   X,   X,   1,   X,   X,   X,   6, YCRCB_SWAPY),
   FI_ENTRY(  1,   1,   X,   X,   X,   X,   X,   X,   X, DXT1_RGB),
   FI_ENTRY(  1,   1,   X,   X,   X,   X,   X,   X,   X, FXT1),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   1,   X,   X, R8G8B8_UNORM),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   1,   X,   X, R8G8B8_SNORM),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   1,   X,   X, R8G8B8_SSCALED),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   1,   X,   X, R8G8B8_USCALED),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   1,   X,   X, R64G64B64A64_FLOAT),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   1,   X,   X, R64G64B64_FLOAT),
   FI_ENTRY(  1,   1,   X,   X,   X,   X,   X,   X,   X, BC4_SNORM),
   FI_ENTRY(  1,   1,   X,   X,   X,   X,   X,   X,   X, BC5_SNORM),
   FI_ENTRY(  5,   5,   X,   X,   X,   X,   6,   X,   X, R16G16B16_FLOAT),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   1,   X,   X, R16G16B16_UNORM),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   1,   X,   X, R16G16B16_SNORM),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   1,   X,   X, R16G16B16_SSCALED),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   1,   X,   X, R16G16B16_USCALED),
   FI_ENTRY(  7,   7,   X,   X,   X,   X,   X,   X,   X, BC6H_SF16),
   FI_ENTRY(  7,   7,   X,   X,   X,   X,   X,   X,   X, BC7_UNORM),
   FI_ENTRY(  7,   7,   X,   X,   X,   X,   X,   X,   X, BC7_UNORM_SRGB),
   FI_ENTRY(  7,   7,   X,   X,   X,   X,   X,   X,   X, BC6H_UF16),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   X,   X,   X, PLANAR_420_8),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   X,   X,   X, R8G8B8_UNORM_SRGB),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   X,   X,   X, ETC1_RGB8),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   X,   X,   X, ETC2_RGB8),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   X,   X,   X, EAC_R11),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   X,   X,   X, EAC_RG11),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   X,   X,   X, EAC_SIGNED_R11),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   X,   X,   X, EAC_SIGNED_RG11),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   X,   X,   X, ETC2_SRGB8),
   FI_ENTRY(  X,   X,   X,   X,   X,   X, 7.5,   X,   X, R16G16B16_UINT),
   FI_ENTRY(  X,   X,   X,   X,   X,   X, 7.5,   X,   X, R16G16B16_SINT),
   FI_ENTRY(  X,   X,   X,   X,   X,   X, 7.5,   X,   X, R32_SFIXED),
   FI_ENTRY(  X,   X,   X,   X,   X,   X, 7.5,   X,   X, R10G10B10A2_SNORM),
   FI_ENTRY(  X,   X,   X,   X,   X,   X, 7.5,   X,   X, R10G10B10A2_USCALED),
   FI_ENTRY(  X,   X,   X,   X,   X,   X, 7.5,   X,   X, R10G10B10A2_SSCALED),
   FI_ENTRY(  X,   X,   X,   X,   X,   X, 7.5,   X,   X, R10G10B10A2_SINT),
   FI_ENTRY(  X,   X,   X,   X,   X,   X, 7.5,   X,   X, B10G10R10A2_SNORM),
   FI_ENTRY(  X,   X,   X,   X,   X,   X, 7.5,   X,   X, B10G10R10A2_USCALED),
   FI_ENTRY(  X,   X,   X,   X,   X,   X, 7.5,   X,   X, B10G10R10A2_SSCALED),
   FI_ENTRY(  X,   X,   X,   X,   X,   X, 7.5,   X,   X, B10G10R10A2_UINT),
   FI_ENTRY(  X,   X,   X,   X,   X,   X, 7.5,   X,   X, B10G10R10A2_SINT),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   X,   X,   X, R64G64B64A64_PASSTHRU),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   X,   X,   X, R64G64B64_PASSTHRU),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   X,   X,   X, ETC2_RGB8_PTA),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   X,   X,   X, ETC2_SRGB8_PTA),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   X,   X,   X, ETC2_EAC_RGBA8),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   X,   X,   X, ETC2_EAC_SRGB8_A8),
   FI_ENTRY(  X,   X,   X,   X,   X,   X, 7.5,   X,   X, R8G8B8_UINT),
   FI_ENTRY(  X,   X,   X,   X,   X,   X, 7.5,   X,   X, R8G8B8_SINT),
   FI_ENTRY(  X,   X,   X,   X,   X,   X,   X,   X,   X, RAW),
};

#undef X
#undef FI_ENTRY
#undef FI_INITIALIZER

static const struct ilo_format_info *
lookup_format_info(const struct ilo_dev_info *dev,
                   enum pipe_format format, unsigned bind)
{
   const int surfaceformat = ilo_translate_format(dev, format, bind);

   return (surfaceformat >= 0 && surfaceformat < Elements(ilo_format_table) &&
           ilo_format_table[surfaceformat].exists) ?
      &ilo_format_table[surfaceformat] : &ilo_format_nonexist;
}

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

static boolean
ilo_is_format_supported(struct pipe_screen *screen,
                        enum pipe_format format,
                        enum pipe_texture_target target,
                        unsigned sample_count,
                        unsigned bindings)
{
   struct ilo_screen *is = ilo_screen(screen);
   const struct ilo_dev_info *dev = &is->dev;
   const bool is_pure_int = util_format_is_pure_integer(format);
   const struct ilo_format_info *info;
   unsigned bind;

   if (!util_format_is_supported(format, bindings))
      return false;

   /* no MSAA support yet */
   if (sample_count > 1)
      return false;

   bind = (bindings & PIPE_BIND_DEPTH_STENCIL);
   if (bind) {
      switch (format) {
      case PIPE_FORMAT_Z16_UNORM:
      case PIPE_FORMAT_Z24X8_UNORM:
      case PIPE_FORMAT_Z32_FLOAT:
      case PIPE_FORMAT_Z24_UNORM_S8_UINT:
      case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
         break;
      case PIPE_FORMAT_S8_UINT:
         /* TODO separate stencil */
      default:
         return false;
      }
   }

   bind = (bindings & PIPE_BIND_RENDER_TARGET);
   if (bind) {
      info = lookup_format_info(dev, format, bind);

      if (dev->gen < info->render_target)
         return false;

      if (!is_pure_int && dev->gen < info->alpha_blend)
         return false;
   }

   bind = (bindings & PIPE_BIND_SAMPLER_VIEW);
   if (bind) {
      info = lookup_format_info(dev, format, bind);

      if (dev->gen < info->sampling)
         return false;

      if (!is_pure_int && dev->gen < info->filtering)
         return false;
   }

   bind = (bindings & PIPE_BIND_VERTEX_BUFFER);
   if (bind) {
      info = lookup_format_info(dev, format, bind);

      if (dev->gen < info->input_vb)
         return false;
   }

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

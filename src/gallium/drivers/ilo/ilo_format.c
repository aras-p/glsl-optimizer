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

/* stolen from classic i965 */
struct surface_format_info {
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

/* This macro allows us to write the table almost as it appears in the PRM,
 * while restructuring it to turn it into the C code we want.
 */
#define SF(sampl, filt, shad, ck, rt, ab, vb, so, color, sf) \
   [sf] = { true, sampl, filt, shad, ck, rt, ab, vb, so, color },

#define Y 0
#define x 999
/**
 * This is the table of support for surface (texture, renderbuffer, and vertex
 * buffer, but not depthbuffer) formats across the various hardware generations.
 *
 * The table is formatted to match the documentation, except that the docs have
 * this ridiculous mapping of Y[*+~^#&] for "supported on DevWhatever".  To put
 * it in our table, here's the mapping:
 *
 * Y*: 45
 * Y+: 45 (g45/gm45)
 * Y~: 50 (gen5)
 * Y^: 60 (gen6)
 * Y#: 70 (gen7)
 *
 * The abbreviations in the header below are:
 * smpl  - Sampling Engine
 * filt  - Sampling Engine Filtering
 * shad  - Sampling Engine Shadow Map
 * CK    - Sampling Engine Chroma Key
 * RT    - Render Target
 * AB    - Alpha Blend Render Target
 * VB    - Input Vertex Buffer
 * SO    - Steamed Output Vertex Buffers (transform feedback)
 * color - Color Processing
 *
 * See page 88 of the Sandybridge PRM VOL4_Part1 PDF.
 *
 * As of Ivybridge, the columns are no longer in that table and the
 * information can be found spread across:
 *
 * - VOL2_Part1 section 2.5.11 Format Conversion (vertex fetch).
 * - VOL4_Part1 section 2.12.2.1.2 Sampler Output Channel Mapping.
 * - VOL4_Part1 section 3.9.11 Render Target Write.
 */
const struct surface_format_info surface_formats[] = {
/* smpl filt shad CK  RT  AB  VB  SO  color */
   SF( Y, 50,  x,  x,  Y,  Y,  Y,  Y,  x, GEN6_FORMAT_R32G32B32A32_FLOAT)
   SF( Y,  x,  x,  x,  Y,  x,  Y,  Y,  x, GEN6_FORMAT_R32G32B32A32_SINT)
   SF( Y,  x,  x,  x,  Y,  x,  Y,  Y,  x, GEN6_FORMAT_R32G32B32A32_UINT)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, GEN6_FORMAT_R32G32B32A32_UNORM)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, GEN6_FORMAT_R32G32B32A32_SNORM)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, GEN6_FORMAT_R64G64_FLOAT)
   SF( Y, 50,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_R32G32B32X32_FLOAT)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, GEN6_FORMAT_R32G32B32A32_SSCALED)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, GEN6_FORMAT_R32G32B32A32_USCALED)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_R32G32B32A32_SFIXED)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_R64G64_PASSTHRU)
   SF( Y, 50,  x,  x,  x,  x,  Y,  Y,  x, GEN6_FORMAT_R32G32B32_FLOAT)
   SF( Y,  x,  x,  x,  x,  x,  Y,  Y,  x, GEN6_FORMAT_R32G32B32_SINT)
   SF( Y,  x,  x,  x,  x,  x,  Y,  Y,  x, GEN6_FORMAT_R32G32B32_UINT)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, GEN6_FORMAT_R32G32B32_UNORM)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, GEN6_FORMAT_R32G32B32_SNORM)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, GEN6_FORMAT_R32G32B32_SSCALED)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, GEN6_FORMAT_R32G32B32_USCALED)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_R32G32B32_SFIXED)
   SF( Y,  Y,  x,  x,  Y, 45,  Y,  x, 60, GEN6_FORMAT_R16G16B16A16_UNORM)
   SF( Y,  Y,  x,  x,  Y, 60,  Y,  x,  x, GEN6_FORMAT_R16G16B16A16_SNORM)
   SF( Y,  x,  x,  x,  Y,  x,  Y,  x,  x, GEN6_FORMAT_R16G16B16A16_SINT)
   SF( Y,  x,  x,  x,  Y,  x,  Y,  x,  x, GEN6_FORMAT_R16G16B16A16_UINT)
   SF( Y,  Y,  x,  x,  Y,  Y,  Y,  x,  x, GEN6_FORMAT_R16G16B16A16_FLOAT)
   SF( Y, 50,  x,  x,  Y,  Y,  Y,  Y,  x, GEN6_FORMAT_R32G32_FLOAT)
   SF( Y,  x,  x,  x,  Y,  x,  Y,  Y,  x, GEN6_FORMAT_R32G32_SINT)
   SF( Y,  x,  x,  x,  Y,  x,  Y,  Y,  x, GEN6_FORMAT_R32G32_UINT)
   SF( Y, 50,  Y,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_R32_FLOAT_X8X24_TYPELESS)
   SF( Y,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_X32_TYPELESS_G8X24_UINT)
   SF( Y, 50,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_L32A32_FLOAT)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, GEN6_FORMAT_R32G32_UNORM)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, GEN6_FORMAT_R32G32_SNORM)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, GEN6_FORMAT_R64_FLOAT)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_R16G16B16X16_UNORM)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_R16G16B16X16_FLOAT)
   SF( Y, 50,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_A32X32_FLOAT)
   SF( Y, 50,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_L32X32_FLOAT)
   SF( Y, 50,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_I32X32_FLOAT)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, GEN6_FORMAT_R16G16B16A16_SSCALED)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, GEN6_FORMAT_R16G16B16A16_USCALED)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, GEN6_FORMAT_R32G32_SSCALED)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, GEN6_FORMAT_R32G32_USCALED)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_R32G32_SFIXED)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_R64_PASSTHRU)
   SF( Y,  Y,  x,  Y,  Y,  Y,  Y,  x, 60, GEN6_FORMAT_B8G8R8A8_UNORM)
   SF( Y,  Y,  x,  x,  Y,  Y,  x,  x,  x, GEN6_FORMAT_B8G8R8A8_UNORM_SRGB)
/* smpl filt shad CK  RT  AB  VB  SO  color */
   SF( Y,  Y,  x,  x,  Y,  Y,  Y,  x, 60, GEN6_FORMAT_R10G10B10A2_UNORM)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x, 60, GEN6_FORMAT_R10G10B10A2_UNORM_SRGB)
   SF( Y,  x,  x,  x,  Y,  x,  Y,  x,  x, GEN6_FORMAT_R10G10B10A2_UINT)
   SF( Y,  Y,  x,  x,  x,  Y,  Y,  x,  x, GEN6_FORMAT_R10G10B10_SNORM_A2_UNORM)
   SF( Y,  Y,  x,  x,  Y,  Y,  Y,  x, 60, GEN6_FORMAT_R8G8B8A8_UNORM)
   SF( Y,  Y,  x,  x,  Y,  Y,  x,  x, 60, GEN6_FORMAT_R8G8B8A8_UNORM_SRGB)
   SF( Y,  Y,  x,  x,  Y, 60,  Y,  x,  x, GEN6_FORMAT_R8G8B8A8_SNORM)
   SF( Y,  x,  x,  x,  Y,  x,  Y,  x,  x, GEN6_FORMAT_R8G8B8A8_SINT)
   SF( Y,  x,  x,  x,  Y,  x,  Y,  x,  x, GEN6_FORMAT_R8G8B8A8_UINT)
   SF( Y,  Y,  x,  x,  Y, 45,  Y,  x,  x, GEN6_FORMAT_R16G16_UNORM)
   SF( Y,  Y,  x,  x,  Y, 60,  Y,  x,  x, GEN6_FORMAT_R16G16_SNORM)
   SF( Y,  x,  x,  x,  Y,  x,  Y,  x,  x, GEN6_FORMAT_R16G16_SINT)
   SF( Y,  x,  x,  x,  Y,  x,  Y,  x,  x, GEN6_FORMAT_R16G16_UINT)
   SF( Y,  Y,  x,  x,  Y,  Y,  Y,  x,  x, GEN6_FORMAT_R16G16_FLOAT)
   SF( Y,  Y,  x,  x,  Y,  Y,  x,  x, 60, GEN6_FORMAT_B10G10R10A2_UNORM)
   SF( Y,  Y,  x,  x,  Y,  Y,  x,  x, 60, GEN6_FORMAT_B10G10R10A2_UNORM_SRGB)
   SF( Y,  Y,  x,  x,  Y,  Y,  Y,  x,  x, GEN6_FORMAT_R11G11B10_FLOAT)
   SF( Y,  x,  x,  x,  Y,  x,  Y,  Y,  x, GEN6_FORMAT_R32_SINT)
   SF( Y,  x,  x,  x,  Y,  x,  Y,  Y,  x, GEN6_FORMAT_R32_UINT)
   SF( Y, 50,  Y,  x,  Y,  Y,  Y,  Y,  x, GEN6_FORMAT_R32_FLOAT)
   SF( Y, 50,  Y,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_R24_UNORM_X8_TYPELESS)
   SF( Y,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_X24_TYPELESS_G8_UINT)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_L16A16_UNORM)
   SF( Y, 50,  Y,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_I24X8_UNORM)
   SF( Y, 50,  Y,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_L24X8_UNORM)
   SF( Y, 50,  Y,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_A24X8_UNORM)
   SF( Y, 50,  Y,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_I32_FLOAT)
   SF( Y, 50,  Y,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_L32_FLOAT)
   SF( Y, 50,  Y,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_A32_FLOAT)
   SF( Y,  Y,  x,  Y,  x,  x,  x,  x, 60, GEN6_FORMAT_B8G8R8X8_UNORM)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_B8G8R8X8_UNORM_SRGB)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_R8G8B8X8_UNORM)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_R8G8B8X8_UNORM_SRGB)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_R9G9B9E5_SHAREDEXP)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_B10G10R10X2_UNORM)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_L16A16_FLOAT)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, GEN6_FORMAT_R32_UNORM)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, GEN6_FORMAT_R32_SNORM)
/* smpl filt shad CK  RT  AB  VB  SO  color */
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, GEN6_FORMAT_R10G10B10X2_USCALED)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, GEN6_FORMAT_R8G8B8A8_SSCALED)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, GEN6_FORMAT_R8G8B8A8_USCALED)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, GEN6_FORMAT_R16G16_SSCALED)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, GEN6_FORMAT_R16G16_USCALED)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, GEN6_FORMAT_R32_SSCALED)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, GEN6_FORMAT_R32_USCALED)
   SF( Y,  Y,  x,  Y,  Y,  Y,  x,  x,  x, GEN6_FORMAT_B5G6R5_UNORM)
   SF( Y,  Y,  x,  x,  Y,  Y,  x,  x,  x, GEN6_FORMAT_B5G6R5_UNORM_SRGB)
   SF( Y,  Y,  x,  Y,  Y,  Y,  x,  x,  x, GEN6_FORMAT_B5G5R5A1_UNORM)
   SF( Y,  Y,  x,  x,  Y,  Y,  x,  x,  x, GEN6_FORMAT_B5G5R5A1_UNORM_SRGB)
   SF( Y,  Y,  x,  Y,  Y,  Y,  x,  x,  x, GEN6_FORMAT_B4G4R4A4_UNORM)
   SF( Y,  Y,  x,  x,  Y,  Y,  x,  x,  x, GEN6_FORMAT_B4G4R4A4_UNORM_SRGB)
   SF( Y,  Y,  x,  x,  Y,  Y,  Y,  x,  x, GEN6_FORMAT_R8G8_UNORM)
   SF( Y,  Y,  x,  Y,  Y, 60,  Y,  x,  x, GEN6_FORMAT_R8G8_SNORM)
   SF( Y,  x,  x,  x,  Y,  x,  Y,  x,  x, GEN6_FORMAT_R8G8_SINT)
   SF( Y,  x,  x,  x,  Y,  x,  Y,  x,  x, GEN6_FORMAT_R8G8_UINT)
   SF( Y,  Y,  Y,  x,  Y, 45,  Y,  x, 70, GEN6_FORMAT_R16_UNORM)
   SF( Y,  Y,  x,  x,  Y, 60,  Y,  x,  x, GEN6_FORMAT_R16_SNORM)
   SF( Y,  x,  x,  x,  Y,  x,  Y,  x,  x, GEN6_FORMAT_R16_SINT)
   SF( Y,  x,  x,  x,  Y,  x,  Y,  x,  x, GEN6_FORMAT_R16_UINT)
   SF( Y,  Y,  x,  x,  Y,  Y,  Y,  x,  x, GEN6_FORMAT_R16_FLOAT)
   SF(50, 50,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_A8P8_UNORM_PALETTE0)
   SF(50, 50,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_A8P8_UNORM_PALETTE1)
   SF( Y,  Y,  Y,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_I16_UNORM)
   SF( Y,  Y,  Y,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_L16_UNORM)
   SF( Y,  Y,  Y,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_A16_UNORM)
   SF( Y,  Y,  x,  Y,  x,  x,  x,  x,  x, GEN6_FORMAT_L8A8_UNORM)
   SF( Y,  Y,  Y,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_I16_FLOAT)
   SF( Y,  Y,  Y,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_L16_FLOAT)
   SF( Y,  Y,  Y,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_A16_FLOAT)
   SF(45, 45,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_L8A8_UNORM_SRGB)
   SF( Y,  Y,  x,  Y,  x,  x,  x,  x,  x, GEN6_FORMAT_R5G5_SNORM_B6_UNORM)
   SF( x,  x,  x,  x,  Y,  Y,  x,  x,  x, GEN6_FORMAT_B5G5R5X1_UNORM)
   SF( x,  x,  x,  x,  Y,  Y,  x,  x,  x, GEN6_FORMAT_B5G5R5X1_UNORM_SRGB)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, GEN6_FORMAT_R8G8_SSCALED)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, GEN6_FORMAT_R8G8_USCALED)
/* smpl filt shad CK  RT  AB  VB  SO  color */
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, GEN6_FORMAT_R16_SSCALED)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, GEN6_FORMAT_R16_USCALED)
   SF(50, 50,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_P8A8_UNORM_PALETTE0)
   SF(50, 50,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_P8A8_UNORM_PALETTE1)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_A1B5G5R5_UNORM)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_A4B4G4R4_UNORM)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_L8A8_UINT)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_L8A8_SINT)
   SF( Y,  Y,  x, 45,  Y,  Y,  Y,  x,  x, GEN6_FORMAT_R8_UNORM)
   SF( Y,  Y,  x,  x,  Y, 60,  Y,  x,  x, GEN6_FORMAT_R8_SNORM)
   SF( Y,  x,  x,  x,  Y,  x,  Y,  x,  x, GEN6_FORMAT_R8_SINT)
   SF( Y,  x,  x,  x,  Y,  x,  Y,  x,  x, GEN6_FORMAT_R8_UINT)
   SF( Y,  Y,  x,  Y,  Y,  Y,  x,  x,  x, GEN6_FORMAT_A8_UNORM)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_I8_UNORM)
   SF( Y,  Y,  x,  Y,  x,  x,  x,  x,  x, GEN6_FORMAT_L8_UNORM)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_P4A4_UNORM_PALETTE0)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_A4P4_UNORM_PALETTE0)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, GEN6_FORMAT_R8_SSCALED)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, GEN6_FORMAT_R8_USCALED)
   SF(45, 45,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_P8_UNORM_PALETTE0)
   SF(45, 45,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_L8_UNORM_SRGB)
   SF(45, 45,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_P8_UNORM_PALETTE1)
   SF(45, 45,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_P4A4_UNORM_PALETTE1)
   SF(45, 45,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_A4P4_UNORM_PALETTE1)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_Y8_UNORM)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_L8_UINT)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_L8_SINT)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_I8_UINT)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_I8_SINT)
   SF(45, 45,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_DXT1_RGB_SRGB)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_R1_UNORM)
   SF( Y,  Y,  x,  Y,  Y,  x,  x,  x, 60, GEN6_FORMAT_YCRCB_NORMAL)
   SF( Y,  Y,  x,  Y,  Y,  x,  x,  x, 60, GEN6_FORMAT_YCRCB_SWAPUVY)
   SF(45, 45,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_P2_UNORM_PALETTE0)
   SF(45, 45,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_P2_UNORM_PALETTE1)
   SF( Y,  Y,  x,  Y,  x,  x,  x,  x,  x, GEN6_FORMAT_BC1_UNORM)
   SF( Y,  Y,  x,  Y,  x,  x,  x,  x,  x, GEN6_FORMAT_BC2_UNORM)
   SF( Y,  Y,  x,  Y,  x,  x,  x,  x,  x, GEN6_FORMAT_BC3_UNORM)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_BC4_UNORM)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_BC5_UNORM)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_BC1_UNORM_SRGB)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_BC2_UNORM_SRGB)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_BC3_UNORM_SRGB)
   SF( Y,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_MONO8)
   SF( Y,  Y,  x,  x,  Y,  x,  x,  x, 60, GEN6_FORMAT_YCRCB_SWAPUV)
   SF( Y,  Y,  x,  x,  Y,  x,  x,  x, 60, GEN6_FORMAT_YCRCB_SWAPY)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_DXT1_RGB)
/* smpl filt shad CK  RT  AB  VB  SO  color */
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_FXT1)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, GEN6_FORMAT_R8G8B8_UNORM)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, GEN6_FORMAT_R8G8B8_SNORM)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, GEN6_FORMAT_R8G8B8_SSCALED)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, GEN6_FORMAT_R8G8B8_USCALED)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, GEN6_FORMAT_R64G64B64A64_FLOAT)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, GEN6_FORMAT_R64G64B64_FLOAT)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_BC4_SNORM)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_BC5_SNORM)
   SF(50, 50,  x,  x,  x,  x, 60,  x,  x, GEN6_FORMAT_R16G16B16_FLOAT)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, GEN6_FORMAT_R16G16B16_UNORM)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, GEN6_FORMAT_R16G16B16_SNORM)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, GEN6_FORMAT_R16G16B16_SSCALED)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, GEN6_FORMAT_R16G16B16_USCALED)
   SF(70, 70,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_BC6H_SF16)
   SF(70, 70,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_BC7_UNORM)
   SF(70, 70,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_BC7_UNORM_SRGB)
   SF(70, 70,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_BC6H_UF16)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_PLANAR_420_8)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_R8G8B8_UNORM_SRGB)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_ETC1_RGB8)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_ETC2_RGB8)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_EAC_R11)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_EAC_RG11)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_EAC_SIGNED_R11)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_EAC_SIGNED_RG11)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_ETC2_SRGB8)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_R16G16B16_UINT)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_R16G16B16_SINT)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_R32_SFIXED)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_R10G10B10A2_SNORM)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_R10G10B10A2_USCALED)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_R10G10B10A2_SSCALED)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_R10G10B10A2_SINT)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_B10G10R10A2_SNORM)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_B10G10R10A2_USCALED)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_B10G10R10A2_SSCALED)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_B10G10R10A2_UINT)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_B10G10R10A2_SINT)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_R64G64B64A64_PASSTHRU)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_R64G64B64_PASSTHRU)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_ETC2_RGB8_PTA)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_ETC2_SRGB8_PTA)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_ETC2_EAC_RGBA8)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_ETC2_EAC_SRGB8_A8)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_R8G8B8_UINT)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, GEN6_FORMAT_R8G8B8_SINT)
};
#undef x
#undef Y

static const struct surface_format_info *
lookup_surface_format_info(enum pipe_format format, unsigned bind)
{
   static const struct surface_format_info nonexist = {
      .exists = false,
      .sampling = 999,
      .filtering = 999,
      .shadow_compare = 999,
      .chroma_key = 999,
      .render_target = 999,
      .alpha_blend = 999,
      .input_vb = 999,
      .streamed_output_vb = 999,
      .color_processing = 999,
   };
   const int surfaceformat = ilo_translate_format(format, bind);

   return (surfaceformat >= 0 && surfaceformat < Elements(surface_formats) &&
           surface_formats[surfaceformat].exists) ?
      &surface_formats[surfaceformat] : &nonexist;
}

/**
 * Translate a color (non-depth/stencil) pipe format to the matching hardware
 * format.  Return -1 on errors.
 */
int
ilo_translate_color_format(enum pipe_format format)
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
      [PIPE_FORMAT_B8G8R8X8_SRGB]         = 0,
      [PIPE_FORMAT_A8R8G8B8_SRGB]         = 0,
      [PIPE_FORMAT_X8R8G8B8_SRGB]         = 0,
      [PIPE_FORMAT_R8G8B8A8_SRGB]         = 0,
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
      [PIPE_FORMAT_R1_UNORM]              = 0,
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
   const int gen = ILO_GEN_GET_MAJOR(is->dev.gen * 10);
   const bool is_pure_int = util_format_is_pure_integer(format);
   const struct surface_format_info *info;
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
      info = lookup_surface_format_info(format, bind);

      if (gen < info->render_target)
         return false;

      if (!is_pure_int && gen < info->alpha_blend)
         return false;
   }

   bind = (bindings & PIPE_BIND_SAMPLER_VIEW);
   if (bind) {
      info = lookup_surface_format_info(format, bind);

      if (gen < info->sampling)
         return false;

      if (!is_pure_int && gen < info->filtering)
         return false;
   }

   bind = (bindings & PIPE_BIND_VERTEX_BUFFER);
   if (bind) {
      info = lookup_surface_format_info(format, bind);

      if (gen < info->input_vb)
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

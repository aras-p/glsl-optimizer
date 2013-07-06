/*
 * Copyright © 2011 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#include "main/context.h"
#include "main/mtypes.h"

#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"
#include "brw_wm.h"

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
   SF( Y, 50,  x,  x,  Y,  Y,  Y,  Y,  x, BRW_SURFACEFORMAT_R32G32B32A32_FLOAT)
   SF( Y,  x,  x,  x,  Y,  x,  Y,  Y,  x, BRW_SURFACEFORMAT_R32G32B32A32_SINT)
   SF( Y,  x,  x,  x,  Y,  x,  Y,  Y,  x, BRW_SURFACEFORMAT_R32G32B32A32_UINT)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R32G32B32A32_UNORM)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R32G32B32A32_SNORM)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R64G64_FLOAT)
   SF( Y, 50,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_R32G32B32X32_FLOAT)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R32G32B32A32_SSCALED)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R32G32B32A32_USCALED)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_R32G32B32A32_SFIXED)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_R64G64_PASSTHRU)
   SF( Y, 50,  x,  x,  x,  x,  Y,  Y,  x, BRW_SURFACEFORMAT_R32G32B32_FLOAT)
   SF( Y,  x,  x,  x,  x,  x,  Y,  Y,  x, BRW_SURFACEFORMAT_R32G32B32_SINT)
   SF( Y,  x,  x,  x,  x,  x,  Y,  Y,  x, BRW_SURFACEFORMAT_R32G32B32_UINT)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R32G32B32_UNORM)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R32G32B32_SNORM)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R32G32B32_SSCALED)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R32G32B32_USCALED)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_R32G32B32_SFIXED)
   SF( Y,  Y,  x,  x,  Y, 45,  Y,  x, 60, BRW_SURFACEFORMAT_R16G16B16A16_UNORM)
   SF( Y,  Y,  x,  x,  Y, 60,  Y,  x,  x, BRW_SURFACEFORMAT_R16G16B16A16_SNORM)
   SF( Y,  x,  x,  x,  Y,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R16G16B16A16_SINT)
   SF( Y,  x,  x,  x,  Y,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R16G16B16A16_UINT)
   SF( Y,  Y,  x,  x,  Y,  Y,  Y,  x,  x, BRW_SURFACEFORMAT_R16G16B16A16_FLOAT)
   SF( Y, 50,  x,  x,  Y,  Y,  Y,  Y,  x, BRW_SURFACEFORMAT_R32G32_FLOAT)
   SF( Y,  x,  x,  x,  Y,  x,  Y,  Y,  x, BRW_SURFACEFORMAT_R32G32_SINT)
   SF( Y,  x,  x,  x,  Y,  x,  Y,  Y,  x, BRW_SURFACEFORMAT_R32G32_UINT)
   SF( Y, 50,  Y,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_R32_FLOAT_X8X24_TYPELESS)
   SF( Y,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_X32_TYPELESS_G8X24_UINT)
   SF( Y, 50,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_L32A32_FLOAT)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R32G32_UNORM)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R32G32_SNORM)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R64_FLOAT)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_R16G16B16X16_UNORM)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_R16G16B16X16_FLOAT)
   SF( Y, 50,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_A32X32_FLOAT)
   SF( Y, 50,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_L32X32_FLOAT)
   SF( Y, 50,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_I32X32_FLOAT)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R16G16B16A16_SSCALED)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R16G16B16A16_USCALED)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R32G32_SSCALED)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R32G32_USCALED)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_R32G32_SFIXED)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_R64_PASSTHRU)
   SF( Y,  Y,  x,  Y,  Y,  Y,  Y,  x, 60, BRW_SURFACEFORMAT_B8G8R8A8_UNORM)
   SF( Y,  Y,  x,  x,  Y,  Y,  x,  x,  x, BRW_SURFACEFORMAT_B8G8R8A8_UNORM_SRGB)
/* smpl filt shad CK  RT  AB  VB  SO  color */
   SF( Y,  Y,  x,  x,  Y,  Y,  Y,  x, 60, BRW_SURFACEFORMAT_R10G10B10A2_UNORM)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x, 60, BRW_SURFACEFORMAT_R10G10B10A2_UNORM_SRGB)
   SF( Y,  x,  x,  x,  Y,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R10G10B10A2_UINT)
   SF( Y,  Y,  x,  x,  x,  Y,  Y,  x,  x, BRW_SURFACEFORMAT_R10G10B10_SNORM_A2_UNORM)
   SF( Y,  Y,  x,  x,  Y,  Y,  Y,  x, 60, BRW_SURFACEFORMAT_R8G8B8A8_UNORM)
   SF( Y,  Y,  x,  x,  Y,  Y,  x,  x, 60, BRW_SURFACEFORMAT_R8G8B8A8_UNORM_SRGB)
   SF( Y,  Y,  x,  x,  Y, 60,  Y,  x,  x, BRW_SURFACEFORMAT_R8G8B8A8_SNORM)
   SF( Y,  x,  x,  x,  Y,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R8G8B8A8_SINT)
   SF( Y,  x,  x,  x,  Y,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R8G8B8A8_UINT)
   SF( Y,  Y,  x,  x,  Y, 45,  Y,  x,  x, BRW_SURFACEFORMAT_R16G16_UNORM)
   SF( Y,  Y,  x,  x,  Y, 60,  Y,  x,  x, BRW_SURFACEFORMAT_R16G16_SNORM)
   SF( Y,  x,  x,  x,  Y,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R16G16_SINT)
   SF( Y,  x,  x,  x,  Y,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R16G16_UINT)
   SF( Y,  Y,  x,  x,  Y,  Y,  Y,  x,  x, BRW_SURFACEFORMAT_R16G16_FLOAT)
   SF( Y,  Y,  x,  x,  Y,  Y,  x,  x, 60, BRW_SURFACEFORMAT_B10G10R10A2_UNORM)
   SF( Y,  Y,  x,  x,  Y,  Y,  x,  x, 60, BRW_SURFACEFORMAT_B10G10R10A2_UNORM_SRGB)
   SF( Y,  Y,  x,  x,  Y,  Y,  Y,  x,  x, BRW_SURFACEFORMAT_R11G11B10_FLOAT)
   SF( Y,  x,  x,  x,  Y,  x,  Y,  Y,  x, BRW_SURFACEFORMAT_R32_SINT)
   SF( Y,  x,  x,  x,  Y,  x,  Y,  Y,  x, BRW_SURFACEFORMAT_R32_UINT)
   SF( Y, 50,  Y,  x,  Y,  Y,  Y,  Y,  x, BRW_SURFACEFORMAT_R32_FLOAT)
   SF( Y, 50,  Y,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_R24_UNORM_X8_TYPELESS)
   SF( Y,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_X24_TYPELESS_G8_UINT)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_L16A16_UNORM)
   SF( Y, 50,  Y,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_I24X8_UNORM)
   SF( Y, 50,  Y,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_L24X8_UNORM)
   SF( Y, 50,  Y,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_A24X8_UNORM)
   SF( Y, 50,  Y,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_I32_FLOAT)
   SF( Y, 50,  Y,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_L32_FLOAT)
   SF( Y, 50,  Y,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_A32_FLOAT)
   SF( Y,  Y,  x,  Y,  x,  x,  x,  x, 60, BRW_SURFACEFORMAT_B8G8R8X8_UNORM)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_B8G8R8X8_UNORM_SRGB)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_R8G8B8X8_UNORM)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_R8G8B8X8_UNORM_SRGB)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_R9G9B9E5_SHAREDEXP)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_B10G10R10X2_UNORM)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_L16A16_FLOAT)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R32_UNORM)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R32_SNORM)
/* smpl filt shad CK  RT  AB  VB  SO  color */
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R10G10B10X2_USCALED)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R8G8B8A8_SSCALED)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R8G8B8A8_USCALED)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R16G16_SSCALED)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R16G16_USCALED)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R32_SSCALED)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R32_USCALED)
   SF( Y,  Y,  x,  Y,  Y,  Y,  x,  x,  x, BRW_SURFACEFORMAT_B5G6R5_UNORM)
   SF( Y,  Y,  x,  x,  Y,  Y,  x,  x,  x, BRW_SURFACEFORMAT_B5G6R5_UNORM_SRGB)
   SF( Y,  Y,  x,  Y,  Y,  Y,  x,  x,  x, BRW_SURFACEFORMAT_B5G5R5A1_UNORM)
   SF( Y,  Y,  x,  x,  Y,  Y,  x,  x,  x, BRW_SURFACEFORMAT_B5G5R5A1_UNORM_SRGB)
   SF( Y,  Y,  x,  Y,  Y,  Y,  x,  x,  x, BRW_SURFACEFORMAT_B4G4R4A4_UNORM)
   SF( Y,  Y,  x,  x,  Y,  Y,  x,  x,  x, BRW_SURFACEFORMAT_B4G4R4A4_UNORM_SRGB)
   SF( Y,  Y,  x,  x,  Y,  Y,  Y,  x,  x, BRW_SURFACEFORMAT_R8G8_UNORM)
   SF( Y,  Y,  x,  Y,  Y, 60,  Y,  x,  x, BRW_SURFACEFORMAT_R8G8_SNORM)
   SF( Y,  x,  x,  x,  Y,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R8G8_SINT)
   SF( Y,  x,  x,  x,  Y,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R8G8_UINT)
   SF( Y,  Y,  Y,  x,  Y, 45,  Y,  x, 70, BRW_SURFACEFORMAT_R16_UNORM)
   SF( Y,  Y,  x,  x,  Y, 60,  Y,  x,  x, BRW_SURFACEFORMAT_R16_SNORM)
   SF( Y,  x,  x,  x,  Y,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R16_SINT)
   SF( Y,  x,  x,  x,  Y,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R16_UINT)
   SF( Y,  Y,  x,  x,  Y,  Y,  Y,  x,  x, BRW_SURFACEFORMAT_R16_FLOAT)
   SF(50, 50,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_A8P8_UNORM_PALETTE0)
   SF(50, 50,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_A8P8_UNORM_PALETTE1)
   SF( Y,  Y,  Y,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_I16_UNORM)
   SF( Y,  Y,  Y,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_L16_UNORM)
   SF( Y,  Y,  Y,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_A16_UNORM)
   SF( Y,  Y,  x,  Y,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_L8A8_UNORM)
   SF( Y,  Y,  Y,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_I16_FLOAT)
   SF( Y,  Y,  Y,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_L16_FLOAT)
   SF( Y,  Y,  Y,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_A16_FLOAT)
   SF(45, 45,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_L8A8_UNORM_SRGB)
   SF( Y,  Y,  x,  Y,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_R5G5_SNORM_B6_UNORM)
   SF( x,  x,  x,  x,  Y,  Y,  x,  x,  x, BRW_SURFACEFORMAT_B5G5R5X1_UNORM)
   SF( x,  x,  x,  x,  Y,  Y,  x,  x,  x, BRW_SURFACEFORMAT_B5G5R5X1_UNORM_SRGB)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R8G8_SSCALED)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R8G8_USCALED)
/* smpl filt shad CK  RT  AB  VB  SO  color */
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R16_SSCALED)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R16_USCALED)
   SF(50, 50,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_P8A8_UNORM_PALETTE0)
   SF(50, 50,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_P8A8_UNORM_PALETTE1)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_A1B5G5R5_UNORM)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_A4B4G4R4_UNORM)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_L8A8_UINT)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_L8A8_SINT)
   SF( Y,  Y,  x, 45,  Y,  Y,  Y,  x,  x, BRW_SURFACEFORMAT_R8_UNORM)
   SF( Y,  Y,  x,  x,  Y, 60,  Y,  x,  x, BRW_SURFACEFORMAT_R8_SNORM)
   SF( Y,  x,  x,  x,  Y,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R8_SINT)
   SF( Y,  x,  x,  x,  Y,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R8_UINT)
   SF( Y,  Y,  x,  Y,  Y,  Y,  x,  x,  x, BRW_SURFACEFORMAT_A8_UNORM)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_I8_UNORM)
   SF( Y,  Y,  x,  Y,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_L8_UNORM)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_P4A4_UNORM)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_A4P4_UNORM)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R8_SSCALED)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R8_USCALED)
   SF(45, 45,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_P8_UNORM_PALETTE0)
   SF(45, 45,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_L8_UNORM_SRGB)
   SF(45, 45,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_P8_UNORM_PALETTE1)
   SF(45, 45,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_P4A4_UNORM_PALETTE1)
   SF(45, 45,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_A4P4_UNORM_PALETTE1)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_Y8_SNORM)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_L8_UINT)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_L8_SINT)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_I8_UINT)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_I8_SINT)
   SF(45, 45,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_DXT1_RGB_SRGB)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_R1_UINT)
   SF( Y,  Y,  x,  Y,  Y,  x,  x,  x, 60, BRW_SURFACEFORMAT_YCRCB_NORMAL)
   SF( Y,  Y,  x,  Y,  Y,  x,  x,  x, 60, BRW_SURFACEFORMAT_YCRCB_SWAPUVY)
   SF(45, 45,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_P2_UNORM_PALETTE0)
   SF(45, 45,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_P2_UNORM_PALETTE1)
   SF( Y,  Y,  x,  Y,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_BC1_UNORM)
   SF( Y,  Y,  x,  Y,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_BC2_UNORM)
   SF( Y,  Y,  x,  Y,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_BC3_UNORM)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_BC4_UNORM)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_BC5_UNORM)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_BC1_UNORM_SRGB)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_BC2_UNORM_SRGB)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_BC3_UNORM_SRGB)
   SF( Y,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_MONO8)
   SF( Y,  Y,  x,  x,  Y,  x,  x,  x, 60, BRW_SURFACEFORMAT_YCRCB_SWAPUV)
   SF( Y,  Y,  x,  x,  Y,  x,  x,  x, 60, BRW_SURFACEFORMAT_YCRCB_SWAPY)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_DXT1_RGB)
/* smpl filt shad CK  RT  AB  VB  SO  color */
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_FXT1)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R8G8B8_UNORM)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R8G8B8_SNORM)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R8G8B8_SSCALED)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R8G8B8_USCALED)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R64G64B64A64_FLOAT)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R64G64B64_FLOAT)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_BC4_SNORM)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_BC5_SNORM)
   SF(50, 50,  x,  x,  x,  x, 60,  x,  x, BRW_SURFACEFORMAT_R16G16B16_FLOAT)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R16G16B16_UNORM)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R16G16B16_SNORM)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R16G16B16_SSCALED)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R16G16B16_USCALED)
   SF(70, 70,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_BC6H_SF16)
   SF(70, 70,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_BC7_UNORM)
   SF(70, 70,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_BC7_UNORM_SRGB)
   SF(70, 70,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_BC6H_UF16)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_PLANAR_420_8)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_R8G8B8_UNORM_SRGB)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_ETC1_RGB8)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_ETC2_RGB8)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_EAC_R11)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_EAC_RG11)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_EAC_SIGNED_R11)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_EAC_SIGNED_RG11)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_ETC2_SRGB8)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_R16G16B16_UINT)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_R16G16B16_SINT)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_R32_SFIXED)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_R10G10B10A2_SNORM)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_R10G10B10A2_USCALED)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_R10G10B10A2_SSCALED)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_R10G10B10A2_SINT)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_B10G10R10A2_SNORM)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_B10G10R10A2_USCALED)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_B10G10R10A2_SSCALED)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_B10G10R10A2_UINT)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_B10G10R10A2_SINT)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_R64G64B64A64_PASSTHRU)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_R64G64B64_PASSTHRU)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_ETC2_RGB8_PTA)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_ETC2_SRGB8_PTA)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_ETC2_EAC_RGBA8)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_ETC2_EAC_SRGB8_A8)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_R8G8B8_UINT)
   SF( x,  x,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_R8G8B8_SINT)
};
#undef x
#undef Y

uint32_t
brw_format_for_mesa_format(gl_format mesa_format)
{
   /* This table is ordered according to the enum ordering in formats.h.  We do
    * expect that enum to be extended without our explicit initialization
    * staying in sync, so we initialize to 0 even though
    * BRW_SURFACEFORMAT_R32G32B32A32_FLOAT happens to also be 0.
    */
   static const uint32_t table[MESA_FORMAT_COUNT] =
   {
      [MESA_FORMAT_RGBA8888] = 0,
      [MESA_FORMAT_RGBA8888_REV] = BRW_SURFACEFORMAT_R8G8B8A8_UNORM,
      [MESA_FORMAT_ARGB8888] = BRW_SURFACEFORMAT_B8G8R8A8_UNORM,
      [MESA_FORMAT_ARGB8888_REV] = 0,
      [MESA_FORMAT_RGBX8888] = 0,
      [MESA_FORMAT_RGBX8888_REV] = BRW_SURFACEFORMAT_R8G8B8X8_UNORM,
      [MESA_FORMAT_XRGB8888] = BRW_SURFACEFORMAT_B8G8R8X8_UNORM,
      [MESA_FORMAT_XRGB8888_REV] = 0,
      [MESA_FORMAT_RGB888] = 0,
      [MESA_FORMAT_BGR888] = BRW_SURFACEFORMAT_R8G8B8_UNORM,
      [MESA_FORMAT_RGB565] = BRW_SURFACEFORMAT_B5G6R5_UNORM,
      [MESA_FORMAT_RGB565_REV] = 0,
      [MESA_FORMAT_ARGB4444] = BRW_SURFACEFORMAT_B4G4R4A4_UNORM,
      [MESA_FORMAT_ARGB4444_REV] = 0,
      [MESA_FORMAT_RGBA5551] = 0,
      [MESA_FORMAT_ARGB1555] = BRW_SURFACEFORMAT_B5G5R5A1_UNORM,
      [MESA_FORMAT_ARGB1555_REV] = 0,
      [MESA_FORMAT_AL44] = 0,
      [MESA_FORMAT_AL88] = BRW_SURFACEFORMAT_L8A8_UNORM,
      [MESA_FORMAT_AL88_REV] = 0,
      [MESA_FORMAT_AL1616] = BRW_SURFACEFORMAT_L16A16_UNORM,
      [MESA_FORMAT_AL1616_REV] = 0,
      [MESA_FORMAT_RGB332] = 0,
      [MESA_FORMAT_A8] = BRW_SURFACEFORMAT_A8_UNORM,
      [MESA_FORMAT_A16] = BRW_SURFACEFORMAT_A16_UNORM,
      [MESA_FORMAT_L8] = BRW_SURFACEFORMAT_L8_UNORM,
      [MESA_FORMAT_L16] = BRW_SURFACEFORMAT_L16_UNORM,
      [MESA_FORMAT_I8] = BRW_SURFACEFORMAT_I8_UNORM,
      [MESA_FORMAT_I16] = BRW_SURFACEFORMAT_I16_UNORM,
      [MESA_FORMAT_YCBCR_REV] = BRW_SURFACEFORMAT_YCRCB_NORMAL,
      [MESA_FORMAT_YCBCR] = BRW_SURFACEFORMAT_YCRCB_SWAPUVY,
      [MESA_FORMAT_R8] = BRW_SURFACEFORMAT_R8_UNORM,
      [MESA_FORMAT_GR88] = BRW_SURFACEFORMAT_R8G8_UNORM,
      [MESA_FORMAT_RG88] = 0,
      [MESA_FORMAT_R16] = BRW_SURFACEFORMAT_R16_UNORM,
      [MESA_FORMAT_GR1616] = BRW_SURFACEFORMAT_R16G16_UNORM,
      [MESA_FORMAT_RG1616] = 0,
      [MESA_FORMAT_ARGB2101010] = BRW_SURFACEFORMAT_B10G10R10A2_UNORM,
      [MESA_FORMAT_Z24_S8] = 0,
      [MESA_FORMAT_S8_Z24] = 0,
      [MESA_FORMAT_Z16] = 0,
      [MESA_FORMAT_X8_Z24] = 0,
      [MESA_FORMAT_Z24_X8] = 0,
      [MESA_FORMAT_Z32] = 0,
      [MESA_FORMAT_S8] = 0,

      [MESA_FORMAT_SRGB8] = 0,
      [MESA_FORMAT_SRGBA8] = 0,
      [MESA_FORMAT_SARGB8] = BRW_SURFACEFORMAT_B8G8R8A8_UNORM_SRGB,
      [MESA_FORMAT_SL8] = BRW_SURFACEFORMAT_L8_UNORM_SRGB,
      [MESA_FORMAT_SLA8] = BRW_SURFACEFORMAT_L8A8_UNORM_SRGB,
      [MESA_FORMAT_SRGB_DXT1] = BRW_SURFACEFORMAT_DXT1_RGB_SRGB,
      [MESA_FORMAT_SRGBA_DXT1] = BRW_SURFACEFORMAT_BC1_UNORM_SRGB,
      [MESA_FORMAT_SRGBA_DXT3] = BRW_SURFACEFORMAT_BC2_UNORM_SRGB,
      [MESA_FORMAT_SRGBA_DXT5] = BRW_SURFACEFORMAT_BC3_UNORM_SRGB,

      [MESA_FORMAT_RGB_FXT1] = BRW_SURFACEFORMAT_FXT1,
      [MESA_FORMAT_RGBA_FXT1] = BRW_SURFACEFORMAT_FXT1,
      [MESA_FORMAT_RGB_DXT1] = BRW_SURFACEFORMAT_DXT1_RGB,
      [MESA_FORMAT_RGBA_DXT1] = BRW_SURFACEFORMAT_BC1_UNORM,
      [MESA_FORMAT_RGBA_DXT3] = BRW_SURFACEFORMAT_BC2_UNORM,
      [MESA_FORMAT_RGBA_DXT5] = BRW_SURFACEFORMAT_BC3_UNORM,

      [MESA_FORMAT_RGBA_FLOAT32] = BRW_SURFACEFORMAT_R32G32B32A32_FLOAT,
      [MESA_FORMAT_RGBA_FLOAT16] = BRW_SURFACEFORMAT_R16G16B16A16_FLOAT,
      [MESA_FORMAT_RGB_FLOAT32] = BRW_SURFACEFORMAT_R32G32B32_FLOAT,
      [MESA_FORMAT_RGB_FLOAT16] = BRW_SURFACEFORMAT_R16G16B16_FLOAT,
      [MESA_FORMAT_ALPHA_FLOAT32] = BRW_SURFACEFORMAT_A32_FLOAT,
      [MESA_FORMAT_ALPHA_FLOAT16] = BRW_SURFACEFORMAT_A16_FLOAT,
      [MESA_FORMAT_LUMINANCE_FLOAT32] = BRW_SURFACEFORMAT_L32_FLOAT,
      [MESA_FORMAT_LUMINANCE_FLOAT16] = BRW_SURFACEFORMAT_L16_FLOAT,
      [MESA_FORMAT_LUMINANCE_ALPHA_FLOAT32] = BRW_SURFACEFORMAT_L32A32_FLOAT,
      [MESA_FORMAT_LUMINANCE_ALPHA_FLOAT16] = BRW_SURFACEFORMAT_L16A16_FLOAT,
      [MESA_FORMAT_INTENSITY_FLOAT32] = BRW_SURFACEFORMAT_I32_FLOAT,
      [MESA_FORMAT_INTENSITY_FLOAT16] = BRW_SURFACEFORMAT_I16_FLOAT,
      [MESA_FORMAT_R_FLOAT32] = BRW_SURFACEFORMAT_R32_FLOAT,
      [MESA_FORMAT_R_FLOAT16] = BRW_SURFACEFORMAT_R16_FLOAT,
      [MESA_FORMAT_RG_FLOAT32] = BRW_SURFACEFORMAT_R32G32_FLOAT,
      [MESA_FORMAT_RG_FLOAT16] = BRW_SURFACEFORMAT_R16G16_FLOAT,

      [MESA_FORMAT_ALPHA_UINT8] = 0,
      [MESA_FORMAT_ALPHA_UINT16] = 0,
      [MESA_FORMAT_ALPHA_UINT32] = 0,
      [MESA_FORMAT_ALPHA_INT8] = 0,
      [MESA_FORMAT_ALPHA_INT16] = 0,
      [MESA_FORMAT_ALPHA_INT32] = 0,

      [MESA_FORMAT_INTENSITY_UINT8] = 0,
      [MESA_FORMAT_INTENSITY_UINT16] = 0,
      [MESA_FORMAT_INTENSITY_UINT32] = 0,
      [MESA_FORMAT_INTENSITY_INT8] = 0,
      [MESA_FORMAT_INTENSITY_INT16] = 0,
      [MESA_FORMAT_INTENSITY_INT32] = 0,

      [MESA_FORMAT_LUMINANCE_UINT8] = 0,
      [MESA_FORMAT_LUMINANCE_UINT16] = 0,
      [MESA_FORMAT_LUMINANCE_UINT32] = 0,
      [MESA_FORMAT_LUMINANCE_INT8] = 0,
      [MESA_FORMAT_LUMINANCE_INT16] = 0,
      [MESA_FORMAT_LUMINANCE_INT32] = 0,

      [MESA_FORMAT_LUMINANCE_ALPHA_UINT8] = 0,
      [MESA_FORMAT_LUMINANCE_ALPHA_UINT16] = 0,
      [MESA_FORMAT_LUMINANCE_ALPHA_UINT32] = 0,
      [MESA_FORMAT_LUMINANCE_ALPHA_INT8] = 0,
      [MESA_FORMAT_LUMINANCE_ALPHA_INT16] = 0,
      [MESA_FORMAT_LUMINANCE_ALPHA_INT32] = 0,

      [MESA_FORMAT_R_INT8] = BRW_SURFACEFORMAT_R8_SINT,
      [MESA_FORMAT_RG_INT8] = BRW_SURFACEFORMAT_R8G8_SINT,
      [MESA_FORMAT_RGB_INT8] = BRW_SURFACEFORMAT_R8G8B8_SINT,
      [MESA_FORMAT_RGBA_INT8] = BRW_SURFACEFORMAT_R8G8B8A8_SINT,
      [MESA_FORMAT_R_INT16] = BRW_SURFACEFORMAT_R16_SINT,
      [MESA_FORMAT_RG_INT16] = BRW_SURFACEFORMAT_R16G16_SINT,
      [MESA_FORMAT_RGB_INT16] = BRW_SURFACEFORMAT_R16G16B16_SINT,
      [MESA_FORMAT_RGBA_INT16] = BRW_SURFACEFORMAT_R16G16B16A16_SINT,
      [MESA_FORMAT_R_INT32] = BRW_SURFACEFORMAT_R32_SINT,
      [MESA_FORMAT_RG_INT32] = BRW_SURFACEFORMAT_R32G32_SINT,
      [MESA_FORMAT_RGB_INT32] = BRW_SURFACEFORMAT_R32G32B32_SINT,
      [MESA_FORMAT_RGBA_INT32] = BRW_SURFACEFORMAT_R32G32B32A32_SINT,

      [MESA_FORMAT_R_UINT8] = BRW_SURFACEFORMAT_R8_UINT,
      [MESA_FORMAT_RG_UINT8] = BRW_SURFACEFORMAT_R8G8_UINT,
      [MESA_FORMAT_RGB_UINT8] = BRW_SURFACEFORMAT_R8G8B8_UINT,
      [MESA_FORMAT_RGBA_UINT8] = BRW_SURFACEFORMAT_R8G8B8A8_UINT,
      [MESA_FORMAT_R_UINT16] = BRW_SURFACEFORMAT_R16_UINT,
      [MESA_FORMAT_RG_UINT16] = BRW_SURFACEFORMAT_R16G16_UINT,
      [MESA_FORMAT_RGB_UINT16] = BRW_SURFACEFORMAT_R16G16B16_UINT,
      [MESA_FORMAT_RGBA_UINT16] = BRW_SURFACEFORMAT_R16G16B16A16_UINT,
      [MESA_FORMAT_R_UINT32] = BRW_SURFACEFORMAT_R32_UINT,
      [MESA_FORMAT_RG_UINT32] = BRW_SURFACEFORMAT_R32G32_UINT,
      [MESA_FORMAT_RGB_UINT32] = BRW_SURFACEFORMAT_R32G32B32_UINT,
      [MESA_FORMAT_RGBA_UINT32] = BRW_SURFACEFORMAT_R32G32B32A32_UINT,

      [MESA_FORMAT_DUDV8] = BRW_SURFACEFORMAT_R8G8_SNORM,
      [MESA_FORMAT_SIGNED_R8] = BRW_SURFACEFORMAT_R8_SNORM,
      [MESA_FORMAT_SIGNED_RG88_REV] = BRW_SURFACEFORMAT_R8G8_SNORM,
      [MESA_FORMAT_SIGNED_RGBX8888] = 0,
      [MESA_FORMAT_SIGNED_RGBA8888] = 0,
      [MESA_FORMAT_SIGNED_RGBA8888_REV] = BRW_SURFACEFORMAT_R8G8B8A8_SNORM,
      [MESA_FORMAT_SIGNED_R16] = BRW_SURFACEFORMAT_R16_SNORM,
      [MESA_FORMAT_SIGNED_GR1616] = BRW_SURFACEFORMAT_R16G16_SNORM,
      [MESA_FORMAT_SIGNED_RGB_16] = BRW_SURFACEFORMAT_R16G16B16_SNORM,
      [MESA_FORMAT_SIGNED_RGBA_16] = BRW_SURFACEFORMAT_R16G16B16A16_SNORM,
      [MESA_FORMAT_RGBA_16] = BRW_SURFACEFORMAT_R16G16B16A16_UNORM,

      [MESA_FORMAT_RED_RGTC1] = BRW_SURFACEFORMAT_BC4_UNORM,
      [MESA_FORMAT_SIGNED_RED_RGTC1] = BRW_SURFACEFORMAT_BC4_SNORM,
      [MESA_FORMAT_RG_RGTC2] = BRW_SURFACEFORMAT_BC5_UNORM,
      [MESA_FORMAT_SIGNED_RG_RGTC2] = BRW_SURFACEFORMAT_BC5_SNORM,

      [MESA_FORMAT_L_LATC1] = 0,
      [MESA_FORMAT_SIGNED_L_LATC1] = 0,
      [MESA_FORMAT_LA_LATC2] = 0,
      [MESA_FORMAT_SIGNED_LA_LATC2] = 0,

      [MESA_FORMAT_ETC1_RGB8] = BRW_SURFACEFORMAT_ETC1_RGB8,
      [MESA_FORMAT_ETC2_RGB8] = BRW_SURFACEFORMAT_ETC2_RGB8,
      [MESA_FORMAT_ETC2_SRGB8] = BRW_SURFACEFORMAT_ETC2_SRGB8,
      [MESA_FORMAT_ETC2_RGBA8_EAC] = BRW_SURFACEFORMAT_ETC2_EAC_RGBA8,
      [MESA_FORMAT_ETC2_SRGB8_ALPHA8_EAC] = BRW_SURFACEFORMAT_ETC2_EAC_SRGB8_A8,
      [MESA_FORMAT_ETC2_R11_EAC] = BRW_SURFACEFORMAT_EAC_R11,
      [MESA_FORMAT_ETC2_RG11_EAC] = BRW_SURFACEFORMAT_EAC_RG11,
      [MESA_FORMAT_ETC2_SIGNED_R11_EAC] = BRW_SURFACEFORMAT_EAC_SIGNED_R11,
      [MESA_FORMAT_ETC2_SIGNED_RG11_EAC] = BRW_SURFACEFORMAT_EAC_SIGNED_RG11,
      [MESA_FORMAT_ETC2_RGB8_PUNCHTHROUGH_ALPHA1] = BRW_SURFACEFORMAT_ETC2_RGB8_PTA,
      [MESA_FORMAT_ETC2_SRGB8_PUNCHTHROUGH_ALPHA1] = BRW_SURFACEFORMAT_ETC2_SRGB8_PTA,

      [MESA_FORMAT_SIGNED_A8] = 0,
      [MESA_FORMAT_SIGNED_L8] = 0,
      [MESA_FORMAT_SIGNED_AL88] = 0,
      [MESA_FORMAT_SIGNED_I8] = 0,
      [MESA_FORMAT_SIGNED_A16] = 0,
      [MESA_FORMAT_SIGNED_L16] = 0,
      [MESA_FORMAT_SIGNED_AL1616] = 0,
      [MESA_FORMAT_SIGNED_I16] = 0,

      [MESA_FORMAT_RGB9_E5_FLOAT] = BRW_SURFACEFORMAT_R9G9B9E5_SHAREDEXP,
      [MESA_FORMAT_R11_G11_B10_FLOAT] = BRW_SURFACEFORMAT_R11G11B10_FLOAT,

      [MESA_FORMAT_Z32_FLOAT] = 0,
      [MESA_FORMAT_Z32_FLOAT_X24S8] = 0,

      [MESA_FORMAT_ARGB2101010_UINT] = BRW_SURFACEFORMAT_B10G10R10A2_UINT,
      [MESA_FORMAT_ABGR2101010_UINT] = BRW_SURFACEFORMAT_R10G10B10A2_UINT,

      [MESA_FORMAT_XRGB4444_UNORM] = 0,
      [MESA_FORMAT_XRGB1555_UNORM] = BRW_SURFACEFORMAT_B5G5R5X1_UNORM,
      [MESA_FORMAT_XBGR8888_SNORM] = 0,
      [MESA_FORMAT_XBGR8888_SRGB] = 0,
      [MESA_FORMAT_XBGR8888_UINT] = 0,
      [MESA_FORMAT_XBGR8888_SINT] = 0,
      [MESA_FORMAT_XRGB2101010_UNORM] = BRW_SURFACEFORMAT_B10G10R10X2_UNORM,
      [MESA_FORMAT_XBGR16161616_UNORM] = BRW_SURFACEFORMAT_R16G16B16X16_UNORM,
      [MESA_FORMAT_XBGR16161616_SNORM] = 0,
      [MESA_FORMAT_XBGR16161616_FLOAT] = BRW_SURFACEFORMAT_R16G16B16X16_FLOAT,
      [MESA_FORMAT_XBGR16161616_UINT] = 0,
      [MESA_FORMAT_XBGR16161616_SINT] = 0,
      [MESA_FORMAT_XBGR32323232_FLOAT] = BRW_SURFACEFORMAT_R32G32B32X32_FLOAT,
      [MESA_FORMAT_XBGR32323232_UINT] = 0,
      [MESA_FORMAT_XBGR32323232_SINT] = 0,
   };
   assert(mesa_format < MESA_FORMAT_COUNT);
   return table[mesa_format];
}

void
brw_init_surface_formats(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   int gen;
   gl_format format;

   gen = brw->gen * 10;
   if (brw->is_g4x)
      gen += 5;

   for (format = MESA_FORMAT_NONE + 1; format < MESA_FORMAT_COUNT; format++) {
      uint32_t texture, render;
      const struct surface_format_info *rinfo, *tinfo;
      bool is_integer = _mesa_is_format_integer_color(format);

      render = texture = brw_format_for_mesa_format(format);
      tinfo = &surface_formats[texture];

      /* The value of BRW_SURFACEFORMAT_R32G32B32A32_FLOAT is 0, so don't skip
       * it.
       */
      if (texture == 0 && format != MESA_FORMAT_RGBA_FLOAT32)
	 continue;

      if (gen >= tinfo->sampling && (gen >= tinfo->filtering || is_integer))
	 ctx->TextureFormatSupported[format] = true;

      /* Re-map some render target formats to make them supported when they
       * wouldn't be using their format for texturing.
       */
      switch (render) {
	 /* For these formats, we just need to read/write the first
	  * channel into R, which is to say that we just treat them as
	  * GL_RED.
	  */
      case BRW_SURFACEFORMAT_I32_FLOAT:
      case BRW_SURFACEFORMAT_L32_FLOAT:
	 render = BRW_SURFACEFORMAT_R32_FLOAT;
	 break;
      case BRW_SURFACEFORMAT_I16_FLOAT:
      case BRW_SURFACEFORMAT_L16_FLOAT:
	 render = BRW_SURFACEFORMAT_R16_FLOAT;
	 break;
      case BRW_SURFACEFORMAT_B8G8R8X8_UNORM:
	 /* XRGB is handled as ARGB because the chips in this family
	  * cannot render to XRGB targets.  This means that we have to
	  * mask writes to alpha (ala glColorMask) and reconfigure the
	  * alpha blending hardware to use GL_ONE (or GL_ZERO) for
	  * cases where GL_DST_ALPHA (or GL_ONE_MINUS_DST_ALPHA) is
	  * used.
	  */
	 render = BRW_SURFACEFORMAT_B8G8R8A8_UNORM;
	 break;
      }

      rinfo = &surface_formats[render];

      /* Note that GL_EXT_texture_integer says that blending doesn't occur for
       * integer, so we don't need hardware support for blending on it.  Other
       * than that, GL in general requires alpha blending for render targets,
       * even though we don't support it for some formats.
       */
      if (gen >= rinfo->render_target &&
	  (gen >= rinfo->alpha_blend || is_integer)) {
	 brw->render_target_format[format] = render;
	 brw->format_supported_as_render_target[format] = true;
      }
   }

   /* We will check this table for FBO completeness, but the surface format
    * table above only covered color rendering.
    */
   brw->format_supported_as_render_target[MESA_FORMAT_S8_Z24] = true;
   brw->format_supported_as_render_target[MESA_FORMAT_X8_Z24] = true;
   brw->format_supported_as_render_target[MESA_FORMAT_S8] = true;
   brw->format_supported_as_render_target[MESA_FORMAT_Z16] = true;
   brw->format_supported_as_render_target[MESA_FORMAT_Z32_FLOAT] = true;
   brw->format_supported_as_render_target[MESA_FORMAT_Z32_FLOAT_X24S8] = true;

   /* We remap depth formats to a supported texturing format in
    * translate_tex_format().
    */
   ctx->TextureFormatSupported[MESA_FORMAT_S8_Z24] = true;
   ctx->TextureFormatSupported[MESA_FORMAT_X8_Z24] = true;
   ctx->TextureFormatSupported[MESA_FORMAT_Z32_FLOAT] = true;
   ctx->TextureFormatSupported[MESA_FORMAT_Z32_FLOAT_X24S8] = true;

   /* It appears that Z16 is slower than Z24 (on Intel Ivybridge and newer
    * hardware at least), so there's no real reason to prefer it unless you're
    * under memory (not memory bandwidth) pressure.  Our speculation is that
    * this is due to either increased fragment shader execution from
    * GL_LEQUAL/GL_EQUAL depth tests at the reduced precision, or due to
    * increased depth stalls from a cacheline-based heuristic for detecting
    * depth stalls.
    *
    * However, desktop GL 3.0+ require that you get exactly 16 bits when
    * asking for DEPTH_COMPONENT16, so we have to respect that.
    */
   if (_mesa_is_desktop_gl(ctx))
      ctx->TextureFormatSupported[MESA_FORMAT_Z16] = true;

   /* On hardware that lacks support for ETC1, we map ETC1 to RGBX
    * during glCompressedTexImage2D(). See intel_mipmap_tree::wraps_etc1.
    */
   ctx->TextureFormatSupported[MESA_FORMAT_ETC1_RGB8] = true;

   /* On hardware that lacks support for ETC2, we map ETC2 to a suitable
    * MESA_FORMAT during glCompressedTexImage2D().
    * See intel_mipmap_tree::wraps_etc2.
    */
   ctx->TextureFormatSupported[MESA_FORMAT_ETC2_RGB8] = true;
   ctx->TextureFormatSupported[MESA_FORMAT_ETC2_SRGB8] = true;
   ctx->TextureFormatSupported[MESA_FORMAT_ETC2_RGBA8_EAC] = true;
   ctx->TextureFormatSupported[MESA_FORMAT_ETC2_SRGB8_ALPHA8_EAC] = true;
   ctx->TextureFormatSupported[MESA_FORMAT_ETC2_R11_EAC] = true;
   ctx->TextureFormatSupported[MESA_FORMAT_ETC2_RG11_EAC] = true;
   ctx->TextureFormatSupported[MESA_FORMAT_ETC2_SIGNED_R11_EAC] = true;
   ctx->TextureFormatSupported[MESA_FORMAT_ETC2_SIGNED_RG11_EAC] = true;
   ctx->TextureFormatSupported[MESA_FORMAT_ETC2_RGB8_PUNCHTHROUGH_ALPHA1] = true;
   ctx->TextureFormatSupported[MESA_FORMAT_ETC2_SRGB8_PUNCHTHROUGH_ALPHA1] = true;
}

bool
brw_render_target_supported(struct brw_context *brw,
			    struct gl_renderbuffer *rb)
{
   gl_format format = rb->Format;

   /* Many integer formats are promoted to RGBA (like XRGB8888 is), which means
    * we would consider them renderable even though we don't have surface
    * support for their alpha behavior and don't have the blending unit
    * available to fake it like we do for XRGB8888.  Force them to being
    * unsupported.
    */
   if ((rb->_BaseFormat != GL_RGBA &&
	rb->_BaseFormat != GL_RG &&
	rb->_BaseFormat != GL_RED) && _mesa_is_format_integer_color(format))
      return false;

   /* Under some conditions, MSAA is not supported for formats whose width is
    * more than 64 bits.
    */
   if (rb->NumSamples > 0 && _mesa_get_format_bytes(format) > 8) {
      /* Gen6: MSAA on >64 bit formats is unsupported. */
      if (brw->gen <= 6)
         return false;

      /* Gen7: 8x MSAA on >64 bit formats is unsupported. */
      if (rb->NumSamples >= 8)
         return false;
   }

   return brw->format_supported_as_render_target[format];
}

GLuint
translate_tex_format(struct brw_context *brw,
                     gl_format mesa_format,
		     GLenum depth_mode,
		     GLenum srgb_decode)
{
   struct gl_context *ctx = &brw->ctx;
   if (srgb_decode == GL_SKIP_DECODE_EXT)
      mesa_format = _mesa_get_srgb_format_linear(mesa_format);

   switch( mesa_format ) {

   case MESA_FORMAT_Z16:
      return BRW_SURFACEFORMAT_I16_UNORM;

   case MESA_FORMAT_S8_Z24:
   case MESA_FORMAT_X8_Z24:
      return BRW_SURFACEFORMAT_I24X8_UNORM;

   case MESA_FORMAT_Z32_FLOAT:
      return BRW_SURFACEFORMAT_I32_FLOAT;

   case MESA_FORMAT_Z32_FLOAT_X24S8:
      return BRW_SURFACEFORMAT_R32G32_FLOAT;

   case MESA_FORMAT_RGBA_FLOAT32:
      /* The value of this BRW_SURFACEFORMAT is 0, which tricks the
       * assertion below.
       */
      return BRW_SURFACEFORMAT_R32G32B32A32_FLOAT;

   case MESA_FORMAT_SRGB_DXT1:
      if (brw->gen == 4 && !brw->is_g4x) {
         /* Work around missing SRGB DXT1 support on original gen4 by just
          * skipping SRGB decode.  It's not worth not supporting sRGB in
          * general to prevent this.
          */
         WARN_ONCE(true, "Demoting sRGB DXT1 texture to non-sRGB\n");
         mesa_format = MESA_FORMAT_RGB_DXT1;
      }
      return brw_format_for_mesa_format(mesa_format);

   default:
      assert(brw_format_for_mesa_format(mesa_format) != 0);
      return brw_format_for_mesa_format(mesa_format);
   }
}

/** Can HiZ be enabled on a depthbuffer of the given format? */
bool
brw_is_hiz_depth_format(struct brw_context *brw, gl_format format)
{
   if (!brw->has_hiz)
      return false;

   switch (format) {
   case MESA_FORMAT_Z32_FLOAT:
   case MESA_FORMAT_Z32_FLOAT_X24S8:
   case MESA_FORMAT_X8_Z24:
   case MESA_FORMAT_S8_Z24:
   case MESA_FORMAT_Z16:
      return true;
   default:
      return false;
   }
}

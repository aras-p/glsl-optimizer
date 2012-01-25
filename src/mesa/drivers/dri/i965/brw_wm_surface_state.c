/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.
 
 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */
                   

#include "main/mtypes.h"
#include "main/samplerobj.h"
#include "program/prog_parameter.h"

#include "intel_mipmap_tree.h"
#include "intel_batchbuffer.h"
#include "intel_tex.h"
#include "intel_fbo.h"
#include "intel_buffer_objects.h"

#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"
#include "brw_wm.h"

GLuint
translate_tex_target(GLenum target)
{
   switch (target) {
   case GL_TEXTURE_1D: 
   case GL_TEXTURE_1D_ARRAY_EXT:
      return BRW_SURFACE_1D;

   case GL_TEXTURE_RECTANGLE_NV: 
      return BRW_SURFACE_2D;

   case GL_TEXTURE_2D: 
   case GL_TEXTURE_2D_ARRAY_EXT:
   case GL_TEXTURE_EXTERNAL_OES:
      return BRW_SURFACE_2D;

   case GL_TEXTURE_3D: 
      return BRW_SURFACE_3D;

   case GL_TEXTURE_CUBE_MAP: 
      return BRW_SURFACE_CUBE;

   default: 
      assert(0); 
      return 0;
   }
}

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
 * See page 88 of the Sandybridge PRM VOL4_Part1 PDF.
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
   SF( Y, 50,  x,  x,  x,  x,  Y,  Y,  x, BRW_SURFACEFORMAT_R32G32B32_FLOAT)
   SF( Y,  x,  x,  x,  x,  x,  Y,  Y,  x, BRW_SURFACEFORMAT_R32G32B32_SINT)
   SF( Y,  x,  x,  x,  x,  x,  Y,  Y,  x, BRW_SURFACEFORMAT_R32G32B32_UINT)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R32G32B32_UNORM)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R32G32B32_SNORM)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R32G32B32_SSCALED)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R32G32B32_USCALED)
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
   SF(45, 45,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_L8_UNORM_SRGB)
   SF(45, 45,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_DXT1_RGB_SRGB)
   SF( Y,  Y,  x,  x,  x,  x,  x,  x,  x, BRW_SURFACEFORMAT_R1_UINT)
   SF( Y,  Y,  x,  Y,  Y,  x,  x,  x, 60, BRW_SURFACEFORMAT_YCRCB_NORMAL)
   SF( Y,  Y,  x,  Y,  Y,  x,  x,  x, 60, BRW_SURFACEFORMAT_YCRCB_SWAPUVY)
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
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R16G16B16_UNORM)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R16G16B16_SNORM)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R16G16B16_SSCALED)
   SF( x,  x,  x,  x,  x,  x,  Y,  x,  x, BRW_SURFACEFORMAT_R16G16B16_USCALED)
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
      [MESA_FORMAT_RGBA8888_REV] = 0,
      [MESA_FORMAT_ARGB8888] = BRW_SURFACEFORMAT_B8G8R8A8_UNORM,
      [MESA_FORMAT_ARGB8888_REV] = 0,
      [MESA_FORMAT_XRGB8888] = BRW_SURFACEFORMAT_B8G8R8X8_UNORM,
      [MESA_FORMAT_XRGB8888_REV] = 0,
      [MESA_FORMAT_RGB888] = 0,
      [MESA_FORMAT_BGR888] = 0,
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
      [MESA_FORMAT_RG1616] = BRW_SURFACEFORMAT_R16G16_UNORM,
      [MESA_FORMAT_RG1616_REV] = 0,
      [MESA_FORMAT_ARGB2101010] = BRW_SURFACEFORMAT_B10G10R10A2_UNORM,
      [MESA_FORMAT_Z24_S8] = 0,
      [MESA_FORMAT_S8_Z24] = 0,
      [MESA_FORMAT_Z16] = 0,
      [MESA_FORMAT_X8_Z24] = 0,
      [MESA_FORMAT_Z24_S8] = 0,
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
      [MESA_FORMAT_RGB_FLOAT32] = 0,
      [MESA_FORMAT_RGB_FLOAT16] = 0,
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
      [MESA_FORMAT_RGB_INT8] = 0,
      [MESA_FORMAT_RGBA_INT8] = BRW_SURFACEFORMAT_R8G8B8A8_SINT,
      [MESA_FORMAT_R_INT16] = BRW_SURFACEFORMAT_R16_SINT,
      [MESA_FORMAT_RG_INT16] = BRW_SURFACEFORMAT_R16G16_SINT,
      [MESA_FORMAT_RGB_INT16] = 0,
      [MESA_FORMAT_RGBA_INT16] = BRW_SURFACEFORMAT_R16G16B16A16_SINT,
      [MESA_FORMAT_R_INT32] = BRW_SURFACEFORMAT_R32_SINT,
      [MESA_FORMAT_RG_INT32] = BRW_SURFACEFORMAT_R32G32_SINT,
      [MESA_FORMAT_RGB_INT32] = BRW_SURFACEFORMAT_R32G32B32_SINT,
      [MESA_FORMAT_RGBA_INT32] = BRW_SURFACEFORMAT_R32G32B32A32_SINT,

      [MESA_FORMAT_R_UINT8] = BRW_SURFACEFORMAT_R8_UINT,
      [MESA_FORMAT_RG_UINT8] = BRW_SURFACEFORMAT_R8G8_UINT,
      [MESA_FORMAT_RGB_UINT8] = 0,
      [MESA_FORMAT_RGBA_UINT8] = BRW_SURFACEFORMAT_R8G8B8A8_UINT,
      [MESA_FORMAT_R_UINT16] = BRW_SURFACEFORMAT_R16_UINT,
      [MESA_FORMAT_RG_UINT16] = BRW_SURFACEFORMAT_R16G16_UINT,
      [MESA_FORMAT_RGB_UINT16] = 0,
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
      [MESA_FORMAT_SIGNED_RGB_16] = 0,
      [MESA_FORMAT_SIGNED_RGBA_16] = 0,
      [MESA_FORMAT_RGBA_16] = BRW_SURFACEFORMAT_R16G16B16A16_UNORM,

      [MESA_FORMAT_RED_RGTC1] = BRW_SURFACEFORMAT_BC4_UNORM,
      [MESA_FORMAT_SIGNED_RED_RGTC1] = BRW_SURFACEFORMAT_BC4_SNORM,
      [MESA_FORMAT_RG_RGTC2] = BRW_SURFACEFORMAT_BC5_UNORM,
      [MESA_FORMAT_SIGNED_RG_RGTC2] = BRW_SURFACEFORMAT_BC5_SNORM,

      [MESA_FORMAT_L_LATC1] = 0,
      [MESA_FORMAT_SIGNED_L_LATC1] = 0,
      [MESA_FORMAT_LA_LATC2] = 0,
      [MESA_FORMAT_SIGNED_LA_LATC2] = 0,

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
   };
   assert(mesa_format < MESA_FORMAT_COUNT);
   return table[mesa_format];
}

void
brw_init_surface_formats(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;
   int gen;
   gl_format format;

   gen = intel->gen * 10;
   if (intel->is_g4x)
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
       *
       * We don't currently support rendering to SNORM textures because some of
       * the ARB_color_buffer_float clamping is broken for it
       * (piglit arb_color_buffer_float-drawpixels GL_RGBA8_SNORM).
       */
      if (gen >= rinfo->render_target &&
	  (gen >= rinfo->alpha_blend || is_integer) &&
	  _mesa_get_format_datatype(format) != GL_SIGNED_NORMALIZED) {
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
   ctx->TextureFormatSupported[MESA_FORMAT_Z16] = true;
}

bool
brw_render_target_supported(struct intel_context *intel,
			    struct gl_renderbuffer *rb)
{
   struct brw_context *brw = brw_context(&intel->ctx);
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

   return brw->format_supported_as_render_target[format];
}

GLuint
translate_tex_format(gl_format mesa_format,
		     GLenum internal_format,
		     GLenum depth_mode,
		     GLenum srgb_decode)
{
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

   case MESA_FORMAT_SARGB8:
   case MESA_FORMAT_SLA8:
   case MESA_FORMAT_SL8:
      if (srgb_decode == GL_DECODE_EXT)
	 return brw_format_for_mesa_format(mesa_format);
      else if (srgb_decode == GL_SKIP_DECODE_EXT)
	 return brw_format_for_mesa_format(_mesa_get_srgb_format_linear(mesa_format));

   case MESA_FORMAT_RGBA8888_REV:
      /* This format is not renderable? */
      return BRW_SURFACEFORMAT_R8G8B8A8_UNORM;

   case MESA_FORMAT_RGBA_FLOAT32:
      /* The value of this BRW_SURFACEFORMAT is 0, which tricks the
       * assertion below.
       */
      return BRW_SURFACEFORMAT_R32G32B32A32_FLOAT;

   default:
      assert(brw_format_for_mesa_format(mesa_format) != 0);
      return brw_format_for_mesa_format(mesa_format);
   }
}

static uint32_t
brw_get_surface_tiling_bits(uint32_t tiling)
{
   switch (tiling) {
   case I915_TILING_X:
      return BRW_SURFACE_TILED;
   case I915_TILING_Y:
      return BRW_SURFACE_TILED | BRW_SURFACE_TILED_Y;
   default:
      return 0;
   }
}

static void
brw_update_texture_surface( struct gl_context *ctx, GLuint unit )
{
   struct brw_context *brw = brw_context(ctx);
   struct gl_texture_object *tObj = ctx->Texture.Unit[unit]._Current;
   struct intel_texture_object *intelObj = intel_texture_object(tObj);
   struct intel_mipmap_tree *mt = intelObj->mt;
   struct gl_texture_image *firstImage = tObj->Image[0][tObj->BaseLevel];
   struct gl_sampler_object *sampler = _mesa_get_samplerobj(ctx, unit);
   const GLuint surf_index = SURF_INDEX_TEXTURE(unit);
   uint32_t *surf;
   int width, height, depth;

   intel_miptree_get_dimensions_for_image(firstImage, &width, &height, &depth);

   surf = brw_state_batch(brw, AUB_TRACE_SURFACE_STATE,
			  6 * 4, 32, &brw->bind.surf_offset[surf_index]);

   surf[0] = (translate_tex_target(tObj->Target) << BRW_SURFACE_TYPE_SHIFT |
	      BRW_SURFACE_MIPMAPLAYOUT_BELOW << BRW_SURFACE_MIPLAYOUT_SHIFT |
	      BRW_SURFACE_CUBEFACE_ENABLES |
	      (translate_tex_format(mt->format,
				    firstImage->InternalFormat,
				    sampler->DepthMode,
				    sampler->sRGBDecode) <<
	       BRW_SURFACE_FORMAT_SHIFT));

   surf[1] = intelObj->mt->region->bo->offset; /* reloc */

   surf[2] = ((intelObj->_MaxLevel - tObj->BaseLevel) << BRW_SURFACE_LOD_SHIFT |
	      (width - 1) << BRW_SURFACE_WIDTH_SHIFT |
	      (height - 1) << BRW_SURFACE_HEIGHT_SHIFT);

   surf[3] = (brw_get_surface_tiling_bits(intelObj->mt->region->tiling) |
	      (depth - 1) << BRW_SURFACE_DEPTH_SHIFT |
	      ((intelObj->mt->region->pitch * intelObj->mt->cpp) - 1) <<
	      BRW_SURFACE_PITCH_SHIFT);

   surf[4] = 0;

   surf[5] = (mt->align_h == 4) ? BRW_SURFACE_VERTICAL_ALIGN_ENABLE : 0;

   /* Emit relocation to surface contents */
   drm_intel_bo_emit_reloc(brw->intel.batch.bo,
			   brw->bind.surf_offset[surf_index] + 4,
			   intelObj->mt->region->bo, 0,
			   I915_GEM_DOMAIN_SAMPLER, 0);
}

/**
 * Create the constant buffer surface.  Vertex/fragment shader constants will be
 * read from this buffer with Data Port Read instructions/messages.
 */
void
brw_create_constant_surface(struct brw_context *brw,
			    drm_intel_bo *bo,
			    int width,
			    uint32_t *out_offset)
{
   struct intel_context *intel = &brw->intel;
   const GLint w = width - 1;
   uint32_t *surf;

   surf = brw_state_batch(brw, AUB_TRACE_SURFACE_STATE,
			  6 * 4, 32, out_offset);

   surf[0] = (BRW_SURFACE_BUFFER << BRW_SURFACE_TYPE_SHIFT |
	      BRW_SURFACE_MIPMAPLAYOUT_BELOW << BRW_SURFACE_MIPLAYOUT_SHIFT |
	      BRW_SURFACEFORMAT_R32G32B32A32_FLOAT << BRW_SURFACE_FORMAT_SHIFT);

   if (intel->gen >= 6)
      surf[0] |= BRW_SURFACE_RC_READ_WRITE;

   surf[1] = bo->offset; /* reloc */

   surf[2] = ((w & 0x7f) << BRW_SURFACE_WIDTH_SHIFT |
	      ((w >> 7) & 0x1fff) << BRW_SURFACE_HEIGHT_SHIFT);

   surf[3] = (((w >> 20) & 0x7f) << BRW_SURFACE_DEPTH_SHIFT |
	      (16 - 1) << BRW_SURFACE_PITCH_SHIFT); /* ignored */

   surf[4] = 0;
   surf[5] = 0;

   /* Emit relocation to surface contents.  Section 5.1.1 of the gen4
    * bspec ("Data Cache") says that the data cache does not exist as
    * a separate cache and is just the sampler cache.
    */
   drm_intel_bo_emit_reloc(brw->intel.batch.bo,
			   *out_offset + 4,
			   bo, 0,
			   I915_GEM_DOMAIN_SAMPLER, 0);
}

/**
 * Set up a binding table entry for use by stream output logic (transform
 * feedback).
 *
 * buffer_size_minus_1 must me less than BRW_MAX_NUM_BUFFER_ENTRIES.
 */
void
brw_update_sol_surface(struct brw_context *brw,
                       struct gl_buffer_object *buffer_obj,
                       uint32_t *out_offset, unsigned num_vector_components,
                       unsigned stride_dwords, unsigned offset_dwords)
{
   struct intel_context *intel = &brw->intel;
   struct intel_buffer_object *intel_bo = intel_buffer_object(buffer_obj);
   drm_intel_bo *bo =
      intel_bufferobj_buffer(intel, intel_bo, INTEL_WRITE_PART);
   uint32_t *surf = brw_state_batch(brw, AUB_TRACE_SURFACE_STATE, 6 * 4, 32,
                                    out_offset);
   uint32_t pitch_minus_1 = 4*stride_dwords - 1;
   uint32_t offset_bytes = 4 * offset_dwords;
   size_t size_dwords = buffer_obj->Size / 4;
   uint32_t buffer_size_minus_1, width, height, depth, surface_format;

   /* FIXME: can we rely on core Mesa to ensure that the buffer isn't
    * too big to map using a single binding table entry?
    */
   assert((size_dwords - offset_dwords) / stride_dwords
          <= BRW_MAX_NUM_BUFFER_ENTRIES);

   if (size_dwords > offset_dwords + num_vector_components) {
      /* There is room for at least 1 transform feedback output in the buffer.
       * Compute the number of additional transform feedback outputs the
       * buffer has room for.
       */
      buffer_size_minus_1 =
         (size_dwords - offset_dwords - num_vector_components) / stride_dwords;
   } else {
      /* There isn't even room for a single transform feedback output in the
       * buffer.  We can't configure the binding table entry to prevent output
       * entirely; we'll have to rely on the geometry shader to detect
       * overflow.  But to minimize the damage in case of a bug, set up the
       * binding table entry to just allow a single output.
       */
      buffer_size_minus_1 = 0;
   }
   width = buffer_size_minus_1 & 0x7f;
   height = (buffer_size_minus_1 & 0xfff80) >> 7;
   depth = (buffer_size_minus_1 & 0x7f00000) >> 20;

   switch (num_vector_components) {
   case 1:
      surface_format = BRW_SURFACEFORMAT_R32_FLOAT;
      break;
   case 2:
      surface_format = BRW_SURFACEFORMAT_R32G32_FLOAT;
      break;
   case 3:
      surface_format = BRW_SURFACEFORMAT_R32G32B32_FLOAT;
      break;
   case 4:
      surface_format = BRW_SURFACEFORMAT_R32G32B32A32_FLOAT;
      break;
   default:
      assert(!"Invalid vector size for transform feedback output");
      surface_format = BRW_SURFACEFORMAT_R32_FLOAT;
      break;
   }

   surf[0] = BRW_SURFACE_BUFFER << BRW_SURFACE_TYPE_SHIFT |
      BRW_SURFACE_MIPMAPLAYOUT_BELOW << BRW_SURFACE_MIPLAYOUT_SHIFT |
      surface_format << BRW_SURFACE_FORMAT_SHIFT |
      BRW_SURFACE_RC_READ_WRITE;
   surf[1] = bo->offset + offset_bytes; /* reloc */
   surf[2] = (width << BRW_SURFACE_WIDTH_SHIFT |
	      height << BRW_SURFACE_HEIGHT_SHIFT);
   surf[3] = (depth << BRW_SURFACE_DEPTH_SHIFT |
              pitch_minus_1 << BRW_SURFACE_PITCH_SHIFT);
   surf[4] = 0;
   surf[5] = 0;

   /* Emit relocation to surface contents. */
   drm_intel_bo_emit_reloc(brw->intel.batch.bo,
			   *out_offset + 4,
			   bo, offset_bytes,
			   I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER);
}

/* Creates a new WM constant buffer reflecting the current fragment program's
 * constants, if needed by the fragment program.
 *
 * Otherwise, constants go through the CURBEs using the brw_constant_buffer
 * state atom.
 */
static void
brw_upload_wm_pull_constants(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->intel.ctx;
   struct intel_context *intel = &brw->intel;
   /* BRW_NEW_FRAGMENT_PROGRAM */
   struct brw_fragment_program *fp =
      (struct brw_fragment_program *) brw->fragment_program;
   struct gl_program_parameter_list *params = fp->program.Base.Parameters;
   const int size = brw->wm.prog_data->nr_pull_params * sizeof(float);
   const int surf_index = SURF_INDEX_FRAG_CONST_BUFFER;
   float *constants;
   unsigned int i;

   _mesa_load_state_parameters(ctx, params);

   /* CACHE_NEW_WM_PROG */
   if (brw->wm.prog_data->nr_pull_params == 0) {
      if (brw->wm.const_bo) {
	 drm_intel_bo_unreference(brw->wm.const_bo);
	 brw->wm.const_bo = NULL;
	 brw->bind.surf_offset[surf_index] = 0;
	 brw->state.dirty.brw |= BRW_NEW_SURFACES;
      }
      return;
   }

   drm_intel_bo_unreference(brw->wm.const_bo);
   brw->wm.const_bo = drm_intel_bo_alloc(intel->bufmgr, "WM const bo",
					 size, 64);

   /* _NEW_PROGRAM_CONSTANTS */
   drm_intel_gem_bo_map_gtt(brw->wm.const_bo);
   constants = brw->wm.const_bo->virtual;
   for (i = 0; i < brw->wm.prog_data->nr_pull_params; i++) {
      constants[i] = convert_param(brw->wm.prog_data->pull_param_convert[i],
				   brw->wm.prog_data->pull_param[i]);
   }
   drm_intel_gem_bo_unmap_gtt(brw->wm.const_bo);

   intel->vtbl.create_constant_surface(brw, brw->wm.const_bo,
				       params->NumParameters,
				       &brw->bind.surf_offset[surf_index]);

   brw->state.dirty.brw |= BRW_NEW_SURFACES;
}

const struct brw_tracked_state brw_wm_pull_constants = {
   .dirty = {
      .mesa = (_NEW_PROGRAM_CONSTANTS),
      .brw = (BRW_NEW_BATCH | BRW_NEW_FRAGMENT_PROGRAM),
      .cache = CACHE_NEW_WM_PROG,
   },
   .emit = brw_upload_wm_pull_constants,
};

static void
brw_update_null_renderbuffer_surface(struct brw_context *brw, unsigned int unit)
{
   struct intel_context *intel = &brw->intel;
   uint32_t *surf;

   surf = brw_state_batch(brw, AUB_TRACE_SURFACE_STATE,
			  6 * 4, 32, &brw->bind.surf_offset[unit]);

   surf[0] = (BRW_SURFACE_NULL << BRW_SURFACE_TYPE_SHIFT |
	      BRW_SURFACEFORMAT_B8G8R8A8_UNORM << BRW_SURFACE_FORMAT_SHIFT);
   if (intel->gen < 6) {
      surf[0] |= (1 << BRW_SURFACE_WRITEDISABLE_R_SHIFT |
		  1 << BRW_SURFACE_WRITEDISABLE_G_SHIFT |
		  1 << BRW_SURFACE_WRITEDISABLE_B_SHIFT |
		  1 << BRW_SURFACE_WRITEDISABLE_A_SHIFT);
   }
   surf[1] = 0;
   surf[2] = 0;
   surf[3] = 0;
   surf[4] = 0;
   surf[5] = 0;
}

/**
 * Sets up a surface state structure to point at the given region.
 * While it is only used for the front/back buffer currently, it should be
 * usable for further buffers when doing ARB_draw_buffer support.
 */
static void
brw_update_renderbuffer_surface(struct brw_context *brw,
				struct gl_renderbuffer *rb,
				unsigned int unit)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);
   struct intel_mipmap_tree *mt = irb->mt;
   struct intel_region *region = irb->mt->region;
   uint32_t *surf;
   uint32_t tile_x, tile_y;
   uint32_t format = 0;
   gl_format rb_format = intel_rb_format(irb);

   surf = brw_state_batch(brw, AUB_TRACE_SURFACE_STATE,
			  6 * 4, 32, &brw->bind.surf_offset[unit]);

   switch (rb_format) {
   case MESA_FORMAT_SARGB8:
      /* without GL_EXT_framebuffer_sRGB we shouldn't bind sRGB
	 surfaces to the blend/update as sRGB */
      if (ctx->Color.sRGBEnabled)
	 format = brw_format_for_mesa_format(rb_format);
      else
	 format = BRW_SURFACEFORMAT_B8G8R8A8_UNORM;
      break;
   default:
      format = brw->render_target_format[rb_format];
      if (unlikely(!brw->format_supported_as_render_target[rb_format])) {
	 _mesa_problem(ctx, "%s: renderbuffer format %s unsupported\n",
		       __FUNCTION__, _mesa_get_format_name(rb_format));
      }
      break;
   }

   surf[0] = (BRW_SURFACE_2D << BRW_SURFACE_TYPE_SHIFT |
	      format << BRW_SURFACE_FORMAT_SHIFT);

   /* reloc */
   surf[1] = (intel_renderbuffer_tile_offsets(irb, &tile_x, &tile_y) +
	      region->bo->offset);

   surf[2] = ((rb->Width - 1) << BRW_SURFACE_WIDTH_SHIFT |
	      (rb->Height - 1) << BRW_SURFACE_HEIGHT_SHIFT);

   surf[3] = (brw_get_surface_tiling_bits(region->tiling) |
	      ((region->pitch * region->cpp) - 1) << BRW_SURFACE_PITCH_SHIFT);

   surf[4] = 0;

   assert(brw->has_surface_tile_offset || (tile_x == 0 && tile_y == 0));
   /* Note that the low bits of these fields are missing, so
    * there's the possibility of getting in trouble.
    */
   assert(tile_x % 4 == 0);
   assert(tile_y % 2 == 0);
   surf[5] = ((tile_x / 4) << BRW_SURFACE_X_OFFSET_SHIFT |
	      (tile_y / 2) << BRW_SURFACE_Y_OFFSET_SHIFT |
	      (mt->align_h == 4 ? BRW_SURFACE_VERTICAL_ALIGN_ENABLE : 0));

   if (intel->gen < 6) {
      /* _NEW_COLOR */
      if (!ctx->Color.ColorLogicOpEnabled &&
	  (ctx->Color.BlendEnabled & (1 << unit)))
	 surf[0] |= BRW_SURFACE_BLEND_ENABLED;

      if (!ctx->Color.ColorMask[unit][0])
	 surf[0] |= 1 << BRW_SURFACE_WRITEDISABLE_R_SHIFT;
      if (!ctx->Color.ColorMask[unit][1])
	 surf[0] |= 1 << BRW_SURFACE_WRITEDISABLE_G_SHIFT;
      if (!ctx->Color.ColorMask[unit][2])
	 surf[0] |= 1 << BRW_SURFACE_WRITEDISABLE_B_SHIFT;

      /* As mentioned above, disable writes to the alpha component when the
       * renderbuffer is XRGB.
       */
      if (ctx->DrawBuffer->Visual.alphaBits == 0 ||
	  !ctx->Color.ColorMask[unit][3]) {
	 surf[0] |= 1 << BRW_SURFACE_WRITEDISABLE_A_SHIFT;
      }
   }

   drm_intel_bo_emit_reloc(brw->intel.batch.bo,
			   brw->bind.surf_offset[unit] + 4,
			   region->bo,
			   surf[1] - region->bo->offset,
			   I915_GEM_DOMAIN_RENDER,
			   I915_GEM_DOMAIN_RENDER);
}

/**
 * Construct SURFACE_STATE objects for renderbuffers/draw buffers.
 */
static void
brw_update_renderbuffer_surfaces(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &brw->intel.ctx;
   GLuint i;

   /* _NEW_BUFFERS | _NEW_COLOR */
   /* Update surfaces for drawing buffers */
   if (ctx->DrawBuffer->_NumColorDrawBuffers >= 1) {
      for (i = 0; i < ctx->DrawBuffer->_NumColorDrawBuffers; i++) {
	 if (intel_renderbuffer(ctx->DrawBuffer->_ColorDrawBuffers[i])) {
	    intel->vtbl.update_renderbuffer_surface(brw, ctx->DrawBuffer->_ColorDrawBuffers[i], i);
	 } else {
	    intel->vtbl.update_null_renderbuffer_surface(brw, i);
	 }
      }
   } else {
      intel->vtbl.update_null_renderbuffer_surface(brw, 0);
   }
   brw->state.dirty.brw |= BRW_NEW_SURFACES;
}

const struct brw_tracked_state brw_renderbuffer_surfaces = {
   .dirty = {
      .mesa = (_NEW_COLOR |
               _NEW_BUFFERS),
      .brw = BRW_NEW_BATCH,
      .cache = 0
   },
   .emit = brw_update_renderbuffer_surfaces,
};

const struct brw_tracked_state gen6_renderbuffer_surfaces = {
   .dirty = {
      .mesa = _NEW_BUFFERS,
      .brw = BRW_NEW_BATCH,
      .cache = 0
   },
   .emit = brw_update_renderbuffer_surfaces,
};

/**
 * Construct SURFACE_STATE objects for enabled textures.
 */
static void
brw_update_texture_surfaces(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->intel.ctx;

   for (unsigned i = 0; i < BRW_MAX_TEX_UNIT; i++) {
      const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[i];
      const GLuint surf = SURF_INDEX_TEXTURE(i);

      /* _NEW_TEXTURE */
      if (texUnit->_ReallyEnabled) {
	 brw->intel.vtbl.update_texture_surface(ctx, i);
      } else {
         brw->bind.surf_offset[surf] = 0;
      }
   }

   brw->state.dirty.brw |= BRW_NEW_SURFACES;
}

const struct brw_tracked_state brw_texture_surfaces = {
   .dirty = {
      .mesa = _NEW_TEXTURE,
      .brw = BRW_NEW_BATCH,
      .cache = 0
   },
   .emit = brw_update_texture_surfaces,
};

/**
 * Constructs the binding table for the WM surface state, which maps unit
 * numbers to surface state objects.
 */
static void
brw_upload_binding_table(struct brw_context *brw)
{
   uint32_t *bind;
   int i;

   /* Might want to calculate nr_surfaces first, to avoid taking up so much
    * space for the binding table.
    */
   bind = brw_state_batch(brw, AUB_TRACE_BINDING_TABLE,
			  sizeof(uint32_t) * BRW_MAX_SURFACES,
			  32, &brw->bind.bo_offset);

   /* BRW_NEW_SURFACES and BRW_NEW_VS_CONSTBUF */
   for (i = 0; i < BRW_MAX_SURFACES; i++) {
      bind[i] = brw->bind.surf_offset[i];
   }

   brw->state.dirty.brw |= BRW_NEW_VS_BINDING_TABLE;
   brw->state.dirty.brw |= BRW_NEW_PS_BINDING_TABLE;
}

const struct brw_tracked_state brw_binding_table = {
   .dirty = {
      .mesa = 0,
      .brw = (BRW_NEW_BATCH |
	      BRW_NEW_VS_CONSTBUF |
	      BRW_NEW_SURFACES),
      .cache = 0
   },
   .emit = brw_upload_binding_table,
};

void
gen4_init_vtable_surface_functions(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;

   intel->vtbl.update_texture_surface = brw_update_texture_surface;
   intel->vtbl.update_renderbuffer_surface = brw_update_renderbuffer_surface;
   intel->vtbl.update_null_renderbuffer_surface =
      brw_update_null_renderbuffer_surface;
   intel->vtbl.create_constant_surface = brw_create_constant_surface;
}

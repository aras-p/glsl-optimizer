/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (c) 2009  VMware, Inc.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */


/**
 * \file s_texfetch.c
 *
 * Texel fetch/store functions
 *
 * \author Gareth Hughes
 */


#include "main/colormac.h"
#include "main/macros.h"
#include "main/texcompress.h"
#include "main/texcompress_fxt1.h"
#include "main/texcompress_s3tc.h"
#include "main/texcompress_rgtc.h"
#include "main/texcompress_etc.h"
#include "main/teximage.h"
#include "main/samplerobj.h"
#include "s_context.h"
#include "s_texfetch.h"
#include "../../gallium/auxiliary/util/u_format_rgb9e5.h"
#include "../../gallium/auxiliary/util/u_format_r11g11b10f.h"


/**
 * Convert an 8-bit sRGB value from non-linear space to a
 * linear RGB value in [0, 1].
 * Implemented with a 256-entry lookup table.
 */
static inline GLfloat
nonlinear_to_linear(GLubyte cs8)
{
   static GLfloat table[256];
   static GLboolean tableReady = GL_FALSE;
   if (!tableReady) {
      /* compute lookup table now */
      GLuint i;
      for (i = 0; i < 256; i++) {
         const GLfloat cs = UBYTE_TO_FLOAT(i);
         if (cs <= 0.04045) {
            table[i] = cs / 12.92f;
         }
         else {
            table[i] = (GLfloat) pow((cs + 0.055) / 1.055, 2.4);
         }
      }
      tableReady = GL_TRUE;
   }
   return table[cs8];
}



/* Texel fetch routines for all supported formats
 */
#define DIM 1
#include "s_texfetch_tmp.h"

#define DIM 2
#include "s_texfetch_tmp.h"

#define DIM 3
#include "s_texfetch_tmp.h"


/**
 * All compressed texture texel fetching is done though this function.
 * Basically just call a core-Mesa texel fetch function.
 */
static void
fetch_compressed(const struct swrast_texture_image *swImage,
                 GLint i, GLint j, GLint k, GLfloat *texel)
{
   /* The FetchCompressedTexel function takes an integer pixel rowstride,
    * while the image's rowstride is bytes per row of blocks.
    */
   GLuint bw, bh;
   GLuint texelBytes = _mesa_get_format_bytes(swImage->Base.TexFormat);
   _mesa_get_format_block_size(swImage->Base.TexFormat, &bw, &bh);
   assert(swImage->RowStride * bw % texelBytes == 0);

   swImage->FetchCompressedTexel(swImage->ImageSlices[k],
                                 swImage->RowStride * bw / texelBytes,
                                 i, j, texel);
}



/**
 * Null texel fetch function.
 *
 * Have to have this so the FetchTexel function pointer is never NULL.
 */
static void fetch_null_texelf( const struct swrast_texture_image *texImage,
                               GLint i, GLint j, GLint k, GLfloat *texel )
{
   (void) texImage; (void) i; (void) j; (void) k;
   texel[RCOMP] = 0.0;
   texel[GCOMP] = 0.0;
   texel[BCOMP] = 0.0;
   texel[ACOMP] = 0.0;
   _mesa_warning(NULL, "fetch_null_texelf() called!");
}


/**
 * Table to map MESA_FORMAT_ to texel fetch/store funcs.
 * XXX this is somewhat temporary.
 */
static struct {
   mesa_format Name;
   FetchTexelFunc Fetch1D;
   FetchTexelFunc Fetch2D;
   FetchTexelFunc Fetch3D;
}
texfetch_funcs[] =
{
   {
      MESA_FORMAT_NONE,
      fetch_null_texelf,
      fetch_null_texelf,
      fetch_null_texelf
   },

   {
      MESA_FORMAT_A8B8G8R8_UNORM,
      fetch_texel_1d_f_rgba8888,
      fetch_texel_2d_f_rgba8888,
      fetch_texel_3d_f_rgba8888
   },
   {
      MESA_FORMAT_R8G8B8A8_UNORM,
      fetch_texel_1d_f_rgba8888_rev,
      fetch_texel_2d_f_rgba8888_rev,
      fetch_texel_3d_f_rgba8888_rev
   },
   {
      MESA_FORMAT_B8G8R8A8_UNORM,
      fetch_texel_1d_f_argb8888,
      fetch_texel_2d_f_argb8888,
      fetch_texel_3d_f_argb8888
   },
   {
      MESA_FORMAT_A8R8G8B8_UNORM,
      fetch_texel_1d_f_argb8888_rev,
      fetch_texel_2d_f_argb8888_rev,
      fetch_texel_3d_f_argb8888_rev
   },
   {
      MESA_FORMAT_X8B8G8R8_UNORM,
      fetch_texel_1d_f_rgbx8888,
      fetch_texel_2d_f_rgbx8888,
      fetch_texel_3d_f_rgbx8888
   },
   {
      MESA_FORMAT_R8G8B8X8_UNORM,
      fetch_texel_1d_f_rgbx8888_rev,
      fetch_texel_2d_f_rgbx8888_rev,
      fetch_texel_3d_f_rgbx8888_rev
   },
   {
      MESA_FORMAT_B8G8R8X8_UNORM,
      fetch_texel_1d_f_xrgb8888,
      fetch_texel_2d_f_xrgb8888,
      fetch_texel_3d_f_xrgb8888
   },
   {
      MESA_FORMAT_X8R8G8B8_UNORM,
      fetch_texel_1d_f_xrgb8888_rev,
      fetch_texel_2d_f_xrgb8888_rev,
      fetch_texel_3d_f_xrgb8888_rev
   },
   {
      MESA_FORMAT_BGR_UNORM8,
      fetch_texel_1d_f_rgb888,
      fetch_texel_2d_f_rgb888,
      fetch_texel_3d_f_rgb888
   },
   {
      MESA_FORMAT_RGB_UNORM8,
      fetch_texel_1d_f_bgr888,
      fetch_texel_2d_f_bgr888,
      fetch_texel_3d_f_bgr888
   },
   {
      MESA_FORMAT_B5G6R5_UNORM,
      fetch_texel_1d_f_rgb565,
      fetch_texel_2d_f_rgb565,
      fetch_texel_3d_f_rgb565
   },
   {
      MESA_FORMAT_R5G6B5_UNORM,
      fetch_texel_1d_f_rgb565_rev,
      fetch_texel_2d_f_rgb565_rev,
      fetch_texel_3d_f_rgb565_rev
   },
   {
      MESA_FORMAT_B4G4R4A4_UNORM,
      fetch_texel_1d_f_argb4444,
      fetch_texel_2d_f_argb4444,
      fetch_texel_3d_f_argb4444
   },
   {
      MESA_FORMAT_A4R4G4B4_UNORM,
      fetch_texel_1d_f_argb4444_rev,
      fetch_texel_2d_f_argb4444_rev,
      fetch_texel_3d_f_argb4444_rev
   },
   {
      MESA_FORMAT_A1B5G5R5_UNORM,
      fetch_texel_1d_f_rgba5551,
      fetch_texel_2d_f_rgba5551,
      fetch_texel_3d_f_rgba5551
   },
   {
      MESA_FORMAT_B5G5R5A1_UNORM,
      fetch_texel_1d_f_argb1555,
      fetch_texel_2d_f_argb1555,
      fetch_texel_3d_f_argb1555
   },
   {
      MESA_FORMAT_A1R5G5B5_UNORM,
      fetch_texel_1d_f_argb1555_rev,
      fetch_texel_2d_f_argb1555_rev,
      fetch_texel_3d_f_argb1555_rev
   },
   {
      MESA_FORMAT_L4A4_UNORM,
      fetch_texel_1d_f_al44,
      fetch_texel_2d_f_al44,
      fetch_texel_3d_f_al44
   },
   {
      MESA_FORMAT_L8A8_UNORM,
      fetch_texel_1d_f_al88,
      fetch_texel_2d_f_al88,
      fetch_texel_3d_f_al88
   },
   {
      MESA_FORMAT_A8L8_UNORM,
      fetch_texel_1d_f_al88_rev,
      fetch_texel_2d_f_al88_rev,
      fetch_texel_3d_f_al88_rev
   },
   {
      MESA_FORMAT_L16A16_UNORM,
      fetch_texel_1d_f_al1616,
      fetch_texel_2d_f_al1616,
      fetch_texel_3d_f_al1616
   },
   {
      MESA_FORMAT_A16L16_UNORM,
      fetch_texel_1d_f_al1616_rev,
      fetch_texel_2d_f_al1616_rev,
      fetch_texel_3d_f_al1616_rev
   },
   {
      MESA_FORMAT_B2G3R3_UNORM,
      fetch_texel_1d_f_rgb332,
      fetch_texel_2d_f_rgb332,
      fetch_texel_3d_f_rgb332
   },
   {
      MESA_FORMAT_A_UNORM8,
      fetch_texel_1d_f_a8,
      fetch_texel_2d_f_a8,
      fetch_texel_3d_f_a8
   },
   {
      MESA_FORMAT_A_UNORM16,
      fetch_texel_1d_f_a16,
      fetch_texel_2d_f_a16,
      fetch_texel_3d_f_a16
   },
   {
      MESA_FORMAT_L_UNORM8,
      fetch_texel_1d_f_l8,
      fetch_texel_2d_f_l8,
      fetch_texel_3d_f_l8
   },
   {
      MESA_FORMAT_L_UNORM16,
      fetch_texel_1d_f_l16,
      fetch_texel_2d_f_l16,
      fetch_texel_3d_f_l16
   },
   {
      MESA_FORMAT_I_UNORM8,
      fetch_texel_1d_f_i8,
      fetch_texel_2d_f_i8,
      fetch_texel_3d_f_i8
   },
   {
      MESA_FORMAT_I_UNORM16,
      fetch_texel_1d_f_i16,
      fetch_texel_2d_f_i16,
      fetch_texel_3d_f_i16
   },
   {
      MESA_FORMAT_YCBCR,
      fetch_texel_1d_f_ycbcr,
      fetch_texel_2d_f_ycbcr,
      fetch_texel_3d_f_ycbcr
   },
   {
      MESA_FORMAT_YCBCR_REV,
      fetch_texel_1d_f_ycbcr_rev,
      fetch_texel_2d_f_ycbcr_rev,
      fetch_texel_3d_f_ycbcr_rev
   },
   {
      MESA_FORMAT_R_UNORM8,
      fetch_texel_1d_f_r8,
      fetch_texel_2d_f_r8,
      fetch_texel_3d_f_r8
   },
   {
      MESA_FORMAT_R8G8_UNORM,
      fetch_texel_1d_f_gr88,
      fetch_texel_2d_f_gr88,
      fetch_texel_3d_f_gr88
   },
   {
      MESA_FORMAT_G8R8_UNORM,
      fetch_texel_1d_f_rg88,
      fetch_texel_2d_f_rg88,
      fetch_texel_3d_f_rg88
   },
   {
      MESA_FORMAT_R_UNORM16,
      fetch_texel_1d_f_r16,
      fetch_texel_2d_f_r16,
      fetch_texel_3d_f_r16
   },
   {
      MESA_FORMAT_R16G16_UNORM,
      fetch_texel_1d_f_rg1616,
      fetch_texel_2d_f_rg1616,
      fetch_texel_3d_f_rg1616
   },
   {
      MESA_FORMAT_G16R16_UNORM,
      fetch_texel_1d_f_rg1616_rev,
      fetch_texel_2d_f_rg1616_rev,
      fetch_texel_3d_f_rg1616_rev
   },
   {
      MESA_FORMAT_B10G10R10A2_UNORM,
      fetch_texel_1d_f_argb2101010,
      fetch_texel_2d_f_argb2101010,
      fetch_texel_3d_f_argb2101010
   },
   {
      MESA_FORMAT_S8_UINT_Z24_UNORM,
      fetch_texel_1d_f_z24_s8,
      fetch_texel_2d_f_z24_s8,
      fetch_texel_3d_f_z24_s8
   },
   {
      MESA_FORMAT_Z24_UNORM_S8_UINT,
      fetch_texel_1d_f_s8_z24,
      fetch_texel_2d_f_s8_z24,
      fetch_texel_3d_f_s8_z24
   },
   {
      MESA_FORMAT_Z_UNORM16,
      fetch_texel_1d_f_z16,
      fetch_texel_2d_f_z16,
      fetch_texel_3d_f_z16
   },
   {
      MESA_FORMAT_Z24_UNORM_X8_UINT,
      fetch_texel_1d_f_s8_z24,
      fetch_texel_2d_f_s8_z24,
      fetch_texel_3d_f_s8_z24
   },
   {
      MESA_FORMAT_X8Z24_UNORM,
      fetch_texel_1d_f_z24_s8,
      fetch_texel_2d_f_z24_s8,
      fetch_texel_3d_f_z24_s8
   },
   {
      MESA_FORMAT_Z_UNORM32,
      fetch_texel_1d_f_z32,
      fetch_texel_2d_f_z32,
      fetch_texel_3d_f_z32
   },
   {
      MESA_FORMAT_S_UINT8,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_BGR_SRGB8,
      fetch_texel_1d_srgb8,
      fetch_texel_2d_srgb8,
      fetch_texel_3d_srgb8
   },
   {
      MESA_FORMAT_A8B8G8R8_SRGB,
      fetch_texel_1d_srgba8,
      fetch_texel_2d_srgba8,
      fetch_texel_3d_srgba8
   },
   {
      MESA_FORMAT_B8G8R8A8_SRGB,
      fetch_texel_1d_sargb8,
      fetch_texel_2d_sargb8,
      fetch_texel_3d_sargb8
   },
   {
      MESA_FORMAT_L_SRGB8,
      fetch_texel_1d_sl8,
      fetch_texel_2d_sl8,
      fetch_texel_3d_sl8
   },
   {
      MESA_FORMAT_L8A8_SRGB,
      fetch_texel_1d_sla8,
      fetch_texel_2d_sla8,
      fetch_texel_3d_sla8
   },
   {
      MESA_FORMAT_SRGB_DXT1,
      fetch_compressed,
      fetch_compressed,
      fetch_compressed
   },
   {
      MESA_FORMAT_SRGBA_DXT1,
      fetch_compressed,
      fetch_compressed,
      fetch_compressed
   },
   {
      MESA_FORMAT_SRGBA_DXT3,
      fetch_compressed,
      fetch_compressed,
      fetch_compressed
   },
   {
      MESA_FORMAT_SRGBA_DXT5,
      fetch_compressed,
      fetch_compressed,
      fetch_compressed
   },

   {
      MESA_FORMAT_RGB_FXT1,
      fetch_compressed,
      fetch_compressed,
      fetch_compressed
   },
   {
      MESA_FORMAT_RGBA_FXT1,
      fetch_compressed,
      fetch_compressed,
      fetch_compressed
   },
   {
      MESA_FORMAT_RGB_DXT1,
      fetch_compressed,
      fetch_compressed,
      fetch_compressed
   },
   {
      MESA_FORMAT_RGBA_DXT1,
      fetch_compressed,
      fetch_compressed,
      fetch_compressed
   },
   {
      MESA_FORMAT_RGBA_DXT3,
      fetch_compressed,
      fetch_compressed,
      fetch_compressed
   },
   {
      MESA_FORMAT_RGBA_DXT5,
      fetch_compressed,
      fetch_compressed,
      fetch_compressed
   },
   {
      MESA_FORMAT_RGBA_FLOAT32,
      fetch_texel_1d_f_rgba_f32,
      fetch_texel_2d_f_rgba_f32,
      fetch_texel_3d_f_rgba_f32
   },
   {
      MESA_FORMAT_RGBA_FLOAT16,
      fetch_texel_1d_f_rgba_f16,
      fetch_texel_2d_f_rgba_f16,
      fetch_texel_3d_f_rgba_f16
   },
   {
      MESA_FORMAT_RGB_FLOAT32,
      fetch_texel_1d_f_rgb_f32,
      fetch_texel_2d_f_rgb_f32,
      fetch_texel_3d_f_rgb_f32
   },
   {
      MESA_FORMAT_RGB_FLOAT16,
      fetch_texel_1d_f_rgb_f16,
      fetch_texel_2d_f_rgb_f16,
      fetch_texel_3d_f_rgb_f16
   },
   {
      MESA_FORMAT_A_FLOAT32,
      fetch_texel_1d_f_alpha_f32,
      fetch_texel_2d_f_alpha_f32,
      fetch_texel_3d_f_alpha_f32
   },
   {
      MESA_FORMAT_A_FLOAT16,
      fetch_texel_1d_f_alpha_f16,
      fetch_texel_2d_f_alpha_f16,
      fetch_texel_3d_f_alpha_f16
   },
   {
      MESA_FORMAT_L_FLOAT32,
      fetch_texel_1d_f_luminance_f32,
      fetch_texel_2d_f_luminance_f32,
      fetch_texel_3d_f_luminance_f32
   },
   {
      MESA_FORMAT_L_FLOAT16,
      fetch_texel_1d_f_luminance_f16,
      fetch_texel_2d_f_luminance_f16,
      fetch_texel_3d_f_luminance_f16
   },
   {
      MESA_FORMAT_LA_FLOAT32,
      fetch_texel_1d_f_luminance_alpha_f32,
      fetch_texel_2d_f_luminance_alpha_f32,
      fetch_texel_3d_f_luminance_alpha_f32
   },
   {
      MESA_FORMAT_LA_FLOAT16,
      fetch_texel_1d_f_luminance_alpha_f16,
      fetch_texel_2d_f_luminance_alpha_f16,
      fetch_texel_3d_f_luminance_alpha_f16
   },
   {
      MESA_FORMAT_I_FLOAT32,
      fetch_texel_1d_f_intensity_f32,
      fetch_texel_2d_f_intensity_f32,
      fetch_texel_3d_f_intensity_f32
   },
   {
      MESA_FORMAT_I_FLOAT16,
      fetch_texel_1d_f_intensity_f16,
      fetch_texel_2d_f_intensity_f16,
      fetch_texel_3d_f_intensity_f16
   },
   {
      MESA_FORMAT_R_FLOAT32,
      fetch_texel_1d_f_r_f32,
      fetch_texel_2d_f_r_f32,
      fetch_texel_3d_f_r_f32
   },
   {
      MESA_FORMAT_R_FLOAT16,
      fetch_texel_1d_f_r_f16,
      fetch_texel_2d_f_r_f16,
      fetch_texel_3d_f_r_f16
   },
   {
      MESA_FORMAT_RG_FLOAT32,
      fetch_texel_1d_f_rg_f32,
      fetch_texel_2d_f_rg_f32,
      fetch_texel_3d_f_rg_f32
   },
   {
      MESA_FORMAT_RG_FLOAT16,
      fetch_texel_1d_f_rg_f16,
      fetch_texel_2d_f_rg_f16,
      fetch_texel_3d_f_rg_f16
   },

   {
      MESA_FORMAT_A_UINT8,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_A_UINT16,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_A_UINT32,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_A_SINT8,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_A_SINT16,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_A_SINT32,
      NULL,
      NULL,
      NULL
   },


   {
      MESA_FORMAT_I_UINT8,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_I_UINT16,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_I_UINT32,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_I_SINT8,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_I_SINT16,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_I_SINT32,
      NULL,
      NULL,
      NULL
   },


   {
      MESA_FORMAT_L_UINT8,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_L_UINT16,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_L_UINT32,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_L_SINT8,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_L_SINT16,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_L_SINT32,
      NULL,
      NULL,
      NULL
   },


   {
      MESA_FORMAT_LA_UINT8,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_LA_UINT16,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_LA_UINT32,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_LA_SINT8,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_LA_SINT16,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_LA_SINT32,
      NULL,
      NULL,
      NULL
   },


   {
      MESA_FORMAT_R_SINT8,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_RG_SINT8,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_RGB_SINT8,
      NULL,
      NULL,
      NULL
   },

   /* non-normalized, signed int */
   {
      MESA_FORMAT_RGBA_SINT8,
      fetch_texel_1d_rgba_int8,
      fetch_texel_2d_rgba_int8,
      fetch_texel_3d_rgba_int8
   },
   {
      MESA_FORMAT_R_SINT16,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RG_SINT16,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RGB_SINT16,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RGBA_SINT16,
      fetch_texel_1d_rgba_int16,
      fetch_texel_2d_rgba_int16,
      fetch_texel_3d_rgba_int16
   },
   {
      MESA_FORMAT_R_SINT32,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RG_SINT32,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RGB_SINT32,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RGBA_SINT32,
      fetch_texel_1d_rgba_int32,
      fetch_texel_2d_rgba_int32,
      fetch_texel_3d_rgba_int32
   },

   /* non-normalized, unsigned int */
   {
      MESA_FORMAT_R_UINT8,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RG_UINT8,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RGB_UINT8,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RGBA_UINT8,
      fetch_texel_1d_rgba_uint8,
      fetch_texel_2d_rgba_uint8,
      fetch_texel_3d_rgba_uint8
   },
   {
      MESA_FORMAT_R_UINT16,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RG_UINT16,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RGB_UINT16,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RGBA_UINT16,
      fetch_texel_1d_rgba_uint16,
      fetch_texel_2d_rgba_uint16,
      fetch_texel_3d_rgba_uint16
   },
   {
      MESA_FORMAT_R_UINT32,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RG_UINT32,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RGB_UINT32,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RGBA_UINT32,
      fetch_texel_1d_rgba_uint32,
      fetch_texel_2d_rgba_uint32,
      fetch_texel_3d_rgba_uint32
   },

   /* dudv */
   {
      MESA_FORMAT_DUDV8,
      fetch_texel_1d_dudv8,
      fetch_texel_2d_dudv8,
      fetch_texel_3d_dudv8
   },

   /* signed, normalized */
   {
      MESA_FORMAT_R_SNORM8,
      fetch_texel_1d_signed_r8,
      fetch_texel_2d_signed_r8,
      fetch_texel_3d_signed_r8
   },
   {
      MESA_FORMAT_R8G8_SNORM,
      fetch_texel_1d_signed_rg88_rev,
      fetch_texel_2d_signed_rg88_rev,
      fetch_texel_3d_signed_rg88_rev
   },
   {
      MESA_FORMAT_X8B8G8R8_SNORM,
      fetch_texel_1d_signed_rgbx8888,
      fetch_texel_2d_signed_rgbx8888,
      fetch_texel_3d_signed_rgbx8888
   },
   {
      MESA_FORMAT_A8B8G8R8_SNORM,
      fetch_texel_1d_signed_rgba8888,
      fetch_texel_2d_signed_rgba8888,
      fetch_texel_3d_signed_rgba8888
   },
   {
      MESA_FORMAT_R8G8B8A8_SNORM,
      fetch_texel_1d_signed_rgba8888_rev,
      fetch_texel_2d_signed_rgba8888_rev,
      fetch_texel_3d_signed_rgba8888_rev
   },
   {
      MESA_FORMAT_R_SNORM16,
      fetch_texel_1d_signed_r16,
      fetch_texel_2d_signed_r16,
      fetch_texel_3d_signed_r16
   },
   {
      MESA_FORMAT_R16G16_SNORM,
      fetch_texel_1d_signed_rg1616,
      fetch_texel_2d_signed_rg1616,
      fetch_texel_3d_signed_rg1616
   },
   {
      MESA_FORMAT_RGB_SNORM16,
      fetch_texel_1d_signed_rgb_16,
      fetch_texel_2d_signed_rgb_16,
      fetch_texel_3d_signed_rgb_16
   },
   {
      MESA_FORMAT_RGBA_SNORM16,
      fetch_texel_1d_signed_rgba_16,
      fetch_texel_2d_signed_rgba_16,
      fetch_texel_3d_signed_rgba_16
   },
   {
      MESA_FORMAT_RGBA_UNORM16,
      fetch_texel_1d_rgba_16,
      fetch_texel_2d_rgba_16,
      fetch_texel_3d_rgba_16
   },
   {
      MESA_FORMAT_R_RGTC1_UNORM,
      fetch_compressed,
      fetch_compressed,
      fetch_compressed
   },
   {
      MESA_FORMAT_R_RGTC1_SNORM,
      fetch_compressed,
      fetch_compressed,
      fetch_compressed
   },
   {
      MESA_FORMAT_RG_RGTC2_UNORM,
      fetch_compressed,
      fetch_compressed,
      fetch_compressed
   },
   {
      MESA_FORMAT_RG_RGTC2_SNORM,
      fetch_compressed,
      fetch_compressed,
      fetch_compressed
   },
   {
      MESA_FORMAT_L_LATC1_UNORM,
      fetch_compressed,
      fetch_compressed,
      fetch_compressed
   },
   {
      MESA_FORMAT_L_LATC1_SNORM,
      fetch_compressed,
      fetch_compressed,
      fetch_compressed
   },
   {
      MESA_FORMAT_LA_LATC2_UNORM,
      fetch_compressed,
      fetch_compressed,
      fetch_compressed
   },
   {
      MESA_FORMAT_LA_LATC2_SNORM,
      fetch_compressed,
      fetch_compressed,
      fetch_compressed
   },
   {
      MESA_FORMAT_ETC1_RGB8,
      fetch_compressed,
      fetch_compressed,
      fetch_compressed
   },
   {
      MESA_FORMAT_ETC2_RGB8,
      fetch_compressed,
      fetch_compressed,
      fetch_compressed
   },
   {
      MESA_FORMAT_ETC2_SRGB8,
      fetch_compressed,
      fetch_compressed,
      fetch_compressed
   },
   {
      MESA_FORMAT_ETC2_RGBA8_EAC,
      fetch_compressed,
      fetch_compressed,
      fetch_compressed
   },
   {
      MESA_FORMAT_ETC2_SRGB8_ALPHA8_EAC,
      fetch_compressed,
      fetch_compressed,
      fetch_compressed
   },
   {
      MESA_FORMAT_ETC2_R11_EAC,
      fetch_compressed,
      fetch_compressed,
      fetch_compressed
   },
   {
      MESA_FORMAT_ETC2_RG11_EAC,
      fetch_compressed,
      fetch_compressed,
      fetch_compressed
   },
   {
      MESA_FORMAT_ETC2_SIGNED_R11_EAC,
      fetch_compressed,
      fetch_compressed,
      fetch_compressed
   },
   {
      MESA_FORMAT_ETC2_SIGNED_RG11_EAC,
      fetch_compressed,
      fetch_compressed,
      fetch_compressed
   },
   {
      MESA_FORMAT_ETC2_RGB8_PUNCHTHROUGH_ALPHA1,
      fetch_compressed,
      fetch_compressed,
      fetch_compressed
   },
   {
      MESA_FORMAT_ETC2_SRGB8_PUNCHTHROUGH_ALPHA1,
      fetch_compressed,
      fetch_compressed,
      fetch_compressed
   },
   {
      MESA_FORMAT_A_SNORM8,
      fetch_texel_1d_signed_a8,
      fetch_texel_2d_signed_a8,
      fetch_texel_3d_signed_a8
   },
   {
      MESA_FORMAT_L_SNORM8,
      fetch_texel_1d_signed_l8,
      fetch_texel_2d_signed_l8,
      fetch_texel_3d_signed_l8
   },
   {
      MESA_FORMAT_L8A8_SNORM,
      fetch_texel_1d_signed_al88,
      fetch_texel_2d_signed_al88,
      fetch_texel_3d_signed_al88
   },
   {
      MESA_FORMAT_I_SNORM8,
      fetch_texel_1d_signed_i8,
      fetch_texel_2d_signed_i8,
      fetch_texel_3d_signed_i8
   },
   {
      MESA_FORMAT_A_SNORM16,
      fetch_texel_1d_signed_a16,
      fetch_texel_2d_signed_a16,
      fetch_texel_3d_signed_a16
   },
   {
      MESA_FORMAT_L_SNORM16,
      fetch_texel_1d_signed_l16,
      fetch_texel_2d_signed_l16,
      fetch_texel_3d_signed_l16
   },
   {
      MESA_FORMAT_LA_SNORM16,
      fetch_texel_1d_signed_al1616,
      fetch_texel_2d_signed_al1616,
      fetch_texel_3d_signed_al1616
   },
   {
      MESA_FORMAT_I_SNORM16,
      fetch_texel_1d_signed_i16,
      fetch_texel_2d_signed_i16,
      fetch_texel_3d_signed_i16
   },
   {
      MESA_FORMAT_R9G9B9E5_FLOAT,
      fetch_texel_1d_rgb9_e5,
      fetch_texel_2d_rgb9_e5,
      fetch_texel_3d_rgb9_e5
   },
   {
      MESA_FORMAT_R11G11B10_FLOAT,
      fetch_texel_1d_r11_g11_b10f,
      fetch_texel_2d_r11_g11_b10f,
      fetch_texel_3d_r11_g11_b10f
   },
   {
      MESA_FORMAT_Z_FLOAT32,
      fetch_texel_1d_f_r_f32, /* Reuse the R32F functions. */
      fetch_texel_2d_f_r_f32,
      fetch_texel_3d_f_r_f32
   },
   {
      MESA_FORMAT_Z32_FLOAT_S8X24_UINT,
      fetch_texel_1d_z32f_x24s8,
      fetch_texel_2d_z32f_x24s8,
      fetch_texel_3d_z32f_x24s8
   },
   {
      MESA_FORMAT_B10G10R10A2_UINT,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_R10G10B10A2_UINT,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_B4G4R4X4_UNORM,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_B5G5R5X1_UNORM,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_R8G8B8X8_SNORM,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_R8G8B8X8_SRGB,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RGBX_UINT8,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RGBX_SINT8,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_B10G10R10X2_UNORM,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RGBX_UNORM16,
      fetch_texel_1d_xbgr16161616_unorm,
      fetch_texel_2d_xbgr16161616_unorm,
      fetch_texel_3d_xbgr16161616_unorm
   },
   {
      MESA_FORMAT_RGBX_SNORM16,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RGBX_FLOAT16,
      fetch_texel_1d_xbgr16161616_float,
      fetch_texel_2d_xbgr16161616_float,
      fetch_texel_3d_xbgr16161616_float
   },
   {
      MESA_FORMAT_RGBX_UINT16,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RGBX_SINT16,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RGBX_FLOAT32,
      fetch_texel_1d_xbgr32323232_float,
      fetch_texel_2d_xbgr32323232_float,
      fetch_texel_3d_xbgr32323232_float
   },
   {
      MESA_FORMAT_RGBX_UINT32,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RGBX_SINT32,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_R10G10B10A2_UNORM,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_G8R8_SNORM,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_G16R16_SNORM,
      NULL,
      NULL,
      NULL
   },
};


/**
 * Initialize the texture image's FetchTexel methods.
 */
static void
set_fetch_functions(const struct gl_sampler_object *samp,
                    struct swrast_texture_image *texImage, GLuint dims)
{
   mesa_format format = texImage->Base.TexFormat;

#ifdef DEBUG
   /* check that the table entries are sorted by format name */
   mesa_format fmt;
   for (fmt = 0; fmt < MESA_FORMAT_COUNT; fmt++) {
      assert(texfetch_funcs[fmt].Name == fmt);
   }
#endif

   STATIC_ASSERT(Elements(texfetch_funcs) == MESA_FORMAT_COUNT);

   if (samp->sRGBDecode == GL_SKIP_DECODE_EXT &&
       _mesa_get_format_color_encoding(format) == GL_SRGB) {
      format = _mesa_get_srgb_format_linear(format);
   }

   assert(format < MESA_FORMAT_COUNT);

   switch (dims) {
   case 1:
      texImage->FetchTexel = texfetch_funcs[format].Fetch1D;
      break;
   case 2:
      texImage->FetchTexel = texfetch_funcs[format].Fetch2D;
      break;
   case 3:
      texImage->FetchTexel = texfetch_funcs[format].Fetch3D;
      break;
   default:
      assert(!"Bad dims in set_fetch_functions()");
   }

   texImage->FetchCompressedTexel = _mesa_get_compressed_fetch_func(format);

   ASSERT(texImage->FetchTexel);
}

void
_mesa_update_fetch_functions(struct gl_context *ctx, GLuint unit)
{
   struct gl_texture_object *texObj = ctx->Texture.Unit[unit]._Current;
   struct gl_sampler_object *samp;
   GLuint face, i;
   GLuint dims;

   if (!texObj)
      return;

   samp = _mesa_get_samplerobj(ctx, unit);

   dims = _mesa_get_texture_dimensions(texObj->Target);

   for (face = 0; face < 6; face++) {
      for (i = 0; i < MAX_TEXTURE_LEVELS; i++) {
         if (texObj->Image[face][i]) {
	    set_fetch_functions(samp,
                                swrast_texture_image(texObj->Image[face][i]),
                                dims);
         }
      }
   }
}

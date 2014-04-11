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


#define FETCH_FUNCS(NAME)       \
   {                            \
      MESA_FORMAT_ ## NAME,     \
      fetch_texel_1d_ ## NAME,  \
      fetch_texel_2d_ ## NAME,  \
      fetch_texel_3d_ ## NAME,  \
   }

#define FETCH_NULL(NAME)        \
   {                            \
      MESA_FORMAT_ ## NAME,     \
      NULL,                     \
      NULL,                     \
      NULL                      \
   }

/**
 * Table to map MESA_FORMAT_ to texel fetch/store funcs.
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

   /* Packed unorm formats */
   FETCH_FUNCS(A8B8G8R8_UNORM),
   FETCH_FUNCS(X8B8G8R8_UNORM),
   FETCH_FUNCS(R8G8B8A8_UNORM),
   FETCH_FUNCS(R8G8B8X8_UNORM),
   FETCH_FUNCS(B8G8R8A8_UNORM),
   FETCH_FUNCS(B8G8R8X8_UNORM),
   FETCH_FUNCS(A8R8G8B8_UNORM),
   FETCH_FUNCS(X8R8G8B8_UNORM),
   FETCH_FUNCS(L16A16_UNORM),
   FETCH_FUNCS(A16L16_UNORM),
   FETCH_FUNCS(B5G6R5_UNORM),
   FETCH_FUNCS(R5G6B5_UNORM),
   FETCH_FUNCS(B4G4R4A4_UNORM),
   FETCH_NULL(B4G4R4X4_UNORM),
   FETCH_FUNCS(A4R4G4B4_UNORM),
   FETCH_FUNCS(A1B5G5R5_UNORM),
   FETCH_FUNCS(B5G5R5A1_UNORM),
   FETCH_NULL(B5G5R5X1_UNORM),
   FETCH_FUNCS(A1R5G5B5_UNORM),
   FETCH_FUNCS(L8A8_UNORM),
   FETCH_FUNCS(A8L8_UNORM),
   FETCH_FUNCS(R8G8_UNORM),
   FETCH_FUNCS(G8R8_UNORM),
   FETCH_FUNCS(L4A4_UNORM),
   FETCH_FUNCS(B2G3R3_UNORM),
   FETCH_FUNCS(R16G16_UNORM),
   FETCH_FUNCS(G16R16_UNORM),
   FETCH_FUNCS(B10G10R10A2_UNORM),
   FETCH_NULL(B10G10R10X2_UNORM),
   FETCH_FUNCS(R10G10B10A2_UNORM),
   FETCH_FUNCS(S8_UINT_Z24_UNORM),
   {
      MESA_FORMAT_X8_UINT_Z24_UNORM,
      fetch_texel_1d_S8_UINT_Z24_UNORM,
      fetch_texel_2d_S8_UINT_Z24_UNORM,
      fetch_texel_3d_S8_UINT_Z24_UNORM
   },
   FETCH_FUNCS(Z24_UNORM_S8_UINT),
   {
      MESA_FORMAT_Z24_UNORM_X8_UINT,
      fetch_texel_1d_Z24_UNORM_S8_UINT,
      fetch_texel_2d_Z24_UNORM_S8_UINT,
      fetch_texel_3d_Z24_UNORM_S8_UINT
   },
   FETCH_FUNCS(YCBCR),
   FETCH_FUNCS(YCBCR_REV),
   FETCH_FUNCS(DUDV8),

   /* Array unorm formats */
   FETCH_FUNCS(A_UNORM8),
   FETCH_FUNCS(A_UNORM16),
   FETCH_FUNCS(L_UNORM8),
   FETCH_FUNCS(L_UNORM16),
   FETCH_FUNCS(I_UNORM8),
   FETCH_FUNCS(I_UNORM16),
   FETCH_FUNCS(R_UNORM8),
   FETCH_FUNCS(R_UNORM16),
   FETCH_FUNCS(BGR_UNORM8),
   FETCH_FUNCS(RGB_UNORM8),
   FETCH_FUNCS(RGBA_UNORM16),
   FETCH_FUNCS(RGBX_UNORM16),
   FETCH_FUNCS(Z_UNORM16),
   FETCH_FUNCS(Z_UNORM32),
   FETCH_NULL(S_UINT8),

   /* Packed signed/normalized formats */
   FETCH_FUNCS(A8B8G8R8_SNORM),
   FETCH_FUNCS(X8B8G8R8_SNORM),
   FETCH_FUNCS(R8G8B8A8_SNORM),
   FETCH_NULL(R8G8B8X8_SNORM),
   FETCH_FUNCS(R16G16_SNORM),
   FETCH_NULL(G16R16_SNORM),
   FETCH_FUNCS(R8G8_SNORM),
   FETCH_NULL(G8R8_SNORM),
   FETCH_FUNCS(L8A8_SNORM),

   /* Array signed/normalized formats */
   FETCH_FUNCS(A_SNORM8),
   FETCH_FUNCS(A_SNORM16),
   FETCH_FUNCS(L_SNORM8),
   FETCH_FUNCS(L_SNORM16),
   FETCH_FUNCS(I_SNORM8),
   FETCH_FUNCS(I_SNORM16),
   FETCH_FUNCS(R_SNORM8),
   FETCH_FUNCS(R_SNORM16),
   FETCH_FUNCS(LA_SNORM16),
   FETCH_FUNCS(RGB_SNORM16),
   FETCH_FUNCS(RGBA_SNORM16),
   FETCH_NULL(RGBX_SNORM16),

   /* Packed sRGB formats */
   FETCH_FUNCS(A8B8G8R8_SRGB),
   FETCH_FUNCS(B8G8R8A8_SRGB),
   FETCH_NULL(B8G8R8X8_SRGB),
   FETCH_FUNCS(R8G8B8A8_SRGB),
   FETCH_FUNCS(R8G8B8X8_SRGB),
   FETCH_FUNCS(L8A8_SRGB),

   /* Array sRGB formats */
   FETCH_FUNCS(L_SRGB8),
   FETCH_FUNCS(BGR_SRGB8),

   /* Packed float formats */
   FETCH_FUNCS(R9G9B9E5_FLOAT),
   FETCH_FUNCS(R11G11B10_FLOAT),
   FETCH_FUNCS(Z32_FLOAT_S8X24_UINT),

   /* Array float formats */
   FETCH_FUNCS(A_FLOAT16),
   FETCH_FUNCS(A_FLOAT32),
   FETCH_FUNCS(L_FLOAT16),
   FETCH_FUNCS(L_FLOAT32),
   FETCH_FUNCS(LA_FLOAT16),
   FETCH_FUNCS(LA_FLOAT32),
   FETCH_FUNCS(I_FLOAT16),
   FETCH_FUNCS(I_FLOAT32),
   FETCH_FUNCS(R_FLOAT16),
   FETCH_FUNCS(R_FLOAT32),
   FETCH_FUNCS(RG_FLOAT16),
   FETCH_FUNCS(RG_FLOAT32),
   FETCH_FUNCS(RGB_FLOAT16),
   FETCH_FUNCS(RGB_FLOAT32),
   FETCH_FUNCS(RGBA_FLOAT16),
   FETCH_FUNCS(RGBA_FLOAT32),
   FETCH_FUNCS(RGBX_FLOAT16),
   FETCH_FUNCS(RGBX_FLOAT32),
   {
      MESA_FORMAT_Z_FLOAT32,
      fetch_texel_1d_R_FLOAT32, /* Reuse the R32F functions. */
      fetch_texel_2d_R_FLOAT32,
      fetch_texel_3d_R_FLOAT32
   },

   /* Packed signed/unsigned non-normalized integer formats */
   FETCH_NULL(B10G10R10A2_UINT),
   FETCH_NULL(R10G10B10A2_UINT),

   /* Array signed/unsigned non-normalized integer formats */
   FETCH_NULL(A_UINT8),
   FETCH_NULL(A_UINT16),
   FETCH_NULL(A_UINT32),
   FETCH_NULL(A_SINT8),
   FETCH_NULL(A_SINT16),
   FETCH_NULL(A_SINT32),
   FETCH_NULL(I_UINT8),
   FETCH_NULL(I_UINT16),
   FETCH_NULL(I_UINT32),
   FETCH_NULL(I_SINT8),
   FETCH_NULL(I_SINT16),
   FETCH_NULL(I_SINT32),
   FETCH_NULL(L_UINT8),
   FETCH_NULL(L_UINT16),
   FETCH_NULL(L_UINT32),
   FETCH_NULL(L_SINT8),
   FETCH_NULL(L_SINT16),
   FETCH_NULL(L_SINT32),
   FETCH_NULL(LA_UINT8),
   FETCH_NULL(LA_UINT16),
   FETCH_NULL(LA_UINT32),
   FETCH_NULL(LA_SINT8),
   FETCH_NULL(LA_SINT16),
   FETCH_NULL(LA_SINT32),
   FETCH_NULL(R_UINT8),
   FETCH_NULL(R_UINT16),
   FETCH_NULL(R_UINT32),
   FETCH_NULL(R_SINT8),
   FETCH_NULL(R_SINT16),
   FETCH_NULL(R_SINT32),
   FETCH_NULL(RG_UINT8),
   FETCH_NULL(RG_UINT16),
   FETCH_NULL(RG_UINT32),
   FETCH_NULL(RG_SINT8),
   FETCH_NULL(RG_SINT16),
   FETCH_NULL(RG_SINT32),
   FETCH_NULL(RGB_UINT8),
   FETCH_NULL(RGB_UINT16),
   FETCH_NULL(RGB_UINT32),
   FETCH_NULL(RGB_SINT8),
   FETCH_NULL(RGB_SINT16),
   FETCH_NULL(RGB_SINT32),
   FETCH_FUNCS(RGBA_UINT8),
   FETCH_FUNCS(RGBA_UINT16),
   FETCH_FUNCS(RGBA_UINT32),
   FETCH_FUNCS(RGBA_SINT8),
   FETCH_FUNCS(RGBA_SINT16),
   FETCH_FUNCS(RGBA_SINT32),
   FETCH_NULL(RGBX_UINT8),
   FETCH_NULL(RGBX_UINT16),
   FETCH_NULL(RGBX_UINT32),
   FETCH_NULL(RGBX_SINT8),
   FETCH_NULL(RGBX_SINT16),
   FETCH_NULL(RGBX_SINT32),

   /* DXT compressed formats */
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

   /* DXT sRGB compressed formats */
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

   /* FXT1 compressed formats */
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

   /* RGTC compressed formats */
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

   /* LATC1/2 compressed formats */
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

   /* ETC1/2 compressed formats */
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
   }
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

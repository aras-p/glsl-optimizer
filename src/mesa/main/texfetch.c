/*
 * Mesa 3-D graphics library
 * Version:  7.7
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
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/**
 * \file texfetch.c
 *
 * Texel fetch/store functions
 *
 * \author Gareth Hughes
 */


#include "colormac.h"
#include "context.h"
#include "texcompress.h"
#include "texcompress_fxt1.h"
#include "texcompress_s3tc.h"
#include "texfetch.h"


/**
 * Convert an 8-bit sRGB value from non-linear space to a
 * linear RGB value in [0, 1].
 * Implemented with a 256-entry lookup table.
 */
static INLINE GLfloat
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
            table[i] = (GLfloat) _mesa_pow((cs + 0.055) / 1.055, 2.4);
         }
      }
      tableReady = GL_TRUE;
   }
   return table[cs8];
}



/* Texel fetch routines for all supported formats
 */
#define DIM 1
#include "texfetch_tmp.h"

#define DIM 2
#include "texfetch_tmp.h"

#define DIM 3
#include "texfetch_tmp.h"

/**
 * Null texel fetch function.
 *
 * Have to have this so the FetchTexel function pointer is never NULL.
 */
static void fetch_null_texelf( const struct gl_texture_image *texImage,
                               GLint i, GLint j, GLint k, GLfloat *texel )
{
   (void) texImage; (void) i; (void) j; (void) k;
   texel[RCOMP] = 0.0;
   texel[GCOMP] = 0.0;
   texel[BCOMP] = 0.0;
   texel[ACOMP] = 0.0;
   _mesa_warning(NULL, "fetch_null_texelf() called!");
}

static void store_null_texel(struct gl_texture_image *texImage,
                             GLint i, GLint j, GLint k, const void *texel)
{
   (void) texImage;
   (void) i;
   (void) j;
   (void) k;
   (void) texel;
   /* no-op */
}



/**
 * Table to map MESA_FORMAT_ to texel fetch/store funcs.
 * XXX this is somewhat temporary.
 */
static struct {
   GLuint Name;
   FetchTexelFuncF Fetch1D;
   FetchTexelFuncF Fetch2D;
   FetchTexelFuncF Fetch3D;
   StoreTexelFunc StoreTexel;
}
texfetch_funcs[MESA_FORMAT_COUNT] =
{
   {
      MESA_FORMAT_SRGB8,
      fetch_texel_1d_srgb8,
      fetch_texel_2d_srgb8,
      fetch_texel_3d_srgb8,
      store_texel_srgb8
   },
   {
      MESA_FORMAT_SRGBA8,
      fetch_texel_1d_srgba8,
      fetch_texel_2d_srgba8,
      fetch_texel_3d_srgba8,
      store_texel_srgba8
   },
   {
      MESA_FORMAT_SARGB8,
      fetch_texel_1d_sargb8,
      fetch_texel_2d_sargb8,
      fetch_texel_3d_sargb8,
      store_texel_sargb8
   },
   {
      MESA_FORMAT_SL8,
      fetch_texel_1d_sl8,
      fetch_texel_2d_sl8,
      fetch_texel_3d_sl8,
      store_texel_sl8
   },
   {
      MESA_FORMAT_SLA8,
      fetch_texel_1d_sla8,
      fetch_texel_2d_sla8,
      fetch_texel_3d_sla8,
      store_texel_sla8
   },
   {
      MESA_FORMAT_RGB_FXT1,
      NULL,
      _mesa_fetch_texel_2d_f_rgb_fxt1,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RGBA_FXT1,
      NULL,
      _mesa_fetch_texel_2d_f_rgba_fxt1,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RGB_DXT1,
      NULL,
      _mesa_fetch_texel_2d_f_rgb_dxt1,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RGBA_DXT1,
      NULL,
      _mesa_fetch_texel_2d_f_rgba_dxt1,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RGBA_DXT3,
      NULL,
      _mesa_fetch_texel_2d_f_rgba_dxt3,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RGBA_DXT5,
      NULL,
      _mesa_fetch_texel_2d_f_rgba_dxt5,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_SRGB_DXT1,
      NULL,
      _mesa_fetch_texel_2d_f_srgb_dxt1,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_SRGBA_DXT1,
      NULL,
      _mesa_fetch_texel_2d_f_srgba_dxt1,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_SRGBA_DXT3,
      NULL,
      _mesa_fetch_texel_2d_f_srgba_dxt3,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_SRGBA_DXT5,
      NULL,
      _mesa_fetch_texel_2d_f_srgba_dxt5,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RGBA_FLOAT32,
      fetch_texel_1d_f_rgba_f32,
      fetch_texel_2d_f_rgba_f32,
      fetch_texel_3d_f_rgba_f32,
      store_texel_rgba_f32
   },
   {
      MESA_FORMAT_RGBA_FLOAT16,
      fetch_texel_1d_f_rgba_f16,
      fetch_texel_2d_f_rgba_f16,
      fetch_texel_3d_f_rgba_f16,
      store_texel_rgba_f16
   },
   {
      MESA_FORMAT_RGB_FLOAT32,
      fetch_texel_1d_f_rgb_f32,
      fetch_texel_2d_f_rgb_f32,
      fetch_texel_3d_f_rgb_f32,
      store_texel_rgb_f32
   },
   {
      MESA_FORMAT_RGB_FLOAT16,
      fetch_texel_1d_f_rgb_f16,
      fetch_texel_2d_f_rgb_f16,
      fetch_texel_3d_f_rgb_f16,
      store_texel_rgb_f16
   },
   {
      MESA_FORMAT_ALPHA_FLOAT32,
      fetch_texel_1d_f_alpha_f32,
      fetch_texel_2d_f_alpha_f32,
      fetch_texel_3d_f_alpha_f32,
      store_texel_alpha_f32
   },
   {
      MESA_FORMAT_ALPHA_FLOAT16,
      fetch_texel_1d_f_alpha_f16,
      fetch_texel_2d_f_alpha_f16,
      fetch_texel_3d_f_alpha_f16,
      store_texel_alpha_f16
   },
   {
      MESA_FORMAT_LUMINANCE_FLOAT32,
      fetch_texel_1d_f_luminance_f32,
      fetch_texel_2d_f_luminance_f32,
      fetch_texel_3d_f_luminance_f32,
      store_texel_luminance_f32
   },
   {
      MESA_FORMAT_LUMINANCE_FLOAT16,
      fetch_texel_1d_f_luminance_f16,
      fetch_texel_2d_f_luminance_f16,
      fetch_texel_3d_f_luminance_f16,
      store_texel_luminance_f16
   },
   {
      MESA_FORMAT_LUMINANCE_ALPHA_FLOAT32,
      fetch_texel_1d_f_luminance_alpha_f32,
      fetch_texel_2d_f_luminance_alpha_f32,
      fetch_texel_3d_f_luminance_alpha_f32,
      store_texel_luminance_alpha_f32
   },
   {
      MESA_FORMAT_LUMINANCE_ALPHA_FLOAT16,
      fetch_texel_1d_f_luminance_alpha_f16,
      fetch_texel_2d_f_luminance_alpha_f16,
      fetch_texel_3d_f_luminance_alpha_f16,
      store_texel_luminance_alpha_f16
   },
   {
      MESA_FORMAT_INTENSITY_FLOAT32,
      fetch_texel_1d_f_intensity_f32,
      fetch_texel_2d_f_intensity_f32,
      fetch_texel_3d_f_intensity_f32,
      store_texel_intensity_f32
   },
   {
      MESA_FORMAT_INTENSITY_FLOAT16,
      fetch_texel_1d_f_intensity_f16,
      fetch_texel_2d_f_intensity_f16,
      fetch_texel_3d_f_intensity_f16,
      store_texel_intensity_f16
   },
   {
      MESA_FORMAT_DUDV8,
      fetch_texel_1d_dudv8,
      fetch_texel_2d_dudv8,
      fetch_texel_3d_dudv8,
      NULL
   },
   {
      MESA_FORMAT_SIGNED_RGBA8888,
      fetch_texel_1d_signed_rgba8888,
      fetch_texel_2d_signed_rgba8888,
      fetch_texel_3d_signed_rgba8888,
      store_texel_signed_rgba8888
   },
   {
      MESA_FORMAT_SIGNED_RGBA8888_REV,
      fetch_texel_1d_signed_rgba8888_rev,
      fetch_texel_2d_signed_rgba8888_rev,
      fetch_texel_3d_signed_rgba8888_rev,
      store_texel_signed_rgba8888_rev
   },
   {
      MESA_FORMAT_SIGNED_RGBA_16,
      NULL, /* XXX to do */
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RGBA8888,
      fetch_texel_1d_f_rgba8888,
      fetch_texel_2d_f_rgba8888,
      fetch_texel_3d_f_rgba8888,
      store_texel_rgba8888
   },
   {
      MESA_FORMAT_RGBA8888_REV,
      fetch_texel_1d_f_rgba8888_rev,
      fetch_texel_2d_f_rgba8888_rev,
      fetch_texel_3d_f_rgba8888_rev,
      store_texel_rgba8888_rev
   },
   {
      MESA_FORMAT_ARGB8888,
      fetch_texel_1d_f_argb8888,
      fetch_texel_2d_f_argb8888,
      fetch_texel_3d_f_argb8888,
      store_texel_argb8888
   },
   {
      MESA_FORMAT_ARGB8888_REV,
      fetch_texel_1d_f_argb8888_rev,
      fetch_texel_2d_f_argb8888_rev,
      fetch_texel_3d_f_argb8888_rev,
      store_texel_argb8888_rev
   },
   {
      MESA_FORMAT_XRGB8888,
      fetch_texel_1d_f_xrgb8888,
      fetch_texel_2d_f_xrgb8888,
      fetch_texel_3d_f_xrgb8888,
      store_texel_xrgb8888
   },
   {
      MESA_FORMAT_XRGB8888_REV,
      fetch_texel_1d_f_xrgb8888_rev,
      fetch_texel_2d_f_xrgb8888_rev,
      fetch_texel_3d_f_xrgb8888_rev,
      store_texel_xrgb8888_rev,
   },
   {
      MESA_FORMAT_RGB888,
      fetch_texel_1d_f_rgb888,
      fetch_texel_2d_f_rgb888,
      fetch_texel_3d_f_rgb888,
      store_texel_rgb888
   },
   {
      MESA_FORMAT_BGR888,
      fetch_texel_1d_f_bgr888,
      fetch_texel_2d_f_bgr888,
      fetch_texel_3d_f_bgr888,
      store_texel_bgr888
   },
   {
      MESA_FORMAT_RGB565,
      fetch_texel_1d_f_rgb565,
      fetch_texel_2d_f_rgb565,
      fetch_texel_3d_f_rgb565,
      store_texel_rgb565
   },
   {
      MESA_FORMAT_RGB565_REV,
      fetch_texel_1d_f_rgb565_rev,
      fetch_texel_2d_f_rgb565_rev,
      fetch_texel_3d_f_rgb565_rev,
      store_texel_rgb565_rev
   },
   {
      MESA_FORMAT_ARGB4444,
      fetch_texel_1d_f_argb4444,
      fetch_texel_2d_f_argb4444,
      fetch_texel_3d_f_argb4444,
      store_texel_argb4444
   },
   {
      MESA_FORMAT_ARGB4444_REV,
      fetch_texel_1d_f_argb4444_rev,
      fetch_texel_2d_f_argb4444_rev,
      fetch_texel_3d_f_argb4444_rev,
      store_texel_argb4444_rev
   },
   {
      MESA_FORMAT_RGBA5551,
      fetch_texel_1d_f_rgba5551,
      fetch_texel_2d_f_rgba5551,
      fetch_texel_3d_f_rgba5551,
      store_texel_rgba5551
   },
   {
      MESA_FORMAT_ARGB1555,
      fetch_texel_1d_f_argb1555,
      fetch_texel_2d_f_argb1555,
      fetch_texel_3d_f_argb1555,
      store_texel_argb1555
   },
   {
      MESA_FORMAT_ARGB1555_REV,
      fetch_texel_1d_f_argb1555_rev,
      fetch_texel_2d_f_argb1555_rev,
      fetch_texel_3d_f_argb1555_rev,
      store_texel_argb1555_rev
   },
   {
      MESA_FORMAT_AL88,
      fetch_texel_1d_f_al88,
      fetch_texel_2d_f_al88,
      fetch_texel_3d_f_al88,
      store_texel_al88
   },
   {
      MESA_FORMAT_AL88_REV,
      fetch_texel_1d_f_al88_rev,
      fetch_texel_2d_f_al88_rev,
      fetch_texel_3d_f_al88_rev,
      store_texel_al88_rev
   },
   {
      MESA_FORMAT_AL1616,
      fetch_texel_1d_f_al1616,
      fetch_texel_2d_f_al1616,
      fetch_texel_3d_f_al1616,
      store_texel_al1616
   },
   {
      MESA_FORMAT_AL1616_REV,
      fetch_texel_1d_f_al1616_rev,
      fetch_texel_2d_f_al1616_rev,
      fetch_texel_3d_f_al1616_rev,
      store_texel_al1616_rev
   },
   {
      MESA_FORMAT_RGB332,
      fetch_texel_1d_f_rgb332,
      fetch_texel_2d_f_rgb332,
      fetch_texel_3d_f_rgb332,
      store_texel_rgb332
   },
   {
      MESA_FORMAT_A8,
      fetch_texel_1d_f_a8,
      fetch_texel_2d_f_a8,
      fetch_texel_3d_f_a8,
      store_texel_a8
   },
   {
      MESA_FORMAT_L8,
      fetch_texel_1d_f_l8,
      fetch_texel_2d_f_l8,
      fetch_texel_3d_f_l8,
      store_texel_l8
   },
   {
      MESA_FORMAT_I8,
      fetch_texel_1d_f_i8,
      fetch_texel_2d_f_i8,
      fetch_texel_3d_f_i8,
      store_texel_i8
   },
   {
      MESA_FORMAT_CI8,
      fetch_texel_1d_f_ci8,
      fetch_texel_2d_f_ci8,
      fetch_texel_3d_f_ci8,
      store_texel_ci8
   },
   {
      MESA_FORMAT_YCBCR,
      fetch_texel_1d_f_ycbcr,
      fetch_texel_2d_f_ycbcr,
      fetch_texel_3d_f_ycbcr,
      store_texel_ycbcr
   },
   {
      MESA_FORMAT_YCBCR_REV,
      fetch_texel_1d_f_ycbcr_rev,
      fetch_texel_2d_f_ycbcr_rev,
      fetch_texel_3d_f_ycbcr_rev,
      store_texel_ycbcr_rev
   },
   {
      MESA_FORMAT_Z24_S8,
      fetch_texel_1d_f_z24_s8,
      fetch_texel_2d_f_z24_s8,
      fetch_texel_3d_f_z24_s8,
      store_texel_z24_s8
   },
   {
      MESA_FORMAT_S8_Z24,
      fetch_texel_1d_f_s8_z24,
      fetch_texel_2d_f_s8_z24,
      fetch_texel_3d_f_s8_z24,
      store_texel_s8_z24
   },
   {
      MESA_FORMAT_Z16,
      fetch_texel_1d_f_z16,
      fetch_texel_2d_f_z16,
      fetch_texel_3d_f_z16,
      store_texel_z16
   },
   {
      MESA_FORMAT_X8_Z24,
      fetch_texel_1d_f_s8_z24,
      fetch_texel_2d_f_s8_z24,
      fetch_texel_3d_f_s8_z24,
      store_texel_s8_z24
   },
   {
      MESA_FORMAT_Z24_X8,
      fetch_texel_1d_f_z24_s8,
      fetch_texel_2d_f_z24_s8,
      fetch_texel_3d_f_z24_s8,
      store_texel_z24_s8
   },
   {
      MESA_FORMAT_Z32,
      fetch_texel_1d_f_z32,
      fetch_texel_2d_f_z32,
      fetch_texel_3d_f_z32,
      store_texel_z32
   }
};


static FetchTexelFuncF
_mesa_get_texel_fetch_func(gl_format format, GLuint dims)
{
   FetchTexelFuncF f = NULL;
   GLuint i;
   /* XXX replace loop with direct table lookup */
   for (i = 0; i < MESA_FORMAT_COUNT; i++) {
      if (texfetch_funcs[i].Name == format) {
         switch (dims) {
         case 1:
            f = texfetch_funcs[i].Fetch1D;
            break;
         case 2:
            f = texfetch_funcs[i].Fetch2D;
            break;
         case 3:
            f = texfetch_funcs[i].Fetch3D;
            break;
         }
         if (!f)
            f = fetch_null_texelf;
         return f;
      }
   }
   return NULL;
}


StoreTexelFunc
_mesa_get_texel_store_func(gl_format format)
{
   GLuint i;
   /* XXX replace loop with direct table lookup */
   for (i = 0; i < MESA_FORMAT_COUNT; i++) {
      if (texfetch_funcs[i].Name == format) {
         if (texfetch_funcs[i].StoreTexel)
            return texfetch_funcs[i].StoreTexel;
         else
            return store_null_texel;
      }
   }
   return NULL;
}



/**
 * Adaptor for fetching a GLchan texel from a float-valued texture.
 */
static void
fetch_texel_float_to_chan(const struct gl_texture_image *texImage,
                          GLint i, GLint j, GLint k, GLchan *texelOut)
{
   GLfloat temp[4];
   GLenum baseFormat = _mesa_get_format_base_format(texImage->TexFormat);

   ASSERT(texImage->FetchTexelf);
   texImage->FetchTexelf(texImage, i, j, k, temp);
   if (baseFormat == GL_DEPTH_COMPONENT ||
       baseFormat == GL_DEPTH_STENCIL_EXT) {
      /* just one channel */
      UNCLAMPED_FLOAT_TO_CHAN(texelOut[0], temp[0]);
   }
   else {
      /* four channels */
      UNCLAMPED_FLOAT_TO_CHAN(texelOut[0], temp[0]);
      UNCLAMPED_FLOAT_TO_CHAN(texelOut[1], temp[1]);
      UNCLAMPED_FLOAT_TO_CHAN(texelOut[2], temp[2]);
      UNCLAMPED_FLOAT_TO_CHAN(texelOut[3], temp[3]);
   }
}


#if 0
/**
 * Adaptor for fetching a float texel from a GLchan-valued texture.
 */
static void
fetch_texel_chan_to_float(const struct gl_texture_image *texImage,
                          GLint i, GLint j, GLint k, GLfloat *texelOut)
{
   GLchan temp[4];
   GLenum baseFormat = _mesa_get_format_base_format(texImage->TexFormat);

   ASSERT(texImage->FetchTexelc);
   texImage->FetchTexelc(texImage, i, j, k, temp);
   if (baseFormat == GL_DEPTH_COMPONENT ||
       baseFormat == GL_DEPTH_STENCIL_EXT) {
      /* just one channel */
      texelOut[0] = CHAN_TO_FLOAT(temp[0]);
   }
   else {
      /* four channels */
      texelOut[0] = CHAN_TO_FLOAT(temp[0]);
      texelOut[1] = CHAN_TO_FLOAT(temp[1]);
      texelOut[2] = CHAN_TO_FLOAT(temp[2]);
      texelOut[3] = CHAN_TO_FLOAT(temp[3]);
   }
}
#endif


/**
 * Initialize the texture image's FetchTexelc and FetchTexelf methods.
 */
void
_mesa_set_fetch_functions(struct gl_texture_image *texImage, GLuint dims)
{
   ASSERT(dims == 1 || dims == 2 || dims == 3);
   ASSERT(texImage->TexFormat);

   if (!texImage->FetchTexelf) {
      texImage->FetchTexelf =
         _mesa_get_texel_fetch_func(texImage->TexFormat, dims);
   }

   /* now check if we need to use a float/chan adaptor */
   if (!texImage->FetchTexelc) {
      texImage->FetchTexelc = fetch_texel_float_to_chan;
   }

   ASSERT(texImage->FetchTexelc);
   ASSERT(texImage->FetchTexelf);
}

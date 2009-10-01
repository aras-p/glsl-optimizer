/*
 * Mesa 3-D graphics library
 * Version:  6.5.1
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
 * Copyright (c) 2008 VMware, Inc.
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
 * \file texformat.c
 * Texture formats.
 *
 * \author Gareth Hughes
 */


#include "colormac.h"
#include "context.h"
#include "texcompress.h"
#include "texcompress_fxt1.h"
#include "texcompress_s3tc.h"
#include "texformat.h"
#include "texstore.h"


#if FEATURE_EXT_texture_sRGB

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


#endif /* FEATURE_EXT_texture_sRGB */


/* Texel fetch routines for all supported formats
 */
#define DIM 1
#include "texformat_tmp.h"

#define DIM 2
#include "texformat_tmp.h"

#define DIM 3
#include "texformat_tmp.h"

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
 * Choose an appropriate texture format given the format, type and
 * internalFormat parameters passed to glTexImage().
 *
 * \param ctx  the GL context.
 * \param internalFormat  user's prefered internal texture format.
 * \param format  incoming image pixel format.
 * \param type  incoming image data type.
 *
 * \return a pointer to a gl_texture_format object which describes the
 * choosen texture format, or NULL on failure.
 * 
 * This is called via dd_function_table::ChooseTextureFormat.  Hardware drivers
 * will typically override this function with a specialized version.
 */
gl_format
_mesa_choose_tex_format( GLcontext *ctx, GLint internalFormat,
                         GLenum format, GLenum type )
{
   (void) format;
   (void) type;

   switch (internalFormat) {
      /* RGBA formats */
      case 4:
      case GL_RGBA:
      case GL_RGB10_A2:
      case GL_RGBA12:
      case GL_RGBA16:
         return MESA_FORMAT_RGBA;
      case GL_RGBA8:
         return MESA_FORMAT_RGBA8888;
      case GL_RGB5_A1:
         return MESA_FORMAT_ARGB1555;
      case GL_RGBA2:
         return MESA_FORMAT_ARGB4444_REV; /* just to test another format*/
      case GL_RGBA4:
         return MESA_FORMAT_ARGB4444;

      /* RGB formats */
      case 3:
      case GL_RGB:
      case GL_RGB10:
      case GL_RGB12:
      case GL_RGB16:
         return MESA_FORMAT_RGB;
      case GL_RGB8:
         return MESA_FORMAT_RGB888;
      case GL_R3_G3_B2:
         return MESA_FORMAT_RGB332;
      case GL_RGB4:
         return MESA_FORMAT_RGB565_REV; /* just to test another format */
      case GL_RGB5:
         return MESA_FORMAT_RGB565;

      /* Alpha formats */
      case GL_ALPHA:
      case GL_ALPHA4:
      case GL_ALPHA12:
      case GL_ALPHA16:
         return MESA_FORMAT_ALPHA;
      case GL_ALPHA8:
         return MESA_FORMAT_A8;

      /* Luminance formats */
      case 1:
      case GL_LUMINANCE:
      case GL_LUMINANCE4:
      case GL_LUMINANCE12:
      case GL_LUMINANCE16:
         return MESA_FORMAT_LUMINANCE;
      case GL_LUMINANCE8:
         return MESA_FORMAT_L8;

      /* Luminance/Alpha formats */
      case 2:
      case GL_LUMINANCE_ALPHA:
      case GL_LUMINANCE4_ALPHA4:
      case GL_LUMINANCE6_ALPHA2:
      case GL_LUMINANCE12_ALPHA4:
      case GL_LUMINANCE12_ALPHA12:
      case GL_LUMINANCE16_ALPHA16:
         return MESA_FORMAT_LUMINANCE_ALPHA;
      case GL_LUMINANCE8_ALPHA8:
         return MESA_FORMAT_AL88;

      case GL_INTENSITY:
      case GL_INTENSITY4:
      case GL_INTENSITY12:
      case GL_INTENSITY16:
         return MESA_FORMAT_INTENSITY;
      case GL_INTENSITY8:
         return MESA_FORMAT_I8;

      case GL_COLOR_INDEX:
      case GL_COLOR_INDEX1_EXT:
      case GL_COLOR_INDEX2_EXT:
      case GL_COLOR_INDEX4_EXT:
      case GL_COLOR_INDEX12_EXT:
      case GL_COLOR_INDEX16_EXT:
      case GL_COLOR_INDEX8_EXT:
         return MESA_FORMAT_CI8;

      default:
         ; /* fallthrough */
   }

   if (ctx->Extensions.ARB_depth_texture) {
      switch (internalFormat) {
         case GL_DEPTH_COMPONENT:
         case GL_DEPTH_COMPONENT24:
         case GL_DEPTH_COMPONENT32:
            return MESA_FORMAT_Z32;
         case GL_DEPTH_COMPONENT16:
            return MESA_FORMAT_Z16;
         default:
            ; /* fallthrough */
      }
   }

   switch (internalFormat) {
      case GL_COMPRESSED_ALPHA_ARB:
         return MESA_FORMAT_ALPHA;
      case GL_COMPRESSED_LUMINANCE_ARB:
         return MESA_FORMAT_LUMINANCE;
      case GL_COMPRESSED_LUMINANCE_ALPHA_ARB:
         return MESA_FORMAT_LUMINANCE_ALPHA;
      case GL_COMPRESSED_INTENSITY_ARB:
         return MESA_FORMAT_INTENSITY;
      case GL_COMPRESSED_RGB_ARB:
#if FEATURE_texture_fxt1
         if (ctx->Extensions.TDFX_texture_compression_FXT1)
            return MESA_FORMAT_RGB_FXT1;
#endif
#if FEATURE_texture_s3tc
         if (ctx->Extensions.EXT_texture_compression_s3tc ||
             ctx->Extensions.S3_s3tc)
            return MESA_FORMAT_RGB_DXT1;
#endif
         return MESA_FORMAT_RGB;
      case GL_COMPRESSED_RGBA_ARB:
#if FEATURE_texture_fxt1
         if (ctx->Extensions.TDFX_texture_compression_FXT1)
            return MESA_FORMAT_RGBA_FXT1;
#endif
#if FEATURE_texture_s3tc
         if (ctx->Extensions.EXT_texture_compression_s3tc ||
             ctx->Extensions.S3_s3tc)
            return MESA_FORMAT_RGBA_DXT3; /* Not rgba_dxt1, see spec */
#endif
         return MESA_FORMAT_RGBA;
      default:
         ; /* fallthrough */
   }

   if (ctx->Extensions.MESA_ycbcr_texture) {
      if (internalFormat == GL_YCBCR_MESA) {
         if (type == GL_UNSIGNED_SHORT_8_8_MESA)
            return MESA_FORMAT_YCBCR;
         else
            return MESA_FORMAT_YCBCR_REV;
      }
   }

#if FEATURE_texture_fxt1
   if (ctx->Extensions.TDFX_texture_compression_FXT1) {
      switch (internalFormat) {
         case GL_COMPRESSED_RGB_FXT1_3DFX:
            return MESA_FORMAT_RGB_FXT1;
         case GL_COMPRESSED_RGBA_FXT1_3DFX:
            return MESA_FORMAT_RGBA_FXT1;
         default:
            ; /* fallthrough */
      }
   }
#endif

#if FEATURE_texture_s3tc
   if (ctx->Extensions.EXT_texture_compression_s3tc) {
      switch (internalFormat) {
         case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
            return MESA_FORMAT_RGB_DXT1;
         case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
            return MESA_FORMAT_RGBA_DXT1;
         case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
            return MESA_FORMAT_RGBA_DXT3;
         case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
            return MESA_FORMAT_RGBA_DXT5;
         default:
            ; /* fallthrough */
      }
   }

   if (ctx->Extensions.S3_s3tc) {
      switch (internalFormat) {
         case GL_RGB_S3TC:
         case GL_RGB4_S3TC:
            return MESA_FORMAT_RGB_DXT1;
         case GL_RGBA_S3TC:
         case GL_RGBA4_S3TC:
            return MESA_FORMAT_RGBA_DXT3;
         default:
            ; /* fallthrough */
      }
   }
#endif

   if (ctx->Extensions.ARB_texture_float) {
      switch (internalFormat) {
         case GL_ALPHA16F_ARB:
            return MESA_FORMAT_ALPHA_FLOAT16;
         case GL_ALPHA32F_ARB:
            return MESA_FORMAT_ALPHA_FLOAT32;
         case GL_LUMINANCE16F_ARB:
            return MESA_FORMAT_LUMINANCE_FLOAT16;
         case GL_LUMINANCE32F_ARB:
            return MESA_FORMAT_LUMINANCE_FLOAT32;
         case GL_LUMINANCE_ALPHA16F_ARB:
            return MESA_FORMAT_LUMINANCE_ALPHA_FLOAT16;
         case GL_LUMINANCE_ALPHA32F_ARB:
            return MESA_FORMAT_LUMINANCE_ALPHA_FLOAT32;
         case GL_INTENSITY16F_ARB:
            return MESA_FORMAT_INTENSITY_FLOAT16;
         case GL_INTENSITY32F_ARB:
            return MESA_FORMAT_INTENSITY_FLOAT32;
         case GL_RGB16F_ARB:
            return MESA_FORMAT_RGB_FLOAT16;
         case GL_RGB32F_ARB:
            return MESA_FORMAT_RGB_FLOAT32;
         case GL_RGBA16F_ARB:
            return MESA_FORMAT_RGBA_FLOAT16;
         case GL_RGBA32F_ARB:
            return MESA_FORMAT_RGBA_FLOAT32;
         default:
            ; /* fallthrough */
      }
   }

   if (ctx->Extensions.EXT_packed_depth_stencil) {
      switch (internalFormat) {
         case GL_DEPTH_STENCIL_EXT:
         case GL_DEPTH24_STENCIL8_EXT:
            return MESA_FORMAT_Z24_S8;
         default:
            ; /* fallthrough */
      }
   }

   if (ctx->Extensions.ATI_envmap_bumpmap) {
      switch (internalFormat) {
         case GL_DUDV_ATI:
         case GL_DU8DV8_ATI:
            return MESA_FORMAT_DUDV8;
         default:
            ; /* fallthrough */
      }
   }

   if (ctx->Extensions.MESA_texture_signed_rgba) {
      switch (internalFormat) {
         case GL_RGBA_SNORM:
         case GL_RGBA8_SNORM:
            return MESA_FORMAT_SIGNED_RGBA8888;
         default:
            ; /* fallthrough */
      }
   }


#if FEATURE_EXT_texture_sRGB
   if (ctx->Extensions.EXT_texture_sRGB) {
      switch (internalFormat) {
         case GL_SRGB_EXT:
         case GL_SRGB8_EXT:
            return MESA_FORMAT_SRGB8;
         case GL_SRGB_ALPHA_EXT:
         case GL_SRGB8_ALPHA8_EXT:
            return MESA_FORMAT_SRGBA8;
         case GL_SLUMINANCE_EXT:
         case GL_SLUMINANCE8_EXT:
            return MESA_FORMAT_SL8;
         case GL_SLUMINANCE_ALPHA_EXT:
         case GL_SLUMINANCE8_ALPHA8_EXT:
            return MESA_FORMAT_SLA8;
         case GL_COMPRESSED_SLUMINANCE_EXT:
            return MESA_FORMAT_SL8;
         case GL_COMPRESSED_SLUMINANCE_ALPHA_EXT:
            return MESA_FORMAT_SLA8;
         case GL_COMPRESSED_SRGB_EXT:
#if FEATURE_texture_s3tc
            if (ctx->Extensions.EXT_texture_compression_s3tc)
               return MESA_FORMAT_SRGB_DXT1;
#endif
            return MESA_FORMAT_SRGB8;
         case GL_COMPRESSED_SRGB_ALPHA_EXT:
#if FEATURE_texture_s3tc
            if (ctx->Extensions.EXT_texture_compression_s3tc)
               return MESA_FORMAT_SRGBA_DXT3; /* Not srgba_dxt1, see spec */
#endif
            return MESA_FORMAT_SRGBA8;
#if FEATURE_texture_s3tc
         case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
            if (ctx->Extensions.EXT_texture_compression_s3tc)
               return MESA_FORMAT_SRGB_DXT1;
            break;
         case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
            if (ctx->Extensions.EXT_texture_compression_s3tc)
               return MESA_FORMAT_SRGBA_DXT1;
            break;
         case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
            if (ctx->Extensions.EXT_texture_compression_s3tc)
               return MESA_FORMAT_SRGBA_DXT3;
            break;
         case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
            if (ctx->Extensions.EXT_texture_compression_s3tc)
               return MESA_FORMAT_SRGBA_DXT5;
            break;
#endif
         default:
            ; /* fallthrough */
      }
   }
#endif /* FEATURE_EXT_texture_sRGB */

   _mesa_problem(ctx, "unexpected format in _mesa_choose_tex_format()");
   return MESA_FORMAT_NONE;
}



/**
 * Return datatype and number of components per texel for the given gl_format.
 */
void
_mesa_format_to_type_and_comps(gl_format format,
                               GLenum *datatype, GLuint *comps)
{
   switch (format) {
   case MESA_FORMAT_RGBA8888:
   case MESA_FORMAT_RGBA8888_REV:
   case MESA_FORMAT_ARGB8888:
   case MESA_FORMAT_ARGB8888_REV:
      *datatype = CHAN_TYPE;
      *comps = 4;
      return;
   case MESA_FORMAT_RGB888:
   case MESA_FORMAT_BGR888:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 3;
      return;
   case MESA_FORMAT_RGB565:
   case MESA_FORMAT_RGB565_REV:
      *datatype = GL_UNSIGNED_SHORT_5_6_5;
      *comps = 3;
      return;

   case MESA_FORMAT_ARGB4444:
   case MESA_FORMAT_ARGB4444_REV:
      *datatype = GL_UNSIGNED_SHORT_4_4_4_4;
      *comps = 4;
      return;

   case MESA_FORMAT_ARGB1555:
   case MESA_FORMAT_ARGB1555_REV:
      *datatype = GL_UNSIGNED_SHORT_1_5_5_5_REV;
      *comps = 4;
      return;

   case MESA_FORMAT_AL88:
   case MESA_FORMAT_AL88_REV:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 2;
      return;
   case MESA_FORMAT_RGB332:
      *datatype = GL_UNSIGNED_BYTE_3_3_2;
      *comps = 3;
      return;

   case MESA_FORMAT_A8:
   case MESA_FORMAT_L8:
   case MESA_FORMAT_I8:
   case MESA_FORMAT_CI8:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 1;
      return;

   case MESA_FORMAT_YCBCR:
   case MESA_FORMAT_YCBCR_REV:
      *datatype = GL_UNSIGNED_SHORT;
      *comps = 2;
      return;

   case MESA_FORMAT_Z24_S8:
      *datatype = GL_UNSIGNED_INT;
      *comps = 1; /* XXX OK? */
      return;

   case MESA_FORMAT_S8_Z24:
      *datatype = GL_UNSIGNED_INT;
      *comps = 1; /* XXX OK? */
      return;

   case MESA_FORMAT_Z16:
      *datatype = GL_UNSIGNED_SHORT;
      *comps = 1;
      return;

   case MESA_FORMAT_Z32:
      *datatype = GL_UNSIGNED_INT;
      *comps = 1;
      return;

   case MESA_FORMAT_DUDV8:
      *datatype = GL_BYTE;
      *comps = 2;
      return;

   case MESA_FORMAT_SIGNED_RGBA8888:
   case MESA_FORMAT_SIGNED_RGBA8888_REV:
      *datatype = GL_BYTE;
      *comps = 4;
      return;

#if FEATURE_EXT_texture_sRGB
   case MESA_FORMAT_SRGB8:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 3;
      return;
   case MESA_FORMAT_SRGBA8:
   case MESA_FORMAT_SARGB8:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 4;
      return;
   case MESA_FORMAT_SL8:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 1;
      return;
   case MESA_FORMAT_SLA8:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 2;
      return;
#endif

#if FEATURE_texture_fxt1
   case MESA_FORMAT_RGB_FXT1:
   case MESA_FORMAT_RGBA_FXT1:
#endif
#if FEATURE_texture_s3tc
   case MESA_FORMAT_RGB_DXT1:
   case MESA_FORMAT_RGBA_DXT1:
   case MESA_FORMAT_RGBA_DXT3:
   case MESA_FORMAT_RGBA_DXT5:
#if FEATURE_EXT_texture_sRGB
   case MESA_FORMAT_SRGB_DXT1:
   case MESA_FORMAT_SRGBA_DXT1:
   case MESA_FORMAT_SRGBA_DXT3:
   case MESA_FORMAT_SRGBA_DXT5:
#endif
      /* XXX generate error instead? */
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 0;
      return;
#endif

   case MESA_FORMAT_RGBA:
      *datatype = CHAN_TYPE;
      *comps = 4;
      return;
   case MESA_FORMAT_RGB:
      *datatype = CHAN_TYPE;
      *comps = 3;
      return;
   case MESA_FORMAT_LUMINANCE_ALPHA:
      *datatype = CHAN_TYPE;
      *comps = 2;
      return;
   case MESA_FORMAT_ALPHA:
   case MESA_FORMAT_LUMINANCE:
   case MESA_FORMAT_INTENSITY:
      *datatype = CHAN_TYPE;
      *comps = 1;
      return;

   case MESA_FORMAT_RGBA_FLOAT32:
      *datatype = GL_FLOAT;
      *comps = 4;
      return;
   case MESA_FORMAT_RGBA_FLOAT16:
      *datatype = GL_HALF_FLOAT_ARB;
      *comps = 4;
      return;
   case MESA_FORMAT_RGB_FLOAT32:
      *datatype = GL_FLOAT;
      *comps = 3;
      return;
   case MESA_FORMAT_RGB_FLOAT16:
      *datatype = GL_HALF_FLOAT_ARB;
      *comps = 3;
      return;
   case MESA_FORMAT_LUMINANCE_ALPHA_FLOAT32:
      *datatype = GL_FLOAT;
      *comps = 2;
      return;
   case MESA_FORMAT_LUMINANCE_ALPHA_FLOAT16:
      *datatype = GL_HALF_FLOAT_ARB;
      *comps = 2;
      return;
   case MESA_FORMAT_ALPHA_FLOAT32:
   case MESA_FORMAT_LUMINANCE_FLOAT32:
   case MESA_FORMAT_INTENSITY_FLOAT32:
      *datatype = GL_FLOAT;
      *comps = 1;
      return;
   case MESA_FORMAT_ALPHA_FLOAT16:
   case MESA_FORMAT_LUMINANCE_FLOAT16:
   case MESA_FORMAT_INTENSITY_FLOAT16:
      *datatype = GL_HALF_FLOAT_ARB;
      *comps = 1;
      return;

   default:
      _mesa_problem(NULL, "bad format in _mesa_format_to_type_and_comps");
      *datatype = 0;
      *comps = 1;
   }
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
      MESA_FORMAT_RGBA,
      fetch_texel_1d_f_rgba,
      fetch_texel_2d_f_rgba,
      fetch_texel_3d_f_rgba,
      store_texel_rgba
   },
   {
      MESA_FORMAT_RGB,
      fetch_texel_1d_f_rgb,
      fetch_texel_2d_f_rgb,
      fetch_texel_3d_f_rgb,
      store_texel_rgb
   },
   {
      MESA_FORMAT_ALPHA,
      fetch_texel_1d_f_alpha,
      fetch_texel_2d_f_alpha,
      fetch_texel_3d_f_alpha,
      store_texel_alpha
   },
   {
      MESA_FORMAT_LUMINANCE,
      fetch_texel_1d_f_luminance,
      fetch_texel_2d_f_luminance,
      fetch_texel_3d_f_luminance,
      store_texel_luminance
   },
   {
      MESA_FORMAT_LUMINANCE_ALPHA,
      fetch_texel_1d_f_luminance_alpha,
      fetch_texel_2d_f_luminance_alpha,
      fetch_texel_3d_f_luminance_alpha,
      store_texel_luminance_alpha
   },
   {
      MESA_FORMAT_INTENSITY,
      fetch_texel_1d_f_intensity,
      fetch_texel_2d_f_intensity,
      fetch_texel_3d_f_intensity,
      store_texel_intensity
   },
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
      MESA_FORMAT_RGBA4444,
      fetch_texel_1d_f_rgba4444,
      fetch_texel_2d_f_rgba4444,
      fetch_texel_3d_f_rgba4444,
      store_texel_rgba4444
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
      MESA_FORMAT_Z32,
      fetch_texel_1d_f_z32,
      fetch_texel_2d_f_z32,
      fetch_texel_3d_f_z32,
      store_texel_z32
   }
};


FetchTexelFuncF
_mesa_get_texel_fetch_func(GLuint format, GLuint dims)
{
   FetchTexelFuncF f;
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
_mesa_get_texel_store_func(GLuint format)
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

/*
 * Mesa 3-D graphics library
 * Version:  7.7
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (c) 2008-2009 VMware, Inc.
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
 * \author Brian Paul
 */


#include "context.h"
#include "texcompress.h"
#include "texformat.h"


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
      case GL_ALPHA8:
         return MESA_FORMAT_A8;

      /* Luminance formats */
      case 1:
      case GL_LUMINANCE:
      case GL_LUMINANCE4:
      case GL_LUMINANCE12:
      case GL_LUMINANCE16:
      case GL_LUMINANCE8:
         return MESA_FORMAT_L8;

      /* Luminance/Alpha formats */
      case 2:
      case GL_LUMINANCE_ALPHA:
      case GL_LUMINANCE4_ALPHA4:
      case GL_LUMINANCE6_ALPHA2:
      case GL_LUMINANCE8_ALPHA8:
         return MESA_FORMAT_AL88;

      case GL_LUMINANCE12_ALPHA4:
      case GL_LUMINANCE12_ALPHA12:
      case GL_LUMINANCE16_ALPHA16:
         return MESA_FORMAT_AL1616;

      case GL_INTENSITY:
      case GL_INTENSITY4:
      case GL_INTENSITY12:
      case GL_INTENSITY16:
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
         return MESA_FORMAT_A8;
      case GL_COMPRESSED_LUMINANCE_ARB:
         return MESA_FORMAT_L8;
      case GL_COMPRESSED_LUMINANCE_ALPHA_ARB:
         return MESA_FORMAT_AL88;
      case GL_COMPRESSED_INTENSITY_ARB:
         return MESA_FORMAT_I8;
      case GL_COMPRESSED_RGB_ARB:
         if (ctx->Extensions.EXT_texture_compression_s3tc ||
             ctx->Extensions.S3_s3tc)
            return MESA_FORMAT_RGB_DXT1;
         if (ctx->Extensions.TDFX_texture_compression_FXT1)
            return MESA_FORMAT_RGB_FXT1;
         return MESA_FORMAT_RGB888;
      case GL_COMPRESSED_RGBA_ARB:
         if (ctx->Extensions.EXT_texture_compression_s3tc ||
             ctx->Extensions.S3_s3tc)
            return MESA_FORMAT_RGBA_DXT3; /* Not rgba_dxt1, see spec */
         if (ctx->Extensions.TDFX_texture_compression_FXT1)
            return MESA_FORMAT_RGBA_FXT1;
         return MESA_FORMAT_RGBA8888;
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


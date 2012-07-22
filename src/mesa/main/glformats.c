/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (c) 2008-2009  VMware, Inc.
 * Copyright (c) 2012 Intel Corporation
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
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include "context.h"
#include "glformats.h"


/**
 * Test if the given format is an integer (non-normalized) format.
 */
GLboolean
_mesa_is_enum_format_integer(GLenum format)
{
   switch (format) {
   /* generic integer formats */
   case GL_RED_INTEGER_EXT:
   case GL_GREEN_INTEGER_EXT:
   case GL_BLUE_INTEGER_EXT:
   case GL_ALPHA_INTEGER_EXT:
   case GL_RGB_INTEGER_EXT:
   case GL_RGBA_INTEGER_EXT:
   case GL_BGR_INTEGER_EXT:
   case GL_BGRA_INTEGER_EXT:
   case GL_LUMINANCE_INTEGER_EXT:
   case GL_LUMINANCE_ALPHA_INTEGER_EXT:
   case GL_RG_INTEGER:
   /* specific integer formats */
   case GL_RGBA32UI_EXT:
   case GL_RGB32UI_EXT:
   case GL_RG32UI:
   case GL_R32UI:
   case GL_ALPHA32UI_EXT:
   case GL_INTENSITY32UI_EXT:
   case GL_LUMINANCE32UI_EXT:
   case GL_LUMINANCE_ALPHA32UI_EXT:
   case GL_RGBA16UI_EXT:
   case GL_RGB16UI_EXT:
   case GL_RG16UI:
   case GL_R16UI:
   case GL_ALPHA16UI_EXT:
   case GL_INTENSITY16UI_EXT:
   case GL_LUMINANCE16UI_EXT:
   case GL_LUMINANCE_ALPHA16UI_EXT:
   case GL_RGBA8UI_EXT:
   case GL_RGB8UI_EXT:
   case GL_RG8UI:
   case GL_R8UI:
   case GL_ALPHA8UI_EXT:
   case GL_INTENSITY8UI_EXT:
   case GL_LUMINANCE8UI_EXT:
   case GL_LUMINANCE_ALPHA8UI_EXT:
   case GL_RGBA32I_EXT:
   case GL_RGB32I_EXT:
   case GL_RG32I:
   case GL_R32I:
   case GL_ALPHA32I_EXT:
   case GL_INTENSITY32I_EXT:
   case GL_LUMINANCE32I_EXT:
   case GL_LUMINANCE_ALPHA32I_EXT:
   case GL_RGBA16I_EXT:
   case GL_RGB16I_EXT:
   case GL_RG16I:
   case GL_R16I:
   case GL_ALPHA16I_EXT:
   case GL_INTENSITY16I_EXT:
   case GL_LUMINANCE16I_EXT:
   case GL_LUMINANCE_ALPHA16I_EXT:
   case GL_RGBA8I_EXT:
   case GL_RGB8I_EXT:
   case GL_RG8I:
   case GL_R8I:
   case GL_ALPHA8I_EXT:
   case GL_INTENSITY8I_EXT:
   case GL_LUMINANCE8I_EXT:
   case GL_LUMINANCE_ALPHA8I_EXT:
   case GL_RGB10_A2UI:
      return GL_TRUE;
   default:
      return GL_FALSE;
   }
}


/**
 * Test if the given type is an integer (non-normalized) format.
 */
GLboolean
_mesa_is_type_integer(GLenum type)
{
   switch (type) {
   case GL_INT:
   case GL_UNSIGNED_INT:
   case GL_SHORT:
   case GL_UNSIGNED_SHORT:
   case GL_BYTE:
   case GL_UNSIGNED_BYTE:
      return GL_TRUE;
   default:
      return GL_FALSE;
   }
}


/**
 * Test if the given format or type is an integer (non-normalized) format.
 */
extern GLboolean
_mesa_is_enum_format_or_type_integer(GLenum format, GLenum type)
{
   return _mesa_is_enum_format_integer(format) || _mesa_is_type_integer(type);
}


GLboolean
_mesa_is_type_unsigned(GLenum type)
{
   switch (type) {
   case GL_UNSIGNED_INT:
   case GL_UNSIGNED_INT_8_8_8_8:
   case GL_UNSIGNED_INT_8_8_8_8_REV:
   case GL_UNSIGNED_INT_10_10_10_2:
   case GL_UNSIGNED_INT_2_10_10_10_REV:

   case GL_UNSIGNED_SHORT:
   case GL_UNSIGNED_SHORT_4_4_4_4:
   case GL_UNSIGNED_SHORT_5_5_5_1:
   case GL_UNSIGNED_SHORT_5_6_5:
   case GL_UNSIGNED_SHORT_5_6_5_REV:
   case GL_UNSIGNED_SHORT_4_4_4_4_REV:
   case GL_UNSIGNED_SHORT_1_5_5_5_REV:
   case GL_UNSIGNED_SHORT_8_8_MESA:
   case GL_UNSIGNED_SHORT_8_8_REV_MESA:

   case GL_UNSIGNED_BYTE:
   case GL_UNSIGNED_BYTE_3_3_2:
   case GL_UNSIGNED_BYTE_2_3_3_REV:
      return GL_TRUE;

   default:
      return GL_FALSE;
   }
}


/**
 * Test if the given image format is a color/RGBA format (i.e., not color
 * index, depth, stencil, etc).
 * \param format  the image format value (may by an internal texture format)
 * \return GL_TRUE if its a color/RGBA format, GL_FALSE otherwise.
 */
GLboolean
_mesa_is_color_format(GLenum format)
{
   switch (format) {
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
      case GL_ALPHA4:
      case GL_ALPHA8:
      case GL_ALPHA12:
      case GL_ALPHA16:
      case 1:
      case GL_LUMINANCE:
      case GL_LUMINANCE4:
      case GL_LUMINANCE8:
      case GL_LUMINANCE12:
      case GL_LUMINANCE16:
      case 2:
      case GL_LUMINANCE_ALPHA:
      case GL_LUMINANCE4_ALPHA4:
      case GL_LUMINANCE6_ALPHA2:
      case GL_LUMINANCE8_ALPHA8:
      case GL_LUMINANCE12_ALPHA4:
      case GL_LUMINANCE12_ALPHA12:
      case GL_LUMINANCE16_ALPHA16:
      case GL_INTENSITY:
      case GL_INTENSITY4:
      case GL_INTENSITY8:
      case GL_INTENSITY12:
      case GL_INTENSITY16:
      case GL_R8:
      case GL_R16:
      case GL_RG:
      case GL_RG8:
      case GL_RG16:
      case 3:
      case GL_RGB:
      case GL_BGR:
      case GL_R3_G3_B2:
      case GL_RGB4:
      case GL_RGB5:
      case GL_RGB565:
      case GL_RGB8:
      case GL_RGB10:
      case GL_RGB12:
      case GL_RGB16:
      case 4:
      case GL_ABGR_EXT:
      case GL_RGBA:
      case GL_BGRA:
      case GL_RGBA2:
      case GL_RGBA4:
      case GL_RGB5_A1:
      case GL_RGBA8:
      case GL_RGB10_A2:
      case GL_RGBA12:
      case GL_RGBA16:
      /* float texture formats */
      case GL_ALPHA16F_ARB:
      case GL_ALPHA32F_ARB:
      case GL_LUMINANCE16F_ARB:
      case GL_LUMINANCE32F_ARB:
      case GL_LUMINANCE_ALPHA16F_ARB:
      case GL_LUMINANCE_ALPHA32F_ARB:
      case GL_INTENSITY16F_ARB:
      case GL_INTENSITY32F_ARB:
      case GL_R16F:
      case GL_R32F:
      case GL_RG16F:
      case GL_RG32F:
      case GL_RGB16F_ARB:
      case GL_RGB32F_ARB:
      case GL_RGBA16F_ARB:
      case GL_RGBA32F_ARB:
      /* compressed formats */
      case GL_COMPRESSED_ALPHA:
      case GL_COMPRESSED_LUMINANCE:
      case GL_COMPRESSED_LUMINANCE_ALPHA:
      case GL_COMPRESSED_INTENSITY:
      case GL_COMPRESSED_RED:
      case GL_COMPRESSED_RG:
      case GL_COMPRESSED_RGB:
      case GL_COMPRESSED_RGBA:
      case GL_RGB_S3TC:
      case GL_RGB4_S3TC:
      case GL_RGBA_S3TC:
      case GL_RGBA4_S3TC:
      case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
      case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
      case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
      case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
      case GL_COMPRESSED_RGB_FXT1_3DFX:
      case GL_COMPRESSED_RGBA_FXT1_3DFX:
#if FEATURE_EXT_texture_sRGB
      case GL_SRGB_EXT:
      case GL_SRGB8_EXT:
      case GL_SRGB_ALPHA_EXT:
      case GL_SRGB8_ALPHA8_EXT:
      case GL_SLUMINANCE_ALPHA_EXT:
      case GL_SLUMINANCE8_ALPHA8_EXT:
      case GL_SLUMINANCE_EXT:
      case GL_SLUMINANCE8_EXT:
      case GL_COMPRESSED_SRGB_EXT:
      case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
      case GL_COMPRESSED_SRGB_ALPHA_EXT:
      case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
      case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
      case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
      case GL_COMPRESSED_SLUMINANCE_EXT:
      case GL_COMPRESSED_SLUMINANCE_ALPHA_EXT:
#endif /* FEATURE_EXT_texture_sRGB */
      case GL_COMPRESSED_RED_RGTC1:
      case GL_COMPRESSED_SIGNED_RED_RGTC1:
      case GL_COMPRESSED_RG_RGTC2:
      case GL_COMPRESSED_SIGNED_RG_RGTC2:
      case GL_COMPRESSED_LUMINANCE_LATC1_EXT:
      case GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT:
      case GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT:
      case GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT:
      case GL_COMPRESSED_LUMINANCE_ALPHA_3DC_ATI:
      case GL_ETC1_RGB8_OES:
      /* generic integer formats */
      case GL_RED_INTEGER_EXT:
      case GL_GREEN_INTEGER_EXT:
      case GL_BLUE_INTEGER_EXT:
      case GL_ALPHA_INTEGER_EXT:
      case GL_RGB_INTEGER_EXT:
      case GL_RGBA_INTEGER_EXT:
      case GL_BGR_INTEGER_EXT:
      case GL_BGRA_INTEGER_EXT:
      case GL_RG_INTEGER:
      case GL_LUMINANCE_INTEGER_EXT:
      case GL_LUMINANCE_ALPHA_INTEGER_EXT:
      /* sized integer formats */
      case GL_RGBA32UI_EXT:
      case GL_RGB32UI_EXT:
      case GL_RG32UI:
      case GL_R32UI:
      case GL_ALPHA32UI_EXT:
      case GL_INTENSITY32UI_EXT:
      case GL_LUMINANCE32UI_EXT:
      case GL_LUMINANCE_ALPHA32UI_EXT:
      case GL_RGBA16UI_EXT:
      case GL_RGB16UI_EXT:
      case GL_RG16UI:
      case GL_R16UI:
      case GL_ALPHA16UI_EXT:
      case GL_INTENSITY16UI_EXT:
      case GL_LUMINANCE16UI_EXT:
      case GL_LUMINANCE_ALPHA16UI_EXT:
      case GL_RGBA8UI_EXT:
      case GL_RGB8UI_EXT:
      case GL_RG8UI:
      case GL_R8UI:
      case GL_ALPHA8UI_EXT:
      case GL_INTENSITY8UI_EXT:
      case GL_LUMINANCE8UI_EXT:
      case GL_LUMINANCE_ALPHA8UI_EXT:
      case GL_RGBA32I_EXT:
      case GL_RGB32I_EXT:
      case GL_RG32I:
      case GL_R32I:
      case GL_ALPHA32I_EXT:
      case GL_INTENSITY32I_EXT:
      case GL_LUMINANCE32I_EXT:
      case GL_LUMINANCE_ALPHA32I_EXT:
      case GL_RGBA16I_EXT:
      case GL_RGB16I_EXT:
      case GL_RG16I:
      case GL_R16I:
      case GL_ALPHA16I_EXT:
      case GL_INTENSITY16I_EXT:
      case GL_LUMINANCE16I_EXT:
      case GL_LUMINANCE_ALPHA16I_EXT:
      case GL_RGBA8I_EXT:
      case GL_RGB8I_EXT:
      case GL_RG8I:
      case GL_R8I:
      case GL_ALPHA8I_EXT:
      case GL_INTENSITY8I_EXT:
      case GL_LUMINANCE8I_EXT:
      case GL_LUMINANCE_ALPHA8I_EXT:
      /* signed, normalized texture formats */
      case GL_RED_SNORM:
      case GL_R8_SNORM:
      case GL_R16_SNORM:
      case GL_RG_SNORM:
      case GL_RG8_SNORM:
      case GL_RG16_SNORM:
      case GL_RGB_SNORM:
      case GL_RGB8_SNORM:
      case GL_RGB16_SNORM:
      case GL_RGBA_SNORM:
      case GL_RGBA8_SNORM:
      case GL_RGBA16_SNORM:
      case GL_ALPHA_SNORM:
      case GL_ALPHA8_SNORM:
      case GL_ALPHA16_SNORM:
      case GL_LUMINANCE_SNORM:
      case GL_LUMINANCE8_SNORM:
      case GL_LUMINANCE16_SNORM:
      case GL_LUMINANCE_ALPHA_SNORM:
      case GL_LUMINANCE8_ALPHA8_SNORM:
      case GL_LUMINANCE16_ALPHA16_SNORM:
      case GL_INTENSITY_SNORM:
      case GL_INTENSITY8_SNORM:
      case GL_INTENSITY16_SNORM:
      case GL_RGB9_E5:
      case GL_R11F_G11F_B10F:
      case GL_RGB10_A2UI:
         return GL_TRUE;
      case GL_YCBCR_MESA:  /* not considered to be RGB */
         /* fall-through */
      default:
         return GL_FALSE;
   }
}


/**
 * Test if the given image format is a depth component format.
 */
GLboolean
_mesa_is_depth_format(GLenum format)
{
   switch (format) {
      case GL_DEPTH_COMPONENT:
      case GL_DEPTH_COMPONENT16:
      case GL_DEPTH_COMPONENT24:
      case GL_DEPTH_COMPONENT32:
      case GL_DEPTH_COMPONENT32F:
         return GL_TRUE;
      default:
         return GL_FALSE;
   }
}


/**
 * Test if the given image format is a stencil format.
 */
GLboolean
_mesa_is_stencil_format(GLenum format)
{
   switch (format) {
      case GL_STENCIL_INDEX:
         return GL_TRUE;
      default:
         return GL_FALSE;
   }
}


/**
 * Test if the given image format is a YCbCr format.
 */
GLboolean
_mesa_is_ycbcr_format(GLenum format)
{
   switch (format) {
      case GL_YCBCR_MESA:
         return GL_TRUE;
      default:
         return GL_FALSE;
   }
}


/**
 * Test if the given image format is a depth+stencil format.
 */
GLboolean
_mesa_is_depthstencil_format(GLenum format)
{
   switch (format) {
      case GL_DEPTH24_STENCIL8_EXT:
      case GL_DEPTH_STENCIL_EXT:
      case GL_DEPTH32F_STENCIL8:
         return GL_TRUE;
      default:
         return GL_FALSE;
   }
}


/**
 * Test if the given image format is a depth or stencil format.
 */
GLboolean
_mesa_is_depth_or_stencil_format(GLenum format)
{
   switch (format) {
      case GL_DEPTH_COMPONENT:
      case GL_DEPTH_COMPONENT16:
      case GL_DEPTH_COMPONENT24:
      case GL_DEPTH_COMPONENT32:
      case GL_STENCIL_INDEX:
      case GL_STENCIL_INDEX1_EXT:
      case GL_STENCIL_INDEX4_EXT:
      case GL_STENCIL_INDEX8_EXT:
      case GL_STENCIL_INDEX16_EXT:
      case GL_DEPTH_STENCIL_EXT:
      case GL_DEPTH24_STENCIL8_EXT:
      case GL_DEPTH_COMPONENT32F:
      case GL_DEPTH32F_STENCIL8:
         return GL_TRUE;
      default:
         return GL_FALSE;
   }
}


/**
 * Test if the given image format is a dudv format.
 */
GLboolean
_mesa_is_dudv_format(GLenum format)
{
   switch (format) {
      case GL_DUDV_ATI:
      case GL_DU8DV8_ATI:
         return GL_TRUE;
      default:
         return GL_FALSE;
   }
}


/**
 * Test if an image format is a supported compressed format.
 * \param format the internal format token provided by the user.
 * \return GL_TRUE if compressed, GL_FALSE if uncompressed
 */
GLboolean
_mesa_is_compressed_format(struct gl_context *ctx, GLenum format)
{
   switch (format) {
   case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
   case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
   case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
   case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
      return ctx->Extensions.EXT_texture_compression_s3tc;
   case GL_RGB_S3TC:
   case GL_RGB4_S3TC:
   case GL_RGBA_S3TC:
   case GL_RGBA4_S3TC:
      return ctx->Extensions.S3_s3tc;
   case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
   case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
   case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
   case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
      return ctx->Extensions.EXT_texture_sRGB
         && ctx->Extensions.EXT_texture_compression_s3tc;
   case GL_COMPRESSED_RGB_FXT1_3DFX:
   case GL_COMPRESSED_RGBA_FXT1_3DFX:
      return ctx->Extensions.TDFX_texture_compression_FXT1;
   case GL_COMPRESSED_RED_RGTC1:
   case GL_COMPRESSED_SIGNED_RED_RGTC1:
   case GL_COMPRESSED_RG_RGTC2:
   case GL_COMPRESSED_SIGNED_RG_RGTC2:
      return ctx->Extensions.ARB_texture_compression_rgtc;
   case GL_COMPRESSED_LUMINANCE_LATC1_EXT:
   case GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT:
   case GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT:
   case GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT:
      return ctx->Extensions.EXT_texture_compression_latc;
   case GL_COMPRESSED_LUMINANCE_ALPHA_3DC_ATI:
      return ctx->Extensions.ATI_texture_compression_3dc;
   case GL_ETC1_RGB8_OES:
      return ctx->Extensions.OES_compressed_ETC1_RGB8_texture;
#if FEATURE_ES
   case GL_PALETTE4_RGB8_OES:
   case GL_PALETTE4_RGBA8_OES:
   case GL_PALETTE4_R5_G6_B5_OES:
   case GL_PALETTE4_RGBA4_OES:
   case GL_PALETTE4_RGB5_A1_OES:
   case GL_PALETTE8_RGB8_OES:
   case GL_PALETTE8_RGBA8_OES:
   case GL_PALETTE8_R5_G6_B5_OES:
   case GL_PALETTE8_RGBA4_OES:
   case GL_PALETTE8_RGB5_A1_OES:
      return ctx->API == API_OPENGLES;
#endif
   default:
      return GL_FALSE;
   }
}


/**
 * Convert various base formats to the cooresponding integer format.
 */
GLenum
_mesa_base_format_to_integer_format(GLenum format)
{
   switch(format) {
   case GL_RED:
      return GL_RED_INTEGER;
   case GL_GREEN:
      return GL_GREEN_INTEGER;
   case GL_BLUE:
      return GL_BLUE_INTEGER;
   case GL_RG:
      return GL_RG_INTEGER;
   case GL_RGB:
      return GL_RGB_INTEGER;
   case GL_RGBA:
      return GL_RGBA_INTEGER;
   case GL_BGR:
      return GL_BGR_INTEGER;
   case GL_BGRA:
      return GL_BGRA_INTEGER;
   case GL_ALPHA:
      return GL_ALPHA_INTEGER;
   case GL_LUMINANCE:
      return GL_LUMINANCE_INTEGER_EXT;
   case GL_LUMINANCE_ALPHA:
      return GL_LUMINANCE_ALPHA_INTEGER_EXT;
   }

   return format;
}


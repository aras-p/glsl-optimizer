/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * Copyright (c) 2008-2010 VMware, Inc.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/


/**
 * Mesa / Gallium format conversion and format selection code.
 * \author Brian Paul
 */

#include "main/imports.h"
#include "main/context.h"
#include "main/texstore.h"
#include "main/enums.h"
#include "main/macros.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_screen.h"
#include "util/u_format.h"
#include "st_context.h"
#include "st_format.h"


static GLuint
format_max_bits(enum pipe_format format)
{
   GLuint size = util_format_get_component_bits(format, UTIL_FORMAT_COLORSPACE_RGB, 0);

   size = MAX2(size, util_format_get_component_bits(format, UTIL_FORMAT_COLORSPACE_RGB, 1));
   size = MAX2(size, util_format_get_component_bits(format, UTIL_FORMAT_COLORSPACE_RGB, 2));
   size = MAX2(size, util_format_get_component_bits(format, UTIL_FORMAT_COLORSPACE_RGB, 3));
   size = MAX2(size, util_format_get_component_bits(format, UTIL_FORMAT_COLORSPACE_ZS, 0));
   size = MAX2(size, util_format_get_component_bits(format, UTIL_FORMAT_COLORSPACE_ZS, 1));
   return size;
}


/**
 * Return basic GL datatype for the given gallium format.
 */
GLenum
st_format_datatype(enum pipe_format format)
{
   const struct util_format_description *desc;

   desc = util_format_description(format);
   assert(desc);

   if (desc->layout == UTIL_FORMAT_LAYOUT_PLAIN) {
      if (format == PIPE_FORMAT_B5G5R5A1_UNORM ||
          format == PIPE_FORMAT_B5G6R5_UNORM) {
         return GL_UNSIGNED_SHORT;
      }
      else if (format == PIPE_FORMAT_Z24S8_UNORM ||
               format == PIPE_FORMAT_S8Z24_UNORM) {
         return GL_UNSIGNED_INT_24_8;
      }
      else {
         const GLuint size = format_max_bits(format);
         if (size == 8) {
            if (desc->channel[0].type == UTIL_FORMAT_TYPE_UNSIGNED)
               return GL_UNSIGNED_BYTE;
            else
               return GL_BYTE;
         }
         else if (size == 16) {
            if (desc->channel[0].type == UTIL_FORMAT_TYPE_UNSIGNED)
               return GL_UNSIGNED_SHORT;
            else
               return GL_SHORT;
         }
         else {
            assert( size <= 32 );
            if (desc->channel[0].type == UTIL_FORMAT_TYPE_UNSIGNED)
               return GL_UNSIGNED_INT;
            else
               return GL_INT;
         }
      }
   }
   else if (format == PIPE_FORMAT_UYVY) {
      return GL_UNSIGNED_SHORT;
   }
   else if (format == PIPE_FORMAT_YUYV) {
      return GL_UNSIGNED_SHORT;
   }
   else {
      /* compressed format? */
      assert(0);
   }

   assert(0);
   return GL_NONE;
}


/**
 * Translate Mesa format to Gallium format.
 */
enum pipe_format
st_mesa_format_to_pipe_format(gl_format mesaFormat)
{
   switch (mesaFormat) {
      /* fix this */
   case MESA_FORMAT_ARGB8888_REV:
   case MESA_FORMAT_ARGB8888:
      return PIPE_FORMAT_B8G8R8A8_UNORM;
   case MESA_FORMAT_XRGB8888:
      return PIPE_FORMAT_B8G8R8X8_UNORM;
   case MESA_FORMAT_ARGB1555:
      return PIPE_FORMAT_B5G5R5A1_UNORM;
   case MESA_FORMAT_ARGB4444:
      return PIPE_FORMAT_B4G4R4A4_UNORM;
   case MESA_FORMAT_RGB565:
      return PIPE_FORMAT_B5G6R5_UNORM;
   case MESA_FORMAT_AL88:
      return PIPE_FORMAT_L8A8_UNORM;
   case MESA_FORMAT_A8:
      return PIPE_FORMAT_A8_UNORM;
   case MESA_FORMAT_L8:
      return PIPE_FORMAT_L8_UNORM;
   case MESA_FORMAT_I8:
      return PIPE_FORMAT_I8_UNORM;
   case MESA_FORMAT_Z16:
      return PIPE_FORMAT_Z16_UNORM;
   case MESA_FORMAT_Z32:
      return PIPE_FORMAT_Z32_UNORM;
   case MESA_FORMAT_Z24_S8:
      return PIPE_FORMAT_S8Z24_UNORM;
   case MESA_FORMAT_S8_Z24:
      return PIPE_FORMAT_Z24S8_UNORM;
   case MESA_FORMAT_Z24_X8:
      return PIPE_FORMAT_X8Z24_UNORM;
   case MESA_FORMAT_X8_Z24:
      return PIPE_FORMAT_Z24X8_UNORM;
   case MESA_FORMAT_YCBCR:
      return PIPE_FORMAT_UYVY;
#if FEATURE_texture_s3tc
   case MESA_FORMAT_RGB_DXT1:
      return PIPE_FORMAT_DXT1_RGB;
   case MESA_FORMAT_RGBA_DXT1:
      return PIPE_FORMAT_DXT1_RGBA;
   case MESA_FORMAT_RGBA_DXT3:
      return PIPE_FORMAT_DXT3_RGBA;
   case MESA_FORMAT_RGBA_DXT5:
      return PIPE_FORMAT_DXT5_RGBA;
#if FEATURE_EXT_texture_sRGB
   case MESA_FORMAT_SRGB_DXT1:
      return PIPE_FORMAT_DXT1_SRGB;
   case MESA_FORMAT_SRGBA_DXT1:
      return PIPE_FORMAT_DXT1_SRGBA;
   case MESA_FORMAT_SRGBA_DXT3:
      return PIPE_FORMAT_DXT3_SRGBA;
   case MESA_FORMAT_SRGBA_DXT5:
      return PIPE_FORMAT_DXT5_SRGBA;
#endif
#endif
#if FEATURE_EXT_texture_sRGB
   case MESA_FORMAT_SLA8:
      return PIPE_FORMAT_L8A8_SRGB;
   case MESA_FORMAT_SL8:
      return PIPE_FORMAT_L8_SRGB;
   case MESA_FORMAT_SRGB8:
      return PIPE_FORMAT_R8G8B8_SRGB;
   case MESA_FORMAT_SRGBA8:
      return PIPE_FORMAT_A8B8G8R8_SRGB;
   case MESA_FORMAT_SARGB8:
      return PIPE_FORMAT_B8G8R8A8_SRGB;
#endif
   default:
      assert(0);
      return 0;
   }
}


/**
 * Translate Gallium format to Mesa format.
 */
gl_format
st_pipe_format_to_mesa_format(enum pipe_format format)
{
   switch (format) {
   case PIPE_FORMAT_B8G8R8A8_UNORM:
      return MESA_FORMAT_ARGB8888;
   case PIPE_FORMAT_B8G8R8X8_UNORM:
      return MESA_FORMAT_XRGB8888;
   case PIPE_FORMAT_A8R8G8B8_UNORM:
      return MESA_FORMAT_ARGB8888_REV;
   case PIPE_FORMAT_X8R8G8B8_UNORM:
      return MESA_FORMAT_XRGB8888_REV;
   case PIPE_FORMAT_B5G5R5A1_UNORM:
      return MESA_FORMAT_ARGB1555;
   case PIPE_FORMAT_B4G4R4A4_UNORM:
      return MESA_FORMAT_ARGB4444;
   case PIPE_FORMAT_B5G6R5_UNORM:
      return MESA_FORMAT_RGB565;
   case PIPE_FORMAT_L8A8_UNORM:
      return MESA_FORMAT_AL88;
   case PIPE_FORMAT_A8_UNORM:
      return MESA_FORMAT_A8;
   case PIPE_FORMAT_L8_UNORM:
      return MESA_FORMAT_L8;
   case PIPE_FORMAT_I8_UNORM:
      return MESA_FORMAT_I8;
   case PIPE_FORMAT_S8_UNORM:
      return MESA_FORMAT_S8;

   case PIPE_FORMAT_R16G16B16A16_SNORM:
      return MESA_FORMAT_SIGNED_RGBA_16;

   case PIPE_FORMAT_Z16_UNORM:
      return MESA_FORMAT_Z16;
   case PIPE_FORMAT_Z32_UNORM:
      return MESA_FORMAT_Z32;
   case PIPE_FORMAT_S8Z24_UNORM:
      return MESA_FORMAT_Z24_S8;
   case PIPE_FORMAT_X8Z24_UNORM:
      return MESA_FORMAT_Z24_X8;
   case PIPE_FORMAT_Z24X8_UNORM:
      return MESA_FORMAT_X8_Z24;
   case PIPE_FORMAT_Z24S8_UNORM:
      return MESA_FORMAT_S8_Z24;

   case PIPE_FORMAT_UYVY:
      return MESA_FORMAT_YCBCR;
   case PIPE_FORMAT_YUYV:
      return MESA_FORMAT_YCBCR_REV;

#if FEATURE_texture_s3tc
   case PIPE_FORMAT_DXT1_RGB:
      return MESA_FORMAT_RGB_DXT1;
   case PIPE_FORMAT_DXT1_RGBA:
      return MESA_FORMAT_RGBA_DXT1;
   case PIPE_FORMAT_DXT3_RGBA:
      return MESA_FORMAT_RGBA_DXT3;
   case PIPE_FORMAT_DXT5_RGBA:
      return MESA_FORMAT_RGBA_DXT5;
#if FEATURE_EXT_texture_sRGB
   case PIPE_FORMAT_DXT1_SRGB:
      return MESA_FORMAT_SRGB_DXT1;
   case PIPE_FORMAT_DXT1_SRGBA:
      return MESA_FORMAT_SRGBA_DXT1;
   case PIPE_FORMAT_DXT3_SRGBA:
      return MESA_FORMAT_SRGBA_DXT3;
   case PIPE_FORMAT_DXT5_SRGBA:
      return MESA_FORMAT_SRGBA_DXT5;
#endif
#endif

#if FEATURE_EXT_texture_sRGB
   case PIPE_FORMAT_L8A8_SRGB:
      return MESA_FORMAT_SLA8;
   case PIPE_FORMAT_L8_SRGB:
      return MESA_FORMAT_SL8;
   case PIPE_FORMAT_R8G8B8_SRGB:
      return MESA_FORMAT_SRGB8;
   case PIPE_FORMAT_A8B8G8R8_SRGB:
      return MESA_FORMAT_SRGBA8;
   case PIPE_FORMAT_B8G8R8A8_SRGB:
      return MESA_FORMAT_SARGB8;
#endif
   default:
      assert(0);
      return MESA_FORMAT_NONE;
   }
}


/**
 * Find an RGBA format supported by the context/winsys.
 */
static enum pipe_format
default_rgba_format(struct pipe_screen *screen, 
                    enum pipe_texture_target target,
                    unsigned tex_usage, 
                    unsigned geom_flags)
{
   static const enum pipe_format colorFormats[] = {
      PIPE_FORMAT_B8G8R8A8_UNORM,
      PIPE_FORMAT_A8R8G8B8_UNORM,
      PIPE_FORMAT_A8B8G8R8_UNORM,
      PIPE_FORMAT_B5G6R5_UNORM
   };
   uint i;
   for (i = 0; i < Elements(colorFormats); i++) {
      if (screen->is_format_supported( screen, colorFormats[i], target, tex_usage, geom_flags )) {
         return colorFormats[i];
      }
   }
   return PIPE_FORMAT_NONE;
}

/**
 * Find an RGB format supported by the context/winsys.
 */
static enum pipe_format
default_rgb_format(struct pipe_screen *screen, 
                   enum pipe_texture_target target,
                   unsigned tex_usage, 
                   unsigned geom_flags)
{
   static const enum pipe_format colorFormats[] = {
      PIPE_FORMAT_B8G8R8X8_UNORM,
      PIPE_FORMAT_X8R8G8B8_UNORM,
      PIPE_FORMAT_X8B8G8R8_UNORM,
      PIPE_FORMAT_B8G8R8A8_UNORM,
      PIPE_FORMAT_A8R8G8B8_UNORM,
      PIPE_FORMAT_A8B8G8R8_UNORM,
      PIPE_FORMAT_B5G6R5_UNORM
   };
   uint i;
   for (i = 0; i < Elements(colorFormats); i++) {
      if (screen->is_format_supported( screen, colorFormats[i], target, tex_usage, geom_flags )) {
         return colorFormats[i];
      }
   }
   return PIPE_FORMAT_NONE;
}

/**
 * Find an sRGBA format supported by the context/winsys.
 */
static enum pipe_format
default_srgba_format(struct pipe_screen *screen, 
                    enum pipe_texture_target target,
                    unsigned tex_usage, 
                    unsigned geom_flags)
{
   static const enum pipe_format colorFormats[] = {
      PIPE_FORMAT_B8G8R8A8_SRGB,
      PIPE_FORMAT_A8R8G8B8_SRGB,
      PIPE_FORMAT_A8B8G8R8_SRGB,
   };
   uint i;
   for (i = 0; i < Elements(colorFormats); i++) {
      if (screen->is_format_supported( screen, colorFormats[i], target, tex_usage, geom_flags )) {
         return colorFormats[i];
      }
   }
   return PIPE_FORMAT_NONE;
}

/**
 * Search list of formats for first RGBA format with >8 bits/channel.
 */
static enum pipe_format
default_deep_rgba_format(struct pipe_screen *screen, 
                         enum pipe_texture_target target,
                         unsigned tex_usage, 
                         unsigned geom_flags)
{
   if (screen->is_format_supported(screen, PIPE_FORMAT_R16G16B16A16_SNORM, target, tex_usage, geom_flags)) {
      return PIPE_FORMAT_R16G16B16A16_SNORM;
   }
   if (tex_usage & PIPE_TEXTURE_USAGE_RENDER_TARGET)
      return default_rgba_format(screen, target, tex_usage, geom_flags);
   else
      return PIPE_FORMAT_NONE;
}


/**
 * Find an Z format supported by the context/winsys.
 */
static enum pipe_format
default_depth_format(struct pipe_screen *screen, 
                     enum pipe_texture_target target,
                     unsigned tex_usage, 
                     unsigned geom_flags)
{
   static const enum pipe_format zFormats[] = {
      PIPE_FORMAT_Z16_UNORM,
      PIPE_FORMAT_Z32_UNORM,
      PIPE_FORMAT_Z24S8_UNORM,
      PIPE_FORMAT_S8Z24_UNORM
   };
   uint i;
   for (i = 0; i < Elements(zFormats); i++) {
      if (screen->is_format_supported( screen, zFormats[i], target, tex_usage, geom_flags )) {
         return zFormats[i];
      }
   }
   return PIPE_FORMAT_NONE;
}


/**
 * Given an OpenGL internalFormat value for a texture or surface, return
 * the best matching PIPE_FORMAT_x, or PIPE_FORMAT_NONE if there's no match.
 * \param target  one of PIPE_TEXTURE_x
 * \param tex_usage  either PIPE_TEXTURE_USAGE_RENDER_TARGET
 *                   or PIPE_TEXTURE_USAGE_SAMPLER
 */
enum pipe_format
st_choose_format(struct pipe_screen *screen, GLenum internalFormat,
                 enum pipe_texture_target target, unsigned tex_usage)
{
   unsigned geom_flags = 0;

   switch (internalFormat) {
   case 4:
   case GL_RGBA:
   case GL_COMPRESSED_RGBA:
   case GL_RGBA8:
   case GL_RGB10_A2:
   case GL_RGBA12:
      return default_rgba_format( screen, target, tex_usage, geom_flags );
   case 3:
   case GL_RGB:
   case GL_COMPRESSED_RGB:
      return default_rgb_format( screen, target, tex_usage, geom_flags );
   case GL_RGBA16:
      if (tex_usage & PIPE_TEXTURE_USAGE_RENDER_TARGET)
         return default_deep_rgba_format( screen, target, tex_usage, geom_flags );
      else
         return default_rgba_format( screen, target, tex_usage, geom_flags );

   case GL_RGBA4:
   case GL_RGBA2:
      if (screen->is_format_supported( screen, PIPE_FORMAT_B4G4R4A4_UNORM, target, tex_usage, geom_flags ))
         return PIPE_FORMAT_B4G4R4A4_UNORM;
      return default_rgba_format( screen, target, tex_usage, geom_flags );

   case GL_RGB5_A1:
      if (screen->is_format_supported( screen, PIPE_FORMAT_B5G5R5A1_UNORM, target, tex_usage, geom_flags ))
         return PIPE_FORMAT_B5G5R5A1_UNORM;
      return default_rgba_format( screen, target, tex_usage, geom_flags );

   case GL_RGB8:
   case GL_RGB10:
   case GL_RGB12:
   case GL_RGB16:
      return default_rgb_format( screen, target, tex_usage, geom_flags );

   case GL_RGB5:
   case GL_RGB4:
   case GL_R3_G3_B2:
      if (screen->is_format_supported( screen, PIPE_FORMAT_B5G6R5_UNORM, target, tex_usage, geom_flags ))
         return PIPE_FORMAT_B5G6R5_UNORM;
      if (screen->is_format_supported( screen, PIPE_FORMAT_B5G5R5A1_UNORM, target, tex_usage, geom_flags ))
         return PIPE_FORMAT_B5G5R5A1_UNORM;
      return default_rgba_format( screen, target, tex_usage, geom_flags );

   case GL_ALPHA:
   case GL_ALPHA4:
   case GL_ALPHA8:
   case GL_ALPHA12:
   case GL_ALPHA16:
   case GL_COMPRESSED_ALPHA:
      if (screen->is_format_supported( screen, PIPE_FORMAT_A8_UNORM, target, tex_usage, geom_flags ))
         return PIPE_FORMAT_A8_UNORM;
      return default_rgba_format( screen, target, tex_usage, geom_flags );

   case 1:
   case GL_LUMINANCE:
   case GL_LUMINANCE4:
   case GL_LUMINANCE8:
   case GL_LUMINANCE12:
   case GL_LUMINANCE16:
   case GL_COMPRESSED_LUMINANCE:
      if (screen->is_format_supported( screen, PIPE_FORMAT_L8_UNORM, target, tex_usage, geom_flags ))
         return PIPE_FORMAT_L8_UNORM;
      return default_rgba_format( screen, target, tex_usage, geom_flags );

   case 2:
   case GL_LUMINANCE_ALPHA:
   case GL_LUMINANCE4_ALPHA4:
   case GL_LUMINANCE6_ALPHA2:
   case GL_LUMINANCE8_ALPHA8:
   case GL_LUMINANCE12_ALPHA4:
   case GL_LUMINANCE12_ALPHA12:
   case GL_LUMINANCE16_ALPHA16:
   case GL_COMPRESSED_LUMINANCE_ALPHA:
      if (screen->is_format_supported( screen, PIPE_FORMAT_L8A8_UNORM, target, tex_usage, geom_flags ))
         return PIPE_FORMAT_L8A8_UNORM;
      return default_rgba_format( screen, target, tex_usage, geom_flags );

   case GL_INTENSITY:
   case GL_INTENSITY4:
   case GL_INTENSITY8:
   case GL_INTENSITY12:
   case GL_INTENSITY16:
   case GL_COMPRESSED_INTENSITY:
      if (screen->is_format_supported( screen, PIPE_FORMAT_I8_UNORM, target, tex_usage, geom_flags ))
         return PIPE_FORMAT_I8_UNORM;
      return default_rgba_format( screen, target, tex_usage, geom_flags );

   case GL_YCBCR_MESA:
      if (screen->is_format_supported(screen, PIPE_FORMAT_UYVY,
                                      target, tex_usage, geom_flags)) {
         return PIPE_FORMAT_UYVY;
      }
      if (screen->is_format_supported(screen, PIPE_FORMAT_YUYV,
                                      target, tex_usage, geom_flags)) {
         return PIPE_FORMAT_YUYV;
      }
      return PIPE_FORMAT_NONE;

   case GL_RGB_S3TC:
   case GL_RGB4_S3TC:
   case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
      return PIPE_FORMAT_DXT1_RGB;

   case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
      return PIPE_FORMAT_DXT1_RGBA;

   case GL_RGBA_S3TC:
   case GL_RGBA4_S3TC:
   case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
      return PIPE_FORMAT_DXT3_RGBA;

   case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
      return PIPE_FORMAT_DXT5_RGBA;

#if 0
   case GL_COMPRESSED_RGB_FXT1_3DFX:
      return PIPE_FORMAT_RGB_FXT1;
   case GL_COMPRESSED_RGBA_FXT1_3DFX:
      return PIPE_FORMAT_RGB_FXT1;
#endif

   case GL_DEPTH_COMPONENT16:
      if (screen->is_format_supported( screen, PIPE_FORMAT_Z16_UNORM, target, tex_usage, geom_flags ))
         return PIPE_FORMAT_Z16_UNORM;
      /* fall-through */
   case GL_DEPTH_COMPONENT24:
      if (screen->is_format_supported( screen, PIPE_FORMAT_Z24S8_UNORM, target, tex_usage, geom_flags ))
         return PIPE_FORMAT_Z24S8_UNORM;
      if (screen->is_format_supported( screen, PIPE_FORMAT_S8Z24_UNORM, target, tex_usage, geom_flags ))
         return PIPE_FORMAT_S8Z24_UNORM;
      /* fall-through */
   case GL_DEPTH_COMPONENT32:
      if (screen->is_format_supported( screen, PIPE_FORMAT_Z32_UNORM, target, tex_usage, geom_flags ))
         return PIPE_FORMAT_Z32_UNORM;
      /* fall-through */
   case GL_DEPTH_COMPONENT:
      return default_depth_format( screen, target, tex_usage, geom_flags );

   case GL_STENCIL_INDEX:
   case GL_STENCIL_INDEX1_EXT:
   case GL_STENCIL_INDEX4_EXT:
   case GL_STENCIL_INDEX8_EXT:
   case GL_STENCIL_INDEX16_EXT:
      if (screen->is_format_supported( screen, PIPE_FORMAT_S8_UNORM, target, tex_usage, geom_flags ))
         return PIPE_FORMAT_S8_UNORM;
      if (screen->is_format_supported( screen, PIPE_FORMAT_Z24S8_UNORM, target, tex_usage, geom_flags ))
         return PIPE_FORMAT_Z24S8_UNORM;
      if (screen->is_format_supported( screen, PIPE_FORMAT_S8Z24_UNORM, target, tex_usage, geom_flags ))
         return PIPE_FORMAT_S8Z24_UNORM;
      return PIPE_FORMAT_NONE;

   case GL_DEPTH_STENCIL_EXT:
   case GL_DEPTH24_STENCIL8_EXT:
      if (screen->is_format_supported( screen, PIPE_FORMAT_Z24S8_UNORM, target, tex_usage, geom_flags ))
         return PIPE_FORMAT_Z24S8_UNORM;
      if (screen->is_format_supported( screen, PIPE_FORMAT_S8Z24_UNORM, target, tex_usage, geom_flags ))
         return PIPE_FORMAT_S8Z24_UNORM;
      return PIPE_FORMAT_NONE;

   case GL_SRGB_EXT:
   case GL_SRGB8_EXT:
   case GL_COMPRESSED_SRGB_EXT:
   case GL_COMPRESSED_SRGB_ALPHA_EXT:
   case GL_SRGB_ALPHA_EXT:
   case GL_SRGB8_ALPHA8_EXT:
      return default_srgba_format( screen, target, tex_usage, geom_flags );
   case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
      return PIPE_FORMAT_DXT1_SRGB;
   case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
      return PIPE_FORMAT_DXT1_SRGBA;
   case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
      return PIPE_FORMAT_DXT3_SRGBA;
   case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
      return PIPE_FORMAT_DXT5_SRGBA;

   case GL_SLUMINANCE_ALPHA_EXT:
   case GL_SLUMINANCE8_ALPHA8_EXT:
   case GL_COMPRESSED_SLUMINANCE_EXT:
   case GL_COMPRESSED_SLUMINANCE_ALPHA_EXT:
      if (screen->is_format_supported( screen, PIPE_FORMAT_L8A8_SRGB, target, tex_usage, geom_flags ))
         return PIPE_FORMAT_L8A8_SRGB;
      return default_srgba_format( screen, target, tex_usage, geom_flags );

   case GL_SLUMINANCE_EXT:
   case GL_SLUMINANCE8_EXT:
      if (screen->is_format_supported( screen, PIPE_FORMAT_L8_SRGB, target, tex_usage, geom_flags ))
         return PIPE_FORMAT_L8_SRGB;
      return default_srgba_format( screen, target, tex_usage, geom_flags );

   default:
      return PIPE_FORMAT_NONE;
   }
}


static GLboolean
is_depth_or_stencil_format(GLenum internalFormat)
{
   switch (internalFormat) {
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
      return GL_TRUE;
   default:
      return GL_FALSE;
   }
}

/**
 * Called by FBO code to choose a PIPE_FORMAT_ for drawing surfaces.
 */
enum pipe_format
st_choose_renderbuffer_format(struct pipe_screen *screen,
                              GLenum internalFormat)
{
   uint usage;
   if (is_depth_or_stencil_format(internalFormat))
      usage = PIPE_TEXTURE_USAGE_DEPTH_STENCIL;
   else
      usage = PIPE_TEXTURE_USAGE_RENDER_TARGET;
   return st_choose_format(screen, internalFormat, PIPE_TEXTURE_2D, usage);
}


/**
 * Called via ctx->Driver.chooseTextureFormat().
 */
gl_format
st_ChooseTextureFormat(GLcontext *ctx, GLint internalFormat,
                       GLenum format, GLenum type)
{
   enum pipe_format pFormat;

   (void) format;
   (void) type;

   pFormat = st_choose_format(ctx->st->pipe->screen, internalFormat,
                              PIPE_TEXTURE_2D, PIPE_TEXTURE_USAGE_SAMPLER);
   if (pFormat == PIPE_FORMAT_NONE)
      return MESA_FORMAT_NONE;

   return st_pipe_format_to_mesa_format(pFormat);
}


/**
 * Test if a gallium format is equivalent to a GL format/type.
 */
GLboolean
st_equal_formats(enum pipe_format pFormat, GLenum format, GLenum type)
{
   switch (pFormat) {
   case PIPE_FORMAT_A8B8G8R8_UNORM:
      return format == GL_RGBA && type == GL_UNSIGNED_BYTE;
   case PIPE_FORMAT_A8R8G8B8_UNORM:
      return format == GL_BGRA && type == GL_UNSIGNED_BYTE;
   case PIPE_FORMAT_B5G6R5_UNORM:
      return format == GL_RGB && type == GL_UNSIGNED_SHORT_5_6_5;
   /* XXX more combos... */
   default:
      return GL_FALSE;
   }
}

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
#include "main/image.h"
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
      else if (format == PIPE_FORMAT_Z24_UNORM_S8_USCALED ||
               format == PIPE_FORMAT_S8_USCALED_Z24_UNORM ||
               format == PIPE_FORMAT_Z24X8_UNORM ||
               format == PIPE_FORMAT_X8Z24_UNORM) {
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
   case MESA_FORMAT_RGBA8888:
      return PIPE_FORMAT_A8B8G8R8_UNORM;
   case MESA_FORMAT_RGBA8888_REV:
      return PIPE_FORMAT_R8G8B8A8_UNORM;
   case MESA_FORMAT_ARGB8888:
      return PIPE_FORMAT_B8G8R8A8_UNORM;
   case MESA_FORMAT_ARGB8888_REV:
      return PIPE_FORMAT_A8R8G8B8_UNORM;
   case MESA_FORMAT_XRGB8888:
      return PIPE_FORMAT_B8G8R8X8_UNORM;
   case MESA_FORMAT_XRGB8888_REV:
      return PIPE_FORMAT_X8R8G8B8_UNORM;
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
      return PIPE_FORMAT_S8_USCALED_Z24_UNORM;
   case MESA_FORMAT_S8_Z24:
      return PIPE_FORMAT_Z24_UNORM_S8_USCALED;
   case MESA_FORMAT_Z24_X8:
      return PIPE_FORMAT_X8Z24_UNORM;
   case MESA_FORMAT_X8_Z24:
      return PIPE_FORMAT_Z24X8_UNORM;
   case MESA_FORMAT_S8:
      return PIPE_FORMAT_S8_USCALED;
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
   case MESA_FORMAT_R8:
      return PIPE_FORMAT_R8_UNORM;
   case MESA_FORMAT_R16:
      return PIPE_FORMAT_R16_UNORM;
   case MESA_FORMAT_RG88:
      return PIPE_FORMAT_R8G8_UNORM;
   case MESA_FORMAT_RG1616:
      return PIPE_FORMAT_R16G16_UNORM;
   default:
      assert(0);
      return PIPE_FORMAT_NONE;
   }
}


/**
 * Translate Gallium format to Mesa format.
 */
gl_format
st_pipe_format_to_mesa_format(enum pipe_format format)
{
   switch (format) {
   case PIPE_FORMAT_A8B8G8R8_UNORM:
      return MESA_FORMAT_RGBA8888;
   case PIPE_FORMAT_R8G8B8A8_UNORM:
      return MESA_FORMAT_RGBA8888_REV;
   case PIPE_FORMAT_B8G8R8A8_UNORM:
      return MESA_FORMAT_ARGB8888;
   case PIPE_FORMAT_A8R8G8B8_UNORM:
      return MESA_FORMAT_ARGB8888_REV;
   case PIPE_FORMAT_B8G8R8X8_UNORM:
      return MESA_FORMAT_XRGB8888;
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
   case PIPE_FORMAT_S8_USCALED:
      return MESA_FORMAT_S8;

   case PIPE_FORMAT_R16G16B16A16_SNORM:
      return MESA_FORMAT_SIGNED_RGBA_16;

   case PIPE_FORMAT_Z16_UNORM:
      return MESA_FORMAT_Z16;
   case PIPE_FORMAT_Z32_UNORM:
      return MESA_FORMAT_Z32;
   case PIPE_FORMAT_S8_USCALED_Z24_UNORM:
      return MESA_FORMAT_Z24_S8;
   case PIPE_FORMAT_X8Z24_UNORM:
      return MESA_FORMAT_Z24_X8;
   case PIPE_FORMAT_Z24X8_UNORM:
      return MESA_FORMAT_X8_Z24;
   case PIPE_FORMAT_Z24_UNORM_S8_USCALED:
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

   case PIPE_FORMAT_R8_UNORM:
      return MESA_FORMAT_R8;
   case PIPE_FORMAT_R16_UNORM:
      return MESA_FORMAT_R16;
   case PIPE_FORMAT_R8G8_UNORM:
      return MESA_FORMAT_RG88;
   case PIPE_FORMAT_R16G16_UNORM:
      return MESA_FORMAT_RG1616;
   default:
      assert(0);
      return MESA_FORMAT_NONE;
   }
}


/**
 * Return first supported format from the given list.
 */
static enum pipe_format
find_supported_format(struct pipe_screen *screen, 
                      const enum pipe_format formats[],
                      uint num_formats,
                      enum pipe_texture_target target,
                      unsigned sample_count,
                      unsigned tex_usage,
                      unsigned geom_flags)
{
   uint i;
   for (i = 0; i < num_formats; i++) {
      if (screen->is_format_supported(screen, formats[i], target,
                                      sample_count, tex_usage, geom_flags)) {
         return formats[i];
      }
   }
   return PIPE_FORMAT_NONE;
}


/**
 * Find an RGBA format supported by the context/winsys.
 */
static enum pipe_format
default_rgba_format(struct pipe_screen *screen, 
                    enum pipe_texture_target target,
                    unsigned sample_count,
                    unsigned tex_usage,
                    unsigned geom_flags)
{
   static const enum pipe_format colorFormats[] = {
      PIPE_FORMAT_B8G8R8A8_UNORM,
      PIPE_FORMAT_A8R8G8B8_UNORM,
      PIPE_FORMAT_A8B8G8R8_UNORM,
      PIPE_FORMAT_B5G6R5_UNORM
   };
   return find_supported_format(screen, colorFormats, Elements(colorFormats),
                                target, sample_count, tex_usage, geom_flags);
}


/**
 * Find an RGB format supported by the context/winsys.
 */
static enum pipe_format
default_rgb_format(struct pipe_screen *screen, 
                   enum pipe_texture_target target,
                   unsigned sample_count,
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
   return find_supported_format(screen, colorFormats, Elements(colorFormats),
                                target, sample_count, tex_usage, geom_flags);
}

/**
 * Find an sRGBA format supported by the context/winsys.
 */
static enum pipe_format
default_srgba_format(struct pipe_screen *screen, 
                    enum pipe_texture_target target,
                    unsigned sample_count,
                    unsigned tex_usage,
                    unsigned geom_flags)
{
   static const enum pipe_format colorFormats[] = {
      PIPE_FORMAT_B8G8R8A8_SRGB,
      PIPE_FORMAT_A8R8G8B8_SRGB,
      PIPE_FORMAT_A8B8G8R8_SRGB,
   };
   return find_supported_format(screen, colorFormats, Elements(colorFormats),
                                target, sample_count, tex_usage, geom_flags);
}


/**
 * Given an OpenGL internalFormat value for a texture or surface, return
 * the best matching PIPE_FORMAT_x, or PIPE_FORMAT_NONE if there's no match.
 * This is called during glTexImage2D, for example.
 *
 * The bindings parameter typically has PIPE_BIND_SAMPLER_VIEW set, plus
 * either PIPE_BINDING_RENDER_TARGET or PIPE_BINDING_DEPTH_STENCIL if
 * we want render-to-texture ability.
 *
 * \param internalFormat  the user value passed to glTexImage2D
 * \param target  one of PIPE_TEXTURE_x
 * \param bindings  bitmask of PIPE_BIND_x flags.
 */
enum pipe_format
st_choose_format(struct pipe_screen *screen, GLenum internalFormat,
                 enum pipe_texture_target target, unsigned sample_count,
                 unsigned bindings)
{
   unsigned geom_flags = 0; /* we don't care about POT vs. NPOT here, yet */

   switch (internalFormat) {
   case 4:
   case GL_RGBA:
   case GL_RGBA8:
   case GL_RGB10_A2:
   case GL_RGBA12:
      return default_rgba_format( screen, target, sample_count, bindings,
                                  geom_flags );
   case 3:
   case GL_RGB:
      return default_rgb_format( screen, target, sample_count, bindings,
                                 geom_flags );
   case GL_RGBA16:
      return default_rgba_format( screen, target, sample_count, bindings,
                                  geom_flags );

   case GL_RGBA4:
   case GL_RGBA2:
      if (screen->is_format_supported( screen, PIPE_FORMAT_B4G4R4A4_UNORM,
                                       target, sample_count, bindings,
                                       geom_flags ))
         return PIPE_FORMAT_B4G4R4A4_UNORM;
      return default_rgba_format( screen, target, sample_count, bindings,
                                  geom_flags );

   case GL_RGB5_A1:
      if (screen->is_format_supported( screen, PIPE_FORMAT_B5G5R5A1_UNORM,
                                       target, sample_count, bindings,
                                       geom_flags ))
         return PIPE_FORMAT_B5G5R5A1_UNORM;
      return default_rgba_format( screen, target, sample_count, bindings,
                                  geom_flags );

   case GL_RGB8:
   case GL_RGB10:
   case GL_RGB12:
   case GL_RGB16:
      return default_rgb_format( screen, target, sample_count, bindings,
                                 geom_flags );

   case GL_RGB5:
   case GL_RGB4:
   case GL_R3_G3_B2:
      if (screen->is_format_supported( screen, PIPE_FORMAT_B5G6R5_UNORM,
                                       target, sample_count, bindings,
                                       geom_flags ))
         return PIPE_FORMAT_B5G6R5_UNORM;
      if (screen->is_format_supported( screen, PIPE_FORMAT_B5G5R5A1_UNORM,
                                       target, sample_count, bindings,
                                       geom_flags ))
         return PIPE_FORMAT_B5G5R5A1_UNORM;
      return default_rgba_format( screen, target, sample_count, bindings,
                                  geom_flags );

   case GL_ALPHA:
   case GL_ALPHA4:
   case GL_ALPHA8:
   case GL_ALPHA12:
   case GL_ALPHA16:
   case GL_COMPRESSED_ALPHA:
      if (screen->is_format_supported( screen, PIPE_FORMAT_A8_UNORM, target,
                                       sample_count, bindings, geom_flags ))
         return PIPE_FORMAT_A8_UNORM;
      return default_rgba_format( screen, target, sample_count, bindings,
                                  geom_flags );

   case 1:
   case GL_LUMINANCE:
   case GL_LUMINANCE4:
   case GL_LUMINANCE8:
   case GL_LUMINANCE12:
   case GL_LUMINANCE16:
   case GL_COMPRESSED_LUMINANCE:
      if (screen->is_format_supported( screen, PIPE_FORMAT_L8_UNORM, target,
                                       sample_count, bindings, geom_flags ))
         return PIPE_FORMAT_L8_UNORM;
      return default_rgba_format( screen, target, sample_count, bindings,
                                  geom_flags );

   case 2:
   case GL_LUMINANCE_ALPHA:
   case GL_LUMINANCE4_ALPHA4:
   case GL_LUMINANCE6_ALPHA2:
   case GL_LUMINANCE8_ALPHA8:
   case GL_LUMINANCE12_ALPHA4:
   case GL_LUMINANCE12_ALPHA12:
   case GL_LUMINANCE16_ALPHA16:
   case GL_COMPRESSED_LUMINANCE_ALPHA:
      if (screen->is_format_supported( screen, PIPE_FORMAT_L8A8_UNORM, target,
                                       sample_count, bindings, geom_flags ))
         return PIPE_FORMAT_L8A8_UNORM;
      return default_rgba_format( screen, target, sample_count, bindings,
                                  geom_flags );

   case GL_INTENSITY:
   case GL_INTENSITY4:
   case GL_INTENSITY8:
   case GL_INTENSITY12:
   case GL_INTENSITY16:
   case GL_COMPRESSED_INTENSITY:
      if (screen->is_format_supported( screen, PIPE_FORMAT_I8_UNORM, target,
                                       sample_count, bindings, geom_flags ))
         return PIPE_FORMAT_I8_UNORM;
      return default_rgba_format( screen, target, sample_count, bindings,
                                  geom_flags );

   case GL_YCBCR_MESA:
      if (screen->is_format_supported(screen, PIPE_FORMAT_UYVY, target,
                                      sample_count, bindings, geom_flags)) {
         return PIPE_FORMAT_UYVY;
      }
      if (screen->is_format_supported(screen, PIPE_FORMAT_YUYV, target,
                                      sample_count, bindings, geom_flags)) {
         return PIPE_FORMAT_YUYV;
      }
      return PIPE_FORMAT_NONE;

   case GL_COMPRESSED_RGB:
      /* can only sample from compressed formats */
      if (bindings & ~PIPE_BIND_SAMPLER_VIEW)
         return PIPE_FORMAT_NONE;
      else if (screen->is_format_supported(screen, PIPE_FORMAT_DXT1_RGB,
                                           target, sample_count, bindings,
                                           geom_flags))
         return PIPE_FORMAT_DXT1_RGB;
      else
         return default_rgb_format(screen, target, sample_count, bindings,
                                   geom_flags);

   case GL_COMPRESSED_RGBA:
      /* can only sample from compressed formats */
      if (bindings & ~PIPE_BIND_SAMPLER_VIEW)
         return PIPE_FORMAT_NONE;
      else if (screen->is_format_supported(screen, PIPE_FORMAT_DXT3_RGBA,
                                           target, sample_count, bindings,
                                           geom_flags))
         return PIPE_FORMAT_DXT3_RGBA;
      else
         return default_rgba_format(screen, target, sample_count, bindings,
                                    geom_flags);

   case GL_RGB_S3TC:
   case GL_RGB4_S3TC:
   case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
      if (screen->is_format_supported(screen, PIPE_FORMAT_DXT1_RGB,
                                      target, sample_count, bindings,
                                      geom_flags))
         return PIPE_FORMAT_DXT1_RGB;
      else
         return PIPE_FORMAT_NONE;

   case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
      if (screen->is_format_supported(screen, PIPE_FORMAT_DXT1_RGBA,
                                      target, sample_count, bindings,
                                      geom_flags))
         return PIPE_FORMAT_DXT1_RGBA;
      else
         return PIPE_FORMAT_NONE;

   case GL_RGBA_S3TC:
   case GL_RGBA4_S3TC:
   case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
      if (screen->is_format_supported(screen, PIPE_FORMAT_DXT3_RGBA,
                                      target, sample_count, bindings,
                                      geom_flags))
         return PIPE_FORMAT_DXT3_RGBA;
      else
         return PIPE_FORMAT_NONE;

   case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
      if (screen->is_format_supported(screen, PIPE_FORMAT_DXT5_RGBA,
                                      target, sample_count, bindings,
                                      geom_flags))
         return PIPE_FORMAT_DXT5_RGBA;
      else
         return PIPE_FORMAT_NONE;

#if 0
   case GL_COMPRESSED_RGB_FXT1_3DFX:
      return PIPE_FORMAT_RGB_FXT1;
   case GL_COMPRESSED_RGBA_FXT1_3DFX:
      return PIPE_FORMAT_RGB_FXT1;
#endif

   case GL_DEPTH_COMPONENT16:
      if (screen->is_format_supported(screen, PIPE_FORMAT_Z16_UNORM, target,
                                      sample_count, bindings, geom_flags))
         return PIPE_FORMAT_Z16_UNORM;
      /* fall-through */
   case GL_DEPTH_COMPONENT24:
      if (screen->is_format_supported(screen, PIPE_FORMAT_Z24_UNORM_S8_USCALED,
                                      target, sample_count, bindings, geom_flags))
         return PIPE_FORMAT_Z24_UNORM_S8_USCALED;
      if (screen->is_format_supported(screen, PIPE_FORMAT_S8_USCALED_Z24_UNORM,
                                      target, sample_count, bindings, geom_flags))
         return PIPE_FORMAT_S8_USCALED_Z24_UNORM;
      /* fall-through */
   case GL_DEPTH_COMPONENT32:
      if (screen->is_format_supported(screen, PIPE_FORMAT_Z32_UNORM, target,
                                      sample_count, bindings, geom_flags))
         return PIPE_FORMAT_Z32_UNORM;
      /* fall-through */
   case GL_DEPTH_COMPONENT:
      {
         static const enum pipe_format formats[] = {
            PIPE_FORMAT_Z16_UNORM,
            PIPE_FORMAT_Z32_UNORM,
            PIPE_FORMAT_Z24_UNORM_S8_USCALED,
            PIPE_FORMAT_S8_USCALED_Z24_UNORM
         };
         return find_supported_format(screen, formats, Elements(formats),
                                      target, sample_count, bindings, geom_flags);
      }

   case GL_STENCIL_INDEX:
   case GL_STENCIL_INDEX1_EXT:
   case GL_STENCIL_INDEX4_EXT:
   case GL_STENCIL_INDEX8_EXT:
   case GL_STENCIL_INDEX16_EXT:
      {
         static const enum pipe_format formats[] = {
            PIPE_FORMAT_S8_USCALED,
            PIPE_FORMAT_Z24_UNORM_S8_USCALED,
            PIPE_FORMAT_S8_USCALED_Z24_UNORM
         };
         return find_supported_format(screen, formats, Elements(formats),
                                      target, sample_count, bindings, geom_flags);
      }

   case GL_DEPTH_STENCIL_EXT:
   case GL_DEPTH24_STENCIL8_EXT:
      {
         static const enum pipe_format formats[] = {
            PIPE_FORMAT_Z24_UNORM_S8_USCALED,
            PIPE_FORMAT_S8_USCALED_Z24_UNORM
         };
         return find_supported_format(screen, formats, Elements(formats),
                                      target, sample_count, bindings, geom_flags);
      }

   case GL_SRGB_EXT:
   case GL_SRGB8_EXT:
   case GL_COMPRESSED_SRGB_EXT:
   case GL_COMPRESSED_SRGB_ALPHA_EXT:
   case GL_SRGB_ALPHA_EXT:
   case GL_SRGB8_ALPHA8_EXT:
      return default_srgba_format( screen, target, sample_count, bindings,
                                   geom_flags );
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
      if (screen->is_format_supported(screen, PIPE_FORMAT_L8A8_SRGB, target,
                                      sample_count, bindings, geom_flags))
         return PIPE_FORMAT_L8A8_SRGB;
      return default_srgba_format( screen, target, sample_count, bindings,
                                   geom_flags );

   case GL_SLUMINANCE_EXT:
   case GL_SLUMINANCE8_EXT:
      if (screen->is_format_supported(screen, PIPE_FORMAT_L8_SRGB, target,
                                      sample_count, bindings, geom_flags))
         return PIPE_FORMAT_L8_SRGB;
      return default_srgba_format( screen, target, sample_count, bindings,
                                   geom_flags );

   case GL_RED:
   case GL_R8:
      if (screen->is_format_supported(screen, PIPE_FORMAT_R8_UNORM, target,
				      sample_count, bindings, geom_flags))
	      return PIPE_FORMAT_R8_UNORM;
      return PIPE_FORMAT_NONE;
   case GL_RG:
   case GL_RG8:
      if (screen->is_format_supported(screen, PIPE_FORMAT_R8G8_UNORM, target,
				      sample_count, bindings, geom_flags))
	      return PIPE_FORMAT_R8G8_UNORM;
      return PIPE_FORMAT_NONE;

   case GL_R16:
      if (screen->is_format_supported(screen, PIPE_FORMAT_R16_UNORM, target,
				      sample_count, bindings, geom_flags))
	      return PIPE_FORMAT_R16_UNORM;
      return PIPE_FORMAT_NONE;

   case GL_RG16:
      if (screen->is_format_supported(screen, PIPE_FORMAT_R16G16_UNORM, target,
				      sample_count, bindings, geom_flags))
	      return PIPE_FORMAT_R16G16_UNORM;
      return PIPE_FORMAT_NONE;

   case GL_COMPRESSED_RED_RGTC1:
      if (screen->is_format_supported(screen, PIPE_FORMAT_RGTC1_UNORM, target,
				      sample_count, bindings, geom_flags))
	      return PIPE_FORMAT_RGTC1_UNORM;
      return PIPE_FORMAT_NONE;

   case GL_COMPRESSED_SIGNED_RED_RGTC1:
      if (screen->is_format_supported(screen, PIPE_FORMAT_RGTC1_SNORM, target,
				      sample_count, bindings, geom_flags))
	      return PIPE_FORMAT_RGTC1_SNORM;
      return PIPE_FORMAT_NONE;

   case GL_COMPRESSED_RG_RGTC2:
      if (screen->is_format_supported(screen, PIPE_FORMAT_RGTC2_UNORM, target,
				      sample_count, bindings, geom_flags))
	      return PIPE_FORMAT_RGTC2_UNORM;
      return PIPE_FORMAT_NONE;

   case GL_COMPRESSED_SIGNED_RG_RGTC2:
      if (screen->is_format_supported(screen, PIPE_FORMAT_RGTC2_SNORM, target,
				      sample_count, bindings, geom_flags))
	      return PIPE_FORMAT_RGTC2_SNORM;
      return PIPE_FORMAT_NONE;

   default:
      return PIPE_FORMAT_NONE;
   }
}


/**
 * Called by FBO code to choose a PIPE_FORMAT_ for drawing surfaces.
 */
enum pipe_format
st_choose_renderbuffer_format(struct pipe_screen *screen,
                              GLenum internalFormat, unsigned sample_count)
{
   uint usage;
   if (_mesa_is_depth_or_stencil_format(internalFormat))
      usage = PIPE_BIND_DEPTH_STENCIL;
   else
      usage = PIPE_BIND_RENDER_TARGET;
   return st_choose_format(screen, internalFormat, PIPE_TEXTURE_2D,
                           sample_count, usage);
}


/**
 * Called via ctx->Driver.chooseTextureFormat().
 */
gl_format
st_ChooseTextureFormat(GLcontext *ctx, GLint internalFormat,
                       GLenum format, GLenum type)
{
   struct pipe_screen *screen = st_context(ctx)->pipe->screen;
   enum pipe_format pFormat;
   uint bindings;

   (void) format;
   (void) type;

   /* GL textures may wind up being render targets, but we don't know
    * that in advance.  Specify potential render target flags now.
    */
   if (_mesa_is_depth_format(internalFormat) ||
       _mesa_is_depthstencil_format(internalFormat))
      bindings = PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_DEPTH_STENCIL;
   else 
      bindings = PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET;

   pFormat = st_choose_format(screen, internalFormat,
                              PIPE_TEXTURE_2D, 0, bindings);

   if (pFormat == PIPE_FORMAT_NONE) {
      /* try choosing format again, this time without render target bindings */
      pFormat = st_choose_format(screen, internalFormat,
                                 PIPE_TEXTURE_2D, 0, PIPE_BIND_SAMPLER_VIEW);
   }

   if (pFormat == PIPE_FORMAT_NONE) {
      /* no luck at all */
      return MESA_FORMAT_NONE;
   }

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

GLboolean
st_sampler_compat_formats(enum pipe_format format1, enum pipe_format format2)
{
   if (format1 == format2)
      return GL_TRUE;

   if (format1 == PIPE_FORMAT_B8G8R8A8_UNORM &&
       format2 == PIPE_FORMAT_B8G8R8X8_UNORM)
      return GL_TRUE;

   if (format1 == PIPE_FORMAT_B8G8R8X8_UNORM &&
       format2 == PIPE_FORMAT_B8G8R8A8_UNORM)
      return GL_TRUE;

   if (format1 == PIPE_FORMAT_A8B8G8R8_UNORM &&
       format2 == PIPE_FORMAT_X8B8G8R8_UNORM)
      return GL_TRUE;

   if (format1 == PIPE_FORMAT_X8B8G8R8_UNORM &&
       format2 == PIPE_FORMAT_A8B8G8R8_UNORM)
      return GL_TRUE;

   if (format1 == PIPE_FORMAT_A8R8G8B8_UNORM &&
       format2 == PIPE_FORMAT_X8R8G8B8_UNORM)
      return GL_TRUE;

   if (format1 == PIPE_FORMAT_X8R8G8B8_UNORM &&
       format2 == PIPE_FORMAT_A8R8G8B8_UNORM)
      return GL_TRUE;

   return GL_FALSE;
}

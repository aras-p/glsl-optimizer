/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * Texture Image-related functions.
 * \author Brian Paul
 */

#include "main/imports.h"
#include "main/context.h"
#include "main/texstore.h"
#include "main/texformat.h"
#include "main/enums.h"
#include "main/macros.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_screen.h"
#include "st_context.h"
#include "st_format.h"

static GLuint
format_bits(
   pipe_format_rgbazs_t  info,
   GLuint comp )
{
   GLuint   size;

   if (pf_swizzle_x(info) == comp) {
      size = pf_size_x(info);
   }
   else if (pf_swizzle_y(info) == comp) {
      size = pf_size_y(info);
   }
   else if (pf_swizzle_z(info) == comp) {
      size = pf_size_z(info);
   }
   else if (pf_swizzle_w(info) == comp) {
      size = pf_size_w(info);
   }
   else {
      size = 0;
   }
   return size << (pf_exp8(info) * 3);
}

static GLuint
format_max_bits(
   pipe_format_rgbazs_t  info )
{
   GLuint   size = format_bits( info, PIPE_FORMAT_COMP_R );

   size = MAX2( size, format_bits( info, PIPE_FORMAT_COMP_G ) );
   size = MAX2( size, format_bits( info, PIPE_FORMAT_COMP_B ) );
   size = MAX2( size, format_bits( info, PIPE_FORMAT_COMP_A ) );
   size = MAX2( size, format_bits( info, PIPE_FORMAT_COMP_Z ) );
   size = MAX2( size, format_bits( info, PIPE_FORMAT_COMP_S ) );
   return size;
}

static GLuint
format_size(
   pipe_format_rgbazs_t  info )
{
   return
      format_bits( info, PIPE_FORMAT_COMP_R ) +
      format_bits( info, PIPE_FORMAT_COMP_G ) +
      format_bits( info, PIPE_FORMAT_COMP_B ) +
      format_bits( info, PIPE_FORMAT_COMP_A ) +
      format_bits( info, PIPE_FORMAT_COMP_Z ) +
      format_bits( info, PIPE_FORMAT_COMP_S );
}

/*
 * XXX temporary here
 */
GLboolean
st_get_format_info(enum pipe_format format, struct pipe_format_info *pinfo)
{
   if (pf_layout(format) == PIPE_FORMAT_LAYOUT_RGBAZS) {
      pipe_format_rgbazs_t info;

      info = format;

#if 0
      {
         char  fmtname[256];

         pf_sprint_name( fmtname, format );
         printf(
            "%s\n",
            fmtname );
      }
#endif

      /* Data type */
      if (format == PIPE_FORMAT_A1R5G5B5_UNORM || format == PIPE_FORMAT_R5G6B5_UNORM) {
         pinfo->datatype = GL_UNSIGNED_SHORT;
      }
      else {
         GLuint size;

         size = format_max_bits( info );
         if (size == 8) {
            if (pf_type(info) == PIPE_FORMAT_TYPE_UNORM)
               pinfo->datatype = GL_UNSIGNED_BYTE;
            else
               pinfo->datatype = GL_BYTE;
         }
         else if (size == 16) {
            if (pf_type(info) == PIPE_FORMAT_TYPE_UNORM)
               pinfo->datatype = GL_UNSIGNED_SHORT;
            else
               pinfo->datatype = GL_SHORT;
         }
         else {
            assert( size <= 32 );
            if (pf_type(info) == PIPE_FORMAT_TYPE_UNORM)
               pinfo->datatype = GL_UNSIGNED_INT;
            else
               pinfo->datatype = GL_INT;
         }
      }

      /* Component bits */
      pinfo->red_bits = format_bits( info, PIPE_FORMAT_COMP_R );
      pinfo->green_bits = format_bits( info, PIPE_FORMAT_COMP_G );
      pinfo->blue_bits = format_bits( info, PIPE_FORMAT_COMP_B );
      pinfo->alpha_bits = format_bits( info, PIPE_FORMAT_COMP_A );
      pinfo->depth_bits = format_bits( info, PIPE_FORMAT_COMP_Z );
      pinfo->stencil_bits = format_bits( info, PIPE_FORMAT_COMP_S );
      pinfo->luminance_bits = 0;
      pinfo->intensity_bits = 0;

      /* Format size */
      pinfo->size = format_size( info ) / 8;

      /* Luminance & Intensity bits */
      if( pf_swizzle_x(info) == PIPE_FORMAT_COMP_R &&
          pf_swizzle_y(info) == PIPE_FORMAT_COMP_R &&
          pf_swizzle_z(info) == PIPE_FORMAT_COMP_R ) {
         if( pf_swizzle_w(info) == PIPE_FORMAT_COMP_R ) {
            pinfo->intensity_bits = pinfo->red_bits;
         }
         else {
            pinfo->luminance_bits = pinfo->red_bits;
         }
         pinfo->red_bits = 0;
      }

      /* Base format */
      if (pinfo->depth_bits) {
         if (pinfo->stencil_bits) {
            pinfo->base_format = GL_DEPTH_STENCIL_EXT;
         }
         else {
            pinfo->base_format = GL_DEPTH_COMPONENT;
         }
      }
      else if (pinfo->stencil_bits) {
         pinfo->base_format = GL_STENCIL_INDEX;
      }
      else {
         pinfo->base_format = GL_RGBA;
      }
   }
   else {
      pipe_format_ycbcr_t info;

      assert( pf_layout(format) == PIPE_FORMAT_LAYOUT_YCBCR );

      info = format;

      /* TODO */
      assert( 0 );
   }

#if 0
   printf(
      "ST_FORMAT: R(%u), G(%u), B(%u), A(%u), Z(%u), S(%u)\n",
      pinfo->red_bits,
      pinfo->green_bits,
      pinfo->blue_bits,
      pinfo->alpha_bits,
      pinfo->depth_bits,
      pinfo->stencil_bits );
#endif

   pinfo->format = format;

   return GL_TRUE;
}


/**
 * Return bytes per pixel for the given format.
 */
GLuint
st_sizeof_format(enum pipe_format format)
{
   struct pipe_format_info info;
   if (!st_get_format_info( format, &info )) {
      assert( 0 );
      return 0;
   }
   return info.size;
}


/**
 * Return bytes per pixel for the given format.
 */
GLenum
st_format_datatype(enum pipe_format format)
{
   struct pipe_format_info info;
   if (!st_get_format_info( format, &info )) {
      assert( 0 );
      return 0;
   }
   return info.datatype;
}


enum pipe_format
st_mesa_format_to_pipe_format(GLuint mesaFormat)
{
   switch (mesaFormat) {
      /* fix this */
   case MESA_FORMAT_ARGB8888_REV:
   case MESA_FORMAT_ARGB8888:
      return PIPE_FORMAT_A8R8G8B8_UNORM;
   case MESA_FORMAT_ARGB1555:
      return PIPE_FORMAT_A1R5G5B5_UNORM;
   case MESA_FORMAT_ARGB4444:
      return PIPE_FORMAT_A4R4G4B4_UNORM;
   case MESA_FORMAT_RGB565:
      return PIPE_FORMAT_R5G6B5_UNORM;
   case MESA_FORMAT_AL88:
      return PIPE_FORMAT_U_A8_L8;
   case MESA_FORMAT_A8:
      return PIPE_FORMAT_U_A8;
   case MESA_FORMAT_L8:
      return PIPE_FORMAT_U_L8;
   case MESA_FORMAT_I8:
      return PIPE_FORMAT_U_I8;
   case MESA_FORMAT_Z16:
      return PIPE_FORMAT_Z16_UNORM;
   default:
      assert(0);
      return 0;
   }
}

/**
 * Find an RGBA format supported by the context/winsys.
 */
static GLuint
default_rgba_format(struct pipe_context *pipe, uint type)
{
   static const enum pipe_format colorFormats[] = {
      PIPE_FORMAT_A8R8G8B8_UNORM,
      PIPE_FORMAT_B8G8R8A8_UNORM,
      PIPE_FORMAT_R8G8B8A8_UNORM,
      PIPE_FORMAT_R5G6B5_UNORM
   };
   struct pipe_screen *screen = pipe->screen;
   uint i;
   for (i = 0; i < Elements(colorFormats); i++) {
      if (screen->is_format_supported( screen, colorFormats[i], type )) {
         return colorFormats[i];
      }
   }
   return PIPE_FORMAT_NONE;
}


/**
 * Search list of formats for first RGBA format with >8 bits/channel.
 */
static GLuint
default_deep_rgba_format(struct pipe_context *pipe, uint type)
{
   struct pipe_screen *screen = pipe->screen;
   if (screen->is_format_supported(screen, PIPE_FORMAT_R16G16B16A16_SNORM, type)) {
      return PIPE_FORMAT_R16G16B16A16_SNORM;
   }
   return PIPE_FORMAT_NONE;
}


/**
 * Find an Z format supported by the context/winsys.
 */
static GLuint
default_depth_format(struct pipe_context *pipe, uint type)
{
   static const enum pipe_format zFormats[] = {
      PIPE_FORMAT_Z16_UNORM,
      PIPE_FORMAT_Z32_UNORM,
      PIPE_FORMAT_S8Z24_UNORM,
      PIPE_FORMAT_Z24S8_UNORM
   };
   struct pipe_screen *screen = pipe->screen;
   uint i;
   for (i = 0; i < Elements(zFormats); i++) {
      if (screen->is_format_supported( screen, zFormats[i], type )) {
         return zFormats[i];
      }
   }
   return PIPE_FORMAT_NONE;
}


/**
 * Choose the PIPE_FORMAT_ to use for user-created renderbuffers.
 *
 * \return PIPE_FORMAT_NONE if error/problem.
 */
enum pipe_format
st_choose_renderbuffer_format(struct pipe_context *pipe, GLint internalFormat)
{
   struct pipe_screen *screen = pipe->screen;
   uint surfType = PIPE_SURFACE;

   switch (internalFormat) {
   case 4:
   case GL_RGBA:
   case GL_COMPRESSED_RGBA:
   case 3:
   case GL_RGB:
   case GL_COMPRESSED_RGB:
   case GL_RGBA8:
   case GL_RGB10_A2:
   case GL_RGBA12:
      return default_rgba_format( pipe, surfType );
   case GL_RGBA16:
      return default_deep_rgba_format( pipe, surfType );

   case GL_RGBA4:
   case GL_RGBA2:
      if (screen->is_format_supported( screen, PIPE_FORMAT_A4R4G4B4_UNORM, surfType ))
         return PIPE_FORMAT_A4R4G4B4_UNORM;
      return default_rgba_format( pipe, surfType );

   case GL_RGB5_A1:
      if (screen->is_format_supported( screen, PIPE_FORMAT_A1R5G5B5_UNORM, surfType ))
         return PIPE_FORMAT_A1R5G5B5_UNORM;
      return default_rgba_format( pipe, surfType );

   case GL_RGB8:
   case GL_RGB10:
   case GL_RGB12:
   case GL_RGB16:
      return default_rgba_format( pipe, surfType );

   case GL_RGB5:
   case GL_RGB4:
   case GL_R3_G3_B2:
      if (screen->is_format_supported( screen, PIPE_FORMAT_A1R5G5B5_UNORM, surfType ))
         return PIPE_FORMAT_A1R5G5B5_UNORM;
      if (screen->is_format_supported( screen, PIPE_FORMAT_R5G6B5_UNORM, surfType ))
         return PIPE_FORMAT_R5G6B5_UNORM;
      return default_rgba_format( pipe, surfType );

   case GL_ALPHA:
   case GL_ALPHA4:
   case GL_ALPHA8:
   case GL_ALPHA12:
   case GL_ALPHA16:
   case GL_COMPRESSED_ALPHA:
      if (screen->is_format_supported( screen, PIPE_FORMAT_U_A8, surfType ))
         return PIPE_FORMAT_U_A8;
      return default_rgba_format( pipe, surfType );

   case 1:
   case GL_LUMINANCE:
   case GL_LUMINANCE4:
   case GL_LUMINANCE8:
   case GL_LUMINANCE12:
   case GL_LUMINANCE16:
   case GL_COMPRESSED_LUMINANCE:
      if (screen->is_format_supported( screen, PIPE_FORMAT_U_L8, surfType ))
         return PIPE_FORMAT_U_A8;
      return default_rgba_format( pipe, surfType );

   case 2:
   case GL_LUMINANCE_ALPHA:
   case GL_LUMINANCE4_ALPHA4:
   case GL_LUMINANCE6_ALPHA2:
   case GL_LUMINANCE8_ALPHA8:
   case GL_LUMINANCE12_ALPHA4:
   case GL_LUMINANCE12_ALPHA12:
   case GL_LUMINANCE16_ALPHA16:
   case GL_COMPRESSED_LUMINANCE_ALPHA:
      if (screen->is_format_supported( screen, PIPE_FORMAT_U_A8_L8, surfType ))
         return PIPE_FORMAT_U_A8_L8;
      return default_rgba_format( pipe, surfType );

   case GL_INTENSITY:
   case GL_INTENSITY4:
   case GL_INTENSITY8:
   case GL_INTENSITY12:
   case GL_INTENSITY16:
   case GL_COMPRESSED_INTENSITY:
      if (screen->is_format_supported( screen, PIPE_FORMAT_U_I8, surfType ))
         return PIPE_FORMAT_U_I8;
      return default_rgba_format( pipe, surfType );

#if 0
   /* not supported for renderbuffers */
   case GL_YCBCR_MESA:
      return PIPE_FORMAT_NONE;
   case GL_COMPRESSED_RGB_FXT1_3DFX:
      return &_mesa_texformat_rgb_fxt1;
   case GL_COMPRESSED_RGBA_FXT1_3DFX:
      return &_mesa_texformat_rgba_fxt1;

   case GL_RGB_S3TC:
   case GL_RGB4_S3TC:
   case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
      return &_mesa_texformat_rgb_dxt1;

   case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
      return &_mesa_texformat_rgba_dxt1;

   case GL_RGBA_S3TC:
   case GL_RGBA4_S3TC:
   case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
      return &_mesa_texformat_rgba_dxt3;

   case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
      return &_mesa_texformat_rgba_dxt5;
#endif

   case GL_DEPTH_COMPONENT16:
      if (screen->is_format_supported( screen, PIPE_FORMAT_Z16_UNORM, surfType ))
         return PIPE_FORMAT_Z16_UNORM;
      /* fall-through */
   case GL_DEPTH_COMPONENT24:
      if (screen->is_format_supported( screen, PIPE_FORMAT_S8Z24_UNORM, surfType ))
         return PIPE_FORMAT_S8Z24_UNORM;
      if (screen->is_format_supported( screen, PIPE_FORMAT_Z24S8_UNORM, surfType ))
         return PIPE_FORMAT_Z24S8_UNORM;
      /* fall-through */
   case GL_DEPTH_COMPONENT32:
      if (screen->is_format_supported( screen, PIPE_FORMAT_Z32_UNORM, surfType ))
         return PIPE_FORMAT_Z32_UNORM;
      /* fall-through */
   case GL_DEPTH_COMPONENT:
      return default_depth_format( pipe, surfType );

   case GL_STENCIL_INDEX:
   case GL_STENCIL_INDEX1_EXT:
   case GL_STENCIL_INDEX4_EXT:
   case GL_STENCIL_INDEX8_EXT:
   case GL_STENCIL_INDEX16_EXT:
      if (screen->is_format_supported( screen, PIPE_FORMAT_U_S8, surfType ))
         return PIPE_FORMAT_U_S8;
      if (screen->is_format_supported( screen, PIPE_FORMAT_S8Z24_UNORM, surfType ))
         return PIPE_FORMAT_S8Z24_UNORM;
      if (screen->is_format_supported( screen, PIPE_FORMAT_Z24S8_UNORM, surfType ))
         return PIPE_FORMAT_Z24S8_UNORM;
      return PIPE_FORMAT_NONE;

   case GL_DEPTH_STENCIL_EXT:
   case GL_DEPTH24_STENCIL8_EXT:
      if (screen->is_format_supported( screen, PIPE_FORMAT_S8Z24_UNORM, surfType ))
         return PIPE_FORMAT_S8Z24_UNORM;
      if (screen->is_format_supported( screen, PIPE_FORMAT_Z24S8_UNORM, surfType ))
         return PIPE_FORMAT_Z24S8_UNORM;
      return PIPE_FORMAT_NONE;

   default:
      return PIPE_FORMAT_NONE;
   }
}



/* It works out that this function is fine for all the supported
 * hardware.  However, there is still a need to map the formats onto
 * hardware descriptors.
 */
/* Note that the i915 can actually support many more formats than
 * these if we take the step of simply swizzling the colors
 * immediately after sampling...
 */
const struct gl_texture_format *
st_ChooseTextureFormat(GLcontext * ctx, GLint internalFormat,
                       GLenum format, GLenum type)
{
#if 0
   struct intel_context *intel = intel_context(ctx);
   const GLboolean do32bpt = (intel->intelScreen->front.cpp == 4);
#else
   const GLboolean do32bpt = 1;
#endif

   (void) ctx;

   switch (internalFormat) {
   case 4:
   case GL_RGBA:
   case GL_COMPRESSED_RGBA:
      if (format == GL_BGRA) {
         if (type == GL_UNSIGNED_BYTE || type == GL_UNSIGNED_INT_8_8_8_8_REV) {
            return &_mesa_texformat_argb8888;
         }
         else if (type == GL_UNSIGNED_SHORT_4_4_4_4_REV) {
            return &_mesa_texformat_argb4444;
         }
         else if (type == GL_UNSIGNED_SHORT_1_5_5_5_REV) {
            return &_mesa_texformat_argb1555;
         }
      }
      return do32bpt ? &_mesa_texformat_argb8888 : &_mesa_texformat_argb4444;

   case 3:
   case GL_RGB:
   case GL_COMPRESSED_RGB:
      if (format == GL_RGB && type == GL_UNSIGNED_SHORT_5_6_5) {
         return &_mesa_texformat_rgb565;
      }
      return do32bpt ? &_mesa_texformat_argb8888 : &_mesa_texformat_rgb565;

   case GL_RGBA8:
   case GL_RGB10_A2:
   case GL_RGBA12:
   case GL_RGBA16:
      return do32bpt ? &_mesa_texformat_argb8888 : &_mesa_texformat_argb4444;

   case GL_RGBA4:
   case GL_RGBA2:
      return &_mesa_texformat_argb4444;

   case GL_RGB5_A1:
      return &_mesa_texformat_argb1555;

   case GL_RGB8:
   case GL_RGB10:
   case GL_RGB12:
   case GL_RGB16:
      return &_mesa_texformat_argb8888;

   case GL_RGB5:
   case GL_RGB4:
   case GL_R3_G3_B2:
      return &_mesa_texformat_rgb565;

   case GL_ALPHA:
   case GL_ALPHA4:
   case GL_ALPHA8:
   case GL_ALPHA12:
   case GL_ALPHA16:
   case GL_COMPRESSED_ALPHA:
      return &_mesa_texformat_a8;

   case 1:
   case GL_LUMINANCE:
   case GL_LUMINANCE4:
   case GL_LUMINANCE8:
   case GL_LUMINANCE12:
   case GL_LUMINANCE16:
   case GL_COMPRESSED_LUMINANCE:
      return &_mesa_texformat_l8;

   case 2:
   case GL_LUMINANCE_ALPHA:
   case GL_LUMINANCE4_ALPHA4:
   case GL_LUMINANCE6_ALPHA2:
   case GL_LUMINANCE8_ALPHA8:
   case GL_LUMINANCE12_ALPHA4:
   case GL_LUMINANCE12_ALPHA12:
   case GL_LUMINANCE16_ALPHA16:
   case GL_COMPRESSED_LUMINANCE_ALPHA:
      return &_mesa_texformat_al88;

   case GL_INTENSITY:
   case GL_INTENSITY4:
   case GL_INTENSITY8:
   case GL_INTENSITY12:
   case GL_INTENSITY16:
   case GL_COMPRESSED_INTENSITY:
      return &_mesa_texformat_i8;

   case GL_YCBCR_MESA:
      if (type == GL_UNSIGNED_SHORT_8_8_MESA || type == GL_UNSIGNED_BYTE)
         return &_mesa_texformat_ycbcr;
      else
         return &_mesa_texformat_ycbcr_rev;

   case GL_COMPRESSED_RGB_FXT1_3DFX:
      return &_mesa_texformat_rgb_fxt1;
   case GL_COMPRESSED_RGBA_FXT1_3DFX:
      return &_mesa_texformat_rgba_fxt1;

   case GL_RGB_S3TC:
   case GL_RGB4_S3TC:
   case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
      return &_mesa_texformat_rgb_dxt1;

   case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
      return &_mesa_texformat_rgba_dxt1;

   case GL_RGBA_S3TC:
   case GL_RGBA4_S3TC:
   case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
      return &_mesa_texformat_rgba_dxt3;

   case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
      return &_mesa_texformat_rgba_dxt5;

   case GL_DEPTH_COMPONENT:
   case GL_DEPTH_COMPONENT16:
   case GL_DEPTH_COMPONENT24:
   case GL_DEPTH_COMPONENT32:
      return &_mesa_texformat_z16;

   case GL_DEPTH_STENCIL_EXT:
   case GL_DEPTH24_STENCIL8_EXT:
      return &_mesa_texformat_z24_s8;

   default:
      fprintf(stderr, "unexpected texture format %s in %s\n",
              _mesa_lookup_enum_by_nr(internalFormat), __FUNCTION__);
      return NULL;
   }

   return NULL;                 /* never get here */
}

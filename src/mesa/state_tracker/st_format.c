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

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "st_context.h"
#include "st_format.h"



/*
 * XXX temporary here
 */
const struct pipe_format_info *
st_get_format_info(GLuint format)
{
   static const struct pipe_format_info info[] = {
      {
         PIPE_FORMAT_U_R8_G8_B8_A8,  /* format */
         GL_RGBA,                    /* base_format */
         8, 8, 8, 8, 0, 0,           /* color bits */
         0, 0,                       /* depth, stencil */
         4                           /* size in bytes */
      },
      {
         PIPE_FORMAT_U_A8_R8_G8_B8,
         GL_RGBA,                    /* base_format */
         8, 8, 8, 8, 0, 0,           /* color bits */
         0, 0,                       /* depth, stencil */
         4                           /* size in bytes */
      },
      {
         PIPE_FORMAT_U_A1_R5_G5_B5,
         GL_RGBA,                    /* base_format */
         5, 5, 5, 1, 0, 0,           /* color bits */
         0, 0,                       /* depth, stencil */
         2                           /* size in bytes */
      },
      {
         PIPE_FORMAT_U_R5_G6_B5,
         GL_RGBA,                    /* base_format */
         5, 6, 5, 0, 0, 0,           /* color bits */
         0, 0,                       /* depth, stencil */
         2                           /* size in bytes */
      },
      /* XXX lots more */
      {
         PIPE_FORMAT_S8_Z24,
         GL_DEPTH_STENCIL_EXT,       /* base_format */
         0, 0, 0, 0, 0, 0,           /* color bits */
         24, 8,                      /* depth, stencil */
         4                           /* size in bytes */
      }
   };         
   GLuint i;

   for (i = 0; i < sizeof(info) / sizeof(info[0]); i++) {
      if (info[i].format == format)
         return info + i;
   }
   return NULL;
}


GLuint
st_sizeof_format(GLuint pipeFormat)
{
   const struct pipe_format_info *info = st_get_format_info(pipeFormat);
   assert(info);
   return info->size;
}


GLuint
st_mesa_format_to_pipe_format(GLuint mesaFormat)
{
   switch (mesaFormat) {
      /* fix this */
   case MESA_FORMAT_ARGB8888_REV:
   case MESA_FORMAT_ARGB8888:
      return PIPE_FORMAT_U_A8_R8_G8_B8;
   case MESA_FORMAT_AL88:
      return PIPE_FORMAT_U_A8_L8;
   case MESA_FORMAT_A8:
      return PIPE_FORMAT_U_A8;
   case MESA_FORMAT_L8:
      return PIPE_FORMAT_U_L8;
   case MESA_FORMAT_I8:
      return PIPE_FORMAT_U_I8;
   case MESA_FORMAT_Z16:
      return PIPE_FORMAT_U_Z16;
   default:
      assert(0);
      return 0;
   }
}


/**
 * Search list of formats for first RGBA format.
 */
static GLuint
default_rgba_format(const GLuint formats[], GLuint num)
{
   GLuint i;
   for (i = 0; i < num; i++) {
      if (formats[i] == PIPE_FORMAT_U_R8_G8_B8_A8 ||
          formats[i] == PIPE_FORMAT_U_A8_R8_G8_B8 ||
          formats[i] == PIPE_FORMAT_U_R5_G6_B5) {
         return formats[i];
      }
   }
   return PIPE_FORMAT_NONE;
}


/**
 * Search list of formats for first depth/Z format.
 */
static GLuint
default_depth_format(const GLuint formats[], GLuint num)
{
   GLuint i;
   for (i = 0; i < num; i++) {
      if (formats[i] == PIPE_FORMAT_U_Z16 ||
          formats[i] == PIPE_FORMAT_U_Z32 ||
          formats[i] == PIPE_FORMAT_S8_Z24) {
         return formats[i];
      }
   }
   return PIPE_FORMAT_NONE;
}


/**
 * Choose the PIPE_FORMAT_ to use for storing a texture image based
 * on the user's internalFormat, format and type parameters.
 * We query the pipe device for a list of formats which it supports
 * and choose from them.
 * If we find a device that needs a more intricate selection mechanism,
 * this function _could_ get pushed down into the pipe device.
 *
 * Note: also used for glRenderbufferStorageEXT()
 *
 * Note: format and type may be GL_NONE (see renderbuffers)
 *
 * \return PIPE_FORMAT_NONE if error/problem.
 */
GLuint
st_choose_pipe_format(struct pipe_context *pipe, GLint internalFormat,
                      GLenum format, GLenum type)
{
   const GLuint *supported;
   GLboolean allow[PIPE_FORMAT_COUNT];
   GLuint i, n;

   /* query supported formats and fill in bool allow[] table */
   supported = pipe->supported_formats(pipe, &n);
   assert(n < PIPE_FORMAT_COUNT); /* sanity check */
   memset(allow, 0, sizeof(allow));
   for (i = 0; i < n; i++) {
      allow[supported[i]] = 1;
   }

   switch (internalFormat) {
   case 4:
   case GL_RGBA:
   case GL_COMPRESSED_RGBA:
      if (format == GL_BGRA) {
         if (type == GL_UNSIGNED_BYTE || type == GL_UNSIGNED_INT_8_8_8_8_REV) {
            if (allow[PIPE_FORMAT_U_A8_R8_G8_B8])
               return PIPE_FORMAT_U_A8_R8_G8_B8;
         }
         else if (type == GL_UNSIGNED_SHORT_4_4_4_4_REV) {
            if (allow[PIPE_FORMAT_U_A4_R4_G4_B4])
               return PIPE_FORMAT_U_A4_R4_G4_B4;
         }
         else if (type == GL_UNSIGNED_SHORT_1_5_5_5_REV) {
            if (allow[PIPE_FORMAT_U_A1_R5_G5_B5])
               return PIPE_FORMAT_U_A1_R5_G5_B5;
         }
      }
      return default_rgba_format(supported, n);

   case 3:
   case GL_RGB:
   case GL_COMPRESSED_RGB:
      if (format == GL_RGB && type == GL_UNSIGNED_SHORT_5_6_5) {
         if (allow[PIPE_FORMAT_U_R5_G6_B5])
            return PIPE_FORMAT_U_R5_G6_B5;
      }
      return default_rgba_format(supported, n);

   case GL_RGBA8:
   case GL_RGB10_A2:
   case GL_RGBA12:
   case GL_RGBA16:
      return default_rgba_format(supported, n);

   case GL_RGBA4:
   case GL_RGBA2:
      if (allow[PIPE_FORMAT_U_A4_R4_G4_B4])
         return PIPE_FORMAT_U_A4_R4_G4_B4;
      return default_rgba_format(supported, n);

   case GL_RGB5_A1:
      if (allow[PIPE_FORMAT_U_A1_R5_G5_B5])
         return PIPE_FORMAT_U_A1_R5_G5_B5;
      return default_rgba_format(supported, n);

   case GL_RGB8:
   case GL_RGB10:
   case GL_RGB12:
   case GL_RGB16:
      return default_rgba_format(supported, n);

   case GL_RGB5:
   case GL_RGB4:
   case GL_R3_G3_B2:
      if (allow[PIPE_FORMAT_U_A1_R5_G5_B5])
         return PIPE_FORMAT_U_A1_R5_G5_B5;
      return default_rgba_format(supported, n);

   case GL_ALPHA:
   case GL_ALPHA4:
   case GL_ALPHA8:
   case GL_ALPHA12:
   case GL_ALPHA16:
   case GL_COMPRESSED_ALPHA:
      if (allow[PIPE_FORMAT_U_A8])
         return PIPE_FORMAT_U_A8;
      return default_rgba_format(supported, n);

   case 1:
   case GL_LUMINANCE:
   case GL_LUMINANCE4:
   case GL_LUMINANCE8:
   case GL_LUMINANCE12:
   case GL_LUMINANCE16:
   case GL_COMPRESSED_LUMINANCE:
      if (allow[PIPE_FORMAT_U_A8])
         return PIPE_FORMAT_U_A8;
      return default_rgba_format(supported, n);

   case 2:
   case GL_LUMINANCE_ALPHA:
   case GL_LUMINANCE4_ALPHA4:
   case GL_LUMINANCE6_ALPHA2:
   case GL_LUMINANCE8_ALPHA8:
   case GL_LUMINANCE12_ALPHA4:
   case GL_LUMINANCE12_ALPHA12:
   case GL_LUMINANCE16_ALPHA16:
   case GL_COMPRESSED_LUMINANCE_ALPHA:
      if (allow[PIPE_FORMAT_U_A8_L8])
         return PIPE_FORMAT_U_A8_L8;
      return default_rgba_format(supported, n);

   case GL_INTENSITY:
   case GL_INTENSITY4:
   case GL_INTENSITY8:
   case GL_INTENSITY12:
   case GL_INTENSITY16:
   case GL_COMPRESSED_INTENSITY:
      if (allow[PIPE_FORMAT_U_I8])
         return PIPE_FORMAT_U_I8;
      return default_rgba_format(supported, n);

   case GL_YCBCR_MESA:
      if (type == GL_UNSIGNED_SHORT_8_8_MESA || type == GL_UNSIGNED_BYTE) {
         if (allow[PIPE_FORMAT_YCBCR])
            return PIPE_FORMAT_YCBCR;
      }
      else {
         if (allow[PIPE_FORMAT_YCBCR_REV])
            return PIPE_FORMAT_YCBCR_REV;
      }
      return PIPE_FORMAT_NONE;

#if 0
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
      if (allow[PIPE_FORMAT_U_Z16])
         return PIPE_FORMAT_U_Z16;
      /* fall-through */
   case GL_DEPTH_COMPONENT24:
      if (allow[PIPE_FORMAT_S8_Z24])
         return PIPE_FORMAT_S8_Z24;
      /* fall-through */
   case GL_DEPTH_COMPONENT32:
      if (allow[PIPE_FORMAT_U_Z32])
         return PIPE_FORMAT_U_Z32;
      /* fall-through */
   case GL_DEPTH_COMPONENT:
      return default_depth_format(supported, n);

   case GL_STENCIL_INDEX:
   case GL_STENCIL_INDEX1_EXT:
   case GL_STENCIL_INDEX4_EXT:
   case GL_STENCIL_INDEX8_EXT:
   case GL_STENCIL_INDEX16_EXT:
      if (allow[PIPE_FORMAT_U_S8])
         return PIPE_FORMAT_U_S8;
      if (allow[PIPE_FORMAT_S8_Z24])
         return PIPE_FORMAT_S8_Z24;
      return PIPE_FORMAT_NONE;

   case GL_DEPTH_STENCIL_EXT:
   case GL_DEPTH24_STENCIL8_EXT:
      if (allow[PIPE_FORMAT_S8_Z24])
         return PIPE_FORMAT_S8_Z24;
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

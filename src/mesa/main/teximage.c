/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
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
 * \file teximage.c
 * Texture image-related functions.
 */

#include <stdbool.h>
#include "glheader.h"
#include "bufferobj.h"
#include "context.h"
#include "enums.h"
#include "fbobject.h"
#include "framebuffer.h"
#include "hash.h"
#include "image.h"
#include "imports.h"
#include "macros.h"
#include "mfeatures.h"
#include "multisample.h"
#include "state.h"
#include "texcompress.h"
#include "texcompress_cpal.h"
#include "teximage.h"
#include "texobj.h"
#include "texstate.h"
#include "texstorage.h"
#include "mtypes.h"
#include "glformats.h"


/**
 * State changes which we care about for glCopyTex[Sub]Image() calls.
 * In particular, we care about pixel transfer state and buffer state
 * (such as glReadBuffer to make sure we read from the right renderbuffer).
 */
#define NEW_COPY_TEX_STATE (_NEW_BUFFERS | _NEW_PIXEL)



/**
 * Return the simple base format for a given internal texture format.
 * For example, given GL_LUMINANCE12_ALPHA4, return GL_LUMINANCE_ALPHA.
 *
 * \param ctx GL context.
 * \param internalFormat the internal texture format token or 1, 2, 3, or 4.
 *
 * \return the corresponding \u base internal format (GL_ALPHA, GL_LUMINANCE,
 * GL_LUMANCE_ALPHA, GL_INTENSITY, GL_RGB, or GL_RGBA), or -1 if invalid enum.
 *
 * This is the format which is used during texture application (i.e. the
 * texture format and env mode determine the arithmetic used.
 */
GLint
_mesa_base_tex_format( struct gl_context *ctx, GLint internalFormat )
{
   switch (internalFormat) {
      case GL_ALPHA:
      case GL_ALPHA4:
      case GL_ALPHA8:
      case GL_ALPHA12:
      case GL_ALPHA16:
         return (ctx->API != API_OPENGL_CORE) ? GL_ALPHA : -1;
      case 1:
      case GL_LUMINANCE:
      case GL_LUMINANCE4:
      case GL_LUMINANCE8:
      case GL_LUMINANCE12:
      case GL_LUMINANCE16:
         return (ctx->API != API_OPENGL_CORE) ? GL_LUMINANCE : -1;
      case 2:
      case GL_LUMINANCE_ALPHA:
      case GL_LUMINANCE4_ALPHA4:
      case GL_LUMINANCE6_ALPHA2:
      case GL_LUMINANCE8_ALPHA8:
      case GL_LUMINANCE12_ALPHA4:
      case GL_LUMINANCE12_ALPHA12:
      case GL_LUMINANCE16_ALPHA16:
         return (ctx->API != API_OPENGL_CORE) ? GL_LUMINANCE_ALPHA : -1;
      case GL_INTENSITY:
      case GL_INTENSITY4:
      case GL_INTENSITY8:
      case GL_INTENSITY12:
      case GL_INTENSITY16:
         return (ctx->API != API_OPENGL_CORE) ? GL_INTENSITY : -1;
      case 3:
         return (ctx->API != API_OPENGL_CORE) ? GL_RGB : -1;
      case GL_RGB:
      case GL_R3_G3_B2:
      case GL_RGB4:
      case GL_RGB5:
      case GL_RGB8:
      case GL_RGB10:
      case GL_RGB12:
      case GL_RGB16:
         return GL_RGB;
      case 4:
         return (ctx->API != API_OPENGL_CORE) ? GL_RGBA : -1;
      case GL_RGBA:
      case GL_RGBA2:
      case GL_RGBA4:
      case GL_RGB5_A1:
      case GL_RGBA8:
      case GL_RGB10_A2:
      case GL_RGBA12:
      case GL_RGBA16:
         return GL_RGBA;
      default:
         ; /* fallthrough */
   }

   /* GL_BGRA can be an internal format *only* in OpenGL ES (1.x or 2.0).
    */
   if (_mesa_is_gles(ctx)) {
      switch (internalFormat) {
         case GL_BGRA:
            return GL_RGBA;
         default:
            ; /* fallthrough */
      }
   }

   if (ctx->Extensions.ARB_ES2_compatibility) {
      switch (internalFormat) {
         case GL_RGB565:
            return GL_RGB;
         default:
            ; /* fallthrough */
      }
   }

   if (ctx->Extensions.ARB_depth_texture) {
      switch (internalFormat) {
         case GL_DEPTH_COMPONENT:
         case GL_DEPTH_COMPONENT16:
         case GL_DEPTH_COMPONENT24:
         case GL_DEPTH_COMPONENT32:
            return GL_DEPTH_COMPONENT;
         default:
            ; /* fallthrough */
      }
   }

   switch (internalFormat) {
   case GL_COMPRESSED_ALPHA:
      return GL_ALPHA;
   case GL_COMPRESSED_LUMINANCE:
      return GL_LUMINANCE;
   case GL_COMPRESSED_LUMINANCE_ALPHA:
      return GL_LUMINANCE_ALPHA;
   case GL_COMPRESSED_INTENSITY:
      return GL_INTENSITY;
   case GL_COMPRESSED_RGB:
      return GL_RGB;
   case GL_COMPRESSED_RGBA:
      return GL_RGBA;
   default:
      ; /* fallthrough */
   }
         
   if (ctx->Extensions.TDFX_texture_compression_FXT1) {
      switch (internalFormat) {
         case GL_COMPRESSED_RGB_FXT1_3DFX:
            return GL_RGB;
         case GL_COMPRESSED_RGBA_FXT1_3DFX:
            return GL_RGBA;
         default:
            ; /* fallthrough */
      }
   }

   /* Assume that the ANGLE flag will always be set if the EXT flag is set.
    */
   if (ctx->Extensions.ANGLE_texture_compression_dxt) {
      switch (internalFormat) {
         case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
            return GL_RGB;
         case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
         case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
         case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
            return GL_RGBA;
         default:
            ; /* fallthrough */
      }
   }

   if (_mesa_is_desktop_gl(ctx)
       && ctx->Extensions.ANGLE_texture_compression_dxt) {
      switch (internalFormat) {
         case GL_RGB_S3TC:
         case GL_RGB4_S3TC:
            return GL_RGB;
         case GL_RGBA_S3TC:
         case GL_RGBA4_S3TC:
            return GL_RGBA;
         default:
            ; /* fallthrough */
      }
   }

   if (ctx->Extensions.MESA_ycbcr_texture) {
      if (internalFormat == GL_YCBCR_MESA)
         return GL_YCBCR_MESA;
   }

   if (ctx->Extensions.ARB_texture_float) {
      switch (internalFormat) {
         case GL_ALPHA16F_ARB:
         case GL_ALPHA32F_ARB:
            return GL_ALPHA;
         case GL_RGBA16F_ARB:
         case GL_RGBA32F_ARB:
            return GL_RGBA;
         case GL_RGB16F_ARB:
         case GL_RGB32F_ARB:
            return GL_RGB;
         case GL_INTENSITY16F_ARB:
         case GL_INTENSITY32F_ARB:
            return GL_INTENSITY;
         case GL_LUMINANCE16F_ARB:
         case GL_LUMINANCE32F_ARB:
            return GL_LUMINANCE;
         case GL_LUMINANCE_ALPHA16F_ARB:
         case GL_LUMINANCE_ALPHA32F_ARB:
            return GL_LUMINANCE_ALPHA;
         default:
            ; /* fallthrough */
      }
   }

   if (ctx->Extensions.ATI_envmap_bumpmap) {
      switch (internalFormat) {
         case GL_DUDV_ATI:
         case GL_DU8DV8_ATI:
            return GL_DUDV_ATI;
         default:
            ; /* fallthrough */
      }
   }

   if (ctx->Extensions.EXT_texture_snorm) {
      switch (internalFormat) {
         case GL_RED_SNORM:
         case GL_R8_SNORM:
         case GL_R16_SNORM:
            return GL_RED;
         case GL_RG_SNORM:
         case GL_RG8_SNORM:
         case GL_RG16_SNORM:
            return GL_RG;
         case GL_RGB_SNORM:
         case GL_RGB8_SNORM:
         case GL_RGB16_SNORM:
            return GL_RGB;
         case GL_RGBA_SNORM:
         case GL_RGBA8_SNORM:
         case GL_RGBA16_SNORM:
            return GL_RGBA;
         case GL_ALPHA_SNORM:
         case GL_ALPHA8_SNORM:
         case GL_ALPHA16_SNORM:
            return GL_ALPHA;
         case GL_LUMINANCE_SNORM:
         case GL_LUMINANCE8_SNORM:
         case GL_LUMINANCE16_SNORM:
            return GL_LUMINANCE;
         case GL_LUMINANCE_ALPHA_SNORM:
         case GL_LUMINANCE8_ALPHA8_SNORM:
         case GL_LUMINANCE16_ALPHA16_SNORM:
            return GL_LUMINANCE_ALPHA;
         case GL_INTENSITY_SNORM:
         case GL_INTENSITY8_SNORM:
         case GL_INTENSITY16_SNORM:
            return GL_INTENSITY;
         default:
            ; /* fallthrough */
      }
   }

   if (ctx->Extensions.EXT_packed_depth_stencil) {
      switch (internalFormat) {
         case GL_DEPTH_STENCIL_EXT:
         case GL_DEPTH24_STENCIL8_EXT:
            return GL_DEPTH_STENCIL_EXT;
         default:
            ; /* fallthrough */
      }
   }

   if (ctx->Extensions.EXT_texture_sRGB) {
      switch (internalFormat) {
      case GL_SRGB_EXT:
      case GL_SRGB8_EXT:
      case GL_COMPRESSED_SRGB_EXT:
      case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
         return GL_RGB;
      case GL_SRGB_ALPHA_EXT:
      case GL_SRGB8_ALPHA8_EXT:
      case GL_COMPRESSED_SRGB_ALPHA_EXT:
      case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
      case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
      case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
         return GL_RGBA;
      case GL_SLUMINANCE_ALPHA_EXT:
      case GL_SLUMINANCE8_ALPHA8_EXT:
      case GL_COMPRESSED_SLUMINANCE_ALPHA_EXT:
         return GL_LUMINANCE_ALPHA;
      case GL_SLUMINANCE_EXT:
      case GL_SLUMINANCE8_EXT:
      case GL_COMPRESSED_SLUMINANCE_EXT:
         return GL_LUMINANCE;
      default:
         ; /* fallthrough */
      }
   }

   if (ctx->Version >= 30 ||
       ctx->Extensions.EXT_texture_integer) {
      switch (internalFormat) {
      case GL_RGBA8UI_EXT:
      case GL_RGBA16UI_EXT:
      case GL_RGBA32UI_EXT:
      case GL_RGBA8I_EXT:
      case GL_RGBA16I_EXT:
      case GL_RGBA32I_EXT:
      case GL_RGB10_A2UI:
         return GL_RGBA;
      case GL_RGB8UI_EXT:
      case GL_RGB16UI_EXT:
      case GL_RGB32UI_EXT:
      case GL_RGB8I_EXT:
      case GL_RGB16I_EXT:
      case GL_RGB32I_EXT:
         return GL_RGB;
      }
   }

   if (ctx->Extensions.EXT_texture_integer) {
      switch (internalFormat) {
      case GL_ALPHA8UI_EXT:
      case GL_ALPHA16UI_EXT:
      case GL_ALPHA32UI_EXT:
      case GL_ALPHA8I_EXT:
      case GL_ALPHA16I_EXT:
      case GL_ALPHA32I_EXT:
         return GL_ALPHA;
      case GL_INTENSITY8UI_EXT:
      case GL_INTENSITY16UI_EXT:
      case GL_INTENSITY32UI_EXT:
      case GL_INTENSITY8I_EXT:
      case GL_INTENSITY16I_EXT:
      case GL_INTENSITY32I_EXT:
         return GL_INTENSITY;
      case GL_LUMINANCE8UI_EXT:
      case GL_LUMINANCE16UI_EXT:
      case GL_LUMINANCE32UI_EXT:
      case GL_LUMINANCE8I_EXT:
      case GL_LUMINANCE16I_EXT:
      case GL_LUMINANCE32I_EXT:
         return GL_LUMINANCE;
      case GL_LUMINANCE_ALPHA8UI_EXT:
      case GL_LUMINANCE_ALPHA16UI_EXT:
      case GL_LUMINANCE_ALPHA32UI_EXT:
      case GL_LUMINANCE_ALPHA8I_EXT:
      case GL_LUMINANCE_ALPHA16I_EXT:
      case GL_LUMINANCE_ALPHA32I_EXT:
         return GL_LUMINANCE_ALPHA;
      default:
         ; /* fallthrough */
      }
   }

   if (ctx->Extensions.ARB_texture_rg) {
      switch (internalFormat) {
      case GL_R16F:
	 /* R16F depends on both ARB_half_float_pixel and ARB_texture_float.
	  */
	 if (!ctx->Extensions.ARB_half_float_pixel)
	    break;
	 /* FALLTHROUGH */
      case GL_R32F:
	 if (!ctx->Extensions.ARB_texture_float)
	    break;
         return GL_RED;
      case GL_R8I:
      case GL_R8UI:
      case GL_R16I:
      case GL_R16UI:
      case GL_R32I:
      case GL_R32UI:
	 if (ctx->Version < 30 && !ctx->Extensions.EXT_texture_integer)
	    break;
	 /* FALLTHROUGH */
      case GL_R8:
      case GL_R16:
      case GL_RED:
      case GL_COMPRESSED_RED:
         return GL_RED;

      case GL_RG16F:
	 /* RG16F depends on both ARB_half_float_pixel and ARB_texture_float.
	  */
	 if (!ctx->Extensions.ARB_half_float_pixel)
	    break;
	 /* FALLTHROUGH */
      case GL_RG32F:
	 if (!ctx->Extensions.ARB_texture_float)
	    break;
         return GL_RG;
      case GL_RG8I:
      case GL_RG8UI:
      case GL_RG16I:
      case GL_RG16UI:
      case GL_RG32I:
      case GL_RG32UI:
	 if (ctx->Version < 30 && !ctx->Extensions.EXT_texture_integer)
	    break;
	 /* FALLTHROUGH */
      case GL_RG:
      case GL_RG8:
      case GL_RG16:
      case GL_COMPRESSED_RG:
         return GL_RG;
      default:
         ; /* fallthrough */
      }
   }

   if (ctx->Extensions.EXT_texture_shared_exponent) {
      switch (internalFormat) {
      case GL_RGB9_E5_EXT:
         return GL_RGB;
      default:
         ; /* fallthrough */
      }
   }

   if (ctx->Extensions.EXT_packed_float) {
      switch (internalFormat) {
      case GL_R11F_G11F_B10F_EXT:
         return GL_RGB;
      default:
         ; /* fallthrough */
      }
   }

   if (ctx->Extensions.ARB_depth_buffer_float) {
      switch (internalFormat) {
      case GL_DEPTH_COMPONENT32F:
         return GL_DEPTH_COMPONENT;
      case GL_DEPTH32F_STENCIL8:
         return GL_DEPTH_STENCIL;
      default:
         ; /* fallthrough */
      }
   }

   if (ctx->Extensions.ARB_texture_compression_rgtc) {
      switch (internalFormat) {
      case GL_COMPRESSED_RED_RGTC1:
      case GL_COMPRESSED_SIGNED_RED_RGTC1:
         return GL_RED;
      case GL_COMPRESSED_RG_RGTC2:
      case GL_COMPRESSED_SIGNED_RG_RGTC2:
         return GL_RG;
      default:
         ; /* fallthrough */
      }
   }

   if (ctx->Extensions.EXT_texture_compression_latc) {
      switch (internalFormat) {
      case GL_COMPRESSED_LUMINANCE_LATC1_EXT:
      case GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT:
         return GL_LUMINANCE;
      case GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT:
      case GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT:
         return GL_LUMINANCE_ALPHA;
      default:
         ; /* fallthrough */
      }
   }

   if (ctx->Extensions.ATI_texture_compression_3dc) {
      switch (internalFormat) {
      case GL_COMPRESSED_LUMINANCE_ALPHA_3DC_ATI:
         return GL_LUMINANCE_ALPHA;
      default:
         ; /* fallthrough */
      }
   }

   if (ctx->Extensions.OES_compressed_ETC1_RGB8_texture) {
      switch (internalFormat) {
      case GL_ETC1_RGB8_OES:
         return GL_RGB;
      default:
         ; /* fallthrough */
      }
   }

   if (_mesa_is_gles3(ctx) || ctx->Extensions.ARB_ES3_compatibility) {
      switch (internalFormat) {
      case GL_COMPRESSED_RGB8_ETC2:
      case GL_COMPRESSED_SRGB8_ETC2:
         return GL_RGB;
      case GL_COMPRESSED_RGBA8_ETC2_EAC:
      case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:
      case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
      case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
         return GL_RGBA;
      case GL_COMPRESSED_R11_EAC:
      case GL_COMPRESSED_SIGNED_R11_EAC:
         return GL_RED;
      case GL_COMPRESSED_RG11_EAC:
      case GL_COMPRESSED_SIGNED_RG11_EAC:
         return GL_RG;
      default:
         ; /* fallthrough */
      }
   }

   if (ctx->API == API_OPENGLES) {
      switch (internalFormat) {
      case GL_PALETTE4_RGB8_OES:
      case GL_PALETTE4_R5_G6_B5_OES:
      case GL_PALETTE8_RGB8_OES:
      case GL_PALETTE8_R5_G6_B5_OES:
	 return GL_RGB;
      case GL_PALETTE4_RGBA8_OES:
      case GL_PALETTE8_RGB5_A1_OES:
      case GL_PALETTE4_RGBA4_OES:
      case GL_PALETTE4_RGB5_A1_OES:
      case GL_PALETTE8_RGBA8_OES:
      case GL_PALETTE8_RGBA4_OES:
	 return GL_RGBA;
      default:
         ; /* fallthrough */
      }
   }

   return -1; /* error */
}


/**
 * For cube map faces, return a face index in [0,5].
 * For other targets return 0;
 */
GLuint
_mesa_tex_target_to_face(GLenum target)
{
   if (_mesa_is_cube_face(target))
      return (GLuint) target - (GLuint) GL_TEXTURE_CUBE_MAP_POSITIVE_X;
   else
      return 0;
}



/**
 * Install gl_texture_image in a gl_texture_object according to the target
 * and level parameters.
 * 
 * \param tObj texture object.
 * \param target texture target.
 * \param level image level.
 * \param texImage texture image.
 */
static void
set_tex_image(struct gl_texture_object *tObj,
              GLenum target, GLint level,
              struct gl_texture_image *texImage)
{
   const GLuint face = _mesa_tex_target_to_face(target);

   ASSERT(tObj);
   ASSERT(texImage);
   if (target == GL_TEXTURE_RECTANGLE_NV || target == GL_TEXTURE_EXTERNAL_OES)
      assert(level == 0);

   tObj->Image[face][level] = texImage;

   /* Set the 'back' pointer */
   texImage->TexObject = tObj;
   texImage->Level = level;
   texImage->Face = face;
}


/**
 * Allocate a texture image structure.
 * 
 * Called via ctx->Driver.NewTextureImage() unless overriden by a device
 * driver.
 *
 * \return a pointer to gl_texture_image struct with all fields initialized to
 * zero.
 */
struct gl_texture_image *
_mesa_new_texture_image( struct gl_context *ctx )
{
   (void) ctx;
   return CALLOC_STRUCT(gl_texture_image);
}


/**
 * Free a gl_texture_image and associated data.
 * This function is a fallback called via ctx->Driver.DeleteTextureImage().
 *
 * \param texImage texture image.
 *
 * Free the texture image structure and the associated image data.
 */
void
_mesa_delete_texture_image(struct gl_context *ctx,
                           struct gl_texture_image *texImage)
{
   /* Free texImage->Data and/or any other driver-specific texture
    * image storage.
    */
   ASSERT(ctx->Driver.FreeTextureImageBuffer);
   ctx->Driver.FreeTextureImageBuffer( ctx, texImage );
   free(texImage);
}


/**
 * Test if a target is a proxy target.
 *
 * \param target texture target.
 *
 * \return GL_TRUE if the target is a proxy target, GL_FALSE otherwise.
 */
GLboolean
_mesa_is_proxy_texture(GLenum target)
{
   /*
    * NUM_TEXTURE_TARGETS should match number of terms below, except there's no
    * proxy for GL_TEXTURE_BUFFER and GL_TEXTURE_EXTERNAL_OES.
    */
   assert(NUM_TEXTURE_TARGETS == 10 + 2);

   return (target == GL_PROXY_TEXTURE_1D ||
           target == GL_PROXY_TEXTURE_2D ||
           target == GL_PROXY_TEXTURE_3D ||
           target == GL_PROXY_TEXTURE_CUBE_MAP_ARB ||
           target == GL_PROXY_TEXTURE_RECTANGLE_NV ||
           target == GL_PROXY_TEXTURE_1D_ARRAY_EXT ||
           target == GL_PROXY_TEXTURE_2D_ARRAY_EXT ||
           target == GL_PROXY_TEXTURE_CUBE_MAP_ARRAY ||
           target == GL_PROXY_TEXTURE_2D_MULTISAMPLE ||
           target == GL_PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY);
}


/**
 * Return the proxy target which corresponds to the given texture target
 */
GLenum
_mesa_get_proxy_target(GLenum target)
{
   switch (target) {
   case GL_TEXTURE_1D:
   case GL_PROXY_TEXTURE_1D:
      return GL_PROXY_TEXTURE_1D;
   case GL_TEXTURE_2D:
   case GL_PROXY_TEXTURE_2D:
      return GL_PROXY_TEXTURE_2D;
   case GL_TEXTURE_3D:
   case GL_PROXY_TEXTURE_3D:
      return GL_PROXY_TEXTURE_3D;
   case GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB:
   case GL_TEXTURE_CUBE_MAP_ARB:
   case GL_PROXY_TEXTURE_CUBE_MAP_ARB:
      return GL_PROXY_TEXTURE_CUBE_MAP_ARB;
   case GL_TEXTURE_RECTANGLE_NV:
   case GL_PROXY_TEXTURE_RECTANGLE_NV:
      return GL_PROXY_TEXTURE_RECTANGLE_NV;
   case GL_TEXTURE_1D_ARRAY_EXT:
   case GL_PROXY_TEXTURE_1D_ARRAY_EXT:
      return GL_PROXY_TEXTURE_1D_ARRAY_EXT;
   case GL_TEXTURE_2D_ARRAY_EXT:
   case GL_PROXY_TEXTURE_2D_ARRAY_EXT:
      return GL_PROXY_TEXTURE_2D_ARRAY_EXT;
   case GL_TEXTURE_CUBE_MAP_ARRAY:
   case GL_PROXY_TEXTURE_CUBE_MAP_ARRAY:
      return GL_PROXY_TEXTURE_CUBE_MAP_ARRAY;
   case GL_TEXTURE_2D_MULTISAMPLE:
   case GL_PROXY_TEXTURE_2D_MULTISAMPLE:
      return GL_PROXY_TEXTURE_2D_MULTISAMPLE;
   case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
   case GL_PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY:
      return GL_PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY;
   default:
      _mesa_problem(NULL, "unexpected target in _mesa_get_proxy_target()");
      return 0;
   }
}


/**
 * Get the texture object that corresponds to the target of the given
 * texture unit.  The target should have already been checked for validity.
 *
 * \param ctx GL context.
 * \param texUnit texture unit.
 * \param target texture target.
 *
 * \return pointer to the texture object on success, or NULL on failure.
 */
struct gl_texture_object *
_mesa_select_tex_object(struct gl_context *ctx,
                        const struct gl_texture_unit *texUnit,
                        GLenum target)
{
   const GLboolean arrayTex = (ctx->Extensions.MESA_texture_array ||
                               ctx->Extensions.EXT_texture_array);

   switch (target) {
      case GL_TEXTURE_1D:
         return texUnit->CurrentTex[TEXTURE_1D_INDEX];
      case GL_PROXY_TEXTURE_1D:
         return ctx->Texture.ProxyTex[TEXTURE_1D_INDEX];
      case GL_TEXTURE_2D:
         return texUnit->CurrentTex[TEXTURE_2D_INDEX];
      case GL_PROXY_TEXTURE_2D:
         return ctx->Texture.ProxyTex[TEXTURE_2D_INDEX];
      case GL_TEXTURE_3D:
         return texUnit->CurrentTex[TEXTURE_3D_INDEX];
      case GL_PROXY_TEXTURE_3D:
         return ctx->Texture.ProxyTex[TEXTURE_3D_INDEX];
      case GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB:
      case GL_TEXTURE_CUBE_MAP_ARB:
         return ctx->Extensions.ARB_texture_cube_map
                ? texUnit->CurrentTex[TEXTURE_CUBE_INDEX] : NULL;
      case GL_PROXY_TEXTURE_CUBE_MAP_ARB:
         return ctx->Extensions.ARB_texture_cube_map
                ? ctx->Texture.ProxyTex[TEXTURE_CUBE_INDEX] : NULL;
      case GL_TEXTURE_CUBE_MAP_ARRAY:
         return ctx->Extensions.ARB_texture_cube_map_array
                ? texUnit->CurrentTex[TEXTURE_CUBE_ARRAY_INDEX] : NULL;
      case GL_PROXY_TEXTURE_CUBE_MAP_ARRAY:
         return ctx->Extensions.ARB_texture_cube_map_array
                ? ctx->Texture.ProxyTex[TEXTURE_CUBE_ARRAY_INDEX] : NULL;
      case GL_TEXTURE_RECTANGLE_NV:
         return ctx->Extensions.NV_texture_rectangle
                ? texUnit->CurrentTex[TEXTURE_RECT_INDEX] : NULL;
      case GL_PROXY_TEXTURE_RECTANGLE_NV:
         return ctx->Extensions.NV_texture_rectangle
                ? ctx->Texture.ProxyTex[TEXTURE_RECT_INDEX] : NULL;
      case GL_TEXTURE_1D_ARRAY_EXT:
         return arrayTex ? texUnit->CurrentTex[TEXTURE_1D_ARRAY_INDEX] : NULL;
      case GL_PROXY_TEXTURE_1D_ARRAY_EXT:
         return arrayTex ? ctx->Texture.ProxyTex[TEXTURE_1D_ARRAY_INDEX] : NULL;
      case GL_TEXTURE_2D_ARRAY_EXT:
         return arrayTex ? texUnit->CurrentTex[TEXTURE_2D_ARRAY_INDEX] : NULL;
      case GL_PROXY_TEXTURE_2D_ARRAY_EXT:
         return arrayTex ? ctx->Texture.ProxyTex[TEXTURE_2D_ARRAY_INDEX] : NULL;
      case GL_TEXTURE_BUFFER:
         return ctx->API == API_OPENGL_CORE &&
                ctx->Extensions.ARB_texture_buffer_object ?
                texUnit->CurrentTex[TEXTURE_BUFFER_INDEX] : NULL;
      case GL_TEXTURE_EXTERNAL_OES:
         return ctx->Extensions.OES_EGL_image_external
            ? texUnit->CurrentTex[TEXTURE_EXTERNAL_INDEX] : NULL;
      case GL_TEXTURE_2D_MULTISAMPLE:
         return ctx->Extensions.ARB_texture_multisample
            ? texUnit->CurrentTex[TEXTURE_2D_MULTISAMPLE_INDEX] : NULL;
      case GL_PROXY_TEXTURE_2D_MULTISAMPLE:
         return ctx->Extensions.ARB_texture_multisample
            ? ctx->Texture.ProxyTex[TEXTURE_2D_MULTISAMPLE_INDEX] : NULL;
      case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
         return ctx->Extensions.ARB_texture_multisample
            ? texUnit->CurrentTex[TEXTURE_2D_MULTISAMPLE_ARRAY_INDEX] : NULL;
      case GL_PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY:
         return ctx->Extensions.ARB_texture_multisample
            ? ctx->Texture.ProxyTex[TEXTURE_2D_MULTISAMPLE_ARRAY_INDEX] : NULL;
      default:
         _mesa_problem(NULL, "bad target in _mesa_select_tex_object()");
         return NULL;
   }
}


/**
 * Return pointer to texture object for given target on current texture unit.
 */
struct gl_texture_object *
_mesa_get_current_tex_object(struct gl_context *ctx, GLenum target)
{
   struct gl_texture_unit *texUnit = _mesa_get_current_tex_unit(ctx);
   return _mesa_select_tex_object(ctx, texUnit, target);
}


/**
 * Get a texture image pointer from a texture object, given a texture
 * target and mipmap level.  The target and level parameters should
 * have already been error-checked.
 *
 * \param ctx GL context.
 * \param texObj texture unit.
 * \param target texture target.
 * \param level image level.
 *
 * \return pointer to the texture image structure, or NULL on failure.
 */
struct gl_texture_image *
_mesa_select_tex_image(struct gl_context *ctx,
                       const struct gl_texture_object *texObj,
		       GLenum target, GLint level)
{
   const GLuint face = _mesa_tex_target_to_face(target);

   ASSERT(texObj);
   ASSERT(level >= 0);
   ASSERT(level < MAX_TEXTURE_LEVELS);

   return texObj->Image[face][level];
}


/**
 * Like _mesa_select_tex_image() but if the image doesn't exist, allocate
 * it and install it.  Only return NULL if passed a bad parameter or run
 * out of memory.
 */
struct gl_texture_image *
_mesa_get_tex_image(struct gl_context *ctx, struct gl_texture_object *texObj,
                    GLenum target, GLint level)
{
   struct gl_texture_image *texImage;

   if (!texObj)
      return NULL;
   
   texImage = _mesa_select_tex_image(ctx, texObj, target, level);
   if (!texImage) {
      texImage = ctx->Driver.NewTextureImage(ctx);
      if (!texImage) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "texture image allocation");
         return NULL;
      }

      set_tex_image(texObj, target, level, texImage);
   }

   return texImage;
}


/**
 * Return pointer to the specified proxy texture image.
 * Note that proxy textures are per-context, not per-texture unit.
 * \return pointer to texture image or NULL if invalid target, invalid
 *         level, or out of memory.
 */
static struct gl_texture_image *
get_proxy_tex_image(struct gl_context *ctx, GLenum target, GLint level)
{
   struct gl_texture_image *texImage;
   GLuint texIndex;

   if (level < 0)
      return NULL;

   switch (target) {
   case GL_PROXY_TEXTURE_1D:
      if (level >= ctx->Const.MaxTextureLevels)
         return NULL;
      texIndex = TEXTURE_1D_INDEX;
      break;
   case GL_PROXY_TEXTURE_2D:
      if (level >= ctx->Const.MaxTextureLevels)
         return NULL;
      texIndex = TEXTURE_2D_INDEX;
      break;
   case GL_PROXY_TEXTURE_3D:
      if (level >= ctx->Const.Max3DTextureLevels)
         return NULL;
      texIndex = TEXTURE_3D_INDEX;
      break;
   case GL_PROXY_TEXTURE_CUBE_MAP:
      if (level >= ctx->Const.MaxCubeTextureLevels)
         return NULL;
      texIndex = TEXTURE_CUBE_INDEX;
      break;
   case GL_PROXY_TEXTURE_RECTANGLE_NV:
      if (level > 0)
         return NULL;
      texIndex = TEXTURE_RECT_INDEX;
      break;
   case GL_PROXY_TEXTURE_1D_ARRAY_EXT:
      if (level >= ctx->Const.MaxTextureLevels)
         return NULL;
      texIndex = TEXTURE_1D_ARRAY_INDEX;
      break;
   case GL_PROXY_TEXTURE_2D_ARRAY_EXT:
      if (level >= ctx->Const.MaxTextureLevels)
         return NULL;
      texIndex = TEXTURE_2D_ARRAY_INDEX;
      break;
   case GL_PROXY_TEXTURE_CUBE_MAP_ARRAY:
      if (level >= ctx->Const.MaxCubeTextureLevels)
         return NULL;
      texIndex = TEXTURE_CUBE_ARRAY_INDEX;
      break;
   case GL_PROXY_TEXTURE_2D_MULTISAMPLE:
      if (level > 0)
         return 0;
      texIndex = TEXTURE_2D_MULTISAMPLE_INDEX;
      break;
   case GL_PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY:
      if (level > 0)
         return 0;
      texIndex = TEXTURE_2D_MULTISAMPLE_ARRAY_INDEX;
      break;
   default:
      return NULL;
   }

   texImage = ctx->Texture.ProxyTex[texIndex]->Image[0][level];
   if (!texImage) {
      texImage = ctx->Driver.NewTextureImage(ctx);
      if (!texImage) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "proxy texture allocation");
         return NULL;
      }
      ctx->Texture.ProxyTex[texIndex]->Image[0][level] = texImage;
      /* Set the 'back' pointer */
      texImage->TexObject = ctx->Texture.ProxyTex[texIndex];
   }
   return texImage;
}


/**
 * Get the maximum number of allowed mipmap levels.
 *
 * \param ctx GL context.
 * \param target texture target.
 * 
 * \return the maximum number of allowed mipmap levels for the given
 * texture target, or zero if passed a bad target.
 *
 * \sa gl_constants.
 */
GLint
_mesa_max_texture_levels(struct gl_context *ctx, GLenum target)
{
   switch (target) {
   case GL_TEXTURE_1D:
   case GL_PROXY_TEXTURE_1D:
   case GL_TEXTURE_2D:
   case GL_PROXY_TEXTURE_2D:
      return ctx->Const.MaxTextureLevels;
   case GL_TEXTURE_3D:
   case GL_PROXY_TEXTURE_3D:
      return ctx->Const.Max3DTextureLevels;
   case GL_TEXTURE_CUBE_MAP:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB:
   case GL_PROXY_TEXTURE_CUBE_MAP_ARB:
      return ctx->Extensions.ARB_texture_cube_map
         ? ctx->Const.MaxCubeTextureLevels : 0;
   case GL_TEXTURE_RECTANGLE_NV:
   case GL_PROXY_TEXTURE_RECTANGLE_NV:
      return ctx->Extensions.NV_texture_rectangle ? 1 : 0;
   case GL_TEXTURE_1D_ARRAY_EXT:
   case GL_PROXY_TEXTURE_1D_ARRAY_EXT:
   case GL_TEXTURE_2D_ARRAY_EXT:
   case GL_PROXY_TEXTURE_2D_ARRAY_EXT:
      return (ctx->Extensions.MESA_texture_array ||
              ctx->Extensions.EXT_texture_array)
         ? ctx->Const.MaxTextureLevels : 0;
   case GL_TEXTURE_CUBE_MAP_ARRAY:
   case GL_PROXY_TEXTURE_CUBE_MAP_ARRAY:
      return ctx->Extensions.ARB_texture_cube_map_array
         ? ctx->Const.MaxCubeTextureLevels : 0;
   case GL_TEXTURE_BUFFER:
      return ctx->API == API_OPENGL_CORE &&
             ctx->Extensions.ARB_texture_buffer_object ? 1 : 0;
   case GL_TEXTURE_2D_MULTISAMPLE:
   case GL_PROXY_TEXTURE_2D_MULTISAMPLE:
   case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
   case GL_PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY:
      return _mesa_is_desktop_gl(ctx)
         && ctx->Extensions.ARB_texture_multisample
         ? 1 : 0;
   case GL_TEXTURE_EXTERNAL_OES:
      /* fall-through */
   default:
      return 0; /* bad target */
   }
}


/**
 * Return number of dimensions per mipmap level for the given texture target.
 */
GLint
_mesa_get_texture_dimensions(GLenum target)
{
   switch (target) {
   case GL_TEXTURE_1D:
   case GL_PROXY_TEXTURE_1D:
      return 1;
   case GL_TEXTURE_2D:
   case GL_TEXTURE_RECTANGLE:
   case GL_TEXTURE_CUBE_MAP:
   case GL_PROXY_TEXTURE_2D:
   case GL_PROXY_TEXTURE_RECTANGLE:
   case GL_PROXY_TEXTURE_CUBE_MAP:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
   case GL_TEXTURE_1D_ARRAY:
   case GL_PROXY_TEXTURE_1D_ARRAY:
   case GL_TEXTURE_EXTERNAL_OES:
   case GL_TEXTURE_2D_MULTISAMPLE:
   case GL_PROXY_TEXTURE_2D_MULTISAMPLE:
      return 2;
   case GL_TEXTURE_3D:
   case GL_PROXY_TEXTURE_3D:
   case GL_TEXTURE_2D_ARRAY:
   case GL_PROXY_TEXTURE_2D_ARRAY:
   case GL_TEXTURE_CUBE_MAP_ARRAY:
   case GL_PROXY_TEXTURE_CUBE_MAP_ARRAY:
   case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
   case GL_PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY:
      return 3;
   case GL_TEXTURE_BUFFER:
      /* fall-through */
   default:
      _mesa_problem(NULL, "invalid target 0x%x in get_texture_dimensions()",
                    target);
      return 2;
   }
}


/**
 * Return the maximum number of mipmap levels for the given target
 * and the dimensions.
 * The dimensions are expected not to include the border.
 */
GLsizei
_mesa_get_tex_max_num_levels(GLenum target, GLsizei width, GLsizei height,
                             GLsizei depth)
{
   GLsizei size;

   switch (target) {
   case GL_TEXTURE_1D:
   case GL_TEXTURE_1D_ARRAY:
      size = width;
      break;
   case GL_TEXTURE_CUBE_MAP:
   case GL_TEXTURE_CUBE_MAP_ARRAY:
      ASSERT(width == height);
      size = width;
      break;
   case GL_TEXTURE_2D:
   case GL_TEXTURE_2D_ARRAY:
      size = MAX2(width, height);
      break;
   case GL_TEXTURE_3D:
      size = MAX3(width, height, depth);
      break;
   case GL_TEXTURE_RECTANGLE:
   case GL_TEXTURE_EXTERNAL_OES:
   case GL_TEXTURE_2D_MULTISAMPLE:
   case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
      return 1;
   default:
      assert(0);
      return 1;
   }

   return _mesa_logbase2(size) + 1;
}


#if 000 /* not used anymore */
/*
 * glTexImage[123]D can accept a NULL image pointer.  In this case we
 * create a texture image with unspecified image contents per the OpenGL
 * spec.
 */
static GLubyte *
make_null_texture(GLint width, GLint height, GLint depth, GLenum format)
{
   const GLint components = _mesa_components_in_format(format);
   const GLint numPixels = width * height * depth;
   GLubyte *data = (GLubyte *) malloc(numPixels * components * sizeof(GLubyte));

#ifdef DEBUG
   /*
    * Let's see if anyone finds this.  If glTexImage2D() is called with
    * a NULL image pointer then load the texture image with something
    * interesting instead of leaving it indeterminate.
    */
   if (data) {
      static const char message[8][32] = {
         "   X   X  XXXXX   XXX     X    ",
         "   XX XX  X      X   X   X X   ",
         "   X X X  X      X      X   X  ",
         "   X   X  XXXX    XXX   XXXXX  ",
         "   X   X  X          X  X   X  ",
         "   X   X  X      X   X  X   X  ",
         "   X   X  XXXXX   XXX   X   X  ",
         "                               "
      };

      GLubyte *imgPtr = data;
      GLint h, i, j, k;
      for (h = 0; h < depth; h++) {
         for (i = 0; i < height; i++) {
            GLint srcRow = 7 - (i % 8);
            for (j = 0; j < width; j++) {
               GLint srcCol = j % 32;
               GLubyte texel = (message[srcRow][srcCol]=='X') ? 255 : 70;
               for (k = 0; k < components; k++) {
                  *imgPtr++ = texel;
               }
            }
         }
      }
   }
#endif

   return data;
}
#endif



/**
 * Set the size and format-related fields of a gl_texture_image struct
 * to zero.  This is used when a proxy texture test fails.
 */
static void
clear_teximage_fields(struct gl_texture_image *img)
{
   ASSERT(img);
   img->_BaseFormat = 0;
   img->InternalFormat = 0;
   img->Border = 0;
   img->Width = 0;
   img->Height = 0;
   img->Depth = 0;
   img->Width2 = 0;
   img->Height2 = 0;
   img->Depth2 = 0;
   img->WidthLog2 = 0;
   img->HeightLog2 = 0;
   img->DepthLog2 = 0;
   img->TexFormat = MESA_FORMAT_NONE;
   img->NumSamples = 0;
   img->FixedSampleLocations = GL_TRUE;
}


/**
 * Initialize basic fields of the gl_texture_image struct.
 *
 * \param ctx GL context.
 * \param img texture image structure to be initialized.
 * \param width image width.
 * \param height image height.
 * \param depth image depth.
 * \param border image border.
 * \param internalFormat internal format.
 * \param format  the actual hardware format (one of MESA_FORMAT_*)
 *
 * Fills in the fields of \p img with the given information.
 * Note: width, height and depth include the border.
 */
void
_mesa_init_teximage_fields(struct gl_context *ctx,
                           struct gl_texture_image *img,
                           GLsizei width, GLsizei height, GLsizei depth,
                           GLint border, GLenum internalFormat,
                           gl_format format)
{
   GLenum target;
   ASSERT(img);
   ASSERT(width >= 0);
   ASSERT(height >= 0);
   ASSERT(depth >= 0);

   target = img->TexObject->Target;
   img->_BaseFormat = _mesa_base_tex_format( ctx, internalFormat );
   ASSERT(img->_BaseFormat > 0);
   img->InternalFormat = internalFormat;
   img->Border = border;
   img->Width = width;
   img->Height = height;
   img->Depth = depth;

   img->Width2 = width - 2 * border;   /* == 1 << img->WidthLog2; */
   img->WidthLog2 = _mesa_logbase2(img->Width2);

   img->NumSamples = 0;
   img->FixedSampleLocations = GL_TRUE;

   switch(target) {
   case GL_TEXTURE_1D:
   case GL_TEXTURE_BUFFER:
   case GL_PROXY_TEXTURE_1D:
      if (height == 0)
         img->Height2 = 0;
      else
         img->Height2 = 1;
      img->HeightLog2 = 0;
      if (depth == 0)
         img->Depth2 = 0;
      else
         img->Depth2 = 1;
      img->DepthLog2 = 0;
      break;
   case GL_TEXTURE_1D_ARRAY:
   case GL_PROXY_TEXTURE_1D_ARRAY:
      img->Height2 = height; /* no border */
      img->HeightLog2 = 0; /* not used */
      if (depth == 0)
         img->Depth2 = 0;
      else
         img->Depth2 = 1;
      img->DepthLog2 = 0;
      break;
   case GL_TEXTURE_2D:
   case GL_TEXTURE_RECTANGLE:
   case GL_TEXTURE_CUBE_MAP:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
   case GL_TEXTURE_EXTERNAL_OES:
   case GL_PROXY_TEXTURE_2D:
   case GL_PROXY_TEXTURE_RECTANGLE:
   case GL_PROXY_TEXTURE_CUBE_MAP:
   case GL_TEXTURE_2D_MULTISAMPLE:
   case GL_PROXY_TEXTURE_2D_MULTISAMPLE:
      img->Height2 = height - 2 * border; /* == 1 << img->HeightLog2; */
      img->HeightLog2 = _mesa_logbase2(img->Height2);
      if (depth == 0)
         img->Depth2 = 0;
      else
         img->Depth2 = 1;
      img->DepthLog2 = 0;
      break;
   case GL_TEXTURE_2D_ARRAY:
   case GL_PROXY_TEXTURE_2D_ARRAY:
   case GL_TEXTURE_CUBE_MAP_ARRAY:
   case GL_PROXY_TEXTURE_CUBE_MAP_ARRAY:
   case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
   case GL_PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY:
      img->Height2 = height - 2 * border; /* == 1 << img->HeightLog2; */
      img->HeightLog2 = _mesa_logbase2(img->Height2);
      img->Depth2 = depth; /* no border */
      img->DepthLog2 = 0; /* not used */
      break;
   case GL_TEXTURE_3D:
   case GL_PROXY_TEXTURE_3D:
      img->Height2 = height - 2 * border; /* == 1 << img->HeightLog2; */
      img->HeightLog2 = _mesa_logbase2(img->Height2);
      img->Depth2 = depth - 2 * border;   /* == 1 << img->DepthLog2; */
      img->DepthLog2 = _mesa_logbase2(img->Depth2);
      break;
   default:
      _mesa_problem(NULL, "invalid target 0x%x in _mesa_init_teximage_fields()",
                    target);
   }

   img->MaxNumLevels =
      _mesa_get_tex_max_num_levels(target,
                                   img->Width2, img->Height2, img->Depth2);
   img->TexFormat = format;
}


/**
 * Free and clear fields of the gl_texture_image struct.
 *
 * \param ctx GL context.
 * \param texImage texture image structure to be cleared.
 *
 * After the call, \p texImage will have no data associated with it.  Its
 * fields are cleared so that its parent object will test incomplete.
 */
void
_mesa_clear_texture_image(struct gl_context *ctx,
                          struct gl_texture_image *texImage)
{
   ctx->Driver.FreeTextureImageBuffer(ctx, texImage);
   clear_teximage_fields(texImage);
}


/**
 * Check the width, height, depth and border of a texture image are legal.
 * Used by all the glTexImage, glCompressedTexImage and glCopyTexImage
 * functions.
 * The target and level parameters will have already been validated.
 * \return GL_TRUE if size is OK, GL_FALSE otherwise.
 */
GLboolean
_mesa_legal_texture_dimensions(struct gl_context *ctx, GLenum target,
                               GLint level, GLint width, GLint height,
                               GLint depth, GLint border)
{
   GLint maxSize;

   switch (target) {
   case GL_TEXTURE_1D:
   case GL_PROXY_TEXTURE_1D:
      maxSize = 1 << (ctx->Const.MaxTextureLevels - 1); /* level zero size */
      maxSize >>= level;  /* level size */
      if (width < 2 * border || width > 2 * border + maxSize)
         return GL_FALSE;
      if (!ctx->Extensions.ARB_texture_non_power_of_two) {
         if (width > 0 && !_mesa_is_pow_two(width - 2 * border))
            return GL_FALSE;
      }
      return GL_TRUE;

   case GL_TEXTURE_2D:
   case GL_PROXY_TEXTURE_2D:
   case GL_TEXTURE_2D_MULTISAMPLE:
   case GL_PROXY_TEXTURE_2D_MULTISAMPLE:
      maxSize = 1 << (ctx->Const.MaxTextureLevels - 1);
      maxSize >>= level;
      if (width < 2 * border || width > 2 * border + maxSize)
         return GL_FALSE;
      if (height < 2 * border || height > 2 * border + maxSize)
         return GL_FALSE;
      if (!ctx->Extensions.ARB_texture_non_power_of_two) {
         if (width > 0 && !_mesa_is_pow_two(width - 2 * border))
            return GL_FALSE;
         if (height > 0 && !_mesa_is_pow_two(height - 2 * border))
            return GL_FALSE;
      }
      return GL_TRUE;

   case GL_TEXTURE_3D:
   case GL_PROXY_TEXTURE_3D:
      maxSize = 1 << (ctx->Const.Max3DTextureLevels - 1);
      maxSize >>= level;
      if (width < 2 * border || width > 2 * border + maxSize)
         return GL_FALSE;
      if (height < 2 * border || height > 2 * border + maxSize)
         return GL_FALSE;
      if (depth < 2 * border || depth > 2 * border + maxSize)
         return GL_FALSE;
      if (!ctx->Extensions.ARB_texture_non_power_of_two) {
         if (width > 0 && !_mesa_is_pow_two(width - 2 * border))
            return GL_FALSE;
         if (height > 0 && !_mesa_is_pow_two(height - 2 * border))
            return GL_FALSE;
         if (depth > 0 && !_mesa_is_pow_two(depth - 2 * border))
            return GL_FALSE;
      }
      return GL_TRUE;

   case GL_TEXTURE_RECTANGLE_NV:
   case GL_PROXY_TEXTURE_RECTANGLE_NV:
      if (level != 0)
         return GL_FALSE;
      maxSize = ctx->Const.MaxTextureRectSize;
      if (width < 0 || width > maxSize)
         return GL_FALSE;
      if (height < 0 || height > maxSize)
         return GL_FALSE;
      return GL_TRUE;

   case GL_TEXTURE_CUBE_MAP:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
   case GL_PROXY_TEXTURE_CUBE_MAP_ARB:
      maxSize = 1 << (ctx->Const.MaxCubeTextureLevels - 1);
      maxSize >>= level;
      if (width < 2 * border || width > 2 * border + maxSize)
         return GL_FALSE;
      if (height < 2 * border || height > 2 * border + maxSize)
         return GL_FALSE;
      if (!ctx->Extensions.ARB_texture_non_power_of_two) {
         if (width > 0 && !_mesa_is_pow_two(width - 2 * border))
            return GL_FALSE;
         if (height > 0 && !_mesa_is_pow_two(height - 2 * border))
            return GL_FALSE;
      }
      return GL_TRUE;

   case GL_TEXTURE_1D_ARRAY_EXT:
   case GL_PROXY_TEXTURE_1D_ARRAY_EXT:
      maxSize = 1 << (ctx->Const.MaxTextureLevels - 1);
      maxSize >>= level;
      if (width < 2 * border || width > 2 * border + maxSize)
         return GL_FALSE;
      if (height < 1 || height > ctx->Const.MaxArrayTextureLayers)
         return GL_FALSE;
      if (!ctx->Extensions.ARB_texture_non_power_of_two) {
         if (width > 0 && !_mesa_is_pow_two(width - 2 * border))
            return GL_FALSE;
      }
      return GL_TRUE;

   case GL_TEXTURE_2D_ARRAY_EXT:
   case GL_PROXY_TEXTURE_2D_ARRAY_EXT:
   case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
   case GL_PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY:
      maxSize = 1 << (ctx->Const.MaxTextureLevels - 1);
      maxSize >>= level;
      if (width < 2 * border || width > 2 * border + maxSize)
         return GL_FALSE;
      if (height < 2 * border || height > 2 * border + maxSize)
         return GL_FALSE;
      if (depth < 1 || depth > ctx->Const.MaxArrayTextureLayers)
         return GL_FALSE;
      if (!ctx->Extensions.ARB_texture_non_power_of_two) {
         if (width > 0 && !_mesa_is_pow_two(width - 2 * border))
            return GL_FALSE;
         if (height > 0 && !_mesa_is_pow_two(height - 2 * border))
            return GL_FALSE;
      }
      return GL_TRUE;

   case GL_TEXTURE_CUBE_MAP_ARRAY:
   case GL_PROXY_TEXTURE_CUBE_MAP_ARRAY:
      maxSize = 1 << (ctx->Const.MaxCubeTextureLevels - 1);
      if (width < 2 * border || width > 2 * border + maxSize)
         return GL_FALSE;
      if (height < 2 * border || height > 2 * border + maxSize)
         return GL_FALSE;
      if (depth < 1 || depth > ctx->Const.MaxArrayTextureLayers)
         return GL_FALSE;
      if (level >= ctx->Const.MaxCubeTextureLevels)
         return GL_FALSE;
      if (!ctx->Extensions.ARB_texture_non_power_of_two) {
         if (width > 0 && !_mesa_is_pow_two(width - 2 * border))
            return GL_FALSE;
         if (height > 0 && !_mesa_is_pow_two(height - 2 * border))
            return GL_FALSE;
      }
      return GL_TRUE;
   default:
      _mesa_problem(ctx, "Invalid target in _mesa_legal_texture_dimensions()");
      return GL_FALSE;
   }
}


/**
 * Do error checking of xoffset, yoffset, zoffset, width, height and depth
 * for glTexSubImage, glCopyTexSubImage and glCompressedTexSubImage.
 * \param destImage  the destination texture image.
 * \return GL_TRUE if error found, GL_FALSE otherwise.
 */
static GLboolean
error_check_subtexture_dimensions(struct gl_context *ctx,
                                  const char *function, GLuint dims,
                                  const struct gl_texture_image *destImage,
                                  GLint xoffset, GLint yoffset, GLint zoffset,
                                  GLsizei subWidth, GLsizei subHeight,
                                  GLsizei subDepth)
{
   const GLenum target = destImage->TexObject->Target;
   GLuint bw, bh;

   /* Check size */
   if (subWidth < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "%s%dD(width=%d)", function, dims, subWidth);
      return GL_TRUE;
   }

   if (dims > 1 && subHeight < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "%s%dD(height=%d)", function, dims, subHeight);
      return GL_TRUE;
   }

   if (dims > 2 && subDepth < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "%s%dD(depth=%d)", function, dims, subDepth);
      return GL_TRUE;
   }

   /* check xoffset and width */
   if (xoffset < -destImage->Border) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s%dD(xoffset)",
                  function, dims);
      return GL_TRUE;
   }

   if (xoffset + subWidth > destImage->Width) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s%dD(xoffset+width)",
                  function, dims);
      return GL_TRUE;
   }

   /* check yoffset and height */
   if (dims > 1) {
      GLint yBorder = (target == GL_TEXTURE_1D_ARRAY) ? 0 : destImage->Border;
      if (yoffset < -yBorder) {
         _mesa_error(ctx, GL_INVALID_VALUE, "%s%dD(yoffset)",
                     function, dims);
         return GL_TRUE;
      }
      if (yoffset + subHeight > destImage->Height) {
         _mesa_error(ctx, GL_INVALID_VALUE, "%s%dD(yoffset+height)",
                     function, dims);
         return GL_TRUE;
      }
   }

   /* check zoffset and depth */
   if (dims > 2) {
      GLint zBorder = (target == GL_TEXTURE_2D_ARRAY) ? 0 : destImage->Border;
      if (zoffset < -zBorder) {
         _mesa_error(ctx, GL_INVALID_VALUE, "%s3D(zoffset)", function);
         return GL_TRUE;
      }
      if (zoffset + subDepth  > (GLint) destImage->Depth) {
         _mesa_error(ctx, GL_INVALID_VALUE, "%s3D(zoffset+depth)", function);
         return GL_TRUE;
      }
   }

   /*
    * The OpenGL spec (and GL_ARB_texture_compression) says only whole
    * compressed texture images can be updated.  But, that restriction may be
    * relaxed for particular compressed formats.  At this time, all the
    * compressed formats supported by Mesa allow sub-textures to be updated
    * along compressed block boundaries.
    */
   _mesa_get_format_block_size(destImage->TexFormat, &bw, &bh);

   if (bw != 1 || bh != 1) {
      /* offset must be multiple of block size */
      if ((xoffset % bw != 0) || (yoffset % bh != 0)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "%s%dD(xoffset = %d, yoffset = %d)",
                     function, dims, xoffset, yoffset);
         return GL_TRUE;
      }

      /* size must be multiple of bw by bh or equal to whole texture size */
      if ((subWidth % bw != 0) && subWidth != destImage->Width) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "%s%dD(width = %d)", function, dims, subWidth);
         return GL_TRUE;
      }

      if ((subHeight % bh != 0) && subHeight != destImage->Height) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "%s%dD(height = %d)", function, dims, subHeight);
         return GL_TRUE;
      }         
   }

   return GL_FALSE;
}




/**
 * This is the fallback for Driver.TestProxyTexImage() for doing device-
 * specific texture image size checks.
 *
 * A hardware driver might override this function if, for example, the
 * max 3D texture size is 512x512x64 (i.e. not a cube).
 *
 * Note that width, height, depth == 0 is not an error.  However, a
 * texture with zero width/height/depth will be considered "incomplete"
 * and texturing will effectively be disabled.
 *
 * \param target  any texture target/type
 * \param level  as passed to glTexImage
 * \param format  the MESA_FORMAT_x for the tex image
 * \param width  as passed to glTexImage
 * \param height  as passed to glTexImage
 * \param depth  as passed to glTexImage
 * \param border  as passed to glTexImage
 * \return GL_TRUE if the image is acceptable, GL_FALSE if not acceptable.
 */
GLboolean
_mesa_test_proxy_teximage(struct gl_context *ctx, GLenum target, GLint level,
                          gl_format format,
                          GLint width, GLint height, GLint depth, GLint border)
{
   /* We just check if the image size is less than MaxTextureMbytes.
    * Some drivers may do more specific checks.
    */
   uint64_t bytes = _mesa_format_image_size64(format, width, height, depth);
   uint64_t mbytes = bytes / (1024 * 1024); /* convert to MB */
   mbytes *= _mesa_num_tex_faces(target);
   return mbytes <= (uint64_t) ctx->Const.MaxTextureMbytes;
}


/**
 * Return true if the format is only valid for glCompressedTexImage.
 */
static GLboolean
compressedteximage_only_format(const struct gl_context *ctx, GLenum format)
{
   switch (format) {
   case GL_ETC1_RGB8_OES:
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
      return GL_TRUE;
   default:
      return GL_FALSE;
   }
}


/**
 * Helper function to determine whether a target and specific compression
 * format are supported.
 */
static GLboolean
target_can_be_compressed(const struct gl_context *ctx, GLenum target,
                         GLenum intFormat)
{
   (void) intFormat;  /* not used yet */

   switch (target) {
   case GL_TEXTURE_2D:
   case GL_PROXY_TEXTURE_2D:
      return GL_TRUE; /* true for any compressed format so far */
   case GL_PROXY_TEXTURE_CUBE_MAP:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
      return ctx->Extensions.ARB_texture_cube_map;
   case GL_PROXY_TEXTURE_2D_ARRAY_EXT:
   case GL_TEXTURE_2D_ARRAY_EXT:
      return (ctx->Extensions.MESA_texture_array ||
              ctx->Extensions.EXT_texture_array);
   case GL_PROXY_TEXTURE_CUBE_MAP_ARRAY:
   case GL_TEXTURE_CUBE_MAP_ARRAY:
      return ctx->Extensions.ARB_texture_cube_map_array;
   default:
      return GL_FALSE;
   }      
}


/**
 * Check if the given texture target value is legal for a
 * glTexImage1/2/3D call.
 */
static GLboolean
legal_teximage_target(struct gl_context *ctx, GLuint dims, GLenum target)
{
   switch (dims) {
   case 1:
      switch (target) {
      case GL_TEXTURE_1D:
      case GL_PROXY_TEXTURE_1D:
         return _mesa_is_desktop_gl(ctx);
      default:
         return GL_FALSE;
      }
   case 2:
      switch (target) {
      case GL_TEXTURE_2D:
         return GL_TRUE;
      case GL_PROXY_TEXTURE_2D:
         return _mesa_is_desktop_gl(ctx);
      case GL_PROXY_TEXTURE_CUBE_MAP:
         return _mesa_is_desktop_gl(ctx)
            && ctx->Extensions.ARB_texture_cube_map;
      case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
         return ctx->Extensions.ARB_texture_cube_map;
      case GL_TEXTURE_RECTANGLE_NV:
      case GL_PROXY_TEXTURE_RECTANGLE_NV:
         return _mesa_is_desktop_gl(ctx)
            && ctx->Extensions.NV_texture_rectangle;
      case GL_TEXTURE_1D_ARRAY_EXT:
      case GL_PROXY_TEXTURE_1D_ARRAY_EXT:
         return _mesa_is_desktop_gl(ctx)
            && (ctx->Extensions.MESA_texture_array ||
                ctx->Extensions.EXT_texture_array);
      default:
         return GL_FALSE;
      }
   case 3:
      switch (target) {
      case GL_TEXTURE_3D:
         return GL_TRUE;
      case GL_PROXY_TEXTURE_3D:
         return _mesa_is_desktop_gl(ctx);
      case GL_TEXTURE_2D_ARRAY_EXT:
         return (_mesa_is_desktop_gl(ctx)
                 && (ctx->Extensions.MESA_texture_array ||
                     ctx->Extensions.EXT_texture_array))
            || _mesa_is_gles3(ctx);
      case GL_PROXY_TEXTURE_2D_ARRAY_EXT:
         return _mesa_is_desktop_gl(ctx)
            && (ctx->Extensions.MESA_texture_array ||
                ctx->Extensions.EXT_texture_array);
      case GL_TEXTURE_CUBE_MAP_ARRAY:
      case GL_PROXY_TEXTURE_CUBE_MAP_ARRAY:
         return ctx->Extensions.ARB_texture_cube_map_array;
      default:
         return GL_FALSE;
      }
   default:
      _mesa_problem(ctx, "invalid dims=%u in legal_teximage_target()", dims);
      return GL_FALSE;
   }
}


/**
 * Check if the given texture target value is legal for a
 * glTexSubImage, glCopyTexSubImage or glCopyTexImage call.
 * The difference compared to legal_teximage_target() above is that
 * proxy targets are not supported.
 */
static GLboolean
legal_texsubimage_target(struct gl_context *ctx, GLuint dims, GLenum target)
{
   switch (dims) {
   case 1:
      return _mesa_is_desktop_gl(ctx) && target == GL_TEXTURE_1D;
   case 2:
      switch (target) {
      case GL_TEXTURE_2D:
         return GL_TRUE;
      case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
         return ctx->Extensions.ARB_texture_cube_map;
      case GL_TEXTURE_RECTANGLE_NV:
         return _mesa_is_desktop_gl(ctx)
            && ctx->Extensions.NV_texture_rectangle;
      case GL_TEXTURE_1D_ARRAY_EXT:
         return _mesa_is_desktop_gl(ctx)
            && (ctx->Extensions.MESA_texture_array ||
                ctx->Extensions.EXT_texture_array);
      default:
         return GL_FALSE;
      }
   case 3:
      switch (target) {
      case GL_TEXTURE_3D:
         return GL_TRUE;
      case GL_TEXTURE_2D_ARRAY_EXT:
         return (_mesa_is_desktop_gl(ctx)
                 && (ctx->Extensions.MESA_texture_array ||
                     ctx->Extensions.EXT_texture_array))
            || _mesa_is_gles3(ctx);
      case GL_TEXTURE_CUBE_MAP_ARRAY:
      case GL_PROXY_TEXTURE_CUBE_MAP_ARRAY:
         return ctx->Extensions.ARB_texture_cube_map_array;
      default:
         return GL_FALSE;
      }
   default:
      _mesa_problem(ctx, "invalid dims=%u in legal_texsubimage_target()",
                    dims);
      return GL_FALSE;
   }
}


/**
 * Helper function to determine if a texture object is mutable (in terms
 * of GL_ARB_texture_storage).
 */
static GLboolean
mutable_tex_object(struct gl_context *ctx, GLenum target)
{
   if (ctx->Extensions.ARB_texture_storage) {
      struct gl_texture_object *texObj =
         _mesa_get_current_tex_object(ctx, target);
      return !texObj->Immutable;
   }
   return GL_TRUE;
}


/**
 * Return expected size of a compressed texture.
 */
static GLuint
compressed_tex_size(GLsizei width, GLsizei height, GLsizei depth,
                    GLenum glformat)
{
   gl_format mesaFormat = _mesa_glenum_to_compressed_format(glformat);
   return _mesa_format_image_size(mesaFormat, width, height, depth);
}


/**
 * Test the glTexImage[123]D() parameters for errors.
 * 
 * \param ctx GL context.
 * \param dimensions texture image dimensions (must be 1, 2 or 3).
 * \param target texture target given by the user (already validated).
 * \param level image level given by the user.
 * \param internalFormat internal format given by the user.
 * \param format pixel data format given by the user.
 * \param type pixel data type given by the user.
 * \param width image width given by the user.
 * \param height image height given by the user.
 * \param depth image depth given by the user.
 * \param border image border given by the user.
 * 
 * \return GL_TRUE if a error is found, GL_FALSE otherwise
 *
 * Verifies each of the parameters against the constants specified in
 * __struct gl_contextRec::Const and the supported extensions, and according
 * to the OpenGL specification.
 * Note that we don't fully error-check the width, height, depth values
 * here.  That's done in _mesa_legal_texture_dimensions() which is used
 * by several other GL entrypoints.  Plus, texture dims have a special
 * interaction with proxy textures.
 */
static GLboolean
texture_error_check( struct gl_context *ctx,
                     GLuint dimensions, GLenum target,
                     GLint level, GLint internalFormat,
                     GLenum format, GLenum type,
                     GLint width, GLint height,
                     GLint depth, GLint border )
{
   GLboolean colorFormat;
   GLenum err;

   /* Even though there are no color-index textures, we still have to support
    * uploading color-index data and remapping it to RGB via the
    * GL_PIXEL_MAP_I_TO_[RGBA] tables.
    */
   const GLboolean indexFormat = (format == GL_COLOR_INDEX);

   /* Note: for proxy textures, some error conditions immediately generate
    * a GL error in the usual way.  But others do not generate a GL error.
    * Instead, they cause the width, height, depth, format fields of the
    * texture image to be zeroed-out.  The GL spec seems to indicate that the
    * zero-out behaviour is only used in cases related to memory allocation.
    */

   /* level check */
   if (level < 0 || level >= _mesa_max_texture_levels(ctx, target)) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glTexImage%dD(level=%d)", dimensions, level);
      return GL_TRUE;
   }

   /* Check border */
   if (border < 0 || border > 1 ||
       ((ctx->API != API_OPENGL_COMPAT ||
         target == GL_TEXTURE_RECTANGLE_NV ||
         target == GL_PROXY_TEXTURE_RECTANGLE_NV) && border != 0)) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glTexImage%dD(border=%d)", dimensions, border);
      return GL_TRUE;
   }

   if (width < 0 || height < 0 || depth < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glTexImage%dD(width, height or depth < 0)", dimensions);
      return GL_TRUE;
   }

   /* OpenGL ES 1.x and OpenGL ES 2.0 impose additional restrictions on the
    * combinations of format, internalFormat, and type that can be used.
    * Formats and types that require additional extensions (e.g., GL_FLOAT
    * requires GL_OES_texture_float) are filtered elsewhere.
    */

   if (_mesa_is_gles(ctx)) {
      if (_mesa_is_gles3(ctx)) {
         err = _mesa_es3_error_check_format_and_type(format, type,
                                                     internalFormat);
      } else {
         if (format != internalFormat) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glTexImage%dD(format = %s, internalFormat = %s)",
                     dimensions,
                     _mesa_lookup_enum_by_nr(format),
                     _mesa_lookup_enum_by_nr(internalFormat));
         return GL_TRUE;
         }

         err = _mesa_es_error_check_format_and_type(format, type, dimensions);
      }
      if (err != GL_NO_ERROR) {
         _mesa_error(ctx, err,
                     "glTexImage%dD(format = %s, type = %s, internalFormat = %s)",
                     dimensions,
                     _mesa_lookup_enum_by_nr(format),
                     _mesa_lookup_enum_by_nr(type),
                     _mesa_lookup_enum_by_nr(internalFormat));
         return GL_TRUE;
      }
   }

   if ((target == GL_PROXY_TEXTURE_CUBE_MAP_ARB ||
        _mesa_is_cube_face(target)) && width != height) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glTexImage2D(cube width != height)");
      return GL_TRUE;
   }

   if ((target == GL_PROXY_TEXTURE_CUBE_MAP_ARRAY ||
        target == GL_TEXTURE_CUBE_MAP_ARRAY) && width != height) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glTexImage3D(cube array width != height)");
      return GL_TRUE;
   }

   if ((target == GL_PROXY_TEXTURE_CUBE_MAP_ARRAY ||
        target == GL_TEXTURE_CUBE_MAP_ARRAY) && (depth % 6)) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glTexImage3D(cube array depth not multiple of 6)");
      return GL_TRUE;
   }

   /* Check internalFormat */
   if (_mesa_base_tex_format(ctx, internalFormat) < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glTexImage%dD(internalFormat=%s)",
                  dimensions, _mesa_lookup_enum_by_nr(internalFormat));
      return GL_TRUE;
   }

   /* Check incoming image format and type */
   err = _mesa_error_check_format_and_type(ctx, format, type);
   if (err != GL_NO_ERROR) {
      _mesa_error(ctx, err,
                  "glTexImage%dD(incompatible format = %s, type = %s)",
                  dimensions, _mesa_lookup_enum_by_nr(format),
                  _mesa_lookup_enum_by_nr(type));
      return GL_TRUE;
   }

   /* make sure internal format and format basically agree */
   colorFormat = _mesa_is_color_format(format);
   if ((_mesa_is_color_format(internalFormat) && !colorFormat && !indexFormat) ||
       (_mesa_is_depth_format(internalFormat) != _mesa_is_depth_format(format)) ||
       (_mesa_is_ycbcr_format(internalFormat) != _mesa_is_ycbcr_format(format)) ||
       (_mesa_is_depthstencil_format(internalFormat) != _mesa_is_depthstencil_format(format)) ||
       (_mesa_is_dudv_format(internalFormat) != _mesa_is_dudv_format(format))) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glTexImage%dD(incompatible internalFormat = %s, format = %s)",
                  dimensions, _mesa_lookup_enum_by_nr(internalFormat),
                  _mesa_lookup_enum_by_nr(format));
      return GL_TRUE;
   }

   /* additional checks for ycbcr textures */
   if (internalFormat == GL_YCBCR_MESA) {
      ASSERT(ctx->Extensions.MESA_ycbcr_texture);
      if (type != GL_UNSIGNED_SHORT_8_8_MESA &&
          type != GL_UNSIGNED_SHORT_8_8_REV_MESA) {
         char message[100];
         _mesa_snprintf(message, sizeof(message),
                        "glTexImage%dD(format/type YCBCR mismatch)",
                        dimensions);
         _mesa_error(ctx, GL_INVALID_ENUM, "%s", message);
         return GL_TRUE; /* error */
      }
      if (target != GL_TEXTURE_2D &&
          target != GL_PROXY_TEXTURE_2D &&
          target != GL_TEXTURE_RECTANGLE_NV &&
          target != GL_PROXY_TEXTURE_RECTANGLE_NV) {
         _mesa_error(ctx, GL_INVALID_ENUM,
                     "glTexImage%dD(bad target for YCbCr texture)",
                     dimensions);
         return GL_TRUE;
      }
      if (border != 0) {
         char message[100];
         _mesa_snprintf(message, sizeof(message),
                        "glTexImage%dD(format=GL_YCBCR_MESA and border=%d)",
                        dimensions, border);
         _mesa_error(ctx, GL_INVALID_VALUE, "%s", message);
         return GL_TRUE;
      }
   }

   /* additional checks for depth textures */
   if (_mesa_base_tex_format(ctx, internalFormat) == GL_DEPTH_COMPONENT
       || _mesa_base_tex_format(ctx, internalFormat) == GL_DEPTH_STENCIL) {
      /* Only 1D, 2D, rect, array and cube textures supported, not 3D
       * Cubemaps are only supported for GL version > 3.0 or with EXT_gpu_shader4 */
      if (target != GL_TEXTURE_1D &&
          target != GL_PROXY_TEXTURE_1D &&
          target != GL_TEXTURE_2D &&
          target != GL_PROXY_TEXTURE_2D &&
          target != GL_TEXTURE_1D_ARRAY &&
          target != GL_PROXY_TEXTURE_1D_ARRAY &&
          target != GL_TEXTURE_2D_ARRAY &&
          target != GL_PROXY_TEXTURE_2D_ARRAY &&
          target != GL_TEXTURE_RECTANGLE_ARB &&
          target != GL_PROXY_TEXTURE_RECTANGLE_ARB &&
         !((_mesa_is_cube_face(target) || target == GL_PROXY_TEXTURE_CUBE_MAP) &&
           (ctx->Version >= 30 || ctx->Extensions.EXT_gpu_shader4
            || (ctx->API == API_OPENGLES2 && ctx->Extensions.OES_depth_texture_cube_map))) &&
          !((target == GL_TEXTURE_CUBE_MAP_ARRAY ||
             target == GL_PROXY_TEXTURE_CUBE_MAP_ARRAY) &&
            ctx->Extensions.ARB_texture_cube_map_array)) {
         _mesa_error(ctx, GL_INVALID_ENUM,
                     "glTexImage%dD(bad target for depth texture)",
                     dimensions);
         return GL_TRUE;
      }
   }

   /* additional checks for compressed textures */
   if (_mesa_is_compressed_format(ctx, internalFormat)) {
      if (!target_can_be_compressed(ctx, target, internalFormat)) {
         _mesa_error(ctx, GL_INVALID_ENUM,
                     "glTexImage%dD(target can't be compressed)", dimensions);
         return GL_TRUE;
      }
      if (compressedteximage_only_format(ctx, internalFormat)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glTexImage%dD(no compression for format)", dimensions);
         return GL_TRUE;
      }
      if (border != 0) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glTexImage%dD(border!=0)", dimensions);
         return GL_TRUE;
      }
   }

   /* additional checks for integer textures */
   if ((ctx->Version >= 30 || ctx->Extensions.EXT_texture_integer) &&
       (_mesa_is_enum_format_integer(format) !=
        _mesa_is_enum_format_integer(internalFormat))) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glTexImage%dD(integer/non-integer format mismatch)",
                  dimensions);
      return GL_TRUE;
   }

   if (!mutable_tex_object(ctx, target)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glTexImage%dD(immutable texture)", dimensions);
      return GL_TRUE;
   }

   /* if we get here, the parameters are OK */
   return GL_FALSE;
}


/**
 * Error checking for glCompressedTexImage[123]D().
 * Note that the width, height and depth values are not fully error checked
 * here.
 * \return GL_TRUE if a error is found, GL_FALSE otherwise
 */
static GLenum
compressed_texture_error_check(struct gl_context *ctx, GLint dimensions,
                               GLenum target, GLint level,
                               GLenum internalFormat, GLsizei width,
                               GLsizei height, GLsizei depth, GLint border,
                               GLsizei imageSize)
{
   const GLint maxLevels = _mesa_max_texture_levels(ctx, target);
   GLint expectedSize;
   GLenum error = GL_NO_ERROR;
   char *reason = ""; /* no error */

   if (!target_can_be_compressed(ctx, target, internalFormat)) {
      reason = "target";
      error = GL_INVALID_ENUM;
      goto error;
   }

   /* This will detect any invalid internalFormat value */
   if (!_mesa_is_compressed_format(ctx, internalFormat)) {
      reason = "internalFormat";
      error = GL_INVALID_ENUM;
      goto error;
   }

   switch (internalFormat) {
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
      /* check level (note that level should be zero or less!) */
      if (level > 0 || level < -maxLevels) {
	 reason = "level";
	 error = GL_INVALID_VALUE;
         goto error;
      }

      if (dimensions != 2) {
	 reason = "compressed paletted textures must be 2D";
	 error = GL_INVALID_OPERATION;
         goto error;
      }

      /* Figure out the expected texture size (in bytes).  This will be
       * checked against the actual size later.
       */
      expectedSize = _mesa_cpal_compressed_size(level, internalFormat,
						width, height);

      /* This is for the benefit of the TestProxyTexImage below.  It expects
       * level to be non-negative.  OES_compressed_paletted_texture uses a
       * weird mechanism where the level specified to glCompressedTexImage2D
       * is -(n-1) number of levels in the texture, and the data specifies the
       * complete mipmap stack.  This is done to ensure the palette is the
       * same for all levels.
       */
      level = -level;
      break;

   default:
      /* check level */
      if (level < 0 || level >= maxLevels) {
	 reason = "level";
	 error = GL_INVALID_VALUE;
         goto error;
      }

      /* Figure out the expected texture size (in bytes).  This will be
       * checked against the actual size later.
       */
      expectedSize = compressed_tex_size(width, height, depth, internalFormat);
      break;
   }

   /* This should really never fail */
   if (_mesa_base_tex_format(ctx, internalFormat) < 0) {
      reason = "internalFormat";
      error = GL_INVALID_ENUM;
      goto error;
   }

   /* No compressed formats support borders at this time */
   if (border != 0) {
      reason = "border != 0";
      error = GL_INVALID_VALUE;
      goto error;
   }

   /* For cube map, width must equal height */
   if ((target == GL_PROXY_TEXTURE_CUBE_MAP_ARB ||
        _mesa_is_cube_face(target)) && width != height) {
      reason = "width != height";
      error = GL_INVALID_VALUE;
      goto error;
   }

   /* check image size in bytes */
   if (expectedSize != imageSize) {
      /* Per GL_ARB_texture_compression:  GL_INVALID_VALUE is generated [...]
       * if <imageSize> is not consistent with the format, dimensions, and
       * contents of the specified image.
       */
      reason = "imageSize inconsistant with width/height/format";
      error = GL_INVALID_VALUE;
      goto error;
   }

   if (!mutable_tex_object(ctx, target)) {
      reason = "immutable texture";
      error = GL_INVALID_OPERATION;
      goto error;
   }

   return GL_FALSE;

error:
   _mesa_error(ctx, error, "glCompressedTexImage%dD(%s)", dimensions, reason);
   return GL_TRUE;
}



/**
 * Test glTexSubImage[123]D() parameters for errors.
 * 
 * \param ctx GL context.
 * \param dimensions texture image dimensions (must be 1, 2 or 3).
 * \param target texture target given by the user (already validated)
 * \param level image level given by the user.
 * \param xoffset sub-image x offset given by the user.
 * \param yoffset sub-image y offset given by the user.
 * \param zoffset sub-image z offset given by the user.
 * \param format pixel data format given by the user.
 * \param type pixel data type given by the user.
 * \param width image width given by the user.
 * \param height image height given by the user.
 * \param depth image depth given by the user.
 * 
 * \return GL_TRUE if an error was detected, or GL_FALSE if no errors.
 *
 * Verifies each of the parameters against the constants specified in
 * __struct gl_contextRec::Const and the supported extensions, and according
 * to the OpenGL specification.
 */
static GLboolean
texsubimage_error_check(struct gl_context *ctx, GLuint dimensions,
                        GLenum target, GLint level,
                        GLint xoffset, GLint yoffset, GLint zoffset,
                        GLint width, GLint height, GLint depth,
                        GLenum format, GLenum type)
{
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   GLenum err;

   /* check target (proxies not allowed) */
   if (!legal_texsubimage_target(ctx, dimensions, target)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glTexSubImage%uD(target=%s)",
                  dimensions, _mesa_lookup_enum_by_nr(target));
      return GL_TRUE;
   }

   /* level check */
   if (level < 0 || level >= _mesa_max_texture_levels(ctx, target)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glTexSubImage%uD(level=%d)",
                  dimensions, level);
      return GL_TRUE;
   }

   /* OpenGL ES 1.x and OpenGL ES 2.0 impose additional restrictions on the
    * combinations of format and type that can be used.  Formats and types
    * that require additional extensions (e.g., GL_FLOAT requires
    * GL_OES_texture_float) are filtered elsewhere.
    */
   if (_mesa_is_gles(ctx) && !_mesa_is_gles3(ctx)) {
      err = _mesa_es_error_check_format_and_type(format, type, dimensions);
      if (err != GL_NO_ERROR) {
         _mesa_error(ctx, err,
                     "glTexSubImage%dD(format = %s, type = %s)",
                     dimensions,
                     _mesa_lookup_enum_by_nr(format),
                     _mesa_lookup_enum_by_nr(type));
         return GL_TRUE;
      }
   }

   err = _mesa_error_check_format_and_type(ctx, format, type);
   if (err != GL_NO_ERROR) {
      _mesa_error(ctx, err,
                  "glTexSubImage%dD(incompatible format = %s, type = %s)",
                  dimensions, _mesa_lookup_enum_by_nr(format),
                  _mesa_lookup_enum_by_nr(type));
      return GL_TRUE;
   }

   /* Get dest texture object / image pointers */
   texObj = _mesa_get_current_tex_object(ctx, target);
   if (!texObj) {
      /* must be out of memory */
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexSubImage%dD()", dimensions);
      return GL_TRUE;
   }

   texImage = _mesa_select_tex_image(ctx, texObj, target, level);
   if (!texImage) {
      /* non-existant texture level */
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glTexSubImage%dD(invalid texture image)", dimensions);
      return GL_TRUE;
   }

   if (error_check_subtexture_dimensions(ctx, "glTexSubImage", dimensions,
                                         texImage, xoffset, yoffset, 0,
                                         width, height, 1)) {
      return GL_TRUE;
   }

   if (_mesa_is_format_compressed(texImage->TexFormat)) {
      if (compressedteximage_only_format(ctx, texImage->InternalFormat)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
               "glTexSubImage%dD(no compression for format)", dimensions);
         return GL_TRUE;
      }
   }

   if (ctx->Version >= 30 || ctx->Extensions.EXT_texture_integer) {
      /* both source and dest must be integer-valued, or neither */
      if (_mesa_is_format_integer_color(texImage->TexFormat) !=
          _mesa_is_enum_format_integer(format)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glTexSubImage%dD(integer/non-integer format mismatch)",
                     dimensions);
         return GL_TRUE;
      }
   }

   return GL_FALSE;
}


/**
 * Test glCopyTexImage[12]D() parameters for errors.
 * 
 * \param ctx GL context.
 * \param dimensions texture image dimensions (must be 1, 2 or 3).
 * \param target texture target given by the user.
 * \param level image level given by the user.
 * \param internalFormat internal format given by the user.
 * \param width image width given by the user.
 * \param height image height given by the user.
 * \param border texture border.
 * 
 * \return GL_TRUE if an error was detected, or GL_FALSE if no errors.
 * 
 * Verifies each of the parameters against the constants specified in
 * __struct gl_contextRec::Const and the supported extensions, and according
 * to the OpenGL specification.
 */
static GLboolean
copytexture_error_check( struct gl_context *ctx, GLuint dimensions,
                         GLenum target, GLint level, GLint internalFormat,
                         GLint width, GLint height, GLint border )
{
   GLint baseFormat;
   GLint rb_base_format;
   struct gl_renderbuffer *rb;
   GLenum rb_internal_format;

   /* check target */
   if (!legal_texsubimage_target(ctx, dimensions, target)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glCopyTexImage%uD(target=%s)",
                  dimensions, _mesa_lookup_enum_by_nr(target));
      return GL_TRUE;
   }       

   /* level check */
   if (level < 0 || level >= _mesa_max_texture_levels(ctx, target)) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glCopyTexImage%dD(level=%d)", dimensions, level);
      return GL_TRUE;
   }

   /* Check that the source buffer is complete */
   if (_mesa_is_user_fbo(ctx->ReadBuffer)) {
      if (ctx->ReadBuffer->_Status == 0) {
         _mesa_test_framebuffer_completeness(ctx, ctx->ReadBuffer);
      }
      if (ctx->ReadBuffer->_Status != GL_FRAMEBUFFER_COMPLETE_EXT) {
         _mesa_error(ctx, GL_INVALID_FRAMEBUFFER_OPERATION_EXT,
                     "glCopyTexImage%dD(invalid readbuffer)", dimensions);
         return GL_TRUE;
      }

      if (ctx->ReadBuffer->Visual.samples > 0) {
	 _mesa_error(ctx, GL_INVALID_OPERATION,
		     "glCopyTexImage%dD(multisample FBO)",
		     dimensions);
	 return GL_TRUE;
      }
   }

   /* Check border */
   if (border < 0 || border > 1 ||
       ((ctx->API != API_OPENGL_COMPAT ||
         target == GL_TEXTURE_RECTANGLE_NV ||
         target == GL_PROXY_TEXTURE_RECTANGLE_NV) && border != 0)) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glCopyTexImage%dD(border=%d)", dimensions, border);
      return GL_TRUE;
   }

   rb = _mesa_get_read_renderbuffer_for_format(ctx, internalFormat);
   if (rb == NULL) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glCopyTexImage%dD(read buffer)", dimensions);
      return GL_TRUE;
   }

   /* OpenGL ES 1.x and OpenGL ES 2.0 impose additional restrictions on the
    * internalFormat.
    */
   if (_mesa_is_gles(ctx) && !_mesa_is_gles3(ctx)) {
      switch (internalFormat) {
      case GL_ALPHA:
      case GL_RGB:
      case GL_RGBA:
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
         break;
      default:
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glCopyTexImage%dD(internalFormat)", dimensions);
         return GL_TRUE;
      }
   }

   baseFormat = _mesa_base_tex_format(ctx, internalFormat);
   if (baseFormat < 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glCopyTexImage%dD(internalFormat)", dimensions);
      return GL_TRUE;
   }

   rb_internal_format = rb->InternalFormat;
   rb_base_format = _mesa_base_tex_format(ctx, rb->InternalFormat);
   if (_mesa_is_color_format(internalFormat)) {
      if (rb_base_format < 0) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glCopyTexImage%dD(internalFormat)", dimensions);
         return GL_TRUE;
      }
   }

   if (_mesa_is_gles(ctx)) {
      bool valid = true;
      if (_mesa_base_format_component_count(baseFormat) >
          _mesa_base_format_component_count(rb_base_format)) {
         valid = false;
      }
      if (baseFormat == GL_DEPTH_COMPONENT ||
          baseFormat == GL_DEPTH_STENCIL ||
          rb_base_format == GL_DEPTH_COMPONENT ||
          rb_base_format == GL_DEPTH_STENCIL ||
          ((baseFormat == GL_LUMINANCE_ALPHA ||
            baseFormat == GL_ALPHA) &&
           rb_base_format != GL_RGBA) ||
          internalFormat == GL_RGB9_E5) {
         valid = false;
      }
      if (internalFormat == GL_RGB9_E5) {
         valid = false;
      }
      if (!valid) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glCopyTexImage%dD(internalFormat)", dimensions);
         return GL_TRUE;
      }
   }

   if (_mesa_is_gles3(ctx)) {
      bool rb_is_srgb = false;
      bool dst_is_srgb = false;

      if (ctx->Extensions.EXT_framebuffer_sRGB &&
          _mesa_get_format_color_encoding(rb->Format) == GL_SRGB) {
         rb_is_srgb = true;
      }

      if (_mesa_get_linear_internalformat(internalFormat) != internalFormat) {
         dst_is_srgb = true;
      }

      if (rb_is_srgb != dst_is_srgb) {
         /* Page 137 (page 149 of the PDF) in section 3.8.5 of the
          * OpenGLES 3.0.0 spec says:
          *
          *     "The error INVALID_OPERATION is also generated if the
          *     value of FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING for the
          *     framebuffer attachment corresponding to the read buffer
          *     is LINEAR (see section 6.1.13) and internalformat is
          *     one of the sRGB formats described in section 3.8.16, or
          *     if the value of FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING is
          *     SRGB and internalformat is not one of the sRGB formats."
          */
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glCopyTexImage%dD(srgb usage mismatch)", dimensions);
         return GL_TRUE;
      }
   }

   if (!_mesa_source_buffer_exists(ctx, baseFormat)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glCopyTexImage%dD(missing readbuffer)", dimensions);
      return GL_TRUE;
   }

   /* From the EXT_texture_integer spec:
    *
    *     "INVALID_OPERATION is generated by CopyTexImage* and CopyTexSubImage*
    *      if the texture internalformat is an integer format and the read color
    *      buffer is not an integer format, or if the internalformat is not an
    *      integer format and the read color buffer is an integer format."
    */
   if (_mesa_is_color_format(internalFormat)) {
      bool is_int = _mesa_is_enum_format_integer(internalFormat);
      bool is_rbint = _mesa_is_enum_format_integer(rb_internal_format);
      if (is_int || is_rbint) {
         if (is_int != is_rbint) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glCopyTexImage%dD(integer vs non-integer)", dimensions);
            return GL_TRUE;
         } else if (_mesa_is_gles(ctx) &&
                    _mesa_is_enum_format_unsigned_int(internalFormat) !=
                      _mesa_is_enum_format_unsigned_int(rb_internal_format)) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glCopyTexImage%dD(signed vs unsigned integer)", dimensions);
            return GL_TRUE;
         }
      }
   }

   if ((target == GL_PROXY_TEXTURE_CUBE_MAP_ARB ||
        _mesa_is_cube_face(target)) && width != height) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glTexImage2D(cube width != height)");
      return GL_TRUE;
   }

   if (_mesa_is_compressed_format(ctx, internalFormat)) {
      if (!target_can_be_compressed(ctx, target, internalFormat)) {
         _mesa_error(ctx, GL_INVALID_ENUM,
                     "glCopyTexImage%dD(target)", dimensions);
         return GL_TRUE;
      }
      if (compressedteximage_only_format(ctx, internalFormat)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
               "glCopyTexImage%dD(no compression for format)", dimensions);
         return GL_TRUE;
      }
      if (border != 0) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glCopyTexImage%dD(border!=0)", dimensions);
         return GL_TRUE;
      }
   }

   if (!mutable_tex_object(ctx, target)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glCopyTexImage%dD(immutable texture)", dimensions);
      return GL_TRUE;
   }

   /* if we get here, the parameters are OK */
   return GL_FALSE;
}


/**
 * Test glCopyTexSubImage[12]D() parameters for errors.
 * \return GL_TRUE if an error was detected, or GL_FALSE if no errors.
 */
static GLboolean
copytexsubimage_error_check(struct gl_context *ctx, GLuint dimensions,
                            GLenum target, GLint level,
                            GLint xoffset, GLint yoffset, GLint zoffset,
                            GLint width, GLint height)
{
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;

   /* Check that the source buffer is complete */
   if (_mesa_is_user_fbo(ctx->ReadBuffer)) {
      if (ctx->ReadBuffer->_Status == 0) {
         _mesa_test_framebuffer_completeness(ctx, ctx->ReadBuffer);
      }
      if (ctx->ReadBuffer->_Status != GL_FRAMEBUFFER_COMPLETE_EXT) {
         _mesa_error(ctx, GL_INVALID_FRAMEBUFFER_OPERATION_EXT,
                     "glCopyTexImage%dD(invalid readbuffer)", dimensions);
         return GL_TRUE;
      }

      if (ctx->ReadBuffer->Visual.samples > 0) {
	 _mesa_error(ctx, GL_INVALID_OPERATION,
		     "glCopyTexSubImage%dD(multisample FBO)",
		     dimensions);
	 return GL_TRUE;
      }
   }

   /* check target (proxies not allowed) */
   if (!legal_texsubimage_target(ctx, dimensions, target)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glCopyTexSubImage%uD(target=%s)",
                  dimensions, _mesa_lookup_enum_by_nr(target));
      return GL_TRUE;
   }

   /* Check level */
   if (level < 0 || level >= _mesa_max_texture_levels(ctx, target)) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glCopyTexSubImage%dD(level=%d)", dimensions, level);
      return GL_TRUE;
   }

   /* Get dest texture object / image pointers */
   texObj = _mesa_get_current_tex_object(ctx, target);
   if (!texObj) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyTexSubImage%dD()", dimensions);
      return GL_TRUE;
   }

   texImage = _mesa_select_tex_image(ctx, texObj, target, level);
   if (!texImage) {
      /* destination image does not exist */
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glCopyTexSubImage%dD(invalid texture image)", dimensions);
      return GL_TRUE;
   }

   if (error_check_subtexture_dimensions(ctx, "glCopyTexSubImage",
                                         dimensions, texImage,
                                         xoffset, yoffset, zoffset,
                                         width, height, 1)) {
      return GL_TRUE;
   }

   if (_mesa_is_format_compressed(texImage->TexFormat)) {
      if (compressedteximage_only_format(ctx, texImage->InternalFormat)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
               "glCopyTexSubImage%dD(no compression for format)", dimensions);
         return GL_TRUE;
      }
   }

   if (texImage->InternalFormat == GL_YCBCR_MESA) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glCopyTexSubImage2D");
      return GL_TRUE;
   }

   if (!_mesa_source_buffer_exists(ctx, texImage->_BaseFormat)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glCopyTexSubImage%dD(missing readbuffer, format=0x%x)",
                  dimensions, texImage->_BaseFormat);
      return GL_TRUE;
   }

   /* From the EXT_texture_integer spec:
    *
    *     "INVALID_OPERATION is generated by CopyTexImage* and CopyTexSubImage*
    *      if the texture internalformat is an integer format and the read color
    *      buffer is not an integer format, or if the internalformat is not an
    *      integer format and the read color buffer is an integer format."
    */
   if (_mesa_is_color_format(texImage->InternalFormat)) {
      struct gl_renderbuffer *rb = ctx->ReadBuffer->_ColorReadBuffer;

      if (_mesa_is_format_integer_color(rb->Format) !=
	  _mesa_is_format_integer_color(texImage->TexFormat)) {
	 _mesa_error(ctx, GL_INVALID_OPERATION,
		     "glCopyTexImage%dD(integer vs non-integer)", dimensions);
	 return GL_TRUE;
      }
   }

   /* if we get here, the parameters are OK */
   return GL_FALSE;
}


/** Callback info for walking over FBO hash table */
struct cb_info
{
   struct gl_context *ctx;
   struct gl_texture_object *texObj;
   GLuint level, face;
};


/**
 * Check render to texture callback.  Called from _mesa_HashWalk().
 */
static void
check_rtt_cb(GLuint key, void *data, void *userData)
{
   struct gl_framebuffer *fb = (struct gl_framebuffer *) data;
   const struct cb_info *info = (struct cb_info *) userData;
   struct gl_context *ctx = info->ctx;
   const struct gl_texture_object *texObj = info->texObj;
   const GLuint level = info->level, face = info->face;

   /* If this is a user-created FBO */
   if (_mesa_is_user_fbo(fb)) {
      GLuint i;
      /* check if any of the FBO's attachments point to 'texObj' */
      for (i = 0; i < BUFFER_COUNT; i++) {
         struct gl_renderbuffer_attachment *att = fb->Attachment + i;
         if (att->Type == GL_TEXTURE &&
             att->Texture == texObj &&
             att->TextureLevel == level &&
             att->CubeMapFace == face) {
            ASSERT(_mesa_get_attachment_teximage(att));
            /* Tell driver about the new renderbuffer texture */
            ctx->Driver.RenderTexture(ctx, ctx->DrawBuffer, att);
            /* Mark fb status as indeterminate to force re-validation */
            fb->_Status = 0;
         }
      }
   }
}


/**
 * When a texture image is specified we have to check if it's bound to
 * any framebuffer objects (render to texture) in order to detect changes
 * in size or format since that effects FBO completeness.
 * Any FBOs rendering into the texture must be re-validated.
 */
void
_mesa_update_fbo_texture(struct gl_context *ctx,
                         struct gl_texture_object *texObj,
                         GLuint face, GLuint level)
{
   /* Only check this texture if it's been marked as RenderToTexture */
   if (texObj->_RenderToTexture) {
      struct cb_info info;
      info.ctx = ctx;
      info.texObj = texObj;
      info.level = level;
      info.face = face;
      _mesa_HashWalk(ctx->Shared->FrameBuffers, check_rtt_cb, &info);
   }
}


/**
 * If the texture object's GenerateMipmap flag is set and we've
 * changed the texture base level image, regenerate the rest of the
 * mipmap levels now.
 */
static inline void
check_gen_mipmap(struct gl_context *ctx, GLenum target,
                 struct gl_texture_object *texObj, GLint level)
{
   ASSERT(target != GL_TEXTURE_CUBE_MAP);
   if (texObj->GenerateMipmap &&
       level == texObj->BaseLevel &&
       level < texObj->MaxLevel) {
      ASSERT(ctx->Driver.GenerateMipmap);
      ctx->Driver.GenerateMipmap(ctx, target, texObj);
   }
}


/** Debug helper: override the user-requested internal format */
static GLenum
override_internal_format(GLenum internalFormat, GLint width, GLint height)
{
#if 0
   if (internalFormat == GL_RGBA16F_ARB ||
       internalFormat == GL_RGBA32F_ARB) {
      printf("Convert rgba float tex to int %d x %d\n", width, height);
      return GL_RGBA;
   }
   else if (internalFormat == GL_RGB16F_ARB ||
            internalFormat == GL_RGB32F_ARB) {
      printf("Convert rgb float tex to int %d x %d\n", width, height);
      return GL_RGB;
   }
   else if (internalFormat == GL_LUMINANCE_ALPHA16F_ARB ||
            internalFormat == GL_LUMINANCE_ALPHA32F_ARB) {
      printf("Convert luminance float tex to int %d x %d\n", width, height);
      return GL_LUMINANCE_ALPHA;
   }
   else if (internalFormat == GL_LUMINANCE16F_ARB ||
            internalFormat == GL_LUMINANCE32F_ARB) {
      printf("Convert luminance float tex to int %d x %d\n", width, height);
      return GL_LUMINANCE;
   }
   else if (internalFormat == GL_ALPHA16F_ARB ||
            internalFormat == GL_ALPHA32F_ARB) {
      printf("Convert luminance float tex to int %d x %d\n", width, height);
      return GL_ALPHA;
   }
   /*
   else if (internalFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) {
      internalFormat = GL_RGBA;
   }
   */
   else {
      return internalFormat;
   }
#else
   return internalFormat;
#endif
}


/**
 * Choose the actual hardware format for a texture image.
 * Try to use the same format as the previous image level when possible.
 * Otherwise, ask the driver for the best format.
 * It's important to try to choose a consistant format for all levels
 * for efficient texture memory layout/allocation.  In particular, this
 * comes up during automatic mipmap generation.
 */
gl_format
_mesa_choose_texture_format(struct gl_context *ctx,
                            struct gl_texture_object *texObj,
                            GLenum target, GLint level,
                            GLenum internalFormat, GLenum format, GLenum type)
{
   gl_format f;

   /* see if we've already chosen a format for the previous level */
   if (level > 0) {
      struct gl_texture_image *prevImage =
	 _mesa_select_tex_image(ctx, texObj, target, level - 1);
      /* See if the prev level is defined and has an internal format which
       * matches the new internal format.
       */
      if (prevImage &&
          prevImage->Width > 0 &&
          prevImage->InternalFormat == internalFormat) {
         /* use the same format */
         ASSERT(prevImage->TexFormat != MESA_FORMAT_NONE);
         return prevImage->TexFormat;
      }
   }

   /* If the application requested compression to an S3TC format but we don't
    * have the DTXn library, force a generic compressed format instead.
    */
   if (internalFormat != format && format != GL_NONE) {
      const GLenum before = internalFormat;

      switch (internalFormat) {
      case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
         if (!ctx->Mesa_DXTn)
            internalFormat = GL_COMPRESSED_RGB;
         break;
      case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
      case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
      case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
         if (!ctx->Mesa_DXTn)
            internalFormat = GL_COMPRESSED_RGBA;
         break;
      default:
         break;
      }

      if (before != internalFormat) {
         _mesa_warning(ctx,
                       "DXT compression requested (%s), "
                       "but libtxc_dxtn library not installed.  Using %s "
                       "instead.",
                       _mesa_lookup_enum_by_nr(before),
                       _mesa_lookup_enum_by_nr(internalFormat));
      }
   }

   /* choose format from scratch */
   f = ctx->Driver.ChooseTextureFormat(ctx, texObj->Target, internalFormat,
                                       format, type);
   ASSERT(f != MESA_FORMAT_NONE);
   return f;
}

/**
 * Adjust pixel unpack params and image dimensions to strip off the
 * one-pixel texture border.
 *
 * Gallium and intel don't support texture borders.  They've seldem been used
 * and seldom been implemented correctly anyway.
 *
 * \param unpackNew returns the new pixel unpack parameters
 */
static void
strip_texture_border(GLenum target,
                     GLint *width, GLint *height, GLint *depth,
                     const struct gl_pixelstore_attrib *unpack,
                     struct gl_pixelstore_attrib *unpackNew)
{
   assert(width);
   assert(height);
   assert(depth);

   *unpackNew = *unpack;

   if (unpackNew->RowLength == 0)
      unpackNew->RowLength = *width;

   if (unpackNew->ImageHeight == 0)
      unpackNew->ImageHeight = *height;

   assert(*width >= 3);
   unpackNew->SkipPixels++;  /* skip the border */
   *width = *width - 2;      /* reduce the width by two border pixels */

   /* The min height of a texture with a border is 3 */
   if (*height >= 3 && target != GL_TEXTURE_1D_ARRAY) {
      unpackNew->SkipRows++;  /* skip the border */
      *height = *height - 2;  /* reduce the height by two border pixels */
   }

   if (*depth >= 3 && target != GL_TEXTURE_2D_ARRAY && target != GL_TEXTURE_CUBE_MAP_ARRAY) {
      unpackNew->SkipImages++;  /* skip the border */
      *depth = *depth - 2;      /* reduce the depth by two border pixels */
   }
}


/**
 * Common code to implement all the glTexImage1D/2D/3D functions
 * as well as glCompressedTexImage1D/2D/3D.
 * \param compressed  only GL_TRUE for glCompressedTexImage1D/2D/3D calls.
 * \param format  the user's image format (only used if !compressed)
 * \param type  the user's image type (only used if !compressed)
 * \param imageSize  only used for glCompressedTexImage1D/2D/3D calls.
 */
static void
teximage(struct gl_context *ctx, GLboolean compressed, GLuint dims,
         GLenum target, GLint level, GLint internalFormat,
         GLsizei width, GLsizei height, GLsizei depth,
         GLint border, GLenum format, GLenum type,
         GLsizei imageSize, const GLvoid *pixels)
{
   const char *func = compressed ? "glCompressedTexImage" : "glTexImage";
   struct gl_pixelstore_attrib unpack_no_border;
   const struct gl_pixelstore_attrib *unpack = &ctx->Unpack;
   struct gl_texture_object *texObj;
   gl_format texFormat;
   GLboolean dimensionsOK, sizeOK;

   FLUSH_VERTICES(ctx, 0);

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE)) {
      if (compressed)
         _mesa_debug(ctx,
                     "glCompressedTexImage%uD %s %d %s %d %d %d %d %p\n",
                     dims,
                     _mesa_lookup_enum_by_nr(target), level,
                     _mesa_lookup_enum_by_nr(internalFormat),
                     width, height, depth, border, pixels);
      else
         _mesa_debug(ctx,
                     "glTexImage%uD %s %d %s %d %d %d %d %s %s %p\n",
                     dims,
                     _mesa_lookup_enum_by_nr(target), level,
                     _mesa_lookup_enum_by_nr(internalFormat),
                     width, height, depth, border,
                     _mesa_lookup_enum_by_nr(format),
                     _mesa_lookup_enum_by_nr(type), pixels);
   }

   internalFormat = override_internal_format(internalFormat, width, height);

   /* target error checking */
   if (!legal_teximage_target(ctx, dims, target)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "%s%uD(target=%s)",
                  func, dims, _mesa_lookup_enum_by_nr(target));
      return;
   }

   /* general error checking */
   if (compressed) {
      if (compressed_texture_error_check(ctx, dims, target, level,
                                         internalFormat,
                                         width, height, depth,
                                         border, imageSize))
         return;
   }
   else {
      if (texture_error_check(ctx, dims, target, level, internalFormat,
                              format, type, width, height, depth, border))
         return;
   }

   /* Here we convert a cpal compressed image into a regular glTexImage2D
    * call by decompressing the texture.  If we really want to support cpal
    * textures in any driver this would have to be changed.
    */
   if (ctx->API == API_OPENGLES && compressed && dims == 2) {
      switch (internalFormat) {
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
         _mesa_cpal_compressed_teximage2d(target, level, internalFormat,
                                          width, height, imageSize, pixels);
         return;
      }
   }

   texObj = _mesa_get_current_tex_object(ctx, target);
   assert(texObj);

   if (compressed) {
      /* For glCompressedTexImage() the driver has no choice about the
       * texture format since we'll never transcode the user's compressed
       * image data.  The internalFormat was error checked earlier.
       */
      texFormat = _mesa_glenum_to_compressed_format(internalFormat);
   }
   else {
      texFormat = _mesa_choose_texture_format(ctx, texObj, target, level,
                                              internalFormat, format, type);
   }

   assert(texFormat != MESA_FORMAT_NONE);

   /* check that width, height, depth are legal for the mipmap level */
   dimensionsOK = _mesa_legal_texture_dimensions(ctx, target, level, width,
                                                 height, depth, border);

   /* check that the texture won't take too much memory, etc */
   sizeOK = ctx->Driver.TestProxyTexImage(ctx, _mesa_get_proxy_target(target),
                                          level, texFormat,
                                          width, height, depth, border);

   if (_mesa_is_proxy_texture(target)) {
      /* Proxy texture: just clear or set state depending on error checking */
      struct gl_texture_image *texImage =
         get_proxy_tex_image(ctx, target, level);

      if (!texImage)
         return;  /* GL_OUT_OF_MEMORY already recorded */

      if (dimensionsOK && sizeOK) {
         _mesa_init_teximage_fields(ctx, texImage, width, height, depth,
                                    border, internalFormat, texFormat);
      }
      else {
         clear_teximage_fields(texImage);
      }
   }
   else {
      /* non-proxy target */
      const GLuint face = _mesa_tex_target_to_face(target);
      struct gl_texture_image *texImage;

      if (!dimensionsOK) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glTexImage%uD(invalid width or height or depth)",
                     dims);
         return;
      }

      if (!sizeOK) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY,
                     "glTexImage%uD(image too large)", dims);
         return;
      }

      /* Allow a hardware driver to just strip out the border, to provide
       * reliable but slightly incorrect hardware rendering instead of
       * rarely-tested software fallback rendering.
       */
      if (border && ctx->Const.StripTextureBorder) {
	 strip_texture_border(target, &width, &height, &depth, unpack,
			      &unpack_no_border);
         border = 0;
	 unpack = &unpack_no_border;
      }

      if (ctx->NewState & _NEW_PIXEL)
	 _mesa_update_state(ctx);

      _mesa_lock_texture(ctx, texObj);
      {
	 texImage = _mesa_get_tex_image(ctx, texObj, target, level);

	 if (!texImage) {
	    _mesa_error(ctx, GL_OUT_OF_MEMORY, "%s%uD", func, dims);
	 }
         else {
            ctx->Driver.FreeTextureImageBuffer(ctx, texImage);

            _mesa_init_teximage_fields(ctx, texImage,
                                       width, height, depth,
                                       border, internalFormat, texFormat);

            /* Give the texture to the driver.  <pixels> may be null. */
            if (width > 0 && height > 0 && depth > 0) {
               if (compressed) {
                  ctx->Driver.CompressedTexImage(ctx, dims, texImage,
                                                 imageSize, pixels);
               }
               else {
                  ctx->Driver.TexImage(ctx, dims, texImage, format,
                                       type, pixels, unpack);
               }
            }

            check_gen_mipmap(ctx, target, texObj, level);

            _mesa_update_fbo_texture(ctx, texObj, face, level);

            _mesa_dirty_texobj(ctx, texObj, GL_TRUE);
         }
      }
      _mesa_unlock_texture(ctx, texObj);
   }
}



/*
 * Called from the API.  Note that width includes the border.
 */
void GLAPIENTRY
_mesa_TexImage1D( GLenum target, GLint level, GLint internalFormat,
                  GLsizei width, GLint border, GLenum format,
                  GLenum type, const GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   teximage(ctx, GL_FALSE, 1, target, level, internalFormat, width, 1, 1,
            border, format, type, 0, pixels);
}


void GLAPIENTRY
_mesa_TexImage2D( GLenum target, GLint level, GLint internalFormat,
                  GLsizei width, GLsizei height, GLint border,
                  GLenum format, GLenum type,
                  const GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   teximage(ctx, GL_FALSE, 2, target, level, internalFormat, width, height, 1,
            border, format, type, 0, pixels);
}


/*
 * Called by the API or display list executor.
 * Note that width and height include the border.
 */
void GLAPIENTRY
_mesa_TexImage3D( GLenum target, GLint level, GLint internalFormat,
                  GLsizei width, GLsizei height, GLsizei depth,
                  GLint border, GLenum format, GLenum type,
                  const GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   teximage(ctx, GL_FALSE, 3, target, level, internalFormat,
            width, height, depth,
            border, format, type, 0, pixels);
}


void GLAPIENTRY
_mesa_TexImage3DEXT( GLenum target, GLint level, GLenum internalFormat,
                     GLsizei width, GLsizei height, GLsizei depth,
                     GLint border, GLenum format, GLenum type,
                     const GLvoid *pixels )
{
   _mesa_TexImage3D(target, level, (GLint) internalFormat, width, height,
                    depth, border, format, type, pixels);
}


void GLAPIENTRY
_mesa_EGLImageTargetTexture2DOES (GLenum target, GLeglImageOES image)
{
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   bool valid_target;
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);

   switch (target) {
   case GL_TEXTURE_2D:
      valid_target = ctx->Extensions.OES_EGL_image;
      break;
   case GL_TEXTURE_EXTERNAL_OES:
      valid_target = ctx->Extensions.OES_EGL_image_external;
      break;
   default:
      valid_target = false;
      break;
   }

   if (!valid_target) {
      _mesa_error(ctx, GL_INVALID_ENUM,
		  "glEGLImageTargetTexture2D(target=%d)", target);
      return;
   }

   if (!image) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
		  "glEGLImageTargetTexture2D(image=%p)", image);
      return;
   }

   if (ctx->NewState & _NEW_PIXEL)
      _mesa_update_state(ctx);

   texObj = _mesa_get_current_tex_object(ctx, target);
   _mesa_lock_texture(ctx, texObj);

   if (texObj->Immutable) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
		  "glEGLImageTargetTexture2D(texture is immutable)");
      _mesa_unlock_texture(ctx, texObj);
      return;
   }

   texImage = _mesa_get_tex_image(ctx, texObj, target, 0);
   if (!texImage) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glEGLImageTargetTexture2D");
   } else {
      ctx->Driver.FreeTextureImageBuffer(ctx, texImage);

      ctx->Driver.EGLImageTargetTexture2D(ctx, target,
					  texObj, texImage, image);

      _mesa_dirty_texobj(ctx, texObj, GL_TRUE);
   }
   _mesa_unlock_texture(ctx, texObj);

}



/**
 * Implement all the glTexSubImage1/2/3D() functions.
 */
static void
texsubimage(struct gl_context *ctx, GLuint dims, GLenum target, GLint level,
            GLint xoffset, GLint yoffset, GLint zoffset,
            GLsizei width, GLsizei height, GLsizei depth,
            GLenum format, GLenum type, const GLvoid *pixels )
{
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;

   FLUSH_VERTICES(ctx, 0);

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glTexSubImage%uD %s %d %d %d %d %d %d %d %s %s %p\n",
                  dims,
                  _mesa_lookup_enum_by_nr(target), level,
                  xoffset, yoffset, zoffset, width, height, depth,
                  _mesa_lookup_enum_by_nr(format),
                  _mesa_lookup_enum_by_nr(type), pixels);

   /* check target (proxies not allowed) */
   if (!legal_texsubimage_target(ctx, dims, target)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glTexSubImage%uD(target=%s)",
                  dims, _mesa_lookup_enum_by_nr(target));
      return;
   }       

   if (ctx->NewState & _NEW_PIXEL)
      _mesa_update_state(ctx);

   if (texsubimage_error_check(ctx, dims, target, level,
                               xoffset, yoffset, zoffset,
                               width, height, depth, format, type)) {
      return;   /* error was detected */
   }

   texObj = _mesa_get_current_tex_object(ctx, target);

   _mesa_lock_texture(ctx, texObj);
   {
      texImage = _mesa_select_tex_image(ctx, texObj, target, level);

      if (width > 0 && height > 0 && depth > 0) {
         /* If we have a border, offset=-1 is legal.  Bias by border width. */
         switch (dims) {
         case 3:
            if (target != GL_TEXTURE_2D_ARRAY)
               zoffset += texImage->Border;
            /* fall-through */
         case 2:
            if (target != GL_TEXTURE_1D_ARRAY)
               yoffset += texImage->Border;
            /* fall-through */
         case 1:
            xoffset += texImage->Border;
         }

         ctx->Driver.TexSubImage(ctx, dims, texImage,
                                 xoffset, yoffset, zoffset,
                                 width, height, depth,
                                 format, type, pixels, &ctx->Unpack);

         check_gen_mipmap(ctx, target, texObj, level);

         ctx->NewState |= _NEW_TEXTURE;
      }
   }
   _mesa_unlock_texture(ctx, texObj);
}


void GLAPIENTRY
_mesa_TexSubImage1D( GLenum target, GLint level,
                     GLint xoffset, GLsizei width,
                     GLenum format, GLenum type,
                     const GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   texsubimage(ctx, 1, target, level,
               xoffset, 0, 0,
               width, 1, 1,
               format, type, pixels);
}


void GLAPIENTRY
_mesa_TexSubImage2D( GLenum target, GLint level,
                     GLint xoffset, GLint yoffset,
                     GLsizei width, GLsizei height,
                     GLenum format, GLenum type,
                     const GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   texsubimage(ctx, 2, target, level,
               xoffset, yoffset, 0,
               width, height, 1,
               format, type, pixels);
}



void GLAPIENTRY
_mesa_TexSubImage3D( GLenum target, GLint level,
                     GLint xoffset, GLint yoffset, GLint zoffset,
                     GLsizei width, GLsizei height, GLsizei depth,
                     GLenum format, GLenum type,
                     const GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   texsubimage(ctx, 3, target, level,
               xoffset, yoffset, zoffset,
               width, height, depth,
               format, type, pixels);
}



/**
 * For glCopyTexSubImage, return the source renderbuffer to copy texel data
 * from.  This depends on whether the texture contains color or depth values.
 */
static struct gl_renderbuffer *
get_copy_tex_image_source(struct gl_context *ctx, gl_format texFormat)
{
   if (_mesa_get_format_bits(texFormat, GL_DEPTH_BITS) > 0) {
      /* reading from depth/stencil buffer */
      return ctx->ReadBuffer->Attachment[BUFFER_DEPTH].Renderbuffer;
   }
   else {
      /* copying from color buffer */
      return ctx->ReadBuffer->_ColorReadBuffer;
   }
}



/**
 * Implement the glCopyTexImage1/2D() functions.
 */
static void
copyteximage(struct gl_context *ctx, GLuint dims,
             GLenum target, GLint level, GLenum internalFormat,
             GLint x, GLint y, GLsizei width, GLsizei height, GLint border )
{
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   const GLuint face = _mesa_tex_target_to_face(target);
   gl_format texFormat;

   FLUSH_VERTICES(ctx, 0);

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glCopyTexImage%uD %s %d %s %d %d %d %d %d\n",
                  dims,
                  _mesa_lookup_enum_by_nr(target), level,
                  _mesa_lookup_enum_by_nr(internalFormat),
                  x, y, width, height, border);

   if (ctx->NewState & NEW_COPY_TEX_STATE)
      _mesa_update_state(ctx);

   if (copytexture_error_check(ctx, dims, target, level, internalFormat,
                               width, height, border))
      return;

   if (!_mesa_legal_texture_dimensions(ctx, target, level, width, height,
                                       1, border)) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glCopyTexImage%uD(invalid width or height)", dims);
      return;
   }

   texObj = _mesa_get_current_tex_object(ctx, target);
   assert(texObj);

   texFormat = _mesa_choose_texture_format(ctx, texObj, target, level,
                                           internalFormat, GL_NONE, GL_NONE);
   assert(texFormat != MESA_FORMAT_NONE);

   if (!ctx->Driver.TestProxyTexImage(ctx, _mesa_get_proxy_target(target),
                                      level, texFormat,
                                      width, height, 1, border)) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY,
                  "glCopyTexImage%uD(image too large)", dims);
      return;
   }

   if (border && ctx->Const.StripTextureBorder) {
      x += border;
      width -= border * 2;
      if (dims == 2) {
	 y += border;
	 height -= border * 2;
      }
      border = 0;
   }

   _mesa_lock_texture(ctx, texObj);
   {
      texImage = _mesa_get_tex_image(ctx, texObj, target, level);

      if (!texImage) {
	 _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyTexImage%uD", dims);
      }
      else {
         GLint srcX = x, srcY = y, dstX = 0, dstY = 0, dstZ = 0;

         /* Free old texture image */
         ctx->Driver.FreeTextureImageBuffer(ctx, texImage);

         _mesa_init_teximage_fields(ctx, texImage, width, height, 1,
                                    border, internalFormat, texFormat);

         if (width && height) {
            /* Allocate texture memory (no pixel data yet) */
            ctx->Driver.AllocTextureImageBuffer(ctx, texImage);

            if (_mesa_clip_copytexsubimage(ctx, &dstX, &dstY, &srcX, &srcY,
                                           &width, &height)) {
               struct gl_renderbuffer *srcRb =
                  get_copy_tex_image_source(ctx, texImage->TexFormat);

               ctx->Driver.CopyTexSubImage(ctx, dims, texImage, dstX, dstY, dstZ,
                                           srcRb, srcX, srcY, width, height);
            }

            check_gen_mipmap(ctx, target, texObj, level);
         }

         _mesa_update_fbo_texture(ctx, texObj, face, level);

         _mesa_dirty_texobj(ctx, texObj, GL_TRUE);
      }
   }
   _mesa_unlock_texture(ctx, texObj);
}



void GLAPIENTRY
_mesa_CopyTexImage1D( GLenum target, GLint level,
                      GLenum internalFormat,
                      GLint x, GLint y,
                      GLsizei width, GLint border )
{
   GET_CURRENT_CONTEXT(ctx);
   copyteximage(ctx, 1, target, level, internalFormat, x, y, width, 1, border);
}



void GLAPIENTRY
_mesa_CopyTexImage2D( GLenum target, GLint level, GLenum internalFormat,
                      GLint x, GLint y, GLsizei width, GLsizei height,
                      GLint border )
{
   GET_CURRENT_CONTEXT(ctx);
   copyteximage(ctx, 2, target, level, internalFormat,
                x, y, width, height, border);
}



/**
 * Implementation for glCopyTexSubImage1/2/3D() functions.
 */
static void
copytexsubimage(struct gl_context *ctx, GLuint dims, GLenum target, GLint level,
                GLint xoffset, GLint yoffset, GLint zoffset,
                GLint x, GLint y, GLsizei width, GLsizei height)
{
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;

   FLUSH_VERTICES(ctx, 0);

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glCopyTexSubImage%uD %s %d %d %d %d %d %d %d %d\n",
                  dims,
                  _mesa_lookup_enum_by_nr(target),
                  level, xoffset, yoffset, zoffset, x, y, width, height);

   if (ctx->NewState & NEW_COPY_TEX_STATE)
      _mesa_update_state(ctx);

   if (copytexsubimage_error_check(ctx, dims, target, level,
                                   xoffset, yoffset, zoffset, width, height)) {
      return;
   }

   texObj = _mesa_get_current_tex_object(ctx, target);

   _mesa_lock_texture(ctx, texObj);
   {
      texImage = _mesa_select_tex_image(ctx, texObj, target, level);

      /* If we have a border, offset=-1 is legal.  Bias by border width. */
      switch (dims) {
      case 3:
         if (target != GL_TEXTURE_2D_ARRAY)
            zoffset += texImage->Border;
         /* fall-through */
      case 2:
         if (target != GL_TEXTURE_1D_ARRAY)
            yoffset += texImage->Border;
         /* fall-through */
      case 1:
         xoffset += texImage->Border;
      }

      if (_mesa_clip_copytexsubimage(ctx, &xoffset, &yoffset, &x, &y,
                                     &width, &height)) {
         struct gl_renderbuffer *srcRb =
            get_copy_tex_image_source(ctx, texImage->TexFormat);

         ctx->Driver.CopyTexSubImage(ctx, dims, texImage,
                                     xoffset, yoffset, zoffset,
                                     srcRb, x, y, width, height);

         check_gen_mipmap(ctx, target, texObj, level);

         ctx->NewState |= _NEW_TEXTURE;
      }
   }
   _mesa_unlock_texture(ctx, texObj);
}


void GLAPIENTRY
_mesa_CopyTexSubImage1D( GLenum target, GLint level,
                         GLint xoffset, GLint x, GLint y, GLsizei width )
{
   GET_CURRENT_CONTEXT(ctx);
   copytexsubimage(ctx, 1, target, level, xoffset, 0, 0, x, y, width, 1);
}



void GLAPIENTRY
_mesa_CopyTexSubImage2D( GLenum target, GLint level,
                         GLint xoffset, GLint yoffset,
                         GLint x, GLint y, GLsizei width, GLsizei height )
{
   GET_CURRENT_CONTEXT(ctx);
   copytexsubimage(ctx, 2, target, level, xoffset, yoffset, 0, x, y,
                   width, height);
}



void GLAPIENTRY
_mesa_CopyTexSubImage3D( GLenum target, GLint level,
                         GLint xoffset, GLint yoffset, GLint zoffset,
                         GLint x, GLint y, GLsizei width, GLsizei height )
{
   GET_CURRENT_CONTEXT(ctx);
   copytexsubimage(ctx, 3, target, level, xoffset, yoffset, zoffset,
                   x, y, width, height);
}




/**********************************************************************/
/******                   Compressed Textures                    ******/
/**********************************************************************/


/**
 * Error checking for glCompressedTexSubImage[123]D().
 * \return error code or GL_NO_ERROR.
 */
static GLenum
compressed_subtexture_error_check(struct gl_context *ctx, GLint dims,
                                  GLenum target, GLint level,
                                  GLint xoffset, GLint yoffset, GLint zoffset,
                                  GLsizei width, GLsizei height, GLsizei depth,
                                  GLenum format, GLsizei imageSize)
{
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   GLint expectedSize;
   GLboolean targetOK;

   switch (dims) {
   case 2:
      switch (target) {
      case GL_TEXTURE_2D:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
         targetOK = GL_TRUE;
         break;
      default:
         targetOK = GL_FALSE;
         break;
      }
      break;
   case 3:
      targetOK = (target == GL_TEXTURE_2D_ARRAY);
      break;
   default:
      assert(dims == 1);
      /* no 1D compressed textures at this time */
      targetOK = GL_FALSE;
      break;
   }
 
   if (!targetOK) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glCompressedTexImage%uD(target)",
                  dims);
      return GL_TRUE;
   }

   /* this will catch any invalid compressed format token */
   if (!_mesa_is_compressed_format(ctx, format)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glCompressedTexImage%uD(format)",
                  dims);
      return GL_TRUE;
   }

   if (level < 0 || level >= _mesa_max_texture_levels(ctx, target)) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glCompressedTexImage%uD(level=%d)",
                  dims, level);
      return GL_TRUE;
   }

   expectedSize = compressed_tex_size(width, height, depth, format);
   if (expectedSize != imageSize) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glCompressedTexImage%uD(size=%d)",
                  dims, imageSize);
      return GL_TRUE;
   }

   texObj = _mesa_get_current_tex_object(ctx, target);
   if (!texObj) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY,
                  "glCompressedTexSubImage%uD()", dims);
      return GL_TRUE;
   }

   texImage = _mesa_select_tex_image(ctx, texObj, target, level);
   if (!texImage) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glCompressedTexSubImage%uD(invalid texture image)", dims);
      return GL_TRUE;
   }

   if ((GLint) format != texImage->InternalFormat) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glCompressedTexSubImage%uD(format=0x%x)", dims, format);
      return GL_TRUE;
   }

   if (compressedteximage_only_format(ctx, format)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glCompressedTexSubImage%uD(format=0x%x cannot be updated)"
                  , dims, format);
      return GL_TRUE;
   }

   if (error_check_subtexture_dimensions(ctx, "glCompressedTexSubImage", dims,
                                         texImage, xoffset, yoffset, zoffset,
                                         width, height, depth)) {
      return GL_TRUE;
   }

   return GL_FALSE;
}


void GLAPIENTRY
_mesa_CompressedTexImage1D(GLenum target, GLint level,
                              GLenum internalFormat, GLsizei width,
                              GLint border, GLsizei imageSize,
                              const GLvoid *data)
{
   GET_CURRENT_CONTEXT(ctx);
   teximage(ctx, GL_TRUE, 1, target, level, internalFormat,
            width, 1, 1, border, GL_NONE, GL_NONE, imageSize, data);
}


void GLAPIENTRY
_mesa_CompressedTexImage2D(GLenum target, GLint level,
                              GLenum internalFormat, GLsizei width,
                              GLsizei height, GLint border, GLsizei imageSize,
                              const GLvoid *data)
{
   GET_CURRENT_CONTEXT(ctx);
   teximage(ctx, GL_TRUE, 2, target, level, internalFormat,
            width, height, 1, border, GL_NONE, GL_NONE, imageSize, data);
}


void GLAPIENTRY
_mesa_CompressedTexImage3D(GLenum target, GLint level,
                              GLenum internalFormat, GLsizei width,
                              GLsizei height, GLsizei depth, GLint border,
                              GLsizei imageSize, const GLvoid *data)
{
   GET_CURRENT_CONTEXT(ctx);
   teximage(ctx, GL_TRUE, 3, target, level, internalFormat,
            width, height, depth, border, GL_NONE, GL_NONE, imageSize, data);
}


/**
 * Common helper for glCompressedTexSubImage1/2/3D().
 */
static void
compressed_tex_sub_image(GLuint dims, GLenum target, GLint level,
                         GLint xoffset, GLint yoffset, GLint zoffset,
                         GLsizei width, GLsizei height, GLsizei depth,
                         GLenum format, GLsizei imageSize, const GLvoid *data)
{
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);

   if (compressed_subtexture_error_check(ctx, dims, target, level,
                                         xoffset, yoffset, zoffset,
                                         width, height, depth,
                                         format, imageSize)) {
      return;
   }

   texObj = _mesa_get_current_tex_object(ctx, target);

   _mesa_lock_texture(ctx, texObj);
   {
      texImage = _mesa_select_tex_image(ctx, texObj, target, level);
      assert(texImage);

      if (width > 0 && height > 0 && depth > 0) {
         ctx->Driver.CompressedTexSubImage(ctx, dims, texImage,
                                           xoffset, yoffset, zoffset,
                                           width, height, depth,
                                           format, imageSize, data);

         check_gen_mipmap(ctx, target, texObj, level);

         ctx->NewState |= _NEW_TEXTURE;
      }
   }
   _mesa_unlock_texture(ctx, texObj);
}


void GLAPIENTRY
_mesa_CompressedTexSubImage1D(GLenum target, GLint level, GLint xoffset,
                                 GLsizei width, GLenum format,
                                 GLsizei imageSize, const GLvoid *data)
{
   compressed_tex_sub_image(1, target, level, xoffset, 0, 0, width, 1, 1,
                            format, imageSize, data);
}


void GLAPIENTRY
_mesa_CompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset,
                                 GLint yoffset, GLsizei width, GLsizei height,
                                 GLenum format, GLsizei imageSize,
                                 const GLvoid *data)
{
   compressed_tex_sub_image(2, target, level, xoffset, yoffset, 0,
                            width, height, 1, format, imageSize, data);
}


void GLAPIENTRY
_mesa_CompressedTexSubImage3D(GLenum target, GLint level, GLint xoffset,
                                 GLint yoffset, GLint zoffset, GLsizei width,
                                 GLsizei height, GLsizei depth, GLenum format,
                                 GLsizei imageSize, const GLvoid *data)
{
   compressed_tex_sub_image(3, target, level, xoffset, yoffset, zoffset,
                            width, height, depth, format, imageSize, data);
}

static gl_format
get_texbuffer_format(const struct gl_context *ctx, GLenum internalFormat)
{
   switch (internalFormat) {
   case GL_ALPHA8:
      return MESA_FORMAT_A8;
   case GL_ALPHA16:
      return MESA_FORMAT_A16;
   case GL_ALPHA16F_ARB:
      return MESA_FORMAT_ALPHA_FLOAT16;
   case GL_ALPHA32F_ARB:
      return MESA_FORMAT_ALPHA_FLOAT32;
   case GL_ALPHA8I_EXT:
      return MESA_FORMAT_ALPHA_INT8;
   case GL_ALPHA16I_EXT:
      return MESA_FORMAT_ALPHA_INT16;
   case GL_ALPHA32I_EXT:
      return MESA_FORMAT_ALPHA_INT32;
   case GL_ALPHA8UI_EXT:
      return MESA_FORMAT_ALPHA_UINT8;
   case GL_ALPHA16UI_EXT:
      return MESA_FORMAT_ALPHA_UINT16;
   case GL_ALPHA32UI_EXT:
      return MESA_FORMAT_ALPHA_UINT32;
   case GL_LUMINANCE8:
      return MESA_FORMAT_L8;
   case GL_LUMINANCE16:
      return MESA_FORMAT_L16;
   case GL_LUMINANCE16F_ARB:
      return MESA_FORMAT_LUMINANCE_FLOAT16;
   case GL_LUMINANCE32F_ARB:
      return MESA_FORMAT_LUMINANCE_FLOAT32;
   case GL_LUMINANCE8I_EXT:
      return MESA_FORMAT_LUMINANCE_INT8;
   case GL_LUMINANCE16I_EXT:
      return MESA_FORMAT_LUMINANCE_INT16;
   case GL_LUMINANCE32I_EXT:
      return MESA_FORMAT_LUMINANCE_INT32;
   case GL_LUMINANCE8UI_EXT:
      return MESA_FORMAT_LUMINANCE_UINT8;
   case GL_LUMINANCE16UI_EXT:
      return MESA_FORMAT_LUMINANCE_UINT16;
   case GL_LUMINANCE32UI_EXT:
      return MESA_FORMAT_LUMINANCE_UINT32;
   case GL_LUMINANCE8_ALPHA8:
      return MESA_FORMAT_AL88;
   case GL_LUMINANCE16_ALPHA16:
      return MESA_FORMAT_AL1616;
   case GL_LUMINANCE_ALPHA16F_ARB:
      return MESA_FORMAT_LUMINANCE_ALPHA_FLOAT16;
   case GL_LUMINANCE_ALPHA32F_ARB:
      return MESA_FORMAT_LUMINANCE_ALPHA_FLOAT32;
   case GL_LUMINANCE_ALPHA8I_EXT:
      return MESA_FORMAT_LUMINANCE_ALPHA_INT8;
   case GL_LUMINANCE_ALPHA16I_EXT:
      return MESA_FORMAT_LUMINANCE_ALPHA_INT8;
   case GL_LUMINANCE_ALPHA32I_EXT:
      return MESA_FORMAT_LUMINANCE_ALPHA_INT16;
   case GL_LUMINANCE_ALPHA8UI_EXT:
      return MESA_FORMAT_LUMINANCE_ALPHA_UINT8;
   case GL_LUMINANCE_ALPHA16UI_EXT:
      return MESA_FORMAT_LUMINANCE_ALPHA_UINT16;
   case GL_LUMINANCE_ALPHA32UI_EXT:
      return MESA_FORMAT_LUMINANCE_ALPHA_UINT32;
   case GL_INTENSITY8:
      return MESA_FORMAT_I8;
   case GL_INTENSITY16:
      return MESA_FORMAT_I16;
   case GL_INTENSITY16F_ARB:
      return MESA_FORMAT_INTENSITY_FLOAT16;
   case GL_INTENSITY32F_ARB:
      return MESA_FORMAT_INTENSITY_FLOAT32;
   case GL_INTENSITY8I_EXT:
      return MESA_FORMAT_INTENSITY_INT8;
   case GL_INTENSITY16I_EXT:
      return MESA_FORMAT_INTENSITY_INT16;
   case GL_INTENSITY32I_EXT:
      return MESA_FORMAT_INTENSITY_INT32;
   case GL_INTENSITY8UI_EXT:
      return MESA_FORMAT_INTENSITY_UINT8;
   case GL_INTENSITY16UI_EXT:
      return MESA_FORMAT_INTENSITY_UINT16;
   case GL_INTENSITY32UI_EXT:
      return MESA_FORMAT_INTENSITY_UINT32;
   case GL_RGBA8:
      return MESA_FORMAT_RGBA8888_REV;
   case GL_RGBA16:
      return MESA_FORMAT_RGBA_16;
   case GL_RGBA16F_ARB:
      return MESA_FORMAT_RGBA_FLOAT16;
   case GL_RGBA32F_ARB:
      return MESA_FORMAT_RGBA_FLOAT32;
   case GL_RGBA8I_EXT:
      return MESA_FORMAT_RGBA_INT8;
   case GL_RGBA16I_EXT:
      return MESA_FORMAT_RGBA_INT16;
   case GL_RGBA32I_EXT:
      return MESA_FORMAT_RGBA_INT32;
   case GL_RGBA8UI_EXT:
      return MESA_FORMAT_RGBA_UINT8;
   case GL_RGBA16UI_EXT:
      return MESA_FORMAT_RGBA_UINT16;
   case GL_RGBA32UI_EXT:
      return MESA_FORMAT_RGBA_UINT32;

   case GL_RG8:
      return MESA_FORMAT_GR88;
   case GL_RG16:
      return MESA_FORMAT_GR1616;
   case GL_RG16F:
      return MESA_FORMAT_RG_FLOAT16;
   case GL_RG32F:
      return MESA_FORMAT_RG_FLOAT32;
   case GL_RG8I:
      return MESA_FORMAT_RG_INT8;
   case GL_RG16I:
      return MESA_FORMAT_RG_INT16;
   case GL_RG32I:
      return MESA_FORMAT_RG_INT32;
   case GL_RG8UI:
      return MESA_FORMAT_RG_UINT8;
   case GL_RG16UI:
      return MESA_FORMAT_RG_UINT16;
   case GL_RG32UI:
      return MESA_FORMAT_RG_UINT32;

   case GL_R8:
      return MESA_FORMAT_R8;
   case GL_R16:
      return MESA_FORMAT_R16;
   case GL_R16F:
      return MESA_FORMAT_R_FLOAT16;
   case GL_R32F:
      return MESA_FORMAT_R_FLOAT32;
   case GL_R8I:
      return MESA_FORMAT_R_INT8;
   case GL_R16I:
      return MESA_FORMAT_R_INT16;
   case GL_R32I:
      return MESA_FORMAT_R_INT32;
   case GL_R8UI:
      return MESA_FORMAT_R_UINT8;
   case GL_R16UI:
      return MESA_FORMAT_R_UINT16;
   case GL_R32UI:
      return MESA_FORMAT_R_UINT32;

   case GL_RGB32F:
      return MESA_FORMAT_RGB_FLOAT32;
   case GL_RGB32UI:
      return MESA_FORMAT_RGB_UINT32;
   case GL_RGB32I:
      return MESA_FORMAT_RGB_INT32;

   default:
      return MESA_FORMAT_NONE;
   }
}

static gl_format
validate_texbuffer_format(const struct gl_context *ctx, GLenum internalFormat)
{
   gl_format format = get_texbuffer_format(ctx, internalFormat);
   GLenum datatype;

   if (format == MESA_FORMAT_NONE)
      return MESA_FORMAT_NONE;

   datatype = _mesa_get_format_datatype(format);
   if (datatype == GL_FLOAT && !ctx->Extensions.ARB_texture_float)
      return MESA_FORMAT_NONE;

   if (datatype == GL_HALF_FLOAT && !ctx->Extensions.ARB_half_float_pixel)
      return MESA_FORMAT_NONE;

   /* The GL_ARB_texture_rg and GL_ARB_texture_buffer_object specs don't make
    * any mention of R/RG formats, but they appear in the GL 3.1 core
    * specification.
    */
   if (ctx->Version <= 30) {
      GLenum base_format = _mesa_get_format_base_format(format);

      if (base_format == GL_R || base_format == GL_RG)
	 return MESA_FORMAT_NONE;
   }

   if (!ctx->Extensions.ARB_texture_buffer_object_rgb32) {
      GLenum base_format = _mesa_get_format_base_format(format);
      if (base_format == GL_RGB)
         return MESA_FORMAT_NONE;
   }
   return format;
}


static void
texbufferrange(struct gl_context *ctx, GLenum target, GLenum internalFormat,
               struct gl_buffer_object *bufObj,
               GLintptr offset, GLsizeiptr size)
{
   struct gl_texture_object *texObj;
   gl_format format;

   FLUSH_VERTICES(ctx, 0);

   if (target != GL_TEXTURE_BUFFER_ARB) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glTexBuffer(target)");
      return;
   }

   format = validate_texbuffer_format(ctx, internalFormat);
   if (format == MESA_FORMAT_NONE) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glTexBuffer(internalFormat 0x%x)",
                  internalFormat);
      return;
   }

   texObj = _mesa_get_current_tex_object(ctx, target);

   _mesa_lock_texture(ctx, texObj);
   {
      _mesa_reference_buffer_object(ctx, &texObj->BufferObject, bufObj);
      texObj->BufferObjectFormat = internalFormat;
      texObj->_BufferObjectFormat = format;
      texObj->BufferOffset = offset;
      texObj->BufferSize = size;
   }
   _mesa_unlock_texture(ctx, texObj);
}

/** GL_ARB_texture_buffer_object */
void GLAPIENTRY
_mesa_TexBuffer(GLenum target, GLenum internalFormat, GLuint buffer)
{
   struct gl_buffer_object *bufObj;

   GET_CURRENT_CONTEXT(ctx);

   /* NOTE: ARB_texture_buffer_object has interactions with
    * the compatibility profile that are not implemented.
    */
   if (!(ctx->API == API_OPENGL_CORE &&
         ctx->Extensions.ARB_texture_buffer_object)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glTexBuffer");
      return;
   }

   bufObj = _mesa_lookup_bufferobj(ctx, buffer);
   if (!bufObj && buffer) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glTexBuffer(buffer %u)", buffer);
      return;
   }

   texbufferrange(ctx, target, internalFormat, bufObj, 0, buffer ? -1 : 0);
}

/** GL_ARB_texture_buffer_range */
void GLAPIENTRY
_mesa_TexBufferRange(GLenum target, GLenum internalFormat, GLuint buffer,
                     GLintptr offset, GLsizeiptr size)
{
   struct gl_buffer_object *bufObj;

   GET_CURRENT_CONTEXT(ctx);

   if (!(ctx->API == API_OPENGL_CORE &&
         ctx->Extensions.ARB_texture_buffer_range)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glTexBufferRange");
      return;
   }

   bufObj = _mesa_lookup_bufferobj(ctx, buffer);
   if (bufObj) {
      if (offset < 0 ||
          size <= 0 ||
          (offset + size) > bufObj->Size) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glTexBufferRange");
         return;
      }
      if (offset % ctx->Const.TextureBufferOffsetAlignment) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glTexBufferRange(invalid offset alignment)");
         return;
      }
   } else if (buffer) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glTexBufferRange(buffer %u)",
                  buffer);
      return;
   } else {
      offset = 0;
      size = 0;
   }

   texbufferrange(ctx, target, internalFormat, bufObj, offset, size);
}

static GLboolean
is_renderable_texture_format(struct gl_context *ctx, GLenum internalformat)
{
   /* Everything that is allowed for renderbuffers,
    * except for a base format of GL_STENCIL_INDEX.
    */
   GLenum baseFormat = _mesa_base_fbo_format(ctx, internalformat);
   return baseFormat != 0 && baseFormat != GL_STENCIL_INDEX;
}

/** GL_ARB_texture_multisample */
static GLboolean
check_multisample_target(GLuint dims, GLenum target)
{
   switch(target) {
   case GL_TEXTURE_2D_MULTISAMPLE:
   case GL_PROXY_TEXTURE_2D_MULTISAMPLE:
      return dims == 2;

   case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
   case GL_PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY:
      return dims == 3;

   default:
      return GL_FALSE;
   }
}

static void
teximagemultisample(GLuint dims, GLenum target, GLsizei samples,
                    GLint internalformat, GLsizei width, GLsizei height,
                    GLsizei depth, GLboolean fixedsamplelocations,
                    GLboolean immutable, const char *func)
{
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   GLboolean sizeOK, dimensionsOK;
   gl_format texFormat;
   GLenum sample_count_error;

   GET_CURRENT_CONTEXT(ctx);

   if (!(ctx->Extensions.ARB_texture_multisample
      && _mesa_is_desktop_gl(ctx))) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(unsupported)", func);
      return;
   }

   if (!check_multisample_target(dims, target)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "%s(target)", func);
      return;
   }

   /* check that the specified internalformat is color/depth/stencil-renderable;
    * refer GL3.1 spec 4.4.4
    */

   if (immutable && !_mesa_is_legal_tex_storage_format(ctx, internalformat)) {
      _mesa_error(ctx, GL_INVALID_ENUM,
            "%s(internalformat=%s not legal for immutable-format)",
            func, _mesa_lookup_enum_by_nr(internalformat));
      return;
   }

   if (!is_renderable_texture_format(ctx, internalformat)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
            "%s(internalformat=%s)",
            func, _mesa_lookup_enum_by_nr(internalformat));
      return;
   }

   sample_count_error = _mesa_check_sample_count(ctx, target,
         internalformat, samples);
   if (sample_count_error != GL_NO_ERROR) {
      _mesa_error(ctx, sample_count_error, "%s(samples)", func);
      return;
   }

   texObj = _mesa_get_current_tex_object(ctx, target);

   if (immutable && (!texObj || (texObj->Name == 0))) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
            "%s(texture object 0)",
            func);
      return;
   }

   texImage = _mesa_get_tex_image(ctx, texObj, 0, 0);

   if (texImage == NULL) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "%s()", func);
      return;
   }

   texFormat = _mesa_choose_texture_format(ctx, texObj, target, 0,
         internalformat, GL_NONE, GL_NONE);
   assert(texFormat != MESA_FORMAT_NONE);

   dimensionsOK = _mesa_legal_texture_dimensions(ctx, target, 0,
         width, height, depth, 0);

   sizeOK = ctx->Driver.TestProxyTexImage(ctx, target, 0, texFormat,
         width, height, depth, 0);

   if (_mesa_is_proxy_texture(target)) {
      if (dimensionsOK && sizeOK) {
         _mesa_init_teximage_fields(ctx, texImage,
               width, height, depth, 0, internalformat, texFormat);
         texImage->NumSamples = samples;
         texImage->FixedSampleLocations = fixedsamplelocations;
      }
      else {
         /* clear all image fields */
         _mesa_init_teximage_fields(ctx, texImage,
               0, 0, 0, 0, GL_NONE, MESA_FORMAT_NONE);
      }
   }
   else {
      if (!dimensionsOK) {
         _mesa_error(ctx, GL_INVALID_VALUE,
               "%s(invalid width or height)", func);
         return;
      }

      if (!sizeOK) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY,
               "%s(texture too large)", func);
         return;
      }

      /* Check if texObj->Immutable is set */
      if (texObj->Immutable) {
         _mesa_error(ctx, GL_INVALID_OPERATION, "%s(immutable)", func);
         return;
      }

      ctx->Driver.FreeTextureImageBuffer(ctx, texImage);

      _mesa_init_teximage_fields(ctx, texImage,
            width, height, depth, 0, internalformat, texFormat);

      texImage->NumSamples = samples;
      texImage->FixedSampleLocations = fixedsamplelocations;

      if (width > 0 && height > 0 && depth > 0) {

         if (!ctx->Driver.AllocTextureStorage(ctx, texObj, 1,
                  width, height, depth)) {
            /* tidy up the texture image state. strictly speaking,
             * we're allowed to just leave this in whatever state we
             * like, but being tidy is good.
             */
            _mesa_init_teximage_fields(ctx, texImage,
                  0, 0, 0, 0, GL_NONE, MESA_FORMAT_NONE);
         }
      }

      texObj->Immutable = immutable;
      _mesa_update_fbo_texture(ctx, texObj, 0, 0);
   }
}

void GLAPIENTRY
_mesa_TexImage2DMultisample(GLenum target, GLsizei samples,
                            GLint internalformat, GLsizei width,
                            GLsizei height, GLboolean fixedsamplelocations)
{
   teximagemultisample(2, target, samples, internalformat,
         width, height, 1, fixedsamplelocations, GL_FALSE, "glTexImage2DMultisample");
}

void GLAPIENTRY
_mesa_TexImage3DMultisample(GLenum target, GLsizei samples,
                            GLint internalformat, GLsizei width,
                            GLsizei height, GLsizei depth,
                            GLboolean fixedsamplelocations)
{
   teximagemultisample(3, target, samples, internalformat,
         width, height, depth, fixedsamplelocations, GL_FALSE, "glTexImage3DMultisample");
}


void GLAPIENTRY
_mesa_TexStorage2DMultisample(GLenum target, GLsizei samples,
                              GLenum internalformat, GLsizei width,
                              GLsizei height, GLboolean fixedsamplelocations)
{
   teximagemultisample(2, target, samples, internalformat,
         width, height, 1, fixedsamplelocations, GL_TRUE, "glTexStorage2DMultisample");
}

void GLAPIENTRY
_mesa_TexStorage3DMultisample(GLenum target, GLsizei samples,
                              GLenum internalformat, GLsizei width,
                              GLsizei height, GLsizei depth,
                              GLboolean fixedsamplelocations)
{
   teximagemultisample(3, target, samples, internalformat,
         width, height, depth, fixedsamplelocations, GL_TRUE, "glTexStorage3DMultisample");
}

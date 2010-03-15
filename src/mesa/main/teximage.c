/*
 * mesa 3-D graphics library
 * Version:  7.6
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


#include "glheader.h"
#include "bufferobj.h"
#include "context.h"
#include "convolve.h"
#include "enums.h"
#include "fbobject.h"
#include "framebuffer.h"
#include "hash.h"
#include "image.h"
#include "imports.h"
#include "macros.h"
#include "state.h"
#include "texcompress.h"
#include "texfetch.h"
#include "teximage.h"
#include "texstate.h"
#include "mtypes.h"


/**
 * State changes which we care about for glCopyTex[Sub]Image() calls.
 * In particular, we care about pixel transfer state and buffer state
 * (such as glReadBuffer to make sure we read from the right renderbuffer).
 */
#define NEW_COPY_TEX_STATE (_MESA_NEW_TRANSFER_STATE | \
                            _NEW_BUFFERS | \
                            _NEW_PIXEL)



/**
 * We allocate texture memory on 512-byte boundaries so we can use MMX/SSE
 * elsewhere.
 */
void *
_mesa_alloc_texmemory(GLsizei bytes)
{
   return _mesa_align_malloc(bytes, 512);
}


/**
 * Free texture memory allocated with _mesa_alloc_texmemory()
 */
void
_mesa_free_texmemory(void *m)
{
   _mesa_align_free(m);
}


/*
 * Compute floor(log_base_2(n)).
 * If n < 0 return -1.
 */
static int
logbase2( int n )
{
   GLint i = 1;
   GLint log2 = 0;

   if (n < 0)
      return -1;

   if (n == 0)
      return 0;

   while ( n > i ) {
      i *= 2;
      log2++;
   }
   if (i != n) {
      return log2 - 1;
   }
   else {
      return log2;
   }
}



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
 *
 * XXX this could be static
 */
GLint
_mesa_base_tex_format( GLcontext *ctx, GLint internalFormat )
{
   switch (internalFormat) {
      case GL_ALPHA:
      case GL_ALPHA4:
      case GL_ALPHA8:
      case GL_ALPHA12:
      case GL_ALPHA16:
         return GL_ALPHA;
      case 1:
      case GL_LUMINANCE:
      case GL_LUMINANCE4:
      case GL_LUMINANCE8:
      case GL_LUMINANCE12:
      case GL_LUMINANCE16:
         return GL_LUMINANCE;
      case 2:
      case GL_LUMINANCE_ALPHA:
      case GL_LUMINANCE4_ALPHA4:
      case GL_LUMINANCE6_ALPHA2:
      case GL_LUMINANCE8_ALPHA8:
      case GL_LUMINANCE12_ALPHA4:
      case GL_LUMINANCE12_ALPHA12:
      case GL_LUMINANCE16_ALPHA16:
         return GL_LUMINANCE_ALPHA;
      case GL_INTENSITY:
      case GL_INTENSITY4:
      case GL_INTENSITY8:
      case GL_INTENSITY12:
      case GL_INTENSITY16:
         return GL_INTENSITY;
      case 3:
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

   if (ctx->Extensions.EXT_paletted_texture) {
      switch (internalFormat) {
         case GL_COLOR_INDEX:
         case GL_COLOR_INDEX1_EXT:
         case GL_COLOR_INDEX2_EXT:
         case GL_COLOR_INDEX4_EXT:
         case GL_COLOR_INDEX8_EXT:
         case GL_COLOR_INDEX12_EXT:
         case GL_COLOR_INDEX16_EXT:
            return GL_COLOR_INDEX;
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

   if (ctx->Extensions.EXT_texture_compression_s3tc) {
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

   if (ctx->Extensions.S3_s3tc) {
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

   if (ctx->Extensions.MESA_texture_signed_rgba) {
      switch (internalFormat) {
         case GL_RGBA_SNORM:
         case GL_RGBA8_SNORM:
            return GL_RGBA;
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

#if FEATURE_EXT_texture_sRGB
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
      case GL_COMPRESSED_SLUMINANCE_EXT:
      case GL_COMPRESSED_SLUMINANCE_ALPHA_EXT:
         return GL_LUMINANCE_ALPHA;
      case GL_SLUMINANCE_EXT:
      case GL_SLUMINANCE8_EXT:
         return GL_LUMINANCE;
      default:
            ; /* fallthrough */
      }
   }

#endif /* FEATURE_EXT_texture_sRGB */

   return -1; /* error */
}


/**
 * Test if it is a supported compressed format.
 * 
 * \param internalFormat the internal format token provided by the user.
 * 
 * \ret GL_TRUE if \p internalFormat is a supported compressed format, or
 * GL_FALSE otherwise.
 *
 * Currently only GL_COMPRESSED_RGB_FXT1_3DFX and GL_COMPRESSED_RGBA_FXT1_3DFX
 * are supported.
 */
static GLboolean
is_compressed_format(GLcontext *ctx, GLenum internalFormat)
{
   GLint supported[100]; /* 100 should be plenty */
   GLuint i, n;

   n = _mesa_get_compressed_formats(ctx, supported, GL_TRUE);
   ASSERT(n < 100);
   for (i = 0; i < n; i++) {
      if ((GLint) internalFormat == supported[i]) {
         return GL_TRUE;
      }
   }
   return GL_FALSE;
}


/**
 * For cube map faces, return a face index in [0,5].
 * For other targets return 0;
 */
GLuint
_mesa_tex_target_to_face(GLenum target)
{
   if (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB &&
       target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB)
      return (GLuint) target - (GLuint) GL_TEXTURE_CUBE_MAP_POSITIVE_X;
   else
      return 0;
}



/**
 * Store a gl_texture_image pointer in a gl_texture_object structure
 * according to the target and level parameters.
 * 
 * \param tObj texture object.
 * \param target texture target.
 * \param level image level.
 * \param texImage texture image.
 * 
 * This was basically prompted by the introduction of cube maps.
 */
void
_mesa_set_tex_image(struct gl_texture_object *tObj,
                    GLenum target, GLint level,
                    struct gl_texture_image *texImage)
{
   const GLuint face = _mesa_tex_target_to_face(target);

   ASSERT(tObj);
   ASSERT(texImage);
   ASSERT(target != GL_TEXTURE_RECTANGLE_NV || level == 0);

   tObj->Image[face][level] = texImage;

   /* Set the 'back' pointer */
   texImage->TexObject = tObj;
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
_mesa_new_texture_image( GLcontext *ctx )
{
   (void) ctx;
   return CALLOC_STRUCT(gl_texture_image);
}


/**
 * Free texture image data.
 * This function is a fallback called via ctx->Driver.FreeTexImageData().
 *
 * \param texImage texture image.
 *
 * Free the texture image data if it's not marked as client data.
 */
void
_mesa_free_texture_image_data(GLcontext *ctx,
                              struct gl_texture_image *texImage)
{
   (void) ctx;

   if (texImage->Data && !texImage->IsClientData) {
      /* free the old texture data */
      _mesa_free_texmemory(texImage->Data);
   }

   texImage->Data = NULL;
}


/**
 * Free texture image.
 *
 * \param texImage texture image.
 *
 * Free the texture image structure and the associated image data.
 */
void
_mesa_delete_texture_image( GLcontext *ctx, struct gl_texture_image *texImage )
{
   /* Free texImage->Data and/or any other driver-specific texture
    * image storage.
    */
   ASSERT(ctx->Driver.FreeTexImageData);
   ctx->Driver.FreeTexImageData( ctx, texImage );

   ASSERT(texImage->Data == NULL);
   if (texImage->ImageOffsets)
      free(texImage->ImageOffsets);
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
   /* NUM_TEXTURE_TARGETS should match number of terms below */
   assert(NUM_TEXTURE_TARGETS == 7);

   return (target == GL_PROXY_TEXTURE_1D ||
           target == GL_PROXY_TEXTURE_2D ||
           target == GL_PROXY_TEXTURE_3D ||
           target == GL_PROXY_TEXTURE_CUBE_MAP_ARB ||
           target == GL_PROXY_TEXTURE_RECTANGLE_NV ||
           target == GL_PROXY_TEXTURE_1D_ARRAY_EXT ||
           target == GL_PROXY_TEXTURE_2D_ARRAY_EXT);
}


/**
 * Get the texture object that corresponds to the target of the given texture unit.
 *
 * \param ctx GL context.
 * \param texUnit texture unit.
 * \param target texture target.
 *
 * \return pointer to the texture object on success, or NULL on failure.
 * 
 * \sa gl_texture_unit.
 */
struct gl_texture_object *
_mesa_select_tex_object(GLcontext *ctx, const struct gl_texture_unit *texUnit,
                        GLenum target)
{
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
      case GL_TEXTURE_RECTANGLE_NV:
         return ctx->Extensions.NV_texture_rectangle
                ? texUnit->CurrentTex[TEXTURE_RECT_INDEX] : NULL;
      case GL_PROXY_TEXTURE_RECTANGLE_NV:
         return ctx->Extensions.NV_texture_rectangle
                ? ctx->Texture.ProxyTex[TEXTURE_RECT_INDEX] : NULL;
      case GL_TEXTURE_1D_ARRAY_EXT:
         return ctx->Extensions.MESA_texture_array
                ? texUnit->CurrentTex[TEXTURE_1D_ARRAY_INDEX] : NULL;
      case GL_PROXY_TEXTURE_1D_ARRAY_EXT:
         return ctx->Extensions.MESA_texture_array
                ? ctx->Texture.ProxyTex[TEXTURE_1D_ARRAY_INDEX] : NULL;
      case GL_TEXTURE_2D_ARRAY_EXT:
         return ctx->Extensions.MESA_texture_array
                ? texUnit->CurrentTex[TEXTURE_2D_ARRAY_INDEX] : NULL;
      case GL_PROXY_TEXTURE_2D_ARRAY_EXT:
         return ctx->Extensions.MESA_texture_array
                ? ctx->Texture.ProxyTex[TEXTURE_2D_ARRAY_INDEX] : NULL;
      default:
         _mesa_problem(NULL, "bad target in _mesa_select_tex_object()");
         return NULL;
   }
}


/**
 * Return pointer to texture object for given target on current texture unit.
 */
struct gl_texture_object *
_mesa_get_current_tex_object(GLcontext *ctx, GLenum target)
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
_mesa_select_tex_image(GLcontext *ctx, const struct gl_texture_object *texObj,
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
_mesa_get_tex_image(GLcontext *ctx, struct gl_texture_object *texObj,
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

      _mesa_set_tex_image(texObj, target, level, texImage);
   }

   return texImage;
}


/**
 * Return pointer to the specified proxy texture image.
 * Note that proxy textures are per-context, not per-texture unit.
 * \return pointer to texture image or NULL if invalid target, invalid
 *         level, or out of memory.
 */
struct gl_texture_image *
_mesa_get_proxy_tex_image(GLcontext *ctx, GLenum target, GLint level)
{
   struct gl_texture_image *texImage;
   GLuint texIndex;

   if (level < 0 )
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
_mesa_max_texture_levels(GLcontext *ctx, GLenum target)
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
   case GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB:
   case GL_TEXTURE_CUBE_MAP_ARB:
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
      return ctx->Extensions.MESA_texture_array
         ? ctx->Const.MaxTextureLevels : 0;
   default:
      return 0; /* bad target */
   }
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
   GLubyte *data = (GLubyte *) MALLOC(numPixels * components * sizeof(GLubyte));

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
 * Reset the fields of a gl_texture_image struct to zero.
 * 
 * \param img texture image structure.
 *
 * This is called when a proxy texture test fails, we set all the
 * image members (except DriverData) to zero.
 * It's also used in glTexImage[123]D as a safeguard to be sure all
 * required fields get initialized properly by the Driver.TexImage[123]D
 * functions.
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
   img->RowStride = 0;
   if (img->ImageOffsets) {
      free(img->ImageOffsets);
      img->ImageOffsets = NULL;
   }
   img->Width2 = 0;
   img->Height2 = 0;
   img->Depth2 = 0;
   img->WidthLog2 = 0;
   img->HeightLog2 = 0;
   img->DepthLog2 = 0;
   img->Data = NULL;
   img->TexFormat = MESA_FORMAT_NONE;
   img->FetchTexelc = NULL;
   img->FetchTexelf = NULL;
}


/**
 * Initialize basic fields of the gl_texture_image struct.
 *
 * \param ctx GL context.
 * \param target texture target (GL_TEXTURE_1D, GL_TEXTURE_RECTANGLE, etc).
 * \param img texture image structure to be initialized.
 * \param width image width.
 * \param height image height.
 * \param depth image depth.
 * \param border image border.
 * \param internalFormat internal format.
 *
 * Fills in the fields of \p img with the given information.
 * Note: width, height and depth include the border.
 */
void
_mesa_init_teximage_fields(GLcontext *ctx, GLenum target,
                           struct gl_texture_image *img,
                           GLsizei width, GLsizei height, GLsizei depth,
                           GLint border, GLenum internalFormat)
{
   GLint i;

   ASSERT(img);
   ASSERT(width >= 0);
   ASSERT(height >= 0);
   ASSERT(depth >= 0);

   img->_BaseFormat = _mesa_base_tex_format( ctx, internalFormat );
   ASSERT(img->_BaseFormat > 0);
   img->InternalFormat = internalFormat;
   img->Border = border;
   img->Width = width;
   img->Height = height;
   img->Depth = depth;

   img->Width2 = width - 2 * border;   /* == 1 << img->WidthLog2; */
   img->WidthLog2 = logbase2(img->Width2);

   if (height == 1) { /* 1-D texture */
      img->Height2 = 1;
      img->HeightLog2 = 0;
   }
   else {
      img->Height2 = height - 2 * border; /* == 1 << img->HeightLog2; */
      img->HeightLog2 = logbase2(img->Height2);
   }

   if (depth == 1) {  /* 2-D texture */
      img->Depth2 = 1;
      img->DepthLog2 = 0;
   }
   else {
      img->Depth2 = depth - 2 * border;   /* == 1 << img->DepthLog2; */
      img->DepthLog2 = logbase2(img->Depth2);
   }

   img->MaxLog2 = MAX2(img->WidthLog2, img->HeightLog2);

   if ((width == 1 || _mesa_is_pow_two(img->Width2)) &&
       (height == 1 || _mesa_is_pow_two(img->Height2)) &&
       (depth == 1 || _mesa_is_pow_two(img->Depth2)))
      img->_IsPowerOfTwo = GL_TRUE;
   else
      img->_IsPowerOfTwo = GL_FALSE;

   /* RowStride and ImageOffsets[] describe how to address texels in 'Data' */
   img->RowStride = width;
   /* Allocate the ImageOffsets array and initialize to typical values.
    * We allocate the array for 1D/2D textures too in order to avoid special-
    * case code in the texstore routines.
    */
   if (img->ImageOffsets)
      free(img->ImageOffsets);
   img->ImageOffsets = (GLuint *) malloc(depth * sizeof(GLuint));
   for (i = 0; i < depth; i++) {
      img->ImageOffsets[i] = i * width * height;
   }

   /* Compute Width/Height/DepthScale for mipmap lod computation */
   if (target == GL_TEXTURE_RECTANGLE_NV) {
      /* scale = 1.0 since texture coords directly map to texels */
      img->WidthScale = 1.0;
      img->HeightScale = 1.0;
      img->DepthScale = 1.0;
   }
   else {
      img->WidthScale = (GLfloat) img->Width;
      img->HeightScale = (GLfloat) img->Height;
      img->DepthScale = (GLfloat) img->Depth;
   }

   img->FetchTexelc = NULL;
   img->FetchTexelf = NULL;
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
_mesa_clear_texture_image(GLcontext *ctx, struct gl_texture_image *texImage)
{
   ctx->Driver.FreeTexImageData(ctx, texImage);
   clear_teximage_fields(texImage);
}


/**
 * This is the fallback for Driver.TestProxyTexImage().  Test the texture
 * level, width, height and depth against the ctx->Const limits for textures.
 *
 * A hardware driver might override this function if, for example, the
 * max 3D texture size is 512x512x64 (i.e. not a cube).
 *
 * Note that width, height, depth == 0 is not an error.  However, a
 * texture with zero width/height/depth will be considered "incomplete"
 * and texturing will effectively be disabled.
 *
 * \param target  one of GL_PROXY_TEXTURE_1D, GL_PROXY_TEXTURE_2D,
 *                GL_PROXY_TEXTURE_3D, GL_PROXY_TEXTURE_RECTANGLE_NV,
 *                GL_PROXY_TEXTURE_CUBE_MAP_ARB.
 * \param level  as passed to glTexImage
 * \param internalFormat  as passed to glTexImage
 * \param format  as passed to glTexImage
 * \param type  as passed to glTexImage
 * \param width  as passed to glTexImage
 * \param height  as passed to glTexImage
 * \param depth  as passed to glTexImage
 * \param border  as passed to glTexImage
 * \return GL_TRUE if the image is acceptable, GL_FALSE if not acceptable.
 */
GLboolean
_mesa_test_proxy_teximage(GLcontext *ctx, GLenum target, GLint level,
                          GLint internalFormat, GLenum format, GLenum type,
                          GLint width, GLint height, GLint depth, GLint border)
{
   GLint maxSize;

   (void) internalFormat;
   (void) format;
   (void) type;

   switch (target) {
   case GL_PROXY_TEXTURE_1D:
      maxSize = 1 << (ctx->Const.MaxTextureLevels - 1);
      if (width < 2 * border || width > 2 + maxSize ||
          (!ctx->Extensions.ARB_texture_non_power_of_two &&
           width >0 && !_mesa_is_pow_two(width - 2 * border)) ||
          level >= ctx->Const.MaxTextureLevels) {
         /* bad width or level */
         return GL_FALSE;
      }
      return GL_TRUE;
   case GL_PROXY_TEXTURE_2D:
      maxSize = 1 << (ctx->Const.MaxTextureLevels - 1);
      if (width < 2 * border || width > 2 + maxSize ||
          (!ctx->Extensions.ARB_texture_non_power_of_two &&
           width > 0 && !_mesa_is_pow_two(width - 2 * border)) ||
          height < 2 * border || height > 2 + maxSize ||
          (!ctx->Extensions.ARB_texture_non_power_of_two &&
           height > 0 && !_mesa_is_pow_two(height - 2 * border)) ||
          level >= ctx->Const.MaxTextureLevels) {
         /* bad width or height or level */
         return GL_FALSE;
      }
      return GL_TRUE;
   case GL_PROXY_TEXTURE_3D:
      maxSize = 1 << (ctx->Const.Max3DTextureLevels - 1);
      if (width < 2 * border || width > 2 + maxSize ||
          (!ctx->Extensions.ARB_texture_non_power_of_two &&
           width > 0 && !_mesa_is_pow_two(width - 2 * border)) ||
          height < 2 * border || height > 2 + maxSize ||
          (!ctx->Extensions.ARB_texture_non_power_of_two &&
           height > 0 && !_mesa_is_pow_two(height - 2 * border)) ||
          depth < 2 * border || depth > 2 + maxSize ||
          (!ctx->Extensions.ARB_texture_non_power_of_two &&
           depth > 0 && !_mesa_is_pow_two(depth - 2 * border)) ||
          level >= ctx->Const.Max3DTextureLevels) {
         /* bad width or height or depth or level */
         return GL_FALSE;
      }
      return GL_TRUE;
   case GL_PROXY_TEXTURE_RECTANGLE_NV:
      if (width < 0 || width > ctx->Const.MaxTextureRectSize ||
          height < 0 || height > ctx->Const.MaxTextureRectSize ||
          level != 0) {
         /* bad width or height or level */
         return GL_FALSE;
      }
      return GL_TRUE;
   case GL_PROXY_TEXTURE_CUBE_MAP_ARB:
      maxSize = 1 << (ctx->Const.MaxCubeTextureLevels - 1);
      if (width < 2 * border || width > 2 + maxSize ||
          (!ctx->Extensions.ARB_texture_non_power_of_two &&
           width > 0 && !_mesa_is_pow_two(width - 2 * border)) ||
          height < 2 * border || height > 2 + maxSize ||
          (!ctx->Extensions.ARB_texture_non_power_of_two &&
           height > 0 && !_mesa_is_pow_two(height - 2 * border)) ||
          level >= ctx->Const.MaxCubeTextureLevels) {
         /* bad width or height */
         return GL_FALSE;
      }
      return GL_TRUE;
   case GL_PROXY_TEXTURE_1D_ARRAY_EXT:
      maxSize = 1 << (ctx->Const.MaxTextureLevels - 1);
      if (width < 2 * border || width > 2 + maxSize ||
          (!ctx->Extensions.ARB_texture_non_power_of_two &&
           width > 0 && !_mesa_is_pow_two(width - 2 * border)) ||
          level >= ctx->Const.MaxTextureLevels) {
         /* bad width or level */
         return GL_FALSE;
      }

      if (height < 1 || height > ctx->Const.MaxArrayTextureLayers) {
         return GL_FALSE;
      }
      return GL_TRUE;
   case GL_PROXY_TEXTURE_2D_ARRAY_EXT:
      maxSize = 1 << (ctx->Const.MaxTextureLevels - 1);
      if (width < 2 * border || width > 2 + maxSize ||
          (!ctx->Extensions.ARB_texture_non_power_of_two &&
           width > 0 && !_mesa_is_pow_two(width - 2 * border)) ||
          height < 2 * border || height > 2 + maxSize ||
          (!ctx->Extensions.ARB_texture_non_power_of_two &&
           height > 0 && !_mesa_is_pow_two(height - 2 * border)) ||
          level >= ctx->Const.MaxTextureLevels) {
         /* bad width or height or level */
         return GL_FALSE;
      }
      if (depth < 1 || depth > ctx->Const.MaxArrayTextureLayers) {
         return GL_FALSE;
      }
      return GL_TRUE;
   default:
      _mesa_problem(ctx, "Invalid target in _mesa_test_proxy_teximage");
      return GL_FALSE;
   }
}


/**
 * Helper function to determine whether a target supports compressed textures
 */
static GLboolean
target_can_be_compressed(GLcontext *ctx, GLenum target)
{
   return (((target == GL_TEXTURE_2D || target == GL_PROXY_TEXTURE_2D))
           || ((ctx->Extensions.ARB_texture_cube_map &&
                (target == GL_PROXY_TEXTURE_CUBE_MAP ||
                 (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X &&
                  target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z))))
           || ((ctx->Extensions.MESA_texture_array &&
                ((target == GL_PROXY_TEXTURE_2D_ARRAY_EXT) ||
                 (target == GL_TEXTURE_2D_ARRAY_EXT)))));
}


/**
 * Test the glTexImage[123]D() parameters for errors.
 * 
 * \param ctx GL context.
 * \param target texture target given by the user.
 * \param level image level given by the user.
 * \param internalFormat internal format given by the user.
 * \param format pixel data format given by the user.
 * \param type pixel data type given by the user.
 * \param dimensions texture image dimensions (must be 1, 2 or 3).
 * \param width image width given by the user.
 * \param height image height given by the user.
 * \param depth image depth given by the user.
 * \param border image border given by the user.
 * 
 * \return GL_TRUE if an error was detected, or GL_FALSE if no errors.
 *
 * Verifies each of the parameters against the constants specified in
 * __GLcontextRec::Const and the supported extensions, and according to the
 * OpenGL specification.
 */
static GLboolean
texture_error_check( GLcontext *ctx, GLenum target,
                     GLint level, GLint internalFormat,
                     GLenum format, GLenum type,
                     GLuint dimensions,
                     GLint width, GLint height,
                     GLint depth, GLint border )
{
   const GLboolean isProxy = _mesa_is_proxy_texture(target);
   GLboolean sizeOK = GL_TRUE;
   GLboolean colorFormat, indexFormat;
   GLenum proxy_target;

   /* Basic level check (more checking in ctx->Driver.TestProxyTexImage) */
   if (level < 0 || level >= MAX_TEXTURE_LEVELS) {
      if (!isProxy) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glTexImage%dD(level=%d)", dimensions, level);
      }
      return GL_TRUE;
   }

   /* Check border */
   if (border < 0 || border > 1 ||
       ((target == GL_TEXTURE_RECTANGLE_NV ||
         target == GL_PROXY_TEXTURE_RECTANGLE_NV) && border != 0)) {
      if (!isProxy) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glTexImage%dD(border=%d)", dimensions, border);
      }
      return GL_TRUE;
   }

   if (width < 0 || height < 0 || depth < 0) {
      if (!isProxy) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glTexImage%dD(width, height or depth < 0)", dimensions);
      }
      return GL_TRUE;
   }

   /* Check target and call ctx->Driver.TestProxyTexImage() to check the
    * level, width, height and depth.
    */
   if (dimensions == 1) {
      if (target == GL_PROXY_TEXTURE_1D || target == GL_TEXTURE_1D) {
         proxy_target = GL_PROXY_TEXTURE_1D;
         height = 1;
         depth = 1;
      }
      else {
         _mesa_error( ctx, GL_INVALID_ENUM, "glTexImage1D(target)" );
         return GL_TRUE;
      }
   }
   else if (dimensions == 2) {
      depth = 1;
      if (target == GL_PROXY_TEXTURE_2D || target == GL_TEXTURE_2D) {
         proxy_target = GL_PROXY_TEXTURE_2D;
      }
      else if (target == GL_PROXY_TEXTURE_CUBE_MAP_ARB ||
               (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB &&
                target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB)) {
         if (!ctx->Extensions.ARB_texture_cube_map) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glTexImage2D(target)");
            return GL_TRUE;
         }
         proxy_target = GL_PROXY_TEXTURE_CUBE_MAP_ARB;
         sizeOK = (width == height);
      }
      else if (target == GL_PROXY_TEXTURE_RECTANGLE_NV ||
               target == GL_TEXTURE_RECTANGLE_NV) {
         if (!ctx->Extensions.NV_texture_rectangle) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glTexImage2D(target)");
            return GL_TRUE;
         }
         proxy_target = GL_PROXY_TEXTURE_RECTANGLE_NV;
      }
      else if (target == GL_PROXY_TEXTURE_1D_ARRAY_EXT ||
               target == GL_TEXTURE_1D_ARRAY_EXT) {
         proxy_target = GL_PROXY_TEXTURE_1D_ARRAY_EXT;
      }
      else {
         _mesa_error(ctx, GL_INVALID_ENUM, "glTexImage2D(target)");
         return GL_TRUE;
      }
   }
   else if (dimensions == 3) {
      if (target == GL_PROXY_TEXTURE_3D || target == GL_TEXTURE_3D) {
         proxy_target = GL_PROXY_TEXTURE_3D;
      }
      else if (target == GL_PROXY_TEXTURE_2D_ARRAY_EXT ||
               target == GL_TEXTURE_2D_ARRAY_EXT) {
         proxy_target = GL_PROXY_TEXTURE_2D_ARRAY_EXT;
      }
      else {
         _mesa_error( ctx, GL_INVALID_ENUM, "glTexImage3D(target)" );
         return GL_TRUE;
      }
   }
   else {
      _mesa_problem( ctx, "bad dims in texture_error_check" );
      return GL_TRUE;
   }

   sizeOK = sizeOK && ctx->Driver.TestProxyTexImage(ctx, proxy_target, level,
                                                    internalFormat, format,
                                                    type, width, height,
                                                    depth, border);
   if (!sizeOK) {
      if (!isProxy) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glTexImage%dD(level=%d, width=%d, height=%d, depth=%d)",
                     dimensions, level, width, height, depth);
      }
      return GL_TRUE;
   }

   /* Check internalFormat */
   if (_mesa_base_tex_format(ctx, internalFormat) < 0) {
      if (!isProxy) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glTexImage%dD(internalFormat=%s)",
                     dimensions, _mesa_lookup_enum_by_nr(internalFormat));
      }
      return GL_TRUE;
   }

   /* Check incoming image format and type */
   if (!_mesa_is_legal_format_and_type(ctx, format, type)) {
      /* Yes, generate GL_INVALID_OPERATION, not GL_INVALID_ENUM, if there
       * is a type/format mismatch.  See 1.2 spec page 94, sec 3.6.4.
       */
      if (!isProxy) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glTexImage%dD(incompatible format 0x%x, type 0x%x)",
                     dimensions, format, type);
      }
      return GL_TRUE;
   }

   /* make sure internal format and format basically agree */
   colorFormat = _mesa_is_color_format(format);
   indexFormat = _mesa_is_index_format(format);
   if ((_mesa_is_color_format(internalFormat) && !colorFormat && !indexFormat) ||
       (_mesa_is_index_format(internalFormat) && !indexFormat) ||
       (_mesa_is_depth_format(internalFormat) != _mesa_is_depth_format(format)) ||
       (_mesa_is_ycbcr_format(internalFormat) != _mesa_is_ycbcr_format(format)) ||
       (_mesa_is_depthstencil_format(internalFormat) != _mesa_is_depthstencil_format(format)) ||
       (_mesa_is_dudv_format(internalFormat) != _mesa_is_dudv_format(format))) {
      if (!isProxy)
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glTexImage%dD(incompatible internalFormat 0x%x, format 0x%x)",
                     dimensions, internalFormat, format);
      return GL_TRUE;
   }

   /* additional checks for ycbcr textures */
   if (internalFormat == GL_YCBCR_MESA) {
      ASSERT(ctx->Extensions.MESA_ycbcr_texture);
      if (type != GL_UNSIGNED_SHORT_8_8_MESA &&
          type != GL_UNSIGNED_SHORT_8_8_REV_MESA) {
         char message[100];
         sprintf(message,
                 "glTexImage%d(format/type YCBCR mismatch", dimensions);
         _mesa_error(ctx, GL_INVALID_ENUM, message);
         return GL_TRUE; /* error */
      }
      if (target != GL_TEXTURE_2D &&
          target != GL_PROXY_TEXTURE_2D &&
          target != GL_TEXTURE_RECTANGLE_NV &&
          target != GL_PROXY_TEXTURE_RECTANGLE_NV) {
         if (!isProxy)
            _mesa_error(ctx, GL_INVALID_ENUM, "glTexImage(target)");
         return GL_TRUE;
      }
      if (border != 0) {
         if (!isProxy) {
            char message[100];
            sprintf(message,
                    "glTexImage%d(format=GL_YCBCR_MESA and border=%d)",
                    dimensions, border);
            _mesa_error(ctx, GL_INVALID_VALUE, message);
         }
         return GL_TRUE;
      }
   }

   /* additional checks for depth textures */
   if (_mesa_base_tex_format(ctx, internalFormat) == GL_DEPTH_COMPONENT) {
      /* Only 1D, 2D and rectangular textures supported, not 3D or cubes */
      if (target != GL_TEXTURE_1D &&
          target != GL_PROXY_TEXTURE_1D &&
          target != GL_TEXTURE_2D &&
          target != GL_PROXY_TEXTURE_2D &&
          target != GL_TEXTURE_RECTANGLE_ARB &&
          target != GL_PROXY_TEXTURE_RECTANGLE_ARB) {
         if (!isProxy)
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glTexImage(target/internalFormat)");
         return GL_TRUE;
      }
   }

   /* additional checks for compressed textures */
   if (is_compressed_format(ctx, internalFormat)) {
      if (!target_can_be_compressed(ctx, target) && !isProxy) {
         _mesa_error(ctx, GL_INVALID_ENUM,
                     "glTexImage%d(target)", dimensions);
         return GL_TRUE;
      }
      if (border != 0) {
         if (!isProxy) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glTexImage%D(border!=0)", dimensions);
         }
         return GL_TRUE;
      }
   }

   /* if we get here, the parameters are OK */
   return GL_FALSE;
}


/**
 * Test glTexSubImage[123]D() parameters for errors.
 * 
 * \param ctx GL context.
 * \param dimensions texture image dimensions (must be 1, 2 or 3).
 * \param target texture target given by the user.
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
 * __GLcontextRec::Const and the supported extensions, and according to the
 * OpenGL specification.
 */
static GLboolean
subtexture_error_check( GLcontext *ctx, GLuint dimensions,
                        GLenum target, GLint level,
                        GLint xoffset, GLint yoffset, GLint zoffset,
                        GLint width, GLint height, GLint depth,
                        GLenum format, GLenum type )
{
   /* Check target */
   if (dimensions == 1) {
      if (target != GL_TEXTURE_1D) {
         _mesa_error( ctx, GL_INVALID_ENUM, "glTexSubImage1D(target)" );
         return GL_TRUE;
      }
   }
   else if (dimensions == 2) {
      if (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB &&
          target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB) {
         if (!ctx->Extensions.ARB_texture_cube_map) {
            _mesa_error( ctx, GL_INVALID_ENUM, "glTexSubImage2D(target)" );
            return GL_TRUE;
         }
      }
      else if (target == GL_TEXTURE_RECTANGLE_NV) {
         if (!ctx->Extensions.NV_texture_rectangle) {
            _mesa_error( ctx, GL_INVALID_ENUM, "glTexSubImage2D(target)" );
            return GL_TRUE;
         }
      }
      else if (target == GL_TEXTURE_1D_ARRAY_EXT) {
        if (!ctx->Extensions.MESA_texture_array) {
           _mesa_error( ctx, GL_INVALID_ENUM, "glTexSubImage2D(target)" );
           return GL_TRUE;
        }
      }
      else if (target != GL_TEXTURE_2D) {
         _mesa_error( ctx, GL_INVALID_ENUM, "glTexSubImage2D(target)" );
         return GL_TRUE;
      }
   }
   else if (dimensions == 3) {
      if (target == GL_TEXTURE_2D_ARRAY_EXT) {
         if (!ctx->Extensions.MESA_texture_array) {
            _mesa_error( ctx, GL_INVALID_ENUM, "glTexSubImage3D(target)" );
            return GL_TRUE;
         }
      }
      else if (target != GL_TEXTURE_3D) {
         _mesa_error( ctx, GL_INVALID_ENUM, "glTexSubImage3D(target)" );
         return GL_TRUE;
      }
   }
   else {
      _mesa_problem( ctx, "invalid dims in texture_error_check" );
      return GL_TRUE;
   }

   /* Basic level check */
   if (level < 0 || level >= MAX_TEXTURE_LEVELS) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glTexSubImage2D(level=%d)", level);
      return GL_TRUE;
   }

   if (width < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glTexSubImage%dD(width=%d)", dimensions, width);
      return GL_TRUE;
   }
   if (height < 0 && dimensions > 1) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glTexSubImage%dD(height=%d)", dimensions, height);
      return GL_TRUE;
   }
   if (depth < 0 && dimensions > 2) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glTexSubImage%dD(depth=%d)", dimensions, depth);
      return GL_TRUE;
   }

   if (!_mesa_is_legal_format_and_type(ctx, format, type)) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glTexSubImage%dD(incompatible format 0x%x, type 0x%x)",
                  dimensions, format, type);
      return GL_TRUE;
   }

   return GL_FALSE;
}


/**
 * Do second part of glTexSubImage which depends on the destination texture.
 * \return GL_TRUE if error recorded, GL_FALSE otherwise
 */
static GLboolean
subtexture_error_check2( GLcontext *ctx, GLuint dimensions,
			 GLenum target, GLint level,
			 GLint xoffset, GLint yoffset, GLint zoffset,
			 GLint width, GLint height, GLint depth,
			 GLenum format, GLenum type,
			 const struct gl_texture_image *destTex )
{
   if (!destTex) {
      /* undefined image level */
      _mesa_error(ctx, GL_INVALID_OPERATION, "glTexSubImage%dD", dimensions);
      return GL_TRUE;
   }

   if (xoffset < -((GLint)destTex->Border)) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glTexSubImage%dD(xoffset)",
                  dimensions);
      return GL_TRUE;
   }
   if (xoffset + width > (GLint) (destTex->Width + destTex->Border)) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glTexSubImage%dD(xoffset+width)",
                  dimensions);
      return GL_TRUE;
   }
   if (dimensions > 1) {
      if (yoffset < -((GLint)destTex->Border)) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glTexSubImage%dD(yoffset)",
                     dimensions);
         return GL_TRUE;
      }
      if (yoffset + height > (GLint) (destTex->Height + destTex->Border)) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glTexSubImage%dD(yoffset+height)",
                     dimensions);
         return GL_TRUE;
      }
   }
   if (dimensions > 2) {
      if (zoffset < -((GLint)destTex->Border)) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glTexSubImage3D(zoffset)");
         return GL_TRUE;
      }
      if (zoffset + depth  > (GLint) (destTex->Depth + destTex->Border)) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glTexSubImage3D(zoffset+depth)");
         return GL_TRUE;
      }
   }

   if (_mesa_is_format_compressed(destTex->TexFormat)) {
      GLuint bw, bh;

      if (!target_can_be_compressed(ctx, target)) {
         _mesa_error(ctx, GL_INVALID_ENUM,
                     "glTexSubImage%D(target=%s)", dimensions,
                     _mesa_lookup_enum_by_nr(target));
         return GL_TRUE;
      }

      /* do tests which depend on compression block size */
      _mesa_get_format_block_size(destTex->TexFormat, &bw, &bh);

      /* offset must be multiple of block size */
      if ((xoffset % bw != 0) || (yoffset % bh != 0)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glTexSubImage%D(xoffset = %d, yoffset = %d)",
                     dimensions, xoffset, yoffset);
         return GL_TRUE;
      }
      /* size must be multiple of bw by bh or equal to whole texture size */
      if ((width % bw != 0) && (GLuint) width != destTex->Width) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glTexSubImage%D(width = %d)", dimensions, width);
         return GL_TRUE;
      }         
      if ((height % bh != 0) && (GLuint) height != destTex->Height) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glTexSubImage%D(height = %d)", dimensions, height);
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
 * __GLcontextRec::Const and the supported extensions, and according to the
 * OpenGL specification.
 */
static GLboolean
copytexture_error_check( GLcontext *ctx, GLuint dimensions,
                         GLenum target, GLint level, GLint internalFormat,
                         GLint width, GLint height, GLint border )
{
   GLenum type;
   GLboolean sizeOK;
   GLint format;

   /* Basic level check (more checking in ctx->Driver.TestProxyTexImage) */
   if (level < 0 || level >= MAX_TEXTURE_LEVELS) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glCopyTexImage%dD(level=%d)", dimensions, level);
      return GL_TRUE;
   }

   /* Check that the source buffer is complete */
   if (ctx->ReadBuffer->Name) {
      _mesa_test_framebuffer_completeness(ctx, ctx->ReadBuffer);
      if (ctx->ReadBuffer->_Status != GL_FRAMEBUFFER_COMPLETE_EXT) {
         _mesa_error(ctx, GL_INVALID_FRAMEBUFFER_OPERATION_EXT,
                     "glCopyTexImage%dD(invalid readbuffer)", dimensions);
         return GL_TRUE;
      }
   }

   /* Check border */
   if (border < 0 || border > 1 ||
       ((target == GL_TEXTURE_RECTANGLE_NV ||
         target == GL_PROXY_TEXTURE_RECTANGLE_NV) && border != 0)) {
      return GL_TRUE;
   }

   format = _mesa_base_tex_format(ctx, internalFormat);
   if (format < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glCopyTexImage%dD(internalFormat)", dimensions);
      return GL_TRUE;
   }

   if (!_mesa_source_buffer_exists(ctx, format)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glCopyTexImage%dD(missing readbuffer)", dimensions);
      return GL_TRUE;
   }

   /* NOTE: the format and type aren't really significant for
    * TestProxyTexImage().  Only the internalformat really matters.
    */
   type = GL_FLOAT;

   /* Check target and call ctx->Driver.TestProxyTexImage() to check the
    * level, width, height and depth.
    */
   if (dimensions == 1) {
      if (target == GL_TEXTURE_1D) {
         sizeOK = ctx->Driver.TestProxyTexImage(ctx, GL_PROXY_TEXTURE_1D,
                                                level, internalFormat,
                                                format, type,
                                                width, 1, 1, border);
      }
      else {
         _mesa_error( ctx, GL_INVALID_ENUM, "glCopyTexImage1D(target)" );
         return GL_TRUE;
      }
   }
   else if (dimensions == 2) {
      if (target == GL_TEXTURE_2D) {
         sizeOK = ctx->Driver.TestProxyTexImage(ctx, GL_PROXY_TEXTURE_2D,
                                                level, internalFormat,
                                                format, type,
                                                width, height, 1, border);
      }
      else if (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB &&
               target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB) {
         if (!ctx->Extensions.ARB_texture_cube_map) {
            _mesa_error( ctx, GL_INVALID_ENUM, "glCopyTexImage2D(target)" );
            return GL_TRUE;
         }
         sizeOK = (width == height) &&
            ctx->Driver.TestProxyTexImage(ctx, GL_PROXY_TEXTURE_CUBE_MAP_ARB,
                                          level, internalFormat, format, type,
                                          width, height, 1, border);
      }
      else if (target == GL_TEXTURE_RECTANGLE_NV) {
         if (!ctx->Extensions.NV_texture_rectangle) {
            _mesa_error( ctx, GL_INVALID_ENUM, "glCopyTexImage2D(target)" );
            return GL_TRUE;
         }
         sizeOK = ctx->Driver.TestProxyTexImage(ctx,
                                                GL_PROXY_TEXTURE_RECTANGLE_NV,
                                                level, internalFormat,
                                                format, type,
                                                width, height, 1, border);
      }
      else if (target == GL_TEXTURE_1D_ARRAY_EXT) {
         if (!ctx->Extensions.MESA_texture_array) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glCopyTexImage2D(target)");
            return GL_TRUE;
         }
         sizeOK = ctx->Driver.TestProxyTexImage(ctx,
                                                GL_PROXY_TEXTURE_1D_ARRAY_EXT,
                                                level, internalFormat,
                                                format, type,
                                                width, height, 1, border);
      }
      else {
         _mesa_error( ctx, GL_INVALID_ENUM, "glCopyTexImage2D(target)" );
         return GL_TRUE;
      }
   }
   else {
      _mesa_problem(ctx, "invalid dimensions in copytexture_error_check");
      return GL_TRUE;
   }

   if (!sizeOK) {
      if (dimensions == 1) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glCopyTexImage1D(width=%d)", width);
      }
      else {
         ASSERT(dimensions == 2);
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glCopyTexImage2D(width=%d, height=%d)", width, height);
      }
      return GL_TRUE;
   }

   if (is_compressed_format(ctx, internalFormat)) {
      if (!target_can_be_compressed(ctx, target)) {
         _mesa_error(ctx, GL_INVALID_ENUM,
                     "glCopyTexImage%d(target)", dimensions);
         return GL_TRUE;
      }
      if (border != 0) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glCopyTexImage%D(border!=0)", dimensions);
         return GL_TRUE;
      }
   }
   else if (_mesa_is_depth_format(internalFormat)) {
      /* make sure we have depth/stencil buffers */
      if (!ctx->ReadBuffer->_DepthBuffer) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glCopyTexImage%D(no depth)", dimensions);
         return GL_TRUE;
      }
   }
   else if (_mesa_is_depthstencil_format(internalFormat)) {
      /* make sure we have depth/stencil buffers */
      if (!ctx->ReadBuffer->_DepthBuffer || !ctx->ReadBuffer->_StencilBuffer) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glCopyTexImage%D(no depth/stencil buffer)", dimensions);
         return GL_TRUE;
      }
   }

   /* if we get here, the parameters are OK */
   return GL_FALSE;
}


/**
 * Test glCopyTexSubImage[12]D() parameters for errors.
 * Note that this is the first part of error checking.
 * See also copytexsubimage_error_check2() below for the second part.
 * 
 * \param ctx GL context.
 * \param dimensions texture image dimensions (must be 1, 2 or 3).
 * \param target texture target given by the user.
 * \param level image level given by the user.
 * 
 * \return GL_TRUE if an error was detected, or GL_FALSE if no errors.
 */
static GLboolean
copytexsubimage_error_check1( GLcontext *ctx, GLuint dimensions,
                              GLenum target, GLint level)
{
   /* Check that the source buffer is complete */
   if (ctx->ReadBuffer->Name) {
      _mesa_test_framebuffer_completeness(ctx, ctx->ReadBuffer);
      if (ctx->ReadBuffer->_Status != GL_FRAMEBUFFER_COMPLETE_EXT) {
         _mesa_error(ctx, GL_INVALID_FRAMEBUFFER_OPERATION_EXT,
                     "glCopyTexImage%dD(invalid readbuffer)", dimensions);
         return GL_TRUE;
      }
   }

   /* Check target */
   if (dimensions == 1) {
      if (target != GL_TEXTURE_1D) {
         _mesa_error( ctx, GL_INVALID_ENUM, "glCopyTexSubImage1D(target)" );
         return GL_TRUE;
      }
   }
   else if (dimensions == 2) {
      if (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB &&
          target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB) {
         if (!ctx->Extensions.ARB_texture_cube_map) {
            _mesa_error( ctx, GL_INVALID_ENUM, "glCopyTexSubImage2D(target)" );
            return GL_TRUE;
         }
      }
      else if (target == GL_TEXTURE_RECTANGLE_NV) {
         if (!ctx->Extensions.NV_texture_rectangle) {
            _mesa_error( ctx, GL_INVALID_ENUM, "glCopyTexSubImage2D(target)" );
            return GL_TRUE;
         }
      }
      else if (target == GL_TEXTURE_1D_ARRAY_EXT) {
         if (!ctx->Extensions.MESA_texture_array) {
            _mesa_error( ctx, GL_INVALID_ENUM, "glCopyTexSubImage2D(target)" );
            return GL_TRUE;
         }
      }
      else if (target != GL_TEXTURE_2D) {
         _mesa_error( ctx, GL_INVALID_ENUM, "glCopyTexSubImage2D(target)" );
         return GL_TRUE;
      }
   }
   else if (dimensions == 3) {
      if (((target != GL_TEXTURE_2D_ARRAY_EXT) ||
	   (!ctx->Extensions.MESA_texture_array))
	  && (target != GL_TEXTURE_3D)) {
	 _mesa_error( ctx, GL_INVALID_ENUM, "glCopyTexSubImage3D(target)" );
	 return GL_TRUE;
      }
   }

   /* Check level */
   if (level < 0 || level >= MAX_TEXTURE_LEVELS) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glCopyTexSubImage%dD(level=%d)", dimensions, level);
      return GL_TRUE;
   }

   return GL_FALSE;
}


/**
 * Second part of error checking for glCopyTexSubImage[12]D().
 * \param xoffset sub-image x offset given by the user.
 * \param yoffset sub-image y offset given by the user.
 * \param zoffset sub-image z offset given by the user.
 * \param width image width given by the user.
 * \param height image height given by the user.
 */
static GLboolean
copytexsubimage_error_check2( GLcontext *ctx, GLuint dimensions,
			      GLenum target, GLint level,
			      GLint xoffset, GLint yoffset, GLint zoffset,
			      GLsizei width, GLsizei height,
			      const struct gl_texture_image *teximage )
{
   /* check that dest tex image exists */
   if (!teximage) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glCopyTexSubImage%dD(undefined texture level: %d)",
                  dimensions, level);
      return GL_TRUE;
   }

   /* Check size */
   if (width < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glCopyTexSubImage%dD(width=%d)", dimensions, width);
      return GL_TRUE;
   }
   if (dimensions > 1 && height < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glCopyTexSubImage%dD(height=%d)", dimensions, height);
      return GL_TRUE;
   }

   /* check x/y offsets */
   if (xoffset < -((GLint)teximage->Border)) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glCopyTexSubImage%dD(xoffset=%d)", dimensions, xoffset);
      return GL_TRUE;
   }
   if (xoffset + width > (GLint) (teximage->Width + teximage->Border)) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glCopyTexSubImage%dD(xoffset+width)", dimensions);
      return GL_TRUE;
   }
   if (dimensions > 1) {
      if (yoffset < -((GLint)teximage->Border)) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glCopyTexSubImage%dD(yoffset=%d)", dimensions, yoffset);
         return GL_TRUE;
      }
      /* NOTE: we're adding the border here, not subtracting! */
      if (yoffset + height > (GLint) (teximage->Height + teximage->Border)) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glCopyTexSubImage%dD(yoffset+height)", dimensions);
         return GL_TRUE;
      }
   }

   /* check z offset */
   if (dimensions > 2) {
      if (zoffset < -((GLint)teximage->Border)) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glCopyTexSubImage%dD(zoffset)", dimensions);
         return GL_TRUE;
      }
      if (zoffset > (GLint) (teximage->Depth + teximage->Border)) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glCopyTexSubImage%dD(zoffset+depth)", dimensions);
         return GL_TRUE;
      }
   }

   if (_mesa_is_format_compressed(teximage->TexFormat)) {
      if (!target_can_be_compressed(ctx, target)) {
         _mesa_error(ctx, GL_INVALID_ENUM,
                     "glCopyTexSubImage%d(target)", dimensions);
         return GL_TRUE;
      }
      /* offset must be multiple of 4 */
      if ((xoffset & 3) || (yoffset & 3)) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glCopyTexSubImage%D(xoffset or yoffset)", dimensions);
         return GL_TRUE;
      }
      /* size must be multiple of 4 */
      if ((width & 3) != 0 && (GLuint) width != teximage->Width) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glCopyTexSubImage%D(width)", dimensions);
         return GL_TRUE;
      }         
      if ((height & 3) != 0 && (GLuint) height != teximage->Height) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glCopyTexSubImage%D(height)", dimensions);
         return GL_TRUE;
      }         
   }

   if (teximage->InternalFormat == GL_YCBCR_MESA) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glCopyTexSubImage2D");
      return GL_TRUE;
   }

   if (!_mesa_source_buffer_exists(ctx, teximage->_BaseFormat)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glCopyTexSubImage%dD(missing readbuffer, format=0x%x)",
                  dimensions, teximage->_BaseFormat);
      return GL_TRUE;
   }

   if (teximage->_BaseFormat == GL_DEPTH_COMPONENT) {
      if (!ctx->ReadBuffer->_DepthBuffer) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glCopyTexSubImage%D(no depth buffer)",
                     dimensions);
         return GL_TRUE;
      }
   }
   else if (teximage->_BaseFormat == GL_DEPTH_STENCIL_EXT) {
      if (!ctx->ReadBuffer->_DepthBuffer || !ctx->ReadBuffer->_StencilBuffer) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glCopyTexSubImage%D(no depth/stencil buffer)",
                     dimensions);
         return GL_TRUE;
      }
   }

   /* if we get here, the parameters are OK */
   return GL_FALSE;
}


/** Callback info for walking over FBO hash table */
struct cb_info
{
   GLcontext *ctx;
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
   GLcontext *ctx = info->ctx;
   const struct gl_texture_object *texObj = info->texObj;
   const GLuint level = info->level, face = info->face;

   /* If this is a user-created FBO */
   if (fb->Name) {
      GLuint i;
      /* check if any of the FBO's attachments point to 'texObj' */
      for (i = 0; i < BUFFER_COUNT; i++) {
         struct gl_renderbuffer_attachment *att = fb->Attachment + i;
         if (att->Type == GL_TEXTURE &&
             att->Texture == texObj &&
             att->TextureLevel == level &&
             att->CubeMapFace == face) {
            ASSERT(att->Texture->Image[att->CubeMapFace][att->TextureLevel]);
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
static void
update_fbo_texture(GLcontext *ctx, struct gl_texture_object *texObj,
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
static INLINE void
check_gen_mipmap(GLcontext *ctx, GLenum target,
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


/*
 * Called from the API.  Note that width includes the border.
 */
void GLAPIENTRY
_mesa_TexImage1D( GLenum target, GLint level, GLint internalFormat,
                  GLsizei width, GLint border, GLenum format,
                  GLenum type, const GLvoid *pixels )
{
   GLsizei postConvWidth = width;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glTexImage1D %s %d %s %d %d %s %s %p\n",
                  _mesa_lookup_enum_by_nr(target), level,
                  _mesa_lookup_enum_by_nr(internalFormat), width, border,
                  _mesa_lookup_enum_by_nr(format),
                  _mesa_lookup_enum_by_nr(type), pixels);

   internalFormat = override_internal_format(internalFormat, width, 1);

#if FEATURE_convolve
   if (_mesa_is_color_format(internalFormat)) {
      _mesa_adjust_image_for_convolution(ctx, 1, &postConvWidth, NULL);
   }
#endif

   if (target == GL_TEXTURE_1D) {
      /* non-proxy target */
      struct gl_texture_object *texObj;
      struct gl_texture_image *texImage;
      const GLuint face = _mesa_tex_target_to_face(target);

      if (texture_error_check(ctx, target, level, internalFormat,
                              format, type, 1, postConvWidth, 1, 1, border)) {
         return;   /* error was recorded */
      }

      if (ctx->NewState & _MESA_NEW_TRANSFER_STATE)
	 _mesa_update_state(ctx);

      texObj = _mesa_get_current_tex_object(ctx, target);
      _mesa_lock_texture(ctx, texObj);
      {
	 texImage = _mesa_get_tex_image(ctx, texObj, target, level);
	 if (!texImage) {
	    _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage1D");
	 }
         else {
            if (texImage->Data) {
               ctx->Driver.FreeTexImageData( ctx, texImage );
            }

            ASSERT(texImage->Data == NULL);

            clear_teximage_fields(texImage); /* not really needed, but helpful */
            _mesa_init_teximage_fields(ctx, target, texImage,
                                       postConvWidth, 1, 1,
                                       border, internalFormat);

            /* Choose actual texture format */
            texImage->TexFormat =
               ctx->Driver.ChooseTextureFormat(ctx, internalFormat,
                                               format, type);
            ASSERT(texImage->TexFormat != MESA_FORMAT_NONE);

            /* Give the texture to the driver.  <pixels> may be null. */
            ASSERT(ctx->Driver.TexImage1D);
            ctx->Driver.TexImage1D(ctx, target, level, internalFormat,
                                   width, border, format, type, pixels,
                                   &ctx->Unpack, texObj, texImage);

            ASSERT(texImage->TexFormat);

            _mesa_set_fetch_functions(texImage, 1);

            check_gen_mipmap(ctx, target, texObj, level);

            update_fbo_texture(ctx, texObj, face, level);

            /* state update */
            texObj->_Complete = GL_FALSE;
            ctx->NewState |= _NEW_TEXTURE;
         }
      }
      _mesa_unlock_texture(ctx, texObj);
   }
   else if (target == GL_PROXY_TEXTURE_1D) {
      /* Proxy texture: check for errors and update proxy state */
      struct gl_texture_image *texImage;
      texImage = _mesa_get_proxy_tex_image(ctx, target, level);
      if (texture_error_check(ctx, target, level, internalFormat,
                              format, type, 1, postConvWidth, 1, 1, border)) {
         /* when error, clear all proxy texture image parameters */
         if (texImage)
            clear_teximage_fields(texImage);
      }
      else {
         /* no error, set the tex image parameters */
         ASSERT(texImage);
         _mesa_init_teximage_fields(ctx, target, texImage,
                                    postConvWidth, 1, 1,
                                    border, internalFormat);
         texImage->TexFormat =
            ctx->Driver.ChooseTextureFormat(ctx, internalFormat, format, type);
      }
   }
   else {
      _mesa_error( ctx, GL_INVALID_ENUM, "glTexImage1D(target)" );
      return;
   }
}


void GLAPIENTRY
_mesa_TexImage2D( GLenum target, GLint level, GLint internalFormat,
                  GLsizei width, GLsizei height, GLint border,
                  GLenum format, GLenum type,
                  const GLvoid *pixels )
{
   GLsizei postConvWidth = width, postConvHeight = height;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glTexImage2D %s %d %s %d %d %d %s %s %p\n",
                  _mesa_lookup_enum_by_nr(target), level,
                  _mesa_lookup_enum_by_nr(internalFormat), width, height,
                  border, _mesa_lookup_enum_by_nr(format),
                  _mesa_lookup_enum_by_nr(type), pixels);

   internalFormat = override_internal_format(internalFormat, width, height);

#if FEATURE_convolve
   if (_mesa_is_color_format(internalFormat)) {
      _mesa_adjust_image_for_convolution(ctx, 2, &postConvWidth,
					 &postConvHeight);
   }
#endif

   if (target == GL_TEXTURE_2D ||
       (ctx->Extensions.ARB_texture_cube_map &&
        target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB &&
        target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB) ||
       (ctx->Extensions.NV_texture_rectangle &&
        target == GL_TEXTURE_RECTANGLE_NV) ||
       (ctx->Extensions.MESA_texture_array &&
        target == GL_TEXTURE_1D_ARRAY_EXT)) {
      /* non-proxy target */
      struct gl_texture_object *texObj;
      struct gl_texture_image *texImage;
      const GLuint face = _mesa_tex_target_to_face(target);

      if (texture_error_check(ctx, target, level, internalFormat,
                              format, type, 2, postConvWidth, postConvHeight,
                              1, border)) {
         return;   /* error was recorded */
      }

      if (ctx->NewState & _MESA_NEW_TRANSFER_STATE)
	 _mesa_update_state(ctx);

      texObj = _mesa_get_current_tex_object(ctx, target);
      _mesa_lock_texture(ctx, texObj);
      {
	 texImage = _mesa_get_tex_image(ctx, texObj, target, level);
	 if (!texImage) {
	    _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage2D");
	 }
         else {
            if (texImage->Data) {
               ctx->Driver.FreeTexImageData( ctx, texImage );
            }

            ASSERT(texImage->Data == NULL);
            clear_teximage_fields(texImage); /* not really needed, but helpful */
            _mesa_init_teximage_fields(ctx, target, texImage,
                                       postConvWidth, postConvHeight, 1,
                                       border, internalFormat);

            /* Choose actual texture format */
            texImage->TexFormat =
               ctx->Driver.ChooseTextureFormat(ctx, internalFormat,
                                               format, type);
            ASSERT(texImage->TexFormat != MESA_FORMAT_NONE);

            /* Give the texture to the driver.  <pixels> may be null. */
            ASSERT(ctx->Driver.TexImage2D);
            ctx->Driver.TexImage2D(ctx, target, level, internalFormat,
                                   width, height, border, format, type,
                                   pixels, &ctx->Unpack, texObj, texImage);

            ASSERT(texImage->TexFormat);

            _mesa_set_fetch_functions(texImage, 2);

            check_gen_mipmap(ctx, target, texObj, level);

            update_fbo_texture(ctx, texObj, face, level);

            /* state update */
            texObj->_Complete = GL_FALSE;
            ctx->NewState |= _NEW_TEXTURE;
         }
      }
      _mesa_unlock_texture(ctx, texObj);
   }
   else if (target == GL_PROXY_TEXTURE_2D ||
            (target == GL_PROXY_TEXTURE_CUBE_MAP_ARB &&
             ctx->Extensions.ARB_texture_cube_map) ||
            (target == GL_PROXY_TEXTURE_RECTANGLE_NV &&
             ctx->Extensions.NV_texture_rectangle) ||
            (ctx->Extensions.MESA_texture_array &&
             target == GL_PROXY_TEXTURE_1D_ARRAY_EXT)) {
      /* Proxy texture: check for errors and update proxy state */
      struct gl_texture_image *texImage;
      texImage = _mesa_get_proxy_tex_image(ctx, target, level);
      if (texture_error_check(ctx, target, level, internalFormat,
                              format, type, 2, postConvWidth, postConvHeight,
                              1, border)) {
         /* when error, clear all proxy texture image parameters */
         if (texImage)
            clear_teximage_fields(texImage);
      }
      else {
         /* no error, set the tex image parameters */
         _mesa_init_teximage_fields(ctx, target, texImage,
                                    postConvWidth, postConvHeight, 1,
                                    border, internalFormat);
         texImage->TexFormat =
            ctx->Driver.ChooseTextureFormat(ctx, internalFormat, format, type);
      }
   }
   else {
      _mesa_error( ctx, GL_INVALID_ENUM, "glTexImage2D(target)" );
      return;
   }
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
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glTexImage3D %s %d %s %d %d %d %d %s %s %p\n",
                  _mesa_lookup_enum_by_nr(target), level,
                  _mesa_lookup_enum_by_nr(internalFormat), width, height,
                  depth, border, _mesa_lookup_enum_by_nr(format),
                  _mesa_lookup_enum_by_nr(type), pixels);

   internalFormat = override_internal_format(internalFormat, width, height);

   if (target == GL_TEXTURE_3D ||
       (ctx->Extensions.MESA_texture_array &&
        target == GL_TEXTURE_2D_ARRAY_EXT)) {
      /* non-proxy target */
      struct gl_texture_object *texObj;
      struct gl_texture_image *texImage;
      const GLuint face = _mesa_tex_target_to_face(target);

      if (texture_error_check(ctx, target, level, (GLint) internalFormat,
                              format, type, 3, width, height, depth, border)) {
         return;   /* error was recorded */
      }

      if (ctx->NewState & _MESA_NEW_TRANSFER_STATE)
	 _mesa_update_state(ctx);

      texObj = _mesa_get_current_tex_object(ctx, target);
      _mesa_lock_texture(ctx, texObj);
      {
	 texImage = _mesa_get_tex_image(ctx, texObj, target, level);
	 if (!texImage) {
	    _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage3D");
	 }
         else {
            if (texImage->Data) {
               ctx->Driver.FreeTexImageData( ctx, texImage );
            }

            ASSERT(texImage->Data == NULL);
            clear_teximage_fields(texImage); /* not really needed, but helpful */
            _mesa_init_teximage_fields(ctx, target, texImage,
                                       width, height, depth,
                                       border, internalFormat);

            /* Choose actual texture format */
            texImage->TexFormat =
               ctx->Driver.ChooseTextureFormat(ctx, internalFormat,
                                               format, type);
            ASSERT(texImage->TexFormat != MESA_FORMAT_NONE);

            /* Give the texture to the driver.  <pixels> may be null. */
            ASSERT(ctx->Driver.TexImage3D);
            ctx->Driver.TexImage3D(ctx, target, level, internalFormat,
                                   width, height, depth, border, format, type,
                                   pixels, &ctx->Unpack, texObj, texImage);

            ASSERT(texImage->TexFormat);

            _mesa_set_fetch_functions(texImage, 3);

            check_gen_mipmap(ctx, target, texObj, level);

            update_fbo_texture(ctx, texObj, face, level);

            /* state update */
            texObj->_Complete = GL_FALSE;
            ctx->NewState |= _NEW_TEXTURE;
         }
      }
      _mesa_unlock_texture(ctx, texObj);
   }
   else if (target == GL_PROXY_TEXTURE_3D ||
       (ctx->Extensions.MESA_texture_array &&
        target == GL_PROXY_TEXTURE_2D_ARRAY_EXT)) {
      /* Proxy texture: check for errors and update proxy state */
      struct gl_texture_image *texImage;
      texImage = _mesa_get_proxy_tex_image(ctx, target, level);
      if (texture_error_check(ctx, target, level, internalFormat,
                              format, type, 3, width, height, depth, border)) {
         /* when error, clear all proxy texture image parameters */
         if (texImage)
            clear_teximage_fields(texImage);
      }
      else {
         /* no error, set the tex image parameters */
         _mesa_init_teximage_fields(ctx, target, texImage, width, height,
                                    depth, border, internalFormat);
         texImage->TexFormat =
            ctx->Driver.ChooseTextureFormat(ctx, internalFormat, format, type);
      }
   }
   else {
      _mesa_error( ctx, GL_INVALID_ENUM, "glTexImage3D(target)" );
      return;
   }
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


#if FEATURE_OES_EGL_image
void GLAPIENTRY
_mesa_EGLImageTargetTexture2DOES (GLenum target, GLeglImageOES image)
{
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (target != GL_TEXTURE_2D) {
      _mesa_error(ctx, GL_INVALID_ENUM,
		  "glEGLImageTargetTexture2D(target=%d)", target);
      return;
   }

   if (ctx->NewState & _MESA_NEW_TRANSFER_STATE)
      _mesa_update_state(ctx);

   texObj = _mesa_get_current_tex_object(ctx, target);
   _mesa_lock_texture(ctx, texObj);

   texImage = _mesa_get_tex_image(ctx, texObj, target, 0);
   if (!texImage) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glEGLImageTargetTexture2D");
   } else {
      if (texImage->Data)
	 ctx->Driver.FreeTexImageData( ctx, texImage );

      ASSERT(texImage->Data == NULL);
      ctx->Driver.EGLImageTargetTexture2D(ctx, target,
					  texObj, texImage, image);

      /* state update */
      texObj->_Complete = GL_FALSE;
      ctx->NewState |= _NEW_TEXTURE;
   }
   _mesa_unlock_texture(ctx, texObj);

}
#endif


void GLAPIENTRY
_mesa_TexSubImage1D( GLenum target, GLint level,
                     GLint xoffset, GLsizei width,
                     GLenum format, GLenum type,
                     const GLvoid *pixels )
{
   GLsizei postConvWidth = width;
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glTexSubImage1D %s %d %d %d %s %s %p\n",
                  _mesa_lookup_enum_by_nr(target), level,
                  xoffset, width, _mesa_lookup_enum_by_nr(format),
                  _mesa_lookup_enum_by_nr(type), pixels);

   if (ctx->NewState & _MESA_NEW_TRANSFER_STATE)
      _mesa_update_state(ctx);

#if FEATURE_convolve
   /* XXX should test internal format */
   if (_mesa_is_color_format(format)) {
      _mesa_adjust_image_for_convolution(ctx, 1, &postConvWidth, NULL);
   }
#endif

   if (subtexture_error_check(ctx, 1, target, level, xoffset, 0, 0,
			       postConvWidth, 1, 1, format, type)) {
      return;   /* error was detected */
   }


   texObj = _mesa_get_current_tex_object(ctx, target);
   assert(texObj);

   _mesa_lock_texture(ctx, texObj);
   {
      texImage = _mesa_select_tex_image(ctx, texObj, target, level);

      if (subtexture_error_check2(ctx, 1, target, level, xoffset, 0, 0,
				  postConvWidth, 1, 1,
                                  format, type, texImage)) {
         /* error was recorded */
      }
      else if (width > 0) {
         /* If we have a border, xoffset=-1 is legal.  Bias by border width */
         xoffset += texImage->Border;

         ASSERT(ctx->Driver.TexSubImage1D);
         ctx->Driver.TexSubImage1D(ctx, target, level, xoffset, width,
                                   format, type, pixels, &ctx->Unpack,
                                   texObj, texImage);

         check_gen_mipmap(ctx, target, texObj, level);

         ctx->NewState |= _NEW_TEXTURE;
      }
   }
   _mesa_unlock_texture(ctx, texObj);
}


void GLAPIENTRY
_mesa_TexSubImage2D( GLenum target, GLint level,
                     GLint xoffset, GLint yoffset,
                     GLsizei width, GLsizei height,
                     GLenum format, GLenum type,
                     const GLvoid *pixels )
{
   GLsizei postConvWidth = width, postConvHeight = height;
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glTexSubImage2D %s %d %d %d %d %d %s %s %p\n",
                  _mesa_lookup_enum_by_nr(target), level,
                  xoffset, yoffset, width, height,
                  _mesa_lookup_enum_by_nr(format),
                  _mesa_lookup_enum_by_nr(type), pixels);

   if (ctx->NewState & _MESA_NEW_TRANSFER_STATE)
      _mesa_update_state(ctx);

#if FEATURE_convolve
   /* XXX should test internal format */
   if (_mesa_is_color_format(format)) {
      _mesa_adjust_image_for_convolution(ctx, 2, &postConvWidth,
                                         &postConvHeight);
   }
#endif

   if (subtexture_error_check(ctx, 2, target, level, xoffset, yoffset, 0,
			      postConvWidth, postConvHeight, 1, format, type)) {
      return;   /* error was detected */
   }

   texObj = _mesa_get_current_tex_object(ctx, target);

   _mesa_lock_texture(ctx, texObj);
   {
      texImage = _mesa_select_tex_image(ctx, texObj, target, level);

      if (subtexture_error_check2(ctx, 2, target, level, xoffset, yoffset, 0,
				  postConvWidth, postConvHeight, 1,
                                  format, type, texImage)) {
	 /* error was recorded */
      }
      else if (width > 0 && height >= 0) {
         /* If we have a border, xoffset=-1 is legal.  Bias by border width */
         xoffset += texImage->Border;
         yoffset += texImage->Border;

         ASSERT(ctx->Driver.TexSubImage2D);
         ctx->Driver.TexSubImage2D(ctx, target, level, xoffset, yoffset,
                                   width, height, format, type, pixels,
                                   &ctx->Unpack, texObj, texImage);

         check_gen_mipmap(ctx, target, texObj, level);

         ctx->NewState |= _NEW_TEXTURE;
      }
   }
   _mesa_unlock_texture(ctx, texObj);
}



void GLAPIENTRY
_mesa_TexSubImage3D( GLenum target, GLint level,
                     GLint xoffset, GLint yoffset, GLint zoffset,
                     GLsizei width, GLsizei height, GLsizei depth,
                     GLenum format, GLenum type,
                     const GLvoid *pixels )
{
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glTexSubImage3D %s %d %d %d %d %d %d %d %s %s %p\n",
                  _mesa_lookup_enum_by_nr(target), level,
                  xoffset, yoffset, zoffset, width, height, depth,
                  _mesa_lookup_enum_by_nr(format),
                  _mesa_lookup_enum_by_nr(type), pixels);

   if (ctx->NewState & _MESA_NEW_TRANSFER_STATE)
      _mesa_update_state(ctx);

   if (subtexture_error_check(ctx, 3, target, level, xoffset, yoffset, zoffset,
                              width, height, depth, format, type)) {
      return;   /* error was detected */
   }

   texObj = _mesa_get_current_tex_object(ctx, target);

   _mesa_lock_texture(ctx, texObj);
   {
      texImage = _mesa_select_tex_image(ctx, texObj, target, level);

      if (subtexture_error_check2(ctx, 3, target, level,
                                  xoffset, yoffset, zoffset,
				  width, height, depth,
                                  format, type, texImage)) {
         /* error was recorded */
      }
      else if (width > 0 && height > 0 && height > 0) {
         /* If we have a border, xoffset=-1 is legal.  Bias by border width */
         xoffset += texImage->Border;
         yoffset += texImage->Border;
         zoffset += texImage->Border;

         ASSERT(ctx->Driver.TexSubImage3D);
         ctx->Driver.TexSubImage3D(ctx, target, level,
                                   xoffset, yoffset, zoffset,
                                   width, height, depth,
                                   format, type, pixels,
                                   &ctx->Unpack, texObj, texImage );

         check_gen_mipmap(ctx, target, texObj, level);

         ctx->NewState |= _NEW_TEXTURE;
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
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   GLsizei postConvWidth = width;
   const GLuint face = _mesa_tex_target_to_face(target);
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glCopyTexImage1D %s %d %s %d %d %d %d\n",
                  _mesa_lookup_enum_by_nr(target), level,
                  _mesa_lookup_enum_by_nr(internalFormat),
                  x, y, width, border);

   if (ctx->NewState & NEW_COPY_TEX_STATE)
      _mesa_update_state(ctx);

#if FEATURE_convolve
   if (_mesa_is_color_format(internalFormat)) {
      _mesa_adjust_image_for_convolution(ctx, 1, &postConvWidth, NULL);
   }
#endif

   if (copytexture_error_check(ctx, 1, target, level, internalFormat,
                               postConvWidth, 1, border))
      return;

   texObj = _mesa_get_current_tex_object(ctx, target);

   _mesa_lock_texture(ctx, texObj);
   {
      texImage = _mesa_get_tex_image(ctx, texObj, target, level);
      if (!texImage) {
	 _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyTexImage1D");
      }
      else {
         if (texImage->Data) {
            ctx->Driver.FreeTexImageData( ctx, texImage );
         }

         ASSERT(texImage->Data == NULL);

         clear_teximage_fields(texImage); /* not really needed, but helpful */
         _mesa_init_teximage_fields(ctx, target, texImage, postConvWidth, 1, 1,
                                    border, internalFormat);

         /* Choose actual texture format */
         texImage->TexFormat =
            ctx->Driver.ChooseTextureFormat(ctx, internalFormat,
                                            GL_NONE, GL_NONE);
         ASSERT(texImage->TexFormat != MESA_FORMAT_NONE);

         ASSERT(ctx->Driver.CopyTexImage1D);
         ctx->Driver.CopyTexImage1D(ctx, target, level, internalFormat,
                                    x, y, width, border);

         ASSERT(texImage->TexFormat);

         _mesa_set_fetch_functions(texImage, 1);

         check_gen_mipmap(ctx, target, texObj, level);

         update_fbo_texture(ctx, texObj, face, level);

         /* state update */
         texObj->_Complete = GL_FALSE;
         ctx->NewState |= _NEW_TEXTURE;
      }
   }
   _mesa_unlock_texture(ctx, texObj);
}



void GLAPIENTRY
_mesa_CopyTexImage2D( GLenum target, GLint level, GLenum internalFormat,
                      GLint x, GLint y, GLsizei width, GLsizei height,
                      GLint border )
{
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   GLsizei postConvWidth = width, postConvHeight = height;
   const GLuint face = _mesa_tex_target_to_face(target);
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glCopyTexImage2D %s %d %s %d %d %d %d %d\n",
                  _mesa_lookup_enum_by_nr(target), level,
                  _mesa_lookup_enum_by_nr(internalFormat),
                  x, y, width, height, border);

   if (ctx->NewState & NEW_COPY_TEX_STATE)
      _mesa_update_state(ctx);

#if FEATURE_convolve
   if (_mesa_is_color_format(internalFormat)) {
      _mesa_adjust_image_for_convolution(ctx, 2, &postConvWidth,
                                         &postConvHeight);
   }
#endif

   if (copytexture_error_check(ctx, 2, target, level, internalFormat,
                               postConvWidth, postConvHeight, border))
      return;

   texObj = _mesa_get_current_tex_object(ctx, target);

   _mesa_lock_texture(ctx, texObj);
   {
      texImage = _mesa_get_tex_image(ctx, texObj, target, level);

      if (!texImage) {
	 _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyTexImage2D");
      }
      else {
         if (texImage->Data) {
            ctx->Driver.FreeTexImageData( ctx, texImage );
         }

         ASSERT(texImage->Data == NULL);

         clear_teximage_fields(texImage); /* not really needed, but helpful */
         _mesa_init_teximage_fields(ctx, target, texImage,
                                    postConvWidth, postConvHeight, 1,
                                    border, internalFormat);

         /* Choose actual texture format */
         texImage->TexFormat =
            ctx->Driver.ChooseTextureFormat(ctx, internalFormat,
                                            GL_NONE, GL_NONE);
         ASSERT(texImage->TexFormat != MESA_FORMAT_NONE);

         ASSERT(ctx->Driver.CopyTexImage2D);
         ctx->Driver.CopyTexImage2D(ctx, target, level, internalFormat,
                                    x, y, width, height, border);

         ASSERT(texImage->TexFormat);

         _mesa_set_fetch_functions(texImage, 2);

         check_gen_mipmap(ctx, target, texObj, level);

         update_fbo_texture(ctx, texObj, face, level);

         /* state update */
         texObj->_Complete = GL_FALSE;
         ctx->NewState |= _NEW_TEXTURE;
      }
   }
   _mesa_unlock_texture(ctx, texObj);
}


void GLAPIENTRY
_mesa_CopyTexSubImage1D( GLenum target, GLint level,
                         GLint xoffset, GLint x, GLint y, GLsizei width )
{
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   GLsizei postConvWidth = width;
   GLint yoffset = 0;
   GLsizei height = 1;

   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glCopyTexSubImage1D %s %d %d %d %d %d\n",
                  _mesa_lookup_enum_by_nr(target),
                  level, xoffset, x, y, width);

   if (ctx->NewState & NEW_COPY_TEX_STATE)
      _mesa_update_state(ctx);

   if (copytexsubimage_error_check1(ctx, 1, target, level))
      return;

   texObj = _mesa_get_current_tex_object(ctx, target);

   _mesa_lock_texture(ctx, texObj);
   {
      texImage = _mesa_select_tex_image(ctx, texObj, target, level);

#if FEATURE_convolve
      if (texImage && _mesa_is_color_format(texImage->InternalFormat)) {
         _mesa_adjust_image_for_convolution(ctx, 1, &postConvWidth, NULL);
      }
#endif

      if (copytexsubimage_error_check2(ctx, 1, target, level,
				       xoffset, 0, 0, postConvWidth, 1,
				       texImage)) {
         /* error was recorded */
      }
      else {
         /* If we have a border, xoffset=-1 is legal.  Bias by border width */
         xoffset += texImage->Border;

         if (_mesa_clip_copytexsubimage(ctx, &xoffset, &yoffset, &x, &y,
                                        &width, &height)) {
            ASSERT(ctx->Driver.CopyTexSubImage1D);
            ctx->Driver.CopyTexSubImage1D(ctx, target, level,
                                          xoffset, x, y, width);

            check_gen_mipmap(ctx, target, texObj, level);

            ctx->NewState |= _NEW_TEXTURE;
         }
      }
   }
   _mesa_unlock_texture(ctx, texObj);
}



void GLAPIENTRY
_mesa_CopyTexSubImage2D( GLenum target, GLint level,
                         GLint xoffset, GLint yoffset,
                         GLint x, GLint y, GLsizei width, GLsizei height )
{
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   GLsizei postConvWidth = width, postConvHeight = height;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glCopyTexSubImage2D %s %d %d %d %d %d %d %d\n",
                  _mesa_lookup_enum_by_nr(target),
                  level, xoffset, yoffset, x, y, width, height);

   if (ctx->NewState & NEW_COPY_TEX_STATE)
      _mesa_update_state(ctx);

   if (copytexsubimage_error_check1(ctx, 2, target, level))
      return;

   texObj = _mesa_get_current_tex_object(ctx, target);

   _mesa_lock_texture(ctx, texObj);
   {
      texImage = _mesa_select_tex_image(ctx, texObj, target, level);

#if FEATURE_convolve
      if (texImage && _mesa_is_color_format(texImage->InternalFormat)) {
         _mesa_adjust_image_for_convolution(ctx, 2,
                                            &postConvWidth, &postConvHeight);
      }
#endif

      if (copytexsubimage_error_check2(ctx, 2, target, level,
                                       xoffset, yoffset, 0,
				       postConvWidth, postConvHeight,
                                       texImage)) {
         /* error was recorded */
      }
      else {
         /* If we have a border, xoffset=-1 is legal.  Bias by border width */
         xoffset += texImage->Border;
         yoffset += texImage->Border;

         if (_mesa_clip_copytexsubimage(ctx, &xoffset, &yoffset, &x, &y,
                                        &width, &height)) {
            ASSERT(ctx->Driver.CopyTexSubImage2D);
            ctx->Driver.CopyTexSubImage2D(ctx, target, level, xoffset, yoffset,
                                          x, y, width, height);

            check_gen_mipmap(ctx, target, texObj, level);

            ctx->NewState |= _NEW_TEXTURE;
         }
      }
   }
   _mesa_unlock_texture(ctx, texObj);
}



void GLAPIENTRY
_mesa_CopyTexSubImage3D( GLenum target, GLint level,
                         GLint xoffset, GLint yoffset, GLint zoffset,
                         GLint x, GLint y, GLsizei width, GLsizei height )
{
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   GLsizei postConvWidth = width, postConvHeight = height;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glCopyTexSubImage3D %s %d %d %d %d %d %d %d %d\n",
                  _mesa_lookup_enum_by_nr(target),
                  level, xoffset, yoffset, zoffset, x, y, width, height);

   if (ctx->NewState & NEW_COPY_TEX_STATE)
      _mesa_update_state(ctx);

   if (copytexsubimage_error_check1(ctx, 3, target, level))
      return;

   texObj = _mesa_get_current_tex_object(ctx, target);

   _mesa_lock_texture(ctx, texObj);
   {
      texImage = _mesa_select_tex_image(ctx, texObj, target, level);

#if FEATURE_convolve
      if (texImage && _mesa_is_color_format(texImage->InternalFormat)) {
         _mesa_adjust_image_for_convolution(ctx, 2,
                                            &postConvWidth, &postConvHeight);
      }
#endif

      if (copytexsubimage_error_check2(ctx, 3, target, level, xoffset, yoffset,
				       zoffset, postConvWidth, postConvHeight,
				       texImage)) {
         /* error was recored */
      }
      else {
         /* If we have a border, xoffset=-1 is legal.  Bias by border width */
         xoffset += texImage->Border;
         yoffset += texImage->Border;
         zoffset += texImage->Border;

         if (_mesa_clip_copytexsubimage(ctx, &xoffset, &yoffset, &x, &y,
                                        &width, &height)) {
            ASSERT(ctx->Driver.CopyTexSubImage3D);
            ctx->Driver.CopyTexSubImage3D(ctx, target, level,
                                          xoffset, yoffset, zoffset,
                                          x, y, width, height);

            check_gen_mipmap(ctx, target, texObj, level);

            ctx->NewState |= _NEW_TEXTURE;
         }
      }
   }
   _mesa_unlock_texture(ctx, texObj);
}




/**********************************************************************/
/******                   Compressed Textures                    ******/
/**********************************************************************/


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


/*
 * Return compressed texture block size, in pixels.
 */
static void
get_compressed_block_size(GLenum glformat, GLuint *bw, GLuint *bh)
{
   gl_format mesaFormat = _mesa_glenum_to_compressed_format(glformat);
   _mesa_get_format_block_size(mesaFormat, bw, bh);
}


/**
 * Error checking for glCompressedTexImage[123]D().
 * \return error code or GL_NO_ERROR.
 */
static GLenum
compressed_texture_error_check(GLcontext *ctx, GLint dimensions,
                               GLenum target, GLint level,
                               GLenum internalFormat, GLsizei width,
                               GLsizei height, GLsizei depth, GLint border,
                               GLsizei imageSize)
{
   GLint expectedSize, maxLevels = 0, maxTextureSize;

   if (dimensions == 1) {
      /* 1D compressed textures not allowed */
      return GL_INVALID_ENUM;
   }
   else if (dimensions == 2) {
      if (target == GL_PROXY_TEXTURE_2D) {
         maxLevels = ctx->Const.MaxTextureLevels;
      }
      else if (target == GL_TEXTURE_2D) {
         maxLevels = ctx->Const.MaxTextureLevels;
      }
      else if (target == GL_PROXY_TEXTURE_CUBE_MAP_ARB) {
         if (!ctx->Extensions.ARB_texture_cube_map)
            return GL_INVALID_ENUM; /*target*/
         maxLevels = ctx->Const.MaxCubeTextureLevels;
      }
      else if (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB &&
               target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB) {
         if (!ctx->Extensions.ARB_texture_cube_map)
            return GL_INVALID_ENUM; /*target*/
         maxLevels = ctx->Const.MaxCubeTextureLevels;
      }
      else {
         return GL_INVALID_ENUM; /*target*/
      }
   }
   else if (dimensions == 3) {
      /* 3D compressed textures not allowed */
      return GL_INVALID_ENUM;
   }

   maxTextureSize = 1 << (maxLevels - 1);

   /* This will detect any invalid internalFormat value */
   if (!is_compressed_format(ctx, internalFormat))
      return GL_INVALID_ENUM;

   /* This should really never fail */
   if (_mesa_base_tex_format(ctx, internalFormat) < 0)
      return GL_INVALID_ENUM;

   if (border != 0)
      return GL_INVALID_VALUE;

   /*
    * XXX We should probably use the proxy texture error check function here.
    */
   if (width < 1 || width > maxTextureSize ||
       (!ctx->Extensions.ARB_texture_non_power_of_two && !_mesa_is_pow_two(width)))
      return GL_INVALID_VALUE;

   if ((height < 1 || height > maxTextureSize ||
       (!ctx->Extensions.ARB_texture_non_power_of_two && !_mesa_is_pow_two(height)))
       && dimensions > 1)
      return GL_INVALID_VALUE;

   if ((depth < 1 || depth > maxTextureSize ||
       (!ctx->Extensions.ARB_texture_non_power_of_two && !_mesa_is_pow_two(depth)))
       && dimensions > 2)
      return GL_INVALID_VALUE;

   /* For cube map, width must equal height */
   if (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB &&
       target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB && width != height)
      return GL_INVALID_VALUE;

   if (level < 0 || level >= maxLevels)
      return GL_INVALID_VALUE;

   expectedSize = compressed_tex_size(width, height, depth, internalFormat);
   if (expectedSize != imageSize)
      return GL_INVALID_VALUE;

#if FEATURE_EXT_texture_sRGB
   if ((internalFormat == GL_COMPRESSED_SRGB_S3TC_DXT1_EXT ||
        internalFormat == GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT ||
        internalFormat == GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT ||
        internalFormat == GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT)
       && border != 0) {
      return GL_INVALID_OPERATION;
   }
#endif

   return GL_NO_ERROR;
}


/**
 * Error checking for glCompressedTexSubImage[123]D().
 * \warning  There are some bad assumptions here about the size of compressed
 *           texture tiles (multiple of 4) used to test the validity of the
 *           offset and size parameters.
 * \return error code or GL_NO_ERROR.
 */
static GLenum
compressed_subtexture_error_check(GLcontext *ctx, GLint dimensions,
                                  GLenum target, GLint level,
                                  GLint xoffset, GLint yoffset, GLint zoffset,
                                  GLsizei width, GLsizei height, GLsizei depth,
                                  GLenum format, GLsizei imageSize)
{
   GLint expectedSize, maxLevels = 0, maxTextureSize;
   GLuint bw, bh;
   (void) zoffset;

   if (dimensions == 1) {
      /* 1D compressed textures not allowed */
      return GL_INVALID_ENUM;
   }
   else if (dimensions == 2) {
      if (target == GL_PROXY_TEXTURE_2D) {
         maxLevels = ctx->Const.MaxTextureLevels;
      }
      else if (target == GL_TEXTURE_2D) {
         maxLevels = ctx->Const.MaxTextureLevels;
      }
      else if (target == GL_PROXY_TEXTURE_CUBE_MAP_ARB) {
         if (!ctx->Extensions.ARB_texture_cube_map)
            return GL_INVALID_ENUM; /*target*/
         maxLevels = ctx->Const.MaxCubeTextureLevels;
      }
      else if (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB &&
               target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB) {
         if (!ctx->Extensions.ARB_texture_cube_map)
            return GL_INVALID_ENUM; /*target*/
         maxLevels = ctx->Const.MaxCubeTextureLevels;
      }
      else {
         return GL_INVALID_ENUM; /*target*/
      }
   }
   else if (dimensions == 3) {
      /* 3D compressed textures not allowed */
      return GL_INVALID_ENUM;
   }

   maxTextureSize = 1 << (maxLevels - 1);

   /* this will catch any invalid compressed format token */
   if (!is_compressed_format(ctx, format))
      return GL_INVALID_ENUM;

   if (width < 1 || width > maxTextureSize)
      return GL_INVALID_VALUE;

   if ((height < 1 || height > maxTextureSize)
       && dimensions > 1)
      return GL_INVALID_VALUE;

   if (level < 0 || level >= maxLevels)
      return GL_INVALID_VALUE;

   /*
    * do checks which depend on compression block size
    */
   get_compressed_block_size(format, &bw, &bh);

   if ((xoffset % bw != 0) || (yoffset % bh != 0))
      return GL_INVALID_VALUE;

   if ((width % bw != 0) && width != 2 && width != 1)
      return GL_INVALID_VALUE;

   if ((height % bh != 0) && height != 2 && height != 1)
      return GL_INVALID_VALUE;

   expectedSize = compressed_tex_size(width, height, depth, format);
   if (expectedSize != imageSize)
      return GL_INVALID_VALUE;

   return GL_NO_ERROR;
}


/**
 * Do second part of glCompressedTexSubImage error checking.
 * \return GL_TRUE if error found, GL_FALSE otherwise.
 */
static GLboolean
compressed_subtexture_error_check2(GLcontext *ctx, GLuint dims,
                                   GLsizei width, GLsizei height,
                                   GLsizei depth, GLenum format,
                                   struct gl_texture_image *texImage)
{

   if ((GLint) format != texImage->InternalFormat) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glCompressedTexSubImage%uD(format=0x%x)", dims, format);
      return GL_TRUE;
   }

   if (((width == 1 || width == 2) &&
        width != (GLsizei) texImage->Width) ||
       (width > (GLsizei) texImage->Width)) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glCompressedTexSubImage%uD(width=%d)", dims, width);
      return GL_TRUE;
   }

   if (dims >= 2) {
      if (((height == 1 || height == 2) &&
           height != (GLsizei) texImage->Height) ||
          (height > (GLsizei) texImage->Height)) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glCompressedTexSubImage%uD(height=%d)", dims, height);
         return GL_TRUE;
      }
   }

   if (dims >= 3) {
      if (((depth == 1 || depth == 2) &&
           depth != (GLsizei) texImage->Depth) ||
          (depth > (GLsizei) texImage->Depth)) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glCompressedTexSubImage%uD(depth=%d)", dims, depth);
         return GL_TRUE;
      }
   }

   return GL_FALSE;
}



void GLAPIENTRY
_mesa_CompressedTexImage1DARB(GLenum target, GLint level,
                              GLenum internalFormat, GLsizei width,
                              GLint border, GLsizei imageSize,
                              const GLvoid *data)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glCompressedTexImage1DARB %s %d %s %d %d %d %p\n",
                  _mesa_lookup_enum_by_nr(target), level,
                  _mesa_lookup_enum_by_nr(internalFormat),
                  width, border, imageSize, data);

   if (target == GL_TEXTURE_1D) {
      /* non-proxy target */
      struct gl_texture_object *texObj;
      struct gl_texture_image *texImage;
      GLenum error = compressed_texture_error_check(ctx, 1, target, level,
                               internalFormat, width, 1, 1, border, imageSize);
      if (error) {
         _mesa_error(ctx, error, "glCompressedTexImage1D");
         return;
      }

      texObj = _mesa_get_current_tex_object(ctx, target);

      _mesa_lock_texture(ctx, texObj);
      {
	 texImage = _mesa_get_tex_image(ctx, texObj, target, level);
	 if (!texImage) {
	    _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCompressedTexImage1D");
	 }
         else {
            if (texImage->Data) {
               ctx->Driver.FreeTexImageData( ctx, texImage );
            }
            ASSERT(texImage->Data == NULL);

            _mesa_init_teximage_fields(ctx, target, texImage, width, 1, 1,
                                       border, internalFormat);

            /* Choose actual texture format */
            texImage->TexFormat =
               ctx->Driver.ChooseTextureFormat(ctx, internalFormat,
                                               GL_NONE, GL_NONE);
            ASSERT(texImage->TexFormat != MESA_FORMAT_NONE);

            ASSERT(ctx->Driver.CompressedTexImage1D);
            ctx->Driver.CompressedTexImage1D(ctx, target, level,
                                             internalFormat, width, border,
                                             imageSize, data,
                                             texObj, texImage);

            _mesa_set_fetch_functions(texImage, 1);

            check_gen_mipmap(ctx, target, texObj, level);

            /* state update */
            texObj->_Complete = GL_FALSE;
            ctx->NewState |= _NEW_TEXTURE;
         }
      }
      _mesa_unlock_texture(ctx, texObj);
   }
   else if (target == GL_PROXY_TEXTURE_1D) {
      /* Proxy texture: check for errors and update proxy state */
      GLenum error = compressed_texture_error_check(ctx, 1, target, level,
                               internalFormat, width, 1, 1, border, imageSize);
      if (!error) {
         ASSERT(ctx->Driver.TestProxyTexImage);
         error = !(*ctx->Driver.TestProxyTexImage)(ctx, target, level,
                                             internalFormat, GL_NONE, GL_NONE,
                                             width, 1, 1, border);
      }
      if (error) {
         /* if error, clear all proxy texture image parameters */
         struct gl_texture_image *texImage;
         texImage = _mesa_get_proxy_tex_image(ctx, target, level);
         if (texImage)
            clear_teximage_fields(texImage);
      }
      else {
         /* store the teximage parameters */
         struct gl_texture_object *texObj;
         struct gl_texture_image *texImage;

         texObj = _mesa_get_current_tex_object(ctx, target);

	 _mesa_lock_texture(ctx, texObj);
	 {
	    texImage = _mesa_select_tex_image(ctx, texObj, target, level);
	    _mesa_init_teximage_fields(ctx, target, texImage, width, 1, 1,
				       border, internalFormat);
	 }
	 _mesa_unlock_texture(ctx, texObj);
      }
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glCompressedTexImage1D(target)");
      return;
   }
}


void GLAPIENTRY
_mesa_CompressedTexImage2DARB(GLenum target, GLint level,
                              GLenum internalFormat, GLsizei width,
                              GLsizei height, GLint border, GLsizei imageSize,
                              const GLvoid *data)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glCompressedTexImage2DARB %s %d %s %d %d %d %d %p\n",
                  _mesa_lookup_enum_by_nr(target), level,
                  _mesa_lookup_enum_by_nr(internalFormat),
                  width, height, border, imageSize, data);

   if (target == GL_TEXTURE_2D ||
       (ctx->Extensions.ARB_texture_cube_map &&
        target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB &&
        target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB)) {
      /* non-proxy target */
      struct gl_texture_object *texObj;
      struct gl_texture_image *texImage;

      GLenum error = compressed_texture_error_check(ctx, 2, target, level,
                          internalFormat, width, height, 1, border, imageSize);
      if (error) {
         _mesa_error(ctx, error, "glCompressedTexImage2D");
         return;
      }

      texObj = _mesa_get_current_tex_object(ctx, target);

      _mesa_lock_texture(ctx, texObj);
      {
	 texImage = _mesa_get_tex_image(ctx, texObj, target, level);
	 if (!texImage) {
	    _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCompressedTexImage2D");
	 }
         else {
            if (texImage->Data) {
               ctx->Driver.FreeTexImageData( ctx, texImage );
            }
            ASSERT(texImage->Data == NULL);

            _mesa_init_teximage_fields(ctx, target, texImage, width, height, 1,
                                       border, internalFormat);

            /* Choose actual texture format */
            texImage->TexFormat =
               ctx->Driver.ChooseTextureFormat(ctx, internalFormat,
                                               GL_NONE, GL_NONE);
            ASSERT(texImage->TexFormat != MESA_FORMAT_NONE);

            ASSERT(ctx->Driver.CompressedTexImage2D);
            ctx->Driver.CompressedTexImage2D(ctx, target, level,
                                             internalFormat, width, height,
                                             border, imageSize, data,
                                             texObj, texImage);

            _mesa_set_fetch_functions(texImage, 2);

            check_gen_mipmap(ctx, target, texObj, level);

            /* state update */
            texObj->_Complete = GL_FALSE;
            ctx->NewState |= _NEW_TEXTURE;
         }
      }
      _mesa_unlock_texture(ctx, texObj);
   }
   else if (target == GL_PROXY_TEXTURE_2D ||
            (target == GL_PROXY_TEXTURE_CUBE_MAP_ARB &&
             ctx->Extensions.ARB_texture_cube_map)) {
      /* Proxy texture: check for errors and update proxy state */
      GLenum error = compressed_texture_error_check(ctx, 2, target, level,
                          internalFormat, width, height, 1, border, imageSize);
      if (!error) {
         ASSERT(ctx->Driver.TestProxyTexImage);
         error = !(*ctx->Driver.TestProxyTexImage)(ctx, target, level,
                                              internalFormat, GL_NONE, GL_NONE,
                                              width, height, 1, border);
      }
      if (error) {
         /* if error, clear all proxy texture image parameters */
         struct gl_texture_image *texImage;
         texImage = _mesa_get_proxy_tex_image(ctx, target, level);
         if (texImage)
            clear_teximage_fields(texImage);
      }
      else {
         /* store the teximage parameters */
         struct gl_texture_object *texObj;
         struct gl_texture_image *texImage;

         texObj = _mesa_get_current_tex_object(ctx, target);

	 _mesa_lock_texture(ctx, texObj);
	 {
	    texImage = _mesa_select_tex_image(ctx, texObj, target, level);
	    _mesa_init_teximage_fields(ctx, target, texImage, width, height, 1,
				       border, internalFormat);
	 }
	 _mesa_unlock_texture(ctx, texObj);
      }
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glCompressedTexImage2D(target)");
      return;
   }
}


void GLAPIENTRY
_mesa_CompressedTexImage3DARB(GLenum target, GLint level,
                              GLenum internalFormat, GLsizei width,
                              GLsizei height, GLsizei depth, GLint border,
                              GLsizei imageSize, const GLvoid *data)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glCompressedTexImage3DARB %s %d %s %d %d %d %d %d %p\n",
                  _mesa_lookup_enum_by_nr(target), level,
                  _mesa_lookup_enum_by_nr(internalFormat),
                  width, height, depth, border, imageSize, data);

   if (target == GL_TEXTURE_3D) {
      /* non-proxy target */
      struct gl_texture_object *texObj;
      struct gl_texture_image *texImage;
      GLenum error = compressed_texture_error_check(ctx, 3, target, level,
                      internalFormat, width, height, depth, border, imageSize);
      if (error) {
         _mesa_error(ctx, error, "glCompressedTexImage3D");
         return;
      }

      texObj = _mesa_get_current_tex_object(ctx, target);

      _mesa_lock_texture(ctx, texObj);
      {
	 texImage = _mesa_get_tex_image(ctx, texObj, target, level);
	 if (!texImage) {
	    _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCompressedTexImage3D");
	 }
         else {
            if (texImage->Data) {
               ctx->Driver.FreeTexImageData( ctx, texImage );
            }
            ASSERT(texImage->Data == NULL);

            _mesa_init_teximage_fields(ctx, target, texImage,
                                       width, height, depth,
                                       border, internalFormat);

            /* Choose actual texture format */
            texImage->TexFormat =
               ctx->Driver.ChooseTextureFormat(ctx, internalFormat,
                                               GL_NONE, GL_NONE);
            ASSERT(texImage->TexFormat != MESA_FORMAT_NONE);

            ASSERT(ctx->Driver.CompressedTexImage3D);
            ctx->Driver.CompressedTexImage3D(ctx, target, level,
                                             internalFormat,
                                             width, height, depth,
                                             border, imageSize, data,
                                             texObj, texImage);

            _mesa_set_fetch_functions(texImage, 3);

            check_gen_mipmap(ctx, target, texObj, level);

            /* state update */
            texObj->_Complete = GL_FALSE;
            ctx->NewState |= _NEW_TEXTURE;
         }
      }
      _mesa_unlock_texture(ctx, texObj);
   }
   else if (target == GL_PROXY_TEXTURE_3D) {
      /* Proxy texture: check for errors and update proxy state */
      GLenum error = compressed_texture_error_check(ctx, 3, target, level,
                      internalFormat, width, height, depth, border, imageSize);
      if (!error) {
         ASSERT(ctx->Driver.TestProxyTexImage);
         error = !(*ctx->Driver.TestProxyTexImage)(ctx, target, level,
                                             internalFormat, GL_NONE, GL_NONE,
                                             width, height, depth, border);
      }
      if (error) {
         /* if error, clear all proxy texture image parameters */
         struct gl_texture_image *texImage;
         texImage = _mesa_get_proxy_tex_image(ctx, target, level);
         if (texImage)
            clear_teximage_fields(texImage);
      }
      else {
         /* store the teximage parameters */
         struct gl_texture_object *texObj;
         struct gl_texture_image *texImage;

         texObj = _mesa_get_current_tex_object(ctx, target);

	 _mesa_lock_texture(ctx, texObj);
	 {
	    texImage = _mesa_select_tex_image(ctx, texObj, target, level);
	    _mesa_init_teximage_fields(ctx, target, texImage, width, height,
				       depth, border, internalFormat);
	 }
	 _mesa_unlock_texture(ctx, texObj);
      }
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glCompressedTexImage3D(target)");
      return;
   }
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
   GLenum error;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   error = compressed_subtexture_error_check(ctx, dims, target, level,
                                             xoffset, 0, 0, /* pos */
                                             width, height, depth,   /* size */
                                             format, imageSize);
   if (error) {
      _mesa_error(ctx, error, "glCompressedTexSubImage%uD", dims);
      return;
   }

   texObj = _mesa_get_current_tex_object(ctx, target);

   _mesa_lock_texture(ctx, texObj);
   {
      texImage = _mesa_select_tex_image(ctx, texObj, target, level);
      assert(texImage);

      if (compressed_subtexture_error_check2(ctx, dims, width, height, depth,
                                             format, texImage)) {
         /* error was recorded */
      }
      else if (width > 0 && height > 0 && depth > 0) {
         switch (dims) {
         case 1:
            if (ctx->Driver.CompressedTexSubImage1D) {
               ctx->Driver.CompressedTexSubImage1D(ctx, target, level,
                                                   xoffset, width,
                                                   format, imageSize, data,
                                                   texObj, texImage);
            }
            break;
         case 2:
            if (ctx->Driver.CompressedTexSubImage2D) {
               ctx->Driver.CompressedTexSubImage2D(ctx, target, level,
                                                   xoffset, yoffset,
                                                   width, height,
                                                   format, imageSize, data,
                                                   texObj, texImage);
            }
            break;
         case 3:
            if (ctx->Driver.CompressedTexSubImage3D) {
               ctx->Driver.CompressedTexSubImage3D(ctx, target, level,
                                                   xoffset, yoffset, zoffset,
                                                   width, height, depth,
                                                   format, imageSize, data,
                                                   texObj, texImage);
            }
            break;
         default:
            ;
         }

         check_gen_mipmap(ctx, target, texObj, level);

         ctx->NewState |= _NEW_TEXTURE;
      }
   }
   _mesa_unlock_texture(ctx, texObj);
}


void GLAPIENTRY
_mesa_CompressedTexSubImage1DARB(GLenum target, GLint level, GLint xoffset,
                                 GLsizei width, GLenum format,
                                 GLsizei imageSize, const GLvoid *data)
{
   compressed_tex_sub_image(1, target, level, xoffset, 0, 0, width, 1, 1,
                            format, imageSize, data);
}


void GLAPIENTRY
_mesa_CompressedTexSubImage2DARB(GLenum target, GLint level, GLint xoffset,
                                 GLint yoffset, GLsizei width, GLsizei height,
                                 GLenum format, GLsizei imageSize,
                                 const GLvoid *data)
{
   compressed_tex_sub_image(2, target, level, xoffset, yoffset, 0,
                            width, height, 1, format, imageSize, data);
}


void GLAPIENTRY
_mesa_CompressedTexSubImage3DARB(GLenum target, GLint level, GLint xoffset,
                                 GLint yoffset, GLint zoffset, GLsizei width,
                                 GLsizei height, GLsizei depth, GLenum format,
                                 GLsizei imageSize, const GLvoid *data)
{
   compressed_tex_sub_image(3, target, level, xoffset, yoffset, zoffset,
                            width, height, depth, format, imageSize, data);
}



/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2013 LunarG, Inc.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Courtney Goeltzenleuchter <courtney@lunarg.com>
 */


/**
 * \file textureview.c
 * GL_ARB_texture_view functions
 */

#include "glheader.h"
#include "context.h"
#include "enums.h"
#include "imports.h"
#include "macros.h"
#include "teximage.h"
#include "texobj.h"
#include "mipmap.h"
#include "texstorage.h"
#include "textureview.h"
#include "stdbool.h"
#include "mtypes.h"

/* Table 3.X.2 (Compatible internal formats for TextureView)
    ---------------------------------------------------------------------------
    | Class                 | Internal formats                                |
    ---------------------------------------------------------------------------
    | VIEW_CLASS_128_BITS   | RGBA32F, RGBA32UI, RGBA32I                      |
    ---------------------------------------------------------------------------
    | VIEW_CLASS_96_BITS    | RGB32F, RGB32UI, RGB32I                         |
    ---------------------------------------------------------------------------
    | VIEW_CLASS_64_BITS    | RGBA16F, RG32F, RGBA16UI, RG32UI, RGBA16I,      |
    |                       | RG32I, RGBA16, RGBA16_SNORM                     |
    ---------------------------------------------------------------------------
    | VIEW_CLASS_48_BITS    | RGB16, RGB16_SNORM, RGB16F, RGB16UI, RGB16I     |
    ---------------------------------------------------------------------------
    | VIEW_CLASS_32_BITS    | RG16F, R11F_G11F_B10F, R32F,                    |
    |                       | RGB10_A2UI, RGBA8UI, RG16UI, R32UI,             |
    |                       | RGBA8I, RG16I, R32I, RGB10_A2, RGBA8, RG16,     |
    |                       | RGBA8_SNORM, RG16_SNORM, SRGB8_ALPHA8, RGB9_E5  |
    ---------------------------------------------------------------------------
    | VIEW_CLASS_24_BITS    | RGB8, RGB8_SNORM, SRGB8, RGB8UI, RGB8I          |
    ---------------------------------------------------------------------------
    | VIEW_CLASS_16_BITS    | R16F, RG8UI, R16UI, RG8I, R16I, RG8, R16,       |
    |                       | RG8_SNORM, R16_SNORM                            |
    ---------------------------------------------------------------------------
    | VIEW_CLASS_8_BITS     | R8UI, R8I, R8, R8_SNORM                         |
    ---------------------------------------------------------------------------
    | VIEW_CLASS_RGTC1_RED  | COMPRESSED_RED_RGTC1,                           |
    |                       | COMPRESSED_SIGNED_RED_RGTC1                     |
    ---------------------------------------------------------------------------
    | VIEW_CLASS_RGTC2_RG   | COMPRESSED_RG_RGTC2,                            |
    |                       | COMPRESSED_SIGNED_RG_RGTC2                      |
    ---------------------------------------------------------------------------
    | VIEW_CLASS_BPTC_UNORM | COMPRESSED_RGBA_BPTC_UNORM,                     |
    |                       | COMPRESSED_SRGB_ALPHA_BPTC_UNORM                |
    ---------------------------------------------------------------------------
    | VIEW_CLASS_BPTC_FLOAT | COMPRESSED_RGB_BPTC_SIGNED_FLOAT,               |
    |                       | COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT              |
    ---------------------------------------------------------------------------
 */
struct internal_format_class_info {
   GLenum view_class;
   GLenum internal_format;
};
static const struct internal_format_class_info compatible_internal_formats[] = {
   {GL_VIEW_CLASS_128_BITS, GL_RGBA32F},
   {GL_VIEW_CLASS_128_BITS, GL_RGBA32UI},
   {GL_VIEW_CLASS_128_BITS, GL_RGBA32I},
   {GL_VIEW_CLASS_96_BITS, GL_RGB32F},
   {GL_VIEW_CLASS_96_BITS, GL_RGB32UI},
   {GL_VIEW_CLASS_96_BITS, GL_RGB32I},
   {GL_VIEW_CLASS_64_BITS, GL_RGBA16F},
   {GL_VIEW_CLASS_64_BITS, GL_RG32F},
   {GL_VIEW_CLASS_64_BITS, GL_RGBA16UI},
   {GL_VIEW_CLASS_64_BITS, GL_RG32UI},
   {GL_VIEW_CLASS_64_BITS, GL_RGBA16I},
   {GL_VIEW_CLASS_64_BITS, GL_RG32I},
   {GL_VIEW_CLASS_64_BITS, GL_RGBA16},
   {GL_VIEW_CLASS_64_BITS, GL_RGBA16_SNORM},
   {GL_VIEW_CLASS_48_BITS, GL_RGB16},
   {GL_VIEW_CLASS_48_BITS, GL_RGB16_SNORM},
   {GL_VIEW_CLASS_48_BITS, GL_RGB16F},
   {GL_VIEW_CLASS_48_BITS, GL_RGB16UI},
   {GL_VIEW_CLASS_48_BITS, GL_RGB16I},
   {GL_VIEW_CLASS_32_BITS, GL_RG16F},
   {GL_VIEW_CLASS_32_BITS, GL_R11F_G11F_B10F},
   {GL_VIEW_CLASS_32_BITS, GL_R32F},
   {GL_VIEW_CLASS_32_BITS, GL_RGB10_A2UI},
   {GL_VIEW_CLASS_32_BITS, GL_RGBA8UI},
   {GL_VIEW_CLASS_32_BITS, GL_RG16UI},
   {GL_VIEW_CLASS_32_BITS, GL_R32UI},
   {GL_VIEW_CLASS_32_BITS, GL_RGBA8I},
   {GL_VIEW_CLASS_32_BITS, GL_RG16I},
   {GL_VIEW_CLASS_32_BITS, GL_R32I},
   {GL_VIEW_CLASS_32_BITS, GL_RGB10_A2},
   {GL_VIEW_CLASS_32_BITS, GL_RGBA8},
   {GL_VIEW_CLASS_32_BITS, GL_RG16},
   {GL_VIEW_CLASS_32_BITS, GL_RGBA8_SNORM},
   {GL_VIEW_CLASS_32_BITS, GL_RG16_SNORM},
   {GL_VIEW_CLASS_32_BITS, GL_SRGB8_ALPHA8},
   {GL_VIEW_CLASS_32_BITS, GL_RGB9_E5},
   {GL_VIEW_CLASS_24_BITS, GL_RGB8},
   {GL_VIEW_CLASS_24_BITS, GL_RGB8_SNORM},
   {GL_VIEW_CLASS_24_BITS, GL_SRGB8},
   {GL_VIEW_CLASS_24_BITS, GL_RGB8UI},
   {GL_VIEW_CLASS_24_BITS, GL_RGB8I},
   {GL_VIEW_CLASS_16_BITS, GL_R16F},
   {GL_VIEW_CLASS_16_BITS, GL_RG8UI},
   {GL_VIEW_CLASS_16_BITS, GL_R16UI},
   {GL_VIEW_CLASS_16_BITS, GL_RG8I},
   {GL_VIEW_CLASS_16_BITS, GL_R16I},
   {GL_VIEW_CLASS_16_BITS, GL_RG8},
   {GL_VIEW_CLASS_16_BITS, GL_R16},
   {GL_VIEW_CLASS_16_BITS, GL_RG8_SNORM},
   {GL_VIEW_CLASS_16_BITS, GL_R16_SNORM},
   {GL_VIEW_CLASS_8_BITS, GL_R8UI},
   {GL_VIEW_CLASS_8_BITS, GL_R8I},
   {GL_VIEW_CLASS_8_BITS, GL_R8},
   {GL_VIEW_CLASS_8_BITS, GL_R8_SNORM},
   {GL_VIEW_CLASS_RGTC1_RED, GL_COMPRESSED_RED_RGTC1},
   {GL_VIEW_CLASS_RGTC1_RED, GL_COMPRESSED_SIGNED_RED_RGTC1},
   {GL_VIEW_CLASS_RGTC2_RG, GL_COMPRESSED_RG_RGTC2},
   {GL_VIEW_CLASS_RGTC2_RG, GL_COMPRESSED_SIGNED_RG_RGTC2},
   {GL_VIEW_CLASS_BPTC_UNORM, GL_COMPRESSED_RGBA_BPTC_UNORM_ARB},
   {GL_VIEW_CLASS_BPTC_UNORM, GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB},
   {GL_VIEW_CLASS_BPTC_FLOAT, GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB},
   {GL_VIEW_CLASS_BPTC_FLOAT, GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB},
};

static const struct internal_format_class_info s3tc_compatible_internal_formats[] = {
   {GL_VIEW_CLASS_S3TC_DXT1_RGB, GL_COMPRESSED_RGB_S3TC_DXT1_EXT},
   {GL_VIEW_CLASS_S3TC_DXT1_RGB, GL_COMPRESSED_SRGB_S3TC_DXT1_EXT},
   {GL_VIEW_CLASS_S3TC_DXT1_RGBA, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT},
   {GL_VIEW_CLASS_S3TC_DXT1_RGBA, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT},
   {GL_VIEW_CLASS_S3TC_DXT3_RGBA, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT},
   {GL_VIEW_CLASS_S3TC_DXT3_RGBA, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT},
   {GL_VIEW_CLASS_S3TC_DXT5_RGBA, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT},
   {GL_VIEW_CLASS_S3TC_DXT5_RGBA, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT},
};

/**
 * Lookup format view class based on internalformat
 * \return VIEW_CLASS if internalformat found in table, false otherwise.
 */
static GLenum
lookup_view_class(struct gl_context *ctx, GLenum internalformat)
{
   GLuint i;

   for (i = 0; i < ARRAY_SIZE(compatible_internal_formats); i++) {
      if (compatible_internal_formats[i].internal_format == internalformat)
         return compatible_internal_formats[i].view_class;
   }

   if (ctx->Extensions.EXT_texture_compression_s3tc && ctx->Extensions.EXT_texture_sRGB) {
      for (i = 0; i < ARRAY_SIZE(s3tc_compatible_internal_formats); i++) {
         if (s3tc_compatible_internal_formats[i].internal_format == internalformat)
            return s3tc_compatible_internal_formats[i].view_class;
      }
   }
   return GL_FALSE;
}

/**
 * Initialize new texture's gl_texture_image structures. Will not call driver
 * to allocate new space, simply record relevant layer, face, format, etc.
 * \return GL_FALSE if any error, GL_TRUE otherwise.
 */
static GLboolean
initialize_texture_fields(struct gl_context *ctx,
                          GLenum target,
                          struct gl_texture_object *texObj,
                          GLint levels,
                          GLsizei width, GLsizei height, GLsizei depth,
                          GLenum internalFormat, mesa_format texFormat)
{
   const GLuint numFaces = _mesa_num_tex_faces(target);
   GLint level, levelWidth = width, levelHeight = height, levelDepth = depth;
   GLuint face;

   /* Pretend we are bound to initialize the gl_texture_image structs */
   texObj->Target = target;

   /* Set up all the texture object's gl_texture_images */
   for (level = 0; level < levels; level++) {
      for (face = 0; face < numFaces; face++) {
         struct gl_texture_image *texImage;
         GLenum faceTarget = target;

         if (target == GL_TEXTURE_CUBE_MAP)
            faceTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X + face;

         texImage = _mesa_get_tex_image(ctx, texObj, faceTarget, level);

         if (!texImage) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexStorage");
            return GL_FALSE;
         }

         _mesa_init_teximage_fields(ctx, texImage,
                                    levelWidth, levelHeight, levelDepth,
                                    0, internalFormat, texFormat);
      }

      _mesa_next_mipmap_level_size(target, 0, levelWidth, levelHeight, levelDepth,
                                   &levelWidth, &levelHeight, &levelDepth);
   }

   /* "unbind" */
   texObj->Target = 0;

   return GL_TRUE;
}

#define RETURN_IF_SUPPORTED(t) do {		\
   if (newTarget == GL_ ## t)                   \
      return true;				\
} while (0)

/**
 * Check for compatible target
 * If an error is found, record it with _mesa_error()
 * \return false if any error, true otherwise.
 */
static bool
target_valid(struct gl_context *ctx, GLenum origTarget, GLenum newTarget)
{
   /*
    * From ARB_texture_view spec:
   ---------------------------------------------------------------------------------------------------------
   | Original target              | Valid new targets |
   ---------------------------------------------------------------------------------------------------------
   | TEXTURE_1D                   | TEXTURE_1D, TEXTURE_1D_ARRAY |
   | ------------------------------------------------------------------------------------------------------- |
   | TEXTURE_2D                   | TEXTURE_2D, TEXTURE_2D_ARRAY |
   | ------------------------------------------------------------------------------------------------------- |
   | TEXTURE_3D                   | TEXTURE_3D |
   | ------------------------------------------------------------------------------------------------------- |
   | TEXTURE_CUBE_MAP             | TEXTURE_CUBE_MAP, TEXTURE_2D, TEXTURE_2D_ARRAY, TEXTURE_CUBE_MAP_ARRAY |
   | ------------------------------------------------------------------------------------------------------- |
   | TEXTURE_RECTANGLE            | TEXTURE_RECTANGLE |
   | ------------------------------------------------------------------------------------------------------- |
   | TEXTURE_BUFFER               | <none> |
   | ------------------------------------------------------------------------------------------------------- |
   | TEXTURE_1D_ARRAY             | TEXTURE_1D_ARRAY, TEXTURE_1D |
   | ------------------------------------------------------------------------------------------------------- |
   | TEXTURE_2D_ARRAY             | TEXTURE_2D_ARRAY, TEXTURE_2D, TEXTURE_CUBE_MAP, TEXTURE_CUBE_MAP_ARRAY |
   | ------------------------------------------------------------------------------------------------------- |
   | TEXTURE_CUBE_MAP_ARRAY       | TEXTURE_CUBE_MAP_ARRAY, TEXTURE_2D_ARRAY, TEXTURE_2D, TEXTURE_CUBE_MAP |
   | ------------------------------------------------------------------------------------------------------- |
   | TEXTURE_2D_MULTISAMPLE       | TEXTURE_2D_MULTISAMPLE, TEXTURE_2D_MULTISAMPLE_ARRAY |
   | ------------------------------------------------------------------------------------------------------- |
   | TEXTURE_2D_MULTISAMPLE_ARRAY | TEXTURE_2D_MULTISAMPLE, TEXTURE_2D_MULTISAMPLE_ARRAY |
   ---------------------------------------------------------------------------------------------------------
    */

   switch (origTarget) {
   case GL_TEXTURE_1D:
   case GL_TEXTURE_1D_ARRAY:
      RETURN_IF_SUPPORTED(TEXTURE_1D);
      RETURN_IF_SUPPORTED(TEXTURE_1D_ARRAY);
      break;
   case GL_TEXTURE_2D:
      RETURN_IF_SUPPORTED(TEXTURE_2D);
      RETURN_IF_SUPPORTED(TEXTURE_2D_ARRAY);
      break;
   case GL_TEXTURE_3D:
      RETURN_IF_SUPPORTED(TEXTURE_3D);
      break;
   case GL_TEXTURE_RECTANGLE:
      RETURN_IF_SUPPORTED(TEXTURE_RECTANGLE);
      break;
   case GL_TEXTURE_CUBE_MAP:
   case GL_TEXTURE_2D_ARRAY:
   case GL_TEXTURE_CUBE_MAP_ARRAY:
      RETURN_IF_SUPPORTED(TEXTURE_2D);
      RETURN_IF_SUPPORTED(TEXTURE_2D_ARRAY);
      RETURN_IF_SUPPORTED(TEXTURE_CUBE_MAP);
      RETURN_IF_SUPPORTED(TEXTURE_CUBE_MAP_ARRAY);
      break;
   case GL_TEXTURE_2D_MULTISAMPLE:
   case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
      RETURN_IF_SUPPORTED(TEXTURE_2D_MULTISAMPLE);
      RETURN_IF_SUPPORTED(TEXTURE_2D_MULTISAMPLE_ARRAY);
      break;
   }
   _mesa_error(ctx, GL_INVALID_OPERATION,
               "glTextureView(illegal target=%s)",
               _mesa_lookup_enum_by_nr(newTarget));
   return false;
}
#undef RETURN_IF_SUPPORTED

/**
 * Check for compatible format
 * If an error is found, record it with _mesa_error()
 * \return false if any error, true otherwise.
 */
GLboolean
_mesa_texture_view_compatible_format(struct gl_context *ctx,
                                     GLenum origInternalFormat,
                                     GLenum newInternalFormat)
{
   unsigned int origViewClass, newViewClass;

   /* The two textures' internal formats must be compatible according to
    * Table 3.X.2 (Compatible internal formats for TextureView)
    * if the internal format exists in that table the view class must match.
    * The internal formats must be identical if not in that table,
    * or an INVALID_OPERATION error is generated.
    */
   if (origInternalFormat == newInternalFormat)
      return GL_TRUE;

   origViewClass = lookup_view_class(ctx, origInternalFormat);
   newViewClass = lookup_view_class(ctx, newInternalFormat);
   if ((origViewClass == newViewClass) && origViewClass != false)
      return GL_TRUE;

   return GL_FALSE;
}
/**
 * Helper function for TexStorage and teximagemultisample to set immutable
 * texture state needed by ARB_texture_view.
 */
void
_mesa_set_texture_view_state(struct gl_context *ctx,
                             struct gl_texture_object *texObj,
                             GLenum target, GLuint levels)
{
   struct gl_texture_image *texImage;

   /* Get a reference to what will become this View's base level */
   texImage = _mesa_select_tex_image(ctx, texObj, target, 0);

   /* When an immutable texture is created via glTexStorage or glTexImageMultisample,
    * TEXTURE_IMMUTABLE_FORMAT becomes TRUE.
    * TEXTURE_IMMUTABLE_LEVELS and TEXTURE_VIEW_NUM_LEVELS become levels.
    * If the texture target is TEXTURE_1D_ARRAY then
    * TEXTURE_VIEW_NUM_LAYERS becomes height.
    * If the texture target is TEXTURE_2D_ARRAY, TEXTURE_CUBE_MAP_ARRAY,
    * or TEXTURE_2D_MULTISAMPLE_ARRAY then TEXTURE_VIEW_NUM_LAYERS becomes depth.
    * If the texture target is TEXTURE_CUBE_MAP, then
    * TEXTURE_VIEW_NUM_LAYERS becomes 6.
    * For any other texture target, TEXTURE_VIEW_NUM_LAYERS becomes 1.
    * 
    * ARB_texture_multisample: Multisample textures do
    * not have multiple image levels.
    */

   texObj->Immutable = GL_TRUE;
   texObj->ImmutableLevels = levels;
   texObj->MinLevel = 0;
   texObj->NumLevels = levels;
   texObj->MinLayer = 0;
   texObj->NumLayers = 1;
   switch (target) {
   case GL_TEXTURE_1D_ARRAY:
      texObj->NumLayers = texImage->Height;
      break;

   case GL_TEXTURE_2D_MULTISAMPLE:
      texObj->NumLevels = 1;
      texObj->ImmutableLevels = 1;
      break;

   case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
      texObj->NumLevels = 1;
      texObj->ImmutableLevels = 1;
      /* fall through to set NumLayers */

   case GL_TEXTURE_2D_ARRAY:
   case GL_TEXTURE_CUBE_MAP_ARRAY:
      texObj->NumLayers = texImage->Depth;
      break;

   case GL_TEXTURE_CUBE_MAP:
      texObj->NumLayers = 6;
      break;

   }
}

/**
 * glTextureView (ARB_texture_view)
 * If an error is found, record it with _mesa_error()
 * \return none.
 */
void GLAPIENTRY
_mesa_TextureView(GLuint texture, GLenum target, GLuint origtexture,
                  GLenum internalformat,
                  GLuint minlevel, GLuint numlevels,
                  GLuint minlayer, GLuint numlayers)
{
   struct gl_texture_object *texObj;
   struct gl_texture_object *origTexObj;
   struct gl_texture_image *origTexImage;
   GLuint newViewMinLevel, newViewMinLayer;
   GLuint newViewNumLevels, newViewNumLayers;
   GLsizei width, height, depth;
   mesa_format texFormat;
   GLboolean sizeOK, dimensionsOK;
   GLenum faceTarget;

   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & (VERBOSE_API | VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glTextureView %d %s %d %s %d %d %d %d\n",
                  texture, _mesa_lookup_enum_by_nr(target), origtexture,
                  _mesa_lookup_enum_by_nr(internalformat),
                  minlevel, numlevels, minlayer, numlayers);

   if (origtexture == 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glTextureView(origtexture = %u)", origtexture);
      return;
   }

   /* Need original texture information to validate arguments */
   origTexObj = _mesa_lookup_texture(ctx, origtexture);

   /* If <origtexture> is not the name of a texture, INVALID_VALUE is generated. */
   if (!origTexObj) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glTextureView(origtexture = %u)", origtexture);
      return;
   }

   /* If <origtexture>'s TEXTURE_IMMUTABLE_FORMAT value is not TRUE,
    * INVALID_OPERATION is generated.
    */
   if (!origTexObj->Immutable) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glTextureView(origtexture not immutable)");
      return;
   }

   /* If <texture> is 0, INVALID_VALUE is generated. */
   if (texture == 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glTextureView(texture = 0)");
      return;
   }

   /* If <texture> is not a valid name returned by GenTextures,
    * the error INVALID_OPERATION is generated.
    */
   texObj = _mesa_lookup_texture(ctx, texture);
   if (texObj == NULL) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glTextureView(texture = %u non-gen name)", texture);
      return;
   }

   /* If <texture> has already been bound and given a target, then
    * the error INVALID_OPERATION is generated.
    */
   if (texObj->Target) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glTextureView(texture = %u already bound)", texture);
      return;
   }

   /* Check for compatible target */
   if (!target_valid(ctx, origTexObj->Target, target)) {
      return; /* error was recorded */
   }

   /* minlevel and minlayer are relative to the view of origtexture
    * If minlevel or minlayer is greater than level or layer, respectively,
    * of origtexture return INVALID_VALUE.
    */
   newViewMinLevel = origTexObj->MinLevel + minlevel;
   newViewMinLayer = origTexObj->MinLayer + minlayer;
   if (newViewMinLevel >= (origTexObj->MinLevel + origTexObj->NumLevels)) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glTextureView(new minlevel (%d) > orig minlevel (%d) + orig numlevels (%d))",
                  newViewMinLevel, origTexObj->MinLevel, origTexObj->NumLevels);
      return;
   }

   if (newViewMinLayer >= (origTexObj->MinLayer + origTexObj->NumLayers)) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glTextureView(new minlayer (%d) > orig minlayer (%d) + orig numlayers (%d))",
                  newViewMinLayer, origTexObj->MinLayer, origTexObj->NumLayers);
      return;
   }

   if (!_mesa_texture_view_compatible_format(ctx,
                                             origTexObj->Image[0][0]->InternalFormat,
                                             internalformat)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glTextureView(internalformat %s not compatible with origtexture %s)",
                  _mesa_lookup_enum_by_nr(internalformat),
                  _mesa_lookup_enum_by_nr(origTexObj->Image[0][0]->InternalFormat));
      return;
   }

   texFormat = _mesa_choose_texture_format(ctx, texObj, target, 0,
                                           internalformat, GL_NONE, GL_NONE);
   assert(texFormat != MESA_FORMAT_NONE);
   if (texFormat == MESA_FORMAT_NONE) return;

   newViewNumLevels = MIN2(numlevels, origTexObj->NumLevels - minlevel);
   newViewNumLayers = MIN2(numlayers, origTexObj->NumLayers - minlayer);

   faceTarget = origTexObj->Target;
   if (faceTarget == GL_TEXTURE_CUBE_MAP)
      faceTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X + minlayer;

   /* Get a reference to what will become this View's base level */
   origTexImage = _mesa_select_tex_image(ctx, origTexObj,
                                         faceTarget, minlevel);
   width = origTexImage->Width;
   height = origTexImage->Height;
   depth = origTexImage->Depth;

   /* Adjust width, height, depth to be appropriate for new target */
   switch (target) {
   case GL_TEXTURE_1D:
      height = 1;
      break;

   case GL_TEXTURE_3D:
      break;

   case GL_TEXTURE_1D_ARRAY:
      height = (GLsizei) newViewNumLayers;
      break;

   case GL_TEXTURE_2D:
   case GL_TEXTURE_2D_MULTISAMPLE:
   case GL_TEXTURE_RECTANGLE:
   case GL_TEXTURE_CUBE_MAP:
      depth = 1;
      break;

   case GL_TEXTURE_2D_ARRAY:
   case GL_TEXTURE_CUBE_MAP_ARRAY:
   case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
      depth = newViewNumLayers;
      break;
   }

   /* If the dimensions of the original texture are larger than the maximum
    * supported dimensions of the new target, the error INVALID_OPERATION is
    * generated. For example, if the original texture has a TEXTURE_2D_ARRAY
    * target and its width is greater than MAX_CUBE_MAP_TEXTURE_SIZE, an error
    * will be generated if TextureView is called to create a TEXTURE_CUBE_MAP
    * view.
    */
   dimensionsOK = _mesa_legal_texture_dimensions(ctx, target, 0,
                                                 width, height, depth, 0);
   if (!dimensionsOK) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glTextureView(invalid width or height or depth)");
      return;
   }

   sizeOK = ctx->Driver.TestProxyTexImage(ctx, target, 0, texFormat,
                                          width, height, depth, 0);
   if (!sizeOK) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glTextureView(invalid texture size)");
      return;
   }

   /* If <target> is TEXTURE_1D, TEXTURE_2D, TEXTURE_3D, TEXTURE_RECTANGLE,
    * or TEXTURE_2D_MULTISAMPLE and <numlayers> does not equal 1, the error
    * INVALID_VALUE is generated.
    */
   switch (target) {
   case GL_TEXTURE_1D:
   case GL_TEXTURE_2D:
   case GL_TEXTURE_3D:
   case GL_TEXTURE_RECTANGLE:
   case GL_TEXTURE_2D_MULTISAMPLE:
      if (numlayers != 1) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glTextureView(numlayers %d != 1)", numlayers);
         return;
      }
      break;

   case GL_TEXTURE_CUBE_MAP:
      /* If the new texture's target is TEXTURE_CUBE_MAP, the clamped <numlayers>
       * must be equal to 6.
       */
      if (newViewNumLayers != 6) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glTextureView(clamped numlayers %d != 6)",
                     newViewNumLayers);
         return;
      }
      break;

   case GL_TEXTURE_CUBE_MAP_ARRAY:
      /* If the new texture's target is TEXTURE_CUBE_MAP_ARRAY,
       * then <numlayers> counts layer-faces rather than layers,
       * and the clamped <numlayers> must be a multiple of 6.
       * Otherwise, the error INVALID_VALUE is generated.
       */
      if ((newViewNumLayers % 6) != 0) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glTextureView(clamped numlayers %d is not a multiple of 6)",
                     newViewNumLayers);
         return;
      }
      break;
   }

   /* If the new texture's target is TEXTURE_CUBE_MAP or
    * TEXTURE_CUBE_MAP_ARRAY, the width and height of the original texture's
    * levels must be equal otherwise the error INVALID_OPERATION is generated.
    */
   if ((target == GL_TEXTURE_CUBE_MAP || target == GL_TEXTURE_CUBE_MAP_ARRAY) &&
       (origTexImage->Width != origTexImage->Height)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glTextureView(origtexture width (%d) != height (%d))",
                  origTexImage->Width, origTexImage->Height);
      return;
   }

   /* When the original texture's target is TEXTURE_CUBE_MAP, the layer
    * parameters are interpreted in the same order as if it were a
    * TEXTURE_CUBE_MAP_ARRAY with 6 layer-faces.
    */

   /* If the internal format does not exactly match the internal format of the
    * original texture, the contents of the memory are reinterpreted in the
    * same manner as for image bindings described in
    * section 3.9.20 (Texture Image Loads and Stores).
    */

   /* TEXTURE_BASE_LEVEL and TEXTURE_MAX_LEVEL are interpreted
    * relative to the view and not relative to the original data store.
    */

   if (!initialize_texture_fields(ctx, target, texObj, newViewNumLevels,
                                  width, height, depth,
                                  internalformat, texFormat)) {
      return; /* Already recorded error */
   }

   texObj->MinLevel = newViewMinLevel;
   texObj->MinLayer = newViewMinLayer;
   texObj->NumLevels = newViewNumLevels;
   texObj->NumLayers = newViewNumLayers;
   texObj->Immutable = GL_TRUE;
   texObj->ImmutableLevels = origTexObj->ImmutableLevels;
   texObj->Target = target;

   if (ctx->Driver.TextureView != NULL && !ctx->Driver.TextureView(ctx, texObj, origTexObj)) {
      return; /* driver recorded error */
   }
}

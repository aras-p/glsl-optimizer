/*
 * Copyright 2013 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
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
 *    Francisco Jerez <currojerez@riseup.net>
 */

#include <assert.h>

#include "shaderimage.h"
#include "mtypes.h"
#include "formats.h"
#include "errors.h"
#include "context.h"
#include "texobj.h"
#include "teximage.h"

/*
 * Define endian-invariant aliases for some mesa formats that are
 * defined in terms of their channel layout from LSB to MSB in a
 * 32-bit word.  The actual byte offsets matter here because the user
 * is allowed to bit-cast one format into another and get predictable
 * results.
 */
#ifdef MESA_BIG_ENDIAN
# define MESA_FORMAT_RGBA_8 MESA_FORMAT_RGBA8888
# define MESA_FORMAT_RG_16 MESA_FORMAT_RG1616
# define MESA_FORMAT_RG_8 MESA_FORMAT_RG88
# define MESA_FORMAT_SIGNED_RGBA_8 MESA_FORMAT_SIGNED_RGBA8888
# define MESA_FORMAT_SIGNED_RG_16 MESA_FORMAT_SIGNED_RG1616
# define MESA_FORMAT_SIGNED_RG_8 MESA_FORMAT_SIGNED_RG88
#else
# define MESA_FORMAT_RGBA_8 MESA_FORMAT_RGBA8888_REV
# define MESA_FORMAT_RG_16 MESA_FORMAT_GR1616
# define MESA_FORMAT_RG_8 MESA_FORMAT_GR88
# define MESA_FORMAT_SIGNED_RGBA_8 MESA_FORMAT_SIGNED_RGBA8888_REV
# define MESA_FORMAT_SIGNED_RG_16 MESA_FORMAT_SIGNED_GR1616
# define MESA_FORMAT_SIGNED_RG_8 MESA_FORMAT_SIGNED_RG88_REV
#endif

static gl_format
get_image_format(GLenum format)
{
   switch (format) {
   case GL_RGBA32F:
      return MESA_FORMAT_RGBA_FLOAT32;

   case GL_RGBA16F:
      return MESA_FORMAT_RGBA_FLOAT16;

   case GL_RG32F:
      return MESA_FORMAT_RG_FLOAT32;

   case GL_RG16F:
      return MESA_FORMAT_RG_FLOAT16;

   case GL_R11F_G11F_B10F:
      return MESA_FORMAT_R11_G11_B10_FLOAT;

   case GL_R32F:
      return MESA_FORMAT_R_FLOAT32;

   case GL_R16F:
      return MESA_FORMAT_R_FLOAT16;

   case GL_RGBA32UI:
      return MESA_FORMAT_RGBA_UINT32;

   case GL_RGBA16UI:
      return MESA_FORMAT_RGBA_UINT16;

   case GL_RGB10_A2UI:
      return MESA_FORMAT_ABGR2101010_UINT;

   case GL_RGBA8UI:
      return MESA_FORMAT_RGBA_UINT8;

   case GL_RG32UI:
      return MESA_FORMAT_RG_UINT32;

   case GL_RG16UI:
      return MESA_FORMAT_RG_UINT16;

   case GL_RG8UI:
      return MESA_FORMAT_RG_UINT8;

   case GL_R32UI:
      return MESA_FORMAT_R_UINT32;

   case GL_R16UI:
      return MESA_FORMAT_R_UINT16;

   case GL_R8UI:
      return MESA_FORMAT_R_UINT8;

   case GL_RGBA32I:
      return MESA_FORMAT_RGBA_INT32;

   case GL_RGBA16I:
      return MESA_FORMAT_RGBA_INT16;

   case GL_RGBA8I:
      return MESA_FORMAT_RGBA_INT8;

   case GL_RG32I:
      return MESA_FORMAT_RG_INT32;

   case GL_RG16I:
      return MESA_FORMAT_RG_INT16;

   case GL_RG8I:
      return MESA_FORMAT_RG_INT8;

   case GL_R32I:
      return MESA_FORMAT_R_INT32;

   case GL_R16I:
      return MESA_FORMAT_R_INT16;

   case GL_R8I:
      return MESA_FORMAT_R_INT8;

   case GL_RGBA16:
      return MESA_FORMAT_RGBA_16;

   case GL_RGB10_A2:
      return MESA_FORMAT_ABGR2101010;

   case GL_RGBA8:
      return MESA_FORMAT_RGBA_8;

   case GL_RG16:
      return MESA_FORMAT_RG_16;

   case GL_RG8:
      return MESA_FORMAT_RG_8;

   case GL_R16:
      return MESA_FORMAT_R16;

   case GL_R8:
      return MESA_FORMAT_R8;

   case GL_RGBA16_SNORM:
      return MESA_FORMAT_SIGNED_RGBA_16;

   case GL_RGBA8_SNORM:
      return MESA_FORMAT_SIGNED_RGBA_8;

   case GL_RG16_SNORM:
      return MESA_FORMAT_SIGNED_RG_16;

   case GL_RG8_SNORM:
      return MESA_FORMAT_SIGNED_RG_8;

   case GL_R16_SNORM:
      return MESA_FORMAT_SIGNED_R16;

   case GL_R8_SNORM:
      return MESA_FORMAT_SIGNED_R8;

   default:
      return MESA_FORMAT_NONE;
   }
}

enum image_format_class
{
   /** Not a valid image format. */
   IMAGE_FORMAT_CLASS_NONE = 0,

   /** Classes of image formats you can cast into each other. */
   /** \{ */
   IMAGE_FORMAT_CLASS_1X8,
   IMAGE_FORMAT_CLASS_1X16,
   IMAGE_FORMAT_CLASS_1X32,
   IMAGE_FORMAT_CLASS_2X8,
   IMAGE_FORMAT_CLASS_2X16,
   IMAGE_FORMAT_CLASS_2X32,
   IMAGE_FORMAT_CLASS_10_11_11,
   IMAGE_FORMAT_CLASS_4X8,
   IMAGE_FORMAT_CLASS_4X16,
   IMAGE_FORMAT_CLASS_4X32,
   IMAGE_FORMAT_CLASS_2_10_10_10
   /** \} */
};

static enum image_format_class
get_image_format_class(gl_format format)
{
   switch (format) {
   case MESA_FORMAT_RGBA_FLOAT32:
      return IMAGE_FORMAT_CLASS_4X32;

   case MESA_FORMAT_RGBA_FLOAT16:
      return IMAGE_FORMAT_CLASS_4X16;

   case MESA_FORMAT_RG_FLOAT32:
      return IMAGE_FORMAT_CLASS_2X32;

   case MESA_FORMAT_RG_FLOAT16:
      return IMAGE_FORMAT_CLASS_2X16;

   case MESA_FORMAT_R11_G11_B10_FLOAT:
      return IMAGE_FORMAT_CLASS_10_11_11;

   case MESA_FORMAT_R_FLOAT32:
      return IMAGE_FORMAT_CLASS_1X32;

   case MESA_FORMAT_R_FLOAT16:
      return IMAGE_FORMAT_CLASS_1X16;

   case MESA_FORMAT_RGBA_UINT32:
      return IMAGE_FORMAT_CLASS_4X32;

   case MESA_FORMAT_RGBA_UINT16:
      return IMAGE_FORMAT_CLASS_4X16;

   case MESA_FORMAT_ABGR2101010_UINT:
      return IMAGE_FORMAT_CLASS_2_10_10_10;

   case MESA_FORMAT_RGBA_UINT8:
      return IMAGE_FORMAT_CLASS_4X8;

   case MESA_FORMAT_RG_UINT32:
      return IMAGE_FORMAT_CLASS_2X32;

   case MESA_FORMAT_RG_UINT16:
      return IMAGE_FORMAT_CLASS_2X16;

   case MESA_FORMAT_RG_UINT8:
      return IMAGE_FORMAT_CLASS_2X8;

   case MESA_FORMAT_R_UINT32:
      return IMAGE_FORMAT_CLASS_1X32;

   case MESA_FORMAT_R_UINT16:
      return IMAGE_FORMAT_CLASS_1X16;

   case MESA_FORMAT_R_UINT8:
      return IMAGE_FORMAT_CLASS_1X8;

   case MESA_FORMAT_RGBA_INT32:
      return IMAGE_FORMAT_CLASS_4X32;

   case MESA_FORMAT_RGBA_INT16:
      return IMAGE_FORMAT_CLASS_4X16;

   case MESA_FORMAT_RGBA_INT8:
      return IMAGE_FORMAT_CLASS_4X8;

   case MESA_FORMAT_RG_INT32:
      return IMAGE_FORMAT_CLASS_2X32;

   case MESA_FORMAT_RG_INT16:
      return IMAGE_FORMAT_CLASS_2X16;

   case MESA_FORMAT_RG_INT8:
      return IMAGE_FORMAT_CLASS_2X8;

   case MESA_FORMAT_R_INT32:
      return IMAGE_FORMAT_CLASS_1X32;

   case MESA_FORMAT_R_INT16:
      return IMAGE_FORMAT_CLASS_1X16;

   case MESA_FORMAT_R_INT8:
      return IMAGE_FORMAT_CLASS_1X8;

   case MESA_FORMAT_RGBA_16:
      return IMAGE_FORMAT_CLASS_4X16;

   case MESA_FORMAT_ABGR2101010:
      return IMAGE_FORMAT_CLASS_2_10_10_10;

   case MESA_FORMAT_RGBA_8:
      return IMAGE_FORMAT_CLASS_4X8;

   case MESA_FORMAT_RG_16:
      return IMAGE_FORMAT_CLASS_2X16;

   case MESA_FORMAT_RG_8:
      return IMAGE_FORMAT_CLASS_2X8;

   case MESA_FORMAT_R16:
      return IMAGE_FORMAT_CLASS_1X16;

   case MESA_FORMAT_R8:
      return IMAGE_FORMAT_CLASS_1X8;

   case MESA_FORMAT_SIGNED_RGBA_16:
      return IMAGE_FORMAT_CLASS_4X16;

   case MESA_FORMAT_SIGNED_RGBA_8:
      return IMAGE_FORMAT_CLASS_4X8;

   case MESA_FORMAT_SIGNED_RG_16:
      return IMAGE_FORMAT_CLASS_2X16;

   case MESA_FORMAT_SIGNED_RG_8:
      return IMAGE_FORMAT_CLASS_2X8;

   case MESA_FORMAT_SIGNED_R16:
      return IMAGE_FORMAT_CLASS_1X16;

   case MESA_FORMAT_SIGNED_R8:
      return IMAGE_FORMAT_CLASS_1X8;

   default:
      return IMAGE_FORMAT_CLASS_NONE;
   }
}

static GLboolean
validate_image_unit(struct gl_context *ctx, struct gl_image_unit *u)
{
   struct gl_texture_object *t = u->TexObj;
   struct gl_texture_image *img;

   if (!t || u->Level < t->BaseLevel ||
       u->Level > t->_MaxLevel)
      return GL_FALSE;

   _mesa_test_texobj_completeness(ctx, t);

   if ((u->Level == t->BaseLevel && !t->_BaseComplete) ||
       (u->Level != t->BaseLevel && !t->_MipmapComplete))
      return GL_FALSE;

   if (_mesa_tex_target_is_layered(t->Target) &&
       u->Layer >= _mesa_get_texture_layers(t, u->Level))
      return GL_FALSE;

   if (t->Target == GL_TEXTURE_CUBE_MAP)
      img = t->Image[u->Layer][u->Level];
   else
      img = t->Image[0][u->Level];

   if (!img || img->Border ||
       get_image_format_class(img->TexFormat) == IMAGE_FORMAT_CLASS_NONE ||
       img->NumSamples > ctx->Const.MaxImageSamples)
      return GL_FALSE;

   switch (t->ImageFormatCompatibilityType) {
   case GL_IMAGE_FORMAT_COMPATIBILITY_BY_SIZE:
      if (_mesa_get_format_bytes(img->TexFormat) !=
          _mesa_get_format_bytes(u->_ActualFormat))
         return GL_FALSE;
      break;

   case GL_IMAGE_FORMAT_COMPATIBILITY_BY_CLASS:
      if (get_image_format_class(img->TexFormat) !=
          get_image_format_class(u->_ActualFormat))
         return GL_FALSE;
      break;

   default:
      assert(!"Unexpected image format compatibility type");
   }

   return GL_TRUE;
}

void
_mesa_validate_image_units(struct gl_context *ctx)
{
   int i;

   for (i = 0; i < ctx->Const.MaxImageUnits; ++i) {
      struct gl_image_unit *u = &ctx->ImageUnits[i];
      u->_Valid = validate_image_unit(ctx, u);
   }
}

static GLboolean
validate_bind_image_texture(struct gl_context *ctx, GLuint unit,
                            GLuint texture, GLint level, GLboolean layered,
                            GLint layer, GLenum access, GLenum format)
{
   assert(ctx->Const.MaxImageUnits <= MAX_IMAGE_UNITS);

   if (unit >= ctx->Const.MaxImageUnits) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindImageTexture(unit)");
      return GL_FALSE;
   }

   if (level < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindImageTexture(level)");
      return GL_FALSE;
   }

   if (layer < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindImageTexture(layer)");
      return GL_FALSE;
   }

   if (access != GL_READ_ONLY &&
       access != GL_WRITE_ONLY &&
       access != GL_READ_WRITE) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindImageTexture(access)");
      return GL_FALSE;
   }

   if (!get_image_format(format)) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindImageTexture(format)");
      return GL_FALSE;
   }

   return GL_TRUE;
}

void GLAPIENTRY
_mesa_BindImageTexture(GLuint unit, GLuint texture, GLint level,
                       GLboolean layered, GLint layer, GLenum access,
                       GLenum format)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_object *t = NULL;
   struct gl_image_unit *u;

   if (!validate_bind_image_texture(ctx, unit, texture, level,
                                    layered, layer, access, format))
      return;

   u = &ctx->ImageUnits[unit];

   FLUSH_VERTICES(ctx, 0);
   ctx->NewDriverState |= ctx->DriverFlags.NewImageUnits;

   if (texture) {
      t = _mesa_lookup_texture(ctx, texture);
      if (!t) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glBindImageTexture(texture)");
         return;
      }

      _mesa_reference_texobj(&u->TexObj, t);
      u->Level = level;
      u->Access = access;
      u->Format = format;
      u->_ActualFormat = get_image_format(format);

      if (_mesa_tex_target_is_layered(t->Target)) {
         u->Layered = layered;
         u->Layer = (layered ? 0 : layer);
      } else {
         u->Layered = GL_FALSE;
         u->Layer = 0;
      }

   } else {
      _mesa_reference_texobj(&u->TexObj, NULL);
   }

   u->_Valid = validate_image_unit(ctx, u);

   if (ctx->Driver.BindImageTexture)
      ctx->Driver.BindImageTexture(ctx, u, t, level, layered,
                                   layer, access, format);
}

void GLAPIENTRY
_mesa_MemoryBarrier(GLbitfield barriers)
{
   GET_CURRENT_CONTEXT(ctx);

   if (ctx->Driver.MemoryBarrier)
      ctx->Driver.MemoryBarrier(ctx, barriers);
}

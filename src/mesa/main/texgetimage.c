/*
 * Mesa 3-D graphics library
 * Version:  7.7
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (c) 2009 VMware, Inc.
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
 * Code for glGetTexImage() and glGetCompressedTexImage().
 */


#include "glheader.h"
#include "bufferobj.h"
#include "enums.h"
#include "context.h"
#include "formats.h"
#include "image.h"
#include "texcompress.h"
#include "texgetimage.h"
#include "teximage.h"
#include "texstate.h"



#if FEATURE_EXT_texture_sRGB

/**
 * Test if given texture image is an sRGB format.
 */
static GLboolean
is_srgb_teximage(const struct gl_texture_image *texImage)
{
   switch (texImage->TexFormat) {
   case MESA_FORMAT_SRGB8:
   case MESA_FORMAT_SRGBA8:
   case MESA_FORMAT_SARGB8:
   case MESA_FORMAT_SL8:
   case MESA_FORMAT_SLA8:
   case MESA_FORMAT_SRGB_DXT1:
   case MESA_FORMAT_SRGBA_DXT1:
   case MESA_FORMAT_SRGBA_DXT3:
   case MESA_FORMAT_SRGBA_DXT5:
      return GL_TRUE;
   default:
      return GL_FALSE;
   }
}


/**
 * Convert a float value from linear space to a
 * non-linear sRGB value in [0, 255].
 * Not terribly efficient.
 */
static INLINE GLfloat
linear_to_nonlinear(GLfloat cl)
{
   /* can't have values outside [0, 1] */
   GLfloat cs;
   if (cl < 0.0031308f) {
      cs = 12.92f * cl;
   }
   else {
      cs = (GLfloat)(1.055 * _mesa_pow(cl, 0.41666) - 0.055);
   }
   return cs;
}

#endif /* FEATURE_EXT_texture_sRGB */


/**
 * Can the given type represent negative values?
 */
static INLINE GLboolean
type_with_negative_values(GLenum type)
{
   switch (type) {
   case GL_BYTE:
   case GL_SHORT:
   case GL_INT:
   case GL_FLOAT:
   case GL_HALF_FLOAT_ARB:
      return GL_TRUE;
   default:
      return GL_FALSE;
   }
}


/**
 * This is the software fallback for Driver.GetTexImage().
 * All error checking will have been done before this routine is called.
 */
void
_mesa_get_teximage(GLcontext *ctx, GLenum target, GLint level,
                   GLenum format, GLenum type, GLvoid *pixels,
                   struct gl_texture_object *texObj,
                   struct gl_texture_image *texImage)
{
   const GLuint dimensions = (target == GL_TEXTURE_3D) ? 3 : 2;

   if (_mesa_is_bufferobj(ctx->Pack.BufferObj)) {
      /* Packing texture image into a PBO.
       * Map the (potentially) VRAM-based buffer into our process space so
       * we can write into it with the code below.
       * A hardware driver might use a sophisticated blit to move the
       * texture data to the PBO if the PBO is in VRAM along with the texture.
       */
      GLubyte *buf = (GLubyte *)
         ctx->Driver.MapBuffer(ctx, GL_PIXEL_PACK_BUFFER_EXT,
                               GL_WRITE_ONLY_ARB, ctx->Pack.BufferObj);
      if (!buf) {
         /* out of memory or other unexpected error */
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGetTexImage(map PBO failed)");
         return;

      }
      /* <pixels> was an offset into the PBO.
       * Now make it a real, client-side pointer inside the mapped region.
       */
      pixels = ADD_POINTERS(buf, pixels);
   }
   else if (!pixels) {
      /* not an error */
      return;
   }

   {
      const GLint width = texImage->Width;
      const GLint height = texImage->Height;
      const GLint depth = texImage->Depth;
      GLint img, row;
      for (img = 0; img < depth; img++) {
         for (row = 0; row < height; row++) {
            /* compute destination address in client memory */
            GLvoid *dest = _mesa_image_address( dimensions, &ctx->Pack, pixels,
                                                width, height, format, type,
                                                img, row, 0);
            assert(dest);

            if (format == GL_COLOR_INDEX) {
               GLuint indexRow[MAX_WIDTH];
               GLint col;
               GLuint indexBits = _mesa_get_format_bits(texImage->TexFormat, GL_TEXTURE_INDEX_SIZE_EXT);
               /* Can't use FetchTexel here because that returns RGBA */
               if (indexBits == 8) {
                  const GLubyte *src = (const GLubyte *) texImage->Data;
                  src += width * (img * texImage->Height + row);
                  for (col = 0; col < width; col++) {
                     indexRow[col] = src[col];
                  }
               }
               else if (indexBits == 16) {
                  const GLushort *src = (const GLushort *) texImage->Data;
                  src += width * (img * texImage->Height + row);
                  for (col = 0; col < width; col++) {
                     indexRow[col] = src[col];
                  }
               }
               else {
                  _mesa_problem(ctx,
                                "Color index problem in _mesa_GetTexImage");
               }
               _mesa_pack_index_span(ctx, width, type, dest,
                                     indexRow, &ctx->Pack,
                                     0 /* no image transfer */);
            }
            else if (format == GL_DEPTH_COMPONENT) {
               GLfloat depthRow[MAX_WIDTH];
               GLint col;
               for (col = 0; col < width; col++) {
                  (*texImage->FetchTexelf)(texImage, col, row, img,
                                           depthRow + col);
               }
               _mesa_pack_depth_span(ctx, width, dest, type,
                                     depthRow, &ctx->Pack);
            }
            else if (format == GL_DEPTH_STENCIL_EXT) {
               /* XXX Note: we're bypassing texImage->FetchTexel()! */
               const GLuint *src = (const GLuint *) texImage->Data;
               src += width * row + width * height * img;
               _mesa_memcpy(dest, src, width * sizeof(GLuint));
               if (ctx->Pack.SwapBytes) {
                  _mesa_swap4((GLuint *) dest, width);
               }
            }
            else if (format == GL_YCBCR_MESA) {
               /* No pixel transfer */
               const GLint rowstride = texImage->RowStride;
               MEMCPY(dest,
                      (const GLushort *) texImage->Data + row * rowstride,
                      width * sizeof(GLushort));
               /* check for byte swapping */
               if ((texImage->TexFormat == MESA_FORMAT_YCBCR
                    && type == GL_UNSIGNED_SHORT_8_8_REV_MESA) ||
                   (texImage->TexFormat == MESA_FORMAT_YCBCR_REV
                    && type == GL_UNSIGNED_SHORT_8_8_MESA)) {
                  if (!ctx->Pack.SwapBytes)
                     _mesa_swap2((GLushort *) dest, width);
               }
               else if (ctx->Pack.SwapBytes) {
                  _mesa_swap2((GLushort *) dest, width);
               }
            }
#if FEATURE_EXT_texture_sRGB
            else if (is_srgb_teximage(texImage)) {
               /* special case this since need to backconvert values */
               /* convert row to RGBA format */
               GLfloat rgba[MAX_WIDTH][4];
               GLint col;
               GLbitfield transferOps = 0x0;

               for (col = 0; col < width; col++) {
                  (*texImage->FetchTexelf)(texImage, col, row, img, rgba[col]);
                  if (texImage->_BaseFormat == GL_LUMINANCE) {
                     rgba[col][RCOMP] = linear_to_nonlinear(rgba[col][RCOMP]);
                     rgba[col][GCOMP] = 0.0;
                     rgba[col][BCOMP] = 0.0;
                  }
                  else if (texImage->_BaseFormat == GL_LUMINANCE_ALPHA) {
                     rgba[col][RCOMP] = linear_to_nonlinear(rgba[col][RCOMP]);
                     rgba[col][GCOMP] = 0.0;
                     rgba[col][BCOMP] = 0.0;
                  }
                  else if (texImage->_BaseFormat == GL_RGB ||
                     texImage->_BaseFormat == GL_RGBA) {
                     rgba[col][RCOMP] = linear_to_nonlinear(rgba[col][RCOMP]);
                     rgba[col][GCOMP] = linear_to_nonlinear(rgba[col][GCOMP]);
                     rgba[col][BCOMP] = linear_to_nonlinear(rgba[col][BCOMP]);
                  }
               }
               _mesa_pack_rgba_span_float(ctx, width, (GLfloat (*)[4]) rgba,
                                          format, type, dest,
                                          &ctx->Pack, transferOps);
            }
#endif /* FEATURE_EXT_texture_sRGB */
            else {
               /* general case:  convert row to RGBA format */
               GLfloat rgba[MAX_WIDTH][4];
               GLint col;
               GLbitfield transferOps = 0x0;
               GLenum dataType =
                  _mesa_get_format_datatype(texImage->TexFormat);

               /* clamp does not apply to GetTexImage (final conversion)?
                * Looks like we need clamp though when going from format
                * containing negative values to unsigned format.
                */
               if (format == GL_LUMINANCE || format == GL_LUMINANCE_ALPHA)
                  transferOps |= IMAGE_CLAMP_BIT;
               else if (!type_with_negative_values(type) &&
                        (dataType == GL_FLOAT ||
                         dataType == GL_SIGNED_NORMALIZED))
                  transferOps |= IMAGE_CLAMP_BIT;

               for (col = 0; col < width; col++) {
                  (*texImage->FetchTexelf)(texImage, col, row, img, rgba[col]);
                  if (texImage->_BaseFormat == GL_ALPHA) {
                     rgba[col][RCOMP] = 0.0;
                     rgba[col][GCOMP] = 0.0;
                     rgba[col][BCOMP] = 0.0;
                  }
                  else if (texImage->_BaseFormat == GL_LUMINANCE) {
                     rgba[col][GCOMP] = 0.0;
                     rgba[col][BCOMP] = 0.0;
                     rgba[col][ACOMP] = 1.0;
                  }
                  else if (texImage->_BaseFormat == GL_LUMINANCE_ALPHA) {
                     rgba[col][GCOMP] = 0.0;
                     rgba[col][BCOMP] = 0.0;
                  }
                  else if (texImage->_BaseFormat == GL_INTENSITY) {
                     rgba[col][GCOMP] = 0.0;
                     rgba[col][BCOMP] = 0.0;
                     rgba[col][ACOMP] = 1.0;
                  }
               }
               _mesa_pack_rgba_span_float(ctx, width, (GLfloat (*)[4]) rgba,
                                          format, type, dest,
                                          &ctx->Pack, transferOps);
            } /* format */
         } /* row */
      } /* img */
   }

   if (_mesa_is_bufferobj(ctx->Pack.BufferObj)) {
      ctx->Driver.UnmapBuffer(ctx, GL_PIXEL_PACK_BUFFER_EXT,
                              ctx->Pack.BufferObj);
   }
}



/**
 * This is the software fallback for Driver.GetCompressedTexImage().
 * All error checking will have been done before this routine is called.
 */
void
_mesa_get_compressed_teximage(GLcontext *ctx, GLenum target, GLint level,
                              GLvoid *img,
                              struct gl_texture_object *texObj,
                              struct gl_texture_image *texImage)
{
   const GLuint size = _mesa_format_image_size(texImage->TexFormat,
                                               texImage->Width,
                                               texImage->Height,
                                               texImage->Depth);

   if (_mesa_is_bufferobj(ctx->Pack.BufferObj)) {
      /* pack texture image into a PBO */
      GLubyte *buf = (GLubyte *)
         ctx->Driver.MapBuffer(ctx, GL_PIXEL_PACK_BUFFER_EXT,
                               GL_WRITE_ONLY_ARB, ctx->Pack.BufferObj);
      if (!buf) {
         /* out of memory or other unexpected error */
         _mesa_error(ctx, GL_OUT_OF_MEMORY,
                     "glGetCompresssedTexImage(map PBO failed)");
         return;
      }
      img = ADD_POINTERS(buf, img);
   }
   else if (!img) {
      /* not an error */
      return;
   }

   /* just memcpy, no pixelstore or pixel transfer */
   _mesa_memcpy(img, texImage->Data, size);

   if (_mesa_is_bufferobj(ctx->Pack.BufferObj)) {
      ctx->Driver.UnmapBuffer(ctx, GL_PIXEL_PACK_BUFFER_EXT,
                              ctx->Pack.BufferObj);
   }
}



/**
 * Do error checking for a glGetTexImage() call.
 * \return GL_TRUE if any error, GL_FALSE if no errors.
 */
static GLboolean
getteximage_error_check(GLcontext *ctx, GLenum target, GLint level,
                        GLenum format, GLenum type, GLvoid *pixels )
{
   const struct gl_texture_unit *texUnit;
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   const GLuint maxLevels = _mesa_max_texture_levels(ctx, target);
   GLenum baseFormat;

   if (maxLevels == 0) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexImage(target=0x%x)", target);
      return GL_TRUE;
   }

   if (level < 0 || level >= maxLevels) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glGetTexImage(level)" );
      return GL_TRUE;
   }

   if (_mesa_sizeof_packed_type(type) <= 0) {
      _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexImage(type)" );
      return GL_TRUE;
   }

   if (_mesa_components_in_format(format) <= 0 ||
       format == GL_STENCIL_INDEX) {
      _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexImage(format)" );
      return GL_TRUE;
   }

   if (!ctx->Extensions.EXT_paletted_texture && _mesa_is_index_format(format)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexImage(format)");
      return GL_TRUE;
   }

   if (!ctx->Extensions.ARB_depth_texture && _mesa_is_depth_format(format)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexImage(format)");
      return GL_TRUE;
   }

   if (!ctx->Extensions.MESA_ycbcr_texture && _mesa_is_ycbcr_format(format)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexImage(format)");
      return GL_TRUE;
   }

   if (!ctx->Extensions.EXT_packed_depth_stencil
       && _mesa_is_depthstencil_format(format)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexImage(format)");
      return GL_TRUE;
   }

   if (!ctx->Extensions.ATI_envmap_bumpmap
       && _mesa_is_dudv_format(format)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexImage(format)");
      return GL_TRUE;
   }

   texUnit = _mesa_get_current_tex_unit(ctx);
   texObj = _mesa_select_tex_object(ctx, texUnit, target);

   if (!texObj || _mesa_is_proxy_texture(target)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexImage(target)");
      return GL_TRUE;
   }

   texImage = _mesa_select_tex_image(ctx, texObj, target, level);
   if (!texImage) {
      /* out of memory */
      return GL_TRUE;
   }

   baseFormat = _mesa_get_format_base_format(texImage->TexFormat);
      
   /* Make sure the requested image format is compatible with the
    * texture's format.  Note that a color index texture can be converted
    * to RGBA so that combo is allowed.
    */
   if (_mesa_is_color_format(format)
       && !_mesa_is_color_format(baseFormat)
       && !_mesa_is_index_format(baseFormat)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetTexImage(format mismatch)");
      return GL_TRUE;
   }
   else if (_mesa_is_index_format(format)
            && !_mesa_is_index_format(baseFormat)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetTexImage(format mismatch)");
      return GL_TRUE;
   }
   else if (_mesa_is_depth_format(format)
            && !_mesa_is_depth_format(baseFormat)
            && !_mesa_is_depthstencil_format(baseFormat)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetTexImage(format mismatch)");
      return GL_TRUE;
   }
   else if (_mesa_is_ycbcr_format(format)
            && !_mesa_is_ycbcr_format(baseFormat)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetTexImage(format mismatch)");
      return GL_TRUE;
   }
   else if (_mesa_is_depthstencil_format(format)
            && !_mesa_is_depthstencil_format(baseFormat)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetTexImage(format mismatch)");
      return GL_TRUE;
   }
   else if (_mesa_is_dudv_format(format)
            && !_mesa_is_dudv_format(baseFormat)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetTexImage(format mismatch)");
      return GL_TRUE;
   }

   if (_mesa_is_bufferobj(ctx->Pack.BufferObj)) {
      /* packing texture image into a PBO */
      const GLuint dimensions = (target == GL_TEXTURE_3D) ? 3 : 2;
      if (!_mesa_validate_pbo_access(dimensions, &ctx->Pack, texImage->Width,
                                     texImage->Height, texImage->Depth,
                                     format, type, pixels)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glGetTexImage(out of bounds PBO write)");
         return GL_TRUE;
      }

      /* PBO should not be mapped */
      if (_mesa_bufferobj_mapped(ctx->Pack.BufferObj)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glGetTexImage(PBO is mapped)");
         return GL_TRUE;
      }
   }

   return GL_FALSE;
}



/**
 * Get texture image.  Called by glGetTexImage.
 *
 * \param target texture target.
 * \param level image level.
 * \param format pixel data format for returned image.
 * \param type pixel data type for returned image.
 * \param pixels returned pixel data.
 */
void GLAPIENTRY
_mesa_GetTexImage( GLenum target, GLint level, GLenum format,
                   GLenum type, GLvoid *pixels )
{
   const struct gl_texture_unit *texUnit;
   struct gl_texture_object *texObj;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (getteximage_error_check(ctx, target, level, format, type, pixels)) {
      return;
   }

   texUnit = _mesa_get_current_tex_unit(ctx);
   texObj = _mesa_select_tex_object(ctx, texUnit, target);

   if (MESA_VERBOSE & (VERBOSE_API | VERBOSE_TEXTURE)) {
      struct gl_texture_image *texImage =
         _mesa_select_tex_image(ctx, texObj, target, level);
      _mesa_debug(ctx, "glGetTexImage(tex %u) format = %s, w=%d, h=%d,"
                  " dstFmt=0x%x, dstType=0x%x\n",
                  texObj->Name,
                  _mesa_get_format_name(texImage->TexFormat),
                  texImage->Width, texImage->Height,
                  format, type);
   }

   _mesa_lock_texture(ctx, texObj);
   {
      struct gl_texture_image *texImage =
         _mesa_select_tex_image(ctx, texObj, target, level);

      /* typically, this will call _mesa_get_teximage() */
      ctx->Driver.GetTexImage(ctx, target, level, format, type, pixels,
                              texObj, texImage);
   }
   _mesa_unlock_texture(ctx, texObj);
}



/**
 * Do error checking for a glGetCompressedTexImage() call.
 * \return GL_TRUE if any error, GL_FALSE if no errors.
 */
static GLboolean
getcompressedteximage_error_check(GLcontext *ctx, GLenum target, GLint level,
                                  GLvoid *img)
{
   const struct gl_texture_unit *texUnit;
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   const GLuint maxLevels = _mesa_max_texture_levels(ctx, target);

   if (maxLevels == 0) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetCompressedTexImage(target=0x%x)",
                  target);
      return GL_TRUE;
   }

   if (level < 0 || level >= maxLevels) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glGetCompressedTexImageARB(bad level = %d)", level);
      return GL_TRUE;
   }

   if (_mesa_is_proxy_texture(target)) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glGetCompressedTexImageARB(bad target = %s)",
                  _mesa_lookup_enum_by_nr(target));
      return GL_TRUE;
   }

   texUnit = _mesa_get_current_tex_unit(ctx);
   texObj = _mesa_select_tex_object(ctx, texUnit, target);

   if (!texObj) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetCompressedTexImageARB(target)");
      return GL_TRUE;
   }

   texImage = _mesa_select_tex_image(ctx, texObj, target, level);

   if (!texImage) {
      /* probably invalid mipmap level */
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glGetCompressedTexImageARB(level)");
      return GL_TRUE;
   }

   if (!_mesa_is_format_compressed(texImage->TexFormat)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glGetCompressedTexImageARB(texture is not compressed)");
      return GL_TRUE;
   }

   if (_mesa_is_bufferobj(ctx->Pack.BufferObj)) {
      GLuint compressedSize;

      /* make sure PBO is not mapped */
      if (_mesa_bufferobj_mapped(ctx->Pack.BufferObj)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glGetCompressedTexImage(PBO is mapped)");
         return GL_TRUE;
      }

      compressedSize = _mesa_format_image_size(texImage->TexFormat,
                                               texImage->Width,
                                               texImage->Height,
                                               texImage->Depth);

      /* do bounds checking on PBO write */
      if ((const GLubyte *) img + compressedSize >
          (const GLubyte *) ctx->Pack.BufferObj->Size) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glGetCompressedTexImage(out of bounds PBO write)");
         return GL_TRUE;
      }
   }

   return GL_FALSE;
}


void GLAPIENTRY
_mesa_GetCompressedTexImageARB(GLenum target, GLint level, GLvoid *img)
{
   const struct gl_texture_unit *texUnit;
   struct gl_texture_object *texObj;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (getcompressedteximage_error_check(ctx, target, level, img)) {
      return;
   }

   texUnit = _mesa_get_current_tex_unit(ctx);
   texObj = _mesa_select_tex_object(ctx, texUnit, target);

   if (MESA_VERBOSE & (VERBOSE_API | VERBOSE_TEXTURE)) {
      struct gl_texture_image *texImage =
         _mesa_select_tex_image(ctx, texObj, target, level);
      _mesa_debug(ctx,
                  "glGetCompressedTexImage(tex %u) format = %s, w=%d, h=%d\n",
                  texObj->Name,
                  _mesa_get_format_name(texImage->TexFormat),
                  texImage->Width, texImage->Height);
   }

   _mesa_lock_texture(ctx, texObj);
   {
      struct gl_texture_image *texImage =
         _mesa_select_tex_image(ctx, texObj, target, level);

      /* this typically calls _mesa_get_compressed_teximage() */
      ctx->Driver.GetCompressedTexImage(ctx, target, level, img,
                                        texObj, texImage);
   }
   _mesa_unlock_texture(ctx, texObj);
}

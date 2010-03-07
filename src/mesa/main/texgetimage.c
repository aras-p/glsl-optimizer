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
#include "texgetimage.h"
#include "teximage.h"



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
 * glGetTexImage for color index pixels.
 */
static void
get_tex_color_index(GLcontext *ctx, GLuint dimensions,
                    GLenum format, GLenum type, GLvoid *pixels,
                    const struct gl_texture_image *texImage)
{
   const GLint width = texImage->Width;
   const GLint height = texImage->Height;
   const GLint depth = texImage->Depth;
   const GLuint indexBits =
      _mesa_get_format_bits(texImage->TexFormat, GL_TEXTURE_INDEX_SIZE_EXT);
   const GLbitfield transferOps = 0x0;
   GLint img, row, col;

   for (img = 0; img < depth; img++) {
      for (row = 0; row < height; row++) {
         GLuint indexRow[MAX_WIDTH] = { 0 };
         void *dest = _mesa_image_address(dimensions, &ctx->Pack, pixels,
                                          width, height, format, type,
                                          img, row, 0);
         assert(dest);

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
            _mesa_problem(ctx, "Color index problem in _mesa_GetTexImage");
         }
         _mesa_pack_index_span(ctx, width, type, dest,
                               indexRow, &ctx->Pack, transferOps);
      }
   }
}


/**
 * glGetTexImage for depth/Z pixels.
 */
static void
get_tex_depth(GLcontext *ctx, GLuint dimensions,
              GLenum format, GLenum type, GLvoid *pixels,
              const struct gl_texture_image *texImage)
{
   const GLint width = texImage->Width;
   const GLint height = texImage->Height;
   const GLint depth = texImage->Depth;
   GLint img, row, col;

   for (img = 0; img < depth; img++) {
      for (row = 0; row < height; row++) {
         GLfloat depthRow[MAX_WIDTH];
         void *dest = _mesa_image_address(dimensions, &ctx->Pack, pixels,
                                          width, height, format, type,
                                          img, row, 0);
         assert(dest);

         for (col = 0; col < width; col++) {
            texImage->FetchTexelf(texImage, col, row, img, depthRow + col);
         }
         _mesa_pack_depth_span(ctx, width, dest, type, depthRow, &ctx->Pack);
      }
   }
}


/**
 * glGetTexImage for depth/stencil pixels.
 */
static void
get_tex_depth_stencil(GLcontext *ctx, GLuint dimensions,
                      GLenum format, GLenum type, GLvoid *pixels,
                      const struct gl_texture_image *texImage)
{
   const GLint width = texImage->Width;
   const GLint height = texImage->Height;
   const GLint depth = texImage->Depth;
   const GLuint *src = (const GLuint *) texImage->Data;
   GLint img, row;

   for (img = 0; img < depth; img++) {
      for (row = 0; row < height; row++) {
         void *dest = _mesa_image_address(dimensions, &ctx->Pack, pixels,
                                          width, height, format, type,
                                          img, row, 0);
         memcpy(dest, src, width * sizeof(GLuint));
         if (ctx->Pack.SwapBytes) {
            _mesa_swap4((GLuint *) dest, width);
         }

         src += width * row + width * height * img;
      }
   }
}


/**
 * glGetTexImage for YCbCr pixels.
 */
static void
get_tex_ycbcr(GLcontext *ctx, GLuint dimensions,
              GLenum format, GLenum type, GLvoid *pixels,
              const struct gl_texture_image *texImage)
{
   const GLint width = texImage->Width;
   const GLint height = texImage->Height;
   const GLint depth = texImage->Depth;
   const GLint rowstride = texImage->RowStride;
   const GLushort *src = (const GLushort *) texImage->Data;
   GLint img, row;

   for (img = 0; img < depth; img++) {
      for (row = 0; row < height; row++) {
         void *dest = _mesa_image_address(dimensions, &ctx->Pack, pixels,
                                          width, height, format, type,
                                          img, row, 0);
         memcpy(dest, src, width * sizeof(GLushort));

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

         src += rowstride;
      }
   }
}


#if FEATURE_EXT_texture_sRGB


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


/**
 * glGetTexImagefor sRGB pixels;
 */
static void
get_tex_srgb(GLcontext *ctx, GLuint dimensions,
             GLenum format, GLenum type, GLvoid *pixels,
             const struct gl_texture_image *texImage)
{
   const GLint width = texImage->Width;
   const GLint height = texImage->Height;
   const GLint depth = texImage->Depth;
   const GLbitfield transferOps = 0x0;
   GLint img, row;

   for (img = 0; img < depth; img++) {
      for (row = 0; row < height; row++) {
         void *dest = _mesa_image_address(dimensions, &ctx->Pack, pixels,
                                          width, height, format, type,
                                          img, row, 0);

         GLfloat rgba[MAX_WIDTH][4];
         GLint col;

         /* convert row to RGBA format */
         for (col = 0; col < width; col++) {
            texImage->FetchTexelf(texImage, col, row, img, rgba[col]);
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
   }
}


#else /* FEATURE_EXT_texture_sRGB */


static INLINE void
get_tex_srgb(GLcontext *ctx, GLuint dimensions,
             GLenum format, GLenum type, GLvoid *pixels,
             const struct gl_texture_image *texImage)
{
   ASSERT_NO_FEATURE();
}


#endif /* FEATURE_EXT_texture_sRGB */


/**
 * glGetTexImagefor RGBA, Luminance, etc. pixels.
 * This is the slow way since we use texture sampling.
 */
static void
get_tex_rgba(GLcontext *ctx, GLuint dimensions,
             GLenum format, GLenum type, GLvoid *pixels,
             const struct gl_texture_image *texImage)
{
   const GLint width = texImage->Width;
   const GLint height = texImage->Height;
   const GLint depth = texImage->Depth;
   /* Normally, no pixel transfer ops are performed during glGetTexImage.
    * The only possible exception is component clamping to [0,1].
    */
   GLbitfield transferOps = 0x0;
   GLint img, row;

   for (img = 0; img < depth; img++) {
      for (row = 0; row < height; row++) {
         void *dest = _mesa_image_address(dimensions, &ctx->Pack, pixels,
                                          width, height, format, type,
                                          img, row, 0);
         GLfloat rgba[MAX_WIDTH][4];
         GLint col;
         GLenum dataType = _mesa_get_format_datatype(texImage->TexFormat);

         /* clamp does not apply to GetTexImage (final conversion)?
          * Looks like we need clamp though when going from format
          * containing negative values to unsigned format.
          */
         if (format == GL_LUMINANCE || format == GL_LUMINANCE_ALPHA) {
            transferOps |= IMAGE_CLAMP_BIT;
         }
         else if (!type_with_negative_values(type) &&
                  (dataType == GL_FLOAT ||
                   dataType == GL_SIGNED_NORMALIZED)) {
            transferOps |= IMAGE_CLAMP_BIT;
         }

         for (col = 0; col < width; col++) {
            texImage->FetchTexelf(texImage, col, row, img, rgba[col]);
            if (texImage->_BaseFormat == GL_ALPHA) {
               rgba[col][RCOMP] = 0.0F;
               rgba[col][GCOMP] = 0.0F;
               rgba[col][BCOMP] = 0.0F;
            }
            else if (texImage->_BaseFormat == GL_LUMINANCE) {
               rgba[col][GCOMP] = 0.0F;
               rgba[col][BCOMP] = 0.0F;
               rgba[col][ACOMP] = 1.0F;
            }
            else if (texImage->_BaseFormat == GL_LUMINANCE_ALPHA) {
               rgba[col][GCOMP] = 0.0F;
               rgba[col][BCOMP] = 0.0F;
            }
            else if (texImage->_BaseFormat == GL_INTENSITY) {
               rgba[col][GCOMP] = 0.0F;
               rgba[col][BCOMP] = 0.0F;
               rgba[col][ACOMP] = 1.0F;
            }
         }
         _mesa_pack_rgba_span_float(ctx, width, (GLfloat (*)[4]) rgba,
                                    format, type, dest,
                                    &ctx->Pack, transferOps);
      }
   }
}


/**
 * Try to do glGetTexImage() with simple memcpy().
 * \return GL_TRUE if done, GL_FALSE otherwise
 */
static GLboolean
get_tex_memcpy(GLcontext *ctx, GLenum format, GLenum type, GLvoid *pixels,
               const struct gl_texture_object *texObj,
               const struct gl_texture_image *texImage)
{
   GLboolean memCopy = GL_FALSE;

   /* Texture image should have been mapped already */
   assert(texImage->Data);

   /*
    * Check if the src/dst formats are compatible.
    * Also note that GL's pixel transfer ops don't apply to glGetTexImage()
    * so we don't have to worry about those.
    * XXX more format combinations could be supported here.
    */
   if ((texObj->Target == GL_TEXTURE_1D ||
        texObj->Target == GL_TEXTURE_2D ||
        texObj->Target == GL_TEXTURE_RECTANGLE ||
        (texObj->Target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X &&
         texObj->Target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z))) {
      if (texImage->TexFormat == MESA_FORMAT_ARGB8888 &&
          format == GL_BGRA &&
          type == GL_UNSIGNED_BYTE &&
          !ctx->Pack.SwapBytes &&
          _mesa_little_endian()) {
         memCopy = GL_TRUE;
      }
      else if (texImage->TexFormat == MESA_FORMAT_AL88 &&
               format == GL_LUMINANCE_ALPHA &&
               type == GL_UNSIGNED_BYTE &&
               !ctx->Pack.SwapBytes &&
               _mesa_little_endian()) {
         memCopy = GL_TRUE;
      }
      else if (texImage->TexFormat == MESA_FORMAT_L8 &&
               format == GL_LUMINANCE &&
               type == GL_UNSIGNED_BYTE) {
         memCopy = GL_TRUE;
      }
      else if (texImage->TexFormat == MESA_FORMAT_A8 &&
               format == GL_ALPHA &&
               type == GL_UNSIGNED_BYTE) {
         memCopy = GL_TRUE;
      }
   }

   if (memCopy) {
      const GLuint bpp = _mesa_get_format_bytes(texImage->TexFormat);
      const GLuint bytesPerRow = texImage->Width * bpp;
      GLubyte *dst =
         _mesa_image_address2d(&ctx->Pack, pixels, texImage->Width,
                               texImage->Height, format, type, 0, 0);
      const GLint dstRowStride =
         _mesa_image_row_stride(&ctx->Pack, texImage->Width, format, type);
      const GLubyte *src = texImage->Data;
      const GLint srcRowStride = texImage->RowStride * bpp;
      GLuint row;

      if (bytesPerRow == dstRowStride && bytesPerRow == srcRowStride) {
         memcpy(dst, src, bytesPerRow * texImage->Height);
      }
      else {
         for (row = 0; row < texImage->Height; row++) {
            memcpy(dst, src, bytesPerRow);
            dst += dstRowStride;
            src += srcRowStride;
         }
      }
   }

   return memCopy;
}


/**
 * This is the software fallback for Driver.GetTexImage().
 * All error checking will have been done before this routine is called.
 * The texture image must be mapped.
 */
void
_mesa_get_teximage(GLcontext *ctx, GLenum target, GLint level,
                   GLenum format, GLenum type, GLvoid *pixels,
                   struct gl_texture_object *texObj,
                   struct gl_texture_image *texImage)
{
   GLuint dimensions;

   /* If we get here, the texture image should be mapped */
   assert(texImage->Data);

   switch (target) {
   case GL_TEXTURE_1D:
      dimensions = 1;
      break;
   case GL_TEXTURE_3D:
      dimensions = 3;
      break;
   default:
      dimensions = 2;
   }

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

   if (get_tex_memcpy(ctx, format, type, pixels, texObj, texImage)) {
      /* all done */
   }
   else if (format == GL_COLOR_INDEX) {
      get_tex_color_index(ctx, dimensions, format, type, pixels, texImage);
   }
   else if (format == GL_DEPTH_COMPONENT) {
      get_tex_depth(ctx, dimensions, format, type, pixels, texImage);
   }
   else if (format == GL_DEPTH_STENCIL_EXT) {
      get_tex_depth_stencil(ctx, dimensions, format, type, pixels, texImage);
   }
   else if (format == GL_YCBCR_MESA) {
      get_tex_ycbcr(ctx, dimensions, format, type, pixels, texImage);
   }
   else if (_mesa_get_format_color_encoding(texImage->TexFormat) == GL_SRGB) {
      get_tex_srgb(ctx, dimensions, format, type, pixels, texImage);
   }
   else {
      get_tex_rgba(ctx, dimensions, format, type, pixels, texImage);
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
   const GLuint row_stride = _mesa_format_row_stride(texImage->TexFormat,
                                                     texImage->Width);
   const GLuint row_stride_stored = _mesa_format_row_stride(texImage->TexFormat,
                                                            texImage->RowStride);
   GLuint i;

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

   /* no pixelstore or pixel transfer, but respect stride */

   if (row_stride == row_stride_stored) {
      const GLuint size = _mesa_format_image_size(texImage->TexFormat,
                                                  texImage->Width,
                                                  texImage->Height,
                                                  texImage->Depth);
      memcpy(img, texImage->Data, size);
   }
   else {
      GLuint bw, bh;
      _mesa_get_format_block_size(texImage->TexFormat, &bw, &bh);
      for (i = 0; i < (texImage->Height + bh - 1) / bh; i++) {
         memcpy((GLubyte *)img + i * row_stride,
                (GLubyte *)texImage->Data + i * row_stride_stored,
                row_stride);
      }
   }

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
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   const GLint maxLevels = _mesa_max_texture_levels(ctx, target);
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

   texObj = _mesa_get_current_tex_object(ctx, target);

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
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (getteximage_error_check(ctx, target, level, format, type, pixels)) {
      return;
   }

   if (!_mesa_is_bufferobj(ctx->Pack.BufferObj) && !pixels) {
      /* not an error, do nothing */
      return;
   }

   texObj = _mesa_get_current_tex_object(ctx, target);
   texImage = _mesa_select_tex_image(ctx, texObj, target, level);

   if (MESA_VERBOSE & (VERBOSE_API | VERBOSE_TEXTURE)) {
      _mesa_debug(ctx, "glGetTexImage(tex %u) format = %s, w=%d, h=%d,"
                  " dstFmt=0x%x, dstType=0x%x\n",
                  texObj->Name,
                  _mesa_get_format_name(texImage->TexFormat),
                  texImage->Width, texImage->Height,
                  format, type);
   }

   _mesa_lock_texture(ctx, texObj);
   {
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
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   const GLint maxLevels = _mesa_max_texture_levels(ctx, target);

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

   texObj = _mesa_get_current_tex_object(ctx, target);
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
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (getcompressedteximage_error_check(ctx, target, level, img)) {
      return;
   }

   if (_mesa_is_bufferobj(ctx->Pack.BufferObj) && !img) {
      /* not an error, do nothing */
      return;
   }

   texObj = _mesa_get_current_tex_object(ctx, target);
   texImage = _mesa_select_tex_image(ctx, texObj, target, level);

   if (MESA_VERBOSE & (VERBOSE_API | VERBOSE_TEXTURE)) {
      _mesa_debug(ctx,
                  "glGetCompressedTexImage(tex %u) format = %s, w=%d, h=%d\n",
                  texObj->Name,
                  _mesa_get_format_name(texImage->TexFormat),
                  texImage->Width, texImage->Height);
   }

   _mesa_lock_texture(ctx, texObj);
   {
      ctx->Driver.GetCompressedTexImage(ctx, target, level, img,
                                        texObj, texImage);
   }
   _mesa_unlock_texture(ctx, texObj);
}

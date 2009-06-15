/*
 * Mesa 3-D graphics library
 * Version:  7.5
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
#include "context.h"
#include "image.h"
#include "texcompress.h"
#include "texformat.h"
#include "texgetimage.h"



#if FEATURE_EXT_texture_sRGB

/**
 * Test if given texture image is an sRGB format.
 */
static GLboolean
is_srgb_teximage(const struct gl_texture_image *texImage)
{
   switch (texImage->TexFormat->MesaFormat) {
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

   if (ctx->Pack.BufferObj->Name) {
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
         /* buffer is already mapped - that's an error */
         _mesa_error(ctx, GL_INVALID_OPERATION,"glGetTexImage(PBO is mapped)");
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
               /* Can't use FetchTexel here because that returns RGBA */
               if (texImage->TexFormat->IndexBits == 8) {
                  const GLubyte *src = (const GLubyte *) texImage->Data;
                  src += width * (img * texImage->Height + row);
                  for (col = 0; col < width; col++) {
                     indexRow[col] = src[col];
                  }
               }
               else if (texImage->TexFormat->IndexBits == 16) {
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
               if ((texImage->TexFormat->MesaFormat == MESA_FORMAT_YCBCR
                    && type == GL_UNSIGNED_SHORT_8_8_REV_MESA) ||
                   (texImage->TexFormat->MesaFormat == MESA_FORMAT_YCBCR_REV
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

               /* clamp does not apply to GetTexImage (final conversion)?
                * Looks like we need clamp though when going from format
                * containing negative values to unsigned format.
                */
               if (format == GL_LUMINANCE || format == GL_LUMINANCE_ALPHA)
                  transferOps |= IMAGE_CLAMP_BIT;
               else if (!type_with_negative_values(type) &&
                        (texImage->TexFormat->DataType == GL_FLOAT ||
                         texImage->TexFormat->DataType == GL_SIGNED_NORMALIZED))
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

   if (ctx->Pack.BufferObj->Name) {
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
   GLuint size;

   if (ctx->Pack.BufferObj->Name) {
      /* pack texture image into a PBO */
      GLubyte *buf;
      if ((const GLubyte *) img + texImage->CompressedSize >
          (const GLubyte *) ctx->Pack.BufferObj->Size) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glGetCompressedTexImage(invalid PBO access)");
         return;
      }
      buf = (GLubyte *) ctx->Driver.MapBuffer(ctx, GL_PIXEL_PACK_BUFFER_EXT,
                                              GL_WRITE_ONLY_ARB,
                                              ctx->Pack.BufferObj);
      if (!buf) {
         /* buffer is already mapped - that's an error */
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glGetCompressedTexImage(PBO is mapped)");
         return;
      }
      img = ADD_POINTERS(buf, img);
   }
   else if (!img) {
      /* not an error */
      return;
   }

   /* don't use texImage->CompressedSize since that may be padded out */
   size = _mesa_compressed_texture_size(ctx, texImage->Width, texImage->Height,
                                        texImage->Depth,
                                        texImage->TexFormat->MesaFormat);

   /* just memcpy, no pixelstore or pixel transfer */
   _mesa_memcpy(img, texImage->Data, size);

   if (ctx->Pack.BufferObj->Name) {
      ctx->Driver.UnmapBuffer(ctx, GL_PIXEL_PACK_BUFFER_EXT,
                              ctx->Pack.BufferObj);
   }
}

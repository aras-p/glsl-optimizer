/*
 * Mesa 3-D graphics library
 * Version:  6.5.1
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
 * Copyright (c) 2008 VMware, Inc.
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
 * \file texcompress.c
 * Helper functions for texture compression.
 */


#include "glheader.h"
#include "imports.h"
#include "colormac.h"
#include "formats.h"
#include "mfeatures.h"
#include "mtypes.h"
#include "texcompress.h"


/**
 * Return list of (and count of) all specific texture compression
 * formats that are supported.
 *
 * \param ctx  the GL context
 * \param formats  the resulting format list (may be NULL).
 * \param all  if true return all formats, even those with  some kind
 *             of restrictions/limitations (See GL_ARB_texture_compression
 *             spec for more info).
 *
 * \return number of formats.
 */
GLuint
_mesa_get_compressed_formats(struct gl_context *ctx, GLint *formats, GLboolean all)
{
   GLuint n = 0;
   if (ctx->Extensions.TDFX_texture_compression_FXT1) {
      if (formats) {
         formats[n++] = GL_COMPRESSED_RGB_FXT1_3DFX;
         formats[n++] = GL_COMPRESSED_RGBA_FXT1_3DFX;
      }
      else {
         n += 2;
      }
   }
   /* don't return RGTC - ARB_texture_compression_rgtc query 19 */
   if (ctx->Extensions.EXT_texture_compression_s3tc) {
      if (formats) {
         formats[n++] = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
         /* This format has some restrictions/limitations and so should
          * not be returned via the GL_COMPRESSED_TEXTURE_FORMATS query.
          * Specifically, all transparent pixels become black.  NVIDIA
          * omits this format too.
          */
         if (all)
             formats[n++] = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
         formats[n++] = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
         formats[n++] = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
      }
      else {
         n += 3;
         if (all)
             n += 1;
      }
   }
   if (ctx->Extensions.S3_s3tc) {
      if (formats) {
         formats[n++] = GL_RGB_S3TC;
         formats[n++] = GL_RGB4_S3TC;
         formats[n++] = GL_RGBA_S3TC;
         formats[n++] = GL_RGBA4_S3TC;
      }
      else {
         n += 4;
      }
   }
#if FEATURE_EXT_texture_sRGB
   if (ctx->Extensions.EXT_texture_sRGB) {
      if (formats) {
         formats[n++] = GL_COMPRESSED_SRGB_S3TC_DXT1_EXT;
         formats[n++] = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
         formats[n++] = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;
         formats[n++] = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
      }
      else {
         n += 4;
      }
   }
#endif /* FEATURE_EXT_texture_sRGB */
   return n;

#if FEATURE_ES1 || FEATURE_ES2
   if (formats) {
      formats[n++] = GL_PALETTE4_RGB8_OES;
      formats[n++] = GL_PALETTE4_RGBA8_OES;
      formats[n++] = GL_PALETTE4_R5_G6_B5_OES;
      formats[n++] = GL_PALETTE4_RGBA4_OES;
      formats[n++] = GL_PALETTE4_RGB5_A1_OES;
      formats[n++] = GL_PALETTE8_RGB8_OES;
      formats[n++] = GL_PALETTE8_RGBA8_OES;
      formats[n++] = GL_PALETTE8_R5_G6_B5_OES;
      formats[n++] = GL_PALETTE8_RGBA4_OES;
      formats[n++] = GL_PALETTE8_RGB5_A1_OES;
   }
   else {
      n += 10;
   }
#endif
}


/**
 * Convert a compressed MESA_FORMAT_x to a GLenum.
 */
gl_format
_mesa_glenum_to_compressed_format(GLenum format)
{
   switch (format) {
   case GL_COMPRESSED_RGB_FXT1_3DFX:
      return MESA_FORMAT_RGB_FXT1;
   case GL_COMPRESSED_RGBA_FXT1_3DFX:
      return MESA_FORMAT_RGBA_FXT1;

   case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
   case GL_RGB_S3TC:
      return MESA_FORMAT_RGB_DXT1;
   case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
   case GL_RGB4_S3TC:
      return MESA_FORMAT_RGBA_DXT1;
   case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
   case GL_RGBA_S3TC:
      return MESA_FORMAT_RGBA_DXT3;
   case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
   case GL_RGBA4_S3TC:
      return MESA_FORMAT_RGBA_DXT5;

   case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
      return MESA_FORMAT_SRGB_DXT1;
   case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
      return MESA_FORMAT_SRGBA_DXT1;
   case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
      return MESA_FORMAT_SRGBA_DXT3;
   case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
      return MESA_FORMAT_SRGBA_DXT5;

   case GL_COMPRESSED_RED_RGTC1:
      return MESA_FORMAT_RED_RGTC1;
   case GL_COMPRESSED_SIGNED_RED_RGTC1:
      return MESA_FORMAT_SIGNED_RED_RGTC1;
   case GL_COMPRESSED_RG_RGTC2:
      return MESA_FORMAT_RG_RGTC2;
   case GL_COMPRESSED_SIGNED_RG_RGTC2:
      return MESA_FORMAT_SIGNED_RG_RGTC2;

   case GL_COMPRESSED_LUMINANCE_LATC1_EXT:
      return MESA_FORMAT_L_LATC1;
   case GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT:
      return MESA_FORMAT_SIGNED_L_LATC1;
   case GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT:
   case GL_COMPRESSED_LUMINANCE_ALPHA_3DC_ATI:
      return MESA_FORMAT_LA_LATC2;
   case GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT:
      return MESA_FORMAT_SIGNED_LA_LATC2;

   default:
      return MESA_FORMAT_NONE;
   }
}


/**
 * Given a compressed MESA_FORMAT_x value, return the corresponding
 * GLenum for that format.
 * This is needed for glGetTexLevelParameter(GL_TEXTURE_INTERNAL_FORMAT)
 * which must return the specific texture format used when the user might
 * have originally specified a generic compressed format in their
 * glTexImage2D() call.
 * For non-compressed textures, we always return the user-specified
 * internal format unchanged.
 */
GLenum
_mesa_compressed_format_to_glenum(struct gl_context *ctx, GLuint mesaFormat)
{
   switch (mesaFormat) {
#if FEATURE_texture_fxt1
   case MESA_FORMAT_RGB_FXT1:
      return GL_COMPRESSED_RGB_FXT1_3DFX;
   case MESA_FORMAT_RGBA_FXT1:
      return GL_COMPRESSED_RGBA_FXT1_3DFX;
#endif
#if FEATURE_texture_s3tc
   case MESA_FORMAT_RGB_DXT1:
      return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
   case MESA_FORMAT_RGBA_DXT1:
      return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
   case MESA_FORMAT_RGBA_DXT3:
      return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
   case MESA_FORMAT_RGBA_DXT5:
      return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
#if FEATURE_EXT_texture_sRGB
   case MESA_FORMAT_SRGB_DXT1:
      return GL_COMPRESSED_SRGB_S3TC_DXT1_EXT;
   case MESA_FORMAT_SRGBA_DXT1:
      return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
   case MESA_FORMAT_SRGBA_DXT3:
      return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;
   case MESA_FORMAT_SRGBA_DXT5:
      return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
#endif
#endif

   case MESA_FORMAT_RED_RGTC1:
      return GL_COMPRESSED_RED_RGTC1;
   case MESA_FORMAT_SIGNED_RED_RGTC1:
      return GL_COMPRESSED_SIGNED_RED_RGTC1;
   case MESA_FORMAT_RG_RGTC2:
      return GL_COMPRESSED_RG_RGTC2;
   case MESA_FORMAT_SIGNED_RG_RGTC2:
      return GL_COMPRESSED_SIGNED_RG_RGTC2;

   case MESA_FORMAT_L_LATC1:
      return GL_COMPRESSED_LUMINANCE_LATC1_EXT;
   case MESA_FORMAT_SIGNED_L_LATC1:
      return GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT;
   case MESA_FORMAT_LA_LATC2:
      return GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT;
   case MESA_FORMAT_SIGNED_LA_LATC2:
      return GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT;

   default:
      _mesa_problem(ctx, "Unexpected mesa texture format in"
                    " _mesa_compressed_format_to_glenum()");
      return 0;
   }
}


/*
 * Return the address of the pixel at (col, row, img) in a
 * compressed texture image.
 * \param col, row, img - image position (3D), should be a multiple of the
 *                        format's block size.
 * \param format - compressed image format
 * \param width - image width (stride) in pixels
 * \param image - the image address
 * \return address of pixel at (row, col, img)
 */
GLubyte *
_mesa_compressed_image_address(GLint col, GLint row, GLint img,
                               gl_format mesaFormat,
                               GLsizei width, const GLubyte *image)
{
   /* XXX only 2D images implemented, not 3D */
   const GLuint blockSize = _mesa_get_format_bytes(mesaFormat);
   GLuint bw, bh;
   GLint offset;

   _mesa_get_format_block_size(mesaFormat, &bw, &bh);

   ASSERT(col % bw == 0);
   ASSERT(row % bh == 0);

   offset = ((width + bw - 1) / bw) * (row / bh) + col / bw;
   offset *= blockSize;

   return (GLubyte *) image + offset;
}

/*
 * Mesa 3-D graphics library
 * Version:  5.1
 *
 * Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
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


#include "glheader.h"
#include "imports.h"
#include "context.h"
#include "image.h"
#include "texcompress.h"
#include "texformat.h"


/**
 * Get the list of supported internal compression formats.
 * \param formats - the results list (may be NULL)
 * \return number of formats.
 */
GLuint
_mesa_get_compressed_formats( GLcontext *ctx, GLint *formats )
{
   GLuint n = 0;
   if (ctx->Extensions.ARB_texture_compression) {
      if (ctx->Extensions.TDFX_texture_compression_FXT1) {
         if (formats) {
            formats[n++] = GL_COMPRESSED_RGB_FXT1_3DFX;
            formats[n++] = GL_COMPRESSED_RGBA_FXT1_3DFX;
         }
         else {
            n += 2;
         }
      }
      if (ctx->Extensions.EXT_texture_compression_s3tc) {
         if (formats) {
            formats[n++] = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
            /* Skip this one because it has a restriction (all transparent
             * pixels become black).  See the texture compressions spec for
             * a detailed explanation.  This is what NVIDIA does.
            formats[n++] = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
            */
            formats[n++] = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            formats[n++] = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
         }
         else {
            n += 3;
         }
      }
   }
   return n;
}



/**
 * Return bytes of storage needed for the given texture size and compressed
 * format.
 * \param width, height, depth  the texture size in texels
 * \param texFormat   one of the specific compressed format enums
 * \return size in bytes, or zero if bad texFormat
 */
GLuint
_mesa_compressed_texture_size( GLcontext *ctx,
                               GLsizei width, GLsizei height, GLsizei depth,
                               GLenum format )
{
   GLuint size;

   switch (format) {
   case GL_COMPRESSED_RGB_FXT1_3DFX:
   case GL_COMPRESSED_RGBA_FXT1_3DFX:
      /* round up to multiple of 4 */
      size = ((width + 7) / 8) * ((height + 3) / 4) * 16;
      return size;
   case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
   case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
      /* round up width, height to next multiple of 4 */
      width = (width + 3) & ~3;
      height = (height + 3) & ~3;
      ASSERT(depth == 1);
      /* 8 bytes per 4x4 tile of RGB[A] texels */
      size = (width * height * 8) / 16;
      /* Textures smaller than 4x4 will effectively be made into 4x4 and
       * take 8 bytes.
       */
      if (size < 8)
         size = 8;
      return size;
   case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
   case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
      /* round up width, height to next multiple of 4 */
      width = (width + 3) & ~3;
      height = (height + 3) & ~3;
      ASSERT(depth == 1);
      /* 16 bytes per 4x4 tile of RGBA texels */
      size = width * height; /* simple! */
      /* Textures smaller than 4x4 will effectively be made into 4x4 and
       * take 16 bytes.
       */
      if (size < 16)
         size = 16;
      return size;
   default:
      _mesa_problem(ctx, "bad texformat in compressed_texture_size");
      return 0;
   }
}


/**
 * Compute the bytes per row in a compressed texture image.
 * We use this for computing the destination address for sub-texture updates.
 * \param format  one of the specific texture compression formats
 * \param width  image width in pixels
 * \return stride, in bytes, between rows for compressed image
 */
GLint
_mesa_compressed_row_stride(GLenum format, GLsizei width)
{
   GLint bytesPerTile, stride;

   switch (format) {
   case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
   case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
      bytesPerTile = 8;
      break;
   case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
   case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
      bytesPerTile = 16;
      break;
   default:
      return 0;
   }

   stride = ((width + 3) / 4) * bytesPerTile;
   return stride;
}


/**
 * Return the address of the pixel at (col, row, img) in a
 * compressed texture image.
 * \param col, row, img - image position (3D)
 * \param format - compressed image format
 * \param width - image width
 * \param image - the image address
 * \return address of pixel at (row, col)
 */
GLubyte *
_mesa_compressed_image_address(GLint col, GLint row, GLint img,
                               GLenum format,
                               GLsizei width, const GLubyte *image)
{
   GLint bytesPerTile, stride;
   GLubyte *addr;

   ASSERT((row & 3) == 0);
   ASSERT((col & 3) == 0);
   (void) img;

   switch (format) {
   case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
   case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
      bytesPerTile = 8;
      break;
   case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
   case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
      bytesPerTile = 16;
      break;
   default:
      return 0;
   }

   stride = ((width + 3) / 4) * bytesPerTile;

   addr = (GLubyte *) image + (row / 4) * stride + (col / 4) * bytesPerTile;
   return addr;
}



/*
 * \param srcRowStride - source stride, in pixels
 */
void
_mesa_compress_teximage( GLcontext *ctx, GLsizei width, GLsizei height,
                         GLenum srcFormat, const GLchan *source,
                         GLint srcRowStride,
                         const struct gl_texture_format *dstFormat,
                         GLubyte *dest, GLint dstRowStride )
{
   switch (dstFormat->MesaFormat) {
   default:
      _mesa_problem(ctx, "Bad dstFormat in _mesa_compress_teximage()");
      return;
   }
}

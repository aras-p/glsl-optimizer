/* $Id: texcompress.c,v 1.1 2002/09/27 02:45:38 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  4.1
 *
 * Copyright (C) 1999-2002  Brian Paul   All Rights Reserved.
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
#include "context.h"
#include "image.h"
#include "mem.h"
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
            n += 4;
         }
      }
   }
   return n;
}



/**
 * Return bytes of storage needed for the given texture size and compressed
 * format.
 * \param width, height, depth - texture size in texels
 * \param texFormat - one of the compressed format enums
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
   default:
      _mesa_problem(ctx, "bad texformat in compressed_texture_size");
      return 0;
   }
}


/*
 * Compute the bytes per row in a compressed texture image.
 */
GLint
_mesa_compressed_row_stride(GLenum format, GLsizei width)
{
   GLint bytesPerTile, stride;

   switch (format) {
   default:
      return 0;
   }

   stride = ((width + 3) / 4) * bytesPerTile;
   return stride;
}


/*
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
                   GLenum srcFormat, const GLchan *source, GLint srcRowStride,
                   GLenum dstFormat, GLubyte *dest, GLint dstRowStride )
{
   GLuint len = 0;

   switch (dstFormat) {
   default:
      _mesa_problem(ctx, "Bad dstFormat in _mesa_compress_teximage()");
      return;
   }

   /* sanity check */
   ASSERT(len == _mesa_compressed_texture_size(ctx, width, height,
                                               1, dstFormat));
}

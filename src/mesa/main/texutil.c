/* $Id: texutil.c,v 1.1 2000/03/24 20:54:21 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.3
 * 
 * Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
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


#ifdef PC_HEADER
#include "all.h"
#else
#include "glheader.h"
#include "image.h"
#include "mem.h"
#include "texutil.h"
#include "types.h"
#endif


/*
 * Texture utilities which may be useful to device drivers.
 */




/*
 * Convert texture image data into one a specific internal format.
 * Input:
 *   dstFormat - the destination internal format
 *   dstWidth, dstHeight - the destination image size
 *   destImage - pointer to destination image buffer
 *   srcWidth, srcHeight - size of texture image
 *   srcFormat, srcType - format and datatype of source image
 *   srcImage - pointer to user's texture image
 *   packing - describes how user's texture image is packed.
 * Return: pointer to resulting image data or NULL if error or out of memory
 *   or NULL if we can't process the given image format/type/internalFormat
 *   combination.
 *
 * Supported type conversions:
 *   srcFormat           srcType                          dstFormat
 *   GL_INTENSITY        GL_UNSIGNED_BYTE                 MESA_I8
 *   GL_LUMINANCE        GL_UNSIGNED_BYTE                 MESA_L8
 *   GL_ALPHA            GL_UNSIGNED_BYTE                 MESA_A8
 *   GL_COLOR_INDEX      GL_UNSIGNED_BYTE                 MESA_C8
 *   GL_LUMINANCE_ALPHA  GL_UNSIGNED_BYTE                 MESA_L8_A8
 *   GL_RGB              GL_UNSIGNED_BYTE                 MESA_R5_G6_B5
 *   GL_RGB              GL_UNSIGNED_SHORT_5_6_5          MESA_R5_G6_B5
 *   GL_RGBA             GL_UNSIGNED_BYTE                 MESA_A4_R4_G4_B4
 *   GL_BGRA             GL_UNSIGNED_SHORT_4_4_4_4_REV    MESA_A4_R4_G4_B4
 *   GL_BGRA             GL_UNSIGHED_SHORT_1_5_5_5_REV    MESA_A1_R5_G5_B5
 *   GL_BGRA             GL_UNSIGNED_INT_8_8_8_8_REV      MESA_A8_R8_G8_B8
 *   more to be added for new drivers...
 *
 * Notes:
 *   Some hardware only supports texture images of specific aspect ratios.
 *   This code will do power-of-two image up-scaling if that's needed.
 *
 *
 */
GLboolean
_mesa_convert_teximage(MesaIntTexFormat dstFormat,
                       GLint dstWidth, GLint dstHeight, GLvoid *dstImage,
                       GLsizei srcWidth, GLsizei srcHeight,
                       GLenum srcFormat, GLenum srcType,
                       const GLvoid *srcImage,
                       const struct gl_pixelstore_attrib *packing)
{
   const GLint wScale = dstWidth / srcWidth;   /* must be power of two */
   const GLint hScale = dstHeight / srcHeight; /* must be power of two */
   ASSERT(dstWidth >= srcWidth);
   ASSERT(dstHeight >= srcHeight);
   ASSERT(dstImage);
   ASSERT(srcImage);
   ASSERT(packing);

   switch (dstFormat) {
      case MESA_I8:
      case MESA_L8:
      case MESA_A8:
      case MESA_C8:
         if (srcType != GL_UNSIGNED_BYTE ||
             ((srcFormat != GL_INTENSITY) &&
              (srcFormat != GL_LUMINANCE) &&
              (srcFormat != GL_ALPHA) &&
              (srcFormat != GL_COLOR_INDEX))) {
            /* bad internal format / srcFormat combination */
            return GL_FALSE;
         }
         else {
            /* store as 8-bit texels */
            if (wScale == 1 && hScale == 1) {
               /* no scaling needed - fast case */
               const GLubyte *src = _mesa_image_address(packing, srcImage,
                             srcWidth, srcHeight, srcFormat, srcType, 0, 0, 0);
               const GLint srcStride = _mesa_image_row_stride(packing,
                                                 srcWidth, srcFormat, srcType);
               GLubyte *dst = (GLubyte *) dstImage;
               GLint row;
               for (row = 0; row < dstHeight; row++) {
                  MEMCPY(dst, src, dstWidth * sizeof(GLubyte));
                  dst += dstWidth;
                  src += srcStride;
               }
            }
            else {
               /* must rescale image */
               GLubyte *dst = (GLubyte *) dstImage;
               GLint row;
               for (row = 0; row < dstHeight; row++) {
                  GLint srcRow = row / hScale;
                  const GLubyte *src = _mesa_image_address(packing, srcImage,
                        srcWidth, srcHeight, srcFormat, srcType, 0, srcRow, 0);
                  GLint col;
                  for (col = 0; col < dstWidth; col++) {
                     dst[col] = src[col / wScale];
                  }
                  dst += dstWidth;
               }
            }
         }
         break;

      case MESA_L8_A8:
         if (srcType != GL_UNSIGNED_BYTE || srcFormat != GL_LUMINANCE_ALPHA) {
            return GL_FALSE;
         }
         else {
            /* store as 16-bit texels */
            if (wScale == 1 && hScale == 1) {
               const GLubyte *src = _mesa_image_address(packing, srcImage,
                             srcWidth, srcHeight, srcFormat, srcType, 0, 0, 0);
               const GLint srcStride = _mesa_image_row_stride(packing,
                                                 srcWidth, srcFormat, srcType);
               GLushort *dst = dstImage;
               GLint row, col;
               for (row = 0; row < dstHeight; row++) {
                  for (col = 0; col < dstWidth; col++) {
                     GLubyte alpha = src[col * 2 + 0];
                     GLubyte luminance = src[col * 2 + 1];
                     dst[col] = ((GLushort) luminance << 8) | alpha;
                  }
                  dst += dstWidth;
                  src += srcStride;
               }
            }
            else {
               /* must rescale */
               GLushort *dst = dstImage;
               GLint row, col;
               for (row = 0; row < dstHeight; row++) {
                  GLint srcRow = row / hScale;
                  const GLubyte *src = _mesa_image_address(packing, srcImage,
                        srcWidth, srcHeight, srcFormat, srcType, 0, srcRow, 0);
                  const GLint srcStride = _mesa_image_row_stride(packing,
                                                 srcWidth, srcFormat, srcType);
                  for (col = 0; col < dstWidth; col++) {
                     GLint srcCol = col / wScale;
                     GLubyte alpha = src[srcCol * 2 + 0];
                     GLubyte luminance = src[srcCol * 2 + 1];
                     dst[col] = ((GLushort) luminance << 8) | alpha;
                  }
                  dst += dstWidth;
                  src += srcStride;
               }
            }
         }
         break;

      case MESA_R5_G6_B5:
         if (srcFormat == GL_RGB && srcType == GL_UNSIGNED_SHORT_5_6_5) {
            /* special, optimized case */
            if (wScale == 1 && hScale == 1) {
               const GLubyte *src = _mesa_image_address(packing, srcImage,
                             srcWidth, srcHeight, srcFormat, srcType, 0, 0, 0);
               const GLint srcStride = _mesa_image_row_stride(packing,
                                                 srcWidth, srcFormat, srcType);
               GLushort *dst = dstImage;
               GLint row;
               for (row = 0; row < dstHeight; row++) {
                  MEMCPY(dst, src, dstWidth * sizeof(GLushort));
                  src += srcStride;
                  dst += dstWidth;
               }
            }
            else {
               /* must rescale image */
               GLushort *dst = dstImage;
               GLint row;
               for (row = 0; row < dstHeight; row++) {
                  GLint srcRow = row / hScale;
                  const GLushort *src = _mesa_image_address(packing, srcImage,
                        srcWidth, srcHeight, srcFormat, srcType, 0, srcRow, 0);
                  GLint col;
                  for (col = 0; col < dstWidth; col++) {
                     dst[col] = src[col / wScale];
                  }
                  dst += dstWidth;
               }
            }
         }
         else if (srcFormat == GL_RGB && srcType == GL_UNSIGNED_BYTE) {
            /* general case */
            if (wScale == 1 && hScale == 1) {
               const GLubyte *src = _mesa_image_address(packing, srcImage,
                             srcWidth, srcHeight, srcFormat, srcType, 0, 0, 0);
               const GLint srcStride = _mesa_image_row_stride(packing,
                                                 srcWidth, srcFormat, srcType);
               GLushort *dst = dstImage;
               GLint row;
               for (row = 0; row < dstHeight; row++) {
                  GLint col, col3;
                  for (col = col3 = 0; col < dstWidth; col++, col3 += 3) {
                     GLubyte r = src[col3 + 0];
                     GLubyte g = src[col3 + 1];
                     GLubyte b = src[col3 + 2];
                     dst[col] = ((r & 0xf8) << 8)
                              | ((g & 0xfc) << 3)
                              | ((b & 0xf8) >> 3);
                  }
                  src += srcStride;
                  dst += dstWidth;
               }
            }
            else {
               /* must rescale image */
               GLushort *dst = dstImage;
               GLint row;
               for (row = 0; row < dstHeight; row++) {
                  GLint srcRow = row / hScale;
                  const GLubyte *src = _mesa_image_address(packing, srcImage,
                        srcWidth, srcHeight, srcFormat, srcType, 0, srcRow, 0);
                  GLint col;
                  for (col = 0; col < dstWidth; col++) {
                     GLint col3 = (col / wScale) * 3;
                     GLubyte r = src[col3 + 0];
                     GLubyte g = src[col3 + 1];
                     GLubyte b = src[col3 + 2];
                     dst[col] = ((r & 0xf8) << 8)
                              | ((g & 0xfc) << 3)
                              | ((b & 0xf8) >> 3);
                  }
                  dst += dstWidth;
               }
            }
         }
         else {
            /* can't handle this srcFormat/srcType combination */
            return GL_FALSE;
         }
         break;

      case MESA_A4_R4_G4_B4:
         /* store as 16-bit texels (GR_TEXFMT_ARGB_4444) */
         if (srcFormat == GL_BGRA && srcType == GL_UNSIGNED_SHORT_4_4_4_4_REV){
            /* special, optimized case */
            if (wScale == 1 && hScale == 1) {
               const GLubyte *src = _mesa_image_address(packing, srcImage,
                             srcWidth, srcHeight, srcFormat, srcType, 0, 0, 0);
               const GLint srcStride = _mesa_image_row_stride(packing,
                                                 srcWidth, srcFormat, srcType);
               GLushort *dst = dstImage;
               GLint row;
               for (row = 0; row < dstHeight; row++) {
                  MEMCPY(dst, src, dstWidth * sizeof(GLushort));
                  src += srcStride;
                  dst += dstWidth;
               }
            }
            else {
               /* must rescale image */
               GLushort *dst = dstImage;
               GLint row;
               for (row = 0; row < dstHeight; row++) {
                  GLint srcRow = row / hScale;
                  const GLushort *src = _mesa_image_address(packing, srcImage,
                        srcWidth, srcHeight, srcFormat, srcType, 0, srcRow, 0);
                  GLint col;
                  for (col = 0; col < dstWidth; col++) {
                     dst[col] = src[col / wScale];
                  }
                  dst += dstWidth;
               }
            }
         }
         else if (srcFormat == GL_RGBA && srcType == GL_UNSIGNED_BYTE) {
            /* general case */
            if (wScale == 1 && hScale == 1) {
               const GLubyte *src = _mesa_image_address(packing, srcImage,
                             srcWidth, srcHeight, srcFormat, srcType, 0, 0, 0);
               const GLint srcStride = _mesa_image_row_stride(packing,
                                                 srcWidth, srcFormat, srcType);
               GLushort *dst = dstImage;
               GLint row;
               for (row = 0; row < dstHeight; row++) {
                  GLint col, col4;
                  for (col = col4 = 0; col < dstWidth; col++, col4 += 4) {
                     GLubyte r = src[col4 + 0];
                     GLubyte g = src[col4 + 1];
                     GLubyte b = src[col4 + 2];
                     GLubyte a = src[col4 + 3];
                     dst[col] = ((a & 0xf0) << 8)
                              | ((r & 0xf0) << 4)
                              | ((g & 0xf0)     )
                              | ((b & 0xf0) >> 4);
                  }
                  src += srcStride;
                  dst += dstWidth;
               }
            }
            else {
               /* must rescale image */
               GLushort *dst = dstImage;
               GLint row;
               for (row = 0; row < dstHeight; row++) {
                  GLint srcRow = row / hScale;
                  const GLubyte *src = _mesa_image_address(packing, srcImage,
                        srcWidth, srcHeight, srcFormat, srcType, 0, srcRow, 0);
                  GLint col;
                  for (col = 0; col < dstWidth; col++) {
                     GLint col4 = (col / wScale) * 4;
                     GLubyte r = src[col4 + 0];
                     GLubyte g = src[col4 + 1];
                     GLubyte b = src[col4 + 2];
                     GLubyte a = src[col4 + 3];
                     dst[col] = ((a & 0xf0) << 8)
                              | ((r & 0xf0) << 4)
                              | ((g & 0xf0)     )
                              | ((b & 0xf0) >> 4);
                  }
                  dst += dstWidth;
               }
            }
         }
         else {
            /* can't handle this format/srcType combination */
            return GL_FALSE;
         }
         break;

      case MESA_A1_R5_G5_B5:
         /* store as 16-bit texels (GR_TEXFMT_ARGB_1555) */
         if (srcFormat == GL_RGBA && srcType == GL_UNSIGNED_SHORT_1_5_5_5_REV){
            /* special, optimized case */
            if (wScale == 1 && hScale == 1) {
               const GLubyte *src = _mesa_image_address(packing, srcImage,
                             srcWidth, srcHeight, srcFormat, srcType, 0, 0, 0);
               const GLint srcStride = _mesa_image_row_stride(packing,
                                                 srcWidth, srcFormat, srcType);
               GLushort *dst = dstImage;
               GLint row;
               for (row = 0; row < dstHeight; row++) {
                  MEMCPY(dst, src, dstWidth * sizeof(GLushort));
                  src += srcStride;
                  dst += dstWidth;
               }
            }
            else {
               /* must rescale image */
               GLushort *dst = dstImage;
               GLint row;
               for (row = 0; row < dstHeight; row++) {
                  GLint srcRow = row / hScale;
                  const GLushort *src = _mesa_image_address(packing, srcImage,
                        srcWidth, srcHeight, srcFormat, srcType, 0, srcRow, 0);
                  GLint col;
                  for (col = 0; col < dstWidth; col++) {
                     dst[col] = src[col / wScale];
                  }
                  dst += dstWidth;
               }
            }
         }
         else if (srcFormat == GL_RGBA && srcType == GL_UNSIGNED_BYTE) {
            /* general case */
            if (wScale == 1 && hScale == 1) {
               const GLubyte *src = _mesa_image_address(packing, srcImage,
                             srcWidth, srcHeight, srcFormat, srcType, 0, 0, 0);
               const GLint srcStride = _mesa_image_row_stride(packing,
                                                 srcWidth, srcFormat, srcType);
               GLushort *dst = dstImage;
               GLint row;
               for (row = 0; row < dstHeight; row++) {
                  GLint col, col4;
                  for (col = col4 = 0; col < dstWidth; col++, col4 += 4) {
                     GLubyte r = src[col4 + 0];
                     GLubyte g = src[col4 + 1];
                     GLubyte b = src[col4 + 2];
                     GLubyte a = src[col4 + 3];
                     dst[col] = ((a & 0x80) << 8)
                              | ((r & 0xf8) << 7)
                              | ((g & 0xf8) << 2)
                              | ((b & 0xf8) >> 3);
                  }
                  src += srcStride;
                  dst += dstWidth;
               }
            }
            else {
               /* must rescale image */
               GLushort *dst = dstImage;
               GLint row;
               for (row = 0; row < dstHeight; row++) {
                  GLint srcRow = row / hScale;
                  const GLubyte *src = _mesa_image_address(packing, srcImage,
                        srcWidth, srcHeight, srcFormat, srcType, 0, srcRow, 0);
                  GLint col;
                  for (col = 0; col < dstWidth; col++) {
                     GLint col4 = (col / wScale) * 4;
                     GLubyte r = src[col4 + 0];
                     GLubyte g = src[col4 + 1];
                     GLubyte b = src[col4 + 2];
                     GLubyte a = src[col4 + 3];
                     dst[col] = ((a & 0x80) << 8)
                              | ((r & 0xf8) << 7)
                              | ((g & 0xf8) << 2)
                              | ((b & 0xf8) >> 3);
                  }
                  dst += dstWidth;
               }
            }
         }
         else {
            /* can't handle this source format/type combination */
            return GL_FALSE;
         }
         break;

      case MESA_A8_R8_G8_B8:
         /* 32-bit texels */
         if (srcFormat == GL_BGRA && srcType == GL_UNSIGNED_INT_8_8_8_8_REV){
            /* special, optimized case */
            if (wScale == 1 && hScale == 1) {
               const GLubyte *src = _mesa_image_address(packing, srcImage,
                             srcWidth, srcHeight, srcFormat, srcType, 0, 0, 0);
               const GLint srcStride = _mesa_image_row_stride(packing,
                                                 srcWidth, srcFormat, srcType);
               GLuint *dst = dstImage;
               GLint row;
               for (row = 0; row < dstHeight; row++) {
                  MEMCPY(dst, src, dstWidth * sizeof(GLuint));
                  src += srcStride;
                  dst += dstWidth;
               }
            }
            else {
               /* must rescale image */
               GLuint *dst = dstImage;
               GLint row;
               for (row = 0; row < dstHeight; row++) {
                  GLint srcRow = row / hScale;
                  const GLuint *src = _mesa_image_address(packing, srcImage,
                        srcWidth, srcHeight, srcFormat, srcType, 0, srcRow, 0);
                  GLint col;
                  for (col = 0; col < dstWidth; col++) {
                     dst[col] = src[col / wScale];
                  }
                  dst += dstWidth;
               }
            }
         }
         else if (srcFormat == GL_RGBA && srcType == GL_UNSIGNED_BYTE) {
            /* general case */
            if (wScale == 1 && hScale == 1) {
               const GLubyte *src = _mesa_image_address(packing, srcImage,
                             srcWidth, srcHeight, srcFormat, srcType, 0, 0, 0);
               const GLint srcStride = _mesa_image_row_stride(packing,
                                                 srcWidth, srcFormat, srcType);
               GLuint *dst = dstImage;
               GLint row;
               for (row = 0; row < dstHeight; row++) {
                  GLint col, col4;
                  for (col = col4 = 0; col < dstWidth; col++, col4 += 4) {
                     GLubyte r = src[col4 + 0];
                     GLubyte g = src[col4 + 1];
                     GLubyte b = src[col4 + 2];
                     GLubyte a = src[col4 + 3];
                     dst[col] = (a << 24) | (r << 16) | (g << 8) | b;
                  }
                  src += srcStride;
                  dst += dstWidth;
               }
            }
            else {
               /* must rescale image */
               GLushort *dst = dstImage;
               GLint row;
               for (row = 0; row < dstHeight; row++) {
                  GLint srcRow = row / hScale;
                  const GLubyte *src = _mesa_image_address(packing, srcImage,
                        srcWidth, srcHeight, srcFormat, srcType, 0, srcRow, 0);
                  GLint col;
                  for (col = 0; col < dstWidth; col++) {
                     GLint col4 = (col / wScale) * 4;
                     GLubyte r = src[col4 + 0];
                     GLubyte g = src[col4 + 1];
                     GLubyte b = src[col4 + 2];
                     GLubyte a = src[col4 + 3];
                     dst[col] = (a << 24) | (r << 16) | (g << 8) | b;
                  }
                  dst += dstWidth;
               }
            }
         }
         else {
            /* can't handle this source format/type combination */
            return GL_FALSE;
         }
         break;


      default:
         /* unexpected internal format! */
         return GL_FALSE;
   }
   return GL_TRUE;
}

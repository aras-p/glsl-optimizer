/* $Id: texutil.c,v 1.2 2000/03/27 18:56:26 brianp Exp $ */

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
#include "context.h"
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


/*
 * Used to convert 16-bit texels into GLubyte color components.
 */
static GLubyte R5G6B5toRed[0xffff];
static GLubyte R5G6B5toGreen[0xffff];
static GLubyte R5G6B5toBlue[0xffff];

static GLubyte A4R4G4B4toRed[0xffff];
static GLubyte A4R4G4B4toGreen[0xffff];
static GLubyte A4R4G4B4toBlue[0xffff];
static GLubyte A4R4G4B4toAlpha[0xffff];

static GLubyte A1R5G5B5toRed[0xffff];
static GLubyte A1R5G5B5toGreen[0xffff];
static GLubyte A1R5G5B5toBlue[0xffff];
static GLubyte A1R5G5B5toAlpha[0xffff];

static void
generate_lookup_tables(void)
{
   GLint i;
   for (i = 0; i <= 0xffff; i++) {
      GLint r = (i >> 8) & 0xf8;
      GLint g = (i >> 3) & 0xfc;
      GLint b = (i << 3) & 0xf8;
      r = r * 255 / 0xf8;
      g = g * 255 / 0xfc;
      b = b * 255 / 0xf8;
      R5G6B5toRed[i]   = r;
      R5G6B5toGreen[i] = g;
      R5G6B5toBlue[i]  = b;
   }

   for (i = 0; i <= 0xffff; i++) {
      GLint r = (i >>  8) & 0xf;
      GLint g = (i >>  4) & 0xf;
      GLint b = (i      ) & 0xf;
      GLint a = (i >> 12) & 0xf;
      r = r * 255 / 0xf;
      g = g * 255 / 0xf;
      b = b * 255 / 0xf;
      a = a * 255 / 0xf;
      A4R4G4B4toRed[i]   = r;
      A4R4G4B4toGreen[i] = g;
      A4R4G4B4toBlue[i]  = b;
      A4R4G4B4toAlpha[i] = b;
   }

   for (i = 0; i <= 0xffff; i++) {
      GLint r = (i >> 10) & 0xf8;
      GLint g = (i >>  5) & 0xf8;
      GLint b = (i      ) & 0xf8;
      GLint a = (i >> 15) & 0x1;
      r = r * 255 / 0xf8;
      g = g * 255 / 0xf8;
      b = b * 255 / 0xf8;
      a = a * 255;
      A1R5G5B5toRed[i]   = r;
      A1R5G5B5toGreen[i] = g;
      A1R5G5B5toBlue[i]  = b;
      A1R5G5B5toAlpha[i] = b;
   }
}



/*
 * Convert a texture image from an internal format to one of Mesa's
 * core internal formats.  This is likely to be used by glGetTexImage
 * and for fetching texture images when falling back to software rendering.
 *
 * Notes:
 *   This function will do power of two image down-scaling to accomodate
 *   drivers with limited texture image aspect ratios.
 *   The implicit dest data type is GL_UNSIGNED_BYTE.
 */
void
_mesa_unconvert_teximage(MesaIntTexFormat srcFormat,
                         GLint srcWidth, GLint srcHeight,
                         const GLvoid *srcImage,
                         GLsizei dstWidth, GLsizei dstHeight,
                         GLenum dstFormat, GLubyte *dstImage)
{
   static GLboolean firstCall = GL_TRUE;
   const GLint wScale = srcWidth / dstWidth;   /* must be power of two */
   const GLint hScale = srcHeight / dstHeight; /* must be power of two */
   ASSERT(srcWidth >= dstWidth);
   ASSERT(srcHeight >= dstHeight);
   ASSERT(dstImage);
   ASSERT(srcImage);

   if (firstCall) {
      generate_lookup_tables();
      firstCall = GL_FALSE;
   }

   switch (srcFormat) {
      case MESA_I8:
      case MESA_L8:
      case MESA_A8:
      case MESA_C8:
#ifdef DEBUG
         if (srcFormat == MESA_I8) {
            ASSERT(dstFormat == GL_INTENSITY);
         }
         else if (srcFormat == MESA_L8) {
            ASSERT(dstFormat == GL_LUMINANCE);
         }
         else if (srcFormat == MESA_A8) {
            ASSERT(dstFormat == GL_ALPHA);
         }
         else if (srcFormat == MESA_C8) {
            ASSERT(dstFormat == GL_COLOR_INDEX);
         }
#endif
         if (wScale == 1 && hScale == 1) {
            /* easy! */
            MEMCPY(dstImage, srcImage, dstWidth * dstHeight * sizeof(GLubyte));
         }
         else {
            /* rescale */
            const GLubyte *src8 = (const GLubyte *) srcImage;
            GLint row, col;
            for (row = 0; row < dstHeight; row++) {
               GLint srcRow = row * hScale;
               for (col = 0; col < dstWidth; col++) {
                  GLint srcCol = col * wScale;
                  *dstImage++ = src8[srcRow * srcWidth + srcCol];
               }
            }
         }
         break;
      case MESA_L8_A8:
         ASSERT(dstFormat == GL_LUMINANCE_ALPHA);
         if (wScale == 1 && hScale == 1) {
            GLint i, n = dstWidth * dstHeight;
            const GLushort *texel = (const GLushort *) srcImage;
            for (i = 0; i < n; i++) {
               const GLushort tex = *texel++;
               *dstImage++ = (tex >> 8);   /* luminance */
               *dstImage++ = (tex & 0xff); /* alpha */
            }
         }
         else {
            /* rescale */
            const GLushort *src16 = (const GLushort *) srcImage;
            GLint row, col;
            for (row = 0; row < dstHeight; row++) {
               GLint srcRow = row * hScale;
               for (col = 0; col < dstWidth; col++) {
                  GLint srcCol = col * wScale;
                  const GLushort tex = src16[srcRow * srcWidth + srcCol];
                  *dstImage++ = (tex >> 8);   /* luminance */
                  *dstImage++ = (tex & 0xff); /* alpha */
               }
            }
         }
         break;
      case MESA_R5_G6_B5:
         ASSERT(dstFormat == GL_RGB);
         if (wScale == 1 && hScale == 1) {
            GLint i, n = dstWidth * dstHeight;
            const GLushort *texel = (const GLushort *) srcImage;
            for (i = 0; i < n; i++) {
               const GLushort tex = *texel++;
               *dstImage++ = R5G6B5toRed[tex];
               *dstImage++ = R5G6B5toGreen[tex];
               *dstImage++ = R5G6B5toBlue[tex];
            }
         }
         else {
            /* rescale */
            const GLushort *src16 = (const GLushort *) srcImage;
            GLint row, col;
            for (row = 0; row < dstHeight; row++) {
               GLint srcRow = row * hScale;
               for (col = 0; col < dstWidth; col++) {
                  GLint srcCol = col * wScale;
                  const GLushort tex = src16[srcRow * srcWidth + srcCol];
                  *dstImage++ = R5G6B5toRed[tex];
                  *dstImage++ = R5G6B5toGreen[tex];
                  *dstImage++ = R5G6B5toBlue[tex];
               }
            }
         }
         break;
      case MESA_A4_R4_G4_B4:
         ASSERT(dstFormat == GL_RGBA);
         if (wScale == 1 && hScale == 1) {
            GLint i, n = dstWidth * dstHeight;
            const GLushort *texel = (const GLushort *) srcImage;
            for (i = 0; i < n; i++) {
               const GLushort tex = *texel++;
               *dstImage++ = A4R4G4B4toRed[tex];
               *dstImage++ = A4R4G4B4toGreen[tex];
               *dstImage++ = A4R4G4B4toBlue[tex];
               *dstImage++ = A4R4G4B4toAlpha[tex];
            }
         }
         else {
            /* rescale */
            const GLushort *src16 = (const GLushort *) srcImage;
            GLint row, col;
            for (row = 0; row < dstHeight; row++) {
               GLint srcRow = row * hScale;
               for (col = 0; col < dstWidth; col++) {
                  GLint srcCol = col * wScale;
                  const GLushort tex = src16[srcRow * srcWidth + srcCol];
                  *dstImage++ = A4R4G4B4toRed[tex];
                  *dstImage++ = A4R4G4B4toGreen[tex];
                  *dstImage++ = A4R4G4B4toBlue[tex];
                  *dstImage++ = A4R4G4B4toAlpha[tex];
               }
            }
         }
         break;
      case MESA_A1_R5_G5_B5:
         ASSERT(dstFormat == GL_RGBA);
         if (wScale == 1 && hScale == 1) {
            GLint i, n = dstWidth * dstHeight;
            const GLushort *texel = (const GLushort *) srcImage;
            for (i = 0; i < n; i++) {
               const GLushort tex = *texel++;
               *dstImage++ = A1R5G5B5toRed[tex];
               *dstImage++ = A1R5G5B5toGreen[tex];
               *dstImage++ = A1R5G5B5toBlue[tex];
               *dstImage++ = A1R5G5B5toAlpha[tex];
            }
         }
         else {
            /* rescale */
            const GLushort *src16 = (const GLushort *) srcImage;
            GLint row, col;
            for (row = 0; row < dstHeight; row++) {
               GLint srcRow = row * hScale;
               for (col = 0; col < dstWidth; col++) {
                  GLint srcCol = col * wScale;
                  const GLushort tex = src16[srcRow * srcWidth + srcCol];
                  *dstImage++ = A1R5G5B5toRed[tex];
                  *dstImage++ = A1R5G5B5toGreen[tex];
                  *dstImage++ = A1R5G5B5toBlue[tex];
                  *dstImage++ = A1R5G5B5toAlpha[tex];
               }
            }
         }
         break;
      case MESA_A8_R8_G8_B8:
         ASSERT(dstFormat == GL_RGBA);
         if (wScale == 1 && hScale == 1) {
            GLint i, n = dstWidth * dstHeight;
            const GLushort *texel = (const GLushort *) srcImage;
            for (i = 0; i < n; i++) {
               const GLuint tex = *texel++;
               *dstImage++ = (tex >> 16) & 0xff; /* R */
               *dstImage++ = (tex >>  8) & 0xff; /* G */
               *dstImage++ = (tex      ) & 0xff; /* B */
               *dstImage++ = (tex >> 24) & 0xff; /* A */
            }
         }
         else {
            /* rescale */
            const GLushort *src16 = (const GLushort *) srcImage;
            GLint row, col;
            for (row = 0; row < dstHeight; row++) {
               GLint srcRow = row * hScale;
               for (col = 0; col < dstWidth; col++) {
                  GLint srcCol = col * wScale;
                  const GLuint tex = src16[srcRow * srcWidth + srcCol];
                  *dstImage++ = (tex >> 16) & 0xff; /* R */
                  *dstImage++ = (tex >>  8) & 0xff; /* G */
                  *dstImage++ = (tex      ) & 0xff; /* B */
                  *dstImage++ = (tex >> 24) & 0xff; /* A */
               }
            }
         }
         break;
      default:
         gl_problem(NULL, "bad srcFormat in _mesa_uncovert_teximage()");
   }
}

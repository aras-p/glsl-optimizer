/* $Id: texstore.c,v 1.23 2001/04/04 22:41:23 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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

/*
 * Authors:
 *   Brian Paul
 */


/*
 * The functions in this file are mostly related to software texture fallbacks.
 * This includes texture image transfer/packing and texel fetching.
 * Hardware drivers will likely override most of this.
 */



#include "colormac.h"
#include "context.h"
#include "convolve.h"
#include "image.h"
#include "macros.h"
#include "mem.h"
#include "texformat.h"
#include "teximage.h"
#include "texstore.h"
#include "texutil.h"


/*
 * Given an internal texture format enum or 1, 2, 3, 4 return the
 * corresponding _base_ internal format:  GL_ALPHA, GL_LUMINANCE,
 * GL_LUMANCE_ALPHA, GL_INTENSITY, GL_RGB, or GL_RGBA.  Return the
 * number of components for the format.  Return -1 if invalid enum.
 *
 * GH: Do we really need this?  We have the number of bytes per texel
 * in the texture format structures, so why don't we just use that?
 */
static GLint
components_in_intformat( GLint format )
{
   switch (format) {
      case GL_ALPHA:
      case GL_ALPHA4:
      case GL_ALPHA8:
      case GL_ALPHA12:
      case GL_ALPHA16:
         return 1;
      case 1:
      case GL_LUMINANCE:
      case GL_LUMINANCE4:
      case GL_LUMINANCE8:
      case GL_LUMINANCE12:
      case GL_LUMINANCE16:
         return 1;
      case 2:
      case GL_LUMINANCE_ALPHA:
      case GL_LUMINANCE4_ALPHA4:
      case GL_LUMINANCE6_ALPHA2:
      case GL_LUMINANCE8_ALPHA8:
      case GL_LUMINANCE12_ALPHA4:
      case GL_LUMINANCE12_ALPHA12:
      case GL_LUMINANCE16_ALPHA16:
         return 2;
      case GL_INTENSITY:
      case GL_INTENSITY4:
      case GL_INTENSITY8:
      case GL_INTENSITY12:
      case GL_INTENSITY16:
         return 1;
      case 3:
      case GL_RGB:
      case GL_R3_G3_B2:
      case GL_RGB4:
      case GL_RGB5:
      case GL_RGB8:
      case GL_RGB10:
      case GL_RGB12:
      case GL_RGB16:
         return 3;
      case 4:
      case GL_RGBA:
      case GL_RGBA2:
      case GL_RGBA4:
      case GL_RGB5_A1:
      case GL_RGBA8:
      case GL_RGB10_A2:
      case GL_RGBA12:
      case GL_RGBA16:
         return 4;
      case GL_COLOR_INDEX:
      case GL_COLOR_INDEX1_EXT:
      case GL_COLOR_INDEX2_EXT:
      case GL_COLOR_INDEX4_EXT:
      case GL_COLOR_INDEX8_EXT:
      case GL_COLOR_INDEX12_EXT:
      case GL_COLOR_INDEX16_EXT:
         return 1;
      case GL_DEPTH_COMPONENT:
      case GL_DEPTH_COMPONENT16_SGIX:
      case GL_DEPTH_COMPONENT24_SGIX:
      case GL_DEPTH_COMPONENT32_SGIX:
         return 1;
      default:
         return -1;  /* error */
   }
}


/*
 * This function is used to transfer the user's image data into a texture
 * image buffer.  We handle both full texture images and subtexture images.
 * We also take care of all image transfer operations here, including
 * convolution, scale/bias, colortables, etc.
 *
 * The destination texel channel type is always GLchan.
 *
 * A hardware driver may use this as a helper routine to unpack and
 * apply pixel transfer ops into a temporary image buffer.  Then,
 * convert the temporary image into the special hardware format.
 *
 * Input:
 *   dimensions - 1, 2, or 3
 *   texFormat - GL_LUMINANCE, GL_INTENSITY, GL_LUMINANCE_ALPHA, GL_ALPHA,
 *               GL_RGB or GL_RGBA
 *   texDestAddr - destination image address
 *   srcWidth, srcHeight, srcDepth - size (in pixels) of src and dest images
 *   dstXoffset, dstYoffset, dstZoffset - position to store the image within
 *      the destination 3D texture
 *   dstRowStride, dstImageStride - dest image strides in bytes
 *   srcFormat - source image format (GL_ALPHA, GL_RED, GL_RGB, etc)
 *   srcType - GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT_5_6_5, GL_FLOAT, etc
 *   srcPacking - describes packing of incoming image.
 */
static void
transfer_teximage(GLcontext *ctx, GLuint dimensions,
                  GLenum texDestFormat, GLvoid *texDestAddr,
                  GLint srcWidth, GLint srcHeight, GLint srcDepth,
                  GLint dstXoffset, GLint dstYoffset, GLint dstZoffset,
                  GLint dstRowStride, GLint dstImageStride,
                  GLenum srcFormat, GLenum srcType,
                  const GLvoid *srcAddr,
                  const struct gl_pixelstore_attrib *srcPacking)
{
   GLint texComponents;

   ASSERT(ctx);
   ASSERT(dimensions >= 1 && dimensions <= 3);
   ASSERT(texDestAddr);
   ASSERT(srcWidth >= 1);
   ASSERT(srcHeight >= 1);
   ASSERT(srcDepth >= 1);
   ASSERT(dstXoffset >= 0);
   ASSERT(dstYoffset >= 0);
   ASSERT(dstZoffset >= 0);
   ASSERT(dstRowStride >= 0);
   ASSERT(dstImageStride >= 0);
   ASSERT(srcAddr);
   ASSERT(srcPacking);

   texComponents = components_in_intformat(texDestFormat);

   /* try common 2D texture cases first */
   if (!ctx->_ImageTransferState && dimensions == 2 && srcType == CHAN_TYPE) {

      if (srcFormat == texDestFormat) {
         /* This will cover the common GL_RGB, GL_RGBA, GL_ALPHA,
          * GL_LUMINANCE_ALPHA, etc. texture formats.  Use memcpy().
          */
         const GLchan *src = (const GLchan *) _mesa_image_address(
                                   srcPacking, srcAddr, srcWidth, srcHeight,
                                   srcFormat, srcType, 0, 0, 0);
         const GLint srcRowStride = _mesa_image_row_stride(srcPacking,
                                               srcWidth, srcFormat, srcType);
         const GLint widthInBytes = srcWidth * texComponents * sizeof(GLchan);
         GLchan *dst = (GLchan *) texDestAddr + dstYoffset * dstRowStride
                      + dstXoffset * texComponents;
         if (srcRowStride == widthInBytes && dstRowStride == widthInBytes) {
            MEMCPY(dst, src, srcHeight * widthInBytes);
         }
         else {
            GLint i;
            for (i = 0; i < srcHeight; i++) {
               MEMCPY(dst, src, widthInBytes);
               src += srcRowStride;
               dst += dstRowStride;
            }
         }
         return;  /* all done */
      }
      else if (srcFormat == GL_RGBA && texDestFormat == GL_RGB) {
         /* commonly used by Quake */
         const GLchan *src = (const GLchan *) _mesa_image_address(
                                   srcPacking, srcAddr, srcWidth, srcHeight,
                                   srcFormat, srcType, 0, 0, 0);
         const GLint srcRowStride = _mesa_image_row_stride(srcPacking,
                                               srcWidth, srcFormat, srcType);
         GLchan *dst = (GLchan *) texDestAddr + dstYoffset * dstRowStride
                      + dstXoffset * texComponents;
         GLint i, j;
         for (i = 0; i < srcHeight; i++) {
            const GLchan *s = src;
            GLchan *d = dst;
            for (j = 0; j < srcWidth; j++) {
               *d++ = *s++;  /*red*/
               *d++ = *s++;  /*green*/
               *d++ = *s++;  /*blue*/
               s++;          /*alpha*/
            }
            src += srcRowStride;
            dst += dstRowStride;
         }
         return;  /* all done */
      }
   }

   /*
    * General case solutions
    */
   if (texDestFormat == GL_COLOR_INDEX) {
      /* color index texture */
      const GLenum texType = CHAN_TYPE;
      GLint img, row;
      GLchan *dest = (GLchan *) texDestAddr + dstZoffset * dstImageStride
                    + dstYoffset * dstRowStride
                    + dstXoffset * texComponents;
      for (img = 0; img < srcDepth; img++) {
         GLchan *destRow = dest;
         for (row = 0; row < srcHeight; row++) {
            const GLvoid *src = _mesa_image_address(srcPacking,
                srcAddr, srcWidth, srcHeight, srcFormat, srcType, img, row, 0);
            _mesa_unpack_index_span(ctx, srcWidth, texType, destRow,
                                    srcType, src, srcPacking,
                                    ctx->_ImageTransferState);
            destRow += dstRowStride;
         }
         dest += dstImageStride;
      }
   }
   else if (texDestFormat == GL_DEPTH_COMPONENT) {
      /* Depth texture (shadow maps) */
      GLint img, row;
      GLubyte *dest = (GLubyte *) texDestAddr
                    + dstZoffset * dstImageStride
                    + dstYoffset * dstRowStride
                    + dstXoffset * texComponents;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *destRow = dest;
         for (row = 0; row < srcHeight; row++) {
            const GLvoid *src = _mesa_image_address(srcPacking,
                srcAddr, srcWidth, srcHeight, srcFormat, srcType, img, row, 0);
            _mesa_unpack_depth_span(ctx, srcWidth, (GLfloat *) destRow,
                                    srcType, src, srcPacking);
            destRow += dstRowStride;
         }
         dest += dstImageStride;
      }
   }
   else {
      /* regular, color texture */
      if ((dimensions == 1 && ctx->Pixel.Convolution1DEnabled) ||
          (dimensions >= 2 && ctx->Pixel.Convolution2DEnabled) ||
          (dimensions >= 2 && ctx->Pixel.Separable2DEnabled)) {
         /*
          * Fill texture image with convolution
          */
         GLint img, row;
         GLint convWidth = srcWidth, convHeight = srcHeight;
         GLfloat *tmpImage, *convImage;
         tmpImage = (GLfloat *) MALLOC(srcWidth * srcHeight * 4 * sizeof(GLfloat));
         if (!tmpImage) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage");
            return;
         }
         convImage = (GLfloat *) MALLOC(srcWidth * srcHeight * 4 * sizeof(GLfloat));
         if (!convImage) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage");
            FREE(tmpImage);
            return;
         }

         for (img = 0; img < srcDepth; img++) {
            const GLfloat *srcf;
            GLfloat *dstf = tmpImage;
            GLchan *dest;

            /* unpack and do transfer ops up to convolution */
            for (row = 0; row < srcHeight; row++) {
               const GLvoid *src = _mesa_image_address(srcPacking,
                                              srcAddr, srcWidth, srcHeight,
                                              srcFormat, srcType, img, row, 0);
               _mesa_unpack_float_color_span(ctx, srcWidth, GL_RGBA, dstf,
                         srcFormat, srcType, src, srcPacking,
                         ctx->_ImageTransferState & IMAGE_PRE_CONVOLUTION_BITS,
                         GL_TRUE);
               dstf += srcWidth * 4;
            }

            /* convolve */
            if (dimensions == 1) {
               ASSERT(ctx->Pixel.Convolution1DEnabled);
               _mesa_convolve_1d_image(ctx, &convWidth, tmpImage, convImage);
            }
            else {
               if (ctx->Pixel.Convolution2DEnabled) {
                  _mesa_convolve_2d_image(ctx, &convWidth, &convHeight,
                                          tmpImage, convImage);
               }
               else {
                  ASSERT(ctx->Pixel.Separable2DEnabled);
                  _mesa_convolve_sep_image(ctx, &convWidth, &convHeight,
                                           tmpImage, convImage);
               }
            }

            /* packing and transfer ops after convolution */
            srcf = convImage;
            dest = (GLchan *) texDestAddr + (dstZoffset + img) * dstImageStride
                 + dstYoffset * dstRowStride;
            for (row = 0; row < convHeight; row++) {
               _mesa_pack_float_rgba_span(ctx, convWidth,
                                          (const GLfloat (*)[4]) srcf,
                                          texDestFormat, CHAN_TYPE,
                                          dest, &_mesa_native_packing,
                                          ctx->_ImageTransferState
                                          & IMAGE_POST_CONVOLUTION_BITS);
               srcf += convWidth * 4;
               dest += dstRowStride;
            }
         }

         FREE(convImage);
         FREE(tmpImage);
      }
      else {
         /*
          * no convolution
          */
         GLint img, row;
         GLchan *dest = (GLchan *) texDestAddr + dstZoffset * dstImageStride
                       + dstYoffset * dstRowStride
                       + dstXoffset * texComponents;
         for (img = 0; img < srcDepth; img++) {
            GLchan *destRow = dest;
            for (row = 0; row < srcHeight; row++) {
               const GLvoid *srcRow = _mesa_image_address(srcPacking,
                                              srcAddr, srcWidth, srcHeight,
                                              srcFormat, srcType, img, row, 0);
               _mesa_unpack_chan_color_span(ctx, srcWidth, texDestFormat,
                                       destRow, srcFormat, srcType, srcRow,
                                       srcPacking, ctx->_ImageTransferState);
               destRow += dstRowStride;
            }
            dest += dstImageStride;
         }
      }
   }
}



/*
 * Transfer a texture image from user space to <destAddr> applying all
 * needed image transfer operations and storing the result in the format
 * specified by <dstFormat>.  <dstFormat> may be any format from texformat.h.
 * Input:
 *   dstRowStride - stride between dest rows in bytes
 *   dstImagetride - stride between dest images in bytes
 *
 * XXX this function is a bit more complicated than it should be.  If
 * _mesa_convert_texsubimage[123]d could handle any dest/source formats
 * or if transfer_teximage() could store in any MESA_FORMAT_* format, we
 * could simplify things here.
 */
void
_mesa_transfer_teximage(GLcontext *ctx, GLuint dimensions,
                        const struct gl_texture_format *dstFormat,
                        GLvoid *dstAddr,
                        GLint srcWidth, GLint srcHeight, GLint srcDepth,
                        GLint dstXoffset, GLint dstYoffset, GLint dstZoffset,
                        GLint dstRowStride, GLint dstImageStride,
                        GLenum srcFormat, GLenum srcType,
                        const GLvoid *srcAddr,
                        const struct gl_pixelstore_attrib *srcPacking)
{
   const GLint dstRowStridePixels = dstRowStride / dstFormat->TexelBytes;
   const GLint dstImageStridePixels = dstImageStride / dstFormat->TexelBytes;
   GLboolean makeTemp;

   /* First, determine if need to make a temporary, intermediate image */
   if (_mesa_is_hardware_tex_format(dstFormat)) {
      if (ctx->_ImageTransferState) {
         makeTemp = GL_TRUE;
      }
      else {
         if (dimensions == 1) {
            makeTemp = !_mesa_convert_texsubimage1d(dstFormat->MesaFormat,
                                                    dstXoffset,
                                                    srcWidth,
                                                    srcFormat, srcType,
                                                    srcPacking, srcAddr,
                                                    dstAddr);
         }
         else if (dimensions == 2) {
            makeTemp = !_mesa_convert_texsubimage2d(dstFormat->MesaFormat,
                                                    dstXoffset, dstYoffset,
                                                    srcWidth, srcHeight,
                                                    dstRowStridePixels,
                                                    srcFormat, srcType,
                                                    srcPacking, srcAddr,
                                                    dstAddr);
         }
         else {
            assert(dimensions == 3);
            makeTemp = !_mesa_convert_texsubimage3d(dstFormat->MesaFormat,
                                      dstXoffset, dstYoffset, dstZoffset,
                                      srcWidth, srcHeight, srcDepth,
                                      dstRowStridePixels, dstImageStridePixels,
                                      srcFormat, srcType,
                                      srcPacking, srcAddr, dstAddr);
         }
         if (!makeTemp) {
            /* all done! */
            return;
         }
      }
   }
   else {
      /* software texture format */
      makeTemp = GL_FALSE;
   }

   if (makeTemp) {
      GLint postConvWidth = srcWidth, postConvHeight = srcHeight;
      GLenum tmpFormat;
      GLuint tmpComps, tmpTexelSize;
      GLint tmpRowStride, tmpImageStride;
      GLubyte *tmpImage;

      if (ctx->_ImageTransferState & IMAGE_CONVOLUTION_BIT) {
         _mesa_adjust_image_for_convolution(ctx, dimensions, &postConvWidth,
                                            &postConvHeight);
      }

      tmpFormat = _mesa_base_tex_format(ctx, dstFormat->IntFormat);
      tmpComps = _mesa_components_in_format(tmpFormat);
      tmpTexelSize = tmpComps * sizeof(GLchan);
      tmpRowStride = postConvWidth * tmpTexelSize;
      tmpImageStride = postConvWidth * postConvHeight * tmpTexelSize;
      tmpImage = (GLubyte *) MALLOC(postConvWidth * postConvHeight *
                                    srcDepth * tmpTexelSize);
      if (!tmpImage)
         return;

      transfer_teximage(ctx, dimensions, tmpFormat, tmpImage,
                        srcWidth, srcHeight, srcDepth,
                        0, 0, 0, /* x/y/zoffset */
                        tmpRowStride, tmpImageStride,
                        srcFormat, srcType, srcAddr, srcPacking);

      /* the temp image is our new source image */
      srcWidth = postConvWidth;
      srcHeight = postConvHeight;
      srcFormat = tmpFormat;
      srcType = CHAN_TYPE;
      srcAddr = tmpImage;
      srcPacking = &_mesa_native_packing;
   }

   if (_mesa_is_hardware_tex_format(dstFormat)) {
      assert(makeTemp);
      if (dimensions == 1) {
         GLboolean b;
         b = _mesa_convert_texsubimage1d(dstFormat->MesaFormat,
                                         dstXoffset,
                                         srcWidth,
                                         srcFormat, srcType,
                                         srcPacking, srcAddr,
                                         dstAddr);
         assert(b);
      }
      else if (dimensions == 2) {
         GLboolean b;
         b = _mesa_convert_texsubimage2d(dstFormat->MesaFormat,
                                         dstXoffset, dstYoffset,
                                         srcWidth, srcHeight,
                                         dstRowStridePixels,
                                         srcFormat, srcType,
                                         srcPacking, srcAddr,
                                         dstAddr);
         assert(b);
      }
      else {
         GLboolean b;
         b = _mesa_convert_texsubimage3d(dstFormat->MesaFormat,
                                      dstXoffset, dstYoffset, dstZoffset,
                                      srcWidth, srcHeight, srcDepth,
                                      dstRowStridePixels, dstImageStridePixels,
                                      srcFormat, srcType,
                                      srcPacking, srcAddr, dstAddr);
         assert(b);
      }
      FREE((void *) srcAddr);  /* the temp image */
   }
   else {
      /* software format */
      GLenum dstBaseFormat = _mesa_base_tex_format(ctx, dstFormat->IntFormat);
      assert(!makeTemp);
      transfer_teximage(ctx, dimensions, dstBaseFormat, dstAddr,
                        srcWidth, srcHeight, srcDepth,
                        dstXoffset, dstYoffset, dstZoffset,
                        dstRowStride, dstImageStride,
                        srcFormat, srcType, srcAddr, srcPacking);
   }
}


/*
 * This is the software fallback for Driver.TexImage1D().
 * The texture image type will be GLchan.
 * The texture image format will be GL_COLOR_INDEX, GL_INTENSITY,
 * GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_ALPHA, GL_RGB or GL_RGBA.
 *
 */
void
_mesa_store_teximage1d(GLcontext *ctx, GLenum target, GLint level,
                       GLint internalFormat,
                       GLint width, GLint border,
                       GLenum format, GLenum type, const GLvoid *pixels,
                       const struct gl_pixelstore_attrib *packing,
                       struct gl_texture_object *texObj,
                       struct gl_texture_image *texImage)
{
   GLint postConvWidth = width;
   GLint texelBytes;

   if (ctx->_ImageTransferState & IMAGE_CONVOLUTION_BIT) {
      _mesa_adjust_image_for_convolution(ctx, 1, &postConvWidth, NULL);
   }

   /* choose the texture format */
   assert(ctx->Driver.ChooseTextureFormat);
   texImage->TexFormat = (*ctx->Driver.ChooseTextureFormat)(ctx,
                                          internalFormat, format, type);
   assert(texImage->TexFormat);

   texelBytes = texImage->TexFormat->TexelBytes;

   /* allocate memory */
   texImage->Data = MALLOC(postConvWidth * texelBytes);
   if (!texImage->Data) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage1D");
      return;
   }

   /* unpack image, apply transfer ops and store in texImage->Data */
   _mesa_transfer_teximage(ctx, 1, texImage->TexFormat, texImage->Data,
                           width, 1, 1, 0, 0, 0,
                           0, /* dstRowStride */
                           0, /* dstImageStride */
                           format, type, pixels, packing);
}


/*
 * This is the software fallback for Driver.TexImage2D().
 * The texture image type will be GLchan.
 * The texture image format will be GL_COLOR_INDEX, GL_INTENSITY,
 * GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_ALPHA, GL_RGB or GL_RGBA.
 *
 */
void
_mesa_store_teximage2d(GLcontext *ctx, GLenum target, GLint level,
                       GLint internalFormat,
                       GLint width, GLint height, GLint border,
                       GLenum format, GLenum type, const void *pixels,
                       const struct gl_pixelstore_attrib *packing,
                       struct gl_texture_object *texObj,
                       struct gl_texture_image *texImage)
{
   GLint postConvWidth = width, postConvHeight = height;
   GLint texelBytes;

   if (ctx->_ImageTransferState & IMAGE_CONVOLUTION_BIT) {
      _mesa_adjust_image_for_convolution(ctx, 2, &postConvWidth,
                                         &postConvHeight);
   }

   /* choose the texture format */
   assert(ctx->Driver.ChooseTextureFormat);
   texImage->TexFormat = (*ctx->Driver.ChooseTextureFormat)(ctx,
                                          internalFormat, format, type);
   assert(texImage->TexFormat);

   texelBytes = texImage->TexFormat->TexelBytes;

   /* allocate memory */
   texImage->Data = MALLOC(postConvWidth * postConvHeight * texelBytes);
   if (!texImage->Data) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage2D");
      return;
   }

   /* unpack image, apply transfer ops and store in texImage->Data */
   _mesa_transfer_teximage(ctx, 2, texImage->TexFormat, texImage->Data,
                           width, height, 1, 0, 0, 0,
                           texImage->Width * texelBytes,
                           0, /* dstImageStride */
                           format, type, pixels, packing);
}



/*
 * This is the software fallback for Driver.TexImage3D().
 * The texture image type will be GLchan.
 * The texture image format will be GL_COLOR_INDEX, GL_INTENSITY,
 * GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_ALPHA, GL_RGB or GL_RGBA.
 *
 */
void
_mesa_store_teximage3d(GLcontext *ctx, GLenum target, GLint level,
                       GLint internalFormat,
                       GLint width, GLint height, GLint depth, GLint border,
                       GLenum format, GLenum type, const void *pixels,
                       const struct gl_pixelstore_attrib *packing,
                       struct gl_texture_object *texObj,
                       struct gl_texture_image *texImage)
{
   GLint texelBytes;

   /* choose the texture format */
   assert(ctx->Driver.ChooseTextureFormat);
   texImage->TexFormat = (*ctx->Driver.ChooseTextureFormat)(ctx,
                                          internalFormat, format, type);
   assert(texImage->TexFormat);

   texelBytes = texImage->TexFormat->TexelBytes;

   /* allocate memory */
   texImage->Data = MALLOC(width * height * depth * texelBytes);
   if (!texImage->Data) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage3D");
      return;
   }

   /* unpack image, apply transfer ops and store in texImage->Data */
   _mesa_transfer_teximage(ctx, 3, texImage->TexFormat, texImage->Data,
                           width, height, depth, 0, 0, 0,
                           texImage->Width * texelBytes,
                           texImage->Width * texImage->Height * texelBytes,
                           format, type, pixels, packing);
}




/*
 * This is the software fallback for Driver.TexSubImage1D().
 */
void
_mesa_store_texsubimage1d(GLcontext *ctx, GLenum target, GLint level,
                          GLint xoffset, GLint width,
                          GLenum format, GLenum type, const void *pixels,
                          const struct gl_pixelstore_attrib *packing,
                          struct gl_texture_object *texObj,
                          struct gl_texture_image *texImage)
{
   _mesa_transfer_teximage(ctx, 1, texImage->TexFormat, texImage->Data,
                           width, 1, 1, /* src size */
                           xoffset, 0, 0, /* dest offsets */
                           0, /* dstRowStride */
                           0, /* dstImageStride */
                           format, type, pixels, packing);
}


/*
 * This is the software fallback for Driver.TexSubImage2D().
 */
void
_mesa_store_texsubimage2d(GLcontext *ctx, GLenum target, GLint level,
                          GLint xoffset, GLint yoffset,
                          GLint width, GLint height,
                          GLenum format, GLenum type, const void *pixels,
                          const struct gl_pixelstore_attrib *packing,
                          struct gl_texture_object *texObj,
                          struct gl_texture_image *texImage)
{
   _mesa_transfer_teximage(ctx, 2, texImage->TexFormat, texImage->Data,
                           width, height, 1, /* src size */
                           xoffset, yoffset, 0, /* dest offsets */
                           texImage->Width * texImage->TexFormat->TexelBytes,
                           0, /* dstImageStride */
                           format, type, pixels, packing);
}


/*
 * This is the software fallback for Driver.TexSubImage3D().
 */
void
_mesa_store_texsubimage3d(GLcontext *ctx, GLenum target, GLint level,
                          GLint xoffset, GLint yoffset, GLint zoffset,
                          GLint width, GLint height, GLint depth,
                          GLenum format, GLenum type, const void *pixels,
                          const struct gl_pixelstore_attrib *packing,
                          struct gl_texture_object *texObj,
                          struct gl_texture_image *texImage)
{
   const GLint texelBytes = texImage->TexFormat->TexelBytes;
   _mesa_transfer_teximage(ctx, 3, texImage->TexFormat, texImage->Data,
                           width, height, depth, /* src size */
                           xoffset, yoffset, xoffset, /* dest offsets */
                           texImage->Width * texelBytes,
                           texImage->Width * texImage->Height * texelBytes,
                           format, type, pixels, packing);
}




/*
 * Fallback for Driver.CompressedTexImage1D()
 */
void
_mesa_store_compressed_teximage1d(GLcontext *ctx, GLenum target, GLint level,
                                  GLint internalFormat,
                                  GLint width, GLint border,
                                  GLsizei imageSize, const GLvoid *data,
                                  struct gl_texture_object *texObj,
                                  struct gl_texture_image *texImage)
{
   /* Nothing here.
    * The device driver has to do it all.
    */
}



/*
 * Fallback for Driver.CompressedTexImage2D()
 */
void
_mesa_store_compressed_teximage2d(GLcontext *ctx, GLenum target, GLint level,
                                  GLint internalFormat,
                                  GLint width, GLint height, GLint border,
                                  GLsizei imageSize, const GLvoid *data,
                                  struct gl_texture_object *texObj,
                                  struct gl_texture_image *texImage)
{
   /* Nothing here.
    * The device driver has to do it all.
    */
}



/*
 * Fallback for Driver.CompressedTexImage3D()
 */
void
_mesa_store_compressed_teximage3d(GLcontext *ctx, GLenum target, GLint level,
                                  GLint internalFormat,
                                  GLint width, GLint height, GLint depth,
                                  GLint border,
                                  GLsizei imageSize, const GLvoid *data,
                                  struct gl_texture_object *texObj,
                                  struct gl_texture_image *texImage)
{
   /* Nothing here.
    * The device driver has to do it all.
    */
}



/*
 * This is the fallback for Driver.TestProxyTexImage().
 */
GLboolean
_mesa_test_proxy_teximage(GLcontext *ctx, GLenum target, GLint level,
                          GLint internalFormat, GLenum format, GLenum type,
                          GLint width, GLint height, GLint depth, GLint border)
{
   struct gl_texture_unit *texUnit;
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;

   (void) format;
   (void) type;

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   texObj = _mesa_select_tex_object(ctx, texUnit, target);
   texImage = _mesa_select_tex_image(ctx, texUnit, target, level);

   /* We always pass.
    * The core Mesa code will have already tested the image size, etc.
    * Drivers may have more stringent texture limits to enforce and will
    * have to override this function.
    */
   /* choose the texture format */
   assert(ctx->Driver.ChooseTextureFormat);
   texImage->TexFormat = (*ctx->Driver.ChooseTextureFormat)(ctx,
                                          internalFormat, format, type);
   assert(texImage->TexFormat);

   return GL_TRUE;
}

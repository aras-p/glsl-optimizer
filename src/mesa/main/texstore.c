/* $Id: texstore.c,v 1.14 2001/03/18 14:05:32 gareth Exp $ */

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
#include "teximage.h"
#include "texstore.h"
#include "swrast/s_depth.h"  /* XXX this is kind of a cheat */
#include "swrast/s_span.h"


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
 *   texAddr - destination image address
 *   srcWidth, srcHeight, srcDepth - size (in pixels) of src and dest images
 *   dstXoffset, dstYoffset, dstZoffset - position to store the image within
 *      the destination 3D texture
 *   dstRowStride, dstImageStride - dest image strides in GLchan's
 *   srcFormat - source image format (GL_ALPHA, GL_RED, GL_RGB, etc)
 *   srcType - GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT_5_6_5, GL_FLOAT, etc
 *   srcPacking - describes packing of incoming image.
 */
void
_mesa_transfer_teximage(GLcontext *ctx, GLuint dimensions,
                        GLenum texFormat, GLchan *texAddr,
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
   ASSERT(texAddr);
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

   texComponents = components_in_intformat(texFormat);

   /* try common 2D texture cases first */
   if (!ctx->_ImageTransferState && dimensions == 2 && srcType == CHAN_TYPE) {

      if (srcFormat == texFormat) {
         /* This will cover the common GL_RGB, GL_RGBA, GL_ALPHA,
          * GL_LUMINANCE_ALPHA, etc. texture formats.  Use memcpy().
          */
         const GLchan *src = (const GLchan *) _mesa_image_address(
                                   srcPacking, srcAddr, srcWidth, srcHeight,
                                   srcFormat, srcType, 0, 0, 0);
         const GLint srcRowStride = _mesa_image_row_stride(srcPacking,
                                               srcWidth, srcFormat, srcType);
         const GLint widthInBytes = srcWidth * texComponents * sizeof(GLchan);
         GLchan *dst = texAddr + dstYoffset * dstRowStride
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
      else if (srcFormat == GL_RGBA && texFormat == GL_RGB) {
         /* commonly used by Quake */
         const GLchan *src = (const GLchan *) _mesa_image_address(
                                   srcPacking, srcAddr, srcWidth, srcHeight,
                                   srcFormat, srcType, 0, 0, 0);
         const GLint srcRowStride = _mesa_image_row_stride(srcPacking,
                                               srcWidth, srcFormat, srcType);
         GLchan *dst = texAddr + dstYoffset * dstRowStride
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
   if (texFormat == GL_COLOR_INDEX) {
      /* color index texture */
      const GLenum texType = CHAN_TYPE;
      GLint img, row;
      GLchan *dest = texAddr + dstZoffset * dstImageStride
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
   else if (texFormat == GL_DEPTH_COMPONENT) {
      /* Depth texture (shadow maps) */
      GLint img, row;
      GLfloat *dest = (GLfloat *) texAddr + dstZoffset * dstImageStride
                    + dstYoffset * dstRowStride
                    + dstXoffset * texComponents;
      for (img = 0; img < srcDepth; img++) {
         GLfloat *destRow = dest;
         for (row = 0; row < srcHeight; row++) {
            const GLvoid *src = _mesa_image_address(srcPacking,
                srcAddr, srcWidth, srcHeight, srcFormat, srcType, img, row, 0);
            _mesa_unpack_depth_span(ctx, srcWidth, destRow,
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
            dest = texAddr + (dstZoffset + img) * dstImageStride
                 + dstYoffset * dstRowStride;
            for (row = 0; row < convHeight; row++) {
               _mesa_pack_float_rgba_span(ctx, convWidth,
                                          (const GLfloat (*)[4]) srcf,
                                          texFormat, CHAN_TYPE,
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
         GLchan *dest = texAddr + dstZoffset * dstImageStride
                       + dstYoffset * dstRowStride
                       + dstXoffset * texComponents;
         for (img = 0; img < srcDepth; img++) {
            GLchan *destRow = dest;
            for (row = 0; row < srcHeight; row++) {
               const GLvoid *srcRow = _mesa_image_address(srcPacking,
                                              srcAddr, srcWidth, srcHeight,
                                              srcFormat, srcType, img, row, 0);
               _mesa_unpack_chan_color_span(ctx, srcWidth, texFormat, destRow,
                                       srcFormat, srcType, srcRow, srcPacking,
                                       ctx->_ImageTransferState);
               destRow += dstRowStride;
            }
            dest += dstImageStride;
         }
      }
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

   if (ctx->_ImageTransferState & IMAGE_CONVOLUTION_BIT) {
      _mesa_adjust_image_for_convolution(ctx, 1, &postConvWidth, NULL);
   }

   /* setup the teximage struct's fields */
   _mesa_init_tex_format( ctx, internalFormat, texImage );
   texImage->FetchTexel = texImage->TexFormat->FetchTexel1D;

   /* allocate memory */
   texImage->Data = (GLchan *) MALLOC(postConvWidth *
				      texImage->TexFormat->TexelBytes);
   if (!texImage->Data)
      return;      /* out of memory */

   /* unpack image, apply transfer ops and store in texImage->Data */
   _mesa_transfer_teximage(ctx, 1, texImage->Format, (GLchan *) texImage->Data,
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

   /* setup the teximage struct's fields */
   _mesa_init_tex_format( ctx, internalFormat, texImage );
   texImage->FetchTexel = texImage->TexFormat->FetchTexel2D;

   texelBytes = texImage->TexFormat->TexelBytes;

   /* allocate memory */
   texImage->Data = (GLchan *) MALLOC(postConvWidth * postConvHeight *
                                      texelBytes);
   if (!texImage->Data)
      return;      /* out of memory */

   /* unpack image, apply transfer ops and store in texImage->Data */
   _mesa_transfer_teximage(ctx, 2, texImage->Format, (GLchan *) texImage->Data,
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

   /* setup the teximage struct's fields */
   _mesa_init_tex_format( ctx, internalFormat, texImage );
   texImage->FetchTexel = texImage->TexFormat->FetchTexel3D;

   texelBytes = texImage->TexFormat->TexelBytes;

   /* allocate memory */
   texImage->Data = (GLchan *) MALLOC(width * height * depth * texelBytes);
   if (!texImage->Data)
      return;      /* out of memory */

   /* unpack image, apply transfer ops and store in texImage->Data */
   _mesa_transfer_teximage(ctx, 3, texImage->Format, (GLchan *) texImage->Data,
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
   _mesa_transfer_teximage(ctx, 1, texImage->Format, (GLchan *) texImage->Data,
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
   const GLint components = components_in_intformat(texImage->IntFormat);
   const GLint compSize = _mesa_sizeof_type(texImage->Type);
   _mesa_transfer_teximage(ctx, 2, texImage->Format, (GLchan *) texImage->Data,
                           width, height, 1, /* src size */
                           xoffset, yoffset, 0, /* dest offsets */
                           texImage->Width * components * compSize,
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
   const GLint components = components_in_intformat(texImage->IntFormat);
   const GLint compSize = _mesa_sizeof_type(texImage->Type);
   _mesa_transfer_teximage(ctx, 3, texImage->Format, (GLchan *) texImage->Data,
                           width, height, depth, /* src size */
                           xoffset, yoffset, xoffset, /* dest offsets */
                           texImage->Width * components * compSize,
                           texImage->Width * texImage->Height * components
                           * compSize,
                           format, type, pixels, packing);
}




/*
 * Read an RGBA image from the frame buffer.
 * This is used by glCopyTex[Sub]Image[12]D().
 * Input:  ctx - the context
 *         x, y - lower left corner
 *         width, height - size of region to read
 * Return: pointer to block of GL_RGBA, GLchan data.
 */
static GLchan *
read_color_image( GLcontext *ctx, GLint x, GLint y,
                  GLsizei width, GLsizei height )
{
   GLint stride, i;
   GLchan *image, *dst;

   image = (GLchan *) MALLOC(width * height * 4 * sizeof(GLchan));
   if (!image)
      return NULL;

   /* Select buffer to read from */
   (*ctx->Driver.SetReadBuffer)( ctx, ctx->ReadBuffer,
                                 ctx->Pixel.DriverReadBuffer );

   RENDER_START(ctx);

   dst = image;
   stride = width * 4;
   for (i = 0; i < height; i++) {
      _mesa_read_rgba_span( ctx, ctx->ReadBuffer, width, x, y + i,
                         (GLchan (*)[4]) dst );
      dst += stride;
   }

   RENDER_FINISH(ctx);

   /* Read from draw buffer (the default) */
   (*ctx->Driver.SetReadBuffer)( ctx, ctx->DrawBuffer,
                                 ctx->Color.DriverDrawBuffer );

   return image;
}


/*
 * As above, but read data from depth buffer.
 */
static GLfloat *
read_depth_image( GLcontext *ctx, GLint x, GLint y,
                  GLsizei width, GLsizei height )
{
   GLint i;
   GLfloat *image, *dst;

   image = (GLfloat *) MALLOC(width * height * sizeof(GLfloat));
   if (!image)
      return NULL;

   RENDER_START(ctx);

   dst = image;
   for (i = 0; i < height; i++) {
      _mesa_read_depth_span_float(ctx, width, x, y + i, dst);
      dst += width;
   }

   RENDER_FINISH(ctx);

   return image;
}



static GLboolean
is_depth_format(GLenum format)
{
   switch (format) {
      case GL_DEPTH_COMPONENT:
      case GL_DEPTH_COMPONENT16_SGIX:
      case GL_DEPTH_COMPONENT24_SGIX:
      case GL_DEPTH_COMPONENT32_SGIX:
         return GL_TRUE;
      default:
         return GL_FALSE;
   }
}


/*
 * Fallback for Driver.CopyTexImage1D().
 */
void
_mesa_copy_teximage1d( GLcontext *ctx, GLenum target, GLint level,
                       GLenum internalFormat,
                       GLint x, GLint y, GLsizei width, GLint border )
{
   struct gl_texture_unit *texUnit;
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   texObj = _mesa_select_tex_object(ctx, texUnit, target);
   ASSERT(texObj);
   texImage = _mesa_select_tex_image(ctx, texUnit, target, level);
   ASSERT(texImage);

   ASSERT(ctx->Driver.TexImage1D);

   if (is_depth_format(internalFormat)) {
      /* read depth image from framebuffer */
      GLfloat *image = read_depth_image(ctx, x, y, width, 1);
      if (!image) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyTexImage1D");
         return;
      }

      /* call glTexImage1D to redefine the texture */
      (*ctx->Driver.TexImage1D)(ctx, target, level, internalFormat,
                                width, border,
                                GL_DEPTH_COMPONENT, GL_FLOAT, image,
                                &_mesa_native_packing, texObj, texImage);
      FREE(image);
   }
   else {
      /* read RGBA image from framebuffer */
      GLchan *image = read_color_image(ctx, x, y, width, 1);
      if (!image) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyTexImage1D");
         return;
      }

      /* call glTexImage1D to redefine the texture */
      (*ctx->Driver.TexImage1D)(ctx, target, level, internalFormat,
                                width, border,
                                GL_RGBA, CHAN_TYPE, image,
                                &_mesa_native_packing, texObj, texImage);
      FREE(image);
   }
}


/*
 * Fallback for Driver.CopyTexImage2D().
 */
void
_mesa_copy_teximage2d( GLcontext *ctx, GLenum target, GLint level,
                       GLenum internalFormat,
                       GLint x, GLint y, GLsizei width, GLsizei height,
                       GLint border )
{
   struct gl_texture_unit *texUnit;
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   texObj = _mesa_select_tex_object(ctx, texUnit, target);
   ASSERT(texObj);
   texImage = _mesa_select_tex_image(ctx, texUnit, target, level);
   ASSERT(texImage);

   ASSERT(ctx->Driver.TexImage2D);

   if (is_depth_format(internalFormat)) {
      /* read depth image from framebuffer */
      GLfloat *image = read_depth_image(ctx, x, y, width, height);
      if (!image) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyTexImage2D");
         return;
      }

      /* call glTexImage2D to redefine the texture */
      (*ctx->Driver.TexImage2D)(ctx, target, level, internalFormat,
                                width, height, border,
                                GL_DEPTH_COMPONENT, GL_FLOAT, image,
                                &_mesa_native_packing, texObj, texImage);
      FREE(image);
   }
   else {
      /* read RGBA image from framebuffer */
      GLchan *image = read_color_image(ctx, x, y, width, height);
      if (!image) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyTexImage2D");
         return;
      }

      /* call glTexImage2D to redefine the texture */
      (*ctx->Driver.TexImage2D)(ctx, target, level, internalFormat,
                                width, height, border,
                                GL_RGBA, CHAN_TYPE, image,
                                &_mesa_native_packing, texObj, texImage);
      FREE(image);
   }
}


/*
 * Fallback for Driver.CopyTexSubImage1D().
 */
void
_mesa_copy_texsubimage1d(GLcontext *ctx, GLenum target, GLint level,
                         GLint xoffset, GLint x, GLint y, GLsizei width)
{
   struct gl_texture_unit *texUnit;
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   texObj = _mesa_select_tex_object(ctx, texUnit, target);
   ASSERT(texObj);
   texImage = _mesa_select_tex_image(ctx, texUnit, target, level);
   ASSERT(texImage);

   ASSERT(ctx->Driver.TexImage1D);

   if (is_depth_format(texImage->IntFormat)) {
      /* read depth image from framebuffer */
      GLfloat *image = read_depth_image(ctx, x, y, width, 1);
      if (!image) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyTexSubImage1D");
         return;
      }

      /* call glTexImage1D to redefine the texture */
      (*ctx->Driver.TexSubImage1D)(ctx, target, level, xoffset, width,
                                   GL_DEPTH_COMPONENT, GL_FLOAT, image,
                                   &_mesa_native_packing, texObj, texImage);
      FREE(image);
   }
   else {
      GLchan *image = read_color_image(ctx, x, y, width, 1);
      if (!image) {
         _mesa_error( ctx, GL_OUT_OF_MEMORY, "glCopyTexSubImage1D" );
         return;
      }

      /* now call glTexSubImage1D to do the real work */
      (*ctx->Driver.TexSubImage1D)(ctx, target, level, xoffset, width,
                                   GL_RGBA, CHAN_TYPE, image,
                                   &_mesa_native_packing, texObj, texImage);
      FREE(image);
   }
}


/*
 * Fallback for Driver.CopyTexSubImage2D().
 */
void
_mesa_copy_texsubimage2d( GLcontext *ctx,
                          GLenum target, GLint level,
                          GLint xoffset, GLint yoffset,
                          GLint x, GLint y, GLsizei width, GLsizei height )
{
   struct gl_texture_unit *texUnit;
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   texObj = _mesa_select_tex_object(ctx, texUnit, target);
   ASSERT(texObj);
   texImage = _mesa_select_tex_image(ctx, texUnit, target, level);
   ASSERT(texImage);

   ASSERT(ctx->Driver.TexImage2D);

   if (is_depth_format(texImage->IntFormat)) {
      /* read depth image from framebuffer */
      GLfloat *image = read_depth_image(ctx, x, y, width, height);
      if (!image) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyTexSubImage2D");
         return;
      }

      /* call glTexImage1D to redefine the texture */
      (*ctx->Driver.TexSubImage2D)(ctx, target, level,
                                   xoffset, yoffset, width, height,
                                   GL_DEPTH_COMPONENT, GL_FLOAT, image,
                                   &_mesa_native_packing, texObj, texImage);
      FREE(image);
   }
   else {
      /* read RGBA image from framebuffer */
      GLchan *image = read_color_image(ctx, x, y, width, height);
      if (!image) {
         _mesa_error( ctx, GL_OUT_OF_MEMORY, "glCopyTexSubImage2D" );
         return;
      }

      /* now call glTexSubImage2D to do the real work */
      (*ctx->Driver.TexSubImage2D)(ctx, target, level,
                                   xoffset, yoffset, width, height,
                                   GL_RGBA, CHAN_TYPE, image,
                                   &_mesa_native_packing, texObj, texImage);
      FREE(image);
   }
}


/*
 * Fallback for Driver.CopyTexSubImage3D().
 */
void
_mesa_copy_texsubimage3d( GLcontext *ctx,
                          GLenum target, GLint level,
                          GLint xoffset, GLint yoffset, GLint zoffset,
                          GLint x, GLint y, GLsizei width, GLsizei height )
{
   struct gl_texture_unit *texUnit;
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   texObj = _mesa_select_tex_object(ctx, texUnit, target);
   ASSERT(texObj);
   texImage = _mesa_select_tex_image(ctx, texUnit, target, level);
   ASSERT(texImage);

   ASSERT(ctx->Driver.TexImage3D);

   if (is_depth_format(texImage->IntFormat)) {
      /* read depth image from framebuffer */
      GLfloat *image = read_depth_image(ctx, x, y, width, height);
      if (!image) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyTexSubImage3D");
         return;
      }

      /* call glTexImage1D to redefine the texture */
      (*ctx->Driver.TexSubImage3D)(ctx, target, level,
                                   xoffset, yoffset, zoffset, width, height, 1,
                                   GL_DEPTH_COMPONENT, GL_FLOAT, image,
                                   &_mesa_native_packing, texObj, texImage);
      FREE(image);
   }
   else {
      /* read RGBA image from framebuffer */
      GLchan *image = read_color_image(ctx, x, y, width, height);
      if (!image) {
         _mesa_error( ctx, GL_OUT_OF_MEMORY, "glCopyTexSubImage3D" );
         return;
      }

      /* now call glTexSubImage3D to do the real work */
      (*ctx->Driver.TexSubImage3D)(ctx, target, level,
                                   xoffset, yoffset, zoffset, width, height, 1,
                                   GL_RGBA, CHAN_TYPE, image,
                                   &_mesa_native_packing, texObj, texImage);
      FREE(image);
   }
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
   /* setup the teximage struct's fields */
   _mesa_init_tex_format( ctx, internalFormat, texImage );

   return GL_TRUE;
}

/*
 * Mesa 3-D graphics library
 * Version:  6.1
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
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
 * The GL texture image functions in teximage.c basically just do
 * error checking and data structure allocation.  They in turn call
 * device driver functions which actually copy/convert/store the user's
 * texture image data.
 *
 * However, most device drivers will be able to use the fallback functions
 * in this file.  That is, most drivers will have the following bit of
 * code:
 *   ctx->Driver.TexImage1D = _mesa_store_teximage1d;
 *   ctx->Driver.TexImage2D = _mesa_store_teximage2d;
 *   ctx->Driver.TexImage3D = _mesa_store_teximage3d;
 *   etc...
 *
 * Texture image processing is actually kind of complicated.  We have to do:
 *    Format/type conversions
 *    pixel unpacking
 *    pixel transfer (scale, bais, lookup, convolution!, etc)
 *
 * These functions can handle most everything, including processing full
 * images and sub-images.
 */


#include "glheader.h"
#include "bufferobj.h"
#include "colormac.h"
#include "context.h"
#include "convolve.h"
#include "image.h"
#include "macros.h"
#include "imports.h"
#include "texcompress.h"
#include "texformat.h"
#include "teximage.h"
#include "texstore.h"
#include "texutil.h"



/*** NEWTEXSTORE ***********************************************************/

static const GLint ZERO = 1000, ONE = 1001;

/**
 * When promoting texture formats (see below) we need to compute the
 * mapping of dest components back to source components.
 * This function does that.
 * \param logicalBaseFormat  the logical format of the texture
 * \param textureBaseFormat  the final texture format
 * \return map[4]  the four mapping values
 */
static void
compute_component_mapping(GLenum logicalBaseFormat, GLenum textureBaseFormat,
                          GLint map[4])
{
   /* compute mapping from dest components back to src components */
   switch (logicalBaseFormat) {
   case GL_LUMINANCE:
      map[0] = map[1] = map[2] = 0;
      if (textureBaseFormat == GL_RGBA)
         map[3] = ONE;
      break;
   case GL_ALPHA:
      ASSERT(textureBaseFormat == GL_RGBA);
      map[0] = map[1] = map[2] = ZERO;
      map[3] = 0;
      break;
   case GL_INTENSITY:
      map[0] = map[1] = map[2] = 0;
      if (textureBaseFormat == GL_RGBA)
         map[3] = 0;
      break;
   case GL_LUMINANCE_ALPHA:
      ASSERT(textureBaseFormat == GL_RGBA);
      map[0] = map[1] = map[2] = 0;
      map[3] = 1;
      break;
   case GL_RGB:
      ASSERT(textureBaseFormat == GL_RGBA);
      map[0] = 0;
      map[1] = 1;
      map[2] = 2;
      map[3] = ONE;
      break;
   default:
      _mesa_problem(NULL, "Unexpected logicalBaseFormat");
      map[0] = map[1] = map[2] = map[3] = 0;
   }
}


/**
 * Make a temporary (color) texture image with GLfloat components.
 * Apply all needed pixel unpacking and pixel transfer operations.
 * Note that there are both logicalBaseFormat and textureBaseFormat parameters.
 * Suppose the user specifies GL_LUMINANCE as the internal texture format
 * but the graphics hardware doesn't support luminance textures.  So, might
 * use an RGB hardware format instead.
 * If logicalBaseFormat != textureBaseFormat we have some extra work to do.
 *
 * \param ctx  the rendering context
 * \param dims  image dimensions: 1, 2 or 3
 * \param logicalBaseFormat  basic texture derived from the user's
 *    internal texture format value
 * \param textureBaseFormat  the actual basic format of the texture
 * \param srcWidth  source image width
 * \param srcHeight  source image height
 * \param srcDepth  source image depth
 * \param srcFormat  source image format
 * \param srcType  source image type
 * \param srcAddr  source image address
 * \param srcPacking  source image pixel packing
 * \return resulting image with format = textureBaseFormat and type = GLfloat.
 */
static GLfloat *
make_temp_float_image(GLcontext *ctx, GLuint dims,
                      GLenum logicalBaseFormat,
                      GLenum textureBaseFormat,
                      GLint srcWidth, GLint srcHeight, GLint srcDepth,
                      GLenum srcFormat, GLenum srcType,
                      const GLvoid *srcAddr,
                      const struct gl_pixelstore_attrib *srcPacking)
{
   GLuint transferOps = ctx->_ImageTransferState;
   GLfloat *tempImage;

   ASSERT(dims >= 1 && dims <= 3);

   ASSERT(logicalBaseFormat == GL_RGBA ||
          logicalBaseFormat == GL_RGB ||
          logicalBaseFormat == GL_LUMINANCE_ALPHA ||
          logicalBaseFormat == GL_LUMINANCE ||
          logicalBaseFormat == GL_ALPHA ||
          logicalBaseFormat == GL_INTENSITY ||
          logicalBaseFormat == GL_COLOR_INDEX ||
          logicalBaseFormat == GL_DEPTH_COMPONENT);

   ASSERT(textureBaseFormat == GL_RGBA ||
          textureBaseFormat == GL_RGB ||
          textureBaseFormat == GL_LUMINANCE_ALPHA ||
          textureBaseFormat == GL_LUMINANCE ||
          textureBaseFormat == GL_ALPHA ||
          textureBaseFormat == GL_INTENSITY ||
          textureBaseFormat == GL_COLOR_INDEX ||
          textureBaseFormat == GL_DEPTH_COMPONENT);

   /* conventional color image */

   if ((dims == 1 && ctx->Pixel.Convolution1DEnabled) ||
       (dims >= 2 && ctx->Pixel.Convolution2DEnabled) ||
       (dims >= 2 && ctx->Pixel.Separable2DEnabled)) {
      /* need image convolution */
      const GLuint preConvTransferOps
         = (transferOps & IMAGE_PRE_CONVOLUTION_BITS) | IMAGE_CLAMP_BIT;
      const GLuint postConvTransferOps
         = (transferOps & IMAGE_POST_CONVOLUTION_BITS) | IMAGE_CLAMP_BIT;
      GLint img, row;
      GLint convWidth, convHeight;
      GLfloat *convImage;

      /* pre-convolution image buffer (3D) */
      tempImage = (GLfloat *) _mesa_malloc(srcWidth * srcHeight * srcDepth
                                           * 4 * sizeof(GLfloat));
      if (!tempImage)
         return NULL;

      /* post-convolution image buffer (2D) */
      convImage = (GLfloat *) _mesa_malloc(srcWidth * srcHeight
                                           * 4 * sizeof(GLfloat));
      if (!convImage) {
         _mesa_free(tempImage);
         return NULL;
      }

      /* loop over 3D image slices */
      for (img = 0; img < srcDepth; img++) {
         GLfloat *dst = tempImage + img * (srcWidth * srcHeight * 4);

         /* unpack and do transfer ops up to convolution */
         for (row = 0; row < srcHeight; row++) {
            const GLvoid *src = _mesa_image_address(srcPacking,
                                              srcAddr, srcWidth, srcHeight,
                                              srcFormat, srcType, img, row, 0);
            _mesa_unpack_color_span_float(ctx, srcWidth, GL_RGBA, dst,
                                          srcFormat, srcType, src,
                                          srcPacking,
                                          preConvTransferOps);
            dst += srcWidth * 4;
         }

         /* do convolution */
         {
            GLfloat *src = tempImage + img * (srcWidth * srcHeight * 4);
            convWidth = srcWidth;
            convHeight = srcHeight;
            if (dims == 1) {
               ASSERT(ctx->Pixel.Convolution1DEnabled);
               _mesa_convolve_1d_image(ctx, &convWidth, src, convImage);
            }
            else {
               if (ctx->Pixel.Convolution2DEnabled) {
                  _mesa_convolve_2d_image(ctx, &convWidth, &convHeight,
                                          src, convImage);
               }
               else {
                  ASSERT(ctx->Pixel.Separable2DEnabled);
                  _mesa_convolve_sep_image(ctx, &convWidth, &convHeight,
                                           src, convImage);
               }
            }
         }

         /* do post-convolution transfer and pack into tempImage */
         {
            const GLfloat *src = convImage;
            GLfloat *dst = tempImage + img * (convWidth * convHeight * 4);
            for (row = 0; row < convHeight; row++) {
               _mesa_pack_rgba_span_float(ctx, convWidth,
                                          (const GLfloat (*)[4]) src,
                                          logicalBaseFormat, GL_FLOAT,
                                          dst, &ctx->DefaultPacking,
                                          postConvTransferOps);
               src += convWidth * 4;
               dst += convWidth * 4;
            }
         }
      } /* loop over 3D image slices */

      _mesa_free(convImage);

      /* might need these below */
      srcWidth = convWidth;
      srcHeight = convHeight;
   }
   else {
      /* no convolution */
      const GLint components = _mesa_components_in_format(logicalBaseFormat);
      const GLint srcStride = _mesa_image_row_stride(srcPacking,
                                                 srcWidth, srcFormat, srcType);
      GLfloat *dst;
      GLint img, row;

      tempImage = (GLfloat *) _mesa_malloc(srcWidth * srcHeight * srcDepth
                                           * components * sizeof(GLfloat));
      if (!tempImage)
         return NULL;

      dst = tempImage;
      for (img = 0; img < srcDepth; img++) {
         const GLubyte *src
            = (const GLubyte *) _mesa_image_address(srcPacking, srcAddr,
                                                    srcWidth, srcHeight,
                                                    srcFormat, srcType,
                                                    img, 0, 0);
         for (row = 0; row < srcHeight; row++) {
            _mesa_unpack_color_span_float(ctx, srcWidth, logicalBaseFormat,
                                          dst, srcFormat, srcType, src,
                                          srcPacking, transferOps);
            dst += srcWidth * components;
            src += srcStride;
         }
      }
   }

   if (logicalBaseFormat != textureBaseFormat) {
      /* more work */
      GLint texComponents = _mesa_components_in_format(textureBaseFormat);
      GLint logComponents = _mesa_components_in_format(logicalBaseFormat);
      GLfloat *newImage;
      GLint i, n;
      GLint map[4];

      /* we only promote up to RGB and RGBA formats for now */
      ASSERT(textureBaseFormat == GL_RGB || textureBaseFormat == GL_RGBA);

      /* The actual texture format should have at least as many components
       * as the logical texture format.
       */
      ASSERT(texComponents >= logComponents);

      newImage = (GLfloat *) _mesa_malloc(srcWidth * srcHeight * srcDepth
                                          * texComponents * sizeof(GLfloat));
      if (!newImage) {
         _mesa_free(tempImage);
         return NULL;
      }

      compute_component_mapping(logicalBaseFormat, textureBaseFormat, map);

      n = srcWidth * srcHeight * srcDepth;
      for (i = 0; i < n; i++) {
         GLint k;
         for (k = 0; k < texComponents; k++) {
            GLint j = map[k];
            if (j == ZERO)
               newImage[i * texComponents + k] = 0.0F;
            else if (j == ONE)
               newImage[i * texComponents + k] = 1.0F;
            else
               newImage[i * texComponents + k] = tempImage[i * logComponents + j];
         }
      }

      _mesa_free(tempImage);
      tempImage = newImage;
   }

   return tempImage;
}


/**
 * Make a temporary (color) texture image with GLchan components.
 * Apply all needed pixel unpacking and pixel transfer operations.
 * Note that there are both logicalBaseFormat and textureBaseFormat parameters.
 * Suppose the user specifies GL_LUMINANCE as the internal texture format
 * but the graphics hardware doesn't support luminance textures.  So, might
 * use an RGB hardware format instead.
 * If logicalBaseFormat != textureBaseFormat we have some extra work to do.
 *
 * \param ctx  the rendering context
 * \param dims  image dimensions: 1, 2 or 3
 * \param logicalBaseFormat  basic texture derived from the user's
 *    internal texture format value
 * \param textureBaseFormat  the actual basic format of the texture
 * \param srcWidth  source image width
 * \param srcHeight  source image height
 * \param srcDepth  source image depth
 * \param srcFormat  source image format
 * \param srcType  source image type
 * \param srcAddr  source image address
 * \param srcPacking  source image pixel packing
 * \return resulting image with format = textureBaseFormat and type = GLchan.
 */
static GLchan *
make_temp_chan_image(GLcontext *ctx, GLuint dims,
                     GLenum logicalBaseFormat,
                     GLenum textureBaseFormat,
                     GLint srcWidth, GLint srcHeight, GLint srcDepth,
                     GLenum srcFormat, GLenum srcType,
                     const GLvoid *srcAddr,
                     const struct gl_pixelstore_attrib *srcPacking)
{
   GLuint transferOps = ctx->_ImageTransferState;
   const GLint components = _mesa_components_in_format(logicalBaseFormat);
   GLboolean freeSrcImage = GL_FALSE;
   GLint img, row;
   GLchan *tempImage, *dst;

   ASSERT(dims >= 1 && dims <= 3);

   ASSERT(logicalBaseFormat == GL_RGBA ||
          logicalBaseFormat == GL_RGB ||
          logicalBaseFormat == GL_LUMINANCE_ALPHA ||
          logicalBaseFormat == GL_LUMINANCE ||
          logicalBaseFormat == GL_ALPHA ||
          logicalBaseFormat == GL_INTENSITY);

   ASSERT(textureBaseFormat == GL_RGBA ||
          textureBaseFormat == GL_RGB ||
          textureBaseFormat == GL_LUMINANCE_ALPHA ||
          textureBaseFormat == GL_LUMINANCE ||
          textureBaseFormat == GL_ALPHA ||
          textureBaseFormat == GL_INTENSITY);

   if ((dims == 1 && ctx->Pixel.Convolution1DEnabled) ||
       (dims >= 2 && ctx->Pixel.Convolution2DEnabled) ||
       (dims >= 2 && ctx->Pixel.Separable2DEnabled)) {
      /* get convolved image */
      GLfloat *convImage = make_temp_float_image(ctx, dims,
                                                 logicalBaseFormat,
                                                 logicalBaseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType,
                                                 srcAddr, srcPacking);
      if (!convImage)
         return NULL;
      /* the convolved image is our new source image */
      srcAddr = convImage;
      srcFormat = logicalBaseFormat;
      srcType = GL_FLOAT;
      srcPacking = &ctx->DefaultPacking;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      transferOps = 0;
      freeSrcImage = GL_TRUE;
   }

   /* unpack and transfer the source image */
   tempImage = (GLchan *) _mesa_malloc(srcWidth * srcHeight * srcDepth
                                       * components * sizeof(GLchan));
   if (!tempImage)
      return NULL;

   dst = tempImage;
   for (img = 0; img < srcDepth; img++) {
      const GLint srcStride = _mesa_image_row_stride(srcPacking,
                                                     srcWidth, srcFormat,
                                                     srcType);
      const GLubyte *src
         = (const GLubyte *) _mesa_image_address(srcPacking, srcAddr,
                                                 srcWidth, srcHeight,
                                                 srcFormat, srcType,
                                                 img, 0, 0);
      for (row = 0; row < srcHeight; row++) {
         _mesa_unpack_color_span_chan(ctx, srcWidth, logicalBaseFormat,
                                      dst, srcFormat, srcType, src,
                                      srcPacking, transferOps);
         dst += srcWidth * components;
         src += srcStride;
      }
   }

   /* If we made a temporary image for convolution, free it here */
   if (freeSrcImage) {
      _mesa_free((void *) srcAddr);
   }

   if (logicalBaseFormat != textureBaseFormat) {
      /* one more conversion step */
      GLint texComponents = _mesa_components_in_format(textureBaseFormat);
      GLint logComponents = _mesa_components_in_format(logicalBaseFormat);
      GLchan *newImage;
      GLint i, n;
      GLint map[4];

      /* we only promote up to RGB and RGBA formats for now */
      ASSERT(textureBaseFormat == GL_RGB || textureBaseFormat == GL_RGBA);

      /* The actual texture format should have at least as many components
       * as the logical texture format.
       */
      ASSERT(texComponents >= logComponents);

      newImage = (GLchan *) _mesa_malloc(srcWidth * srcHeight * srcDepth
                                          * texComponents * sizeof(GLchan));
      if (!newImage) {
         _mesa_free(tempImage);
         return NULL;
      }

      compute_component_mapping(logicalBaseFormat, textureBaseFormat, map);

      n = srcWidth * srcHeight * srcDepth;
      for (i = 0; i < n; i++) {
         GLint k;
         for (k = 0; k < texComponents; k++) {
            GLint j = map[k];
            if (j == ZERO)
               newImage[i * texComponents + k] = 0;
            else if (j == ONE)
               newImage[i * texComponents + k] = CHAN_MAX;
            else
               newImage[i * texComponents + k] = tempImage[i * logComponents + j];
         }
      }

      _mesa_free(tempImage);
      tempImage = newImage;
   }

   return tempImage;
}



/**
 * Teximage storage routine for when a simple memcpy will do.
 * No pixel transfer operations or special texel encodings allowed.
 * 1D, 2D and 3D images supported.
 */
static void
memcpy_texture(const struct gl_texture_format *dstFormat,
               GLvoid *dstAddr,
               GLint dstXoffset, GLint dstYoffset, GLint dstZoffset,
               GLint dstRowStride, GLint dstImageStride,
               GLint srcWidth, GLint srcHeight, GLint srcDepth,
               GLenum srcFormat, GLenum srcType,
               const GLvoid *srcAddr,
               const struct gl_pixelstore_attrib *srcPacking)
{
   const GLint srcRowStride = _mesa_image_row_stride(srcPacking, srcWidth,
                                                     srcFormat, srcType);
   const GLint srcImageStride = _mesa_image_image_stride(srcPacking,
                                      srcWidth, srcHeight, srcFormat, srcType);
   const GLubyte *srcImage = (const GLubyte *) _mesa_image_address(srcPacking,
                    srcAddr, srcWidth, srcHeight, srcFormat, srcType, 0, 0, 0);
   const GLint bytesPerRow = srcWidth * dstFormat->TexelBytes;
   const GLint bytesPerImage = srcHeight * bytesPerRow;
   const GLint bytesPerTexture = srcDepth * bytesPerImage;
   GLubyte *dstImage = (GLubyte *) dstAddr
                     + dstZoffset * dstImageStride
                     + dstYoffset * dstRowStride
                     + dstXoffset * dstFormat->TexelBytes;

   if (dstRowStride == srcRowStride &&
       dstRowStride == bytesPerRow &&
       ((dstImageStride == srcImageStride &&
         dstImageStride == bytesPerImage) ||
        (srcDepth == 1))) {
      /* one big memcpy */
      _mesa_memcpy(dstImage, srcImage, bytesPerTexture);
   }
   else {
      GLint img, row;
      for (img = 0; img < srcDepth; img++) {
         const GLubyte *srcRow = srcImage;
         GLubyte *dstRow = dstImage;
         for (row = 0; row < srcHeight; row++) {
            _mesa_memcpy(dstRow, srcRow, bytesPerRow);
            dstRow += dstRowStride;
            srcRow += srcRowStride;
         }
         srcImage += srcImageStride;
         dstImage += dstImageStride;
      }
   }
}



/**
 * Store an image in any of the formats:
 *   _mesa_texformat_rgba
 *   _mesa_texformat_rgb
 *   _mesa_texformat_alpha
 *   _mesa_texformat_luminance
 *   _mesa_texformat_luminance_alpha
 *   _mesa_texformat_intensity
 * 
 * \param dims  either 1 or 2 or 3
 * \param baseInternalFormat  user-specified base internal format
 * \param dstFormat  destination Mesa texture format
 * \param dstAddr  destination image address
 * \param dstX/Y/Zoffset  destination x/y/z offset (ala TexSubImage), in texels
 * \param dstRowStride  destination image row stride, in bytes
 * \param dstImageStride  destination image layer stride, in bytes
 * \param srcWidth/Height/Depth  source image size, in pixels
 * \param srcFormat  incoming image format
 * \param srcType  incoming image data type
 * \param srcAddr  source image address
 * \param srcPacking  source image packing parameters
 */
GLboolean
_mesa_texstore_rgba(GLcontext *ctx, GLuint dims,
                    GLenum baseInternalFormat,
                    const struct gl_texture_format *dstFormat,
                    GLvoid *dstAddr,
                    GLint dstXoffset, GLint dstYoffset, GLint dstZoffset,
                    GLint dstRowStride, GLint dstImageStride,
                    GLint srcWidth, GLint srcHeight, GLint srcDepth,
                    GLenum srcFormat, GLenum srcType,
                    const GLvoid *srcAddr,
                    const struct gl_pixelstore_attrib *srcPacking)
{
   const GLint components = _mesa_components_in_format(baseInternalFormat);

   ASSERT(dstFormat == &_mesa_texformat_rgba ||
          dstFormat == &_mesa_texformat_rgb ||
          dstFormat == &_mesa_texformat_alpha ||
          dstFormat == &_mesa_texformat_luminance ||
          dstFormat == &_mesa_texformat_luminance_alpha ||
          dstFormat == &_mesa_texformat_intensity);
   ASSERT(baseInternalFormat == GL_RGBA ||
          baseInternalFormat == GL_RGB ||
          baseInternalFormat == GL_ALPHA ||
          baseInternalFormat == GL_LUMINANCE ||
          baseInternalFormat == GL_LUMINANCE_ALPHA ||
          baseInternalFormat == GL_INTENSITY);
   ASSERT(dstFormat->TexelBytes == components * sizeof(GLchan));

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == srcFormat &&
       srcType == CHAN_TYPE) {
      /* simple memcpy path */
      memcpy_texture(dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride, dstImageStride,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else if (!ctx->_ImageTransferState &&
            !srcPacking->SwapBytes &&
            dstFormat == &_mesa_texformat_rgb &&
            srcFormat == GL_RGBA &&
            srcType == CHAN_TYPE) {
      /* extract RGB from RGBA */
      int img, row, col;
      GLchan *dstImage = (GLchan *) (GLubyte *) dstAddr
                       + dstZoffset * dstImageStride
                       + dstYoffset * dstRowStride
                       + dstXoffset * dstFormat->TexelBytes;
      for (img = 0; img < srcDepth; img++) {
         const GLint srcRowStride = _mesa_image_row_stride(srcPacking,
                                                 srcWidth, srcFormat, srcType);
         GLchan *srcRow = _mesa_image_address(srcPacking, srcAddr, srcWidth,
                                              srcHeight, srcFormat, srcType,
                                              img, 0, 0);
         GLchan *dstRow = dstImage;
         for (row = 0; row < srcHeight; row++) {
            for (col = 0; col < srcWidth; col++) {
               dstRow[col * 3 + RCOMP] = srcRow[col * 4 + RCOMP];
               dstRow[col * 3 + GCOMP] = srcRow[col * 4 + GCOMP];
               dstRow[col * 3 + BCOMP] = srcRow[col * 4 + BCOMP];
            }
            dstRow += dstRowStride;
            srcRow = (GLchan *) ((GLubyte *) srcRow + srcRowStride);
         }
         dstImage += dstImageStride;
      }
   }
   else {
      /* general path */
      const GLchan *tempImage = make_temp_chan_image(ctx, dims,
                                                 baseInternalFormat,
                                                 dstFormat->BaseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLchan *src = tempImage;
      const GLint bytesPerRow = srcWidth * components * sizeof(GLchan);
      GLubyte *dstImage = (GLubyte *) dstAddr
                        + dstZoffset * dstImageStride
                        + dstYoffset * dstRowStride
                        + dstXoffset * dstFormat->TexelBytes;
      GLint img, row;
      if (!tempImage)
         return GL_FALSE;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstImage;
         for (row = 0; row < srcHeight; row++) {
            _mesa_memcpy(dstRow, src, bytesPerRow);
            dstRow += dstRowStride;
            src += srcWidth * components;
         }
         dstImage += dstImageStride;
      }

      _mesa_free((void *) tempImage);
   }
   return GL_TRUE;
}


/**
 * Store a color index texture image
 */
GLboolean
_mesa_texstore_color_index(STORE_PARAMS)
{
   ASSERT(dstFormat == &_mesa_texformat_color_index);
   ASSERT(dstFormat->TexelBytes == 1 * sizeof(GLchan));

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == GL_COLOR_INDEX &&
       srcFormat == GL_COLOR_INDEX &&
       srcType == CHAN_TYPE) {
      /* simple memcpy path */
      memcpy_texture(dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride, dstImageStride,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      GLubyte *dstImage = (GLubyte *) dstAddr
                        + dstZoffset * dstImageStride
                        + dstYoffset * dstRowStride
                        + dstXoffset * dstFormat->TexelBytes;
      GLint img, row;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstImage;
         for (row = 0; row < srcHeight; row++) {
            const GLvoid *src = _mesa_image_address(srcPacking,
                srcAddr, srcWidth, srcHeight, srcFormat, srcType, img, row, 0);
            _mesa_unpack_index_span(ctx, srcWidth, CHAN_TYPE, dstRow,
                                    srcType, src, srcPacking,
                                    ctx->_ImageTransferState);
            dstRow += dstRowStride;
         }
         dstImage += dstImageStride;
      }
   }
   return GL_TRUE;
}


/**
 * Store a floating point depth component texture image.
 */
GLboolean
_mesa_texstore_depth_component_float32(STORE_PARAMS)
{
   ASSERT(dstFormat == &_mesa_texformat_depth_component_float32);
   ASSERT(dstFormat->TexelBytes == sizeof(GLfloat));

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == GL_DEPTH_COMPONENT &&
       srcFormat == GL_DEPTH_COMPONENT &&
       srcType == GL_FLOAT) {
      /* simple memcpy path */
      memcpy_texture(dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride, dstImageStride,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      GLubyte *dstImage = (GLubyte *) dstAddr
                        + dstZoffset * dstImageStride
                        + dstYoffset * dstRowStride
                        + dstXoffset * dstFormat->TexelBytes;
      GLint img, row;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstImage;
         for (row = 0; row < srcHeight; row++) {
            const GLvoid *src = _mesa_image_address(srcPacking,
                srcAddr, srcWidth, srcHeight, srcFormat, srcType, img, row, 0);
            _mesa_unpack_depth_span(ctx, srcWidth, (GLfloat *) dstRow,
                                    srcType, src, srcPacking);
            dstRow += dstRowStride;
         }
         dstImage += dstImageStride;
      }
   }
   return GL_TRUE;
}


/**
 * Store a 16-bit integer depth component texture image.
 */
GLboolean
_mesa_texstore_depth_component16(STORE_PARAMS)
{
   ASSERT(dstFormat == &_mesa_texformat_depth_component16);
   ASSERT(dstFormat->TexelBytes == sizeof(GLushort));

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == GL_DEPTH_COMPONENT &&
       srcFormat == GL_DEPTH_COMPONENT &&
       srcType == GL_UNSIGNED_SHORT) {
      /* simple memcpy path */
      memcpy_texture(dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride, dstImageStride,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      GLubyte *dstImage = (GLubyte *) dstAddr
                        + dstZoffset * dstImageStride
                        + dstYoffset * dstRowStride
                        + dstXoffset * dstFormat->TexelBytes;
      GLint img, row, col;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstImage;
         for (row = 0; row < srcHeight; row++) {
            GLfloat depthTemp[MAX_WIDTH];
            const GLvoid *src = _mesa_image_address(srcPacking,
                srcAddr, srcWidth, srcHeight, srcFormat, srcType, img, row, 0);
            GLushort *dst16 = (GLushort *) dstRow;
            _mesa_unpack_depth_span(ctx, srcWidth, depthTemp,
                                    srcType, src, srcPacking);
            for (col = 0; col < srcWidth; col++) {
               dst16[col] = (GLushort) (depthTemp[col] * 65535.0F);
            }
            dstRow += dstRowStride;
         }
         dstImage += dstImageStride;
      }
   }
   return GL_TRUE;
}




/**
 * Store an rgb565 texture image.
 */
GLboolean
_mesa_texstore_rgb565(STORE_PARAMS)
{
   ASSERT(dstFormat == &_mesa_texformat_rgb565);
   ASSERT(dstFormat->TexelBytes == 2);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == GL_RGB &&
       srcFormat == GL_RGB &&
       srcType == GL_UNSIGNED_SHORT_5_6_5) {
      /* simple memcpy path */
      memcpy_texture(dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride, dstImageStride,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else if (!ctx->_ImageTransferState &&
            !srcPacking->SwapBytes &&
            baseInternalFormat == GL_RGB &&
            srcFormat == GL_RGB &&
            srcType == GL_UNSIGNED_BYTE &&
            dims == 2) {
      /* do optimized tex store */
      const GLint srcRowStride = _mesa_image_row_stride(srcPacking, srcWidth,
                                                        srcFormat, srcType);
      const GLubyte *src = (const GLubyte *)
         _mesa_image_address(srcPacking, srcAddr, srcWidth, srcHeight,
                             srcFormat, srcType, 0, 0, 0);
      GLubyte *dst = (GLubyte *) dstAddr
                   + dstZoffset * dstImageStride
                   + dstYoffset * dstRowStride
                   + dstXoffset * dstFormat->TexelBytes;
      GLint row, col;
      for (row = 0; row < srcHeight; row++) {
         const GLubyte *srcUB = (const GLubyte *) src;
         GLushort *dstUS = (GLushort *) dst;
         for (col = 0; col < srcWidth; col++) {
            dstUS[col] = PACK_COLOR_565( srcUB[0], srcUB[1], srcUB[2] );
            srcUB += 3;
         }
         dst += dstRowStride;
         src += srcRowStride;
      }
   }
   else {
      /* general path */
      const GLchan *tempImage = make_temp_chan_image(ctx, dims,
                                                 baseInternalFormat,
                                                 dstFormat->BaseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLchan *src = tempImage;
      GLubyte *dstImage = (GLubyte *) dstAddr
                        + dstZoffset * dstImageStride
                        + dstYoffset * dstRowStride
                        + dstXoffset * dstFormat->TexelBytes;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstImage;
         for (row = 0; row < srcHeight; row++) {
            GLushort *dstUS = (GLushort *) dstRow;
            for (col = 0; col < srcWidth; col++) {
               dstUS[col] = PACK_COLOR_565( CHAN_TO_UBYTE(src[RCOMP]),
                                            CHAN_TO_UBYTE(src[GCOMP]),
                                            CHAN_TO_UBYTE(src[BCOMP]) );
               src += 3;
            }
            dstRow += dstRowStride;
         }
         dstImage += dstImageStride;
      }
      _mesa_free((void *) tempImage);
   }
   return GL_TRUE;
}


GLboolean
_mesa_texstore_rgba8888(STORE_PARAMS)
{
   const GLuint ui = 1;
   const GLubyte littleEndian = *((const GLubyte *) &ui);

   ASSERT(dstFormat == &_mesa_texformat_rgba8888);
   ASSERT(dstFormat->TexelBytes == 4);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == GL_RGBA &&
       srcFormat == GL_RGBA &&
       ((srcType == GL_UNSIGNED_INT_8_8_8_8_REV && littleEndian) ||
        (srcType == GL_UNSIGNED_INT_8_8_8_8 && !littleEndian))) {
      /* simple memcpy path */
      memcpy_texture(dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride, dstImageStride,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLchan *tempImage = make_temp_chan_image(ctx, dims,
                                                 baseInternalFormat,
                                                 dstFormat->BaseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLchan *src = tempImage;
      GLubyte *dstImage = (GLubyte *) dstAddr
                        + dstZoffset * dstImageStride
                        + dstYoffset * dstRowStride
                        + dstXoffset * dstFormat->TexelBytes;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstImage;
         for (row = 0; row < srcHeight; row++) {
            GLuint *dstUI = (GLuint *) dstRow;
            for (col = 0; col < srcWidth; col++) {
               dstUI[col] = PACK_COLOR_8888( CHAN_TO_UBYTE(src[RCOMP]),
                                             CHAN_TO_UBYTE(src[GCOMP]),
                                             CHAN_TO_UBYTE(src[BCOMP]),
                                             CHAN_TO_UBYTE(src[ACOMP]) );
               src += 4;
            }
            dstRow += dstRowStride;
         }
         dstImage += dstImageStride;
      }
      _mesa_free((void *) tempImage);
   }
   return GL_TRUE;
}


GLboolean
_mesa_texstore_argb8888(STORE_PARAMS)
{
   const GLuint ui = 1;
   const GLubyte littleEndian = *((const GLubyte *) &ui);

   ASSERT(dstFormat == &_mesa_texformat_argb8888);
   ASSERT(dstFormat->TexelBytes == 4);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == GL_RGBA &&
       srcFormat == GL_BGRA &&
       ((srcType == GL_UNSIGNED_BYTE && littleEndian) ||
        (srcType == GL_UNSIGNED_INT_8_8_8_8_REV && littleEndian) ||
        (srcType == GL_UNSIGNED_INT_8_8_8_8 && !littleEndian))) {
      /* simple memcpy path */
      memcpy_texture(dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride, dstImageStride,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLchan *tempImage = make_temp_chan_image(ctx, dims,
                                                 baseInternalFormat,
                                                 dstFormat->BaseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLchan *src = tempImage;
      GLubyte *dstImage = (GLubyte *) dstAddr
                        + dstZoffset * dstImageStride
                        + dstYoffset * dstRowStride
                        + dstXoffset * dstFormat->TexelBytes;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstImage;
         for (row = 0; row < srcHeight; row++) {
            GLuint *dstUI = (GLuint *) dstRow;
            for (col = 0; col < srcWidth; col++) {
               dstUI[col] = PACK_COLOR_8888( CHAN_TO_UBYTE(src[ACOMP]),
                                             CHAN_TO_UBYTE(src[RCOMP]),
                                             CHAN_TO_UBYTE(src[GCOMP]),
                                             CHAN_TO_UBYTE(src[BCOMP]) );
               src += 4;
            }
            dstRow += dstRowStride;
         }
         dstImage += dstImageStride;
      }
      _mesa_free((void *) tempImage);
   }
   return GL_TRUE;
}



GLboolean
_mesa_texstore_rgb888(STORE_PARAMS)
{
   const GLuint ui = 1;
   const GLubyte littleEndian = *((const GLubyte *) &ui);

   ASSERT(dstFormat == &_mesa_texformat_rgb888);
   ASSERT(dstFormat->TexelBytes == 3);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == GL_RGB &&
       srcFormat == GL_BGR &&
       srcType == GL_UNSIGNED_BYTE &&
       littleEndian) {
      /* simple memcpy path */
      memcpy_texture(dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride, dstImageStride,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else if (!ctx->_ImageTransferState &&
            !srcPacking->SwapBytes &&
            srcFormat == GL_RGBA &&
            srcType == GL_UNSIGNED_BYTE) {
      /* extract BGR from RGBA */
      int img, row, col;
      GLubyte *dstImage = (GLubyte *) dstAddr
                        + dstZoffset * dstImageStride
                        + dstYoffset * dstRowStride
                        + dstXoffset * dstFormat->TexelBytes;
      for (img = 0; img < srcDepth; img++) {
         const GLint srcRowStride = _mesa_image_row_stride(srcPacking,
                                                 srcWidth, srcFormat, srcType);
         GLubyte *srcRow = _mesa_image_address(srcPacking, srcAddr, srcWidth,
                                              srcHeight, srcFormat, srcType,
                                              img, 0, 0);
         GLubyte *dstRow = dstImage;
         for (row = 0; row < srcHeight; row++) {
            for (col = 0; col < srcWidth; col++) {
               dstRow[col * 3 + 0] = srcRow[col * 4 + BCOMP];
               dstRow[col * 3 + 1] = srcRow[col * 4 + GCOMP];
               dstRow[col * 3 + 2] = srcRow[col * 4 + RCOMP];
            }
            dstRow += dstRowStride;
            srcRow += srcRowStride;
         }
         dstImage += dstImageStride;
      }
   }
   else {
      /* general path */
      const GLchan *tempImage = make_temp_chan_image(ctx, dims,
                                                 baseInternalFormat,
                                                 dstFormat->BaseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLchan *src = (const GLubyte *) tempImage;
      GLubyte *dstImage = (GLubyte *) dstAddr
                        + dstZoffset * dstImageStride
                        + dstYoffset * dstRowStride
                        + dstXoffset * dstFormat->TexelBytes;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstImage;
         for (row = 0; row < srcHeight; row++) {
#if 0
            if (littleEndian) {
               for (col = 0; col < srcWidth; col++) {
                  dstRow[col * 3 + 0] = CHAN_TO_UBYTE(src[RCOMP]);
                  dstRow[col * 3 + 1] = CHAN_TO_UBYTE(src[GCOMP]);
                  dstRow[col * 3 + 2] = CHAN_TO_UBYTE(src[BCOMP]);
                  srcUB += 3;
               }
            }
            else {
               for (col = 0; col < srcWidth; col++) {
                  dstRow[col * 3 + 0] = srcUB[BCOMP];
                  dstRow[col * 3 + 1] = srcUB[GCOMP];
                  dstRow[col * 3 + 2] = srcUB[RCOMP];
                  srcUB += 3;
               }
            }
#else
            for (col = 0; col < srcWidth; col++) {
               dstRow[col * 3 + 0] = CHAN_TO_UBYTE(src[BCOMP]);
               dstRow[col * 3 + 1] = CHAN_TO_UBYTE(src[GCOMP]);
               dstRow[col * 3 + 2] = CHAN_TO_UBYTE(src[RCOMP]);
               src += 3;
            }
#endif
            dstRow += dstRowStride;
         }
         dstImage += dstImageStride;
      }
      _mesa_free((void *) tempImage);
   }
   return GL_TRUE;
}


GLboolean
_mesa_texstore_argb4444(STORE_PARAMS)
{
   const GLuint ui = 1;
   const GLubyte littleEndian = *((const GLubyte *) &ui);

   ASSERT(dstFormat == &_mesa_texformat_argb4444);
   ASSERT(dstFormat->TexelBytes == 2);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == GL_RGBA &&
       srcFormat == GL_BGRA &&
       ((srcType == GL_UNSIGNED_SHORT_4_4_4_4_REV && littleEndian) ||
        (srcType == GL_UNSIGNED_SHORT_4_4_4_4 && !littleEndian))) {
      /* simple memcpy path */
      memcpy_texture(dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride, dstImageStride,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLchan *tempImage = make_temp_chan_image(ctx, dims,
                                                 baseInternalFormat,
                                                 dstFormat->BaseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLchan *src = tempImage;
      GLubyte *dstImage = (GLubyte *) dstAddr
                        + dstZoffset * dstImageStride
                        + dstYoffset * dstRowStride
                        + dstXoffset * dstFormat->TexelBytes;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstImage;
         for (row = 0; row < srcHeight; row++) {
            GLushort *dstUS = (GLushort *) dstRow;
            for (col = 0; col < srcWidth; col++) {
               dstUS[col] = PACK_COLOR_4444( CHAN_TO_UBYTE(src[ACOMP]),
                                             CHAN_TO_UBYTE(src[RCOMP]),
                                             CHAN_TO_UBYTE(src[GCOMP]),
                                             CHAN_TO_UBYTE(src[BCOMP]) );
               src += 4;
            }
            dstRow += dstRowStride;
         }
         dstImage += dstImageStride;
      }
      _mesa_free((void *) tempImage);
   }
   return GL_TRUE;
}


GLboolean
_mesa_texstore_argb1555(STORE_PARAMS)
{
   const GLuint ui = 1;
   const GLubyte littleEndian = *((const GLubyte *) &ui);

   ASSERT(dstFormat == &_mesa_texformat_argb1555);
   ASSERT(dstFormat->TexelBytes == 2);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == GL_RGBA &&
       srcFormat == GL_BGRA &&
       ((srcType == GL_UNSIGNED_SHORT_1_5_5_5_REV && littleEndian) ||
        (srcType == GL_UNSIGNED_SHORT_5_5_5_1 && !littleEndian))) {
      /* simple memcpy path */
      memcpy_texture(dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride, dstImageStride,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLchan *tempImage = make_temp_chan_image(ctx, dims,
                                                 baseInternalFormat,
                                                 dstFormat->BaseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLchan *src =tempImage;
      GLubyte *dstImage = (GLubyte *) dstAddr
                        + dstZoffset * dstImageStride
                        + dstYoffset * dstRowStride
                        + dstXoffset * dstFormat->TexelBytes;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstImage;
         for (row = 0; row < srcHeight; row++) {
            GLushort *dstUS = (GLushort *) dstRow;
            for (col = 0; col < srcWidth; col++) {
               dstUS[col] = PACK_COLOR_1555( CHAN_TO_UBYTE(src[ACOMP]),
                                             CHAN_TO_UBYTE(src[RCOMP]),
                                             CHAN_TO_UBYTE(src[GCOMP]),
                                             CHAN_TO_UBYTE(src[BCOMP]) );
               src += 4;
            }
            dstRow += dstRowStride;
         }
         dstImage += dstImageStride;
      }
      _mesa_free((void *) tempImage);
   }
   return GL_TRUE;
}


GLboolean
_mesa_texstore_al88(STORE_PARAMS)
{
   const GLuint ui = 1;
   const GLubyte littleEndian = *((const GLubyte *) &ui);

   ASSERT(dstFormat == &_mesa_texformat_al88);
   ASSERT(dstFormat->TexelBytes == 2);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == GL_LUMINANCE_ALPHA &&
       srcFormat == GL_LUMINANCE_ALPHA &&
       srcType == GL_UNSIGNED_BYTE &&
       littleEndian) {
      /* simple memcpy path */
      memcpy_texture(dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride, dstImageStride,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLchan *tempImage = make_temp_chan_image(ctx, dims,
                                                 baseInternalFormat,
                                                 dstFormat->BaseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLchan *src = tempImage;
      GLubyte *dstImage = (GLubyte *) dstAddr
                        + dstZoffset * dstImageStride
                        + dstYoffset * dstRowStride
                        + dstXoffset * dstFormat->TexelBytes;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstImage;
         for (row = 0; row < srcHeight; row++) {
            GLushort *dstUS = (GLushort *) dstRow;
            for (col = 0; col < srcWidth; col++) {
               dstUS[col] = PACK_COLOR_88( CHAN_TO_UBYTE(src[ACOMP]),
                                           CHAN_TO_UBYTE(src[RCOMP]) );
               src += 2;
            }
            dstRow += dstRowStride;
         }
         dstImage += dstImageStride;
      }
      _mesa_free((void *) tempImage);
   }
   return GL_TRUE;
}


GLboolean
_mesa_texstore_rgb332(STORE_PARAMS)
{
   ASSERT(dstFormat == &_mesa_texformat_rgb332);
   ASSERT(dstFormat->TexelBytes == 1);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == GL_RGB &&
       srcFormat == GL_RGB && srcType == GL_UNSIGNED_BYTE_3_3_2) {
      /* simple memcpy path */
      memcpy_texture(dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride, dstImageStride,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLchan *tempImage = make_temp_chan_image(ctx, dims,
                                                 baseInternalFormat,
                                                 dstFormat->BaseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLchan *src = tempImage;
      GLubyte *dstImage = (GLubyte *) dstAddr
                        + dstZoffset * dstImageStride
                        + dstYoffset * dstRowStride
                        + dstXoffset * dstFormat->TexelBytes;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstImage;
         for (row = 0; row < srcHeight; row++) {
            for (col = 0; col < srcWidth; col++) {
               dstRow[col] = PACK_COLOR_332( CHAN_TO_UBYTE(src[RCOMP]),
                                             CHAN_TO_UBYTE(src[GCOMP]),
                                             CHAN_TO_UBYTE(src[BCOMP]) );
               src += 3;
            }
            dstRow += dstRowStride;
         }
         dstImage += dstImageStride;
      }
      _mesa_free((void *) tempImage);
   }
   return GL_TRUE;
}



/**
 * Texstore for _mesa_texformat_a8, _mesa_texformat_l8, _mesa_texformat_i8.
 */
GLboolean
_mesa_texstore_a8(STORE_PARAMS)
{
   ASSERT(dstFormat == &_mesa_texformat_a8 ||
          dstFormat == &_mesa_texformat_l8 ||
          dstFormat == &_mesa_texformat_i8);
   ASSERT(dstFormat->TexelBytes == 1);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == srcFormat &&
       srcType == GL_UNSIGNED_BYTE) {
      /* simple memcpy path */
      memcpy_texture(dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride, dstImageStride,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLchan *tempImage = make_temp_chan_image(ctx, dims,
                                                 baseInternalFormat,
                                                 dstFormat->BaseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLchan *src = tempImage;
      GLubyte *dstImage = (GLubyte *) dstAddr
                        + dstZoffset * dstImageStride
                        + dstYoffset * dstRowStride
                        + dstXoffset * dstFormat->TexelBytes;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstImage;
         for (row = 0; row < srcHeight; row++) {
            for (col = 0; col < srcWidth; col++) {
               dstRow[col] = CHAN_TO_UBYTE(src[col]);
            }
            dstRow += dstRowStride;
            src += srcWidth;
         }
         dstImage += dstImageStride;
      }
      _mesa_free((void *) tempImage);
   }
   return GL_TRUE;
}



GLboolean
_mesa_texstore_ci8(STORE_PARAMS)
{
   ASSERT(dstFormat == &_mesa_texformat_ci8);
   ASSERT(dstFormat->TexelBytes == 1);
   ASSERT(baseInternalFormat == GL_COLOR_INDEX);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       srcFormat == GL_COLOR_INDEX &&
       srcType == GL_UNSIGNED_BYTE) {
      /* simple memcpy path */
      memcpy_texture(dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride, dstImageStride,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      GLubyte *dstImage = (GLubyte *) dstAddr
                        + dstZoffset * dstImageStride
                        + dstYoffset * dstRowStride
                        + dstXoffset * dstFormat->TexelBytes;
      GLint img, row;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstImage;
         for (row = 0; row < srcHeight; row++) {
            const GLvoid *src = _mesa_image_address(srcPacking,
                srcAddr, srcWidth, srcHeight, srcFormat, srcType, img, row, 0);
            _mesa_unpack_index_span(ctx, srcWidth, GL_UNSIGNED_BYTE, dstRow,
                                    srcType, src, srcPacking,
                                    ctx->_ImageTransferState);
            dstRow += dstRowStride;
         }
         dstImage += dstImageStride;
      }
   }
   return GL_TRUE;
}


/**
 * Texstore for _mesa_texformat_ycbcr or _mesa_texformat_ycbcr_rev.
 */
GLboolean
_mesa_texstore_ycbcr(STORE_PARAMS)
{
   const GLuint ui = 1;
   const GLubyte littleEndian = *((const GLubyte *) &ui);

   ASSERT((dstFormat == &_mesa_texformat_ycbcr) ||
          (dstFormat == &_mesa_texformat_ycbcr_rev));
   ASSERT(dstFormat->TexelBytes == 2);
   ASSERT(ctx->Extensions.MESA_ycbcr_texture);
   ASSERT(srcFormat == GL_YCBCR_MESA);
   ASSERT((srcType == GL_UNSIGNED_SHORT_8_8_MESA) ||
          (srcType == GL_UNSIGNED_SHORT_8_8_REV_MESA));
   ASSERT(baseInternalFormat == GL_YCBCR_MESA);

   /* always just memcpy since no pixel transfer ops apply */
   memcpy_texture(dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                  dstRowStride, dstImageStride,
                  srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                  srcAddr, srcPacking);

   /* Check if we need byte swapping */
   /* XXX the logic here _might_ be wrong */
   if (srcPacking->SwapBytes ^
       (srcType == GL_UNSIGNED_SHORT_8_8_REV_MESA) ^
       (dstFormat == &_mesa_texformat_ycbcr_rev) ^
       !littleEndian) {
      GLushort *pImage = (GLushort *) ((GLubyte *) dstAddr
                                       + dstZoffset * dstImageStride
                                       + dstYoffset * dstRowStride
                                       + dstXoffset * dstFormat->TexelBytes);
      GLint img, row;
      for (img = 0; img < srcDepth; img++) {
         GLushort *pRow = pImage;
         for (row = 0; row < srcHeight; row++) {
            _mesa_swap2(pRow, srcWidth);
            pRow += dstRowStride;
         }
         pImage += dstImageStride;
      }
   }
   return GL_TRUE;
}




/**
 * Store an image in any of the formats:
 *   _mesa_texformat_rgba_float32
 *   _mesa_texformat_rgb_float32
 *   _mesa_texformat_alpha_float32
 *   _mesa_texformat_luminance_float32
 *   _mesa_texformat_luminance_alpha_float32
 *   _mesa_texformat_intensity_float32
 */
GLboolean
_mesa_texstore_rgba_float32(STORE_PARAMS)
{
   const GLint components = _mesa_components_in_format(baseInternalFormat);

   ASSERT(dstFormat == &_mesa_texformat_rgba_float32 ||
          dstFormat == &_mesa_texformat_rgb_float32 ||
          dstFormat == &_mesa_texformat_alpha_float32 ||
          dstFormat == &_mesa_texformat_luminance_float32 ||
          dstFormat == &_mesa_texformat_luminance_alpha_float32 ||
          dstFormat == &_mesa_texformat_intensity_float32);
   ASSERT(baseInternalFormat == GL_RGBA ||
          baseInternalFormat == GL_RGB ||
          baseInternalFormat == GL_ALPHA ||
          baseInternalFormat == GL_LUMINANCE ||
          baseInternalFormat == GL_LUMINANCE_ALPHA ||
          baseInternalFormat == GL_INTENSITY);
   ASSERT(dstFormat->TexelBytes == components * sizeof(GLfloat));

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == srcFormat &&
       srcType == GL_FLOAT) {
      /* simple memcpy path */
      memcpy_texture(dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride, dstImageStride,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLfloat *tempImage = make_temp_float_image(ctx, dims,
                                                 baseInternalFormat,
                                                 dstFormat->BaseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLfloat *src = tempImage;
      const GLint bytesPerRow = srcWidth * components * sizeof(GLfloat);
      GLubyte *dstImage = (GLubyte *) dstAddr
                        + dstZoffset * dstImageStride
                        + dstYoffset * dstRowStride
                        + dstXoffset * dstFormat->TexelBytes;
      GLint img, row;
      if (!tempImage)
         return GL_FALSE;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dst = dstImage;
         for (row = 0; row < srcHeight; row++) {
            _mesa_memcpy(dst, src, bytesPerRow);
            dst += dstRowStride;
            src += srcWidth * components;
         }
         dstImage += dstImageStride;
      }

      _mesa_free((void *) tempImage);
   }
   return GL_TRUE;
}


/**
 * As above, but store 16-bit floats.
 */
GLboolean
_mesa_texstore_rgba_float16(STORE_PARAMS)
{
   const GLint components = _mesa_components_in_format(baseInternalFormat);

   ASSERT(dstFormat == &_mesa_texformat_rgba_float16 ||
          dstFormat == &_mesa_texformat_rgb_float16 ||
          dstFormat == &_mesa_texformat_alpha_float16 ||
          dstFormat == &_mesa_texformat_luminance_float16 ||
          dstFormat == &_mesa_texformat_luminance_alpha_float16 ||
          dstFormat == &_mesa_texformat_intensity_float16);
   ASSERT(baseInternalFormat == GL_RGBA ||
          baseInternalFormat == GL_RGB ||
          baseInternalFormat == GL_ALPHA ||
          baseInternalFormat == GL_LUMINANCE ||
          baseInternalFormat == GL_LUMINANCE_ALPHA ||
          baseInternalFormat == GL_INTENSITY);
   ASSERT(dstFormat->TexelBytes == components * sizeof(GLhalfARB));

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == srcFormat &&
       srcType == GL_HALF_FLOAT_ARB) {
      /* simple memcpy path */
      memcpy_texture(dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride, dstImageStride,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLfloat *tempImage = make_temp_float_image(ctx, dims,
                                                 baseInternalFormat,
                                                 dstFormat->BaseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLfloat *src = tempImage;
      GLubyte *dstImage = (GLubyte *) dstAddr
                        + dstZoffset * dstImageStride
                        + dstYoffset * dstRowStride
                        + dstXoffset * dstFormat->TexelBytes;
      GLint img, row;
      if (!tempImage)
         return GL_FALSE;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstImage;
         for (row = 0; row < srcHeight; row++) {
            GLhalfARB *dstTexel = (GLhalfARB *) dstRow;
            GLint i;
            for (i = 0; i < srcWidth * components; i++) {
               dstTexel[i] = _mesa_float_to_half(src[i]);
            }
            dstRow += dstRowStride;
            src += srcWidth * components;
         }
         dstImage += dstImageStride;
      }

      _mesa_free((void *) tempImage);
   }
   return GL_TRUE;
}


#if !NEWTEXSTORE /*****************************************************/


/*
 * Given an internal texture format enum or 1, 2, 3, 4 return the
 * corresponding _base_ internal format:  GL_ALPHA, GL_LUMINANCE,
 * GL_LUMANCE_ALPHA, GL_INTENSITY, GL_RGB, or GL_RGBA.  Return the
 * number of components for the format.  Return -1 if invalid enum.
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
      case GL_YCBCR_MESA:
         return 2; /* Y + (Cb or Cr) */
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
 * The destination texel type is always GLchan.
 * The destination texel format is one of the 6 basic types.
 *
 * A hardware driver may use this as a helper routine to unpack and
 * apply pixel transfer ops into a temporary image buffer.  Then,
 * convert the temporary image into the special hardware format.
 *
 * \param
 *   dimensions - 1, 2, or 3
 *   texDestFormat - GL_LUMINANCE, GL_INTENSITY, GL_LUMINANCE_ALPHA, GL_ALPHA,
 *                   GL_RGB or GL_RGBA (the destination format)
 *   texDestAddr - destination image address
 *   srcWidth, srcHeight, srcDepth - size (in pixels) of src and dest images
 *   dstXoffset, dstYoffset, dstZoffset - position to store the image within
 *      the destination 3D texture
 *   dstRowStride, dstImageStride - dest image strides in bytes
 *   srcFormat - source image format (GL_ALPHA, GL_RED, GL_RGB, etc)
 *   srcType - GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT_5_6_5, GL_FLOAT, etc
 *   srcPacking - describes packing of incoming image.
 *   transferOps - mask of pixel transfer operations
 */
static void
transfer_teximage(GLcontext *ctx, GLuint dimensions,
                  GLenum texDestFormat, GLvoid *texDestAddr,
                  GLint srcWidth, GLint srcHeight, GLint srcDepth,
                  GLint dstXoffset, GLint dstYoffset, GLint dstZoffset,
                  GLint dstRowStride, GLint dstImageStride,
                  GLenum srcFormat, GLenum srcType,
                  const GLvoid *srcAddr,
                  const struct gl_pixelstore_attrib *srcPacking,
                  GLuint transferOps)
{
   GLint texComponents;

   ASSERT(ctx);
   ASSERT(dimensions >= 1 && dimensions <= 3);
   ASSERT(texDestFormat == GL_LUMINANCE ||
          texDestFormat == GL_INTENSITY ||
          texDestFormat == GL_LUMINANCE_ALPHA ||
          texDestFormat == GL_ALPHA ||
          texDestFormat == GL_RGB ||
          texDestFormat == GL_RGBA ||
          texDestFormat == GL_COLOR_INDEX ||
          texDestFormat == GL_DEPTH_COMPONENT);
   ASSERT(texDestAddr);
   ASSERT(srcWidth >= 0);
   ASSERT(srcHeight >= 0);
   ASSERT(srcDepth >= 0);
   ASSERT(dstXoffset >= 0);
   ASSERT(dstYoffset >= 0);
   ASSERT(dstZoffset >= 0);
   ASSERT(dstRowStride >= 0);
   ASSERT(dstImageStride >= 0);
   ASSERT(srcAddr);
   ASSERT(srcPacking);

   texComponents = components_in_intformat(texDestFormat);

   /* try common 2D texture cases first */
   if (!transferOps && dimensions == 2 && srcType == CHAN_TYPE) {

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
         GLchan *dst = (GLchan *) texDestAddr
                     + dstYoffset * (dstRowStride / sizeof(GLchan))
                     + dstXoffset * texComponents;
         if (srcRowStride == widthInBytes && dstRowStride == widthInBytes) {
            MEMCPY(dst, src, srcHeight * widthInBytes);
         }
         else {
            GLint i;
            for (i = 0; i < srcHeight; i++) {
               MEMCPY(dst, src, widthInBytes);
               src += (srcRowStride / sizeof(GLchan));
               dst += (dstRowStride / sizeof(GLchan));
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
         GLchan *dst = (GLchan *) texDestAddr
                     + dstYoffset * (dstRowStride / sizeof(GLchan))
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
            src += (srcRowStride / sizeof(GLchan));
            dst += (dstRowStride / sizeof(GLchan));
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
      GLchan *dest = (GLchan *) texDestAddr
                   + dstZoffset * (dstImageStride / sizeof(GLchan))
                   + dstYoffset * (dstRowStride / sizeof(GLchan))
                   + dstXoffset * texComponents;
      for (img = 0; img < srcDepth; img++) {
         GLchan *destRow = dest;
         for (row = 0; row < srcHeight; row++) {
            const GLvoid *src = _mesa_image_address(srcPacking,
                srcAddr, srcWidth, srcHeight, srcFormat, srcType, img, row, 0);
            _mesa_unpack_index_span(ctx, srcWidth, texType, destRow,
                                    srcType, src, srcPacking, transferOps);
            destRow += (dstRowStride / sizeof(GLchan));
         }
         dest += dstImageStride;
      }
   }
   else if (texDestFormat == GL_YCBCR_MESA) {
      /* YCbCr texture */
      GLint img, row;
      GLushort *dest = (GLushort *) texDestAddr
                     + dstZoffset * (dstImageStride / sizeof(GLushort))
                     + dstYoffset * (dstRowStride / sizeof(GLushort))
                     + dstXoffset * texComponents;
      ASSERT(ctx->Extensions.MESA_ycbcr_texture);
      for (img = 0; img < srcDepth; img++) {
         GLushort *destRow = dest;
         for (row = 0; row < srcHeight; row++) {
            const GLvoid *srcRow = _mesa_image_address(srcPacking,
                                          srcAddr, srcWidth, srcHeight,
                                          srcFormat, srcType, img, row, 0);
            MEMCPY(destRow, srcRow, srcWidth * sizeof(GLushort));
            destRow += (dstRowStride / sizeof(GLushort));
         }
         dest += dstImageStride / sizeof(GLushort);
      }
   }
   else if (texDestFormat == GL_DEPTH_COMPONENT) {
      /* Depth texture (shadow maps) */
      GLint img, row;
      GLubyte *dest = (GLubyte *) texDestAddr
                    + dstZoffset * dstImageStride
                    + dstYoffset * (dstRowStride / sizeof(GLchan))
                    + dstXoffset * texComponents;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *destRow = dest;
         for (row = 0; row < srcHeight; row++) {
            const GLvoid *src = _mesa_image_address(srcPacking,
                srcAddr, srcWidth, srcHeight, srcFormat, srcType, img, row, 0);
            _mesa_unpack_depth_span(ctx, srcWidth, (GLfloat *) destRow,
                                    srcType, src, srcPacking);
            destRow += (dstRowStride / sizeof(GLchan));
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
               _mesa_unpack_color_span_float(ctx, srcWidth, GL_RGBA, dstf,
                 srcFormat, srcType, src, srcPacking,
                 (transferOps & IMAGE_PRE_CONVOLUTION_BITS) | IMAGE_CLAMP_BIT);
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
            dest = (GLchan *) texDestAddr
                 + (dstZoffset + img) * (dstImageStride / sizeof(GLchan))
                 + dstYoffset * (dstRowStride / sizeof(GLchan));
            for (row = 0; row < convHeight; row++) {
               _mesa_pack_rgba_span_float(ctx, convWidth,
                                          (const GLfloat (*)[4]) srcf,
                                          texDestFormat, CHAN_TYPE,
                                          dest, &ctx->DefaultPacking,
                                          transferOps
                                          & IMAGE_POST_CONVOLUTION_BITS);
               srcf += convWidth * 4;
               dest += (dstRowStride / sizeof(GLchan));
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
         GLchan *dest = (GLchan *) texDestAddr
                      + dstZoffset * (dstImageStride / sizeof(GLchan))
                      + dstYoffset * (dstRowStride / sizeof(GLchan))
                      + dstXoffset * texComponents;
         for (img = 0; img < srcDepth; img++) {
            GLchan *destRow = dest;
            for (row = 0; row < srcHeight; row++) {
               const GLvoid *srcRow = _mesa_image_address(srcPacking,
                                              srcAddr, srcWidth, srcHeight,
                                              srcFormat, srcType, img, row, 0);
               _mesa_unpack_color_span_chan(ctx, srcWidth, texDestFormat,
                                       destRow, srcFormat, srcType, srcRow,
                                       srcPacking, transferOps);
               destRow += (dstRowStride / sizeof(GLchan));
            }
            dest += dstImageStride / sizeof(GLchan);
         }
      }
   }
}



/*
 * Transfer a texture image from user space to <destAddr> applying all
 * needed image transfer operations and storing the result in the format
 * specified by <dstFormat>.  <dstFormat> may be any format from texformat.h.
 * \param
 *   dimensions - 1, 2 or 3
 *   baseInternalFormat - base format of the internal texture format
 *       specified by the user.  This is very important, see below.
 *   dstFormat - destination image format
 *   dstAddr - destination address
 *   srcWidth, srcHeight, srcDepth - size of source iamge
 *   dstX/Y/Zoffset - as specified by glTexSubImage
 *   dstRowStride - stride between dest rows in bytes
 *   dstImageStride - stride between dest images in bytes
 *   srcFormat, srcType - incoming image format and data type
 *   srcAddr - source image address
 *   srcPacking - packing params of source image
 *
 * XXX this function is a bit more complicated than it should be.  If
 * _mesa_convert_texsubimage[123]d could handle any dest/source formats
 * or if transfer_teximage() could store in any MESA_FORMAT_* format, we
 * could simplify things here.
 */
void
_mesa_transfer_teximage(GLcontext *ctx, GLuint dimensions,
                        GLenum baseInternalFormat,
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
   GLuint transferOps = ctx->_ImageTransferState;
   GLboolean freeSourceData = GL_FALSE;
   GLint postConvWidth = srcWidth, postConvHeight = srcHeight;

   assert(baseInternalFormat > 0);
   ASSERT(baseInternalFormat == GL_LUMINANCE ||
          baseInternalFormat == GL_INTENSITY ||
          baseInternalFormat == GL_LUMINANCE_ALPHA ||
          baseInternalFormat == GL_ALPHA ||
          baseInternalFormat == GL_RGB ||
          baseInternalFormat == GL_RGBA ||
          baseInternalFormat == GL_YCBCR_MESA ||
          baseInternalFormat == GL_COLOR_INDEX ||
          baseInternalFormat == GL_DEPTH_COMPONENT);

   if (transferOps & IMAGE_CONVOLUTION_BIT) {
      _mesa_adjust_image_for_convolution(ctx, dimensions, &postConvWidth,
                                         &postConvHeight);
   }

   /*
    * Consider this scenario:  The user's source image is GL_RGB and the
    * requested internal format is GL_LUMINANCE.  Now suppose the device
    * driver doesn't support GL_LUMINANCE and instead uses RGB16 as the
    * texture format.  In that case we still need to do an intermediate
    * conversion to luminance format so that the incoming red channel gets
    * replicated into the dest red, green and blue channels.  The following
    * code takes care of that.
    */
   if (dstFormat->BaseFormat != baseInternalFormat) {
      /* Allocate storage for temporary image in the baseInternalFormat */
      const GLint texelSize = _mesa_components_in_format(baseInternalFormat)
         * sizeof(GLchan);
      const GLint bytes = texelSize * postConvWidth * postConvHeight *srcDepth;
      const GLint tmpRowStride = texelSize * postConvWidth;
      const GLint tmpImgStride = texelSize * postConvWidth * postConvHeight;
      GLvoid *tmpImage = MALLOC(bytes);
      if (!tmpImage)
         return;
      transfer_teximage(ctx, dimensions, baseInternalFormat, tmpImage,
                        srcWidth, srcHeight, srcDepth,
                        0, 0, 0, /* x/y/zoffset */
                        tmpRowStride, tmpImgStride,
                        srcFormat, srcType, srcAddr, srcPacking, transferOps);

      /* this is our new source image */
      srcWidth = postConvWidth;
      srcHeight = postConvHeight;
      srcFormat = baseInternalFormat;
      srcType = CHAN_TYPE;
      srcAddr = tmpImage;
      srcPacking = &ctx->DefaultPacking;
      freeSourceData = GL_TRUE;
      transferOps = 0;  /* image transfer ops were completed */
   }

   /* Let the optimized tex conversion functions take a crack at the
    * image conversion if the dest format is a h/w format.
    */
   if (_mesa_is_hardware_tex_format(dstFormat)) {
      if (transferOps) {
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
            if (freeSourceData)
               FREE((void *) srcAddr);
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

      if (transferOps & IMAGE_CONVOLUTION_BIT) {
         _mesa_adjust_image_for_convolution(ctx, dimensions, &postConvWidth,
                                            &postConvHeight);
      }

      tmpFormat = dstFormat->BaseFormat;
      tmpComps = _mesa_components_in_format(tmpFormat);
      tmpTexelSize = tmpComps * sizeof(GLchan);
      tmpRowStride = postConvWidth * tmpTexelSize;
      tmpImageStride = postConvWidth * postConvHeight * tmpTexelSize;
      tmpImage = (GLubyte *) MALLOC(postConvWidth * postConvHeight *
                                    srcDepth * tmpTexelSize);
      if (!tmpImage) {
         if (freeSourceData)
            FREE((void *) srcAddr);
         return;
      }

      transfer_teximage(ctx, dimensions, tmpFormat, tmpImage,
                        srcWidth, srcHeight, srcDepth,
                        0, 0, 0, /* x/y/zoffset */
                        tmpRowStride, tmpImageStride,
                        srcFormat, srcType, srcAddr, srcPacking, transferOps);

      if (freeSourceData)
         FREE((void *) srcAddr);

      /* the temp image is our new source image */
      srcWidth = postConvWidth;
      srcHeight = postConvHeight;
      srcFormat = tmpFormat;
      srcType = CHAN_TYPE;
      srcAddr = tmpImage;
      srcPacking = &ctx->DefaultPacking;
      freeSourceData = GL_TRUE;
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
         (void) b;
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
         (void) b;
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
         (void) b;
      }
   }
   else {
      /* software format */
      assert(!makeTemp);
      transfer_teximage(ctx, dimensions, dstFormat->BaseFormat, dstAddr,
                        srcWidth, srcHeight, srcDepth,
                        dstXoffset, dstYoffset, dstZoffset,
                        dstRowStride, dstImageStride,
                        srcFormat, srcType, srcAddr, srcPacking, transferOps);
   }

   if (freeSourceData)
      FREE((void *) srcAddr);  /* the temp image */
}



/**
 * Given a user's uncompressed texture image, this function takes care of
 * pixel unpacking, pixel transfer, format conversion and compression.
 */
static void
transfer_compressed_teximage(GLcontext *ctx, GLuint dimensions,
                             GLsizei width, GLsizei height, GLsizei depth,
                             GLenum srcFormat, GLenum srcType,
                             const struct gl_pixelstore_attrib *unpacking,
                             const GLvoid *source,
                             const struct gl_texture_format *dstFormat,
                             GLubyte *dest,
                             GLint dstRowStride)
{
   GLchan *tempImage = NULL;
   GLint srcRowStride;
   GLenum baseFormat;

   ASSERT(dimensions == 2);
   /* TexelBytes is zero if and only if it's a compressed format */
   ASSERT(dstFormat->TexelBytes == 0);

   baseFormat = dstFormat->BaseFormat;

   if (srcFormat != baseFormat || srcType != CHAN_TYPE ||
       ctx->_ImageTransferState != 0 || unpacking->SwapBytes) {
      /* need to convert user's image to texImage->Format, GLchan */
      GLint comps = components_in_intformat(baseFormat);
      GLint postConvWidth = width, postConvHeight = height;

      /* XXX convolution untested */
      if (ctx->_ImageTransferState & IMAGE_CONVOLUTION_BIT) {
         _mesa_adjust_image_for_convolution(ctx, dimensions, &postConvWidth,
                                            &postConvHeight);
      }

      tempImage = (GLchan*) MALLOC(width * height * comps * sizeof(GLchan));
      if (!tempImage) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage2D");
         return;
      }
      transfer_teximage(ctx, dimensions,
                        baseFormat,             /* dest format */
                        tempImage,              /* dst address */
                        width, height, depth,   /* src size */
                        0, 0, 0,                /* x/y/zoffset */
                        comps * width,          /* dst row stride */
                        comps * width * height, /* dst image stride */
                        srcFormat, srcType,     /* src format, type */
                        source, unpacking,      /* src and src packing */
                        ctx->_ImageTransferState);
      source = tempImage;
      width = postConvWidth;
      height = postConvHeight;
      srcRowStride = width;
   }
   else {
      if (unpacking->RowLength)
         srcRowStride = unpacking->RowLength;
      else
         srcRowStride = width;
   }

   _mesa_compress_teximage(ctx, width, height, baseFormat,
                           (const GLchan *) source, srcRowStride,
                           dstFormat, dest, dstRowStride);
   if (tempImage) {
      FREE(tempImage);
   }
}


#endif /************************************************************/




/**
 * Validate acces to a PBO for texture data.
 *
 * \todo If the PBO is really resident in VRAM, this won't work; the
 * device driver should check for that and do the right thing.
 */
static const GLvoid *
validate_pbo_teximage( GLsizei width, GLsizei height, GLsizei depth,
                       GLenum format, GLenum type, const GLvoid *pixels,
                       const struct gl_pixelstore_attrib *unpack )
{
   if (unpack->BufferObj->Name == 0) {
      /* no PBO */
      return pixels;
   }
   else if (_mesa_validate_pbo_access(unpack, width, height, depth, format,
                                      type, pixels)) {
      return ADD_POINTERS(unpack->BufferObj->Data, pixels);
   }
   /* bad access! */
   return NULL;
}


/**
 * Validate that unpacking compressed texture image data from a PBO
 * won't go out of bounds.
 *
 * \todo If the PBO is really resident in VRAM, this won't work; the
 * device driver should check for that and do the right thing.
 */
static const GLvoid *
validate_pbo_compressed_teximage(GLsizei imageSize, const GLvoid *pixels,
                               const struct gl_pixelstore_attrib *packing)
{
   if (packing->BufferObj->Name == 0) {
      /* not using a PBO - return pointer unchanged */
      return pixels;
   }
   else {
      /* using a PBO */
      if ((const GLubyte *) pixels + imageSize >
          (const GLubyte *) packing->BufferObj->Size) {
         /* out of bounds read! */
         return NULL;
      }
      /* OK! */
      return ADD_POINTERS(packing->BufferObj->Data, pixels);
   }
}



/*
 * This is the software fallback for Driver.TexImage1D()
 * and Driver.CopyTexImage1D().
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
   GLint sizeInBytes;

   if (ctx->_ImageTransferState & IMAGE_CONVOLUTION_BIT) {
      _mesa_adjust_image_for_convolution(ctx, 1, &postConvWidth, NULL);
   }

   /* choose the texture format */
   assert(ctx->Driver.ChooseTextureFormat);
   texImage->TexFormat = ctx->Driver.ChooseTextureFormat(ctx, internalFormat,
                                                         format, type);
   assert(texImage->TexFormat);
   texImage->FetchTexelc = texImage->TexFormat->FetchTexel1D;
   texImage->FetchTexelf = texImage->TexFormat->FetchTexel1Df;

   /* allocate memory */
   if (texImage->IsCompressed)
      sizeInBytes = texImage->CompressedSize;
   else
      sizeInBytes = postConvWidth * texImage->TexFormat->TexelBytes;
   texImage->Data = MESA_PBUFFER_ALLOC(sizeInBytes);
   if (!texImage->Data) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage1D");
      return;
   }

   pixels = validate_pbo_teximage(width, 1, 1, format, type, pixels, packing);
   if (!pixels)
      return;

#if NEWTEXSTORE
   {
      const GLint dstRowStride = 0, dstImageStride = 0;
      GLboolean success;
      ASSERT(texImage->TexFormat->StoreImage);
      success = texImage->TexFormat->StoreImage(ctx, 1, texImage->Format,
                                                texImage->TexFormat,
                                                texImage->Data,
                                                0, 0, 0,  /* dstX/Y/Zoffset */
                                                dstRowStride, dstImageStride,
                                                width, 1, 1,
                                                format, type, pixels, packing);
      if (!success) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage1D");
         return;
      }
   }
#else
   /* unpack image, apply transfer ops and store in texImage->Data */
   if (texImage->IsCompressed) {
      GLint dstRowStride = _mesa_compressed_row_stride(texImage->IntFormat,
                                                       width);
      transfer_compressed_teximage(ctx, 1, width, 1, 1,
                                   format, type, packing,
                                   pixels, texImage->TexFormat,
                                   (GLubyte *) texImage->Data, dstRowStride);
   }
   else {
      _mesa_transfer_teximage(ctx, 1,
                              texImage->Format, /* base format */
                              texImage->TexFormat, texImage->Data,
                              width, 1, 1,  /* src size */
                              0, 0, 0,      /* dstX/Y/Zoffset */
                              0, /* dstRowStride */
                              0, /* dstImageStride */
                              format, type, pixels, packing);
   }
#endif

   /* GL_SGIS_generate_mipmap */
   if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
      _mesa_generate_mipmap(ctx, target,
                            &ctx->Texture.Unit[ctx->Texture.CurrentUnit],
                            texObj);
   }
}


/*
 * This is the software fallback for Driver.TexImage2D()
 * and Driver.CopyTexImage2D().
 * Reasons why a driver might override this function:
 *  - Special memory allocation needs
 *  - Unusual row/image strides
 *  - Special housekeeping
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
   GLint texelBytes, sizeInBytes;

   if (ctx->_ImageTransferState & IMAGE_CONVOLUTION_BIT) {
      _mesa_adjust_image_for_convolution(ctx, 2, &postConvWidth,
                                         &postConvHeight);
   }

   /* choose the texture format */
   assert(ctx->Driver.ChooseTextureFormat);
   texImage->TexFormat = (*ctx->Driver.ChooseTextureFormat)(ctx,
                                          internalFormat, format, type);
   assert(texImage->TexFormat);
   texImage->FetchTexelc = texImage->TexFormat->FetchTexel2D;
   texImage->FetchTexelf = texImage->TexFormat->FetchTexel2Df;

   texelBytes = texImage->TexFormat->TexelBytes;

   /* allocate memory */
   if (texImage->IsCompressed)
      sizeInBytes = texImage->CompressedSize;
   else
      sizeInBytes = postConvWidth * postConvHeight * texelBytes;
   texImage->Data = MESA_PBUFFER_ALLOC(sizeInBytes);
   if (!texImage->Data) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage2D");
      return;
   }

   pixels = validate_pbo_teximage(width, height, 1,
                                  format, type, pixels, packing);
   if (!pixels)
      return;

#if NEWTEXSTORE
   {
      GLint dstRowStride, dstImageStride = 0;
      GLboolean success;
      if (texImage->IsCompressed) {
         dstRowStride = _mesa_compressed_row_stride(texImage->IntFormat,width);
      }
      else {
         dstRowStride = width * texImage->TexFormat->TexelBytes;
      }
      ASSERT(texImage->TexFormat->StoreImage);
      success = texImage->TexFormat->StoreImage(ctx, 2, texImage->Format,
                                                texImage->TexFormat,
                                                texImage->Data,
                                                0, 0, 0,  /* dstX/Y/Zoffset */
                                                dstRowStride, dstImageStride,
                                                width, height, 1,
                                                format, type, pixels, packing);
      if (!success) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage2D");
         return;
      }
   }
#else
   /* unpack image, apply transfer ops and store in texImage->Data */
   if (texImage->IsCompressed) {
      GLint dstRowStride = _mesa_compressed_row_stride(texImage->IntFormat,
                                                       width);
      transfer_compressed_teximage(ctx, 2, width, height, 1,
                                   format, type, packing,
                                   pixels, texImage->TexFormat,
                                   (GLubyte *) texImage->Data, dstRowStride);
   }
   else {
      _mesa_transfer_teximage(ctx, 2,
                              texImage->Format,
                              texImage->TexFormat, texImage->Data,
                              width, height, 1,  /* src size */
                              0, 0, 0,           /* dstX/Y/Zoffset */
                              texImage->Width * texelBytes, /* dstRowStride */
                              0, /* dstImageStride */
                              format, type, pixels, packing);
   }
#endif

   /* GL_SGIS_generate_mipmap */
   if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
      _mesa_generate_mipmap(ctx, target,
                            &ctx->Texture.Unit[ctx->Texture.CurrentUnit],
                            texObj);
   }
}



/*
 * This is the software fallback for Driver.TexImage3D()
 * and Driver.CopyTexImage3D().
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
   GLint texelBytes, sizeInBytes;

   /* choose the texture format */
   assert(ctx->Driver.ChooseTextureFormat);
   texImage->TexFormat = (*ctx->Driver.ChooseTextureFormat)(ctx,
                                          internalFormat, format, type);
   assert(texImage->TexFormat);
   texImage->FetchTexelc = texImage->TexFormat->FetchTexel3D;
   texImage->FetchTexelf = texImage->TexFormat->FetchTexel3Df;

   texelBytes = texImage->TexFormat->TexelBytes;

   /* allocate memory */
   if (texImage->IsCompressed)
      sizeInBytes = texImage->CompressedSize;
   else
      sizeInBytes = width * height * depth * texelBytes;
   texImage->Data = MESA_PBUFFER_ALLOC(sizeInBytes);
   if (!texImage->Data) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage3D");
      return;
   }

   pixels = validate_pbo_teximage(width, height, depth,
                                  format, type, pixels, packing);
   if (!pixels)
      return;

   /* unpack image, apply transfer ops and store in texImage->Data */
#if NEWTEXSTORE
   {
      GLint dstRowStride, dstImageStride;
      GLboolean success;
      if (texImage->IsCompressed) {
         dstRowStride = _mesa_compressed_row_stride(texImage->IntFormat,width);
         dstImageStride = 0;
      }
      else {
         dstRowStride = width * texImage->TexFormat->TexelBytes;
         dstImageStride = dstRowStride * height;
      }
      ASSERT(texImage->TexFormat->StoreImage);
      success = texImage->TexFormat->StoreImage(ctx, 3, texImage->Format,
                                                texImage->TexFormat,
                                                texImage->Data,
                                                0, 0, 0,  /* dstX/Y/Zoffset */
                                                dstRowStride, dstImageStride,
                                                width, height, depth,
                                                format, type, pixels, packing);
      if (!success) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage3D");
         return;
      }
   }
#else
   if (texImage->IsCompressed) {
      GLint dstRowStride = _mesa_compressed_row_stride(texImage->IntFormat,
                                                       width);
      transfer_compressed_teximage(ctx, 3, width, height, depth,
                                   format, type, packing,
                                   pixels, texImage->TexFormat,
                                   (GLubyte *) texImage->Data, dstRowStride);
   }
   else {
      _mesa_transfer_teximage(ctx, 3,
                              texImage->Format,
                              texImage->TexFormat, texImage->Data,
                              width, height, depth, /* src size */
                              0, 0, 0,  /* dstX/Y/Zoffset */
                              texImage->Width * texelBytes, /* dstRowStride */
                              texImage->Width * texImage->Height * texelBytes,
                              format, type, pixels, packing);
   }
#endif

   /* GL_SGIS_generate_mipmap */
   if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
      _mesa_generate_mipmap(ctx, target,
                            &ctx->Texture.Unit[ctx->Texture.CurrentUnit],
                            texObj);
   }
}




/*
 * This is the software fallback for Driver.TexSubImage1D()
 * and Driver.CopyTexSubImage1D().
 */
void
_mesa_store_texsubimage1d(GLcontext *ctx, GLenum target, GLint level,
                          GLint xoffset, GLint width,
                          GLenum format, GLenum type, const void *pixels,
                          const struct gl_pixelstore_attrib *packing,
                          struct gl_texture_object *texObj,
                          struct gl_texture_image *texImage)
{
   pixels = validate_pbo_teximage(width, 1, 1,
                                  format, type, pixels, packing);
   if (!pixels)
      return;

#if NEWTEXSTORE
   {
      const GLint dstRowStride = 0, dstImageStride = 0;
      GLboolean success;
      ASSERT(texImage->TexFormat->StoreImage);
      success = texImage->TexFormat->StoreImage(ctx, 1, texImage->Format,
                                                texImage->TexFormat,
                                                texImage->Data,
                                                xoffset, 0, 0,  /* offsets */
                                                dstRowStride, dstImageStride,
                                                width, 1, 1,
                                                format, type, pixels, packing);
      if (!success) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexSubImage1D");
         return;
      }
   }
#else
   if (texImage->IsCompressed) {
      GLint dstRowStride = _mesa_compressed_row_stride(texImage->IntFormat,
                                                       texImage->Width);
      GLubyte *dest = _mesa_compressed_image_address(xoffset, 0, 0,
                                                     texImage->IntFormat,
                                                     texImage->Width,
                                          (GLubyte*) texImage->Data);
      transfer_compressed_teximage(ctx, 1,             /* dimensions */
                                   width, 1, 1,        /* size to replace */
                                   format, type,       /* source format/type */
                                   packing,            /* source packing */
                                   pixels,             /* source data */
                                   texImage->TexFormat,/* dest format */
                                   dest, dstRowStride);
   }
   else {
      _mesa_transfer_teximage(ctx, 1,
                              texImage->Format,
                              texImage->TexFormat, texImage->Data,
                              width, 1, 1, /* src size */
                              xoffset, 0, 0, /* dest offsets */
                              0, /* dstRowStride */
                              0, /* dstImageStride */
                              format, type, pixels, packing);
   }
#endif

   /* GL_SGIS_generate_mipmap */
   if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
      _mesa_generate_mipmap(ctx, target,
                            &ctx->Texture.Unit[ctx->Texture.CurrentUnit],
                            texObj);
   }
}



/**
 * This is the software fallback for Driver.TexSubImage2D()
 * and Driver.CopyTexSubImage2D().
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
   pixels = validate_pbo_teximage(width, height, 1,
                                  format, type, pixels, packing);
   if (!pixels)
      return;

#if NEWTEXSTORE
   {
      GLint dstRowStride = 0, dstImageStride = 0;
      GLboolean success;
      if (texImage->IsCompressed) {
         dstRowStride = _mesa_compressed_row_stride(texImage->IntFormat,
                                                    texImage->Width);
      }
      else {
         dstRowStride = texImage->Width * texImage->TexFormat->TexelBytes;
      }
      ASSERT(texImage->TexFormat->StoreImage);
      success = texImage->TexFormat->StoreImage(ctx, 2, texImage->Format,
                                                texImage->TexFormat,
                                                texImage->Data,
                                                xoffset, yoffset, 0,
                                                dstRowStride, dstImageStride,
                                                width, height, 1,
                                                format, type, pixels, packing);
      if (!success) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexSubImage2D");
         return;
      }
   }
#else

   if (texImage->IsCompressed) {
      GLint dstRowStride = _mesa_compressed_row_stride(texImage->IntFormat,
                                                       texImage->Width);
      GLubyte *dest = _mesa_compressed_image_address(xoffset, yoffset, 0,
                                                     texImage->IntFormat,
                                                     texImage->Width,
                                          (GLubyte*) texImage->Data);
      transfer_compressed_teximage(ctx, 2,             /* dimensions */
                                   width, height, 1,   /* size to replace */
                                   format, type,       /* source format/type */
                                   packing,            /* source packing */
                                   pixels,             /* source data */
                                   texImage->TexFormat,/* dest format */
                                   dest, dstRowStride);
   }
   else {
         /* old path */
      _mesa_transfer_teximage(ctx, 2,
                              texImage->Format,
                              texImage->TexFormat, texImage->Data,
                              width, height, 1, /* src size */
                              xoffset, yoffset, 0, /* dest offsets */
                              texImage->Width *texImage->TexFormat->TexelBytes,
                              0, /* dstImageStride */
                              format, type, pixels, packing);
   }
#endif

   /* GL_SGIS_generate_mipmap */
   if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
      _mesa_generate_mipmap(ctx, target,
                            &ctx->Texture.Unit[ctx->Texture.CurrentUnit],
                            texObj);
   }
}


/*
 * This is the software fallback for Driver.TexSubImage3D().
 * and Driver.CopyTexSubImage3D().
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
   pixels = validate_pbo_teximage(width, height, depth,
                                  format, type, pixels, packing);
   if (!pixels)
      return;

#if NEWTEXSTORE
   {
      GLint dstRowStride, dstImageStride;
      GLboolean success;
      if (texImage->IsCompressed) {
         dstRowStride = _mesa_compressed_row_stride(texImage->IntFormat,
                                                    texImage->Width);
         dstImageStride = 0; /* XXX fix */
      }
      else {
         dstRowStride = texImage->Width * texImage->TexFormat->TexelBytes;
         dstImageStride = dstRowStride * texImage->Height;
      }
      ASSERT(texImage->TexFormat->StoreImage);
      success = texImage->TexFormat->StoreImage(ctx, 3, texImage->Format,
                                                texImage->TexFormat,
                                                texImage->Data,
                                                xoffset, yoffset, zoffset,
                                                dstRowStride, dstImageStride,
                                                width, height, depth,
                                                format, type, pixels, packing);
      if (!success) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexSubImage3D");
         return;
      }
   }
#else
   if (texImage->IsCompressed) {
      GLint dstRowStride = _mesa_compressed_row_stride(texImage->IntFormat,
                                                       texImage->Width);
      GLubyte *dest = _mesa_compressed_image_address(xoffset, yoffset, zoffset,
                                                     texImage->IntFormat,
                                                     texImage->Width,
                                          (GLubyte*) texImage->Data);
      transfer_compressed_teximage(ctx, 3,              /* dimensions */
                                   width, height, depth,/* size to replace */
                                   format, type,       /* source format/type */
                                   packing,            /* source packing */
                                   pixels,             /* source data */
                                   texImage->TexFormat,/* dest format */
                                   dest, dstRowStride);
   }
   else {
      const GLint texelBytes = texImage->TexFormat->TexelBytes;
      _mesa_transfer_teximage(ctx, 3,
                           texImage->Format,
                           texImage->TexFormat, texImage->Data,
                           width, height, depth, /* src size */
                           xoffset, yoffset, zoffset, /* dest offsets */
                           texImage->Width * texelBytes,  /* dst row stride */
                           texImage->Width * texImage->Height * texelBytes,
                           format, type, pixels, packing);
   }
#endif
   /* GL_SGIS_generate_mipmap */
   if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
      _mesa_generate_mipmap(ctx, target,
                            &ctx->Texture.Unit[ctx->Texture.CurrentUnit],
                            texObj);
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
   /* this space intentionally left blank */
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
   /* This is pretty simple, basically just do a memcpy without worrying
    * about the usual image unpacking or image transfer operations.
    */
   ASSERT(texObj);
   ASSERT(texImage);
   ASSERT(texImage->Width > 0);
   ASSERT(texImage->Height > 0);
   ASSERT(texImage->Depth == 1);
   ASSERT(texImage->Data == NULL); /* was freed in glCompressedTexImage2DARB */

   /* choose the texture format */
   assert(ctx->Driver.ChooseTextureFormat);
   texImage->TexFormat = (*ctx->Driver.ChooseTextureFormat)(ctx,
                                          internalFormat, 0, 0);
   assert(texImage->TexFormat);
   texImage->FetchTexelc = texImage->TexFormat->FetchTexel2D;
   texImage->FetchTexelf = texImage->TexFormat->FetchTexel2Df;

   /* allocate storage */
   texImage->Data = MESA_PBUFFER_ALLOC(imageSize);
   if (!texImage->Data) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCompressedTexImage2DARB");
      return;
   }

   data = validate_pbo_compressed_teximage(imageSize, data, &ctx->Unpack);
   if (!data)
      return;

   /* copy the data */
   ASSERT(texImage->CompressedSize == (GLuint) imageSize);
   MEMCPY(texImage->Data, data, imageSize);
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
   /* this space intentionally left blank */
}



/**
 * Fallback for Driver.CompressedTexSubImage1D()
 */
void
_mesa_store_compressed_texsubimage1d(GLcontext *ctx, GLenum target,
                                     GLint level,
                                     GLint xoffset, GLsizei width,
                                     GLenum format,
                                     GLsizei imageSize, const GLvoid *data,
                                     struct gl_texture_object *texObj,
                                     struct gl_texture_image *texImage)
{
   /* this space intentionally left blank */
}


/**
 * Fallback for Driver.CompressedTexSubImage2D()
 */
void
_mesa_store_compressed_texsubimage2d(GLcontext *ctx, GLenum target,
                                     GLint level,
                                     GLint xoffset, GLint yoffset,
                                     GLsizei width, GLsizei height,
                                     GLenum format,
                                     GLsizei imageSize, const GLvoid *data,
                                     struct gl_texture_object *texObj,
                                     struct gl_texture_image *texImage)
{
   GLint bytesPerRow, destRowStride, srcRowStride;
   GLint i, rows;
   GLubyte *dest;
   const GLubyte *src;

   /* these should have been caught sooner */
   ASSERT((width & 3) == 0 || width == 2 || width == 1);
   ASSERT((height & 3) == 0 || height == 2 || height == 1);
   ASSERT((xoffset & 3) == 0);
   ASSERT((yoffset & 3) == 0);

   data = validate_pbo_compressed_teximage(imageSize, data, &ctx->Unpack);
   if (!data)
      return;

   srcRowStride = _mesa_compressed_row_stride(texImage->IntFormat, width);
   src = (const GLubyte *) data;

   destRowStride = _mesa_compressed_row_stride(texImage->IntFormat,
                                               texImage->Width);
   dest = _mesa_compressed_image_address(xoffset, yoffset, 0,
                                         texImage->IntFormat,
                                         texImage->Width,
                              (GLubyte*) texImage->Data);

   bytesPerRow = srcRowStride;
   rows = height / 4;

   for (i = 0; i < rows; i++) {
      MEMCPY(dest, src, bytesPerRow);
      dest += destRowStride;
      src += srcRowStride;
   }
}


/**
 * Fallback for Driver.CompressedTexSubImage3D()
 */
void
_mesa_store_compressed_texsubimage3d(GLcontext *ctx, GLenum target,
                                GLint level,
                                GLint xoffset, GLint yoffset, GLint zoffset,
                                GLsizei width, GLsizei height, GLsizei depth,
                                GLenum format,
                                GLsizei imageSize, const GLvoid *data,
                                struct gl_texture_object *texObj,
                                struct gl_texture_image *texImage)
{
   /* this space intentionally left blank */
}


/*
 * Average together two rows of a source image to produce a single new
 * row in the dest image.  It's legal for the two source rows to point
 * to the same data.  The source width must be equal to either the
 * dest width or two times the dest width.
 */
static void
do_row(const struct gl_texture_format *format, GLint srcWidth,
       const GLvoid *srcRowA, const GLvoid *srcRowB,
       GLint dstWidth, GLvoid *dstRow)
{
   const GLuint k0 = (srcWidth == dstWidth) ? 0 : 1;
   const GLuint colStride = (srcWidth == dstWidth) ? 1 : 2;

   /* This assertion is no longer valid with non-power-of-2 textures
   assert(srcWidth == dstWidth || srcWidth == 2 * dstWidth);
   */

   switch (format->MesaFormat) {
   case MESA_FORMAT_RGBA:
      {
         GLuint i, j, k;
         const GLchan (*rowA)[4] = (const GLchan (*)[4]) srcRowA;
         const GLchan (*rowB)[4] = (const GLchan (*)[4]) srcRowB;
         GLchan (*dst)[4] = (GLchan (*)[4]) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            dst[i][0] = (rowA[j][0] + rowA[k][0] +
                         rowB[j][0] + rowB[k][0]) / 4;
            dst[i][1] = (rowA[j][1] + rowA[k][1] +
                         rowB[j][1] + rowB[k][1]) / 4;
            dst[i][2] = (rowA[j][2] + rowA[k][2] +
                         rowB[j][2] + rowB[k][2]) / 4;
            dst[i][3] = (rowA[j][3] + rowA[k][3] +
                         rowB[j][3] + rowB[k][3]) / 4;
         }
      }
      return;
   case MESA_FORMAT_RGB:
      {
         GLuint i, j, k;
         const GLchan (*rowA)[3] = (const GLchan (*)[3]) srcRowA;
         const GLchan (*rowB)[3] = (const GLchan (*)[3]) srcRowB;
         GLchan (*dst)[3] = (GLchan (*)[3]) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            dst[i][0] = (rowA[j][0] + rowA[k][0] +
                         rowB[j][0] + rowB[k][0]) / 4;
            dst[i][1] = (rowA[j][1] + rowA[k][1] +
                         rowB[j][1] + rowB[k][1]) / 4;
            dst[i][2] = (rowA[j][2] + rowA[k][2] +
                         rowB[j][2] + rowB[k][2]) / 4;
         }
      }
      return;
   case MESA_FORMAT_ALPHA:
   case MESA_FORMAT_LUMINANCE:
   case MESA_FORMAT_INTENSITY:
   case MESA_FORMAT_COLOR_INDEX:
      {
         GLuint i, j, k;
         const GLchan *rowA = (const GLchan *) srcRowA;
         const GLchan *rowB = (const GLchan *) srcRowB;
         GLchan *dst = (GLchan *) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            dst[i] = (rowA[j] + rowA[k] + rowB[j] + rowB[k]) / 4;
         }
      }
      return;
   case MESA_FORMAT_LUMINANCE_ALPHA:
      {
         GLuint i, j, k;
         const GLchan (*rowA)[2] = (const GLchan (*)[2]) srcRowA;
         const GLchan (*rowB)[2] = (const GLchan (*)[2]) srcRowB;
         GLchan (*dst)[2] = (GLchan (*)[2]) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            dst[i][0] = (rowA[j][0] + rowA[k][0] +
                         rowB[j][0] + rowB[k][0]) / 4;
            dst[i][1] = (rowA[j][1] + rowA[k][1] +
                         rowB[j][1] + rowB[k][1]) / 4;
         }
      }
      return;
   case MESA_FORMAT_DEPTH_COMPONENT_FLOAT32:
      {
         GLuint i, j, k;
         const GLfloat *rowA = (const GLfloat *) srcRowA;
         const GLfloat *rowB = (const GLfloat *) srcRowB;
         GLfloat *dst = (GLfloat *) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            dst[i] = (rowA[j] + rowA[k] + rowB[j] + rowB[k]) * 0.25F;
         }
      }
      return;
   case MESA_FORMAT_DEPTH_COMPONENT16:
      {
         GLuint i, j, k;
         const GLushort *rowA = (const GLushort *) srcRowA;
         const GLushort *rowB = (const GLushort *) srcRowB;
         GLushort *dst = (GLushort *) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            dst[i] = (rowA[j] + rowA[k] + rowB[j] + rowB[k]) / 4;
         }
      }
      return;
   /* Begin hardware formats */
   case MESA_FORMAT_RGBA8888:
   case MESA_FORMAT_ARGB8888:
      {
         GLuint i, j, k;
         const GLubyte (*rowA)[4] = (const GLubyte (*)[4]) srcRowA;
         const GLubyte (*rowB)[4] = (const GLubyte (*)[4]) srcRowB;
         GLubyte (*dst)[4] = (GLubyte (*)[4]) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            dst[i][0] = (rowA[j][0] + rowA[k][0] +
                         rowB[j][0] + rowB[k][0]) / 4;
            dst[i][1] = (rowA[j][1] + rowA[k][1] +
                         rowB[j][1] + rowB[k][1]) / 4;
            dst[i][2] = (rowA[j][2] + rowA[k][2] +
                         rowB[j][2] + rowB[k][2]) / 4;
            dst[i][3] = (rowA[j][3] + rowA[k][3] +
                         rowB[j][3] + rowB[k][3]) / 4;
         }
      }
      return;
   case MESA_FORMAT_RGB888:
      {
         GLuint i, j, k;
         const GLubyte (*rowA)[3] = (const GLubyte (*)[3]) srcRowA;
         const GLubyte (*rowB)[3] = (const GLubyte (*)[3]) srcRowB;
         GLubyte (*dst)[3] = (GLubyte (*)[3]) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            dst[i][0] = (rowA[j][0] + rowA[k][0] +
                         rowB[j][0] + rowB[k][0]) / 4;
            dst[i][1] = (rowA[j][1] + rowA[k][1] +
                         rowB[j][1] + rowB[k][1]) / 4;
            dst[i][2] = (rowA[j][2] + rowA[k][2] +
                         rowB[j][2] + rowB[k][2]) / 4;
         }
      }
      return;
   case MESA_FORMAT_RGB565:
      {
         GLuint i, j, k;
         const GLushort *rowA = (const GLushort *) srcRowA;
         const GLushort *rowB = (const GLushort *) srcRowB;
         GLushort *dst = (GLushort *) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            const GLint rowAr0 = rowA[j] & 0x1f;
            const GLint rowAr1 = rowA[k] & 0x1f;
            const GLint rowBr0 = rowB[j] & 0x1f;
            const GLint rowBr1 = rowB[k] & 0x1f;
            const GLint rowAg0 = (rowA[j] >> 5) & 0x3f;
            const GLint rowAg1 = (rowA[k] >> 5) & 0x3f;
            const GLint rowBg0 = (rowB[j] >> 5) & 0x3f;
            const GLint rowBg1 = (rowB[k] >> 5) & 0x3f;
            const GLint rowAb0 = (rowA[j] >> 11) & 0x1f;
            const GLint rowAb1 = (rowA[k] >> 11) & 0x1f;
            const GLint rowBb0 = (rowB[j] >> 11) & 0x1f;
            const GLint rowBb1 = (rowB[k] >> 11) & 0x1f;
            const GLint red   = (rowAr0 + rowAr1 + rowBr0 + rowBr1) >> 2;
            const GLint green = (rowAg0 + rowAg1 + rowBg0 + rowBg1) >> 2;
            const GLint blue  = (rowAb0 + rowAb1 + rowBb0 + rowBb1) >> 2;
            dst[i] = (blue << 11) | (green << 5) | red;
         }
      }
      return;
   case MESA_FORMAT_ARGB4444:
      {
         GLuint i, j, k;
         const GLushort *rowA = (const GLushort *) srcRowA;
         const GLushort *rowB = (const GLushort *) srcRowB;
         GLushort *dst = (GLushort *) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            const GLint rowAr0 = rowA[j] & 0xf;
            const GLint rowAr1 = rowA[k] & 0xf;
            const GLint rowBr0 = rowB[j] & 0xf;
            const GLint rowBr1 = rowB[k] & 0xf;
            const GLint rowAg0 = (rowA[j] >> 4) & 0xf;
            const GLint rowAg1 = (rowA[k] >> 4) & 0xf;
            const GLint rowBg0 = (rowB[j] >> 4) & 0xf;
            const GLint rowBg1 = (rowB[k] >> 4) & 0xf;
            const GLint rowAb0 = (rowA[j] >> 8) & 0xf;
            const GLint rowAb1 = (rowA[k] >> 8) & 0xf;
            const GLint rowBb0 = (rowB[j] >> 8) & 0xf;
            const GLint rowBb1 = (rowB[k] >> 8) & 0xf;
            const GLint rowAa0 = (rowA[j] >> 12) & 0xf;
            const GLint rowAa1 = (rowA[k] >> 12) & 0xf;
            const GLint rowBa0 = (rowB[j] >> 12) & 0xf;
            const GLint rowBa1 = (rowB[k] >> 12) & 0xf;
            const GLint red   = (rowAr0 + rowAr1 + rowBr0 + rowBr1) >> 2;
            const GLint green = (rowAg0 + rowAg1 + rowBg0 + rowBg1) >> 2;
            const GLint blue  = (rowAb0 + rowAb1 + rowBb0 + rowBb1) >> 2;
            const GLint alpha = (rowAa0 + rowAa1 + rowBa0 + rowBa1) >> 2;
            dst[i] = (alpha << 12) | (blue << 8) | (green << 4) | red;
         }
      }
      return;
   case MESA_FORMAT_ARGB1555:
      {
         GLuint i, j, k;
         const GLushort *rowA = (const GLushort *) srcRowA;
         const GLushort *rowB = (const GLushort *) srcRowB;
         GLushort *dst = (GLushort *) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            const GLint rowAr0 = rowA[j] & 0x1f;
            const GLint rowAr1 = rowA[k] & 0x1f;
            const GLint rowBr0 = rowB[j] & 0x1f;
            const GLint rowBr1 = rowB[k] & 0xf;
            const GLint rowAg0 = (rowA[j] >> 5) & 0x1f;
            const GLint rowAg1 = (rowA[k] >> 5) & 0x1f;
            const GLint rowBg0 = (rowB[j] >> 5) & 0x1f;
            const GLint rowBg1 = (rowB[k] >> 5) & 0x1f;
            const GLint rowAb0 = (rowA[j] >> 10) & 0x1f;
            const GLint rowAb1 = (rowA[k] >> 10) & 0x1f;
            const GLint rowBb0 = (rowB[j] >> 10) & 0x1f;
            const GLint rowBb1 = (rowB[k] >> 10) & 0x1f;
            const GLint rowAa0 = (rowA[j] >> 15) & 0x1;
            const GLint rowAa1 = (rowA[k] >> 15) & 0x1;
            const GLint rowBa0 = (rowB[j] >> 15) & 0x1;
            const GLint rowBa1 = (rowB[k] >> 15) & 0x1;
            const GLint red   = (rowAr0 + rowAr1 + rowBr0 + rowBr1) >> 2;
            const GLint green = (rowAg0 + rowAg1 + rowBg0 + rowBg1) >> 2;
            const GLint blue  = (rowAb0 + rowAb1 + rowBb0 + rowBb1) >> 2;
            const GLint alpha = (rowAa0 + rowAa1 + rowBa0 + rowBa1) >> 2;
            dst[i] = (alpha << 15) | (blue << 10) | (green << 5) | red;
         }
      }
      return;
   case MESA_FORMAT_AL88:
      {
         GLuint i, j, k;
         const GLubyte (*rowA)[2] = (const GLubyte (*)[2]) srcRowA;
         const GLubyte (*rowB)[2] = (const GLubyte (*)[2]) srcRowB;
         GLubyte (*dst)[2] = (GLubyte (*)[2]) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            dst[i][0] = (rowA[j][0] + rowA[k][0] +
                         rowB[j][0] + rowB[k][0]) >> 2;
            dst[i][1] = (rowA[j][1] + rowA[k][1] +
                         rowB[j][1] + rowB[k][1]) >> 2;
         }
      }
      return;
   case MESA_FORMAT_RGB332:
      {
         GLuint i, j, k;
         const GLubyte *rowA = (const GLubyte *) srcRowA;
         const GLubyte *rowB = (const GLubyte *) srcRowB;
         GLubyte *dst = (GLubyte *) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            const GLint rowAr0 = rowA[j] & 0x3;
            const GLint rowAr1 = rowA[k] & 0x3;
            const GLint rowBr0 = rowB[j] & 0x3;
            const GLint rowBr1 = rowB[k] & 0x3;
            const GLint rowAg0 = (rowA[j] >> 2) & 0x7;
            const GLint rowAg1 = (rowA[k] >> 2) & 0x7;
            const GLint rowBg0 = (rowB[j] >> 2) & 0x7;
            const GLint rowBg1 = (rowB[k] >> 2) & 0x7;
            const GLint rowAb0 = (rowA[j] >> 5) & 0x7;
            const GLint rowAb1 = (rowA[k] >> 5) & 0x7;
            const GLint rowBb0 = (rowB[j] >> 5) & 0x7;
            const GLint rowBb1 = (rowB[k] >> 5) & 0x7;
            const GLint red   = (rowAr0 + rowAr1 + rowBr0 + rowBr1) >> 2;
            const GLint green = (rowAg0 + rowAg1 + rowBg0 + rowBg1) >> 2;
            const GLint blue  = (rowAb0 + rowAb1 + rowBb0 + rowBb1) >> 2;
            dst[i] = (blue << 5) | (green << 2) | red;
         }
      }
      return;
   case MESA_FORMAT_A8:
   case MESA_FORMAT_L8:
   case MESA_FORMAT_I8:
   case MESA_FORMAT_CI8:
      {
         GLuint i, j, k;
         const GLubyte *rowA = (const GLubyte *) srcRowA;
         const GLubyte *rowB = (const GLubyte *) srcRowB;
         GLubyte *dst = (GLubyte *) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            dst[i] = (rowA[j] + rowA[k] + rowB[j] + rowB[k]) >> 2;
         }
      }
      return;
   case MESA_FORMAT_RGBA_FLOAT32:
      {
         GLuint i, j, k;
         const GLfloat (*rowA)[4] = (const GLfloat (*)[4]) srcRowA;
         const GLfloat (*rowB)[4] = (const GLfloat (*)[4]) srcRowB;
         GLfloat (*dst)[4] = (GLfloat (*)[4]) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            dst[i][0] = (rowA[j][0] + rowA[k][0] +
                         rowB[j][0] + rowB[k][0]) * 0.25F;
            dst[i][1] = (rowA[j][1] + rowA[k][1] +
                         rowB[j][1] + rowB[k][1]) * 0.25F;
            dst[i][2] = (rowA[j][2] + rowA[k][2] +
                         rowB[j][2] + rowB[k][2]) * 0.25F;
            dst[i][3] = (rowA[j][3] + rowA[k][3] +
                         rowB[j][3] + rowB[k][3]) * 0.25F;
         }
      }
      return;
   case MESA_FORMAT_RGBA_FLOAT16:
      {
         GLuint i, j, k, comp;
         const GLhalfARB (*rowA)[4] = (const GLhalfARB (*)[4]) srcRowA;
         const GLhalfARB (*rowB)[4] = (const GLhalfARB (*)[4]) srcRowB;
         GLhalfARB (*dst)[4] = (GLhalfARB (*)[4]) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            for (comp = 0; comp < 4; comp++) {
               GLfloat aj, ak, bj, bk;
               aj = _mesa_half_to_float(rowA[j][comp]);
               ak = _mesa_half_to_float(rowA[k][comp]);
               bj = _mesa_half_to_float(rowB[j][comp]);
               bk = _mesa_half_to_float(rowB[k][comp]);
               dst[i][comp] = _mesa_float_to_half((aj + ak + bj + bk) * 0.25F);
            }
         }
      }
      return;
   case MESA_FORMAT_RGB_FLOAT32:
      {
         GLuint i, j, k;
         const GLfloat (*rowA)[3] = (const GLfloat (*)[3]) srcRowA;
         const GLfloat (*rowB)[3] = (const GLfloat (*)[3]) srcRowB;
         GLfloat (*dst)[3] = (GLfloat (*)[3]) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            dst[i][0] = (rowA[j][0] + rowA[k][0] +
                         rowB[j][0] + rowB[k][0]) * 0.25F;
            dst[i][1] = (rowA[j][1] + rowA[k][1] +
                         rowB[j][1] + rowB[k][1]) * 0.25F;
            dst[i][2] = (rowA[j][2] + rowA[k][2] +
                         rowB[j][2] + rowB[k][2]) * 0.25F;
         }
      }
      return;
   case MESA_FORMAT_RGB_FLOAT16:
      {
         GLuint i, j, k, comp;
         const GLhalfARB (*rowA)[3] = (const GLhalfARB (*)[3]) srcRowA;
         const GLhalfARB (*rowB)[3] = (const GLhalfARB (*)[3]) srcRowB;
         GLhalfARB (*dst)[3] = (GLhalfARB (*)[3]) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            for (comp = 0; comp < 3; comp++) {
               GLfloat aj, ak, bj, bk;
               aj = _mesa_half_to_float(rowA[j][comp]);
               ak = _mesa_half_to_float(rowA[k][comp]);
               bj = _mesa_half_to_float(rowB[j][comp]);
               bk = _mesa_half_to_float(rowB[k][comp]);
               dst[i][comp] = _mesa_float_to_half((aj + ak + bj + bk) * 0.25F);
            }
         }
      }
      return;
   case MESA_FORMAT_LUMINANCE_ALPHA_FLOAT32:
      {
         GLuint i, j, k;
         const GLfloat (*rowA)[2] = (const GLfloat (*)[2]) srcRowA;
         const GLfloat (*rowB)[2] = (const GLfloat (*)[2]) srcRowB;
         GLfloat (*dst)[2] = (GLfloat (*)[2]) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            dst[i][0] = (rowA[j][0] + rowA[k][0] +
                         rowB[j][0] + rowB[k][0]) * 0.25F;
            dst[i][1] = (rowA[j][1] + rowA[k][1] +
                         rowB[j][1] + rowB[k][1]) * 0.25F;
         }
      }
      return;
   case MESA_FORMAT_LUMINANCE_ALPHA_FLOAT16:
      {
         GLuint i, j, k, comp;
         const GLhalfARB (*rowA)[2] = (const GLhalfARB (*)[2]) srcRowA;
         const GLhalfARB (*rowB)[2] = (const GLhalfARB (*)[2]) srcRowB;
         GLhalfARB (*dst)[2] = (GLhalfARB (*)[2]) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            for (comp = 0; comp < 2; comp++) {
               GLfloat aj, ak, bj, bk;
               aj = _mesa_half_to_float(rowA[j][comp]);
               ak = _mesa_half_to_float(rowA[k][comp]);
               bj = _mesa_half_to_float(rowB[j][comp]);
               bk = _mesa_half_to_float(rowB[k][comp]);
               dst[i][comp] = _mesa_float_to_half((aj + ak + bj + bk) * 0.25F);
            }
         }
      }
      return;
   case MESA_FORMAT_ALPHA_FLOAT32:
   case MESA_FORMAT_LUMINANCE_FLOAT32:
   case MESA_FORMAT_INTENSITY_FLOAT32:
      {
         GLuint i, j, k;
         const GLfloat *rowA = (const GLfloat *) srcRowA;
         const GLfloat *rowB = (const GLfloat *) srcRowB;
         GLfloat *dst = (GLfloat *) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            dst[i] = (rowA[j] + rowA[k] + rowB[j] + rowB[k]) * 0.25F;
         }
      }
      return;
   case MESA_FORMAT_ALPHA_FLOAT16:
   case MESA_FORMAT_LUMINANCE_FLOAT16:
   case MESA_FORMAT_INTENSITY_FLOAT16:
      {
         GLuint i, j, k;
         const GLhalfARB *rowA = (const GLhalfARB *) srcRowA;
         const GLhalfARB *rowB = (const GLhalfARB *) srcRowB;
         GLhalfARB *dst = (GLhalfARB *) dstRow;
         for (i = j = 0, k = k0; i < (GLuint) dstWidth;
              i++, j += colStride, k += colStride) {
            GLfloat aj, ak, bj, bk;
            aj = _mesa_half_to_float(rowA[j]);
            ak = _mesa_half_to_float(rowA[k]);
            bj = _mesa_half_to_float(rowB[j]);
            bk = _mesa_half_to_float(rowB[k]);
            dst[i] = _mesa_float_to_half((aj + ak + bj + bk) * 0.25F);
         }
      }
      return;

   default:
      _mesa_problem(NULL, "bad format in do_row()");
   }
}


/*
 * These functions generate a 1/2-size mipmap image from a source image.
 * Texture borders are handled by copying or averaging the source image's
 * border texels, depending on the scale-down factor.
 */

static void
make_1d_mipmap(const struct gl_texture_format *format, GLint border,
               GLint srcWidth, const GLubyte *srcPtr,
               GLint dstWidth, GLubyte *dstPtr)
{
   const GLint bpt = format->TexelBytes;
   const GLubyte *src;
   GLubyte *dst;

   /* skip the border pixel, if any */
   src = srcPtr + border * bpt;
   dst = dstPtr + border * bpt;

   /* we just duplicate the input row, kind of hack, saves code */
   do_row(format, srcWidth - 2 * border, src, src,
          dstWidth - 2 * border, dst);

   if (border) {
      /* copy left-most pixel from source */
      MEMCPY(dstPtr, srcPtr, bpt);
      /* copy right-most pixel from source */
      MEMCPY(dstPtr + (dstWidth - 1) * bpt,
             srcPtr + (srcWidth - 1) * bpt,
             bpt);
   }
}


static void
make_2d_mipmap(const struct gl_texture_format *format, GLint border,
               GLint srcWidth, GLint srcHeight, const GLubyte *srcPtr,
               GLint dstWidth, GLint dstHeight, GLubyte *dstPtr)
{
   const GLint bpt = format->TexelBytes;
   const GLint srcWidthNB = srcWidth - 2 * border;  /* sizes w/out border */
   const GLint dstWidthNB = dstWidth - 2 * border;
   const GLint dstHeightNB = dstHeight - 2 * border;
   const GLint srcRowStride = bpt * srcWidth;
   const GLint dstRowStride = bpt * dstWidth;
   const GLubyte *srcA, *srcB;
   GLubyte *dst;
   GLint row, colStride;

   colStride = (srcWidth == dstWidth) ? 1 : 2;

   /* Compute src and dst pointers, skipping any border */
   srcA = srcPtr + border * ((srcWidth + 1) * bpt);
   if (srcHeight > 1)
      srcB = srcA + srcRowStride;
   else
      srcB = srcA;
   dst = dstPtr + border * ((dstWidth + 1) * bpt);

   for (row = 0; row < dstHeightNB; row++) {
      do_row(format, srcWidthNB, srcA, srcB,
             dstWidthNB, dst);
      srcA += 2 * srcRowStride;
      srcB += 2 * srcRowStride;
      dst += dstRowStride;
   }

   /* This is ugly but probably won't be used much */
   if (border > 0) {
      /* fill in dest border */
      /* lower-left border pixel */
      MEMCPY(dstPtr, srcPtr, bpt);
      /* lower-right border pixel */
      MEMCPY(dstPtr + (dstWidth - 1) * bpt,
             srcPtr + (srcWidth - 1) * bpt, bpt);
      /* upper-left border pixel */
      MEMCPY(dstPtr + dstWidth * (dstHeight - 1) * bpt,
             srcPtr + srcWidth * (srcHeight - 1) * bpt, bpt);
      /* upper-right border pixel */
      MEMCPY(dstPtr + (dstWidth * dstHeight - 1) * bpt,
             srcPtr + (srcWidth * srcHeight - 1) * bpt, bpt);
      /* lower border */
      do_row(format, srcWidthNB,
             srcPtr + bpt,
             srcPtr + bpt,
             dstWidthNB, dstPtr + bpt);
      /* upper border */
      do_row(format, srcWidthNB,
             srcPtr + (srcWidth * (srcHeight - 1) + 1) * bpt,
             srcPtr + (srcWidth * (srcHeight - 1) + 1) * bpt,
             dstWidthNB,
             dstPtr + (dstWidth * (dstHeight - 1) + 1) * bpt);
      /* left and right borders */
      if (srcHeight == dstHeight) {
         /* copy border pixel from src to dst */
         for (row = 1; row < srcHeight; row++) {
            MEMCPY(dstPtr + dstWidth * row * bpt,
                   srcPtr + srcWidth * row * bpt, bpt);
            MEMCPY(dstPtr + (dstWidth * row + dstWidth - 1) * bpt,
                   srcPtr + (srcWidth * row + srcWidth - 1) * bpt, bpt);
         }
      }
      else {
         /* average two src pixels each dest pixel */
         for (row = 0; row < dstHeightNB; row += 2) {
            do_row(format, 1,
                   srcPtr + (srcWidth * (row * 2 + 1)) * bpt,
                   srcPtr + (srcWidth * (row * 2 + 2)) * bpt,
                   1, dstPtr + (dstWidth * row + 1) * bpt);
            do_row(format, 1,
                   srcPtr + (srcWidth * (row * 2 + 1) + srcWidth - 1) * bpt,
                   srcPtr + (srcWidth * (row * 2 + 2) + srcWidth - 1) * bpt,
                   1, dstPtr + (dstWidth * row + 1 + dstWidth - 1) * bpt);
         }
      }
   }
}


static void
make_3d_mipmap(const struct gl_texture_format *format, GLint border,
               GLint srcWidth, GLint srcHeight, GLint srcDepth,
               const GLubyte *srcPtr,
               GLint dstWidth, GLint dstHeight, GLint dstDepth,
               GLubyte *dstPtr)
{
   const GLint bpt = format->TexelBytes;
   const GLint srcWidthNB = srcWidth - 2 * border;  /* sizes w/out border */
   const GLint srcDepthNB = srcDepth - 2 * border;
   const GLint dstWidthNB = dstWidth - 2 * border;
   const GLint dstHeightNB = dstHeight - 2 * border;
   const GLint dstDepthNB = dstDepth - 2 * border;
   GLvoid *tmpRowA, *tmpRowB;
   GLint img, row;
   GLint bytesPerSrcImage, bytesPerDstImage;
   GLint bytesPerSrcRow, bytesPerDstRow;
   GLint srcImageOffset, srcRowOffset;

   (void) srcDepthNB; /* silence warnings */

   /* Need two temporary row buffers */
   tmpRowA = MALLOC(srcWidth * bpt);
   if (!tmpRowA)
      return;
   tmpRowB = MALLOC(srcWidth * bpt);
   if (!tmpRowB) {
      FREE(tmpRowA);
      return;
   }

   bytesPerSrcImage = srcWidth * srcHeight * bpt;
   bytesPerDstImage = dstWidth * dstHeight * bpt;

   bytesPerSrcRow = srcWidth * bpt;
   bytesPerDstRow = dstWidth * bpt;

   /* Offset between adjacent src images to be averaged together */
   srcImageOffset = (srcDepth == dstDepth) ? 0 : bytesPerSrcImage;

   /* Offset between adjacent src rows to be averaged together */
   srcRowOffset = (srcHeight == dstHeight) ? 0 : srcWidth * bpt;

   /*
    * Need to average together up to 8 src pixels for each dest pixel.
    * Break that down into 3 operations:
    *   1. take two rows from source image and average them together.
    *   2. take two rows from next source image and average them together.
    *   3. take the two averaged rows and average them for the final dst row.
    */

   /*
   _mesa_printf("mip3d %d x %d x %d  ->  %d x %d x %d\n",
          srcWidth, srcHeight, srcDepth, dstWidth, dstHeight, dstDepth);
   */

   for (img = 0; img < dstDepthNB; img++) {
      /* first source image pointer, skipping border */
      const GLubyte *imgSrcA = srcPtr
         + (bytesPerSrcImage + bytesPerSrcRow + border) * bpt * border
         + img * (bytesPerSrcImage + srcImageOffset);
      /* second source image pointer, skipping border */
      const GLubyte *imgSrcB = imgSrcA + srcImageOffset;
      /* address of the dest image, skipping border */
      GLubyte *imgDst = dstPtr
         + (bytesPerDstImage + bytesPerDstRow + border) * bpt * border
         + img * bytesPerDstImage;

      /* setup the four source row pointers and the dest row pointer */
      const GLubyte *srcImgARowA = imgSrcA;
      const GLubyte *srcImgARowB = imgSrcA + srcRowOffset;
      const GLubyte *srcImgBRowA = imgSrcB;
      const GLubyte *srcImgBRowB = imgSrcB + srcRowOffset;
      GLubyte *dstImgRow = imgDst;

      for (row = 0; row < dstHeightNB; row++) {
         /* Average together two rows from first src image */
         do_row(format, srcWidthNB, srcImgARowA, srcImgARowB,
                srcWidthNB, tmpRowA);
         /* Average together two rows from second src image */
         do_row(format, srcWidthNB, srcImgBRowA, srcImgBRowB,
                srcWidthNB, tmpRowB);
         /* Average together the temp rows to make the final row */
         do_row(format, srcWidthNB, tmpRowA, tmpRowB,
                dstWidthNB, dstImgRow);
         /* advance to next rows */
         srcImgARowA += bytesPerSrcRow + srcRowOffset;
         srcImgARowB += bytesPerSrcRow + srcRowOffset;
         srcImgBRowA += bytesPerSrcRow + srcRowOffset;
         srcImgBRowB += bytesPerSrcRow + srcRowOffset;
         dstImgRow += bytesPerDstRow;
      }
   }

   FREE(tmpRowA);
   FREE(tmpRowB);

   /* Luckily we can leverage the make_2d_mipmap() function here! */
   if (border > 0) {
      /* do front border image */
      make_2d_mipmap(format, 1, srcWidth, srcHeight, srcPtr,
                     dstWidth, dstHeight, dstPtr);
      /* do back border image */
      make_2d_mipmap(format, 1, srcWidth, srcHeight,
                     srcPtr + bytesPerSrcImage * (srcDepth - 1),
                     dstWidth, dstHeight,
                     dstPtr + bytesPerDstImage * (dstDepth - 1));
      /* do four remaining border edges that span the image slices */
      if (srcDepth == dstDepth) {
         /* just copy border pixels from src to dst */
         for (img = 0; img < dstDepthNB; img++) {
            const GLubyte *src;
            GLubyte *dst;

            /* do border along [img][row=0][col=0] */
            src = srcPtr + (img + 1) * bytesPerSrcImage;
            dst = dstPtr + (img + 1) * bytesPerDstImage;
            MEMCPY(dst, src, bpt);

            /* do border along [img][row=dstHeight-1][col=0] */
            src = srcPtr + (img * 2 + 1) * bytesPerSrcImage
                         + (srcHeight - 1) * bytesPerSrcRow;
            dst = dstPtr + (img + 1) * bytesPerDstImage
                         + (dstHeight - 1) * bytesPerDstRow;
            MEMCPY(dst, src, bpt);

            /* do border along [img][row=0][col=dstWidth-1] */
            src = srcPtr + (img * 2 + 1) * bytesPerSrcImage
                         + (srcWidth - 1) * bpt;
            dst = dstPtr + (img + 1) * bytesPerDstImage
                         + (dstWidth - 1) * bpt;
            MEMCPY(dst, src, bpt);

            /* do border along [img][row=dstHeight-1][col=dstWidth-1] */
            src = srcPtr + (img * 2 + 1) * bytesPerSrcImage
                         + (bytesPerSrcImage - bpt);
            dst = dstPtr + (img + 1) * bytesPerDstImage
                         + (bytesPerDstImage - bpt);
            MEMCPY(dst, src, bpt);
         }
      }
      else {
         /* average border pixels from adjacent src image pairs */
         ASSERT(srcDepthNB == 2 * dstDepthNB);
         for (img = 0; img < dstDepthNB; img++) {
            const GLubyte *src;
            GLubyte *dst;

            /* do border along [img][row=0][col=0] */
            src = srcPtr + (img * 2 + 1) * bytesPerSrcImage;
            dst = dstPtr + (img + 1) * bytesPerDstImage;
            do_row(format, 1, src, src + srcImageOffset, 1, dst);

            /* do border along [img][row=dstHeight-1][col=0] */
            src = srcPtr + (img * 2 + 1) * bytesPerSrcImage
                         + (srcHeight - 1) * bytesPerSrcRow;
            dst = dstPtr + (img + 1) * bytesPerDstImage
                         + (dstHeight - 1) * bytesPerDstRow;
            do_row(format, 1, src, src + srcImageOffset, 1, dst);

            /* do border along [img][row=0][col=dstWidth-1] */
            src = srcPtr + (img * 2 + 1) * bytesPerSrcImage
                         + (srcWidth - 1) * bpt;
            dst = dstPtr + (img + 1) * bytesPerDstImage
                         + (dstWidth - 1) * bpt;
            do_row(format, 1, src, src + srcImageOffset, 1, dst);

            /* do border along [img][row=dstHeight-1][col=dstWidth-1] */
            src = srcPtr + (img * 2 + 1) * bytesPerSrcImage
                         + (bytesPerSrcImage - bpt);
            dst = dstPtr + (img + 1) * bytesPerDstImage
                         + (bytesPerDstImage - bpt);
            do_row(format, 1, src, src + srcImageOffset, 1, dst);
         }
      }
   }
}


/*
 * For GL_SGIX_generate_mipmap:
 * Generate a complete set of mipmaps from texObj's base-level image.
 * Stop at texObj's MaxLevel or when we get to the 1x1 texture.
 */
void
_mesa_generate_mipmap(GLcontext *ctx, GLenum target,
                      const struct gl_texture_unit *texUnit,
                      struct gl_texture_object *texObj)
{
   const struct gl_texture_image *srcImage;
   const struct gl_texture_format *convertFormat;
   const GLubyte *srcData = NULL;
   GLubyte *dstData = NULL;
   GLint level, maxLevels;

   ASSERT(texObj);
   srcImage = texObj->Image[0][texObj->BaseLevel];
   ASSERT(srcImage);

   maxLevels = _mesa_max_texture_levels(ctx, texObj->Target);
   ASSERT(maxLevels > 0);  /* bad target */

   /* Find convertFormat - the format that do_row() will process */
   if (srcImage->IsCompressed) {
      /* setup for compressed textures */
      GLuint row;
      GLint  components, size;
      GLchan *dst;

      assert(texObj->Target == GL_TEXTURE_2D);

      if (srcImage->Format == GL_RGB) {
         convertFormat = &_mesa_texformat_rgb;
         components = 3;
      }
      else if (srcImage->Format == GL_RGBA) {
         convertFormat = &_mesa_texformat_rgba;
         components = 4;
      }
      else {
         _mesa_problem(ctx, "bad srcImage->Format in _mesa_generate_mipmaps");
         return;
      }

      /* allocate storage for uncompressed GL_RGB or GL_RGBA images */
      size = _mesa_bytes_per_pixel(srcImage->Format, CHAN_TYPE)
         * srcImage->Width * srcImage->Height * srcImage->Depth + 20;
      /* 20 extra bytes, just be safe when calling last FetchTexel */
      srcData = (GLubyte *) MALLOC(size);
      if (!srcData) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "generate mipmaps");
         return;
      }
      dstData = (GLubyte *) MALLOC(size / 2);  /* 1/4 would probably be OK */
      if (!dstData) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "generate mipmaps");
         FREE((void *) srcData);
         return;
      }

      /* decompress base image here */
      dst = (GLchan *) srcData;
      for (row = 0; row < srcImage->Height; row++) {
         GLuint col;
         for (col = 0; col < srcImage->Width; col++) {
            srcImage->FetchTexelc(srcImage, col, row, 0, dst);
            dst += components;
         }
      }
   }
   else {
      /* uncompressed */
      convertFormat = srcImage->TexFormat;
   }

   for (level = texObj->BaseLevel; level < texObj->MaxLevel
           && level < maxLevels - 1; level++) {
      /* generate image[level+1] from image[level] */
      const struct gl_texture_image *srcImage;
      struct gl_texture_image *dstImage;
      GLint srcWidth, srcHeight, srcDepth;
      GLint dstWidth, dstHeight, dstDepth;
      GLint border, bytesPerTexel;

      /* get src image parameters */
      srcImage = _mesa_select_tex_image(ctx, texUnit, target, level);
      ASSERT(srcImage);
      srcWidth = srcImage->Width;
      srcHeight = srcImage->Height;
      srcDepth = srcImage->Depth;
      border = srcImage->Border;

      /* compute next (level+1) image size */
      if (srcWidth - 2 * border > 1) {
         dstWidth = (srcWidth - 2 * border) / 2 + 2 * border;
      }
      else {
         dstWidth = srcWidth; /* can't go smaller */
      }
      if (srcHeight - 2 * border > 1) {
         dstHeight = (srcHeight - 2 * border) / 2 + 2 * border;
      }
      else {
         dstHeight = srcHeight; /* can't go smaller */
      }
      if (srcDepth - 2 * border > 1) {
         dstDepth = (srcDepth - 2 * border) / 2 + 2 * border;
      }
      else {
         dstDepth = srcDepth; /* can't go smaller */
      }

      if (dstWidth == srcWidth &&
          dstHeight == srcHeight &&
          dstDepth == srcDepth) {
         /* all done */
         if (srcImage->IsCompressed) {
            FREE((void *) srcData);
            FREE(dstData);
         }
         return;
      }

      /* get dest gl_texture_image */
      dstImage = _mesa_get_tex_image(ctx, texUnit, target, level + 1);
      if (!dstImage) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "generating mipmaps");
         return;
      }

      /* Free old image data */
      if (dstImage->Data)
         MESA_PBUFFER_FREE(dstImage->Data);

      /* initialize new image */
      _mesa_init_teximage_fields(ctx, target, dstImage, dstWidth, dstHeight,
                                 dstDepth, border, srcImage->IntFormat);
      dstImage->DriverData = NULL;
      dstImage->TexFormat = srcImage->TexFormat;
      dstImage->FetchTexelc = srcImage->FetchTexelc;
      dstImage->FetchTexelf = srcImage->FetchTexelf;
      ASSERT(dstImage->TexFormat);
      ASSERT(dstImage->FetchTexelc);
      ASSERT(dstImage->FetchTexelf);

      /* Alloc new teximage data buffer.
       * Setup src and dest data pointers.
       */
      if (dstImage->IsCompressed) {
         ASSERT(dstImage->CompressedSize > 0); /* set by init_teximage_fields*/
         dstImage->Data = MESA_PBUFFER_ALLOC(dstImage->CompressedSize);
         if (!dstImage->Data) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "generating mipmaps");
            return;
         }
         /* srcData and dstData are already set */
         ASSERT(srcData);
         ASSERT(dstData);
      }
      else {
         bytesPerTexel = srcImage->TexFormat->TexelBytes;
         ASSERT(dstWidth * dstHeight * dstDepth * bytesPerTexel > 0);
         dstImage->Data = MESA_PBUFFER_ALLOC(dstWidth * dstHeight * dstDepth
                                             * bytesPerTexel);
         if (!dstImage->Data) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "generating mipmaps");
            return;
         }
         srcData = (const GLubyte *) srcImage->Data;
         dstData = (GLubyte *) dstImage->Data;
      }

      /*
       * We use simple 2x2 averaging to compute the next mipmap level.
       */
      switch (target) {
         case GL_TEXTURE_1D:
            make_1d_mipmap(convertFormat, border,
                           srcWidth, srcData,
                           dstWidth, dstData);
            break;
         case GL_TEXTURE_2D:
         case GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB:
         case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB:
         case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB:
         case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB:
         case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB:
         case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB:
            make_2d_mipmap(convertFormat, border,
                           srcWidth, srcHeight, srcData,
                           dstWidth, dstHeight, dstData);
            break;
         case GL_TEXTURE_3D:
            make_3d_mipmap(convertFormat, border,
                           srcWidth, srcHeight, srcDepth, srcData,
                           dstWidth, dstHeight, dstDepth, dstData);
            break;
         case GL_TEXTURE_RECTANGLE_NV:
            /* no mipmaps, do nothing */
            break;
         default:
            _mesa_problem(ctx, "bad dimensions in _mesa_generate_mipmaps");
            return;
      }

      if (dstImage->IsCompressed) {
         GLubyte *temp;
         /* compress image from dstData into dstImage->Data */
         const GLenum srcFormat = convertFormat->BaseFormat;
         GLint dstRowStride = _mesa_compressed_row_stride(srcImage->IntFormat,
                                                          dstWidth);
         ASSERT(srcFormat == GL_RGB || srcFormat == GL_RGBA);
         _mesa_compress_teximage(ctx,
                                 dstWidth, dstHeight, /* size */
                                 srcFormat,           /* source format */
                (const GLchan *) dstData,             /* source buffer */
                                 dstWidth,            /* source row stride */
                                 dstImage->TexFormat, /* dest format */
                      (GLubyte*) dstImage->Data,      /* dest buffer */
                                 dstRowStride );      /* dest row stride */

         /* swap src and dest pointers */
         temp = (GLubyte *) srcData;
         srcData = dstData;
         dstData = temp;
      }

   } /* loop over mipmap levels */
}

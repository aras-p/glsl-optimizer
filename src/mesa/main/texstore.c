/* $Id: texstore.c,v 1.2 2001/02/07 03:27:41 brianp Exp $ */

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



#include "context.h"
#include "convolve.h"
#include "image.h"
#include "macros.h"
#include "mem.h"
#include "teximage.h"
#include "texstore.h"


/*
 * Get texture palette entry.
 */
static void
palette_sample(GLcontext *ctx,
               const struct gl_texture_object *tObj,
               GLint index, GLchan rgba[4] )
{
   const GLchan *palette;
   GLenum format;

   if (ctx->Texture.SharedPalette) {
      ASSERT(!ctx->Texture.Palette.FloatTable);
      palette = (const GLchan *) ctx->Texture.Palette.Table;
      format = ctx->Texture.Palette.Format;
   }
   else {
      ASSERT(!tObj->Palette.FloatTable);
      palette = (const GLchan *) tObj->Palette.Table;
      format = tObj->Palette.Format;
   }

   switch (format) {
      case GL_ALPHA:
         rgba[ACOMP] = palette[index];
         return;
      case GL_LUMINANCE:
      case GL_INTENSITY:
         rgba[RCOMP] = palette[index];
         return;
      case GL_LUMINANCE_ALPHA:
         rgba[RCOMP] = palette[(index << 1) + 0];
         rgba[ACOMP] = palette[(index << 1) + 1];
         return;
      case GL_RGB:
         rgba[RCOMP] = palette[index * 3 + 0];
         rgba[GCOMP] = palette[index * 3 + 1];
         rgba[BCOMP] = palette[index * 3 + 2];
         return;
      case GL_RGBA:
         rgba[RCOMP] = palette[(index << 2) + 0];
         rgba[GCOMP] = palette[(index << 2) + 1];
         rgba[BCOMP] = palette[(index << 2) + 2];
         rgba[ACOMP] = palette[(index << 2) + 3];
         return;
      default:
         gl_problem(NULL, "Bad palette format in palette_sample");
   }
}



/*
 * Default 1-D texture texel fetch function.  This will typically be
 * overridden by hardware drivers which store their texture images in
 * special ways.
 */
static void
fetch_1d_texel(GLcontext *ctx,
               const struct gl_texture_object *tObj,
               const struct gl_texture_image *img,
               GLint i, GLint j, GLint k, GLchan rgba[4])
{
   const GLchan *data = (GLchan *) img->Data;
   const GLchan *texel;
#ifdef DEBUG
   GLint width = img->Width;
   assert(i >= 0);
   assert(i < width);
#endif

   switch (img->Format) {
      case GL_COLOR_INDEX:
         {
            GLint index = data[i];
            palette_sample(ctx, tObj, index, rgba);
            return;
         }
      case GL_ALPHA:
         rgba[ACOMP] = data[i];
         return;
      case GL_LUMINANCE:
      case GL_INTENSITY:
         rgba[RCOMP] = data[i];
         return;
      case GL_LUMINANCE_ALPHA:
         texel = data + i * 2;
         rgba[RCOMP] = texel[0];
         rgba[ACOMP] = texel[1];
         return;
      case GL_RGB:
         texel = data + i * 3;
         rgba[RCOMP] = texel[0];
         rgba[GCOMP] = texel[1];
         rgba[BCOMP] = texel[2];
         return;
      case GL_RGBA:
         texel = data + i * 4;
         rgba[RCOMP] = texel[0];
         rgba[GCOMP] = texel[1];
         rgba[BCOMP] = texel[2];
         rgba[ACOMP] = texel[3];
         return;
      default:
         gl_problem(NULL, "Bad format in fetch_1d_texel");
         return;
   }
}


/*
 * Default 2-D texture texel fetch function.
 */
static void
fetch_2d_texel(GLcontext *ctx,
               const struct gl_texture_object *tObj,
               const struct gl_texture_image *img,
               GLint i, GLint j, GLint k, GLchan rgba[4])
{
   const GLint width = img->Width;    /* includes border */
   const GLchan *data = (GLchan *) img->Data;
   const GLchan *texel;

#ifdef DEBUG
   const GLint height = img->Height;  /* includes border */
   assert(i >= 0);
   assert(i < width);
   assert(j >= 0);
   assert(j < height);
#endif

   switch (img->Format) {
      case GL_COLOR_INDEX:
         {
            GLint index = data[width *j + i];
            palette_sample(ctx, tObj, index, rgba );
            return;
         }
      case GL_ALPHA:
         rgba[ACOMP] = data[width * j + i];
         return;
      case GL_LUMINANCE:
      case GL_INTENSITY:
         rgba[RCOMP] = data[ width * j + i];
         return;
      case GL_LUMINANCE_ALPHA:
         texel = data + (width * j + i) * 2;
         rgba[RCOMP] = texel[0];
         rgba[ACOMP] = texel[1];
         return;
      case GL_RGB:
         texel = data + (width * j + i) * 3;
         rgba[RCOMP] = texel[0];
         rgba[GCOMP] = texel[1];
         rgba[BCOMP] = texel[2];
         return;
      case GL_RGBA:
         texel = data + (width * j + i) * 4;
         rgba[RCOMP] = texel[0];
         rgba[GCOMP] = texel[1];
         rgba[BCOMP] = texel[2];
         rgba[ACOMP] = texel[3];
         return;
      default:
         gl_problem(NULL, "Bad format in fetch_2d_texel");
   }
}


/*
 * Default 2-D texture texel fetch function.
 */
static void
fetch_3d_texel(GLcontext *ctx,
               const struct gl_texture_object *tObj,
               const struct gl_texture_image *img,
               GLint i, GLint j, GLint k, GLchan rgba[4])
{
   const GLint width = img->Width;    /* includes border */
   const GLint height = img->Height;  /* includes border */
   const GLint rectarea = width * height;
   const GLchan *data = (GLchan *) img->Data;
   const GLchan *texel;

#ifdef DEBUG
   const GLint depth = img->Depth;    /* includes border */
   assert(i >= 0);
   assert(i < width);
   assert(j >= 0);
   assert(j < height);
   assert(k >= 0);
   assert(k < depth);
#endif

   switch (img->Format) {
      case GL_COLOR_INDEX:
         {
            GLint index = data[ rectarea * k +  width * j + i ];
            palette_sample(ctx, tObj, index, rgba );
            return;
         }
      case GL_ALPHA:
         rgba[ACOMP] = data[ rectarea * k +  width * j + i ];
         return;
      case GL_LUMINANCE:
      case GL_INTENSITY:
         rgba[RCOMP] = data[ rectarea * k +  width * j + i ];
         return;
      case GL_LUMINANCE_ALPHA:
         texel = data + ( rectarea * k + width * j + i) * 2;
         rgba[RCOMP] = texel[0];
         rgba[ACOMP] = texel[1];
         return;
      case GL_RGB:
         texel = data + (rectarea * k + width * j + i) * 3;
         rgba[RCOMP] = texel[0];
         rgba[GCOMP] = texel[1];
         rgba[BCOMP] = texel[2];
         return;
      case GL_RGBA:
         texel = data + (rectarea * k + width * j + i) * 4;
         rgba[RCOMP] = texel[0];
         rgba[GCOMP] = texel[1];
         rgba[BCOMP] = texel[2];
         rgba[ACOMP] = texel[3];
         return;
      default:
         gl_problem(NULL, "Bad format in fetch_3d_texel");
   }
}



/*
 * Examine the texImage->Format field and set the Red, Green, Blue, etc
 * texel component sizes to default values.
 * These fields are set only here by core Mesa but device drivers may
 * overwritting these fields to indicate true texel resolution.
 */
static void
set_teximage_component_sizes( struct gl_texture_image *texImage )
{
   switch (texImage->Format) {
      case GL_ALPHA:
         texImage->RedBits = 0;
         texImage->GreenBits = 0;
         texImage->BlueBits = 0;
         texImage->AlphaBits = 8 * sizeof(GLchan);
         texImage->IntensityBits = 0;
         texImage->LuminanceBits = 0;
         texImage->IndexBits = 0;
         break;
      case GL_LUMINANCE:
         texImage->RedBits = 0;
         texImage->GreenBits = 0;
         texImage->BlueBits = 0;
         texImage->AlphaBits = 0;
         texImage->IntensityBits = 0;
         texImage->LuminanceBits = 8 * sizeof(GLchan);
         texImage->IndexBits = 0;
         break;
      case GL_LUMINANCE_ALPHA:
         texImage->RedBits = 0;
         texImage->GreenBits = 0;
         texImage->BlueBits = 0;
         texImage->AlphaBits = 8 * sizeof(GLchan);
         texImage->IntensityBits = 0;
         texImage->LuminanceBits = 8 * sizeof(GLchan);
         texImage->IndexBits = 0;
         break;
      case GL_INTENSITY:
         texImage->RedBits = 0;
         texImage->GreenBits = 0;
         texImage->BlueBits = 0;
         texImage->AlphaBits = 0;
         texImage->IntensityBits = 8 * sizeof(GLchan);
         texImage->LuminanceBits = 0;
         texImage->IndexBits = 0;
         break;
      case GL_RED:
         texImage->RedBits = 8 * sizeof(GLchan);
         texImage->GreenBits = 0;
         texImage->BlueBits = 0;
         texImage->AlphaBits = 0;
         texImage->IntensityBits = 0;
         texImage->LuminanceBits = 0;
         texImage->IndexBits = 0;
         break;
      case GL_GREEN:
         texImage->RedBits = 0;
         texImage->GreenBits = 8 * sizeof(GLchan);
         texImage->BlueBits = 0;
         texImage->AlphaBits = 0;
         texImage->IntensityBits = 0;
         texImage->LuminanceBits = 0;
         texImage->IndexBits = 0;
         break;
      case GL_BLUE:
         texImage->RedBits = 0;
         texImage->GreenBits = 0;
         texImage->BlueBits = 8 * sizeof(GLchan);
         texImage->AlphaBits = 0;
         texImage->IntensityBits = 0;
         texImage->LuminanceBits = 0;
         texImage->IndexBits = 0;
         break;
      case GL_RGB:
      case GL_BGR:
         texImage->RedBits = 8 * sizeof(GLchan);
         texImage->GreenBits = 8 * sizeof(GLchan);
         texImage->BlueBits = 8 * sizeof(GLchan);
         texImage->AlphaBits = 0;
         texImage->IntensityBits = 0;
         texImage->LuminanceBits = 0;
         texImage->IndexBits = 0;
         break;
      case GL_RGBA:
      case GL_BGRA:
      case GL_ABGR_EXT:
         texImage->RedBits = 8 * sizeof(GLchan);
         texImage->GreenBits = 8 * sizeof(GLchan);
         texImage->BlueBits = 8 * sizeof(GLchan);
         texImage->AlphaBits = 8 * sizeof(GLchan);
         texImage->IntensityBits = 0;
         texImage->LuminanceBits = 0;
         texImage->IndexBits = 0;
         break;
      case GL_COLOR_INDEX:
         texImage->RedBits = 0;
         texImage->GreenBits = 0;
         texImage->BlueBits = 0;
         texImage->AlphaBits = 0;
         texImage->IntensityBits = 0;
         texImage->LuminanceBits = 0;
         texImage->IndexBits = 8 * sizeof(GLchan);
         break;
      default:
         gl_problem(NULL, "unexpected format in set_teximage_component_sizes");
   }
}


/*
 * Compute log base 2 of n.
 * If n isn't an exact power of two return -1.
 * If n<0 return -1.
 */
static int
logbase2( int n )
{
   GLint i = 1;
   GLint log2 = 0;

   if (n<0) {
      return -1;
   }

   while ( n > i ) {
      i *= 2;
      log2++;
   }
   if (i != n) {
      return -1;
   }
   else {
      return log2;
   }
}



/*
 * Return GL_TRUE if internalFormat is a compressed format, return GL_FALSE
 * otherwise.
 */
static GLboolean
is_compressed_format(GLcontext *ctx, GLenum internalFormat)
{
    if (ctx->Driver.IsCompressedFormat) {
        return (*ctx->Driver.IsCompressedFormat)(ctx, internalFormat);
    }
    return GL_FALSE;
}



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
   if (!ctx->_ImageTransferState && dimensions == 2
       && srcType == GL_UNSIGNED_BYTE) {

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
      const GLenum texType = GL_UNSIGNED_BYTE;
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
            gl_error(ctx, GL_OUT_OF_MEMORY, "glTexImage");
            return;
         }
         convImage = (GLfloat *) MALLOC(srcWidth * srcHeight * 4 * sizeof(GLfloat));
         if (!convImage) {
            gl_error(ctx, GL_OUT_OF_MEMORY, "glTexImage");
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
                                          texFormat, GL_UNSIGNED_BYTE,
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
   const GLint components = components_in_intformat(internalFormat);
   GLint postConvWidth = width;

   if (ctx->_ImageTransferState & IMAGE_CONVOLUTION_BIT) {
      _mesa_adjust_image_for_convolution(ctx, 1, &postConvWidth, NULL);
   }

   /* setup the teximage struct's fields */
   texImage->Format = (GLenum) _mesa_base_tex_format(ctx, internalFormat);
   texImage->Type = CHAN_TYPE; /* usually GL_UNSIGNED_BYTE */
   texImage->FetchTexel = fetch_1d_texel;
   set_teximage_component_sizes(texImage);

   /* allocate memory */
   texImage->Data = (GLchan *) MALLOC(postConvWidth
                                      * components * sizeof(GLchan));
   if (!texImage->Data)
      return;      /* out of memory */

   /* unpack image, apply transfer ops and store in texImage->Data */
   _mesa_transfer_teximage(ctx, 1, texImage->Format, texImage->Data,
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
   const GLint components = components_in_intformat(internalFormat);
   GLint postConvWidth = width, postConvHeight = height;

   if (ctx->_ImageTransferState & IMAGE_CONVOLUTION_BIT) {
      _mesa_adjust_image_for_convolution(ctx, 2, &postConvWidth,
                                         &postConvHeight);
   }

   /* setup the teximage struct's fields */
   texImage->Format = (GLenum) _mesa_base_tex_format(ctx, internalFormat);
   texImage->Type = CHAN_TYPE; /* usually GL_UNSIGNED_BYTE */
   texImage->FetchTexel = fetch_2d_texel;
   set_teximage_component_sizes(texImage);

   /* allocate memory */
   texImage->Data = (GLchan *) MALLOC(postConvWidth * postConvHeight
                                      * components * sizeof(GLchan));
   if (!texImage->Data)
      return;      /* out of memory */

   /* unpack image, apply transfer ops and store in texImage->Data */
   _mesa_transfer_teximage(ctx, 2, texImage->Format, texImage->Data,
                           width, height, 1, 0, 0, 0,
                           texImage->Width * components * sizeof(GLchan),
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
   const GLint components = components_in_intformat(internalFormat);

   /* setup the teximage struct's fields */
   texImage->Format = (GLenum) _mesa_base_tex_format(ctx, internalFormat);
   texImage->Type = CHAN_TYPE; /* usually GL_UNSIGNED_BYTE */
   texImage->FetchTexel = fetch_3d_texel;
   set_teximage_component_sizes(texImage);

   /* allocate memory */
   texImage->Data = (GLchan *) MALLOC(width * height * depth
                                      * components * sizeof(GLchan));
   if (!texImage->Data)
      return;      /* out of memory */

   /* unpack image, apply transfer ops and store in texImage->Data */
   _mesa_transfer_teximage(ctx, 3, texImage->Format, texImage->Data,
                           width, height, depth, 0, 0, 0,
                           texImage->Width * components * sizeof(GLchan),
                           texImage->Width * texImage->Height * components
                           * sizeof(GLchan),
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
   _mesa_transfer_teximage(ctx, 1, texImage->Format, texImage->Data,
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
   _mesa_transfer_teximage(ctx, 2, texImage->Format, texImage->Data,
                           width, height, 1, /* src size */
                           xoffset, yoffset, 0, /* dest offsets */
                           texImage->Width * components * sizeof(GLchan),
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
   _mesa_transfer_teximage(ctx, 3, texImage->Format, texImage->Data,
                           width, height, depth, /* src size */
                           xoffset, yoffset, xoffset, /* dest offsets */
                           texImage->Width * components * sizeof(GLchan),
                           texImage->Width * texImage->Height * components
                           * sizeof(GLchan),
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
   /* setup the teximage struct's fields */
   texImage->Format = (GLenum) _mesa_base_tex_format(ctx, internalFormat);
   texImage->Type = CHAN_TYPE; /* usually GL_UNSIGNED_BYTE */
   set_teximage_component_sizes(texImage);

   return GL_TRUE;
}


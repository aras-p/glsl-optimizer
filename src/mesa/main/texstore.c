/*
 * Mesa 3-D graphics library
 * Version:  7.5
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (c) 2008-2009  VMware, Inc.
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

/**
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
#include "mipmap.h"
#include "imports.h"
#include "texcompress.h"
#include "texcompress_fxt1.h"
#include "texcompress_s3tc.h"
#include "teximage.h"
#include "texstore.h"
#include "enums.h"


enum {
   ZERO = 4, 
   ONE = 5
};


/**
 * Texture image storage function.
 */
typedef GLboolean (*StoreTexImageFunc)(TEXSTORE_PARAMS);


/**
 * Return GL_TRUE if the given image format is one that be converted
 * to another format by swizzling.
 */
static GLboolean
can_swizzle(GLenum logicalBaseFormat)
{
   switch (logicalBaseFormat) {
   case GL_RGBA:
   case GL_RGB:
   case GL_LUMINANCE_ALPHA:
   case GL_INTENSITY:
   case GL_ALPHA:
   case GL_LUMINANCE:
   case GL_RED:
   case GL_GREEN:
   case GL_BLUE:
   case GL_BGR:
   case GL_BGRA:
   case GL_ABGR_EXT:
      return GL_TRUE;
   default:
      return GL_FALSE;
   }
}



enum {
   IDX_LUMINANCE = 0,
   IDX_ALPHA,
   IDX_INTENSITY,
   IDX_LUMINANCE_ALPHA,
   IDX_RGB,
   IDX_RGBA,
   IDX_RED,
   IDX_GREEN,
   IDX_BLUE,
   IDX_BGR,
   IDX_BGRA,
   IDX_ABGR,
   MAX_IDX
};

#define MAP1(x)       MAP4(x, ZERO, ZERO, ZERO)
#define MAP2(x,y)     MAP4(x, y, ZERO, ZERO)
#define MAP3(x,y,z)   MAP4(x, y, z, ZERO)
#define MAP4(x,y,z,w) { x, y, z, w, ZERO, ONE }


static const struct {
   GLubyte format_idx;
   GLubyte to_rgba[6];
   GLubyte from_rgba[6];
} mappings[MAX_IDX] = 
{
   {
      IDX_LUMINANCE,
      MAP4(0,0,0,ONE),
      MAP1(0)
   },

   {
      IDX_ALPHA,
      MAP4(ZERO, ZERO, ZERO, 0),
      MAP1(3)
   },

   {
      IDX_INTENSITY,
      MAP4(0, 0, 0, 0),
      MAP1(0),
   },

   {
      IDX_LUMINANCE_ALPHA,
      MAP4(0,0,0,1),
      MAP2(0,3)
   },

   {
      IDX_RGB,
      MAP4(0,1,2,ONE),
      MAP3(0,1,2)
   },

   {
      IDX_RGBA,
      MAP4(0,1,2,3),
      MAP4(0,1,2,3),
   },


   {
      IDX_RED,
      MAP4(0, ZERO, ZERO, ONE),
      MAP1(0),
   },

   {
      IDX_GREEN,
      MAP4(ZERO, 0, ZERO, ONE),
      MAP1(1),
   },

   {
      IDX_BLUE,
      MAP4(ZERO, ZERO, 0, ONE),
      MAP1(2),
   },

   {
      IDX_BGR,
      MAP4(2,1,0,ONE),
      MAP3(2,1,0)
   },

   {
      IDX_BGRA,
      MAP4(2,1,0,3),
      MAP4(2,1,0,3)
   },

   {
      IDX_ABGR,
      MAP4(3,2,1,0),
      MAP4(3,2,1,0)
   },
};



/**
 * Convert a GL image format enum to an IDX_* value (see above).
 */
static int
get_map_idx(GLenum value)
{
   switch (value) {
   case GL_LUMINANCE: return IDX_LUMINANCE;
   case GL_ALPHA: return IDX_ALPHA;
   case GL_INTENSITY: return IDX_INTENSITY;
   case GL_LUMINANCE_ALPHA: return IDX_LUMINANCE_ALPHA;
   case GL_RGB: return IDX_RGB;
   case GL_RGBA: return IDX_RGBA;
   case GL_RED: return IDX_RED;
   case GL_GREEN: return IDX_GREEN;
   case GL_BLUE: return IDX_BLUE;
   case GL_BGR: return IDX_BGR;
   case GL_BGRA: return IDX_BGRA;
   case GL_ABGR_EXT: return IDX_ABGR;
   default:
      _mesa_problem(NULL, "Unexpected inFormat");
      return 0;
   }
}   


/**
 * When promoting texture formats (see below) we need to compute the
 * mapping of dest components back to source components.
 * This function does that.
 * \param inFormat  the incoming format of the texture
 * \param outFormat  the final texture format
 * \return map[6]  a full 6-component map
 */
static void
compute_component_mapping(GLenum inFormat, GLenum outFormat, 
			  GLubyte *map)
{
   const int inFmt = get_map_idx(inFormat);
   const int outFmt = get_map_idx(outFormat);
   const GLubyte *in2rgba = mappings[inFmt].to_rgba;
   const GLubyte *rgba2out = mappings[outFmt].from_rgba;
   int i;
   
   for (i = 0; i < 4; i++)
      map[i] = in2rgba[rgba2out[i]];

   map[ZERO] = ZERO;
   map[ONE] = ONE;   

#if 0
   printf("from %x/%s to %x/%s map %d %d %d %d %d %d\n",
	  inFormat, _mesa_lookup_enum_by_nr(inFormat),
	  outFormat, _mesa_lookup_enum_by_nr(outFormat),
	  map[0], 
	  map[1], 
	  map[2], 
	  map[3], 
	  map[4], 
	  map[5]); 
#endif
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
      GLint convWidth = srcWidth, convHeight = srcHeight;
      GLfloat *convImage;

      /* pre-convolution image buffer (3D) */
      tempImage = (GLfloat *) malloc(srcWidth * srcHeight * srcDepth
                                           * 4 * sizeof(GLfloat));
      if (!tempImage)
         return NULL;

      /* post-convolution image buffer (2D) */
      convImage = (GLfloat *) malloc(srcWidth * srcHeight
                                           * 4 * sizeof(GLfloat));
      if (!convImage) {
         free(tempImage);
         return NULL;
      }

      /* loop over 3D image slices */
      for (img = 0; img < srcDepth; img++) {
         GLfloat *dst = tempImage + img * (srcWidth * srcHeight * 4);

         /* unpack and do transfer ops up to convolution */
         for (row = 0; row < srcHeight; row++) {
            const GLvoid *src = _mesa_image_address(dims, srcPacking,
                                              srcAddr, srcWidth, srcHeight,
                                              srcFormat, srcType, img, row, 0);
            _mesa_unpack_color_span_float(ctx, srcWidth, GL_RGBA, dst,
                                          srcFormat, srcType, src,
                                          srcPacking,
                                          preConvTransferOps);
            dst += srcWidth * 4;
         }

         /* size after optional convolution */
         convWidth = srcWidth;
         convHeight = srcHeight;

#if FEATURE_convolve
         /* do convolution */
         {
            GLfloat *src = tempImage + img * (srcWidth * srcHeight * 4);
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
#endif
         /* do post-convolution transfer and pack into tempImage */
         {
            const GLint logComponents
               = _mesa_components_in_format(logicalBaseFormat);
            const GLfloat *src = convImage;
            GLfloat *dst = tempImage + img * (convWidth * convHeight * 4);
            for (row = 0; row < convHeight; row++) {
               _mesa_pack_rgba_span_float(ctx, convWidth,
                                          (GLfloat (*)[4]) src,
                                          logicalBaseFormat, GL_FLOAT,
                                          dst, &ctx->DefaultPacking,
                                          postConvTransferOps);
               src += convWidth * 4;
               dst += convWidth * logComponents;
            }
         }
      } /* loop over 3D image slices */

      free(convImage);

      /* might need these below */
      srcWidth = convWidth;
      srcHeight = convHeight;
   }
   else {
      /* no convolution */
      const GLint components = _mesa_components_in_format(logicalBaseFormat);
      const GLint srcStride =
         _mesa_image_row_stride(srcPacking, srcWidth, srcFormat, srcType);
      GLfloat *dst;
      GLint img, row;

      tempImage = (GLfloat *) malloc(srcWidth * srcHeight * srcDepth
                                           * components * sizeof(GLfloat));
      if (!tempImage)
         return NULL;

      dst = tempImage;
      for (img = 0; img < srcDepth; img++) {
         const GLubyte *src
            = (const GLubyte *) _mesa_image_address(dims, srcPacking, srcAddr,
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
      GLubyte map[6];

      /* we only promote up to RGB, RGBA and LUMINANCE_ALPHA formats for now */
      ASSERT(textureBaseFormat == GL_RGB || textureBaseFormat == GL_RGBA ||
             textureBaseFormat == GL_LUMINANCE_ALPHA);

      /* The actual texture format should have at least as many components
       * as the logical texture format.
       */
      ASSERT(texComponents >= logComponents);

      newImage = (GLfloat *) malloc(srcWidth * srcHeight * srcDepth
                                          * texComponents * sizeof(GLfloat));
      if (!newImage) {
         free(tempImage);
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

      free(tempImage);
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
GLchan *
_mesa_make_temp_chan_image(GLcontext *ctx, GLuint dims,
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

#if FEATURE_convolve
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
#endif

   /* unpack and transfer the source image */
   tempImage = (GLchan *) malloc(srcWidth * srcHeight * srcDepth
                                       * components * sizeof(GLchan));
   if (!tempImage) {
      if (freeSrcImage) {
         free((void *) srcAddr);
      }
      return NULL;
   }

   dst = tempImage;
   for (img = 0; img < srcDepth; img++) {
      const GLint srcStride =
         _mesa_image_row_stride(srcPacking, srcWidth, srcFormat, srcType);
      const GLubyte *src =
         (const GLubyte *) _mesa_image_address(dims, srcPacking, srcAddr,
                                               srcWidth, srcHeight,
                                               srcFormat, srcType,
                                               img, 0, 0);
      for (row = 0; row < srcHeight; row++) {
         _mesa_unpack_color_span_chan(ctx, srcWidth, logicalBaseFormat, dst,
                                      srcFormat, srcType, src, srcPacking,
                                      transferOps);
         dst += srcWidth * components;
         src += srcStride;
      }
   }

   /* If we made a temporary image for convolution, free it here */
   if (freeSrcImage) {
      free((void *) srcAddr);
   }

   if (logicalBaseFormat != textureBaseFormat) {
      /* one more conversion step */
      GLint texComponents = _mesa_components_in_format(textureBaseFormat);
      GLint logComponents = _mesa_components_in_format(logicalBaseFormat);
      GLchan *newImage;
      GLint i, n;
      GLubyte map[6];

      /* we only promote up to RGB, RGBA and LUMINANCE_ALPHA formats for now */
      ASSERT(textureBaseFormat == GL_RGB || textureBaseFormat == GL_RGBA ||
             textureBaseFormat == GL_LUMINANCE_ALPHA);

      /* The actual texture format should have at least as many components
       * as the logical texture format.
       */
      ASSERT(texComponents >= logComponents);

      newImage = (GLchan *) malloc(srcWidth * srcHeight * srcDepth
                                         * texComponents * sizeof(GLchan));
      if (!newImage) {
         free(tempImage);
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

      free(tempImage);
      tempImage = newImage;
   }

   return tempImage;
}


/**
 * Copy GLubyte pixels from <src> to <dst> with swizzling.
 * \param dst  destination pixels
 * \param dstComponents  number of color components in destination pixels
 * \param src  source pixels
 * \param srcComponents  number of color components in source pixels
 * \param map  the swizzle mapping.  map[X] says where to find the X component
 *             in the source image's pixels.  For example, if the source image
 *             is GL_BGRA and X = red, map[0] yields 2.
 * \param count  number of pixels to copy/swizzle.
 */
static void
swizzle_copy(GLubyte *dst, GLuint dstComponents, const GLubyte *src, 
             GLuint srcComponents, const GLubyte *map, GLuint count)
{
#define SWZ_CPY(dst, src, count, dstComps, srcComps) \
   do {                                              \
      GLuint i;                                      \
      for (i = 0; i < count; i++) {                  \
         GLuint j;                                   \
         if (srcComps == 4) {                        \
            COPY_4UBV(tmp, src);                     \
         }                                           \
         else {                                      \
            for (j = 0; j < srcComps; j++) {         \
               tmp[j] = src[j];                      \
            }                                        \
         }                                           \
         src += srcComps;                            \
         for (j = 0; j < dstComps; j++) {            \
            dst[j] = tmp[map[j]];                    \
         }                                           \
         dst += dstComps;                            \
      }                                              \
   } while (0)

   GLubyte tmp[6];

   tmp[ZERO] = 0x0;
   tmp[ONE] = 0xff;

   ASSERT(srcComponents <= 4);
   ASSERT(dstComponents <= 4);

   switch (dstComponents) {
   case 4:
      switch (srcComponents) {
      case 4:
         SWZ_CPY(dst, src, count, 4, 4);
         break;
      case 3:
         SWZ_CPY(dst, src, count, 4, 3);
         break;
      case 2:
         SWZ_CPY(dst, src, count, 4, 2);
         break;
      case 1:
         SWZ_CPY(dst, src, count, 4, 1);
         break;
      default:
         ;
      }
      break;
   case 3:
      switch (srcComponents) {
      case 4:
         SWZ_CPY(dst, src, count, 3, 4);
         break;
      case 3:
         SWZ_CPY(dst, src, count, 3, 3);
         break;
      case 2:
         SWZ_CPY(dst, src, count, 3, 2);
         break;
      case 1:
         SWZ_CPY(dst, src, count, 3, 1);
         break;
      default:
         ;
      }
      break;
   case 2:
      switch (srcComponents) {
      case 4:
         SWZ_CPY(dst, src, count, 2, 4);
         break;
      case 3:
         SWZ_CPY(dst, src, count, 2, 3);
         break;
      case 2:
         SWZ_CPY(dst, src, count, 2, 2);
         break;
      case 1:
         SWZ_CPY(dst, src, count, 2, 1);
         break;
      default:
         ;
      }
      break;
   case 1:
      switch (srcComponents) {
      case 4:
         SWZ_CPY(dst, src, count, 1, 4);
         break;
      case 3:
         SWZ_CPY(dst, src, count, 1, 3);
         break;
      case 2:
         SWZ_CPY(dst, src, count, 1, 2);
         break;
      case 1:
         SWZ_CPY(dst, src, count, 1, 1);
         break;
      default:
         ;
      }
      break;
   default:
      ;
   }
#undef SWZ_CPY
}



static const GLubyte map_identity[6] = { 0, 1, 2, 3, ZERO, ONE };
static const GLubyte map_3210[6] = { 3, 2, 1, 0, ZERO, ONE };

/* Deal with the _REV input types:
 */
static const GLubyte *
type_mapping( GLenum srcType )
{
   switch (srcType) {
   case GL_BYTE:
   case GL_UNSIGNED_BYTE:
      return map_identity;
   case GL_UNSIGNED_INT_8_8_8_8:
      return _mesa_little_endian() ? map_3210 : map_identity;
   case GL_UNSIGNED_INT_8_8_8_8_REV:
      return _mesa_little_endian() ? map_identity : map_3210;
   default:
      return NULL;
   }
}

/* Mapping required if input type is 
 */
static const GLubyte *
byteswap_mapping( GLboolean swapBytes,
		  GLenum srcType )
{
   if (!swapBytes) 
      return map_identity;

   switch (srcType) {
   case GL_BYTE:
   case GL_UNSIGNED_BYTE:
      return map_identity;
   case GL_UNSIGNED_INT_8_8_8_8:
   case GL_UNSIGNED_INT_8_8_8_8_REV:
      return map_3210;
   default:
      return NULL;
   }
}



/**
 * Transfer a GLubyte texture image with component swizzling.
 */
static void
_mesa_swizzle_ubyte_image(GLcontext *ctx, 
			  GLuint dimensions,
			  GLenum srcFormat,
			  GLenum srcType,

			  GLenum baseInternalFormat,

			  const GLubyte *rgba2dst,
			  GLuint dstComponents,

			  GLvoid *dstAddr,
			  GLint dstXoffset, GLint dstYoffset, GLint dstZoffset,
			  GLint dstRowStride,
                          const GLuint *dstImageOffsets,

			  GLint srcWidth, GLint srcHeight, GLint srcDepth,
			  const GLvoid *srcAddr,
			  const struct gl_pixelstore_attrib *srcPacking )
{
   GLint srcComponents = _mesa_components_in_format(srcFormat);
   const GLubyte *srctype2ubyte, *swap;
   GLubyte map[4], src2base[6], base2rgba[6];
   GLint i;
   const GLint srcRowStride =
      _mesa_image_row_stride(srcPacking, srcWidth,
                             srcFormat, GL_UNSIGNED_BYTE);
   const GLint srcImageStride
      = _mesa_image_image_stride(srcPacking, srcWidth, srcHeight, srcFormat,
                                 GL_UNSIGNED_BYTE);
   const GLubyte *srcImage
      = (const GLubyte *) _mesa_image_address(dimensions, srcPacking, srcAddr,
                                              srcWidth, srcHeight, srcFormat,
                                              GL_UNSIGNED_BYTE, 0, 0, 0);

   (void) ctx;

   /* Translate from src->baseInternal->GL_RGBA->dst.  This will
    * correctly deal with RGBA->RGB->RGBA conversions where the final
    * A value must be 0xff regardless of the incoming alpha values.
    */
   compute_component_mapping(srcFormat, baseInternalFormat, src2base);
   compute_component_mapping(baseInternalFormat, GL_RGBA, base2rgba);
   swap = byteswap_mapping(srcPacking->SwapBytes, srcType);
   srctype2ubyte = type_mapping(srcType);


   for (i = 0; i < 4; i++)
      map[i] = srctype2ubyte[swap[src2base[base2rgba[rgba2dst[i]]]]];

/*    printf("map %d %d %d %d\n", map[0], map[1], map[2], map[3]);  */

   if (srcComponents == dstComponents &&
       srcRowStride == dstRowStride &&
       srcRowStride == srcWidth * srcComponents &&
       dimensions < 3) {
      /* 1 and 2D images only */
      GLubyte *dstImage = (GLubyte *) dstAddr
         + dstYoffset * dstRowStride
         + dstXoffset * dstComponents;
      swizzle_copy(dstImage, dstComponents, srcImage, srcComponents, map, 
		   srcWidth * srcHeight);
   }
   else {
      GLint img, row;
      for (img = 0; img < srcDepth; img++) {
         const GLubyte *srcRow = srcImage;
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * dstComponents
            + dstYoffset * dstRowStride
            + dstXoffset * dstComponents;
         for (row = 0; row < srcHeight; row++) {
	    swizzle_copy(dstRow, dstComponents, srcRow, srcComponents, map, srcWidth);
            dstRow += dstRowStride;
            srcRow += srcRowStride;
         }
         srcImage += srcImageStride;
      }
   }
}


/**
 * Teximage storage routine for when a simple memcpy will do.
 * No pixel transfer operations or special texel encodings allowed.
 * 1D, 2D and 3D images supported.
 */
static void
memcpy_texture(GLcontext *ctx,
	       GLuint dimensions,
               gl_format dstFormat,
               GLvoid *dstAddr,
               GLint dstXoffset, GLint dstYoffset, GLint dstZoffset,
               GLint dstRowStride,
               const GLuint *dstImageOffsets,
               GLint srcWidth, GLint srcHeight, GLint srcDepth,
               GLenum srcFormat, GLenum srcType,
               const GLvoid *srcAddr,
               const struct gl_pixelstore_attrib *srcPacking)
{
   const GLint srcRowStride = _mesa_image_row_stride(srcPacking, srcWidth,
                                                     srcFormat, srcType);
   const GLint srcImageStride = _mesa_image_image_stride(srcPacking,
                                      srcWidth, srcHeight, srcFormat, srcType);
   const GLubyte *srcImage = (const GLubyte *) _mesa_image_address(dimensions,
        srcPacking, srcAddr, srcWidth, srcHeight, srcFormat, srcType, 0, 0, 0);
   const GLuint texelBytes = _mesa_get_format_bytes(dstFormat);
   const GLint bytesPerRow = srcWidth * texelBytes;

#if 0
   /* XXX update/re-enable for dstImageOffsets array */
   const GLint bytesPerImage = srcHeight * bytesPerRow;
   const GLint bytesPerTexture = srcDepth * bytesPerImage;
   GLubyte *dstImage = (GLubyte *) dstAddr
                     + dstZoffset * dstImageStride
                     + dstYoffset * dstRowStride
                     + dstXoffset * texelBytes;

   if (dstRowStride == srcRowStride &&
       dstRowStride == bytesPerRow &&
       ((dstImageStride == srcImageStride &&
         dstImageStride == bytesPerImage) ||
        (srcDepth == 1))) {
      /* one big memcpy */
      ctx->Driver.TextureMemCpy(dstImage, srcImage, bytesPerTexture);
   }
   else
   {
      GLint img, row;
      for (img = 0; img < srcDepth; img++) {
         const GLubyte *srcRow = srcImage;
         GLubyte *dstRow = dstImage;
         for (row = 0; row < srcHeight; row++) {
            ctx->Driver.TextureMemCpy(dstRow, srcRow, bytesPerRow);
            dstRow += dstRowStride;
            srcRow += srcRowStride;
         }
         srcImage += srcImageStride;
         dstImage += dstImageStride;
      }
   }
#endif

   GLint img, row;
   for (img = 0; img < srcDepth; img++) {
      const GLubyte *srcRow = srcImage;
      GLubyte *dstRow = (GLubyte *) dstAddr
         + dstImageOffsets[dstZoffset + img] * texelBytes
         + dstYoffset * dstRowStride
         + dstXoffset * texelBytes;
      for (row = 0; row < srcHeight; row++) {
         ctx->Driver.TextureMemCpy(dstRow, srcRow, bytesPerRow);
         dstRow += dstRowStride;
         srcRow += srcRowStride;
      }
      srcImage += srcImageStride;
   }
}



/**
 * Store a 32-bit integer depth component texture image.
 */
static GLboolean
_mesa_texstore_z32(TEXSTORE_PARAMS)
{
   const GLuint depthScale = 0xffffffff;
   const GLuint texelBytes = _mesa_get_format_bytes(dstFormat);
   (void) dims;
   ASSERT(dstFormat == MESA_FORMAT_Z32);
   ASSERT(texelBytes == sizeof(GLuint));

   if (ctx->Pixel.DepthScale == 1.0f &&
       ctx->Pixel.DepthBias == 0.0f &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == GL_DEPTH_COMPONENT &&
       srcFormat == GL_DEPTH_COMPONENT &&
       srcType == GL_UNSIGNED_INT) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      GLint img, row;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * texelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * texelBytes;
         for (row = 0; row < srcHeight; row++) {
            const GLvoid *src = _mesa_image_address(dims, srcPacking,
                srcAddr, srcWidth, srcHeight, srcFormat, srcType, img, row, 0);
            _mesa_unpack_depth_span(ctx, srcWidth,
                                    GL_UNSIGNED_INT, (GLuint *) dstRow,
                                    depthScale, srcType, src, srcPacking);
            dstRow += dstRowStride;
         }
      }
   }
   return GL_TRUE;
}


/**
 * Store a 24-bit integer depth component texture image.
 */
static GLboolean
_mesa_texstore_x8_z24(TEXSTORE_PARAMS)
{
   const GLuint depthScale = 0xffffff;
   const GLuint texelBytes = 4;

   (void) dims;
   ASSERT(dstFormat == MESA_FORMAT_X8_Z24);

   {
      /* general path */
      GLint img, row;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * texelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * texelBytes;
         for (row = 0; row < srcHeight; row++) {
            const GLvoid *src = _mesa_image_address(dims, srcPacking,
                srcAddr, srcWidth, srcHeight, srcFormat, srcType, img, row, 0);
            _mesa_unpack_depth_span(ctx, srcWidth,
                                    GL_UNSIGNED_INT, (GLuint *) dstRow,
                                    depthScale, srcType, src, srcPacking);
            dstRow += dstRowStride;
         }
      }
   }
   return GL_TRUE;
}


/**
 * Store a 24-bit integer depth component texture image.
 */
static GLboolean
_mesa_texstore_z24_x8(TEXSTORE_PARAMS)
{
   const GLuint depthScale = 0xffffff;
   const GLuint texelBytes = 4;

   (void) dims;
   ASSERT(dstFormat == MESA_FORMAT_Z24_X8);

   {
      /* general path */
      GLint img, row;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * texelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * texelBytes;
         for (row = 0; row < srcHeight; row++) {
            const GLvoid *src = _mesa_image_address(dims, srcPacking,
                srcAddr, srcWidth, srcHeight, srcFormat, srcType, img, row, 0);
            GLuint *dst = (GLuint *) dstRow;
            GLint i;
            _mesa_unpack_depth_span(ctx, srcWidth,
                                    GL_UNSIGNED_INT, dst,
                                    depthScale, srcType, src, srcPacking);
            for (i = 0; i < srcWidth; i++)
               dst[i] <<= 8;
            dstRow += dstRowStride;
         }
      }
   }
   return GL_TRUE;
}


/**
 * Store a 16-bit integer depth component texture image.
 */
static GLboolean
_mesa_texstore_z16(TEXSTORE_PARAMS)
{
   const GLuint depthScale = 0xffff;
   const GLuint texelBytes = _mesa_get_format_bytes(dstFormat);
   (void) dims;
   ASSERT(dstFormat == MESA_FORMAT_Z16);
   ASSERT(texelBytes == sizeof(GLushort));

   if (ctx->Pixel.DepthScale == 1.0f &&
       ctx->Pixel.DepthBias == 0.0f &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == GL_DEPTH_COMPONENT &&
       srcFormat == GL_DEPTH_COMPONENT &&
       srcType == GL_UNSIGNED_SHORT) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      GLint img, row;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * texelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * texelBytes;
         for (row = 0; row < srcHeight; row++) {
            const GLvoid *src = _mesa_image_address(dims, srcPacking,
                srcAddr, srcWidth, srcHeight, srcFormat, srcType, img, row, 0);
            GLushort *dst16 = (GLushort *) dstRow;
            _mesa_unpack_depth_span(ctx, srcWidth,
                                    GL_UNSIGNED_SHORT, dst16, depthScale,
                                    srcType, src, srcPacking);
            dstRow += dstRowStride;
         }
      }
   }
   return GL_TRUE;
}


/**
 * Store an rgb565 or rgb565_rev texture image.
 */
static GLboolean
_mesa_texstore_rgb565(TEXSTORE_PARAMS)
{
   const GLuint texelBytes = _mesa_get_format_bytes(dstFormat);
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);

   ASSERT(dstFormat == MESA_FORMAT_RGB565 ||
          dstFormat == MESA_FORMAT_RGB565_REV);
   ASSERT(texelBytes == 2);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       dstFormat == MESA_FORMAT_RGB565 &&
       baseInternalFormat == GL_RGB &&
       srcFormat == GL_RGB &&
       srcType == GL_UNSIGNED_SHORT_5_6_5) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
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
      const GLint srcRowStride =
         _mesa_image_row_stride(srcPacking, srcWidth, srcFormat, srcType);
      const GLubyte *src = (const GLubyte *)
         _mesa_image_address(dims, srcPacking, srcAddr, srcWidth, srcHeight,
                             srcFormat, srcType, 0, 0, 0);
      GLubyte *dst = (GLubyte *) dstAddr
                   + dstYoffset * dstRowStride
                   + dstXoffset * texelBytes;
      GLint row, col;
      for (row = 0; row < srcHeight; row++) {
         const GLubyte *srcUB = (const GLubyte *) src;
         GLushort *dstUS = (GLushort *) dst;
         /* check for byteswapped format */
         if (dstFormat == MESA_FORMAT_RGB565) {
            for (col = 0; col < srcWidth; col++) {
               dstUS[col] = PACK_COLOR_565( srcUB[0], srcUB[1], srcUB[2] );
               srcUB += 3;
            }
         }
         else {
            for (col = 0; col < srcWidth; col++) {
               dstUS[col] = PACK_COLOR_565_REV( srcUB[0], srcUB[1], srcUB[2] );
               srcUB += 3;
            }
         }
         dst += dstRowStride;
         src += srcRowStride;
      }
   }
   else {
      /* general path */
      const GLchan *tempImage = _mesa_make_temp_chan_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLchan *src = tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * texelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * texelBytes;
         for (row = 0; row < srcHeight; row++) {
            GLushort *dstUS = (GLushort *) dstRow;
            /* check for byteswapped format */
            if (dstFormat == MESA_FORMAT_RGB565) {
               for (col = 0; col < srcWidth; col++) {
                  dstUS[col] = PACK_COLOR_565( CHAN_TO_UBYTE(src[RCOMP]),
                                               CHAN_TO_UBYTE(src[GCOMP]),
                                               CHAN_TO_UBYTE(src[BCOMP]) );
                  src += 3;
               }
            }
            else {
               for (col = 0; col < srcWidth; col++) {
                  dstUS[col] = PACK_COLOR_565_REV( CHAN_TO_UBYTE(src[RCOMP]),
                                                   CHAN_TO_UBYTE(src[GCOMP]),
                                                   CHAN_TO_UBYTE(src[BCOMP]) );
                  src += 3;
               }
            }
            dstRow += dstRowStride;
         }
      }
      free((void *) tempImage);
   }
   return GL_TRUE;
}


/**
 * Store a texture in MESA_FORMAT_RGBA8888 or MESA_FORMAT_RGBA8888_REV.
 */
static GLboolean
_mesa_texstore_rgba8888(TEXSTORE_PARAMS)
{
   const GLboolean littleEndian = _mesa_little_endian();
   const GLuint texelBytes = _mesa_get_format_bytes(dstFormat);
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);

   ASSERT(dstFormat == MESA_FORMAT_RGBA8888 ||
          dstFormat == MESA_FORMAT_RGBA8888_REV);
   ASSERT(texelBytes == 4);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       dstFormat == MESA_FORMAT_RGBA8888 &&
       baseInternalFormat == GL_RGBA &&
      ((srcFormat == GL_RGBA && srcType == GL_UNSIGNED_INT_8_8_8_8) ||
       (srcFormat == GL_RGBA && srcType == GL_UNSIGNED_BYTE && !littleEndian) ||
       (srcFormat == GL_ABGR_EXT && srcType == GL_UNSIGNED_INT_8_8_8_8_REV) ||
       (srcFormat == GL_ABGR_EXT && srcType == GL_UNSIGNED_BYTE && littleEndian))) {
       /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       dstFormat == MESA_FORMAT_RGBA8888_REV &&
       baseInternalFormat == GL_RGBA &&
      ((srcFormat == GL_RGBA && srcType == GL_UNSIGNED_INT_8_8_8_8_REV) ||
       (srcFormat == GL_RGBA && srcType == GL_UNSIGNED_BYTE && littleEndian) ||
       (srcFormat == GL_ABGR_EXT && srcType == GL_UNSIGNED_INT_8_8_8_8) ||
       (srcFormat == GL_ABGR_EXT && srcType == GL_UNSIGNED_BYTE && !littleEndian))) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else if (!ctx->_ImageTransferState &&
	    (srcType == GL_UNSIGNED_BYTE ||
	     srcType == GL_UNSIGNED_INT_8_8_8_8 ||
	     srcType == GL_UNSIGNED_INT_8_8_8_8_REV) &&
	    can_swizzle(baseInternalFormat) &&
	    can_swizzle(srcFormat)) {

      GLubyte dstmap[4];

      /* dstmap - how to swizzle from RGBA to dst format:
       */
      if ((littleEndian && dstFormat == MESA_FORMAT_RGBA8888) ||
	  (!littleEndian && dstFormat == MESA_FORMAT_RGBA8888_REV)) {
	 dstmap[3] = 0;
	 dstmap[2] = 1;
	 dstmap[1] = 2;
	 dstmap[0] = 3;
      }
      else {
	 dstmap[3] = 3;
	 dstmap[2] = 2;
	 dstmap[1] = 1;
	 dstmap[0] = 0;
      }
      
      _mesa_swizzle_ubyte_image(ctx, dims,
				srcFormat,
				srcType,
				baseInternalFormat,
				dstmap, 4,
				dstAddr, dstXoffset, dstYoffset, dstZoffset,
				dstRowStride, dstImageOffsets,
				srcWidth, srcHeight, srcDepth, srcAddr,
				srcPacking);      
   }
   else {
      /* general path */
      const GLchan *tempImage = _mesa_make_temp_chan_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLchan *src = tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * texelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * texelBytes;
         for (row = 0; row < srcHeight; row++) {
            GLuint *dstUI = (GLuint *) dstRow;
            if (dstFormat == MESA_FORMAT_RGBA8888) {
               for (col = 0; col < srcWidth; col++) {
                  dstUI[col] = PACK_COLOR_8888( CHAN_TO_UBYTE(src[RCOMP]),
                                                CHAN_TO_UBYTE(src[GCOMP]),
                                                CHAN_TO_UBYTE(src[BCOMP]),
                                                CHAN_TO_UBYTE(src[ACOMP]) );
                  src += 4;
               }
            }
            else {
               for (col = 0; col < srcWidth; col++) {
                  dstUI[col] = PACK_COLOR_8888_REV( CHAN_TO_UBYTE(src[RCOMP]),
                                                    CHAN_TO_UBYTE(src[GCOMP]),
                                                    CHAN_TO_UBYTE(src[BCOMP]),
                                                    CHAN_TO_UBYTE(src[ACOMP]) );
                  src += 4;
               }
            }
            dstRow += dstRowStride;
         }
      }
      free((void *) tempImage);
   }
   return GL_TRUE;
}


static GLboolean
_mesa_texstore_argb8888(TEXSTORE_PARAMS)
{
   const GLboolean littleEndian = _mesa_little_endian();
   const GLuint texelBytes = _mesa_get_format_bytes(dstFormat);
   const GLenum baseFormat = GL_RGBA;

   ASSERT(dstFormat == MESA_FORMAT_ARGB8888 ||
          dstFormat == MESA_FORMAT_ARGB8888_REV ||
          dstFormat == MESA_FORMAT_XRGB8888 ||
          dstFormat == MESA_FORMAT_XRGB8888_REV );
   ASSERT(texelBytes == 4);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       (dstFormat == MESA_FORMAT_ARGB8888 ||
        dstFormat == MESA_FORMAT_XRGB8888) &&
       baseInternalFormat == GL_RGBA &&
       srcFormat == GL_BGRA &&
       ((srcType == GL_UNSIGNED_BYTE && littleEndian) ||
        srcType == GL_UNSIGNED_INT_8_8_8_8_REV)) {
      /* simple memcpy path (little endian) */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       (dstFormat == MESA_FORMAT_ARGB8888_REV ||
        dstFormat == MESA_FORMAT_XRGB8888_REV) &&
       baseInternalFormat == GL_RGBA &&
       srcFormat == GL_BGRA &&
       ((srcType == GL_UNSIGNED_BYTE && !littleEndian) ||
        srcType == GL_UNSIGNED_INT_8_8_8_8)) {
      /* simple memcpy path (big endian) */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else if (!ctx->_ImageTransferState &&
            !srcPacking->SwapBytes &&
	    (dstFormat == MESA_FORMAT_ARGB8888 ||
             dstFormat == MESA_FORMAT_XRGB8888) &&
            srcFormat == GL_RGB &&
	    (baseInternalFormat == GL_RGBA ||
	     baseInternalFormat == GL_RGB) &&
            srcType == GL_UNSIGNED_BYTE) {
      int img, row, col;
      for (img = 0; img < srcDepth; img++) {
         const GLint srcRowStride =
            _mesa_image_row_stride(srcPacking, srcWidth, srcFormat, srcType);
         GLubyte *srcRow = (GLubyte *) _mesa_image_address(dims, srcPacking,
                  srcAddr, srcWidth, srcHeight, srcFormat, srcType, img, 0, 0);
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * texelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * texelBytes;
         for (row = 0; row < srcHeight; row++) {
            GLuint *d4 = (GLuint *) dstRow;
            for (col = 0; col < srcWidth; col++) {
               d4[col] = PACK_COLOR_8888(0xff,
                                         srcRow[col * 3 + RCOMP],
                                         srcRow[col * 3 + GCOMP],
                                         srcRow[col * 3 + BCOMP]);
            }
            dstRow += dstRowStride;
            srcRow += srcRowStride;
         }
      }
   }
   else if (!ctx->_ImageTransferState &&
            !srcPacking->SwapBytes &&
	    dstFormat == MESA_FORMAT_ARGB8888 &&
            srcFormat == GL_RGBA &&
	    baseInternalFormat == GL_RGBA &&
            srcType == GL_UNSIGNED_BYTE) {
      /* same as above case, but src data has alpha too */
      GLint img, row, col;
      /* For some reason, streaming copies to write-combined regions
       * are extremely sensitive to the characteristics of how the
       * source data is retrieved.  By reordering the source reads to
       * be in-order, the speed of this operation increases by half.
       * Strangely the same isn't required for the RGB path, above.
       */
      for (img = 0; img < srcDepth; img++) {
         const GLint srcRowStride =
            _mesa_image_row_stride(srcPacking, srcWidth, srcFormat, srcType);
         GLubyte *srcRow = (GLubyte *) _mesa_image_address(dims, srcPacking,
                  srcAddr, srcWidth, srcHeight, srcFormat, srcType, img, 0, 0);
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * texelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * texelBytes;
         for (row = 0; row < srcHeight; row++) {
            GLuint *d4 = (GLuint *) dstRow;
            for (col = 0; col < srcWidth; col++) {
               d4[col] = PACK_COLOR_8888(srcRow[col * 4 + ACOMP],
                                         srcRow[col * 4 + RCOMP],
                                         srcRow[col * 4 + GCOMP],
                                         srcRow[col * 4 + BCOMP]);
            }
            dstRow += dstRowStride;
            srcRow += srcRowStride;
         }
      }
   }
   else if (!ctx->_ImageTransferState &&
	    (srcType == GL_UNSIGNED_BYTE ||
	     srcType == GL_UNSIGNED_INT_8_8_8_8 ||
	     srcType == GL_UNSIGNED_INT_8_8_8_8_REV) &&
	    can_swizzle(baseInternalFormat) &&	   
	    can_swizzle(srcFormat)) {

      GLubyte dstmap[4];

      /* dstmap - how to swizzle from RGBA to dst format:
       */
      if ((littleEndian && dstFormat == MESA_FORMAT_ARGB8888) ||
          (littleEndian && dstFormat == MESA_FORMAT_XRGB8888) ||
	  (!littleEndian && dstFormat == MESA_FORMAT_ARGB8888_REV) ||
	  (!littleEndian && dstFormat == MESA_FORMAT_XRGB8888_REV)) {
	 dstmap[3] = 3;		/* alpha */
	 dstmap[2] = 0;		/* red */
	 dstmap[1] = 1;		/* green */
	 dstmap[0] = 2;		/* blue */
      }
      else {
	 assert((littleEndian && dstFormat == MESA_FORMAT_ARGB8888_REV) ||
		(!littleEndian && dstFormat == MESA_FORMAT_ARGB8888) ||
		(littleEndian && dstFormat == MESA_FORMAT_XRGB8888_REV) ||
		(!littleEndian && dstFormat == MESA_FORMAT_XRGB8888));
	 dstmap[3] = 2;
	 dstmap[2] = 1;
	 dstmap[1] = 0;
	 dstmap[0] = 3;
      }
 
      _mesa_swizzle_ubyte_image(ctx, dims,
				srcFormat,
				srcType,

				baseInternalFormat,
				dstmap, 4,
				dstAddr, dstXoffset, dstYoffset, dstZoffset,
				dstRowStride,
                                dstImageOffsets,
				srcWidth, srcHeight, srcDepth, srcAddr,
				srcPacking);      
   }
   else {
      /* general path */
      const GLchan *tempImage = _mesa_make_temp_chan_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLchan *src = tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * texelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * texelBytes;
         for (row = 0; row < srcHeight; row++) {
            GLuint *dstUI = (GLuint *) dstRow;
            if (dstFormat == MESA_FORMAT_ARGB8888) {
               for (col = 0; col < srcWidth; col++) {
                  dstUI[col] = PACK_COLOR_8888( CHAN_TO_UBYTE(src[ACOMP]),
                                                CHAN_TO_UBYTE(src[RCOMP]),
                                                CHAN_TO_UBYTE(src[GCOMP]),
                                                CHAN_TO_UBYTE(src[BCOMP]) );
                  src += 4;
               }
            }
            else if (dstFormat == MESA_FORMAT_XRGB8888) {
               for (col = 0; col < srcWidth; col++) {
                  dstUI[col] = PACK_COLOR_8888( 0xff,
                                                CHAN_TO_UBYTE(src[RCOMP]),
                                                CHAN_TO_UBYTE(src[GCOMP]),
                                                CHAN_TO_UBYTE(src[BCOMP]) );
                  src += 4;
               }
            }
            else {
               for (col = 0; col < srcWidth; col++) {
                  dstUI[col] = PACK_COLOR_8888_REV( CHAN_TO_UBYTE(src[ACOMP]),
                                                    CHAN_TO_UBYTE(src[RCOMP]),
                                                    CHAN_TO_UBYTE(src[GCOMP]),
                                                    CHAN_TO_UBYTE(src[BCOMP]) );
                  src += 4;
               }
            }
            dstRow += dstRowStride;
         }
      }
      free((void *) tempImage);
   }
   return GL_TRUE;
}


static GLboolean
_mesa_texstore_rgb888(TEXSTORE_PARAMS)
{
   const GLboolean littleEndian = _mesa_little_endian();
   const GLuint texelBytes = _mesa_get_format_bytes(dstFormat);
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);

   ASSERT(dstFormat == MESA_FORMAT_RGB888);
   ASSERT(texelBytes == 3);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == GL_RGB &&
       srcFormat == GL_BGR &&
       srcType == GL_UNSIGNED_BYTE &&
       littleEndian) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else if (!ctx->_ImageTransferState &&
            !srcPacking->SwapBytes &&
            srcFormat == GL_RGBA &&
            srcType == GL_UNSIGNED_BYTE) {
      /* extract RGB from RGBA */
      GLint img, row, col;
      for (img = 0; img < srcDepth; img++) {
         const GLint srcRowStride =
            _mesa_image_row_stride(srcPacking, srcWidth, srcFormat, srcType);
         GLubyte *srcRow = (GLubyte *) _mesa_image_address(dims, srcPacking,
                  srcAddr, srcWidth, srcHeight, srcFormat, srcType, img, 0, 0);
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * texelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * texelBytes;
         for (row = 0; row < srcHeight; row++) {
            for (col = 0; col < srcWidth; col++) {
               dstRow[col * 3 + 0] = srcRow[col * 4 + BCOMP];
               dstRow[col * 3 + 1] = srcRow[col * 4 + GCOMP];
               dstRow[col * 3 + 2] = srcRow[col * 4 + RCOMP];
            }
            dstRow += dstRowStride;
            srcRow += srcRowStride;
         }
      }
   }
   else if (!ctx->_ImageTransferState &&
	    srcType == GL_UNSIGNED_BYTE &&
	    can_swizzle(baseInternalFormat) &&
	    can_swizzle(srcFormat)) {

      GLubyte dstmap[4];

      /* dstmap - how to swizzle from RGBA to dst format:
       */
      dstmap[0] = 2;
      dstmap[1] = 1;
      dstmap[2] = 0;
      dstmap[3] = ONE;		/* ? */
      
      _mesa_swizzle_ubyte_image(ctx, dims,
				srcFormat,
				srcType,
				baseInternalFormat,
				dstmap, 3,
				dstAddr, dstXoffset, dstYoffset, dstZoffset,
				dstRowStride, dstImageOffsets,
				srcWidth, srcHeight, srcDepth, srcAddr,
				srcPacking);      
   }
   else {
      /* general path */
      const GLchan *tempImage = _mesa_make_temp_chan_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLchan *src = (const GLchan *) tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * texelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * texelBytes;
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
      }
      free((void *) tempImage);
   }
   return GL_TRUE;
}


static GLboolean
_mesa_texstore_bgr888(TEXSTORE_PARAMS)
{
   const GLboolean littleEndian = _mesa_little_endian();
   const GLuint texelBytes = _mesa_get_format_bytes(dstFormat);
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);

   ASSERT(dstFormat == MESA_FORMAT_BGR888);
   ASSERT(texelBytes == 3);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == GL_RGB &&
       srcFormat == GL_RGB &&
       srcType == GL_UNSIGNED_BYTE &&
       littleEndian) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else if (!ctx->_ImageTransferState &&
            !srcPacking->SwapBytes &&
            srcFormat == GL_RGBA &&
            srcType == GL_UNSIGNED_BYTE) {
      /* extract BGR from RGBA */
      int img, row, col;
      for (img = 0; img < srcDepth; img++) {
         const GLint srcRowStride =
            _mesa_image_row_stride(srcPacking, srcWidth, srcFormat, srcType);
         GLubyte *srcRow = (GLubyte *) _mesa_image_address(dims, srcPacking,
                  srcAddr, srcWidth, srcHeight, srcFormat, srcType, img, 0, 0);
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * texelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * texelBytes;
         for (row = 0; row < srcHeight; row++) {
            for (col = 0; col < srcWidth; col++) {
               dstRow[col * 3 + 0] = srcRow[col * 4 + RCOMP];
               dstRow[col * 3 + 1] = srcRow[col * 4 + GCOMP];
               dstRow[col * 3 + 2] = srcRow[col * 4 + BCOMP];
            }
            dstRow += dstRowStride;
            srcRow += srcRowStride;
         }
      }
   }
   else if (!ctx->_ImageTransferState &&
	    srcType == GL_UNSIGNED_BYTE &&
	    can_swizzle(baseInternalFormat) &&
	    can_swizzle(srcFormat)) {

      GLubyte dstmap[4];

      /* dstmap - how to swizzle from RGBA to dst format:
       */
      dstmap[0] = 0;
      dstmap[1] = 1;
      dstmap[2] = 2;
      dstmap[3] = ONE;		/* ? */
      
      _mesa_swizzle_ubyte_image(ctx, dims,
				srcFormat,
				srcType,
				baseInternalFormat,
				dstmap, 3,
				dstAddr, dstXoffset, dstYoffset, dstZoffset,
				dstRowStride, dstImageOffsets,
				srcWidth, srcHeight, srcDepth, srcAddr,
				srcPacking);      
   }   
   else {
      /* general path */
      const GLchan *tempImage = _mesa_make_temp_chan_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLchan *src = (const GLchan *) tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * texelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * texelBytes;
         for (row = 0; row < srcHeight; row++) {
            for (col = 0; col < srcWidth; col++) {
               dstRow[col * 3 + 0] = CHAN_TO_UBYTE(src[RCOMP]);
               dstRow[col * 3 + 1] = CHAN_TO_UBYTE(src[GCOMP]);
               dstRow[col * 3 + 2] = CHAN_TO_UBYTE(src[BCOMP]);
               src += 3;
            }
            dstRow += dstRowStride;
         }
      }
      free((void *) tempImage);
   }
   return GL_TRUE;
}


static GLboolean
_mesa_texstore_argb4444(TEXSTORE_PARAMS)
{
   const GLuint texelBytes = _mesa_get_format_bytes(dstFormat);
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);

   ASSERT(dstFormat == MESA_FORMAT_ARGB4444 ||
          dstFormat == MESA_FORMAT_ARGB4444_REV);
   ASSERT(texelBytes == 2);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       dstFormat == MESA_FORMAT_ARGB4444 &&
       baseInternalFormat == GL_RGBA &&
       srcFormat == GL_BGRA &&
       srcType == GL_UNSIGNED_SHORT_4_4_4_4_REV) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLchan *tempImage = _mesa_make_temp_chan_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLchan *src = tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * texelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * texelBytes;
         for (row = 0; row < srcHeight; row++) {
            GLushort *dstUS = (GLushort *) dstRow;
            if (dstFormat == MESA_FORMAT_ARGB4444) {
               for (col = 0; col < srcWidth; col++) {
                  dstUS[col] = PACK_COLOR_4444( CHAN_TO_UBYTE(src[ACOMP]),
                                                CHAN_TO_UBYTE(src[RCOMP]),
                                                CHAN_TO_UBYTE(src[GCOMP]),
                                                CHAN_TO_UBYTE(src[BCOMP]) );
                  src += 4;
               }
            }
            else {
               for (col = 0; col < srcWidth; col++) {
                  dstUS[col] = PACK_COLOR_4444_REV( CHAN_TO_UBYTE(src[ACOMP]),
                                                    CHAN_TO_UBYTE(src[RCOMP]),
                                                    CHAN_TO_UBYTE(src[GCOMP]),
                                                    CHAN_TO_UBYTE(src[BCOMP]) );
                  src += 4;
               }
            }
            dstRow += dstRowStride;
         }
      }
      free((void *) tempImage);
   }
   return GL_TRUE;
}

static GLboolean
_mesa_texstore_rgba5551(TEXSTORE_PARAMS)
{
   const GLuint texelBytes = _mesa_get_format_bytes(dstFormat);
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);

   ASSERT(dstFormat == MESA_FORMAT_RGBA5551);
   ASSERT(texelBytes == 2);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       dstFormat == MESA_FORMAT_RGBA5551 &&
       baseInternalFormat == GL_RGBA &&
       srcFormat == GL_RGBA &&
       srcType == GL_UNSIGNED_SHORT_5_5_5_1) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLchan *tempImage = _mesa_make_temp_chan_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLchan *src =tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * texelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * texelBytes;
         for (row = 0; row < srcHeight; row++) {
            GLushort *dstUS = (GLushort *) dstRow;
	    for (col = 0; col < srcWidth; col++) {
	       dstUS[col] = PACK_COLOR_5551( CHAN_TO_UBYTE(src[RCOMP]),
					     CHAN_TO_UBYTE(src[GCOMP]),
					     CHAN_TO_UBYTE(src[BCOMP]),
					     CHAN_TO_UBYTE(src[ACOMP]) );
	      src += 4;
	    }
            dstRow += dstRowStride;
         }
      }
      free((void *) tempImage);
   }
   return GL_TRUE;
}

static GLboolean
_mesa_texstore_argb1555(TEXSTORE_PARAMS)
{
   const GLuint texelBytes = _mesa_get_format_bytes(dstFormat);
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);

   ASSERT(dstFormat == MESA_FORMAT_ARGB1555 ||
          dstFormat == MESA_FORMAT_ARGB1555_REV);
   ASSERT(texelBytes == 2);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       dstFormat == MESA_FORMAT_ARGB1555 &&
       baseInternalFormat == GL_RGBA &&
       srcFormat == GL_BGRA &&
       srcType == GL_UNSIGNED_SHORT_1_5_5_5_REV) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLchan *tempImage = _mesa_make_temp_chan_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLchan *src =tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * texelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * texelBytes;
         for (row = 0; row < srcHeight; row++) {
            GLushort *dstUS = (GLushort *) dstRow;
            if (dstFormat == MESA_FORMAT_ARGB1555) {
               for (col = 0; col < srcWidth; col++) {
                  dstUS[col] = PACK_COLOR_1555( CHAN_TO_UBYTE(src[ACOMP]),
                                                CHAN_TO_UBYTE(src[RCOMP]),
                                                CHAN_TO_UBYTE(src[GCOMP]),
                                                CHAN_TO_UBYTE(src[BCOMP]) );
                  src += 4;
               }
            }
            else {
               for (col = 0; col < srcWidth; col++) {
                  dstUS[col] = PACK_COLOR_1555_REV( CHAN_TO_UBYTE(src[ACOMP]),
                                                    CHAN_TO_UBYTE(src[RCOMP]),
                                                    CHAN_TO_UBYTE(src[GCOMP]),
                                                    CHAN_TO_UBYTE(src[BCOMP]) );
                  src += 4;
               }
            }
            dstRow += dstRowStride;
         }
      }
      free((void *) tempImage);
   }
   return GL_TRUE;
}


static GLboolean
_mesa_texstore_al88(TEXSTORE_PARAMS)
{
   const GLboolean littleEndian = _mesa_little_endian();
   const GLuint texelBytes = _mesa_get_format_bytes(dstFormat);
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);

   ASSERT(dstFormat == MESA_FORMAT_AL88 ||
          dstFormat == MESA_FORMAT_AL88_REV);
   ASSERT(texelBytes == 2);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       dstFormat == MESA_FORMAT_AL88 &&
       baseInternalFormat == GL_LUMINANCE_ALPHA &&
       srcFormat == GL_LUMINANCE_ALPHA &&
       srcType == GL_UNSIGNED_BYTE &&
       littleEndian) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else if (!ctx->_ImageTransferState &&
	    littleEndian &&
	    srcType == GL_UNSIGNED_BYTE &&
	    can_swizzle(baseInternalFormat) &&
	    can_swizzle(srcFormat)) {

      GLubyte dstmap[4];

      /* dstmap - how to swizzle from RGBA to dst format:
       */
      if ((littleEndian && dstFormat == MESA_FORMAT_AL88) ||
	  (!littleEndian && dstFormat == MESA_FORMAT_AL88_REV)) {
	 dstmap[0] = 0;
	 dstmap[1] = 3;
      }
      else {
	 dstmap[0] = 3;
	 dstmap[1] = 0;
      }
      dstmap[2] = ZERO;		/* ? */
      dstmap[3] = ONE;		/* ? */
      
      _mesa_swizzle_ubyte_image(ctx, dims,
				srcFormat,
				srcType,
				baseInternalFormat,
				dstmap, 2,
				dstAddr, dstXoffset, dstYoffset, dstZoffset,
				dstRowStride, dstImageOffsets,
				srcWidth, srcHeight, srcDepth, srcAddr,
				srcPacking);      
   }   
   else {
      /* general path */
      const GLchan *tempImage = _mesa_make_temp_chan_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLchan *src = tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * texelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * texelBytes;
         for (row = 0; row < srcHeight; row++) {
            GLushort *dstUS = (GLushort *) dstRow;
            if (dstFormat == MESA_FORMAT_AL88) {
               for (col = 0; col < srcWidth; col++) {
                  /* src[0] is luminance, src[1] is alpha */
                 dstUS[col] = PACK_COLOR_88( CHAN_TO_UBYTE(src[1]),
                                             CHAN_TO_UBYTE(src[0]) );
                 src += 2;
               }
            }
            else {
               for (col = 0; col < srcWidth; col++) {
                  /* src[0] is luminance, src[1] is alpha */
                 dstUS[col] = PACK_COLOR_88_REV( CHAN_TO_UBYTE(src[1]),
                                                 CHAN_TO_UBYTE(src[0]) );
                 src += 2;
               }
            }
            dstRow += dstRowStride;
         }
      }
      free((void *) tempImage);
   }
   return GL_TRUE;
}


static GLboolean
_mesa_texstore_al1616(TEXSTORE_PARAMS)
{
   const GLboolean littleEndian = _mesa_little_endian();
   const GLuint texelBytes = _mesa_get_format_bytes(dstFormat);
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);

   ASSERT(dstFormat == MESA_FORMAT_AL1616 ||
          dstFormat == MESA_FORMAT_AL1616_REV);
   ASSERT(texelBytes == 4);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       dstFormat == MESA_FORMAT_AL1616 &&
       baseInternalFormat == GL_LUMINANCE_ALPHA &&
       srcFormat == GL_LUMINANCE_ALPHA &&
       srcType == GL_UNSIGNED_SHORT &&
       littleEndian) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLfloat *tempImage = make_temp_float_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLfloat *src = tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * texelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * texelBytes;
         for (row = 0; row < srcHeight; row++) {
            GLuint *dstUI = (GLuint *) dstRow;
            if (dstFormat == MESA_FORMAT_AL1616) {
               for (col = 0; col < srcWidth; col++) {
		  GLushort l, a;

		  UNCLAMPED_FLOAT_TO_USHORT(l, src[0]);
		  UNCLAMPED_FLOAT_TO_USHORT(a, src[1]);
		  dstUI[col] = PACK_COLOR_1616(a, l);
		  src += 2;
               }
            }
            else {
               for (col = 0; col < srcWidth; col++) {
		  GLushort l, a;

		  UNCLAMPED_FLOAT_TO_USHORT(l, src[0]);
		  UNCLAMPED_FLOAT_TO_USHORT(a, src[1]);
		  dstUI[col] = PACK_COLOR_1616_REV(a, l);
		  src += 2;
               }
            }
            dstRow += dstRowStride;
         }
      }
      free((void *) tempImage);
   }
   return GL_TRUE;
}


static GLboolean
_mesa_texstore_rgb332(TEXSTORE_PARAMS)
{
   const GLuint texelBytes = _mesa_get_format_bytes(dstFormat);
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);

   ASSERT(dstFormat == MESA_FORMAT_RGB332);
   ASSERT(texelBytes == 1);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == GL_RGB &&
       srcFormat == GL_RGB && srcType == GL_UNSIGNED_BYTE_3_3_2) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLchan *tempImage = _mesa_make_temp_chan_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLchan *src = tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * texelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * texelBytes;
         for (row = 0; row < srcHeight; row++) {
            for (col = 0; col < srcWidth; col++) {
               dstRow[col] = PACK_COLOR_332( CHAN_TO_UBYTE(src[RCOMP]),
                                             CHAN_TO_UBYTE(src[GCOMP]),
                                             CHAN_TO_UBYTE(src[BCOMP]) );
               src += 3;
            }
            dstRow += dstRowStride;
         }
      }
      free((void *) tempImage);
   }
   return GL_TRUE;
}


/**
 * Texstore for _mesa_texformat_a8, _mesa_texformat_l8, _mesa_texformat_i8.
 */
static GLboolean
_mesa_texstore_a8(TEXSTORE_PARAMS)
{
   const GLuint texelBytes = _mesa_get_format_bytes(dstFormat);
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);

   ASSERT(dstFormat == MESA_FORMAT_A8 ||
          dstFormat == MESA_FORMAT_L8 ||
          dstFormat == MESA_FORMAT_I8);
   ASSERT(texelBytes == 1);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == srcFormat &&
       srcType == GL_UNSIGNED_BYTE) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else if (!ctx->_ImageTransferState &&
	    srcType == GL_UNSIGNED_BYTE &&
	    can_swizzle(baseInternalFormat) &&
	    can_swizzle(srcFormat)) {

      GLubyte dstmap[4];

      /* dstmap - how to swizzle from RGBA to dst format:
       */
      if (dstFormat == MESA_FORMAT_A8) {
	 dstmap[0] = 3;
      }
      else {
	 dstmap[0] = 0;
      }
      dstmap[1] = ZERO;		/* ? */
      dstmap[2] = ZERO;		/* ? */
      dstmap[3] = ONE;		/* ? */
      
      _mesa_swizzle_ubyte_image(ctx, dims,
				srcFormat,
				srcType,
				baseInternalFormat,
				dstmap, 1,
				dstAddr, dstXoffset, dstYoffset, dstZoffset,
				dstRowStride, dstImageOffsets,
				srcWidth, srcHeight, srcDepth, srcAddr,
				srcPacking);      
   }   
   else {
      /* general path */
      const GLchan *tempImage = _mesa_make_temp_chan_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLchan *src = tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * texelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * texelBytes;
         for (row = 0; row < srcHeight; row++) {
            for (col = 0; col < srcWidth; col++) {
               dstRow[col] = CHAN_TO_UBYTE(src[col]);
            }
            dstRow += dstRowStride;
            src += srcWidth;
         }
      }
      free((void *) tempImage);
   }
   return GL_TRUE;
}



static GLboolean
_mesa_texstore_ci8(TEXSTORE_PARAMS)
{
   const GLuint texelBytes = _mesa_get_format_bytes(dstFormat);

   (void) dims; (void) baseInternalFormat;
   ASSERT(dstFormat == MESA_FORMAT_CI8);
   ASSERT(texelBytes == 1);
   ASSERT(baseInternalFormat == GL_COLOR_INDEX);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       srcFormat == GL_COLOR_INDEX &&
       srcType == GL_UNSIGNED_BYTE) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      GLint img, row;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * texelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * texelBytes;
         for (row = 0; row < srcHeight; row++) {
            const GLvoid *src = _mesa_image_address(dims, srcPacking,
                srcAddr, srcWidth, srcHeight, srcFormat, srcType, img, row, 0);
            _mesa_unpack_index_span(ctx, srcWidth, GL_UNSIGNED_BYTE, dstRow,
                                    srcType, src, srcPacking,
                                    ctx->_ImageTransferState);
            dstRow += dstRowStride;
         }
      }
   }
   return GL_TRUE;
}


/**
 * Texstore for _mesa_texformat_ycbcr or _mesa_texformat_ycbcr_REV.
 */
static GLboolean
_mesa_texstore_ycbcr(TEXSTORE_PARAMS)
{
   const GLboolean littleEndian = _mesa_little_endian();
   const GLuint texelBytes = _mesa_get_format_bytes(dstFormat);

   (void) ctx; (void) dims; (void) baseInternalFormat;

   ASSERT((dstFormat == MESA_FORMAT_YCBCR) ||
          (dstFormat == MESA_FORMAT_YCBCR_REV));
   ASSERT(texelBytes == 2);
   ASSERT(ctx->Extensions.MESA_ycbcr_texture);
   ASSERT(srcFormat == GL_YCBCR_MESA);
   ASSERT((srcType == GL_UNSIGNED_SHORT_8_8_MESA) ||
          (srcType == GL_UNSIGNED_SHORT_8_8_REV_MESA));
   ASSERT(baseInternalFormat == GL_YCBCR_MESA);

   /* always just memcpy since no pixel transfer ops apply */
   memcpy_texture(ctx, dims,
                  dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                  dstRowStride,
                  dstImageOffsets,
                  srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                  srcAddr, srcPacking);

   /* Check if we need byte swapping */
   /* XXX the logic here _might_ be wrong */
   if (srcPacking->SwapBytes ^
       (srcType == GL_UNSIGNED_SHORT_8_8_REV_MESA) ^
       (dstFormat == MESA_FORMAT_YCBCR_REV) ^
       !littleEndian) {
      GLint img, row;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * texelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * texelBytes;
         for (row = 0; row < srcHeight; row++) {
            _mesa_swap2((GLushort *) dstRow, srcWidth);
            dstRow += dstRowStride;
         }
      }
   }
   return GL_TRUE;
}

static GLboolean
_mesa_texstore_dudv8(TEXSTORE_PARAMS)
{
   const GLboolean littleEndian = _mesa_little_endian();
   const GLuint texelBytes = _mesa_get_format_bytes(dstFormat);

   ASSERT(dstFormat == MESA_FORMAT_DUDV8);
   ASSERT(texelBytes == 2);
   ASSERT(ctx->Extensions.ATI_envmap_bumpmap);
   ASSERT((srcFormat == GL_DU8DV8_ATI) ||
	  (srcFormat == GL_DUDV_ATI));
   ASSERT(baseInternalFormat == GL_DUDV_ATI);

   if (!srcPacking->SwapBytes && srcType == GL_BYTE &&
       littleEndian) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else if (srcType == GL_BYTE) {

      GLubyte dstmap[4];

      /* dstmap - how to swizzle from RGBA to dst format:
       */
      if (littleEndian) {
	 dstmap[0] = 0;
	 dstmap[1] = 3;
      }
      else {
	 dstmap[0] = 3;
	 dstmap[1] = 0;
      }
      dstmap[2] = ZERO;		/* ? */
      dstmap[3] = ONE;		/* ? */
      
      _mesa_swizzle_ubyte_image(ctx, dims,
				GL_LUMINANCE_ALPHA, /* hack */
				GL_UNSIGNED_BYTE, /* hack */
				GL_LUMINANCE_ALPHA, /* hack */
				dstmap, 2,
				dstAddr, dstXoffset, dstYoffset, dstZoffset,
				dstRowStride, dstImageOffsets,
				srcWidth, srcHeight, srcDepth, srcAddr,
				srcPacking);      
   }   
   else {
      /* general path - note this is defined for 2d textures only */
      const GLint components = _mesa_components_in_format(baseInternalFormat);
      const GLint srcStride = _mesa_image_row_stride(srcPacking, srcWidth,
                                                     srcFormat, srcType);
      GLbyte *tempImage, *dst, *src;
      GLint row;

      tempImage = (GLbyte *) malloc(srcWidth * srcHeight * srcDepth
                                          * components * sizeof(GLbyte));
      if (!tempImage)
         return GL_FALSE;

      src = (GLbyte *) _mesa_image_address(dims, srcPacking, srcAddr,
                                           srcWidth, srcHeight,
                                           srcFormat, srcType,
                                           0, 0, 0);

      dst = tempImage;
      for (row = 0; row < srcHeight; row++) {
         _mesa_unpack_dudv_span_byte(ctx, srcWidth, baseInternalFormat,
                                     dst, srcFormat, srcType, src,
                                     srcPacking, 0);
         dst += srcWidth * components;
         src += srcStride;
      }
 
      src = tempImage;
      dst = (GLbyte *) dstAddr
            + dstYoffset * dstRowStride
            + dstXoffset * texelBytes;
      for (row = 0; row < srcHeight; row++) {
         memcpy(dst, src, srcWidth * texelBytes);
         dst += dstRowStride;
         src += srcWidth * texelBytes;
      }
      free((void *) tempImage);
   }
   return GL_TRUE;
}

/**
 * Store a texture in MESA_FORMAT_SIGNED_RGBA8888 or MESA_FORMAT_SIGNED_RGBA8888_REV
 */
static GLboolean
_mesa_texstore_signed_rgba8888(TEXSTORE_PARAMS)
{
   const GLboolean littleEndian = _mesa_little_endian();
   const GLuint texelBytes = _mesa_get_format_bytes(dstFormat);
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);

   ASSERT(dstFormat == MESA_FORMAT_SIGNED_RGBA8888 ||
          dstFormat == MESA_FORMAT_SIGNED_RGBA8888_REV);
   ASSERT(texelBytes == 4);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       dstFormat == MESA_FORMAT_SIGNED_RGBA8888 &&
       baseInternalFormat == GL_RGBA &&
      ((srcFormat == GL_RGBA && srcType == GL_BYTE && !littleEndian) ||
       (srcFormat == GL_ABGR_EXT && srcType == GL_BYTE && littleEndian))) {
       /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       dstFormat == MESA_FORMAT_SIGNED_RGBA8888_REV &&
       baseInternalFormat == GL_RGBA &&
      ((srcFormat == GL_RGBA && srcType == GL_BYTE && littleEndian) ||
       (srcFormat == GL_ABGR_EXT && srcType == GL_BYTE && !littleEndian))) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else if (!ctx->_ImageTransferState &&
	    (srcType == GL_BYTE) &&
	    can_swizzle(baseInternalFormat) &&
	    can_swizzle(srcFormat)) {

      GLubyte dstmap[4];

      /* dstmap - how to swizzle from RGBA to dst format:
       */
      if ((littleEndian && dstFormat == MESA_FORMAT_SIGNED_RGBA8888) ||
	  (!littleEndian && dstFormat == MESA_FORMAT_SIGNED_RGBA8888_REV)) {
	 dstmap[3] = 0;
	 dstmap[2] = 1;
	 dstmap[1] = 2;
	 dstmap[0] = 3;
      }
      else {
	 dstmap[3] = 3;
	 dstmap[2] = 2;
	 dstmap[1] = 1;
	 dstmap[0] = 0;
      }
      
      _mesa_swizzle_ubyte_image(ctx, dims,
				srcFormat,
				srcType,
				baseInternalFormat,
				dstmap, 4,
				dstAddr, dstXoffset, dstYoffset, dstZoffset,
				dstRowStride, dstImageOffsets,
				srcWidth, srcHeight, srcDepth, srcAddr,
				srcPacking);      
   }
   else {
      /* general path */
      const GLfloat *tempImage = make_temp_float_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLfloat *srcRow = tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * texelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * texelBytes;
         for (row = 0; row < srcHeight; row++) {
            GLuint *dstUI = (GLuint *) dstRow;
            if (dstFormat == MESA_FORMAT_SIGNED_RGBA8888) {
               for (col = 0; col < srcWidth; col++) {
                  dstUI[col] = PACK_COLOR_8888( FLOAT_TO_BYTE_TEX(srcRow[RCOMP]),
                                                FLOAT_TO_BYTE_TEX(srcRow[GCOMP]),
                                                FLOAT_TO_BYTE_TEX(srcRow[BCOMP]),
                                                FLOAT_TO_BYTE_TEX(srcRow[ACOMP]) );
                  srcRow += 4;
               }
            }
            else {
               for (col = 0; col < srcWidth; col++) {
                  dstUI[col] = PACK_COLOR_8888_REV( FLOAT_TO_BYTE_TEX(srcRow[RCOMP]),
                                                    FLOAT_TO_BYTE_TEX(srcRow[GCOMP]),
                                                    FLOAT_TO_BYTE_TEX(srcRow[BCOMP]),
                                                    FLOAT_TO_BYTE_TEX(srcRow[ACOMP]) );
                  srcRow += 4;
               }
            }
            dstRow += dstRowStride;
         }
      }
      free((void *) tempImage);
   }
   return GL_TRUE;
}

/**
 * Store a combined depth/stencil texture image.
 */
static GLboolean
_mesa_texstore_z24_s8(TEXSTORE_PARAMS)
{
   const GLuint depthScale = 0xffffff;
   const GLint srcRowStride
      = _mesa_image_row_stride(srcPacking, srcWidth, srcFormat, srcType)
      / sizeof(GLuint);
   GLint img, row;

   ASSERT(dstFormat == MESA_FORMAT_Z24_S8);
   ASSERT(srcFormat == GL_DEPTH_STENCIL_EXT || srcFormat == GL_DEPTH_COMPONENT);
   ASSERT(srcFormat != GL_DEPTH_STENCIL_EXT || srcType == GL_UNSIGNED_INT_24_8_EXT);

   /* In case we only upload depth we need to preserve the stencil */
   if (srcFormat == GL_DEPTH_COMPONENT) {
      for (img = 0; img < srcDepth; img++) {
         GLuint *dstRow = (GLuint *) dstAddr
            + dstImageOffsets[dstZoffset + img]
            + dstYoffset * dstRowStride / sizeof(GLuint)
            + dstXoffset;
         const GLuint *src
            = (const GLuint *) _mesa_image_address(dims, srcPacking, srcAddr,
                  srcWidth, srcHeight,
                  srcFormat, srcType,
                  img, 0, 0);
         for (row = 0; row < srcHeight; row++) {
            GLuint depth[MAX_WIDTH];
            GLint i;
            _mesa_unpack_depth_span(ctx, srcWidth,
                                    GL_UNSIGNED_INT, /* dst type */
                                    depth, /* dst addr */
                                    depthScale,
                                    srcType, src, srcPacking);

            for (i = 0; i < srcWidth; i++)
               dstRow[i] = depth[i] << 8 | (dstRow[i] & 0x000000FF);

            src += srcRowStride;
            dstRow += dstRowStride / sizeof(GLuint);
         }
      }
   }
   else if (ctx->Pixel.DepthScale == 1.0f &&
       ctx->Pixel.DepthBias == 0.0f &&
       !srcPacking->SwapBytes) {
      /* simple path */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLint srcRowStride
         = _mesa_image_row_stride(srcPacking, srcWidth, srcFormat, srcType)
         / sizeof(GLuint);
      GLint img, row;

      for (img = 0; img < srcDepth; img++) {
         GLuint *dstRow = (GLuint *) dstAddr
            + dstImageOffsets[dstZoffset + img]
            + dstYoffset * dstRowStride / sizeof(GLuint)
            + dstXoffset;
         const GLuint *src
            = (const GLuint *) _mesa_image_address(dims, srcPacking, srcAddr,
                                                   srcWidth, srcHeight,
                                                   srcFormat, srcType,
                                                   img, 0, 0);
         for (row = 0; row < srcHeight; row++) {
            GLubyte stencil[MAX_WIDTH];
            GLint i;
            /* the 24 depth bits will be in the high position: */
            _mesa_unpack_depth_span(ctx, srcWidth,
                                    GL_UNSIGNED_INT_24_8_EXT, /* dst type */
                                    dstRow, /* dst addr */
                                    depthScale,
                                    srcType, src, srcPacking);
            /* get the 8-bit stencil values */
            _mesa_unpack_stencil_span(ctx, srcWidth,
                                      GL_UNSIGNED_BYTE, /* dst type */
                                      stencil, /* dst addr */
                                      srcType, src, srcPacking,
                                      ctx->_ImageTransferState);
            /* merge stencil values into depth values */
            for (i = 0; i < srcWidth; i++)
               dstRow[i] |= stencil[i];

            src += srcRowStride;
            dstRow += dstRowStride / sizeof(GLuint);
         }
      }
   }
   return GL_TRUE;
}


/**
 * Store a combined depth/stencil texture image.
 */
static GLboolean
_mesa_texstore_s8_z24(TEXSTORE_PARAMS)
{
   const GLuint depthScale = 0xffffff;
   const GLint srcRowStride
      = _mesa_image_row_stride(srcPacking, srcWidth, srcFormat, srcType)
      / sizeof(GLuint);
   GLint img, row;

   ASSERT(dstFormat == MESA_FORMAT_S8_Z24);
   ASSERT(srcFormat == GL_DEPTH_STENCIL_EXT || srcFormat == GL_DEPTH_COMPONENT);
   ASSERT(srcFormat != GL_DEPTH_STENCIL_EXT || srcType == GL_UNSIGNED_INT_24_8_EXT);

   /* In case we only upload depth we need to preserve the stencil */
   if (srcFormat == GL_DEPTH_COMPONENT) {
      for (img = 0; img < srcDepth; img++) {
         GLuint *dstRow = (GLuint *) dstAddr
            + dstImageOffsets[dstZoffset + img]
            + dstYoffset * dstRowStride / sizeof(GLuint)
            + dstXoffset;
         const GLuint *src
            = (const GLuint *) _mesa_image_address(dims, srcPacking, srcAddr,
                  srcWidth, srcHeight,
                  srcFormat, srcType,
                  img, 0, 0);
         for (row = 0; row < srcHeight; row++) {
            GLuint depth[MAX_WIDTH];
            GLint i;
            _mesa_unpack_depth_span(ctx, srcWidth,
                                    GL_UNSIGNED_INT, /* dst type */
                                    depth, /* dst addr */
                                    depthScale,
                                    srcType, src, srcPacking);

            for (i = 0; i < srcWidth; i++)
               dstRow[i] = depth[i] | (dstRow[i] & 0xFF000000);

            src += srcRowStride;
            dstRow += dstRowStride / sizeof(GLuint);
         }
      }
   }
   else {
      for (img = 0; img < srcDepth; img++) {
         GLuint *dstRow = (GLuint *) dstAddr
            + dstImageOffsets[dstZoffset + img]
            + dstYoffset * dstRowStride / sizeof(GLuint)
            + dstXoffset;
         const GLuint *src
            = (const GLuint *) _mesa_image_address(dims, srcPacking, srcAddr,
                  srcWidth, srcHeight,
                  srcFormat, srcType,
                  img, 0, 0);
         for (row = 0; row < srcHeight; row++) {
            GLubyte stencil[MAX_WIDTH];
            GLint i;
            /* the 24 depth bits will be in the low position: */
            _mesa_unpack_depth_span(ctx, srcWidth,
                                    GL_UNSIGNED_INT, /* dst type */
                                    dstRow, /* dst addr */
                                    depthScale,
                                    srcType, src, srcPacking);
            /* get the 8-bit stencil values */
            _mesa_unpack_stencil_span(ctx, srcWidth,
                                      GL_UNSIGNED_BYTE, /* dst type */
                                      stencil, /* dst addr */
                                      srcType, src, srcPacking,
                                      ctx->_ImageTransferState);
            /* merge stencil values into depth values */
            for (i = 0; i < srcWidth; i++)
               dstRow[i] |= stencil[i] << 24;

            src += srcRowStride;
            dstRow += dstRowStride / sizeof(GLuint);
         }
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
static GLboolean
_mesa_texstore_rgba_float32(TEXSTORE_PARAMS)
{
   const GLuint texelBytes = _mesa_get_format_bytes(dstFormat);
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);
   const GLint components = _mesa_components_in_format(baseFormat);

   ASSERT(dstFormat == MESA_FORMAT_RGBA_FLOAT32 ||
          dstFormat == MESA_FORMAT_RGB_FLOAT32 ||
          dstFormat == MESA_FORMAT_ALPHA_FLOAT32 ||
          dstFormat == MESA_FORMAT_LUMINANCE_FLOAT32 ||
          dstFormat == MESA_FORMAT_LUMINANCE_ALPHA_FLOAT32 ||
          dstFormat == MESA_FORMAT_INTENSITY_FLOAT32);
   ASSERT(baseInternalFormat == GL_RGBA ||
          baseInternalFormat == GL_RGB ||
          baseInternalFormat == GL_ALPHA ||
          baseInternalFormat == GL_LUMINANCE ||
          baseInternalFormat == GL_LUMINANCE_ALPHA ||
          baseInternalFormat == GL_INTENSITY);
   ASSERT(texelBytes == components * sizeof(GLfloat));

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == srcFormat &&
       srcType == GL_FLOAT) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLfloat *tempImage = make_temp_float_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLfloat *srcRow = tempImage;
      GLint bytesPerRow;
      GLint img, row;
      if (!tempImage)
         return GL_FALSE;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      bytesPerRow = srcWidth * components * sizeof(GLfloat);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * texelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * texelBytes;
         for (row = 0; row < srcHeight; row++) {
            memcpy(dstRow, srcRow, bytesPerRow);
            dstRow += dstRowStride;
            srcRow += srcWidth * components;
         }
      }

      free((void *) tempImage);
   }
   return GL_TRUE;
}


/**
 * As above, but store 16-bit floats.
 */
static GLboolean
_mesa_texstore_rgba_float16(TEXSTORE_PARAMS)
{
   const GLuint texelBytes = _mesa_get_format_bytes(dstFormat);
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);
   const GLint components = _mesa_components_in_format(baseFormat);

   ASSERT(dstFormat == MESA_FORMAT_RGBA_FLOAT16 ||
          dstFormat == MESA_FORMAT_RGB_FLOAT16 ||
          dstFormat == MESA_FORMAT_ALPHA_FLOAT16 ||
          dstFormat == MESA_FORMAT_LUMINANCE_FLOAT16 ||
          dstFormat == MESA_FORMAT_LUMINANCE_ALPHA_FLOAT16 ||
          dstFormat == MESA_FORMAT_INTENSITY_FLOAT16);
   ASSERT(baseInternalFormat == GL_RGBA ||
          baseInternalFormat == GL_RGB ||
          baseInternalFormat == GL_ALPHA ||
          baseInternalFormat == GL_LUMINANCE ||
          baseInternalFormat == GL_LUMINANCE_ALPHA ||
          baseInternalFormat == GL_INTENSITY);
   ASSERT(texelBytes == components * sizeof(GLhalfARB));

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == srcFormat &&
       srcType == GL_HALF_FLOAT_ARB) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLfloat *tempImage = make_temp_float_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLfloat *src = tempImage;
      GLint img, row;
      if (!tempImage)
         return GL_FALSE;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * texelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * texelBytes;
         for (row = 0; row < srcHeight; row++) {
            GLhalfARB *dstTexel = (GLhalfARB *) dstRow;
            GLint i;
            for (i = 0; i < srcWidth * components; i++) {
               dstTexel[i] = _mesa_float_to_half(src[i]);
            }
            dstRow += dstRowStride;
            src += srcWidth * components;
         }
      }

      free((void *) tempImage);
   }
   return GL_TRUE;
}


#if FEATURE_EXT_texture_sRGB
static GLboolean
_mesa_texstore_srgb8(TEXSTORE_PARAMS)
{
   gl_format newDstFormat;
   GLboolean k;

   ASSERT(dstFormat == MESA_FORMAT_SRGB8);

   /* reuse normal rgb texstore code */
   newDstFormat = MESA_FORMAT_RGB888;

   k = _mesa_texstore_rgb888(ctx, dims, baseInternalFormat,
             newDstFormat, dstAddr,
             dstXoffset, dstYoffset, dstZoffset,
             dstRowStride, dstImageOffsets,
             srcWidth, srcHeight, srcDepth,
             srcFormat, srcType,
             srcAddr, srcPacking);
   return k;
}


static GLboolean
_mesa_texstore_srgba8(TEXSTORE_PARAMS)
{
   gl_format newDstFormat;
   GLboolean k;

   ASSERT(dstFormat == MESA_FORMAT_SRGBA8);

   /* reuse normal rgba texstore code */
   newDstFormat = MESA_FORMAT_RGBA8888;
   k = _mesa_texstore_rgba8888(ctx, dims, baseInternalFormat,
                               newDstFormat, dstAddr,
                               dstXoffset, dstYoffset, dstZoffset,
                               dstRowStride, dstImageOffsets,
                               srcWidth, srcHeight, srcDepth,
                               srcFormat, srcType,
                               srcAddr, srcPacking);
   return k;
}


static GLboolean
_mesa_texstore_sargb8(TEXSTORE_PARAMS)
{
   gl_format newDstFormat;
   GLboolean k;

   ASSERT(dstFormat == MESA_FORMAT_SARGB8);

   /* reuse normal rgba texstore code */
   newDstFormat = MESA_FORMAT_ARGB8888;

   k = _mesa_texstore_argb8888(ctx, dims, baseInternalFormat,
                               newDstFormat, dstAddr,
                               dstXoffset, dstYoffset, dstZoffset,
                               dstRowStride, dstImageOffsets,
                               srcWidth, srcHeight, srcDepth,
                               srcFormat, srcType,
                               srcAddr, srcPacking);
   return k;
}


static GLboolean
_mesa_texstore_sl8(TEXSTORE_PARAMS)
{
   gl_format newDstFormat;
   GLboolean k;

   ASSERT(dstFormat == MESA_FORMAT_SL8);

   newDstFormat = MESA_FORMAT_L8;

   /* _mesa_textore_a8 handles luminance8 too */
   k = _mesa_texstore_a8(ctx, dims, baseInternalFormat,
                         newDstFormat, dstAddr,
                         dstXoffset, dstYoffset, dstZoffset,
                         dstRowStride, dstImageOffsets,
                         srcWidth, srcHeight, srcDepth,
                         srcFormat, srcType,
                         srcAddr, srcPacking);
   return k;
}


static GLboolean
_mesa_texstore_sla8(TEXSTORE_PARAMS)
{
   gl_format newDstFormat;
   GLboolean k;

   ASSERT(dstFormat == MESA_FORMAT_SLA8);

   /* reuse normal luminance/alpha texstore code */
   newDstFormat = MESA_FORMAT_AL88;

   k = _mesa_texstore_al88(ctx, dims, baseInternalFormat,
                           newDstFormat, dstAddr,
                           dstXoffset, dstYoffset, dstZoffset,
                           dstRowStride, dstImageOffsets,
                           srcWidth, srcHeight, srcDepth,
                           srcFormat, srcType,
                           srcAddr, srcPacking);
   return k;
}

#else

/* these are used only in texstore_funcs[] below */
#define _mesa_texstore_srgb8 NULL
#define _mesa_texstore_srgba8 NULL
#define _mesa_texstore_sargb8 NULL
#define _mesa_texstore_sl8 NULL
#define _mesa_texstore_sla8 NULL

#endif /* FEATURE_EXT_texture_sRGB */




/**
 * Table mapping MESA_FORMAT_8 to _mesa_texstore_*()
 * XXX this is somewhat temporary.
 */
static const struct {
   gl_format Name;
   StoreTexImageFunc Store;
}
texstore_funcs[MESA_FORMAT_COUNT] =
{
   { MESA_FORMAT_NONE, NULL },
   { MESA_FORMAT_RGBA8888, _mesa_texstore_rgba8888 },
   { MESA_FORMAT_RGBA8888_REV, _mesa_texstore_rgba8888 },
   { MESA_FORMAT_ARGB8888, _mesa_texstore_argb8888 },
   { MESA_FORMAT_ARGB8888_REV, _mesa_texstore_argb8888 },
   { MESA_FORMAT_XRGB8888, _mesa_texstore_argb8888 },
   { MESA_FORMAT_XRGB8888_REV, _mesa_texstore_argb8888 },
   { MESA_FORMAT_RGB888, _mesa_texstore_rgb888 },
   { MESA_FORMAT_BGR888, _mesa_texstore_bgr888 },
   { MESA_FORMAT_RGB565, _mesa_texstore_rgb565 },
   { MESA_FORMAT_RGB565_REV, _mesa_texstore_rgb565 },
   { MESA_FORMAT_ARGB4444, _mesa_texstore_argb4444 },
   { MESA_FORMAT_ARGB4444_REV, _mesa_texstore_argb4444 },
   { MESA_FORMAT_RGBA5551, _mesa_texstore_rgba5551 },
   { MESA_FORMAT_ARGB1555, _mesa_texstore_argb1555 },
   { MESA_FORMAT_ARGB1555_REV, _mesa_texstore_argb1555 },
   { MESA_FORMAT_AL88, _mesa_texstore_al88 },
   { MESA_FORMAT_AL88_REV, _mesa_texstore_al88 },
   { MESA_FORMAT_AL1616, _mesa_texstore_al1616 },
   { MESA_FORMAT_AL1616_REV, _mesa_texstore_al1616 },
   { MESA_FORMAT_RGB332, _mesa_texstore_rgb332 },
   { MESA_FORMAT_A8, _mesa_texstore_a8 },
   { MESA_FORMAT_L8, _mesa_texstore_a8 },
   { MESA_FORMAT_I8, _mesa_texstore_a8 },
   { MESA_FORMAT_CI8, _mesa_texstore_ci8 },
   { MESA_FORMAT_YCBCR, _mesa_texstore_ycbcr },
   { MESA_FORMAT_YCBCR_REV, _mesa_texstore_ycbcr },
   { MESA_FORMAT_Z24_S8, _mesa_texstore_z24_s8 },
   { MESA_FORMAT_S8_Z24, _mesa_texstore_s8_z24 },
   { MESA_FORMAT_Z16, _mesa_texstore_z16 },
   { MESA_FORMAT_X8_Z24, _mesa_texstore_x8_z24 },
   { MESA_FORMAT_Z24_X8, _mesa_texstore_z24_x8 },
   { MESA_FORMAT_Z32, _mesa_texstore_z32 },
   { MESA_FORMAT_S8, NULL/*_mesa_texstore_s8*/ },
   { MESA_FORMAT_SRGB8, _mesa_texstore_srgb8 },
   { MESA_FORMAT_SRGBA8, _mesa_texstore_srgba8 },
   { MESA_FORMAT_SARGB8, _mesa_texstore_sargb8 },
   { MESA_FORMAT_SL8, _mesa_texstore_sl8 },
   { MESA_FORMAT_SLA8, _mesa_texstore_sla8 },
   { MESA_FORMAT_SRGB_DXT1, _mesa_texstore_rgb_dxt1 },
   { MESA_FORMAT_SRGBA_DXT1, _mesa_texstore_rgba_dxt1 },
   { MESA_FORMAT_SRGBA_DXT3, _mesa_texstore_rgba_dxt3 },
   { MESA_FORMAT_SRGBA_DXT5, _mesa_texstore_rgba_dxt5 },
   { MESA_FORMAT_RGB_FXT1, _mesa_texstore_rgb_fxt1 },
   { MESA_FORMAT_RGBA_FXT1, _mesa_texstore_rgba_fxt1 },
   { MESA_FORMAT_RGB_DXT1, _mesa_texstore_rgb_dxt1 },
   { MESA_FORMAT_RGBA_DXT1, _mesa_texstore_rgba_dxt1 },
   { MESA_FORMAT_RGBA_DXT3, _mesa_texstore_rgba_dxt3 },
   { MESA_FORMAT_RGBA_DXT5, _mesa_texstore_rgba_dxt5 },
   { MESA_FORMAT_RGBA_FLOAT32, _mesa_texstore_rgba_float32 },
   { MESA_FORMAT_RGBA_FLOAT16, _mesa_texstore_rgba_float16 },
   { MESA_FORMAT_RGB_FLOAT32, _mesa_texstore_rgba_float32 },
   { MESA_FORMAT_RGB_FLOAT16, _mesa_texstore_rgba_float16 },
   { MESA_FORMAT_ALPHA_FLOAT32, _mesa_texstore_rgba_float32 },
   { MESA_FORMAT_ALPHA_FLOAT16, _mesa_texstore_rgba_float16 },
   { MESA_FORMAT_LUMINANCE_FLOAT32, _mesa_texstore_rgba_float32 },
   { MESA_FORMAT_LUMINANCE_FLOAT16, _mesa_texstore_rgba_float16 },
   { MESA_FORMAT_LUMINANCE_ALPHA_FLOAT32, _mesa_texstore_rgba_float32 },
   { MESA_FORMAT_LUMINANCE_ALPHA_FLOAT16, _mesa_texstore_rgba_float16 },
   { MESA_FORMAT_INTENSITY_FLOAT32, _mesa_texstore_rgba_float32 },
   { MESA_FORMAT_INTENSITY_FLOAT16, _mesa_texstore_rgba_float16 },
   { MESA_FORMAT_DUDV8, _mesa_texstore_dudv8 },
   { MESA_FORMAT_SIGNED_RGBA8888, _mesa_texstore_signed_rgba8888 },
   { MESA_FORMAT_SIGNED_RGBA8888_REV, _mesa_texstore_signed_rgba8888 },
   { MESA_FORMAT_SIGNED_RGBA_16, NULL },
};


static GLboolean
_mesa_texstore_null(TEXSTORE_PARAMS)
{
   (void) ctx; (void) dims;
   (void) baseInternalFormat;
   (void) dstFormat;
   (void) dstAddr;
   (void) dstXoffset; (void) dstYoffset; (void) dstZoffset;
   (void) dstRowStride; (void) dstImageOffsets;
   (void) srcWidth; (void) srcHeight; (void) srcDepth;
   (void) srcFormat; (void) srcType;
   (void) srcAddr;
   (void) srcPacking;

   /* should never happen */
   _mesa_problem(NULL, "_mesa_texstore_null() is called");
   return GL_FALSE;
}


/**
 * Return the StoreTexImageFunc pointer to store an image in the given format.
 */
static StoreTexImageFunc
_mesa_get_texstore_func(gl_format format)
{
#ifdef DEBUG
   GLuint i;
   for (i = 0; i < MESA_FORMAT_COUNT; i++) {
      ASSERT(texstore_funcs[i].Name == i);
   }
#endif
   ASSERT(texstore_funcs[format].Name == format);

   if (texstore_funcs[format].Store)
      return texstore_funcs[format].Store;
   else
      return _mesa_texstore_null;
}


/**
 * Store user data into texture memory.
 * Called via glTex[Sub]Image1/2/3D()
 */
GLboolean
_mesa_texstore(TEXSTORE_PARAMS)
{
   StoreTexImageFunc storeImage;
   GLboolean success;

   storeImage = _mesa_get_texstore_func(dstFormat);

   success = storeImage(ctx, dims, baseInternalFormat,
                        dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                        dstRowStride, dstImageOffsets,
                        srcWidth, srcHeight, srcDepth,
                        srcFormat, srcType, srcAddr, srcPacking);
   return success;
}


/**
 * Check if an unpack PBO is active prior to fetching a texture image.
 * If so, do bounds checking and map the buffer into main memory.
 * Any errors detected will be recorded.
 * The caller _must_ call _mesa_unmap_teximage_pbo() too!
 */
const GLvoid *
_mesa_validate_pbo_teximage(GLcontext *ctx, GLuint dimensions,
			    GLsizei width, GLsizei height, GLsizei depth,
			    GLenum format, GLenum type, const GLvoid *pixels,
			    const struct gl_pixelstore_attrib *unpack,
			    const char *funcName)
{
   GLubyte *buf;

   if (!_mesa_is_bufferobj(unpack->BufferObj)) {
      /* no PBO */
      return pixels;
   }
   if (!_mesa_validate_pbo_access(dimensions, unpack, width, height, depth,
                                  format, type, pixels)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, funcName, "(invalid PBO access");
      return NULL;
   }

   buf = (GLubyte *) ctx->Driver.MapBuffer(ctx, GL_PIXEL_UNPACK_BUFFER_EXT,
                                          GL_READ_ONLY_ARB, unpack->BufferObj);
   if (!buf) {
      _mesa_error(ctx, GL_INVALID_OPERATION, funcName, "(PBO is mapped");
      return NULL;
   }

   return ADD_POINTERS(buf, pixels);
}


/**
 * Check if an unpack PBO is active prior to fetching a compressed texture
 * image.
 * If so, do bounds checking and map the buffer into main memory.
 * Any errors detected will be recorded.
 * The caller _must_ call _mesa_unmap_teximage_pbo() too!
 */
const GLvoid *
_mesa_validate_pbo_compressed_teximage(GLcontext *ctx,
                                 GLsizei imageSize, const GLvoid *pixels,
                                 const struct gl_pixelstore_attrib *packing,
                                 const char *funcName)
{
   GLubyte *buf;

   if (!_mesa_is_bufferobj(packing->BufferObj)) {
      /* not using a PBO - return pointer unchanged */
      return pixels;
   }
   if ((const GLubyte *) pixels + imageSize >
       ((const GLubyte *) 0) + packing->BufferObj->Size) {
      /* out of bounds read! */
      _mesa_error(ctx, GL_INVALID_OPERATION, funcName, "(invalid PBO access");
      return NULL;
   }

   buf = (GLubyte*) ctx->Driver.MapBuffer(ctx, GL_PIXEL_UNPACK_BUFFER_EXT,
                                         GL_READ_ONLY_ARB, packing->BufferObj);
   if (!buf) {
      _mesa_error(ctx, GL_INVALID_OPERATION, funcName, "(PBO is mapped");
      return NULL;
   }

   return ADD_POINTERS(buf, pixels);
}


/**
 * This function must be called after either of the validate_pbo_*_teximage()
 * functions.  It unmaps the PBO buffer if it was mapped earlier.
 */
void
_mesa_unmap_teximage_pbo(GLcontext *ctx,
                         const struct gl_pixelstore_attrib *unpack)
{
   if (_mesa_is_bufferobj(unpack->BufferObj)) {
      ctx->Driver.UnmapBuffer(ctx, GL_PIXEL_UNPACK_BUFFER_EXT,
                              unpack->BufferObj);
   }
}


/** Return texture size in bytes */
static GLuint
texture_size(const struct gl_texture_image *texImage)
{
   GLuint sz = _mesa_format_image_size(texImage->TexFormat, texImage->Width,
                                       texImage->Height, texImage->Depth);
   return sz;
}


/** Return row stride in bytes */
static GLuint
texture_row_stride(const struct gl_texture_image *texImage)
{
   GLuint stride = _mesa_format_row_stride(texImage->TexFormat,
                                           texImage->Width);
   return stride;
}



/**
 * This is the software fallback for Driver.TexImage1D()
 * and Driver.CopyTexImage1D().
 * \sa _mesa_store_teximage2d()
 * Note that the width may not be the actual texture width since it may
 * be changed by convolution w/ GL_REDUCE.  The texImage->Width field will
 * have the actual texture size.
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
   GLuint sizeInBytes;
   (void) border;

   /* allocate memory */
   sizeInBytes = texture_size(texImage);
   texImage->Data = _mesa_alloc_texmemory(sizeInBytes);
   if (!texImage->Data) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage1D");
      return;
   }

   pixels = _mesa_validate_pbo_teximage(ctx, 1, width, 1, 1, format, type,
                                        pixels, packing, "glTexImage1D");
   if (!pixels) {
      /* Note: we check for a NULL image pointer here, _after_ we allocated
       * memory for the texture.  That's what the GL spec calls for.
       */
      return;
   }
   else {
      const GLint dstRowStride = 0;
      GLboolean success = _mesa_texstore(ctx, 1, texImage->_BaseFormat,
                                         texImage->TexFormat,
                                         texImage->Data,
                                         0, 0, 0,  /* dstX/Y/Zoffset */
                                         dstRowStride,
                                         texImage->ImageOffsets,
                                         width, 1, 1,
                                         format, type, pixels, packing);
      if (!success) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage1D");
      }
   }

   _mesa_unmap_teximage_pbo(ctx, packing);
}


/**
 * This is the software fallback for Driver.TexImage2D()
 * and Driver.CopyTexImage2D().
 *
 * This function is oriented toward storing images in main memory, rather
 * than VRAM.  Device driver's can easily plug in their own replacement.
 *
 * Note: width and height may be pre-convolved dimensions, but
 * texImage->Width and texImage->Height will be post-convolved dimensions.
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
   GLuint sizeInBytes;
   (void) border;

   /* allocate memory */
   sizeInBytes = texture_size(texImage);
   texImage->Data = _mesa_alloc_texmemory(sizeInBytes);
   if (!texImage->Data) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage2D");
      return;
   }

   pixels = _mesa_validate_pbo_teximage(ctx, 2, width, height, 1, format, type,
                                        pixels, packing, "glTexImage2D");
   if (!pixels) {
      /* Note: we check for a NULL image pointer here, _after_ we allocated
       * memory for the texture.  That's what the GL spec calls for.
       */
      return;
   }
   else {
      GLint dstRowStride = texture_row_stride(texImage);
      GLboolean success = _mesa_texstore(ctx, 2, texImage->_BaseFormat,
                                         texImage->TexFormat,
                                         texImage->Data,
                                         0, 0, 0,  /* dstX/Y/Zoffset */
                                         dstRowStride,
                                         texImage->ImageOffsets,
                                         width, height, 1,
                                         format, type, pixels, packing);
      if (!success) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage2D");
      }
   }

   _mesa_unmap_teximage_pbo(ctx, packing);
}



/**
 * This is the software fallback for Driver.TexImage3D()
 * and Driver.CopyTexImage3D().
 * \sa _mesa_store_teximage2d()
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
   GLuint sizeInBytes;
   (void) border;

   /* allocate memory */
   sizeInBytes = texture_size(texImage);
   texImage->Data = _mesa_alloc_texmemory(sizeInBytes);
   if (!texImage->Data) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage3D");
      return;
   }

   pixels = _mesa_validate_pbo_teximage(ctx, 3, width, height, depth, format,
                                        type, pixels, packing, "glTexImage3D");
   if (!pixels) {
      /* Note: we check for a NULL image pointer here, _after_ we allocated
       * memory for the texture.  That's what the GL spec calls for.
       */
      return;
   }
   else {
      GLint dstRowStride = texture_row_stride(texImage);
      GLboolean success = _mesa_texstore(ctx, 3, texImage->_BaseFormat,
                                         texImage->TexFormat,
                                         texImage->Data,
                                         0, 0, 0,  /* dstX/Y/Zoffset */
                                         dstRowStride,
                                         texImage->ImageOffsets,
                                         width, height, depth,
                                         format, type, pixels, packing);
      if (!success) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage3D");
      }
   }

   _mesa_unmap_teximage_pbo(ctx, packing);
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
   /* get pointer to src pixels (may be in a pbo which we'll map here) */
   pixels = _mesa_validate_pbo_teximage(ctx, 1, width, 1, 1, format, type,
                                        pixels, packing, "glTexSubImage1D");
   if (!pixels)
      return;

   {
      const GLint dstRowStride = 0;
      GLboolean success = _mesa_texstore(ctx, 1, texImage->_BaseFormat,
                                         texImage->TexFormat,
                                         texImage->Data,
                                         xoffset, 0, 0,  /* offsets */
                                         dstRowStride,
                                         texImage->ImageOffsets,
                                         width, 1, 1,
                                         format, type, pixels, packing);
      if (!success) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexSubImage1D");
      }
   }

   _mesa_unmap_teximage_pbo(ctx, packing);
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
   /* get pointer to src pixels (may be in a pbo which we'll map here) */
   pixels = _mesa_validate_pbo_teximage(ctx, 2, width, height, 1, format, type,
                                        pixels, packing, "glTexSubImage2D");
   if (!pixels)
      return;

   {
      GLint dstRowStride = texture_row_stride(texImage);
      GLboolean success = _mesa_texstore(ctx, 2, texImage->_BaseFormat,
                                         texImage->TexFormat,
                                         texImage->Data,
                                         xoffset, yoffset, 0,
                                         dstRowStride,
                                         texImage->ImageOffsets,
                                         width, height, 1,
                                         format, type, pixels, packing);
      if (!success) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexSubImage2D");
      }
   }

   _mesa_unmap_teximage_pbo(ctx, packing);
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
   /* get pointer to src pixels (may be in a pbo which we'll map here) */
   pixels = _mesa_validate_pbo_teximage(ctx, 3, width, height, depth, format,
                                        type, pixels, packing,
                                        "glTexSubImage3D");
   if (!pixels)
      return;

   {
      GLint dstRowStride = texture_row_stride(texImage);
      GLboolean success = _mesa_texstore(ctx, 3, texImage->_BaseFormat,
                                         texImage->TexFormat,
                                         texImage->Data,
                                         xoffset, yoffset, zoffset,
                                         dstRowStride,
                                         texImage->ImageOffsets,
                                         width, height, depth,
                                         format, type, pixels, packing);
      if (!success) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexSubImage3D");
      }
   }

   _mesa_unmap_teximage_pbo(ctx, packing);
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
   (void) ctx;
   (void) target; (void) level;
   (void) internalFormat;
   (void) width; (void) border;
   (void) imageSize; (void) data;
   (void) texObj;
   (void) texImage;
}



/**
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
   (void) width; (void) height; (void) border;

   /* This is pretty simple, basically just do a memcpy without worrying
    * about the usual image unpacking or image transfer operations.
    */
   ASSERT(texObj);
   ASSERT(texImage);
   ASSERT(texImage->Width > 0);
   ASSERT(texImage->Height > 0);
   ASSERT(texImage->Depth == 1);
   ASSERT(texImage->Data == NULL); /* was freed in glCompressedTexImage2DARB */

   /* allocate storage */
   texImage->Data = _mesa_alloc_texmemory(imageSize);
   if (!texImage->Data) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCompressedTexImage2DARB");
      return;
   }

   data = _mesa_validate_pbo_compressed_teximage(ctx, imageSize, data,
                                                 &ctx->Unpack,
                                                 "glCompressedTexImage2D");
   if (!data)
      return;

   /* copy the data */
   memcpy(texImage->Data, data, imageSize);

   _mesa_unmap_teximage_pbo(ctx, &ctx->Unpack);
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
   (void) ctx;
   (void) target; (void) level;
   (void) internalFormat;
   (void) width; (void) height; (void) depth;
   (void) border;
   (void) imageSize; (void) data;
   (void) texObj;
   (void) texImage;
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
   /* there are no compressed 1D texture formats yet */
   (void) ctx;
   (void) target; (void) level;
   (void) xoffset; (void) width;
   (void) format;
   (void) imageSize; (void) data;
   (void) texObj;
   (void) texImage;
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
   const gl_format texFormat = texImage->TexFormat;
   const GLint destWidth = texImage->Width;
   GLuint bw, bh;

   _mesa_get_format_block_size(texFormat, &bw, &bh);

   (void) level;
   (void) format;

   /* these should have been caught sooner */
   ASSERT((width % bw) == 0 || width == 2 || width == 1);
   ASSERT((height % bh) == 0 || height == 2 || height == 1);
   ASSERT((xoffset % bw) == 0);
   ASSERT((yoffset % bh) == 0);

   /* get pointer to src pixels (may be in a pbo which we'll map here) */
   data = _mesa_validate_pbo_compressed_teximage(ctx, imageSize, data,
                                                 &ctx->Unpack,
                                                 "glCompressedTexSubImage2D");
   if (!data)
      return;

   srcRowStride = _mesa_format_row_stride(texFormat, width);
   src = (const GLubyte *) data;

   destRowStride = _mesa_format_row_stride(texFormat, destWidth);
   dest = _mesa_compressed_image_address(xoffset, yoffset, 0,
                                         texFormat, destWidth,
                                         (GLubyte *) texImage->Data);

   bytesPerRow = srcRowStride;  /* bytes per row of blocks */
   rows = height / bh;  /* rows in blocks */

   /* copy rows of blocks */
   for (i = 0; i < rows; i++) {
      memcpy(dest, src, bytesPerRow);
      dest += destRowStride;
      src += srcRowStride;
   }

   _mesa_unmap_teximage_pbo(ctx, &ctx->Unpack);
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
   /* there are no compressed 3D texture formats yet */
   (void) ctx;
   (void) target; (void) level;
   (void) xoffset; (void) yoffset; (void) zoffset;
   (void) width; (void) height; (void) depth;
   (void) format;
   (void) imageSize; (void) data;
   (void) texObj;
   (void) texImage;
}

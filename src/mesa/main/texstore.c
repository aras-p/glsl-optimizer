/*
 * Mesa 3-D graphics library
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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
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
 *   ctx->Driver.TexImage = _mesa_store_teximage;
 *   ctx->Driver.TexSubImage = _mesa_store_texsubimage;
 *   etc...
 *
 * Texture image processing is actually kind of complicated.  We have to do:
 *    Format/type conversions
 *    pixel unpacking
 *    pixel transfer (scale, bais, lookup, etc)
 *
 * These functions can handle most everything, including processing full
 * images and sub-images.
 */


#include "glheader.h"
#include "bufferobj.h"
#include "colormac.h"
#include "format_pack.h"
#include "format_utils.h"
#include "image.h"
#include "macros.h"
#include "mipmap.h"
#include "mtypes.h"
#include "pack.h"
#include "pbo.h"
#include "imports.h"
#include "texcompress.h"
#include "texcompress_fxt1.h"
#include "texcompress_rgtc.h"
#include "texcompress_s3tc.h"
#include "texcompress_etc.h"
#include "texcompress_bptc.h"
#include "teximage.h"
#include "texstore.h"
#include "enums.h"
#include "glformats.h"
#include "../../gallium/auxiliary/util/u_format_rgb9e5.h"
#include "../../gallium/auxiliary/util/u_format_r11g11b10f.h"


enum {
   ZERO = 4, 
   ONE = 5
};


/**
 * Texture image storage function.
 */
typedef GLboolean (*StoreTexImageFunc)(TEXSTORE_PARAMS);


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
   IDX_RG,
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

   {
      IDX_RG,
      MAP4(0, 1, ZERO, ONE),
      MAP2(0, 1)
   },
};



/**
 * Convert a GL image format enum to an IDX_* value (see above).
 */
static int
get_map_idx(GLenum value)
{
   switch (value) {
   case GL_LUMINANCE:
   case GL_LUMINANCE_INTEGER_EXT:
      return IDX_LUMINANCE;
   case GL_ALPHA:
   case GL_ALPHA_INTEGER:
      return IDX_ALPHA;
   case GL_INTENSITY:
      return IDX_INTENSITY;
   case GL_LUMINANCE_ALPHA:
   case GL_LUMINANCE_ALPHA_INTEGER_EXT:
      return IDX_LUMINANCE_ALPHA;
   case GL_RGB:
   case GL_RGB_INTEGER:
      return IDX_RGB;
   case GL_RGBA:
   case GL_RGBA_INTEGER:
      return IDX_RGBA;
   case GL_RED:
   case GL_RED_INTEGER:
      return IDX_RED;
   case GL_GREEN:
      return IDX_GREEN;
   case GL_BLUE:
      return IDX_BLUE;
   case GL_BGR:
   case GL_BGR_INTEGER:
      return IDX_BGR;
   case GL_BGRA:
   case GL_BGRA_INTEGER:
      return IDX_BGRA;
   case GL_ABGR_EXT:
      return IDX_ABGR;
   case GL_RG:
   case GL_RG_INTEGER:
      return IDX_RG;
   default:
      _mesa_problem(NULL, "Unexpected inFormat %s",
                    _mesa_lookup_enum_by_nr(value));
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
 * but the graphics hardware doesn't support luminance textures.  So, we might
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
GLfloat *
_mesa_make_temp_float_image(struct gl_context *ctx, GLuint dims,
			    GLenum logicalBaseFormat,
			    GLenum textureBaseFormat,
			    GLint srcWidth, GLint srcHeight, GLint srcDepth,
			    GLenum srcFormat, GLenum srcType,
			    const GLvoid *srcAddr,
			    const struct gl_pixelstore_attrib *srcPacking,
			    GLbitfield transferOps)
{
   GLfloat *tempImage;
   const GLint components = _mesa_components_in_format(logicalBaseFormat);
   const GLint srcStride =
      _mesa_image_row_stride(srcPacking, srcWidth, srcFormat, srcType);
   GLfloat *dst;
   GLint img, row;

   ASSERT(dims >= 1 && dims <= 3);

   ASSERT(logicalBaseFormat == GL_RGBA ||
          logicalBaseFormat == GL_RGB ||
          logicalBaseFormat == GL_RG ||
          logicalBaseFormat == GL_RED ||
          logicalBaseFormat == GL_LUMINANCE_ALPHA ||
          logicalBaseFormat == GL_LUMINANCE ||
          logicalBaseFormat == GL_ALPHA ||
          logicalBaseFormat == GL_INTENSITY ||
          logicalBaseFormat == GL_DEPTH_COMPONENT);

   ASSERT(textureBaseFormat == GL_RGBA ||
          textureBaseFormat == GL_RGB ||
          textureBaseFormat == GL_RG ||
          textureBaseFormat == GL_RED ||
          textureBaseFormat == GL_LUMINANCE_ALPHA ||
          textureBaseFormat == GL_LUMINANCE ||
          textureBaseFormat == GL_ALPHA ||
          textureBaseFormat == GL_INTENSITY ||
          textureBaseFormat == GL_DEPTH_COMPONENT);

   tempImage = malloc(srcWidth * srcHeight * srcDepth
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

      newImage = malloc(srcWidth * srcHeight * srcDepth
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
 * Make temporary image with uint pixel values.  Used for unsigned
 * integer-valued textures.
 */
static GLuint *
make_temp_uint_image(struct gl_context *ctx, GLuint dims,
                     GLenum logicalBaseFormat,
                     GLenum textureBaseFormat,
                     GLint srcWidth, GLint srcHeight, GLint srcDepth,
                     GLenum srcFormat, GLenum srcType,
                     const GLvoid *srcAddr,
                     const struct gl_pixelstore_attrib *srcPacking)
{
   GLuint *tempImage;
   const GLint components = _mesa_components_in_format(logicalBaseFormat);
   const GLint srcStride =
      _mesa_image_row_stride(srcPacking, srcWidth, srcFormat, srcType);
   GLuint *dst;
   GLint img, row;

   ASSERT(dims >= 1 && dims <= 3);

   ASSERT(logicalBaseFormat == GL_RGBA ||
          logicalBaseFormat == GL_RGB ||
          logicalBaseFormat == GL_RG ||
          logicalBaseFormat == GL_RED ||
          logicalBaseFormat == GL_LUMINANCE_ALPHA ||
          logicalBaseFormat == GL_LUMINANCE ||
          logicalBaseFormat == GL_INTENSITY ||
          logicalBaseFormat == GL_ALPHA);

   ASSERT(textureBaseFormat == GL_RGBA ||
          textureBaseFormat == GL_RGB ||
          textureBaseFormat == GL_RG ||
          textureBaseFormat == GL_RED ||
          textureBaseFormat == GL_LUMINANCE_ALPHA ||
          textureBaseFormat == GL_LUMINANCE ||
          textureBaseFormat == GL_INTENSITY ||
          textureBaseFormat == GL_ALPHA);

   tempImage = malloc(srcWidth * srcHeight * srcDepth
                                 * components * sizeof(GLuint));
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
	 _mesa_unpack_color_span_uint(ctx, srcWidth, logicalBaseFormat,
                                      dst, srcFormat, srcType, src,
                                      srcPacking);
	 dst += srcWidth * components;
	 src += srcStride;
      }
   }

   if (logicalBaseFormat != textureBaseFormat) {
      /* more work */
      GLint texComponents = _mesa_components_in_format(textureBaseFormat);
      GLint logComponents = _mesa_components_in_format(logicalBaseFormat);
      GLuint *newImage;
      GLint i, n;
      GLubyte map[6];

      /* we only promote up to RGB, RGBA and LUMINANCE_ALPHA formats for now */
      ASSERT(textureBaseFormat == GL_RGB || textureBaseFormat == GL_RGBA ||
             textureBaseFormat == GL_LUMINANCE_ALPHA);

      /* The actual texture format should have at least as many components
       * as the logical texture format.
       */
      ASSERT(texComponents >= logComponents);

      newImage = malloc(srcWidth * srcHeight * srcDepth
                                   * texComponents * sizeof(GLuint));
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
               newImage[i * texComponents + k] = 1;
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
 * Make a temporary (color) texture image with GLubyte components.
 * Apply all needed pixel unpacking and pixel transfer operations.
 * Note that there are both logicalBaseFormat and textureBaseFormat parameters.
 * Suppose the user specifies GL_LUMINANCE as the internal texture format
 * but the graphics hardware doesn't support luminance textures.  So, we might
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
 * \return resulting image with format = textureBaseFormat and type = GLubyte.
 */
GLubyte *
_mesa_make_temp_ubyte_image(struct gl_context *ctx, GLuint dims,
                            GLenum logicalBaseFormat,
                            GLenum textureBaseFormat,
                            GLint srcWidth, GLint srcHeight, GLint srcDepth,
                            GLenum srcFormat, GLenum srcType,
                            const GLvoid *srcAddr,
                            const struct gl_pixelstore_attrib *srcPacking)
{
   GLuint transferOps = ctx->_ImageTransferState;
   const GLint components = _mesa_components_in_format(logicalBaseFormat);
   GLint img, row;
   GLubyte *tempImage, *dst;

   ASSERT(dims >= 1 && dims <= 3);

   ASSERT(logicalBaseFormat == GL_RGBA ||
          logicalBaseFormat == GL_RGB ||
          logicalBaseFormat == GL_RG ||
          logicalBaseFormat == GL_RED ||
          logicalBaseFormat == GL_LUMINANCE_ALPHA ||
          logicalBaseFormat == GL_LUMINANCE ||
          logicalBaseFormat == GL_ALPHA ||
          logicalBaseFormat == GL_INTENSITY);

   ASSERT(textureBaseFormat == GL_RGBA ||
          textureBaseFormat == GL_RGB ||
          textureBaseFormat == GL_RG ||
          textureBaseFormat == GL_RED ||
          textureBaseFormat == GL_LUMINANCE_ALPHA ||
          textureBaseFormat == GL_LUMINANCE ||
          textureBaseFormat == GL_ALPHA ||
          textureBaseFormat == GL_INTENSITY);

   /* unpack and transfer the source image */
   tempImage = malloc(srcWidth * srcHeight * srcDepth
                                       * components * sizeof(GLubyte));
   if (!tempImage) {
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
         _mesa_unpack_color_span_ubyte(ctx, srcWidth, logicalBaseFormat, dst,
                                       srcFormat, srcType, src, srcPacking,
                                       transferOps);
         dst += srcWidth * components;
         src += srcStride;
      }
   }

   if (logicalBaseFormat != textureBaseFormat) {
      /* one more conversion step */
      GLint texComponents = _mesa_components_in_format(textureBaseFormat);
      GLint logComponents = _mesa_components_in_format(logicalBaseFormat);
      GLubyte *newImage;
      GLint i, n;
      GLubyte map[6];

      /* we only promote up to RGB, RGBA and LUMINANCE_ALPHA formats for now */
      ASSERT(textureBaseFormat == GL_RGB || textureBaseFormat == GL_RGBA ||
             textureBaseFormat == GL_LUMINANCE_ALPHA);

      /* The actual texture format should have at least as many components
       * as the logical texture format.
       */
      ASSERT(texComponents >= logComponents);

      newImage = malloc(srcWidth * srcHeight * srcDepth
                                         * texComponents * sizeof(GLubyte));
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
               newImage[i * texComponents + k] = 255;
            else
               newImage[i * texComponents + k] = tempImage[i * logComponents + j];
         }
      }

      free(tempImage);
      tempImage = newImage;
   }

   return tempImage;
}


static const GLubyte map_identity[6] = { 0, 1, 2, 3, ZERO, ONE };
static const GLubyte map_3210[6] = { 3, 2, 1, 0, ZERO, ONE };
static const GLubyte map_1032[6] = { 1, 0, 3, 2, ZERO, ONE };


/**
 * Teximage storage routine for when a simple memcpy will do.
 * No pixel transfer operations or special texel encodings allowed.
 * 1D, 2D and 3D images supported.
 */
static void
memcpy_texture(struct gl_context *ctx,
	       GLuint dimensions,
               mesa_format dstFormat,
               GLint dstRowStride,
               GLubyte **dstSlices,
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

   if (dstRowStride == srcRowStride &&
       dstRowStride == bytesPerRow) {
      /* memcpy image by image */
      GLint img;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstImage = dstSlices[img];
         memcpy(dstImage, srcImage, bytesPerRow * srcHeight);
         srcImage += srcImageStride;
      }
   }
   else {
      /* memcpy row by row */
      GLint img, row;
      for (img = 0; img < srcDepth; img++) {
         const GLubyte *srcRow = srcImage;
         GLubyte *dstRow = dstSlices[img];
         for (row = 0; row < srcHeight; row++) {
            memcpy(dstRow, srcRow, bytesPerRow);
            dstRow += dstRowStride;
            srcRow += srcRowStride;
         }
         srcImage += srcImageStride;
      }
   }
}


/**
 * General-case function for storing a color texture images with
 * components that can be represented with ubytes.  Example destination
 * texture formats are MESA_FORMAT_ARGB888, ARGB4444, RGB565.
 */
static GLboolean
store_ubyte_texture(TEXSTORE_PARAMS)
{
   const GLint srcRowStride = srcWidth * 4 * sizeof(GLubyte);
   GLubyte *tempImage, *src;
   GLint img;

   tempImage = _mesa_make_temp_ubyte_image(ctx, dims,
                                           baseInternalFormat,
                                           GL_RGBA,
                                           srcWidth, srcHeight, srcDepth,
                                           srcFormat, srcType, srcAddr,
                                           srcPacking);
   if (!tempImage)
      return GL_FALSE;

   /* This way we will use the RGB versions of the packing functions and it
    * will work for both RGB and sRGB textures*/
   dstFormat = _mesa_get_srgb_format_linear(dstFormat);

   src = tempImage;
   for (img = 0; img < srcDepth; img++) {
      _mesa_pack_ubyte_rgba_rect(dstFormat, srcWidth, srcHeight,
                                 src, srcRowStride,
                                 dstSlices[img], dstRowStride);
      src += srcHeight * srcRowStride;
   }
   free(tempImage);

   return GL_TRUE;
}




/**
 * Store a 32-bit integer or float depth component texture image.
 */
static GLboolean
_mesa_texstore_z32(TEXSTORE_PARAMS)
{
   const GLuint depthScale = 0xffffffff;
   GLenum dstType;
   (void) dims;
   ASSERT(dstFormat == MESA_FORMAT_Z_UNORM32 ||
          dstFormat == MESA_FORMAT_Z_FLOAT32);
   ASSERT(_mesa_get_format_bytes(dstFormat) == sizeof(GLuint));

   if (dstFormat == MESA_FORMAT_Z_UNORM32)
      dstType = GL_UNSIGNED_INT;
   else
      dstType = GL_FLOAT;

   {
      /* general path */
      GLint img, row;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];
         for (row = 0; row < srcHeight; row++) {
            const GLvoid *src = _mesa_image_address(dims, srcPacking,
                srcAddr, srcWidth, srcHeight, srcFormat, srcType, img, row, 0);
            _mesa_unpack_depth_span(ctx, srcWidth,
                                    dstType, dstRow,
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

   (void) dims;
   ASSERT(dstFormat == MESA_FORMAT_Z24_UNORM_X8_UINT);

   {
      /* general path */
      GLint img, row;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];
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

   (void) dims;
   ASSERT(dstFormat == MESA_FORMAT_X8_UINT_Z24_UNORM);

   {
      /* general path */
      GLint img, row;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];
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
   (void) dims;
   ASSERT(dstFormat == MESA_FORMAT_Z_UNORM16);
   ASSERT(_mesa_get_format_bytes(dstFormat) == sizeof(GLushort));

   {
      /* general path */
      GLint img, row;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];
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
   ASSERT(dstFormat == MESA_FORMAT_B5G6R5_UNORM ||
          dstFormat == MESA_FORMAT_R5G6B5_UNORM);
   ASSERT(_mesa_get_format_bytes(dstFormat) == 2);

   if (!ctx->_ImageTransferState &&
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
      GLubyte *dst = dstSlices[0];
      GLint row, col;
      for (row = 0; row < srcHeight; row++) {
         const GLubyte *srcUB = (const GLubyte *) src;
         GLushort *dstUS = (GLushort *) dst;
         /* check for byteswapped format */
         if (dstFormat == MESA_FORMAT_B5G6R5_UNORM) {
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
      return GL_TRUE;
   } else {
      return GL_FALSE;
   }
}


/**
 * Texstore for _mesa_texformat_ycbcr or _mesa_texformat_ycbcr_REV.
 */
static GLboolean
_mesa_texstore_ycbcr(TEXSTORE_PARAMS)
{
   const GLboolean littleEndian = _mesa_little_endian();

   (void) ctx; (void) dims; (void) baseInternalFormat;

   ASSERT((dstFormat == MESA_FORMAT_YCBCR) ||
          (dstFormat == MESA_FORMAT_YCBCR_REV));
   ASSERT(_mesa_get_format_bytes(dstFormat) == 2);
   ASSERT(ctx->Extensions.MESA_ycbcr_texture);
   ASSERT(srcFormat == GL_YCBCR_MESA);
   ASSERT((srcType == GL_UNSIGNED_SHORT_8_8_MESA) ||
          (srcType == GL_UNSIGNED_SHORT_8_8_REV_MESA));
   ASSERT(baseInternalFormat == GL_YCBCR_MESA);

   /* always just memcpy since no pixel transfer ops apply */
   memcpy_texture(ctx, dims,
                  dstFormat,
                  dstRowStride, dstSlices,
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
         GLubyte *dstRow = dstSlices[img];
         for (row = 0; row < srcHeight; row++) {
            _mesa_swap2((GLushort *) dstRow, srcWidth);
            dstRow += dstRowStride;
         }
      }
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
      = _mesa_image_row_stride(srcPacking, srcWidth, srcFormat, srcType);
   GLint img, row;
   GLuint *depth = malloc(srcWidth * sizeof(GLuint));
   GLubyte *stencil = malloc(srcWidth * sizeof(GLubyte));

   ASSERT(dstFormat == MESA_FORMAT_S8_UINT_Z24_UNORM);
   ASSERT(srcFormat == GL_DEPTH_STENCIL_EXT ||
          srcFormat == GL_DEPTH_COMPONENT ||
          srcFormat == GL_STENCIL_INDEX);
   ASSERT(srcFormat != GL_DEPTH_STENCIL_EXT ||
          srcType == GL_UNSIGNED_INT_24_8_EXT ||
          srcType == GL_FLOAT_32_UNSIGNED_INT_24_8_REV);

   if (!depth || !stencil) {
      free(depth);
      free(stencil);
      return GL_FALSE;
   }

   /* In case we only upload depth we need to preserve the stencil */
   for (img = 0; img < srcDepth; img++) {
      GLuint *dstRow = (GLuint *) dstSlices[img];
      const GLubyte *src
         = (const GLubyte *) _mesa_image_address(dims, srcPacking, srcAddr,
               srcWidth, srcHeight,
               srcFormat, srcType,
               img, 0, 0);
      for (row = 0; row < srcHeight; row++) {
         GLint i;
         GLboolean keepdepth = GL_FALSE, keepstencil = GL_FALSE;

         if (srcFormat == GL_DEPTH_COMPONENT) { /* preserve stencil */
            keepstencil = GL_TRUE;
         }
         else if (srcFormat == GL_STENCIL_INDEX) { /* preserve depth */
            keepdepth = GL_TRUE;
         }

         if (keepdepth == GL_FALSE)
            /* the 24 depth bits will be in the low position: */
            _mesa_unpack_depth_span(ctx, srcWidth,
                                    GL_UNSIGNED_INT, /* dst type */
                                    keepstencil ? depth : dstRow, /* dst addr */
                                    depthScale,
                                    srcType, src, srcPacking);

         if (keepstencil == GL_FALSE)
            /* get the 8-bit stencil values */
            _mesa_unpack_stencil_span(ctx, srcWidth,
                                      GL_UNSIGNED_BYTE, /* dst type */
                                      stencil, /* dst addr */
                                      srcType, src, srcPacking,
                                      ctx->_ImageTransferState);

         for (i = 0; i < srcWidth; i++) {
            if (keepstencil)
               dstRow[i] = depth[i] << 8 | (dstRow[i] & 0x000000FF);
            else
               dstRow[i] = (dstRow[i] & 0xFFFFFF00) | (stencil[i] & 0xFF);
         }
         src += srcRowStride;
         dstRow += dstRowStride / sizeof(GLuint);
      }
   }

   free(depth);
   free(stencil);
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
      = _mesa_image_row_stride(srcPacking, srcWidth, srcFormat, srcType);
   GLint img, row;
   GLuint *depth;
   GLubyte *stencil;

   ASSERT(dstFormat == MESA_FORMAT_Z24_UNORM_S8_UINT);
   ASSERT(srcFormat == GL_DEPTH_STENCIL_EXT ||
          srcFormat == GL_DEPTH_COMPONENT ||
          srcFormat == GL_STENCIL_INDEX);
   ASSERT(srcFormat != GL_DEPTH_STENCIL_EXT ||
          srcType == GL_UNSIGNED_INT_24_8_EXT ||
          srcType == GL_FLOAT_32_UNSIGNED_INT_24_8_REV);

   depth = malloc(srcWidth * sizeof(GLuint));
   stencil = malloc(srcWidth * sizeof(GLubyte));

   if (!depth || !stencil) {
      free(depth);
      free(stencil);
      return GL_FALSE;
   }

   for (img = 0; img < srcDepth; img++) {
      GLuint *dstRow = (GLuint *) dstSlices[img];
      const GLubyte *src
	 = (const GLubyte *) _mesa_image_address(dims, srcPacking, srcAddr,
						srcWidth, srcHeight,
						srcFormat, srcType,
						img, 0, 0);
      for (row = 0; row < srcHeight; row++) {
	 GLint i;
	 GLboolean keepdepth = GL_FALSE, keepstencil = GL_FALSE;
	 
	 if (srcFormat == GL_DEPTH_COMPONENT) { /* preserve stencil */
	    keepstencil = GL_TRUE;
	 }
         else if (srcFormat == GL_STENCIL_INDEX) { /* preserve depth */
	    keepdepth = GL_TRUE;
	 }

	 if (keepdepth == GL_FALSE)
	    /* the 24 depth bits will be in the low position: */
	    _mesa_unpack_depth_span(ctx, srcWidth,
				    GL_UNSIGNED_INT, /* dst type */
				    keepstencil ? depth : dstRow, /* dst addr */
				    depthScale,
				    srcType, src, srcPacking);	 

	 if (keepstencil == GL_FALSE)
	    /* get the 8-bit stencil values */
	    _mesa_unpack_stencil_span(ctx, srcWidth,
				      GL_UNSIGNED_BYTE, /* dst type */
				      stencil, /* dst addr */
				      srcType, src, srcPacking,
				      ctx->_ImageTransferState);

	 /* merge stencil values into depth values */
	 for (i = 0; i < srcWidth; i++) {
	    if (keepstencil)
	       dstRow[i] = depth[i] | (dstRow[i] & 0xFF000000);
	    else
	       dstRow[i] = (dstRow[i] & 0xFFFFFF) | (stencil[i] << 24);

	 }
	 src += srcRowStride;
	 dstRow += dstRowStride / sizeof(GLuint);
      }
   }

   free(depth);
   free(stencil);

   return GL_TRUE;
}


/**
 * Store simple 8-bit/value stencil texture data.
 */
static GLboolean
_mesa_texstore_s8(TEXSTORE_PARAMS)
{
   ASSERT(dstFormat == MESA_FORMAT_S_UINT8);
   ASSERT(srcFormat == GL_STENCIL_INDEX);

   {
      const GLint srcRowStride
	 = _mesa_image_row_stride(srcPacking, srcWidth, srcFormat, srcType);
      GLint img, row;
      GLubyte *stencil = malloc(srcWidth * sizeof(GLubyte));

      if (!stencil)
         return GL_FALSE;

      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];
         const GLubyte *src
            = (const GLubyte *) _mesa_image_address(dims, srcPacking, srcAddr,
                                                   srcWidth, srcHeight,
                                                   srcFormat, srcType,
                                                   img, 0, 0);
         for (row = 0; row < srcHeight; row++) {
            GLint i;

            /* get the 8-bit stencil values */
            _mesa_unpack_stencil_span(ctx, srcWidth,
                                      GL_UNSIGNED_BYTE, /* dst type */
                                      stencil, /* dst addr */
                                      srcType, src, srcPacking,
                                      ctx->_ImageTransferState);
            /* merge stencil values into depth values */
            for (i = 0; i < srcWidth; i++)
               dstRow[i] = stencil[i];

            src += srcRowStride;
            dstRow += dstRowStride / sizeof(GLubyte);
         }
      }

      free(stencil);
   }

   return GL_TRUE;
}


static GLboolean
_mesa_texstore_z32f_x24s8(TEXSTORE_PARAMS)
{
   GLint img, row;
   const GLint srcRowStride
      = _mesa_image_row_stride(srcPacking, srcWidth, srcFormat, srcType)
      / sizeof(uint64_t);

   ASSERT(dstFormat == MESA_FORMAT_Z32_FLOAT_S8X24_UINT);
   ASSERT(srcFormat == GL_DEPTH_STENCIL ||
          srcFormat == GL_DEPTH_COMPONENT ||
          srcFormat == GL_STENCIL_INDEX);
   ASSERT(srcFormat != GL_DEPTH_STENCIL ||
          srcType == GL_UNSIGNED_INT_24_8 ||
          srcType == GL_FLOAT_32_UNSIGNED_INT_24_8_REV);

   /* In case we only upload depth we need to preserve the stencil */
   for (img = 0; img < srcDepth; img++) {
      uint64_t *dstRow = (uint64_t *) dstSlices[img];
      const uint64_t *src
         = (const uint64_t *) _mesa_image_address(dims, srcPacking, srcAddr,
               srcWidth, srcHeight,
               srcFormat, srcType,
               img, 0, 0);
      for (row = 0; row < srcHeight; row++) {
         /* The unpack functions with:
          *    dstType = GL_FLOAT_32_UNSIGNED_INT_24_8_REV
          * only write their own dword, so the other dword (stencil
          * or depth) is preserved. */
         if (srcFormat != GL_STENCIL_INDEX)
            _mesa_unpack_depth_span(ctx, srcWidth,
                                    GL_FLOAT_32_UNSIGNED_INT_24_8_REV, /* dst type */
                                    dstRow, /* dst addr */
                                    ~0U, srcType, src, srcPacking);

         if (srcFormat != GL_DEPTH_COMPONENT)
            _mesa_unpack_stencil_span(ctx, srcWidth,
                                      GL_FLOAT_32_UNSIGNED_INT_24_8_REV, /* dst type */
                                      dstRow, /* dst addr */
                                      srcType, src, srcPacking,
                                      ctx->_ImageTransferState);

         src += srcRowStride;
         dstRow += dstRowStride / sizeof(uint64_t);
      }
   }
   return GL_TRUE;
}

static GLboolean
_mesa_texstore_argb2101010_uint(TEXSTORE_PARAMS)
{
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);

   ASSERT(dstFormat == MESA_FORMAT_B10G10R10A2_UINT);
   ASSERT(_mesa_get_format_bytes(dstFormat) == 4);

   {
      /* general path */
      const GLuint *tempImage = make_temp_uint_image(ctx, dims,
                                                     baseInternalFormat,
                                                     baseFormat,
                                                     srcWidth, srcHeight,
                                                     srcDepth, srcFormat,
                                                     srcType, srcAddr,
                                                     srcPacking);
      const GLuint *src = tempImage;
      GLint img, row, col;
      GLboolean is_unsigned = _mesa_is_type_unsigned(srcType);
      if (!tempImage)
         return GL_FALSE;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];

         for (row = 0; row < srcHeight; row++) {
            GLuint *dstUI = (GLuint *) dstRow;
            if (is_unsigned) {
               for (col = 0; col < srcWidth; col++) {
                  GLushort a,r,g,b;
                  r = MIN2(src[RCOMP], 0x3ff);
                  g = MIN2(src[GCOMP], 0x3ff);
                  b = MIN2(src[BCOMP], 0x3ff);
                  a = MIN2(src[ACOMP], 0x003);
                  dstUI[col] = (a << 30) | (r << 20) | (g << 10) | (b);
                  src += 4;
               }
            } else {
               for (col = 0; col < srcWidth; col++) {
                  GLushort a,r,g,b;
                  r = CLAMP((GLint) src[RCOMP], 0, 0x3ff);
                  g = CLAMP((GLint) src[GCOMP], 0, 0x3ff);
                  b = CLAMP((GLint) src[BCOMP], 0, 0x3ff);
                  a = CLAMP((GLint) src[ACOMP], 0, 0x003);
                  dstUI[col] = (a << 30) | (r << 20) | (g << 10) | (b);
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
_mesa_texstore_abgr2101010_uint(TEXSTORE_PARAMS)
{
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);

   ASSERT(dstFormat == MESA_FORMAT_R10G10B10A2_UINT);
   ASSERT(_mesa_get_format_bytes(dstFormat) == 4);

   {
      /* general path */
      const GLuint *tempImage = make_temp_uint_image(ctx, dims,
                                                     baseInternalFormat,
                                                     baseFormat,
                                                     srcWidth, srcHeight,
                                                     srcDepth, srcFormat,
                                                     srcType, srcAddr,
                                                     srcPacking);
      const GLuint *src = tempImage;
      GLint img, row, col;
      GLboolean is_unsigned = _mesa_is_type_unsigned(srcType);
      if (!tempImage)
         return GL_FALSE;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];

         for (row = 0; row < srcHeight; row++) {
            GLuint *dstUI = (GLuint *) dstRow;
            if (is_unsigned) {
               for (col = 0; col < srcWidth; col++) {
                  GLushort a,r,g,b;
                  r = MIN2(src[RCOMP], 0x3ff);
                  g = MIN2(src[GCOMP], 0x3ff);
                  b = MIN2(src[BCOMP], 0x3ff);
                  a = MIN2(src[ACOMP], 0x003);
                  dstUI[col] = (a << 30) | (b << 20) | (g << 10) | (r);
                  src += 4;
               }
            } else {
               for (col = 0; col < srcWidth; col++) {
                  GLushort a,r,g,b;
                  r = CLAMP((GLint) src[RCOMP], 0, 0x3ff);
                  g = CLAMP((GLint) src[GCOMP], 0, 0x3ff);
                  b = CLAMP((GLint) src[BCOMP], 0, 0x3ff);
                  a = CLAMP((GLint) src[ACOMP], 0, 0x003);
                  dstUI[col] = (a << 30) | (b << 20) | (g << 10) | (r);
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
texstore_depth_stencil(TEXSTORE_PARAMS)
{
   static StoreTexImageFunc table[MESA_FORMAT_COUNT];
   static GLboolean initialized = GL_FALSE;

   if (!initialized) {
      memset(table, 0, sizeof table);

      table[MESA_FORMAT_S8_UINT_Z24_UNORM] = _mesa_texstore_z24_s8;
      table[MESA_FORMAT_Z24_UNORM_S8_UINT] = _mesa_texstore_s8_z24;
      table[MESA_FORMAT_Z_UNORM16] = _mesa_texstore_z16;
      table[MESA_FORMAT_Z24_UNORM_X8_UINT] = _mesa_texstore_x8_z24;
      table[MESA_FORMAT_X8_UINT_Z24_UNORM] = _mesa_texstore_z24_x8;
      table[MESA_FORMAT_Z_UNORM32] = _mesa_texstore_z32;
      table[MESA_FORMAT_S_UINT8] = _mesa_texstore_s8;
      table[MESA_FORMAT_Z_FLOAT32] = _mesa_texstore_z32;
      table[MESA_FORMAT_Z32_FLOAT_S8X24_UINT] = _mesa_texstore_z32f_x24s8;

      initialized = GL_TRUE;
   }

   ASSERT(table[dstFormat]);
   return table[dstFormat](ctx, dims, baseInternalFormat,
                           dstFormat, dstRowStride, dstSlices,
                           srcWidth, srcHeight, srcDepth,
                           srcFormat, srcType, srcAddr, srcPacking);
}

static GLboolean
texstore_compressed(TEXSTORE_PARAMS)
{
   static StoreTexImageFunc table[MESA_FORMAT_COUNT];
   static GLboolean initialized = GL_FALSE;

   if (!initialized) {
      memset(table, 0, sizeof table);

      table[MESA_FORMAT_SRGB_DXT1] = _mesa_texstore_rgb_dxt1;
      table[MESA_FORMAT_SRGBA_DXT1] = _mesa_texstore_rgba_dxt1;
      table[MESA_FORMAT_SRGBA_DXT3] = _mesa_texstore_rgba_dxt3;
      table[MESA_FORMAT_SRGBA_DXT5] = _mesa_texstore_rgba_dxt5;
      table[MESA_FORMAT_RGB_FXT1] = _mesa_texstore_rgb_fxt1;
      table[MESA_FORMAT_RGBA_FXT1] = _mesa_texstore_rgba_fxt1;
      table[MESA_FORMAT_RGB_DXT1] = _mesa_texstore_rgb_dxt1;
      table[MESA_FORMAT_RGBA_DXT1] = _mesa_texstore_rgba_dxt1;
      table[MESA_FORMAT_RGBA_DXT3] = _mesa_texstore_rgba_dxt3;
      table[MESA_FORMAT_RGBA_DXT5] = _mesa_texstore_rgba_dxt5;
      table[MESA_FORMAT_R_RGTC1_UNORM] = _mesa_texstore_red_rgtc1;
      table[MESA_FORMAT_R_RGTC1_SNORM] = _mesa_texstore_signed_red_rgtc1;
      table[MESA_FORMAT_RG_RGTC2_UNORM] = _mesa_texstore_rg_rgtc2;
      table[MESA_FORMAT_RG_RGTC2_SNORM] = _mesa_texstore_signed_rg_rgtc2;
      table[MESA_FORMAT_L_LATC1_UNORM] = _mesa_texstore_red_rgtc1;
      table[MESA_FORMAT_L_LATC1_SNORM] = _mesa_texstore_signed_red_rgtc1;
      table[MESA_FORMAT_LA_LATC2_UNORM] = _mesa_texstore_rg_rgtc2;
      table[MESA_FORMAT_LA_LATC2_SNORM] = _mesa_texstore_signed_rg_rgtc2;
      table[MESA_FORMAT_ETC1_RGB8] = _mesa_texstore_etc1_rgb8;
      table[MESA_FORMAT_ETC2_RGB8] = _mesa_texstore_etc2_rgb8;
      table[MESA_FORMAT_ETC2_SRGB8] = _mesa_texstore_etc2_srgb8;
      table[MESA_FORMAT_ETC2_RGBA8_EAC] = _mesa_texstore_etc2_rgba8_eac;
      table[MESA_FORMAT_ETC2_SRGB8_ALPHA8_EAC] = _mesa_texstore_etc2_srgb8_alpha8_eac;
      table[MESA_FORMAT_ETC2_R11_EAC] = _mesa_texstore_etc2_r11_eac;
      table[MESA_FORMAT_ETC2_RG11_EAC] = _mesa_texstore_etc2_rg11_eac;
      table[MESA_FORMAT_ETC2_SIGNED_R11_EAC] = _mesa_texstore_etc2_signed_r11_eac;
      table[MESA_FORMAT_ETC2_SIGNED_RG11_EAC] = _mesa_texstore_etc2_signed_rg11_eac;
      table[MESA_FORMAT_ETC2_RGB8_PUNCHTHROUGH_ALPHA1] =
         _mesa_texstore_etc2_rgb8_punchthrough_alpha1;
      table[MESA_FORMAT_ETC2_SRGB8_PUNCHTHROUGH_ALPHA1] =
         _mesa_texstore_etc2_srgb8_punchthrough_alpha1;

      table[MESA_FORMAT_BPTC_RGBA_UNORM] =
         _mesa_texstore_bptc_rgba_unorm;
      table[MESA_FORMAT_BPTC_SRGB_ALPHA_UNORM] =
         _mesa_texstore_bptc_rgba_unorm;
      table[MESA_FORMAT_BPTC_RGB_SIGNED_FLOAT] =
         _mesa_texstore_bptc_rgb_signed_float;
      table[MESA_FORMAT_BPTC_RGB_UNSIGNED_FLOAT] =
         _mesa_texstore_bptc_rgb_unsigned_float;

      initialized = GL_TRUE;
   }

   ASSERT(table[dstFormat]);
   return table[dstFormat](ctx, dims, baseInternalFormat,
                           dstFormat, dstRowStride, dstSlices,
                           srcWidth, srcHeight, srcDepth,
                           srcFormat, srcType, srcAddr, srcPacking);
}

static void
invert_swizzle(uint8_t dst[4], const uint8_t src[4])
{
   int i, j;

   dst[0] = MESA_FORMAT_SWIZZLE_NONE;
   dst[1] = MESA_FORMAT_SWIZZLE_NONE;
   dst[2] = MESA_FORMAT_SWIZZLE_NONE;
   dst[3] = MESA_FORMAT_SWIZZLE_NONE;

   for (i = 0; i < 4; ++i)
      for (j = 0; j < 4; ++j)
         if (src[j] == i && dst[i] == MESA_FORMAT_SWIZZLE_NONE)
            dst[i] = j;
}

/** Store a texture by per-channel conversions and swizzling.
 *
 * This function attempts to perform a texstore operation by doing simple
 * per-channel conversions and swizzling.  This covers a huge chunk of the
 * texture storage operations that anyone cares about.  If this function is
 * incapable of performing the operation, it bails and returns GL_FALSE.
 */
static GLboolean
texstore_swizzle(TEXSTORE_PARAMS)
{
   const GLint srcRowStride = _mesa_image_row_stride(srcPacking, srcWidth,
                                                     srcFormat, srcType);
   const GLint srcImageStride = _mesa_image_image_stride(srcPacking,
                                      srcWidth, srcHeight, srcFormat, srcType);
   const GLubyte *srcImage = (const GLubyte *) _mesa_image_address(dims,
        srcPacking, srcAddr, srcWidth, srcHeight, srcFormat, srcType, 0, 0, 0);
   const int src_components = _mesa_components_in_format(srcFormat);

   GLubyte swizzle[4], rgba2base[6], base2src[6], rgba2dst[4], dst2rgba[4];
   const GLubyte *swap;
   GLenum dst_type;
   int dst_components;
   bool is_array, normalized, need_swap;
   GLint i, img, row;
   const GLubyte *src_row;
   GLubyte *dst_row;

   is_array = _mesa_format_to_array(dstFormat, &dst_type, &dst_components,
                                    rgba2dst, &normalized);

   if (!is_array)
      return GL_FALSE;

   if (srcFormat == GL_COLOR_INDEX)
      return GL_FALSE;

   if (_mesa_texstore_needs_transfer_ops(ctx, baseInternalFormat, dstFormat))
      return GL_FALSE;

   switch (srcType) {
   case GL_FLOAT:
   case GL_UNSIGNED_BYTE:
   case GL_BYTE:
   case GL_UNSIGNED_SHORT:
   case GL_SHORT:
   case GL_UNSIGNED_INT:
   case GL_INT:
      /* If wa have to swap bytes in a multi-byte datatype, that means
       * we're not doing an array conversion anymore */
      if (srcPacking->SwapBytes)
         return GL_FALSE;
      need_swap = false;
      break;
   case GL_UNSIGNED_INT_8_8_8_8:
      need_swap = srcPacking->SwapBytes;
      if (_mesa_little_endian())
         need_swap = !need_swap;
      srcType = GL_UNSIGNED_BYTE;
      break;
   case GL_UNSIGNED_INT_8_8_8_8_REV:
      need_swap = srcPacking->SwapBytes;
      if (!_mesa_little_endian())
         need_swap = !need_swap;
      srcType = GL_UNSIGNED_BYTE;
      break;
   default:
      return GL_FALSE;
   }
   swap = need_swap ? map_3210 : map_identity;

   compute_component_mapping(srcFormat, baseInternalFormat, base2src);
   compute_component_mapping(baseInternalFormat, GL_RGBA, rgba2base);
   invert_swizzle(dst2rgba, rgba2dst);

   for (i = 0; i < 4; i++) {
      if (dst2rgba[i] == MESA_FORMAT_SWIZZLE_NONE)
         swizzle[i] = MESA_FORMAT_SWIZZLE_NONE;
      else
         swizzle[i] = swap[base2src[rgba2base[dst2rgba[i]]]];
   }

   /* Is it normalized? */
   normalized |= !_mesa_is_enum_format_integer(srcFormat);

   for (img = 0; img < srcDepth; img++) {
      if (dstRowStride == srcWidth * dst_components &&
          srcRowStride == srcWidth * src_components) {
         _mesa_swizzle_and_convert(dstSlices[img], dst_type, dst_components,
                                   srcImage, srcType, src_components,
                                   swizzle, normalized, srcWidth * srcHeight);
      } else {
         src_row = srcImage;
         dst_row = dstSlices[img];
         for (row = 0; row < srcHeight; row++) {
            _mesa_swizzle_and_convert(dst_row, dst_type, dst_components,
                                      src_row, srcType, src_components,
                                      swizzle, normalized, srcWidth);
            dst_row += dstRowStride;
            src_row += srcRowStride;
         }
      }
      srcImage += srcImageStride;
   }

   return GL_TRUE;
}


/** Stores a texture by converting float and then to the texture format
 *
 * This function performs a texstore operation by converting to float,
 * applying pixel transfer ops, and then converting to the texture's
 * internal format using pixel store functions.  This function will work
 * for any rgb or srgb textore format.
 */
static GLboolean
texstore_via_float(TEXSTORE_PARAMS)
{
   GLuint i, img, row;
   const GLint src_stride =
      _mesa_image_row_stride(srcPacking, srcWidth, srcFormat, srcType);
   float *tmp_row;
   bool need_convert;
   uint8_t *src_row, *dst_row, map[4], rgba2base[6], base2rgba[6];

   tmp_row = malloc(srcWidth * 4 * sizeof(*tmp_row));
   if (!tmp_row)
      return GL_FALSE;

   /* The GL spec (4.0, compatibility profile) only specifies srgb
    * conversion as something that is done in the sampler during the
    * filtering process before the colors are handed to the shader.
    * Furthermore, the flowchart (Figure 3.7 in the 4.0 compatibility spec)
    * does not list RGB <-> sRGB conversions anywhere.  Therefore, we just
    * treat sRGB formats the same as RGB formats for the purposes of
    * texture upload and transfer ops.
    */
   dstFormat = _mesa_get_srgb_format_linear(dstFormat);

   need_convert = false;
   if (baseInternalFormat != _mesa_get_format_base_format(dstFormat)) {
      compute_component_mapping(GL_RGBA, baseInternalFormat, base2rgba);
      compute_component_mapping(baseInternalFormat, GL_RGBA, rgba2base);
      for (i = 0; i < 4; ++i) {
         map[i] = base2rgba[rgba2base[i]];
         if (map[i] != i)
            need_convert = true;
      }
   }

   for (img = 0; img < srcDepth; img++) {
      dst_row = dstSlices[img];
      src_row = _mesa_image_address(dims, srcPacking, srcAddr,
                                    srcWidth, srcHeight,
                                    srcFormat, srcType,
                                    img, 0, 0);
      for (row = 0; row < srcHeight; row++) {
	 _mesa_unpack_color_span_float(ctx, srcWidth, GL_RGBA, tmp_row,
                                       srcFormat, srcType, src_row,
				       srcPacking, ctx->_ImageTransferState);
         if (need_convert)
            _mesa_swizzle_and_convert(tmp_row, GL_FLOAT, 4,
                                      tmp_row, GL_FLOAT, 4,
                                      map, false, srcWidth);
         _mesa_pack_float_rgba_row(dstFormat, srcWidth,
                                   (const GLfloat (*)[4])tmp_row,
                                   dst_row);
         dst_row += dstRowStride;
         src_row += src_stride;
      }
   }

   free(tmp_row);

   return GL_TRUE;
}

/** Stores an integer rgba texture
 *
 * This function performs an integer texture storage operation by unpacking
 * the texture to 32-bit integers, and repacking it into the internal
 * format of the texture.  This will work for any integer rgb texture
 * storage operation.
 */
static GLboolean
texstore_rgba_integer(TEXSTORE_PARAMS)
{
   GLuint i, img, row, *tmp_row;
   GLenum dst_type, tmp_type;
   const GLint src_stride =
      _mesa_image_row_stride(srcPacking, srcWidth, srcFormat, srcType);
   int num_dst_components;
   bool is_array, normalized;
   uint8_t *src_row, *dst_row;
   uint8_t swizzle[4], rgba2base[6], base2rgba[6], rgba2dst[4], dst2rgba[4];

   tmp_row = malloc(srcWidth * 4 * sizeof(*tmp_row));
   if (!tmp_row)
      return GL_FALSE;

   is_array = _mesa_format_to_array(dstFormat, &dst_type, &num_dst_components,
                                    rgba2dst, &normalized);

   assert(is_array && !normalized);

   if (!is_array)
      return GL_FALSE;

   invert_swizzle(dst2rgba, rgba2dst);
   compute_component_mapping(GL_RGBA, baseInternalFormat, base2rgba);
   compute_component_mapping(baseInternalFormat, GL_RGBA, rgba2base);

   for (i = 0; i < 4; ++i) {
      if (dst2rgba[i] == MESA_FORMAT_SWIZZLE_NONE)
         swizzle[i] = MESA_FORMAT_SWIZZLE_NONE;
      else
         swizzle[i] = base2rgba[rgba2base[dst2rgba[i]]];
   }

   if (_mesa_is_type_unsigned(srcType)) {
      tmp_type = GL_UNSIGNED_INT;
   } else {
      tmp_type = GL_INT;
   }

   for (img = 0; img < srcDepth; img++) {
      dst_row = dstSlices[img];
      src_row = _mesa_image_address(dims, srcPacking, srcAddr,
                                    srcWidth, srcHeight,
                                    srcFormat, srcType,
                                    img, 0, 0);
      for (row = 0; row < srcHeight; row++) {
	 _mesa_unpack_color_span_uint(ctx, srcWidth, GL_RGBA, tmp_row,
                                      srcFormat, srcType, src_row, srcPacking);
         _mesa_swizzle_and_convert(dst_row, dst_type, num_dst_components,
                                   tmp_row, tmp_type, 4,
                                   swizzle, false, srcWidth);
         dst_row += dstRowStride;
         src_row += src_stride;
      }
   }

   free(tmp_row);

   return GL_TRUE;
}

static GLboolean
texstore_rgba(TEXSTORE_PARAMS)
{
   static StoreTexImageFunc table[MESA_FORMAT_COUNT];
   static GLboolean initialized = GL_FALSE;

   if (!initialized) {
      memset(table, 0, sizeof table);

      table[MESA_FORMAT_B5G6R5_UNORM] = _mesa_texstore_rgb565;
      table[MESA_FORMAT_R5G6B5_UNORM] = _mesa_texstore_rgb565;
      table[MESA_FORMAT_YCBCR] = _mesa_texstore_ycbcr;
      table[MESA_FORMAT_YCBCR_REV] = _mesa_texstore_ycbcr;

      table[MESA_FORMAT_B10G10R10A2_UINT] = _mesa_texstore_argb2101010_uint;
      table[MESA_FORMAT_R10G10B10A2_UINT] = _mesa_texstore_abgr2101010_uint;

      initialized = GL_TRUE;
   }

   if (table[dstFormat] && table[dstFormat](ctx, dims, baseInternalFormat,
                                            dstFormat, dstRowStride, dstSlices,
                                            srcWidth, srcHeight, srcDepth,
                                            srcFormat, srcType, srcAddr,
                                            srcPacking)) {
      return GL_TRUE;
   }

   if (texstore_swizzle(ctx, dims, baseInternalFormat,
                        dstFormat,
                        dstRowStride, dstSlices,
                        srcWidth, srcHeight, srcDepth,
                        srcFormat, srcType, srcAddr, srcPacking)) {
      return GL_TRUE;
   }

   if (_mesa_is_format_integer(dstFormat)) {
      return texstore_rgba_integer(ctx, dims, baseInternalFormat,
                                   dstFormat, dstRowStride, dstSlices,
                                   srcWidth, srcHeight, srcDepth,
                                   srcFormat, srcType, srcAddr,
                                   srcPacking);
   } else if (_mesa_get_format_max_bits(dstFormat) <= 8 &&
              !_mesa_is_format_signed(dstFormat)) {
      return store_ubyte_texture(ctx, dims, baseInternalFormat,
                                 dstFormat,
                                 dstRowStride, dstSlices,
                                 srcWidth, srcHeight, srcDepth,
                                 srcFormat, srcType, srcAddr, srcPacking);
   } else {
      return texstore_via_float(ctx, dims, baseInternalFormat,
                                dstFormat, dstRowStride, dstSlices,
                                srcWidth, srcHeight, srcDepth,
                                srcFormat, srcType, srcAddr,
                                srcPacking);
   }
}

GLboolean
_mesa_texstore_needs_transfer_ops(struct gl_context *ctx,
                                  GLenum baseInternalFormat,
                                  mesa_format dstFormat)
{
   GLenum dstType;

   /* There are different rules depending on the base format. */
   switch (baseInternalFormat) {
   case GL_DEPTH_COMPONENT:
   case GL_DEPTH_STENCIL:
      return ctx->Pixel.DepthScale != 1.0f ||
             ctx->Pixel.DepthBias != 0.0f;

   case GL_STENCIL_INDEX:
      return GL_FALSE;

   default:
      /* Color formats.
       * Pixel transfer ops (scale, bias, table lookup) do not apply
       * to integer formats.
       */
      dstType = _mesa_get_format_datatype(dstFormat);

      return dstType != GL_INT && dstType != GL_UNSIGNED_INT &&
             ctx->_ImageTransferState;
   }
}


GLboolean
_mesa_texstore_can_use_memcpy(struct gl_context *ctx,
                              GLenum baseInternalFormat, mesa_format dstFormat,
                              GLenum srcFormat, GLenum srcType,
                              const struct gl_pixelstore_attrib *srcPacking)
{
   if (_mesa_texstore_needs_transfer_ops(ctx, baseInternalFormat, dstFormat)) {
      return GL_FALSE;
   }

   /* The base internal format and the base Mesa format must match. */
   if (baseInternalFormat != _mesa_get_format_base_format(dstFormat)) {
      return GL_FALSE;
   }

   /* The Mesa format must match the input format and type. */
   if (!_mesa_format_matches_format_and_type(dstFormat, srcFormat, srcType,
                                             srcPacking->SwapBytes)) {
      return GL_FALSE;
   }

   /* Depth texture data needs clamping in following cases:
    * - Floating point dstFormat with signed srcType: clamp to [0.0, 1.0].
    * - Fixed point dstFormat with signed srcType: clamp to [0, 2^n -1].
    *
    * All the cases except one (float dstFormat with float srcType) are ruled
    * out by _mesa_format_matches_format_and_type() check above. Handle the
    * remaining case here.
    */
   if ((baseInternalFormat == GL_DEPTH_COMPONENT ||
        baseInternalFormat == GL_DEPTH_STENCIL) &&
       (srcType == GL_FLOAT ||
        srcType == GL_FLOAT_32_UNSIGNED_INT_24_8_REV)) {
      return GL_FALSE;
   }

   return GL_TRUE;
}

static GLboolean
_mesa_texstore_memcpy(TEXSTORE_PARAMS)
{
   if (!_mesa_texstore_can_use_memcpy(ctx, baseInternalFormat, dstFormat,
                                      srcFormat, srcType, srcPacking)) {
      return GL_FALSE;
   }

   memcpy_texture(ctx, dims,
                  dstFormat,
                  dstRowStride, dstSlices,
                  srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                  srcAddr, srcPacking);
   return GL_TRUE;
}
/**
 * Store user data into texture memory.
 * Called via glTex[Sub]Image1/2/3D()
 * \return GL_TRUE for success, GL_FALSE for failure (out of memory).
 */
GLboolean
_mesa_texstore(TEXSTORE_PARAMS)
{
   if (_mesa_texstore_memcpy(ctx, dims, baseInternalFormat,
                             dstFormat,
                             dstRowStride, dstSlices,
                             srcWidth, srcHeight, srcDepth,
                             srcFormat, srcType, srcAddr, srcPacking)) {
      return GL_TRUE;
   }

   if (_mesa_is_depth_or_stencil_format(baseInternalFormat)) {
      return texstore_depth_stencil(ctx, dims, baseInternalFormat,
                                    dstFormat, dstRowStride, dstSlices,
                                    srcWidth, srcHeight, srcDepth,
                                    srcFormat, srcType, srcAddr, srcPacking);
   } else if (_mesa_is_format_compressed(dstFormat)) {
      return texstore_compressed(ctx, dims, baseInternalFormat,
                                 dstFormat, dstRowStride, dstSlices,
                                 srcWidth, srcHeight, srcDepth,
                                 srcFormat, srcType, srcAddr, srcPacking);
   } else {
      return texstore_rgba(ctx, dims, baseInternalFormat,
                           dstFormat, dstRowStride, dstSlices,
                           srcWidth, srcHeight, srcDepth,
                           srcFormat, srcType, srcAddr, srcPacking);
   }
}


/**
 * Normally, we'll only _write_ texel data to a texture when we map it.
 * But if the user is providing depth or stencil values and the texture
 * image is a combined depth/stencil format, we'll actually read from
 * the texture buffer too (in order to insert the depth or stencil values.
 * \param userFormat  the user-provided image format
 * \param texFormat  the destination texture format
 */
static GLbitfield
get_read_write_mode(GLenum userFormat, mesa_format texFormat)
{
   if ((userFormat == GL_STENCIL_INDEX || userFormat == GL_DEPTH_COMPONENT)
       && _mesa_get_format_base_format(texFormat) == GL_DEPTH_STENCIL)
      return GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
   else
      return GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT;
}


/**
 * Helper function for storing 1D, 2D, 3D whole and subimages into texture
 * memory.
 * The source of the image data may be user memory or a PBO.  In the later
 * case, we'll map the PBO, copy from it, then unmap it.
 */
static void
store_texsubimage(struct gl_context *ctx,
                  struct gl_texture_image *texImage,
                  GLint xoffset, GLint yoffset, GLint zoffset,
                  GLint width, GLint height, GLint depth,
                  GLenum format, GLenum type, const GLvoid *pixels,
                  const struct gl_pixelstore_attrib *packing,
                  const char *caller)

{
   const GLbitfield mapMode = get_read_write_mode(format, texImage->TexFormat);
   const GLenum target = texImage->TexObject->Target;
   GLboolean success = GL_FALSE;
   GLuint dims, slice, numSlices = 1, sliceOffset = 0;
   GLint srcImageStride = 0;
   const GLubyte *src;

   assert(xoffset + width <= texImage->Width);
   assert(yoffset + height <= texImage->Height);
   assert(zoffset + depth <= texImage->Depth);

   switch (target) {
   case GL_TEXTURE_1D:
      dims = 1;
      break;
   case GL_TEXTURE_2D_ARRAY:
   case GL_TEXTURE_CUBE_MAP_ARRAY:
   case GL_TEXTURE_3D:
      dims = 3;
      break;
   default:
      dims = 2;
   }

   /* get pointer to src pixels (may be in a pbo which we'll map here) */
   src = (const GLubyte *)
      _mesa_validate_pbo_teximage(ctx, dims, width, height, depth,
                                  format, type, pixels, packing, caller);
   if (!src)
      return;

   /* compute slice info (and do some sanity checks) */
   switch (target) {
   case GL_TEXTURE_2D:
   case GL_TEXTURE_RECTANGLE:
   case GL_TEXTURE_CUBE_MAP:
   case GL_TEXTURE_EXTERNAL_OES:
      /* one image slice, nothing special needs to be done */
      break;
   case GL_TEXTURE_1D:
      assert(height == 1);
      assert(depth == 1);
      assert(yoffset == 0);
      assert(zoffset == 0);
      break;
   case GL_TEXTURE_1D_ARRAY:
      assert(depth == 1);
      assert(zoffset == 0);
      numSlices = height;
      sliceOffset = yoffset;
      height = 1;
      yoffset = 0;
      srcImageStride = _mesa_image_row_stride(packing, width, format, type);
      break;
   case GL_TEXTURE_2D_ARRAY:
      numSlices = depth;
      sliceOffset = zoffset;
      depth = 1;
      zoffset = 0;
      srcImageStride = _mesa_image_image_stride(packing, width, height,
                                                format, type);
      break;
   case GL_TEXTURE_3D:
      /* we'll store 3D images as a series of slices */
      numSlices = depth;
      sliceOffset = zoffset;
      srcImageStride = _mesa_image_image_stride(packing, width, height,
                                                format, type);
      break;
   case GL_TEXTURE_CUBE_MAP_ARRAY:
      numSlices = depth;
      sliceOffset = zoffset;
      srcImageStride = _mesa_image_image_stride(packing, width, height,
                                                format, type);
      break;
   default:
      _mesa_warning(ctx, "Unexpected target 0x%x in store_texsubimage()", target);
      return;
   }

   assert(numSlices == 1 || srcImageStride != 0);

   for (slice = 0; slice < numSlices; slice++) {
      GLubyte *dstMap;
      GLint dstRowStride;

      ctx->Driver.MapTextureImage(ctx, texImage,
                                  slice + sliceOffset,
                                  xoffset, yoffset, width, height,
                                  mapMode, &dstMap, &dstRowStride);
      if (dstMap) {
         /* Note: we're only storing a 2D (or 1D) slice at a time but we need
          * to pass the right 'dims' value so that GL_UNPACK_SKIP_IMAGES is
          * used for 3D images.
          */
         success = _mesa_texstore(ctx, dims, texImage->_BaseFormat,
                                  texImage->TexFormat,
                                  dstRowStride,
                                  &dstMap,
                                  width, height, 1,  /* w, h, d */
                                  format, type, src, packing);

         ctx->Driver.UnmapTextureImage(ctx, texImage, slice + sliceOffset);
      }

      src += srcImageStride;

      if (!success)
         break;
   }

   if (!success)
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "%s", caller);

   _mesa_unmap_teximage_pbo(ctx, packing);
}



/**
 * Fallback code for ctx->Driver.TexImage().
 * Basically, allocate storage for the texture image, then copy the
 * user's image into it.
 */
void
_mesa_store_teximage(struct gl_context *ctx,
                     GLuint dims,
                     struct gl_texture_image *texImage,
                     GLenum format, GLenum type, const GLvoid *pixels,
                     const struct gl_pixelstore_attrib *packing)
{
   assert(dims == 1 || dims == 2 || dims == 3);

   if (texImage->Width == 0 || texImage->Height == 0 || texImage->Depth == 0)
      return;

   /* allocate storage for texture data */
   if (!ctx->Driver.AllocTextureImageBuffer(ctx, texImage)) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage%uD", dims);
      return;
   }

   store_texsubimage(ctx, texImage,
                     0, 0, 0, texImage->Width, texImage->Height, texImage->Depth,
                     format, type, pixels, packing, "glTexImage");
}


/*
 * Fallback for Driver.TexSubImage().
 */
void
_mesa_store_texsubimage(struct gl_context *ctx, GLuint dims,
                        struct gl_texture_image *texImage,
                        GLint xoffset, GLint yoffset, GLint zoffset,
                        GLint width, GLint height, GLint depth,
                        GLenum format, GLenum type, const void *pixels,
                        const struct gl_pixelstore_attrib *packing)
{
   store_texsubimage(ctx, texImage,
                     xoffset, yoffset, zoffset, width, height, depth,
                     format, type, pixels, packing, "glTexSubImage");
}

static void
clear_image_to_zero(GLubyte *dstMap, GLint dstRowStride,
                    GLsizei width, GLsizei height,
                    GLsizei clearValueSize)
{
   GLsizei y;

   for (y = 0; y < height; y++) {
      memset(dstMap, 0, clearValueSize * width);
      dstMap += dstRowStride;
   }
}

static void
clear_image_to_value(GLubyte *dstMap, GLint dstRowStride,
                     GLsizei width, GLsizei height,
                     const GLvoid *clearValue,
                     GLsizei clearValueSize)
{
   GLsizei y, x;

   for (y = 0; y < height; y++) {
      for (x = 0; x < width; x++) {
         memcpy(dstMap, clearValue, clearValueSize);
         dstMap += clearValueSize;
      }
      dstMap += dstRowStride - clearValueSize * width;
   }
}

/*
 * Fallback for Driver.ClearTexSubImage().
 */
void
_mesa_store_cleartexsubimage(struct gl_context *ctx,
                             struct gl_texture_image *texImage,
                             GLint xoffset, GLint yoffset, GLint zoffset,
                             GLsizei width, GLsizei height, GLsizei depth,
                             const GLvoid *clearValue)
{
   GLubyte *dstMap;
   GLint dstRowStride;
   GLsizeiptr clearValueSize;
   GLsizei z;

   clearValueSize = _mesa_get_format_bytes(texImage->TexFormat);

   for (z = 0; z < depth; z++) {
      ctx->Driver.MapTextureImage(ctx, texImage,
                                  z + zoffset, xoffset, yoffset,
                                  width, height,
                                  GL_MAP_WRITE_BIT,
                                  &dstMap, &dstRowStride);
      if (dstMap == NULL) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glClearTex*Image");
         return;
      }

      if (clearValue) {
         clear_image_to_value(dstMap, dstRowStride,
                              width, height,
                              clearValue,
                              clearValueSize);
      } else {
         clear_image_to_zero(dstMap, dstRowStride,
                             width, height,
                             clearValueSize);
      }

      ctx->Driver.UnmapTextureImage(ctx, texImage, z + zoffset);
   }
}

/**
 * Fallback for Driver.CompressedTexImage()
 */
void
_mesa_store_compressed_teximage(struct gl_context *ctx, GLuint dims,
                                struct gl_texture_image *texImage,
                                GLsizei imageSize, const GLvoid *data)
{
   /* only 2D and 3D compressed images are supported at this time */
   if (dims == 1) {
      _mesa_problem(ctx, "Unexpected glCompressedTexImage1D call");
      return;
   }

   /* This is pretty simple, because unlike the general texstore path we don't
    * have to worry about the usual image unpacking or image transfer
    * operations.
    */
   ASSERT(texImage);
   ASSERT(texImage->Width > 0);
   ASSERT(texImage->Height > 0);
   ASSERT(texImage->Depth > 0);

   /* allocate storage for texture data */
   if (!ctx->Driver.AllocTextureImageBuffer(ctx, texImage)) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCompressedTexImage%uD", dims);
      return;
   }

   _mesa_store_compressed_texsubimage(ctx, dims, texImage,
                                      0, 0, 0,
                                      texImage->Width, texImage->Height, texImage->Depth,
                                      texImage->TexFormat,
                                      imageSize, data);
}


/**
 * Compute compressed_pixelstore parameters for copying compressed
 * texture data.
 * \param dims  number of texture image dimensions: 1, 2 or 3
 * \param texFormat  the compressed texture format
 * \param width, height, depth  size of image to copy
 * \param packing  pixelstore parameters describing user-space image packing
 * \param store  returns the compressed_pixelstore parameters
 */
void
_mesa_compute_compressed_pixelstore(GLuint dims, mesa_format texFormat,
                                    GLsizei width, GLsizei height,
                                    GLsizei depth,
                                    const struct gl_pixelstore_attrib *packing,
                                    struct compressed_pixelstore *store)
{
   GLuint bw, bh;

   _mesa_get_format_block_size(texFormat, &bw, &bh);

   store->SkipBytes = 0;
   store->TotalBytesPerRow = store->CopyBytesPerRow =
         _mesa_format_row_stride(texFormat, width);
   store->TotalRowsPerSlice = store->CopyRowsPerSlice =
         (height + bh - 1) / bh;
   store->CopySlices = depth;

   if (packing->CompressedBlockWidth &&
       packing->CompressedBlockSize) {

      bw = packing->CompressedBlockWidth;

      if (packing->RowLength) {
         store->TotalBytesPerRow = packing->CompressedBlockSize *
            (packing->RowLength + bw - 1) / bw;
      }

      store->SkipBytes += packing->SkipPixels * packing->CompressedBlockSize / bw;
   }

   if (dims > 1 && packing->CompressedBlockHeight &&
       packing->CompressedBlockSize) {

      bh = packing->CompressedBlockHeight;

      store->SkipBytes += packing->SkipRows * store->TotalBytesPerRow / bh;
      store->CopyRowsPerSlice = (height + bh - 1) / bh;  /* rows in blocks */

      if (packing->ImageHeight) {
         store->TotalRowsPerSlice = (packing->ImageHeight + bh - 1) / bh;
      }
   }

   if (dims > 2 && packing->CompressedBlockDepth &&
       packing->CompressedBlockSize) {

      int bd = packing->CompressedBlockDepth;

      store->SkipBytes += packing->SkipImages * store->TotalBytesPerRow *
            store->TotalRowsPerSlice / bd;
   }
}


/**
 * Fallback for Driver.CompressedTexSubImage()
 */
void
_mesa_store_compressed_texsubimage(struct gl_context *ctx, GLuint dims,
                                   struct gl_texture_image *texImage,
                                   GLint xoffset, GLint yoffset, GLint zoffset,
                                   GLsizei width, GLsizei height, GLsizei depth,
                                   GLenum format,
                                   GLsizei imageSize, const GLvoid *data)
{
   struct compressed_pixelstore store;
   GLint dstRowStride;
   GLint i, slice;
   GLubyte *dstMap;
   const GLubyte *src;

   if (dims == 1) {
      _mesa_problem(ctx, "Unexpected 1D compressed texsubimage call");
      return;
   }

   _mesa_compute_compressed_pixelstore(dims, texImage->TexFormat,
                                       width, height, depth,
                                       &ctx->Unpack, &store);

   /* get pointer to src pixels (may be in a pbo which we'll map here) */
   data = _mesa_validate_pbo_compressed_teximage(ctx, dims, imageSize, data,
                                                 &ctx->Unpack,
                                                 "glCompressedTexSubImage");
   if (!data)
      return;

   src = (const GLubyte *) data + store.SkipBytes;

   for (slice = 0; slice < store.CopySlices; slice++) {
      /* Map dest texture buffer */
      ctx->Driver.MapTextureImage(ctx, texImage, slice + zoffset,
                                  xoffset, yoffset, width, height,
                                  GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT,
                                  &dstMap, &dstRowStride);

      if (dstMap) {

         /* copy rows of blocks */
         for (i = 0; i < store.CopyRowsPerSlice; i++) {
            memcpy(dstMap, src, store.CopyBytesPerRow);
            dstMap += dstRowStride;
            src += store.TotalBytesPerRow;
         }

         ctx->Driver.UnmapTextureImage(ctx, texImage, slice + zoffset);

         /* advance to next slice */
         src += store.TotalBytesPerRow * (store.TotalRowsPerSlice - store.CopyRowsPerSlice);
      }
      else {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCompressedTexSubImage%uD",
                     dims);
      }
   }

   _mesa_unmap_teximage_pbo(ctx, &ctx->Unpack);
}

/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
 * Copyright (c) 2008 VMware, Inc.
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


/**
 * \file texcompress_s3tc.c
 * GL_EXT_texture_compression_s3tc support.
 */

#ifndef USE_EXTERNAL_DXTN_LIB
#define USE_EXTERNAL_DXTN_LIB 1
#endif

#include "glheader.h"
#include "imports.h"
#include "colormac.h"
#include "dlopen.h"
#include "image.h"
#include "macros.h"
#include "mtypes.h"
#include "texcompress.h"
#include "texcompress_s3tc.h"
#include "texstore.h"
#include "format_unpack.h"
#include "util/format_srgb.h"


#if defined(_WIN32) || defined(WIN32)
#define DXTN_LIBNAME "dxtn.dll"
#define RTLD_LAZY 0
#define RTLD_GLOBAL 0
#else
#define DXTN_LIBNAME "libtxc_dxtn.so"
#endif

typedef void (*dxtFetchTexelFuncExt)( GLint srcRowstride, const GLubyte *pixdata, GLint col, GLint row, GLvoid *texelOut );

static dxtFetchTexelFuncExt fetch_ext_rgb_dxt1 = NULL;
static dxtFetchTexelFuncExt fetch_ext_rgba_dxt1 = NULL;
static dxtFetchTexelFuncExt fetch_ext_rgba_dxt3 = NULL;
static dxtFetchTexelFuncExt fetch_ext_rgba_dxt5 = NULL;

typedef void (*dxtCompressTexFuncExt)(GLint srccomps, GLint width,
                                      GLint height, const GLubyte *srcPixData,
                                      GLenum destformat, GLubyte *dest,
                                      GLint dstRowStride);

static dxtCompressTexFuncExt ext_tx_compress_dxtn = NULL;

static void *dxtlibhandle = NULL;


void
_mesa_init_texture_s3tc( struct gl_context *ctx )
{
   /* called during context initialization */
   ctx->Mesa_DXTn = GL_FALSE;
#if USE_EXTERNAL_DXTN_LIB
   if (!dxtlibhandle) {
      dxtlibhandle = _mesa_dlopen(DXTN_LIBNAME, 0);
      if (!dxtlibhandle) {
	 _mesa_warning(ctx, "couldn't open " DXTN_LIBNAME ", software DXTn "
	    "compression/decompression unavailable");
      }
      else {
         /* the fetch functions are not per context! Might be problematic... */
         fetch_ext_rgb_dxt1 = (dxtFetchTexelFuncExt)
            _mesa_dlsym(dxtlibhandle, "fetch_2d_texel_rgb_dxt1");
         fetch_ext_rgba_dxt1 = (dxtFetchTexelFuncExt)
            _mesa_dlsym(dxtlibhandle, "fetch_2d_texel_rgba_dxt1");
         fetch_ext_rgba_dxt3 = (dxtFetchTexelFuncExt)
            _mesa_dlsym(dxtlibhandle, "fetch_2d_texel_rgba_dxt3");
         fetch_ext_rgba_dxt5 = (dxtFetchTexelFuncExt)
            _mesa_dlsym(dxtlibhandle, "fetch_2d_texel_rgba_dxt5");
         ext_tx_compress_dxtn = (dxtCompressTexFuncExt)
            _mesa_dlsym(dxtlibhandle, "tx_compress_dxtn");

         if (!fetch_ext_rgb_dxt1 ||
             !fetch_ext_rgba_dxt1 ||
             !fetch_ext_rgba_dxt3 ||
             !fetch_ext_rgba_dxt5 ||
             !ext_tx_compress_dxtn) {
	    _mesa_warning(ctx, "couldn't reference all symbols in "
	       DXTN_LIBNAME ", software DXTn compression/decompression "
	       "unavailable");
            fetch_ext_rgb_dxt1 = NULL;
            fetch_ext_rgba_dxt1 = NULL;
            fetch_ext_rgba_dxt3 = NULL;
            fetch_ext_rgba_dxt5 = NULL;
            ext_tx_compress_dxtn = NULL;
            _mesa_dlclose(dxtlibhandle);
            dxtlibhandle = NULL;
         }
      }
   }
   if (dxtlibhandle) {
      ctx->Mesa_DXTn = GL_TRUE;
   }
#else
   (void) ctx;
#endif
}

/**
 * Store user's image in rgb_dxt1 format.
 */
GLboolean
_mesa_texstore_rgb_dxt1(TEXSTORE_PARAMS)
{
   const GLubyte *pixels;
   GLubyte *dst;
   const GLubyte *tempImage = NULL;

   ASSERT(dstFormat == MESA_FORMAT_RGB_DXT1 ||
          dstFormat == MESA_FORMAT_SRGB_DXT1);

   if (srcFormat != GL_RGB ||
       srcType != GL_UNSIGNED_BYTE ||
       ctx->_ImageTransferState ||
       srcPacking->RowLength != srcWidth ||
       srcPacking->SwapBytes) {
      /* convert image to RGB/GLubyte */
      tempImage = _mesa_make_temp_ubyte_image(ctx, dims,
                                             baseInternalFormat,
                                             _mesa_get_format_base_format(dstFormat),
                                             srcWidth, srcHeight, srcDepth,
                                             srcFormat, srcType, srcAddr,
                                             srcPacking);
      if (!tempImage)
         return GL_FALSE; /* out of memory */
      pixels = tempImage;
      srcFormat = GL_RGB;
   }
   else {
      pixels = _mesa_image_address2d(srcPacking, srcAddr, srcWidth, srcHeight,
                                     srcFormat, srcType, 0, 0);
   }

   dst = dstSlices[0];

   if (ext_tx_compress_dxtn) {
      (*ext_tx_compress_dxtn)(3, srcWidth, srcHeight, pixels,
                              GL_COMPRESSED_RGB_S3TC_DXT1_EXT,
                              dst, dstRowStride);
   }
   else {
      _mesa_warning(ctx, "external dxt library not available: texstore_rgb_dxt1");
   }

   free((void *) tempImage);

   return GL_TRUE;
}


/**
 * Store user's image in rgba_dxt1 format.
 */
GLboolean
_mesa_texstore_rgba_dxt1(TEXSTORE_PARAMS)
{
   const GLubyte *pixels;
   GLubyte *dst;
   const GLubyte *tempImage = NULL;

   ASSERT(dstFormat == MESA_FORMAT_RGBA_DXT1 ||
          dstFormat == MESA_FORMAT_SRGBA_DXT1);

   if (srcFormat != GL_RGBA ||
       srcType != GL_UNSIGNED_BYTE ||
       ctx->_ImageTransferState ||
       srcPacking->RowLength != srcWidth ||
       srcPacking->SwapBytes) {
      /* convert image to RGBA/GLubyte */
      tempImage = _mesa_make_temp_ubyte_image(ctx, dims,
                                             baseInternalFormat,
                                             _mesa_get_format_base_format(dstFormat),
                                             srcWidth, srcHeight, srcDepth,
                                             srcFormat, srcType, srcAddr,
                                             srcPacking);
      if (!tempImage)
         return GL_FALSE; /* out of memory */
      pixels = tempImage;
      srcFormat = GL_RGBA;
   }
   else {
      pixels = _mesa_image_address2d(srcPacking, srcAddr, srcWidth, srcHeight,
                                     srcFormat, srcType, 0, 0);
   }

   dst = dstSlices[0];

   if (ext_tx_compress_dxtn) {
      (*ext_tx_compress_dxtn)(4, srcWidth, srcHeight, pixels,
                              GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,
                              dst, dstRowStride);
   }
   else {
      _mesa_warning(ctx, "external dxt library not available: texstore_rgba_dxt1");
   }

   free((void*) tempImage);

   return GL_TRUE;
}


/**
 * Store user's image in rgba_dxt3 format.
 */
GLboolean
_mesa_texstore_rgba_dxt3(TEXSTORE_PARAMS)
{
   const GLubyte *pixels;
   GLubyte *dst;
   const GLubyte *tempImage = NULL;

   ASSERT(dstFormat == MESA_FORMAT_RGBA_DXT3 ||
          dstFormat == MESA_FORMAT_SRGBA_DXT3);

   if (srcFormat != GL_RGBA ||
       srcType != GL_UNSIGNED_BYTE ||
       ctx->_ImageTransferState ||
       srcPacking->RowLength != srcWidth ||
       srcPacking->SwapBytes) {
      /* convert image to RGBA/GLubyte */
      tempImage = _mesa_make_temp_ubyte_image(ctx, dims,
                                             baseInternalFormat,
                                             _mesa_get_format_base_format(dstFormat),
                                             srcWidth, srcHeight, srcDepth,
                                             srcFormat, srcType, srcAddr,
                                             srcPacking);
      if (!tempImage)
         return GL_FALSE; /* out of memory */
      pixels = tempImage;
   }
   else {
      pixels = _mesa_image_address2d(srcPacking, srcAddr, srcWidth, srcHeight,
                                     srcFormat, srcType, 0, 0);
   }

   dst = dstSlices[0];

   if (ext_tx_compress_dxtn) {
      (*ext_tx_compress_dxtn)(4, srcWidth, srcHeight, pixels,
                              GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,
                              dst, dstRowStride);
   }
   else {
      _mesa_warning(ctx, "external dxt library not available: texstore_rgba_dxt3");
   }

   free((void *) tempImage);

   return GL_TRUE;
}


/**
 * Store user's image in rgba_dxt5 format.
 */
GLboolean
_mesa_texstore_rgba_dxt5(TEXSTORE_PARAMS)
{
   const GLubyte *pixels;
   GLubyte *dst;
   const GLubyte *tempImage = NULL;

   ASSERT(dstFormat == MESA_FORMAT_RGBA_DXT5 ||
          dstFormat == MESA_FORMAT_SRGBA_DXT5);

   if (srcFormat != GL_RGBA ||
       srcType != GL_UNSIGNED_BYTE ||
       ctx->_ImageTransferState ||
       srcPacking->RowLength != srcWidth ||
       srcPacking->SwapBytes) {
      /* convert image to RGBA/GLubyte */
      tempImage = _mesa_make_temp_ubyte_image(ctx, dims,
                                             baseInternalFormat,
                                   	     _mesa_get_format_base_format(dstFormat),
                                             srcWidth, srcHeight, srcDepth,
                                             srcFormat, srcType, srcAddr,
                                             srcPacking);
      if (!tempImage)
         return GL_FALSE; /* out of memory */
      pixels = tempImage;
   }
   else {
      pixels = _mesa_image_address2d(srcPacking, srcAddr, srcWidth, srcHeight,
                                     srcFormat, srcType, 0, 0);
   }

   dst = dstSlices[0];

   if (ext_tx_compress_dxtn) {
      (*ext_tx_compress_dxtn)(4, srcWidth, srcHeight, pixels,
                              GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,
                              dst, dstRowStride);
   }
   else {
      _mesa_warning(ctx, "external dxt library not available: texstore_rgba_dxt5");
   }

   free((void *) tempImage);

   return GL_TRUE;
}


/** Report problem with dxt texture decompression, once */
static void
problem(const char *func)
{
   static GLboolean warned = GL_FALSE;
   if (!warned) {
      _mesa_debug(NULL, "attempted to decode DXT texture without "
                  "library available: %s\n", func);
      warned = GL_TRUE;
   }
}


static void
fetch_rgb_dxt1(const GLubyte *map,
               GLint rowStride, GLint i, GLint j, GLfloat *texel)
{
   if (fetch_ext_rgb_dxt1) {
      GLubyte tex[4];
      fetch_ext_rgb_dxt1(rowStride, map, i, j, tex);
      texel[RCOMP] = UBYTE_TO_FLOAT(tex[RCOMP]);
      texel[GCOMP] = UBYTE_TO_FLOAT(tex[GCOMP]);
      texel[BCOMP] = UBYTE_TO_FLOAT(tex[BCOMP]);
      texel[ACOMP] = UBYTE_TO_FLOAT(tex[ACOMP]);
   }
   else {
      problem("rgb_dxt1");
   }
}

static void
fetch_rgba_dxt1(const GLubyte *map,
                GLint rowStride, GLint i, GLint j, GLfloat *texel)
{
   if (fetch_ext_rgba_dxt1) {
      GLubyte tex[4];
      fetch_ext_rgba_dxt1(rowStride, map, i, j, tex);
      texel[RCOMP] = UBYTE_TO_FLOAT(tex[RCOMP]);
      texel[GCOMP] = UBYTE_TO_FLOAT(tex[GCOMP]);
      texel[BCOMP] = UBYTE_TO_FLOAT(tex[BCOMP]);
      texel[ACOMP] = UBYTE_TO_FLOAT(tex[ACOMP]);
   }
   else {
      problem("rgba_dxt1");
   }
}

static void
fetch_rgba_dxt3(const GLubyte *map,
                GLint rowStride, GLint i, GLint j, GLfloat *texel)
{
   if (fetch_ext_rgba_dxt3) {
      GLubyte tex[4];
      fetch_ext_rgba_dxt3(rowStride, map, i, j, tex);
      texel[RCOMP] = UBYTE_TO_FLOAT(tex[RCOMP]);
      texel[GCOMP] = UBYTE_TO_FLOAT(tex[GCOMP]);
      texel[BCOMP] = UBYTE_TO_FLOAT(tex[BCOMP]);
      texel[ACOMP] = UBYTE_TO_FLOAT(tex[ACOMP]);
   }
   else {
      problem("rgba_dxt3");
   }
}

static void
fetch_rgba_dxt5(const GLubyte *map,
                GLint rowStride, GLint i, GLint j, GLfloat *texel)
{
   if (fetch_ext_rgba_dxt5) {
      GLubyte tex[4];
      fetch_ext_rgba_dxt5(rowStride, map, i, j, tex);
      texel[RCOMP] = UBYTE_TO_FLOAT(tex[RCOMP]);
      texel[GCOMP] = UBYTE_TO_FLOAT(tex[GCOMP]);
      texel[BCOMP] = UBYTE_TO_FLOAT(tex[BCOMP]);
      texel[ACOMP] = UBYTE_TO_FLOAT(tex[ACOMP]);
   }
   else {
      problem("rgba_dxt5");
   }
}


static void
fetch_srgb_dxt1(const GLubyte *map,
                GLint rowStride, GLint i, GLint j, GLfloat *texel)
{
   if (fetch_ext_rgb_dxt1) {
      GLubyte tex[4];
      fetch_ext_rgb_dxt1(rowStride, map, i, j, tex);
      texel[RCOMP] = util_format_srgb_8unorm_to_linear_float(tex[RCOMP]);
      texel[GCOMP] = util_format_srgb_8unorm_to_linear_float(tex[GCOMP]);
      texel[BCOMP] = util_format_srgb_8unorm_to_linear_float(tex[BCOMP]);
      texel[ACOMP] = UBYTE_TO_FLOAT(tex[ACOMP]);
   }
   else {
      problem("srgb_dxt1");
   }
}

static void
fetch_srgba_dxt1(const GLubyte *map,
                 GLint rowStride, GLint i, GLint j, GLfloat *texel)
{
   if (fetch_ext_rgba_dxt1) {
      GLubyte tex[4];
      fetch_ext_rgba_dxt1(rowStride, map, i, j, tex);
      texel[RCOMP] = util_format_srgb_8unorm_to_linear_float(tex[RCOMP]);
      texel[GCOMP] = util_format_srgb_8unorm_to_linear_float(tex[GCOMP]);
      texel[BCOMP] = util_format_srgb_8unorm_to_linear_float(tex[BCOMP]);
      texel[ACOMP] = UBYTE_TO_FLOAT(tex[ACOMP]);
   }
   else {
      problem("srgba_dxt1");
   }
}

static void
fetch_srgba_dxt3(const GLubyte *map,
                 GLint rowStride, GLint i, GLint j, GLfloat *texel)
{
   if (fetch_ext_rgba_dxt3) {
      GLubyte tex[4];
      fetch_ext_rgba_dxt3(rowStride, map, i, j, tex);
      texel[RCOMP] = util_format_srgb_8unorm_to_linear_float(tex[RCOMP]);
      texel[GCOMP] = util_format_srgb_8unorm_to_linear_float(tex[GCOMP]);
      texel[BCOMP] = util_format_srgb_8unorm_to_linear_float(tex[BCOMP]);
      texel[ACOMP] = UBYTE_TO_FLOAT(tex[ACOMP]);
   }
   else {
      problem("srgba_dxt3");
   }
}

static void
fetch_srgba_dxt5(const GLubyte *map,
                 GLint rowStride, GLint i, GLint j, GLfloat *texel)
{
   if (fetch_ext_rgba_dxt5) {
      GLubyte tex[4];
      fetch_ext_rgba_dxt5(rowStride, map, i, j, tex);
      texel[RCOMP] = util_format_srgb_8unorm_to_linear_float(tex[RCOMP]);
      texel[GCOMP] = util_format_srgb_8unorm_to_linear_float(tex[GCOMP]);
      texel[BCOMP] = util_format_srgb_8unorm_to_linear_float(tex[BCOMP]);
      texel[ACOMP] = UBYTE_TO_FLOAT(tex[ACOMP]);
   }
   else {
      problem("srgba_dxt5");
   }
}



compressed_fetch_func
_mesa_get_dxt_fetch_func(mesa_format format)
{
   switch (format) {
   case MESA_FORMAT_RGB_DXT1:
      return fetch_rgb_dxt1;
   case MESA_FORMAT_RGBA_DXT1:
      return fetch_rgba_dxt1;
   case MESA_FORMAT_RGBA_DXT3:
      return fetch_rgba_dxt3;
   case MESA_FORMAT_RGBA_DXT5:
      return fetch_rgba_dxt5;
   case MESA_FORMAT_SRGB_DXT1:
      return fetch_srgb_dxt1;
   case MESA_FORMAT_SRGBA_DXT1:
      return fetch_srgba_dxt1;
   case MESA_FORMAT_SRGBA_DXT3:
      return fetch_srgba_dxt3;
   case MESA_FORMAT_SRGBA_DXT5:
      return fetch_srgba_dxt5;
   default:
      return NULL;
   }
}

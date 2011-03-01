/*
 * Copyright (C) 2011 Red Hat Inc.
 * 
 * block compression parts are:
 * Copyright (C) 2004  Roland Scheidegger   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author:
 *    Dave Airlie
 */

/**
 * \file texcompress_rgtc.c
 * GL_EXT_texture_compression_rgtc support.
 */


#include "glheader.h"
#include "imports.h"
#include "colormac.h"
#include "image.h"
#include "macros.h"
#include "mfeatures.h"
#include "mipmap.h"
#include "texcompress.h"
#include "texcompress_rgtc.h"
#include "texstore.h"

#define RGTC_DEBUG 0

static void unsigned_encode_rgtc_chan(GLubyte *blkaddr, GLubyte srccolors[4][4],
					GLint numxpixels, GLint numypixels);
static void signed_encode_rgtc_chan(GLbyte *blkaddr, GLbyte srccolors[4][4],
			     GLint numxpixels, GLint numypixels);

static void extractsrc_u( GLubyte srcpixels[4][4], const GLchan *srcaddr,
			  GLint srcRowStride, GLint numxpixels, GLint numypixels, GLint comps)
{
   GLubyte i, j;
   const GLchan *curaddr;
   for (j = 0; j < numypixels; j++) {
      curaddr = srcaddr + j * srcRowStride * comps;
      for (i = 0; i < numxpixels; i++) {
	 srcpixels[j][i] = *curaddr / (CHAN_MAX / 255);
	 curaddr += comps;
      }
   }
}

static void extractsrc_s( GLbyte srcpixels[4][4], const GLfloat *srcaddr,
			  GLint srcRowStride, GLint numxpixels, GLint numypixels, GLint comps)
{
   GLubyte i, j;
   const GLfloat *curaddr;
   for (j = 0; j < numypixels; j++) {
      curaddr = srcaddr + j * srcRowStride * comps;
      for (i = 0; i < numxpixels; i++) {
	 srcpixels[j][i] = FLOAT_TO_BYTE_TEX(*curaddr);
	 curaddr += comps;
      }
   }
}


GLboolean
_mesa_texstore_red_rgtc1(TEXSTORE_PARAMS)
{
   GLubyte *dst;
   const GLint texWidth = dstRowStride * 4 / 8; /* a bit of a hack */
   const GLchan *tempImage = NULL;
   int i, j;
   int numxpixels, numypixels;
   const GLchan *srcaddr;
   GLubyte srcpixels[4][4];
   GLubyte *blkaddr;
   GLint dstRowDiff;
   ASSERT(dstFormat == MESA_FORMAT_RED_RGTC1);
   ASSERT(dstXoffset % 4 == 0);
   ASSERT(dstYoffset % 4 == 0);
   ASSERT(dstZoffset % 4 == 0);
   (void) dstZoffset;
   (void) dstImageOffsets;


   tempImage = _mesa_make_temp_chan_image(ctx, dims,
					  baseInternalFormat,
					  _mesa_get_format_base_format(dstFormat),
					  srcWidth, srcHeight, srcDepth,
					  srcFormat, srcType, srcAddr,
					  srcPacking);
   if (!tempImage)
      return GL_FALSE; /* out of memory */

   dst = _mesa_compressed_image_address(dstXoffset, dstYoffset, 0,
                                        dstFormat,
                                        texWidth, (GLubyte *) dstAddr);

   blkaddr = dst;
   dstRowDiff = dstRowStride >= (srcWidth * 4) ? dstRowStride - (((srcWidth + 3) & ~3) * 4) : 0;
   for (j = 0; j < srcHeight; j+=4) {
      if (srcHeight > j + 3) numypixels = 4;
      else numypixels = srcHeight - j;
      srcaddr = tempImage + j * srcWidth;
      for (i = 0; i < srcWidth; i += 4) {
	 if (srcWidth > i + 3) numxpixels = 4;
	 else numxpixels = srcWidth - i;
	 extractsrc_u(srcpixels, srcaddr, srcWidth, numxpixels, numypixels, 1);
	 unsigned_encode_rgtc_chan(blkaddr, srcpixels, numxpixels, numypixels);
	 srcaddr += numxpixels;
	 blkaddr += 8;
      }
      blkaddr += dstRowDiff;
   }
   if (tempImage)
      free((void *) tempImage);

   return GL_TRUE;
}

GLboolean
_mesa_texstore_signed_red_rgtc1(TEXSTORE_PARAMS)
{
   GLbyte *dst;
   const GLint texWidth = dstRowStride * 4 / 8; /* a bit of a hack */
   const GLfloat *tempImage = NULL;
   int i, j;
   int numxpixels, numypixels;
   const GLfloat *srcaddr;
   GLbyte srcpixels[4][4];
   GLbyte *blkaddr;
   GLint dstRowDiff;
   ASSERT(dstFormat == MESA_FORMAT_SIGNED_RED_RGTC1);
   ASSERT(dstXoffset % 4 == 0);
   ASSERT(dstYoffset % 4 == 0);
   ASSERT(dstZoffset % 4 == 0);
   (void) dstZoffset;
   (void) dstImageOffsets;

   tempImage = _mesa_make_temp_float_image(ctx, dims,
					   baseInternalFormat,
					   _mesa_get_format_base_format(dstFormat),
					   srcWidth, srcHeight, srcDepth,
					   srcFormat, srcType, srcAddr,
					   srcPacking, 0x0);
   if (!tempImage)
      return GL_FALSE; /* out of memory */

   dst = (GLbyte *)_mesa_compressed_image_address(dstXoffset, dstYoffset, 0,
						  dstFormat,
						  texWidth, (GLubyte *) dstAddr);

   blkaddr = dst;
   dstRowDiff = dstRowStride >= (srcWidth * 4) ? dstRowStride - (((srcWidth + 3) & ~3) * 4) : 0;
   for (j = 0; j < srcHeight; j+=4) {
      if (srcHeight > j + 3) numypixels = 4;
      else numypixels = srcHeight - j;
      srcaddr = tempImage + j * srcWidth;
      for (i = 0; i < srcWidth; i += 4) {
	 if (srcWidth > i + 3) numxpixels = 4;
	 else numxpixels = srcWidth - i;
	 extractsrc_s(srcpixels, srcaddr, srcWidth, numxpixels, numypixels, 1);
	 signed_encode_rgtc_chan(blkaddr, srcpixels, numxpixels, numypixels);
	 srcaddr += numxpixels;
	 blkaddr += 8;
      }
      blkaddr += dstRowDiff;
   }
   if (tempImage)
      free((void *) tempImage);

   return GL_TRUE;
}

GLboolean
_mesa_texstore_rg_rgtc2(TEXSTORE_PARAMS)
{
   GLubyte *dst;
   const GLint texWidth = dstRowStride * 4 / 16; /* a bit of a hack */
   const GLchan *tempImage = NULL;
   int i, j;
   int numxpixels, numypixels;
   const GLchan *srcaddr;
   GLubyte srcpixels[4][4];
   GLubyte *blkaddr;
   GLint dstRowDiff;

   ASSERT(dstFormat == MESA_FORMAT_RG_RGTC2);
   ASSERT(dstXoffset % 4 == 0);
   ASSERT(dstYoffset % 4 == 0);
   ASSERT(dstZoffset % 4 == 0);
   (void) dstZoffset;
   (void) dstImageOffsets;

   tempImage = _mesa_make_temp_chan_image(ctx, dims,
					  baseInternalFormat,
					  _mesa_get_format_base_format(dstFormat),
					  srcWidth, srcHeight, srcDepth,
					  srcFormat, srcType, srcAddr,
					  srcPacking);
   if (!tempImage)
      return GL_FALSE; /* out of memory */

   dst = _mesa_compressed_image_address(dstXoffset, dstYoffset, 0,
                                        dstFormat,
                                        texWidth, (GLubyte *) dstAddr);

   blkaddr = dst;
   dstRowDiff = dstRowStride >= (srcWidth * 8) ? dstRowStride - (((srcWidth + 7) & ~7) * 8) : 0;
   for (j = 0; j < srcHeight; j+=4) {
      if (srcHeight > j + 3) numypixels = 4;
      else numypixels = srcHeight - j;
      srcaddr = tempImage + j * srcWidth * 2;
      for (i = 0; i < srcWidth; i += 4) {
	 if (srcWidth > i + 3) numxpixels = 4;
	 else numxpixels = srcWidth - i;
	 extractsrc_u(srcpixels, srcaddr, srcWidth, numxpixels, numypixels, 2);
	 unsigned_encode_rgtc_chan(blkaddr, srcpixels, numxpixels, numypixels);

	 blkaddr += 8;
	 extractsrc_u(srcpixels, (GLchan *)srcaddr + 1, srcWidth, numxpixels, numypixels, 2);
	 unsigned_encode_rgtc_chan(blkaddr, srcpixels, numxpixels, numypixels);

	 blkaddr += 8;

	 srcaddr += numxpixels * 2;
      }
      blkaddr += dstRowDiff;
   }
   if (tempImage)
      free((void *) tempImage);

   return GL_TRUE;
}

GLboolean
_mesa_texstore_signed_rg_rgtc2(TEXSTORE_PARAMS)
{
   GLbyte *dst;
   const GLint texWidth = dstRowStride * 4 / 16; /* a bit of a hack */
   const GLfloat *tempImage = NULL;
   int i, j;
   int numxpixels, numypixels;
   const GLfloat *srcaddr;
   GLbyte srcpixels[4][4];
   GLbyte *blkaddr;
   GLint dstRowDiff;

   ASSERT(dstFormat == MESA_FORMAT_SIGNED_RG_RGTC2);
   ASSERT(dstXoffset % 4 == 0);
   ASSERT(dstYoffset % 4 == 0);
   ASSERT(dstZoffset % 4 == 0);
   (void) dstZoffset;
   (void) dstImageOffsets;

   tempImage = _mesa_make_temp_float_image(ctx, dims,
					   baseInternalFormat,
					   _mesa_get_format_base_format(dstFormat),
					   srcWidth, srcHeight, srcDepth,
					   srcFormat, srcType, srcAddr,
					   srcPacking, 0x0);
   if (!tempImage)
      return GL_FALSE; /* out of memory */

   dst = (GLbyte *)_mesa_compressed_image_address(dstXoffset, dstYoffset, 0,
						  dstFormat,
						  texWidth, (GLubyte *) dstAddr);

   blkaddr = dst;
   dstRowDiff = dstRowStride >= (srcWidth * 8) ? dstRowStride - (((srcWidth + 7) & ~7) * 8) : 0;
   for (j = 0; j < srcHeight; j += 4) {
      if (srcHeight > j + 3) numypixels = 4;
      else numypixels = srcHeight - j;
      srcaddr = tempImage + j * srcWidth * 2;
      for (i = 0; i < srcWidth; i += 4) {
	 if (srcWidth > i + 3) numxpixels = 4;
	 else numxpixels = srcWidth - i;

	 extractsrc_s(srcpixels, srcaddr, srcWidth, numxpixels, numypixels, 2);
	 signed_encode_rgtc_chan(blkaddr, srcpixels, numxpixels, numypixels);
	 blkaddr += 8;

	 extractsrc_s(srcpixels, srcaddr + 1, srcWidth, numxpixels, numypixels, 2);
	 signed_encode_rgtc_chan(blkaddr, srcpixels, numxpixels, numypixels);
	 blkaddr += 8;

	 srcaddr += numxpixels * 2;

      }
      blkaddr += dstRowDiff;
   }
   if (tempImage)
      free((void *) tempImage);

   return GL_TRUE;
}

static void _fetch_texel_rgtc_u(GLint srcRowStride, const GLubyte *pixdata,
				GLint i, GLint j, GLchan *value, int comps)
{
   GLchan decode;
   const GLubyte *blksrc = (pixdata + ((srcRowStride + 3) / 4 * (j / 4) + (i / 4)) * 8 * comps);
   const GLubyte alpha0 = blksrc[0];
   const GLubyte alpha1 = blksrc[1];
   const GLubyte bit_pos = ((j&3) * 4 + (i&3)) * 3;
   const GLubyte acodelow = blksrc[2 + bit_pos / 8];
   const GLubyte acodehigh = blksrc[3 + bit_pos / 8];
   const GLubyte code = (acodelow >> (bit_pos & 0x7) |
      (acodehigh  << (8 - (bit_pos & 0x7)))) & 0x7;

   if (code == 0)
      decode = UBYTE_TO_CHAN( alpha0 );
   else if (code == 1)
      decode = UBYTE_TO_CHAN( alpha1 );
   else if (alpha0 > alpha1)
      decode = UBYTE_TO_CHAN( ((alpha0 * (8 - code) + (alpha1 * (code - 1))) / 7) );
   else if (code < 6)
      decode = UBYTE_TO_CHAN( ((alpha0 * (6 - code) + (alpha1 * (code - 1))) / 5) );
   else if (code == 6)
      decode = 0;
   else
      decode = CHAN_MAX;

   *value = decode;
}


static void _fetch_texel_rgtc_s(GLint srcRowStride, const GLbyte *pixdata,
				GLint i, GLint j, GLbyte *value, int comps)
{
   GLbyte decode;
   const GLbyte *blksrc = (pixdata + ((srcRowStride + 3) / 4 * (j / 4) + (i / 4)) * 8 * comps);
   const GLbyte alpha0 = blksrc[0];
   const GLbyte alpha1 = blksrc[1];
   const GLbyte bit_pos = ((j&3) * 4 + (i&3)) * 3;
   const GLbyte acodelow = blksrc[2 + bit_pos / 8];
   const GLbyte acodehigh = blksrc[3 + bit_pos / 8];
   const GLbyte code = (acodelow >> (bit_pos & 0x7) |
      (acodehigh  << (8 - (bit_pos & 0x7)))) & 0x7;

   if (code == 0)
      decode = alpha0;
   else if (code == 1)
      decode = alpha1;
   else if (alpha0 > alpha1)
      decode = ((alpha0 * (8 - code) + (alpha1 * (code - 1))) / 7);
   else if (code < 6)
      decode = ((alpha0 * (6 - code) + (alpha1 * (code - 1))) / 5);
   else if (code == 6)
      decode = -128;
   else
      decode = 127;

   *value = decode;
}

void
_mesa_fetch_texel_2d_f_red_rgtc1(const struct gl_texture_image *texImage,
				 GLint i, GLint j, GLint k, GLfloat *texel)
{
   GLchan red;
   _fetch_texel_rgtc_u(texImage->RowStride, (GLubyte *)(texImage->Data),
		       i, j, &red, 1);
   texel[RCOMP] = CHAN_TO_FLOAT(red);
   texel[GCOMP] = 0.0;
   texel[BCOMP] = 0.0;
   texel[ACOMP] = 1.0;
}

void
_mesa_fetch_texel_2d_f_signed_red_rgtc1(const struct gl_texture_image *texImage,
					GLint i, GLint j, GLint k, GLfloat *texel)
{
   GLbyte red;
   _fetch_texel_rgtc_s(texImage->RowStride, (GLbyte *)(texImage->Data),
		       i, j, &red, 1);
   texel[RCOMP] = BYTE_TO_FLOAT_TEX(red);
   texel[GCOMP] = 0.0;
   texel[BCOMP] = 0.0;
   texel[ACOMP] = 1.0;
}

void
_mesa_fetch_texel_2d_f_rg_rgtc2(const struct gl_texture_image *texImage,
				 GLint i, GLint j, GLint k, GLfloat *texel)
{
   GLchan red, green;
   _fetch_texel_rgtc_u(texImage->RowStride, (GLubyte *)(texImage->Data),
		     i, j, &red, 2);
   _fetch_texel_rgtc_u(texImage->RowStride, (GLubyte *)(texImage->Data) + 8,
		     i, j, &green, 2);
   texel[RCOMP] = CHAN_TO_FLOAT(red);
   texel[GCOMP] = CHAN_TO_FLOAT(green);
   texel[BCOMP] = 0.0;
   texel[ACOMP] = 1.0;
}

void
_mesa_fetch_texel_2d_f_signed_rg_rgtc2(const struct gl_texture_image *texImage,
				       GLint i, GLint j, GLint k, GLfloat *texel)
{
   GLbyte red, green;
   _fetch_texel_rgtc_s(texImage->RowStride, (GLbyte *)(texImage->Data),
		     i, j, &red, 2);
   _fetch_texel_rgtc_s(texImage->RowStride, (GLbyte *)(texImage->Data) + 8,
		     i, j, &green, 2);
   texel[RCOMP] = BYTE_TO_FLOAT_TEX(red);
   texel[GCOMP] = BYTE_TO_FLOAT_TEX(green);
   texel[BCOMP] = 0.0;
   texel[ACOMP] = 1.0;
}

#define TAG(x) unsigned_##x

#define TYPE GLubyte
#define T_MIN 0
#define T_MAX 0xff

#include "texcompress_rgtc_tmp.h"

#undef TAG
#undef TYPE
#undef T_MIN
#undef T_MAX

#define TAG(x) signed_##x
#define TYPE GLbyte
#define T_MIN (GLbyte)-127
#define T_MAX (GLbyte)127

#include "texcompress_rgtc_tmp.h"

#undef TAG
#undef TYPE
#undef T_MIN
#undef T_MAX

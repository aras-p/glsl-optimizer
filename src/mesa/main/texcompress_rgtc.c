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

static void encode_rgtc_chan_u(GLubyte *blkaddr, GLubyte srccolors[4][4],
			     GLint numxpixels, GLint numypixels);
static void encode_rgtc_chan_s(GLbyte *blkaddr, GLbyte srccolors[4][4],
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
   const void *srcaddr;
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
	 encode_rgtc_chan_u(blkaddr, srcpixels, numxpixels, numypixels);
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
	 encode_rgtc_chan_s(blkaddr, srcpixels, numxpixels, numypixels);
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
   const void *srcaddr;
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
	 encode_rgtc_chan_u(blkaddr, srcpixels, numxpixels, numypixels);

	 blkaddr += 8;
	 extractsrc_u(srcpixels, (GLchan *)srcaddr + 1, srcWidth, numxpixels, numypixels, 2);
	 encode_rgtc_chan_u(blkaddr, srcpixels, numxpixels, numypixels);

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
	 encode_rgtc_chan_s(blkaddr, srcpixels, numxpixels, numypixels);
	 blkaddr += 8;

	 extractsrc_s(srcpixels, srcaddr + 1, srcWidth, numxpixels, numypixels, 2);
	 encode_rgtc_chan_s(blkaddr, srcpixels, numxpixels, numypixels);
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

static void write_rgtc_encoded_channel(GLubyte *blkaddr,
				       GLubyte alphabase1,
				       GLubyte alphabase2,
				       GLubyte alphaenc[16])
{
   *blkaddr++ = alphabase1;
   *blkaddr++ = alphabase2;
   *blkaddr++ = alphaenc[0] | (alphaenc[1] << 3) | ((alphaenc[2] & 3) << 6);
   *blkaddr++ = (alphaenc[2] >> 2) | (alphaenc[3] << 1) | (alphaenc[4] << 4) | ((alphaenc[5] & 1) << 7);
   *blkaddr++ = (alphaenc[5] >> 1) | (alphaenc[6] << 2) | (alphaenc[7] << 5);
   *blkaddr++ = alphaenc[8] | (alphaenc[9] << 3) | ((alphaenc[10] & 3) << 6);
   *blkaddr++ = (alphaenc[10] >> 2) | (alphaenc[11] << 1) | (alphaenc[12] << 4) | ((alphaenc[13] & 1) << 7);
   *blkaddr++ = (alphaenc[13] >> 1) | (alphaenc[14] << 2) | (alphaenc[15] << 5);
}

static void encode_rgtc_chan_u(GLubyte *blkaddr, GLubyte srccolors[4][4],
			     GLint numxpixels, GLint numypixels)
{
   GLubyte alphabase[2], alphause[2];
   GLshort alphatest[2] = { 0 };
   GLuint alphablockerror1, alphablockerror2, alphablockerror3;
   GLubyte i, j, aindex, acutValues[7];
   GLubyte alphaenc1[16], alphaenc2[16], alphaenc3[16];
   GLboolean alphaabsmin = GL_FALSE;
   GLboolean alphaabsmax = GL_FALSE;
   GLshort alphadist;

   /* find lowest and highest alpha value in block, alphabase[0] lowest, alphabase[1] highest */
   alphabase[0] = 0xff; alphabase[1] = 0x0;
   for (j = 0; j < numypixels; j++) {
      for (i = 0; i < numxpixels; i++) {
         if (srccolors[j][i] == 0)
            alphaabsmin = GL_TRUE;
         else if (srccolors[j][i] == 255)
            alphaabsmax = GL_TRUE;
         else {
            if (srccolors[j][i] > alphabase[1])
               alphabase[1] = srccolors[j][i];
            if (srccolors[j][i] < alphabase[0])
               alphabase[0] = srccolors[j][i];
         }
      }
   }


   if ((alphabase[0] > alphabase[1]) && !(alphaabsmin && alphaabsmax)) { /* one color, either max or min */
      /* shortcut here since it is a very common case (and also avoids later problems) */
      /* || (alphabase[0] == alphabase[1] && !alphaabsmin && !alphaabsmax) */
      /* could also thest for alpha0 == alpha1 (and not min/max), but probably not common, so don't bother */

      *blkaddr++ = srccolors[0][0];
      blkaddr++;
      *blkaddr++ = 0;
      *blkaddr++ = 0;
      *blkaddr++ = 0;
      *blkaddr++ = 0;
      *blkaddr++ = 0;
      *blkaddr++ = 0;
#if RGTC_DEBUG
      fprintf(stderr, "enc0 used\n");
#endif
      return;
   }

   /* find best encoding for alpha0 > alpha1 */
   /* it's possible this encoding is better even if both alphaabsmin and alphaabsmax are true */
   alphablockerror1 = 0x0;
   alphablockerror2 = 0xffffffff;
   alphablockerror3 = 0xffffffff;
   if (alphaabsmin) alphause[0] = 0;
   else alphause[0] = alphabase[0];
   if (alphaabsmax) alphause[1] = 255;
   else alphause[1] = alphabase[1];
   /* calculate the 7 cut values, just the middle between 2 of the computed alpha values */
   for (aindex = 0; aindex < 7; aindex++) {
      /* don't forget here is always rounded down */
      acutValues[aindex] = (alphause[0] * (2*aindex + 1) + alphause[1] * (14 - (2*aindex + 1))) / 14;
   }

   for (j = 0; j < numypixels; j++) {
      for (i = 0; i < numxpixels; i++) {
         /* maybe it's overkill to have the most complicated calculation just for the error
            calculation which we only need to figure out if encoding1 or encoding2 is better... */
         if (srccolors[j][i] > acutValues[0]) {
            alphaenc1[4*j + i] = 0;
            alphadist = srccolors[j][i] - alphause[1];
         }
         else if (srccolors[j][i] > acutValues[1]) {
            alphaenc1[4*j + i] = 2;
            alphadist = srccolors[j][i] - (alphause[1] * 6 + alphause[0] * 1) / 7;
         }
         else if (srccolors[j][i] > acutValues[2]) {
            alphaenc1[4*j + i] = 3;
            alphadist = srccolors[j][i] - (alphause[1] * 5 + alphause[0] * 2) / 7;
         }
         else if (srccolors[j][i] > acutValues[3]) {
            alphaenc1[4*j + i] = 4;
            alphadist = srccolors[j][i] - (alphause[1] * 4 + alphause[0] * 3) / 7;
         }
         else if (srccolors[j][i] > acutValues[4]) {
            alphaenc1[4*j + i] = 5;
            alphadist = srccolors[j][i] - (alphause[1] * 3 + alphause[0] * 4) / 7;
         }
         else if (srccolors[j][i] > acutValues[5]) {
            alphaenc1[4*j + i] = 6;
            alphadist = srccolors[j][i] - (alphause[1] * 2 + alphause[0] * 5) / 7;
         }
         else if (srccolors[j][i] > acutValues[6]) {
            alphaenc1[4*j + i] = 7;
            alphadist = srccolors[j][i] - (alphause[1] * 1 + alphause[0] * 6) / 7;
         }
         else {
            alphaenc1[4*j + i] = 1;
            alphadist = srccolors[j][i] - alphause[0];
         }
         alphablockerror1 += alphadist * alphadist;
      }
   }

#if RGTC_DEBUG
   for (i = 0; i < 16; i++) {
      fprintf(stderr, "%d ", alphaenc1[i]);
   }
   fprintf(stderr, "cutVals ");
   for (i = 0; i < 8; i++) {
      fprintf(stderr, "%d ", acutValues[i]);
   }
   fprintf(stderr, "srcVals ");
   for (j = 0; j < numypixels; j++) {
      for (i = 0; i < numxpixels; i++) {
	 fprintf(stderr, "%d ", srccolors[j][i]);
      }
   }
   fprintf(stderr, "\n");
#endif

   /* it's not very likely this encoding is better if both alphaabsmin and alphaabsmax
      are false but try it anyway */
   if (alphablockerror1 >= 32) {

      /* don't bother if encoding is already very good, this condition should also imply
      we have valid alphabase colors which we absolutely need (alphabase[0] <= alphabase[1]) */
      alphablockerror2 = 0;
      for (aindex = 0; aindex < 5; aindex++) {
         /* don't forget here is always rounded down */
         acutValues[aindex] = (alphabase[0] * (10 - (2*aindex + 1)) + alphabase[1] * (2*aindex + 1)) / 10;
      }
      for (j = 0; j < numypixels; j++) {
         for (i = 0; i < numxpixels; i++) {
             /* maybe it's overkill to have the most complicated calculation just for the error
               calculation which we only need to figure out if encoding1 or encoding2 is better... */
            if (srccolors[j][i] == 0) {
               alphaenc2[4*j + i] = 6;
               alphadist = 0;
            }
            else if (srccolors[j][i] == 255) {
               alphaenc2[4*j + i] = 7;
               alphadist = 0;
            }
            else if (srccolors[j][i] <= acutValues[0]) {
               alphaenc2[4*j + i] = 0;
               alphadist = srccolors[j][i] - alphabase[0];
            }
            else if (srccolors[j][i] <= acutValues[1]) {
               alphaenc2[4*j + i] = 2;
               alphadist = srccolors[j][i] - (alphabase[0] * 4 + alphabase[1] * 1) / 5;
            }
            else if (srccolors[j][i] <= acutValues[2]) {
               alphaenc2[4*j + i] = 3;
               alphadist = srccolors[j][i] - (alphabase[0] * 3 + alphabase[1] * 2) / 5;
            }
            else if (srccolors[j][i] <= acutValues[3]) {
               alphaenc2[4*j + i] = 4;
               alphadist = srccolors[j][i] - (alphabase[0] * 2 + alphabase[1] * 3) / 5;
            }
            else if (srccolors[j][i] <= acutValues[4]) {
               alphaenc2[4*j + i] = 5;
               alphadist = srccolors[j][i] - (alphabase[0] * 1 + alphabase[1] * 4) / 5;
            }
            else {
               alphaenc2[4*j + i] = 1;
               alphadist = srccolors[j][i] - alphabase[1];
            }
            alphablockerror2 += alphadist * alphadist;
         }
      }


      /* skip this if the error is already very small
         this encoding is MUCH better on average than #2 though, but expensive! */
      if ((alphablockerror2 > 96) && (alphablockerror1 > 96)) {
         GLshort blockerrlin1 = 0;
         GLshort blockerrlin2 = 0;
         GLubyte nralphainrangelow = 0;
         GLubyte nralphainrangehigh = 0;
         alphatest[0] = 0xff;
         alphatest[1] = 0x0;
         /* if we have large range it's likely there are values close to 0/255, try to map them to 0/255 */
         for (j = 0; j < numypixels; j++) {
            for (i = 0; i < numxpixels; i++) {
               if ((srccolors[j][i] > alphatest[1]) && (srccolors[j][i] < (255 -(alphabase[1] - alphabase[0]) / 28)))
                  alphatest[1] = srccolors[j][i];
               if ((srccolors[j][i] < alphatest[0]) && (srccolors[j][i] > (alphabase[1] - alphabase[0]) / 28))
                  alphatest[0] = srccolors[j][i];
            }
         }
          /* shouldn't happen too often, don't really care about those degenerated cases */
          if (alphatest[1] <= alphatest[0]) {
             alphatest[0] = 1;
             alphatest[1] = 254;
         }
         for (aindex = 0; aindex < 5; aindex++) {
         /* don't forget here is always rounded down */
            acutValues[aindex] = (alphatest[0] * (10 - (2*aindex + 1)) + alphatest[1] * (2*aindex + 1)) / 10;
         }

         /* find the "average" difference between the alpha values and the next encoded value.
            This is then used to calculate new base values.
            Should there be some weighting, i.e. those values closer to alphatest[x] have more weight,
            since they will see more improvement, and also because the values in the middle are somewhat
            likely to get no improvement at all (because the base values might move in different directions)?
            OTOH it would mean the values in the middle are even less likely to get an improvement
         */
         for (j = 0; j < numypixels; j++) {
            for (i = 0; i < numxpixels; i++) {
               if (srccolors[j][i] <= alphatest[0] / 2) {
               }
               else if (srccolors[j][i] > ((255 + alphatest[1]) / 2)) {
               }
               else if (srccolors[j][i] <= acutValues[0]) {
                  blockerrlin1 += (srccolors[j][i] - alphatest[0]);
                  nralphainrangelow += 1;
               }
               else if (srccolors[j][i] <= acutValues[1]) {
                  blockerrlin1 += (srccolors[j][i] - (alphatest[0] * 4 + alphatest[1] * 1) / 5);
                  blockerrlin2 += (srccolors[j][i] - (alphatest[0] * 4 + alphatest[1] * 1) / 5);
                  nralphainrangelow += 1;
                  nralphainrangehigh += 1;
               }
               else if (srccolors[j][i] <= acutValues[2]) {
                  blockerrlin1 += (srccolors[j][i] - (alphatest[0] * 3 + alphatest[1] * 2) / 5);
                  blockerrlin2 += (srccolors[j][i] - (alphatest[0] * 3 + alphatest[1] * 2) / 5);
                  nralphainrangelow += 1;
                  nralphainrangehigh += 1;
               }
               else if (srccolors[j][i] <= acutValues[3]) {
                  blockerrlin1 += (srccolors[j][i] - (alphatest[0] * 2 + alphatest[1] * 3) / 5);
                  blockerrlin2 += (srccolors[j][i] - (alphatest[0] * 2 + alphatest[1] * 3) / 5);
                  nralphainrangelow += 1;
                  nralphainrangehigh += 1;
               }
               else if (srccolors[j][i] <= acutValues[4]) {
                  blockerrlin1 += (srccolors[j][i] - (alphatest[0] * 1 + alphatest[1] * 4) / 5);
                  blockerrlin2 += (srccolors[j][i] - (alphatest[0] * 1 + alphatest[1] * 4) / 5);
                  nralphainrangelow += 1;
                  nralphainrangehigh += 1;
                  }
               else {
                  blockerrlin2 += (srccolors[j][i] - alphatest[1]);
                  nralphainrangehigh += 1;
               }
            }
         }
         /* shouldn't happen often, needed to avoid div by zero */
         if (nralphainrangelow == 0) nralphainrangelow = 1;
         if (nralphainrangehigh == 0) nralphainrangehigh = 1;
         alphatest[0] = alphatest[0] + (blockerrlin1 / nralphainrangelow);
#if RGTC_DEBUG
         fprintf(stderr, "block err lin low %d, nr %d\n", blockerrlin1, nralphainrangelow);
         fprintf(stderr, "block err lin high %d, nr %d\n", blockerrlin2, nralphainrangehigh);
#endif
         /* again shouldn't really happen often... */
         if (alphatest[0] < 0) {
            alphatest[0] = 0;
         }
         alphatest[1] = alphatest[1] + (blockerrlin2 / nralphainrangehigh);
         if (alphatest[1] > 255) {
            alphatest[1] = 255;
         }

         alphablockerror3 = 0;
         for (aindex = 0; aindex < 5; aindex++) {
         /* don't forget here is always rounded down */
            acutValues[aindex] = (alphatest[0] * (10 - (2*aindex + 1)) + alphatest[1] * (2*aindex + 1)) / 10;
         }
         for (j = 0; j < numypixels; j++) {
            for (i = 0; i < numxpixels; i++) {
                /* maybe it's overkill to have the most complicated calculation just for the error
                  calculation which we only need to figure out if encoding1 or encoding2 is better... */
               if (srccolors[j][i] <= alphatest[0] / 2) {
                  alphaenc3[4*j + i] = 6;
                  alphadist = srccolors[j][i];
               }
               else if (srccolors[j][i] > ((255 + alphatest[1]) / 2)) {
                  alphaenc3[4*j + i] = 7;
                  alphadist = 255 - srccolors[j][i];
               }
               else if (srccolors[j][i] <= acutValues[0]) {
                  alphaenc3[4*j + i] = 0;
                  alphadist = srccolors[j][i] - alphatest[0];
               }
               else if (srccolors[j][i] <= acutValues[1]) {
                 alphaenc3[4*j + i] = 2;
                 alphadist = srccolors[j][i] - (alphatest[0] * 4 + alphatest[1] * 1) / 5;
               }
               else if (srccolors[j][i] <= acutValues[2]) {
                  alphaenc3[4*j + i] = 3;
                  alphadist = srccolors[j][i] - (alphatest[0] * 3 + alphatest[1] * 2) / 5;
               }
               else if (srccolors[j][i] <= acutValues[3]) {
                  alphaenc3[4*j + i] = 4;
                  alphadist = srccolors[j][i] - (alphatest[0] * 2 + alphatest[1] * 3) / 5;
               }
               else if (srccolors[j][i] <= acutValues[4]) {
                  alphaenc3[4*j + i] = 5;
                  alphadist = srccolors[j][i] - (alphatest[0] * 1 + alphatest[1] * 4) / 5;
               }
               else {
                  alphaenc3[4*j + i] = 1;
                  alphadist = srccolors[j][i] - alphatest[1];
               }
               alphablockerror3 += alphadist * alphadist;
            }
         }
      }
   }
  /* write the alpha values and encoding back. */
   if ((alphablockerror1 <= alphablockerror2) && (alphablockerror1 <= alphablockerror3)) {
#if RGTC_DEBUG
      if (alphablockerror1 > 96) fprintf(stderr, "enc1 used, error %d\n", alphablockerror1);
#endif
      write_rgtc_encoded_channel( blkaddr, alphause[1], alphause[0], alphaenc1 );
   }
   else if (alphablockerror2 <= alphablockerror3) {
#if RGTC_DEBUG
      if (alphablockerror2 > 96) fprintf(stderr, "enc2 used, error %d\n", alphablockerror2);
#endif
      write_rgtc_encoded_channel( blkaddr, alphabase[0], alphabase[1], alphaenc2 );
   }
   else {
#if RGTC_DEBUG
      fprintf(stderr, "enc3 used, error %d\n", alphablockerror3);
#endif
      write_rgtc_encoded_channel( blkaddr, (GLubyte)alphatest[0], (GLubyte)alphatest[1], alphaenc3 );
   }
}


static void write_rgtc_encoded_channel_s(GLbyte *blkaddr,
					 GLbyte alphabase1,
					 GLbyte alphabase2,
					 GLbyte alphaenc[16])
{
   *blkaddr++ = alphabase1;
   *blkaddr++ = alphabase2;
   *blkaddr++ = alphaenc[0] | (alphaenc[1] << 3) | ((alphaenc[2] & 3) << 6);
   *blkaddr++ = (alphaenc[2] >> 2) | (alphaenc[3] << 1) | (alphaenc[4] << 4) | ((alphaenc[5] & 1) << 7);
   *blkaddr++ = (alphaenc[5] >> 1) | (alphaenc[6] << 2) | (alphaenc[7] << 5);
   *blkaddr++ = alphaenc[8] | (alphaenc[9] << 3) | ((alphaenc[10] & 3) << 6);
   *blkaddr++ = (alphaenc[10] >> 2) | (alphaenc[11] << 1) | (alphaenc[12] << 4) | ((alphaenc[13] & 1) << 7);
   *blkaddr++ = (alphaenc[13] >> 1) | (alphaenc[14] << 2) | (alphaenc[15] << 5);
}

static void encode_rgtc_chan_s(GLbyte *blkaddr, GLbyte srccolors[4][4],
			       GLint numxpixels, GLint numypixels)
{
   GLbyte alphabase[2], alphause[2];
   GLshort alphatest[2] = { 0 };
   GLuint alphablockerror1, alphablockerror2, alphablockerror3;
   GLbyte i, j, aindex, acutValues[7];
   GLbyte alphaenc1[16], alphaenc2[16], alphaenc3[16];
   GLboolean alphaabsmin = GL_FALSE;
   GLboolean alphaabsmax = GL_FALSE;
   GLshort alphadist;

   /* find lowest and highest alpha value in block, alphabase[0] lowest, alphabase[1] highest */
   alphabase[0] = 0xff; alphabase[1] = 0x0;
   for (j = 0; j < numypixels; j++) {
      for (i = 0; i < numxpixels; i++) {
         if (srccolors[j][i] == 0)
            alphaabsmin = GL_TRUE;
         else if (srccolors[j][i] == 255)
            alphaabsmax = GL_TRUE;
         else {
            if (srccolors[j][i] > alphabase[1])
               alphabase[1] = srccolors[j][i];
            if (srccolors[j][i] < alphabase[0])
               alphabase[0] = srccolors[j][i];
         }
      }
   }


   if ((alphabase[0] > alphabase[1]) && !(alphaabsmin && alphaabsmax)) { /* one color, either max or min */
      /* shortcut here since it is a very common case (and also avoids later problems) */
      /* || (alphabase[0] == alphabase[1] && !alphaabsmin && !alphaabsmax) */
      /* could also thest for alpha0 == alpha1 (and not min/max), but probably not common, so don't bother */

      *blkaddr++ = srccolors[0][0];
      blkaddr++;
      *blkaddr++ = 0;
      *blkaddr++ = 0;
      *blkaddr++ = 0;
      *blkaddr++ = 0;
      *blkaddr++ = 0;
      *blkaddr++ = 0;
#if RGTC_DEBUG
      fprintf(stderr, "enc0 used\n");
#endif
      return;
   }

   /* find best encoding for alpha0 > alpha1 */
   /* it's possible this encoding is better even if both alphaabsmin and alphaabsmax are true */
   alphablockerror1 = 0x0;
   alphablockerror2 = 0xffffffff;
   alphablockerror3 = 0xffffffff;
   if (alphaabsmin) alphause[0] = 0;
   else alphause[0] = alphabase[0];
   if (alphaabsmax) alphause[1] = 255;
   else alphause[1] = alphabase[1];
   /* calculate the 7 cut values, just the middle between 2 of the computed alpha values */
   for (aindex = 0; aindex < 7; aindex++) {
      /* don't forget here is always rounded down */
      acutValues[aindex] = (alphause[0] * (2*aindex + 1) + alphause[1] * (14 - (2*aindex + 1))) / 14;
   }

   for (j = 0; j < numypixels; j++) {
      for (i = 0; i < numxpixels; i++) {
         /* maybe it's overkill to have the most complicated calculation just for the error
            calculation which we only need to figure out if encoding1 or encoding2 is better... */
         if (srccolors[j][i] > acutValues[0]) {
            alphaenc1[4*j + i] = 0;
            alphadist = srccolors[j][i] - alphause[1];
         }
         else if (srccolors[j][i] > acutValues[1]) {
            alphaenc1[4*j + i] = 2;
            alphadist = srccolors[j][i] - (alphause[1] * 6 + alphause[0] * 1) / 7;
         }
         else if (srccolors[j][i] > acutValues[2]) {
            alphaenc1[4*j + i] = 3;
            alphadist = srccolors[j][i] - (alphause[1] * 5 + alphause[0] * 2) / 7;
         }
         else if (srccolors[j][i] > acutValues[3]) {
            alphaenc1[4*j + i] = 4;
            alphadist = srccolors[j][i] - (alphause[1] * 4 + alphause[0] * 3) / 7;
         }
         else if (srccolors[j][i] > acutValues[4]) {
            alphaenc1[4*j + i] = 5;
            alphadist = srccolors[j][i] - (alphause[1] * 3 + alphause[0] * 4) / 7;
         }
         else if (srccolors[j][i] > acutValues[5]) {
            alphaenc1[4*j + i] = 6;
            alphadist = srccolors[j][i] - (alphause[1] * 2 + alphause[0] * 5) / 7;
         }
         else if (srccolors[j][i] > acutValues[6]) {
            alphaenc1[4*j + i] = 7;
            alphadist = srccolors[j][i] - (alphause[1] * 1 + alphause[0] * 6) / 7;
         }
         else {
            alphaenc1[4*j + i] = 1;
            alphadist = srccolors[j][i] - alphause[0];
         }
         alphablockerror1 += alphadist * alphadist;
      }
   }
#if RGTC_DEBUG
   for (i = 0; i < 16; i++) {
      fprintf(stderr, "%d ", alphaenc1[i]);
   }
   fprintf(stderr, "cutVals ");
   for (i = 0; i < 8; i++) {
      fprintf(stderr, "%d ", acutValues[i]);
   }
   fprintf(stderr, "srcVals ");
   for (j = 0; j < numypixels; j++)
      for (i = 0; i < numxpixels; i++) {
	 fprintf(stderr, "%d ", srccolors[j][i]);
      }
   
   fprintf(stderr, "\n");
#endif

   /* it's not very likely this encoding is better if both alphaabsmin and alphaabsmax
      are false but try it anyway */
   if (alphablockerror1 >= 32) {

      /* don't bother if encoding is already very good, this condition should also imply
      we have valid alphabase colors which we absolutely need (alphabase[0] <= alphabase[1]) */
      alphablockerror2 = 0;
      for (aindex = 0; aindex < 5; aindex++) {
         /* don't forget here is always rounded down */
         acutValues[aindex] = (alphabase[0] * (10 - (2*aindex + 1)) + alphabase[1] * (2*aindex + 1)) / 10;
      }
      for (j = 0; j < numypixels; j++) {
         for (i = 0; i < numxpixels; i++) {
             /* maybe it's overkill to have the most complicated calculation just for the error
               calculation which we only need to figure out if encoding1 or encoding2 is better... */
            if (srccolors[j][i] == 0) {
               alphaenc2[4*j + i] = 6;
               alphadist = 0;
            }
            else if (srccolors[j][i] == 255) {
               alphaenc2[4*j + i] = 7;
               alphadist = 0;
            }
            else if (srccolors[j][i] <= acutValues[0]) {
               alphaenc2[4*j + i] = 0;
               alphadist = srccolors[j][i] - alphabase[0];
            }
            else if (srccolors[j][i] <= acutValues[1]) {
               alphaenc2[4*j + i] = 2;
               alphadist = srccolors[j][i] - (alphabase[0] * 4 + alphabase[1] * 1) / 5;
            }
            else if (srccolors[j][i] <= acutValues[2]) {
               alphaenc2[4*j + i] = 3;
               alphadist = srccolors[j][i] - (alphabase[0] * 3 + alphabase[1] * 2) / 5;
            }
            else if (srccolors[j][i] <= acutValues[3]) {
               alphaenc2[4*j + i] = 4;
               alphadist = srccolors[j][i] - (alphabase[0] * 2 + alphabase[1] * 3) / 5;
            }
            else if (srccolors[j][i] <= acutValues[4]) {
               alphaenc2[4*j + i] = 5;
               alphadist = srccolors[j][i] - (alphabase[0] * 1 + alphabase[1] * 4) / 5;
            }
            else {
               alphaenc2[4*j + i] = 1;
               alphadist = srccolors[j][i] - alphabase[1];
            }
            alphablockerror2 += alphadist * alphadist;
         }
      }


      /* skip this if the error is already very small
         this encoding is MUCH better on average than #2 though, but expensive! */
      if ((alphablockerror2 > 96) && (alphablockerror1 > 96)) {
         GLshort blockerrlin1 = 0;
         GLshort blockerrlin2 = 0;
         GLubyte nralphainrangelow = 0;
         GLubyte nralphainrangehigh = 0;
         alphatest[0] = 0xff;
         alphatest[1] = 0x0;
         /* if we have large range it's likely there are values close to 0/255, try to map them to 0/255 */
         for (j = 0; j < numypixels; j++) {
            for (i = 0; i < numxpixels; i++) {
               if ((srccolors[j][i] > alphatest[1]) && (srccolors[j][i] < (255 -(alphabase[1] - alphabase[0]) / 28)))
                  alphatest[1] = srccolors[j][i];
               if ((srccolors[j][i] < alphatest[0]) && (srccolors[j][i] > (alphabase[1] - alphabase[0]) / 28))
                  alphatest[0] = srccolors[j][i];
            }
         }
          /* shouldn't happen too often, don't really care about those degenerated cases */
          if (alphatest[1] <= alphatest[0]) {
             alphatest[0] = 1;
             alphatest[1] = 254;
         }
         for (aindex = 0; aindex < 5; aindex++) {
         /* don't forget here is always rounded down */
            acutValues[aindex] = (alphatest[0] * (10 - (2*aindex + 1)) + alphatest[1] * (2*aindex + 1)) / 10;
         }

         /* find the "average" difference between the alpha values and the next encoded value.
            This is then used to calculate new base values.
            Should there be some weighting, i.e. those values closer to alphatest[x] have more weight,
            since they will see more improvement, and also because the values in the middle are somewhat
            likely to get no improvement at all (because the base values might move in different directions)?
            OTOH it would mean the values in the middle are even less likely to get an improvement
         */
         for (j = 0; j < numypixels; j++) {
            for (i = 0; i < numxpixels; i++) {
               if (srccolors[j][i] <= alphatest[0] / 2) {
               }
               else if (srccolors[j][i] > ((255 + alphatest[1]) / 2)) {
               }
               else if (srccolors[j][i] <= acutValues[0]) {
                  blockerrlin1 += (srccolors[j][i] - alphatest[0]);
                  nralphainrangelow += 1;
               }
               else if (srccolors[j][i] <= acutValues[1]) {
                  blockerrlin1 += (srccolors[j][i] - (alphatest[0] * 4 + alphatest[1] * 1) / 5);
                  blockerrlin2 += (srccolors[j][i] - (alphatest[0] * 4 + alphatest[1] * 1) / 5);
                  nralphainrangelow += 1;
                  nralphainrangehigh += 1;
               }
               else if (srccolors[j][i] <= acutValues[2]) {
                  blockerrlin1 += (srccolors[j][i] - (alphatest[0] * 3 + alphatest[1] * 2) / 5);
                  blockerrlin2 += (srccolors[j][i] - (alphatest[0] * 3 + alphatest[1] * 2) / 5);
                  nralphainrangelow += 1;
                  nralphainrangehigh += 1;
               }
               else if (srccolors[j][i] <= acutValues[3]) {
                  blockerrlin1 += (srccolors[j][i] - (alphatest[0] * 2 + alphatest[1] * 3) / 5);
                  blockerrlin2 += (srccolors[j][i] - (alphatest[0] * 2 + alphatest[1] * 3) / 5);
                  nralphainrangelow += 1;
                  nralphainrangehigh += 1;
               }
               else if (srccolors[j][i] <= acutValues[4]) {
                  blockerrlin1 += (srccolors[j][i] - (alphatest[0] * 1 + alphatest[1] * 4) / 5);
                  blockerrlin2 += (srccolors[j][i] - (alphatest[0] * 1 + alphatest[1] * 4) / 5);
                  nralphainrangelow += 1;
                  nralphainrangehigh += 1;
                  }
               else {
                  blockerrlin2 += (srccolors[j][i] - alphatest[1]);
                  nralphainrangehigh += 1;
               }
            }
         }
         /* shouldn't happen often, needed to avoid div by zero */
         if (nralphainrangelow == 0) nralphainrangelow = 1;
         if (nralphainrangehigh == 0) nralphainrangehigh = 1;
         alphatest[0] = alphatest[0] + (blockerrlin1 / nralphainrangelow);
#if RGTC_DEBUG
	 fprintf(stderr, "block err lin low %d, nr %d\n", blockerrlin1, nralphainrangelow);
         fprintf(stderr, "block err lin high %d, nr %d\n", blockerrlin2, nralphainrangehigh);
#endif
         /* again shouldn't really happen often... */
         if (alphatest[0] < 0) {
            alphatest[0] = 0;
         }
         alphatest[1] = alphatest[1] + (blockerrlin2 / nralphainrangehigh);
         if (alphatest[1] > 255) {
            alphatest[1] = 255;
         }

         alphablockerror3 = 0;
         for (aindex = 0; aindex < 5; aindex++) {
         /* don't forget here is always rounded down */
            acutValues[aindex] = (alphatest[0] * (10 - (2*aindex + 1)) + alphatest[1] * (2*aindex + 1)) / 10;
         }
         for (j = 0; j < numypixels; j++) {
            for (i = 0; i < numxpixels; i++) {
                /* maybe it's overkill to have the most complicated calculation just for the error
                  calculation which we only need to figure out if encoding1 or encoding2 is better... */
               if (srccolors[j][i] <= alphatest[0] / 2) {
                  alphaenc3[4*j + i] = 6;
                  alphadist = srccolors[j][i];
               }
               else if (srccolors[j][i] > ((255 + alphatest[1]) / 2)) {
                  alphaenc3[4*j + i] = 7;
                  alphadist = 255 - srccolors[j][i];
               }
               else if (srccolors[j][i] <= acutValues[0]) {
                  alphaenc3[4*j + i] = 0;
                  alphadist = srccolors[j][i] - alphatest[0];
               }
               else if (srccolors[j][i] <= acutValues[1]) {
                 alphaenc3[4*j + i] = 2;
                 alphadist = srccolors[j][i] - (alphatest[0] * 4 + alphatest[1] * 1) / 5;
               }
               else if (srccolors[j][i] <= acutValues[2]) {
                  alphaenc3[4*j + i] = 3;
                  alphadist = srccolors[j][i] - (alphatest[0] * 3 + alphatest[1] * 2) / 5;
               }
               else if (srccolors[j][i] <= acutValues[3]) {
                  alphaenc3[4*j + i] = 4;
                  alphadist = srccolors[j][i] - (alphatest[0] * 2 + alphatest[1] * 3) / 5;
               }
               else if (srccolors[j][i] <= acutValues[4]) {
                  alphaenc3[4*j + i] = 5;
                  alphadist = srccolors[j][i] - (alphatest[0] * 1 + alphatest[1] * 4) / 5;
               }
               else {
                  alphaenc3[4*j + i] = 1;
                  alphadist = srccolors[j][i] - alphatest[1];
               }
               alphablockerror3 += alphadist * alphadist;
            }
         }
      }
   }
  /* write the alpha values and encoding back. */
   if ((alphablockerror1 <= alphablockerror2) && (alphablockerror1 <= alphablockerror3)) {
#if RGTC_DEBUG
      if (alphablockerror1 > 96) fprintf(stderr, "enc1 used, error %d\n", alphablockerror1);
#endif
      write_rgtc_encoded_channel_s( blkaddr, alphause[1], alphause[0], alphaenc1 );
   }
   else if (alphablockerror2 <= alphablockerror3) {
#if RGTC_DEBUG
      if (alphablockerror2 > 96) fprintf(stderr, "enc2 used, error %d\n", alphablockerror2);
#endif
      write_rgtc_encoded_channel_s( blkaddr, alphabase[0], alphabase[1], alphaenc2 );
   }
   else {
#if RGTC_DEBUG
      fprintf(stderr, "enc3 used, error %d\n", alphablockerror3);
#endif
      write_rgtc_encoded_channel_s( blkaddr, (GLubyte)alphatest[0], (GLubyte)alphatest[1], alphaenc3 );
   }
}

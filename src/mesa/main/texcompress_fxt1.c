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


/**
 * \file texcompress_fxt1.c
 * GL_EXT_texture_compression_fxt1 support.
 */


#include "glheader.h"
#include "imports.h"
#include "colormac.h"
#include "context.h"
#include "convolve.h"
#include "image.h"
#include "texcompress.h"
#include "texformat.h"
#include "texstore.h"


int
fxt1_encode (GLcontext *ctx,
             unsigned int width, unsigned int height,
             int srcFormat,
             const void *source, int srcRowStride,
             void *dest, int destRowStride);
void
fxt1_decode_1 (const void *texture, int width,
               int i, int j, unsigned char *rgba);


/**
 * Called during context initialization.
 */
void
_mesa_init_texture_fxt1( GLcontext *ctx )
{
}


/**
 * Called via TexFormat->StoreImage to store an RGB_FXT1 texture.
 */
static GLboolean
texstore_rgb_fxt1(STORE_PARAMS)
{
   const GLchan *pixels;
   GLint srcRowStride;
   GLubyte *dst;
   const GLint texWidth = dstRowStride * 8 / 16; /* a bit of a hack */
   const GLchan *tempImage = NULL;

   ASSERT(dstFormat == &_mesa_texformat_rgb_fxt1);
   ASSERT(dstXoffset % 8 == 0);
   ASSERT(dstYoffset % 4 == 0);
   ASSERT(dstZoffset     == 0);

   if (srcFormat != GL_RGB ||
       srcType != CHAN_TYPE ||
       ctx->_ImageTransferState ||
       srcPacking->SwapBytes) {
      /* convert image to RGB/GLchan */
      tempImage = _mesa_make_temp_chan_image(ctx, dims,
                                             baseInternalFormat,
                                             dstFormat->BaseFormat,
                                             srcWidth, srcHeight, srcDepth,
                                             srcFormat, srcType, srcAddr,
                                             srcPacking);
      if (!tempImage)
         return GL_FALSE; /* out of memory */
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      pixels = tempImage;
      srcRowStride = 3 * srcWidth;
      srcFormat = GL_RGB;
   }
   else {
      pixels = (const GLchan *) srcAddr;
      srcRowStride = _mesa_image_row_stride(srcPacking, srcWidth, srcFormat,
                                            srcType) / sizeof(GLchan);
   }

   dst = _mesa_compressed_image_address(dstXoffset, dstYoffset, 0,
                                        GL_COMPRESSED_RGB_FXT1_3DFX,
                                        texWidth, (GLubyte *) dstAddr);

   fxt1_encode(ctx, srcWidth, srcHeight, srcFormat, pixels, srcRowStride,
               dst, dstRowStride);

   if (tempImage)
      _mesa_free((void*) tempImage);

   return GL_TRUE;
}


/**
 * Called via TexFormat->StoreImage to store an RGBA_FXT1 texture.
 */
static GLboolean
texstore_rgba_fxt1(STORE_PARAMS)
{
   const GLchan *pixels;
   GLint srcRowStride;
   GLubyte *dst;
   GLint texWidth = dstRowStride * 8 / 16; /* a bit of a hack */
   const GLchan *tempImage = NULL;

   ASSERT(dstFormat == &_mesa_texformat_rgba_fxt1);
   ASSERT(dstXoffset % 8 == 0);
   ASSERT(dstYoffset % 4 == 0);
   ASSERT(dstZoffset     == 0);

   if (srcFormat != GL_RGBA ||
       srcType != CHAN_TYPE ||
       ctx->_ImageTransferState ||
       srcPacking->SwapBytes) {
      /* convert image to RGBA/GLchan */
      tempImage = _mesa_make_temp_chan_image(ctx, dims,
                                             baseInternalFormat,
                                             dstFormat->BaseFormat,
                                             srcWidth, srcHeight, srcDepth,
                                             srcFormat, srcType, srcAddr,
                                             srcPacking);
      if (!tempImage)
         return GL_FALSE; /* out of memory */
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      pixels = tempImage;
      srcRowStride = 4 * srcWidth;
      srcFormat = GL_RGBA;
   }
   else {
      pixels = (const GLchan *) srcAddr;
      srcRowStride = _mesa_image_row_stride(srcPacking, srcWidth, srcFormat,
                                            srcType) / sizeof(GLchan);
   }

   dst = _mesa_compressed_image_address(dstXoffset, dstYoffset, 0,
                                        GL_COMPRESSED_RGBA_FXT1_3DFX,
                                        texWidth, (GLubyte *) dstAddr);

   fxt1_encode(ctx, srcWidth, srcHeight, srcFormat, pixels, srcRowStride,
               dst, dstRowStride);

   if (tempImage)
      _mesa_free((void*) tempImage);

   return GL_TRUE;
}


static void
fetch_texel_2d_rgba_fxt1( const struct gl_texture_image *texImage,
                          GLint i, GLint j, GLint k, GLchan *texel )
{
   fxt1_decode_1(texImage->Data, texImage->Width, i, j, texel);
}


static void
fetch_texel_2d_f_rgba_fxt1( const struct gl_texture_image *texImage,
                            GLint i, GLint j, GLint k, GLfloat *texel )
{
   /* just sample as GLchan and convert to float here */
   GLchan rgba[4];
   fxt1_decode_1(texImage->Data, texImage->Width, i, j, rgba);
   texel[RCOMP] = CHAN_TO_FLOAT(rgba[RCOMP]);
   texel[GCOMP] = CHAN_TO_FLOAT(rgba[GCOMP]);
   texel[BCOMP] = CHAN_TO_FLOAT(rgba[BCOMP]);
   texel[ACOMP] = CHAN_TO_FLOAT(rgba[ACOMP]);
}


static void
fetch_texel_2d_rgb_fxt1( const struct gl_texture_image *texImage,
                         GLint i, GLint j, GLint k, GLchan *texel )
{
   fxt1_decode_1(texImage->Data, texImage->Width, i, j, texel);
   texel[ACOMP] = 255;
}


static void
fetch_texel_2d_f_rgb_fxt1( const struct gl_texture_image *texImage,
                           GLint i, GLint j, GLint k, GLfloat *texel )
{
   /* just sample as GLchan and convert to float here */
   GLchan rgba[4];
   fxt1_decode_1(texImage->Data, texImage->Width, i, j, rgba);
   texel[RCOMP] = CHAN_TO_FLOAT(rgba[RCOMP]);
   texel[GCOMP] = CHAN_TO_FLOAT(rgba[GCOMP]);
   texel[BCOMP] = CHAN_TO_FLOAT(rgba[BCOMP]);
   texel[ACOMP] = 1.0;
}



const struct gl_texture_format _mesa_texformat_rgb_fxt1 = {
   MESA_FORMAT_RGB_FXT1,		/* MesaFormat */
   GL_RGB,				/* BaseFormat */
   GL_UNSIGNED_NORMALIZED_ARB,		/* DataType */
   4, /*approx*/			/* RedBits */
   4, /*approx*/			/* GreenBits */
   4, /*approx*/			/* BlueBits */
   0,					/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   0,					/* TexelBytes */
   texstore_rgb_fxt1,			/* StoreTexImageFunc */
   NULL, /*impossible*/ 		/* FetchTexel1D */
   fetch_texel_2d_rgb_fxt1, 		/* FetchTexel2D */
   NULL, /*impossible*/ 		/* FetchTexel3D */
   NULL, /*impossible*/ 		/* FetchTexel1Df */
   fetch_texel_2d_f_rgb_fxt1, 		/* FetchTexel2Df */
   NULL, /*impossible*/ 		/* FetchTexel3Df */
};

const struct gl_texture_format _mesa_texformat_rgba_fxt1 = {
   MESA_FORMAT_RGBA_FXT1,		/* MesaFormat */
   GL_RGBA,				/* BaseFormat */
   GL_UNSIGNED_NORMALIZED_ARB,		/* DataType */
   4, /*approx*/			/* RedBits */
   4, /*approx*/			/* GreenBits */
   4, /*approx*/			/* BlueBits */
   1, /*approx*/			/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   0,					/* TexelBytes */
   texstore_rgba_fxt1,			/* StoreTexImageFunc */
   NULL, /*impossible*/ 		/* FetchTexel1D */
   fetch_texel_2d_rgba_fxt1, 		/* FetchTexel2D */
   NULL, /*impossible*/ 		/* FetchTexel3D */
   NULL, /*impossible*/ 		/* FetchTexel1Df */
   fetch_texel_2d_f_rgba_fxt1, 		/* FetchTexel2Df */
   NULL, /*impossible*/ 		/* FetchTexel3Df */
};


/***************************************************************************\
 * FXT1 encoder
 *
 * The encoder was built by reversing the decoder,
 * and is vaguely based on Texus2 by 3dfx. Note that this code
 * is merely a proof of concept, since it is higly UNoptimized;
 * moreover it is sub-optimal due to Lloyd's algorithm.
 * Only CHROMA and non-lerp ALPHA is implemented!
\***************************************************************************/


#define MAX_COMP 4 /* ever meeded maximum number of components in texel */
#define MAX_VECT 4 /* ever needed maximum number of base vectors to find */
#define N_TEXELS 32 /* number of texels in a block (always 32) */
#define LL_N_REP 50 /* number of iterations in lloyd's vq */
#define LL_MAX_E 255 /* fault tolerance (maximum error) */


static int
fxt1_besterr (float vec[][MAX_COMP], int nv,
              unsigned char input[MAX_COMP], int nc,
              float *d)
{
   int i, j, best = -1;
   float err = 1e5; /* big enough */

   for (j = 0; j < nv; j++) {
      float e = 0;
      for (i = 0; i < nc; i++) {
         e += (vec[j][i] - input[i]) * (vec[j][i] - input[i]);
      }
      if (e < err) {
         err = e;
         best = j;
      }
   }

   *d = err;
   return best;
}


static int
fxt1_worsterr (float vec[MAX_COMP],
               unsigned char input[N_TEXELS][MAX_COMP], int nc, int n)
{
   int i, k, worst = -1;
   float err = -1; /* small enough */

   for (k = 0; k < n; k++) {
      float e = 0;
      for (i = 0; i < nc; i++) {
         e += (vec[i] - input[k][i]) * (vec[i] - input[k][i]);
      }
      if (e > err) {
         err = e;
         worst = k;
      }
   }

   return worst;
}


static void
fxt1_lloyd (float vec[][MAX_COMP], int nv,
            unsigned char input[N_TEXELS][MAX_COMP], int nc, int n)
{
   /* Use the generalized lloyd's algorithm for VQ:
    *     find 4 color vectors.
    *
    *     for each sample color
    *         sort to nearest vector.
    *
    *     replace each vector with the centroid of it's matching colors.
    *
    *     repeat until RMS doesn't improve.
    *
    *     if a color vector has no samples, or becomes the same as another
    *     vector, replace it with the color which is farthest from a sample.
    *
    * vec[][MAX_COMP]           resulting colors
    * nv                        number of resulting colors required
    * input[N_TEXELS][MAX_COMP] input texels
    * nc                        number of components in input / vec
    * n                         number of input samples
    */

   int sum[MAX_VECT][MAX_COMP]; /* used to accumulate closest texels */
   int cnt[MAX_VECT]; /* how many times a certain vector was chosen */
   float error;

   int i, j, k, rep;

   /* choose the base vectors from input */
   for (j = 0; j < nv; j++) {
      int m = j * (n - 1) / (nv - 1);
      for (i = 0; i < nc; i++) {
         vec[j][i] = input[m][i];
      }
   }

   /* the quantizer */
   for (rep = 0; rep < LL_N_REP; rep++) {
      /* reset sums & counters */
      for (j = 0; j < nv; j++) {
         for (i = 0; i < nc; i++) {
            sum[j][i] = 0;
         }
         cnt[j] = 0;
      }
      error = 0;

      /* scan whole block */
      for (k = 0; k < n; k++) {
         float d;
         int best = fxt1_besterr(vec, nv, input[k], nc, &d);
         /* add in closest color */
         for (i = 0; i < nc; i++) {
            sum[best][i] += input[k][i];
         }
         /* mark this vector as used */
         cnt[best]++;
         /* accumulate error */
         error += d;
      }

      /* accumulated distance (error) small enough? */
      if (error < LL_MAX_E) {
         break;
      }

      /* move each vector to the barycenter of its closest colors */
      for (j = 0; j < nv; j++) {
         if (cnt[j]) {
            float div = 1.0 / cnt[j];
            for (i = 0; i < nc; i++) {
               vec[j][i] = div * sum[j][i];
            }
         } else {
            /* this vec has no samples or is identical with a previous vec */
            int worst = fxt1_worsterr(vec[j], input, nc, n);
            for (i = 0; i < nc; i++) {
               vec[j][i] = input[worst][i];
            }
         }
      }
   }
}


static void
fxt1_quantize_CHROMA (unsigned long *cc,
                      unsigned char input[N_TEXELS][MAX_COMP])
{
   const int n_vect = 4; /* 4 base vectors to find */
   const int n_comp = 3; /* 3 components: R, G, B */
   float vec[MAX_VECT][MAX_COMP];
   int i, j, k;
   unsigned long long hihi; /* high quadword */
   unsigned long lohi, lolo; /* low quadword: hi dword, lo dword */
   float d;

   fxt1_lloyd(vec, n_vect, input, n_comp, N_TEXELS);

   hihi = 4; /* cc-chroma = "010" + unused bit */
   for (j = 0; j < n_vect; j++) {
      for (i = 0; i < n_comp; i++) {
         /* add in colors */
         hihi <<= 5;
         hihi |= (unsigned int)vec[n_vect - 1 - j][i] >> 3;
      }
   }
   ((unsigned long long *)cc)[1] = hihi;

   lohi = lolo = 0;
   /* right microtile */
   for (k = N_TEXELS - 1; k >= N_TEXELS/2; k--) {
      lohi <<= 2;
      lohi |= fxt1_besterr(vec, n_vect, input[k], n_comp, &d);
   }
   /* left microtile */
   for (; k >= 0; k--) {
      lolo <<= 2;
      lolo |= fxt1_besterr(vec, n_vect, input[k], n_comp, &d);
   }
   cc[1] = lohi;
   cc[0] = lolo;
}


static void
fxt1_quantize_ALPHA0 (unsigned long *cc,
                      unsigned char input[N_TEXELS][MAX_COMP],
                      unsigned char reord[N_TEXELS][MAX_COMP], int n)
{
   const int n_vect = 3; /* 3 base vectors to find */
   const int n_comp = 4; /* 4 components: R, G, B, A */
   float vec[MAX_VECT][MAX_COMP];
   int i, j, k;
   unsigned long long hihi; /* high quadword */
   unsigned long lohi, lolo; /* low quadword: hi dword, lo dword */
   float d;

   /* the last vector indicates zero */
   for (i = 0; i < n_comp; i++) {
      vec[n_vect][i] = 0;
   }

   /* the first n texels in reord are guaranteed to be non-zero */
   fxt1_lloyd(vec, n_vect, reord, n_comp, n);

   hihi = 6; /* alpha = "011" + lerp = 0 */
   for (j = 0; j < n_vect; j++) {
      /* add in alphas */
      hihi <<= 5;
      hihi |= (unsigned int)vec[n_vect - 1 - j][n_comp - 1] >> 3;
   }
   for (j = 0; j < n_vect; j++) {
      for (i = 0; i < n_comp - 1; i++) {
         /* add in colors */
         hihi <<= 5;
         hihi |= (unsigned int)vec[n_vect - 1 - j][i] >> 3;
      }
   }
   ((unsigned long long *)cc)[1] = hihi;

   lohi = lolo = 0;
   /* right microtile */
   for (k = N_TEXELS - 1; k >= N_TEXELS/2; k--) {
      lohi <<= 2;
      lohi |= fxt1_besterr(vec, n_vect + 1, input[k], n_comp, &d);
   }
   /* left microtile */
   for (; k >= 0; k--) {
      lolo <<= 2;
      lolo |= fxt1_besterr(vec, n_vect + 1, input[k], n_comp, &d);
   }
   cc[1] = lohi;
   cc[0] = lolo;
}


static void
fxt1_quantize (unsigned long *cc, const unsigned char *lines[], int comps)
{
   int trualpha = 0;
   unsigned char reord[N_TEXELS][MAX_COMP];

   unsigned char input[N_TEXELS][MAX_COMP];
   int i, k, l;

   /* 8 texels each line */
   for (l = 0; l < 4; l++) {
      for (k = 0; k < 4; k++) {
         for (i = 0; i < comps; i++) {
            input[k + l * 4][i] = *lines[l]++;
         }
         for (; i < MAX_COMP; i++) {
            input[k + l * 4][i] = 255;
         }
      }
      for (k = 0; k < 4; k++) {
         for (i = 0; i < comps; i++) {
            input[k + l * 4 + 16][i] = *lines[l]++;
         }
         for (; i < MAX_COMP; i++) {
            input[k + l * 4 + 16][i] = 255;
         }
      }
   }

   /* [dBorca]
    * stupidity flows forth from this
    */

   if (comps == 4) {
      /* skip all transparent black texels */
      l = 0;
      for (k = 0; k < N_TEXELS; k++) {
         int t = 0;
         /* test all components against 0 */
         for (i = 0; i < comps; i++) {
            reord[l][i] = input[k][i];
            t += input[k][i];
         }
         if (t) {
            /* texel is not transparent black */
            if (reord[l][comps - 1] < 255) {
               /* non-opaque texel */
               trualpha = !0;
            }
            l++;
         } else {
            /* transparent black texel */
            trualpha = !0;
         }
      }
   }

   if (trualpha) {
      fxt1_quantize_ALPHA0(cc, input, reord, l);
   } else {
      fxt1_quantize_CHROMA(cc, input);
   }
}


int
fxt1_encode (GLcontext *ctx,
             unsigned int width, unsigned int height,
             int srcFormat,
             const void *source, int srcRowStride,
             void *dest, int destRowStride)
{
   const int comps = (srcFormat == GL_RGB) ? 3 : 4;
   unsigned int x, y;
   const unsigned char *data = source;
   unsigned long *encoded = dest;
   GLubyte *newSource = NULL;

   /*
    * Rescale image if width is less than 8 or height is less than 4.
    */
   if (width < 8 || height < 4) {
      GLint newWidth = (width + 7) & ~7;
      GLint newHeight = (height + 3) & ~3;
      newSource = MALLOC(comps * newWidth * newHeight * sizeof(GLchan));
      _mesa_upscale_teximage2d(width, height, newWidth, newHeight,
                               comps, source, srcRowStride, newSource);
      source = newSource;
      width = newWidth;
      height = newHeight;
      srcRowStride = comps * newWidth;
   }

   destRowStride = (destRowStride - width * 2) / 4;
   for (y = 0; y < height; y += 4) {
      for (x = 0; x < width; x += 8) {
         const unsigned char *lines[4];
         lines[0] = &data[x * comps + (y + 0) * srcRowStride];
         lines[1] = &data[x * comps + (y + 1) * srcRowStride];
         lines[2] = &data[x * comps + (y + 2) * srcRowStride];
         lines[3] = &data[x * comps + (y + 3) * srcRowStride];
         fxt1_quantize(encoded, lines, comps);
         /* 128 bits per 8x4 block = 4bpp */
         encoded += 4;
      }
      encoded += destRowStride;
   }

   return 0;
}


/***************************************************************************\
 * FXT1 decoder
 *
 * The decoder is based on GL_3DFX_texture_compression_FXT1
 * specification and serves as a concept for the encoder.
\***************************************************************************/


/* lookup table for scaling 5 bit colors up to 8 bits */
static unsigned char _rgb_scale_5[] = {
   0,   8,   16,  25,  33,  41,  49,  58,
   66,  74,  82,  90,  99,  107, 115, 123,
   132, 140, 148, 156, 165, 173, 181, 189,
   197, 206, 214, 222, 230, 239, 247, 255
};

/* lookup table for scaling 6 bit colors up to 8 bits */
static unsigned char _rgb_scale_6[] = {
   0,   4,   8,   12,  16,  20,  24,  28,
   32,  36,  40,  45,  49,  53,  57,  61,
   65,  69,  73,  77,  81,  85,  89,  93,
   97,  101, 105, 109, 113, 117, 121, 125,
   130, 134, 138, 142, 146, 150, 154, 158,
   162, 166, 170, 174, 178, 182, 186, 190,
   194, 198, 202, 206, 210, 215, 219, 223,
   227, 231, 235, 239, 243, 247, 251, 255
};


#define CC_SEL(cc, which) ((cc)[(which) / 32] >> ((which) & 31))
#define UP5(c) _rgb_scale_5[(c) & 31]
#define UP6(c, b) _rgb_scale_6[(((c) & 31) << 1) | ((b) & 1)]
#define LERP(n, t, c0, c1) (((n) - (t)) * (c0) + (t) * (c1) + (n) / 2) / (n)
#define ZERO_4UBV(v) *((unsigned long *)(v)) = 0


static void
fxt1_decode_1HI (unsigned long code, int t, unsigned char *rgba)
{
   const unsigned long *cc;

   t *= 3;
   cc = (unsigned long *)(code + t / 8);
   t = (cc[0] >> (t & 7)) & 7;

   if (t == 7) {
      ZERO_4UBV(rgba);
   } else {
      cc = (unsigned long *)(code + 12);
      if (t == 0) {
         rgba[BCOMP] = UP5(CC_SEL(cc, 0));
         rgba[GCOMP] = UP5(CC_SEL(cc, 5));
         rgba[RCOMP] = UP5(CC_SEL(cc, 10));
      } else if (t == 6) {
         rgba[BCOMP] = UP5(CC_SEL(cc, 15));
         rgba[GCOMP] = UP5(CC_SEL(cc, 20));
         rgba[RCOMP] = UP5(CC_SEL(cc, 25));
      } else {
         rgba[BCOMP] = LERP(6, t, UP5(CC_SEL(cc, 0)), UP5(CC_SEL(cc, 15)));
         rgba[GCOMP] = LERP(6, t, UP5(CC_SEL(cc, 5)), UP5(CC_SEL(cc, 20)));
         rgba[RCOMP] = LERP(6, t, UP5(CC_SEL(cc, 10)), UP5(CC_SEL(cc, 25)));
      }
      rgba[ACOMP] = 255;
   }
}


static void
fxt1_decode_1CHROMA (unsigned long code, int t, unsigned char *rgba)
{
   const unsigned long *cc;
   unsigned long kk;

   cc = (unsigned long *)code;
   if (t & 16) {
      cc++;
      t &= 15;
   }
   t = (cc[0] >> (t * 2)) & 3;

   t *= 15;
   cc = (unsigned long *)(code + 8 + t / 8);
   kk = cc[0] >> (t & 7);
   rgba[BCOMP] = UP5(kk);
   rgba[GCOMP] = UP5(kk >> 5);
   rgba[RCOMP] = UP5(kk >> 10);
   rgba[ACOMP] = 255;
}


static void
fxt1_decode_1MIXED (unsigned long code, int t, unsigned char *rgba)
{
   const unsigned long *cc;
   unsigned int col[2][3];
   int glsb, selb;

   cc = (unsigned long *)code;
   if (t & 16) {
      t &= 15;
      t = (cc[1] >> (t * 2)) & 3;
      /* col 2 */
      col[0][BCOMP] = (*(unsigned long *)(code + 11)) >> 6;
      col[0][GCOMP] = CC_SEL(cc, 99);
      col[0][RCOMP] = CC_SEL(cc, 104);
      /* col 3 */
      col[1][BCOMP] = CC_SEL(cc, 109);
      col[1][GCOMP] = CC_SEL(cc, 114);
      col[1][RCOMP] = CC_SEL(cc, 119);
      glsb = CC_SEL(cc, 126);
      selb = CC_SEL(cc, 33);
   } else {
      t = (cc[0] >> (t * 2)) & 3;
      /* col 0 */
      col[0][BCOMP] = CC_SEL(cc, 64);
      col[0][GCOMP] = CC_SEL(cc, 69);
      col[0][RCOMP] = CC_SEL(cc, 74);
      /* col 1 */
      col[1][BCOMP] = CC_SEL(cc, 79);
      col[1][GCOMP] = CC_SEL(cc, 84);
      col[1][RCOMP] = CC_SEL(cc, 89);
      glsb = CC_SEL(cc, 125);
      selb = CC_SEL(cc, 1);
   }

   if (CC_SEL(cc, 124) & 1) {
      /* alpha[0] == 1 */

      if (t == 3) {
         ZERO_4UBV(rgba);
      } else {
         if (t == 0) {
            rgba[BCOMP] = UP5(col[0][BCOMP]);
            rgba[GCOMP] = UP5(col[0][GCOMP]);
            rgba[RCOMP] = UP5(col[0][RCOMP]);
         } else if (t == 2) {
            rgba[BCOMP] = UP5(col[1][BCOMP]);
            rgba[GCOMP] = UP6(col[1][GCOMP], glsb);
            rgba[RCOMP] = UP5(col[1][RCOMP]);
         } else {
            rgba[BCOMP] = (UP5(col[0][BCOMP]) + UP5(col[1][BCOMP])) / 2;
            rgba[GCOMP] = (UP5(col[0][GCOMP]) + UP6(col[1][GCOMP], glsb)) / 2;
            rgba[RCOMP] = (UP5(col[0][RCOMP]) + UP5(col[1][RCOMP])) / 2;
         }
         rgba[ACOMP] = 255;
      }
   } else {
      /* alpha[0] == 0 */

      if (t == 0) {
         rgba[BCOMP] = UP5(col[0][BCOMP]);
         rgba[GCOMP] = UP6(col[0][GCOMP], glsb ^ selb);
         rgba[RCOMP] = UP5(col[0][RCOMP]);
      } else if (t == 3) {
         rgba[BCOMP] = UP5(col[1][BCOMP]);
         rgba[GCOMP] = UP6(col[1][GCOMP], glsb);
         rgba[RCOMP] = UP5(col[1][RCOMP]);
      } else {
         rgba[BCOMP] = LERP(3, t, UP5(col[0][BCOMP]), UP5(col[1][BCOMP]));
         rgba[GCOMP] = LERP(3, t, UP6(col[0][GCOMP], glsb ^ selb),
                                  UP6(col[1][GCOMP], glsb));
         rgba[RCOMP] = LERP(3, t, UP5(col[0][RCOMP]), UP5(col[1][RCOMP]));
      }
      rgba[ACOMP] = 255;
   }
}


static void
fxt1_decode_1ALPHA (unsigned long code, int t, unsigned char *rgba)
{
   const unsigned long *cc;

   cc = (unsigned long *)code;
   if (CC_SEL(cc, 124) & 1) {
      /* lerp == 1 */
      unsigned int col0[4];

      if (t & 16) {
         t &= 15;
         t = (cc[1] >> (t * 2)) & 3;
         /* col 2 */
         col0[BCOMP] = (*(unsigned long *)(code + 11)) >> 6;
         col0[GCOMP] = CC_SEL(cc, 99);
         col0[RCOMP] = CC_SEL(cc, 104);
         col0[ACOMP] = CC_SEL(cc, 119);
      } else {
         t = (cc[0] >> (t * 2)) & 3;
         /* col 0 */
         col0[BCOMP] = CC_SEL(cc, 64);
         col0[GCOMP] = CC_SEL(cc, 69);
         col0[RCOMP] = CC_SEL(cc, 74);
         col0[ACOMP] = CC_SEL(cc, 109);
      }

      if (t == 0) {
         rgba[BCOMP] = UP5(col0[BCOMP]);
         rgba[GCOMP] = UP5(col0[GCOMP]);
         rgba[RCOMP] = UP5(col0[RCOMP]);
         rgba[ACOMP] = UP5(col0[ACOMP]);
      } else if (t == 3) {
         rgba[BCOMP] = UP5(CC_SEL(cc, 79));
         rgba[GCOMP] = UP5(CC_SEL(cc, 84));
         rgba[RCOMP] = UP5(CC_SEL(cc, 89));
         rgba[ACOMP] = UP5(CC_SEL(cc, 114));
      } else {
         rgba[BCOMP] = LERP(3, t, UP5(col0[BCOMP]), UP5(CC_SEL(cc, 79)));
         rgba[GCOMP] = LERP(3, t, UP5(col0[GCOMP]), UP5(CC_SEL(cc, 84)));
         rgba[RCOMP] = LERP(3, t, UP5(col0[RCOMP]), UP5(CC_SEL(cc, 89)));
         rgba[ACOMP] = LERP(3, t, UP5(col0[ACOMP]), UP5(CC_SEL(cc, 114)));
      }
   } else {
      /* lerp == 0 */

      if (t & 16) {
         cc++;
         t &= 15;
      }
      t = (cc[0] >> (t * 2)) & 3;

      if (t == 3) {
         ZERO_4UBV(rgba);
      } else {
         unsigned long kk;
         cc = (unsigned long *)code;
         rgba[ACOMP] = UP5(cc[3] >> (t * 5 + 13));
         t *= 15;
         cc = (unsigned long *)(code + 8 + t / 8);
         kk = cc[0] >> (t & 7);
         rgba[BCOMP] = UP5(kk);
         rgba[GCOMP] = UP5(kk >> 5);
         rgba[RCOMP] = UP5(kk >> 10);
      }
   }
}


void
fxt1_decode_1 (const void *texture, int width,
               int i, int j, unsigned char *rgba)
{
   static void (*decode_1[]) (unsigned long, int, unsigned char *) = {
      fxt1_decode_1HI,     /* cc-high   = "00?" */
      fxt1_decode_1HI,     /* cc-high   = "00?" */
      fxt1_decode_1CHROMA, /* cc-chroma = "010" */
      fxt1_decode_1ALPHA,  /* alpha     = "011" */
      fxt1_decode_1MIXED,  /* mixed     = "1??" */
      fxt1_decode_1MIXED,  /* mixed     = "1??" */
      fxt1_decode_1MIXED,  /* mixed     = "1??" */
      fxt1_decode_1MIXED   /* mixed     = "1??" */
   };

   unsigned long code = (unsigned long)texture +
                        ((j / 4) * (width / 8) + (i / 8)) * 16;
   int mode = CC_SEL((unsigned long *)code, 125);
   int t = i & 7;

   if (t & 4) {
      t += 12;
   }
   t += (j & 3) * 4;

   decode_1[mode](code, t, rgba);
}

/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

/**
 * RGBA/float tile get/put functions.
 * Usable both by drivers and state trackers.
 */


#include "pipe/p_defines.h"
#include "pipe/p_inlines.h"

#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_rect.h"
#include "util/u_tile.h"
#include "lp_tile_cache.h"
#include "lp_tile_soa.h"


const unsigned char
tile_offset[TILE_VECTOR_HEIGHT][TILE_VECTOR_WIDTH] = {
   {  0,  1,  4,  5,  8,  9, 12, 13},
   {  2,  3,  6,  7, 10, 11, 14, 15}
};



/*** PIPE_FORMAT_A8R8G8B8_UNORM ***/

static void
a8r8g8b8_get_tile_rgba(const unsigned *src,
                       unsigned w, unsigned h,
                       uint8_t *p)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      for (j = 0; j < w; j++) {
         const unsigned pixel = *src++;
         TILE_PIXEL(p, j, i, 0) = (pixel >> 16) & 0xff;
         TILE_PIXEL(p, j, i, 1) = (pixel >>  8) & 0xff;
         TILE_PIXEL(p, j, i, 2) = (pixel >>  0) & 0xff;
         TILE_PIXEL(p, j, i, 3) = (pixel >> 24) & 0xff;
      }
   }
}


static void
a8r8g8b8_put_tile_rgba(unsigned *dst,
                       unsigned w, unsigned h,
                       const uint8_t *p)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      for (j = 0; j < w; j++) {
         unsigned r, g, b, a;
         r = TILE_PIXEL(p, j, i, 0);
         g = TILE_PIXEL(p, j, i, 1);
         b = TILE_PIXEL(p, j, i, 2);
         a = TILE_PIXEL(p, j, i, 3);
         *dst++ = (a << 24) | (r << 16) | (g << 8) | b;
      }
   }
}


/*** PIPE_FORMAT_A8R8G8B8_UNORM ***/

static void
x8r8g8b8_get_tile_rgba(const unsigned *src,
                       unsigned w, unsigned h,
                       uint8_t *p)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      for (j = 0; j < w; j++) {
         const unsigned pixel = *src++;
         TILE_PIXEL(p, j, i, 0) = (pixel >> 16) & 0xff;
         TILE_PIXEL(p, j, i, 1) = (pixel >>  8) & 0xff;
         TILE_PIXEL(p, j, i, 2) = (pixel >>  0) & 0xff;
         TILE_PIXEL(p, j, i, 3) = 0xff;
      }
   }
}


static void
x8r8g8b8_put_tile_rgba(unsigned *dst,
                       unsigned w, unsigned h,
                       const uint8_t *p)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      for (j = 0; j < w; j++) {
         unsigned r, g, b;
         r = TILE_PIXEL(p, j, i, 0);
         g = TILE_PIXEL(p, j, i, 1);
         b = TILE_PIXEL(p, j, i, 2);
         *dst++ = (0xff << 24) | (r << 16) | (g << 8) | b;
      }
   }
}


/*** PIPE_FORMAT_B8G8R8A8_UNORM ***/

static void
b8g8r8a8_get_tile_rgba(const unsigned *src,
                       unsigned w, unsigned h,
                       uint8_t *p)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      for (j = 0; j < w; j++) {
         const unsigned pixel = *src++;
         TILE_PIXEL(p, j, i, 0) = (pixel >>  8) & 0xff;
         TILE_PIXEL(p, j, i, 1) = (pixel >> 16) & 0xff;
         TILE_PIXEL(p, j, i, 2) = (pixel >> 24) & 0xff;
         TILE_PIXEL(p, j, i, 3) = (pixel >>  0) & 0xff;
      }
   }
}


static void
b8g8r8a8_put_tile_rgba(unsigned *dst,
                       unsigned w, unsigned h,
                       const uint8_t *p)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      for (j = 0; j < w; j++) {
         unsigned r, g, b, a;
         r = TILE_PIXEL(p, j, i, 0);
         g = TILE_PIXEL(p, j, i, 1);
         b = TILE_PIXEL(p, j, i, 2);
         a = TILE_PIXEL(p, j, i, 3);
         *dst++ = (b << 24) | (g << 16) | (r << 8) | a;
      }
   }
}


/*** PIPE_FORMAT_A1R5G5B5_UNORM ***/

static void
a1r5g5b5_get_tile_rgba(const ushort *src,
                       unsigned w, unsigned h,
                       uint8_t *p)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      for (j = 0; j < w; j++) {
         const ushort pixel = *src++;
         TILE_PIXEL(p, j, i, 0) = ((pixel >> 10) & 0x1f) * 255 / 31;
         TILE_PIXEL(p, j, i, 1) = ((pixel >>  5) & 0x1f) * 255 / 31;
         TILE_PIXEL(p, j, i, 2) = ((pixel      ) & 0x1f) * 255 / 31;
         TILE_PIXEL(p, j, i, 3) = ((pixel >> 15)       ) * 255;
      }
   }
}


static void
a1r5g5b5_put_tile_rgba(ushort *dst,
                       unsigned w, unsigned h,
                       const uint8_t *p)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      for (j = 0; j < w; j++) {
         unsigned r, g, b, a;
         r = TILE_PIXEL(p, j, i, 0);
         g = TILE_PIXEL(p, j, i, 1);
         b = TILE_PIXEL(p, j, i, 2);
         a = TILE_PIXEL(p, j, i, 3);
         r = r >> 3;  /* 5 bits */
         g = g >> 3;  /* 5 bits */
         b = b >> 3;  /* 5 bits */
         a = a >> 7;  /* 1 bit */
         *dst++ = (a << 15) | (r << 10) | (g << 5) | b;
      }
   }
}


/*** PIPE_FORMAT_A4R4G4B4_UNORM ***/

static void
a4r4g4b4_get_tile_rgba(const ushort *src,
                       unsigned w, unsigned h,
                       uint8_t *p)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      for (j = 0; j < w; j++) {
         const ushort pixel = *src++;
         TILE_PIXEL(p, j, i, 0) = ((pixel >>  8) & 0xf) * 255 / 15;
         TILE_PIXEL(p, j, i, 1) = ((pixel >>  4) & 0xf) * 255 / 15;
         TILE_PIXEL(p, j, i, 2) = ((pixel      ) & 0xf) * 255 / 15;
         TILE_PIXEL(p, j, i, 3) = ((pixel >> 12)      ) * 255 / 15;
      }
   }
}


static void
a4r4g4b4_put_tile_rgba(ushort *dst,
                       unsigned w, unsigned h,
                       const uint8_t *p)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      for (j = 0; j < w; j++) {
         unsigned r, g, b, a;
         r = TILE_PIXEL(p, j, i, 0);
         g = TILE_PIXEL(p, j, i, 1);
         b = TILE_PIXEL(p, j, i, 2);
         a = TILE_PIXEL(p, j, i, 3);
         r >>= 4;
         g >>= 4;
         b >>= 4;
         a >>= 4;
         *dst++ = (a << 12) | (r << 16) | (g << 4) | b;
      }
   }
}


/*** PIPE_FORMAT_R5G6B5_UNORM ***/

static void
r5g6b5_get_tile_rgba(const ushort *src,
                     unsigned w, unsigned h,
                     uint8_t *p)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      for (j = 0; j < w; j++) {
         const ushort pixel = *src++;
         TILE_PIXEL(p, j, i, 0) = ((pixel >> 11) & 0x1f) * 255 / 31;
         TILE_PIXEL(p, j, i, 1) = ((pixel >>  5) & 0x3f) * 255 / 63;
         TILE_PIXEL(p, j, i, 2) = ((pixel      ) & 0x1f) * 255 / 31;
         TILE_PIXEL(p, j, i, 3) = 255;
      }
   }
}


static void
r5g6b5_put_tile_rgba(ushort *dst,
                     unsigned w, unsigned h,
                     const uint8_t *p)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      for (j = 0; j < w; j++) {
         uint r = (uint) TILE_PIXEL(p, j, i, 0) * 31 / 255;
         uint g = (uint) TILE_PIXEL(p, j, i, 1) * 63 / 255;
         uint b = (uint) TILE_PIXEL(p, j, i, 2) * 31 / 255;
         *dst++ = (r << 11) | (g << 5) | (b);
      }
   }
}



/*** PIPE_FORMAT_Z16_UNORM ***/

/**
 * Return each Z value as four floats in [0,1].
 */
static void
z16_get_tile_rgba(const ushort *src,
                  unsigned w, unsigned h,
                  uint8_t *p)
{
   const float scale = 1.0f / 65535.0f;
   unsigned i, j;

   for (i = 0; i < h; i++) {
      for (j = 0; j < w; j++) {
         TILE_PIXEL(p, j, i, 0) =
         TILE_PIXEL(p, j, i, 1) =
         TILE_PIXEL(p, j, i, 2) =
         TILE_PIXEL(p, j, i, 3) = *src++ * scale;
      }
   }
}




/*** PIPE_FORMAT_L8_UNORM ***/

static void
l8_get_tile_rgba(const ubyte *src,
                 unsigned w, unsigned h,
                 uint8_t *p)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      for (j = 0; j < w; j++, src++) {
         TILE_PIXEL(p, j, i, 0) =
         TILE_PIXEL(p, j, i, 1) =
         TILE_PIXEL(p, j, i, 2) = *src;
         TILE_PIXEL(p, j, i, 3) = 255;
      }
   }
}


static void
l8_put_tile_rgba(ubyte *dst,
                 unsigned w, unsigned h,
                 const uint8_t *p)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      for (j = 0; j < w; j++) {
         unsigned r;
         r = TILE_PIXEL(p, j, i, 0);
         *dst++ = (ubyte) r;
      }
   }
}



/*** PIPE_FORMAT_A8_UNORM ***/

static void
a8_get_tile_rgba(const ubyte *src,
                 unsigned w, unsigned h,
                 uint8_t *p)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      for (j = 0; j < w; j++, src++) {
         TILE_PIXEL(p, j, i, 0) =
         TILE_PIXEL(p, j, i, 1) =
         TILE_PIXEL(p, j, i, 2) = 0;
         TILE_PIXEL(p, j, i, 3) = *src;
      }
   }
}


static void
a8_put_tile_rgba(ubyte *dst,
                 unsigned w, unsigned h,
                 const uint8_t *p)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      for (j = 0; j < w; j++) {
         unsigned a;
         a = TILE_PIXEL(p, j, i, 3);
         *dst++ = (ubyte) a;
      }
   }
}



/*** PIPE_FORMAT_R16_SNORM ***/

static void
r16_get_tile_rgba(const short *src,
                  unsigned w, unsigned h,
                  uint8_t *p)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      for (j = 0; j < w; j++, src++) {
         TILE_PIXEL(p, j, i, 0) = MAX2(src[0] >> 7, 0);
         TILE_PIXEL(p, j, i, 1) =
         TILE_PIXEL(p, j, i, 2) = 0;
         TILE_PIXEL(p, j, i, 3) = 255;
      }
   }
}


static void
r16_put_tile_rgba(short *dst,
                  unsigned w, unsigned h,
                  const uint8_t *p)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      for (j = 0; j < w; j++, dst++) {
         dst[0] = TILE_PIXEL(p, j, i, 0) << 7;
      }
   }
}


/*** PIPE_FORMAT_R16G16B16A16_SNORM ***/

static void
r16g16b16a16_get_tile_rgba(const short *src,
                           unsigned w, unsigned h,
                           uint8_t *p)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      for (j = 0; j < w; j++, src += 4) {
         TILE_PIXEL(p, j, i, 0) = src[0] >> 8;
         TILE_PIXEL(p, j, i, 1) = src[1] >> 8;
         TILE_PIXEL(p, j, i, 2) = src[2] >> 8;
         TILE_PIXEL(p, j, i, 3) = src[3] >> 8;
      }
   }
}


static void
r16g16b16a16_put_tile_rgba(short *dst,
                           unsigned w, unsigned h,
                           const uint8_t *p)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      for (j = 0; j < w; j++, dst += 4) {
         dst[0] = TILE_PIXEL(p, j, i, 0) << 8;
         dst[1] = TILE_PIXEL(p, j, i, 1) << 8;
         dst[2] = TILE_PIXEL(p, j, i, 2) << 8;
         dst[3] = TILE_PIXEL(p, j, i, 3) << 8;
      }
   }
}



/*** PIPE_FORMAT_I8_UNORM ***/

static void
i8_get_tile_rgba(const ubyte *src,
                 unsigned w, unsigned h,
                 uint8_t *p)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      for (j = 0; j < w; j++, src++) {
         TILE_PIXEL(p, j, i, 0) =
         TILE_PIXEL(p, j, i, 1) =
         TILE_PIXEL(p, j, i, 2) =
         TILE_PIXEL(p, j, i, 3) = *src;
      }
   }
}


static void
i8_put_tile_rgba(ubyte *dst,
                 unsigned w, unsigned h,
                 const uint8_t *p)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      for (j = 0; j < w; j++) {
         unsigned r;
         r = TILE_PIXEL(p, j, i, 0);
         *dst++ = (ubyte) r;
      }
   }
}


/*** PIPE_FORMAT_A8L8_UNORM ***/

static void
a8l8_get_tile_rgba(const ushort *src,
                   unsigned w, unsigned h,
                   uint8_t *p)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      for (j = 0; j < w; j++) {
         ushort ra = *src++;
         TILE_PIXEL(p, j, i, 0) =
         TILE_PIXEL(p, j, i, 1) =
         TILE_PIXEL(p, j, i, 2) = ra & 0xff;
         TILE_PIXEL(p, j, i, 3) = ra >> 8;
      }
   }
}


static void
a8l8_put_tile_rgba(ushort *dst,
                   unsigned w, unsigned h,
                   const uint8_t *p)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      for (j = 0; j < w; j++) {
         unsigned r, a;
         r = TILE_PIXEL(p, j, i, 0);
         a = TILE_PIXEL(p, j, i, 3);
         *dst++ = (a << 8) | r;
      }
   }
}




/*** PIPE_FORMAT_Z32_UNORM ***/

/**
 * Return each Z value as four floats in [0,1].
 */
static void
z32_get_tile_rgba(const unsigned *src,
                  unsigned w, unsigned h,
                  uint8_t *p)
{
   const double scale = 1.0 / (double) 0xffffffff;
   unsigned i, j;

   for (i = 0; i < h; i++) {
      for (j = 0; j < w; j++) {
         TILE_PIXEL(p, j, i, 0) =
         TILE_PIXEL(p, j, i, 1) =
         TILE_PIXEL(p, j, i, 2) =
         TILE_PIXEL(p, j, i, 3) = (float) (*src++ * scale);
      }
   }
}


/*** PIPE_FORMAT_S8Z24_UNORM ***/

/**
 * Return Z component as four float in [0,1].  Stencil part ignored.
 */
static void
s8z24_get_tile_rgba(const unsigned *src,
                    unsigned w, unsigned h,
                    uint8_t *p)
{
   const double scale = 1.0 / ((1 << 24) - 1);
   unsigned i, j;

   for (i = 0; i < h; i++) {
      for (j = 0; j < w; j++) {
         TILE_PIXEL(p, j, i, 0) =
         TILE_PIXEL(p, j, i, 1) =
         TILE_PIXEL(p, j, i, 2) =
         TILE_PIXEL(p, j, i, 3) = (float) (scale * (*src++ & 0xffffff));
      }
   }
}


/*** PIPE_FORMAT_Z24S8_UNORM ***/

/**
 * Return Z component as four float in [0,1].  Stencil part ignored.
 */
static void
z24s8_get_tile_rgba(const unsigned *src,
                    unsigned w, unsigned h,
                    uint8_t *p)
{
   const double scale = 1.0 / ((1 << 24) - 1);
   unsigned i, j;

   for (i = 0; i < h; i++) {
      for (j = 0; j < w; j++) {
         TILE_PIXEL(p, j, i, 0) =
         TILE_PIXEL(p, j, i, 1) =
         TILE_PIXEL(p, j, i, 2) =
         TILE_PIXEL(p, j, i, 3) = (float) (scale * (*src++ >> 8));
      }
   }
}


/*** PIPE_FORMAT_Z32_FLOAT ***/

/**
 * Return each Z value as four floats in [0,1].
 */
static void
z32f_get_tile_rgba(const float *src,
                   unsigned w, unsigned h,
                   uint8_t *p)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      for (j = 0; j < w; j++) {
         TILE_PIXEL(p, j, i, 0) =
         TILE_PIXEL(p, j, i, 1) =
         TILE_PIXEL(p, j, i, 2) =
         TILE_PIXEL(p, j, i, 3) = *src++;
      }
   }
}


/*** PIPE_FORMAT_YCBCR / PIPE_FORMAT_YCBCR_REV ***/

/**
 * Convert YCbCr (or YCrCb) to RGBA.
 */
static void
ycbcr_get_tile_rgba(const ushort *src,
                    unsigned w, unsigned h,
                    uint8_t *p,
                    boolean rev)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      /* do two texels at a time */
      for (j = 0; j < (w & ~1); j += 2, src += 2) {
         const ushort t0 = src[0];
         const ushort t1 = src[1];
         const ubyte y0 = (t0 >> 8) & 0xff;  /* luminance */
         const ubyte y1 = (t1 >> 8) & 0xff;  /* luminance */
         ubyte cb, cr;
         float r, g, b;

         if (rev) {
            cb = t1 & 0xff;         /* chroma U */
            cr = t0 & 0xff;         /* chroma V */
         }
         else {
            cb = t0 & 0xff;         /* chroma U */
            cr = t1 & 0xff;         /* chroma V */
         }

         /* even pixel: y0,cr,cb */
         r = 1.164f * (y0-16) + 1.596f * (cr-128);
         g = 1.164f * (y0-16) - 0.813f * (cr-128) - 0.391f * (cb-128);
         b = 1.164f * (y0-16) + 2.018f * (cb-128);
         TILE_PIXEL(p, j, i, 0) = r;
         TILE_PIXEL(p, j, i, 1) = g;
         TILE_PIXEL(p, j, i, 2) = b;
         TILE_PIXEL(p, j, i, 3) = 255;

         /* odd pixel: use y1,cr,cb */
         r = 1.164f * (y1-16) + 1.596f * (cr-128);
         g = 1.164f * (y1-16) - 0.813f * (cr-128) - 0.391f * (cb-128);
         b = 1.164f * (y1-16) + 2.018f * (cb-128);
         TILE_PIXEL(p, j + 1, i, 0) = r;
         TILE_PIXEL(p, j + 1, i, 1) = g;
         TILE_PIXEL(p, j + 1, i, 2) = b;
         TILE_PIXEL(p, j + 1, i, 3) = 255;
      }
      /* do the last texel */
      if (w & 1) {
         const ushort t0 = src[0];
         const ushort t1 = src[1];
         const ubyte y0 = (t0 >> 8) & 0xff;  /* luminance */
         ubyte cb, cr;
         float r, g, b;

         if (rev) {
            cb = t1 & 0xff;         /* chroma U */
            cr = t0 & 0xff;         /* chroma V */
         }
         else {
            cb = t0 & 0xff;         /* chroma U */
            cr = t1 & 0xff;         /* chroma V */
         }

         /* even pixel: y0,cr,cb */
         r = 1.164f * (y0-16) + 1.596f * (cr-128);
         g = 1.164f * (y0-16) - 0.813f * (cr-128) - 0.391f * (cb-128);
         b = 1.164f * (y0-16) + 2.018f * (cb-128);
         TILE_PIXEL(p, j, i, 0) = r;
         TILE_PIXEL(p, j, i, 1) = g;
         TILE_PIXEL(p, j, i, 2) = b;
         TILE_PIXEL(p, j, i, 3) = 255;
      }
   }
}


static void
fake_get_tile_rgba(const ushort *src,
                   unsigned w, unsigned h,
                   uint8_t *p)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      for (j = 0; j < w; j++) {
         TILE_PIXEL(p, j, i, 0) =
         TILE_PIXEL(p, j, i, 1) =
         TILE_PIXEL(p, j, i, 2) =
         TILE_PIXEL(p, j, i, 3) = (i ^ j) & 1 ? 255 : 0;
      }
   }
}


static void
lp_tile_raw_to_rgba_soa(enum pipe_format format,
                        void *src,
                        uint w, uint h,
                        uint8_t *p)
{
   switch (format) {
   case PIPE_FORMAT_A8R8G8B8_UNORM:
      a8r8g8b8_get_tile_rgba((unsigned *) src, w, h, p);
      break;
   case PIPE_FORMAT_X8R8G8B8_UNORM:
      x8r8g8b8_get_tile_rgba((unsigned *) src, w, h, p);
      break;
   case PIPE_FORMAT_B8G8R8A8_UNORM:
      b8g8r8a8_get_tile_rgba((unsigned *) src, w, h, p);
      break;
   case PIPE_FORMAT_A1R5G5B5_UNORM:
      a1r5g5b5_get_tile_rgba((ushort *) src, w, h, p);
      break;
   case PIPE_FORMAT_A4R4G4B4_UNORM:
      a4r4g4b4_get_tile_rgba((ushort *) src, w, h, p);
      break;
   case PIPE_FORMAT_R5G6B5_UNORM:
      r5g6b5_get_tile_rgba((ushort *) src, w, h, p);
      break;
   case PIPE_FORMAT_L8_UNORM:
      l8_get_tile_rgba((ubyte *) src, w, h, p);
      break;
   case PIPE_FORMAT_A8_UNORM:
      a8_get_tile_rgba((ubyte *) src, w, h, p);
      break;
   case PIPE_FORMAT_I8_UNORM:
      i8_get_tile_rgba((ubyte *) src, w, h, p);
      break;
   case PIPE_FORMAT_A8L8_UNORM:
      a8l8_get_tile_rgba((ushort *) src, w, h, p);
      break;
   case PIPE_FORMAT_R16_SNORM:
      r16_get_tile_rgba((short *) src, w, h, p);
      break;
   case PIPE_FORMAT_R16G16B16A16_SNORM:
      r16g16b16a16_get_tile_rgba((short *) src, w, h, p);
      break;
   case PIPE_FORMAT_Z16_UNORM:
      z16_get_tile_rgba((ushort *) src, w, h, p);
      break;
   case PIPE_FORMAT_Z32_UNORM:
      z32_get_tile_rgba((unsigned *) src, w, h, p);
      break;
   case PIPE_FORMAT_S8Z24_UNORM:
   case PIPE_FORMAT_X8Z24_UNORM:
      s8z24_get_tile_rgba((unsigned *) src, w, h, p);
      break;
   case PIPE_FORMAT_Z24S8_UNORM:
   case PIPE_FORMAT_Z24X8_UNORM:
      z24s8_get_tile_rgba((unsigned *) src, w, h, p);
      break;
   case PIPE_FORMAT_Z32_FLOAT:
      z32f_get_tile_rgba((float *) src, w, h, p);
      break;
   case PIPE_FORMAT_YCBCR:
      ycbcr_get_tile_rgba((ushort *) src, w, h, p, FALSE);
      break;
   case PIPE_FORMAT_YCBCR_REV:
      ycbcr_get_tile_rgba((ushort *) src, w, h, p, TRUE);
      break;
   default:
      debug_printf("%s: unsupported format %s\n", __FUNCTION__, pf_name(format));
      fake_get_tile_rgba(src, w, h, p);
   }
}


void
lp_get_tile_rgba_soa(struct pipe_transfer *pt,
                     uint x, uint y,
                     uint8_t *p)
{
   uint w = TILE_SIZE, h = TILE_SIZE;
   void *packed;

   if (pipe_clip_tile(x, y, &w, &h, pt))
      return;

   packed = MALLOC(pf_get_nblocks(&pt->block, w, h) * pt->block.size);

   if (!packed)
      return;

   if(pt->format == PIPE_FORMAT_YCBCR || pt->format == PIPE_FORMAT_YCBCR_REV)
      assert((x & 1) == 0);

   pipe_get_tile_raw(pt, x, y, w, h, packed, 0);

   lp_tile_raw_to_rgba_soa(pt->format, packed, w, h, p);

   FREE(packed);
}


void
lp_put_tile_rgba_soa(struct pipe_transfer *pt,
                     uint x, uint y,
                     const uint8_t *p)
{
   uint w = TILE_SIZE, h = TILE_SIZE;
   void *packed;

   if (pipe_clip_tile(x, y, &w, &h, pt))
      return;

   packed = MALLOC(pf_get_nblocks(&pt->block, w, h) * pt->block.size);

   if (!packed)
      return;

   switch (pt->format) {
   case PIPE_FORMAT_A8R8G8B8_UNORM:
      a8r8g8b8_put_tile_rgba((unsigned *) packed, w, h, p);
      break;
   case PIPE_FORMAT_X8R8G8B8_UNORM:
      x8r8g8b8_put_tile_rgba((unsigned *) packed, w, h, p);
      break;
   case PIPE_FORMAT_B8G8R8A8_UNORM:
      b8g8r8a8_put_tile_rgba((unsigned *) packed, w, h, p);
      break;
   case PIPE_FORMAT_A1R5G5B5_UNORM:
      a1r5g5b5_put_tile_rgba((ushort *) packed, w, h, p);
      break;
   case PIPE_FORMAT_R5G6B5_UNORM:
      r5g6b5_put_tile_rgba((ushort *) packed, w, h, p);
      break;
   case PIPE_FORMAT_R8G8B8A8_UNORM:
      assert(0);
      break;
   case PIPE_FORMAT_A4R4G4B4_UNORM:
      a4r4g4b4_put_tile_rgba((ushort *) packed, w, h, p);
      break;
   case PIPE_FORMAT_L8_UNORM:
      l8_put_tile_rgba((ubyte *) packed, w, h, p);
      break;
   case PIPE_FORMAT_A8_UNORM:
      a8_put_tile_rgba((ubyte *) packed, w, h, p);
      break;
   case PIPE_FORMAT_I8_UNORM:
      i8_put_tile_rgba((ubyte *) packed, w, h, p);
      break;
   case PIPE_FORMAT_A8L8_UNORM:
      a8l8_put_tile_rgba((ushort *) packed, w, h, p);
      break;
   case PIPE_FORMAT_R16_SNORM:
      r16_put_tile_rgba((short *) packed, w, h, p);
      break;
   case PIPE_FORMAT_R16G16B16A16_SNORM:
      r16g16b16a16_put_tile_rgba((short *) packed, w, h, p);
      break;
   case PIPE_FORMAT_Z16_UNORM:
      /*z16_put_tile_rgba((ushort *) packed, w, h, p);*/
      break;
   case PIPE_FORMAT_Z32_UNORM:
      /*z32_put_tile_rgba((unsigned *) packed, w, h, p);*/
      break;
   case PIPE_FORMAT_S8Z24_UNORM:
   case PIPE_FORMAT_X8Z24_UNORM:
      /*s8z24_put_tile_rgba((unsigned *) packed, w, h, p);*/
      break;
   case PIPE_FORMAT_Z24S8_UNORM:
   case PIPE_FORMAT_Z24X8_UNORM:
      /*z24s8_put_tile_rgba((unsigned *) packed, w, h, p);*/
      break;
   default:
      debug_printf("%s: unsupported format %s\n", __FUNCTION__, pf_name(pt->format));
   }

   pipe_put_tile_raw(pt, x, y, w, h, packed, 0);

   FREE(packed);
}



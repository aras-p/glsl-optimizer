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
#include "util/u_inlines.h"

#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_rect.h"
#include "util/u_tile.h"


/**
 * Move raw block of pixels from transfer object to user memory.
 */
void
pipe_get_tile_raw(struct pipe_transfer *pt,
                  uint x, uint y, uint w, uint h,
                  void *dst, int dst_stride)
{
   struct pipe_screen *screen = pt->texture->screen;
   const void *src;

   if (dst_stride == 0)
      dst_stride = util_format_get_stride(pt->texture->format, w);

   if (pipe_clip_tile(x, y, &w, &h, pt))
      return;

   src = screen->transfer_map(screen, pt);
   assert(src);
   if(!src)
      return;

   util_copy_rect(dst, pt->texture->format, dst_stride, 0, 0, w, h, src, pt->stride, x, y);

   screen->transfer_unmap(screen, pt);
}


/**
 * Move raw block of pixels from user memory to transfer object.
 */
void
pipe_put_tile_raw(struct pipe_transfer *pt,
                  uint x, uint y, uint w, uint h,
                  const void *src, int src_stride)
{
   struct pipe_screen *screen = pt->texture->screen;
   void *dst;
   enum pipe_format format = pt->texture->format;

   if (src_stride == 0)
      src_stride = util_format_get_stride(format, w);

   if (pipe_clip_tile(x, y, &w, &h, pt))
      return;

   dst = screen->transfer_map(screen, pt);
   assert(dst);
   if(!dst)
      return;

   util_copy_rect(dst, format, pt->stride, x, y, w, h, src, src_stride, 0, 0);

   screen->transfer_unmap(screen, pt);
}




/** Convert short in [-32768,32767] to GLfloat in [-1.0,1.0] */
#define SHORT_TO_FLOAT(S)   ((2.0F * (S) + 1.0F) * (1.0F/65535.0F))

#define UNCLAMPED_FLOAT_TO_SHORT(us, f)  \
   us = ( (short) ( CLAMP((f), -1.0, 1.0) * 32767.0F) )



/*** PIPE_FORMAT_B8G8R8A8_UNORM ***/

static void
a8r8g8b8_get_tile_rgba(const unsigned *src,
                       unsigned w, unsigned h,
                       float *p,
                       unsigned dst_stride)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++, pRow += 4) {
         const unsigned pixel = *src++;
         pRow[0] = ubyte_to_float((pixel >> 16) & 0xff);
         pRow[1] = ubyte_to_float((pixel >>  8) & 0xff);
         pRow[2] = ubyte_to_float((pixel >>  0) & 0xff);
         pRow[3] = ubyte_to_float((pixel >> 24) & 0xff);
      }
      p += dst_stride;
   }
}


static void
a8r8g8b8_put_tile_rgba(unsigned *dst,
                       unsigned w, unsigned h,
                       const float *p,
                       unsigned src_stride)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      const float *pRow = p;
      for (j = 0; j < w; j++, pRow += 4) {
         unsigned r, g, b, a;
         r = float_to_ubyte(pRow[0]);
         g = float_to_ubyte(pRow[1]);
         b = float_to_ubyte(pRow[2]);
         a = float_to_ubyte(pRow[3]);
         *dst++ = (a << 24) | (r << 16) | (g << 8) | b;
      }
      p += src_stride;
   }
}


/*** PIPE_FORMAT_B8G8R8X8_UNORM ***/

static void
x8r8g8b8_get_tile_rgba(const unsigned *src,
                       unsigned w, unsigned h,
                       float *p,
                       unsigned dst_stride)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++, pRow += 4) {
         const unsigned pixel = *src++;
         pRow[0] = ubyte_to_float((pixel >> 16) & 0xff);
         pRow[1] = ubyte_to_float((pixel >>  8) & 0xff);
         pRow[2] = ubyte_to_float((pixel >>  0) & 0xff);
         pRow[3] = 1.0F;
      }
      p += dst_stride;
   }
}


static void
x8r8g8b8_put_tile_rgba(unsigned *dst,
                       unsigned w, unsigned h,
                       const float *p,
                       unsigned src_stride)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      const float *pRow = p;
      for (j = 0; j < w; j++, pRow += 4) {
         unsigned r, g, b;
         r = float_to_ubyte(pRow[0]);
         g = float_to_ubyte(pRow[1]);
         b = float_to_ubyte(pRow[2]);
         *dst++ = (0xff << 24) | (r << 16) | (g << 8) | b;
      }
      p += src_stride;
   }
}


/*** PIPE_FORMAT_A8R8G8B8_UNORM ***/

static void
b8g8r8a8_get_tile_rgba(const unsigned *src,
                       unsigned w, unsigned h,
                       float *p,
                       unsigned dst_stride)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++, pRow += 4) {
         const unsigned pixel = *src++;
         pRow[0] = ubyte_to_float((pixel >>  8) & 0xff);
         pRow[1] = ubyte_to_float((pixel >> 16) & 0xff);
         pRow[2] = ubyte_to_float((pixel >> 24) & 0xff);
         pRow[3] = ubyte_to_float((pixel >>  0) & 0xff);
      }
      p += dst_stride;
   }
}


static void
b8g8r8a8_put_tile_rgba(unsigned *dst,
                       unsigned w, unsigned h,
                       const float *p,
                       unsigned src_stride)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      const float *pRow = p;
      for (j = 0; j < w; j++, pRow += 4) {
         unsigned r, g, b, a;
         r = float_to_ubyte(pRow[0]);
         g = float_to_ubyte(pRow[1]);
         b = float_to_ubyte(pRow[2]);
         a = float_to_ubyte(pRow[3]);
         *dst++ = (b << 24) | (g << 16) | (r << 8) | a;
      }
      p += src_stride;
   }
}


/*** PIPE_FORMAT_A8B8G8R8_UNORM ***/

static void
r8g8b8a8_get_tile_rgba(const unsigned *src,
                       unsigned w, unsigned h,
                       float *p,
                       unsigned dst_stride)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++, pRow += 4) {
         const unsigned pixel = *src++;
         pRow[0] = ubyte_to_float((pixel >> 24) & 0xff);
         pRow[1] = ubyte_to_float((pixel >> 16) & 0xff);
         pRow[2] = ubyte_to_float((pixel >>  8) & 0xff);
         pRow[3] = ubyte_to_float((pixel >>  0) & 0xff);
      }
      p += dst_stride;
   }
}


static void
r8g8b8a8_put_tile_rgba(unsigned *dst,
                       unsigned w, unsigned h,
                       const float *p,
                       unsigned src_stride)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      const float *pRow = p;
      for (j = 0; j < w; j++, pRow += 4) {
         unsigned r, g, b, a;
         r = float_to_ubyte(pRow[0]);
         g = float_to_ubyte(pRow[1]);
         b = float_to_ubyte(pRow[2]);
         a = float_to_ubyte(pRow[3]);
         *dst++ = (r << 24) | (g << 16) | (b << 8) | a;
      }
      p += src_stride;
   }
}


/*** PIPE_FORMAT_B5G5R5A1_UNORM ***/

static void
a1r5g5b5_get_tile_rgba(const ushort *src,
                       unsigned w, unsigned h,
                       float *p,
                       unsigned dst_stride)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++, pRow += 4) {
         const ushort pixel = *src++;
         pRow[0] = ((pixel >> 10) & 0x1f) * (1.0f / 31.0f);
         pRow[1] = ((pixel >>  5) & 0x1f) * (1.0f / 31.0f);
         pRow[2] = ((pixel      ) & 0x1f) * (1.0f / 31.0f);
         pRow[3] = ((pixel >> 15)       ) * 1.0f;
      }
      p += dst_stride;
   }
}


static void
a1r5g5b5_put_tile_rgba(ushort *dst,
                       unsigned w, unsigned h,
                       const float *p,
                       unsigned src_stride)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      const float *pRow = p;
      for (j = 0; j < w; j++, pRow += 4) {
         unsigned r, g, b, a;
         r = float_to_ubyte(pRow[0]);
         g = float_to_ubyte(pRow[1]);
         b = float_to_ubyte(pRow[2]);
         a = float_to_ubyte(pRow[3]);
         r = r >> 3;  /* 5 bits */
         g = g >> 3;  /* 5 bits */
         b = b >> 3;  /* 5 bits */
         a = a >> 7;  /* 1 bit */
         *dst++ = (a << 15) | (r << 10) | (g << 5) | b;
      }
      p += src_stride;
   }
}


/*** PIPE_FORMAT_B4G4R4A4_UNORM ***/

static void
a4r4g4b4_get_tile_rgba(const ushort *src,
                       unsigned w, unsigned h,
                       float *p,
                       unsigned dst_stride)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++, pRow += 4) {
         const ushort pixel = *src++;
         pRow[0] = ((pixel >>  8) & 0xf) * (1.0f / 15.0f);
         pRow[1] = ((pixel >>  4) & 0xf) * (1.0f / 15.0f);
         pRow[2] = ((pixel      ) & 0xf) * (1.0f / 15.0f);
         pRow[3] = ((pixel >> 12)      ) * (1.0f / 15.0f);
      }
      p += dst_stride;
   }
}


static void
a4r4g4b4_put_tile_rgba(ushort *dst,
                       unsigned w, unsigned h,
                       const float *p,
                       unsigned src_stride)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      const float *pRow = p;
      for (j = 0; j < w; j++, pRow += 4) {
         unsigned r, g, b, a;
         r = float_to_ubyte(pRow[0]);
         g = float_to_ubyte(pRow[1]);
         b = float_to_ubyte(pRow[2]);
         a = float_to_ubyte(pRow[3]);
         r >>= 4;
         g >>= 4;
         b >>= 4;
         a >>= 4;
         *dst++ = (a << 12) | (r << 8) | (g << 4) | b;
      }
      p += src_stride;
   }
}


/*** PIPE_FORMAT_B5G6R5_UNORM ***/

static void
r5g6b5_get_tile_rgba(const ushort *src,
                     unsigned w, unsigned h,
                     float *p,
                     unsigned dst_stride)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++, pRow += 4) {
         const ushort pixel = *src++;
         pRow[0] = ((pixel >> 11) & 0x1f) * (1.0f / 31.0f);
         pRow[1] = ((pixel >>  5) & 0x3f) * (1.0f / 63.0f);
         pRow[2] = ((pixel      ) & 0x1f) * (1.0f / 31.0f);
         pRow[3] = 1.0f;
      }
      p += dst_stride;
   }
}


static void
r5g6b5_put_tile_rgba(ushort *dst,
                     unsigned w, unsigned h,
                     const float *p,
                     unsigned src_stride)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      const float *pRow = p;
      for (j = 0; j < w; j++, pRow += 4) {
         uint r = (uint) (CLAMP(pRow[0], 0.0, 1.0) * 31.0);
         uint g = (uint) (CLAMP(pRow[1], 0.0, 1.0) * 63.0);
         uint b = (uint) (CLAMP(pRow[2], 0.0, 1.0) * 31.0);
         *dst++ = (r << 11) | (g << 5) | (b);
      }
      p += src_stride;
   }
}



/*** PIPE_FORMAT_R8G8B8_UNORM ***/

static void
r8g8b8_get_tile_rgba(const ubyte *src,
                     unsigned w, unsigned h,
                     float *p,
                     unsigned dst_stride)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++, pRow += 4) {
         pRow[0] = ubyte_to_float(src[0]);
         pRow[1] = ubyte_to_float(src[1]);
         pRow[2] = ubyte_to_float(src[2]);
         pRow[3] = 1.0f;
         src += 3;
      }
      p += dst_stride;
   }
}


static void
r8g8b8_put_tile_rgba(ubyte *dst,
                     unsigned w, unsigned h,
                     const float *p,
                     unsigned src_stride)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      const float *pRow = p;
      for (j = 0; j < w; j++, pRow += 4) {
         dst[0] = float_to_ubyte(pRow[0]);
         dst[1] = float_to_ubyte(pRow[1]);
         dst[2] = float_to_ubyte(pRow[2]);
         dst += 3;
      }
      p += src_stride;
   }
}



/*** PIPE_FORMAT_Z16_UNORM ***/

/**
 * Return each Z value as four floats in [0,1].
 */
static void
z16_get_tile_rgba(const ushort *src,
                  unsigned w, unsigned h,
                  float *p,
                  unsigned dst_stride)
{
   const float scale = 1.0f / 65535.0f;
   unsigned i, j;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++, pRow += 4) {
         pRow[0] =
         pRow[1] =
         pRow[2] =
         pRow[3] = *src++ * scale;
      }
      p += dst_stride;
   }
}




/*** PIPE_FORMAT_L8_UNORM ***/

static void
l8_get_tile_rgba(const ubyte *src,
                 unsigned w, unsigned h,
                 float *p,
                 unsigned dst_stride)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++, src++, pRow += 4) {
         pRow[0] =
         pRow[1] =
         pRow[2] = ubyte_to_float(*src);
         pRow[3] = 1.0;
      }
      p += dst_stride;
   }
}


static void
l8_put_tile_rgba(ubyte *dst,
                 unsigned w, unsigned h,
                 const float *p,
                 unsigned src_stride)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      const float *pRow = p;
      for (j = 0; j < w; j++, pRow += 4) {
         unsigned r;
         r = float_to_ubyte(pRow[0]);
         *dst++ = (ubyte) r;
      }
      p += src_stride;
   }
}



/*** PIPE_FORMAT_A8_UNORM ***/

static void
a8_get_tile_rgba(const ubyte *src,
                 unsigned w, unsigned h,
                 float *p,
                 unsigned dst_stride)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++, src++, pRow += 4) {
         pRow[0] =
         pRow[1] =
         pRow[2] = 0.0;
         pRow[3] = ubyte_to_float(*src);
      }
      p += dst_stride;
   }
}


static void
a8_put_tile_rgba(ubyte *dst,
                 unsigned w, unsigned h,
                 const float *p,
                 unsigned src_stride)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      const float *pRow = p;
      for (j = 0; j < w; j++, pRow += 4) {
         unsigned a;
         a = float_to_ubyte(pRow[3]);
         *dst++ = (ubyte) a;
      }
      p += src_stride;
   }
}



/*** PIPE_FORMAT_R16_SNORM ***/

static void
r16_get_tile_rgba(const short *src,
                  unsigned w, unsigned h,
                  float *p,
                  unsigned dst_stride)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++, src++, pRow += 4) {
         pRow[0] = SHORT_TO_FLOAT(src[0]);
         pRow[1] =
         pRow[2] = 0.0;
         pRow[3] = 1.0;
      }
      p += dst_stride;
   }
}


static void
r16_put_tile_rgba(short *dst,
                  unsigned w, unsigned h,
                  const float *p,
                  unsigned src_stride)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      const float *pRow = p;
      for (j = 0; j < w; j++, dst++, pRow += 4) {
         UNCLAMPED_FLOAT_TO_SHORT(dst[0], pRow[0]);
      }
      p += src_stride;
   }
}


/*** PIPE_FORMAT_R16G16B16A16_SNORM ***/

static void
r16g16b16a16_get_tile_rgba(const short *src,
                           unsigned w, unsigned h,
                           float *p,
                           unsigned dst_stride)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++, src += 4, pRow += 4) {
         pRow[0] = SHORT_TO_FLOAT(src[0]);
         pRow[1] = SHORT_TO_FLOAT(src[1]);
         pRow[2] = SHORT_TO_FLOAT(src[2]);
         pRow[3] = SHORT_TO_FLOAT(src[3]);
      }
      p += dst_stride;
   }
}


static void
r16g16b16a16_put_tile_rgba(short *dst,
                           unsigned w, unsigned h,
                           const float *p,
                           unsigned src_stride)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      const float *pRow = p;
      for (j = 0; j < w; j++, dst += 4, pRow += 4) {
         UNCLAMPED_FLOAT_TO_SHORT(dst[0], pRow[0]);
         UNCLAMPED_FLOAT_TO_SHORT(dst[1], pRow[1]);
         UNCLAMPED_FLOAT_TO_SHORT(dst[2], pRow[2]);
         UNCLAMPED_FLOAT_TO_SHORT(dst[3], pRow[3]);
      }
      p += src_stride;
   }
}


/*** PIPE_FORMAT_A8B8G8R8_SRGB ***/

/**
 * Convert an 8-bit sRGB value from non-linear space to a
 * linear RGB value in [0, 1].
 * Implemented with a 256-entry lookup table.
 */
static INLINE float
srgb_to_linear(ubyte cs8)
{
   static float table[256];
   static boolean tableReady = FALSE;
   if (!tableReady) {
      /* compute lookup table now */
      uint i;
      for (i = 0; i < 256; i++) {
         const float cs = ubyte_to_float(i);
         if (cs <= 0.04045) {
            table[i] = cs / 12.92f;
         }
         else {
            table[i] = (float) powf((cs + 0.055) / 1.055, 2.4);
         }
      }
      tableReady = TRUE;
   }
   return table[cs8];
}


/**
 * Convert linear float in [0,1] to an srgb ubyte value in [0,255].
 * XXX this hasn't been tested (render to srgb surface).
 * XXX this needs optimization.
 */
static INLINE ubyte
linear_to_srgb(float cl)
{
   if (cl >= 1.0F)
      return 255;
   else if (cl >= 0.0031308F)
      return float_to_ubyte(1.055F * powf(cl, 0.41666F) - 0.055F);
   else if (cl > 0.0F)
      return float_to_ubyte(12.92F * cl);
   else
      return 0.0;
}


static void
a8r8g8b8_srgb_get_tile_rgba(const unsigned *src,
                            unsigned w, unsigned h,
                            float *p,
                            unsigned dst_stride)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++, pRow += 4) {
         const unsigned pixel = *src++;
         pRow[0] = srgb_to_linear((pixel >> 16) & 0xff);
         pRow[1] = srgb_to_linear((pixel >>  8) & 0xff);
         pRow[2] = srgb_to_linear((pixel >>  0) & 0xff);
         pRow[3] = ubyte_to_float((pixel >> 24) & 0xff);
      }
      p += dst_stride;
   }
}

static void
a8r8g8b8_srgb_put_tile_rgba(unsigned *dst,
                            unsigned w, unsigned h,
                            const float *p,
                            unsigned src_stride)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      const float *pRow = p;
      for (j = 0; j < w; j++, pRow += 4) {
         unsigned r, g, b, a;
         r = linear_to_srgb(pRow[0]);
         g = linear_to_srgb(pRow[1]);
         b = linear_to_srgb(pRow[2]);
         a = float_to_ubyte(pRow[3]);
         *dst++ = (a << 24) | (r << 16) | (g << 8) | b;
      }
      p += src_stride;
   }
}


/*** PIPE_FORMAT_L8A8_SRGB ***/

static void
a8l8_srgb_get_tile_rgba(const ushort *src,
                        unsigned w, unsigned h,
                        float *p,
                        unsigned dst_stride)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++, pRow += 4) {
         ushort p = *src++;
         pRow[0] =
         pRow[1] =
         pRow[2] = srgb_to_linear(p & 0xff);
         pRow[3] = ubyte_to_float(p >> 8);
      }
      p += dst_stride;
   }
}

static void
a8l8_srgb_put_tile_rgba(ushort *dst,
                        unsigned w, unsigned h,
                        const float *p,
                        unsigned src_stride)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      const float *pRow = p;
      for (j = 0; j < w; j++, pRow += 4) {
         unsigned r, a;
         r = linear_to_srgb(pRow[0]);
         a = float_to_ubyte(pRow[3]);
         *dst++ = (a << 8) | r;
      }
      p += src_stride;
   }
}


/*** PIPE_FORMAT_L8_SRGB ***/

static void
l8_srgb_get_tile_rgba(const ubyte *src,
                      unsigned w, unsigned h,
                      float *p,
                      unsigned dst_stride)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++, src++, pRow += 4) {
         pRow[0] =
         pRow[1] =
         pRow[2] = srgb_to_linear(*src);
         pRow[3] = 1.0;
      }
      p += dst_stride;
   }
}

static void
l8_srgb_put_tile_rgba(ubyte *dst,
                      unsigned w, unsigned h,
                      const float *p,
                      unsigned src_stride)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      const float *pRow = p;
      for (j = 0; j < w; j++, pRow += 4) {
         unsigned r;
         r = linear_to_srgb(pRow[0]);
         *dst++ = (ubyte) r;
      }
      p += src_stride;
   }
}


/*** PIPE_FORMAT_I8_UNORM ***/

static void
i8_get_tile_rgba(const ubyte *src,
                 unsigned w, unsigned h,
                 float *p,
                 unsigned dst_stride)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++, src++, pRow += 4) {
         pRow[0] =
         pRow[1] =
         pRow[2] =
         pRow[3] = ubyte_to_float(*src);
      }
      p += dst_stride;
   }
}


static void
i8_put_tile_rgba(ubyte *dst,
                 unsigned w, unsigned h,
                 const float *p,
                 unsigned src_stride)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      const float *pRow = p;
      for (j = 0; j < w; j++, pRow += 4) {
         unsigned r;
         r = float_to_ubyte(pRow[0]);
         *dst++ = (ubyte) r;
      }
      p += src_stride;
   }
}


/*** PIPE_FORMAT_L8A8_UNORM ***/

static void
a8l8_get_tile_rgba(const ushort *src,
                   unsigned w, unsigned h,
                   float *p,
                   unsigned dst_stride)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++, pRow += 4) {
         ushort p = *src++;
         pRow[0] =
         pRow[1] =
         pRow[2] = ubyte_to_float(p & 0xff);
         pRow[3] = ubyte_to_float(p >> 8);
      }
      p += dst_stride;
   }
}


static void
a8l8_put_tile_rgba(ushort *dst,
                   unsigned w, unsigned h,
                   const float *p,
                   unsigned src_stride)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      const float *pRow = p;
      for (j = 0; j < w; j++, pRow += 4) {
         unsigned r, a;
         r = float_to_ubyte(pRow[0]);
         a = float_to_ubyte(pRow[3]);
         *dst++ = (a << 8) | r;
      }
      p += src_stride;
   }
}




/*** PIPE_FORMAT_Z32_UNORM ***/

/**
 * Return each Z value as four floats in [0,1].
 */
static void
z32_get_tile_rgba(const unsigned *src,
                  unsigned w, unsigned h,
                  float *p,
                  unsigned dst_stride)
{
   const double scale = 1.0 / (double) 0xffffffff;
   unsigned i, j;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++, pRow += 4) {
         pRow[0] =
         pRow[1] =
         pRow[2] =
         pRow[3] = (float) (*src++ * scale);
      }
      p += dst_stride;
   }
}


/*** PIPE_FORMAT_Z24S8_UNORM ***/

/**
 * Return Z component as four float in [0,1].  Stencil part ignored.
 */
static void
s8z24_get_tile_rgba(const unsigned *src,
                    unsigned w, unsigned h,
                    float *p,
                    unsigned dst_stride)
{
   const double scale = 1.0 / ((1 << 24) - 1);
   unsigned i, j;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++, pRow += 4) {
         pRow[0] =
         pRow[1] =
         pRow[2] =
         pRow[3] = (float) (scale * (*src++ & 0xffffff));
      }
      p += dst_stride;
   }
}


/*** PIPE_FORMAT_S8Z24_UNORM ***/

/**
 * Return Z component as four float in [0,1].  Stencil part ignored.
 */
static void
z24s8_get_tile_rgba(const unsigned *src,
                    unsigned w, unsigned h,
                    float *p,
                    unsigned dst_stride)
{
   const double scale = 1.0 / ((1 << 24) - 1);
   unsigned i, j;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++, pRow += 4) {
         pRow[0] =
         pRow[1] =
         pRow[2] =
         pRow[3] = (float) (scale * (*src++ >> 8));
      }
      p += dst_stride;
   }
}


/*** PIPE_FORMAT_Z32_FLOAT ***/

/**
 * Return each Z value as four floats in [0,1].
 */
static void
z32f_get_tile_rgba(const float *src,
                   unsigned w, unsigned h,
                   float *p,
                   unsigned dst_stride)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++, pRow += 4) {
         pRow[0] =
         pRow[1] =
         pRow[2] =
         pRow[3] = *src++;
      }
      p += dst_stride;
   }
}


/*** PIPE_FORMAT_UYVY / PIPE_FORMAT_YUYV ***/

/**
 * Convert YCbCr (or YCrCb) to RGBA.
 */
static void
ycbcr_get_tile_rgba(const ushort *src,
                    unsigned w, unsigned h,
                    float *p,
                    unsigned dst_stride,
                    boolean rev)
{
   const float scale = 1.0f / 255.0f;
   unsigned i, j;

   for (i = 0; i < h; i++) {
      float *pRow = p;
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
         pRow[0] = r * scale;
         pRow[1] = g * scale;
         pRow[2] = b * scale;
         pRow[3] = 1.0f;
         pRow += 4;

         /* odd pixel: use y1,cr,cb */
         r = 1.164f * (y1-16) + 1.596f * (cr-128);
         g = 1.164f * (y1-16) - 0.813f * (cr-128) - 0.391f * (cb-128);
         b = 1.164f * (y1-16) + 2.018f * (cb-128);
         pRow[0] = r * scale;
         pRow[1] = g * scale;
         pRow[2] = b * scale;
         pRow[3] = 1.0f;
         pRow += 4;

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
         pRow[0] = r * scale;
         pRow[1] = g * scale;
         pRow[2] = b * scale;
         pRow[3] = 1.0f;
         pRow += 4;
      }
      p += dst_stride;
   }
}


void
pipe_tile_raw_to_rgba(enum pipe_format format,
                      void *src,
                      uint w, uint h,
                      float *dst, unsigned dst_stride)
{
   switch (format) {
   case PIPE_FORMAT_B8G8R8A8_UNORM:
      a8r8g8b8_get_tile_rgba((unsigned *) src, w, h, dst, dst_stride);
      break;
   case PIPE_FORMAT_B8G8R8X8_UNORM:
      x8r8g8b8_get_tile_rgba((unsigned *) src, w, h, dst, dst_stride);
      break;
   case PIPE_FORMAT_A8R8G8B8_UNORM:
      b8g8r8a8_get_tile_rgba((unsigned *) src, w, h, dst, dst_stride);
      break;
   case PIPE_FORMAT_A8B8G8R8_UNORM:
      r8g8b8a8_get_tile_rgba((unsigned *) src, w, h, dst, dst_stride);
      break;
   case PIPE_FORMAT_B5G5R5A1_UNORM:
      a1r5g5b5_get_tile_rgba((ushort *) src, w, h, dst, dst_stride);
      break;
   case PIPE_FORMAT_B4G4R4A4_UNORM:
      a4r4g4b4_get_tile_rgba((ushort *) src, w, h, dst, dst_stride);
      break;
   case PIPE_FORMAT_B5G6R5_UNORM:
      r5g6b5_get_tile_rgba((ushort *) src, w, h, dst, dst_stride);
      break;
   case PIPE_FORMAT_R8G8B8_UNORM:
      r8g8b8_get_tile_rgba((ubyte *) src, w, h, dst, dst_stride);
      break;
   case PIPE_FORMAT_L8_UNORM:
      l8_get_tile_rgba((ubyte *) src, w, h, dst, dst_stride);
      break;
   case PIPE_FORMAT_A8_UNORM:
      a8_get_tile_rgba((ubyte *) src, w, h, dst, dst_stride);
      break;
   case PIPE_FORMAT_I8_UNORM:
      i8_get_tile_rgba((ubyte *) src, w, h, dst, dst_stride);
      break;
   case PIPE_FORMAT_L8A8_UNORM:
      a8l8_get_tile_rgba((ushort *) src, w, h, dst, dst_stride);
      break;
   case PIPE_FORMAT_R16_SNORM:
      r16_get_tile_rgba((short *) src, w, h, dst, dst_stride);
      break;
   case PIPE_FORMAT_R16G16B16A16_SNORM:
      r16g16b16a16_get_tile_rgba((short *) src, w, h, dst, dst_stride);
      break;
   case PIPE_FORMAT_B8G8R8A8_SRGB:
      a8r8g8b8_srgb_get_tile_rgba((unsigned *) src, w, h, dst, dst_stride);
      break;
   case PIPE_FORMAT_L8A8_SRGB:
      a8l8_srgb_get_tile_rgba((ushort *) src, w, h, dst, dst_stride);
      break;
   case PIPE_FORMAT_L8_SRGB:
      l8_srgb_get_tile_rgba((ubyte *) src, w, h, dst, dst_stride);
      break;
   case PIPE_FORMAT_Z16_UNORM:
      z16_get_tile_rgba((ushort *) src, w, h, dst, dst_stride);
      break;
   case PIPE_FORMAT_Z32_UNORM:
      z32_get_tile_rgba((unsigned *) src, w, h, dst, dst_stride);
      break;
   case PIPE_FORMAT_Z24S8_UNORM:
   case PIPE_FORMAT_Z24X8_UNORM:
      s8z24_get_tile_rgba((unsigned *) src, w, h, dst, dst_stride);
      break;
   case PIPE_FORMAT_S8Z24_UNORM:
   case PIPE_FORMAT_X8Z24_UNORM:
      z24s8_get_tile_rgba((unsigned *) src, w, h, dst, dst_stride);
      break;
   case PIPE_FORMAT_Z32_FLOAT:
      z32f_get_tile_rgba((float *) src, w, h, dst, dst_stride);
      break;
   case PIPE_FORMAT_UYVY:
      ycbcr_get_tile_rgba((ushort *) src, w, h, dst, dst_stride, FALSE);
      break;
   case PIPE_FORMAT_YUYV:
      ycbcr_get_tile_rgba((ushort *) src, w, h, dst, dst_stride, TRUE);
      break;
   default:
      util_format_read_4f(format,
                          dst, dst_stride * sizeof(float),
                          src, util_format_get_stride(format, w),
                          0, 0, w, h);
   }
}


void
pipe_get_tile_rgba(struct pipe_transfer *pt,
                   uint x, uint y, uint w, uint h,
                   float *p)
{
   unsigned dst_stride = w * 4;
   void *packed;
   enum pipe_format format = pt->texture->format;

   if (pipe_clip_tile(x, y, &w, &h, pt))
      return;

   packed = MALLOC(util_format_get_nblocks(format, w, h) * util_format_get_blocksize(format));

   if (!packed)
      return;

   if(format == PIPE_FORMAT_UYVY || format == PIPE_FORMAT_YUYV)
      assert((x & 1) == 0);

   pipe_get_tile_raw(pt, x, y, w, h, packed, 0);

   pipe_tile_raw_to_rgba(format, packed, w, h, p, dst_stride);

   FREE(packed);
}


void
pipe_put_tile_rgba(struct pipe_transfer *pt,
                   uint x, uint y, uint w, uint h,
                   const float *p)
{
   unsigned src_stride = w * 4;
   void *packed;
   enum pipe_format format = pt->texture->format;

   if (pipe_clip_tile(x, y, &w, &h, pt))
      return;

   packed = MALLOC(util_format_get_nblocks(format, w, h) * util_format_get_blocksize(format));

   if (!packed)
      return;

   switch (format) {
   case PIPE_FORMAT_B8G8R8A8_UNORM:
      a8r8g8b8_put_tile_rgba((unsigned *) packed, w, h, p, src_stride);
      break;
   case PIPE_FORMAT_B8G8R8X8_UNORM:
      x8r8g8b8_put_tile_rgba((unsigned *) packed, w, h, p, src_stride);
      break;
   case PIPE_FORMAT_A8R8G8B8_UNORM:
      b8g8r8a8_put_tile_rgba((unsigned *) packed, w, h, p, src_stride);
      break;
   case PIPE_FORMAT_A8B8G8R8_UNORM:
      r8g8b8a8_put_tile_rgba((unsigned *) packed, w, h, p, src_stride);
      break;
   case PIPE_FORMAT_B5G5R5A1_UNORM:
      a1r5g5b5_put_tile_rgba((ushort *) packed, w, h, p, src_stride);
      break;
   case PIPE_FORMAT_B5G6R5_UNORM:
      r5g6b5_put_tile_rgba((ushort *) packed, w, h, p, src_stride);
      break;
   case PIPE_FORMAT_R8G8B8_UNORM:
      r8g8b8_put_tile_rgba((ubyte *) packed, w, h, p, src_stride);
      break;
   case PIPE_FORMAT_B4G4R4A4_UNORM:
      a4r4g4b4_put_tile_rgba((ushort *) packed, w, h, p, src_stride);
      break;
   case PIPE_FORMAT_L8_UNORM:
      l8_put_tile_rgba((ubyte *) packed, w, h, p, src_stride);
      break;
   case PIPE_FORMAT_A8_UNORM:
      a8_put_tile_rgba((ubyte *) packed, w, h, p, src_stride);
      break;
   case PIPE_FORMAT_I8_UNORM:
      i8_put_tile_rgba((ubyte *) packed, w, h, p, src_stride);
      break;
   case PIPE_FORMAT_L8A8_UNORM:
      a8l8_put_tile_rgba((ushort *) packed, w, h, p, src_stride);
      break;
   case PIPE_FORMAT_R16_SNORM:
      r16_put_tile_rgba((short *) packed, w, h, p, src_stride);
      break;
   case PIPE_FORMAT_R16G16B16A16_SNORM:
      r16g16b16a16_put_tile_rgba((short *) packed, w, h, p, src_stride);
      break;
   case PIPE_FORMAT_B8G8R8A8_SRGB:
      a8r8g8b8_srgb_put_tile_rgba((unsigned *) packed, w, h, p, src_stride);
      break;
   case PIPE_FORMAT_L8A8_SRGB:
      a8l8_srgb_put_tile_rgba((ushort *) packed, w, h, p, src_stride);
      break;
   case PIPE_FORMAT_L8_SRGB:
      l8_srgb_put_tile_rgba((ubyte *) packed, w, h, p, src_stride);
      break;
   case PIPE_FORMAT_Z16_UNORM:
      /*z16_put_tile_rgba((ushort *) packed, w, h, p, src_stride);*/
      break;
   case PIPE_FORMAT_Z32_UNORM:
      /*z32_put_tile_rgba((unsigned *) packed, w, h, p, src_stride);*/
      break;
   case PIPE_FORMAT_Z24S8_UNORM:
   case PIPE_FORMAT_Z24X8_UNORM:
      /*s8z24_put_tile_rgba((unsigned *) packed, w, h, p, src_stride);*/
      break;
   case PIPE_FORMAT_S8Z24_UNORM:
   case PIPE_FORMAT_X8Z24_UNORM:
      /*z24s8_put_tile_rgba((unsigned *) packed, w, h, p, src_stride);*/
      break;
   default:
      util_format_write_4f(format,
                           p, src_stride * sizeof(float),
                           packed, util_format_get_stride(format, w),
                           0, 0, w, h);
   }

   pipe_put_tile_raw(pt, x, y, w, h, packed, 0);

   FREE(packed);
}


/**
 * Get a block of Z values, converted to 32-bit range.
 */
void
pipe_get_tile_z(struct pipe_transfer *pt,
                uint x, uint y, uint w, uint h,
                uint *z)
{
   struct pipe_screen *screen = pt->texture->screen;
   const uint dstStride = w;
   ubyte *map;
   uint *pDest = z;
   uint i, j;
   enum pipe_format format = pt->texture->format;

   if (pipe_clip_tile(x, y, &w, &h, pt))
      return;

   map = (ubyte *)screen->transfer_map(screen, pt);
   if (!map) {
      assert(0);
      return;
   }

   switch (format) {
   case PIPE_FORMAT_Z32_UNORM:
      {
         const uint *ptrc
            = (const uint *)(map  + y * pt->stride + x*4);
         for (i = 0; i < h; i++) {
            memcpy(pDest, ptrc, 4 * w);
            pDest += dstStride;
            ptrc += pt->stride/4;
         }
      }
      break;
   case PIPE_FORMAT_Z24S8_UNORM:
   case PIPE_FORMAT_Z24X8_UNORM:
      {
         const uint *ptrc
            = (const uint *)(map + y * pt->stride + x*4);
         for (i = 0; i < h; i++) {
            for (j = 0; j < w; j++) {
               /* convert 24-bit Z to 32-bit Z */
               pDest[j] = (ptrc[j] << 8) | ((ptrc[j] >> 16) & 0xff);
            }
            pDest += dstStride;
            ptrc += pt->stride/4;
         }
      }
      break;
   case PIPE_FORMAT_S8Z24_UNORM:
   case PIPE_FORMAT_X8Z24_UNORM:
      {
         const uint *ptrc
            = (const uint *)(map + y * pt->stride + x*4);
         for (i = 0; i < h; i++) {
            for (j = 0; j < w; j++) {
               /* convert 24-bit Z to 32-bit Z */
               pDest[j] = (ptrc[j] & 0xffffff00) | ((ptrc[j] >> 24) & 0xff);
            }
            pDest += dstStride;
            ptrc += pt->stride/4;
         }
      }
      break;
   case PIPE_FORMAT_Z16_UNORM:
      {
         const ushort *ptrc
            = (const ushort *)(map + y * pt->stride + x*2);
         for (i = 0; i < h; i++) {
            for (j = 0; j < w; j++) {
               /* convert 16-bit Z to 32-bit Z */
               pDest[j] = (ptrc[j] << 16) | ptrc[j];
            }
            pDest += dstStride;
            ptrc += pt->stride/2;
         }
      }
      break;
   default:
      assert(0);
   }

   screen->transfer_unmap(screen, pt);
}


void
pipe_put_tile_z(struct pipe_transfer *pt,
                uint x, uint y, uint w, uint h,
                const uint *zSrc)
{
   struct pipe_screen *screen = pt->texture->screen;
   const uint srcStride = w;
   const uint *ptrc = zSrc;
   ubyte *map;
   uint i, j;
   enum pipe_format format = pt->texture->format;

   if (pipe_clip_tile(x, y, &w, &h, pt))
      return;

   map = (ubyte *)screen->transfer_map(screen, pt);
   if (!map) {
      assert(0);
      return;
   }

   switch (format) {
   case PIPE_FORMAT_Z32_UNORM:
      {
         uint *pDest = (uint *) (map + y * pt->stride + x*4);
         for (i = 0; i < h; i++) {
            memcpy(pDest, ptrc, 4 * w);
            pDest += pt->stride/4;
            ptrc += srcStride;
         }
      }
      break;
   case PIPE_FORMAT_Z24S8_UNORM:
      {
         uint *pDest = (uint *) (map + y * pt->stride + x*4);
         assert((pt->usage & PIPE_TRANSFER_READ_WRITE) == PIPE_TRANSFER_READ_WRITE);
         for (i = 0; i < h; i++) {
            for (j = 0; j < w; j++) {
               /* convert 32-bit Z to 24-bit Z, preserve stencil */
               pDest[j] = (pDest[j] & 0xff000000) | ptrc[j] >> 8;
            }
            pDest += pt->stride/4;
            ptrc += srcStride;
         }
      }
      break;
   case PIPE_FORMAT_Z24X8_UNORM:
      {
         uint *pDest = (uint *) (map + y * pt->stride + x*4);
         for (i = 0; i < h; i++) {
            for (j = 0; j < w; j++) {
               /* convert 32-bit Z to 24-bit Z (0 stencil) */
               pDest[j] = ptrc[j] >> 8;
            }
            pDest += pt->stride/4;
            ptrc += srcStride;
         }
      }
      break;
   case PIPE_FORMAT_S8Z24_UNORM:
      {
         uint *pDest = (uint *) (map + y * pt->stride + x*4);
         assert((pt->usage & PIPE_TRANSFER_READ_WRITE) == PIPE_TRANSFER_READ_WRITE);
         for (i = 0; i < h; i++) {
            for (j = 0; j < w; j++) {
               /* convert 32-bit Z to 24-bit Z, preserve stencil */
               pDest[j] = (pDest[j] & 0xff) | (ptrc[j] & 0xffffff00);
            }
            pDest += pt->stride/4;
            ptrc += srcStride;
         }
      }
      break;
   case PIPE_FORMAT_X8Z24_UNORM:
      {
         uint *pDest = (uint *) (map + y * pt->stride + x*4);
         for (i = 0; i < h; i++) {
            for (j = 0; j < w; j++) {
               /* convert 32-bit Z to 24-bit Z (0 stencil) */
               pDest[j] = ptrc[j] & 0xffffff00;
            }
            pDest += pt->stride/4;
            ptrc += srcStride;
         }
      }
      break;
   case PIPE_FORMAT_Z16_UNORM:
      {
         ushort *pDest = (ushort *) (map + y * pt->stride + x*2);
         for (i = 0; i < h; i++) {
            for (j = 0; j < w; j++) {
               /* convert 32-bit Z to 16-bit Z */
               pDest[j] = ptrc[j] >> 16;
            }
            pDest += pt->stride/2;
            ptrc += srcStride;
         }
      }
      break;
   default:
      assert(0);
   }

   screen->transfer_unmap(screen, pt);
}



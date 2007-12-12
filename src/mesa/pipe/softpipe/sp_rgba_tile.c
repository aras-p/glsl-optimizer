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
 * XXX eventually this should go into a utility library usable by
 * the state tracker too.
 * Then, remove pipe->get/put_tile_rgba()
 */


#include "pipe/p_defines.h"
#include "pipe/p_util.h"
#include "pipe/p_inlines.h"
#include "sp_surface.h"
#include "sp_rgba_tile.h"


/** Convert short in [-32768,32767] to GLfloat in [-1.0,1.0] */
#define SHORT_TO_FLOAT(S)   ((2.0F * (S) + 1.0F) * (1.0F/65535.0F))

#define UNCLAMPED_FLOAT_TO_SHORT(us, f)  \
   us = ( (short) ( CLAMP((f), -1.0, 1.0) * 32767.0F) )



#define CLIP_TILE \
   do { \
      if (x >= ps->width) \
         return; \
      if (y >= ps->height) \
         return; \
      if (x + w > ps->width) \
         w = ps->width - x; \
      if (y + h > ps->height) \
         h = ps->height - y; \
   } while(0)


/*** PIPE_FORMAT_A8R8G8B8_UNORM ***/

static void
a8r8g8b8_get_tile_rgba(struct pipe_surface *ps,
                       unsigned x, unsigned y, unsigned w, unsigned h,
                       float *p)
{
   const unsigned *src
      = ((const unsigned *) (ps->map))
      + y * ps->pitch + x;
   unsigned i, j;
   unsigned w0 = w;

   assert(ps->format == PIPE_FORMAT_A8R8G8B8_UNORM);

   CLIP_TILE;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++) {
         const unsigned pixel = src[j];
         pRow[0] = UBYTE_TO_FLOAT((pixel >> 16) & 0xff);
         pRow[1] = UBYTE_TO_FLOAT((pixel >>  8) & 0xff);
         pRow[2] = UBYTE_TO_FLOAT((pixel >>  0) & 0xff);
         pRow[3] = UBYTE_TO_FLOAT((pixel >> 24) & 0xff);
         pRow += 4;
      }
      src += ps->pitch;
      p += w0 * 4;
   }
}


static void
a8r8g8b8_put_tile_rgba(struct pipe_surface *ps,
                       unsigned x, unsigned y, unsigned w, unsigned h,
                       const float *p)
{
   unsigned *dst
      = ((unsigned *) (ps->map))
      + y * ps->pitch + x;
   unsigned i, j;
   unsigned w0 = w;

   assert(ps->format == PIPE_FORMAT_A8R8G8B8_UNORM);

   CLIP_TILE;

   for (i = 0; i < h; i++) {
      const float *pRow = p;
      for (j = 0; j < w; j++) {
         unsigned r, g, b, a;
         UNCLAMPED_FLOAT_TO_UBYTE(r, pRow[0]);
         UNCLAMPED_FLOAT_TO_UBYTE(g, pRow[1]);
         UNCLAMPED_FLOAT_TO_UBYTE(b, pRow[2]);
         UNCLAMPED_FLOAT_TO_UBYTE(a, pRow[3]);
         dst[j] = (a << 24) | (r << 16) | (g << 8) | b;
         pRow += 4;
      }
      dst += ps->pitch;
      p += w0 * 4;
   }
}


/*** PIPE_FORMAT_B8G8R8A8_UNORM ***/

static void
b8g8r8a8_get_tile_rgba(struct pipe_surface *ps,
                       unsigned x, unsigned y, unsigned w, unsigned h,
                       float *p)
{
   const unsigned *src
      = ((const unsigned *) (ps->map))
      + y * ps->pitch + x;
   unsigned i, j;
   unsigned w0 = w;

   assert(ps->format == PIPE_FORMAT_B8G8R8A8_UNORM);

   CLIP_TILE;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++) {
         const unsigned pixel = src[j];
         pRow[0] = UBYTE_TO_FLOAT((pixel >>  8) & 0xff);
         pRow[1] = UBYTE_TO_FLOAT((pixel >> 16) & 0xff);
         pRow[2] = UBYTE_TO_FLOAT((pixel >> 24) & 0xff);
         pRow[3] = UBYTE_TO_FLOAT((pixel >>  0) & 0xff);
         pRow += 4;
      }
      src += ps->pitch;
      p += w0 * 4;
   }
}


static void
b8g8r8a8_put_tile_rgba(struct pipe_surface *ps,
                       unsigned x, unsigned y, unsigned w, unsigned h,
                       const float *p)
{
   unsigned *dst
      = ((unsigned *) (ps->map))
      + y * ps->pitch + x;
   unsigned i, j;
   unsigned w0 = w;

   assert(ps->format == PIPE_FORMAT_B8G8R8A8_UNORM);

   CLIP_TILE;

   for (i = 0; i < h; i++) {
      const float *pRow = p;
      for (j = 0; j < w; j++) {
         unsigned r, g, b, a;
         UNCLAMPED_FLOAT_TO_UBYTE(r, pRow[0]);
         UNCLAMPED_FLOAT_TO_UBYTE(g, pRow[1]);
         UNCLAMPED_FLOAT_TO_UBYTE(b, pRow[2]);
         UNCLAMPED_FLOAT_TO_UBYTE(a, pRow[3]);
         dst[j] = (b << 24) | (g << 16) | (r << 8) | a;
         pRow += 4;
      }
      dst += ps->pitch;
      p += w0 * 4;
   }
}


/*** PIPE_FORMAT_A1R5G5B5_UNORM ***/

static void
a1r5g5b5_get_tile_rgba(struct pipe_surface *ps,
                       unsigned x, unsigned y, unsigned w, unsigned h,
                       float *p)
{
   const ushort *src
      = ((const ushort *) (ps->map))
      + y * ps->pitch + x;
   unsigned i, j;

   assert(ps->format == PIPE_FORMAT_A1R5G5B5_UNORM);

   for (i = 0; i < h; i++) {
      for (j = 0; j < w; j++) {
         const ushort pixel = src[j];
         p[0] = ((pixel >> 10) & 0x1f) * (1.0f / 31.0f);
         p[1] = ((pixel >>  5) & 0x1f) * (1.0f / 31.0f);
         p[2] = ((pixel      ) & 0x1f) * (1.0f / 31.0f);
         p[3] = ((pixel >> 15)       ) * 1.0f;
         p += 4;
      }
      src += ps->pitch;
   }
}


/*** PIPE_FORMAT_A4R4G4B4_UNORM ***/

static void
a4r4g4b4_get_tile_rgba(struct pipe_surface *ps,
                       unsigned x, unsigned y, unsigned w, unsigned h,
                       float *p)
{
   const ushort *src
      = ((const ushort *) (ps->map))
      + y * ps->pitch + x;
   unsigned i, j;

   assert(ps->format == PIPE_FORMAT_A4R4G4B4_UNORM);

   for (i = 0; i < h; i++) {
      for (j = 0; j < w; j++) {
         const ushort pixel = src[j];
         p[0] = ((pixel >>  8) & 0xf) * (1.0f / 15.0f);
         p[1] = ((pixel >>  4) & 0xf) * (1.0f / 15.0f);
         p[2] = ((pixel      ) & 0xf) * (1.0f / 15.0f);
         p[3] = ((pixel >> 12)      ) * (1.0f / 15.0f);
         p += 4;
      }
      src += ps->pitch;
   }
}


/*** PIPE_FORMAT_R5G6B5_UNORM ***/

static void
r5g6b5_get_tile_rgba(struct pipe_surface *ps,
                     unsigned x, unsigned y, unsigned w, unsigned h,
                     float *p)
{
   const ushort *src
      = ((const ushort *) (ps->map))
      + y * ps->pitch + x;
   unsigned i, j;

   assert(ps->format == PIPE_FORMAT_R5G6B5_UNORM);

   for (i = 0; i < h; i++) {
      for (j = 0; j < w; j++) {
         const ushort pixel = src[j];
         p[0] = ((pixel >> 11) & 0x1f) * (1.0f / 31.0f);
         p[1] = ((pixel >>  5) & 0x3f) * (1.0f / 63.0f);
         p[2] = ((pixel      ) & 0x1f) * (1.0f / 31.0f);
         p[3] = 1.0f;
         p += 4;
      }
      src += ps->pitch;
   }
}


/*** PIPE_FORMAT_Z16_UNORM ***/

/**
 * Return each Z value as four floats in [0,1].
 */
static void
z16_get_tile_rgba(struct pipe_surface *ps,
                  unsigned x, unsigned y, unsigned w, unsigned h,
                  float *p)
{
   const ushort *src
      = ((const ushort *) (ps->map))
      + y * ps->pitch + x;
   const float scale = 1.0f / 65535.0f;
   unsigned i, j;
   unsigned w0 = w;

   assert(ps->format == PIPE_FORMAT_Z16_UNORM);

   CLIP_TILE;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++) {
         pRow[j * 4 + 0] =
         pRow[j * 4 + 1] =
         pRow[j * 4 + 2] =
         pRow[j * 4 + 3] = src[j] * scale;
      }
      src += ps->pitch;
      p += 4 * w0;
   }
}




/*** PIPE_FORMAT_U_L8 ***/

static void
l8_get_tile_rgba(struct pipe_surface *ps,
                 unsigned x, unsigned y, unsigned w, unsigned h,
                 float *p)
{
   const ubyte *src
      = ((const ubyte *) (ps->map))
      + y * ps->pitch + x;
   unsigned i, j;
   unsigned w0 = w;

   assert(ps->format == PIPE_FORMAT_U_L8);

   CLIP_TILE;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++) {
         pRow[0] =
         pRow[1] =
         pRow[2] = UBYTE_TO_FLOAT(src[j]);
         pRow[3] = 1.0;
         pRow += 4;
      }
      src += ps->pitch;
      p += w0 * 4;
   }
}


/*** PIPE_FORMAT_U_A8 ***/

static void
a8_get_tile_rgba(struct pipe_surface *ps,
                 unsigned x, unsigned y, unsigned w, unsigned h,
                 float *p)
{
   const ubyte *src
      = ((const ubyte *) (ps->map))
      + y * ps->pitch + x;
   unsigned i, j;
   unsigned w0 = w;

   assert(ps->format == PIPE_FORMAT_U_A8);

   CLIP_TILE;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++) {
         pRow[0] =
         pRow[1] =
         pRow[2] = 0.0;
         pRow[3] = UBYTE_TO_FLOAT(src[j]);
         pRow += 4;
      }
      src += ps->pitch;
      p += w0 * 4;
   }
}


/*** PIPE_FORMAT_R16G16B16A16_SNORM ***/

static void
r16g16b16a16_get_tile_rgba(struct pipe_surface *ps,
                           unsigned x, unsigned y, unsigned w, unsigned h,
                           float *p)
{
   const short *src
      = ((const short *) (ps->map))
      + (y * ps->pitch + x) * 4;
   unsigned i, j;
   unsigned w0 = w;

   assert(ps->format == PIPE_FORMAT_R16G16B16A16_SNORM);

   CLIP_TILE;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      const short *pixel = src;
      for (j = 0; j < w; j++) {
         pRow[0] = SHORT_TO_FLOAT(pixel[0]);
         pRow[1] = SHORT_TO_FLOAT(pixel[1]);
         pRow[2] = SHORT_TO_FLOAT(pixel[2]);
         pRow[3] = SHORT_TO_FLOAT(pixel[3]);
         pRow += 4;
         pixel += 4;
      }
      src += ps->pitch * 4;
      p += w0 * 4;
   }
}


static void
r16g16b16a16_put_tile_rgba(struct pipe_surface *ps,
                           unsigned x, unsigned y, unsigned w, unsigned h,
                           const float *p)
{
   short *dst
      = ((short *) (ps->map))
      + (y * ps->pitch + x) * 4;
   unsigned i, j;
   unsigned w0 = w;

   assert(ps->format == PIPE_FORMAT_R16G16B16A16_SNORM);

   CLIP_TILE;

   for (i = 0; i < h; i++) {
      const float *pRow = p;
      for (j = 0; j < w; j++) {
         short r, g, b, a;
         UNCLAMPED_FLOAT_TO_SHORT(r, pRow[0]);
         UNCLAMPED_FLOAT_TO_SHORT(g, pRow[1]);
         UNCLAMPED_FLOAT_TO_SHORT(b, pRow[2]);
         UNCLAMPED_FLOAT_TO_SHORT(a, pRow[3]);
         dst[j*4+0] = r;
         dst[j*4+1] = g;
         dst[j*4+2] = b;
         dst[j*4+3] = a;
         pRow += 4;
      }
      dst += ps->pitch * 4;
      p += w0 * 4;
   }
}



/*** PIPE_FORMAT_U_I8 ***/

static void
i8_get_tile_rgba(struct pipe_surface *ps,
                 unsigned x, unsigned y, unsigned w, unsigned h,
                 float *p)
{
   const ubyte *src
      = ((const ubyte *) (ps->map))
      + y * ps->pitch + x;
   unsigned i, j;
   unsigned w0 = w;

   assert(ps->format == PIPE_FORMAT_U_I8);

   CLIP_TILE;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++) {
         pRow[0] =
         pRow[1] =
         pRow[2] =
         pRow[3] = UBYTE_TO_FLOAT(src[j]);
         pRow += 4;
      }
      src += ps->pitch;
      p += w0 * 4;
   }
}


/*** PIPE_FORMAT_U_A8_L8 ***/

static void
a8_l8_get_tile_rgba(struct pipe_surface *ps,
                    unsigned x, unsigned y, unsigned w, unsigned h,
                    float *p)
{
   const ushort *src
      = ((const ushort *) (ps->map))
      + y * ps->pitch + x;
   unsigned i, j;
   unsigned w0 = w;

   assert(ps->format == PIPE_FORMAT_U_A8_L8);

   CLIP_TILE;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++) {
         const ushort p = src[j];
         pRow[0] =
         pRow[1] =
         pRow[2] = UBYTE_TO_FLOAT(p & 0xff);
         pRow[3] = UBYTE_TO_FLOAT(p >> 8);
         pRow += 4;
      }
      src += ps->pitch;
      p += w0 * 4;
   }
}




/*** PIPE_FORMAT_Z32_UNORM ***/

/**
 * Return each Z value as four floats in [0,1].
 */
static void
z32_get_tile_rgba(struct pipe_surface *ps,
                  unsigned x, unsigned y, unsigned w, unsigned h,
                  float *p)
{
   const uint *src
      = ((const uint *) (ps->map))
      + y * ps->pitch + x;
   const double scale = 1.0 / (double) 0xffffffff;
   unsigned i, j;
   unsigned w0 = w;

   assert(ps->format == PIPE_FORMAT_Z16_UNORM);

   CLIP_TILE;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++) {
         pRow[j * 4 + 0] =
         pRow[j * 4 + 1] =
         pRow[j * 4 + 2] =
         pRow[j * 4 + 3] = (float) (scale * src[j]);
      }
      src += ps->pitch;
      p += 4 * w0;
   }
}


/*** PIPE_FORMAT_S8Z24_UNORM ***/

/**
 * Return Z component as four float in [0,1].  Stencil part ignored.
 */
static void
s8z24_get_tile_rgba(struct pipe_surface *ps,
                    unsigned x, unsigned y, unsigned w, unsigned h,
                    float *p)
{
   const uint *src
      = ((const uint *) (ps->map))
      + y * ps->pitch + x;
   const double scale = 1.0 / ((1 << 24) - 1);
   unsigned i, j;
   unsigned w0 = w;

   assert(ps->format == PIPE_FORMAT_S8Z24_UNORM);

   CLIP_TILE;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++) {
         pRow[j * 4 + 0] =
         pRow[j * 4 + 1] =
         pRow[j * 4 + 2] =
         pRow[j * 4 + 3] = (float) (scale * (src[j] & 0xffffff));
      }
      src += ps->pitch;
      p += 4 * w0;
   }
}


/*** PIPE_FORMAT_Z24S8_UNORM ***/

/**
 * Return Z component as four float in [0,1].  Stencil part ignored.
 */
static void
z24s8_get_tile_rgba(struct pipe_surface *ps,
                    unsigned x, unsigned y, unsigned w, unsigned h,
                    float *p)
{
   const uint *src
      = ((const uint *) (ps->map))
      + y * ps->pitch + x;
   const double scale = 1.0 / ((1 << 24) - 1);
   unsigned i, j;
   unsigned w0 = w;

   assert(ps->format == PIPE_FORMAT_Z24S8_UNORM);

   CLIP_TILE;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++) {
         pRow[j * 4 + 0] =
         pRow[j * 4 + 1] =
         pRow[j * 4 + 2] =
         pRow[j * 4 + 3] = (float) (scale * (src[j] >> 8));
      }
      src += ps->pitch;
      p += 4 * w0;
   }
}


void
softpipe_get_tile_rgba(struct pipe_context *pipe,
                       struct pipe_surface *ps,
                       uint x, uint y, uint w, uint h,
                       float *p)
{
   switch (ps->format) {
   case PIPE_FORMAT_A8R8G8B8_UNORM:
      a8r8g8b8_get_tile_rgba(ps, x, y, w, h, p);
      break;
   case PIPE_FORMAT_B8G8R8A8_UNORM:
      b8g8r8a8_get_tile_rgba(ps, x, y, w, h, p);
      break;
   case PIPE_FORMAT_A1R5G5B5_UNORM:
      a1r5g5b5_get_tile_rgba(ps, x, y, w, h, p);
      break;
   case PIPE_FORMAT_A4R4G4B4_UNORM:
      a4r4g4b4_get_tile_rgba(ps, x, y, w, h, p);
      break;
   case PIPE_FORMAT_R5G6B5_UNORM:
      r5g6b5_get_tile_rgba(ps, x, y, w, h, p);
      break;
   case PIPE_FORMAT_U_L8:
      l8_get_tile_rgba(ps, x, y, w, h, p);
      break;
   case PIPE_FORMAT_U_A8:
      a8_get_tile_rgba(ps, x, y, w, h, p);
      break;
   case PIPE_FORMAT_U_I8:
      i8_get_tile_rgba(ps, x, y, w, h, p);
      break;
   case PIPE_FORMAT_U_A8_L8:
      a8_l8_get_tile_rgba(ps, x, y, w, h, p);
      break;
   case PIPE_FORMAT_R16G16B16A16_SNORM:
      r16g16b16a16_get_tile_rgba(ps, x, y, w, h, p);
      break;
   case PIPE_FORMAT_Z16_UNORM:
      z16_get_tile_rgba(ps, x, y, w, h, p);
      break;
   case PIPE_FORMAT_Z32_UNORM:
      z32_get_tile_rgba(ps, x, y, w, h, p);
      break;
   case PIPE_FORMAT_S8Z24_UNORM:
      s8z24_get_tile_rgba(ps, x, y, w, h, p);
      break;
   case PIPE_FORMAT_Z24S8_UNORM:
      z24s8_get_tile_rgba(ps, x, y, w, h, p);
      break;
   default:
      assert(0);
   }
}


void
softpipe_put_tile_rgba(struct pipe_context *pipe,
                       struct pipe_surface *ps,
                       uint x, uint y, uint w, uint h,
                       const float *p)
{
   switch (ps->format) {
   case PIPE_FORMAT_A8R8G8B8_UNORM:
      a8r8g8b8_put_tile_rgba(ps, x, y, w, h, p);
      break;
   case PIPE_FORMAT_B8G8R8A8_UNORM:
      b8g8r8a8_put_tile_rgba(ps, x, y, w, h, p);
      break;
   case PIPE_FORMAT_A1R5G5B5_UNORM:
      /*a1r5g5b5_put_tile_rgba(ps, x, y, w, h, p);*/
      break;
   case PIPE_FORMAT_R5G6B5_UNORM:
   case PIPE_FORMAT_R8G8B8A8_UNORM:
      /* XXX need these */
      fprintf(stderr, "unsup pipe format in softpipe_put_tile_rgba()\n");
      break;
   case PIPE_FORMAT_U_L8:
      /*l8_put_tile_rgba(ps, x, y, w, h, p);*/
      break;
   case PIPE_FORMAT_U_A8:
      /*a8_put_tile_rgba(ps, x, y, w, h, p);*/
      break;
   case PIPE_FORMAT_U_I8:
      /*i8_put_tile_rgba(ps, x, y, w, h, p);*/
      break;
   case PIPE_FORMAT_U_A8_L8:
      /*a8_l8_put_tile_rgba(ps, x, y, w, h, p);*/
      break;
   case PIPE_FORMAT_R16G16B16A16_SNORM:
      r16g16b16a16_put_tile_rgba(ps, x, y, w, h, p);
      break;
   case PIPE_FORMAT_Z16_UNORM:
      /*z16_put_tile_rgba(ps, x, y, w, h, p);*/
      break;
   case PIPE_FORMAT_Z32_UNORM:
      /*z32_put_tile_rgba(ps, x, y, w, h, p);*/
      break;
   case PIPE_FORMAT_S8Z24_UNORM:
      /*s8z24_put_tile_rgba(ps, x, y, w, h, p);*/
      break;
   case PIPE_FORMAT_Z24S8_UNORM:
      /*z24s8_put_tile_rgba(ps, x, y, w, h, p);*/
      break;
   default:
      assert(0);
   }
}

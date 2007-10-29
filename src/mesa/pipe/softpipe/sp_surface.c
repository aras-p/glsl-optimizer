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

#include "pipe/p_defines.h"
#include "pipe/p_util.h"
#include "pipe/p_inlines.h"
#include "pipe/p_winsys.h"
#include "sp_context.h"
#include "sp_state.h"
#include "sp_surface.h"
#include "sp_tile_cache.h"

/**
 * Softpipe surface functions.
 */


/** Convert GLshort in [-32768,32767] to GLfloat in [-1.0,1.0] */
#define SHORT_TO_FLOAT(S)   ((2.0F * (S) + 1.0F) * (1.0F/65535.0F))

#define UNCLAMPED_FLOAT_TO_SHORT(us, f)  \
   us = ( (short) ( CLAMP((f), -1.0, 1.0) * 32767.0F) )



#if 0
#define CLIP_TILE \
   do { \
      assert(x + w <= ps->width);               \
      assert(y + h <= ps->height);              \
   } while(0)

#else
#define CLIP_TILE \
   do { \
      if (x >= ps->width) \
         return; \
      if (y >= ps->height) \
         return; \
      if (x + w > ps->width) \
         w = ps->width - x; \
      if (y + h > ps->height) \
         h = ps->height -y; \
   } while(0)
#endif


/*** PIPE_FORMAT_U_A8_R8_G8_B8 ***/

static void
a8r8g8b8_get_tile(struct pipe_surface *ps,
                  unsigned x, unsigned y, unsigned w, unsigned h, float *p)
{
   const unsigned *src
      = ((const unsigned *) (ps->region->map + ps->offset))
      + y * ps->region->pitch + x;
   unsigned i, j;
   unsigned w0 = w;

   assert(ps->format == PIPE_FORMAT_U_A8_R8_G8_B8);

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
      src += ps->region->pitch;
      p += w0 * 4;
   }
}


static void
a8r8g8b8_put_tile(struct pipe_surface *ps,
                  unsigned x, unsigned y, unsigned w, unsigned h,
                  const float *p)
{
   unsigned *dst
      = ((unsigned *) (ps->region->map + ps->offset))
      + y * ps->region->pitch + x;
   unsigned i, j;
   unsigned w0 = w;

   assert(ps->format == PIPE_FORMAT_U_A8_R8_G8_B8);

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
      dst += ps->region->pitch;
      p += w0 * 4;
   }
}


/*** PIPE_FORMAT_U_A1_R5_G5_B5 ***/

static void
a1r5g5b5_get_tile(struct pipe_surface *ps,
                  unsigned x, unsigned y, unsigned w, unsigned h, float *p)
{
   const ushort *src
      = ((const ushort *) (ps->region->map + ps->offset))
      + y * ps->region->pitch + x;
   unsigned i, j;

   assert(ps->format == PIPE_FORMAT_U_A1_R5_G5_B5);

   for (i = 0; i < h; i++) {
      for (j = 0; j < w; j++) {
         const ushort pixel = src[j];
         p[0] = ((pixel >> 10) & 0x1f) * (1.0f / 31.0f);
         p[1] = ((pixel >>  5) & 0x1f) * (1.0f / 31.0f);
         p[2] = ((pixel      ) & 0x1f) * (1.0f / 31.0f);
         p[3] = ((pixel >> 15)       ) * 1.0f;
         p += 4;
      }
      src += ps->region->pitch;
   }
}



/*** PIPE_FORMAT_U_Z16 ***/

/**
 * Return each Z value as four floats in [0,1].
 */
static void
z16_get_tile(struct pipe_surface *ps,
             unsigned x, unsigned y, unsigned w, unsigned h, float *p)
{
   const ushort *src
      = ((const ushort *) (ps->region->map + ps->offset))
      + y * ps->region->pitch + x;
   const float scale = 1.0f / 65535.0f;
   unsigned i, j;
   unsigned w0 = w;

   assert(ps->format == PIPE_FORMAT_U_Z16);

   CLIP_TILE;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++) {
         pRow[j * 4 + 0] =
         pRow[j * 4 + 1] =
         pRow[j * 4 + 2] =
         pRow[j * 4 + 3] = src[j] * scale;
      }
      src += ps->region->pitch;
      p += 4 * w0;
   }
}




/*** PIPE_FORMAT_U_L8 ***/

static void
l8_get_tile(struct pipe_surface *ps,
                  unsigned x, unsigned y, unsigned w, unsigned h, float *p)
{
   const ubyte *src
      = ((const ubyte *) (ps->region->map + ps->offset))
      + y * ps->region->pitch + x;
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
      src += ps->region->pitch;
      p += w0 * 4;
   }
}


/*** PIPE_FORMAT_U_A8 ***/

static void
a8_get_tile(struct pipe_surface *ps,
                  unsigned x, unsigned y, unsigned w, unsigned h, float *p)
{
   const ubyte *src
      = ((const ubyte *) (ps->region->map + ps->offset))
      + y * ps->region->pitch + x;
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
      src += ps->region->pitch;
      p += w0 * 4;
   }
}


/*** PIPE_FORMAT_S_R16_G16_B16_A16 ***/

static void
r16g16b16a16_get_tile(struct pipe_surface *ps,
                      unsigned x, unsigned y, unsigned w, unsigned h, float *p)
{
   const short *src
      = ((const short *) (ps->region->map + ps->offset))
      + (y * ps->region->pitch + x) * 4;
   unsigned i, j;
   unsigned w0 = w;

   assert(ps->format == PIPE_FORMAT_S_R16_G16_B16_A16);

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
      src += ps->region->pitch * 4;
      p += w0 * 4;
   }
}


static void
r16g16b16a16_put_tile(struct pipe_surface *ps,
                      unsigned x, unsigned y, unsigned w, unsigned h,
                      const float *p)
{
   short *dst
      = ((short *) (ps->region->map + ps->offset))
      + (y * ps->region->pitch + x) * 4;
   unsigned i, j;
   unsigned w0 = w;

   assert(ps->format == PIPE_FORMAT_S_R16_G16_B16_A16);

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
      dst += ps->region->pitch * 4;
      p += w0 * 4;
   }
}



/*** PIPE_FORMAT_U_I8 ***/

static void
i8_get_tile(struct pipe_surface *ps,
            unsigned x, unsigned y, unsigned w, unsigned h, float *p)
{
   const ubyte *src
      = ((const ubyte *) (ps->region->map + ps->offset))
      + y * ps->region->pitch + x;
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
      src += ps->region->pitch;
      p += w0 * 4;
   }
}


/*** PIPE_FORMAT_U_A8_L8 ***/

static void
a8_l8_get_tile(struct pipe_surface *ps,
               unsigned x, unsigned y, unsigned w, unsigned h, float *p)
{
   const ushort *src
      = ((const ushort *) (ps->region->map + ps->offset))
      + y * ps->region->pitch + x;
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
      src += ps->region->pitch;
      p += w0 * 4;
   }
}




/*** PIPE_FORMAT_U_Z32 ***/

/**
 * Return each Z value as four floats in [0,1].
 */
static void
z32_get_tile(struct pipe_surface *ps,
             unsigned x, unsigned y, unsigned w, unsigned h, float *p)
{
   const uint *src
      = ((const uint *) (ps->region->map + ps->offset))
      + y * ps->region->pitch + x;
   const double scale = 1.0 / (double) 0xffffffff;
   unsigned i, j;
   unsigned w0 = w;

   assert(ps->format == PIPE_FORMAT_U_Z16);

   CLIP_TILE;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++) {
         pRow[j * 4 + 0] =
         pRow[j * 4 + 1] =
         pRow[j * 4 + 2] =
         pRow[j * 4 + 3] = (float) (scale * src[j]);
      }
      src += ps->region->pitch;
      p += 4 * w0;
   }
}


/*** PIPE_FORMAT_S8_Z24 ***/

/**
 * Return Z component as four float in [0,1].  Stencil part ignored.
 */
static void
s8z24_get_tile(struct pipe_surface *ps,
               unsigned x, unsigned y, unsigned w, unsigned h, float *p)
{
   const uint *src
      = ((const uint *) (ps->region->map + ps->offset))
      + y * ps->region->pitch + x;
   const double scale = 1.0 / ((1 << 24) - 1);
   unsigned i, j;
   unsigned w0 = w;

   assert(ps->format == PIPE_FORMAT_S8_Z24);

   CLIP_TILE;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++) {
         pRow[j * 4 + 0] =
         pRow[j * 4 + 1] =
         pRow[j * 4 + 2] =
         pRow[j * 4 + 3] = (float) (scale * (src[j] & 0xffffff));
      }
      src += ps->region->pitch;
      p += 4 * w0;
   }
}


/**
 * Called via pipe->get_tex_surface()
 * XXX is this in the right place?
 */
struct pipe_surface *
softpipe_get_tex_surface(struct pipe_context *pipe,
                         struct pipe_mipmap_tree *mt,
                         unsigned face, unsigned level, unsigned zslice)
{
   struct pipe_surface *ps;
   unsigned offset;  /* in bytes */

   offset = mt->level[level].level_offset;

   if (mt->target == PIPE_TEXTURE_CUBE) {
      offset += mt->level[level].image_offset[face] * mt->cpp;
   }
   else if (mt->target == PIPE_TEXTURE_3D) {
      offset += mt->level[level].image_offset[zslice] * mt->cpp;
   }
   else {
      assert(face == 0);
      assert(zslice == 0);
   }

   ps = pipe->winsys->surface_alloc(pipe->winsys, mt->format);
   if (ps) {
      assert(ps->format);
      assert(ps->refcount);
      pipe_region_reference(&ps->region, mt->region);
      ps->width = mt->level[level].width;
      ps->height = mt->level[level].height;
      ps->offset = offset;
   }
   return ps;
}


/**
 * Move raw block of pixels from surface to user memory.
 */
static void
softpipe_get_tile(struct pipe_context *pipe,
                  struct pipe_surface *ps,
                  uint x, uint y, uint w, uint h,
                  void *p, int dst_stride)
{
   const uint cpp = ps->region->cpp;
   const uint w0 = w;
   const ubyte *pSrc;
   ubyte *pDest;
   uint i;

   assert(ps->region->map);

   CLIP_TILE;

   if (dst_stride == 0) {
      dst_stride = w0 * cpp;
   }

   pSrc = ps->region->map + ps->offset + (y * ps->region->pitch + x) * cpp;
   pDest = (ubyte *) p;

   for (i = 0; i < h; i++) {
      memcpy(pDest, pSrc, w0 * cpp);
      pDest += dst_stride;
      pSrc += ps->region->pitch * cpp;
   }
}


/**
 * Move raw block of pixels from user memory to surface.
 */
static void
softpipe_put_tile(struct pipe_context *pipe,
                  struct pipe_surface *ps,
                  uint x, uint y, uint w, uint h,
                  const void *p, int src_stride)
{
   const uint cpp = ps->region->cpp;
   const uint w0 = w;
   const ubyte *pSrc;
   ubyte *pDest;
   uint i;

   assert(ps->region->map);

   CLIP_TILE;

   if (src_stride == 0) {
      src_stride = w0 * cpp;
   }

   pSrc = (const ubyte *) p;
   pDest = ps->region->map + ps->offset + (y * ps->region->pitch + x) * cpp;

   for (i = 0; i < h; i++) {
      memcpy(pDest, pSrc, w0 * cpp);
      pDest += ps->region->pitch * cpp;
      pSrc += src_stride;
   }
}


/* XXX TEMPORARY */
void
softpipe_get_tile_rgba(struct pipe_context *pipe,
                       struct pipe_surface *ps,
                       uint x, uint y, uint w, uint h,
                       float *p)
{
   switch (ps->format) {
   case PIPE_FORMAT_U_A8_R8_G8_B8:
      a8r8g8b8_get_tile(ps, x, y, w, h, p);
      break;
   case PIPE_FORMAT_U_A1_R5_G5_B5:
      a1r5g5b5_get_tile(ps, x, y, w, h, p);
      break;
   case PIPE_FORMAT_U_L8:
      l8_get_tile(ps, x, y, w, h, p);
      break;
   case PIPE_FORMAT_U_A8:
      a8_get_tile(ps, x, y, w, h, p);
      break;
   case PIPE_FORMAT_U_I8:
      i8_get_tile(ps, x, y, w, h, p);
      break;
   case PIPE_FORMAT_U_A8_L8:
      a8_l8_get_tile(ps, x, y, w, h, p);
      break;
   case PIPE_FORMAT_S_R16_G16_B16_A16:
      r16g16b16a16_get_tile(ps, x, y, w, h, p);
      break;
   case PIPE_FORMAT_U_Z16:
      z16_get_tile(ps, x, y, w, h, p);
      break;
   case PIPE_FORMAT_U_Z32:
      z32_get_tile(ps, x, y, w, h, p);
      break;
   case PIPE_FORMAT_S8_Z24:
      s8z24_get_tile(ps, x, y, w, h, p);
      break;
   default:
      assert(0);
   }
}


/* XXX TEMPORARY */
void
softpipe_put_tile_rgba(struct pipe_context *pipe,
                       struct pipe_surface *ps,
                       uint x, uint y, uint w, uint h,
                       const float *p)
{
   switch (ps->format) {
   case PIPE_FORMAT_U_A8_R8_G8_B8:
      a8r8g8b8_put_tile(ps, x, y, w, h, p);
      break;
   case PIPE_FORMAT_U_A1_R5_G5_B5:
      /*a1r5g5b5_put_tile(ps, x, y, w, h, p);*/
      break;
   case PIPE_FORMAT_U_L8:
      /*l8_put_tile(ps, x, y, w, h, p);*/
      break;
   case PIPE_FORMAT_U_A8:
      /*a8_put_tile(ps, x, y, w, h, p);*/
      break;
   case PIPE_FORMAT_U_I8:
      /*i8_put_tile(ps, x, y, w, h, p);*/
      break;
   case PIPE_FORMAT_U_A8_L8:
      /*a8_l8_put_tile(ps, x, y, w, h, p);*/
      break;
   case PIPE_FORMAT_S_R16_G16_B16_A16:
      r16g16b16a16_put_tile(ps, x, y, w, h, p);
      break;
   case PIPE_FORMAT_U_Z16:
      /*z16_put_tile(ps, x, y, w, h, p);*/
      break;
   case PIPE_FORMAT_U_Z32:
      /*z32_put_tile(ps, x, y, w, h, p);*/
      break;
   case PIPE_FORMAT_S8_Z24:
      /*s8z24_put_tile(ps, x, y, w, h, p);*/
      break;
   default:
      assert(0);
   }
}



void
sp_init_surface_functions(struct softpipe_context *sp)
{
   sp->pipe.get_tile = softpipe_get_tile;
   sp->pipe.put_tile = softpipe_put_tile;

   sp->pipe.get_tile_rgba = softpipe_get_tile_rgba;
   sp->pipe.put_tile_rgba = softpipe_put_tile_rgba;
}

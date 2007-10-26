/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "i915_context.h"
#include "i915_state.h"
#include "pipe/p_defines.h"
#include "pipe/p_util.h"
#include "pipe/p_winsys.h"


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


/**
 * Note: this is exactly like a8r8g8b8_get_tile() in sp_surface.c
 * Share it someday.
 */
static void
i915_get_tile_rgba(struct pipe_context *pipe,
                   struct pipe_surface *ps,
                   uint x, uint y, uint w, uint h, float *p)
{
   const unsigned *src
      = ((const unsigned *) (ps->region->map + ps->offset))
      + y * ps->region->pitch + x;
   unsigned i, j;
   unsigned w0 = w;

   CLIP_TILE;

   switch (ps->format) {
   case PIPE_FORMAT_U_A8_R8_G8_B8:
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
      break;
   case PIPE_FORMAT_S8_Z24:
      {
         const float scale = 1.0 / (float) 0xffffff;
         for (i = 0; i < h; i++) {
            float *pRow = p;
            for (j = 0; j < w; j++) {
               const unsigned pixel = src[j];
               pRow[0] =
               pRow[1] =
               pRow[2] =
               pRow[3] = (pixel & 0xffffff) * scale;
               pRow += 4;
            }
            src += ps->region->pitch;
            p += w0 * 4;
         }
      }
      break;
   default:
      assert(0);
   }
}


static void
i915_put_tile_rgba(struct pipe_context *pipe,
                   struct pipe_surface *ps,
                   uint x, uint y, uint w, uint h, const float *p)
{
   /* TODO */
   assert(0);
}


/*
 * XXX note: same as code in sp_surface.c
 */
static void
i915_get_tile(struct pipe_context *pipe,
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


/*
 * XXX note: same as code in sp_surface.c
 */
static void
i915_put_tile(struct pipe_context *pipe,
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


/*
 * XXX note: same as code in sp_surface.c
 */
static struct pipe_surface *
i915_get_tex_surface(struct pipe_context *pipe,
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


void
i915_init_surface_functions(struct i915_context *i915)
{
   i915->pipe.get_tex_surface = i915_get_tex_surface;
   i915->pipe.get_tile = i915_get_tile;
   i915->pipe.put_tile = i915_put_tile;
   i915->pipe.get_tile_rgba = i915_get_tile_rgba;
   i915->pipe.put_tile_rgba = i915_put_tile_rgba;
}

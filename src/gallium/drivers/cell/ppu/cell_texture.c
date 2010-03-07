/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  *   Michel Dänzer <michel@tungstengraphics.com>
  *   Brian Paul
  */

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "util/u_simple_screen.h"

#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_memory.h"

#include "cell_context.h"
#include "cell_state.h"
#include "cell_texture.h"



static void
cell_texture_layout(struct cell_texture *ct)
{
   struct pipe_texture *pt = &ct->base;
   unsigned level;
   unsigned width = pt->width0;
   unsigned height = pt->height0;
   unsigned depth = pt->depth0;

   ct->buffer_size = 0;

   for (level = 0; level <= pt->last_level; level++) {
      unsigned size;
      unsigned w_tile, h_tile;

      assert(level < CELL_MAX_TEXTURE_LEVELS);

      /* width, height, rounded up to tile size */
      w_tile = align(width, TILE_SIZE);
      h_tile = align(height, TILE_SIZE);

      ct->stride[level] = util_format_get_stride(pt->format, w_tile);

      ct->level_offset[level] = ct->buffer_size;

      size = ct->stride[level] * util_format_get_nblocksy(pt->format, h_tile);
      if (pt->target == PIPE_TEXTURE_CUBE)
         size *= 6;
      else
         size *= depth;

      ct->buffer_size += size;

      width = u_minify(width, 1);
      height = u_minify(height, 1);
      depth = u_minify(depth, 1);
   }
}


static struct pipe_texture *
cell_texture_create(struct pipe_screen *screen,
                    const struct pipe_texture *templat)
{
   struct cell_texture *ct = CALLOC_STRUCT(cell_texture);
   if (!ct)
      return NULL;

   ct->base = *templat;
   pipe_reference_init(&ct->base.reference, 1);
   ct->base.screen = screen;

   cell_texture_layout(ct);

   ct->buffer = screen->buffer_create(screen, 32, PIPE_BUFFER_USAGE_PIXEL,
                                   ct->buffer_size);

   if (!ct->buffer) {
      FREE(ct);
      return NULL;
   }

   return &ct->base;
}


static void
cell_texture_destroy(struct pipe_texture *pt)
{
   struct cell_texture *ct = cell_texture(pt);

   if (ct->mapped) {
      pipe_buffer_unmap(ct->buffer->screen, ct->buffer);
      ct->mapped = NULL;
   }

   pipe_buffer_reference(&ct->buffer, NULL);

   FREE(ct);
}



/**
 * Convert image from linear layout to tiled layout.  4-byte pixels.
 */
static void
twiddle_image_uint(uint w, uint h, uint tile_size, uint *dst,
                   uint src_stride, const uint *src)
{
   const uint tile_size2 = tile_size * tile_size;
   const uint h_t = (h + tile_size - 1) / tile_size;
   const uint w_t = (w + tile_size - 1) / tile_size;

   uint it, jt;  /* tile counters */
   uint i, j;    /* intra-tile counters */

   src_stride /= 4; /* convert from bytes to pixels */

   /* loop over dest tiles */
   for (it = 0; it < h_t; it++) {
      for (jt = 0; jt < w_t; jt++) {
         /* start of dest tile: */
         uint *tdst = dst + (it * w_t + jt) * tile_size2;

         /* compute size of this tile (may be smaller than tile_size) */
         /* XXX note: a compiler bug was found here. That's why the code
          * looks as it does.
          */
         uint tile_width = w - jt * tile_size;
         tile_width = MIN2(tile_width, tile_size);
         uint tile_height = h - it * tile_size;
         tile_height = MIN2(tile_height, tile_size);

         /* loop over texels in the tile */
         for (i = 0; i < tile_height; i++) {
            for (j = 0; j < tile_width; j++) {
               const uint srci = it * tile_size + i;
               const uint srcj = jt * tile_size + j;
               ASSERT(srci < h);
               ASSERT(srcj < w);
               tdst[i * tile_size + j] = src[srci * src_stride + srcj];
            }
         }
      }
   }
}


/**
 * For Cell.  Basically, rearrange the pixels/quads from this layout:
 *  +--+--+--+--+
 *  |p0|p1|p2|p3|....
 *  +--+--+--+--+
 *
 * to this layout:
 *  +--+--+
 *  |p0|p1|....
 *  +--+--+
 *  |p2|p3|
 *  +--+--+
 */
static void
twiddle_tile(const uint *tileIn, uint *tileOut)
{
   int y, x;

   for (y = 0; y < TILE_SIZE; y+=2) {
      for (x = 0; x < TILE_SIZE; x+=2) {
         int k = 4 * (y/2 * TILE_SIZE/2 + x/2);
         tileOut[y * TILE_SIZE + (x + 0)] = tileIn[k];
         tileOut[y * TILE_SIZE + (x + 1)] = tileIn[k+1];
         tileOut[(y + 1) * TILE_SIZE + (x + 0)] = tileIn[k+2];
         tileOut[(y + 1) * TILE_SIZE + (x + 1)] = tileIn[k+3];
      }
   }
}


/**
 * Convert image from tiled layout to linear layout.  4-byte pixels.
 */
static void
untwiddle_image_uint(uint w, uint h, uint tile_size, uint *dst,
                     uint dst_stride, const uint *src)
{
   const uint tile_size2 = tile_size * tile_size;
   const uint h_t = (h + tile_size - 1) / tile_size;
   const uint w_t = (w + tile_size - 1) / tile_size;
   uint *tile_buf;
   uint it, jt;  /* tile counters */
   uint i, j;    /* intra-tile counters */

   dst_stride /= 4; /* convert from bytes to pixels */

   tile_buf = align_malloc(tile_size * tile_size * 4, 16);
   
   /* loop over src tiles */
   for (it = 0; it < h_t; it++) {
      for (jt = 0; jt < w_t; jt++) {
         /* start of src tile: */
         const uint *tsrc = src + (it * w_t + jt) * tile_size2;
         
         twiddle_tile(tsrc, tile_buf);
         tsrc = tile_buf;

         /* compute size of this tile (may be smaller than tile_size) */
         /* XXX note: a compiler bug was found here. That's why the code
          * looks as it does.
          */
         uint tile_width = w - jt * tile_size;
         tile_width = MIN2(tile_width, tile_size);
         uint tile_height = h - it * tile_size;
         tile_height = MIN2(tile_height, tile_size);

         /* loop over texels in the tile */
         for (i = 0; i < tile_height; i++) {
            for (j = 0; j < tile_width; j++) {
               uint dsti = it * tile_size + i;
               uint dstj = jt * tile_size + j;
               ASSERT(dsti < h);
               ASSERT(dstj < w);
               dst[dsti * dst_stride + dstj] = tsrc[i * tile_size + j];
            }
         }
      }
   }

   align_free(tile_buf);
}


static struct pipe_surface *
cell_get_tex_surface(struct pipe_screen *screen,
                     struct pipe_texture *pt,
                     unsigned face, unsigned level, unsigned zslice,
                     unsigned usage)
{
   struct cell_texture *ct = cell_texture(pt);
   struct pipe_surface *ps;

   ps = CALLOC_STRUCT(pipe_surface);
   if (ps) {
      pipe_reference_init(&ps->reference, 1);
      pipe_texture_reference(&ps->texture, pt);
      ps->format = pt->format;
      ps->width = u_minify(pt->width0, level);
      ps->height = u_minify(pt->height0, level);
      ps->offset = ct->level_offset[level];
      /* XXX may need to override usage flags (see sp_texture.c) */
      ps->usage = usage;
      ps->face = face;
      ps->level = level;
      ps->zslice = zslice;

      if (pt->target == PIPE_TEXTURE_CUBE) {
         unsigned h_tile = align(ps->height, TILE_SIZE);
         ps->offset += face * util_format_get_nblocksy(ps->format, h_tile) * ct->stride[level];
      }
      else if (pt->target == PIPE_TEXTURE_3D) {
         unsigned h_tile = align(ps->height, TILE_SIZE);
         ps->offset += zslice * util_format_get_nblocksy(ps->format, h_tile) * ct->stride[level];
      }
      else {
         assert(face == 0);
         assert(zslice == 0);
      }
   }
   return ps;
}


static void 
cell_tex_surface_destroy(struct pipe_surface *surf)
{
   pipe_texture_reference(&surf->texture, NULL);
   FREE(surf);
}


/**
 * Create new pipe_transfer object.
 * This is used by the user to put tex data into a texture (and get it
 * back out for glGetTexImage).
 */
static struct pipe_transfer *
cell_get_tex_transfer(struct pipe_screen *screen,
                      struct pipe_texture *texture,
                      unsigned face, unsigned level, unsigned zslice,
                      enum pipe_transfer_usage usage,
                      unsigned x, unsigned y, unsigned w, unsigned h)
{
   struct cell_texture *ct = cell_texture(texture);
   struct cell_transfer *ctrans;

   assert(texture);
   assert(level <= texture->last_level);

   ctrans = CALLOC_STRUCT(cell_transfer);
   if (ctrans) {
      struct pipe_transfer *pt = &ctrans->base;
      pipe_texture_reference(&pt->texture, texture);
      pt->x = x;
      pt->y = y;
      pt->width = w;
      pt->height = h;
      pt->stride = ct->stride[level];
      pt->usage = usage;
      pt->face = face;
      pt->level = level;
      pt->zslice = zslice;

      ctrans->offset = ct->level_offset[level];

      if (texture->target == PIPE_TEXTURE_CUBE) {
         unsigned h_tile = align(u_minify(texture->height0, level), TILE_SIZE);
         ctrans->offset += face * util_format_get_nblocksy(texture->format, h_tile) * pt->stride;
      }
      else if (texture->target == PIPE_TEXTURE_3D) {
         unsigned h_tile = align(u_minify(texture->height0, level), TILE_SIZE);
         ctrans->offset += zslice * util_format_get_nblocksy(texture->format, h_tile) * pt->stride;
      }
      else {
         assert(face == 0);
         assert(zslice == 0);
      }
      return pt;
   }
   return NULL;
}


static void 
cell_tex_transfer_destroy(struct pipe_transfer *t)
{
   struct cell_transfer *transfer = cell_transfer(t);
   /* Effectively do the texture_update work here - if texture images
    * needed post-processing to put them into hardware layout, this is
    * where it would happen.  For cell, nothing to do.
    */
   assert (transfer->base.texture);
   pipe_texture_reference(&transfer->base.texture, NULL);
   FREE(transfer);
}


/**
 * Return pointer to texture image data in linear layout.
 */
static void *
cell_transfer_map(struct pipe_screen *screen, struct pipe_transfer *transfer)
{
   struct cell_transfer *ctrans = cell_transfer(transfer);
   struct pipe_texture *pt = transfer->texture;
   struct cell_texture *ct = cell_texture(pt);
   const uint level = ctrans->base.level;
   const uint texWidth = u_minify(pt->width0, level);
   const uint texHeight = u_minify(pt->height0, level);
   const uint stride = ct->stride[level];
   unsigned size;

   assert(transfer->texture);

   if (!ct->mapped) {
      /* map now */
      ct->mapped = pipe_buffer_map(screen, ct->buffer,
                                   pipe_transfer_buffer_flags(transfer));
   }

   /*
    * Create a buffer of ordinary memory for the linear texture.
    * This is the memory that the user will read/write.
    */
   size = util_format_get_stride(pt->format, align(texWidth, TILE_SIZE)) *
          util_format_get_nblocksy(pt->format, align(texHeight, TILE_SIZE));

   ctrans->map = align_malloc(size, 16);
   if (!ctrans->map)
      return NULL; /* out of memory */

   if (transfer->usage & PIPE_TRANSFER_READ) {
      /* need to untwiddle the texture to make a linear version */
      const uint bpp = util_format_get_blocksize(ct->base.format);
      if (bpp == 4) {
         const uint *src = (uint *) (ct->mapped + ctrans->offset);
         uint *dst = ctrans->map;
         untwiddle_image_uint(texWidth, texHeight, TILE_SIZE,
                              dst, stride, src);
      }
      else {
         // xxx fix
      }
   }

   return ctrans->map;
}


/**
 * Called when user is done reading/writing texture data.
 * If new data was written, this is where we convert the linear data
 * to tiled data.
 */
static void
cell_transfer_unmap(struct pipe_screen *screen,
                    struct pipe_transfer *transfer)
{
   struct cell_transfer *ctrans = cell_transfer(transfer);
   struct pipe_texture *pt = transfer->texture;
   struct cell_texture *ct = cell_texture(pt);
   const uint level = ctrans->base.level;
   const uint texWidth = u_minify(pt->width0, level);
   const uint texHeight = u_minify(pt->height0, level);
   const uint stride = ct->stride[level];

   if (!ct->mapped) {
      /* map now */
      ct->mapped = pipe_buffer_map(screen, ct->buffer,
                                   PIPE_BUFFER_USAGE_CPU_READ);
   }

   if (transfer->usage & PIPE_TRANSFER_WRITE) {
      /* The user wrote new texture data into the mapped buffer.
       * We need to convert the new linear data into the twiddled/tiled format.
       */
      const uint bpp = util_format_get_blocksize(ct->base.format);
      if (bpp == 4) {
         const uint *src = ctrans->map;
         uint *dst = (uint *) (ct->mapped + ctrans->offset);
         twiddle_image_uint(texWidth, texHeight, TILE_SIZE, dst, stride, src);
      }
      else {
         // xxx fix
      }
   }

   align_free(ctrans->map);
   ctrans->map = NULL;
}


void
cell_init_screen_texture_funcs(struct pipe_screen *screen)
{
   screen->texture_create = cell_texture_create;
   screen->texture_destroy = cell_texture_destroy;

   screen->get_tex_surface = cell_get_tex_surface;
   screen->tex_surface_destroy = cell_tex_surface_destroy;

   screen->get_tex_transfer = cell_get_tex_transfer;
   screen->tex_transfer_destroy = cell_tex_transfer_destroy;

   screen->transfer_map = cell_transfer_map;
   screen->transfer_unmap = cell_transfer_unmap;
}

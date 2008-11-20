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
#include "pipe/p_inlines.h"
#include "pipe/p_winsys.h"
#include "util/u_math.h"
#include "util/u_memory.h"

#include "cell_context.h"
#include "cell_state.h"
#include "cell_texture.h"



static unsigned
minify(unsigned d)
{
   return MAX2(1, d>>1);
}


static void
cell_texture_layout(struct cell_texture *ct)
{
   struct pipe_texture *pt = &ct->base;
   unsigned level;
   unsigned width = pt->width[0];
   unsigned height = pt->height[0];
   unsigned depth = pt->depth[0];

   ct->buffer_size = 0;

   for ( level = 0 ; level <= pt->last_level ; level++ ) {
      unsigned size;
      unsigned w_tile, h_tile;

      assert(level < CELL_MAX_TEXTURE_LEVELS);

      /* width, height, rounded up to tile size */
      w_tile = align(width, TILE_SIZE);
      h_tile = align(height, TILE_SIZE);

      pt->width[level] = width;
      pt->height[level] = height;
      pt->depth[level] = depth;
      pt->nblocksx[level] = pf_get_nblocksx(&pt->block, w_tile);  
      pt->nblocksy[level] = pf_get_nblocksy(&pt->block, h_tile);  

      ct->stride[level] = pt->nblocksx[level] * pt->block.size;

      ct->level_offset[level] = ct->buffer_size;

      size = pt->nblocksx[level] * pt->nblocksy[level] * pt->block.size;
      if (pt->target == PIPE_TEXTURE_CUBE)
         size *= 6;
      else
         size *= depth;

      ct->buffer_size += size;

      width  = minify(width);
      height = minify(height);
      depth = minify(depth);
   }
}


static struct pipe_texture *
cell_texture_create(struct pipe_screen *screen,
                    const struct pipe_texture *templat)
{
   struct pipe_winsys *ws = screen->winsys;
   struct cell_texture *ct = CALLOC_STRUCT(cell_texture);
   if (!ct)
      return NULL;

   ct->base = *templat;
   ct->base.refcount = 1;
   ct->base.screen = screen;

   cell_texture_layout(ct);

   ct->buffer = ws->buffer_create(ws, 32, PIPE_BUFFER_USAGE_PIXEL,
                                  ct->buffer_size);

   if (!ct->buffer) {
      FREE(ct);
      return NULL;
   }

   return &ct->base;
}


static void
cell_texture_release(struct pipe_screen *screen,
                     struct pipe_texture **pt)
{
   if (!*pt)
      return;

   /*
   DBG("%s %p refcount will be %d\n",
       __FUNCTION__, (void *) *pt, (*pt)->refcount - 1);
   */
   if (--(*pt)->refcount <= 0) {
      /* Delete this texture now.
       * But note that the underlying pipe_buffer may linger...
       */
      struct cell_texture *ct = cell_texture(*pt);
      uint i;

      /*
      DBG("%s deleting %p\n", __FUNCTION__, (void *) ct);
      */

      pipe_buffer_reference(screen, &ct->buffer, NULL);

      for (i = 0; i < CELL_MAX_TEXTURE_LEVELS; i++) {
         /* Unreference the tiled image buffer.
          * It may not actually be deleted until a fence is hit.
          */
         if (ct->tiled_buffer[i]) {
            ct->tiled_mapped[i] = NULL;
            winsys_buffer_reference(screen->winsys, &ct->tiled_buffer[i], NULL);
         }
      }

      FREE(ct);
   }
   *pt = NULL;
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


/**
 * Convert linear texture image data to tiled format for SPU usage.
 */
static void
cell_twiddle_texture(struct pipe_screen *screen,
                     struct pipe_surface *surface)
{
   struct cell_texture *ct = cell_texture(surface->texture);
   const uint level = surface->level;
   const uint texWidth = ct->base.width[level];
   const uint texHeight = ct->base.height[level];
   const uint bufWidth = align(texWidth, TILE_SIZE);
   const uint bufHeight = align(texHeight, TILE_SIZE);
   const void *map = pipe_buffer_map(screen, surface->buffer,
                                     PIPE_BUFFER_USAGE_CPU_READ);
   const uint *src = (const uint *) ((const ubyte *) map + surface->offset);

   switch (ct->base.format) {
   case PIPE_FORMAT_A8R8G8B8_UNORM:
   case PIPE_FORMAT_B8G8R8A8_UNORM:
   case PIPE_FORMAT_S8Z24_UNORM:
      {
         int numFaces = ct->base.target == PIPE_TEXTURE_CUBE ? 6 : 1;
         int offset = bufWidth * bufHeight * 4 * surface->face;
         uint *dst;

         if (!ct->tiled_buffer[level]) {
            /* allocate buffer for tiled data now */
            struct pipe_winsys *ws = screen->winsys;
            uint bytes = bufWidth * bufHeight * 4 * numFaces;
            ct->tiled_buffer[level] = ws->buffer_create(ws, 16,
                                                        PIPE_BUFFER_USAGE_PIXEL,
                                                        bytes);
            /* and map it */
            ct->tiled_mapped[level] = ws->buffer_map(ws, ct->tiled_buffer[level],
                                                     PIPE_BUFFER_USAGE_GPU_READ);
         }
         dst = (uint *) ((ubyte *) ct->tiled_mapped[level] + offset);

         twiddle_image_uint(texWidth, texHeight, TILE_SIZE, dst,
                            surface->stride, src);
      }
      break;
   default:
      printf("Cell: twiddle unsupported texture format %s\n", pf_name(ct->base.format));
      ;
   }

   pipe_buffer_unmap(screen, surface->buffer);
}


/**
 * Convert SPU tiled texture image data to linear format for app usage.
 */
static void
cell_untwiddle_texture(struct pipe_screen *screen,
                     struct pipe_surface *surface)
{
   struct cell_texture *ct = cell_texture(surface->texture);
   const uint level = surface->level;
   const uint texWidth = ct->base.width[level];
   const uint texHeight = ct->base.height[level];
   const void *map = pipe_buffer_map(screen, surface->buffer,
                                     PIPE_BUFFER_USAGE_CPU_READ);
   const uint *src = (const uint *) ((const ubyte *) map + surface->offset);

   switch (ct->base.format) {
   case PIPE_FORMAT_A8R8G8B8_UNORM:
   case PIPE_FORMAT_B8G8R8A8_UNORM:
   case PIPE_FORMAT_S8Z24_UNORM:
      {
         int numFaces = ct->base.target == PIPE_TEXTURE_CUBE ? 6 : 1;
         int offset = surface->stride * texHeight * 4 * surface->face;
         uint *dst;

         if (!ct->untiled_data[level]) {
            ct->untiled_data[level] =
               align_malloc(surface->stride * texHeight * 4 * numFaces, 16);
         }

         dst = (uint *) ((ubyte *) ct->untiled_data[level] + offset);

         untwiddle_image_uint(texWidth, texHeight, TILE_SIZE, dst,
                              surface->stride, src);
      }
      break;
   default:
      {
         ct->untiled_data[level] = NULL;
         printf("Cell: untwiddle unsupported texture format %s\n", pf_name(ct->base.format));
      }
   }

   pipe_buffer_unmap(screen, surface->buffer);
}


static struct pipe_surface *
cell_get_tex_surface(struct pipe_screen *screen,
                     struct pipe_texture *pt,
                     unsigned face, unsigned level, unsigned zslice,
                     unsigned usage)
{
   struct pipe_winsys *ws = screen->winsys;
   struct cell_texture *ct = cell_texture(pt);
   struct pipe_surface *ps;

   ps = ws->surface_alloc(ws);
   if (ps) {
      assert(ps->refcount);
      assert(ps->winsys);
      winsys_buffer_reference(ws, &ps->buffer, ct->buffer);
      ps->format = pt->format;
      ps->block = pt->block;
      ps->width = pt->width[level];
      ps->height = pt->height[level];
      ps->nblocksx = pt->nblocksx[level];
      ps->nblocksy = pt->nblocksy[level];
      ps->stride = ct->stride[level];
      ps->offset = ct->level_offset[level];
      ps->usage = usage;

      /* XXX may need to override usage flags (see sp_texture.c) */

      pipe_texture_reference(&ps->texture, pt); 
      ps->face = face;
      ps->level = level;
      ps->zslice = zslice;

      if (pt->target == PIPE_TEXTURE_CUBE || pt->target == PIPE_TEXTURE_3D) {
	         ps->offset += ((pt->target == PIPE_TEXTURE_CUBE) ? face : zslice) *
		      ps->nblocksy *
		      ps->stride;
      }
      else {
         assert(face == 0);
         assert(zslice == 0);
      }

      if (ps->usage & PIPE_BUFFER_USAGE_CPU_READ) {
         /* convert from tiled to linear layout */
         cell_untwiddle_texture(screen, ps);
      }
   }
   return ps;
}


static void 
cell_tex_surface_release(struct pipe_screen *screen, 
                         struct pipe_surface **s)
{
   struct cell_texture *ct = cell_texture((*s)->texture);
   const uint level = (*s)->level;

   if (((*s)->usage & PIPE_BUFFER_USAGE_CPU_READ) && (ct->untiled_data[level]))
   {
      align_free(ct->untiled_data[level]);
      ct->untiled_data[level] = NULL;
   }

   /* XXX if done rendering to teximage, re-tile */

   pipe_texture_reference(&(*s)->texture, NULL); 

   screen->winsys->surface_release(screen->winsys, s);
}


static void *
cell_surface_map(struct pipe_screen *screen,
                 struct pipe_surface *surface,
                 unsigned flags)
{
   ubyte *map;
   struct cell_texture *ct = cell_texture(surface->texture);
   const uint level = surface->level;

   assert(ct);

   if (flags & ~surface->usage) {
      assert(0);
      return NULL;
   }

   map = pipe_buffer_map( screen, surface->buffer, flags );
   if (map == NULL)
      return NULL;
   else
   {
      if ((surface->usage & PIPE_BUFFER_USAGE_CPU_READ) && (ct->untiled_data[level])) {
         return (void *) ((ubyte *) ct->untiled_data[level] + surface->offset);
      }
      else {
         return (void *) (map + surface->offset);
      }
   }
}


static void
cell_surface_unmap(struct pipe_screen *screen,
                   struct pipe_surface *surface)
{
   struct cell_texture *ct = cell_texture(surface->texture);

   assert(ct);

   if ((ct->base.tex_usage & PIPE_TEXTURE_USAGE_SAMPLER) &&
       (surface->usage & PIPE_BUFFER_USAGE_CPU_WRITE)) {
      /* convert from linear to tiled layout */
      cell_twiddle_texture(screen, surface);
   }

   pipe_buffer_unmap( screen, surface->buffer );
}



void
cell_init_screen_texture_funcs(struct pipe_screen *screen)
{
   screen->texture_create = cell_texture_create;
   screen->texture_release = cell_texture_release;

   screen->get_tex_surface = cell_get_tex_surface;
   screen->tex_surface_release = cell_tex_surface_release;

   screen->surface_map = cell_surface_map;
   screen->surface_unmap = cell_surface_unmap;
}

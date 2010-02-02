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
 * Texture tile caching.
 *
 * Author:
 *    Brian Paul
 */

#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_tile.h"
#include "util/u_rect.h"
#include "lp_context.h"
#include "lp_tile_soa.h"
#include "lp_tile_cache.h"


#define MAX_WIDTH 4096
#define MAX_HEIGHT 4096


enum llvmpipe_tile_status
{
   LP_TILE_STATUS_UNDEFINED = 0,
   LP_TILE_STATUS_CLEAR = 1,
   LP_TILE_STATUS_DEFINED = 2
};


struct llvmpipe_cached_tile
{
   enum llvmpipe_tile_status status;

   /** color in SOA format */
   uint8_t *color;
};


struct llvmpipe_tile_cache
{
   struct pipe_screen *screen;
   struct pipe_surface *surface;  /**< the surface we're caching */
   struct pipe_transfer *transfer;
   void *transfer_map;

   struct llvmpipe_cached_tile entries[MAX_WIDTH/TILE_SIZE][MAX_HEIGHT/TILE_SIZE];

   uint8_t clear_color[4];  /**< for color bufs */
   uint clear_val;        /**< for z+stencil, or packed color clear value */

   struct llvmpipe_cached_tile *last_tile;  /**< most recently retrieved tile */
};


struct llvmpipe_tile_cache *
lp_create_tile_cache( struct pipe_screen *screen )
{
   struct llvmpipe_tile_cache *tc;
   int maxLevels, maxTexSize;

   /* sanity checking: max sure MAX_WIDTH/HEIGHT >= largest texture image */
   maxLevels = screen->get_param(screen, PIPE_CAP_MAX_TEXTURE_2D_LEVELS);
   maxTexSize = 1 << (maxLevels - 1);
   assert(MAX_WIDTH >= maxTexSize);

   tc = CALLOC_STRUCT( llvmpipe_tile_cache );
   if(!tc)
      return NULL;

   tc->screen = screen;

   return tc;
}


void
lp_destroy_tile_cache(struct llvmpipe_tile_cache *tc)
{
   struct pipe_screen *screen;
   unsigned x, y;

   for (y = 0; y < MAX_HEIGHT; y += TILE_SIZE) {
      for (x = 0; x < MAX_WIDTH; x += TILE_SIZE) {
         struct llvmpipe_cached_tile *tile = &tc->entries[y/TILE_SIZE][x/TILE_SIZE];

         if(tile->color)
            align_free(tile->color);
      }
   }

   if (tc->transfer) {
      screen = tc->transfer->texture->screen;
      screen->tex_transfer_destroy(tc->transfer);
   }

   FREE( tc );
}


/**
 * Specify the surface to cache.
 */
void
lp_tile_cache_set_surface(struct llvmpipe_tile_cache *tc,
                          struct pipe_surface *ps)
{
   if (tc->transfer) {
      struct pipe_screen *screen = tc->transfer->texture->screen;

      if (ps == tc->surface)
         return;

      if (tc->transfer_map) {
         screen->transfer_unmap(screen, tc->transfer);
         tc->transfer_map = NULL;
      }

      screen->tex_transfer_destroy(tc->transfer);
      tc->transfer = NULL;
   }

   tc->surface = ps;

   if (ps) {
      struct pipe_screen *screen = ps->texture->screen;
      unsigned x, y;

      tc->transfer = screen->get_tex_transfer(screen, ps->texture, ps->face,
                                              ps->level, ps->zslice,
                                              PIPE_TRANSFER_READ_WRITE,
                                              0, 0, ps->width, ps->height);

      for (y = 0; y < ps->height; y += TILE_SIZE) {
         for (x = 0; x < ps->width; x += TILE_SIZE) {
            struct llvmpipe_cached_tile *tile = &tc->entries[y/TILE_SIZE][x/TILE_SIZE];

            tile->status = LP_TILE_STATUS_UNDEFINED;

            if(!tile->color)
               tile->color = align_malloc( TILE_SIZE*TILE_SIZE*NUM_CHANNELS, 16 );
         }
      }
   }
}


/**
 * Return the transfer being cached.
 */
struct pipe_surface *
lp_tile_cache_get_surface(struct llvmpipe_tile_cache *tc)
{
   return tc->surface;
}


void
lp_tile_cache_map_transfers(struct llvmpipe_tile_cache *tc)
{
   if (tc->transfer && !tc->transfer_map)
      tc->transfer_map = tc->screen->transfer_map(tc->screen, tc->transfer);
}


void
lp_tile_cache_unmap_transfers(struct llvmpipe_tile_cache *tc)
{
   if (tc->transfer_map) {
      tc->screen->transfer_unmap(tc->screen, tc->transfer);
      tc->transfer_map = NULL;
   }
}


/**
 * Set a tile to a solid color.
 */
static void
clear_tile(struct llvmpipe_cached_tile *tile,
           uint8_t clear_color[4])
{
   if (clear_color[0] == clear_color[1] &&
       clear_color[1] == clear_color[2] &&
       clear_color[2] == clear_color[3]) {
      memset(tile->color, clear_color[0], TILE_SIZE * TILE_SIZE * 4);
   }
   else {
      uint x, y, chan;
      for (y = 0; y < TILE_SIZE; y++)
         for (x = 0; x < TILE_SIZE; x++)
            for (chan = 0; chan < 4; ++chan)
               TILE_PIXEL(tile->color, x, y, chan) = clear_color[chan];
   }
}


/**
 * Flush the tile cache: write all dirty tiles back to the transfer.
 * any tiles "flagged" as cleared will be "really" cleared.
 */
void
lp_flush_tile_cache(struct llvmpipe_tile_cache *tc)
{
   struct pipe_transfer *pt = tc->transfer;
   unsigned x, y;

   if(!pt)
      return;

   assert(tc->transfer_map);

   /* push the tile to all positions marked as clear */
   for (y = 0; y < pt->height; y += TILE_SIZE) {
      for (x = 0; x < pt->width; x += TILE_SIZE) {
         struct llvmpipe_cached_tile *tile = &tc->entries[y/TILE_SIZE][x/TILE_SIZE];

         if(tile->status != LP_TILE_STATUS_UNDEFINED) {
            unsigned w = TILE_SIZE;
            unsigned h = TILE_SIZE;

            if (!pipe_clip_tile(x, y, &w, &h, pt)) {
               switch(tile->status) {
               case LP_TILE_STATUS_CLEAR:
                  /* Actually clear the tiles which were flagged as being in a
                   * clear state. */
                  util_fill_rect(tc->transfer_map, pt->texture->format, pt->stride,
                                 x, y, w, h,
                                 tc->clear_val);
                  break;

               case LP_TILE_STATUS_DEFINED:
                  lp_tile_write_4ub(pt->texture->format,
                                    tile->color,
                                    tc->transfer_map, pt->stride,
                                    x, y, w, h);
                  break;

               default:
                  assert(0);
                  break;
               }
            }

            tile->status = LP_TILE_STATUS_UNDEFINED;
         }
      }
   }
}


/**
 * Get a tile from the cache.
 * \param x, y  position of tile, in pixels
 */
void *
lp_get_cached_tile(struct llvmpipe_tile_cache *tc,
                   unsigned x, unsigned y )
{
   struct llvmpipe_cached_tile *tile = &tc->entries[y/TILE_SIZE][x/TILE_SIZE];
   struct pipe_transfer *pt = tc->transfer;
   
   assert(tc->surface);
   assert(tc->transfer);

   if(!tc->transfer_map)
      lp_tile_cache_map_transfers(tc);

   assert(tc->transfer_map);

   switch(tile->status) {
   case LP_TILE_STATUS_CLEAR:
      /* don't get tile from framebuffer, just clear it */
      clear_tile(tile, tc->clear_color);
      tile->status = LP_TILE_STATUS_DEFINED;
      break;

   case LP_TILE_STATUS_UNDEFINED: {
      unsigned w = TILE_SIZE;
      unsigned h = TILE_SIZE;

      x &= ~(TILE_SIZE - 1);
      y &= ~(TILE_SIZE - 1);

      if (!pipe_clip_tile(x, y, &w, &h, tc->transfer))
         lp_tile_read_4ub(pt->texture->format,
                          tile->color,
                          tc->transfer_map, tc->transfer->stride,
                          x, y, w, h);

      tile->status = LP_TILE_STATUS_DEFINED;
      break;
   }

   case LP_TILE_STATUS_DEFINED:
      /* nothing to do */
      break;
   }

   return tile->color;
}


/**
 * When a whole surface is being cleared to a value we can avoid
 * fetching tiles above.
 * Save the color and set a 'clearflag' for each tile of the screen.
 */
void
lp_tile_cache_clear(struct llvmpipe_tile_cache *tc, const float *rgba,
                    uint clearValue)
{
   struct pipe_transfer *pt = tc->transfer;
   const unsigned w = pt->width;
   const unsigned h = pt->height;
   unsigned x, y, chan;

   for(chan = 0; chan < 4; ++chan)
      tc->clear_color[chan] = float_to_ubyte(rgba[chan]);

   tc->clear_val = clearValue;

   /* push the tile to all positions marked as clear */
   for (y = 0; y < h; y += TILE_SIZE) {
      for (x = 0; x < w; x += TILE_SIZE) {
         struct llvmpipe_cached_tile *tile = &tc->entries[y/TILE_SIZE][x/TILE_SIZE];
         tile->status = LP_TILE_STATUS_CLEAR;
      }
   }
}

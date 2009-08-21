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

#include "pipe/p_inlines.h"
#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_tile.h"
#include "util/u_rect.h"
#include "lp_context.h"
#include "lp_surface.h"
#include "lp_texture.h"
#include "lp_tile_soa.h"
#include "lp_tile_cache.h"



/**
 * Return the position in the cache for the tile that contains win pos (x,y).
 * We currently use a direct mapped cache so this is like a hack key.
 * At some point we should investige something more sophisticated, like
 * a LRU replacement policy.
 */
#define CACHE_POS(x, y) \
   (((x) + (y) * 5) % NUM_ENTRIES)



/**
 * Is the tile at (x,y) in cleared state?
 */
static INLINE uint
is_clear_flag_set(const uint *bitvec, union tile_address addr)
{
   int pos, bit;
   pos = addr.bits.y * (MAX_WIDTH / TILE_SIZE) + addr.bits.x;
   assert(pos / 32 < (MAX_WIDTH / TILE_SIZE) * (MAX_HEIGHT / TILE_SIZE) / 32);
   bit = bitvec[pos / 32] & (1 << (pos & 31));
   return bit;
}
   

/**
 * Mark the tile at (x,y) as not cleared.
 */
static INLINE void
clear_clear_flag(uint *bitvec, union tile_address addr)
{
   int pos;
   pos = addr.bits.y * (MAX_WIDTH / TILE_SIZE) + addr.bits.x;
   assert(pos / 32 < (MAX_WIDTH / TILE_SIZE) * (MAX_HEIGHT / TILE_SIZE) / 32);
   bitvec[pos / 32] &= ~(1 << (pos & 31));
}
   

struct llvmpipe_tile_cache *
lp_create_tile_cache( struct pipe_screen *screen )
{
   struct llvmpipe_tile_cache *tc;
   uint pos;

   tc = align_malloc( sizeof(struct llvmpipe_tile_cache), 16 );
   if (tc) {
      memset(tc, 0, sizeof *tc);
      tc->screen = screen;
      for (pos = 0; pos < NUM_ENTRIES; pos++) {
         tc->entries[pos].addr.bits.invalid = 1;
      }
      tc->last_tile = &tc->entries[0]; /* any tile */
   }
   return tc;
}


void
lp_destroy_tile_cache(struct llvmpipe_tile_cache *tc)
{
   struct pipe_screen *screen;
   uint pos;

   for (pos = 0; pos < NUM_ENTRIES; pos++) {
      /*assert(tc->entries[pos].x < 0);*/
   }
   if (tc->transfer) {
      screen = tc->transfer->texture->screen;
      screen->tex_transfer_destroy(tc->transfer);
   }

   align_free( tc );
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

      tc->transfer = screen->get_tex_transfer(screen, ps->texture, ps->face,
                                              ps->level, ps->zslice,
                                              PIPE_TRANSFER_READ_WRITE,
                                              0, 0, ps->width, ps->height);
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
 * Actually clear the tiles which were flagged as being in a clear state.
 */
static void
lp_tile_cache_flush_clear(struct llvmpipe_tile_cache *tc)
{
   struct pipe_transfer *pt = tc->transfer;
   struct pipe_screen *screen = pt->texture->screen;
   const uint w = tc->transfer->width;
   const uint h = tc->transfer->height;
   uint x, y;
   uint numCleared = 0;

   /* push the tile to all positions marked as clear */
   for (y = 0; y < h; y += TILE_SIZE) {
      for (x = 0; x < w; x += TILE_SIZE) {
         union tile_address addr = tile_address(x, y, 0, 0, 0);

         if (is_clear_flag_set(tc->clear_flags, addr)) {
            unsigned tw = TILE_SIZE;
            unsigned th = TILE_SIZE;
            void *dst;

            if (pipe_clip_tile(x, y, &tw, &th, pt))
               continue;

            dst = screen->transfer_map(screen, pt);
            assert(dst);
            if(!dst)
               continue;

            pipe_fill_rect(dst, &pt->block, pt->stride,
                           x, y, tw,  th,
                           tc->clear_val);

            /* do this? */
            clear_clear_flag(tc->clear_flags, addr);

            numCleared++;
         }
      }
   }
#if 0
   debug_printf("num cleared: %u\n", numCleared);
#endif
}


/**
 * Flush the tile cache: write all dirty tiles back to the transfer.
 * any tiles "flagged" as cleared will be "really" cleared.
 */
void
lp_flush_tile_cache(struct llvmpipe_tile_cache *tc)
{
   struct pipe_transfer *pt = tc->transfer;
   int inuse = 0, pos;

   if (pt) {
      /* caching a drawing transfer */
      for (pos = 0; pos < NUM_ENTRIES; pos++) {
         struct llvmpipe_cached_tile *tile = tc->entries + pos;
         if (!tile->addr.bits.invalid) {
            lp_put_tile_rgba_soa(pt,
                                 tile->addr.bits.x * TILE_SIZE,
                                 tile->addr.bits.y * TILE_SIZE,
                                 tile->color);
            tile->addr.bits.invalid = 1;  /* mark as empty */
            inuse++;
         }
      }

#if TILE_CLEAR_OPTIMIZATION
      lp_tile_cache_flush_clear(tc);
#endif
   }

#if 0
   debug_printf("flushed tiles in use: %d\n", inuse);
#endif
}


/**
 * Get a tile from the cache.
 * \param x, y  position of tile, in pixels
 */
void *
lp_find_cached_tile(struct llvmpipe_tile_cache *tc, 
                    union tile_address addr )
{
   struct pipe_transfer *pt = tc->transfer;
   
   /* cache pos/entry: */
   const int pos = CACHE_POS(addr.bits.x,
                             addr.bits.y);
   struct llvmpipe_cached_tile *tile = tc->entries + pos;

   if (addr.value != tile->addr.value) {

      if (tile->addr.bits.invalid == 0) {
         /* put dirty tile back in framebuffer */
         lp_put_tile_rgba_soa(pt,
                              tile->addr.bits.x * TILE_SIZE,
                              tile->addr.bits.y * TILE_SIZE,
                              tile->color);
      }

      tile->addr = addr;

      if (is_clear_flag_set(tc->clear_flags, addr)) {
         /* don't get tile from framebuffer, just clear it */
         clear_tile(tile, tc->clear_color);
         clear_clear_flag(tc->clear_flags, addr);
      }
      else {
         /* get new tile data from transfer */
         lp_get_tile_rgba_soa(pt,
                              tile->addr.bits.x * TILE_SIZE,
                              tile->addr.bits.y * TILE_SIZE,
                              tile->color);
      }
   }

   tc->last_tile = tile;

   return tile->color;
}


/**
 * Given the texture face, level, zslice, x and y values, compute
 * the cache entry position/index where we'd hope to find the
 * cached texture tile.
 * This is basically a direct-map cache.
 * XXX There's probably lots of ways in which we can improve this.
 */
static INLINE uint
tex_cache_pos( union tile_address addr )
{
   uint entry = (addr.bits.x + 
                 addr.bits.y * 9 + 
                 addr.bits.z * 3 + 
                 addr.bits.face + 
                 addr.bits.level * 7);

   return entry % NUM_ENTRIES;
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
   unsigned chan;
   uint pos;

   for(chan = 0; chan < 4; ++chan)
      tc->clear_color[chan] = float_to_ubyte(rgba[chan]);

   tc->clear_val = clearValue;

#if TILE_CLEAR_OPTIMIZATION
   /* set flags to indicate all the tiles are cleared */
   memset(tc->clear_flags, 255, sizeof(tc->clear_flags));
#else
   /* disable the optimization */
   memset(tc->clear_flags, 0, sizeof(tc->clear_flags));
#endif

   for (pos = 0; pos < NUM_ENTRIES; pos++) {
      struct llvmpipe_cached_tile *tile = tc->entries + pos;
      tile->addr.bits.invalid = 1;
   }
}

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
#include "util/u_tile.h"
#include "util/u_format.h"
#include "lp_context.h"
#include "lp_texture.h"
#include "lp_tex_cache.h"



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
is_clear_flag_set(const uint *bitvec, union tex_tile_address addr)
{
   int pos, bit;
   pos = addr.bits.y * (MAX_TEX_WIDTH / TEX_TILE_SIZE) + addr.bits.x;
   assert(pos / 32 < (MAX_TEX_WIDTH / TEX_TILE_SIZE) * (MAX_TEX_HEIGHT / TEX_TILE_SIZE) / 32);
   bit = bitvec[pos / 32] & (1 << (pos & 31));
   return bit;
}
   

/**
 * Mark the tile at (x,y) as not cleared.
 */
static INLINE void
clear_clear_flag(uint *bitvec, union tex_tile_address addr)
{
   int pos;
   pos = addr.bits.y * (MAX_TEX_WIDTH / TEX_TILE_SIZE) + addr.bits.x;
   assert(pos / 32 < (MAX_TEX_WIDTH / TEX_TILE_SIZE) * (MAX_TEX_HEIGHT / TEX_TILE_SIZE) / 32);
   bitvec[pos / 32] &= ~(1 << (pos & 31));
}
   

struct llvmpipe_tex_tile_cache *
lp_create_tex_tile_cache( struct pipe_screen *screen )
{
   struct llvmpipe_tex_tile_cache *tc;
   uint pos;

   tc = CALLOC_STRUCT( llvmpipe_tex_tile_cache );
   if (tc) {
      tc->screen = screen;
      for (pos = 0; pos < NUM_ENTRIES; pos++) {
         tc->entries[pos].addr.bits.invalid = 1;
      }
      tc->last_tile = &tc->entries[0]; /* any tile */
   }
   return tc;
}


void
lp_destroy_tex_tile_cache(struct llvmpipe_tex_tile_cache *tc)
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
   if (tc->tex_trans) {
      screen = tc->tex_trans->texture->screen;
      screen->tex_transfer_destroy(tc->tex_trans);
   }

   FREE( tc );
}


void
lp_tex_tile_cache_map_transfers(struct llvmpipe_tex_tile_cache *tc)
{
   if (tc->transfer && !tc->transfer_map)
      tc->transfer_map = tc->screen->transfer_map(tc->screen, tc->transfer);

   if (tc->tex_trans && !tc->tex_trans_map)
      tc->tex_trans_map = tc->screen->transfer_map(tc->screen, tc->tex_trans);
}


void
lp_tex_tile_cache_unmap_transfers(struct llvmpipe_tex_tile_cache *tc)
{
   if (tc->transfer_map) {
      tc->screen->transfer_unmap(tc->screen, tc->transfer);
      tc->transfer_map = NULL;
   }

   if (tc->tex_trans_map) {
      tc->screen->transfer_unmap(tc->screen, tc->tex_trans);
      tc->tex_trans_map = NULL;
   }
}

void
lp_tex_tile_cache_validate_texture(struct llvmpipe_tex_tile_cache *tc)
{
   if (tc->texture) {
      struct llvmpipe_texture *lpt = llvmpipe_texture(tc->texture);
      if (lpt->timestamp != tc->timestamp) {
         /* texture was modified, invalidate all cached tiles */
         uint i;
         debug_printf("INV %d %d\n", tc->timestamp, lpt->timestamp);
         for (i = 0; i < NUM_ENTRIES; i++) {
            tc->entries[i].addr.bits.invalid = 1;
         }

         tc->timestamp = lpt->timestamp;
      }
   }
}

/**
 * Specify the texture to cache.
 */
void
lp_tex_tile_cache_set_texture(struct llvmpipe_tex_tile_cache *tc,
                          struct pipe_texture *texture)
{
   uint i;

   assert(!tc->transfer);

   if (tc->texture != texture) {
      pipe_texture_reference(&tc->texture, texture);

      if (tc->tex_trans) {
         struct pipe_screen *screen = tc->tex_trans->texture->screen;
         
         if (tc->tex_trans_map) {
            screen->transfer_unmap(screen, tc->tex_trans);
            tc->tex_trans_map = NULL;
         }

         screen->tex_transfer_destroy(tc->tex_trans);
         tc->tex_trans = NULL;
      }

      /* mark as entries as invalid/empty */
      /* XXX we should try to avoid this when the teximage hasn't changed */
      for (i = 0; i < NUM_ENTRIES; i++) {
         tc->entries[i].addr.bits.invalid = 1;
      }

      tc->tex_face = -1; /* any invalid value here */
   }
}


/**
 * Given the texture face, level, zslice, x and y values, compute
 * the cache entry position/index where we'd hope to find the
 * cached texture tile.
 * This is basically a direct-map cache.
 * XXX There's probably lots of ways in which we can improve this.
 */
static INLINE uint
tex_cache_pos( union tex_tile_address addr )
{
   uint entry = (addr.bits.x + 
                 addr.bits.y * 9 + 
                 addr.bits.z * 3 + 
                 addr.bits.face + 
                 addr.bits.level * 7);

   return entry % NUM_ENTRIES;
}

/**
 * Similar to lp_get_cached_tile() but for textures.
 * Tiles are read-only and indexed with more params.
 */
const struct llvmpipe_cached_tex_tile *
lp_find_cached_tex_tile(struct llvmpipe_tex_tile_cache *tc,
                        union tex_tile_address addr )
{
   struct pipe_screen *screen = tc->screen;
   struct llvmpipe_cached_tex_tile *tile;
   
   tile = tc->entries + tex_cache_pos( addr );

   if (addr.value != tile->addr.value) {

      /* cache miss.  Most misses are because we've invaldiated the
       * texture cache previously -- most commonly on binding a new
       * texture.  Currently we effectively flush the cache on texture
       * bind.
       */
#if 0
      _debug_printf("miss at %u:  x=%d y=%d z=%d face=%d level=%d\n"
                    "   tile %u:  x=%d y=%d z=%d face=%d level=%d\n",
                    pos, x/TEX_TILE_SIZE, y/TEX_TILE_SIZE, z, face, level,
                    pos, tile->addr.bits.x, tile->addr.bits.y, tile->z, tile->face, tile->level);
#endif

      /* check if we need to get a new transfer */
      if (!tc->tex_trans ||
          tc->tex_face != addr.bits.face ||
          tc->tex_level != addr.bits.level ||
          tc->tex_z != addr.bits.z) {
         /* get new transfer (view into texture) */

         if (tc->tex_trans) {
            if (tc->tex_trans_map) {
               tc->screen->transfer_unmap(tc->screen, tc->tex_trans);
               tc->tex_trans_map = NULL;
            }

            screen->tex_transfer_destroy(tc->tex_trans);
            tc->tex_trans = NULL;
         }

         tc->tex_trans = 
            screen->get_tex_transfer(screen, tc->texture, 
                                     addr.bits.face, 
                                     addr.bits.level, 
                                     addr.bits.z, 
                                     PIPE_TRANSFER_READ, 0, 0,
                                     tc->texture->width[addr.bits.level],
                                     tc->texture->height[addr.bits.level]);

         tc->tex_trans_map = screen->transfer_map(screen, tc->tex_trans);

         tc->tex_face = addr.bits.face;
         tc->tex_level = addr.bits.level;
         tc->tex_z = addr.bits.z;
      }

      {
         unsigned x = addr.bits.x * TEX_TILE_SIZE;
         unsigned y = addr.bits.y * TEX_TILE_SIZE;
         unsigned w = TEX_TILE_SIZE;
         unsigned h = TEX_TILE_SIZE;

         if (pipe_clip_tile(x, y, &w, &h, tc->tex_trans)) {
            assert(0);
         }

         util_format_read_4ub(tc->tex_trans->format,
                              (uint8_t *)tile->color, sizeof tile->color[0],
                              tc->tex_trans_map, tc->tex_trans->stride,
                              x, y, w, h);
      }

      tile->addr = addr;
   }

   tc->last_tile = tile;
   return tile;
}

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
#include "util/u_tile.h"
#include "util/u_math.h"
#include "sp_context.h"
#include "sp_texture.h"
#include "sp_tex_tile_cache.h"

   

struct softpipe_tex_tile_cache *
sp_create_tex_tile_cache( struct pipe_screen *screen )
{
   struct softpipe_tex_tile_cache *tc;
   uint pos;

   tc = CALLOC_STRUCT( softpipe_tex_tile_cache );
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
sp_destroy_tex_tile_cache(struct softpipe_tex_tile_cache *tc)
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
sp_tex_tile_cache_map_transfers(struct softpipe_tex_tile_cache *tc)
{
   if (tc->tex_trans && !tc->tex_trans_map)
      tc->tex_trans_map = tc->screen->transfer_map(tc->screen, tc->tex_trans);
}


void
sp_tex_tile_cache_unmap_transfers(struct softpipe_tex_tile_cache *tc)
{
   if (tc->tex_trans_map) {
      tc->screen->transfer_unmap(tc->screen, tc->tex_trans);
      tc->tex_trans_map = NULL;
   }
}

/**
 * Invalidate all cached tiles for the cached texture.
 * Should be called when the texture is modified.
 */
void
sp_tex_tile_cache_validate_texture(struct softpipe_tex_tile_cache *tc)
{
   unsigned i;

   assert(tc);
   assert(tc->texture);

   for (i = 0; i < NUM_ENTRIES; i++) {
      tc->entries[i].addr.bits.invalid = 1;
   }
}

/**
 * Specify the texture to cache.
 */
void
sp_tex_tile_cache_set_texture(struct softpipe_tex_tile_cache *tc,
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
 * Flush the tile cache: write all dirty tiles back to the transfer.
 * any tiles "flagged" as cleared will be "really" cleared.
 */
void
sp_flush_tex_tile_cache(struct softpipe_tex_tile_cache *tc)
{
   int pos;

   if (tc->texture) {
      /* caching a texture, mark all entries as empty */
      for (pos = 0; pos < NUM_ENTRIES; pos++) {
         tc->entries[pos].addr.bits.invalid = 1;
      }
      tc->tex_face = -1;
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
 * Similar to sp_get_cached_tile() but for textures.
 * Tiles are read-only and indexed with more params.
 */
const struct softpipe_tex_cached_tile *
sp_find_cached_tile_tex(struct softpipe_tex_tile_cache *tc, 
                        union tex_tile_address addr )
{
   struct pipe_screen *screen = tc->screen;
   struct softpipe_tex_cached_tile *tile;
   
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
                    pos, x/TILE_SIZE, y/TILE_SIZE, z, face, level,
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
                                     u_minify(tc->texture->width0, addr.bits.level),
                                     u_minify(tc->texture->height0, addr.bits.level));
         
         tc->tex_trans_map = screen->transfer_map(screen, tc->tex_trans);

         tc->tex_face = addr.bits.face;
         tc->tex_level = addr.bits.level;
         tc->tex_z = addr.bits.z;
      }

      /* get tile from the transfer (view into texture) */
      pipe_get_tile_rgba(tc->tex_trans,
                         addr.bits.x * TILE_SIZE, 
                         addr.bits.y * TILE_SIZE,
                         TILE_SIZE, TILE_SIZE,
                         (float *) tile->data.color);
      tile->addr = addr;
   }

   tc->last_tile = tile;
   return tile;
}




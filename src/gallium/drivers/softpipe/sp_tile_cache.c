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
#include "sp_context.h"
#include "sp_surface.h"
#include "sp_texture.h"
#include "sp_tile_cache.h"



/**
 * Return the position in the cache for the tile that contains win pos (x,y).
 * We currently use a direct mapped cache so this is like a hack key.
 * At some point we should investige something more sophisticated, like
 * a LRU replacement policy.
 */
#define CACHE_POS(x, y) \
   (((x) / TILE_SIZE + ((y) / TILE_SIZE) * 5) % NUM_ENTRIES)



/**
 * Is the tile at (x,y) in cleared state?
 */
static INLINE uint
is_clear_flag_set(const uint *bitvec, int x, int y)
{
   int pos, bit;
   x /= TILE_SIZE;
   y /= TILE_SIZE;
   pos = y * (MAX_WIDTH / TILE_SIZE) + x;
   assert(pos / 32 < (MAX_WIDTH / TILE_SIZE) * (MAX_HEIGHT / TILE_SIZE) / 32);
   bit = bitvec[pos / 32] & (1 << (pos & 31));
   return bit;
}
   

/**
 * Mark the tile at (x,y) as not cleared.
 */
static INLINE void
clear_clear_flag(uint *bitvec, int x, int y)
{
   int pos;
   x /= TILE_SIZE;
   y /= TILE_SIZE;
   pos = y * (MAX_WIDTH / TILE_SIZE) + x;
   assert(pos / 32 < (MAX_WIDTH / TILE_SIZE) * (MAX_HEIGHT / TILE_SIZE) / 32);
   bitvec[pos / 32] &= ~(1 << (pos & 31));
}
   

struct softpipe_tile_cache *
sp_create_tile_cache( struct pipe_screen *screen )
{
   struct softpipe_tile_cache *tc;
   uint pos;

   tc = CALLOC_STRUCT( softpipe_tile_cache );
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
sp_destroy_tile_cache(struct softpipe_tile_cache *tc)
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


/**
 * Specify the surface to cache.
 */
void
sp_tile_cache_set_surface(struct softpipe_tile_cache *tc,
                          struct pipe_surface *ps)
{
   assert(!tc->texture);

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

      tc->depth_stencil = (ps->format == PIPE_FORMAT_S8Z24_UNORM ||
                           ps->format == PIPE_FORMAT_X8Z24_UNORM ||
                           ps->format == PIPE_FORMAT_Z24S8_UNORM ||
                           ps->format == PIPE_FORMAT_Z24X8_UNORM ||
                           ps->format == PIPE_FORMAT_Z16_UNORM ||
                           ps->format == PIPE_FORMAT_Z32_UNORM ||
                           ps->format == PIPE_FORMAT_S8_UNORM);
   }
}


/**
 * Return the transfer being cached.
 */
struct pipe_surface *
sp_tile_cache_get_surface(struct softpipe_tile_cache *tc)
{
   return tc->surface;
}


void
sp_tile_cache_map_transfers(struct softpipe_tile_cache *tc)
{
   if (tc->transfer && !tc->transfer_map)
      tc->transfer_map = tc->screen->transfer_map(tc->screen, tc->transfer);

   if (tc->tex_trans && !tc->tex_trans_map)
      tc->tex_trans_map = tc->screen->transfer_map(tc->screen, tc->tex_trans);
}


void
sp_tile_cache_unmap_transfers(struct softpipe_tile_cache *tc)
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
sp_tile_cache_validate_texture(struct softpipe_tile_cache *tc)
{
   if (tc->texture) {
      struct softpipe_texture *spt = softpipe_texture(tc->texture);
      if (spt->timestamp != tc->timestamp) {
         /* texture was modified, invalidate all cached tiles */
         uint i;
         _debug_printf("INV %d %d\n", tc->timestamp, spt->timestamp);
         for (i = 0; i < NUM_ENTRIES; i++) {
            tc->entries[i].addr.bits.invalid = 1;
         }

         tc->timestamp = spt->timestamp;
      }
   }
}

/**
 * Specify the texture to cache.
 */
void
sp_tile_cache_set_texture(struct softpipe_tile_cache *tc,
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
 * Set pixels in a tile to the given clear color/value, float.
 */
static void
clear_tile_rgba(struct softpipe_cached_tile *tile,
                enum pipe_format format,
                const float clear_value[4])
{
   if (clear_value[0] == 0.0 &&
       clear_value[1] == 0.0 &&
       clear_value[2] == 0.0 &&
       clear_value[3] == 0.0) {
      memset(tile->data.color, 0, sizeof(tile->data.color));
   }
   else {
      uint i, j;
      for (i = 0; i < TILE_SIZE; i++) {
         for (j = 0; j < TILE_SIZE; j++) {
            tile->data.color[i][j][0] = clear_value[0];
            tile->data.color[i][j][1] = clear_value[1];
            tile->data.color[i][j][2] = clear_value[2];
            tile->data.color[i][j][3] = clear_value[3];
         }
      }
   }
}


/**
 * Set a tile to a solid value/color.
 */
static void
clear_tile(struct softpipe_cached_tile *tile,
           enum pipe_format format,
           uint clear_value)
{
   uint i, j;

   switch (pf_get_size(format)) {
   case 1:
      memset(tile->data.any, 0, TILE_SIZE * TILE_SIZE);
      break;
   case 2:
      if (clear_value == 0) {
         memset(tile->data.any, 0, 2 * TILE_SIZE * TILE_SIZE);
      }
      else {
         for (i = 0; i < TILE_SIZE; i++) {
            for (j = 0; j < TILE_SIZE; j++) {
               tile->data.depth16[i][j] = (ushort) clear_value;
            }
         }
      }
      break;
   case 4:
      if (clear_value == 0) {
         memset(tile->data.any, 0, 4 * TILE_SIZE * TILE_SIZE);
      }
      else {
         for (i = 0; i < TILE_SIZE; i++) {
            for (j = 0; j < TILE_SIZE; j++) {
               tile->data.color32[i][j] = clear_value;
            }
         }
      }
      break;
   default:
      assert(0);
   }
}


/**
 * Actually clear the tiles which were flagged as being in a clear state.
 */
static void
sp_tile_cache_flush_clear(struct softpipe_tile_cache *tc)
{
   struct pipe_transfer *pt = tc->transfer;
   const uint w = tc->transfer->width;
   const uint h = tc->transfer->height;
   uint x, y;
   uint numCleared = 0;

   /* clear the scratch tile to the clear value */
   clear_tile(&tc->tile, pt->format, tc->clear_val);

   /* push the tile to all positions marked as clear */
   for (y = 0; y < h; y += TILE_SIZE) {
      for (x = 0; x < w; x += TILE_SIZE) {
         if (is_clear_flag_set(tc->clear_flags, x, y)) {
            pipe_put_tile_raw(pt,
                              x, y, TILE_SIZE, TILE_SIZE,
                              tc->tile.data.color32, 0/*STRIDE*/);

            /* do this? */
            clear_clear_flag(tc->clear_flags, x, y);

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
sp_flush_tile_cache(struct softpipe_tile_cache *tc)
{
   struct pipe_transfer *pt = tc->transfer;
   int inuse = 0, pos;

   if (pt) {
      /* caching a drawing transfer */
      for (pos = 0; pos < NUM_ENTRIES; pos++) {
         struct softpipe_cached_tile *tile = tc->entries + pos;
         if (!tile->addr.bits.invalid) {
            if (tc->depth_stencil) {
               pipe_put_tile_raw(pt,
                                 tile->addr.bits.x * TILE_SIZE, 
                                 tile->addr.bits.y * TILE_SIZE, 
                                 TILE_SIZE, TILE_SIZE,
                                 tile->data.depth32, 0/*STRIDE*/);
            }
            else {
               pipe_put_tile_rgba(pt,
                                  tile->addr.bits.x * TILE_SIZE, 
                                  tile->addr.bits.y * TILE_SIZE, 
                                  TILE_SIZE, TILE_SIZE,
                                  (float *) tile->data.color);
            }
            tile->addr.bits.invalid = 1;  /* mark as empty */
            inuse++;
         }
      }

#if TILE_CLEAR_OPTIMIZATION
      sp_tile_cache_flush_clear(tc);
#endif
   }
   else if (tc->texture) {
      /* caching a texture, mark all entries as empty */
      for (pos = 0; pos < NUM_ENTRIES; pos++) {
         tc->entries[pos].addr.bits.invalid = 1;
      }
      tc->tex_face = -1;
   }

#if 0
   debug_printf("flushed tiles in use: %d\n", inuse);
#endif
}


/**
 * Get a tile from the cache.
 * \param x, y  position of tile, in pixels
 */
struct softpipe_cached_tile *
sp_get_cached_tile(struct softpipe_tile_cache *tc, int x, int y)
{
   struct pipe_transfer *pt = tc->transfer;
   
   /* tile pos in framebuffer: */
   union tile_address addr = tile_address( x, y, 0, 0, 0 );
   /* cache pos/entry: */
   const int pos = CACHE_POS(x, y);
   struct softpipe_cached_tile *tile = tc->entries + pos;

   if (addr.value != tile->addr.value) {

      if (tile->addr.bits.invalid == 0) {
         /* put dirty tile back in framebuffer */
         if (tc->depth_stencil) {
            pipe_put_tile_raw(pt,
                              tile->addr.bits.x * TILE_SIZE,
                              tile->addr.bits.y * TILE_SIZE,
                              TILE_SIZE, TILE_SIZE,
                              tile->data.depth32, 0/*STRIDE*/);
         }
         else {
            pipe_put_tile_rgba(pt,
                               tile->addr.bits.x * TILE_SIZE,
                               tile->addr.bits.y * TILE_SIZE,
                               TILE_SIZE, TILE_SIZE,
                               (float *) tile->data.color);
         }
      }

      tile->addr = addr;

      if (is_clear_flag_set(tc->clear_flags, x, y)) {
         /* don't get tile from framebuffer, just clear it */
         if (tc->depth_stencil) {
            clear_tile(tile, pt->format, tc->clear_val);
         }
         else {
            clear_tile_rgba(tile, pt->format, tc->clear_color);
         }
         clear_clear_flag(tc->clear_flags, x, y);
      }
      else {
         /* get new tile data from transfer */
         if (tc->depth_stencil) {
            pipe_get_tile_raw(pt,
                              tile->addr.bits.x * TILE_SIZE, 
                              tile->addr.bits.y * TILE_SIZE, 
                              TILE_SIZE, TILE_SIZE,
                              tile->data.depth32, 0/*STRIDE*/);
         }
         else {
            pipe_get_tile_rgba(pt,
                               tile->addr.bits.x * TILE_SIZE, 
                               tile->addr.bits.y * TILE_SIZE,
                               TILE_SIZE, TILE_SIZE,
                               (float *) tile->data.color);
         }
      }
   }

   return tile;
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
 * Similar to sp_get_cached_tile() but for textures.
 * Tiles are read-only and indexed with more params.
 */
const struct softpipe_cached_tile *
sp_find_cached_tile_tex(struct softpipe_tile_cache *tc, 
                        union tile_address addr )
{
   struct pipe_screen *screen = tc->screen;
   struct softpipe_cached_tile *tile;
   
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
                                     tc->texture->width[addr.bits.level],
                                     tc->texture->height[addr.bits.level]);

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


/**
 * When a whole surface is being cleared to a value we can avoid
 * fetching tiles above.
 * Save the color and set a 'clearflag' for each tile of the screen.
 */
void
sp_tile_cache_clear(struct softpipe_tile_cache *tc, const float *rgba,
                    uint clearValue)
{
   uint pos;

   tc->clear_color[0] = rgba[0];
   tc->clear_color[1] = rgba[1];
   tc->clear_color[2] = rgba[2];
   tc->clear_color[3] = rgba[3];

   tc->clear_val = clearValue;

#if TILE_CLEAR_OPTIMIZATION
   /* set flags to indicate all the tiles are cleared */
   memset(tc->clear_flags, 255, sizeof(tc->clear_flags));
#else
   /* disable the optimization */
   memset(tc->clear_flags, 0, sizeof(tc->clear_flags));
#endif

   for (pos = 0; pos < NUM_ENTRIES; pos++) {
      struct softpipe_cached_tile *tile = tc->entries + pos;
      tile->addr.bits.invalid = 1;
   }
}

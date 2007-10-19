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
 * Framebuffer/surface tile caching.
 *
 * Author:
 *    Brian Paul
 */


#include "sp_context.h"
#include "sp_surface.h"
#include "sp_tile_cache.h"


#define NUM_ENTRIES 20


/** XXX move these */
#define MAX_WIDTH 2048
#define MAX_HEIGHT 2048


struct softpipe_tile_cache
{
   struct softpipe_cached_tile entries[NUM_ENTRIES];
   uint clear_flags[(MAX_WIDTH / TILE_SIZE) * (MAX_HEIGHT / TILE_SIZE) / 32];
};


/**
 * Return the position in the cache for the tile that contains win pos (x,y).
 * We currently use a direct mapped cache so this is like a hack key.
 * At some point we should investige something more sophisticated, like
 * a LRU replacement policy.
 */
#define CACHE_POS(x, y) \
   (((x) / TILE_SIZE + ((y) / TILE_SIZE) * 5) % NUM_ENTRIES)



static uint
is_clear_flag_set(const uint *bitvec, int x, int y)
{
   int pos, bit;
   x /= TILE_SIZE;
   y /= TILE_SIZE;
   pos = y * (MAX_WIDTH / TILE_SIZE) + x;
   bit = bitvec[pos / 32] & (1 << (pos & 31));
   return bit;
}
   

static void
clear_clear_flag(uint *bitvec, int x, int y)
{
   int pos;
   x /= TILE_SIZE;
   y /= TILE_SIZE;
   pos = y * (MAX_WIDTH / TILE_SIZE) + x;
   bitvec[pos / 32] &= ~(1 << (pos & 31));
}
   

struct softpipe_tile_cache *
sp_create_tile_cache(void)
{
   struct softpipe_tile_cache *tc;
   uint pos;

   tc = calloc(1, sizeof(*tc));
   if (tc) {
      for (pos = 0; pos < NUM_ENTRIES; pos++) {
         tc->entries[pos].x =
         tc->entries[pos].y = -1;
      }
   }
   return tc;
}


void
sp_destroy_tile_cache(struct softpipe_tile_cache *tc)
{
   uint pos;
   for (pos = 0; pos < NUM_ENTRIES; pos++) {
      assert(tc->entries[pos].x < 0);
   }
   free(tc);
}



void
sp_flush_tile_cache(struct softpipe_surface *sps)
{
   struct softpipe_tile_cache *tc = sps->tc;
   /*
   struct softpipe_surface *zsurf = softpipe_surface(softpipe->zbuf);
   */
   int inuse = 0, pos;

   for (pos = 0; pos < NUM_ENTRIES; pos++) {
      struct softpipe_cached_tile *tile = tc->entries + pos;
      if (tile->x >= 0) {
         sps->surface.put_tile(&sps->surface,
                               tile->x,
                               tile->y,
                               TILE_SIZE, TILE_SIZE,
                               (float *) tile->data.color);
         /*
         sps->surface.put_tile(&zsurf->surface,
                               tile->x,
                               tile->y,
                               TILE_SIZE, TILE_SIZE,
                               (float *) tile->depth);
         */

         tile->x = tile->y = -1;  /* mark as empty */
         inuse++;
      }
   }

   /*printf("flushed tiles in use: %d\n", inuse);*/
}


struct softpipe_cached_tile *
sp_get_cached_tile(struct softpipe_surface *sps, int x, int y)
{
   struct softpipe_tile_cache *tc = sps->tc;

   /* tile pos in framebuffer: */
   const int tile_x = x & ~(TILE_SIZE - 1);
   const int tile_y = y & ~(TILE_SIZE - 1);

   /* cache pos/entry: */
   const int pos = CACHE_POS(x, y);
   struct softpipe_cached_tile *tile = tc->entries + pos;

   /*
   printf("output %d, %d at pos %d\n", x, y, pos);
   */

   if (tile_x != tile->x ||
       tile_y != tile->y) {

      /*
      printf("new tile at %d, %d, pos %d\n", tile_x, tile_y, pos);
      */

      if (tile->x != -1) {
         /* put dirty tile back in framebuffer */
         sps->surface.put_tile(&sps->surface, tile->x, tile->y,
                               TILE_SIZE, TILE_SIZE, (float *) tile->data.color);
      }

      if (is_clear_flag_set(tc->clear_flags, x, y)) {
         printf("clear tile\n");
#if 0
         uint i, j;
         for (i = 0; i < TILE_SIZE; i++) {
            for (j = 0; j < TILE_SIZE; j++) {
               tile->data.color[i][j][0] = 0.5;
               tile->data.color[i][j][1] = 0.5;
               tile->data.color[i][j][2] = 0.5;
               tile->data.color[i][j][3] = 0.5;
            }
         }
#endif
         memset(tile->data.color, 0, sizeof(tile->data.color));
         clear_clear_flag(tc->clear_flags, x, y);
      }
      else {
         /* get new tile from framebuffer */
         sps->surface.get_tile(&sps->surface, tile_x, tile_y,
                               TILE_SIZE, TILE_SIZE, (float *) tile->data.color);
      }

      tile->x = tile_x;
      tile->y = tile_y;
   }

   return tile;
}


void
sp_clear_tile_cache(struct softpipe_surface *sps, unsigned clearval)
{
   (void) clearval; /* XXX use this */
   memset(sps->tc->clear_flags, 255, sizeof(sps->tc->clear_flags));
}

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

#define CLEAR_OPTIMIZATION 0

#define NUM_ENTRIES 20


/** XXX move these */
#define MAX_WIDTH 2048
#define MAX_HEIGHT 2048


struct softpipe_tile_cache
{
   struct softpipe_surface *surface;  /**< the surface we're caching */
   struct pipe_mipmap_tree *texture;  /**< if caching a texture */
   struct softpipe_cached_tile entries[NUM_ENTRIES];
   uint clear_flags[(MAX_WIDTH / TILE_SIZE) * (MAX_HEIGHT / TILE_SIZE) / 32];
   float clear_value[4];
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
   assert(pos / 32 < (MAX_WIDTH / TILE_SIZE) * (MAX_HEIGHT / TILE_SIZE) / 32);
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
   assert(pos / 32 < (MAX_WIDTH / TILE_SIZE) * (MAX_HEIGHT / TILE_SIZE) / 32);
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
sp_tile_cache_set_surface(struct softpipe_tile_cache *tc,
                          struct softpipe_surface *sps)
{
   tc->surface = sps;
}


struct softpipe_surface *
sp_tile_cache_get_surface(struct softpipe_tile_cache *tc)
{
   return tc->surface;
}


void
sp_tile_cache_set_texture(struct softpipe_tile_cache *tc,
                          struct pipe_mipmap_tree *texture)
{
   uint i;

   tc->texture = texture;

   /* mark as entries as invalid/empty */
   /* XXX we should try to avoid this when the teximage hasn't changed */
   for (i = 0; i < NUM_ENTRIES; i++) {
      tc->entries[i].x = -1;
   }
}


void
sp_flush_tile_cache(struct softpipe_context *softpipe,
                    struct softpipe_tile_cache *tc)
{
   struct pipe_context *pipe = &softpipe->pipe;
   struct pipe_surface *ps = &tc->surface->surface;
   boolean is_depth_stencil;
   int inuse = 0, pos;

   if (!ps || !ps->region || !ps->region->map)
      return;

   is_depth_stencil = (ps->format == PIPE_FORMAT_S8_Z24 ||
                       ps->format == PIPE_FORMAT_U_Z16 ||
                       ps->format == PIPE_FORMAT_U_Z32 ||
                       ps->format == PIPE_FORMAT_U_S8);

   for (pos = 0; pos < NUM_ENTRIES; pos++) {
      struct softpipe_cached_tile *tile = tc->entries + pos;
      if (tile->x >= 0) {
         if (is_depth_stencil) {
            pipe->put_tile(pipe, ps,
                           tile->x, tile->y, TILE_SIZE, TILE_SIZE,
                           tile->data.depth32, 0/*STRIDE*/);
         }
         else {
            pipe->put_tile_rgba(pipe, ps,
                                tile->x, tile->y, TILE_SIZE, TILE_SIZE,
                                (float *) tile->data.color);
         }

         tile->x = tile->y = -1;  /* mark as empty */
         inuse++;
      }
   }

   /*
   printf("flushed tiles in use: %d\n", inuse);
   */
}


struct softpipe_cached_tile *
sp_get_cached_tile(struct softpipe_context *softpipe,
                   struct softpipe_tile_cache *tc, int x, int y)
{
   struct pipe_context *pipe = &softpipe->pipe;
   struct pipe_surface *ps = &tc->surface->surface;
   boolean is_depth_stencil
      = (ps->format == PIPE_FORMAT_S8_Z24 ||
         ps->format == PIPE_FORMAT_U_Z16 ||
         ps->format == PIPE_FORMAT_U_Z32 ||
         ps->format == PIPE_FORMAT_U_S8);

   /* tile pos in framebuffer: */
   const int tile_x = x & ~(TILE_SIZE - 1);
   const int tile_y = y & ~(TILE_SIZE - 1);

   /* cache pos/entry: */
   const int pos = CACHE_POS(x, y);
   struct softpipe_cached_tile *tile = tc->entries + pos;

   if (tile_x != tile->x ||
       tile_y != tile->y) {

      if (tile->x != -1) {
         /* put dirty tile back in framebuffer */
         if (is_depth_stencil) {
            pipe->put_tile(pipe, ps,
                           tile->x, tile->y, TILE_SIZE, TILE_SIZE,
                           tile->data.depth32, 0 /*STRIDE*/);
         }
         else {
            pipe->put_tile_rgba(pipe, ps,
                                tile->x, tile->y, TILE_SIZE, TILE_SIZE,
                                (float *) tile->data.color);
         }
      }

      if (is_clear_flag_set(tc->clear_flags, x, y)) {
         /* don't get tile from framebuffer, just clear it */
         uint i, j;
         /* XXX these loops could be optimized */
         switch (ps->format) {
         case PIPE_FORMAT_U_Z16:
            {
               ushort clear_val = (ushort) (tc->clear_value[0] * 0xffff);
               for (i = 0; i < TILE_SIZE; i++) {
                  for (j = 0; j < TILE_SIZE; j++) {
                     tile->data.depth16[i][j] = clear_val;
                  }
               }
            }
            break;
         case PIPE_FORMAT_U_Z32:
            {
               uint clear_val = (uint) (tc->clear_value[0] * 0xffffffff);
               for (i = 0; i < TILE_SIZE; i++) {
                  for (j = 0; j < TILE_SIZE; j++) {
                     tile->data.depth32[i][j] = clear_val;
                  }
               }
            }
            break;
         case PIPE_FORMAT_S8_Z24:
            {
               uint clear_val = (uint) (tc->clear_value[0] * 0xffffff);
               clear_val |= ((uint) tc->clear_value[1]) << 24;
               for (i = 0; i < TILE_SIZE; i++) {
                  for (j = 0; j < TILE_SIZE; j++) {
                     tile->data.depth32[i][j] = clear_val;
                  }
               }
            }
            break;
         case PIPE_FORMAT_U_S8:
            {
               ubyte clear_val = (uint) tc->clear_value[0];
               for (i = 0; i < TILE_SIZE; i++) {
                  for (j = 0; j < TILE_SIZE; j++) {
                     tile->data.stencil8[i][j] = clear_val;
                  }
               }
            }
            break;
         default:
            /* color */
            for (i = 0; i < TILE_SIZE; i++) {
               for (j = 0; j < TILE_SIZE; j++) {
                  tile->data.color[i][j][0] = tc->clear_value[0];
                  tile->data.color[i][j][1] = tc->clear_value[1];
                  tile->data.color[i][j][2] = tc->clear_value[2];
                  tile->data.color[i][j][3] = tc->clear_value[3];
               }
            }
         }
         clear_clear_flag(tc->clear_flags, x, y);
      }
      else {
         /* get new tile from framebuffer */
         if (is_depth_stencil) {
            pipe->get_tile(pipe, ps,
                           tile_x, tile_y, TILE_SIZE, TILE_SIZE,
                           tile->data.depth32, 0/*STRIDE*/);
         }
         else {
            pipe->get_tile_rgba(pipe, ps,
                                tile_x, tile_y, TILE_SIZE, TILE_SIZE,
                                (float *) tile->data.color);
         }
      }

      tile->x = tile_x;
      tile->y = tile_y;
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
tex_cache_pos(int x, int y, int z, int face, int level)
{
   uint entry = x + y * 5 + z * 4 + face + level;
   return entry % NUM_ENTRIES;
}


/**
 * Similar to sp_get_cached_tile() but for textures.
 * Tiles are read-only and indexed with more params.
 */
const struct softpipe_cached_tile *
sp_get_cached_tile_tex(struct pipe_context *pipe,
                       struct softpipe_tile_cache *tc, int x, int y, int z,
                       int face, int level)
{
   /* tile pos in framebuffer: */
   const int tile_x = x & ~(TILE_SIZE - 1);
   const int tile_y = y & ~(TILE_SIZE - 1);
   /* cache pos/entry: */
   const uint pos = tex_cache_pos(x / TILE_SIZE, y / TILE_SIZE, z,
                                  face, level);
   struct softpipe_cached_tile *tile = tc->entries + pos;

   if (tile_x != tile->x ||
       tile_y != tile->y ||
       z != tile->z ||
       face != tile->face ||
       level != tile->level) {
      /* XXX this call is a bit heavier than we'd like: */
      struct pipe_surface *ps
         = pipe->get_tex_surface(pipe, tc->texture, face, level, z);

      pipe->get_tile_rgba(pipe, ps,
                          tile_x, tile_y, TILE_SIZE, TILE_SIZE,
                          (float *) tile->data.color);

      pipe_surface_reference(&ps, NULL);

      tile->x = tile_x;
      tile->y = tile_y;
      tile->z = z;
      tile->face = face;
      tile->level = level;
   }

   return tile;
}


/**
 * When a whole surface is being cleared to a value we can avoid
 * fetching tiles above.
 * Save the color and set a 'clearflag' for each tile of the screen.
 */
void
sp_tile_cache_clear(struct softpipe_tile_cache *tc, const float value[4])
{
   tc->clear_value[0] = value[0];
   tc->clear_value[1] = value[1];
   tc->clear_value[2] = value[2];
   tc->clear_value[3] = value[3];

#if CLEAR_OPTIMIZATION
   memset(tc->clear_flags, 255, sizeof(tc->clear_flags));
#else
   memset(tc->clear_flags, 0, sizeof(tc->clear_flags));
#endif
}

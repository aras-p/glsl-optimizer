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

#include "pipe/p_inlines.h"
#include "util/u_memory.h"
#include "util/u_tile.h"
#include "sp_context.h"
#include "sp_surface.h"
#include "sp_texture.h"
#include "sp_tile_cache.h"

#define NUM_ENTRIES 32


/** XXX move these */
#define MAX_WIDTH 2048
#define MAX_HEIGHT 2048


struct softpipe_tile_cache
{
   struct pipe_screen *screen;
   struct pipe_surface *surface;  /**< the surface we're caching */
   void *surface_map;
   struct pipe_texture *texture;  /**< if caching a texture */
   struct softpipe_cached_tile entries[NUM_ENTRIES];
   uint clear_flags[(MAX_WIDTH / TILE_SIZE) * (MAX_HEIGHT / TILE_SIZE) / 32];
   float clear_color[4];
   uint clear_val;
   boolean depth_stencil; /** Is the surface a depth/stencil format? */

   struct pipe_surface *tex_surf;
   void *tex_surf_map;
   int tex_face, tex_level, tex_z;

   struct softpipe_cached_tile tile;  /**< scratch tile for clears */
};


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
      /*assert(tc->entries[pos].x < 0);*/
   }
   if (tc->surface) {
      pipe_surface_reference(&tc->surface, NULL);
   }
   if (tc->tex_surf) {
      pipe_surface_reference(&tc->tex_surf, NULL);
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

   if (tc->surface_map) {
      tc->screen->surface_unmap(tc->screen, tc->surface);
      tc->surface_map = NULL;
   }

   pipe_surface_reference(&tc->surface, ps);

   if (tc->surface) {
      if (tc->surface_map) /* XXX: this is always NULL!? */
	 tc->surface_map = tc->screen->surface_map(tc->screen, tc->surface,
                                                   PIPE_BUFFER_USAGE_CPU_READ | 
                                                   PIPE_BUFFER_USAGE_CPU_WRITE);

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
 * Return the surface being cached.
 */
struct pipe_surface *
sp_tile_cache_get_surface(struct softpipe_tile_cache *tc)
{
   return tc->surface;
}


void
sp_tile_cache_map_surfaces(struct softpipe_tile_cache *tc)
{
   if (tc->surface && !tc->surface_map)
      tc->surface_map = tc->screen->surface_map(tc->screen, tc->surface,
                                                PIPE_BUFFER_USAGE_CPU_WRITE |
                                                PIPE_BUFFER_USAGE_CPU_READ);

   if (tc->tex_surf && !tc->tex_surf_map)
      tc->tex_surf_map = tc->screen->surface_map(tc->screen, tc->tex_surf,
                                                 PIPE_BUFFER_USAGE_CPU_READ);
}


void
sp_tile_cache_unmap_surfaces(struct softpipe_tile_cache *tc)
{
   if (tc->surface_map) {
      tc->screen->surface_unmap(tc->screen, tc->surface);
      tc->surface_map = NULL;
   }

   if (tc->tex_surf_map) {
      tc->screen->surface_unmap(tc->screen, tc->tex_surf);
      tc->tex_surf_map = NULL;
   }
}


/**
 * Specify the texture to cache.
 */
void
sp_tile_cache_set_texture(struct pipe_context *pipe,
                          struct softpipe_tile_cache *tc,
                          struct pipe_texture *texture)
{
   uint i;

   assert(!tc->surface);

   pipe_texture_reference(&tc->texture, texture);

   if (tc->tex_surf_map) {
      tc->screen->surface_unmap(tc->screen, tc->tex_surf);
      tc->tex_surf_map = NULL;
   }
   pipe_surface_reference(&tc->tex_surf, NULL);

   /* mark as entries as invalid/empty */
   /* XXX we should try to avoid this when the teximage hasn't changed */
   for (i = 0; i < NUM_ENTRIES; i++) {
      tc->entries[i].x = -1;
   }

   tc->tex_face = -1; /* any invalid value here */
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
sp_tile_cache_flush_clear(struct pipe_context *pipe,
                          struct softpipe_tile_cache *tc)
{
   struct pipe_surface *ps = tc->surface;
   const uint w = tc->surface->width;
   const uint h = tc->surface->height;
   uint x, y;
   uint numCleared = 0;

   /* clear the scratch tile to the clear value */
   clear_tile(&tc->tile, ps->format, tc->clear_val);

   /* push the tile to all positions marked as clear */
   for (y = 0; y < h; y += TILE_SIZE) {
      for (x = 0; x < w; x += TILE_SIZE) {
         if (is_clear_flag_set(tc->clear_flags, x, y)) {
            pipe_put_tile_raw(ps,
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
 * Flush the tile cache: write all dirty tiles back to the surface.
 * any tiles "flagged" as cleared will be "really" cleared.
 */
void
sp_flush_tile_cache(struct softpipe_context *softpipe,
                    struct softpipe_tile_cache *tc)
{
   struct pipe_surface *ps = tc->surface;
   int inuse = 0, pos;

   if (ps && ps->buffer) {
      /* caching a drawing surface */
      for (pos = 0; pos < NUM_ENTRIES; pos++) {
         struct softpipe_cached_tile *tile = tc->entries + pos;
         if (tile->x >= 0) {
            if (tc->depth_stencil) {
               pipe_put_tile_raw(ps,
                                 tile->x, tile->y, TILE_SIZE, TILE_SIZE,
                                 tile->data.depth32, 0/*STRIDE*/);
            }
            else {
               pipe_put_tile_rgba(ps,
                                  tile->x, tile->y, TILE_SIZE, TILE_SIZE,
                                  (float *) tile->data.color);
            }
            tile->x = tile->y = -1;  /* mark as empty */
            inuse++;
         }
      }

#if TILE_CLEAR_OPTIMIZATION
      sp_tile_cache_flush_clear(&softpipe->pipe, tc);
#endif
   }
   else if (tc->texture) {
      /* caching a texture, mark all entries as empty */
      for (pos = 0; pos < NUM_ENTRIES; pos++) {
         tc->entries[pos].x = -1;
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
sp_get_cached_tile(struct softpipe_context *softpipe,
                   struct softpipe_tile_cache *tc, int x, int y)
{
   struct pipe_surface *ps = tc->surface;

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
         if (tc->depth_stencil) {
            pipe_put_tile_raw(ps,
                              tile->x, tile->y, TILE_SIZE, TILE_SIZE,
                              tile->data.depth32, 0/*STRIDE*/);
         }
         else {
            pipe_put_tile_rgba(ps,
                               tile->x, tile->y, TILE_SIZE, TILE_SIZE,
                               (float *) tile->data.color);
         }
      }

      tile->x = tile_x;
      tile->y = tile_y;

      if (is_clear_flag_set(tc->clear_flags, x, y)) {
         /* don't get tile from framebuffer, just clear it */
         if (tc->depth_stencil) {
            clear_tile(tile, ps->format, tc->clear_val);
         }
         else {
            clear_tile_rgba(tile, ps->format, tc->clear_color);
         }
         clear_clear_flag(tc->clear_flags, x, y);
      }
      else {
         /* get new tile data from surface */
         if (tc->depth_stencil) {
            pipe_get_tile_raw(ps,
                              tile->x, tile->y, TILE_SIZE, TILE_SIZE,
                              tile->data.depth32, 0/*STRIDE*/);
         }
         else {
            pipe_get_tile_rgba(ps,
                               tile->x, tile->y, TILE_SIZE, TILE_SIZE,
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
   struct pipe_screen *screen = pipe->screen;
   /* tile pos in framebuffer: */
   const int tile_x = x & ~(TILE_SIZE - 1);
   const int tile_y = y & ~(TILE_SIZE - 1);
   /* cache pos/entry: */
   const uint pos = tex_cache_pos(x / TILE_SIZE, y / TILE_SIZE, z,
                                  face, level);
   struct softpipe_cached_tile *tile = tc->entries + pos;

   if (tc->texture) {
      struct softpipe_texture *spt = softpipe_texture(tc->texture);
      if (spt->modified) {
         /* texture was modified, force a cache reload */
         tile->x = -1;
         spt->modified = FALSE;
      }
   }

   if (tile_x != tile->x ||
       tile_y != tile->y ||
       z != tile->z ||
       face != tile->face ||
       level != tile->level) {
      /* cache miss */

      /* check if we need to get a new surface */
      if (!tc->tex_surf ||
          tc->tex_face != face ||
          tc->tex_level != level ||
          tc->tex_z != z) {
         /* get new surface (view into texture) */

	 if (tc->tex_surf_map)
            tc->screen->surface_unmap(tc->screen, tc->tex_surf);

         tc->tex_surf = screen->get_tex_surface(screen, tc->texture, face, level, z, 
                                                PIPE_BUFFER_USAGE_CPU_READ);
         tc->tex_surf_map = screen->surface_map(screen, tc->tex_surf,
                                                PIPE_BUFFER_USAGE_CPU_READ);

         tc->tex_face = face;
         tc->tex_level = level;
         tc->tex_z = z;
      }

      /* get tile from the surface (view into texture) */
      pipe_get_tile_rgba(tc->tex_surf,
                         tile_x, tile_y, TILE_SIZE, TILE_SIZE,
                         (float *) tile->data.color);
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
sp_tile_cache_clear(struct softpipe_tile_cache *tc, uint clearValue)
{
   uint r, g, b, a;
   uint pos;

   tc->clear_val = clearValue;

   switch (tc->surface->format) {
   case PIPE_FORMAT_R8G8B8A8_UNORM:
      r = (clearValue >> 24) & 0xff;
      g = (clearValue >> 16) & 0xff;
      b = (clearValue >>  8) & 0xff;
      a = (clearValue      ) & 0xff;
      break;
   case PIPE_FORMAT_A8R8G8B8_UNORM:
      r = (clearValue >> 16) & 0xff;
      g = (clearValue >>  8) & 0xff;
      b = (clearValue      ) & 0xff;
      a = (clearValue >> 24) & 0xff;
      break;
   case PIPE_FORMAT_B8G8R8A8_UNORM:
      r = (clearValue >>  8) & 0xff;
      g = (clearValue >> 16) & 0xff;
      b = (clearValue >> 24) & 0xff;
      a = (clearValue      ) & 0xff;
      break;
   default:
      r = g = b = a = 0;
   }

   tc->clear_color[0] = r / 255.0f;
   tc->clear_color[1] = g / 255.0f;
   tc->clear_color[2] = b / 255.0f;
   tc->clear_color[3] = a / 255.0f;

#if TILE_CLEAR_OPTIMIZATION
   /* set flags to indicate all the tiles are cleared */
   memset(tc->clear_flags, 255, sizeof(tc->clear_flags));
#else
   /* disable the optimization */
   memset(tc->clear_flags, 0, sizeof(tc->clear_flags));
#endif

   for (pos = 0; pos < NUM_ENTRIES; pos++) {
      struct softpipe_cached_tile *tile = tc->entries + pos;
      tile->x = tile->y = -1;
   }
}

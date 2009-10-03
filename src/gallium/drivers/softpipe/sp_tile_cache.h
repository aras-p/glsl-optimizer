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

#ifndef SP_TILE_CACHE_H
#define SP_TILE_CACHE_H

#define TILE_CLEAR_OPTIMIZATION 1


#include "pipe/p_compiler.h"


struct softpipe_tile_cache;


/**
 * Cache tile size (width and height). This needs to be a power of two.
 */
#define TILE_SIZE 64


/* If we need to support > 4096, just expand this to be a 64 bit
 * union, or consider tiling in Z as well.
 */
union tile_address {
   struct {
      unsigned x:6;             /* 4096 / TILE_SIZE */
      unsigned y:6;             /* 4096 / TILE_SIZE */
      unsigned invalid:1;
      unsigned pad:19;
   } bits;
   unsigned value;
};


struct softpipe_cached_tile
{
   union tile_address addr;
   union {
      float color[TILE_SIZE][TILE_SIZE][4];
      uint color32[TILE_SIZE][TILE_SIZE];
      uint depth32[TILE_SIZE][TILE_SIZE];
      ushort depth16[TILE_SIZE][TILE_SIZE];
      ubyte stencil8[TILE_SIZE][TILE_SIZE];
      ubyte any[1];
   } data;
};

#define NUM_ENTRIES 50


/** XXX move these */
#define MAX_WIDTH 4096
#define MAX_HEIGHT 4096


struct softpipe_tile_cache
{
   struct pipe_screen *screen;
   struct pipe_surface *surface;  /**< the surface we're caching */
   struct pipe_transfer *transfer;
   void *transfer_map;

   struct softpipe_cached_tile entries[NUM_ENTRIES];
   uint clear_flags[(MAX_WIDTH / TILE_SIZE) * (MAX_HEIGHT / TILE_SIZE) / 32];
   float clear_color[4];  /**< for color bufs */
   uint clear_val;        /**< for z+stencil, or packed color clear value */
   boolean depth_stencil; /**< Is the surface a depth/stencil format? */

   struct softpipe_cached_tile tile;  /**< scratch tile for clears */

   struct softpipe_cached_tile *last_tile;  /**< most recently retrieved tile */
};


extern struct softpipe_tile_cache *
sp_create_tile_cache( struct pipe_screen *screen );

extern void
sp_destroy_tile_cache(struct softpipe_tile_cache *tc);

extern void
sp_tile_cache_set_surface(struct softpipe_tile_cache *tc,
                          struct pipe_surface *sps);

extern struct pipe_surface *
sp_tile_cache_get_surface(struct softpipe_tile_cache *tc);

extern void
sp_tile_cache_map_transfers(struct softpipe_tile_cache *tc);

extern void
sp_tile_cache_unmap_transfers(struct softpipe_tile_cache *tc);

extern void
sp_flush_tile_cache(struct softpipe_tile_cache *tc);

extern void
sp_tile_cache_clear(struct softpipe_tile_cache *tc, const float *rgba,
                    uint clearValue);

extern struct softpipe_cached_tile *
sp_find_cached_tile(struct softpipe_tile_cache *tc, 
                    union tile_address addr );


static INLINE union tile_address
tile_address( unsigned x,
              unsigned y )
{
   union tile_address addr;

   addr.value = 0;
   addr.bits.x = x / TILE_SIZE;
   addr.bits.y = y / TILE_SIZE;
      
   return addr;
}

/* Quickly retrieve tile if it matches last lookup.
 */
static INLINE struct softpipe_cached_tile *
sp_get_cached_tile(struct softpipe_tile_cache *tc, 
                   int x, int y )
{
   union tile_address addr = tile_address( x, y );

   if (tc->last_tile->addr.value == addr.value)
      return tc->last_tile;

   return sp_find_cached_tile( tc, addr );
}




#endif /* SP_TILE_CACHE_H */


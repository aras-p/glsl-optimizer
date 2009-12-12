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

#ifndef LP_TILE_CACHE_H
#define LP_TILE_CACHE_H


#include "pipe/p_compiler.h"
#include "lp_tile_soa.h"


struct llvmpipe_tile_cache;  /* opaque */


extern struct llvmpipe_tile_cache *
lp_create_tile_cache( struct pipe_screen *screen );

extern void
lp_destroy_tile_cache(struct llvmpipe_tile_cache *tc);

extern void
lp_tile_cache_set_surface(struct llvmpipe_tile_cache *tc,
                          struct pipe_surface *lps);

extern struct pipe_surface *
lp_tile_cache_get_surface(struct llvmpipe_tile_cache *tc);

extern void
lp_tile_cache_map_transfers(struct llvmpipe_tile_cache *tc);

extern void
lp_tile_cache_unmap_transfers(struct llvmpipe_tile_cache *tc);

extern void
lp_flush_tile_cache(struct llvmpipe_tile_cache *tc);

extern void
lp_tile_cache_clear(struct llvmpipe_tile_cache *tc, const float *rgba,
                    uint clearValue);

extern void *
lp_get_cached_tile(struct llvmpipe_tile_cache *tc,
                   unsigned x, unsigned y );


#endif /* LP_TILE_CACHE_H */


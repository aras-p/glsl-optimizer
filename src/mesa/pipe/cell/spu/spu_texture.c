/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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


#include "pipe/p_compiler.h"
#include "spu_main.h"
#include "spu_texture.h"
#include "spu_tile.h"


/**
 * Number of texture tiles to cache.
 * Note that this will probably be the largest consumer of SPU local store/
 * memory for this driver!
 */
#define CACHE_SIZE 16

static tile_t tex_tiles[CACHE_SIZE]  ALIGN16_ATTRIB;

static int tex_tile_x[CACHE_SIZE], tex_tile_y[CACHE_SIZE];



/**
 * Mark all tex cache entries as invalid.
 */
void
invalidate_tex_cache(void)
{
   /* XXX memset? */
   uint i;
   for (i = 0; i < CACHE_SIZE; i++)
      tex_tile_x[i] = tex_tile_y[i] = -1;
}


/**
 * Return the cache pos/index which corresponds to texel (i,j)
 */
static INLINE uint
cache_pos(uint i, uint j)
{
   uint tx = i / TILE_SIZE;
   uint ty = j / TILE_SIZE;
   uint pos = (tx + ty * 4) % CACHE_SIZE;
   return pos;
}


/**
 * Make sure the tile for texel (i,j) is present, return its position/index
 * in the cache.
 */
static uint
get_tex_tile(uint i, uint j)
{
   const int tx = i / TILE_SIZE;
   const int ty = j / TILE_SIZE;
   const uint pos = cache_pos(i, j);

   if (tex_tile_x[pos] != tx || tex_tile_y[pos] != ty) {
      /* texture cache miss, fetch tile from main memory */
      const uint tiles_per_row = spu.texture.width / TILE_SIZE;
      const uint bytes_per_tile = sizeof(tile_t);
      const void *src = (const ubyte *) spu.texture.start
         + (ty * tiles_per_row + tx) * bytes_per_tile;

      printf("SPU %u: tex cache miss at %d, %d  pos=%u  old=%d,%d\n",
             spu.init.id, tx, ty, pos,
             tex_tile_x[pos], tex_tile_y[pos]);
#if 0
      printf("SPU %u: get tex tile from %p to %p\n",
             spu.init.id, src, tex_tiles[pos].t32);
#endif

      ASSERT_ALIGN16(tex_tiles[pos].t32);
      ASSERT_ALIGN16(src);

      mfc_get(tex_tiles[pos].t32,  /* dest */
              (unsigned int) src,
              bytes_per_tile,      /* size */
              TAG_TEXTURE_TILE,
              0, /* tid */
              0  /* rid */);

      wait_on_mask(1 << TAG_TEXTURE_TILE);

      tex_tile_x[pos] = tx;
      tex_tile_y[pos] = ty;
   }
   else {
#if 0
      printf("SPU %u: tex cache HIT at %d, %d\n",
             spu.init.id, tx, ty);
#endif
   }

   return pos;
}


/**
 * Get texture sample at texcoord.
 * XXX this is extremely primitive for now.
 */
uint
sample_texture(const float *texcoord)
{
   /* wrap/repeat */
   uint i = (uint) (texcoord[0] * spu.texture.width) % spu.texture.width;
   uint j = (uint) (texcoord[1] * spu.texture.height) % spu.texture.height;
   uint pos = get_tex_tile(i, j);
   uint texel = tex_tiles[pos].t32[j % TILE_SIZE][i % TILE_SIZE];
   return texel;
}

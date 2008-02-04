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


#include <vec_literal.h>

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

static vector unsigned int tex_tile_xy[CACHE_SIZE];



/**
 * Mark all tex cache entries as invalid.
 */
void
invalidate_tex_cache(void)
{
   /* XXX memset? */
   uint i;
   for (i = 0; i < CACHE_SIZE; i++) {
      tex_tile_xy[i] = VEC_LITERAL(vector unsigned int, ~0U, ~0U, ~0U, ~0U);
   }
}


/**
 * Return the cache pos/index which corresponds to tile (tx,ty)
 */
static INLINE uint
cache_pos(vector unsigned int txty)
{
   uint pos = (spu_extract(txty,0) + spu_extract(txty,1) * 4) % CACHE_SIZE;
   return pos;
}


/**
 * Make sure the tile for texel (i,j) is present, return its position/index
 * in the cache.
 */
static uint
get_tex_tile(vector unsigned int ij)
{
   /* tile address: tx,ty */
   const vector unsigned int txty = spu_rlmask(ij, -5);  /* divide by 32 */
   const uint pos = cache_pos(txty);

   if ((spu_extract(tex_tile_xy[pos], 0) != spu_extract(txty, 0)) ||
       (spu_extract(tex_tile_xy[pos], 1) != spu_extract(txty, 1))) {

      /* texture cache miss, fetch tile from main memory */
      const uint tiles_per_row = spu.texture.width / TILE_SIZE;
      const uint bytes_per_tile = sizeof(tile_t);
      const void *src = (const ubyte *) spu.texture.start
         + (spu_extract(txty,1) * tiles_per_row + spu_extract(txty,0)) * bytes_per_tile;

      printf("SPU %u: tex cache miss at %d, %d  pos=%u  old=%d,%d\n",
             spu.init.id,
             spu_extract(txty,0),
             spu_extract(txty,1),
             pos,
             spu_extract(tex_tile_xy[pos],0),
             spu_extract(tex_tile_xy[pos],1));

      ASSERT_ALIGN16(tex_tiles[pos].ui);
      ASSERT_ALIGN16(src);

      mfc_get(tex_tiles[pos].ui,  /* dest */
              (unsigned int) src,
              bytes_per_tile,      /* size */
              TAG_TEXTURE_TILE,
              0, /* tid */
              0  /* rid */);

      wait_on_mask(1 << TAG_TEXTURE_TILE);

      tex_tile_xy[pos] = txty;
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
sample_texture(vector float texcoord)
{
   vector float tc = spu_mul(texcoord, spu.tex_size);
   vector unsigned int itc = spu_convtu(tc, 0);  /* convert to int */
   itc = spu_and(itc, spu.tex_size_mask);        /* mask (GL_REPEAT) */
   vector unsigned int ij = spu_and(itc, TILE_SIZE-1); /* intra tile addr */
   uint pos = get_tex_tile(itc);
   uint texel = tex_tiles[pos].ui[spu_extract(ij, 1)][spu_extract(ij, 0)];
   return texel;
}

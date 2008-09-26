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



#include "spu_tile.h"
#include "spu_main.h"


/**
 * Get tile of color or Z values from main memory, put into SPU memory.
 */
void
get_tile(uint tx, uint ty, tile_t *tile, int tag, int zBuf)
{
   const uint offset = ty * spu.fb.width_tiles + tx;
   const uint bytesPerTile = TILE_SIZE * TILE_SIZE * (zBuf ? spu.fb.zsize : 4);
   const ubyte *src = zBuf ? spu.fb.depth_start : spu.fb.color_start;

   src += offset * bytesPerTile;

   ASSERT(tx < spu.fb.width_tiles);
   ASSERT(ty < spu.fb.height_tiles);
   ASSERT_ALIGN16(tile);
   /*
   printf("get_tile:  dest: %p  src: 0x%x  size: %d\n",
          tile, (unsigned int) src, bytesPerTile);
   */
   mfc_get(tile->ui,  /* dest in local memory */
           (unsigned int) src, /* src in main memory */
           bytesPerTile,
           tag,
           0, /* tid */
           0  /* rid */);
}


/**
 * Move tile of color or Z values from SPU memory to main memory.
 */
void
put_tile(uint tx, uint ty, const tile_t *tile, int tag, int zBuf)
{
   const uint offset = ty * spu.fb.width_tiles + tx;
   const uint bytesPerTile = TILE_SIZE * TILE_SIZE * (zBuf ? spu.fb.zsize : 4);
   ubyte *dst = zBuf ? spu.fb.depth_start : spu.fb.color_start;

   dst += offset * bytesPerTile;

   ASSERT(tx < spu.fb.width_tiles);
   ASSERT(ty < spu.fb.height_tiles);
   ASSERT_ALIGN16(tile);
   /*
   printf("SPU %u: put_tile:  src: %p  dst: 0x%x  size: %d\n",
          spu.init.id,
          tile, (unsigned int) dst, bytesPerTile);
   */
   mfc_put((void *) tile->ui,  /* src in local memory */
           (unsigned int) dst,  /* dst in main memory */
           bytesPerTile,
           tag,
           0, /* tid */
           0  /* rid */);
}


/**
 * For tiles whose status is TILE_STATUS_CLEAR, write solid-filled
 * tiles back to the main framebuffer.
 */
void
really_clear_tiles(uint surfaceIndex)
{
   const uint num_tiles = spu.fb.width_tiles * spu.fb.height_tiles;
   uint i;

   if (surfaceIndex == 0) {
      clear_c_tile(&spu.ctile);

      for (i = spu.init.id; i < num_tiles; i += spu.init.num_spus) {
         uint tx = i % spu.fb.width_tiles;
         uint ty = i / spu.fb.width_tiles;
         if (spu.ctile_status[ty][tx] == TILE_STATUS_CLEAR) {
            put_tile(tx, ty, &spu.ctile, TAG_SURFACE_CLEAR, 0);
         }
      }
   }
   else {
      clear_z_tile(&spu.ztile);

      for (i = spu.init.id; i < num_tiles; i += spu.init.num_spus) {
         uint tx = i % spu.fb.width_tiles;
         uint ty = i / spu.fb.width_tiles;
         if (spu.ztile_status[ty][tx] == TILE_STATUS_CLEAR)
            put_tile(tx, ty, &spu.ctile, TAG_SURFACE_CLEAR, 1);
      }
   }

#if 0
   wait_on_mask(1 << TAG_SURFACE_CLEAR);
#endif
}

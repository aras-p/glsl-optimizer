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



tile_t ctile ALIGN16_ATTRIB;
tile_t ztile ALIGN16_ATTRIB;

ubyte tile_status[MAX_HEIGHT/TILE_SIZE][MAX_WIDTH/TILE_SIZE] ALIGN16_ATTRIB;
ubyte tile_status_z[MAX_HEIGHT/TILE_SIZE][MAX_WIDTH/TILE_SIZE] ALIGN16_ATTRIB;

ubyte cur_tile_status_c, cur_tile_status_z;


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


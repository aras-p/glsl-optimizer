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

#ifndef SPU_TILE_H
#define SPU_TILE_H


#include <libmisc.h>
#include <spu_mfcio.h>
#include "spu_main.h"
#include "cell/common.h"



extern void
get_tile(uint tx, uint ty, tile_t *tile, int tag, int zBuf);

extern void
put_tile(uint tx, uint ty, const tile_t *tile, int tag, int zBuf);

extern void
really_clear_tiles(uint surfaceIndex);


static INLINE void
clear_c_tile(tile_t *ctile)
{
   memset32((uint*) ctile->ui,
            spu.fb.color_clear_value,
            TILE_SIZE * TILE_SIZE);
}


static INLINE void
clear_z_tile(tile_t *ztile)
{
   if (spu.fb.zsize == 2) {
      memset16((ushort*) ztile->us,
               spu.fb.depth_clear_value,
               TILE_SIZE * TILE_SIZE);
   }
   else {
      ASSERT(spu.fb.zsize != 0);
      memset32((uint*) ztile->ui,
               spu.fb.depth_clear_value,
               TILE_SIZE * TILE_SIZE);
   }
}


#endif /* SPU_TILE_H */

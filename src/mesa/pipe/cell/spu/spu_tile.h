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
#include "pipe/cell/common.h"


#define MAX_WIDTH 1024
#define MAX_HEIGHT 1024


typedef union {
   ushort t16[TILE_SIZE][TILE_SIZE];
   uint   t32[TILE_SIZE][TILE_SIZE];
   vector unsigned short us8[TILE_SIZE/2][TILE_SIZE/4];
   vector unsigned int ui4[TILE_SIZE/2][TILE_SIZE/2];
} tile_t;


extern tile_t ctile ALIGN16_ATTRIB;
extern tile_t ztile ALIGN16_ATTRIB;


#define TILE_STATUS_CLEAR   1
#define TILE_STATUS_DEFINED 2  /**< defined in FB, but not in local store */
#define TILE_STATUS_CLEAN   3  /**< in local store, but not changed */
#define TILE_STATUS_DIRTY   4  /**< modified locally, but not put back yet */
#define TILE_STATUS_GETTING 5  /**< mfc_get() called but not yet arrived */

extern ubyte tile_status[MAX_HEIGHT/TILE_SIZE][MAX_WIDTH/TILE_SIZE] ALIGN16_ATTRIB;
extern ubyte tile_status_z[MAX_HEIGHT/TILE_SIZE][MAX_WIDTH/TILE_SIZE] ALIGN16_ATTRIB;

extern ubyte cur_tile_status_c, cur_tile_status_z;


void
get_tile(uint tx, uint ty, tile_t *tile, int tag, int zBuf);

void
put_tile(uint tx, uint ty, const tile_t *tile, int tag, int zBuf);



static INLINE void
clear_c_tile(tile_t *ctile)
{
   memset32((uint*) ctile->t32,
            spu.fb.color_clear_value,
            TILE_SIZE * TILE_SIZE);
}


static INLINE void
clear_z_tile(tile_t *ztile)
{
   if (spu.fb.depth_format == PIPE_FORMAT_Z16_UNORM) {
      memset16((ushort*) ztile->t16,
               spu.fb.depth_clear_value,
               TILE_SIZE * TILE_SIZE);
   }
   else {
      ASSERT(spu.fb.depth_format == PIPE_FORMAT_Z32_UNORM);
#if SIMD_Z
      union fi z;
      z.f = 1.0;
      memset32((uint*) ztile->t32,
               z.i,/*spu.fb.depth_clear_value,*/
               TILE_SIZE * TILE_SIZE);
#else
      memset32((uint*) ztile->t32,
               spu.fb.depth_clear_value,
               TILE_SIZE * TILE_SIZE);
#endif
   }
}


#endif /* SPU_TILE_H */

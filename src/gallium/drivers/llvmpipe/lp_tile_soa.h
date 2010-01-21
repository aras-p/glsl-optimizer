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

#ifndef LP_TILE_SOA_H
#define LP_TILE_SOA_H

#include "pipe/p_compiler.h"
#include "tgsi/tgsi_exec.h" /* for NUM_CHANNELS */
#include "lp_tile_size.h"

#ifdef __cplusplus
extern "C" {
#endif


struct pipe_transfer;


#define TILE_VECTOR_HEIGHT 4
#define TILE_VECTOR_WIDTH 4

extern const unsigned char
tile_offset[TILE_VECTOR_HEIGHT][TILE_VECTOR_WIDTH];

#define TILE_C_STRIDE (TILE_VECTOR_HEIGHT * TILE_VECTOR_WIDTH) //16
#define TILE_X_STRIDE (NUM_CHANNELS * TILE_C_STRIDE) //64
#define TILE_Y_STRIDE (TILE_VECTOR_HEIGHT * TILE_SIZE * NUM_CHANNELS) //1024

#define TILE_PIXEL(_p, _x, _y, _c) \
   ((_p)[((_y) / TILE_VECTOR_HEIGHT) * TILE_Y_STRIDE + \
         ((_x) / TILE_VECTOR_WIDTH) * TILE_X_STRIDE + \
         (_c) * TILE_C_STRIDE + \
         tile_offset[(_y) % TILE_VECTOR_HEIGHT][(_x) % TILE_VECTOR_WIDTH]])


void
lp_tile_read_4ub(enum pipe_format format,
                 uint8_t *dst,
                 const void *src, unsigned src_stride,
                 unsigned x, unsigned y, unsigned w, unsigned h);


void
lp_tile_write_4ub(enum pipe_format format,
                  const uint8_t *src,
                  void *dst, unsigned dst_stride,
                  unsigned x, unsigned y, unsigned w, unsigned h);



#ifdef __cplusplus
}
#endif

#endif

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

#ifndef MAIN_H
#define MAIN_H


#include <libmisc.h>
#include <spu_mfcio.h>
#include "pipe/cell/common.h"


#define MAX_WIDTH 1024
#define MAX_HEIGHT 1024


extern volatile struct cell_init_info init;

struct framebuffer {
   void *color_start;              /**< addr of color surface in main memory */
   void *depth_start;              /**< addr of depth surface in main memory */
   enum pipe_format color_format;
   enum pipe_format depth_format;
   uint width, height;             /**< size in pixels */
   uint width_tiles, height_tiles; /**< width and height in tiles */

   uint color_clear_value;
   uint depth_clear_value;
};

/* XXX Collect these globals in a struct: */

extern struct framebuffer fb;

extern uint ctile[TILE_SIZE][TILE_SIZE] ALIGN16_ATTRIB;
extern ushort ztile[TILE_SIZE][TILE_SIZE] ALIGN16_ATTRIB;


/* DMA TAGS */

#define TAG_SURFACE_CLEAR     10
#define TAG_VERTEX_BUFFER     11
#define TAG_INDEX_BUFFER      16
#define TAG_READ_TILE_COLOR   12
#define TAG_READ_TILE_Z       13
#define TAG_WRITE_TILE_COLOR  14
#define TAG_WRITE_TILE_Z      15



#define TILE_STATUS_CLEAR   1
#define TILE_STATUS_DEFINED 2  /**< defined pixel data */
#define TILE_STATUS_DIRTY   3  /**< modified, but not put back yet */

extern ubyte tile_status[MAX_HEIGHT/TILE_SIZE][MAX_WIDTH/TILE_SIZE] ALIGN16_ATTRIB;
extern ubyte tile_status_z[MAX_HEIGHT/TILE_SIZE][MAX_WIDTH/TILE_SIZE] ALIGN16_ATTRIB;


void
wait_on_mask(unsigned tag);

void
get_tile(const struct framebuffer *fb, uint tx, uint ty, uint *tile,
          int tag, int zBuf);

void
put_tile(const struct framebuffer *fb, uint tx, uint ty, const uint *tile,
         int tag, int zBuf);

void
clear_tile(uint tile[TILE_SIZE][TILE_SIZE], uint value);

void
clear_tile_z(ushort tile[TILE_SIZE][TILE_SIZE], uint value);


#endif /* MAIN_H */

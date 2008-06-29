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

#include "pipe/p_defines.h"
#include "pipe/p_util.h"
#include "pipe/p_inlines.h"
#include "pipe/p_winsys.h"
#include "util/p_tile.h"
#include "cell_context.h"
#include "cell_surface.h"


static void
cell_surface_copy(struct pipe_context *pipe,
                  boolean do_flip,
                  struct pipe_surface *dst,
                  unsigned dstx, unsigned dsty,
                  struct pipe_surface *src,
                  unsigned srcx, unsigned srcy,
                  unsigned width, unsigned height)
{
   assert( dst->cpp == src->cpp );

   pipe_copy_rect(pipe_surface_map(dst),
                  &dst->block,
                  dst->stride,
                  dstx, dsty,
                  width, height,
                  pipe_surface_map(src),
                  do_flip ? -src->stride : src->stride,
                  srcx, do_flip ? height - 1 - srcy : srcy);

   pipe_surface_unmap(src);
   pipe_surface_unmap(dst);
}


static void *
get_pointer(struct pipe_surface *dst, void *dst_map, unsigned x, unsigned y)
{
   return (char *)dst_map + y / dst->block.height * dst->stride + x / dst->block.width * dst->block.size;
}


#define UBYTE_TO_USHORT(B) ((B) | ((B) << 8))


/**
 * Fill a rectangular sub-region.  Need better logic about when to
 * push buffers into AGP - will currently do so whenever possible.
 */
static void
cell_surface_fill(struct pipe_context *pipe,
                  struct pipe_surface *dst,
                  unsigned dstx, unsigned dsty,
                  unsigned width, unsigned height, unsigned value)
{
   unsigned i, j;
   void *dst_map = pipe_surface_map(dst);

   assert(dst->stride > 0);

   switch (dst->block.size) {
   case 1:
   case 2:
   case 4:
      pipe_fill_rect(dst_map, &dst->block, dst->stride, dstx, dsty, width, height, value);
      break;
   case 8:
      {
         /* expand the 4-byte clear value to an 8-byte value */
         ushort *row = (ushort *) get_pointer(dst, dst_map, dstx, dsty);
         ushort val0 = UBYTE_TO_USHORT((value >>  0) & 0xff);
         ushort val1 = UBYTE_TO_USHORT((value >>  8) & 0xff);
         ushort val2 = UBYTE_TO_USHORT((value >> 16) & 0xff);
         ushort val3 = UBYTE_TO_USHORT((value >> 24) & 0xff);
         val0 = (val0 << 8) | val0;
         val1 = (val1 << 8) | val1;
         val2 = (val2 << 8) | val2;
         val3 = (val3 << 8) | val3;
         for (i = 0; i < height; i++) {
            for (j = 0; j < width; j++) {
               row[j*4+0] = val0;
               row[j*4+1] = val1;
               row[j*4+2] = val2;
               row[j*4+3] = val3;
            }
            row += dst->stride/2;
         }
      }
      break;
   default:
      assert(0);
      break;
   }

   pipe_surface_unmap( dst );
}


void
cell_init_surface_functions(struct cell_context *cell)
{
   cell->pipe.surface_copy = cell_surface_copy;
   cell->pipe.surface_fill = cell_surface_fill;
}

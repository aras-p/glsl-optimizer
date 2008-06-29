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

/**
 * Miscellaneous utility functions.
 */


#include "pipe/p_defines.h"
#include "pipe/p_util.h"
#include "pipe/p_format.h"


/**
 * Copy 2D rect from one place to another.
 * Position and sizes are in pixels.
 * src_pitch may be negative to do vertical flip of pixels from source.
 */
void
pipe_copy_rect(ubyte * dst,
               const struct pipe_format_block *block,
               unsigned dst_stride,
               unsigned dst_x,
               unsigned dst_y,
               unsigned width,
               unsigned height,
               const ubyte * src,
               int src_stride,
               unsigned src_x, 
               int src_y)
{
   unsigned i;
   int src_stride_pos = src_stride < 0 ? -src_stride : src_stride;

   assert(block->size > 0);
   assert(block->width > 0);
   assert(block->height > 0);
   assert(src_x >= 0);
   assert(src_y >= 0);
   assert(dst_x >= 0);
   assert(dst_y >= 0);

   dst_x /= block->width;
   dst_y /= block->height;
   width = (width + block->width - 1)/block->width;
   height = (height + block->height - 1)/block->height;
   src_x /= block->width;
   src_y /= block->height;
   
   dst += dst_x * block->size;
   src += src_x * block->size;
   dst += dst_y * dst_stride;
   src += src_y * src_stride_pos;
   width *= block->size;

   if (width == dst_stride && width == src_stride)
      memcpy(dst, src, height * width);
   else {
      for (i = 0; i < height; i++) {
         memcpy(dst, src, width);
         dst += dst_stride;
         src += src_stride;
      }
   }
}

void
pipe_fill_rect(ubyte * dst,
               const struct pipe_format_block *block,
               unsigned dst_stride,
               unsigned dst_x,
               unsigned dst_y,
               unsigned width,
               unsigned height,
               uint32_t value)
{
   unsigned i, j;
   unsigned width_size;

   assert(block->size > 0);
   assert(block->width > 0);
   assert(block->height > 0);
   assert(dst_x >= 0);
   assert(dst_y >= 0);

   dst_x /= block->width;
   dst_y /= block->height;
   width = (width + block->width - 1)/block->width;
   height = (height + block->height - 1)/block->height;
   
   dst += dst_x * block->size;
   dst += dst_y * dst_stride;
   width_size = width * block->size;
   
   switch (block->size) {
   case 1:
      if(dst_stride == width_size)
	 memset(dst, (ubyte) value, height * width_size);
      else {
	 for (i = 0; i < height; i++) {
	    memset(dst, (ubyte) value, width_size);
	    dst += dst_stride;
	 }
      }
      break;
   case 2:
      for (i = 0; i < height; i++) {
	 uint16_t *row = (uint16_t *)dst;
	 for (j = 0; j < width; j++)
	    *row++ = (uint16_t) value;
	 dst += dst_stride;
      }
      break;
   case 4:
      for (i = 0; i < height; i++) {
	 uint32_t *row = (uint32_t *)dst;
	 for (j = 0; j < width; j++)
	    *row++ = value;
	 dst += dst_stride;
      }
      break;
   default:
	 assert(0);
	 break;
   }
}

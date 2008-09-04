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
 * Rectangle-related helper functions.
 */


#include "pipe/p_defines.h"
#include "pipe/p_format.h"
#include "pipe/p_context.h"
#include "pipe/p_screen.h"
#include "util/u_rect.h"


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



/**
 * Fallback function for pipe->surface_copy().
 * Note: (X,Y)=(0,0) is always the upper-left corner.
 * if do_flip, flip the image vertically on its way from src rect to dst rect.
 * XXX should probably put this in new u_surface.c file...
 */
void
util_surface_copy(struct pipe_context *pipe,
                  boolean do_flip,
                  struct pipe_surface *dst,
                  unsigned dst_x, unsigned dst_y,
                  struct pipe_surface *src,
                  unsigned src_x, unsigned src_y, 
                  unsigned w, unsigned h)
{
   struct pipe_screen *screen = pipe->screen;
   struct pipe_surface *new_src = NULL, *new_dst = NULL;
   void *dst_map;
   const void *src_map;

   assert(dst->block.size == src->block.size);
   assert(dst->block.width == src->block.width);
   assert(dst->block.height == src->block.height);

   if ((src->usage & PIPE_BUFFER_USAGE_CPU_READ) == 0) {
      /* Need to create new src surface which is CPU readable */
      assert(src->texture);
      if (!src->texture)
         return;
      new_src = screen->get_tex_surface(screen,
                                        src->texture,
                                        src->face,
                                        src->level,
                                        src->zslice,
                                        PIPE_BUFFER_USAGE_CPU_READ);
      src = new_src;
   }

   if ((dst->usage & PIPE_BUFFER_USAGE_CPU_WRITE) == 0) {
      /* Need to create new dst surface which is CPU writable */
      assert(dst->texture);
      if (!dst->texture)
         return;
      new_dst = screen->get_tex_surface(screen,
                                        dst->texture,
                                        dst->face,
                                        dst->level,
                                        dst->zslice,
                                        PIPE_BUFFER_USAGE_CPU_WRITE);
      dst = new_dst;
   }

   src_map = pipe->screen->surface_map(screen,
                                       src, PIPE_BUFFER_USAGE_CPU_READ);
   dst_map = pipe->screen->surface_map(screen,
                                       dst, PIPE_BUFFER_USAGE_CPU_WRITE);

   assert(src_map);
   assert(dst_map);

   if (src_map && dst_map) {
      /* If do_flip, invert src_y position and pass negative src stride */
      pipe_copy_rect(dst_map,
                     &dst->block,
                     dst->stride,
                     dst_x, dst_y,
                     w, h,
                     src_map,
                     do_flip ? -(int) src->stride : src->stride,
                     src_x, src_y);
   }

   pipe->screen->surface_unmap(pipe->screen, src);
   pipe->screen->surface_unmap(pipe->screen, dst);

   if (new_src)
      screen->tex_surface_release(screen, &new_src);
   if (new_dst)
      screen->tex_surface_release(screen, &new_dst);
}



static void *
get_pointer(struct pipe_surface *dst, void *dst_map, unsigned x, unsigned y)
{
   return (char *)dst_map
      + y / dst->block.height * dst->stride
      + x / dst->block.width * dst->block.size;
}


#define UBYTE_TO_USHORT(B) ((B) | ((B) << 8))


/**
 * Fallback for pipe->surface_fill() function.
 * XXX should probably put this in new u_surface.c file...
 */
void
util_surface_fill(struct pipe_context *pipe,
                  struct pipe_surface *dst,
                  unsigned dstx, unsigned dsty,
                  unsigned width, unsigned height, unsigned value)
{
   struct pipe_screen *screen = pipe->screen;
   struct pipe_surface *new_dst = NULL;
   void *dst_map;

   if ((dst->usage & PIPE_BUFFER_USAGE_CPU_WRITE) == 0) {
      /* Need to create new dst surface which is CPU writable */
      assert(dst->texture);
      if (!dst->texture)
         return;
      new_dst = screen->get_tex_surface(screen,
                                        dst->texture,
                                        dst->face,
                                        dst->level,
                                        dst->zslice,
                                        PIPE_BUFFER_USAGE_CPU_WRITE);
      dst = new_dst;
   }

   dst_map = pipe->screen->surface_map(screen,
                                       dst, PIPE_BUFFER_USAGE_CPU_WRITE);

   assert(dst_map);

   if (dst_map) {
      assert(dst->stride > 0);

      switch (dst->block.size) {
      case 1:
      case 2:
      case 4:
         pipe_fill_rect(dst_map, &dst->block, dst->stride,
                        dstx, dsty, width, height, value);
         break;
      case 8:
         {
            /* expand the 4-byte clear value to an 8-byte value */
            ushort *row = (ushort *) get_pointer(dst, dst_map, dstx, dsty);
            ushort val0 = UBYTE_TO_USHORT((value >>  0) & 0xff);
            ushort val1 = UBYTE_TO_USHORT((value >>  8) & 0xff);
            ushort val2 = UBYTE_TO_USHORT((value >> 16) & 0xff);
            ushort val3 = UBYTE_TO_USHORT((value >> 24) & 0xff);
            unsigned i, j;
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
   }

   pipe->screen->surface_unmap(pipe->screen, dst);

   if (new_dst)
      screen->tex_surface_release(screen, &new_dst);
}

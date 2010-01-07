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
#include "util/u_format.h"
#include "util/u_rect.h"


/**
 * Copy 2D rect from one place to another.
 * Position and sizes are in pixels.
 * src_stride may be negative to do vertical flip of pixels from source.
 */
void
util_copy_rect(ubyte * dst,
               enum pipe_format format,
               unsigned dst_stride,
               unsigned dst_x,
               unsigned dst_y,
               unsigned width,
               unsigned height,
               const ubyte * src,
               int src_stride,
               unsigned src_x, 
               unsigned src_y)
{
   unsigned i;
   int src_stride_pos = src_stride < 0 ? -src_stride : src_stride;
   int blocksize = util_format_get_blocksize(format);
   int blockwidth = util_format_get_blockwidth(format);
   int blockheight = util_format_get_blockheight(format);

   assert(blocksize > 0);
   assert(blockwidth > 0);
   assert(blockheight > 0);

   dst_x /= blockwidth;
   dst_y /= blockheight;
   width = (width + blockwidth - 1)/blockwidth;
   height = (height + blockheight - 1)/blockheight;
   src_x /= blockwidth;
   src_y /= blockheight;
   
   dst += dst_x * blocksize;
   src += src_x * blocksize;
   dst += dst_y * dst_stride;
   src += src_y * src_stride_pos;
   width *= blocksize;

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
util_fill_rect(ubyte * dst,
               enum pipe_format format,
               unsigned dst_stride,
               unsigned dst_x,
               unsigned dst_y,
               unsigned width,
               unsigned height,
               uint32_t value)
{
   unsigned i, j;
   unsigned width_size;
   int blocksize = util_format_get_blocksize(format);
   int blockwidth = util_format_get_blockwidth(format);
   int blockheight = util_format_get_blockheight(format);

   assert(blocksize > 0);
   assert(blockwidth > 0);
   assert(blockheight > 0);

   dst_x /= blockwidth;
   dst_y /= blockheight;
   width = (width + blockwidth - 1)/blockwidth;
   height = (height + blockheight - 1)/blockheight;
   
   dst += dst_x * blocksize;
   dst += dst_y * dst_stride;
   width_size = width * blocksize;
   
   switch (blocksize) {
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
   struct pipe_transfer *src_trans, *dst_trans;
   void *dst_map;
   const void *src_map;
   enum pipe_format src_format, dst_format;

   assert(src->texture && dst->texture);
   if (!src->texture || !dst->texture)
      return;

   src_format = src->texture->format;
   dst_format = dst->texture->format;

   src_trans = screen->get_tex_transfer(screen,
                                        src->texture,
                                        src->face,
                                        src->level,
                                        src->zslice,
                                        PIPE_TRANSFER_READ,
                                        src_x, src_y, w, h);

   dst_trans = screen->get_tex_transfer(screen,
                                        dst->texture,
                                        dst->face,
                                        dst->level,
                                        dst->zslice,
                                        PIPE_TRANSFER_WRITE,
                                        dst_x, dst_y, w, h);

   assert(util_format_get_blocksize(dst_format) == util_format_get_blocksize(src_format));
   assert(util_format_get_blockwidth(dst_format) == util_format_get_blockwidth(src_format));
   assert(util_format_get_blockheight(dst_format) == util_format_get_blockheight(src_format));

   src_map = pipe->screen->transfer_map(screen, src_trans);
   dst_map = pipe->screen->transfer_map(screen, dst_trans);

   assert(src_map);
   assert(dst_map);

   if (src_map && dst_map) {
      /* If do_flip, invert src_y position and pass negative src stride */
      util_copy_rect(dst_map,
                     dst_format,
                     dst_trans->stride,
                     0, 0,
                     w, h,
                     src_map,
                     do_flip ? -(int) src_trans->stride : src_trans->stride,
                     0,
                     do_flip ? h - 1 : 0);
   }

   pipe->screen->transfer_unmap(pipe->screen, src_trans);
   pipe->screen->transfer_unmap(pipe->screen, dst_trans);

   screen->tex_transfer_destroy(src_trans);
   screen->tex_transfer_destroy(dst_trans);
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
   struct pipe_transfer *dst_trans;
   void *dst_map;

   assert(dst->texture);
   if (!dst->texture)
      return;
   dst_trans = screen->get_tex_transfer(screen,
                                        dst->texture,
                                        dst->face,
                                        dst->level,
                                        dst->zslice,
                                        PIPE_TRANSFER_WRITE,
                                        dstx, dsty, width, height);

   dst_map = pipe->screen->transfer_map(screen, dst_trans);

   assert(dst_map);

   if (dst_map) {
      assert(dst_trans->stride > 0);

      switch (util_format_get_blocksize(dst_trans->texture->format)) {
      case 1:
      case 2:
      case 4:
         util_fill_rect(dst_map, dst_trans->texture->format, dst_trans->stride,
                        0, 0, width, height, value);
         break;
      case 8:
         {
            /* expand the 4-byte clear value to an 8-byte value */
            ushort *row = (ushort *) dst_map;
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
               row += dst_trans->stride/2;
            }
         }
         break;
      default:
         assert(0);
         break;
      }
   }

   pipe->screen->transfer_unmap(pipe->screen, dst_trans);
   screen->tex_transfer_destroy(dst_trans);
}

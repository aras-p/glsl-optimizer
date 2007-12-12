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
#include "sp_context.h"
#include "sp_surface.h"
#include "sp_texture.h"
#include "sp_rgba_tile.h"



#define CLIP_TILE \
   do { \
      if (x >= ps->width) \
         return; \
      if (y >= ps->height) \
         return; \
      if (x + w > ps->width) \
         w = ps->width - x; \
      if (y + h > ps->height) \
         h = ps->height - y; \
   } while(0)


/**
 * Called via pipe->get_tex_surface()
 * XXX is this in the right place?
 */
struct pipe_surface *
softpipe_get_tex_surface(struct pipe_context *pipe,
                         struct pipe_texture *pt,
                         unsigned face, unsigned level, unsigned zslice)
{
   struct softpipe_texture *spt = softpipe_texture(pt);
   struct pipe_surface *ps;
   unsigned offset;  /* in bytes */

   offset = spt->level_offset[level];

   if (pt->target == PIPE_TEXTURE_CUBE) {
      offset += spt->image_offset[level][face] * pt->cpp;
   }
   else if (pt->target == PIPE_TEXTURE_3D) {
      offset += spt->image_offset[level][zslice] * pt->cpp;
   }
   else {
      assert(face == 0);
      assert(zslice == 0);
   }

   ps = pipe->winsys->surface_alloc(pipe->winsys);
   if (ps) {
      assert(ps->refcount);
      pipe->winsys->buffer_reference(pipe->winsys, &ps->buffer, spt->buffer);
      ps->format = pt->format;
      ps->cpp = pt->cpp;
      ps->width = pt->width[level];
      ps->height = pt->height[level];
      ps->pitch = spt->pitch;
      ps->offset = offset;
   }
   return ps;
}


/**
 * Move raw block of pixels from surface to user memory.
 */
void
softpipe_get_tile(struct pipe_context *pipe, struct pipe_surface *ps,
                  uint x, uint y, uint w, uint h,
                  void *p, int dst_stride)
{
   const uint cpp = ps->cpp;
   const ubyte *pSrc;
   const uint src_stride = ps->pitch * cpp;
   ubyte *pDest;
   uint i;

   assert(ps->map);

   if (dst_stride == 0) {
      dst_stride = w * cpp;
   }

   CLIP_TILE;

   pSrc = ps->map + (y * ps->pitch + x) * cpp;
   pDest = (ubyte *) p;

   for (i = 0; i < h; i++) {
      memcpy(pDest, pSrc, w * cpp);
      pDest += dst_stride;
      pSrc += src_stride;
   }
}


/**
 * Move raw block of pixels from user memory to surface.
 */
void
softpipe_put_tile(struct pipe_context *pipe, struct pipe_surface *ps,
                  uint x, uint y, uint w, uint h,
                  const void *p, int src_stride)
{
   const uint cpp = ps->cpp;
   const ubyte *pSrc;
   const uint dst_stride = ps->pitch * cpp;
   ubyte *pDest;
   uint i;

   assert(ps->map);

   if (src_stride == 0) {
      src_stride = w * cpp;
   }

   CLIP_TILE;

   pSrc = (const ubyte *) p;
   pDest = ps->map + (y * ps->pitch + x) * cpp;

   for (i = 0; i < h; i++) {
      memcpy(pDest, pSrc, w * cpp);
      pDest += dst_stride;
      pSrc += src_stride;
   }
}


/**
 * Copy 2D rect from one place to another.
 * Position and sizes are in pixels.
 */
static void
copy_rect(ubyte * dst,
          unsigned cpp,
          unsigned dst_pitch,
          unsigned dst_x,
          unsigned dst_y,
          unsigned width,
          unsigned height,
          const ubyte * src,
          unsigned src_pitch,
          unsigned src_x, 
          unsigned src_y)
{
   unsigned i;

   dst_pitch *= cpp;
   src_pitch *= cpp;
   dst += dst_x * cpp;
   src += src_x * cpp;
   dst += dst_y * dst_pitch;
   src += src_y * src_pitch;
   width *= cpp;

   if (width == dst_pitch && width == src_pitch)
      memcpy(dst, src, height * width);
   else {
      for (i = 0; i < height; i++) {
         memcpy(dst, src, width);
         dst += dst_pitch;
         src += src_pitch;
      }
   }
}


/* Upload data to a rectangular sub-region.  Lots of choices how to do this:
 *
 * - memcpy by span to current destination
 * - upload data as new buffer and blit
 *
 * Currently always memcpy.
 */
static void
sp_surface_data(struct pipe_context *pipe,
		struct pipe_surface *dst,
		unsigned dstx, unsigned dsty,
		const void *src, unsigned src_pitch,
		unsigned srcx, unsigned srcy, unsigned width, unsigned height)
{
   copy_rect(pipe_surface_map(dst),
             dst->cpp,
             dst->pitch,
             dstx, dsty, width, height, src, src_pitch, srcx, srcy);

   pipe_surface_unmap(dst);
}

/* Assumes all values are within bounds -- no checking at this level -
 * do it higher up if required.
 */
static void
sp_surface_copy(struct pipe_context *pipe,
		struct pipe_surface *dst,
		unsigned dstx, unsigned dsty,
		struct pipe_surface *src,
		unsigned srcx, unsigned srcy, unsigned width, unsigned height)
{
   assert( dst->cpp == src->cpp );

   copy_rect(pipe_surface_map(dst),
             dst->cpp,
             dst->pitch,
             dstx, dsty,
             width, height,
             pipe_surface_map(src),
             src->pitch,
             srcx, srcy);

   pipe_surface_unmap(src);
   pipe_surface_unmap(dst);
}


static ubyte *
get_pointer(struct pipe_surface *dst, unsigned x, unsigned y)
{
   return dst->map + (y * dst->pitch + x) * dst->cpp;
}


#define UBYTE_TO_USHORT(B) ((B) | ((B) << 8))


/**
 * Fill a rectangular sub-region.  Need better logic about when to
 * push buffers into AGP - will currently do so whenever possible.
 */
static void
sp_surface_fill(struct pipe_context *pipe,
		struct pipe_surface *dst,
		unsigned dstx, unsigned dsty,
		unsigned width, unsigned height, unsigned value)
{
   unsigned i, j;

   assert(dst->pitch > 0);
   assert(width <= dst->pitch);

   (void)pipe_surface_map(dst);

   switch (dst->cpp) {
   case 1:
      {
         ubyte *row = get_pointer(dst, dstx, dsty);
         for (i = 0; i < height; i++) {
            memset(row, value, width);
	 row += dst->pitch;
         }
      }
      break;
   case 2:
      {
         ushort *row = (ushort *) get_pointer(dst, dstx, dsty);
         for (i = 0; i < height; i++) {
            for (j = 0; j < width; j++)
               row[j] = (ushort) value;
            row += dst->pitch;
         }
      }
      break;
   case 4:
      {
         unsigned *row = (unsigned *) get_pointer(dst, dstx, dsty);
         for (i = 0; i < height; i++) {
            for (j = 0; j < width; j++)
               row[j] = value;
            row += dst->pitch;
         }
      }
      break;
   case 8:
      {
         /* expand the 4-byte clear value to an 8-byte value */
         ushort *row = (ushort *) get_pointer(dst, dstx, dsty);
         ushort val0 = UBYTE_TO_USHORT((value >>  0) & 0xff);
         ushort val1 = UBYTE_TO_USHORT((value >>  8) & 0xff);
         ushort val2 = UBYTE_TO_USHORT((value >> 16) & 0xff);
         ushort val3 = UBYTE_TO_USHORT((value >> 24) & 0xff);
         for (i = 0; i < height; i++) {
            for (j = 0; j < width; j++) {
               row[j*4+0] = val0;
               row[j*4+1] = val1;
               row[j*4+2] = val2;
               row[j*4+3] = val3;
            }
            row += dst->pitch * 4;
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
sp_init_surface_functions(struct softpipe_context *sp)
{
   sp->pipe.get_tile = softpipe_get_tile;
   sp->pipe.put_tile = softpipe_put_tile;

   sp->pipe.get_tile_rgba = softpipe_get_tile_rgba;
   sp->pipe.put_tile_rgba = softpipe_put_tile_rgba;

   sp->pipe.surface_data = sp_surface_data;
   sp->pipe.surface_copy = sp_surface_copy;
   sp->pipe.surface_fill = sp_surface_fill;
}

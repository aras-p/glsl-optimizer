/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "i915_context.h"
#include "i915_blit.h"
#include "i915_state.h"
#include "pipe/p_defines.h"
#include "pipe/p_inlines.h"
#include "pipe/p_util.h"
#include "pipe/p_inlines.h"
#include "pipe/p_winsys.h"
#include "pipe/util/p_tile.h"


/*
 * XXX note: same as code in sp_surface.c
 */
static struct pipe_surface *
i915_get_tex_surface(struct pipe_context *pipe,
                     struct pipe_texture *pt,
                     unsigned face, unsigned level, unsigned zslice)
{
   struct i915_texture *tex = (struct i915_texture *)pt;
   struct pipe_surface *ps;
   unsigned offset;  /* in bytes */

   offset = tex->level_offset[level];

   if (pt->target == PIPE_TEXTURE_CUBE) {
      offset += tex->image_offset[level][face] * pt->cpp;
   }
   else if (pt->target == PIPE_TEXTURE_3D) {
      offset += tex->image_offset[level][zslice] * pt->cpp;
   }
   else {
      assert(face == 0);
      assert(zslice == 0);
   }

   ps = pipe->winsys->surface_alloc(pipe->winsys);
   if (ps) {
      assert(ps->refcount);
      assert(ps->winsys);
      pipe->winsys->buffer_reference(pipe->winsys, &ps->buffer, tex->buffer);
      ps->format = pt->format;
      ps->cpp = pt->cpp;
      ps->width = pt->width[level];
      ps->height = pt->height[level];
      ps->pitch = tex->pitch;
      ps->offset = offset;
   }
   return ps;
}


static void
copy_rect(ubyte * dst,
          unsigned cpp,
          unsigned dst_pitch,
          unsigned dst_x,
          unsigned dst_y,
          unsigned width,
          unsigned height,
          const ubyte *src,
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
   src += src_y * dst_pitch;
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
i915_surface_data(struct pipe_context *pipe,
		  struct pipe_surface *dst,
		  unsigned dstx, unsigned dsty,
		  const void *src, unsigned src_pitch,
		  unsigned srcx, unsigned srcy, unsigned width, unsigned height)
{
   copy_rect(pipe_surface_map(dst),
             dst->cpp, dst->pitch,
             dstx, dsty, width, height, src, src_pitch, srcx, srcy);

   pipe_surface_unmap(dst);
}


/* Assumes all values are within bounds -- no checking at this level -
 * do it higher up if required.
 */
static void
i915_surface_copy(struct pipe_context *pipe,
		  struct pipe_surface *dst,
		  unsigned dstx, unsigned dsty,
		  struct pipe_surface *src,
		  unsigned srcx, unsigned srcy, unsigned width, unsigned height)
{
   assert( dst != src );
   assert( dst->cpp == src->cpp );

   if (0) {
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
   else {
      i915_copy_blit( i915_context(pipe),
		      dst->cpp,
		      (short) src->pitch, src->buffer, src->offset,
		      (short) dst->pitch, dst->buffer, dst->offset,
		      (short) srcx, (short) srcy, (short) dstx, (short) dsty, (short) width, (short) height );
   }
}

/* Fill a rectangular sub-region.  Need better logic about when to
 * push buffers into AGP - will currently do so whenever possible.
 */
static void *
get_pointer(struct pipe_surface *dst, void *dst_map, unsigned x, unsigned y)
{
   return (char *)dst_map + (y * dst->pitch + x) * dst->cpp;
}


static void
i915_surface_fill(struct pipe_context *pipe,
		  struct pipe_surface *dst,
		  unsigned dstx, unsigned dsty,
		  unsigned width, unsigned height, unsigned value)
{
   if (0) {
      unsigned i, j;
      void *dst_map = pipe_surface_map(dst);

      switch (dst->cpp) {
      case 1: {
	 ubyte *row = get_pointer(dst, dst_map, dstx, dsty);
	 for (i = 0; i < height; i++) {
	    memset(row, value, width);
	    row += dst->pitch;
	 }
      }
	 break;
      case 2: {
	 ushort *row = get_pointer(dst, dst_map, dstx, dsty);
	 for (i = 0; i < height; i++) {
	    for (j = 0; j < width; j++)
	       row[j] = (ushort) value;
	    row += dst->pitch;
	 }
      }
	 break;
      case 4: {
	 unsigned *row = get_pointer(dst, dst_map, dstx, dsty);
	 for (i = 0; i < height; i++) {
	    for (j = 0; j < width; j++)
	       row[j] = value;
	    row += dst->pitch;
	 }
      }
	 break;
      default:
	 assert(0);
	 break;
      }

      pipe_surface_unmap( dst );
   }
   else {
      i915_fill_blit( i915_context(pipe),
		      dst->cpp,
		      (short) dst->pitch, 
		      dst->buffer, dst->offset,
		      (short) dstx, (short) dsty, 
		      (short) width, (short) height, 
		      value );
   }
}


void
i915_init_surface_functions(struct i915_context *i915)
{
   i915->pipe.get_tex_surface = i915_get_tex_surface;
   i915->pipe.get_tile = pipe_get_tile_raw;
   i915->pipe.put_tile = pipe_put_tile_raw;

   i915->pipe.surface_data = i915_surface_data;
   i915->pipe.surface_copy = i915_surface_copy;
   i915->pipe.surface_fill = i915_surface_fill;
}

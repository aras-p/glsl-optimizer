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
#include "pipe/p_inlines.h"
#include "pipe/p_winsys.h"
#include "util/u_tile.h"
#include "util/u_rect.h"


/* Assumes all values are within bounds -- no checking at this level -
 * do it higher up if required.
 */
static void
i915_surface_copy(struct pipe_context *pipe,
                  boolean do_flip,
		  struct pipe_surface *dst,
		  unsigned dstx, unsigned dsty,
		  struct pipe_surface *src,
		  unsigned srcx, unsigned srcy, unsigned width, unsigned height)
{
   assert( dst != src );
   assert( dst->block.size == src->block.size );
   assert( dst->block.width == src->block.height );
   assert( dst->block.height == src->block.height );

   if (0) {
      void *dst_map = pipe->screen->surface_map( pipe->screen,
                                                 dst,
                                                 PIPE_BUFFER_USAGE_CPU_WRITE );
      
      const void *src_map = pipe->screen->surface_map( pipe->screen,
                                                       src,
                                                       PIPE_BUFFER_USAGE_CPU_READ );
      
      pipe_copy_rect(dst_map,
                     &dst->block,
                     dst->stride,
                     dstx, dsty,
                     width, height,
                     src_map,
                     do_flip ? -(int) src->stride : src->stride,
                     srcx, do_flip ? height - 1 - srcy : srcy);

      pipe->screen->surface_unmap(pipe->screen, src);
      pipe->screen->surface_unmap(pipe->screen, dst);
   }
   else {
      assert(dst->block.width == 1);
      assert(dst->block.height == 1);
      i915_copy_blit( i915_context(pipe),
                      do_flip,
                      dst->block.size,
		      (short) src->stride, src->buffer, src->offset,
		      (short) dst->stride, dst->buffer, dst->offset,
		      (short) srcx, (short) srcy, (short) dstx, (short) dsty, (short) width, (short) height );
   }
}


static void
i915_surface_fill(struct pipe_context *pipe,
		  struct pipe_surface *dst,
		  unsigned dstx, unsigned dsty,
		  unsigned width, unsigned height, unsigned value)
{
   if (0) {
      void *dst_map = pipe->screen->surface_map( pipe->screen,
                                                 dst,
                                                 PIPE_BUFFER_USAGE_CPU_WRITE );

      pipe_fill_rect(dst_map, &dst->block, dst->stride, dstx, dsty, width, height, value);

      pipe->screen->surface_unmap(pipe->screen, dst);
   }
   else {
      assert(dst->block.width == 1);
      assert(dst->block.height == 1);
      i915_fill_blit( i915_context(pipe),
		      dst->block.size,
		      (short) dst->stride,
		      dst->buffer, dst->offset,
		      (short) dstx, (short) dsty,
		      (short) width, (short) height,
		      value );
   }
}


void
i915_init_surface_functions(struct i915_context *i915)
{
   i915->pipe.surface_copy = i915_surface_copy;
   i915->pipe.surface_fill = i915_surface_fill;
}

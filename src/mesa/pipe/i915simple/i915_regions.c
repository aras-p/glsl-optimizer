/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.
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

/* Provide additional functionality on top of bufmgr buffers:
 *   - 2d semantics and blit operations (XXX: remove/simplify blits??)
 *   - refcounting of buffers for multiple images in a buffer.
 *   - refcounting of buffer mappings.
 */

#include "pipe/p_defines.h"
#include "pipe/p_winsys.h"
#include "i915_context.h"




static ubyte *
i915_region_map(struct pipe_context *pipe, struct pipe_region *region)
{
   struct i915_context *i915 = i915_context( pipe );

   if (!region->map_refcount++) {
      region->map = i915->pipe.winsys->buffer_map( i915->pipe.winsys,
						   region->buffer,
						   PIPE_BUFFER_FLAG_WRITE | 
						   PIPE_BUFFER_FLAG_READ);
   }

   return region->map;
}

static void
i915_region_unmap(struct pipe_context *pipe, struct pipe_region *region)
{
   struct i915_context *i915 = i915_context( pipe );

   if (region->map_refcount > 0) {
      assert(region->map);
      if (!--region->map_refcount) {
         i915->pipe.winsys->buffer_unmap( i915->pipe.winsys,
                                          region->buffer );
         region->map = NULL;
      }
   }
}


void
i915_init_region_functions(struct i915_context *i915)
{
   i915->pipe.region_map = i915_region_map;
   i915->pipe.region_unmap = i915_region_unmap;
}


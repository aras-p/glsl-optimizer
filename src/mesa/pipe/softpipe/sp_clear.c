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

/* Author:
 *    Brian Paul
 */


#include "pipe/p_defines.h"
#include "sp_clear.h"
#include "sp_context.h"
#include "sp_surface.h"
#include "sp_state.h"
#include "sp_tile_cache.h"


/**
 * Clear the given surface to the specified value.
 * No masking, no scissor (clear entire buffer).
 */
void
softpipe_clear(struct pipe_context *pipe, struct pipe_surface *ps,
               unsigned clearValue)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);
   struct softpipe_surface *sps = softpipe_surface(ps);
   unsigned x, y, w, h;

   softpipe_update_derived(softpipe); /* not needed?? */

   w = softpipe->framebuffer.cbufs[0]->width;
   h = softpipe->framebuffer.cbufs[0]->height;

   /* Use the X coord to trick region_fill() into filling at an offset
    * from the start of the region.  Perhaps pipe_region should have the
    * 'offset' field, not pipe_surface???
    */
   assert(ps->offset % ps->region->cpp == 0);
   x = ps->offset / ps->region->cpp;
   y = 0;

   assert(w <= ps->region->pitch);
   assert(h <= ps->region->height);

   /* XXX skip this fill if we're using tile cache */
   pipe->region_fill(pipe, ps->region, 0, x, y, w, h, clearValue);

   sp_clear_tile_cache(sps, clearValue);
}

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
#include "util/u_pack_color.h"
#include "sp_clear.h"
#include "sp_context.h"
#include "sp_surface.h"
#include "sp_state.h"
#include "sp_tile_cache.h"


/**
 * Convert packed pixel from one format to another.
 */
static unsigned
convert_color(enum pipe_format srcFormat, unsigned srcColor,
              enum pipe_format dstFormat)
{
   ubyte r, g, b, a;
   unsigned dstColor;

   util_unpack_color_ub(srcFormat, &srcColor, &r, &g, &b, &a);
   util_pack_color_ub(r, g, b, a, dstFormat, &dstColor);

   return dstColor;
}



/**
 * Clear the given surface to the specified value.
 * No masking, no scissor (clear entire buffer).
 * Note: when clearing a color buffer, the clearValue is always
 * encoded as PIPE_FORMAT_A8R8G8B8_UNORM.
 */
void
softpipe_clear(struct pipe_context *pipe, struct pipe_surface *ps,
               unsigned clearValue)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);
   uint i;

   if (softpipe->no_rast)
      return;

#if 0
   softpipe_update_derived(softpipe); /* not needed?? */
#endif

   if (ps == sp_tile_cache_get_surface(softpipe->zsbuf_cache)) {
      sp_tile_cache_clear(softpipe->zsbuf_cache, clearValue);
      softpipe->framebuffer.zsbuf->status = PIPE_SURFACE_STATUS_CLEAR;
#if TILE_CLEAR_OPTIMIZATION
      return;
#endif
   }

   for (i = 0; i < softpipe->framebuffer.num_cbufs; i++) {
      if (ps == sp_tile_cache_get_surface(softpipe->cbuf_cache[i])) {
         unsigned cv;
         if (ps->format != PIPE_FORMAT_A8R8G8B8_UNORM) {
            cv = convert_color(PIPE_FORMAT_A8R8G8B8_UNORM, clearValue,
                               ps->format);
         }
         else {
            cv = clearValue;
         }
         sp_tile_cache_clear(softpipe->cbuf_cache[i], cv);
         softpipe->framebuffer.cbufs[i]->status = PIPE_SURFACE_STATUS_CLEAR;
      }
   }

#if !TILE_CLEAR_OPTIMIZATION
   /* non-cached surface */
   pipe->surface_fill(pipe, ps, 0, 0, ps->width, ps->height, clearValue);
#endif
}

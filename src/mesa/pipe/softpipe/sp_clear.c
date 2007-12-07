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

   softpipe_update_derived(softpipe); /* not needed?? */

   if (ps == sp_tile_cache_get_surface(softpipe->zbuf_cache)) {
      float clear[4];
      clear[0] = 1.0; /* XXX hack */
      sp_tile_cache_clear(softpipe->zbuf_cache, clear);
   }
   else if (ps == sp_tile_cache_get_surface(softpipe->cbuf_cache[0])) {
      float clear[4];
      /* XXX it sure would be nice if the clear color was passed to
       * this function as float[4]....
       */
      uint r, g, b, a;
      switch (ps->format) {
      case PIPE_FORMAT_R8G8B8A8_UNORM:
         r = (clearValue >> 24) & 0xff;
         g = (clearValue >> 16) & 0xff;
         g = (clearValue >>  8) & 0xff;
         a = (clearValue      ) & 0xff;
         break;
      case PIPE_FORMAT_A8R8G8B8_UNORM:
         r = (clearValue >> 16) & 0xff;
         g = (clearValue >>  8) & 0xff;
         b = (clearValue      ) & 0xff;
         a = (clearValue >> 24) & 0xff;
         break;
      case PIPE_FORMAT_B8G8R8A8_UNORM:
         r = (clearValue >>  8) & 0xff;
         g = (clearValue >> 16) & 0xff;
         b = (clearValue >> 24) & 0xff;
         a = (clearValue      ) & 0xff;
         break;
      default:
         assert(0);
      }

      clear[0] = r / 255.0;
      clear[1] = g / 255.0;
      clear[2] = b / 255.0;
      clear[3] = a / 255.0;

      sp_tile_cache_clear(softpipe->cbuf_cache[0], clear);
   }

   pipe->surface_fill(pipe, ps, 0, 0, ps->width, ps->height, clearValue);


#if 0
   sp_clear_tile_cache(ps, clearValue);
#endif
}

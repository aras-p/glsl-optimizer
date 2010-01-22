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
 *    Keith Whitwell <keith@tungstengraphics.com>
 */


#include "pipe/p_defines.h"
#include "draw/draw_context.h"
#include "lp_flush.h"
#include "lp_context.h"
#include "lp_surface.h"
#include "lp_state.h"
#include "lp_tile_cache.h"


void
llvmpipe_flush( struct pipe_context *pipe,
		unsigned flags,
                struct pipe_fence_handle **fence )
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
   uint i;

   draw_flush(llvmpipe->draw);

   if (flags & PIPE_FLUSH_SWAPBUFFERS) {
      /* If this is a swapbuffers, just flush color buffers.
       *
       * The zbuffer changes are not discarded, but held in the cache
       * in the hope that a later clear will wipe them out.
       */
      for (i = 0; i < llvmpipe->framebuffer.nr_cbufs; i++)
         if (llvmpipe->cbuf_cache[i]) {
            lp_tile_cache_map_transfers(llvmpipe->cbuf_cache[i]);
            lp_flush_tile_cache(llvmpipe->cbuf_cache[i]);
         }

      /* Need this call for hardware buffers before swapbuffers.
       *
       * there should probably be another/different flush-type function
       * that's called before swapbuffers because we don't always want
       * to unmap surfaces when flushing.
       */
      llvmpipe_unmap_transfers(llvmpipe);
   }
   else if (flags & PIPE_FLUSH_RENDER_CACHE) {
      for (i = 0; i < llvmpipe->framebuffer.nr_cbufs; i++)
         if (llvmpipe->cbuf_cache[i]) {
            lp_tile_cache_map_transfers(llvmpipe->cbuf_cache[i]);
            lp_flush_tile_cache(llvmpipe->cbuf_cache[i]);
         }

      /* FIXME: untile zsbuf! */
     
      llvmpipe->dirty_render_cache = FALSE;
   }

   /* Enable to dump BMPs of the color/depth buffers each frame */
#if 0
   if(flags & PIPE_FLUSH_FRAME) {
      static unsigned frame_no = 1;
      static char filename[256];
      util_snprintf(filename, sizeof(filename), "cbuf_%u.bmp", frame_no);
      debug_dump_surface_bmp(filename, llvmpipe->framebuffer.cbufs[0]);
      util_snprintf(filename, sizeof(filename), "zsbuf_%u.bmp", frame_no);
      debug_dump_surface_bmp(filename, llvmpipe->framebuffer.zsbuf);
      ++frame_no;
   }
#endif
   
   if (fence)
      *fence = NULL;
}


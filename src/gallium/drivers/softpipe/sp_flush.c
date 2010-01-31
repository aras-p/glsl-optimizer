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
#include "sp_flush.h"
#include "sp_context.h"
#include "sp_state.h"
#include "sp_tile_cache.h"
#include "sp_tex_tile_cache.h"


void
softpipe_flush( struct pipe_context *pipe,
		unsigned flags,
                struct pipe_fence_handle **fence )
{
   struct softpipe_context *softpipe = softpipe_context(pipe);
   uint i;

   draw_flush(softpipe->draw);

   if (flags & PIPE_FLUSH_TEXTURE_CACHE) {
      for (i = 0; i < softpipe->num_textures; i++) {
         sp_flush_tex_tile_cache(softpipe->tex_cache[i]);
      }
      for (i = 0; i < softpipe->num_vertex_textures; i++) {
         sp_flush_tex_tile_cache(softpipe->vertex_tex_cache[i]);
      }
   }

   if (flags & PIPE_FLUSH_SWAPBUFFERS) {
      /* If this is a swapbuffers, just flush color buffers.
       *
       * The zbuffer changes are not discarded, but held in the cache
       * in the hope that a later clear will wipe them out.
       */
      for (i = 0; i < softpipe->framebuffer.nr_cbufs; i++)
         if (softpipe->cbuf_cache[i])
            sp_flush_tile_cache(softpipe->cbuf_cache[i]);

      /* Need this call for hardware buffers before swapbuffers.
       *
       * there should probably be another/different flush-type function
       * that's called before swapbuffers because we don't always want
       * to unmap surfaces when flushing.
       */
      softpipe_unmap_transfers(softpipe);
   }
   else if (flags & PIPE_FLUSH_RENDER_CACHE) {
      for (i = 0; i < softpipe->framebuffer.nr_cbufs; i++)
         if (softpipe->cbuf_cache[i])
            sp_flush_tile_cache(softpipe->cbuf_cache[i]);

      if (softpipe->zsbuf_cache)
         sp_flush_tile_cache(softpipe->zsbuf_cache);
     
      softpipe->dirty_render_cache = FALSE;
   }

   /* Enable to dump BMPs of the color/depth buffers each frame */
#if 0
   if(flags & PIPE_FLUSH_FRAME) {
      static unsigned frame_no = 1;
      static char filename[256];
      util_snprintf(filename, sizeof(filename), "cbuf_%u.bmp", frame_no);
      debug_dump_surface_bmp(filename, softpipe->framebuffer.cbufs[0]);
      util_snprintf(filename, sizeof(filename), "zsbuf_%u.bmp", frame_no);
      debug_dump_surface_bmp(filename, softpipe->framebuffer.zsbuf);
      ++frame_no;
   }
#endif
   
   if (fence)
      *fence = NULL;
}


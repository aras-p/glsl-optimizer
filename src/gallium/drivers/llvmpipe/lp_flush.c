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
#include "lp_setup.h"


void
llvmpipe_flush( struct pipe_context *pipe,
		unsigned flags,
                struct pipe_fence_handle **fence )
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);

   draw_flush(llvmpipe->draw);

   if (fence) {
      if ((flags & (PIPE_FLUSH_SWAPBUFFERS |
                    PIPE_FLUSH_RENDER_CACHE))) {
         /* if we're going to flush the setup/rasterization modules, emit
          * a fence.
          * XXX this (and the code below) may need fine tuning...
          */
         *fence = lp_setup_fence( llvmpipe->setup );
      }
      else {
         *fence = NULL;
      }
   }

   /* XXX the lp_setup_flush(flags) param is not a bool, and it's ignored
    * at this time!
    */
   if (flags & PIPE_FLUSH_SWAPBUFFERS) {
      lp_setup_flush( llvmpipe->setup, FALSE );
   }
   else if (flags & PIPE_FLUSH_RENDER_CACHE) {
      lp_setup_flush( llvmpipe->setup, TRUE );
   }

   /* Enable to dump BMPs of the color/depth buffers each frame */
#if 0
   if (flags & PIPE_FLUSH_FRAME) {
      static unsigned frame_no = 1;
      char filename[256];
      unsigned i;

      for (i = 0; i < llvmpipe->framebuffer.nr_cbufs; i++) {
	 util_snprintf(filename, sizeof(filename), "cbuf%u_%u", i, frame_no);
         debug_dump_surface(filename, llvmpipe->framebuffer.cbufs[i]);
      }

      if (0) {
         util_snprintf(filename, sizeof(filename), "zsbuf_%u", frame_no);
         debug_dump_surface(filename, llvmpipe->framebuffer.zsbuf);
      }

      ++frame_no;
   }
#endif
}


/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * Copyright 2009 VMware, Inc.  All Rights Reserved.
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
 *    Michel Dänzer
 */


#include "pipe/p_defines.h"
#include "util/u_pack_color.h"
#include "lp_clear.h"
#include "lp_context.h"
#include "lp_surface.h"
#include "lp_state.h"
#include "lp_tile_cache.h"


/**
 * Clear the given buffers to the specified values.
 * No masking, no scissor (clear entire buffer).
 */
void
llvmpipe_clear(struct pipe_context *pipe, unsigned buffers, const float *rgba,
               double depth, unsigned stencil)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
   unsigned cv;
   uint i;

   if (llvmpipe->no_rast)
      return;

#if 0
   llvmpipe_update_derived(llvmpipe); /* not needed?? */
#endif

   if (buffers & PIPE_CLEAR_COLOR) {
      for (i = 0; i < llvmpipe->framebuffer.nr_cbufs; i++) {
         struct pipe_surface *ps = llvmpipe->framebuffer.cbufs[i];

         util_pack_color(rgba, ps->format, &cv);
         lp_tile_cache_clear(llvmpipe->cbuf_cache[i], rgba, cv);
      }
      llvmpipe->dirty_render_cache = TRUE;
   }

   if (buffers & PIPE_CLEAR_DEPTHSTENCIL) {
      struct pipe_surface *ps = llvmpipe->framebuffer.zsbuf;

      cv = util_pack_z_stencil(ps->format, depth, stencil);

      /* non-cached surface */
      pipe->surface_fill(pipe, ps, 0, 0, ps->width, ps->height, cv);
   }
}

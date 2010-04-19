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

#include "pipe/p_defines.h"
#include "util/u_memory.h"
#include "lp_context.h"
#include "lp_state.h"
#include "lp_setup.h"
#include "draw/draw_context.h"



void *
llvmpipe_create_rasterizer_state(struct pipe_context *pipe,
                                 const struct pipe_rasterizer_state *rast)
{
   /* We do nothing special with rasterizer state.
    * The CSO handle is just a pointer to a pipe_rasterizer_state object.
    */
   return mem_dup(rast, sizeof(*rast));
}



void
llvmpipe_bind_rasterizer_state(struct pipe_context *pipe, void *handle)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
   const struct pipe_rasterizer_state *rasterizer =
      (const struct pipe_rasterizer_state *) handle;

   if (llvmpipe->rasterizer == rasterizer)
      return;

   /* pass-through to draw module */
   draw_set_rasterizer_state(llvmpipe->draw, rasterizer, handle);

   llvmpipe->rasterizer = rasterizer;

   /* Note: we can immediately set the triangle state here and
    * not worry about binning because we handle culling during
    * triangle setup, not when rasterizing the bins.
    */
   if (llvmpipe->rasterizer) {
      lp_setup_set_triangle_state( llvmpipe->setup,
                   llvmpipe->rasterizer->cull_mode,
                   llvmpipe->rasterizer->front_winding == PIPE_WINDING_CCW,
                   llvmpipe->rasterizer->scissor);
   }

   llvmpipe->dirty |= LP_NEW_RASTERIZER;
}


void llvmpipe_delete_rasterizer_state(struct pipe_context *pipe,
                                      void *rasterizer)
{
   FREE( rasterizer );
}



/**************************************************************************
 * 
 * Copyright 2009 VMware, Inc.
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

/**
 * @author Jose Fonseca <jfonseca@vmware.com>
 * @author Keith Whitwell <keith@tungstengraphics.com>
 */

#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_debug_dump.h"
#include "lp_screen.h"
#include "lp_context.h"
#include "lp_state.h"


void *
llvmpipe_create_blend_state(struct pipe_context *pipe,
                            const struct pipe_blend_state *blend)
{
   return mem_dup(blend, sizeof(*blend));
}

void llvmpipe_bind_blend_state( struct pipe_context *pipe,
                                void *blend )
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);

   llvmpipe->blend = blend;

   llvmpipe->dirty |= LP_NEW_BLEND;
}

void llvmpipe_delete_blend_state(struct pipe_context *pipe,
                                 void *blend)
{
   FREE( blend );
}


void llvmpipe_set_blend_color( struct pipe_context *pipe,
			     const struct pipe_blend_color *blend_color )
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
   unsigned i, j;

   memcpy(&llvmpipe->blend_color, blend_color, sizeof *blend_color);

   if(!llvmpipe->jit_context.blend_color)
      llvmpipe->jit_context.blend_color = align_malloc(4 * 16, 16);
   for (i = 0; i < 4; ++i) {
      uint8_t c = float_to_ubyte(blend_color->color[i]);
      for (j = 0; j < 16; ++j)
         llvmpipe->jit_context.blend_color[i*16 + j] = c;
   }
}


/** XXX move someday?  Or consolidate all these simple state setters
 * into one file.
 */


void *
llvmpipe_create_depth_stencil_state(struct pipe_context *pipe,
				    const struct pipe_depth_stencil_alpha_state *depth_stencil)
{
   return mem_dup(depth_stencil, sizeof(*depth_stencil));
}

void
llvmpipe_bind_depth_stencil_state(struct pipe_context *pipe,
                                  void *depth_stencil)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);

   llvmpipe->depth_stencil = (const struct pipe_depth_stencil_alpha_state *)depth_stencil;

   if(llvmpipe->depth_stencil)
      llvmpipe->jit_context.alpha_ref_value = llvmpipe->depth_stencil->alpha.ref_value;

   llvmpipe->dirty |= LP_NEW_DEPTH_STENCIL_ALPHA;
}

void
llvmpipe_delete_depth_stencil_state(struct pipe_context *pipe, void *depth)
{
   FREE( depth );
}

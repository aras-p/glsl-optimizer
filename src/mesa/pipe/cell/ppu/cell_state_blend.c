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

/* Authors:  Keith Whitwell <keith@tungstengraphics.com>
 */

#include "pipe/p_util.h"
#include "cell_context.h"
#include "cell_state.h"

void *
cell_create_blend_state(struct pipe_context *pipe,
                            const struct pipe_blend_state *blend)
{
   struct pipe_blend_state *state = MALLOC( sizeof(struct pipe_blend_state) );
   memcpy(state, blend, sizeof(struct pipe_blend_state));
   return state;
}

void cell_bind_blend_state( struct pipe_context *pipe,
                                void *blend )
{
   struct cell_context *cell = cell_context(pipe);

   cell->blend = (const struct pipe_blend_state *)blend;

   cell->dirty |= CELL_NEW_BLEND;
}

void cell_delete_blend_state(struct pipe_context *pipe,
                                 void *blend)
{
   FREE( blend );
}


void cell_set_blend_color( struct pipe_context *pipe,
			     const struct pipe_blend_color *blend_color )
{
   struct cell_context *cell = cell_context(pipe);

   cell->blend_color = *blend_color;

   cell->dirty |= CELL_NEW_BLEND;
}



void *
cell_create_depth_stencil_state(struct pipe_context *pipe,
                              const struct pipe_depth_stencil_alpha_state *depth_stencil)
{
   struct pipe_depth_stencil_alpha_state *state =
      MALLOC( sizeof(struct pipe_depth_stencil_alpha_state) );
   memcpy(state, depth_stencil, sizeof(struct pipe_depth_stencil_alpha_state));
   return state;
}

void
cell_bind_depth_stencil_state(struct pipe_context *pipe,
                                  void *depth_stencil)
{
   struct cell_context *cell = cell_context(pipe);

   cell->depth_stencil = (const struct pipe_depth_stencil_alpha_state *)depth_stencil;

   cell->dirty |= CELL_NEW_DEPTH_STENCIL;
}

void
cell_delete_depth_stencil_state(struct pipe_context *pipe, void *depth)
{
   FREE( depth );
}

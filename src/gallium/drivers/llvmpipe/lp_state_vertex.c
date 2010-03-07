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


#include "lp_context.h"
#include "lp_state.h"

#include "draw/draw_context.h"


void
llvmpipe_set_vertex_elements(struct pipe_context *pipe,
                             unsigned count,
                             const struct pipe_vertex_element *attribs)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);

   assert(count <= PIPE_MAX_ATTRIBS);

   memcpy(llvmpipe->vertex_element, attribs,
          count * sizeof(struct pipe_vertex_element));
   llvmpipe->num_vertex_elements = count;

   llvmpipe->dirty |= LP_NEW_VERTEX;

   draw_set_vertex_elements(llvmpipe->draw, count, attribs);
}


void
llvmpipe_set_vertex_buffers(struct pipe_context *pipe,
                            unsigned count,
                            const struct pipe_vertex_buffer *buffers)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);

   assert(count <= PIPE_MAX_ATTRIBS);

   memcpy(llvmpipe->vertex_buffer, buffers, count * sizeof(buffers[0]));
   llvmpipe->num_vertex_buffers = count;

   llvmpipe->dirty |= LP_NEW_VERTEX;

   draw_set_vertex_buffers(llvmpipe->draw, count, buffers);
}

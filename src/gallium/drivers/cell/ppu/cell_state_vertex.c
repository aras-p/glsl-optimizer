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


#include "cell_context.h"
#include "cell_state.h"

#include "draw/draw_context.h"


static void
cell_set_vertex_elements(struct pipe_context *pipe,
                         unsigned count,
                         const struct pipe_vertex_element *elements)
{
   struct cell_context *cell = cell_context(pipe);

   assert(count <= PIPE_MAX_ATTRIBS);

   memcpy(cell->vertex_element, elements, count * sizeof(elements[0]));
   cell->num_vertex_elements = count;

   cell->dirty |= CELL_NEW_VERTEX;

   draw_set_vertex_elements(cell->draw, count, elements);
}


static void
cell_set_vertex_buffers(struct pipe_context *pipe,
                        unsigned count,
                        const struct pipe_vertex_buffer *buffers)
{
   struct cell_context *cell = cell_context(pipe);

   assert(count <= PIPE_MAX_ATTRIBS);

   memcpy(cell->vertex_buffer, buffers, count * sizeof(buffers[0]));
   cell->num_vertex_buffers = count;

   cell->dirty |= CELL_NEW_VERTEX;

   draw_set_vertex_buffers(cell->draw, count, buffers);
}


void
cell_init_vertex_functions(struct cell_context *cell)
{
   cell->pipe.set_vertex_buffers = cell_set_vertex_buffers;
   cell->pipe.set_vertex_elements = cell_set_vertex_elements;
}

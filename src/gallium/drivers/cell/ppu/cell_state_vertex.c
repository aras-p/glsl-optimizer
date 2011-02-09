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

#include "util/u_memory.h"
#include "util/u_transfer.h"
#include "draw/draw_context.h"


static void *
cell_create_vertex_elements_state(struct pipe_context *pipe,
                                  unsigned count,
                                  const struct pipe_vertex_element *attribs)
{
   struct cell_velems_state *velems;
   assert(count <= PIPE_MAX_ATTRIBS);
   velems = (struct cell_velems_state *) MALLOC(sizeof(struct cell_velems_state));
   if (velems) {
      velems->count = count;
      memcpy(velems->velem, attribs, sizeof(*attribs) * count);
   }
   return velems;
}

static void
cell_bind_vertex_elements_state(struct pipe_context *pipe,
                                void *velems)
{
   struct cell_context *cell = cell_context(pipe);
   struct cell_velems_state *cell_velems = (struct cell_velems_state *) velems;

   cell->velems = cell_velems;

   cell->dirty |= CELL_NEW_VERTEX;

   if (cell_velems)
      draw_set_vertex_elements(cell->draw, cell_velems->count, cell_velems->velem);
}

static void
cell_delete_vertex_elements_state(struct pipe_context *pipe, void *velems)
{
   FREE( velems );
}


static void
cell_set_vertex_buffers(struct pipe_context *pipe,
                        unsigned count,
                        const struct pipe_vertex_buffer *buffers)
{
   struct cell_context *cell = cell_context(pipe);

   assert(count <= PIPE_MAX_ATTRIBS);

   util_copy_vertex_buffers(cell->vertex_buffer,
                            &cell->num_vertex_buffers,
                            buffers, count);

   cell->dirty |= CELL_NEW_VERTEX;

   draw_set_vertex_buffers(cell->draw, count, buffers);
}


static void
cell_set_index_buffer(struct pipe_context *pipe,
                      const struct pipe_index_buffer *ib)
{
   struct cell_context *cell = cell_context(pipe);

   if (ib)
      memcpy(&cell->index_buffer, ib, sizeof(cell->index_buffer));
   else
      memset(&cell->index_buffer, 0, sizeof(cell->index_buffer));

   draw_set_index_buffer(cell->draw, ib);
}


void
cell_init_vertex_functions(struct cell_context *cell)
{
   cell->pipe.set_vertex_buffers = cell_set_vertex_buffers;
   cell->pipe.set_index_buffer = cell_set_index_buffer;
   cell->pipe.create_vertex_elements_state = cell_create_vertex_elements_state;
   cell->pipe.bind_vertex_elements_state = cell_bind_vertex_elements_state;
   cell->pipe.delete_vertex_elements_state = cell_delete_vertex_elements_state;
   cell->pipe.redefine_user_buffer = u_default_redefine_user_buffer;
}

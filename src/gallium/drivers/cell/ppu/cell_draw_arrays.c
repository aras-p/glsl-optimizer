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
 *    Brian Paul
 *    Keith Whitwell
 */


#include "pipe/p_defines.h"
#include "pipe/p_context.h"
#include "util/u_inlines.h"

#include "cell_context.h"
#include "cell_draw_arrays.h"
#include "cell_state.h"
#include "cell_flush.h"
#include "cell_texture.h"

#include "draw/draw_context.h"






/**
 * Draw vertex arrays, with optional indexing.
 * Basically, map the vertex buffers (and drawing surfaces), then hand off
 * the drawing to the 'draw' module.
 *
 * XXX should the element buffer be specified/bound with a separate function?
 */
static void
cell_draw_vbo(struct pipe_context *pipe, const struct pipe_draw_info *info)
{
   struct cell_context *cell = cell_context(pipe);
   struct draw_context *draw = cell->draw;
   void *mapped_indices = NULL;
   unsigned i;

   if (cell->dirty)
      cell_update_derived( cell );

#if 0
   cell_map_surfaces(cell);
#endif

   /*
    * Map vertex buffers
    */
   for (i = 0; i < cell->num_vertex_buffers; i++) {
      void *buf = cell_resource(cell->vertex_buffer[i].buffer)->data;
      draw_set_mapped_vertex_buffer(draw, i, buf);
   }
   /* Map index buffer, if present */
   if (info->indexed && cell->index_buffer.buffer)
      mapped_indices = cell_resource(cell->index_buffer.buffer)->data;

   draw_set_mapped_index_buffer(draw, mapped_indices);

   /* draw! */
   draw_vbo(draw, info);

   /*
    * unmap vertex/index buffers - will cause draw module to flush
    */
   for (i = 0; i < cell->num_vertex_buffers; i++) {
      draw_set_mapped_vertex_buffer(draw, i, NULL);
   }
   if (mapped_indices) {
      draw_set_mapped_index_buffer(draw, NULL);
   }

   /*
    * TODO: Flush only when a user vertex/index buffer is present
    * (or even better, modify draw module to do this
    * internally when this condition is seen?)
    */
   draw_flush(draw);
}


void
cell_init_draw_functions(struct cell_context *cell)
{
   cell->pipe.draw_vbo = cell_draw_vbo;
}


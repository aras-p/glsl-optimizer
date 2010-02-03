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
#include "util/u_simple_screen.h"
#include "util/u_inlines.h"

#include "cell_context.h"
#include "cell_draw_arrays.h"
#include "cell_state.h"
#include "cell_flush.h"

#include "draw/draw_context.h"



static void
cell_map_constant_buffers(struct cell_context *sp)
{
   struct pipe_winsys *ws = sp->pipe.winsys;
   uint i;
   for (i = 0; i < 2; i++) {
      if (sp->constants[i] && sp->constants[i]->size) {
         sp->mapped_constants[i] = ws->buffer_map(ws, sp->constants[i],
                                                   PIPE_BUFFER_USAGE_CPU_READ);
         cell_flush_buffer_range(sp, sp->mapped_constants[i], 
                                 sp->constants[i]->size);
      }
   }

   draw_set_mapped_constant_buffer(sp->draw, PIPE_SHADER_VERTEX, 0,
                                   sp->mapped_constants[PIPE_SHADER_VERTEX],
                                   sp->constants[PIPE_SHADER_VERTEX]->size);
}

static void
cell_unmap_constant_buffers(struct cell_context *sp)
{
   struct pipe_winsys *ws = sp->pipe.winsys;
   uint i;
   for (i = 0; i < 2; i++) {
      if (sp->constants[i] && sp->constants[i]->size)
         ws->buffer_unmap(ws, sp->constants[i]);
      sp->mapped_constants[i] = NULL;
   }
}



/**
 * Draw vertex arrays, with optional indexing.
 * Basically, map the vertex buffers (and drawing surfaces), then hand off
 * the drawing to the 'draw' module.
 *
 * XXX should the element buffer be specified/bound with a separate function?
 */
static void
cell_draw_range_elements(struct pipe_context *pipe,
                         struct pipe_buffer *indexBuffer,
                         unsigned indexSize,
                         unsigned min_index,
                         unsigned max_index,
                         unsigned mode, unsigned start, unsigned count)
{
   struct cell_context *sp = cell_context(pipe);
   struct draw_context *draw = sp->draw;
   unsigned i;

   if (sp->dirty)
      cell_update_derived( sp );

#if 0
   cell_map_surfaces(sp);
#endif
   cell_map_constant_buffers(sp);

   /*
    * Map vertex buffers
    */
   for (i = 0; i < sp->num_vertex_buffers; i++) {
      void *buf = pipe_buffer_map(pipe->screen,
                                           sp->vertex_buffer[i].buffer,
                                           PIPE_BUFFER_USAGE_CPU_READ);
      cell_flush_buffer_range(sp, buf, sp->vertex_buffer[i].buffer->size);
      draw_set_mapped_vertex_buffer(draw, i, buf);
   }
   /* Map index buffer, if present */
   if (indexBuffer) {
      void *mapped_indexes = pipe_buffer_map(pipe->screen,
                                                      indexBuffer,
                                                      PIPE_BUFFER_USAGE_CPU_READ);
      draw_set_mapped_element_buffer(draw, indexSize, mapped_indexes);
   }
   else {
      /* no index/element buffer */
      draw_set_mapped_element_buffer(draw, 0, NULL);
   }


   /* draw! */
   draw_arrays(draw, mode, start, count);

   /*
    * unmap vertex/index buffers - will cause draw module to flush
    */
   for (i = 0; i < sp->num_vertex_buffers; i++) {
      draw_set_mapped_vertex_buffer(draw, i, NULL);
      pipe_buffer_unmap(pipe->screen, sp->vertex_buffer[i].buffer);
   }
   if (indexBuffer) {
      draw_set_mapped_element_buffer(draw, 0, NULL);
      pipe_buffer_unmap(pipe->screen, indexBuffer);
   }

   /* Note: leave drawing surfaces mapped */
   cell_unmap_constant_buffers(sp);
}


static void
cell_draw_elements(struct pipe_context *pipe,
                   struct pipe_buffer *indexBuffer,
                   unsigned indexSize,
                   unsigned mode, unsigned start, unsigned count)
{
   cell_draw_range_elements( pipe, indexBuffer,
                             indexSize,
                             0, 0xffffffff,
                             mode, start, count );
}


static void
cell_draw_arrays(struct pipe_context *pipe, unsigned mode,
                     unsigned start, unsigned count)
{
   cell_draw_elements(pipe, NULL, 0, mode, start, count);
}


void
cell_init_draw_functions(struct cell_context *cell)
{
   cell->pipe.draw_arrays = cell_draw_arrays;
   cell->pipe.draw_elements = cell_draw_elements;
   cell->pipe.draw_range_elements = cell_draw_range_elements;
}


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
#include "cell_buffer.h"

#include "draw/draw_context.h"






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

   /*
    * Map vertex buffers
    */
   for (i = 0; i < sp->num_vertex_buffers; i++) {
      void *buf = cell_buffer(cell->vertex_buffer[i].buffer)->data;
      draw_set_mapped_vertex_buffer(draw, i, buf);
   }
   /* Map index buffer, if present */
   if (indexBuffer) {
      void *mapped_indexes = cell_buffer(indexBuffer)->data;
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
   }
   if (indexBuffer) {
      draw_set_mapped_element_buffer(draw, 0, NULL);
   }

   /*
    * TODO: Flush only when a user vertex/index buffer is present
    * (or even better, modify draw module to do this
    * internally when this condition is seen?)
    */
   draw_flush(draw);
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


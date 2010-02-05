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
#include "util/u_prim.h"

#include "lp_buffer.h"
#include "lp_context.h"
#include "lp_state.h"

#include "draw/draw_context.h"



void
llvmpipe_draw_arrays(struct pipe_context *pipe, unsigned mode,
                     unsigned start, unsigned count)
{
   llvmpipe_draw_elements(pipe, NULL, 0, mode, start, count);
}


/**
 * Draw vertex arrays, with optional indexing.
 * Basically, map the vertex buffers (and drawing surfaces), then hand off
 * the drawing to the 'draw' module.
 */
void
llvmpipe_draw_range_elements(struct pipe_context *pipe,
                             struct pipe_buffer *indexBuffer,
                             unsigned indexSize,
                             unsigned min_index,
                             unsigned max_index,
                             unsigned mode, unsigned start, unsigned count)
{
   struct llvmpipe_context *lp = llvmpipe_context(pipe);
   struct draw_context *draw = lp->draw;
   unsigned i;

   if (lp->dirty)
      llvmpipe_update_derived( lp );

   /*
    * Map vertex buffers
    */
   for (i = 0; i < lp->num_vertex_buffers; i++) {
      void *buf = llvmpipe_buffer(lp->vertex_buffer[i].buffer)->data;
      draw_set_mapped_vertex_buffer(draw, i, buf);
   }

   /* Map index buffer, if present */
   if (indexBuffer) {
      void *mapped_indexes = llvmpipe_buffer(indexBuffer)->data;
      draw_set_mapped_element_buffer_range(draw, indexSize,
                                           min_index,
                                           max_index,
                                           mapped_indexes);
   }
   else {
      /* no index/element buffer */
      draw_set_mapped_element_buffer_range(draw, 0, start,
                                           start + count - 1, NULL);
   }

   /* draw! */
   draw_arrays(draw, mode, start, count);

   /*
    * unmap vertex/index buffers
    */
   for (i = 0; i < lp->num_vertex_buffers; i++) {
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


void
llvmpipe_draw_elements(struct pipe_context *pipe,
                       struct pipe_buffer *indexBuffer,
                       unsigned indexSize,
                       unsigned mode, unsigned start, unsigned count)
{
   llvmpipe_draw_range_elements( pipe, indexBuffer,
                                 indexSize,
                                 0, 0xffffffff,
                                 mode, start, count );
}


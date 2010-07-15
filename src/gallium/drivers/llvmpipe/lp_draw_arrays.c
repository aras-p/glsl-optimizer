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
#include "util/u_draw_quad.h"

#include "lp_context.h"
#include "lp_state.h"

#include "draw/draw_context.h"



/**
 * Draw vertex arrays, with optional indexing, optional instancing.
 * All the other drawing functions are implemented in terms of this function.
 * Basically, map the vertex buffers (and drawing surfaces), then hand off
 * the drawing to the 'draw' module.
 */
static void
llvmpipe_draw_vbo(struct pipe_context *pipe, const struct pipe_draw_info *info)
{
   struct llvmpipe_context *lp = llvmpipe_context(pipe);
   struct draw_context *draw = lp->draw;
   void *mapped_indices = NULL;
   unsigned i;

   if (lp->dirty)
      llvmpipe_update_derived( lp );

   /*
    * Map vertex buffers
    */
   for (i = 0; i < lp->num_vertex_buffers; i++) {
      void *buf = llvmpipe_resource_data(lp->vertex_buffer[i].buffer);
      draw_set_mapped_vertex_buffer(draw, i, buf);
   }

   /* Map index buffer, if present */
   if (info->indexed && lp->index_buffer.buffer) {
      mapped_indices = llvmpipe_resource_data(lp->index_buffer.buffer);
      mapped_indices += lp->index_buffer.offset;
   }

   draw_set_mapped_element_buffer_range(draw, (mapped_indices) ?
                                        lp->index_buffer.index_size : 0,
                                        info->index_bias,
                                        info->min_index,
                                        info->max_index,
                                        mapped_indices);

   llvmpipe_prepare_vertex_sampling(lp,
                                    lp->num_vertex_sampler_views,
                                    lp->vertex_sampler_views);

   /* draw! */
   draw_arrays_instanced(draw, info->mode, info->start, info->count,
         info->start_instance, info->instance_count);

   /*
    * unmap vertex/index buffers
    */
   for (i = 0; i < lp->num_vertex_buffers; i++) {
      draw_set_mapped_vertex_buffer(draw, i, NULL);
   }
   if (mapped_indices) {
      draw_set_mapped_element_buffer(draw, 0, 0, NULL);
   }
   llvmpipe_cleanup_vertex_sampling(lp);

   /*
    * TODO: Flush only when a user vertex/index buffer is present
    * (or even better, modify draw module to do this
    * internally when this condition is seen?)
    */
   draw_flush(draw);
}


static void
llvmpipe_draw_range_elements_instanced(struct pipe_context *pipe,
                                       struct pipe_resource *indexBuffer,
                                       unsigned indexSize,
                                       int indexBias,
                                       unsigned minIndex,
                                       unsigned maxIndex,
                                       unsigned mode,
                                       unsigned start,
                                       unsigned count,
                                       unsigned startInstance,
                                       unsigned instanceCount)
{
   struct llvmpipe_context *lp = llvmpipe_context(pipe);
   struct pipe_draw_info info;
   struct pipe_index_buffer saved_ib, ib;

   util_draw_init_info(&info);
   info.mode = mode;
   info.start = start;
   info.count = count;
   info.start_instance = startInstance;
   info.instance_count = instanceCount;

   info.index_bias = indexBias;
   info.min_index = minIndex;
   info.max_index = maxIndex;

   if (indexBuffer) {
      info.indexed = TRUE;
      saved_ib = lp->index_buffer;

      ib.buffer = indexBuffer;
      ib.offset = 0;
      ib.index_size = indexSize;
      pipe->set_index_buffer(pipe, &ib);
   }

   llvmpipe_draw_vbo(pipe, &info);

   if (indexBuffer)
      pipe->set_index_buffer(pipe, &saved_ib);
}

static void
llvmpipe_draw_arrays_instanced(struct pipe_context *pipe,
                               unsigned mode,
                               unsigned start,
                               unsigned count,
                               unsigned startInstance,
                               unsigned instanceCount)
{
   llvmpipe_draw_range_elements_instanced(pipe,
                                          NULL, /* no indexBuffer */
                                          0, 0, /* indexSize, indexBias */
                                          0, ~0, /* minIndex, maxIndex */
                                          mode,
                                          start,
                                          count,
                                          startInstance,
                                          instanceCount);
}


static void
llvmpipe_draw_elements_instanced(struct pipe_context *pipe,
                                 struct pipe_resource *indexBuffer,
                                 unsigned indexSize,
                                 int indexBias,
                                 unsigned mode,
                                 unsigned start,
                                 unsigned count,
                                 unsigned startInstance,
                                 unsigned instanceCount)
{
   llvmpipe_draw_range_elements_instanced(pipe,
                                          indexBuffer,
                                          indexSize, indexBias,
                                          0, ~0, /* minIndex, maxIndex */
                                          mode,
                                          start,
                                          count,
                                          startInstance,
                                          instanceCount);
}


static void
llvmpipe_draw_elements(struct pipe_context *pipe,
                       struct pipe_resource *indexBuffer,
                       unsigned indexSize,
                       int indexBias,
                       unsigned mode,
                       unsigned start,
                       unsigned count)
{
   llvmpipe_draw_range_elements_instanced(pipe,
                                          indexBuffer,
                                          indexSize, indexBias,
                                          0, 0xffffffff,  /* min, maxIndex */
                                          mode, start, count,
                                          0,  /* startInstance */
                                          1);  /* instanceCount */
}


static void
llvmpipe_draw_range_elements(struct pipe_context *pipe,
                             struct pipe_resource *indexBuffer,
                             unsigned indexSize,
                             int indexBias,
                             unsigned min_index,
                             unsigned max_index,
                             unsigned mode,
                             unsigned start,
                             unsigned count)
{
   llvmpipe_draw_range_elements_instanced(pipe,
                                          indexBuffer,
                                          indexSize, indexBias,
                                          min_index, max_index,
                                          mode, start, count,
                                          0,  /* startInstance */
                                          1);  /* instanceCount */
}


static void
llvmpipe_draw_arrays(struct pipe_context *pipe,
                     unsigned mode,
                     unsigned start,
                     unsigned count)
{
   llvmpipe_draw_range_elements_instanced(pipe,
                                          NULL,  /* indexBuffer */
                                          0,  /* indexSize */
                                          0,  /* indexBias */
                                          0, ~0,  /* min, maxIndex */
                                          mode, start, count,
                                          0,  /* startInstance */
                                          1);  /* instanceCount */
}


void
llvmpipe_init_draw_funcs(struct llvmpipe_context *llvmpipe)
{
   llvmpipe->pipe.draw_arrays = llvmpipe_draw_arrays;
   llvmpipe->pipe.draw_elements = llvmpipe_draw_elements;
   llvmpipe->pipe.draw_range_elements = llvmpipe_draw_range_elements;
   llvmpipe->pipe.draw_arrays_instanced = llvmpipe_draw_arrays_instanced;
   llvmpipe->pipe.draw_elements_instanced = llvmpipe_draw_elements_instanced;

   llvmpipe->pipe.draw_vbo = llvmpipe_draw_vbo;
}

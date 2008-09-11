/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "i915_context.h"
#include "i915_winsys.h"
#include "i915_state.h"
#include "i915_batch.h"
#include "i915_texture.h"
#include "i915_reg.h"

#include "draw/draw_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_winsys.h"
#include "pipe/p_inlines.h"
#include "util/u_memory.h"
#include "pipe/p_screen.h"


static void i915_destroy( struct pipe_context *pipe )
{
   struct i915_context *i915 = i915_context( pipe );

   draw_destroy( i915->draw );
   
   if(i915->winsys->destroy)
      i915->winsys->destroy(i915->winsys);

   FREE( i915 );
}


static boolean
i915_draw_range_elements(struct pipe_context *pipe,
			     struct pipe_buffer *indexBuffer,
			     unsigned indexSize,
			     unsigned min_index,
			     unsigned max_index,
			     unsigned prim, unsigned start, unsigned count)
{
   struct i915_context *i915 = i915_context( pipe );
   struct draw_context *draw = i915->draw;
   unsigned i;

   if (i915->dirty)
      i915_update_derived( i915 );

   /*
    * Map vertex buffers
    */
   for (i = 0; i < i915->num_vertex_buffers; i++) {
      void *buf
         = pipe_buffer_map(pipe->screen,
                                    i915->vertex_buffer[i].buffer,
                                    PIPE_BUFFER_USAGE_CPU_READ);
      draw_set_mapped_vertex_buffer(draw, i, buf);
   }
   /* Map index buffer, if present */
   if (indexBuffer) {
      void *mapped_indexes
         = pipe_buffer_map(pipe->screen, indexBuffer,
                                    PIPE_BUFFER_USAGE_CPU_READ);
      draw_set_mapped_element_buffer_range(draw, indexSize,
					   min_index,
					   max_index,
					   mapped_indexes);
   }
   else {
      /* no index/element buffer */
      draw_set_mapped_element_buffer(draw, 0, NULL);
   }


   draw_set_mapped_constant_buffer(draw,
                                   i915->current.constants[PIPE_SHADER_VERTEX],
                                   ( i915->current.num_user_constants[PIPE_SHADER_VERTEX] * 
                                     4 * sizeof(float) ));

   /* draw! */
   draw_arrays(i915->draw, prim, start, count);

   /*
    * unmap vertex/index buffers
    */
   for (i = 0; i < i915->num_vertex_buffers; i++) {
      pipe_buffer_unmap(pipe->screen, i915->vertex_buffer[i].buffer);
      draw_set_mapped_vertex_buffer(draw, i, NULL);
   }
   if (indexBuffer) {
      pipe_buffer_unmap(pipe->screen, indexBuffer);
      draw_set_mapped_element_buffer_range(draw, 0, start, start + count - 1, NULL);
   }

   return TRUE;
}

static boolean
i915_draw_elements( struct pipe_context *pipe,
                    struct pipe_buffer *indexBuffer,
                    unsigned indexSize,
                    unsigned prim, unsigned start, unsigned count)
{
   return i915_draw_range_elements( pipe, indexBuffer,
					indexSize,
					0, 0xffffffff,
					prim, start, count );
}

static boolean i915_draw_arrays( struct pipe_context *pipe,
				 unsigned prim, unsigned start, unsigned count)
{
   return i915_draw_elements(pipe, NULL, 0, prim, start, count);
}



struct pipe_context *i915_create_context( struct pipe_screen *screen,
                                          struct pipe_winsys *pipe_winsys,
                                          struct i915_winsys *i915_winsys )
{
   struct i915_context *i915;

   i915 = CALLOC_STRUCT(i915_context);
   if (i915 == NULL)
      return NULL;

   i915->winsys = i915_winsys;
   i915->pipe.winsys = pipe_winsys;
   i915->pipe.screen = screen;

   i915->pipe.destroy = i915_destroy;

   i915->pipe.clear = i915_clear;


   i915->pipe.draw_arrays = i915_draw_arrays;
   i915->pipe.draw_elements = i915_draw_elements;
   i915->pipe.draw_range_elements = i915_draw_range_elements;

   /*
    * Create drawing context and plug our rendering stage into it.
    */
   i915->draw = draw_create();
   assert(i915->draw);
   if (!debug_get_bool_option("I915_NO_VBUF", FALSE)) {
      draw_set_rasterize_stage(i915->draw, i915_draw_vbuf_stage(i915));
   }
   else {
      draw_set_rasterize_stage(i915->draw, i915_draw_render_stage(i915));
   }

   i915_init_surface_functions(i915);
   i915_init_state_functions(i915);
   i915_init_flush_functions(i915);
   i915_init_texture_functions(i915);

   draw_install_aaline_stage(i915->draw, &i915->pipe);
   draw_install_aapoint_stage(i915->draw, &i915->pipe);

   i915->dirty = ~0;
   i915->hardware_dirty = ~0;

   /* Batch stream debugging is a bit hacked up at the moment:
    */
   i915->batch = i915_winsys->batch_get(i915_winsys);
   i915->batch->winsys = i915_winsys;

   return &i915->pipe;
}


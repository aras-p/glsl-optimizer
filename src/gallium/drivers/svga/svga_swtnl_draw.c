/**********************************************************
 * Copyright 2008-2009 VMware, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************/

#include "draw/draw_context.h"
#include "draw/draw_vbuf.h"
#include "util/u_inlines.h"
#include "pipe/p_state.h"

#include "svga_context.h"
#include "svga_swtnl.h"
#include "svga_state.h"
#include "svga_swtnl_private.h"



enum pipe_error
svga_swtnl_draw_range_elements(struct svga_context *svga,
                               struct pipe_buffer *indexBuffer,
                               unsigned indexSize,
                               unsigned min_index,
                               unsigned max_index,
                               unsigned prim, unsigned start, unsigned count)
{
   struct draw_context *draw = svga->swtnl.draw;
   unsigned i;
   const void *map;
   enum pipe_error ret;

   assert(!svga->dirty);
   assert(svga->state.sw.need_swtnl);
   assert(draw);

   ret = svga_update_state(svga, SVGA_STATE_SWTNL_DRAW);
   if (ret) {
      svga_context_flush(svga, NULL);
      ret = svga_update_state(svga, SVGA_STATE_SWTNL_DRAW);
      svga->swtnl.new_vbuf = TRUE;
      assert(ret == PIPE_OK);
   }

   /*
    * Map vertex buffers
    */
   for (i = 0; i < svga->curr.num_vertex_buffers; i++) {
      map = pipe_buffer_map(svga->pipe.screen,
                            svga->curr.vb[i].buffer,
                            PIPE_BUFFER_USAGE_CPU_READ);

      draw_set_mapped_vertex_buffer(draw, i, map);
   }

   /* Map index buffer, if present */
   if (indexBuffer) {
      map = pipe_buffer_map(svga->pipe.screen, indexBuffer,
                            PIPE_BUFFER_USAGE_CPU_READ);

      draw_set_mapped_element_buffer_range(draw, 
                                           indexSize, 
                                           min_index,
                                           max_index,
                                           map);
   }
   
   if (svga->curr.cb[PIPE_SHADER_VERTEX]) {
      map = pipe_buffer_map(svga->pipe.screen,
                            svga->curr.cb[PIPE_SHADER_VERTEX],
                            PIPE_BUFFER_USAGE_CPU_READ);
      assert(map);
      draw_set_mapped_constant_buffer(
         draw, PIPE_SHADER_VERTEX, 0,
         map,
         svga->curr.cb[PIPE_SHADER_VERTEX]->size);
   }

   draw_arrays(svga->swtnl.draw, prim, start, count);

   draw_flush(svga->swtnl.draw);

   /* Ensure the draw module didn't touch this */
   assert(i == svga->curr.num_vertex_buffers);
   
   /*
    * unmap vertex/index buffers
    */
   for (i = 0; i < svga->curr.num_vertex_buffers; i++) {
      pipe_buffer_unmap(svga->pipe.screen, svga->curr.vb[i].buffer);
      draw_set_mapped_vertex_buffer(draw, i, NULL);
   }

   if (indexBuffer) {
      pipe_buffer_unmap(svga->pipe.screen, indexBuffer);
      draw_set_mapped_element_buffer(draw, 0, NULL);
   }

   if (svga->curr.cb[PIPE_SHADER_VERTEX]) {
      pipe_buffer_unmap(svga->pipe.screen,
                        svga->curr.cb[PIPE_SHADER_VERTEX]);
   }

   return ret;
}




boolean svga_init_swtnl( struct svga_context *svga )
{
   svga->swtnl.backend = svga_vbuf_render_create(svga);
   if(!svga->swtnl.backend)
      goto fail;

   /*
    * Create drawing context and plug our rendering stage into it.
    */
   svga->swtnl.draw = draw_create(&svga->pipe);
   if (svga->swtnl.draw == NULL)
      goto fail;


   draw_set_rasterize_stage(svga->swtnl.draw, 
                            draw_vbuf_stage( svga->swtnl.draw, svga->swtnl.backend ));

   draw_set_render(svga->swtnl.draw, svga->swtnl.backend);

   draw_install_aaline_stage(svga->swtnl.draw, &svga->pipe);
   draw_install_aapoint_stage(svga->swtnl.draw, &svga->pipe);
   draw_install_pstipple_stage(svga->swtnl.draw, &svga->pipe);

   draw_set_driver_clipping(svga->swtnl.draw, debug_get_bool_option("SVGA_SWTNL_FSE", FALSE));

   return TRUE;

fail:
   if (svga->swtnl.backend)
      svga->swtnl.backend->destroy( svga->swtnl.backend );

   if (svga->swtnl.draw)
      draw_destroy( svga->swtnl.draw );

   return FALSE;
}


void svga_destroy_swtnl( struct svga_context *svga )
{
   draw_destroy( svga->swtnl.draw );
}

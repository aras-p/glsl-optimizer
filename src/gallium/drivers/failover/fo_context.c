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


#include "pipe/p_defines.h"
#include "util/u_memory.h"
#include "pipe/p_context.h"
#include "util/u_inlines.h"

#include "fo_context.h"
#include "fo_winsys.h"



static void failover_destroy( struct pipe_context *pipe )
{
   struct failover_context *failover = failover_context( pipe );
   unsigned i;

   for (i = 0; i < failover->num_vertex_buffers; i++) {
      pipe_resource_reference(&failover->vertex_buffers[i].buffer, NULL);
   }

   FREE( failover );
}


void failover_fail_over( struct failover_context *failover )
{
   failover->dirty = TRUE;
   failover->mode = FO_SW;
}


static void failover_draw_vbo( struct pipe_context *pipe,
                               const struct pipe_draw_info *info)
{
   struct failover_context *failover = failover_context( pipe );

   /* If there has been any statechange since last time, try hardware
    * rendering again:
    */
   if (failover->dirty) {
      failover->mode = FO_HW;
   }

   /* Try hardware:
    */
   if (failover->mode == FO_HW) {
      failover->hw->draw_vbo( failover->hw, info );
   }

   /* Possibly try software:
    */
   if (failover->mode == FO_SW) {

      if (failover->dirty) {
         failover->hw->flush( failover->hw, NULL );
	 failover_state_emit( failover );
      }

      failover->sw->draw_vbo( failover->sw, info );

      /* Be ready to switch back to hardware rendering without an
       * intervening flush.  Unlikely to be much performance impact to
       * this:
       */
      failover->sw->flush( failover->sw, NULL );
   }
}

struct pipe_context *failover_create( struct pipe_context *hw,
				      struct pipe_context *sw )
{
   struct failover_context *failover = CALLOC_STRUCT(failover_context);
   if (failover == NULL)
      return NULL;

   failover->hw = hw;
   failover->sw = sw;
   failover->pipe.winsys = hw->winsys;
   failover->pipe.screen = hw->screen;
   failover->pipe.destroy = failover_destroy;
#if 0
   failover->pipe.is_format_supported = hw->is_format_supported;
   failover->pipe.get_name = hw->get_name;
   failover->pipe.get_vendor = hw->get_vendor;
   failover->pipe.get_param = hw->get_param;
   failover->pipe.get_shader_param = hw->get_shader_param;
   failover->pipe.get_paramf = hw->get_paramf;
#endif

   failover->pipe.draw_vbo = failover_draw_vbo;
   failover->pipe.clear = hw->clear;
   failover->pipe.clear_render_target = hw->clear_render_target;
   failover->pipe.clear_depth_stencil = hw->clear_depth_stencil;

   /* No software occlusion fallback (or other optional functionality)
    * at this point - if the hardware doesn't support it, don't
    * advertise it to the application.
    */
   failover->pipe.begin_query = hw->begin_query;
   failover->pipe.end_query = hw->end_query;

   failover_init_state_functions( failover );

   failover->pipe.resource_copy_region = hw->resource_copy_region;

#if 0
   failover->pipe.resource_create = hw->resource_create;
   failover->pipe.resource_destroy = hw->resource_destroy;
   failover->pipe.create_surface = hw->create_surface;
   failover->pipe.surface_destroy = hw->surface_destroy;
#endif

   failover->pipe.flush = hw->flush;

   failover->dirty = 0;

   return &failover->pipe;
}


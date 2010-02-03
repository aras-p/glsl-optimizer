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
#include "util/u_simple_screen.h"
#include "util/u_memory.h"
#include "pipe/p_context.h"

#include "fo_context.h"
#include "fo_winsys.h"



static void failover_destroy( struct pipe_context *pipe )
{
   struct failover_context *failover = failover_context( pipe );

   free( failover );
}


void failover_fail_over( struct failover_context *failover )
{
   failover->dirty = TRUE;
   failover->mode = FO_SW;
}


static void failover_draw_elements( struct pipe_context *pipe,
                                    struct pipe_buffer *indexBuffer,
                                    unsigned indexSize,
                                    unsigned prim, 
                                    unsigned start, 
                                    unsigned count)
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
      failover->hw->draw_elements( failover->hw, 
                                   indexBuffer, 
                                   indexSize, 
                                   prim, 
                                   start, 
                                   count );
   }

   /* Possibly try software:
    */
   if (failover->mode == FO_SW) {

      if (failover->dirty) {
         failover->hw->flush( failover->hw, ~0, NULL );
	 failover_state_emit( failover );
      }

      failover->sw->draw_elements( failover->sw, 
				   indexBuffer, 
				   indexSize, 
				   prim, 
				   start, 
				   count );

      /* Be ready to switch back to hardware rendering without an
       * intervening flush.  Unlikely to be much performance impact to
       * this:
       */
      failover->sw->flush( failover->sw, ~0, NULL );
   }
}


static void failover_draw_arrays( struct pipe_context *pipe,
				     unsigned prim, unsigned start, unsigned count)
{
   failover_draw_elements(pipe, NULL, 0, prim, start, count);
}

static unsigned int
failover_is_texture_referenced( struct pipe_context *_pipe,
				struct pipe_texture *texture,
				unsigned face, unsigned level)
{
   struct failover_context *failover = failover_context( _pipe );
   struct pipe_context *pipe = (failover->mode == FO_HW) ?
      failover->hw : failover->sw;

   return pipe->is_texture_referenced(pipe, texture, face, level);
}

static unsigned int
failover_is_buffer_referenced( struct pipe_context *_pipe,
			       struct pipe_buffer *buf)
{
   struct failover_context *failover = failover_context( _pipe );
   struct pipe_context *pipe = (failover->mode == FO_HW) ?
      failover->hw : failover->sw;

   return pipe->is_buffer_referenced(pipe, buf);
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
   failover->pipe.get_paramf = hw->get_paramf;
#endif

   failover->pipe.draw_arrays = failover_draw_arrays;
   failover->pipe.draw_elements = failover_draw_elements;
   failover->pipe.clear = hw->clear;

   /* No software occlusion fallback (or other optional functionality)
    * at this point - if the hardware doesn't support it, don't
    * advertise it to the application.
    */
   failover->pipe.begin_query = hw->begin_query;
   failover->pipe.end_query = hw->end_query;

   failover_init_state_functions( failover );

   failover->pipe.surface_copy = hw->surface_copy;
   failover->pipe.surface_fill = hw->surface_fill;

#if 0
   failover->pipe.texture_create = hw->texture_create;
   failover->pipe.texture_destroy = hw->texture_destroy;
   failover->pipe.get_tex_surface = hw->get_tex_surface;
   failover->pipe.texture_update = hw->texture_update;
#endif

   failover->pipe.flush = hw->flush;
   failover->pipe.is_texture_referenced = failover_is_texture_referenced;
   failover->pipe.is_buffer_referenced = failover_is_buffer_referenced;

   failover->dirty = 0;

   return &failover->pipe;
}


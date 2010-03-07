/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.
 
 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */


#include "pipe/p_context.h"
#include "util/u_inlines.h"
#include "util/u_simple_list.h"

#include "brw_context.h"
#include "brw_draw.h"
#include "brw_state.h"
#include "brw_batchbuffer.h"
#include "brw_winsys.h"
#include "brw_screen.h"


static void brw_destroy_context( struct pipe_context *pipe )
{
   struct brw_context *brw = brw_context(pipe);
   int i;

   brw_context_flush( brw );
   brw_batchbuffer_free( brw->batch );
   brw_destroy_state(brw);

   brw_draw_cleanup( brw );

   brw_pipe_blend_cleanup( brw );
   brw_pipe_depth_stencil_cleanup( brw );
   brw_pipe_framebuffer_cleanup( brw );
   brw_pipe_flush_cleanup( brw );
   brw_pipe_misc_cleanup( brw );
   brw_pipe_query_cleanup( brw );
   brw_pipe_rast_cleanup( brw );
   brw_pipe_sampler_cleanup( brw );
   brw_pipe_shader_cleanup( brw );
   brw_pipe_vertex_cleanup( brw );
   brw_pipe_clear_cleanup( brw );

   brw_hw_cc_cleanup( brw );


   FREE(brw->wm.compile_data);

   for (i = 0; i < brw->curr.fb.nr_cbufs; i++)
      pipe_surface_reference(&brw->curr.fb.cbufs[i], NULL);
   brw->curr.fb.nr_cbufs = 0;
   pipe_surface_reference(&brw->curr.fb.zsbuf, NULL);

   bo_reference(&brw->curbe.curbe_bo, NULL);
   bo_reference(&brw->vs.prog_bo, NULL);
   bo_reference(&brw->vs.state_bo, NULL);
   bo_reference(&brw->vs.bind_bo, NULL);
   bo_reference(&brw->gs.prog_bo, NULL);
   bo_reference(&brw->gs.state_bo, NULL);
   bo_reference(&brw->clip.prog_bo, NULL);
   bo_reference(&brw->clip.state_bo, NULL);
   bo_reference(&brw->clip.vp_bo, NULL);
   bo_reference(&brw->sf.prog_bo, NULL);
   bo_reference(&brw->sf.state_bo, NULL);
   bo_reference(&brw->sf.vp_bo, NULL);

   for (i = 0; i < Elements(brw->wm.sdc_bo); i++)
      bo_reference(&brw->wm.sdc_bo[i], NULL);

   bo_reference(&brw->wm.bind_bo, NULL);

   for (i = 0; i < Elements(brw->wm.surf_bo); i++)
      bo_reference(&brw->wm.surf_bo[i], NULL);

   bo_reference(&brw->wm.sampler_bo, NULL);
   bo_reference(&brw->wm.prog_bo, NULL);
   bo_reference(&brw->wm.state_bo, NULL);
}


struct pipe_context *brw_create_context(struct pipe_screen *screen,
					void *priv)
{
   struct brw_context *brw = (struct brw_context *) CALLOC_STRUCT(brw_context);

   if (!brw) {
      debug_printf("%s: failed to alloc context\n", __FUNCTION__);
      return NULL;
   }

   brw->base.screen = screen;
   brw->base.priv = priv;
   brw->base.destroy = brw_destroy_context;
   brw->sws = brw_screen(screen)->sws;
   brw->chipset = brw_screen(screen)->chipset;

   brw_pipe_blend_init( brw );
   brw_pipe_depth_stencil_init( brw );
   brw_pipe_framebuffer_init( brw );
   brw_pipe_flush_init( brw );
   brw_pipe_misc_init( brw );
   brw_pipe_query_init( brw );
   brw_pipe_rast_init( brw );
   brw_pipe_sampler_init( brw );
   brw_pipe_shader_init( brw );
   brw_pipe_vertex_init( brw );
   brw_pipe_clear_init( brw );

   brw_hw_cc_init( brw );

   brw_init_state( brw );
   brw_draw_init( brw );

   brw->state.dirty.mesa = ~0;
   brw->state.dirty.brw = ~0;

   brw->flags.always_emit_state = 0;

   make_empty_list(&brw->query.active_head);

   brw->batch = brw_batchbuffer_alloc( brw->sws, brw->chipset );
   if (brw->batch == NULL)
      goto fail;

   return &brw->base;

fail:
   if (brw->batch)
      brw_batchbuffer_free( brw->batch );
   return NULL;
}


/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * Copyright 2008 VMware, Inc.  All rights reserved.
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
 *    Keith Whitwell <keith@tungstengraphics.com>
 */

#include "draw/draw_context.h"
#include "draw/draw_vbuf.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "lp_clear.h"
#include "lp_context.h"
#include "lp_flush.h"
#include "lp_perf.h"
#include "lp_state.h"
#include "lp_surface.h"
#include "lp_query.h"
#include "lp_setup.h"





static void llvmpipe_destroy( struct pipe_context *pipe )
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context( pipe );
   uint i;

   lp_print_counters();

   /* This will also destroy llvmpipe->setup:
    */
   if (llvmpipe->draw)
      draw_destroy( llvmpipe->draw );

   for (i = 0; i < PIPE_MAX_COLOR_BUFS; i++) {
      pipe_surface_reference(&llvmpipe->framebuffer.cbufs[i], NULL);
   }

   pipe_surface_reference(&llvmpipe->framebuffer.zsbuf, NULL);

   for (i = 0; i < PIPE_MAX_SAMPLERS; i++) {
      pipe_texture_reference(&llvmpipe->texture[i], NULL);
   }

   for (i = 0; i < PIPE_MAX_VERTEX_SAMPLERS; i++) {
      pipe_texture_reference(&llvmpipe->vertex_textures[i], NULL);
   }

   for (i = 0; i < Elements(llvmpipe->constants); i++) {
      if (llvmpipe->constants[i]) {
         pipe_buffer_reference(&llvmpipe->constants[i], NULL);
      }
   }

   align_free( llvmpipe );
}

static unsigned int
llvmpipe_is_texture_referenced( struct pipe_context *pipe,
				struct pipe_texture *texture,
				unsigned face, unsigned level)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context( pipe );

   return lp_setup_is_texture_referenced(llvmpipe->setup, texture);
}

static unsigned int
llvmpipe_is_buffer_referenced( struct pipe_context *pipe,
			       struct pipe_buffer *buf)
{
   return PIPE_UNREFERENCED;
}

struct pipe_context *
llvmpipe_create_context( struct pipe_screen *screen, void *priv )
{
   struct llvmpipe_context *llvmpipe;

   llvmpipe = align_malloc(sizeof(struct llvmpipe_context), 16);
   if (!llvmpipe)
      return NULL;

   util_init_math();

   memset(llvmpipe, 0, sizeof *llvmpipe);

   llvmpipe->pipe.winsys = screen->winsys;
   llvmpipe->pipe.screen = screen;
   llvmpipe->pipe.priv = priv;
   llvmpipe->pipe.destroy = llvmpipe_destroy;

   /* state setters */
   llvmpipe->pipe.create_blend_state = llvmpipe_create_blend_state;
   llvmpipe->pipe.bind_blend_state   = llvmpipe_bind_blend_state;
   llvmpipe->pipe.delete_blend_state = llvmpipe_delete_blend_state;

   llvmpipe->pipe.create_sampler_state = llvmpipe_create_sampler_state;
   llvmpipe->pipe.bind_fragment_sampler_states  = llvmpipe_bind_sampler_states;
   llvmpipe->pipe.bind_vertex_sampler_states  = llvmpipe_bind_vertex_sampler_states;
   llvmpipe->pipe.delete_sampler_state = llvmpipe_delete_sampler_state;

   llvmpipe->pipe.create_depth_stencil_alpha_state = llvmpipe_create_depth_stencil_state;
   llvmpipe->pipe.bind_depth_stencil_alpha_state   = llvmpipe_bind_depth_stencil_state;
   llvmpipe->pipe.delete_depth_stencil_alpha_state = llvmpipe_delete_depth_stencil_state;

   llvmpipe->pipe.create_rasterizer_state = llvmpipe_create_rasterizer_state;
   llvmpipe->pipe.bind_rasterizer_state   = llvmpipe_bind_rasterizer_state;
   llvmpipe->pipe.delete_rasterizer_state = llvmpipe_delete_rasterizer_state;

   llvmpipe->pipe.create_fs_state = llvmpipe_create_fs_state;
   llvmpipe->pipe.bind_fs_state   = llvmpipe_bind_fs_state;
   llvmpipe->pipe.delete_fs_state = llvmpipe_delete_fs_state;

   llvmpipe->pipe.create_vs_state = llvmpipe_create_vs_state;
   llvmpipe->pipe.bind_vs_state   = llvmpipe_bind_vs_state;
   llvmpipe->pipe.delete_vs_state = llvmpipe_delete_vs_state;

   llvmpipe->pipe.set_blend_color = llvmpipe_set_blend_color;
   llvmpipe->pipe.set_stencil_ref = llvmpipe_set_stencil_ref;
   llvmpipe->pipe.set_clip_state = llvmpipe_set_clip_state;
   llvmpipe->pipe.set_constant_buffer = llvmpipe_set_constant_buffer;
   llvmpipe->pipe.set_framebuffer_state = llvmpipe_set_framebuffer_state;
   llvmpipe->pipe.set_polygon_stipple = llvmpipe_set_polygon_stipple;
   llvmpipe->pipe.set_scissor_state = llvmpipe_set_scissor_state;
   llvmpipe->pipe.set_fragment_sampler_textures = llvmpipe_set_sampler_textures;
   llvmpipe->pipe.set_vertex_sampler_textures = llvmpipe_set_vertex_sampler_textures;
   llvmpipe->pipe.set_viewport_state = llvmpipe_set_viewport_state;

   llvmpipe->pipe.set_vertex_buffers = llvmpipe_set_vertex_buffers;
   llvmpipe->pipe.set_vertex_elements = llvmpipe_set_vertex_elements;

   llvmpipe->pipe.draw_arrays = llvmpipe_draw_arrays;
   llvmpipe->pipe.draw_elements = llvmpipe_draw_elements;
   llvmpipe->pipe.draw_range_elements = llvmpipe_draw_range_elements;

   llvmpipe->pipe.clear = llvmpipe_clear;
   llvmpipe->pipe.flush = llvmpipe_flush;

   llvmpipe->pipe.is_texture_referenced = llvmpipe_is_texture_referenced;
   llvmpipe->pipe.is_buffer_referenced = llvmpipe_is_buffer_referenced;

   llvmpipe_init_query_funcs( llvmpipe );

   /*
    * Create drawing context and plug our rendering stage into it.
    */
   llvmpipe->draw = draw_create(&llvmpipe->pipe);
   if (!llvmpipe->draw) 
      goto fail;

   /* FIXME: devise alternative to draw_texture_samplers */

   if (debug_get_bool_option( "LP_NO_RAST", FALSE ))
      llvmpipe->no_rast = TRUE;

   llvmpipe->setup = lp_setup_create( &llvmpipe->pipe,
                                      llvmpipe->draw );
   if (!llvmpipe->setup)
      goto fail;

   /* plug in AA line/point stages */
   draw_install_aaline_stage(llvmpipe->draw, &llvmpipe->pipe);
   draw_install_aapoint_stage(llvmpipe->draw, &llvmpipe->pipe);

#if USE_DRAW_STAGE_PSTIPPLE
   /* Do polygon stipple w/ texture map + frag prog? */
   draw_install_pstipple_stage(llvmpipe->draw, &llvmpipe->pipe);
#endif

   lp_init_surface_functions(llvmpipe);

   lp_reset_counters();

   return &llvmpipe->pipe;

 fail:
   llvmpipe_destroy(&llvmpipe->pipe);
   return NULL;
}


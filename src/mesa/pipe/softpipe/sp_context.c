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
 *    Keith Whitwell <keith@tungstengraphics.com>
 */

#include "main/imports.h"
#include "main/macros.h"
#include "pipe/draw/draw_context.h"
#include "sp_context.h"
#include "sp_clear.h"
#include "sp_state.h"
#include "sp_prim_setup.h"


static void softpipe_destroy( struct pipe_context *pipe )
{
   struct softpipe_context *softpipe = softpipe_context( pipe );

   draw_destroy( softpipe->draw );

   free( softpipe );
}


static void softpipe_draw_vb( struct pipe_context *pipe,
			     struct vertex_buffer *VB )
{
   struct softpipe_context *softpipe = softpipe_context( pipe );

   if (softpipe->dirty)
      softpipe_update_derived( softpipe );

   draw_vb( softpipe->draw, VB );
}


struct pipe_context *softpipe_create( void )
{
   struct softpipe_context *softpipe = CALLOC_STRUCT(softpipe_context);

   softpipe->pipe.destroy = softpipe_destroy;
   softpipe->pipe.set_alpha_test_state = softpipe_set_alpha_test_state;
   softpipe->pipe.set_blend_color = softpipe_set_blend_color;
   softpipe->pipe.set_blend_state = softpipe_set_blend_state;
   softpipe->pipe.set_clip_state = softpipe_set_clip_state;
   softpipe->pipe.set_clear_color_state = softpipe_set_clear_color_state;
   softpipe->pipe.set_depth_state = softpipe_set_depth_test_state;
   softpipe->pipe.set_framebuffer_state = softpipe_set_framebuffer_state;
   softpipe->pipe.set_fs_state = softpipe_set_fs_state;
   softpipe->pipe.set_polygon_stipple = softpipe_set_polygon_stipple;
   softpipe->pipe.set_sampler_state = softpipe_set_sampler_state;
   softpipe->pipe.set_scissor_state = softpipe_set_scissor_state;
   softpipe->pipe.set_setup_state = softpipe_set_setup_state;
   softpipe->pipe.set_stencil_state = softpipe_set_stencil_state;
   softpipe->pipe.set_texture_state = softpipe_set_texture_state;
   softpipe->pipe.set_viewport_state = softpipe_set_viewport_state;
   softpipe->pipe.draw_vb = softpipe_draw_vb;
   softpipe->pipe.clear = softpipe_clear;

   softpipe->quad.polygon_stipple = sp_quad_polygon_stipple_stage(softpipe);
   softpipe->quad.shade = sp_quad_shade_stage(softpipe);
   softpipe->quad.alpha_test = sp_quad_alpha_test_stage(softpipe);
   softpipe->quad.blend = sp_quad_blend_stage(softpipe);
   softpipe->quad.depth_test = sp_quad_depth_test_stage(softpipe);
   softpipe->quad.stencil_test = sp_quad_stencil_test_stage(softpipe);
   softpipe->quad.output = sp_quad_output_stage(softpipe);

   /*
    * Create drawing context and plug our render/setup stage into it.
    */
   softpipe->draw = draw_create();
   draw_set_setup_stage(softpipe->draw, prim_setup(softpipe));

   return &softpipe->pipe;
}

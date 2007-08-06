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
#include "pipe/p_defines.h"
#include "sp_buffer.h"
#include "sp_clear.h"
#include "sp_context.h"
#include "sp_prim_setup.h"
#include "sp_region.h"
#include "sp_state.h"
#include "sp_surface.h"
#include "sp_tex_layout.h"
#include "sp_winsys.h"



/**
 * Return list of supported surface/texture formats.
 * If we find texture and drawable support differs, add a selector
 * parameter or another function.
 */
static const GLuint *
softpipe_supported_formats(struct pipe_context *pipe, GLuint *numFormats)
{
   static const GLuint supported[] = {
      PIPE_FORMAT_U_R8_G8_B8_A8,
      PIPE_FORMAT_U_A8_R8_G8_B8,
      PIPE_FORMAT_U_R5_G6_B5,
      PIPE_FORMAT_U_L8,
      PIPE_FORMAT_U_A8,
      PIPE_FORMAT_U_I8,
      PIPE_FORMAT_U_L8_A8,
      PIPE_FORMAT_S_R16_G16_B16_A16,
      PIPE_FORMAT_YCBCR,
      PIPE_FORMAT_YCBCR_REV,
      PIPE_FORMAT_U_Z16,
      PIPE_FORMAT_U_Z32,
      PIPE_FORMAT_F_Z32,
      PIPE_FORMAT_S8_Z24,
      PIPE_FORMAT_U_S8
   };

   *numFormats = sizeof(supported)/sizeof(supported[0]);
   return supported;
}



static void map_surfaces(struct softpipe_context *sp)
{
   struct pipe_context *pipe = &sp->pipe;
   GLuint i;

   for (i = 0; i < sp->framebuffer.num_cbufs; i++) {
      struct softpipe_surface *sps = softpipe_surface(sp->framebuffer.cbufs[i]);      pipe->region_map(pipe, sps->surface.region);
   }

   if (sp->framebuffer.zbuf) {
      struct softpipe_surface *sps = softpipe_surface(sp->framebuffer.zbuf);
      pipe->region_map(pipe, sps->surface.region);
   }

   /* XXX depth & stencil bufs */
}


static void unmap_surfaces(struct softpipe_context *sp)
{
   struct pipe_context *pipe = &sp->pipe;
   GLuint i;

   for (i = 0; i < sp->framebuffer.num_cbufs; i++) {
      struct softpipe_surface *sps = softpipe_surface(sp->framebuffer.cbufs[i]);
      pipe->region_unmap(pipe, sps->surface.region);
   }

   if (sp->framebuffer.zbuf) {
      struct softpipe_surface *sps = softpipe_surface(sp->framebuffer.zbuf);
      pipe->region_unmap(pipe, sps->surface.region);
   }
   /* XXX depth & stencil bufs */
}


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

   /* XXX move mapping/unmapping to higher/coarser level? */
   map_surfaces(softpipe);
   draw_vb( softpipe->draw, VB );
   unmap_surfaces(softpipe);
}


static void
softpipe_draw_vertices(struct pipe_context *pipe,
                       GLuint mode,
                       GLuint numVertex, const GLfloat *verts,
                       GLuint numAttribs, const GLuint attribs[])
{
   struct softpipe_context *softpipe = softpipe_context( pipe );

   if (softpipe->dirty)
      softpipe_update_derived( softpipe );

   /* XXX move mapping/unmapping to higher/coarser level? */
   map_surfaces(softpipe);
   draw_vertices(softpipe->draw, mode, numVertex, verts, numAttribs, attribs);
   unmap_surfaces(softpipe);
}



static void softpipe_reset_occlusion_counter(struct pipe_context *pipe)
{
   struct softpipe_context *softpipe = softpipe_context( pipe );
   softpipe->occlusion_counter = 0;
}

/* XXX pipe param should be const */
static GLuint softpipe_get_occlusion_counter(struct pipe_context *pipe)
{
   struct softpipe_context *softpipe = softpipe_context( pipe );
   return softpipe->occlusion_counter;
}


struct pipe_context *softpipe_create( struct softpipe_winsys *sws )
{
   struct softpipe_context *softpipe = CALLOC_STRUCT(softpipe_context);

   softpipe->pipe.destroy = softpipe_destroy;

   softpipe->pipe.supported_formats = softpipe_supported_formats;

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
   softpipe->pipe.draw_vertices = softpipe_draw_vertices;
   softpipe->pipe.clear = softpipe_clear;
   softpipe->pipe.reset_occlusion_counter = softpipe_reset_occlusion_counter;
   softpipe->pipe.get_occlusion_counter = softpipe_get_occlusion_counter;

   softpipe->pipe.mipmap_tree_layout = softpipe_mipmap_tree_layout;

   softpipe->quad.polygon_stipple = sp_quad_polygon_stipple_stage(softpipe);
   softpipe->quad.shade = sp_quad_shade_stage(softpipe);
   softpipe->quad.alpha_test = sp_quad_alpha_test_stage(softpipe);
   softpipe->quad.depth_test = sp_quad_depth_test_stage(softpipe);
   softpipe->quad.stencil_test = sp_quad_stencil_test_stage(softpipe);
   softpipe->quad.occlusion = sp_quad_occlusion_stage(softpipe);
   softpipe->quad.coverage = sp_quad_coverage_stage(softpipe);
   softpipe->quad.bufloop = sp_quad_bufloop_stage(softpipe);
   softpipe->quad.blend = sp_quad_blend_stage(softpipe);
   softpipe->quad.colormask = sp_quad_colormask_stage(softpipe);
   softpipe->quad.output = sp_quad_output_stage(softpipe);

   softpipe->winsys = sws;

   /*
    * Create drawing context and plug our rendering stage into it.
    */
   softpipe->draw = draw_create();
   assert(softpipe->draw);
   draw_set_setup_stage(softpipe->draw, sp_draw_render_stage(softpipe));

   sp_init_buffer_functions(softpipe);
   sp_init_region_functions(softpipe);
   sp_init_surface_functions(softpipe);

   /*
    * XXX we could plug GL selection/feedback into the drawing pipeline
    * by specifying a different setup/render stage.
    */

   return &softpipe->pipe;
}

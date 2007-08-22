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

#include "pipe/draw/draw_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_util.h"
#include "sp_clear.h"
#include "sp_context.h"
#include "sp_flush.h"
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
static const unsigned *
softpipe_supported_formats(struct pipe_context *pipe, unsigned *numFormats)
{
#if 0
   static const unsigned supported[] = {
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
#else
   struct softpipe_context *softpipe = softpipe_context( pipe );
   return softpipe->winsys->supported_formats( softpipe->winsys, numFormats );
#endif
}


/** XXX remove these? */
#define MAX_TEXTURE_LEVELS 11
#define MAX_TEXTURE_RECT_SIZE 2048
#define MAX_3D_TEXTURE_LEVELS 8

static void
softpipe_max_texture_size(struct pipe_context *pipe, unsigned textureType,
                          unsigned *maxWidth, unsigned *maxHeight,
                          unsigned *maxDepth)
{
   switch (textureType) {
   case PIPE_TEXTURE_1D:
      *maxWidth = 1 << (MAX_TEXTURE_LEVELS - 1);
      break;
   case PIPE_TEXTURE_2D:
      *maxWidth =
      *maxHeight = 1 << (MAX_TEXTURE_LEVELS - 1);
      break;
   case PIPE_TEXTURE_3D:
      *maxWidth =
      *maxHeight =
      *maxDepth = 1 << (MAX_3D_TEXTURE_LEVELS - 1);
      break;
   case PIPE_TEXTURE_CUBE:
      *maxWidth =
      *maxHeight = MAX_TEXTURE_RECT_SIZE;
      break;
   default:
      assert(0);
   }
}


void
softpipe_map_surfaces(struct softpipe_context *sp)
{
   struct pipe_context *pipe = &sp->pipe;
   unsigned i;

   for (i = 0; i < sp->framebuffer.num_cbufs; i++) {
      struct softpipe_surface *sps = softpipe_surface(sp->framebuffer.cbufs[i]);
      if (sps->surface.region)
         pipe->region_map(pipe, sps->surface.region);
   }

   if (sp->framebuffer.zbuf) {
      struct softpipe_surface *sps = softpipe_surface(sp->framebuffer.zbuf);
      if (sps->surface.region)
         pipe->region_map(pipe, sps->surface.region);
   }

   if (sp->framebuffer.sbuf) {
      struct softpipe_surface *sps = softpipe_surface(sp->framebuffer.sbuf);
      if (sps->surface.region)
         pipe->region_map(pipe, sps->surface.region);
   }

   /* textures */
   for (i = 0; i < PIPE_MAX_SAMPLERS; i++) {
      struct pipe_mipmap_tree *mt = sp->texture[i];
      if (mt) {
         pipe->region_map(pipe, mt->region);
      }
   }

   /* XXX depth & stencil bufs */
}


void
softpipe_unmap_surfaces(struct softpipe_context *sp)
{
   struct pipe_context *pipe = &sp->pipe;
   unsigned i;

   for (i = 0; i < sp->framebuffer.num_cbufs; i++) {
      struct softpipe_surface *sps = softpipe_surface(sp->framebuffer.cbufs[i]);
      if (sps->surface.region)
         pipe->region_unmap(pipe, sps->surface.region);
   }

   if (sp->framebuffer.zbuf) {
      struct softpipe_surface *sps = softpipe_surface(sp->framebuffer.zbuf);
      if (sps->surface.region)
         pipe->region_unmap(pipe, sps->surface.region);
   }

   if (sp->framebuffer.sbuf) {
      struct softpipe_surface *sps = softpipe_surface(sp->framebuffer.sbuf);
      if (sps->surface.region)
         pipe->region_unmap(pipe, sps->surface.region);
   }

   /* textures */
   for (i = 0; i < PIPE_MAX_SAMPLERS; i++) {
      struct pipe_mipmap_tree *mt = sp->texture[i];
      if (mt) {
         pipe->region_unmap(pipe, mt->region);
      }
   }

   /* XXX depth & stencil bufs */
}


static void softpipe_destroy( struct pipe_context *pipe )
{
   struct softpipe_context *softpipe = softpipe_context( pipe );

   draw_destroy( softpipe->draw );

   free( softpipe );
}


static void softpipe_reset_occlusion_counter(struct pipe_context *pipe)
{
   struct softpipe_context *softpipe = softpipe_context( pipe );
   softpipe->occlusion_counter = 0;
}

/* XXX pipe param should be const */
static unsigned softpipe_get_occlusion_counter(struct pipe_context *pipe)
{
   struct softpipe_context *softpipe = softpipe_context( pipe );
   return softpipe->occlusion_counter;
}

static const char *softpipe_get_name( struct pipe_context *pipe )
{
   return "softpipe";
}

static const char *softpipe_get_vendor( struct pipe_context *pipe )
{
   return "Tungsten Graphics, Inc.";
}


struct pipe_context *softpipe_create( struct pipe_winsys *pipe_winsys,
				      struct softpipe_winsys *softpipe_winsys )
{
   struct softpipe_context *softpipe = CALLOC_STRUCT(softpipe_context);

   softpipe->pipe.winsys = pipe_winsys;
   softpipe->pipe.destroy = softpipe_destroy;

   /* queries */
   softpipe->pipe.supported_formats = softpipe_supported_formats;
   softpipe->pipe.max_texture_size = softpipe_max_texture_size;

   /* state setters */
   softpipe->pipe.set_alpha_test_state = softpipe_set_alpha_test_state;
   softpipe->pipe.set_blend_color = softpipe_set_blend_color;
   softpipe->pipe.set_blend_state = softpipe_set_blend_state;
   softpipe->pipe.set_clip_state = softpipe_set_clip_state;
   softpipe->pipe.set_clear_color_state = softpipe_set_clear_color_state;
   softpipe->pipe.set_constant_buffer = softpipe_set_constant_buffer;
   softpipe->pipe.set_depth_state = softpipe_set_depth_test_state;
   softpipe->pipe.set_framebuffer_state = softpipe_set_framebuffer_state;
   softpipe->pipe.set_fs_state = softpipe_set_fs_state;
   softpipe->pipe.set_vs_state = softpipe_set_vs_state;
   softpipe->pipe.set_polygon_stipple = softpipe_set_polygon_stipple;
   softpipe->pipe.set_sampler_state = softpipe_set_sampler_state;
   softpipe->pipe.set_scissor_state = softpipe_set_scissor_state;
   softpipe->pipe.set_setup_state = softpipe_set_setup_state;
   softpipe->pipe.set_stencil_state = softpipe_set_stencil_state;
   softpipe->pipe.set_texture_state = softpipe_set_texture_state;
   softpipe->pipe.set_viewport_state = softpipe_set_viewport_state;
   softpipe->pipe.set_vertex_buffer = softpipe_set_vertex_buffer;
   softpipe->pipe.set_vertex_element = softpipe_set_vertex_element;

   softpipe->pipe.draw_arrays = softpipe_draw_arrays;
   softpipe->pipe.draw_elements = softpipe_draw_elements;

   softpipe->pipe.clear = softpipe_clear;
   softpipe->pipe.flush = softpipe_flush;
   softpipe->pipe.reset_occlusion_counter = softpipe_reset_occlusion_counter;
   softpipe->pipe.get_occlusion_counter = softpipe_get_occlusion_counter;
   softpipe->pipe.get_name = softpipe_get_name;
   softpipe->pipe.get_vendor = softpipe_get_vendor;

   /* textures */
   softpipe->pipe.mipmap_tree_layout = softpipe_mipmap_tree_layout;
   softpipe->pipe.get_tex_surface = softpipe_get_tex_surface;

   /* setup quad rendering stages */
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

   softpipe->winsys = softpipe_winsys;

   /*
    * Create drawing context and plug our rendering stage into it.
    */
   softpipe->draw = draw_create();
   assert(softpipe->draw);
   draw_set_setup_stage(softpipe->draw, sp_draw_render_stage(softpipe));

   sp_init_region_functions(softpipe);
   sp_init_surface_functions(softpipe);

   /*
    * XXX we could plug GL selection/feedback into the drawing pipeline
    * by specifying a different setup/render stage.
    */

   return &softpipe->pipe;
}

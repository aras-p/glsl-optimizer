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
#include "pipe/p_inlines.h"
#include "pipe/p_util.h"
#include "sp_clear.h"
#include "sp_context.h"
#include "sp_flush.h"
#include "sp_prim_setup.h"
#include "sp_state.h"
#include "sp_surface.h"
#include "sp_tile_cache.h"
#include "sp_texture.h"
#include "sp_winsys.h"
#include "sp_query.h"



/**
 * Query format support for creating a texture, drawing surface, etc.
 * \param format  the format to test
 * \param type  one of PIPE_TEXTURE, PIPE_SURFACE, PIPE_SCREEN_SURFACE
 */
static boolean
softpipe_is_format_supported( struct pipe_context *pipe,
                              enum pipe_format format, uint type )
{
   struct softpipe_context *softpipe = softpipe_context( pipe );

   switch (type) {
   case PIPE_TEXTURE:
      /* softpipe supports all texture formats */
      return TRUE;
   case PIPE_SURFACE:
      /* softpipe supports all (off-screen) surface formats */
      return TRUE;
   case PIPE_SCREEN_SURFACE:
      return softpipe->winsys->is_format_supported( softpipe->winsys, format );
   default:
      assert(0);
      return FALSE;
   }
}


/**
 * Map any drawing surfaces which aren't already mapped
 */
void
softpipe_map_surfaces(struct softpipe_context *sp)
{
   struct pipe_surface *ps;
   unsigned i;

   for (i = 0; i < sp->framebuffer.num_cbufs; i++) {
      ps = sp->framebuffer.cbufs[i];
      if (ps->buffer)
         pipe_surface_map(ps);
   }

   ps = sp->framebuffer.zbuf;
   if (ps && ps->buffer)
      pipe_surface_map(ps);

   ps = sp->framebuffer.sbuf;
   if (ps && ps->buffer)
      pipe_surface_map(ps);
}


/**
 * Unmap any mapped drawing surfaces
 */
void
softpipe_unmap_surfaces(struct softpipe_context *sp)
{
   struct pipe_surface *ps;
   uint i;

   for (i = 0; i < sp->framebuffer.num_cbufs; i++)
      sp_flush_tile_cache(sp, sp->cbuf_cache[i]);
   sp_flush_tile_cache(sp, sp->zbuf_cache);
   sp_flush_tile_cache(sp, sp->sbuf_cache);

   for (i = 0; i < sp->framebuffer.num_cbufs; i++) {
      ps = sp->framebuffer.cbufs[i];
      if (ps->map)
         pipe_surface_unmap(ps);
   }

   ps = sp->framebuffer.zbuf;
   if (ps && ps->map)
      pipe_surface_unmap(ps);

   ps = sp->framebuffer.sbuf;
   if (ps && ps->map)
      pipe_surface_unmap(ps);
}


static void softpipe_destroy( struct pipe_context *pipe )
{
   struct softpipe_context *softpipe = softpipe_context( pipe );

   draw_destroy( softpipe->draw );

   softpipe->quad.polygon_stipple->destroy( softpipe->quad.polygon_stipple );
   softpipe->quad.earlyz->destroy( softpipe->quad.earlyz );
   softpipe->quad.shade->destroy( softpipe->quad.shade );
   softpipe->quad.alpha_test->destroy( softpipe->quad.alpha_test );
   softpipe->quad.depth_test->destroy( softpipe->quad.depth_test );
   softpipe->quad.stencil_test->destroy( softpipe->quad.stencil_test );
   softpipe->quad.occlusion->destroy( softpipe->quad.occlusion );
   softpipe->quad.coverage->destroy( softpipe->quad.coverage );
   softpipe->quad.bufloop->destroy( softpipe->quad.bufloop );
   softpipe->quad.blend->destroy( softpipe->quad.blend );
   softpipe->quad.colormask->destroy( softpipe->quad.colormask );
   softpipe->quad.output->destroy( softpipe->quad.output );

   FREE( softpipe );
}


static const char *softpipe_get_name( struct pipe_context *pipe )
{
   return "softpipe";
}

static const char *softpipe_get_vendor( struct pipe_context *pipe )
{
   return "Tungsten Graphics, Inc.";
}

static int softpipe_get_param(struct pipe_context *pipe, int param)
{
   switch (param) {
   case PIPE_CAP_MAX_TEXTURE_IMAGE_UNITS:
      return 8;
   case PIPE_CAP_NPOT_TEXTURES:
      return 1;
   case PIPE_CAP_TWO_SIDED_STENCIL:
      return 1;
   case PIPE_CAP_GLSL:
      return 1;
   case PIPE_CAP_S3TC:
      return 0;
   case PIPE_CAP_ANISOTROPIC_FILTER:
      return 0;
   case PIPE_CAP_POINT_SPRITE:
      return 1;
   case PIPE_CAP_MAX_RENDER_TARGETS:
      return 1;
   case PIPE_CAP_OCCLUSION_QUERY:
      return 1;
   case PIPE_CAP_TEXTURE_SHADOW_MAP:
      return 1;
   case PIPE_CAP_MAX_TEXTURE_2D_LEVELS:
      return 12; /* max 2Kx2K */
   case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
      return 8;  /* max 128x128x128 */
   case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
      return 12; /* max 2Kx2K */
   default:
      return 0;
   }
}

static float softpipe_get_paramf(struct pipe_context *pipe, int param)
{
   switch (param) {
   case PIPE_CAP_MAX_LINE_WIDTH:
      /* fall-through */
   case PIPE_CAP_MAX_LINE_WIDTH_AA:
      return 255.0; /* arbitrary */

   case PIPE_CAP_MAX_POINT_WIDTH:
      /* fall-through */
   case PIPE_CAP_MAX_POINT_WIDTH_AA:
      return 255.0; /* arbitrary */

   case PIPE_CAP_MAX_TEXTURE_ANISOTROPY:
      return 0.0;

   case PIPE_CAP_MAX_TEXTURE_LOD_BIAS:
      return 16.0; /* arbitrary */

   default:
      return 0;
   }
}

struct pipe_context *softpipe_create( struct pipe_winsys *pipe_winsys,
				      struct softpipe_winsys *softpipe_winsys )
{
   struct softpipe_context *softpipe = CALLOC_STRUCT(softpipe_context);
   uint i;

#if defined(__i386__) || defined(__386__)
   softpipe->use_sse = GETENV( "GALLIUM_NOSSE" ) == NULL;
#else
   softpipe->use_sse = FALSE;
#endif

   softpipe->dump_fs = GETENV( "GALLIUM_DUMP_FS" ) != NULL;

   softpipe->pipe.winsys = pipe_winsys;
   softpipe->pipe.destroy = softpipe_destroy;

   /* queries */
   softpipe->pipe.is_format_supported = softpipe_is_format_supported;
   softpipe->pipe.get_name = softpipe_get_name;
   softpipe->pipe.get_vendor = softpipe_get_vendor;
   softpipe->pipe.get_param = softpipe_get_param;
   softpipe->pipe.get_paramf = softpipe_get_paramf;

   /* state setters */
   softpipe->pipe.create_alpha_test_state = softpipe_create_alpha_test_state;
   softpipe->pipe.bind_alpha_test_state   = softpipe_bind_alpha_test_state;
   softpipe->pipe.delete_alpha_test_state = softpipe_delete_alpha_test_state;

   softpipe->pipe.create_blend_state = softpipe_create_blend_state;
   softpipe->pipe.bind_blend_state   = softpipe_bind_blend_state;
   softpipe->pipe.delete_blend_state = softpipe_delete_blend_state;

   softpipe->pipe.create_sampler_state = softpipe_create_sampler_state;
   softpipe->pipe.bind_sampler_state   = softpipe_bind_sampler_state;
   softpipe->pipe.delete_sampler_state = softpipe_delete_sampler_state;

   softpipe->pipe.create_depth_stencil_state = softpipe_create_depth_stencil_state;
   softpipe->pipe.bind_depth_stencil_state   = softpipe_bind_depth_stencil_state;
   softpipe->pipe.delete_depth_stencil_state = softpipe_delete_depth_stencil_state;

   softpipe->pipe.create_rasterizer_state = softpipe_create_rasterizer_state;
   softpipe->pipe.bind_rasterizer_state   = softpipe_bind_rasterizer_state;
   softpipe->pipe.delete_rasterizer_state = softpipe_delete_rasterizer_state;

   softpipe->pipe.create_fs_state = softpipe_create_fs_state;
   softpipe->pipe.bind_fs_state   = softpipe_bind_fs_state;
   softpipe->pipe.delete_fs_state = softpipe_delete_fs_state;

   softpipe->pipe.create_vs_state = softpipe_create_vs_state;
   softpipe->pipe.bind_vs_state   = softpipe_bind_vs_state;
   softpipe->pipe.delete_vs_state = softpipe_delete_vs_state;

   softpipe->pipe.set_blend_color = softpipe_set_blend_color;
   softpipe->pipe.set_clip_state = softpipe_set_clip_state;
   softpipe->pipe.set_constant_buffer = softpipe_set_constant_buffer;
   softpipe->pipe.set_framebuffer_state = softpipe_set_framebuffer_state;
   softpipe->pipe.set_polygon_stipple = softpipe_set_polygon_stipple;
   softpipe->pipe.set_scissor_state = softpipe_set_scissor_state;
   softpipe->pipe.set_sampler_texture = softpipe_set_sampler_texture;
   softpipe->pipe.set_viewport_state = softpipe_set_viewport_state;

   softpipe->pipe.set_vertex_buffer = softpipe_set_vertex_buffer;
   softpipe->pipe.set_vertex_element = softpipe_set_vertex_element;

   softpipe->pipe.draw_arrays = softpipe_draw_arrays;
   softpipe->pipe.draw_elements = softpipe_draw_elements;

   softpipe->pipe.clear = softpipe_clear;
   softpipe->pipe.flush = softpipe_flush;

   softpipe_init_query_funcs( softpipe );

   /* textures */
   softpipe->pipe.texture_create = softpipe_texture_create;
   softpipe->pipe.texture_release = softpipe_texture_release;
   softpipe->pipe.get_tex_surface = softpipe_get_tex_surface;

   /*
    * Alloc caches for accessing drawing surfaces and textures.
    * Must be before quad stage setup!
    */
   for (i = 0; i < PIPE_MAX_COLOR_BUFS; i++)
      softpipe->cbuf_cache[i] = sp_create_tile_cache();
   softpipe->zbuf_cache = sp_create_tile_cache();
   softpipe->sbuf_cache_sep = sp_create_tile_cache();
   softpipe->sbuf_cache = softpipe->sbuf_cache_sep; /* initial value */

   for (i = 0; i < PIPE_MAX_SAMPLERS; i++)
      softpipe->tex_cache[i] = sp_create_tile_cache();


   /* setup quad rendering stages */
   softpipe->quad.polygon_stipple = sp_quad_polygon_stipple_stage(softpipe);
   softpipe->quad.earlyz = sp_quad_earlyz_stage(softpipe);
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
   softpipe->setup = sp_draw_render_stage(softpipe);

   if (GETENV( "SP_VBUF" ) != NULL) {
      softpipe->vbuf = sp_draw_vbuf_stage(softpipe->draw, 
                                          &softpipe->pipe, 
                                          sp_vbuf_setup_draw);

      draw_set_rasterize_stage(softpipe->draw, softpipe->vbuf);
   }
   else {
      draw_set_rasterize_stage(softpipe->draw, softpipe->setup);
   }

   sp_init_surface_functions(softpipe);

   return &softpipe->pipe;
}

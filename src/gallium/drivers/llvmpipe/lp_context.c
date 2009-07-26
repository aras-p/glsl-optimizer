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
#include "pipe/p_defines.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "lp_clear.h"
#include "lp_context.h"
#include "lp_flush.h"
#include "lp_prim_setup.h"
#include "lp_prim_vbuf.h"
#include "lp_state.h"
#include "lp_surface.h"
#include "lp_tile_cache.h"
#include "lp_texture.h"
#include "lp_winsys.h"
#include "lp_query.h"



/**
 * Map any drawing surfaces which aren't already mapped
 */
void
llvmpipe_map_transfers(struct llvmpipe_context *lp)
{
   unsigned i;

   for (i = 0; i < lp->framebuffer.nr_cbufs; i++) {
      lp_tile_cache_map_transfers(lp->cbuf_cache[i]);
   }

   lp_tile_cache_map_transfers(lp->zsbuf_cache);
}


/**
 * Unmap any mapped drawing surfaces
 */
void
llvmpipe_unmap_transfers(struct llvmpipe_context *lp)
{
   uint i;

   for (i = 0; i < lp->framebuffer.nr_cbufs; i++)
      lp_flush_tile_cache(lp, lp->cbuf_cache[i]);
   lp_flush_tile_cache(lp, lp->zsbuf_cache);

   for (i = 0; i < lp->framebuffer.nr_cbufs; i++) {
      lp_tile_cache_unmap_transfers(lp->cbuf_cache[i]);
   }
   lp_tile_cache_unmap_transfers(lp->zsbuf_cache);
}


static void llvmpipe_destroy( struct pipe_context *pipe )
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context( pipe );
   uint i;

   if (llvmpipe->draw)
      draw_destroy( llvmpipe->draw );

   for (i = 0; i < SP_NUM_QUAD_THREADS; i++) {
      llvmpipe->quad[i].polygon_stipple->destroy( llvmpipe->quad[i].polygon_stipple );
      llvmpipe->quad[i].earlyz->destroy( llvmpipe->quad[i].earlyz );
      llvmpipe->quad[i].shade->destroy( llvmpipe->quad[i].shade );
      llvmpipe->quad[i].alpha_test->destroy( llvmpipe->quad[i].alpha_test );
      llvmpipe->quad[i].depth_test->destroy( llvmpipe->quad[i].depth_test );
      llvmpipe->quad[i].stencil_test->destroy( llvmpipe->quad[i].stencil_test );
      llvmpipe->quad[i].occlusion->destroy( llvmpipe->quad[i].occlusion );
      llvmpipe->quad[i].coverage->destroy( llvmpipe->quad[i].coverage );
      llvmpipe->quad[i].blend->destroy( llvmpipe->quad[i].blend );
      llvmpipe->quad[i].colormask->destroy( llvmpipe->quad[i].colormask );
      llvmpipe->quad[i].output->destroy( llvmpipe->quad[i].output );
   }

   for (i = 0; i < PIPE_MAX_COLOR_BUFS; i++)
      lp_destroy_tile_cache(llvmpipe->cbuf_cache[i]);
   lp_destroy_tile_cache(llvmpipe->zsbuf_cache);

   for (i = 0; i < PIPE_MAX_SAMPLERS; i++)
      lp_destroy_tile_cache(llvmpipe->tex_cache[i]);

   for (i = 0; i < Elements(llvmpipe->constants); i++) {
      if (llvmpipe->constants[i].buffer) {
         pipe_buffer_reference(&llvmpipe->constants[i].buffer, NULL);
      }
   }

   FREE( llvmpipe );
}

static unsigned int
llvmpipe_is_texture_referenced( struct pipe_context *pipe,
				struct pipe_texture *texture,
				unsigned face, unsigned level)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context( pipe );
   unsigned i;

   if(llvmpipe->dirty_render_cache) {
      for (i = 0; i < llvmpipe->framebuffer.nr_cbufs; i++) {
         if(llvmpipe->framebuffer.cbufs[i] && 
            llvmpipe->framebuffer.cbufs[i]->texture == texture)
            return PIPE_REFERENCED_FOR_WRITE;
      }
      if(llvmpipe->framebuffer.zsbuf && 
         llvmpipe->framebuffer.zsbuf->texture == texture)
         return PIPE_REFERENCED_FOR_WRITE;
   }
   
   /* FIXME: we also need to do the same for the texture cache */
   
   return PIPE_UNREFERENCED;
}

static unsigned int
llvmpipe_is_buffer_referenced( struct pipe_context *pipe,
			       struct pipe_buffer *buf)
{
   return PIPE_UNREFERENCED;
}

struct pipe_context *
llvmpipe_create( struct pipe_screen *screen )
{
   struct llvmpipe_context *llvmpipe = CALLOC_STRUCT(llvmpipe_context);
   uint i;

   util_init_math();

#ifdef PIPE_ARCH_X86
   llvmpipe->use_sse = !debug_get_bool_option( "GALLIUM_NOSSE", FALSE );
#else
   llvmpipe->use_sse = FALSE;
#endif

   llvmpipe->dump_fs = debug_get_bool_option( "GALLIUM_DUMP_FS", FALSE );

   llvmpipe->pipe.winsys = screen->winsys;
   llvmpipe->pipe.screen = screen;
   llvmpipe->pipe.destroy = llvmpipe_destroy;

   /* state setters */
   llvmpipe->pipe.create_blend_state = llvmpipe_create_blend_state;
   llvmpipe->pipe.bind_blend_state   = llvmpipe_bind_blend_state;
   llvmpipe->pipe.delete_blend_state = llvmpipe_delete_blend_state;

   llvmpipe->pipe.create_sampler_state = llvmpipe_create_sampler_state;
   llvmpipe->pipe.bind_sampler_states  = llvmpipe_bind_sampler_states;
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
   llvmpipe->pipe.set_clip_state = llvmpipe_set_clip_state;
   llvmpipe->pipe.set_constant_buffer = llvmpipe_set_constant_buffer;
   llvmpipe->pipe.set_framebuffer_state = llvmpipe_set_framebuffer_state;
   llvmpipe->pipe.set_polygon_stipple = llvmpipe_set_polygon_stipple;
   llvmpipe->pipe.set_scissor_state = llvmpipe_set_scissor_state;
   llvmpipe->pipe.set_sampler_textures = llvmpipe_set_sampler_textures;
   llvmpipe->pipe.set_viewport_state = llvmpipe_set_viewport_state;

   llvmpipe->pipe.set_vertex_buffers = llvmpipe_set_vertex_buffers;
   llvmpipe->pipe.set_vertex_elements = llvmpipe_set_vertex_elements;

   llvmpipe->pipe.draw_arrays = llvmpipe_draw_arrays;
   llvmpipe->pipe.draw_elements = llvmpipe_draw_elements;
   llvmpipe->pipe.draw_range_elements = llvmpipe_draw_range_elements;
   llvmpipe->pipe.set_edgeflags = llvmpipe_set_edgeflags;


   llvmpipe->pipe.clear = llvmpipe_clear;
   llvmpipe->pipe.flush = llvmpipe_flush;

   llvmpipe->pipe.is_texture_referenced = llvmpipe_is_texture_referenced;
   llvmpipe->pipe.is_buffer_referenced = llvmpipe_is_buffer_referenced;

   llvmpipe_init_query_funcs( llvmpipe );
   llvmpipe_init_texture_funcs( llvmpipe );

   /*
    * Alloc caches for accessing drawing surfaces and textures.
    * Must be before quad stage setup!
    */
   for (i = 0; i < PIPE_MAX_COLOR_BUFS; i++)
      llvmpipe->cbuf_cache[i] = lp_create_tile_cache( screen );
   llvmpipe->zsbuf_cache = lp_create_tile_cache( screen );

   for (i = 0; i < PIPE_MAX_SAMPLERS; i++)
      llvmpipe->tex_cache[i] = lp_create_tile_cache( screen );


   /* setup quad rendering stages */
   for (i = 0; i < SP_NUM_QUAD_THREADS; i++) {
      llvmpipe->quad[i].polygon_stipple = lp_quad_polygon_stipple_stage(llvmpipe);
      llvmpipe->quad[i].earlyz = lp_quad_earlyz_stage(llvmpipe);
      llvmpipe->quad[i].shade = lp_quad_shade_stage(llvmpipe);
      llvmpipe->quad[i].alpha_test = lp_quad_alpha_test_stage(llvmpipe);
      llvmpipe->quad[i].depth_test = lp_quad_depth_test_stage(llvmpipe);
      llvmpipe->quad[i].stencil_test = lp_quad_stencil_test_stage(llvmpipe);
      llvmpipe->quad[i].occlusion = lp_quad_occlusion_stage(llvmpipe);
      llvmpipe->quad[i].coverage = lp_quad_coverage_stage(llvmpipe);
      llvmpipe->quad[i].blend = lp_quad_blend_stage(llvmpipe);
      llvmpipe->quad[i].colormask = lp_quad_colormask_stage(llvmpipe);
      llvmpipe->quad[i].output = lp_quad_output_stage(llvmpipe);
   }

   /* vertex shader samplers */
   for (i = 0; i < PIPE_MAX_SAMPLERS; i++) {
      llvmpipe->tgsi.vert_samplers[i].base.get_samples = lp_get_samples_vertex;
      llvmpipe->tgsi.vert_samplers[i].unit = i;
      llvmpipe->tgsi.vert_samplers[i].lp = llvmpipe;
      llvmpipe->tgsi.vert_samplers[i].cache = llvmpipe->tex_cache[i];
      llvmpipe->tgsi.vert_samplers_list[i] = &llvmpipe->tgsi.vert_samplers[i];
   }

   /* fragment shader samplers */
   for (i = 0; i < PIPE_MAX_SAMPLERS; i++) {
      llvmpipe->tgsi.frag_samplers[i].base.get_samples = lp_get_samples_fragment;
      llvmpipe->tgsi.frag_samplers[i].unit = i;
      llvmpipe->tgsi.frag_samplers[i].lp = llvmpipe;
      llvmpipe->tgsi.frag_samplers[i].cache = llvmpipe->tex_cache[i];
      llvmpipe->tgsi.frag_samplers_list[i] = &llvmpipe->tgsi.frag_samplers[i];
   }

   /*
    * Create drawing context and plug our rendering stage into it.
    */
   llvmpipe->draw = draw_create();
   if (!llvmpipe->draw) 
      goto fail;

   draw_texture_samplers(llvmpipe->draw,
                         PIPE_MAX_SAMPLERS,
                         (struct tgsi_sampler **)
                            llvmpipe->tgsi.vert_samplers_list);

   llvmpipe->setup = lp_draw_render_stage(llvmpipe);
   if (!llvmpipe->setup)
      goto fail;

   if (debug_get_bool_option( "SP_NO_RAST", FALSE ))
      llvmpipe->no_rast = TRUE;

   if (debug_get_bool_option( "SP_NO_VBUF", FALSE )) {
      /* Deprecated path -- vbuf is the intended interface to the draw module:
       */
      draw_set_rasterize_stage(llvmpipe->draw, llvmpipe->setup);
   }
   else {
      lp_init_vbuf(llvmpipe);
   }

   /* plug in AA line/point stages */
   draw_install_aaline_stage(llvmpipe->draw, &llvmpipe->pipe);
   draw_install_aapoint_stage(llvmpipe->draw, &llvmpipe->pipe);

#if USE_DRAW_STAGE_PSTIPPLE
   /* Do polygon stipple w/ texture map + frag prog? */
   draw_install_pstipple_stage(llvmpipe->draw, &llvmpipe->pipe);
#endif

   lp_init_surface_functions(llvmpipe);

   return &llvmpipe->pipe;

 fail:
   llvmpipe_destroy(&llvmpipe->pipe);
   return NULL;
}


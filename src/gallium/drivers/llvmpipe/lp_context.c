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
#include "lp_prim_vbuf.h"
#include "lp_state.h"
#include "lp_surface.h"
#include "lp_tile_cache.h"
#include "lp_tex_cache.h"
#include "lp_texture.h"
#include "lp_winsys.h"
#include "lp_query.h"



/**
 * Map any drawing surfaces which aren't already mapped
 */
void
llvmpipe_map_transfers(struct llvmpipe_context *lp)
{
   struct pipe_screen *screen = lp->pipe.screen;
   struct pipe_surface *zsbuf = lp->framebuffer.zsbuf;
   unsigned i;

   for (i = 0; i < lp->framebuffer.nr_cbufs; i++) {
      lp_tile_cache_map_transfers(lp->cbuf_cache[i]);
   }

   if(zsbuf) {
      if(!lp->zsbuf_transfer)
         lp->zsbuf_transfer = screen->get_tex_transfer(screen, zsbuf->texture,
                                                       zsbuf->face, zsbuf->level, zsbuf->zslice,
                                                       PIPE_TRANSFER_READ_WRITE,
                                                       0, 0, zsbuf->width, zsbuf->height);
      if(lp->zsbuf_transfer && !lp->zsbuf_map)
         lp->zsbuf_map = screen->transfer_map(screen, lp->zsbuf_transfer);

   }
}


/**
 * Unmap any mapped drawing surfaces
 */
void
llvmpipe_unmap_transfers(struct llvmpipe_context *lp)
{
   uint i;

   for (i = 0; i < lp->framebuffer.nr_cbufs; i++) {
      lp_tile_cache_unmap_transfers(lp->cbuf_cache[i]);
   }

   if(lp->zsbuf_transfer) {
      struct pipe_screen *screen = lp->pipe.screen;

      if(lp->zsbuf_map) {
         screen->transfer_unmap(screen, lp->zsbuf_transfer);
         lp->zsbuf_map = NULL;
      }
   }
}


static void llvmpipe_destroy( struct pipe_context *pipe )
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context( pipe );
   uint i;

   if (llvmpipe->draw)
      draw_destroy( llvmpipe->draw );

   for (i = 0; i < PIPE_MAX_COLOR_BUFS; i++) {
      lp_destroy_tile_cache(llvmpipe->cbuf_cache[i]);
      pipe_surface_reference(&llvmpipe->framebuffer.cbufs[i], NULL);
   }
   pipe_surface_reference(&llvmpipe->framebuffer.zsbuf, NULL);

   for (i = 0; i < PIPE_MAX_SAMPLERS; i++) {
      lp_destroy_tex_tile_cache(llvmpipe->tex_cache[i]);
      pipe_texture_reference(&llvmpipe->texture[i], NULL);
   }

   for (i = 0; i < PIPE_MAX_VERTEX_SAMPLERS; i++) {
      lp_destroy_tex_tile_cache(llvmpipe->vertex_tex_cache[i]);
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
   unsigned i;

   /* check if any of the bound drawing surfaces are this texture */
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

   /* check if any of the tex_cache textures are this texture */
   for (i = 0; i < PIPE_MAX_SAMPLERS; i++) {
      if (llvmpipe->tex_cache[i] &&
            llvmpipe->tex_cache[i]->texture == texture)
         return PIPE_REFERENCED_FOR_READ;
   }
   for (i = 0; i < PIPE_MAX_VERTEX_SAMPLERS; i++) {
      if (llvmpipe->vertex_tex_cache[i] &&
          llvmpipe->vertex_tex_cache[i]->texture == texture)
         return PIPE_REFERENCED_FOR_READ;
   }
   
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
   struct llvmpipe_context *llvmpipe;
   uint i;

   llvmpipe = align_malloc(sizeof(struct llvmpipe_context), 16);
   if (!llvmpipe)
      return NULL;

   util_init_math();

   memset(llvmpipe, 0, sizeof *llvmpipe);

   llvmpipe->pipe.winsys = screen->winsys;
   llvmpipe->pipe.screen = screen;
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
   llvmpipe_init_texture_funcs( llvmpipe );

   /*
    * Alloc caches for accessing drawing surfaces and textures.
    */
   for (i = 0; i < PIPE_MAX_COLOR_BUFS; i++)
      llvmpipe->cbuf_cache[i] = lp_create_tile_cache( screen );

   for (i = 0; i < PIPE_MAX_SAMPLERS; i++)
      llvmpipe->tex_cache[i] = lp_create_tex_tile_cache( screen );
   for (i = 0; i < PIPE_MAX_VERTEX_SAMPLERS; i++)
      llvmpipe->vertex_tex_cache[i] = lp_create_tex_tile_cache(screen);


   /*
    * Create drawing context and plug our rendering stage into it.
    */
   llvmpipe->draw = draw_create();
   if (!llvmpipe->draw) 
      goto fail;

   /* FIXME: devise alternative to draw_texture_samplers */

   if (debug_get_bool_option( "LP_NO_RAST", FALSE ))
      llvmpipe->no_rast = TRUE;

   llvmpipe->vbuf_backend = lp_create_vbuf_backend(llvmpipe);
   if (!llvmpipe->vbuf_backend)
      goto fail;

   llvmpipe->vbuf = draw_vbuf_stage(llvmpipe->draw, llvmpipe->vbuf_backend);
   if (!llvmpipe->vbuf)
      goto fail;

   draw_set_rasterize_stage(llvmpipe->draw, llvmpipe->vbuf);
   draw_set_render(llvmpipe->draw, llvmpipe->vbuf_backend);



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


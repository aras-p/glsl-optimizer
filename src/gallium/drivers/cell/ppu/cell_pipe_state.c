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

/* Authors:
 *  Keith Whitwell <keith@tungstengraphics.com>
 *  Brian Paul
 */

#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "draw/draw_context.h"
#include "cell_context.h"
#include "cell_flush.h"
#include "cell_pipe_state.h"
#include "cell_state.h"
#include "cell_texture.h"



static void *
cell_create_blend_state(struct pipe_context *pipe,
                        const struct pipe_blend_state *blend)
{
   return mem_dup(blend, sizeof(*blend));
}


static void
cell_bind_blend_state(struct pipe_context *pipe, void *blend)
{
   struct cell_context *cell = cell_context(pipe);

   draw_flush(cell->draw);

   cell->blend = (struct pipe_blend_state *) blend;
   cell->dirty |= CELL_NEW_BLEND;
}


static void
cell_delete_blend_state(struct pipe_context *pipe, void *blend)
{
   FREE(blend);
}


static void
cell_set_blend_color(struct pipe_context *pipe,
                     const struct pipe_blend_color *blend_color)
{
   struct cell_context *cell = cell_context(pipe);

   draw_flush(cell->draw);

   cell->blend_color = *blend_color;

   cell->dirty |= CELL_NEW_BLEND;
}




static void *
cell_create_depth_stencil_alpha_state(struct pipe_context *pipe,
                 const struct pipe_depth_stencil_alpha_state *dsa)
{
   return mem_dup(dsa, sizeof(*dsa));
}


static void
cell_bind_depth_stencil_alpha_state(struct pipe_context *pipe,
                                    void *dsa)
{
   struct cell_context *cell = cell_context(pipe);

   draw_flush(cell->draw);

   cell->depth_stencil = (struct pipe_depth_stencil_alpha_state *) dsa;
   cell->dirty |= CELL_NEW_DEPTH_STENCIL;
}


static void
cell_delete_depth_stencil_alpha_state(struct pipe_context *pipe, void *dsa)
{
   FREE(dsa);
}


static void
cell_set_stencil_ref(struct pipe_context *pipe,
                     const struct pipe_stencil_ref *stencil_ref)
{
   struct cell_context *cell = cell_context(pipe);

   draw_flush(cell->draw);

   cell->stencil_ref = *stencil_ref;

   cell->dirty |= CELL_NEW_DEPTH_STENCIL;
}


static void
cell_set_clip_state(struct pipe_context *pipe,
                    const struct pipe_clip_state *clip)
{
   struct cell_context *cell = cell_context(pipe);

   /* pass the clip state to the draw module */
   draw_set_clip_state(cell->draw, clip);
}


static void
cell_set_sample_mask(struct pipe_context *pipe,
                     unsigned sample_mask)
{
}


/* Called when driver state tracker notices changes to the viewport
 * matrix:
 */
static void
cell_set_viewport_state( struct pipe_context *pipe,
                         const struct pipe_viewport_state *viewport )
{
   struct cell_context *cell = cell_context(pipe);

   cell->viewport = *viewport; /* struct copy */
   cell->dirty |= CELL_NEW_VIEWPORT;

   /* pass the viewport info to the draw module */
   draw_set_viewport_state(cell->draw, viewport);

   /* Using tnl/ and vf/ modules is temporary while getting started.
    * Full pipe will have vertex shader, vertex fetch of its own.
    */
}


static void
cell_set_scissor_state( struct pipe_context *pipe,
                        const struct pipe_scissor_state *scissor )
{
   struct cell_context *cell = cell_context(pipe);

   memcpy( &cell->scissor, scissor, sizeof(*scissor) );
   cell->dirty |= CELL_NEW_SCISSOR;
}


static void
cell_set_polygon_stipple( struct pipe_context *pipe,
                          const struct pipe_poly_stipple *stipple )
{
   struct cell_context *cell = cell_context(pipe);

   memcpy( &cell->poly_stipple, stipple, sizeof(*stipple) );
   cell->dirty |= CELL_NEW_STIPPLE;
}



static void *
cell_create_rasterizer_state(struct pipe_context *pipe,
                             const struct pipe_rasterizer_state *rasterizer)
{
   return mem_dup(rasterizer, sizeof(*rasterizer));
}


static void
cell_bind_rasterizer_state(struct pipe_context *pipe, void *rast)
{
   struct pipe_rasterizer_state *rasterizer =
      (struct pipe_rasterizer_state *) rast;
   struct cell_context *cell = cell_context(pipe);

   /* pass-through to draw module */
   draw_set_rasterizer_state(cell->draw, rasterizer, rast);

   cell->rasterizer = rasterizer;

   cell->dirty |= CELL_NEW_RASTERIZER;
}


static void
cell_delete_rasterizer_state(struct pipe_context *pipe, void *rasterizer)
{
   FREE(rasterizer);
}



static void *
cell_create_sampler_state(struct pipe_context *pipe,
                          const struct pipe_sampler_state *sampler)
{
   return mem_dup(sampler, sizeof(*sampler));
}


static void
cell_bind_sampler_states(struct pipe_context *pipe,
                         unsigned num, void **samplers)
{
   struct cell_context *cell = cell_context(pipe);
   uint i, changed = 0x0;

   assert(num <= CELL_MAX_SAMPLERS);

   draw_flush(cell->draw);

   for (i = 0; i < CELL_MAX_SAMPLERS; i++) {
      struct pipe_sampler_state *new_samp = i < num ? samplers[i] : NULL;
      if (cell->sampler[i] != new_samp) {
         cell->sampler[i] = new_samp;
         changed |= (1 << i);
      }
   }

   if (changed) {
      cell->dirty |= CELL_NEW_SAMPLER;
      cell->dirty_samplers |= changed;
   }
}


static void
cell_delete_sampler_state(struct pipe_context *pipe,
                              void *sampler)
{
   FREE( sampler );
}



static void
cell_set_fragment_sampler_views(struct pipe_context *pipe,
                                unsigned num,
                                struct pipe_sampler_view **views)
{
   struct cell_context *cell = cell_context(pipe);
   uint i, changed = 0x0;

   assert(num <= CELL_MAX_SAMPLERS);

   for (i = 0; i < CELL_MAX_SAMPLERS; i++) {
      struct pipe_sampler_view *new_view = i < num ? views[i] : NULL;
      struct pipe_sampler_view *old_view = cell->fragment_sampler_views[i];

      if (old_view != new_view) {
         struct pipe_resource *new_tex = new_view ? new_view->texture : NULL;

         pipe_sampler_view_reference(&cell->fragment_sampler_views[i],
                                     new_view);
         pipe_resource_reference((struct pipe_resource **) &cell->texture[i],
                                (struct pipe_resource *) new_tex);

         changed |= (1 << i);
      }
   }

   cell->num_textures = num;

   if (changed) {
      cell->dirty |= CELL_NEW_TEXTURE;
      cell->dirty_textures |= changed;
   }
}


static struct pipe_sampler_view *
cell_create_sampler_view(struct pipe_context *pipe,
                         struct pipe_resource *texture,
                         const struct pipe_sampler_view *templ)
{
   struct pipe_sampler_view *view = CALLOC_STRUCT(pipe_sampler_view);

   if (view) {
      *view = *templ;
      view->reference.count = 1;
      view->texture = NULL;
      pipe_resource_reference(&view->texture, texture);
      view->context = pipe;
   }

   return view;
}


static void
cell_sampler_view_destroy(struct pipe_context *pipe,
                          struct pipe_sampler_view *view)
{
   pipe_resource_reference(&view->texture, NULL);
   FREE(view);
}


/**
 * Map color and z/stencil framebuffer surfaces.
 */
static void
cell_map_surfaces(struct cell_context *cell)
{
#if 0
   struct pipe_screen *screen = cell->pipe.screen;
#endif
   uint i;

   for (i = 0; i < 1; i++) {
      struct pipe_surface *ps = cell->framebuffer.cbufs[i];
      if (ps) {
         struct cell_resource *ct = cell_resource(ps->texture);
#if 0
         cell->cbuf_map[i] = screen->buffer_map(screen,
                                                ct->buffer,
                                                (PIPE_BUFFER_USAGE_GPU_READ |
                                                 PIPE_BUFFER_USAGE_GPU_WRITE));
#else
         cell->cbuf_map[i] = ct->data;
#endif
      }
   }

   {
      struct pipe_surface *ps = cell->framebuffer.zsbuf;
      if (ps) {
         struct cell_resource *ct = cell_resource(ps->texture);
#if 0
         cell->zsbuf_map = screen->buffer_map(screen,
                                              ct->buffer,
                                              (PIPE_BUFFER_USAGE_GPU_READ |
                                               PIPE_BUFFER_USAGE_GPU_WRITE));
#else
         cell->zsbuf_map = ct->data;
#endif
      }
   }
}


/**
 * Unmap color and z/stencil framebuffer surfaces.
 */
static void
cell_unmap_surfaces(struct cell_context *cell)
{
   /*struct pipe_screen *screen = cell->pipe.screen;*/
   uint i;

   for (i = 0; i < PIPE_MAX_COLOR_BUFS; i++) {
      struct pipe_surface *ps = cell->framebuffer.cbufs[i];
      if (ps && cell->cbuf_map[i]) {
         /*struct cell_resource *ct = cell_resource(ps->texture);*/
         assert(ps->texture);
         /*assert(ct->buffer);*/

         /*screen->buffer_unmap(screen, ct->buffer);*/
         cell->cbuf_map[i] = NULL;
      }
   }

   {
      struct pipe_surface *ps = cell->framebuffer.zsbuf;
      if (ps && cell->zsbuf_map) {
         /*struct cell_resource *ct = cell_resource(ps->texture);*/
         /*screen->buffer_unmap(screen, ct->buffer);*/
         cell->zsbuf_map = NULL;
      }
   }
}


static void
cell_set_framebuffer_state(struct pipe_context *pipe,
                           const struct pipe_framebuffer_state *fb)
{
   struct cell_context *cell = cell_context(pipe);

   if (1 /*memcmp(&cell->framebuffer, fb, sizeof(*fb))*/) {
      uint i;

      /* unmap old surfaces */
      cell_unmap_surfaces(cell);

      /* Finish any pending rendering to the current surface before
       * installing a new surface!
       */
      cell_flush_int(cell, CELL_FLUSH_WAIT);

      /* update my state
       * (this is also where old surfaces will finally get freed)
       */
      cell->framebuffer.width = fb->width;
      cell->framebuffer.height = fb->height;
      cell->framebuffer.nr_cbufs = fb->nr_cbufs;
      for (i = 0; i < PIPE_MAX_COLOR_BUFS; i++) {
         pipe_surface_reference(&cell->framebuffer.cbufs[i], fb->cbufs[i]);
      }
      pipe_surface_reference(&cell->framebuffer.zsbuf, fb->zsbuf);

      /* map new surfaces */
      cell_map_surfaces(cell);

      cell->dirty |= CELL_NEW_FRAMEBUFFER;
   }
}


void
cell_init_state_functions(struct cell_context *cell)
{
   cell->pipe.create_blend_state = cell_create_blend_state;
   cell->pipe.bind_blend_state   = cell_bind_blend_state;
   cell->pipe.delete_blend_state = cell_delete_blend_state;

   cell->pipe.create_sampler_state = cell_create_sampler_state;
   cell->pipe.bind_fragment_sampler_states = cell_bind_sampler_states;
   cell->pipe.delete_sampler_state = cell_delete_sampler_state;

   cell->pipe.set_fragment_sampler_views = cell_set_fragment_sampler_views;
   cell->pipe.create_sampler_view = cell_create_sampler_view;
   cell->pipe.sampler_view_destroy = cell_sampler_view_destroy;

   cell->pipe.create_depth_stencil_alpha_state = cell_create_depth_stencil_alpha_state;
   cell->pipe.bind_depth_stencil_alpha_state   = cell_bind_depth_stencil_alpha_state;
   cell->pipe.delete_depth_stencil_alpha_state = cell_delete_depth_stencil_alpha_state;

   cell->pipe.create_rasterizer_state = cell_create_rasterizer_state;
   cell->pipe.bind_rasterizer_state   = cell_bind_rasterizer_state;
   cell->pipe.delete_rasterizer_state = cell_delete_rasterizer_state;

   cell->pipe.set_blend_color = cell_set_blend_color;
   cell->pipe.set_stencil_ref = cell_set_stencil_ref;
   cell->pipe.set_clip_state = cell_set_clip_state;
   cell->pipe.set_sample_mask = cell_set_sample_mask;

   cell->pipe.set_framebuffer_state = cell_set_framebuffer_state;

   cell->pipe.set_polygon_stipple = cell_set_polygon_stipple;
   cell->pipe.set_scissor_state = cell_set_scissor_state;
   cell->pipe.set_viewport_state = cell_set_viewport_state;
}

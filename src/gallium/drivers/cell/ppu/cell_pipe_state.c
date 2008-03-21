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

#include "pipe/p_util.h"
#include "pipe/p_inlines.h"
#include "draw/draw_context.h"
#include "cell_context.h"
#include "cell_state.h"
#include "cell_texture.h"
#include "cell_state_per_fragment.h"



static void *
cell_create_blend_state(struct pipe_context *pipe,
                        const struct pipe_blend_state *blend)
{
   struct cell_blend_state *cb = MALLOC(sizeof(struct cell_blend_state));

   (void) memcpy(cb, blend, sizeof(*blend));
   cell_generate_alpha_blend(cb);

   return cb;
}


static void
cell_bind_blend_state(struct pipe_context *pipe, void *state)
{
   struct cell_context *cell = cell_context(pipe);

   draw_flush(cell->draw);

   cell->blend = (struct cell_blend_state *) state;
   cell->dirty |= CELL_NEW_BLEND;
}


static void
cell_delete_blend_state(struct pipe_context *pipe, void *blend)
{
   struct cell_blend_state *cb = (struct cell_blend_state *) blend;

   spe_release_func(& cb->code);
   FREE(cb);
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
                 const struct pipe_depth_stencil_alpha_state *depth_stencil)
{
   struct cell_depth_stencil_alpha_state *cdsa =
       MALLOC(sizeof(struct cell_depth_stencil_alpha_state));

   (void) memcpy(cdsa, depth_stencil, sizeof(*depth_stencil));
   cell_generate_depth_stencil_test(cdsa);

   return cdsa;
}


static void
cell_bind_depth_stencil_alpha_state(struct pipe_context *pipe,
                                    void *depth_stencil)
{
   struct cell_context *cell = cell_context(pipe);

   draw_flush(cell->draw);

   cell->depth_stencil =
       (struct cell_depth_stencil_alpha_state *) depth_stencil;
   cell->dirty |= CELL_NEW_DEPTH_STENCIL;
}


static void
cell_delete_depth_stencil_alpha_state(struct pipe_context *pipe, void *depth)
{
   struct cell_depth_stencil_alpha_state *cdsa =
       (struct cell_depth_stencil_alpha_state *) depth;

   spe_release_func(& cdsa->code);
   FREE(cdsa);
}


static void cell_set_clip_state( struct pipe_context *pipe,
			     const struct pipe_clip_state *clip )
{
   struct cell_context *cell = cell_context(pipe);

   /* pass the clip state to the draw module */
   draw_set_clip_state(cell->draw, clip);
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
                             const struct pipe_rasterizer_state *setup)
{
   struct pipe_rasterizer_state *state
      = MALLOC(sizeof(struct pipe_rasterizer_state));
   memcpy(state, setup, sizeof(struct pipe_rasterizer_state));
   return state;
}


static void
cell_bind_rasterizer_state(struct pipe_context *pipe, void *setup)
{
   struct cell_context *cell = cell_context(pipe);

   /* pass-through to draw module */
   draw_set_rasterizer_state(cell->draw, setup);

   cell->rasterizer = (struct pipe_rasterizer_state *)setup;

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

   draw_flush(cell->draw);

   assert(unit < PIPE_MAX_SAMPLERS);

   memcpy(cell->sampler, samplers, num * sizeof(void *));
   memset(&cell->sampler[num], 0, (PIPE_MAX_SAMPLERS - num) *
          sizeof(void *));
   cell->num_samplers = num;

   cell->dirty |= CELL_NEW_SAMPLER;
}


static void
cell_delete_sampler_state(struct pipe_context *pipe,
                              void *sampler)
{
   FREE( sampler );
}



static void
cell_set_sampler_textures(struct pipe_context *pipe,
                          unsigned num, struct pipe_texture **texture)
{
   struct cell_context *cell = cell_context(pipe);
   uint i;

   /* Check for no-op */
   if (num == cell->num_textures &&
       !memcmp(cell->texture, texture, num * sizeof(struct pipe_texture *)))
      return;

   draw_flush(cell->draw);

   for (i = 0; i < PIPE_MAX_SAMPLERS; i++) {
      struct pipe_texture *tex = i < num ? texture[i] : NULL;

      pipe_texture_reference((struct pipe_texture **) &cell->texture[i], tex);
   }

   cell_update_texture_mapping(cell);

   cell->dirty |= CELL_NEW_TEXTURE;
}



static void
cell_set_framebuffer_state(struct pipe_context *pipe,
                           const struct pipe_framebuffer_state *fb)
{
   struct cell_context *cell = cell_context(pipe);

   if (1 /*memcmp(&cell->framebuffer, fb, sizeof(*fb))*/) {
      struct pipe_surface *csurf = fb->cbufs[0];
      struct pipe_surface *zsurf = fb->zsbuf;
      uint i;

      /* unmap old surfaces */
      for (i = 0; i < PIPE_MAX_COLOR_BUFS; i++) {
         if (cell->framebuffer.cbufs[i] && cell->cbuf_map[i]) {
            pipe_surface_unmap(cell->framebuffer.cbufs[i]);
            cell->cbuf_map[i] = NULL;
         }
      }

      if (cell->framebuffer.zsbuf && cell->zsbuf_map) {
         pipe_surface_unmap(cell->framebuffer.zsbuf);
         cell->zsbuf_map = NULL;
      }

      /* update my state */
      cell->framebuffer = *fb;

      /* map new surfaces */
      if (csurf)
         cell->cbuf_map[0] = pipe_surface_map(csurf);

      if (zsurf)
         cell->zsbuf_map = pipe_surface_map(zsurf);

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
   cell->pipe.bind_sampler_states = cell_bind_sampler_states;
   cell->pipe.delete_sampler_state = cell_delete_sampler_state;

   cell->pipe.set_sampler_textures = cell_set_sampler_textures;

   cell->pipe.create_depth_stencil_alpha_state = cell_create_depth_stencil_alpha_state;
   cell->pipe.bind_depth_stencil_alpha_state   = cell_bind_depth_stencil_alpha_state;
   cell->pipe.delete_depth_stencil_alpha_state = cell_delete_depth_stencil_alpha_state;

   cell->pipe.create_rasterizer_state = cell_create_rasterizer_state;
   cell->pipe.bind_rasterizer_state   = cell_bind_rasterizer_state;
   cell->pipe.delete_rasterizer_state = cell_delete_rasterizer_state;

   cell->pipe.set_blend_color = cell_set_blend_color;
   cell->pipe.set_clip_state = cell_set_clip_state;

   cell->pipe.set_framebuffer_state = cell_set_framebuffer_state;

   cell->pipe.set_polygon_stipple = cell_set_polygon_stipple;
   cell->pipe.set_scissor_state = cell_set_scissor_state;
   cell->pipe.set_viewport_state = cell_set_viewport_state;
}

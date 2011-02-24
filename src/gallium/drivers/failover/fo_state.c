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

/* Authors:  Keith Whitwell <keith@tungstengraphics.com>
 */

#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_transfer.h"

#include "fo_context.h"


/* This looks like a lot of work at the moment - we're keeping a
 * duplicate copy of the state up-to-date.  
 *
 * This can change in two ways:
 * - With constant state objects we would only need to save a pointer,
 *     not the whole object.
 * - By adding a callback in the state tracker to re-emit state.  The
 *     state tracker knows the current state already and can re-emit it 
 *     without additional complexity.
 *
 * This works as a proof-of-concept, but a final version will have
 * lower overheads.
 */



static void *
failover_create_blend_state( struct pipe_context *pipe,
                             const struct pipe_blend_state *blend )
{
   struct fo_state *state = MALLOC(sizeof(struct fo_state));
   struct failover_context *failover = failover_context(pipe);

   state->sw_state = failover->sw->create_blend_state(failover->sw, blend);
   state->hw_state = failover->hw->create_blend_state(failover->hw, blend);

   return state;
}

static void
failover_bind_blend_state( struct pipe_context *pipe,
                           void *blend )
{
   struct failover_context *failover = failover_context(pipe);
   struct fo_state *state = (struct fo_state *)blend;
   failover->blend = state;
   failover->dirty |= FO_NEW_BLEND;
   failover->sw->bind_blend_state( failover->sw, state->sw_state );
   failover->hw->bind_blend_state( failover->hw, state->hw_state );
}

static void
failover_delete_blend_state( struct pipe_context *pipe,
                             void *blend )
{
   struct fo_state *state = (struct fo_state*)blend;
   struct failover_context *failover = failover_context(pipe);

   failover->sw->delete_blend_state(failover->sw, state->sw_state);
   failover->hw->delete_blend_state(failover->hw, state->hw_state);
   state->sw_state = 0;
   state->hw_state = 0;
   FREE(state);
}

static void
failover_set_blend_color( struct pipe_context *pipe,
                          const struct pipe_blend_color *blend_color )
{
   struct failover_context *failover = failover_context(pipe);

   failover->blend_color = *blend_color;
   failover->dirty |= FO_NEW_BLEND_COLOR;
   failover->sw->set_blend_color( failover->sw, blend_color );
   failover->hw->set_blend_color( failover->hw, blend_color );
}

static void
failover_set_stencil_ref( struct pipe_context *pipe,
                          const struct pipe_stencil_ref *stencil_ref )
{
   struct failover_context *failover = failover_context(pipe);

   failover->stencil_ref = *stencil_ref;
   failover->dirty |= FO_NEW_STENCIL_REF;
   failover->sw->set_stencil_ref( failover->sw, stencil_ref );
   failover->hw->set_stencil_ref( failover->hw, stencil_ref );
}

static void 
failover_set_clip_state( struct pipe_context *pipe,
                         const struct pipe_clip_state *clip )
{
   struct failover_context *failover = failover_context(pipe);

   failover->clip = *clip;
   failover->dirty |= FO_NEW_CLIP;
   failover->sw->set_clip_state( failover->sw, clip );
   failover->hw->set_clip_state( failover->hw, clip );
}

static void
failover_set_sample_mask(struct pipe_context *pipe,
                         unsigned sample_mask)
{
   struct failover_context *failover = failover_context(pipe);

   failover->sample_mask = sample_mask;
   failover->dirty |= FO_NEW_SAMPLE_MASK;
   failover->sw->set_sample_mask( failover->sw, sample_mask );
   failover->hw->set_sample_mask( failover->hw, sample_mask );

}


static void *
failover_create_depth_stencil_state(struct pipe_context *pipe,
                              const struct pipe_depth_stencil_alpha_state *templ)
{
   struct fo_state *state = MALLOC(sizeof(struct fo_state));
   struct failover_context *failover = failover_context(pipe);

   state->sw_state = failover->sw->create_depth_stencil_alpha_state(failover->sw, templ);
   state->hw_state = failover->hw->create_depth_stencil_alpha_state(failover->hw, templ);

   return state;
}

static void
failover_bind_depth_stencil_state(struct pipe_context *pipe,
                                  void *depth_stencil)
{
   struct failover_context *failover = failover_context(pipe);
   struct fo_state *state = (struct fo_state *)depth_stencil;
   failover->depth_stencil = state;
   failover->dirty |= FO_NEW_DEPTH_STENCIL;
   failover->sw->bind_depth_stencil_alpha_state(failover->sw, state->sw_state);
   failover->hw->bind_depth_stencil_alpha_state(failover->hw, state->hw_state);
}

static void
failover_delete_depth_stencil_state(struct pipe_context *pipe,
                                    void *ds)
{
   struct fo_state *state = (struct fo_state*)ds;
   struct failover_context *failover = failover_context(pipe);

   failover->sw->delete_depth_stencil_alpha_state(failover->sw, state->sw_state);
   failover->hw->delete_depth_stencil_alpha_state(failover->hw, state->hw_state);
   state->sw_state = 0;
   state->hw_state = 0;
   FREE(state);
}

static void
failover_set_framebuffer_state(struct pipe_context *pipe,
			       const struct pipe_framebuffer_state *framebuffer)
{
   struct failover_context *failover = failover_context(pipe);

   failover->framebuffer = *framebuffer;
   failover->dirty |= FO_NEW_FRAMEBUFFER;
   failover->sw->set_framebuffer_state( failover->sw, framebuffer );
   failover->hw->set_framebuffer_state( failover->hw, framebuffer );
}


static void *
failover_create_fs_state(struct pipe_context *pipe,
                         const struct pipe_shader_state *templ)
{
   struct fo_state *state = MALLOC(sizeof(struct fo_state));
   struct failover_context *failover = failover_context(pipe);

   state->sw_state = failover->sw->create_fs_state(failover->sw, templ);
   state->hw_state = failover->hw->create_fs_state(failover->hw, templ);

   return state;
}

static void
failover_bind_fs_state(struct pipe_context *pipe, void *fs)
{
   struct failover_context *failover = failover_context(pipe);
   struct fo_state *state = (struct fo_state*)fs;
   failover->fragment_shader = state;
   failover->dirty |= FO_NEW_FRAGMENT_SHADER;
   failover->sw->bind_fs_state(failover->sw, state->sw_state);
   failover->hw->bind_fs_state(failover->hw, state->hw_state);
}

static void
failover_delete_fs_state(struct pipe_context *pipe,
                         void *fs)
{
   struct fo_state *state = (struct fo_state*)fs;
   struct failover_context *failover = failover_context(pipe);

   failover->sw->delete_fs_state(failover->sw, state->sw_state);
   failover->hw->delete_fs_state(failover->hw, state->hw_state);
   state->sw_state = 0;
   state->hw_state = 0;
   FREE(state);
}

static void *
failover_create_vs_state(struct pipe_context *pipe,
                         const struct pipe_shader_state *templ)
{
   struct fo_state *state = MALLOC(sizeof(struct fo_state));
   struct failover_context *failover = failover_context(pipe);

   state->sw_state = failover->sw->create_vs_state(failover->sw, templ);
   state->hw_state = failover->hw->create_vs_state(failover->hw, templ);

   return state;
}

static void
failover_bind_vs_state(struct pipe_context *pipe,
                       void *vs)
{
   struct failover_context *failover = failover_context(pipe);

   struct fo_state *state = (struct fo_state*)vs;
   failover->vertex_shader = state;
   failover->dirty |= FO_NEW_VERTEX_SHADER;
   failover->sw->bind_vs_state(failover->sw, state->sw_state);
   failover->hw->bind_vs_state(failover->hw, state->hw_state);
}

static void
failover_delete_vs_state(struct pipe_context *pipe,
                         void *vs)
{
   struct fo_state *state = (struct fo_state*)vs;
   struct failover_context *failover = failover_context(pipe);

   failover->sw->delete_vs_state(failover->sw, state->sw_state);
   failover->hw->delete_vs_state(failover->hw, state->hw_state);
   state->sw_state = 0;
   state->hw_state = 0;
   FREE(state);
}



static void *
failover_create_vertex_elements_state( struct pipe_context *pipe,
                                       unsigned count,
                                       const struct pipe_vertex_element *velems )
{
   struct fo_state *state = MALLOC(sizeof(struct fo_state));
   struct failover_context *failover = failover_context(pipe);

   state->sw_state = failover->sw->create_vertex_elements_state(failover->sw, count, velems);
   state->hw_state = failover->hw->create_vertex_elements_state(failover->hw, count, velems);

   return state;
}

static void
failover_bind_vertex_elements_state(struct pipe_context *pipe,
                                    void *velems )
{
   struct failover_context *failover = failover_context(pipe);
   struct fo_state *state = (struct fo_state*)velems;

   failover->vertex_elements = state;
   failover->dirty |= FO_NEW_VERTEX_ELEMENT;
   failover->sw->bind_vertex_elements_state( failover->sw, velems );
   failover->hw->bind_vertex_elements_state( failover->hw, velems );
}

static void
failover_delete_vertex_elements_state( struct pipe_context *pipe,
                                       void *velems )
{
   struct fo_state *state = (struct fo_state*)velems;
   struct failover_context *failover = failover_context(pipe);

   failover->sw->delete_vertex_elements_state(failover->sw, state->sw_state);
   failover->hw->delete_vertex_elements_state(failover->hw, state->hw_state);
   state->sw_state = 0;
   state->hw_state = 0;
   FREE(state);
}

static void 
failover_set_polygon_stipple( struct pipe_context *pipe,
                              const struct pipe_poly_stipple *stipple )
{
   struct failover_context *failover = failover_context(pipe);

   failover->poly_stipple = *stipple;
   failover->dirty |= FO_NEW_STIPPLE;
   failover->sw->set_polygon_stipple( failover->sw, stipple );
   failover->hw->set_polygon_stipple( failover->hw, stipple );
}


static void *
failover_create_rasterizer_state(struct pipe_context *pipe,
                                 const struct pipe_rasterizer_state *templ)
{
   struct fo_state *state = MALLOC(sizeof(struct fo_state));
   struct failover_context *failover = failover_context(pipe);

   state->sw_state = failover->sw->create_rasterizer_state(failover->sw, templ);
   state->hw_state = failover->hw->create_rasterizer_state(failover->hw, templ);

   return state;
}

static void
failover_bind_rasterizer_state(struct pipe_context *pipe,
                               void *raster)
{
   struct failover_context *failover = failover_context(pipe);

   struct fo_state *state = (struct fo_state*)raster;
   failover->rasterizer = state;
   failover->dirty |= FO_NEW_RASTERIZER;
   failover->sw->bind_rasterizer_state(failover->sw, state->sw_state);
   failover->hw->bind_rasterizer_state(failover->hw, state->hw_state);
}

static void
failover_delete_rasterizer_state(struct pipe_context *pipe,
                                 void *raster)
{
   struct fo_state *state = (struct fo_state*)raster;
   struct failover_context *failover = failover_context(pipe);

   failover->sw->delete_rasterizer_state(failover->sw, state->sw_state);
   failover->hw->delete_rasterizer_state(failover->hw, state->hw_state);
   state->sw_state = 0;
   state->hw_state = 0;
   FREE(state);
}


static void 
failover_set_scissor_state( struct pipe_context *pipe,
                                 const struct pipe_scissor_state *scissor )
{
   struct failover_context *failover = failover_context(pipe);

   failover->scissor = *scissor;
   failover->dirty |= FO_NEW_SCISSOR;
   failover->sw->set_scissor_state( failover->sw, scissor );
   failover->hw->set_scissor_state( failover->hw, scissor );
}


static void *
failover_create_sampler_state(struct pipe_context *pipe,
                              const struct pipe_sampler_state *templ)
{
   struct fo_state *state = MALLOC(sizeof(struct fo_state));
   struct failover_context *failover = failover_context(pipe);

   state->sw_state = failover->sw->create_sampler_state(failover->sw, templ);
   state->hw_state = failover->hw->create_sampler_state(failover->hw, templ);

   return state;
}

static void
failover_bind_fragment_sampler_states(struct pipe_context *pipe,
                                      unsigned num,
                                      void **sampler)
{
   struct failover_context *failover = failover_context(pipe);
   struct fo_state *state = (struct fo_state*)sampler;
   uint i;
   assert(num <= PIPE_MAX_SAMPLERS);
   /* Check for no-op */
   if (num == failover->num_samplers &&
       !memcmp(failover->sampler, sampler, num * sizeof(void *)))
      return;
   for (i = 0; i < PIPE_MAX_SAMPLERS; i++) {
      failover->sw_sampler_state[i] = i < num ? state[i].sw_state : NULL;
      failover->hw_sampler_state[i] = i < num ? state[i].hw_state : NULL;
   }
   failover->dirty |= FO_NEW_SAMPLER;
   failover->num_samplers = num;
   failover->sw->bind_fragment_sampler_states(failover->sw, num,
                                              failover->sw_sampler_state);
   failover->hw->bind_fragment_sampler_states(failover->hw, num,
                                              failover->hw_sampler_state);
}

static void
failover_bind_vertex_sampler_states(struct pipe_context *pipe,
                                    unsigned num_samplers,
                                    void **samplers)
{
   struct failover_context *failover = failover_context(pipe);
   struct fo_state *state = (struct fo_state*)samplers;
   uint i;

   assert(num_samplers <= PIPE_MAX_VERTEX_SAMPLERS);

   /* Check for no-op */
   if (num_samplers == failover->num_vertex_samplers &&
       !memcmp(failover->vertex_samplers, samplers, num_samplers * sizeof(void *))) {
      return;
   }
   for (i = 0; i < PIPE_MAX_VERTEX_SAMPLERS; i++) {
      failover->sw_vertex_sampler_state[i] = i < num_samplers ? state[i].sw_state : NULL;
      failover->hw_vertex_sampler_state[i] = i < num_samplers ? state[i].hw_state : NULL;
   }
   failover->dirty |= FO_NEW_SAMPLER;
   failover->num_vertex_samplers = num_samplers;
   failover->sw->bind_vertex_sampler_states(failover->sw,
                                            num_samplers,
                                            failover->sw_vertex_sampler_state);
   failover->hw->bind_vertex_sampler_states(failover->hw,
                                            num_samplers,
                                            failover->hw_vertex_sampler_state);
}

static void
failover_delete_sampler_state(struct pipe_context *pipe, void *sampler)
{
   struct fo_state *state = (struct fo_state*)sampler;
   struct failover_context *failover = failover_context(pipe);

   failover->sw->delete_sampler_state(failover->sw, state->sw_state);
   failover->hw->delete_sampler_state(failover->hw, state->hw_state);
   state->sw_state = 0;
   state->hw_state = 0;
   FREE(state);
}


static struct pipe_sampler_view *
failover_create_sampler_view(struct pipe_context *pipe,
                             struct pipe_resource *texture,
                             const struct pipe_sampler_view *templ)
{
   struct fo_sampler_view *view = MALLOC(sizeof(struct fo_sampler_view));
   struct failover_context *failover = failover_context(pipe);

   view->sw = failover->sw->create_sampler_view(failover->sw, texture, templ);
   view->hw = failover->hw->create_sampler_view(failover->hw, texture, templ);

   view->base = *templ;
   view->base.reference.count = 1;
   view->base.texture = NULL;
   pipe_resource_reference(&view->base.texture, texture);
   view->base.context = pipe;

   return &view->base;
}

static void
failover_sampler_view_destroy(struct pipe_context *pipe,
                              struct pipe_sampler_view *view)
{
   struct fo_sampler_view *fo_view = (struct fo_sampler_view *)view;
   struct failover_context *failover = failover_context(pipe);

   failover->sw->sampler_view_destroy(failover->sw, fo_view->sw);
   failover->hw->sampler_view_destroy(failover->hw, fo_view->hw);

   pipe_resource_reference(&fo_view->base.texture, NULL);
   FREE(fo_view);
}

static void
failover_set_fragment_sampler_views(struct pipe_context *pipe,
                                    unsigned num,
                                    struct pipe_sampler_view **views)
{
   struct failover_context *failover = failover_context(pipe);
   struct pipe_sampler_view *hw_views[PIPE_MAX_SAMPLERS];
   uint i;

   assert(num <= PIPE_MAX_SAMPLERS);

   /* Check for no-op */
   if (num == failover->num_fragment_sampler_views &&
       !memcmp(failover->fragment_sampler_views, views, num * sizeof(struct pipe_sampler_view *)))
      return;
   for (i = 0; i < num; i++) {
      struct fo_sampler_view *fo_view = (struct fo_sampler_view *)views[i];

      pipe_sampler_view_reference((struct pipe_sampler_view **)&failover->fragment_sampler_views[i], views[i]);
      hw_views[i] = fo_view->hw;
   }
   for (i = num; i < failover->num_fragment_sampler_views; i++)
      pipe_sampler_view_reference((struct pipe_sampler_view **)&failover->fragment_sampler_views[i], NULL);
   failover->dirty |= FO_NEW_SAMPLER_VIEW;
   failover->num_fragment_sampler_views = num;
   failover->hw->set_fragment_sampler_views(failover->hw, num, hw_views);
}


static void
failover_set_vertex_sampler_views(struct pipe_context *pipe,
                                  unsigned num,
                                  struct pipe_sampler_view **views)
{
   struct failover_context *failover = failover_context(pipe);
   struct pipe_sampler_view *hw_views[PIPE_MAX_VERTEX_SAMPLERS];
   uint i;

   assert(num <= PIPE_MAX_VERTEX_SAMPLERS);

   /* Check for no-op */
   if (num == failover->num_vertex_sampler_views &&
       !memcmp(failover->vertex_sampler_views, views, num * sizeof(struct pipe_sampler_view *))) {
      return;
   }
   for (i = 0; i < num; i++) {
      struct fo_sampler_view *fo_view = (struct fo_sampler_view *)views[i];

      pipe_sampler_view_reference((struct pipe_sampler_view **)&failover->vertex_sampler_views[i], views[i]);
      hw_views[i] = fo_view->hw;
   }
   for (i = num; i < failover->num_vertex_sampler_views; i++)
      pipe_sampler_view_reference((struct pipe_sampler_view **)&failover->vertex_sampler_views[i], NULL);
   failover->dirty |= FO_NEW_SAMPLER_VIEW;
   failover->num_vertex_sampler_views = num;
   failover->hw->set_vertex_sampler_views(failover->hw, num, hw_views);
}


static void 
failover_set_viewport_state( struct pipe_context *pipe,
			     const struct pipe_viewport_state *viewport )
{
   struct failover_context *failover = failover_context(pipe);

   failover->viewport = *viewport; 
   failover->dirty |= FO_NEW_VIEWPORT;
   failover->sw->set_viewport_state( failover->sw, viewport );
   failover->hw->set_viewport_state( failover->hw, viewport );
}


static void
failover_set_vertex_buffers(struct pipe_context *pipe,
                            unsigned count,
                            const struct pipe_vertex_buffer *vertex_buffers)
{
   struct failover_context *failover = failover_context(pipe);

   util_copy_vertex_buffers(failover->vertex_buffers,
                            &failover->num_vertex_buffers,
                            vertex_buffers, count);
   failover->dirty |= FO_NEW_VERTEX_BUFFER;
   failover->sw->set_vertex_buffers( failover->sw, count, vertex_buffers );
   failover->hw->set_vertex_buffers( failover->hw, count, vertex_buffers );
}


static void
failover_set_index_buffer(struct pipe_context *pipe,
                          const struct pipe_index_buffer *ib)
{
   struct failover_context *failover = failover_context(pipe);

   if (ib)
      memcpy(&failover->index_buffer, ib, sizeof(failover->index_buffer));
   else
      memset(&failover->index_buffer, 0, sizeof(failover->index_buffer));

   failover->dirty |= FO_NEW_INDEX_BUFFER;
   failover->sw->set_index_buffer( failover->sw, ib );
   failover->hw->set_index_buffer( failover->hw, ib );
}


void
failover_set_constant_buffer(struct pipe_context *pipe,
                             uint shader, uint index,
                             struct pipe_resource *res)
{
   struct failover_context *failover = failover_context(pipe);

   assert(shader < PIPE_SHADER_TYPES);
   assert(index == 0);

   failover->sw->set_constant_buffer(failover->sw, shader, index, res);
   failover->hw->set_constant_buffer(failover->hw, shader, index, res);
}


void
failover_init_state_functions( struct failover_context *failover )
{
   failover->pipe.create_blend_state = failover_create_blend_state;
   failover->pipe.bind_blend_state   = failover_bind_blend_state;
   failover->pipe.delete_blend_state = failover_delete_blend_state;
   failover->pipe.create_sampler_state = failover_create_sampler_state;
   failover->pipe.bind_fragment_sampler_states  = failover_bind_fragment_sampler_states;
   failover->pipe.bind_vertex_sampler_states  = failover_bind_vertex_sampler_states;
   failover->pipe.delete_sampler_state = failover_delete_sampler_state;
   failover->pipe.create_depth_stencil_alpha_state = failover_create_depth_stencil_state;
   failover->pipe.bind_depth_stencil_alpha_state   = failover_bind_depth_stencil_state;
   failover->pipe.delete_depth_stencil_alpha_state = failover_delete_depth_stencil_state;
   failover->pipe.create_rasterizer_state = failover_create_rasterizer_state;
   failover->pipe.bind_rasterizer_state = failover_bind_rasterizer_state;
   failover->pipe.delete_rasterizer_state = failover_delete_rasterizer_state;
   failover->pipe.create_fs_state = failover_create_fs_state;
   failover->pipe.bind_fs_state   = failover_bind_fs_state;
   failover->pipe.delete_fs_state = failover_delete_fs_state;
   failover->pipe.create_vs_state = failover_create_vs_state;
   failover->pipe.bind_vs_state   = failover_bind_vs_state;
   failover->pipe.delete_vs_state = failover_delete_vs_state;
   failover->pipe.create_vertex_elements_state = failover_create_vertex_elements_state;
   failover->pipe.bind_vertex_elements_state = failover_bind_vertex_elements_state;
   failover->pipe.delete_vertex_elements_state = failover_delete_vertex_elements_state;

   failover->pipe.set_blend_color = failover_set_blend_color;
   failover->pipe.set_stencil_ref = failover_set_stencil_ref;
   failover->pipe.set_clip_state = failover_set_clip_state;
   failover->pipe.set_sample_mask = failover_set_sample_mask;
   failover->pipe.set_framebuffer_state = failover_set_framebuffer_state;
   failover->pipe.set_polygon_stipple = failover_set_polygon_stipple;
   failover->pipe.set_scissor_state = failover_set_scissor_state;
   failover->pipe.set_fragment_sampler_views = failover_set_fragment_sampler_views;
   failover->pipe.set_vertex_sampler_views = failover_set_vertex_sampler_views;
   failover->pipe.set_viewport_state = failover_set_viewport_state;
   failover->pipe.set_vertex_buffers = failover_set_vertex_buffers;
   failover->pipe.set_index_buffer = failover_set_index_buffer;
   failover->pipe.set_constant_buffer = failover_set_constant_buffer;
   failover->pipe.create_sampler_view = failover_create_sampler_view;
   failover->pipe.sampler_view_destroy = failover_sampler_view_destroy;
   failover->pipe.redefine_user_buffer = u_default_redefine_user_buffer;
}

/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_simple_list.h"

#include "pipe/p_format.h"
#include "pipe/p_screen.h"

#include "tr_dump.h"
#include "tr_dump_state.h"
#include "tr_public.h"
#include "tr_screen.h"
#include "tr_texture.h"
#include "tr_context.h"





static INLINE struct pipe_resource *
trace_resource_unwrap(struct trace_context *tr_ctx,
                     struct pipe_resource *resource)
{
   struct trace_resource *tr_res;

   if(!resource)
      return NULL;

   tr_res = trace_resource(resource);

   assert(tr_res->resource);
   return tr_res->resource;
}


static INLINE struct pipe_surface *
trace_surface_unwrap(struct trace_context *tr_ctx,
                     struct pipe_surface *surface)
{
   struct trace_screen *tr_scr = trace_screen(tr_ctx->base.screen);
   struct trace_surface *tr_surf;

   if(!surface)
      return NULL;

   assert(surface->texture);
   if(!surface->texture)
      return surface;

   tr_surf = trace_surface(surface);

   assert(tr_surf->surface);
   assert(tr_surf->surface->texture->screen == tr_scr->screen);
   (void) tr_scr;
   return tr_surf->surface;
}


static INLINE void
trace_context_draw_vbo(struct pipe_context *_pipe,
                       const struct pipe_draw_info *info)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   trace_dump_call_begin("pipe_context", "draw_vbo");

   trace_dump_arg(ptr,  pipe);
   trace_dump_arg(draw_info, info);

   pipe->draw_vbo(pipe, info);

   trace_dump_call_end();
}


static INLINE struct pipe_query *
trace_context_create_query(struct pipe_context *_pipe,
                           unsigned query_type)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;
   struct pipe_query *result;

   trace_dump_call_begin("pipe_context", "create_query");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(uint, query_type);

   result = pipe->create_query(pipe, query_type);

   trace_dump_ret(ptr, result);

   trace_dump_call_end();

   return result;
}


static INLINE void
trace_context_destroy_query(struct pipe_context *_pipe,
                            struct pipe_query *query)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   trace_dump_call_begin("pipe_context", "destroy_query");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, query);

   pipe->destroy_query(pipe, query);

   trace_dump_call_end();
}


static INLINE void
trace_context_begin_query(struct pipe_context *_pipe,
                          struct pipe_query *query)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   trace_dump_call_begin("pipe_context", "begin_query");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, query);

   pipe->begin_query(pipe, query);

   trace_dump_call_end();
}


static INLINE void
trace_context_end_query(struct pipe_context *_pipe,
                        struct pipe_query *query)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   trace_dump_call_begin("pipe_context", "end_query");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, query);

   pipe->end_query(pipe, query);

   trace_dump_call_end();
}


static INLINE boolean
trace_context_get_query_result(struct pipe_context *_pipe,
                               struct pipe_query *query,
                               boolean wait,
                               void *presult)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;
   uint64_t result;
   boolean _result;

   trace_dump_call_begin("pipe_context", "get_query_result");

   trace_dump_arg(ptr, pipe);

   _result = pipe->get_query_result(pipe, query, wait, presult);
   result = *((uint64_t*)presult);

   trace_dump_arg(uint, result);
   trace_dump_ret(bool, _result);

   trace_dump_call_end();

   return _result;
}


static INLINE void *
trace_context_create_blend_state(struct pipe_context *_pipe,
                                 const struct pipe_blend_state *state)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;
   void * result;

   trace_dump_call_begin("pipe_context", "create_blend_state");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(blend_state, state);

   result = pipe->create_blend_state(pipe, state);

   trace_dump_ret(ptr, result);

   trace_dump_call_end();

   return result;
}


static INLINE void
trace_context_bind_blend_state(struct pipe_context *_pipe,
                               void *state)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   trace_dump_call_begin("pipe_context", "bind_blend_state");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, state);

   pipe->bind_blend_state(pipe, state);

   trace_dump_call_end();
}


static INLINE void
trace_context_delete_blend_state(struct pipe_context *_pipe,
                                 void *state)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   trace_dump_call_begin("pipe_context", "delete_blend_state");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, state);

   pipe->delete_blend_state(pipe, state);

   trace_dump_call_end();
}


static INLINE void *
trace_context_create_sampler_state(struct pipe_context *_pipe,
                                   const struct pipe_sampler_state *state)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;
   void * result;

   trace_dump_call_begin("pipe_context", "create_sampler_state");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(sampler_state, state);

   result = pipe->create_sampler_state(pipe, state);

   trace_dump_ret(ptr, result);

   trace_dump_call_end();

   return result;
}


static INLINE void
trace_context_bind_fragment_sampler_states(struct pipe_context *_pipe,
                                           unsigned num_states,
                                           void **states)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   trace_dump_call_begin("pipe_context", "bind_fragment_sampler_states");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(uint, num_states);
   trace_dump_arg_array(ptr, states, num_states);

   pipe->bind_fragment_sampler_states(pipe, num_states, states);

   trace_dump_call_end();
}


static INLINE void
trace_context_bind_vertex_sampler_states(struct pipe_context *_pipe,
                                         unsigned num_states,
                                         void **states)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   if (!pipe->bind_vertex_sampler_states)
      return;

   trace_dump_call_begin("pipe_context", "bind_vertex_sampler_states");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(uint, num_states);
   trace_dump_arg_array(ptr, states, num_states);

   pipe->bind_vertex_sampler_states(pipe, num_states, states);

   trace_dump_call_end();
}


static INLINE void
trace_context_delete_sampler_state(struct pipe_context *_pipe,
                                   void *state)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   trace_dump_call_begin("pipe_context", "delete_sampler_state");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, state);

   pipe->delete_sampler_state(pipe, state);

   trace_dump_call_end();
}


static INLINE void *
trace_context_create_rasterizer_state(struct pipe_context *_pipe,
                                      const struct pipe_rasterizer_state *state)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;
   void * result;

   trace_dump_call_begin("pipe_context", "create_rasterizer_state");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(rasterizer_state, state);

   result = pipe->create_rasterizer_state(pipe, state);

   trace_dump_ret(ptr, result);

   trace_dump_call_end();

   return result;
}


static INLINE void
trace_context_bind_rasterizer_state(struct pipe_context *_pipe,
                                    void *state)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   trace_dump_call_begin("pipe_context", "bind_rasterizer_state");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, state);

   pipe->bind_rasterizer_state(pipe, state);

   trace_dump_call_end();
}


static INLINE void
trace_context_delete_rasterizer_state(struct pipe_context *_pipe,
                                      void *state)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   trace_dump_call_begin("pipe_context", "delete_rasterizer_state");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, state);

   pipe->delete_rasterizer_state(pipe, state);

   trace_dump_call_end();
}


static INLINE void *
trace_context_create_depth_stencil_alpha_state(struct pipe_context *_pipe,
                                               const struct pipe_depth_stencil_alpha_state *state)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;
   void * result;

   trace_dump_call_begin("pipe_context", "create_depth_stencil_alpha_state");

   result = pipe->create_depth_stencil_alpha_state(pipe, state);

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(depth_stencil_alpha_state, state);

   trace_dump_ret(ptr, result);

   trace_dump_call_end();

   return result;
}


static INLINE void
trace_context_bind_depth_stencil_alpha_state(struct pipe_context *_pipe,
                                             void *state)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   trace_dump_call_begin("pipe_context", "bind_depth_stencil_alpha_state");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, state);

   pipe->bind_depth_stencil_alpha_state(pipe, state);

   trace_dump_call_end();
}


static INLINE void
trace_context_delete_depth_stencil_alpha_state(struct pipe_context *_pipe,
                                               void *state)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   trace_dump_call_begin("pipe_context", "delete_depth_stencil_alpha_state");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, state);

   pipe->delete_depth_stencil_alpha_state(pipe, state);

   trace_dump_call_end();
}


static INLINE void *
trace_context_create_fs_state(struct pipe_context *_pipe,
                              const struct pipe_shader_state *state)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;
   void * result;

   trace_dump_call_begin("pipe_context", "create_fs_state");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(shader_state, state);

   result = pipe->create_fs_state(pipe, state);

   trace_dump_ret(ptr, result);

   trace_dump_call_end();

   return result;
}


static INLINE void
trace_context_bind_fs_state(struct pipe_context *_pipe,
                            void *state)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   trace_dump_call_begin("pipe_context", "bind_fs_state");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, state);

   pipe->bind_fs_state(pipe, state);

   trace_dump_call_end();
}


static INLINE void
trace_context_delete_fs_state(struct pipe_context *_pipe,
                              void *state)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   trace_dump_call_begin("pipe_context", "delete_fs_state");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, state);

   pipe->delete_fs_state(pipe, state);

   trace_dump_call_end();
}


static INLINE void *
trace_context_create_vs_state(struct pipe_context *_pipe,
                              const struct pipe_shader_state *state)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;
   void * result;

   trace_dump_call_begin("pipe_context", "create_vs_state");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(shader_state, state);

   result = pipe->create_vs_state(pipe, state);

   trace_dump_ret(ptr, result);

   trace_dump_call_end();

   return result;
}


static INLINE void
trace_context_bind_vs_state(struct pipe_context *_pipe,
                            void *state)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   trace_dump_call_begin("pipe_context", "bind_vs_state");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, state);

   pipe->bind_vs_state(pipe, state);

   trace_dump_call_end();
}


static INLINE void
trace_context_delete_vs_state(struct pipe_context *_pipe,
                              void *state)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   trace_dump_call_begin("pipe_context", "delete_vs_state");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, state);

   pipe->delete_vs_state(pipe, state);

   trace_dump_call_end();
}


static INLINE void *
trace_context_create_vertex_elements_state(struct pipe_context *_pipe,
                                           unsigned num_elements,
                                           const struct  pipe_vertex_element *elements)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;
   void * result;

   trace_dump_call_begin("pipe_context", "create_vertex_elements_state");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(uint, num_elements);

   trace_dump_arg_begin("elements");
   trace_dump_struct_array(vertex_element, elements, num_elements);
   trace_dump_arg_end();

   result = pipe->create_vertex_elements_state(pipe, num_elements, elements);

   trace_dump_ret(ptr, result);

   trace_dump_call_end();

   return result;
}


static INLINE void
trace_context_bind_vertex_elements_state(struct pipe_context *_pipe,
                                         void *state)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   trace_dump_call_begin("pipe_context", "bind_vertex_elements_state");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, state);

   pipe->bind_vertex_elements_state(pipe, state);

   trace_dump_call_end();
}


static INLINE void
trace_context_delete_vertex_elements_state(struct pipe_context *_pipe,
                                           void *state)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   trace_dump_call_begin("pipe_context", "delete_vertex_elements_state");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, state);

   pipe->delete_vertex_elements_state(pipe, state);

   trace_dump_call_end();
}


static INLINE void
trace_context_set_blend_color(struct pipe_context *_pipe,
                              const struct pipe_blend_color *state)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   trace_dump_call_begin("pipe_context", "set_blend_color");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(blend_color, state);

   pipe->set_blend_color(pipe, state);

   trace_dump_call_end();
}


static INLINE void
trace_context_set_stencil_ref(struct pipe_context *_pipe,
                              const struct pipe_stencil_ref *state)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   trace_dump_call_begin("pipe_context", "set_stencil_ref");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(stencil_ref, state);

   pipe->set_stencil_ref(pipe, state);

   trace_dump_call_end();
}


static INLINE void
trace_context_set_clip_state(struct pipe_context *_pipe,
                             const struct pipe_clip_state *state)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   trace_dump_call_begin("pipe_context", "set_clip_state");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(clip_state, state);

   pipe->set_clip_state(pipe, state);

   trace_dump_call_end();
}

static INLINE void
trace_context_set_sample_mask(struct pipe_context *_pipe,
                              unsigned sample_mask)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   trace_dump_call_begin("pipe_context", "set_sample_mask");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(uint, sample_mask);

   pipe->set_sample_mask(pipe, sample_mask);

   trace_dump_call_end();
}

static INLINE void
trace_context_set_constant_buffer(struct pipe_context *_pipe,
                                  uint shader, uint index,
                                  struct pipe_resource *buffer)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   if (buffer) {
      buffer = trace_resource_unwrap(tr_ctx, buffer);
   }

   trace_dump_call_begin("pipe_context", "set_constant_buffer");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(uint, shader);
   trace_dump_arg(uint, index);
   trace_dump_arg(ptr, buffer);

   pipe->set_constant_buffer(pipe, shader, index, buffer);

   trace_dump_call_end();
}


static INLINE void
trace_context_set_framebuffer_state(struct pipe_context *_pipe,
                                    const struct pipe_framebuffer_state *state)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;
   struct pipe_framebuffer_state unwrapped_state;
   unsigned i;


   /* Unwrap the input state */
   memcpy(&unwrapped_state, state, sizeof(unwrapped_state));
   for(i = 0; i < state->nr_cbufs; ++i)
      unwrapped_state.cbufs[i] = trace_surface_unwrap(tr_ctx, state->cbufs[i]);
   for(i = state->nr_cbufs; i < PIPE_MAX_COLOR_BUFS; ++i)
      unwrapped_state.cbufs[i] = NULL;
   unwrapped_state.zsbuf = trace_surface_unwrap(tr_ctx, state->zsbuf);
   state = &unwrapped_state;

   trace_dump_call_begin("pipe_context", "set_framebuffer_state");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(framebuffer_state, state);

   pipe->set_framebuffer_state(pipe, state);

   trace_dump_call_end();
}


static INLINE void
trace_context_set_polygon_stipple(struct pipe_context *_pipe,
                                  const struct pipe_poly_stipple *state)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   trace_dump_call_begin("pipe_context", "set_polygon_stipple");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(poly_stipple, state);

   pipe->set_polygon_stipple(pipe, state);

   trace_dump_call_end();
}


static INLINE void
trace_context_set_scissor_state(struct pipe_context *_pipe,
                                const struct pipe_scissor_state *state)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   trace_dump_call_begin("pipe_context", "set_scissor_state");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(scissor_state, state);

   pipe->set_scissor_state(pipe, state);

   trace_dump_call_end();
}


static INLINE void
trace_context_set_viewport_state(struct pipe_context *_pipe,
                                 const struct pipe_viewport_state *state)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   trace_dump_call_begin("pipe_context", "set_viewport_state");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(viewport_state, state);

   pipe->set_viewport_state(pipe, state);

   trace_dump_call_end();
}


static struct pipe_sampler_view *
trace_create_sampler_view(struct pipe_context *_pipe,
                          struct pipe_resource *_resource,
                          const struct pipe_sampler_view *templ)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct trace_resource *tr_res = trace_resource(_resource);
   struct pipe_context *pipe = tr_ctx->pipe;
   struct pipe_resource *resource = tr_res->resource;
   struct pipe_sampler_view *result;
   struct trace_sampler_view *tr_view;

   trace_dump_call_begin("pipe_context", "create_sampler_view");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, resource);

   trace_dump_arg_begin("templ");
   trace_dump_sampler_view_template(templ, resource->target);
   trace_dump_arg_end();

   result = pipe->create_sampler_view(pipe, resource, templ);

   trace_dump_ret(ptr, result);

   trace_dump_call_end();

   /*
    * Wrap pipe_sampler_view
    */
   tr_view = CALLOC_STRUCT(trace_sampler_view);
   tr_view->base = *templ;
   tr_view->base.reference.count = 1;
   tr_view->base.texture = NULL;
   pipe_resource_reference(&tr_view->base.texture, _resource);
   tr_view->base.context = _pipe;
   tr_view->sampler_view = result;
   result = &tr_view->base;

   return result;
}


static void
trace_sampler_view_destroy(struct pipe_context *_pipe,
                           struct pipe_sampler_view *_view)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct trace_sampler_view *tr_view = trace_sampler_view(_view);
   struct pipe_context *pipe = tr_ctx->pipe;
   struct pipe_sampler_view *view = tr_view->sampler_view;

   trace_dump_call_begin("pipe_context", "sampler_view_destroy");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, view);

   pipe_sampler_view_reference(&tr_view->sampler_view, NULL);

   trace_dump_call_end();

   pipe_resource_reference(&_view->texture, NULL);
   FREE(_view);
}

/********************************************************************
 * surface
 */


static struct pipe_surface *
trace_create_surface(struct pipe_context *_pipe,
                     struct pipe_resource *_resource,
                     const struct pipe_surface *surf_tmpl)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct trace_resource *tr_res = trace_resource(_resource);
   struct pipe_context *pipe = tr_ctx->pipe;
   struct pipe_resource *resource = tr_res->resource;
   struct pipe_surface *result = NULL;

   trace_dump_call_begin("pipe_context", "create_surface");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, resource);
   
   trace_dump_arg_begin("surf_tmpl");
   trace_dump_surface_template(surf_tmpl, resource->target);
   trace_dump_arg_end();


   result = pipe->create_surface(pipe, resource, surf_tmpl);

   trace_dump_ret(ptr, result);

   trace_dump_call_end();

   result = trace_surf_create(tr_res, result);

   return result;
}


static void
trace_surface_destroy(struct pipe_context *_pipe,
                      struct pipe_surface *_surface)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;
   struct trace_surface *tr_surf = trace_surface(_surface);
   struct pipe_surface *surface = tr_surf->surface;

   trace_dump_call_begin("pipe_context", "surface_destroy");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, surface);

   trace_dump_call_end();

   trace_surf_destroy(tr_surf);
}


static INLINE void
trace_context_set_fragment_sampler_views(struct pipe_context *_pipe,
                                         unsigned num,
                                         struct pipe_sampler_view **views)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct trace_sampler_view *tr_view;
   struct pipe_context *pipe = tr_ctx->pipe;
   struct pipe_sampler_view *unwrapped_views[PIPE_MAX_SAMPLERS];
   unsigned i;

   for(i = 0; i < num; ++i) {
      tr_view = trace_sampler_view(views[i]);
      unwrapped_views[i] = tr_view ? tr_view->sampler_view : NULL;
   }
   views = unwrapped_views;

   trace_dump_call_begin("pipe_context", "set_fragment_sampler_views");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(uint, num);
   trace_dump_arg_array(ptr, views, num);

   pipe->set_fragment_sampler_views(pipe, num, views);

   trace_dump_call_end();
}


static INLINE void
trace_context_set_vertex_sampler_views(struct pipe_context *_pipe,
                                       unsigned num,
                                       struct pipe_sampler_view **views)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct trace_sampler_view *tr_view;
   struct pipe_context *pipe = tr_ctx->pipe;
   struct pipe_sampler_view *unwrapped_views[PIPE_MAX_VERTEX_SAMPLERS];
   unsigned i;

   if (!pipe->set_vertex_sampler_views)
      return;

   for(i = 0; i < num; ++i) {
      tr_view = trace_sampler_view(views[i]);
      unwrapped_views[i] = tr_view ? tr_view->sampler_view : NULL;
   }
   views = unwrapped_views;

   trace_dump_call_begin("pipe_context", "set_vertex_sampler_views");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(uint, num);
   trace_dump_arg_array(ptr, views, num);

   pipe->set_vertex_sampler_views(pipe, num, views);

   trace_dump_call_end();
}


static INLINE void
trace_context_set_vertex_buffers(struct pipe_context *_pipe,
                                 unsigned num_buffers,
                                 const struct pipe_vertex_buffer *buffers)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;
   unsigned i;

   trace_dump_call_begin("pipe_context", "set_vertex_buffers");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(uint, num_buffers);

   trace_dump_arg_begin("buffers");
   trace_dump_struct_array(vertex_buffer, buffers, num_buffers);
   trace_dump_arg_end();

   if (num_buffers) {
      struct pipe_vertex_buffer *_buffers = MALLOC(num_buffers * sizeof(*_buffers));
      memcpy(_buffers, buffers, num_buffers * sizeof(*_buffers));
      for (i = 0; i < num_buffers; i++)
         _buffers[i].buffer = trace_resource_unwrap(tr_ctx, buffers[i].buffer);
      pipe->set_vertex_buffers(pipe, num_buffers, _buffers);
      FREE(_buffers);
   } else {
      pipe->set_vertex_buffers(pipe, num_buffers, NULL);
   }

   trace_dump_call_end();
}


static INLINE void
trace_context_set_index_buffer(struct pipe_context *_pipe,
                               const struct pipe_index_buffer *ib)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   trace_dump_call_begin("pipe_context", "set_index_buffer");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(index_buffer, ib);

   if (ib) {
      struct pipe_index_buffer _ib;
      _ib = *ib;
      _ib.buffer = trace_resource_unwrap(tr_ctx, ib->buffer);
      pipe->set_index_buffer(pipe, &_ib);
   } else {
      pipe->set_index_buffer(pipe, NULL);
   }

   trace_dump_call_end();
}

static INLINE void
trace_context_resource_copy_region(struct pipe_context *_pipe,
                                   struct pipe_resource *dst,
                                   unsigned dst_level,
                                   unsigned dstx, unsigned dsty, unsigned dstz,
                                   struct pipe_resource *src,
                                   unsigned src_level,
                                   const struct pipe_box *src_box)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   dst = trace_resource_unwrap(tr_ctx, dst);
   src = trace_resource_unwrap(tr_ctx, src);

   trace_dump_call_begin("pipe_context", "resource_copy_region");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, dst);
   trace_dump_arg(uint, dst_level);
   trace_dump_arg(uint, dstx);
   trace_dump_arg(uint, dsty);
   trace_dump_arg(uint, dstz);
   trace_dump_arg(ptr, src);
   trace_dump_arg(uint, src_level);
   trace_dump_arg(box, src_box);

   pipe->resource_copy_region(pipe,
                              dst, dst_level, dstx, dsty, dstz,
                              src, src_level, src_box);

   trace_dump_call_end();
}


static INLINE void
trace_context_clear(struct pipe_context *_pipe,
                    unsigned buffers,
                    const float *rgba,
                    double depth,
                    unsigned stencil)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   trace_dump_call_begin("pipe_context", "clear");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(uint, buffers);
   if (rgba)
      trace_dump_arg_array(float, rgba, 4);
   else
      trace_dump_null();
   trace_dump_arg(float, depth);
   trace_dump_arg(uint, stencil);

   pipe->clear(pipe, buffers, rgba, depth, stencil);

   trace_dump_call_end();
}


static INLINE void
trace_context_clear_render_target(struct pipe_context *_pipe,
                                  struct pipe_surface *dst,
                                  const float *rgba,
                                  unsigned dstx, unsigned dsty,
                                  unsigned width, unsigned height)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   dst = trace_surface_unwrap(tr_ctx, dst);

   trace_dump_call_begin("pipe_context", "clear_render_target");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, dst);
   trace_dump_arg_array(float, rgba, 4);
   trace_dump_arg(uint, dstx);
   trace_dump_arg(uint, dsty);
   trace_dump_arg(uint, width);
   trace_dump_arg(uint, height);

   pipe->clear_render_target(pipe, dst, rgba, dstx, dsty, width, height);

   trace_dump_call_end();
}

static INLINE void
trace_context_clear_depth_stencil(struct pipe_context *_pipe,
                                  struct pipe_surface *dst,
                                  unsigned clear_flags,
                                  double depth,
                                  unsigned stencil,
                                  unsigned dstx, unsigned dsty,
                                  unsigned width, unsigned height)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   dst = trace_surface_unwrap(tr_ctx, dst);

   trace_dump_call_begin("pipe_context", "clear_depth_stencil");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, dst);
   trace_dump_arg(uint, clear_flags);
   trace_dump_arg(float, depth);
   trace_dump_arg(uint, stencil);
   trace_dump_arg(uint, dstx);
   trace_dump_arg(uint, dsty);
   trace_dump_arg(uint, width);
   trace_dump_arg(uint, height);

   pipe->clear_depth_stencil(pipe, dst, clear_flags, depth, stencil,
                             dstx, dsty, width, height);

   trace_dump_call_end();
}

static INLINE void
trace_context_flush(struct pipe_context *_pipe,
                    struct pipe_fence_handle **fence)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   trace_dump_call_begin("pipe_context", "flush");

   trace_dump_arg(ptr, pipe);

   pipe->flush(pipe, fence);

   if(fence)
      trace_dump_ret(ptr, *fence);

   trace_dump_call_end();
}


static INLINE void
trace_context_destroy(struct pipe_context *_pipe)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   trace_dump_call_begin("pipe_context", "destroy");
   trace_dump_arg(ptr, pipe);
   trace_dump_call_end();

   pipe->destroy(pipe);

   FREE(tr_ctx);
}


/********************************************************************
 * transfer
 */


static struct pipe_transfer *
trace_context_get_transfer(struct pipe_context *_context,
                           struct pipe_resource *_resource,
                           unsigned level,
                           unsigned usage,
                           const struct pipe_box *box)
{
   struct trace_context *tr_context = trace_context(_context);
   struct trace_resource *tr_res = trace_resource(_resource);
   struct pipe_context *context = tr_context->pipe;
   struct pipe_resource *texture = tr_res->resource;
   struct pipe_transfer *result = NULL;

   assert(texture->screen == context->screen);

   /*
    * Map and transfers can't be serialized so we convert all write transfers
    * to transfer_inline_write and ignore read transfers.
    */

   result = context->get_transfer(context, texture, level, usage, box);

   if (result)
      result = trace_transfer_create(tr_context, tr_res, result);

   return result;
}


static void
trace_context_transfer_destroy(struct pipe_context *_context,
                               struct pipe_transfer *_transfer)
{
   struct trace_context *tr_context = trace_context(_context);
   struct trace_transfer *tr_trans = trace_transfer(_transfer);

   trace_transfer_destroy(tr_context, tr_trans);
}


static void *
trace_context_transfer_map(struct pipe_context *_context,
                          struct pipe_transfer *_transfer)
{
   struct trace_context *tr_context = trace_context(_context);
   struct trace_transfer *tr_trans = trace_transfer(_transfer);
   struct pipe_context *context = tr_context->pipe;
   struct pipe_transfer *transfer = tr_trans->transfer;
   void *map;

   map = context->transfer_map(context, transfer);
   if(map) {
      if(transfer->usage & PIPE_TRANSFER_WRITE) {
         assert(!tr_trans->map);
         tr_trans->map = map;
      }
   }

   return map;
}


static void
trace_context_transfer_flush_region( struct pipe_context *_context,
				     struct pipe_transfer *_transfer,
				     const struct pipe_box *box)
{
   struct trace_context *tr_context = trace_context(_context);
   struct trace_transfer *tr_transfer = trace_transfer(_transfer);
   struct pipe_context *context = tr_context->pipe;
   struct pipe_transfer *transfer = tr_transfer->transfer;

   context->transfer_flush_region(context,
				  transfer,
				  box);
}

static void
trace_context_transfer_unmap(struct pipe_context *_context,
                             struct pipe_transfer *_transfer)
{
   struct trace_context *tr_ctx = trace_context(_context);
   struct trace_transfer *tr_trans = trace_transfer(_transfer);
   struct pipe_context *context = tr_ctx->pipe;
   struct pipe_transfer *transfer = tr_trans->transfer;

   if(tr_trans->map) {
      /*
       * Fake a transfer_inline_write
       */

      struct pipe_resource *resource = transfer->resource;
      unsigned level = transfer->level;
      unsigned usage = transfer->usage;
      const struct pipe_box *box = &transfer->box;
      unsigned stride = transfer->stride;
      unsigned layer_stride = transfer->layer_stride;

      trace_dump_call_begin("pipe_context", "transfer_inline_write");

      trace_dump_arg(ptr, context);
      trace_dump_arg(ptr, resource);
      trace_dump_arg(uint, level);
      trace_dump_arg(uint, usage);
      trace_dump_arg(box, box);

      trace_dump_arg_begin("data");
      trace_dump_box_bytes(tr_trans->map,
                           resource->format,
                           box,
                           stride,
                           layer_stride);
      trace_dump_arg_end();

      trace_dump_arg(uint, stride);
      trace_dump_arg(uint, layer_stride);

      trace_dump_call_end();

      tr_trans->map = NULL;
   }

   context->transfer_unmap(context, transfer);
}


static void
trace_context_transfer_inline_write(struct pipe_context *_context,
                                    struct pipe_resource *_resource,
                                    unsigned level,
                                    unsigned usage,
                                    const struct pipe_box *box,
                                    const void *data,
                                    unsigned stride,
                                    unsigned layer_stride)
{
   struct trace_context *tr_context = trace_context(_context);
   struct trace_resource *tr_res = trace_resource(_resource);
   struct pipe_context *context = tr_context->pipe;
   struct pipe_resource *resource = tr_res->resource;

   assert(resource->screen == context->screen);

   trace_dump_call_begin("pipe_context", "transfer_inline_write");

   trace_dump_arg(ptr, context);
   trace_dump_arg(ptr, resource);
   trace_dump_arg(uint, level);
   trace_dump_arg(uint, usage);
   trace_dump_arg(box, box);

   trace_dump_arg_begin("data");
   trace_dump_box_bytes(data,
                        resource->format,
                        box,
                        stride,
                        layer_stride);
   trace_dump_arg_end();

   trace_dump_arg(uint, stride);
   trace_dump_arg(uint, layer_stride);

   trace_dump_call_end();

   context->transfer_inline_write(context, resource,
                                  level, usage, box, data, stride, layer_stride);
}


static void trace_redefine_user_buffer(struct pipe_context *_context,
                                       struct pipe_resource *_resource,
                                       unsigned offset, unsigned size)
{
   struct trace_context *tr_context = trace_context(_context);
   struct trace_resource *tr_res = trace_resource(_resource);
   struct pipe_context *context = tr_context->pipe;
   struct pipe_resource *resource = tr_res->resource;

   assert(resource->screen == context->screen);

   trace_dump_call_begin("pipe_context", "redefine_user_buffer");

   trace_dump_arg(ptr, context);
   trace_dump_arg(ptr, resource);
   trace_dump_arg(uint, offset);
   trace_dump_arg(uint, size);

   trace_dump_call_end();

   context->redefine_user_buffer(context, resource, offset, size);
}


static void trace_render_condition(struct pipe_context *_context,
                                   struct pipe_query *query,
                                   uint mode)
{
   struct trace_context *tr_context = trace_context(_context);
   struct pipe_context *context = tr_context->pipe;

   trace_dump_call_begin("pipe_context", "render_condition");

   trace_dump_arg(ptr, context);
   trace_dump_arg(ptr, query);
   trace_dump_arg(uint, mode);

   trace_dump_call_end();

   context->render_condition(context, query, mode);
}


static void trace_texture_barrier(struct pipe_context *_context)
{
   struct trace_context *tr_context = trace_context(_context);
   struct pipe_context *context = tr_context->pipe;

   trace_dump_call_begin("pipe_context", "texture_barrier");

   trace_dump_arg(ptr, context);

   trace_dump_call_end();

   context->texture_barrier(context);
}


static const struct debug_named_value rbug_blocker_flags[] = {
   {"before", 1, NULL},
   {"after", 2, NULL},
   DEBUG_NAMED_VALUE_END
};

struct pipe_context *
trace_context_create(struct trace_screen *tr_scr,
                     struct pipe_context *pipe)
{
   struct trace_context *tr_ctx;

   if(!pipe)
      goto error1;

   if(!trace_enabled())
      goto error1;

   tr_ctx = CALLOC_STRUCT(trace_context);
   if(!tr_ctx)
      goto error1;

   tr_ctx->base.winsys = NULL;
   tr_ctx->base.priv = pipe->priv; /* expose wrapped priv data */
   tr_ctx->base.screen = &tr_scr->base;

   tr_ctx->base.destroy = trace_context_destroy;
   tr_ctx->base.draw_vbo = trace_context_draw_vbo;
   tr_ctx->base.create_query = trace_context_create_query;
   tr_ctx->base.destroy_query = trace_context_destroy_query;
   tr_ctx->base.begin_query = trace_context_begin_query;
   tr_ctx->base.end_query = trace_context_end_query;
   tr_ctx->base.get_query_result = trace_context_get_query_result;
   tr_ctx->base.create_blend_state = trace_context_create_blend_state;
   tr_ctx->base.bind_blend_state = trace_context_bind_blend_state;
   tr_ctx->base.delete_blend_state = trace_context_delete_blend_state;
   tr_ctx->base.create_sampler_state = trace_context_create_sampler_state;
   tr_ctx->base.bind_fragment_sampler_states = trace_context_bind_fragment_sampler_states;
   tr_ctx->base.bind_vertex_sampler_states = trace_context_bind_vertex_sampler_states;
   tr_ctx->base.delete_sampler_state = trace_context_delete_sampler_state;
   tr_ctx->base.create_rasterizer_state = trace_context_create_rasterizer_state;
   tr_ctx->base.bind_rasterizer_state = trace_context_bind_rasterizer_state;
   tr_ctx->base.delete_rasterizer_state = trace_context_delete_rasterizer_state;
   tr_ctx->base.create_depth_stencil_alpha_state = trace_context_create_depth_stencil_alpha_state;
   tr_ctx->base.bind_depth_stencil_alpha_state = trace_context_bind_depth_stencil_alpha_state;
   tr_ctx->base.delete_depth_stencil_alpha_state = trace_context_delete_depth_stencil_alpha_state;
   tr_ctx->base.create_fs_state = trace_context_create_fs_state;
   tr_ctx->base.bind_fs_state = trace_context_bind_fs_state;
   tr_ctx->base.delete_fs_state = trace_context_delete_fs_state;
   tr_ctx->base.create_vs_state = trace_context_create_vs_state;
   tr_ctx->base.bind_vs_state = trace_context_bind_vs_state;
   tr_ctx->base.delete_vs_state = trace_context_delete_vs_state;
   tr_ctx->base.create_vertex_elements_state = trace_context_create_vertex_elements_state;
   tr_ctx->base.bind_vertex_elements_state = trace_context_bind_vertex_elements_state;
   tr_ctx->base.delete_vertex_elements_state = trace_context_delete_vertex_elements_state;
   tr_ctx->base.set_blend_color = trace_context_set_blend_color;
   tr_ctx->base.set_stencil_ref = trace_context_set_stencil_ref;
   tr_ctx->base.set_clip_state = trace_context_set_clip_state;
   tr_ctx->base.set_sample_mask = trace_context_set_sample_mask;
   tr_ctx->base.set_constant_buffer = trace_context_set_constant_buffer;
   tr_ctx->base.set_framebuffer_state = trace_context_set_framebuffer_state;
   tr_ctx->base.set_polygon_stipple = trace_context_set_polygon_stipple;
   tr_ctx->base.set_scissor_state = trace_context_set_scissor_state;
   tr_ctx->base.set_viewport_state = trace_context_set_viewport_state;
   tr_ctx->base.set_fragment_sampler_views = trace_context_set_fragment_sampler_views;
   tr_ctx->base.set_vertex_sampler_views = trace_context_set_vertex_sampler_views;
   tr_ctx->base.create_sampler_view = trace_create_sampler_view;
   tr_ctx->base.sampler_view_destroy = trace_sampler_view_destroy;
   tr_ctx->base.create_surface = trace_create_surface;
   tr_ctx->base.surface_destroy = trace_surface_destroy;
   tr_ctx->base.set_vertex_buffers = trace_context_set_vertex_buffers;
   tr_ctx->base.set_index_buffer = trace_context_set_index_buffer;
   tr_ctx->base.resource_copy_region = trace_context_resource_copy_region;
   tr_ctx->base.clear = trace_context_clear;
   tr_ctx->base.clear_render_target = trace_context_clear_render_target;
   tr_ctx->base.clear_depth_stencil = trace_context_clear_depth_stencil;
   tr_ctx->base.flush = trace_context_flush;
   tr_ctx->base.render_condition = pipe->render_condition ? trace_render_condition : NULL;
   tr_ctx->base.texture_barrier = pipe->texture_barrier ? trace_texture_barrier : NULL;

   tr_ctx->base.get_transfer = trace_context_get_transfer;
   tr_ctx->base.transfer_destroy = trace_context_transfer_destroy;
   tr_ctx->base.transfer_map = trace_context_transfer_map;
   tr_ctx->base.transfer_unmap = trace_context_transfer_unmap;
   tr_ctx->base.transfer_flush_region = trace_context_transfer_flush_region;
   tr_ctx->base.transfer_inline_write = trace_context_transfer_inline_write;
   tr_ctx->base.redefine_user_buffer = trace_redefine_user_buffer;

   tr_ctx->pipe = pipe;

   return &tr_ctx->base;

error1:
   return pipe;
}

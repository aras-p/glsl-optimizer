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

#include "util/u_memory.h"
#include "pipe/p_screen.h"

#include "tr_dump.h"
#include "tr_state.h"
#include "tr_screen.h"
#include "tr_texture.h"
#include "tr_winsys.h"
#include "tr_context.h"


static INLINE struct pipe_texture * 
trace_texture_unwrap(struct trace_context *tr_ctx,
                     struct pipe_texture *texture)
{
   struct trace_screen *tr_scr = trace_screen(tr_ctx->base.screen); 
   struct trace_texture *tr_tex;
   
   if(!texture)
      return NULL;
   
   tr_tex = trace_texture(tr_scr, texture);
   
   assert(tr_tex->texture);
   assert(tr_tex->texture->screen == tr_scr->screen);
   return tr_tex->texture;
}


static INLINE struct pipe_surface * 
trace_surface_unwrap(struct trace_context *tr_ctx,
                     struct pipe_surface *surface)
{
   struct trace_screen *tr_scr = trace_screen(tr_ctx->base.screen); 
   struct trace_texture *tr_tex;
   struct trace_surface *tr_surf;
   
   if(!surface)
      return NULL;

   assert(surface->texture);
   if(!surface->texture)
      return surface;
   
   tr_tex = trace_texture(tr_scr, surface->texture);
   tr_surf = trace_surface(tr_tex, surface);
   
   assert(tr_surf->surface);
   assert(tr_surf->surface->texture->screen == tr_scr->screen);
   return tr_surf->surface;
}


static INLINE void
trace_context_set_edgeflags(struct pipe_context *_pipe,
                            const unsigned *bitfield)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   trace_dump_call_begin("pipe_context", "set_edgeflags");
   
   trace_dump_arg(ptr, pipe);
   /* FIXME: we don't know how big this array is */
   trace_dump_arg(ptr, bitfield);

   pipe->set_edgeflags(pipe, bitfield);;

   trace_dump_call_end();
}


static INLINE boolean
trace_context_draw_arrays(struct pipe_context *_pipe,
                          unsigned mode, unsigned start, unsigned count)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;
   boolean result;

   trace_dump_call_begin("pipe_context", "draw_arrays");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(uint, mode);
   trace_dump_arg(uint, start);
   trace_dump_arg(uint, count);

   result = pipe->draw_arrays(pipe, mode, start, count);;

   trace_dump_ret(bool, result);
   
   trace_dump_call_end();
   
   return result;
}


static INLINE boolean
trace_context_draw_elements(struct pipe_context *_pipe,
                          struct pipe_buffer *indexBuffer,
                          unsigned indexSize,
                          unsigned mode, unsigned start, unsigned count)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;
   boolean result;

   trace_winsys_user_buffer_update(_pipe->winsys, indexBuffer);

   trace_dump_call_begin("pipe_context", "draw_elements");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, indexBuffer);
   trace_dump_arg(uint, indexSize);
   trace_dump_arg(uint, mode);
   trace_dump_arg(uint, start);
   trace_dump_arg(uint, count);

   result = pipe->draw_elements(pipe, indexBuffer, indexSize, mode, start, count);;

   trace_dump_ret(bool, result);
   
   trace_dump_call_end();
   
   return result;
}


static INLINE boolean
trace_context_draw_range_elements(struct pipe_context *_pipe,
                                  struct pipe_buffer *indexBuffer,
                                  unsigned indexSize,
                                  unsigned minIndex,
                                  unsigned maxIndex,
                                  unsigned mode, 
                                  unsigned start, 
                                  unsigned count)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;
   boolean result;

   trace_winsys_user_buffer_update(_pipe->winsys, indexBuffer);

   trace_dump_call_begin("pipe_context", "draw_range_elements");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, indexBuffer);
   trace_dump_arg(uint, indexSize);
   trace_dump_arg(uint, minIndex);
   trace_dump_arg(uint, maxIndex);
   trace_dump_arg(uint, mode);
   trace_dump_arg(uint, start);
   trace_dump_arg(uint, count);

   result = pipe->draw_range_elements(pipe, 
                                      indexBuffer, 
                                      indexSize, minIndex, maxIndex, 
                                      mode, start, count);
   
   trace_dump_ret(bool, result);
   
   trace_dump_call_end();
   
   return result;
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

   result = pipe->create_query(pipe, query_type);;

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

   pipe->destroy_query(pipe, query);;

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

   pipe->begin_query(pipe, query);;

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
                               uint64 *presult)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;
   uint64 result;
   boolean _result;

   trace_dump_call_begin("pipe_context", "get_query_result");

   trace_dump_arg(ptr, pipe);

   _result = pipe->get_query_result(pipe, query, wait, presult);;
   result = *presult;

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

   result = pipe->create_blend_state(pipe, state);;

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

   pipe->bind_blend_state(pipe, state);;

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

   pipe->delete_blend_state(pipe, state);;

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

   result = pipe->create_sampler_state(pipe, state);;

   trace_dump_ret(ptr, result);
   
   trace_dump_call_end();
   
   return result;
}


static INLINE void
trace_context_bind_sampler_states(struct pipe_context *_pipe, 
                                  unsigned num_states, void **states)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   trace_dump_call_begin("pipe_context", "bind_sampler_states");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(uint, num_states);
   trace_dump_arg_array(ptr, states, num_states);

   pipe->bind_sampler_states(pipe, num_states, states);;

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

   pipe->delete_sampler_state(pipe, state);;

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

   result = pipe->create_rasterizer_state(pipe, state);;

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

   pipe->bind_rasterizer_state(pipe, state);;

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

   pipe->delete_rasterizer_state(pipe, state);;

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

   result = pipe->create_depth_stencil_alpha_state(pipe, state);;

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

   pipe->bind_depth_stencil_alpha_state(pipe, state);;

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

   pipe->delete_depth_stencil_alpha_state(pipe, state);;

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

   result = pipe->create_fs_state(pipe, state);;

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

   pipe->bind_fs_state(pipe, state);;

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

   pipe->delete_fs_state(pipe, state);;

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

   result = pipe->create_vs_state(pipe, state);;

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

   pipe->bind_vs_state(pipe, state);;

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

   pipe->delete_vs_state(pipe, state);;

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

   pipe->set_blend_color(pipe, state);;

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

   pipe->set_clip_state(pipe, state);;

   trace_dump_call_end();
}


static INLINE void
trace_context_set_constant_buffer(struct pipe_context *_pipe,
                                  uint shader, uint index,
                                  const struct pipe_constant_buffer *buffer)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   trace_winsys_user_buffer_update(_pipe->winsys, (struct pipe_buffer *)buffer);
   
   trace_dump_call_begin("pipe_context", "set_constant_buffer");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(uint, shader);
   trace_dump_arg(uint, index);
   trace_dump_arg(constant_buffer, buffer);

   pipe->set_constant_buffer(pipe, shader, index, buffer);;

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
   for(i = 0; i < state->num_cbufs; ++i)
      unwrapped_state.cbufs[i] = trace_surface_unwrap(tr_ctx, state->cbufs[i]);
   for(i = state->num_cbufs; i < PIPE_MAX_COLOR_BUFS; ++i)
      unwrapped_state.cbufs[i] = NULL;
   unwrapped_state.zsbuf = trace_surface_unwrap(tr_ctx, state->zsbuf);
   state = &unwrapped_state;
   
   trace_dump_call_begin("pipe_context", "set_framebuffer_state");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(framebuffer_state, state);

   pipe->set_framebuffer_state(pipe, state);;

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

   pipe->set_polygon_stipple(pipe, state);;

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

   pipe->set_scissor_state(pipe, state);;

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

   pipe->set_viewport_state(pipe, state);;

   trace_dump_call_end();
}


static INLINE void
trace_context_set_sampler_textures(struct pipe_context *_pipe,
                                   unsigned num_textures,
                                   struct pipe_texture **textures)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;
   struct pipe_texture *unwrapped_textures[PIPE_MAX_SAMPLERS];
   unsigned i;
   
   for(i = 0; i < num_textures; ++i)
      unwrapped_textures[i] = trace_texture_unwrap(tr_ctx, textures[i]);
   textures = unwrapped_textures;

   trace_dump_call_begin("pipe_context", "set_sampler_textures");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(uint, num_textures);
   trace_dump_arg_array(ptr, textures, num_textures);

   pipe->set_sampler_textures(pipe, num_textures, textures);;

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

   for(i = 0; i < num_buffers; ++i)
      trace_winsys_user_buffer_update(_pipe->winsys, buffers[i].buffer);

   trace_dump_call_begin("pipe_context", "set_vertex_buffers");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(uint, num_buffers);
   
   trace_dump_arg_begin("buffers");
   trace_dump_struct_array(vertex_buffer, buffers, num_buffers);
   trace_dump_arg_end();

   pipe->set_vertex_buffers(pipe, num_buffers, buffers);;

   trace_dump_call_end();
}


static INLINE void
trace_context_set_vertex_elements(struct pipe_context *_pipe,
                                  unsigned num_elements,
                                  const struct pipe_vertex_element *elements)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   trace_dump_call_begin("pipe_context", "set_vertex_elements");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(uint, num_elements);

   trace_dump_arg_begin("elements");
   trace_dump_struct_array(vertex_element, elements, num_elements);
   trace_dump_arg_end();

   pipe->set_vertex_elements(pipe, num_elements, elements);;

   trace_dump_call_end();
}


static INLINE void
trace_context_surface_copy(struct pipe_context *_pipe,
                           boolean do_flip,
                           struct pipe_surface *dest,
                           unsigned destx, unsigned desty,
                           struct pipe_surface *src,
                           unsigned srcx, unsigned srcy,
                           unsigned width, unsigned height)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   dest = trace_surface_unwrap(tr_ctx, dest);
   src = trace_surface_unwrap(tr_ctx, src);
   
   trace_dump_call_begin("pipe_context", "surface_copy");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(bool, do_flip);
   trace_dump_arg(ptr, dest);
   trace_dump_arg(uint, destx);
   trace_dump_arg(uint, desty);
   trace_dump_arg(ptr, src);
   trace_dump_arg(uint, srcx);
   trace_dump_arg(uint, srcy);
   trace_dump_arg(uint, width);
   trace_dump_arg(uint, height);

   pipe->surface_copy(pipe, do_flip, 
                      dest, destx, desty, 
                      src, srcx, srcy, width, height);
   
   trace_dump_call_end();
}


static INLINE void
trace_context_surface_fill(struct pipe_context *_pipe,
                           struct pipe_surface *dst,
                           unsigned dstx, unsigned dsty,
                           unsigned width, unsigned height,
                           unsigned value)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   dst = trace_surface_unwrap(tr_ctx, dst);

   trace_dump_call_begin("pipe_context", "surface_fill");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, dst);
   trace_dump_arg(uint, dstx);
   trace_dump_arg(uint, dsty);
   trace_dump_arg(uint, width);
   trace_dump_arg(uint, height);

   pipe->surface_fill(pipe, dst, dstx, dsty, width, height, value);;

   trace_dump_call_end();
}


static INLINE void
trace_context_clear(struct pipe_context *_pipe, 
                    struct pipe_surface *surface,
                    unsigned clearValue)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   surface = trace_surface_unwrap(tr_ctx, surface);

   trace_dump_call_begin("pipe_context", "clear");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, surface);
   trace_dump_arg(uint, clearValue);

   pipe->clear(pipe, surface, clearValue);;

   trace_dump_call_end();
}


static INLINE void
trace_context_flush(struct pipe_context *_pipe,
                    unsigned flags,
                    struct pipe_fence_handle **fence)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   trace_dump_call_begin("pipe_context", "flush");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(uint, flags);

   pipe->flush(pipe, flags, fence);;

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

   pipe->destroy(pipe);
   
   trace_dump_call_end();

   FREE(tr_ctx);
}


struct pipe_context *
trace_context_create(struct pipe_screen *screen, 
                     struct pipe_context *pipe)
{
   struct trace_context *tr_ctx;
   
   if(!pipe)
      goto error1;
   
   if(!trace_dump_enabled())
      goto error1;
   
   tr_ctx = CALLOC_STRUCT(trace_context);
   if(!tr_ctx)
      goto error1;

   tr_ctx->base.winsys = screen->winsys;
   tr_ctx->base.screen = screen;
   tr_ctx->base.destroy = trace_context_destroy;
   tr_ctx->base.set_edgeflags = trace_context_set_edgeflags;
   tr_ctx->base.draw_arrays = trace_context_draw_arrays;
   tr_ctx->base.draw_elements = trace_context_draw_elements;
   tr_ctx->base.draw_range_elements = trace_context_draw_range_elements;
   tr_ctx->base.create_query = trace_context_create_query;
   tr_ctx->base.destroy_query = trace_context_destroy_query;
   tr_ctx->base.begin_query = trace_context_begin_query;
   tr_ctx->base.end_query = trace_context_end_query;
   tr_ctx->base.get_query_result = trace_context_get_query_result;
   tr_ctx->base.create_blend_state = trace_context_create_blend_state;
   tr_ctx->base.bind_blend_state = trace_context_bind_blend_state;
   tr_ctx->base.delete_blend_state = trace_context_delete_blend_state;
   tr_ctx->base.create_sampler_state = trace_context_create_sampler_state;
   tr_ctx->base.bind_sampler_states = trace_context_bind_sampler_states;
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
   tr_ctx->base.set_blend_color = trace_context_set_blend_color;
   tr_ctx->base.set_clip_state = trace_context_set_clip_state;
   tr_ctx->base.set_constant_buffer = trace_context_set_constant_buffer;
   tr_ctx->base.set_framebuffer_state = trace_context_set_framebuffer_state;
   tr_ctx->base.set_polygon_stipple = trace_context_set_polygon_stipple;
   tr_ctx->base.set_scissor_state = trace_context_set_scissor_state;
   tr_ctx->base.set_viewport_state = trace_context_set_viewport_state;
   tr_ctx->base.set_sampler_textures = trace_context_set_sampler_textures;
   tr_ctx->base.set_vertex_buffers = trace_context_set_vertex_buffers;
   tr_ctx->base.set_vertex_elements = trace_context_set_vertex_elements;
   tr_ctx->base.surface_copy = trace_context_surface_copy;
   tr_ctx->base.surface_fill = trace_context_surface_fill;
   tr_ctx->base.clear = trace_context_clear;
   tr_ctx->base.flush = trace_context_flush;

   tr_ctx->pipe = pipe;
   
   trace_dump_call_begin("", "pipe_context_create");
   trace_dump_arg_begin("screen");
   trace_dump_ptr(pipe->screen);
   trace_dump_arg_end();
   trace_dump_ret(ptr, pipe);
   trace_dump_call_end();

   return &tr_ctx->base;
   
error1:
   return pipe;
}

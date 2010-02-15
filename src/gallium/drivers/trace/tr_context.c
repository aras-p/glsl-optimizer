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
#include "util/u_simple_list.h"

#include "pipe/p_screen.h"

#include "tr_dump.h"
#include "tr_dump_state.h"
#include "tr_state.h"
#include "tr_buffer.h"
#include "tr_screen.h"
#include "tr_texture.h"


static INLINE struct pipe_buffer *
trace_buffer_unwrap(struct trace_context *tr_ctx,
                     struct pipe_buffer *buffer)
{
   struct trace_screen *tr_scr = trace_screen(tr_ctx->base.screen);
   struct trace_buffer *tr_buf;

   if(!buffer)
      return NULL;

   tr_buf = trace_buffer(buffer);

   assert(tr_buf->buffer);
   assert(tr_buf->buffer->screen == tr_scr->screen);
   (void) tr_scr;
   return tr_buf->buffer;
}


static INLINE struct pipe_texture *
trace_texture_unwrap(struct trace_context *tr_ctx,
                     struct pipe_texture *texture)
{
   struct trace_texture *tr_tex;

   if(!texture)
      return NULL;

   tr_tex = trace_texture(texture);

   assert(tr_tex->texture);
   return tr_tex->texture;
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
trace_context_draw_block(struct trace_context *tr_ctx, int flag)
{
   int k;

   pipe_mutex_lock(tr_ctx->draw_mutex);

   if (tr_ctx->draw_blocker & flag) {
      tr_ctx->draw_blocked |= flag;
   } else if ((tr_ctx->draw_rule.blocker & flag) &&
              (tr_ctx->draw_blocker & 4)) {
      boolean block = FALSE;
      debug_printf("%s (%p %p) (%p %p) (%p %u) (%p %u)\n", __FUNCTION__,
                   (void *) tr_ctx->draw_rule.fs, (void *) tr_ctx->curr.fs,
                   (void *) tr_ctx->draw_rule.vs, (void *) tr_ctx->curr.vs,
                   (void *) tr_ctx->draw_rule.surf, 0,
                   (void *) tr_ctx->draw_rule.tex, 0);
      if (tr_ctx->draw_rule.fs &&
          tr_ctx->draw_rule.fs == tr_ctx->curr.fs)
         block = TRUE;
      if (tr_ctx->draw_rule.vs &&
          tr_ctx->draw_rule.vs == tr_ctx->curr.vs)
         block = TRUE;
      if (tr_ctx->draw_rule.surf &&
          tr_ctx->draw_rule.surf == tr_ctx->curr.zsbuf)
            block = TRUE;
      if (tr_ctx->draw_rule.surf)
         for (k = 0; k < tr_ctx->curr.nr_cbufs; k++)
            if (tr_ctx->draw_rule.surf == tr_ctx->curr.cbufs[k])
               block = TRUE;
      if (tr_ctx->draw_rule.tex) {
         for (k = 0; k < tr_ctx->curr.num_texs; k++)
            if (tr_ctx->draw_rule.tex == tr_ctx->curr.tex[k])
               block = TRUE;
         for (k = 0; k < tr_ctx->curr.num_vert_texs; k++) {
            if (tr_ctx->draw_rule.tex == tr_ctx->curr.vert_tex[k]) {
               block = TRUE;
            }
         }
      }

      if (block)
         tr_ctx->draw_blocked |= (flag | 4);
   }

   if (tr_ctx->draw_blocked)
      trace_rbug_notify_draw_blocked(tr_ctx);

   /* wait for rbug to clear the blocked flag */
   while (tr_ctx->draw_blocked & flag) {
      tr_ctx->draw_blocked |= flag;
#ifdef PIPE_THREAD_HAVE_CONDVAR
      pipe_condvar_wait(tr_ctx->draw_cond, tr_ctx->draw_mutex);
#else
      pipe_mutex_unlock(tr_ctx->draw_mutex);
#ifdef PIPE_SUBSYSTEM_WINDOWS_USER
      Sleep(1);
#endif
      pipe_mutex_lock(tr_ctx->draw_mutex);
#endif
   }

   pipe_mutex_unlock(tr_ctx->draw_mutex);
}

static INLINE void
trace_context_draw_arrays(struct pipe_context *_pipe,
                          unsigned mode, unsigned start, unsigned count)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   if (tr_ctx->curr.fs->disabled || tr_ctx->curr.vs->disabled)
      return;

   trace_context_draw_block(tr_ctx, 1);

   trace_dump_call_begin("pipe_context", "draw_arrays");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(uint, mode);
   trace_dump_arg(uint, start);
   trace_dump_arg(uint, count);

   pipe->draw_arrays(pipe, mode, start, count);

   trace_dump_call_end();

   trace_context_draw_block(tr_ctx, 2);
}


static INLINE void
trace_context_draw_elements(struct pipe_context *_pipe,
                          struct pipe_buffer *_indexBuffer,
                          unsigned indexSize,
                          unsigned mode, unsigned start, unsigned count)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct trace_buffer *tr_buf = trace_buffer(_indexBuffer);
   struct pipe_context *pipe = tr_ctx->pipe;
   struct pipe_buffer *indexBuffer = tr_buf->buffer;

   if (tr_ctx->curr.fs->disabled || tr_ctx->curr.vs->disabled)
      return;

   trace_context_draw_block(tr_ctx, 1);

   trace_screen_user_buffer_update(_pipe->screen, indexBuffer);

   trace_dump_call_begin("pipe_context", "draw_elements");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, indexBuffer);
   trace_dump_arg(uint, indexSize);
   trace_dump_arg(uint, mode);
   trace_dump_arg(uint, start);
   trace_dump_arg(uint, count);

   pipe->draw_elements(pipe, indexBuffer, indexSize, mode, start, count);

   trace_dump_call_end();

   trace_context_draw_block(tr_ctx, 2);
}


static INLINE void
trace_context_draw_range_elements(struct pipe_context *_pipe,
                                  struct pipe_buffer *_indexBuffer,
                                  unsigned indexSize,
                                  unsigned minIndex,
                                  unsigned maxIndex,
                                  unsigned mode,
                                  unsigned start,
                                  unsigned count)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct trace_buffer *tr_buf = trace_buffer(_indexBuffer);
   struct pipe_context *pipe = tr_ctx->pipe;
   struct pipe_buffer *indexBuffer = tr_buf->buffer;

   if (tr_ctx->curr.fs->disabled || tr_ctx->curr.vs->disabled)
      return;

   trace_context_draw_block(tr_ctx, 1);

   trace_screen_user_buffer_update(_pipe->screen, indexBuffer);

   trace_dump_call_begin("pipe_context", "draw_range_elements");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, indexBuffer);
   trace_dump_arg(uint, indexSize);
   trace_dump_arg(uint, minIndex);
   trace_dump_arg(uint, maxIndex);
   trace_dump_arg(uint, mode);
   trace_dump_arg(uint, start);
   trace_dump_arg(uint, count);

   pipe->draw_range_elements(pipe,
                             indexBuffer,
                             indexSize, minIndex, maxIndex,
                             mode, start, count);

   trace_dump_call_end();

   trace_context_draw_block(tr_ctx, 2);
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
                               uint64_t *presult)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;
   uint64_t result;
   boolean _result;

   trace_dump_call_begin("pipe_context", "get_query_result");

   trace_dump_arg(ptr, pipe);

   _result = pipe->get_query_result(pipe, query, wait, presult);
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

   result = trace_shader_create(tr_ctx, state, result, TRACE_SHADER_FRAGMENT);

   return result;
}


static INLINE void
trace_context_bind_fs_state(struct pipe_context *_pipe,
                            void *_state)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct trace_shader *tr_shdr = trace_shader(_state);
   struct pipe_context *pipe = tr_ctx->pipe;
   void *state = tr_shdr ? tr_shdr->state : NULL;

   trace_dump_call_begin("pipe_context", "bind_fs_state");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, state);

   tr_ctx->curr.fs = tr_shdr;

   if (tr_shdr && tr_shdr->replaced)
      state = tr_shdr->replaced;

   pipe->bind_fs_state(pipe, state);

   trace_dump_call_end();
}


static INLINE void
trace_context_delete_fs_state(struct pipe_context *_pipe,
                              void *_state)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct trace_shader *tr_shdr = trace_shader(_state);
   struct pipe_context *pipe = tr_ctx->pipe;
   void *state = tr_shdr->state;

   trace_dump_call_begin("pipe_context", "delete_fs_state");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, state);

   pipe->delete_fs_state(pipe, state);

   trace_dump_call_end();

   trace_shader_destroy(tr_ctx, tr_shdr);
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

   result = trace_shader_create(tr_ctx, state, result, TRACE_SHADER_VERTEX);

   return result;
}


static INLINE void
trace_context_bind_vs_state(struct pipe_context *_pipe,
                            void *_state)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct trace_shader *tr_shdr = trace_shader(_state);
   struct pipe_context *pipe = tr_ctx->pipe;
   void *state = tr_shdr ? tr_shdr->state : NULL;

   trace_dump_call_begin("pipe_context", "bind_vs_state");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, state);

   tr_ctx->curr.vs = tr_shdr;

   if (tr_shdr && tr_shdr->replaced)
      state = tr_shdr->replaced;

   pipe->bind_vs_state(pipe, state);

   trace_dump_call_end();
}


static INLINE void
trace_context_delete_vs_state(struct pipe_context *_pipe,
                              void *_state)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct trace_shader *tr_shdr = trace_shader(_state);
   struct pipe_context *pipe = tr_ctx->pipe;
   void *state = tr_shdr->state;

   trace_dump_call_begin("pipe_context", "delete_vs_state");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, state);

   pipe->delete_vs_state(pipe, state);

   trace_dump_call_end();

   trace_shader_destroy(tr_ctx, tr_shdr);
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
trace_context_set_constant_buffer(struct pipe_context *_pipe,
                                  uint shader, uint index,
                                  struct pipe_buffer *buffer)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   if (buffer) {
      trace_screen_user_buffer_update(_pipe->screen, buffer);
      buffer = trace_buffer_unwrap(tr_ctx, buffer);
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

   {
      tr_ctx->curr.nr_cbufs = state->nr_cbufs;
      for (i = 0; i < state->nr_cbufs; i++)
         if (state->cbufs[i])
            tr_ctx->curr.cbufs[i] = trace_texture(state->cbufs[i]->texture);
         else
            tr_ctx->curr.cbufs[i] = NULL;
      if (state->zsbuf)
         tr_ctx->curr.zsbuf = trace_texture(state->zsbuf->texture);
      else
         tr_ctx->curr.zsbuf = NULL;
   }

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


static INLINE void
trace_context_set_fragment_sampler_textures(struct pipe_context *_pipe,
                                            unsigned num_textures,
                                            struct pipe_texture **textures)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct trace_texture *tr_tex;
   struct pipe_context *pipe = tr_ctx->pipe;
   struct pipe_texture *unwrapped_textures[PIPE_MAX_SAMPLERS];
   unsigned i;

   tr_ctx->curr.num_texs = num_textures;
   for(i = 0; i < num_textures; ++i) {
      tr_tex = trace_texture(textures[i]);
      tr_ctx->curr.tex[i] = tr_tex;
      unwrapped_textures[i] = tr_tex ? tr_tex->texture : NULL;
   }
   textures = unwrapped_textures;

   trace_dump_call_begin("pipe_context", "set_fragment_sampler_textures");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(uint, num_textures);
   trace_dump_arg_array(ptr, textures, num_textures);

   pipe->set_fragment_sampler_textures(pipe, num_textures, textures);

   trace_dump_call_end();
}


static INLINE void
trace_context_set_vertex_sampler_textures(struct pipe_context *_pipe,
                                          unsigned num_textures,
                                          struct pipe_texture **textures)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct trace_texture *tr_tex;
   struct pipe_context *pipe = tr_ctx->pipe;
   struct pipe_texture *unwrapped_textures[PIPE_MAX_VERTEX_SAMPLERS];
   unsigned i;

   tr_ctx->curr.num_vert_texs = num_textures;
   for(i = 0; i < num_textures; ++i) {
      tr_tex = trace_texture(textures[i]);
      tr_ctx->curr.vert_tex[i] = tr_tex;
      unwrapped_textures[i] = tr_tex ? tr_tex->texture : NULL;
   }
   textures = unwrapped_textures;

   trace_dump_call_begin("pipe_context", "set_vertex_sampler_textures");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(uint, num_textures);
   trace_dump_arg_array(ptr, textures, num_textures);

   pipe->set_vertex_sampler_textures(pipe, num_textures, textures);

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
      trace_screen_user_buffer_update(_pipe->screen, buffers[i].buffer);

   trace_dump_call_begin("pipe_context", "set_vertex_buffers");

   trace_dump_arg(ptr, pipe);
   trace_dump_arg(uint, num_buffers);

   trace_dump_arg_begin("buffers");
   trace_dump_struct_array(vertex_buffer, buffers, num_buffers);
   trace_dump_arg_end();

   if (num_buffers) {
      struct pipe_vertex_buffer *_buffers = malloc(num_buffers * sizeof(*_buffers));
      memcpy(_buffers, buffers, num_buffers * sizeof(*_buffers));
      for (i = 0; i < num_buffers; i++)
         _buffers[i].buffer = trace_buffer_unwrap(tr_ctx, buffers[i].buffer);
      pipe->set_vertex_buffers(pipe, num_buffers, _buffers);
      free(_buffers);
   } else {
      pipe->set_vertex_buffers(pipe, num_buffers, NULL);
   }

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

   pipe->set_vertex_elements(pipe, num_elements, elements);

   trace_dump_call_end();
}


static INLINE void
trace_context_surface_copy(struct pipe_context *_pipe,
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
   trace_dump_arg(ptr, dest);
   trace_dump_arg(uint, destx);
   trace_dump_arg(uint, desty);
   trace_dump_arg(ptr, src);
   trace_dump_arg(uint, srcx);
   trace_dump_arg(uint, srcy);
   trace_dump_arg(uint, width);
   trace_dump_arg(uint, height);

   pipe->surface_copy(pipe,
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

   pipe->surface_fill(pipe, dst, dstx, dsty, width, height, value);

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
   trace_dump_arg_array(float, rgba, 4);
   trace_dump_arg(float, depth);
   trace_dump_arg(uint, stencil);

   pipe->clear(pipe, buffers, rgba, depth, stencil);

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

   pipe->flush(pipe, flags, fence);

   if(fence)
      trace_dump_ret(ptr, *fence);

   trace_dump_call_end();
}


static INLINE void
trace_context_destroy(struct pipe_context *_pipe)
{
   struct trace_screen *tr_scr = trace_screen(_pipe->screen);
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;

   trace_dump_call_begin("pipe_context", "destroy");
   trace_dump_arg(ptr, pipe);
   trace_dump_call_end();

   trace_screen_remove_from_list(tr_scr, contexts, tr_ctx);

   pipe->destroy(pipe);

   FREE(tr_ctx);
}

static unsigned int
trace_is_texture_referenced( struct pipe_context *_pipe,
			    struct pipe_texture *_texture,
			    unsigned face, unsigned level)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct trace_texture *tr_tex = trace_texture(_texture);
   struct pipe_context *pipe = tr_ctx->pipe;
   struct pipe_texture *texture = tr_tex->texture;
   unsigned int referenced;

   trace_dump_call_begin("pipe_context", "is_texture_referenced");
   trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, texture);
   trace_dump_arg(uint, face);
   trace_dump_arg(uint, level);

   referenced = pipe->is_texture_referenced(pipe, texture, face, level);

   trace_dump_ret(uint, referenced);
   trace_dump_call_end();

   return referenced;
}

static unsigned int
trace_is_buffer_referenced( struct pipe_context *_pipe,
			    struct pipe_buffer *_buf)
{
   struct trace_context *tr_ctx = trace_context(_pipe);
   struct trace_buffer *tr_buf = trace_buffer(_buf);
   struct pipe_context *pipe = tr_ctx->pipe;
   struct pipe_buffer *buf = tr_buf->buffer;
   unsigned int referenced;

   trace_dump_call_begin("pipe_context", "is_buffer_referenced");
   trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, buf);

   referenced = pipe->is_buffer_referenced(pipe, buf);

   trace_dump_ret(uint, referenced);
   trace_dump_call_end();

   return referenced;
}

static const struct debug_named_value rbug_blocker_flags[] = {
   {"before", 1},
   {"after", 2},
   {NULL, 0},
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
   tr_ctx->draw_blocker = debug_get_flags_option("RBUG_BLOCK",
                                                 rbug_blocker_flags,
                                                 0);
   pipe_mutex_init(tr_ctx->draw_mutex);
   pipe_condvar_init(tr_ctx->draw_cond);
   pipe_mutex_init(tr_ctx->list_mutex);
   make_empty_list(&tr_ctx->shaders);

   tr_ctx->base.destroy = trace_context_destroy;
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
   tr_ctx->base.set_blend_color = trace_context_set_blend_color;
   tr_ctx->base.set_stencil_ref = trace_context_set_stencil_ref;
   tr_ctx->base.set_clip_state = trace_context_set_clip_state;
   tr_ctx->base.set_constant_buffer = trace_context_set_constant_buffer;
   tr_ctx->base.set_framebuffer_state = trace_context_set_framebuffer_state;
   tr_ctx->base.set_polygon_stipple = trace_context_set_polygon_stipple;
   tr_ctx->base.set_scissor_state = trace_context_set_scissor_state;
   tr_ctx->base.set_viewport_state = trace_context_set_viewport_state;
   tr_ctx->base.set_fragment_sampler_textures = trace_context_set_fragment_sampler_textures;
   tr_ctx->base.set_vertex_sampler_textures = trace_context_set_vertex_sampler_textures;
   tr_ctx->base.set_vertex_buffers = trace_context_set_vertex_buffers;
   tr_ctx->base.set_vertex_elements = trace_context_set_vertex_elements;
   if (pipe->surface_copy)
      tr_ctx->base.surface_copy = trace_context_surface_copy;
   if (pipe->surface_fill)
      tr_ctx->base.surface_fill = trace_context_surface_fill;
   tr_ctx->base.clear = trace_context_clear;
   tr_ctx->base.flush = trace_context_flush;
   tr_ctx->base.is_texture_referenced = trace_is_texture_referenced;
   tr_ctx->base.is_buffer_referenced = trace_is_buffer_referenced;

   tr_ctx->pipe = pipe;

   trace_screen_add_to_list(tr_scr, contexts, tr_ctx);

   return &tr_ctx->base;

error1:
   return pipe;
}

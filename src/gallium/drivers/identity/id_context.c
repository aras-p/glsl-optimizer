/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


#include "pipe/p_context.h"
#include "util/u_memory.h"

#include "id_context.h"
#include "id_objects.h"


static void
identity_destroy(struct pipe_context *_pipe)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->destroy(pipe);

   free(id_pipe);
}

static void
identity_draw_arrays(struct pipe_context *_pipe,
                     unsigned prim,
                     unsigned start,
                     unsigned count)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->draw_arrays(pipe,
                     prim,
                     start,
                     count);
}

static void
identity_draw_elements(struct pipe_context *_pipe,
                       struct pipe_buffer *_indexBuffer,
                       unsigned indexSize,
                       unsigned prim,
                       unsigned start,
                       unsigned count)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct identity_buffer *id_buffer = identity_buffer(_indexBuffer);
   struct pipe_context *pipe = id_pipe->pipe;
   struct pipe_buffer *indexBuffer = id_buffer->buffer;

   pipe->draw_elements(pipe,
                       indexBuffer,
                       indexSize,
                       prim,
                       start,
                       count);
}

static void
identity_draw_range_elements(struct pipe_context *_pipe,
                             struct pipe_buffer *_indexBuffer,
                             unsigned indexSize,
                             unsigned minIndex,
                             unsigned maxIndex,
                             unsigned mode,
                             unsigned start,
                             unsigned count)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct identity_buffer *id_buffer = identity_buffer(_indexBuffer);
   struct pipe_context *pipe = id_pipe->pipe;
   struct pipe_buffer *indexBuffer = id_buffer->buffer;

   pipe->draw_range_elements(pipe,
                             indexBuffer,
                             indexSize,
                             minIndex,
                             maxIndex,
                             mode,
                             start,
                             count);
}

static struct pipe_query *
identity_create_query(struct pipe_context *_pipe,
                      unsigned query_type)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   return pipe->create_query(pipe,
                             query_type);
}

static void
identity_destroy_query(struct pipe_context *_pipe,
                       struct pipe_query *query)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->destroy_query(pipe,
                       query);
}

static void
identity_begin_query(struct pipe_context *_pipe,
                     struct pipe_query *query)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->begin_query(pipe,
                     query);
}

static void
identity_end_query(struct pipe_context *_pipe,
                   struct pipe_query *query)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->end_query(pipe,
                   query);
}

static boolean
identity_get_query_result(struct pipe_context *_pipe,
                          struct pipe_query *query,
                          boolean wait,
                          uint64_t *result)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   return pipe->get_query_result(pipe,
                                 query,
                                 wait,
                                 result);
}

static void *
identity_create_blend_state(struct pipe_context *_pipe,
                            const struct pipe_blend_state *blend)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   return pipe->create_blend_state(pipe,
                                   blend);
}

static void
identity_bind_blend_state(struct pipe_context *_pipe,
                          void *blend)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->bind_blend_state(pipe,
                              blend);
}

static void
identity_delete_blend_state(struct pipe_context *_pipe,
                            void *blend)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->delete_blend_state(pipe,
                            blend);
}

static void *
identity_create_sampler_state(struct pipe_context *_pipe,
                              const struct pipe_sampler_state *sampler)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   return pipe->create_sampler_state(pipe,
                                     sampler);
}

static void
identity_bind_fragment_sampler_states(struct pipe_context *_pipe,
                                      unsigned num_samplers,
                                      void **samplers)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->bind_fragment_sampler_states(pipe,
                                      num_samplers,
                                      samplers);
}

static void
identity_bind_vertex_sampler_states(struct pipe_context *_pipe,
                                    unsigned num_samplers,
                                    void **samplers)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->bind_vertex_sampler_states(pipe,
                                    num_samplers,
                                    samplers);
}

static void
identity_delete_sampler_state(struct pipe_context *_pipe,
                              void *sampler)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->delete_sampler_state(pipe,
                              sampler);
}

static void *
identity_create_rasterizer_state(struct pipe_context *_pipe,
                                 const struct pipe_rasterizer_state *rasterizer)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   return pipe->create_rasterizer_state(pipe,
                                        rasterizer);
}

static void
identity_bind_rasterizer_state(struct pipe_context *_pipe,
                               void *rasterizer)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->bind_rasterizer_state(pipe,
                               rasterizer);
}

static void
identity_delete_rasterizer_state(struct pipe_context *_pipe,
                                 void *rasterizer)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->delete_rasterizer_state(pipe,
                                 rasterizer);
}

static void *
identity_create_depth_stencil_alpha_state(struct pipe_context *_pipe,
                                          const struct pipe_depth_stencil_alpha_state *depth_stencil_alpha)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   return pipe->create_depth_stencil_alpha_state(pipe,
                                                 depth_stencil_alpha);
}

static void
identity_bind_depth_stencil_alpha_state(struct pipe_context *_pipe,
                                        void *depth_stencil_alpha)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->bind_depth_stencil_alpha_state(pipe,
                                        depth_stencil_alpha);
}

static void
identity_delete_depth_stencil_alpha_state(struct pipe_context *_pipe,
                                          void *depth_stencil_alpha)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->delete_depth_stencil_alpha_state(pipe,
                                          depth_stencil_alpha);
}

static void *
identity_create_fs_state(struct pipe_context *_pipe,
                         const struct pipe_shader_state *fs)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   return pipe->create_fs_state(pipe,
                                fs);
}

static void
identity_bind_fs_state(struct pipe_context *_pipe,
                       void *fs)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->bind_fs_state(pipe,
                       fs);
}

static void
identity_delete_fs_state(struct pipe_context *_pipe,
                         void *fs)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->delete_fs_state(pipe,
                         fs);
}

static void *
identity_create_vs_state(struct pipe_context *_pipe,
                         const struct pipe_shader_state *vs)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   return pipe->create_vs_state(pipe,
                                vs);
}

static void
identity_bind_vs_state(struct pipe_context *_pipe,
                       void *vs)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->bind_vs_state(pipe,
                       vs);
}

static void
identity_delete_vs_state(struct pipe_context *_pipe,
                         void *vs)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->delete_vs_state(pipe,
                         vs);
}

static void
identity_set_blend_color(struct pipe_context *_pipe,
                         const struct pipe_blend_color *blend_color)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->set_blend_color(pipe,
                         blend_color);
}

static void
identity_set_stencil_ref(struct pipe_context *_pipe,
                         const struct pipe_stencil_ref *stencil_ref)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->set_stencil_ref(pipe,
                         stencil_ref);
}

static void
identity_set_clip_state(struct pipe_context *_pipe,
                        const struct pipe_clip_state *clip)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->set_clip_state(pipe,
                        clip);
}

static void
identity_set_constant_buffer(struct pipe_context *_pipe,
                             uint shader,
                             uint index,
                             struct pipe_buffer *_buffer)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;
   struct pipe_buffer *unwrapped_buffer;
   struct pipe_buffer *buffer = NULL;

   /* XXX hmm? unwrap the input state */
   if (_buffer) {
      unwrapped_buffer = identity_buffer_unwrap(_buffer);
      buffer = unwrapped_buffer;
   }

   pipe->set_constant_buffer(pipe,
                             shader,
                             index,
                             buffer);
}

static void
identity_set_framebuffer_state(struct pipe_context *_pipe,
                               const struct pipe_framebuffer_state *_state)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;
   struct pipe_framebuffer_state unwrapped_state;
   struct pipe_framebuffer_state *state = NULL;
   unsigned i;

   /* unwrap the input state */
   if (_state) {
      memcpy(&unwrapped_state, _state, sizeof(unwrapped_state));
      for(i = 0; i < _state->nr_cbufs; i++)
         unwrapped_state.cbufs[i] = identity_surface_unwrap(_state->cbufs[i]);
      for (; i < PIPE_MAX_COLOR_BUFS; i++)
         unwrapped_state.cbufs[i] = NULL;
      unwrapped_state.zsbuf = identity_surface_unwrap(_state->zsbuf);
      state = &unwrapped_state;
   }

   pipe->set_framebuffer_state(pipe,
                               state);
}

static void
identity_set_polygon_stipple(struct pipe_context *_pipe,
                             const struct pipe_poly_stipple *poly_stipple)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->set_polygon_stipple(pipe,
                             poly_stipple);
}

static void
identity_set_scissor_state(struct pipe_context *_pipe,
                           const struct pipe_scissor_state *scissor)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->set_scissor_state(pipe,
                           scissor);
}

static void
identity_set_viewport_state(struct pipe_context *_pipe,
                            const struct pipe_viewport_state *viewport)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->set_viewport_state(pipe,
                            viewport);
}

static void
identity_set_fragment_sampler_textures(struct pipe_context *_pipe,
                                       unsigned num_textures,
                                       struct pipe_texture **_textures)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;
   struct pipe_texture *unwrapped_textures[PIPE_MAX_SAMPLERS];
   struct pipe_texture **textures = NULL;
   unsigned i;

   if (_textures) {
      for (i = 0; i < num_textures; i++)
         unwrapped_textures[i] = identity_texture_unwrap(_textures[i]);
      for (; i < PIPE_MAX_SAMPLERS; i++)
         unwrapped_textures[i] = NULL;

      textures = unwrapped_textures;
   }

   pipe->set_fragment_sampler_textures(pipe,
                                       num_textures,
                                       textures);
}

static void
identity_set_vertex_sampler_textures(struct pipe_context *_pipe,
                                     unsigned num_textures,
                                     struct pipe_texture **_textures)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;
   struct pipe_texture *unwrapped_textures[PIPE_MAX_VERTEX_SAMPLERS];
   struct pipe_texture **textures = NULL;
   unsigned i;

   if (_textures) {
      for (i = 0; i < num_textures; i++)
         unwrapped_textures[i] = identity_texture_unwrap(_textures[i]);
      for (; i < PIPE_MAX_VERTEX_SAMPLERS; i++)
         unwrapped_textures[i] = NULL;

      textures = unwrapped_textures;
   }

   pipe->set_vertex_sampler_textures(pipe,
                                     num_textures,
                                     textures);
}

static void
identity_set_vertex_buffers(struct pipe_context *_pipe,
                            unsigned num_buffers,
                            const struct pipe_vertex_buffer *_buffers)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;
   struct pipe_vertex_buffer unwrapped_buffers[PIPE_MAX_SHADER_INPUTS];
   struct pipe_vertex_buffer *buffers = NULL;
   unsigned i;

   if (num_buffers) {
      memcpy(unwrapped_buffers, _buffers, num_buffers * sizeof(*_buffers));
      for (i = 0; i < num_buffers; i++)
         unwrapped_buffers[i].buffer = identity_buffer_unwrap(_buffers[i].buffer);
      buffers = unwrapped_buffers;
   }

   pipe->set_vertex_buffers(pipe,
                            num_buffers,
                            buffers);
}

static void
identity_set_vertex_elements(struct pipe_context *_pipe,
                             unsigned num_elements,
                             const struct pipe_vertex_element *vertex_elements)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->set_vertex_elements(pipe,
                             num_elements,
                             vertex_elements);
}

static void
identity_surface_copy(struct pipe_context *_pipe,
                      struct pipe_surface *_dst,
                      unsigned dstx,
                      unsigned dsty,
                      struct pipe_surface *_src,
                      unsigned srcx,
                      unsigned srcy,
                      unsigned width,
                      unsigned height)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct identity_surface *id_surface_dst = identity_surface(_dst);
   struct identity_surface *id_surface_src = identity_surface(_src);
   struct pipe_context *pipe = id_pipe->pipe;
   struct pipe_surface *dst = id_surface_dst->surface;
   struct pipe_surface *src = id_surface_src->surface;

   pipe->surface_copy(pipe,
                      dst,
                      dstx,
                      dsty,
                      src,
                      srcx,
                      srcy,
                      width,
                      height);
}

static void
identity_surface_fill(struct pipe_context *_pipe,
                      struct pipe_surface *_dst,
                      unsigned dstx,
                      unsigned dsty,
                      unsigned width,
                      unsigned height,
                      unsigned value)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct identity_surface *id_surface_dst = identity_surface(_dst);
   struct pipe_context *pipe = id_pipe->pipe;
   struct pipe_surface *dst = id_surface_dst->surface;

   pipe->surface_fill(pipe,
                      dst,
                      dstx,
                      dsty,
                      width,
                      height,
                      value);
}

static void
identity_clear(struct pipe_context *_pipe,
               unsigned buffers,
               const float *rgba,
               double depth,
               unsigned stencil)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->clear(pipe,
               buffers,
               rgba,
               depth,
               stencil);
}

static void
identity_flush(struct pipe_context *_pipe,
               unsigned flags,
               struct pipe_fence_handle **fence)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct pipe_context *pipe = id_pipe->pipe;

   pipe->flush(pipe,
               flags,
               fence);
}

static unsigned int
identity_is_texture_referenced(struct pipe_context *_pipe,
                               struct pipe_texture *_texture,
                               unsigned face,
                               unsigned level)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct identity_texture *id_texture = identity_texture(_texture);
   struct pipe_context *pipe = id_pipe->pipe;
   struct pipe_texture *texture = id_texture->texture;

   return pipe->is_texture_referenced(pipe,
                                      texture,
                                      face,
                                      level);
}

static unsigned int
identity_is_buffer_referenced(struct pipe_context *_pipe,
                              struct pipe_buffer *_buffer)
{
   struct identity_context *id_pipe = identity_context(_pipe);
   struct identity_buffer *id_buffer = identity_buffer(_buffer);
   struct pipe_context *pipe = id_pipe->pipe;
   struct pipe_buffer *buffer = id_buffer->buffer;

   return pipe->is_buffer_referenced(pipe,
                                     buffer);
}

struct pipe_context *
identity_context_create(struct pipe_screen *_screen, struct pipe_context *pipe)
{
   struct identity_context *id_pipe;
   (void)identity_screen(_screen);

   id_pipe = CALLOC_STRUCT(identity_context);
   if (!id_pipe) {
      return NULL;
   }

   id_pipe->base.winsys = NULL;
   id_pipe->base.screen = _screen;
   id_pipe->base.priv = pipe->priv; /* expose wrapped data */
   id_pipe->base.draw = NULL;

   id_pipe->base.destroy = identity_destroy;
   id_pipe->base.draw_arrays = identity_draw_arrays;
   id_pipe->base.draw_elements = identity_draw_elements;
   id_pipe->base.draw_range_elements = identity_draw_range_elements;
   id_pipe->base.create_query = identity_create_query;
   id_pipe->base.destroy_query = identity_destroy_query;
   id_pipe->base.begin_query = identity_begin_query;
   id_pipe->base.end_query = identity_end_query;
   id_pipe->base.get_query_result = identity_get_query_result;
   id_pipe->base.create_blend_state = identity_create_blend_state;
   id_pipe->base.bind_blend_state = identity_bind_blend_state;
   id_pipe->base.delete_blend_state = identity_delete_blend_state;
   id_pipe->base.create_sampler_state = identity_create_sampler_state;
   id_pipe->base.bind_fragment_sampler_states = identity_bind_fragment_sampler_states;
   id_pipe->base.bind_vertex_sampler_states = identity_bind_vertex_sampler_states;
   id_pipe->base.delete_sampler_state = identity_delete_sampler_state;
   id_pipe->base.create_rasterizer_state = identity_create_rasterizer_state;
   id_pipe->base.bind_rasterizer_state = identity_bind_rasterizer_state;
   id_pipe->base.delete_rasterizer_state = identity_delete_rasterizer_state;
   id_pipe->base.create_depth_stencil_alpha_state = identity_create_depth_stencil_alpha_state;
   id_pipe->base.bind_depth_stencil_alpha_state = identity_bind_depth_stencil_alpha_state;
   id_pipe->base.delete_depth_stencil_alpha_state = identity_delete_depth_stencil_alpha_state;
   id_pipe->base.create_fs_state = identity_create_fs_state;
   id_pipe->base.bind_fs_state = identity_bind_fs_state;
   id_pipe->base.delete_fs_state = identity_delete_fs_state;
   id_pipe->base.create_vs_state = identity_create_vs_state;
   id_pipe->base.bind_vs_state = identity_bind_vs_state;
   id_pipe->base.delete_vs_state = identity_delete_vs_state;
   id_pipe->base.set_blend_color = identity_set_blend_color;
   id_pipe->base.set_stencil_ref = identity_set_stencil_ref;
   id_pipe->base.set_clip_state = identity_set_clip_state;
   id_pipe->base.set_constant_buffer = identity_set_constant_buffer;
   id_pipe->base.set_framebuffer_state = identity_set_framebuffer_state;
   id_pipe->base.set_polygon_stipple = identity_set_polygon_stipple;
   id_pipe->base.set_scissor_state = identity_set_scissor_state;
   id_pipe->base.set_viewport_state = identity_set_viewport_state;
   id_pipe->base.set_fragment_sampler_textures = identity_set_fragment_sampler_textures;
   id_pipe->base.set_vertex_sampler_textures = identity_set_vertex_sampler_textures;
   id_pipe->base.set_vertex_buffers = identity_set_vertex_buffers;
   id_pipe->base.set_vertex_elements = identity_set_vertex_elements;
   id_pipe->base.surface_copy = identity_surface_copy;
   id_pipe->base.surface_fill = identity_surface_fill;
   id_pipe->base.clear = identity_clear;
   id_pipe->base.flush = identity_flush;
   id_pipe->base.is_texture_referenced = identity_is_texture_referenced;
   id_pipe->base.is_buffer_referenced = identity_is_buffer_referenced;

   id_pipe->pipe = pipe;

   return &id_pipe->base;
}

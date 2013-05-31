/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2012-2013 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#include "util/u_framebuffer.h"
#include "util/u_helpers.h"

#include "ilo_context.h"
#include "ilo_shader.h"
#include "ilo_state.h"

/*
 * We simply remember the pipe states here and derive HW commands/states from
 * them later.  We could do better by deriving (some of the) HW
 * commands/states directly.
 */

static void
finalize_shader_states(struct ilo_context *ilo)
{
   /* this table is ugly and is a burden to maintain.. */
   const struct {
      struct ilo_shader_state *state;
      struct ilo_shader *prev_shader;
      uint32_t prev_cache_seqno;
      uint32_t dirty;
      uint32_t deps;
   } sh[PIPE_SHADER_TYPES] = {
      [PIPE_SHADER_VERTEX] = {
         .state = ilo->vs,
         .prev_shader = (ilo->vs) ? ilo->vs->shader : NULL,
         .prev_cache_seqno = (ilo->vs) ? ilo->vs->shader->cache_seqno : 0,
         .dirty = ILO_DIRTY_VS,
         .deps = ILO_DIRTY_VERTEX_SAMPLER_VIEWS |
                 ILO_DIRTY_RASTERIZER,
      },
      [PIPE_SHADER_FRAGMENT] = {
         .state = ilo->fs,
         .prev_shader = (ilo->fs) ? ilo->fs->shader : NULL,
         .prev_cache_seqno = (ilo->fs) ? ilo->fs->shader->cache_seqno : 0,
         .dirty = ILO_DIRTY_FS,
         .deps = ILO_DIRTY_FRAGMENT_SAMPLER_VIEWS |
                 ILO_DIRTY_RASTERIZER |
                 ILO_DIRTY_FRAMEBUFFER,
      },
      [PIPE_SHADER_GEOMETRY] = {
         .state = ilo->gs,
         .prev_shader = (ilo->gs) ? ilo->gs->shader : NULL,
         .prev_cache_seqno = (ilo->gs) ? ilo->gs->shader->cache_seqno : 0,
         .dirty = ILO_DIRTY_GS,
         .deps = ILO_DIRTY_GEOMETRY_SAMPLER_VIEWS |
                 ILO_DIRTY_VS |
                 ILO_DIRTY_RASTERIZER,
      },
      [PIPE_SHADER_COMPUTE] = {
         .state = NULL,
         .prev_shader = NULL,
         .prev_cache_seqno = 0,
         .dirty = 0,
         .deps = 0,
      },
   };
   struct ilo_shader *shaders[PIPE_SHADER_TYPES];
   int num_shaders = 0, i;

   for (i = 0; i < PIPE_SHADER_TYPES; i++) {
      /* no state bound */
      if (!sh[i].state)
         continue;

      /* switch variant if the shader or the states it depends on changed */
      if (ilo->dirty & (sh[i].dirty | sh[i].deps)) {
         struct ilo_shader_variant variant;

         ilo_shader_variant_init(&variant, &sh[i].state->info, ilo);
         ilo_shader_state_use_variant(sh[i].state, &variant);
      }

      shaders[num_shaders++] = sh[i].state->shader;
   }

   ilo_shader_cache_set(ilo->shader_cache, shaders, num_shaders);

   for (i = 0; i < PIPE_SHADER_TYPES; i++) {
      /* no state bound */
      if (!sh[i].state)
         continue;

      /*
       * mark the shader state dirty if
       *
       *  - a new variant is selected, or
       *  - the kernel is uploaded to a different bo
       */
      if (sh[i].state->shader != sh[i].prev_shader ||
          sh[i].state->shader->cache_seqno != sh[i].prev_cache_seqno)
         ilo->dirty |= sh[i].dirty;
   }
}

static void
finalize_constant_buffers(struct ilo_context *ilo)
{
   int sh;

   if (!(ilo->dirty & ILO_DIRTY_CONSTANT_BUFFER))
      return;

   for (sh = 0; sh < PIPE_SHADER_TYPES; sh++) {
      int last_cbuf = Elements(ilo->cbuf[sh].states) - 1;

      /* find the last cbuf */
      while (last_cbuf >= 0 &&
             !ilo->cbuf[sh].states[last_cbuf].buffer)
         last_cbuf--;

      ilo->cbuf[sh].count = last_cbuf + 1;
   }
}

/**
 * Finalize states.  Some states depend on other states and are
 * incomplete/invalid until finalized.
 */
void
ilo_finalize_states(struct ilo_context *ilo)
{
   finalize_shader_states(ilo);
   finalize_constant_buffers(ilo);
}

static void *
ilo_create_blend_state(struct pipe_context *pipe,
                       const struct pipe_blend_state *state)
{
   struct ilo_context *ilo = ilo_context(pipe);
   struct ilo_blend_state *blend;

   blend = MALLOC_STRUCT(ilo_blend_state);
   assert(blend);

   ilo_gpe_init_blend(ilo->dev, state, blend);

   return blend;
}

static void
ilo_bind_blend_state(struct pipe_context *pipe, void *state)
{
   struct ilo_context *ilo = ilo_context(pipe);

   ilo->blend = state;

   ilo->dirty |= ILO_DIRTY_BLEND;
}

static void
ilo_delete_blend_state(struct pipe_context *pipe, void  *state)
{
   FREE(state);
}

static void *
ilo_create_sampler_state(struct pipe_context *pipe,
                         const struct pipe_sampler_state *state)
{
   struct ilo_context *ilo = ilo_context(pipe);
   struct ilo_sampler_cso *sampler;

   sampler = MALLOC_STRUCT(ilo_sampler_cso);
   assert(sampler);

   ilo_gpe_init_sampler_cso(ilo->dev, state, sampler);

   return sampler;
}

static void
bind_samplers(struct ilo_context *ilo,
              unsigned shader, unsigned start, unsigned count,
              void **samplers, bool unbind_old)
{
   const struct ilo_sampler_cso **dst = ilo->sampler[shader].cso;
   unsigned i;

   assert(start + count <= Elements(ilo->sampler[shader].cso));

   if (unbind_old) {
      if (!samplers) {
         start = 0;
         count = 0;
      }

      for (i = 0; i < start; i++)
         dst[i] = NULL;
      for (; i < start + count; i++)
         dst[i] = samplers[i - start];
      for (; i < ilo->sampler[shader].count; i++)
         dst[i] = NULL;

      ilo->sampler[shader].count = start + count;

      return;
   }

   dst += start;
   if (samplers) {
      for (i = 0; i < count; i++)
         dst[i] = samplers[i];
   }
   else {
      for (i = 0; i < count; i++)
         dst[i] = NULL;
   }

   if (ilo->sampler[shader].count <= start + count) {
      count += start;

      while (count > 0 && !ilo->sampler[shader].cso[count - 1])
         count--;

      ilo->sampler[shader].count = count;
   }
}

static void
ilo_bind_fragment_sampler_states(struct pipe_context *pipe,
                                 unsigned num_samplers,
                                 void **samplers)
{
   struct ilo_context *ilo = ilo_context(pipe);

   bind_samplers(ilo, PIPE_SHADER_FRAGMENT, 0, num_samplers, samplers, true);
   ilo->dirty |= ILO_DIRTY_FRAGMENT_SAMPLERS;
}

static void
ilo_bind_vertex_sampler_states(struct pipe_context *pipe,
                               unsigned num_samplers,
                               void **samplers)
{
   struct ilo_context *ilo = ilo_context(pipe);

   bind_samplers(ilo, PIPE_SHADER_VERTEX, 0, num_samplers, samplers, true);
   ilo->dirty |= ILO_DIRTY_VERTEX_SAMPLERS;
}

static void
ilo_bind_geometry_sampler_states(struct pipe_context *pipe,
                                 unsigned num_samplers,
                                 void **samplers)
{
   struct ilo_context *ilo = ilo_context(pipe);

   bind_samplers(ilo, PIPE_SHADER_GEOMETRY, 0, num_samplers, samplers, true);
   ilo->dirty |= ILO_DIRTY_GEOMETRY_SAMPLERS;
}

static void
ilo_bind_compute_sampler_states(struct pipe_context *pipe,
                                unsigned start_slot,
                                unsigned num_samplers,
                                void **samplers)
{
   struct ilo_context *ilo = ilo_context(pipe);

   bind_samplers(ilo, PIPE_SHADER_COMPUTE,
         start_slot, num_samplers, samplers, false);
   ilo->dirty |= ILO_DIRTY_COMPUTE_SAMPLERS;
}

static void
ilo_delete_sampler_state(struct pipe_context *pipe, void *state)
{
   FREE(state);
}

static void *
ilo_create_rasterizer_state(struct pipe_context *pipe,
                            const struct pipe_rasterizer_state *state)
{
   struct ilo_rasterizer_state *rast;

   rast = MALLOC_STRUCT(ilo_rasterizer_state);
   assert(rast);

   rast->state = *state;

   return rast;
}

static void
ilo_bind_rasterizer_state(struct pipe_context *pipe, void *state)
{
   struct ilo_context *ilo = ilo_context(pipe);

   ilo->rasterizer = state;

   ilo->dirty |= ILO_DIRTY_RASTERIZER;
}

static void
ilo_delete_rasterizer_state(struct pipe_context *pipe, void *state)
{
   FREE(state);
}

static void *
ilo_create_depth_stencil_alpha_state(struct pipe_context *pipe,
                                     const struct pipe_depth_stencil_alpha_state *state)
{
   struct ilo_dsa_state *dsa;

   dsa = MALLOC_STRUCT(ilo_dsa_state);
   assert(dsa);

   dsa->state = *state;

   return dsa;
}

static void
ilo_bind_depth_stencil_alpha_state(struct pipe_context *pipe, void *state)
{
   struct ilo_context *ilo = ilo_context(pipe);

   ilo->dsa = state;

   ilo->dirty |= ILO_DIRTY_DEPTH_STENCIL_ALPHA;
}

static void
ilo_delete_depth_stencil_alpha_state(struct pipe_context *pipe, void *state)
{
   FREE(state);
}

static void *
ilo_create_fs_state(struct pipe_context *pipe,
                    const struct pipe_shader_state *state)
{
   struct ilo_context *ilo = ilo_context(pipe);
   return ilo_shader_state_create(ilo, PIPE_SHADER_FRAGMENT, state);
}

static void
ilo_bind_fs_state(struct pipe_context *pipe, void *state)
{
   struct ilo_context *ilo = ilo_context(pipe);

   ilo->fs = state;

   ilo->dirty |= ILO_DIRTY_FS;
}

static void
ilo_delete_fs_state(struct pipe_context *pipe, void *state)
{
   struct ilo_shader_state *fs = (struct ilo_shader_state *) state;
   ilo_shader_state_destroy(fs);
}

static void *
ilo_create_vs_state(struct pipe_context *pipe,
                    const struct pipe_shader_state *state)
{
   struct ilo_context *ilo = ilo_context(pipe);
   return ilo_shader_state_create(ilo, PIPE_SHADER_VERTEX, state);
}

static void
ilo_bind_vs_state(struct pipe_context *pipe, void *state)
{
   struct ilo_context *ilo = ilo_context(pipe);

   ilo->vs = state;

   ilo->dirty |= ILO_DIRTY_VS;
}

static void
ilo_delete_vs_state(struct pipe_context *pipe, void *state)
{
   struct ilo_shader_state *vs = (struct ilo_shader_state *) state;
   ilo_shader_state_destroy(vs);
}

static void *
ilo_create_gs_state(struct pipe_context *pipe,
                    const struct pipe_shader_state *state)
{
   struct ilo_context *ilo = ilo_context(pipe);
   return ilo_shader_state_create(ilo, PIPE_SHADER_GEOMETRY, state);
}

static void
ilo_bind_gs_state(struct pipe_context *pipe, void *state)
{
   struct ilo_context *ilo = ilo_context(pipe);

   ilo->gs = state;

   ilo->dirty |= ILO_DIRTY_GS;
}

static void
ilo_delete_gs_state(struct pipe_context *pipe, void *state)
{
   struct ilo_shader_state *gs = (struct ilo_shader_state *) state;
   ilo_shader_state_destroy(gs);
}

static void *
ilo_create_vertex_elements_state(struct pipe_context *pipe,
                                 unsigned num_elements,
                                 const struct pipe_vertex_element *elements)
{
   struct ilo_ve_state *ve;

   ve = MALLOC_STRUCT(ilo_ve_state);
   assert(ve);

   memcpy(ve->states, elements, sizeof(*elements) * num_elements);
   ve->count = num_elements;

   return ve;
}

static void
ilo_bind_vertex_elements_state(struct pipe_context *pipe, void *state)
{
   struct ilo_context *ilo = ilo_context(pipe);

   ilo->ve = state;

   ilo->dirty |= ILO_DIRTY_VERTEX_ELEMENTS;
}

static void
ilo_delete_vertex_elements_state(struct pipe_context *pipe, void *state)
{
   struct ilo_ve_state *ve = state;

   FREE(ve);
}

static void
ilo_set_blend_color(struct pipe_context *pipe,
                    const struct pipe_blend_color *state)
{
   struct ilo_context *ilo = ilo_context(pipe);

   ilo->blend_color = *state;

   ilo->dirty |= ILO_DIRTY_BLEND_COLOR;
}

static void
ilo_set_stencil_ref(struct pipe_context *pipe,
                    const struct pipe_stencil_ref *state)
{
   struct ilo_context *ilo = ilo_context(pipe);

   ilo->stencil_ref = *state;

   ilo->dirty |= ILO_DIRTY_STENCIL_REF;
}

static void
ilo_set_sample_mask(struct pipe_context *pipe,
                    unsigned sample_mask)
{
   struct ilo_context *ilo = ilo_context(pipe);

   ilo->sample_mask = sample_mask;

   ilo->dirty |= ILO_DIRTY_SAMPLE_MASK;
}

static void
ilo_set_clip_state(struct pipe_context *pipe,
                   const struct pipe_clip_state *state)
{
   struct ilo_context *ilo = ilo_context(pipe);

   ilo->clip = *state;

   ilo->dirty |= ILO_DIRTY_CLIP;
}

static void
ilo_set_constant_buffer(struct pipe_context *pipe,
                        uint shader, uint index,
                        struct pipe_constant_buffer *buf)
{
   struct ilo_context *ilo = ilo_context(pipe);
   struct pipe_constant_buffer *cbuf;

   assert(shader < Elements(ilo->cbuf));
   assert(index < Elements(ilo->cbuf[shader].states));

   cbuf = &ilo->cbuf[shader].states[index];

   pipe_resource_reference(&cbuf->buffer, NULL);

   if (buf) {
      pipe_resource_reference(&cbuf->buffer, buf->buffer);
      cbuf->buffer_offset = buf->buffer_offset;
      cbuf->buffer_size = buf->buffer_size;
      cbuf->user_buffer = buf->user_buffer;
   }
   else {
      cbuf->buffer_offset = 0;
      cbuf->buffer_size = 0;
      cbuf->user_buffer = 0;
   }

   /* the correct value will be set in ilo_finalize_states() */
   ilo->cbuf[shader].count = 0;

   ilo->dirty |= ILO_DIRTY_CONSTANT_BUFFER;
}

static void
ilo_set_framebuffer_state(struct pipe_context *pipe,
                          const struct pipe_framebuffer_state *state)
{
   struct ilo_context *ilo = ilo_context(pipe);

   util_copy_framebuffer_state(&ilo->fb.state, state);

   if (state->nr_cbufs)
      ilo->fb.num_samples = state->cbufs[0]->texture->nr_samples;
   else if (state->zsbuf)
      ilo->fb.num_samples = state->zsbuf->texture->nr_samples;
   else
      ilo->fb.num_samples = 1;

   if (!ilo->fb.num_samples)
      ilo->fb.num_samples = 1;

   ilo->dirty |= ILO_DIRTY_FRAMEBUFFER;
}

static void
ilo_set_polygon_stipple(struct pipe_context *pipe,
                        const struct pipe_poly_stipple *state)
{
   struct ilo_context *ilo = ilo_context(pipe);

   ilo->poly_stipple = *state;

   ilo->dirty |= ILO_DIRTY_POLY_STIPPLE;
}

static void
ilo_set_scissor_states(struct pipe_context *pipe,
                       unsigned start_slot,
                       unsigned num_scissors,
                       const struct pipe_scissor_state *scissors)
{
   struct ilo_context *ilo = ilo_context(pipe);

   ilo_gpe_set_scissor(ilo->dev, start_slot, num_scissors,
         scissors, &ilo->scissor);

   ilo->dirty |= ILO_DIRTY_SCISSOR;
}

static void
ilo_set_viewport_states(struct pipe_context *pipe,
                        unsigned start_slot,
                        unsigned num_viewports,
                        const struct pipe_viewport_state *viewports)
{
   struct ilo_context *ilo = ilo_context(pipe);

   if (viewports) {
      unsigned i;

      for (i = 0; i < num_viewports; i++) {
         ilo_gpe_set_viewport_cso(ilo->dev, &viewports[i],
               &ilo->viewport.cso[start_slot + i]);
      }

      if (ilo->viewport.count < start_slot + num_viewports)
         ilo->viewport.count = start_slot + num_viewports;

      /* need to save viewport 0 for util_blitter */
      if (!start_slot && num_viewports)
         ilo->viewport.viewport0 = viewports[0];
   }
   else {
      if (ilo->viewport.count <= start_slot + num_viewports &&
          ilo->viewport.count > start_slot)
         ilo->viewport.count = start_slot;
   }

   ilo->dirty |= ILO_DIRTY_VIEWPORT;
}

static void
set_sampler_views(struct ilo_context *ilo,
                  unsigned shader, unsigned start, unsigned count,
                  struct pipe_sampler_view **views, bool unset_old)
{
   struct pipe_sampler_view **dst = ilo->view[shader].states;
   unsigned i;

   assert(start + count <= Elements(ilo->view[shader].states));

   if (unset_old) {
      if (!views) {
         start = 0;
         count = 0;
      }

      for (i = 0; i < start; i++)
         pipe_sampler_view_reference(&dst[i], NULL);
      for (; i < start + count; i++)
         pipe_sampler_view_reference(&dst[i], views[i - start]);
      for (; i < ilo->view[shader].count; i++)
         pipe_sampler_view_reference(&dst[i], NULL);

      ilo->view[shader].count = start + count;

      return;
   }

   dst += start;
   if (views) {
      for (i = 0; i < count; i++)
         pipe_sampler_view_reference(&dst[i], views[i]);
   }
   else {
      for (i = 0; i < count; i++)
         pipe_sampler_view_reference(&dst[i], NULL);
   }

   if (ilo->view[shader].count <= start + count) {
      count += start;

      while (count > 0 && !ilo->view[shader].states[count - 1])
         count--;

      ilo->view[shader].count = count;
   }
}

static void
ilo_set_fragment_sampler_views(struct pipe_context *pipe,
                               unsigned num_views,
                               struct pipe_sampler_view **views)
{
   struct ilo_context *ilo = ilo_context(pipe);

   set_sampler_views(ilo, PIPE_SHADER_FRAGMENT, 0, num_views, views, true);
   ilo->dirty |= ILO_DIRTY_FRAGMENT_SAMPLER_VIEWS;
}

static void
ilo_set_vertex_sampler_views(struct pipe_context *pipe,
                             unsigned num_views,
                             struct pipe_sampler_view **views)
{
   struct ilo_context *ilo = ilo_context(pipe);

   set_sampler_views(ilo, PIPE_SHADER_VERTEX, 0, num_views, views, true);
   ilo->dirty |= ILO_DIRTY_VERTEX_SAMPLER_VIEWS;
}

static void
ilo_set_geometry_sampler_views(struct pipe_context *pipe,
                               unsigned num_views,
                               struct pipe_sampler_view **views)
{
   struct ilo_context *ilo = ilo_context(pipe);

   set_sampler_views(ilo, PIPE_SHADER_GEOMETRY, 0, num_views, views, true);
   ilo->dirty |= ILO_DIRTY_GEOMETRY_SAMPLER_VIEWS;
}

static void
ilo_set_compute_sampler_views(struct pipe_context *pipe,
                              unsigned start_slot, unsigned num_views,
                              struct pipe_sampler_view **views)
{
   struct ilo_context *ilo = ilo_context(pipe);

   set_sampler_views(ilo, PIPE_SHADER_COMPUTE,
         start_slot, num_views, views, false);

   ilo->dirty |= ILO_DIRTY_COMPUTE_SAMPLER_VIEWS;
}

static void
ilo_set_shader_resources(struct pipe_context *pipe,
                         unsigned start, unsigned count,
                         struct pipe_surface **surfaces)
{
   struct ilo_context *ilo = ilo_context(pipe);
   struct pipe_surface **dst = ilo->resource.states;
   unsigned i;

   assert(start + count <= Elements(ilo->resource.states));

   dst += start;
   if (surfaces) {
      for (i = 0; i < count; i++)
         pipe_surface_reference(&dst[i], surfaces[i]);
   }
   else {
      for (i = 0; i < count; i++)
         pipe_surface_reference(&dst[i], NULL);
   }

   if (ilo->resource.count <= start + count) {
      count += start;

      while (count > 0 && !ilo->resource.states[count - 1])
         count--;

      ilo->resource.count = count;
   }

   ilo->dirty |= ILO_DIRTY_SHADER_RESOURCES;
}

static void
ilo_set_vertex_buffers(struct pipe_context *pipe,
                       unsigned start_slot, unsigned num_buffers,
                       const struct pipe_vertex_buffer *buffers)
{
   struct ilo_context *ilo = ilo_context(pipe);

   util_set_vertex_buffers_mask(ilo->vb.states,
         &ilo->vb.enabled_mask, buffers, start_slot, num_buffers);

   ilo->dirty |= ILO_DIRTY_VERTEX_BUFFERS;
}

static void
ilo_set_index_buffer(struct pipe_context *pipe,
                     const struct pipe_index_buffer *state)
{
   struct ilo_context *ilo = ilo_context(pipe);

   if (state) {
      ilo->ib.state.index_size = state->index_size;
      ilo->ib.state.offset = state->offset;
      pipe_resource_reference(&ilo->ib.state.buffer, state->buffer);
      ilo->ib.state.user_buffer = state->user_buffer;
   }
   else {
      ilo->ib.state.index_size = 0;
      ilo->ib.state.offset = 0;
      pipe_resource_reference(&ilo->ib.state.buffer, NULL);
      ilo->ib.state.user_buffer = NULL;
   }

   ilo->dirty |= ILO_DIRTY_INDEX_BUFFER;
}

static struct pipe_stream_output_target *
ilo_create_stream_output_target(struct pipe_context *pipe,
                                struct pipe_resource *res,
                                unsigned buffer_offset,
                                unsigned buffer_size)
{
   struct pipe_stream_output_target *target;

   target = MALLOC_STRUCT(pipe_stream_output_target);
   assert(target);

   pipe_reference_init(&target->reference, 1);
   target->buffer = NULL;
   pipe_resource_reference(&target->buffer, res);
   target->context = pipe;
   target->buffer_offset = buffer_offset;
   target->buffer_size = buffer_size;

   return target;
}

static void
ilo_set_stream_output_targets(struct pipe_context *pipe,
                              unsigned num_targets,
                              struct pipe_stream_output_target **targets,
                              unsigned append_bitmask)
{
   struct ilo_context *ilo = ilo_context(pipe);
   unsigned i;

   if (!targets)
      num_targets = 0;

   for (i = 0; i < num_targets; i++)
      pipe_so_target_reference(&ilo->so.states[i], targets[i]);

   for (; i < ilo->so.count; i++)
      pipe_so_target_reference(&ilo->so.states[i], NULL);

   ilo->so.count = num_targets;
   ilo->so.append_bitmask = append_bitmask;

   ilo->so.enabled = (ilo->so.count > 0);

   ilo->dirty |= ILO_DIRTY_STREAM_OUTPUT_TARGETS;
}

static void
ilo_stream_output_target_destroy(struct pipe_context *pipe,
                                 struct pipe_stream_output_target *target)
{
   pipe_resource_reference(&target->buffer, NULL);
   FREE(target);
}

static struct pipe_sampler_view *
ilo_create_sampler_view(struct pipe_context *pipe,
                        struct pipe_resource *res,
                        const struct pipe_sampler_view *templ)
{
   struct pipe_sampler_view *view;

   view = MALLOC_STRUCT(pipe_sampler_view);
   assert(view);

   *view = *templ;
   pipe_reference_init(&view->reference, 1);
   view->texture = NULL;
   pipe_resource_reference(&view->texture, res);
   view->context = pipe;

   return view;
}

static void
ilo_sampler_view_destroy(struct pipe_context *pipe,
                         struct pipe_sampler_view *view)
{
   pipe_resource_reference(&view->texture, NULL);
   FREE(view);
}

static struct pipe_surface *
ilo_create_surface(struct pipe_context *pipe,
                   struct pipe_resource *res,
                   const struct pipe_surface *templ)
{
   struct pipe_surface *surface;

   surface = MALLOC_STRUCT(pipe_surface);
   assert(surface);

   *surface = *templ;
   pipe_reference_init(&surface->reference, 1);
   surface->texture = NULL;
   pipe_resource_reference(&surface->texture, res);

   surface->context = pipe;
   surface->width = u_minify(res->width0, surface->u.tex.level);
   surface->height = u_minify(res->height0, surface->u.tex.level);

   return surface;
}

static void
ilo_surface_destroy(struct pipe_context *pipe,
                    struct pipe_surface *surface)
{
   pipe_resource_reference(&surface->texture, NULL);
   FREE(surface);
}

static void *
ilo_create_compute_state(struct pipe_context *pipe,
                         const struct pipe_compute_state *state)
{
   struct ilo_context *ilo = ilo_context(pipe);
   return ilo_shader_state_create(ilo, PIPE_SHADER_COMPUTE, state);
}

static void
ilo_bind_compute_state(struct pipe_context *pipe, void *state)
{
   struct ilo_context *ilo = ilo_context(pipe);

   ilo->cs = state;

   ilo->dirty |= ILO_DIRTY_COMPUTE;
}

static void
ilo_delete_compute_state(struct pipe_context *pipe, void *state)
{
   struct ilo_shader_state *cs = (struct ilo_shader_state *) state;
   ilo_shader_state_destroy(cs);
}

static void
ilo_set_compute_resources(struct pipe_context *pipe,
                          unsigned start, unsigned count,
                          struct pipe_surface **surfaces)
{
   struct ilo_context *ilo = ilo_context(pipe);
   struct pipe_surface **dst = ilo->cs_resource.states;
   unsigned i;

   assert(start + count <= Elements(ilo->cs_resource.states));

   dst += start;
   if (surfaces) {
      for (i = 0; i < count; i++)
         pipe_surface_reference(&dst[i], surfaces[i]);
   }
   else {
      for (i = 0; i < count; i++)
         pipe_surface_reference(&dst[i], NULL);
   }

   if (ilo->cs_resource.count <= start + count) {
      count += start;

      while (count > 0 && !ilo->cs_resource.states[count - 1])
         count--;

      ilo->cs_resource.count = count;
   }

   ilo->dirty |= ILO_DIRTY_COMPUTE_RESOURCES;
}

static void
ilo_set_global_binding(struct pipe_context *pipe,
                       unsigned start, unsigned count,
                       struct pipe_resource **resources,
                       uint32_t **handles)
{
   struct ilo_context *ilo = ilo_context(pipe);
   struct pipe_resource **dst = ilo->global_binding.resources;
   unsigned i;

   assert(start + count <= Elements(ilo->global_binding.resources));

   dst += start;
   if (resources) {
      for (i = 0; i < count; i++)
         pipe_resource_reference(&dst[i], resources[i]);
   }
   else {
      for (i = 0; i < count; i++)
         pipe_resource_reference(&dst[i], NULL);
   }

   if (ilo->global_binding.count <= start + count) {
      count += start;

      while (count > 0 && !ilo->global_binding.resources[count - 1])
         count--;

      ilo->global_binding.count = count;
   }

   ilo->dirty |= ILO_DIRTY_GLOBAL_BINDING;
}

/**
 * Initialize state-related functions.
 */
void
ilo_init_state_functions(struct ilo_context *ilo)
{
   STATIC_ASSERT(ILO_STATE_COUNT <= 32);

   ilo->base.create_blend_state = ilo_create_blend_state;
   ilo->base.bind_blend_state = ilo_bind_blend_state;
   ilo->base.delete_blend_state = ilo_delete_blend_state;
   ilo->base.create_sampler_state = ilo_create_sampler_state;
   ilo->base.bind_fragment_sampler_states = ilo_bind_fragment_sampler_states;
   ilo->base.bind_vertex_sampler_states = ilo_bind_vertex_sampler_states;
   ilo->base.bind_geometry_sampler_states = ilo_bind_geometry_sampler_states;
   ilo->base.bind_compute_sampler_states = ilo_bind_compute_sampler_states;
   ilo->base.delete_sampler_state = ilo_delete_sampler_state;
   ilo->base.create_rasterizer_state = ilo_create_rasterizer_state;
   ilo->base.bind_rasterizer_state = ilo_bind_rasterizer_state;
   ilo->base.delete_rasterizer_state = ilo_delete_rasterizer_state;
   ilo->base.create_depth_stencil_alpha_state = ilo_create_depth_stencil_alpha_state;
   ilo->base.bind_depth_stencil_alpha_state = ilo_bind_depth_stencil_alpha_state;
   ilo->base.delete_depth_stencil_alpha_state = ilo_delete_depth_stencil_alpha_state;
   ilo->base.create_fs_state = ilo_create_fs_state;
   ilo->base.bind_fs_state = ilo_bind_fs_state;
   ilo->base.delete_fs_state = ilo_delete_fs_state;
   ilo->base.create_vs_state = ilo_create_vs_state;
   ilo->base.bind_vs_state = ilo_bind_vs_state;
   ilo->base.delete_vs_state = ilo_delete_vs_state;
   ilo->base.create_gs_state = ilo_create_gs_state;
   ilo->base.bind_gs_state = ilo_bind_gs_state;
   ilo->base.delete_gs_state = ilo_delete_gs_state;
   ilo->base.create_vertex_elements_state = ilo_create_vertex_elements_state;
   ilo->base.bind_vertex_elements_state = ilo_bind_vertex_elements_state;
   ilo->base.delete_vertex_elements_state = ilo_delete_vertex_elements_state;

   ilo->base.set_blend_color = ilo_set_blend_color;
   ilo->base.set_stencil_ref = ilo_set_stencil_ref;
   ilo->base.set_sample_mask = ilo_set_sample_mask;
   ilo->base.set_clip_state = ilo_set_clip_state;
   ilo->base.set_constant_buffer = ilo_set_constant_buffer;
   ilo->base.set_framebuffer_state = ilo_set_framebuffer_state;
   ilo->base.set_polygon_stipple = ilo_set_polygon_stipple;
   ilo->base.set_scissor_states = ilo_set_scissor_states;
   ilo->base.set_viewport_states = ilo_set_viewport_states;
   ilo->base.set_fragment_sampler_views = ilo_set_fragment_sampler_views;
   ilo->base.set_vertex_sampler_views = ilo_set_vertex_sampler_views;
   ilo->base.set_geometry_sampler_views = ilo_set_geometry_sampler_views;
   ilo->base.set_compute_sampler_views = ilo_set_compute_sampler_views;
   ilo->base.set_shader_resources = ilo_set_shader_resources;
   ilo->base.set_vertex_buffers = ilo_set_vertex_buffers;
   ilo->base.set_index_buffer = ilo_set_index_buffer;

   ilo->base.create_stream_output_target = ilo_create_stream_output_target;
   ilo->base.stream_output_target_destroy = ilo_stream_output_target_destroy;
   ilo->base.set_stream_output_targets = ilo_set_stream_output_targets;

   ilo->base.create_sampler_view = ilo_create_sampler_view;
   ilo->base.sampler_view_destroy = ilo_sampler_view_destroy;

   ilo->base.create_surface = ilo_create_surface;
   ilo->base.surface_destroy = ilo_surface_destroy;

   ilo->base.create_compute_state = ilo_create_compute_state;
   ilo->base.bind_compute_state = ilo_bind_compute_state;
   ilo->base.delete_compute_state = ilo_delete_compute_state;
   ilo->base.set_compute_resources = ilo_set_compute_resources;
   ilo->base.set_global_binding = ilo_set_global_binding;
}

void
ilo_init_states(struct ilo_context *ilo)
{
   ilo_gpe_set_scissor_null(ilo->dev, &ilo->scissor);
}

void
ilo_cleanup_states(struct ilo_context *ilo)
{
}

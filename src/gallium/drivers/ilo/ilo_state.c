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

#include "util/u_helpers.h"
#include "util/u_upload_mgr.h"

#include "ilo_context.h"
#include "ilo_resource.h"
#include "ilo_shader.h"
#include "ilo_state.h"

static void
finalize_shader_states(struct ilo_context *ilo)
{
   unsigned type;

   for (type = 0; type < PIPE_SHADER_TYPES; type++) {
      struct ilo_shader_state *shader;
      uint32_t state;

      switch (type) {
      case PIPE_SHADER_VERTEX:
         shader = ilo->vs;
         state = ILO_DIRTY_VS;
         break;
      case PIPE_SHADER_GEOMETRY:
         shader = ilo->gs;
         state = ILO_DIRTY_GS;
         break;
      case PIPE_SHADER_FRAGMENT:
         shader = ilo->fs;
         state = ILO_DIRTY_FS;
         break;
      default:
         shader = NULL;
         state = 0;
         break;
      }

      if (!shader)
         continue;

      /* compile if the shader or the states it depends on changed */
      if (ilo->dirty & state) {
         ilo_shader_select_kernel(shader, ilo, ILO_DIRTY_ALL);
      }
      else if (ilo_shader_select_kernel(shader, ilo, ilo->dirty)) {
         /* mark the state dirty if a new kernel is selected */
         ilo->dirty |= state;
      }

      /* need to setup SBE for FS */
      if (type == PIPE_SHADER_FRAGMENT && ilo->dirty &
            (state | ILO_DIRTY_GS | ILO_DIRTY_VS | ILO_DIRTY_RASTERIZER)) {
         if (ilo_shader_select_kernel_routing(shader,
               (ilo->gs) ? ilo->gs : ilo->vs, ilo->rasterizer))
            ilo->dirty |= state;
      }
   }
}

static void
finalize_cbuf_state(struct ilo_context *ilo,
                    struct ilo_cbuf_state *cbuf,
                    const struct ilo_shader_state *sh)
{
   uint32_t upload_mask = cbuf->enabled_mask;

   /* skip CBUF0 if the kernel does not need it */
   upload_mask &=
      ~ilo_shader_get_kernel_param(sh, ILO_KERNEL_SKIP_CBUF0_UPLOAD);

   while (upload_mask) {
      const enum pipe_format elem_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
      unsigned offset, i;

      i = u_bit_scan(&upload_mask);
      /* no need to upload */
      if (cbuf->cso[i].resource)
         continue;

      u_upload_data(ilo->uploader, 0, cbuf->cso[i].user_buffer_size,
            cbuf->cso[i].user_buffer, &offset, &cbuf->cso[i].resource);

      ilo_gpe_init_view_surface_for_buffer(ilo->dev,
            ilo_buffer(cbuf->cso[i].resource),
            offset, cbuf->cso[i].user_buffer_size,
            util_format_get_blocksize(elem_format), elem_format,
            false, false, &cbuf->cso[i].surface);

      ilo->dirty |= ILO_DIRTY_CBUF;
   }
}

static void
finalize_constant_buffers(struct ilo_context *ilo)
{
   if (ilo->dirty & (ILO_DIRTY_CBUF | ILO_DIRTY_VS))
      finalize_cbuf_state(ilo, &ilo->cbuf[PIPE_SHADER_VERTEX], ilo->vs);

   if (ilo->dirty & (ILO_DIRTY_CBUF | ILO_DIRTY_FS))
      finalize_cbuf_state(ilo, &ilo->cbuf[PIPE_SHADER_FRAGMENT], ilo->fs);
}

static void
finalize_index_buffer(struct ilo_context *ilo)
{
   const bool need_upload = (ilo->draw->indexed &&
         (ilo->ib.user_buffer || ilo->ib.offset % ilo->ib.index_size));
   struct pipe_resource *current_hw_res = NULL;

   if (!(ilo->dirty & ILO_DIRTY_IB) && !need_upload)
      return;

   pipe_resource_reference(&current_hw_res, ilo->ib.hw_resource);

   if (need_upload) {
      const unsigned offset = ilo->ib.index_size * ilo->draw->start;
      const unsigned size = ilo->ib.index_size * ilo->draw->count;
      unsigned hw_offset;

      if (ilo->ib.user_buffer) {
         u_upload_data(ilo->uploader, 0, size,
               ilo->ib.user_buffer + offset, &hw_offset, &ilo->ib.hw_resource);
      }
      else {
         u_upload_buffer(ilo->uploader, 0, ilo->ib.offset + offset, size,
               ilo->ib.buffer, &hw_offset, &ilo->ib.hw_resource);
      }

      /* the HW offset should be aligned */
      assert(hw_offset % ilo->ib.index_size == 0);
      ilo->ib.draw_start_offset = hw_offset / ilo->ib.index_size;

      /*
       * INDEX[ilo->draw->start] in the original buffer is INDEX[0] in the HW
       * resource
       */
      ilo->ib.draw_start_offset -= ilo->draw->start;
   }
   else {
      pipe_resource_reference(&ilo->ib.hw_resource, ilo->ib.buffer);

      /* note that index size may be zero when the draw is not indexed */
      if (ilo->draw->indexed)
         ilo->ib.draw_start_offset = ilo->ib.offset / ilo->ib.index_size;
      else
         ilo->ib.draw_start_offset = 0;
   }

   /* treat the IB as clean if the HW states do not change */
   if (ilo->ib.hw_resource == current_hw_res &&
       ilo->ib.hw_index_size == ilo->ib.index_size)
      ilo->dirty &= ~ILO_DIRTY_IB;
   else
      ilo->ib.hw_index_size = ilo->ib.index_size;

   pipe_resource_reference(&current_hw_res, NULL);
}

/**
 * Finalize states.  Some states depend on other states and are
 * incomplete/invalid until finalized.
 */
void
ilo_finalize_3d_states(struct ilo_context *ilo,
                       const struct pipe_draw_info *draw)
{
   ilo->draw = draw;

   finalize_shader_states(ilo);
   finalize_constant_buffers(ilo);
   finalize_index_buffer(ilo);

   u_upload_unmap(ilo->uploader);
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
ilo_bind_sampler_states(struct pipe_context *pipe, unsigned shader,
                        unsigned start, unsigned count, void **samplers)
{
   struct ilo_context *ilo = ilo_context(pipe);
   struct ilo_sampler_state *dst = &ilo->sampler[shader];
   bool changed = false;
   unsigned i;

   assert(start + count <= Elements(dst->cso));

   if (samplers) {
      for (i = 0; i < count; i++) {
         if (dst->cso[start + i] != samplers[i]) {
            dst->cso[start + i] = samplers[i];

            /*
             * This function is sometimes called to reduce the number of bound
             * samplers.  Do not consider that as a state change (and create a
             * new array of SAMPLER_STATE).
             */
            if (samplers[i])
               changed = true;
         }
      }
   }
   else {
      for (i = 0; i < count; i++)
         dst->cso[start + i] = NULL;
   }

   if (dst->count <= start + count) {
      if (samplers)
         count += start;
      else
         count = start;

      while (count > 0 && !dst->cso[count - 1])
         count--;

      dst->count = count;
   }

   if (changed) {
      switch (shader) {
      case PIPE_SHADER_VERTEX:
         ilo->dirty |= ILO_DIRTY_SAMPLER_VS;
         break;
      case PIPE_SHADER_GEOMETRY:
         ilo->dirty |= ILO_DIRTY_SAMPLER_GS;
         break;
      case PIPE_SHADER_FRAGMENT:
         ilo->dirty |= ILO_DIRTY_SAMPLER_FS;
         break;
      case PIPE_SHADER_COMPUTE:
         ilo->dirty |= ILO_DIRTY_SAMPLER_CS;
         break;
      }
   }
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
   struct ilo_context *ilo = ilo_context(pipe);
   struct ilo_rasterizer_state *rast;

   rast = MALLOC_STRUCT(ilo_rasterizer_state);
   assert(rast);

   rast->state = *state;
   ilo_gpe_init_rasterizer(ilo->dev, state, rast);

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
   struct ilo_context *ilo = ilo_context(pipe);
   struct ilo_dsa_state *dsa;

   dsa = MALLOC_STRUCT(ilo_dsa_state);
   assert(dsa);

   ilo_gpe_init_dsa(ilo->dev, state, dsa);

   return dsa;
}

static void
ilo_bind_depth_stencil_alpha_state(struct pipe_context *pipe, void *state)
{
   struct ilo_context *ilo = ilo_context(pipe);

   ilo->dsa = state;

   ilo->dirty |= ILO_DIRTY_DSA;
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
   struct ilo_shader_state *shader;

   shader = ilo_shader_create_fs(ilo->dev, state, ilo);
   assert(shader);

   ilo_shader_cache_add(ilo->shader_cache, shader);

   return shader;
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
   struct ilo_context *ilo = ilo_context(pipe);
   struct ilo_shader_state *fs = (struct ilo_shader_state *) state;

   ilo_shader_cache_remove(ilo->shader_cache, fs);
   ilo_shader_destroy(fs);
}

static void *
ilo_create_vs_state(struct pipe_context *pipe,
                    const struct pipe_shader_state *state)
{
   struct ilo_context *ilo = ilo_context(pipe);
   struct ilo_shader_state *shader;

   shader = ilo_shader_create_vs(ilo->dev, state, ilo);
   assert(shader);

   ilo_shader_cache_add(ilo->shader_cache, shader);

   return shader;
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
   struct ilo_context *ilo = ilo_context(pipe);
   struct ilo_shader_state *vs = (struct ilo_shader_state *) state;

   ilo_shader_cache_remove(ilo->shader_cache, vs);
   ilo_shader_destroy(vs);
}

static void *
ilo_create_gs_state(struct pipe_context *pipe,
                    const struct pipe_shader_state *state)
{
   struct ilo_context *ilo = ilo_context(pipe);
   struct ilo_shader_state *shader;

   shader = ilo_shader_create_gs(ilo->dev, state, ilo);
   assert(shader);

   ilo_shader_cache_add(ilo->shader_cache, shader);

   return shader;
}

static void
ilo_bind_gs_state(struct pipe_context *pipe, void *state)
{
   struct ilo_context *ilo = ilo_context(pipe);

   /* util_blitter may set this unnecessarily */
   if (ilo->gs == state)
      return;

   ilo->gs = state;

   ilo->dirty |= ILO_DIRTY_GS;
}

static void
ilo_delete_gs_state(struct pipe_context *pipe, void *state)
{
   struct ilo_context *ilo = ilo_context(pipe);
   struct ilo_shader_state *gs = (struct ilo_shader_state *) state;

   ilo_shader_cache_remove(ilo->shader_cache, gs);
   ilo_shader_destroy(gs);
}

static void *
ilo_create_vertex_elements_state(struct pipe_context *pipe,
                                 unsigned num_elements,
                                 const struct pipe_vertex_element *elements)
{
   struct ilo_context *ilo = ilo_context(pipe);
   struct ilo_ve_state *ve;

   ve = MALLOC_STRUCT(ilo_ve_state);
   assert(ve);

   ilo_gpe_init_ve(ilo->dev, num_elements, elements, ve);

   return ve;
}

static void
ilo_bind_vertex_elements_state(struct pipe_context *pipe, void *state)
{
   struct ilo_context *ilo = ilo_context(pipe);

   ilo->ve = state;

   ilo->dirty |= ILO_DIRTY_VE;
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

   /* util_blitter may set this unnecessarily */
   if (!memcmp(&ilo->stencil_ref, state, sizeof(*state)))
      return;

   ilo->stencil_ref = *state;

   ilo->dirty |= ILO_DIRTY_STENCIL_REF;
}

static void
ilo_set_sample_mask(struct pipe_context *pipe,
                    unsigned sample_mask)
{
   struct ilo_context *ilo = ilo_context(pipe);

   /* util_blitter may set this unnecessarily */
   if (ilo->sample_mask == sample_mask)
      return;

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
   struct ilo_cbuf_state *cbuf = &ilo->cbuf[shader];
   const unsigned count = 1;
   unsigned i;

   assert(shader < Elements(ilo->cbuf));
   assert(index + count <= Elements(ilo->cbuf[shader].cso));

   if (buf) {
      for (i = 0; i < count; i++) {
         struct ilo_cbuf_cso *cso = &cbuf->cso[index + i];

         pipe_resource_reference(&cso->resource, buf[i].buffer);

         if (buf[i].buffer) {
            const enum pipe_format elem_format =
               PIPE_FORMAT_R32G32B32A32_FLOAT;

            ilo_gpe_init_view_surface_for_buffer(ilo->dev,
                  ilo_buffer(buf[i].buffer),
                  buf[i].buffer_offset, buf[i].buffer_size,
                  util_format_get_blocksize(elem_format), elem_format,
                  false, false, &cso->surface);

            cso->user_buffer = NULL;
            cso->user_buffer_size = 0;

            cbuf->enabled_mask |= 1 << (index + i);
         }
         else if (buf[i].user_buffer) {
            cso->surface.bo = NULL;

            /* buffer_offset does not apply for user buffer */
            cso->user_buffer = buf[i].user_buffer;
            cso->user_buffer_size = buf[i].buffer_size;

            cbuf->enabled_mask |= 1 << (index + i);
         }
         else {
            cso->surface.bo = NULL;
            cso->user_buffer = NULL;
            cso->user_buffer_size = 0;

            cbuf->enabled_mask &= ~(1 << (index + i));
         }
      }
   }
   else {
      for (i = 0; i < count; i++) {
         struct ilo_cbuf_cso *cso = &cbuf->cso[index + i];

         pipe_resource_reference(&cso->resource, NULL);
         cso->surface.bo = NULL;
         cso->user_buffer = NULL;
         cso->user_buffer_size = 0;

         cbuf->enabled_mask &= ~(1 << (index + i));
      }
   }

   ilo->dirty |= ILO_DIRTY_CBUF;
}

static void
ilo_set_framebuffer_state(struct pipe_context *pipe,
                          const struct pipe_framebuffer_state *state)
{
   struct ilo_context *ilo = ilo_context(pipe);

   ilo_gpe_set_fb(ilo->dev, state, &ilo->fb);

   ilo->dirty |= ILO_DIRTY_FB;
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
ilo_set_sampler_views(struct pipe_context *pipe, unsigned shader,
                      unsigned start, unsigned count,
                      struct pipe_sampler_view **views)
{
   struct ilo_context *ilo = ilo_context(pipe);
   struct ilo_view_state *dst = &ilo->view[shader];
   unsigned i;

   assert(start + count <= Elements(dst->states));

   if (views) {
      for (i = 0; i < count; i++)
         pipe_sampler_view_reference(&dst->states[start + i], views[i]);
   }
   else {
      for (i = 0; i < count; i++)
         pipe_sampler_view_reference(&dst->states[start + i], NULL);
   }

   if (dst->count <= start + count) {
      if (views)
         count += start;
      else
         count = start;

      while (count > 0 && !dst->states[count - 1])
         count--;

      dst->count = count;
   }

   switch (shader) {
   case PIPE_SHADER_VERTEX:
      ilo->dirty |= ILO_DIRTY_VIEW_VS;
      break;
   case PIPE_SHADER_GEOMETRY:
      ilo->dirty |= ILO_DIRTY_VIEW_GS;
      break;
   case PIPE_SHADER_FRAGMENT:
      ilo->dirty |= ILO_DIRTY_VIEW_FS;
      break;
   case PIPE_SHADER_COMPUTE:
      ilo->dirty |= ILO_DIRTY_VIEW_CS;
      break;
   }
}

static void
ilo_set_shader_resources(struct pipe_context *pipe,
                         unsigned start, unsigned count,
                         struct pipe_surface **surfaces)
{
   struct ilo_context *ilo = ilo_context(pipe);
   struct ilo_resource_state *dst = &ilo->resource;
   unsigned i;

   assert(start + count <= Elements(dst->states));

   if (surfaces) {
      for (i = 0; i < count; i++)
         pipe_surface_reference(&dst->states[start + i], surfaces[i]);
   }
   else {
      for (i = 0; i < count; i++)
         pipe_surface_reference(&dst->states[start + i], NULL);
   }

   if (dst->count <= start + count) {
      if (surfaces)
         count += start;
      else
         count = start;

      while (count > 0 && !dst->states[count - 1])
         count--;

      dst->count = count;
   }

   ilo->dirty |= ILO_DIRTY_RESOURCE;
}

static void
ilo_set_vertex_buffers(struct pipe_context *pipe,
                       unsigned start_slot, unsigned num_buffers,
                       const struct pipe_vertex_buffer *buffers)
{
   struct ilo_context *ilo = ilo_context(pipe);
   unsigned i;

   /* no PIPE_CAP_USER_VERTEX_BUFFERS */
   if (buffers) {
      for (i = 0; i < num_buffers; i++)
         assert(!buffers[i].user_buffer);
   }

   util_set_vertex_buffers_mask(ilo->vb.states,
         &ilo->vb.enabled_mask, buffers, start_slot, num_buffers);

   ilo->dirty |= ILO_DIRTY_VB;
}

static void
ilo_set_index_buffer(struct pipe_context *pipe,
                     const struct pipe_index_buffer *state)
{
   struct ilo_context *ilo = ilo_context(pipe);

   if (state) {
      pipe_resource_reference(&ilo->ib.buffer, state->buffer);
      ilo->ib.user_buffer = state->user_buffer;
      ilo->ib.offset = state->offset;
      ilo->ib.index_size = state->index_size;
   }
   else {
      pipe_resource_reference(&ilo->ib.buffer, NULL);
      ilo->ib.user_buffer = NULL;
      ilo->ib.offset = 0;
      ilo->ib.index_size = 0;
   }

   ilo->dirty |= ILO_DIRTY_IB;
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
                              const unsigned *offset)
{
   struct ilo_context *ilo = ilo_context(pipe);
   unsigned i;
   unsigned append_bitmask = 0;

   if (!targets)
      num_targets = 0;

   /* util_blitter may set this unnecessarily */
   if (!ilo->so.count && !num_targets)
      return;

   for (i = 0; i < num_targets; i++) {
      pipe_so_target_reference(&ilo->so.states[i], targets[i]);
      if (offset[i] == (unsigned)-1)
         append_bitmask |= 1 << i;
   }

   for (; i < ilo->so.count; i++)
      pipe_so_target_reference(&ilo->so.states[i], NULL);

   ilo->so.count = num_targets;
   ilo->so.append_bitmask = append_bitmask;

   ilo->so.enabled = (ilo->so.count > 0);

   ilo->dirty |= ILO_DIRTY_SO;
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
   struct ilo_context *ilo = ilo_context(pipe);
   struct ilo_view_cso *view;

   view = MALLOC_STRUCT(ilo_view_cso);
   assert(view);

   view->base = *templ;
   pipe_reference_init(&view->base.reference, 1);
   view->base.texture = NULL;
   pipe_resource_reference(&view->base.texture, res);
   view->base.context = pipe;

   if (res->target == PIPE_BUFFER) {
      const unsigned elem_size = util_format_get_blocksize(templ->format);
      const unsigned first_elem = templ->u.buf.first_element;
      const unsigned num_elems = templ->u.buf.last_element - first_elem + 1;

      ilo_gpe_init_view_surface_for_buffer(ilo->dev, ilo_buffer(res),
            first_elem * elem_size, num_elems * elem_size,
            elem_size, templ->format, false, false, &view->surface);
   }
   else {
      struct ilo_texture *tex = ilo_texture(res);

      /* warn about degraded performance because of a missing binding flag */
      if (tex->tiling == INTEL_TILING_NONE &&
          !(tex->base.bind & PIPE_BIND_SAMPLER_VIEW)) {
         ilo_warn("creating sampler view for a resource "
                  "not created for sampling\n");
      }

      ilo_gpe_init_view_surface_for_texture(ilo->dev, tex,
            templ->format,
            templ->u.tex.first_level,
            templ->u.tex.last_level - templ->u.tex.first_level + 1,
            templ->u.tex.first_layer,
            templ->u.tex.last_layer - templ->u.tex.first_layer + 1,
            false, false, &view->surface);
   }

   return &view->base;
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
   struct ilo_context *ilo = ilo_context(pipe);
   struct ilo_surface_cso *surf;

   surf = MALLOC_STRUCT(ilo_surface_cso);
   assert(surf);

   surf->base = *templ;
   pipe_reference_init(&surf->base.reference, 1);
   surf->base.texture = NULL;
   pipe_resource_reference(&surf->base.texture, res);

   surf->base.context = pipe;
   surf->base.width = u_minify(res->width0, templ->u.tex.level);
   surf->base.height = u_minify(res->height0, templ->u.tex.level);

   surf->is_rt = !util_format_is_depth_or_stencil(templ->format);

   if (surf->is_rt) {
      /* relax this? */
      assert(res->target != PIPE_BUFFER);

      /*
       * classic i965 sets render_cache_rw for constant buffers and sol
       * surfaces but not render buffers.  Why?
       */
      ilo_gpe_init_view_surface_for_texture(ilo->dev, ilo_texture(res),
            templ->format, templ->u.tex.level, 1,
            templ->u.tex.first_layer,
            templ->u.tex.last_layer - templ->u.tex.first_layer + 1,
            true, false, &surf->u.rt);
   }
   else {
      assert(res->target != PIPE_BUFFER);

      ilo_gpe_init_zs_surface(ilo->dev, ilo_texture(res),
            templ->format, templ->u.tex.level,
            templ->u.tex.first_layer,
            templ->u.tex.last_layer - templ->u.tex.first_layer + 1,
            false, &surf->u.zs);
   }

   return &surf->base;
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
   struct ilo_shader_state *shader;

   shader = ilo_shader_create_cs(ilo->dev, state, ilo);
   assert(shader);

   ilo_shader_cache_add(ilo->shader_cache, shader);

   return shader;
}

static void
ilo_bind_compute_state(struct pipe_context *pipe, void *state)
{
   struct ilo_context *ilo = ilo_context(pipe);

   ilo->cs = state;

   ilo->dirty |= ILO_DIRTY_CS;
}

static void
ilo_delete_compute_state(struct pipe_context *pipe, void *state)
{
   struct ilo_context *ilo = ilo_context(pipe);
   struct ilo_shader_state *cs = (struct ilo_shader_state *) state;

   ilo_shader_cache_remove(ilo->shader_cache, cs);
   ilo_shader_destroy(cs);
}

static void
ilo_set_compute_resources(struct pipe_context *pipe,
                          unsigned start, unsigned count,
                          struct pipe_surface **surfaces)
{
   struct ilo_context *ilo = ilo_context(pipe);
   struct ilo_resource_state *dst = &ilo->cs_resource;
   unsigned i;

   assert(start + count <= Elements(dst->states));

   if (surfaces) {
      for (i = 0; i < count; i++)
         pipe_surface_reference(&dst->states[start + i], surfaces[i]);
   }
   else {
      for (i = 0; i < count; i++)
         pipe_surface_reference(&dst->states[start + i], NULL);
   }

   if (dst->count <= start + count) {
      if (surfaces)
         count += start;
      else
         count = start;

      while (count > 0 && !dst->states[count - 1])
         count--;

      dst->count = count;
   }

   ilo->dirty |= ILO_DIRTY_CS_RESOURCE;
}

static void
ilo_set_global_binding(struct pipe_context *pipe,
                       unsigned start, unsigned count,
                       struct pipe_resource **resources,
                       uint32_t **handles)
{
   struct ilo_context *ilo = ilo_context(pipe);
   struct ilo_global_binding *dst = &ilo->global_binding;
   unsigned i;

   assert(start + count <= Elements(dst->resources));

   if (resources) {
      for (i = 0; i < count; i++)
         pipe_resource_reference(&dst->resources[start + i], resources[i]);
   }
   else {
      for (i = 0; i < count; i++)
         pipe_resource_reference(&dst->resources[start + i], NULL);
   }

   if (dst->count <= start + count) {
      if (resources)
         count += start;
      else
         count = start;

      while (count > 0 && !dst->resources[count - 1])
         count--;

      dst->count = count;
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
   ilo->base.bind_sampler_states = ilo_bind_sampler_states;
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
   ilo->base.set_sampler_views = ilo_set_sampler_views;
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

   ilo_gpe_init_zs_surface(ilo->dev, NULL, PIPE_FORMAT_NONE,
         0, 0, 1, false, &ilo->fb.null_zs);

   ilo->dirty = ILO_DIRTY_ALL;
}

void
ilo_cleanup_states(struct ilo_context *ilo)
{
   unsigned i, sh;

   for (i = 0; i < Elements(ilo->vb.states); i++) {
      if (ilo->vb.enabled_mask & (1 << i))
         pipe_resource_reference(&ilo->vb.states[i].buffer, NULL);
   }

   pipe_resource_reference(&ilo->ib.buffer, NULL);
   pipe_resource_reference(&ilo->ib.hw_resource, NULL);

   for (i = 0; i < ilo->so.count; i++)
      pipe_so_target_reference(&ilo->so.states[i], NULL);

   for (sh = 0; sh < PIPE_SHADER_TYPES; sh++) {
      for (i = 0; i < ilo->view[sh].count; i++) {
         struct pipe_sampler_view *view = ilo->view[sh].states[i];
         pipe_sampler_view_reference(&view, NULL);
      }

      for (i = 0; i < Elements(ilo->cbuf[sh].cso); i++) {
         struct ilo_cbuf_cso *cbuf = &ilo->cbuf[sh].cso[i];
         pipe_resource_reference(&cbuf->resource, NULL);
      }
   }

   for (i = 0; i < ilo->resource.count; i++)
      pipe_surface_reference(&ilo->resource.states[i], NULL);

   for (i = 0; i < ilo->fb.state.nr_cbufs; i++)
      pipe_surface_reference(&ilo->fb.state.cbufs[i], NULL);

   if (ilo->fb.state.zsbuf)
      pipe_surface_reference(&ilo->fb.state.zsbuf, NULL);

   for (i = 0; i < ilo->cs_resource.count; i++)
      pipe_surface_reference(&ilo->cs_resource.states[i], NULL);

   for (i = 0; i < ilo->global_binding.count; i++)
      pipe_resource_reference(&ilo->global_binding.resources[i], NULL);
}

/**
 * Mark all states that have the resource dirty.
 */
void
ilo_mark_states_with_resource_dirty(struct ilo_context *ilo,
                                    const struct pipe_resource *res)
{
   uint32_t states = 0;
   unsigned sh, i;

   if (res->target == PIPE_BUFFER) {
      uint32_t vb_mask = ilo->vb.enabled_mask;

      while (vb_mask) {
         const unsigned idx = u_bit_scan(&vb_mask);

         if (ilo->vb.states[idx].buffer == res) {
            states |= ILO_DIRTY_VB;
            break;
         }
      }

      if (ilo->ib.buffer == res) {
         states |= ILO_DIRTY_IB;

         /*
          * finalize_index_buffer() has an optimization that clears
          * ILO_DIRTY_IB when the HW states do not change.  However, it fails
          * to flush the VF cache when the HW states do not change, but the
          * contents of the IB has changed.  Here, we set the index size to an
          * invalid value to avoid the optimization.
          */
         ilo->ib.hw_index_size = 0;
      }

      for (i = 0; i < ilo->so.count; i++) {
         if (ilo->so.states[i]->buffer == res) {
            states |= ILO_DIRTY_SO;
            break;
         }
      }
   }

   for (sh = 0; sh < PIPE_SHADER_TYPES; sh++) {
      for (i = 0; i < ilo->view[sh].count; i++) {
         struct pipe_sampler_view *view = ilo->view[sh].states[i];

         if (view->texture == res) {
            static const unsigned view_dirty_bits[PIPE_SHADER_TYPES] = {
               [PIPE_SHADER_VERTEX]    = ILO_DIRTY_VIEW_VS,
               [PIPE_SHADER_FRAGMENT]  = ILO_DIRTY_VIEW_FS,
               [PIPE_SHADER_GEOMETRY]  = ILO_DIRTY_VIEW_GS,
               [PIPE_SHADER_COMPUTE]   = ILO_DIRTY_VIEW_CS,
            };

            states |= view_dirty_bits[sh];
            break;
         }
      }

      if (res->target == PIPE_BUFFER) {
         for (i = 0; i < Elements(ilo->cbuf[sh].cso); i++) {
            struct ilo_cbuf_cso *cbuf = &ilo->cbuf[sh].cso[i];

            if (cbuf->resource == res) {
               states |= ILO_DIRTY_CBUF;
               break;
            }
         }
      }
   }

   for (i = 0; i < ilo->resource.count; i++) {
      if (ilo->resource.states[i]->texture == res) {
         states |= ILO_DIRTY_RESOURCE;
         break;
      }
   }

   /* for now? */
   if (res->target != PIPE_BUFFER) {
      for (i = 0; i < ilo->fb.state.nr_cbufs; i++) {
         const struct pipe_surface *surf = ilo->fb.state.cbufs[i];
         if (surf && surf->texture == res) {
            states |= ILO_DIRTY_FB;
            break;
         }
      }

      if (ilo->fb.state.zsbuf && ilo->fb.state.zsbuf->texture == res)
         states |= ILO_DIRTY_FB;
   }

   for (i = 0; i < ilo->cs_resource.count; i++) {
      pipe_surface_reference(&ilo->cs_resource.states[i], NULL);
      if (ilo->cs_resource.states[i]->texture == res) {
         states |= ILO_DIRTY_CS_RESOURCE;
         break;
      }
   }

   for (i = 0; i < ilo->global_binding.count; i++) {
      if (ilo->global_binding.resources[i] == res) {
         states |= ILO_DIRTY_GLOBAL_BINDING;
         break;
      }
   }

   ilo->dirty |= states;
}

void
ilo_dump_dirty_flags(uint32_t dirty)
{
   static const char *state_names[ILO_STATE_COUNT] = {
      [ILO_STATE_VB]              = "VB",
      [ILO_STATE_VE]              = "VE",
      [ILO_STATE_IB]              = "IB",
      [ILO_STATE_VS]              = "VS",
      [ILO_STATE_GS]              = "GS",
      [ILO_STATE_SO]              = "SO",
      [ILO_STATE_CLIP]            = "CLIP",
      [ILO_STATE_VIEWPORT]        = "VIEWPORT",
      [ILO_STATE_SCISSOR]         = "SCISSOR",
      [ILO_STATE_RASTERIZER]      = "RASTERIZER",
      [ILO_STATE_POLY_STIPPLE]    = "POLY_STIPPLE",
      [ILO_STATE_SAMPLE_MASK]     = "SAMPLE_MASK",
      [ILO_STATE_FS]              = "FS",
      [ILO_STATE_DSA]             = "DSA",
      [ILO_STATE_STENCIL_REF]     = "STENCIL_REF",
      [ILO_STATE_BLEND]           = "BLEND",
      [ILO_STATE_BLEND_COLOR]     = "BLEND_COLOR",
      [ILO_STATE_FB]              = "FB",
      [ILO_STATE_SAMPLER_VS]      = "SAMPLER_VS",
      [ILO_STATE_SAMPLER_GS]      = "SAMPLER_GS",
      [ILO_STATE_SAMPLER_FS]      = "SAMPLER_FS",
      [ILO_STATE_SAMPLER_CS]      = "SAMPLER_CS",
      [ILO_STATE_VIEW_VS]         = "VIEW_VS",
      [ILO_STATE_VIEW_GS]         = "VIEW_GS",
      [ILO_STATE_VIEW_FS]         = "VIEW_FS",
      [ILO_STATE_VIEW_CS]         = "VIEW_CS",
      [ILO_STATE_CBUF]            = "CBUF",
      [ILO_STATE_RESOURCE]        = "RESOURCE",
      [ILO_STATE_CS]              = "CS",
      [ILO_STATE_CS_RESOURCE]     = "CS_RESOURCE",
      [ILO_STATE_GLOBAL_BINDING]  = "GLOBAL_BINDING",
   };

   if (!dirty) {
      ilo_printf("no state is dirty\n");
      return;
   }

   dirty &= (1U << ILO_STATE_COUNT) - 1;

   ilo_printf("%2d states are dirty:", util_bitcount(dirty));
   while (dirty) {
      const enum ilo_state state = u_bit_scan(&dirty);
      ilo_printf(" %s", state_names[state]);
   }
   ilo_printf("\n");
}

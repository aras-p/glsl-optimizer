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
#include "ilo_state_gen.h"

static void
finalize_shader_states(struct ilo_state_vector *vec)
{
   unsigned type;

   for (type = 0; type < PIPE_SHADER_TYPES; type++) {
      struct ilo_shader_state *shader;
      uint32_t state;

      switch (type) {
      case PIPE_SHADER_VERTEX:
         shader = vec->vs;
         state = ILO_DIRTY_VS;
         break;
      case PIPE_SHADER_GEOMETRY:
         shader = vec->gs;
         state = ILO_DIRTY_GS;
         break;
      case PIPE_SHADER_FRAGMENT:
         shader = vec->fs;
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
      if (vec->dirty & state) {
         ilo_shader_select_kernel(shader, vec, ILO_DIRTY_ALL);
      }
      else if (ilo_shader_select_kernel(shader, vec, vec->dirty)) {
         /* mark the state dirty if a new kernel is selected */
         vec->dirty |= state;
      }

      /* need to setup SBE for FS */
      if (type == PIPE_SHADER_FRAGMENT && vec->dirty &
            (state | ILO_DIRTY_GS | ILO_DIRTY_VS | ILO_DIRTY_RASTERIZER)) {
         if (ilo_shader_select_kernel_routing(shader,
               (vec->gs) ? vec->gs : vec->vs, vec->rasterizer))
            vec->dirty |= state;
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

      ilo->state_vector.dirty |= ILO_DIRTY_CBUF;
   }
}

static void
finalize_constant_buffers(struct ilo_context *ilo)
{
   struct ilo_state_vector *vec = &ilo->state_vector;

   if (vec->dirty & (ILO_DIRTY_CBUF | ILO_DIRTY_VS))
      finalize_cbuf_state(ilo, &vec->cbuf[PIPE_SHADER_VERTEX], vec->vs);

   if (ilo->state_vector.dirty & (ILO_DIRTY_CBUF | ILO_DIRTY_FS))
      finalize_cbuf_state(ilo, &vec->cbuf[PIPE_SHADER_FRAGMENT], vec->fs);
}

static void
finalize_index_buffer(struct ilo_context *ilo)
{
   struct ilo_state_vector *vec = &ilo->state_vector;
   const bool need_upload = (vec->draw->indexed &&
         (vec->ib.user_buffer || vec->ib.offset % vec->ib.index_size));
   struct pipe_resource *current_hw_res = NULL;

   if (!(vec->dirty & ILO_DIRTY_IB) && !need_upload)
      return;

   pipe_resource_reference(&current_hw_res, vec->ib.hw_resource);

   if (need_upload) {
      const unsigned offset = vec->ib.index_size * vec->draw->start;
      const unsigned size = vec->ib.index_size * vec->draw->count;
      unsigned hw_offset;

      if (vec->ib.user_buffer) {
         u_upload_data(ilo->uploader, 0, size,
               vec->ib.user_buffer + offset, &hw_offset, &vec->ib.hw_resource);
      }
      else {
         u_upload_buffer(ilo->uploader, 0, vec->ib.offset + offset, size,
               vec->ib.buffer, &hw_offset, &vec->ib.hw_resource);
      }

      /* the HW offset should be aligned */
      assert(hw_offset % vec->ib.index_size == 0);
      vec->ib.draw_start_offset = hw_offset / vec->ib.index_size;

      /*
       * INDEX[vec->draw->start] in the original buffer is INDEX[0] in the HW
       * resource
       */
      vec->ib.draw_start_offset -= vec->draw->start;
   }
   else {
      pipe_resource_reference(&vec->ib.hw_resource, vec->ib.buffer);

      /* note that index size may be zero when the draw is not indexed */
      if (vec->draw->indexed)
         vec->ib.draw_start_offset = vec->ib.offset / vec->ib.index_size;
      else
         vec->ib.draw_start_offset = 0;
   }

   /* treat the IB as clean if the HW states do not change */
   if (vec->ib.hw_resource == current_hw_res &&
       vec->ib.hw_index_size == vec->ib.index_size)
      vec->dirty &= ~ILO_DIRTY_IB;
   else
      vec->ib.hw_index_size = vec->ib.index_size;

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
   ilo->state_vector.draw = draw;

   finalize_shader_states(&ilo->state_vector);
   finalize_constant_buffers(ilo);
   finalize_index_buffer(ilo);

   u_upload_unmap(ilo->uploader);
}

static void *
ilo_create_blend_state(struct pipe_context *pipe,
                       const struct pipe_blend_state *state)
{
   const struct ilo_dev_info *dev = ilo_context(pipe)->dev;
   struct ilo_blend_state *blend;

   blend = MALLOC_STRUCT(ilo_blend_state);
   assert(blend);

   ilo_gpe_init_blend(dev, state, blend);

   return blend;
}

static void
ilo_bind_blend_state(struct pipe_context *pipe, void *state)
{
   struct ilo_state_vector *vec = &ilo_context(pipe)->state_vector;

   vec->blend = state;

   vec->dirty |= ILO_DIRTY_BLEND;
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
   const struct ilo_dev_info *dev = ilo_context(pipe)->dev;
   struct ilo_sampler_cso *sampler;

   sampler = MALLOC_STRUCT(ilo_sampler_cso);
   assert(sampler);

   ilo_gpe_init_sampler_cso(dev, state, sampler);

   return sampler;
}

static void
ilo_bind_sampler_states(struct pipe_context *pipe, unsigned shader,
                        unsigned start, unsigned count, void **samplers)
{
   struct ilo_state_vector *vec = &ilo_context(pipe)->state_vector;
   struct ilo_sampler_state *dst = &vec->sampler[shader];
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
         vec->dirty |= ILO_DIRTY_SAMPLER_VS;
         break;
      case PIPE_SHADER_GEOMETRY:
         vec->dirty |= ILO_DIRTY_SAMPLER_GS;
         break;
      case PIPE_SHADER_FRAGMENT:
         vec->dirty |= ILO_DIRTY_SAMPLER_FS;
         break;
      case PIPE_SHADER_COMPUTE:
         vec->dirty |= ILO_DIRTY_SAMPLER_CS;
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
   const struct ilo_dev_info *dev = ilo_context(pipe)->dev;
   struct ilo_rasterizer_state *rast;

   rast = MALLOC_STRUCT(ilo_rasterizer_state);
   assert(rast);

   rast->state = *state;
   ilo_gpe_init_rasterizer(dev, state, rast);

   return rast;
}

static void
ilo_bind_rasterizer_state(struct pipe_context *pipe, void *state)
{
   struct ilo_state_vector *vec = &ilo_context(pipe)->state_vector;

   vec->rasterizer = state;

   vec->dirty |= ILO_DIRTY_RASTERIZER;
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
   const struct ilo_dev_info *dev = ilo_context(pipe)->dev;
   struct ilo_dsa_state *dsa;

   dsa = MALLOC_STRUCT(ilo_dsa_state);
   assert(dsa);

   ilo_gpe_init_dsa(dev, state, dsa);

   return dsa;
}

static void
ilo_bind_depth_stencil_alpha_state(struct pipe_context *pipe, void *state)
{
   struct ilo_state_vector *vec = &ilo_context(pipe)->state_vector;

   vec->dsa = state;

   vec->dirty |= ILO_DIRTY_DSA;
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

   shader = ilo_shader_create_fs(ilo->dev, state, &ilo->state_vector);
   assert(shader);

   ilo_shader_cache_add(ilo->shader_cache, shader);

   return shader;
}

static void
ilo_bind_fs_state(struct pipe_context *pipe, void *state)
{
   struct ilo_state_vector *vec = &ilo_context(pipe)->state_vector;

   vec->fs = state;

   vec->dirty |= ILO_DIRTY_FS;
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

   shader = ilo_shader_create_vs(ilo->dev, state, &ilo->state_vector);
   assert(shader);

   ilo_shader_cache_add(ilo->shader_cache, shader);

   return shader;
}

static void
ilo_bind_vs_state(struct pipe_context *pipe, void *state)
{
   struct ilo_state_vector *vec = &ilo_context(pipe)->state_vector;

   vec->vs = state;

   vec->dirty |= ILO_DIRTY_VS;
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

   shader = ilo_shader_create_gs(ilo->dev, state, &ilo->state_vector);
   assert(shader);

   ilo_shader_cache_add(ilo->shader_cache, shader);

   return shader;
}

static void
ilo_bind_gs_state(struct pipe_context *pipe, void *state)
{
   struct ilo_state_vector *vec = &ilo_context(pipe)->state_vector;

   /* util_blitter may set this unnecessarily */
   if (vec->gs == state)
      return;

   vec->gs = state;

   vec->dirty |= ILO_DIRTY_GS;
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
   const struct ilo_dev_info *dev = ilo_context(pipe)->dev;
   struct ilo_ve_state *ve;

   ve = MALLOC_STRUCT(ilo_ve_state);
   assert(ve);

   ilo_gpe_init_ve(dev, num_elements, elements, ve);

   return ve;
}

static void
ilo_bind_vertex_elements_state(struct pipe_context *pipe, void *state)
{
   struct ilo_state_vector *vec = &ilo_context(pipe)->state_vector;

   vec->ve = state;

   vec->dirty |= ILO_DIRTY_VE;
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
   struct ilo_state_vector *vec = &ilo_context(pipe)->state_vector;

   vec->blend_color = *state;

   vec->dirty |= ILO_DIRTY_BLEND_COLOR;
}

static void
ilo_set_stencil_ref(struct pipe_context *pipe,
                    const struct pipe_stencil_ref *state)
{
   struct ilo_state_vector *vec = &ilo_context(pipe)->state_vector;

   /* util_blitter may set this unnecessarily */
   if (!memcmp(&vec->stencil_ref, state, sizeof(*state)))
      return;

   vec->stencil_ref = *state;

   vec->dirty |= ILO_DIRTY_STENCIL_REF;
}

static void
ilo_set_sample_mask(struct pipe_context *pipe,
                    unsigned sample_mask)
{
   struct ilo_state_vector *vec = &ilo_context(pipe)->state_vector;

   /* util_blitter may set this unnecessarily */
   if (vec->sample_mask == sample_mask)
      return;

   vec->sample_mask = sample_mask;

   vec->dirty |= ILO_DIRTY_SAMPLE_MASK;
}

static void
ilo_set_clip_state(struct pipe_context *pipe,
                   const struct pipe_clip_state *state)
{
   struct ilo_state_vector *vec = &ilo_context(pipe)->state_vector;

   vec->clip = *state;

   vec->dirty |= ILO_DIRTY_CLIP;
}

static void
ilo_set_constant_buffer(struct pipe_context *pipe,
                        uint shader, uint index,
                        struct pipe_constant_buffer *buf)
{
   const struct ilo_dev_info *dev = ilo_context(pipe)->dev;
   struct ilo_state_vector *vec = &ilo_context(pipe)->state_vector;
   struct ilo_cbuf_state *cbuf = &vec->cbuf[shader];
   const unsigned count = 1;
   unsigned i;

   assert(shader < Elements(vec->cbuf));
   assert(index + count <= Elements(vec->cbuf[shader].cso));

   if (buf) {
      for (i = 0; i < count; i++) {
         struct ilo_cbuf_cso *cso = &cbuf->cso[index + i];

         pipe_resource_reference(&cso->resource, buf[i].buffer);

         if (buf[i].buffer) {
            const enum pipe_format elem_format =
               PIPE_FORMAT_R32G32B32A32_FLOAT;

            ilo_gpe_init_view_surface_for_buffer(dev,
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

   vec->dirty |= ILO_DIRTY_CBUF;
}

static void
ilo_set_framebuffer_state(struct pipe_context *pipe,
                          const struct pipe_framebuffer_state *state)
{
   const struct ilo_dev_info *dev = ilo_context(pipe)->dev;
   struct ilo_state_vector *vec = &ilo_context(pipe)->state_vector;

   ilo_gpe_set_fb(dev, state, &vec->fb);

   vec->dirty |= ILO_DIRTY_FB;
}

static void
ilo_set_polygon_stipple(struct pipe_context *pipe,
                        const struct pipe_poly_stipple *state)
{
   struct ilo_state_vector *vec = &ilo_context(pipe)->state_vector;

   vec->poly_stipple = *state;

   vec->dirty |= ILO_DIRTY_POLY_STIPPLE;
}

static void
ilo_set_scissor_states(struct pipe_context *pipe,
                       unsigned start_slot,
                       unsigned num_scissors,
                       const struct pipe_scissor_state *scissors)
{
   const struct ilo_dev_info *dev = ilo_context(pipe)->dev;
   struct ilo_state_vector *vec = &ilo_context(pipe)->state_vector;

   ilo_gpe_set_scissor(dev, start_slot, num_scissors,
         scissors, &vec->scissor);

   vec->dirty |= ILO_DIRTY_SCISSOR;
}

static void
ilo_set_viewport_states(struct pipe_context *pipe,
                        unsigned start_slot,
                        unsigned num_viewports,
                        const struct pipe_viewport_state *viewports)
{
   const struct ilo_dev_info *dev = ilo_context(pipe)->dev;
   struct ilo_state_vector *vec = &ilo_context(pipe)->state_vector;

   if (viewports) {
      unsigned i;

      for (i = 0; i < num_viewports; i++) {
         ilo_gpe_set_viewport_cso(dev, &viewports[i],
               &vec->viewport.cso[start_slot + i]);
      }

      if (vec->viewport.count < start_slot + num_viewports)
         vec->viewport.count = start_slot + num_viewports;

      /* need to save viewport 0 for util_blitter */
      if (!start_slot && num_viewports)
         vec->viewport.viewport0 = viewports[0];
   }
   else {
      if (vec->viewport.count <= start_slot + num_viewports &&
          vec->viewport.count > start_slot)
         vec->viewport.count = start_slot;
   }

   vec->dirty |= ILO_DIRTY_VIEWPORT;
}

static void
ilo_set_sampler_views(struct pipe_context *pipe, unsigned shader,
                      unsigned start, unsigned count,
                      struct pipe_sampler_view **views)
{
   struct ilo_state_vector *vec = &ilo_context(pipe)->state_vector;
   struct ilo_view_state *dst = &vec->view[shader];
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
      vec->dirty |= ILO_DIRTY_VIEW_VS;
      break;
   case PIPE_SHADER_GEOMETRY:
      vec->dirty |= ILO_DIRTY_VIEW_GS;
      break;
   case PIPE_SHADER_FRAGMENT:
      vec->dirty |= ILO_DIRTY_VIEW_FS;
      break;
   case PIPE_SHADER_COMPUTE:
      vec->dirty |= ILO_DIRTY_VIEW_CS;
      break;
   }
}

static void
ilo_set_shader_resources(struct pipe_context *pipe,
                         unsigned start, unsigned count,
                         struct pipe_surface **surfaces)
{
   struct ilo_state_vector *vec = &ilo_context(pipe)->state_vector;
   struct ilo_resource_state *dst = &vec->resource;
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

   vec->dirty |= ILO_DIRTY_RESOURCE;
}

static void
ilo_set_vertex_buffers(struct pipe_context *pipe,
                       unsigned start_slot, unsigned num_buffers,
                       const struct pipe_vertex_buffer *buffers)
{
   struct ilo_state_vector *vec = &ilo_context(pipe)->state_vector;
   unsigned i;

   /* no PIPE_CAP_USER_VERTEX_BUFFERS */
   if (buffers) {
      for (i = 0; i < num_buffers; i++)
         assert(!buffers[i].user_buffer);
   }

   util_set_vertex_buffers_mask(vec->vb.states,
         &vec->vb.enabled_mask, buffers, start_slot, num_buffers);

   vec->dirty |= ILO_DIRTY_VB;
}

static void
ilo_set_index_buffer(struct pipe_context *pipe,
                     const struct pipe_index_buffer *state)
{
   struct ilo_state_vector *vec = &ilo_context(pipe)->state_vector;

   if (state) {
      pipe_resource_reference(&vec->ib.buffer, state->buffer);
      vec->ib.user_buffer = state->user_buffer;
      vec->ib.offset = state->offset;
      vec->ib.index_size = state->index_size;
   }
   else {
      pipe_resource_reference(&vec->ib.buffer, NULL);
      vec->ib.user_buffer = NULL;
      vec->ib.offset = 0;
      vec->ib.index_size = 0;
   }

   vec->dirty |= ILO_DIRTY_IB;
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
   struct ilo_state_vector *vec = &ilo_context(pipe)->state_vector;
   unsigned i;
   unsigned append_bitmask = 0;

   if (!targets)
      num_targets = 0;

   /* util_blitter may set this unnecessarily */
   if (!vec->so.count && !num_targets)
      return;

   for (i = 0; i < num_targets; i++) {
      pipe_so_target_reference(&vec->so.states[i], targets[i]);
      if (offset[i] == (unsigned)-1)
         append_bitmask |= 1 << i;
   }

   for (; i < vec->so.count; i++)
      pipe_so_target_reference(&vec->so.states[i], NULL);

   vec->so.count = num_targets;
   vec->so.append_bitmask = append_bitmask;

   vec->so.enabled = (vec->so.count > 0);

   vec->dirty |= ILO_DIRTY_SO;
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
   const struct ilo_dev_info *dev = ilo_context(pipe)->dev;
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

      ilo_gpe_init_view_surface_for_buffer(dev, ilo_buffer(res),
            first_elem * elem_size, num_elems * elem_size,
            elem_size, templ->format, false, false, &view->surface);
   }
   else {
      struct ilo_texture *tex = ilo_texture(res);

      /* warn about degraded performance because of a missing binding flag */
      if (tex->layout.tiling == INTEL_TILING_NONE &&
          !(tex->base.bind & PIPE_BIND_SAMPLER_VIEW)) {
         ilo_warn("creating sampler view for a resource "
                  "not created for sampling\n");
      }

      ilo_gpe_init_view_surface_for_texture(dev, tex,
            templ->format,
            templ->u.tex.first_level,
            templ->u.tex.last_level - templ->u.tex.first_level + 1,
            templ->u.tex.first_layer,
            templ->u.tex.last_layer - templ->u.tex.first_layer + 1,
            false, &view->surface);
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
   const struct ilo_dev_info *dev = ilo_context(pipe)->dev;
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
      ilo_gpe_init_view_surface_for_texture(dev, ilo_texture(res),
            templ->format, templ->u.tex.level, 1,
            templ->u.tex.first_layer,
            templ->u.tex.last_layer - templ->u.tex.first_layer + 1,
            true, &surf->u.rt);
   }
   else {
      assert(res->target != PIPE_BUFFER);

      ilo_gpe_init_zs_surface(dev, ilo_texture(res),
            templ->format, templ->u.tex.level,
            templ->u.tex.first_layer,
            templ->u.tex.last_layer - templ->u.tex.first_layer + 1,
            &surf->u.zs);
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

   shader = ilo_shader_create_cs(ilo->dev, state, &ilo->state_vector);
   assert(shader);

   ilo_shader_cache_add(ilo->shader_cache, shader);

   return shader;
}

static void
ilo_bind_compute_state(struct pipe_context *pipe, void *state)
{
   struct ilo_state_vector *vec = &ilo_context(pipe)->state_vector;

   vec->cs = state;

   vec->dirty |= ILO_DIRTY_CS;
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
   struct ilo_state_vector *vec = &ilo_context(pipe)->state_vector;
   struct ilo_resource_state *dst = &vec->cs_resource;
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

   vec->dirty |= ILO_DIRTY_CS_RESOURCE;
}

static void
ilo_set_global_binding(struct pipe_context *pipe,
                       unsigned start, unsigned count,
                       struct pipe_resource **resources,
                       uint32_t **handles)
{
   struct ilo_state_vector *vec = &ilo_context(pipe)->state_vector;
   struct ilo_global_binding *dst = &vec->global_binding;
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

   vec->dirty |= ILO_DIRTY_GLOBAL_BINDING;
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
ilo_state_vector_init(const struct ilo_dev_info *dev,
                      struct ilo_state_vector *vec)
{
   ilo_gpe_set_scissor_null(dev, &vec->scissor);

   ilo_gpe_init_zs_surface(dev, NULL, PIPE_FORMAT_NONE,
         0, 0, 1, &vec->fb.null_zs);

   vec->dirty = ILO_DIRTY_ALL;
}

void
ilo_state_vector_cleanup(struct ilo_state_vector *vec)
{
   unsigned i, sh;

   for (i = 0; i < Elements(vec->vb.states); i++) {
      if (vec->vb.enabled_mask & (1 << i))
         pipe_resource_reference(&vec->vb.states[i].buffer, NULL);
   }

   pipe_resource_reference(&vec->ib.buffer, NULL);
   pipe_resource_reference(&vec->ib.hw_resource, NULL);

   for (i = 0; i < vec->so.count; i++)
      pipe_so_target_reference(&vec->so.states[i], NULL);

   for (sh = 0; sh < PIPE_SHADER_TYPES; sh++) {
      for (i = 0; i < vec->view[sh].count; i++) {
         struct pipe_sampler_view *view = vec->view[sh].states[i];
         pipe_sampler_view_reference(&view, NULL);
      }

      for (i = 0; i < Elements(vec->cbuf[sh].cso); i++) {
         struct ilo_cbuf_cso *cbuf = &vec->cbuf[sh].cso[i];
         pipe_resource_reference(&cbuf->resource, NULL);
      }
   }

   for (i = 0; i < vec->resource.count; i++)
      pipe_surface_reference(&vec->resource.states[i], NULL);

   for (i = 0; i < vec->fb.state.nr_cbufs; i++)
      pipe_surface_reference(&vec->fb.state.cbufs[i], NULL);

   if (vec->fb.state.zsbuf)
      pipe_surface_reference(&vec->fb.state.zsbuf, NULL);

   for (i = 0; i < vec->cs_resource.count; i++)
      pipe_surface_reference(&vec->cs_resource.states[i], NULL);

   for (i = 0; i < vec->global_binding.count; i++)
      pipe_resource_reference(&vec->global_binding.resources[i], NULL);
}

/**
 * Mark all states that have the resource dirty.
 */
void
ilo_state_vector_resource_renamed(struct ilo_state_vector *vec,
                                  struct pipe_resource *res)
{
   struct intel_bo *bo = ilo_resource_get_bo(res);
   uint32_t states = 0;
   unsigned sh, i;

   if (res->target == PIPE_BUFFER) {
      uint32_t vb_mask = vec->vb.enabled_mask;

      while (vb_mask) {
         const unsigned idx = u_bit_scan(&vb_mask);

         if (vec->vb.states[idx].buffer == res) {
            states |= ILO_DIRTY_VB;
            break;
         }
      }

      if (vec->ib.buffer == res) {
         states |= ILO_DIRTY_IB;

         /*
          * finalize_index_buffer() has an optimization that clears
          * ILO_DIRTY_IB when the HW states do not change.  However, it fails
          * to flush the VF cache when the HW states do not change, but the
          * contents of the IB has changed.  Here, we set the index size to an
          * invalid value to avoid the optimization.
          */
         vec->ib.hw_index_size = 0;
      }

      for (i = 0; i < vec->so.count; i++) {
         if (vec->so.states[i]->buffer == res) {
            states |= ILO_DIRTY_SO;
            break;
         }
      }
   }

   for (sh = 0; sh < PIPE_SHADER_TYPES; sh++) {
      for (i = 0; i < vec->view[sh].count; i++) {
         struct ilo_view_cso *cso = (struct ilo_view_cso *) vec->view[sh].states[i];

         if (cso->base.texture == res) {
            static const unsigned view_dirty_bits[PIPE_SHADER_TYPES] = {
               [PIPE_SHADER_VERTEX]    = ILO_DIRTY_VIEW_VS,
               [PIPE_SHADER_FRAGMENT]  = ILO_DIRTY_VIEW_FS,
               [PIPE_SHADER_GEOMETRY]  = ILO_DIRTY_VIEW_GS,
               [PIPE_SHADER_COMPUTE]   = ILO_DIRTY_VIEW_CS,
            };
            cso->surface.bo = bo;

            states |= view_dirty_bits[sh];
            break;
         }
      }

      if (res->target == PIPE_BUFFER) {
         for (i = 0; i < Elements(vec->cbuf[sh].cso); i++) {
            struct ilo_cbuf_cso *cbuf = &vec->cbuf[sh].cso[i];

            if (cbuf->resource == res) {
               cbuf->surface.bo = bo;
               states |= ILO_DIRTY_CBUF;
               break;
            }
         }
      }
   }

   for (i = 0; i < vec->resource.count; i++) {
      struct ilo_surface_cso *cso =
         (struct ilo_surface_cso *) vec->resource.states[i];

      if (cso->base.texture == res) {
         cso->u.rt.bo = bo;
         states |= ILO_DIRTY_RESOURCE;
         break;
      }
   }

   /* for now? */
   if (res->target != PIPE_BUFFER) {
      for (i = 0; i < vec->fb.state.nr_cbufs; i++) {
         struct ilo_surface_cso *cso =
            (struct ilo_surface_cso *) vec->fb.state.cbufs[i];
         if (cso && cso->base.texture == res) {
            cso->u.rt.bo = bo;
            states |= ILO_DIRTY_FB;
            break;
         }
      }

      if (vec->fb.state.zsbuf && vec->fb.state.zsbuf->texture == res) {
         struct ilo_surface_cso *cso =
            (struct ilo_surface_cso *) vec->fb.state.zsbuf;

         cso->u.rt.bo = bo;
         states |= ILO_DIRTY_FB;
      }
   }

   for (i = 0; i < vec->cs_resource.count; i++) {
      struct ilo_surface_cso *cso =
         (struct ilo_surface_cso *) vec->cs_resource.states[i];
      if (cso->base.texture == res) {
         cso->u.rt.bo = bo;
         states |= ILO_DIRTY_CS_RESOURCE;
         break;
      }
   }

   for (i = 0; i < vec->global_binding.count; i++) {
      if (vec->global_binding.resources[i] == res) {
         states |= ILO_DIRTY_GLOBAL_BINDING;
         break;
      }
   }

   vec->dirty |= states;
}

void
ilo_state_vector_dump_dirty(const struct ilo_state_vector *vec)
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
   uint32_t dirty = vec->dirty;

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

/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2013 LunarG, Inc.
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

#include "genhw/genhw.h"
#include "util/u_dual_blend.h"

#include "ilo_blitter.h"
#include "ilo_builder_3d.h"
#include "ilo_builder_render.h"
#include "ilo_shader.h"
#include "ilo_state.h"
#include "ilo_render.h"
#include "ilo_render_gen.h"
#include "ilo_render_gen7.h"

/**
 * A wrapper for gen6_PIPE_CONTROL().
 */
static inline void
gen7_pipe_control(struct ilo_render *r, uint32_t dw1)
{
   struct intel_bo *bo = (dw1 & GEN6_PIPE_CONTROL_WRITE__MASK) ?
      r->workaround_bo : NULL;

   ILO_DEV_ASSERT(r->dev, 7, 7.5);

   if (dw1 & GEN6_PIPE_CONTROL_CS_STALL) {
      /* CS stall cannot be set alone */
      const uint32_t mask = GEN6_PIPE_CONTROL_RENDER_CACHE_FLUSH |
                            GEN6_PIPE_CONTROL_DEPTH_CACHE_FLUSH |
                            GEN6_PIPE_CONTROL_PIXEL_SCOREBOARD_STALL |
                            GEN6_PIPE_CONTROL_DEPTH_STALL |
                            GEN6_PIPE_CONTROL_WRITE__MASK;
      if (!(dw1 & mask))
         dw1 |= GEN6_PIPE_CONTROL_PIXEL_SCOREBOARD_STALL;
   }

   gen6_PIPE_CONTROL(r->builder, dw1, bo, 0, false);


   r->state.current_pipe_control_dw1 |= dw1;
   r->state.deferred_pipe_control_dw1 &= ~dw1;
}

static void
gen7_wa_post_3dstate_push_constant_alloc_ps(struct ilo_render *r)
{
   /*
    * From the Ivy Bridge PRM, volume 2 part 1, page 292:
    *
    *     "A PIPE_CONTOL command with the CS Stall bit set must be programmed
    *      in the ring after this instruction
    *      (3DSTATE_PUSH_CONSTANT_ALLOC_PS)."
    */
   const uint32_t dw1 = GEN6_PIPE_CONTROL_CS_STALL;

   ILO_DEV_ASSERT(r->dev, 7, 7.5);

   r->state.deferred_pipe_control_dw1 |= dw1;
}

static void
gen7_wa_pre_vs(struct ilo_render *r)
{
   /*
    * From the Ivy Bridge PRM, volume 2 part 1, page 106:
    *
    *     "A PIPE_CONTROL with Post-Sync Operation set to 1h and a depth stall
    *      needs to be sent just prior to any 3DSTATE_VS, 3DSTATE_URB_VS,
    *      3DSTATE_CONSTANT_VS, 3DSTATE_BINDING_TABLE_POINTER_VS,
    *      3DSTATE_SAMPLER_STATE_POINTER_VS command.  Only one PIPE_CONTROL
    *      needs to be sent before any combination of VS associated 3DSTATE."
    */
   const uint32_t dw1 = GEN6_PIPE_CONTROL_DEPTH_STALL |
                        GEN6_PIPE_CONTROL_WRITE_IMM;

   ILO_DEV_ASSERT(r->dev, 7, 7.5);

   if ((r->state.current_pipe_control_dw1 & dw1) != dw1)
      gen7_pipe_control(r, dw1);
}

static void
gen7_wa_pre_3dstate_sf_depth_bias(struct ilo_render *r)
{
   /*
    * From the Ivy Bridge PRM, volume 2 part 1, page 258:
    *
    *     "Due to an HW issue driver needs to send a pipe control with stall
    *      when ever there is state change in depth bias related state (in
    *      3DSTATE_SF)"
    */
   const uint32_t dw1 = GEN6_PIPE_CONTROL_CS_STALL;

   ILO_DEV_ASSERT(r->dev, 7, 7.5);

   if ((r->state.current_pipe_control_dw1 & dw1) != dw1)
      gen7_pipe_control(r, dw1);
}

static void
gen7_wa_pre_3dstate_multisample(struct ilo_render *r)
{
   /*
    * From the Ivy Bridge PRM, volume 2 part 1, page 304:
    *
    *     "Driver must ierarchi that all the caches in the depth pipe are
    *      flushed before this command (3DSTATE_MULTISAMPLE) is parsed. This
    *      requires driver to send a PIPE_CONTROL with a CS stall along with a
    *      Depth Flush prior to this command.
    */
   const uint32_t dw1 = GEN6_PIPE_CONTROL_DEPTH_CACHE_FLUSH |
                        GEN6_PIPE_CONTROL_CS_STALL;

   ILO_DEV_ASSERT(r->dev, 7, 7.5);

   if ((r->state.current_pipe_control_dw1 & dw1) != dw1)
      gen7_pipe_control(r, dw1);
}

static void
gen7_wa_pre_depth(struct ilo_render *r)
{
   /*
    * From the Ivy Bridge PRM, volume 2 part 1, page 315:
    *
    *     "Driver must send a least one PIPE_CONTROL command with CS Stall and
    *      a post sync operation prior to the group of depth
    *      commands(3DSTATE_DEPTH_BUFFER, 3DSTATE_CLEAR_PARAMS,
    *      3DSTATE_STENCIL_BUFFER, and 3DSTATE_HIER_DEPTH_BUFFER)."
    */
   const uint32_t dw1 = GEN6_PIPE_CONTROL_CS_STALL |
                        GEN6_PIPE_CONTROL_WRITE_IMM;

   ILO_DEV_ASSERT(r->dev, 7, 7.5);

   if ((r->state.current_pipe_control_dw1 & dw1) != dw1)
      gen7_pipe_control(r, dw1);

   /*
    * From the Ivy Bridge PRM, volume 2 part 1, page 315:
    *
    *     "Restriction: Prior to changing Depth/Stencil Buffer state (i.e.,
    *      any combination of 3DSTATE_DEPTH_BUFFER, 3DSTATE_CLEAR_PARAMS,
    *      3DSTATE_STENCIL_BUFFER, 3DSTATE_HIER_DEPTH_BUFFER) SW must first
    *      issue a pipelined depth stall (PIPE_CONTROL with Depth Stall bit
    *      set), followed by a pipelined depth cache flush (PIPE_CONTROL with
    *      Depth Flush Bit set, followed by another pipelined depth stall
    *      (PIPE_CONTROL with Depth Stall Bit set), unless SW can otherwise
    *      guarantee that the pipeline from WM onwards is already flushed
    *      (e.g., via a preceding MI_FLUSH)."
    */
   gen7_pipe_control(r, GEN6_PIPE_CONTROL_DEPTH_STALL);
   gen7_pipe_control(r, GEN6_PIPE_CONTROL_DEPTH_CACHE_FLUSH);
   gen7_pipe_control(r, GEN6_PIPE_CONTROL_DEPTH_STALL);
}

static void
gen7_wa_pre_3dstate_ps_max_threads(struct ilo_render *r)
{
   /*
    * From the Ivy Bridge PRM, volume 2 part 1, page 286:
    *
    *     "If this field (Maximum Number of Threads in 3DSTATE_PS) is changed
    *      between 3DPRIMITIVE commands, a PIPE_CONTROL command with Stall at
    *      Pixel Scoreboard set is required to be issued."
    */
   const uint32_t dw1 = GEN6_PIPE_CONTROL_PIXEL_SCOREBOARD_STALL;

   ILO_DEV_ASSERT(r->dev, 7, 7.5);

   if ((r->state.current_pipe_control_dw1 & dw1) != dw1)
      gen7_pipe_control(r, dw1);
}

static void
gen7_wa_post_ps_and_later(struct ilo_render *r)
{
   /*
    * From the Ivy Bridge PRM, volume 2 part 1, page 276:
    *
    *     "The driver must make sure a PIPE_CONTROL with the Depth Stall
    *      Enable bit set after all the following states are programmed:
    *
    *       - 3DSTATE_PS
    *       - 3DSTATE_VIEWPORT_STATE_POINTERS_CC
    *       - 3DSTATE_CONSTANT_PS
    *       - 3DSTATE_BINDING_TABLE_POINTERS_PS
    *       - 3DSTATE_SAMPLER_STATE_POINTERS_PS
    *       - 3DSTATE_CC_STATE_POINTERS
    *       - 3DSTATE_BLEND_STATE_POINTERS
    *       - 3DSTATE_DEPTH_STENCIL_STATE_POINTERS"
    */
   const uint32_t dw1 = GEN6_PIPE_CONTROL_DEPTH_STALL;

   ILO_DEV_ASSERT(r->dev, 7, 7.5);

   r->state.deferred_pipe_control_dw1 |= dw1;
}

#define DIRTY(state) (session->pipe_dirty & ILO_DIRTY_ ## state)

static void
gen7_draw_common_urb(struct ilo_render *r,
                     const struct ilo_state_vector *vec,
                     struct gen6_draw_session *session)
{
   /* 3DSTATE_URB_{VS,GS,HS,DS} */
   if (DIRTY(VE) || DIRTY(VS)) {
      /* the first 16KB are reserved for VS and PS PCBs */
      const int offset = (ilo_dev_gen(r->dev) == ILO_GEN(7.5) &&
            r->dev->gt == 3) ? 32768 : 16384;
      int vs_entry_size, vs_total_size;

      vs_entry_size = (vec->vs) ?
         ilo_shader_get_kernel_param(vec->vs, ILO_KERNEL_OUTPUT_COUNT) : 0;

      /*
       * From the Ivy Bridge PRM, volume 2 part 1, page 35:
       *
       *     "Programming Restriction: As the VS URB entry serves as both the
       *      per-vertex input and output of the VS shader, the VS URB
       *      Allocation Size must be sized to the maximum of the vertex input
       *      and output structures."
       */
      if (vs_entry_size < vec->ve->count)
         vs_entry_size = vec->ve->count;

      vs_entry_size *= sizeof(float) * 4;
      vs_total_size = r->dev->urb_size - offset;

      gen7_wa_pre_vs(r);

      gen7_3DSTATE_URB_VS(r->builder,
            offset, vs_total_size, vs_entry_size);

      gen7_3DSTATE_URB_GS(r->builder, offset, 0, 0);
      gen7_3DSTATE_URB_HS(r->builder, offset, 0, 0);
      gen7_3DSTATE_URB_DS(r->builder, offset, 0, 0);
   }
}

static void
gen7_draw_common_pcb_alloc(struct ilo_render *r,
                           const struct ilo_state_vector *vec,
                           struct gen6_draw_session *session)
{
   /* 3DSTATE_PUSH_CONSTANT_ALLOC_{VS,PS} */
   if (r->hw_ctx_changed) {
      /*
       * Push constant buffers are only allowed to take up at most the first
       * 16KB of the URB.  Split the space evenly for VS and FS.
       */
      const int max_size = (ilo_dev_gen(r->dev) == ILO_GEN(7.5) &&
            r->dev->gt == 3) ? 32768 : 16384;
      const int size = max_size / 2;
      int offset = 0;

      gen7_3DSTATE_PUSH_CONSTANT_ALLOC_VS(r->builder, offset, size);
      offset += size;

      gen7_3DSTATE_PUSH_CONSTANT_ALLOC_PS(r->builder, offset, size);

      if (ilo_dev_gen(r->dev) == ILO_GEN(7))
         gen7_wa_post_3dstate_push_constant_alloc_ps(r);
   }
}

static void
gen7_draw_common_pointers_1(struct ilo_render *r,
                            const struct ilo_state_vector *vec,
                            struct gen6_draw_session *session)
{
   /* 3DSTATE_VIEWPORT_STATE_POINTERS_{CC,SF_CLIP} */
   if (session->viewport_state_changed) {
      gen7_3DSTATE_VIEWPORT_STATE_POINTERS_CC(r->builder,
            r->state.CC_VIEWPORT);

      gen7_3DSTATE_VIEWPORT_STATE_POINTERS_SF_CLIP(r->builder,
            r->state.SF_CLIP_VIEWPORT);
   }
}

static void
gen7_draw_common_pointers_2(struct ilo_render *r,
                            const struct ilo_state_vector *vec,
                            struct gen6_draw_session *session)
{
   /* 3DSTATE_BLEND_STATE_POINTERS */
   if (session->cc_state_blend_changed) {
      gen7_3DSTATE_BLEND_STATE_POINTERS(r->builder,
            r->state.BLEND_STATE);
   }

   /* 3DSTATE_CC_STATE_POINTERS */
   if (session->cc_state_cc_changed) {
      gen7_3DSTATE_CC_STATE_POINTERS(r->builder,
            r->state.COLOR_CALC_STATE);
   }

   /* 3DSTATE_DEPTH_STENCIL_STATE_POINTERS */
   if (session->cc_state_dsa_changed) {
      gen7_3DSTATE_DEPTH_STENCIL_STATE_POINTERS(r->builder,
            r->state.DEPTH_STENCIL_STATE);
   }
}

static void
gen7_draw_vs(struct ilo_render *r,
             const struct ilo_state_vector *vec,
             struct gen6_draw_session *session)
{
   const bool emit_3dstate_binding_table = session->binding_table_vs_changed;
   const bool emit_3dstate_sampler_state = session->sampler_state_vs_changed;
   /* see gen6_draw_vs() */
   const bool emit_3dstate_constant_vs = session->pcb_state_vs_changed;
   const bool emit_3dstate_vs = (DIRTY(VS) || DIRTY(SAMPLER_VS) ||
           r->instruction_bo_changed);

   /* emit depth stall before any of the VS commands */
   if (emit_3dstate_binding_table || emit_3dstate_sampler_state ||
           emit_3dstate_constant_vs || emit_3dstate_vs)
      gen7_wa_pre_vs(r);

   /* 3DSTATE_BINDING_TABLE_POINTERS_VS */
   if (emit_3dstate_binding_table) {
      gen7_3DSTATE_BINDING_TABLE_POINTERS_VS(r->builder,
              r->state.vs.BINDING_TABLE_STATE);
   }

   /* 3DSTATE_SAMPLER_STATE_POINTERS_VS */
   if (emit_3dstate_sampler_state) {
      gen7_3DSTATE_SAMPLER_STATE_POINTERS_VS(r->builder,
              r->state.vs.SAMPLER_STATE);
   }

   /* 3DSTATE_CONSTANT_VS */
   if (emit_3dstate_constant_vs) {
      gen7_3DSTATE_CONSTANT_VS(r->builder,
              &r->state.vs.PUSH_CONSTANT_BUFFER,
              &r->state.vs.PUSH_CONSTANT_BUFFER_size,
              1);
   }

   /* 3DSTATE_VS */
   if (emit_3dstate_vs) {
      const int num_samplers = vec->sampler[PIPE_SHADER_VERTEX].count;

      gen6_3DSTATE_VS(r->builder, vec->vs, num_samplers);
   }
}

static void
gen7_draw_hs(struct ilo_render *r,
             const struct ilo_state_vector *vec,
             struct gen6_draw_session *session)
{
   /* 3DSTATE_CONSTANT_HS and 3DSTATE_HS */
   if (r->hw_ctx_changed) {
      gen7_3DSTATE_CONSTANT_HS(r->builder, 0, 0, 0);
      gen7_3DSTATE_HS(r->builder, NULL, 0);
   }

   /* 3DSTATE_BINDING_TABLE_POINTERS_HS */
   if (r->hw_ctx_changed)
      gen7_3DSTATE_BINDING_TABLE_POINTERS_HS(r->builder, 0);
}

static void
gen7_draw_te(struct ilo_render *r,
             const struct ilo_state_vector *vec,
             struct gen6_draw_session *session)
{
   /* 3DSTATE_TE */
   if (r->hw_ctx_changed)
      gen7_3DSTATE_TE(r->builder);
}

static void
gen7_draw_ds(struct ilo_render *r,
             const struct ilo_state_vector *vec,
             struct gen6_draw_session *session)
{
   /* 3DSTATE_CONSTANT_DS and 3DSTATE_DS */
   if (r->hw_ctx_changed) {
      gen7_3DSTATE_CONSTANT_DS(r->builder, 0, 0, 0);
      gen7_3DSTATE_DS(r->builder, NULL, 0);
   }

   /* 3DSTATE_BINDING_TABLE_POINTERS_DS */
   if (r->hw_ctx_changed)
      gen7_3DSTATE_BINDING_TABLE_POINTERS_DS(r->builder, 0);

}

static void
gen7_draw_gs(struct ilo_render *r,
             const struct ilo_state_vector *vec,
             struct gen6_draw_session *session)
{
   /* 3DSTATE_CONSTANT_GS and 3DSTATE_GS */
   if (r->hw_ctx_changed) {
      gen7_3DSTATE_CONSTANT_GS(r->builder, 0, 0, 0);
      gen7_3DSTATE_GS(r->builder, NULL, 0);
   }

   /* 3DSTATE_BINDING_TABLE_POINTERS_GS */
   if (session->binding_table_gs_changed) {
      gen7_3DSTATE_BINDING_TABLE_POINTERS_GS(r->builder,
            r->state.gs.BINDING_TABLE_STATE);
   }
}

static void
gen7_draw_sol(struct ilo_render *r,
              const struct ilo_state_vector *vec,
              struct gen6_draw_session *session)
{
   const struct pipe_stream_output_info *so_info;
   const struct ilo_shader_state *shader;
   bool dirty_sh = false;

   if (vec->gs) {
      shader = vec->gs;
      dirty_sh = DIRTY(GS);
   }
   else {
      shader = vec->vs;
      dirty_sh = DIRTY(VS);
   }

   so_info = ilo_shader_get_kernel_so_info(shader);

   /* 3DSTATE_SO_BUFFER */
   if ((DIRTY(SO) || dirty_sh || r->batch_bo_changed) &&
       vec->so.enabled) {
      int i;

      for (i = 0; i < vec->so.count; i++) {
         const int stride = so_info->stride[i] * 4; /* in bytes */
         int base = 0;

         gen7_3DSTATE_SO_BUFFER(r->builder, i, base, stride,
               vec->so.states[i]);
      }

      for (; i < 4; i++)
         gen7_3DSTATE_SO_BUFFER(r->builder, i, 0, 0, NULL);
   }

   /* 3DSTATE_SO_DECL_LIST */
   if (dirty_sh && vec->so.enabled)
      gen7_3DSTATE_SO_DECL_LIST(r->builder, so_info);

   /* 3DSTATE_STREAMOUT */
   if (DIRTY(SO) || DIRTY(RASTERIZER) || dirty_sh) {
      const unsigned buffer_mask = (1 << vec->so.count) - 1;
      const int output_count = ilo_shader_get_kernel_param(shader,
            ILO_KERNEL_OUTPUT_COUNT);

      gen7_3DSTATE_STREAMOUT(r->builder, buffer_mask, output_count,
            vec->rasterizer->state.rasterizer_discard);
   }
}

static void
gen7_draw_sf(struct ilo_render *r,
             const struct ilo_state_vector *vec,
             struct gen6_draw_session *session)
{
   /* 3DSTATE_SBE */
   if (DIRTY(RASTERIZER) || DIRTY(FS))
      gen7_3DSTATE_SBE(r->builder, vec->rasterizer, vec->fs);

   /* 3DSTATE_SF */
   if (DIRTY(RASTERIZER) || DIRTY(FB)) {
      struct pipe_surface *zs = vec->fb.state.zsbuf;

      gen7_wa_pre_3dstate_sf_depth_bias(r);
      gen7_3DSTATE_SF(r->builder, vec->rasterizer,
            (zs) ? zs->format : PIPE_FORMAT_NONE);
   }
}

static void
gen7_draw_wm(struct ilo_render *r,
             const struct ilo_state_vector *vec,
             struct gen6_draw_session *session)
{
   /* 3DSTATE_WM */
   if (DIRTY(FS) || DIRTY(BLEND) || DIRTY(DSA) || DIRTY(RASTERIZER)) {
      const bool cc_may_kill = (vec->dsa->dw_alpha ||
                                vec->blend->alpha_to_coverage);

      gen7_3DSTATE_WM(r->builder, vec->fs,
            vec->rasterizer, cc_may_kill, 0);
   }

   /* 3DSTATE_BINDING_TABLE_POINTERS_PS */
   if (session->binding_table_fs_changed) {
      gen7_3DSTATE_BINDING_TABLE_POINTERS_PS(r->builder,
            r->state.wm.BINDING_TABLE_STATE);
   }

   /* 3DSTATE_SAMPLER_STATE_POINTERS_PS */
   if (session->sampler_state_fs_changed) {
      gen7_3DSTATE_SAMPLER_STATE_POINTERS_PS(r->builder,
            r->state.wm.SAMPLER_STATE);
   }

   /* 3DSTATE_CONSTANT_PS */
   if (session->pcb_state_fs_changed) {
      gen7_3DSTATE_CONSTANT_PS(r->builder,
            &r->state.wm.PUSH_CONSTANT_BUFFER,
            &r->state.wm.PUSH_CONSTANT_BUFFER_size,
            1);
   }

   /* 3DSTATE_PS */
   if (DIRTY(FS) || DIRTY(SAMPLER_FS) || DIRTY(BLEND) ||
       r->instruction_bo_changed) {
      const int num_samplers = vec->sampler[PIPE_SHADER_FRAGMENT].count;
      const bool dual_blend = vec->blend->dual_blend;

      if ((ilo_dev_gen(r->dev) == ILO_GEN(7) ||
           ilo_dev_gen(r->dev) == ILO_GEN(7.5)) &&
          r->hw_ctx_changed)
         gen7_wa_pre_3dstate_ps_max_threads(r);

      gen7_3DSTATE_PS(r->builder, vec->fs, num_samplers, dual_blend);
   }

   /* 3DSTATE_SCISSOR_STATE_POINTERS */
   if (session->scissor_state_changed) {
      gen6_3DSTATE_SCISSOR_STATE_POINTERS(r->builder,
            r->state.SCISSOR_RECT);
   }

   /* XXX what is the best way to know if this workaround is needed? */
   {
      const bool emit_3dstate_ps =
         (DIRTY(FS) || DIRTY(SAMPLER_FS) || DIRTY(BLEND));
      const bool emit_3dstate_depth_buffer =
         (DIRTY(FB) || DIRTY(DSA) || r->state_bo_changed);

      if (emit_3dstate_ps ||
          session->pcb_state_fs_changed ||
          session->viewport_state_changed ||
          session->binding_table_fs_changed ||
          session->sampler_state_fs_changed ||
          session->cc_state_cc_changed ||
          session->cc_state_blend_changed ||
          session->cc_state_dsa_changed)
         gen7_wa_post_ps_and_later(r);

      if (emit_3dstate_depth_buffer)
         gen7_wa_pre_depth(r);
   }

   /* 3DSTATE_DEPTH_BUFFER and 3DSTATE_CLEAR_PARAMS */
   if (DIRTY(FB) || r->batch_bo_changed) {
      const struct ilo_zs_surface *zs;
      uint32_t clear_params;

      if (vec->fb.state.zsbuf) {
         const struct ilo_surface_cso *surface =
            (const struct ilo_surface_cso *) vec->fb.state.zsbuf;
         const struct ilo_texture_slice *slice =
            ilo_texture_get_slice(ilo_texture(surface->base.texture),
                  surface->base.u.tex.level, surface->base.u.tex.first_layer);

         assert(!surface->is_rt);
         zs = &surface->u.zs;
         clear_params = slice->clear_value;
      }
      else {
         zs = &vec->fb.null_zs;
         clear_params = 0;
      }

      gen6_3DSTATE_DEPTH_BUFFER(r->builder, zs);
      gen6_3DSTATE_HIER_DEPTH_BUFFER(r->builder, zs);
      gen6_3DSTATE_STENCIL_BUFFER(r->builder, zs);
      gen7_3DSTATE_CLEAR_PARAMS(r->builder, clear_params);
   }
}

static void
gen7_draw_wm_multisample(struct ilo_render *r,
                         const struct ilo_state_vector *vec,
                         struct gen6_draw_session *session)
{
   /* 3DSTATE_MULTISAMPLE and 3DSTATE_SAMPLE_MASK */
   if (DIRTY(SAMPLE_MASK) || DIRTY(FB)) {
      const uint32_t *packed_sample_pos;

      gen7_wa_pre_3dstate_multisample(r);

      packed_sample_pos =
         (vec->fb.num_samples > 4) ? r->packed_sample_position_8x :
         (vec->fb.num_samples > 1) ? &r->packed_sample_position_4x :
         &r->packed_sample_position_1x;

      gen6_3DSTATE_MULTISAMPLE(r->builder,
            vec->fb.num_samples, packed_sample_pos,
            vec->rasterizer->state.half_pixel_center);

      gen7_3DSTATE_SAMPLE_MASK(r->builder,
            (vec->fb.num_samples > 1) ? vec->sample_mask : 0x1,
            vec->fb.num_samples);
   }
}

static void
gen7_draw_vf_draw(struct ilo_render *r,
                  const struct ilo_state_vector *vec,
                  struct gen6_draw_session *session)
{
   if (r->state.deferred_pipe_control_dw1)
      gen7_pipe_control(r, r->state.deferred_pipe_control_dw1);

   /* 3DPRIMITIVE */
   gen7_3DPRIMITIVE(r->builder, vec->draw, &vec->ib);

   r->state.current_pipe_control_dw1 = 0;
   r->state.deferred_pipe_control_dw1 = 0;
}

static void
gen7_draw_commands(struct ilo_render *render,
                   const struct ilo_state_vector *vec,
                   struct gen6_draw_session *session)
{
   /*
    * We try to keep the order of the commands match, as closely as possible,
    * that of the classic i965 driver.  It allows us to compare the command
    * streams easily.
    */
   gen6_draw_common_select(render, vec, session);
   gen6_draw_common_sip(render, vec, session);
   gen6_draw_vf_statistics(render, vec, session);
   gen7_draw_common_pcb_alloc(render, vec, session);
   gen6_draw_common_base_address(render, vec, session);
   gen7_draw_common_pointers_1(render, vec, session);
   gen7_draw_common_urb(render, vec, session);
   gen7_draw_common_pointers_2(render, vec, session);
   gen7_draw_wm_multisample(render, vec, session);
   gen7_draw_gs(render, vec, session);
   gen7_draw_hs(render, vec, session);
   gen7_draw_te(render, vec, session);
   gen7_draw_ds(render, vec, session);
   gen7_draw_vs(render, vec, session);
   gen7_draw_sol(render, vec, session);
   gen6_draw_clip(render, vec, session);
   gen7_draw_sf(render, vec, session);
   gen7_draw_wm(render, vec, session);
   gen6_draw_wm_raster(render, vec, session);
   gen6_draw_sf_rect(render, vec, session);
   gen6_draw_vf(render, vec, session);
   gen7_draw_vf_draw(render, vec, session);
}

static void
ilo_render_emit_draw_gen7(struct ilo_render *render,
                          const struct ilo_state_vector *vec)
{
   struct gen6_draw_session session;

   gen6_draw_prepare(render, vec, &session);

   session.emit_draw_states = gen6_draw_states;
   session.emit_draw_commands = gen7_draw_commands;

   gen6_draw_emit(render, vec, &session);
   gen6_draw_end(render, vec, &session);
}

static void
gen7_rectlist_pcb_alloc(struct ilo_render *r,
                        const struct ilo_blitter *blitter,
                        struct gen6_rectlist_session *session)
{
   /*
    * Push constant buffers are only allowed to take up at most the first
    * 16KB of the URB.  Split the space evenly for VS and FS.
    */
   const int max_size =
      (ilo_dev_gen(r->dev) == ILO_GEN(7.5) && r->dev->gt == 3) ? 32768 : 16384;
   const int size = max_size / 2;
   int offset = 0;

   gen7_3DSTATE_PUSH_CONSTANT_ALLOC_VS(r->builder, offset, size);
   offset += size;

   gen7_3DSTATE_PUSH_CONSTANT_ALLOC_PS(r->builder, offset, size);

   gen7_wa_post_3dstate_push_constant_alloc_ps(r);
}

static void
gen7_rectlist_urb(struct ilo_render *r,
                  const struct ilo_blitter *blitter,
                  struct gen6_rectlist_session *session)
{
   /* the first 16KB are reserved for VS and PS PCBs */
   const int offset =
      (ilo_dev_gen(r->dev) == ILO_GEN(7.5) && r->dev->gt == 3) ? 32768 : 16384;

   gen7_3DSTATE_URB_VS(r->builder, offset, r->dev->urb_size - offset,
         blitter->ve.count * 4 * sizeof(float));

   gen7_3DSTATE_URB_GS(r->builder, offset, 0, 0);
   gen7_3DSTATE_URB_HS(r->builder, offset, 0, 0);
   gen7_3DSTATE_URB_DS(r->builder, offset, 0, 0);
}

static void
gen7_rectlist_vs_to_sf(struct ilo_render *r,
                       const struct ilo_blitter *blitter,
                       struct gen6_rectlist_session *session)
{
   gen7_3DSTATE_CONSTANT_VS(r->builder, NULL, NULL, 0);
   gen6_3DSTATE_VS(r->builder, NULL, 0);

   gen7_3DSTATE_CONSTANT_HS(r->builder, NULL, NULL, 0);
   gen7_3DSTATE_HS(r->builder, NULL, 0);

   gen7_3DSTATE_TE(r->builder);

   gen7_3DSTATE_CONSTANT_DS(r->builder, NULL, NULL, 0);
   gen7_3DSTATE_DS(r->builder, NULL, 0);

   gen7_3DSTATE_CONSTANT_GS(r->builder, NULL, NULL, 0);
   gen7_3DSTATE_GS(r->builder, NULL, 0);

   gen7_3DSTATE_STREAMOUT(r->builder, 0x0, 0, false);

   gen6_3DSTATE_CLIP(r->builder, NULL, NULL, false, 0);

   gen7_wa_pre_3dstate_sf_depth_bias(r);

   gen7_3DSTATE_SF(r->builder, NULL, blitter->fb.dst.base.format);
   gen7_3DSTATE_SBE(r->builder, NULL, NULL);
}

static void
gen7_rectlist_wm(struct ilo_render *r,
                 const struct ilo_blitter *blitter,
                 struct gen6_rectlist_session *session)
{
   uint32_t hiz_op;

   switch (blitter->op) {
   case ILO_BLITTER_RECTLIST_CLEAR_ZS:
      hiz_op = GEN7_WM_DW1_DEPTH_CLEAR;
      break;
   case ILO_BLITTER_RECTLIST_RESOLVE_Z:
      hiz_op = GEN7_WM_DW1_DEPTH_RESOLVE;
      break;
   case ILO_BLITTER_RECTLIST_RESOLVE_HIZ:
      hiz_op = GEN7_WM_DW1_HIZ_RESOLVE;
      break;
   default:
      hiz_op = 0;
      break;
   }

   gen7_3DSTATE_WM(r->builder, NULL, NULL, false, hiz_op);

   gen7_3DSTATE_CONSTANT_PS(r->builder, NULL, NULL, 0);

   gen7_wa_pre_3dstate_ps_max_threads(r);
   gen7_3DSTATE_PS(r->builder, NULL, 0, false);
}

static void
gen7_rectlist_wm_depth(struct ilo_render *r,
                       const struct ilo_blitter *blitter,
                       struct gen6_rectlist_session *session)
{
   gen7_wa_pre_depth(r);

   if (blitter->uses & (ILO_BLITTER_USE_FB_DEPTH |
                        ILO_BLITTER_USE_FB_STENCIL)) {
      gen6_3DSTATE_DEPTH_BUFFER(r->builder,
            &blitter->fb.dst.u.zs);
   }

   if (blitter->uses & ILO_BLITTER_USE_FB_DEPTH) {
      gen6_3DSTATE_HIER_DEPTH_BUFFER(r->builder,
            &blitter->fb.dst.u.zs);
   }

   if (blitter->uses & ILO_BLITTER_USE_FB_STENCIL) {
      gen6_3DSTATE_STENCIL_BUFFER(r->builder,
            &blitter->fb.dst.u.zs);
   }

   gen7_3DSTATE_CLEAR_PARAMS(r->builder,
         blitter->depth_clear_value);
}

static void
gen7_rectlist_wm_multisample(struct ilo_render *r,
                             const struct ilo_blitter *blitter,
                             struct gen6_rectlist_session *session)
{
   const uint32_t *packed_sample_pos =
      (blitter->fb.num_samples > 4) ? r->packed_sample_position_8x :
      (blitter->fb.num_samples > 1) ? &r->packed_sample_position_4x :
      &r->packed_sample_position_1x;

   gen7_wa_pre_3dstate_multisample(r);

   gen6_3DSTATE_MULTISAMPLE(r->builder, blitter->fb.num_samples,
         packed_sample_pos, true);

   gen7_3DSTATE_SAMPLE_MASK(r->builder,
         (1 << blitter->fb.num_samples) - 1, blitter->fb.num_samples);
}

static void
gen7_rectlist_commands(struct ilo_render *r,
                       const struct ilo_blitter *blitter,
                       struct gen6_rectlist_session *session)
{
   gen7_rectlist_wm_multisample(r, blitter, session);

   gen6_state_base_address(r->builder, true);

   gen6_3DSTATE_VERTEX_BUFFERS(r->builder,
         &blitter->ve, &blitter->vb);

   gen6_3DSTATE_VERTEX_ELEMENTS(r->builder,
         &blitter->ve, false, false);

   gen7_rectlist_pcb_alloc(r, blitter, session);

   /* needed for any VS-related commands */
   gen7_wa_pre_vs(r);

   gen7_rectlist_urb(r, blitter, session);

   if (blitter->uses & ILO_BLITTER_USE_DSA) {
      gen7_3DSTATE_DEPTH_STENCIL_STATE_POINTERS(r->builder,
            session->DEPTH_STENCIL_STATE);
   }

   if (blitter->uses & ILO_BLITTER_USE_CC) {
      gen7_3DSTATE_CC_STATE_POINTERS(r->builder,
            session->COLOR_CALC_STATE);
   }

   gen7_rectlist_vs_to_sf(r, blitter, session);
   gen7_rectlist_wm(r, blitter, session);

   if (blitter->uses & ILO_BLITTER_USE_VIEWPORT) {
      gen7_3DSTATE_VIEWPORT_STATE_POINTERS_CC(r->builder,
            session->CC_VIEWPORT);
   }

   gen7_rectlist_wm_depth(r, blitter, session);

   gen6_3DSTATE_DRAWING_RECTANGLE(r->builder, 0, 0,
         blitter->fb.width, blitter->fb.height);

   gen7_3DPRIMITIVE(r->builder, &blitter->draw, NULL);
}

static void
gen7_rectlist_states(struct ilo_render *r,
                     const struct ilo_blitter *blitter,
                     struct gen6_rectlist_session *session)
{
   if (blitter->uses & ILO_BLITTER_USE_DSA) {
      session->DEPTH_STENCIL_STATE =
         gen6_DEPTH_STENCIL_STATE(r->builder, &blitter->dsa);
   }

   if (blitter->uses & ILO_BLITTER_USE_CC) {
      session->COLOR_CALC_STATE =
         gen6_COLOR_CALC_STATE(r->builder, &blitter->cc.stencil_ref,
               blitter->cc.alpha_ref, &blitter->cc.blend_color);
   }

   if (blitter->uses & ILO_BLITTER_USE_VIEWPORT) {
      session->CC_VIEWPORT =
         gen6_CC_VIEWPORT(r->builder, &blitter->viewport, 1);
   }
}

static void
ilo_render_emit_rectlist_gen7(struct ilo_render *render,
                              const struct ilo_blitter *blitter)
{
   struct gen6_rectlist_session session;

   memset(&session, 0, sizeof(session));
   gen7_rectlist_states(render, blitter, &session);
   gen7_rectlist_commands(render, blitter, &session);
}

static int
gen7_render_max_command_size(const struct ilo_render *render)
{
   static int size;

   if (!size) {
      size += GEN7_3DSTATE_URB_ANY__SIZE * 4;
      size += GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_ANY__SIZE * 5;
      size += GEN6_3DSTATE_CONSTANT_ANY__SIZE * 5;
      size += GEN7_3DSTATE_POINTERS_ANY__SIZE * (5 + 5 + 4);
      size += GEN7_3DSTATE_SO_BUFFER__SIZE * 4;
      size += GEN6_PIPE_CONTROL__SIZE * 5;

      size +=
         GEN6_STATE_BASE_ADDRESS__SIZE +
         GEN6_STATE_SIP__SIZE +
         GEN6_3DSTATE_VF_STATISTICS__SIZE +
         GEN6_PIPELINE_SELECT__SIZE +
         GEN6_3DSTATE_CLEAR_PARAMS__SIZE +
         GEN6_3DSTATE_DEPTH_BUFFER__SIZE +
         GEN6_3DSTATE_STENCIL_BUFFER__SIZE +
         GEN6_3DSTATE_HIER_DEPTH_BUFFER__SIZE +
         GEN6_3DSTATE_VERTEX_BUFFERS__SIZE +
         GEN6_3DSTATE_VERTEX_ELEMENTS__SIZE +
         GEN6_3DSTATE_INDEX_BUFFER__SIZE +
         GEN75_3DSTATE_VF__SIZE +
         GEN6_3DSTATE_VS__SIZE +
         GEN6_3DSTATE_GS__SIZE +
         GEN6_3DSTATE_CLIP__SIZE +
         GEN6_3DSTATE_SF__SIZE +
         GEN6_3DSTATE_WM__SIZE +
         GEN6_3DSTATE_SAMPLE_MASK__SIZE +
         GEN7_3DSTATE_HS__SIZE +
         GEN7_3DSTATE_TE__SIZE +
         GEN7_3DSTATE_DS__SIZE +
         GEN7_3DSTATE_STREAMOUT__SIZE +
         GEN7_3DSTATE_SBE__SIZE +
         GEN7_3DSTATE_PS__SIZE +
         GEN6_3DSTATE_DRAWING_RECTANGLE__SIZE +
         GEN6_3DSTATE_POLY_STIPPLE_OFFSET__SIZE +
         GEN6_3DSTATE_POLY_STIPPLE_PATTERN__SIZE +
         GEN6_3DSTATE_LINE_STIPPLE__SIZE +
         GEN6_3DSTATE_AA_LINE_PARAMETERS__SIZE +
         GEN6_3DSTATE_MULTISAMPLE__SIZE +
         GEN7_3DSTATE_SO_DECL_LIST__SIZE +
         GEN6_3DPRIMITIVE__SIZE;
   }

   return size;
}

static int
ilo_render_estimate_size_gen7(struct ilo_render *render,
                              enum ilo_render_action action,
                              const void *arg)
{
   int size;

   switch (action) {
   case ILO_RENDER_DRAW:
      {
         const struct ilo_state_vector *ilo = arg;

         size = gen7_render_max_command_size(render) +
            gen6_render_estimate_state_size(render, ilo);
      }
      break;
   case ILO_RENDER_FLUSH:
      size = GEN6_PIPE_CONTROL__SIZE;
      break;
   case ILO_RENDER_QUERY:
      size = gen6_render_estimate_query_size(render,
            (const struct ilo_query *) arg);
      break;
   case ILO_RENDER_RECTLIST:
      size = 64 + 256; /* states + commands */
      break;
   default:
      assert(!"unknown render action");
      size = 0;
      break;
   }

   return size;
}

void
ilo_render_init_gen7(struct ilo_render *render)
{
   render->estimate_size = ilo_render_estimate_size_gen7;
   render->emit_draw = ilo_render_emit_draw_gen7;
   render->emit_flush = ilo_render_emit_flush_gen6;
   render->emit_query = ilo_render_emit_query_gen6;
   render->emit_rectlist = ilo_render_emit_rectlist_gen7;
}

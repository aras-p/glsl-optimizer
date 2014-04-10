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
#include "ilo_context.h"
#include "ilo_cp.h"
#include "ilo_gpe_gen7.h"
#include "ilo_shader.h"
#include "ilo_state.h"
#include "ilo_3d_pipeline.h"
#include "ilo_3d_pipeline_gen6.h"
#include "ilo_3d_pipeline_gen7.h"

static void
gen7_wa_pipe_control_cs_stall(struct ilo_3d_pipeline *p,
                              bool change_multisample_state,
                              bool change_depth_state)
{
   struct intel_bo *bo = NULL;
   uint32_t dw1 = GEN6_PIPE_CONTROL_CS_STALL;

   assert(p->dev->gen == ILO_GEN(7) || p->dev->gen == ILO_GEN(7.5));

   /* emit once */
   if (p->state.has_gen6_wa_pipe_control)
      return;
   p->state.has_gen6_wa_pipe_control = true;

   /*
    * From the Ivy Bridge PRM, volume 2 part 1, page 258:
    *
    *     "Due to an HW issue driver needs to send a pipe control with stall
    *      when ever there is state change in depth bias related state"
    *
    * From the Ivy Bridge PRM, volume 2 part 1, page 292:
    *
    *     "A PIPE_CONTOL command with the CS Stall bit set must be programmed
    *      in the ring after this instruction
    *      (3DSTATE_PUSH_CONSTANT_ALLOC_PS)."
    *
    * From the Ivy Bridge PRM, volume 2 part 1, page 304:
    *
    *     "Driver must ierarchi that all the caches in the depth pipe are
    *      flushed before this command (3DSTATE_MULTISAMPLE) is parsed. This
    *      requires driver to send a PIPE_CONTROL with a CS stall along with a
    *      Depth Flush prior to this command.
    *
    * From the Ivy Bridge PRM, volume 2 part 1, page 315:
    *
    *     "Driver must send a least one PIPE_CONTROL command with CS Stall and
    *      a post sync operation prior to the group of depth
    *      commands(3DSTATE_DEPTH_BUFFER, 3DSTATE_CLEAR_PARAMS,
    *      3DSTATE_STENCIL_BUFFER, and 3DSTATE_HIER_DEPTH_BUFFER)."
    */

   if (change_multisample_state)
      dw1 |= GEN6_PIPE_CONTROL_DEPTH_CACHE_FLUSH;

   if (change_depth_state) {
      dw1 |= GEN6_PIPE_CONTROL_WRITE_IMM;
      bo = p->workaround_bo;
   }

   gen6_emit_PIPE_CONTROL(p->dev, dw1, bo, 0, false, p->cp);
}

static void
gen7_wa_pipe_control_vs_depth_stall(struct ilo_3d_pipeline *p)
{
   assert(p->dev->gen == ILO_GEN(7) || p->dev->gen == ILO_GEN(7.5));

   /*
    * From the Ivy Bridge PRM, volume 2 part 1, page 106:
    *
    *     "A PIPE_CONTROL with Post-Sync Operation set to 1h and a depth stall
    *      needs to be sent just prior to any 3DSTATE_VS, 3DSTATE_URB_VS,
    *      3DSTATE_CONSTANT_VS, 3DSTATE_BINDING_TABLE_POINTER_VS,
    *      3DSTATE_SAMPLER_STATE_POINTER_VS command.  Only one PIPE_CONTROL
    *      needs to be sent before any combination of VS associated 3DSTATE."
    */
   gen6_emit_PIPE_CONTROL(p->dev,
         GEN6_PIPE_CONTROL_DEPTH_STALL |
         GEN6_PIPE_CONTROL_WRITE_IMM,
         p->workaround_bo, 0, false, p->cp);
}

static void
gen7_wa_pipe_control_wm_depth_stall(struct ilo_3d_pipeline *p,
                                    bool change_depth_buffer)
{
   assert(p->dev->gen == ILO_GEN(7) || p->dev->gen == ILO_GEN(7.5));

   /*
    * From the Ivy Bridge PRM, volume 2 part 1, page 276:
    *
    *     "The driver must make sure a PIPE_CONTROL with the Depth Stall
    *      Enable bit set after all the following states are programmed:
    *
    *       * 3DSTATE_PS
    *       * 3DSTATE_VIEWPORT_STATE_POINTERS_CC
    *       * 3DSTATE_CONSTANT_PS
    *       * 3DSTATE_BINDING_TABLE_POINTERS_PS
    *       * 3DSTATE_SAMPLER_STATE_POINTERS_PS
    *       * 3DSTATE_CC_STATE_POINTERS
    *       * 3DSTATE_BLEND_STATE_POINTERS
    *       * 3DSTATE_DEPTH_STENCIL_STATE_POINTERS"
    *
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
   gen6_emit_PIPE_CONTROL(p->dev,
         GEN6_PIPE_CONTROL_DEPTH_STALL,
         NULL, 0, false, p->cp);

   if (!change_depth_buffer)
      return;

   gen6_emit_PIPE_CONTROL(p->dev,
         GEN6_PIPE_CONTROL_DEPTH_CACHE_FLUSH,
         NULL, 0, false, p->cp);

   gen6_emit_PIPE_CONTROL(p->dev,
         GEN6_PIPE_CONTROL_DEPTH_STALL,
         NULL, 0, false, p->cp);
}

static void
gen7_wa_pipe_control_ps_max_threads_stall(struct ilo_3d_pipeline *p)
{
   assert(p->dev->gen == ILO_GEN(7) || p->dev->gen == ILO_GEN(7.5));

   /*
    * From the Ivy Bridge PRM, volume 2 part 1, page 286:
    *
    *     "If this field (Maximum Number of Threads in 3DSTATE_PS) is changed
    *      between 3DPRIMITIVE commands, a PIPE_CONTROL command with Stall at
    *      Pixel Scoreboard set is required to be issued."
    */
   gen6_emit_PIPE_CONTROL(p->dev,
         GEN6_PIPE_CONTROL_PIXEL_SCOREBOARD_STALL,
         NULL, 0, false, p->cp);

}

#define DIRTY(state) (session->pipe_dirty & ILO_DIRTY_ ## state)

static void
gen7_pipeline_common_urb(struct ilo_3d_pipeline *p,
                         const struct ilo_context *ilo,
                         struct gen6_pipeline_session *session)
{
   /* 3DSTATE_URB_{VS,GS,HS,DS} */
   if (DIRTY(VE) || DIRTY(VS)) {
      /* the first 16KB are reserved for VS and PS PCBs */
      const int offset =
         (p->dev->gen == ILO_GEN(7.5) && p->dev->gt == 3) ? 32768 : 16384;
      int vs_entry_size, vs_total_size;

      vs_entry_size = (ilo->vs) ?
         ilo_shader_get_kernel_param(ilo->vs, ILO_KERNEL_OUTPUT_COUNT) : 0;

      /*
       * From the Ivy Bridge PRM, volume 2 part 1, page 35:
       *
       *     "Programming Restriction: As the VS URB entry serves as both the
       *      per-vertex input and output of the VS shader, the VS URB
       *      Allocation Size must be sized to the maximum of the vertex input
       *      and output structures."
       */
      if (vs_entry_size < ilo->ve->count)
         vs_entry_size = ilo->ve->count;

      vs_entry_size *= sizeof(float) * 4;
      vs_total_size = ilo->dev->urb_size - offset;

      gen7_wa_pipe_control_vs_depth_stall(p);

      gen7_emit_3DSTATE_URB_VS(p->dev,
            offset, vs_total_size, vs_entry_size, p->cp);

      gen7_emit_3DSTATE_URB_GS(p->dev, offset, 0, 0, p->cp);
      gen7_emit_3DSTATE_URB_HS(p->dev, offset, 0, 0, p->cp);
      gen7_emit_3DSTATE_URB_DS(p->dev, offset, 0, 0, p->cp);
   }
}

static void
gen7_pipeline_common_pcb_alloc(struct ilo_3d_pipeline *p,
                               const struct ilo_context *ilo,
                               struct gen6_pipeline_session *session)
{
   /* 3DSTATE_PUSH_CONSTANT_ALLOC_{VS,PS} */
   if (session->hw_ctx_changed) {
      /*
       * Push constant buffers are only allowed to take up at most the first
       * 16KB of the URB.  Split the space evenly for VS and FS.
       */
      const int max_size =
         (p->dev->gen == ILO_GEN(7.5) && p->dev->gt == 3) ? 32768 : 16384;
      const int size = max_size / 2;
      int offset = 0;

      gen7_emit_3DSTATE_PUSH_CONSTANT_ALLOC_VS(p->dev, offset, size, p->cp);
      offset += size;

      gen7_emit_3DSTATE_PUSH_CONSTANT_ALLOC_PS(p->dev, offset, size, p->cp);

      if (p->dev->gen == ILO_GEN(7))
         gen7_wa_pipe_control_cs_stall(p, true, true);
   }
}

static void
gen7_pipeline_common_pointers_1(struct ilo_3d_pipeline *p,
                                const struct ilo_context *ilo,
                                struct gen6_pipeline_session *session)
{
   /* 3DSTATE_VIEWPORT_STATE_POINTERS_{CC,SF_CLIP} */
   if (session->viewport_state_changed) {
      gen7_emit_3DSTATE_VIEWPORT_STATE_POINTERS_CC(p->dev,
            p->state.CC_VIEWPORT, p->cp);

      gen7_emit_3DSTATE_VIEWPORT_STATE_POINTERS_SF_CLIP(p->dev,
            p->state.SF_CLIP_VIEWPORT, p->cp);
   }
}

static void
gen7_pipeline_common_pointers_2(struct ilo_3d_pipeline *p,
                                const struct ilo_context *ilo,
                                struct gen6_pipeline_session *session)
{
   /* 3DSTATE_BLEND_STATE_POINTERS */
   if (session->cc_state_blend_changed) {
      gen7_emit_3DSTATE_BLEND_STATE_POINTERS(p->dev,
            p->state.BLEND_STATE, p->cp);
   }

   /* 3DSTATE_CC_STATE_POINTERS */
   if (session->cc_state_cc_changed) {
      gen7_emit_3DSTATE_CC_STATE_POINTERS(p->dev,
            p->state.COLOR_CALC_STATE, p->cp);
   }

   /* 3DSTATE_DEPTH_STENCIL_STATE_POINTERS */
   if (session->cc_state_dsa_changed) {
      gen7_emit_3DSTATE_DEPTH_STENCIL_STATE_POINTERS(p->dev,
            p->state.DEPTH_STENCIL_STATE, p->cp);
   }
}

static void
gen7_pipeline_vs(struct ilo_3d_pipeline *p,
                 const struct ilo_context *ilo,
                 struct gen6_pipeline_session *session)
{
   const bool emit_3dstate_binding_table = session->binding_table_vs_changed;
   const bool emit_3dstate_sampler_state = session->sampler_state_vs_changed;
   /* see gen6_pipeline_vs() */
   const bool emit_3dstate_constant_vs = session->pcb_state_vs_changed;
   const bool emit_3dstate_vs = (DIRTY(VS) || DIRTY(SAMPLER_VS) ||
           session->kernel_bo_changed);

   /* emit depth stall before any of the VS commands */
   if (emit_3dstate_binding_table || emit_3dstate_sampler_state ||
           emit_3dstate_constant_vs || emit_3dstate_vs)
      gen7_wa_pipe_control_vs_depth_stall(p);

   /* 3DSTATE_BINDING_TABLE_POINTERS_VS */
   if (emit_3dstate_binding_table) {
      gen7_emit_3DSTATE_BINDING_TABLE_POINTERS_VS(p->dev,
              p->state.vs.BINDING_TABLE_STATE, p->cp);
   }

   /* 3DSTATE_SAMPLER_STATE_POINTERS_VS */
   if (emit_3dstate_sampler_state) {
      gen7_emit_3DSTATE_SAMPLER_STATE_POINTERS_VS(p->dev,
              p->state.vs.SAMPLER_STATE, p->cp);
   }

   /* 3DSTATE_CONSTANT_VS */
   if (emit_3dstate_constant_vs) {
      gen7_emit_3DSTATE_CONSTANT_VS(p->dev,
              &p->state.vs.PUSH_CONSTANT_BUFFER,
              &p->state.vs.PUSH_CONSTANT_BUFFER_size,
              1, p->cp);
   }

   /* 3DSTATE_VS */
   if (emit_3dstate_vs) {
      const int num_samplers = ilo->sampler[PIPE_SHADER_VERTEX].count;

      gen6_emit_3DSTATE_VS(p->dev, ilo->vs, num_samplers, p->cp);
   }
}

static void
gen7_pipeline_hs(struct ilo_3d_pipeline *p,
                 const struct ilo_context *ilo,
                 struct gen6_pipeline_session *session)
{
   /* 3DSTATE_CONSTANT_HS and 3DSTATE_HS */
   if (session->hw_ctx_changed) {
      gen7_emit_3DSTATE_CONSTANT_HS(p->dev, 0, 0, 0, p->cp);
      gen7_emit_3DSTATE_HS(p->dev, NULL, 0, p->cp);
   }

   /* 3DSTATE_BINDING_TABLE_POINTERS_HS */
   if (session->hw_ctx_changed)
      gen7_emit_3DSTATE_BINDING_TABLE_POINTERS_HS(p->dev, 0, p->cp);
}

static void
gen7_pipeline_te(struct ilo_3d_pipeline *p,
                 const struct ilo_context *ilo,
                 struct gen6_pipeline_session *session)
{
   /* 3DSTATE_TE */
   if (session->hw_ctx_changed)
      gen7_emit_3DSTATE_TE(p->dev, p->cp);
}

static void
gen7_pipeline_ds(struct ilo_3d_pipeline *p,
                 const struct ilo_context *ilo,
                 struct gen6_pipeline_session *session)
{
   /* 3DSTATE_CONSTANT_DS and 3DSTATE_DS */
   if (session->hw_ctx_changed) {
      gen7_emit_3DSTATE_CONSTANT_DS(p->dev, 0, 0, 0, p->cp);
      gen7_emit_3DSTATE_DS(p->dev, NULL, 0, p->cp);
   }

   /* 3DSTATE_BINDING_TABLE_POINTERS_DS */
   if (session->hw_ctx_changed)
      gen7_emit_3DSTATE_BINDING_TABLE_POINTERS_DS(p->dev, 0, p->cp);

}

static void
gen7_pipeline_gs(struct ilo_3d_pipeline *p,
                 const struct ilo_context *ilo,
                 struct gen6_pipeline_session *session)
{
   /* 3DSTATE_CONSTANT_GS and 3DSTATE_GS */
   if (session->hw_ctx_changed) {
      gen7_emit_3DSTATE_CONSTANT_GS(p->dev, 0, 0, 0, p->cp);
      gen7_emit_3DSTATE_GS(p->dev, NULL, 0, p->cp);
   }

   /* 3DSTATE_BINDING_TABLE_POINTERS_GS */
   if (session->binding_table_gs_changed) {
      gen7_emit_3DSTATE_BINDING_TABLE_POINTERS_GS(p->dev,
            p->state.gs.BINDING_TABLE_STATE, p->cp);
   }
}

static void
gen7_pipeline_sol(struct ilo_3d_pipeline *p,
                  const struct ilo_context *ilo,
                  struct gen6_pipeline_session *session)
{
   const struct pipe_stream_output_info *so_info;
   const struct ilo_shader_state *shader;
   bool dirty_sh = false;

   if (ilo->gs) {
      shader = ilo->gs;
      dirty_sh = DIRTY(GS);
   }
   else {
      shader = ilo->vs;
      dirty_sh = DIRTY(VS);
   }

   so_info = ilo_shader_get_kernel_so_info(shader);

   gen6_pipeline_update_max_svbi(p, ilo, session);

   /* 3DSTATE_SO_BUFFER */
   if ((DIRTY(SO) || dirty_sh || session->batch_bo_changed) &&
       ilo->so.enabled) {
      int i;

      for (i = 0; i < ilo->so.count; i++) {
         const int stride = so_info->stride[i] * 4; /* in bytes */
         int base = 0;

         gen7_emit_3DSTATE_SO_BUFFER(p->dev, i, base, stride,
               ilo->so.states[i], p->cp);
      }

      for (; i < 4; i++)
         gen7_emit_3DSTATE_SO_BUFFER(p->dev, i, 0, 0, NULL, p->cp);
   }

   /* 3DSTATE_SO_DECL_LIST */
   if (dirty_sh && ilo->so.enabled)
      gen7_emit_3DSTATE_SO_DECL_LIST(p->dev, so_info, p->cp);

   /* 3DSTATE_STREAMOUT */
   if (DIRTY(SO) || DIRTY(RASTERIZER) || dirty_sh) {
      const unsigned buffer_mask = (1 << ilo->so.count) - 1;
      const int output_count = ilo_shader_get_kernel_param(shader,
            ILO_KERNEL_OUTPUT_COUNT);

      gen7_emit_3DSTATE_STREAMOUT(p->dev, buffer_mask, output_count,
            ilo->rasterizer->state.rasterizer_discard, p->cp);
   }
}

static void
gen7_pipeline_sf(struct ilo_3d_pipeline *p,
                 const struct ilo_context *ilo,
                 struct gen6_pipeline_session *session)
{
   /* 3DSTATE_SBE */
   if (DIRTY(RASTERIZER) || DIRTY(FS))
      gen7_emit_3DSTATE_SBE(p->dev, ilo->rasterizer, ilo->fs, ilo->cp);

   /* 3DSTATE_SF */
   if (DIRTY(RASTERIZER) || DIRTY(FB)) {
      struct pipe_surface *zs = ilo->fb.state.zsbuf;

      gen7_wa_pipe_control_cs_stall(p, true, true);
      gen7_emit_3DSTATE_SF(p->dev, ilo->rasterizer,
            (zs) ? zs->format : PIPE_FORMAT_NONE, p->cp);
   }
}

static void
gen7_pipeline_wm(struct ilo_3d_pipeline *p,
                 const struct ilo_context *ilo,
                 struct gen6_pipeline_session *session)
{
   /* 3DSTATE_WM */
   if (DIRTY(FS) || DIRTY(BLEND) || DIRTY(DSA) || DIRTY(RASTERIZER)) {
      const bool cc_may_kill = (ilo->dsa->dw_alpha ||
                                ilo->blend->alpha_to_coverage);

      gen7_emit_3DSTATE_WM(p->dev, ilo->fs,
            ilo->rasterizer, cc_may_kill, 0, p->cp);
   }

   /* 3DSTATE_BINDING_TABLE_POINTERS_PS */
   if (session->binding_table_fs_changed) {
      gen7_emit_3DSTATE_BINDING_TABLE_POINTERS_PS(p->dev,
            p->state.wm.BINDING_TABLE_STATE, p->cp);
   }

   /* 3DSTATE_SAMPLER_STATE_POINTERS_PS */
   if (session->sampler_state_fs_changed) {
      gen7_emit_3DSTATE_SAMPLER_STATE_POINTERS_PS(p->dev,
            p->state.wm.SAMPLER_STATE, p->cp);
   }

   /* 3DSTATE_CONSTANT_PS */
   if (session->pcb_state_fs_changed) {
      gen7_emit_3DSTATE_CONSTANT_PS(p->dev,
            &p->state.wm.PUSH_CONSTANT_BUFFER,
            &p->state.wm.PUSH_CONSTANT_BUFFER_size,
            1, p->cp);
   }

   /* 3DSTATE_PS */
   if (DIRTY(FS) || DIRTY(SAMPLER_FS) || DIRTY(BLEND) ||
       session->kernel_bo_changed) {
      const int num_samplers = ilo->sampler[PIPE_SHADER_FRAGMENT].count;
      const bool dual_blend = ilo->blend->dual_blend;

      if ((p->dev->gen == ILO_GEN(7) || p->dev->gen == ILO_GEN(7.5)) &&
          session->hw_ctx_changed)
         gen7_wa_pipe_control_ps_max_threads_stall(p);

      gen7_emit_3DSTATE_PS(p->dev, ilo->fs, num_samplers, dual_blend, p->cp);
   }

   /* 3DSTATE_SCISSOR_STATE_POINTERS */
   if (session->scissor_state_changed) {
      gen6_emit_3DSTATE_SCISSOR_STATE_POINTERS(p->dev,
            p->state.SCISSOR_RECT, p->cp);
   }

   /* XXX what is the best way to know if this workaround is needed? */
   {
      const bool emit_3dstate_ps =
         (DIRTY(FS) || DIRTY(SAMPLER_FS) || DIRTY(BLEND));
      const bool emit_3dstate_depth_buffer =
         (DIRTY(FB) || DIRTY(DSA) || session->state_bo_changed);

      if (emit_3dstate_ps ||
          emit_3dstate_depth_buffer ||
          session->pcb_state_fs_changed ||
          session->viewport_state_changed ||
          session->binding_table_fs_changed ||
          session->sampler_state_fs_changed ||
          session->cc_state_cc_changed ||
          session->cc_state_blend_changed ||
          session->cc_state_dsa_changed)
         gen7_wa_pipe_control_wm_depth_stall(p, emit_3dstate_depth_buffer);
   }

   /* 3DSTATE_DEPTH_BUFFER and 3DSTATE_CLEAR_PARAMS */
   if (DIRTY(FB) || session->batch_bo_changed) {
      const struct ilo_zs_surface *zs;
      uint32_t clear_params;

      if (ilo->fb.state.zsbuf) {
         const struct ilo_surface_cso *surface =
            (const struct ilo_surface_cso *) ilo->fb.state.zsbuf;
         const struct ilo_texture_slice *slice =
            ilo_texture_get_slice(ilo_texture(surface->base.texture),
                  surface->base.u.tex.level, surface->base.u.tex.first_layer);

         assert(!surface->is_rt);
         zs = &surface->u.zs;
         clear_params = slice->clear_value;
      }
      else {
         zs = &ilo->fb.null_zs;
         clear_params = 0;
      }

      gen6_emit_3DSTATE_DEPTH_BUFFER(p->dev, zs, p->cp);
      gen6_emit_3DSTATE_HIER_DEPTH_BUFFER(p->dev, zs, p->cp);
      gen6_emit_3DSTATE_STENCIL_BUFFER(p->dev, zs, p->cp);
      gen7_emit_3DSTATE_CLEAR_PARAMS(p->dev, clear_params, p->cp);
   }
}

static void
gen7_pipeline_wm_multisample(struct ilo_3d_pipeline *p,
                             const struct ilo_context *ilo,
                             struct gen6_pipeline_session *session)
{
   /* 3DSTATE_MULTISAMPLE and 3DSTATE_SAMPLE_MASK */
   if (DIRTY(SAMPLE_MASK) || DIRTY(FB)) {
      const uint32_t *packed_sample_pos;

      gen7_wa_pipe_control_cs_stall(p, true, true);

      packed_sample_pos =
         (ilo->fb.num_samples > 4) ? p->packed_sample_position_8x :
         (ilo->fb.num_samples > 1) ? &p->packed_sample_position_4x :
         &p->packed_sample_position_1x;

      gen6_emit_3DSTATE_MULTISAMPLE(p->dev,
            ilo->fb.num_samples, packed_sample_pos,
            ilo->rasterizer->state.half_pixel_center, p->cp);

      gen7_emit_3DSTATE_SAMPLE_MASK(p->dev,
            (ilo->fb.num_samples > 1) ? ilo->sample_mask : 0x1,
            ilo->fb.num_samples, p->cp);
   }
}

static void
gen7_pipeline_vf_draw(struct ilo_3d_pipeline *p,
                      const struct ilo_context *ilo,
                      struct gen6_pipeline_session *session)
{
   /* 3DPRIMITIVE */
   gen7_emit_3DPRIMITIVE(p->dev, ilo->draw, &ilo->ib, false, p->cp);
   p->state.has_gen6_wa_pipe_control = false;
}

static void
gen7_pipeline_commands(struct ilo_3d_pipeline *p,
                       const struct ilo_context *ilo,
                       struct gen6_pipeline_session *session)
{
   /*
    * We try to keep the order of the commands match, as closely as possible,
    * that of the classic i965 driver.  It allows us to compare the command
    * streams easily.
    */
   gen6_pipeline_common_select(p, ilo, session);
   gen6_pipeline_common_sip(p, ilo, session);
   gen6_pipeline_vf_statistics(p, ilo, session);
   gen7_pipeline_common_pcb_alloc(p, ilo, session);
   gen6_pipeline_common_base_address(p, ilo, session);
   gen7_pipeline_common_pointers_1(p, ilo, session);
   gen7_pipeline_common_urb(p, ilo, session);
   gen7_pipeline_common_pointers_2(p, ilo, session);
   gen7_pipeline_wm_multisample(p, ilo, session);
   gen7_pipeline_gs(p, ilo, session);
   gen7_pipeline_hs(p, ilo, session);
   gen7_pipeline_te(p, ilo, session);
   gen7_pipeline_ds(p, ilo, session);
   gen7_pipeline_vs(p, ilo, session);
   gen7_pipeline_sol(p, ilo, session);
   gen6_pipeline_clip(p, ilo, session);
   gen7_pipeline_sf(p, ilo, session);
   gen7_pipeline_wm(p, ilo, session);
   gen6_pipeline_wm_raster(p, ilo, session);
   gen6_pipeline_sf_rect(p, ilo, session);
   gen6_pipeline_vf(p, ilo, session);
   gen7_pipeline_vf_draw(p, ilo, session);
}

static void
ilo_3d_pipeline_emit_draw_gen7(struct ilo_3d_pipeline *p,
                               const struct ilo_context *ilo)
{
   struct gen6_pipeline_session session;

   gen6_pipeline_prepare(p, ilo, &session);

   session.emit_draw_states = gen6_pipeline_states;
   session.emit_draw_commands = gen7_pipeline_commands;

   gen6_pipeline_draw(p, ilo, &session);
   gen6_pipeline_end(p, ilo, &session);
}

static void
gen7_rectlist_pcb_alloc(struct ilo_3d_pipeline *p,
                        const struct ilo_blitter *blitter,
                        struct gen6_rectlist_session *session)
{
   /*
    * Push constant buffers are only allowed to take up at most the first
    * 16KB of the URB.  Split the space evenly for VS and FS.
    */
   const int max_size =
      (p->dev->gen == ILO_GEN(7.5) && p->dev->gt == 3) ? 32768 : 16384;
   const int size = max_size / 2;
   int offset = 0;

   gen7_emit_3DSTATE_PUSH_CONSTANT_ALLOC_VS(p->dev, offset, size, p->cp);
   offset += size;

   gen7_emit_3DSTATE_PUSH_CONSTANT_ALLOC_PS(p->dev, offset, size, p->cp);

   gen7_wa_pipe_control_cs_stall(p, true, true);
}

static void
gen7_rectlist_urb(struct ilo_3d_pipeline *p,
                  const struct ilo_blitter *blitter,
                  struct gen6_rectlist_session *session)
{
   /* the first 16KB are reserved for VS and PS PCBs */
   const int offset =
      (p->dev->gen == ILO_GEN(7.5) && p->dev->gt == 3) ? 32768 : 16384;

   gen7_emit_3DSTATE_URB_VS(p->dev, offset, p->dev->urb_size - offset,
         blitter->ve.count * 4 * sizeof(float), p->cp);

   gen7_emit_3DSTATE_URB_GS(p->dev, offset, 0, 0, p->cp);
   gen7_emit_3DSTATE_URB_HS(p->dev, offset, 0, 0, p->cp);
   gen7_emit_3DSTATE_URB_DS(p->dev, offset, 0, 0, p->cp);
}

static void
gen7_rectlist_vs_to_sf(struct ilo_3d_pipeline *p,
                       const struct ilo_blitter *blitter,
                       struct gen6_rectlist_session *session)
{
   gen7_emit_3DSTATE_CONSTANT_VS(p->dev, NULL, NULL, 0, p->cp);
   gen6_emit_3DSTATE_VS(p->dev, NULL, 0, p->cp);

   gen7_emit_3DSTATE_CONSTANT_HS(p->dev, NULL, NULL, 0, p->cp);
   gen7_emit_3DSTATE_HS(p->dev, NULL, 0, p->cp);

   gen7_emit_3DSTATE_TE(p->dev, p->cp);

   gen7_emit_3DSTATE_CONSTANT_DS(p->dev, NULL, NULL, 0, p->cp);
   gen7_emit_3DSTATE_DS(p->dev, NULL, 0, p->cp);

   gen7_emit_3DSTATE_CONSTANT_GS(p->dev, NULL, NULL, 0, p->cp);
   gen7_emit_3DSTATE_GS(p->dev, NULL, 0, p->cp);

   gen7_emit_3DSTATE_STREAMOUT(p->dev, 0x0, 0, false, p->cp);

   gen6_emit_3DSTATE_CLIP(p->dev, NULL, NULL, false, 0, p->cp);

   gen7_wa_pipe_control_cs_stall(p, true, true);

   gen7_emit_3DSTATE_SF(p->dev, NULL, blitter->fb.dst.base.format, p->cp);
   gen7_emit_3DSTATE_SBE(p->dev, NULL, NULL, p->cp);
}

static void
gen7_rectlist_wm(struct ilo_3d_pipeline *p,
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

   gen7_emit_3DSTATE_WM(p->dev, NULL, NULL, false, hiz_op, p->cp);

   gen7_emit_3DSTATE_CONSTANT_PS(p->dev, NULL, NULL, 0, p->cp);

   gen7_wa_pipe_control_ps_max_threads_stall(p);
   gen7_emit_3DSTATE_PS(p->dev, NULL, 0, false, p->cp);
}

static void
gen7_rectlist_wm_depth(struct ilo_3d_pipeline *p,
                       const struct ilo_blitter *blitter,
                       struct gen6_rectlist_session *session)
{
   gen7_wa_pipe_control_wm_depth_stall(p, true);

   if (blitter->uses & (ILO_BLITTER_USE_FB_DEPTH |
                        ILO_BLITTER_USE_FB_STENCIL)) {
      gen6_emit_3DSTATE_DEPTH_BUFFER(p->dev,
            &blitter->fb.dst.u.zs, p->cp);
   }

   if (blitter->uses & ILO_BLITTER_USE_FB_DEPTH) {
      gen6_emit_3DSTATE_HIER_DEPTH_BUFFER(p->dev,
            &blitter->fb.dst.u.zs, p->cp);
   }

   if (blitter->uses & ILO_BLITTER_USE_FB_STENCIL) {
      gen6_emit_3DSTATE_STENCIL_BUFFER(p->dev,
            &blitter->fb.dst.u.zs, p->cp);
   }

   gen7_emit_3DSTATE_CLEAR_PARAMS(p->dev,
         blitter->depth_clear_value, p->cp);
}

static void
gen7_rectlist_wm_multisample(struct ilo_3d_pipeline *p,
                             const struct ilo_blitter *blitter,
                             struct gen6_rectlist_session *session)
{
   const uint32_t *packed_sample_pos =
      (blitter->fb.num_samples > 4) ? p->packed_sample_position_8x :
      (blitter->fb.num_samples > 1) ? &p->packed_sample_position_4x :
      &p->packed_sample_position_1x;

   gen7_wa_pipe_control_cs_stall(p, true, true);

   gen6_emit_3DSTATE_MULTISAMPLE(p->dev, blitter->fb.num_samples,
         packed_sample_pos, true, p->cp);

   gen7_emit_3DSTATE_SAMPLE_MASK(p->dev,
         (1 << blitter->fb.num_samples) - 1, blitter->fb.num_samples, p->cp);
}

static void
gen7_rectlist_commands(struct ilo_3d_pipeline *p,
                       const struct ilo_blitter *blitter,
                       struct gen6_rectlist_session *session)
{
   gen7_rectlist_wm_multisample(p, blitter, session);

   gen6_emit_STATE_BASE_ADDRESS(p->dev,
         NULL,                /* General State Base */
         p->cp->bo,           /* Surface State Base */
         p->cp->bo,           /* Dynamic State Base */
         NULL,                /* Indirect Object Base */
         NULL,                /* Instruction Base */
         0, 0, 0, 0, p->cp);

   gen6_emit_3DSTATE_VERTEX_BUFFERS(p->dev,
         &blitter->ve, &blitter->vb, p->cp);

   gen6_emit_3DSTATE_VERTEX_ELEMENTS(p->dev,
         &blitter->ve, false, false, p->cp);

   gen7_rectlist_pcb_alloc(p, blitter, session);

   /* needed for any VS-related commands */
   gen7_wa_pipe_control_vs_depth_stall(p);

   gen7_rectlist_urb(p, blitter, session);

   if (blitter->uses & ILO_BLITTER_USE_DSA) {
      gen7_emit_3DSTATE_DEPTH_STENCIL_STATE_POINTERS(p->dev,
            session->DEPTH_STENCIL_STATE, p->cp);
   }

   if (blitter->uses & ILO_BLITTER_USE_CC) {
      gen7_emit_3DSTATE_CC_STATE_POINTERS(p->dev,
            session->COLOR_CALC_STATE, p->cp);
   }

   gen7_rectlist_vs_to_sf(p, blitter, session);
   gen7_rectlist_wm(p, blitter, session);

   if (blitter->uses & ILO_BLITTER_USE_VIEWPORT) {
      gen7_emit_3DSTATE_VIEWPORT_STATE_POINTERS_CC(p->dev,
            session->CC_VIEWPORT, p->cp);
   }

   gen7_rectlist_wm_depth(p, blitter, session);

   gen6_emit_3DSTATE_DRAWING_RECTANGLE(p->dev, 0, 0,
         blitter->fb.width, blitter->fb.height, p->cp);

   gen7_emit_3DPRIMITIVE(p->dev, &blitter->draw, NULL, true, p->cp);
}

static void
gen7_rectlist_states(struct ilo_3d_pipeline *p,
                     const struct ilo_blitter *blitter,
                     struct gen6_rectlist_session *session)
{
   if (blitter->uses & ILO_BLITTER_USE_DSA) {
      session->DEPTH_STENCIL_STATE =
         gen6_emit_DEPTH_STENCIL_STATE(p->dev, &blitter->dsa, p->cp);
   }

   if (blitter->uses & ILO_BLITTER_USE_CC) {
      session->COLOR_CALC_STATE =
         gen6_emit_COLOR_CALC_STATE(p->dev, &blitter->cc.stencil_ref,
               blitter->cc.alpha_ref, &blitter->cc.blend_color, p->cp);
   }

   if (blitter->uses & ILO_BLITTER_USE_VIEWPORT) {
      session->CC_VIEWPORT =
         gen6_emit_CC_VIEWPORT(p->dev, &blitter->viewport, 1, p->cp);
   }
}

static void
ilo_3d_pipeline_emit_rectlist_gen7(struct ilo_3d_pipeline *p,
                                   const struct ilo_blitter *blitter)
{
   struct gen6_rectlist_session session;

   memset(&session, 0, sizeof(session));
   gen7_rectlist_states(p, blitter, &session);
   gen7_rectlist_commands(p, blitter, &session);
}

static int
gen7_pipeline_max_command_size(const struct ilo_3d_pipeline *p)
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
ilo_3d_pipeline_estimate_size_gen7(struct ilo_3d_pipeline *p,
                                   enum ilo_3d_pipeline_action action,
                                   const void *arg)
{
   int size;

   switch (action) {
   case ILO_3D_PIPELINE_DRAW:
      {
         const struct ilo_context *ilo = arg;

         size = gen7_pipeline_max_command_size(p) +
            gen6_pipeline_estimate_state_size(p, ilo);
      }
      break;
   case ILO_3D_PIPELINE_FLUSH:
   case ILO_3D_PIPELINE_WRITE_TIMESTAMP:
   case ILO_3D_PIPELINE_WRITE_DEPTH_COUNT:
      size = GEN6_PIPE_CONTROL__SIZE;
      break;
   case ILO_3D_PIPELINE_WRITE_STATISTICS:
      {
         const int num_regs = 10;
         const int num_pads = 1;

         size = GEN6_PIPE_CONTROL__SIZE;
         size += GEN6_MI_STORE_REGISTER_MEM__SIZE * 2 * num_regs;
         size += GEN6_MI_STORE_DATA_IMM__SIZE * num_pads;
      }
      break;
   case ILO_3D_PIPELINE_RECTLIST:
      size = 64 + 256; /* states + commands */
      break;
   default:
      assert(!"unknown 3D pipeline action");
      size = 0;
      break;
   }

   return size;
}

void
ilo_3d_pipeline_init_gen7(struct ilo_3d_pipeline *p)
{
   p->estimate_size = ilo_3d_pipeline_estimate_size_gen7;
   p->emit_draw = ilo_3d_pipeline_emit_draw_gen7;
   p->emit_flush = ilo_3d_pipeline_emit_flush_gen6;
   p->emit_write_timestamp = ilo_3d_pipeline_emit_write_timestamp_gen6;
   p->emit_write_depth_count = ilo_3d_pipeline_emit_write_depth_count_gen6;
   p->emit_write_statistics = ilo_3d_pipeline_emit_write_statistics_gen6;
   p->emit_rectlist = ilo_3d_pipeline_emit_rectlist_gen7;
}

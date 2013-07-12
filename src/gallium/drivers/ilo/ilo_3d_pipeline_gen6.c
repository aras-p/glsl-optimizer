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

#include "util/u_dual_blend.h"
#include "util/u_prim.h"
#include "intel_reg.h"

#include "ilo_3d.h"
#include "ilo_context.h"
#include "ilo_cp.h"
#include "ilo_gpe_gen6.h"
#include "ilo_shader.h"
#include "ilo_state.h"
#include "ilo_3d_pipeline.h"
#include "ilo_3d_pipeline_gen6.h"

/**
 * This should be called before any depth stall flush (including those
 * produced by non-pipelined state commands) or cache flush on GEN6.
 *
 * \see intel_emit_post_sync_nonzero_flush()
 */
static void
gen6_wa_pipe_control_post_sync(struct ilo_3d_pipeline *p,
                               bool caller_post_sync)
{
   assert(p->dev->gen == ILO_GEN(6));

   /* emit once */
   if (p->state.has_gen6_wa_pipe_control)
      return;

   p->state.has_gen6_wa_pipe_control = true;

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 60:
    *
    *     "Pipe-control with CS-stall bit set must be sent BEFORE the
    *      pipe-control with a post-sync op and no write-cache flushes."
    *
    * The workaround below necessitates this workaround.
    */
   p->gen6_PIPE_CONTROL(p->dev,
         PIPE_CONTROL_CS_STALL |
         PIPE_CONTROL_STALL_AT_SCOREBOARD,
         NULL, 0, false, p->cp);

   /* the caller will emit the post-sync op */
   if (caller_post_sync)
      return;

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 60:
    *
    *     "Before any depth stall flush (including those produced by
    *      non-pipelined state commands), software needs to first send a
    *      PIPE_CONTROL with no bits set except Post-Sync Operation != 0."
    *
    *     "Before a PIPE_CONTROL with Write Cache Flush Enable =1, a
    *      PIPE_CONTROL with any non-zero post-sync-op is required."
    */
   p->gen6_PIPE_CONTROL(p->dev,
         PIPE_CONTROL_WRITE_IMMEDIATE,
         p->workaround_bo, 0, false, p->cp);
}

static void
gen6_wa_pipe_control_wm_multisample_flush(struct ilo_3d_pipeline *p)
{
   assert(p->dev->gen == ILO_GEN(6));

   gen6_wa_pipe_control_post_sync(p, false);

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 305:
    *
    *     "Driver must guarentee that all the caches in the depth pipe are
    *      flushed before this command (3DSTATE_MULTISAMPLE) is parsed. This
    *      requires driver to send a PIPE_CONTROL with a CS stall along with a
    *      Depth Flush prior to this command."
    */
   p->gen6_PIPE_CONTROL(p->dev,
         PIPE_CONTROL_DEPTH_CACHE_FLUSH |
         PIPE_CONTROL_CS_STALL,
         0, 0, false, p->cp);
}

static void
gen6_wa_pipe_control_wm_depth_flush(struct ilo_3d_pipeline *p)
{
   assert(p->dev->gen == ILO_GEN(6));

   gen6_wa_pipe_control_post_sync(p, false);

   /*
    * According to intel_emit_depth_stall_flushes() of classic i965, we need
    * to emit a sequence of PIPE_CONTROLs prior to emitting depth related
    * commands.
    */
   p->gen6_PIPE_CONTROL(p->dev,
         PIPE_CONTROL_DEPTH_STALL,
         NULL, 0, false, p->cp);

   p->gen6_PIPE_CONTROL(p->dev,
         PIPE_CONTROL_DEPTH_CACHE_FLUSH,
         NULL, 0, false, p->cp);

   p->gen6_PIPE_CONTROL(p->dev,
         PIPE_CONTROL_DEPTH_STALL,
         NULL, 0, false, p->cp);
}

static void
gen6_wa_pipe_control_wm_max_threads_stall(struct ilo_3d_pipeline *p)
{
   assert(p->dev->gen == ILO_GEN(6));

   /* the post-sync workaround should cover this already */
   if (p->state.has_gen6_wa_pipe_control)
      return;

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 274:
    *
    *     "A PIPE_CONTROL command, with only the Stall At Pixel Scoreboard
    *      field set (DW1 Bit 1), must be issued prior to any change to the
    *      value in this field (Maximum Number of Threads in 3DSTATE_WM)"
    */
   p->gen6_PIPE_CONTROL(p->dev,
         PIPE_CONTROL_STALL_AT_SCOREBOARD,
         NULL, 0, false, p->cp);

}

static void
gen6_wa_pipe_control_vs_const_flush(struct ilo_3d_pipeline *p)
{
   assert(p->dev->gen == ILO_GEN(6));

   gen6_wa_pipe_control_post_sync(p, false);

   /*
    * According to upload_vs_state() of classic i965, we need to emit
    * PIPE_CONTROL after 3DSTATE_CONSTANT_VS so that the command is kept being
    * buffered by VS FF, to the point that the FF dies.
    */
   p->gen6_PIPE_CONTROL(p->dev,
         PIPE_CONTROL_DEPTH_STALL |
         PIPE_CONTROL_INSTRUCTION_FLUSH |
         PIPE_CONTROL_STATE_CACHE_INVALIDATE,
         NULL, 0, false, p->cp);
}

#define DIRTY(state) (session->pipe_dirty & ILO_DIRTY_ ## state)

void
gen6_pipeline_common_select(struct ilo_3d_pipeline *p,
                            const struct ilo_context *ilo,
                            struct gen6_pipeline_session *session)
{
   /* PIPELINE_SELECT */
   if (session->hw_ctx_changed) {
      if (p->dev->gen == ILO_GEN(6))
         gen6_wa_pipe_control_post_sync(p, false);

      p->gen6_PIPELINE_SELECT(p->dev, 0x0, p->cp);
   }
}

void
gen6_pipeline_common_sip(struct ilo_3d_pipeline *p,
                         const struct ilo_context *ilo,
                         struct gen6_pipeline_session *session)
{
   /* STATE_SIP */
   if (session->hw_ctx_changed) {
      if (p->dev->gen == ILO_GEN(6))
         gen6_wa_pipe_control_post_sync(p, false);

      p->gen6_STATE_SIP(p->dev, 0, p->cp);
   }
}

void
gen6_pipeline_common_base_address(struct ilo_3d_pipeline *p,
                                  const struct ilo_context *ilo,
                                  struct gen6_pipeline_session *session)
{
   /* STATE_BASE_ADDRESS */
   if (session->state_bo_changed || session->kernel_bo_changed ||
       session->batch_bo_changed) {
      if (p->dev->gen == ILO_GEN(6))
         gen6_wa_pipe_control_post_sync(p, false);

      p->gen6_STATE_BASE_ADDRESS(p->dev,
            NULL, p->cp->bo, p->cp->bo, NULL, ilo->hw3d->kernel.bo,
            0, 0, 0, 0, p->cp);

      /*
       * From the Sandy Bridge PRM, volume 1 part 1, page 28:
       *
       *     "The following commands must be reissued following any change to
       *      the base addresses:
       *
       *       * 3DSTATE_BINDING_TABLE_POINTERS
       *       * 3DSTATE_SAMPLER_STATE_POINTERS
       *       * 3DSTATE_VIEWPORT_STATE_POINTERS
       *       * 3DSTATE_CC_POINTERS
       *       * MEDIA_STATE_POINTERS"
       *
       * 3DSTATE_SCISSOR_STATE_POINTERS is not on the list, but it is
       * reasonable to also reissue the command.  Same to PCB.
       */
      session->viewport_state_changed = true;

      session->cc_state_blend_changed = true;
      session->cc_state_dsa_changed = true;
      session->cc_state_cc_changed = true;

      session->scissor_state_changed = true;

      session->binding_table_vs_changed = true;
      session->binding_table_gs_changed = true;
      session->binding_table_fs_changed = true;

      session->sampler_state_vs_changed = true;
      session->sampler_state_gs_changed = true;
      session->sampler_state_fs_changed = true;

      session->pcb_state_vs_changed = true;
      session->pcb_state_gs_changed = true;
      session->pcb_state_fs_changed = true;
   }
}

static void
gen6_pipeline_common_urb(struct ilo_3d_pipeline *p,
                         const struct ilo_context *ilo,
                         struct gen6_pipeline_session *session)
{
   /* 3DSTATE_URB */
   if (DIRTY(VE) || DIRTY(VS) || DIRTY(GS)) {
      const bool gs_active = (ilo->gs || (ilo->vs &&
               ilo_shader_get_kernel_param(ilo->vs, ILO_KERNEL_VS_GEN6_SO)));
      int vs_entry_size, gs_entry_size;
      int vs_total_size, gs_total_size;

      vs_entry_size = (ilo->vs) ?
         ilo_shader_get_kernel_param(ilo->vs, ILO_KERNEL_OUTPUT_COUNT) : 0;

      /*
       * As indicated by 2e712e41db0c0676e9f30fc73172c0e8de8d84d4, VF and VS
       * share VUE handles.  The VUE allocation size must be large enough to
       * store either VF outputs (number of VERTEX_ELEMENTs) and VS outputs.
       *
       * I am not sure if the PRM explicitly states that VF and VS share VUE
       * handles.  But here is a citation that implies so:
       *
       * From the Sandy Bridge PRM, volume 2 part 1, page 44:
       *
       *     "Once a FF stage that spawn threads has sufficient input to
       *      initiate a thread, it must guarantee that it is safe to request
       *      the thread initiation. For all these FF stages, this check is
       *      based on :
       *
       *      - The availability of output URB entries:
       *        - VS: As the input URB entries are overwritten with the
       *          VS-generated output data, output URB availability isn't a
       *          factor."
       */
      if (vs_entry_size < ilo->ve->count)
         vs_entry_size = ilo->ve->count;

      gs_entry_size = (ilo->gs) ?
         ilo_shader_get_kernel_param(ilo->gs, ILO_KERNEL_OUTPUT_COUNT) :
         (gs_active) ? vs_entry_size : 0;

      /* in bytes */
      vs_entry_size *= sizeof(float) * 4;
      gs_entry_size *= sizeof(float) * 4;
      vs_total_size = ilo->dev->urb_size;

      if (gs_active) {
         vs_total_size /= 2;
         gs_total_size = vs_total_size;
      }
      else {
         gs_total_size = 0;
      }

      p->gen6_3DSTATE_URB(p->dev, vs_total_size, gs_total_size,
            vs_entry_size, gs_entry_size, p->cp);

      /*
       * From the Sandy Bridge PRM, volume 2 part 1, page 27:
       *
       *     "Because of a urb corruption caused by allocating a previous
       *      gsunit's urb entry to vsunit software is required to send a
       *      "GS NULL Fence" (Send URB fence with VS URB size == 1 and GS URB
       *      size == 0) plus a dummy DRAW call before any case where VS will
       *      be taking over GS URB space."
       */
      if (p->state.gs.active && !gs_active)
         ilo_3d_pipeline_emit_flush_gen6(p);

      p->state.gs.active = gs_active;
   }
}

static void
gen6_pipeline_common_pointers_1(struct ilo_3d_pipeline *p,
                                const struct ilo_context *ilo,
                                struct gen6_pipeline_session *session)
{
   /* 3DSTATE_VIEWPORT_STATE_POINTERS */
   if (session->viewport_state_changed) {
      p->gen6_3DSTATE_VIEWPORT_STATE_POINTERS(p->dev,
            p->state.CLIP_VIEWPORT,
            p->state.SF_VIEWPORT,
            p->state.CC_VIEWPORT, p->cp);
   }
}

static void
gen6_pipeline_common_pointers_2(struct ilo_3d_pipeline *p,
                                const struct ilo_context *ilo,
                                struct gen6_pipeline_session *session)
{
   /* 3DSTATE_CC_STATE_POINTERS */
   if (session->cc_state_blend_changed ||
       session->cc_state_dsa_changed ||
       session->cc_state_cc_changed) {
      p->gen6_3DSTATE_CC_STATE_POINTERS(p->dev,
            p->state.BLEND_STATE,
            p->state.DEPTH_STENCIL_STATE,
            p->state.COLOR_CALC_STATE, p->cp);
   }

   /* 3DSTATE_SAMPLER_STATE_POINTERS */
   if (session->sampler_state_vs_changed ||
       session->sampler_state_gs_changed ||
       session->sampler_state_fs_changed) {
      p->gen6_3DSTATE_SAMPLER_STATE_POINTERS(p->dev,
            p->state.vs.SAMPLER_STATE,
            0,
            p->state.wm.SAMPLER_STATE, p->cp);
   }
}

static void
gen6_pipeline_common_pointers_3(struct ilo_3d_pipeline *p,
                                const struct ilo_context *ilo,
                                struct gen6_pipeline_session *session)
{
   /* 3DSTATE_SCISSOR_STATE_POINTERS */
   if (session->scissor_state_changed) {
      p->gen6_3DSTATE_SCISSOR_STATE_POINTERS(p->dev,
            p->state.SCISSOR_RECT, p->cp);
   }

   /* 3DSTATE_BINDING_TABLE_POINTERS */
   if (session->binding_table_vs_changed ||
       session->binding_table_gs_changed ||
       session->binding_table_fs_changed) {
      p->gen6_3DSTATE_BINDING_TABLE_POINTERS(p->dev,
            p->state.vs.BINDING_TABLE_STATE,
            p->state.gs.BINDING_TABLE_STATE,
            p->state.wm.BINDING_TABLE_STATE, p->cp);
   }
}

void
gen6_pipeline_vf(struct ilo_3d_pipeline *p,
                 const struct ilo_context *ilo,
                 struct gen6_pipeline_session *session)
{
   /* 3DSTATE_INDEX_BUFFER */
   if (DIRTY(IB) || session->primitive_restart_changed ||
       session->batch_bo_changed) {
      p->gen6_3DSTATE_INDEX_BUFFER(p->dev,
            &ilo->ib, ilo->draw->primitive_restart, p->cp);
   }

   /* 3DSTATE_VERTEX_BUFFERS */
   if (DIRTY(VB) || DIRTY(VE) || session->batch_bo_changed) {
      p->gen6_3DSTATE_VERTEX_BUFFERS(p->dev,
            ilo->vb.states, ilo->vb.enabled_mask, ilo->ve, p->cp);
   }

   /* 3DSTATE_VERTEX_ELEMENTS */
   if (DIRTY(VE) || DIRTY(VS)) {
      const struct ilo_ve_state *ve = ilo->ve;
      bool last_velement_edgeflag = false;
      bool prepend_generate_ids = false;

      if (ilo->vs) {
         if (ilo_shader_get_kernel_param(ilo->vs,
                  ILO_KERNEL_VS_INPUT_EDGEFLAG)) {
            /* we rely on the state tracker here */
            assert(ilo_shader_get_kernel_param(ilo->vs,
                     ILO_KERNEL_INPUT_COUNT) == ve->count);

            last_velement_edgeflag = true;
         }

         if (ilo_shader_get_kernel_param(ilo->vs,
                  ILO_KERNEL_VS_INPUT_INSTANCEID) ||
             ilo_shader_get_kernel_param(ilo->vs,
                  ILO_KERNEL_VS_INPUT_VERTEXID))
            prepend_generate_ids = true;
      }

      p->gen6_3DSTATE_VERTEX_ELEMENTS(p->dev, ve,
            last_velement_edgeflag, prepend_generate_ids, p->cp);
   }
}

void
gen6_pipeline_vf_statistics(struct ilo_3d_pipeline *p,
                            const struct ilo_context *ilo,
                            struct gen6_pipeline_session *session)
{
   /* 3DSTATE_VF_STATISTICS */
   if (session->hw_ctx_changed)
      p->gen6_3DSTATE_VF_STATISTICS(p->dev, false, p->cp);
}

void
gen6_pipeline_vf_draw(struct ilo_3d_pipeline *p,
                      const struct ilo_context *ilo,
                      struct gen6_pipeline_session *session)
{
   /* 3DPRIMITIVE */
   p->gen6_3DPRIMITIVE(p->dev, ilo->draw, &ilo->ib, false, p->cp);
   p->state.has_gen6_wa_pipe_control = false;
}

void
gen6_pipeline_vs(struct ilo_3d_pipeline *p,
                 const struct ilo_context *ilo,
                 struct gen6_pipeline_session *session)
{
   const bool emit_3dstate_vs = (DIRTY(VS) || DIRTY(SAMPLER_VS) ||
                                 session->kernel_bo_changed);
   const bool emit_3dstate_constant_vs = session->pcb_state_vs_changed;

   /*
    * the classic i965 does this in upload_vs_state(), citing a spec that I
    * cannot find
    */
   if (emit_3dstate_vs && p->dev->gen == ILO_GEN(6))
      gen6_wa_pipe_control_post_sync(p, false);

   /* 3DSTATE_CONSTANT_VS */
   if (emit_3dstate_constant_vs) {
      p->gen6_3DSTATE_CONSTANT_VS(p->dev,
            &p->state.vs.PUSH_CONSTANT_BUFFER,
            &p->state.vs.PUSH_CONSTANT_BUFFER_size,
            1, p->cp);
   }

   /* 3DSTATE_VS */
   if (emit_3dstate_vs) {
      const int num_samplers = ilo->sampler[PIPE_SHADER_VERTEX].count;

      p->gen6_3DSTATE_VS(p->dev, ilo->vs, num_samplers, p->cp);
   }

   if (emit_3dstate_constant_vs && p->dev->gen == ILO_GEN(6))
      gen6_wa_pipe_control_vs_const_flush(p);
}

static void
gen6_pipeline_gs(struct ilo_3d_pipeline *p,
                 const struct ilo_context *ilo,
                 struct gen6_pipeline_session *session)
{
   /* 3DSTATE_CONSTANT_GS */
   if (session->pcb_state_gs_changed)
      p->gen6_3DSTATE_CONSTANT_GS(p->dev, NULL, NULL, 0, p->cp);

   /* 3DSTATE_GS */
   if (DIRTY(GS) || DIRTY(VS) ||
       session->prim_changed || session->kernel_bo_changed) {
      const int verts_per_prim = u_vertices_per_prim(session->reduced_prim);

      p->gen6_3DSTATE_GS(p->dev, ilo->gs, ilo->vs, verts_per_prim, p->cp);
   }
}

bool
gen6_pipeline_update_max_svbi(struct ilo_3d_pipeline *p,
                              const struct ilo_context *ilo,
                              struct gen6_pipeline_session *session)
{
   if (DIRTY(VS) || DIRTY(GS) || DIRTY(SO)) {
      const struct pipe_stream_output_info *so_info =
         (ilo->gs) ? ilo_shader_get_kernel_so_info(ilo->gs) :
         (ilo->vs) ? ilo_shader_get_kernel_so_info(ilo->vs) : NULL;
      unsigned max_svbi = 0xffffffff;
      int i;

      for (i = 0; i < so_info->num_outputs; i++) {
         const int output_buffer = so_info->output[i].output_buffer;
         const struct pipe_stream_output_target *so =
            ilo->so.states[output_buffer];
         const int struct_size = so_info->stride[output_buffer] * 4;
         const int elem_size = so_info->output[i].num_components * 4;
         int buf_size, count;

         if (!so) {
            max_svbi = 0;
            break;
         }

         buf_size = so->buffer_size - so_info->output[i].dst_offset * 4;

         count = buf_size / struct_size;
         if (buf_size % struct_size >= elem_size)
            count++;

         if (count < max_svbi)
            max_svbi = count;
      }

      if (p->state.so_max_vertices != max_svbi) {
         p->state.so_max_vertices = max_svbi;
         return true;
      }
   }

   return false;
}

static void
gen6_pipeline_gs_svbi(struct ilo_3d_pipeline *p,
                      const struct ilo_context *ilo,
                      struct gen6_pipeline_session *session)
{
   const bool emit = gen6_pipeline_update_max_svbi(p, ilo, session);

   /* 3DSTATE_GS_SVB_INDEX */
   if (emit) {
      if (p->dev->gen == ILO_GEN(6))
         gen6_wa_pipe_control_post_sync(p, false);

      p->gen6_3DSTATE_GS_SVB_INDEX(p->dev,
            0, p->state.so_num_vertices, p->state.so_max_vertices,
            false, p->cp);

      if (session->hw_ctx_changed) {
         int i;

         /*
          * From the Sandy Bridge PRM, volume 2 part 1, page 148:
          *
          *     "If a buffer is not enabled then the SVBI must be set to 0x0
          *      in order to not cause overflow in that SVBI."
          *
          *     "If a buffer is not enabled then the MaxSVBI must be set to
          *      0xFFFFFFFF in order to not cause overflow in that SVBI."
          */
         for (i = 1; i < 4; i++) {
            p->gen6_3DSTATE_GS_SVB_INDEX(p->dev,
                  i, 0, 0xffffffff, false, p->cp);
         }
      }
   }
}

void
gen6_pipeline_clip(struct ilo_3d_pipeline *p,
                   const struct ilo_context *ilo,
                   struct gen6_pipeline_session *session)
{
   /* 3DSTATE_CLIP */
   if (DIRTY(RASTERIZER) || DIRTY(FS) || DIRTY(VIEWPORT) || DIRTY(FB)) {
      bool enable_guardband = true;
      unsigned i;

      /*
       * We do not do 2D clipping yet.  Guard band test should only be enabled
       * when the viewport is larger than the framebuffer.
       */
      for (i = 0; i < ilo->viewport.count; i++) {
         const struct ilo_viewport_cso *vp = &ilo->viewport.cso[i];

         if (vp->min_x > 0.0f || vp->max_x < ilo->fb.state.width ||
             vp->min_y > 0.0f || vp->max_y < ilo->fb.state.height) {
            enable_guardband = false;
            break;
         }
      }

      p->gen6_3DSTATE_CLIP(p->dev, ilo->rasterizer,
            ilo->fs, enable_guardband, 1, p->cp);
   }
}

static void
gen6_pipeline_sf(struct ilo_3d_pipeline *p,
                 const struct ilo_context *ilo,
                 struct gen6_pipeline_session *session)
{
   /* 3DSTATE_SF */
   if (DIRTY(RASTERIZER) || DIRTY(VS) || DIRTY(GS) || DIRTY(FS)) {
      p->gen6_3DSTATE_SF(p->dev, ilo->rasterizer, ilo->fs,
            (ilo->gs) ? ilo->gs : ilo->vs, p->cp);
   }
}

void
gen6_pipeline_sf_rect(struct ilo_3d_pipeline *p,
                      const struct ilo_context *ilo,
                      struct gen6_pipeline_session *session)
{
   /* 3DSTATE_DRAWING_RECTANGLE */
   if (DIRTY(FB)) {
      if (p->dev->gen == ILO_GEN(6))
         gen6_wa_pipe_control_post_sync(p, false);

      p->gen6_3DSTATE_DRAWING_RECTANGLE(p->dev, 0, 0,
            ilo->fb.state.width, ilo->fb.state.height, p->cp);
   }
}

static void
gen6_pipeline_wm(struct ilo_3d_pipeline *p,
                 const struct ilo_context *ilo,
                 struct gen6_pipeline_session *session)
{
   /* 3DSTATE_CONSTANT_PS */
   if (session->pcb_state_fs_changed)
      p->gen6_3DSTATE_CONSTANT_PS(p->dev, NULL, NULL, 0, p->cp);

   /* 3DSTATE_WM */
   if (DIRTY(FS) || DIRTY(SAMPLER_FS) || DIRTY(BLEND) || DIRTY(DSA) ||
       DIRTY(RASTERIZER) || session->kernel_bo_changed) {
      const int num_samplers = ilo->sampler[PIPE_SHADER_FRAGMENT].count;
      const bool dual_blend = ilo->blend->dual_blend;
      const bool cc_may_kill = (ilo->dsa->alpha.enabled ||
                                ilo->blend->alpha_to_coverage);

      if (p->dev->gen == ILO_GEN(6) && session->hw_ctx_changed)
         gen6_wa_pipe_control_wm_max_threads_stall(p);

      p->gen6_3DSTATE_WM(p->dev, ilo->fs, num_samplers,
            ilo->rasterizer, dual_blend, cc_may_kill, p->cp);
   }
}

static void
gen6_pipeline_wm_multisample(struct ilo_3d_pipeline *p,
                             const struct ilo_context *ilo,
                             struct gen6_pipeline_session *session)
{
   /* 3DSTATE_MULTISAMPLE and 3DSTATE_SAMPLE_MASK */
   if (DIRTY(SAMPLE_MASK) || DIRTY(FB)) {
      const uint32_t *packed_sample_pos;

      packed_sample_pos = (ilo->fb.num_samples > 1) ?
         &p->packed_sample_position_4x : &p->packed_sample_position_1x;

      if (p->dev->gen == ILO_GEN(6)) {
         gen6_wa_pipe_control_post_sync(p, false);
         gen6_wa_pipe_control_wm_multisample_flush(p);
      }

      p->gen6_3DSTATE_MULTISAMPLE(p->dev,
            ilo->fb.num_samples, packed_sample_pos,
            ilo->rasterizer->state.half_pixel_center, p->cp);

      p->gen6_3DSTATE_SAMPLE_MASK(p->dev,
            (ilo->fb.num_samples > 1) ? ilo->sample_mask : 0x1, p->cp);
   }
}

static void
gen6_pipeline_wm_depth(struct ilo_3d_pipeline *p,
                       const struct ilo_context *ilo,
                       struct gen6_pipeline_session *session)
{
   /* 3DSTATE_DEPTH_BUFFER and 3DSTATE_CLEAR_PARAMS */
   if (DIRTY(FB) || session->batch_bo_changed) {
      const struct ilo_zs_surface *zs;

      if (ilo->fb.state.zsbuf) {
         const struct ilo_surface_cso *surface =
            (const struct ilo_surface_cso *) ilo->fb.state.zsbuf;

         assert(!surface->is_rt);
         zs = &surface->u.zs;
      }
      else {
         zs = &ilo->fb.null_zs;
      }

      if (p->dev->gen == ILO_GEN(6)) {
         gen6_wa_pipe_control_post_sync(p, false);
         gen6_wa_pipe_control_wm_depth_flush(p);
      }

      p->gen6_3DSTATE_DEPTH_BUFFER(p->dev, zs, p->cp);

      /* TODO */
      p->gen6_3DSTATE_CLEAR_PARAMS(p->dev, 0, p->cp);
   }
}

void
gen6_pipeline_wm_raster(struct ilo_3d_pipeline *p,
                        const struct ilo_context *ilo,
                        struct gen6_pipeline_session *session)
{
   /* 3DSTATE_POLY_STIPPLE_PATTERN and 3DSTATE_POLY_STIPPLE_OFFSET */
   if ((DIRTY(RASTERIZER) || DIRTY(POLY_STIPPLE)) &&
       ilo->rasterizer->state.poly_stipple_enable) {
      if (p->dev->gen == ILO_GEN(6))
         gen6_wa_pipe_control_post_sync(p, false);

      p->gen6_3DSTATE_POLY_STIPPLE_PATTERN(p->dev,
            &ilo->poly_stipple, p->cp);

      p->gen6_3DSTATE_POLY_STIPPLE_OFFSET(p->dev, 0, 0, p->cp);
   }

   /* 3DSTATE_LINE_STIPPLE */
   if (DIRTY(RASTERIZER) && ilo->rasterizer->state.line_stipple_enable) {
      if (p->dev->gen == ILO_GEN(6))
         gen6_wa_pipe_control_post_sync(p, false);

      p->gen6_3DSTATE_LINE_STIPPLE(p->dev,
            ilo->rasterizer->state.line_stipple_pattern,
            ilo->rasterizer->state.line_stipple_factor + 1, p->cp);
   }

   /* 3DSTATE_AA_LINE_PARAMETERS */
   if (DIRTY(RASTERIZER) && ilo->rasterizer->state.line_smooth) {
      if (p->dev->gen == ILO_GEN(6))
         gen6_wa_pipe_control_post_sync(p, false);

      p->gen6_3DSTATE_AA_LINE_PARAMETERS(p->dev, p->cp);
   }
}

static void
gen6_pipeline_state_viewports(struct ilo_3d_pipeline *p,
                              const struct ilo_context *ilo,
                              struct gen6_pipeline_session *session)
{
   /* SF_CLIP_VIEWPORT and CC_VIEWPORT */
   if (p->dev->gen >= ILO_GEN(7) && DIRTY(VIEWPORT)) {
      p->state.SF_CLIP_VIEWPORT = p->gen7_SF_CLIP_VIEWPORT(p->dev,
            ilo->viewport.cso, ilo->viewport.count, p->cp);

      p->state.CC_VIEWPORT = p->gen6_CC_VIEWPORT(p->dev,
            ilo->viewport.cso, ilo->viewport.count, p->cp);

      session->viewport_state_changed = true;
   }
   /* SF_VIEWPORT, CLIP_VIEWPORT, and CC_VIEWPORT */
   else if (DIRTY(VIEWPORT)) {
      p->state.CLIP_VIEWPORT = p->gen6_CLIP_VIEWPORT(p->dev,
            ilo->viewport.cso, ilo->viewport.count, p->cp);

      p->state.SF_VIEWPORT = p->gen6_SF_VIEWPORT(p->dev,
            ilo->viewport.cso, ilo->viewport.count, p->cp);

      p->state.CC_VIEWPORT = p->gen6_CC_VIEWPORT(p->dev,
            ilo->viewport.cso, ilo->viewport.count, p->cp);

      session->viewport_state_changed = true;
   }
}

static void
gen6_pipeline_state_cc(struct ilo_3d_pipeline *p,
                       const struct ilo_context *ilo,
                       struct gen6_pipeline_session *session)
{
   /* BLEND_STATE */
   if (DIRTY(BLEND) || DIRTY(FB) || DIRTY(DSA)) {
      p->state.BLEND_STATE = p->gen6_BLEND_STATE(p->dev,
            ilo->blend, &ilo->fb, &ilo->dsa->alpha, p->cp);

      session->cc_state_blend_changed = true;
   }

   /* COLOR_CALC_STATE */
   if (DIRTY(DSA) || DIRTY(STENCIL_REF) || DIRTY(BLEND_COLOR)) {
      p->state.COLOR_CALC_STATE =
         p->gen6_COLOR_CALC_STATE(p->dev, &ilo->stencil_ref,
               ilo->dsa->alpha.ref_value, &ilo->blend_color, p->cp);

      session->cc_state_cc_changed = true;
   }

   /* DEPTH_STENCIL_STATE */
   if (DIRTY(DSA)) {
      p->state.DEPTH_STENCIL_STATE =
         p->gen6_DEPTH_STENCIL_STATE(p->dev, ilo->dsa, p->cp);

      session->cc_state_dsa_changed = true;
   }
}

static void
gen6_pipeline_state_scissors(struct ilo_3d_pipeline *p,
                             const struct ilo_context *ilo,
                             struct gen6_pipeline_session *session)
{
   /* SCISSOR_RECT */
   if (DIRTY(SCISSOR) || DIRTY(VIEWPORT)) {
      /* there should be as many scissors as there are viewports */
      p->state.SCISSOR_RECT = p->gen6_SCISSOR_RECT(p->dev,
            &ilo->scissor, ilo->viewport.count, p->cp);

      session->scissor_state_changed = true;
   }
}

static void
gen6_pipeline_state_surfaces_rt(struct ilo_3d_pipeline *p,
                                const struct ilo_context *ilo,
                                struct gen6_pipeline_session *session)
{
   /* SURFACE_STATEs for render targets */
   if (DIRTY(FB)) {
      const struct ilo_fb_state *fb = &ilo->fb;
      const int offset = ILO_WM_DRAW_SURFACE(0);
      uint32_t *surface_state = &p->state.wm.SURFACE_STATE[offset];
      int i;

      for (i = 0; i < fb->state.nr_cbufs; i++) {
         const struct ilo_surface_cso *surface =
            (const struct ilo_surface_cso *) fb->state.cbufs[i];

         assert(surface && surface->is_rt);
         surface_state[i] =
            p->gen6_SURFACE_STATE(p->dev, &surface->u.rt, true, p->cp);
      }

      /*
       * Upload at least one render target, as
       * brw_update_renderbuffer_surfaces() does.  I don't know why.
       */
      if (i == 0) {
         struct ilo_view_surface null_surface;

         ilo_gpe_init_view_surface_null(p->dev,
               fb->state.width, fb->state.height,
               1, 0, &null_surface);

         surface_state[i] =
            p->gen6_SURFACE_STATE(p->dev, &null_surface, true, p->cp);

         i++;
      }

      memset(&surface_state[i], 0, (ILO_MAX_DRAW_BUFFERS - i) * 4);

      if (i && session->num_surfaces[PIPE_SHADER_FRAGMENT] < offset + i)
         session->num_surfaces[PIPE_SHADER_FRAGMENT] = offset + i;

      session->binding_table_fs_changed = true;
   }
}

static void
gen6_pipeline_state_surfaces_so(struct ilo_3d_pipeline *p,
                                const struct ilo_context *ilo,
                                struct gen6_pipeline_session *session)
{
   const struct ilo_so_state *so = &ilo->so;

   if (p->dev->gen != ILO_GEN(6))
      return;

   /* SURFACE_STATEs for stream output targets */
   if (DIRTY(VS) || DIRTY(GS) || DIRTY(SO)) {
      const struct pipe_stream_output_info *so_info =
         (ilo->gs) ? ilo_shader_get_kernel_so_info(ilo->gs) :
         (ilo->vs) ? ilo_shader_get_kernel_so_info(ilo->vs) : NULL;
      const int offset = ILO_GS_SO_SURFACE(0);
      uint32_t *surface_state = &p->state.gs.SURFACE_STATE[offset];
      int i;

      for (i = 0; so_info && i < so_info->num_outputs; i++) {
         const int target = so_info->output[i].output_buffer;
         const struct pipe_stream_output_target *so_target =
            (target < so->count) ? so->states[target] : NULL;

         if (so_target) {
            surface_state[i] = p->gen6_so_SURFACE_STATE(p->dev,
                  so_target, so_info, i, p->cp);
         }
         else {
            surface_state[i] = 0;
         }
      }

      memset(&surface_state[i], 0, (ILO_MAX_SO_BINDINGS - i) * 4);

      if (i && session->num_surfaces[PIPE_SHADER_GEOMETRY] < offset + i)
         session->num_surfaces[PIPE_SHADER_GEOMETRY] = offset + i;

      session->binding_table_gs_changed = true;
   }
}

static void
gen6_pipeline_state_surfaces_view(struct ilo_3d_pipeline *p,
                                  const struct ilo_context *ilo,
                                  int shader_type,
                                  struct gen6_pipeline_session *session)
{
   const struct ilo_view_state *view = &ilo->view[shader_type];
   uint32_t *surface_state;
   int offset, i;
   bool skip = false;

   /* SURFACE_STATEs for sampler views */
   switch (shader_type) {
   case PIPE_SHADER_VERTEX:
      if (DIRTY(VIEW_VS)) {
         offset = ILO_VS_TEXTURE_SURFACE(0);
         surface_state = &p->state.vs.SURFACE_STATE[offset];

         session->binding_table_vs_changed = true;
      }
      else {
         skip = true;
      }
      break;
   case PIPE_SHADER_FRAGMENT:
      if (DIRTY(VIEW_FS)) {
         offset = ILO_WM_TEXTURE_SURFACE(0);
         surface_state = &p->state.wm.SURFACE_STATE[offset];

         session->binding_table_fs_changed = true;
      }
      else {
         skip = true;
      }
      break;
   default:
      skip = true;
      break;
   }

   if (skip)
      return;

   for (i = 0; i < view->count; i++) {
      if (view->states[i]) {
         const struct ilo_view_cso *cso =
            (const struct ilo_view_cso *) view->states[i];

         surface_state[i] =
            p->gen6_SURFACE_STATE(p->dev, &cso->surface, false, p->cp);
      }
      else {
         surface_state[i] = 0;
      }
   }

   memset(&surface_state[i], 0, (ILO_MAX_SAMPLER_VIEWS - i) * 4);

   if (i && session->num_surfaces[shader_type] < offset + i)
      session->num_surfaces[shader_type] = offset + i;
}

static void
gen6_pipeline_state_surfaces_const(struct ilo_3d_pipeline *p,
                                   const struct ilo_context *ilo,
                                   int shader_type,
                                   struct gen6_pipeline_session *session)
{
   const struct ilo_cbuf_state *cbuf = &ilo->cbuf[shader_type];
   uint32_t *surface_state;
   int offset, count, i;
   bool skip = false;

   /* SURFACE_STATEs for constant buffers */
   switch (shader_type) {
   case PIPE_SHADER_VERTEX:
      if (DIRTY(CBUF)) {
         offset = ILO_VS_CONST_SURFACE(0);
         surface_state = &p->state.vs.SURFACE_STATE[offset];

         session->binding_table_vs_changed = true;
      }
      else {
         skip = true;
      }
      break;
   case PIPE_SHADER_FRAGMENT:
      if (DIRTY(CBUF)) {
         offset = ILO_WM_CONST_SURFACE(0);
         surface_state = &p->state.wm.SURFACE_STATE[offset];

         session->binding_table_fs_changed = true;
      }
      else {
         skip = true;
      }
      break;
   default:
      skip = true;
      break;
   }

   if (skip)
      return;

   count = util_last_bit(cbuf->enabled_mask);
   for (i = 0; i < count; i++) {
      if (cbuf->cso[i].resource) {
         surface_state[i] = p->gen6_SURFACE_STATE(p->dev,
               &cbuf->cso[i].surface, false, p->cp);
      }
      else {
         surface_state[i] = 0;
      }
   }

   memset(&surface_state[count], 0, (ILO_MAX_CONST_BUFFERS - count) * 4);

   if (count && session->num_surfaces[shader_type] < offset + count)
      session->num_surfaces[shader_type] = offset + count;
}

static void
gen6_pipeline_state_binding_tables(struct ilo_3d_pipeline *p,
                                   const struct ilo_context *ilo,
                                   int shader_type,
                                   struct gen6_pipeline_session *session)
{
   uint32_t *binding_table_state, *surface_state;
   int *binding_table_state_size, size;
   bool skip = false;

   /* BINDING_TABLE_STATE */
   switch (shader_type) {
   case PIPE_SHADER_VERTEX:
      surface_state = p->state.vs.SURFACE_STATE;
      binding_table_state = &p->state.vs.BINDING_TABLE_STATE;
      binding_table_state_size = &p->state.vs.BINDING_TABLE_STATE_size;

      skip = !session->binding_table_vs_changed;
      break;
   case PIPE_SHADER_GEOMETRY:
      surface_state = p->state.gs.SURFACE_STATE;
      binding_table_state = &p->state.gs.BINDING_TABLE_STATE;
      binding_table_state_size = &p->state.gs.BINDING_TABLE_STATE_size;

      skip = !session->binding_table_gs_changed;
      break;
   case PIPE_SHADER_FRAGMENT:
      surface_state = p->state.wm.SURFACE_STATE;
      binding_table_state = &p->state.wm.BINDING_TABLE_STATE;
      binding_table_state_size = &p->state.wm.BINDING_TABLE_STATE_size;

      skip = !session->binding_table_fs_changed;
      break;
   default:
      skip = true;
      break;
   }

   if (skip)
      return;

   /*
    * If we have seemingly less SURFACE_STATEs than before, it could be that
    * we did not touch those reside at the tail in this upload.  Loop over
    * them to figure out the real number of SURFACE_STATEs.
    */
   for (size = *binding_table_state_size;
         size > session->num_surfaces[shader_type]; size--) {
      if (surface_state[size - 1])
         break;
   }
   if (size < session->num_surfaces[shader_type])
      size = session->num_surfaces[shader_type];

   *binding_table_state = p->gen6_BINDING_TABLE_STATE(p->dev,
         surface_state, size, p->cp);
   *binding_table_state_size = size;
}

static void
gen6_pipeline_state_samplers(struct ilo_3d_pipeline *p,
                             const struct ilo_context *ilo,
                             int shader_type,
                             struct gen6_pipeline_session *session)
{
   const struct ilo_sampler_cso * const *samplers =
      ilo->sampler[shader_type].cso;
   const struct pipe_sampler_view * const *views =
      (const struct pipe_sampler_view **) ilo->view[shader_type].states;
   const int num_samplers = ilo->sampler[shader_type].count;
   const int num_views = ilo->view[shader_type].count;
   uint32_t *sampler_state, *border_color_state;
   bool emit_border_color = false;
   bool skip = false;

   /* SAMPLER_BORDER_COLOR_STATE and SAMPLER_STATE */
   switch (shader_type) {
   case PIPE_SHADER_VERTEX:
      if (DIRTY(SAMPLER_VS) || DIRTY(VIEW_VS)) {
         sampler_state = &p->state.vs.SAMPLER_STATE;
         border_color_state = p->state.vs.SAMPLER_BORDER_COLOR_STATE;

         if (DIRTY(SAMPLER_VS))
            emit_border_color = true;

         session->sampler_state_vs_changed = true;
      }
      else {
         skip = true;
      }
      break;
   case PIPE_SHADER_FRAGMENT:
      if (DIRTY(SAMPLER_FS) || DIRTY(VIEW_FS)) {
         sampler_state = &p->state.wm.SAMPLER_STATE;
         border_color_state = p->state.wm.SAMPLER_BORDER_COLOR_STATE;

         if (DIRTY(SAMPLER_FS))
            emit_border_color = true;

         session->sampler_state_fs_changed = true;
      }
      else {
         skip = true;
      }
      break;
   default:
      skip = true;
      break;
   }

   if (skip)
      return;

   if (emit_border_color) {
      int i;

      for (i = 0; i < num_samplers; i++) {
         border_color_state[i] = (samplers[i]) ?
            p->gen6_SAMPLER_BORDER_COLOR_STATE(p->dev,
                  samplers[i], p->cp) : 0;
      }
   }

   /* should we take the minimum of num_samplers and num_views? */
   *sampler_state = p->gen6_SAMPLER_STATE(p->dev,
         samplers, views,
         border_color_state,
         MIN2(num_samplers, num_views), p->cp);
}

static void
gen6_pipeline_state_pcb(struct ilo_3d_pipeline *p,
                        const struct ilo_context *ilo,
                        struct gen6_pipeline_session *session)
{
   /* push constant buffer for VS */
   if (DIRTY(VS) || DIRTY(CLIP)) {
      const int clip_state_size = (ilo->vs) ?
            ilo_shader_get_kernel_param(ilo->vs,
                  ILO_KERNEL_VS_PCB_UCP_SIZE) : 0;

      if (clip_state_size) {
         void *pcb;

         p->state.vs.PUSH_CONSTANT_BUFFER_size = clip_state_size;
         p->state.vs.PUSH_CONSTANT_BUFFER =
            p->gen6_push_constant_buffer(p->dev,
                  p->state.vs.PUSH_CONSTANT_BUFFER_size, &pcb, p->cp);

         memcpy(pcb, &ilo->clip, clip_state_size);
      }
      else {
         p->state.vs.PUSH_CONSTANT_BUFFER_size = 0;
         p->state.vs.PUSH_CONSTANT_BUFFER = 0;
      }

      session->pcb_state_vs_changed = true;
   }
}

#undef DIRTY

static void
gen6_pipeline_commands(struct ilo_3d_pipeline *p,
                       const struct ilo_context *ilo,
                       struct gen6_pipeline_session *session)
{
   /*
    * We try to keep the order of the commands match, as closely as possible,
    * that of the classic i965 driver.  It allows us to compare the command
    * streams easily.
    */
   gen6_pipeline_common_select(p, ilo, session);
   gen6_pipeline_gs_svbi(p, ilo, session);
   gen6_pipeline_common_sip(p, ilo, session);
   gen6_pipeline_vf_statistics(p, ilo, session);
   gen6_pipeline_common_base_address(p, ilo, session);
   gen6_pipeline_common_pointers_1(p, ilo, session);
   gen6_pipeline_common_urb(p, ilo, session);
   gen6_pipeline_common_pointers_2(p, ilo, session);
   gen6_pipeline_wm_multisample(p, ilo, session);
   gen6_pipeline_vs(p, ilo, session);
   gen6_pipeline_gs(p, ilo, session);
   gen6_pipeline_clip(p, ilo, session);
   gen6_pipeline_sf(p, ilo, session);
   gen6_pipeline_wm(p, ilo, session);
   gen6_pipeline_common_pointers_3(p, ilo, session);
   gen6_pipeline_wm_depth(p, ilo, session);
   gen6_pipeline_wm_raster(p, ilo, session);
   gen6_pipeline_sf_rect(p, ilo, session);
   gen6_pipeline_vf(p, ilo, session);
   gen6_pipeline_vf_draw(p, ilo, session);
}

void
gen6_pipeline_states(struct ilo_3d_pipeline *p,
                     const struct ilo_context *ilo,
                     struct gen6_pipeline_session *session)
{
   int shader_type;

   gen6_pipeline_state_viewports(p, ilo, session);
   gen6_pipeline_state_cc(p, ilo, session);
   gen6_pipeline_state_scissors(p, ilo, session);
   gen6_pipeline_state_pcb(p, ilo, session);

   /*
    * upload all SURAFCE_STATEs together so that we know there are minimal
    * paddings
    */
   gen6_pipeline_state_surfaces_rt(p, ilo, session);
   gen6_pipeline_state_surfaces_so(p, ilo, session);
   for (shader_type = 0; shader_type < PIPE_SHADER_TYPES; shader_type++) {
      gen6_pipeline_state_surfaces_view(p, ilo, shader_type, session);
      gen6_pipeline_state_surfaces_const(p, ilo, shader_type, session);
   }

   for (shader_type = 0; shader_type < PIPE_SHADER_TYPES; shader_type++) {
      gen6_pipeline_state_samplers(p, ilo, shader_type, session);
      /* this must be called after all SURFACE_STATEs are uploaded */
      gen6_pipeline_state_binding_tables(p, ilo, shader_type, session);
   }
}

void
gen6_pipeline_prepare(const struct ilo_3d_pipeline *p,
                      const struct ilo_context *ilo,
                      struct gen6_pipeline_session *session)
{
   memset(session, 0, sizeof(*session));
   session->pipe_dirty = ilo->dirty;
   session->reduced_prim = u_reduced_prim(ilo->draw->mode);

   /* available space before the session */
   session->init_cp_space = ilo_cp_space(p->cp);

   session->hw_ctx_changed =
      (p->invalidate_flags & ILO_3D_PIPELINE_INVALIDATE_HW);

   if (session->hw_ctx_changed) {
      /* these should be enough to make everything uploaded */
      session->batch_bo_changed = true;
      session->state_bo_changed = true;
      session->kernel_bo_changed = true;
      session->prim_changed = true;
      session->primitive_restart_changed = true;
   }
   else {
      /*
       * Any state that involves resources needs to be re-emitted when the
       * batch bo changed.  This is because we do not pin the resources and
       * their offsets (or existence) may change between batch buffers.
       *
       * Since we messed around with ILO_3D_PIPELINE_INVALIDATE_BATCH_BO in
       * handle_invalid_batch_bo(), use ILO_3D_PIPELINE_INVALIDATE_STATE_BO as
       * a temporary workaround.
       */
      session->batch_bo_changed =
         (p->invalidate_flags & ILO_3D_PIPELINE_INVALIDATE_STATE_BO);

      session->state_bo_changed =
         (p->invalidate_flags & ILO_3D_PIPELINE_INVALIDATE_STATE_BO);
      session->kernel_bo_changed =
         (p->invalidate_flags & ILO_3D_PIPELINE_INVALIDATE_KERNEL_BO);
      session->prim_changed = (p->state.reduced_prim != session->reduced_prim);
      session->primitive_restart_changed =
         (p->state.primitive_restart != ilo->draw->primitive_restart);
   }
}

void
gen6_pipeline_draw(struct ilo_3d_pipeline *p,
                   const struct ilo_context *ilo,
                   struct gen6_pipeline_session *session)
{
   /* force all states to be uploaded if the state bo changed */
   if (session->state_bo_changed)
      session->pipe_dirty = ILO_DIRTY_ALL;
   else
      session->pipe_dirty = ilo->dirty;

   session->emit_draw_states(p, ilo, session);

   /* force all commands to be uploaded if the HW context changed */
   if (session->hw_ctx_changed)
      session->pipe_dirty = ILO_DIRTY_ALL;
   else
      session->pipe_dirty = ilo->dirty;

   session->emit_draw_commands(p, ilo, session);
}

void
gen6_pipeline_end(struct ilo_3d_pipeline *p,
                  const struct ilo_context *ilo,
                  struct gen6_pipeline_session *session)
{
   /* sanity check size estimation */
   assert(session->init_cp_space - ilo_cp_space(p->cp) <=
         ilo_3d_pipeline_estimate_size(p, ILO_3D_PIPELINE_DRAW, ilo));

   p->state.reduced_prim = session->reduced_prim;
   p->state.primitive_restart = ilo->draw->primitive_restart;
}

static void
ilo_3d_pipeline_emit_draw_gen6(struct ilo_3d_pipeline *p,
                               const struct ilo_context *ilo)
{
   struct gen6_pipeline_session session;

   gen6_pipeline_prepare(p, ilo, &session);

   session.emit_draw_states = gen6_pipeline_states;
   session.emit_draw_commands = gen6_pipeline_commands;

   gen6_pipeline_draw(p, ilo, &session);
   gen6_pipeline_end(p, ilo, &session);
}

void
ilo_3d_pipeline_emit_flush_gen6(struct ilo_3d_pipeline *p)
{
   if (p->dev->gen == ILO_GEN(6))
      gen6_wa_pipe_control_post_sync(p, false);

   p->gen6_PIPE_CONTROL(p->dev,
         PIPE_CONTROL_INSTRUCTION_FLUSH |
         PIPE_CONTROL_WRITE_FLUSH |
         PIPE_CONTROL_DEPTH_CACHE_FLUSH |
         PIPE_CONTROL_VF_CACHE_INVALIDATE |
         PIPE_CONTROL_TC_FLUSH |
         PIPE_CONTROL_NO_WRITE |
         PIPE_CONTROL_CS_STALL,
         0, 0, false, p->cp);
}

void
ilo_3d_pipeline_emit_write_timestamp_gen6(struct ilo_3d_pipeline *p,
                                          struct intel_bo *bo, int index)
{
   if (p->dev->gen == ILO_GEN(6))
      gen6_wa_pipe_control_post_sync(p, true);

   p->gen6_PIPE_CONTROL(p->dev,
         PIPE_CONTROL_WRITE_TIMESTAMP,
         bo, index * sizeof(uint64_t) | PIPE_CONTROL_GLOBAL_GTT_WRITE,
         true, p->cp);
}

void
ilo_3d_pipeline_emit_write_depth_count_gen6(struct ilo_3d_pipeline *p,
                                            struct intel_bo *bo, int index)
{
   if (p->dev->gen == ILO_GEN(6))
      gen6_wa_pipe_control_post_sync(p, false);

   p->gen6_PIPE_CONTROL(p->dev,
         PIPE_CONTROL_DEPTH_STALL |
         PIPE_CONTROL_WRITE_DEPTH_COUNT,
         bo, index * sizeof(uint64_t) | PIPE_CONTROL_GLOBAL_GTT_WRITE,
         true, p->cp);
}

static int
gen6_pipeline_estimate_commands(const struct ilo_3d_pipeline *p,
                                const struct ilo_gpe_gen6 *gen6,
                                const struct ilo_context *ilo)
{
   static int size;
   enum ilo_gpe_gen6_command cmd;

   if (size)
      return size;

   for (cmd = 0; cmd < ILO_GPE_GEN6_COMMAND_COUNT; cmd++) {
      int count;

      switch (cmd) {
      case ILO_GPE_GEN6_PIPE_CONTROL:
         /* for the workaround */
         count = 2;
         /* another one after 3DSTATE_URB */
         count += 1;
         /* and another one after 3DSTATE_CONSTANT_VS */
         count += 1;
         break;
      case ILO_GPE_GEN6_3DSTATE_GS_SVB_INDEX:
         /* there are 4 SVBIs */
         count = 4;
         break;
      case ILO_GPE_GEN6_3DSTATE_VERTEX_BUFFERS:
         count = 33;
         break;
      case ILO_GPE_GEN6_3DSTATE_VERTEX_ELEMENTS:
         count = 34;
         break;
      case ILO_GPE_GEN6_MEDIA_VFE_STATE:
      case ILO_GPE_GEN6_MEDIA_CURBE_LOAD:
      case ILO_GPE_GEN6_MEDIA_INTERFACE_DESCRIPTOR_LOAD:
      case ILO_GPE_GEN6_MEDIA_GATEWAY_STATE:
      case ILO_GPE_GEN6_MEDIA_STATE_FLUSH:
      case ILO_GPE_GEN6_MEDIA_OBJECT_WALKER:
         /* media commands */
         count = 0;
         break;
      default:
         count = 1;
         break;
      }

      if (count)
         size += gen6->estimate_command_size(p->dev, cmd, count);
   }

   return size;
}

static int
gen6_pipeline_estimate_states(const struct ilo_3d_pipeline *p,
                              const struct ilo_gpe_gen6 *gen6,
                              const struct ilo_context *ilo)
{
   static int static_size;
   int shader_type, count, size;

   if (!static_size) {
      struct {
         enum ilo_gpe_gen6_state state;
         int count;
      } static_states[] = {
         /* viewports */
         { ILO_GPE_GEN6_SF_VIEWPORT,                 1 },
         { ILO_GPE_GEN6_CLIP_VIEWPORT,               1 },
         { ILO_GPE_GEN6_CC_VIEWPORT,                 1 },
         /* cc */
         { ILO_GPE_GEN6_COLOR_CALC_STATE,            1 },
         { ILO_GPE_GEN6_BLEND_STATE,                 ILO_MAX_DRAW_BUFFERS },
         { ILO_GPE_GEN6_DEPTH_STENCIL_STATE,         1 },
         /* scissors */
         { ILO_GPE_GEN6_SCISSOR_RECT,                1 },
         /* binding table (vs, gs, fs) */
         { ILO_GPE_GEN6_BINDING_TABLE_STATE,         ILO_MAX_VS_SURFACES },
         { ILO_GPE_GEN6_BINDING_TABLE_STATE,         ILO_MAX_GS_SURFACES },
         { ILO_GPE_GEN6_BINDING_TABLE_STATE,         ILO_MAX_WM_SURFACES },
      };
      int i;

      for (i = 0; i < Elements(static_states); i++) {
         static_size += gen6->estimate_state_size(p->dev,
               static_states[i].state,
               static_states[i].count);
      }
   }

   size = static_size;

   /*
    * render targets (fs)
    * stream outputs (gs)
    * sampler views (vs, fs)
    * constant buffers (vs, fs)
    */
   count = ilo->fb.state.nr_cbufs;

   if (ilo->gs) {
      const struct pipe_stream_output_info *so_info =
         ilo_shader_get_kernel_so_info(ilo->gs);

      count += so_info->num_outputs;
   }
   else if (ilo->vs) {
      const struct pipe_stream_output_info *so_info =
         ilo_shader_get_kernel_so_info(ilo->vs);

      count += so_info->num_outputs;
   }

   for (shader_type = 0; shader_type < PIPE_SHADER_TYPES; shader_type++) {
      count += ilo->view[shader_type].count;
      count += util_bitcount(ilo->cbuf[shader_type].enabled_mask);
   }

   if (count) {
      size += gen6->estimate_state_size(p->dev,
            ILO_GPE_GEN6_SURFACE_STATE, count);
   }

   /* samplers (vs, fs) */
   for (shader_type = 0; shader_type < PIPE_SHADER_TYPES; shader_type++) {
      count = ilo->sampler[shader_type].count;
      if (count) {
         size += gen6->estimate_state_size(p->dev,
               ILO_GPE_GEN6_SAMPLER_BORDER_COLOR_STATE, count);
         size += gen6->estimate_state_size(p->dev,
               ILO_GPE_GEN6_SAMPLER_STATE, count);
      }
   }

   /* pcb (vs) */
   if (ilo->vs &&
       ilo_shader_get_kernel_param(ilo->vs, ILO_KERNEL_VS_PCB_UCP_SIZE)) {
      const int pcb_size =
         ilo_shader_get_kernel_param(ilo->vs, ILO_KERNEL_VS_PCB_UCP_SIZE);

      size += gen6->estimate_state_size(p->dev,
            ILO_GPE_GEN6_PUSH_CONSTANT_BUFFER, pcb_size);
   }

   return size;
}

static int
ilo_3d_pipeline_estimate_size_gen6(struct ilo_3d_pipeline *p,
                                   enum ilo_3d_pipeline_action action,
                                   const void *arg)
{
   const struct ilo_gpe_gen6 *gen6 = ilo_gpe_gen6_get();
   int size;

   switch (action) {
   case ILO_3D_PIPELINE_DRAW:
      {
         const struct ilo_context *ilo = arg;

         size = gen6_pipeline_estimate_commands(p, gen6, ilo) +
            gen6_pipeline_estimate_states(p, gen6, ilo);
      }
      break;
   case ILO_3D_PIPELINE_FLUSH:
      size = gen6->estimate_command_size(p->dev,
            ILO_GPE_GEN6_PIPE_CONTROL, 1) * 3;
      break;
   case ILO_3D_PIPELINE_WRITE_TIMESTAMP:
      size = gen6->estimate_command_size(p->dev,
            ILO_GPE_GEN6_PIPE_CONTROL, 1) * 2;
      break;
   case ILO_3D_PIPELINE_WRITE_DEPTH_COUNT:
      size = gen6->estimate_command_size(p->dev,
            ILO_GPE_GEN6_PIPE_CONTROL, 1) * 3;
      break;
   default:
      assert(!"unknown 3D pipeline action");
      size = 0;
      break;
   }

   return size;
}

void
ilo_3d_pipeline_init_gen6(struct ilo_3d_pipeline *p)
{
   const struct ilo_gpe_gen6 *gen6 = ilo_gpe_gen6_get();

   p->estimate_size = ilo_3d_pipeline_estimate_size_gen6;
   p->emit_draw = ilo_3d_pipeline_emit_draw_gen6;
   p->emit_flush = ilo_3d_pipeline_emit_flush_gen6;
   p->emit_write_timestamp = ilo_3d_pipeline_emit_write_timestamp_gen6;
   p->emit_write_depth_count = ilo_3d_pipeline_emit_write_depth_count_gen6;

#define GEN6_USE(p, name, from) \
   p->gen6_ ## name = from->emit_ ## name
   GEN6_USE(p, STATE_BASE_ADDRESS, gen6);
   GEN6_USE(p, STATE_SIP, gen6);
   GEN6_USE(p, PIPELINE_SELECT, gen6);
   GEN6_USE(p, 3DSTATE_BINDING_TABLE_POINTERS, gen6);
   GEN6_USE(p, 3DSTATE_SAMPLER_STATE_POINTERS, gen6);
   GEN6_USE(p, 3DSTATE_URB, gen6);
   GEN6_USE(p, 3DSTATE_VERTEX_BUFFERS, gen6);
   GEN6_USE(p, 3DSTATE_VERTEX_ELEMENTS, gen6);
   GEN6_USE(p, 3DSTATE_INDEX_BUFFER, gen6);
   GEN6_USE(p, 3DSTATE_VF_STATISTICS, gen6);
   GEN6_USE(p, 3DSTATE_VIEWPORT_STATE_POINTERS, gen6);
   GEN6_USE(p, 3DSTATE_CC_STATE_POINTERS, gen6);
   GEN6_USE(p, 3DSTATE_SCISSOR_STATE_POINTERS, gen6);
   GEN6_USE(p, 3DSTATE_VS, gen6);
   GEN6_USE(p, 3DSTATE_GS, gen6);
   GEN6_USE(p, 3DSTATE_CLIP, gen6);
   GEN6_USE(p, 3DSTATE_SF, gen6);
   GEN6_USE(p, 3DSTATE_WM, gen6);
   GEN6_USE(p, 3DSTATE_CONSTANT_VS, gen6);
   GEN6_USE(p, 3DSTATE_CONSTANT_GS, gen6);
   GEN6_USE(p, 3DSTATE_CONSTANT_PS, gen6);
   GEN6_USE(p, 3DSTATE_SAMPLE_MASK, gen6);
   GEN6_USE(p, 3DSTATE_DRAWING_RECTANGLE, gen6);
   GEN6_USE(p, 3DSTATE_DEPTH_BUFFER, gen6);
   GEN6_USE(p, 3DSTATE_POLY_STIPPLE_OFFSET, gen6);
   GEN6_USE(p, 3DSTATE_POLY_STIPPLE_PATTERN, gen6);
   GEN6_USE(p, 3DSTATE_LINE_STIPPLE, gen6);
   GEN6_USE(p, 3DSTATE_AA_LINE_PARAMETERS, gen6);
   GEN6_USE(p, 3DSTATE_GS_SVB_INDEX, gen6);
   GEN6_USE(p, 3DSTATE_MULTISAMPLE, gen6);
   GEN6_USE(p, 3DSTATE_STENCIL_BUFFER, gen6);
   GEN6_USE(p, 3DSTATE_HIER_DEPTH_BUFFER, gen6);
   GEN6_USE(p, 3DSTATE_CLEAR_PARAMS, gen6);
   GEN6_USE(p, PIPE_CONTROL, gen6);
   GEN6_USE(p, 3DPRIMITIVE, gen6);
   GEN6_USE(p, INTERFACE_DESCRIPTOR_DATA, gen6);
   GEN6_USE(p, SF_VIEWPORT, gen6);
   GEN6_USE(p, CLIP_VIEWPORT, gen6);
   GEN6_USE(p, CC_VIEWPORT, gen6);
   GEN6_USE(p, COLOR_CALC_STATE, gen6);
   GEN6_USE(p, BLEND_STATE, gen6);
   GEN6_USE(p, DEPTH_STENCIL_STATE, gen6);
   GEN6_USE(p, SCISSOR_RECT, gen6);
   GEN6_USE(p, BINDING_TABLE_STATE, gen6);
   GEN6_USE(p, SURFACE_STATE, gen6);
   GEN6_USE(p, so_SURFACE_STATE, gen6);
   GEN6_USE(p, SAMPLER_STATE, gen6);
   GEN6_USE(p, SAMPLER_BORDER_COLOR_STATE, gen6);
   GEN6_USE(p, push_constant_buffer, gen6);
#undef GEN6_USE
}

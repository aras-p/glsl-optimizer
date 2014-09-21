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
#include "util/u_prim.h"

#include "ilo_3d.h"
#include "ilo_blitter.h"
#include "ilo_builder_3d.h"
#include "ilo_builder_mi.h"
#include "ilo_builder_render.h"
#include "ilo_cp.h"
#include "ilo_query.h"
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
   assert(ilo_dev_gen(p->dev) == ILO_GEN(6));

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
   gen6_PIPE_CONTROL(p->builder,
         GEN6_PIPE_CONTROL_CS_STALL |
         GEN6_PIPE_CONTROL_PIXEL_SCOREBOARD_STALL,
         NULL, 0, false);

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
   gen6_PIPE_CONTROL(p->builder,
         GEN6_PIPE_CONTROL_WRITE_IMM,
         p->workaround_bo, 0, false);
}

static void
gen6_wa_pipe_control_wm_multisample_flush(struct ilo_3d_pipeline *p)
{
   assert(ilo_dev_gen(p->dev) == ILO_GEN(6));

   gen6_wa_pipe_control_post_sync(p, false);

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 305:
    *
    *     "Driver must guarentee that all the caches in the depth pipe are
    *      flushed before this command (3DSTATE_MULTISAMPLE) is parsed. This
    *      requires driver to send a PIPE_CONTROL with a CS stall along with a
    *      Depth Flush prior to this command."
    */
   gen6_PIPE_CONTROL(p->builder,
         GEN6_PIPE_CONTROL_DEPTH_CACHE_FLUSH |
         GEN6_PIPE_CONTROL_CS_STALL,
         0, 0, false);
}

static void
gen6_wa_pipe_control_wm_depth_flush(struct ilo_3d_pipeline *p)
{
   assert(ilo_dev_gen(p->dev) == ILO_GEN(6));

   gen6_wa_pipe_control_post_sync(p, false);

   /*
    * According to intel_emit_depth_stall_flushes() of classic i965, we need
    * to emit a sequence of PIPE_CONTROLs prior to emitting depth related
    * commands.
    */
   gen6_PIPE_CONTROL(p->builder,
         GEN6_PIPE_CONTROL_DEPTH_STALL,
         NULL, 0, false);

   gen6_PIPE_CONTROL(p->builder,
         GEN6_PIPE_CONTROL_DEPTH_CACHE_FLUSH,
         NULL, 0, false);

   gen6_PIPE_CONTROL(p->builder,
         GEN6_PIPE_CONTROL_DEPTH_STALL,
         NULL, 0, false);
}

static void
gen6_wa_pipe_control_wm_max_threads_stall(struct ilo_3d_pipeline *p)
{
   assert(ilo_dev_gen(p->dev) == ILO_GEN(6));

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
   gen6_PIPE_CONTROL(p->builder,
         GEN6_PIPE_CONTROL_PIXEL_SCOREBOARD_STALL,
         NULL, 0, false);

}

static void
gen6_wa_pipe_control_vs_const_flush(struct ilo_3d_pipeline *p)
{
   assert(ilo_dev_gen(p->dev) == ILO_GEN(6));

   gen6_wa_pipe_control_post_sync(p, false);

   /*
    * According to upload_vs_state() of classic i965, we need to emit
    * PIPE_CONTROL after 3DSTATE_CONSTANT_VS so that the command is kept being
    * buffered by VS FF, to the point that the FF dies.
    */
   gen6_PIPE_CONTROL(p->builder,
         GEN6_PIPE_CONTROL_DEPTH_STALL |
         GEN6_PIPE_CONTROL_INSTRUCTION_CACHE_INVALIDATE |
         GEN6_PIPE_CONTROL_STATE_CACHE_INVALIDATE,
         NULL, 0, false);
}

#define DIRTY(state) (session->pipe_dirty & ILO_DIRTY_ ## state)

void
gen6_pipeline_common_select(struct ilo_3d_pipeline *p,
                            const struct ilo_state_vector *vec,
                            struct gen6_pipeline_session *session)
{
   /* PIPELINE_SELECT */
   if (session->hw_ctx_changed) {
      if (ilo_dev_gen(p->dev) == ILO_GEN(6))
         gen6_wa_pipe_control_post_sync(p, false);

      gen6_PIPELINE_SELECT(p->builder, 0x0);
   }
}

void
gen6_pipeline_common_sip(struct ilo_3d_pipeline *p,
                         const struct ilo_state_vector *vec,
                         struct gen6_pipeline_session *session)
{
   /* STATE_SIP */
   if (session->hw_ctx_changed) {
      if (ilo_dev_gen(p->dev) == ILO_GEN(6))
         gen6_wa_pipe_control_post_sync(p, false);

      gen6_STATE_SIP(p->builder, 0);
   }
}

void
gen6_pipeline_common_base_address(struct ilo_3d_pipeline *p,
                                  const struct ilo_state_vector *vec,
                                  struct gen6_pipeline_session *session)
{
   /* STATE_BASE_ADDRESS */
   if (session->state_bo_changed || session->kernel_bo_changed ||
       session->batch_bo_changed) {
      if (ilo_dev_gen(p->dev) == ILO_GEN(6))
         gen6_wa_pipe_control_post_sync(p, false);

      gen6_state_base_address(p->builder, session->hw_ctx_changed);

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
                         const struct ilo_state_vector *vec,
                         struct gen6_pipeline_session *session)
{
   /* 3DSTATE_URB */
   if (DIRTY(VE) || DIRTY(VS) || DIRTY(GS)) {
      const bool gs_active = (vec->gs || (vec->vs &&
               ilo_shader_get_kernel_param(vec->vs, ILO_KERNEL_VS_GEN6_SO)));
      int vs_entry_size, gs_entry_size;
      int vs_total_size, gs_total_size;

      vs_entry_size = (vec->vs) ?
         ilo_shader_get_kernel_param(vec->vs, ILO_KERNEL_OUTPUT_COUNT) : 0;

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
      if (vs_entry_size < vec->ve->count)
         vs_entry_size = vec->ve->count;

      gs_entry_size = (vec->gs) ?
         ilo_shader_get_kernel_param(vec->gs, ILO_KERNEL_OUTPUT_COUNT) :
         (gs_active) ? vs_entry_size : 0;

      /* in bytes */
      vs_entry_size *= sizeof(float) * 4;
      gs_entry_size *= sizeof(float) * 4;
      vs_total_size = p->dev->urb_size;

      if (gs_active) {
         vs_total_size /= 2;
         gs_total_size = vs_total_size;
      }
      else {
         gs_total_size = 0;
      }

      gen6_3DSTATE_URB(p->builder, vs_total_size, gs_total_size,
            vs_entry_size, gs_entry_size);

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
                                const struct ilo_state_vector *vec,
                                struct gen6_pipeline_session *session)
{
   /* 3DSTATE_VIEWPORT_STATE_POINTERS */
   if (session->viewport_state_changed) {
      gen6_3DSTATE_VIEWPORT_STATE_POINTERS(p->builder,
            p->state.CLIP_VIEWPORT,
            p->state.SF_VIEWPORT,
            p->state.CC_VIEWPORT);
   }
}

static void
gen6_pipeline_common_pointers_2(struct ilo_3d_pipeline *p,
                                const struct ilo_state_vector *vec,
                                struct gen6_pipeline_session *session)
{
   /* 3DSTATE_CC_STATE_POINTERS */
   if (session->cc_state_blend_changed ||
       session->cc_state_dsa_changed ||
       session->cc_state_cc_changed) {
      gen6_3DSTATE_CC_STATE_POINTERS(p->builder,
            p->state.BLEND_STATE,
            p->state.DEPTH_STENCIL_STATE,
            p->state.COLOR_CALC_STATE);
   }

   /* 3DSTATE_SAMPLER_STATE_POINTERS */
   if (session->sampler_state_vs_changed ||
       session->sampler_state_gs_changed ||
       session->sampler_state_fs_changed) {
      gen6_3DSTATE_SAMPLER_STATE_POINTERS(p->builder,
            p->state.vs.SAMPLER_STATE,
            0,
            p->state.wm.SAMPLER_STATE);
   }
}

static void
gen6_pipeline_common_pointers_3(struct ilo_3d_pipeline *p,
                                const struct ilo_state_vector *vec,
                                struct gen6_pipeline_session *session)
{
   /* 3DSTATE_SCISSOR_STATE_POINTERS */
   if (session->scissor_state_changed) {
      gen6_3DSTATE_SCISSOR_STATE_POINTERS(p->builder,
            p->state.SCISSOR_RECT);
   }

   /* 3DSTATE_BINDING_TABLE_POINTERS */
   if (session->binding_table_vs_changed ||
       session->binding_table_gs_changed ||
       session->binding_table_fs_changed) {
      gen6_3DSTATE_BINDING_TABLE_POINTERS(p->builder,
            p->state.vs.BINDING_TABLE_STATE,
            p->state.gs.BINDING_TABLE_STATE,
            p->state.wm.BINDING_TABLE_STATE);
   }
}

void
gen6_pipeline_vf(struct ilo_3d_pipeline *p,
                 const struct ilo_state_vector *vec,
                 struct gen6_pipeline_session *session)
{
   if (ilo_dev_gen(p->dev) >= ILO_GEN(7.5)) {
      /* 3DSTATE_INDEX_BUFFER */
      if (DIRTY(IB) || session->batch_bo_changed) {
         gen6_3DSTATE_INDEX_BUFFER(p->builder,
               &vec->ib, false);
      }

      /* 3DSTATE_VF */
      if (session->primitive_restart_changed) {
         gen7_3DSTATE_VF(p->builder, vec->draw->primitive_restart,
               vec->draw->restart_index);
      }
   }
   else {
      /* 3DSTATE_INDEX_BUFFER */
      if (DIRTY(IB) || session->primitive_restart_changed ||
          session->batch_bo_changed) {
         gen6_3DSTATE_INDEX_BUFFER(p->builder,
               &vec->ib, vec->draw->primitive_restart);
      }
   }

   /* 3DSTATE_VERTEX_BUFFERS */
   if (DIRTY(VB) || DIRTY(VE) || session->batch_bo_changed)
      gen6_3DSTATE_VERTEX_BUFFERS(p->builder, vec->ve, &vec->vb);

   /* 3DSTATE_VERTEX_ELEMENTS */
   if (DIRTY(VE) || DIRTY(VS)) {
      const struct ilo_ve_state *ve = vec->ve;
      bool last_velement_edgeflag = false;
      bool prepend_generate_ids = false;

      if (vec->vs) {
         if (ilo_shader_get_kernel_param(vec->vs,
                  ILO_KERNEL_VS_INPUT_EDGEFLAG)) {
            /* we rely on the state tracker here */
            assert(ilo_shader_get_kernel_param(vec->vs,
                     ILO_KERNEL_INPUT_COUNT) == ve->count);

            last_velement_edgeflag = true;
         }

         if (ilo_shader_get_kernel_param(vec->vs,
                  ILO_KERNEL_VS_INPUT_INSTANCEID) ||
             ilo_shader_get_kernel_param(vec->vs,
                  ILO_KERNEL_VS_INPUT_VERTEXID))
            prepend_generate_ids = true;
      }

      gen6_3DSTATE_VERTEX_ELEMENTS(p->builder, ve,
            last_velement_edgeflag, prepend_generate_ids);
   }
}

void
gen6_pipeline_vf_statistics(struct ilo_3d_pipeline *p,
                            const struct ilo_state_vector *vec,
                            struct gen6_pipeline_session *session)
{
   /* 3DSTATE_VF_STATISTICS */
   if (session->hw_ctx_changed)
      gen6_3DSTATE_VF_STATISTICS(p->builder, false);
}

static void
gen6_pipeline_vf_draw(struct ilo_3d_pipeline *p,
                      const struct ilo_state_vector *vec,
                      struct gen6_pipeline_session *session)
{
   /* 3DPRIMITIVE */
   gen6_3DPRIMITIVE(p->builder, vec->draw, &vec->ib);
   p->state.has_gen6_wa_pipe_control = false;
}

void
gen6_pipeline_vs(struct ilo_3d_pipeline *p,
                 const struct ilo_state_vector *vec,
                 struct gen6_pipeline_session *session)
{
   const bool emit_3dstate_vs = (DIRTY(VS) || DIRTY(SAMPLER_VS) ||
                                 session->kernel_bo_changed);
   const bool emit_3dstate_constant_vs = session->pcb_state_vs_changed;

   /*
    * the classic i965 does this in upload_vs_state(), citing a spec that I
    * cannot find
    */
   if (emit_3dstate_vs && ilo_dev_gen(p->dev) == ILO_GEN(6))
      gen6_wa_pipe_control_post_sync(p, false);

   /* 3DSTATE_CONSTANT_VS */
   if (emit_3dstate_constant_vs) {
      gen6_3DSTATE_CONSTANT_VS(p->builder,
            &p->state.vs.PUSH_CONSTANT_BUFFER,
            &p->state.vs.PUSH_CONSTANT_BUFFER_size,
            1);
   }

   /* 3DSTATE_VS */
   if (emit_3dstate_vs) {
      const int num_samplers = vec->sampler[PIPE_SHADER_VERTEX].count;

      gen6_3DSTATE_VS(p->builder, vec->vs, num_samplers);
   }

   if (emit_3dstate_constant_vs && ilo_dev_gen(p->dev) == ILO_GEN(6))
      gen6_wa_pipe_control_vs_const_flush(p);
}

static void
gen6_pipeline_gs(struct ilo_3d_pipeline *p,
                 const struct ilo_state_vector *vec,
                 struct gen6_pipeline_session *session)
{
   /* 3DSTATE_CONSTANT_GS */
   if (session->pcb_state_gs_changed)
      gen6_3DSTATE_CONSTANT_GS(p->builder, NULL, NULL, 0);

   /* 3DSTATE_GS */
   if (DIRTY(GS) || DIRTY(VS) ||
       session->prim_changed || session->kernel_bo_changed) {
      const int verts_per_prim = u_vertices_per_prim(session->reduced_prim);

      gen6_3DSTATE_GS(p->builder, vec->gs, vec->vs, verts_per_prim);
   }
}

bool
gen6_pipeline_update_max_svbi(struct ilo_3d_pipeline *p,
                              const struct ilo_state_vector *vec,
                              struct gen6_pipeline_session *session)
{
   if (DIRTY(VS) || DIRTY(GS) || DIRTY(SO)) {
      const struct pipe_stream_output_info *so_info =
         (vec->gs) ? ilo_shader_get_kernel_so_info(vec->gs) :
         (vec->vs) ? ilo_shader_get_kernel_so_info(vec->vs) : NULL;
      unsigned max_svbi = 0xffffffff;
      int i;

      for (i = 0; i < so_info->num_outputs; i++) {
         const int output_buffer = so_info->output[i].output_buffer;
         const struct pipe_stream_output_target *so =
            vec->so.states[output_buffer];
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
                      const struct ilo_state_vector *vec,
                      struct gen6_pipeline_session *session)
{
   const bool emit = gen6_pipeline_update_max_svbi(p, vec, session);

   /* 3DSTATE_GS_SVB_INDEX */
   if (emit) {
      if (ilo_dev_gen(p->dev) == ILO_GEN(6))
         gen6_wa_pipe_control_post_sync(p, false);

      gen6_3DSTATE_GS_SVB_INDEX(p->builder,
            0, p->state.so_num_vertices, p->state.so_max_vertices,
            false);

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
            gen6_3DSTATE_GS_SVB_INDEX(p->builder,
                  i, 0, 0xffffffff, false);
         }
      }
   }
}

void
gen6_pipeline_clip(struct ilo_3d_pipeline *p,
                   const struct ilo_state_vector *vec,
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
      for (i = 0; i < vec->viewport.count; i++) {
         const struct ilo_viewport_cso *vp = &vec->viewport.cso[i];

         if (vp->min_x > 0.0f || vp->max_x < vec->fb.state.width ||
             vp->min_y > 0.0f || vp->max_y < vec->fb.state.height) {
            enable_guardband = false;
            break;
         }
      }

      gen6_3DSTATE_CLIP(p->builder, vec->rasterizer,
            vec->fs, enable_guardband, 1);
   }
}

static void
gen6_pipeline_sf(struct ilo_3d_pipeline *p,
                 const struct ilo_state_vector *vec,
                 struct gen6_pipeline_session *session)
{
   /* 3DSTATE_SF */
   if (DIRTY(RASTERIZER) || DIRTY(FS))
      gen6_3DSTATE_SF(p->builder, vec->rasterizer, vec->fs);
}

void
gen6_pipeline_sf_rect(struct ilo_3d_pipeline *p,
                      const struct ilo_state_vector *vec,
                      struct gen6_pipeline_session *session)
{
   /* 3DSTATE_DRAWING_RECTANGLE */
   if (DIRTY(FB)) {
      if (ilo_dev_gen(p->dev) == ILO_GEN(6))
         gen6_wa_pipe_control_post_sync(p, false);

      gen6_3DSTATE_DRAWING_RECTANGLE(p->builder, 0, 0,
            vec->fb.state.width, vec->fb.state.height);
   }
}

static void
gen6_pipeline_wm(struct ilo_3d_pipeline *p,
                 const struct ilo_state_vector *vec,
                 struct gen6_pipeline_session *session)
{
   /* 3DSTATE_CONSTANT_PS */
   if (session->pcb_state_fs_changed) {
      gen6_3DSTATE_CONSTANT_PS(p->builder,
            &p->state.wm.PUSH_CONSTANT_BUFFER,
            &p->state.wm.PUSH_CONSTANT_BUFFER_size,
            1);
   }

   /* 3DSTATE_WM */
   if (DIRTY(FS) || DIRTY(SAMPLER_FS) || DIRTY(BLEND) || DIRTY(DSA) ||
       DIRTY(RASTERIZER) || session->kernel_bo_changed) {
      const int num_samplers = vec->sampler[PIPE_SHADER_FRAGMENT].count;
      const bool dual_blend = vec->blend->dual_blend;
      const bool cc_may_kill = (vec->dsa->dw_alpha ||
                                vec->blend->alpha_to_coverage);

      if (ilo_dev_gen(p->dev) == ILO_GEN(6) && session->hw_ctx_changed)
         gen6_wa_pipe_control_wm_max_threads_stall(p);

      gen6_3DSTATE_WM(p->builder, vec->fs, num_samplers,
            vec->rasterizer, dual_blend, cc_may_kill, 0);
   }
}

static void
gen6_pipeline_wm_multisample(struct ilo_3d_pipeline *p,
                             const struct ilo_state_vector *vec,
                             struct gen6_pipeline_session *session)
{
   /* 3DSTATE_MULTISAMPLE and 3DSTATE_SAMPLE_MASK */
   if (DIRTY(SAMPLE_MASK) || DIRTY(FB)) {
      const uint32_t *packed_sample_pos;

      packed_sample_pos = (vec->fb.num_samples > 1) ?
         &p->packed_sample_position_4x : &p->packed_sample_position_1x;

      if (ilo_dev_gen(p->dev) == ILO_GEN(6)) {
         gen6_wa_pipe_control_post_sync(p, false);
         gen6_wa_pipe_control_wm_multisample_flush(p);
      }

      gen6_3DSTATE_MULTISAMPLE(p->builder,
            vec->fb.num_samples, packed_sample_pos,
            vec->rasterizer->state.half_pixel_center);

      gen6_3DSTATE_SAMPLE_MASK(p->builder,
            (vec->fb.num_samples > 1) ? vec->sample_mask : 0x1);
   }
}

static void
gen6_pipeline_wm_depth(struct ilo_3d_pipeline *p,
                       const struct ilo_state_vector *vec,
                       struct gen6_pipeline_session *session)
{
   /* 3DSTATE_DEPTH_BUFFER and 3DSTATE_CLEAR_PARAMS */
   if (DIRTY(FB) || session->batch_bo_changed) {
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

      if (ilo_dev_gen(p->dev) == ILO_GEN(6)) {
         gen6_wa_pipe_control_post_sync(p, false);
         gen6_wa_pipe_control_wm_depth_flush(p);
      }

      gen6_3DSTATE_DEPTH_BUFFER(p->builder, zs);
      gen6_3DSTATE_HIER_DEPTH_BUFFER(p->builder, zs);
      gen6_3DSTATE_STENCIL_BUFFER(p->builder, zs);
      gen6_3DSTATE_CLEAR_PARAMS(p->builder, clear_params);
   }
}

void
gen6_pipeline_wm_raster(struct ilo_3d_pipeline *p,
                        const struct ilo_state_vector *vec,
                        struct gen6_pipeline_session *session)
{
   /* 3DSTATE_POLY_STIPPLE_PATTERN and 3DSTATE_POLY_STIPPLE_OFFSET */
   if ((DIRTY(RASTERIZER) || DIRTY(POLY_STIPPLE)) &&
       vec->rasterizer->state.poly_stipple_enable) {
      if (ilo_dev_gen(p->dev) == ILO_GEN(6))
         gen6_wa_pipe_control_post_sync(p, false);

      gen6_3DSTATE_POLY_STIPPLE_PATTERN(p->builder,
            &vec->poly_stipple);

      gen6_3DSTATE_POLY_STIPPLE_OFFSET(p->builder, 0, 0);
   }

   /* 3DSTATE_LINE_STIPPLE */
   if (DIRTY(RASTERIZER) && vec->rasterizer->state.line_stipple_enable) {
      if (ilo_dev_gen(p->dev) == ILO_GEN(6))
         gen6_wa_pipe_control_post_sync(p, false);

      gen6_3DSTATE_LINE_STIPPLE(p->builder,
            vec->rasterizer->state.line_stipple_pattern,
            vec->rasterizer->state.line_stipple_factor + 1);
   }

   /* 3DSTATE_AA_LINE_PARAMETERS */
   if (DIRTY(RASTERIZER) && vec->rasterizer->state.line_smooth) {
      if (ilo_dev_gen(p->dev) == ILO_GEN(6))
         gen6_wa_pipe_control_post_sync(p, false);

      gen6_3DSTATE_AA_LINE_PARAMETERS(p->builder);
   }
}

static void
gen6_pipeline_state_viewports(struct ilo_3d_pipeline *p,
                              const struct ilo_state_vector *vec,
                              struct gen6_pipeline_session *session)
{
   /* SF_CLIP_VIEWPORT and CC_VIEWPORT */
   if (ilo_dev_gen(p->dev) >= ILO_GEN(7) && DIRTY(VIEWPORT)) {
      p->state.SF_CLIP_VIEWPORT = gen7_SF_CLIP_VIEWPORT(p->builder,
            vec->viewport.cso, vec->viewport.count);

      p->state.CC_VIEWPORT = gen6_CC_VIEWPORT(p->builder,
            vec->viewport.cso, vec->viewport.count);

      session->viewport_state_changed = true;
   }
   /* SF_VIEWPORT, CLIP_VIEWPORT, and CC_VIEWPORT */
   else if (DIRTY(VIEWPORT)) {
      p->state.CLIP_VIEWPORT = gen6_CLIP_VIEWPORT(p->builder,
            vec->viewport.cso, vec->viewport.count);

      p->state.SF_VIEWPORT = gen6_SF_VIEWPORT(p->builder,
            vec->viewport.cso, vec->viewport.count);

      p->state.CC_VIEWPORT = gen6_CC_VIEWPORT(p->builder,
            vec->viewport.cso, vec->viewport.count);

      session->viewport_state_changed = true;
   }
}

static void
gen6_pipeline_state_cc(struct ilo_3d_pipeline *p,
                       const struct ilo_state_vector *vec,
                       struct gen6_pipeline_session *session)
{
   /* BLEND_STATE */
   if (DIRTY(BLEND) || DIRTY(FB) || DIRTY(DSA)) {
      p->state.BLEND_STATE = gen6_BLEND_STATE(p->builder,
            vec->blend, &vec->fb, vec->dsa);

      session->cc_state_blend_changed = true;
   }

   /* COLOR_CALC_STATE */
   if (DIRTY(DSA) || DIRTY(STENCIL_REF) || DIRTY(BLEND_COLOR)) {
      p->state.COLOR_CALC_STATE =
         gen6_COLOR_CALC_STATE(p->builder, &vec->stencil_ref,
               vec->dsa->alpha_ref, &vec->blend_color);

      session->cc_state_cc_changed = true;
   }

   /* DEPTH_STENCIL_STATE */
   if (DIRTY(DSA)) {
      p->state.DEPTH_STENCIL_STATE =
         gen6_DEPTH_STENCIL_STATE(p->builder, vec->dsa);

      session->cc_state_dsa_changed = true;
   }
}

static void
gen6_pipeline_state_scissors(struct ilo_3d_pipeline *p,
                             const struct ilo_state_vector *vec,
                             struct gen6_pipeline_session *session)
{
   /* SCISSOR_RECT */
   if (DIRTY(SCISSOR) || DIRTY(VIEWPORT)) {
      /* there should be as many scissors as there are viewports */
      p->state.SCISSOR_RECT = gen6_SCISSOR_RECT(p->builder,
            &vec->scissor, vec->viewport.count);

      session->scissor_state_changed = true;
   }
}

static void
gen6_pipeline_state_surfaces_rt(struct ilo_3d_pipeline *p,
                                const struct ilo_state_vector *vec,
                                struct gen6_pipeline_session *session)
{
   /* SURFACE_STATEs for render targets */
   if (DIRTY(FB)) {
      const struct ilo_fb_state *fb = &vec->fb;
      const int offset = ILO_WM_DRAW_SURFACE(0);
      uint32_t *surface_state = &p->state.wm.SURFACE_STATE[offset];
      int i;

      for (i = 0; i < fb->state.nr_cbufs; i++) {
         const struct ilo_surface_cso *surface =
            (const struct ilo_surface_cso *) fb->state.cbufs[i];

         if (!surface) {
            surface_state[i] =
               gen6_SURFACE_STATE(p->builder, &fb->null_rt, true);
         }
         else {
            assert(surface && surface->is_rt);
            surface_state[i] =
               gen6_SURFACE_STATE(p->builder, &surface->u.rt, true);
         }
      }

      /*
       * Upload at least one render target, as
       * brw_update_renderbuffer_surfaces() does.  I don't know why.
       */
      if (i == 0) {
         surface_state[i] =
            gen6_SURFACE_STATE(p->builder, &fb->null_rt, true);

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
                                const struct ilo_state_vector *vec,
                                struct gen6_pipeline_session *session)
{
   const struct ilo_so_state *so = &vec->so;

   if (ilo_dev_gen(p->dev) != ILO_GEN(6))
      return;

   /* SURFACE_STATEs for stream output targets */
   if (DIRTY(VS) || DIRTY(GS) || DIRTY(SO)) {
      const struct pipe_stream_output_info *so_info =
         (vec->gs) ? ilo_shader_get_kernel_so_info(vec->gs) :
         (vec->vs) ? ilo_shader_get_kernel_so_info(vec->vs) : NULL;
      const int offset = ILO_GS_SO_SURFACE(0);
      uint32_t *surface_state = &p->state.gs.SURFACE_STATE[offset];
      int i;

      for (i = 0; so_info && i < so_info->num_outputs; i++) {
         const int target = so_info->output[i].output_buffer;
         const struct pipe_stream_output_target *so_target =
            (target < so->count) ? so->states[target] : NULL;

         if (so_target) {
            surface_state[i] = gen6_so_SURFACE_STATE(p->builder,
                  so_target, so_info, i);
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
                                  const struct ilo_state_vector *vec,
                                  int shader_type,
                                  struct gen6_pipeline_session *session)
{
   const struct ilo_view_state *view = &vec->view[shader_type];
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
            gen6_SURFACE_STATE(p->builder, &cso->surface, false);
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
                                   const struct ilo_state_vector *vec,
                                   int shader_type,
                                   struct gen6_pipeline_session *session)
{
   const struct ilo_cbuf_state *cbuf = &vec->cbuf[shader_type];
   uint32_t *surface_state;
   bool *binding_table_changed;
   int offset, count, i;

   if (!DIRTY(CBUF))
      return;

   /* SURFACE_STATEs for constant buffers */
   switch (shader_type) {
   case PIPE_SHADER_VERTEX:
      offset = ILO_VS_CONST_SURFACE(0);
      surface_state = &p->state.vs.SURFACE_STATE[offset];
      binding_table_changed = &session->binding_table_vs_changed;
      break;
   case PIPE_SHADER_FRAGMENT:
      offset = ILO_WM_CONST_SURFACE(0);
      surface_state = &p->state.wm.SURFACE_STATE[offset];
      binding_table_changed = &session->binding_table_fs_changed;
      break;
   default:
      return;
      break;
   }

   /* constants are pushed via PCB */
   if (cbuf->enabled_mask == 0x1 && !cbuf->cso[0].resource) {
      memset(surface_state, 0, ILO_MAX_CONST_BUFFERS * 4);
      return;
   }

   count = util_last_bit(cbuf->enabled_mask);
   for (i = 0; i < count; i++) {
      if (cbuf->cso[i].resource) {
         surface_state[i] = gen6_SURFACE_STATE(p->builder,
               &cbuf->cso[i].surface, false);
      }
      else {
         surface_state[i] = 0;
      }
   }

   memset(&surface_state[count], 0, (ILO_MAX_CONST_BUFFERS - count) * 4);

   if (count && session->num_surfaces[shader_type] < offset + count)
      session->num_surfaces[shader_type] = offset + count;

   *binding_table_changed = true;
}

static void
gen6_pipeline_state_binding_tables(struct ilo_3d_pipeline *p,
                                   const struct ilo_state_vector *vec,
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

   *binding_table_state = gen6_BINDING_TABLE_STATE(p->builder,
         surface_state, size);
   *binding_table_state_size = size;
}

static void
gen6_pipeline_state_samplers(struct ilo_3d_pipeline *p,
                             const struct ilo_state_vector *vec,
                             int shader_type,
                             struct gen6_pipeline_session *session)
{
   const struct ilo_sampler_cso * const *samplers =
      vec->sampler[shader_type].cso;
   const struct pipe_sampler_view * const *views =
      (const struct pipe_sampler_view **) vec->view[shader_type].states;
   const int num_samplers = vec->sampler[shader_type].count;
   const int num_views = vec->view[shader_type].count;
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
            gen6_SAMPLER_BORDER_COLOR_STATE(p->builder, samplers[i]) : 0;
      }
   }

   /* should we take the minimum of num_samplers and num_views? */
   *sampler_state = gen6_SAMPLER_STATE(p->builder,
         samplers, views,
         border_color_state,
         MIN2(num_samplers, num_views));
}

static void
gen6_pipeline_state_pcb(struct ilo_3d_pipeline *p,
                        const struct ilo_state_vector *vec,
                        struct gen6_pipeline_session *session)
{
   /* push constant buffer for VS */
   if (DIRTY(VS) || DIRTY(CBUF) || DIRTY(CLIP)) {
      const int cbuf0_size = (vec->vs) ?
            ilo_shader_get_kernel_param(vec->vs,
                  ILO_KERNEL_PCB_CBUF0_SIZE) : 0;
      const int clip_state_size = (vec->vs) ?
            ilo_shader_get_kernel_param(vec->vs,
                  ILO_KERNEL_VS_PCB_UCP_SIZE) : 0;
      const int total_size = cbuf0_size + clip_state_size;

      if (total_size) {
         void *pcb;

         p->state.vs.PUSH_CONSTANT_BUFFER =
            gen6_push_constant_buffer(p->builder, total_size, &pcb);
         p->state.vs.PUSH_CONSTANT_BUFFER_size = total_size;

         if (cbuf0_size) {
            const struct ilo_cbuf_state *cbuf =
               &vec->cbuf[PIPE_SHADER_VERTEX];

            if (cbuf0_size <= cbuf->cso[0].user_buffer_size) {
               memcpy(pcb, cbuf->cso[0].user_buffer, cbuf0_size);
            }
            else {
               memcpy(pcb, cbuf->cso[0].user_buffer,
                     cbuf->cso[0].user_buffer_size);
               memset(pcb + cbuf->cso[0].user_buffer_size, 0,
                     cbuf0_size - cbuf->cso[0].user_buffer_size);
            }

            pcb += cbuf0_size;
         }

         if (clip_state_size)
            memcpy(pcb, &vec->clip, clip_state_size);

         session->pcb_state_vs_changed = true;
      }
      else if (p->state.vs.PUSH_CONSTANT_BUFFER_size) {
         p->state.vs.PUSH_CONSTANT_BUFFER = 0;
         p->state.vs.PUSH_CONSTANT_BUFFER_size = 0;

         session->pcb_state_vs_changed = true;
      }
   }

   /* push constant buffer for FS */
   if (DIRTY(FS) || DIRTY(CBUF)) {
      const int cbuf0_size = (vec->fs) ?
         ilo_shader_get_kernel_param(vec->fs, ILO_KERNEL_PCB_CBUF0_SIZE) : 0;

      if (cbuf0_size) {
         const struct ilo_cbuf_state *cbuf = &vec->cbuf[PIPE_SHADER_FRAGMENT];
         void *pcb;

         p->state.wm.PUSH_CONSTANT_BUFFER =
            gen6_push_constant_buffer(p->builder, cbuf0_size, &pcb);
         p->state.wm.PUSH_CONSTANT_BUFFER_size = cbuf0_size;

         if (cbuf0_size <= cbuf->cso[0].user_buffer_size) {
            memcpy(pcb, cbuf->cso[0].user_buffer, cbuf0_size);
         }
         else {
            memcpy(pcb, cbuf->cso[0].user_buffer,
                  cbuf->cso[0].user_buffer_size);
            memset(pcb + cbuf->cso[0].user_buffer_size, 0,
                  cbuf0_size - cbuf->cso[0].user_buffer_size);
         }

         session->pcb_state_fs_changed = true;
      }
      else if (p->state.wm.PUSH_CONSTANT_BUFFER_size) {
         p->state.wm.PUSH_CONSTANT_BUFFER = 0;
         p->state.wm.PUSH_CONSTANT_BUFFER_size = 0;

         session->pcb_state_fs_changed = true;
      }
   }
}

#undef DIRTY

static void
gen6_pipeline_commands(struct ilo_3d_pipeline *p,
                       const struct ilo_state_vector *vec,
                       struct gen6_pipeline_session *session)
{
   /*
    * We try to keep the order of the commands match, as closely as possible,
    * that of the classic i965 driver.  It allows us to compare the command
    * streams easily.
    */
   gen6_pipeline_common_select(p, vec, session);
   gen6_pipeline_gs_svbi(p, vec, session);
   gen6_pipeline_common_sip(p, vec, session);
   gen6_pipeline_vf_statistics(p, vec, session);
   gen6_pipeline_common_base_address(p, vec, session);
   gen6_pipeline_common_pointers_1(p, vec, session);
   gen6_pipeline_common_urb(p, vec, session);
   gen6_pipeline_common_pointers_2(p, vec, session);
   gen6_pipeline_wm_multisample(p, vec, session);
   gen6_pipeline_vs(p, vec, session);
   gen6_pipeline_gs(p, vec, session);
   gen6_pipeline_clip(p, vec, session);
   gen6_pipeline_sf(p, vec, session);
   gen6_pipeline_wm(p, vec, session);
   gen6_pipeline_common_pointers_3(p, vec, session);
   gen6_pipeline_wm_depth(p, vec, session);
   gen6_pipeline_wm_raster(p, vec, session);
   gen6_pipeline_sf_rect(p, vec, session);
   gen6_pipeline_vf(p, vec, session);
   gen6_pipeline_vf_draw(p, vec, session);
}

void
gen6_pipeline_states(struct ilo_3d_pipeline *p,
                     const struct ilo_state_vector *vec,
                     struct gen6_pipeline_session *session)
{
   int shader_type;

   gen6_pipeline_state_viewports(p, vec, session);
   gen6_pipeline_state_cc(p, vec, session);
   gen6_pipeline_state_scissors(p, vec, session);
   gen6_pipeline_state_pcb(p, vec, session);

   /*
    * upload all SURAFCE_STATEs together so that we know there are minimal
    * paddings
    */
   gen6_pipeline_state_surfaces_rt(p, vec, session);
   gen6_pipeline_state_surfaces_so(p, vec, session);
   for (shader_type = 0; shader_type < PIPE_SHADER_TYPES; shader_type++) {
      gen6_pipeline_state_surfaces_view(p, vec, shader_type, session);
      gen6_pipeline_state_surfaces_const(p, vec, shader_type, session);
   }

   for (shader_type = 0; shader_type < PIPE_SHADER_TYPES; shader_type++) {
      gen6_pipeline_state_samplers(p, vec, shader_type, session);
      /* this must be called after all SURFACE_STATEs are uploaded */
      gen6_pipeline_state_binding_tables(p, vec, shader_type, session);
   }
}

void
gen6_pipeline_prepare(const struct ilo_3d_pipeline *p,
                      const struct ilo_state_vector *vec,
                      struct gen6_pipeline_session *session)
{
   memset(session, 0, sizeof(*session));
   session->pipe_dirty = vec->dirty;
   session->reduced_prim = u_reduced_prim(vec->draw->mode);

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
         (p->state.primitive_restart != vec->draw->primitive_restart);
   }
}

void
gen6_pipeline_draw(struct ilo_3d_pipeline *p,
                   const struct ilo_state_vector *vec,
                   struct gen6_pipeline_session *session)
{
   /* force all states to be uploaded if the state bo changed */
   if (session->state_bo_changed)
      session->pipe_dirty = ILO_DIRTY_ALL;
   else
      session->pipe_dirty = vec->dirty;

   session->emit_draw_states(p, vec, session);

   /* force all commands to be uploaded if the HW context changed */
   if (session->hw_ctx_changed)
      session->pipe_dirty = ILO_DIRTY_ALL;
   else
      session->pipe_dirty = vec->dirty;

   session->emit_draw_commands(p, vec, session);
}

void
gen6_pipeline_end(struct ilo_3d_pipeline *p,
                  const struct ilo_state_vector *vec,
                  struct gen6_pipeline_session *session)
{
   /* sanity check size estimation */
   assert(session->init_cp_space - ilo_cp_space(p->cp) <=
         ilo_3d_pipeline_estimate_size(p, ILO_3D_PIPELINE_DRAW, vec));

   p->state.reduced_prim = session->reduced_prim;
   p->state.primitive_restart = vec->draw->primitive_restart;
}

static void
ilo_3d_pipeline_emit_draw_gen6(struct ilo_3d_pipeline *p,
                               const struct ilo_state_vector *vec)
{
   struct gen6_pipeline_session session;

   gen6_pipeline_prepare(p, vec, &session);

   session.emit_draw_states = gen6_pipeline_states;
   session.emit_draw_commands = gen6_pipeline_commands;

   gen6_pipeline_draw(p, vec, &session);
   gen6_pipeline_end(p, vec, &session);
}

void
ilo_3d_pipeline_emit_flush_gen6(struct ilo_3d_pipeline *p)
{
   if (ilo_dev_gen(p->dev) == ILO_GEN(6))
      gen6_wa_pipe_control_post_sync(p, false);

   gen6_PIPE_CONTROL(p->builder,
         GEN6_PIPE_CONTROL_INSTRUCTION_CACHE_INVALIDATE |
         GEN6_PIPE_CONTROL_RENDER_CACHE_FLUSH |
         GEN6_PIPE_CONTROL_DEPTH_CACHE_FLUSH |
         GEN6_PIPE_CONTROL_VF_CACHE_INVALIDATE |
         GEN6_PIPE_CONTROL_TEXTURE_CACHE_INVALIDATE |
         GEN6_PIPE_CONTROL_WRITE_NONE |
         GEN6_PIPE_CONTROL_CS_STALL,
         0, 0, false);
}

void
ilo_3d_pipeline_emit_query_gen6(struct ilo_3d_pipeline *p,
                                struct ilo_query *q, uint32_t offset)
{
   const uint32_t pipeline_statistics_regs[] = {
      GEN6_REG_IA_VERTICES_COUNT,
      GEN6_REG_IA_PRIMITIVES_COUNT,
      GEN6_REG_VS_INVOCATION_COUNT,
      GEN6_REG_GS_INVOCATION_COUNT,
      GEN6_REG_GS_PRIMITIVES_COUNT,
      GEN6_REG_CL_INVOCATION_COUNT,
      GEN6_REG_CL_PRIMITIVES_COUNT,
      GEN6_REG_PS_INVOCATION_COUNT,
      (ilo_dev_gen(p->dev) >= ILO_GEN(7)) ? GEN7_REG_HS_INVOCATION_COUNT : 0,
      (ilo_dev_gen(p->dev) >= ILO_GEN(7)) ? GEN7_REG_DS_INVOCATION_COUNT : 0,
      0,
   };
   const uint32_t *regs;
   int reg_count = 0, i;

   ILO_DEV_ASSERT(p->dev, 6, 7.5);

   switch (q->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
      if (ilo_dev_gen(p->dev) == ILO_GEN(6))
         gen6_wa_pipe_control_post_sync(p, false);

      gen6_PIPE_CONTROL(p->builder,
            GEN6_PIPE_CONTROL_DEPTH_STALL |
            GEN6_PIPE_CONTROL_WRITE_PS_DEPTH_COUNT,
            q->bo, offset, true);
      break;
   case PIPE_QUERY_TIMESTAMP:
   case PIPE_QUERY_TIME_ELAPSED:
      if (ilo_dev_gen(p->dev) == ILO_GEN(6))
         gen6_wa_pipe_control_post_sync(p, true);

      gen6_PIPE_CONTROL(p->builder,
            GEN6_PIPE_CONTROL_WRITE_TIMESTAMP,
            q->bo, offset, true);
      break;
   case PIPE_QUERY_PIPELINE_STATISTICS:
      regs = pipeline_statistics_regs;
      reg_count = Elements(pipeline_statistics_regs);
      break;
   default:
      break;
   }

   if (!reg_count)
      return;

   p->emit_flush(p);

   for (i = 0; i < reg_count; i++) {
      if (regs[i]) {
         /* store lower 32 bits */
         gen6_MI_STORE_REGISTER_MEM(p->builder, q->bo, offset, regs[i]);
         /* store higher 32 bits */
         gen6_MI_STORE_REGISTER_MEM(p->builder, q->bo,
               offset + 4, regs[i] + 4);
      } else {
         gen6_MI_STORE_DATA_IMM(p->builder, q->bo, offset, 0, true);
      }

      offset += 8;
   }
}

static void
gen6_rectlist_vs_to_sf(struct ilo_3d_pipeline *p,
                       const struct ilo_blitter *blitter,
                       struct gen6_rectlist_session *session)
{
   gen6_3DSTATE_CONSTANT_VS(p->builder, NULL, NULL, 0);
   gen6_3DSTATE_VS(p->builder, NULL, 0);

   gen6_wa_pipe_control_vs_const_flush(p);

   gen6_3DSTATE_CONSTANT_GS(p->builder, NULL, NULL, 0);
   gen6_3DSTATE_GS(p->builder, NULL, NULL, 0);

   gen6_3DSTATE_CLIP(p->builder, NULL, NULL, false, 0);
   gen6_3DSTATE_SF(p->builder, NULL, NULL);
}

static void
gen6_rectlist_wm(struct ilo_3d_pipeline *p,
                 const struct ilo_blitter *blitter,
                 struct gen6_rectlist_session *session)
{
   uint32_t hiz_op;

   switch (blitter->op) {
   case ILO_BLITTER_RECTLIST_CLEAR_ZS:
      hiz_op = GEN6_WM_DW4_DEPTH_CLEAR;
      break;
   case ILO_BLITTER_RECTLIST_RESOLVE_Z:
      hiz_op = GEN6_WM_DW4_DEPTH_RESOLVE;
      break;
   case ILO_BLITTER_RECTLIST_RESOLVE_HIZ:
      hiz_op = GEN6_WM_DW4_HIZ_RESOLVE;
      break;
   default:
      hiz_op = 0;
      break;
   }

   gen6_3DSTATE_CONSTANT_PS(p->builder, NULL, NULL, 0);

   gen6_wa_pipe_control_wm_max_threads_stall(p);
   gen6_3DSTATE_WM(p->builder, NULL, 0, NULL, false, false, hiz_op);
}

static void
gen6_rectlist_wm_depth(struct ilo_3d_pipeline *p,
                       const struct ilo_blitter *blitter,
                       struct gen6_rectlist_session *session)
{
   gen6_wa_pipe_control_wm_depth_flush(p);

   if (blitter->uses & (ILO_BLITTER_USE_FB_DEPTH |
                        ILO_BLITTER_USE_FB_STENCIL)) {
      gen6_3DSTATE_DEPTH_BUFFER(p->builder,
            &blitter->fb.dst.u.zs);
   }

   if (blitter->uses & ILO_BLITTER_USE_FB_DEPTH) {
      gen6_3DSTATE_HIER_DEPTH_BUFFER(p->builder,
            &blitter->fb.dst.u.zs);
   }

   if (blitter->uses & ILO_BLITTER_USE_FB_STENCIL) {
      gen6_3DSTATE_STENCIL_BUFFER(p->builder,
            &blitter->fb.dst.u.zs);
   }

   gen6_3DSTATE_CLEAR_PARAMS(p->builder,
         blitter->depth_clear_value);
}

static void
gen6_rectlist_wm_multisample(struct ilo_3d_pipeline *p,
                             const struct ilo_blitter *blitter,
                             struct gen6_rectlist_session *session)
{
   const uint32_t *packed_sample_pos = (blitter->fb.num_samples > 1) ?
      &p->packed_sample_position_4x : &p->packed_sample_position_1x;

   gen6_wa_pipe_control_wm_multisample_flush(p);

   gen6_3DSTATE_MULTISAMPLE(p->builder, blitter->fb.num_samples,
         packed_sample_pos, true);

   gen6_3DSTATE_SAMPLE_MASK(p->builder,
         (1 << blitter->fb.num_samples) - 1);
}

static void
gen6_rectlist_commands(struct ilo_3d_pipeline *p,
                       const struct ilo_blitter *blitter,
                       struct gen6_rectlist_session *session)
{
   gen6_wa_pipe_control_post_sync(p, false);

   gen6_rectlist_wm_multisample(p, blitter, session);

   gen6_state_base_address(p->builder, true);

   gen6_3DSTATE_VERTEX_BUFFERS(p->builder,
         &blitter->ve, &blitter->vb);

   gen6_3DSTATE_VERTEX_ELEMENTS(p->builder,
         &blitter->ve, false, false);

   gen6_3DSTATE_URB(p->builder,
         p->dev->urb_size, 0, blitter->ve.count * 4 * sizeof(float), 0);
   /* 3DSTATE_URB workaround */
   if (p->state.gs.active) {
      ilo_3d_pipeline_emit_flush_gen6(p);
      p->state.gs.active = false;
   }

   if (blitter->uses &
       (ILO_BLITTER_USE_DSA | ILO_BLITTER_USE_CC)) {
      gen6_3DSTATE_CC_STATE_POINTERS(p->builder, 0,
            session->DEPTH_STENCIL_STATE, session->COLOR_CALC_STATE);
   }

   gen6_rectlist_vs_to_sf(p, blitter, session);
   gen6_rectlist_wm(p, blitter, session);

   if (blitter->uses & ILO_BLITTER_USE_VIEWPORT) {
      gen6_3DSTATE_VIEWPORT_STATE_POINTERS(p->builder,
            0, 0, session->CC_VIEWPORT);
   }

   gen6_rectlist_wm_depth(p, blitter, session);

   gen6_3DSTATE_DRAWING_RECTANGLE(p->builder, 0, 0,
         blitter->fb.width, blitter->fb.height);

   gen6_3DPRIMITIVE(p->builder, &blitter->draw, NULL);
}

static void
gen6_rectlist_states(struct ilo_3d_pipeline *p,
                     const struct ilo_blitter *blitter,
                     struct gen6_rectlist_session *session)
{
   if (blitter->uses & ILO_BLITTER_USE_DSA) {
      session->DEPTH_STENCIL_STATE =
         gen6_DEPTH_STENCIL_STATE(p->builder, &blitter->dsa);
   }

   if (blitter->uses & ILO_BLITTER_USE_CC) {
      session->COLOR_CALC_STATE =
         gen6_COLOR_CALC_STATE(p->builder, &blitter->cc.stencil_ref,
               blitter->cc.alpha_ref, &blitter->cc.blend_color);
   }

   if (blitter->uses & ILO_BLITTER_USE_VIEWPORT) {
      session->CC_VIEWPORT =
         gen6_CC_VIEWPORT(p->builder, &blitter->viewport, 1);
   }
}

static void
ilo_3d_pipeline_emit_rectlist_gen6(struct ilo_3d_pipeline *p,
                                   const struct ilo_blitter *blitter)
{
   struct gen6_rectlist_session session;

   memset(&session, 0, sizeof(session));
   gen6_rectlist_states(p, blitter, &session);
   gen6_rectlist_commands(p, blitter, &session);
}

static int
gen6_pipeline_max_command_size(const struct ilo_3d_pipeline *p)
{
   static int size;

   if (!size) {
      size += GEN6_3DSTATE_CONSTANT_ANY__SIZE * 3;
      size += GEN6_3DSTATE_GS_SVB_INDEX__SIZE * 4;
      size += GEN6_PIPE_CONTROL__SIZE * 5;

      size +=
         GEN6_STATE_BASE_ADDRESS__SIZE +
         GEN6_STATE_SIP__SIZE +
         GEN6_3DSTATE_VF_STATISTICS__SIZE +
         GEN6_PIPELINE_SELECT__SIZE +
         GEN6_3DSTATE_BINDING_TABLE_POINTERS__SIZE +
         GEN6_3DSTATE_SAMPLER_STATE_POINTERS__SIZE +
         GEN6_3DSTATE_URB__SIZE +
         GEN6_3DSTATE_VERTEX_BUFFERS__SIZE +
         GEN6_3DSTATE_VERTEX_ELEMENTS__SIZE +
         GEN6_3DSTATE_INDEX_BUFFER__SIZE +
         GEN6_3DSTATE_VIEWPORT_STATE_POINTERS__SIZE +
         GEN6_3DSTATE_CC_STATE_POINTERS__SIZE +
         GEN6_3DSTATE_SCISSOR_STATE_POINTERS__SIZE +
         GEN6_3DSTATE_VS__SIZE +
         GEN6_3DSTATE_GS__SIZE +
         GEN6_3DSTATE_CLIP__SIZE +
         GEN6_3DSTATE_SF__SIZE +
         GEN6_3DSTATE_WM__SIZE +
         GEN6_3DSTATE_SAMPLE_MASK__SIZE +
         GEN6_3DSTATE_DRAWING_RECTANGLE__SIZE +
         GEN6_3DSTATE_DEPTH_BUFFER__SIZE +
         GEN6_3DSTATE_POLY_STIPPLE_OFFSET__SIZE +
         GEN6_3DSTATE_POLY_STIPPLE_PATTERN__SIZE +
         GEN6_3DSTATE_LINE_STIPPLE__SIZE +
         GEN6_3DSTATE_AA_LINE_PARAMETERS__SIZE +
         GEN6_3DSTATE_MULTISAMPLE__SIZE +
         GEN6_3DSTATE_STENCIL_BUFFER__SIZE +
         GEN6_3DSTATE_HIER_DEPTH_BUFFER__SIZE +
         GEN6_3DSTATE_CLEAR_PARAMS__SIZE +
         GEN6_3DPRIMITIVE__SIZE;
   }

   return size;
}

int
gen6_pipeline_estimate_state_size(const struct ilo_3d_pipeline *p,
                                  const struct ilo_state_vector *vec)
{
   static int static_size;
   int sh_type, size;

   if (!static_size) {
      /* 64 bytes, or 16 dwords */
      const int alignment = 64 / 4;

      /* pad first */
      size = alignment - 1;

      /* CC states */
      size += align(GEN6_BLEND_STATE__SIZE * ILO_MAX_DRAW_BUFFERS, alignment);
      size += align(GEN6_DEPTH_STENCIL_STATE__SIZE, alignment);
      size += align(GEN6_COLOR_CALC_STATE__SIZE, alignment);

      /* viewport arrays */
      if (ilo_dev_gen(p->dev) >= ILO_GEN(7)) {
         size +=
            align(GEN7_SF_CLIP_VIEWPORT__SIZE * ILO_MAX_VIEWPORTS, 16) +
            align(GEN6_CC_VIEWPORT__SIZE * ILO_MAX_VIEWPORTS, 8) +
            align(GEN6_SCISSOR_RECT__SIZE * ILO_MAX_VIEWPORTS, 8);
      }
      else {
         size +=
            align(GEN6_SF_VIEWPORT__SIZE * ILO_MAX_VIEWPORTS, 8) +
            align(GEN6_CLIP_VIEWPORT__SIZE * ILO_MAX_VIEWPORTS, 8) +
            align(GEN6_CC_VIEWPORT__SIZE * ILO_MAX_VIEWPORTS, 8) +
            align(GEN6_SCISSOR_RECT__SIZE * ILO_MAX_VIEWPORTS, 8);
      }

      static_size = size;
   }

   size = static_size;

   for (sh_type = 0; sh_type < PIPE_SHADER_TYPES; sh_type++) {
      const int alignment = 32 / 4;
      int num_samplers, num_surfaces, pcb_size;

      /* samplers */
      num_samplers = vec->sampler[sh_type].count;

      /* sampler views and constant buffers */
      num_surfaces = vec->view[sh_type].count +
         util_bitcount(vec->cbuf[sh_type].enabled_mask);

      pcb_size = 0;

      switch (sh_type) {
      case PIPE_SHADER_VERTEX:
         if (vec->vs) {
            if (ilo_dev_gen(p->dev) == ILO_GEN(6)) {
               const struct pipe_stream_output_info *so_info =
                  ilo_shader_get_kernel_so_info(vec->vs);

               /* stream outputs */
               num_surfaces += so_info->num_outputs;
            }

            pcb_size = ilo_shader_get_kernel_param(vec->vs,
                  ILO_KERNEL_PCB_CBUF0_SIZE);
            pcb_size += ilo_shader_get_kernel_param(vec->vs,
                  ILO_KERNEL_VS_PCB_UCP_SIZE);
         }
         break;
      case PIPE_SHADER_GEOMETRY:
         if (vec->gs && ilo_dev_gen(p->dev) == ILO_GEN(6)) {
            const struct pipe_stream_output_info *so_info =
               ilo_shader_get_kernel_so_info(vec->gs);

            /* stream outputs */
            num_surfaces += so_info->num_outputs;
         }
         break;
      case PIPE_SHADER_FRAGMENT:
         /* render targets */
         num_surfaces += vec->fb.state.nr_cbufs;

         if (vec->fs) {
            pcb_size = ilo_shader_get_kernel_param(vec->fs,
                  ILO_KERNEL_PCB_CBUF0_SIZE);
         }
         break;
      default:
         break;
      }

      /* SAMPLER_STATE array and SAMPLER_BORDER_COLORs */
      if (num_samplers) {
         size += align(GEN6_SAMPLER_STATE__SIZE * num_samplers, alignment) +
            align(GEN6_SAMPLER_BORDER_COLOR__SIZE, alignment) * num_samplers;
      }

      /* BINDING_TABLE_STATE and SURFACE_STATEs */
      if (num_surfaces) {
         size += align(num_surfaces, alignment) +
            align(GEN6_SURFACE_STATE__SIZE, alignment) * num_surfaces;
      }

      /* PCB */
      if (pcb_size)
         size += align(pcb_size, alignment);
   }

   return size;
}

int
gen6_pipeline_estimate_query_size(const struct ilo_3d_pipeline *p,
                                  const struct ilo_query *q)
{
   int size;

   ILO_DEV_ASSERT(p->dev, 6, 7.5);

   switch (q->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
      size = GEN6_PIPE_CONTROL__SIZE;
      if (ilo_dev_gen(p->dev) == ILO_GEN(6))
         size *= 3;
      break;
   case PIPE_QUERY_TIMESTAMP:
   case PIPE_QUERY_TIME_ELAPSED:
      size = GEN6_PIPE_CONTROL__SIZE;
      if (ilo_dev_gen(p->dev) == ILO_GEN(6))
         size *= 2;
      break;
   case PIPE_QUERY_PIPELINE_STATISTICS:
      if (ilo_dev_gen(p->dev) >= ILO_GEN(7)) {
         const int num_regs = 10;
         const int num_pads = 1;

         size = GEN6_PIPE_CONTROL__SIZE +
            GEN6_MI_STORE_REGISTER_MEM__SIZE * 2 * num_regs +
            GEN6_MI_STORE_DATA_IMM__SIZE * num_pads;
      } else {
         const int num_regs = 8;
         const int num_pads = 3;

         size = GEN6_PIPE_CONTROL__SIZE * 3 +
            GEN6_MI_STORE_REGISTER_MEM__SIZE * 2 * num_regs +
            GEN6_MI_STORE_DATA_IMM__SIZE * num_pads;
      }
      break;
   default:
      size = 0;
      break;
   }

   return size;
}

static int
ilo_3d_pipeline_estimate_size_gen6(struct ilo_3d_pipeline *p,
                                   enum ilo_3d_pipeline_action action,
                                   const void *arg)
{
   int size;

   switch (action) {
   case ILO_3D_PIPELINE_DRAW:
      {
         const struct ilo_state_vector *ilo = arg;

         size = gen6_pipeline_max_command_size(p) +
            gen6_pipeline_estimate_state_size(p, ilo);
      }
      break;
   case ILO_3D_PIPELINE_FLUSH:
      size = GEN6_PIPE_CONTROL__SIZE * 3;
      break;
   case ILO_3D_PIPELINE_QUERY:
      size = gen6_pipeline_estimate_query_size(p,
            (const struct ilo_query *) arg);
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
ilo_3d_pipeline_init_gen6(struct ilo_3d_pipeline *p)
{
   p->estimate_size = ilo_3d_pipeline_estimate_size_gen6;
   p->emit_draw = ilo_3d_pipeline_emit_draw_gen6;
   p->emit_flush = ilo_3d_pipeline_emit_flush_gen6;
   p->emit_query = ilo_3d_pipeline_emit_query_gen6;
   p->emit_rectlist = ilo_3d_pipeline_emit_rectlist_gen6;
}

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

#include "ilo_blitter.h"
#include "ilo_builder_3d.h"
#include "ilo_builder_mi.h"
#include "ilo_builder_render.h"
#include "ilo_query.h"
#include "ilo_shader.h"
#include "ilo_state.h"
#include "ilo_render.h"
#include "ilo_render_gen.h"

/**
 * A wrapper for gen6_PIPE_CONTROL().
 */
static inline void
gen6_pipe_control(struct ilo_render *r, uint32_t dw1)
{
   struct intel_bo *bo = (dw1 & GEN6_PIPE_CONTROL_WRITE__MASK) ?
      r->workaround_bo : NULL;

   ILO_DEV_ASSERT(r->dev, 6, 6);

   gen6_PIPE_CONTROL(r->builder, dw1, bo, 0, false);

   r->state.current_pipe_control_dw1 |= dw1;

   assert(!r->state.deferred_pipe_control_dw1);
}

/**
 * This should be called before PIPE_CONTROL.
 */
void
gen6_wa_pre_pipe_control(struct ilo_render *r, uint32_t dw1)
{
   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 60:
    *
    *     "Pipe-control with CS-stall bit set must be sent BEFORE the
    *      pipe-control with a post-sync op and no write-cache flushes."
    *
    * This WA may also be triggered indirectly by the other two WAs on the
    * same page:
    *
    *     "Before any depth stall flush (including those produced by
    *      non-pipelined state commands), software needs to first send a
    *      PIPE_CONTROL with no bits set except Post-Sync Operation != 0."
    *
    *     "Before a PIPE_CONTROL with Write Cache Flush Enable =1, a
    *      PIPE_CONTROL with any non-zero post-sync-op is required."
    */
   const bool direct_wa_cond = (dw1 & GEN6_PIPE_CONTROL_WRITE__MASK) &&
                               !(dw1 & GEN6_PIPE_CONTROL_RENDER_CACHE_FLUSH);
   const bool indirect_wa_cond = (dw1 & GEN6_PIPE_CONTROL_DEPTH_STALL) |
                                 (dw1 & GEN6_PIPE_CONTROL_RENDER_CACHE_FLUSH);

   ILO_DEV_ASSERT(r->dev, 6, 6);

   if (!direct_wa_cond && !indirect_wa_cond)
      return;

   if (!(r->state.current_pipe_control_dw1 & GEN6_PIPE_CONTROL_CS_STALL)) {
      /*
       * From the Sandy Bridge PRM, volume 2 part 1, page 73:
       *
       *     "1 of the following must also be set (when CS stall is set):
       *
       *       - Depth Cache Flush Enable ([0] of DW1)
       *       - Stall at Pixel Scoreboard ([1] of DW1)
       *       - Depth Stall ([13] of DW1)
       *       - Post-Sync Operation ([13] of DW1)
       *       - Render Target Cache Flush Enable ([12] of DW1)
       *       - Notify Enable ([8] of DW1)"
       *
       * Because of the WAs above, we have to pick Stall at Pixel Scoreboard.
       */
      const uint32_t direct_wa = GEN6_PIPE_CONTROL_CS_STALL |
                                 GEN6_PIPE_CONTROL_PIXEL_SCOREBOARD_STALL;

      gen6_pipe_control(r, direct_wa);
   }

   if (indirect_wa_cond &&
       !(r->state.current_pipe_control_dw1 & GEN6_PIPE_CONTROL_WRITE__MASK)) {
      const uint32_t indirect_wa = GEN6_PIPE_CONTROL_WRITE_IMM;

      gen6_pipe_control(r, indirect_wa);
   }
}

/**
 * This should be called before any non-pipelined state command.
 */
static void
gen6_wa_pre_non_pipelined(struct ilo_render *r)
{
   ILO_DEV_ASSERT(r->dev, 6, 6);

   /* non-pipelined state commands produce depth stall */
   gen6_wa_pre_pipe_control(r, GEN6_PIPE_CONTROL_DEPTH_STALL);
}

static void
gen6_wa_post_3dstate_constant_vs(struct ilo_render *r)
{
   /*
    * According to upload_vs_state() of the classic driver, we need to emit a
    * PIPE_CONTROL after 3DSTATE_CONSTANT_VS, otherwise the command is kept
    * being buffered by VS FF, to the point that the FF dies.
    */
   const uint32_t dw1 = GEN6_PIPE_CONTROL_DEPTH_STALL |
                        GEN6_PIPE_CONTROL_INSTRUCTION_CACHE_INVALIDATE |
                        GEN6_PIPE_CONTROL_STATE_CACHE_INVALIDATE;

   gen6_wa_pre_pipe_control(r, dw1);

   if ((r->state.current_pipe_control_dw1 & dw1) != dw1)
      gen6_pipe_control(r, dw1);
}

static void
gen6_wa_pre_3dstate_wm_max_threads(struct ilo_render *r)
{
   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 274:
    *
    *     "A PIPE_CONTROL command, with only the Stall At Pixel Scoreboard
    *      field set (DW1 Bit 1), must be issued prior to any change to the
    *      value in this field (Maximum Number of Threads in 3DSTATE_WM)"
    */
   const uint32_t dw1 = GEN6_PIPE_CONTROL_PIXEL_SCOREBOARD_STALL;

   ILO_DEV_ASSERT(r->dev, 6, 6);

   gen6_wa_pre_pipe_control(r, dw1);

   if ((r->state.current_pipe_control_dw1 & dw1) != dw1)
      gen6_pipe_control(r, dw1);
}

static void
gen6_wa_pre_3dstate_multisample(struct ilo_render *r)
{
   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 305:
    *
    *     "Driver must guarentee that all the caches in the depth pipe are
    *      flushed before this command (3DSTATE_MULTISAMPLE) is parsed. This
    *      requires driver to send a PIPE_CONTROL with a CS stall along with a
    *      Depth Flush prior to this command."
    */
   const uint32_t dw1 = GEN6_PIPE_CONTROL_DEPTH_CACHE_FLUSH |
                        GEN6_PIPE_CONTROL_CS_STALL;

   ILO_DEV_ASSERT(r->dev, 6, 6);

   gen6_wa_pre_pipe_control(r, dw1);

   if ((r->state.current_pipe_control_dw1 & dw1) != dw1)
      gen6_pipe_control(r, dw1);
}

static void
gen6_wa_pre_depth(struct ilo_render *r)
{
   ILO_DEV_ASSERT(r->dev, 6, 6);

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
    *
    * According to the classic driver, it also applies for GEN6.
    */
   gen6_wa_pre_pipe_control(r, GEN6_PIPE_CONTROL_DEPTH_STALL |
                               GEN6_PIPE_CONTROL_DEPTH_CACHE_FLUSH);

   gen6_pipe_control(r, GEN6_PIPE_CONTROL_DEPTH_STALL);
   gen6_pipe_control(r, GEN6_PIPE_CONTROL_DEPTH_CACHE_FLUSH);
   gen6_pipe_control(r, GEN6_PIPE_CONTROL_DEPTH_STALL);
}

#define DIRTY(state) (session->pipe_dirty & ILO_DIRTY_ ## state)

void
gen6_draw_common_select(struct ilo_render *r,
                        const struct ilo_state_vector *vec,
                        struct gen6_draw_session *session)
{
   /* PIPELINE_SELECT */
   if (r->hw_ctx_changed) {
      if (ilo_dev_gen(r->dev) == ILO_GEN(6))
         gen6_wa_pre_non_pipelined(r);

      gen6_PIPELINE_SELECT(r->builder, 0x0);
   }
}

void
gen6_draw_common_sip(struct ilo_render *r,
                     const struct ilo_state_vector *vec,
                     struct gen6_draw_session *session)
{
   /* STATE_SIP */
   if (r->hw_ctx_changed) {
      if (ilo_dev_gen(r->dev) == ILO_GEN(6))
         gen6_wa_pre_non_pipelined(r);

      gen6_STATE_SIP(r->builder, 0);
   }
}

void
gen6_draw_common_base_address(struct ilo_render *r,
                              const struct ilo_state_vector *vec,
                              struct gen6_draw_session *session)
{
   /* STATE_BASE_ADDRESS */
   if (r->state_bo_changed || r->instruction_bo_changed ||
       r->batch_bo_changed) {
      if (ilo_dev_gen(r->dev) == ILO_GEN(6))
         gen6_wa_pre_non_pipelined(r);

      gen6_state_base_address(r->builder, r->hw_ctx_changed);

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
      session->viewport_changed = true;

      session->scissor_changed = true;

      session->blend_changed = true;
      session->dsa_changed = true;
      session->cc_changed = true;

      session->sampler_vs_changed = true;
      session->sampler_gs_changed = true;
      session->sampler_fs_changed = true;

      session->pcb_vs_changed = true;
      session->pcb_gs_changed = true;
      session->pcb_fs_changed = true;

      session->binding_table_vs_changed = true;
      session->binding_table_gs_changed = true;
      session->binding_table_fs_changed = true;
   }
}

static void
gen6_draw_common_urb(struct ilo_render *r,
                     const struct ilo_state_vector *vec,
                     struct gen6_draw_session *session)
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
      vs_total_size = r->dev->urb_size;

      if (gs_active) {
         vs_total_size /= 2;
         gs_total_size = vs_total_size;
      }
      else {
         gs_total_size = 0;
      }

      gen6_3DSTATE_URB(r->builder, vs_total_size, gs_total_size,
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
      if (r->state.gs.active && !gs_active)
         ilo_render_emit_flush(r);

      r->state.gs.active = gs_active;
   }
}

static void
gen6_draw_common_pointers_1(struct ilo_render *r,
                            const struct ilo_state_vector *vec,
                            struct gen6_draw_session *session)
{
   /* 3DSTATE_VIEWPORT_STATE_POINTERS */
   if (session->viewport_changed) {
      gen6_3DSTATE_VIEWPORT_STATE_POINTERS(r->builder,
            r->state.CLIP_VIEWPORT,
            r->state.SF_VIEWPORT,
            r->state.CC_VIEWPORT);
   }
}

static void
gen6_draw_common_pointers_2(struct ilo_render *r,
                            const struct ilo_state_vector *vec,
                            struct gen6_draw_session *session)
{
   /* 3DSTATE_CC_STATE_POINTERS */
   if (session->blend_changed ||
       session->dsa_changed ||
       session->cc_changed) {
      gen6_3DSTATE_CC_STATE_POINTERS(r->builder,
            r->state.BLEND_STATE,
            r->state.DEPTH_STENCIL_STATE,
            r->state.COLOR_CALC_STATE);
   }

   /* 3DSTATE_SAMPLER_STATE_POINTERS */
   if (session->sampler_vs_changed ||
       session->sampler_gs_changed ||
       session->sampler_fs_changed) {
      gen6_3DSTATE_SAMPLER_STATE_POINTERS(r->builder,
            r->state.vs.SAMPLER_STATE,
            0,
            r->state.wm.SAMPLER_STATE);
   }
}

static void
gen6_draw_common_pointers_3(struct ilo_render *r,
                            const struct ilo_state_vector *vec,
                            struct gen6_draw_session *session)
{
   /* 3DSTATE_SCISSOR_STATE_POINTERS */
   if (session->scissor_changed) {
      gen6_3DSTATE_SCISSOR_STATE_POINTERS(r->builder,
            r->state.SCISSOR_RECT);
   }

   /* 3DSTATE_BINDING_TABLE_POINTERS */
   if (session->binding_table_vs_changed ||
       session->binding_table_gs_changed ||
       session->binding_table_fs_changed) {
      gen6_3DSTATE_BINDING_TABLE_POINTERS(r->builder,
            r->state.vs.BINDING_TABLE_STATE,
            r->state.gs.BINDING_TABLE_STATE,
            r->state.wm.BINDING_TABLE_STATE);
   }
}

void
gen6_draw_vf(struct ilo_render *r,
             const struct ilo_state_vector *vec,
             struct gen6_draw_session *session)
{
   if (ilo_dev_gen(r->dev) >= ILO_GEN(7.5)) {
      /* 3DSTATE_INDEX_BUFFER */
      if (DIRTY(IB) || r->batch_bo_changed) {
         gen6_3DSTATE_INDEX_BUFFER(r->builder,
               &vec->ib, false);
      }

      /* 3DSTATE_VF */
      if (session->primitive_restart_changed) {
         gen7_3DSTATE_VF(r->builder, vec->draw->primitive_restart,
               vec->draw->restart_index);
      }
   }
   else {
      /* 3DSTATE_INDEX_BUFFER */
      if (DIRTY(IB) || session->primitive_restart_changed ||
          r->batch_bo_changed) {
         gen6_3DSTATE_INDEX_BUFFER(r->builder,
               &vec->ib, vec->draw->primitive_restart);
      }
   }

   /* 3DSTATE_VERTEX_BUFFERS */
   if (DIRTY(VB) || DIRTY(VE) || r->batch_bo_changed)
      gen6_3DSTATE_VERTEX_BUFFERS(r->builder, vec->ve, &vec->vb);

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

      gen6_3DSTATE_VERTEX_ELEMENTS(r->builder, ve,
            last_velement_edgeflag, prepend_generate_ids);
   }
}

void
gen6_draw_vf_statistics(struct ilo_render *r,
                        const struct ilo_state_vector *vec,
                        struct gen6_draw_session *session)
{
   /* 3DSTATE_VF_STATISTICS */
   if (r->hw_ctx_changed)
      gen6_3DSTATE_VF_STATISTICS(r->builder, false);
}

static void
gen6_draw_vf_draw(struct ilo_render *r,
                  const struct ilo_state_vector *vec,
                  struct gen6_draw_session *session)
{
   /* 3DPRIMITIVE */
   gen6_3DPRIMITIVE(r->builder, vec->draw, &vec->ib);

   r->state.current_pipe_control_dw1 = 0;
   assert(!r->state.deferred_pipe_control_dw1);
}

void
gen6_draw_vs(struct ilo_render *r,
             const struct ilo_state_vector *vec,
             struct gen6_draw_session *session)
{
   const bool emit_3dstate_vs = (DIRTY(VS) || DIRTY(SAMPLER_VS) ||
                                 r->instruction_bo_changed);
   const bool emit_3dstate_constant_vs = session->pcb_vs_changed;

   /*
    * the classic i965 does this in upload_vs_state(), citing a spec that I
    * cannot find
    */
   if (emit_3dstate_vs && ilo_dev_gen(r->dev) == ILO_GEN(6))
      gen6_wa_pre_non_pipelined(r);

   /* 3DSTATE_CONSTANT_VS */
   if (emit_3dstate_constant_vs) {
      gen6_3DSTATE_CONSTANT_VS(r->builder,
            &r->state.vs.PUSH_CONSTANT_BUFFER,
            &r->state.vs.PUSH_CONSTANT_BUFFER_size,
            1);
   }

   /* 3DSTATE_VS */
   if (emit_3dstate_vs) {
      const int num_samplers = vec->sampler[PIPE_SHADER_VERTEX].count;

      gen6_3DSTATE_VS(r->builder, vec->vs, num_samplers);
   }

   if (emit_3dstate_constant_vs && ilo_dev_gen(r->dev) == ILO_GEN(6))
      gen6_wa_post_3dstate_constant_vs(r);
}

static void
gen6_draw_gs(struct ilo_render *r,
             const struct ilo_state_vector *vec,
             struct gen6_draw_session *session)
{
   /* 3DSTATE_CONSTANT_GS */
   if (session->pcb_gs_changed)
      gen6_3DSTATE_CONSTANT_GS(r->builder, NULL, NULL, 0);

   /* 3DSTATE_GS */
   if (DIRTY(GS) || DIRTY(VS) ||
       session->prim_changed || r->instruction_bo_changed) {
      const int verts_per_prim = u_vertices_per_prim(session->reduced_prim);

      gen6_3DSTATE_GS(r->builder, vec->gs, vec->vs, verts_per_prim);
   }
}

static bool
gen6_draw_update_max_svbi(struct ilo_render *r,
                          const struct ilo_state_vector *vec,
                          struct gen6_draw_session *session)
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

      if (r->state.so_max_vertices != max_svbi) {
         r->state.so_max_vertices = max_svbi;
         return true;
      }
   }

   return false;
}

static void
gen6_draw_gs_svbi(struct ilo_render *r,
                  const struct ilo_state_vector *vec,
                  struct gen6_draw_session *session)
{
   const bool emit = gen6_draw_update_max_svbi(r, vec, session);

   /* 3DSTATE_GS_SVB_INDEX */
   if (emit) {
      if (ilo_dev_gen(r->dev) == ILO_GEN(6))
         gen6_wa_pre_non_pipelined(r);

      gen6_3DSTATE_GS_SVB_INDEX(r->builder,
            0, 0, r->state.so_max_vertices,
            false);

      if (r->hw_ctx_changed) {
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
            gen6_3DSTATE_GS_SVB_INDEX(r->builder,
                  i, 0, 0xffffffff, false);
         }
      }
   }
}

void
gen6_draw_clip(struct ilo_render *r,
               const struct ilo_state_vector *vec,
               struct gen6_draw_session *session)
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

      gen6_3DSTATE_CLIP(r->builder, vec->rasterizer,
            vec->fs, enable_guardband, 1);
   }
}

static void
gen6_draw_sf(struct ilo_render *r,
             const struct ilo_state_vector *vec,
             struct gen6_draw_session *session)
{
   /* 3DSTATE_SF */
   if (DIRTY(RASTERIZER) || DIRTY(FS))
      gen6_3DSTATE_SF(r->builder, vec->rasterizer, vec->fs);
}

void
gen6_draw_sf_rect(struct ilo_render *r,
                  const struct ilo_state_vector *vec,
                  struct gen6_draw_session *session)
{
   /* 3DSTATE_DRAWING_RECTANGLE */
   if (DIRTY(FB)) {
      if (ilo_dev_gen(r->dev) == ILO_GEN(6))
         gen6_wa_pre_non_pipelined(r);

      gen6_3DSTATE_DRAWING_RECTANGLE(r->builder, 0, 0,
            vec->fb.state.width, vec->fb.state.height);
   }
}

static void
gen6_draw_wm(struct ilo_render *r,
             const struct ilo_state_vector *vec,
             struct gen6_draw_session *session)
{
   /* 3DSTATE_CONSTANT_PS */
   if (session->pcb_fs_changed) {
      gen6_3DSTATE_CONSTANT_PS(r->builder,
            &r->state.wm.PUSH_CONSTANT_BUFFER,
            &r->state.wm.PUSH_CONSTANT_BUFFER_size,
            1);
   }

   /* 3DSTATE_WM */
   if (DIRTY(FS) || DIRTY(SAMPLER_FS) || DIRTY(BLEND) || DIRTY(DSA) ||
       DIRTY(RASTERIZER) || r->instruction_bo_changed) {
      const int num_samplers = vec->sampler[PIPE_SHADER_FRAGMENT].count;
      const bool dual_blend = vec->blend->dual_blend;
      const bool cc_may_kill = (vec->dsa->dw_alpha ||
                                vec->blend->alpha_to_coverage);

      if (ilo_dev_gen(r->dev) == ILO_GEN(6) && r->hw_ctx_changed)
         gen6_wa_pre_3dstate_wm_max_threads(r);

      gen6_3DSTATE_WM(r->builder, vec->fs, num_samplers,
            vec->rasterizer, dual_blend, cc_may_kill, 0);
   }
}

static void
gen6_draw_wm_multisample(struct ilo_render *r,
                         const struct ilo_state_vector *vec,
                         struct gen6_draw_session *session)
{
   /* 3DSTATE_MULTISAMPLE and 3DSTATE_SAMPLE_MASK */
   if (DIRTY(SAMPLE_MASK) || DIRTY(FB)) {
      const uint32_t *packed_sample_pos;

      packed_sample_pos = (vec->fb.num_samples > 1) ?
         &r->packed_sample_position_4x : &r->packed_sample_position_1x;

      if (ilo_dev_gen(r->dev) == ILO_GEN(6)) {
         gen6_wa_pre_non_pipelined(r);
         gen6_wa_pre_3dstate_multisample(r);
      }

      gen6_3DSTATE_MULTISAMPLE(r->builder,
            vec->fb.num_samples, packed_sample_pos,
            vec->rasterizer->state.half_pixel_center);

      gen6_3DSTATE_SAMPLE_MASK(r->builder,
            (vec->fb.num_samples > 1) ? vec->sample_mask : 0x1);
   }
}

static void
gen6_draw_wm_depth(struct ilo_render *r,
                   const struct ilo_state_vector *vec,
                   struct gen6_draw_session *session)
{
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

      if (ilo_dev_gen(r->dev) == ILO_GEN(6)) {
         gen6_wa_pre_non_pipelined(r);
         gen6_wa_pre_depth(r);
      }

      gen6_3DSTATE_DEPTH_BUFFER(r->builder, zs);
      gen6_3DSTATE_HIER_DEPTH_BUFFER(r->builder, zs);
      gen6_3DSTATE_STENCIL_BUFFER(r->builder, zs);
      gen6_3DSTATE_CLEAR_PARAMS(r->builder, clear_params);
   }
}

void
gen6_draw_wm_raster(struct ilo_render *r,
                    const struct ilo_state_vector *vec,
                    struct gen6_draw_session *session)
{
   /* 3DSTATE_POLY_STIPPLE_PATTERN and 3DSTATE_POLY_STIPPLE_OFFSET */
   if ((DIRTY(RASTERIZER) || DIRTY(POLY_STIPPLE)) &&
       vec->rasterizer->state.poly_stipple_enable) {
      if (ilo_dev_gen(r->dev) == ILO_GEN(6))
         gen6_wa_pre_non_pipelined(r);

      gen6_3DSTATE_POLY_STIPPLE_PATTERN(r->builder,
            &vec->poly_stipple);

      gen6_3DSTATE_POLY_STIPPLE_OFFSET(r->builder, 0, 0);
   }

   /* 3DSTATE_LINE_STIPPLE */
   if (DIRTY(RASTERIZER) && vec->rasterizer->state.line_stipple_enable) {
      if (ilo_dev_gen(r->dev) == ILO_GEN(6))
         gen6_wa_pre_non_pipelined(r);

      gen6_3DSTATE_LINE_STIPPLE(r->builder,
            vec->rasterizer->state.line_stipple_pattern,
            vec->rasterizer->state.line_stipple_factor + 1);
   }

   /* 3DSTATE_AA_LINE_PARAMETERS */
   if (DIRTY(RASTERIZER) && vec->rasterizer->state.line_smooth) {
      if (ilo_dev_gen(r->dev) == ILO_GEN(6))
         gen6_wa_pre_non_pipelined(r);

      gen6_3DSTATE_AA_LINE_PARAMETERS(r->builder);
   }
}

#undef DIRTY

static void
gen6_draw_commands(struct ilo_render *render,
                   const struct ilo_state_vector *vec,
                   struct gen6_draw_session *session)
{
   /*
    * We try to keep the order of the commands match, as closely as possible,
    * that of the classic i965 driver.  It allows us to compare the command
    * streams easily.
    */
   gen6_draw_common_select(render, vec, session);
   gen6_draw_gs_svbi(render, vec, session);
   gen6_draw_common_sip(render, vec, session);
   gen6_draw_vf_statistics(render, vec, session);
   gen6_draw_common_base_address(render, vec, session);
   gen6_draw_common_pointers_1(render, vec, session);
   gen6_draw_common_urb(render, vec, session);
   gen6_draw_common_pointers_2(render, vec, session);
   gen6_draw_wm_multisample(render, vec, session);
   gen6_draw_vs(render, vec, session);
   gen6_draw_gs(render, vec, session);
   gen6_draw_clip(render, vec, session);
   gen6_draw_sf(render, vec, session);
   gen6_draw_wm(render, vec, session);
   gen6_draw_common_pointers_3(render, vec, session);
   gen6_draw_wm_depth(render, vec, session);
   gen6_draw_wm_raster(render, vec, session);
   gen6_draw_sf_rect(render, vec, session);
   gen6_draw_vf(render, vec, session);
   gen6_draw_vf_draw(render, vec, session);
}

void
gen6_draw_prepare(struct ilo_render *render,
                  const struct ilo_state_vector *vec,
                  struct gen6_draw_session *session)
{
   memset(session, 0, sizeof(*session));
   session->pipe_dirty = vec->dirty;
   session->reduced_prim = u_reduced_prim(vec->draw->mode);

   if (render->hw_ctx_changed) {
      /* these should be enough to make everything uploaded */
      render->batch_bo_changed = true;
      render->state_bo_changed = true;
      render->instruction_bo_changed = true;

      session->prim_changed = true;
      session->primitive_restart_changed = true;
   } else {
      session->prim_changed =
         (render->state.reduced_prim != session->reduced_prim);
      session->primitive_restart_changed =
         (render->state.primitive_restart != vec->draw->primitive_restart);
   }
}

void
gen6_draw_emit(struct ilo_render *render,
               const struct ilo_state_vector *vec,
               struct gen6_draw_session *session)
{
   /* force all states to be uploaded if the state bo changed */
   if (render->state_bo_changed)
      session->pipe_dirty = ILO_DIRTY_ALL;
   else
      session->pipe_dirty = vec->dirty;

   ilo_render_emit_draw_dynamic_states(render, vec, session);
   ilo_render_emit_draw_surface_states(render, vec, session);

   /* force all commands to be uploaded if the HW context changed */
   if (render->hw_ctx_changed)
      session->pipe_dirty = ILO_DIRTY_ALL;
   else
      session->pipe_dirty = vec->dirty;

   session->emit_draw_commands(render, vec, session);
}

void
gen6_draw_end(struct ilo_render *render,
              const struct ilo_state_vector *vec,
              struct gen6_draw_session *session)
{
   render->hw_ctx_changed = false;

   render->batch_bo_changed = false;
   render->state_bo_changed = false;
   render->instruction_bo_changed = false;

   render->state.reduced_prim = session->reduced_prim;
   render->state.primitive_restart = vec->draw->primitive_restart;
}

static void
ilo_render_emit_draw_gen6(struct ilo_render *render,
                          const struct ilo_state_vector *vec)
{
   struct gen6_draw_session session;

   gen6_draw_prepare(render, vec, &session);

   session.emit_draw_commands = gen6_draw_commands;

   gen6_draw_emit(render, vec, &session);
   gen6_draw_end(render, vec, &session);
}

static void
gen6_rectlist_vs_to_sf(struct ilo_render *r,
                       const struct ilo_blitter *blitter)
{
   gen6_3DSTATE_CONSTANT_VS(r->builder, NULL, NULL, 0);
   gen6_3DSTATE_VS(r->builder, NULL, 0);

   gen6_wa_post_3dstate_constant_vs(r);

   gen6_3DSTATE_CONSTANT_GS(r->builder, NULL, NULL, 0);
   gen6_3DSTATE_GS(r->builder, NULL, NULL, 0);

   gen6_3DSTATE_CLIP(r->builder, NULL, NULL, false, 0);
   gen6_3DSTATE_SF(r->builder, NULL, NULL);
}

static void
gen6_rectlist_wm(struct ilo_render *r,
                 const struct ilo_blitter *blitter)
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

   gen6_3DSTATE_CONSTANT_PS(r->builder, NULL, NULL, 0);

   gen6_wa_pre_3dstate_wm_max_threads(r);
   gen6_3DSTATE_WM(r->builder, NULL, 0, NULL, false, false, hiz_op);
}

static void
gen6_rectlist_wm_depth(struct ilo_render *r,
                       const struct ilo_blitter *blitter)
{
   gen6_wa_pre_depth(r);

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

   gen6_3DSTATE_CLEAR_PARAMS(r->builder,
         blitter->depth_clear_value);
}

static void
gen6_rectlist_wm_multisample(struct ilo_render *r,
                             const struct ilo_blitter *blitter)
{
   const uint32_t *packed_sample_pos = (blitter->fb.num_samples > 1) ?
      &r->packed_sample_position_4x : &r->packed_sample_position_1x;

   gen6_wa_pre_3dstate_multisample(r);

   gen6_3DSTATE_MULTISAMPLE(r->builder, blitter->fb.num_samples,
         packed_sample_pos, true);

   gen6_3DSTATE_SAMPLE_MASK(r->builder,
         (1 << blitter->fb.num_samples) - 1);
}

int
ilo_render_get_rectlist_commands_len_gen6(const struct ilo_render *render,
                                          const struct ilo_blitter *blitter)
{
   ILO_DEV_ASSERT(render->dev, 6, 7.5);

   return 256;
}

void
ilo_render_emit_rectlist_commands_gen6(struct ilo_render *r,
                                       const struct ilo_blitter *blitter)
{
   ILO_DEV_ASSERT(r->dev, 6, 6);

   gen6_wa_pre_non_pipelined(r);

   gen6_rectlist_wm_multisample(r, blitter);

   gen6_state_base_address(r->builder, true);

   gen6_3DSTATE_VERTEX_BUFFERS(r->builder,
         &blitter->ve, &blitter->vb);

   gen6_3DSTATE_VERTEX_ELEMENTS(r->builder,
         &blitter->ve, false, false);

   gen6_3DSTATE_URB(r->builder,
         r->dev->urb_size, 0, blitter->ve.count * 4 * sizeof(float), 0);
   /* 3DSTATE_URB workaround */
   if (r->state.gs.active) {
      ilo_render_emit_flush(r);
      r->state.gs.active = false;
   }

   if (blitter->uses &
       (ILO_BLITTER_USE_DSA | ILO_BLITTER_USE_CC)) {
      gen6_3DSTATE_CC_STATE_POINTERS(r->builder, 0,
            r->state.DEPTH_STENCIL_STATE, r->state.COLOR_CALC_STATE);
   }

   gen6_rectlist_vs_to_sf(r, blitter);
   gen6_rectlist_wm(r, blitter);

   if (blitter->uses & ILO_BLITTER_USE_VIEWPORT) {
      gen6_3DSTATE_VIEWPORT_STATE_POINTERS(r->builder,
            0, 0, r->state.CC_VIEWPORT);
   }

   gen6_rectlist_wm_depth(r, blitter);

   gen6_3DSTATE_DRAWING_RECTANGLE(r->builder, 0, 0,
         blitter->fb.width, blitter->fb.height);

   gen6_3DPRIMITIVE(r->builder, &blitter->draw, NULL);
}

static int
gen6_render_max_command_size(const struct ilo_render *render)
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

static int
ilo_render_estimate_size_gen6(struct ilo_render *render,
                              enum ilo_render_action action,
                              const void *arg)
{
   int size;

   switch (action) {
   case ILO_RENDER_DRAW:
      {
         const struct ilo_state_vector *vec = arg;

         size = gen6_render_max_command_size(render) +
            ilo_render_get_draw_dynamic_states_len(render, vec) +
            ilo_render_get_draw_surface_states_len(render, vec);
      }
      break;
   default:
      assert(!"unknown render action");
      size = 0;
      break;
   }

   return size;
}

void
ilo_render_init_gen6(struct ilo_render *render)
{
   render->estimate_size = ilo_render_estimate_size_gen6;
   render->emit_draw = ilo_render_emit_draw_gen6;
}

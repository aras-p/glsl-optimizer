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

#ifndef ILO_3D_PIPELINE_GEN6_H
#define ILO_3D_PIPELINE_GEN6_H

#include "ilo_common.h"

struct ilo_3d_pipeline;
struct ilo_context;

struct gen6_pipeline_session {
   uint32_t pipe_dirty;

   int reduced_prim;
   int init_cp_space;

   bool hw_ctx_changed;
   bool batch_bo_changed;
   bool state_bo_changed;
   bool kernel_bo_changed;
   bool prim_changed;
   bool primitive_restart_changed;

   void (*emit_draw_states)(struct ilo_3d_pipeline *p,
                            const struct ilo_context *ilo,
                            struct gen6_pipeline_session *session);

   void (*emit_draw_commands)(struct ilo_3d_pipeline *p,
                              const struct ilo_context *ilo,
                              struct gen6_pipeline_session *session);

   /* indirect states */
   bool viewport_state_changed;
   bool cc_state_blend_changed;
   bool cc_state_dsa_changed;
   bool cc_state_cc_changed;
   bool scissor_state_changed;
   bool binding_table_vs_changed;
   bool binding_table_gs_changed;
   bool binding_table_fs_changed;
   bool sampler_state_vs_changed;
   bool sampler_state_gs_changed;
   bool sampler_state_fs_changed;
   bool pcb_state_vs_changed;
   bool pcb_state_gs_changed;
   bool pcb_state_fs_changed;

   int num_surfaces[PIPE_SHADER_TYPES];
};

struct gen6_rectlist_session {
   uint32_t DEPTH_STENCIL_STATE;
   uint32_t COLOR_CALC_STATE;
   uint32_t CC_VIEWPORT;
};

void
gen6_pipeline_prepare(const struct ilo_3d_pipeline *p,
                      const struct ilo_context *ilo,
                      struct gen6_pipeline_session *session);

void
gen6_pipeline_draw(struct ilo_3d_pipeline *p,
                   const struct ilo_context *ilo,
                   struct gen6_pipeline_session *session);

void
gen6_pipeline_end(struct ilo_3d_pipeline *p,
                  const struct ilo_context *ilo,
                  struct gen6_pipeline_session *session);

void
gen6_pipeline_common_select(struct ilo_3d_pipeline *p,
                            const struct ilo_context *ilo,
                            struct gen6_pipeline_session *session);

void
gen6_pipeline_common_sip(struct ilo_3d_pipeline *p,
                         const struct ilo_context *ilo,
                         struct gen6_pipeline_session *session);

void
gen6_pipeline_common_base_address(struct ilo_3d_pipeline *p,
                                  const struct ilo_context *ilo,
                                  struct gen6_pipeline_session *session);

void
gen6_pipeline_vf(struct ilo_3d_pipeline *p,
                 const struct ilo_context *ilo,
                 struct gen6_pipeline_session *session);

void
gen6_pipeline_vf_statistics(struct ilo_3d_pipeline *p,
                            const struct ilo_context *ilo,
                            struct gen6_pipeline_session *session);

void
gen6_pipeline_vs(struct ilo_3d_pipeline *p,
                 const struct ilo_context *ilo,
                 struct gen6_pipeline_session *session);

void
gen6_pipeline_clip(struct ilo_3d_pipeline *p,
                   const struct ilo_context *ilo,
                   struct gen6_pipeline_session *session);

void
gen6_pipeline_sf_rect(struct ilo_3d_pipeline *p,
                      const struct ilo_context *ilo,
                      struct gen6_pipeline_session *session);

void
gen6_pipeline_wm_raster(struct ilo_3d_pipeline *p,
                        const struct ilo_context *ilo,
                        struct gen6_pipeline_session *session);

void
gen6_pipeline_states(struct ilo_3d_pipeline *p,
                     const struct ilo_context *ilo,
                     struct gen6_pipeline_session *session);

bool
gen6_pipeline_update_max_svbi(struct ilo_3d_pipeline *p,
                              const struct ilo_context *ilo,
                              struct gen6_pipeline_session *session);

int
gen6_pipeline_estimate_state_size(const struct ilo_3d_pipeline *p,
                                  const struct ilo_context *ilo);

void
ilo_3d_pipeline_emit_flush_gen6(struct ilo_3d_pipeline *p);

void
ilo_3d_pipeline_emit_write_timestamp_gen6(struct ilo_3d_pipeline *p,
                                          struct intel_bo *bo, int index);

void
ilo_3d_pipeline_emit_write_depth_count_gen6(struct ilo_3d_pipeline *p,
                                            struct intel_bo *bo, int index);

void
ilo_3d_pipeline_emit_write_statistics_gen6(struct ilo_3d_pipeline *p,
                                           struct intel_bo *bo, int index);

void
ilo_3d_pipeline_init_gen6(struct ilo_3d_pipeline *p);

#endif /* ILO_3D_PIPELINE_GEN6_H */

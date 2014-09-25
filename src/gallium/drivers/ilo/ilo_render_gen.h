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

#ifndef ILO_RENDER_GEN_H
#define ILO_RENDER_GEN_H

#include "ilo_common.h"

struct ilo_query;
struct ilo_render;
struct ilo_state_vector;

struct gen6_draw_session {
   uint32_t pipe_dirty;

   int reduced_prim;

   bool prim_changed;
   bool primitive_restart_changed;

   void (*emit_draw_states)(struct ilo_render *render,
                            const struct ilo_state_vector *ilo,
                            struct gen6_draw_session *session);

   void (*emit_draw_commands)(struct ilo_render *render,
                              const struct ilo_state_vector *ilo,
                              struct gen6_draw_session *session);

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
gen6_wa_pre_pipe_control(struct ilo_render *r, uint32_t dw1);

void
gen6_draw_prepare(struct ilo_render *r,
                  const struct ilo_state_vector *ilo,
                  struct gen6_draw_session *session);

void
gen6_draw_emit(struct ilo_render *r,
               const struct ilo_state_vector *ilo,
               struct gen6_draw_session *session);

void
gen6_draw_end(struct ilo_render *r,
              const struct ilo_state_vector *ilo,
              struct gen6_draw_session *session);

void
gen6_draw_common_select(struct ilo_render *r,
                        const struct ilo_state_vector *ilo,
                        struct gen6_draw_session *session);

void
gen6_draw_common_sip(struct ilo_render *r,
                     const struct ilo_state_vector *ilo,
                     struct gen6_draw_session *session);

void
gen6_draw_common_base_address(struct ilo_render *r,
                              const struct ilo_state_vector *ilo,
                              struct gen6_draw_session *session);

void
gen6_draw_vf(struct ilo_render *r,
             const struct ilo_state_vector *ilo,
             struct gen6_draw_session *session);

void
gen6_draw_vf_statistics(struct ilo_render *r,
                        const struct ilo_state_vector *ilo,
                        struct gen6_draw_session *session);

void
gen6_draw_vs(struct ilo_render *r,
             const struct ilo_state_vector *ilo,
             struct gen6_draw_session *session);

void
gen6_draw_clip(struct ilo_render *r,
               const struct ilo_state_vector *ilo,
               struct gen6_draw_session *session);

void
gen6_draw_sf_rect(struct ilo_render *r,
                  const struct ilo_state_vector *ilo,
                  struct gen6_draw_session *session);

void
gen6_draw_wm_raster(struct ilo_render *r,
                    const struct ilo_state_vector *ilo,
                    struct gen6_draw_session *session);

void
gen6_draw_states(struct ilo_render *r,
                 const struct ilo_state_vector *ilo,
                 struct gen6_draw_session *session);

int
gen6_render_estimate_state_size(const struct ilo_render *render,
                                const struct ilo_state_vector *ilo);

void
ilo_render_init_gen6(struct ilo_render *render);

#endif /* ILO_RENDER_GEN_H */

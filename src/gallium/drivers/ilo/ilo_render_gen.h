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
#include "ilo_builder.h"
#include "ilo_render.h"

struct ilo_blitter;
struct ilo_render;
struct ilo_state_vector;

struct gen6_draw_session {
   uint32_t pipe_dirty;

   /* commands */
   int reduced_prim;

   bool prim_changed;
   bool primitive_restart_changed;

   /* dynamic states */
   bool viewport_changed;
   bool scissor_changed;

   bool cc_changed;
   bool dsa_changed;
   bool blend_changed;

   bool sampler_vs_changed;
   bool sampler_gs_changed;
   bool sampler_fs_changed;

   bool pcb_vs_changed;
   bool pcb_gs_changed;
   bool pcb_fs_changed;

   /* surface states */
   bool binding_table_vs_changed;
   bool binding_table_gs_changed;
   bool binding_table_fs_changed;

   int num_surfaces[PIPE_SHADER_TYPES];
};

int
ilo_render_get_draw_commands_len_gen6(const struct ilo_render *render,
                                      const struct ilo_state_vector *vec);

int
ilo_render_get_draw_commands_len_gen7(const struct ilo_render *render,
                                      const struct ilo_state_vector *vec);

static inline int
ilo_render_get_draw_commands_len(const struct ilo_render *render,
                                 const struct ilo_state_vector *vec)
{
   if (ilo_dev_gen(render->dev) >= ILO_GEN(7))
      return ilo_render_get_draw_commands_len_gen7(render, vec);
   else
      return ilo_render_get_draw_commands_len_gen6(render, vec);
}

void
ilo_render_emit_draw_commands_gen6(struct ilo_render *render,
                                   const struct ilo_state_vector *vec,
                                   struct gen6_draw_session *session);

void
ilo_render_emit_draw_commands_gen7(struct ilo_render *render,
                                   const struct ilo_state_vector *vec,
                                   struct gen6_draw_session *session);

static inline void
ilo_render_emit_draw_commands(struct ilo_render *render,
                              const struct ilo_state_vector *vec,
                              struct gen6_draw_session *session)
{
   const unsigned batch_used = ilo_builder_batch_used(render->builder);

   if (ilo_dev_gen(render->dev) >= ILO_GEN(7))
      ilo_render_emit_draw_commands_gen7(render, vec, session);
   else
      ilo_render_emit_draw_commands_gen6(render, vec, session);

   assert(ilo_builder_batch_used(render->builder) <= batch_used +
         ilo_render_get_draw_commands_len(render, vec));
}

int
ilo_render_get_rectlist_commands_len_gen6(const struct ilo_render *render,
                                          const struct ilo_blitter *blitter);

static inline int
ilo_render_get_rectlist_commands_len(const struct ilo_render *render,
                                     const struct ilo_blitter *blitter)
{
   return ilo_render_get_rectlist_commands_len_gen6(render, blitter);
}

void
ilo_render_emit_rectlist_commands_gen6(struct ilo_render *r,
                                       const struct ilo_blitter *blitter);

void
ilo_render_emit_rectlist_commands_gen7(struct ilo_render *r,
                                       const struct ilo_blitter *blitter);

static inline void
ilo_render_emit_rectlist_commands(struct ilo_render *render,
                                  const struct ilo_blitter *blitter)
{
   const unsigned batch_used = ilo_builder_batch_used(render->builder);

   if (ilo_dev_gen(render->dev) >= ILO_GEN(7))
      ilo_render_emit_rectlist_commands_gen7(render, blitter);
   else
      ilo_render_emit_rectlist_commands_gen6(render, blitter);

   assert(ilo_builder_batch_used(render->builder) <= batch_used +
         ilo_render_get_rectlist_commands_len(render, blitter));
}

int
ilo_render_get_draw_dynamic_states_len(const struct ilo_render *render,
                                       const struct ilo_state_vector *vec);

void
ilo_render_emit_draw_dynamic_states(struct ilo_render *render,
                                    const struct ilo_state_vector *vec,
                                    struct gen6_draw_session *session);

int
ilo_render_get_rectlist_dynamic_states_len(const struct ilo_render *render,
                                           const struct ilo_blitter *blitter);

void
ilo_render_emit_rectlist_dynamic_states(struct ilo_render *render,
                                        const struct ilo_blitter *blitter);

int
ilo_render_get_draw_surface_states_len(const struct ilo_render *render,
                                       const struct ilo_state_vector *vec);

void
ilo_render_emit_draw_surface_states(struct ilo_render *render,
                                    const struct ilo_state_vector *vec,
                                    struct gen6_draw_session *session);

void
gen6_wa_pre_pipe_control(struct ilo_render *r, uint32_t dw1);

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

#endif /* ILO_RENDER_GEN_H */

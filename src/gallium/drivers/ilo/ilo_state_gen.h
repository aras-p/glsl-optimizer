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

#ifndef ILO_STATE_GEN_H
#define ILO_STATE_GEN_H

#include "genhw/genhw.h"
#include "intel_winsys.h"

#include "ilo_common.h"
#include "ilo_state.h"

/**
 * Translate winsys tiling to hardware tiling.
 */
static inline int
ilo_gpe_gen6_translate_winsys_tiling(enum intel_tiling_mode tiling)
{
   switch (tiling) {
   case INTEL_TILING_NONE:
      return GEN6_TILING_NONE;
   case INTEL_TILING_X:
      return GEN6_TILING_X;
   case INTEL_TILING_Y:
      return GEN6_TILING_Y;
   default:
      assert(!"unknown tiling");
      return GEN6_TILING_NONE;
   }
}

/**
 * Translate a pipe texture target to the matching hardware surface type.
 */
static inline int
ilo_gpe_gen6_translate_texture(enum pipe_texture_target target)
{
   switch (target) {
   case PIPE_BUFFER:
      return GEN6_SURFTYPE_BUFFER;
   case PIPE_TEXTURE_1D:
   case PIPE_TEXTURE_1D_ARRAY:
      return GEN6_SURFTYPE_1D;
   case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_RECT:
   case PIPE_TEXTURE_2D_ARRAY:
      return GEN6_SURFTYPE_2D;
   case PIPE_TEXTURE_3D:
      return GEN6_SURFTYPE_3D;
   case PIPE_TEXTURE_CUBE:
   case PIPE_TEXTURE_CUBE_ARRAY:
      return GEN6_SURFTYPE_CUBE;
   default:
      assert(!"unknown texture target");
      return GEN6_SURFTYPE_BUFFER;
   }
}

void
ilo_gpe_init_ve(const struct ilo_dev_info *dev,
                unsigned num_states,
                const struct pipe_vertex_element *states,
                struct ilo_ve_state *ve);

void
ilo_gpe_set_viewport_cso(const struct ilo_dev_info *dev,
                         const struct pipe_viewport_state *state,
                         struct ilo_viewport_cso *vp);

void
ilo_gpe_set_scissor(const struct ilo_dev_info *dev,
                    unsigned start_slot,
                    unsigned num_states,
                    const struct pipe_scissor_state *states,
                    struct ilo_scissor_state *scissor);

void
ilo_gpe_set_scissor_null(const struct ilo_dev_info *dev,
                         struct ilo_scissor_state *scissor);

void
ilo_gpe_init_rasterizer_clip(const struct ilo_dev_info *dev,
                             const struct pipe_rasterizer_state *state,
                             struct ilo_rasterizer_clip *clip);

void
ilo_gpe_init_rasterizer_sf(const struct ilo_dev_info *dev,
                           const struct pipe_rasterizer_state *state,
                           struct ilo_rasterizer_sf *sf);

void
ilo_gpe_init_rasterizer_wm_gen6(const struct ilo_dev_info *dev,
                                const struct pipe_rasterizer_state *state,
                                struct ilo_rasterizer_wm *wm);

void
ilo_gpe_init_rasterizer_wm_gen7(const struct ilo_dev_info *dev,
                                const struct pipe_rasterizer_state *state,
                                struct ilo_rasterizer_wm *wm);

static inline void
ilo_gpe_init_rasterizer(const struct ilo_dev_info *dev,
                        const struct pipe_rasterizer_state *state,
                        struct ilo_rasterizer_state *rasterizer)
{
   ilo_gpe_init_rasterizer_clip(dev, state, &rasterizer->clip);
   ilo_gpe_init_rasterizer_sf(dev, state, &rasterizer->sf);

   if (ilo_dev_gen(dev) >= ILO_GEN(7))
      ilo_gpe_init_rasterizer_wm_gen7(dev, state, &rasterizer->wm);
   else
      ilo_gpe_init_rasterizer_wm_gen6(dev, state, &rasterizer->wm);
}

void
ilo_gpe_init_dsa(const struct ilo_dev_info *dev,
                 const struct pipe_depth_stencil_alpha_state *state,
                 struct ilo_dsa_state *dsa);

void
ilo_gpe_init_blend(const struct ilo_dev_info *dev,
                   const struct pipe_blend_state *state,
                   struct ilo_blend_state *blend);

void
ilo_gpe_init_sampler_cso(const struct ilo_dev_info *dev,
                         const struct pipe_sampler_state *state,
                         struct ilo_sampler_cso *sampler);

void
ilo_gpe_init_view_surface_null_gen6(const struct ilo_dev_info *dev,
                                    unsigned width, unsigned height,
                                    unsigned depth, unsigned level,
                                    struct ilo_view_surface *surf);

void
ilo_gpe_init_view_surface_for_buffer_gen6(const struct ilo_dev_info *dev,
                                          const struct ilo_buffer *buf,
                                          unsigned offset, unsigned size,
                                          unsigned struct_size,
                                          enum pipe_format elem_format,
                                          bool is_rt, bool render_cache_rw,
                                          struct ilo_view_surface *surf);

void
ilo_gpe_init_view_surface_for_texture_gen6(const struct ilo_dev_info *dev,
                                           const struct ilo_texture *tex,
                                           enum pipe_format format,
                                           unsigned first_level,
                                           unsigned num_levels,
                                           unsigned first_layer,
                                           unsigned num_layers,
                                           bool is_rt,
                                           struct ilo_view_surface *surf);

void
ilo_gpe_init_view_surface_null_gen7(const struct ilo_dev_info *dev,
                                    unsigned width, unsigned height,
                                    unsigned depth, unsigned level,
                                    struct ilo_view_surface *surf);

void
ilo_gpe_init_view_surface_for_buffer_gen7(const struct ilo_dev_info *dev,
                                          const struct ilo_buffer *buf,
                                          unsigned offset, unsigned size,
                                          unsigned struct_size,
                                          enum pipe_format elem_format,
                                          bool is_rt, bool render_cache_rw,
                                          struct ilo_view_surface *surf);

void
ilo_gpe_init_view_surface_for_texture_gen7(const struct ilo_dev_info *dev,
                                           const struct ilo_texture *tex,
                                           enum pipe_format format,
                                           unsigned first_level,
                                           unsigned num_levels,
                                           unsigned first_layer,
                                           unsigned num_layers,
                                           bool is_rt,
                                           struct ilo_view_surface *surf);

static inline void
ilo_gpe_init_view_surface_null(const struct ilo_dev_info *dev,
                               unsigned width, unsigned height,
                               unsigned depth, unsigned level,
                               struct ilo_view_surface *surf)
{
   if (ilo_dev_gen(dev) >= ILO_GEN(7)) {
      ilo_gpe_init_view_surface_null_gen7(dev,
            width, height, depth, level, surf);
   }
   else {
      ilo_gpe_init_view_surface_null_gen6(dev,
            width, height, depth, level, surf);
   }
}

static inline void
ilo_gpe_init_view_surface_for_buffer(const struct ilo_dev_info *dev,
                                     const struct ilo_buffer *buf,
                                     unsigned offset, unsigned size,
                                     unsigned struct_size,
                                     enum pipe_format elem_format,
                                     bool is_rt, bool render_cache_rw,
                                     struct ilo_view_surface *surf)
{
   if (ilo_dev_gen(dev) >= ILO_GEN(7)) {
      ilo_gpe_init_view_surface_for_buffer_gen7(dev, buf, offset, size,
            struct_size, elem_format, is_rt, render_cache_rw, surf);
   }
   else {
      ilo_gpe_init_view_surface_for_buffer_gen6(dev, buf, offset, size,
            struct_size, elem_format, is_rt, render_cache_rw, surf);
   }
}

static inline void
ilo_gpe_init_view_surface_for_texture(const struct ilo_dev_info *dev,
                                      const struct ilo_texture *tex,
                                      enum pipe_format format,
                                      unsigned first_level,
                                      unsigned num_levels,
                                      unsigned first_layer,
                                      unsigned num_layers,
                                      bool is_rt,
                                      struct ilo_view_surface *surf)
{
   if (ilo_dev_gen(dev) >= ILO_GEN(7)) {
      ilo_gpe_init_view_surface_for_texture_gen7(dev, tex, format,
            first_level, num_levels, first_layer, num_layers,
            is_rt, surf);
   }
   else {
      ilo_gpe_init_view_surface_for_texture_gen6(dev, tex, format,
            first_level, num_levels, first_layer, num_layers,
            is_rt, surf);
   }
}

void
ilo_gpe_init_zs_surface(const struct ilo_dev_info *dev,
                        const struct ilo_texture *tex,
                        enum pipe_format format, unsigned level,
                        unsigned first_layer, unsigned num_layers,
                        struct ilo_zs_surface *zs);

void
ilo_gpe_init_vs_cso(const struct ilo_dev_info *dev,
                    const struct ilo_shader_state *vs,
                    struct ilo_shader_cso *cso);

void
ilo_gpe_init_gs_cso_gen6(const struct ilo_dev_info *dev,
                         const struct ilo_shader_state *gs,
                         struct ilo_shader_cso *cso);

void
ilo_gpe_init_gs_cso_gen7(const struct ilo_dev_info *dev,
                         const struct ilo_shader_state *gs,
                         struct ilo_shader_cso *cso);

static inline void
ilo_gpe_init_gs_cso(const struct ilo_dev_info *dev,
                    const struct ilo_shader_state *gs,
                    struct ilo_shader_cso *cso)
{
   if (ilo_dev_gen(dev) >= ILO_GEN(7)) {
      ilo_gpe_init_gs_cso_gen7(dev, gs, cso);
   }
   else {
      ilo_gpe_init_gs_cso_gen6(dev, gs, cso);
   }
}

void
ilo_gpe_init_fs_cso_gen6(const struct ilo_dev_info *dev,
                         const struct ilo_shader_state *fs,
                         struct ilo_shader_cso *cso);

void
ilo_gpe_init_fs_cso_gen7(const struct ilo_dev_info *dev,
                         const struct ilo_shader_state *fs,
                         struct ilo_shader_cso *cso);

static inline void
ilo_gpe_init_fs_cso(const struct ilo_dev_info *dev,
                    const struct ilo_shader_state *fs,
                    struct ilo_shader_cso *cso)
{
   if (ilo_dev_gen(dev) >= ILO_GEN(7)) {
      ilo_gpe_init_fs_cso_gen7(dev, fs, cso);
   }
   else {
      ilo_gpe_init_fs_cso_gen6(dev, fs, cso);
   }
}

void
ilo_gpe_set_fb(const struct ilo_dev_info *dev,
               const struct pipe_framebuffer_state *state,
               struct ilo_fb_state *fb);

#endif /* ILO_STATE_GEN_H */

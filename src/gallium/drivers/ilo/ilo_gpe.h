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

#ifndef ILO_GPE_H
#define ILO_GPE_H

#include "ilo_common.h"

/**
 * \see brw_context.h
 */
#define ILO_MAX_DRAW_BUFFERS    8
#define ILO_MAX_CONST_BUFFERS   (1 + 12)
#define ILO_MAX_SAMPLER_VIEWS   16
#define ILO_MAX_SAMPLERS        16
#define ILO_MAX_SO_BINDINGS     64
#define ILO_MAX_SO_BUFFERS      4
#define ILO_MAX_VIEWPORTS       1

#define ILO_MAX_VS_SURFACES        (ILO_MAX_CONST_BUFFERS + ILO_MAX_SAMPLER_VIEWS)
#define ILO_VS_CONST_SURFACE(i)    (i)
#define ILO_VS_TEXTURE_SURFACE(i)  (ILO_MAX_CONST_BUFFERS  + i)

#define ILO_MAX_GS_SURFACES        (ILO_MAX_SO_BINDINGS)
#define ILO_GS_SO_SURFACE(i)       (i)

#define ILO_MAX_WM_SURFACES        (ILO_MAX_DRAW_BUFFERS + ILO_MAX_CONST_BUFFERS + ILO_MAX_SAMPLER_VIEWS)
#define ILO_WM_DRAW_SURFACE(i)     (i)
#define ILO_WM_CONST_SURFACE(i)    (ILO_MAX_DRAW_BUFFERS + i)
#define ILO_WM_TEXTURE_SURFACE(i)  (ILO_MAX_DRAW_BUFFERS + ILO_MAX_CONST_BUFFERS  + i)

struct ilo_buffer;
struct ilo_texture;
struct ilo_shader_state;

struct ilo_vb_state {
   struct pipe_vertex_buffer states[PIPE_MAX_ATTRIBS];
   uint32_t enabled_mask;
};

struct ilo_ib_state {
   struct pipe_resource *buffer;
   const void *user_buffer;
   unsigned offset;
   unsigned index_size;

   /* these are not valid until the state is finalized */
   struct pipe_resource *hw_resource;
   unsigned hw_index_size;
   /* an offset to be added to pipe_draw_info::start */
   int64_t draw_start_offset;
};

struct ilo_ve_cso {
   /* VERTEX_ELEMENT_STATE */
   uint32_t payload[2];
};

struct ilo_ve_state {
   struct ilo_ve_cso cso[PIPE_MAX_ATTRIBS];
   unsigned count;

   unsigned instance_divisors[PIPE_MAX_ATTRIBS];
   unsigned vb_mapping[PIPE_MAX_ATTRIBS];
   unsigned vb_count;
};

struct ilo_so_state {
   struct pipe_stream_output_target *states[ILO_MAX_SO_BUFFERS];
   unsigned count;
   unsigned append_bitmask;

   bool enabled;
};

struct ilo_viewport_cso {
   /* matrix form */
   float m00, m11, m22, m30, m31, m32;

   /* guardband in NDC space */
   float min_gbx, min_gby, max_gbx, max_gby;

   /* viewport in screen space */
   float min_x, min_y, min_z;
   float max_x, max_y, max_z;
};

struct ilo_viewport_state {
   struct ilo_viewport_cso cso[ILO_MAX_VIEWPORTS];
   unsigned count;

   struct pipe_viewport_state viewport0;
};

struct ilo_scissor_state {
   /* SCISSOR_RECT */
   uint32_t payload[ILO_MAX_VIEWPORTS * 2];

   struct pipe_scissor_state scissor0;
};

struct ilo_rasterizer_clip {
   /* 3DSTATE_CLIP */
   uint32_t payload[3];

   uint32_t can_enable_guardband;
};

struct ilo_rasterizer_sf {
   /* 3DSTATE_SF */
   uint32_t payload[6];
   uint32_t dw_msaa;
};

struct ilo_rasterizer_wm {
   /* 3DSTATE_WM */
   uint32_t payload[2];
   uint32_t dw_msaa_rast;
   uint32_t dw_msaa_disp;
};

struct ilo_rasterizer_state {
   struct pipe_rasterizer_state state;

   struct ilo_rasterizer_clip clip;
   struct ilo_rasterizer_sf sf;
   struct ilo_rasterizer_wm wm;
};

struct ilo_dsa_state {
   /* DEPTH_STENCIL_STATE */
   uint32_t payload[3];

   uint32_t dw_alpha;
   ubyte alpha_ref;
};

struct ilo_blend_cso {
   /* BLEND_STATE */
   uint32_t payload[2];

   uint32_t dw_blend;
   uint32_t dw_blend_dst_alpha_forced_one;

   uint32_t dw_logicop;
   uint32_t dw_alpha_mod;
};

struct ilo_blend_state {
   struct ilo_blend_cso cso[ILO_MAX_DRAW_BUFFERS];

   bool independent_blend_enable;
   bool dual_blend;
   bool alpha_to_coverage;
};

struct ilo_sampler_cso {
   /* SAMPLER_STATE and SAMPLER_BORDER_COLOR_STATE */
   uint32_t payload[15];

   uint32_t dw_filter;
   uint32_t dw_filter_aniso;
   uint32_t dw_wrap;
   uint32_t dw_wrap_1d;
   uint32_t dw_wrap_cube;

   bool anisotropic;
   bool saturate_r;
   bool saturate_s;
   bool saturate_t;
};

struct ilo_sampler_state {
   const struct ilo_sampler_cso *cso[ILO_MAX_SAMPLERS];
   unsigned count;
};

struct ilo_view_surface {
   /* SURFACE_STATE */
   uint32_t payload[8];
   struct intel_bo *bo;
};

struct ilo_view_cso {
   struct pipe_sampler_view base;

   struct ilo_view_surface surface;
};

struct ilo_view_state {
   struct pipe_sampler_view *states[ILO_MAX_SAMPLER_VIEWS];
   unsigned count;
};

struct ilo_cbuf_cso {
   struct pipe_resource *resource;
   struct ilo_view_surface surface;

   /*
    * this CSO is not so constant because user buffer needs to be uploaded in
    * finalize_constant_buffers()
    */
   const void *user_buffer;
   unsigned user_buffer_size;
};

struct ilo_cbuf_state {
   struct ilo_cbuf_cso cso[ILO_MAX_CONST_BUFFERS];
   uint32_t enabled_mask;
};

struct ilo_resource_state {
   struct pipe_surface *states[PIPE_MAX_SHADER_RESOURCES];
   unsigned count;
};

struct ilo_surface_cso {
   struct pipe_surface base;

   bool is_rt;
   union {
      struct ilo_view_surface rt;
      struct ilo_zs_surface {
         uint32_t payload[10];
         struct intel_bo *bo;
         struct intel_bo *hiz_bo;
         struct intel_bo *separate_s8_bo;
      } zs;
   } u;
};

struct ilo_fb_state {
   struct pipe_framebuffer_state state;

   struct ilo_view_surface null_rt;
   struct ilo_zs_surface null_zs;

   unsigned num_samples;
   bool offset_to_layers;
};

struct ilo_global_binding {
   /*
    * XXX These should not be treated as real resources (and there could be
    * thousands of them).  They should be treated as regions in GLOBAL
    * resource, which is the only real resource.
    *
    * That is, a resource here should instead be
    *
    *   struct ilo_global_region {
    *     struct pipe_resource base;
    *     int offset;
    *     int size;
    *   };
    *
    * and it describes the region [offset, offset + size) in GLOBAL
    * resource.
    */
   struct pipe_resource *resources[PIPE_MAX_SHADER_RESOURCES];
   uint32_t *handles[PIPE_MAX_SHADER_RESOURCES];
   unsigned count;
};

struct ilo_shader_cso {
   uint32_t payload[5];
};

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

   if (dev->gen >= ILO_GEN(7))
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
                                           bool is_rt, bool offset_to_layer,
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
                                           bool is_rt, bool offset_to_layer,
                                           struct ilo_view_surface *surf);

static inline void
ilo_gpe_init_view_surface_null(const struct ilo_dev_info *dev,
                               unsigned width, unsigned height,
                               unsigned depth, unsigned level,
                               struct ilo_view_surface *surf)
{
   if (dev->gen >= ILO_GEN(7)) {
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
   if (dev->gen >= ILO_GEN(7)) {
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
                                      bool is_rt, bool offset_to_layer,
                                      struct ilo_view_surface *surf)
{
   if (dev->gen >= ILO_GEN(7)) {
      ilo_gpe_init_view_surface_for_texture_gen7(dev, tex, format,
            first_level, num_levels, first_layer, num_layers,
            is_rt, offset_to_layer, surf);
   }
   else {
      ilo_gpe_init_view_surface_for_texture_gen6(dev, tex, format,
            first_level, num_levels, first_layer, num_layers,
            is_rt, offset_to_layer, surf);
   }
}

void
ilo_gpe_init_zs_surface(const struct ilo_dev_info *dev,
                        const struct ilo_texture *tex,
                        enum pipe_format format, unsigned level,
                        unsigned first_layer, unsigned num_layers,
                        bool offset_to_layer, struct ilo_zs_surface *zs);

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
   if (dev->gen >= ILO_GEN(7)) {
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
   if (dev->gen >= ILO_GEN(7)) {
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

#endif /* ILO_GPE_H */

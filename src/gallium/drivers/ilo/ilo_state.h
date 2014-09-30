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

#ifndef ILO_STATE_H
#define ILO_STATE_H

#include "pipe/p_state.h"

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

/**
 * States that we track.
 *
 * XXX Do we want to count each sampler or vertex buffer as a state?  If that
 * is the case, there are simply not enough bits.
 *
 * XXX We want to treat primitive type and depth clear value as states, but
 * there are not enough bits.
 */
enum ilo_state {
   ILO_STATE_VB,
   ILO_STATE_VE,
   ILO_STATE_IB,
   ILO_STATE_VS,
   ILO_STATE_GS,
   ILO_STATE_SO,
   ILO_STATE_CLIP,
   ILO_STATE_VIEWPORT,
   ILO_STATE_SCISSOR,
   ILO_STATE_RASTERIZER,
   ILO_STATE_POLY_STIPPLE,
   ILO_STATE_SAMPLE_MASK,
   ILO_STATE_FS,
   ILO_STATE_DSA,
   ILO_STATE_STENCIL_REF,
   ILO_STATE_BLEND,
   ILO_STATE_BLEND_COLOR,
   ILO_STATE_FB,

   ILO_STATE_SAMPLER_VS,
   ILO_STATE_SAMPLER_GS,
   ILO_STATE_SAMPLER_FS,
   ILO_STATE_SAMPLER_CS,
   ILO_STATE_VIEW_VS,
   ILO_STATE_VIEW_GS,
   ILO_STATE_VIEW_FS,
   ILO_STATE_VIEW_CS,
   ILO_STATE_CBUF,
   ILO_STATE_RESOURCE,

   ILO_STATE_CS,
   ILO_STATE_CS_RESOURCE,
   ILO_STATE_GLOBAL_BINDING,

   ILO_STATE_COUNT,
};

/**
 * Dirty flags of the states.
 */
enum ilo_dirty_flags {
   ILO_DIRTY_VB               = 1 << ILO_STATE_VB,
   ILO_DIRTY_VE               = 1 << ILO_STATE_VE,
   ILO_DIRTY_IB               = 1 << ILO_STATE_IB,
   ILO_DIRTY_VS               = 1 << ILO_STATE_VS,
   ILO_DIRTY_GS               = 1 << ILO_STATE_GS,
   ILO_DIRTY_SO               = 1 << ILO_STATE_SO,
   ILO_DIRTY_CLIP             = 1 << ILO_STATE_CLIP,
   ILO_DIRTY_VIEWPORT         = 1 << ILO_STATE_VIEWPORT,
   ILO_DIRTY_SCISSOR          = 1 << ILO_STATE_SCISSOR,
   ILO_DIRTY_RASTERIZER       = 1 << ILO_STATE_RASTERIZER,
   ILO_DIRTY_POLY_STIPPLE     = 1 << ILO_STATE_POLY_STIPPLE,
   ILO_DIRTY_SAMPLE_MASK      = 1 << ILO_STATE_SAMPLE_MASK,
   ILO_DIRTY_FS               = 1 << ILO_STATE_FS,
   ILO_DIRTY_DSA              = 1 << ILO_STATE_DSA,
   ILO_DIRTY_STENCIL_REF      = 1 << ILO_STATE_STENCIL_REF,
   ILO_DIRTY_BLEND            = 1 << ILO_STATE_BLEND,
   ILO_DIRTY_BLEND_COLOR      = 1 << ILO_STATE_BLEND_COLOR,
   ILO_DIRTY_FB               = 1 << ILO_STATE_FB,
   ILO_DIRTY_SAMPLER_VS       = 1 << ILO_STATE_SAMPLER_VS,
   ILO_DIRTY_SAMPLER_GS       = 1 << ILO_STATE_SAMPLER_GS,
   ILO_DIRTY_SAMPLER_FS       = 1 << ILO_STATE_SAMPLER_FS,
   ILO_DIRTY_SAMPLER_CS       = 1 << ILO_STATE_SAMPLER_CS,
   ILO_DIRTY_VIEW_VS          = 1 << ILO_STATE_VIEW_VS,
   ILO_DIRTY_VIEW_GS          = 1 << ILO_STATE_VIEW_GS,
   ILO_DIRTY_VIEW_FS          = 1 << ILO_STATE_VIEW_FS,
   ILO_DIRTY_VIEW_CS          = 1 << ILO_STATE_VIEW_CS,
   ILO_DIRTY_CBUF             = 1 << ILO_STATE_CBUF,
   ILO_DIRTY_RESOURCE         = 1 << ILO_STATE_RESOURCE,
   ILO_DIRTY_CS               = 1 << ILO_STATE_CS,
   ILO_DIRTY_CS_RESOURCE      = 1 << ILO_STATE_CS_RESOURCE,
   ILO_DIRTY_GLOBAL_BINDING   = 1 << ILO_STATE_GLOBAL_BINDING,
   ILO_DIRTY_ALL              = 0xffffffff,
};

struct intel_bo;
struct ilo_buffer;
struct ilo_context;
struct ilo_shader_state;
struct ilo_texture;

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

   /* these are not valid until the state is finalized */
   struct ilo_ve_cso edgeflag_cso;
   bool last_cso_edgeflag;

   struct ilo_ve_cso nosrc_cso;
   bool prepend_nosrc_cso;
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
         uint32_t dw_aligned_8x4;

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

struct ilo_state_vector {
   const struct pipe_draw_info *draw;

   uint32_t dirty;

   struct ilo_vb_state vb;
   struct ilo_ve_state *ve;
   struct ilo_ib_state ib;

   struct ilo_shader_state *vs;
   struct ilo_shader_state *gs;

   struct ilo_so_state so;

   struct pipe_clip_state clip;
   struct ilo_viewport_state viewport;
   struct ilo_scissor_state scissor;

   const struct ilo_rasterizer_state *rasterizer;
   struct pipe_poly_stipple poly_stipple;
   unsigned sample_mask;

   struct ilo_shader_state *fs;

   const struct ilo_dsa_state *dsa;
   struct pipe_stencil_ref stencil_ref;
   const struct ilo_blend_state *blend;
   struct pipe_blend_color blend_color;
   struct ilo_fb_state fb;

   /* shader resources */
   struct ilo_sampler_state sampler[PIPE_SHADER_TYPES];
   struct ilo_view_state view[PIPE_SHADER_TYPES];
   struct ilo_cbuf_state cbuf[PIPE_SHADER_TYPES];
   struct ilo_resource_state resource;

   /* GPGPU */
   struct ilo_shader_state *cs;
   struct ilo_resource_state cs_resource;
   struct ilo_global_binding global_binding;
};

void
ilo_init_state_functions(struct ilo_context *ilo);

void
ilo_finalize_3d_states(struct ilo_context *ilo,
                       const struct pipe_draw_info *draw);

void
ilo_state_vector_init(const struct ilo_dev_info *dev,
                      struct ilo_state_vector *vec);

void
ilo_state_vector_cleanup(struct ilo_state_vector *vec);

void
ilo_state_vector_resource_renamed(struct ilo_state_vector *vec,
                                  struct pipe_resource *res);

void
ilo_state_vector_dump_dirty(const struct ilo_state_vector *vec);

#endif /* ILO_STATE_H */

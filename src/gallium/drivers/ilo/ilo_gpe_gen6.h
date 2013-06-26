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

#ifndef ILO_GPE_GEN6_H
#define ILO_GPE_GEN6_H

#include "ilo_common.h"
#include "ilo_gpe.h"

#define ILO_GPE_VALID_GEN(dev, min_gen, max_gen) \
   assert((dev)->gen >= ILO_GEN(min_gen) && (dev)->gen <= ILO_GEN(max_gen))

#define ILO_GPE_CMD(pipeline, op, subop) \
   (0x3 << 29 | (pipeline) << 27 | (op) << 24 | (subop) << 16)

/**
 * Commands that GEN6 GPE could emit.
 */
enum ilo_gpe_gen6_command {
   ILO_GPE_GEN6_STATE_BASE_ADDRESS,                  /* (0x0, 0x1, 0x01) */
   ILO_GPE_GEN6_STATE_SIP,                           /* (0x0, 0x1, 0x02) */
   ILO_GPE_GEN6_3DSTATE_VF_STATISTICS,               /* (0x1, 0x0, 0x0b) */
   ILO_GPE_GEN6_PIPELINE_SELECT,                     /* (0x1, 0x1, 0x04) */
   ILO_GPE_GEN6_MEDIA_VFE_STATE,                     /* (0x2, 0x0, 0x00) */
   ILO_GPE_GEN6_MEDIA_CURBE_LOAD,                    /* (0x2, 0x0, 0x01) */
   ILO_GPE_GEN6_MEDIA_INTERFACE_DESCRIPTOR_LOAD,     /* (0x2, 0x0, 0x02) */
   ILO_GPE_GEN6_MEDIA_GATEWAY_STATE,                 /* (0x2, 0x0, 0x03) */
   ILO_GPE_GEN6_MEDIA_STATE_FLUSH,                   /* (0x2, 0x0, 0x04) */
   ILO_GPE_GEN6_MEDIA_OBJECT_WALKER,                 /* (0x2, 0x1, 0x03) */
   ILO_GPE_GEN6_3DSTATE_BINDING_TABLE_POINTERS,      /* (0x3, 0x0, 0x01) */
   ILO_GPE_GEN6_3DSTATE_SAMPLER_STATE_POINTERS,      /* (0x3, 0x0, 0x02) */
   ILO_GPE_GEN6_3DSTATE_URB,                         /* (0x3, 0x0, 0x05) */
   ILO_GPE_GEN6_3DSTATE_VERTEX_BUFFERS,              /* (0x3, 0x0, 0x08) */
   ILO_GPE_GEN6_3DSTATE_VERTEX_ELEMENTS,             /* (0x3, 0x0, 0x09) */
   ILO_GPE_GEN6_3DSTATE_INDEX_BUFFER,                /* (0x3, 0x0, 0x0a) */
   ILO_GPE_GEN6_3DSTATE_VIEWPORT_STATE_POINTERS,     /* (0x3, 0x0, 0x0d) */
   ILO_GPE_GEN6_3DSTATE_CC_STATE_POINTERS,           /* (0x3, 0x0, 0x0e) */
   ILO_GPE_GEN6_3DSTATE_SCISSOR_STATE_POINTERS,      /* (0x3, 0x0, 0x0f) */
   ILO_GPE_GEN6_3DSTATE_VS,                          /* (0x3, 0x0, 0x10) */
   ILO_GPE_GEN6_3DSTATE_GS,                          /* (0x3, 0x0, 0x11) */
   ILO_GPE_GEN6_3DSTATE_CLIP,                        /* (0x3, 0x0, 0x12) */
   ILO_GPE_GEN6_3DSTATE_SF,                          /* (0x3, 0x0, 0x13) */
   ILO_GPE_GEN6_3DSTATE_WM,                          /* (0x3, 0x0, 0x14) */
   ILO_GPE_GEN6_3DSTATE_CONSTANT_VS,                 /* (0x3, 0x0, 0x15) */
   ILO_GPE_GEN6_3DSTATE_CONSTANT_GS,                 /* (0x3, 0x0, 0x16) */
   ILO_GPE_GEN6_3DSTATE_CONSTANT_PS,                 /* (0x3, 0x0, 0x17) */
   ILO_GPE_GEN6_3DSTATE_SAMPLE_MASK,                 /* (0x3, 0x0, 0x18) */
   ILO_GPE_GEN6_3DSTATE_DRAWING_RECTANGLE,           /* (0x3, 0x1, 0x00) */
   ILO_GPE_GEN6_3DSTATE_DEPTH_BUFFER,                /* (0x3, 0x1, 0x05) */
   ILO_GPE_GEN6_3DSTATE_POLY_STIPPLE_OFFSET,         /* (0x3, 0x1, 0x06) */
   ILO_GPE_GEN6_3DSTATE_POLY_STIPPLE_PATTERN,        /* (0x3, 0x1, 0x07) */
   ILO_GPE_GEN6_3DSTATE_LINE_STIPPLE,                /* (0x3, 0x1, 0x08) */
   ILO_GPE_GEN6_3DSTATE_AA_LINE_PARAMETERS,          /* (0x3, 0x1, 0x0a) */
   ILO_GPE_GEN6_3DSTATE_GS_SVB_INDEX,                /* (0x3, 0x1, 0x0b) */
   ILO_GPE_GEN6_3DSTATE_MULTISAMPLE,                 /* (0x3, 0x1, 0x0d) */
   ILO_GPE_GEN6_3DSTATE_STENCIL_BUFFER,              /* (0x3, 0x1, 0x0e) */
   ILO_GPE_GEN6_3DSTATE_HIER_DEPTH_BUFFER,           /* (0x3, 0x1, 0x0f) */
   ILO_GPE_GEN6_3DSTATE_CLEAR_PARAMS,                /* (0x3, 0x1, 0x10) */
   ILO_GPE_GEN6_PIPE_CONTROL,                        /* (0x3, 0x2, 0x00) */
   ILO_GPE_GEN6_3DPRIMITIVE,                         /* (0x3, 0x3, 0x00) */

   ILO_GPE_GEN6_COMMAND_COUNT,
};

/**
 * Indirect states that GEN6 GPE could emit.
 */
enum ilo_gpe_gen6_state {
   ILO_GPE_GEN6_INTERFACE_DESCRIPTOR_DATA,
   ILO_GPE_GEN6_SF_VIEWPORT,
   ILO_GPE_GEN6_CLIP_VIEWPORT,
   ILO_GPE_GEN6_CC_VIEWPORT,
   ILO_GPE_GEN6_COLOR_CALC_STATE,
   ILO_GPE_GEN6_BLEND_STATE,
   ILO_GPE_GEN6_DEPTH_STENCIL_STATE,
   ILO_GPE_GEN6_SCISSOR_RECT,
   ILO_GPE_GEN6_BINDING_TABLE_STATE,
   ILO_GPE_GEN6_SURFACE_STATE,
   ILO_GPE_GEN6_SAMPLER_STATE,
   ILO_GPE_GEN6_SAMPLER_BORDER_COLOR_STATE,
   ILO_GPE_GEN6_PUSH_CONSTANT_BUFFER,

   ILO_GPE_GEN6_STATE_COUNT,
};

enum intel_tiling_mode;

struct intel_bo;
struct ilo_cp;
struct ilo_texture;
struct ilo_shader;

typedef void
(*ilo_gpe_gen6_STATE_BASE_ADDRESS)(const struct ilo_dev_info *dev,
                                   struct intel_bo *general_state_bo,
                                   struct intel_bo *surface_state_bo,
                                   struct intel_bo *dynamic_state_bo,
                                   struct intel_bo *indirect_object_bo,
                                   struct intel_bo *instruction_bo,
                                   uint32_t general_state_size,
                                   uint32_t dynamic_state_size,
                                   uint32_t indirect_object_size,
                                   uint32_t instruction_size,
                                   struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen6_STATE_SIP)(const struct ilo_dev_info *dev,
                          uint32_t sip,
                          struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen6_3DSTATE_VF_STATISTICS)(const struct ilo_dev_info *dev,
                                      bool enable,
                                      struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen6_PIPELINE_SELECT)(const struct ilo_dev_info *dev,
                                int pipeline,
                                struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen6_MEDIA_VFE_STATE)(const struct ilo_dev_info *dev,
                                int max_threads, int num_urb_entries,
                                int urb_entry_size,
                                struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen6_MEDIA_CURBE_LOAD)(const struct ilo_dev_info *dev,
                                 uint32_t buf, int size,
                                 struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen6_MEDIA_INTERFACE_DESCRIPTOR_LOAD)(const struct ilo_dev_info *dev,
                                                uint32_t offset, int num_ids,
                                                struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen6_MEDIA_GATEWAY_STATE)(const struct ilo_dev_info *dev,
                                    int id, int byte, int thread_count,
                                    struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen6_MEDIA_STATE_FLUSH)(const struct ilo_dev_info *dev,
                                  int thread_count_water_mark,
                                  int barrier_mask,
                                  struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen6_MEDIA_OBJECT_WALKER)(const struct ilo_dev_info *dev,
                                    struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen6_3DSTATE_BINDING_TABLE_POINTERS)(const struct ilo_dev_info *dev,
                                               uint32_t vs_binding_table,
                                               uint32_t gs_binding_table,
                                               uint32_t ps_binding_table,
                                               struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen6_3DSTATE_SAMPLER_STATE_POINTERS)(const struct ilo_dev_info *dev,
                                               uint32_t vs_sampler_state,
                                               uint32_t gs_sampler_state,
                                               uint32_t ps_sampler_state,
                                               struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen6_3DSTATE_URB)(const struct ilo_dev_info *dev,
                            int vs_total_size, int gs_total_size,
                            int vs_entry_size, int gs_entry_size,
                            struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen6_3DSTATE_VERTEX_BUFFERS)(const struct ilo_dev_info *dev,
                                       const struct pipe_vertex_buffer *vbuffers,
                                       uint64_t vbuffer_mask,
                                       const struct ilo_ve_state *ve,
                                       struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen6_3DSTATE_VERTEX_ELEMENTS)(const struct ilo_dev_info *dev,
                                        const struct ilo_ve_state *ve,
                                        bool last_velement_edgeflag,
                                        bool prepend_generated_ids,
                                        struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen6_3DSTATE_INDEX_BUFFER)(const struct ilo_dev_info *dev,
                                     const struct ilo_ib_state *ib,
                                     bool enable_cut_index,
                                     struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen6_3DSTATE_VIEWPORT_STATE_POINTERS)(const struct ilo_dev_info *dev,
                                                uint32_t clip_viewport,
                                                uint32_t sf_viewport,
                                                uint32_t cc_viewport,
                                                struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen6_3DSTATE_CC_STATE_POINTERS)(const struct ilo_dev_info *dev,
                                          uint32_t blend_state,
                                          uint32_t depth_stencil_state,
                                          uint32_t color_calc_state,
                                          struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen6_3DSTATE_SCISSOR_STATE_POINTERS)(const struct ilo_dev_info *dev,
                                               uint32_t scissor_rect,
                                               struct ilo_cp *cp);


typedef void
(*ilo_gpe_gen6_3DSTATE_VS)(const struct ilo_dev_info *dev,
                           const struct ilo_shader_state *vs,
                           int num_samplers,
                           struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen6_3DSTATE_GS)(const struct ilo_dev_info *dev,
                           const struct ilo_shader_state *gs,
                           const struct ilo_shader_state *vs,
                           int verts_per_prim,
                           struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen6_3DSTATE_CLIP)(const struct ilo_dev_info *dev,
                             const struct ilo_rasterizer_state *rasterizer,
                             const struct ilo_shader_state *fs,
                             bool enable_guardband,
                             int num_viewports,
                             struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen6_3DSTATE_SF)(const struct ilo_dev_info *dev,
                           const struct ilo_rasterizer_state *rasterizer,
                           const struct ilo_shader_state *fs,
                           const struct ilo_shader_state *last_sh,
                           struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen6_3DSTATE_WM)(const struct ilo_dev_info *dev,
                           const struct ilo_shader_state *fs,
                           int num_samplers,
                           const struct ilo_rasterizer_state *rasterizer,
                           bool dual_blend, bool cc_may_kill,
                           struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen6_3DSTATE_CONSTANT_VS)(const struct ilo_dev_info *dev,
                                    const uint32_t *bufs, const int *sizes,
                                    int num_bufs,
                                    struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen6_3DSTATE_CONSTANT_GS)(const struct ilo_dev_info *dev,
                                    const uint32_t *bufs, const int *sizes,
                                    int num_bufs,
                                    struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen6_3DSTATE_CONSTANT_PS)(const struct ilo_dev_info *dev,
                                    const uint32_t *bufs, const int *sizes,
                                    int num_bufs,
                                    struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen6_3DSTATE_SAMPLE_MASK)(const struct ilo_dev_info *dev,
                                    unsigned sample_mask,
                                    struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen6_3DSTATE_DRAWING_RECTANGLE)(const struct ilo_dev_info *dev,
                                          unsigned x, unsigned y,
                                          unsigned width, unsigned height,
                                          struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen6_3DSTATE_DEPTH_BUFFER)(const struct ilo_dev_info *dev,
                                     const struct ilo_zs_surface *zs,
                                     struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen6_3DSTATE_POLY_STIPPLE_OFFSET)(const struct ilo_dev_info *dev,
                                            int x_offset, int y_offset,
                                            struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen6_3DSTATE_POLY_STIPPLE_PATTERN)(const struct ilo_dev_info *dev,
                                             const struct pipe_poly_stipple *pattern,
                                             struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen6_3DSTATE_LINE_STIPPLE)(const struct ilo_dev_info *dev,
                                     unsigned pattern, unsigned factor,
                                     struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen6_3DSTATE_AA_LINE_PARAMETERS)(const struct ilo_dev_info *dev,
                                           struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen6_3DSTATE_GS_SVB_INDEX)(const struct ilo_dev_info *dev,
                                     int index, unsigned svbi,
                                     unsigned max_svbi,
                                     bool load_vertex_count,
                                     struct ilo_cp *cp);


typedef void
(*ilo_gpe_gen6_3DSTATE_MULTISAMPLE)(const struct ilo_dev_info *dev,
                                    int num_samples,
                                    const uint32_t *packed_sample_pos,
                                    bool pixel_location_center,
                                    struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen6_3DSTATE_STENCIL_BUFFER)(const struct ilo_dev_info *dev,
                                       const struct ilo_zs_surface *zs,
                                       struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen6_3DSTATE_HIER_DEPTH_BUFFER)(const struct ilo_dev_info *dev,
                                          const struct ilo_zs_surface *zs,
                                          struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen6_3DSTATE_CLEAR_PARAMS)(const struct ilo_dev_info *dev,
                                     uint32_t clear_val,
                                     struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen6_PIPE_CONTROL)(const struct ilo_dev_info *dev,
                             uint32_t dw1,
                             struct intel_bo *bo, uint32_t bo_offset,
                             bool write_qword,
                             struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen6_3DPRIMITIVE)(const struct ilo_dev_info *dev,
                            const struct pipe_draw_info *info,
                            const struct ilo_ib_state *ib,
                            bool rectlist,
                            struct ilo_cp *cp);

typedef uint32_t
(*ilo_gpe_gen6_INTERFACE_DESCRIPTOR_DATA)(const struct ilo_dev_info *dev,
                                          const struct ilo_shader_state **cs,
                                          uint32_t *sampler_state,
                                          int *num_samplers,
                                          uint32_t *binding_table_state,
                                          int *num_surfaces,
                                          int num_ids,
                                          struct ilo_cp *cp);
typedef uint32_t
(*ilo_gpe_gen6_SF_VIEWPORT)(const struct ilo_dev_info *dev,
                            const struct ilo_viewport_cso *viewports,
                            unsigned num_viewports,
                            struct ilo_cp *cp);

typedef uint32_t
(*ilo_gpe_gen6_CLIP_VIEWPORT)(const struct ilo_dev_info *dev,
                              const struct ilo_viewport_cso *viewports,
                              unsigned num_viewports,
                              struct ilo_cp *cp);

typedef uint32_t
(*ilo_gpe_gen6_CC_VIEWPORT)(const struct ilo_dev_info *dev,
                            const struct ilo_viewport_cso *viewports,
                            unsigned num_viewports,
                            struct ilo_cp *cp);

typedef uint32_t
(*ilo_gpe_gen6_COLOR_CALC_STATE)(const struct ilo_dev_info *dev,
                                 const struct pipe_stencil_ref *stencil_ref,
                                 float alpha_ref,
                                 const struct pipe_blend_color *blend_color,
                                 struct ilo_cp *cp);

typedef uint32_t
(*ilo_gpe_gen6_BLEND_STATE)(const struct ilo_dev_info *dev,
                            const struct ilo_blend_state *blend,
                            const struct ilo_fb_state *fb,
                            const struct pipe_alpha_state *alpha,
                            struct ilo_cp *cp);

typedef uint32_t
(*ilo_gpe_gen6_DEPTH_STENCIL_STATE)(const struct ilo_dev_info *dev,
                                    const struct ilo_dsa_state *dsa,
                                    struct ilo_cp *cp);

typedef uint32_t
(*ilo_gpe_gen6_SCISSOR_RECT)(const struct ilo_dev_info *dev,
                             const struct ilo_scissor_state *scissor,
                             unsigned num_viewports,
                             struct ilo_cp *cp);

typedef uint32_t
(*ilo_gpe_gen6_BINDING_TABLE_STATE)(const struct ilo_dev_info *dev,
                                    uint32_t *surface_states,
                                    int num_surface_states,
                                    struct ilo_cp *cp);

typedef uint32_t
(*ilo_gpe_gen6_SURFACE_STATE)(const struct ilo_dev_info *dev,
                              const struct ilo_view_surface *surface,
                              bool for_render,
                              struct ilo_cp *cp);

typedef uint32_t
(*ilo_gpe_gen6_so_SURFACE_STATE)(const struct ilo_dev_info *dev,
                                 const struct pipe_stream_output_target *so,
                                 const struct pipe_stream_output_info *so_info,
                                 int so_index,
                                 struct ilo_cp *cp);

typedef uint32_t
(*ilo_gpe_gen6_SAMPLER_STATE)(const struct ilo_dev_info *dev,
                              const struct ilo_sampler_cso * const *samplers,
                              const struct pipe_sampler_view * const *views,
                              const uint32_t *sampler_border_colors,
                              int num_samplers,
                              struct ilo_cp *cp);

typedef uint32_t
(*ilo_gpe_gen6_SAMPLER_BORDER_COLOR_STATE)(const struct ilo_dev_info *dev,
                                           const struct ilo_sampler_cso *sampler,
                                           struct ilo_cp *cp);

typedef uint32_t
(*ilo_gpe_gen6_push_constant_buffer)(const struct ilo_dev_info *dev,
                                     int size, void **pcb,
                                     struct ilo_cp *cp);

/**
 * GEN6 graphics processing engine
 *
 * This is a low-level interface.  It does not handle the interdependencies
 * between states.
 */
struct ilo_gpe_gen6 {
   int (*estimate_command_size)(const struct ilo_dev_info *dev,
                                enum ilo_gpe_gen6_command cmd,
                                int arg);

   int (*estimate_state_size)(const struct ilo_dev_info *dev,
                              enum ilo_gpe_gen6_state state,
                              int arg);

#define GEN6_EMIT(name) ilo_gpe_gen6_ ## name emit_ ## name
   GEN6_EMIT(STATE_BASE_ADDRESS);
   GEN6_EMIT(STATE_SIP);
   GEN6_EMIT(3DSTATE_VF_STATISTICS);
   GEN6_EMIT(PIPELINE_SELECT);
   GEN6_EMIT(MEDIA_VFE_STATE);
   GEN6_EMIT(MEDIA_CURBE_LOAD);
   GEN6_EMIT(MEDIA_INTERFACE_DESCRIPTOR_LOAD);
   GEN6_EMIT(MEDIA_GATEWAY_STATE);
   GEN6_EMIT(MEDIA_STATE_FLUSH);
   GEN6_EMIT(MEDIA_OBJECT_WALKER);
   GEN6_EMIT(3DSTATE_BINDING_TABLE_POINTERS);
   GEN6_EMIT(3DSTATE_SAMPLER_STATE_POINTERS);
   GEN6_EMIT(3DSTATE_URB);
   GEN6_EMIT(3DSTATE_VERTEX_BUFFERS);
   GEN6_EMIT(3DSTATE_VERTEX_ELEMENTS);
   GEN6_EMIT(3DSTATE_INDEX_BUFFER);
   GEN6_EMIT(3DSTATE_VIEWPORT_STATE_POINTERS);
   GEN6_EMIT(3DSTATE_CC_STATE_POINTERS);
   GEN6_EMIT(3DSTATE_SCISSOR_STATE_POINTERS);
   GEN6_EMIT(3DSTATE_VS);
   GEN6_EMIT(3DSTATE_GS);
   GEN6_EMIT(3DSTATE_CLIP);
   GEN6_EMIT(3DSTATE_SF);
   GEN6_EMIT(3DSTATE_WM);
   GEN6_EMIT(3DSTATE_CONSTANT_VS);
   GEN6_EMIT(3DSTATE_CONSTANT_GS);
   GEN6_EMIT(3DSTATE_CONSTANT_PS);
   GEN6_EMIT(3DSTATE_SAMPLE_MASK);
   GEN6_EMIT(3DSTATE_DRAWING_RECTANGLE);
   GEN6_EMIT(3DSTATE_DEPTH_BUFFER);
   GEN6_EMIT(3DSTATE_POLY_STIPPLE_OFFSET);
   GEN6_EMIT(3DSTATE_POLY_STIPPLE_PATTERN);
   GEN6_EMIT(3DSTATE_LINE_STIPPLE);
   GEN6_EMIT(3DSTATE_AA_LINE_PARAMETERS);
   GEN6_EMIT(3DSTATE_GS_SVB_INDEX);
   GEN6_EMIT(3DSTATE_MULTISAMPLE);
   GEN6_EMIT(3DSTATE_STENCIL_BUFFER);
   GEN6_EMIT(3DSTATE_HIER_DEPTH_BUFFER);
   GEN6_EMIT(3DSTATE_CLEAR_PARAMS);
   GEN6_EMIT(PIPE_CONTROL);
   GEN6_EMIT(3DPRIMITIVE);
   GEN6_EMIT(INTERFACE_DESCRIPTOR_DATA);
   GEN6_EMIT(SF_VIEWPORT);
   GEN6_EMIT(CLIP_VIEWPORT);
   GEN6_EMIT(CC_VIEWPORT);
   GEN6_EMIT(COLOR_CALC_STATE);
   GEN6_EMIT(BLEND_STATE);
   GEN6_EMIT(DEPTH_STENCIL_STATE);
   GEN6_EMIT(SCISSOR_RECT);
   GEN6_EMIT(BINDING_TABLE_STATE);
   GEN6_EMIT(SURFACE_STATE);
   GEN6_EMIT(so_SURFACE_STATE);
   GEN6_EMIT(SAMPLER_STATE);
   GEN6_EMIT(SAMPLER_BORDER_COLOR_STATE);
   GEN6_EMIT(push_constant_buffer);
#undef GEN6_EMIT
};

const struct ilo_gpe_gen6 *
ilo_gpe_gen6_get(void);

/* Below are helpers for other GENs */

int
ilo_gpe_gen6_translate_winsys_tiling(enum intel_tiling_mode tiling);

int
ilo_gpe_gen6_translate_pipe_prim(unsigned prim);

int
ilo_gpe_gen6_translate_texture(enum pipe_texture_target target);

void
ilo_gpe_gen6_fill_3dstate_sf_raster(const struct ilo_dev_info *dev,
                                    const struct ilo_rasterizer_state *rasterizer,
                                    int num_samples,
                                    enum pipe_format depth_format,
                                    uint32_t *payload, unsigned payload_len);

void
ilo_gpe_gen6_fill_3dstate_sf_sbe(const struct ilo_dev_info *dev,
                                 const struct ilo_rasterizer_state *rasterizer,
                                 const struct ilo_shader_state *fs,
                                 const struct ilo_shader_state *last_sh,
                                 uint32_t *dw, int num_dwords);

#endif /* ILO_GPE_GEN6_H */

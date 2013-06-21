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

#ifndef ILO_GPE_GEN7_H
#define ILO_GPE_GEN7_H

#include "ilo_common.h"
#include "ilo_gpe_gen6.h"

/**
 * Commands that GEN7 GPE could emit.
 */
enum ilo_gpe_gen7_command {
   ILO_GPE_GEN7_STATE_BASE_ADDRESS,                  /* (0x0, 0x1, 0x01) */
   ILO_GPE_GEN7_STATE_SIP,                           /* (0x0, 0x1, 0x02) */
   ILO_GPE_GEN7_3DSTATE_VF_STATISTICS,               /* (0x1, 0x0, 0x0b) */
   ILO_GPE_GEN7_PIPELINE_SELECT,                     /* (0x1, 0x1, 0x04) */
   ILO_GPE_GEN7_MEDIA_VFE_STATE,                     /* (0x2, 0x0, 0x00) */
   ILO_GPE_GEN7_MEDIA_CURBE_LOAD,                    /* (0x2, 0x0, 0x01) */
   ILO_GPE_GEN7_MEDIA_INTERFACE_DESCRIPTOR_LOAD,     /* (0x2, 0x0, 0x02) */
   ILO_GPE_GEN7_MEDIA_STATE_FLUSH,                   /* (0x2, 0x0, 0x04) */
   ILO_GPE_GEN7_GPGPU_WALKER,                        /* (0x2, 0x1, 0x05) */
   ILO_GPE_GEN7_3DSTATE_CLEAR_PARAMS,                /* (0x3, 0x0, 0x04) */
   ILO_GPE_GEN7_3DSTATE_DEPTH_BUFFER,                /* (0x3, 0x0, 0x05) */
   ILO_GPE_GEN7_3DSTATE_STENCIL_BUFFER,              /* (0x3, 0x0, 0x06) */
   ILO_GPE_GEN7_3DSTATE_HIER_DEPTH_BUFFER,           /* (0x3, 0x0, 0x07) */
   ILO_GPE_GEN7_3DSTATE_VERTEX_BUFFERS,              /* (0x3, 0x0, 0x08) */
   ILO_GPE_GEN7_3DSTATE_VERTEX_ELEMENTS,             /* (0x3, 0x0, 0x09) */
   ILO_GPE_GEN7_3DSTATE_INDEX_BUFFER,                /* (0x3, 0x0, 0x0a) */
   ILO_GPE_GEN7_3DSTATE_CC_STATE_POINTERS,           /* (0x3, 0x0, 0x0e) */
   ILO_GPE_GEN7_3DSTATE_SCISSOR_STATE_POINTERS,      /* (0x3, 0x0, 0x0f) */
   ILO_GPE_GEN7_3DSTATE_VS,                          /* (0x3, 0x0, 0x10) */
   ILO_GPE_GEN7_3DSTATE_GS,                          /* (0x3, 0x0, 0x11) */
   ILO_GPE_GEN7_3DSTATE_CLIP,                        /* (0x3, 0x0, 0x12) */
   ILO_GPE_GEN7_3DSTATE_SF,                          /* (0x3, 0x0, 0x13) */
   ILO_GPE_GEN7_3DSTATE_WM,                          /* (0x3, 0x0, 0x14) */
   ILO_GPE_GEN7_3DSTATE_CONSTANT_VS,                 /* (0x3, 0x0, 0x15) */
   ILO_GPE_GEN7_3DSTATE_CONSTANT_GS,                 /* (0x3, 0x0, 0x16) */
   ILO_GPE_GEN7_3DSTATE_CONSTANT_PS,                 /* (0x3, 0x0, 0x17) */
   ILO_GPE_GEN7_3DSTATE_SAMPLE_MASK,                 /* (0x3, 0x0, 0x18) */
   ILO_GPE_GEN7_3DSTATE_CONSTANT_HS,                 /* (0x3, 0x0, 0x19) */
   ILO_GPE_GEN7_3DSTATE_CONSTANT_DS,                 /* (0x3, 0x0, 0x1a) */
   ILO_GPE_GEN7_3DSTATE_HS,                          /* (0x3, 0x0, 0x1b) */
   ILO_GPE_GEN7_3DSTATE_TE,                          /* (0x3, 0x0, 0x1c) */
   ILO_GPE_GEN7_3DSTATE_DS,                          /* (0x3, 0x0, 0x1d) */
   ILO_GPE_GEN7_3DSTATE_STREAMOUT,                   /* (0x3, 0x0, 0x1e) */
   ILO_GPE_GEN7_3DSTATE_SBE,                         /* (0x3, 0x0, 0x1f) */
   ILO_GPE_GEN7_3DSTATE_PS,                          /* (0x3, 0x0, 0x20) */
   ILO_GPE_GEN7_3DSTATE_VIEWPORT_STATE_POINTERS_SF_CLIP, /* (0x3, 0x0, 0x21) */
   ILO_GPE_GEN7_3DSTATE_VIEWPORT_STATE_POINTERS_CC,  /* (0x3, 0x0, 0x23) */
   ILO_GPE_GEN7_3DSTATE_BLEND_STATE_POINTERS,        /* (0x3, 0x0, 0x24) */
   ILO_GPE_GEN7_3DSTATE_DEPTH_STENCIL_STATE_POINTERS, /* (0x3, 0x0, 0x25) */
   ILO_GPE_GEN7_3DSTATE_BINDING_TABLE_POINTERS_VS,   /* (0x3, 0x0, 0x26) */
   ILO_GPE_GEN7_3DSTATE_BINDING_TABLE_POINTERS_HS,   /* (0x3, 0x0, 0x27) */
   ILO_GPE_GEN7_3DSTATE_BINDING_TABLE_POINTERS_DS,   /* (0x3, 0x0, 0x28) */
   ILO_GPE_GEN7_3DSTATE_BINDING_TABLE_POINTERS_GS,   /* (0x3, 0x0, 0x29) */
   ILO_GPE_GEN7_3DSTATE_BINDING_TABLE_POINTERS_PS,   /* (0x3, 0x0, 0x2a) */
   ILO_GPE_GEN7_3DSTATE_SAMPLER_STATE_POINTERS_VS,   /* (0x3, 0x0, 0x2b) */
   ILO_GPE_GEN7_3DSTATE_SAMPLER_STATE_POINTERS_HS,   /* (0x3, 0x0, 0x2c) */
   ILO_GPE_GEN7_3DSTATE_SAMPLER_STATE_POINTERS_DS,   /* (0x3, 0x0, 0x2d) */
   ILO_GPE_GEN7_3DSTATE_SAMPLER_STATE_POINTERS_GS,   /* (0x3, 0x0, 0x2e) */
   ILO_GPE_GEN7_3DSTATE_SAMPLER_STATE_POINTERS_PS,   /* (0x3, 0x0, 0x2f) */
   ILO_GPE_GEN7_3DSTATE_URB_VS,                      /* (0x3, 0x0, 0x30) */
   ILO_GPE_GEN7_3DSTATE_URB_HS,                      /* (0x3, 0x0, 0x31) */
   ILO_GPE_GEN7_3DSTATE_URB_DS,                      /* (0x3, 0x0, 0x32) */
   ILO_GPE_GEN7_3DSTATE_URB_GS,                      /* (0x3, 0x0, 0x33) */
   ILO_GPE_GEN7_3DSTATE_DRAWING_RECTANGLE,           /* (0x3, 0x1, 0x00) */
   ILO_GPE_GEN7_3DSTATE_POLY_STIPPLE_OFFSET,         /* (0x3, 0x1, 0x06) */
   ILO_GPE_GEN7_3DSTATE_POLY_STIPPLE_PATTERN,        /* (0x3, 0x1, 0x07) */
   ILO_GPE_GEN7_3DSTATE_LINE_STIPPLE,                /* (0x3, 0x1, 0x08) */
   ILO_GPE_GEN7_3DSTATE_AA_LINE_PARAMETERS,          /* (0x3, 0x1, 0x0a) */
   ILO_GPE_GEN7_3DSTATE_MULTISAMPLE,                 /* (0x3, 0x1, 0x0d) */
   ILO_GPE_GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_VS,      /* (0x3, 0x1, 0x12) */
   ILO_GPE_GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_HS,      /* (0x3, 0x1, 0x13) */
   ILO_GPE_GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_DS,      /* (0x3, 0x1, 0x14) */
   ILO_GPE_GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_GS,      /* (0x3, 0x1, 0x15) */
   ILO_GPE_GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_PS,      /* (0x3, 0x1, 0x16) */
   ILO_GPE_GEN7_3DSTATE_SO_DECL_LIST,                /* (0x3, 0x1, 0x17) */
   ILO_GPE_GEN7_3DSTATE_SO_BUFFER,                   /* (0x3, 0x1, 0x18) */
   ILO_GPE_GEN7_PIPE_CONTROL,                        /* (0x3, 0x2, 0x00) */
   ILO_GPE_GEN7_3DPRIMITIVE,                         /* (0x3, 0x3, 0x00) */

   ILO_GPE_GEN7_COMMAND_COUNT,
};

/**
 * Indirect states that GEN7 GPE could emit.
 */
enum ilo_gpe_gen7_state {
   ILO_GPE_GEN7_INTERFACE_DESCRIPTOR_DATA,
   ILO_GPE_GEN7_SF_CLIP_VIEWPORT,
   ILO_GPE_GEN7_CC_VIEWPORT,
   ILO_GPE_GEN7_COLOR_CALC_STATE,
   ILO_GPE_GEN7_BLEND_STATE,
   ILO_GPE_GEN7_DEPTH_STENCIL_STATE,
   ILO_GPE_GEN7_SCISSOR_RECT,
   ILO_GPE_GEN7_BINDING_TABLE_STATE,
   ILO_GPE_GEN7_SURFACE_STATE,
   ILO_GPE_GEN7_SAMPLER_STATE,
   ILO_GPE_GEN7_SAMPLER_BORDER_COLOR_STATE,
   ILO_GPE_GEN7_PUSH_CONSTANT_BUFFER,

   ILO_GPE_GEN7_STATE_COUNT,
};

typedef ilo_gpe_gen6_STATE_BASE_ADDRESS ilo_gpe_gen7_STATE_BASE_ADDRESS;
typedef ilo_gpe_gen6_STATE_SIP ilo_gpe_gen7_STATE_SIP;
typedef ilo_gpe_gen6_3DSTATE_VF_STATISTICS ilo_gpe_gen7_3DSTATE_VF_STATISTICS;
typedef ilo_gpe_gen6_PIPELINE_SELECT ilo_gpe_gen7_PIPELINE_SELECT;
typedef ilo_gpe_gen6_MEDIA_VFE_STATE ilo_gpe_gen7_MEDIA_VFE_STATE;
typedef ilo_gpe_gen6_MEDIA_CURBE_LOAD ilo_gpe_gen7_MEDIA_CURBE_LOAD;
typedef ilo_gpe_gen6_MEDIA_INTERFACE_DESCRIPTOR_LOAD ilo_gpe_gen7_MEDIA_INTERFACE_DESCRIPTOR_LOAD;
typedef ilo_gpe_gen6_MEDIA_STATE_FLUSH ilo_gpe_gen7_MEDIA_STATE_FLUSH;

typedef void
(*ilo_gpe_gen7_GPGPU_WALKER)(const struct ilo_dev_info *dev,
                             struct ilo_cp *cp);

typedef ilo_gpe_gen6_3DSTATE_CLEAR_PARAMS ilo_gpe_gen7_3DSTATE_CLEAR_PARAMS;
typedef ilo_gpe_gen6_3DSTATE_DEPTH_BUFFER ilo_gpe_gen7_3DSTATE_DEPTH_BUFFER;
typedef ilo_gpe_gen6_3DSTATE_STENCIL_BUFFER ilo_gpe_gen7_3DSTATE_STENCIL_BUFFER;
typedef ilo_gpe_gen6_3DSTATE_HIER_DEPTH_BUFFER ilo_gpe_gen7_3DSTATE_HIER_DEPTH_BUFFER;
typedef ilo_gpe_gen6_3DSTATE_VERTEX_BUFFERS ilo_gpe_gen7_3DSTATE_VERTEX_BUFFERS;
typedef ilo_gpe_gen6_3DSTATE_VERTEX_ELEMENTS ilo_gpe_gen7_3DSTATE_VERTEX_ELEMENTS;
typedef ilo_gpe_gen6_3DSTATE_INDEX_BUFFER ilo_gpe_gen7_3DSTATE_INDEX_BUFFER;

typedef void
(*ilo_gpe_gen7_3DSTATE_CC_STATE_POINTERS)(const struct ilo_dev_info *dev,
                                          uint32_t color_calc_state,
                                          struct ilo_cp *cp);

typedef ilo_gpe_gen6_3DSTATE_SCISSOR_STATE_POINTERS ilo_gpe_gen7_3DSTATE_SCISSOR_STATE_POINTERS;
typedef ilo_gpe_gen6_3DSTATE_VS ilo_gpe_gen7_3DSTATE_VS;

typedef void
(*ilo_gpe_gen7_3DSTATE_GS)(const struct ilo_dev_info *dev,
                           const struct ilo_shader_state *gs,
                           int num_samplers,
                           struct ilo_cp *cp);

typedef ilo_gpe_gen6_3DSTATE_CLIP ilo_gpe_gen7_3DSTATE_CLIP;

typedef void
(*ilo_gpe_gen7_3DSTATE_SF)(const struct ilo_dev_info *dev,
                           const struct ilo_rasterizer_state *rasterizer,
                           const struct pipe_surface *zs_surf,
                           struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen7_3DSTATE_WM)(const struct ilo_dev_info *dev,
                           const struct ilo_shader_state *fs,
                           const struct ilo_rasterizer_state *rasterizer,
                           bool cc_may_kill,
                           struct ilo_cp *cp);

typedef ilo_gpe_gen6_3DSTATE_CONSTANT_VS ilo_gpe_gen7_3DSTATE_CONSTANT_VS;
typedef ilo_gpe_gen6_3DSTATE_CONSTANT_GS ilo_gpe_gen7_3DSTATE_CONSTANT_GS;
typedef ilo_gpe_gen6_3DSTATE_CONSTANT_PS ilo_gpe_gen7_3DSTATE_CONSTANT_PS;

typedef void
(*ilo_gpe_gen7_3DSTATE_SAMPLE_MASK)(const struct ilo_dev_info *dev,
                                    unsigned sample_mask,
                                    int num_samples,
                                    struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen7_3DSTATE_CONSTANT_HS)(const struct ilo_dev_info *dev,
                                    const uint32_t *bufs, const int *sizes,
                                    int num_bufs,
                                    struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen7_3DSTATE_CONSTANT_DS)(const struct ilo_dev_info *dev,
                                    const uint32_t *bufs, const int *sizes,
                                    int num_bufs,
                                    struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen7_3DSTATE_HS)(const struct ilo_dev_info *dev,
                           const struct ilo_shader_state *hs,
                           int num_samplers,
                           struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen7_3DSTATE_TE)(const struct ilo_dev_info *dev,
                           struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen7_3DSTATE_DS)(const struct ilo_dev_info *dev,
                           const struct ilo_shader_state *ds,
                           int num_samplers,
                           struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen7_3DSTATE_STREAMOUT)(const struct ilo_dev_info *dev,
                                  unsigned buffer_mask,
                                  int vertex_attrib_count,
                                  bool rasterizer_discard,
                                  struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen7_3DSTATE_SBE)(const struct ilo_dev_info *dev,
                            const struct ilo_rasterizer_state *rasterizer,
                            const struct ilo_shader_state *fs,
                            const struct ilo_shader_state *last_sh,
                            struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen7_3DSTATE_PS)(const struct ilo_dev_info *dev,
                           const struct ilo_shader_state *fs,
                           int num_samplers, bool dual_blend,
                           struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen7_3DSTATE_VIEWPORT_STATE_POINTERS_SF_CLIP)(const struct ilo_dev_info *dev,
                                                        uint32_t viewport,
                                                        struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen7_3DSTATE_VIEWPORT_STATE_POINTERS_CC)(const struct ilo_dev_info *dev,
                                                   uint32_t viewport,
                                                   struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen7_3DSTATE_BLEND_STATE_POINTERS)(const struct ilo_dev_info *dev,
                                             uint32_t blend,
                                             struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen7_3DSTATE_DEPTH_STENCIL_STATE_POINTERS)(const struct ilo_dev_info *dev,
                                                     uint32_t depth_stencil,
                                                     struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen7_3DSTATE_BINDING_TABLE_POINTERS_VS)(const struct ilo_dev_info *dev,
                                                  uint32_t binding_table,
                                                  struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen7_3DSTATE_BINDING_TABLE_POINTERS_HS)(const struct ilo_dev_info *dev,
                                                  uint32_t binding_table,
                                                  struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen7_3DSTATE_BINDING_TABLE_POINTERS_DS)(const struct ilo_dev_info *dev,
                                                  uint32_t binding_table,
                                                  struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen7_3DSTATE_BINDING_TABLE_POINTERS_GS)(const struct ilo_dev_info *dev,
                                                  uint32_t binding_table,
                                                  struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen7_3DSTATE_BINDING_TABLE_POINTERS_PS)(const struct ilo_dev_info *dev,
                                                  uint32_t binding_table,
                                                  struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen7_3DSTATE_SAMPLER_STATE_POINTERS_VS)(const struct ilo_dev_info *dev,
                                                  uint32_t sampler_state,
                                                  struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen7_3DSTATE_SAMPLER_STATE_POINTERS_HS)(const struct ilo_dev_info *dev,
                                                  uint32_t sampler_state,
                                                  struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen7_3DSTATE_SAMPLER_STATE_POINTERS_DS)(const struct ilo_dev_info *dev,
                                                  uint32_t sampler_state,
                                                  struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen7_3DSTATE_SAMPLER_STATE_POINTERS_GS)(const struct ilo_dev_info *dev,
                                                  uint32_t sampler_state,
                                                  struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen7_3DSTATE_SAMPLER_STATE_POINTERS_PS)(const struct ilo_dev_info *dev,
                                                  uint32_t sampler_state,
                                                  struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen7_3DSTATE_URB_VS)(const struct ilo_dev_info *dev,
                               int offset, int size, int entry_size,
                               struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen7_3DSTATE_URB_HS)(const struct ilo_dev_info *dev,
                               int offset, int size, int entry_size,
                               struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen7_3DSTATE_URB_DS)(const struct ilo_dev_info *dev,
                               int offset, int size, int entry_size,
                               struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen7_3DSTATE_URB_GS)(const struct ilo_dev_info *dev,
                               int offset, int size, int entry_size,
                               struct ilo_cp *cp);

typedef ilo_gpe_gen6_3DSTATE_DRAWING_RECTANGLE ilo_gpe_gen7_3DSTATE_DRAWING_RECTANGLE;
typedef ilo_gpe_gen6_3DSTATE_POLY_STIPPLE_OFFSET ilo_gpe_gen7_3DSTATE_POLY_STIPPLE_OFFSET;
typedef ilo_gpe_gen6_3DSTATE_POLY_STIPPLE_PATTERN ilo_gpe_gen7_3DSTATE_POLY_STIPPLE_PATTERN;
typedef ilo_gpe_gen6_3DSTATE_LINE_STIPPLE ilo_gpe_gen7_3DSTATE_LINE_STIPPLE;
typedef ilo_gpe_gen6_3DSTATE_AA_LINE_PARAMETERS ilo_gpe_gen7_3DSTATE_AA_LINE_PARAMETERS;
typedef ilo_gpe_gen6_3DSTATE_MULTISAMPLE ilo_gpe_gen7_3DSTATE_MULTISAMPLE;

typedef void
(*ilo_gpe_gen7_3DSTATE_PUSH_CONSTANT_ALLOC_VS)(const struct ilo_dev_info *dev,
                                               int offset, int size,
                                               struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen7_3DSTATE_PUSH_CONSTANT_ALLOC_HS)(const struct ilo_dev_info *dev,
                                               int offset, int size,
                                               struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen7_3DSTATE_PUSH_CONSTANT_ALLOC_DS)(const struct ilo_dev_info *dev,
                                               int offset, int size,
                                               struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen7_3DSTATE_PUSH_CONSTANT_ALLOC_GS)(const struct ilo_dev_info *dev,
                                               int offset, int size,
                                               struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen7_3DSTATE_PUSH_CONSTANT_ALLOC_PS)(const struct ilo_dev_info *dev,
                                               int offset, int size,
                                               struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen7_3DSTATE_SO_DECL_LIST)(const struct ilo_dev_info *dev,
                                     const struct pipe_stream_output_info *so_info,
                                     struct ilo_cp *cp);

typedef void
(*ilo_gpe_gen7_3DSTATE_SO_BUFFER)(const struct ilo_dev_info *dev,
                                  int index, int base, int stride,
                                  const struct pipe_stream_output_target *so_target,
                                  struct ilo_cp *cp);

typedef ilo_gpe_gen6_PIPE_CONTROL ilo_gpe_gen7_PIPE_CONTROL;
typedef ilo_gpe_gen6_3DPRIMITIVE ilo_gpe_gen7_3DPRIMITIVE;
typedef ilo_gpe_gen6_INTERFACE_DESCRIPTOR_DATA ilo_gpe_gen7_INTERFACE_DESCRIPTOR_DATA;

typedef uint32_t
(*ilo_gpe_gen7_SF_CLIP_VIEWPORT)(const struct ilo_dev_info *dev,
                                 const struct ilo_viewport_cso *viewports,
                                 unsigned num_viewports,
                                 struct ilo_cp *cp);

typedef ilo_gpe_gen6_CC_VIEWPORT ilo_gpe_gen7_CC_VIEWPORT;
typedef ilo_gpe_gen6_COLOR_CALC_STATE ilo_gpe_gen7_COLOR_CALC_STATE;
typedef ilo_gpe_gen6_BLEND_STATE ilo_gpe_gen7_BLEND_STATE;
typedef ilo_gpe_gen6_DEPTH_STENCIL_STATE ilo_gpe_gen7_DEPTH_STENCIL_STATE;
typedef ilo_gpe_gen6_SCISSOR_RECT ilo_gpe_gen7_SCISSOR_RECT;
typedef ilo_gpe_gen6_BINDING_TABLE_STATE ilo_gpe_gen7_BINDING_TABLE_STATE;
typedef ilo_gpe_gen6_SURFACE_STATE ilo_gpe_gen7_SURFACE_STATE;
typedef ilo_gpe_gen6_SAMPLER_STATE ilo_gpe_gen7_SAMPLER_STATE;
typedef ilo_gpe_gen6_SAMPLER_BORDER_COLOR_STATE ilo_gpe_gen7_SAMPLER_BORDER_COLOR_STATE;
typedef ilo_gpe_gen6_push_constant_buffer ilo_gpe_gen7_push_constant_buffer;

/**
 * GEN7 graphics processing engine
 *
 * \see ilo_gpe_gen6
 */
struct ilo_gpe_gen7 {
   int (*estimate_command_size)(const struct ilo_dev_info *dev,
                                enum ilo_gpe_gen7_command cmd,
                                int arg);

   int (*estimate_state_size)(const struct ilo_dev_info *dev,
                              enum ilo_gpe_gen7_state state,
                              int arg);

#define GEN7_EMIT(name) ilo_gpe_gen7_ ## name emit_ ## name
   GEN7_EMIT(STATE_BASE_ADDRESS);
   GEN7_EMIT(STATE_SIP);
   GEN7_EMIT(3DSTATE_VF_STATISTICS);
   GEN7_EMIT(PIPELINE_SELECT);
   GEN7_EMIT(MEDIA_VFE_STATE);
   GEN7_EMIT(MEDIA_CURBE_LOAD);
   GEN7_EMIT(MEDIA_INTERFACE_DESCRIPTOR_LOAD);
   GEN7_EMIT(MEDIA_STATE_FLUSH);
   GEN7_EMIT(GPGPU_WALKER);
   GEN7_EMIT(3DSTATE_CLEAR_PARAMS);
   GEN7_EMIT(3DSTATE_DEPTH_BUFFER);
   GEN7_EMIT(3DSTATE_STENCIL_BUFFER);
   GEN7_EMIT(3DSTATE_HIER_DEPTH_BUFFER);
   GEN7_EMIT(3DSTATE_VERTEX_BUFFERS);
   GEN7_EMIT(3DSTATE_VERTEX_ELEMENTS);
   GEN7_EMIT(3DSTATE_INDEX_BUFFER);
   GEN7_EMIT(3DSTATE_CC_STATE_POINTERS);
   GEN7_EMIT(3DSTATE_SCISSOR_STATE_POINTERS);
   GEN7_EMIT(3DSTATE_VS);
   GEN7_EMIT(3DSTATE_GS);
   GEN7_EMIT(3DSTATE_CLIP);
   GEN7_EMIT(3DSTATE_SF);
   GEN7_EMIT(3DSTATE_WM);
   GEN7_EMIT(3DSTATE_CONSTANT_VS);
   GEN7_EMIT(3DSTATE_CONSTANT_GS);
   GEN7_EMIT(3DSTATE_CONSTANT_PS);
   GEN7_EMIT(3DSTATE_SAMPLE_MASK);
   GEN7_EMIT(3DSTATE_CONSTANT_HS);
   GEN7_EMIT(3DSTATE_CONSTANT_DS);
   GEN7_EMIT(3DSTATE_HS);
   GEN7_EMIT(3DSTATE_TE);
   GEN7_EMIT(3DSTATE_DS);
   GEN7_EMIT(3DSTATE_STREAMOUT);
   GEN7_EMIT(3DSTATE_SBE);
   GEN7_EMIT(3DSTATE_PS);
   GEN7_EMIT(3DSTATE_VIEWPORT_STATE_POINTERS_SF_CLIP);
   GEN7_EMIT(3DSTATE_VIEWPORT_STATE_POINTERS_CC);
   GEN7_EMIT(3DSTATE_BLEND_STATE_POINTERS);
   GEN7_EMIT(3DSTATE_DEPTH_STENCIL_STATE_POINTERS);
   GEN7_EMIT(3DSTATE_BINDING_TABLE_POINTERS_VS);
   GEN7_EMIT(3DSTATE_BINDING_TABLE_POINTERS_HS);
   GEN7_EMIT(3DSTATE_BINDING_TABLE_POINTERS_DS);
   GEN7_EMIT(3DSTATE_BINDING_TABLE_POINTERS_GS);
   GEN7_EMIT(3DSTATE_BINDING_TABLE_POINTERS_PS);
   GEN7_EMIT(3DSTATE_SAMPLER_STATE_POINTERS_VS);
   GEN7_EMIT(3DSTATE_SAMPLER_STATE_POINTERS_HS);
   GEN7_EMIT(3DSTATE_SAMPLER_STATE_POINTERS_DS);
   GEN7_EMIT(3DSTATE_SAMPLER_STATE_POINTERS_GS);
   GEN7_EMIT(3DSTATE_SAMPLER_STATE_POINTERS_PS);
   GEN7_EMIT(3DSTATE_URB_VS);
   GEN7_EMIT(3DSTATE_URB_HS);
   GEN7_EMIT(3DSTATE_URB_DS);
   GEN7_EMIT(3DSTATE_URB_GS);
   GEN7_EMIT(3DSTATE_DRAWING_RECTANGLE);
   GEN7_EMIT(3DSTATE_POLY_STIPPLE_OFFSET);
   GEN7_EMIT(3DSTATE_POLY_STIPPLE_PATTERN);
   GEN7_EMIT(3DSTATE_LINE_STIPPLE);
   GEN7_EMIT(3DSTATE_AA_LINE_PARAMETERS);
   GEN7_EMIT(3DSTATE_MULTISAMPLE);
   GEN7_EMIT(3DSTATE_PUSH_CONSTANT_ALLOC_VS);
   GEN7_EMIT(3DSTATE_PUSH_CONSTANT_ALLOC_HS);
   GEN7_EMIT(3DSTATE_PUSH_CONSTANT_ALLOC_DS);
   GEN7_EMIT(3DSTATE_PUSH_CONSTANT_ALLOC_GS);
   GEN7_EMIT(3DSTATE_PUSH_CONSTANT_ALLOC_PS);
   GEN7_EMIT(3DSTATE_SO_DECL_LIST);
   GEN7_EMIT(3DSTATE_SO_BUFFER);
   GEN7_EMIT(PIPE_CONTROL);
   GEN7_EMIT(3DPRIMITIVE);
   GEN7_EMIT(INTERFACE_DESCRIPTOR_DATA);
   GEN7_EMIT(SF_CLIP_VIEWPORT);
   GEN7_EMIT(CC_VIEWPORT);
   GEN7_EMIT(COLOR_CALC_STATE);
   GEN7_EMIT(BLEND_STATE);
   GEN7_EMIT(DEPTH_STENCIL_STATE);
   GEN7_EMIT(SCISSOR_RECT);
   GEN7_EMIT(BINDING_TABLE_STATE);
   GEN7_EMIT(SURFACE_STATE);
   GEN7_EMIT(SAMPLER_STATE);
   GEN7_EMIT(SAMPLER_BORDER_COLOR_STATE);
   GEN7_EMIT(push_constant_buffer);
#undef GEN7_EMIT
};

const struct ilo_gpe_gen7 *
ilo_gpe_gen7_get(void);

#endif /* ILO_GPE_GEN7_H */

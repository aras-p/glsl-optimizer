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

#ifndef ILO_SHADER_H
#define ILO_SHADER_H

#include "ilo_common.h"

enum ilo_kernel_param {
   ILO_KERNEL_INPUT_COUNT,
   ILO_KERNEL_OUTPUT_COUNT,
   ILO_KERNEL_URB_DATA_START_REG,
   ILO_KERNEL_SKIP_CBUF0_UPLOAD,
   ILO_KERNEL_PCB_CBUF0_SIZE,

   ILO_KERNEL_VS_INPUT_INSTANCEID,
   ILO_KERNEL_VS_INPUT_VERTEXID,
   ILO_KERNEL_VS_INPUT_EDGEFLAG,
   ILO_KERNEL_VS_PCB_UCP_SIZE,
   ILO_KERNEL_VS_GEN6_SO,
   ILO_KERNEL_VS_GEN6_SO_START_REG,
   ILO_KERNEL_VS_GEN6_SO_POINT_OFFSET,
   ILO_KERNEL_VS_GEN6_SO_LINE_OFFSET,
   ILO_KERNEL_VS_GEN6_SO_TRI_OFFSET,

   ILO_KERNEL_GS_DISCARD_ADJACENCY,
   ILO_KERNEL_GS_GEN6_SVBI_POST_INC,

   ILO_KERNEL_FS_INPUT_Z,
   ILO_KERNEL_FS_INPUT_W,
   ILO_KERNEL_FS_OUTPUT_Z,
   ILO_KERNEL_FS_USE_KILL,
   ILO_KERNEL_FS_BARYCENTRIC_INTERPOLATIONS,
   ILO_KERNEL_FS_DISPATCH_16_OFFSET,

   ILO_KERNEL_PARAM_COUNT,
};

struct ilo_kernel_routing {
   uint32_t const_interp_enable;
   uint32_t point_sprite_enable;
   unsigned source_skip, source_len;

   bool swizzle_enable;
   uint16_t swizzles[16];
};

struct intel_bo;
struct ilo_context;
struct ilo_rasterizer_state;
struct ilo_shader_cache;
struct ilo_shader_state;
struct ilo_shader_cso;

struct ilo_shader_cache *
ilo_shader_cache_create(void);

void
ilo_shader_cache_destroy(struct ilo_shader_cache *shc);

void
ilo_shader_cache_add(struct ilo_shader_cache *shc,
                     struct ilo_shader_state *shader);

void
ilo_shader_cache_remove(struct ilo_shader_cache *shc,
                        struct ilo_shader_state *shader);

int
ilo_shader_cache_upload(struct ilo_shader_cache *shc,
                        struct intel_bo *bo, unsigned offset,
                        bool incremental);

struct ilo_shader_state *
ilo_shader_create_vs(const struct ilo_dev_info *dev,
                     const struct pipe_shader_state *state,
                     const struct ilo_context *precompile);

struct ilo_shader_state *
ilo_shader_create_gs(const struct ilo_dev_info *dev,
                     const struct pipe_shader_state *state,
                     const struct ilo_context *precompile);

struct ilo_shader_state *
ilo_shader_create_fs(const struct ilo_dev_info *dev,
                     const struct pipe_shader_state *state,
                     const struct ilo_context *precompile);

struct ilo_shader_state *
ilo_shader_create_cs(const struct ilo_dev_info *dev,
                     const struct pipe_compute_state *state,
                     const struct ilo_context *precompile);

void
ilo_shader_destroy(struct ilo_shader_state *shader);

int
ilo_shader_get_type(const struct ilo_shader_state *shader);

bool
ilo_shader_select_kernel(struct ilo_shader_state *shader,
                         const struct ilo_context *ilo,
                         uint32_t dirty);

bool
ilo_shader_select_kernel_routing(struct ilo_shader_state *shader,
                                 const struct ilo_shader_state *source,
                                 const struct ilo_rasterizer_state *rasterizer);

uint32_t
ilo_shader_get_kernel_offset(const struct ilo_shader_state *shader);

int
ilo_shader_get_kernel_param(const struct ilo_shader_state *shader,
                            enum ilo_kernel_param param);

const struct ilo_shader_cso *
ilo_shader_get_kernel_cso(const struct ilo_shader_state *shader);

const struct pipe_stream_output_info *
ilo_shader_get_kernel_so_info(const struct ilo_shader_state *shader);

const struct ilo_kernel_routing *
ilo_shader_get_kernel_routing(const struct ilo_shader_state *shader);

#endif /* ILO_SHADER_H */

/**************************************************************************
 *
 * Copyright 2007 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/
#ifndef P_SCREEN_H
#define P_SCREEN_H

/*
 * glsl-optimizer change: minimal p_screen.h. The GLSL compiler only reads the
 * NIR options and a few capabilities from the gallium screen. Struct
 * definitions are from Mesa 26.2.6 pipe/p_screen.h and pipe/p_defines.h;
 * pipe_caps and pipe_screen keep only the members the compiler uses.
 */

#include <stdbool.h>
#include "util/compiler.h"
#include "compiler/shader_enums.h"

#ifdef __cplusplus
extern "C" {
#endif

struct nir_shader_compiler_options;

struct pipe_shader_caps {
   unsigned max_instructions; /* if 0, it means the stage is unsupported */
   unsigned max_alu_instructions;
   unsigned max_tex_instructions;
   unsigned max_tex_indirections;
   unsigned max_control_flow_depth;
   unsigned max_inputs;
   unsigned max_outputs;
   unsigned max_const_buffer0_size;
   unsigned max_const_buffers;
   unsigned max_temps;
   unsigned max_texture_samplers;
   unsigned max_sampler_views;
   unsigned max_shader_buffers;
   unsigned max_shader_images;
   unsigned max_hw_atomic_counters;
   unsigned max_hw_atomic_counter_buffers;
   unsigned supported_irs;

   bool cont_supported;
   bool indirect_temp_addr;
   bool indirect_const_addr;
   bool subroutines; /* BGNSUB, ENDSUB, CAL, RET */
   bool integers;
   bool int64_atomics;
   bool fp16;
   bool fp16_derivatives;
   bool fp16_const_buffers;
   bool fp16_no_denorms;
   bool int16;
   bool glsl_16bit_consts;
   bool glsl_16bit_load_dst; /* fp16 or int16 is AND'ed with this */
   bool tgsi_sqrt_supported;
   bool tgsi_any_inout_decl_range;
};

struct pipe_mesh_caps {
   unsigned max_task_work_group_total_count;
   unsigned max_mesh_work_group_total_count;
   unsigned max_mesh_work_group_invocations;
   unsigned max_task_work_group_invocations;
   unsigned max_task_payload_size;
   unsigned max_task_shared_memory_size;
   unsigned max_mesh_shared_memory_size;
   unsigned max_task_payload_and_shared_memory_size;
   unsigned max_mesh_payload_and_shared_memory_size;
   unsigned max_mesh_output_memory_size;
   unsigned max_mesh_payload_and_output_memory_size;
   unsigned max_mesh_output_vertices;
   unsigned max_mesh_output_primitives;
   unsigned max_mesh_output_components;
   unsigned max_mesh_output_layers;
   unsigned max_mesh_multiview_view_count;
   unsigned mesh_output_per_vertex_granularity;
   unsigned mesh_output_per_primitive_granularity;
   unsigned max_preferred_task_work_group_invocations;
   unsigned max_preferred_mesh_work_group_invocations;
   bool mesh_prefers_local_invocation_vertex_output;
   bool mesh_prefers_local_invocation_primitive_output;
   bool mesh_prefers_compact_vertex_output;
   bool mesh_prefers_compact_primitive_output;
   unsigned max_task_work_group_count[3];
   unsigned max_mesh_work_group_count[3];
   unsigned max_task_work_group_size[3];
   unsigned max_mesh_work_group_size[3];
   bool pipeline_statistic_queries;
};

/** Subset of upstream struct pipe_caps */
struct pipe_caps {
   bool packed_uniforms;
   unsigned shader_pixel_local_storage_size;
   struct pipe_mesh_caps mesh;
};

/** Subset of upstream struct pipe_screen */
struct pipe_screen {
   const struct pipe_caps caps;
   const struct pipe_shader_caps shader_caps[MESA_SHADER_MESH_STAGES];
   const struct nir_shader_compiler_options *nir_options[MESA_SHADER_MESH_STAGES];
};

#ifdef __cplusplus
}
#endif

#endif /* P_SCREEN_H */

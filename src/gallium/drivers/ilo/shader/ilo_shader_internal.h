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

#ifndef ILO_SHADER_INTERNAL_H
#define ILO_SHADER_INTERNAL_H

#include "ilo_common.h"
#include "ilo_context.h"
#include "ilo_shader.h"

/* XXX The interface needs to be reworked */

/**
 * A shader variant.  It consists of non-orthogonal states of the pipe context
 * affecting the compilation of a shader.
 */
struct ilo_shader_variant {
   union {
      struct {
         bool rasterizer_discard;
         int num_ucps;
      } vs;

      struct {
         bool rasterizer_discard;
         int num_inputs;
         int semantic_names[PIPE_MAX_SHADER_INPUTS];
         int semantic_indices[PIPE_MAX_SHADER_INPUTS];
      } gs;

      struct {
         bool flatshade;
         int fb_height;
         int num_cbufs;
      } fs;
   } u;

   bool use_pcb;

   int num_sampler_views;
   struct {
      unsigned r:3;
      unsigned g:3;
      unsigned b:3;
      unsigned a:3;
   } sampler_view_swizzles[ILO_MAX_SAMPLER_VIEWS];

   uint32_t saturate_tex_coords[3];
};

/**
 * A compiled shader.
 */
struct ilo_shader {
   struct ilo_shader_variant variant;

   struct ilo_shader_cso cso;

   struct {
      int semantic_names[PIPE_MAX_SHADER_INPUTS];
      int semantic_indices[PIPE_MAX_SHADER_INPUTS];
      int interp[PIPE_MAX_SHADER_INPUTS];
      bool centroid[PIPE_MAX_SHADER_INPUTS];
      int count;

      int start_grf;
      bool has_pos;
      bool has_linear_interp;
      int barycentric_interpolation_mode;
      uint32_t const_interp_enable;
      bool discard_adj;
   } in;

   struct {
      int register_indices[PIPE_MAX_SHADER_OUTPUTS];
      int semantic_names[PIPE_MAX_SHADER_OUTPUTS];
      int semantic_indices[PIPE_MAX_SHADER_OUTPUTS];
      int count;

      bool has_pos;
   } out;

   bool skip_cbuf0_upload;

   bool has_kill;
   bool dispatch_16;

   bool stream_output;
   int svbi_post_inc;
   struct pipe_stream_output_info so_info;

   /* for VS stream output / rasterizer discard */
   int gs_offsets[3];
   int gs_start_grf;

   void *kernel;
   int kernel_size;

   bool routing_initialized;
   int routing_src_semantics[PIPE_MAX_SHADER_OUTPUTS];
   int routing_src_indices[PIPE_MAX_SHADER_OUTPUTS];
   uint32_t routing_sprite_coord_enable;
   struct ilo_kernel_routing routing;

   /* what does the push constant buffer consist of? */
   struct {
      int cbuf0_size;
      int clip_state_size;
   } pcb;

   struct list_head list;

   /* managed by shader cache */
   bool uploaded;
   uint32_t cache_offset;
};

/**
 * Information about a shader state.
 */
struct ilo_shader_info {
   const struct ilo_dev_info *dev;
   int type;

   const struct tgsi_token *tokens;

   struct pipe_stream_output_info stream_output;
   struct {
      unsigned req_local_mem;
      unsigned req_private_mem;
      unsigned req_input_mem;
   } compute;

   uint32_t non_orthogonal_states;

   bool has_color_interp;
   bool has_pos;
   bool has_vertexid;
   bool has_instanceid;
   bool fs_color0_writes_all_cbufs;

   int edgeflag_in;
   int edgeflag_out;

   uint32_t shadow_samplers;
   int num_samplers;
};

/**
 * A shader state.
 */
struct ilo_shader_state {
   struct ilo_shader_info info;

   struct list_head variants;
   int num_variants, total_size;

   struct ilo_shader *shader;

   /* managed by shader cache */
   struct ilo_shader_cache *cache;
   struct list_head list;
};

void
ilo_shader_variant_init(struct ilo_shader_variant *variant,
                        const struct ilo_shader_info *info,
                        const struct ilo_context *ilo);

bool
ilo_shader_state_use_variant(struct ilo_shader_state *state,
                             const struct ilo_shader_variant *variant);

struct ilo_shader *
ilo_shader_compile_vs(const struct ilo_shader_state *state,
                      const struct ilo_shader_variant *variant);

struct ilo_shader *
ilo_shader_compile_gs(const struct ilo_shader_state *state,
                      const struct ilo_shader_variant *variant);

bool
ilo_shader_compile_gs_passthrough(const struct ilo_shader_state *vs_state,
                                  const struct ilo_shader_variant *vs_variant,
                                  const int *so_mapping,
                                  struct ilo_shader *vs);

struct ilo_shader *
ilo_shader_compile_fs(const struct ilo_shader_state *state,
                      const struct ilo_shader_variant *variant);

struct ilo_shader *
ilo_shader_compile_cs(const struct ilo_shader_state *state,
                      const struct ilo_shader_variant *variant);

static inline void
ilo_shader_destroy_kernel(struct ilo_shader *sh)
{
   FREE(sh->kernel);
   FREE(sh);
}

#endif /* ILO_SHADER_INTERNAL_H */

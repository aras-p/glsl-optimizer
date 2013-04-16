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

#ifndef ILO_CONTEXT_H
#define ILO_CONTEXT_H

#include "pipe/p_context.h"

#include "ilo_common.h"

/**
 * \see brw_context.h
 */
#define ILO_MAX_DRAW_BUFFERS    8
#define ILO_MAX_CONST_BUFFERS   1
#define ILO_MAX_SAMPLER_VIEWS   16
#define ILO_MAX_SAMPLERS        16
#define ILO_MAX_SO_BINDINGS     64
#define ILO_MAX_SO_BUFFERS      4

#define ILO_MAX_VS_SURFACES        (ILO_MAX_CONST_BUFFERS + ILO_MAX_SAMPLER_VIEWS)
#define ILO_VS_CONST_SURFACE(i)    (i)
#define ILO_VS_TEXTURE_SURFACE(i)  (ILO_MAX_CONST_BUFFERS  + i)

#define ILO_MAX_GS_SURFACES        (ILO_MAX_SO_BINDINGS)
#define ILO_GS_SO_SURFACE(i)       (i)

#define ILO_MAX_WM_SURFACES        (ILO_MAX_DRAW_BUFFERS + ILO_MAX_CONST_BUFFERS + ILO_MAX_SAMPLER_VIEWS)
#define ILO_WM_DRAW_SURFACE(i)     (i)
#define ILO_WM_CONST_SURFACE(i)    (ILO_MAX_DRAW_BUFFERS + i)
#define ILO_WM_TEXTURE_SURFACE(i)  (ILO_MAX_DRAW_BUFFERS + ILO_MAX_CONST_BUFFERS  + i)

struct intel_winsys;
struct intel_bo;
struct ilo_cp;
struct ilo_screen;
struct ilo_shader_state;

struct ilo_vertex_element {
   struct pipe_vertex_element elements[PIPE_MAX_ATTRIBS];
   unsigned num_elements;
};

struct ilo_context {
   struct pipe_context base;

   struct intel_winsys *winsys;
   int devid;
   int gen;
   int gt;

   int max_vs_threads;
   int max_gs_threads;
   int max_wm_threads;
   struct {
      int size;
      int max_vs_entries;
      int max_gs_entries;
   } urb;

   struct ilo_cp *cp;
   struct intel_bo *last_cp_bo;

   struct ilo_shader_cache *shader_cache;

   uint32_t dirty;

   struct pipe_blend_state *blend;
   struct pipe_rasterizer_state *rasterizer;
   struct pipe_depth_stencil_alpha_state *depth_stencil_alpha;
   struct ilo_shader_state *fs;
   struct ilo_shader_state *vs;
   struct ilo_shader_state *gs;
   struct ilo_vertex_element *vertex_elements;

   struct pipe_blend_color blend_color;
   struct pipe_stencil_ref stencil_ref;
   unsigned sample_mask;
   struct pipe_clip_state clip;
   struct pipe_framebuffer_state framebuffer;
   struct pipe_poly_stipple poly_stipple;
   struct pipe_scissor_state scissor;
   struct pipe_viewport_state viewport;
   struct pipe_index_buffer index_buffer;

   struct {
      struct pipe_vertex_buffer buffers[PIPE_MAX_ATTRIBS];
      unsigned num_buffers;
   } vertex_buffers;

   struct {
      struct pipe_sampler_state *samplers[ILO_MAX_SAMPLERS];
      unsigned num_samplers;
   } samplers[PIPE_SHADER_TYPES];

   struct {
      struct pipe_sampler_view *views[ILO_MAX_SAMPLER_VIEWS];
      unsigned num_views;
   } sampler_views[PIPE_SHADER_TYPES];

   struct {
      struct pipe_constant_buffer buffers[ILO_MAX_CONST_BUFFERS];
      unsigned num_buffers;
   } constant_buffers[PIPE_SHADER_TYPES];

   struct {
      struct pipe_stream_output_target *targets[ILO_MAX_SO_BUFFERS];
      unsigned num_targets;
      unsigned append_bitmask;
   } stream_output_targets;

   struct {
      struct pipe_surface *surfaces[PIPE_MAX_SHADER_RESOURCES];
      unsigned num_surfaces;
   } shader_resources;

   struct ilo_shader_state *compute;

   struct {
      struct pipe_surface *surfaces[PIPE_MAX_SHADER_RESOURCES];
      unsigned num_surfaces;
   } compute_resources;

   struct {
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
      unsigned num_resources;
   } global_binding;
};

static inline struct ilo_context *
ilo_context(struct pipe_context *pipe)
{
   return (struct ilo_context *) pipe;
}

void
ilo_init_context_functions(struct ilo_screen *is);

#endif /* ILO_CONTEXT_H */

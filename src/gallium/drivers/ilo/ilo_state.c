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

#include "ilo_context.h"
#include "ilo_state.h"

/**
 * Initialize state-related functions.
 */
void
ilo_init_state_functions(struct ilo_context *ilo)
{
   ilo->base.create_blend_state = NULL;
   ilo->base.bind_blend_state = NULL;
   ilo->base.delete_blend_state = NULL;
   ilo->base.create_sampler_state = NULL;
   ilo->base.bind_fragment_sampler_states = NULL;
   ilo->base.bind_vertex_sampler_states = NULL;
   ilo->base.bind_geometry_sampler_states = NULL;
   ilo->base.bind_compute_sampler_states = NULL;
   ilo->base.delete_sampler_state = NULL;
   ilo->base.create_rasterizer_state = NULL;
   ilo->base.bind_rasterizer_state = NULL;
   ilo->base.delete_rasterizer_state = NULL;
   ilo->base.create_depth_stencil_alpha_state = NULL;
   ilo->base.bind_depth_stencil_alpha_state = NULL;
   ilo->base.delete_depth_stencil_alpha_state = NULL;
   ilo->base.create_fs_state = NULL;
   ilo->base.bind_fs_state = NULL;
   ilo->base.delete_fs_state = NULL;
   ilo->base.create_vs_state = NULL;
   ilo->base.bind_vs_state = NULL;
   ilo->base.delete_vs_state = NULL;
   ilo->base.create_gs_state = NULL;
   ilo->base.bind_gs_state = NULL;
   ilo->base.delete_gs_state = NULL;
   ilo->base.create_vertex_elements_state = NULL;
   ilo->base.bind_vertex_elements_state = NULL;
   ilo->base.delete_vertex_elements_state = NULL;

   ilo->base.set_blend_color = NULL;
   ilo->base.set_stencil_ref = NULL;
   ilo->base.set_sample_mask = NULL;
   ilo->base.set_clip_state = NULL;
   ilo->base.set_constant_buffer = NULL;
   ilo->base.set_framebuffer_state = NULL;
   ilo->base.set_polygon_stipple = NULL;
   ilo->base.set_scissor_state = NULL;
   ilo->base.set_viewport_state = NULL;
   ilo->base.set_fragment_sampler_views = NULL;
   ilo->base.set_vertex_sampler_views = NULL;
   ilo->base.set_geometry_sampler_views = NULL;
   ilo->base.set_compute_sampler_views = NULL;
   ilo->base.set_shader_resources = NULL;
   ilo->base.set_vertex_buffers = NULL;
   ilo->base.set_index_buffer = NULL;

   ilo->base.create_stream_output_target = NULL;
   ilo->base.stream_output_target_destroy = NULL;
   ilo->base.set_stream_output_targets = NULL;

   ilo->base.create_sampler_view = NULL;
   ilo->base.sampler_view_destroy = NULL;

   ilo->base.create_surface = NULL;
   ilo->base.surface_destroy = NULL;

   ilo->base.create_compute_state = NULL;
   ilo->base.bind_compute_state = NULL;
   ilo->base.delete_compute_state = NULL;
   ilo->base.set_compute_resources = NULL;
   ilo->base.set_global_binding = NULL;
}

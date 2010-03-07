/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


#include "pipe/p_compiler.h"
#include "util/u_memory.h"
#include "util/u_format.h"
#include "tgsi/tgsi_dump.h"

#include "tr_dump.h"
#include "tr_dump_state.h"


void trace_dump_format(enum pipe_format format)
{
   if (!trace_dumping_enabled_locked())
      return;

   trace_dump_enum(util_format_name(format) );
}


void trace_dump_template(const struct pipe_texture *templat)
{
   if (!trace_dumping_enabled_locked())
      return;

   if(!templat) {
      trace_dump_null();
      return;
   }

   trace_dump_struct_begin("pipe_texture");

   trace_dump_member(int, templat, target);
   trace_dump_member(format, templat, format);

   trace_dump_member_begin("width");
   trace_dump_uint(templat->width0);
   trace_dump_member_end();

   trace_dump_member_begin("height");
   trace_dump_uint(templat->height0);
   trace_dump_member_end();

   trace_dump_member_begin("depth");
   trace_dump_uint(templat->depth0);
   trace_dump_member_end();

   trace_dump_member(uint, templat, last_level);
   trace_dump_member(uint, templat, tex_usage);

   trace_dump_struct_end();
}


void trace_dump_rasterizer_state(const struct pipe_rasterizer_state *state)
{
   if (!trace_dumping_enabled_locked())
      return;

   if(!state) {
      trace_dump_null();
      return;
   }

   trace_dump_struct_begin("pipe_rasterizer_state");

   trace_dump_member(bool, state, flatshade);
   trace_dump_member(bool, state, light_twoside);
   trace_dump_member(uint, state, front_winding);
   trace_dump_member(uint, state, cull_mode);
   trace_dump_member(uint, state, fill_cw);
   trace_dump_member(uint, state, fill_ccw);
   trace_dump_member(bool, state, offset_cw);
   trace_dump_member(bool, state, offset_ccw);
   trace_dump_member(bool, state, scissor);
   trace_dump_member(bool, state, poly_smooth);
   trace_dump_member(bool, state, poly_stipple_enable);
   trace_dump_member(bool, state, point_smooth);
   trace_dump_member(uint, state, sprite_coord_enable);
   trace_dump_member(bool, state, sprite_coord_mode);
   trace_dump_member(bool, state, point_quad_rasterization);
   trace_dump_member(bool, state, point_size_per_vertex);
   trace_dump_member(bool, state, multisample);
   trace_dump_member(bool, state, line_smooth);
   trace_dump_member(bool, state, line_stipple_enable);
   trace_dump_member(uint, state, line_stipple_factor);
   trace_dump_member(uint, state, line_stipple_pattern);
   trace_dump_member(bool, state, line_last_pixel);
   trace_dump_member(bool, state, flatshade_first);
   trace_dump_member(bool, state, gl_rasterization_rules);

   trace_dump_member(float, state, line_width);
   trace_dump_member(float, state, point_size);
   trace_dump_member(float, state, offset_units);
   trace_dump_member(float, state, offset_scale);

   trace_dump_struct_end();
}


void trace_dump_poly_stipple(const struct pipe_poly_stipple *state)
{
   if (!trace_dumping_enabled_locked())
      return;

   if(!state) {
      trace_dump_null();
      return;
   }

   trace_dump_struct_begin("pipe_poly_stipple");

   trace_dump_member_begin("stipple");
   trace_dump_array(uint,
                    state->stipple,
                    Elements(state->stipple));
   trace_dump_member_end();

   trace_dump_struct_end();
}


void trace_dump_viewport_state(const struct pipe_viewport_state *state)
{
   if (!trace_dumping_enabled_locked())
      return;

   if(!state) {
      trace_dump_null();
      return;
   }

   trace_dump_struct_begin("pipe_viewport_state");

   trace_dump_member_array(float, state, scale);
   trace_dump_member_array(float, state, translate);

   trace_dump_struct_end();
}


void trace_dump_scissor_state(const struct pipe_scissor_state *state)
{
   if (!trace_dumping_enabled_locked())
      return;

   if(!state) {
      trace_dump_null();
      return;
   }

   trace_dump_struct_begin("pipe_scissor_state");

   trace_dump_member(uint, state, minx);
   trace_dump_member(uint, state, miny);
   trace_dump_member(uint, state, maxx);
   trace_dump_member(uint, state, maxy);

   trace_dump_struct_end();
}


void trace_dump_clip_state(const struct pipe_clip_state *state)
{
   unsigned i;

   if (!trace_dumping_enabled_locked())
      return;

   if(!state) {
      trace_dump_null();
      return;
   }

   trace_dump_struct_begin("pipe_clip_state");

   trace_dump_member_begin("ucp");
   trace_dump_array_begin();
   for(i = 0; i < PIPE_MAX_CLIP_PLANES; ++i) {
      trace_dump_elem_begin();
      trace_dump_array(float, state->ucp[i], 4);
      trace_dump_elem_end();
   }
   trace_dump_array_end();
   trace_dump_member_end();

   trace_dump_member(uint, state, nr);

   trace_dump_struct_end();
}


void trace_dump_shader_state(const struct pipe_shader_state *state)
{
   static char str[8192];

   if (!trace_dumping_enabled_locked())
      return;

   if(!state) {
      trace_dump_null();
      return;
   }

   tgsi_dump_str(state->tokens, 0, str, sizeof(str));

   trace_dump_struct_begin("pipe_shader_state");

   trace_dump_member_begin("tokens");
   trace_dump_string(str);
   trace_dump_member_end();

   trace_dump_struct_end();
}


void trace_dump_depth_stencil_alpha_state(const struct pipe_depth_stencil_alpha_state *state)
{
   unsigned i;

   if (!trace_dumping_enabled_locked())
      return;

   if(!state) {
      trace_dump_null();
      return;
   }

   trace_dump_struct_begin("pipe_depth_stencil_alpha_state");

   trace_dump_member_begin("depth");
   trace_dump_struct_begin("pipe_depth_state");
   trace_dump_member(bool, &state->depth, enabled);
   trace_dump_member(bool, &state->depth, writemask);
   trace_dump_member(uint, &state->depth, func);
   trace_dump_struct_end();
   trace_dump_member_end();

   trace_dump_member_begin("stencil");
   trace_dump_array_begin();
   for(i = 0; i < Elements(state->stencil); ++i) {
      trace_dump_elem_begin();
      trace_dump_struct_begin("pipe_stencil_state");
      trace_dump_member(bool, &state->stencil[i], enabled);
      trace_dump_member(uint, &state->stencil[i], func);
      trace_dump_member(uint, &state->stencil[i], fail_op);
      trace_dump_member(uint, &state->stencil[i], zpass_op);
      trace_dump_member(uint, &state->stencil[i], zfail_op);
      trace_dump_member(uint, &state->stencil[i], valuemask);
      trace_dump_member(uint, &state->stencil[i], writemask);
      trace_dump_struct_end();
      trace_dump_elem_end();
   }
   trace_dump_array_end();
   trace_dump_member_end();

   trace_dump_member_begin("alpha");
   trace_dump_struct_begin("pipe_alpha_state");
   trace_dump_member(bool, &state->alpha, enabled);
   trace_dump_member(uint, &state->alpha, func);
   trace_dump_member(float, &state->alpha, ref_value);
   trace_dump_struct_end();
   trace_dump_member_end();

   trace_dump_struct_end();
}

void trace_dump_blend_state(const struct pipe_blend_state *state)
{
   if (!trace_dumping_enabled_locked())
      return;

   if(!state) {
      trace_dump_null();
      return;
   }

   trace_dump_bytes(state, sizeof *state);
}


void trace_dump_blend_color(const struct pipe_blend_color *state)
{
   if (!trace_dumping_enabled_locked())
      return;

   if(!state) {
      trace_dump_null();
      return;
   }

   trace_dump_struct_begin("pipe_blend_color");

   trace_dump_member_array(float, state, color);

   trace_dump_struct_end();
}

void trace_dump_stencil_ref(const struct pipe_stencil_ref *state)
{
   if (!trace_dumping_enabled_locked())
      return;

   if(!state) {
      trace_dump_null();
      return;
   }

   trace_dump_struct_begin("pipe_stencil_ref");

   trace_dump_member_array(uint, state, ref_value);

   trace_dump_struct_end();
}

void trace_dump_framebuffer_state(const struct pipe_framebuffer_state *state)
{
   if (!trace_dumping_enabled_locked())
      return;

   trace_dump_struct_begin("pipe_framebuffer_state");

   trace_dump_member(uint, state, width);
   trace_dump_member(uint, state, height);
   trace_dump_member(uint, state, nr_cbufs);
   trace_dump_member_array(ptr, state, cbufs);
   trace_dump_member(ptr, state, zsbuf);

   trace_dump_struct_end();
}


void trace_dump_sampler_state(const struct pipe_sampler_state *state)
{
   if (!trace_dumping_enabled_locked())
      return;

   if(!state) {
      trace_dump_null();
      return;
   }

   trace_dump_struct_begin("pipe_sampler_state");

   trace_dump_member(uint, state, wrap_s);
   trace_dump_member(uint, state, wrap_t);
   trace_dump_member(uint, state, wrap_r);
   trace_dump_member(uint, state, min_img_filter);
   trace_dump_member(uint, state, min_mip_filter);
   trace_dump_member(uint, state, mag_img_filter);
   trace_dump_member(uint, state, compare_mode);
   trace_dump_member(uint, state, compare_func);
   trace_dump_member(bool, state, normalized_coords);
   trace_dump_member(uint, state, max_anisotropy);
   trace_dump_member(float, state, lod_bias);
   trace_dump_member(float, state, min_lod);
   trace_dump_member(float, state, max_lod);
   trace_dump_member_array(float, state, border_color);

   trace_dump_struct_end();
}


void trace_dump_surface(const struct pipe_surface *state)
{
   if (!trace_dumping_enabled_locked())
      return;

   if(!state) {
      trace_dump_null();
      return;
   }

   trace_dump_struct_begin("pipe_surface");

   trace_dump_member(format, state, format);
   trace_dump_member(uint, state, width);
   trace_dump_member(uint, state, height);

   trace_dump_member(uint, state, layout);
   trace_dump_member(uint, state, offset);
   trace_dump_member(uint, state, usage);

   trace_dump_member(ptr, state, texture);
   trace_dump_member(uint, state, face);
   trace_dump_member(uint, state, level);
   trace_dump_member(uint, state, zslice);

   trace_dump_struct_end();
}


void trace_dump_transfer(const struct pipe_transfer *state)
{
   if (!trace_dumping_enabled_locked())
      return;

   if(!state) {
      trace_dump_null();
      return;
   }

   trace_dump_struct_begin("pipe_transfer");

   trace_dump_member(uint, state, width);
   trace_dump_member(uint, state, height);

   trace_dump_member(uint, state, stride);
   trace_dump_member(uint, state, usage);

   trace_dump_member(ptr, state, texture);
   trace_dump_member(uint, state, face);
   trace_dump_member(uint, state, level);
   trace_dump_member(uint, state, zslice);

   trace_dump_struct_end();
}


void trace_dump_vertex_buffer(const struct pipe_vertex_buffer *state)
{
   if (!trace_dumping_enabled_locked())
      return;

   if(!state) {
      trace_dump_null();
      return;
   }

   trace_dump_struct_begin("pipe_vertex_buffer");

   trace_dump_member(uint, state, stride);
   trace_dump_member(uint, state, max_index);
   trace_dump_member(uint, state, buffer_offset);
   trace_dump_member(buffer_ptr, state, buffer);

   trace_dump_struct_end();
}


void trace_dump_vertex_element(const struct pipe_vertex_element *state)
{
   if (!trace_dumping_enabled_locked())
      return;

   if(!state) {
      trace_dump_null();
      return;
   }

   trace_dump_struct_begin("pipe_vertex_element");

   trace_dump_member(uint, state, src_offset);

   trace_dump_member(uint, state, vertex_buffer_index);
   trace_dump_member(uint, state, nr_components);

   trace_dump_member(format, state, src_format);

   trace_dump_struct_end();
}

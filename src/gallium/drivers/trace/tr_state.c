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
#include "tgsi/tgsi_dump.h"

#include "tr_dump.h"
#include "tr_state.h"


void trace_dump_format(struct trace_stream *stream, 
                       enum pipe_format format)
{
   trace_dump_enum(stream, pf_name(format) );
}


void trace_dump_block(struct trace_stream *stream, 
                      const struct pipe_format_block *block)
{
   trace_dump_struct_begin(stream, "pipe_format_block");
   trace_dump_member(stream, uint, block, size);
   trace_dump_member(stream, uint, block, width);
   trace_dump_member(stream, uint, block, height);
   trace_dump_struct_end(stream);
}


void trace_dump_template(struct trace_stream *stream, 
                         const struct pipe_texture *templat)
{
   assert(templat);
   if(!templat) {
      trace_dump_null(stream);
      return;
   }

   trace_dump_struct_begin(stream, "pipe_texture");
   
   trace_dump_member(stream, int, templat, target);
   trace_dump_member(stream, format, templat, format);
   
   trace_dump_member_begin(stream, "width");
   trace_dump_array(stream, uint, templat->width, 1);
   trace_dump_member_end(stream);

   trace_dump_member_begin(stream, "height");
   trace_dump_array(stream, uint, templat->height, 1);
   trace_dump_member_end(stream);

   trace_dump_member_begin(stream, "depth");
   trace_dump_array(stream, uint, templat->depth, 1);
   trace_dump_member_end(stream);

   trace_dump_member_begin(stream, "block");
   trace_dump_block(stream, &templat->block);
   trace_dump_member_end(stream);
   
   trace_dump_member(stream, uint, templat, last_level);
   trace_dump_member(stream, uint, templat, tex_usage);
   
   trace_dump_struct_end(stream);
}


void trace_dump_rasterizer_state(struct trace_stream *stream, 
                                 const struct pipe_rasterizer_state *state)
{
   assert(state);
   if(!state) {
      trace_dump_null(stream);
      return;
   }

   trace_dump_struct_begin(stream, "pipe_rasterizer_state");

   trace_dump_member(stream, bool, state, flatshade);
   trace_dump_member(stream, bool, state, light_twoside);
   trace_dump_member(stream, uint, state, front_winding);
   trace_dump_member(stream, uint, state, cull_mode);
   trace_dump_member(stream, uint, state, fill_cw);
   trace_dump_member(stream, uint, state, fill_ccw);
   trace_dump_member(stream, bool, state, offset_cw);
   trace_dump_member(stream, bool, state, offset_ccw);
   trace_dump_member(stream, bool, state, scissor);
   trace_dump_member(stream, bool, state, poly_smooth);
   trace_dump_member(stream, bool, state, poly_stipple_enable);
   trace_dump_member(stream, bool, state, point_smooth);
   trace_dump_member(stream, bool, state, point_sprite);
   trace_dump_member(stream, bool, state, point_size_per_vertex);
   trace_dump_member(stream, bool, state, multisample);
   trace_dump_member(stream, bool, state, line_smooth);
   trace_dump_member(stream, bool, state, line_stipple_enable);
   trace_dump_member(stream, uint, state, line_stipple_factor);
   trace_dump_member(stream, uint, state, line_stipple_pattern);
   trace_dump_member(stream, bool, state, line_last_pixel);
   trace_dump_member(stream, bool, state, bypass_clipping);
   trace_dump_member(stream, bool, state, bypass_vs);
   trace_dump_member(stream, bool, state, origin_lower_left);
   trace_dump_member(stream, bool, state, flatshade_first);
   trace_dump_member(stream, bool, state, gl_rasterization_rules);

   trace_dump_member(stream, float, state, line_width);
   trace_dump_member(stream, float, state, point_size);
   trace_dump_member(stream, float, state, point_size_min);
   trace_dump_member(stream, float, state, point_size_max);
   trace_dump_member(stream, float, state, offset_units);
   trace_dump_member(stream, float, state, offset_scale);
   
   trace_dump_member_array(stream, uint, state, sprite_coord_mode);
   
   trace_dump_struct_end(stream);
}


void trace_dump_poly_stipple(struct trace_stream *stream,
                             const struct pipe_poly_stipple *state)
{
   assert(state);
   if(!state) {
      trace_dump_null(stream);
      return;
   }

   trace_dump_struct_begin(stream, "pipe_poly_stipple");

   trace_dump_member_begin(stream, "stipple");
   trace_dump_bytes(stream, 
                    state->stipple, 
                    sizeof(state->stipple));
   trace_dump_member_end(stream);
   
   trace_dump_struct_end(stream);
}


void trace_dump_viewport_state(struct trace_stream *stream,
                               const struct pipe_viewport_state *state)
{
   assert(state);
   if(!state) {
      trace_dump_null(stream);
      return;
   }

   trace_dump_struct_begin(stream, "pipe_viewport_state");

   trace_dump_member_array(stream, float, state, scale);
   trace_dump_member_array(stream, float, state, translate);
   
   trace_dump_struct_end(stream);
}


void trace_dump_scissor_state(struct trace_stream *stream,
                              const struct pipe_scissor_state *state)
{
   assert(state);
   if(!state) {
      trace_dump_null(stream);
      return;
   }

   trace_dump_struct_begin(stream, "pipe_scissor_state");

   trace_dump_member(stream, uint, state, minx);
   trace_dump_member(stream, uint, state, miny);
   trace_dump_member(stream, uint, state, maxx);
   trace_dump_member(stream, uint, state, maxy);

   trace_dump_struct_end(stream);
}


void trace_dump_clip_state(struct trace_stream *stream,
                           const struct pipe_clip_state *state)
{
   unsigned i;
   
   assert(state);
   if(!state) {
      trace_dump_null(stream);
      return;
   }

   trace_dump_struct_begin(stream, "pipe_scissor_state");

   trace_dump_member_begin(stream, "ucp");
   trace_dump_array_begin(stream);
   for(i = 0; i < PIPE_MAX_CLIP_PLANES; ++i)
      trace_dump_array(stream, float, state->ucp[i], 4);
   trace_dump_array_end(stream);
   trace_dump_member_end(stream);

   trace_dump_member(stream, uint, state, nr);

   trace_dump_struct_end(stream);
}


void trace_dump_constant_buffer(struct trace_stream *stream,
                                const struct pipe_constant_buffer *state)
{
   assert(state);
   if(!state) {
      trace_dump_null(stream);
      return;
   }

   trace_dump_struct_begin(stream, "pipe_constant_buffer");

   trace_dump_member(stream, ptr, state, buffer);
   trace_dump_member(stream, uint, state, size);

   trace_dump_struct_end(stream);
}


void trace_dump_shader_state(struct trace_stream *stream,
                             const struct pipe_shader_state *state)
{
   static char str[8192];
   assert(state);
   if(!state) {
      trace_dump_null(stream);
      return;
   }

   tgsi_dump_str(state->tokens, 0, str, sizeof(str));
   
   trace_dump_struct_begin(stream, "pipe_shader_state");

   trace_dump_member_begin(stream, "tokens");
   trace_dump_string(stream, str);
   trace_dump_member_end(stream);

   trace_dump_struct_end(stream);
}


void trace_dump_depth_stencil_alpha_state(struct trace_stream *stream,
                                          const struct pipe_depth_stencil_alpha_state *state)
{
   unsigned i;
   
   assert(state);
   if(!state) {
      trace_dump_null(stream);
      return;
   }

   trace_dump_struct_begin(stream, "pipe_depth_stencil_alpha_state");

   trace_dump_member_begin(stream, "depth");
   trace_dump_struct_begin(stream, "pipe_depth_state");
   trace_dump_member(stream, bool, &state->depth, enabled);
   trace_dump_member(stream, bool, &state->depth, writemask);
   trace_dump_member(stream, uint, &state->depth, func);
   trace_dump_member(stream, bool, &state->depth, occlusion_count);
   trace_dump_struct_end(stream);
   trace_dump_member_end(stream);
   
   trace_dump_member_begin(stream, "stencil");
   trace_dump_array_begin(stream);
   for(i = 0; i < Elements(state->stencil); ++i) {
      trace_dump_elem_begin(stream);
      trace_dump_struct_begin(stream, "pipe_stencil_state");
      trace_dump_member(stream, bool, &state->stencil[i], enabled);
      trace_dump_member(stream, uint, &state->stencil[i], func);
      trace_dump_member(stream, uint, &state->stencil[i], fail_op);
      trace_dump_member(stream, uint, &state->stencil[i], zpass_op);
      trace_dump_member(stream, uint, &state->stencil[i], zfail_op);
      trace_dump_member(stream, uint, &state->stencil[i], ref_value);    
      trace_dump_member(stream, uint, &state->stencil[i], value_mask);
      trace_dump_member(stream, uint, &state->stencil[i], write_mask);
      trace_dump_struct_end(stream);
      trace_dump_elem_end(stream);
   }
   trace_dump_array_end(stream);
   trace_dump_member_end(stream);

   trace_dump_member_begin(stream, "alpha");
   trace_dump_struct_begin(stream, "pipe_alpha_state");
   trace_dump_member(stream, bool, &state->alpha, enabled);
   trace_dump_member(stream, uint, &state->alpha, func);
   trace_dump_member(stream, float, &state->alpha, ref);
   trace_dump_struct_end(stream);
   trace_dump_member_end(stream);

   trace_dump_struct_end(stream);
}


void trace_dump_blend_state(struct trace_stream *stream,
                            const struct pipe_blend_state *state)
{
   assert(state);
   if(!state) {
      trace_dump_null(stream);
      return;
   }

   trace_dump_struct_begin(stream, "pipe_blend_state");

   trace_dump_member(stream, bool, state, blend_enable);

   trace_dump_member(stream, uint, state, rgb_func);
   trace_dump_member(stream, uint, state, rgb_src_factor);
   trace_dump_member(stream, uint, state, rgb_dst_factor);

   trace_dump_member(stream, uint, state, alpha_func);
   trace_dump_member(stream, uint, state, alpha_src_factor);
   trace_dump_member(stream, uint, state, alpha_dst_factor);

   trace_dump_member(stream, bool, state, logicop_enable);
   trace_dump_member(stream, uint, state, logicop_func);

   trace_dump_member(stream, uint, state, colormask);
   trace_dump_member(stream, bool, state, dither);

   trace_dump_struct_end(stream);
}


void trace_dump_blend_color(struct trace_stream *stream,
                            const struct pipe_blend_color *state)
{
   assert(state);
   if(!state) {
      trace_dump_null(stream);
      return;
   }

   trace_dump_struct_begin(stream, "pipe_blend_color");

   trace_dump_member_array(stream, float, state, color);

   trace_dump_struct_end(stream);
}


void trace_dump_framebuffer_state(struct trace_stream *stream,
                                  const struct pipe_framebuffer_state *state)
{
   trace_dump_struct_begin(stream, "pipe_framebuffer_state");

   trace_dump_member(stream, uint, state, width);
   trace_dump_member(stream, uint, state, height);
   trace_dump_member(stream, uint, state, num_cbufs);
   trace_dump_member_array(stream, ptr, state, cbufs);
   trace_dump_member(stream, ptr, state, zsbuf);

   trace_dump_struct_end(stream);
}


void trace_dump_sampler_state(struct trace_stream *stream,
                              const struct pipe_sampler_state *state)
{
   assert(state);
   if(!state) {
      trace_dump_null(stream);
      return;
   }

   trace_dump_struct_begin(stream, "pipe_sampler_state");

   trace_dump_member(stream, uint, state, wrap_s);
   trace_dump_member(stream, uint, state, wrap_t);
   trace_dump_member(stream, uint, state, wrap_r);
   trace_dump_member(stream, uint, state, min_img_filter);
   trace_dump_member(stream, uint, state, min_mip_filter);
   trace_dump_member(stream, uint, state, mag_img_filter);
   trace_dump_member(stream, bool, state, compare_mode);
   trace_dump_member(stream, uint, state, compare_func);
   trace_dump_member(stream, bool, state, normalized_coords);
   trace_dump_member(stream, uint, state, prefilter);
   trace_dump_member(stream, float, state, shadow_ambient);
   trace_dump_member(stream, float, state, lod_bias);
   trace_dump_member(stream, float, state, min_lod);
   trace_dump_member(stream, float, state, max_lod);
   trace_dump_member_array(stream, float, state, border_color);
   trace_dump_member(stream, float, state, max_anisotropy);

   trace_dump_struct_end(stream);
}


void trace_dump_surface(struct trace_stream *stream,
                        const struct pipe_surface *state)
{
   assert(state);
   if(!state) {
      trace_dump_null(stream);
      return;
   }

   trace_dump_struct_begin(stream, "pipe_surface");

   trace_dump_member(stream, ptr, state, buffer);
   trace_dump_member(stream, format, state, format);
   trace_dump_member(stream, uint, state, status);
   trace_dump_member(stream, uint, state, clear_value);
   trace_dump_member(stream, uint, state, width);
   trace_dump_member(stream, uint, state, height);

   trace_dump_member_begin(stream, "block");
   trace_dump_block(stream, &state->block);
   trace_dump_member_end(stream);
   
   trace_dump_member(stream, uint, state, nblocksx);
   trace_dump_member(stream, uint, state, nblocksy);
   trace_dump_member(stream, uint, state, stride);
   trace_dump_member(stream, uint, state, layout);
   trace_dump_member(stream, uint, state, offset);
   trace_dump_member(stream, uint, state, refcount);
   trace_dump_member(stream, uint, state, usage);

   trace_dump_member(stream, ptr, state, texture);
   trace_dump_member(stream, uint, state, face);
   trace_dump_member(stream, uint, state, level);
   trace_dump_member(stream, uint, state, zslice);

   trace_dump_struct_end(stream);
}


void trace_dump_vertex_buffer(struct trace_stream *stream,
                              const struct pipe_vertex_buffer *state)
{
   assert(state);
   if(!state) {
      trace_dump_null(stream);
      return;
   }

   trace_dump_struct_begin(stream, "pipe_vertex_buffer");

   trace_dump_member(stream, uint, state, pitch);
   trace_dump_member(stream, uint, state, max_index);
   trace_dump_member(stream, uint, state, buffer_offset);
   trace_dump_member(stream, ptr, state, buffer);

   trace_dump_struct_end(stream);
}


void trace_dump_vertex_element(struct trace_stream *stream,
                               const struct pipe_vertex_element *state)
{
   assert(state);
   if(!state) {
      trace_dump_null(stream);
      return;
   }

   trace_dump_struct_begin(stream, "pipe_vertex_element");

   trace_dump_member(stream, uint, state, src_offset);

   trace_dump_member(stream, uint, state, vertex_buffer_index);
   trace_dump_member(stream, uint, state, nr_components);
 
   trace_dump_member(stream, format, state, src_format);

   trace_dump_struct_end(stream);
}

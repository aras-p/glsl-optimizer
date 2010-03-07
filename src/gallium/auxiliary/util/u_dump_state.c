/**************************************************************************
 *
 * Copyright 2008-2010 VMware, Inc.
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


#include "pipe/p_compiler.h"
#include "os/os_stream.h"
#include "util/u_memory.h"
#include "util/u_string.h"
#include "util/u_format.h"
#include "tgsi/tgsi_dump.h"

#include "u_dump.h"


/*
 * Dump primitives
 */

static INLINE void
util_stream_writef(struct os_stream *stream, const char *format, ...)
{
   static char buf[1024];
   unsigned len;
   va_list ap;
   va_start(ap, format);
   len = util_vsnprintf(buf, sizeof(buf), format, ap);
   va_end(ap);
   os_stream_write(stream, buf, len);
}

static void
util_dump_bool(struct os_stream *stream, int value)
{
   util_stream_writef(stream, "%c", value ? '1' : '0');
}

static void
util_dump_int(struct os_stream *stream, long long int value)
{
   util_stream_writef(stream, "%lli", value);
}

static void
util_dump_uint(struct os_stream *stream, long long unsigned value)
{
   util_stream_writef(stream, "%llu", value);
}

static void
util_dump_float(struct os_stream *stream, double value)
{
   util_stream_writef(stream, "%g", value);
}

static void
util_dump_string(struct os_stream *stream, const char *str)
{
   os_stream_write_str(stream, "\"");
   os_stream_write_str(stream, str);
   os_stream_write_str(stream, "\"");
}

static void
util_dump_enum(struct os_stream *stream, const char *value)
{
   os_stream_write_str(stream, value);
}

static void
util_dump_array_begin(struct os_stream *stream)
{
   os_stream_write_str(stream, "{");
}

static void
util_dump_array_end(struct os_stream *stream)
{
   os_stream_write_str(stream, "}");
}

static void
util_dump_elem_begin(struct os_stream *stream)
{
}

static void
util_dump_elem_end(struct os_stream *stream)
{
   os_stream_write_str(stream, ", ");
}

static void
util_dump_struct_begin(struct os_stream *stream, const char *name)
{
   os_stream_write_str(stream, "{");
}

static void
util_dump_struct_end(struct os_stream *stream)
{
   os_stream_write_str(stream, "}");
}

static void
util_dump_member_begin(struct os_stream *stream, const char *name)
{
   util_stream_writef(stream, "%s = ", name);
}

static void
util_dump_member_end(struct os_stream *stream)
{
   os_stream_write_str(stream, ", ");
}

static void
util_dump_null(struct os_stream *stream)
{
   os_stream_write_str(stream, "NULL");
}

static void
util_dump_ptr(struct os_stream *stream, const void *value)
{
   if(value)
      util_stream_writef(stream, "0x%08lx", (unsigned long)(uintptr_t)value);
   else
      util_dump_null(stream);
}


/*
 * Code saving macros.
 */

#define util_dump_arg(_stream, _type, _arg) \
   do { \
      util_dump_arg_begin(_stream, #_arg); \
      util_dump_##_type(_stream, _arg); \
      util_dump_arg_end(_stream); \
   } while(0)

#define util_dump_ret(_stream, _type, _arg) \
   do { \
      util_dump_ret_begin(_stream); \
      util_dump_##_type(_stream, _arg); \
      util_dump_ret_end(_stream); \
   } while(0)

#define util_dump_array(_stream, _type, _obj, _size) \
   do { \
      size_t idx; \
      util_dump_array_begin(_stream); \
      for(idx = 0; idx < (_size); ++idx) { \
         util_dump_elem_begin(_stream); \
         util_dump_##_type(_stream, (_obj)[idx]); \
         util_dump_elem_end(_stream); \
      } \
      util_dump_array_end(_stream); \
   } while(0)

#define util_dump_struct_array(_stream, _type, _obj, _size) \
   do { \
      size_t idx; \
      util_dump_array_begin(_stream); \
      for(idx = 0; idx < (_size); ++idx) { \
         util_dump_elem_begin(_stream); \
         util_dump_##_type(_stream, &(_obj)[idx]); \
         util_dump_elem_end(_stream); \
      } \
      util_dump_array_end(_stream); \
   } while(0)

#define util_dump_member(_stream, _type, _obj, _member) \
   do { \
      util_dump_member_begin(_stream, #_member); \
      util_dump_##_type(_stream, (_obj)->_member); \
      util_dump_member_end(_stream); \
   } while(0)

#define util_dump_arg_array(_stream, _type, _arg, _size) \
   do { \
      util_dump_arg_begin(_stream, #_arg); \
      util_dump_array(_stream, _type, _arg, _size); \
      util_dump_arg_end(_stream); \
   } while(0)

#define util_dump_member_array(_stream, _type, _obj, _member) \
   do { \
      util_dump_member_begin(_stream, #_member); \
      util_dump_array(_stream, _type, (_obj)->_member, sizeof((_obj)->_member)/sizeof((_obj)->_member[0])); \
      util_dump_member_end(_stream); \
   } while(0)



/*
 * Wrappers for enum -> string dumpers.
 */


static void
util_dump_format(struct os_stream *stream, enum pipe_format format)
{
   util_dump_enum(stream, util_format_name(format));
}


static void
util_dump_enum_blend_factor(struct os_stream *stream, unsigned value)
{
   util_dump_enum(stream, util_dump_blend_factor(value, TRUE));
}

static void
util_dump_enum_blend_func(struct os_stream *stream, unsigned value)
{
   util_dump_enum(stream, util_dump_blend_func(value, TRUE));
}

static void
util_dump_enum_func(struct os_stream *stream, unsigned value)
{
   util_dump_enum(stream, util_dump_func(value, TRUE));
}


/*
 * Public functions
 */


void
util_dump_template(struct os_stream *stream, const struct pipe_texture *templat)
{
   if(!templat) {
      util_dump_null(stream);
      return;
   }

   util_dump_struct_begin(stream, "pipe_texture");

   util_dump_member(stream, int, templat, target);
   util_dump_member(stream, format, templat, format);

   util_dump_member_begin(stream, "width");
   util_dump_uint(stream, templat->width0);
   util_dump_member_end(stream);

   util_dump_member_begin(stream, "height");
   util_dump_uint(stream, templat->height0);
   util_dump_member_end(stream);

   util_dump_member_begin(stream, "depth");
   util_dump_uint(stream, templat->depth0);
   util_dump_member_end(stream);

   util_dump_member(stream, uint, templat, last_level);
   util_dump_member(stream, uint, templat, tex_usage);

   util_dump_struct_end(stream);
}


void
util_dump_rasterizer_state(struct os_stream *stream, const struct pipe_rasterizer_state *state)
{
   if(!state) {
      util_dump_null(stream);
      return;
   }

   util_dump_struct_begin(stream, "pipe_rasterizer_state");

   util_dump_member(stream, bool, state, flatshade);
   util_dump_member(stream, bool, state, light_twoside);
   util_dump_member(stream, uint, state, front_winding);
   util_dump_member(stream, uint, state, cull_mode);
   util_dump_member(stream, uint, state, fill_cw);
   util_dump_member(stream, uint, state, fill_ccw);
   util_dump_member(stream, bool, state, offset_cw);
   util_dump_member(stream, bool, state, offset_ccw);
   util_dump_member(stream, bool, state, scissor);
   util_dump_member(stream, bool, state, poly_smooth);
   util_dump_member(stream, bool, state, poly_stipple_enable);
   util_dump_member(stream, bool, state, point_smooth);
   util_dump_member(stream, uint, state, sprite_coord_enable);
   util_dump_member(stream, bool, state, sprite_coord_mode);
   util_dump_member(stream, bool, state, point_quad_rasterization);
   util_dump_member(stream, bool, state, point_size_per_vertex);
   util_dump_member(stream, bool, state, multisample);
   util_dump_member(stream, bool, state, line_smooth);
   util_dump_member(stream, bool, state, line_stipple_enable);
   util_dump_member(stream, uint, state, line_stipple_factor);
   util_dump_member(stream, uint, state, line_stipple_pattern);
   util_dump_member(stream, bool, state, line_last_pixel);
   util_dump_member(stream, bool, state, flatshade_first);
   util_dump_member(stream, bool, state, gl_rasterization_rules);

   util_dump_member(stream, float, state, line_width);
   util_dump_member(stream, float, state, point_size);
   util_dump_member(stream, float, state, offset_units);
   util_dump_member(stream, float, state, offset_scale);

   util_dump_struct_end(stream);
}


void
util_dump_poly_stipple(struct os_stream *stream, const struct pipe_poly_stipple *state)
{
   if(!state) {
      util_dump_null(stream);
      return;
   }

   util_dump_struct_begin(stream, "pipe_poly_stipple");

   util_dump_member_begin(stream, "stipple");
   util_dump_member_array(stream, uint, state, stipple);
   util_dump_member_end(stream);

   util_dump_struct_end(stream);
}


void
util_dump_viewport_state(struct os_stream *stream, const struct pipe_viewport_state *state)
{
   if(!state) {
      util_dump_null(stream);
      return;
   }

   util_dump_struct_begin(stream, "pipe_viewport_state");

   util_dump_member_array(stream, float, state, scale);
   util_dump_member_array(stream, float, state, translate);

   util_dump_struct_end(stream);
}


void
util_dump_scissor_state(struct os_stream *stream, const struct pipe_scissor_state *state)
{
   if(!state) {
      util_dump_null(stream);
      return;
   }

   util_dump_struct_begin(stream, "pipe_scissor_state");

   util_dump_member(stream, uint, state, minx);
   util_dump_member(stream, uint, state, miny);
   util_dump_member(stream, uint, state, maxx);
   util_dump_member(stream, uint, state, maxy);

   util_dump_struct_end(stream);
}


void
util_dump_clip_state(struct os_stream *stream, const struct pipe_clip_state *state)
{
   unsigned i;

   if(!state) {
      util_dump_null(stream);
      return;
   }

   util_dump_struct_begin(stream, "pipe_clip_state");

   util_dump_member_begin(stream, "ucp");
   util_dump_array_begin(stream);
   for(i = 0; i < PIPE_MAX_CLIP_PLANES; ++i) {
      util_dump_elem_begin(stream);
      util_dump_array(stream, float, state->ucp[i], 4);
      util_dump_elem_end(stream);
   }
   util_dump_array_end(stream);
   util_dump_member_end(stream);

   util_dump_member(stream, uint, state, nr);

   util_dump_struct_end(stream);
}


void
util_dump_shader_state(struct os_stream *stream, const struct pipe_shader_state *state)
{
   char str[8192];

   if(!state) {
      util_dump_null(stream);
      return;
   }

   tgsi_dump_str(state->tokens, 0, str, sizeof(str));

   util_dump_struct_begin(stream, "pipe_shader_state");

   util_dump_member_begin(stream, "tokens");
   util_dump_string(stream, str);
   util_dump_member_end(stream);

   util_dump_struct_end(stream);
}


void
util_dump_depth_stencil_alpha_state(struct os_stream *stream, const struct pipe_depth_stencil_alpha_state *state)
{
   unsigned i;

   if(!state) {
      util_dump_null(stream);
      return;
   }

   util_dump_struct_begin(stream, "pipe_depth_stencil_alpha_state");

   util_dump_member_begin(stream, "depth");
   util_dump_struct_begin(stream, "pipe_depth_state");
   util_dump_member(stream, bool, &state->depth, enabled);
   if (state->depth.enabled) {
      util_dump_member(stream, bool, &state->depth, writemask);
      util_dump_member(stream, enum_func, &state->depth, func);
   }
   util_dump_struct_end(stream);
   util_dump_member_end(stream);

   util_dump_member_begin(stream, "stencil");
   util_dump_array_begin(stream);
   for(i = 0; i < Elements(state->stencil); ++i) {
      util_dump_elem_begin(stream);
      util_dump_struct_begin(stream, "pipe_stencil_state");
      util_dump_member(stream, bool, &state->stencil[i], enabled);
      if (state->stencil[i].enabled) {
         util_dump_member(stream, enum_func, &state->stencil[i], func);
         util_dump_member(stream, uint, &state->stencil[i], fail_op);
         util_dump_member(stream, uint, &state->stencil[i], zpass_op);
         util_dump_member(stream, uint, &state->stencil[i], zfail_op);
         util_dump_member(stream, uint, &state->stencil[i], valuemask);
         util_dump_member(stream, uint, &state->stencil[i], writemask);
      }
      util_dump_struct_end(stream);
      util_dump_elem_end(stream);
   }
   util_dump_array_end(stream);
   util_dump_member_end(stream);

   util_dump_member_begin(stream, "alpha");
   util_dump_struct_begin(stream, "pipe_alpha_state");
   util_dump_member(stream, bool, &state->alpha, enabled);
   if (state->alpha.enabled) {
      util_dump_member(stream, enum_func, &state->alpha, func);
      util_dump_member(stream, float, &state->alpha, ref_value);
   }
   util_dump_struct_end(stream);
   util_dump_member_end(stream);

   util_dump_struct_end(stream);
}

void
util_dump_rt_blend_state(struct os_stream *stream, const struct pipe_rt_blend_state *state)
{
   util_dump_struct_begin(stream, "pipe_rt_blend_state");

   util_dump_member(stream, uint, state, blend_enable);
   if (state->blend_enable) {
      util_dump_member(stream, enum_blend_func, state, rgb_func);
      util_dump_member(stream, enum_blend_factor, state, rgb_src_factor);
      util_dump_member(stream, enum_blend_factor, state, rgb_dst_factor);

      util_dump_member(stream, enum_blend_func, state, alpha_func);
      util_dump_member(stream, enum_blend_factor, state, alpha_src_factor);
      util_dump_member(stream, enum_blend_factor, state, alpha_dst_factor);
   }

   util_dump_member(stream, uint, state, colormask);

   util_dump_struct_end(stream);
}

void
util_dump_blend_state(struct os_stream *stream, const struct pipe_blend_state *state)
{
   unsigned valid_entries = 1;

   if(!state) {
      util_dump_null(stream);
      return;
   }

   util_dump_struct_begin(stream, "pipe_blend_state");

   util_dump_member(stream, bool, state, dither);

   util_dump_member(stream, bool, state, logicop_enable);
   if (state->logicop_enable) {
      util_dump_member(stream, enum_func, state, logicop_func);
   }
   else {
      util_dump_member(stream, bool, state, independent_blend_enable);

      util_dump_member_begin(stream, "rt");
      if (state->independent_blend_enable)
         valid_entries = PIPE_MAX_COLOR_BUFS;
      util_dump_struct_array(stream, rt_blend_state, state->rt, valid_entries);
      util_dump_member_end(stream);
   }

   util_dump_struct_end(stream);
}


void
util_dump_blend_color(struct os_stream *stream, const struct pipe_blend_color *state)
{
   if(!state) {
      util_dump_null(stream);
      return;
   }

   util_dump_struct_begin(stream, "pipe_blend_color");

   util_dump_member_array(stream, float, state, color);

   util_dump_struct_end(stream);
}

void
util_dump_stencil_ref(struct os_stream *stream, const struct pipe_stencil_ref *state)
{
   if(!state) {
      util_dump_null(stream);
      return;
   }

   util_dump_struct_begin(stream, "pipe_stencil_ref");

   util_dump_member_array(stream, uint, state, ref_value);

   util_dump_struct_end(stream);
}

void
util_dump_framebuffer_state(struct os_stream *stream, const struct pipe_framebuffer_state *state)
{
   util_dump_struct_begin(stream, "pipe_framebuffer_state");

   util_dump_member(stream, uint, state, width);
   util_dump_member(stream, uint, state, height);
   util_dump_member(stream, uint, state, nr_cbufs);
   util_dump_member_array(stream, ptr, state, cbufs);
   util_dump_member(stream, ptr, state, zsbuf);

   util_dump_struct_end(stream);
}


void
util_dump_sampler_state(struct os_stream *stream, const struct pipe_sampler_state *state)
{
   if(!state) {
      util_dump_null(stream);
      return;
   }

   util_dump_struct_begin(stream, "pipe_sampler_state");

   util_dump_member(stream, uint, state, wrap_s);
   util_dump_member(stream, uint, state, wrap_t);
   util_dump_member(stream, uint, state, wrap_r);
   util_dump_member(stream, uint, state, min_img_filter);
   util_dump_member(stream, uint, state, min_mip_filter);
   util_dump_member(stream, uint, state, mag_img_filter);
   util_dump_member(stream, uint, state, compare_mode);
   util_dump_member(stream, enum_func, state, compare_func);
   util_dump_member(stream, bool, state, normalized_coords);
   util_dump_member(stream, uint, state, max_anisotropy);
   util_dump_member(stream, float, state, lod_bias);
   util_dump_member(stream, float, state, min_lod);
   util_dump_member(stream, float, state, max_lod);
   util_dump_member_array(stream, float, state, border_color);

   util_dump_struct_end(stream);
}


void
util_dump_surface(struct os_stream *stream, const struct pipe_surface *state)
{
   if(!state) {
      util_dump_null(stream);
      return;
   }

   util_dump_struct_begin(stream, "pipe_surface");

   util_dump_member(stream, format, state, format);
   util_dump_member(stream, uint, state, width);
   util_dump_member(stream, uint, state, height);

   util_dump_member(stream, uint, state, layout);
   util_dump_member(stream, uint, state, offset);
   util_dump_member(stream, uint, state, usage);

   util_dump_member(stream, ptr, state, texture);
   util_dump_member(stream, uint, state, face);
   util_dump_member(stream, uint, state, level);
   util_dump_member(stream, uint, state, zslice);

   util_dump_struct_end(stream);
}


void
util_dump_transfer(struct os_stream *stream, const struct pipe_transfer *state)
{
   if(!state) {
      util_dump_null(stream);
      return;
   }

   util_dump_struct_begin(stream, "pipe_transfer");

   util_dump_member(stream, uint, state, width);
   util_dump_member(stream, uint, state, height);

   util_dump_member(stream, uint, state, stride);
   util_dump_member(stream, uint, state, usage);

   util_dump_member(stream, ptr, state, texture);
   util_dump_member(stream, uint, state, face);
   util_dump_member(stream, uint, state, level);
   util_dump_member(stream, uint, state, zslice);

   util_dump_struct_end(stream);
}


void
util_dump_vertex_buffer(struct os_stream *stream, const struct pipe_vertex_buffer *state)
{
   if(!state) {
      util_dump_null(stream);
      return;
   }

   util_dump_struct_begin(stream, "pipe_vertex_buffer");

   util_dump_member(stream, uint, state, stride);
   util_dump_member(stream, uint, state, max_index);
   util_dump_member(stream, uint, state, buffer_offset);
   util_dump_member(stream, ptr, state, buffer);

   util_dump_struct_end(stream);
}


void
util_dump_vertex_element(struct os_stream *stream, const struct pipe_vertex_element *state)
{
   if(!state) {
      util_dump_null(stream);
      return;
   }

   util_dump_struct_begin(stream, "pipe_vertex_element");

   util_dump_member(stream, uint, state, src_offset);

   util_dump_member(stream, uint, state, vertex_buffer_index);
   util_dump_member(stream, uint, state, nr_components);

   util_dump_member(stream, format, state, src_format);

   util_dump_struct_end(stream);
}

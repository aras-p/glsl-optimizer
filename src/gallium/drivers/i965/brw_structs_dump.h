/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 **************************************************************************/

/**
 * @file
 * Dump i965 data structures.
 *
 * Generated automatically from brw_structs.h by brw_structs_dump.py.
 */

#ifndef BRW_STRUCTS_DUMP_H
#define BRW_STRUCTS_DUMP_H

struct brw_3d_control;
struct brw_3d_primitive;
struct brw_aa_line_parameters;
struct brw_binding_table_pointers;
struct brw_blend_constant_color;
struct brw_cc0;
struct brw_cc1;
struct brw_cc2;
struct brw_cc3;
struct brw_cc4;
struct brw_cc5;
struct brw_cc6;
struct brw_cc7;
struct brw_cc_unit_state;
struct brw_cc_viewport;
struct brw_clip_unit_state;
struct brw_clipper_viewport;
struct brw_constant_buffer;
struct brw_cs_urb_state;
struct brw_depthbuffer;
struct brw_depthbuffer_g4x;
struct brw_drawrect;
struct brw_global_depth_offset_clamp;
struct brw_gs_unit_state;
struct brw_indexbuffer;
struct brw_line_stipple;
struct brw_mi_flush;
struct brw_pipe_control;
struct brw_pipeline_select;
struct brw_pipelined_state_pointers;
struct brw_polygon_stipple;
struct brw_polygon_stipple_offset;
struct brw_sampler_default_color;
struct brw_sampler_state;
struct brw_sf_unit_state;
struct brw_sf_viewport;
struct brw_ss0;
struct brw_ss1;
struct brw_ss2;
struct brw_ss3;
struct brw_state_base_address;
struct brw_state_prefetch;
struct brw_surf_ss0;
struct brw_surf_ss1;
struct brw_surf_ss2;
struct brw_surf_ss3;
struct brw_surf_ss4;
struct brw_surf_ss5;
struct brw_surface_state;
struct brw_system_instruction_pointer;
struct brw_urb_fence;
struct brw_urb_immediate;
struct brw_vb_array_state;
struct brw_vertex_buffer_state;
struct brw_vertex_element_packet;
struct brw_vertex_element_state;
struct brw_vf_statistics;
struct brw_vs_unit_state;
struct brw_wm_unit_state;

void
brw_dump_3d_control(const struct brw_3d_control *ptr);

void
brw_dump_3d_primitive(const struct brw_3d_primitive *ptr);

void
brw_dump_aa_line_parameters(const struct brw_aa_line_parameters *ptr);

void
brw_dump_binding_table_pointers(const struct brw_binding_table_pointers *ptr);

void
brw_dump_blend_constant_color(const struct brw_blend_constant_color *ptr);

void
brw_dump_cc0(const struct brw_cc0 *ptr);

void
brw_dump_cc1(const struct brw_cc1 *ptr);

void
brw_dump_cc2(const struct brw_cc2 *ptr);

void
brw_dump_cc3(const struct brw_cc3 *ptr);

void
brw_dump_cc4(const struct brw_cc4 *ptr);

void
brw_dump_cc5(const struct brw_cc5 *ptr);

void
brw_dump_cc6(const struct brw_cc6 *ptr);

void
brw_dump_cc7(const struct brw_cc7 *ptr);

void
brw_dump_cc_unit_state(const struct brw_cc_unit_state *ptr);

void
brw_dump_cc_viewport(const struct brw_cc_viewport *ptr);

void
brw_dump_clip_unit_state(const struct brw_clip_unit_state *ptr);

void
brw_dump_clipper_viewport(const struct brw_clipper_viewport *ptr);

void
brw_dump_constant_buffer(const struct brw_constant_buffer *ptr);

void
brw_dump_cs_urb_state(const struct brw_cs_urb_state *ptr);

void
brw_dump_depthbuffer(const struct brw_depthbuffer *ptr);

void
brw_dump_depthbuffer_g4x(const struct brw_depthbuffer_g4x *ptr);

void
brw_dump_drawrect(const struct brw_drawrect *ptr);

void
brw_dump_global_depth_offset_clamp(const struct brw_global_depth_offset_clamp *ptr);

void
brw_dump_gs_unit_state(const struct brw_gs_unit_state *ptr);

void
brw_dump_indexbuffer(const struct brw_indexbuffer *ptr);

void
brw_dump_line_stipple(const struct brw_line_stipple *ptr);

void
brw_dump_mi_flush(const struct brw_mi_flush *ptr);

void
brw_dump_pipe_control(const struct brw_pipe_control *ptr);

void
brw_dump_pipeline_select(const struct brw_pipeline_select *ptr);

void
brw_dump_pipelined_state_pointers(const struct brw_pipelined_state_pointers *ptr);

void
brw_dump_polygon_stipple(const struct brw_polygon_stipple *ptr);

void
brw_dump_polygon_stipple_offset(const struct brw_polygon_stipple_offset *ptr);

void
brw_dump_sampler_default_color(const struct brw_sampler_default_color *ptr);

void
brw_dump_sampler_state(const struct brw_sampler_state *ptr);

void
brw_dump_sf_unit_state(const struct brw_sf_unit_state *ptr);

void
brw_dump_sf_viewport(const struct brw_sf_viewport *ptr);

void
brw_dump_ss0(const struct brw_ss0 *ptr);

void
brw_dump_ss1(const struct brw_ss1 *ptr);

void
brw_dump_ss2(const struct brw_ss2 *ptr);

void
brw_dump_ss3(const struct brw_ss3 *ptr);

void
brw_dump_state_base_address(const struct brw_state_base_address *ptr);

void
brw_dump_state_prefetch(const struct brw_state_prefetch *ptr);

void
brw_dump_surf_ss0(const struct brw_surf_ss0 *ptr);

void
brw_dump_surf_ss1(const struct brw_surf_ss1 *ptr);

void
brw_dump_surf_ss2(const struct brw_surf_ss2 *ptr);

void
brw_dump_surf_ss3(const struct brw_surf_ss3 *ptr);

void
brw_dump_surf_ss4(const struct brw_surf_ss4 *ptr);

void
brw_dump_surf_ss5(const struct brw_surf_ss5 *ptr);

void
brw_dump_surface_state(const struct brw_surface_state *ptr);

void
brw_dump_system_instruction_pointer(const struct brw_system_instruction_pointer *ptr);

void
brw_dump_urb_fence(const struct brw_urb_fence *ptr);

void
brw_dump_urb_immediate(const struct brw_urb_immediate *ptr);

void
brw_dump_vb_array_state(const struct brw_vb_array_state *ptr);

void
brw_dump_vertex_buffer_state(const struct brw_vertex_buffer_state *ptr);

void
brw_dump_vertex_element_packet(const struct brw_vertex_element_packet *ptr);

void
brw_dump_vertex_element_state(const struct brw_vertex_element_state *ptr);

void
brw_dump_vf_statistics(const struct brw_vf_statistics *ptr);

void
brw_dump_vs_unit_state(const struct brw_vs_unit_state *ptr);

void
brw_dump_wm_unit_state(const struct brw_wm_unit_state *ptr);


#endif /* BRW_STRUCTS_DUMP_H */

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

#include "util/u_debug.h"

#include "brw_types.h"
#include "brw_structs.h"
#include "brw_structs_dump.h"

void
brw_dump_3d_control(const struct brw_3d_control *ptr)
{
   debug_printf("\t\t.header.length = 0x%x\n", (*ptr).header.length);
   debug_printf("\t\t.header.notify_enable = 0x%x\n", (*ptr).header.notify_enable);
   debug_printf("\t\t.header.wc_flush_enable = 0x%x\n", (*ptr).header.wc_flush_enable);
   debug_printf("\t\t.header.depth_stall_enable = 0x%x\n", (*ptr).header.depth_stall_enable);
   debug_printf("\t\t.header.operation = 0x%x\n", (*ptr).header.operation);
   debug_printf("\t\t.header.opcode = 0x%x\n", (*ptr).header.opcode);
   debug_printf("\t\t.dest.dest_addr_type = 0x%x\n", (*ptr).dest.dest_addr_type);
   debug_printf("\t\t.dest.dest_addr = 0x%x\n", (*ptr).dest.dest_addr);
   debug_printf("\t\t.dword2 = 0x%x\n", (*ptr).dword2);
   debug_printf("\t\t.dword3 = 0x%x\n", (*ptr).dword3);
}

void
brw_dump_3d_primitive(const struct brw_3d_primitive *ptr)
{
   debug_printf("\t\t.header.length = 0x%x\n", (*ptr).header.length);
   debug_printf("\t\t.header.topology = 0x%x\n", (*ptr).header.topology);
   debug_printf("\t\t.header.indexed = 0x%x\n", (*ptr).header.indexed);
   debug_printf("\t\t.header.opcode = 0x%x\n", (*ptr).header.opcode);
   debug_printf("\t\t.verts_per_instance = 0x%x\n", (*ptr).verts_per_instance);
   debug_printf("\t\t.start_vert_location = 0x%x\n", (*ptr).start_vert_location);
   debug_printf("\t\t.instance_count = 0x%x\n", (*ptr).instance_count);
   debug_printf("\t\t.start_instance_location = 0x%x\n", (*ptr).start_instance_location);
   debug_printf("\t\t.base_vert_location = 0x%x\n", (*ptr).base_vert_location);
}

void
brw_dump_aa_line_parameters(const struct brw_aa_line_parameters *ptr)
{
   debug_printf("\t\t.header.length = 0x%x\n", (*ptr).header.length);
   debug_printf("\t\t.header.opcode = 0x%x\n", (*ptr).header.opcode);
   debug_printf("\t\t.bits0.aa_coverage_scope = 0x%x\n", (*ptr).bits0.aa_coverage_scope);
   debug_printf("\t\t.bits0.aa_coverage_bias = 0x%x\n", (*ptr).bits0.aa_coverage_bias);
   debug_printf("\t\t.bits1.aa_coverage_endcap_slope = 0x%x\n", (*ptr).bits1.aa_coverage_endcap_slope);
   debug_printf("\t\t.bits1.aa_coverage_endcap_bias = 0x%x\n", (*ptr).bits1.aa_coverage_endcap_bias);
}

void
brw_dump_binding_table_pointers(const struct brw_binding_table_pointers *ptr)
{
   debug_printf("\t\t.header.length = 0x%x\n", (*ptr).header.length);
   debug_printf("\t\t.header.opcode = 0x%x\n", (*ptr).header.opcode);
   debug_printf("\t\t.vs = 0x%x\n", (*ptr).vs);
   debug_printf("\t\t.gs = 0x%x\n", (*ptr).gs);
   debug_printf("\t\t.clp = 0x%x\n", (*ptr).clp);
   debug_printf("\t\t.sf = 0x%x\n", (*ptr).sf);
   debug_printf("\t\t.wm = 0x%x\n", (*ptr).wm);
}

void
brw_dump_blend_constant_color(const struct brw_blend_constant_color *ptr)
{
   debug_printf("\t\t.header.length = 0x%x\n", (*ptr).header.length);
   debug_printf("\t\t.header.opcode = 0x%x\n", (*ptr).header.opcode);
   debug_printf("\t\t.blend_constant_color[0] = %f\n", (*ptr).blend_constant_color[0]);
   debug_printf("\t\t.blend_constant_color[1] = %f\n", (*ptr).blend_constant_color[1]);
   debug_printf("\t\t.blend_constant_color[2] = %f\n", (*ptr).blend_constant_color[2]);
   debug_printf("\t\t.blend_constant_color[3] = %f\n", (*ptr).blend_constant_color[3]);
}

void
brw_dump_cc0(const struct brw_cc0 *ptr)
{
   debug_printf("\t\t.bf_stencil_pass_depth_pass_op = 0x%x\n", (*ptr).bf_stencil_pass_depth_pass_op);
   debug_printf("\t\t.bf_stencil_pass_depth_fail_op = 0x%x\n", (*ptr).bf_stencil_pass_depth_fail_op);
   debug_printf("\t\t.bf_stencil_fail_op = 0x%x\n", (*ptr).bf_stencil_fail_op);
   debug_printf("\t\t.bf_stencil_func = 0x%x\n", (*ptr).bf_stencil_func);
   debug_printf("\t\t.bf_stencil_enable = 0x%x\n", (*ptr).bf_stencil_enable);
   debug_printf("\t\t.stencil_write_enable = 0x%x\n", (*ptr).stencil_write_enable);
   debug_printf("\t\t.stencil_pass_depth_pass_op = 0x%x\n", (*ptr).stencil_pass_depth_pass_op);
   debug_printf("\t\t.stencil_pass_depth_fail_op = 0x%x\n", (*ptr).stencil_pass_depth_fail_op);
   debug_printf("\t\t.stencil_fail_op = 0x%x\n", (*ptr).stencil_fail_op);
   debug_printf("\t\t.stencil_func = 0x%x\n", (*ptr).stencil_func);
   debug_printf("\t\t.stencil_enable = 0x%x\n", (*ptr).stencil_enable);
}

void
brw_dump_cc1(const struct brw_cc1 *ptr)
{
   debug_printf("\t\t.bf_stencil_ref = 0x%x\n", (*ptr).bf_stencil_ref);
   debug_printf("\t\t.stencil_write_mask = 0x%x\n", (*ptr).stencil_write_mask);
   debug_printf("\t\t.stencil_test_mask = 0x%x\n", (*ptr).stencil_test_mask);
   debug_printf("\t\t.stencil_ref = 0x%x\n", (*ptr).stencil_ref);
}

void
brw_dump_cc2(const struct brw_cc2 *ptr)
{
   debug_printf("\t\t.logicop_enable = 0x%x\n", (*ptr).logicop_enable);
   debug_printf("\t\t.depth_write_enable = 0x%x\n", (*ptr).depth_write_enable);
   debug_printf("\t\t.depth_test_function = 0x%x\n", (*ptr).depth_test_function);
   debug_printf("\t\t.depth_test = 0x%x\n", (*ptr).depth_test);
   debug_printf("\t\t.bf_stencil_write_mask = 0x%x\n", (*ptr).bf_stencil_write_mask);
   debug_printf("\t\t.bf_stencil_test_mask = 0x%x\n", (*ptr).bf_stencil_test_mask);
}

void
brw_dump_cc3(const struct brw_cc3 *ptr)
{
   debug_printf("\t\t.alpha_test_func = 0x%x\n", (*ptr).alpha_test_func);
   debug_printf("\t\t.alpha_test = 0x%x\n", (*ptr).alpha_test);
   debug_printf("\t\t.blend_enable = 0x%x\n", (*ptr).blend_enable);
   debug_printf("\t\t.ia_blend_enable = 0x%x\n", (*ptr).ia_blend_enable);
   debug_printf("\t\t.alpha_test_format = 0x%x\n", (*ptr).alpha_test_format);
}

void
brw_dump_cc4(const struct brw_cc4 *ptr)
{
   debug_printf("\t\t.cc_viewport_state_offset = 0x%x\n", (*ptr).cc_viewport_state_offset);
}

void
brw_dump_cc5(const struct brw_cc5 *ptr)
{
   debug_printf("\t\t.ia_dest_blend_factor = 0x%x\n", (*ptr).ia_dest_blend_factor);
   debug_printf("\t\t.ia_src_blend_factor = 0x%x\n", (*ptr).ia_src_blend_factor);
   debug_printf("\t\t.ia_blend_function = 0x%x\n", (*ptr).ia_blend_function);
   debug_printf("\t\t.statistics_enable = 0x%x\n", (*ptr).statistics_enable);
   debug_printf("\t\t.logicop_func = 0x%x\n", (*ptr).logicop_func);
   debug_printf("\t\t.dither_enable = 0x%x\n", (*ptr).dither_enable);
}

void
brw_dump_cc6(const struct brw_cc6 *ptr)
{
   debug_printf("\t\t.clamp_post_alpha_blend = 0x%x\n", (*ptr).clamp_post_alpha_blend);
   debug_printf("\t\t.clamp_pre_alpha_blend = 0x%x\n", (*ptr).clamp_pre_alpha_blend);
   debug_printf("\t\t.clamp_range = 0x%x\n", (*ptr).clamp_range);
   debug_printf("\t\t.y_dither_offset = 0x%x\n", (*ptr).y_dither_offset);
   debug_printf("\t\t.x_dither_offset = 0x%x\n", (*ptr).x_dither_offset);
   debug_printf("\t\t.dest_blend_factor = 0x%x\n", (*ptr).dest_blend_factor);
   debug_printf("\t\t.src_blend_factor = 0x%x\n", (*ptr).src_blend_factor);
   debug_printf("\t\t.blend_function = 0x%x\n", (*ptr).blend_function);
}

void
brw_dump_cc7(const struct brw_cc7 *ptr)
{
   debug_printf("\t\t.alpha_ref.f = %f\n", (*ptr).alpha_ref.f);
   debug_printf("\t\t.alpha_ref.ub[0] = 0x%x\n", (*ptr).alpha_ref.ub[0]);
   debug_printf("\t\t.alpha_ref.ub[1] = 0x%x\n", (*ptr).alpha_ref.ub[1]);
   debug_printf("\t\t.alpha_ref.ub[2] = 0x%x\n", (*ptr).alpha_ref.ub[2]);
   debug_printf("\t\t.alpha_ref.ub[3] = 0x%x\n", (*ptr).alpha_ref.ub[3]);
}

void
brw_dump_cc_unit_state(const struct brw_cc_unit_state *ptr)
{
   debug_printf("\t\t.cc0.bf_stencil_pass_depth_pass_op = 0x%x\n", (*ptr).cc0.bf_stencil_pass_depth_pass_op);
   debug_printf("\t\t.cc0.bf_stencil_pass_depth_fail_op = 0x%x\n", (*ptr).cc0.bf_stencil_pass_depth_fail_op);
   debug_printf("\t\t.cc0.bf_stencil_fail_op = 0x%x\n", (*ptr).cc0.bf_stencil_fail_op);
   debug_printf("\t\t.cc0.bf_stencil_func = 0x%x\n", (*ptr).cc0.bf_stencil_func);
   debug_printf("\t\t.cc0.bf_stencil_enable = 0x%x\n", (*ptr).cc0.bf_stencil_enable);
   debug_printf("\t\t.cc0.stencil_write_enable = 0x%x\n", (*ptr).cc0.stencil_write_enable);
   debug_printf("\t\t.cc0.stencil_pass_depth_pass_op = 0x%x\n", (*ptr).cc0.stencil_pass_depth_pass_op);
   debug_printf("\t\t.cc0.stencil_pass_depth_fail_op = 0x%x\n", (*ptr).cc0.stencil_pass_depth_fail_op);
   debug_printf("\t\t.cc0.stencil_fail_op = 0x%x\n", (*ptr).cc0.stencil_fail_op);
   debug_printf("\t\t.cc0.stencil_func = 0x%x\n", (*ptr).cc0.stencil_func);
   debug_printf("\t\t.cc0.stencil_enable = 0x%x\n", (*ptr).cc0.stencil_enable);
   debug_printf("\t\t.cc1.bf_stencil_ref = 0x%x\n", (*ptr).cc1.bf_stencil_ref);
   debug_printf("\t\t.cc1.stencil_write_mask = 0x%x\n", (*ptr).cc1.stencil_write_mask);
   debug_printf("\t\t.cc1.stencil_test_mask = 0x%x\n", (*ptr).cc1.stencil_test_mask);
   debug_printf("\t\t.cc1.stencil_ref = 0x%x\n", (*ptr).cc1.stencil_ref);
   debug_printf("\t\t.cc2.logicop_enable = 0x%x\n", (*ptr).cc2.logicop_enable);
   debug_printf("\t\t.cc2.depth_write_enable = 0x%x\n", (*ptr).cc2.depth_write_enable);
   debug_printf("\t\t.cc2.depth_test_function = 0x%x\n", (*ptr).cc2.depth_test_function);
   debug_printf("\t\t.cc2.depth_test = 0x%x\n", (*ptr).cc2.depth_test);
   debug_printf("\t\t.cc2.bf_stencil_write_mask = 0x%x\n", (*ptr).cc2.bf_stencil_write_mask);
   debug_printf("\t\t.cc2.bf_stencil_test_mask = 0x%x\n", (*ptr).cc2.bf_stencil_test_mask);
   debug_printf("\t\t.cc3.alpha_test_func = 0x%x\n", (*ptr).cc3.alpha_test_func);
   debug_printf("\t\t.cc3.alpha_test = 0x%x\n", (*ptr).cc3.alpha_test);
   debug_printf("\t\t.cc3.blend_enable = 0x%x\n", (*ptr).cc3.blend_enable);
   debug_printf("\t\t.cc3.ia_blend_enable = 0x%x\n", (*ptr).cc3.ia_blend_enable);
   debug_printf("\t\t.cc3.alpha_test_format = 0x%x\n", (*ptr).cc3.alpha_test_format);
   debug_printf("\t\t.cc4.cc_viewport_state_offset = 0x%x\n", (*ptr).cc4.cc_viewport_state_offset);
   debug_printf("\t\t.cc5.ia_dest_blend_factor = 0x%x\n", (*ptr).cc5.ia_dest_blend_factor);
   debug_printf("\t\t.cc5.ia_src_blend_factor = 0x%x\n", (*ptr).cc5.ia_src_blend_factor);
   debug_printf("\t\t.cc5.ia_blend_function = 0x%x\n", (*ptr).cc5.ia_blend_function);
   debug_printf("\t\t.cc5.statistics_enable = 0x%x\n", (*ptr).cc5.statistics_enable);
   debug_printf("\t\t.cc5.logicop_func = 0x%x\n", (*ptr).cc5.logicop_func);
   debug_printf("\t\t.cc5.dither_enable = 0x%x\n", (*ptr).cc5.dither_enable);
   debug_printf("\t\t.cc6.clamp_post_alpha_blend = 0x%x\n", (*ptr).cc6.clamp_post_alpha_blend);
   debug_printf("\t\t.cc6.clamp_pre_alpha_blend = 0x%x\n", (*ptr).cc6.clamp_pre_alpha_blend);
   debug_printf("\t\t.cc6.clamp_range = 0x%x\n", (*ptr).cc6.clamp_range);
   debug_printf("\t\t.cc6.y_dither_offset = 0x%x\n", (*ptr).cc6.y_dither_offset);
   debug_printf("\t\t.cc6.x_dither_offset = 0x%x\n", (*ptr).cc6.x_dither_offset);
   debug_printf("\t\t.cc6.dest_blend_factor = 0x%x\n", (*ptr).cc6.dest_blend_factor);
   debug_printf("\t\t.cc6.src_blend_factor = 0x%x\n", (*ptr).cc6.src_blend_factor);
   debug_printf("\t\t.cc6.blend_function = 0x%x\n", (*ptr).cc6.blend_function);
   debug_printf("\t\t.cc7.alpha_ref.f = %f\n", (*ptr).cc7.alpha_ref.f);
   debug_printf("\t\t.cc7.alpha_ref.ub[0] = 0x%x\n", (*ptr).cc7.alpha_ref.ub[0]);
   debug_printf("\t\t.cc7.alpha_ref.ub[1] = 0x%x\n", (*ptr).cc7.alpha_ref.ub[1]);
   debug_printf("\t\t.cc7.alpha_ref.ub[2] = 0x%x\n", (*ptr).cc7.alpha_ref.ub[2]);
   debug_printf("\t\t.cc7.alpha_ref.ub[3] = 0x%x\n", (*ptr).cc7.alpha_ref.ub[3]);
}

void
brw_dump_cc_viewport(const struct brw_cc_viewport *ptr)
{
   debug_printf("\t\t.min_depth = %f\n", (*ptr).min_depth);
   debug_printf("\t\t.max_depth = %f\n", (*ptr).max_depth);
}

void
brw_dump_clip_unit_state(const struct brw_clip_unit_state *ptr)
{
   debug_printf("\t\t.thread0.grf_reg_count = 0x%x\n", (*ptr).thread0.grf_reg_count);
   debug_printf("\t\t.thread0.kernel_start_pointer = 0x%x\n", (*ptr).thread0.kernel_start_pointer);
   debug_printf("\t\t.thread1.sw_exception_enable = 0x%x\n", (*ptr).thread1.sw_exception_enable);
   debug_printf("\t\t.thread1.mask_stack_exception_enable = 0x%x\n", (*ptr).thread1.mask_stack_exception_enable);
   debug_printf("\t\t.thread1.illegal_op_exception_enable = 0x%x\n", (*ptr).thread1.illegal_op_exception_enable);
   debug_printf("\t\t.thread1.floating_point_mode = 0x%x\n", (*ptr).thread1.floating_point_mode);
   debug_printf("\t\t.thread1.thread_priority = 0x%x\n", (*ptr).thread1.thread_priority);
   debug_printf("\t\t.thread1.binding_table_entry_count = 0x%x\n", (*ptr).thread1.binding_table_entry_count);
   debug_printf("\t\t.thread1.single_program_flow = 0x%x\n", (*ptr).thread1.single_program_flow);
   debug_printf("\t\t.thread2.per_thread_scratch_space = 0x%x\n", (*ptr).thread2.per_thread_scratch_space);
   debug_printf("\t\t.thread2.scratch_space_base_pointer = 0x%x\n", (*ptr).thread2.scratch_space_base_pointer);
   debug_printf("\t\t.thread3.dispatch_grf_start_reg = 0x%x\n", (*ptr).thread3.dispatch_grf_start_reg);
   debug_printf("\t\t.thread3.urb_entry_read_offset = 0x%x\n", (*ptr).thread3.urb_entry_read_offset);
   debug_printf("\t\t.thread3.urb_entry_read_length = 0x%x\n", (*ptr).thread3.urb_entry_read_length);
   debug_printf("\t\t.thread3.const_urb_entry_read_offset = 0x%x\n", (*ptr).thread3.const_urb_entry_read_offset);
   debug_printf("\t\t.thread3.const_urb_entry_read_length = 0x%x\n", (*ptr).thread3.const_urb_entry_read_length);
   debug_printf("\t\t.thread4.gs_output_stats = 0x%x\n", (*ptr).thread4.gs_output_stats);
   debug_printf("\t\t.thread4.stats_enable = 0x%x\n", (*ptr).thread4.stats_enable);
   debug_printf("\t\t.thread4.nr_urb_entries = 0x%x\n", (*ptr).thread4.nr_urb_entries);
   debug_printf("\t\t.thread4.urb_entry_allocation_size = 0x%x\n", (*ptr).thread4.urb_entry_allocation_size);
   debug_printf("\t\t.thread4.max_threads = 0x%x\n", (*ptr).thread4.max_threads);
   debug_printf("\t\t.clip5.clip_mode = 0x%x\n", (*ptr).clip5.clip_mode);
   debug_printf("\t\t.clip5.userclip_enable_flags = 0x%x\n", (*ptr).clip5.userclip_enable_flags);
   debug_printf("\t\t.clip5.userclip_must_clip = 0x%x\n", (*ptr).clip5.userclip_must_clip);
   debug_printf("\t\t.clip5.negative_w_clip_test = 0x%x\n", (*ptr).clip5.negative_w_clip_test);
   debug_printf("\t\t.clip5.guard_band_enable = 0x%x\n", (*ptr).clip5.guard_band_enable);
   debug_printf("\t\t.clip5.viewport_z_clip_enable = 0x%x\n", (*ptr).clip5.viewport_z_clip_enable);
   debug_printf("\t\t.clip5.viewport_xy_clip_enable = 0x%x\n", (*ptr).clip5.viewport_xy_clip_enable);
   debug_printf("\t\t.clip5.vertex_position_space = 0x%x\n", (*ptr).clip5.vertex_position_space);
   debug_printf("\t\t.clip5.api_mode = 0x%x\n", (*ptr).clip5.api_mode);
   debug_printf("\t\t.clip6.clipper_viewport_state_ptr = 0x%x\n", (*ptr).clip6.clipper_viewport_state_ptr);
   debug_printf("\t\t.viewport_xmin = %f\n", (*ptr).viewport_xmin);
   debug_printf("\t\t.viewport_xmax = %f\n", (*ptr).viewport_xmax);
   debug_printf("\t\t.viewport_ymin = %f\n", (*ptr).viewport_ymin);
   debug_printf("\t\t.viewport_ymax = %f\n", (*ptr).viewport_ymax);
}

void
brw_dump_clipper_viewport(const struct brw_clipper_viewport *ptr)
{
   debug_printf("\t\t.xmin = %f\n", (*ptr).xmin);
   debug_printf("\t\t.xmax = %f\n", (*ptr).xmax);
   debug_printf("\t\t.ymin = %f\n", (*ptr).ymin);
   debug_printf("\t\t.ymax = %f\n", (*ptr).ymax);
}

void
brw_dump_constant_buffer(const struct brw_constant_buffer *ptr)
{
   debug_printf("\t\t.header.length = 0x%x\n", (*ptr).header.length);
   debug_printf("\t\t.header.valid = 0x%x\n", (*ptr).header.valid);
   debug_printf("\t\t.header.opcode = 0x%x\n", (*ptr).header.opcode);
   debug_printf("\t\t.bits0.buffer_length = 0x%x\n", (*ptr).bits0.buffer_length);
   debug_printf("\t\t.bits0.buffer_address = 0x%x\n", (*ptr).bits0.buffer_address);
}

void
brw_dump_cs_urb_state(const struct brw_cs_urb_state *ptr)
{
   debug_printf("\t\t.header.length = 0x%x\n", (*ptr).header.length);
   debug_printf("\t\t.header.opcode = 0x%x\n", (*ptr).header.opcode);
   debug_printf("\t\t.bits0.nr_urb_entries = 0x%x\n", (*ptr).bits0.nr_urb_entries);
   debug_printf("\t\t.bits0.urb_entry_size = 0x%x\n", (*ptr).bits0.urb_entry_size);
}

void
brw_dump_depthbuffer(const struct brw_depthbuffer *ptr)
{
   debug_printf("\t\t.header.bits.length = 0x%x\n", (*ptr).header.bits.length);
   debug_printf("\t\t.header.bits.opcode = 0x%x\n", (*ptr).header.bits.opcode);
   debug_printf("\t\t.dword1.bits.pitch = 0x%x\n", (*ptr).dword1.bits.pitch);
   debug_printf("\t\t.dword1.bits.format = 0x%x\n", (*ptr).dword1.bits.format);
   debug_printf("\t\t.dword1.bits.software_tiled_rendering_mode = 0x%x\n", (*ptr).dword1.bits.software_tiled_rendering_mode);
   debug_printf("\t\t.dword1.bits.depth_offset_disable = 0x%x\n", (*ptr).dword1.bits.depth_offset_disable);
   debug_printf("\t\t.dword1.bits.tile_walk = 0x%x\n", (*ptr).dword1.bits.tile_walk);
   debug_printf("\t\t.dword1.bits.tiled_surface = 0x%x\n", (*ptr).dword1.bits.tiled_surface);
   debug_printf("\t\t.dword1.bits.surface_type = 0x%x\n", (*ptr).dword1.bits.surface_type);
   debug_printf("\t\t.dword2_base_addr = 0x%x\n", (*ptr).dword2_base_addr);
   debug_printf("\t\t.dword3.bits.mipmap_layout = 0x%x\n", (*ptr).dword3.bits.mipmap_layout);
   debug_printf("\t\t.dword3.bits.lod = 0x%x\n", (*ptr).dword3.bits.lod);
   debug_printf("\t\t.dword3.bits.width = 0x%x\n", (*ptr).dword3.bits.width);
   debug_printf("\t\t.dword3.bits.height = 0x%x\n", (*ptr).dword3.bits.height);
   debug_printf("\t\t.dword4.bits.min_array_element = 0x%x\n", (*ptr).dword4.bits.min_array_element);
   debug_printf("\t\t.dword4.bits.depth = 0x%x\n", (*ptr).dword4.bits.depth);
}

void
brw_dump_depthbuffer_g4x(const struct brw_depthbuffer_g4x *ptr)
{
   debug_printf("\t\t.header.bits.length = 0x%x\n", (*ptr).header.bits.length);
   debug_printf("\t\t.header.bits.opcode = 0x%x\n", (*ptr).header.bits.opcode);
   debug_printf("\t\t.dword1.bits.pitch = 0x%x\n", (*ptr).dword1.bits.pitch);
   debug_printf("\t\t.dword1.bits.format = 0x%x\n", (*ptr).dword1.bits.format);
   debug_printf("\t\t.dword1.bits.software_tiled_rendering_mode = 0x%x\n", (*ptr).dword1.bits.software_tiled_rendering_mode);
   debug_printf("\t\t.dword1.bits.depth_offset_disable = 0x%x\n", (*ptr).dword1.bits.depth_offset_disable);
   debug_printf("\t\t.dword1.bits.tile_walk = 0x%x\n", (*ptr).dword1.bits.tile_walk);
   debug_printf("\t\t.dword1.bits.tiled_surface = 0x%x\n", (*ptr).dword1.bits.tiled_surface);
   debug_printf("\t\t.dword1.bits.surface_type = 0x%x\n", (*ptr).dword1.bits.surface_type);
   debug_printf("\t\t.dword2_base_addr = 0x%x\n", (*ptr).dword2_base_addr);
   debug_printf("\t\t.dword3.bits.mipmap_layout = 0x%x\n", (*ptr).dword3.bits.mipmap_layout);
   debug_printf("\t\t.dword3.bits.lod = 0x%x\n", (*ptr).dword3.bits.lod);
   debug_printf("\t\t.dword3.bits.width = 0x%x\n", (*ptr).dword3.bits.width);
   debug_printf("\t\t.dword3.bits.height = 0x%x\n", (*ptr).dword3.bits.height);
   debug_printf("\t\t.dword4.bits.min_array_element = 0x%x\n", (*ptr).dword4.bits.min_array_element);
   debug_printf("\t\t.dword4.bits.depth = 0x%x\n", (*ptr).dword4.bits.depth);
   debug_printf("\t\t.dword5.bits.xoffset = 0x%x\n", (*ptr).dword5.bits.xoffset);
   debug_printf("\t\t.dword5.bits.yoffset = 0x%x\n", (*ptr).dword5.bits.yoffset);
}

void
brw_dump_drawrect(const struct brw_drawrect *ptr)
{
   debug_printf("\t\t.header.length = 0x%x\n", (*ptr).header.length);
   debug_printf("\t\t.header.opcode = 0x%x\n", (*ptr).header.opcode);
   debug_printf("\t\t.xmin = 0x%x\n", (*ptr).xmin);
   debug_printf("\t\t.ymin = 0x%x\n", (*ptr).ymin);
   debug_printf("\t\t.xmax = 0x%x\n", (*ptr).xmax);
   debug_printf("\t\t.ymax = 0x%x\n", (*ptr).ymax);
   debug_printf("\t\t.xorg = 0x%x\n", (*ptr).xorg);
   debug_printf("\t\t.yorg = 0x%x\n", (*ptr).yorg);
}

void
brw_dump_global_depth_offset_clamp(const struct brw_global_depth_offset_clamp *ptr)
{
   debug_printf("\t\t.header.length = 0x%x\n", (*ptr).header.length);
   debug_printf("\t\t.header.opcode = 0x%x\n", (*ptr).header.opcode);
   debug_printf("\t\t.depth_offset_clamp = %f\n", (*ptr).depth_offset_clamp);
}

void
brw_dump_gs_unit_state(const struct brw_gs_unit_state *ptr)
{
   debug_printf("\t\t.thread0.grf_reg_count = 0x%x\n", (*ptr).thread0.grf_reg_count);
   debug_printf("\t\t.thread0.kernel_start_pointer = 0x%x\n", (*ptr).thread0.kernel_start_pointer);
   debug_printf("\t\t.thread1.ext_halt_exception_enable = 0x%x\n", (*ptr).thread1.ext_halt_exception_enable);
   debug_printf("\t\t.thread1.sw_exception_enable = 0x%x\n", (*ptr).thread1.sw_exception_enable);
   debug_printf("\t\t.thread1.mask_stack_exception_enable = 0x%x\n", (*ptr).thread1.mask_stack_exception_enable);
   debug_printf("\t\t.thread1.timeout_exception_enable = 0x%x\n", (*ptr).thread1.timeout_exception_enable);
   debug_printf("\t\t.thread1.illegal_op_exception_enable = 0x%x\n", (*ptr).thread1.illegal_op_exception_enable);
   debug_printf("\t\t.thread1.depth_coef_urb_read_offset = 0x%x\n", (*ptr).thread1.depth_coef_urb_read_offset);
   debug_printf("\t\t.thread1.floating_point_mode = 0x%x\n", (*ptr).thread1.floating_point_mode);
   debug_printf("\t\t.thread1.thread_priority = 0x%x\n", (*ptr).thread1.thread_priority);
   debug_printf("\t\t.thread1.binding_table_entry_count = 0x%x\n", (*ptr).thread1.binding_table_entry_count);
   debug_printf("\t\t.thread1.single_program_flow = 0x%x\n", (*ptr).thread1.single_program_flow);
   debug_printf("\t\t.thread2.per_thread_scratch_space = 0x%x\n", (*ptr).thread2.per_thread_scratch_space);
   debug_printf("\t\t.thread2.scratch_space_base_pointer = 0x%x\n", (*ptr).thread2.scratch_space_base_pointer);
   debug_printf("\t\t.thread3.dispatch_grf_start_reg = 0x%x\n", (*ptr).thread3.dispatch_grf_start_reg);
   debug_printf("\t\t.thread3.urb_entry_read_offset = 0x%x\n", (*ptr).thread3.urb_entry_read_offset);
   debug_printf("\t\t.thread3.urb_entry_read_length = 0x%x\n", (*ptr).thread3.urb_entry_read_length);
   debug_printf("\t\t.thread3.const_urb_entry_read_offset = 0x%x\n", (*ptr).thread3.const_urb_entry_read_offset);
   debug_printf("\t\t.thread3.const_urb_entry_read_length = 0x%x\n", (*ptr).thread3.const_urb_entry_read_length);
   debug_printf("\t\t.thread4.rendering_enable = 0x%x\n", (*ptr).thread4.rendering_enable);
   debug_printf("\t\t.thread4.stats_enable = 0x%x\n", (*ptr).thread4.stats_enable);
   debug_printf("\t\t.thread4.nr_urb_entries = 0x%x\n", (*ptr).thread4.nr_urb_entries);
   debug_printf("\t\t.thread4.urb_entry_allocation_size = 0x%x\n", (*ptr).thread4.urb_entry_allocation_size);
   debug_printf("\t\t.thread4.max_threads = 0x%x\n", (*ptr).thread4.max_threads);
   debug_printf("\t\t.gs5.sampler_count = 0x%x\n", (*ptr).gs5.sampler_count);
   debug_printf("\t\t.gs5.sampler_state_pointer = 0x%x\n", (*ptr).gs5.sampler_state_pointer);
   debug_printf("\t\t.gs6.max_vp_index = 0x%x\n", (*ptr).gs6.max_vp_index);
   debug_printf("\t\t.gs6.svbi_post_inc_value = 0x%x\n", (*ptr).gs6.svbi_post_inc_value);
   debug_printf("\t\t.gs6.svbi_post_inc_enable = 0x%x\n", (*ptr).gs6.svbi_post_inc_enable);
   debug_printf("\t\t.gs6.svbi_payload = 0x%x\n", (*ptr).gs6.svbi_payload);
   debug_printf("\t\t.gs6.discard_adjaceny = 0x%x\n", (*ptr).gs6.discard_adjaceny);
   debug_printf("\t\t.gs6.reorder_enable = 0x%x\n", (*ptr).gs6.reorder_enable);
}

void
brw_dump_indexbuffer(const struct brw_indexbuffer *ptr)
{
   debug_printf("\t\t.header.bits.length = 0x%x\n", (*ptr).header.bits.length);
   debug_printf("\t\t.header.bits.index_format = 0x%x\n", (*ptr).header.bits.index_format);
   debug_printf("\t\t.header.bits.cut_index_enable = 0x%x\n", (*ptr).header.bits.cut_index_enable);
   debug_printf("\t\t.header.bits.opcode = 0x%x\n", (*ptr).header.bits.opcode);
   debug_printf("\t\t.buffer_start = 0x%x\n", (*ptr).buffer_start);
   debug_printf("\t\t.buffer_end = 0x%x\n", (*ptr).buffer_end);
}

void
brw_dump_line_stipple(const struct brw_line_stipple *ptr)
{
   debug_printf("\t\t.header.length = 0x%x\n", (*ptr).header.length);
   debug_printf("\t\t.header.opcode = 0x%x\n", (*ptr).header.opcode);
   debug_printf("\t\t.bits0.pattern = 0x%x\n", (*ptr).bits0.pattern);
   debug_printf("\t\t.bits1.repeat_count = 0x%x\n", (*ptr).bits1.repeat_count);
   debug_printf("\t\t.bits1.inverse_repeat_count = 0x%x\n", (*ptr).bits1.inverse_repeat_count);
}

void
brw_dump_mi_flush(const struct brw_mi_flush *ptr)
{
   debug_printf("\t\t.flags = 0x%x\n", (*ptr).flags);
   debug_printf("\t\t.opcode = 0x%x\n", (*ptr).opcode);
}

void
brw_dump_pipe_control(const struct brw_pipe_control *ptr)
{
   debug_printf("\t\t.header.length = 0x%x\n", (*ptr).header.length);
   debug_printf("\t\t.header.notify_enable = 0x%x\n", (*ptr).header.notify_enable);
   debug_printf("\t\t.header.texture_cache_flush_enable = 0x%x\n", (*ptr).header.texture_cache_flush_enable);
   debug_printf("\t\t.header.indirect_state_pointers_disable = 0x%x\n", (*ptr).header.indirect_state_pointers_disable);
   debug_printf("\t\t.header.instruction_state_cache_flush_enable = 0x%x\n", (*ptr).header.instruction_state_cache_flush_enable);
   debug_printf("\t\t.header.write_cache_flush_enable = 0x%x\n", (*ptr).header.write_cache_flush_enable);
   debug_printf("\t\t.header.depth_stall_enable = 0x%x\n", (*ptr).header.depth_stall_enable);
   debug_printf("\t\t.header.post_sync_operation = 0x%x\n", (*ptr).header.post_sync_operation);
   debug_printf("\t\t.header.opcode = 0x%x\n", (*ptr).header.opcode);
   debug_printf("\t\t.bits1.dest_addr_type = 0x%x\n", (*ptr).bits1.dest_addr_type);
   debug_printf("\t\t.bits1.dest_addr = 0x%x\n", (*ptr).bits1.dest_addr);
   debug_printf("\t\t.data0 = 0x%x\n", (*ptr).data0);
   debug_printf("\t\t.data1 = 0x%x\n", (*ptr).data1);
}

void
brw_dump_pipeline_select(const struct brw_pipeline_select *ptr)
{
   debug_printf("\t\t.header.pipeline_select = 0x%x\n", (*ptr).header.pipeline_select);
   debug_printf("\t\t.header.opcode = 0x%x\n", (*ptr).header.opcode);
}

void
brw_dump_pipelined_state_pointers(const struct brw_pipelined_state_pointers *ptr)
{
   debug_printf("\t\t.header.length = 0x%x\n", (*ptr).header.length);
   debug_printf("\t\t.header.opcode = 0x%x\n", (*ptr).header.opcode);
   debug_printf("\t\t.vs.offset = 0x%x\n", (*ptr).vs.offset);
   debug_printf("\t\t.gs.enable = 0x%x\n", (*ptr).gs.enable);
   debug_printf("\t\t.gs.offset = 0x%x\n", (*ptr).gs.offset);
   debug_printf("\t\t.clp.enable = 0x%x\n", (*ptr).clp.enable);
   debug_printf("\t\t.clp.offset = 0x%x\n", (*ptr).clp.offset);
   debug_printf("\t\t.sf.offset = 0x%x\n", (*ptr).sf.offset);
   debug_printf("\t\t.wm.offset = 0x%x\n", (*ptr).wm.offset);
   debug_printf("\t\t.cc.offset = 0x%x\n", (*ptr).cc.offset);
}

void
brw_dump_polygon_stipple(const struct brw_polygon_stipple *ptr)
{
   debug_printf("\t\t.header.length = 0x%x\n", (*ptr).header.length);
   debug_printf("\t\t.header.opcode = 0x%x\n", (*ptr).header.opcode);
   debug_printf("\t\t.stipple[0] = 0x%x\n", (*ptr).stipple[0]);
   debug_printf("\t\t.stipple[1] = 0x%x\n", (*ptr).stipple[1]);
   debug_printf("\t\t.stipple[2] = 0x%x\n", (*ptr).stipple[2]);
   debug_printf("\t\t.stipple[3] = 0x%x\n", (*ptr).stipple[3]);
   debug_printf("\t\t.stipple[4] = 0x%x\n", (*ptr).stipple[4]);
   debug_printf("\t\t.stipple[5] = 0x%x\n", (*ptr).stipple[5]);
   debug_printf("\t\t.stipple[6] = 0x%x\n", (*ptr).stipple[6]);
   debug_printf("\t\t.stipple[7] = 0x%x\n", (*ptr).stipple[7]);
   debug_printf("\t\t.stipple[8] = 0x%x\n", (*ptr).stipple[8]);
   debug_printf("\t\t.stipple[9] = 0x%x\n", (*ptr).stipple[9]);
   debug_printf("\t\t.stipple[10] = 0x%x\n", (*ptr).stipple[10]);
   debug_printf("\t\t.stipple[11] = 0x%x\n", (*ptr).stipple[11]);
   debug_printf("\t\t.stipple[12] = 0x%x\n", (*ptr).stipple[12]);
   debug_printf("\t\t.stipple[13] = 0x%x\n", (*ptr).stipple[13]);
   debug_printf("\t\t.stipple[14] = 0x%x\n", (*ptr).stipple[14]);
   debug_printf("\t\t.stipple[15] = 0x%x\n", (*ptr).stipple[15]);
   debug_printf("\t\t.stipple[16] = 0x%x\n", (*ptr).stipple[16]);
   debug_printf("\t\t.stipple[17] = 0x%x\n", (*ptr).stipple[17]);
   debug_printf("\t\t.stipple[18] = 0x%x\n", (*ptr).stipple[18]);
   debug_printf("\t\t.stipple[19] = 0x%x\n", (*ptr).stipple[19]);
   debug_printf("\t\t.stipple[20] = 0x%x\n", (*ptr).stipple[20]);
   debug_printf("\t\t.stipple[21] = 0x%x\n", (*ptr).stipple[21]);
   debug_printf("\t\t.stipple[22] = 0x%x\n", (*ptr).stipple[22]);
   debug_printf("\t\t.stipple[23] = 0x%x\n", (*ptr).stipple[23]);
   debug_printf("\t\t.stipple[24] = 0x%x\n", (*ptr).stipple[24]);
   debug_printf("\t\t.stipple[25] = 0x%x\n", (*ptr).stipple[25]);
   debug_printf("\t\t.stipple[26] = 0x%x\n", (*ptr).stipple[26]);
   debug_printf("\t\t.stipple[27] = 0x%x\n", (*ptr).stipple[27]);
   debug_printf("\t\t.stipple[28] = 0x%x\n", (*ptr).stipple[28]);
   debug_printf("\t\t.stipple[29] = 0x%x\n", (*ptr).stipple[29]);
   debug_printf("\t\t.stipple[30] = 0x%x\n", (*ptr).stipple[30]);
   debug_printf("\t\t.stipple[31] = 0x%x\n", (*ptr).stipple[31]);
}

void
brw_dump_polygon_stipple_offset(const struct brw_polygon_stipple_offset *ptr)
{
   debug_printf("\t\t.header.length = 0x%x\n", (*ptr).header.length);
   debug_printf("\t\t.header.opcode = 0x%x\n", (*ptr).header.opcode);
   debug_printf("\t\t.bits0.y_offset = 0x%x\n", (*ptr).bits0.y_offset);
   debug_printf("\t\t.bits0.x_offset = 0x%x\n", (*ptr).bits0.x_offset);
}

void
brw_dump_sampler_default_color(const struct brw_sampler_default_color *ptr)
{
   debug_printf("\t\t.color[0] = %f\n", (*ptr).color[0]);
   debug_printf("\t\t.color[1] = %f\n", (*ptr).color[1]);
   debug_printf("\t\t.color[2] = %f\n", (*ptr).color[2]);
   debug_printf("\t\t.color[3] = %f\n", (*ptr).color[3]);
}

void
brw_dump_sampler_state(const struct brw_sampler_state *ptr)
{
   debug_printf("\t\t.ss0.shadow_function = 0x%x\n", (*ptr).ss0.shadow_function);
   debug_printf("\t\t.ss0.lod_bias = 0x%x\n", (*ptr).ss0.lod_bias);
   debug_printf("\t\t.ss0.min_filter = 0x%x\n", (*ptr).ss0.min_filter);
   debug_printf("\t\t.ss0.mag_filter = 0x%x\n", (*ptr).ss0.mag_filter);
   debug_printf("\t\t.ss0.mip_filter = 0x%x\n", (*ptr).ss0.mip_filter);
   debug_printf("\t\t.ss0.base_level = 0x%x\n", (*ptr).ss0.base_level);
   debug_printf("\t\t.ss0.lod_preclamp = 0x%x\n", (*ptr).ss0.lod_preclamp);
   debug_printf("\t\t.ss0.default_color_mode = 0x%x\n", (*ptr).ss0.default_color_mode);
   debug_printf("\t\t.ss0.disable = 0x%x\n", (*ptr).ss0.disable);
   debug_printf("\t\t.ss1.r_wrap_mode = 0x%x\n", (*ptr).ss1.r_wrap_mode);
   debug_printf("\t\t.ss1.t_wrap_mode = 0x%x\n", (*ptr).ss1.t_wrap_mode);
   debug_printf("\t\t.ss1.s_wrap_mode = 0x%x\n", (*ptr).ss1.s_wrap_mode);
   debug_printf("\t\t.ss1.max_lod = 0x%x\n", (*ptr).ss1.max_lod);
   debug_printf("\t\t.ss1.min_lod = 0x%x\n", (*ptr).ss1.min_lod);
   debug_printf("\t\t.ss2.default_color_pointer = 0x%x\n", (*ptr).ss2.default_color_pointer);
   debug_printf("\t\t.ss3.max_aniso = 0x%x\n", (*ptr).ss3.max_aniso);
   debug_printf("\t\t.ss3.chroma_key_mode = 0x%x\n", (*ptr).ss3.chroma_key_mode);
   debug_printf("\t\t.ss3.chroma_key_index = 0x%x\n", (*ptr).ss3.chroma_key_index);
   debug_printf("\t\t.ss3.chroma_key_enable = 0x%x\n", (*ptr).ss3.chroma_key_enable);
   debug_printf("\t\t.ss3.monochrome_filter_width = 0x%x\n", (*ptr).ss3.monochrome_filter_width);
   debug_printf("\t\t.ss3.monochrome_filter_height = 0x%x\n", (*ptr).ss3.monochrome_filter_height);
}

void
brw_dump_sf_unit_state(const struct brw_sf_unit_state *ptr)
{
   debug_printf("\t\t.thread0.grf_reg_count = 0x%x\n", (*ptr).thread0.grf_reg_count);
   debug_printf("\t\t.thread0.kernel_start_pointer = 0x%x\n", (*ptr).thread0.kernel_start_pointer);
   debug_printf("\t\t.thread1.ext_halt_exception_enable = 0x%x\n", (*ptr).thread1.ext_halt_exception_enable);
   debug_printf("\t\t.thread1.sw_exception_enable = 0x%x\n", (*ptr).thread1.sw_exception_enable);
   debug_printf("\t\t.thread1.mask_stack_exception_enable = 0x%x\n", (*ptr).thread1.mask_stack_exception_enable);
   debug_printf("\t\t.thread1.timeout_exception_enable = 0x%x\n", (*ptr).thread1.timeout_exception_enable);
   debug_printf("\t\t.thread1.illegal_op_exception_enable = 0x%x\n", (*ptr).thread1.illegal_op_exception_enable);
   debug_printf("\t\t.thread1.depth_coef_urb_read_offset = 0x%x\n", (*ptr).thread1.depth_coef_urb_read_offset);
   debug_printf("\t\t.thread1.floating_point_mode = 0x%x\n", (*ptr).thread1.floating_point_mode);
   debug_printf("\t\t.thread1.thread_priority = 0x%x\n", (*ptr).thread1.thread_priority);
   debug_printf("\t\t.thread1.binding_table_entry_count = 0x%x\n", (*ptr).thread1.binding_table_entry_count);
   debug_printf("\t\t.thread1.single_program_flow = 0x%x\n", (*ptr).thread1.single_program_flow);
   debug_printf("\t\t.thread2.per_thread_scratch_space = 0x%x\n", (*ptr).thread2.per_thread_scratch_space);
   debug_printf("\t\t.thread2.scratch_space_base_pointer = 0x%x\n", (*ptr).thread2.scratch_space_base_pointer);
   debug_printf("\t\t.thread3.dispatch_grf_start_reg = 0x%x\n", (*ptr).thread3.dispatch_grf_start_reg);
   debug_printf("\t\t.thread3.urb_entry_read_offset = 0x%x\n", (*ptr).thread3.urb_entry_read_offset);
   debug_printf("\t\t.thread3.urb_entry_read_length = 0x%x\n", (*ptr).thread3.urb_entry_read_length);
   debug_printf("\t\t.thread3.const_urb_entry_read_offset = 0x%x\n", (*ptr).thread3.const_urb_entry_read_offset);
   debug_printf("\t\t.thread3.const_urb_entry_read_length = 0x%x\n", (*ptr).thread3.const_urb_entry_read_length);
   debug_printf("\t\t.thread4.stats_enable = 0x%x\n", (*ptr).thread4.stats_enable);
   debug_printf("\t\t.thread4.nr_urb_entries = 0x%x\n", (*ptr).thread4.nr_urb_entries);
   debug_printf("\t\t.thread4.urb_entry_allocation_size = 0x%x\n", (*ptr).thread4.urb_entry_allocation_size);
   debug_printf("\t\t.thread4.max_threads = 0x%x\n", (*ptr).thread4.max_threads);
   debug_printf("\t\t.sf5.front_winding = 0x%x\n", (*ptr).sf5.front_winding);
   debug_printf("\t\t.sf5.viewport_transform = 0x%x\n", (*ptr).sf5.viewport_transform);
   debug_printf("\t\t.sf5.sf_viewport_state_offset = 0x%x\n", (*ptr).sf5.sf_viewport_state_offset);
   debug_printf("\t\t.sf6.dest_org_vbias = 0x%x\n", (*ptr).sf6.dest_org_vbias);
   debug_printf("\t\t.sf6.dest_org_hbias = 0x%x\n", (*ptr).sf6.dest_org_hbias);
   debug_printf("\t\t.sf6.scissor = 0x%x\n", (*ptr).sf6.scissor);
   debug_printf("\t\t.sf6.disable_2x2_trifilter = 0x%x\n", (*ptr).sf6.disable_2x2_trifilter);
   debug_printf("\t\t.sf6.disable_zero_pix_trifilter = 0x%x\n", (*ptr).sf6.disable_zero_pix_trifilter);
   debug_printf("\t\t.sf6.point_rast_rule = 0x%x\n", (*ptr).sf6.point_rast_rule);
   debug_printf("\t\t.sf6.line_endcap_aa_region_width = 0x%x\n", (*ptr).sf6.line_endcap_aa_region_width);
   debug_printf("\t\t.sf6.line_width = 0x%x\n", (*ptr).sf6.line_width);
   debug_printf("\t\t.sf6.fast_scissor_disable = 0x%x\n", (*ptr).sf6.fast_scissor_disable);
   debug_printf("\t\t.sf6.cull_mode = 0x%x\n", (*ptr).sf6.cull_mode);
   debug_printf("\t\t.sf6.aa_enable = 0x%x\n", (*ptr).sf6.aa_enable);
   debug_printf("\t\t.sf7.point_size = 0x%x\n", (*ptr).sf7.point_size);
   debug_printf("\t\t.sf7.use_point_size_state = 0x%x\n", (*ptr).sf7.use_point_size_state);
   debug_printf("\t\t.sf7.subpixel_precision = 0x%x\n", (*ptr).sf7.subpixel_precision);
   debug_printf("\t\t.sf7.sprite_point = 0x%x\n", (*ptr).sf7.sprite_point);
   debug_printf("\t\t.sf7.aa_line_distance_mode = 0x%x\n", (*ptr).sf7.aa_line_distance_mode);
   debug_printf("\t\t.sf7.trifan_pv = 0x%x\n", (*ptr).sf7.trifan_pv);
   debug_printf("\t\t.sf7.linestrip_pv = 0x%x\n", (*ptr).sf7.linestrip_pv);
   debug_printf("\t\t.sf7.tristrip_pv = 0x%x\n", (*ptr).sf7.tristrip_pv);
   debug_printf("\t\t.sf7.line_last_pixel_enable = 0x%x\n", (*ptr).sf7.line_last_pixel_enable);
}

void
brw_dump_sf_viewport(const struct brw_sf_viewport *ptr)
{
   debug_printf("\t\t.viewport.m00 = %f\n", (*ptr).viewport.m00);
   debug_printf("\t\t.viewport.m11 = %f\n", (*ptr).viewport.m11);
   debug_printf("\t\t.viewport.m22 = %f\n", (*ptr).viewport.m22);
   debug_printf("\t\t.viewport.m30 = %f\n", (*ptr).viewport.m30);
   debug_printf("\t\t.viewport.m31 = %f\n", (*ptr).viewport.m31);
   debug_printf("\t\t.viewport.m32 = %f\n", (*ptr).viewport.m32);
   debug_printf("\t\t.scissor.xmin = 0x%x\n", (*ptr).scissor.xmin);
   debug_printf("\t\t.scissor.ymin = 0x%x\n", (*ptr).scissor.ymin);
   debug_printf("\t\t.scissor.xmax = 0x%x\n", (*ptr).scissor.xmax);
   debug_printf("\t\t.scissor.ymax = 0x%x\n", (*ptr).scissor.ymax);
}

void
brw_dump_ss0(const struct brw_ss0 *ptr)
{
   debug_printf("\t\t.shadow_function = 0x%x\n", (*ptr).shadow_function);
   debug_printf("\t\t.lod_bias = 0x%x\n", (*ptr).lod_bias);
   debug_printf("\t\t.min_filter = 0x%x\n", (*ptr).min_filter);
   debug_printf("\t\t.mag_filter = 0x%x\n", (*ptr).mag_filter);
   debug_printf("\t\t.mip_filter = 0x%x\n", (*ptr).mip_filter);
   debug_printf("\t\t.base_level = 0x%x\n", (*ptr).base_level);
   debug_printf("\t\t.lod_preclamp = 0x%x\n", (*ptr).lod_preclamp);
   debug_printf("\t\t.default_color_mode = 0x%x\n", (*ptr).default_color_mode);
   debug_printf("\t\t.disable = 0x%x\n", (*ptr).disable);
}

void
brw_dump_ss1(const struct brw_ss1 *ptr)
{
   debug_printf("\t\t.r_wrap_mode = 0x%x\n", (*ptr).r_wrap_mode);
   debug_printf("\t\t.t_wrap_mode = 0x%x\n", (*ptr).t_wrap_mode);
   debug_printf("\t\t.s_wrap_mode = 0x%x\n", (*ptr).s_wrap_mode);
   debug_printf("\t\t.max_lod = 0x%x\n", (*ptr).max_lod);
   debug_printf("\t\t.min_lod = 0x%x\n", (*ptr).min_lod);
}

void
brw_dump_ss2(const struct brw_ss2 *ptr)
{
   debug_printf("\t\t.default_color_pointer = 0x%x\n", (*ptr).default_color_pointer);
}

void
brw_dump_ss3(const struct brw_ss3 *ptr)
{
   debug_printf("\t\t.max_aniso = 0x%x\n", (*ptr).max_aniso);
   debug_printf("\t\t.chroma_key_mode = 0x%x\n", (*ptr).chroma_key_mode);
   debug_printf("\t\t.chroma_key_index = 0x%x\n", (*ptr).chroma_key_index);
   debug_printf("\t\t.chroma_key_enable = 0x%x\n", (*ptr).chroma_key_enable);
   debug_printf("\t\t.monochrome_filter_width = 0x%x\n", (*ptr).monochrome_filter_width);
   debug_printf("\t\t.monochrome_filter_height = 0x%x\n", (*ptr).monochrome_filter_height);
}

void
brw_dump_state_base_address(const struct brw_state_base_address *ptr)
{
   debug_printf("\t\t.header.length = 0x%x\n", (*ptr).header.length);
   debug_printf("\t\t.header.opcode = 0x%x\n", (*ptr).header.opcode);
   debug_printf("\t\t.bits0.modify_enable = 0x%x\n", (*ptr).bits0.modify_enable);
   debug_printf("\t\t.bits0.general_state_address = 0x%x\n", (*ptr).bits0.general_state_address);
   debug_printf("\t\t.bits1.modify_enable = 0x%x\n", (*ptr).bits1.modify_enable);
   debug_printf("\t\t.bits1.surface_state_address = 0x%x\n", (*ptr).bits1.surface_state_address);
   debug_printf("\t\t.bits2.modify_enable = 0x%x\n", (*ptr).bits2.modify_enable);
   debug_printf("\t\t.bits2.indirect_object_state_address = 0x%x\n", (*ptr).bits2.indirect_object_state_address);
   debug_printf("\t\t.bits3.modify_enable = 0x%x\n", (*ptr).bits3.modify_enable);
   debug_printf("\t\t.bits3.general_state_upper_bound = 0x%x\n", (*ptr).bits3.general_state_upper_bound);
   debug_printf("\t\t.bits4.modify_enable = 0x%x\n", (*ptr).bits4.modify_enable);
   debug_printf("\t\t.bits4.indirect_object_state_upper_bound = 0x%x\n", (*ptr).bits4.indirect_object_state_upper_bound);
}

void
brw_dump_state_prefetch(const struct brw_state_prefetch *ptr)
{
   debug_printf("\t\t.header.length = 0x%x\n", (*ptr).header.length);
   debug_printf("\t\t.header.opcode = 0x%x\n", (*ptr).header.opcode);
   debug_printf("\t\t.bits0.prefetch_count = 0x%x\n", (*ptr).bits0.prefetch_count);
   debug_printf("\t\t.bits0.prefetch_pointer = 0x%x\n", (*ptr).bits0.prefetch_pointer);
}

void
brw_dump_surf_ss0(const struct brw_surf_ss0 *ptr)
{
   debug_printf("\t\t.cube_pos_z = 0x%x\n", (*ptr).cube_pos_z);
   debug_printf("\t\t.cube_neg_z = 0x%x\n", (*ptr).cube_neg_z);
   debug_printf("\t\t.cube_pos_y = 0x%x\n", (*ptr).cube_pos_y);
   debug_printf("\t\t.cube_neg_y = 0x%x\n", (*ptr).cube_neg_y);
   debug_printf("\t\t.cube_pos_x = 0x%x\n", (*ptr).cube_pos_x);
   debug_printf("\t\t.cube_neg_x = 0x%x\n", (*ptr).cube_neg_x);
   debug_printf("\t\t.mipmap_layout_mode = 0x%x\n", (*ptr).mipmap_layout_mode);
   debug_printf("\t\t.vert_line_stride_ofs = 0x%x\n", (*ptr).vert_line_stride_ofs);
   debug_printf("\t\t.vert_line_stride = 0x%x\n", (*ptr).vert_line_stride);
   debug_printf("\t\t.color_blend = 0x%x\n", (*ptr).color_blend);
   debug_printf("\t\t.writedisable_blue = 0x%x\n", (*ptr).writedisable_blue);
   debug_printf("\t\t.writedisable_green = 0x%x\n", (*ptr).writedisable_green);
   debug_printf("\t\t.writedisable_red = 0x%x\n", (*ptr).writedisable_red);
   debug_printf("\t\t.writedisable_alpha = 0x%x\n", (*ptr).writedisable_alpha);
   debug_printf("\t\t.surface_format = 0x%x\n", (*ptr).surface_format);
   debug_printf("\t\t.data_return_format = 0x%x\n", (*ptr).data_return_format);
   debug_printf("\t\t.surface_type = 0x%x\n", (*ptr).surface_type);
}

void
brw_dump_surf_ss1(const struct brw_surf_ss1 *ptr)
{
   debug_printf("\t\t.base_addr = 0x%x\n", (*ptr).base_addr);
}

void
brw_dump_surf_ss2(const struct brw_surf_ss2 *ptr)
{
   debug_printf("\t\t.mip_count = 0x%x\n", (*ptr).mip_count);
   debug_printf("\t\t.width = 0x%x\n", (*ptr).width);
   debug_printf("\t\t.height = 0x%x\n", (*ptr).height);
}

void
brw_dump_surf_ss3(const struct brw_surf_ss3 *ptr)
{
   debug_printf("\t\t.tile_walk = 0x%x\n", (*ptr).tile_walk);
   debug_printf("\t\t.tiled_surface = 0x%x\n", (*ptr).tiled_surface);
   debug_printf("\t\t.pitch = 0x%x\n", (*ptr).pitch);
   debug_printf("\t\t.depth = 0x%x\n", (*ptr).depth);
}

void
brw_dump_surf_ss4(const struct brw_surf_ss4 *ptr)
{
   debug_printf("\t\t.multisample_position_palette_index = 0x%x\n", (*ptr).multisample_position_palette_index);
   debug_printf("\t\t.num_multisamples = 0x%x\n", (*ptr).num_multisamples);
   debug_printf("\t\t.render_target_view_extent = 0x%x\n", (*ptr).render_target_view_extent);
   debug_printf("\t\t.min_array_elt = 0x%x\n", (*ptr).min_array_elt);
   debug_printf("\t\t.min_lod = 0x%x\n", (*ptr).min_lod);
}

void
brw_dump_surf_ss5(const struct brw_surf_ss5 *ptr)
{
   debug_printf("\t\t.llc_mapping = 0x%x\n", (*ptr).llc_mapping);
   debug_printf("\t\t.mlc_mapping = 0x%x\n", (*ptr).mlc_mapping);
   debug_printf("\t\t.gfdt = 0x%x\n", (*ptr).gfdt);
   debug_printf("\t\t.gfdt_src = 0x%x\n", (*ptr).gfdt_src);
   debug_printf("\t\t.y_offset = 0x%x\n", (*ptr).y_offset);
   debug_printf("\t\t.x_offset = 0x%x\n", (*ptr).x_offset);
}

void
brw_dump_surface_state(const struct brw_surface_state *ptr)
{
   debug_printf("\t\t.ss0.cube_pos_z = 0x%x\n", (*ptr).ss0.cube_pos_z);
   debug_printf("\t\t.ss0.cube_neg_z = 0x%x\n", (*ptr).ss0.cube_neg_z);
   debug_printf("\t\t.ss0.cube_pos_y = 0x%x\n", (*ptr).ss0.cube_pos_y);
   debug_printf("\t\t.ss0.cube_neg_y = 0x%x\n", (*ptr).ss0.cube_neg_y);
   debug_printf("\t\t.ss0.cube_pos_x = 0x%x\n", (*ptr).ss0.cube_pos_x);
   debug_printf("\t\t.ss0.cube_neg_x = 0x%x\n", (*ptr).ss0.cube_neg_x);
   debug_printf("\t\t.ss0.mipmap_layout_mode = 0x%x\n", (*ptr).ss0.mipmap_layout_mode);
   debug_printf("\t\t.ss0.vert_line_stride_ofs = 0x%x\n", (*ptr).ss0.vert_line_stride_ofs);
   debug_printf("\t\t.ss0.vert_line_stride = 0x%x\n", (*ptr).ss0.vert_line_stride);
   debug_printf("\t\t.ss0.color_blend = 0x%x\n", (*ptr).ss0.color_blend);
   debug_printf("\t\t.ss0.writedisable_blue = 0x%x\n", (*ptr).ss0.writedisable_blue);
   debug_printf("\t\t.ss0.writedisable_green = 0x%x\n", (*ptr).ss0.writedisable_green);
   debug_printf("\t\t.ss0.writedisable_red = 0x%x\n", (*ptr).ss0.writedisable_red);
   debug_printf("\t\t.ss0.writedisable_alpha = 0x%x\n", (*ptr).ss0.writedisable_alpha);
   debug_printf("\t\t.ss0.surface_format = 0x%x\n", (*ptr).ss0.surface_format);
   debug_printf("\t\t.ss0.data_return_format = 0x%x\n", (*ptr).ss0.data_return_format);
   debug_printf("\t\t.ss0.surface_type = 0x%x\n", (*ptr).ss0.surface_type);
   debug_printf("\t\t.ss1.base_addr = 0x%x\n", (*ptr).ss1.base_addr);
   debug_printf("\t\t.ss2.mip_count = 0x%x\n", (*ptr).ss2.mip_count);
   debug_printf("\t\t.ss2.width = 0x%x\n", (*ptr).ss2.width);
   debug_printf("\t\t.ss2.height = 0x%x\n", (*ptr).ss2.height);
   debug_printf("\t\t.ss3.tile_walk = 0x%x\n", (*ptr).ss3.tile_walk);
   debug_printf("\t\t.ss3.tiled_surface = 0x%x\n", (*ptr).ss3.tiled_surface);
   debug_printf("\t\t.ss3.pitch = 0x%x\n", (*ptr).ss3.pitch);
   debug_printf("\t\t.ss3.depth = 0x%x\n", (*ptr).ss3.depth);
   debug_printf("\t\t.ss4.multisample_position_palette_index = 0x%x\n", (*ptr).ss4.multisample_position_palette_index);
   debug_printf("\t\t.ss4.num_multisamples = 0x%x\n", (*ptr).ss4.num_multisamples);
   debug_printf("\t\t.ss4.render_target_view_extent = 0x%x\n", (*ptr).ss4.render_target_view_extent);
   debug_printf("\t\t.ss4.min_array_elt = 0x%x\n", (*ptr).ss4.min_array_elt);
   debug_printf("\t\t.ss4.min_lod = 0x%x\n", (*ptr).ss4.min_lod);
   debug_printf("\t\t.ss5.llc_mapping = 0x%x\n", (*ptr).ss5.llc_mapping);
   debug_printf("\t\t.ss5.mlc_mapping = 0x%x\n", (*ptr).ss5.mlc_mapping);
   debug_printf("\t\t.ss5.gfdt = 0x%x\n", (*ptr).ss5.gfdt);
   debug_printf("\t\t.ss5.gfdt_src = 0x%x\n", (*ptr).ss5.gfdt_src);
   debug_printf("\t\t.ss5.y_offset = 0x%x\n", (*ptr).ss5.y_offset);
   debug_printf("\t\t.ss5.x_offset = 0x%x\n", (*ptr).ss5.x_offset);
}

void
brw_dump_system_instruction_pointer(const struct brw_system_instruction_pointer *ptr)
{
   debug_printf("\t\t.header.length = 0x%x\n", (*ptr).header.length);
   debug_printf("\t\t.header.opcode = 0x%x\n", (*ptr).header.opcode);
   debug_printf("\t\t.bits0.system_instruction_pointer = 0x%x\n", (*ptr).bits0.system_instruction_pointer);
}

void
brw_dump_urb_fence(const struct brw_urb_fence *ptr)
{
   debug_printf("\t\t.header.length = 0x%x\n", (*ptr).header.length);
   debug_printf("\t\t.header.vs_realloc = 0x%x\n", (*ptr).header.vs_realloc);
   debug_printf("\t\t.header.gs_realloc = 0x%x\n", (*ptr).header.gs_realloc);
   debug_printf("\t\t.header.clp_realloc = 0x%x\n", (*ptr).header.clp_realloc);
   debug_printf("\t\t.header.sf_realloc = 0x%x\n", (*ptr).header.sf_realloc);
   debug_printf("\t\t.header.vfe_realloc = 0x%x\n", (*ptr).header.vfe_realloc);
   debug_printf("\t\t.header.cs_realloc = 0x%x\n", (*ptr).header.cs_realloc);
   debug_printf("\t\t.header.opcode = 0x%x\n", (*ptr).header.opcode);
   debug_printf("\t\t.bits0.vs_fence = 0x%x\n", (*ptr).bits0.vs_fence);
   debug_printf("\t\t.bits0.gs_fence = 0x%x\n", (*ptr).bits0.gs_fence);
   debug_printf("\t\t.bits0.clp_fence = 0x%x\n", (*ptr).bits0.clp_fence);
   debug_printf("\t\t.bits1.sf_fence = 0x%x\n", (*ptr).bits1.sf_fence);
   debug_printf("\t\t.bits1.vf_fence = 0x%x\n", (*ptr).bits1.vf_fence);
   debug_printf("\t\t.bits1.cs_fence = 0x%x\n", (*ptr).bits1.cs_fence);
}

void
brw_dump_urb_immediate(const struct brw_urb_immediate *ptr)
{
   debug_printf("\t\t.opcode = 0x%x\n", (*ptr).opcode);
   debug_printf("\t\t.offset = 0x%x\n", (*ptr).offset);
   debug_printf("\t\t.swizzle_control = 0x%x\n", (*ptr).swizzle_control);
   debug_printf("\t\t.allocate = 0x%x\n", (*ptr).allocate);
   debug_printf("\t\t.used = 0x%x\n", (*ptr).used);
   debug_printf("\t\t.complete = 0x%x\n", (*ptr).complete);
   debug_printf("\t\t.response_length = 0x%x\n", (*ptr).response_length);
   debug_printf("\t\t.msg_length = 0x%x\n", (*ptr).msg_length);
   debug_printf("\t\t.msg_target = 0x%x\n", (*ptr).msg_target);
   debug_printf("\t\t.end_of_thread = 0x%x\n", (*ptr).end_of_thread);
}

void
brw_dump_vb_array_state(const struct brw_vb_array_state *ptr)
{
   debug_printf("\t\t.header.length = 0x%x\n", (*ptr).header.length);
   debug_printf("\t\t.header.opcode = 0x%x\n", (*ptr).header.opcode);
   debug_printf("\t\t.vb[0].vb0.pitch = 0x%x\n", (*ptr).vb[0].vb0.pitch);
   debug_printf("\t\t.vb[0].vb0.access_type = 0x%x\n", (*ptr).vb[0].vb0.access_type);
   debug_printf("\t\t.vb[0].vb0.vb_index = 0x%x\n", (*ptr).vb[0].vb0.vb_index);
   debug_printf("\t\t.vb[0].start_addr = 0x%x\n", (*ptr).vb[0].start_addr);
   debug_printf("\t\t.vb[0].max_index = 0x%x\n", (*ptr).vb[0].max_index);
   debug_printf("\t\t.vb[0].instance_data_step_rate = 0x%x\n", (*ptr).vb[0].instance_data_step_rate);
   debug_printf("\t\t.vb[1].vb0.pitch = 0x%x\n", (*ptr).vb[1].vb0.pitch);
   debug_printf("\t\t.vb[1].vb0.access_type = 0x%x\n", (*ptr).vb[1].vb0.access_type);
   debug_printf("\t\t.vb[1].vb0.vb_index = 0x%x\n", (*ptr).vb[1].vb0.vb_index);
   debug_printf("\t\t.vb[1].start_addr = 0x%x\n", (*ptr).vb[1].start_addr);
   debug_printf("\t\t.vb[1].max_index = 0x%x\n", (*ptr).vb[1].max_index);
   debug_printf("\t\t.vb[1].instance_data_step_rate = 0x%x\n", (*ptr).vb[1].instance_data_step_rate);
   debug_printf("\t\t.vb[2].vb0.pitch = 0x%x\n", (*ptr).vb[2].vb0.pitch);
   debug_printf("\t\t.vb[2].vb0.access_type = 0x%x\n", (*ptr).vb[2].vb0.access_type);
   debug_printf("\t\t.vb[2].vb0.vb_index = 0x%x\n", (*ptr).vb[2].vb0.vb_index);
   debug_printf("\t\t.vb[2].start_addr = 0x%x\n", (*ptr).vb[2].start_addr);
   debug_printf("\t\t.vb[2].max_index = 0x%x\n", (*ptr).vb[2].max_index);
   debug_printf("\t\t.vb[2].instance_data_step_rate = 0x%x\n", (*ptr).vb[2].instance_data_step_rate);
   debug_printf("\t\t.vb[3].vb0.pitch = 0x%x\n", (*ptr).vb[3].vb0.pitch);
   debug_printf("\t\t.vb[3].vb0.access_type = 0x%x\n", (*ptr).vb[3].vb0.access_type);
   debug_printf("\t\t.vb[3].vb0.vb_index = 0x%x\n", (*ptr).vb[3].vb0.vb_index);
   debug_printf("\t\t.vb[3].start_addr = 0x%x\n", (*ptr).vb[3].start_addr);
   debug_printf("\t\t.vb[3].max_index = 0x%x\n", (*ptr).vb[3].max_index);
   debug_printf("\t\t.vb[3].instance_data_step_rate = 0x%x\n", (*ptr).vb[3].instance_data_step_rate);
   debug_printf("\t\t.vb[4].vb0.pitch = 0x%x\n", (*ptr).vb[4].vb0.pitch);
   debug_printf("\t\t.vb[4].vb0.access_type = 0x%x\n", (*ptr).vb[4].vb0.access_type);
   debug_printf("\t\t.vb[4].vb0.vb_index = 0x%x\n", (*ptr).vb[4].vb0.vb_index);
   debug_printf("\t\t.vb[4].start_addr = 0x%x\n", (*ptr).vb[4].start_addr);
   debug_printf("\t\t.vb[4].max_index = 0x%x\n", (*ptr).vb[4].max_index);
   debug_printf("\t\t.vb[4].instance_data_step_rate = 0x%x\n", (*ptr).vb[4].instance_data_step_rate);
   debug_printf("\t\t.vb[5].vb0.pitch = 0x%x\n", (*ptr).vb[5].vb0.pitch);
   debug_printf("\t\t.vb[5].vb0.access_type = 0x%x\n", (*ptr).vb[5].vb0.access_type);
   debug_printf("\t\t.vb[5].vb0.vb_index = 0x%x\n", (*ptr).vb[5].vb0.vb_index);
   debug_printf("\t\t.vb[5].start_addr = 0x%x\n", (*ptr).vb[5].start_addr);
   debug_printf("\t\t.vb[5].max_index = 0x%x\n", (*ptr).vb[5].max_index);
   debug_printf("\t\t.vb[5].instance_data_step_rate = 0x%x\n", (*ptr).vb[5].instance_data_step_rate);
   debug_printf("\t\t.vb[6].vb0.pitch = 0x%x\n", (*ptr).vb[6].vb0.pitch);
   debug_printf("\t\t.vb[6].vb0.access_type = 0x%x\n", (*ptr).vb[6].vb0.access_type);
   debug_printf("\t\t.vb[6].vb0.vb_index = 0x%x\n", (*ptr).vb[6].vb0.vb_index);
   debug_printf("\t\t.vb[6].start_addr = 0x%x\n", (*ptr).vb[6].start_addr);
   debug_printf("\t\t.vb[6].max_index = 0x%x\n", (*ptr).vb[6].max_index);
   debug_printf("\t\t.vb[6].instance_data_step_rate = 0x%x\n", (*ptr).vb[6].instance_data_step_rate);
   debug_printf("\t\t.vb[7].vb0.pitch = 0x%x\n", (*ptr).vb[7].vb0.pitch);
   debug_printf("\t\t.vb[7].vb0.access_type = 0x%x\n", (*ptr).vb[7].vb0.access_type);
   debug_printf("\t\t.vb[7].vb0.vb_index = 0x%x\n", (*ptr).vb[7].vb0.vb_index);
   debug_printf("\t\t.vb[7].start_addr = 0x%x\n", (*ptr).vb[7].start_addr);
   debug_printf("\t\t.vb[7].max_index = 0x%x\n", (*ptr).vb[7].max_index);
   debug_printf("\t\t.vb[7].instance_data_step_rate = 0x%x\n", (*ptr).vb[7].instance_data_step_rate);
   debug_printf("\t\t.vb[8].vb0.pitch = 0x%x\n", (*ptr).vb[8].vb0.pitch);
   debug_printf("\t\t.vb[8].vb0.access_type = 0x%x\n", (*ptr).vb[8].vb0.access_type);
   debug_printf("\t\t.vb[8].vb0.vb_index = 0x%x\n", (*ptr).vb[8].vb0.vb_index);
   debug_printf("\t\t.vb[8].start_addr = 0x%x\n", (*ptr).vb[8].start_addr);
   debug_printf("\t\t.vb[8].max_index = 0x%x\n", (*ptr).vb[8].max_index);
   debug_printf("\t\t.vb[8].instance_data_step_rate = 0x%x\n", (*ptr).vb[8].instance_data_step_rate);
   debug_printf("\t\t.vb[9].vb0.pitch = 0x%x\n", (*ptr).vb[9].vb0.pitch);
   debug_printf("\t\t.vb[9].vb0.access_type = 0x%x\n", (*ptr).vb[9].vb0.access_type);
   debug_printf("\t\t.vb[9].vb0.vb_index = 0x%x\n", (*ptr).vb[9].vb0.vb_index);
   debug_printf("\t\t.vb[9].start_addr = 0x%x\n", (*ptr).vb[9].start_addr);
   debug_printf("\t\t.vb[9].max_index = 0x%x\n", (*ptr).vb[9].max_index);
   debug_printf("\t\t.vb[9].instance_data_step_rate = 0x%x\n", (*ptr).vb[9].instance_data_step_rate);
   debug_printf("\t\t.vb[10].vb0.pitch = 0x%x\n", (*ptr).vb[10].vb0.pitch);
   debug_printf("\t\t.vb[10].vb0.access_type = 0x%x\n", (*ptr).vb[10].vb0.access_type);
   debug_printf("\t\t.vb[10].vb0.vb_index = 0x%x\n", (*ptr).vb[10].vb0.vb_index);
   debug_printf("\t\t.vb[10].start_addr = 0x%x\n", (*ptr).vb[10].start_addr);
   debug_printf("\t\t.vb[10].max_index = 0x%x\n", (*ptr).vb[10].max_index);
   debug_printf("\t\t.vb[10].instance_data_step_rate = 0x%x\n", (*ptr).vb[10].instance_data_step_rate);
   debug_printf("\t\t.vb[11].vb0.pitch = 0x%x\n", (*ptr).vb[11].vb0.pitch);
   debug_printf("\t\t.vb[11].vb0.access_type = 0x%x\n", (*ptr).vb[11].vb0.access_type);
   debug_printf("\t\t.vb[11].vb0.vb_index = 0x%x\n", (*ptr).vb[11].vb0.vb_index);
   debug_printf("\t\t.vb[11].start_addr = 0x%x\n", (*ptr).vb[11].start_addr);
   debug_printf("\t\t.vb[11].max_index = 0x%x\n", (*ptr).vb[11].max_index);
   debug_printf("\t\t.vb[11].instance_data_step_rate = 0x%x\n", (*ptr).vb[11].instance_data_step_rate);
   debug_printf("\t\t.vb[12].vb0.pitch = 0x%x\n", (*ptr).vb[12].vb0.pitch);
   debug_printf("\t\t.vb[12].vb0.access_type = 0x%x\n", (*ptr).vb[12].vb0.access_type);
   debug_printf("\t\t.vb[12].vb0.vb_index = 0x%x\n", (*ptr).vb[12].vb0.vb_index);
   debug_printf("\t\t.vb[12].start_addr = 0x%x\n", (*ptr).vb[12].start_addr);
   debug_printf("\t\t.vb[12].max_index = 0x%x\n", (*ptr).vb[12].max_index);
   debug_printf("\t\t.vb[12].instance_data_step_rate = 0x%x\n", (*ptr).vb[12].instance_data_step_rate);
   debug_printf("\t\t.vb[13].vb0.pitch = 0x%x\n", (*ptr).vb[13].vb0.pitch);
   debug_printf("\t\t.vb[13].vb0.access_type = 0x%x\n", (*ptr).vb[13].vb0.access_type);
   debug_printf("\t\t.vb[13].vb0.vb_index = 0x%x\n", (*ptr).vb[13].vb0.vb_index);
   debug_printf("\t\t.vb[13].start_addr = 0x%x\n", (*ptr).vb[13].start_addr);
   debug_printf("\t\t.vb[13].max_index = 0x%x\n", (*ptr).vb[13].max_index);
   debug_printf("\t\t.vb[13].instance_data_step_rate = 0x%x\n", (*ptr).vb[13].instance_data_step_rate);
   debug_printf("\t\t.vb[14].vb0.pitch = 0x%x\n", (*ptr).vb[14].vb0.pitch);
   debug_printf("\t\t.vb[14].vb0.access_type = 0x%x\n", (*ptr).vb[14].vb0.access_type);
   debug_printf("\t\t.vb[14].vb0.vb_index = 0x%x\n", (*ptr).vb[14].vb0.vb_index);
   debug_printf("\t\t.vb[14].start_addr = 0x%x\n", (*ptr).vb[14].start_addr);
   debug_printf("\t\t.vb[14].max_index = 0x%x\n", (*ptr).vb[14].max_index);
   debug_printf("\t\t.vb[14].instance_data_step_rate = 0x%x\n", (*ptr).vb[14].instance_data_step_rate);
   debug_printf("\t\t.vb[15].vb0.pitch = 0x%x\n", (*ptr).vb[15].vb0.pitch);
   debug_printf("\t\t.vb[15].vb0.access_type = 0x%x\n", (*ptr).vb[15].vb0.access_type);
   debug_printf("\t\t.vb[15].vb0.vb_index = 0x%x\n", (*ptr).vb[15].vb0.vb_index);
   debug_printf("\t\t.vb[15].start_addr = 0x%x\n", (*ptr).vb[15].start_addr);
   debug_printf("\t\t.vb[15].max_index = 0x%x\n", (*ptr).vb[15].max_index);
   debug_printf("\t\t.vb[15].instance_data_step_rate = 0x%x\n", (*ptr).vb[15].instance_data_step_rate);
   debug_printf("\t\t.vb[16].vb0.pitch = 0x%x\n", (*ptr).vb[16].vb0.pitch);
   debug_printf("\t\t.vb[16].vb0.access_type = 0x%x\n", (*ptr).vb[16].vb0.access_type);
   debug_printf("\t\t.vb[16].vb0.vb_index = 0x%x\n", (*ptr).vb[16].vb0.vb_index);
   debug_printf("\t\t.vb[16].start_addr = 0x%x\n", (*ptr).vb[16].start_addr);
   debug_printf("\t\t.vb[16].max_index = 0x%x\n", (*ptr).vb[16].max_index);
   debug_printf("\t\t.vb[16].instance_data_step_rate = 0x%x\n", (*ptr).vb[16].instance_data_step_rate);
}

void
brw_dump_vertex_buffer_state(const struct brw_vertex_buffer_state *ptr)
{
   debug_printf("\t\t.vb0.pitch = 0x%x\n", (*ptr).vb0.pitch);
   debug_printf("\t\t.vb0.access_type = 0x%x\n", (*ptr).vb0.access_type);
   debug_printf("\t\t.vb0.vb_index = 0x%x\n", (*ptr).vb0.vb_index);
   debug_printf("\t\t.start_addr = 0x%x\n", (*ptr).start_addr);
   debug_printf("\t\t.max_index = 0x%x\n", (*ptr).max_index);
   debug_printf("\t\t.instance_data_step_rate = 0x%x\n", (*ptr).instance_data_step_rate);
}

void
brw_dump_vertex_element_packet(const struct brw_vertex_element_packet *ptr)
{
   debug_printf("\t\t.header.length = 0x%x\n", (*ptr).header.length);
   debug_printf("\t\t.header.opcode = 0x%x\n", (*ptr).header.opcode);
   debug_printf("\t\t.ve[0].ve0.src_offset = 0x%x\n", (*ptr).ve[0].ve0.src_offset);
   debug_printf("\t\t.ve[0].ve0.src_format = 0x%x\n", (*ptr).ve[0].ve0.src_format);
   debug_printf("\t\t.ve[0].ve0.valid = 0x%x\n", (*ptr).ve[0].ve0.valid);
   debug_printf("\t\t.ve[0].ve0.vertex_buffer_index = 0x%x\n", (*ptr).ve[0].ve0.vertex_buffer_index);
   debug_printf("\t\t.ve[0].ve1.dst_offset = 0x%x\n", (*ptr).ve[0].ve1.dst_offset);
   debug_printf("\t\t.ve[0].ve1.vfcomponent3 = 0x%x\n", (*ptr).ve[0].ve1.vfcomponent3);
   debug_printf("\t\t.ve[0].ve1.vfcomponent2 = 0x%x\n", (*ptr).ve[0].ve1.vfcomponent2);
   debug_printf("\t\t.ve[0].ve1.vfcomponent1 = 0x%x\n", (*ptr).ve[0].ve1.vfcomponent1);
   debug_printf("\t\t.ve[0].ve1.vfcomponent0 = 0x%x\n", (*ptr).ve[0].ve1.vfcomponent0);
   debug_printf("\t\t.ve[1].ve0.src_offset = 0x%x\n", (*ptr).ve[1].ve0.src_offset);
   debug_printf("\t\t.ve[1].ve0.src_format = 0x%x\n", (*ptr).ve[1].ve0.src_format);
   debug_printf("\t\t.ve[1].ve0.valid = 0x%x\n", (*ptr).ve[1].ve0.valid);
   debug_printf("\t\t.ve[1].ve0.vertex_buffer_index = 0x%x\n", (*ptr).ve[1].ve0.vertex_buffer_index);
   debug_printf("\t\t.ve[1].ve1.dst_offset = 0x%x\n", (*ptr).ve[1].ve1.dst_offset);
   debug_printf("\t\t.ve[1].ve1.vfcomponent3 = 0x%x\n", (*ptr).ve[1].ve1.vfcomponent3);
   debug_printf("\t\t.ve[1].ve1.vfcomponent2 = 0x%x\n", (*ptr).ve[1].ve1.vfcomponent2);
   debug_printf("\t\t.ve[1].ve1.vfcomponent1 = 0x%x\n", (*ptr).ve[1].ve1.vfcomponent1);
   debug_printf("\t\t.ve[1].ve1.vfcomponent0 = 0x%x\n", (*ptr).ve[1].ve1.vfcomponent0);
   debug_printf("\t\t.ve[2].ve0.src_offset = 0x%x\n", (*ptr).ve[2].ve0.src_offset);
   debug_printf("\t\t.ve[2].ve0.src_format = 0x%x\n", (*ptr).ve[2].ve0.src_format);
   debug_printf("\t\t.ve[2].ve0.valid = 0x%x\n", (*ptr).ve[2].ve0.valid);
   debug_printf("\t\t.ve[2].ve0.vertex_buffer_index = 0x%x\n", (*ptr).ve[2].ve0.vertex_buffer_index);
   debug_printf("\t\t.ve[2].ve1.dst_offset = 0x%x\n", (*ptr).ve[2].ve1.dst_offset);
   debug_printf("\t\t.ve[2].ve1.vfcomponent3 = 0x%x\n", (*ptr).ve[2].ve1.vfcomponent3);
   debug_printf("\t\t.ve[2].ve1.vfcomponent2 = 0x%x\n", (*ptr).ve[2].ve1.vfcomponent2);
   debug_printf("\t\t.ve[2].ve1.vfcomponent1 = 0x%x\n", (*ptr).ve[2].ve1.vfcomponent1);
   debug_printf("\t\t.ve[2].ve1.vfcomponent0 = 0x%x\n", (*ptr).ve[2].ve1.vfcomponent0);
   debug_printf("\t\t.ve[3].ve0.src_offset = 0x%x\n", (*ptr).ve[3].ve0.src_offset);
   debug_printf("\t\t.ve[3].ve0.src_format = 0x%x\n", (*ptr).ve[3].ve0.src_format);
   debug_printf("\t\t.ve[3].ve0.valid = 0x%x\n", (*ptr).ve[3].ve0.valid);
   debug_printf("\t\t.ve[3].ve0.vertex_buffer_index = 0x%x\n", (*ptr).ve[3].ve0.vertex_buffer_index);
   debug_printf("\t\t.ve[3].ve1.dst_offset = 0x%x\n", (*ptr).ve[3].ve1.dst_offset);
   debug_printf("\t\t.ve[3].ve1.vfcomponent3 = 0x%x\n", (*ptr).ve[3].ve1.vfcomponent3);
   debug_printf("\t\t.ve[3].ve1.vfcomponent2 = 0x%x\n", (*ptr).ve[3].ve1.vfcomponent2);
   debug_printf("\t\t.ve[3].ve1.vfcomponent1 = 0x%x\n", (*ptr).ve[3].ve1.vfcomponent1);
   debug_printf("\t\t.ve[3].ve1.vfcomponent0 = 0x%x\n", (*ptr).ve[3].ve1.vfcomponent0);
   debug_printf("\t\t.ve[4].ve0.src_offset = 0x%x\n", (*ptr).ve[4].ve0.src_offset);
   debug_printf("\t\t.ve[4].ve0.src_format = 0x%x\n", (*ptr).ve[4].ve0.src_format);
   debug_printf("\t\t.ve[4].ve0.valid = 0x%x\n", (*ptr).ve[4].ve0.valid);
   debug_printf("\t\t.ve[4].ve0.vertex_buffer_index = 0x%x\n", (*ptr).ve[4].ve0.vertex_buffer_index);
   debug_printf("\t\t.ve[4].ve1.dst_offset = 0x%x\n", (*ptr).ve[4].ve1.dst_offset);
   debug_printf("\t\t.ve[4].ve1.vfcomponent3 = 0x%x\n", (*ptr).ve[4].ve1.vfcomponent3);
   debug_printf("\t\t.ve[4].ve1.vfcomponent2 = 0x%x\n", (*ptr).ve[4].ve1.vfcomponent2);
   debug_printf("\t\t.ve[4].ve1.vfcomponent1 = 0x%x\n", (*ptr).ve[4].ve1.vfcomponent1);
   debug_printf("\t\t.ve[4].ve1.vfcomponent0 = 0x%x\n", (*ptr).ve[4].ve1.vfcomponent0);
   debug_printf("\t\t.ve[5].ve0.src_offset = 0x%x\n", (*ptr).ve[5].ve0.src_offset);
   debug_printf("\t\t.ve[5].ve0.src_format = 0x%x\n", (*ptr).ve[5].ve0.src_format);
   debug_printf("\t\t.ve[5].ve0.valid = 0x%x\n", (*ptr).ve[5].ve0.valid);
   debug_printf("\t\t.ve[5].ve0.vertex_buffer_index = 0x%x\n", (*ptr).ve[5].ve0.vertex_buffer_index);
   debug_printf("\t\t.ve[5].ve1.dst_offset = 0x%x\n", (*ptr).ve[5].ve1.dst_offset);
   debug_printf("\t\t.ve[5].ve1.vfcomponent3 = 0x%x\n", (*ptr).ve[5].ve1.vfcomponent3);
   debug_printf("\t\t.ve[5].ve1.vfcomponent2 = 0x%x\n", (*ptr).ve[5].ve1.vfcomponent2);
   debug_printf("\t\t.ve[5].ve1.vfcomponent1 = 0x%x\n", (*ptr).ve[5].ve1.vfcomponent1);
   debug_printf("\t\t.ve[5].ve1.vfcomponent0 = 0x%x\n", (*ptr).ve[5].ve1.vfcomponent0);
   debug_printf("\t\t.ve[6].ve0.src_offset = 0x%x\n", (*ptr).ve[6].ve0.src_offset);
   debug_printf("\t\t.ve[6].ve0.src_format = 0x%x\n", (*ptr).ve[6].ve0.src_format);
   debug_printf("\t\t.ve[6].ve0.valid = 0x%x\n", (*ptr).ve[6].ve0.valid);
   debug_printf("\t\t.ve[6].ve0.vertex_buffer_index = 0x%x\n", (*ptr).ve[6].ve0.vertex_buffer_index);
   debug_printf("\t\t.ve[6].ve1.dst_offset = 0x%x\n", (*ptr).ve[6].ve1.dst_offset);
   debug_printf("\t\t.ve[6].ve1.vfcomponent3 = 0x%x\n", (*ptr).ve[6].ve1.vfcomponent3);
   debug_printf("\t\t.ve[6].ve1.vfcomponent2 = 0x%x\n", (*ptr).ve[6].ve1.vfcomponent2);
   debug_printf("\t\t.ve[6].ve1.vfcomponent1 = 0x%x\n", (*ptr).ve[6].ve1.vfcomponent1);
   debug_printf("\t\t.ve[6].ve1.vfcomponent0 = 0x%x\n", (*ptr).ve[6].ve1.vfcomponent0);
   debug_printf("\t\t.ve[7].ve0.src_offset = 0x%x\n", (*ptr).ve[7].ve0.src_offset);
   debug_printf("\t\t.ve[7].ve0.src_format = 0x%x\n", (*ptr).ve[7].ve0.src_format);
   debug_printf("\t\t.ve[7].ve0.valid = 0x%x\n", (*ptr).ve[7].ve0.valid);
   debug_printf("\t\t.ve[7].ve0.vertex_buffer_index = 0x%x\n", (*ptr).ve[7].ve0.vertex_buffer_index);
   debug_printf("\t\t.ve[7].ve1.dst_offset = 0x%x\n", (*ptr).ve[7].ve1.dst_offset);
   debug_printf("\t\t.ve[7].ve1.vfcomponent3 = 0x%x\n", (*ptr).ve[7].ve1.vfcomponent3);
   debug_printf("\t\t.ve[7].ve1.vfcomponent2 = 0x%x\n", (*ptr).ve[7].ve1.vfcomponent2);
   debug_printf("\t\t.ve[7].ve1.vfcomponent1 = 0x%x\n", (*ptr).ve[7].ve1.vfcomponent1);
   debug_printf("\t\t.ve[7].ve1.vfcomponent0 = 0x%x\n", (*ptr).ve[7].ve1.vfcomponent0);
   debug_printf("\t\t.ve[8].ve0.src_offset = 0x%x\n", (*ptr).ve[8].ve0.src_offset);
   debug_printf("\t\t.ve[8].ve0.src_format = 0x%x\n", (*ptr).ve[8].ve0.src_format);
   debug_printf("\t\t.ve[8].ve0.valid = 0x%x\n", (*ptr).ve[8].ve0.valid);
   debug_printf("\t\t.ve[8].ve0.vertex_buffer_index = 0x%x\n", (*ptr).ve[8].ve0.vertex_buffer_index);
   debug_printf("\t\t.ve[8].ve1.dst_offset = 0x%x\n", (*ptr).ve[8].ve1.dst_offset);
   debug_printf("\t\t.ve[8].ve1.vfcomponent3 = 0x%x\n", (*ptr).ve[8].ve1.vfcomponent3);
   debug_printf("\t\t.ve[8].ve1.vfcomponent2 = 0x%x\n", (*ptr).ve[8].ve1.vfcomponent2);
   debug_printf("\t\t.ve[8].ve1.vfcomponent1 = 0x%x\n", (*ptr).ve[8].ve1.vfcomponent1);
   debug_printf("\t\t.ve[8].ve1.vfcomponent0 = 0x%x\n", (*ptr).ve[8].ve1.vfcomponent0);
   debug_printf("\t\t.ve[9].ve0.src_offset = 0x%x\n", (*ptr).ve[9].ve0.src_offset);
   debug_printf("\t\t.ve[9].ve0.src_format = 0x%x\n", (*ptr).ve[9].ve0.src_format);
   debug_printf("\t\t.ve[9].ve0.valid = 0x%x\n", (*ptr).ve[9].ve0.valid);
   debug_printf("\t\t.ve[9].ve0.vertex_buffer_index = 0x%x\n", (*ptr).ve[9].ve0.vertex_buffer_index);
   debug_printf("\t\t.ve[9].ve1.dst_offset = 0x%x\n", (*ptr).ve[9].ve1.dst_offset);
   debug_printf("\t\t.ve[9].ve1.vfcomponent3 = 0x%x\n", (*ptr).ve[9].ve1.vfcomponent3);
   debug_printf("\t\t.ve[9].ve1.vfcomponent2 = 0x%x\n", (*ptr).ve[9].ve1.vfcomponent2);
   debug_printf("\t\t.ve[9].ve1.vfcomponent1 = 0x%x\n", (*ptr).ve[9].ve1.vfcomponent1);
   debug_printf("\t\t.ve[9].ve1.vfcomponent0 = 0x%x\n", (*ptr).ve[9].ve1.vfcomponent0);
   debug_printf("\t\t.ve[10].ve0.src_offset = 0x%x\n", (*ptr).ve[10].ve0.src_offset);
   debug_printf("\t\t.ve[10].ve0.src_format = 0x%x\n", (*ptr).ve[10].ve0.src_format);
   debug_printf("\t\t.ve[10].ve0.valid = 0x%x\n", (*ptr).ve[10].ve0.valid);
   debug_printf("\t\t.ve[10].ve0.vertex_buffer_index = 0x%x\n", (*ptr).ve[10].ve0.vertex_buffer_index);
   debug_printf("\t\t.ve[10].ve1.dst_offset = 0x%x\n", (*ptr).ve[10].ve1.dst_offset);
   debug_printf("\t\t.ve[10].ve1.vfcomponent3 = 0x%x\n", (*ptr).ve[10].ve1.vfcomponent3);
   debug_printf("\t\t.ve[10].ve1.vfcomponent2 = 0x%x\n", (*ptr).ve[10].ve1.vfcomponent2);
   debug_printf("\t\t.ve[10].ve1.vfcomponent1 = 0x%x\n", (*ptr).ve[10].ve1.vfcomponent1);
   debug_printf("\t\t.ve[10].ve1.vfcomponent0 = 0x%x\n", (*ptr).ve[10].ve1.vfcomponent0);
   debug_printf("\t\t.ve[11].ve0.src_offset = 0x%x\n", (*ptr).ve[11].ve0.src_offset);
   debug_printf("\t\t.ve[11].ve0.src_format = 0x%x\n", (*ptr).ve[11].ve0.src_format);
   debug_printf("\t\t.ve[11].ve0.valid = 0x%x\n", (*ptr).ve[11].ve0.valid);
   debug_printf("\t\t.ve[11].ve0.vertex_buffer_index = 0x%x\n", (*ptr).ve[11].ve0.vertex_buffer_index);
   debug_printf("\t\t.ve[11].ve1.dst_offset = 0x%x\n", (*ptr).ve[11].ve1.dst_offset);
   debug_printf("\t\t.ve[11].ve1.vfcomponent3 = 0x%x\n", (*ptr).ve[11].ve1.vfcomponent3);
   debug_printf("\t\t.ve[11].ve1.vfcomponent2 = 0x%x\n", (*ptr).ve[11].ve1.vfcomponent2);
   debug_printf("\t\t.ve[11].ve1.vfcomponent1 = 0x%x\n", (*ptr).ve[11].ve1.vfcomponent1);
   debug_printf("\t\t.ve[11].ve1.vfcomponent0 = 0x%x\n", (*ptr).ve[11].ve1.vfcomponent0);
   debug_printf("\t\t.ve[12].ve0.src_offset = 0x%x\n", (*ptr).ve[12].ve0.src_offset);
   debug_printf("\t\t.ve[12].ve0.src_format = 0x%x\n", (*ptr).ve[12].ve0.src_format);
   debug_printf("\t\t.ve[12].ve0.valid = 0x%x\n", (*ptr).ve[12].ve0.valid);
   debug_printf("\t\t.ve[12].ve0.vertex_buffer_index = 0x%x\n", (*ptr).ve[12].ve0.vertex_buffer_index);
   debug_printf("\t\t.ve[12].ve1.dst_offset = 0x%x\n", (*ptr).ve[12].ve1.dst_offset);
   debug_printf("\t\t.ve[12].ve1.vfcomponent3 = 0x%x\n", (*ptr).ve[12].ve1.vfcomponent3);
   debug_printf("\t\t.ve[12].ve1.vfcomponent2 = 0x%x\n", (*ptr).ve[12].ve1.vfcomponent2);
   debug_printf("\t\t.ve[12].ve1.vfcomponent1 = 0x%x\n", (*ptr).ve[12].ve1.vfcomponent1);
   debug_printf("\t\t.ve[12].ve1.vfcomponent0 = 0x%x\n", (*ptr).ve[12].ve1.vfcomponent0);
   debug_printf("\t\t.ve[13].ve0.src_offset = 0x%x\n", (*ptr).ve[13].ve0.src_offset);
   debug_printf("\t\t.ve[13].ve0.src_format = 0x%x\n", (*ptr).ve[13].ve0.src_format);
   debug_printf("\t\t.ve[13].ve0.valid = 0x%x\n", (*ptr).ve[13].ve0.valid);
   debug_printf("\t\t.ve[13].ve0.vertex_buffer_index = 0x%x\n", (*ptr).ve[13].ve0.vertex_buffer_index);
   debug_printf("\t\t.ve[13].ve1.dst_offset = 0x%x\n", (*ptr).ve[13].ve1.dst_offset);
   debug_printf("\t\t.ve[13].ve1.vfcomponent3 = 0x%x\n", (*ptr).ve[13].ve1.vfcomponent3);
   debug_printf("\t\t.ve[13].ve1.vfcomponent2 = 0x%x\n", (*ptr).ve[13].ve1.vfcomponent2);
   debug_printf("\t\t.ve[13].ve1.vfcomponent1 = 0x%x\n", (*ptr).ve[13].ve1.vfcomponent1);
   debug_printf("\t\t.ve[13].ve1.vfcomponent0 = 0x%x\n", (*ptr).ve[13].ve1.vfcomponent0);
   debug_printf("\t\t.ve[14].ve0.src_offset = 0x%x\n", (*ptr).ve[14].ve0.src_offset);
   debug_printf("\t\t.ve[14].ve0.src_format = 0x%x\n", (*ptr).ve[14].ve0.src_format);
   debug_printf("\t\t.ve[14].ve0.valid = 0x%x\n", (*ptr).ve[14].ve0.valid);
   debug_printf("\t\t.ve[14].ve0.vertex_buffer_index = 0x%x\n", (*ptr).ve[14].ve0.vertex_buffer_index);
   debug_printf("\t\t.ve[14].ve1.dst_offset = 0x%x\n", (*ptr).ve[14].ve1.dst_offset);
   debug_printf("\t\t.ve[14].ve1.vfcomponent3 = 0x%x\n", (*ptr).ve[14].ve1.vfcomponent3);
   debug_printf("\t\t.ve[14].ve1.vfcomponent2 = 0x%x\n", (*ptr).ve[14].ve1.vfcomponent2);
   debug_printf("\t\t.ve[14].ve1.vfcomponent1 = 0x%x\n", (*ptr).ve[14].ve1.vfcomponent1);
   debug_printf("\t\t.ve[14].ve1.vfcomponent0 = 0x%x\n", (*ptr).ve[14].ve1.vfcomponent0);
   debug_printf("\t\t.ve[15].ve0.src_offset = 0x%x\n", (*ptr).ve[15].ve0.src_offset);
   debug_printf("\t\t.ve[15].ve0.src_format = 0x%x\n", (*ptr).ve[15].ve0.src_format);
   debug_printf("\t\t.ve[15].ve0.valid = 0x%x\n", (*ptr).ve[15].ve0.valid);
   debug_printf("\t\t.ve[15].ve0.vertex_buffer_index = 0x%x\n", (*ptr).ve[15].ve0.vertex_buffer_index);
   debug_printf("\t\t.ve[15].ve1.dst_offset = 0x%x\n", (*ptr).ve[15].ve1.dst_offset);
   debug_printf("\t\t.ve[15].ve1.vfcomponent3 = 0x%x\n", (*ptr).ve[15].ve1.vfcomponent3);
   debug_printf("\t\t.ve[15].ve1.vfcomponent2 = 0x%x\n", (*ptr).ve[15].ve1.vfcomponent2);
   debug_printf("\t\t.ve[15].ve1.vfcomponent1 = 0x%x\n", (*ptr).ve[15].ve1.vfcomponent1);
   debug_printf("\t\t.ve[15].ve1.vfcomponent0 = 0x%x\n", (*ptr).ve[15].ve1.vfcomponent0);
   debug_printf("\t\t.ve[16].ve0.src_offset = 0x%x\n", (*ptr).ve[16].ve0.src_offset);
   debug_printf("\t\t.ve[16].ve0.src_format = 0x%x\n", (*ptr).ve[16].ve0.src_format);
   debug_printf("\t\t.ve[16].ve0.valid = 0x%x\n", (*ptr).ve[16].ve0.valid);
   debug_printf("\t\t.ve[16].ve0.vertex_buffer_index = 0x%x\n", (*ptr).ve[16].ve0.vertex_buffer_index);
   debug_printf("\t\t.ve[16].ve1.dst_offset = 0x%x\n", (*ptr).ve[16].ve1.dst_offset);
   debug_printf("\t\t.ve[16].ve1.vfcomponent3 = 0x%x\n", (*ptr).ve[16].ve1.vfcomponent3);
   debug_printf("\t\t.ve[16].ve1.vfcomponent2 = 0x%x\n", (*ptr).ve[16].ve1.vfcomponent2);
   debug_printf("\t\t.ve[16].ve1.vfcomponent1 = 0x%x\n", (*ptr).ve[16].ve1.vfcomponent1);
   debug_printf("\t\t.ve[16].ve1.vfcomponent0 = 0x%x\n", (*ptr).ve[16].ve1.vfcomponent0);
   debug_printf("\t\t.ve[17].ve0.src_offset = 0x%x\n", (*ptr).ve[17].ve0.src_offset);
   debug_printf("\t\t.ve[17].ve0.src_format = 0x%x\n", (*ptr).ve[17].ve0.src_format);
   debug_printf("\t\t.ve[17].ve0.valid = 0x%x\n", (*ptr).ve[17].ve0.valid);
   debug_printf("\t\t.ve[17].ve0.vertex_buffer_index = 0x%x\n", (*ptr).ve[17].ve0.vertex_buffer_index);
   debug_printf("\t\t.ve[17].ve1.dst_offset = 0x%x\n", (*ptr).ve[17].ve1.dst_offset);
   debug_printf("\t\t.ve[17].ve1.vfcomponent3 = 0x%x\n", (*ptr).ve[17].ve1.vfcomponent3);
   debug_printf("\t\t.ve[17].ve1.vfcomponent2 = 0x%x\n", (*ptr).ve[17].ve1.vfcomponent2);
   debug_printf("\t\t.ve[17].ve1.vfcomponent1 = 0x%x\n", (*ptr).ve[17].ve1.vfcomponent1);
   debug_printf("\t\t.ve[17].ve1.vfcomponent0 = 0x%x\n", (*ptr).ve[17].ve1.vfcomponent0);
}

void
brw_dump_vertex_element_state(const struct brw_vertex_element_state *ptr)
{
   debug_printf("\t\t.ve0.src_offset = 0x%x\n", (*ptr).ve0.src_offset);
   debug_printf("\t\t.ve0.src_format = 0x%x\n", (*ptr).ve0.src_format);
   debug_printf("\t\t.ve0.valid = 0x%x\n", (*ptr).ve0.valid);
   debug_printf("\t\t.ve0.vertex_buffer_index = 0x%x\n", (*ptr).ve0.vertex_buffer_index);
   debug_printf("\t\t.ve1.dst_offset = 0x%x\n", (*ptr).ve1.dst_offset);
   debug_printf("\t\t.ve1.vfcomponent3 = 0x%x\n", (*ptr).ve1.vfcomponent3);
   debug_printf("\t\t.ve1.vfcomponent2 = 0x%x\n", (*ptr).ve1.vfcomponent2);
   debug_printf("\t\t.ve1.vfcomponent1 = 0x%x\n", (*ptr).ve1.vfcomponent1);
   debug_printf("\t\t.ve1.vfcomponent0 = 0x%x\n", (*ptr).ve1.vfcomponent0);
}

void
brw_dump_vf_statistics(const struct brw_vf_statistics *ptr)
{
   debug_printf("\t\t.statistics_enable = 0x%x\n", (*ptr).statistics_enable);
   debug_printf("\t\t.opcode = 0x%x\n", (*ptr).opcode);
}

void
brw_dump_vs_unit_state(const struct brw_vs_unit_state *ptr)
{
   debug_printf("\t\t.thread0.grf_reg_count = 0x%x\n", (*ptr).thread0.grf_reg_count);
   debug_printf("\t\t.thread0.kernel_start_pointer = 0x%x\n", (*ptr).thread0.kernel_start_pointer);
   debug_printf("\t\t.thread1.ext_halt_exception_enable = 0x%x\n", (*ptr).thread1.ext_halt_exception_enable);
   debug_printf("\t\t.thread1.sw_exception_enable = 0x%x\n", (*ptr).thread1.sw_exception_enable);
   debug_printf("\t\t.thread1.mask_stack_exception_enable = 0x%x\n", (*ptr).thread1.mask_stack_exception_enable);
   debug_printf("\t\t.thread1.timeout_exception_enable = 0x%x\n", (*ptr).thread1.timeout_exception_enable);
   debug_printf("\t\t.thread1.illegal_op_exception_enable = 0x%x\n", (*ptr).thread1.illegal_op_exception_enable);
   debug_printf("\t\t.thread1.depth_coef_urb_read_offset = 0x%x\n", (*ptr).thread1.depth_coef_urb_read_offset);
   debug_printf("\t\t.thread1.floating_point_mode = 0x%x\n", (*ptr).thread1.floating_point_mode);
   debug_printf("\t\t.thread1.thread_priority = 0x%x\n", (*ptr).thread1.thread_priority);
   debug_printf("\t\t.thread1.binding_table_entry_count = 0x%x\n", (*ptr).thread1.binding_table_entry_count);
   debug_printf("\t\t.thread1.single_program_flow = 0x%x\n", (*ptr).thread1.single_program_flow);
   debug_printf("\t\t.thread2.per_thread_scratch_space = 0x%x\n", (*ptr).thread2.per_thread_scratch_space);
   debug_printf("\t\t.thread2.scratch_space_base_pointer = 0x%x\n", (*ptr).thread2.scratch_space_base_pointer);
   debug_printf("\t\t.thread3.dispatch_grf_start_reg = 0x%x\n", (*ptr).thread3.dispatch_grf_start_reg);
   debug_printf("\t\t.thread3.urb_entry_read_offset = 0x%x\n", (*ptr).thread3.urb_entry_read_offset);
   debug_printf("\t\t.thread3.urb_entry_read_length = 0x%x\n", (*ptr).thread3.urb_entry_read_length);
   debug_printf("\t\t.thread3.const_urb_entry_read_offset = 0x%x\n", (*ptr).thread3.const_urb_entry_read_offset);
   debug_printf("\t\t.thread3.const_urb_entry_read_length = 0x%x\n", (*ptr).thread3.const_urb_entry_read_length);
   debug_printf("\t\t.thread4.stats_enable = 0x%x\n", (*ptr).thread4.stats_enable);
   debug_printf("\t\t.thread4.nr_urb_entries = 0x%x\n", (*ptr).thread4.nr_urb_entries);
   debug_printf("\t\t.thread4.urb_entry_allocation_size = 0x%x\n", (*ptr).thread4.urb_entry_allocation_size);
   debug_printf("\t\t.thread4.max_threads = 0x%x\n", (*ptr).thread4.max_threads);
   debug_printf("\t\t.vs5.sampler_count = 0x%x\n", (*ptr).vs5.sampler_count);
   debug_printf("\t\t.vs5.sampler_state_pointer = 0x%x\n", (*ptr).vs5.sampler_state_pointer);
   debug_printf("\t\t.vs6.vs_enable = 0x%x\n", (*ptr).vs6.vs_enable);
   debug_printf("\t\t.vs6.vert_cache_disable = 0x%x\n", (*ptr).vs6.vert_cache_disable);
}

void
brw_dump_wm_unit_state(const struct brw_wm_unit_state *ptr)
{
   debug_printf("\t\t.thread0.grf_reg_count = 0x%x\n", (*ptr).thread0.grf_reg_count);
   debug_printf("\t\t.thread0.kernel_start_pointer = 0x%x\n", (*ptr).thread0.kernel_start_pointer);
   debug_printf("\t\t.thread1.ext_halt_exception_enable = 0x%x\n", (*ptr).thread1.ext_halt_exception_enable);
   debug_printf("\t\t.thread1.sw_exception_enable = 0x%x\n", (*ptr).thread1.sw_exception_enable);
   debug_printf("\t\t.thread1.mask_stack_exception_enable = 0x%x\n", (*ptr).thread1.mask_stack_exception_enable);
   debug_printf("\t\t.thread1.timeout_exception_enable = 0x%x\n", (*ptr).thread1.timeout_exception_enable);
   debug_printf("\t\t.thread1.illegal_op_exception_enable = 0x%x\n", (*ptr).thread1.illegal_op_exception_enable);
   debug_printf("\t\t.thread1.depth_coef_urb_read_offset = 0x%x\n", (*ptr).thread1.depth_coef_urb_read_offset);
   debug_printf("\t\t.thread1.floating_point_mode = 0x%x\n", (*ptr).thread1.floating_point_mode);
   debug_printf("\t\t.thread1.thread_priority = 0x%x\n", (*ptr).thread1.thread_priority);
   debug_printf("\t\t.thread1.binding_table_entry_count = 0x%x\n", (*ptr).thread1.binding_table_entry_count);
   debug_printf("\t\t.thread1.single_program_flow = 0x%x\n", (*ptr).thread1.single_program_flow);
   debug_printf("\t\t.thread2.per_thread_scratch_space = 0x%x\n", (*ptr).thread2.per_thread_scratch_space);
   debug_printf("\t\t.thread2.scratch_space_base_pointer = 0x%x\n", (*ptr).thread2.scratch_space_base_pointer);
   debug_printf("\t\t.thread3.dispatch_grf_start_reg = 0x%x\n", (*ptr).thread3.dispatch_grf_start_reg);
   debug_printf("\t\t.thread3.urb_entry_read_offset = 0x%x\n", (*ptr).thread3.urb_entry_read_offset);
   debug_printf("\t\t.thread3.urb_entry_read_length = 0x%x\n", (*ptr).thread3.urb_entry_read_length);
   debug_printf("\t\t.thread3.const_urb_entry_read_offset = 0x%x\n", (*ptr).thread3.const_urb_entry_read_offset);
   debug_printf("\t\t.thread3.const_urb_entry_read_length = 0x%x\n", (*ptr).thread3.const_urb_entry_read_length);
   debug_printf("\t\t.wm4.stats_enable = 0x%x\n", (*ptr).wm4.stats_enable);
   debug_printf("\t\t.wm4.depth_buffer_clear = 0x%x\n", (*ptr).wm4.depth_buffer_clear);
   debug_printf("\t\t.wm4.sampler_count = 0x%x\n", (*ptr).wm4.sampler_count);
   debug_printf("\t\t.wm4.sampler_state_pointer = 0x%x\n", (*ptr).wm4.sampler_state_pointer);
   debug_printf("\t\t.wm5.enable_8_pix = 0x%x\n", (*ptr).wm5.enable_8_pix);
   debug_printf("\t\t.wm5.enable_16_pix = 0x%x\n", (*ptr).wm5.enable_16_pix);
   debug_printf("\t\t.wm5.enable_32_pix = 0x%x\n", (*ptr).wm5.enable_32_pix);
   debug_printf("\t\t.wm5.enable_con_32_pix = 0x%x\n", (*ptr).wm5.enable_con_32_pix);
   debug_printf("\t\t.wm5.enable_con_64_pix = 0x%x\n", (*ptr).wm5.enable_con_64_pix);
   debug_printf("\t\t.wm5.legacy_global_depth_bias = 0x%x\n", (*ptr).wm5.legacy_global_depth_bias);
   debug_printf("\t\t.wm5.line_stipple = 0x%x\n", (*ptr).wm5.line_stipple);
   debug_printf("\t\t.wm5.depth_offset = 0x%x\n", (*ptr).wm5.depth_offset);
   debug_printf("\t\t.wm5.polygon_stipple = 0x%x\n", (*ptr).wm5.polygon_stipple);
   debug_printf("\t\t.wm5.line_aa_region_width = 0x%x\n", (*ptr).wm5.line_aa_region_width);
   debug_printf("\t\t.wm5.line_endcap_aa_region_width = 0x%x\n", (*ptr).wm5.line_endcap_aa_region_width);
   debug_printf("\t\t.wm5.early_depth_test = 0x%x\n", (*ptr).wm5.early_depth_test);
   debug_printf("\t\t.wm5.thread_dispatch_enable = 0x%x\n", (*ptr).wm5.thread_dispatch_enable);
   debug_printf("\t\t.wm5.program_uses_depth = 0x%x\n", (*ptr).wm5.program_uses_depth);
   debug_printf("\t\t.wm5.program_computes_depth = 0x%x\n", (*ptr).wm5.program_computes_depth);
   debug_printf("\t\t.wm5.program_uses_killpixel = 0x%x\n", (*ptr).wm5.program_uses_killpixel);
   debug_printf("\t\t.wm5.legacy_line_rast = 0x%x\n", (*ptr).wm5.legacy_line_rast);
   debug_printf("\t\t.wm5.transposed_urb_read_enable = 0x%x\n", (*ptr).wm5.transposed_urb_read_enable);
   debug_printf("\t\t.wm5.max_threads = 0x%x\n", (*ptr).wm5.max_threads);
   debug_printf("\t\t.global_depth_offset_constant = %f\n", (*ptr).global_depth_offset_constant);
   debug_printf("\t\t.global_depth_offset_scale = %f\n", (*ptr).global_depth_offset_scale);
   debug_printf("\t\t.wm8.grf_reg_count_1 = 0x%x\n", (*ptr).wm8.grf_reg_count_1);
   debug_printf("\t\t.wm8.kernel_start_pointer_1 = 0x%x\n", (*ptr).wm8.kernel_start_pointer_1);
   debug_printf("\t\t.wm9.grf_reg_count_2 = 0x%x\n", (*ptr).wm9.grf_reg_count_2);
   debug_printf("\t\t.wm9.kernel_start_pointer_2 = 0x%x\n", (*ptr).wm9.kernel_start_pointer_2);
   debug_printf("\t\t.wm10.grf_reg_count_3 = 0x%x\n", (*ptr).wm10.grf_reg_count_3);
   debug_printf("\t\t.wm10.kernel_start_pointer_3 = 0x%x\n", (*ptr).wm10.kernel_start_pointer_3);
}


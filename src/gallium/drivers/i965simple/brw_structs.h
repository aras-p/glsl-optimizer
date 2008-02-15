/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:

 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */


#ifndef BRW_STRUCTS_H
#define BRW_STRUCTS_H

#include "pipe/p_compiler.h"

/* Command packets:
 */
struct header
{
   unsigned length:16;
   unsigned opcode:16;
};


union header_union
{
   struct header bits;
   unsigned dword;
};

struct brw_3d_control
{
   struct
   {
      unsigned length:8;
      unsigned notify_enable:1;
      unsigned pad:3;
      unsigned wc_flush_enable:1;
      unsigned depth_stall_enable:1;
      unsigned operation:2;
      unsigned opcode:16;
   } header;

   struct
   {
      unsigned pad:2;
      unsigned dest_addr_type:1;
      unsigned dest_addr:29;
   } dest;

   unsigned dword2;
   unsigned dword3;
};


struct brw_3d_primitive
{
   struct
   {
      unsigned length:8;
      unsigned pad:2;
      unsigned topology:5;
      unsigned indexed:1;
      unsigned opcode:16;
   } header;

   unsigned verts_per_instance;
   unsigned start_vert_location;
   unsigned instance_count;
   unsigned start_instance_location;
   unsigned base_vert_location;
};

/* These seem to be passed around as function args, so it works out
 * better to keep them as #defines:
 */
#define BRW_FLUSH_READ_CACHE           0x1
#define BRW_FLUSH_STATE_CACHE          0x2
#define BRW_INHIBIT_FLUSH_RENDER_CACHE 0x4
#define BRW_FLUSH_SNAPSHOT_COUNTERS    0x8

struct brw_mi_flush
{
   unsigned flags:4;
   unsigned pad:12;
   unsigned opcode:16;
};

struct brw_vf_statistics
{
   unsigned statistics_enable:1;
   unsigned pad:15;
   unsigned opcode:16;
};



struct brw_binding_table_pointers
{
   struct header header;
   unsigned vs;
   unsigned gs;
   unsigned clp;
   unsigned sf;
   unsigned wm;
};


struct brw_blend_constant_color
{
   struct header header;
   float blend_constant_color[4];
};


struct brw_depthbuffer
{
   union header_union header;

   union {
      struct {
	 unsigned pitch:18;
	 unsigned format:3;
	 unsigned pad:4;
	 unsigned depth_offset_disable:1;
	 unsigned tile_walk:1;
	 unsigned tiled_surface:1;
	 unsigned pad2:1;
	 unsigned surface_type:3;
      } bits;
      unsigned dword;
   } dword1;

   unsigned dword2_base_addr;

   union {
      struct {
	 unsigned pad:1;
	 unsigned mipmap_layout:1;
	 unsigned lod:4;
	 unsigned width:13;
	 unsigned height:13;
      } bits;
      unsigned dword;
   } dword3;

   union {
      struct {
	 unsigned pad:12;
	 unsigned min_array_element:9;
	 unsigned depth:11;
      } bits;
      unsigned dword;
   } dword4;
};

struct brw_drawrect
{
   struct header header;
   unsigned xmin:16;
   unsigned ymin:16;
   unsigned xmax:16;
   unsigned ymax:16;
   unsigned xorg:16;
   unsigned yorg:16;
};




struct brw_global_depth_offset_clamp
{
   struct header header;
   float depth_offset_clamp;
};

struct brw_indexbuffer
{
   union {
      struct
      {
	 unsigned length:8;
	 unsigned index_format:2;
	 unsigned cut_index_enable:1;
	 unsigned pad:5;
	 unsigned opcode:16;
      } bits;
      unsigned dword;

   } header;

   unsigned buffer_start;
   unsigned buffer_end;
};


struct brw_line_stipple
{
   struct header header;

   struct
   {
      unsigned pattern:16;
      unsigned pad:16;
   } bits0;

   struct
   {
      unsigned repeat_count:9;
      unsigned pad:7;
      unsigned inverse_repeat_count:16;
   } bits1;
};


struct brw_pipelined_state_pointers
{
   struct header header;

   struct {
      unsigned pad:5;
      unsigned offset:27;
   } vs;

   struct
   {
      unsigned enable:1;
      unsigned pad:4;
      unsigned offset:27;
   } gs;

   struct
   {
      unsigned enable:1;
      unsigned pad:4;
      unsigned offset:27;
   } clp;

   struct
   {
      unsigned pad:5;
      unsigned offset:27;
   } sf;

   struct
   {
      unsigned pad:5;
      unsigned offset:27;
   } wm;

   struct
   {
      unsigned pad:5;
      unsigned offset:27; /* KW: check me! */
   } cc;
};


struct brw_polygon_stipple_offset
{
   struct header header;

   struct {
      unsigned y_offset:5;
      unsigned pad:3;
      unsigned x_offset:5;
      unsigned pad0:19;
   } bits0;
};



struct brw_polygon_stipple
{
   struct header header;
   unsigned stipple[32];
};



struct brw_pipeline_select
{
   struct
   {
      unsigned pipeline_select:1;
      unsigned pad:15;
      unsigned opcode:16;
   } header;
};


struct brw_pipe_control
{
   struct
   {
      unsigned length:8;
      unsigned notify_enable:1;
      unsigned pad:2;
      unsigned instruction_state_cache_flush_enable:1;
      unsigned write_cache_flush_enable:1;
      unsigned depth_stall_enable:1;
      unsigned post_sync_operation:2;

      unsigned opcode:16;
   } header;

   struct
   {
      unsigned pad:2;
      unsigned dest_addr_type:1;
      unsigned dest_addr:29;
   } bits1;

   unsigned data0;
   unsigned data1;
};


struct brw_urb_fence
{
   struct
   {
      unsigned length:8;
      unsigned vs_realloc:1;
      unsigned gs_realloc:1;
      unsigned clp_realloc:1;
      unsigned sf_realloc:1;
      unsigned vfe_realloc:1;
      unsigned cs_realloc:1;
      unsigned pad:2;
      unsigned opcode:16;
   } header;

   struct
   {
      unsigned vs_fence:10;
      unsigned gs_fence:10;
      unsigned clp_fence:10;
      unsigned pad:2;
   } bits0;

   struct
   {
      unsigned sf_fence:10;
      unsigned vf_fence:10;
      unsigned cs_fence:10;
      unsigned pad:2;
   } bits1;
};

struct brw_constant_buffer_state /* previously brw_command_streamer */
{
   struct header header;

   struct
   {
      unsigned nr_urb_entries:3;
      unsigned pad:1;
      unsigned urb_entry_size:5;
      unsigned pad0:23;
   } bits0;
};

struct brw_constant_buffer
{
   struct
   {
      unsigned length:8;
      unsigned valid:1;
      unsigned pad:7;
      unsigned opcode:16;
   } header;

   struct
   {
      unsigned buffer_length:6;
      unsigned buffer_address:26;
   } bits0;
};

struct brw_state_base_address
{
   struct header header;

   struct
   {
      unsigned modify_enable:1;
      unsigned pad:4;
      unsigned general_state_address:27;
   } bits0;

   struct
   {
      unsigned modify_enable:1;
      unsigned pad:4;
      unsigned surface_state_address:27;
   } bits1;

   struct
   {
      unsigned modify_enable:1;
      unsigned pad:4;
      unsigned indirect_object_state_address:27;
   } bits2;

   struct
   {
      unsigned modify_enable:1;
      unsigned pad:11;
      unsigned general_state_upper_bound:20;
   } bits3;

   struct
   {
      unsigned modify_enable:1;
      unsigned pad:11;
      unsigned indirect_object_state_upper_bound:20;
   } bits4;
};

struct brw_state_prefetch
{
   struct header header;

   struct
   {
      unsigned prefetch_count:3;
      unsigned pad:3;
      unsigned prefetch_pointer:26;
   } bits0;
};

struct brw_system_instruction_pointer
{
   struct header header;

   struct
   {
      unsigned pad:4;
      unsigned system_instruction_pointer:28;
   } bits0;
};




/* State structs for the various fixed function units:
 */


struct thread0
{
   unsigned pad0:1;
   unsigned grf_reg_count:3;
   unsigned pad1:2;
   unsigned kernel_start_pointer:26;
};

struct thread1
{
   unsigned ext_halt_exception_enable:1;
   unsigned sw_exception_enable:1;
   unsigned mask_stack_exception_enable:1;
   unsigned timeout_exception_enable:1;
   unsigned illegal_op_exception_enable:1;
   unsigned pad0:3;
   unsigned depth_coef_urb_read_offset:6;	/* WM only */
   unsigned pad1:2;
   unsigned floating_point_mode:1;
   unsigned thread_priority:1;
   unsigned binding_table_entry_count:8;
   unsigned pad3:5;
   unsigned single_program_flow:1;
};

struct thread2
{
   unsigned per_thread_scratch_space:4;
   unsigned pad0:6;
   unsigned scratch_space_base_pointer:22;
};


struct thread3
{
   unsigned dispatch_grf_start_reg:4;
   unsigned urb_entry_read_offset:6;
   unsigned pad0:1;
   unsigned urb_entry_read_length:6;
   unsigned pad1:1;
   unsigned const_urb_entry_read_offset:6;
   unsigned pad2:1;
   unsigned const_urb_entry_read_length:6;
   unsigned pad3:1;
};



struct brw_clip_unit_state
{
   struct thread0 thread0;
   struct
   {
      unsigned pad0:7;
      unsigned sw_exception_enable:1;
      unsigned pad1:3;
      unsigned mask_stack_exception_enable:1;
      unsigned pad2:1;
      unsigned illegal_op_exception_enable:1;
      unsigned pad3:2;
      unsigned floating_point_mode:1;
      unsigned thread_priority:1;
      unsigned binding_table_entry_count:8;
      unsigned pad4:5;
      unsigned single_program_flow:1;
   } thread1;

   struct thread2 thread2;
   struct thread3 thread3;

   struct
   {
      unsigned pad0:9;
      unsigned gs_output_stats:1; /* not always */
      unsigned stats_enable:1;
      unsigned nr_urb_entries:7;
      unsigned pad1:1;
      unsigned urb_entry_allocation_size:5;
      unsigned pad2:1;
      unsigned max_threads:1; 	/* may be less */
      unsigned pad3:6;
   } thread4;

   struct
   {
      unsigned pad0:13;
      unsigned clip_mode:3;
      unsigned userclip_enable_flags:8;
      unsigned userclip_must_clip:1;
      unsigned pad1:1;
      unsigned guard_band_enable:1;
      unsigned viewport_z_clip_enable:1;
      unsigned viewport_xy_clip_enable:1;
      unsigned vertex_position_space:1;
      unsigned api_mode:1;
      unsigned pad2:1;
   } clip5;

   struct
   {
      unsigned pad0:5;
      unsigned clipper_viewport_state_ptr:27;
   } clip6;


   float viewport_xmin;
   float viewport_xmax;
   float viewport_ymin;
   float viewport_ymax;
};



struct brw_cc_unit_state
{
   struct
   {
      unsigned pad0:3;
      unsigned bf_stencil_pass_depth_pass_op:3;
      unsigned bf_stencil_pass_depth_fail_op:3;
      unsigned bf_stencil_fail_op:3;
      unsigned bf_stencil_func:3;
      unsigned bf_stencil_enable:1;
      unsigned pad1:2;
      unsigned stencil_write_enable:1;
      unsigned stencil_pass_depth_pass_op:3;
      unsigned stencil_pass_depth_fail_op:3;
      unsigned stencil_fail_op:3;
      unsigned stencil_func:3;
      unsigned stencil_enable:1;
   } cc0;


   struct
   {
      unsigned bf_stencil_ref:8;
      unsigned stencil_write_mask:8;
      unsigned stencil_test_mask:8;
      unsigned stencil_ref:8;
   } cc1;


   struct
   {
      unsigned logicop_enable:1;
      unsigned pad0:10;
      unsigned depth_write_enable:1;
      unsigned depth_test_function:3;
      unsigned depth_test:1;
      unsigned bf_stencil_write_mask:8;
      unsigned bf_stencil_test_mask:8;
   } cc2;


   struct
   {
      unsigned pad0:8;
      unsigned alpha_test_func:3;
      unsigned alpha_test:1;
      unsigned blend_enable:1;
      unsigned ia_blend_enable:1;
      unsigned pad1:1;
      unsigned alpha_test_format:1;
      unsigned pad2:16;
   } cc3;

   struct
   {
      unsigned pad0:5;
      unsigned cc_viewport_state_offset:27;
   } cc4;

   struct
   {
      unsigned pad0:2;
      unsigned ia_dest_blend_factor:5;
      unsigned ia_src_blend_factor:5;
      unsigned ia_blend_function:3;
      unsigned statistics_enable:1;
      unsigned logicop_func:4;
      unsigned pad1:11;
      unsigned dither_enable:1;
   } cc5;

   struct
   {
      unsigned clamp_post_alpha_blend:1;
      unsigned clamp_pre_alpha_blend:1;
      unsigned clamp_range:2;
      unsigned pad0:11;
      unsigned y_dither_offset:2;
      unsigned x_dither_offset:2;
      unsigned dest_blend_factor:5;
      unsigned src_blend_factor:5;
      unsigned blend_function:3;
   } cc6;

   struct {
      union {
	 float f;
	 ubyte ub[4];
      } alpha_ref;
   } cc7;
};



struct brw_sf_unit_state
{
   struct thread0 thread0;
   struct thread1 thread1;
   struct thread2 thread2;
   struct thread3 thread3;

   struct
   {
      unsigned pad0:10;
      unsigned stats_enable:1;
      unsigned nr_urb_entries:7;
      unsigned pad1:1;
      unsigned urb_entry_allocation_size:5;
      unsigned pad2:1;
      unsigned max_threads:6;
      unsigned pad3:1;
   } thread4;

   struct
   {
      unsigned front_winding:1;
      unsigned viewport_transform:1;
      unsigned pad0:3;
      unsigned sf_viewport_state_offset:27;
   } sf5;

   struct
   {
      unsigned pad0:9;
      unsigned dest_org_vbias:4;
      unsigned dest_org_hbias:4;
      unsigned scissor:1;
      unsigned disable_2x2_trifilter:1;
      unsigned disable_zero_pix_trifilter:1;
      unsigned point_rast_rule:2;
      unsigned line_endcap_aa_region_width:2;
      unsigned line_width:4;
      unsigned fast_scissor_disable:1;
      unsigned cull_mode:2;
      unsigned aa_enable:1;
   } sf6;

   struct
   {
      unsigned point_size:11;
      unsigned use_point_size_state:1;
      unsigned subpixel_precision:1;
      unsigned sprite_point:1;
      unsigned pad0:11;
      unsigned trifan_pv:2;
      unsigned linestrip_pv:2;
      unsigned tristrip_pv:2;
      unsigned line_last_pixel_enable:1;
   } sf7;

};


struct brw_gs_unit_state
{
   struct thread0 thread0;
   struct thread1 thread1;
   struct thread2 thread2;
   struct thread3 thread3;

   struct
   {
      unsigned pad0:10;
      unsigned stats_enable:1;
      unsigned nr_urb_entries:7;
      unsigned pad1:1;
      unsigned urb_entry_allocation_size:5;
      unsigned pad2:1;
      unsigned max_threads:1;
      unsigned pad3:6;
   } thread4;

   struct
   {
      unsigned sampler_count:3;
      unsigned pad0:2;
      unsigned sampler_state_pointer:27;
   } gs5;


   struct
   {
      unsigned max_vp_index:4;
      unsigned pad0:26;
      unsigned reorder_enable:1;
      unsigned pad1:1;
   } gs6;
};


struct brw_vs_unit_state
{
   struct thread0 thread0;
   struct thread1 thread1;
   struct thread2 thread2;
   struct thread3 thread3;

   struct
   {
      unsigned pad0:10;
      unsigned stats_enable:1;
      unsigned nr_urb_entries:7;
      unsigned pad1:1;
      unsigned urb_entry_allocation_size:5;
      unsigned pad2:1;
      unsigned max_threads:4;
      unsigned pad3:3;
   } thread4;

   struct
   {
      unsigned sampler_count:3;
      unsigned pad0:2;
      unsigned sampler_state_pointer:27;
   } vs5;

   struct
   {
      unsigned vs_enable:1;
      unsigned vert_cache_disable:1;
      unsigned pad0:30;
   } vs6;
};


struct brw_wm_unit_state
{
   struct thread0 thread0;
   struct thread1 thread1;
   struct thread2 thread2;
   struct thread3 thread3;

   struct {
      unsigned stats_enable:1;
      unsigned pad0:1;
      unsigned sampler_count:3;
      unsigned sampler_state_pointer:27;
   } wm4;

   struct
   {
      unsigned enable_8_pix:1;
      unsigned enable_16_pix:1;
      unsigned enable_32_pix:1;
      unsigned pad0:7;
      unsigned legacy_global_depth_bias:1;
      unsigned line_stipple:1;
      unsigned depth_offset:1;
      unsigned polygon_stipple:1;
      unsigned line_aa_region_width:2;
      unsigned line_endcap_aa_region_width:2;
      unsigned early_depth_test:1;
      unsigned thread_dispatch_enable:1;
      unsigned program_uses_depth:1;
      unsigned program_computes_depth:1;
      unsigned program_uses_killpixel:1;
      unsigned legacy_line_rast: 1;
      unsigned pad1:1;
      unsigned max_threads:6;
      unsigned pad2:1;
   } wm5;

   float global_depth_offset_constant;
   float global_depth_offset_scale;
};

struct brw_sampler_default_color {
   float color[4];
};

struct brw_sampler_state
{

   struct
   {
      unsigned shadow_function:3;
      unsigned lod_bias:11;
      unsigned min_filter:3;
      unsigned mag_filter:3;
      unsigned mip_filter:2;
      unsigned base_level:5;
      unsigned pad:1;
      unsigned lod_preclamp:1;
      unsigned default_color_mode:1;
      unsigned pad0:1;
      unsigned disable:1;
   } ss0;

   struct
   {
      unsigned r_wrap_mode:3;
      unsigned t_wrap_mode:3;
      unsigned s_wrap_mode:3;
      unsigned pad:3;
      unsigned max_lod:10;
      unsigned min_lod:10;
   } ss1;


   struct
   {
      unsigned pad:5;
      unsigned default_color_pointer:27;
   } ss2;

   struct
   {
      unsigned pad:19;
      unsigned max_aniso:3;
      unsigned chroma_key_mode:1;
      unsigned chroma_key_index:2;
      unsigned chroma_key_enable:1;
      unsigned monochrome_filter_width:3;
      unsigned monochrome_filter_height:3;
   } ss3;
};


struct brw_clipper_viewport
{
   float xmin;
   float xmax;
   float ymin;
   float ymax;
};

struct brw_cc_viewport
{
   float min_depth;
   float max_depth;
};

struct brw_sf_viewport
{
   struct {
      float m00;
      float m11;
      float m22;
      float m30;
      float m31;
      float m32;
   } viewport;

   struct {
      short xmin;
      short ymin;
      short xmax;
      short ymax;
   } scissor;
};

/* Documented in the subsystem/shared-functions/sampler chapter...
 */
struct brw_surface_state
{
   struct {
      unsigned cube_pos_z:1;
      unsigned cube_neg_z:1;
      unsigned cube_pos_y:1;
      unsigned cube_neg_y:1;
      unsigned cube_pos_x:1;
      unsigned cube_neg_x:1;
      unsigned pad:4;
      unsigned mipmap_layout_mode:1;
      unsigned vert_line_stride_ofs:1;
      unsigned vert_line_stride:1;
      unsigned color_blend:1;
      unsigned writedisable_blue:1;
      unsigned writedisable_green:1;
      unsigned writedisable_red:1;
      unsigned writedisable_alpha:1;
      unsigned surface_format:9;
      unsigned data_return_format:1;
      unsigned pad0:1;
      unsigned surface_type:3;
   } ss0;

   struct {
      unsigned base_addr;
   } ss1;

   struct {
      unsigned pad:2;
      unsigned mip_count:4;
      unsigned width:13;
      unsigned height:13;
   } ss2;

   struct {
      unsigned tile_walk:1;
      unsigned tiled_surface:1;
      unsigned pad:1;
      unsigned pitch:18;
      unsigned depth:11;
   } ss3;

   struct {
      unsigned pad:19;
      unsigned min_array_elt:9;
      unsigned min_lod:4;
   } ss4;
};



struct brw_vertex_buffer_state
{
   struct {
      unsigned pitch:11;
      unsigned pad:15;
      unsigned access_type:1;
      unsigned vb_index:5;
   } vb0;

   unsigned start_addr;
   unsigned max_index;
#if 1
   unsigned instance_data_step_rate; /* not included for sequential/random vertices? */
#endif
};

#define BRW_VBP_MAX 17

struct brw_vb_array_state {
   struct header header;
   struct brw_vertex_buffer_state vb[BRW_VBP_MAX];
};


struct brw_vertex_element_state
{
   struct
   {
      unsigned src_offset:11;
      unsigned pad:5;
      unsigned src_format:9;
      unsigned pad0:1;
      unsigned valid:1;
      unsigned vertex_buffer_index:5;
   } ve0;

   struct
   {
      unsigned dst_offset:8;
      unsigned pad:8;
      unsigned vfcomponent3:4;
      unsigned vfcomponent2:4;
      unsigned vfcomponent1:4;
      unsigned vfcomponent0:4;
   } ve1;
};

#define BRW_VEP_MAX 18

struct brw_vertex_element_packet {
   struct header header;
   struct brw_vertex_element_state ve[BRW_VEP_MAX]; /* note: less than _TNL_ATTRIB_MAX */
};


struct brw_urb_immediate {
   unsigned opcode:4;
   unsigned offset:6;
   unsigned swizzle_control:2;
   unsigned pad:1;
   unsigned allocate:1;
   unsigned used:1;
   unsigned complete:1;
   unsigned response_length:4;
   unsigned msg_length:4;
   unsigned msg_target:4;
   unsigned pad1:3;
   unsigned end_of_thread:1;
};

/* Instruction format for the execution units:
 */

struct brw_instruction
{
   struct
   {
      unsigned opcode:7;
      unsigned pad:1;
      unsigned access_mode:1;
      unsigned mask_control:1;
      unsigned dependency_control:2;
      unsigned compression_control:2;
      unsigned thread_control:2;
      unsigned predicate_control:4;
      unsigned predicate_inverse:1;
      unsigned execution_size:3;
      unsigned destreg__conditonalmod:4; /* destreg - send, conditionalmod - others */
      unsigned pad0:2;
      unsigned debug_control:1;
      unsigned saturate:1;
   } header;

   union {
      struct
      {
	 unsigned dest_reg_file:2;
	 unsigned dest_reg_type:3;
	 unsigned src0_reg_file:2;
	 unsigned src0_reg_type:3;
	 unsigned src1_reg_file:2;
	 unsigned src1_reg_type:3;
	 unsigned pad:1;
	 unsigned dest_subreg_nr:5;
	 unsigned dest_reg_nr:8;
	 unsigned dest_horiz_stride:2;
	 unsigned dest_address_mode:1;
      } da1;

      struct
      {
	 unsigned dest_reg_file:2;
	 unsigned dest_reg_type:3;
	 unsigned src0_reg_file:2;
	 unsigned src0_reg_type:3;
	 unsigned pad:6;
	 int dest_indirect_offset:10;	/* offset against the deref'd address reg */
	 unsigned dest_subreg_nr:3; /* subnr for the address reg a0.x */
	 unsigned dest_horiz_stride:2;
	 unsigned dest_address_mode:1;
      } ia1;

      struct
      {
	 unsigned dest_reg_file:2;
	 unsigned dest_reg_type:3;
	 unsigned src0_reg_file:2;
	 unsigned src0_reg_type:3;
	 unsigned src1_reg_file:2;
	 unsigned src1_reg_type:3;
	 unsigned pad0:1;
	 unsigned dest_writemask:4;
	 unsigned dest_subreg_nr:1;
	 unsigned dest_reg_nr:8;
	 unsigned pad1:2;
	 unsigned dest_address_mode:1;
      } da16;

      struct
      {
	 unsigned dest_reg_file:2;
	 unsigned dest_reg_type:3;
	 unsigned src0_reg_file:2;
	 unsigned src0_reg_type:3;
	 unsigned pad0:6;
	 unsigned dest_writemask:4;
	 int dest_indirect_offset:6;
	 unsigned dest_subreg_nr:3;
	 unsigned pad1:2;
	 unsigned dest_address_mode:1;
      } ia16;
   } bits1;


   union {
      struct
      {
	 unsigned src0_subreg_nr:5;
	 unsigned src0_reg_nr:8;
	 unsigned src0_abs:1;
	 unsigned src0_negate:1;
	 unsigned src0_address_mode:1;
	 unsigned src0_horiz_stride:2;
	 unsigned src0_width:3;
	 unsigned src0_vert_stride:4;
	 unsigned flag_reg_nr:1;
	 unsigned pad:6;
      } da1;

      struct
      {
	 int src0_indirect_offset:10;
	 unsigned src0_subreg_nr:3;
	 unsigned src0_abs:1;
	 unsigned src0_negate:1;
	 unsigned src0_address_mode:1;
	 unsigned src0_horiz_stride:2;
	 unsigned src0_width:3;
	 unsigned src0_vert_stride:4;
	 unsigned flag_reg_nr:1;
	 unsigned pad:6;
      } ia1;

      struct
      {
	 unsigned src0_swz_x:2;
	 unsigned src0_swz_y:2;
	 unsigned src0_subreg_nr:1;
	 unsigned src0_reg_nr:8;
	 unsigned src0_abs:1;
	 unsigned src0_negate:1;
	 unsigned src0_address_mode:1;
	 unsigned src0_swz_z:2;
	 unsigned src0_swz_w:2;
	 unsigned pad0:1;
	 unsigned src0_vert_stride:4;
	 unsigned flag_reg_nr:1;
	 unsigned pad1:6;
      } da16;

      struct
      {
	 unsigned src0_swz_x:2;
	 unsigned src0_swz_y:2;
	 int src0_indirect_offset:6;
	 unsigned src0_subreg_nr:3;
	 unsigned src0_abs:1;
	 unsigned src0_negate:1;
	 unsigned src0_address_mode:1;
	 unsigned src0_swz_z:2;
	 unsigned src0_swz_w:2;
	 unsigned pad0:1;
	 unsigned src0_vert_stride:4;
	 unsigned flag_reg_nr:1;
	 unsigned pad1:6;
      } ia16;

   } bits2;

   union
   {
      struct
      {
	 unsigned src1_subreg_nr:5;
	 unsigned src1_reg_nr:8;
	 unsigned src1_abs:1;
	 unsigned src1_negate:1;
	 unsigned pad:1;
	 unsigned src1_horiz_stride:2;
	 unsigned src1_width:3;
	 unsigned src1_vert_stride:4;
	 unsigned pad0:7;
      } da1;

      struct
      {
	 unsigned src1_swz_x:2;
	 unsigned src1_swz_y:2;
	 unsigned src1_subreg_nr:1;
	 unsigned src1_reg_nr:8;
	 unsigned src1_abs:1;
	 unsigned src1_negate:1;
	 unsigned pad0:1;
	 unsigned src1_swz_z:2;
	 unsigned src1_swz_w:2;
	 unsigned pad1:1;
	 unsigned src1_vert_stride:4;
	 unsigned pad2:7;
      } da16;

      struct
      {
	 int  src1_indirect_offset:10;
	 unsigned src1_subreg_nr:3;
	 unsigned src1_abs:1;
	 unsigned src1_negate:1;
	 unsigned pad0:1;
	 unsigned src1_horiz_stride:2;
	 unsigned src1_width:3;
	 unsigned src1_vert_stride:4;
	 unsigned flag_reg_nr:1;
	 unsigned pad1:6;
      } ia1;

      struct
      {
	 unsigned src1_swz_x:2;
	 unsigned src1_swz_y:2;
	 int  src1_indirect_offset:6;
	 unsigned src1_subreg_nr:3;
	 unsigned src1_abs:1;
	 unsigned src1_negate:1;
	 unsigned pad0:1;
	 unsigned src1_swz_z:2;
	 unsigned src1_swz_w:2;
	 unsigned pad1:1;
	 unsigned src1_vert_stride:4;
	 unsigned flag_reg_nr:1;
	 unsigned pad2:6;
      } ia16;


      struct
      {
	 int  jump_count:16;	/* note: signed */
	 unsigned  pop_count:4;
	 unsigned  pad0:12;
      } if_else;

      struct {
	 unsigned function:4;
	 unsigned int_type:1;
	 unsigned precision:1;
	 unsigned saturate:1;
	 unsigned data_type:1;
	 unsigned pad0:8;
	 unsigned response_length:4;
	 unsigned msg_length:4;
	 unsigned msg_target:4;
	 unsigned pad1:3;
	 unsigned end_of_thread:1;
      } math;

      struct {
	 unsigned binding_table_index:8;
	 unsigned sampler:4;
	 unsigned return_format:2;
	 unsigned msg_type:2;
	 unsigned response_length:4;
	 unsigned msg_length:4;
	 unsigned msg_target:4;
	 unsigned pad1:3;
	 unsigned end_of_thread:1;
      } sampler;

      struct brw_urb_immediate urb;

      struct {
	 unsigned binding_table_index:8;
	 unsigned msg_control:4;
	 unsigned msg_type:2;
	 unsigned target_cache:2;
	 unsigned response_length:4;
	 unsigned msg_length:4;
	 unsigned msg_target:4;
	 unsigned pad1:3;
	 unsigned end_of_thread:1;
      } dp_read;

      struct {
	 unsigned binding_table_index:8;
	 unsigned msg_control:3;
	 unsigned pixel_scoreboard_clear:1;
	 unsigned msg_type:3;
	 unsigned send_commit_msg:1;
	 unsigned response_length:4;
	 unsigned msg_length:4;
	 unsigned msg_target:4;
	 unsigned pad1:3;
	 unsigned end_of_thread:1;
      } dp_write;

      struct {
	 unsigned pad:16;
	 unsigned response_length:4;
	 unsigned msg_length:4;
	 unsigned msg_target:4;
	 unsigned pad1:3;
	 unsigned end_of_thread:1;
      } generic;

      int d;
      unsigned ud;
   } bits3;
};


#endif

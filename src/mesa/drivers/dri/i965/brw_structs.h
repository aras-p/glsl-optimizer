/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics to
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
  *   Keith Whitwell <keithw@vmware.com>
  */


#ifndef BRW_STRUCTS_H
#define BRW_STRUCTS_H

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
      unsigned cs_fence:11;
      unsigned pad:1;
   } bits1;
};

/* State structs for the various fixed function units:
 */


struct thread0
{
   unsigned pad0:1;
   unsigned grf_reg_count:3;
   unsigned pad1:2;
   unsigned kernel_start_pointer:26; /* Offset from GENERAL_STATE_BASE */
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
      unsigned max_threads:5; 	/* may be less */
      unsigned pad3:2;
   } thread4;

   struct
   {
      unsigned pad0:13;
      unsigned clip_mode:3;
      unsigned userclip_enable_flags:8;
      unsigned userclip_must_clip:1;
      unsigned negative_w_clip_test:1;
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

struct gen6_blend_state
{
   struct {
      unsigned dest_blend_factor:5;
      unsigned source_blend_factor:5;
      unsigned pad3:1;
      unsigned blend_func:3;
      unsigned pad2:1;
      unsigned ia_dest_blend_factor:5;
      unsigned ia_source_blend_factor:5;
      unsigned pad1:1;
      unsigned ia_blend_func:3;
      unsigned pad0:1;
      unsigned ia_blend_enable:1;
      unsigned blend_enable:1;
   } blend0;

   struct {
      unsigned post_blend_clamp_enable:1;
      unsigned pre_blend_clamp_enable:1;
      unsigned clamp_range:2;
      unsigned pad0:4;
      unsigned x_dither_offset:2;
      unsigned y_dither_offset:2;
      unsigned dither_enable:1;
      unsigned alpha_test_func:3;
      unsigned alpha_test_enable:1;
      unsigned pad1:1;
      unsigned logic_op_func:4;
      unsigned logic_op_enable:1;
      unsigned pad2:1;
      unsigned write_disable_b:1;
      unsigned write_disable_g:1;
      unsigned write_disable_r:1;
      unsigned write_disable_a:1;
      unsigned pad3:1;
      unsigned alpha_to_coverage_dither:1;
      unsigned alpha_to_one:1;
      unsigned alpha_to_coverage:1;
   } blend1;
};

struct gen6_color_calc_state
{
   struct {
      unsigned alpha_test_format:1;
      unsigned pad0:14;
      unsigned round_disable:1;
      unsigned bf_stencil_ref:8;
      unsigned stencil_ref:8;
   } cc0;

   union {
      float alpha_ref_f;
      struct {
	 unsigned ui:8;
	 unsigned pad0:24;
      } alpha_ref_fi;
   } cc1;

   float constant_r;
   float constant_g;
   float constant_b;
   float constant_a;
};

struct gen6_depth_stencil_state
{
   struct {
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
   } ds0;

   struct {
      unsigned bf_stencil_write_mask:8;
      unsigned bf_stencil_test_mask:8;
      unsigned stencil_write_mask:8;
      unsigned stencil_test_mask:8;
   } ds1;

   struct {
      unsigned pad0:26;
      unsigned depth_write_enable:1;
      unsigned depth_test_func:3;
      unsigned pad1:1;
      unsigned depth_test_enable:1;
   } ds2;
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
      unsigned cc_viewport_state_offset:27; /* Offset from GENERAL_STATE_BASE */
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
	 uint8_t ub[4];
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
      unsigned sf_viewport_state_offset:27; /* Offset from GENERAL_STATE_BASE */
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
      unsigned pad0:10;
      unsigned aa_line_distance_mode:1;
      unsigned trifan_pv:2;
      unsigned linestrip_pv:2;
      unsigned tristrip_pv:2;
      unsigned line_last_pixel_enable:1;
   } sf7;

};

struct gen6_scissor_rect
{
   unsigned xmin:16;
   unsigned ymin:16;
   unsigned xmax:16;
   unsigned ymax:16;
};

struct brw_gs_unit_state
{
   struct thread0 thread0;
   struct thread1 thread1;
   struct thread2 thread2;
   struct thread3 thread3;

   struct
   {
      unsigned pad0:8;
      unsigned rendering_enable:1; /* for Ironlake */
      unsigned pad4:1;
      unsigned stats_enable:1;
      unsigned nr_urb_entries:7;
      unsigned pad1:1;
      unsigned urb_entry_allocation_size:5;
      unsigned pad2:1;
      unsigned max_threads:5;
      unsigned pad3:2;
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
      unsigned pad0:12;
      unsigned svbi_post_inc_value:10;
      unsigned pad1:1;
      unsigned svbi_post_inc_enable:1;
      unsigned svbi_payload:1;
      unsigned discard_adjaceny:1;
      unsigned reorder_enable:1;
      unsigned pad2:1;
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
      unsigned max_threads:6;
      unsigned pad3:1;
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
      unsigned depth_buffer_clear:1;
      unsigned sampler_count:3;
      unsigned sampler_state_pointer:27;
   } wm4;

   struct
   {
      unsigned enable_8_pix:1;
      unsigned enable_16_pix:1;
      unsigned enable_32_pix:1;
      unsigned enable_con_32_pix:1;
      unsigned enable_con_64_pix:1;
      unsigned pad0:1;

      /* These next four bits are for Ironlake+ */
      unsigned fast_span_coverage_enable:1;
      unsigned depth_buffer_clear:1;
      unsigned depth_buffer_resolve_enable:1;
      unsigned hierarchical_depth_buffer_resolve_enable:1;

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
      unsigned transposed_urb_read_enable:1;
      unsigned max_threads:7;
   } wm5;

   float global_depth_offset_constant;
   float global_depth_offset_scale;

   /* for Ironlake only */
   struct {
      unsigned pad0:1;
      unsigned grf_reg_count_1:3;
      unsigned pad1:2;
      unsigned kernel_start_pointer_1:26;
   } wm8;

   struct {
      unsigned pad0:1;
      unsigned grf_reg_count_2:3;
      unsigned pad1:2;
      unsigned kernel_start_pointer_2:26;
   } wm9;

   struct {
      unsigned pad0:1;
      unsigned grf_reg_count_3:3;
      unsigned pad1:2;
      unsigned kernel_start_pointer_3:26;
   } wm10;
};

struct brw_sampler_default_color {
   float color[4];
};

struct gen5_sampler_default_color {
   uint8_t ub[4];
   float f[4];
   uint16_t hf[4];
   uint16_t us[4];
   int16_t s[4];
   uint8_t b[4];
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
      unsigned min_mag_neq:1;
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
      unsigned cube_control_mode:1;
      unsigned pad:2;
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
      unsigned non_normalized_coord:1;
      unsigned pad:12;
      unsigned address_round:6;
      unsigned max_aniso:3;
      unsigned chroma_key_mode:1;
      unsigned chroma_key_index:2;
      unsigned chroma_key_enable:1;
      unsigned monochrome_filter_width:3;
      unsigned monochrome_filter_height:3;
   } ss3;
};

struct gen7_sampler_state
{
   struct
   {
      unsigned aniso_algorithm:1;
      unsigned lod_bias:13;
      unsigned min_filter:3;
      unsigned mag_filter:3;
      unsigned mip_filter:2;
      unsigned base_level:5;
      unsigned pad1:1;
      unsigned lod_preclamp:1;
      unsigned default_color_mode:1;
      unsigned pad0:1;
      unsigned disable:1;
   } ss0;

   struct
   {
      unsigned cube_control_mode:1;
      unsigned shadow_function:3;
      unsigned pad:4;
      unsigned max_lod:12;
      unsigned min_lod:12;
   } ss1;

   struct
   {
      unsigned pad:5;
      unsigned default_color_pointer:27;
   } ss2;

   struct
   {
      unsigned r_wrap_mode:3;
      unsigned t_wrap_mode:3;
      unsigned s_wrap_mode:3;
      unsigned pad:1;
      unsigned non_normalized_coord:1;
      unsigned trilinear_quality:2;
      unsigned address_round:6;
      unsigned max_aniso:3;
      unsigned chroma_key_mode:1;
      unsigned chroma_key_index:2;
      unsigned chroma_key_enable:1;
      unsigned pad0:6;
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

   /* scissor coordinates are inclusive */
   struct {
      int16_t xmin;
      int16_t ymin;
      int16_t xmax;
      int16_t ymax;
   } scissor;
};

struct gen6_sf_viewport {
   float m00;
   float m11;
   float m22;
   float m30;
   float m31;
   float m32;
};

struct gen7_sf_clip_viewport {
   struct {
      float m00;
      float m11;
      float m22;
      float m30;
      float m31;
      float m32;
   } viewport;

   unsigned pad0[2];

   struct {
      float xmin;
      float xmax;
      float ymin;
      float ymax;
   } guardband;

   float pad1[4];
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
      unsigned compression_control:2; /* gen6: quarter control */
      unsigned thread_control:2;
      unsigned predicate_control:4;
      unsigned predicate_inverse:1;
      unsigned execution_size:3;
      /**
       * Conditional Modifier for most instructions.  On Gen6+, this is also
       * used for the SEND instruction's Message Target/SFID.
       */
      unsigned destreg__conditionalmod:4;
      unsigned acc_wr_control:1;
      unsigned cmpt_control:1;
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
         unsigned nibctrl:1; /* gen7+ */
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
	 unsigned src1_reg_file:2;        /* 0x00000c00 */
	 unsigned src1_reg_type:3;        /* 0x00007000 */
         unsigned nibctrl:1; /* gen7+ */
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
         unsigned nibctrl:1; /* gen7+ */
	 unsigned dest_writemask:4;
	 unsigned dest_subreg_nr:1;
	 unsigned dest_reg_nr:8;
	 unsigned dest_horiz_stride:2;
	 unsigned dest_address_mode:1;
      } da16;

      struct
      {
	 unsigned dest_reg_file:2;
	 unsigned dest_reg_type:3;
	 unsigned src0_reg_file:2;
	 unsigned src0_reg_type:3;
         unsigned src1_reg_file:2;
         unsigned src1_reg_type:3;
         unsigned nibctrl:1; /* gen7+ */
	 unsigned dest_writemask:4;
	 int dest_indirect_offset:6;
	 unsigned dest_subreg_nr:3;
	 unsigned dest_horiz_stride:2;
	 unsigned dest_address_mode:1;
      } ia16;

      struct {
	 unsigned dest_reg_file:2;
	 unsigned dest_reg_type:3;
	 unsigned src0_reg_file:2;
	 unsigned src0_reg_type:3;
	 unsigned src1_reg_file:2;
	 unsigned src1_reg_type:3;
	 unsigned pad:1;

	 int jump_count:16;
      } branch_gen6;

      struct {
         unsigned dest_reg_file:1; /* gen6, not gen7+ */
	 unsigned flag_subreg_num:1;
         unsigned flag_reg_nr:1; /* gen7+ */
         unsigned pad0:1;
	 unsigned src0_abs:1;
	 unsigned src0_negate:1;
	 unsigned src1_abs:1;
	 unsigned src1_negate:1;
	 unsigned src2_abs:1;
	 unsigned src2_negate:1;
         unsigned src_type:2; /* gen7+ */
         unsigned dst_type:2; /* gen7+ */
         unsigned pad1:1;
         unsigned nibctrl:1; /* gen7+ */
         unsigned pad2:1;
	 unsigned dest_writemask:4;
	 unsigned dest_subreg_nr:3;
	 unsigned dest_reg_nr:8;
      } da3src;

      uint32_t ud;
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
	 unsigned flag_subreg_nr:1;
         unsigned flag_reg_nr:1; /* gen7+ */
	 unsigned pad:5;
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
	 unsigned flag_subreg_nr:1;
         unsigned flag_reg_nr:1; /* gen7+ */
	 unsigned pad:5;
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
	 unsigned flag_subreg_nr:1;
         unsigned flag_reg_nr:1; /* gen7+ */
	 unsigned pad1:5;
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
	 unsigned flag_subreg_nr:1;
         unsigned flag_reg_nr:1; /* gen7+ */
	 unsigned pad1:5;
      } ia16;

      /* Extended Message Descriptor for Ironlake (Gen5) SEND instruction.
       *
       * Does not apply to Gen6+.  The SFID/message target moved to bits
       * 27:24 of the header (destreg__conditionalmod); EOT is in bits3.
       */
       struct
       {
           unsigned pad:26;
           unsigned end_of_thread:1;
           unsigned pad1:1;
           unsigned sfid:4;
       } send_gen5;  /* for Ironlake only */

      struct {
	 unsigned src0_rep_ctrl:1;
	 unsigned src0_swizzle:8;
	 unsigned src0_subreg_nr:3;
	 unsigned src0_reg_nr:8;
	 unsigned pad0:1;
	 unsigned src1_rep_ctrl:1;
	 unsigned src1_swizzle:8;
	 unsigned src1_subreg_nr_low:2;
      } da3src;

      uint32_t ud;
   } bits2;

   union
   {
      struct
      {
	 unsigned src1_subreg_nr:5;
	 unsigned src1_reg_nr:8;
	 unsigned src1_abs:1;
	 unsigned src1_negate:1;
	 unsigned src1_address_mode:1;
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
	 unsigned src1_address_mode:1;
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
	 unsigned src1_address_mode:1;
	 unsigned src1_horiz_stride:2;
	 unsigned src1_width:3;
	 unsigned src1_vert_stride:4;
	 unsigned pad1:7;
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
	 unsigned pad2:7;
      } ia16;


      struct
      {
	 int  jump_count:16;	/* note: signed */
	 unsigned  pop_count:4;
	 unsigned  pad0:12;
      } if_else;

      /* This is also used for gen7 IF/ELSE instructions */
      struct
      {
	 /* Signed jump distance to the ip to jump to if all channels
	  * are disabled after the break or continue.  It should point
	  * to the end of the innermost control flow block, as that's
	  * where some channel could get re-enabled.
	  */
	 int jip:16;

	 /* Signed jump distance to the location to resume execution
	  * of this channel if it's enabled for the break or continue.
	  */
	 int uip:16;
      } break_cont;

      /**
       * \defgroup SEND instructions / Message Descriptors
       *
       * @{
       */

      /**
       * Generic Message Descriptor for Gen4 SEND instructions.  The structs
       * below expand function_control to something specific for their
       * message.  Due to struct packing issues, they duplicate these bits.
       *
       * See the G45 PRM, Volume 4, Table 14-15.
       */
      struct {
	 unsigned function_control:16;
	 unsigned response_length:4;
	 unsigned msg_length:4;
	 unsigned msg_target:4;
	 unsigned pad1:3;
	 unsigned end_of_thread:1;
      } generic;

      /**
       * Generic Message Descriptor for Gen5-7 SEND instructions.
       *
       * See the Sandybridge PRM, Volume 2 Part 2, Table 8-15.  (Sadly, most
       * of the information on the SEND instruction is missing from the public
       * Ironlake PRM.)
       *
       * The table claims that bit 31 is reserved/MBZ on Gen6+, but it lies.
       * According to the SEND instruction description:
       * "The MSb of the message description, the EOT field, always comes from
       *  bit 127 of the instruction word"...which is bit 31 of this field.
       */
      struct {
	 unsigned function_control:19;
	 unsigned header_present:1;
	 unsigned response_length:5;
	 unsigned msg_length:4;
	 unsigned pad1:2;
	 unsigned end_of_thread:1;
      } generic_gen5;

      /** G45 PRM, Volume 4, Section 6.1.1.1 */
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

      /** Ironlake PRM, Volume 4 Part 1, Section 6.1.1.1 */
      struct {
	 unsigned function:4;
	 unsigned int_type:1;
	 unsigned precision:1;
	 unsigned saturate:1;
	 unsigned data_type:1;
	 unsigned snapshot:1;
	 unsigned pad0:10;
	 unsigned header_present:1;
	 unsigned response_length:5;
	 unsigned msg_length:4;
	 unsigned pad1:2;
	 unsigned end_of_thread:1;
      } math_gen5;

      /** G45 PRM, Volume 4, Section 4.8.1.1.1 [DevBW] and [DevCL] */
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

      /** G45 PRM, Volume 4, Section 4.8.1.1.2 [DevCTG] */
      struct {
         unsigned binding_table_index:8;
         unsigned sampler:4;
         unsigned msg_type:4;
         unsigned response_length:4;
         unsigned msg_length:4;
         unsigned msg_target:4;
         unsigned pad1:3;
         unsigned end_of_thread:1;
      } sampler_g4x;

      /** Ironlake PRM, Volume 4 Part 1, Section 4.11.1.1.3 */
      struct {
	 unsigned binding_table_index:8;
	 unsigned sampler:4;
	 unsigned msg_type:4;
	 unsigned simd_mode:2;
	 unsigned pad0:1;
	 unsigned header_present:1;
	 unsigned response_length:5;
	 unsigned msg_length:4;
	 unsigned pad1:2;
	 unsigned end_of_thread:1;
      } sampler_gen5;

      struct {
	 unsigned binding_table_index:8;
	 unsigned sampler:4;
	 unsigned msg_type:5;
	 unsigned simd_mode:2;
	 unsigned header_present:1;
	 unsigned response_length:5;
	 unsigned msg_length:4;
	 unsigned pad1:2;
	 unsigned end_of_thread:1;
      } sampler_gen7;

      struct brw_urb_immediate urb;

      struct {
	 unsigned opcode:4;
	 unsigned offset:6;
	 unsigned swizzle_control:2;
	 unsigned pad:1;
	 unsigned allocate:1;
	 unsigned used:1;
	 unsigned complete:1;
	 unsigned pad0:3;
	 unsigned header_present:1;
	 unsigned response_length:5;
	 unsigned msg_length:4;
	 unsigned pad1:2;
	 unsigned end_of_thread:1;
      } urb_gen5;

      struct {
	 unsigned opcode:3;
	 unsigned offset:11;
	 unsigned swizzle_control:1;
	 unsigned complete:1;
	 unsigned per_slot_offset:1;
	 unsigned pad0:2;
	 unsigned header_present:1;
	 unsigned response_length:5;
	 unsigned msg_length:4;
	 unsigned pad1:2;
	 unsigned end_of_thread:1;
      } urb_gen7;

      /** 965 PRM, Volume 4, Section 5.10.1.1: Message Descriptor */
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

      /** G45 PRM, Volume 4, Section 5.10.1.1.2 */
      struct {
	 unsigned binding_table_index:8;
	 unsigned msg_control:3;
	 unsigned msg_type:3;
	 unsigned target_cache:2;
	 unsigned response_length:4;
	 unsigned msg_length:4;
	 unsigned msg_target:4;
	 unsigned pad1:3;
	 unsigned end_of_thread:1;
      } dp_read_g4x;

      /** Ironlake PRM, Volume 4 Part 1, Section 5.10.2.1.2. */
      struct {
	 unsigned binding_table_index:8;
	 unsigned msg_control:3;
	 unsigned msg_type:3;
	 unsigned target_cache:2;
	 unsigned pad0:3;
	 unsigned header_present:1;
	 unsigned response_length:5;
	 unsigned msg_length:4;
	 unsigned pad1:2;
	 unsigned end_of_thread:1;
      } dp_read_gen5;

      /** G45 PRM, Volume 4, Section 5.10.1.1.2.  For both Gen4 and G45. */
      struct {
	 unsigned binding_table_index:8;
	 unsigned msg_control:3;
	 unsigned last_render_target:1;
	 unsigned msg_type:3;
	 unsigned send_commit_msg:1;
	 unsigned response_length:4;
	 unsigned msg_length:4;
	 unsigned msg_target:4;
	 unsigned pad1:3;
	 unsigned end_of_thread:1;
      } dp_write;

      /** Ironlake PRM, Volume 4 Part 1, Section 5.10.2.1.2. */
      struct {
	 unsigned binding_table_index:8;
	 unsigned msg_control:3;
	 unsigned last_render_target:1;
	 unsigned msg_type:3;
	 unsigned send_commit_msg:1;
	 unsigned pad0:3;
	 unsigned header_present:1;
	 unsigned response_length:5;
	 unsigned msg_length:4;
	 unsigned pad1:2;
	 unsigned end_of_thread:1;
      } dp_write_gen5;

      /**
       * Message for the Sandybridge Sampler Cache or Constant Cache Data Port.
       *
       * See the Sandybridge PRM, Volume 4 Part 1, Section 3.9.2.1.1.
       **/
      struct {
	 unsigned binding_table_index:8;
	 unsigned msg_control:5;
	 unsigned msg_type:3;
	 unsigned pad0:3;
	 unsigned header_present:1;
	 unsigned response_length:5;
	 unsigned msg_length:4;
	 unsigned pad1:2;
	 unsigned end_of_thread:1;
      } gen6_dp_sampler_const_cache;

      /**
       * Message for the Sandybridge Render Cache Data Port.
       *
       * Most fields are defined in the Sandybridge PRM, Volume 4 Part 1,
       * Section 3.9.2.1.1: Message Descriptor.
       *
       * "Slot Group Select" and "Last Render Target" are part of the
       * 5-bit message control for Render Target Write messages.  See
       * Section 3.9.9.2.1 of the same volume.
       */
      struct {
	 unsigned binding_table_index:8;
	 unsigned msg_control:3;
	 unsigned slot_group_select:1;
	 unsigned last_render_target:1;
	 unsigned msg_type:4;
	 unsigned send_commit_msg:1;
	 unsigned pad0:1;
	 unsigned header_present:1;
	 unsigned response_length:5;
	 unsigned msg_length:4;
	 unsigned pad1:2;
	 unsigned end_of_thread:1;
      } gen6_dp;

      /**
       * Message for any of the Gen7 Data Port caches.
       *
       * Most fields are defined in the Ivybridge PRM, Volume 4 Part 1,
       * section 3.9.2.1.1 "Message Descriptor".  Once again, "Slot Group
       * Select" and "Last Render Target" are part of the 6-bit message
       * control for Render Target Writes (section 3.9.11.2).
       */
      struct {
	 unsigned binding_table_index:8;
	 unsigned msg_control:3;
	 unsigned slot_group_select:1;
	 unsigned last_render_target:1;
	 unsigned msg_control_pad:1;
	 unsigned msg_type:4;
	 unsigned pad1:1;
	 unsigned header_present:1;
	 unsigned response_length:5;
	 unsigned msg_length:4;
	 unsigned pad2:2;
	 unsigned end_of_thread:1;
      } gen7_dp;

      /**
       * Message for the Gen7 Pixel Interpolator.
       *
       * Defined in the Ivybridge PRM, Volume 4 Part 2,
       * section 4.1.1.1.
       */
      struct {
         GLuint msg_data:8;
         GLuint pad1:3;
         GLuint slot_group:1;
         GLuint msg_type:2;
         GLuint interpolation_mode:1;
         GLuint pad2:1;
         GLuint simd_mode:1;
         GLuint pad3:1;
         GLuint response_length:5;
         GLuint msg_length:4;
         GLuint pad4:2;
         GLuint end_of_thread:1;
      } gen7_pi;
      /** @} */

      struct {
	 unsigned src1_subreg_nr_high:1;
	 unsigned src1_reg_nr:8;
	 unsigned pad0:1;
	 unsigned src2_rep_ctrl:1;
	 unsigned src2_swizzle:8;
	 unsigned src2_subreg_nr:3;
	 unsigned src2_reg_nr:8;
	 unsigned pad1:2;
      } da3src;

      int d;
      unsigned ud;
      float f;
   } bits3;
};

struct brw_compact_instruction {
   struct {
      unsigned opcode:7;          /*  0- 6 */
      unsigned debug_control:1;   /*  7- 7 */
      unsigned control_index:5;   /*  8-12 */
      unsigned data_type_index:5; /* 13-17 */
      unsigned sub_reg_index:5;   /* 18-22 */
      unsigned acc_wr_control:1;  /* 23-23 */
      unsigned conditionalmod:4;  /* 24-27 */
      unsigned flag_subreg_nr:1;     /* 28-28 */
      unsigned cmpt_ctrl:1;       /* 29-29 */
      unsigned src0_index:2;      /* 30-31 */
   } dw0;

   struct {
      unsigned src0_index:3;  /* 32-24 */
      unsigned src1_index:5;  /* 35-39 */
      unsigned dst_reg_nr:8;  /* 40-47 */
      unsigned src0_reg_nr:8; /* 48-55 */
      unsigned src1_reg_nr:8; /* 56-63 */
   } dw1;
};

#endif

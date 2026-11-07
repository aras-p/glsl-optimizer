/*
 * Copyright © 2014 Connor Abbott
 * SPDX-License-Identifier: MIT
 */

#ifndef NIR_SHADER_COMPILER_OPTIONS_H
#define NIR_SHADER_COMPILER_OPTIONS_H

#include "util/macros.h"
#include "nir_defines.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
   nir_lower_imul64 = (1 << 0),
   nir_lower_isign64 = (1 << 1),
   /** Lower all int64 modulus and division opcodes */
   nir_lower_divmod64 = (1 << 2),
   /** Lower all 64-bit umul_high and imul_high opcodes */
   nir_lower_imul_high64 = (1 << 3),
   nir_lower_bcsel64 = (1 << 4),
   nir_lower_icmp64 = (1 << 5),
   nir_lower_iadd64 = (1 << 6),
   nir_lower_iabs64 = (1 << 7),
   nir_lower_ineg64 = (1 << 8),
   nir_lower_logic64 = (1 << 9),
   nir_lower_minmax64 = (1 << 10),
   nir_lower_shift64 = (1 << 11),
   nir_lower_imul_2x32_64 = (1 << 12),
   nir_lower_extract64 = (1 << 13),
   nir_lower_ufind_msb64 = (1 << 14),
   nir_lower_bit_count64 = (1 << 15),
   nir_lower_subgroup_shuffle64 = (1 << 16),
   nir_lower_scan_reduce_bitwise64 = (1 << 17),
   nir_lower_scan_reduce_iadd64 = (1 << 18),
   nir_lower_vote_ieq64 = (1 << 19),
   nir_lower_usub_sat64 = (1 << 20),
   nir_lower_iadd_sat64 = (1 << 21),
   nir_lower_find_lsb64 = (1 << 22),
   nir_lower_conv64 = (1 << 23),
   nir_lower_uadd_sat64 = (1 << 24),
   nir_lower_iadd3_64 = (1 << 25),
   nir_lower_bitfield_reverse64 = (1 << 26),
   nir_lower_bitfield_extract64 = (1 << 27),
} nir_lower_int64_options;

typedef enum {
   nir_lower_drcp = (1 << 0),
   nir_lower_dsqrt = (1 << 1),
   nir_lower_drsq = (1 << 2),
   nir_lower_dtrunc = (1 << 3),
   nir_lower_dfloor = (1 << 4),
   nir_lower_dceil = (1 << 5),
   nir_lower_dfract = (1 << 6),
   nir_lower_dround_even = (1 << 7),
   nir_lower_dmod = (1 << 8),
   nir_lower_dsub = (1 << 9),
   nir_lower_ddiv = (1 << 10),
   nir_lower_dsign = (1 << 11),
   nir_lower_dminmax = (1 << 12),
   nir_lower_dsat = (1 << 13),
   nir_lower_fp64_full_software = (1 << 14),
} nir_lower_doubles_options;

typedef enum {
   nir_divergence_single_prim_per_subgroup = (1 << 0),
   nir_divergence_single_patch_per_tcs_subgroup = (1 << 1),
   nir_divergence_single_patch_per_tes_subgroup = (1 << 2),
   nir_divergence_view_index_uniform = (1 << 3),
   nir_divergence_single_frag_shading_rate_per_subgroup = (1 << 4),
   nir_divergence_multiple_workgroup_per_compute_subgroup = (1 << 5),
   nir_divergence_shader_record_ptr_uniform = (1 << 6),
   nir_divergence_uniform_load_tears = (1 << 7),
   /* If used, this allows phis for divergent merges with undef and a uniform source to be considered uniform */
   nir_divergence_ignore_undef_if_phi_srcs = (1 << 8),

   /* Whether to compute vertex divergence (meaning between vertices
    * of the same primitive) instead of subgroup invocation divergence
    * (between invocations of the same subgroup). For example, patch input
    * loads are always convergent, while subgroup intrinsics are divergent.
    */
   nir_divergence_vertex = (1 << 11),

   /* Whether to compute divergence of subgroup operations as if multiple
    * subgroups ran in lock-step (for example subgroup operations normally
    * convergent are divergent).
    */
   nir_divergence_across_subgroups = (1 << 12),

   /* Whether local_invocation_id.z is considered uniform, to be set
    * by the driver based on the subgroup size when the hardware's
    * walk order guarantees that its Z component will be uniform
    * across the desired set of invocations.
    */
   nir_divergence_uniform_local_invocation_id_z = (1 << 13),

   /* Whether InvocationID in TCS is considered uniform */
   nir_divergence_tcs_invocation_id_uniform = (1 << 14),
} nir_divergence_options;

/** An instruction filtering callback
 *
 * Returns true if the instruction should be processed and false otherwise.
 */
typedef bool (*nir_instr_filter_cb)(const nir_instr *, const void *);

typedef enum {
   /**
    * Whether a fragment shader can interpolate the same input multiple times
    * with different modes (smooth, noperspective) and locations (pixel,
    * centroid, sample, at_offset, at_sample), excluding the flat mode.
    *
    * This matches AMD GPU flexibility and limitations and is a superset of
    * the GL4 requirement that each input can be interpolated at its specified
    * location, and then also as centroid, at_offset, and at_sample.
    */
   nir_io_has_flexible_input_interpolation_except_flat = BITFIELD_BIT(0),

   /**
    * nir_opt_varyings compacts (relocates) components of varyings by
    * rewriting their locations completely, effectively moving components of
    * varyings between slots. This option forces nir_opt_varyings to make
    * VARYING_SLOT_POS unused by moving its contents to VARn if the consumer
    * is not FS. If this option is not set and POS is unused, it moves
    * components of VARn to POS until it's fully used.
    */
   nir_io_dont_use_pos_for_non_fs_varyings = BITFIELD_BIT(1),

   nir_io_16bit_input_output_support = BITFIELD_BIT(2),

   /**
    * Implement mediump inputs and outputs as normal 32-bit IO.
    * Causes the mediump flag to be not set for IO semantics, essentially
    * destroying any mediump-related IO information in the shader.
    */
   nir_io_mediump_is_32bit = BITFIELD_BIT(3),

   /**
    * Whether nir_opt_vectorize_io should ignore FS inputs.
    */
   nir_io_prefer_scalar_fs_inputs = BITFIELD_BIT(4),

   /**
    * Whether interpolated fragment shader vec4 slots can use load_input for
    * a subset of its components to skip interpolation for those components.
    * The result of such load_input is a value from a random (not necessarily
    * provoking) vertex. If a value from the provoking vertex is required,
    * the vec4 slot should have no load_interpolated_input instructions.
    *
    * This exposes the AMD capability that allows packing flat inputs with
    * interpolated inputs in a limited number of cases. Normally, flat
    * components must be in a separate vec4 slot to get the value from
    * the provoking vertex. If the compiler can prove that all per-vertex
    * values are equal (convergent, i.e. the provoking vertex doesn't matter),
    * it can put such flat components into any interpolated vec4 slot.
    *
    * It should also be set if the hw can mix flat and interpolated components
    * in the same vec4 slot.
    *
    * This causes nir_opt_varyings to skip interpolation for all varyings
    * that are convergent, and enables better compaction and inter-shader code
    * motion for convergent varyings.
    */
   nir_io_mix_convergent_flat_with_interpolated = BITFIELD_BIT(5),

   /**
    * Whether src_type and dest_type of IO intrinsics are irrelevant and
    * should be ignored by nir_opt_vectorize_io. All drivers that always treat
    * load_input and store_output as untyped and load_interpolated_input as
    * float##bit_size should set this.
    */
   nir_io_vectorizer_ignores_types = BITFIELD_BIT(6),

   /**
    * Whether nir_opt_varyings should never promote convergent FS inputs
    * to flat.
    */
   nir_io_always_interpolate_convergent_fs_inputs = BITFIELD_BIT(7),

   /**
    * Whether the first assigned color channel component should be equal to
    * the first unused VARn component.
    *
    * For example, if the first unused VARn channel is VAR0.z, color channels
    * are assigned in this order:
    *       COL0.z, COL0.w, COL0.x, COL0.y, COL1.z, COL1.w, COL1.x, COL1.y
    *
    * This allows certain drivers to merge outputs if each output sets
    * different components, for example 2 outputs writing VAR0.xy and COL0.z
    * will only use 1 HW output.
    */
   nir_io_compaction_rotates_color_channels = BITFIELD_BIT(8),

   /**
    * Whether to group TES inputs as follows:
    * - inputs used to compute only POS/CLIP outputs are first
    * - inputs used to compute both POS/CLIP outputs and other outputs are next
    * - inputs used to compute only other outputs are last
    */
   nir_io_compaction_groups_tes_inputs_into_pos_and_var_groups = BITFIELD_BIT(9),

   /**
    * RADV expects that high 16 bits of outputs set component >= 4. That's not
    * legal in NIR, but RADV unfortunately relies on it because it's not
    * validated.
    */
   nir_io_radv_intrinsic_component_workaround = BITFIELD_BIT(10),

   /**
    * nir_recompute_io_bases will assign VARYING_SLOT_COL0-1 such that
    * their bases are after all other inputs.
    */
   nir_io_assign_color_input_bases_after_all_other_inputs = BITFIELD_BIT(11),

   /**
    * Whether nir_lower_io should use FRAG_RESULT_DUAL_SRC_BLEND instead of
    * nir_io_semantics::dual_source_blend_index.
    *
    * This has the advantage that it behaves like a normal output and its
    * presence is reflected in shader_info::outputs_written instead of
    * shader_info::fs::color_is_dual_source.
    */
   nir_io_use_frag_result_dual_src_blend = BITFIELD_BIT(12),

   /**
    * Whether the implementation can compact 16-bit values in the higher
    * 32-bits of a varying.
    */
   nir_io_compact_to_higher_16 = BITFIELD_BIT(13),

   /* Options affecting the GLSL compiler or Gallium are below. */

   /**
    * Lower load_deref/store_deref to load_input/store_output/etc. intrinsics.
    * This is only affects GLSL compilation and Gallium.
    */
   nir_io_has_intrinsics = BITFIELD_BIT(16),
} nir_io_options;

typedef enum {
   nir_lower_packing_op_pack_64_2x32,
   nir_lower_packing_op_unpack_64_2x32,
   nir_lower_packing_op_pack_64_4x16,
   nir_lower_packing_op_unpack_64_4x16,
   nir_lower_packing_op_pack_32_2x16,
   nir_lower_packing_op_unpack_32_2x16,
   nir_lower_packing_op_pack_32_4x8,
   nir_lower_packing_op_unpack_32_4x8,
   nir_lower_packing_num_ops,
} nir_lower_packing_op;

typedef enum {
   /* When set: use frag_coord_xy, frag_coord_z, frag_coord_w
    * When unset: use frag_coord
    */
   nir_frag_coord_xy_z_w_separate = BITFIELD_BIT(0),

   /* Use frag_coord_w_rcp instead of frag_coord_w. */
   nir_frag_coord_use_w_rcp = BITFIELD_BIT(1),

   /* Use pixel_coord + (pixel_center_integer ? 0 : 0.5) instead of
    * frag_coord_xy. This is always correct for OpenGL without VRS because
    * even sample shading must have gl_FragCoord at pixel center.
    */
   nir_frag_coord_use_pixel_coord = BITFIELD_BIT(2),
} nir_frag_coord_form;

typedef enum {
   nir_float_muladd_support_has_ffma       = 0x01,
   nir_float_muladd_support_has_fmad       = 0x02,

   /** Strongly hints that fmad or fmul+fadd is preferred over ffma */
   nir_float_muladd_support_prefers_split  = 0x04,

   /** ffma_weak won't be lowered */
   nir_float_muladd_support_keep_weak_ffma = 0x08,

   nir_float_muladd_support_fuse           = 0x10,
} nir_float_muladd_support;
MESA_DEFINE_CPP_ENUM_BITFIELD_OPERATORS(nir_float_muladd_support)

typedef struct nir_shader_compiler_options {
   bool lower_fdiv;
   nir_float_muladd_support float_mul_add16;
   nir_float_muladd_support float_mul_add32;
   nir_float_muladd_support float_mul_add64;
   bool lower_flrp16;
   bool lower_flrp32;
   /** Lowers flrp when it does not support doubles */
   bool lower_flrp64;
   bool lower_fpow;
   bool lower_fsat;
   bool lower_fsqrt;
   bool lower_sincos;
   bool lower_fmod;
   /** Lowers ibitfield_extract/ubitfield_extract for 8, 16 & 32 bits. */
   bool lower_bitfield_extract8;
   bool lower_bitfield_extract16;
   bool lower_bitfield_extract;
   /** Lowers bitfield_insert. */
   bool lower_bitfield_insert;
   /** Lowers bitfield_reverse to shifts. */
   bool lower_bitfield_reverse;
   /** Lowers bit_count to shifts. */
   bool lower_bit_count;
   /** Lowers ifind_msb. */
   bool lower_ifind_msb;
   /** Lowers ufind_msb. */
   bool lower_ufind_msb;
   /** Lowers find_lsb to ufind_msb and logic ops */
   bool lower_find_lsb;
   bool lower_uadd_carry;
   bool lower_usub_borrow;
   /** Lowers imul_high/umul_high to 16-bit multiplies and carry operations. */
   bool lower_mul_high;
   bool lower_mul_high16;
   /** lowers fneg to fmul(x, -1.0). Driver must call nir_opt_algebraic_late() */
   bool lower_fneg;
   /** lowers ineg to isub. Driver must call nir_opt_algebraic_late(). */
   bool lower_ineg;
   /** lowers fisnormal to alu ops. */
   bool lower_fisnormal;

   /* lower {slt,sge,seq,sne} to {flt,fge,feq,fneu} + b2f: */
   bool lower_scmp;

   /** enable rules to avoid bit ops */
   bool lower_bitops;

   /** enables rules to lower isign to imin+imax */
   bool lower_isign;

   /** enables rules to lower fsign to fsub and flt */
   bool lower_fsign;

   /** enables rules to lower iabs to ineg+imax */
   bool lower_iabs;

   /** enable rules that avoid generating umax from signed integer ops */
   bool lower_umax;

   /** enable rules that avoid generating umin from signed integer ops */
   bool lower_umin;

   /* lower fmin/fmax with signed zero preserve to fmin/fmax with
    * no_signed_zero, for backends whose fmin/fmax implementations do not
    * implement IEEE-754-2019 semantics for signed zero.
    */
   bool lower_fminmax_signed_zero;

   /* lower fdph to fdot4 */
   bool lower_fdph;

   /* Does the native fdot instruction replicate its result for four
    * components?  If so, then opt_algebraic_late will turn all fdotN
    * instructions into fdotN_replicated instructions.
    */
   bool fdot_replicates;

   /** lowers ffloor to fsub+ffract: */
   bool lower_ffloor;

   /** lowers ffract to fsub+ffloor: */
   bool lower_ffract;

   /** lowers fceil to fneg+ffloor+fneg: */
   bool lower_fceil;

   bool lower_ftrunc;

   /** Lowers fround_even to ffract+feq+csel.
    *
    * Not correct in that it doesn't correctly handle the "_even" part of the
    * rounding, but good enough for DX9 array indexing handling on DX9-class
    * hardware.
    */
   bool lower_fround_even;

   bool lower_pack_half_2x16;
   bool lower_pack_unorm_2x16;
   bool lower_pack_snorm_2x16;
   bool lower_pack_unorm_4x8;
   bool lower_pack_snorm_4x8;
   bool lower_pack_64_2x32;
   bool lower_pack_64_4x16;
   bool lower_pack_32_2x16;
   bool lower_pack_64_2x32_split;
   bool lower_pack_32_2x16_split;
   bool lower_unpack_half_2x16;
   bool lower_unpack_unorm_2x16;
   bool lower_unpack_snorm_2x16;
   bool lower_unpack_unorm_4x8;
   bool lower_unpack_snorm_4x8;
   bool lower_unpack_64_2x32_split;
   bool lower_unpack_32_2x16_split;

   bool lower_pack_split;

   bool lower_extract_byte;
   bool lower_extract_word;
   bool lower_insert_byte;
   bool lower_insert_word;

   /* Indicates that the driver only has zero-based vertex id */
   bool vertex_id_zero_based;

   /**
    * If enabled, gl_BaseVertex will be lowered as:
    * is_indexed_draw (~0/0) & firstvertex
    */
   bool lower_base_vertex;

   /* Indicates that gl_InstanceIndex already includes base index
    * and doesn't require further lowering.
    */
   bool instance_id_includes_base_index;

   /**
    * If enabled, gl_HelperInvocation will be lowered as:
    *
    * - non-sample-shading: sample_mask_in == 0.
    * - sample shading:     !((1 << sample_id) & sample_mask_in))
    *
    * For this to be correct, it requires that fs.uses_sample_shading is set to
    * true when sample shading is enabled.  This means that you need shader
    * variants to set the flag when Vulkan's
    * VkPipelineMultisampleStateCreateInfo->sampleShadingEnable or GL's
    * glMinSampleshading() are enabled.
    *
    * This depends on some possibly hw implementation details, which may
    * not be true for all hw.  In particular that the FS is only executed
    * for covered samples or for helper invocations.  So, do not blindly
    * enable this option.
    *
    * Note: See also issue #22 in ARB_shader_image_load_store
    */
   bool lower_helper_invocation;

   /**
    * Convert gl_SampleMaskIn to gl_HelperInvocation as follows:
    *
    *   gl_SampleMaskIn == 0 ---> gl_HelperInvocation
    *   gl_SampleMaskIn != 0 ---> !gl_HelperInvocation
    */
   bool optimize_sample_mask_in;

   /**
    * Optimize load_front_face ? a : -a to load_front_face_fsign * a
    */
   bool optimize_load_front_face_fsign;

   /**
    * Optimize boolean reductions of quad broadcasts. This should only be enabled if
    * nir_intrinsic_reduce supports INCLUDE_HELPERS.
    */
   bool optimize_quad_vote_to_reduce;

   bool lower_cs_local_index_to_id;
   bool lower_cs_local_id_to_index;

   /* Prevents lowering global_invocation_id to be in terms of workgroup_id */
   bool has_cs_global_id;

   bool lower_device_index_to_zero;

   /* Set if nir_lower_pntc_ytransform() should invert gl_PointCoord.
    * Either when frame buffer is flipped or GL_POINT_SPRITE_COORD_ORIGIN
    * is GL_LOWER_LEFT.
    */
   bool lower_wpos_pntc;

   /**
    * Set if nir_op_[iu]hadd and nir_op_[iu]rhadd instructions should be
    * lowered to simple arithmetic.
    *
    * If this flag is set, the lowering will be applied to all bit-sizes of
    * these instructions.
    *
    * :c:member:`lower_hadd64`
    */
   bool lower_hadd;

   /**
    * Set if only 64-bit nir_op_[iu]hadd and nir_op_[iu]rhadd instructions
    * should be lowered to simple arithmetic.
    *
    * If this flag is set, the lowering will be applied to only 64-bit
    * versions of these instructions.
    *
    * :c:member:`lower_hadd`
    */
   bool lower_hadd64;

   /**
    * Set if nir_op_uadd_sat should be lowered to simple arithmetic.
    *
    * If this flag is set, the lowering will be applied to all bit-sizes of
    * these instructions.
    */
   bool lower_uadd_sat;

   /**
    * Set if nir_op_usub_sat should be lowered to simple arithmetic.
    *
    * If this flag is set, the lowering will be applied to all bit-sizes of
    * these instructions.
    */
   bool lower_usub_sat;

   /**
    * Set if nir_op_iadd_sat and nir_op_isub_sat should be lowered to simple
    * arithmetic.
    *
    * If this flag is set, the lowering will be applied to all bit-sizes of
    * these instructions.
    */
   bool lower_iadd_sat;

   /**
    * Set if imul_32x16 and umul_32x16 should be lowered to simple
    * arithmetic.
    */
   bool lower_mul_32x16;

   /**
    * Set if bf2f and f2bf should be lowered to arithmetic.
    */
   bool lower_bfloat16_conversions;

   /**
    * Set if f2u_sat (or f2i_sat) is supported for converting from 16-, 32-,
    * or 64-bit float types to 8-, 16-, or 32-bit integer types (with small
    * exceptions).
    *
    * Due to the prevalence of drivers using \c nir_split_conversion for
    * conversions from 64-bit float to 8-bit integer, these flags will not
    * enable generation of f2u_sat from 64-bit float types to 8-bit integer
    * types.
    */
   bool has_f2u_sat;
   bool has_f2i_sat;

   bool vectorize_tess_levels;
   bool lower_to_scalar;
   nir_instr_filter_cb lower_to_scalar_filter;

   /**
    * Disables potentially harmful algebraic transformations for architectures
    * with SIMD-within-a-register semantics.
    *
    * Note, to actually vectorize 16bit instructions, use nir_opt_vectorize()
    * with a suitable callback function.
    */
   bool vectorize_vec2_16bit;

   /**
    * Should the linker unify inputs_read/outputs_written between adjacent
    * shader stages which are linked into a single program?
    */
   bool unify_interfaces;

   /**
    * Whether nir_shader_gather_info ignores INTERP_MODE_NONE.
    */
   bool ignore_none_interpolation_in_sysval_gathering;

   /**
    * Whether nir_lower_io() will lower interpolateAt functions to
    * load_interpolated_input intrinsics.
    *
    * Unlike nir_lower_io_use_interpolated_input_intrinsics this will only
    * lower these functions and leave input load intrinsics untouched.
    */
   bool lower_interpolate_at;

   /* Lowers when 32x32->64 bit multiplication is not supported */
   bool lower_mul_2x32_64;

   /* Indicates that ldexp is supported. */
   bool has_ldexp;

   /* Indicates that urol and uror are supported */
   bool has_rotate8;
   bool has_rotate16;
   bool has_rotate32;

   /** Backend supports shfr */
   bool has_shfr32;

   /** Backend supports ternary addition */
   bool has_iadd3;

   /**
    * Backend supports amul and would like them generated whenever
    * possible. This is stronger than has_imul24 for amul, but does not imply
    * support for imul24.
    */
   bool has_amul;

   /**
    * Backend supports imul24, and would like to use it (when possible)
    * for address/offset calculation.  If true, driver should call
    * nir_lower_amul().  (If not set, amul will automatically be lowered
    * to imul.)
    */
   bool has_imul24;

   /** Backend supports umul24, if not set  umul24 will automatically be lowered
    * to imul with masked inputs */
   bool has_umul24;

   /** Backend supports imul24_relaxed and umul24_relaxed, if not set they will be lowered
    * to imul24, umul24 or imul.
    */
   bool has_mul24_relaxed;

   /** Backend supports umul_16x16. */
   bool has_umul_16x16;

   /** Backend supports 32-bit imad */
   bool has_imad32;

   /** Backend supports umad24, if not set  umad24 will automatically be lowered
    * to imul with masked inputs and iadd */
   bool has_umad24;

   /* Backend supports fused compare against zero and csel */
   bool has_fused_comp_and_csel;
   /* Backend supports fused int eq/ne against zero and csel. */
   bool has_icsel_eqz64;
   bool has_icsel_eqz32;
   bool has_icsel_eqz16;

   /* Backend supports fneo, fequ, fltu, fgeu. */
   bool has_fneo_fcmpu;

   /* Backend supports ford and funord. */
   bool has_ford_funord;

   /** Backend supports fsub, if not set fsub will automatically be lowered to
    * fadd(x, fneg(y)). If true, driver should call nir_opt_algebraic_late(). */
   bool has_fsub;

   /** Backend supports isub, if not set isub will automatically be lowered to
    * iadd(x, ineg(y)). If true, driver should call nir_opt_algebraic_late(). */
   bool has_isub;

   /** Backend supports pack_32_4x8 or pack_32_4x8_split. */
   bool has_pack_32_4x8;

   /** Backend supports nir_load_pixel_coord */
   bool has_pixel_coord;

   /** Backend supports nir_load_texture_scale and prefers it over txs for nir
    * lowerings. */
   bool has_texture_scaling;

   /** Backend supports sdot_4x8_iadd. */
   bool has_sdot_4x8;

   /** Backend supports udot_4x8_uadd. */
   bool has_udot_4x8;

   /** Backend supports sudot_4x8_iadd. */
   bool has_sudot_4x8;

   /** Backend supports sdot_4x8_iadd_sat. */
   bool has_sdot_4x8_sat;

   /** Backend supports udot_4x8_uadd_sat. */
   bool has_udot_4x8_sat;

   /** Backend supports sudot_4x8_iadd_sat. */
   bool has_sudot_4x8_sat;

   /** Backend supports sdot_2x16 and udot_2x16 opcodes. */
   bool has_dot_2x16;

   /** Backend supports bfdot2_bfadd opcode. */
   bool has_bfdot2_bfadd;

   /** Backend supports fmulz (and fmadz if has_fmad) */
   bool has_fmulz;

   /**
    * Backend supports fmulz (and fmadz if has_fmad) but only if
    * FLOAT_CONTROLS_DENORM_PRESERVE_FP32 is not set
    */
   bool has_fmulz_no_denorms;

   /**
    * Backend supports ffmaz but only if
    * FLOAT_CONTROLS_DENORM_PRESERVE_FP32 is not set
    */
   bool has_ffmaz_no_denorms;

   /** Backend supports fcanonicalize, if not set fcanonicalize will be lowered
    * to fmul(a, 1.0)
    */
   bool has_fcanonicalize;

   /** Backend supports 32bit ufind_msb_rev and ifind_msb_rev. */
   bool has_find_msb_rev;

   /** Backend supports pack_half_2x16_rtz_split. */
   bool has_pack_half_2x16_rtz;

   /** Backend supports bitz/bitnz. */
   bool has_bit_test;

   /** Backend supports ubfe/ibfe. */
   bool has_bfe;

   /** Backend supports bfm. */
   bool has_bfm;

   /** Backend supports bfi. */
   bool has_bfi;

   /** Backend supports bitfield_select. */
   bool has_bitfield_select;

   /** Backend supports uclz. */
   bool has_uclz;

   /** Backend support msad_u4x8. */
   bool has_msad;

   /** Backend supports f2e4m3fn_satfn */
   bool has_f2e4m3fn_satfn;

   /** Backend supports load_global_bounded intrinsics. */
   bool has_load_global_bounded;

   /** Backend supports load/store_global_offset*/
   bool has_global_offset;

   /** Backend supports f2i32_rtne opcode. */
   bool has_f2i32_rtne;

   /** Backend supports atomic isub. */
   bool has_atomic_isub;

   /** Backend supports atomic load/store. */
   bool has_atomic_load_store;

   /**
    * Is this the Intel vec4 backend?
    *
    * Used to inhibit algebraic optimizations that are known to be harmful on
    * the Intel vec4 backend.  This is generally applicable to any
    * optimization that might cause more immediate values to be used in
    * 3-source (e.g., ffma and flrp) instructions.
    */
   bool intel_vec4;

   /**
    * For most Intel GPUs, all ternary operations such as FMA and BFE cannot
    * have immediates, so two to three instructions may eventually be needed.
    */
   bool avoid_ternary_with_two_constants;

   /* Avoid using fabs on sources to ternary operations
    * r600 can't use source modifiers for these
    */
   bool avoid_ternary_with_fabs;

   /** Whether 8-bit ALU is supported. */
   bool support_8bit_alu;

   /** Whether 16-bit ALU is supported. */
   bool support_16bit_alu;

   unsigned max_unroll_iterations;
   unsigned max_unroll_iterations_aggressive;
   unsigned max_unroll_iterations_fp64;

   bool lower_uniforms_to_ubo;

   /* Specifies if indirect sampler array access will trigger forced loop
    * unrolling.
    */
   bool force_indirect_unrolling_sampler;

   /* Some older drivers don't support GLSL versions with the concept of flat
    * varyings and also don't support integers. This setting helps us avoid
    * marking varyings as flat and potentially having them changed to ints via
    * varying packing.
    */
   bool no_integers;

   /**
    * Specifies which type of indirectly accessed variables should force
    * loop unrolling.
    */
   nir_variable_mode force_indirect_unrolling;

   bool driver_functions;

   /**
    * If true, the driver will call nir_lower_int64 itself and the frontend
    * should not do so. This may enable better optimization around address
    * modes.
    */
   bool late_lower_int64;
   nir_lower_int64_options lower_int64_options;
   nir_lower_doubles_options lower_doubles_options;
   nir_divergence_options divergence_analysis_options;

   /**
    * The masks of shader stages that support indirect indexing with
    * load_input and store_output intrinsics. It's used by
    * nir_lower_io_passes.
    */
   uint8_t support_indirect_inputs;
   uint8_t support_indirect_outputs;

   /**
    * If set, the maximum MSAA sample count supported -- can hint loop unrolling
    * to optimistically unroll a loop doing txf_ms per sample.
    */
   uint8_t max_samples;

   /**
    * Lower fmulz to `min(abs(a), abs(b)) == 0.0 ? 0.0 : a * b`.
    */
   bool lower_fmulz_with_abs_min;

   /** store the variable offset into the instrinsic range_base instead
    *  of adding it to the image index.
    */
   bool lower_image_offset_to_range_base;

   /** store the variable offset into the instrinsic range_base instead
    *  of adding it to the atomic source
    */
   bool lower_atomic_offset_to_range_base;

   /** Don't convert medium-precision casts (e.g. f2fmp) into concrete
    *  type casts (e.g. f2f16).
    */
   bool preserve_mediump;

   /** lowers fquantize2f16 to alu ops. */
   bool lower_fquantize2f16;

   /** Lower f2f16 to f2f16_rtz when execution mode is not rtne. */
   bool force_f2f16_rtz;

   /** Lower VARYING_SLOT_LAYER in FS to SYSTEM_VALUE_LAYER_ID. */
   bool lower_layer_fs_input_to_sysval;

   /** How nir_build_frag_coord generates frag_coord. */
   nir_frag_coord_form frag_coord_form;

   /** clip/cull distance and tess level arrays use compact semantics */
   bool compact_arrays;

   /**
    * Whether discard gets emitted as nir_intrinsic_demote.
    * Otherwise, nir_intrinsic_terminate is being used.
    */
   bool discard_is_demote;

   /** Whether derivative intrinsics must be scalarized. */
   bool scalarize_ddx;

   /**
    * Whether unspecified derivative intrinsics are always coarse.
    * If this is false, they might be either coarse or fine.
    */
   bool coarse_ddx;

   /**
    * Assign a range of driver locations to per-view outputs, with unique
    * slots for each view. If unset, per-view outputs will be treated
    * similarly to other arrayed IO, and only slots for one view will be
    * assigned. Regardless of this setting, per-view outputs are only assigned
    * slots for one value in var->data.location.
    */
   bool per_view_unique_driver_locations;

   /**
    * Emit nir_intrinsic_store_per_view_output with compacted view indices
    * rather than absolute view indices. When using compacted indices, the Nth
    * index refers to the Nth enabled view, not the Nth absolute view. For
    * example, with view mask 0b1010, compacted index 0 is absolute index 1,
    * and compacted index 1 is absolute index 3. Note that compacted view
    * indices do not correspond directly to gl_ViewIndex.
    *
    * If compact_view_index is unset, per-view indices must be constant before
    * nir_lower_io. This can be guaranteed by calling nir_lower_io_temporaries
    * first.
    */
   bool compact_view_index;

   /** Options determining lowering and behavior of inputs and outputs. */
   nir_io_options io_options;

   /**
    * Bit mask of nir_lower_packing_op to skip lowering some nir ops in
    * nir_lower_packing().
    */
   unsigned skip_lower_packing_ops;

   /** Driver callback where drivers can define how to lower mediump.
    *  Used by nir_lower_io_passes.
    */
   void (*lower_mediump_io)(struct nir_shader *nir);

   /**
    * If driver wishes to control which @convert_alu_types to lower, it
    * can implement this callback.
    */
   bool (*lower_convert_alu_types)(nir_intrinsic_instr *convert_alu_types);

   /**
    * Return the maximum cost of an expression that's written to a shader
    * output that can be moved into the next shader to remove that output.
    *
    * Currently only uniform expressions are moved. A uniform expression is
    * any ALU expression sourcing only constants, uniforms, and UBO loads.
    *
    * Set to NULL or return 0 if you only want to propagate constants from
    * outputs to inputs.
    *
    * Drivers can set the maximum cost based on the types of consecutive
    * shaders or shader BLAKE3s.
    *
    * Drivers should also set "varying_estimate_instr_cost".
    */
   unsigned (*varying_expression_max_cost)(struct nir_shader *consumer,
                                           struct nir_shader *producer);

   /**
    * Return the cost of an instruction that could be moved into the next
    * shader. If the cost of all instructions in an expression is <=
    * varying_expression_max_cost(), the instruction is moved.
    *
    * When this callback isn't set, nir_opt_varyings uses its own version.
    */
   unsigned (*varying_estimate_instr_cost)(struct nir_instr *instr);

   /**
    * When the varying_expression_max_cost callback isn't set, this specifies
    * the maximum cost of a uniform expression that is allowed to be moved
    * from output stores into the next shader stage to eliminate those output
    * stores and corresponding inputs.
    *
    * 0 only allows propagating constants written to output stores to
    * the next shader.
    *
    * At least 2 is required for moving a uniform stored in an output into
    * the next shader according to default_varying_estimate_instr_cost.
    */
   unsigned max_varying_expression_cost;

   /**
    * Used by nir_lower_explicit_io to determine the maximum offset_shift to
    * use when lowering the deref address of the given intrinsic.
    */
   unsigned (*max_offset_shift)(nir_intrinsic_instr *, const void *);

   /**
    * Passed to the callbacks that accept a data pointer.
    */
   const void *cb_data;

   /** Maximum amount of invocations per workgroup. */
   unsigned max_workgroup_invocations;

   /** Maximum compute shader / kernel dispatchable work size. */
   unsigned max_workgroup_count[3];
} nir_shader_compiler_options;

#ifdef __cplusplus
}
#endif

#endif /* NIR_SHADER_COMPILER_OPTIONS_H */

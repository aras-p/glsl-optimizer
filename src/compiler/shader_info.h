/*
 * Copyright © 2016 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */

#ifndef SHADER_INFO_H
#define SHADER_INFO_H

#include "util/bitset.h"
/* glsl-optimizer change: removed include of util/mesa-blake3.h */
#include "shader_enums.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_XFB_BUFFERS        4
#define MAX_INLINABLE_UNIFORMS 4

enum shader_info_hash_type {
   SHADER_INFO_HASH_TYPE_RAW = 0,
   SHADER_INFO_HASH_TYPE_DXIL,
   SHADER_INFO_HASH_TYPE_DXBC,
};

typedef struct shader_info {
   const char *name;

   /* Descriptive name provided by the client; may be NULL */
   const char *label;

   /* Shader is internal, and should be ignored by things like NIR_DEBUG=print */
   bool internal;

   /* glsl-optimizer change: no source hashes (blake3) */

   /* Human-readable description of specialization constants passed to shader. */
   const char *spec;

   /** The shader stage, such as MESA_SHADER_VERTEX. */
   mesa_shader_stage stage:8;

   /* If the shader is linked, this is the previous shader, else MESA_SHADER_NONE. */
   mesa_shader_stage prev_stage:8;

   /* If the shader is linked, this is the next shader, else MESA_SHADER_NONE. */
   mesa_shader_stage next_stage:8;

   /* Whether the previous stage has XFB if the shader is linked (prev_stage != NONE). */
   bool prev_stage_has_xfb;

   /* Number of textures used by this shader */
   uint8_t num_textures;
   /* Number of uniform buffers used by this shader */
   uint8_t num_ubos;
   /* Number of atomic buffers used by this shader */
   uint8_t num_abos;
   /* Number of shader storage buffers (max .driver_location + 1) used by this
    * shader.  In the case of nir_lower_atomics_to_ssbo being used, this will
    * be the number of actual SSBOs in gl_program->info, and the lowered SSBOs
    * and atomic counters in nir_shader->info.
    */
   uint8_t num_ssbos;
   /* Number of images used by this shader */
   uint8_t num_images;

   /* Which inputs are actually read */
   uint64_t inputs_read;
   /* Which inputs occupy 2 slots. */
   uint64_t dual_slot_inputs;
   /* Which outputs are actually written */
   uint64_t outputs_written;
   /* Which outputs are actually read */
   uint64_t outputs_read;
   /* Which system values are actually read */
   BITSET_DECLARE(system_values_read, SYSTEM_VALUE_MAX);

   /* If known_interpolation_qualifiers is set, bitsets mapping locations to
    * interpolation qualifiers perspective/linear/flat.
    */
   uint64_t perspective_varyings;
   uint64_t linear_varyings;

   /* Which I/O is per-primitive, for read/written information combine with
    * the fields above.
    */
   uint64_t per_primitive_inputs;
   uint64_t per_primitive_outputs;

   /* Which I/O is per-view */
   uint64_t per_view_outputs;
   /* Enabled view mask, for per-view outputs */
   uint32_t view_mask;

   /* Which 16-bit inputs and outputs are used corresponding to
    * VARYING_SLOT_VARn_16BIT.
    */
   uint16_t inputs_read_16bit;
   uint16_t outputs_written_16bit;
   uint16_t outputs_read_16bit;
   uint16_t inputs_read_indirectly_16bit;
   uint16_t outputs_read_indirectly_16bit;
   uint16_t outputs_written_indirectly_16bit;

   /* Which patch inputs are actually read */
   uint32_t patch_inputs_read;
   /* Which patch outputs are actually written */
   uint32_t patch_outputs_written;
   /* Which patch outputs are read */
   uint32_t patch_outputs_read;

   /* Which inputs are read indirectly (subset of inputs_read) */
   uint64_t inputs_read_indirectly;
   /* Which outputs are read or written indirectly */
   uint64_t outputs_read_indirectly;
   uint64_t outputs_written_indirectly;
   /* Which patch inputs are read indirectly (subset of patch_inputs_read) */
   uint32_t patch_inputs_read_indirectly;
   /* Which patch outputs are read or written indirectly */
   uint32_t patch_outputs_read_indirectly;
   uint32_t patch_outputs_written_indirectly;

   /** Bitfield of which textures are used */
   BITSET_DECLARE(textures_used, 128);

   /** Bitfield of which textures are used by texelFetch() */
   BITSET_DECLARE(textures_used_by_txf, 128);

   /** Bitfield of which textures are texel buffers */
   BITSET_DECLARE(texture_buffers, 128);

   /** Bitfield of which samplers are used */
   BITSET_DECLARE(samplers_used, 32);

   /** Bitfield of which images are used */
   BITSET_DECLARE(images_used, 64);
   /** Bitfield of which images are buffers. */
   BITSET_DECLARE(image_buffers, 64);
   /** Bitfield of which images are MSAA. */
   BITSET_DECLARE(msaa_images, 64);

   /* SPV_KHR_float_controls: execution mode for floating point ops */
   uint32_t float_controls_execution_mode;

   /**
    * Size of shared variables accessed by compute/task/mesh shaders.
    */
   unsigned shared_size;

   /**
    * Size of task payload variables accessed by task/mesh shaders.
    */
   unsigned task_payload_size;

   /**
    * Number of ray tracing queries in the shader (counts all elements of all
    * variables).
    */
   unsigned ray_queries;

   /**
    * Local workgroup size used by compute/task/mesh shaders.
    */
   uint16_t workgroup_size[3];

   uint8_t num_subgroups;

   /* The value reported in gl_SubgroupSize.
    * Must be a power of two between 1 and 128
    * or 0 if still unknown.
    */
   uint8_t api_subgroup_size;

   /* The maximum subgroup size dispatched by the hw.
    * Must be a power of two between 1 and 128.
    * Must not be larger than api_subgroup_size,
    * (unless api_subgroup_size is 0).
    */
   uint8_t max_subgroup_size;

   /* The minimum subgroup size dispatched by the hw.
    * Must be a power of two between 1 and 128.
    * Must not be larger than max_subgroup_size.
    */
   uint8_t min_subgroup_size;

   /* api_subgroup_size must appear to be uniform in
    * the current stage for a whole draw.
    * There is no equivalent for dispatches,
    * because it would be required to be true.
    */
   bool api_subgroup_size_draw_uniform:1;

   /**
    * Uses subgroup intrinsics which can communicate across a quad.
    */
   bool uses_wide_subgroup_intrinsics:1;

   /* Transform feedback buffer strides in dwords, max. 1K - 4. */
   uint8_t xfb_stride[MAX_XFB_BUFFERS];

   uint16_t inlinable_uniform_dw_offsets[MAX_INLINABLE_UNIFORMS];
   uint8_t num_inlinable_uniforms:4;

   /* The size of the gl_ClipDistance[] array, if declared. */
   uint8_t clip_distance_array_size:4;

   /* The size of the gl_CullDistance[] array, if declared. */
   uint8_t cull_distance_array_size:4;

   /* Whether or not this shader ever uses textureGather() */
   bool uses_texture_gather:1;

   /* Whether texture size, levels, or samples is queried. */
   bool uses_resource_info_query:1;

   /* Whether a shader abort instruction is used. */
   bool uses_abort:1;

   /* Bitmask of bit-sizes used with ALU instructions. */
   uint8_t bit_sizes_float;
   uint8_t bit_sizes_int;

   /* Whether the first UBO is the default uniform buffer, i.e. uniforms. */
   bool first_ubo_is_default_ubo:1;

   /* Whether or not separate shader objects were used */
   bool separate_shader:1;

   /* Whether perspective_varyings/linear_varyings can be trusted. This depends
    * on what this shader was linked with, as well as the API.
    */
   bool known_interpolation_qualifiers:1;

   /** Was this shader linked with any transform feedback varyings? */
   bool has_transform_feedback_varyings:1;

   /* Whether flrp has been lowered. */
   bool flrp_lowered:1;

   /* Whether nir_opt_constant_folding should not fold offset srcs of
    * IO intrinsics.
    */
   bool disable_input_offset_src_constant_folding:1;
   bool disable_output_offset_src_constant_folding:1;

   /* Whether nir_lower_io has been called to lower derefs.
    * nir_variables for inputs and outputs might not be present in the IR.
    */
   bool io_lowered:1;

   /* If true, fail an assertion in nir_opt_dce if a dead input is eliminated.
    *
    * This is a debug aid to easily identify passes that cause shader inputs
    * to become dead after nir_opt_varyings when it's preferrable that dead
    * shader inputs are identified before nir_opt_varyings.
    *
    * It shouldn't be enabled by default because inputs can become dead late
    * for all sorts of reasons.
    */
   bool assert_inputs_not_dead : 1;

   /** Has nir_lower_var_copies called. To avoid calling any
    * lowering/optimization that would introduce any copy_deref later.
    */
   bool var_copies_lowered:1;

   /* Whether the shader writes memory, including transform feedback. */
   bool writes_memory:1;

   /* Whether gl_Layer is viewport-relative */
   bool layer_viewport_relative:1;

   /* Whether explicit barriers are used */
   bool uses_control_barrier : 1;
   bool uses_memory_barrier : 1;

   /* Whether ARB_bindless_texture ops or variables are used */
   bool uses_bindless : 1;

   /* Number of embedded samplers used by this shader */
   bool uses_embedded_samplers : 1;

   /**
    * Shared memory types have explicit layout set.  Used for
    * SPV_KHR_workgroup_storage_explicit_layout.
    */
   bool shared_memory_explicit_layout:1;

   /**
    * Used for VK_KHR_zero_initialize_workgroup_memory.
    */
   bool zero_initialize_shared_memory:1;

   /**
    * Used for ARB_compute_variable_group_size.
    */
   bool workgroup_size_variable:1;

   /**
    * Whether the shader uses printf instructions.
    */
   bool uses_printf:1;

   /**
    * VK_KHR_shader_maximal_reconvergence
    */
   bool maximally_reconverges:1;

   /* Use ACO instead of LLVM on AMD. */
   bool use_aco_amd:1;

   /**
    * Whether image intrinsics have been lowered to global intrinsics
    *
    * This is potentially useful on some implementation that need to know that
    * an image barrier needs to include global barriers due to the lowering.
    */
   bool use_lowered_image_to_global:1;

   /**
     * Set if this shader uses legacy (DX9 or ARB assembly) math rules.
     *
     * From the ARB_fragment_program specification:
     *
     *    "The following rules apply to multiplication:
     *
     *      1. <x> * <y> == <y> * <x>, for all <x> and <y>.
     *      2. +/-0.0 * <x> = +/-0.0, at least for all <x> that correspond to
     *         *representable numbers (IEEE "not a number" and "infinity"
     *         *encodings may be exceptions).
     *      3. +1.0 * <x> = <x>, for all <x>.""
     *
     * However, in effect this was due to DX9 semantics implying that 0*x=0 even
     * for inf/nan if the hardware generated them instead of float_min/max.  So,
     * you should not have an exception for inf/nan to rule 2 above.
     *
     * One implementation of this behavior would be to flush all generated NaNs
     * to zero, at which point 0*Inf=Nan=0.  Most DX9/ARB-asm hardware did not
     * generate NaNs, and the only way the GPU saw one was to possibly feed it
     * in as a uniform.
     */
   bool use_legacy_math_rules;

   /*
    * Arrangement of invocations used to calculate derivatives in
    * compute/task/mesh shaders.  From KHR_compute_shader_derivatives.
    */
   enum gl_derivative_group derivative_group:2;

   /* Assume that data races do not happen. If this isn't set, data races
    * read/write undefined values, but do not cause undefined behaviour. This
    * is set when the Vulkan memory model is used.
    */
   bool assume_no_data_races:1;

   /* This shader requires occupancy-bounded forward progress guarantees
    * between workgroups in order to be executed correctly. This means that
    * each workgroup which has already executed at least one step (here
    * defined as an atomic memory operation) must eventually execute another
    * step or terminate. Algorithms that require this guarantee include
    * spin-loops where each workgroup only waits for workgroups with a lower
    * index and indices are assigned via atomicAdd() of a counter.
    */
   bool occupancy_bounded_workgroup_fairness:1;

   /* Type of hash carried in source_blake3.
    */
   enum shader_info_hash_type hash_type:2;

   union {
      struct {
         /* Which inputs are doubles */
         uint64_t double_inputs;

         /* For AMD-specific driver-internal shaders. It replaces vertex
          * buffer loads with code generating VS inputs from scalar registers.
          *
          * Valid values: SI_VS_BLIT_SGPRS_POS_*
          */
         uint8_t blit_sgprs_amd:4;

         /* Software TES executing as HW VS */
         bool tes_poly:1;

         /* True if the shader writes position in window space coordinates pre-transform */
         bool window_space_position:1;

         /** Is an edge flag input needed? */
         bool needs_edge_flag:1;
      } vs;

      struct {
         /** The output primitive type */
         enum mesa_prim output_primitive;

         /** The input primitive type */
         enum mesa_prim input_primitive;

         /** The maximum number of vertices the geometry shader might write. */
         uint16_t vertices_out;

         /** 1 .. MAX_GEOMETRY_SHADER_INVOCATIONS */
         uint8_t invocations;

         /** The number of vertices received per input primitive (max. 6) */
         uint8_t vertices_in:3;

         /** Whether or not this shader uses EndPrimitive */
         bool uses_end_primitive:1;

         /** The streams used in this shaders (max. 4) */
         uint8_t active_stream_mask:4;
      } gs;

      struct {
         bool uses_discard:1;
         bool uses_fbfetch_output:1;
         bool fbfetch_coherent:1;
         bool color_is_dual_source:1;

         /**
          * True if this fragment shader requires full quad invocations. This
          * forces the shader to always behave as-if quad groups start with
          * four active invocations, even if there are no derivatives or quad
          * operations. Because helper invocations cannot have side effects,
          * this mainly impacts subgroup operations such as ballot().
          */
         bool require_full_quads:1;

         /**
          * Whether the derivative group must be equivalent to the quad group.
          */
         bool quad_derivatives:1;

         /**
          * True if this fragment shader requires helper invocations used by
          * coarse derivatives. This can be caused by the use of ALU
          * derivative ops, texture instructions which do implicit
          * derivatives, the use of quad subgroup operations or if the shader
          * requires full quads.
          */
         bool needs_coarse_quad_helper_invocations:1;

         /**
          * True if this fragment shader requires helper invocations for all
          * four fragments in the quad. This can be caused by all the same
          * things as needs_coarse_quad_helper_invocations, except that coarse
          * derivatives don't count as they usually only use 3 out of the 4
          * fragments in a quad.
          */
         bool needs_full_quad_helper_invocations:1;

         /**
          * Whether any inputs are declared with the "sample" qualifier.
          */
         bool uses_sample_qualifier:1;

         /**
          * Whether sample shading is used.
          */
         bool uses_sample_shading:1;

         /**
          * Whether early fragment tests are enabled as defined by
          * ARB_shader_image_load_store.
          */
         bool early_fragment_tests:1;

         /**
          * Defined by INTEL_conservative_rasterization.
          */
         bool inner_coverage:1;

         bool post_depth_coverage:1;

         /**
          * \name ARB_fragment_coord_conventions
          * @{
          */
         bool pixel_center_integer:1;
         bool origin_upper_left:1;
         /*@}*/

         bool pixel_interlock_ordered:1;
         bool pixel_interlock_unordered:1;
         bool sample_interlock_ordered:1;
         bool sample_interlock_unordered:1;

         /**
          * Whether the original shader had sample_mask_in regardless of
          * whether NIR lowered it or optimized it away. The presence of
          * sample_mask_in has side effects such as
          * fragmentShadingRateWithShaderSampleMask == VK_FALSE forcing FSR
          * to be disabled even if sample_mask_in is later optimized away.
          */
         bool sample_mask_in_declared:1;

         /**
          * whether this shader has pixel_local_storage load/store instructions
          */
         bool accesses_pixel_local_storage:1;

         /**
          * Flags whether NIR's base types on the FS color outputs should be
          * ignored.
          *
          * GLSL requires that fragment shader output base types match the
          * render target's base types for the behavior to be defined.  From
          * the GL 4.6 spec:
          *
          *     "If the values written by the fragment shader do not match the
          *      format(s) of the corresponding color buffer(s), the result is
          *      undefined."
          *
          * However, for NIR shaders translated from TGSI, we don't have the
          * output types any more, so the driver will need to do whatever
          * fixups are necessary to handle effectively untyped data being
          * output from the FS.
          */
         bool untyped_color_outputs:1;

         /** gl_FragDepth layout for ARB_conservative_depth. */
         enum gl_frag_depth_layout depth_layout:3;

         /* Bitmask of gl_advanced_blend_mode values that may be used with this
          * shader.
          */
         unsigned advanced_blend_modes;

         /**
          * Defined by AMD_shader_early_and_late_fragment_tests.
          */
         bool early_and_late_fragment_tests:1;
         enum gl_frag_stencil_layout stencil_front_layout:3;
         enum gl_frag_stencil_layout stencil_back_layout:3;
      } fs;

      struct {
         uint16_t workgroup_size_hint[3];

         uint8_t user_data_components_amd:4;

         /*
          * If the shader might run with shared mem on top of `shared_size`.
          */
         bool has_variable_shared_mem:1;

         /**
          * If the shader has any use of a cooperative matrix. From
          * SPV_KHR_cooperative_matrix.
          */
         bool has_cooperative_matrix:1;

         /*
          * If the shader might have a control barrier with only one of
          * NIR_MEMORY_CONTROL_ARRIVE/NIR_MEMORY_CONTROL_WAIT.
          */
         bool has_split_control_barriers:1;

         /**
          * Number of bytes of shared imageblock memory per thread. Currently,
          * this requires that the workgroup size is 32x32x1 and that
          * shared_size = 0. These requirements could be lifted in the future.
          * However, there is no current OpenGL/Vulkan API support for
          * imageblocks. This is only used internally to accelerate blit/copy.
          */
         uint8_t image_block_size_per_thread_agx;

         /**
          * pointer size is:
          *   AddressingModelLogical:    0    (default)
          *   AddressingModelPhysical32: 32
          *   AddressingModelPhysical64: 64
          */
         unsigned ptr_size;

         /** Index provided by VkPipelineShaderStageNodeCreateInfoAMDX or ShaderIndexAMDX */
         uint32_t shader_index;

         /** Maximum size required by any output node payload array */
         uint32_t node_payloads_size;

         /** Static workgroup count for overwriting the enqueued workgroup count. (0 if dynamic) */
         uint32_t workgroup_count[3];
      } cs;

      /* Applies to both TCS and TES. */
      struct {
         enum tess_primitive_mode _primitive_mode;

         /** The number of vertices in the TCS output patch. */
         uint8_t tcs_vertices_out;
         unsigned spacing:2; /*gl_tess_spacing*/

         /** Is the vertex order counterclockwise? */
         bool ccw:1;
         bool point_mode:1;

         /* Bit mask of TCS per-vertex inputs (VS outputs) that are used
          * with a vertex index that is equal to the invocation id.
          *
          * Not mutually exclusive with tcs_cross_invocation_inputs_read, i.e.
          * both input[0] and input[invocation_id] can be present.
          */
         uint64_t tcs_same_invocation_inputs_read;

         /* Bit mask of TCS per-vertex inputs (VS outputs) that are read
          * with a vertex index that is NOT the invocation id
          */
         uint64_t tcs_cross_invocation_inputs_read;

         /* Bit mask of TCS per-vertex outputs that are read
          * with a vertex index that is NOT the invocation id
          */
         uint64_t tcs_cross_invocation_outputs_read;

         /* Bit mask of TCS per-vertex outputs that are written
          * with a vertex index that is NOT the invocation id
          */
         uint64_t tcs_cross_invocation_outputs_written;

         /* Bit mask of TCS per-vertex outputs that are read by TES. */
         uint64_t tcs_outputs_read_by_tes;

         /* Bit mask of TCS per-patch outputs that are read by TES. */
         uint32_t tcs_patch_outputs_read_by_tes;

         /* Bit mask of TCS per-vertex 16-bit outputs that are read by TES.
          * (VARYING_SLOT_VAR0_16BIT + 0..15)
          */
         uint16_t tcs_outputs_read_by_tes_16bit;
      } tess;

      /* Applies to MESH and TASK. */
      struct {
         /* Bit mask of MS outputs that are used
          * with an index that is NOT the local invocation index.
          */
         uint64_t ms_cross_invocation_output_access;

         /* Dimensions of task->mesh dispatch (EmitMeshTasksEXT)
          * when they are known compile-time constants.
          * 0 means they are not known.
          */
         uint32_t ts_mesh_dispatch_dimensions[3];

         uint16_t max_vertices_out;
         uint16_t max_primitives_out;
         enum mesa_prim primitive_type; /* POINTS, LINES or TRIANGLES. */

         /* TODO: remove this when we stop supporting NV_mesh_shader. */
         bool nv;
      } mesh;
   };
} shader_info;

#ifdef __cplusplus
}
#endif

#endif /* SHADER_INFO_H */

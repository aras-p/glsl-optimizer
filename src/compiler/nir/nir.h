/*
 * Copyright © 2014 Connor Abbott
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
 * Authors:
 *    Connor Abbott (cwabbott0@gmail.com)
 *
 */

#ifndef NIR_H
#define NIR_H

#include <stdint.h>
#include "compiler/glsl_types.h"
#include "compiler/list.h"
#include "compiler/shader_enums.h"
#include "compiler/shader_info.h"
#include "util/bitset.h"
#include "util/compiler.h"
#include "util/enum_operators.h"
#include "util/hash_table.h"
#include "util/list.h"
#include "util/log.h"
#include "util/macros.h"
#include "util/ralloc.h"
#include "util/range_minimum_query.h"
#include "util/set.h"
#include "util/simple_mtx.h"
#include "util/sparse_bitset.h"
#include "util/u_dynarray.h"
#include "util/u_math.h"
#include "nir_defines.h"
#include "nir_shader_compiler_options.h"
#include <stdio.h>

#ifndef NDEBUG
#include "util/u_debug.h"
#endif /* NDEBUG */

#include "nir_opcodes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct u_printf_info u_printf_info;
extern uint32_t nir_debug;
extern bool nir_debug_print_shader[MESA_SHADER_KERNEL + 1];

#ifndef NDEBUG
#define NIR_DEBUG(flag) unlikely(nir_debug &(NIR_DEBUG_##flag))
#else
#define NIR_DEBUG(flag) false
#endif

#define NIR_DEBUG_CLONE                  (1u << 0)
#define NIR_DEBUG_SERIALIZE              (1u << 1)
#define NIR_DEBUG_NOVALIDATE             (1u << 2)
#define NIR_DEBUG_EXTENDED_VALIDATION    (1u << 3)
#define NIR_DEBUG_TGSI                   (1u << 4)
#define NIR_DEBUG_PRINT_VS               (1u << 5)
#define NIR_DEBUG_PRINT_TCS              (1u << 6)
#define NIR_DEBUG_PRINT_TES              (1u << 7)
#define NIR_DEBUG_PRINT_GS               (1u << 8)
#define NIR_DEBUG_PRINT_FS               (1u << 9)
#define NIR_DEBUG_PRINT_CS               (1u << 10)
#define NIR_DEBUG_PRINT_TS               (1u << 11)
#define NIR_DEBUG_PRINT_MS               (1u << 12)
#define NIR_DEBUG_PRINT_RGS              (1u << 13)
#define NIR_DEBUG_PRINT_AHS              (1u << 14)
#define NIR_DEBUG_PRINT_CHS              (1u << 15)
#define NIR_DEBUG_PRINT_MHS              (1u << 16)
#define NIR_DEBUG_PRINT_IS               (1u << 17)
#define NIR_DEBUG_PRINT_CBS              (1u << 18)
#define NIR_DEBUG_PRINT_KS               (1u << 19)
#define NIR_DEBUG_PRINT_NO_INLINE_CONSTS (1u << 20)
#define NIR_DEBUG_PRINT_INTERNAL         (1u << 21)
#define NIR_DEBUG_PRINT_PASS_FLAGS       (1u << 22)
#define NIR_DEBUG_INVALIDATE_METADATA    (1u << 23)
#define NIR_DEBUG_PRINT_STRUCT_DECLS     (1u << 24)
#define NIR_DEBUG_PROGRESS_VALIDATION    (1u << 25)

#define NIR_DEBUG_PRINT (NIR_DEBUG_PRINT_VS |  \
                         NIR_DEBUG_PRINT_TCS | \
                         NIR_DEBUG_PRINT_TES | \
                         NIR_DEBUG_PRINT_GS |  \
                         NIR_DEBUG_PRINT_FS |  \
                         NIR_DEBUG_PRINT_CS |  \
                         NIR_DEBUG_PRINT_TS |  \
                         NIR_DEBUG_PRINT_MS |  \
                         NIR_DEBUG_PRINT_RGS | \
                         NIR_DEBUG_PRINT_AHS | \
                         NIR_DEBUG_PRINT_CHS | \
                         NIR_DEBUG_PRINT_MHS | \
                         NIR_DEBUG_PRINT_IS |  \
                         NIR_DEBUG_PRINT_CBS | \
                         NIR_DEBUG_PRINT_KS)

#define NIR_FALSE              0u
#define NIR_TRUE               (~0u)
#define NIR_MAX_VEC_COMPONENTS 16
#define NIR_MAX_MATRIX_COLUMNS 4
#define NIR_STREAM_PACKED      (1 << 8)
typedef uint16_t nir_component_mask_t;

/*
 * Round up a vector size to a vector size that's valid in NIR. At present, NIR
 * supports only vec2-5, vec8, and vec16. Attempting to generate other sizes
 * will fail validation.
 */
static inline unsigned
nir_round_up_components(unsigned n)
{
   return (n > 5) ? util_next_power_of_two(n) : n;
}

static inline unsigned
nir_round_down_components(unsigned n)
{
   return (n > 5) ? MAX2(1 << util_logbase2(n), 5) : n;
}

static inline nir_component_mask_t
nir_component_mask(unsigned num_components)
{
   assert(nir_num_components_valid(num_components));
   return (1u << num_components) - 1;
}

void
nir_process_debug_variable(void);

bool nir_component_mask_can_reinterpret(nir_component_mask_t mask,
                                        unsigned old_bit_size,
                                        unsigned new_bit_size);
nir_component_mask_t
nir_component_mask_reinterpret(nir_component_mask_t mask,
                               unsigned old_bit_size,
                               unsigned new_bit_size);

/** Defines a cast function
 *
 * This macro defines a cast function from in_type to out_type where
 * out_type is some structure type that contains a field of type out_type.
 *
 * Note that you have to be a bit careful as the generated cast function
 * destroys constness.
 */
#define NIR_DEFINE_CAST(name, in_type, out_type, field,   \
                        type_field, type_value)           \
   static inline out_type *                               \
   name(const in_type *parent)                            \
   {                                                      \
      assert(parent && parent->type_field == type_value); \
      return exec_node_data(out_type, parent, field);     \
   }

/**
 * Description of built-in state associated with a uniform
 *
 * :c:member:`nir_variable.state_slots`
 */
typedef struct nir_state_slot {
   gl_state_index16 tokens[STATE_LENGTH];
} nir_state_slot;

/**
 * Resource types according to VkSpirvResourceTypeFlagsKHR.
 */
typedef enum {
   nir_resource_type_sampler = 1u << 0,
   nir_resource_type_sampled_image = 1u << 1,
   nir_resource_type_read_only_image = 1u << 2,
   nir_resource_type_read_write_image = 1u << 3,
   nir_resource_type_combined_sampled_image = 1u << 4,
   nir_resource_type_uniform_buffer = 1u << 5,
   nir_resource_type_read_only_storage_buffer = 1u << 6,
   nir_resource_type_read_write_storage_buffer = 1u << 7,
   nir_resource_type_acceleration_structure = 1u << 8,
} nir_resource_type;

/**
 * Descriptor types.
 */
typedef enum {
   nir_descriptor_type_uniform_buffer = 0,
   nir_descriptor_type_storage_buffer = 1,
   nir_descriptor_type_acceleration_structure = 2,
} nir_descriptor_type;

/**
 * NIR image intrinsics variants.
 */
typedef enum {
   nir_image_intrinsic_type_default = 0,
   nir_image_intrinsic_type_deref = 1,
   nir_image_intrinsic_type_bindless = 2,
   nir_image_intrinsic_type_heap = 3,
} nir_image_intrinsic_type;

/**
 * Rounding modes.
 */
typedef enum {
   nir_rounding_mode_undef = 0,
   nir_rounding_mode_rtne = 1, /* round to nearest even */
   nir_rounding_mode_ru = 2,   /* round up */
   nir_rounding_mode_rd = 3,   /* round down */
   nir_rounding_mode_rtz = 4,  /* round towards zero */
} nir_rounding_mode;

/**
 * Ray query values that can read from a RayQueryKHR object.
 */
typedef enum {
   nir_ray_query_value_intersection_type,
   nir_ray_query_value_intersection_t,
   nir_ray_query_value_intersection_instance_custom_index,
   nir_ray_query_value_intersection_instance_id,
   nir_ray_query_value_intersection_instance_sbt_index,
   nir_ray_query_value_intersection_geometry_index,
   nir_ray_query_value_intersection_primitive_index,
   nir_ray_query_value_intersection_barycentrics,
   nir_ray_query_value_intersection_front_face,
   nir_ray_query_value_intersection_object_ray_direction,
   nir_ray_query_value_intersection_object_ray_origin,
   nir_ray_query_value_intersection_object_to_world,
   nir_ray_query_value_intersection_world_to_object,
   nir_ray_query_value_intersection_candidate_aabb_opaque,
   nir_ray_query_value_tmin,
   nir_ray_query_value_flags,
   nir_ray_query_value_world_ray_direction,
   nir_ray_query_value_world_ray_origin,
   nir_ray_query_value_intersection_triangle_vertex_positions
} nir_ray_query_value;

/**
 * Intel resource flags
 */
typedef enum {
   nir_resource_intel_bindless = 1u << 0,
   nir_resource_intel_pushable = 1u << 1,
   nir_resource_intel_sampler = 1u << 2,
   nir_resource_intel_non_uniform = 1u << 3,
   nir_resource_intel_sampler_embedded = 1u << 4,
   nir_resource_intel_internal = 1u << 5,
} nir_resource_data_intel;

/**
 * Register class for registers managed by nir_opt_preamble. General can handle
 * anything, the others are driver-specific but with common names for nir_print.
 */
typedef enum {
   nir_preamble_class_general,
   nir_preamble_class_image,
   nir_preamble_class_sampler,
   nir_preamble_num_classes,
} nir_preamble_class;

/**
 * Which components to interpret as signed in cmat_muladd.
 * See 'Cooperative Matrix Operands' in SPV_KHR_cooperative_matrix.
 */
typedef enum {
   NIR_CMAT_A_SIGNED = 1u << 0,
   NIR_CMAT_B_SIGNED = 1u << 1,
   NIR_CMAT_C_SIGNED = 1u << 2,
   NIR_CMAT_RESULT_SIGNED = 1u << 3,
} nir_cmat_signed;

typedef enum {
   NIR_CMAT_REDUCE_ROW = 1u << 0,
   NIR_CMAT_REDUCE_COLUMN = 1u << 1,
   NIR_CMAT_REDUCE_2X2 = 1u << 2,
} nir_cmat_reduce;

#define nir_const_value_to_array(arr, c, components, m) \
   do {                                                 \
      for (unsigned i = 0; i < components; ++i)         \
         arr[i] = c[i].m;                               \
   } while (false)

static inline nir_const_value
nir_const_value_for_raw_uint(uint64_t x, unsigned bit_size)
{
   nir_const_value v;
   memset(&v, 0, sizeof(v));

   /* clang-format off */
   switch (bit_size) {
   case 1:  v.b   = (bool)x;  break;
   case 8:  v.u8  = (uint8_t)x;  break;
   case 16: v.u16 = (uint16_t)x;  break;
   case 32: v.u32 = (uint32_t)x;  break;
   case 64: v.u64 = x;  break;
   default:
      UNREACHABLE("Invalid bit size");
   }
   /* clang-format on */

   return v;
}

static inline nir_const_value
nir_const_value_for_int(int64_t i, unsigned bit_size)
{
   assert(bit_size <= 64);
   if (bit_size < 64) {
      assert(i >= (-(1ll << (bit_size - 1))));
      assert(i < (1ll << (bit_size - 1)));
   }

   return nir_const_value_for_raw_uint(i, bit_size);
}

static inline nir_const_value
nir_const_value_for_uint(uint64_t u, unsigned bit_size)
{
   assert(bit_size <= 64);
   if (bit_size < 64)
      assert(u < (1ull << bit_size));

   return nir_const_value_for_raw_uint(u, bit_size);
}

static inline nir_const_value
nir_const_value_for_bool(bool b, unsigned bit_size)
{
   /* Booleans use a 0/-1 convention */
   return nir_const_value_for_int(-(int)b, bit_size);
}

/* This one isn't inline because it requires half-float conversion */
nir_const_value nir_const_value_for_float(double b, unsigned bit_size);

static inline int64_t
nir_const_value_as_int(nir_const_value value, unsigned bit_size)
{
   /* clang-format off */
   switch (bit_size) {
   /* int1_t uses 0/-1 convention */
   case 1:  return -(int)value.b;
   case 8:  return value.i8;
   case 16: return value.i16;
   case 32: return value.i32;
   case 64: return value.i64;
   default:
      UNREACHABLE("Invalid bit size");
   }
   /* clang-format on */
}

static inline uint64_t
nir_const_value_as_uint(nir_const_value value, unsigned bit_size)
{
   /* clang-format off */
   switch (bit_size) {
   case 1:  return value.b;
   case 8:  return value.u8;
   case 16: return value.u16;
   case 32: return value.u32;
   case 64: return value.u64;
   default:
      UNREACHABLE("Invalid bit size");
   }
   /* clang-format on */
}

static inline bool
nir_const_value_as_bool(nir_const_value value, unsigned bit_size)
{
   int64_t i = nir_const_value_as_int(value, bit_size);

   /* Booleans of any size use 0/-1 convention */
   assert(i == 0 || i == -1);

   return i != 0;
}

/* This one isn't inline because it requires half-float conversion */
double nir_const_value_as_float(nir_const_value value, unsigned bit_size);

typedef struct nir_constant nir_constant;

struct nir_constant {
   /**
    * Value of the constant.
    *
    * The field used to back the values supplied by the constant is determined
    * by the type associated with the ``nir_variable``.  Constants may be
    * scalars, vectors, or matrices.
    */
   nir_const_value values[NIR_MAX_VEC_COMPONENTS];

   /* Indicates all the values are 0s which can enable some optimizations */
   bool is_null_constant;

   /* we could get this from the var->type but makes clone *much* easier to
    * not have to care about the type.
    */
   unsigned num_elements;

   /* Array elements / Structure Fields */
   nir_constant **elements;
};

/**
 * Layout qualifiers for gl_FragDepth.
 *
 * The AMD/ARB_conservative_depth extensions allow gl_FragDepth to be redeclared
 * with a layout qualifier.
 */
typedef enum {
   /** No depth layout is specified. */
   nir_depth_layout_none,
   nir_depth_layout_any,
   nir_depth_layout_greater,
   nir_depth_layout_less,
   nir_depth_layout_unchanged
} nir_depth_layout;

/**
 * Enum keeping track of how a variable was declared.
 */
typedef enum {
   /**
    * Normal declaration.
    */
   nir_var_declared_normally = 0,

   /**
    * Variable is an implicitly declared built-in that has not been explicitly
    * re-declared by the shader.
    */
   nir_var_declared_implicitly,

   /**
    * Variable is implicitly generated by the compiler and should not be
    * visible via the API.
    */
   nir_var_hidden,
} nir_var_declaration_type;

typedef struct nir_variable_data nir_variable_data;

/**
 * Either a uniform, global variable, shader input, or shader output. Based on
 * ir_variable - it should be easy to translate between the two.
 */

typedef struct nir_variable {
   struct exec_node node;

   /**
    * Declared type of the variable
    */
   const struct glsl_type *type;

   /**
    * Declared name of the variable
    */
   char *name;

   struct nir_variable_data {
      /**
       * Storage class of the variable.
       *
       * :c:struct:`nir_variable_mode`
       */
      unsigned mode : 26;

      /* A temporary for passes to store information. Used, for example, to
       * replace an unordered set with an ordered util_dynarray.
       */
      bool pass_flags : 1;

      /**
       * Is the variable read-only?
       *
       * This is set for variables declared as ``const``, shader inputs,
       * and uniforms.
       */
      unsigned read_only : 1;
      unsigned centroid : 1;
      unsigned sample : 1;
      unsigned patch : 1;
      unsigned invariant : 1;

      /**
       * Was an 'invariant' qualifier explicitly set in the shader?
       *
       * This is used to cross validate glsl qualifiers.
       */
      unsigned explicit_invariant:1;

      /**
       * Is the variable a ray query?
       */
      unsigned ray_query : 1;

      /**
       * Precision qualifier.
       *
       * In desktop GLSL we do not care about precision qualifiers at all, in
       * fact, the spec says that precision qualifiers are ignored.
       *
       * To make things easy, we make it so that this field is always
       * GLSL_PRECISION_NONE on desktop shaders. This way all the variables
       * have the same precision value and the checks we add in the compiler
       * for this field will never break a desktop shader compile.
       */
      unsigned precision : 2;

      /**
       * Has this variable been statically assigned?
       *
       * This answers whether the variable was assigned in any path of
       * the shader during ast_to_hir.  This doesn't answer whether it is
       * still written after dead code removal, nor is it maintained in
       * non-ast_to_hir.cpp (GLSL parsing) paths.
       */
      unsigned assigned : 1;

      /**
       * Can this variable be coalesced with another?
       *
       * This is set by nir_lower_io_vars_to_temporaries to say that any
       * copies involving this variable should stay put. Propagating it can
       * duplicate the resulting load/store, which is not wanted, and may
       * result in a load/store of the variable with an indirect offset which
       * the backend may not be able to handle.
       */
      unsigned cannot_coalesce : 1;

      /**
       * When separate shader programs are enabled, only input/outputs between
       * the stages of a multi-stage separate program can be safely removed
       * from the shader interface. Other input/outputs must remains active.
       *
       * This is also used to make sure xfb varyings that are unused by the
       * fragment shader are not removed.
       */
      unsigned always_active_io : 1;

      /**
       * Interpolation mode for shader inputs / outputs
       *
       * :c:enum:`glsl_interp_mode`
       */
      unsigned interpolation : 3;

      /**
       * If non-zero, then this variable may be packed along with other variables
       * into a single varying slot, so this offset should be applied when
       * accessing components.  For example, an offset of 1 means that the x
       * component of this variable is actually stored in component y of the
       * location specified by ``location``.
       */
      unsigned location_frac : 2;

      /**
       * If true, this variable represents an array of scalars that should
       * be tightly packed.  In other words, consecutive array elements
       * should be stored one component apart, rather than one slot apart.
       */
      unsigned compact : 1;

      /**
       * Whether this is a fragment shader output implicitly initialized with
       * the previous contents of the specified render target at the
       * framebuffer location corresponding to this shader invocation.
       */
      unsigned fb_fetch_output : 1;

      /**
       * Non-zero if this variable is considered bindless as defined by
       * ARB_bindless_texture.
       */
      unsigned bindless : 1;

      /**
       * Was an explicit binding set in the shader?
       */
      unsigned explicit_binding : 1;

      /**
       * Was the location explicitly set in the shader?
       *
       * If the location is explicitly set in the shader, it **cannot** be changed
       * by the linker or by the API (e.g., calls to ``glBindAttribLocation`` have
       * no effect).
       */
      unsigned explicit_location : 1;

      /* Was the array implicitly sized during linking */
      unsigned implicit_sized_array : 1;

      /**
       * Highest element accessed with a constant array index
       *
       * Not used for non-array variables. -1 is never accessed.
       */
      int max_array_access;

      /**
       * Does this variable have an initializer?
       *
       * This is used by the linker to cross-validiate initializers of global
       * variables.
       */
      unsigned has_initializer:1;

      /**
       * Is the initializer created by the compiler (glsl_zero_init)
       */
      unsigned is_implicit_initializer:1;

      /**
       * Is this varying used by transform feedback?
       *
       * This is used by the linker to decide if it's safe to pack the varying.
       */
      unsigned is_xfb : 1;

      /**
       * Is this varying used only by transform feedback?
       *
       * This is used by the linker to decide if its safe to pack the varying.
       */
      unsigned is_xfb_only : 1;

      /**
       * Was a transfer feedback buffer set in the shader?
       */
      unsigned explicit_xfb_buffer : 1;

      /**
       * Was a transfer feedback stride set in the shader?
       */
      unsigned explicit_xfb_stride : 1;

      /**
       * Was an explicit offset set in the shader?
       */
      unsigned explicit_offset : 1;

      /**
       * Layout of the matrix.  Uses glsl_matrix_layout values.
       */
      unsigned matrix_layout : 2;

      /**
       * Non-zero if this variable was created by lowering a named interface
       * block.
       */
      unsigned from_named_ifc_block : 1;

      /**
       * Unsized array buffer variable.
       */
      unsigned from_ssbo_unsized_array : 1;

      /**
       * Non-zero if the variable must be a shader input. This is useful for
       * constraints on function parameters.
       */
      unsigned must_be_shader_input : 1;

      /**
       * Has this variable been used for reading or writing?
       *
       * Several GLSL semantic checks require knowledge of whether or not a
       * variable has been used.  For example, it is an error to redeclare a
       * variable as invariant after it has been used.
       */
      unsigned used:1;

      /**
       * How the variable was declared.  See nir_var_declaration_type.
       *
       * This is used to detect variables generated by the compiler, so should
       * not be visible via the API.
       */
      unsigned how_declared : 2;

      /**
       * Is this variable per-view?  If so, we know it must be an array with
       * size corresponding to the number of views.
       */
      unsigned per_view : 1;

      /**
       * Whether the variable is per-primitive.
       * Can be use by Mesh Shader outputs and corresponding Fragment Shader inputs.
       */
      unsigned per_primitive : 1;

      /**
       * Whether the variable is declared to indicate that a fragment shader
       * input will not have interpolated values.
       */
      unsigned per_vertex : 1;

      /**
       * Whether the shared memory block that this variable represent alias
       * with other similarly decorated shared memory blocks.  These are Blocks
       * marked as Aliased in SPIR-V.
       *
       * See SPV_KHR_workgroup_storage_explicit_layout for details.
       */
      unsigned aliased_shared_memory : 1;

      /**
       * Layout qualifier for gl_FragDepth. See nir_depth_layout.
       *
       * This is not equal to ``ir_depth_layout_none`` if and only if this
       * variable is ``gl_FragDepth`` and a layout qualifier is specified.
       */
      unsigned depth_layout : 3;

      /**
       * Vertex stream output identifier.
       *
       * For packed outputs, NIR_STREAM_PACKED is set and bits [2*i+1,2*i]
       * indicate the stream of the i-th component.
       */
      unsigned stream : 9;

      /**
       * See gl_access_qualifier.
       *
       * Access flags for memory variables (SSBO/global), image uniforms, and
       * bindless images in uniforms/inputs/outputs.
       */
      unsigned access : 9;

      /* NIR resource type bit index that this variable would have. */
      nir_resource_type resource_type;

      /**
       * Descriptor set binding for sampler or UBO.
       */
      unsigned descriptor_set;

#define NIR_VARIABLE_NO_INDEX ~0

      /**
       * Output index for dual source blending or input attachment index. If
       * it is not declared it is NIR_VARIABLE_NO_INDEX.
       */
      unsigned index;

      /**
       * Initial binding point for a sampler or UBO.
       *
       * For array types, this represents the binding point for the first element.
       */
      unsigned binding;

      /**
       * Storage location of the base of this variable
       *
       * The precise meaning of this field depends on the nature of the variable.
       *
       *   - Vertex shader input: one of the values from ``gl_vert_attrib``.
       *   - Vertex shader output: one of the values from ``gl_varying_slot``.
       *   - Geometry shader input: one of the values from ``gl_varying_slot``.
       *   - Geometry shader output: one of the values from ``gl_varying_slot``.
       *   - Fragment shader input: one of the values from ``gl_varying_slot``.
       *   - Fragment shader output: one of the values from ``gl_frag_result``.
       *   - Task shader output: one of the values from ``gl_varying_slot``.
       *   - Mesh shader input: one of the values from ``gl_varying_slot``.
       *   - Mesh shader output: one of the values from ``gl_varying_slot``.
       *   - Uniforms: Per-stage uniform slot number for default uniform block.
       *   - Uniforms: Index within the uniform block definition for UBO members.
       *   - Non-UBO Uniforms: uniform slot number.
       *   - Other: This field is not currently used.
       *
       * If the variable is a uniform, shader input, or shader output, and the
       * slot has not been assigned, the value will be -1.
       */
      int location;

      /** Required alignment of this variable */
      unsigned alignment;

      /**
       * The actual location of the variable in the IR. Only valid for inputs,
       * outputs, uniforms (including samplers and images), and for UBO and SSBO
       * variables in GLSL.
       */
      unsigned driver_location;

      /**
       * Location an atomic counter or transform feedback is stored at.
       */
      unsigned offset;

      union {
         struct {
            /** Image internal format if specified explicitly, otherwise PIPE_FORMAT_NONE. */
            enum pipe_format format;
         } image;

         struct {
            /**
             * For OpenCL inline samplers. See cl_sampler_addressing_mode and cl_sampler_filter_mode
             */
            unsigned is_inline_sampler : 1;
            unsigned addressing_mode : 3;
            unsigned normalized_coordinates : 1;
            unsigned filter_mode : 1;
         } sampler;

         struct {
            /**
             * Transform feedback buffer.
             */
            uint16_t buffer : 2;

            /**
             * Transform feedback stride.
             */
            uint16_t stride;
         } xfb;
      };

      /** Name of the node this payload will be enqueued to. */
      const char *node_name;
   } data;

   /**
    * Identifier for this variable generated by nir_index_vars() that is unique
    * among other variables in the same exec_list.
    */
   unsigned index;

   /* Number of nir_variable_data members */
   uint16_t num_members;

   /**
    * For variables with non NULL interface_type, this points to an array of
    * integers such that if the ith member of the interface block is an array,
    * max_ifc_array_access[i] is the maximum array element of that member that
    * has been accessed.  If the ith member of the interface block is not an
    * array, max_ifc_array_access[i] is unused.
    *
    * For variables whose type is not an interface block, this pointer is
    * NULL.
    */
   int *max_ifc_array_access;

   /**
    * Built-in state that backs this uniform
    *
    * Once set at variable creation, ``state_slots`` must remain invariant.
    * This is because, ideally, this array would be shared by all clones of
    * this variable in the IR tree.  In other words, we'd really like for it
    * to be a fly-weight.
    *
    * If the variable is not a uniform, ``num_state_slots`` will be zero and
    * ``state_slots`` will be ``NULL``.
    *
    * Number of state slots used.
    */
   uint16_t num_state_slots;
   /** State descriptors. */
   nir_state_slot *state_slots;

   /**
    * Constant expression assigned in the initializer of the variable
    *
    * This field should only be used temporarily by creators of NIR shaders
    * and then nir_lower_variable_initializers can be used to get rid of them.
    * Most of the rest of NIR ignores this field or asserts that it's NULL.
    */
   nir_constant *constant_initializer;

   /**
    * Global variable assigned in the initializer of the variable
    * This field should only be used temporarily by creators of NIR shaders
    * and then nir_lower_variable_initializers can be used to get rid of them.
    * Most of the rest of NIR ignores this field or asserts that it's NULL.
    */
   nir_variable *pointer_initializer;

   /**
    * For variables that are in an interface block or are an instance of an
    * interface block, this is the ``GLSL_TYPE_INTERFACE`` type for that block.
    *
    * ``ir_variable.location``
    */
   const struct glsl_type *interface_type;

   /**
    * Description of per-member data for per-member struct variables
    *
    * This is used for variables which are actually an amalgamation of
    * multiple entities such as a struct of built-in values or a struct of
    * inputs each with their own layout specifier.  This is only allowed on
    * variables with a struct or array of array of struct type.
    */
   nir_variable_data *members;

   /**
    * This is the backing storage for "name" if the name is short enough
    * to fit in it, so that we don't have to ralloc the name.
    *
    * It eliminates 95.6% ralloc calls for variable names when compiling
    * my GLSL shader-db.
    */
   char _name_storage[16];
} nir_variable;

static inline bool
_nir_shader_variable_has_mode(nir_variable *var, unsigned modes)
{
   /* This isn't a shader variable */
   assert(!(modes & nir_var_function_temp));
   return var->data.mode & modes;
}

#define nir_foreach_variable_in_list(var, var_list) \
   foreach_list_typed(nir_variable, var, node, var_list)

#define nir_foreach_variable_in_list_safe(var, var_list) \
   foreach_list_typed_safe(nir_variable, var, node, var_list)

#define nir_foreach_variable_in_shader(var, shader) \
   nir_foreach_variable_in_list(var, &(shader)->variables)

#define nir_foreach_variable_in_shader_safe(var, shader) \
   nir_foreach_variable_in_list_safe(var, &(shader)->variables)

#define nir_foreach_variable_with_modes(var, shader, modes) \
   nir_foreach_variable_in_shader(var, shader)              \
      if (_nir_shader_variable_has_mode(var, modes))

#define nir_foreach_variable_with_modes_safe(var, shader, modes) \
   nir_foreach_variable_in_shader_safe(var, shader)              \
      if (_nir_shader_variable_has_mode(var, modes))

#define nir_foreach_shader_in_variable(var, shader) \
   nir_foreach_variable_with_modes(var, shader, nir_var_shader_in)

#define nir_foreach_shader_in_variable_safe(var, shader) \
   nir_foreach_variable_with_modes_safe(var, shader, nir_var_shader_in)

#define nir_foreach_shader_out_variable(var, shader) \
   nir_foreach_variable_with_modes(var, shader, nir_var_shader_out)

#define nir_foreach_shader_out_variable_safe(var, shader) \
   nir_foreach_variable_with_modes_safe(var, shader, nir_var_shader_out)

#define nir_foreach_uniform_variable(var, shader) \
   nir_foreach_variable_with_modes(var, shader, nir_var_uniform)

#define nir_foreach_uniform_variable_safe(var, shader) \
   nir_foreach_variable_with_modes_safe(var, shader, nir_var_uniform)

#define nir_foreach_image_variable(var, shader) \
   nir_foreach_variable_with_modes(var, shader, nir_var_image)

#define nir_foreach_image_variable_safe(var, shader) \
   nir_foreach_variable_with_modes_safe(var, shader, nir_var_image)

static inline bool
nir_variable_is_global(const nir_variable *var)
{
   return var->data.mode != nir_var_function_temp;
}

typedef enum ENUM_PACKED {
   nir_instr_type_alu,
   nir_instr_type_deref,
   nir_instr_type_call,
   nir_instr_type_tex,
   nir_instr_type_intrinsic,
   nir_instr_type_load_const,
   nir_instr_type_jump,
   nir_instr_type_undef,
   nir_instr_type_phi,
   nir_instr_type_cmat_call,
} nir_instr_type;

typedef struct nir_instr {
   struct exec_node node;
   nir_block *block;
   nir_instr_type type;

   /* A temporary for optimization and analysis passes to use for storing
    * flags.  For instance, DCE uses this to store the "dead/live" info.
    */
   uint8_t pass_flags;

   /* Equal to nir_shader::has_debug_info and intended to be used by
    * functions that deal with debug information but do not have access to
    * the nir_shader.
    */
   bool has_debug_info;

   /** generic instruction index. */
   uint32_t index;
} nir_instr;

static inline nir_instr *
nir_instr_next(nir_instr *instr)
{
   struct exec_node *next = exec_node_get_next(&instr->node);
   if (exec_node_is_tail_sentinel(next))
      return NULL;
   else
      return exec_node_data(nir_instr, next, node);
}

static inline nir_instr *
nir_instr_prev(nir_instr *instr)
{
   struct exec_node *prev = exec_node_get_prev(&instr->node);
   if (exec_node_is_head_sentinel(prev))
      return NULL;
   else
      return exec_node_data(nir_instr, prev, node);
}

static inline bool
nir_instr_is_first(const nir_instr *instr)
{
   return exec_node_is_head_sentinel(exec_node_get_prev_const(&instr->node));
}

static inline bool
nir_instr_is_last(const nir_instr *instr)
{
   return exec_node_is_tail_sentinel(exec_node_get_next_const(&instr->node));
}

typedef struct nir_def {
   /** set of nir_instrs where this register is used (read from) */
   struct list_head uses;

   /** generic SSA definition index. */
   unsigned index;

   uint8_t num_components;

   /* The bit-size of each channel; must be one of 1, 8, 16, 32, or 64 */
   uint8_t bit_size;

   /**
    * True if this SSA value may have different values in different SIMD
    * invocations of the shader.  This is set by nir_divergence_analysis.
    */
   bool divergent;

   /**
    * True if this SSA value is loop invariant w.r.t. the innermost parent
    * loop.  This is set by nir_divergence_analysis and used to determine
    * the divergence of a nir_src.
    */
   bool loop_invariant;
} nir_def;

typedef struct nir_src {
   /* Instruction or if-statement that consumes this value as a source. This
    * should only be accessed through nir_src_* helpers.
    *
    * Internally, it is a tagged pointer to a nir_instr or nir_if.
    */
   uintptr_t _parent;

   struct list_head use_link;
   nir_def *ssa;
} nir_src;

/* Layout of the _parent pointer. Bottom bit is set for nir_if parents (clear
 * for nir_instr parents). Remaining bits are the pointer.
 */
#define NIR_SRC_PARENT_IS_IF (0x1)
#define NIR_SRC_PARENT_MASK (~((uintptr_t) NIR_SRC_PARENT_IS_IF))

static inline bool
nir_src_is_if(const nir_src *src)
{
   return src->_parent & NIR_SRC_PARENT_IS_IF;
}

static inline nir_instr *
nir_src_use_instr(const nir_src *src)
{
   assert(!nir_src_is_if(src));

   /* Because it is not an if, the tag is 0, therefore we do not need to mask */
   return (nir_instr *)(src->_parent);
}

static inline nir_if *
nir_src_use_if(const nir_src *src)
{
   assert(nir_src_is_if(src));

   /* Because it is an if, the tag is 1, so we need to mask */
   return (nir_if *)(src->_parent & NIR_SRC_PARENT_MASK);
}

static inline void
_nir_src_set_use(nir_src *src, void *parent, bool is_if)
{
    uintptr_t ptr = (uintptr_t) parent;
    assert((ptr & ~NIR_SRC_PARENT_MASK) == 0 && "pointer must be aligned");

    if (is_if)
       ptr |= NIR_SRC_PARENT_IS_IF;

    src->_parent = ptr;
}

static inline void
nir_src_set_use_instr(nir_src *src, nir_instr *parent_instr)
{
   _nir_src_set_use(src, parent_instr, false);
}

static inline void
nir_src_set_use_if(nir_src *src, nir_if *parent_if)
{
   _nir_src_set_use(src, parent_if, true);
}

static inline nir_src
nir_src_init(void)
{
   nir_src src = { 0 };
   return src;
}

#define NIR_SRC_INIT nir_src_init()

#define nir_foreach_use_including_if(src, reg_or_ssa_def) \
   list_for_each_entry(nir_src, src, &(reg_or_ssa_def)->uses, use_link)

#define nir_foreach_use_including_if_safe(src, reg_or_ssa_def) \
   list_for_each_entry_safe(nir_src, src, &(reg_or_ssa_def)->uses, use_link)

#define nir_foreach_use(src, reg_or_ssa_def)         \
   nir_foreach_use_including_if(src, reg_or_ssa_def) \
      if (!nir_src_is_if(src))

#define nir_foreach_use_safe(src, reg_or_ssa_def)         \
   nir_foreach_use_including_if_safe(src, reg_or_ssa_def) \
      if (!nir_src_is_if(src))

#define nir_foreach_if_use(src, reg_or_ssa_def)      \
   nir_foreach_use_including_if(src, reg_or_ssa_def) \
      if (nir_src_is_if(src))

#define nir_foreach_if_use_safe(src, reg_or_ssa_def)      \
   nir_foreach_use_including_if_safe(src, reg_or_ssa_def) \
      if (nir_src_is_if(src))

static inline bool
nir_def_used_by_if(const nir_def *def)
{
   nir_foreach_if_use(_, def)
      return true;

   return false;
}

static inline bool
nir_def_only_used_by_if(const nir_def *def)
{
   nir_foreach_use(_, def)
      return false;

   return true;
}

static inline nir_src
nir_src_for_ssa(nir_def *def)
{
   nir_src src = NIR_SRC_INIT;

   src.ssa = def;

   return src;
}

static inline unsigned
nir_src_bit_size(nir_src src)
{
   return src.ssa->bit_size;
}

static inline unsigned
nir_src_num_components(nir_src src)
{
   return src.ssa->num_components;
}

nir_block *nir_src_get_block(nir_src *src);

bool nir_def_is_divergent_at_use_block(nir_def *def, nir_block *block);

static inline bool
nir_src_is_divergent(nir_src *src)
{
   return nir_def_is_divergent_at_use_block(src->ssa, nir_src_get_block(src));
}

/* Are all components the same, ie. .xxxx */
static inline bool
nir_is_same_comp_swizzle(uint8_t *swiz, unsigned nr_comp)
{
   for (unsigned i = 1; i < nr_comp; i++)
      if (swiz[i] != swiz[0])
         return false;
   return true;
}

/* Are all components sequential, ie. .yzw */
static inline bool
nir_is_sequential_comp_swizzle(uint8_t *swiz, unsigned nr_comp)
{
   for (unsigned i = 1; i < nr_comp; i++)
      if (swiz[i] != (swiz[0] + i))
         return false;
   return true;
}

/***/
typedef struct nir_alu_src {
   /** Base source */
   nir_src src;

   /**
    * For each input component, says which component of the register it is
    * chosen from.
    *
    * Note that which elements of the swizzle are used and which are ignored
    * are based on the write mask for most opcodes - for example, a statement
    * like "foo.xzw = bar.zyx" would have a writemask of 1101b and a swizzle
    * of {2, 1, x, 0} where x means "don't care."
    */
   uint8_t swizzle[NIR_MAX_VEC_COMPONENTS];
} nir_alu_src;

nir_alu_type
nir_get_nir_type_for_glsl_base_type(enum glsl_base_type base_type);

static inline nir_alu_type
nir_get_nir_type_for_glsl_type(const struct glsl_type *type)
{
   return nir_get_nir_type_for_glsl_base_type(glsl_get_base_type(type));
}

enum glsl_base_type
nir_get_glsl_base_type_for_nir_type(nir_alu_type base_type);

nir_op nir_type_conversion_op(nir_alu_type src, nir_alu_type dst,
                              nir_rounding_mode rnd);

/**
 * Atomic intrinsics perform different operations depending on the value of
 * their atomic_op constant index. nir_atomic_op defines the operations.
 */
typedef enum {
   nir_atomic_op_iadd,
   nir_atomic_op_isub,
   nir_atomic_op_imin,
   nir_atomic_op_umin,
   nir_atomic_op_imax,
   nir_atomic_op_umax,
   nir_atomic_op_iand,
   nir_atomic_op_ior,
   nir_atomic_op_ixor,
   nir_atomic_op_xchg,
   nir_atomic_op_fadd,
   nir_atomic_op_fmin,
   nir_atomic_op_fmax,
   nir_atomic_op_cmpxchg,
   nir_atomic_op_fcmpxchg,
   nir_atomic_op_inc_wrap,
   nir_atomic_op_dec_wrap,
   nir_atomic_op_ordered_add_gfx12_amd,
} nir_atomic_op;

static inline nir_alu_type
nir_atomic_op_type(nir_atomic_op op)
{
   switch (op) {
   case nir_atomic_op_imin:
   case nir_atomic_op_imax:
      return nir_type_int;

   case nir_atomic_op_fadd:
   case nir_atomic_op_fmin:
   case nir_atomic_op_fmax:
   case nir_atomic_op_fcmpxchg:
      return nir_type_float;

   case nir_atomic_op_iadd:
   case nir_atomic_op_isub:
   case nir_atomic_op_iand:
   case nir_atomic_op_ior:
   case nir_atomic_op_ixor:
   case nir_atomic_op_xchg:
   case nir_atomic_op_cmpxchg:
   case nir_atomic_op_umin:
   case nir_atomic_op_umax:
   case nir_atomic_op_inc_wrap:
   case nir_atomic_op_dec_wrap:
   case nir_atomic_op_ordered_add_gfx12_amd:
      return nir_type_uint;
   }

   UNREACHABLE("Invalid nir_atomic_op");
}

nir_op
nir_atomic_op_to_alu(nir_atomic_op op);

/** Returns nir_op_vec<num_components> or nir_op_mov if num_components == 1
 *
 * This is subtly different from nir_op_is_vec() which returns false for
 * nir_op_mov.  Returning nir_op_mov from nir_op_vec() when num_components == 1
 * makes sense under the assumption that the num_components of the resulting
 * nir_def will same as what is passed in here because a single-component mov
 * is effectively a vec1.  However, if alu->def.num_components > 1, nir_op_mov
 * has different semantics from nir_op_vec* so so code which detects "is this
 * a vec?" typically needs to handle nir_op_mov separate from nir_op_vecN.
 *
 * In the unlikely case where you can handle nir_op_vecN and nir_op_mov
 * together, use nir_op_is_vec_or_mov().
 */
nir_op
nir_op_vec(unsigned num_components);

/** Returns true if this op is one of nir_op_vec*
 *
 * Returns false for nir_op_mov.  See nir_op_vec() for more details.
 */
bool
nir_op_is_vec(nir_op op);

static inline bool
nir_op_is_vec_or_mov(nir_op op)
{
   return op == nir_op_mov || nir_op_is_vec(op);
}

static inline bool
nir_is_denorm_flush_to_zero(unsigned execution_mode, unsigned bit_size)
{
   return (16 == bit_size && execution_mode & FLOAT_CONTROLS_DENORM_FLUSH_TO_ZERO_FP16) ||
          (32 == bit_size && execution_mode & FLOAT_CONTROLS_DENORM_FLUSH_TO_ZERO_FP32) ||
          (64 == bit_size && execution_mode & FLOAT_CONTROLS_DENORM_FLUSH_TO_ZERO_FP64);
}

static inline bool
nir_is_denorm_preserve(unsigned execution_mode, unsigned bit_size)
{
   return (16 == bit_size && execution_mode & FLOAT_CONTROLS_DENORM_PRESERVE_FP16) ||
          (32 == bit_size && execution_mode & FLOAT_CONTROLS_DENORM_PRESERVE_FP32) ||
          (64 == bit_size && execution_mode & FLOAT_CONTROLS_DENORM_PRESERVE_FP64);
}

static inline bool
nir_is_rounding_mode_rtne(unsigned execution_mode, unsigned bit_size)
{
   return (16 == bit_size && execution_mode & FLOAT_CONTROLS_ROUNDING_MODE_RTE_FP16) ||
          (32 == bit_size && execution_mode & FLOAT_CONTROLS_ROUNDING_MODE_RTE_FP32) ||
          (64 == bit_size && execution_mode & FLOAT_CONTROLS_ROUNDING_MODE_RTE_FP64);
}

static inline bool
nir_is_rounding_mode_rtz(unsigned execution_mode, unsigned bit_size)
{
   return (16 == bit_size && execution_mode & FLOAT_CONTROLS_ROUNDING_MODE_RTZ_FP16) ||
          (32 == bit_size && execution_mode & FLOAT_CONTROLS_ROUNDING_MODE_RTZ_FP32) ||
          (64 == bit_size && execution_mode & FLOAT_CONTROLS_ROUNDING_MODE_RTZ_FP64);
}

static inline bool
nir_has_any_rounding_mode_rtz(unsigned execution_mode)
{
   return (execution_mode & FLOAT_CONTROLS_ROUNDING_MODE_RTZ_FP16) ||
          (execution_mode & FLOAT_CONTROLS_ROUNDING_MODE_RTZ_FP32) ||
          (execution_mode & FLOAT_CONTROLS_ROUNDING_MODE_RTZ_FP64);
}

static inline bool
nir_has_any_rounding_mode_rtne(unsigned execution_mode)
{
   return (execution_mode & FLOAT_CONTROLS_ROUNDING_MODE_RTE_FP16) ||
          (execution_mode & FLOAT_CONTROLS_ROUNDING_MODE_RTE_FP32) ||
          (execution_mode & FLOAT_CONTROLS_ROUNDING_MODE_RTE_FP64);
}

static inline nir_rounding_mode
nir_get_rounding_mode_from_float_controls(unsigned execution_mode,
                                          nir_alu_type type)
{
   if (nir_alu_type_get_base_type(type) != nir_type_float)
      return nir_rounding_mode_undef;

   unsigned bit_size = nir_alu_type_get_type_size(type);

   if (nir_is_rounding_mode_rtz(execution_mode, bit_size))
      return nir_rounding_mode_rtz;
   if (nir_is_rounding_mode_rtne(execution_mode, bit_size))
      return nir_rounding_mode_rtne;
   return nir_rounding_mode_undef;
}

static inline bool
nir_has_any_rounding_mode_enabled(unsigned execution_mode)
{
   bool result =
      nir_has_any_rounding_mode_rtne(execution_mode) ||
      nir_has_any_rounding_mode_rtz(execution_mode);
   return result;
}

typedef enum {
   /**
    * Operation where the first two sources are commutative.
    *
    * For 2-source operations, this just mathematical commutativity.  Some
    * 3-source operations, like ffma, are only commutative in the first two
    * sources.
    */
   NIR_OP_IS_2SRC_COMMUTATIVE = (1 << 0),

   /**
    * Operation is associative
    */
   NIR_OP_IS_ASSOCIATIVE = (1 << 1),

   /**
    * Operation where src[0] is used to select src[1] on true or src[2] false.
    * src[0] may be Boolean, or it may be another type used in an implicit
    * comparison.
    */
   NIR_OP_IS_SELECTION = (1 << 2),

   /**
    * Operation is associative mathematically (as real numbers), but not
    * associative with floating-point math. This can be treated as associative
    * iff the operation's nir_fp_no_reassoc bit is not set.
    */
   NIR_OP_IS_INEXACT_ASSOCIATIVE = (1 << 3),
} nir_op_algebraic_property;

/* vec16 is the widest ALU op in NIR, making the max number of input of ALU
 * instructions to be the same as NIR_MAX_VEC_COMPONENTS.
 */
#define NIR_ALU_MAX_INPUTS NIR_MAX_VEC_COMPONENTS

/***/
typedef struct nir_op_info {
   /** Name of the NIR ALU opcode */
   const char *name;

   /** Number of inputs (sources) */
   uint8_t num_inputs;

   /**
    * The number of components in the output
    *
    * If non-zero, this is the size of the output and input sizes are
    * explicitly given; swizzle and writemask are still in effect, but if
    * the output component is masked out, then the input component may
    * still be in use.
    *
    * If zero, the opcode acts in the standard, per-component manner; the
    * operation is performed on each component (except the ones that are
    * masked out) with the input being taken from the input swizzle for
    * that component.
    *
    * The size of some of the inputs may be given (i.e. non-zero) even
    * though output_size is zero; in that case, the inputs with a zero
    * size act per-component, while the inputs with non-zero size don't.
    */
   uint8_t output_size;

   /**
    * The type of vector that the instruction outputs.
    */
   nir_alu_type output_type;

   /**
    * The number of components in each input
    *
    * See nir_op_infos::output_size for more detail about the relationship
    * between input and output sizes.
    */
   uint8_t input_sizes[NIR_ALU_MAX_INPUTS];

   /**
    * The type of vector that each input takes.
    */
   nir_alu_type input_types[NIR_ALU_MAX_INPUTS];

   /** Algebraic properties of this opcode */
   nir_op_algebraic_property algebraic_properties;

   /** Whether this represents a numeric conversion opcode */
   bool is_conversion;

   uint8_t valid_fp_math_ctrl;
} nir_op_info;

/** Metadata for each nir_op, indexed by opcode */
extern const nir_op_info nir_op_infos[nir_num_opcodes];

static inline bool
nir_op_is_selection(nir_op op)
{
   return (nir_op_infos[op].algebraic_properties & NIR_OP_IS_SELECTION) != 0;
}

#define NIR_FP_MATH_CONTROL_BIT_COUNT 6
/**
 * Floating point fast math control.
 *
 * All new bits must restrict optimizations when they are set, not when they
 * are missing. This means a bitwise OR always produces a no less restrictive set.
 */
typedef enum {
   /**
    * If not set, any -0.0 or +0.0 output can have either sign,
    * and any zero input can be treated as having opposite sign.
    */
   nir_fp_preserve_signed_zero = BITFIELD_BIT(0),

   /**
    * If not set, the output value is undefined if
    * it or any input is -Inf or +Inf.
    */
   nir_fp_preserve_inf = BITFIELD_BIT(1),

   /**
    * If not set, the output value is undefined if
    * it or any input is a NaN.
    */
   nir_fp_preserve_nan = BITFIELD_BIT(2),

   /* If not set, allow all optimizations that result in a value closer
    * to the infinitely precise result than the original pattern.
    * One common example here is fma fusing.
    */
   nir_fp_no_contract = BITFIELD_BIT(3),

   /* If not set, allow to treat opcodes with NIR_OP_IS_INEXACT_ASSOCIATIVE
    * as associative.
    */
   nir_fp_no_reassoc = BITFIELD_BIT(4),

   /* If not set, allow any optimization that would be valid for real numbers.
    *
    * Must be set if nir_fp_no_contract or nir_fp_no_reassoc are set.
    */
   nir_fp_no_transform = BITFIELD_BIT(5),

   /** Indicates that this ALU instruction generates an exact value
    *
    * This is kind of a mixture of GLSL "precise" and "invariant" and not
    * really equivalent to either.  This indicates that the value generated by
    * this operation is high-precision and any code transformations that touch
    * it must ensure that the resulting value is bit-for-bit identical to the
    * original, with the exception of undefindness allowed by other
    * nir_fp_math_control bits and NaN patterns.
    *
    * The sub bits allow more fine grained control over the kind of inexact
    * optimizations allowed.
    */
   nir_fp_exact = nir_fp_no_contract |
                  nir_fp_no_reassoc |
                  nir_fp_no_transform,

   nir_fp_preserve_sz_inf_nan = nir_fp_preserve_signed_zero |
                                nir_fp_preserve_inf |
                                nir_fp_preserve_nan,

   nir_fp_fast_math = 0,
   nir_fp_no_fast_math = BITFIELD_MASK(NIR_FP_MATH_CONTROL_BIT_COUNT),
} nir_fp_math_control;

/***/
typedef struct nir_alu_instr {
   /** Base instruction */
   nir_instr instr;

   /** Destination */
   nir_def def;

   /** Opcode */
   nir_op op;

   /**
    * Indicates that this instruction doese not cause signed integer wrapping
    * to occur, in the form of overflow or underflow.
    */
   bool no_signed_wrap : 1;

   /**
    * Indicates that this instruction does not cause unsigned integer wrapping
    * to occur, in the form of overflow or underflow.
    */
   bool no_unsigned_wrap : 1;

   /**
    * The float controls bit float_controls2 cares about. The float_controls1 bits
    * that have no float_controls2 equivalent (rounding mode and denorm handling)
    * remain in the execution mode only.
    */
   uint32_t fp_math_ctrl : NIR_FP_MATH_CONTROL_BIT_COUNT;

   /** Sources
    *
    * The size of the array is given by :c:member:`nir_op_info.num_inputs`.
    */
   nir_alu_src src[];
} nir_alu_instr;

static inline bool
nir_alu_instr_is_signed_zero_preserve(const nir_alu_instr *alu)
{
   return alu->fp_math_ctrl & nir_fp_preserve_signed_zero;
}

static inline bool
nir_alu_instr_is_inf_preserve(const nir_alu_instr *alu)
{
   return alu->fp_math_ctrl & nir_fp_preserve_inf;
}

static inline bool
nir_alu_instr_is_nan_preserve(const nir_alu_instr *alu)
{
   return alu->fp_math_ctrl & nir_fp_preserve_nan;
}

static inline bool
nir_alu_instr_is_signed_zero_inf_nan_preserve(const nir_alu_instr *alu)
{
   return alu->fp_math_ctrl & nir_fp_preserve_sz_inf_nan;
}

static inline bool
nir_alu_instr_is_exact(const nir_alu_instr *alu)
{
   return alu->fp_math_ctrl & nir_fp_exact;
}

static inline bool
nir_alu_instr_no_contract(const nir_alu_instr *alu)
{
   return alu->fp_math_ctrl & nir_fp_no_contract;
}

static inline bool
nir_alu_instr_no_reassoc(const nir_alu_instr *alu)
{
   return alu->fp_math_ctrl & nir_fp_no_reassoc;
}

static inline bool
nir_alu_instr_no_transform(const nir_alu_instr *alu)
{
   return alu->fp_math_ctrl & nir_fp_no_transform;
}

static inline unsigned
nir_op_valid_fp_math_ctrl(nir_op op, unsigned fp_math_ctrl)
{
   return fp_math_ctrl & nir_op_infos[op].valid_fp_math_ctrl;
}

void nir_alu_src_copy(nir_alu_src *dest, const nir_alu_src *src);

nir_component_mask_t
nir_alu_instr_src_read_mask(const nir_alu_instr *instr, unsigned src);
/**
 * Get the number of channels used for a source
 */
unsigned
nir_ssa_alu_instr_src_components(const nir_alu_instr *instr, unsigned src);

/* is this source channel used? */
static inline bool
nir_alu_instr_channel_used(const nir_alu_instr *instr, unsigned src,
                           unsigned channel)
{
   return channel < nir_ssa_alu_instr_src_components(instr, src);
}

bool
nir_alu_instr_is_comparison(const nir_alu_instr *instr);

static inline bool
nir_alu_instr_is_mul_add(const nir_alu_instr *instr)
{
   if (!instr)
      return false;

   switch (instr->op) {
   case nir_op_ffma:
   case nir_op_ffma_weak:
   case nir_op_fmad:
      return true;
   default:
      return false;
   }
}

static inline bool
nir_alu_instr_is_mul_add_z(const nir_alu_instr *instr)
{
   if (!instr)
      return false;

   switch (instr->op) {
   case nir_op_ffmaz:
   case nir_op_fmadz:
      return true;
   default:
      return false;
   }
}

static inline bool
nir_alu_instr_is_any_mul_add(const nir_alu_instr *alu)
{
   return nir_alu_instr_is_mul_add(alu) || nir_alu_instr_is_mul_add_z(alu);
}

bool nir_const_value_negative_equal(nir_const_value c1, nir_const_value c2,
                                    nir_alu_type full_type);

bool nir_alu_srcs_equal(const nir_alu_instr *alu1, const nir_alu_instr *alu2,
                        unsigned src1, unsigned src2);

bool nir_alu_srcs_negative_equal_typed(const nir_alu_instr *alu1,
                                       const nir_alu_instr *alu2,
                                       unsigned src1, unsigned src2,
                                       nir_alu_type base_type);
bool nir_alu_srcs_negative_equal(const nir_alu_instr *alu1,
                                 const nir_alu_instr *alu2,
                                 unsigned src1, unsigned src2);

bool nir_alu_src_is_trivial_ssa(const nir_alu_src *src, unsigned num_components);

bool nir_alu_has_trivial_src(const nir_alu_instr *alu, unsigned srcn);

typedef enum {
   nir_deref_type_var,
   nir_deref_type_array,
   nir_deref_type_array_wildcard,
   nir_deref_type_ptr_as_array,
   nir_deref_type_struct,
   nir_deref_type_cast,
} nir_deref_type;

typedef struct nir_deref_instr {
   nir_instr instr;

   /** Destination to store the resulting "pointer" */
   nir_def def;

   /** The type of this deref instruction */
   nir_deref_type deref_type;

   /** Bitmask what modes the underlying variable might be
    *
    * For OpenCL-style generic pointers, we may not know exactly what mode it
    * is at any given point in time in the compile process.  This bitfield
    * contains the set of modes which it MAY be.
    *
    * Generally, this field should not be accessed directly.  Use one of the
    * nir_deref_mode_ helpers instead.
    */
   nir_variable_mode modes;

   /** The dereferenced type of the resulting pointer value */
   const struct glsl_type *type;

   union {
      /** Variable being dereferenced if deref_type is a deref_var */
      nir_variable *var;

      /** Parent deref if deref_type is not deref_var */
      nir_src parent;
   };

   /** Additional deref parameters */
   union {
      struct {
         nir_src index;
         bool in_bounds;
      } arr;

      struct {
         unsigned index;
      } strct;

      struct {
         unsigned ptr_stride;
         unsigned align_mul;
         unsigned align_offset;
      } cast;
   };
} nir_deref_instr;

/**
 * Returns true if the cast is trivial, i.e. the source and destination type is
 * the same.
 */
bool nir_deref_cast_is_trivial(nir_deref_instr *cast);

/** Returns true if deref might have one of the given modes
 *
 * For multi-mode derefs, this returns true if any of the possible modes for
 * the deref to have any of the specified modes.  This function returning true
 * does NOT mean that the deref definitely has one of those modes.  It simply
 * means that, with the best information we have at the time, it might.
 */
static inline bool
nir_deref_mode_may_be(const nir_deref_instr *deref, nir_variable_mode modes)
{
   assert(!(modes & ~nir_var_all));
   assert(deref->modes != 0);
   return (deref->modes & modes) != 0;
}

/** Returns true if deref must have one of the given modes
 *
 * For multi-mode derefs, this returns true if NIR can prove that the given
 * deref has one of the specified modes.  This function returning false does
 * NOT mean that deref doesn't have one of the given mode.  It very well may
 * have one of those modes, we just don't have enough information to prove
 * that it does for sure.
 */
static inline bool
nir_deref_mode_must_be(const nir_deref_instr *deref, nir_variable_mode modes)
{
   assert(!(modes & ~nir_var_all));
   assert(deref->modes != 0);
   return !(deref->modes & ~modes);
}

/** Returns true if deref has the given mode
 *
 * This returns true if the deref has exactly the mode specified.  If the
 * deref may have that mode but may also have a different mode (i.e. modes has
 * multiple bits set), this will assert-fail.
 *
 * If you're confused about which nir_deref_mode_ helper to use, use this one
 * or nir_deref_mode_is_one_of below.
 */
static inline bool
nir_deref_mode_is(const nir_deref_instr *deref, nir_variable_mode mode)
{
   assert(util_bitcount(mode) == 1 && (mode & nir_var_all));
   assert(deref->modes != 0);

   /* This is only for "simple" cases so, if modes might interact with this
    * deref then the deref has to have a single mode.
    */
   if (nir_deref_mode_may_be(deref, mode)) {
      assert(util_bitcount(deref->modes) == 1);
      assert(deref->modes == mode);
   }

   return deref->modes == mode;
}

/** Returns true if deref has one of the given modes
 *
 * This returns true if the deref has exactly one possible mode and that mode
 * is one of the modes specified.  If the deref may have one of those modes
 * but may also have a different mode (i.e. modes has multiple bits set), this
 * will assert-fail.
 */
static inline bool
nir_deref_mode_is_one_of(const nir_deref_instr *deref, nir_variable_mode modes)
{
   /* This is only for "simple" cases so, if modes might interact with this
    * deref then the deref has to have a single mode.
    */
   if (nir_deref_mode_may_be(deref, modes)) {
      assert(util_bitcount(deref->modes) == 1);
      assert(nir_deref_mode_must_be(deref, modes));
   }

   return nir_deref_mode_may_be(deref, modes);
}

/** Returns true if deref's possible modes lie in the given set of modes
 *
 * This returns true if the deref's modes lie in the given set of modes.  If
 * the deref's modes overlap with the specified modes but aren't entirely
 * contained in the specified set of modes, this will assert-fail.  In
 * particular, if this is used in a generic pointers scenario, the specified
 * modes has to contain all or none of the possible generic pointer modes.
 *
 * This is intended mostly for mass-lowering of derefs which might have
 * generic pointers.
 */
static inline bool
nir_deref_mode_is_in_set(const nir_deref_instr *deref, nir_variable_mode modes)
{
   if (nir_deref_mode_may_be(deref, modes))
      assert(nir_deref_mode_must_be(deref, modes));

   return nir_deref_mode_may_be(deref, modes);
}

static inline nir_deref_instr *nir_src_as_deref(nir_src src);

/** Returns true if deref->arr is valid */
static inline bool
nir_deref_instr_is_arr(const nir_deref_instr *deref)
{
   return deref->deref_type == nir_deref_type_array ||
          deref->deref_type == nir_deref_type_ptr_as_array;
}

static inline nir_deref_instr *
nir_deref_instr_parent(const nir_deref_instr *instr)
{
   if (instr->deref_type == nir_deref_type_var)
      return NULL;
   else
      return nir_src_as_deref(instr->parent);
}

bool nir_deref_instr_has_indirect(nir_deref_instr *instr);
bool nir_deref_instr_is_known_out_of_bounds(nir_deref_instr *instr);

typedef enum {
   nir_deref_instr_has_complex_use_allow_memcpy_src = (1 << 0),
   nir_deref_instr_has_complex_use_allow_memcpy_dst = (1 << 1),
   nir_deref_instr_has_complex_use_allow_atomics = (1 << 2),
} nir_deref_instr_has_complex_use_options;

bool nir_deref_instr_has_complex_use(nir_deref_instr *instr,
                                     nir_deref_instr_has_complex_use_options opts);

bool nir_deref_instr_remove_if_unused(nir_deref_instr *instr);

unsigned nir_deref_instr_array_stride(nir_deref_instr *instr);

typedef struct nir_call_instr {
   nir_instr instr;

   nir_function *callee;
   /* If this function call is indirect, the function pointer to call.
    * Otherwise, null initialized.
    */
   nir_src indirect_callee;

   unsigned num_params;
   nir_src params[];
} nir_call_instr;

#define NIR_CMAT_CALL_MAX_CONST_INDEX 1

typedef enum {
   /*
    * Cooperative matrix row/column reduce operation
    * reduce (dst, src) - const index row/col
    */
   nir_cmat_call_op_reduce,
   /*
    * Cooperative matrix reduce operation finish
    * for split flexible dimension matricies
    * reduce_finish (dst, src0, src1) - const index 0 row/col reduce
    */
   nir_cmat_call_op_reduce_finish,
   /*
    * Cooperative matrix 2x2 reduce operation
    * reduce 2x2 dst, src0, src1, src2, src3.
    */
   nir_cmat_call_op_reduce_2x2,
   /*
    * Cooperative matrix per-element operation call
    * per-element dst, row offset, col offset, src
    */
   nir_cmat_call_op_per_element_op,
} nir_cmat_call_op;

typedef struct nir_cmat_call_instr {
   nir_instr instr;

   nir_cmat_call_op op;

   nir_function *callee;
   int const_index[NIR_CMAT_CALL_MAX_CONST_INDEX];

   unsigned num_params;
   nir_src params[];
} nir_cmat_call_instr;

static inline nir_cmat_reduce nir_cmat_call_reduce_flags(nir_cmat_call_instr *call)
{
   return (nir_cmat_reduce)call->const_index[0];
}

#include "nir_intrinsics.h"

#define NIR_INTRINSIC_MAX_CONST_INDEX 9

/** Represents an intrinsic
 *
 * An intrinsic is an instruction type for handling things that are
 * more-or-less regular operations but don't just consume and produce SSA
 * values like ALU operations do.  Intrinsics are not for things that have
 * special semantic meaning such as phi nodes and parallel copies.
 * Examples of intrinsics include variable load/store operations, system
 * value loads, and the like.  Even though texturing more-or-less falls
 * under this category, texturing is its own instruction type because
 * trying to represent texturing with intrinsics would lead to a
 * combinatorial explosion of intrinsic opcodes.
 *
 * By having a single instruction type for handling a lot of different
 * cases, optimization passes can look for intrinsics and, for the most
 * part, completely ignore them.  Each intrinsic type also has a few
 * possible flags that govern whether or not they can be reordered or
 * eliminated.  That way passes like dead code elimination can still work
 * on intrisics without understanding the meaning of each.
 *
 * Each intrinsic has some number of constant indices, some number of
 * variables, and some number of sources.  What these sources, variables,
 * and indices mean depends on the intrinsic and is documented with the
 * intrinsic declaration in nir_intrinsics.h.  Intrinsics and texture
 * instructions are the only types of instruction that can operate on
 * variables.
 */
typedef struct nir_intrinsic_instr {
   nir_instr instr;

   nir_def def;

   nir_intrinsic_op intrinsic;

   /** number of components if this is a vectorized intrinsic
    *
    * Similarly to ALU operations, some intrinsics are vectorized.
    * An intrinsic is vectorized if nir_intrinsic_infos.dest_components == 0.
    * For vectorized intrinsics, the num_components field specifies the
    * number of destination components and the number of source components
    * for all sources with nir_intrinsic_infos.src_components[i] == 0.
    */
   uint8_t num_components;

   int const_index[NIR_INTRINSIC_MAX_CONST_INDEX];

   /* a variable name associated with this instr; cannot be modified or freed */
   const char *name;

   nir_src src[];
} nir_intrinsic_instr;

typedef enum {
   /* Memory ordering. */
   NIR_MEMORY_ACQUIRE = 1 << 0,
   NIR_MEMORY_RELEASE = 1 << 1,
   NIR_MEMORY_ACQ_REL = NIR_MEMORY_ACQUIRE | NIR_MEMORY_RELEASE,

   /* Memory visibility operations. */
   NIR_MEMORY_MAKE_AVAILABLE = 1 << 2,
   NIR_MEMORY_MAKE_VISIBLE = 1 << 3,

   /* Control barrier operations. If both of these are set, or neither are set
    * and the execution scope is not SCOPE_NONE, it's a combined arrive+wait
    * barrier.
    *
    * Because a barrier can be a control one without either of these, the best
    * way to see if it's a control one is to check the execution scope.
    */
   NIR_MEMORY_CONTROL_ARRIVE = 1 << 4,
   NIR_MEMORY_CONTROL_WAIT = 1 << 5,
   NIR_MEMORY_CONTROL_ARRIVE_WAIT = NIR_MEMORY_CONTROL_ARRIVE | NIR_MEMORY_CONTROL_WAIT,
} nir_memory_semantics;

/**
 * NIR intrinsics semantic flags
 *
 * information about what the compiler can do with the intrinsics.
 *
 * :c:member:`nir_intrinsic_info.flags`
 */
typedef enum {
   /**
    * whether the intrinsic can be safely eliminated if none of its output
    * value is not being used.
    */
   NIR_INTRINSIC_CAN_ELIMINATE = BITFIELD_BIT(0),

   /**
    * Whether the intrinsic can be reordered with respect to any other
    * intrinsic, i.e. whether the only reordering dependencies of the
    * intrinsic are due to the register reads/writes.
    */
   NIR_INTRINSIC_CAN_REORDER = BITFIELD_BIT(1),

   /**
    * Identifies any subgroup-like operation whose behaviour depends on other
    * logical threads. This is incompatible with CAN_REORDER.
    */
   NIR_INTRINSIC_SUBGROUP = BITFIELD_BIT(2),

   /**
    * Identifies an operation whose behaviour depends (only) on the local quad.
    * Any QUADGROUP intrinsic is also SUBGROUP.
    */
   NIR_INTRINSIC_QUADGROUP = BITFIELD_BIT(3),
} nir_intrinsic_semantic_flag;

/**
 * Maximum valid value for a nir align_mul value (in intrinsics or derefs).
 *
 * Offsets can be signed, so this is the largest power of two in int32_t.
 */
#define NIR_ALIGN_MUL_MAX 0x40000000

typedef struct nir_io_semantics {
   unsigned location : 7;  /* gl_vert_attrib, gl_varying_slot, or gl_frag_result */
   unsigned num_slots : 6; /* max 32, may be pessimistic with const indexing */
   unsigned dual_source_blend_index : 1; /* used without nir_io_use_frag_result_dual_src_blend */
   unsigned fb_fetch_output : 1;  /* for GL_KHR_blend_equation_advanced */
   unsigned fb_fetch_output_coherent : 1;
   unsigned gs_streams : 8;       /* xxyyzzww: 2-bit stream index for each component */
   unsigned medium_precision : 1; /* GLSL mediump qualifier */
   unsigned per_view : 1;
   unsigned high_16bits : 1; /* whether accessing low or high half of the slot */
   unsigned high_dvec2 : 1; /* whether accessing the high half of dvec3/dvec4 */
   /* CLIP_DISTn, LAYER, VIEWPORT, and TESS_LEVEL_* have up to 3 uses:
    * - an output consumed by the next stage
    * - a system value output affecting fixed-func hardware, e.g. the clipper
    * - a transform feedback output written to memory
    * The following fields disable the first two. Transform feedback is disabled
    * by transform feedback info.
    */
   unsigned no_varying : 1;       /* whether this output isn't consumed by the next stage */
   unsigned no_sysval_output : 1; /* whether this system value output has no
                                     effect due to current pipeline states */
   unsigned interp_explicit_strict : 1; /* preserve original vertex order */
   /* Skip nir_validate of the intrinsic. Any new code that sets it will ba NAK'd. */
   unsigned no_validate : 1;

   /* Start of the second uint. */
   unsigned no_signed_zero : 1; /* whether it matters if the input/output is -0.0 or +0.0. */
   unsigned clamp : 1; /* whether the IO component should be clamped when GL_CLAMP_VERTEX_COLOR is enabled */
   unsigned padding : 30;
} nir_io_semantics;

/* Transform feedback info for 4 outputs. */
typedef struct nir_io_xfb {
   struct {
      /* start_component is equal to the index of out[]; add 2 for io_xfb2 */
      /* start_component is not relative to nir_intrinsic_component */
      /* get the stream index from nir_io_semantics */
      uint8_t num_components : 4; /* max 4; if this is 0, xfb is disabled */
      uint8_t buffer : 4;         /* buffer index, max 3 */
      uint8_t offset;             /* transform feedback buffer offset in dwords,
                                     max (1K - 4) bytes */
   } out[4];
} nir_io_xfb;

unsigned
nir_instr_xfb_write_mask(nir_intrinsic_instr *instr);

#define NIR_INTRINSIC_MAX_INPUTS 11

typedef struct nir_intrinsic_info {
   const char *name;

   /** number of register/SSA inputs */
   uint8_t num_srcs;

   /** number of components of each input register
    *
    * If this value is 0, the number of components is given by the
    * num_components field of nir_intrinsic_instr.  If this value is -1, the
    * intrinsic consumes however many components are provided and it is not
    * validated at all.
    */
   int8_t src_components[NIR_INTRINSIC_MAX_INPUTS];

   bool has_dest;

   /** number of components of the output register
    *
    * If this value is 0, the number of components is given by the
    * num_components field of nir_intrinsic_instr.
    */
   uint8_t dest_components;

   /** bitfield of legal bit sizes */
   uint8_t dest_bit_sizes;

   /** source which the destination bit size must match
    *
    * Some intrinsics, such as subgroup intrinsics, are data manipulation
    * intrinsics and they have similar bit-size rules to ALU ops. This enables
    * validation to validate a bit more and enables auto-generated builder code
    * to properly determine destination bit sizes automatically.
    */
   int8_t bit_size_src;

   /** the number of constant indices used by the intrinsic */
   uint8_t num_indices;

   /** the number of 32bit slots used for storing constant indices. */
   uint8_t num_index_slots;

   /** list of indices */
   uint8_t indices[NIR_INTRINSIC_MAX_CONST_INDEX];

   /** indicates the usage of intr->const_index[n] */
   uint8_t index_map[NIR_INTRINSIC_NUM_INDEX_FLAGS];

   /** semantic flags for calls to this intrinsic */
   nir_intrinsic_semantic_flag flags;
} nir_intrinsic_info;

extern const nir_intrinsic_info nir_intrinsic_infos[nir_num_intrinsics];

unsigned
nir_intrinsic_src_components(const nir_intrinsic_instr *intr, unsigned srcn);

unsigned
nir_intrinsic_dest_components(nir_intrinsic_instr *intr);

nir_alu_type
nir_intrinsic_instr_src_type(const nir_intrinsic_instr *intrin, unsigned src);

nir_alu_type
nir_intrinsic_instr_dest_type(const nir_intrinsic_instr *intrin);

/**
 * Helper to copy const_index[] from src to dst, without assuming they
 * match in order.
 */
void nir_intrinsic_copy_const_indices(nir_intrinsic_instr *dst, nir_intrinsic_instr *src);

#include "nir_intrinsics_indices.h"

static inline void
nir_intrinsic_set_align(nir_intrinsic_instr *intrin,
                        unsigned align_mul, unsigned align_offset)
{
   assert(util_is_power_of_two_nonzero(align_mul));
   assert(align_offset < align_mul);
   nir_intrinsic_set_align_mul(intrin, align_mul);
   nir_intrinsic_set_align_offset(intrin, align_offset);
}

/** Returns a simple alignment for an align_mul/offset pair
 *
 * This helper converts from the full mul+offset alignment scheme used by
 * most NIR intrinsics to a simple alignment.  The returned value is the
 * largest power of two which divides both align_mul and align_offset.
 * For any offset X which satisfies the complex alignment described by
 * align_mul/offset, X % align == 0.
 */
static inline uint32_t
nir_combined_align(uint32_t align_mul, uint32_t align_offset)
{
   assert(util_is_power_of_two_nonzero(align_mul));
   assert(align_offset < align_mul);
   return align_offset ? 1 << (ffs(align_offset) - 1) : align_mul;
}

/** Returns a simple alignment for a load/store intrinsic offset
 *
 * Instead of the full mul+offset alignment scheme provided by the ALIGN_MUL
 * and ALIGN_OFFSET parameters, this helper takes both into account and
 * provides a single simple alignment parameter.  The offset X is guaranteed
 * to satisfy X % align == 0.
 */
static inline unsigned
nir_intrinsic_align(const nir_intrinsic_instr *intrin)
{
   return nir_combined_align(nir_intrinsic_align_mul(intrin),
                             nir_intrinsic_align_offset(intrin));
}

static inline bool
nir_intrinsic_has_align(const nir_intrinsic_instr *intrin)
{
   return nir_intrinsic_has_align_mul(intrin) &&
          nir_intrinsic_has_align_offset(intrin);
}

unsigned
nir_image_intrinsic_coord_components(const nir_intrinsic_instr *instr);

/* Converts an image intrinsic into a different type. */
void nir_rewrite_image_intrinsic(nir_intrinsic_instr *instr,
                                 nir_def *handle, nir_image_intrinsic_type type);

/* Determine if an intrinsic can be arbitrarily reordered and eliminated. */
bool nir_intrinsic_can_reorder(nir_intrinsic_instr *instr);
bool nir_instr_can_speculate(nir_instr *instr);
bool nir_intrinsic_writes_external_memory(const nir_intrinsic_instr *instr);

static inline bool
nir_intrinsic_has_semantic(const nir_intrinsic_instr *intr,
                           nir_intrinsic_semantic_flag flag)
{
   return nir_intrinsic_infos[intr->intrinsic].flags & flag;
}

static inline bool
nir_intrinsic_is_ray_query(nir_intrinsic_op intrinsic)
{
   switch (intrinsic) {
   case nir_intrinsic_rq_confirm_intersection:
   case nir_intrinsic_rq_generate_intersection:
   case nir_intrinsic_rq_initialize:
   case nir_intrinsic_rq_load:
   case nir_intrinsic_rq_proceed:
   case nir_intrinsic_rq_terminate:
      return true;
   default:
      return false;
   }
}

/** Texture instruction source type */
typedef enum nir_tex_src_type {
   /** Texture coordinate
    *
    * Must have :c:member:`nir_tex_instr.coord_components` components.
    */
   nir_tex_src_coord,

   /** Projector
    *
    * The texture coordinate (except for the array component, if any) is
    * divided by this value before LOD computation and sampling.
    *
    * Must be a float scalar.
    */
   nir_tex_src_projector,

   /** Shadow comparator
    *
    * For shadow sampling, the fetched texel values are compared against the
    * shadow comparator using the compare op specified by the sampler object
    * and converted to 1.0 if the comparison succeeds and 0.0 if it fails.
    * Interpolation happens after this conversion so the actual result may be
    * anywhere in the range [0.0, 1.0].
    *
    * Only valid if :c:member:`nir_tex_instr.is_shadow` and must be a float
    * scalar.
    */
   nir_tex_src_comparator,

   /** Coordinate offset
    *
    * An integer value that is added to the texel address before sampling.
    * This is only allowed with operations that take an explicit LOD as it is
    * applied in integer texel space after LOD selection and not normalized
    * coordinate space.
    */
   nir_tex_src_offset,

   /** LOD bias
    *
    * This value is added to the computed LOD before mip-mapping.
    */
   nir_tex_src_bias,

   /** Explicit LOD */
   nir_tex_src_lod,

   /** Min LOD
    *
    * The computed LOD is clamped to be at least as large as min_lod before
    * mip-mapping.
    */
   nir_tex_src_min_lod,

   /** Max LOD
    *
    * The computed LOD is clamped to be at most as large as max_lod before
    * mip-mapping.
    */
   nir_tex_src_max_lod_kk,

   /** LOD bias + min LOD packed together into 32-bits. This is the common case
    * for texturing on Honeykrisp with DX12, where both LOD bias and min LOD are
    * emulated and passed in a single hardware source together. So it's
    * important to optimize so e.g. nir_opt_preamble can make good decisions
    * that avoid extra moves.
    */
   nir_tex_src_lod_bias_min_agx,

   /** MSAA sample index */
   nir_tex_src_ms_index,

   /** Intel-specific MSAA compression data */
   nir_tex_src_ms_mcs_intel,

   /** Explicit horizontal (X-major) coordinate derivative */
   nir_tex_src_ddx,

   /** Explicit vertical (Y-major) coordinate derivative */
   nir_tex_src_ddy,

   /** Texture variable dereference */
   nir_tex_src_texture_deref,

   /** Sampler variable dereference */
   nir_tex_src_sampler_deref,

   /** Texture index offset
    *
    * This is added to :c:member:`nir_tex_instr.texture_index`.  Unless
    * :c:member:`nir_tex_instr.texture_non_uniform` is set, this is guaranteed
    * to be dynamically uniform.
    */
   nir_tex_src_texture_offset,

   /** Dynamically uniform sampler index offset
    *
    * This is added to :c:member:`nir_tex_instr.sampler_index`.  Unless
    * :c:member:`nir_tex_instr.sampler_non_uniform` is set, this is guaranteed to be
    * dynamically uniform.  This should not be present until GLSL ES 3.20, GLSL
    * 4.00, or ARB_gpu_shader5, because in ES 3.10 and GL 3.30 samplers said
    * "When aggregated into arrays within a shader, samplers can only be indexed
    * with a constant integral expression."
    */
   nir_tex_src_sampler_offset,

   /** Bindless texture handle
    *
    * This is, unfortunately, a bit overloaded at the moment.  There are
    * generally two types of bindless handles:
    *
    *  1. For GL_ARB_bindless bindless handles. These are part of the
    *     GL/Gallium-level API and are always a 64-bit integer.
    *
    *  2. HW-specific handles.  GL_ARB_bindless handles may be lowered to
    *     these.  Also, these are used by many Vulkan drivers to implement
    *     descriptor sets, especially for UPDATE_AFTER_BIND descriptors.
    *     The details of hardware handles (bit size, format, etc.) is
    *     HW-specific.
    *
    * Because of this overloading and the resulting ambiguity, we currently
    * don't validate anything for these.
    */
   nir_tex_src_texture_handle,

   /** Bindless sampler handle
    *
    * See nir_tex_src_texture_handle,
    */
   nir_tex_src_sampler_handle,

   /** Texture descriptor heap offset (in bytes) */
   nir_tex_src_texture_heap_offset,

   /** Sampler descriptor heap offset (in bytes) */
   nir_tex_src_sampler_heap_offset,

   /** Tex src intrinsic
    *
    * This is an intrinsic used before function inlining i.e. before we know
    * if a bindless value has been given as function param for use as a tex
    * src.
    */
   nir_tex_src_sampler_deref_intrinsic,
   nir_tex_src_texture_deref_intrinsic,

   /** Plane index for multi-plane YCbCr textures */
   nir_tex_src_plane,

   /**
    * Backend-specific vec4 tex src argument.
    *
    * Can be used to have NIR optimization (copy propagation, lower_vec_to_regs)
    * apply to the packing of the tex srcs.  This lowering must only happen
    * after nir_lower_tex().
    *
    * The nir_tex_instr_src_type() of this argument is float, so no lowering
    * will happen if nir_lower_int_to_float is used.
    */
   nir_tex_src_backend1,

   /** Second backend-specific vec4 tex src argument, see nir_tex_src_backend1. */
   nir_tex_src_backend2,

   /** VK_QCOM_image_processing sources */
   nir_tex_src_ref_coord,
   nir_tex_src_texture_2_deref,
   nir_tex_src_sampler_2_deref,
   nir_tex_src_texture_2_handle,
   nir_tex_src_sampler_2_handle,
   nir_tex_src_block_size,
   nir_tex_src_box_size,

   nir_num_tex_src_types
} nir_tex_src_type;

/** A texture instruction source */
typedef struct nir_tex_src {
   /** Base source */
   nir_src src;

   /** Type of this source */
   nir_tex_src_type src_type;
} nir_tex_src;

/** Texture instruction opcode */
typedef enum nir_texop {
   /** Regular texture look-up */
   nir_texop_tex,
   /** Texture look-up with LOD bias */
   nir_texop_txb,
   /** Texture look-up with explicit LOD */
   nir_texop_txl,
   /** Texture look-up with partial derivatives */
   nir_texop_txd,
   /** Texel fetch with explicit LOD */
   nir_texop_txf,
   /** Multisample texture fetch */
   nir_texop_txf_ms,
   /** Multisample texture fetch from framebuffer */
   nir_texop_txf_ms_fb,
   /** Multisample compression value fetch */
   nir_texop_txf_ms_mcs_intel,
   /** Texture size */
   nir_texop_txs,
   /** Texture lod query */
   nir_texop_lod,
   /** Texture gather */
   nir_texop_tg4,
   /** Texture levels query */
   nir_texop_query_levels,
   /** Texture samples query */
   nir_texop_texture_samples,
   /** Query whether all samples are definitely identical. */
   nir_texop_samples_identical,
   /** Regular texture look-up, eligible for pre-dispatch */
   nir_texop_tex_prefetch,
   /** Returns the sampler's LOD bias (if sampler LOD bias is lowered) */
   nir_texop_lod_bias,
   /** Multisample fragment color texture fetch */
   nir_texop_fragment_fetch_amd,
   /** Multisample fragment mask texture fetch */
   nir_texop_fragment_mask_fetch_amd,
   /** Returns a buffer or image descriptor. */
   nir_texop_descriptor_amd,
   /** Returns a sampler descriptor. */
   nir_texop_sampler_descriptor_amd,
   /** Returns the image view's min LOD */
   nir_texop_image_min_lod_agx,
   /** Returns a bool indicating that the sampler uses a custom border colour */
   nir_texop_has_custom_border_color_agx,
   /** Returns the sampler's custom border colour (if has_custom_border_agx) */
   nir_texop_custom_border_color_agx,
   /** Maps to TXQ.DIMENSION */
   nir_texop_hdr_dim_nv,
   /** Maps to TXQ.TEXTURE_TYPE */
   nir_texop_tex_type_nv,
   /** Maps to TXQ.SAMPLER_POS */
   nir_texop_sample_pos_nv,
   /** Maps to TEX_GRADIENT */
   nir_texop_gradient_pan,
   /**
    * Returns the weighted average of a region of texels in the texture, using
    * the filter kernel sampled from ref_texture. (VK_QCOM_image_processing)
   */
   nir_texop_sample_weighted_qcom,
   /**
    * Returns the result of a weighted average of the texels in a box of size
    * box_size centered at coord. (VK_QCOM_image_processing)
   */
   nir_texop_box_filter_qcom,
   /**
    * Returns the result of SAD/SSD block matching on 2D textures.
    * (VK_QCOM_image_processing)
    *
    * The textures are always 2D dim, and the coord and ref_coord texture
    * sources are ivec2s.  The size of the block is specified in the block_size
    * texture source (uvec2 from SPIRV, packed u32 in the backend)
   */
   nir_texop_block_match_sad_qcom,
   nir_texop_block_match_ssd_qcom,
   /** txs in .xyz and query_levels in .w */
   nir_texop_resinfo_intel,

   /** Returns red data and a sparse residency code for sampling ops */
   nir_texop_sparse_residency_intel,
   /** Returns red data and a sparse residency code for texel fetches */
   nir_texop_sparse_residency_txf_intel,
} nir_texop;

/** Represents a texture instruction */
typedef struct nir_tex_instr {
   /** Base instruction */
   nir_instr instr;

   /** Destination */
   nir_def def;

   /** Dimensionality of the texture operation
    *
    * This will typically match the dimensionality of the texture deref type
    * if a nir_tex_src_texture_deref is present.  However, it may not if
    * texture lowering has occurred.
    */
   enum glsl_sampler_dim sampler_dim;

   /** ALU type of the destination
    *
    * This is the canonical sampled type for this texture operation and may
    * not exactly match the sampled type of the deref type when a
    * nir_tex_src_texture_deref is present.  For OpenCL, the sampled type of
    * the texture deref will be GLSL_TYPE_VOID and this is allowed to be
    * anything.  With SPIR-V, the signedness of integer types is allowed to
    * differ.  For all APIs, the bit size may differ if the driver has done
    * any sort of mediump or similar lowering since texture types always have
    * 32-bit sampled types.
    */
   nir_alu_type dest_type;

   /** Texture opcode */
   nir_texop op;

   /** Array of sources
    *
    * This array has :c:member:`nir_tex_instr.num_srcs` elements
    */
   nir_tex_src *src;

   /** Number of sources */
   unsigned num_srcs;

   /** Number of components in the coordinate, if any.
    *
    * This applies to the nir_tex_src_coord and nir_tex_src_ref_coord src types.
    */
   unsigned coord_components;

   /** True if the texture instruction acts on an array texture */
   bool is_array;

   /** True if the texture instruction performs a shadow comparison
    *
    * If this is true, the texture instruction must have a
    * nir_tex_src_comparator.
    */
   bool is_shadow;

   /**
    * If is_shadow is true, whether this is the old-style shadow that outputs
    * 4 components or the new-style shadow that outputs 1 component.
    */
   bool is_new_style_shadow;

   /**
    * True if this texture instruction should return a sparse residency code.
    * The code is in the last component of the result.
    */
   bool is_sparse;

   /** nir_texop_tg4 component selector
    *
    * This determines which RGBA component is gathered.
    */
   unsigned component : 2;

   /** Validation needs to know this for gradient component count */
   unsigned array_is_lowered_cube : 1;

   /** True if this tg4 instruction has an implicit LOD or LOD bias, instead of using level 0 */
   unsigned is_gather_implicit_lod : 1;

   /** True if helper invocation loads can be skipped
    *
    * This is set by nir_opt_load_skip_helpers().
    */
   unsigned skip_helpers : 1;

   /** Gather offsets */
   int8_t tg4_offsets[4][2];

   /** True if the texture index or handle is not dynamically uniform */
   bool texture_non_uniform;

   /** True if the sampler index or handle is not dynamically uniform.
    *
    * This may be set when VK_EXT_descriptor_indexing is supported and the
    * appropriate capability is enabled.
    *
    * This should always be false in GLSL (GLSL ES 3.20 says "When aggregated
    * into arrays within a shader, opaque types can only be indexed with a
    * dynamically uniform integral expression", and GLSL 4.60 says "When
    * aggregated into arrays within a shader, [texture, sampler, and
    * samplerShadow] types can only be indexed with a dynamically uniform
    * expression, or texture lookup will result in undefined values.").
    */
   bool sampler_non_uniform;

   /** True if this texture instruction uses an embedded sampler.
    *
    * In this case, sampler_index is the index in embedded sampler table.
    */
   bool embedded_sampler;

   /** True if the offset is not dynamically uniform */
   bool offset_non_uniform;

   /** True whether this returns the same result anywhere in the shader and
    *  doesn't cause page faults.
    */
   bool can_speculate;

   /** The texture index
    *
    * If this texture instruction has a nir_tex_src_texture_offset source,
    * then the texture index is given by texture_index + texture_offset.
    */
   unsigned texture_index;

   /** The sampler index
    *
    * The following operations do not require a sampler and, as such, this
    * field should be ignored:
    *
    *    - nir_texop_txf
    *    - nir_texop_txf_ms
    *    - nir_texop_txs
    *    - nir_texop_query_levels
    *    - nir_texop_texture_samples
    *    - nir_texop_samples_identical
    *
    * If this texture instruction has a nir_tex_src_sampler_offset source,
    * then the sampler index is given by sampler_index + sampler_offset.
    */
   unsigned sampler_index;

   /* Back-end specific flags, intended to be used in combination with
    * nir_tex_src_backend1/2 to provide additional hw-specific information
    * to the back-end compiler.
    */
   uint32_t backend_flags;
} nir_tex_instr;

/**
 * Returns true if the texture operation requires a sampler as a general rule
 *
 * Note that the specific hw/driver backend could require to a sampler
 * object/configuration packet in any case, for some other reason.
 *
 * See also :c:member:`nir_tex_instr.sampler_index`.
 */
bool nir_tex_instr_need_sampler(const nir_tex_instr *instr);

/** Returns the number of components returned by this nir_tex_instr
 *
 * Useful for code building texture instructions when you don't want to think
 * about how many components a particular texture op returns.  This does not
 * include the sparse residency code.
 */
unsigned
nir_tex_instr_result_size(const nir_tex_instr *instr);

/**
 * Returns the destination size of this nir_tex_instr including the sparse
 * residency code, if any.
 */
static inline unsigned
nir_tex_instr_dest_size(const nir_tex_instr *instr)
{
   /* One more component is needed for the residency code. */
   return nir_tex_instr_result_size(instr) + instr->is_sparse;
}

/**
 * Returns true if this texture operation queries something about the texture
 * rather than actually sampling it.
 */
bool
nir_tex_instr_is_query(const nir_tex_instr *instr);

/** Returns true if this texture instruction does implicit derivatives
 *
 * This is important as there are extra control-flow rules around derivatives
 * and texture instructions which perform them implicitly.
 */
bool
nir_tex_instr_has_implicit_derivative(const nir_tex_instr *instr);

/** Returns the ALU type of the given texture instruction source */
nir_alu_type
nir_tex_instr_src_type(const nir_tex_instr *instr, unsigned src);

/**
 * Returns the number of components required by the given texture instruction
 * source
 */
unsigned
nir_tex_instr_src_size(const nir_tex_instr *instr, unsigned src);

/**
 * Returns the index of the texture instruction source with the given
 * nir_tex_src_type or -1 if no such source exists.
 */
static inline int
nir_tex_instr_src_index(const nir_tex_instr *instr, nir_tex_src_type type)
{
   for (unsigned i = 0; i < instr->num_srcs; i++)
      if (instr->src[i].src_type == type)
         return (int)i;

   return -1;
}

/** Adds a source to a texture instruction */
void nir_tex_instr_add_src(nir_tex_instr *tex,
                           nir_tex_src_type src_type,
                           nir_def *src);

/** Removes a source from a texture instruction */
void nir_tex_instr_remove_src(nir_tex_instr *tex, unsigned src_idx);

static inline nir_def *
nir_get_tex_src(nir_tex_instr *tex, nir_tex_src_type type_)
{
   int idx = nir_tex_instr_src_index(tex, type_);
   if (idx < 0)
      return NULL;

   return tex->src[idx].src.ssa;
}

static inline nir_deref_instr *
nir_get_tex_deref(nir_tex_instr *tex, nir_tex_src_type type_)
{
   int idx = nir_tex_instr_src_index(tex, type_);
   if (idx < 0)
      return NULL;

   return nir_src_as_deref(tex->src[idx].src);
}

/*
 * Find a texture source, remove it, and return its nir_def. If the texture
 * source does not exist, return NULL. This is useful for texture lowering pass
 * that consume their input sources and produce a new lowered source.
 */
static inline nir_def *
nir_steal_tex_src(nir_tex_instr *tex, nir_tex_src_type type_)
{
   int idx = nir_tex_instr_src_index(tex, type_);
   if (idx < 0)
      return NULL;

   nir_def *ssa = tex->src[idx].src.ssa;
   nir_tex_instr_remove_src(tex, idx);
   return ssa;
}

static inline nir_deref_instr *
nir_steal_tex_deref(nir_tex_instr *tex, nir_tex_src_type type_)
{
   int idx = nir_tex_instr_src_index(tex, type_);
   if (idx < 0)
      return NULL;

   nir_deref_instr *deref = nir_src_as_deref(tex->src[idx].src);
   nir_tex_instr_remove_src(tex, idx);
   return deref;
}

bool nir_tex_instr_has_explicit_tg4_offsets(nir_tex_instr *tex);

typedef struct nir_load_const_instr {
   nir_instr instr;

   nir_def def;

   nir_const_value value[];
} nir_load_const_instr;

typedef enum {
   /** Return from a function
    *
    * This instruction is a classic function return.  It jumps to
    * nir_function_impl::end_block.  No return value is provided in this
    * instruction.  Instead, the function is expected to write any return
    * data to a deref passed in from the caller.
    */
   nir_jump_return,

   /** Immediately exit the current shader
    *
    * This instruction is roughly the equivalent of C's "exit()" in that it
    * immediately terminates the current shader invocation.  From a CFG
    * perspective, it looks like a jump to nir_function_impl::end_block but
    * it actually jumps to the end block of the shader entrypoint.  A halt
    * instruction in the shader entrypoint itself is semantically identical
    * to a return.
    *
    * For shaders with built-in I/O, any outputs written prior to a halt
    * instruction remain written and any outputs not written prior to the
    * halt have undefined values.  It does NOT cause an implicit discard of
    * written results.  If one wants discard results in a fragment shader,
    * for instance, a discard or demote intrinsic is required.
    */
   nir_jump_halt,

   /** Immediately exit the current shader due to a fatal error
    *
    * This has the same CFG semantics as nir_jump_halt — it jumps to the end
    * block of the shader entrypoint — but carries the additional semantic
    * that the invocation is terminating because a fatal error was detected
    * (e.g. nir_intrinsic_abort wrote a message to the abort buffer).
    *
    * Backends that support hardware-level abort signalling (device fault,
    * special sendmsg, etc.) may generate different code for nir_jump_abort
    * vs nir_jump_halt.  Backends that do not distinguish the two can lower
    * nir_jump_abort to nir_jump_halt before instruction selection.
    */
   nir_jump_abort,

   /** Break out of the inner-most loop
    *
    * This has the same semantics as C's "break" statement.
    */
   nir_jump_break,

   /** Jump back to the top of the inner-most loop
    *
    * This has the same semantics as C's "continue" statement assuming that a
    * NIR loop is implemented as "while (1) { body }".
    */
   nir_jump_continue,

   /** Jumps for unstructured CFG.
    *
    * As within an unstructured CFG we can't rely on block ordering we need to
    * place explicit jumps at the end of every block.
    */
   nir_jump_goto,
   nir_jump_goto_if,
} nir_jump_type;

typedef struct nir_jump_instr {
   nir_instr instr;
   nir_jump_type type;
   nir_src condition;
   nir_block *target;
   nir_block *else_target;
} nir_jump_instr;

/* creates a new SSA variable in an undefined state */

typedef struct nir_undef_instr {
   nir_instr instr;
   nir_def def;
} nir_undef_instr;

typedef struct nir_phi_src {
   struct exec_node node;

   /* The predecessor block corresponding to this source */
   nir_block *pred;

   nir_src src;
} nir_phi_src;

#define nir_foreach_phi_src(phi_src, phi) \
   foreach_list_typed(nir_phi_src, phi_src, node, &(phi)->srcs)
#define nir_foreach_phi_src_safe(phi_src, phi) \
   foreach_list_typed_safe(nir_phi_src, phi_src, node, &(phi)->srcs)

typedef struct nir_phi_instr {
   nir_instr instr;

   nir_def def;

   /** list of nir_phi_src */
   struct exec_list srcs;
} nir_phi_instr;

static inline nir_phi_src *
nir_phi_get_src_from_block(nir_phi_instr *phi, nir_block *block)
{
   nir_foreach_phi_src(src, phi) {
      if (src->pred == block)
         return src;
   }

   assert(!"Block is not a predecessor of phi.");
   return NULL;
}

const nir_instr *nir_def_instr_noninline(const nir_def *def);

static inline nir_instr *
nir_def_instr_nonconst(nir_def *def)
{
   static_assert(offsetof(nir_alu_instr, def) == offsetof(nir_undef_instr, def),
                 "nir_alu_instr: nir_def always has to be at the same offset relative to nir_instr.");
   static_assert(offsetof(nir_deref_instr, def) == offsetof(nir_undef_instr, def),
                 "nir_deref_instr: nir_def always has to be at the same offset relative to nir_instr.");
   static_assert(offsetof(nir_tex_instr, def) == offsetof(nir_undef_instr, def),
                 "nir_tex_instr: nir_def always has to be at the same offset relative to nir_instr.");
   static_assert(offsetof(nir_intrinsic_instr, def) == offsetof(nir_undef_instr, def),
                 "nir_intrinsic_instr: nir_def always has to be at the same offset relative to nir_instr.");
   static_assert(offsetof(nir_load_const_instr, def) == offsetof(nir_undef_instr, def),
                 "nir_load_const_instr: nir_def always has to be at the same offset relative to nir_instr.");
   static_assert(offsetof(nir_phi_instr, def) == offsetof(nir_undef_instr, def),
                 "nir_phi_instr: nir_def always has to be at the same offset relative to nir_instr.");
   return &container_of(def, nir_undef_instr, def)->instr;
}

static inline const nir_instr *
nir_def_instr_const(const nir_def *def)
{
   return (const nir_instr *)nir_def_instr_nonconst((nir_def *)def);
}

#ifdef __cplusplus
/* Required for function overloading */
extern "C++" {

static inline nir_instr *
nir_def_instr(nir_def *def)
{
   return nir_def_instr_nonconst(def);
}

static inline const nir_instr *
nir_def_instr(const nir_def *def)
{
   return nir_def_instr_const(def);
}
}
#else
/*
 * For C we can use _Generic to overload on the constantness properly.
 */
#define nir_def_instr(def) _Generic((def), \
   const nir_def *: nir_def_instr_const,   \
   nir_def *: nir_def_instr_nonconst)(def)
#endif

static inline nir_block *
nir_def_block(nir_def *def)
{
   return nir_def_instr(def)->block;
}

static inline nir_instr_type
nir_def_instr_type(const nir_def *def)
{
   return nir_def_instr(def)->type;
}

/* This struct contains metadata for correlating the final nir shader
 * (after many lowering and optimization passes) with the source spir-v
 * or glsl. To avoid adding unnecessary overhead when the driver does not
 * preserve non-semantic information (which is the common case), debug
 * information is allocated before the instruction:
 *
 * +-------------------+-----------+--------------------------------+
 * | Debug information | nir_instr | Instruction type specific data |
 * +-------------------+-----------+--------------------------------+
 *
 * This is only allocated if nir_shader::has_debug_info is set. Accesses
 * to nir_instr_debug_info should therefore check nir_shader::has_debug_info
 * or nir_instr::has_debug_info.
 */
typedef struct nir_instr_debug_info {
   /* Path to the source file this instruction originates from. */
   char *filename;
   /* 0 if uninitialized. */
   uint32_t line;
   uint32_t column;
   uint32_t spirv_offset;

   /* Line in the output of nir_print_shader. 0 if uninitialized. */
   uint32_t nir_line;

   /* Contains the name of the variable/input/... that was lowered to this
    * def by a pass like nir_lower_vars_to_ssa.
    */
   char *variable_name;

   /* The nir_instr has to be the last field since it has a varying size. */
   nir_instr instr;
} nir_instr_debug_info;

typedef struct nir_scalar {
   nir_def *def;
   unsigned comp;
} nir_scalar;

NIR_DEFINE_CAST(nir_instr_as_alu, nir_instr, nir_alu_instr, instr,
                type, nir_instr_type_alu)
NIR_DEFINE_CAST(nir_instr_as_deref, nir_instr, nir_deref_instr, instr,
                type, nir_instr_type_deref)
NIR_DEFINE_CAST(nir_instr_as_call, nir_instr, nir_call_instr, instr,
                type, nir_instr_type_call)
NIR_DEFINE_CAST(nir_instr_as_jump, nir_instr, nir_jump_instr, instr,
                type, nir_instr_type_jump)
NIR_DEFINE_CAST(nir_instr_as_tex, nir_instr, nir_tex_instr, instr,
                type, nir_instr_type_tex)
NIR_DEFINE_CAST(nir_instr_as_intrinsic, nir_instr, nir_intrinsic_instr, instr,
                type, nir_instr_type_intrinsic)
NIR_DEFINE_CAST(nir_instr_as_load_const, nir_instr, nir_load_const_instr, instr,
                type, nir_instr_type_load_const)
NIR_DEFINE_CAST(nir_instr_as_undef, nir_instr, nir_undef_instr, instr,
                type, nir_instr_type_undef)
NIR_DEFINE_CAST(nir_instr_as_phi, nir_instr, nir_phi_instr, instr,
                type, nir_instr_type_phi)
NIR_DEFINE_CAST(nir_instr_as_cmat_call, nir_instr, nir_cmat_call_instr, instr,
                type, nir_instr_type_cmat_call)

#define NIR_DEFINE_DEF_AS_INSTR(instr_type, suffix, cast)                    \
   static inline instr_type *nir_def_as_##cast(const nir_def *def)           \
   {                                                                         \
      return nir_instr_as_##cast(nir_def_instr(def));                 \
   }                                                                         \
                                                                             \
   static inline bool nir_def_is_##suffix(const nir_def *def)                \
   {                                                                         \
      return nir_def_instr(def)->type == nir_instr_type_##cast;       \
   }                                                                         \
                                                                             \
   static inline instr_type *nir_def_as_##cast##_or_null(const nir_def *def) \
   {                                                                         \
      return nir_def_is_##suffix(def) ? nir_def_as_##cast(def) : NULL;       \
   }                                                                         \
                                                                             \
   static inline bool nir_src_is_##suffix(nir_src src)                       \
   {                                                                         \
      return nir_def_is_##suffix(src.ssa);                                   \
   }                                                                         \
                                                                             \
   static inline bool nir_scalar_is_##suffix(nir_scalar s)                   \
   {                                                                         \
      return nir_def_is_##suffix(s.def);                                     \
   }                                                                         \
                                                                             \
   static inline instr_type *nir_src_as_##cast(nir_src src)                  \
   {                                                                         \
      return nir_src_is_##suffix(src) ? nir_def_as_##cast(src.ssa) : NULL;   \
   }                                                                         \
                                                                             \
   static inline instr_type *nir_scalar_as_##cast(nir_scalar s)              \
   {                                                                         \
      return nir_def_is_##suffix(s.def) ? nir_def_as_##cast(s.def) : NULL;   \
   }

NIR_DEFINE_DEF_AS_INSTR(nir_alu_instr, alu, alu)
NIR_DEFINE_DEF_AS_INSTR(nir_intrinsic_instr, intrinsic, intrinsic)
NIR_DEFINE_DEF_AS_INSTR(nir_tex_instr, tex, tex)
NIR_DEFINE_DEF_AS_INSTR(nir_phi_instr, phi, phi)
NIR_DEFINE_DEF_AS_INSTR(nir_deref_instr, deref, deref)
NIR_DEFINE_DEF_AS_INSTR(nir_load_const_instr, const, load_const)
NIR_DEFINE_DEF_AS_INSTR(nir_undef_instr, undef, undef)

#undef NIR_DEFINE_DEF_AS_INSTR

#define NIR_DEFINE_SRC_AS_CONST(type, suffix)                 \
   static inline type                                         \
   nir_src_comp_as_##suffix(nir_src src, unsigned comp)       \
   {                                                          \
      assert(nir_src_is_const(src));                          \
      nir_load_const_instr *load =                            \
         nir_def_as_load_const(src.ssa);                      \
      assert(comp < load->def.num_components);                \
      return nir_const_value_as_##suffix(load->value[comp],   \
                                         load->def.bit_size); \
   }                                                          \
                                                              \
   static inline type                                         \
   nir_src_as_##suffix(nir_src src)                           \
   {                                                          \
      assert(nir_src_num_components(src) == 1);               \
      return nir_src_comp_as_##suffix(src, 0);                \
   }

NIR_DEFINE_SRC_AS_CONST(int64_t, int)
NIR_DEFINE_SRC_AS_CONST(uint64_t, uint)
NIR_DEFINE_SRC_AS_CONST(bool, bool)
NIR_DEFINE_SRC_AS_CONST(double, float)

#undef NIR_DEFINE_SRC_AS_CONST

static inline bool
nir_src_is_zero(nir_src src)
{
   if (!nir_src_is_const(src))
      return false;

   for (unsigned i = 0; i < nir_src_num_components(src); i++) {
      if (nir_src_comp_as_uint(src, i) != 0)
         return false;
   }

   return true;
}

static inline nir_const_value
nir_scalar_as_const_value(nir_scalar s)
{
   assert(s.comp < s.def->num_components);
   nir_load_const_instr *load = nir_scalar_as_load_const(s);
   return load->value[s.comp];
}

#define NIR_DEFINE_SCALAR_AS_CONST(type, suffix)         \
   static inline type                                    \
      nir_scalar_as_##suffix(nir_scalar s)               \
   {                                                     \
      return nir_const_value_as_##suffix(                \
         nir_scalar_as_const_value(s), s.def->bit_size); \
   }

NIR_DEFINE_SCALAR_AS_CONST(int64_t, int)
NIR_DEFINE_SCALAR_AS_CONST(uint64_t, uint)
NIR_DEFINE_SCALAR_AS_CONST(bool, bool)
NIR_DEFINE_SCALAR_AS_CONST(double, float)

#undef NIR_DEFINE_SCALAR_AS_CONST

static inline bool
nir_scalar_is_zero(nir_scalar s)
{
   return nir_scalar_is_const(s) && nir_scalar_as_uint(s) == 0;
}

static inline nir_op
nir_scalar_alu_op(nir_scalar s)
{
   return nir_def_as_alu(s.def)->op;
}

static inline nir_intrinsic_op
nir_scalar_intrinsic_op(nir_scalar s)
{
   return nir_def_as_intrinsic(s.def)->intrinsic;
}

static inline nir_scalar
nir_scalar_chase_alu_src(nir_scalar s, unsigned alu_src_idx)
{
   nir_scalar out = { NULL, 0 };

   nir_alu_instr *alu = nir_scalar_as_alu(s);
   assert(alu_src_idx < nir_op_infos[alu->op].num_inputs);

   /* Our component must be written */
   assert(s.comp < s.def->num_components);

   out.def = alu->src[alu_src_idx].src.ssa;

   if (nir_op_infos[alu->op].input_sizes[alu_src_idx] == 0) {
      /* The ALU src is unsized so the source component follows the
       * destination component.
       */
      out.comp = alu->src[alu_src_idx].swizzle[s.comp];
   } else {
      /* This is a sized source so all source components work together to
       * produce all the destination components.  Since we need to return a
       * scalar, this only works if the source is a scalar.
       */
      assert(nir_op_infos[alu->op].input_sizes[alu_src_idx] == 1);
      out.comp = alu->src[alu_src_idx].swizzle[0];
   }
   assert(out.comp < out.def->num_components);

   return out;
}

nir_scalar nir_scalar_chase_movs(nir_scalar s);

static inline nir_scalar
nir_get_scalar(nir_def *def, unsigned channel)
{
   nir_scalar s = { def, channel };
   return s;
}

/** Returns a nir_scalar where we've followed the bit-exact mov/vec use chain to the original definition */
static inline nir_scalar
nir_scalar_resolved(nir_def *def, unsigned channel)
{
   return nir_scalar_chase_movs(nir_get_scalar(def, channel));
}

static inline bool
nir_scalar_equal(nir_scalar s1, nir_scalar s2)
{
   return s1.def == s2.def && s1.comp == s2.comp;
}

static inline uint64_t
nir_alu_src_as_uint(nir_alu_src src)
{
   nir_scalar scalar = nir_get_scalar(src.src.ssa, src.swizzle[0]);
   return nir_scalar_as_uint(scalar);
}

static inline uint64_t
nir_alu_src_comp_as_uint(nir_alu_src src, unsigned comp)
{
   nir_scalar scalar = nir_scalar_resolved(src.src.ssa, src.swizzle[comp]);
   return nir_scalar_as_uint(scalar);
}

static inline bool
nir_alu_src_comp_get_uint(nir_alu_src src, unsigned comp, uint64_t *value)
{
   nir_scalar scalar = nir_scalar_resolved(src.src.ssa, src.swizzle[comp]);

   if (nir_scalar_is_const(scalar)) {
      *value = nir_scalar_as_uint(scalar);
      return true;
   }

   return false;
}

typedef struct nir_binding {
   bool success;

   nir_variable *var;
   unsigned desc_set;
   unsigned binding;
   unsigned resource_type;
   unsigned num_indices;
   nir_src indices[4];
   bool read_first_invocation;
} nir_binding;

nir_binding nir_chase_binding(nir_src rsrc);
nir_variable *nir_get_binding_variable(nir_shader *shader, nir_binding binding);

/*
 * Control flow
 *
 * Control flow consists of a tree of control flow nodes, which include
 * if-statements and loops. The leaves of the tree are basic blocks, lists of
 * instructions that always run start-to-finish. Each basic block also keeps
 * track of its successors (blocks which may run immediately after the current
 * block) and predecessors (blocks which could have run immediately before the
 * current block). Each function also has a start block and an end block which
 * all return statements point to (which is always empty). Together, all the
 * blocks with their predecessors and successors make up the control flow
 * graph (CFG) of the function. There are helpers that modify the tree of
 * control flow nodes while modifying the CFG appropriately; these should be
 * used instead of modifying the tree directly.
 */

typedef enum {
   nir_cf_node_block,
   nir_cf_node_if,
   nir_cf_node_loop,
   nir_cf_node_function
} nir_cf_node_type;

typedef struct nir_cf_node {
   struct exec_node node;
   nir_cf_node_type type;
   nir_cf_node *parent;
} nir_cf_node;

typedef struct nir_block {
   nir_cf_node cf_node;

   /** list of nir_instr */
   struct exec_list instr_list;

   /** pointer to the NIR function impl */
   nir_function_impl *impl;

   /** generic block index; generated by nir_index_blocks */
   unsigned index;

   /* This indicates whether the block or any parent block is executed
    * conditionally and whether the condition uses a divergent value.
    */
   bool divergent;

   /*
    * Each block can only have up to 2 successors, so we put them in a simple
    * array - no need for anything more complicated.
    */
   nir_block *successors[2];

   /* Set of nir_block predecessors in the CFG */
   struct util_dynarray predecessors;
   nir_block *_preds_storage[2];

   /*
    * this node's immediate dominator in the dominance tree - set to NULL for
    * the start block and any unreachable blocks.
    */
   nir_block *imm_dom;

   /* This node's children in the dominance tree */
   unsigned num_dom_children;
   nir_block **dom_children;

   /* This is the backing storage for "dom_children" if the array is small
    * enough to fit in it, so that we don't have to ralloc the array.
    *
    * 99.99996% of blocks in my GLSL shader-db have between 1 and 3 dom
    * children, the remaning 0.00004% have 4, and none have more than 4.
    * This reduces ralloc overhead for allocating "dom_children" to basically 0.
    */
   nir_block *_dom_children_storage[3];

   /* Set of nir_blocks on the dominance frontier of this block */
   struct set dom_frontier;

   /*
    * These two indices have the property that dom_{pre,post}_index for each
    * child of this block in the dominance tree will always be between
    * dom_pre_index and dom_post_index for this block, which makes testing if
    * a given block is dominated by another block an O(1) operation.
    */
   uint32_t dom_pre_index, dom_post_index;

   /**
    * Value just before the first nir_instr->index in the block, but after
    * end_ip that of any predecessor block.
    */
   uint32_t start_ip;
   /**
    * Value just after the last nir_instr->index in the block, but before the
    * start_ip of any successor block.
    */
   uint32_t end_ip;

   /* SSA def live in and out for this block; used for liveness analysis.
    * Indexed by ssa_def->index
    */
   struct u_sparse_bitset live_in;
   struct u_sparse_bitset live_out;
} nir_block;

static ALWAYS_INLINE nir_block **
_nir_pred_iter_begin(const nir_block *block)
{
   return (nir_block **)util_dynarray_begin(&block->predecessors);
}

static ALWAYS_INLINE bool
_nir_pred_iter_end(const nir_block *block, nir_block **iter, nir_block **pred)
{
   if (iter == (nir_block **)util_dynarray_end(&block->predecessors))
      return false;
   *pred = *iter;
   return true;
}

#define nir_foreach_pred(pred, block)                                    \
   for (nir_block * pred, **pred##_iter = _nir_pred_iter_begin((block)); \
        _nir_pred_iter_end((block), pred##_iter, &pred);                 \
        pred##_iter++)

static inline size_t
nir_block_num_preds(const nir_block *block)
{
   return util_dynarray_num_elements(&block->predecessors, nir_block *);
}

static inline bool
nir_block_has_pred(const nir_block *block, const nir_block *pred)
{
   return pred->successors[0] == block || pred->successors[1] == block;
}

static inline void
nir_block_add_pred(nir_block *block, nir_block *pred)
{
   util_dynarray_append(&block->predecessors, pred);
}

static inline void
nir_block_remove_pred(nir_block *block, nir_block *pred)
{
   util_dynarray_delete_unordered(&block->predecessors, nir_block *, pred);
}

static inline bool
nir_block_is_reachable(nir_block *b)
{
   /* See also nir_block_dominates */
   return b->dom_post_index != 0;
}

static inline nir_instr *
nir_block_first_instr(nir_block *block)
{
   struct exec_node *head = exec_list_get_head(&block->instr_list);
   return exec_node_data(nir_instr, head, node);
}

static inline nir_instr *
nir_block_last_instr(nir_block *block)
{
   struct exec_node *tail = exec_list_get_tail(&block->instr_list);
   return exec_node_data(nir_instr, tail, node);
}

static inline bool
nir_block_ends_in_jump(nir_block *block)
{
   return !exec_list_is_empty(&block->instr_list) &&
          nir_block_last_instr(block)->type == nir_instr_type_jump;
}

static inline bool
nir_block_ends_in_return_or_halt(nir_block *block)
{
   if (exec_list_is_empty(&block->instr_list))
      return false;

   nir_instr *instr = nir_block_last_instr(block);
   if (instr->type != nir_instr_type_jump)
      return false;

   nir_jump_instr *jump_instr = nir_instr_as_jump(instr);
   return jump_instr->type == nir_jump_return ||
          jump_instr->type == nir_jump_halt;
}

static inline bool
nir_block_ends_in_break(nir_block *block)
{
   if (exec_list_is_empty(&block->instr_list))
      return false;

   nir_instr *instr = nir_block_last_instr(block);
   return instr->type == nir_instr_type_jump &&
          nir_instr_as_jump(instr)->type == nir_jump_break;
}

bool nir_block_contains_work(nir_block *block);

#define nir_foreach_instr(instr, block) \
   foreach_list_typed(nir_instr, instr, node, &(block)->instr_list)
#define nir_foreach_instr_reverse(instr, block) \
   foreach_list_typed_reverse(nir_instr, instr, node, &(block)->instr_list)
#define nir_foreach_instr_safe(instr, block) \
   foreach_list_typed_safe(nir_instr, instr, node, &(block)->instr_list)
#define nir_foreach_instr_reverse_safe(instr, block) \
   foreach_list_typed_reverse_safe(nir_instr, instr, node, &(block)->instr_list)

/* Phis come first in the block */
static inline nir_phi_instr *
nir_first_phi_in_block(nir_block *block)
{
   nir_foreach_instr(instr, block) {
      if (instr->type == nir_instr_type_phi)
         return nir_instr_as_phi(instr);
      else
         return NULL;
   }

   return NULL;
}

static inline nir_phi_instr *
nir_next_phi(nir_phi_instr *phi)
{
   nir_instr *next = nir_instr_next(&phi->instr);

   if (next && next->type == nir_instr_type_phi)
      return nir_instr_as_phi(next);
   else
      return NULL;
}

#define nir_foreach_phi(instr, block)                                        \
   for (nir_phi_instr *instr = nir_first_phi_in_block(block); instr != NULL; \
        instr = nir_next_phi(instr))

#define nir_foreach_phi_safe(instr, block)                          \
   for (nir_phi_instr *instr = nir_first_phi_in_block(block),       \
                      *__next = instr ? nir_next_phi(instr) : NULL; \
        instr != NULL;                                              \
        instr = __next, __next = instr ? nir_next_phi(instr) : NULL)

static inline nir_phi_instr *
nir_block_last_phi_instr(nir_block *block)
{
   nir_phi_instr *last_phi = NULL;
   nir_foreach_phi(instr, block)
      last_phi = instr;

   return last_phi;
}

typedef enum {
   nir_selection_control_none = 0x0,

   /**
    * Defined by SPIR-V spec 3.22 "Selection Control".
    * The application prefers to remove control flow.
    */
   nir_selection_control_flatten = 0x1,

   /**
    * Defined by SPIR-V spec 3.22 "Selection Control".
    * The application prefers to keep control flow.
    */
   nir_selection_control_dont_flatten = 0x2,

   /**
    * May be applied by the compiler stack when it knows
    * that a branch is divergent, and:
    * - either both the if and else are always taken
    * - the if or else is empty and the other is always taken
    */
   nir_selection_control_divergent_always_taken = 0x3,
} nir_selection_control;

typedef struct nir_if {
   nir_cf_node cf_node;
   nir_src condition;
   nir_selection_control control;

   /** list of nir_cf_node */
   struct exec_list then_list;

   /** list of nir_cf_node */
   struct exec_list else_list;
} nir_if;

typedef struct nir_loop_terminator {
   nir_if *nif;

   /** Condition instruction that contains the induction variable */
   nir_instr *conditional_instr;

   /** Init source of the induction variable used in conditional_instr. */
   nir_src *init_src;

   /** Block within ::nif that has the break instruction. */
   nir_block *break_block;

   /** Last block for the then- or else-path that does not contain the break. */
   nir_block *continue_from_block;

   /** True when ::break_block is in the else-path of ::nif. */
   bool continue_from_then;
   bool induction_rhs;

   /* This is true if the terminators exact trip count is unknown. For
    * example:
    *
    *    for (int i = 0; i < imin(x, 4); i++)
    *       ...
    *
    * Here loop analysis would have set a max_trip_count of 4 however we dont
    * know for sure that this is the exact trip count.
    */
   bool exact_trip_count_unknown;

   struct list_head loop_terminator_link;
} nir_loop_terminator;

typedef struct nir_loop_induction_variable {
   /* SSA def of the phi-node associated with this induction variable. */
   nir_def *basis;

   /* SSA def of the increment of the induction variable. */
   nir_def *def;

   /* Init statement */
   nir_src *init_src;

   /* Update statement */
   nir_alu_src *update_src;
} nir_loop_induction_variable;

typedef struct nir_loop_info {
   /* Estimated cost (in number of instructions) of the loop */
   unsigned instr_cost;

   /* Contains fp64 ops that will be lowered */
   bool has_soft_fp64;

   /* Guessed trip count based on array indexing */
   unsigned guessed_trip_count;

   /* Maximum number of times the loop is run (if known) */
   unsigned max_trip_count;

   /* Do we know the exact number of times the loop will be run */
   bool exact_trip_count_known;

   /* Unroll the loop regardless of its size */
   bool force_unroll;

   /* Whether all control flow gets eliminated upon unrolling. */
   bool flattens_all_control_flow;

   /* Does the loop contain complex loop terminators, continues or other
    * complex behaviours? If this is true we can't rely on
    * loop_terminator_list to be complete or accurate.
    */
   bool complex_loop;

   nir_loop_terminator *limiting_terminator;

   /* A list of loop_terminators terminating this loop. */
   struct list_head loop_terminator_list;

   /* hash table of induction variables for this loop */
   struct hash_table *induction_vars;
} nir_loop_info;

typedef enum {
   nir_loop_control_none = 0x0,
   nir_loop_control_unroll = 0x1,
   nir_loop_control_dont_unroll = 0x2,
} nir_loop_control;

typedef struct nir_loop {
   nir_cf_node cf_node;

   /** list of nir_cf_node */
   struct exec_list body;

   /** (optional) list of nir_cf_node */
   struct exec_list continue_list;

   nir_loop_info *info;
   nir_loop_control control;
   bool partially_unrolled;
   bool do_while;

   /**
    * Whether some loop-active invocations might take a different control-flow path:
    * divergent_continue indicates that a continue statement might be taken by
    * only some of the loop-active invocations. A subsequent break is always
    * considered divergent.
    */
   bool divergent_continue;
   bool divergent_break;
} nir_loop;

static inline bool
nir_loop_is_divergent(nir_loop *loop)
{
   return loop->divergent_continue || loop->divergent_break;
}

/**
 * Various bits of metadata that can may be created or required by
 * optimization and analysis passes
 */
typedef enum {
   nir_metadata_none = 0x0,

   /** Indicates that nir_block::index values are valid.
    *
    * The start block has index 0 and they increase through a natural walk of
    * the CFG.  nir_function_impl::num_blocks is the number of blocks and
    * every block index is in the range [0, nir_function_impl::num_blocks].
    *
    * A pass can preserve this metadata type if it doesn't touch the CFG.
    */
   nir_metadata_block_index = 0x1,

   /** Indicates that block dominance information is valid
    *
    * This includes:
    *
    *   - nir_block::num_dom_children
    *   - nir_block::dom_children
    *   - nir_block::dom_frontier
    *   - nir_block::dom_pre_index
    *   - nir_block::dom_post_index
    *
    * A pass can preserve this metadata type if it doesn't touch the CFG.
    */
   nir_metadata_dominance = 0x2,

   /** Indicates that SSA def data-flow liveness information is valid
    *
    * This includes:
    *
    *   - nir_block::live_in
    *   - nir_block::live_out
    *
    * A pass can preserve this metadata type if it never adds or removes any
    * SSA defs or uses of SSA defs (most passes shouldn't preserve this
    * metadata type).
    */
   nir_metadata_live_defs = 0x4,

   /** A dummy metadata value to track when a pass forgot to preserve metadata.
    *
    * A pass should always clear this value even if it doesn't make any
    * progress to indicate that it thought about preserving metadata.
    */
   nir_metadata_not_properly_reset = 0x8,

   /** Indicates that loop analysis information is valid.
    *
    * This includes everything pointed to by nir_loop::info.
    *
    * A pass can preserve this metadata type if it is guaranteed to not affect
    * any loop metadata.  However, since loop metadata includes things like
    * loop counts which depend on arithmetic in the loop, this is very hard to
    * determine.  Most passes shouldn't preserve this metadata type.
    */
   nir_metadata_loop_analysis = 0x10,

   /** Indicates that nir_instr::index values are valid.
    *
    * The start instruction has index 0 and they increase through a natural
    * walk of instructions in blocks in the CFG.  The indices my have holes
    * after passes such as DCE.
    *
    * A pass can preserve this metadata type if it never adds or moves any
    * instructions (most passes shouldn't preserve this metadata type), but
    * can preserve it if it only removes instructions.
    */
   nir_metadata_instr_index = 0x20,

   /** Indicates that divergence analysis information is valid.
    *
    * This includes:
    *   - nir_def::divergent
    *   - nir_def::loop_invariant
    *   - nir_block::divergent
    *   - nir_loop::divergent_break
    *   - nir_loop::divergent_continue
    *
    * A pass can preserve this metadata type if it never adds any instructions or
    * moves them across loop breaks, as well as if it only removes instructions.
    * CF modifications usually invalidate this metadata.  Most passes
    * shouldn't preserve this metadata type.
    */
   nir_metadata_divergence = 0x40,

   /** Indicates that block dominance lca information is valid
    *
    * This includes:
    *
    *   - nir_function_impl::dom_lca_info
    *
    * A pass can preserve this metadata type if it preserves
    * nir_metadata_dominance.
    */
   nir_metadata_dominance_lca = 0x80,

   /** All control flow metadata
    *
    * This includes all metadata preserved by a pass that preserves control flow
    * but modifies instructions. For example, a pass using
    * nir_shader_instructions_pass will typically preserve this if it does not
    * insert control flow.
    *
    * This is the most common metadata set to preserve, so it has its own alias.
    */
   nir_metadata_control_flow = nir_metadata_block_index |
                               nir_metadata_dominance |
                               nir_metadata_dominance_lca,

   /** All metadata
    *
    * This includes all nir_metadata flags except not_properly_reset.  Passes
    * which do not change the shader in any way should use this.
    */
   nir_metadata_all = ~nir_metadata_not_properly_reset,
} nir_metadata;
MESA_DEFINE_CPP_ENUM_BITFIELD_OPERATORS(nir_metadata)

typedef struct nir_function_impl {
   nir_cf_node cf_node;

   /** pointer to the function of which this is an implementation */
   nir_function *function;

   /**
    * For entrypoints, a pointer to a nir_function_impl which runs before
    * it, once per draw or dispatch, communicating via store_preamble and
    * load_preamble intrinsics. If NULL then there is no preamble.
    */
   nir_function *preamble;

   /** list of nir_cf_node */
   struct exec_list body;

   nir_block *end_block;

   /** list for all local variables in the function */
   struct exec_list locals;

   /** next available SSA value index */
   unsigned ssa_alloc;

   /* total number of basic blocks, only valid when block_index_dirty = false */
   unsigned num_blocks;

   /** Information used for LCA queries */
   struct nir_dom_lca_info {
      struct range_minimum_query_table table;
      nir_block **block_from_idx;
   } dom_lca_info;

   /** True if this nir_function_impl uses structured control-flow
    *
    * Structured nir_function_impls have different validation rules.
    */
   bool structured;

   nir_metadata valid_metadata;
   nir_variable_mode loop_analysis_indirect_mask;
   bool loop_analysis_force_unroll_sampler_indirect;
} nir_function_impl;

#define nir_foreach_function_temp_variable(var, impl) \
   foreach_list_typed(nir_variable, var, node, &(impl)->locals)

#define nir_foreach_function_temp_variable_safe(var, impl) \
   foreach_list_typed_safe(nir_variable, var, node, &(impl)->locals)

ATTRIBUTE_RETURNS_NONNULL static inline nir_block *
nir_start_block(nir_function_impl *impl)
{
   return (nir_block *)impl->body.head_sentinel.next;
}

ATTRIBUTE_RETURNS_NONNULL static inline nir_block *
nir_impl_last_block(nir_function_impl *impl)
{
   return (nir_block *)impl->body.tail_sentinel.prev;
}

static inline nir_cf_node *
nir_cf_node_next(nir_cf_node *node)
{
   struct exec_node *next = exec_node_get_next(&node->node);
   if (exec_node_is_tail_sentinel(next))
      return NULL;
   else
      return exec_node_data(nir_cf_node, next, node);
}

static inline nir_cf_node *
nir_cf_node_prev(nir_cf_node *node)
{
   struct exec_node *prev = exec_node_get_prev(&node->node);
   if (exec_node_is_head_sentinel(prev))
      return NULL;
   else
      return exec_node_data(nir_cf_node, prev, node);
}

static inline bool
nir_cf_node_is_first(const nir_cf_node *node)
{
   return exec_node_is_head_sentinel(node->node.prev);
}

static inline bool
nir_cf_node_is_last(const nir_cf_node *node)
{
   return exec_node_is_tail_sentinel(node->node.next);
}

NIR_DEFINE_CAST(nir_cf_node_as_block, nir_cf_node, nir_block, cf_node,
                type, nir_cf_node_block)
NIR_DEFINE_CAST(nir_cf_node_as_if, nir_cf_node, nir_if, cf_node,
                type, nir_cf_node_if)
NIR_DEFINE_CAST(nir_cf_node_as_loop, nir_cf_node, nir_loop, cf_node,
                type, nir_cf_node_loop)
NIR_DEFINE_CAST(nir_cf_node_as_function, nir_cf_node,
                nir_function_impl, cf_node, type, nir_cf_node_function)

static inline nir_block *
nir_if_first_then_block(nir_if *if_stmt)
{
   struct exec_node *head = exec_list_get_head(&if_stmt->then_list);
   return nir_cf_node_as_block(exec_node_data(nir_cf_node, head, node));
}

static inline nir_block *
nir_if_last_then_block(nir_if *if_stmt)
{
   struct exec_node *tail = exec_list_get_tail(&if_stmt->then_list);
   return nir_cf_node_as_block(exec_node_data(nir_cf_node, tail, node));
}

static inline nir_block *
nir_if_first_else_block(nir_if *if_stmt)
{
   struct exec_node *head = exec_list_get_head(&if_stmt->else_list);
   return nir_cf_node_as_block(exec_node_data(nir_cf_node, head, node));
}

static inline nir_block *
nir_if_last_else_block(nir_if *if_stmt)
{
   struct exec_node *tail = exec_list_get_tail(&if_stmt->else_list);
   return nir_cf_node_as_block(exec_node_data(nir_cf_node, tail, node));
}

static inline nir_block *
nir_loop_first_block(nir_loop *loop)
{
   struct exec_node *head = exec_list_get_head(&loop->body);
   return nir_cf_node_as_block(exec_node_data(nir_cf_node, head, node));
}

static inline nir_block *
nir_loop_last_block(nir_loop *loop)
{
   struct exec_node *tail = exec_list_get_tail(&loop->body);
   return nir_cf_node_as_block(exec_node_data(nir_cf_node, tail, node));
}

static inline bool
nir_loop_has_continue_construct(const nir_loop *loop)
{
   return !exec_list_is_empty(&loop->continue_list);
}

static inline nir_block *
nir_loop_first_continue_block(nir_loop *loop)
{
   assert(nir_loop_has_continue_construct(loop));
   struct exec_node *head = exec_list_get_head(&loop->continue_list);
   return nir_cf_node_as_block(exec_node_data(nir_cf_node, head, node));
}

static inline nir_block *
nir_loop_last_continue_block(nir_loop *loop)
{
   assert(nir_loop_has_continue_construct(loop));
   struct exec_node *tail = exec_list_get_tail(&loop->continue_list);
   return nir_cf_node_as_block(exec_node_data(nir_cf_node, tail, node));
}

bool nir_loop_has_back_edge(nir_loop *loop);

/**
 * Return true if this list of cf_nodes contains a single empty block.
 */
static inline bool
nir_cf_list_is_empty_block(struct exec_list *cf_list)
{
   if (exec_list_is_singular(cf_list)) {
      struct exec_node *head = exec_list_get_head(cf_list);
      nir_block *block =
         nir_cf_node_as_block(exec_node_data(nir_cf_node, head, node));
      return exec_list_is_empty(&block->instr_list);
   }
   return false;
}

typedef struct nir_parameter {
   uint8_t num_components;
   uint8_t bit_size;

   /* True if this parameter is a deref used for returning values */
   bool is_return;

   bool implicit_conversion_prohibited;

   /* True if this parameter is not divergent. This is inverted to make
    * parameters divergent by default unless explicitly specified
    * otherwise.
    */
   bool is_uniform;

   nir_variable_mode mode;

   /* Drivers may optionally stash flags here describing the parameter.
    * For example, this might encode whether the driver expects the value
    * to be uniform or divergent, if the driver handles divergent parameters
    * differently from uniform ones.
    *
    * NIR will preserve this value but does not interpret it in any way.
    */
   uint32_t driver_attributes;

   /* The type of the function param */
   const struct glsl_type *type;

   /* Name if known, null if unknown */
   const char *name;
} nir_parameter;

typedef struct nir_function {
   struct exec_node node;

   const char *name;
   nir_shader *shader;

   unsigned num_params;
   nir_parameter *params;

   /** The implementation of this function.
    *
    * If the function is only declared and not implemented, this is NULL.
    *
    * Unless setting to NULL or NIR_SERIALIZE_FUNC_HAS_IMPL, set with
    * nir_function_set_impl to maintain IR invariants.
    */
   nir_function_impl *impl;

   /* Drivers may optionally stash flags here describing the function call.
    * For example, this might encode the ABI used for the call if a driver
    * supports multiple ABIs.
    *
    * NIR will preserve this value but does not interpret it in any way.
    */
   uint32_t driver_attributes;

   bool is_entrypoint;
   /* called by a cmat call */
   bool cmat_call;
   /* from SPIR-V linkage, only for libraries */
   bool is_exported;
   bool is_preamble;
   /* from SPIR-V function control */
   bool should_inline;
   bool dont_inline; /* from SPIR-V */

   /* Static workgroup size, if this is a kernel function in a library of OpenCL
    * kernels. Normally, the size in the shader info is used instead.
    */
   unsigned workgroup_size[3];

   /**
    * Is this function a subroutine type declaration
    * e.g. subroutine void type1(float arg1);
    */
   bool is_subroutine;

   /* Temporary function created to wrap global instructions before they can
    * be inlined into the main function.
    */
   bool is_tmp_globals_wrapper;

   /**
    * Is this function associated to a subroutine type
    * e.g. subroutine (type1, type2) function_name { function_body };
    * would have num_subroutine_types 2,
    * and pointers to the type1 and type2 types.
    */
   int num_subroutine_types;
   const struct glsl_type **subroutine_types;

   int subroutine_index;

   /* A temporary for passes to use for storing flags. */
   uint32_t pass_flags;
} nir_function;

/* Like nir_instr_filter_cb but specialized to intrinsics */
typedef bool (*nir_intrin_filter_cb)(const nir_intrinsic_instr *, const void *);

/** A vectorization width callback
 *
 * Returns the maximum vectorization width per instruction.
 * 0, if the instruction must not be modified.
 *
 * The vectorization width must be a power of 2.
 */
typedef uint8_t (*nir_vectorize_cb)(const nir_instr *, const void *);

typedef struct nir_shader {
   gc_ctx *gctx;

   /** list of uniforms (nir_variable) */
   struct exec_list variables;

   /** Set of driver-specific options for the shader.
    *
    * The memory for the options is expected to be kept in a single static
    * copy by the driver.
    */
   const nir_shader_compiler_options *options;

   /** Various bits of compile-time information about a given shader */
   struct shader_info info;

   /** list of nir_function */
   struct exec_list functions;

   /**
    * The size of the variable space for load_input_*, load_uniform_*, etc.
    * intrinsics.  This is in back-end specific units which is likely one of
    * bytes, dwords, or vec4s depending on context and back-end.
    */
   unsigned num_inputs, num_uniforms, num_outputs;

   /** Size in bytes of required implicitly bound global memory */
   unsigned global_mem_size;

   /** Size in bytes of required scratch space */
   unsigned scratch_size;

   /** Constant data associated with this shader.
    *
    * Constant data is loaded through load_constant intrinsics (as compared to
    * the NIR load_const instructions which have the constant value inlined
    * into them).  This is usually generated by nir_opt_large_constants (so
    * shaders don't have to load_const into a temporary array when they want
    * to indirect on a const array).
    */
   void *constant_data;
   /** Size of the constant data associated with the shader, in bytes */
   unsigned constant_data_size;

   nir_xfb_info *xfb_info;

   unsigned printf_info_count;
   u_printf_info *printf_info;

   bool has_debug_info;
   uint32_t nir_pass_depth;
   bool nir_pass_recursed;
} nir_shader;

#define nir_foreach_function(func, shader) \
   foreach_list_typed(nir_function, func, node, &(shader)->functions)

#define nir_foreach_function_safe(func, shader) \
   foreach_list_typed_safe(nir_function, func, node, &(shader)->functions)

#define nir_foreach_entrypoint(func, lib) \
   nir_foreach_function(func, lib)        \
      if (func->is_entrypoint)

#define nir_foreach_entrypoint_safe(func, lib) \
   nir_foreach_function_safe(func, lib)        \
      if (func->is_entrypoint)

static inline nir_function *
nir_foreach_function_with_impl_first(const nir_shader *shader)
{
   foreach_list_typed(nir_function, func, node, &shader->functions) {
      if (func->impl != NULL)
         return func;
   }

   return NULL;
}

static inline nir_function_impl *
nir_foreach_function_with_impl_next(nir_function **it)
{
   foreach_list_typed_from(nir_function, func, node, _, (*it)->node.next) {
      if (func->impl != NULL) {
         *it = func;
         return func->impl;
      }
   }

   return NULL;
}

#define nir_foreach_function_with_impl(it, impl_it, shader)              \
   for (nir_function *it = nir_foreach_function_with_impl_first(shader); \
        it != NULL;                                                      \
        it = NULL)                                                       \
                                                                         \
      for (nir_function_impl *impl_it = it->impl;                        \
           impl_it != NULL;                                              \
           impl_it = nir_foreach_function_with_impl_next(&it))

/* Equivalent to
 *
 *    nir_foreach_function(func, shader) {
 *       if (func->impl != NULL) {
 *             ...
 *       }
 *    }
 *
 * Carefully written to ensure break/continue work in the user code.
 */

#define nir_foreach_function_impl(it, shader) \
   nir_foreach_function_with_impl(_func_##it, it, shader)

static inline nir_function_impl *
nir_shader_get_entrypoint(const nir_shader *shader)
{
   nir_function *func = NULL;

   nir_foreach_function(function, shader) {
      assert(func == NULL);
      if (function->is_entrypoint) {
         func = function;
#ifndef NDEBUG
         break;
#endif
      }
   }

   if (!func)
      return NULL;

   assert(func->num_params == 0);
   assert(func->impl);
   return func->impl;
}

static inline nir_function *
nir_shader_get_function_for_name(const nir_shader *shader, const char *name)
{
   nir_foreach_function(func, shader) {
      if (func->name && strcmp(func->name, name) == 0)
         return func;
   }

   return NULL;
}

static inline nir_variable *
nir_deref_instr_get_variable(const nir_deref_instr *instr)
{
   while (instr->deref_type != nir_deref_type_var) {
      if (instr->deref_type == nir_deref_type_cast &&
          !nir_def_is_deref(instr->parent.ssa))
         return NULL;

      instr = nir_deref_instr_parent(instr);
   }

   return instr->var;
}

static inline nir_variable *
nir_intrinsic_get_var(const nir_intrinsic_instr *intrin, unsigned i)
{
   return nir_deref_instr_get_variable(nir_src_as_deref(intrin->src[i]));
}

/*
 * After all functions are forcibly inlined, these passes remove redundant
 * functions from a shader and library respectively.
 */
void nir_remove_non_entrypoints(nir_shader *shader);
void nir_remove_non_cmat_call_entrypoints(nir_shader *nir);
void nir_remove_non_exported(nir_shader *shader);
void nir_remove_entrypoints(nir_shader *shader);
void nir_fixup_is_exported(nir_shader *shader);

nir_shader *nir_shader_create(void *mem_ctx,
                              mesa_shader_stage stage,
                              const nir_shader_compiler_options *options);

bool nir_shader_bisect_select(nir_shader *s);

/** Adds a variable to the appropriate list in nir_shader */
void nir_shader_add_variable(nir_shader *shader, nir_variable *var);

static inline void
nir_function_impl_add_variable(nir_function_impl *impl, nir_variable *var)
{
   assert(var->data.mode == nir_var_function_temp);
   exec_list_push_tail(&impl->locals, &var->node);
}

nir_variable *nir_variable_create_zeroed(nir_shader *nir);
void nir_variable_set_name(nir_shader *nir, nir_variable *var,
                           const char *name);
void nir_variable_set_namef(nir_shader *nir, nir_variable *var,
                            const char *fmt, ...) PRINTFLIKE(3, 4);
void nir_variable_append_namef(nir_shader *nir, nir_variable *var,
                               const char *fmt, ...) PRINTFLIKE(3, 4);
void nir_variable_steal_name(nir_shader *nir, nir_variable *dst,
                             nir_variable *src);

/** creates a variable, sets a few defaults, and adds it to the list */
nir_variable *nir_variable_create(nir_shader *shader,
                                  nir_variable_mode mode,
                                  const struct glsl_type *type,
                                  const char *name);
/** creates a local variable and adds it to the list */
nir_variable *nir_local_variable_create(nir_function_impl *impl,
                                        const struct glsl_type *type,
                                        const char *name);

/** Creates a uniform builtin state variable. */
nir_variable *
nir_state_variable_create(nir_shader *shader,
                          const struct glsl_type *type,
                          const char *name,
                          const gl_state_index16 tokens[STATE_LENGTH]);

/* Gets the variable for the given mode and location, creating it (with the given
 * type) if necessary.
 */
nir_variable *
nir_get_variable_with_location(nir_shader *shader, nir_variable_mode mode, int location,
                               const struct glsl_type *type);

/* Creates a variable for the given mode and location.
 */
nir_variable *
nir_create_variable_with_location(nir_shader *shader, nir_variable_mode mode, int location,
                                  const struct glsl_type *type);

nir_variable *nir_find_variable_with_location(nir_shader *shader,
                                              nir_variable_mode mode,
                                              unsigned location);

nir_variable *nir_find_variable_with_driver_location(nir_shader *shader,
                                                     nir_variable_mode mode,
                                                     unsigned location);

nir_variable *nir_find_state_variable(nir_shader *s,
                                      gl_state_index16 tokens[STATE_LENGTH]);

nir_variable *nir_find_sampler_variable_with_tex_index(nir_shader *shader,
                                                       unsigned texture_index);

void nir_sort_variables_with_modes(nir_shader *shader,
                                   int (*compar)(const nir_variable *,
                                                 const nir_variable *),
                                   nir_variable_mode modes);

/** creates a function and adds it to the shader's list of functions */
nir_function *nir_function_create(nir_shader *shader, const char *name);

static inline void
nir_function_set_impl(nir_function *func, nir_function_impl *impl)
{
   func->impl = impl;
   impl->function = func;
}

nir_function_impl *nir_function_impl_create(nir_function *func);
/** creates a function_impl that isn't tied to any particular function */
nir_function_impl *nir_function_impl_create_bare(nir_shader *shader);

nir_block *nir_block_create(nir_function_impl *impl);
nir_if *nir_if_create(nir_function_impl *impl);
nir_loop *nir_loop_create(nir_function_impl *impl);

nir_function_impl *nir_cf_node_get_function(nir_cf_node *node);

/** requests that the given pieces of metadata be generated */
void nir_metadata_require(nir_function_impl *impl, nir_metadata required, ...);
/** Preserves all metadata for the given shader */
void nir_shader_preserve_all_metadata(nir_shader *shader);
/** dirties all metadata and fills it with obviously wrong information */
void nir_metadata_invalidate(nir_shader *shader);

/**
 * Indicate progress on an implementation, preserving only the specified
 * metadata. The supplied progress is returned to improve ergonomics.
 */
bool nir_progress(bool progress, nir_function_impl *impl, nir_metadata preserved);

/** Indicate that there is no progress. All metadata is preserved. */
static inline bool
nir_no_progress(nir_function_impl *impl)
{
   return nir_progress(false, impl, nir_metadata_none /* ignored */);
}

/** creates an instruction with default swizzle/writemask/etc. with NULL registers */
nir_alu_instr *nir_alu_instr_create(nir_shader *shader, nir_op op);

nir_deref_instr *nir_deref_instr_create(nir_shader *shader,
                                        nir_deref_type deref_type);

nir_jump_instr *nir_jump_instr_create(nir_shader *shader, nir_jump_type type);

nir_load_const_instr *nir_load_const_instr_create(nir_shader *shader,
                                                  unsigned num_components,
                                                  unsigned bit_size);

nir_intrinsic_instr *nir_intrinsic_instr_create(nir_shader *shader,
                                                nir_intrinsic_op op);

nir_call_instr *nir_call_instr_create(nir_shader *shader,
                                      nir_function *callee);

int nir_cmat_call_op_params(nir_cmat_call_op op, nir_function *callee);
nir_cmat_call_instr *nir_cmat_call_instr_create(nir_shader *shader,
                                                nir_cmat_call_op op,
                                                nir_function *callee);

/** Creates a NIR texture instruction */
nir_tex_instr *nir_tex_instr_create(nir_shader *shader, unsigned num_srcs);

nir_phi_instr *nir_phi_instr_create(nir_shader *shader);
nir_phi_src *nir_phi_instr_add_src(nir_phi_instr *instr,
                                   nir_block *pred, nir_def *src);

nir_undef_instr *nir_undef_instr_create(nir_shader *shader,
                                        unsigned num_components,
                                        unsigned bit_size);

nir_const_value nir_alu_binop_identity(nir_op binop, unsigned bit_size);

/**
 * NIR Cursors and Instruction Insertion API
 * @{
 *
 * A tiny struct representing a point to insert/extract instructions or
 * control flow nodes.  Helps reduce the combinatorial explosion of possible
 * points to insert/extract.
 *
 * \sa nir_control_flow.h
 */
typedef enum {
   nir_cursor_before_block,
   nir_cursor_after_block,
   nir_cursor_before_instr,
   nir_cursor_after_instr,
} nir_cursor_option;

typedef struct nir_cursor {
   nir_cursor_option option;
   union {
      nir_block *block;
      nir_instr *instr;
   };
} nir_cursor;

static inline nir_block *
nir_cursor_current_block(nir_cursor cursor)
{
   if (cursor.option == nir_cursor_before_instr ||
       cursor.option == nir_cursor_after_instr) {
      return cursor.instr->block;
   } else {
      return cursor.block;
   }
}

bool nir_cursors_equal(nir_cursor a, nir_cursor b);

static inline nir_cursor
nir_before_block(nir_block *block)
{
   nir_cursor cursor;
   cursor.option = nir_cursor_before_block;
   cursor.block = block;
   return cursor;
}

static inline nir_cursor
nir_after_block(nir_block *block)
{
   nir_cursor cursor;
   cursor.option = nir_cursor_after_block;
   cursor.block = block;
   return cursor;
}

static inline nir_cursor
nir_before_instr(nir_instr *instr)
{
   nir_cursor cursor;
   cursor.option = nir_cursor_before_instr;
   cursor.instr = instr;
   return cursor;
}

static inline nir_cursor
nir_after_instr(nir_instr *instr)
{
   nir_cursor cursor;
   cursor.option = nir_cursor_after_instr;
   cursor.instr = instr;
   return cursor;
}

static inline nir_cursor
nir_before_block_after_phis(nir_block *block)
{
   nir_phi_instr *last_phi = nir_block_last_phi_instr(block);
   if (last_phi)
      return nir_after_instr(&last_phi->instr);
   else
      return nir_before_block(block);
}

static inline nir_cursor
nir_after_block_before_jump(nir_block *block)
{
   nir_instr *last_instr = nir_block_last_instr(block);
   if (last_instr && last_instr->type == nir_instr_type_jump) {
      return nir_before_instr(last_instr);
   } else {
      return nir_after_block(block);
   }
}

static inline nir_cursor
nir_before_src(nir_src *src)
{
   if (nir_src_is_if(src)) {
      nir_block *prev_block =
         nir_cf_node_as_block(nir_cf_node_prev(&nir_src_use_if(src)->cf_node));
      return nir_after_block(prev_block);
   } else if (nir_src_use_instr(src)->type == nir_instr_type_phi) {
#ifndef NDEBUG
      nir_phi_instr *cond_phi = nir_instr_as_phi(nir_src_use_instr(src));
      bool found = false;
      nir_foreach_phi_src(phi_src, cond_phi) {
         if (phi_src->src.ssa == src->ssa) {
            found = true;
            break;
         }
      }
      assert(found);
#endif
      /* The list_entry() macro is a generic container-of macro, it just happens
       * to have a more specific name.
       */
      nir_phi_src *phi_src = list_entry(src, nir_phi_src, src);
      return nir_after_block_before_jump(phi_src->pred);
   } else {
      return nir_before_instr(nir_src_use_instr(src));
   }
}

static inline nir_cursor
nir_before_cf_node(nir_cf_node *node)
{
   if (node->type == nir_cf_node_block)
      return nir_before_block(nir_cf_node_as_block(node));

   return nir_after_block(nir_cf_node_as_block(nir_cf_node_prev(node)));
}

static inline nir_cursor
nir_after_cf_node(nir_cf_node *node)
{
   if (node->type == nir_cf_node_block)
      return nir_after_block(nir_cf_node_as_block(node));

   return nir_before_block(nir_cf_node_as_block(nir_cf_node_next(node)));
}

static inline nir_cursor
nir_after_phis(nir_block *block)
{
   nir_foreach_instr(instr, block) {
      if (instr->type != nir_instr_type_phi)
         return nir_before_instr(instr);
   }
   return nir_after_block(block);
}

static inline nir_cursor
nir_after_instr_and_phis(nir_instr *instr)
{
   if (instr->type == nir_instr_type_phi)
      return nir_after_phis(instr->block);
   else
      return nir_after_instr(instr);
}

static inline nir_cursor
nir_after_cf_node_and_phis(nir_cf_node *node)
{
   if (node->type == nir_cf_node_block)
      return nir_after_block(nir_cf_node_as_block(node));

   nir_block *block = nir_cf_node_as_block(nir_cf_node_next(node));

   return nir_after_phis(block);
}

static inline nir_cursor
nir_before_cf_list(struct exec_list *cf_list)
{
   nir_cf_node *first_node = exec_node_data(nir_cf_node,
                                            exec_list_get_head(cf_list), node);
   return nir_before_cf_node(first_node);
}

static inline nir_cursor
nir_after_cf_list(struct exec_list *cf_list)
{
   nir_cf_node *last_node = exec_node_data(nir_cf_node,
                                           exec_list_get_tail(cf_list), node);
   return nir_after_cf_node(last_node);
}

static inline nir_cursor
nir_before_impl(nir_function_impl *impl)
{
   return nir_before_cf_list(&impl->body);
}

static inline nir_cursor
nir_after_impl(nir_function_impl *impl)
{
   return nir_after_cf_list(&impl->body);
}

static inline nir_cursor
nir_before_def(nir_def *def)
{
   return nir_before_instr(nir_def_instr(def));
}

static inline nir_cursor
nir_after_def(nir_def *def)
{
   return nir_after_instr(nir_def_instr(def));
}

/**
 * Insert a NIR instruction at the given cursor.
 *
 * Note: This does not update the cursor.
 */
void nir_instr_insert(nir_cursor cursor, nir_instr *instr);

bool nir_instr_move(nir_cursor cursor, nir_instr *instr);

static inline void
nir_instr_insert_before(nir_instr *instr, nir_instr *before)
{
   nir_instr_insert(nir_before_instr(instr), before);
}

static inline void
nir_instr_insert_after(nir_instr *instr, nir_instr *after)
{
   nir_instr_insert(nir_after_instr(instr), after);
}

static inline void
nir_instr_insert_before_block(nir_block *block, nir_instr *before)
{
   nir_instr_insert(nir_before_block(block), before);
}

static inline void
nir_instr_insert_after_block(nir_block *block, nir_instr *after)
{
   nir_instr_insert(nir_after_block(block), after);
}

static inline void
nir_instr_insert_before_cf(nir_cf_node *node, nir_instr *before)
{
   nir_instr_insert(nir_before_cf_node(node), before);
}

static inline void
nir_instr_insert_after_cf(nir_cf_node *node, nir_instr *after)
{
   nir_instr_insert(nir_after_cf_node(node), after);
}

static inline void
nir_instr_insert_before_cf_list(struct exec_list *list, nir_instr *before)
{
   nir_instr_insert(nir_before_cf_list(list), before);
}

static inline void
nir_instr_insert_after_cf_list(struct exec_list *list, nir_instr *after)
{
   nir_instr_insert(nir_after_cf_list(list), after);
}

void nir_instr_remove_v(nir_instr *instr);
void nir_instr_free(nir_instr *instr);
void nir_instr_free_list(struct exec_list *list);

static inline nir_cursor
nir_instr_remove(nir_instr *instr)
{
   nir_cursor cursor;
   nir_instr *prev = nir_instr_prev(instr);
   if (prev) {
      cursor = nir_after_instr(prev);
   } else {
      cursor = nir_before_block(instr->block);
   }
   nir_instr_remove_v(instr);
   return cursor;
}

nir_cursor nir_instr_free_and_dce(nir_instr *instr);

/** @} */

nir_def *nir_instr_def(nir_instr *instr);

/* Return the debug information associated with this instruction,
 * assuming the parent shader has debug info.
 */
static ALWAYS_INLINE nir_instr_debug_info *
nir_instr_get_debug_info(nir_instr *instr)
{
   assert(instr->has_debug_info);
   return container_of(instr, nir_instr_debug_info, instr);
}

static inline void *
nir_instr_get_gc_pointer(nir_instr *instr)
{
   if (unlikely(instr->has_debug_info))
      return nir_instr_get_debug_info(instr);

   return instr;
}

typedef bool (*nir_foreach_def_cb)(nir_def *def, void *state);
typedef bool (*nir_foreach_src_cb)(nir_src *src, void *state);
static inline bool nir_foreach_src(nir_instr *instr, nir_foreach_src_cb cb, void *state);
bool nir_foreach_phi_src_leaving_block(nir_block *instr,
                                       nir_foreach_src_cb cb,
                                       void *state);

nir_const_value *nir_src_as_const_value(nir_src src);

const char *nir_src_as_string(nir_src src);

#define NIR_SRC_AS_SRC_(name)                                                  \
   static inline nir_##name##_src *                                            \
   nir_src_as_##name##_src(nir_src *src)                                       \
   {                                                                           \
      assert(src && nir_src_use_instr(src)->type == nir_instr_type_##name);    \
      return container_of(src, nir_##name##_src, src);                         \
   }

NIR_SRC_AS_SRC_(alu);
NIR_SRC_AS_SRC_(phi);
NIR_SRC_AS_SRC_(tex);

bool nir_src_is_always_uniform(nir_src src);
bool nir_srcs_equal(nir_src src1, nir_src src2);
bool nir_instrs_equal(const nir_instr *instr1, const nir_instr *instr2);

static inline void
nir_src_rewrite(nir_src *src, nir_def *new_ssa)
{
   assert(src->ssa);
   assert(nir_src_is_if(src) ? (nir_src_use_if(src) != NULL) : (nir_src_use_instr(src) != NULL));
   list_del(&src->use_link);
   src->ssa = new_ssa;
   list_addtail(&src->use_link, &new_ssa->uses);
}

static inline void
nir_alu_src_rewrite_scalar(nir_alu_src *alu, nir_scalar s)
{
   nir_src_rewrite(&alu->src, s.def);
   alu->swizzle[0] = (uint8_t)s.comp;
}

/** Initialize a nir_src
 *
 * This is almost never the helper you want to use.  This helper assumes that
 * the source is uninitialized garbage and blasts over it without doing any
 * tear-down the existing source, including removing it from uses lists.
 * Using this helper on a source that currently exists in any uses list will
 * result in linked list corruption.  It also assumes that the instruction is
 * currently live in the IR and adds the source to the uses list for the given
 * nir_def as part of setup.
 *
 * This is pretty much only useful for adding sources to extant instructions
 * or manipulating parallel copy instructions as part of out-of-SSA.
 *
 * When in doubt, use nir_src_rewrite() instead.
 */
void nir_instr_init_src(nir_instr *instr, nir_src *src, nir_def *def);

/** Clear a nir_src
 *
 * This helper clears a nir_src by removing it from any uses lists and
 * resetting its contents to NIR_SRC_INIT.  This is typically used as a
 * precursor to removing the source from the instruction by adjusting a
 * num_srcs parameter somewhere or overwriting it with nir_instr_move_src().
 */
void nir_instr_clear_src(nir_instr *instr, nir_src *src);

void nir_instr_move_src(nir_instr *dest_instr, nir_src *dest, nir_src *src);

/** Returns true if first comes before second in a block. */
bool nir_instr_is_before(nir_instr *first, nir_instr *second);

void nir_def_init(nir_instr *instr, nir_def *def,
                  unsigned num_components, unsigned bit_size);
static inline void
nir_def_init_for_type(nir_instr *instr, nir_def *def,
                      const struct glsl_type *type)
{
   assert(glsl_type_is_vector_or_scalar(type));
   nir_def_init(instr, def, glsl_get_components(type),
                glsl_get_bit_size(type));
}
void nir_def_rewrite_uses(nir_def *def, nir_def *new_ssa);
void nir_def_rewrite_uses_src(nir_def *def, nir_src new_src);
void nir_def_rewrite_uses_after_instr(nir_def *def, nir_def *new_ssa,
                                      nir_instr *after_me);

static inline void
nir_def_rewrite_uses_after(nir_def *def, nir_def *new_ssa)
{
   nir_def_rewrite_uses_after_instr(def, new_ssa, nir_def_instr(new_ssa));
}

static inline void
nir_def_replace(nir_def *def, nir_def *new_ssa)
{
   nir_def_rewrite_uses(def, new_ssa);
   nir_instr_remove_v(nir_def_instr(def));
}

nir_component_mask_t nir_src_components_read(const nir_src *src);
nir_component_mask_t nir_def_components_read(const nir_def *def);
bool nir_def_all_uses_are_fsat(const nir_def *def);
bool nir_def_all_uses_ignore_sign_bit(const nir_def *def);

static inline int
nir_def_first_component_read(nir_def *def)
{
    return (int)ffs(nir_def_components_read(def)) - 1;
}

static inline int
nir_def_last_component_read(nir_def *def)
{
    return (int)util_last_bit(nir_def_components_read(def)) - 1;
}

static inline bool
nir_def_is_unused(nir_def *ssa)
{
   return list_is_empty(&ssa->uses);
}

/** Sorts unstructured blocks
 *
 * NIR requires that unstructured blocks be sorted in reverse post
 * depth-first-search order.  This is the standard ordering used in the
 * compiler literature which guarantees dominance.  In particular, reverse
 * post-DFS order guarantees that dominators occur in the list before the
 * blocks they dominate.
 *
 * NOTE: This function also implicitly deletes any unreachable blocks.
 */
void nir_sort_unstructured_blocks(nir_function_impl *impl);

/** Returns the next block
 *
 * For structured control-flow, this follows the same order as
 * nir_block_cf_tree_next().  For unstructured control-flow the blocks are in
 * reverse post-DFS order.  (See nir_sort_unstructured_blocks() above.)
 */
nir_block *nir_block_unstructured_next(nir_block *block);
nir_block *nir_unstructured_start_block(nir_function_impl *impl);

#define nir_foreach_block_unstructured(block, impl)                           \
   for (nir_block *block = nir_unstructured_start_block(impl); block != NULL; \
        block = nir_block_unstructured_next(block))

#define nir_foreach_block_unstructured_safe(block, impl)       \
   for (nir_block *block = nir_unstructured_start_block(impl), \
                  *next = nir_block_unstructured_next(block);  \
        block != NULL;                                         \
        block = next, next = nir_block_unstructured_next(block))

/*
 * finds the next basic block in source-code order, returns NULL if there is
 * none
 */

nir_block *nir_block_cf_tree_next(nir_block *block);

/* Performs the opposite of nir_block_cf_tree_next() */

nir_block *nir_block_cf_tree_prev(nir_block *block);

/* Gets the first block in a CF node in source-code order */

nir_block *nir_cf_node_cf_tree_first(nir_cf_node *node);

/* Gets the last block in a CF node in source-code order */

nir_block *nir_cf_node_cf_tree_last(nir_cf_node *node);

/* Gets the next block after a CF node in source-code order */

nir_block *nir_cf_node_cf_tree_next(nir_cf_node *node);

/* Gets the block before a CF node in source-code order */

nir_block *nir_cf_node_cf_tree_prev(nir_cf_node *node);

/* Macros for loops that visit blocks in source-code order */

#define nir_foreach_block(block, impl)                           \
   for (nir_block *block = nir_start_block(impl); block != NULL; \
        block = nir_block_cf_tree_next(block))

#define nir_foreach_block_safe(block, impl)              \
   for (nir_block *block = nir_start_block(impl),        \
                  *next = nir_block_cf_tree_next(block); \
        block != NULL;                                   \
        block = next, next = nir_block_cf_tree_next(block))

#define nir_foreach_block_reverse(block, impl)                       \
   for (nir_block *block = nir_impl_last_block(impl); block != NULL; \
        block = nir_block_cf_tree_prev(block))

#define nir_foreach_block_reverse_safe(block, impl)      \
   for (nir_block *block = nir_impl_last_block(impl),    \
                  *prev = nir_block_cf_tree_prev(block); \
        block != NULL;                                   \
        block = prev, prev = nir_block_cf_tree_prev(block))

#define nir_foreach_block_in_cf_node(block, node)           \
   for (nir_block *block = nir_cf_node_cf_tree_first(node); \
        block != nir_cf_node_cf_tree_next(node);            \
        block = nir_block_cf_tree_next(block))

#define nir_foreach_block_in_cf_node_safe(block, node)      \
   for (nir_block *block = nir_cf_node_cf_tree_first(node), \
                  *next = nir_block_cf_tree_next(block);    \
        block != nir_cf_node_cf_tree_next(node);            \
        block = next, next = nir_block_cf_tree_next(block))

#define nir_foreach_block_in_cf_node_reverse(block, node)  \
   for (nir_block *block = nir_cf_node_cf_tree_last(node); \
        block != nir_cf_node_cf_tree_prev(node);           \
        block = nir_block_cf_tree_prev(block))

#define nir_foreach_block_in_cf_node_reverse_safe(block, node) \
   for (nir_block *block = nir_cf_node_cf_tree_last(node),     \
                  *prev = nir_block_cf_tree_prev(block);       \
        block != nir_cf_node_cf_tree_prev(node);               \
        block = prev, prev = nir_block_cf_tree_prev(block))

static inline nir_block *
nir_loop_predecessor_block(nir_loop *loop)
{
   return nir_cf_node_cf_tree_prev(&loop->cf_node);
}

static inline nir_block *
nir_loop_successor_block(nir_loop *loop)
{
   return nir_cf_node_cf_tree_next(&loop->cf_node);
}

/* If the following CF node is an if, this function returns that if.
 * Otherwise, it returns NULL.
 */
nir_if *nir_block_get_following_if(nir_block *block);

nir_loop *nir_block_get_following_loop(nir_block *block);

nir_block **nir_block_get_predecessors_sorted(const nir_block *block, void *mem_ctx);

void nir_index_ssa_defs(nir_function_impl *impl);
unsigned nir_index_instrs(nir_function_impl *impl);

void nir_index_blocks(nir_function_impl *impl);

void nir_shader_clear_pass_flags(nir_shader *shader);

unsigned nir_shader_index_vars(nir_shader *shader, nir_variable_mode modes);
unsigned nir_function_impl_index_vars(nir_function_impl *impl);

void nir_print_shader(nir_shader *shader, FILE *fp);
void nir_print_function_body(nir_function_impl *impl, FILE *fp);
void nir_print_shader_annotated(nir_shader *shader, FILE *fp, struct hash_table *errors);
void nir_print_instr(const nir_instr *instr, FILE *fp);
void nir_print_deref(const nir_deref_instr *deref, FILE *fp);
void nir_log_shader_annotated_tagged(enum mesa_log_level level, const char *tag, nir_shader *shader, struct hash_table *annotations);
#define nir_log_shadere(s)                       nir_log_shader_annotated_tagged(MESA_LOG_ERROR, (MESA_LOG_TAG), (s), NULL)
#define nir_log_shaderw(s)                       nir_log_shader_annotated_tagged(MESA_LOG_WARN, (MESA_LOG_TAG), (s), NULL)
#define nir_log_shaderi(s)                       nir_log_shader_annotated_tagged(MESA_LOG_INFO, (MESA_LOG_TAG), (s), NULL)
#define nir_log_shader_annotated(s, annotations) nir_log_shader_annotated_tagged(MESA_LOG_ERROR, (MESA_LOG_TAG), (s), annotations)

char *nir_shader_as_str(nir_shader *nir, void *mem_ctx);
char *nir_shader_as_str_annotated(nir_shader *nir, struct hash_table *annotations, void *mem_ctx);
char *nir_instr_as_str(const nir_instr *instr, void *mem_ctx);

/** Adds debug information to the shader. The line numbers point to
 * the corresponding lines in the printed NIR, starting first_line;
 */
char *nir_shader_gather_debug_info(nir_shader *shader, const char *filename, uint32_t first_line);

/** Shallow clone of a single instruction. */
nir_instr *nir_instr_clone(nir_shader *s, const nir_instr *orig);

/** Clone a single instruction, including a remap table to rewrite sources. */
nir_instr *nir_instr_clone_deep(nir_shader *s, const nir_instr *orig,
                                struct hash_table *remap_table);

/** Shallow clone of a single ALU instruction. */
nir_alu_instr *nir_alu_instr_clone(nir_shader *s, const nir_alu_instr *orig);

nir_shader *nir_shader_clone(void *mem_ctx, const nir_shader *s);
nir_function *nir_function_clone(nir_shader *ns, const nir_function *fxn);
nir_function_impl *nir_function_impl_clone(nir_shader *shader,
                                           const nir_function_impl *fi);
nir_function_impl *
nir_function_impl_clone_remap_globals(nir_shader *shader,
                                      const nir_function_impl *fi,
                                      struct hash_table *remap_table);
nir_constant *nir_constant_clone(const nir_constant *c, void *mem_ctx);
nir_variable *nir_variable_clone(const nir_variable *c, nir_shader *shader);

void nir_shader_replace(nir_shader *dest, nir_shader *src);

void nir_shader_serialize_deserialize(nir_shader *s);

#ifndef NDEBUG
void nir_validate_shader(nir_shader *shader, const char *when);
void nir_validate_ssa_dominance(nir_shader *shader, const char *when);
void nir_metadata_set_validation_flag(nir_shader *shader);
void nir_metadata_check_validation_flag(nir_shader *shader);
void nir_metadata_require_most(nir_shader *shader);
/* glsl-optimizer change: no NIR serialization, so no progress validation
 * (it compares serialized shaders before/after each pass). */
static inline struct blob
nir_validate_progress_setup(nir_shader *shader)
{
   struct blob b = {0};
   (void)shader;
   return b;
}
static inline void
nir_validate_progress_finish(nir_shader *shader, struct blob *setup_blob, bool progress, const char *when)
{
   (void)shader;
   (void)setup_blob;
   (void)progress;
   (void)when;
}

static inline bool
should_skip_nir(const char *name)
{
   static const char *list = NULL;
   if (!list) {
      /* Comma separated list of names to skip. */
      list = os_get_option("NIR_SKIP");
      if (!list)
         list = "";
   }

   if (!list[0])
      return false;

   return comma_separated_list_contains(list, name);
}

static inline bool
should_print_nir(nir_shader *shader)
{
   if ((shader->info.internal && !NIR_DEBUG(PRINT_INTERNAL)) ||
       shader->info.stage < 0 ||
       shader->info.stage > MESA_SHADER_KERNEL)
      return false;

   return unlikely(nir_debug_print_shader[shader->info.stage]);
}
#else
static inline void
nir_validate_shader(nir_shader *shader, const char *when)
{
   (void)shader;
   (void)when;
}
static inline void
nir_validate_ssa_dominance(nir_shader *shader, const char *when)
{
   (void)shader;
   (void)when;
}
static inline void
nir_metadata_set_validation_flag(nir_shader *shader)
{
   (void)shader;
}
static inline void
nir_metadata_check_validation_flag(nir_shader *shader)
{
   (void)shader;
}
static inline void
nir_metadata_require_most(nir_shader *shader)
{
   (void)shader;
}
static inline struct blob
nir_validate_progress_setup(nir_shader *shader)
{
   struct blob b = {0};
   return b;
}
static inline void
nir_validate_progress_finish(nir_shader *shader, struct blob *setup_blob, bool progress, const char *when)
{
   (void)shader;
   (void)setup_blob;
   (void)progress;
   (void)when;
}
static inline bool
should_skip_nir(UNUSED const char *pass_name)
{
   return false;
}
static inline bool
should_print_nir(UNUSED nir_shader *shader)
{
   return false;
}
#endif /* NDEBUG */

#define _PASS(pass, nir, do_pass)                                       \
   do {                                                                 \
      if (should_skip_nir(#pass)) {                                     \
         printf("skipping %s\n", #pass);                                \
         break;                                                         \
      }                                                                 \
      if (NIR_DEBUG(INVALIDATE_METADATA))                               \
         nir_metadata_invalidate(nir);                                  \
      else if (NIR_DEBUG(EXTENDED_VALIDATION))                          \
         nir_metadata_require_most(nir);                                \
      do_pass if (NIR_DEBUG(CLONE))                                     \
      {                                                                 \
         nir_shader *_clone = nir_shader_clone(ralloc_parent(nir), nir);\
         nir_shader_replace(nir, _clone);                               \
      }                                                                 \
      /* glsl-optimizer change: no NIR_DEBUG(SERIALIZE) */             \
   } while (0)

#define NIR_STRINGIZE_INNER(x) #x
#define NIR_STRINGIZE(x)       NIR_STRINGIZE_INNER(x)

extern simple_mtx_t nir_print_lock;

#define NIR_PASS(progress, nir, pass, ...) _PASS(pass, nir, {                            \
   nir_metadata_set_validation_flag(nir);                                                \
   if (should_print_nir(nir)) {                                                          \
      if ((nir)->nir_pass_depth++ == 0) {                                                \
         simple_mtx_lock(&nir_print_lock);                                               \
      } else {                                                                           \
         (nir)->nir_pass_recursed = true;                                                \
      }                                                                                  \
      printf("%s\n", #pass);                                                             \
   }                                                                                     \
   static const char *when = "after " #pass " in " __FILE__ ":" NIR_STRINGIZE(__LINE__); \
   struct blob blob_before = nir_validate_progress_setup(nir);                           \
   if (pass(nir, ##__VA_ARGS__)) {                                                       \
      nir_validate_shader(nir, when);                                                    \
      UNUSED bool _;                                                                     \
      progress = true;                                                                   \
      if (should_print_nir(nir)) {                                                       \
         if ((nir)->nir_pass_recursed)                                                   \
            printf("%s (finished)\n", #pass);                                            \
         nir_print_shader(nir, stdout);                                                  \
      }                                                                                  \
      nir_metadata_check_validation_flag(nir);                                           \
      nir_validate_progress_finish(nir, &blob_before, true, when);                       \
   } else {                                                                              \
      if (NIR_DEBUG(EXTENDED_VALIDATION))                                                \
         nir_validate_shader(nir, when);                                                 \
      nir_validate_progress_finish(nir, &blob_before, false, when);                      \
   }                                                                                     \
   if (should_print_nir(nir) && --(nir)->nir_pass_depth == 0) {                          \
      simple_mtx_unlock(&nir_print_lock);                                                \
      (nir)->nir_pass_recursed = false;                                                  \
   }                                                                                     \
})

#define _NIR_LOOP_PASS(progress, idempotent, skip, nir, pass, ...)   \
do {                                                                 \
   bool nir_loop_pass_progress = false;                              \
   if (!_mesa_set_search(skip, (void (*)())&pass))                   \
      NIR_PASS(nir_loop_pass_progress, nir, pass, ##__VA_ARGS__);    \
   if (nir_loop_pass_progress)                                       \
      _mesa_set_clear(skip, NULL);                                   \
   if (idempotent || !nir_loop_pass_progress)                        \
      _mesa_set_add(skip, (void (*)())&pass);                        \
   UNUSED bool _ = false;                                            \
   progress |= nir_loop_pass_progress;                               \
} while (0)

/* Helper to skip a pass if no different passes have made progress since it was
 * previously run. Note that two passes are considered the same if they have
 * the same function pointer, even if they used different options.
 *
 * The usage of this is mostly identical to NIR_PASS. "skip" is a "struct set *"
 * (created by _mesa_pointer_set_create) which the macro uses to keep track of
 * already run passes.
 *
 * Example:
 * bool progress = true;
 * struct set *skip = _mesa_pointer_set_create(NULL);
 * while (progress) {
 *    progress = false;
 *    NIR_LOOP_PASS(progress, skip, nir, pass1);
 *    NIR_LOOP_PASS_NOT_IDEMPOTENT(progress, skip, nir, nir_opt_algebraic);
 *    NIR_LOOP_PASS(progress, skip, nir, pass2);
 *    ...
 * }
 * _mesa_set_destroy(skip, NULL);
 *
 * You shouldn't mix usage of this with the NIR_PASS set of helpers, without
 * using a new "skip" in-between.
 */
#define NIR_LOOP_PASS(progress, skip, nir, pass, ...) \
   _NIR_LOOP_PASS(progress, true, skip, nir, pass, ##__VA_ARGS__)

/* Like NIR_LOOP_PASS, but use this for passes which may make further progress
 * when repeated.
 */
#define NIR_LOOP_PASS_NOT_IDEMPOTENT(progress, skip, nir, pass, ...) \
   _NIR_LOOP_PASS(progress, false, skip, nir, pass, ##__VA_ARGS__)

#define NIR_SKIP(name) should_skip_nir(#name)

#ifndef NDEBUG

void _nir_assert_no_progress(bool progress, const char *when);

/* Abort if the pass has made any progress. This can be used to assert that:
 * - our minimalist pass invocation loops in drivers are sufficient and don't
 *   leave any unoptimized code in the shader
 * - our lowering is sufficient and other passes don't add instructions that
 *   would cause lowering having to be repeated
 */
#define NIR_PASS_ASSERT_NO_PROGRESS(nir, pass, ...) \
   do { \
      if (!NIR_DEBUG(NOVALIDATE)) { \
         static const char *when = #pass " in " __FILE__ ":" NIR_STRINGIZE(__LINE__); \
         bool _progress = false; \
         NIR_PASS(_progress, nir, pass, ##__VA_ARGS__); \
         _nir_assert_no_progress(_progress, when); \
      } \
   } while (0)

#else
#define NIR_PASS_ASSERT_NO_PROGRESS(nir, pass, ...)
#endif

/** An instruction filtering callback with writemask
 *
 * Returns true if the instruction should be processed with the associated
 * writemask and false otherwise.
 */
typedef bool (*nir_instr_writemask_filter_cb)(const nir_instr *,
                                              unsigned writemask, const void *);

/** A simple instruction lowering callback
 *
 * Many instruction lowering passes can be written as a simple function which
 * takes an instruction as its input and returns a sequence of instructions
 * that implement the consumed instruction.  This function type represents
 * such a lowering function.  When called, a function with this prototype
 * should either return NULL indicating that no lowering needs to be done or
 * emit a sequence of instructions using the provided builder (whose cursor
 * will already be placed after the instruction to be lowered) and return the
 * resulting nir_def.
 */
typedef nir_def *(*nir_lower_instr_cb)(nir_builder *,
                                       nir_instr *, void *);

/**
 * Special return value for nir_lower_instr_cb when some progress occurred
 * (like changing an input to the instr) that didn't result in a replacement
 * SSA def being generated.
 */
#define NIR_LOWER_INSTR_PROGRESS ((nir_def *)(uintptr_t)1)

/**
 * Special return value for nir_lower_instr_cb when some progress occurred
 * that should remove the current instruction that doesn't create an output
 * (like a store)
 */

#define NIR_LOWER_INSTR_PROGRESS_REPLACE ((nir_def *)(uintptr_t)2)

/** Iterate over all the instructions in a nir_function_impl and lower them
 *  using the provided callbacks
 *
 * This function implements the guts of a standard lowering pass for you.  It
 * iterates over all of the instructions in a nir_function_impl and calls the
 * filter callback on each one.  If the filter callback returns true, it then
 * calls the lowering call back on the instruction.  (Splitting it this way
 * allows us to avoid some save/restore work for instructions we know won't be
 * lowered.)  If the instruction is dead after the lowering is complete, it
 * will be removed.  If new instructions are added, the lowering callback will
 * also be called on them in case multiple lowerings are required.
 *
 * If the callback indicates that the original instruction is replaced (either
 * through a new SSA def or NIR_LOWER_INSTR_PROGRESS_REPLACE), then the
 * instruction is removed along with any now-dead SSA defs it used.
 *
 * The metadata for the nir_function_impl will also be updated.  If any blocks
 * are added (they cannot be removed), dominance and block indices will be
 * invalidated.
 */
bool nir_function_impl_lower_instructions(nir_function_impl *impl,
                                          nir_instr_filter_cb filter,
                                          nir_lower_instr_cb lower,
                                          void *cb_data);
bool nir_shader_lower_instructions(nir_shader *shader,
                                   nir_instr_filter_cb filter,
                                   nir_lower_instr_cb lower,
                                   void *cb_data);

void nir_calc_dominance_impl(nir_function_impl *impl);
void nir_calc_dominance(nir_shader *shader);
void nir_calc_dominance_lca_impl(nir_function_impl *impl);

/**
 * Computes the lowest common ancestor of two blocks in the dominator tree.
 *
 * If one of the blocks is null or unreachable, the other block is returned or
 * NULL if it's unreachable.
 *
 * Requires nir_metadata_dominance_lca
 */
nir_block *nir_dominance_lca(nir_block *b1, nir_block *b2);

bool nir_block_dominates(nir_block *parent, nir_block *child);
bool nir_block_is_unreachable(nir_block *block);

void nir_dump_dom_tree_impl(nir_function_impl *impl, FILE *fp);
void nir_dump_dom_tree(nir_shader *shader, FILE *fp);

void nir_dump_dom_frontier_impl(nir_function_impl *impl, FILE *fp);
void nir_dump_dom_frontier(nir_shader *shader, FILE *fp);

void nir_dump_cfg_impl(nir_function_impl *impl, FILE *fp);
void nir_dump_cfg(nir_shader *shader, FILE *fp);

void nir_gs_count_vertices_and_primitives(const nir_shader *shader,
                                          int *out_vtxcnt,
                                          int *out_prmcnt,
                                          int *out_decomposed_prmcnt,
                                          unsigned num_streams);

typedef enum {
   nir_group_all,
   nir_group_same_resource_only,
} nir_load_grouping;

bool nir_opt_group_loads(nir_shader *shader, nir_load_grouping grouping,
                         unsigned max_distance);

bool nir_shrink_vec_array_vars(nir_shader *shader, nir_variable_mode modes);
bool nir_split_array_vars(nir_shader *shader, nir_variable_mode modes);
bool nir_split_var_copies(nir_shader *shader);
bool nir_separate_merged_clip_cull_io(nir_shader *nir);
bool nir_split_per_member_structs(nir_shader *shader);
bool nir_split_struct_vars(nir_shader *shader, nir_variable_mode modes);

bool nir_lower_returns_impl(nir_function_impl *impl);
bool nir_lower_returns(nir_shader *shader);

bool nir_lower_workgroup_size(nir_shader *shader, const uint32_t target_wg_size);

nir_def *nir_inline_function_impl(nir_builder *b,
                                  const nir_function_impl *impl,
                                  nir_def **params,
                                  struct hash_table *shader_var_remap);
bool nir_inline_functions(nir_shader *shader);
void nir_cleanup_functions(nir_shader *shader);
bool nir_link_shader_functions(nir_shader *shader,
                               const nir_shader *link_shader);
bool nir_shader_fully_linked(const nir_shader *shader);
bool nir_lower_calls_to_builtins(nir_shader *s);

void nir_find_inlinable_uniforms(nir_shader *shader);
bool nir_inline_uniforms(nir_shader *shader, unsigned num_uniforms,
                         const uint32_t *uniform_values,
                         const uint16_t *uniform_dw_offsets);

bool nir_inline_sysval(nir_shader *shader, nir_intrinsic_op op, uint64_t imm);

bool nir_propagate_invariant(nir_shader *shader, bool invariant_prim);

void nir_lower_var_copy_instr(nir_intrinsic_instr *copy, nir_shader *shader);
void nir_lower_deref_copy_instr(nir_builder *b,
                                nir_intrinsic_instr *copy);
bool nir_lower_var_copies(nir_shader *shader);

bool nir_opt_memcpy(nir_shader *shader);
bool nir_lower_memcpy(nir_shader *shader);

bool nir_fixup_deref_modes(nir_shader *shader);
bool nir_fixup_deref_types(nir_shader *shader);

bool nir_lower_global_vars_to_local(nir_shader *shader);
void nir_lower_constant_to_temp(nir_shader *shader);

typedef enum {
   nir_lower_direct_array_deref_of_vec_load = (1 << 0),
   nir_lower_indirect_array_deref_of_vec_load = (1 << 1),
   nir_lower_direct_array_deref_of_vec_store = (1 << 2),
   nir_lower_indirect_array_deref_of_vec_store = (1 << 3),
} nir_lower_array_deref_of_vec_options;

bool nir_lower_array_deref_of_vec(nir_shader *shader, nir_variable_mode modes,
                                  bool (*filter)(nir_variable *),
                                  nir_lower_array_deref_of_vec_options options);

bool nir_lower_indirect_derefs_to_if_else_trees(nir_shader *shader,
                                                nir_variable_mode modes,
                                                uint32_t max_lower_array_len);

bool nir_lower_indirect_var_derefs_to_if_else_trees(nir_shader *shader,
                                                    const struct set *vars);

bool nir_lower_locals_to_regs(nir_shader *shader, uint8_t bool_bitsize);

bool nir_downgrade_pls_vars(nir_shader *shader);

bool nir_lower_io_vars_to_temporaries(nir_shader *shader,
                                      nir_function_impl *entrypoint,
                                      nir_variable_mode modes);

bool nir_lower_vars_to_scratch(nir_shader *shader,
                               int size_threshold,
                               glsl_type_size_align_func variable_size_align,
                               glsl_type_size_align_func scratch_layout_size_align);

typedef void (*nir_lower_vars_to_scratch_cb)(struct util_dynarray *, void *);

bool nir_lower_vars_to_scratch_global(nir_shader *shader,
                                      glsl_type_size_align_func scratch_layout_size_align,
                                      nir_lower_vars_to_scratch_cb cb, void *data);

bool nir_lower_scratch_to_var(nir_shader *nir);

bool nir_lower_clip_halfz(nir_shader *shader);
bool nir_lower_clip_halfz_dynamic(nir_shader *shader);

void nir_shader_gather_info(nir_shader *shader, nir_function_impl *entrypoint);
void nir_gather_clip_cull_distance_sizes_from_vars(nir_shader *nir);

void nir_gather_types(nir_function_impl *impl,
                      BITSET_WORD *float_types,
                      BITSET_WORD *int_types);

/* Some helpers to do very simple linking */
bool nir_remove_unused_varyings(nir_shader *producer, nir_shader *consumer);
bool nir_remove_unused_io_vars(nir_shader *shader, nir_variable_mode mode,
                               uint64_t *used_by_other_stage,
                               uint64_t *used_by_other_stage_patches);
void nir_compact_varyings(nir_shader *producer, nir_shader *consumer,
                          bool default_to_smooth_interp);
bool nir_link_opt_varyings(nir_shader *producer, nir_shader *consumer);
void nir_link_varying_precision(nir_shader *producer, nir_shader *consumer);
nir_variable *nir_clone_uniform_variable(nir_shader *nir,
                                         nir_variable *uniform, bool spirv);
nir_deref_instr *nir_clone_deref_instr(nir_builder *b,
                                       nir_variable *var,
                                       nir_deref_instr *deref);


/* Return status from nir_opt_varyings. */
typedef enum {
   /* Whether the IR changed such that NIR optimizations should be run, such
    * as due to removal of loads and stores. IO semantic changes such as
    * compaction don't count as IR changes because they don't affect NIR
    * optimizations.
    */
   nir_progress_producer = BITFIELD_BIT(0),
   nir_progress_consumer = BITFIELD_BIT(1),
} nir_opt_varyings_progress;

nir_opt_varyings_progress
nir_opt_varyings(nir_shader *producer, nir_shader *consumer, bool spirv,
                 unsigned max_uniform_components, unsigned max_ubos_per_stage,
                 bool debug_no_algebraic);

unsigned
nir_varying_var_mask(nir_shader *nir);

void
nir_opt_varyings_bulk(nir_shader **shaders, uint32_t num_shaders, bool spirv,
                      unsigned max_uniform_comps, unsigned max_ubos,
                      void (*optimize)(nir_shader *, void *),
                      void *optimize_data);

unsigned nir_slot_num_components(gl_varying_slot slot, mesa_shader_stage stage);
bool nir_slot_is_sysval_output(gl_varying_slot slot,
                               mesa_shader_stage next_shader);
bool nir_slot_is_varying(gl_varying_slot slot, mesa_shader_stage next_shader);
bool nir_slot_is_sysval_output_and_varying(gl_varying_slot slot,
                                           mesa_shader_stage next_shader);
bool nir_remove_varying(nir_intrinsic_instr *intr, mesa_shader_stage next_shader);
bool nir_remove_sysval_output(nir_intrinsic_instr *intr, mesa_shader_stage next_shader);

bool nir_lower_amul(nir_shader *shader,
                    unsigned (*type_size)(const struct glsl_type *, bool));

bool nir_lower_ubo_vec4(nir_shader *shader);

void nir_sort_variables_by_location(nir_shader *shader, nir_variable_mode mode);
void nir_assign_io_var_locations(nir_shader *shader, nir_variable_mode mode);

bool nir_opt_clip_cull_const(nir_shader *shader);

typedef enum {
   /* If set, this causes all 64-bit IO loads and stores to be lowered to 32
    * bits + pack/unpack. This is only valid for nir_var_shader_in/out.
    *
    * Only VS inputs:
    *
    * This option should be used when VS input locations (nir_intrinsic_base
    * values) are final. With this option, dual-slot VS inputs must always
    * occupy 2 VS input locations. For example, VS inputs only consisting
    * of dual-slot inputs should have VS input locations set to 0,2,4,6 and
    * loading .xy from (base) and .zw from (base + 1). nir_lower_io will lower
    * 64 bits into 32 bits like this:
    *
    *   0(previously 0.xy), 1(previously 0.zw), ...
    *
    * After that's done, the information about whether load_input loads
    * the low or high portion of dual-slot inputs is not saved
    * in the intrinsics. This is why the location must be final or the driver
    * must keep the original VS input semantics info in a separate structure
    * since it can't be gathered from NIR anymore.
    *
    * TODO: This option should be removed because it offers no benefit over
    * the next one.
    */
   nir_lower_io_lower_64bit_to_32 = (1 << 0),

   /* If set, this causes all 64-bit IO loads and stores to be lowered to 32
    * bits + pack/unpack. This is only valid for nir_var_shader_in/out.
    *
    * Only VS inputs:
    *
    * This option allows eliminating VS inputs and thus bound vertex buffers
    * after nir_opt_varyings makes them dead. The information whether
    * load_input loads from the low or high portion of dual-slot inputs is
    * saved in nir_io_semantics::high_dvec2. shader_info::dual_slot_inputs is
    * set by nir_shader_gather_info for all inputs that have high_dvec2 == 1.
    * Since all information about dual-slot VS inputs is preserved in NIR, it
    * allows linking optimizations to run after nir_lower_io, eliminate dead VS
    * outputs, which then eliminates dead VS inputs, and the driver can gather
    * shader_info and stop binding eliminated vertex buffers. The GLSL linker
    * and st/mesa use this to reduce the number of bound vertex buffers for
    * gallium when nir_opt_varyings makes them dead. It also naturally demotes
    * dual-slot VS inputs to single-slot VS inputs when the high half of
    * dual-slot VS inputs is DCE'd.
    *
    * nir_recompute_io_bases assigns the following VS input locations for dual
    * slot VS inputs:
    *   0(low), 0(high_dvec2=1), 1(low), 1(high_dvec2=1)
    *
    * Alternatively, a prefix bitmask of shader_info::inputs_read can be used
    * to get the input locations instead of calling nir_recompute_io_bases.
    * That numbering is for OpenGL where a dual-slot VS input occupies exactly
    * 1 input location.
    *
    * However, any other numbering is possible. For example, Vulkan drivers may
    * assign these VS input locations after DCE, which can be done by computing
    * a prefix bitmask from shader_info::inputs_read + a prefix bitmask of
    * shader_info::dual_slot_inputs + high_dvec2 (the Intel driver does this),
    * resulting in:
    *   0(low), 1(high_dvec2=1), 2(low), 3(high_dvec2=1)
    */
   nir_lower_io_lower_64bit_to_32_new = (1 << 1),

   /* Should nir_lower_io() create load_interpolated_input intrinsics?
    *
    * If not, it generates regular load_input intrinsics and interpolation
    * information must be inferred from the list of input nir_variables.
    */
   nir_lower_io_use_interpolated_input_intrinsics = (1 << 2),
} nir_lower_io_options;
bool nir_lower_io(nir_shader *shader,
                  nir_variable_mode modes,
                  unsigned (*type_size)(const struct glsl_type *, bool),
                  nir_lower_io_options);

void nir_lower_io_passes(nir_shader *nir, bool renumber_vs_inputs);
bool nir_io_add_intrinsic_xfb_info(nir_shader *nir);
bool nir_lower_io_indirect_loads(nir_shader *nir, nir_variable_mode modes);
bool nir_remove_outputs(nir_shader *shader, mesa_shader_stage next_stage,
                        uint64_t remove_varying, uint64_t remove_sysval);

bool
nir_lower_vars_to_explicit_types(nir_shader *shader,
                                 nir_variable_mode modes,
                                 glsl_type_size_align_func type_info);
void
nir_gather_explicit_io_initializers(nir_shader *shader,
                                    void *dst, size_t dst_size,
                                    nir_variable_mode mode);

bool nir_lower_vec3_to_vec4(nir_shader *shader, nir_variable_mode modes);

unsigned
nir_address_format_bit_size(nir_address_format addr_format);

unsigned
nir_address_format_num_components(nir_address_format addr_format);

static inline const struct glsl_type *
nir_address_format_to_glsl_type(nir_address_format addr_format)
{
   unsigned bit_size = nir_address_format_bit_size(addr_format);
   assert(bit_size == 32 || bit_size == 64);
   return glsl_vector_type(bit_size == 32 ? GLSL_TYPE_UINT : GLSL_TYPE_UINT64,
                           nir_address_format_num_components(addr_format));
}

const nir_const_value *nir_address_format_null_value(nir_address_format addr_format);

nir_def *nir_build_addr_iadd(nir_builder *b, nir_def *addr,
                             nir_address_format addr_format,
                             nir_variable_mode modes,
                             nir_def *offset);

nir_def *nir_build_addr_iadd_imm(nir_builder *b, nir_def *addr,
                                 nir_address_format addr_format,
                                 nir_variable_mode modes,
                                 int64_t offset);

nir_def *nir_build_addr_ieq(nir_builder *b, nir_def *addr0, nir_def *addr1,
                            nir_address_format addr_format);

nir_def *nir_build_addr_isub(nir_builder *b, nir_def *addr0, nir_def *addr1,
                             nir_address_format addr_format);

nir_def *nir_explicit_io_address_from_deref(nir_builder *b,
                                            nir_deref_instr *deref,
                                            nir_def *base_addr,
                                            nir_address_format addr_format);

bool nir_get_explicit_deref_align(nir_deref_instr *deref,
                                  bool default_to_type_align,
                                  uint32_t *align_mul,
                                  uint32_t *align_offset);

void nir_lower_explicit_io_instr(nir_builder *b,
                                 nir_intrinsic_instr *io_instr,
                                 nir_def *addr,
                                 nir_address_format addr_format);

bool nir_lower_explicit_io(nir_shader *shader,
                           nir_variable_mode modes,
                           nir_address_format);

bool nir_convert_address_format(nir_shader *shader, nir_variable_mode modes,
                                nir_address_format from, nir_address_format to);
nir_def *nir_build_convert_address_format(nir_builder *b, nir_def *addr,
                                          nir_address_format from,
                                          nir_address_format to);

typedef enum {
   /* Use open-coded funnel shifts for each component. */
   nir_mem_access_shift_method_scalar,
   /* Prefer to use 64-bit shifts to do the same with less instructions. Useful
    * if 64-bit shifts are cheap.
    */
   nir_mem_access_shift_method_shift64,
   /* If nir_op_alignbyte_amd can be used, this is the best option with just a
    * single nir_op_alignbyte_amd for each 32-bit components.
    */
   nir_mem_access_shift_method_bytealign_amd,
} nir_mem_access_shift_method;

typedef struct nir_mem_access_size_align {
   uint8_t num_components;
   uint8_t bit_size;
   uint16_t align;
   /* If a load's alignment is increased, this specifies how the data should be
    * shifted before converting to the original bit size.
    */
   nir_mem_access_shift_method shift;
} nir_mem_access_size_align;

/* clang-format off */
typedef nir_mem_access_size_align
   (*nir_lower_mem_access_bit_sizes_cb)(nir_intrinsic_op intrin,
                                        uint8_t bytes,
                                        uint8_t bit_size,
                                        uint32_t align_mul,
                                        uint32_t align_offset,
                                        bool offset_is_const,
                                        enum gl_access_qualifier,
                                        const void *cb_data);
/* clang-format on */

typedef struct nir_lower_mem_access_bit_sizes_options {
   nir_lower_mem_access_bit_sizes_cb callback;
   nir_variable_mode modes;
   bool may_lower_unaligned_stores_to_atomics;
   void *cb_data;
} nir_lower_mem_access_bit_sizes_options;

bool nir_lower_mem_access_bit_sizes(nir_shader *shader,
                                    const nir_lower_mem_access_bit_sizes_options *options);

bool nir_lower_robust_access(nir_shader *s,
                             nir_intrin_filter_cb filter, const void *data);

/* clang-format off */
typedef bool (*nir_should_vectorize_mem_func)(unsigned align_mul,
                                              unsigned align_offset,
                                              unsigned bit_size,
                                              unsigned num_components,
                                              /* The hole between low and
                                               * high if they are not adjacent. */
                                              int64_t hole_size,
                                              nir_intrinsic_instr *low,
                                              nir_intrinsic_instr *high,
                                              void *data);
/* clang-format on */

typedef struct nir_load_store_vectorize_options {
   nir_should_vectorize_mem_func callback;
   unsigned (*round_up_components)(unsigned);
   nir_variable_mode modes;
   nir_variable_mode robust_modes;
   nir_variable_mode bounds_checked_modes; /* modes with per-component bounds-checking */
   void *cb_data;
   bool has_shared2_amd;
   bool round_up_store_components;
} nir_load_store_vectorize_options;

bool nir_opt_load_store_vectorize(nir_shader *shader, const nir_load_store_vectorize_options *options);
bool nir_opt_load_store_update_alignments(nir_shader *shader);

typedef bool (*nir_lower_shader_calls_should_remat_func)(nir_instr *instr, void *data);

typedef struct nir_lower_shader_calls_options {
   /* Address format used for load/store operations on the call stack. */
   nir_address_format address_format;

   /* Stack alignment */
   unsigned stack_alignment;

   /* Put loads from the stack as close as possible from where they're needed.
    * You might want to disable combined_loads for best effects.
    */
   bool localized_loads;

   /* If this function pointer is not NULL, lower_shader_calls will run
    * nir_opt_load_store_vectorize for stack load/store operations. Otherwise
    * the optimizaion is not run.
    */
   nir_should_vectorize_mem_func vectorizer_callback;

   /* Data passed to vectorizer_callback */
   void *vectorizer_data;

   /* If this function pointer is not NULL, lower_shader_calls will call this
    * function on instructions that require spill/fill/rematerialization of
    * their value. If this function returns true, lower_shader_calls will
    * ensure that the instruction is rematerialized, adding the sources of the
    * instruction to be spilled/filled.
    */
   nir_lower_shader_calls_should_remat_func should_remat_callback;

   /* Data passed to should_remat_callback */
   void *should_remat_data;
} nir_lower_shader_calls_options;

bool
nir_lower_shader_calls(nir_shader *shader,
                       const nir_lower_shader_calls_options *options,
                       nir_shader ***resume_shaders_out,
                       uint32_t *num_resume_shaders_out,
                       void *mem_ctx);

int nir_get_io_offset_src_number(const nir_intrinsic_instr *instr);
int nir_get_io_uniform_offset_src_number(const nir_intrinsic_instr *instr);
int nir_get_io_index_src_number(const nir_intrinsic_instr *instr);
int nir_get_io_data_src_number(const nir_intrinsic_instr *instr);
int nir_get_io_arrayed_index_src_number(const nir_intrinsic_instr *instr);

nir_src *nir_get_io_offset_src(nir_intrinsic_instr *instr);
nir_src *nir_get_io_uniform_offset_src(nir_intrinsic_instr *instr);
nir_src *nir_get_io_index_src(nir_intrinsic_instr *instr);
nir_src *nir_get_io_data_src(nir_intrinsic_instr *instr);
nir_src *nir_get_io_arrayed_index_src(nir_intrinsic_instr *instr);
nir_src *nir_get_shader_call_payload_src(nir_intrinsic_instr *call);

static inline unsigned
nir_get_io_base_size_nv(const nir_intrinsic_instr *intr)
{
   switch (intr->intrinsic) {
   case nir_intrinsic_global_atomic_swap_nv:
   case nir_intrinsic_shared_atomic_nv:
   case nir_intrinsic_shared_atomic_swap_nv:
   case nir_intrinsic_load_global_nv:
   case nir_intrinsic_load_scratch_nv:
   case nir_intrinsic_load_shared_nv:
   case nir_intrinsic_load_shared_lock_nv:
   case nir_intrinsic_store_global_nv:
   case nir_intrinsic_store_scratch_nv:
   case nir_intrinsic_store_shared_nv:
   case nir_intrinsic_store_shared_unlock_nv:
      return 24;
   case nir_intrinsic_global_atomic_nv:
      /* TODO: SM100+ only has 23 bits for the UGPR + GPR form */
      return 23;
   case nir_intrinsic_ldc_nv:
   case nir_intrinsic_ldcx_nv:
      return 16;
   case nir_intrinsic_vild_nv:
      return 8;
   default:
      UNREACHABLE("unknown nvidia intrinsic");
      return -1;
   }
}

bool nir_is_shared_access(nir_intrinsic_instr *intr);
bool nir_is_output_load(nir_intrinsic_instr *intr);
bool nir_is_input_load(nir_intrinsic_instr *intr);
bool nir_is_arrayed_io(const nir_variable *var, mesa_shader_stage stage);

/* Represents an offset used by intrinsics that support the offset_shift
 * index. The final offset in bytes is (def << shift).
 */
typedef struct {
   nir_def *def;
   unsigned shift;
} nir_io_offset;

nir_io_offset nir_io_offset_iadd(nir_builder *b, nir_intrinsic_instr *intr,
                                 int offset_diff_bytes);
void nir_set_io_offset(nir_intrinsic_instr *intr, nir_io_offset offset);
void nir_add_io_offset(nir_builder *b, nir_intrinsic_instr *intr,
                       int offset_diff_bytes);

bool nir_lower_reg_intrinsics_to_ssa_impl(nir_function_impl *impl);
bool nir_lower_reg_intrinsics_to_ssa(nir_shader *shader);
bool nir_lower_vars_to_ssa(nir_shader *shader);

bool nir_remove_dead_derefs(nir_shader *shader);
bool nir_remove_dead_derefs_impl(nir_function_impl *impl);

typedef struct nir_remove_dead_variables_options {
   bool (*can_remove_var)(nir_variable *var, void *data);
   void *can_remove_var_data;
} nir_remove_dead_variables_options;

bool nir_remove_dead_variables(nir_shader *shader, nir_variable_mode modes,
                               const nir_remove_dead_variables_options *options);

bool nir_lower_variable_initializers(nir_shader *shader,
                                     nir_variable_mode modes);
bool nir_zero_initialize_shared_memory(nir_shader *shader,
                                       const unsigned shared_size,
                                       const unsigned chunk_size);
bool nir_clear_shared_memory(nir_shader *shader,
                             const unsigned shared_size,
                             const unsigned chunk_size);

typedef enum {
   /* If the instructions to move are in the function entry block, do nothing,
    * else move them at the end (not the beginning) of the entry block.
    *
    * If this is not set, all selected instructions are always moved
    * to the beginning of the entry block.
    *
    * This has the following advantages:
    * - not moving all the way to the beginning reduces register usage within
    *   the entry block
    * - CSE within the entry block is still maximally effective
    *   (nir_opt_varyings recommends that each input component is loaded only
    *    once, and this option + CSE guarantees that)
    * - the pass does nothing if all affected instructions are already
    *   in the entry block.
    */
   nir_move_to_entry_block_only = BITFIELD_BIT(0),

   /* Instruction options. */

   /* Simple input loads are non-interpolated loads and interpolated loads
    * with pixel, centroid, and sample barycentrics. Other barycentrics are
    * excluded.
    */
   nir_move_to_top_input_loads_simple = BITFIELD_BIT(1),

   /* Interpolated loads with non-trivial barycentrics, such as at_offset and
    * at_sample. (this option is not recommended for Control (game) because
    * it moves at_sample with complex ALU perspective-correct interpolation
    * out of conditional blocks)
    */
   nir_move_to_top_input_loads_complex_baryc = BITFIELD_BIT(2),

   nir_move_to_top_load_smem_amd = BITFIELD_BIT(3),
} nir_opt_move_to_top_options;

bool nir_opt_move_to_top(nir_shader *nir, nir_opt_move_to_top_options options);

bool nir_move_vec_src_uses_to_dest(nir_shader *shader, bool skip_const_srcs);
bool nir_move_output_stores_to_end(nir_shader *nir);
bool nir_lower_vec_to_regs(nir_shader *shader, nir_instr_writemask_filter_cb cb,
                           const void *_data);

bool nir_lower_alpha_to_coverage(nir_shader *shader,
                                 bool has_intrinsic,
                                 nir_def *dyn_enable);

bool nir_lower_alpha_to_one(nir_shader *shader);

bool nir_lower_alu(nir_shader *shader);

bool nir_lower_flrp(nir_shader *shader, unsigned lowering_mask,
                    bool always_precise);

bool nir_scale_fdiv(nir_shader *shader);

bool nir_lower_alu_to_scalar(nir_shader *shader, nir_instr_filter_cb cb, const void *data);
bool nir_lower_alu_width(nir_shader *shader, nir_vectorize_cb cb, const void *data);
bool nir_lower_alu_vec8_16_srcs(nir_shader *shader);
bool nir_lower_bool_to_float(nir_shader *shader, bool has_fcsel_ne);
bool nir_lower_bool_to_int32(nir_shader *shader);
bool nir_opt_simplify_convert_alu_types(nir_shader *shader);
bool nir_lower_const_arrays_to_uniforms(nir_shader *shader,
                                        unsigned max_uniform_components);
bool nir_lower_convert_alu_types(nir_shader *shader,
                                 bool (*should_lower)(nir_intrinsic_instr *));
bool nir_lower_constant_convert_alu_types(nir_shader *shader);
bool nir_lower_alu_conversion_to_intrinsic(nir_shader *shader);
bool nir_lower_int_to_float(nir_shader *shader);
bool nir_lower_load_const_to_scalar(nir_shader *shader);
bool nir_lower_read_invocation_to_scalar(nir_shader *shader);
bool nir_lower_phis_to_scalar(nir_shader *shader, nir_vectorize_cb cb, const void *data);
bool nir_lower_all_phis_to_scalar(nir_shader *shader);
void nir_lower_io_array_vars_to_elements(nir_shader *producer, nir_shader *consumer);
bool nir_lower_io_array_vars_to_elements_no_indirects(nir_shader *shader,
                                                      bool outputs_only);
bool nir_lower_io_to_scalar(nir_shader *shader, nir_variable_mode mask, nir_intrin_filter_cb filter, void *filter_data);
bool nir_lower_io_vars_to_scalar(nir_shader *shader, nir_variable_mode mask);
bool nir_opt_vectorize_io_vars(nir_shader *shader, nir_variable_mode mask);
bool nir_lower_tess_level_array_vars_to_vec(nir_shader *shader);
nir_shader *nir_create_passthrough_tcs_impl(const nir_shader_compiler_options *options,
                                            unsigned *locations, unsigned num_locations,
                                            uint8_t patch_vertices);
nir_shader *nir_create_passthrough_tcs(const nir_shader_compiler_options *options,
                                       const nir_shader *vs, uint8_t patch_vertices);
nir_shader *nir_create_passthrough_gs(const nir_shader_compiler_options *options,
                                      const nir_shader *prev_stage,
                                      enum mesa_prim primitive_type,
                                      enum mesa_prim output_primitive_type,
                                      bool emulate_edgeflags,
                                      bool force_line_strip_out,
                                      bool passthrough_prim_id);

bool nir_lower_fragcolor(nir_shader *shader, unsigned max_cbufs);
bool nir_lower_fragcoord_wtrans(nir_shader *shader);
bool nir_opt_frag_coord_to_pixel_coord(nir_shader *shader);
bool nir_lower_frag_coord_to_pixel_coord(nir_shader *shader);
bool nir_lower_viewport_transform(nir_shader *shader);
bool nir_lower_uniforms_to_ubo(nir_shader *shader, bool dword_packed, bool load_vec4);

bool nir_lower_is_helper_invocation(nir_shader *shader);

typedef struct nir_lower_single_sampled_options {
   bool lower_sample_mask_in;
} nir_lower_single_sampled_options;

bool nir_lower_single_sampled(nir_shader *shader,
                              const nir_lower_single_sampled_options *options);
bool nir_lower_sample_shading(nir_shader *shader);

bool nir_lower_atomics(nir_shader *shader, nir_instr_filter_cb filter);

typedef struct nir_lower_subgroups_options {
   /* In addition to the boolean lowering options below, this optional callback
    * will filter instructions for lowering if non-NULL. The data passed will be
    * filter_data.
    */
   nir_intrin_filter_cb filter;

   /* Extra data passed to the filter. */
   const void *filter_data;

   /* In case the exact subgroup size is not known, subgroup_size should be
    * set to 0. In that case, the maximum subgroup size will be calculated by
    * ballot_components * ballot_bit_size.
    */
   uint8_t subgroup_size;
   uint8_t ballot_bit_size;
   uint8_t ballot_components;
   bool lower_to_scalar : 1;
   bool lower_vote : 1;
   bool lower_vote_trivial : 1;
   bool lower_vote_feq : 1;
   bool lower_vote_ieq : 1;
   bool lower_vote_bool_eq : 1;
   bool lower_first_invocation_to_ballot : 1;
   bool lower_read_first_invocation : 1;
   bool lower_subgroup_masks : 1;
   bool lower_relative_shuffle : 1;
   bool lower_shuffle_to_32bit : 1;
   bool lower_shuffle_to_swizzle_amd : 1;
   bool lower_shuffle : 1;
   bool lower_quad : 1;
   bool lower_quad_broadcast_dynamic : 1;
   bool lower_quad_broadcast_dynamic_to_const : 1;
   bool lower_quad_vote : 1;
   bool lower_elect : 1;
   bool lower_read_invocation_to_cond : 1;
   bool lower_rotate_to_shuffle : 1;
   bool lower_rotate_clustered_to_shuffle : 1;
   bool lower_ballot_bit_count_to_mbcnt_amd : 1;
   bool lower_inverse_ballot : 1;
   bool lower_reduce : 1;
   bool lower_boolean_reduce : 1;
   bool lower_boolean_shuffle : 1;
} nir_lower_subgroups_options;

bool nir_lower_subgroups(nir_shader *shader,
                         const nir_lower_subgroups_options *options);

bool nir_lower_system_values(nir_shader *shader);

nir_def *
nir_build_lowered_load_helper_invocation(nir_builder *b);

typedef struct nir_lower_compute_system_values_options {
   bool has_base_global_invocation_id : 1;
   bool has_base_workgroup_id : 1;
   bool has_global_size : 1;
   bool shuffle_local_ids_for_quad_derivatives : 1;
   bool lower_local_invocation_index : 1;
   bool lower_cs_local_id_to_index : 1;
   bool lower_workgroup_id_to_index : 1;
   bool global_id_is_32bit : 1;
   /* At shader execution time, check if WorkGroupId should be 1D
    * and compute it quickly. Fall back to slow computation if not.
    */
   bool shortcut_1d_workgroup_id : 1;
   uint32_t num_workgroups[3]; /* Compile-time-known dispatch sizes, or 0 if unknown. */
} nir_lower_compute_system_values_options;

bool nir_lower_compute_system_values(nir_shader *shader,
                                     const nir_lower_compute_system_values_options *options);

typedef struct nir_lower_sysvals_to_varyings_options {
   bool frag_coord : 1;
   bool front_face : 1;
   bool point_coord : 1;
   bool layer_id : 1;
   bool view_index : 1;
   bool primitive_id : 1;
} nir_lower_sysvals_to_varyings_options;

bool
nir_lower_sysvals_to_varyings(nir_shader *shader,
                              const nir_lower_sysvals_to_varyings_options *options);

/***/
enum ENUM_PACKED nir_lower_tex_packing {
   /** No packing */
   nir_lower_tex_packing_none = 0,
   /**
    * The sampler returns up to 2 32-bit words of half floats or 16-bit signed
    * or unsigned ints based on the sampler type
    */
   nir_lower_tex_packing_16,
   /** The sampler returns 1 32-bit word of 4x8 unorm */
   nir_lower_tex_packing_8,
};

/***/
typedef struct nir_lower_tex_options {
   /**
    * bitmask of (1 << GLSL_SAMPLER_DIM_x) to control for which
    * sampler types a texture projector is lowered.
    */
   unsigned lower_txp;

   /**
    * If true, lower texture projector for any array sampler dims
    */
   bool lower_txp_array;

   /**
    * If true, lower away nir_tex_src_offset for all texelfetch instructions.
    */
   bool lower_txf_offset;

   /**
    * If true, lower away nir_tex_src_offset for all rect textures.
    */
   bool lower_rect_offset;

   /**
    * If not NULL, this filter will return true for tex instructions that
    * should lower away nir_tex_src_offset.
    */
   nir_instr_filter_cb lower_offset_filter;

   /**
    * If true, lower rect textures to 2D, using txs to fetch the
    * texture dimensions and dividing the texture coords by the
    * texture dims to normalize.
    */
   bool lower_rect;

   /**
    * If true, lower 1D textures to 2D. This requires the GL/VK driver to map 1D
    * textures to 2D textures with height=1.
    *
    * lower_1d_shadow does this lowering for shadow textures only.
    */
   bool lower_1d;
   bool lower_1d_shadow;

   /**
    * If true, convert yuv to rgb.
    */
   unsigned lower_y_uv_external;
   unsigned lower_y_vu_external;
   unsigned lower_y_u_v_external;
   unsigned lower_yx_xuxv_external;
   unsigned lower_yx_xvxu_external;
   unsigned lower_xy_uxvx_external;
   unsigned lower_xy_vxux_external;
   unsigned lower_ayuv_external;
   unsigned lower_xyuv_external;
   unsigned lower_yuv_external;
   unsigned lower_yu_yv_external;
   unsigned lower_yv_yu_external;
   unsigned lower_y41x_external;
   unsigned lower_sx10_external;
   unsigned lower_sx12_external;
   unsigned bt709_external;
   unsigned bt2020_external;
   unsigned yuv_full_range_external;

   /**
    * To emulate certain texture wrap modes, this can be used
    * to saturate the specified tex coord to [0.0, 1.0].  The
    * bits are according to sampler #, ie. if, for example:
    *
    *   (conf->saturate_s & (1 << n))
    *
    * is true, then the s coord for sampler n is saturated.
    *
    * Note that clamping must happen *after* projector lowering
    * so any projected texture sample instruction with a clamped
    * coordinate gets automatically lowered, regardless of the
    * 'lower_txp' setting.
    */
   unsigned saturate_s;
   unsigned saturate_t;
   unsigned saturate_r;

   /* Bitmask of textures that need swizzling.
    *
    * If (swizzle_result & (1 << texture_index)), then the swizzle in
    * swizzles[texture_index] is applied to the result of the texturing
    * operation.
    */
   unsigned swizzle_result;

   /* A swizzle for each texture.  Values 0-3 represent x, y, z, or w swizzles
    * while 4 and 5 represent 0 and 1 respectively.
    *
    * Indexed by texture-id.
    */
   uint8_t swizzles[32][4];

   /* Can be used to scale sampled values in range required by the
    * format.
    *
    * Indexed by texture-id.
    */
   float scale_factors[32];

   /**
    * Bitmap of textures that need srgb to linear conversion.  If
    * (lower_srgb & (1 << texture_index)) then the rgb (xyz) components
    * of the texture are lowered to linear.
    */
   unsigned lower_srgb;

   /**
    * If true, lower nir_texop_txd on cube maps with nir_texop_txl.
    */
   bool lower_txd_cube_map;

   /**
    * If true, lower nir_texop_txd on 3D surfaces with nir_texop_txl.
    */
   bool lower_txd_3d;

   /**
    * If true, lower nir_texop_txd any array surfaces with nir_texop_txl.
    */
   bool lower_txd_array;

   /**
    * If true, lower nir_texop_txd on shadow samplers (except cube maps)
    * with nir_texop_txl. Notice that cube map shadow samplers are lowered
    * with lower_txd_cube_map.
    */
   bool lower_txd_shadow;

   /**
    * If true, lower nir_texop_txd on all samplers to a nir_texop_txl.
    * Implies lower_txd_cube_map and lower_txd_shadow.
    */
   bool lower_txd;

   /**
    * If true, lower nir_texop_txd  when it uses min_lod.
    */
   bool lower_txd_clamp;

   /**
    * If true, lower nir_texop_txb that try to use shadow compare and min_lod
    * at the same time to a nir_texop_lod, some math, and nir_texop_tex.
    */
   bool lower_txb_shadow_clamp;

   /**
    * A generic callback to nir_texop_txd.
    */
   bool (*lower_txd_cb)(const nir_tex_instr *tex, const void *data);
   const void *lower_txd_data;

   /**
    * If true, lower nir_texop_txs with a non-0-lod into nir_texop_txs with
    * 0-lod followed by a nir_ishr.
    */
   bool lower_txs_lod;

   /**
    * If true, lower nir_texop_txs for cube arrays to a nir_texop_txs with a
    * 2D array type followed by a nir_idiv by 6.
    */
   bool lower_txs_cube_array;

   /**
    * If true, apply a .bagr swizzle on tg4 results to handle Broadcom's
    * mixed-up tg4 locations.
    */
   bool lower_tg4_broadcom_swizzle;

   /**
    * If true, lowers tg4 with 4 constant offsets to 4 tg4 calls
    */
   bool lower_tg4_offsets;

   /**
    * Lower txf_ms to fragment_mask_fetch and fragment_fetch and samples_identical to
    * fragment_mask_fetch.
    */
   bool lower_to_fragment_fetch_amd;

   /**
    * To lower packed sampler return formats. This will be called for all
    * tex instructions.
    */
   enum nir_lower_tex_packing (*lower_tex_packing_cb)(const nir_tex_instr *tex, const void *data);
   const void *lower_tex_packing_data;

   /**
    * If true, lower nir_texop_lod to return -FLT_MAX if the sum of the
    * absolute values of derivatives is 0 for all coordinates.
    */
   bool lower_lod_zero_width;

   /**
    * If true, emulates sampler descriptor LOD bias by adding the sampler
    * LOD bias to every texture instruction's LOD.
    */
   bool lower_sampler_lod_bias;

   /* Turns nir_op_tex and other ops with an implicit derivative, in stages
    * without implicit derivatives (like the vertex shader) to have an explicit
    * LOD with a value of 0.
    */
   bool lower_invalid_implicit_lod;

   /* If true, texture_index (sampler_index) will be zero if a texture_offset
    * (sampler_offset) source is present. This is convenient for backends that
    * support indirect indexing of textures (samplers) but not offsetting it.
    */
   bool lower_index_to_offset;

   /* Optimize txd(coord, ddxy_coarse(coord)) to tex(coord). */
   bool optimize_txd;

   /* If true, lower tg4 shadow compare operations to 16bit. */
   bool lower_tg4_shadow_to_16bit;

   /**
    * Payload data to be sent to callback / filter functions.
    */
   void *callback_data;
} nir_lower_tex_options;

unsigned nir_tex_parse_txd_coords(nir_shader *shader, nir_tex_instr *tex, nir_instr **ddxy_instrs);

/** Lowers complex texture instructions to simpler ones */
bool nir_lower_tex(nir_shader *shader,
                   const nir_lower_tex_options *options);

typedef struct nir_lower_tex_shadow_swizzle {
   unsigned swizzle_r : 3;
   unsigned swizzle_g : 3;
   unsigned swizzle_b : 3;
   unsigned swizzle_a : 3;
} nir_lower_tex_shadow_swizzle;

bool
nir_lower_tex_shadow(nir_shader *s,
                     unsigned n_states,
                     enum compare_func *compare_func,
                     nir_lower_tex_shadow_swizzle *tex_swizzles,
                     bool is_fixed_point_format);

bool
nir_update_image_intrinsic_from_var(nir_shader *nir);

typedef struct nir_lower_image_options {
   /**
    * If true, lower cube size operations.
    */
   bool lower_cube_size;

   /**
    * Lower multi sample image load and samples_identical to use fragment_mask_load.
    */
   bool lower_to_fragment_mask_load_amd;

   /**
    * Lower image_samples to a constant in case the driver doesn't support multisampled
    * images.
    */
   bool lower_image_samples_to_one;
} nir_lower_image_options;

bool nir_lower_image(nir_shader *nir,
                     const nir_lower_image_options *options);

bool
nir_lower_image_atomics_to_global(nir_shader *s,
                                  nir_intrin_filter_cb filter,
                                  const void *data);

bool nir_lower_readonly_images_to_tex(nir_shader *shader, bool per_variable);

enum nir_lower_non_uniform_access_type {
   nir_lower_non_uniform_ubo_access = (1 << 0),
   nir_lower_non_uniform_ssbo_access = (1 << 1),
   nir_lower_non_uniform_texture_access = (1 << 2),
   nir_lower_non_uniform_image_access = (1 << 3),
   nir_lower_non_uniform_get_ssbo_size = (1 << 4),
   nir_lower_non_uniform_texture_offset_access = (1 << 5),
   nir_lower_non_uniform_texture_query = (1 << 6),
   nir_lower_non_uniform_image_query = (1 << 7),
   nir_lower_non_uniform_access_type_count = 8,
};

typedef bool (*nir_lower_non_uniform_src_access_callback)(const nir_tex_instr *, unsigned, void *);
/* Given the nir_src used for the resource, return the channels which might be non-uniform. */
typedef nir_component_mask_t (*nir_lower_non_uniform_access_callback)(const nir_src *, void *);

typedef struct nir_lower_non_uniform_access_options {
   enum nir_lower_non_uniform_access_type types;
   /* Called on nir_tex_instr to ask whether non-uniform lowering is required
    * for a particular source
    */
   nir_lower_non_uniform_src_access_callback tex_src_callback;
   nir_lower_non_uniform_access_callback callback;
   void *callback_data;
} nir_lower_non_uniform_access_options;

bool nir_has_non_uniform_access(nir_shader *shader, enum nir_lower_non_uniform_access_type types);
bool nir_opt_non_uniform_access(nir_shader *shader);
bool nir_lower_non_uniform_access(nir_shader *shader,
                                  const nir_lower_non_uniform_access_options *options);

typedef struct nir_lower_idiv_options {
   /* Whether 16-bit floating point arithmetic should be allowed in 8-bit
    * division lowering
    */
   bool allow_fp16;
} nir_lower_idiv_options;

bool nir_lower_idiv(nir_shader *shader, const nir_lower_idiv_options *options);

typedef struct nir_input_attachment_options {
   bool use_ia_coord_intrin;
   bool use_view_id_for_layer;
   bool gmem_depth_stencil_ir3;
   uint32_t gmem_input_attachment_ir3;
} nir_input_attachment_options;

bool nir_lower_input_attachments(nir_shader *shader,
                                 const nir_input_attachment_options *options);

bool nir_lower_clip_vs(nir_shader *shader, unsigned ucp_enables,
                       bool use_vars,
                       bool use_clipdist_array,
                       const gl_state_index16 clipplane_state_tokens[][STATE_LENGTH]);
bool nir_lower_clip_gs(nir_shader *shader, unsigned ucp_enables,
                       bool use_clipdist_array,
                       const gl_state_index16 clipplane_state_tokens[][STATE_LENGTH]);
bool nir_lower_clip_fs(nir_shader *shader, unsigned ucp_enables,
                       bool use_clipdist_array, bool use_load_interp);

bool nir_lower_clip_cull_distance_to_vec4s(nir_shader *shader);
bool nir_merge_clip_cull_distance_vars(nir_shader *nir);
bool nir_lower_clip_disable(nir_shader *shader, unsigned clip_plane_enable);

bool nir_lower_frexp(nir_shader *nir);

bool nir_lower_two_sided_color(nir_shader *shader, bool face_sysval);

bool nir_lower_clamp_color_outputs(nir_shader *shader);

bool nir_lower_flatshade(nir_shader *shader);

bool nir_lower_passthrough_edgeflags(nir_shader *shader);
bool nir_lower_patch_vertices(nir_shader *nir, unsigned static_count,
                              const gl_state_index16 *uniform_state_tokens);

typedef struct nir_lower_wpos_ytransform_options {
   gl_state_index16 state_tokens[STATE_LENGTH];
   bool fs_coord_origin_upper_left : 1;
   bool fs_coord_origin_lower_left : 1;
   bool fs_coord_pixel_center_integer : 1;
   bool fs_coord_pixel_center_half_integer : 1;
} nir_lower_wpos_ytransform_options;

bool nir_lower_wpos_ytransform(nir_shader *shader,
                               const nir_lower_wpos_ytransform_options *options);
bool nir_lower_wpos_center(nir_shader *shader);

bool nir_lower_pntc_ytransform(nir_shader *shader,
                               const gl_state_index16 clipplane_state_tokens[][STATE_LENGTH]);

bool nir_lower_wrmasks(nir_shader *shader);

bool nir_lower_fb_read(nir_shader *shader);

typedef struct nir_lower_bitmap_options {
   unsigned sampler;
   bool swizzle_xxxx;
} nir_lower_bitmap_options;

bool nir_lower_bitmap(nir_shader *shader, const nir_lower_bitmap_options *options);

bool nir_lower_atomics_to_ssbo(nir_shader *shader, unsigned offset_align_state);

typedef enum {
   nir_lower_gs_intrinsics_per_stream = 1 << 0,
   nir_lower_gs_intrinsics_count_primitives = 1 << 1,
   nir_lower_gs_intrinsics_count_vertices_per_primitive = 1 << 2,
   nir_lower_gs_intrinsics_overwrite_incomplete = 1 << 3,
} nir_lower_gs_intrinsics_flags;

bool nir_lower_gs_intrinsics(nir_shader *shader, nir_lower_gs_intrinsics_flags options);

bool nir_lower_halt_to_return(nir_shader *nir);

bool nir_lower_tess_coord_z(nir_shader *shader, bool triangles);

typedef struct nir_lower_task_shader_options {
   bool payload_to_shared_for_atomics : 1;
   bool payload_to_shared_for_small_types : 1;
   uint32_t payload_offset_in_bytes;
} nir_lower_task_shader_options;

bool nir_lower_task_shader(nir_shader *shader, nir_lower_task_shader_options options);

typedef unsigned (*nir_lower_bit_size_callback)(const nir_instr *, void *);

bool nir_lower_bit_size(nir_shader *shader,
                        nir_lower_bit_size_callback callback,
                        void *callback_data);
bool nir_lower_64bit_phis(nir_shader *shader);

typedef struct nir_split_conversions_options {
   nir_lower_bit_size_callback callback;
   void *callback_data;
   /* True if the implementation supports nir_intrinsic_convert_alu_types */
   bool has_convert_alu_types;
} nir_split_conversions_options;

bool nir_split_conversions(nir_shader *shader,
                           const nir_split_conversions_options *options);

bool nir_split_64bit_vec3_and_vec4(nir_shader *shader);

nir_lower_int64_options nir_lower_int64_op_to_options_mask(nir_op opcode);
bool nir_lower_int64(nir_shader *shader);
bool nir_lower_int64_float_conversions(nir_shader *shader);

nir_lower_doubles_options nir_lower_doubles_op_to_options_mask(nir_op opcode);
bool nir_lower_doubles(nir_shader *shader, const nir_shader *softfp64,
                       nir_lower_doubles_options options);
bool nir_lower_floats(nir_shader *shader, const nir_shader *softfp32);
bool nir_lower_pack(nir_shader *shader);

nir_intrinsic_instr *nir_get_io_intrinsic(nir_instr *instr, nir_variable_mode modes,
                                          nir_variable_mode *out_mode);
bool nir_recompute_io_bases(nir_shader *nir, nir_variable_mode modes);
bool nir_lower_mediump_vars(nir_shader *nir, nir_variable_mode modes);
bool nir_lower_mediump_io(nir_shader *nir, nir_variable_mode modes,
                          uint64_t varying_mask, bool use_16bit_slots);
bool nir_clear_mediump_io_flag(nir_shader *nir);

typedef struct nir_opt_tex_srcs_options {
   unsigned sampler_dims;
   unsigned src_types;
} nir_opt_tex_srcs_options;

typedef struct nir_opt_16bit_tex_image_options {
   nir_rounding_mode rounding_mode;
   nir_alu_type opt_tex_dest_types;
   nir_alu_type opt_image_dest_types;
   bool integer_dest_saturates;
   bool opt_image_store_data;
   bool opt_image_srcs;
   unsigned opt_srcs_options_count;
   nir_opt_tex_srcs_options *opt_srcs_options;
} nir_opt_16bit_tex_image_options;

bool nir_opt_16bit_tex_image(nir_shader *nir,
                             nir_opt_16bit_tex_image_options *options);

typedef struct nir_tex_src_type_constraint {
   bool legalize_type;         /* whether this src should be legalized */
   uint8_t bit_size;           /* bit_size to enforce */
   nir_tex_src_type match_src; /* if bit_size is 0, match bit size of this */
} nir_tex_src_type_constraint, nir_tex_src_type_constraints[nir_num_tex_src_types];

bool nir_legalize_16bit_sampler_srcs(nir_shader *nir,
                                     nir_tex_src_type_constraints constraints);

bool nir_lower_point_size(nir_shader *shader, float min, float max);

bool nir_lower_default_point_size(nir_shader *nir);

bool nir_lower_texcoord_replace(nir_shader *s, unsigned coord_replace,
                                bool point_coord_is_sysval, bool yinvert);

bool nir_lower_texcoord_replace_late(nir_shader *s, unsigned coord_replace,
                                     bool point_coord_is_sysval);

typedef enum {
   nir_lower_interpolation_at_sample = (1 << 1),
   nir_lower_interpolation_at_offset = (1 << 2),
   nir_lower_interpolation_centroid = (1 << 3),
   nir_lower_interpolation_pixel = (1 << 4),
   nir_lower_interpolation_sample = (1 << 5),
} nir_lower_interpolation_options;

bool nir_lower_interpolation(nir_shader *shader,
                             nir_lower_interpolation_options options);

typedef enum {
   nir_lower_demote_if_to_cf = (1 << 0),
   nir_lower_terminate_if_to_cf = (1 << 1),
   nir_move_terminate_out_of_loops = (1 << 2),
} nir_lower_discard_if_options;

bool nir_lower_discard_if(nir_shader *shader, nir_lower_discard_if_options options);

bool nir_lower_terminate_to_demote(nir_shader *nir);

bool nir_lower_memory_model(nir_shader *shader);
bool nir_lower_disordered_control_barriers(nir_shader *shader);

bool nir_lower_goto_ifs(nir_shader *shader);
void nir_simplify_loop(nir_loop *loop, nir_jump_type type);
bool nir_lower_continue_constructs(nir_shader *shader);

typedef struct nir_lower_multiview_options {
   uint32_t view_mask;

   /**
    * Bitfield of output locations that may be converted to a per-view array.
    *
    * If a variable exists in an allowed location, it will be converted to an
    * array even if its value does not depend on the view index.
    */
   uint64_t allowed_per_view_outputs;
} nir_lower_multiview_options;

bool nir_shader_uses_view_index(nir_shader *shader);
bool nir_can_lower_multiview(nir_shader *shader, nir_lower_multiview_options options);
bool nir_lower_multiview(nir_shader *shader, nir_lower_multiview_options options);

bool nir_lower_view_index_to_device_index(nir_shader *shader);

typedef enum {
   nir_lower_fp16_rtz = (1 << 0),
   nir_lower_fp16_rtne = (1 << 1),
   nir_lower_fp16_ru = (1 << 2),
   nir_lower_fp16_rd = (1 << 3),
   nir_lower_fp16_all = 0xf,
   nir_lower_fp16_split_fp64 = (1 << 4),
} nir_lower_fp16_cast_options;
bool nir_lower_fp16_casts(nir_shader *shader, nir_lower_fp16_cast_options options);
bool nir_normalize_cubemap_coords(nir_shader *shader);

bool nir_shader_supports_implicit_lod(nir_shader *shader);

void nir_live_defs_impl(nir_function_impl *impl);

struct u_sparse_bitset *nir_get_live_defs(nir_cursor cursor, void *mem_ctx);

void nir_loop_analyze_impl(nir_function_impl *impl,
                           nir_variable_mode indirect_mask,
                           bool force_unroll_sampler_indirect);

/* This requires both nir_metadata_live_defs and nir_metadata_instr_index. */
bool nir_defs_interfere(nir_def *a, nir_def *b);

bool nir_repair_ssa_impl(nir_function_impl *impl);
bool nir_repair_ssa(nir_shader *shader);

void nir_convert_loop_to_lcssa(nir_loop *loop);
bool nir_convert_to_lcssa(nir_shader *shader, bool skip_invariants, bool skip_bool_invariants);
void nir_divergence_analysis_impl(nir_function_impl *impl, nir_divergence_options options);
void nir_divergence_analysis(nir_shader *shader);
void nir_custom_divergence_analysis(nir_shader *shader, nir_divergence_options options);
bool nir_has_divergent_loop(nir_shader *shader);

void
nir_rewrite_uses_to_load_reg(nir_builder *b, nir_def *old,
                             nir_def *reg);

/* If phi_webs_only is true, only convert SSA values involved in phi nodes to
 * registers.  If false, convert all values (even those not involved in a phi
 * node) to registers.
 * If consider_divergence is true, this pass will use divergence information
 * in order to not coalesce copies from uniform to divergent registers.
 */
bool nir_convert_from_ssa(nir_shader *shader,
                          bool phi_webs_only, bool consider_divergence);

bool nir_lower_phis_to_regs_block(nir_block *block,
                                  bool place_writes_in_imm_preds);
bool nir_lower_ssa_defs_to_regs_block(nir_block *block);

bool nir_rematerialize_deref_in_use_blocks(nir_deref_instr *instr);
bool nir_rematerialize_derefs_in_use_blocks_impl(nir_function_impl *impl);

bool nir_lower_samplers(nir_shader *shader);
bool nir_lower_cl_images(nir_shader *shader, bool lower_image_derefs, bool lower_sampler_derefs);
bool nir_dedup_inline_samplers(nir_shader *shader);

typedef struct nir_lower_ssbo_options {
   bool native_loads;
   bool native_offset;

   /* If non-zero, use @get_ssbo_size to only use global memory accesses for
    * SSBOs larger than this. Keep the existing SSBO accesses for smaller
    * SSBOs. This is useful for HW hat has native SSBO access instructions
    * that only support a limited size to only fall back to global memory for
    * larger buffers.
    */
   uint32_t min_ssbo_size;

   /* Add manual bounds checks for the generated global memory accesses. This
    * is mostly useful in combination with `min_ssbo_size` when the native
    * SSBO access instructions do the bounds check in HW so we don't want to
    * add bounds checks for all SSBO accesses.
    */
   bool bounds_check;
} nir_lower_ssbo_options;

bool nir_lower_ssbo(nir_shader *shader, const nir_lower_ssbo_options *opts);

bool nir_lower_helper_writes(nir_shader *shader, bool lower_plain_stores);

typedef struct nir_lower_printf_options {
   unsigned max_buffer_size;
   unsigned ptr_bit_size;
   bool hash_format_strings;
} nir_lower_printf_options;

bool nir_lower_printf(nir_shader *nir, const nir_lower_printf_options *options);

typedef struct nir_lower_abort_options {
   uint64_t buffer_addr;
   unsigned max_buffer_size;
   unsigned ptr_bit_size;
} nir_lower_abort_options;

bool nir_lower_abort(nir_shader *nir, const nir_lower_abort_options *options);

/* This is here for unit tests. */
bool nir_opt_comparison_pre_impl(nir_function_impl *impl);

bool nir_opt_comparison_pre(nir_shader *shader);

typedef struct nir_opt_access_options {
   bool is_vulkan;
} nir_opt_access_options;

bool nir_opt_access(nir_shader *shader, const nir_opt_access_options *options);
bool nir_opt_algebraic(nir_shader *shader);
bool nir_opt_algebraic_before_ffma(nir_shader *shader);
bool nir_opt_algebraic_before_lower_int64(nir_shader *shader);
bool nir_opt_algebraic_late(nir_shader *shader);
bool nir_opt_algebraic_distribute_src_mods(nir_shader *shader);
bool nir_opt_algebraic_integer_promotion(nir_shader *shader);
bool nir_opt_reassociate_matrix_mul(nir_shader *shader);
bool nir_opt_reassociate_for_fma(nir_shader *shader);
bool nir_opt_constant_folding(nir_shader *shader);

bool nir_opt_fp_math_ctrl(nir_shader *shader);

nir_def *nir_try_constant_fold_alu(nir_builder *b, nir_alu_instr *alu);

/* Try to combine a and b into a.  Return true if combination was possible,
 * which will result in b being removed by the pass.  Return false if
 * combination wasn't possible.
 */
typedef bool (*nir_combine_barrier_cb)(
   nir_intrinsic_instr *a, nir_intrinsic_instr *b, void *data);

bool nir_opt_combine_barriers(nir_shader *shader,
                              nir_combine_barrier_cb combine_cb,
                              void *data);
bool nir_opt_acquire_release_barriers(nir_shader *shader, mesa_scope max_scope);
bool nir_opt_barrier_modes(nir_shader *shader);

bool nir_minimize_call_live_states(nir_shader *shader);

bool nir_opt_combine_stores(nir_shader *shader, nir_variable_mode modes);

bool nir_opt_copy_prop_impl(nir_function_impl *impl);
bool nir_opt_copy_prop(nir_shader *shader);

bool nir_opt_copy_prop_vars(nir_shader *shader);

bool nir_opt_cse(nir_shader *shader);

bool nir_opt_dce_impl(nir_function_impl *impl);
bool nir_opt_dce(nir_shader *shader);

bool nir_opt_dead_cf(nir_shader *shader);

bool nir_opt_dead_write_vars(nir_shader *shader);

bool nir_opt_deref_impl(nir_function_impl *impl);
bool nir_opt_deref(nir_shader *shader);

bool nir_opt_find_array_copies(nir_shader *shader);

bool nir_def_is_frag_coord_z(nir_def *def);
bool nir_opt_fragdepth(nir_shader *shader);

bool nir_opt_gcm(nir_shader *shader, bool value_number,
                 bool hoist_tex_from_loops);

bool nir_opt_generate_bfi(nir_shader *shader);

bool nir_opt_idiv_const(nir_shader *shader, unsigned min_bit_size);

bool nir_opt_mqsad(nir_shader *shader);

typedef enum {
   nir_opt_if_optimize_phi_true_false = (1 << 0),
   nir_opt_if_avoid_64bit_phis = (1 << 1),
} nir_opt_if_options;

bool nir_opt_if(nir_shader *shader, nir_opt_if_options options);

bool nir_opt_intrinsics(nir_shader *shader);

bool nir_opt_large_constants(nir_shader *shader,
                             glsl_type_size_align_func size_align,
                             unsigned threshold);

typedef bool (*nir_opt_licm_filter_cb)(nir_instr *instr, nir_loop *loop,
                                       bool instr_block_dominates_exit);

bool nir_opt_licm(nir_shader *shader,
                  nir_opt_licm_filter_cb filter);
bool nir_opt_loop(nir_shader *shader);

bool nir_opt_loop_unroll(nir_shader *shader);

typedef enum {
   nir_move_const_undef =              BITFIELD_BIT(0),
   nir_move_alu =                      BITFIELD_BIT(1),
   nir_move_copies =                   BITFIELD_BIT(2),
   nir_move_comparisons =              BITFIELD_BIT(3),
   nir_dont_move_byte_word_vecs =      BITFIELD_BIT(4),

   /* Tex opcodes */
   nir_move_tex_sample =               BITFIELD_BIT(8),
   nir_move_tex_load =                 BITFIELD_BIT(9),
   nir_move_tex_load_fragment_mask =   BITFIELD_BIT(10),
   nir_move_tex_lod =                  BITFIELD_BIT(11),
   nir_move_tex_query =                BITFIELD_BIT(12),

   /* Intrinsics */
   nir_move_load_image =               BITFIELD_BIT(13),
   nir_move_load_image_fragment_mask = BITFIELD_BIT(14),
   nir_move_query_image =              BITFIELD_BIT(15),

   nir_move_load_input =               BITFIELD_BIT(16),
   nir_move_load_global =              BITFIELD_BIT(17),
   nir_move_load_ubo =                 BITFIELD_BIT(18),
   nir_move_load_ssbo =                BITFIELD_BIT(19),
   nir_move_load_uniform =             BITFIELD_BIT(20),
   nir_move_load_buffer_amd =          BITFIELD_BIT(21),
   nir_move_load_frag_coord =          BITFIELD_BIT(22),

   /* The following options only impact load_global/ubo/ssbo. */
   nir_move_only_convergent =          BITFIELD_BIT(30),
   nir_move_only_divergent =           BITFIELD_BIT(31),
} nir_move_options;

bool nir_can_move_instr(nir_instr *instr, nir_move_options options);

bool nir_opt_sink(nir_shader *shader, nir_move_options options);

bool nir_opt_move(nir_shader *shader, nir_move_options options);

typedef struct nir_opt_offsets_options {
   /** nir_load_uniform max base offset */
   uint32_t uniform_max;

   /** nir_load_ubo_vec4 max base offset */
   uint32_t ubo_vec4_max;

   /** nir_var_mem_shared max base offset */
   uint32_t shared_max;

   /** nir_var_mem_shared atomic max base offset */
   uint32_t shared_atomic_max;

   /** nir_load/store_buffer_amd max base offset */
   uint32_t buffer_max;

   /** nir_load/store_global_offset max base offset */
   uint32_t global_max;

   /**
    * Callback to get the max base offset for instructions for which the
    * corresponding value above is zero.
    */
   uint32_t (*max_offset_cb)(nir_intrinsic_instr *intr, const void *data);

   /** Data to pass to callbacks. */
   const void *cb_data;

   /**
    * Allow the offset calculation to wrap. If false, constant additions that
    * might wrap will not be folded into the offset.
    */
   bool (*allow_offset_wrap_cb)(nir_intrinsic_instr *intr, const void *data);
} nir_opt_offsets_options;

bool nir_opt_offsets(nir_shader *shader, const nir_opt_offsets_options *options);

typedef struct nir_opt_peephole_select_options {
   unsigned limit; /* Set to max to flatten all control flow. */
   bool indirect_load_ok;
   bool expensive_alu_ok;
   bool discard_ok;
} nir_opt_peephole_select_options;

bool nir_opt_peephole_select(nir_shader *shader,
                             const nir_opt_peephole_select_options *options);

typedef enum {
   /* Enable the global CSE heuristic */
   nir_reassociate_cse_heuristic = (1 << 0),

   /* Indicate the backend can accelerate scalar (i.e. non-divergent) math. If
    * set, we try to reassociate expressions like ((div + con) + con) to be
    * (div + (con + con)) to save a vector ALU operation. We're fairly
    * aggressive about it: in some circumstances, we will spend 2 scalar
    * instructions to save 1 vector instruction.
    *
    * This is true for backends using nir_opt_preamble even if there's no scalar
    * ALU, since non-divergence is a decent proxy for uniformity.
    */
   nir_reassociate_scalar_math = (1 << 1),
} nir_reassociate_options;

bool nir_opt_reassociate(nir_shader *shader, nir_reassociate_options opts);

bool nir_opt_reassociate_loop(nir_shader *shader, nir_reassociate_options opts);

bool nir_opt_reassociate_bfi(nir_shader *shader);

bool nir_opt_rematerialize_compares(nir_shader *shader);

bool nir_opt_remove_phis(nir_shader *shader);
bool nir_remove_single_src_phis_block(nir_block *block);

bool nir_opt_phi_precision(nir_shader *shader);

bool nir_opt_phi_to_bool(nir_shader *shader);

bool nir_opt_shrink_stores(nir_shader *shader, bool shrink_image_store);

bool nir_opt_shrink_vectors(nir_shader *shader, bool shrink_start);

bool nir_opt_undef(nir_shader *shader);

typedef bool (*nir_lower_undef_to_zero_filter)(nir_undef_instr *);
bool nir_lower_undef_to_zero(nir_shader *shader,
                             nir_lower_undef_to_zero_filter filter);

bool nir_opt_uniform_atomics(nir_shader *shader, bool fs_atomics_predicated);

bool nir_opt_uniform_subgroup(nir_shader *shader,
                              const nir_lower_subgroups_options *);

bool nir_opt_shared_vars_to_subgroup(nir_shader *shader,
                                     unsigned ballot_num_components,
                                     unsigned ballot_size);

bool nir_opt_vectorize(nir_shader *shader, nir_vectorize_cb filter,
                       void *data);
bool nir_opt_vectorize_io(nir_shader *shader, nir_variable_mode modes,
                          bool allow_holes);

bool nir_opt_move_discards_to_top(nir_shader *shader);

bool nir_opt_ray_queries(nir_shader *shader);

bool nir_opt_ray_query_ranges(nir_shader *shader);

typedef struct {
   /* Optimize imul to muls with smaller bit sizes if possible. Only set this
    * after optimizations (e.g., nir_opt_load_store_vectorize) that want to
    * analyze imuls have run.
    */
   bool opt_imul;
} nir_opt_uub_options;

bool nir_opt_uub(nir_shader *shader, const nir_opt_uub_options *options);

typedef bool (*nir_skip_helpers_instrinsic_cb)(
   nir_intrinsic_instr *intr, void *data);

typedef struct nir_opt_load_skip_helpers_options {
   bool no_add_divergence : 1;
   nir_skip_helpers_instrinsic_cb intrinsic_cb;
   void *intrinsic_cb_data;
} nir_opt_load_skip_helpers_options;

bool nir_opt_load_skip_helpers(nir_shader *shader, nir_opt_load_skip_helpers_options *options);

void nir_sweep(nir_shader *shader);

nir_intrinsic_op nir_intrinsic_from_system_value(gl_system_value val);
gl_system_value nir_system_value_from_intrinsic(nir_intrinsic_op intrin);
gl_system_value nir_system_value_from_instr(nir_instr *instr);

static inline bool
nir_variable_is_in_ubo(const nir_variable *var)
{
   return (var->data.mode == nir_var_mem_ubo &&
           var->interface_type != NULL);
}

static inline bool
nir_variable_is_in_ssbo(const nir_variable *var)
{
   return (var->data.mode == nir_var_mem_ssbo &&
           var->interface_type != NULL);
}

static inline bool
nir_variable_is_in_block(const nir_variable *var)
{
   return nir_variable_is_in_ubo(var) || nir_variable_is_in_ssbo(var);
}

static inline unsigned
nir_variable_count_slots(const nir_variable *var, const struct glsl_type *type)
{
   return var->data.compact ? DIV_ROUND_UP(var->data.location_frac + glsl_get_length(type), 4) : glsl_count_attribute_slots(type, false);
}

static inline unsigned
nir_deref_count_slots(nir_deref_instr *deref, nir_variable *var)
{
   if (var->data.compact) {
      switch (deref->deref_type) {
      case nir_deref_type_array:
         return 1;
      case nir_deref_type_var:
         return nir_variable_count_slots(var, deref->type);
      default:
         UNREACHABLE("illegal deref type");
      }
   }
   return glsl_count_attribute_slots(deref->type, false);
}
uint32_t
nir_unsigned_upper_bound(nir_shader *shader, struct hash_table *range_ht,
                         nir_scalar scalar);

bool
nir_addition_might_overflow(nir_shader *shader, struct hash_table *range_ht,
                            nir_scalar ssa, unsigned const_val);

typedef struct nir_opt_preamble_options {
   /* True if gl_DrawID is considered uniform, i.e. if the preamble is run
    * at least once per "internal" draw rather than per user-visible draw.
    */
   bool drawid_uniform;

   /* True if the subgroup size is uniform. */
   bool subgroup_size_uniform;

   /* True if load_workgroup_size is supported in the preamble. */
   bool load_workgroup_size_allowed;

   /* size/align/class for load/store_preamble.
    *
    * Defs with class "general" will always be allocated as general. Other
    * classes will attempt to allocate as the specialized class but may fallback
    * to general. This mechanism enables "tiered" classes in a single
    * nir_opt_preamble call with proper global behaviour.
    */
   void (*def_size)(nir_def *def, unsigned *size, unsigned *align,
                    nir_preamble_class *class_);

   /* Total available size per class for load/store_preamble storage, in units
    * determined by def_size.
    */
   unsigned preamble_storage_size[nir_preamble_num_classes];

   /* Give the cost for an instruction. nir_opt_preamble will prioritize
    * instructions with higher costs. Instructions with cost 0 may still be
    * lifted, but only when required to lift other instructions with non-0
    * cost (e.g. a load_const source of an expression).
    */
   float (*instr_cost_cb)(nir_instr *instr, const void *data);

   /* Give the cost of rewriting the instruction to use load_preamble. This
    * may happen from inserting move instructions, etc. If the benefit doesn't
    * exceed the cost here then we won't rewrite it.
    */
   float (*rewrite_cost_cb)(nir_def *def, const void *data);

   /* Instructions whose definitions should not be rewritten. These could
    * still be moved to the preamble, but they shouldn't be the root of a
    * replacement expression. Instructions with cost 0 and derefs are
    * automatically included by the pass.
    */
   nir_instr_filter_cb avoid_instr_cb;

   const void *cb_data;
} nir_opt_preamble_options;

bool
nir_opt_preamble(nir_shader *shader,
                 const nir_opt_preamble_options *options,
                 unsigned *size);

nir_function_impl *nir_shader_get_preamble(nir_shader *shader);

bool nir_lower_point_smooth(nir_shader *shader, bool set_barycentrics);
bool nir_lower_poly_line_smooth(nir_shader *shader, unsigned num_smooth_aa_sample);

bool nir_mod_analysis(nir_scalar val, nir_alu_type val_type, unsigned div, unsigned *mod);

bool
nir_remove_tex_shadow(nir_shader *shader, unsigned textures_bitmask);

bool
nir_trivialize_registers(nir_shader *s);

unsigned
nir_static_workgroup_size(const nir_shader *s);

static inline nir_intrinsic_instr *
nir_reg_get_decl(nir_def *reg)
{
   assert(nir_def_is_intrinsic(reg));
   nir_intrinsic_instr *decl = nir_def_as_intrinsic(reg);
   assert(decl->intrinsic == nir_intrinsic_decl_reg);

   return decl;
}

static inline nir_intrinsic_instr *
nir_next_decl_reg(nir_intrinsic_instr *prev, nir_function_impl *impl)
{
   nir_instr *start;
   if (prev != NULL)
      start = nir_instr_next(&prev->instr);
   else if (impl != NULL)
      start = nir_block_first_instr(nir_start_block(impl));
   else
      return NULL;

   for (nir_instr *instr = start; instr; instr = nir_instr_next(instr)) {
      if (instr->type != nir_instr_type_intrinsic)
         continue;

      nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
      if (intrin->intrinsic == nir_intrinsic_decl_reg)
         return intrin;
   }

   return NULL;
}

#define nir_foreach_reg_decl(reg, impl)                           \
   for (nir_intrinsic_instr *reg = nir_next_decl_reg(NULL, impl); \
        reg; reg = nir_next_decl_reg(reg, NULL))

#define nir_foreach_reg_decl_safe(reg, impl)                       \
   for (nir_intrinsic_instr *reg = nir_next_decl_reg(NULL, impl),  \
                            *next_ = nir_next_decl_reg(reg, NULL); \
        reg; reg = next_, next_ = nir_next_decl_reg(next_, NULL))

static inline nir_cursor
nir_after_reg_decls(nir_function_impl *impl)
{
   nir_intrinsic_instr *last_reg_decl = NULL;
   nir_foreach_reg_decl(reg_decl, impl)
      last_reg_decl = reg_decl;

   if (last_reg_decl != NULL)
      return nir_after_instr(&last_reg_decl->instr);
   return nir_before_impl(impl);
}

static inline bool
nir_is_load_reg(nir_intrinsic_instr *intr)
{
   return intr->intrinsic == nir_intrinsic_load_reg ||
          intr->intrinsic == nir_intrinsic_load_reg_indirect;
}

static inline bool
nir_is_store_reg(nir_intrinsic_instr *intr)
{
   return intr->intrinsic == nir_intrinsic_store_reg ||
          intr->intrinsic == nir_intrinsic_store_reg_indirect;
}

#define nir_foreach_reg_load(load, reg)              \
   assert(reg->intrinsic == nir_intrinsic_decl_reg); \
                                                     \
   nir_foreach_use(load, &reg->def)                  \
      if (nir_is_load_reg(nir_instr_as_intrinsic(nir_src_use_instr(load))))

#define nir_foreach_reg_load_safe(load, reg)         \
   assert(reg->intrinsic == nir_intrinsic_decl_reg); \
                                                     \
   nir_foreach_use_safe(load, &reg->def)             \
      if (nir_is_load_reg(nir_instr_as_intrinsic(nir_src_use_instr(load))))

#define nir_foreach_reg_store(store, reg)            \
   assert(reg->intrinsic == nir_intrinsic_decl_reg); \
                                                     \
   nir_foreach_use(store, &reg->def)                 \
      if (nir_is_store_reg(nir_instr_as_intrinsic(nir_src_use_instr(store))))

#define nir_foreach_reg_store_safe(store, reg)       \
   assert(reg->intrinsic == nir_intrinsic_decl_reg); \
                                                     \
   nir_foreach_use_safe(store, &reg->def)            \
      if (nir_is_store_reg(nir_instr_as_intrinsic(nir_src_use_instr(store))))

static inline nir_intrinsic_instr *
nir_load_reg_for_def(const nir_def *def)
{
   if (!nir_def_is_intrinsic(def))
      return NULL;

   nir_intrinsic_instr *intr = nir_def_as_intrinsic(def);
   if (!nir_is_load_reg(intr))
      return NULL;

   return intr;
}

static inline nir_intrinsic_instr *
nir_store_reg_for_def(const nir_def *def)
{
   /* Look for the trivial store: single use of our destination by a
    * store_register intrinsic.
    */
   if (!list_is_singular(&def->uses))
      return NULL;

   nir_src *src = list_first_entry(&def->uses, nir_src, use_link);
   if (nir_src_is_if(src))
      return NULL;

   nir_instr *parent = nir_src_use_instr(src);
   if (parent->type != nir_instr_type_intrinsic)
      return NULL;

   nir_intrinsic_instr *intr = nir_instr_as_intrinsic(parent);
   if (!nir_is_store_reg(intr))
      return NULL;

   /* The first value is data. Third is indirect index, ignore that one. */
   if (&intr->src[0] != src)
      return NULL;

   return intr;
}

typedef struct nir_use_dominance_state nir_use_dominance_state;

nir_use_dominance_state *
nir_calc_use_dominance_impl(nir_function_impl *impl, bool post_dominance);

nir_instr *
nir_get_immediate_use_dominator(nir_use_dominance_state *state,
                                nir_instr *instr);
nir_instr *nir_use_dominance_lca(nir_use_dominance_state *state,
                                 nir_instr *i1, nir_instr *i2);
bool nir_instr_dominates_use(nir_use_dominance_state *state,
                             nir_instr *parent, nir_instr *child);
void nir_print_use_dominators(nir_use_dominance_state *state,
                              nir_instr **instructions,
                              unsigned num_instructions);

static inline unsigned
nir_verts_in_output_prim(nir_shader *gs)
{
   assert(gs->info.stage == MESA_SHADER_GEOMETRY);
   return mesa_vertices_per_prim(gs->info.gs.output_primitive);
}

typedef struct {
   struct {
      /* The list of instructions that affect this output including the output
       * store itself. If NULL, the output isn't stored.
       */
      nir_instr **instr_list;
      unsigned num_instr;
   } output[NUM_TOTAL_VARYING_SLOTS];
} nir_output_deps;

void nir_gather_output_dependencies(nir_shader *nir, nir_output_deps *deps);
void nir_free_output_dependencies(nir_output_deps *deps);

typedef struct {
   struct {
      /* Per component mask of input slots. */
      BITSET_DECLARE(inputs, NUM_TOTAL_VARYING_SLOTS * 8);
      bool defined;
      bool uses_ssbo_reads;
      bool uses_image_reads;
   } output[NUM_TOTAL_VARYING_SLOTS];
} nir_input_to_output_deps;

void nir_gather_input_to_output_dependencies(nir_shader *nir,
                                             nir_input_to_output_deps *out_deps);
void nir_print_input_to_output_deps(nir_input_to_output_deps *deps,
                                    nir_shader *nir, FILE *f);

typedef struct {
   /* 1 bit per 16-bit component. */
   BITSET_DECLARE(pos_only, NUM_TOTAL_VARYING_SLOTS * 8);
   BITSET_DECLARE(var_only, NUM_TOTAL_VARYING_SLOTS * 8);
   BITSET_DECLARE(both, NUM_TOTAL_VARYING_SLOTS * 8);
} nir_output_clipper_var_groups;

void nir_gather_output_clipper_var_groups(nir_shader *nir,
                                          nir_output_clipper_var_groups *groups);

bool nir_lower_cooperative_matrix_flexible_dimensions(nir_shader *shader, unsigned m_gran, unsigned n_gran, unsigned k_gran);

bool nir_unlower_io_to_vars(nir_shader *nir, bool keep_intrinsics);

bool nir_opt_barycentric(nir_shader *shader, bool lower_sample_to_pos);

bool nir_normalize_sin_cos(nir_shader *shader);

#include "nir_inline_helpers.h"

static inline bool
nir_is_io_compact(nir_shader *nir, bool is_output, unsigned location)
{
   return nir->options->compact_arrays &&
          (nir->info.stage != MESA_SHADER_VERTEX || is_output) &&
          (nir->info.stage != MESA_SHADER_FRAGMENT || !is_output) &&
          (location == VARYING_SLOT_CLIP_DIST0 ||
           location == VARYING_SLOT_CLIP_DIST1 ||
           location == VARYING_SLOT_CULL_DIST0 ||
           location == VARYING_SLOT_CULL_DIST1 ||
           (nir->info.stage != MESA_SHADER_MESH && location == VARYING_SLOT_TESS_LEVEL_OUTER) ||
           (nir->info.stage != MESA_SHADER_MESH && location == VARYING_SLOT_TESS_LEVEL_INNER));
}

static inline nir_float_muladd_support
nir_float_muladd_for_bitsize(const nir_shader *nir, unsigned bit_size)
{
   switch (bit_size) {
   case 16:
      return nir->options->float_mul_add16;
   case 32:
      return nir->options->float_mul_add32;
   case 64:
      return nir->options->float_mul_add64;
   default:
      UNREACHABLE("unsupported bit_size");
      return (nir_float_muladd_support)0;
   }
}

static inline bool
nir_has_ffma(const nir_shader *nir, unsigned bit_size)
{
   nir_float_muladd_support muladd = nir_float_muladd_for_bitsize(nir, bit_size);
   return (muladd & nir_float_muladd_support_has_ffma) != 0;
}

static inline bool
nir_prefers_fmad(const nir_shader *nir, unsigned bit_size)
{
   nir_float_muladd_support muladd = nir_float_muladd_for_bitsize(nir, bit_size);
   return  (muladd & nir_float_muladd_support_prefers_split) != 0 ||
           (muladd & nir_float_muladd_support_has_ffma) == 0;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NIR_H */

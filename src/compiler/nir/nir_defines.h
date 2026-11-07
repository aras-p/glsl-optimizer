/*
 * Copyright © 2014 Connor Abbott
 * SPDX-License-Identifier: MIT
 */

/**
 * @file nir_defines.h
 *
 * This file contains forward declarations and basic type definitions for NIR.
 * it is meant to be used when the full NIR header is not necessary, such as
 * in parts that work with NIR, but don't need the full implementation details
 * of the various NIR structures.
 *
 * The contents of this file should be kept simple enough that we should be able
 * to also include it in OpenCL programs.
 */

#ifndef NIR_DEFINES_H
#define NIR_DEFINES_H

#include <stdbool.h>
#include <stdint.h>
#include "util/macros.h"
#include "util/enum_operators.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct shader_info shader_info;

typedef struct nir_shader nir_shader;
typedef struct nir_shader_compiler_options nir_shader_compiler_options;
typedef struct nir_builder nir_builder;
typedef struct nir_def nir_def;
typedef struct nir_variable nir_variable;

typedef struct nir_cf_node nir_cf_node;
typedef struct nir_block nir_block;
typedef struct nir_if nir_if;
typedef struct nir_loop nir_loop;
typedef struct nir_function nir_function;
typedef struct nir_function_impl nir_function_impl;

typedef struct nir_instr nir_instr;
typedef struct nir_alu_instr nir_alu_instr;
typedef struct nir_deref_instr nir_deref_instr;
typedef struct nir_call_instr nir_call_instr;
typedef struct nir_jump_instr nir_jump_instr;
typedef struct nir_tex_instr nir_tex_instr;
typedef struct nir_intrinsic_instr nir_intrinsic_instr;
typedef struct nir_load_const_instr nir_load_const_instr;
typedef struct nir_undef_instr nir_undef_instr;
typedef struct nir_phi_instr nir_phi_instr;

typedef struct nir_xfb_info nir_xfb_info;
typedef struct nir_tcs_info nir_tcs_info;

/* clang-format off */
typedef enum {
   nir_var_system_value          = (1 << 0),
   nir_var_uniform               = (1 << 1),
   nir_var_shader_in             = (1 << 2),
   nir_var_shader_out            = (1 << 3),
   nir_var_image                 = (1 << 4),
   /** Incoming call or ray payload data for ray-tracing shaders */
   nir_var_shader_call_data      = (1 << 5),
   /** Ray hit attributes */
   nir_var_ray_hit_attrib        = (1 << 6),

   /* Modes named nir_var_mem_* have explicit data layout */
   nir_var_mem_ubo               = (1 << 7),
   nir_var_mem_push_const        = (1 << 8),
   nir_var_mem_ssbo              = (1 << 9),
   nir_var_mem_constant          = (1 << 10),
   nir_var_mem_task_payload      = (1 << 11),
   nir_var_mem_node_payload      = (1 << 12),
   nir_var_mem_node_payload_in   = (1 << 13),
   nir_var_mem_pixel_local_in    = (1 << 14),
   nir_var_mem_pixel_local_out   = (1 << 15),
   nir_var_mem_pixel_local_inout = (1 << 16),

   nir_var_function_in           = (1 << 17),
   nir_var_function_out          = (1 << 18),
   nir_var_function_inout        = (1 << 19),

   nir_var_resource_heap         = (1 << 20),
   nir_var_sampler_heap          = (1 << 21),

   /* Generic modes intentionally come last. See encode_dref_modes() in
    * nir_serialize.c for more details.
    */
   nir_var_shader_temp           = (1 << 22),
   nir_var_function_temp         = (1 << 23),
   nir_var_mem_shared            = (1 << 24),
   nir_var_mem_global            = (1 << 25),

   nir_var_mem_generic           = (nir_var_shader_temp |
                                    nir_var_function_temp |
                                    nir_var_mem_shared |
                                    nir_var_mem_global),

   nir_var_read_only_modes       = nir_var_shader_in | nir_var_uniform |
                                   nir_var_system_value | nir_var_mem_constant |
                                   nir_var_mem_ubo,
   /* Modes where vector derefs can be indexed as arrays. nir_var_shader_out
    * is only for mesh stages. nir_var_system_value is only for kernel stages.
    */
   nir_var_vec_indexable_modes   = nir_var_shader_temp | nir_var_function_temp |
                                 nir_var_mem_ubo | nir_var_mem_ssbo |
                                 nir_var_mem_shared | nir_var_mem_global |
                                 nir_var_mem_push_const | nir_var_mem_task_payload |
                                 nir_var_shader_out | nir_var_system_value,

   /* modes that can serve as fragment shader outputs */
   nir_var_any_pixel_local   = nir_var_mem_pixel_local_in |
                               nir_var_mem_pixel_local_out |
                               nir_var_mem_pixel_local_inout,

   nir_num_variable_modes        = 26,
   nir_var_all                   = (1 << nir_num_variable_modes) - 1,
} nir_variable_mode;
MESA_DEFINE_CPP_ENUM_BITFIELD_OPERATORS(nir_variable_mode)
/* clang-format on */

/** NIR sized and unsized types
 *
 * The values in this enum are carefully chosen so that the sized type is
 * just the unsized type OR the number of bits.
 */
/* clang-format off */
typedef enum ENUM_PACKED {
   nir_type_invalid =   0, /* Not a valid type */
   nir_type_int =       2,
   nir_type_uint =      4,
   nir_type_bool =      6,
   nir_type_float =     128,
   nir_type_bool1 =     1  | nir_type_bool,
   nir_type_bool32 =    32 | nir_type_bool,
   nir_type_int1 =      1  | nir_type_int,
   nir_type_int8 =      8  | nir_type_int,
   nir_type_int16 =     16 | nir_type_int,
   nir_type_int32 =     32 | nir_type_int,
   nir_type_int64 =     64 | nir_type_int,
   nir_type_uint1 =     1  | nir_type_uint,
   nir_type_uint8 =     8  | nir_type_uint,
   nir_type_uint16 =    16 | nir_type_uint,
   nir_type_uint32 =    32 | nir_type_uint,
   nir_type_uint64 =    64 | nir_type_uint,
   nir_type_float16 =   16 | nir_type_float,
   nir_type_float32 =   32 | nir_type_float,
   nir_type_float64 =   64 | nir_type_float,
} nir_alu_type;
/* clang-format on */

typedef enum {
   /**
    * An address format which is a simple 32-bit global GPU address.
    */
   nir_address_format_32bit_global,

   /**
    * An address format which is a simple 64-bit global GPU address.
    */
   nir_address_format_64bit_global,

   /**
    * An address format which is a 64-bit global GPU address encoded as a
    * 2x32-bit vector.
    */
   nir_address_format_2x32bit_global,

   /**
    * An address format which is a 64-bit global base address and a 32-bit
    * offset.
    *
    * This is identical to 64bit_bounded_global except that bounds checking
    * is not applied when lowering to global access.  Even though the size is
    * never used for an actual bounds check, it needs to be valid so we can
    * lower deref_buffer_array_length properly.
    */
   nir_address_format_64bit_global_32bit_offset,

   /**
    * An address format which is a bounds-checked 64-bit global GPU address.
    *
    * The address is comprised as a 32-bit vec4 where .xy are a uint64_t base
    * address stored with the low bits in .x and high bits in .y, .z is a
    * size, and .w is an offset.  When the final I/O operation is lowered, .w
    * is checked against .z and the operation is predicated on the result.
    */
   nir_address_format_64bit_bounded_global,

   /**
    * An address format which is comprised of a vec2 where the first
    * component is a buffer index and the second is an offset.
    */
   nir_address_format_32bit_index_offset,

   /**
    * An address format which is a 64-bit value, where the high 32 bits
    * are a buffer index, and the low 32 bits are an offset.
    */
   nir_address_format_32bit_index_offset_pack64,

   /**
    * An address format which is comprised of a vec3 where the first two
    * components specify the buffer and the third is an offset.
    */
   nir_address_format_vec2_index_32bit_offset,

   /**
    * An address format which represents generic pointers with a 62-bit
    * pointer and a 2-bit enum in the top two bits.  The top two bits have
    * the following meanings:
    *
    *  - 0x0: Global memory
    *  - 0x1: Shared memory
    *  - 0x2: Scratch memory
    *  - 0x3: Global memory
    *
    * The redundancy between 0x0 and 0x3 is because of Intel sign-extension of
    * addresses.  Valid global memory addresses may naturally have either 0 or
    * ~0 as their high bits.
    *
    * Shared and scratch pointers are represented as 32-bit offsets with the
    * top 32 bits only being used for the enum.  This allows us to avoid
    * 64-bit address calculations in a bunch of cases.
    */
   nir_address_format_62bit_generic,

   /**
    * An address format which is a simple 32-bit offset.
    */
   nir_address_format_32bit_offset,

   /**
    * An address format which is a simple 32-bit offset cast to 64-bit.
    */
   nir_address_format_32bit_offset_as_64bit,

   /**
    * An address format representing a purely logical addressing model.  In
    * this model, all deref chains must be complete from the dereference
    * operation to the variable.  Cast derefs are not allowed.  These
    * addresses will be 32-bit scalars but the format is immaterial because
    * you can always chase the chain.
    */
   nir_address_format_logical,
} nir_address_format;

typedef union {
   bool b;
   float f32;
   double f64;
   int8_t i8;
   uint8_t u8;
   int16_t i16;
   uint16_t u16;
   int32_t i32;
   uint32_t u32;
   int64_t i64;
   uint64_t u64;
} nir_const_value;

#define NIR_ALU_TYPE_SIZE_MASK      0x79
#define NIR_ALU_TYPE_BASE_TYPE_MASK 0x86

static inline bool
nir_num_components_valid(unsigned num_components)
{
   return (num_components >= 1 &&
           num_components <= 5) ||
          num_components == 8 ||
          num_components == 16;
}

static inline unsigned
nir_alu_type_get_type_size(nir_alu_type type)
{
   return type & NIR_ALU_TYPE_SIZE_MASK;
}

static inline nir_alu_type
nir_alu_type_get_base_type(nir_alu_type type)
{
   return (nir_alu_type)(type & NIR_ALU_TYPE_BASE_TYPE_MASK);
}

#ifdef __cplusplus
}
#endif

#endif

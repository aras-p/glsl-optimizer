
#include "nir.h"

nir_op
nir_type_conversion_op(nir_alu_type src, nir_alu_type dst, nir_rounding_mode rnd)
{
   nir_alu_type src_base = (nir_alu_type) nir_alu_type_get_base_type(src);
   nir_alu_type dst_base = (nir_alu_type) nir_alu_type_get_base_type(dst);
   unsigned src_bit_size = nir_alu_type_get_type_size(src);
   unsigned dst_bit_size = nir_alu_type_get_type_size(dst);

   if (src == dst && src_base == nir_type_float) {
      return nir_op_mov;
   } else if (src == dst && src_base == nir_type_bool) {
      return nir_op_mov;
   } else if ((src_base == nir_type_int || src_base == nir_type_uint) &&
              (dst_base == nir_type_int || dst_base == nir_type_uint) &&
              src_bit_size == dst_bit_size) {
      /* Integer <-> integer conversions with the same bit-size on both
       * ends are just no-op moves.
       */
      return nir_op_mov;
   }

   /* f2b, i2b, and u2b do not exist.  Use ine or fne (via nir_type_conversion)
    * instead.
    */
   assert(src_base == dst_base || dst_base != nir_type_bool);

   switch (src_base) {
      case nir_type_int:
         switch (dst_base) {
            case nir_type_int:
            case nir_type_uint:

               switch (dst_bit_size) {
                  case 1:
                     assert(rnd == nir_rounding_mode_undef);
                     return nir_op_i2i1;
                  case 8:
                     assert(rnd == nir_rounding_mode_undef);
                     return nir_op_i2i8;
                  case 16:
                     assert(rnd == nir_rounding_mode_undef);
                     return nir_op_i2i16;
                  case 32:
                     assert(rnd == nir_rounding_mode_undef);
                     return nir_op_i2i32;
                  case 64:
                     assert(rnd == nir_rounding_mode_undef);
                     return nir_op_i2i64;
                  default:
                     UNREACHABLE("Invalid nir alu bit size");
               }
            case nir_type_float:
               switch (dst_bit_size) {
                  case 16:
                     assert(rnd == nir_rounding_mode_undef);
                     return nir_op_i2f16;
                  case 32:
                     assert(rnd == nir_rounding_mode_undef);
                     return nir_op_i2f32;
                  case 64:
                     assert(rnd == nir_rounding_mode_undef);
                     return nir_op_i2f64;
                  default:
                     UNREACHABLE("Invalid nir alu bit size");
               }
            case nir_type_bool:
            default:
               UNREACHABLE("Invalid nir alu base type");
         }
      case nir_type_uint:
         switch (dst_base) {
            case nir_type_int:
            case nir_type_uint:

               switch (dst_bit_size) {
                  case 1:
                     assert(rnd == nir_rounding_mode_undef);
                     return nir_op_u2u1;
                  case 8:
                     assert(rnd == nir_rounding_mode_undef);
                     return nir_op_u2u8;
                  case 16:
                     assert(rnd == nir_rounding_mode_undef);
                     return nir_op_u2u16;
                  case 32:
                     assert(rnd == nir_rounding_mode_undef);
                     return nir_op_u2u32;
                  case 64:
                     assert(rnd == nir_rounding_mode_undef);
                     return nir_op_u2u64;
                  default:
                     UNREACHABLE("Invalid nir alu bit size");
               }
            case nir_type_float:
               switch (dst_bit_size) {
                  case 16:
                     assert(rnd == nir_rounding_mode_undef);
                     return nir_op_u2f16;
                  case 32:
                     assert(rnd == nir_rounding_mode_undef);
                     return nir_op_u2f32;
                  case 64:
                     assert(rnd == nir_rounding_mode_undef);
                     return nir_op_u2f64;
                  default:
                     UNREACHABLE("Invalid nir alu bit size");
               }
            case nir_type_bool:
            default:
               UNREACHABLE("Invalid nir alu base type");
         }
      case nir_type_float:
         switch (dst_base) {
            case nir_type_int:
               switch (dst_bit_size) {
                  case 1:
                     assert(rnd == nir_rounding_mode_undef);
                     return nir_op_f2i1;
                  case 8:
                     assert(rnd == nir_rounding_mode_undef);
                     return nir_op_f2i8;
                  case 16:
                     assert(rnd == nir_rounding_mode_undef);
                     return nir_op_f2i16;
                  case 32:
                     assert(rnd == nir_rounding_mode_undef);
                     return nir_op_f2i32;
                  case 64:
                     assert(rnd == nir_rounding_mode_undef);
                     return nir_op_f2i64;
                  default:
                     UNREACHABLE("Invalid nir alu bit size");
               }
            case nir_type_uint:
               switch (dst_bit_size) {
                  case 1:
                     assert(rnd == nir_rounding_mode_undef);
                     return nir_op_f2u1;
                  case 8:
                     assert(rnd == nir_rounding_mode_undef);
                     return nir_op_f2u8;
                  case 16:
                     assert(rnd == nir_rounding_mode_undef);
                     return nir_op_f2u16;
                  case 32:
                     assert(rnd == nir_rounding_mode_undef);
                     return nir_op_f2u32;
                  case 64:
                     assert(rnd == nir_rounding_mode_undef);
                     return nir_op_f2u64;
                  default:
                     UNREACHABLE("Invalid nir alu bit size");
               }
            case nir_type_float:
               switch (dst_bit_size) {
                  case 16:
                     switch(rnd) {
                        case nir_rounding_mode_rtne:
                           return nir_op_f2f16_rtne;
                        case nir_rounding_mode_rtz:
                           return nir_op_f2f16_rtz;
                        case nir_rounding_mode_undef:
                           return nir_op_f2f16;
                        default:
                           UNREACHABLE("Invalid 16-bit nir rounding mode");
                     }
                  case 32:
                     assert(rnd == nir_rounding_mode_undef);
                     return nir_op_f2f32;
                  case 64:
                     assert(rnd == nir_rounding_mode_undef);
                     return nir_op_f2f64;
                  default:
                     UNREACHABLE("Invalid nir alu bit size");
               }
            case nir_type_bool:
            default:
               UNREACHABLE("Invalid nir alu base type");
         }
      case nir_type_bool:
         switch (dst_base) {
            case nir_type_int:
            case nir_type_uint:

               switch (dst_bit_size) {
                  case 1:
                     assert(rnd == nir_rounding_mode_undef);
                     return nir_op_b2i1;
                  case 8:
                     assert(rnd == nir_rounding_mode_undef);
                     return nir_op_b2i8;
                  case 16:
                     assert(rnd == nir_rounding_mode_undef);
                     return nir_op_b2i16;
                  case 32:
                     assert(rnd == nir_rounding_mode_undef);
                     return nir_op_b2i32;
                  case 64:
                     assert(rnd == nir_rounding_mode_undef);
                     return nir_op_b2i64;
                  default:
                     UNREACHABLE("Invalid nir alu bit size");
               }
            case nir_type_float:
               switch (dst_bit_size) {
                  case 16:
                     assert(rnd == nir_rounding_mode_undef);
                     return nir_op_b2f16;
                  case 32:
                     assert(rnd == nir_rounding_mode_undef);
                     return nir_op_b2f32;
                  case 64:
                     assert(rnd == nir_rounding_mode_undef);
                     return nir_op_b2f64;
                  default:
                     UNREACHABLE("Invalid nir alu bit size");
               }
            case nir_type_bool:
               switch (dst_bit_size) {
                  case 1:
                     assert(rnd == nir_rounding_mode_undef);
                     return nir_op_b2b1;
                  case 32:
                     assert(rnd == nir_rounding_mode_undef);
                     return nir_op_b2b32;
                  default:
                     UNREACHABLE("Invalid nir alu bit size");
               }
            default:
               UNREACHABLE("Invalid nir alu base type");
         }
      default:
         UNREACHABLE("Invalid nir alu base type");
   }
}

const nir_op_info nir_op_infos[nir_num_opcodes] = {
{
   .name = "alignbyte_amd",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_uint32,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint32, nir_type_uint32, nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "amul",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_int,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE | NIR_OP_IS_ASSOCIATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "andg_ir3",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "arshift_and_pan",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_int,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_int, nir_type_uint8, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "arshift_or_pan",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_int,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_int, nir_type_uint8, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "arshift_xor_pan",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_int,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_int, nir_type_uint8, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "b2b1",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_bool1,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_bool
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "b2b32",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_bool32,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_bool
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "b2f16",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float16,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_bool
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero
},
{
   .name = "b2f32",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float32,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_bool
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero
},
{
   .name = "b2f64",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float64,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_bool
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero
},
{
   .name = "b2i1",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_int1,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_bool
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "b2i16",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_int16,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_bool
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "b2i32",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_int32,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_bool
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "b2i64",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_int64,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_bool
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "b2i8",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_int8,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_bool
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "b32all_fequal16",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool32,
   .input_sizes = {
      16, 16
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "b32all_fequal2",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool32,
   .input_sizes = {
      2, 2
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "b32all_fequal3",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool32,
   .input_sizes = {
      3, 3
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "b32all_fequal4",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool32,
   .input_sizes = {
      4, 4
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "b32all_fequal5",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool32,
   .input_sizes = {
      5, 5
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "b32all_fequal8",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool32,
   .input_sizes = {
      8, 8
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "b32all_iequal16",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool32,
   .input_sizes = {
      16, 16
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "b32all_iequal2",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool32,
   .input_sizes = {
      2, 2
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "b32all_iequal3",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool32,
   .input_sizes = {
      3, 3
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "b32all_iequal4",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool32,
   .input_sizes = {
      4, 4
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "b32all_iequal5",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool32,
   .input_sizes = {
      5, 5
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "b32all_iequal8",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool32,
   .input_sizes = {
      8, 8
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "b32any_fnequal16",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool32,
   .input_sizes = {
      16, 16
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "b32any_fnequal2",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool32,
   .input_sizes = {
      2, 2
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "b32any_fnequal3",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool32,
   .input_sizes = {
      3, 3
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "b32any_fnequal4",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool32,
   .input_sizes = {
      4, 4
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "b32any_fnequal5",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool32,
   .input_sizes = {
      5, 5
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "b32any_fnequal8",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool32,
   .input_sizes = {
      8, 8
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "b32any_inequal16",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool32,
   .input_sizes = {
      16, 16
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "b32any_inequal2",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool32,
   .input_sizes = {
      2, 2
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "b32any_inequal3",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool32,
   .input_sizes = {
      3, 3
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "b32any_inequal4",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool32,
   .input_sizes = {
      4, 4
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "b32any_inequal5",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool32,
   .input_sizes = {
      5, 5
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "b32any_inequal8",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool32,
   .input_sizes = {
      8, 8
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "b32csel",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_bool32, nir_type_uint, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_SELECTION,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "b32fcsel_mdg",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_bool32, nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_SELECTION,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "ball_fequal16",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool1,
   .input_sizes = {
      16, 16
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "ball_fequal2",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool1,
   .input_sizes = {
      2, 2
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "ball_fequal3",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool1,
   .input_sizes = {
      3, 3
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "ball_fequal4",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool1,
   .input_sizes = {
      4, 4
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "ball_fequal5",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool1,
   .input_sizes = {
      5, 5
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "ball_fequal8",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool1,
   .input_sizes = {
      8, 8
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "ball_iequal16",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool1,
   .input_sizes = {
      16, 16
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "ball_iequal2",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool1,
   .input_sizes = {
      2, 2
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "ball_iequal3",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool1,
   .input_sizes = {
      3, 3
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "ball_iequal4",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool1,
   .input_sizes = {
      4, 4
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "ball_iequal5",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool1,
   .input_sizes = {
      5, 5
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "ball_iequal8",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool1,
   .input_sizes = {
      8, 8
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "bany_fnequal16",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool1,
   .input_sizes = {
      16, 16
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "bany_fnequal2",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool1,
   .input_sizes = {
      2, 2
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "bany_fnequal3",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool1,
   .input_sizes = {
      3, 3
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "bany_fnequal4",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool1,
   .input_sizes = {
      4, 4
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "bany_fnequal5",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool1,
   .input_sizes = {
      5, 5
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "bany_fnequal8",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool1,
   .input_sizes = {
      8, 8
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "bany_inequal16",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool1,
   .input_sizes = {
      16, 16
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "bany_inequal2",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool1,
   .input_sizes = {
      2, 2
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "bany_inequal3",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool1,
   .input_sizes = {
      3, 3
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "bany_inequal4",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool1,
   .input_sizes = {
      4, 4
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "bany_inequal5",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool1,
   .input_sizes = {
      5, 5
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "bany_inequal8",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_bool1,
   .input_sizes = {
      8, 8
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "bcsel",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_bool1, nir_type_uint, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_SELECTION,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "bcsel_pan",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_SELECTION,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "bf2f",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float32,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_uint16
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "bfdot16",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_uint16,
   .input_sizes = {
      16, 16
   },
   .input_types = {
      nir_type_uint16, nir_type_uint16
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "bfdot2",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_uint16,
   .input_sizes = {
      2, 2
   },
   .input_types = {
      nir_type_uint16, nir_type_uint16
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "bfdot2_bfadd",
   .num_inputs = 3,
   .output_size = 1,
   .output_type = nir_type_int16,
   .input_sizes = {
      2, 2, 1
   },
   .input_types = {
      nir_type_int16, nir_type_int16, nir_type_int16
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "bfdot2_fadd",
   .num_inputs = 3,
   .output_size = 1,
   .output_type = nir_type_float32,
   .input_sizes = {
      2, 2, 1
   },
   .input_types = {
      nir_type_int16, nir_type_int16, nir_type_float32
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "bfdot3",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_uint16,
   .input_sizes = {
      3, 3
   },
   .input_types = {
      nir_type_uint16, nir_type_uint16
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "bfdot4",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_uint16,
   .input_sizes = {
      4, 4
   },
   .input_types = {
      nir_type_uint16, nir_type_uint16
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "bfdot5",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_uint16,
   .input_sizes = {
      5, 5
   },
   .input_types = {
      nir_type_uint16, nir_type_uint16
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "bfdot8",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_uint16,
   .input_sizes = {
      8, 8
   },
   .input_types = {
      nir_type_uint16, nir_type_uint16
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "bffma",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_uint16,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint16, nir_type_uint16, nir_type_uint16
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "bffma_mixed_intel",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_uint16,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_float32, nir_type_uint16, nir_type_uint16
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "bfi",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_uint32,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint32, nir_type_uint32, nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "bfm",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint32,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_int32, nir_type_int32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "bfmul",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint16,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint16, nir_type_uint16
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE | NIR_OP_IS_INEXACT_ASSOCIATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "bfmul_mixed_intel",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint16,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint16, nir_type_float32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "bit_count",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_uint32,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "bitfield_insert",
   .num_inputs = 4,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0, 0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint, nir_type_int32, nir_type_int32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "bitfield_insert_etna",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0, 2
   },
   .input_types = {
      nir_type_uint, nir_type_uint, nir_type_int32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "bitfield_reverse",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "bitfield_select",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "bitnz",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_bool1,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "bitz",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_bool1,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "bounds_agx",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_int,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_int, nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "byte_perm_amd",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_uint32,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint32, nir_type_uint32, nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "cube_amd",
   .num_inputs = 1,
   .output_size = 4,
   .output_type = nir_type_float32,
   .input_sizes = {
      3
   },
   .input_types = {
      nir_type_float32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "e4m3fn2f",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float32,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_uint8
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "e4m3fn_dot4_fadd",
   .num_inputs = 3,
   .output_size = 1,
   .output_type = nir_type_float32,
   .input_sizes = {
      1, 1, 1
   },
   .input_types = {
      nir_type_uint32, nir_type_uint32, nir_type_float32
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "e4m3fn_e5m2_dot4_fadd",
   .num_inputs = 3,
   .output_size = 1,
   .output_type = nir_type_float32,
   .input_sizes = {
      1, 1, 1
   },
   .input_types = {
      nir_type_uint32, nir_type_uint32, nir_type_float32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "e5m22f",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float32,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_uint8
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "e5m2_dot4_fadd",
   .num_inputs = 3,
   .output_size = 1,
   .output_type = nir_type_float32,
   .input_sizes = {
      1, 1, 1
   },
   .input_types = {
      nir_type_uint32, nir_type_uint32, nir_type_float32
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "extr_agx",
   .num_inputs = 4,
   .output_size = 0,
   .output_type = nir_type_uint32,
   .input_sizes = {
      0, 0, 0, 0
   },
   .input_types = {
      nir_type_uint32, nir_type_uint32, nir_type_uint32, nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "extract_i16",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_int,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "extract_i8",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_int,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "extract_u16",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "extract_u8",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "f16dot2_fadd",
   .num_inputs = 3,
   .output_size = 1,
   .output_type = nir_type_float,
   .input_sizes = {
      2, 2, 1
   },
   .input_types = {
      nir_type_float16, nir_type_float16, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "f2bf",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_uint16,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float32
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "f2e4m3fn",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_uint8,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float32
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "f2e4m3fn_sat",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_uint8,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float32
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "f2e4m3fn_satfn",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_uint8,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float32
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "f2e5m2",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_uint8,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float32
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "f2e5m2_sat",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_uint8,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float32
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "f2f16",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float16,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "f2f16_rd",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float16,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "f2f16_rtne",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float16,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "f2f16_rtz",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float16,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "f2f16_ru",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float16,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "f2f32",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float32,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "f2f64",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float64,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "f2fmp",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float16,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float32
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "f2i1",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_int1,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_exact
},
{
   .name = "f2i16",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_int16,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_exact
},
{
   .name = "f2i16_sat",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_int16,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "f2i32",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_int32,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_exact
},
{
   .name = "f2i32_rtne",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_int32,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float32
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_exact
},
{
   .name = "f2i32_sat",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_int32,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "f2i64",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_int64,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_exact
},
{
   .name = "f2i8",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_int8,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_exact
},
{
   .name = "f2i8_sat",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_int8,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "f2imp",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_int16,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float32
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_exact
},
{
   .name = "f2snorm_16_v3d",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_uint32,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "f2u1",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_uint1,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_exact
},
{
   .name = "f2u16",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_uint16,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_exact
},
{
   .name = "f2u16_sat",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_uint16,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "f2u32",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_uint32,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_exact
},
{
   .name = "f2u32_rtne",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_uint32,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float32
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_exact
},
{
   .name = "f2u32_sat",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_uint32,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "f2u64",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_uint64,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_exact
},
{
   .name = "f2u8",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_uint8,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_exact
},
{
   .name = "f2u8_sat",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_uint8,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "f2ump",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_uint16,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float32
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_exact
},
{
   .name = "f2unorm_16_v3d",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_uint32,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "fabs",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fadd",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE | NIR_OP_IS_INEXACT_ASSOCIATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fcanonicalize",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fceil",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fclamp_pos",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fcopysign_pco",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fcos",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fcos_mdg",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fcos_normalized_2_pi",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fcsel",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_float32,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_float32, nir_type_float32, nir_type_float32
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_SELECTION,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "fcsel_ge",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_float32,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_float32, nir_type_float32, nir_type_float32
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_SELECTION,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "fcsel_gt",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_float32,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_float32, nir_type_float32, nir_type_float32
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_SELECTION,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "fdiv",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fdot16",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_float,
   .input_sizes = {
      16, 16
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fdot16_replicated",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      16, 16
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fdot2",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_float,
   .input_sizes = {
      2, 2
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fdot2_replicated",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      2, 2
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fdot3",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_float,
   .input_sizes = {
      3, 3
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fdot3_replicated",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      3, 3
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fdot4",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_float,
   .input_sizes = {
      4, 4
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fdot4_replicated",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      4, 4
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fdot5",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_float,
   .input_sizes = {
      5, 5
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fdot5_replicated",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      5, 5
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fdot8",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_float,
   .input_sizes = {
      8, 8
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fdot8_replicated",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      8, 8
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fdph",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_float,
   .input_sizes = {
      3, 4
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fdph_replicated",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      3, 4
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "feq",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_bool1,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "feq32",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_bool32,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "feq_pan",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "fequ",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_bool1,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "fequ32",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_bool32,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "fequ_pan",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "fexp2",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "ffloor",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "ffma",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "ffma_weak",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "ffmaz",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_float32,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_float32, nir_type_float32, nir_type_float32
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "ffract",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fge",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_bool1,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "fge32",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_bool32,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "fge_pan",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "fgeu",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_bool1,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "fgeu32",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_bool32,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "fgeu_pan",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "find_lsb",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_int32,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "fisfinite",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_bool1,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fisfinite32",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_bool32,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fisnormal",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_bool1,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "flog2",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "flrp",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "flt",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_bool1,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "flt32",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_bool32,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "flt_pan",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "fltu",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_bool1,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "fltu32",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_bool32,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "fltu_pan",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "fmad",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fmadz",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_float32,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_float32, nir_type_float32, nir_type_float32
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fmax",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE | NIR_OP_IS_ASSOCIATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "fmax_agx",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint32,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint32, nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE | NIR_OP_IS_ASSOCIATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "fmin",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE | NIR_OP_IS_ASSOCIATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "fmin_agx",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint32,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint32, nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE | NIR_OP_IS_ASSOCIATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "fmod",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fmul",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE | NIR_OP_IS_INEXACT_ASSOCIATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fmul_rtz",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_float32,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float32, nir_type_float32
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE | NIR_OP_IS_INEXACT_ASSOCIATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fmulz",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_float32,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float32, nir_type_float32
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE | NIR_OP_IS_INEXACT_ASSOCIATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fneg",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fneo",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_bool1,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "fneo32",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_bool32,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "fneo_pan",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "fneu",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_bool1,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "fneu32",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_bool32,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "fneu_pan",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "ford",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_bool1,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "ford32",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_bool32,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "ford_pan",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "fpow",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fquantize2f16",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float32,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "frcp",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "frem",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "frexp_exp",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_int32,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "frexp_sig",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fround_even",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "frsq",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fsat",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fsat_signed",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fsign",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fsin",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fsin_agx",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fsin_mdg",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fsin_normalized_2_pi",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fsqrt",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fsub",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fsum2",
   .num_inputs = 1,
   .output_size = 1,
   .output_type = nir_type_float,
   .input_sizes = {
      2
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fsum3",
   .num_inputs = 1,
   .output_size = 1,
   .output_type = nir_type_float,
   .input_sizes = {
      3
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "fsum4",
   .num_inputs = 1,
   .output_size = 1,
   .output_type = nir_type_float,
   .input_sizes = {
      4
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "ftrunc",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "funord",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_bool1,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "funord32",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_bool32,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "funord_pan",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "i2f16",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float16,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_int
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_exact
},
{
   .name = "i2f32",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float32,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_int
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_exact
},
{
   .name = "i2f64",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float64,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_int
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_exact
},
{
   .name = "i2fmp",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float16,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_int32
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_exact
},
{
   .name = "i2i1",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_int1,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_int
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "i2i16",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_int16,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_int
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "i2i32",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_int32,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_int
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "i2i64",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_int64,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_int
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "i2i8",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_int8,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_int
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "i2imp",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_int16,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_int32
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "i32csel_ge",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_int32,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_int32, nir_type_int32, nir_type_int32
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_SELECTION,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "i32csel_gt",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_int32,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_int32, nir_type_int32, nir_type_int32
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_SELECTION,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "iabs",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_int,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "iadd",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_int,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE | NIR_OP_IS_ASSOCIATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "iadd3",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_int,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_int, nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "iadd_sat",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_int,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "iand",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE | NIR_OP_IS_ASSOCIATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "ibfe",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_int32,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_int32, nir_type_uint32, nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "ibitfield_extract",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_int,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_int, nir_type_int32, nir_type_int32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "icsel_eqz",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_int,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_int, nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_SELECTION,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "idiv",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_int,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "ieq",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_bool1,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "ieq32",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_bool32,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "ieq_pan",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "ifind_msb",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_int32,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_int32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "ifind_msb_rev",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_int32,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_int32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "ige",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_bool1,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "ige32",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_bool32,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "ige_pan",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "ihadd",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_int,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "ilea_agx",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_uint64,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint64, nir_type_int32, nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "ilt",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_bool1,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "ilt32",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_bool32,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "ilt_pan",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "imad",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_int,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_int, nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "imad24_ir3",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_int32,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_int32, nir_type_int32, nir_type_int32
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "imadsh_mix16",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_int32,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_int32, nir_type_int32, nir_type_int32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "imadshl_agx",
   .num_inputs = 4,
   .output_size = 0,
   .output_type = nir_type_int,
   .input_sizes = {
      0, 0, 0, 0
   },
   .input_types = {
      nir_type_int, nir_type_int, nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "imax",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_int,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE | NIR_OP_IS_ASSOCIATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "imin",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_int,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE | NIR_OP_IS_ASSOCIATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "imod",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_int,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "imsubshl_agx",
   .num_inputs = 4,
   .output_size = 0,
   .output_type = nir_type_int,
   .input_sizes = {
      0, 0, 0, 0
   },
   .input_types = {
      nir_type_int, nir_type_int, nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "imul",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_int,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE | NIR_OP_IS_ASSOCIATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "imul24",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_int32,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_int32, nir_type_int32
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "imul24_relaxed",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_int32,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_int32, nir_type_int32
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "imul_2x32_64",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_int64,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_int32, nir_type_int32
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "imul_32x16",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_int32,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_int32, nir_type_int32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "imul_high",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_int,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "ine",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_bool1,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "ine32",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_bool32,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "ine_pan",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "ineg",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_int,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "inot",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_int,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "insert_u16",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "insert_u8",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "interleave",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint32,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "ior",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE | NIR_OP_IS_ASSOCIATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "irem",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_int,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "irhadd",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_int,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "ishl",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_int,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_int, nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "ishr",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_int,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_int, nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "isign",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_int,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "isub",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_int,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "isub_sat",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_int,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "ixor",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE | NIR_OP_IS_ASSOCIATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "ldexp",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_int32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "ldexp16_pan",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_float16,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float16, nir_type_int16
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "lea_nv",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint, nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "lrot_and_pan",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint8, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "lrot_or_pan",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint8, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "lrot_xor_pan",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint8, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "lshift_and_pan",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint8, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "lshift_or_pan",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint8, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "lshift_xor_pan",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint8, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "mov",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "mqsad_4x8",
   .num_inputs = 3,
   .output_size = 4,
   .output_type = nir_type_uint32,
   .input_sizes = {
      1, 2, 4
   },
   .input_types = {
      nir_type_uint32, nir_type_uint32, nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "msad_4x8",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_uint32,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint32, nir_type_uint32, nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "pack_2x16_to_snorm_2x8_v3d",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_uint32,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "pack_2x16_to_unorm_10_2_v3d",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_uint32,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "pack_2x16_to_unorm_2x10_v3d",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_uint32,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "pack_2x16_to_unorm_2x8_v3d",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_uint32,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "pack_2x32_to_2x16_v3d",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_uint32,
   .input_sizes = {
      1, 1
   },
   .input_types = {
      nir_type_uint32, nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "pack_32_2x16",
   .num_inputs = 1,
   .output_size = 1,
   .output_type = nir_type_uint32,
   .input_sizes = {
      2
   },
   .input_types = {
      nir_type_uint16
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "pack_32_2x16_split",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint32,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint16, nir_type_uint16
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "pack_32_4x8",
   .num_inputs = 1,
   .output_size = 1,
   .output_type = nir_type_uint32,
   .input_sizes = {
      4
   },
   .input_types = {
      nir_type_uint8
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "pack_32_4x8_split",
   .num_inputs = 4,
   .output_size = 0,
   .output_type = nir_type_uint32,
   .input_sizes = {
      0, 0, 0, 0
   },
   .input_types = {
      nir_type_uint8, nir_type_uint8, nir_type_uint8, nir_type_uint8
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "pack_32_to_r11g11b10_v3d",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint32,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint32, nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "pack_4x16_to_4x8_v3d",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint32,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint32, nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "pack_64_2x32",
   .num_inputs = 1,
   .output_size = 1,
   .output_type = nir_type_uint64,
   .input_sizes = {
      2
   },
   .input_types = {
      nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "pack_64_2x32_split",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint64,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint32, nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "pack_64_4x16",
   .num_inputs = 1,
   .output_size = 1,
   .output_type = nir_type_uint64,
   .input_sizes = {
      4
   },
   .input_types = {
      nir_type_uint16
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "pack_double_2x32_dxil",
   .num_inputs = 1,
   .output_size = 1,
   .output_type = nir_type_uint64,
   .input_sizes = {
      2
   },
   .input_types = {
      nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "pack_half_2x16",
   .num_inputs = 1,
   .output_size = 1,
   .output_type = nir_type_uint32,
   .input_sizes = {
      2
   },
   .input_types = {
      nir_type_float32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "pack_half_2x16_rtz_split",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_uint32,
   .input_sizes = {
      1, 1
   },
   .input_types = {
      nir_type_float32, nir_type_float32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "pack_half_2x16_split",
   .num_inputs = 2,
   .output_size = 1,
   .output_type = nir_type_uint32,
   .input_sizes = {
      1, 1
   },
   .input_types = {
      nir_type_float32, nir_type_float32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "pack_sint_2x16",
   .num_inputs = 1,
   .output_size = 1,
   .output_type = nir_type_int32,
   .input_sizes = {
      2
   },
   .input_types = {
      nir_type_int32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "pack_snorm_2x16",
   .num_inputs = 1,
   .output_size = 1,
   .output_type = nir_type_uint32,
   .input_sizes = {
      2
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "pack_snorm_4x8",
   .num_inputs = 1,
   .output_size = 1,
   .output_type = nir_type_uint32,
   .input_sizes = {
      4
   },
   .input_types = {
      nir_type_float32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "pack_uint_2x16",
   .num_inputs = 1,
   .output_size = 1,
   .output_type = nir_type_uint32,
   .input_sizes = {
      2
   },
   .input_types = {
      nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "pack_uint_32_to_r10g10b10a2_v3d",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint32,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint32, nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "pack_unorm_2x16",
   .num_inputs = 1,
   .output_size = 1,
   .output_type = nir_type_uint32,
   .input_sizes = {
      2
   },
   .input_types = {
      nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "pack_unorm_4x8",
   .num_inputs = 1,
   .output_size = 1,
   .output_type = nir_type_uint32,
   .input_sizes = {
      4
   },
   .input_types = {
      nir_type_float32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "pack_uvec2_to_uint",
   .num_inputs = 1,
   .output_size = 1,
   .output_type = nir_type_uint32,
   .input_sizes = {
      2
   },
   .input_types = {
      nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "pack_uvec4_to_uint",
   .num_inputs = 1,
   .output_size = 1,
   .output_type = nir_type_uint32,
   .input_sizes = {
      4
   },
   .input_types = {
      nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "prmt_nv",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_uint32,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint32, nir_type_uint32, nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "rrot_and_pan",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint8, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "rrot_or_pan",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint8, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "rrot_xor_pan",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint8, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "rshift_and_pan",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint8, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "rshift_or_pan",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint8, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "rshift_xor_pan",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint8, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "sdot_2x16_iadd",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_int32,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint32, nir_type_uint32, nir_type_int32
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "sdot_2x16_iadd_sat",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_int32,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint32, nir_type_uint32, nir_type_int32
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "sdot_4x8_iadd",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_int32,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint32, nir_type_uint32, nir_type_int32
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "sdot_4x8_iadd_sat",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_int32,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint32, nir_type_uint32, nir_type_int32
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "seq",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "sge",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "shfr",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_uint32,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint32, nir_type_uint32, nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "shlg_ir3",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint32, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "shlm_ir3",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint32, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "shrg_ir3",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint32, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "shrm_ir3",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint32, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "slt",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "sne",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_float,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_float, nir_type_float
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan
},
{
   .name = "snorm2f_16_v3d",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_uint32,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "sudot_4x8_iadd",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_int32,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint32, nir_type_uint32, nir_type_int32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "sudot_4x8_iadd_sat",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_int32,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint32, nir_type_uint32, nir_type_int32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "u2f16",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float16,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_uint
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_exact
},
{
   .name = "u2f32",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float32,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_uint
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_exact
},
{
   .name = "u2f64",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float64,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_uint
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_exact
},
{
   .name = "u2fmp",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float16,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_uint32
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_exact
},
{
   .name = "u2u1",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_uint1,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_uint
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "u2u16",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_uint16,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_uint
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "u2u32",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_uint32,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_uint
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "u2u64",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_uint64,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_uint
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "u2u8",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_uint8,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_uint
   },
   .is_conversion = true,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "uabs_isub",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_int, nir_type_int
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "uabs_usub",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "uadd64_32",
   .num_inputs = 3,
   .output_size = 2,
   .output_type = nir_type_uint32,
   .input_sizes = {
      1, 1, 1
   },
   .input_types = {
      nir_type_uint32, nir_type_uint32, nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "uadd_carry",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "uadd_sat",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "ubfe",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_uint32,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint32, nir_type_uint32, nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "ubitfield_extract",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_int32, nir_type_int32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "uclz",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_uint32,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "udiv",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "udiv_aligned_4",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "udot_2x16_uadd",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_uint32,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint32, nir_type_uint32, nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "udot_2x16_uadd_sat",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_uint32,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint32, nir_type_uint32, nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "udot_4x8_uadd",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_uint32,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint32, nir_type_uint32, nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "udot_4x8_uadd_sat",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_int32,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint32, nir_type_uint32, nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "ufind_msb",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_int32,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "ufind_msb_rev",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_int32,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "uge",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_bool1,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "uge32",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_bool32,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "uge_pan",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "uhadd",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "ulea_agx",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_uint64,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint64, nir_type_uint32, nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "ult",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_bool1,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "ult32",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_bool32,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "ult_pan",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "umad24",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_uint32,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint32, nir_type_uint32, nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "umad24_relaxed",
   .num_inputs = 3,
   .output_size = 0,
   .output_type = nir_type_uint32,
   .input_sizes = {
      0, 0, 0
   },
   .input_types = {
      nir_type_uint32, nir_type_uint32, nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "umad64_32",
   .num_inputs = 4,
   .output_size = 2,
   .output_type = nir_type_uint32,
   .input_sizes = {
      1, 1, 1, 1
   },
   .input_types = {
      nir_type_uint32, nir_type_uint32, nir_type_uint32, nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "umax",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE | NIR_OP_IS_ASSOCIATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "umax_4x8_vc4",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_int32,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_int32, nir_type_int32
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE | NIR_OP_IS_ASSOCIATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "umin",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE | NIR_OP_IS_ASSOCIATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "umin_4x8_vc4",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_int32,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_int32, nir_type_int32
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE | NIR_OP_IS_ASSOCIATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "umod",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "umul24",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_int32,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_int32, nir_type_int32
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "umul24_relaxed",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint32,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint32, nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "umul_16x16",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint32,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint32, nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "umul_2x32_64",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint64,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint32, nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "umul_32x16",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint32,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint32, nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "umul_high",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "umul_unorm_4x8_vc4",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint32,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint32, nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE | NIR_OP_IS_ASSOCIATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "unorm2f_16_v3d",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_uint32,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "unpack_32_2x16",
   .num_inputs = 1,
   .output_size = 2,
   .output_type = nir_type_uint16,
   .input_sizes = {
      1
   },
   .input_types = {
      nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "unpack_32_2x16_split_x",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_uint16,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "unpack_32_2x16_split_y",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_uint16,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "unpack_32_4x8",
   .num_inputs = 1,
   .output_size = 4,
   .output_type = nir_type_uint8,
   .input_sizes = {
      1
   },
   .input_types = {
      nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "unpack_64_2x32",
   .num_inputs = 1,
   .output_size = 2,
   .output_type = nir_type_uint32,
   .input_sizes = {
      1
   },
   .input_types = {
      nir_type_uint64
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "unpack_64_2x32_split_x",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_uint32,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_uint64
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "unpack_64_2x32_split_y",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_uint32,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_uint64
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "unpack_64_4x16",
   .num_inputs = 1,
   .output_size = 4,
   .output_type = nir_type_uint16,
   .input_sizes = {
      1
   },
   .input_types = {
      nir_type_uint64
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "unpack_double_2x32_dxil",
   .num_inputs = 1,
   .output_size = 2,
   .output_type = nir_type_uint32,
   .input_sizes = {
      1
   },
   .input_types = {
      nir_type_uint64
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "unpack_half_2x16",
   .num_inputs = 1,
   .output_size = 2,
   .output_type = nir_type_float32,
   .input_sizes = {
      1
   },
   .input_types = {
      nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "unpack_half_x_dxil",
   .num_inputs = 1,
   .output_size = 0,
   .output_type = nir_type_float32,
   .input_sizes = {
      0
   },
   .input_types = {
      nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_preserve_inf | nir_fp_preserve_nan | nir_fp_exact
},
{
   .name = "unpack_snorm_2x16",
   .num_inputs = 1,
   .output_size = 2,
   .output_type = nir_type_float32,
   .input_sizes = {
      1
   },
   .input_types = {
      nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_exact
},
{
   .name = "unpack_snorm_4x8",
   .num_inputs = 1,
   .output_size = 4,
   .output_type = nir_type_float32,
   .input_sizes = {
      1
   },
   .input_types = {
      nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_exact
},
{
   .name = "unpack_unorm_2x16",
   .num_inputs = 1,
   .output_size = 2,
   .output_type = nir_type_float32,
   .input_sizes = {
      1
   },
   .input_types = {
      nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_exact
},
{
   .name = "unpack_unorm_4x8",
   .num_inputs = 1,
   .output_size = 4,
   .output_type = nir_type_float32,
   .input_sizes = {
      1
   },
   .input_types = {
      nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      nir_fp_preserve_signed_zero | nir_fp_exact
},
{
   .name = "urhadd",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "urol",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "uror",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "usadd_4x8_vc4",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_int32,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_int32, nir_type_int32
   },
   .is_conversion = false,
   .algebraic_properties =
      NIR_OP_IS_2SRC_COMMUTATIVE | NIR_OP_IS_ASSOCIATIVE,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "ushr",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "ussub_4x8_vc4",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_int32,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_int32, nir_type_int32
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "usub_borrow",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "usub_sat",
   .num_inputs = 2,
   .output_size = 0,
   .output_type = nir_type_uint,
   .input_sizes = {
      0, 0
   },
   .input_types = {
      nir_type_uint, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "vec16",
   .num_inputs = 16,
   .output_size = 16,
   .output_type = nir_type_uint,
   .input_sizes = {
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
   },
   .input_types = {
      nir_type_uint, nir_type_uint, nir_type_uint, nir_type_uint, nir_type_uint, nir_type_uint, nir_type_uint, nir_type_uint, nir_type_uint, nir_type_uint, nir_type_uint, nir_type_uint, nir_type_uint, nir_type_uint, nir_type_uint, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "vec2",
   .num_inputs = 2,
   .output_size = 2,
   .output_type = nir_type_uint,
   .input_sizes = {
      1, 1
   },
   .input_types = {
      nir_type_uint, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "vec3",
   .num_inputs = 3,
   .output_size = 3,
   .output_type = nir_type_uint,
   .input_sizes = {
      1, 1, 1
   },
   .input_types = {
      nir_type_uint, nir_type_uint, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "vec4",
   .num_inputs = 4,
   .output_size = 4,
   .output_type = nir_type_uint,
   .input_sizes = {
      1, 1, 1, 1
   },
   .input_types = {
      nir_type_uint, nir_type_uint, nir_type_uint, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "vec5",
   .num_inputs = 5,
   .output_size = 5,
   .output_type = nir_type_uint,
   .input_sizes = {
      1, 1, 1, 1, 1
   },
   .input_types = {
      nir_type_uint, nir_type_uint, nir_type_uint, nir_type_uint, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
{
   .name = "vec8",
   .num_inputs = 8,
   .output_size = 8,
   .output_type = nir_type_uint,
   .input_sizes = {
      1, 1, 1, 1, 1, 1, 1, 1
   },
   .input_types = {
      nir_type_uint, nir_type_uint, nir_type_uint, nir_type_uint, nir_type_uint, nir_type_uint, nir_type_uint, nir_type_uint
   },
   .is_conversion = false,
   .algebraic_properties =
      0,
   .valid_fp_math_ctrl =
      0
},
};


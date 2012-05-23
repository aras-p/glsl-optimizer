//===-- AMDILInstructions.td - AMDIL Instruction definitions --------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//

// Operations in this file are generic to all data types
// This opcode has custom swizzle pattern encoded in Swizzle Encoder
let isReMaterializable = 1, isAsCheapAsAMove = 1 in {
  defm LOADCONST  : ILConstant<"mov $dst, $val">;
  defm MOVE       : UnaryOpMC<IL_OP_MOV, IL_mov>;
  defm PHIMOVE    : UnaryOpMC<IL_OP_MOV, IL_phimov>;
}
defm BINARY_NOT : UnaryOpMC<IL_OP_I_NOT, IL_not>;
defm BINARY_OR  : BinaryOpMC<IL_OP_I_OR, IL_or>;
defm BINARY_AND : BinaryOpMC<IL_OP_AND, IL_and>;
defm BINARY_XOR : BinaryOpMC<IL_OP_I_XOR, IL_xor>;
defm AND        : BinaryOpMCInt<IL_OP_AND, and>;
defm CMOV       : BinaryOpMC<IL_OP_CMOV, IL_cmov>;
defm DIV_INF    : BinaryOpMC<IL_OP_DIV_INF, IL_div_inf>;
defm SMAX       : BinaryOpMCInt<IL_OP_I_MAX, IL_smax>;
// This opcode has a custom swizzle pattern in the Swizzle Encoder and 
// should never be selected in ISel. It should only be generated in the
// I/O expansion code. These are different from the CMOVLOG instruction
// in that the src0 argument uses a custom swizzle for the Y/Z/W
// vector channel respectively instead of the default channel.
def CMOVLOG_Y_i32 : ThreeInOneOut<IL_OP_CMOV_LOGICAL, (outs GPRI32:$dst),
    (ins GPRI32:$src0, GPRI32:$src1, GPRI32:$src2),
    !strconcat(IL_OP_CMOV_LOGICAL.Text, " $dst, $src0, $src1, $src2"),
    []>;
def CMOVLOG_Z_i32 : ThreeInOneOut<IL_OP_CMOV_LOGICAL, (outs GPRI32:$dst),
    (ins GPRI32:$src0, GPRI32:$src1, GPRI32:$src2),
    !strconcat(IL_OP_CMOV_LOGICAL.Text, " $dst, $src0, $src1, $src2"),
    []>;
def CMOVLOG_W_i32 : ThreeInOneOut<IL_OP_CMOV_LOGICAL, (outs GPRI32:$dst),
    (ins GPRI32:$src0, GPRI32:$src1 ,GPRI32:$src2),
    !strconcat(IL_OP_CMOV_LOGICAL.Text, " $dst, $src0, $src1, $src2"),
    []>;
defm SELECTBIN  : TernaryOpMCScalar<IL_OP_CMOV_LOGICAL, select>;
//===---------------------------------------------------------------------===//
// Signed 8bit integer math instructions start here
//===---------------------------------------------------------------------===//
def INTTOANY_i8 : OneInOneOut<IL_OP_MOV, (outs GPRI8:$dst), (ins GPRI32:$src0),
    !strconcat(IL_OP_MOV.Text, " $dst, $src0"),
    [(set GPRI8:$dst, (IL_inttoany GPRI32:$src0))]>;
//===---------------------------------------------------------------------===//
// Signed 16bit integer math instructions start here
//===---------------------------------------------------------------------===//
def INTTOANY_i16: OneInOneOut<IL_OP_MOV, (outs GPRI16:$dst), (ins GPRI32:$src0),
    !strconcat(IL_OP_MOV.Text," $dst, $src0"), 
    [(set GPRI16:$dst, (IL_inttoany GPRI32:$src0))]>;
//===---------------------------------------------------------------------===//
// Signed 32bit integer math instructions start here
//===---------------------------------------------------------------------===//
defm NEGATE     : UnaryOpMCi32<IL_OP_I_NEGATE, IL_inegate>;
defm SMUL       : BinaryOpMCi32<IL_OP_I_MUL, mul>;
defm SMULHI     : BinaryOpMCi32<IL_OP_I_MUL_HIGH, mulhs>;
defm SHL        : BinaryOpMCi32Const<IL_OP_I_SHL, shl>;
defm SHR        : BinaryOpMCi32Const<IL_OP_I_SHR, sra>;
defm SHLVEC     : BinaryOpMCi32<IL_OP_I_SHL, shl>;
defm SHRVEC     : BinaryOpMCi32<IL_OP_I_SHR, sra>;
defm ADD        : BinaryOpMCi32<IL_OP_I_ADD, add>;
defm CUSTOM_XOR : BinaryOpMCInt<IL_OP_I_XOR, xor>;
// get rid of the addri via the tablegen instead of custom lowered instruction
defm CUSTOM_ADD : BinaryOpMCi32<IL_OP_I_ADD, IL_add>;
defm EADD   : BinaryOpMCi32<IL_OP_I_ADD, adde>;
def INTTOANY_i32: OneInOneOut<IL_OP_MOV, (outs GPRI32:$dst), (ins GPRI32:$src0),
    !strconcat(IL_OP_MOV.Text, " $dst, $src0"), 
    [(set GPRI32:$dst, (IL_inttoany GPRI32:$src0))]>;
// Integer offsets for addressing
// This opcode has custom swizzle pattern encoded in Swizzle Encoder
def ADDir       : TwoInOneOut<IL_OP_I_ADD, (outs GPRI32:$dst),
      (ins MEMI32:$ptr, GPRI32:$offset),
          !strconcat(IL_OP_I_ADD.Text, " $dst, $ptr, $offset"),
          [(set GPRI32:$dst,
        (IL_addaddrri ADDR:$ptr,
          (i32 GPRI32:$offset)))]>;
// This opcode has custom swizzle pattern encoded in Swizzle Encoder
def ADDri       : TwoInOneOut<IL_OP_I_ADD, (outs GPRI32:$dst),
      (ins GPRI32:$offset,  MEMI32:$ptr),
          !strconcat(IL_OP_I_ADD.Text, " $dst, $offset, $ptr"),
          [(set GPRI32:$dst,
        (IL_addaddrir
          (i32 GPRI32:$offset), ADDR:$ptr))]>;

defm IFFB_HI    : UnaryOpMCi32<IL_OP_I_FFB_HI, IL_ffb_hi>;
defm IFFB_LO    : UnaryOpMCi32<IL_OP_I_FFB_LO, IL_ffb_lo>;
let mayLoad = 0, mayStore = 0 in {
defm BITCOUNT : UnaryIntrinsicInt<IL_OP_IBIT_COUNT, int_AMDIL_bit_count_i32>;
defm FFB_LO : UnaryIntrinsicInt<IL_OP_I_FFB_LO, int_AMDIL_bit_find_first_lo>;
defm FFB_HI : UnaryIntrinsicInt<IL_OP_I_FFB_HI, int_AMDIL_bit_find_first_hi>;
defm FFB_SGN : UnaryIntrinsicInt<IL_OP_I_FFB_SGN,
        int_AMDIL_bit_find_first_sgn>;
defm IMULHI  : BinaryIntrinsicInt<IL_OP_I_MUL_HIGH, int_AMDIL_mulhi_i32>;
let Predicates = [HasHWSign24Bit] in {
defm IMUL24 : BinaryIntrinsicInt<IL_OP_I_MUL24, int_AMDIL_mul24_i32>;
defm IMULHI24 : BinaryIntrinsicInt<IL_OP_I_MULHI24, int_AMDIL_mulhi24_i32>;
defm IMAD24  : TernaryIntrinsicInt<IL_OP_I_MAD24, int_AMDIL_mad24_i32>;
}
defm CARRY  : BinaryIntrinsicInt<IL_OP_I_CARRY, int_AMDIL_carry_i32>;
defm BORROW  : BinaryIntrinsicInt<IL_OP_I_BORROW, int_AMDIL_borrow_i32>;
defm IMAX  : BinaryIntrinsicInt<IL_OP_I_MAX, int_AMDIL_max_i32>;
defm IBIT_EXTRACT : TernaryIntrinsicInt<IL_OP_IBIT_EXTRACT,
          int_AMDIL_bit_extract_i32>;
defm IMAD  : TernaryIntrinsicInt<IL_OP_I_MAD, int_AMDIL_mad_i32>;
defm SAD  : TernaryIntrinsicInt<IL_OP_SAD, int_AMDIL_media_sad>;
defm SADHI  : TernaryIntrinsicInt<IL_OP_SAD_HI,
          int_AMDIL_media_sad_hi>;
}
def SAD4_i32  : ThreeInOneOut<IL_OP_SAD4, (outs GPRI32:$dst),
      (ins GPRV4I32:$src, GPRV4I32:$src1, GPRI32:$src2),
      !strconcat(IL_OP_SAD4.Text, " $dst, $src, $src1, $src2"),
      [(set GPRI32:$dst,
      (int_AMDIL_media_sad4 GPRV4I32:$src, GPRV4I32:$src1,
      GPRI32:$src2))]>;
def FTOV4U8_i32 : OneInOneOut<IL_OP_F2U4, (outs GPRI32:$dst),
      (ins GPRV4F32:$src),
      !strconcat(IL_OP_F2U4.Text, " $dst, $src"),
      [(set GPRI32:$dst,
      (int_AMDIL_media_convert_f2v4u8 GPRV4F32:$src))]>;
//===---------------------------------------------------------------------===//
// Unsigned 32bit integer math instructions start here
//===---------------------------------------------------------------------===//
defm UMUL       : BinaryOpMCi32<IL_OP_U_MUL, IL_umul>;
defm UMULHI     : BinaryOpMCi32<IL_OP_U_MUL_HIGH, mulhu>;
defm USHR       : BinaryOpMCi32Const<IL_OP_U_SHR, srl>;
defm USHRVEC    : BinaryOpMCi32<IL_OP_U_SHR, srl>;
defm UDIV       : BinaryOpMCi32<IL_OP_U_DIV, udiv>;
defm NATIVE_UDIV  : BinaryIntrinsicInt<IL_OP_U_DIV, int_AMDIL_udiv>;
let mayLoad=0, mayStore=0 in {
defm UBIT_REVERSE : UnaryIntrinsicInt<IL_OP_UBIT_REVERSE,
        int_AMDIL_bit_reverse_u32>;
defm UMULHI_INT : BinaryIntrinsicInt<IL_OP_U_MUL_HIGH, int_AMDIL_mulhi_u32>;
defm UMULHI24   : BinaryIntrinsicInt<IL_OP_U_MULHI24, int_AMDIL_mulhi24_u32>;
defm UMUL24     : BinaryIntrinsicInt<IL_OP_U_MUL24, int_AMDIL_mul24_u32>;
defm UMAX  : BinaryIntrinsicInt<IL_OP_U_MAX, int_AMDIL_max_u32>;
defm UBIT_EXTRACT : TernaryIntrinsicInt<IL_OP_UBIT_EXTRACT,
          int_AMDIL_bit_extract_u32>;
defm UBIT_INSERT : QuaternaryIntrinsicInt<IL_OP_UBIT_INSERT,
          int_AMDIL_bit_insert_u32>;
defm BFI : TernaryIntrinsicInt<IL_OP_BFI, int_AMDIL_bfi>;
defm BFM : BinaryIntrinsicInt<IL_OP_BFM, int_AMDIL_bfm>;
defm UMAD  : TernaryIntrinsicInt<IL_OP_U_MAD, int_AMDIL_mad_u32>;
defm UMAD24  : TernaryIntrinsicInt<IL_OP_U_MAD24, int_AMDIL_mad24_u32>;
defm U4LERP  : TernaryIntrinsicInt<IL_OP_U4_LERP,
          int_AMDIL_media_lerp_u4>;
defm BITALIGN : TernaryIntrinsicInt<IL_OP_BIT_ALIGN, int_AMDIL_media_bitalign>;
defm BYTEALIGN : TernaryIntrinsicInt<IL_OP_BYTE_ALIGN, int_AMDIL_media_bytealign>;
}
//===---------------------------------------------------------------------===//
// Signed 64bit integer math instructions start here
//===---------------------------------------------------------------------===//
// This opcode has custom swizzle pattern encoded in Swizzle Encoder
def LNEGATE     : OneInOneOut<IL_OP_MOV,  (outs GPRI64:$dst), (ins GPRI64:$src),
                !strconcat(IL_OP_MOV.Text, " $dst, $src"),
                [(set GPRI64:$dst, (IL_inegate GPRI64:$src))]>;
// This opcode has custom swizzle pattern encoded in Swizzle Encoder
def LNEGATE_v2i64: OneInOneOut<IL_OP_MOV,  (outs GPRV2I64:$dst),
                (ins GPRV2I64:$src),
                !strconcat(IL_OP_MOV.Text, " $dst, $src"),
                [(set GPRV2I64:$dst, (IL_inegate GPRV2I64:$src))]>;
let Predicates = [HasHW64Bit] in {
def LADD        : TwoInOneOut<IL_OP_I64_ADD, (outs GPRI64:$dst),
                  (ins GPRI64:$src1, GPRI64:$src2),
                  !strconcat(IL_OP_I64_ADD.Text, " $dst, $src1, $src2"),
                [(set GPRI64:$dst, (IL_add GPRI64:$src1, GPRI64:$src2))]>;
defm IMAX64 : BinaryIntrinsicLong<IL_OP_I64_MAX, int_AMDIL_max_i32>;
defm UMAX64 : BinaryIntrinsicLong<IL_OP_U64_MAX, int_AMDIL_max_u32>;
}
let Predicates = [HasHW64Bit] in {
def LSHR        : TwoInOneOut<IL_OP_I64_SHR, (outs GPRI64:$dst),
                  (ins GPRI64:$src1, GPRI32:$src2),
                  !strconcat(IL_OP_I64_SHR.Text, " $dst, $src1, $src2"),
                [(set GPRI64:$dst, (sra GPRI64:$src1, GPRI32:$src2))]>;
def LSHL       : TwoInOneOut<IL_OP_I64_SHL, (outs GPRI64:$dst),
                  (ins GPRI64:$src1, GPRI32:$src2),
                  !strconcat(IL_OP_I64_SHL.Text, " $dst, $src1, $src2"),
                [(set GPRI64:$dst, (shl GPRI64:$src1, GPRI32:$src2))]>;
}


//===---------------------------------------------------------------------===//
// Unsigned 64bit integer math instructions start here
//===---------------------------------------------------------------------===//
let Predicates = [HasTmrRegister] in {
  def Tmr : ILFormat<IL_OP_MOV, (outs GPRI64:$tmr),
      (ins), !strconcat(IL_OP_MOV.Text, " $tmr, Tmr"),
      [(set GPRI64:$tmr, (int_AMDIL_get_cycle_count))]>;
}
let Predicates = [HasDeviceIDInst] in {
def CU_ID : ILFormat<IL_OP_CU_ID, (outs GPRI32:$id), (ins),
    !strconcat(IL_OP_CU_ID.Text, " $id"),
    [(set GPRI32:$id, (int_AMDIL_compute_unit_id))]>;
def WAVE_ID : ILFormat<IL_OP_WAVE_ID, (outs GPRI32:$id), (ins),
    !strconcat(IL_OP_WAVE_ID.Text, " $id"),
    [(set GPRI32:$id, (int_AMDIL_wavefront_id))]>;
}
let Predicates = [HasHW64Bit] in {
def LUSHR        : TwoInOneOut<IL_OP_U64_SHR, (outs GPRI64:$dst),
                  (ins GPRI64:$src1, GPRI32:$src2),
                  !strconcat(IL_OP_U64_SHR.Text, " $dst, $src1, $src2"),
                [(set GPRI64:$dst, (srl GPRI64:$src1, GPRI32:$src2))]>;
}


//===---------------------------------------------------------------------===//
// Generic Float Instructions
//===---------------------------------------------------------------------===//
//===---------------------------------------------------------------------===//
// float math instructions start here
//===---------------------------------------------------------------------===//
let mayLoad=0, mayStore=0 in {
defm ABS : UnaryIntrinsicFloat<IL_OP_ABS, int_AMDIL_fabs>;
defm PIREDUCE : UnaryIntrinsicFloat<IL_OP_PI_REDUCE, int_AMDIL_pireduce>;
defm ROUND_NEGINF : UnaryIntrinsicFloat<IL_OP_ROUND_NEG_INF,
          int_AMDIL_round_neginf>;
defm ROUND_ZERO : UnaryIntrinsicFloat<IL_OP_ROUND_ZERO,
          int_AMDIL_round_zero>;
defm ACOS : UnaryIntrinsicFloatScalar<IL_OP_ACOS, int_AMDIL_acos>;
defm ATAN : UnaryIntrinsicFloatScalar<IL_OP_ATAN, int_AMDIL_atan>;
defm ASIN : UnaryIntrinsicFloatScalar<IL_OP_ASIN, int_AMDIL_asin>;
defm TAN : UnaryIntrinsicFloatScalar<IL_OP_TAN, int_AMDIL_tan>;
defm SIN : UnaryIntrinsicFloatScalar<IL_OP_SIN, int_AMDIL_sin>;
defm COS : UnaryIntrinsicFloatScalar<IL_OP_COS, int_AMDIL_cos>;
defm SQRT : UnaryIntrinsicFloatScalar<IL_OP_SQRT, int_AMDIL_sqrt>;
defm EXP : UnaryIntrinsicFloatScalar<IL_OP_EXP, int_AMDIL_exp>;
defm EXPVEC : UnaryIntrinsicFloat<IL_OP_EXP_VEC, int_AMDIL_exp_vec>;
defm SQRTVEC : UnaryIntrinsicFloat<IL_OP_SQRT_VEC, int_AMDIL_sqrt_vec>;
defm COSVEC : UnaryIntrinsicFloat<IL_OP_COS_VEC, int_AMDIL_cos_vec>;
defm SINVEC : UnaryIntrinsicFloat<IL_OP_SIN_VEC, int_AMDIL_sin_vec>;
defm LOGVEC : UnaryIntrinsicFloat<IL_OP_LOG_VEC, int_AMDIL_log_vec>;
defm RSQVEC : UnaryIntrinsicFloat<IL_OP_RSQ_VEC, int_AMDIL_rsq_vec>;
defm EXN : UnaryIntrinsicFloatScalar<IL_OP_EXN, int_AMDIL_exn>;
defm SIGN : UnaryIntrinsicFloat<IL_OP_SGN, int_AMDIL_sign>;
defm LENGTH : UnaryIntrinsicFloat<IL_OP_LEN, int_AMDIL_length>;
defm POW : BinaryIntrinsicFloat<IL_OP_POW, int_AMDIL_pow>;
}

let hasIEEEFlag = 1 in {
  let mayLoad = 0, mayStore=0 in {
  }
defm MOD  : BinaryOpMCf32<IL_OP_MOD, frem>;
}
let hasZeroOpFlag = 1 in {
  let mayLoad = 0, mayStore=0 in {
defm LN  : UnaryIntrinsicFloatScalar<IL_OP_LN, int_AMDIL_ln>;
defm LOG : UnaryIntrinsicFloatScalar<IL_OP_LOG, int_AMDIL_log>;
defm RSQ : UnaryIntrinsicFloatScalar<IL_OP_RSQ, int_AMDIL_rsq>;
defm DIV  : BinaryIntrinsicFloat<IL_OP_DIV, int_AMDIL_div>;
  }
}
  let mayLoad = 0, mayStore=0 in {
defm CLAMP : TernaryIntrinsicFloat<IL_OP_CLAMP, int_AMDIL_clamp>;
defm FMA  : TernaryIntrinsicFloat<IL_OP_FMA, int_AMDIL_fma>;
defm LERP  : TernaryIntrinsicFloat<IL_OP_LERP, int_AMDIL_lerp>;
  }
defm SUB  : BinaryOpMCf32<IL_OP_SUB, fsub>;
defm FABS  : UnaryOpMCf32<IL_OP_ABS, fabs>;
defm NEAR : UnaryOpMCf32<IL_OP_ROUND_NEAR, fnearbyint>;
defm RND_Z : UnaryOpMCf32<IL_OP_ROUND_ZERO, ftrunc>;

// This opcode has custom swizzle pattern encoded in Swizzle Encoder
def NEG_f32         : OneInOneOut<IL_OP_MOV, (outs GPRF32:$dst),
  (ins GPRF32:$src0),
    !strconcat(IL_OP_MOV.Text, " $dst, $src0"),
    [(set GPRF32:$dst, (fneg GPRF32:$src0))]>;
def INTTOANY_f32    : OneInOneOut<IL_OP_MOV, (outs GPRF32:$dst),
  (ins GPRI32:$src0),
    !strconcat(IL_OP_MOV.Text, " $dst, $src0"),
    [(set GPRF32:$dst, (IL_inttoany GPRI32:$src0))]>;
let hasIEEEFlag = 1 in {
def DP2ADD_f32 : ThreeInOneOut<IL_OP_DP2_ADD, (outs GPRF32:$dst),
    (ins GPRV2F32:$src0, GPRV2F32:$src1, GPRF32:$src2),
    !strconcat(IL_OP_DP2_ADD.Text, " $dst, $src0, $src1, $src2"),
    [(set GPRF32:$dst,
    (int_AMDIL_dp2_add GPRV2F32:$src0,
    GPRV2F32:$src1, GPRF32:$src2))]>;
def DP2_f32 : TwoInOneOut<IL_OP_DP2, (outs GPRF32:$dst),
    (ins GPRV2F32:$src0, GPRV2F32:$src1),
    !strconcat(IL_OP_DP2.Text, " $dst, $src0, $src1"),
    [(set GPRF32:$dst,
    (int_AMDIL_dp2 GPRV2F32:$src0, GPRV2F32:$src1))]>;
def DP3_f32 : TwoInOneOut<IL_OP_DP3, (outs GPRF32:$dst),
    (ins GPRV4F32:$src0, GPRV4F32:$src1),
    !strconcat(IL_OP_DP3.Text, " $dst, $src0, $src1"),
    [(set GPRF32:$dst,
    (int_AMDIL_dp3 GPRV4F32:$src0, GPRV4F32:$src1))]>;
def DP4_f32 : TwoInOneOut<IL_OP_DP4, (outs GPRF32:$dst),
    (ins GPRV4F32:$src0, GPRV4F32:$src1),
    !strconcat(IL_OP_DP4.Text, " $dst, $src0, $src1"),
    [(set GPRF32:$dst,
    (int_AMDIL_dp4 GPRV4F32:$src0, GPRV4F32:$src1))]>;
}
defm UNPACK_B0 : IntrConvertI32TOF32<IL_OP_UNPACK_0, int_AMDIL_media_unpack_byte_0>;
defm UNPACK_B1 : IntrConvertI32TOF32<IL_OP_UNPACK_1, int_AMDIL_media_unpack_byte_1>;
defm UNPACK_B2 : IntrConvertI32TOF32<IL_OP_UNPACK_2, int_AMDIL_media_unpack_byte_2>;
defm UNPACK_B3 : IntrConvertI32TOF32<IL_OP_UNPACK_3, int_AMDIL_media_unpack_byte_3>;
defm FTOI_FLR  : IntrConvertF32TOI32<IL_OP_FTOI_FLR, int_AMDIL_convert_f32_i32_flr>;
defm FTOI_RPI  : IntrConvertF32TOI32<IL_OP_FTOI_RPI, int_AMDIL_convert_f32_i32_rpi>;
defm HTOF      : IntrConvertF16TOF32<IL_OP_F16_TO_F32, int_AMDIL_convert_f16_f32>;
defm FTOH      : IntrConvertF32TOF16<IL_OP_F32_TO_F16, int_AMDIL_convert_f32_f16>;
defm FTOH_NEAR     : IntrConvertF32TOF16<IL_OP_F32_TO_F16_NEAR, int_AMDIL_convert_f32_f16_near>;
defm FTOH_NEG_INF  : IntrConvertF32TOF16<IL_OP_F32_TO_F16_NEG_INF, int_AMDIL_convert_f32_f16_neg_inf>;
defm FTOH_PLUS_INF : IntrConvertF32TOF16<IL_OP_F32_TO_F16_PLUS_INF, int_AMDIL_convert_f32_f16_plus_inf>;
//===---------------------------------------------------------------------===//
// float math instructions end here
//===---------------------------------------------------------------------===//

//===---------------------------------------------------------------------===//
// float2 math instructions start here
//===---------------------------------------------------------------------===//
// This opcode has custom swizzle pattern encoded in Swizzle Encoder
def NEG_v2f32       : OneInOneOut<IL_OP_MOV, (outs GPRV2F32:$dst),
  (ins GPRV2F32:$src0),
    !strconcat(IL_OP_MOV.Text, " $dst, $src0"),
    [(set GPRV2F32:$dst, (fneg GPRV2F32:$src0))]>;
//===---------------------------------------------------------------------===//
// float2 math instructions end here
//===---------------------------------------------------------------------===//

//===---------------------------------------------------------------------===//
// float4 math instructions start here
//===---------------------------------------------------------------------===//
// This opcode has custom swizzle pattern encoded in Swizzle Encoder
def NEG_v4f32 : OneInOneOut<IL_OP_MOV, (outs GPRV4F32:$dst),
  (ins GPRV4F32:$src0),
    !strconcat(IL_OP_MOV.Text, " $dst, $src0"),
    [(set GPRV4F32:$dst, (fneg GPRV4F32:$src0))]>;
//===---------------------------------------------------------------------===//
// float4 math instructions end here
//===---------------------------------------------------------------------===//

//===---------------------------------------------------------------------===//
// double math instructions start here
//===---------------------------------------------------------------------===//
// This opcode has custom swizzle pattern encoded in Swizzle Encoder
def  SUB_f64       : TwoInOneOut<IL_OP_D_ADD, (outs GPRF64:$dst),
  (ins GPRF64:$src0, GPRF64:$src1),
     !strconcat(IL_OP_D_ADD.Text, " $dst, $src0, $src1"),
     [(set GPRF64:$dst, (fsub GPRF64:$src0, GPRF64:$src1))]>;
// This opcode has custom swizzle pattern encoded in Swizzle Encoder
def  SUB_v2f64      : TwoInOneOut<IL_OP_D_ADD, (outs GPRV2F64:$dst),
  (ins GPRV2F64:$src0, GPRV2F64:$src1),
     !strconcat(IL_OP_D_ADD.Text, " $dst, $src0, $src1"),
     [(set GPRV2F64:$dst, (fsub GPRV2F64:$src0, GPRV2F64:$src1))]>;
// This opcode has custom swizzle pattern encoded in Swizzle Encoder
def NEG_f64       : OneInOneOut<IL_OP_MOV, (outs GPRF64:$dst),
  (ins GPRF64:$src0),
    !strconcat(IL_OP_MOV.Text, " $dst, $src0"),
    [(set GPRF64:$dst, (fneg GPRF64:$src0))]>;
// This opcode has custom swizzle pattern encoded in Swizzle Encoder
def NEG_v2f64       : OneInOneOut<IL_OP_MOV, (outs GPRV2F64:$dst),
  (ins GPRV2F64:$src0),
    !strconcat(IL_OP_MOV.Text, " $dst, $src0"),
    [(set GPRV2F64:$dst, (fneg GPRV2F64:$src0))]>;
  let mayLoad = 0, mayStore=0 in {
defm MAX  : BinaryIntrinsicDouble<IL_OP_D_MAX, int_AMDIL_max>;
defm DIV  : BinaryIntrinsicDouble<IL_OP_D_DIV, int_AMDIL_div>;
defm MAD  : TernaryIntrinsicDouble<IL_OP_D_MAD, int_AMDIL_mad>;
defm DFMA : TernaryIntrinsicDouble<IL_OP_D_MAD, int_AMDIL_fma>;
defm FRAC : UnaryIntrinsicDouble<IL_OP_D_FRC, int_AMDIL_fraction>;
defm SQRT : UnaryIntrinsicDouble<IL_OP_D_SQRT, int_AMDIL_sqrt>;
defm RSQ  : UnaryIntrinsicDoubleScalar<IL_OP_D_RSQ, int_AMDIL_rsq>;
defm RCP  : UnaryIntrinsicDoubleScalar<IL_OP_D_RCP, int_AMDIL_drcp>;
defm DMAD : TernaryOpMCf64<IL_OP_D_MAD, IL_mad>;
  }
def FREXP_f64 : OneInOneOut<IL_OP_D_FREXP, (outs GPRV2I64:$dst),
      (ins GPRF64:$src),
      !strconcat(IL_OP_D_FREXP.Text," $dst, $src"),
      [(set GPRV2I64:$dst,
      (int_AMDIL_frexp_f64 GPRF64:$src))]>;
def LDEXP_f64 : TwoInOneOut<IL_OP_D_LDEXP, (outs GPRF64:$dst),
      (ins GPRF64:$src, GPRI32:$src1),
      !strconcat(IL_OP_D_LDEXP.Text, " $dst, $src, $src1"),
      [(set GPRF64:$dst,
      (int_AMDIL_ldexp GPRF64:$src, GPRI32:$src1))]>;
def LDEXP_v2f64 : TwoInOneOut<IL_OP_D_LDEXP, (outs GPRV2F64:$dst),
      (ins GPRV2F64:$src, GPRV2I32:$src1),
      !strconcat(IL_OP_D_LDEXP.Text, " $dst, $src, $src1"),
      [(set GPRV2F64:$dst,
      (int_AMDIL_ldexp GPRV2F64:$src, GPRV2I32:$src1))]>;
//===---------------------------------------------------------------------===//
// double math instructions end here
//===---------------------------------------------------------------------===//

//===---------------------------------------------------------------------===//
// Various Macros
//===---------------------------------------------------------------------===//
def MACRO__sdiv_i8   : BinaryMacro< GPRI8, GPRI8, GPRI8, sdiv>;
def MACRO__sdiv_i16  : BinaryMacro<GPRI16, GPRI16, GPRI16, sdiv>;
def MACRO__sdiv_i32  : BinaryMacro<GPRI32, GPRI32, GPRI32, sdiv>;
def MACRO__udiv_i8   : BinaryMacro< GPRI8, GPRI8, GPRI8, udiv>;
def MACRO__udiv_i16  : BinaryMacro<GPRI16, GPRI16, GPRI16, udiv>;
def MACRO__udiv_i32  : BinaryMacro<GPRI32, GPRI32, GPRI32, udiv>;
def MACRO__smod_i8   : BinaryMacro< GPRI8, GPRI8, GPRI8, srem>;
def MACRO__smod_i16  : BinaryMacro<GPRI16, GPRI16, GPRI16, srem>;
def MACRO__smod_i32  : BinaryMacro<GPRI32, GPRI32, GPRI32, srem>;
def MACRO__umod_i8   : BinaryMacro< GPRI8, GPRI8, GPRI8, urem>;
def MACRO__umod_i16  : BinaryMacro<GPRI16, GPRI16, GPRI16, urem>;
def MACRO__umod_i32  : BinaryMacro<GPRI32, GPRI32, GPRI32, urem>;
let Predicates = [HasSWDDiv] in {
  def MACRO__ddiv_f64: BinaryMacro<GPRF64, GPRF64, GPRF64, fdiv>;
}
let Predicates = [HasHWDDiv] in {
  def MACRO__ddiv_f64_fma: BinaryMacro<GPRF64, GPRF64, GPRF64, fdiv>;
}
def MACRO__ftol_i64  : UnaryMacro<GPRI64, GPRF32, fp_to_sint>;
def MACRO__ftoul_i64 : UnaryMacro<GPRI64, GPRF32, fp_to_uint>;
def MACRO__ultof_f32 : UnaryMacro<GPRF32, GPRI64, uint_to_fp>;
def MACRO__ltof_f32  : UnaryMacro<GPRF32, GPRI64, sint_to_fp>;
let Predicates = [HasSW64Mul] in {
def MACRO__mul_i64   : BinaryMacro<GPRI64, GPRI64, GPRI64, mul>;
def MACRO__mul_v2i64 : BinaryMacro<GPRV2I64, GPRV2I64, GPRV2I64, mul>;
}
let Predicates = [HasSW64DivMod] in {
def MACRO__sdiv_i64  : BinaryMacro<GPRI64, GPRI64, GPRI64, sdiv>;
def MACRO__udiv_i64  : BinaryMacro<GPRI64, GPRI64, GPRI64, udiv>;
def MACRO__smod_i64  : BinaryMacro<GPRI64, GPRI64, GPRI64, srem>;
def MACRO__umod_i64  : BinaryMacro<GPRI64, GPRI64, GPRI64, urem>;
}
let Predicates = [HasHW64DivMod] in {
  defm SDIV : BinaryOpMCi64<IL_OP_I64_DIV, sdiv>;
  defm UDIV : BinaryOpMCi64<IL_OP_U64_DIV, udiv>;
  defm SMOD : BinaryOpMCi64<IL_OP_I64_MOD, srem>;
  defm UMOD : BinaryOpMCi64<IL_OP_U64_MOD, urem>;
}
let Predicates = [HasHW64Mul] in {
  defm SMUL       : BinaryOpMCi64<IL_OP_I64_MUL, mul>;
  defm UMUL       : BinaryOpMCi64<IL_OP_U64_MUL, IL_umul>;
}
def MACRO__shr_v2i64 : BinaryMacro<GPRV2I64, GPRV2I64, GPRV2I32, srl>;
def MACRO__shl_v2i64 : BinaryMacro<GPRV2I64, GPRV2I64, GPRV2I32, shl>;
def MACRO__sra_v2i64 : BinaryMacro<GPRV2I64, GPRV2I64, GPRV2I32, sra>;

let Predicates = [HasSW64Bit] in {
def MACRO__shr_i64   : BinaryMacro<GPRI64, GPRI64, GPRI32, srl>;
def MACRO__shl_i64   : BinaryMacro<GPRI64, GPRI64, GPRI32, shl>;
def MACRO__sra_i64   : BinaryMacro<GPRI64, GPRI64, GPRI32, sra>;
}
//===---------------------------------------------------------------------===//
// 32-bit floating point operations
//===---------------------------------------------------------------------===//
def FEQ         : TwoInOneOut<IL_OP_EQ, (outs GPRF32:$dst),
        (ins GPRF32:$lhs, GPRF32:$rhs),
        !strconcat(IL_OP_EQ.Text, " $dst, $lhs, $rhs")
        , []>;
def FGE         : TwoInOneOut<IL_OP_GE, (outs GPRF32:$dst),
        (ins GPRF32:$lhs, GPRF32:$rhs),
        !strconcat(IL_OP_GE.Text, " $dst, $lhs, $rhs")
        , []>;
def FLT         : TwoInOneOut<IL_OP_LT, (outs GPRF32:$dst),
        (ins GPRF32:$lhs, GPRF32:$rhs),
        !strconcat(IL_OP_LT.Text, " $dst, $lhs, $rhs")
        , []>;
def FLT_v2f32 : TwoInOneOut<IL_OP_LT, (outs GPRV2F32:$dst),
        (ins GPRV2F32:$lhs, GPRV2F32:$rhs),
        !strconcat(IL_OP_LT.Text, " $dst, $lhs, $rhs")
        , []>;
def FLT_v4f32 : TwoInOneOut<IL_OP_LT, (outs GPRV4F32:$dst),
        (ins GPRV4F32:$lhs, GPRV4F32:$rhs),
        !strconcat(IL_OP_LT.Text, " $dst, $lhs, $rhs")
        , []>;
def FNE         : TwoInOneOut<IL_OP_NE, (outs GPRF32:$dst),
        (ins GPRF32:$lhs, GPRF32:$rhs),
        !strconcat(IL_OP_NE.Text, " $dst, $lhs, $rhs")
        , []>;

//===---------------------------------------------------------------------===//
//TODO: need to correctly define comparison instructions
//===---------------------------------------------------------------------===//
def DEQ        : TwoInOneOut<IL_OP_D_EQ, (outs GPRF64:$dst),
        (ins GPRF64:$lhs, GPRF64:$rhs),
        !strconcat(IL_OP_D_EQ.Text, " $dst, $lhs, $rhs")
        , []>;
def DEQ_v2f64        : TwoInOneOut<IL_OP_D_EQ, (outs GPRV2F64:$dst),
        (ins GPRV2F64:$lhs, GPRV2F64:$rhs),
        !strconcat(IL_OP_D_EQ.Text, " $dst, $lhs, $rhs")
        , []>;
def DGE        : TwoInOneOut<IL_OP_D_GE, (outs GPRF64:$dst),
        (ins GPRF64:$lhs, GPRF64:$rhs),
        !strconcat(IL_OP_D_GE.Text, " $dst, $lhs, $rhs")
        , []>;
def DLT        : TwoInOneOut<IL_OP_D_LT, (outs GPRF64:$dst),
        (ins GPRF64:$lhs, GPRF64:$rhs),
        !strconcat(IL_OP_D_LT.Text, " $dst, $lhs, $rhs")
        , []>;
def DNE        : TwoInOneOut<IL_OP_D_NE, (outs GPRF64:$dst),
        (ins GPRF64:$lhs, GPRF64:$rhs),
        !strconcat(IL_OP_D_NE.Text, " $dst, $lhs, $rhs")
        , []>;

//===---------------------------------------------------------------------===//
//TODO: need to correctly define comparison instructions
//===---------------------------------------------------------------------===//
def IEQ        : TwoInOneOut<IL_OP_I_EQ, (outs GPRI32:$dst),
        (ins GPRI32:$lhs, GPRI32:$rhs),
        !strconcat(IL_OP_I_EQ.Text, " $dst, $lhs, $rhs")
        , []>;
def IEQ_v2i32        : TwoInOneOut<IL_OP_I_EQ, (outs GPRV2I32:$dst),
        (ins GPRV2I32:$lhs, GPRV2I32:$rhs),
        !strconcat(IL_OP_I_EQ.Text, " $dst, $lhs, $rhs")
        , []>;
def IEQ_v4i32        : TwoInOneOut<IL_OP_I_EQ, (outs GPRV4I32:$dst),
        (ins GPRV4I32:$lhs, GPRV4I32:$rhs),
        !strconcat(IL_OP_I_EQ.Text, " $dst, $lhs, $rhs")
        , []>;
def IGE        : TwoInOneOut<IL_OP_I_GE, (outs GPRI32:$dst),
        (ins GPRI32:$lhs, GPRI32:$rhs),
        !strconcat(IL_OP_I_GE.Text, " $dst, $lhs, $rhs")
        , []>;
def IGE_v2i32        : TwoInOneOut<IL_OP_I_GE, (outs GPRV2I32:$dst),
        (ins GPRV2I32:$lhs, GPRV2I32:$rhs),
        !strconcat(IL_OP_I_GE.Text, " $dst, $lhs, $rhs")
        , []>;
def IGE_v4i32        : TwoInOneOut<IL_OP_I_GE, (outs GPRV4I32:$dst),
        (ins GPRV4I32:$lhs, GPRV4I32:$rhs),
        !strconcat(IL_OP_I_GE.Text, " $dst, $lhs, $rhs")
        , []>;
def ILT        : TwoInOneOut<IL_OP_I_LT, (outs GPRI32:$dst),
        (ins GPRI32:$lhs, GPRI32:$rhs),
        !strconcat(IL_OP_I_LT.Text, " $dst, $lhs, $rhs")
        , []>;
def ILT_v2i32        : TwoInOneOut<IL_OP_I_LT, (outs GPRV2I32:$dst),
        (ins GPRV2I32:$lhs, GPRV2I32:$rhs),
        !strconcat(IL_OP_I_LT.Text, " $dst, $lhs, $rhs")
        , []>;
def ILT_v4i32        : TwoInOneOut<IL_OP_I_LT, (outs GPRV4I32:$dst),
        (ins GPRV4I32:$lhs, GPRV4I32:$rhs),
        !strconcat(IL_OP_I_LT.Text, " $dst, $lhs, $rhs")
        , []>;
def INE        : TwoInOneOut<IL_OP_I_NE, (outs GPRI32:$dst),
        (ins GPRI32:$lhs, GPRI32:$rhs),
        !strconcat(IL_OP_I_NE.Text, " $dst, $lhs, $rhs")
        , []>;
def INE_v2i32        : TwoInOneOut<IL_OP_I_NE, (outs GPRV2I32:$dst),
        (ins GPRV2I32:$lhs, GPRV2I32:$rhs),
        !strconcat(IL_OP_I_NE.Text, " $dst, $lhs, $rhs")
        , []>;
def INE_v4i32        : TwoInOneOut<IL_OP_I_NE, (outs GPRV4I32:$dst),
        (ins GPRV4I32:$lhs, GPRV4I32:$rhs),
        !strconcat(IL_OP_I_NE.Text, " $dst, $lhs, $rhs")
        , []>;
let Predicates = [HasHW64Bit] in {
def LEQ        : TwoInOneOut<IL_OP_I64_EQ, (outs GPRI64:$dst),
        (ins GPRI64:$lhs, GPRI64:$rhs),
        !strconcat(IL_OP_I64_EQ.Text, " $dst, $lhs, $rhs")
        , []>;
def LGE        : TwoInOneOut<IL_OP_I64_GE, (outs GPRI64:$dst),
        (ins GPRI64:$lhs, GPRI64:$rhs),
        !strconcat(IL_OP_I64_GE.Text, " $dst, $lhs, $rhs")
        , []>;
def LLE        : TwoInOneOut<IL_OP_I64_GE, (outs GPRI64:$dst),
        (ins GPRI64:$lhs, GPRI64:$rhs),
        !strconcat(IL_OP_I64_GE.Text, " $dst, $rhs, $lhs")
        , []>;
def LGT        : TwoInOneOut<IL_OP_I64_LT, (outs GPRI64:$dst),
        (ins GPRI64:$lhs, GPRI64:$rhs),
        !strconcat(IL_OP_I64_LT.Text, " $dst, $rhs, $lhs")
        , []>;
def LLT        : TwoInOneOut<IL_OP_I64_LT, (outs GPRI64:$dst),
        (ins GPRI64:$lhs, GPRI64:$rhs),
        !strconcat(IL_OP_I64_LT.Text, " $dst, $lhs, $rhs")
        , []>;
def LNE        : TwoInOneOut<IL_OP_I64_NE, (outs GPRI64:$dst),
        (ins GPRI64:$lhs, GPRI64:$rhs),
        !strconcat(IL_OP_I64_NE.Text, " $dst, $lhs, $rhs")
        , []>;
}

//===---------------------------------------------------------------------===//
// Unsigned Integer Operations
//===---------------------------------------------------------------------===//

//===---------------------------------------------------------------------===//
//TODO: need to correctly define comparison instructions
//===---------------------------------------------------------------------===//
def UEQ        : TwoInOneOut<IL_OP_I_EQ, (outs GPRI32:$dst),
        (ins GPRI32:$lhs, GPRI32:$rhs),
        !strconcat(IL_OP_I_EQ.Text, " $dst, $lhs, $rhs")
        , []>;
def UEQ_v2i32        : TwoInOneOut<IL_OP_I_EQ, (outs GPRV2I32:$dst),
        (ins GPRV2I32:$lhs, GPRV2I32:$rhs),
        !strconcat(IL_OP_I_EQ.Text, " $dst, $lhs, $rhs")
        , []>;
def UEQ_v4i32        : TwoInOneOut<IL_OP_I_EQ, (outs GPRV4I32:$dst),
        (ins GPRV4I32:$lhs, GPRV4I32:$rhs),
        !strconcat(IL_OP_I_EQ.Text, " $dst, $lhs, $rhs")
        , []>;
def ULE        : TwoInOneOut<IL_OP_U_GE, (outs GPRI32:$dst),
        (ins GPRI32:$lhs, GPRI32:$rhs),
        !strconcat(IL_OP_U_GE.Text, " $dst, $lhs, $rhs")
        , []>;
def ULE_v2i32        : TwoInOneOut<IL_OP_U_GE, (outs GPRV2I32:$dst),
        (ins GPRV2I32:$lhs, GPRV2I32:$rhs),
        !strconcat(IL_OP_U_GE.Text, " $dst, $lhs, $rhs")
        , []>;
def ULE_v4i32        : TwoInOneOut<IL_OP_U_GE, (outs GPRV4I32:$dst),
        (ins GPRV4I32:$lhs, GPRV4I32:$rhs),
        !strconcat(IL_OP_U_GE.Text, " $dst, $lhs, $rhs")
        , []>;
def UGT        : TwoInOneOut<IL_OP_U_LT, (outs GPRI32:$dst),
        (ins GPRI32:$lhs, GPRI32:$rhs),
        !strconcat(IL_OP_U_LT.Text, " $dst, $lhs, $rhs")
        , []>;
def UGT_v2i32        : TwoInOneOut<IL_OP_U_LT, (outs GPRV2I32:$dst),
        (ins GPRV2I32:$lhs, GPRV2I32:$rhs),
        !strconcat(IL_OP_U_LT.Text, " $dst, $lhs, $rhs")
        , []>;
def UGT_v4i32        : TwoInOneOut<IL_OP_U_LT, (outs GPRV4I32:$dst),
        (ins GPRV4I32:$lhs, GPRV4I32:$rhs),
        !strconcat(IL_OP_U_LT.Text, " $dst, $lhs, $rhs")
        , []>;
def UGE        : TwoInOneOut<IL_OP_U_GE, (outs GPRI32:$dst),
        (ins GPRI32:$lhs, GPRI32:$rhs),
        !strconcat(IL_OP_U_GE.Text, " $dst, $lhs, $rhs")
        , []>;
def UGE_v2i32        : TwoInOneOut<IL_OP_U_GE, (outs GPRV2I32:$dst),
        (ins GPRV2I32:$lhs, GPRV2I32:$rhs),
        !strconcat(IL_OP_U_GE.Text, " $dst, $lhs, $rhs")
        , []>;
def UGE_v4i32        : TwoInOneOut<IL_OP_U_GE, (outs GPRV4I32:$dst),
        (ins GPRV4I32:$lhs, GPRV4I32:$rhs),
        !strconcat(IL_OP_U_GE.Text, " $dst, $lhs, $rhs")
        , []>;
def ULT        : TwoInOneOut<IL_OP_U_LT, (outs GPRI32:$dst),
        (ins GPRI32:$lhs, GPRI32:$rhs),
        !strconcat(IL_OP_U_LT.Text, " $dst, $lhs, $rhs")
        , []>;
def ULT_v2i32        : TwoInOneOut<IL_OP_U_LT, (outs GPRV2I32:$dst),
        (ins GPRV2I32:$lhs, GPRV2I32:$rhs),
        !strconcat(IL_OP_U_LT.Text, " $dst, $lhs, $rhs")
        , []>;
def ULT_v4i32        : TwoInOneOut<IL_OP_U_LT, (outs GPRV4I32:$dst),
        (ins GPRV4I32:$lhs, GPRV4I32:$rhs),
        !strconcat(IL_OP_U_LT.Text, " $dst, $lhs, $rhs")
        , []>;
def UNE        : TwoInOneOut<IL_OP_I_NE, (outs GPRI32:$dst),
        (ins GPRI32:$lhs, GPRI32:$rhs),
        !strconcat(IL_OP_I_NE.Text, " $dst, $lhs, $rhs")
        , []>;
def UNE_v2i32        : TwoInOneOut<IL_OP_I_NE, (outs GPRV2I32:$dst),
        (ins GPRV2I32:$lhs, GPRV2I32:$rhs),
        !strconcat(IL_OP_I_NE.Text, " $dst, $lhs, $rhs")
        , []>;
def UNE_v4i32        : TwoInOneOut<IL_OP_I_NE, (outs GPRV4I32:$dst),
        (ins GPRV4I32:$lhs, GPRV4I32:$rhs),
        !strconcat(IL_OP_I_NE.Text, " $dst, $lhs, $rhs")
        , []>;
let Predicates = [HasHW64Bit] in {
def ULLE        : TwoInOneOut<IL_OP_U64_GE, (outs GPRI64:$dst),
        (ins GPRI64:$lhs, GPRI64:$rhs),
        !strconcat(IL_OP_U64_GE.Text, " $dst, $rhs, $lhs")
        , []>;
def ULGT        : TwoInOneOut<IL_OP_U64_LT, (outs GPRI64:$dst),
        (ins GPRI64:$lhs, GPRI64:$rhs),
        !strconcat(IL_OP_U64_LT.Text, " $dst, $rhs, $lhs")
        , []>;
def ULGE        : TwoInOneOut<IL_OP_U64_GE, (outs GPRI64:$dst),
        (ins GPRI64:$lhs, GPRI64:$rhs),
        !strconcat(IL_OP_U64_GE.Text, " $dst, $lhs, $rhs")
        , []>;
def ULLT        : TwoInOneOut<IL_OP_U64_LT, (outs GPRI64:$dst),
        (ins GPRI64:$lhs, GPRI64:$rhs),
        !strconcat(IL_OP_U64_LT.Text, " $dst, $lhs, $rhs")
        , []>;
}
//===---------------------------------------------------------------------===//
// Scalar ==> Scalar conversion functions
//===---------------------------------------------------------------------===//
// f32 ==> f64
def FTOD        : UnaryOp<IL_OP_F_2_D,         fextend,     GPRF64, GPRF32>;
// f64 ==> f32
def DTOF        : UnaryOp<IL_OP_D_2_F,         IL_d2f,     GPRF32, GPRF64>;
// f32 ==> i32 signed
def FTOI        : UnaryOp<IL_OP_FTOI,          fp_to_sint, GPRI32, GPRF32>;
def FTOI_v2i32  : UnaryOp<IL_OP_FTOI,          fp_to_sint, GPRV2I32, GPRV2F32>;
def FTOI_v4i32  : UnaryOp<IL_OP_FTOI,          fp_to_sint, GPRV4I32, GPRV4F32>;
// i32 ==> f32 signed
def ITOF        : UnaryOp<IL_OP_ITOF,          sint_to_fp, GPRF32, GPRI32>;
def ITOF_v2f32  : UnaryOp<IL_OP_ITOF,          sint_to_fp, GPRV2F32, GPRV2I32>;
def ITOF_v4f32  : UnaryOp<IL_OP_ITOF,          sint_to_fp, GPRV4F32, GPRV4I32>;
// f32 ==> i32 unsigned
def FTOU        : UnaryOp<IL_OP_FTOU,          fp_to_uint, GPRI32, GPRF32>;
def FTOU_v2i32  : UnaryOp<IL_OP_FTOU,          fp_to_uint, GPRV2I32, GPRV2F32>;
def FTOU_v4i32  : UnaryOp<IL_OP_FTOU,          fp_to_uint, GPRV4I32, GPRV4F32>;
// i32 ==> f32 unsigned
def UTOF        : UnaryOp<IL_OP_UTOF,          uint_to_fp, GPRF32, GPRI32>;
def UTOF_v2f32  : UnaryOp<IL_OP_UTOF,          uint_to_fp, GPRV2F32, GPRV2I32>;
def UTOF_v4f32  : UnaryOp<IL_OP_UTOF,          uint_to_fp, GPRV4F32, GPRV4I32>;
// Get upper 32 bits of f64
// This opcode has custom swizzle pattern encoded in Swizzle Encoder
def DHI         : OneInOneOut<IL_OP_MOV,  (outs GPRI32:$dst), 
                (ins GPRF64:$src),
                !strconcat(IL_OP_MOV.Text, " $dst, $src"),
                [(set GPRI32:$dst, (IL_dcomphi GPRF64:$src))]>;
// This opcode has custom swizzle pattern encoded in Swizzle Encoder
def DHI_v2f64   : OneInOneOut<IL_OP_MOV,  (outs GPRV2I32:$dst),
                (ins GPRV2F64:$src),
                !strconcat(IL_OP_MOV.Text, " $dst, $src"),
                [(set GPRV2I32:$dst, (IL_dcomphi2 GPRV2F64:$src))]>;
// Get lower 32 bits of f64
// This opcode has custom swizzle pattern encoded in Swizzle Encoder
def DLO         : OneInOneOut<IL_OP_MOV,  (outs GPRI32:$dst), 
                (ins GPRF64:$src),
                !strconcat(IL_OP_MOV.Text, " $dst, $src"),
                [(set GPRI32:$dst, (IL_dcomplo GPRF64:$src))]>;
// This opcode has custom swizzle pattern encoded in Swizzle Encoder
def DLO_v2f64   : OneInOneOut<IL_OP_MOV,  (outs GPRV2I32:$dst),
                (ins GPRV2F64:$src),
                !strconcat(IL_OP_MOV.Text, " $dst, $src"),
                [(set GPRV2I32:$dst, (IL_dcomplo2 GPRV2F64:$src))]>;
// Convert two 32 bit integers into a f64
// This opcode has custom swizzle pattern encoded in Swizzle Encoder
def DCREATE     : TwoInOneOut<IL_OP_I_ADD, (outs GPRF64:$dst),
                (ins GPRI32:$src0, GPRI32:$src1),
                !strconcat(IL_OP_I_ADD.Text, " $dst, $src0, $src1"),
                [(set GPRF64:$dst, (IL_dcreate GPRI32:$src0, GPRI32:$src1))]>;
// This opcode has custom swizzle pattern encoded in Swizzle Encoder
def DCREATE_v2f64 : TwoInOneOut<IL_OP_I_ADD, (outs GPRV2F64:$dst),
                (ins GPRV2I32:$src0, GPRV2I32:$src1),
                !strconcat(IL_OP_I_ADD.Text, " $dst, $src0, $src1"),
                [(set GPRV2F64:$dst,
                    (IL_dcreate2 GPRV2I32:$src0, GPRV2I32:$src1))]>;
// Get upper 32 bits of i64
// This opcode has custom swizzle pattern encoded in Swizzle Encoder
def LHI         : OneInOneOut<IL_OP_MOV,  (outs GPRI32:$dst), 
                (ins GPRI64:$src),
                !strconcat(IL_OP_MOV.Text, " $dst, $src"),
                [(set GPRI32:$dst, (IL_lcomphi GPRI64:$src))]>;
// This opcode has custom swizzle pattern encoded in Swizzle Encoder
def LHI_v2i64         : OneInOneOut<IL_OP_MOV,  (outs GPRV2I32:$dst),
                (ins GPRV2I64:$src),
                !strconcat(IL_OP_MOV.Text, " $dst, $src"),
                [(set GPRV2I32:$dst, (IL_lcomphi2 GPRV2I64:$src))]>;
// Get lower 32 bits of i64
// This opcode has custom swizzle pattern encoded in Swizzle Encoder
def LLO         : OneInOneOut<IL_OP_MOV,  (outs GPRI32:$dst), 
                (ins GPRI64:$src),
                !strconcat(IL_OP_MOV.Text, " $dst, $src"),
                [(set GPRI32:$dst, (IL_lcomplo GPRI64:$src))]>;
// This opcode has custom swizzle pattern encoded in Swizzle Encoder
def LLO_v2i64         : OneInOneOut<IL_OP_MOV,  (outs GPRV2I32:$dst), 
                (ins GPRV2I64:$src),
                !strconcat(IL_OP_MOV.Text, " $dst, $src"),
                [(set GPRV2I32:$dst, (IL_lcomplo2 GPRV2I64:$src))]>;
// This opcode has custom swizzle pattern encoded in Swizzle Encoder
def HILO_BITOR_v4i16 : TwoInOneOut<IL_OP_I_OR, (outs GPRI32:$dst), 
                (ins GPRI32:$src, GPRI32:$src2),
                !strconcat(IL_OP_I_OR.Text, " $dst, $src, $src2"), []>;
// This opcode has custom swizzle pattern encoded in Swizzle Encoder
def HILO_BITOR_v2i32 : TwoInOneOut<IL_OP_I_OR, (outs GPRI32:$dst), 
                (ins GPRI32:$src, GPRI32:$src2),
                !strconcat(IL_OP_I_OR.Text, " $dst, $src, $src2"), []>;
// This opcode has custom swizzle pattern encoded in Swizzle Encoder
def HILO_BITOR_v2i64 : TwoInOneOut<IL_OP_I_OR, (outs GPRI64:$dst), 
                (ins GPRI64:$src, GPRI64:$src2),
                !strconcat(IL_OP_I_OR.Text, " $dst, $src, $src2"), []>;
// Convert two 32 bit integers into a i64
// This opcode has custom swizzle pattern encoded in Swizzle Encoder
def LCREATE     : TwoInOneOut<IL_OP_I_ADD, (outs GPRI64:$dst), 
                (ins GPRI32:$src0, GPRI32:$src1),
                !strconcat(IL_OP_I_ADD.Text, " $dst, $src0, $src1"),
                [(set GPRI64:$dst, (IL_lcreate GPRI32:$src0, GPRI32:$src1))]>;
// This opcode has custom swizzle pattern encoded in Swizzle Encoder
def LCREATE_v2i64     : TwoInOneOut<IL_OP_I_ADD, (outs GPRV2I64:$dst), 
                (ins GPRV2I32:$src0, GPRV2I32:$src1),
                !strconcat(IL_OP_I_ADD.Text, " $dst, $src0, $src1"),
                [(set GPRV2I64:$dst, 
                    (IL_lcreate2 GPRV2I32:$src0, GPRV2I32:$src1))]>;
//===---------------------------------------------------------------------===//
// Scalar ==> Vector conversion functions
//===---------------------------------------------------------------------===//
// This opcode has custom swizzle pattern encoded in Swizzle Encoder
defm VCREATE          : UnaryOpMCVec<IL_OP_MOV, IL_vbuild>;

//===---------------------------------------------------------------------===//
// Vector ==> Scalar conversion functions
//===---------------------------------------------------------------------===//

// This opcode has custom swizzle pattern encoded in Swizzle Encoder
defm VEXTRACT         : VectorExtract<IL_vextract>;

//===---------------------------------------------------------------------===//
// Vector ==> Vector conversion functions
//===---------------------------------------------------------------------===//
// This opcode has custom swizzle pattern encoded in Swizzle Encoder
defm VINSERT          : VectorInsert<IL_vinsert>;
// This opcode has custom swizzle pattern encoded in Swizzle Encoder
defm VCONCAT      : VectorConcat<IL_vconcat>;

//===---------------------------------------------------------------------===//
// Bit conversion functions
//===---------------------------------------------------------------------===//
defm IL_ASCHAR  : BitConversion<IL_OP_MOV, GPRI8, IL_bitconv>;
defm IL_ASSHORT : BitConversion<IL_OP_MOV, GPRI16, IL_bitconv>;
defm IL_ASINT   : BitConversion<IL_OP_MOV, GPRI32, IL_bitconv>;
defm IL_ASFLOAT : BitConversion<IL_OP_MOV, GPRF32, IL_bitconv>;
defm IL_ASDOUBLE : BitConversion<IL_OP_MOV, GPRF64, IL_bitconv>;
defm IL_ASLONG  : BitConversion<IL_OP_MOV, GPRI64, IL_bitconv>;
defm IL_ASV2CHAR  : BitConversion<IL_OP_MOV, GPRV2I8, IL_bitconv>;
defm IL_ASV2SHORT : BitConversion<IL_OP_MOV, GPRV2I16, IL_bitconv>;
defm IL_ASV2INT   : BitConversion<IL_OP_MOV, GPRV2I32, IL_bitconv>;
defm IL_ASV2FLOAT : BitConversion<IL_OP_MOV, GPRV2F32, IL_bitconv>;
defm IL_ASV2DOUBLE : BitConversion<IL_OP_MOV, GPRV2F64, IL_bitconv>;
defm IL_ASV2LONG  : BitConversion<IL_OP_MOV, GPRV2I64, IL_bitconv>;
defm IL_ASV4CHAR  : BitConversion<IL_OP_MOV, GPRV4I8, IL_bitconv>;
defm IL_ASV4SHORT : BitConversion<IL_OP_MOV, GPRV4I16, IL_bitconv>;
defm IL_ASV4INT   : BitConversion<IL_OP_MOV, GPRV4I32, IL_bitconv>;
defm IL_ASV4FLOAT : BitConversion<IL_OP_MOV, GPRV4F32, IL_bitconv>;

let Predicates = [Has32BitPtr] in {
  let isCodeGenOnly=1 in {
    //===----------------------------------------------------------------------===//
    // Store Memory Operations
    //===----------------------------------------------------------------------===//
    defm GLOBALTRUNCSTORE   : GTRUNCSTORE<"!global trunc store">;
    defm GLOBALSTORE        : STORE<"!global store"         , global_store>;
    defm LOCALTRUNCSTORE    : LTRUNCSTORE<"!local trunc store">;
    defm LOCALSTORE         : STORE<"!local store"          , local_store>;
    defm PRIVATETRUNCSTORE  : PTRUNCSTORE<"!private trunc store">;
    defm PRIVATESTORE       : STORE<"!private store"        , private_store>;
    defm REGIONTRUNCSTORE   : RTRUNCSTORE<"!region trunc store">;
    defm REGIONSTORE        : STORE<"!region hw store"      , region_store>;


    //===---------------------------------------------------------------------===//
    // Load Memory Operations
    //===---------------------------------------------------------------------===//
    defm GLOBALLOAD         : LOAD<"!global load"            , global_load>;
    defm GLOBALZEXTLOAD     : LOAD<"!global zext load"       , global_zext_load>;
    defm GLOBALSEXTLOAD     : LOAD<"!global sext load"       , global_sext_load>;
    defm GLOBALAEXTLOAD     : LOAD<"!global aext load"       , global_aext_load>;
    defm PRIVATELOAD        : LOAD<"!private load"           , private_load>;
    defm PRIVATEZEXTLOAD    : LOAD<"!private zext load"      , private_zext_load>;
    defm PRIVATESEXTLOAD    : LOAD<"!private sext load"      , private_sext_load>;
    defm PRIVATEAEXTLOAD    : LOAD<"!private aext load"      , private_aext_load>;
    defm CPOOLLOAD          : LOAD<"!constant pool load"     , cp_load>;
    defm CPOOLZEXTLOAD      : LOAD<"!constant pool zext load", cp_zext_load>;
    defm CPOOLSEXTLOAD      : LOAD<"!constant pool sext load", cp_sext_load>;
    defm CPOOLAEXTLOAD      : LOAD<"!constant aext pool load", cp_aext_load>;
    defm CONSTANTLOAD       : LOAD<"!constant load"          , constant_load>;
    defm CONSTANTZEXTLOAD   : LOAD<"!constant zext load"     , constant_zext_load>;
    defm CONSTANTSEXTLOAD   : LOAD<"!constant sext load"     , constant_sext_load>;
    defm CONSTANTAEXTLOAD   : LOAD<"!constant aext load"     , constant_aext_load>;
    defm LOCALLOAD          : LOAD<"!local load"             , local_load>;
    defm LOCALZEXTLOAD      : LOAD<"!local zext load"        , local_zext_load>;
    defm LOCALSEXTLOAD      : LOAD<"!local sext load"        , local_sext_load>;
    defm LOCALAEXTLOAD      : LOAD<"!local aext load"        , local_aext_load>;
    defm REGIONLOAD         : LOAD<"!region load"            , region_load>;
    defm REGIONZEXTLOAD     : LOAD<"!region zext load"       , region_zext_load>;
    defm REGIONSEXTLOAD     : LOAD<"!region sext load"       , region_sext_load>;
    defm REGIONAEXTLOAD     : LOAD<"!region aext load"       , region_aext_load>;
  }


  //===---------------------------------------------------------------------===//
  // IO Expansion Load/Store Instructions
  //===---------------------------------------------------------------------===//
  let mayLoad = 1 in {
    // This opcode has custom swizzle patterns for some of the arguments.
    def SCRATCHLOAD : TwoInOneOut<IL_OP_MOV, (outs GPRV4I32:$dst),
        (ins GPRI32:$addy, i32imm:$id),
        !strconcat(IL_OP_MOV.Text, " $dst, x$id[$addy]"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def CBLOAD : TwoInOneOut<IL_OP_MOV, (outs GPRV4I32:$dst),
        (ins GPRI32:$addy, i32imm:$id),
        !strconcat(IL_OP_MOV.Text, " $dst, cb$id[$addy]"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def GDSLOAD : TwoInOneOut<IL_OP_GDS_LOAD, (outs GPRI32:$dst),
        (ins GPRI32:$addy, i32imm:$id),
        !strconcat(IL_OP_GDS_LOAD.Text, "_id($id) $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def GDSLOAD_Y : TwoInOneOut<IL_OP_GDS_LOAD, (outs GPRI32:$dst),
        (ins GPRI32:$addy, i32imm:$id),
        !strconcat(IL_OP_GDS_LOAD.Text, "_id($id) $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def GDSLOAD_Z : TwoInOneOut<IL_OP_GDS_LOAD, (outs GPRI32:$dst),
        (ins GPRI32:$addy, i32imm:$id),
        !strconcat(IL_OP_GDS_LOAD.Text, "_id($id) $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def GDSLOAD_W : TwoInOneOut<IL_OP_GDS_LOAD, (outs GPRI32:$dst),
        (ins GPRI32:$addy, i32imm:$id),
        !strconcat(IL_OP_GDS_LOAD.Text, "_id($id) $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def LDSLOADVEC : TwoInOneOut<IL_OP_LDS_LOAD_VEC, (outs GPRI32:$dst),
        (ins GPRI32:$addy, i32imm:$id),
        !strconcat(IL_OP_LDS_LOAD_VEC.Text, "_id($id) $dst, $addy, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def LDSLOADVEC_v2i32 : TwoInOneOut<IL_OP_LDS_LOAD_VEC, (outs GPRV2I32:$dst),
        (ins GPRI32:$addy, i32imm:$id),
        !strconcat(IL_OP_LDS_LOAD_VEC.Text, "_id($id) $dst, $addy, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def LDSLOADVEC_v4i32 : TwoInOneOut<IL_OP_LDS_LOAD_VEC, (outs GPRV4I32:$dst),
        (ins GPRI32:$addy, i32imm:$id),
        !strconcat(IL_OP_LDS_LOAD_VEC.Text, "_id($id) $dst, $addy, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def LDSLOAD : TwoInOneOut<IL_OP_LDS_LOAD, (outs GPRI32:$dst),
        (ins GPRI32:$addy, i32imm:$id),
        !strconcat(IL_OP_LDS_LOAD.Text, "_id($id) $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def LDSLOAD_i8 : TwoInOneOut<IL_OP_LDS_LOAD_BYTE, (outs GPRI32:$dst),
        (ins GPRI32:$addy, i32imm:$id),
        !strconcat(IL_OP_LDS_LOAD_BYTE.Text, "_id($id) $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def LDSLOAD_u8 : TwoInOneOut<IL_OP_LDS_LOAD_UBYTE, (outs GPRI32:$dst),
        (ins GPRI32:$addy, i32imm:$id),
        !strconcat(IL_OP_LDS_LOAD_UBYTE.Text, "_id($id) $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def LDSLOAD_i16 : TwoInOneOut<IL_OP_LDS_LOAD_SHORT, (outs GPRI32:$dst),
        (ins GPRI32:$addy, i32imm:$id),
        !strconcat(IL_OP_LDS_LOAD_SHORT.Text, "_id($id) $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def LDSLOAD_u16 : TwoInOneOut<IL_OP_LDS_LOAD_USHORT, (outs GPRI32:$dst),
        (ins GPRI32:$addy, i32imm:$id),
        !strconcat(IL_OP_LDS_LOAD_USHORT.Text, "_id($id) $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def LDSLOAD_Y : TwoInOneOut<IL_OP_LDS_LOAD, (outs GPRI32:$dst),
        (ins GPRI32:$addy, i32imm:$id),
        !strconcat(IL_OP_LDS_LOAD.Text, "_id($id) $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def LDSLOAD_Z : TwoInOneOut<IL_OP_LDS_LOAD, (outs GPRI32:$dst),
        (ins GPRI32:$addy, i32imm:$id),
        !strconcat(IL_OP_LDS_LOAD.Text, "_id($id) $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def LDSLOAD_W : TwoInOneOut<IL_OP_LDS_LOAD, (outs GPRI32:$dst),
        (ins GPRI32:$addy, i32imm:$id),
        !strconcat(IL_OP_LDS_LOAD.Text, "_id($id) $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def UAVARENALOAD_i8 : TwoInOneOut<IL_OP_ARENA_UAV_LOAD, (outs GPRI32:$dst),
        (ins GPRI32:$addy, i32imm:$id),
        !strconcat(IL_OP_ARENA_UAV_LOAD.Text, "_id($id)_size(byte) $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def UAVARENALOAD_i16 : TwoInOneOut<IL_OP_ARENA_UAV_LOAD, (outs GPRI32:$dst),
        (ins GPRI32:$addy, i32imm:$id),
        !strconcat(IL_OP_ARENA_UAV_LOAD.Text, "_id($id)_size(short) $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def UAVARENALOAD_i32 : TwoInOneOut<IL_OP_ARENA_UAV_LOAD, (outs GPRI32:$dst),
        (ins GPRI32:$addy, i32imm:$id),
        !strconcat(IL_OP_ARENA_UAV_LOAD.Text, "_id($id)_size(dword) $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def UAVARENALOAD_Y_i32 : TwoInOneOut<IL_OP_ARENA_UAV_LOAD, (outs GPRI32:$dst),
        (ins GPRI32:$addy, i32imm:$id),
        !strconcat(IL_OP_ARENA_UAV_LOAD.Text, "_id($id)_size(dword) $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def UAVARENALOAD_Z_i32 : TwoInOneOut<IL_OP_ARENA_UAV_LOAD, (outs GPRI32:$dst),
        (ins GPRI32:$addy, i32imm:$id),
        !strconcat(IL_OP_ARENA_UAV_LOAD.Text, "_id($id)_size(dword) $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def UAVARENALOAD_W_i32 : TwoInOneOut<IL_OP_ARENA_UAV_LOAD, (outs GPRI32:$dst),
        (ins GPRI32:$addy, i32imm:$id),
        !strconcat(IL_OP_ARENA_UAV_LOAD.Text, "_id($id)_size(dword) $dst, $addy"), []>;
   // This opcode has custom swizzle patterns for some of the arguments.
    def UAVRAWLOAD_i32 : TwoInOneOut<IL_OP_RAW_UAV_LOAD, (outs GPRI32:$dst),
        (ins GPRI32:$addy, i32imm:$id),
        !strconcat(IL_OP_RAW_UAV_LOAD.Text, "_id($id) $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def UAVRAWLOAD_v2i32 : TwoInOneOut<IL_OP_RAW_UAV_LOAD, (outs GPRV2I32:$dst),
        (ins GPRI32:$addy, i32imm:$id),
        !strconcat(IL_OP_RAW_UAV_LOAD.Text, "_id($id) $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def UAVRAWLOAD_v4i32 : TwoInOneOut<IL_OP_RAW_UAV_LOAD, (outs GPRV4I32:$dst),
        (ins GPRI32:$addy, i32imm:$id),
        !strconcat(IL_OP_RAW_UAV_LOAD.Text, "_id($id) $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def UAVRAWLOADCACHED_i32 : TwoInOneOut<IL_OP_RAW_UAV_LOAD, (outs GPRI32:$dst),
        (ins GPRI32:$addy, i32imm:$id),
        !strconcat(IL_OP_RAW_UAV_LOAD.Text, "_id($id)_cached $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def UAVRAWLOADCACHED_v2i32 : TwoInOneOut<IL_OP_RAW_UAV_LOAD, (outs GPRV2I32:$dst),
        (ins GPRI32:$addy, i32imm:$id),
        !strconcat(IL_OP_RAW_UAV_LOAD.Text, "_id($id)_cached $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def UAVRAWLOADCACHED_v4i32 : TwoInOneOut<IL_OP_RAW_UAV_LOAD, (outs GPRV4I32:$dst),
        (ins GPRI32:$addy, i32imm:$id),
        !strconcat(IL_OP_RAW_UAV_LOAD.Text, "_id($id)_cached $dst, $addy"), []>;
  }
  let mayStore = 1 in {
    // This opcode has custom swizzle patterns for some of the arguments.
    def SCRATCHSTORE : TwoInOneOut<IL_OP_MOV, (outs GPRI32:$addy),
        (ins GPRV4I32:$data, i32imm:$id),
        !strconcat(IL_OP_MOV.Text, " x$id[$addy], $data"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def SCRATCHSTORE_X : TwoInOneOut<IL_OP_MOV, (outs GPRI32:$addy),
        (ins GPRI32:$data, i32imm:$id),
        !strconcat(IL_OP_MOV.Text, " x$id[$addy].x___, $data"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def SCRATCHSTORE_Y : TwoInOneOut<IL_OP_MOV, (outs GPRI32:$addy),
        (ins GPRI32:$data, i32imm:$id),
        !strconcat(IL_OP_MOV.Text, " x$id[$addy]._y__, $data"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def SCRATCHSTORE_Z : TwoInOneOut<IL_OP_MOV, (outs GPRI32:$addy),
        (ins GPRI32:$data, i32imm:$id),
        !strconcat(IL_OP_MOV.Text, " x$id[$addy].__z_, $data"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def SCRATCHSTORE_W : TwoInOneOut<IL_OP_MOV, (outs GPRI32:$addy),
        (ins GPRI32:$data, i32imm:$id),
        !strconcat(IL_OP_MOV.Text, " x$id[$addy].___w, $data"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def SCRATCHSTORE_XY : TwoInOneOut<IL_OP_MOV, (outs GPRI32:$addy),
        (ins GPRV2I32:$data, i32imm:$id),
        !strconcat(IL_OP_MOV.Text, " x$id[$addy].xy__, $data"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def SCRATCHSTORE_ZW : TwoInOneOut<IL_OP_MOV, (outs GPRI32:$addy),
        (ins GPRV2I32:$data, i32imm:$id),
        !strconcat(IL_OP_MOV.Text, " x$id[$addy].__zw, $data"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def GDSSTORE : TwoInOneOut<IL_OP_GDS_STORE, (outs GPRI32:$addy),
        (ins GPRI32:$src, i32imm:$id),
        !strconcat(IL_OP_GDS_STORE.Text, "_id($id) $addy, $src"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def GDSSTORE_Y : TwoInOneOut<IL_OP_GDS_STORE, (outs GPRI32:$addy),
        (ins GPRI32:$src, i32imm:$id),
        !strconcat(IL_OP_GDS_STORE.Text, "_id($id) $addy, $src"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def GDSSTORE_Z : TwoInOneOut<IL_OP_GDS_STORE, (outs GPRI32:$addy),
        (ins GPRI32:$src, i32imm:$id),
        !strconcat(IL_OP_GDS_STORE.Text, "_id($id) $addy, $src"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def GDSSTORE_W : TwoInOneOut<IL_OP_GDS_STORE, (outs GPRI32:$addy),
        (ins GPRI32:$src, i32imm:$id),
        !strconcat(IL_OP_GDS_STORE.Text, "_id($id) $addy, $src"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def LDSSTOREVEC : ThreeInOneOut<IL_OP_LDS_STORE_VEC, (outs GPRI32:$mem),
        (ins GPRI32:$addy, GPRI32:$src, i32imm:$id),
        !strconcat(IL_OP_LDS_STORE_VEC.Text, "_id($id) $mem, $addy, $src, $src"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def LDSSTOREVEC_v2i32 : ThreeInOneOut<IL_OP_LDS_STORE_VEC, (outs GPRV2I32:$mem),
        (ins GPRI32:$addy, GPRV2I32:$src, i32imm:$id),
        !strconcat(IL_OP_LDS_STORE_VEC.Text, "_id($id) $mem, $addy, $src, $src"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def LDSSTOREVEC_v4i32 : ThreeInOneOut<IL_OP_LDS_STORE_VEC, (outs GPRV4I32:$mem),
        (ins GPRI32:$addy, GPRV4I32:$src, i32imm:$id),
        !strconcat(IL_OP_LDS_STORE_VEC.Text, "_id($id) $mem, $addy, $src, $src"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def LDSSTORE : TwoInOneOut<IL_OP_LDS_STORE, (outs GPRI32:$addy),
        (ins GPRI32:$src, i32imm:$id),
        !strconcat(IL_OP_LDS_STORE.Text, "_id($id) $addy, $src"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def LDSSTORE_i8 : TwoInOneOut<IL_OP_LDS_STORE_BYTE, (outs GPRI32:$addy),
        (ins GPRI32:$src, i32imm:$id),
        !strconcat(IL_OP_LDS_STORE_BYTE.Text, "_id($id) $addy, $src"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def LDSSTORE_i16 : TwoInOneOut<IL_OP_LDS_STORE_SHORT, (outs GPRI32:$addy),
        (ins GPRI32:$src, i32imm:$id),
        !strconcat(IL_OP_LDS_STORE_SHORT.Text, "_id($id) $addy, $src"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def LDSSTORE_Y : TwoInOneOut<IL_OP_LDS_STORE, (outs GPRI32:$addy),
        (ins GPRI32:$src, i32imm:$id),
        !strconcat(IL_OP_LDS_STORE.Text, "_id($id) $addy, $src"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def LDSSTORE_Z : TwoInOneOut<IL_OP_LDS_STORE, (outs GPRI32:$addy),
        (ins GPRI32:$src, i32imm:$id),
        !strconcat(IL_OP_LDS_STORE.Text, "_id($id) $addy, $src"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def LDSSTORE_W : TwoInOneOut<IL_OP_LDS_STORE, (outs GPRI32:$addy),
        (ins GPRI32:$src, i32imm:$id),
        !strconcat(IL_OP_LDS_STORE.Text, "_id($id) $addy, $src"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def UAVARENASTORE_i8 : TwoInOneOut<IL_OP_ARENA_UAV_STORE, (outs GPRI32:$addy),
        (ins GPRI8:$src, i32imm:$id),
        !strconcat(IL_OP_ARENA_UAV_STORE.Text, 
            "_id($id)_size(byte) $addy, $src"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def UAVARENASTORE_i16 : TwoInOneOut<IL_OP_ARENA_UAV_STORE, (outs GPRI32:$addy),
        (ins GPRI16:$src, i32imm:$id),
        !strconcat(IL_OP_ARENA_UAV_STORE.Text, 
            "_id($id)_size(short) $addy, $src"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def UAVARENASTORE_i32 : TwoInOneOut<IL_OP_ARENA_UAV_STORE, (outs GPRI32:$addy),
        (ins GPRI32:$src, i32imm:$id),
        !strconcat(IL_OP_ARENA_UAV_STORE.Text, 
            "_id($id)_size(dword) $addy, $src"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def UAVARENASTORE_Y_i32 : TwoInOneOut<IL_OP_ARENA_UAV_STORE, (outs GPRI32:$addy),
        (ins GPRI32:$src, i32imm:$id),
        !strconcat(IL_OP_ARENA_UAV_STORE.Text, 
            "_id($id)_size(dword) $addy, $src"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def UAVARENASTORE_Z_i32 : TwoInOneOut<IL_OP_ARENA_UAV_STORE, (outs GPRI32:$addy),
        (ins GPRI32:$src, i32imm:$id),
        !strconcat(IL_OP_ARENA_UAV_STORE.Text, 
            "_id($id)_size(dword) $addy, $src"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def UAVARENASTORE_W_i32 : TwoInOneOut<IL_OP_ARENA_UAV_STORE, (outs GPRI32:$addy),
        (ins GPRI32:$src, i32imm:$id),
        !strconcat(IL_OP_ARENA_UAV_STORE.Text, 
            "_id($id)_size(dword) $addy, $src"), []>;
   // This opcode has custom swizzle patterns for some of the arguments.
    def UAVRAWSTORE_i32 : TwoInOneOut<IL_OP_RAW_UAV_STORE, (outs GPRI32:$mem),
        (ins GPRI32:$addy, GPRI32:$src, i32imm:$id),
        !strconcat(IL_OP_RAW_UAV_STORE.Text, "_id($id) $mem, $addy, $src"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def UAVRAWSTORE_v2i32 : TwoInOneOut<IL_OP_RAW_UAV_STORE, (outs GPRV2I32:$mem),
        (ins GPRI32:$addy, GPRV2I32:$src, i32imm:$id),
        !strconcat(IL_OP_RAW_UAV_STORE.Text, "_id($id) $mem, $addy, $src"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def UAVRAWSTORE_v4i32 : TwoInOneOut<IL_OP_RAW_UAV_STORE, (outs GPRV4I32:$mem),
        (ins GPRI32:$addy, GPRV4I32:$src, i32imm:$id),
        !strconcat(IL_OP_RAW_UAV_STORE.Text, "_id($id) $mem, $addy, $src"), []>;
  }
}
let Predicates = [Has64BitPtr] in {
  let isCodeGenOnly=1 in {
    //===----------------------------------------------------------------------===//
    // Store Memory Operations
    //===----------------------------------------------------------------------===//
    defm GLOBALTRUNCSTORE64 : GTRUNCSTORE64<"!global trunc store">;
    defm GLOBALSTORE64      : STORE64<"!global store"         , global_store>;
    defm LOCALTRUNCSTORE64  : LTRUNCSTORE64<"!local trunc store">;
    defm LOCALSTORE64       : STORE64<"!local store"          , local_store>;
    defm PRIVATETRUNCSTORE64 : PTRUNCSTORE64<"!private trunc store">;
    defm PRIVATESTORE64     : STORE64<"!private store"        , private_store>;
    defm REGIONTRUNCSTORE64 : RTRUNCSTORE64<"!region trunc store">;
    defm REGIONSTORE64      : STORE64<"!region hw store"      , region_store>;


    //===---------------------------------------------------------------------===//
    // Load Memory Operations
    //===---------------------------------------------------------------------===//
    defm GLOBALLOAD64       : LOAD64<"!global load"            , global_load>;
    defm GLOBALZEXTLOAD64   : LOAD64<"!global zext load"       , global_zext_load>;
    defm GLOBALSEXTLOAD64   : LOAD64<"!global sext load"       , global_sext_load>;
    defm GLOBALAEXTLOAD64   : LOAD64<"!global aext load"       , global_aext_load>;
    defm PRIVATELOAD64      : LOAD64<"!private load"           , private_load>;
    defm PRIVATEZEXTLOAD64  : LOAD64<"!private zext load"      , private_zext_load>;
    defm PRIVATESEXTLOAD64  : LOAD64<"!private sext load"      , private_sext_load>;
    defm PRIVATEAEXTLOAD64  : LOAD64<"!private aext load"      , private_aext_load>;
    defm CPOOLLOAD64        : LOAD64<"!constant pool load"     , cp_load>;
    defm CPOOLZEXTLOAD64    : LOAD64<"!constant pool zext load", cp_zext_load>;
    defm CPOOLSEXTLOAD64    : LOAD64<"!constant pool sext load", cp_sext_load>;
    defm CPOOLAEXTLOAD64    : LOAD64<"!constant aext pool load", cp_aext_load>;
    defm CONSTANTLOAD64     : LOAD64<"!constant load"          , constant_load>;
    defm CONSTANTZEXTLOAD64 : LOAD64<"!constant zext load"     , constant_zext_load>;
    defm CONSTANTSEXTLOAD64 : LOAD64<"!constant sext load"     , constant_sext_load>;
    defm CONSTANTAEXTLOAD64 : LOAD64<"!constant aext load"     , constant_aext_load>;
    defm LOCALLOAD64        : LOAD64<"!local load"             , local_load>;
    defm LOCALZEXTLOAD64    : LOAD64<"!local zext load"        , local_zext_load>;
    defm LOCALSEXTLOAD64    : LOAD64<"!local sext load"        , local_sext_load>;
    defm LOCALAEXTLOAD64    : LOAD64<"!local aext load"        , local_aext_load>;
    defm REGIONLOAD64       : LOAD64<"!region load"            , region_load>;
    defm REGIONZEXTLOAD64   : LOAD64<"!region zext load"       , region_zext_load>;
    defm REGIONSEXTLOAD64   : LOAD64<"!region sext load"       , region_sext_load>;
    defm REGIONAEXTLOAD64   : LOAD64<"!region aext load"       , region_aext_load>;
  }


  //===---------------------------------------------------------------------===//
  // IO Expansion Load/Store Instructions
  //===---------------------------------------------------------------------===//
  let mayLoad = 1 in {
    // This opcode has custom swizzle patterns for some of the arguments.
    def SCRATCHLOAD64 : TwoInOneOut<IL_OP_MOV, (outs GPRV4I32:$dst),
        (ins GPRI32:$addy, i64imm:$id),
        !strconcat(IL_OP_MOV.Text, " $dst, x$id[$addy]"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def CBLOAD64 : TwoInOneOut<IL_OP_MOV, (outs GPRV4I32:$dst),
        (ins GPRI32:$addy, i64imm:$id),
        !strconcat(IL_OP_MOV.Text, " $dst, cb$id[$addy]"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def GDSLOAD64 : TwoInOneOut<IL_OP_GDS_LOAD, (outs GPRI32:$dst),
        (ins GPRI32:$addy, i64imm:$id),
        !strconcat(IL_OP_GDS_LOAD.Text, "_id($id) $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def GDSLOAD64_Y : TwoInOneOut<IL_OP_GDS_LOAD, (outs GPRI32:$dst),
        (ins GPRI32:$addy, i64imm:$id),
        !strconcat(IL_OP_GDS_LOAD.Text, "_id($id) $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def GDSLOAD64_Z : TwoInOneOut<IL_OP_GDS_LOAD, (outs GPRI32:$dst),
        (ins GPRI32:$addy, i64imm:$id),
        !strconcat(IL_OP_GDS_LOAD.Text, "_id($id) $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def GDSLOAD64_W : TwoInOneOut<IL_OP_GDS_LOAD, (outs GPRI32:$dst),
        (ins GPRI32:$addy, i64imm:$id),
        !strconcat(IL_OP_GDS_LOAD.Text, "_id($id) $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def LDSLOADVEC64 : TwoInOneOut<IL_OP_LDS_LOAD_VEC, (outs GPRI32:$dst),
        (ins GPRI32:$addy, i64imm:$id),
        !strconcat(IL_OP_LDS_LOAD_VEC.Text, "_id($id) $dst, $addy, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def LDSLOADVEC64_v2i32 : TwoInOneOut<IL_OP_LDS_LOAD_VEC, (outs GPRV2I32:$dst),
        (ins GPRI32:$addy, i64imm:$id),
        !strconcat(IL_OP_LDS_LOAD_VEC.Text, "_id($id) $dst, $addy, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def LDSLOADVEC64_v4i32 : TwoInOneOut<IL_OP_LDS_LOAD_VEC, (outs GPRV4I32:$dst),
        (ins GPRI32:$addy, i64imm:$id),
        !strconcat(IL_OP_LDS_LOAD_VEC.Text, "_id($id) $dst, $addy, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def LDSLOAD64 : TwoInOneOut<IL_OP_LDS_LOAD, (outs GPRI32:$dst),
        (ins GPRI32:$addy, i64imm:$id),
        !strconcat(IL_OP_LDS_LOAD.Text, "_id($id) $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def LDSLOAD64_i8 : TwoInOneOut<IL_OP_LDS_LOAD_BYTE, (outs GPRI32:$dst),
        (ins GPRI32:$addy, i64imm:$id),
        !strconcat(IL_OP_LDS_LOAD_BYTE.Text, "_id($id) $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def LDSLOAD64_u8 : TwoInOneOut<IL_OP_LDS_LOAD_UBYTE, (outs GPRI32:$dst),
        (ins GPRI32:$addy, i64imm:$id),
        !strconcat(IL_OP_LDS_LOAD_UBYTE.Text, "_id($id) $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def LDSLOAD64_i16 : TwoInOneOut<IL_OP_LDS_LOAD_SHORT, (outs GPRI32:$dst),
        (ins GPRI32:$addy, i64imm:$id),
        !strconcat(IL_OP_LDS_LOAD_SHORT.Text, "_id($id) $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def LDSLOAD64_u16 : TwoInOneOut<IL_OP_LDS_LOAD_USHORT, (outs GPRI32:$dst),
        (ins GPRI32:$addy, i64imm:$id),
        !strconcat(IL_OP_LDS_LOAD_USHORT.Text, "_id($id) $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def LDSLOAD64_Y : TwoInOneOut<IL_OP_LDS_LOAD, (outs GPRI32:$dst),
        (ins GPRI32:$addy, i64imm:$id),
        !strconcat(IL_OP_LDS_LOAD.Text, "_id($id) $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def LDSLOAD64_Z : TwoInOneOut<IL_OP_LDS_LOAD, (outs GPRI32:$dst),
        (ins GPRI32:$addy, i64imm:$id),
        !strconcat(IL_OP_LDS_LOAD.Text, "_id($id) $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def LDSLOAD64_W : TwoInOneOut<IL_OP_LDS_LOAD, (outs GPRI32:$dst),
        (ins GPRI32:$addy, i64imm:$id),
        !strconcat(IL_OP_LDS_LOAD.Text, "_id($id) $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def UAVARENALOAD64_i8 : TwoInOneOut<IL_OP_ARENA_UAV_LOAD, (outs GPRI32:$dst),
        (ins GPRI32:$addy, i64imm:$id),
        !strconcat(IL_OP_ARENA_UAV_LOAD.Text, "_id($id)_size(byte) $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def UAVARENALOAD64_i16 : TwoInOneOut<IL_OP_ARENA_UAV_LOAD, (outs GPRI32:$dst),
        (ins GPRI32:$addy, i64imm:$id),
        !strconcat(IL_OP_ARENA_UAV_LOAD.Text, "_id($id)_size(short) $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def UAVARENALOAD64_i32 : TwoInOneOut<IL_OP_ARENA_UAV_LOAD, (outs GPRI32:$dst),
        (ins GPRI32:$addy, i64imm:$id),
        !strconcat(IL_OP_ARENA_UAV_LOAD.Text, "_id($id)_size(dword) $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def UAVARENALOAD64_Y_i32 : TwoInOneOut<IL_OP_ARENA_UAV_LOAD, (outs GPRI32:$dst),
        (ins GPRI32:$addy, i64imm:$id),
        !strconcat(IL_OP_ARENA_UAV_LOAD.Text, "_id($id)_size(dword) $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def UAVARENALOAD64_Z_i32 : TwoInOneOut<IL_OP_ARENA_UAV_LOAD, (outs GPRI32:$dst),
        (ins GPRI32:$addy, i64imm:$id),
        !strconcat(IL_OP_ARENA_UAV_LOAD.Text, "_id($id)_size(dword) $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def UAVARENALOAD64_W_i32 : TwoInOneOut<IL_OP_ARENA_UAV_LOAD, (outs GPRI32:$dst),
        (ins GPRI32:$addy, i64imm:$id),
        !strconcat(IL_OP_ARENA_UAV_LOAD.Text, "_id($id)_size(dword) $dst, $addy"), []>;
   // This opcode has custom swizzle patterns for some of the arguments.
    def UAVRAWLOAD64_i32 : TwoInOneOut<IL_OP_RAW_UAV_LOAD, (outs GPRI32:$dst),
        (ins GPRI32:$addy, i64imm:$id),
        !strconcat(IL_OP_RAW_UAV_LOAD.Text, "_id($id) $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def UAVRAWLOAD64_v2i32 : TwoInOneOut<IL_OP_RAW_UAV_LOAD, (outs GPRV2I32:$dst),
        (ins GPRI32:$addy, i64imm:$id),
        !strconcat(IL_OP_RAW_UAV_LOAD.Text, "_id($id) $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def UAVRAWLOAD64_v4i32 : TwoInOneOut<IL_OP_RAW_UAV_LOAD, (outs GPRV4I32:$dst),
        (ins GPRI32:$addy, i64imm:$id),
        !strconcat(IL_OP_RAW_UAV_LOAD.Text, "_id($id) $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def UAVRAWLOADCACHED64_i32 : TwoInOneOut<IL_OP_RAW_UAV_LOAD, (outs GPRI32:$dst),
        (ins GPRI32:$addy, i64imm:$id),
        !strconcat(IL_OP_RAW_UAV_LOAD.Text, "_id($id)_cached $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def UAVRAWLOADCACHED64_v2i32 : TwoInOneOut<IL_OP_RAW_UAV_LOAD, (outs GPRV2I32:$dst),
        (ins GPRI32:$addy, i64imm:$id),
        !strconcat(IL_OP_RAW_UAV_LOAD.Text, "_id($id)_cached $dst, $addy"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def UAVRAWLOADCACHED64_v4i32 : TwoInOneOut<IL_OP_RAW_UAV_LOAD, (outs GPRV4I32:$dst),
        (ins GPRI32:$addy, i64imm:$id),
        !strconcat(IL_OP_RAW_UAV_LOAD.Text, "_id($id)_cached $dst, $addy"), []>;
  }
  let mayStore = 1 in {
    // This opcode has custom swizzle patterns for some of the arguments.
    def SCRATCHSTORE64 : TwoInOneOut<IL_OP_MOV, (outs GPRI32:$addy),
        (ins GPRV4I32:$data, i64imm:$id),
        !strconcat(IL_OP_MOV.Text, " x$id[$addy], $data"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def SCRATCHSTORE64_X : TwoInOneOut<IL_OP_MOV, (outs GPRI32:$addy),
        (ins GPRI32:$data, i64imm:$id),
        !strconcat(IL_OP_MOV.Text, " x$id[$addy].x___, $data"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def SCRATCHSTORE64_Y : TwoInOneOut<IL_OP_MOV, (outs GPRI32:$addy),
        (ins GPRI32:$data, i64imm:$id),
        !strconcat(IL_OP_MOV.Text, " x$id[$addy]._y__, $data"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def SCRATCHSTORE64_Z : TwoInOneOut<IL_OP_MOV, (outs GPRI32:$addy),
        (ins GPRI32:$data, i64imm:$id),
        !strconcat(IL_OP_MOV.Text, " x$id[$addy].__z_, $data"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def SCRATCHSTORE64_W : TwoInOneOut<IL_OP_MOV, (outs GPRI32:$addy),
        (ins GPRI32:$data, i64imm:$id),
        !strconcat(IL_OP_MOV.Text, " x$id[$addy].___w, $data"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def SCRATCHSTORE64_XY : TwoInOneOut<IL_OP_MOV, (outs GPRI32:$addy),
        (ins GPRV2I32:$data, i64imm:$id),
        !strconcat(IL_OP_MOV.Text, " x$id[$addy].xy__, $data"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def SCRATCHSTORE64_ZW : TwoInOneOut<IL_OP_MOV, (outs GPRI32:$addy),
        (ins GPRV2I32:$data, i64imm:$id),
        !strconcat(IL_OP_MOV.Text, " x$id[$addy].__zw, $data"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def GDSSTORE64 : TwoInOneOut<IL_OP_GDS_STORE, (outs GPRI32:$addy),
        (ins GPRI32:$src, i64imm:$id),
        !strconcat(IL_OP_GDS_STORE.Text, "_id($id) $addy, $src"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def GDSSTORE64_Y : TwoInOneOut<IL_OP_GDS_STORE, (outs GPRI32:$addy),
        (ins GPRI32:$src, i64imm:$id),
        !strconcat(IL_OP_GDS_STORE.Text, "_id($id) $addy, $src"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def GDSSTORE64_Z : TwoInOneOut<IL_OP_GDS_STORE, (outs GPRI32:$addy),
        (ins GPRI32:$src, i64imm:$id),
        !strconcat(IL_OP_GDS_STORE.Text, "_id($id) $addy, $src"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def GDSSTORE64_W : TwoInOneOut<IL_OP_GDS_STORE, (outs GPRI32:$addy),
        (ins GPRI32:$src, i64imm:$id),
        !strconcat(IL_OP_GDS_STORE.Text, "_id($id) $addy, $src"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def LDSSTOREVEC64 : ThreeInOneOut<IL_OP_LDS_STORE_VEC, (outs GPRI32:$mem),
        (ins GPRI32:$addy, GPRI32:$src, i64imm:$id),
        !strconcat(IL_OP_LDS_STORE_VEC.Text, "_id($id) $mem, $addy, $src, $src"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def LDSSTOREVEC64_v2i32 : ThreeInOneOut<IL_OP_LDS_STORE_VEC, (outs GPRV2I32:$mem),
        (ins GPRI32:$addy, GPRV2I32:$src, i64imm:$id),
        !strconcat(IL_OP_LDS_STORE_VEC.Text, "_id($id) $mem, $addy, $src, $src"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def LDSSTOREVEC64_v4i32 : ThreeInOneOut<IL_OP_LDS_STORE_VEC, (outs GPRV4I32:$mem),
        (ins GPRI32:$addy, GPRV4I32:$src, i64imm:$id),
        !strconcat(IL_OP_LDS_STORE_VEC.Text, "_id($id) $mem, $addy, $src, $src"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def LDSSTORE64 : TwoInOneOut<IL_OP_LDS_STORE, (outs GPRI32:$addy),
        (ins GPRI32:$src, i64imm:$id),
        !strconcat(IL_OP_LDS_STORE.Text, "_id($id) $addy, $src"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def LDSSTORE64_i8 : TwoInOneOut<IL_OP_LDS_STORE_BYTE, (outs GPRI32:$addy),
        (ins GPRI32:$src, i64imm:$id),
        !strconcat(IL_OP_LDS_STORE_BYTE.Text, "_id($id) $addy, $src"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def LDSSTORE64_i16 : TwoInOneOut<IL_OP_LDS_STORE_SHORT, (outs GPRI32:$addy),
        (ins GPRI32:$src, i64imm:$id),
        !strconcat(IL_OP_LDS_STORE_SHORT.Text, "_id($id) $addy, $src"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def LDSSTORE64_Y : TwoInOneOut<IL_OP_LDS_STORE, (outs GPRI32:$addy),
        (ins GPRI32:$src, i64imm:$id),
        !strconcat(IL_OP_LDS_STORE.Text, "_id($id) $addy, $src"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def LDSSTORE64_Z : TwoInOneOut<IL_OP_LDS_STORE, (outs GPRI32:$addy),
        (ins GPRI32:$src, i64imm:$id),
        !strconcat(IL_OP_LDS_STORE.Text, "_id($id) $addy, $src"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def LDSSTORE64_W : TwoInOneOut<IL_OP_LDS_STORE, (outs GPRI32:$addy),
        (ins GPRI32:$src, i64imm:$id),
        !strconcat(IL_OP_LDS_STORE.Text, "_id($id) $addy, $src"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def UAVARENASTORE64_i8 : TwoInOneOut<IL_OP_ARENA_UAV_STORE, (outs GPRI32:$addy),
        (ins GPRI8:$src, i64imm:$id),
        !strconcat(IL_OP_ARENA_UAV_STORE.Text, 
            "_id($id)_size(byte) $addy, $src"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def UAVARENASTORE64_i16 : TwoInOneOut<IL_OP_ARENA_UAV_STORE, (outs GPRI32:$addy),
        (ins GPRI16:$src, i64imm:$id),
        !strconcat(IL_OP_ARENA_UAV_STORE.Text, 
            "_id($id)_size(short) $addy, $src"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def UAVARENASTORE64_i32 : TwoInOneOut<IL_OP_ARENA_UAV_STORE, (outs GPRI32:$addy),
        (ins GPRI32:$src, i64imm:$id),
        !strconcat(IL_OP_ARENA_UAV_STORE.Text, 
            "_id($id)_size(dword) $addy, $src"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def UAVARENASTORE64_Y_i32 : TwoInOneOut<IL_OP_ARENA_UAV_STORE, (outs GPRI32:$addy),
        (ins GPRI32:$src, i64imm:$id),
        !strconcat(IL_OP_ARENA_UAV_STORE.Text, 
            "_id($id)_size(dword) $addy, $src"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def UAVARENASTORE64_Z_i32 : TwoInOneOut<IL_OP_ARENA_UAV_STORE, (outs GPRI32:$addy),
        (ins GPRI32:$src, i64imm:$id),
        !strconcat(IL_OP_ARENA_UAV_STORE.Text, 
            "_id($id)_size(dword) $addy, $src"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def UAVARENASTORE64_W_i32 : TwoInOneOut<IL_OP_ARENA_UAV_STORE, (outs GPRI32:$addy),
        (ins GPRI32:$src, i64imm:$id),
        !strconcat(IL_OP_ARENA_UAV_STORE.Text, 
            "_id($id)_size(dword) $addy, $src"), []>;
   // This opcode has custom swizzle patterns for some of the arguments.
    def UAVRAWSTORE64_i32 : TwoInOneOut<IL_OP_RAW_UAV_STORE, (outs GPRI32:$mem),
        (ins GPRI32:$addy, GPRI32:$src, i64imm:$id),
        !strconcat(IL_OP_RAW_UAV_STORE.Text, "_id($id) $mem, $addy, $src"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def UAVRAWSTORE64_v2i32 : TwoInOneOut<IL_OP_RAW_UAV_STORE, (outs GPRV2I32:$mem),
        (ins GPRI32:$addy, GPRV2I32:$src, i64imm:$id),
        !strconcat(IL_OP_RAW_UAV_STORE.Text, "_id($id) $mem, $addy, $src"), []>;
    // This opcode has custom swizzle patterns for some of the arguments.
    def UAVRAWSTORE64_v4i32 : TwoInOneOut<IL_OP_RAW_UAV_STORE, (outs GPRV4I32:$mem),
        (ins GPRI32:$addy, GPRV4I32:$src, i64imm:$id),
        !strconcat(IL_OP_RAW_UAV_STORE.Text, "_id($id) $mem, $addy, $src"), []>;
  }
}
//===---------------------------------------------------------------------===//
// Custom Inserter for Branches and returns, this eventually will be a
// seperate pass
//===---------------------------------------------------------------------===//
let isTerminator = 1 in {
  def BRANCH : ILFormat<IL_PSEUDO_INST, (outs), (ins brtarget:$target),
      "; Pseudo unconditional branch instruction",
      [(br bb:$target)]>;
  defm BRANCH_COND : BranchConditional<IL_brcond>;
}
//===---------------------------------------------------------------------===//
// return instructions
//===---------------------------------------------------------------------===//
let isTerminator = 1, isReturn = 1, isBarrier = 1, hasCtrlDep = 1 in {
  def RETURN          : ILFormat<IL_OP_RET,(outs), (ins variable_ops),
      IL_OP_RET.Text, [(IL_retflag)]>;
}
//===---------------------------------------------------------------------===//
// Lower and raise the stack x amount
//===---------------------------------------------------------------------===//
def ADJCALLSTACKDOWN : ILFormat<IL_PSEUDO_INST, (outs), (ins i32imm:$amt),
    "; begin of call sequence $amt",
    [(IL_callseq_start timm:$amt)]>;
def ADJCALLSTACKUP : ILFormat<IL_PSEUDO_INST, (outs), (ins i32imm:$amt1,
    i32imm:$amt2),
    "; end of call sequence $amt1 $amt2",
    [(IL_callseq_end timm:$amt1, timm:$amt2)]>;

//===---------------------------------------------------------------------===//
// Handle a function call
//===---------------------------------------------------------------------===//
let isCall = 1,
    Defs = [
    R110, R111,
    R112, R113, R114, R115, R116, R117, R118, R119, R120, R121, R122, R123, R124,
    R125, R126, R127,
    R128, R129, R130, R131, R132, R133, R134, R135, R136, R137, R138, R139, R140,
    R141, R142, R143,
    R144, R145, R146, R147, R148, R149, R150, R151, R152, R153, R154, R155, R156,
    R157, R158, R159,
    R160, R161, R162, R163, R164, R165, R166, R167, R168, R169, R170, R171, R172,
    R173, R174, R175,
    R176, R177, R178, R179, R180, R181, R182, R183, R184, R185, R186, R187, R188,
    R189, R190, R191,
    R192, R193, R194, R195, R196, R197, R198, R199, R200, R201, R202, R203, R204,
    R205, R206, R207,
    R208, R209, R210, R211, R212, R213, R214, R215, R216, R217, R218, R219, R220,
    R221, R222, R223,
    R224, R225, R226, R227, R228, R229, R230, R231, R232, R233, R234, R235, R236,
    R237, R238, R239,
    R240, R241, R242, R243, R244, R245, R246, R247, R248, R249, R250, R251, R252,
    R253, R254, R255
    ]
    ,
    Uses = [
    R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12, R13, R14, R15,
    R16, R17, R18, R19, R20, R21, R22, R23, R24, R25, R26, R27, R28, R29, R30, R31,
    R32, R33, R34, R35, R36, R37, R38, R39, R40, R41, R42, R43, R44, R45, R46, R47,
    R48, R49, R50, R51, R52, R53, R54, R55, R56, R57, R58, R59, R60, R61, R62, R63,
    R64, R65, R66, R67, R68, R69, R70, R71, R72, R73, R74, R75, R76, R77, R78, R79,
    R80, R81, R82, R83, R84, R85, R86, R87, R88, R89, R90, R91, R92, R93, R94, R95,
    R96, R97, R98, R99, R100, R101, R102, R103, R104, R105, R106, R107, R108, R109
    ]
    in {
      def CALL : UnaryOpNoRet<IL_OP_CALL, (outs),
      (ins calltarget:$dst, variable_ops), 
      !strconcat(IL_OP_CALL.Text, " $dst"), []>;
    }


//===---------------------------------------------------------------------===//
// Flow and Program control Instructions
//===---------------------------------------------------------------------===//
let isTerminator=1 in {
  def SWITCH      : ILFormat<IL_OP_SWITCH, (outs), (ins GPRI32:$src),
  !strconcat(IL_OP_SWITCH.Text, " $src"), []>;
  def CASE        : ILFormat<IL_OP_CASE, (outs), (ins GPRI32:$src),
      !strconcat(IL_OP_CASE.Text, " $src"), []>;
  def BREAK       : ILFormat<IL_OP_BREAK, (outs), (ins),
      IL_OP_BREAK.Text, []>;
  def CONTINUE    : ILFormat<IL_OP_CONTINUE, (outs), (ins),
      IL_OP_CONTINUE.Text, []>;
  def DEFAULT     : ILFormat<IL_OP_DEFAULT, (outs), (ins),
      IL_OP_DEFAULT.Text, []>;
  def ELSE        : ILFormat<IL_OP_ELSE, (outs), (ins),
      IL_OP_ELSE.Text, []>;
  def ENDSWITCH   : ILFormat<IL_OP_ENDSWITCH, (outs), (ins),
      IL_OP_ENDSWITCH.Text, []>;
  def ENDMAIN     : ILFormat<IL_OP_ENDMAIN, (outs), (ins),
      IL_OP_ENDMAIN.Text, []>;
  def END         : ILFormat<IL_OP_END, (outs), (ins),
      IL_OP_END.Text, []>;
  def ENDFUNC     : ILFormat<IL_OP_ENDFUNC, (outs), (ins),
      IL_OP_ENDFUNC.Text, []>;
  def ENDIF       : ILFormat<IL_OP_ENDIF, (outs), (ins),
      IL_OP_ENDIF.Text, []>;
  def WHILELOOP   : ILFormat<IL_OP_WHILE, (outs), (ins),
      IL_OP_WHILE.Text, []>;
  def ENDLOOP     : ILFormat<IL_OP_ENDLOOP, (outs), (ins),
      IL_OP_ENDLOOP.Text, []>;
  def FUNC        : ILFormat<IL_OP_FUNC, (outs), (ins),
      IL_OP_FUNC.Text, []>;
  def RETDYN      : ILFormat<IL_OP_RET_DYN, (outs), (ins),
      IL_OP_RET_DYN.Text, []>;
  // This opcode has custom swizzle pattern encoded in Swizzle Encoder
  defm IF_LOGICALNZ  : BranchInstr<IL_OP_IF_LOGICALNZ>;
  // This opcode has custom swizzle pattern encoded in Swizzle Encoder
  defm IF_LOGICALZ   : BranchInstr<IL_OP_IF_LOGICALZ>;
  // This opcode has custom swizzle pattern encoded in Swizzle Encoder
  defm BREAK_LOGICALNZ : BranchInstr<IL_OP_BREAK_LOGICALNZ>;
  // This opcode has custom swizzle pattern encoded in Swizzle Encoder
  defm BREAK_LOGICALZ : BranchInstr<IL_OP_BREAK_LOGICALZ>;
  // This opcode has custom swizzle pattern encoded in Swizzle Encoder
  defm CONTINUE_LOGICALNZ : BranchInstr<IL_OP_CONTINUE_LOGICALNZ>;
  // This opcode has custom swizzle pattern encoded in Swizzle Encoder
  defm CONTINUE_LOGICALZ : BranchInstr<IL_OP_CONTINUE_LOGICALZ>;
  defm IFC         : BranchInstr2<IL_OP_IFC>;
  defm BREAKC      : BranchInstr2<IL_OP_BREAKC>;
  defm CONTINUEC   : BranchInstr2<IL_OP_CONTINUEC>;
}
let isTerminator = 1, isBarrier = 1, hasCtrlDep = 1 in {
  def TRAP : ILFormat<IL_OP_NOP, (outs), (ins),
      IL_OP_NOP.Text, [(trap)]>;
}

//===---------------------------------------------------------------------===//
//----------------- Work Item Functions - OpenCL 6.11.1 ---------------------//
//===---------------------------------------------------------------------===//
let isCall = 1, isAsCheapAsAMove = 1 in {
  def GET_WORK_DIM : ILFormat<IL_OP_MOV, (outs GPRI32:$dst), (ins),
      !strconcat(IL_OP_MOV.Text, " $dst, cb0[0].w"),
      [(set GPRI32:$dst, (int_AMDIL_get_work_dim))]>;

  def GET_GLOBAL_ID : ILFormat<IL_OP_MOV, (outs GPRV4I32:$dst),
      (ins), !strconcat(IL_OP_MOV.Text, " $dst, r1021.xyz0"),
      [(set GPRV4I32:$dst, (int_AMDIL_get_global_id))]>;

  def GET_LOCAL_ID : ILFormat<IL_OP_MOV, (outs GPRV4I32:$dst),
      (ins), !strconcat(IL_OP_MOV.Text, " $dst, r1022.xyz0"),
      [(set GPRV4I32:$dst, (int_AMDIL_get_local_id))]>;

  def GET_GROUP_ID : ILFormat<IL_OP_MOV, (outs GPRV4I32:$dst),
      (ins), !strconcat(IL_OP_MOV.Text, " $dst, r1023.xyz0"),
      [(set GPRV4I32:$dst, (int_AMDIL_get_group_id))]>;

  def GET_GLOBAL_SIZE : ILFormat<IL_OP_MOV, (outs GPRV4I32:$dst),
      (ins), !strconcat(IL_OP_MOV.Text, " $dst, cb0[0].xyz0"),
      [(set GPRV4I32:$dst, (int_AMDIL_get_global_size))]>;

  def GET_LOCAL_SIZE : ILFormat<IL_OP_MOV, (outs GPRV4I32:$dst),
      (ins), !strconcat(IL_OP_MOV.Text, " $dst, cb0[1].xyz0"),
      [(set GPRV4I32:$dst, (int_AMDIL_get_local_size))]>;

  def GET_NUM_GROUPS : ILFormat<IL_OP_MOV, (outs GPRV4I32:$dst),
      (ins), !strconcat(IL_OP_MOV.Text, " $dst, cb0[2].xyz0"),
      [(set GPRV4I32:$dst, (int_AMDIL_get_num_groups))]>;

  def GET_GLOBAL_OFFSET : ILFormat<IL_OP_MOV, (outs GPRV4I32:$dst),
      (ins), !strconcat(IL_OP_MOV.Text, " $dst, cb0[9].xyz0"),
      [(set GPRV4I32:$dst, (int_AMDIL_get_global_offset))]>;

  let Predicates = [Has64BitPtr] in {
    def GET_PRINTF_OFFSET_i64: ILFormat<IL_OP_MOV, (outs GPRI32:$dst),
        (ins), !strconcat(IL_OP_MOV.Text, " $dst, cb0[8].zw"),
        [(set GPRI32:$dst, (int_AMDIL_get_printf_offset))]>;
    def GET_PRINTF_SIZE_i64 : ILFormat<IL_OP_MOV, (outs GPRI32:$dst),
        (ins), !strconcat(IL_OP_MOV.Text, " $dst, cb0[9].x0"),
        [(set GPRI32:$dst, (int_AMDIL_get_printf_size))]>;
  }
  let Predicates = [Has32BitPtr] in {
    def GET_PRINTF_OFFSET_i32 : ILFormat<IL_OP_MOV, (outs GPRI32:$dst),
        (ins), !strconcat(IL_OP_MOV.Text, " $dst, cb0[8].y0"),
        [(set GPRI32:$dst, (int_AMDIL_get_printf_offset))]>;
    def GET_PRINTF_SIZE_i32 : ILFormat<IL_OP_MOV, (outs GPRI32:$dst),
        (ins), !strconcat(IL_OP_MOV.Text, " $dst, cb0[8].z0"),
        [(set GPRI32:$dst, (int_AMDIL_get_printf_size))]>;
  }
}
//===---------------------------------------------------------------------===//
//------------- Synchronization Functions - OpenCL 6.11.9 -------------------//
//===---------------------------------------------------------------------===//
let isCall=1 in {

  def FENCE : BinaryOpNoRet<IL_OP_FENCE, (outs), (ins GPRI32:$flag),
      "fence_lds_memory_gds",
      [(int_AMDIL_fence GPRI32:$flag)]>;

  def FENCE_LOCAL : BinaryOpNoRet<IL_OP_FENCE, (outs), (ins GPRI32:$flag),
      "fence_lds",
      [(int_AMDIL_fence_local GPRI32:$flag)]>;

  def FENCE_GLOBAL : BinaryOpNoRet<IL_OP_FENCE, (outs), (ins GPRI32:$flag),
      "fence_memory",
      [(int_AMDIL_fence_global GPRI32:$flag)]>;

  def FENCE_REGION : BinaryOpNoRet<IL_OP_FENCE, (outs), (ins GPRI32:$flag),
      "fence_gds",
      [(int_AMDIL_fence_region GPRI32:$flag)]>;

  def FENCE_READ_ONLY : BinaryOpNoRet<IL_OP_FENCE_READ_ONLY, (outs),
      (ins GPRI32:$flag),
      "fence_lds_gds_memory_mem_read_only",
      [(int_AMDIL_fence_read_only GPRI32:$flag)]>;

  def FENCE_READ_ONLY_LOCAL : BinaryOpNoRet<IL_OP_FENCE_READ_ONLY, (outs),
      (ins GPRI32:$flag),
      "fence_lds_mem_read_only",
      [(int_AMDIL_fence_read_only_local GPRI32:$flag)]>;

  def FENCE_READ_ONLY_GLOBAL : BinaryOpNoRet<IL_OP_FENCE_READ_ONLY, (outs),
      (ins GPRI32:$flag),
      "fence_mem_read_only",
      [(int_AMDIL_fence_read_only_global GPRI32:$flag)]>;

  def FENCE_READ_ONLY_REGION : BinaryOpNoRet<IL_OP_FENCE_READ_ONLY, (outs),
      (ins GPRI32:$flag),
      "fence_gds_mem_read_only",
      [(int_AMDIL_fence_read_only_region GPRI32:$flag)]>;

  def FENCE_WRITE_ONLY : BinaryOpNoRet<IL_OP_FENCE_WRITE_ONLY, (outs),
      (ins GPRI32:$flag),
      "fence_lds_gds_memory_mem_write_only",
      [(int_AMDIL_fence_write_only GPRI32:$flag)]>;

  def FENCE_WRITE_ONLY_LOCAL : BinaryOpNoRet<IL_OP_FENCE_WRITE_ONLY, (outs),
      (ins GPRI32:$flag),
      "fence_lds_mem_write_only",
      [(int_AMDIL_fence_write_only_local GPRI32:$flag)]>;

  def FENCE_WRITE_ONLY_GLOBAL : BinaryOpNoRet<IL_OP_FENCE_WRITE_ONLY, (outs),
      (ins GPRI32:$flag),
      "fence_mem_write_only",
      [(int_AMDIL_fence_write_only_global GPRI32:$flag)]>;

  def FENCE_WRITE_ONLY_REGION : BinaryOpNoRet<IL_OP_FENCE_WRITE_ONLY, (outs),
      (ins GPRI32:$flag),
      "fence_gds_mem_write_only",
      [(int_AMDIL_fence_write_only_region GPRI32:$flag)]>;
}
let isReturn = 1 in {
  def EARLY_EXIT : UnaryOpNoRet<IL_OP_RET_LOGICALNZ, (outs),
      (ins GPRI32:$flag),
      !strconcat(IL_OP_RET_LOGICALNZ.Text, " $flag"),
      [(int_AMDIL_early_exit GPRI32:$flag)]>;
}
def MEDIA_UNPACK_0 : OneInOneOut<IL_OP_UNPACK_0, (outs GPRV4F32:$dst),
    (ins GPRV4I32:$src),
    !strconcat(IL_OP_UNPACK_0.Text, " $dst, $src"),
    [(set GPRV4F32:$dst,
        (v4f32 (int_AMDIL_media_unpack_byte_0 GPRV4I32:$src)))]>;
def MEDIA_UNPACK_1 : OneInOneOut<IL_OP_UNPACK_1, (outs GPRV4F32:$dst),
    (ins GPRV4I32:$src),
    !strconcat(IL_OP_UNPACK_1.Text, " $dst, $src"),
    [(set GPRV4F32:$dst,
        (v4f32 (int_AMDIL_media_unpack_byte_1 GPRV4I32:$src)))]>;
def MEDIA_UNPACK_2 : OneInOneOut<IL_OP_UNPACK_2, (outs GPRV4F32:$dst),
    (ins GPRV4I32:$src),
    !strconcat(IL_OP_UNPACK_2.Text, " $dst, $src"),
    [(set GPRV4F32:$dst,
        (v4f32 (int_AMDIL_media_unpack_byte_2 GPRV4I32:$src)))]>;
def MEDIA_UNPACK_3 : OneInOneOut<IL_OP_UNPACK_3, (outs GPRV4F32:$dst),
    (ins GPRV4I32:$src),
    !strconcat(IL_OP_UNPACK_3.Text, " $dst, $src"),
    [(set GPRV4F32:$dst,
        (v4f32 (int_AMDIL_media_unpack_byte_3 GPRV4I32:$src)))]>;
let Predicates = [Has32BitPtr] in {
// All of the image functions
def IMAGE1D_READ : ILFormat<IL_OP_SAMPLE, (outs GPRV4I32:$dst),
    (ins MEMI32:$ptr, GPRI32:$sampler, GPRV4F32:$addy),
    !strconcat(IL_OP_SAMPLE.Text, 
        "_resource($ptr)_sampler($sampler)_coordtype(normalized) $dst, $addy"),
    [(set GPRV4I32:$dst,
        (int_AMDIL_image1d_read_norm ADDR:$ptr, GPRI32:$sampler, GPRV4F32:$addy))]>;
def IMAGE1DA_READ : ILFormat<IL_OP_SAMPLE, (outs GPRV4I32:$dst),
    (ins MEMI32:$ptr, GPRI32:$sampler, GPRV4F32:$addy),
    !strconcat(IL_OP_SAMPLE.Text, 
        "_resource($ptr)_sampler($sampler)_coordtype(normalized) $dst, $addy"),
    [(set GPRV4I32:$dst,
        (int_AMDIL_image1d_array_read_norm ADDR:$ptr, GPRI32:$sampler, GPRV4F32:$addy))]>;
def IMAGE2D_READ : ILFormat<IL_OP_SAMPLE, (outs GPRV4I32:$dst),
    (ins MEMI32:$ptr, GPRI32:$sampler, GPRV4F32:$addy),
    !strconcat(IL_OP_SAMPLE.Text, 
        "_resource($ptr)_sampler($sampler)_coordtype(normalized) $dst, $addy"),
    [(set GPRV4I32:$dst,
        (int_AMDIL_image2d_read_norm ADDR:$ptr, GPRI32:$sampler, GPRV4F32:$addy))]>;
def IMAGE2DA_READ : ILFormat<IL_OP_SAMPLE, (outs GPRV4I32:$dst),
    (ins MEMI32:$ptr, GPRI32:$sampler, GPRV4F32:$addy),
    !strconcat(IL_OP_SAMPLE.Text, 
        "_resource($ptr)_sampler($sampler)_coordtype(normalized) $dst, $addy"),
    [(set GPRV4I32:$dst,
        (int_AMDIL_image2d_array_read_norm ADDR:$ptr, GPRI32:$sampler, GPRV4F32:$addy))]>;
def IMAGE3D_READ : ILFormat<IL_OP_SAMPLE, (outs GPRV4I32:$dst),
    (ins MEMI32:$ptr, GPRI32:$sampler, GPRV4F32:$addy),
    !strconcat(IL_OP_SAMPLE.Text, 
        "_resource($ptr)_sampler($sampler)_coordtype(normalized) $dst, $addy"),
    [(set GPRV4I32:$dst,
        (int_AMDIL_image3d_read_norm ADDR:$ptr, GPRI32:$sampler, GPRV4F32:$addy))]>;
def IMAGE1D_READ_UNNORM : ILFormat<IL_OP_SAMPLE, (outs GPRV4I32:$dst),
    (ins MEMI32:$ptr, GPRI32:$sampler, GPRV4F32:$addy),
    !strconcat(IL_OP_SAMPLE.Text, 
        "_resource($ptr)_sampler($sampler)_coordtype(unnormalized) $dst, $addy"),
    [(set GPRV4I32:$dst,
        (int_AMDIL_image1d_read_unnorm ADDR:$ptr, GPRI32:$sampler, GPRV4F32:$addy))]>;
def IMAGE1DA_READ_UNNORM : ILFormat<IL_OP_SAMPLE, (outs GPRV4I32:$dst),
    (ins MEMI32:$ptr, GPRI32:$sampler, GPRV4F32:$addy),
    !strconcat(IL_OP_SAMPLE.Text, 
        "_resource($ptr)_sampler($sampler)_coordtype(unnormalized) $dst, $addy"),
    [(set GPRV4I32:$dst,
        (int_AMDIL_image1d_array_read_unnorm ADDR:$ptr, GPRI32:$sampler, GPRV4F32:$addy))]>;
def IMAGE2D_READ_UNNORM : ILFormat<IL_OP_SAMPLE, (outs GPRV4I32:$dst),
    (ins MEMI32:$ptr, GPRI32:$sampler, GPRV4F32:$addy),
    !strconcat(IL_OP_SAMPLE.Text, 
        "_resource($ptr)_sampler($sampler)_coordtype(unnormalized) $dst, $addy"),
    [(set GPRV4I32:$dst,
        (int_AMDIL_image2d_read_unnorm ADDR:$ptr, GPRI32:$sampler, GPRV4F32:$addy))]>;
def IMAGE2DA_READ_UNNORM : ILFormat<IL_OP_SAMPLE, (outs GPRV4I32:$dst),
    (ins MEMI32:$ptr, GPRI32:$sampler, GPRV4F32:$addy),
    !strconcat(IL_OP_SAMPLE.Text, 
        "_resource($ptr)_sampler($sampler)_coordtype(unnormalized) $dst, $addy"),
    [(set GPRV4I32:$dst,
        (int_AMDIL_image2d_array_read_unnorm ADDR:$ptr, GPRI32:$sampler, GPRV4F32:$addy))]>;
def IMAGE3D_READ_UNNORM : ILFormat<IL_OP_SAMPLE, (outs GPRV4I32:$dst),
    (ins MEMI32:$ptr, GPRI32:$sampler, GPRV4F32:$addy),
    !strconcat(IL_OP_SAMPLE.Text, 
        "_resource($ptr)_sampler($sampler)_coordtype(unnormalized) $dst, $addy"),
    [(set GPRV4I32:$dst,
        (int_AMDIL_image3d_read_unnorm ADDR:$ptr, GPRI32:$sampler, GPRV4F32:$addy))]>;
def IMAGE1D_INFO0 : ILFormat<IL_OP_MOV, (outs GPRV4I32:$dst),
    (ins MEMI32:$ptr),
    !strconcat(IL_OP_MOV.Text, " $dst, $ptr"),
    [(set GPRV4I32:$dst, (int_AMDIL_image1d_info0 ADDR:$ptr))]>;
def IMAGE1D_INFO1 : ILFormat<IL_OP_MOV, (outs GPRV4I32:$dst),
    (ins MEMI32:$ptr),
    !strconcat(IL_OP_MOV.Text, " $dst, $ptr"),
    [(set GPRV4I32:$dst, (int_AMDIL_image1d_info1 ADDR:$ptr))]>;
def IMAGE1DA_INFO0 : ILFormat<IL_OP_MOV, (outs GPRV4I32:$dst),
    (ins MEMI32:$ptr),
    !strconcat(IL_OP_MOV.Text, " $dst, $ptr"),
    [(set GPRV4I32:$dst, (int_AMDIL_image1d_array_info0 ADDR:$ptr))]>;
def IMAGE1DA_INFO1 : ILFormat<IL_OP_MOV, (outs GPRV4I32:$dst),
    (ins MEMI32:$ptr),
    !strconcat(IL_OP_MOV.Text, " $dst, $ptr"),
    [(set GPRV4I32:$dst, (int_AMDIL_image1d_array_info1 ADDR:$ptr))]>;
def IMAGE2D_INFO0 : ILFormat<IL_OP_MOV, (outs GPRV4I32:$dst),
    (ins MEMI32:$ptr),
    !strconcat(IL_OP_MOV.Text, " $dst, $ptr"),
    [(set GPRV4I32:$dst, (int_AMDIL_image2d_info0 ADDR:$ptr))]>;
def IMAGE2D_INFO1 : ILFormat<IL_OP_MOV, (outs GPRV4I32:$dst),
    (ins MEMI32:$ptr),
    !strconcat(IL_OP_MOV.Text, " $dst, $ptr"),
    [(set GPRV4I32:$dst, (int_AMDIL_image2d_info1 ADDR:$ptr))]>;
def IMAGE2DA_INFO0 : ILFormat<IL_OP_MOV, (outs GPRV4I32:$dst),
    (ins MEMI32:$ptr),
    !strconcat(IL_OP_MOV.Text, " $dst, $ptr"),
    [(set GPRV4I32:$dst, (int_AMDIL_image2d_array_info0 ADDR:$ptr))]>;
def IMAGE2DA_INFO1 : ILFormat<IL_OP_MOV, (outs GPRV4I32:$dst),
    (ins MEMI32:$ptr),
    !strconcat(IL_OP_MOV.Text, " $dst, $ptr"),
    [(set GPRV4I32:$dst, (int_AMDIL_image2d_array_info1 ADDR:$ptr))]>;
def IMAGE3D_INFO0 : ILFormat<IL_OP_MOV, (outs GPRV4I32:$dst),
    (ins MEMI32:$ptr),
    !strconcat(IL_OP_MOV.Text, " $dst, $ptr"),
    [(set GPRV4I32:$dst, (int_AMDIL_image3d_info0 ADDR:$ptr))]>;
def IMAGE3D_INFO1 : ILFormat<IL_OP_MOV, (outs GPRV4I32:$dst),
    (ins MEMI32:$ptr),
    !strconcat(IL_OP_MOV.Text, " $dst, $ptr"),
    [(set GPRV4I32:$dst, (int_AMDIL_image3d_info1 ADDR:$ptr))]>;
def IMAGE1D_WRITE : ILFormat<IL_OP_UAV_STORE, (outs),
    (ins MEMI32:$ptr, GPRV2I32:$addy, GPRV4I32:$data),
    !strconcat(IL_OP_UAV_STORE.Text,
        "_id($ptr) $addy, $data"),
    [(int_AMDIL_image1d_write ADDR:$ptr, GPRV2I32:$addy, GPRV4I32:$data)]>;
def IMAGE1DA_WRITE : ILFormat<IL_OP_UAV_STORE, (outs),
    (ins MEMI32:$ptr, GPRV2I32:$addy, GPRV4I32:$data),
    !strconcat(IL_OP_UAV_STORE.Text,
        "_id($ptr) $addy, $data"),
    [(int_AMDIL_image1d_array_write ADDR:$ptr, GPRV2I32:$addy, GPRV4I32:$data)]>;
def IMAGE2D_WRITE : ILFormat<IL_OP_UAV_STORE, (outs),
    (ins MEMI32:$ptr, GPRV2I32:$addy, GPRV4I32:$data),
    !strconcat(IL_OP_UAV_STORE.Text,
        "_id($ptr) $addy, $data"),
    [(int_AMDIL_image2d_write ADDR:$ptr, GPRV2I32:$addy, GPRV4I32:$data)]>;
def IMAGE2DA_WRITE : ILFormat<IL_OP_UAV_STORE, (outs),
    (ins MEMI32:$ptr, GPRV2I32:$addy, GPRV4I32:$data),
    !strconcat(IL_OP_UAV_STORE.Text,
        "_id($ptr) $addy, $data"),
    [(int_AMDIL_image2d_array_write ADDR:$ptr, GPRV2I32:$addy, GPRV4I32:$data)]>;
def IMAGE3D_WRITE : ILFormat<IL_OP_UAV_STORE, (outs),
    (ins MEMI32:$ptr, GPRV4I32:$addy, GPRV4I32:$data),
    !strconcat(IL_OP_UAV_STORE.Text,
        "_id($ptr) $addy, $data"),
    [(int_AMDIL_image3d_write ADDR:$ptr, GPRV4I32:$addy, GPRV4I32:$data)]>;
let hasSideEffects = 1, isNotDuplicable = 1 in {
  // All of the noret atomic functions
  def ATOM_G_ADD_NORET     : BinAtomNoRet<IL_OP_UAV_ADD,
      "_id($id)", atom_g_add_noret>;
  def ATOM_G_AND_NORET     : BinAtomNoRet<IL_OP_UAV_AND,
      "_id($id)", atom_g_and_noret>;
  def ATOM_G_MAX_NORET     : BinAtomNoRet<IL_OP_UAV_MAX,
      "_id($id)", atom_g_max_noret>;
  def ATOM_G_MIN_NORET     : BinAtomNoRet<IL_OP_UAV_MIN,
      "_id($id)", atom_g_min_noret>;
  def ATOM_G_UMAX_NORET    : BinAtomNoRet<IL_OP_UAV_UMAX,
      "_id($id)", atom_g_umax_noret>;
  def ATOM_G_UMIN_NORET    : BinAtomNoRet<IL_OP_UAV_UMIN,
      "_id($id)", atom_g_umin_noret>;
  def ATOM_G_OR_NORET      : BinAtomNoRet<IL_OP_UAV_OR,
      "_id($id)", atom_g_or_noret>;
  def ATOM_G_RSUB_NORET    : BinAtomNoRet<IL_OP_UAV_RSUB,
      "_id($id)", atom_g_rsub_noret>;
  def ATOM_G_SUB_NORET     : BinAtomNoRet<IL_OP_UAV_SUB,
      "_id($id)", atom_g_sub_noret>;
  def ATOM_G_XOR_NORET     : BinAtomNoRet<IL_OP_UAV_XOR,
      "_id($id)", atom_g_xor_noret>;
  def ATOM_G_INC_NORET     : BinAtomNoRet<IL_OP_UAV_INC,
      "_id($id)", atom_g_inc_noret>;
  def ATOM_G_DEC_NORET     : BinAtomNoRet<IL_OP_UAV_DEC,
      "_id($id)", atom_g_dec_noret>;
  def ATOM_G_CMPXCHG_NORET    : CmpXChgNoRet<IL_OP_UAV_CMP,
      "_id($id)", atom_g_cmpxchg_noret>;
  def ATOM_A_ADD_NORET     : BinAtomNoRet<IL_OP_UAV_ADD,
      "_id($id)_arena", atom_g_add_noret>;
  def ATOM_A_AND_NORET     : BinAtomNoRet<IL_OP_UAV_AND,
      "_id($id)_arena", atom_g_and_noret>;
  def ATOM_A_MAX_NORET     : BinAtomNoRet<IL_OP_UAV_MAX,
      "_id($id)_arena", atom_g_max_noret>;
  def ATOM_A_MIN_NORET     : BinAtomNoRet<IL_OP_UAV_MIN,
      "_id($id)_arena", atom_g_min_noret>;
  def ATOM_A_UMAX_NORET    : BinAtomNoRet<IL_OP_UAV_UMAX,
      "_id($id)_arena", atom_g_umax_noret>;
  def ATOM_A_UMIN_NORET    : BinAtomNoRet<IL_OP_UAV_UMIN,
      "_id($id)_arena", atom_g_umin_noret>;
  def ATOM_A_OR_NORET      : BinAtomNoRet<IL_OP_UAV_OR,
      "_id($id)_arena", atom_g_or_noret>;
  def ATOM_A_RSUB_NORET    : BinAtomNoRet<IL_OP_UAV_RSUB,
      "_id($id)_arena", atom_g_rsub_noret>;
  def ATOM_A_SUB_NORET     : BinAtomNoRet<IL_OP_UAV_SUB,
      "_id($id)_arena", atom_g_sub_noret>;
  def ATOM_A_XOR_NORET     : BinAtomNoRet<IL_OP_UAV_XOR,
      "_id($id)_arena", atom_g_xor_noret>;
  def ATOM_A_INC_NORET     : BinAtomNoRet<IL_OP_UAV_INC,
      "_id($id)_arena", atom_g_inc_noret>;
  def ATOM_A_DEC_NORET     : BinAtomNoRet<IL_OP_UAV_DEC,
      "_id($id)_arena", atom_g_dec_noret>;
  def ATOM_A_CMPXCHG_NORET    : CmpXChgNoRet<IL_OP_UAV_CMP,
      "_id($id)_arena", atom_g_cmpxchg_noret>;
  def ATOM_L_ADD_NORET     : BinAtomNoRet<IL_OP_LDS_ADD,
      "_resource($id)", atom_l_add_noret>;
  def ATOM_L_AND_NORET     : BinAtomNoRet<IL_OP_LDS_AND,
      "_resource($id)", atom_l_and_noret>;
  def ATOM_L_MAX_NORET     : BinAtomNoRet<IL_OP_LDS_MAX,
      "_resource($id)", atom_l_max_noret>;
  def ATOM_L_MIN_NORET     : BinAtomNoRet<IL_OP_LDS_MIN,
      "_resource($id)", atom_l_min_noret>;
  def ATOM_L_UMAX_NORET    : BinAtomNoRet<IL_OP_LDS_UMAX,
      "_resource($id)", atom_l_umax_noret>;
  def ATOM_L_UMIN_NORET    : BinAtomNoRet<IL_OP_LDS_UMIN,
      "_resource($id)", atom_l_umin_noret>;
  def ATOM_L_MSKOR_NORET   : TriAtomNoRet<IL_OP_LDS_MSKOR,
      "_resource($id)", atom_l_mskor_noret>;
  def ATOM_L_OR_NORET      : BinAtomNoRet<IL_OP_LDS_OR,
      "_resource($id)", atom_l_or_noret>;
  def ATOM_L_RSUB_NORET    : BinAtomNoRet<IL_OP_LDS_RSUB,
      "_resource($id)", atom_l_rsub_noret>;
  def ATOM_L_SUB_NORET     : BinAtomNoRet<IL_OP_LDS_SUB,
      "_resource($id)", atom_l_sub_noret>;
  def ATOM_L_XOR_NORET     : BinAtomNoRet<IL_OP_LDS_XOR,
      "_resource($id)", atom_l_xor_noret>;
  def ATOM_L_INC_NORET     : BinAtomNoRet<IL_OP_LDS_INC,
      "_resource($id)", atom_l_inc_noret>;
  def ATOM_L_DEC_NORET     : BinAtomNoRet<IL_OP_LDS_DEC,
      "_resource($id)", atom_l_dec_noret>;
  def ATOM_L_CMPXCHG_NORET    : TriAtomNoRet<IL_OP_LDS_CMP,
      "_resource($id)", atom_l_cmpxchg_noret>;
  def ATOM_R_ADD_NORET     : BinAtomNoRet<IL_OP_GDS_ADD,
      "_resource($id)", atom_r_add_noret>;
  def ATOM_R_AND_NORET     : BinAtomNoRet<IL_OP_GDS_AND,
      "_resource($id)", atom_r_and_noret>;
  def ATOM_R_MAX_NORET     : BinAtomNoRet<IL_OP_GDS_MAX,
      "_resource($id)", atom_r_max_noret>;
  def ATOM_R_MIN_NORET     : BinAtomNoRet<IL_OP_GDS_MIN,
      "_resource($id)", atom_r_min_noret>;
  def ATOM_R_UMAX_NORET    : BinAtomNoRet<IL_OP_GDS_UMAX,
      "_resource($id)", atom_r_umax_noret>;
  def ATOM_R_UMIN_NORET    : BinAtomNoRet<IL_OP_GDS_UMIN,
      "_resource($id)", atom_r_umin_noret>;
  def ATOM_R_MSKOR_NORET   : TriAtomNoRet<IL_OP_GDS_MSKOR,
      "_resource($id)", atom_r_mskor_noret>;
  def ATOM_R_OR_NORET      : BinAtomNoRet<IL_OP_GDS_OR,
      "_resource($id)", atom_r_or_noret>;
  def ATOM_R_RSUB_NORET    : BinAtomNoRet<IL_OP_GDS_RSUB,
      "_resource($id)", atom_r_rsub_noret>;
  def ATOM_R_SUB_NORET     : BinAtomNoRet<IL_OP_GDS_SUB,
      "_resource($id)", atom_r_sub_noret>;
  def ATOM_R_XOR_NORET     : BinAtomNoRet<IL_OP_GDS_XOR,
      "_resource($id)", atom_r_xor_noret>;
  def ATOM_R_INC_NORET     : BinAtomNoRet<IL_OP_GDS_INC,
      "_resource($id)", atom_r_inc_noret>;
  def ATOM_R_DEC_NORET     : BinAtomNoRet<IL_OP_GDS_DEC,
      "_resource($id)", atom_r_dec_noret>;
  def ATOM_R_CMPXCHG_NORET    : CmpXChgNoRet<IL_OP_GDS_CMP,
      "_resource($id)", atom_r_cmpxchg_noret>;
  def APPEND_ALLOC_NORET : AppendNoRet<IL_OP_APPEND_BUF_ALLOC,
      "_id($id)", append_alloc_noret>;
  def APPEND_CONSUME_NORET : AppendNoRet<IL_OP_APPEND_BUF_CONSUME,
      "_id($id)", append_consume_noret>;
  // All of the atomic functions that return
  def ATOM_G_ADD     : BinAtom<IL_OP_UAV_READ_ADD, 
      "_id($id)", atom_g_add>;
  def ATOM_G_AND     : BinAtom<IL_OP_UAV_READ_AND, 
      "_id($id)", atom_g_and>;
  def ATOM_G_MAX     : BinAtom<IL_OP_UAV_READ_MAX, 
      "_id($id)", atom_g_max>;
  def ATOM_G_MIN     : BinAtom<IL_OP_UAV_READ_MIN, 
      "_id($id)", atom_g_min>;
  def ATOM_G_UMAX    : BinAtom<IL_OP_UAV_READ_UMAX, 
      "_id($id)", atom_g_umax>;
  def ATOM_G_UMIN    : BinAtom<IL_OP_UAV_READ_UMIN, 
      "_id($id)", atom_g_umin>;
  def ATOM_G_OR      : BinAtom<IL_OP_UAV_READ_OR, 
      "_id($id)", atom_g_or>;
  def ATOM_G_RSUB    : BinAtom<IL_OP_UAV_READ_RSUB, 
      "_id($id)", atom_g_rsub>;
  def ATOM_G_SUB     : BinAtom<IL_OP_UAV_READ_SUB, 
      "_id($id)", atom_g_sub>;
  def ATOM_G_XOR     : BinAtom<IL_OP_UAV_READ_XOR, 
      "_id($id)", atom_g_xor>;
  def ATOM_G_INC     : BinAtom<IL_OP_UAV_READ_INC, 
      "_id($id)", atom_g_inc>;
  def ATOM_G_DEC     : BinAtom<IL_OP_UAV_READ_DEC, 
      "_id($id)", atom_g_dec>;
  def ATOM_G_XCHG    : BinAtom<IL_OP_UAV_READ_XCHG, 
      "_id($id)", atom_g_xchg>;
  def ATOM_G_CMPXCHG : CmpXChg<IL_OP_UAV_READ_CMPXCHG, 
      "_id($id)", atom_g_cmpxchg>;
  // Arena atomic accesses
  def ATOM_A_ADD     : BinAtom<IL_OP_UAV_READ_ADD, 
      "_id($id)_arena", atom_g_add>;
  def ATOM_A_AND     : BinAtom<IL_OP_UAV_READ_AND, 
      "_id($id)_arena", atom_g_and>;
  def ATOM_A_MAX     : BinAtom<IL_OP_UAV_READ_MAX, 
      "_id($id)_arena", atom_g_max>;
  def ATOM_A_MIN     : BinAtom<IL_OP_UAV_READ_MIN, 
      "_id($id)_arena", atom_g_min>;
  def ATOM_A_UMAX    : BinAtom<IL_OP_UAV_READ_UMAX, 
      "_id($id)_arena", atom_g_umax>;
  def ATOM_A_UMIN    : BinAtom<IL_OP_UAV_READ_UMIN, 
      "_id($id)_arena", atom_g_umin>;
  def ATOM_A_OR      : BinAtom<IL_OP_UAV_READ_OR, 
      "_id($id)_arena", atom_g_or>;
  def ATOM_A_RSUB    : BinAtom<IL_OP_UAV_READ_RSUB, 
      "_id($id)_arena", atom_g_rsub>;
  def ATOM_A_SUB     : BinAtom<IL_OP_UAV_READ_SUB, 
      "_id($id)_arena", atom_g_sub>;
  def ATOM_A_XOR     : BinAtom<IL_OP_UAV_READ_XOR, 
      "_id($id)_arena", atom_g_xor>;
  def ATOM_A_INC     : BinAtom<IL_OP_UAV_READ_INC, 
      "_id($id)_arena", atom_g_inc>;
  def ATOM_A_DEC     : BinAtom<IL_OP_UAV_READ_DEC, 
      "_id($id)_arena", atom_g_dec>;
  def ATOM_A_XCHG    : BinAtom<IL_OP_UAV_READ_XCHG, 
      "_id($id)_arena", atom_g_xchg>;
  def ATOM_A_CMPXCHG : CmpXChg<IL_OP_UAV_READ_CMPXCHG, 
      "_id($id)_arena", atom_g_cmpxchg>;
  def ATOM_L_ADD     : BinAtom<IL_OP_LDS_READ_ADD, 
      "_resource($id)", atom_l_add>;
  def ATOM_L_AND     : BinAtom<IL_OP_LDS_READ_AND, 
      "_resource($id)", atom_l_and>;
  def ATOM_L_MAX     : BinAtom<IL_OP_LDS_READ_MAX, 
      "_resource($id)", atom_l_max>;
  def ATOM_L_MIN     : BinAtom<IL_OP_LDS_READ_MIN, 
      "_resource($id)", atom_l_min>;
  def ATOM_L_UMAX    : BinAtom<IL_OP_LDS_READ_UMAX, 
      "_resource($id)", atom_l_umax>;
  def ATOM_L_UMIN    : BinAtom<IL_OP_LDS_READ_UMIN, 
      "_resource($id)", atom_l_umin>;
  def ATOM_L_OR      : BinAtom<IL_OP_LDS_READ_OR, 
      "_resource($id)", atom_l_or>;
  def ATOM_L_MSKOR   : TriAtom<IL_OP_LDS_READ_MSKOR, 
      "_resource($id)", atom_l_mskor>;
  def ATOM_L_RSUB    : BinAtom<IL_OP_LDS_READ_RSUB, 
      "_resource($id)", atom_l_rsub>;
  def ATOM_L_SUB     : BinAtom<IL_OP_LDS_READ_SUB, 
      "_resource($id)", atom_l_sub>;
  def ATOM_L_XOR     : BinAtom<IL_OP_LDS_READ_XOR, 
      "_resource($id)", atom_l_xor>;
  def ATOM_L_INC     : BinAtom<IL_OP_LDS_READ_INC, 
      "_resource($id)", atom_l_inc>;
  def ATOM_L_DEC     : BinAtom<IL_OP_LDS_READ_DEC, 
      "_resource($id)", atom_l_dec>;
  def ATOM_L_XCHG    : BinAtom<IL_OP_LDS_READ_XCHG, 
      "_resource($id)", atom_l_xchg>;
  def ATOM_L_CMPXCHG : TriAtom<IL_OP_LDS_READ_CMPXCHG, 
      "_resource($id)", atom_l_cmpxchg>;
  def ATOM_R_ADD     : BinAtom<IL_OP_GDS_READ_ADD, 
      "_resource($id)", atom_r_add>;
  def ATOM_R_AND     : BinAtom<IL_OP_GDS_READ_AND, 
      "_resource($id)", atom_r_and>;
  def ATOM_R_MAX     : BinAtom<IL_OP_GDS_READ_MAX, 
      "_resource($id)", atom_r_max>;
  def ATOM_R_MIN     : BinAtom<IL_OP_GDS_READ_MIN, 
      "_resource($id)", atom_r_min>;
  def ATOM_R_UMAX    : BinAtom<IL_OP_GDS_READ_UMAX, 
      "_resource($id)", atom_r_umax>;
  def ATOM_R_UMIN    : BinAtom<IL_OP_GDS_READ_UMIN, 
      "_resource($id)", atom_r_umin>;
  def ATOM_R_OR      : BinAtom<IL_OP_GDS_READ_OR, 
      "_resource($id)", atom_r_or>;
  def ATOM_R_MSKOR   : TriAtom<IL_OP_GDS_READ_MSKOR, 
      "_resource($id)", atom_r_mskor>;
  def ATOM_R_RSUB    : BinAtom<IL_OP_GDS_READ_RSUB, 
      "_resource($id)", atom_r_rsub>;
  def ATOM_R_SUB     : BinAtom<IL_OP_GDS_READ_SUB, 
      "_resource($id)", atom_r_sub>;
  def ATOM_R_XOR     : BinAtom<IL_OP_GDS_READ_XOR, 
      "_resource($id)", atom_r_xor>;
  def ATOM_R_INC     : BinAtom<IL_OP_GDS_READ_INC, 
      "_resource($id)", atom_r_inc>;
  def ATOM_R_DEC     : BinAtom<IL_OP_GDS_READ_DEC, 
      "_resource($id)", atom_r_dec>;
  def ATOM_R_XCHG    : BinAtom<IL_OP_GDS_READ_XCHG, 
      "_resource($id)", atom_r_xchg>;
  def ATOM_R_CMPXCHG : CmpXChg<IL_OP_GDS_READ_CMPXCHG, 
      "_resource($id)", atom_r_cmpxchg>;
  def APPEND_ALLOC : Append<IL_OP_APPEND_BUF_ALLOC,
      "_id($id)", append_alloc>;
  def APPEND_CONSUME : Append<IL_OP_APPEND_BUF_CONSUME,
      "_id($id)", append_consume>;
}
}
let Predicates = [Has64BitPtr] in {
// All of the image functions
def IMAGE1D64_READ : ILFormat<IL_OP_SAMPLE, (outs GPRV4I32:$dst),
    (ins MEMI64:$ptr, GPRI32:$sampler, GPRV4F32:$addy),
    !strconcat(IL_OP_SAMPLE.Text, 
        "_resource($ptr)_sampler($sampler)_coordtype(normalized) $dst, $addy"),
    [(set GPRV4I32:$dst,
        (int_AMDIL_image1d_read_norm ADDR64:$ptr, GPRI32:$sampler, GPRV4F32:$addy))]>;
def IMAGE1DA64_READ : ILFormat<IL_OP_SAMPLE, (outs GPRV4I32:$dst),
    (ins MEMI64:$ptr, GPRI32:$sampler, GPRV4F32:$addy),
    !strconcat(IL_OP_SAMPLE.Text, 
        "_resource($ptr)_sampler($sampler)_coordtype(normalized) $dst, $addy"),
    [(set GPRV4I32:$dst,
        (int_AMDIL_image1d_array_read_norm ADDR64:$ptr, GPRI32:$sampler, GPRV4F32:$addy))]>;
def IMAGE2D64_READ : ILFormat<IL_OP_SAMPLE, (outs GPRV4I32:$dst),
    (ins MEMI64:$ptr, GPRI32:$sampler, GPRV4F32:$addy),
    !strconcat(IL_OP_SAMPLE.Text, 
        "_resource($ptr)_sampler($sampler)_coordtype(normalized) $dst, $addy"),
    [(set GPRV4I32:$dst,
        (int_AMDIL_image2d_read_norm ADDR64:$ptr, GPRI32:$sampler, GPRV4F32:$addy))]>;
def IMAGE2DA64_READ : ILFormat<IL_OP_SAMPLE, (outs GPRV4I32:$dst),
    (ins MEMI64:$ptr, GPRI32:$sampler, GPRV4F32:$addy),
    !strconcat(IL_OP_SAMPLE.Text, 
        "_resource($ptr)_sampler($sampler)_coordtype(normalized) $dst, $addy"),
    [(set GPRV4I32:$dst,
        (int_AMDIL_image2d_array_read_norm ADDR64:$ptr, GPRI32:$sampler, GPRV4F32:$addy))]>;
def IMAGE3D64_READ : ILFormat<IL_OP_SAMPLE, (outs GPRV4I32:$dst),
    (ins MEMI64:$ptr, GPRI32:$sampler, GPRV4F32:$addy),
    !strconcat(IL_OP_SAMPLE.Text, 
        "_resource($ptr)_sampler($sampler)_coordtype(normalized) $dst, $addy"),
    [(set GPRV4I32:$dst,
        (int_AMDIL_image3d_read_norm ADDR64:$ptr, GPRI32:$sampler, GPRV4F32:$addy))]>;
def IMAGE1D64_READ_UNNORM : ILFormat<IL_OP_SAMPLE, (outs GPRV4I32:$dst),
    (ins MEMI64:$ptr, GPRI32:$sampler, GPRV4F32:$addy),
    !strconcat(IL_OP_SAMPLE.Text, 
        "_resource($ptr)_sampler($sampler)_coordtype(unnormalized) $dst, $addy"),
    [(set GPRV4I32:$dst,
        (int_AMDIL_image1d_read_unnorm ADDR64:$ptr, GPRI32:$sampler, GPRV4F32:$addy))]>;
def IMAGE1DA64_READ_UNNORM : ILFormat<IL_OP_SAMPLE, (outs GPRV4I32:$dst),
    (ins MEMI64:$ptr, GPRI32:$sampler, GPRV4F32:$addy),
    !strconcat(IL_OP_SAMPLE.Text, 
        "_resource($ptr)_sampler($sampler)_coordtype(unnormalized) $dst, $addy"),
    [(set GPRV4I32:$dst,
        (int_AMDIL_image1d_array_read_unnorm ADDR64:$ptr, GPRI32:$sampler, GPRV4F32:$addy))]>;
def IMAGE2D64_READ_UNNORM : ILFormat<IL_OP_SAMPLE, (outs GPRV4I32:$dst),
    (ins MEMI64:$ptr, GPRI32:$sampler, GPRV4F32:$addy),
    !strconcat(IL_OP_SAMPLE.Text, 
        "_resource($ptr)_sampler($sampler)_coordtype(unnormalized) $dst, $addy"),
    [(set GPRV4I32:$dst,
        (int_AMDIL_image2d_read_unnorm ADDR64:$ptr, GPRI32:$sampler, GPRV4F32:$addy))]>;
def IMAGE2DA64_READ_UNNORM : ILFormat<IL_OP_SAMPLE, (outs GPRV4I32:$dst),
    (ins MEMI64:$ptr, GPRI32:$sampler, GPRV4F32:$addy),
    !strconcat(IL_OP_SAMPLE.Text, 
        "_resource($ptr)_sampler($sampler)_coordtype(unnormalized) $dst, $addy"),
    [(set GPRV4I32:$dst,
        (int_AMDIL_image2d_array_read_unnorm ADDR64:$ptr, GPRI32:$sampler, GPRV4F32:$addy))]>;
def IMAGE3D64_READ_UNNORM : ILFormat<IL_OP_SAMPLE, (outs GPRV4I32:$dst),
    (ins MEMI64:$ptr, GPRI32:$sampler, GPRV4F32:$addy),
    !strconcat(IL_OP_SAMPLE.Text, 
        "_resource($ptr)_sampler($sampler)_coordtype(unnormalized) $dst, $addy"),
    [(set GPRV4I32:$dst,
        (int_AMDIL_image3d_read_unnorm ADDR64:$ptr, GPRI32:$sampler, GPRV4F32:$addy))]>;
def IMAGE1D64_INFO0 : ILFormat<IL_OP_MOV, (outs GPRV4I32:$dst),
    (ins MEMI64:$ptr),
    !strconcat(IL_OP_MOV.Text, " $dst, $ptr"),
    [(set GPRV4I32:$dst, (int_AMDIL_image1d_info0 ADDR64:$ptr))]>;
def IMAGE1D64_INFO1 : ILFormat<IL_OP_MOV, (outs GPRV4I32:$dst),
    (ins MEMI64:$ptr),
    !strconcat(IL_OP_MOV.Text, " $dst, $ptr"),
    [(set GPRV4I32:$dst, (int_AMDIL_image1d_info1 ADDR64:$ptr))]>;
def IMAGE1DA64_INFO0 : ILFormat<IL_OP_MOV, (outs GPRV4I32:$dst),
    (ins MEMI64:$ptr),
    !strconcat(IL_OP_MOV.Text, " $dst, $ptr"),
    [(set GPRV4I32:$dst, (int_AMDIL_image1d_array_info0 ADDR64:$ptr))]>;
def IMAGE1DA64_INFO1 : ILFormat<IL_OP_MOV, (outs GPRV4I32:$dst),
    (ins MEMI64:$ptr),
    !strconcat(IL_OP_MOV.Text, " $dst, $ptr"),
    [(set GPRV4I32:$dst, (int_AMDIL_image1d_array_info1 ADDR64:$ptr))]>;
def IMAGE2DA64_INFO0 : ILFormat<IL_OP_MOV, (outs GPRV4I32:$dst),
    (ins MEMI64:$ptr),
    !strconcat(IL_OP_MOV.Text, " $dst, $ptr"),
    [(set GPRV4I32:$dst, (int_AMDIL_image2d_array_info0 ADDR64:$ptr))]>;
def IMAGE2DA64_INFO1 : ILFormat<IL_OP_MOV, (outs GPRV4I32:$dst),
    (ins MEMI64:$ptr),
    !strconcat(IL_OP_MOV.Text, " $dst, $ptr"),
    [(set GPRV4I32:$dst, (int_AMDIL_image2d_array_info1 ADDR64:$ptr))]>;
def IMAGE2D64_INFO0 : ILFormat<IL_OP_MOV, (outs GPRV4I32:$dst),
    (ins MEMI64:$ptr),
    !strconcat(IL_OP_MOV.Text, " $dst, $ptr"),
    [(set GPRV4I32:$dst, (int_AMDIL_image2d_info0 ADDR64:$ptr))]>;
def IMAGE2D64_INFO1 : ILFormat<IL_OP_MOV, (outs GPRV4I32:$dst),
    (ins MEMI64:$ptr),
    !strconcat(IL_OP_MOV.Text, " $dst, $ptr"),
    [(set GPRV4I32:$dst, (int_AMDIL_image2d_info1 ADDR64:$ptr))]>;
def IMAGE3D64_INFO0 : ILFormat<IL_OP_MOV, (outs GPRV4I32:$dst),
    (ins MEMI64:$ptr),
    !strconcat(IL_OP_MOV.Text, " $dst, $ptr"),
    [(set GPRV4I32:$dst, (int_AMDIL_image3d_info0 ADDR64:$ptr))]>;
def IMAGE3D64_INFO1 : ILFormat<IL_OP_MOV, (outs GPRV4I32:$dst),
    (ins MEMI64:$ptr),
    !strconcat(IL_OP_MOV.Text, " $dst, $ptr"),
    [(set GPRV4I32:$dst, (int_AMDIL_image3d_info1 ADDR64:$ptr))]>;
def IMAGE1D64_WRITE : ILFormat<IL_OP_UAV_STORE, (outs),
    (ins MEMI64:$ptr, GPRV2I32:$addy, GPRV4I32:$data),
    !strconcat(IL_OP_UAV_STORE.Text,
        "_id($ptr) $addy, $data"),
    [(int_AMDIL_image1d_write ADDR64:$ptr, GPRV2I32:$addy, GPRV4I32:$data)]>;
def IMAGE1DA64_WRITE : ILFormat<IL_OP_UAV_STORE, (outs),
    (ins MEMI64:$ptr, GPRV2I32:$addy, GPRV4I32:$data),
    !strconcat(IL_OP_UAV_STORE.Text,
        "_id($ptr) $addy, $data"),
    [(int_AMDIL_image1d_array_write ADDR64:$ptr, GPRV2I32:$addy, GPRV4I32:$data)]>;
def IMAGE2D64_WRITE : ILFormat<IL_OP_UAV_STORE, (outs),
    (ins MEMI64:$ptr, GPRV2I32:$addy, GPRV4I32:$data),
    !strconcat(IL_OP_UAV_STORE.Text,
        "_id($ptr) $addy, $data"),
    [(int_AMDIL_image2d_write ADDR64:$ptr, GPRV2I32:$addy, GPRV4I32:$data)]>;
def IMAGE2DA64_WRITE : ILFormat<IL_OP_UAV_STORE, (outs),
    (ins MEMI64:$ptr, GPRV2I32:$addy, GPRV4I32:$data),
    !strconcat(IL_OP_UAV_STORE.Text,
        "_id($ptr) $addy, $data"),
    [(int_AMDIL_image2d_array_write ADDR64:$ptr, GPRV2I32:$addy, GPRV4I32:$data)]>;
def IMAGE3D64_WRITE : ILFormat<IL_OP_UAV_STORE, (outs),
    (ins MEMI64:$ptr, GPRV4I32:$addy, GPRV4I32:$data),
    !strconcat(IL_OP_UAV_STORE.Text,
        "_id($ptr) $addy, $data"),
    [(int_AMDIL_image3d_write ADDR64:$ptr, GPRV4I32:$addy, GPRV4I32:$data)]>;
let hasSideEffects= 1 in {
  // All of the noret atomic functions
  def ATOM_G64_ADD_NORET     : BinAtomNoRet64<IL_OP_UAV_ADD,
      "_id($id)", atom_g_add_noret>;
  def ATOM_G64_AND_NORET     : BinAtomNoRet64<IL_OP_UAV_AND,
      "_id($id)", atom_g_and_noret>;
  def ATOM_G64_MAX_NORET     : BinAtomNoRet64<IL_OP_UAV_MAX,
      "_id($id)", atom_g_max_noret>;
  def ATOM_G64_MIN_NORET     : BinAtomNoRet64<IL_OP_UAV_MIN,
      "_id($id)", atom_g_min_noret>;
  def ATOM_G64_UMAX_NORET    : BinAtomNoRet64<IL_OP_UAV_UMAX,
      "_id($id)", atom_g_umax_noret>;
  def ATOM_G64_UMIN_NORET    : BinAtomNoRet64<IL_OP_UAV_UMIN,
      "_id($id)", atom_g_umin_noret>;
  def ATOM_G64_OR_NORET      : BinAtomNoRet64<IL_OP_UAV_OR,
      "_id($id)", atom_g_or_noret>;
  def ATOM_G64_RSUB_NORET    : BinAtomNoRet64<IL_OP_UAV_RSUB,
      "_id($id)", atom_g_rsub_noret>;
  def ATOM_G64_SUB_NORET     : BinAtomNoRet64<IL_OP_UAV_SUB,
      "_id($id)", atom_g_sub_noret>;
  def ATOM_G64_XOR_NORET     : BinAtomNoRet64<IL_OP_UAV_XOR,
      "_id($id)", atom_g_xor_noret>;
  def ATOM_G64_INC_NORET     : BinAtomNoRet64<IL_OP_UAV_INC,
      "_id($id)", atom_g_inc_noret>;
  def ATOM_G64_DEC_NORET     : BinAtomNoRet64<IL_OP_UAV_DEC,
      "_id($id)", atom_g_dec_noret>;
  def ATOM_G64_CMPXCHG_NORET    : CmpXChgNoRet64<IL_OP_UAV_CMP,
      "_id($id)", atom_g_cmpxchg_noret>;
  def ATOM_A64_ADD_NORET     : BinAtomNoRet64<IL_OP_UAV_ADD,
      "_id($id)_arena", atom_g_add_noret>;
  def ATOM_A64_AND_NORET     : BinAtomNoRet64<IL_OP_UAV_AND,
      "_id($id)_arena", atom_g_and_noret>;
  def ATOM_A64_MAX_NORET     : BinAtomNoRet64<IL_OP_UAV_MAX,
      "_id($id)_arena", atom_g_max_noret>;
  def ATOM_A64_MIN_NORET     : BinAtomNoRet64<IL_OP_UAV_MIN,
      "_id($id)_arena", atom_g_min_noret>;
  def ATOM_A64_UMAX_NORET    : BinAtomNoRet64<IL_OP_UAV_UMAX,
      "_id($id)_arena", atom_g_umax_noret>;
  def ATOM_A64_UMIN_NORET    : BinAtomNoRet64<IL_OP_UAV_UMIN,
      "_id($id)_arena", atom_g_umin_noret>;
  def ATOM_A64_OR_NORET      : BinAtomNoRet64<IL_OP_UAV_OR,
      "_id($id)_arena", atom_g_or_noret>;
  def ATOM_A64_RSUB_NORET    : BinAtomNoRet64<IL_OP_UAV_RSUB,
      "_id($id)_arena", atom_g_rsub_noret>;
  def ATOM_A64_SUB_NORET     : BinAtomNoRet64<IL_OP_UAV_SUB,
      "_id($id)_arena", atom_g_sub_noret>;
  def ATOM_A64_XOR_NORET     : BinAtomNoRet64<IL_OP_UAV_XOR,
      "_id($id)_arena", atom_g_xor_noret>;
  def ATOM_A64_INC_NORET     : BinAtomNoRet64<IL_OP_UAV_INC,
      "_id($id)_arena", atom_g_inc_noret>;
  def ATOM_A64_DEC_NORET     : BinAtomNoRet64<IL_OP_UAV_DEC,
      "_id($id)_arena", atom_g_dec_noret>;
  def ATOM_A64_CMPXCHG_NORET    : CmpXChgNoRet64<IL_OP_UAV_CMP,
      "_id($id)_arena", atom_g_cmpxchg_noret>;
  def ATOM_L64_ADD_NORET     : BinAtomNoRet64<IL_OP_LDS_ADD,
      "_resource($id)", atom_l_add_noret>;
  def ATOM_L64_AND_NORET     : BinAtomNoRet64<IL_OP_LDS_AND,
      "_resource($id)", atom_l_and_noret>;
  def ATOM_L64_MAX_NORET     : BinAtomNoRet64<IL_OP_LDS_MAX,
      "_resource($id)", atom_l_max_noret>;
  def ATOM_L64_MIN_NORET     : BinAtomNoRet64<IL_OP_LDS_MIN,
      "_resource($id)", atom_l_min_noret>;
  def ATOM_L64_UMAX_NORET    : BinAtomNoRet64<IL_OP_LDS_UMAX,
      "_resource($id)", atom_l_umax_noret>;
  def ATOM_L64_UMIN_NORET    : BinAtomNoRet64<IL_OP_LDS_UMIN,
      "_resource($id)", atom_l_umin_noret>;
  def ATOM_L64_MSKOR_NORET   : TriAtomNoRet64<IL_OP_LDS_MSKOR,
      "_resource($id)", atom_l_mskor_noret>;
  def ATOM_L64_OR_NORET      : BinAtomNoRet64<IL_OP_LDS_OR,
      "_resource($id)", atom_l_or_noret>;
  def ATOM_L64_RSUB_NORET    : BinAtomNoRet64<IL_OP_LDS_RSUB,
      "_resource($id)", atom_l_rsub_noret>;
  def ATOM_L64_SUB_NORET     : BinAtomNoRet64<IL_OP_LDS_SUB,
      "_resource($id)", atom_l_sub_noret>;
  def ATOM_L64_XOR_NORET     : BinAtomNoRet64<IL_OP_LDS_XOR,
      "_resource($id)", atom_l_xor_noret>;
  def ATOM_L64_INC_NORET     : BinAtomNoRet64<IL_OP_LDS_INC,
      "_resource($id)", atom_l_inc_noret>;
  def ATOM_L64_DEC_NORET     : BinAtomNoRet64<IL_OP_LDS_DEC,
      "_resource($id)", atom_l_dec_noret>;
  def ATOM_L64_CMPXCHG_NORET    : TriAtomNoRet64<IL_OP_LDS_CMP,
      "_resource($id)", atom_l_cmpxchg_noret>;
  def ATOM_R64_ADD_NORET     : BinAtomNoRet64<IL_OP_GDS_ADD,
      "_resource($id)", atom_r_add_noret>;
  def ATOM_R64_AND_NORET     : BinAtomNoRet64<IL_OP_GDS_AND,
      "_resource($id)", atom_r_and_noret>;
  def ATOM_R64_MAX_NORET     : BinAtomNoRet64<IL_OP_GDS_MAX,
      "_resource($id)", atom_r_max_noret>;
  def ATOM_R64_MIN_NORET     : BinAtomNoRet64<IL_OP_GDS_MIN,
      "_resource($id)", atom_r_min_noret>;
  def ATOM_R64_UMAX_NORET    : BinAtomNoRet64<IL_OP_GDS_UMAX,
      "_resource($id)", atom_r_umax_noret>;
  def ATOM_R64_UMIN_NORET    : BinAtomNoRet64<IL_OP_GDS_UMIN,
      "_resource($id)", atom_r_umin_noret>;
  def ATOM_R64_MSKOR_NORET   : TriAtomNoRet64<IL_OP_GDS_MSKOR,
      "_resource($id)", atom_r_mskor_noret>;
  def ATOM_R64_OR_NORET      : BinAtomNoRet64<IL_OP_GDS_OR,
      "_resource($id)", atom_r_or_noret>;
  def ATOM_R64_RSUB_NORET    : BinAtomNoRet64<IL_OP_GDS_RSUB,
      "_resource($id)", atom_r_rsub_noret>;
  def ATOM_R64_SUB_NORET     : BinAtomNoRet64<IL_OP_GDS_SUB,
      "_resource($id)", atom_r_sub_noret>;
  def ATOM_R64_XOR_NORET     : BinAtomNoRet64<IL_OP_GDS_XOR,
      "_resource($id)", atom_r_xor_noret>;
  def ATOM_R64_INC_NORET     : BinAtomNoRet64<IL_OP_GDS_INC,
      "_resource($id)", atom_r_inc_noret>;
  def ATOM_R64_DEC_NORET     : BinAtomNoRet64<IL_OP_GDS_DEC,
      "_resource($id)", atom_r_dec_noret>;
  def ATOM_R64_CMPXCHG_NORET    : CmpXChgNoRet64<IL_OP_GDS_CMP,
      "_resource($id)", atom_r_cmpxchg_noret>;
  def APPEND_ALLOC64_NORET : AppendNoRet64<IL_OP_APPEND_BUF_ALLOC,
      "_id($id)", append_alloc_noret>;
  def APPEND_CONSUME64_NORET : AppendNoRet64<IL_OP_APPEND_BUF_CONSUME,
      "_id($id)", append_consume_noret>;
  // All of the atomic functions that return
  def ATOM_G64_ADD     : BinAtom64<IL_OP_UAV_READ_ADD, 
      "_id($id)", atom_g_add>;
  def ATOM_G64_AND     : BinAtom64<IL_OP_UAV_READ_AND, 
      "_id($id)", atom_g_and>;
  def ATOM_G64_MAX     : BinAtom64<IL_OP_UAV_READ_MAX, 
      "_id($id)", atom_g_max>;
  def ATOM_G64_MIN     : BinAtom64<IL_OP_UAV_READ_MIN, 
      "_id($id)", atom_g_min>;
  def ATOM_G64_UMAX    : BinAtom64<IL_OP_UAV_READ_UMAX, 
      "_id($id)", atom_g_umax>;
  def ATOM_G64_UMIN    : BinAtom64<IL_OP_UAV_READ_UMIN, 
      "_id($id)", atom_g_umin>;
  def ATOM_G64_OR      : BinAtom64<IL_OP_UAV_READ_OR, 
      "_id($id)", atom_g_or>;
  def ATOM_G64_RSUB    : BinAtom64<IL_OP_UAV_READ_RSUB, 
      "_id($id)", atom_g_rsub>;
  def ATOM_G64_SUB     : BinAtom64<IL_OP_UAV_READ_SUB, 
      "_id($id)", atom_g_sub>;
  def ATOM_G64_XOR     : BinAtom64<IL_OP_UAV_READ_XOR, 
      "_id($id)", atom_g_xor>;
  def ATOM_G64_INC     : BinAtom64<IL_OP_UAV_READ_INC, 
      "_id($id)", atom_g_inc>;
  def ATOM_G64_DEC     : BinAtom64<IL_OP_UAV_READ_DEC, 
      "_id($id)", atom_g_dec>;
  def ATOM_G64_XCHG    : BinAtom64<IL_OP_UAV_READ_XCHG, 
      "_id($id)", atom_g_xchg>;
  def ATOM_G64_CMPXCHG : CmpXChg64<IL_OP_UAV_READ_CMPXCHG, 
      "_id($id)", atom_g_cmpxchg>;
  // Arena atomic accesses
  def ATOM_A64_ADD     : BinAtom64<IL_OP_UAV_READ_ADD, 
      "_id($id)_arena", atom_g_add>;
  def ATOM_A64_AND     : BinAtom64<IL_OP_UAV_READ_AND, 
      "_id($id)_arena", atom_g_and>;
  def ATOM_A64_MAX     : BinAtom64<IL_OP_UAV_READ_MAX, 
      "_id($id)_arena", atom_g_max>;
  def ATOM_A64_MIN     : BinAtom64<IL_OP_UAV_READ_MIN, 
      "_id($id)_arena", atom_g_min>;
  def ATOM_A64_UMAX    : BinAtom64<IL_OP_UAV_READ_UMAX, 
      "_id($id)_arena", atom_g_umax>;
  def ATOM_A64_UMIN    : BinAtom64<IL_OP_UAV_READ_UMIN, 
      "_id($id)_arena", atom_g_umin>;
  def ATOM_A64_OR      : BinAtom64<IL_OP_UAV_READ_OR, 
      "_id($id)_arena", atom_g_or>;
  def ATOM_A64_RSUB    : BinAtom64<IL_OP_UAV_READ_RSUB, 
      "_id($id)_arena", atom_g_rsub>;
  def ATOM_A64_SUB     : BinAtom64<IL_OP_UAV_READ_SUB, 
      "_id($id)_arena", atom_g_sub>;
  def ATOM_A64_XOR     : BinAtom64<IL_OP_UAV_READ_XOR, 
      "_id($id)_arena", atom_g_xor>;
  def ATOM_A64_INC     : BinAtom64<IL_OP_UAV_READ_INC, 
      "_id($id)_arena", atom_g_inc>;
  def ATOM_A64_DEC     : BinAtom64<IL_OP_UAV_READ_DEC, 
      "_id($id)_arena", atom_g_dec>;
  def ATOM_A64_XCHG    : BinAtom64<IL_OP_UAV_READ_XCHG, 
      "_id($id)_arena", atom_g_xchg>;
  def ATOM_A64_CMPXCHG : CmpXChg64<IL_OP_UAV_READ_CMPXCHG, 
      "_id($id)_arena", atom_g_cmpxchg>;
  def ATOM_L64_ADD     : BinAtom64<IL_OP_LDS_READ_ADD, 
      "_resource($id)", atom_l_add>;
  def ATOM_L64_AND     : BinAtom64<IL_OP_LDS_READ_AND, 
      "_resource($id)", atom_l_and>;
  def ATOM_L64_MAX     : BinAtom64<IL_OP_LDS_READ_MAX, 
      "_resource($id)", atom_l_max>;
  def ATOM_L64_MIN     : BinAtom64<IL_OP_LDS_READ_MIN, 
      "_resource($id)", atom_l_min>;
  def ATOM_L64_UMAX    : BinAtom64<IL_OP_LDS_READ_UMAX, 
      "_resource($id)", atom_l_umax>;
  def ATOM_L64_UMIN    : BinAtom64<IL_OP_LDS_READ_UMIN, 
      "_resource($id)", atom_l_umin>;
  def ATOM_L64_OR      : BinAtom64<IL_OP_LDS_READ_OR, 
      "_resource($id)", atom_l_or>;
  def ATOM_L64_MSKOR   : TriAtom64<IL_OP_LDS_READ_MSKOR, 
      "_resource($id)", atom_l_mskor>;
  def ATOM_L64_RSUB    : BinAtom64<IL_OP_LDS_READ_RSUB, 
      "_resource($id)", atom_l_rsub>;
  def ATOM_L64_SUB     : BinAtom64<IL_OP_LDS_READ_SUB, 
      "_resource($id)", atom_l_sub>;
  def ATOM_L64_XOR     : BinAtom64<IL_OP_LDS_READ_XOR, 
      "_resource($id)", atom_l_xor>;
  def ATOM_L64_INC     : BinAtom64<IL_OP_LDS_READ_INC, 
      "_resource($id)", atom_l_inc>;
  def ATOM_L64_DEC     : BinAtom64<IL_OP_LDS_READ_DEC, 
      "_resource($id)", atom_l_dec>;
  def ATOM_L64_XCHG    : BinAtom64<IL_OP_LDS_READ_XCHG, 
      "_resource($id)", atom_l_xchg>;
  def ATOM_L64_CMPXCHG : TriAtom64<IL_OP_LDS_READ_CMPXCHG, 
      "_resource($id)", atom_l_cmpxchg>;
  def ATOM_R64_ADD     : BinAtom64<IL_OP_GDS_READ_ADD, 
      "_resource($id)", atom_r_add>;
  def ATOM_R64_AND     : BinAtom64<IL_OP_GDS_READ_AND, 
      "_resource($id)", atom_r_and>;
  def ATOM_R64_MAX     : BinAtom64<IL_OP_GDS_READ_MAX, 
      "_resource($id)", atom_r_max>;
  def ATOM_R64_MIN     : BinAtom64<IL_OP_GDS_READ_MIN, 
      "_resource($id)", atom_r_min>;
  def ATOM_R64_UMAX    : BinAtom64<IL_OP_GDS_READ_UMAX, 
      "_resource($id)", atom_r_umax>;
  def ATOM_R64_UMIN    : BinAtom64<IL_OP_GDS_READ_UMIN, 
      "_resource($id)", atom_r_umin>;
  def ATOM_R64_OR      : BinAtom64<IL_OP_GDS_READ_OR, 
      "_resource($id)", atom_r_or>;
  def ATOM_R64_MSKOR   : TriAtom64<IL_OP_GDS_READ_MSKOR, 
      "_resource($id)", atom_r_mskor>;
  def ATOM_R64_RSUB    : BinAtom64<IL_OP_GDS_READ_RSUB, 
      "_resource($id)", atom_r_rsub>;
  def ATOM_R64_SUB     : BinAtom64<IL_OP_GDS_READ_SUB, 
      "_resource($id)", atom_r_sub>;
  def ATOM_R64_XOR     : BinAtom64<IL_OP_GDS_READ_XOR, 
      "_resource($id)", atom_r_xor>;
  def ATOM_R64_INC     : BinAtom64<IL_OP_GDS_READ_INC, 
      "_resource($id)", atom_r_inc>;
  def ATOM_R64_DEC     : BinAtom64<IL_OP_GDS_READ_DEC, 
      "_resource($id)", atom_r_dec>;
  def ATOM_R64_XCHG    : BinAtom64<IL_OP_GDS_READ_XCHG, 
      "_resource($id)", atom_r_xchg>;
  def ATOM_R64_CMPXCHG : CmpXChg64<IL_OP_GDS_READ_CMPXCHG, 
      "_resource($id)", atom_r_cmpxchg>;
  def APPEND_ALLOC64 : Append64<IL_OP_APPEND_BUF_ALLOC,
      "_id($id)", append_alloc>;
  def APPEND_CONSUME64 : Append64<IL_OP_APPEND_BUF_CONSUME,
      "_id($id)", append_consume>;
}
}
/*
def SEMAPHORE_INIT : BinaryOpNoRet<IL_OP_SEMAPHORE_INIT, (outs),
    (ins MEMI32:$ptr, i32imm:$val),
    !strconcat(IL_OP_SEMAPHORE_INIT.Text, "_id($ptr)_value($val)"),
    [(int_AMDIL_semaphore_init ADDR:$ptr, timm:$val)]>;

def SEMAPHORE_WAIT : UnaryOpNoRet<IL_OP_SEMAPHORE_WAIT, (outs), 
    (ins MEMI32:$ptr),
    !strconcat(IL_OP_SEMAPHORE_WAIT.Text, "_id($ptr)"),
    [(int_AMDIL_semaphore_wait ADDR:$ptr)]>;

def SEMAPHORE_SIGNAL : UnaryOpNoRet<IL_OP_SEMAPHORE_SIGNAL, (outs), 
    (ins MEMI32:$ptr),
    !strconcat(IL_OP_SEMAPHORE_SIGNAL.Text, "_id($ptr)"),
    [(int_AMDIL_semaphore_signal ADDR:$ptr)]>;
*/

//===- AMDILInstrPatterns.td - AMDIL Target ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//
//===--------------------------------------------------------------------===//
// This file holds all the custom patterns that are used by the amdil backend
//
//===--------------------------------------------------------------------===//
//===--------------------------------------------------------------------===//
// Custom patterns for conversion operations
//===--------------------------------------------------------------------===////
// Pattern to remap integer or to IL_or
def : Pat<(i32 (or GPRI32:$src0, GPRI32:$src1)),
          (i32 (BINARY_OR_i32 GPRI32:$src0, GPRI32:$src1))>;
// float ==> long patterns
// unsigned: f32 -> i64
def FTOUL : Pat<(i64 (fp_to_uint GPRF32:$src)),
    (LCREATE (FTOU GPRF32:$src), (LOADCONST_i32 0))>;
// signed: f32 -> i64
def FTOL : Pat<(i64 (fp_to_sint GPRF32:$src)),
    (LCREATE (FTOI GPRF32:$src), (LOADCONST_i32 0))>;
// unsigned: i64 -> f32
def ULTOF : Pat<(f32 (uint_to_fp GPRI64:$src)),
    (UTOF (LLO GPRI64:$src))>;
// signed: i64 -> f32
def LTOF : Pat<(f32 (sint_to_fp GPRI64:$src)),
    (ITOF (LLO GPRI64:$src))>;

// integer subtraction
// a - b ==> a + (-b)
def SUB_i8 : Pat<(sub GPRI8:$src0, GPRI8:$src1),
    (ADD_i8 GPRI8:$src0, (NEGATE_i8 GPRI8:$src1))>;
def SUB_v2i8 : Pat<(sub GPRV2I8:$src0, GPRV2I8:$src1),
    (ADD_v2i8 GPRV2I8:$src0, (NEGATE_v2i8 GPRV2I8:$src1))>;
def SUB_v4i8 : Pat<(sub GPRV4I8:$src0, GPRV4I8:$src1),
    (ADD_v4i8 GPRV4I8:$src0, (NEGATE_v4i8 GPRV4I8:$src1))>;
def SUB_i16 : Pat<(sub GPRI16:$src0, GPRI16:$src1),
    (ADD_i16 GPRI16:$src0, (NEGATE_i16 GPRI16:$src1))>;
def SUB_v2i16 : Pat<(sub GPRV2I16:$src0, GPRV2I16:$src1),
    (ADD_v2i16 GPRV2I16:$src0, (NEGATE_v2i16 GPRV2I16:$src1))>;
def SUB_v4i16 : Pat<(sub GPRV4I16:$src0, GPRV4I16:$src1),
    (ADD_v4i16 GPRV4I16:$src0, (NEGATE_v4i16 GPRV4I16:$src1))>;
def SUB_i32 : Pat<(sub GPRI32:$src0, GPRI32:$src1),
    (ADD_i32 GPRI32:$src0, (NEGATE_i32 GPRI32:$src1))>;
def SUB_v2i32 : Pat<(sub GPRV2I32:$src0, GPRV2I32:$src1),
    (ADD_v2i32 GPRV2I32:$src0, (NEGATE_v2i32 GPRV2I32:$src1))>;
def SUB_v4i32 : Pat<(sub GPRV4I32:$src0, GPRV4I32:$src1),
    (ADD_v4i32 GPRV4I32:$src0, (NEGATE_v4i32 GPRV4I32:$src1))>;
// LLVM isn't lowering this correctly, so writing a pattern that
// matches it isntead.
def : Pat<(build_vector (i32 imm:$src)),
    (VCREATE_v4i32 (LOADCONST_i32 imm:$src))>;

// Calls:
def : Pat<(IL_call tglobaladdr:$dst),
    (CALL tglobaladdr:$dst)>;
def : Pat<(IL_call texternalsym:$dst),
    (CALL texternalsym:$dst)>;
def : Pat<(IL_call tconstpool:$dst),
  (CALL tconstpool:$dst)>;

include "AMDILConversions.td"

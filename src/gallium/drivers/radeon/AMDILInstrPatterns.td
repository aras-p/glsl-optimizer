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

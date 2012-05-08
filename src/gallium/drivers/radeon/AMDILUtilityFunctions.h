//===-- AMDILUtilityFunctions.h - AMDIL Utility Functions Header --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//
//
// This file provides helper macros for expanding case statements.
//
//===----------------------------------------------------------------------===//
#ifndef AMDILUTILITYFUNCTIONS_H_
#define AMDILUTILITYFUNCTIONS_H_

// Macros that are used to help with switch statements for various data types
// However, these macro's do not return anything unlike the second set below.
#define ExpandCaseTo32bitIntTypes(Instr)  \
case Instr##_i8: \
case Instr##_i16: \
case Instr##_i32:

#define ExpandCaseTo32bitIntTruncTypes(Instr)  \
case Instr##_i16i8: \
case Instr##_i32i8: \
case Instr##_i32i16: 

#define ExpandCaseToIntTypes(Instr) \
    ExpandCaseTo32bitIntTypes(Instr) \
case Instr##_i64:

#define ExpandCaseToIntTruncTypes(Instr) \
    ExpandCaseTo32bitIntTruncTypes(Instr) \
case Instr##_i64i8:\
case Instr##_i64i16:\
case Instr##_i64i32:\

#define ExpandCaseToFloatTypes(Instr) \
    case Instr##_f32: \
case Instr##_f64:

#define ExpandCaseToFloatTruncTypes(Instr) \
case Instr##_f64f32:

#define ExpandCaseTo32bitScalarTypes(Instr) \
    ExpandCaseTo32bitIntTypes(Instr) \
case Instr##_f32:

#define ExpandCaseToAllScalarTypes(Instr) \
    ExpandCaseToFloatTypes(Instr) \
ExpandCaseToIntTypes(Instr)

#define ExpandCaseToAllScalarTruncTypes(Instr) \
    ExpandCaseToFloatTruncTypes(Instr) \
ExpandCaseToIntTruncTypes(Instr)

// Vector versions of above macros
#define ExpandCaseToVectorIntTypes(Instr) \
    case Instr##_v2i8: \
case Instr##_v4i8: \
case Instr##_v2i16: \
case Instr##_v4i16: \
case Instr##_v2i32: \
case Instr##_v4i32: \
case Instr##_v2i64:

#define ExpandCaseToVectorIntTruncTypes(Instr) \
case Instr##_v2i16i8: \
case Instr##_v4i16i8: \
case Instr##_v2i32i8: \
case Instr##_v4i32i8: \
case Instr##_v2i32i16: \
case Instr##_v4i32i16: \
case Instr##_v2i64i8: \
case Instr##_v2i64i16: \
case Instr##_v2i64i32: 

#define ExpandCaseToVectorFloatTypes(Instr) \
    case Instr##_v2f32: \
case Instr##_v4f32: \
case Instr##_v2f64:

#define ExpandCaseToVectorFloatTruncTypes(Instr) \
case Instr##_v2f64f32:

#define ExpandCaseToVectorByteTypes(Instr) \
  case Instr##_v4i8:\
case Instr##_v2i16: \
case Instr##_v4i16:

#define ExpandCaseToAllVectorTypes(Instr) \
    ExpandCaseToVectorFloatTypes(Instr) \
ExpandCaseToVectorIntTypes(Instr)

#define ExpandCaseToAllVectorTruncTypes(Instr) \
    ExpandCaseToVectorFloatTruncTypes(Instr) \
ExpandCaseToVectorIntTruncTypes(Instr)

#define ExpandCaseToAllTypes(Instr) \
    ExpandCaseToAllVectorTypes(Instr) \
ExpandCaseToAllScalarTypes(Instr)

#define ExpandCaseToAllTruncTypes(Instr) \
    ExpandCaseToAllVectorTruncTypes(Instr) \
ExpandCaseToAllScalarTruncTypes(Instr)

#define ExpandCaseToPackedTypes(Instr) \
    case Instr##_v2i8: \
    case Instr##_v4i8: \
    case Instr##_v2i16: \
    case Instr##_v4i16:

#define ExpandCaseToByteShortTypes(Instr) \
    case Instr##_i8: \
    case Instr##_i16: \
    ExpandCaseToPackedTypes(Instr)

// Macros that expand into case statements with return values
#define ExpandCaseTo32bitIntReturn(Instr, Return)  \
case Instr##_i8: return Return##_i8;\
case Instr##_i16: return Return##_i16;\
case Instr##_i32: return Return##_i32;

#define ExpandCaseToIntReturn(Instr, Return) \
    ExpandCaseTo32bitIntReturn(Instr, Return) \
case Instr##_i64: return Return##_i64;

#define ExpandCaseToFloatReturn(Instr, Return) \
    case Instr##_f32: return Return##_f32;\
case Instr##_f64: return Return##_f64;

#define ExpandCaseToAllScalarReturn(Instr, Return) \
    ExpandCaseToFloatReturn(Instr, Return) \
ExpandCaseToIntReturn(Instr, Return)

// These macros expand to common groupings of RegClass ID's
#define ExpandCaseTo1CompRegID \
case AMDIL::GPRI8RegClassID: \
case AMDIL::GPRI16RegClassID: \
case AMDIL::GPRI32RegClassID: \
case AMDIL::GPRF32RegClassID:

#define ExpandCaseTo2CompRegID \
    case AMDIL::GPRI64RegClassID: \
case AMDIL::GPRF64RegClassID: \
case AMDIL::GPRV2I8RegClassID: \
case AMDIL::GPRV2I16RegClassID: \
case AMDIL::GPRV2I32RegClassID: \
case AMDIL::GPRV2F32RegClassID:

// Macros that expand to case statements for specific bitlengths
#define ExpandCaseTo8BitType(Instr) \
    case Instr##_i8:

#define ExpandCaseTo16BitType(Instr) \
    case Instr##_v2i8: \
case Instr##_i16:

#define ExpandCaseTo32BitType(Instr) \
    case Instr##_v4i8: \
case Instr##_v2i16: \
case Instr##_i32: \
case Instr##_f32:

#define ExpandCaseTo64BitType(Instr) \
    case Instr##_v4i16: \
case Instr##_v2i32: \
case Instr##_v2f32: \
case Instr##_i64: \
case Instr##_f64:

#define ExpandCaseTo128BitType(Instr) \
    case Instr##_v4i32: \
case Instr##_v4f32: \
case Instr##_v2i64: \
case Instr##_v2f64:

#endif // AMDILUTILITYFUNCTIONS_H_

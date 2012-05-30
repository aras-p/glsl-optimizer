//===- AMDILIntrinsicInfo.cpp - AMDIL Intrinsic Information ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//
//
// This file contains the AMDIL Implementation of the IntrinsicInfo class.
//
//===-----------------------------------------------------------------------===//

#include "AMDILIntrinsicInfo.h"
#include "AMDIL.h"
#include "AMDILSubtarget.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Intrinsics.h"
#include "llvm/Module.h"

using namespace llvm;

#define GET_LLVM_INTRINSIC_FOR_GCC_BUILTIN
#include "AMDGPUGenIntrinsics.inc"
#undef GET_LLVM_INTRINSIC_FOR_GCC_BUILTIN

AMDILIntrinsicInfo::AMDILIntrinsicInfo(TargetMachine *tm) 
  : TargetIntrinsicInfo(), mTM(tm)
{
}

std::string 
AMDILIntrinsicInfo::getName(unsigned int IntrID, Type **Tys,
    unsigned int numTys) const 
{
  static const char* const names[] = {
#define GET_INTRINSIC_NAME_TABLE
#include "AMDGPUGenIntrinsics.inc"
#undef GET_INTRINSIC_NAME_TABLE
  };

  //assert(!isOverloaded(IntrID)
  //&& "AMDIL Intrinsics are not overloaded");
  if (IntrID < Intrinsic::num_intrinsics) {
    return 0;
  }
  assert(IntrID < AMDGPUIntrinsic::num_AMDIL_intrinsics
      && "Invalid intrinsic ID");

  std::string Result(names[IntrID - Intrinsic::num_intrinsics]);
  return Result;
}

  static bool
checkTruncation(const char *Name, unsigned int& Len)
{
  const char *ptr = Name + (Len - 1);
  while(ptr != Name && *ptr != '_') {
    --ptr;
  }
  // We don't want to truncate on atomic instructions
  // but we do want to enter the check Truncation
  // section so that we can translate the atomic
  // instructions if we need to.
  if (!strncmp(Name, "__atom", 6)) {
    return true;
  }
  if (strstr(ptr, "i32")
      || strstr(ptr, "u32")
      || strstr(ptr, "i64")
      || strstr(ptr, "u64")
      || strstr(ptr, "f32")
      || strstr(ptr, "f64")
      || strstr(ptr, "i16")
      || strstr(ptr, "u16")
      || strstr(ptr, "i8")
      || strstr(ptr, "u8")) {
    Len = (unsigned int)(ptr - Name);
    return true;
  }
  return false;
}

// We don't want to support both the OpenCL 1.0 atomics
// and the 1.1 atomics with different names, so we translate
// the 1.0 atomics to the 1.1 naming here if needed.
static char*
atomTranslateIfNeeded(const char *Name, unsigned int Len) 
{
  char *buffer = NULL;
  if (strncmp(Name, "__atom_", 7))  {
    // If we are not starting with __atom_, then
    // go ahead and continue on with the allocation.
    buffer = new char[Len + 1];
    memcpy(buffer, Name, Len);
  } else {
    buffer = new char[Len + 3];
    memcpy(buffer, "__atomic_", 9);
    memcpy(buffer + 9, Name + 7, Len - 7);
    Len += 2;
  }
  buffer[Len] = '\0';
  return buffer;
}

unsigned int
AMDILIntrinsicInfo::lookupName(const char *Name, unsigned int Len) const 
{
#define GET_FUNCTION_RECOGNIZER
#include "AMDGPUGenIntrinsics.inc"
#undef GET_FUNCTION_RECOGNIZER
  AMDGPUIntrinsic::ID IntrinsicID
    = (AMDGPUIntrinsic::ID)Intrinsic::not_intrinsic;
  if (checkTruncation(Name, Len)) {
    char *buffer = atomTranslateIfNeeded(Name, Len);
    IntrinsicID = getIntrinsicForGCCBuiltin("AMDIL", buffer);
    delete [] buffer;
  } else {
    IntrinsicID = getIntrinsicForGCCBuiltin("AMDIL", Name);
  }
  if (!isValidIntrinsic(IntrinsicID)) {
    return 0;
  }
  if (IntrinsicID != (AMDGPUIntrinsic::ID)Intrinsic::not_intrinsic) {
    return IntrinsicID;
  }
  return 0;
}

bool 
AMDILIntrinsicInfo::isOverloaded(unsigned id) const 
{
  // Overload Table
#define GET_INTRINSIC_OVERLOAD_TABLE
#include "AMDGPUGenIntrinsics.inc"
#undef GET_INTRINSIC_OVERLOAD_TABLE
}

/// This defines the "getAttributes(ID id)" method.
#define GET_INTRINSIC_ATTRIBUTES
#include "AMDGPUGenIntrinsics.inc"
#undef GET_INTRINSIC_ATTRIBUTES

Function*
AMDILIntrinsicInfo::getDeclaration(Module *M, unsigned IntrID,
    Type **Tys,
    unsigned numTys) const 
{
  assert(!"Not implemented");
}

/// Because the code generator has to support different SC versions, 
/// this function is added to check that the intrinsic being used
/// is actually valid. In the case where it isn't valid, the 
/// function call is not translated into an intrinsic and the
/// fall back software emulated path should pick up the result.
bool
AMDILIntrinsicInfo::isValidIntrinsic(unsigned int IntrID) const
{
  const AMDILSubtarget &STM = mTM->getSubtarget<AMDILSubtarget>();
  switch (IntrID) {
    default:
      return true;
    case AMDGPUIntrinsic::AMDIL_convert_f32_i32_rpi:
    case AMDGPUIntrinsic::AMDIL_convert_f32_i32_flr:
    case AMDGPUIntrinsic::AMDIL_convert_f32_f16_near:
    case AMDGPUIntrinsic::AMDIL_convert_f32_f16_neg_inf:
    case AMDGPUIntrinsic::AMDIL_convert_f32_f16_plus_inf:
        return STM.calVersion() >= CAL_VERSION_SC_139;
  };
}

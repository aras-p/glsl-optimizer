//===-- AMDILMCTargetDesc.h - AMDIL Target Descriptions -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file provides AMDIL specific target descriptions.
//
//===----------------------------------------------------------------------===//
//

#ifndef AMDILMCTARGETDESC_H
#define AMDILMCTARGETDESC_H

namespace llvm {
class MCSubtargetInfo;
class Target;

extern Target TheAMDGPUTarget;

} // End llvm namespace

#define GET_REGINFO_ENUM
#include "AMDGPUGenRegisterInfo.inc"

#define GET_INSTRINFO_ENUM
#include "AMDGPUGenInstrInfo.inc"

#define GET_SUBTARGETINFO_ENUM
#include "AMDGPUGenSubtargetInfo.inc"

#endif // AMDILMCTARGETDESC_H

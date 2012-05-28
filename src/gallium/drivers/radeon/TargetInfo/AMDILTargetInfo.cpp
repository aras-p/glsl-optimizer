//===-- TargetInfo/AMDILTargetInfo.cpp - TODO: Add brief description -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// TODO: Add full description
//
//===----------------------------------------------------------------------===//

#include "AMDIL.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

/// The target for the AMDGPU backend
Target llvm::TheAMDGPUTarget;

/// Extern function to initialize the targets for the AMDIL backend
extern "C" void LLVMInitializeAMDGPUTargetInfo() {
  RegisterTarget<Triple::r600, false>
    R600(TheAMDGPUTarget, "r600", "AMD GPUs HD2XXX-HD6XXX");
}

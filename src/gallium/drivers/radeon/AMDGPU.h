//===-- AMDGPU.h - TODO: Add brief description -------===//
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

#ifndef AMDGPU_H
#define AMDGPU_H

#include "AMDGPUTargetMachine.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {
    class FunctionPass;
    class AMDGPUTargetMachine;

    FunctionPass *createR600CodeEmitterPass(formatted_raw_ostream &OS);
    FunctionPass *createR600LowerShaderInstructionsPass(TargetMachine &tm);
    FunctionPass *createR600LowerInstructionsPass(TargetMachine &tm);

    FunctionPass *createSIAssignInterpRegsPass(TargetMachine &tm);
    FunctionPass *createSIInitMachineFunctionInfoPass(TargetMachine &tm);
    FunctionPass *createSILowerShaderInstructionsPass(TargetMachine &tm);
    FunctionPass *createSIPropagateImmReadsPass(TargetMachine &tm);
    FunctionPass *createSICodeEmitterPass(formatted_raw_ostream &OS);

    FunctionPass *createAMDGPUReorderPreloadInstructionsPass(TargetMachine &tm);

    FunctionPass *createAMDGPULowerInstructionsPass(TargetMachine &tm);
    FunctionPass *createAMDGPULowerShaderInstructionsPass(TargetMachine &tm);

    FunctionPass *createAMDGPUDelimitInstGroupsPass(TargetMachine &tm);

    FunctionPass *createAMDGPUConvertToISAPass(TargetMachine &tm);

    FunctionPass *createAMDGPUFixRegClassesPass(TargetMachine &tm);

} /* End namespace llvm */
#endif /* AMDGPU_H */

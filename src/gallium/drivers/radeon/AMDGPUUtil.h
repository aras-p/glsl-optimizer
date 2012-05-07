//===-- AMDGPUUtil.h - TODO: Add brief description -------===//
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

#ifndef AMDGPU_UTIL_H
#define AMDGPU_UTIL_H

#include "AMDGPURegisterInfo.h"
#include "llvm/Support/DataTypes.h"

namespace llvm {

class AMDILMachineFunctionInfo;

class TargetMachine;
class TargetRegisterInfo;

bool isPlaceHolderOpcode(unsigned opcode);

bool isTransOp(unsigned opcode);
bool isTexOp(unsigned opcode);
bool isReductionOp(unsigned opcode);
bool isCubeOp(unsigned opcode);
bool isFCOp(unsigned opcode);

/* XXX: Move these to AMDGPUInstrInfo.h */
#define MO_FLAG_CLAMP (1 << 0)
#define MO_FLAG_NEG   (1 << 1)
#define MO_FLAG_ABS   (1 << 2)
#define MO_FLAG_MASK  (1 << 3)

} /* End namespace llvm */

namespace AMDGPU {

void utilAddLiveIn(llvm::MachineFunction * MF, llvm::MachineRegisterInfo & MRI,
    const struct llvm::TargetInstrInfo * TII, unsigned physReg, unsigned virtReg);

} // End namespace AMDGPU

#endif /* AMDGPU_UTIL_H */

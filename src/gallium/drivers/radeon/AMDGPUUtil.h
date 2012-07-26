//===-- AMDGPUUtil.h - AMDGPU Utility function declarations -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Declarations for utility functions common to all hw codegen targets.
//
//===----------------------------------------------------------------------===//

#ifndef AMDGPU_UTIL_H
#define AMDGPU_UTIL_H

namespace llvm {

class MachineFunction;
class MachineRegisterInfo;
class TargetInstrInfo;

namespace AMDGPU {

void utilAddLiveIn(MachineFunction * MF, MachineRegisterInfo & MRI,
    const TargetInstrInfo * TII, unsigned physReg, unsigned virtReg);

} // End namespace AMDGPU

} // End namespace llvm

#endif // AMDGPU_UTIL_H

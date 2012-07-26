//===-- AMDGPUUtil.cpp - AMDGPU Utility functions -------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Common utility functions used by hw codegen targets
//
//===----------------------------------------------------------------------===//

#include "AMDGPUUtil.h"
#include "AMDGPUInstrInfo.h"
#include "AMDGPURegisterInfo.h"
#include "AMDIL.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetRegisterInfo.h"

using namespace llvm;

void AMDGPU::utilAddLiveIn(MachineFunction * MF,
                           MachineRegisterInfo & MRI,
                           const TargetInstrInfo * TII,
                           unsigned physReg, unsigned virtReg)
{
    if (!MRI.isLiveIn(physReg)) {
      MRI.addLiveIn(physReg, virtReg);
      MF->front().addLiveIn(physReg);
      BuildMI(MF->front(), MF->front().begin(), DebugLoc(),
              TII->get(TargetOpcode::COPY), virtReg)
                .addReg(physReg);
    } else {
      MRI.replaceRegWith(virtReg, MRI.getLiveInVirtReg(physReg));
    }
}

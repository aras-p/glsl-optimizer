//===-- AMDGPULowerShaderInstructions.cpp - TODO: Add brief description -------===//
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


#include "AMDGPULowerShaderInstructions.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/Target/TargetInstrInfo.h"

using namespace llvm;

void AMDGPULowerShaderInstructionsPass::preloadRegister(MachineFunction * MF,
    const TargetInstrInfo * TII, unsigned physReg, unsigned virtReg) const
{
  if (!MRI->isLiveIn(physReg)) {
    MRI->addLiveIn(physReg, virtReg);
    MachineBasicBlock &EntryMBB = MF->front();
    BuildMI(MF->front(), EntryMBB.begin(), DebugLoc(), TII->get(TargetOpcode::COPY),
            virtReg)
            .addReg(physReg);
  } else {
    /* We can't mark the same register as preloaded twice, but we still must
     * associate virtReg with the correct preloaded register. */
    unsigned newReg = MRI->getLiveInVirtReg(physReg);
    MRI->replaceRegWith(virtReg, newReg);
  }
}

//===-- AMDGPUReorderPreloadInstructions.cpp - TODO: Add brief description -------===//
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

#include "AMDGPU.h"
#include "AMDIL.h"
#include "AMDILInstrInfo.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/Function.h"

using namespace llvm;

namespace {
  class AMDGPUReorderPreloadInstructionsPass : public MachineFunctionPass {

  private:
    static char ID;
    TargetMachine &TM;

  public:
    AMDGPUReorderPreloadInstructionsPass(TargetMachine &tm) :
      MachineFunctionPass(ID), TM(tm) { }

      bool runOnMachineFunction(MachineFunction &MF);

      const char *getPassName() const { return "AMDGPU Reorder Preload Instructions"; }
    };
} /* End anonymous namespace */

char AMDGPUReorderPreloadInstructionsPass::ID = 0;

FunctionPass *llvm::createAMDGPUReorderPreloadInstructionsPass(TargetMachine &tm) {
    return new AMDGPUReorderPreloadInstructionsPass(tm);
}

/* This pass moves instructions that represent preloaded registers to the
 * start of the program. */
bool AMDGPUReorderPreloadInstructionsPass::runOnMachineFunction(MachineFunction &MF)
{
  const AMDGPUInstrInfo * TII =
                        static_cast<const AMDGPUInstrInfo*>(TM.getInstrInfo());

  for (MachineFunction::iterator BB = MF.begin(), BB_E = MF.end();
                                                  BB != BB_E; ++BB) {
    MachineBasicBlock &MBB = *BB;
    for (MachineBasicBlock::iterator I = MBB.begin(), Next = llvm::next(I);
         I != MBB.end(); I = Next, Next = llvm::next(I) ) {
      MachineInstr &MI = *I;
      if (TII->isRegPreload(MI)) {
         MF.front().insert(MF.front().begin(), MI.removeFromParent());
      }
    }
  }
  return false;
}

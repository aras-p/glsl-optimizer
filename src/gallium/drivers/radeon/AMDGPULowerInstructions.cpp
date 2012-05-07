//===-- AMDGPULowerInstructions.cpp - TODO: Add brief description -------===//
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
#include "AMDGPURegisterInfo.h"
#include "AMDIL.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"

using namespace llvm;

namespace {
  class AMDGPULowerInstructionsPass : public MachineFunctionPass {

  private:
    static char ID;
    TargetMachine &TM;
    void lowerVCREATE_v4(MachineInstr &MI, MachineBasicBlock::iterator I,
                              MachineBasicBlock &MBB, MachineFunction &MF);

  public:
    AMDGPULowerInstructionsPass(TargetMachine &tm) :
      MachineFunctionPass(ID), TM(tm) { }

    virtual bool runOnMachineFunction(MachineFunction &MF);

  };
} /* End anonymous namespace */

char AMDGPULowerInstructionsPass::ID = 0;

FunctionPass *llvm::createAMDGPULowerInstructionsPass(TargetMachine &tm) {
  return new AMDGPULowerInstructionsPass(tm);
}

bool AMDGPULowerInstructionsPass::runOnMachineFunction(MachineFunction &MF)
{
  for (MachineFunction::iterator BB = MF.begin(), BB_E = MF.end();
                                                  BB != BB_E; ++BB) {
    MachineBasicBlock &MBB = *BB;
    for (MachineBasicBlock::iterator I = MBB.begin(), Next = llvm::next(I);
         I != MBB.end(); I = Next, Next = llvm::next(I) ) {
      MachineInstr &MI = *I;

      switch (MI.getOpcode()) {
      default: continue;
      case AMDIL::VCREATE_v4f32:
      case AMDIL::VCREATE_v4i32:
        lowerVCREATE_v4(MI, I, MBB, MF); break;
      }
      MI.eraseFromParent();
    }
  }
  return false;
}

void AMDGPULowerInstructionsPass::lowerVCREATE_v4(MachineInstr &MI,
    MachineBasicBlock::iterator I, MachineBasicBlock &MBB, MachineFunction &MF)
{
  MachineRegisterInfo & MRI = MF.getRegInfo();
  unsigned tmp = MRI.createVirtualRegister(
                  MRI.getRegClass(MI.getOperand(0).getReg()));

  BuildMI(MBB, I, DebugLoc(), TM.getInstrInfo()->get(AMDIL::IMPLICIT_DEF), tmp);

  BuildMI(MBB, I, DebugLoc(), TM.getInstrInfo()->get(AMDIL::INSERT_SUBREG))
          .addOperand(MI.getOperand(0))
          .addReg(tmp)
          .addOperand(MI.getOperand(1))
          .addImm(AMDIL::sel_x);
}

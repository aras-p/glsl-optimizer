//===-- SILowerShaderInstructions.cpp - Pass for lowering shader instructions  -------===//
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
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"

using namespace llvm;

namespace {
  class SILowerShaderInstructionsPass : public MachineFunctionPass {

  private:
    static char ID;
    TargetMachine &TM;
    MachineRegisterInfo * MRI;

  public:
    SILowerShaderInstructionsPass(TargetMachine &tm) :
      MachineFunctionPass(ID), TM(tm) { }

    bool runOnMachineFunction(MachineFunction &MF);

    const char *getPassName() const { return "SI Lower Shader Instructions"; }

    void lowerRETURN(MachineBasicBlock &MBB, MachineBasicBlock::iterator I);
    void lowerSET_M0(MachineInstr &MI, MachineBasicBlock &MBB,
                     MachineBasicBlock::iterator I);
  };
} /* End anonymous namespace */

char SILowerShaderInstructionsPass::ID = 0;

FunctionPass *llvm::createSILowerShaderInstructionsPass(TargetMachine &tm) {
    return new SILowerShaderInstructionsPass(tm);
}

bool SILowerShaderInstructionsPass::runOnMachineFunction(MachineFunction &MF)
{
  MRI = &MF.getRegInfo();
  for (MachineFunction::iterator BB = MF.begin(), BB_E = MF.end();
                                                  BB != BB_E; ++BB) {
    MachineBasicBlock &MBB = *BB;
    for (MachineBasicBlock::iterator I = MBB.begin(), Next = llvm::next(I);
         I != MBB.end(); I = Next, Next = llvm::next(I) ) {
      MachineInstr &MI = *I;
      switch (MI.getOpcode()) {
      case AMDIL::RETURN:
        lowerRETURN(MBB, I);
        break;
      case AMDIL::SET_M0:
        lowerSET_M0(MI, MBB, I);
        break;
      default: continue;
      }
      MI.removeFromParent();
    }
  }

  return false;
}

void SILowerShaderInstructionsPass::lowerRETURN(MachineBasicBlock &MBB,
    MachineBasicBlock::iterator I)
{
  const struct TargetInstrInfo * TII = TM.getInstrInfo();
  BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(AMDIL::S_ENDPGM));
}

void SILowerShaderInstructionsPass::lowerSET_M0(MachineInstr &MI,
    MachineBasicBlock &MBB, MachineBasicBlock::iterator I)
{
  const struct TargetInstrInfo * TII = TM.getInstrInfo();
  BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(AMDIL::S_MOV_IMM_I32))
          .addReg(AMDIL::M0)
          .addOperand(MI.getOperand(1));
}

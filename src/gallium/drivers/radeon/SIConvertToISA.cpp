//===-- SIConvertToISA.cpp - TODO: Add brief description -------===//
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
  class SIConvertToISAPass : public MachineFunctionPass {

  private:
    static char ID;
    TargetMachine &TM;
    void convertVCREATE_v4f32(MachineInstr &MI, MachineBasicBlock::iterator I,
                              MachineBasicBlock &MBB, MachineFunction &MF);

  public:
    SIConvertToISAPass(TargetMachine &tm) :
      MachineFunctionPass(ID), TM(tm) { }

    virtual bool runOnMachineFunction(MachineFunction &MF);

  };
} /* End anonymous namespace */

char SIConvertToISAPass::ID = 0;

FunctionPass *llvm::createSIConvertToISAPass(TargetMachine &tm) {
  return new SIConvertToISAPass(tm);
}

bool SIConvertToISAPass::runOnMachineFunction(MachineFunction &MF)
{
  for (MachineFunction::iterator BB = MF.begin(), BB_E = MF.end();
                                                  BB != BB_E; ++BB) {
    MachineBasicBlock &MBB = *BB;
    for (MachineBasicBlock::iterator I = MBB.begin(), Next = llvm::next(I);
         I != MBB.end(); I = Next, Next = llvm::next(I) ) {
      MachineInstr &MI = *I;

      switch (MI.getOpcode()) {
      default: continue;
      case AMDIL::VCREATE_v4f32: convertVCREATE_v4f32(MI, I, MBB, MF);

      }
      MI.removeFromParent();
    }
  }
  return false;
}

void SIConvertToISAPass::convertVCREATE_v4f32(MachineInstr &MI,
    MachineBasicBlock::iterator I, MachineBasicBlock &MBB, MachineFunction &MF)
{
  MachineInstrBuilder implicitDef;
  MachineInstrBuilder insertSubreg;
  MachineRegisterInfo & MRI = MF.getRegInfo();
  unsigned tmp = MRI.createVirtualRegister(&AMDIL::VReg_128RegClass);

  implicitDef = BuildMI(MF, MBB.findDebugLoc(I),
                        TM.getInstrInfo()->get(AMDIL::IMPLICIT_DEF), tmp);

  MRI.setRegClass(MI.getOperand(1).getReg(), &AMDIL::VReg_32RegClass);
  insertSubreg = BuildMI(MF, MBB.findDebugLoc(I),
                        TM.getInstrInfo()->get(AMDIL::INSERT_SUBREG))
                        .addOperand(MI.getOperand(0))
                        .addReg(tmp)
                        .addOperand(MI.getOperand(1))
                        .addImm(AMDIL::sel_x);

  MBB.insert(I, implicitDef);
  MBB.insert(I, insertSubreg);
}

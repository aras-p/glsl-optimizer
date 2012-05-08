//===-- R600LowerShaderInstructions.cpp - TODO: Add brief description -------===//
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
#include "AMDGPUUtil.h"
#include "AMDIL.h"
#include "AMDILInstrInfo.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"

using namespace llvm;

namespace {
  class R600LowerShaderInstructionsPass : public MachineFunctionPass {

  private:
    static char ID;
    TargetMachine &TM;
    MachineRegisterInfo * MRI;

    void lowerEXPORT_REG_FAKE(MachineInstr &MI, MachineBasicBlock &MBB,
        MachineBasicBlock::iterator I);
    bool lowerSTORE_OUTPUT(MachineInstr & MI, MachineBasicBlock &MBB,
        MachineBasicBlock::iterator I);

  public:
    R600LowerShaderInstructionsPass(TargetMachine &tm) :
      MachineFunctionPass(ID), TM(tm) { }

      bool runOnMachineFunction(MachineFunction &MF);

      const char *getPassName() const { return "R600 Lower Shader Instructions"; }
    };
} /* End anonymous namespace */

char R600LowerShaderInstructionsPass::ID = 0;

FunctionPass *llvm::createR600LowerShaderInstructionsPass(TargetMachine &tm) {
    return new R600LowerShaderInstructionsPass(tm);
}

#define INSTR_CASE_FLOAT_V(inst) \
  case AMDIL:: inst##_v4f32: \

#define INSTR_CASE_FLOAT_S(inst) \
  case AMDIL:: inst##_f32:

#define INSTR_CASE_FLOAT(inst) \
  INSTR_CASE_FLOAT_V(inst) \
  INSTR_CASE_FLOAT_S(inst)
bool R600LowerShaderInstructionsPass::runOnMachineFunction(MachineFunction &MF)
{
  MRI = &MF.getRegInfo();


  for (MachineFunction::iterator BB = MF.begin(), BB_E = MF.end();
                                                  BB != BB_E; ++BB) {
    MachineBasicBlock &MBB = *BB;
    for (MachineBasicBlock::iterator I = MBB.begin(); I != MBB.end();) {
      MachineInstr &MI = *I;
      bool deleteInstr = false;
      switch (MI.getOpcode()) {

      default: break;

      case AMDIL::RESERVE_REG:
      case AMDIL::EXPORT_REG:
        deleteInstr = true;
        break;

      case AMDIL::STORE_OUTPUT:
        deleteInstr = lowerSTORE_OUTPUT(MI, MBB, I);
        break;

      }

      ++I;

      if (deleteInstr) {
        MI.eraseFromParent();
      }
    }
  }

  return false;
}

bool R600LowerShaderInstructionsPass::lowerSTORE_OUTPUT(MachineInstr &MI,
    MachineBasicBlock &MBB, MachineBasicBlock::iterator I)
{
  MachineOperand &valueOp = MI.getOperand(1);
  MachineOperand &indexOp = MI.getOperand(2);
  unsigned valueReg = valueOp.getReg();
  int64_t outputIndex = indexOp.getImm();
  const TargetRegisterClass * outputClass = TM.getRegisterInfo()->getRegClass(AMDIL::R600_TReg32RegClassID);
  unsigned newRegister = outputClass->getRegister(outputIndex);

  BuildMI(MBB, I, MBB.findDebugLoc(I), TM.getInstrInfo()->get(AMDIL::COPY),
                  newRegister)
                  .addReg(valueReg);

  if (!MRI->isLiveOut(newRegister))
    MRI->addLiveOut(newRegister);

  return true;

}

//===-- R600LowerInstructions.cpp - Lower unsupported AMDIL instructions --===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This pass lowers AMDIL MachineInstrs that aren't supported by the R600
// target to either supported AMDIL MachineInstrs or R600 MachineInstrs.
//
//===----------------------------------------------------------------------===//

#include "AMDGPU.h"
#include "AMDGPUInstrInfo.h"
#include "AMDGPUUtil.h"
#include "AMDIL.h"
#include "AMDILRegisterInfo.h"
#include "R600InstrInfo.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/Constants.h"
#include "llvm/Target/TargetInstrInfo.h"

#include <stdio.h>

using namespace llvm;

namespace {
  class R600LowerInstructionsPass : public MachineFunctionPass {

  private:
    static char ID;
    const R600InstrInfo * TII;

  public:
    R600LowerInstructionsPass(TargetMachine &tm) :
      MachineFunctionPass(ID),
      TII(static_cast<const R600InstrInfo*>(tm.getInstrInfo()))
      { }

    const char *getPassName() const { return "R600 Lower Instructions"; }
    virtual bool runOnMachineFunction(MachineFunction &MF);

  };
} /* End anonymous namespace */

char R600LowerInstructionsPass::ID = 0;

FunctionPass *llvm::createR600LowerInstructionsPass(TargetMachine &tm) {
  return new R600LowerInstructionsPass(tm);
}

bool R600LowerInstructionsPass::runOnMachineFunction(MachineFunction &MF)
{
  for (MachineFunction::iterator BB = MF.begin(), BB_E = MF.end();
                                                  BB != BB_E; ++BB) {
    MachineBasicBlock &MBB = *BB;
    for (MachineBasicBlock::iterator I = MBB.begin(), Next = llvm::next(I);
         I != MBB.end(); I = Next, Next = llvm::next(I) ) {

      MachineInstr &MI = *I;
      switch(MI.getOpcode()) {
      case AMDIL::LOADCONST_f32:
      case AMDIL::LOADCONST_i32:
        {
          bool canInline = false;
          unsigned inlineReg;
          MachineOperand & dstOp = MI.getOperand(0);
          MachineOperand & immOp = MI.getOperand(1);
          if (immOp.isFPImm()) {
            const ConstantFP * cfp = immOp.getFPImm();
            if (cfp->isZero()) {
              canInline = true;
              inlineReg = AMDIL::ZERO;
            } else if (cfp->isExactlyValue(1.0f)) {
              canInline = true;
              inlineReg = AMDIL::ONE;
            } else if (cfp->isExactlyValue(0.5f)) {
              canInline = true;
              inlineReg = AMDIL::HALF;
            }
          }

          if (canInline) {
            BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(AMDIL::COPY))
                    .addOperand(dstOp)
                    .addReg(inlineReg);
          } else {
            BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(AMDIL::MOV))
                    .addOperand(dstOp)
                    .addReg(AMDIL::ALU_LITERAL_X)
                    .addOperand(immOp);
          }
          break;
        }
      default:
        continue;
      }
      MI.eraseFromParent();
    }
  }
  return false;
}

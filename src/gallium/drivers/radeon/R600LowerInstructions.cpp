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
    TargetMachine &TM;
    const R600InstrInfo * TII;
    MachineRegisterInfo * MRI;

    void lowerFLT(MachineInstr &MI);

    void calcAddress(const MachineOperand &ptrOp,
                     const MachineOperand &indexOp,
                     unsigned indexReg,
                     MachineBasicBlock &MBB,
                     MachineBasicBlock::iterator I) const;

  public:
    R600LowerInstructionsPass(TargetMachine &tm) :
      MachineFunctionPass(ID), TM(tm),
      TII(static_cast<const R600InstrInfo*>(tm.getInstrInfo())),
      MRI(NULL)
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
  MRI = &MF.getRegInfo();

  for (MachineFunction::iterator BB = MF.begin(), BB_E = MF.end();
                                                  BB != BB_E; ++BB) {
    MachineBasicBlock &MBB = *BB;
    for (MachineBasicBlock::iterator I = MBB.begin(), Next = llvm::next(I);
         I != MBB.end(); I = Next, Next = llvm::next(I) ) {

      MachineInstr &MI = *I;
      switch(MI.getOpcode()) {
      case AMDIL::FLT:
        BuildMI(MBB, I, MBB.findDebugLoc(I), TM.getInstrInfo()->get(AMDIL::FGE))
                .addOperand(MI.getOperand(0))
                .addOperand(MI.getOperand(2))
                .addOperand(MI.getOperand(1));
        break;

      /* XXX: Figure out the semantics of DIV_INF_f32 and make sure this is OK */
/*      case AMDIL::DIV_INF_f32:
        {
          unsigned tmp0 = MRI->createVirtualRegister(&AMDIL::GPRF32RegClass);
          BuildMI(MBB, I, MBB.findDebugLoc(I),
                          TM.getInstrInfo()->get(AMDIL::RECIP_CLAMPED), tmp0)
                  .addOperand(MI.getOperand(2));
          BuildMI(MBB, I, MBB.findDebugLoc(I),
                          TM.getInstrInfo()->get(AMDIL::MUL_IEEE_f32))
                  .addOperand(MI.getOperand(0))
                  .addReg(tmp0)
                  .addOperand(MI.getOperand(1));
          break;
        }
*/        /* XXX: This is an optimization */

      case AMDIL::GLOBALLOAD_f32:
      case AMDIL::GLOBALLOAD_i32:
        {
          MachineOperand &ptrOperand = MI.getOperand(1);
          MachineOperand &indexOperand = MI.getOperand(2);
          unsigned indexReg =
                   MRI->createVirtualRegister(&AMDIL::R600_TReg32_XRegClass);

          /* Calculate the address with in the VTX buffer */
          calcAddress(ptrOperand, indexOperand, indexReg, MBB, I);

          /* Make sure the VTX_READ_eg writes to the X chan */
          MRI->setRegClass(MI.getOperand(0).getReg(),
                          &AMDIL::R600_TReg32_XRegClass);

          /* Add the VTX_READ_eg instruction */
          BuildMI(MBB, I, MBB.findDebugLoc(I),
                          TII->get(AMDIL::VTX_READ_eg))
                  .addOperand(MI.getOperand(0))
                  .addReg(indexReg)
                  .addImm(1);
          break;
        }

      case AMDIL::GLOBALSTORE_i32:
      case AMDIL::GLOBALSTORE_f32:
        {
          MachineOperand &ptrOperand = MI.getOperand(1);
          MachineOperand &indexOperand = MI.getOperand(2);
          unsigned rwReg =
                   MRI->createVirtualRegister(&AMDIL::R600_TReg32_XRegClass);
          unsigned byteIndexReg =
                   MRI->createVirtualRegister(&AMDIL::R600_TReg32RegClass);
          unsigned shiftReg =
                   MRI->createVirtualRegister(&AMDIL::R600_TReg32RegClass);
          unsigned indexReg =
                   MRI->createVirtualRegister(&AMDIL::R600_TReg32_XRegClass);

          /* Move the store value to the correct register class */
          BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(AMDIL::COPY), rwReg)
                  .addOperand(MI.getOperand(0));

          /* Calculate the address in the RAT */
          calcAddress(ptrOperand, indexOperand, byteIndexReg, MBB, I);


          BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(AMDIL::MOV), shiftReg)
                  .addReg(AMDIL::ALU_LITERAL_X)
                  .addImm(2);

          /* XXX: Check GPU family */
          BuildMI(MBB, I, MBB.findDebugLoc(I),
                          TII->get(AMDIL::LSHR_eg), indexReg)
                 .addReg(byteIndexReg)
                 .addReg(shiftReg);

          /* XXX: Check GPU Family */
          BuildMI(MBB, I, MBB.findDebugLoc(I),
                          TII->get(AMDIL::RAT_WRITE_CACHELESS_eg))
                  .addReg(rwReg)
                  .addReg(indexReg)
                  .addImm(0);
          break;
        }

      case AMDIL::ILT:
        BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(AMDIL::SETGT_INT))
                .addOperand(MI.getOperand(0))
                .addOperand(MI.getOperand(2))
                .addOperand(MI.getOperand(1));
        break;
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

      case AMDIL::ULT:
        BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(AMDIL::SETGT_UINT))
                .addOperand(MI.getOperand(0))
                .addOperand(MI.getOperand(2))
                .addOperand(MI.getOperand(1));
        break;

      default:
        continue;
      }
      MI.eraseFromParent();
    }
  }
  return false;
}

void R600LowerInstructionsPass::calcAddress(const MachineOperand &ptrOp,
                                            const MachineOperand &indexOp,
                                            unsigned indexReg,
                                            MachineBasicBlock &MBB,
                                            MachineBasicBlock::iterator I) const
{
  /* Optimize the case where the indexOperand is 0 */
  if (indexOp.isImm() && indexOp.getImm() == 0) {
    assert(ptrOp.isReg());
    BuildMI(MBB, I, MBB.findDebugLoc(I),
                    TII->get(AMDIL::COPY), indexReg)
            .addOperand(ptrOp);
  } else {
    BuildMI(MBB, I, MBB.findDebugLoc(I),
                    TII->get(AMDIL::ADD_INT), indexReg)
            .addOperand(indexOp)
            .addOperand(ptrOp);
  }
}

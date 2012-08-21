//===-- R600ExpandSpecialInstrs.cpp - Expand special instructions ---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// Vector, Reduction, and Cube instructions need to fill the entire instruction
// group to work correctly.  This pass expands these individual instructions
// into several instructions that will completely fill the instruction group. 
//===----------------------------------------------------------------------===//

#include "AMDGPU.h"
#include "R600InstrInfo.h"
#include "R600RegisterInfo.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"

using namespace llvm;

namespace {

class R600ExpandSpecialInstrsPass : public MachineFunctionPass {

private:
  static char ID;
  const R600InstrInfo *TII;

public:
  R600ExpandSpecialInstrsPass(TargetMachine &tm) : MachineFunctionPass(ID),
    TII (static_cast<const R600InstrInfo *>(tm.getInstrInfo())) { }

  virtual bool runOnMachineFunction(MachineFunction &MF);

  const char *getPassName() const {
    return "R600 Expand special instructions pass";
  }
};

} // End anonymous namespace

char R600ExpandSpecialInstrsPass::ID = 0;

FunctionPass *llvm::createR600ExpandSpecialInstrsPass(TargetMachine &TM) {
  return new R600ExpandSpecialInstrsPass(TM);
}

bool R600ExpandSpecialInstrsPass::runOnMachineFunction(MachineFunction &MF) {

  const R600RegisterInfo &TRI = TII->getRegisterInfo();

  for (MachineFunction::iterator BB = MF.begin(), BB_E = MF.end();
                                                  BB != BB_E; ++BB) {
    MachineBasicBlock &MBB = *BB;
    MachineBasicBlock::iterator I = MBB.begin();
    while (I != MBB.end()) {
      MachineInstr &MI = *I;
      I = llvm::next(I);

      bool IsReduction = TII->isReductionOp(MI.getOpcode());
      bool IsVector = TII->isVector(MI);
      if (!IsReduction && !IsVector) {
        continue;
      }

      // Expand the instruction
      //
      // Reduction instructions:
      // T0_X = DP4 T1_XYZW, T2_XYZW
      // becomes:
      // TO_X = DP4 T1_X, T2_X
      // TO_Y (write masked) = DP4 T1_Y, T2_Y
      // TO_Z (write masked) = DP4 T1_Z, T2_Z
      // TO_W (write masked) = DP4 T1_W, T2_W
      //
      // Vector instructions:
      // T0_X = MULLO_INT T1_X, T2_X
      // becomes:
      // T0_X = MULLO_INT T1_X, T2_X
      // T0_Y (write masked) = MULLO_INT T1_X, T2_X
      // T0_Z (write masked) = MULLO_INT T1_X, T2_X
      // T0_W (write masked) = MULLO_INT T1_X, T2_X
      for (unsigned Chan = 0; Chan < 4; Chan++) {
        unsigned DstReg = MI.getOperand(0).getReg();
        unsigned Src0 = MI.getOperand(1).getReg();
        unsigned Src1 = MI.getOperand(2).getReg();
        if (IsReduction) {
          unsigned SubRegIndex = TRI.getSubRegFromChannel(Chan);
          Src0 = TRI.getSubReg(Src0, SubRegIndex);
          Src1 = TRI.getSubReg(Src1, SubRegIndex);
        }
        unsigned DstBase = TRI.getHWRegIndex(DstReg);
        unsigned NewDstReg = AMDGPU::R600_TReg32RegClass.getRegister((DstBase * 4) + Chan);
        unsigned Flags = (Chan != TRI.getHWRegChan(DstReg) ? MO_FLAG_MASK : 0);
        Flags |= (Chan == 3 ? MO_FLAG_LAST : 0);
        MachineOperand NewDstOp = MachineOperand::CreateReg(NewDstReg, true);
        NewDstOp.addTargetFlag(Flags);

        BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(MI.getOpcode()))
                .addOperand(NewDstOp)
                .addReg(Src0)
                .addReg(Src1)
                ->setIsInsideBundle(Chan != 0);
      }
      MI.eraseFromParent();
    }
  }
  return false;
}

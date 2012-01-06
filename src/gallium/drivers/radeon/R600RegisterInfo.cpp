//===-- R600RegisterInfo.cpp - TODO: Add brief description -------===//
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

#include "R600RegisterInfo.h"
#include "AMDGPUTargetMachine.h"

using namespace llvm;

R600RegisterInfo::R600RegisterInfo(AMDGPUTargetMachine &tm,
    const TargetInstrInfo &tii)
: AMDGPURegisterInfo(tm, tii),
  TM(tm),
  TII(tii)
  { }

BitVector R600RegisterInfo::getReservedRegs(const MachineFunction &MF) const
{
  BitVector Reserved(getNumRegs());
  Reserved.set(AMDIL::ZERO);
  Reserved.set(AMDIL::HALF);
  Reserved.set(AMDIL::ONE);
  Reserved.set(AMDIL::ONE_INT);
  Reserved.set(AMDIL::NEG_HALF);
  Reserved.set(AMDIL::NEG_ONE);
  Reserved.set(AMDIL::PV_X);
  Reserved.set(AMDIL::ALU_LITERAL_X);

  for (TargetRegisterClass::iterator I = AMDIL::R600_CReg32RegClass.begin(),
                        E = AMDIL::R600_CReg32RegClass.end(); I != E; ++I) {
    Reserved.set(*I);
  }

  for (MachineFunction::const_iterator BB = MF.begin(),
                                 BB_E = MF.end(); BB != BB_E; ++BB) {
    const MachineBasicBlock &MBB = *BB;
    for (MachineBasicBlock::const_iterator I = MBB.begin(), E = MBB.end();
                                                                  I != E; ++I) {
      const MachineInstr &MI = *I;
      if (MI.getOpcode() == AMDIL::RESERVE_REG) {
        if (!TargetRegisterInfo::isVirtualRegister(MI.getOperand(0).getReg())) {
          Reserved.set(MI.getOperand(0).getReg());
        }
      }
    }
  }
  return Reserved;
}

const TargetRegisterClass *
R600RegisterInfo::getISARegClass(const TargetRegisterClass * rc) const
{
  switch (rc->getID()) {
  case AMDIL::GPRV4F32RegClassID:
  case AMDIL::GPRV4I32RegClassID:
    return &AMDIL::R600_Reg128RegClass;
  case AMDIL::GPRF32RegClassID:
  case AMDIL::GPRI32RegClassID:
    return &AMDIL::R600_Reg32RegClass;
  default: return rc;
  }
}

unsigned R600RegisterInfo::getHWRegIndex(unsigned reg) const
{
  switch(reg) {
  case AMDIL::ZERO: return 248;
  case AMDIL::ONE:
  case AMDIL::NEG_ONE: return 249;
  case AMDIL::ONE_INT: return 250;
  case AMDIL::HALF:
  case AMDIL::NEG_HALF: return 252;
  case AMDIL::ALU_LITERAL_X: return 253;
  default: return getHWRegIndexGen(reg);
  }
}

unsigned R600RegisterInfo::getHWRegChan(unsigned reg) const
{
  switch(reg) {
  case AMDIL::ZERO:
  case AMDIL::ONE:
  case AMDIL::ONE_INT:
  case AMDIL::NEG_ONE:
  case AMDIL::HALF:
  case AMDIL::NEG_HALF:
  case AMDIL::ALU_LITERAL_X:
    return 0;
  default: return getHWRegChanGen(reg);
  }
}

#include "R600HwRegInfo.include"

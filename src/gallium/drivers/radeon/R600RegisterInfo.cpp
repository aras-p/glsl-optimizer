//===-- R600RegisterInfo.cpp - R600 Register Information ------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// The file contains the R600 implementation of the TargetRegisterInfo class.
//
//===----------------------------------------------------------------------===//

#include "R600RegisterInfo.h"
#include "AMDGPUTargetMachine.h"
#include "R600MachineFunctionInfo.h"

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
  const R600MachineFunctionInfo * MFI = MF.getInfo<R600MachineFunctionInfo>();

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

  for (std::vector<unsigned>::const_iterator I = MFI->ReservedRegs.begin(),
                                    E = MFI->ReservedRegs.end(); I != E; ++I) {
    Reserved.set(*I);
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

const TargetRegisterClass * R600RegisterInfo::getCFGStructurizerRegClass(
                                                                   MVT VT) const
{
  switch(VT.SimpleTy) {
  default:
  case MVT::i32: return AMDIL::R600_TReg32RegisterClass;
  }
}
#include "R600HwRegInfo.include"

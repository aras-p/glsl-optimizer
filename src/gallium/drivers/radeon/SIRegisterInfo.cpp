//===-- SIRegisterInfo.cpp - SI Register Information ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the SI implementation of the TargetRegisterInfo class.
//
//===----------------------------------------------------------------------===//


#include "SIRegisterInfo.h"
#include "AMDGPUTargetMachine.h"
#include "AMDGPUUtil.h"

using namespace llvm;

SIRegisterInfo::SIRegisterInfo(AMDGPUTargetMachine &tm,
    const TargetInstrInfo &tii)
: AMDGPURegisterInfo(tm, tii),
  TM(tm),
  TII(tii)
  { }

BitVector SIRegisterInfo::getReservedRegs(const MachineFunction &MF) const
{
  BitVector Reserved(getNumRegs());
  return Reserved;
}

unsigned SIRegisterInfo::getBinaryCode(unsigned reg) const
{
  switch (reg) {
    case AMDIL::M0: return 124;
    case AMDIL::SREG_LIT_0: return 128;
    default: return getHWRegNum(reg);
  }
}

const TargetRegisterClass *
SIRegisterInfo::getISARegClass(const TargetRegisterClass * rc) const
{
  switch (rc->getID()) {
  case AMDIL::GPRF32RegClassID:
    return &AMDIL::VReg_32RegClass;
  default: return rc;
  }
}

const TargetRegisterClass * SIRegisterInfo::getCFGStructurizerRegClass(
                                                                   MVT VT) const
{
  switch(VT.SimpleTy) {
    default:
    case MVT::i32: return AMDIL::VReg_32RegisterClass;
  }
}
#include "SIRegisterGetHWRegNum.inc"

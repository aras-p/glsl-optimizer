//===-- SIRegisterInfo.cpp - TODO: Add brief description -------===//
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

bool SIRegisterInfo::isBaseRegClass(unsigned regClassID) const
{
  switch (regClassID) {
  default: return true;
  case AMDIL::AllReg_32RegClassID:
  case AMDIL::AllReg_64RegClassID:
    return false;
  }
}

const TargetRegisterClass *
SIRegisterInfo::getISARegClass(const TargetRegisterClass * rc) const
{
  switch (rc->getID()) {
  case AMDIL::GPRF32RegClassID:
    return &AMDIL::VReg_32RegClass;
  case AMDIL::GPRV4F32RegClassID:
  case AMDIL::GPRV4I32RegClassID:
    return &AMDIL::VReg_128RegClass;
  default: return rc;
  }
}

#include "SIRegisterGetHWRegNum.inc"

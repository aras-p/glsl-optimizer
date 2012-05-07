//===-- R600InstrInfo.cpp - TODO: Add brief description -------===//
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

#include "R600InstrInfo.h"
#include "AMDGPUTargetMachine.h"
#include "R600RegisterInfo.h"

using namespace llvm;

R600InstrInfo::R600InstrInfo(AMDGPUTargetMachine &tm)
  : AMDGPUInstrInfo(tm),
    RI(tm, *this),
    TM(tm)
  { }

const R600RegisterInfo &R600InstrInfo::getRegisterInfo() const
{
  return RI;
}

bool R600InstrInfo::isTrig(const MachineInstr &MI) const
{
  return get(MI.getOpcode()).TSFlags & R600_InstFlag::TRIG;
}

void
R600InstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                           MachineBasicBlock::iterator MI, DebugLoc DL,
                           unsigned DestReg, unsigned SrcReg,
                           bool KillSrc) const
{

  unsigned subRegMap[4] = {AMDIL::sel_x, AMDIL::sel_y, AMDIL::sel_z, AMDIL::sel_w};

  if (AMDIL::R600_Reg128RegClass.contains(DestReg)
      && AMDIL::R600_Reg128RegClass.contains(SrcReg)) {
    for (unsigned i = 0; i < 4; i++) {
      BuildMI(MBB, MI, DL, get(AMDIL::MOV))
              .addReg(RI.getSubReg(DestReg, subRegMap[i]), RegState::Define)
              .addReg(RI.getSubReg(SrcReg, subRegMap[i]))
              .addReg(DestReg, RegState::Define | RegState::Implicit);
    }
  } else {

    /* We can't copy vec4 registers */
    assert(!AMDIL::R600_Reg128RegClass.contains(DestReg)
           && !AMDIL::R600_Reg128RegClass.contains(SrcReg));

    BuildMI(MBB, MI, DL, get(AMDIL::MOV), DestReg)
      .addReg(SrcReg, getKillRegState(KillSrc));
  }
}

unsigned R600InstrInfo::getISAOpcode(unsigned opcode) const
{
  switch (opcode) {
    default: return AMDGPUInstrInfo::getISAOpcode(opcode);
    case AMDIL::CUSTOM_ADD_i32:
      return AMDIL::ADD_INT;
    case AMDIL::CUSTOM_XOR_i32:
      return AMDIL::XOR_INT;
    case AMDIL::MOVE_f32:
    case AMDIL::MOVE_i32:
      return AMDIL::MOV;
    case AMDIL::SHR_i32:
      return getASHRop();
    case AMDIL::USHR_i32:
      return getLSHRop();
  }
}

unsigned R600InstrInfo::getASHRop() const
{
	unsigned gen = TM.getSubtarget<AMDILSubtarget>().device()->getGeneration();
	if (gen < AMDILDeviceInfo::HD5XXX) {
		return AMDIL::ASHR_r600;
	} else {
		return AMDIL::ASHR_eg;
	}
}

unsigned R600InstrInfo::getLSHRop() const
{
  unsigned gen = TM.getSubtarget<AMDILSubtarget>().device()->getGeneration();
  if (gen < AMDILDeviceInfo::HD5XXX) {
    return AMDIL::LSHR_r600;
  } else {
    return AMDIL::LSHR_eg;
  }
}

unsigned R600InstrInfo::getMULHI_UINT() const
{
  unsigned gen = TM.getSubtarget<AMDILSubtarget>().device()->getGeneration();

  if (gen < AMDILDeviceInfo::HD5XXX) {
    return AMDIL::MULHI_UINT_r600;
  } else {
    return AMDIL::MULHI_UINT_eg;
  }
}

unsigned R600InstrInfo::getMULLO_UINT() const
{
  unsigned gen = TM.getSubtarget<AMDILSubtarget>().device()->getGeneration();

  if (gen < AMDILDeviceInfo::HD5XXX) {
    return AMDIL::MULLO_UINT_r600;
  } else {
    return AMDIL::MULLO_UINT_eg;
  }
}

unsigned R600InstrInfo::getRECIP_UINT() const
{
  const AMDILDevice * dev = TM.getSubtarget<AMDILSubtarget>().device();

  if (dev->getGeneration() < AMDILDeviceInfo::HD5XXX) {
    return AMDIL::RECIP_UINT_r600;
  } else if (dev->getDeviceFlag() != OCL_DEVICE_CAYMAN) {
    return AMDIL::RECIP_UINT_eg;
  } else {
    return AMDIL::RECIP_UINT_cm;
  }
}

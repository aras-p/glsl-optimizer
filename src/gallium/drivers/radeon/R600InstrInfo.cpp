//===-- R600InstrInfo.cpp - R600 Instruction Information ------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// R600 Implementation of TargetInstrInfo.
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
    default: return opcode;
    case AMDIL::IEQ:
      return AMDIL::SETE_INT;
    case AMDIL::INE:
      return AMDIL::SETNE_INT;
    case AMDIL::IGE:
      return AMDIL::SETGE_INT;
    case AMDIL::MOVE_f32:
    case AMDIL::MOVE_i32:
      return AMDIL::MOV;
    case AMDIL::UGE:
      return AMDIL::SETGE_UINT;
    case AMDIL::UGT:
      return AMDIL::SETGT_UINT;
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

MachineInstr * R600InstrInfo::getMovImmInstr(MachineFunction *MF,
                                             unsigned DstReg, int64_t Imm) const
{
  MachineInstr * MI = MF->CreateMachineInstr(get(AMDIL::MOV), DebugLoc());
  MachineInstrBuilder(MI).addReg(DstReg, RegState::Define);
  MachineInstrBuilder(MI).addReg(AMDIL::ALU_LITERAL_X);
  MachineInstrBuilder(MI).addImm(Imm);

  return MI;
}

unsigned R600InstrInfo::getIEQOpcode() const
{
  return AMDIL::SETE_INT;
}

bool R600InstrInfo::isMov(unsigned Opcode) const
{
  switch(Opcode) {
  default: return false;
  case AMDIL::MOV:
  case AMDIL::MOV_IMM_F32:
  case AMDIL::MOV_IMM_I32:
    return true;
  }
}

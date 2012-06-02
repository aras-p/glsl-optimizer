//===-- SIInstrInfo.cpp - SI Instruction Information  ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// SI Implementation of TargetInstrInfo.
//
//===----------------------------------------------------------------------===//


#include "SIInstrInfo.h"
#include "AMDGPUTargetMachine.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/MC/MCInstrDesc.h"

#include <stdio.h>

using namespace llvm;

SIInstrInfo::SIInstrInfo(AMDGPUTargetMachine &tm)
  : AMDGPUInstrInfo(tm),
    RI(tm, *this),
    TM(tm)
    { }

const SIRegisterInfo &SIInstrInfo::getRegisterInfo() const
{
  return RI;
}

void
SIInstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                           MachineBasicBlock::iterator MI, DebugLoc DL,
                           unsigned DestReg, unsigned SrcReg,
                           bool KillSrc) const
{
  BuildMI(MBB, MI, DL, get(AMDIL::V_MOV_B32_e32), DestReg)
   .addReg(SrcReg, getKillRegState(KillSrc));
}

unsigned SIInstrInfo::getEncodingType(const MachineInstr &MI) const
{
  return get(MI.getOpcode()).TSFlags & SI_INSTR_FLAGS_ENCODING_MASK;
}

unsigned SIInstrInfo::getEncodingBytes(const MachineInstr &MI) const
{

  /* Instructions with literal constants are expanded to 64-bits, and
   * the constant is stored in bits [63:32] */
  for (unsigned i = 0; i < MI.getNumOperands(); i++) {
    if (MI.getOperand(i).getType() == MachineOperand::MO_FPImmediate) {
      return 8;
    }
  }

  /* This instruction always has a literal */
  if (MI.getOpcode() == AMDIL::S_MOV_IMM_I32) {
    return 8;
  }

  unsigned encoding_type = getEncodingType(MI);
  switch (encoding_type) {
    case SIInstrEncodingType::EXP:
    case SIInstrEncodingType::LDS:
    case SIInstrEncodingType::MUBUF:
    case SIInstrEncodingType::MTBUF:
    case SIInstrEncodingType::MIMG:
    case SIInstrEncodingType::VOP3:
      return 8;
    default:
      return 4;
  }
}

MachineInstr * SIInstrInfo::convertToISA(MachineInstr & MI, MachineFunction &MF,
    DebugLoc DL) const
{
  MachineInstr * newMI = AMDGPUInstrInfo::convertToISA(MI, MF, DL);
  const MCInstrDesc &newDesc = get(newMI->getOpcode());

  /* If this instruction was converted to a VOP3, we need to add the extra
   * operands for abs, clamp, omod, and negate. */
  if (getEncodingType(*newMI) == SIInstrEncodingType::VOP3
      && newMI->getNumOperands() < newDesc.getNumOperands()) {
    MachineInstrBuilder builder(newMI);
    for (unsigned op_idx = newMI->getNumOperands();
                  op_idx < newDesc.getNumOperands(); op_idx++) {
      builder.addImm(0);
    }
  }
  return newMI;
}

unsigned SIInstrInfo::getISAOpcode(unsigned AMDILopcode) const
{
  switch (AMDILopcode) {
  //XXX We need a better way of detecting end of program
  case AMDIL::MOVE_f32: return AMDIL::V_MOV_B32_e32;
  default: return AMDILopcode;
  }
}

MachineInstr * SIInstrInfo::getMovImmInstr(MachineFunction *MF, unsigned DstReg,
                                           int64_t Imm) const
{
  MachineInstr * MI = MF->CreateMachineInstr(get(AMDIL::V_MOV_IMM_I32), DebugLoc());
  MachineInstrBuilder(MI).addReg(DstReg, RegState::Define);
  MachineInstrBuilder(MI).addImm(Imm);

  return MI;

}

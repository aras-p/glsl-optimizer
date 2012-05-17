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

  switch (MI.getOpcode()) {
    default: break;
    case AMDIL::ABS_f32: return convertABS_f32(MI, MF, DL);
    case AMDIL::CLAMP_f32: return convertCLAMP_f32(MI, MF, DL);
  }

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
  case AMDIL::RETURN: return AMDIL::S_ENDPGM;
  default: return AMDGPUInstrInfo::getISAOpcode(AMDILopcode);
  }
}

MachineInstr * SIInstrInfo::convertABS_f32(MachineInstr & absInstr,
    MachineFunction &MF, DebugLoc DL) const
{
  MachineRegisterInfo &MRI = MF.getRegInfo();
  MachineOperand &dst = absInstr.getOperand(0);

  /* Convert the desination register to the VReg_32 class */
  if (TargetRegisterInfo::isVirtualRegister(dst.getReg())) {
    MRI.setRegClass(dst.getReg(), AMDIL::VReg_32RegisterClass);
  }

  return BuildMI(MF, DL, get(AMDIL::V_MOV_B32_e64))
                 .addOperand(absInstr.getOperand(0))
                 .addOperand(absInstr.getOperand(1))
                /* VSRC1-2 are unused, but we still need to fill all the
                 * operand slots, so we just reuse the VSRC0 operand */
                 .addOperand(absInstr.getOperand(1))
                 .addOperand(absInstr.getOperand(1))
                 .addImm(1) // ABS
                 .addImm(0) // CLAMP
                 .addImm(0) // OMOD
                 .addImm(0); // NEG
}

MachineInstr * SIInstrInfo::convertCLAMP_f32(MachineInstr & clampInstr,
    MachineFunction &MF, DebugLoc DL) const
{
  MachineRegisterInfo &MRI = MF.getRegInfo();
  /* XXX: HACK assume that low == zero and high == one for now until
   * we have a way to propogate the immediates. */

/*
  uint32_t zero = (uint32_t)APFloat(0.0f).bitcastToAPInt().getZExtValue();
  uint32_t one = (uint32_t)APFloat(1.0f).bitcastToAPInt().getZExtValue();
  uint32_t low = clampInstr.getOperand(2).getImm();
  uint32_t high = clampInstr.getOperand(3).getImm();
*/
//  if (low == zero && high == one) {
  
  /* Convert the desination register to the VReg_32 class */
  if (TargetRegisterInfo::isVirtualRegister(clampInstr.getOperand(0).getReg())) {
    MRI.setRegClass(clampInstr.getOperand(0).getReg(),
                    AMDIL::VReg_32RegisterClass);
  }
  return BuildMI(MF, DL, get(AMDIL::V_MOV_B32_e64))
           .addOperand(clampInstr.getOperand(0))
           .addOperand(clampInstr.getOperand(1))
          /* VSRC1-2 are unused, but we still need to fill all the
           * operand slots, so we just reuse the VSRC0 operand */
           .addOperand(clampInstr.getOperand(1))
           .addOperand(clampInstr.getOperand(1))
           .addImm(0) // ABS
           .addImm(1) // CLAMP
           .addImm(0) // OMOD
           .addImm(0); // NEG
//  } else {
    /* XXX: Handle other cases */
//    abort();
//  }
}

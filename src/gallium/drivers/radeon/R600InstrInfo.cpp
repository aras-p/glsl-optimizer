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
#include "AMDGPUSubtarget.h"
#include "R600RegisterInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "AMDILUtilityFunctions.h"

#define GET_INSTRINFO_CTOR
#include "AMDGPUGenDFAPacketizer.inc"

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

bool R600InstrInfo::isVector(const MachineInstr &MI) const
{
  return get(MI.getOpcode()).TSFlags & R600_InstFlag::VECTOR;
}

void
R600InstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                           MachineBasicBlock::iterator MI, DebugLoc DL,
                           unsigned DestReg, unsigned SrcReg,
                           bool KillSrc) const
{

  unsigned subRegMap[4] = {AMDGPU::sel_x, AMDGPU::sel_y,
                           AMDGPU::sel_z, AMDGPU::sel_w};

  if (AMDGPU::R600_Reg128RegClass.contains(DestReg)
      && AMDGPU::R600_Reg128RegClass.contains(SrcReg)) {
    for (unsigned i = 0; i < 4; i++) {
      BuildMI(MBB, MI, DL, get(AMDGPU::MOV))
              .addReg(RI.getSubReg(DestReg, subRegMap[i]), RegState::Define)
              .addReg(RI.getSubReg(SrcReg, subRegMap[i]))
              .addReg(0) // PREDICATE_BIT
              .addReg(DestReg, RegState::Define | RegState::Implicit);
    }
  } else {

    /* We can't copy vec4 registers */
    assert(!AMDGPU::R600_Reg128RegClass.contains(DestReg)
           && !AMDGPU::R600_Reg128RegClass.contains(SrcReg));

    BuildMI(MBB, MI, DL, get(AMDGPU::MOV), DestReg)
      .addReg(SrcReg, getKillRegState(KillSrc))
      .addReg(0); // PREDICATE_BIT
  }
}

MachineInstr * R600InstrInfo::getMovImmInstr(MachineFunction *MF,
                                             unsigned DstReg, int64_t Imm) const
{
  MachineInstr * MI = MF->CreateMachineInstr(get(AMDGPU::MOV), DebugLoc());
  MachineInstrBuilder(MI).addReg(DstReg, RegState::Define);
  MachineInstrBuilder(MI).addReg(AMDGPU::ALU_LITERAL_X);
  MachineInstrBuilder(MI).addImm(Imm);
  MachineInstrBuilder(MI).addReg(0); // PREDICATE_BIT

  return MI;
}

unsigned R600InstrInfo::getIEQOpcode() const
{
  return AMDGPU::SETE_INT;
}

bool R600InstrInfo::isMov(unsigned Opcode) const
{
  switch(Opcode) {
  default: return false;
  case AMDGPU::MOV:
  case AMDGPU::MOV_IMM_F32:
  case AMDGPU::MOV_IMM_I32:
    return true;
  }
}

// Some instructions act as place holders to emulate operations that the GPU
// hardware does automatically. This function can be used to check if
// an opcode falls into this category.
bool R600InstrInfo::isPlaceHolderOpcode(unsigned opcode) const
{
  switch (opcode) {
  default: return false;
  case AMDGPU::RETURN:
  case AMDGPU::LAST:
  case AMDGPU::MASK_WRITE:
  case AMDGPU::RESERVE_REG:
    return true;
  }
}

bool R600InstrInfo::isTexOp(unsigned opcode) const
{
  switch(opcode) {
  default: return false;
  case AMDGPU::TEX_LD:
  case AMDGPU::TEX_GET_TEXTURE_RESINFO:
  case AMDGPU::TEX_SAMPLE:
  case AMDGPU::TEX_SAMPLE_C:
  case AMDGPU::TEX_SAMPLE_L:
  case AMDGPU::TEX_SAMPLE_C_L:
  case AMDGPU::TEX_SAMPLE_LB:
  case AMDGPU::TEX_SAMPLE_C_LB:
  case AMDGPU::TEX_SAMPLE_G:
  case AMDGPU::TEX_SAMPLE_C_G:
  case AMDGPU::TEX_GET_GRADIENTS_H:
  case AMDGPU::TEX_GET_GRADIENTS_V:
  case AMDGPU::TEX_SET_GRADIENTS_H:
  case AMDGPU::TEX_SET_GRADIENTS_V:
    return true;
  }
}

bool R600InstrInfo::isReductionOp(unsigned opcode) const
{
  switch(opcode) {
    default: return false;
    case AMDGPU::DOT4_r600:
    case AMDGPU::DOT4_eg:
      return true;
  }
}

bool R600InstrInfo::isCubeOp(unsigned opcode) const
{
  switch(opcode) {
    default: return false;
    case AMDGPU::CUBE_r600:
    case AMDGPU::CUBE_eg:
      return true;
  }
}


bool R600InstrInfo::isFCOp(unsigned opcode) const
{
  switch(opcode) {
  default: return false;
  case AMDGPU::BREAK_LOGICALZ_f32:
  case AMDGPU::BREAK_LOGICALNZ_i32:
  case AMDGPU::BREAK_LOGICALZ_i32:
  case AMDGPU::BREAK_LOGICALNZ_f32:
  case AMDGPU::CONTINUE_LOGICALNZ_f32:
  case AMDGPU::IF_LOGICALNZ_i32:
  case AMDGPU::IF_LOGICALZ_f32:
  case AMDGPU::ELSE:
  case AMDGPU::ENDIF:
  case AMDGPU::ENDLOOP:
  case AMDGPU::IF_LOGICALNZ_f32:
  case AMDGPU::WHILELOOP:
    return true;
  }
}

DFAPacketizer *R600InstrInfo::CreateTargetScheduleState(const TargetMachine *TM,
    const ScheduleDAG *DAG) const
{
  const InstrItineraryData *II = TM->getInstrItineraryData();
  return TM->getSubtarget<AMDGPUSubtarget>().createDFAPacketizer(II);
}

bool
R600InstrInfo::isPredicated(const MachineInstr *MI) const
{
  int idx = MI->findFirstPredOperandIdx();
  if (idx < 0)
    return false;

  MI->dump();
  unsigned Reg = MI->getOperand(idx).getReg();
  switch (Reg) {
  default: return false;
  case AMDGPU::PRED_SEL_ONE:
  case AMDGPU::PRED_SEL_ZERO:
  case AMDGPU::PREDICATE_BIT:
    return true;
  }
}

bool
R600InstrInfo::isPredicable(MachineInstr *MI) const
{
  return AMDGPUInstrInfo::isPredicable(MI);
}

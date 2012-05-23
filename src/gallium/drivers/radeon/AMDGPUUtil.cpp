//===-- AMDGPUUtil.cpp - AMDGPU Utility functions -------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Common utility functions used by hw codegen targets
//
//===----------------------------------------------------------------------===//

#include "AMDGPUUtil.h"
#include "AMDGPURegisterInfo.h"
#include "AMDIL.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetRegisterInfo.h"

using namespace llvm;

// Some instructions act as place holders to emulate operations that the GPU
// hardware does automatically. This function can be used to check if
// an opcode falls into this category.
bool AMDGPU::isPlaceHolderOpcode(unsigned opcode)
{
  switch (opcode) {
  default: return false;
  case AMDIL::RETURN:
  case AMDIL::LOAD_INPUT:
  case AMDIL::LAST:
  case AMDIL::MASK_WRITE:
  case AMDIL::RESERVE_REG:
    return true;
  }
}

bool AMDGPU::isTransOp(unsigned opcode)
{
  switch(opcode) {
    default: return false;

    case AMDIL::COS_f32:
    case AMDIL::COS_r600:
    case AMDIL::COS_eg:
    case AMDIL::RSQ_f32:
    case AMDIL::FTOI:
    case AMDIL::ITOF:
    case AMDIL::MULLIT:
    case AMDIL::MUL_LIT_r600:
    case AMDIL::MUL_LIT_eg:
    case AMDIL::SHR_i32:
    case AMDIL::SIN_f32:
    case AMDIL::EXP_IEEE_r600:
    case AMDIL::EXP_IEEE_eg:
    case AMDIL::LOG_CLAMPED_r600:
    case AMDIL::LOG_IEEE_r600:
    case AMDIL::LOG_CLAMPED_eg:
    case AMDIL::LOG_IEEE_eg:
    case AMDIL::LOG_f32:
      return true;
  }
}

bool AMDGPU::isTexOp(unsigned opcode)
{
  switch(opcode) {
  default: return false;
  case AMDIL::TEX_LD:
  case AMDIL::TEX_GET_TEXTURE_RESINFO:
  case AMDIL::TEX_SAMPLE:
  case AMDIL::TEX_SAMPLE_C:
  case AMDIL::TEX_SAMPLE_L:
  case AMDIL::TEX_SAMPLE_C_L:
  case AMDIL::TEX_SAMPLE_LB:
  case AMDIL::TEX_SAMPLE_C_LB:
  case AMDIL::TEX_SAMPLE_G:
  case AMDIL::TEX_SAMPLE_C_G:
  case AMDIL::TEX_GET_GRADIENTS_H:
  case AMDIL::TEX_GET_GRADIENTS_V:
  case AMDIL::TEX_SET_GRADIENTS_H:
  case AMDIL::TEX_SET_GRADIENTS_V:
    return true;
  }
}

bool AMDGPU::isReductionOp(unsigned opcode)
{
  switch(opcode) {
    default: return false;
    case AMDIL::DOT4_r600:
    case AMDIL::DOT4_eg:
      return true;
  }
}

bool AMDGPU::isCubeOp(unsigned opcode)
{
  switch(opcode) {
    default: return false;
    case AMDIL::CUBE_r600:
    case AMDIL::CUBE_eg:
      return true;
  }
}


bool AMDGPU::isFCOp(unsigned opcode)
{
  switch(opcode) {
  default: return false;
  case AMDIL::BREAK_LOGICALZ_f32:
  case AMDIL::BREAK_LOGICALNZ_i32:
  case AMDIL::BREAK_LOGICALZ_i32:
  case AMDIL::BREAK_LOGICALNZ_f32:
  case AMDIL::CONTINUE_LOGICALNZ_f32:
  case AMDIL::IF_LOGICALNZ_i32:
  case AMDIL::IF_LOGICALZ_f32:
  case AMDIL::ELSE:
  case AMDIL::ENDIF:
  case AMDIL::ENDLOOP:
  case AMDIL::IF_LOGICALNZ_f32:
  case AMDIL::WHILELOOP:
    return true;
  }
}

void AMDGPU::utilAddLiveIn(llvm::MachineFunction * MF,
                           llvm::MachineRegisterInfo & MRI,
                           const llvm::TargetInstrInfo * TII,
                           unsigned physReg, unsigned virtReg)
{
    if (!MRI.isLiveIn(physReg)) {
      MRI.addLiveIn(physReg, virtReg);
      MF->front().addLiveIn(physReg);
      BuildMI(MF->front(), MF->front().begin(), DebugLoc(),
              TII->get(TargetOpcode::COPY), virtReg)
                .addReg(physReg);
    } else {
      MRI.replaceRegWith(virtReg, MRI.getLiveInVirtReg(physReg));
    }
}

//===-- SIAssignInterpRegs.cpp - TODO: Add brief description -------===//
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



#include "AMDGPU.h"
#include "AMDGPUUtil.h"
#include "AMDIL.h"
#include "SIMachineFunctionInfo.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"

using namespace llvm;

namespace {
  class SIAssignInterpRegsPass : public MachineFunctionPass {

  private:
    static char ID;
    TargetMachine &TM;

  public:
    SIAssignInterpRegsPass(TargetMachine &tm) :
      MachineFunctionPass(ID), TM(tm) { }

    virtual bool runOnMachineFunction(MachineFunction &MF);

    const char *getPassName() const { return "SI Assign intrpolation registers"; }
  };
} // End anonymous namespace

char SIAssignInterpRegsPass::ID = 0;

#define INTERP_VALUES 16

struct interp_info {
  bool enabled;
  unsigned regs[3];
  unsigned reg_count;
};


FunctionPass *llvm::createSIAssignInterpRegsPass(TargetMachine &tm) {
  return new SIAssignInterpRegsPass(tm);
}

bool SIAssignInterpRegsPass::runOnMachineFunction(MachineFunction &MF)
{

  struct interp_info InterpUse[INTERP_VALUES] = {
    {false, {AMDIL::PERSP_SAMPLE_I, AMDIL::PERSP_SAMPLE_J}, 2},
    {false, {AMDIL::PERSP_CENTER_I, AMDIL::PERSP_CENTER_J}, 2},
    {false, {AMDIL::PERSP_CENTROID_I, AMDIL::PERSP_CENTROID_J}, 2},
    {false, {AMDIL::PERSP_I_W, AMDIL::PERSP_J_W, AMDIL::PERSP_1_W}, 3},
    {false, {AMDIL::LINEAR_SAMPLE_I, AMDIL::LINEAR_SAMPLE_J}, 2},
    {false, {AMDIL::LINEAR_CENTER_I, AMDIL::LINEAR_CENTER_J}, 2},
    {false, {AMDIL::LINEAR_CENTROID_I, AMDIL::LINEAR_CENTROID_J}, 2},
    {false, {AMDIL::LINE_STIPPLE_TEX_COORD}, 1},
    {false, {AMDIL::POS_X_FLOAT}, 1},
    {false, {AMDIL::POS_Y_FLOAT}, 1},
    {false, {AMDIL::POS_Z_FLOAT}, 1},
    {false, {AMDIL::POS_W_FLOAT}, 1},
    {false, {AMDIL::FRONT_FACE}, 1},
    {false, {AMDIL::ANCILLARY}, 1},
    {false, {AMDIL::SAMPLE_COVERAGE}, 1},
    {false, {AMDIL::POS_FIXED_PT}, 1}
  };

  SIMachineFunctionInfo * MFI = MF.getInfo<SIMachineFunctionInfo>();
  MachineRegisterInfo &MRI = MF.getRegInfo();

  /* First pass, mark the interpolation values that are used. */
  for (unsigned interp_idx = 0; interp_idx < INTERP_VALUES; interp_idx++) {
    for (unsigned reg_idx = 0; reg_idx < InterpUse[interp_idx].reg_count;
                                                               reg_idx++) {
      InterpUse[interp_idx].enabled =
                            !MRI.use_empty(InterpUse[interp_idx].regs[reg_idx]);
    }
  }

  unsigned used_vgprs = 0;

  /* Second pass, replace with VGPRs. */
  for (unsigned interp_idx = 0; interp_idx < INTERP_VALUES; interp_idx++) {
    if (!InterpUse[interp_idx].enabled) {
      continue;
    }
    MFI->spi_ps_input_addr |= (1 << interp_idx);

    for (unsigned reg_idx = 0; reg_idx < InterpUse[interp_idx].reg_count;
                                                  reg_idx++, used_vgprs++) {
      unsigned new_reg = AMDIL::VReg_32RegisterClass->getRegister(used_vgprs);
      unsigned virt_reg = MRI.createVirtualRegister(AMDIL::VReg_32RegisterClass);
      MRI.replaceRegWith(InterpUse[interp_idx].regs[reg_idx], virt_reg);
      AMDGPU::utilAddLiveIn(&MF, MRI, TM.getInstrInfo(), new_reg, virt_reg);
    }
  }

  return false;
}

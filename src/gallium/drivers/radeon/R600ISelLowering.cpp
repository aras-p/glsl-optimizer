//===-- R600ISelLowering.cpp - TODO: Add brief description -------===//
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

#include "R600ISelLowering.h"
#include "R600InstrInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"

using namespace llvm;

R600TargetLowering::R600TargetLowering(TargetMachine &TM) :
    AMDGPUTargetLowering(TM),
    TII(static_cast<const R600InstrInfo*>(TM.getInstrInfo()))
{
  setOperationAction(ISD::MUL, MVT::i64, Expand);
//  setSchedulingPreference(Sched::VLIW);
  addRegisterClass(MVT::v4f32, &AMDIL::R600_Reg128RegClass);
  addRegisterClass(MVT::f32, &AMDIL::R600_Reg32RegClass);
  addRegisterClass(MVT::v4i32, &AMDIL::R600_Reg128RegClass);
  addRegisterClass(MVT::i32, &AMDIL::R600_Reg32RegClass);

  setOperationAction(ISD::EXTRACT_VECTOR_ELT, MVT::v4f32, Legal);
  setOperationAction(ISD::INSERT_VECTOR_ELT, MVT::v4f32, Legal);
  setOperationAction(ISD::EXTRACT_VECTOR_ELT, MVT::v4i32, Legal);
  setOperationAction(ISD::INSERT_VECTOR_ELT, MVT::v4i32, Legal);
}

MachineBasicBlock * R600TargetLowering::EmitInstrWithCustomInserter(
    MachineInstr * MI, MachineBasicBlock * BB) const
{
  MachineFunction * MF = BB->getParent();
  MachineRegisterInfo &MRI = MF->getRegInfo();

  switch (MI->getOpcode()) {
  default: return AMDGPUTargetLowering::EmitInstrWithCustomInserter(MI, BB);
  /* XXX: Use helper function from AMDGPULowerShaderInstructions here */
  case AMDIL::TGID_X:
    addLiveIn(MI, MF, MRI, TII, AMDIL::T1_X);
    break;
  case AMDIL::TGID_Y:
    addLiveIn(MI, MF, MRI, TII, AMDIL::T1_Y);
    break;
  case AMDIL::TGID_Z:
    addLiveIn(MI, MF, MRI, TII, AMDIL::T1_Z);
    break;
  case AMDIL::TIDIG_X:
    addLiveIn(MI, MF, MRI, TII, AMDIL::T0_X);
    break;
  case AMDIL::TIDIG_Y:
    addLiveIn(MI, MF, MRI, TII, AMDIL::T0_Y);
    break;
  case AMDIL::TIDIG_Z:
    addLiveIn(MI, MF, MRI, TII, AMDIL::T0_Z);
    break;
  case AMDIL::NGROUPS_X:
    lowerImplicitParameter(MI, *BB, MRI, 0);
    break;
  case AMDIL::NGROUPS_Y:
    lowerImplicitParameter(MI, *BB, MRI, 1);
    break;
  case AMDIL::NGROUPS_Z:
    lowerImplicitParameter(MI, *BB, MRI, 2);
    break;
  case AMDIL::GLOBAL_SIZE_X:
    lowerImplicitParameter(MI, *BB, MRI, 3);
    break;
  case AMDIL::GLOBAL_SIZE_Y:
    lowerImplicitParameter(MI, *BB, MRI, 4);
    break;
  case AMDIL::GLOBAL_SIZE_Z:
    lowerImplicitParameter(MI, *BB, MRI, 5);
    break;
  case AMDIL::LOCAL_SIZE_X:
    lowerImplicitParameter(MI, *BB, MRI, 6);
    break;
  case AMDIL::LOCAL_SIZE_Y:
    lowerImplicitParameter(MI, *BB, MRI, 7);
    break;
  case AMDIL::LOCAL_SIZE_Z:
    lowerImplicitParameter(MI, *BB, MRI, 8);
    break;
  }
  MI->eraseFromParent();
  return BB;
}

void R600TargetLowering::lowerImplicitParameter(MachineInstr *MI, MachineBasicBlock &BB,
    MachineRegisterInfo & MRI, unsigned dword_offset) const
{
  MachineBasicBlock::iterator I = *MI;
  unsigned offsetReg = MRI.createVirtualRegister(&AMDIL::R600_TReg32_XRegClass);
  MRI.setRegClass(MI->getOperand(0).getReg(), &AMDIL::R600_TReg32_XRegClass);

  BuildMI(BB, I, BB.findDebugLoc(I), TII->get(AMDIL::MOV), offsetReg)
          .addReg(AMDIL::ALU_LITERAL_X)
          .addImm(dword_offset * 4);

  BuildMI(BB, I, BB.findDebugLoc(I), TII->get(AMDIL::VTX_READ_eg))
          .addOperand(MI->getOperand(0))
          .addReg(offsetReg)
          .addImm(0);
}

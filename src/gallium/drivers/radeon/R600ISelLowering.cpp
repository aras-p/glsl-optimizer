//===-- R600ISelLowering.cpp - R600 DAG Lowering Implementation -----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Most of the DAG lowering is handled in AMDILISelLowering.cpp.  This file
// is mostly EmitInstrWithCustomInserter().
//
//===----------------------------------------------------------------------===//

#include "R600ISelLowering.h"
#include "R600InstrInfo.h"
#include "R600MachineFunctionInfo.h"
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

  setOperationAction(ISD::FSUB, MVT::f32, Expand);

#if 0

  setTargetDAGCombine(ISD::Constant);
  setTargetDAGCombine(ISD::ConstantFP);

#endif
}

MachineBasicBlock * R600TargetLowering::EmitInstrWithCustomInserter(
    MachineInstr * MI, MachineBasicBlock * BB) const
{
  MachineFunction * MF = BB->getParent();
  MachineRegisterInfo &MRI = MF->getRegInfo();
  MachineBasicBlock::iterator I = *MI;

  switch (MI->getOpcode()) {
  default: return AMDGPUTargetLowering::EmitInstrWithCustomInserter(MI, BB);
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

  case AMDIL::R600_LOAD_CONST:
    {
      int64_t RegIndex = MI->getOperand(1).getImm();
      unsigned ConstantReg = AMDIL::R600_CReg32RegClass.getRegister(RegIndex);
      BuildMI(*BB, I, BB->findDebugLoc(I), TII->get(AMDIL::COPY))
                  .addOperand(MI->getOperand(0))
                  .addReg(ConstantReg);
      break;
    }

  case AMDIL::LOAD_INPUT:
    {
      int64_t RegIndex = MI->getOperand(1).getImm();
      addLiveIn(MI, MF, MRI, TII,
                AMDIL::R600_TReg32RegClass.getRegister(RegIndex));
      break;
    }
  case AMDIL::STORE_OUTPUT:
    {
      int64_t OutputIndex = MI->getOperand(1).getImm();
      unsigned OutputReg = AMDIL::R600_TReg32RegClass.getRegister(OutputIndex);

      BuildMI(*BB, I, BB->findDebugLoc(I), TII->get(AMDIL::COPY), OutputReg)
                  .addOperand(MI->getOperand(0));

      if (!MRI.isLiveOut(OutputReg)) {
        MRI.addLiveOut(OutputReg);
      }
      break;
    }

  case AMDIL::RESERVE_REG:
    {
      R600MachineFunctionInfo * MFI = MF->getInfo<R600MachineFunctionInfo>();
      int64_t ReservedIndex = MI->getOperand(0).getImm();
      unsigned ReservedReg =
                          AMDIL::R600_TReg32RegClass.getRegister(ReservedIndex);
      MFI->ReservedRegs.push_back(ReservedReg);
      break;
    }

  case AMDIL::TXD:
    {
      unsigned t0 = MRI.createVirtualRegister(AMDIL::R600_Reg128RegisterClass);
      unsigned t1 = MRI.createVirtualRegister(AMDIL::R600_Reg128RegisterClass);

      BuildMI(*BB, I, BB->findDebugLoc(I), TII->get(AMDIL::TEX_SET_GRADIENTS_H), t0)
              .addOperand(MI->getOperand(3))
              .addOperand(MI->getOperand(4))
              .addOperand(MI->getOperand(5));
      BuildMI(*BB, I, BB->findDebugLoc(I), TII->get(AMDIL::TEX_SET_GRADIENTS_V), t1)
              .addOperand(MI->getOperand(2))
              .addOperand(MI->getOperand(4))
              .addOperand(MI->getOperand(5));
      BuildMI(*BB, I, BB->findDebugLoc(I), TII->get(AMDIL::TEX_SAMPLE_G))
              .addOperand(MI->getOperand(0))
              .addOperand(MI->getOperand(1))
              .addOperand(MI->getOperand(4))
              .addOperand(MI->getOperand(5))
              .addReg(t0, RegState::Implicit)
              .addReg(t1, RegState::Implicit);
      break;
    }
  case AMDIL::TXD_SHADOW:
    {
      unsigned t0 = MRI.createVirtualRegister(AMDIL::R600_Reg128RegisterClass);
      unsigned t1 = MRI.createVirtualRegister(AMDIL::R600_Reg128RegisterClass);

      BuildMI(*BB, I, BB->findDebugLoc(I), TII->get(AMDIL::TEX_SET_GRADIENTS_H), t0)
              .addOperand(MI->getOperand(3))
              .addOperand(MI->getOperand(4))
              .addOperand(MI->getOperand(5));
      BuildMI(*BB, I, BB->findDebugLoc(I), TII->get(AMDIL::TEX_SET_GRADIENTS_V), t1)
              .addOperand(MI->getOperand(2))
              .addOperand(MI->getOperand(4))
              .addOperand(MI->getOperand(5));
      BuildMI(*BB, I, BB->findDebugLoc(I), TII->get(AMDIL::TEX_SAMPLE_C_G))
              .addOperand(MI->getOperand(0))
              .addOperand(MI->getOperand(1))
              .addOperand(MI->getOperand(4))
              .addOperand(MI->getOperand(5))
              .addReg(t0, RegState::Implicit)
              .addReg(t1, RegState::Implicit);
      break;
    }


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

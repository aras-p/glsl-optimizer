//===-- R600ISelLowering.cpp - R600 DAG Lowering Implementation -----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Most of the DAG lowering is handled in AMDGPUISelLowering.cpp.  This file
// is mostly EmitInstrWithCustomInserter().
//
//===----------------------------------------------------------------------===//

#include "R600ISelLowering.h"
#include "R600Defines.h"
#include "R600InstrInfo.h"
#include "R600MachineFunctionInfo.h"
#include "llvm/Argument.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SelectionDAG.h"

using namespace llvm;

R600TargetLowering::R600TargetLowering(TargetMachine &TM) :
    AMDGPUTargetLowering(TM),
    TII(static_cast<const R600InstrInfo*>(TM.getInstrInfo()))
{
  setOperationAction(ISD::MUL, MVT::i64, Expand);
  addRegisterClass(MVT::v4f32, &AMDGPU::R600_Reg128RegClass);
  addRegisterClass(MVT::f32, &AMDGPU::R600_Reg32RegClass);
  addRegisterClass(MVT::v4i32, &AMDGPU::R600_Reg128RegClass);
  addRegisterClass(MVT::i32, &AMDGPU::R600_Reg32RegClass);
  computeRegisterProperties();

  setOperationAction(ISD::BR_CC, MVT::i32, Custom);
  setOperationAction(ISD::BR_CC, MVT::f32, Custom);
  
  setOperationAction(ISD::FSUB, MVT::f32, Expand);

  setOperationAction(ISD::INTRINSIC_VOID, MVT::Other, Custom);
  setOperationAction(ISD::INTRINSIC_WO_CHAIN, MVT::Other, Custom);

  setOperationAction(ISD::ROTL, MVT::i32, Custom);

  setOperationAction(ISD::SELECT_CC, MVT::f32, Custom);
  setOperationAction(ISD::SELECT_CC, MVT::i32, Custom);

  setOperationAction(ISD::SETCC, MVT::i32, Custom);
  setOperationAction(ISD::SETCC, MVT::f32, Custom);
  setOperationAction(ISD::FP_TO_UINT, MVT::i1, Custom);
  setSchedulingPreference(Sched::VLIW);
}

MachineBasicBlock * R600TargetLowering::EmitInstrWithCustomInserter(
    MachineInstr * MI, MachineBasicBlock * BB) const
{
  MachineFunction * MF = BB->getParent();
  MachineRegisterInfo &MRI = MF->getRegInfo();
  MachineBasicBlock::iterator I = *MI;

  switch (MI->getOpcode()) {
  default: return AMDGPUTargetLowering::EmitInstrWithCustomInserter(MI, BB);
  case AMDGPU::SHADER_TYPE: break;
  case AMDGPU::CLAMP_R600:
    {
      MachineInstr *NewMI =
        BuildMI(*BB, I, BB->findDebugLoc(I), TII->get(AMDGPU::MOV))
               .addOperand(MI->getOperand(0))
               .addOperand(MI->getOperand(1))
               .addImm(0) // Flags
               .addReg(AMDGPU::PRED_SEL_OFF);
      TII->addFlag(NewMI, 0, MO_FLAG_CLAMP);
      break;
    }
  case AMDGPU::FABS_R600:
    {
      MachineInstr *NewMI =
        BuildMI(*BB, I, BB->findDebugLoc(I), TII->get(AMDGPU::MOV))
               .addOperand(MI->getOperand(0))
               .addOperand(MI->getOperand(1))
               .addImm(0) // Flags
               .addReg(AMDGPU::PRED_SEL_OFF);
      TII->addFlag(NewMI, 1, MO_FLAG_ABS);
      break;
    }

  case AMDGPU::FNEG_R600:
    {
      MachineInstr *NewMI =
        BuildMI(*BB, I, BB->findDebugLoc(I), TII->get(AMDGPU::MOV))
                .addOperand(MI->getOperand(0))
                .addOperand(MI->getOperand(1))
                .addImm(0) // Flags
                .addReg(AMDGPU::PRED_SEL_OFF);
      TII->addFlag(NewMI, 1, MO_FLAG_NEG);
    break;
    }

  case AMDGPU::R600_LOAD_CONST:
    {
      int64_t RegIndex = MI->getOperand(1).getImm();
      unsigned ConstantReg = AMDGPU::R600_CReg32RegClass.getRegister(RegIndex);
      BuildMI(*BB, I, BB->findDebugLoc(I), TII->get(AMDGPU::COPY))
                  .addOperand(MI->getOperand(0))
                  .addReg(ConstantReg);
      break;
    }

  case AMDGPU::MASK_WRITE:
    {
      unsigned maskedRegister = MI->getOperand(0).getReg();
      assert(TargetRegisterInfo::isVirtualRegister(maskedRegister));
      MachineInstr * defInstr = MRI.getVRegDef(maskedRegister);
      TII->addFlag(defInstr, 0, MO_FLAG_MASK);
      // Return early so the instruction is not erased
      return BB;
    }

  case AMDGPU::RAT_WRITE_CACHELESS_eg:
    {
      // Convert to DWORD address
      unsigned NewAddr = MRI.createVirtualRegister(
                                             &AMDGPU::R600_TReg32_XRegClass);
      unsigned ShiftValue = MRI.createVirtualRegister(
                                              &AMDGPU::R600_TReg32RegClass);
      unsigned EOP = (llvm::next(I)->getOpcode() == AMDGPU::RETURN) ? 1 : 0;

      // XXX In theory, we should be able to pass ShiftValue directly to
      // the LSHR_eg instruction as an inline literal, but I tried doing it
      // this way and it didn't produce the correct results.
      BuildMI(*BB, I, BB->findDebugLoc(I), TII->get(AMDGPU::MOV_IMM_I32),
              ShiftValue)
              .addReg(AMDGPU::ALU_LITERAL_X)
              .addReg(AMDGPU::PRED_SEL_OFF)
              .addImm(2);
      BuildMI(*BB, I, BB->findDebugLoc(I), TII->get(AMDGPU::LSHR_eg), NewAddr)
              .addOperand(MI->getOperand(1))
              .addReg(ShiftValue)
              .addReg(AMDGPU::PRED_SEL_OFF);
      BuildMI(*BB, I, BB->findDebugLoc(I), TII->get(MI->getOpcode()))
              .addOperand(MI->getOperand(0))
              .addReg(NewAddr)
              .addImm(EOP); // Set End of program bit
      break;
    }

  case AMDGPU::RESERVE_REG:
    {
      R600MachineFunctionInfo * MFI = MF->getInfo<R600MachineFunctionInfo>();
      int64_t ReservedIndex = MI->getOperand(0).getImm();
      unsigned ReservedReg =
                          AMDGPU::R600_TReg32RegClass.getRegister(ReservedIndex);
      MFI->ReservedRegs.push_back(ReservedReg);
      break;
    }

  case AMDGPU::TXD:
    {
      unsigned t0 = MRI.createVirtualRegister(&AMDGPU::R600_Reg128RegClass);
      unsigned t1 = MRI.createVirtualRegister(&AMDGPU::R600_Reg128RegClass);

      BuildMI(*BB, I, BB->findDebugLoc(I), TII->get(AMDGPU::TEX_SET_GRADIENTS_H), t0)
              .addOperand(MI->getOperand(3))
              .addOperand(MI->getOperand(4))
              .addOperand(MI->getOperand(5));
      BuildMI(*BB, I, BB->findDebugLoc(I), TII->get(AMDGPU::TEX_SET_GRADIENTS_V), t1)
              .addOperand(MI->getOperand(2))
              .addOperand(MI->getOperand(4))
              .addOperand(MI->getOperand(5));
      BuildMI(*BB, I, BB->findDebugLoc(I), TII->get(AMDGPU::TEX_SAMPLE_G))
              .addOperand(MI->getOperand(0))
              .addOperand(MI->getOperand(1))
              .addOperand(MI->getOperand(4))
              .addOperand(MI->getOperand(5))
              .addReg(t0, RegState::Implicit)
              .addReg(t1, RegState::Implicit);
      break;
    }
  case AMDGPU::TXD_SHADOW:
    {
      unsigned t0 = MRI.createVirtualRegister(AMDGPU::R600_Reg128RegisterClass);
      unsigned t1 = MRI.createVirtualRegister(AMDGPU::R600_Reg128RegisterClass);

      BuildMI(*BB, I, BB->findDebugLoc(I), TII->get(AMDGPU::TEX_SET_GRADIENTS_H), t0)
              .addOperand(MI->getOperand(3))
              .addOperand(MI->getOperand(4))
              .addOperand(MI->getOperand(5));
      BuildMI(*BB, I, BB->findDebugLoc(I), TII->get(AMDGPU::TEX_SET_GRADIENTS_V), t1)
              .addOperand(MI->getOperand(2))
              .addOperand(MI->getOperand(4))
              .addOperand(MI->getOperand(5));
      BuildMI(*BB, I, BB->findDebugLoc(I), TII->get(AMDGPU::TEX_SAMPLE_C_G))
              .addOperand(MI->getOperand(0))
              .addOperand(MI->getOperand(1))
              .addOperand(MI->getOperand(4))
              .addOperand(MI->getOperand(5))
              .addReg(t0, RegState::Implicit)
              .addReg(t1, RegState::Implicit);
      break;
    }
  case AMDGPU::BRANCH:
      BuildMI(*BB, I, BB->findDebugLoc(I), TII->get(AMDGPU::JUMP))
              .addOperand(MI->getOperand(0))
              .addReg(0);
      break;
  case AMDGPU::BRANCH_COND_f32:
    {
      MachineInstr *NewMI =
        BuildMI(*BB, I, BB->findDebugLoc(I), TII->get(AMDGPU::PRED_X))
                .addReg(AMDGPU::PREDICATE_BIT)
                .addOperand(MI->getOperand(1))
                .addImm(OPCODE_IS_NOT_ZERO)
                .addImm(0); // Flags
      TII->addFlag(NewMI, 1, MO_FLAG_PUSH);
      BuildMI(*BB, I, BB->findDebugLoc(I), TII->get(AMDGPU::JUMP))
              .addOperand(MI->getOperand(0))
              .addReg(AMDGPU::PREDICATE_BIT, RegState::Kill);
      break;
    }
  case AMDGPU::BRANCH_COND_i32:
    {
      MachineInstr *NewMI =
        BuildMI(*BB, I, BB->findDebugLoc(I), TII->get(AMDGPU::PRED_X))
              .addReg(AMDGPU::PREDICATE_BIT)
              .addOperand(MI->getOperand(1))
              .addImm(OPCODE_IS_NOT_ZERO_INT)
              .addImm(0); // Flags
      TII->addFlag(NewMI, 1, MO_FLAG_PUSH);
      BuildMI(*BB, I, BB->findDebugLoc(I), TII->get(AMDGPU::JUMP))
             .addOperand(MI->getOperand(0))
              .addReg(AMDGPU::PREDICATE_BIT, RegState::Kill);
      break;
    }
  }

  MI->eraseFromParent();
  return BB;
}

//===----------------------------------------------------------------------===//
// Custom DAG Lowering Operations
//===----------------------------------------------------------------------===//

using namespace llvm::Intrinsic;
using namespace llvm::AMDGPUIntrinsic;

SDValue R600TargetLowering::LowerOperation(SDValue Op, SelectionDAG &DAG) const
{
  switch (Op.getOpcode()) {
  default: return AMDGPUTargetLowering::LowerOperation(Op, DAG);
  case ISD::BR_CC: return LowerBR_CC(Op, DAG);
  case ISD::ROTL: return LowerROTL(Op, DAG);
  case ISD::SELECT_CC: return LowerSELECT_CC(Op, DAG);
  case ISD::SETCC: return LowerSETCC(Op, DAG);
  case ISD::INTRINSIC_VOID: {
    SDValue Chain = Op.getOperand(0);
    unsigned IntrinsicID =
                         cast<ConstantSDNode>(Op.getOperand(1))->getZExtValue();
    switch (IntrinsicID) {
    case AMDGPUIntrinsic::AMDGPU_store_output: {
      MachineFunction &MF = DAG.getMachineFunction();
      MachineRegisterInfo &MRI = MF.getRegInfo();
      int64_t RegIndex = cast<ConstantSDNode>(Op.getOperand(3))->getZExtValue();
      unsigned Reg = AMDGPU::R600_TReg32RegClass.getRegister(RegIndex);
      if (!MRI.isLiveOut(Reg)) {
        MRI.addLiveOut(Reg);
      }
      return DAG.getCopyToReg(Chain, Op.getDebugLoc(), Reg, Op.getOperand(2));
    }
    // default for switch(IntrinsicID)
    default: break;
    }
    // break out of case ISD::INTRINSIC_VOID in switch(Op.getOpcode())
    break;
  }
  case ISD::INTRINSIC_WO_CHAIN: {
    unsigned IntrinsicID =
                         cast<ConstantSDNode>(Op.getOperand(0))->getZExtValue();
    EVT VT = Op.getValueType();
    DebugLoc DL = Op.getDebugLoc();
    switch(IntrinsicID) {
    default: return AMDGPUTargetLowering::LowerOperation(Op, DAG);
    case AMDGPUIntrinsic::R600_load_input: {
      int64_t RegIndex = cast<ConstantSDNode>(Op.getOperand(1))->getZExtValue();
      unsigned Reg = AMDGPU::R600_TReg32RegClass.getRegister(RegIndex);
      return CreateLiveInRegister(DAG, &AMDGPU::R600_TReg32RegClass, Reg, VT);
    }

    case r600_read_ngroups_x:
      return LowerImplicitParameter(DAG, VT, DL, 0);
    case r600_read_ngroups_y:
      return LowerImplicitParameter(DAG, VT, DL, 1);
    case r600_read_ngroups_z:
      return LowerImplicitParameter(DAG, VT, DL, 2);
    case r600_read_global_size_x:
      return LowerImplicitParameter(DAG, VT, DL, 3);
    case r600_read_global_size_y:
      return LowerImplicitParameter(DAG, VT, DL, 4);
    case r600_read_global_size_z:
      return LowerImplicitParameter(DAG, VT, DL, 5);
    case r600_read_local_size_x:
      return LowerImplicitParameter(DAG, VT, DL, 6);
    case r600_read_local_size_y:
      return LowerImplicitParameter(DAG, VT, DL, 7);
    case r600_read_local_size_z:
      return LowerImplicitParameter(DAG, VT, DL, 8);

    case r600_read_tgid_x:
      return CreateLiveInRegister(DAG, &AMDGPU::R600_TReg32RegClass,
                                  AMDGPU::T1_X, VT);
    case r600_read_tgid_y:
      return CreateLiveInRegister(DAG, &AMDGPU::R600_TReg32RegClass,
                                  AMDGPU::T1_Y, VT);
    case r600_read_tgid_z:
      return CreateLiveInRegister(DAG, &AMDGPU::R600_TReg32RegClass,
                                  AMDGPU::T1_Z, VT);
    case r600_read_tidig_x:
      return CreateLiveInRegister(DAG, &AMDGPU::R600_TReg32RegClass,
                                  AMDGPU::T0_X, VT);
    case r600_read_tidig_y:
      return CreateLiveInRegister(DAG, &AMDGPU::R600_TReg32RegClass,
                                  AMDGPU::T0_Y, VT);
    case r600_read_tidig_z:
      return CreateLiveInRegister(DAG, &AMDGPU::R600_TReg32RegClass,
                                  AMDGPU::T0_Z, VT);
    }
    // break out of case ISD::INTRINSIC_WO_CHAIN in switch(Op.getOpcode())
    break;
  }
  } // end switch(Op.getOpcode())
  return SDValue();
}

void R600TargetLowering::ReplaceNodeResults(SDNode *N,
                                            SmallVectorImpl<SDValue> &Results, 
                                            SelectionDAG &DAG) const
{
  switch (N->getOpcode()) {
  default: return;
  case ISD::FP_TO_UINT: Results.push_back(LowerFPTOUINT(N->getOperand(0), DAG));
  }
}

SDValue R600TargetLowering::LowerFPTOUINT(SDValue Op, SelectionDAG &DAG) const
{
  return DAG.getNode(
      ISD::SETCC,
      Op.getDebugLoc(),
      MVT::i1,
      Op, DAG.getConstantFP(0.0f, MVT::f32),
      DAG.getCondCode(ISD::SETNE)
      );
}

SDValue R600TargetLowering::LowerBR_CC(SDValue Op, SelectionDAG &DAG) const
{
  SDValue Chain = Op.getOperand(0);
  SDValue CC = Op.getOperand(1);
  SDValue LHS   = Op.getOperand(2);
  SDValue RHS   = Op.getOperand(3);
  SDValue JumpT  = Op.getOperand(4);
  SDValue CmpValue;
  SDValue Result;
  
  if (LHS.getValueType() == MVT::i32) {
    CmpValue = DAG.getNode(
        ISD::SELECT_CC,
        Op.getDebugLoc(),
        MVT::i32,
        LHS, RHS,
        DAG.getConstant(-1, MVT::i32),
        DAG.getConstant(0, MVT::i32),
        CC);
  } else if (LHS.getValueType() == MVT::f32) {
    CmpValue = DAG.getNode(
        ISD::SELECT_CC,
        Op.getDebugLoc(),
        MVT::f32,
        LHS, RHS,
        DAG.getConstantFP(1.0f, MVT::f32),
        DAG.getConstantFP(0.0f, MVT::f32),
        CC);
  } else {
    assert(0 && "Not valid type for br_cc");
  }
  Result = DAG.getNode(
      AMDGPUISD::BRANCH_COND,
      CmpValue.getDebugLoc(),
      MVT::Other, Chain,
      JumpT, CmpValue);
  return Result;
}

SDValue R600TargetLowering::LowerImplicitParameter(SelectionDAG &DAG, EVT VT,
                                                   DebugLoc DL,
                                                   unsigned DwordOffset) const
{
  unsigned ByteOffset = DwordOffset * 4;
  PointerType * PtrType = PointerType::get(VT.getTypeForEVT(*DAG.getContext()),
                                      AMDGPUAS::PARAM_I_ADDRESS);

  // We shouldn't be using an offset wider than 16-bits for implicit parameters.
  assert(isInt<16>(ByteOffset));

  return DAG.getLoad(VT, DL, DAG.getEntryNode(),
                     DAG.getConstant(ByteOffset, MVT::i32), // PTR
                     MachinePointerInfo(ConstantPointerNull::get(PtrType)),
                     false, false, false, 0);
}

SDValue R600TargetLowering::LowerROTL(SDValue Op, SelectionDAG &DAG) const
{
  DebugLoc DL = Op.getDebugLoc();
  EVT VT = Op.getValueType();

  return DAG.getNode(AMDGPUISD::BITALIGN, DL, VT,
                     Op.getOperand(0),
                     Op.getOperand(0),
                     DAG.getNode(ISD::SUB, DL, VT,
                                 DAG.getConstant(32, MVT::i32),
                                 Op.getOperand(1)));
}

SDValue R600TargetLowering::LowerSELECT_CC(SDValue Op, SelectionDAG &DAG) const
{
  DebugLoc DL = Op.getDebugLoc();
  EVT VT = Op.getValueType();

  SDValue LHS = Op.getOperand(0);
  SDValue RHS = Op.getOperand(1);
  SDValue True = Op.getOperand(2);
  SDValue False = Op.getOperand(3);
  SDValue CC = Op.getOperand(4);
  ISD::CondCode CCOpcode = cast<CondCodeSDNode>(CC)->get();
  SDValue Temp;

  // LHS and RHS are guaranteed to be the same value type
  EVT CompareVT = LHS.getValueType();

  // We need all the operands of SELECT_CC to have the same value type, so if
  // necessary we need to convert LHS and RHS to be the same type True and
  // False.  True and False are guaranteed to have the same type as this
  // SELECT_CC node.
  
  if (isHWTrueValue(True) && isHWFalseValue(False)) {
    if (CompareVT !=  VT) {
      if (VT == MVT::f32 && CompareVT == MVT::i32) {
        SDValue Boolean = DAG.getNode(ISD::SELECT_CC, DL, CompareVT,
            LHS, RHS,
            DAG.getConstant(-1, MVT::i32),
            DAG.getConstant(0, MVT::i32),
            CC);
        return DAG.getNode(ISD::UINT_TO_FP, DL, VT, Boolean);
      } else if (VT == MVT::i32 && CompareVT == MVT::f32) {
        SDValue BoolAsFlt = DAG.getNode(ISD::SELECT_CC, DL, CompareVT,
            LHS, RHS,
            DAG.getConstantFP(1.0f, MVT::f32),
            DAG.getConstantFP(0.0f, MVT::f32),
            CC);
        return DAG.getNode(ISD::FP_TO_UINT, DL, VT, BoolAsFlt);
      } else {
        // I don't think there will be any other type pairings.
        assert(!"Unhandled operand type parings in SELECT_CC");
      }
    } else {
      return DAG.getNode(ISD::SELECT_CC, DL, VT, LHS, RHS, True, False, CC);
    }
  }


  // XXX If True is a hardware TRUE value and False is a hardware FALSE value,
  // we can handle this with a native instruction, but we need to swap true
  // and false and change the conditional.
  if (isHWTrueValue(False) && isHWFalseValue(True)) {
  }

  // XXX Check if we can lower this to a SELECT or if it is supported by a native
  // operation. (The code below does this but we don't have the Instruction
  // selection patterns to do this yet.
#if 0
  if (isZero(LHS) || isZero(RHS)) {
    SDValue Cond = (isZero(LHS) ? RHS : LHS);
    bool SwapTF = false;
    switch (CCOpcode) {
    case ISD::SETOEQ:
    case ISD::SETUEQ:
    case ISD::SETEQ:
      SwapTF = true;
      // Fall through
    case ISD::SETONE:
    case ISD::SETUNE:
    case ISD::SETNE:
      // We can lower to select
      if (SwapTF) {
        Temp = True;
        True = False;
        False = Temp;
      }
      // CNDE
      return DAG.getNode(ISD::SELECT, DL, VT, Cond, True, False);
    default:
      // Supported by a native operation (CNDGE, CNDGT)
      return DAG.getNode(ISD::SELECT_CC, DL, VT, LHS, RHS, True, False, CC);
    }
  }
#endif

  // If we make it this for it means we have no native instructions to handle
  // this SELECT_CC, so we must lower it.
  SDValue HWTrue, HWFalse;

  if (VT == MVT::f32) {
    HWTrue = DAG.getConstantFP(1.0f, VT);
    HWFalse = DAG.getConstantFP(0.0f, VT);
  } else if (VT == MVT::i32) {
    HWTrue = DAG.getConstant(-1, VT);
    HWFalse = DAG.getConstant(0, VT);
  }
  else {
    assert(!"Unhandled value type in LowerSELECT_CC");
  }

  // Lower this unsupported SELECT_CC into a combination of two supported
  // SELECT_CC operations.
  SDValue Cond = DAG.getNode(ISD::SELECT_CC, DL, VT, LHS, RHS, HWTrue, HWFalse, CC);

  // Convert floating point condition to i1
  if (VT == MVT::f32) {
    Cond = DAG.getNode(ISD::FP_TO_SINT, DL, MVT::i32,
                       DAG.getNode(ISD::FNEG, DL, VT, Cond));
  }

  return DAG.getNode(ISD::SELECT, DL, VT, Cond, True, False);
}

SDValue R600TargetLowering::LowerSETCC(SDValue Op, SelectionDAG &DAG) const
{
  SDValue Cond;
  SDValue LHS = Op.getOperand(0);
  SDValue RHS = Op.getOperand(1);
  SDValue CC  = Op.getOperand(2);
  DebugLoc DL = Op.getDebugLoc();
  assert(Op.getValueType() == MVT::i32);
  if (LHS.getValueType() == MVT::i32) {
    Cond = DAG.getNode(
        ISD::SELECT_CC,
        Op.getDebugLoc(),
        MVT::i32,
        LHS, RHS,
        DAG.getConstant(-1, MVT::i32),
        DAG.getConstant(0, MVT::i32),
        CC);
  } else if (LHS.getValueType() == MVT::f32) {
    Cond = DAG.getNode(
        ISD::SELECT_CC,
        Op.getDebugLoc(),
        MVT::f32,
        LHS, RHS,
        DAG.getConstantFP(1.0f, MVT::f32),
        DAG.getConstantFP(0.0f, MVT::f32),
        CC);
    Cond = DAG.getNode(
        ISD::FP_TO_SINT,
        DL,
        MVT::i32,
        Cond);
  } else {
    assert(0 && "Not valid type for set_cc");
  }
  Cond = DAG.getNode(
      ISD::AND,
      DL,
      MVT::i32,
      DAG.getConstant(1, MVT::i32),
      Cond);
  return Cond;
}

// XXX Only kernel functions are supporte, so we can assume for now that
// every function is a kernel function, but in the future we should use
// separate calling conventions for kernel and non-kernel functions.
// Only kernel functions are supported, so we can assume for now
SDValue R600TargetLowering::LowerFormalArguments(
                                      SDValue Chain,
                                      CallingConv::ID CallConv,
                                      bool isVarArg,
                                      const SmallVectorImpl<ISD::InputArg> &Ins,
                                      DebugLoc DL, SelectionDAG &DAG,
                                      SmallVectorImpl<SDValue> &InVals) const
{
  unsigned ParamOffsetBytes = 36;
  for (unsigned i = 0, e = Ins.size(); i < e; ++i) {
    EVT VT = Ins[i].VT;
    PointerType *PtrTy = PointerType::get(VT.getTypeForEVT(*DAG.getContext()),
                                                    AMDGPUAS::PARAM_I_ADDRESS);
    SDValue Arg = DAG.getLoad(VT, DL, DAG.getRoot(),
                                DAG.getConstant(ParamOffsetBytes, MVT::i32),
                                MachinePointerInfo(new Argument(PtrTy)),
                                false, false, false, 4);
    InVals.push_back(Arg);
    ParamOffsetBytes += (VT.getStoreSize());
  }
  return Chain;
}

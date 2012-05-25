//===-- AMDILISelLowering.cpp - AMDIL DAG Lowering Implementation ---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//
//
// This file implements the interfaces that AMDIL uses to lower LLVM code into a
// selection DAG.
//
//===----------------------------------------------------------------------===//

#include "AMDILISelLowering.h"
#include "AMDILDevices.h"
#include "AMDILIntrinsicInfo.h"
#include "AMDILSubtarget.h"
#include "AMDILTargetMachine.h"
#include "AMDILUtilityFunctions.h"
#include "llvm/CallingConv.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/PseudoSourceValue.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Instructions.h"
#include "llvm/Intrinsics.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetOptions.h"

using namespace llvm;
#define ISDBITCAST  ISD::BITCAST
#define MVTGLUE     MVT::Glue
//===----------------------------------------------------------------------===//
// Calling Convention Implementation
//===----------------------------------------------------------------------===//
#include "AMDILGenCallingConv.inc"

//===----------------------------------------------------------------------===//
// TargetLowering Implementation Help Functions Begin
//===----------------------------------------------------------------------===//
  static SDValue
getConversionNode(SelectionDAG &DAG, SDValue& Src, SDValue& Dst, bool asType)
{
  DebugLoc DL = Src.getDebugLoc();
  EVT svt = Src.getValueType().getScalarType();
  EVT dvt = Dst.getValueType().getScalarType();
  if (svt.isFloatingPoint() && dvt.isFloatingPoint()) {
    if (dvt.bitsGT(svt)) {
      Src = DAG.getNode(ISD::FP_EXTEND, DL, dvt, Src);
    } else if (svt.bitsLT(svt)) {
      Src = DAG.getNode(ISD::FP_ROUND, DL, dvt, Src,
          DAG.getConstant(1, MVT::i32));
    }
  } else if (svt.isInteger() && dvt.isInteger()) {
    if (!svt.bitsEq(dvt)) {
      Src = DAG.getSExtOrTrunc(Src, DL, dvt);
    } else {
      Src = DAG.getNode(AMDILISD::MOVE, DL, dvt, Src);
    }
  } else if (svt.isInteger()) {
    unsigned opcode = (asType) ? ISDBITCAST : ISD::SINT_TO_FP;
    if (!svt.bitsEq(dvt)) {
      if (dvt.getSimpleVT().SimpleTy == MVT::f32) {
        Src = DAG.getSExtOrTrunc(Src, DL, MVT::i32);
      } else if (dvt.getSimpleVT().SimpleTy == MVT::f64) {
        Src = DAG.getSExtOrTrunc(Src, DL, MVT::i64);
      } else {
        assert(0 && "We only support 32 and 64bit fp types");
      }
    }
    Src = DAG.getNode(opcode, DL, dvt, Src);
  } else if (dvt.isInteger()) {
    unsigned opcode = (asType) ? ISDBITCAST : ISD::FP_TO_SINT;
    if (svt.getSimpleVT().SimpleTy == MVT::f32) {
      Src = DAG.getNode(opcode, DL, MVT::i32, Src);
    } else if (svt.getSimpleVT().SimpleTy == MVT::f64) {
      Src = DAG.getNode(opcode, DL, MVT::i64, Src);
    } else {
      assert(0 && "We only support 32 and 64bit fp types");
    }
    Src = DAG.getSExtOrTrunc(Src, DL, dvt);
  }
  return Src;
}
// CondCCodeToCC - Convert a DAG condition code to a AMDIL CC
// condition.
  static AMDILCC::CondCodes
CondCCodeToCC(ISD::CondCode CC, const MVT::SimpleValueType& type)
{
  switch (CC) {
    default:
      {
        errs()<<"Condition Code: "<< (unsigned int)CC<<"\n";
        assert(0 && "Unknown condition code!");
      }
    case ISD::SETO:
      switch(type) {
        case MVT::f32:
          return AMDILCC::IL_CC_F_O;
        case MVT::f64:
          return AMDILCC::IL_CC_D_O;
        default:
          assert(0 && "Opcode combination not generated correctly!");
          return AMDILCC::COND_ERROR;
      };
    case ISD::SETUO:
      switch(type) {
        case MVT::f32:
          return AMDILCC::IL_CC_F_UO;
        case MVT::f64:
          return AMDILCC::IL_CC_D_UO;
        default:
          assert(0 && "Opcode combination not generated correctly!");
          return AMDILCC::COND_ERROR;
      };
    case ISD::SETGT:
      switch (type) {
        case MVT::i1:
        case MVT::i8:
        case MVT::i16:
        case MVT::i32:
          return AMDILCC::IL_CC_I_GT;
        case MVT::f32:
          return AMDILCC::IL_CC_F_GT;
        case MVT::f64:
          return AMDILCC::IL_CC_D_GT;
        case MVT::i64:
          return AMDILCC::IL_CC_L_GT;
        default:
          assert(0 && "Opcode combination not generated correctly!");
          return AMDILCC::COND_ERROR;
      };
    case ISD::SETGE:
      switch (type) {
        case MVT::i1:
        case MVT::i8:
        case MVT::i16:
        case MVT::i32:
          return AMDILCC::IL_CC_I_GE;
        case MVT::f32:
          return AMDILCC::IL_CC_F_GE;
        case MVT::f64:
          return AMDILCC::IL_CC_D_GE;
        case MVT::i64:
          return AMDILCC::IL_CC_L_GE;
        default:
          assert(0 && "Opcode combination not generated correctly!");
          return AMDILCC::COND_ERROR;
      };
    case ISD::SETLT:
      switch (type) {
        case MVT::i1:
        case MVT::i8:
        case MVT::i16:
        case MVT::i32:
          return AMDILCC::IL_CC_I_LT;
        case MVT::f32:
          return AMDILCC::IL_CC_F_LT;
        case MVT::f64:
          return AMDILCC::IL_CC_D_LT;
        case MVT::i64:
          return AMDILCC::IL_CC_L_LT;
        default:
          assert(0 && "Opcode combination not generated correctly!");
          return AMDILCC::COND_ERROR;
      };
    case ISD::SETLE:
      switch (type) {
        case MVT::i1:
        case MVT::i8:
        case MVT::i16:
        case MVT::i32:
          return AMDILCC::IL_CC_I_LE;
        case MVT::f32:
          return AMDILCC::IL_CC_F_LE;
        case MVT::f64:
          return AMDILCC::IL_CC_D_LE;
        case MVT::i64:
          return AMDILCC::IL_CC_L_LE;
        default:
          assert(0 && "Opcode combination not generated correctly!");
          return AMDILCC::COND_ERROR;
      };
    case ISD::SETNE:
      switch (type) {
        case MVT::i1:
        case MVT::i8:
        case MVT::i16:
        case MVT::i32:
          return AMDILCC::IL_CC_I_NE;
        case MVT::f32:
          return AMDILCC::IL_CC_F_NE;
        case MVT::f64:
          return AMDILCC::IL_CC_D_NE;
        case MVT::i64:
          return AMDILCC::IL_CC_L_NE;
        default:
          assert(0 && "Opcode combination not generated correctly!");
          return AMDILCC::COND_ERROR;
      };
    case ISD::SETEQ:
      switch (type) {
        case MVT::i1:
        case MVT::i8:
        case MVT::i16:
        case MVT::i32:
          return AMDILCC::IL_CC_I_EQ;
        case MVT::f32:
          return AMDILCC::IL_CC_F_EQ;
        case MVT::f64:
          return AMDILCC::IL_CC_D_EQ;
        case MVT::i64:
          return AMDILCC::IL_CC_L_EQ;
        default:
          assert(0 && "Opcode combination not generated correctly!");
          return AMDILCC::COND_ERROR;
      };
    case ISD::SETUGT:
      switch (type) {
        case MVT::i1:
        case MVT::i8:
        case MVT::i16:
        case MVT::i32:
          return AMDILCC::IL_CC_U_GT;
        case MVT::f32:
          return AMDILCC::IL_CC_F_UGT;
        case MVT::f64:
          return AMDILCC::IL_CC_D_UGT;
        case MVT::i64:
          return AMDILCC::IL_CC_UL_GT;
        default:
          assert(0 && "Opcode combination not generated correctly!");
          return AMDILCC::COND_ERROR;
      };
    case ISD::SETUGE:
      switch (type) {
        case MVT::i1:
        case MVT::i8:
        case MVT::i16:
        case MVT::i32:
          return AMDILCC::IL_CC_U_GE;
        case MVT::f32:
          return AMDILCC::IL_CC_F_UGE;
        case MVT::f64:
          return AMDILCC::IL_CC_D_UGE;
        case MVT::i64:
          return AMDILCC::IL_CC_UL_GE;
        default:
          assert(0 && "Opcode combination not generated correctly!");
          return AMDILCC::COND_ERROR;
      };
    case ISD::SETULT:
      switch (type) {
        case MVT::i1:
        case MVT::i8:
        case MVT::i16:
        case MVT::i32:
          return AMDILCC::IL_CC_U_LT;
        case MVT::f32:
          return AMDILCC::IL_CC_F_ULT;
        case MVT::f64:
          return AMDILCC::IL_CC_D_ULT;
        case MVT::i64:
          return AMDILCC::IL_CC_UL_LT;
        default:
          assert(0 && "Opcode combination not generated correctly!");
          return AMDILCC::COND_ERROR;
      };
    case ISD::SETULE:
      switch (type) {
        case MVT::i1:
        case MVT::i8:
        case MVT::i16:
        case MVT::i32:
          return AMDILCC::IL_CC_U_LE;
        case MVT::f32:
          return AMDILCC::IL_CC_F_ULE;
        case MVT::f64:
          return AMDILCC::IL_CC_D_ULE;
        case MVT::i64:
          return AMDILCC::IL_CC_UL_LE;
        default:
          assert(0 && "Opcode combination not generated correctly!");
          return AMDILCC::COND_ERROR;
      };
    case ISD::SETUNE:
      switch (type) {
        case MVT::i1:
        case MVT::i8:
        case MVT::i16:
        case MVT::i32:
          return AMDILCC::IL_CC_U_NE;
        case MVT::f32:
          return AMDILCC::IL_CC_F_UNE;
        case MVT::f64:
          return AMDILCC::IL_CC_D_UNE;
        case MVT::i64:
          return AMDILCC::IL_CC_UL_NE;
        default:
          assert(0 && "Opcode combination not generated correctly!");
          return AMDILCC::COND_ERROR;
      };
    case ISD::SETUEQ:
      switch (type) {
        case MVT::i1:
        case MVT::i8:
        case MVT::i16:
        case MVT::i32:
          return AMDILCC::IL_CC_U_EQ;
        case MVT::f32:
          return AMDILCC::IL_CC_F_UEQ;
        case MVT::f64:
          return AMDILCC::IL_CC_D_UEQ;
        case MVT::i64:
          return AMDILCC::IL_CC_UL_EQ;
        default:
          assert(0 && "Opcode combination not generated correctly!");
          return AMDILCC::COND_ERROR;
      };
    case ISD::SETOGT:
      switch (type) {
        case MVT::f32:
          return AMDILCC::IL_CC_F_OGT;
        case MVT::f64:
          return AMDILCC::IL_CC_D_OGT;
        case MVT::i1:
        case MVT::i8:
        case MVT::i16:
        case MVT::i32:
        case MVT::i64:
        default:
          assert(0 && "Opcode combination not generated correctly!");
          return AMDILCC::COND_ERROR;
      };
    case ISD::SETOGE:
      switch (type) {
        case MVT::f32:
          return AMDILCC::IL_CC_F_OGE;
        case MVT::f64:
          return AMDILCC::IL_CC_D_OGE;
        case MVT::i1:
        case MVT::i8:
        case MVT::i16:
        case MVT::i32:
        case MVT::i64:
        default:
          assert(0 && "Opcode combination not generated correctly!");
          return AMDILCC::COND_ERROR;
      };
    case ISD::SETOLT:
      switch (type) {
        case MVT::f32:
          return AMDILCC::IL_CC_F_OLT;
        case MVT::f64:
          return AMDILCC::IL_CC_D_OLT;
        case MVT::i1:
        case MVT::i8:
        case MVT::i16:
        case MVT::i32:
        case MVT::i64:
        default:
          assert(0 && "Opcode combination not generated correctly!");
          return AMDILCC::COND_ERROR;
      };
    case ISD::SETOLE:
      switch (type) {
        case MVT::f32:
          return AMDILCC::IL_CC_F_OLE;
        case MVT::f64:
          return AMDILCC::IL_CC_D_OLE;
        case MVT::i1:
        case MVT::i8:
        case MVT::i16:
        case MVT::i32:
        case MVT::i64:
        default:
          assert(0 && "Opcode combination not generated correctly!");
          return AMDILCC::COND_ERROR;
      };
    case ISD::SETONE:
      switch (type) {
        case MVT::f32:
          return AMDILCC::IL_CC_F_ONE;
        case MVT::f64:
          return AMDILCC::IL_CC_D_ONE;
        case MVT::i1:
        case MVT::i8:
        case MVT::i16:
        case MVT::i32:
        case MVT::i64:
        default:
          assert(0 && "Opcode combination not generated correctly!");
          return AMDILCC::COND_ERROR;
      };
    case ISD::SETOEQ:
      switch (type) {
        case MVT::f32:
          return AMDILCC::IL_CC_F_OEQ;
        case MVT::f64:
          return AMDILCC::IL_CC_D_OEQ;
        case MVT::i1:
        case MVT::i8:
        case MVT::i16:
        case MVT::i32:
        case MVT::i64:
        default:
          assert(0 && "Opcode combination not generated correctly!");
          return AMDILCC::COND_ERROR;
      };
  };
}

/// Helper function used by LowerFormalArguments
static const TargetRegisterClass*
getRegClassFromType(unsigned int type) {
  switch (type) {
  default:
    assert(0 && "Passed in type does not match any register classes.");
  case MVT::i8:
    return &AMDIL::GPRI8RegClass;
  case MVT::i16:
    return &AMDIL::GPRI16RegClass;
  case MVT::i32:
    return &AMDIL::GPRI32RegClass;
  case MVT::f32:
    return &AMDIL::GPRF32RegClass;
  case MVT::i64:
    return &AMDIL::GPRI64RegClass;
  case MVT::f64:
    return &AMDIL::GPRF64RegClass;
  case MVT::v4f32:
    return &AMDIL::GPRV4F32RegClass;
  case MVT::v4i8:
    return &AMDIL::GPRV4I8RegClass;
  case MVT::v4i16:
    return &AMDIL::GPRV4I16RegClass;
  case MVT::v4i32:
    return &AMDIL::GPRV4I32RegClass;
  case MVT::v2f32:
    return &AMDIL::GPRV2F32RegClass;
  case MVT::v2i8:
    return &AMDIL::GPRV2I8RegClass;
  case MVT::v2i16:
    return &AMDIL::GPRV2I16RegClass;
  case MVT::v2i32:
    return &AMDIL::GPRV2I32RegClass;
  case MVT::v2f64:
    return &AMDIL::GPRV2F64RegClass;
  case MVT::v2i64:
    return &AMDIL::GPRV2I64RegClass;
  }
}

SDValue
AMDILTargetLowering::LowerMemArgument(
    SDValue Chain,
    CallingConv::ID CallConv,
    const SmallVectorImpl<ISD::InputArg> &Ins,
    DebugLoc dl, SelectionDAG &DAG,
    const CCValAssign &VA,
    MachineFrameInfo *MFI,
    unsigned i) const
{
  // Create the nodes corresponding to a load from this parameter slot.
  ISD::ArgFlagsTy Flags = Ins[i].Flags;

  bool AlwaysUseMutable = (CallConv==CallingConv::Fast) &&
    getTargetMachine().Options.GuaranteedTailCallOpt;
  bool isImmutable = !AlwaysUseMutable && !Flags.isByVal();

  // FIXME: For now, all byval parameter objects are marked mutable. This can
  // be changed with more analysis.
  // In case of tail call optimization mark all arguments mutable. Since they
  // could be overwritten by lowering of arguments in case of a tail call.
  int FI = MFI->CreateFixedObject(VA.getValVT().getSizeInBits()/8,
      VA.getLocMemOffset(), isImmutable);
  SDValue FIN = DAG.getFrameIndex(FI, getPointerTy());

  if (Flags.isByVal())
    return FIN;
  return DAG.getLoad(VA.getValVT(), dl, Chain, FIN,
      MachinePointerInfo::getFixedStack(FI),
      false, false, false, 0);
}
//===----------------------------------------------------------------------===//
// TargetLowering Implementation Help Functions End
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//
// Instruction generation functions
//===----------------------------------------------------------------------===//
MachineOperand
AMDILTargetLowering::convertToReg(MachineOperand op) const
{
  if (op.isReg()) {
    return op;
  } else if (op.isImm()) {
    uint32_t loadReg
      = genVReg(op.getParent()->getDesc().OpInfo[0].RegClass);
    generateMachineInst(AMDIL::LOADCONST_i32, loadReg)
      .addImm(op.getImm());
    op.ChangeToRegister(loadReg, false);
  } else if (op.isFPImm()) {
    uint32_t loadReg
      = genVReg(op.getParent()->getDesc().OpInfo[0].RegClass);
    generateMachineInst(AMDIL::LOADCONST_f32, loadReg)
      .addFPImm(op.getFPImm());
    op.ChangeToRegister(loadReg, false);
  } else if (op.isMBB()) {
    op.ChangeToRegister(0, false);
  } else if (op.isFI()) {
    op.ChangeToRegister(0, false);
  } else if (op.isCPI()) {
    op.ChangeToRegister(0, false);
  } else if (op.isJTI()) {
    op.ChangeToRegister(0, false);
  } else if (op.isGlobal()) {
    op.ChangeToRegister(0, false);
  } else if (op.isSymbol()) {
    op.ChangeToRegister(0, false);
  }/* else if (op.isMetadata()) {
      op.ChangeToRegister(0, false);
      }*/
  return op;
}

//===----------------------------------------------------------------------===//
// TargetLowering Class Implementation Begins
//===----------------------------------------------------------------------===//
  AMDILTargetLowering::AMDILTargetLowering(TargetMachine &TM)
: TargetLowering(TM, new TargetLoweringObjectFileELF())
{
  int types[] =
  {
    (int)MVT::i8,
    (int)MVT::i16,
    (int)MVT::i32,
    (int)MVT::f32,
    (int)MVT::f64,
    (int)MVT::i64,
    (int)MVT::v2i8,
    (int)MVT::v4i8,
    (int)MVT::v2i16,
    (int)MVT::v4i16,
    (int)MVT::v4f32,
    (int)MVT::v4i32,
    (int)MVT::v2f32,
    (int)MVT::v2i32,
    (int)MVT::v2f64,
    (int)MVT::v2i64
  };

  int IntTypes[] =
  {
    (int)MVT::i8,
    (int)MVT::i16,
    (int)MVT::i32,
    (int)MVT::i64
  };

  int FloatTypes[] =
  {
    (int)MVT::f32,
    (int)MVT::f64
  };

  int VectorTypes[] =
  {
    (int)MVT::v2i8,
    (int)MVT::v4i8,
    (int)MVT::v2i16,
    (int)MVT::v4i16,
    (int)MVT::v4f32,
    (int)MVT::v4i32,
    (int)MVT::v2f32,
    (int)MVT::v2i32,
    (int)MVT::v2f64,
    (int)MVT::v2i64
  };
  size_t numTypes = sizeof(types) / sizeof(*types);
  size_t numFloatTypes = sizeof(FloatTypes) / sizeof(*FloatTypes);
  size_t numIntTypes = sizeof(IntTypes) / sizeof(*IntTypes);
  size_t numVectorTypes = sizeof(VectorTypes) / sizeof(*VectorTypes);

  const AMDILSubtarget *stm = reinterpret_cast<const AMDILTargetMachine*>(
      &this->getTargetMachine())->getSubtargetImpl();
  // These are the current register classes that are
  // supported

  addRegisterClass(MVT::i32, AMDIL::GPRI32RegisterClass);
  addRegisterClass(MVT::f32, AMDIL::GPRF32RegisterClass);

  if (stm->device()->isSupported(AMDILDeviceInfo::DoubleOps)) {
    addRegisterClass(MVT::f64, AMDIL::GPRF64RegisterClass);
    addRegisterClass(MVT::v2f64, AMDIL::GPRV2F64RegisterClass);
  }
  if (stm->device()->isSupported(AMDILDeviceInfo::ByteOps)) {
    addRegisterClass(MVT::i8, AMDIL::GPRI8RegisterClass);
    addRegisterClass(MVT::v2i8, AMDIL::GPRV2I8RegisterClass);
    addRegisterClass(MVT::v4i8, AMDIL::GPRV4I8RegisterClass);
    setOperationAction(ISD::Constant          , MVT::i8   , Legal);
  }
  if (stm->device()->isSupported(AMDILDeviceInfo::ShortOps)) {
    addRegisterClass(MVT::i16, AMDIL::GPRI16RegisterClass);
    addRegisterClass(MVT::v2i16, AMDIL::GPRV2I16RegisterClass);
    addRegisterClass(MVT::v4i16, AMDIL::GPRV4I16RegisterClass);
    setOperationAction(ISD::Constant          , MVT::i16  , Legal);
  }
  addRegisterClass(MVT::v2f32, AMDIL::GPRV2F32RegisterClass);
  addRegisterClass(MVT::v4f32, AMDIL::GPRV4F32RegisterClass);
  addRegisterClass(MVT::v2i32, AMDIL::GPRV2I32RegisterClass);
  addRegisterClass(MVT::v4i32, AMDIL::GPRV4I32RegisterClass);
  if (stm->device()->isSupported(AMDILDeviceInfo::LongOps)) {
    addRegisterClass(MVT::i64, AMDIL::GPRI64RegisterClass);
    addRegisterClass(MVT::v2i64, AMDIL::GPRV2I64RegisterClass);
  }

  for (unsigned int x  = 0; x < numTypes; ++x) {
    MVT::SimpleValueType VT = (MVT::SimpleValueType)types[x];

    //FIXME: SIGN_EXTEND_INREG is not meaningful for floating point types
    // We cannot sextinreg, expand to shifts
    setOperationAction(ISD::SIGN_EXTEND_INREG, VT, Custom);
    setOperationAction(ISD::EXTRACT_SUBVECTOR, VT, Custom);
    setOperationAction(ISD::FP_ROUND, VT, Expand);
    setOperationAction(ISD::SUBE, VT, Expand);
    setOperationAction(ISD::SUBC, VT, Expand);
    setOperationAction(ISD::ADDE, VT, Expand);
    setOperationAction(ISD::ADDC, VT, Expand);
    setOperationAction(ISD::SETCC, VT, Custom);
    setOperationAction(ISD::BRCOND, VT, Custom);
    setOperationAction(ISD::BR_CC, VT, Custom);
    setOperationAction(ISD::BR_JT, VT, Expand);
    setOperationAction(ISD::BRIND, VT, Expand);
    // TODO: Implement custom UREM/SREM routines
    setOperationAction(ISD::UREM, VT, Expand);
    setOperationAction(ISD::SREM, VT, Expand);
    setOperationAction(ISD::UINT_TO_FP, VT, Custom);
    setOperationAction(ISD::FP_TO_UINT, VT, Custom);
    setOperationAction(ISD::GlobalAddress, VT, Custom);
    setOperationAction(ISD::JumpTable, VT, Custom);
    setOperationAction(ISD::ConstantPool, VT, Custom);
    setOperationAction(ISD::SELECT_CC, VT, Custom);
    setOperationAction(ISD::SELECT, VT, Custom);
    setOperationAction(ISD::SMUL_LOHI, VT, Expand);
    setOperationAction(ISD::UMUL_LOHI, VT, Expand);
    if (VT != MVT::i64 && VT != MVT::v2i64) {
      setOperationAction(ISD::SDIV, VT, Custom);
    }
    setOperationAction(ISD::INSERT_VECTOR_ELT, VT, Custom);
    setOperationAction(ISD::EXTRACT_VECTOR_ELT, VT, Custom);
  }
  for (unsigned int x = 0; x < numFloatTypes; ++x) {
    MVT::SimpleValueType VT = (MVT::SimpleValueType)FloatTypes[x];

    // IL does not have these operations for floating point types
    setOperationAction(ISD::FP_ROUND_INREG, VT, Expand);
    setOperationAction(ISD::FP_ROUND, VT, Custom);
    setOperationAction(ISD::SETOLT, VT, Expand);
    setOperationAction(ISD::SETOGE, VT, Expand);
    setOperationAction(ISD::SETOGT, VT, Expand);
    setOperationAction(ISD::SETOLE, VT, Expand);
    setOperationAction(ISD::SETULT, VT, Expand);
    setOperationAction(ISD::SETUGE, VT, Expand);
    setOperationAction(ISD::SETUGT, VT, Expand);
    setOperationAction(ISD::SETULE, VT, Expand);
  }

  for (unsigned int x = 0; x < numIntTypes; ++x) {
    MVT::SimpleValueType VT = (MVT::SimpleValueType)IntTypes[x];

    // GPU also does not have divrem function for signed or unsigned
    setOperationAction(ISD::SDIVREM, VT, Expand);
    setOperationAction(ISD::UDIVREM, VT, Expand);
    setOperationAction(ISD::FP_ROUND, VT, Expand);

    // GPU does not have [S|U]MUL_LOHI functions as a single instruction
    setOperationAction(ISD::SMUL_LOHI, VT, Expand);
    setOperationAction(ISD::UMUL_LOHI, VT, Expand);

    // GPU doesn't have a rotl, rotr, or byteswap instruction
    setOperationAction(ISD::ROTR, VT, Expand);
    setOperationAction(ISD::ROTL, VT, Expand);
    setOperationAction(ISD::BSWAP, VT, Expand);

    // GPU doesn't have any counting operators
    setOperationAction(ISD::CTPOP, VT, Expand);
    setOperationAction(ISD::CTTZ, VT, Expand);
    setOperationAction(ISD::CTLZ, VT, Expand);
  }

  for ( unsigned int ii = 0; ii < numVectorTypes; ++ii )
  {
    MVT::SimpleValueType VT = (MVT::SimpleValueType)VectorTypes[ii];

    setOperationAction(ISD::BUILD_VECTOR, VT, Custom);
    setOperationAction(ISD::EXTRACT_SUBVECTOR, VT, Custom);
    setOperationAction(ISD::SCALAR_TO_VECTOR, VT, Custom);
    setOperationAction(ISD::VECTOR_SHUFFLE, VT, Expand);
    setOperationAction(ISD::CONCAT_VECTORS, VT, Custom);
    setOperationAction(ISD::FP_ROUND, VT, Expand);
    setOperationAction(ISD::SDIVREM, VT, Expand);
    setOperationAction(ISD::UDIVREM, VT, Expand);
    setOperationAction(ISD::SMUL_LOHI, VT, Expand);
    // setOperationAction(ISD::VSETCC, VT, Expand);
    setOperationAction(ISD::SETCC, VT, Expand);
    setOperationAction(ISD::SELECT_CC, VT, Expand);
    setOperationAction(ISD::SELECT, VT, Expand);

  }
  setOperationAction(ISD::FP_ROUND, MVT::Other, Expand);
  if (stm->device()->isSupported(AMDILDeviceInfo::LongOps)) {
    if (stm->calVersion() < CAL_VERSION_SC_139
        || stm->device()->getGeneration() == AMDILDeviceInfo::HD4XXX) {
      setOperationAction(ISD::MUL, MVT::i64, Custom);
    }
    setOperationAction(ISD::SUB, MVT::i64, Custom);
    setOperationAction(ISD::ADD, MVT::i64, Custom);
    setOperationAction(ISD::MULHU, MVT::i64, Expand);
    setOperationAction(ISD::MULHU, MVT::v2i64, Expand);
    setOperationAction(ISD::MULHS, MVT::i64, Expand);
    setOperationAction(ISD::MULHS, MVT::v2i64, Expand);
    setOperationAction(ISD::MUL, MVT::v2i64, Expand);
    setOperationAction(ISD::SUB, MVT::v2i64, Expand);
    setOperationAction(ISD::ADD, MVT::v2i64, Expand);
    setOperationAction(ISD::SREM, MVT::v2i64, Expand);
    setOperationAction(ISD::Constant          , MVT::i64  , Legal);
    setOperationAction(ISD::SDIV, MVT::v2i64, Expand);
    setOperationAction(ISD::UINT_TO_FP, MVT::v2i64, Expand);
    setOperationAction(ISD::FP_TO_UINT, MVT::v2i64, Expand);
    setOperationAction(ISD::TRUNCATE, MVT::v2i64, Expand);
    setOperationAction(ISD::SIGN_EXTEND, MVT::v2i64, Expand);
    setOperationAction(ISD::ZERO_EXTEND, MVT::v2i64, Expand);
    setOperationAction(ISD::ANY_EXTEND, MVT::v2i64, Expand);
  }
  if (stm->device()->isSupported(AMDILDeviceInfo::DoubleOps)) {
    // we support loading/storing v2f64 but not operations on the type
    setOperationAction(ISD::FADD, MVT::v2f64, Expand);
    setOperationAction(ISD::FSUB, MVT::v2f64, Expand);
    setOperationAction(ISD::FMUL, MVT::v2f64, Expand);
    setOperationAction(ISD::FP_ROUND, MVT::v2f64, Expand);
    setOperationAction(ISD::FP_ROUND_INREG, MVT::v2f64, Expand);
    setOperationAction(ISD::FP_EXTEND, MVT::v2f64, Expand);
    setOperationAction(ISD::ConstantFP        , MVT::f64  , Legal);
    setOperationAction(ISD::FDIV, MVT::v2f64, Expand);
    // We want to expand vector conversions into their scalar
    // counterparts.
    setOperationAction(ISD::UINT_TO_FP, MVT::v2f64, Expand);
    setOperationAction(ISD::FP_TO_UINT, MVT::v2f64, Expand);
    setOperationAction(ISD::TRUNCATE, MVT::v2f64, Expand);
    setOperationAction(ISD::SIGN_EXTEND, MVT::v2f64, Expand);
    setOperationAction(ISD::ZERO_EXTEND, MVT::v2f64, Expand);
    setOperationAction(ISD::ANY_EXTEND, MVT::v2f64, Expand);
    setOperationAction(ISD::FABS, MVT::f64, Expand);
    setOperationAction(ISD::FABS, MVT::v2f64, Expand);
  }
  // TODO: Fix the UDIV24 algorithm so it works for these
  // types correctly. This needs vector comparisons
  // for this to work correctly.
  setOperationAction(ISD::UDIV, MVT::v2i8, Expand);
  setOperationAction(ISD::UDIV, MVT::v4i8, Expand);
  setOperationAction(ISD::UDIV, MVT::v2i16, Expand);
  setOperationAction(ISD::UDIV, MVT::v4i16, Expand);
  setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i1, Custom);
  setOperationAction(ISD::SUBC, MVT::Other, Expand);
  setOperationAction(ISD::ADDE, MVT::Other, Expand);
  setOperationAction(ISD::ADDC, MVT::Other, Expand);
  setOperationAction(ISD::BRCOND, MVT::Other, Custom);
  setOperationAction(ISD::BR_CC, MVT::Other, Custom);
  setOperationAction(ISD::BR_JT, MVT::Other, Expand);
  setOperationAction(ISD::BRIND, MVT::Other, Expand);
  setOperationAction(ISD::SETCC, MVT::Other, Custom);
  setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::Other, Expand);
  setOperationAction(ISD::FDIV, MVT::f32, Custom);
  setOperationAction(ISD::FDIV, MVT::v2f32, Custom);
  setOperationAction(ISD::FDIV, MVT::v4f32, Custom);

  setOperationAction(ISD::BUILD_VECTOR, MVT::Other, Custom);
  // Use the default implementation.
  setOperationAction(ISD::VAARG             , MVT::Other, Expand);
  setOperationAction(ISD::VACOPY            , MVT::Other, Expand);
  setOperationAction(ISD::VAEND             , MVT::Other, Expand);
  setOperationAction(ISD::STACKSAVE         , MVT::Other, Expand);
  setOperationAction(ISD::STACKRESTORE      , MVT::Other, Expand);
  setOperationAction(ISD::DYNAMIC_STACKALLOC, MVT::i32  , Custom);
  setOperationAction(ISD::ConstantFP        , MVT::f32    , Legal);
  setOperationAction(ISD::Constant          , MVT::i32    , Legal);
  setOperationAction(ISD::TRAP              , MVT::Other  , Legal);

  setStackPointerRegisterToSaveRestore(AMDIL::SP);
  setSchedulingPreference(Sched::RegPressure);
  setPow2DivIsCheap(false);
  setPrefLoopAlignment(16);
  setSelectIsExpensive(true);
  setJumpIsExpensive(true);
  computeRegisterProperties();

  maxStoresPerMemcpy  = 4096;
  maxStoresPerMemmove = 4096;
  maxStoresPerMemset  = 4096;

#undef numTypes
#undef numIntTypes
#undef numVectorTypes
#undef numFloatTypes
}

const char *
AMDILTargetLowering::getTargetNodeName(unsigned Opcode) const
{
  switch (Opcode) {
    default: return 0;
    case AMDILISD::INTTOANY: return "AMDILISD::INTTOANY";
    case AMDILISD::DP_TO_FP:  return "AMDILISD::DP_TO_FP";
    case AMDILISD::FP_TO_DP:  return "AMDILISD::FP_TO_DP";
    case AMDILISD::BITCONV: return "AMDILISD::BITCONV";
    case AMDILISD::CMOV:  return "AMDILISD::CMOV";
    case AMDILISD::CMOVLOG:  return "AMDILISD::CMOVLOG";
    case AMDILISD::INEGATE:  return "AMDILISD::INEGATE";
    case AMDILISD::MAD:  return "AMDILISD::MAD";
    case AMDILISD::UMAD:  return "AMDILISD::UMAD";
    case AMDILISD::CALL:  return "AMDILISD::CALL";
    case AMDILISD::RET:   return "AMDILISD::RET";
    case AMDILISD::IFFB_HI: return "AMDILISD::IFFB_HI";
    case AMDILISD::IFFB_LO: return "AMDILISD::IFFB_LO";
    case AMDILISD::ADD: return "AMDILISD::ADD";
    case AMDILISD::UMUL: return "AMDILISD::UMUL";
    case AMDILISD::AND: return "AMDILISD::AND";
    case AMDILISD::OR: return "AMDILISD::OR";
    case AMDILISD::NOT: return "AMDILISD::NOT";
    case AMDILISD::XOR: return "AMDILISD::XOR";
    case AMDILISD::DIV_INF: return "AMDILISD::DIV_INF";
    case AMDILISD::SMAX: return "AMDILISD::SMAX";
    case AMDILISD::PHIMOVE: return "AMDILISD::PHIMOVE";
    case AMDILISD::MOVE: return "AMDILISD::MOVE";
    case AMDILISD::VBUILD: return "AMDILISD::VBUILD";
    case AMDILISD::VEXTRACT: return "AMDILISD::VEXTRACT";
    case AMDILISD::VINSERT: return "AMDILISD::VINSERT";
    case AMDILISD::VCONCAT: return "AMDILISD::VCONCAT";
    case AMDILISD::LCREATE: return "AMDILISD::LCREATE";
    case AMDILISD::LCOMPHI: return "AMDILISD::LCOMPHI";
    case AMDILISD::LCOMPLO: return "AMDILISD::LCOMPLO";
    case AMDILISD::DCREATE: return "AMDILISD::DCREATE";
    case AMDILISD::DCOMPHI: return "AMDILISD::DCOMPHI";
    case AMDILISD::DCOMPLO: return "AMDILISD::DCOMPLO";
    case AMDILISD::LCREATE2: return "AMDILISD::LCREATE2";
    case AMDILISD::LCOMPHI2: return "AMDILISD::LCOMPHI2";
    case AMDILISD::LCOMPLO2: return "AMDILISD::LCOMPLO2";
    case AMDILISD::DCREATE2: return "AMDILISD::DCREATE2";
    case AMDILISD::DCOMPHI2: return "AMDILISD::DCOMPHI2";
    case AMDILISD::DCOMPLO2: return "AMDILISD::DCOMPLO2";
    case AMDILISD::CMP: return "AMDILISD::CMP";
    case AMDILISD::IL_CC_I_LT: return "AMDILISD::IL_CC_I_LT";
    case AMDILISD::IL_CC_I_LE: return "AMDILISD::IL_CC_I_LE";
    case AMDILISD::IL_CC_I_GT: return "AMDILISD::IL_CC_I_GT";
    case AMDILISD::IL_CC_I_GE: return "AMDILISD::IL_CC_I_GE";
    case AMDILISD::IL_CC_I_EQ: return "AMDILISD::IL_CC_I_EQ";
    case AMDILISD::IL_CC_I_NE: return "AMDILISD::IL_CC_I_NE";
    case AMDILISD::RET_FLAG: return "AMDILISD::RET_FLAG";
    case AMDILISD::BRANCH_COND: return "AMDILISD::BRANCH_COND";
    case AMDILISD::LOOP_NZERO: return "AMDILISD::LOOP_NZERO";
    case AMDILISD::LOOP_ZERO: return "AMDILISD::LOOP_ZERO";
    case AMDILISD::LOOP_CMP: return "AMDILISD::LOOP_CMP";
    case AMDILISD::ADDADDR: return "AMDILISD::ADDADDR";
    case AMDILISD::ATOM_G_ADD: return "AMDILISD::ATOM_G_ADD";
    case AMDILISD::ATOM_G_AND: return "AMDILISD::ATOM_G_AND";
    case AMDILISD::ATOM_G_CMPXCHG: return "AMDILISD::ATOM_G_CMPXCHG";
    case AMDILISD::ATOM_G_DEC: return "AMDILISD::ATOM_G_DEC";
    case AMDILISD::ATOM_G_INC: return "AMDILISD::ATOM_G_INC";
    case AMDILISD::ATOM_G_MAX: return "AMDILISD::ATOM_G_MAX";
    case AMDILISD::ATOM_G_UMAX: return "AMDILISD::ATOM_G_UMAX";
    case AMDILISD::ATOM_G_MIN: return "AMDILISD::ATOM_G_MIN";
    case AMDILISD::ATOM_G_UMIN: return "AMDILISD::ATOM_G_UMIN";
    case AMDILISD::ATOM_G_OR: return "AMDILISD::ATOM_G_OR";
    case AMDILISD::ATOM_G_SUB: return "AMDILISD::ATOM_G_SUB";
    case AMDILISD::ATOM_G_RSUB: return "AMDILISD::ATOM_G_RSUB";
    case AMDILISD::ATOM_G_XCHG: return "AMDILISD::ATOM_G_XCHG";
    case AMDILISD::ATOM_G_XOR: return "AMDILISD::ATOM_G_XOR";
    case AMDILISD::ATOM_G_ADD_NORET: return "AMDILISD::ATOM_G_ADD_NORET";
    case AMDILISD::ATOM_G_AND_NORET: return "AMDILISD::ATOM_G_AND_NORET";
    case AMDILISD::ATOM_G_CMPXCHG_NORET: return "AMDILISD::ATOM_G_CMPXCHG_NORET";
    case AMDILISD::ATOM_G_DEC_NORET: return "AMDILISD::ATOM_G_DEC_NORET";
    case AMDILISD::ATOM_G_INC_NORET: return "AMDILISD::ATOM_G_INC_NORET";
    case AMDILISD::ATOM_G_MAX_NORET: return "AMDILISD::ATOM_G_MAX_NORET";
    case AMDILISD::ATOM_G_UMAX_NORET: return "AMDILISD::ATOM_G_UMAX_NORET";
    case AMDILISD::ATOM_G_MIN_NORET: return "AMDILISD::ATOM_G_MIN_NORET";
    case AMDILISD::ATOM_G_UMIN_NORET: return "AMDILISD::ATOM_G_UMIN_NORET";
    case AMDILISD::ATOM_G_OR_NORET: return "AMDILISD::ATOM_G_OR_NORET";
    case AMDILISD::ATOM_G_SUB_NORET: return "AMDILISD::ATOM_G_SUB_NORET";
    case AMDILISD::ATOM_G_RSUB_NORET: return "AMDILISD::ATOM_G_RSUB_NORET";
    case AMDILISD::ATOM_G_XCHG_NORET: return "AMDILISD::ATOM_G_XCHG_NORET";
    case AMDILISD::ATOM_G_XOR_NORET: return "AMDILISD::ATOM_G_XOR_NORET";
    case AMDILISD::ATOM_L_ADD: return "AMDILISD::ATOM_L_ADD";
    case AMDILISD::ATOM_L_AND: return "AMDILISD::ATOM_L_AND";
    case AMDILISD::ATOM_L_CMPXCHG: return "AMDILISD::ATOM_L_CMPXCHG";
    case AMDILISD::ATOM_L_DEC: return "AMDILISD::ATOM_L_DEC";
    case AMDILISD::ATOM_L_INC: return "AMDILISD::ATOM_L_INC";
    case AMDILISD::ATOM_L_MAX: return "AMDILISD::ATOM_L_MAX";
    case AMDILISD::ATOM_L_UMAX: return "AMDILISD::ATOM_L_UMAX";
    case AMDILISD::ATOM_L_MIN: return "AMDILISD::ATOM_L_MIN";
    case AMDILISD::ATOM_L_UMIN: return "AMDILISD::ATOM_L_UMIN";
    case AMDILISD::ATOM_L_OR: return "AMDILISD::ATOM_L_OR";
    case AMDILISD::ATOM_L_SUB: return "AMDILISD::ATOM_L_SUB";
    case AMDILISD::ATOM_L_RSUB: return "AMDILISD::ATOM_L_RSUB";
    case AMDILISD::ATOM_L_XCHG: return "AMDILISD::ATOM_L_XCHG";
    case AMDILISD::ATOM_L_XOR: return "AMDILISD::ATOM_L_XOR";
    case AMDILISD::ATOM_L_ADD_NORET: return "AMDILISD::ATOM_L_ADD_NORET";
    case AMDILISD::ATOM_L_AND_NORET: return "AMDILISD::ATOM_L_AND_NORET";
    case AMDILISD::ATOM_L_CMPXCHG_NORET: return "AMDILISD::ATOM_L_CMPXCHG_NORET";
    case AMDILISD::ATOM_L_DEC_NORET: return "AMDILISD::ATOM_L_DEC_NORET";
    case AMDILISD::ATOM_L_INC_NORET: return "AMDILISD::ATOM_L_INC_NORET";
    case AMDILISD::ATOM_L_MAX_NORET: return "AMDILISD::ATOM_L_MAX_NORET";
    case AMDILISD::ATOM_L_UMAX_NORET: return "AMDILISD::ATOM_L_UMAX_NORET";
    case AMDILISD::ATOM_L_MIN_NORET: return "AMDILISD::ATOM_L_MIN_NORET";
    case AMDILISD::ATOM_L_UMIN_NORET: return "AMDILISD::ATOM_L_UMIN_NORET";
    case AMDILISD::ATOM_L_OR_NORET: return "AMDILISD::ATOM_L_OR_NORET";
    case AMDILISD::ATOM_L_SUB_NORET: return "AMDILISD::ATOM_L_SUB_NORET";
    case AMDILISD::ATOM_L_RSUB_NORET: return "AMDILISD::ATOM_L_RSUB_NORET";
    case AMDILISD::ATOM_L_XCHG_NORET: return "AMDILISD::ATOM_L_XCHG_NORET";
    case AMDILISD::ATOM_R_ADD: return "AMDILISD::ATOM_R_ADD";
    case AMDILISD::ATOM_R_AND: return "AMDILISD::ATOM_R_AND";
    case AMDILISD::ATOM_R_CMPXCHG: return "AMDILISD::ATOM_R_CMPXCHG";
    case AMDILISD::ATOM_R_DEC: return "AMDILISD::ATOM_R_DEC";
    case AMDILISD::ATOM_R_INC: return "AMDILISD::ATOM_R_INC";
    case AMDILISD::ATOM_R_MAX: return "AMDILISD::ATOM_R_MAX";
    case AMDILISD::ATOM_R_UMAX: return "AMDILISD::ATOM_R_UMAX";
    case AMDILISD::ATOM_R_MIN: return "AMDILISD::ATOM_R_MIN";
    case AMDILISD::ATOM_R_UMIN: return "AMDILISD::ATOM_R_UMIN";
    case AMDILISD::ATOM_R_OR: return "AMDILISD::ATOM_R_OR";
    case AMDILISD::ATOM_R_MSKOR: return "AMDILISD::ATOM_R_MSKOR";
    case AMDILISD::ATOM_R_SUB: return "AMDILISD::ATOM_R_SUB";
    case AMDILISD::ATOM_R_RSUB: return "AMDILISD::ATOM_R_RSUB";
    case AMDILISD::ATOM_R_XCHG: return "AMDILISD::ATOM_R_XCHG";
    case AMDILISD::ATOM_R_XOR: return "AMDILISD::ATOM_R_XOR";
    case AMDILISD::ATOM_R_ADD_NORET: return "AMDILISD::ATOM_R_ADD_NORET";
    case AMDILISD::ATOM_R_AND_NORET: return "AMDILISD::ATOM_R_AND_NORET";
    case AMDILISD::ATOM_R_CMPXCHG_NORET: return "AMDILISD::ATOM_R_CMPXCHG_NORET";
    case AMDILISD::ATOM_R_DEC_NORET: return "AMDILISD::ATOM_R_DEC_NORET";
    case AMDILISD::ATOM_R_INC_NORET: return "AMDILISD::ATOM_R_INC_NORET";
    case AMDILISD::ATOM_R_MAX_NORET: return "AMDILISD::ATOM_R_MAX_NORET";
    case AMDILISD::ATOM_R_UMAX_NORET: return "AMDILISD::ATOM_R_UMAX_NORET";
    case AMDILISD::ATOM_R_MIN_NORET: return "AMDILISD::ATOM_R_MIN_NORET";
    case AMDILISD::ATOM_R_UMIN_NORET: return "AMDILISD::ATOM_R_UMIN_NORET";
    case AMDILISD::ATOM_R_OR_NORET: return "AMDILISD::ATOM_R_OR_NORET";
    case AMDILISD::ATOM_R_MSKOR_NORET: return "AMDILISD::ATOM_R_MSKOR_NORET";
    case AMDILISD::ATOM_R_SUB_NORET: return "AMDILISD::ATOM_R_SUB_NORET";
    case AMDILISD::ATOM_R_RSUB_NORET: return "AMDILISD::ATOM_R_RSUB_NORET";
    case AMDILISD::ATOM_R_XCHG_NORET: return "AMDILISD::ATOM_R_XCHG_NORET";
    case AMDILISD::ATOM_R_XOR_NORET: return "AMDILISD::ATOM_R_XOR_NORET";
    case AMDILISD::APPEND_ALLOC: return "AMDILISD::APPEND_ALLOC";
    case AMDILISD::APPEND_ALLOC_NORET: return "AMDILISD::APPEND_ALLOC_NORET";
    case AMDILISD::APPEND_CONSUME: return "AMDILISD::APPEND_CONSUME";
    case AMDILISD::APPEND_CONSUME_NORET: return "AMDILISD::APPEND_CONSUME_NORET";
    case AMDILISD::IMAGE2D_READ: return "AMDILISD::IMAGE2D_READ";
    case AMDILISD::IMAGE2D_WRITE: return "AMDILISD::IMAGE2D_WRITE";
    case AMDILISD::IMAGE2D_INFO0: return "AMDILISD::IMAGE2D_INFO0";
    case AMDILISD::IMAGE2D_INFO1: return "AMDILISD::IMAGE2D_INFO1";
    case AMDILISD::IMAGE3D_READ: return "AMDILISD::IMAGE3D_READ";
    case AMDILISD::IMAGE3D_WRITE: return "AMDILISD::IMAGE3D_WRITE";
    case AMDILISD::IMAGE3D_INFO0: return "AMDILISD::IMAGE3D_INFO0";
    case AMDILISD::IMAGE3D_INFO1: return "AMDILISD::IMAGE3D_INFO1";

  };
}
bool
AMDILTargetLowering::getTgtMemIntrinsic(IntrinsicInfo &Info,
    const CallInst &I, unsigned Intrinsic) const
{
  if (Intrinsic <= AMDGPUIntrinsic::last_non_AMDIL_intrinsic 
      || Intrinsic > AMDGPUIntrinsic::num_AMDIL_intrinsics) {
    return false;
  }
  bool bitCastToInt = false;
  unsigned IntNo;
  bool isRet = true;
  const AMDILSubtarget *STM = &this->getTargetMachine()
    .getSubtarget<AMDILSubtarget>();
  switch (Intrinsic) {
    default: return false; // Don't custom lower most intrinsics.
    case AMDGPUIntrinsic::AMDIL_atomic_add_gi32:
    case AMDGPUIntrinsic::AMDIL_atomic_add_gu32:
             IntNo = AMDILISD::ATOM_G_ADD; break;
    case AMDGPUIntrinsic::AMDIL_atomic_add_gi32_noret:
    case AMDGPUIntrinsic::AMDIL_atomic_add_gu32_noret:
             isRet = false;
             IntNo = AMDILISD::ATOM_G_ADD_NORET; break;
    case AMDGPUIntrinsic::AMDIL_atomic_add_lu32:
    case AMDGPUIntrinsic::AMDIL_atomic_add_li32:
             IntNo = AMDILISD::ATOM_L_ADD; break;
    case AMDGPUIntrinsic::AMDIL_atomic_add_li32_noret:
    case AMDGPUIntrinsic::AMDIL_atomic_add_lu32_noret:
             isRet = false;
             IntNo = AMDILISD::ATOM_L_ADD_NORET; break;
    case AMDGPUIntrinsic::AMDIL_atomic_add_ru32:
    case AMDGPUIntrinsic::AMDIL_atomic_add_ri32:
             IntNo = AMDILISD::ATOM_R_ADD; break;
    case AMDGPUIntrinsic::AMDIL_atomic_add_ri32_noret:
    case AMDGPUIntrinsic::AMDIL_atomic_add_ru32_noret:
             isRet = false;
             IntNo = AMDILISD::ATOM_R_ADD_NORET; break;
    case AMDGPUIntrinsic::AMDIL_atomic_and_gi32:
    case AMDGPUIntrinsic::AMDIL_atomic_and_gu32:
             IntNo = AMDILISD::ATOM_G_AND; break;
    case AMDGPUIntrinsic::AMDIL_atomic_and_gi32_noret:
    case AMDGPUIntrinsic::AMDIL_atomic_and_gu32_noret:
             isRet = false;
             IntNo = AMDILISD::ATOM_G_AND_NORET; break;
    case AMDGPUIntrinsic::AMDIL_atomic_and_li32:
    case AMDGPUIntrinsic::AMDIL_atomic_and_lu32:
             IntNo = AMDILISD::ATOM_L_AND; break;
    case AMDGPUIntrinsic::AMDIL_atomic_and_li32_noret:
    case AMDGPUIntrinsic::AMDIL_atomic_and_lu32_noret:
             isRet = false;
             IntNo = AMDILISD::ATOM_L_AND_NORET; break;
    case AMDGPUIntrinsic::AMDIL_atomic_and_ri32:
    case AMDGPUIntrinsic::AMDIL_atomic_and_ru32:
             IntNo = AMDILISD::ATOM_R_AND; break;
    case AMDGPUIntrinsic::AMDIL_atomic_and_ri32_noret:
    case AMDGPUIntrinsic::AMDIL_atomic_and_ru32_noret:
             isRet = false;
             IntNo = AMDILISD::ATOM_R_AND_NORET; break;
    case AMDGPUIntrinsic::AMDIL_atomic_cmpxchg_gi32:
    case AMDGPUIntrinsic::AMDIL_atomic_cmpxchg_gu32:
             IntNo = AMDILISD::ATOM_G_CMPXCHG; break;
    case AMDGPUIntrinsic::AMDIL_atomic_cmpxchg_gi32_noret:
    case AMDGPUIntrinsic::AMDIL_atomic_cmpxchg_gu32_noret:
             isRet = false;
             IntNo = AMDILISD::ATOM_G_CMPXCHG_NORET; break;
    case AMDGPUIntrinsic::AMDIL_atomic_cmpxchg_li32:
    case AMDGPUIntrinsic::AMDIL_atomic_cmpxchg_lu32:
             IntNo = AMDILISD::ATOM_L_CMPXCHG; break;
    case AMDGPUIntrinsic::AMDIL_atomic_cmpxchg_li32_noret:
    case AMDGPUIntrinsic::AMDIL_atomic_cmpxchg_lu32_noret:
             isRet = false;
             IntNo = AMDILISD::ATOM_L_CMPXCHG_NORET; break;
    case AMDGPUIntrinsic::AMDIL_atomic_cmpxchg_ri32:
    case AMDGPUIntrinsic::AMDIL_atomic_cmpxchg_ru32:
             IntNo = AMDILISD::ATOM_R_CMPXCHG; break;
    case AMDGPUIntrinsic::AMDIL_atomic_cmpxchg_ri32_noret:
    case AMDGPUIntrinsic::AMDIL_atomic_cmpxchg_ru32_noret:
             isRet = false;
             IntNo = AMDILISD::ATOM_R_CMPXCHG_NORET; break;
    case AMDGPUIntrinsic::AMDIL_atomic_dec_gi32:
    case AMDGPUIntrinsic::AMDIL_atomic_dec_gu32:
             if (STM->calVersion() >= CAL_VERSION_SC_136) {
               IntNo = AMDILISD::ATOM_G_DEC;
             } else {
               IntNo = AMDILISD::ATOM_G_SUB;
             }
             break;
    case AMDGPUIntrinsic::AMDIL_atomic_dec_gi32_noret:
    case AMDGPUIntrinsic::AMDIL_atomic_dec_gu32_noret:
             isRet = false;
             if (STM->calVersion() >= CAL_VERSION_SC_136) {
               IntNo = AMDILISD::ATOM_G_DEC_NORET;
             } else {
               IntNo = AMDILISD::ATOM_G_SUB_NORET; 
             }
             break;
    case AMDGPUIntrinsic::AMDIL_atomic_dec_li32:
    case AMDGPUIntrinsic::AMDIL_atomic_dec_lu32:
             if (STM->calVersion() >= CAL_VERSION_SC_136) {
               IntNo = AMDILISD::ATOM_L_DEC;
             } else {
               IntNo = AMDILISD::ATOM_L_SUB; 
             } 
             break;
    case AMDGPUIntrinsic::AMDIL_atomic_dec_li32_noret:
    case AMDGPUIntrinsic::AMDIL_atomic_dec_lu32_noret:
             isRet = false;
             if (STM->calVersion() >= CAL_VERSION_SC_136) {
               IntNo = AMDILISD::ATOM_L_DEC_NORET;
             } else {
               IntNo = AMDILISD::ATOM_L_SUB_NORET; 
             }
             break;
    case AMDGPUIntrinsic::AMDIL_atomic_dec_ri32:
    case AMDGPUIntrinsic::AMDIL_atomic_dec_ru32:
             if (STM->calVersion() >= CAL_VERSION_SC_136) {
               IntNo = AMDILISD::ATOM_R_DEC;
             } else {
               IntNo = AMDILISD::ATOM_R_SUB; 
             }
             break;
    case AMDGPUIntrinsic::AMDIL_atomic_dec_ri32_noret:
    case AMDGPUIntrinsic::AMDIL_atomic_dec_ru32_noret:
             isRet = false;
             if (STM->calVersion() >= CAL_VERSION_SC_136) {
               IntNo = AMDILISD::ATOM_R_DEC_NORET;
             } else {
               IntNo = AMDILISD::ATOM_R_SUB_NORET; 
             }
             break;
    case AMDGPUIntrinsic::AMDIL_atomic_inc_gi32:
    case AMDGPUIntrinsic::AMDIL_atomic_inc_gu32:
             if (STM->calVersion() >= CAL_VERSION_SC_136) {
               IntNo = AMDILISD::ATOM_G_INC;
             } else {
               IntNo = AMDILISD::ATOM_G_ADD; 
             }
             break;
    case AMDGPUIntrinsic::AMDIL_atomic_inc_gi32_noret:
    case AMDGPUIntrinsic::AMDIL_atomic_inc_gu32_noret:
             isRet = false;
             if (STM->calVersion() >= CAL_VERSION_SC_136) {
               IntNo = AMDILISD::ATOM_G_INC_NORET;
             } else {
               IntNo = AMDILISD::ATOM_G_ADD_NORET; 
             }
             break;
    case AMDGPUIntrinsic::AMDIL_atomic_inc_li32:
    case AMDGPUIntrinsic::AMDIL_atomic_inc_lu32:
             if (STM->calVersion() >= CAL_VERSION_SC_136) {
               IntNo = AMDILISD::ATOM_L_INC;
             } else {
               IntNo = AMDILISD::ATOM_L_ADD; 
             }
             break;
    case AMDGPUIntrinsic::AMDIL_atomic_inc_li32_noret:
    case AMDGPUIntrinsic::AMDIL_atomic_inc_lu32_noret:
             isRet = false;
             if (STM->calVersion() >= CAL_VERSION_SC_136) {
               IntNo = AMDILISD::ATOM_L_INC_NORET;
             } else {
               IntNo = AMDILISD::ATOM_L_ADD_NORET; 
             }
             break;
    case AMDGPUIntrinsic::AMDIL_atomic_inc_ri32:
    case AMDGPUIntrinsic::AMDIL_atomic_inc_ru32:
             if (STM->calVersion() >= CAL_VERSION_SC_136) {
               IntNo = AMDILISD::ATOM_R_INC;
             } else {
               IntNo = AMDILISD::ATOM_R_ADD; 
             }
             break;
    case AMDGPUIntrinsic::AMDIL_atomic_inc_ri32_noret:
    case AMDGPUIntrinsic::AMDIL_atomic_inc_ru32_noret:
             isRet = false;
             if (STM->calVersion() >= CAL_VERSION_SC_136) {
               IntNo = AMDILISD::ATOM_R_INC_NORET;
             } else {
               IntNo = AMDILISD::ATOM_R_ADD_NORET; 
             } 
             break;
    case AMDGPUIntrinsic::AMDIL_atomic_max_gi32:
             IntNo = AMDILISD::ATOM_G_MAX; break;
    case AMDGPUIntrinsic::AMDIL_atomic_max_gu32:
             IntNo = AMDILISD::ATOM_G_UMAX; break;
    case AMDGPUIntrinsic::AMDIL_atomic_max_gi32_noret:
             isRet = false;
             IntNo = AMDILISD::ATOM_G_MAX_NORET; break;
    case AMDGPUIntrinsic::AMDIL_atomic_max_gu32_noret:
             isRet = false;
             IntNo = AMDILISD::ATOM_G_UMAX_NORET; break;
    case AMDGPUIntrinsic::AMDIL_atomic_max_li32:
             IntNo = AMDILISD::ATOM_L_MAX; break;
    case AMDGPUIntrinsic::AMDIL_atomic_max_lu32:
             IntNo = AMDILISD::ATOM_L_UMAX; break;
    case AMDGPUIntrinsic::AMDIL_atomic_max_li32_noret:
             isRet = false;
             IntNo = AMDILISD::ATOM_L_MAX_NORET; break;
    case AMDGPUIntrinsic::AMDIL_atomic_max_lu32_noret:
             isRet = false;
             IntNo = AMDILISD::ATOM_L_UMAX_NORET; break;
    case AMDGPUIntrinsic::AMDIL_atomic_max_ri32:
             IntNo = AMDILISD::ATOM_R_MAX; break;
    case AMDGPUIntrinsic::AMDIL_atomic_max_ru32:
             IntNo = AMDILISD::ATOM_R_UMAX; break;
    case AMDGPUIntrinsic::AMDIL_atomic_max_ri32_noret:
             isRet = false;
             IntNo = AMDILISD::ATOM_R_MAX_NORET; break;
    case AMDGPUIntrinsic::AMDIL_atomic_max_ru32_noret:
             isRet = false;
             IntNo = AMDILISD::ATOM_R_UMAX_NORET; break;
    case AMDGPUIntrinsic::AMDIL_atomic_min_gi32:
             IntNo = AMDILISD::ATOM_G_MIN; break;
    case AMDGPUIntrinsic::AMDIL_atomic_min_gu32:
             IntNo = AMDILISD::ATOM_G_UMIN; break;
    case AMDGPUIntrinsic::AMDIL_atomic_min_gi32_noret:
             isRet = false;
             IntNo = AMDILISD::ATOM_G_MIN_NORET; break;
    case AMDGPUIntrinsic::AMDIL_atomic_min_gu32_noret:
             isRet = false;
             IntNo = AMDILISD::ATOM_G_UMIN_NORET; break;
    case AMDGPUIntrinsic::AMDIL_atomic_min_li32:
             IntNo = AMDILISD::ATOM_L_MIN; break;
    case AMDGPUIntrinsic::AMDIL_atomic_min_lu32:
             IntNo = AMDILISD::ATOM_L_UMIN; break;
    case AMDGPUIntrinsic::AMDIL_atomic_min_li32_noret:
             isRet = false;
             IntNo = AMDILISD::ATOM_L_MIN_NORET; break;
    case AMDGPUIntrinsic::AMDIL_atomic_min_lu32_noret:
             isRet = false;
             IntNo = AMDILISD::ATOM_L_UMIN_NORET; break;
    case AMDGPUIntrinsic::AMDIL_atomic_min_ri32:
             IntNo = AMDILISD::ATOM_R_MIN; break;
    case AMDGPUIntrinsic::AMDIL_atomic_min_ru32:
             IntNo = AMDILISD::ATOM_R_UMIN; break;
    case AMDGPUIntrinsic::AMDIL_atomic_min_ri32_noret:
             isRet = false;
             IntNo = AMDILISD::ATOM_R_MIN_NORET; break;
    case AMDGPUIntrinsic::AMDIL_atomic_min_ru32_noret:
             isRet = false;
             IntNo = AMDILISD::ATOM_R_UMIN_NORET; break;
    case AMDGPUIntrinsic::AMDIL_atomic_or_gi32:
    case AMDGPUIntrinsic::AMDIL_atomic_or_gu32:
             IntNo = AMDILISD::ATOM_G_OR; break;
    case AMDGPUIntrinsic::AMDIL_atomic_or_gi32_noret:
    case AMDGPUIntrinsic::AMDIL_atomic_or_gu32_noret:
             isRet = false;
             IntNo = AMDILISD::ATOM_G_OR_NORET; break;
    case AMDGPUIntrinsic::AMDIL_atomic_or_li32:
    case AMDGPUIntrinsic::AMDIL_atomic_or_lu32:
             IntNo = AMDILISD::ATOM_L_OR; break;
    case AMDGPUIntrinsic::AMDIL_atomic_or_li32_noret:
    case AMDGPUIntrinsic::AMDIL_atomic_or_lu32_noret:
             isRet = false;
             IntNo = AMDILISD::ATOM_L_OR_NORET; break;
    case AMDGPUIntrinsic::AMDIL_atomic_or_ri32:
    case AMDGPUIntrinsic::AMDIL_atomic_or_ru32:
             IntNo = AMDILISD::ATOM_R_OR; break;
    case AMDGPUIntrinsic::AMDIL_atomic_or_ri32_noret:
    case AMDGPUIntrinsic::AMDIL_atomic_or_ru32_noret:
             isRet = false;
             IntNo = AMDILISD::ATOM_R_OR_NORET; break;
    case AMDGPUIntrinsic::AMDIL_atomic_sub_gi32:
    case AMDGPUIntrinsic::AMDIL_atomic_sub_gu32:
             IntNo = AMDILISD::ATOM_G_SUB; break;
    case AMDGPUIntrinsic::AMDIL_atomic_sub_gi32_noret:
    case AMDGPUIntrinsic::AMDIL_atomic_sub_gu32_noret:
             isRet = false;
             IntNo = AMDILISD::ATOM_G_SUB_NORET; break;
    case AMDGPUIntrinsic::AMDIL_atomic_sub_li32:
    case AMDGPUIntrinsic::AMDIL_atomic_sub_lu32:
             IntNo = AMDILISD::ATOM_L_SUB; break;
    case AMDGPUIntrinsic::AMDIL_atomic_sub_li32_noret:
    case AMDGPUIntrinsic::AMDIL_atomic_sub_lu32_noret:
             isRet = false;
             IntNo = AMDILISD::ATOM_L_SUB_NORET; break;
    case AMDGPUIntrinsic::AMDIL_atomic_sub_ri32:
    case AMDGPUIntrinsic::AMDIL_atomic_sub_ru32:
             IntNo = AMDILISD::ATOM_R_SUB; break;
    case AMDGPUIntrinsic::AMDIL_atomic_sub_ri32_noret:
    case AMDGPUIntrinsic::AMDIL_atomic_sub_ru32_noret:
             isRet = false;
             IntNo = AMDILISD::ATOM_R_SUB_NORET; break;
    case AMDGPUIntrinsic::AMDIL_atomic_rsub_gi32:
    case AMDGPUIntrinsic::AMDIL_atomic_rsub_gu32:
             IntNo = AMDILISD::ATOM_G_RSUB; break;
    case AMDGPUIntrinsic::AMDIL_atomic_rsub_gi32_noret:
    case AMDGPUIntrinsic::AMDIL_atomic_rsub_gu32_noret:
             isRet = false;
             IntNo = AMDILISD::ATOM_G_RSUB_NORET; break;
    case AMDGPUIntrinsic::AMDIL_atomic_rsub_li32:
    case AMDGPUIntrinsic::AMDIL_atomic_rsub_lu32:
             IntNo = AMDILISD::ATOM_L_RSUB; break;
    case AMDGPUIntrinsic::AMDIL_atomic_rsub_li32_noret:
    case AMDGPUIntrinsic::AMDIL_atomic_rsub_lu32_noret:
             isRet = false;
             IntNo = AMDILISD::ATOM_L_RSUB_NORET; break;
    case AMDGPUIntrinsic::AMDIL_atomic_rsub_ri32:
    case AMDGPUIntrinsic::AMDIL_atomic_rsub_ru32:
             IntNo = AMDILISD::ATOM_R_RSUB; break;
    case AMDGPUIntrinsic::AMDIL_atomic_rsub_ri32_noret:
    case AMDGPUIntrinsic::AMDIL_atomic_rsub_ru32_noret:
             isRet = false;
             IntNo = AMDILISD::ATOM_R_RSUB_NORET; break;
    case AMDGPUIntrinsic::AMDIL_atomic_xchg_gf32:
             bitCastToInt = true;
    case AMDGPUIntrinsic::AMDIL_atomic_xchg_gi32:
    case AMDGPUIntrinsic::AMDIL_atomic_xchg_gu32:
             IntNo = AMDILISD::ATOM_G_XCHG; break;
    case AMDGPUIntrinsic::AMDIL_atomic_xchg_gf32_noret:
             bitCastToInt = true;
    case AMDGPUIntrinsic::AMDIL_atomic_xchg_gi32_noret:
    case AMDGPUIntrinsic::AMDIL_atomic_xchg_gu32_noret:
             isRet = false;
             IntNo = AMDILISD::ATOM_G_XCHG_NORET; break;
    case AMDGPUIntrinsic::AMDIL_atomic_xchg_lf32:
             bitCastToInt = true;
    case AMDGPUIntrinsic::AMDIL_atomic_xchg_li32:
    case AMDGPUIntrinsic::AMDIL_atomic_xchg_lu32:
             IntNo = AMDILISD::ATOM_L_XCHG; break;
    case AMDGPUIntrinsic::AMDIL_atomic_xchg_lf32_noret:
             bitCastToInt = true;
    case AMDGPUIntrinsic::AMDIL_atomic_xchg_li32_noret:
    case AMDGPUIntrinsic::AMDIL_atomic_xchg_lu32_noret:
             isRet = false;
             IntNo = AMDILISD::ATOM_L_XCHG_NORET; break;
    case AMDGPUIntrinsic::AMDIL_atomic_xchg_rf32:
             bitCastToInt = true;
    case AMDGPUIntrinsic::AMDIL_atomic_xchg_ri32:
    case AMDGPUIntrinsic::AMDIL_atomic_xchg_ru32:
             IntNo = AMDILISD::ATOM_R_XCHG; break;
    case AMDGPUIntrinsic::AMDIL_atomic_xchg_rf32_noret:
             bitCastToInt = true;
    case AMDGPUIntrinsic::AMDIL_atomic_xchg_ri32_noret:
    case AMDGPUIntrinsic::AMDIL_atomic_xchg_ru32_noret:
             isRet = false;
             IntNo = AMDILISD::ATOM_R_XCHG_NORET; break;
    case AMDGPUIntrinsic::AMDIL_atomic_xor_gi32:
    case AMDGPUIntrinsic::AMDIL_atomic_xor_gu32:
             IntNo = AMDILISD::ATOM_G_XOR; break;
    case AMDGPUIntrinsic::AMDIL_atomic_xor_gi32_noret:
    case AMDGPUIntrinsic::AMDIL_atomic_xor_gu32_noret:
             isRet = false;
             IntNo = AMDILISD::ATOM_G_XOR_NORET; break;
    case AMDGPUIntrinsic::AMDIL_atomic_xor_li32:
    case AMDGPUIntrinsic::AMDIL_atomic_xor_lu32:
             IntNo = AMDILISD::ATOM_L_XOR; break;
    case AMDGPUIntrinsic::AMDIL_atomic_xor_li32_noret:
    case AMDGPUIntrinsic::AMDIL_atomic_xor_lu32_noret:
             isRet = false;
             IntNo = AMDILISD::ATOM_L_XOR_NORET; break;
    case AMDGPUIntrinsic::AMDIL_atomic_xor_ri32:
    case AMDGPUIntrinsic::AMDIL_atomic_xor_ru32:
             IntNo = AMDILISD::ATOM_R_XOR; break;
    case AMDGPUIntrinsic::AMDIL_atomic_xor_ri32_noret:
    case AMDGPUIntrinsic::AMDIL_atomic_xor_ru32_noret:
             isRet = false;
             IntNo = AMDILISD::ATOM_R_XOR_NORET; break;
    case AMDGPUIntrinsic::AMDIL_append_alloc_i32:
             IntNo = AMDILISD::APPEND_ALLOC; break;
    case AMDGPUIntrinsic::AMDIL_append_alloc_i32_noret:
             isRet = false;
             IntNo = AMDILISD::APPEND_ALLOC_NORET; break;
    case AMDGPUIntrinsic::AMDIL_append_consume_i32:
             IntNo = AMDILISD::APPEND_CONSUME; break;
    case AMDGPUIntrinsic::AMDIL_append_consume_i32_noret:
             isRet = false;
             IntNo = AMDILISD::APPEND_CONSUME_NORET; break;
  };

  Info.opc = IntNo;
  Info.memVT = (bitCastToInt) ? MVT::f32 : MVT::i32;
  Info.ptrVal = I.getOperand(0);
  Info.offset = 0;
  Info.align = 4;
  Info.vol = true;
  Info.readMem = isRet;
  Info.writeMem = true;
  return true;
}
// The backend supports 32 and 64 bit floating point immediates
bool
AMDILTargetLowering::isFPImmLegal(const APFloat &Imm, EVT VT) const
{
  if (VT.getScalarType().getSimpleVT().SimpleTy == MVT::f32
      || VT.getScalarType().getSimpleVT().SimpleTy == MVT::f64) {
    return true;
  } else {
    return false;
  }
}

bool
AMDILTargetLowering::ShouldShrinkFPConstant(EVT VT) const
{
  if (VT.getScalarType().getSimpleVT().SimpleTy == MVT::f32
      || VT.getScalarType().getSimpleVT().SimpleTy == MVT::f64) {
    return false;
  } else {
    return true;
  }
}


// isMaskedValueZeroForTargetNode - Return true if 'Op & Mask' is known to
// be zero. Op is expected to be a target specific node. Used by DAG
// combiner.

void
AMDILTargetLowering::computeMaskedBitsForTargetNode(
    const SDValue Op,
    APInt &KnownZero,
    APInt &KnownOne,
    const SelectionDAG &DAG,
    unsigned Depth) const
{
  APInt KnownZero2;
  APInt KnownOne2;
  KnownZero = KnownOne = APInt(KnownOne.getBitWidth(), 0); // Don't know anything
  switch (Op.getOpcode()) {
    default: break;
    case AMDILISD::SELECT_CC:
             DAG.ComputeMaskedBits(
                 Op.getOperand(1),
                 KnownZero,
                 KnownOne,
                 Depth + 1
                 );
             DAG.ComputeMaskedBits(
                 Op.getOperand(0),
                 KnownZero2,
                 KnownOne2
                 );
             assert((KnownZero & KnownOne) == 0
                 && "Bits known to be one AND zero?");
             assert((KnownZero2 & KnownOne2) == 0
                 && "Bits known to be one AND zero?");
             // Only known if known in both the LHS and RHS
             KnownOne &= KnownOne2;
             KnownZero &= KnownZero2;
             break;
  };
}

// This is the function that determines which calling convention should
// be used. Currently there is only one calling convention
CCAssignFn*
AMDILTargetLowering::CCAssignFnForNode(unsigned int Op) const
{
  //uint64_t CC = cast<ConstantSDNode>(Op.getOperand(1))->getZExtValue();
  return CC_AMDIL32;
}

// LowerCallResult - Lower the result values of an ISD::CALL into the
// appropriate copies out of appropriate physical registers.  This assumes that
// Chain/InFlag are the input chain/flag to use, and that TheCall is the call
// being lowered.  The returns a SDNode with the same number of values as the
// ISD::CALL.
SDValue
AMDILTargetLowering::LowerCallResult(
    SDValue Chain,
    SDValue InFlag,
    CallingConv::ID CallConv,
    bool isVarArg,
    const SmallVectorImpl<ISD::InputArg> &Ins,
    DebugLoc dl,
    SelectionDAG &DAG,
    SmallVectorImpl<SDValue> &InVals) const
{
  // Assign locations to each value returned by this call
  SmallVector<CCValAssign, 16> RVLocs;
  CCState CCInfo(CallConv, isVarArg, DAG.getMachineFunction(),
                 getTargetMachine(), RVLocs, *DAG.getContext());
  CCInfo.AnalyzeCallResult(Ins, RetCC_AMDIL32);

  // Copy all of the result registers out of their specified physreg.
  for (unsigned i = 0; i != RVLocs.size(); ++i) {
    EVT CopyVT = RVLocs[i].getValVT();
    if (RVLocs[i].isRegLoc()) {
      Chain = DAG.getCopyFromReg(
          Chain,
          dl,
          RVLocs[i].getLocReg(),
          CopyVT,
          InFlag
          ).getValue(1);
      SDValue Val = Chain.getValue(0);
      InFlag = Chain.getValue(2);
      InVals.push_back(Val);
    }
  }

  return Chain;

}

//===----------------------------------------------------------------------===//
//                           Other Lowering Hooks
//===----------------------------------------------------------------------===//

// Recursively assign SDNodeOrdering to any unordered nodes
// This is necessary to maintain source ordering of instructions
// under -O0 to avoid odd-looking "skipping around" issues.
  static const SDValue
Ordered( SelectionDAG &DAG, unsigned order, const SDValue New )
{
  if (order != 0 && DAG.GetOrdering( New.getNode() ) == 0) {
    DAG.AssignOrdering( New.getNode(), order );
    for (unsigned i = 0, e = New.getNumOperands(); i < e; ++i)
      Ordered( DAG, order, New.getOperand(i) );
  }
  return New;
}

#define LOWER(A) \
  case ISD:: A: \
return Ordered( DAG, DAG.GetOrdering( Op.getNode() ), Lower##A(Op, DAG) )

SDValue
AMDILTargetLowering::LowerOperation(SDValue Op, SelectionDAG &DAG) const
{
  switch (Op.getOpcode()) {
    default:
      Op.getNode()->dump();
      assert(0 && "Custom lowering code for this"
          "instruction is not implemented yet!");
      break;
      LOWER(GlobalAddress);
      LOWER(JumpTable);
      LOWER(ConstantPool);
      LOWER(ExternalSymbol);
      LOWER(FP_TO_UINT);
      LOWER(UINT_TO_FP);
      LOWER(MUL);
      LOWER(SUB);
      LOWER(FDIV);
      LOWER(SDIV);
      LOWER(SREM);
      LOWER(UREM);
      LOWER(BUILD_VECTOR);
      LOWER(INSERT_VECTOR_ELT);
      LOWER(EXTRACT_VECTOR_ELT);
      LOWER(EXTRACT_SUBVECTOR);
      LOWER(SCALAR_TO_VECTOR);
      LOWER(CONCAT_VECTORS);
      LOWER(SELECT);
      LOWER(SETCC);
      LOWER(SIGN_EXTEND_INREG);
      LOWER(DYNAMIC_STACKALLOC);
      LOWER(BRCOND);
      LOWER(BR_CC);
      LOWER(FP_ROUND);
  }
  return Op;
}

int
AMDILTargetLowering::getVarArgsFrameOffset() const
{
  return VarArgsFrameOffset;
}
#undef LOWER

SDValue
AMDILTargetLowering::LowerGlobalAddress(SDValue Op, SelectionDAG &DAG) const
{
  SDValue DST = Op;
  const GlobalAddressSDNode *GADN = cast<GlobalAddressSDNode>(Op);
  const GlobalValue *G = GADN->getGlobal();
  DebugLoc DL = Op.getDebugLoc();
  const GlobalVariable *GV = dyn_cast<GlobalVariable>(G);
  if (!GV) {
    DST = DAG.getTargetGlobalAddress(GV, DL, MVT::i32);
  } else {
    if (GV->hasInitializer()) {
      const Constant *C = dyn_cast<Constant>(GV->getInitializer());
      if (const ConstantInt *CI = dyn_cast<ConstantInt>(C)) {
        DST = DAG.getConstant(CI->getValue(), Op.getValueType());
      } else if (const ConstantFP *CF = dyn_cast<ConstantFP>(C)) {
        DST = DAG.getConstantFP(CF->getValueAPF(),
            Op.getValueType());
      } else if (dyn_cast<ConstantAggregateZero>(C)) {
        EVT VT = Op.getValueType();
        if (VT.isInteger()) {
          DST = DAG.getConstant(0, VT);
        } else {
          DST = DAG.getConstantFP(0, VT);
        }
      } else {
        assert(!"lowering this type of Global Address "
            "not implemented yet!");
        C->dump();
        DST = DAG.getTargetGlobalAddress(GV, DL, MVT::i32);
      }
    } else {
      DST = DAG.getTargetGlobalAddress(GV, DL, MVT::i32);
    }
  }
  return DST;
}

SDValue
AMDILTargetLowering::LowerJumpTable(SDValue Op, SelectionDAG &DAG) const
{
  JumpTableSDNode *JT = cast<JumpTableSDNode>(Op);
  SDValue Result = DAG.getTargetJumpTable(JT->getIndex(), MVT::i32);
  return Result;
}
SDValue
AMDILTargetLowering::LowerConstantPool(SDValue Op, SelectionDAG &DAG) const
{
  ConstantPoolSDNode *CP = cast<ConstantPoolSDNode>(Op);
  EVT PtrVT = Op.getValueType();
  SDValue Result;
  if (CP->isMachineConstantPoolEntry()) {
    Result = DAG.getTargetConstantPool(CP->getMachineCPVal(), PtrVT,
        CP->getAlignment(), CP->getOffset(), CP->getTargetFlags());
  } else {
    Result = DAG.getTargetConstantPool(CP->getConstVal(), PtrVT,
        CP->getAlignment(), CP->getOffset(), CP->getTargetFlags());
  }
  return Result;
}

SDValue
AMDILTargetLowering::LowerExternalSymbol(SDValue Op, SelectionDAG &DAG) const
{
  const char *Sym = cast<ExternalSymbolSDNode>(Op)->getSymbol();
  SDValue Result = DAG.getTargetExternalSymbol(Sym, MVT::i32);
  return Result;
}

/// LowerFORMAL_ARGUMENTS - transform physical registers into
/// virtual registers and generate load operations for
/// arguments places on the stack.
/// TODO: isVarArg, hasStructRet, isMemReg
  SDValue
AMDILTargetLowering::LowerFormalArguments(SDValue Chain,
    CallingConv::ID CallConv,
    bool isVarArg,
    const SmallVectorImpl<ISD::InputArg> &Ins,
    DebugLoc dl,
    SelectionDAG &DAG,
    SmallVectorImpl<SDValue> &InVals)
const
{

  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo *MFI = MF.getFrameInfo();
  //const Function *Fn = MF.getFunction();
  //MachineRegisterInfo &RegInfo = MF.getRegInfo();

  SmallVector<CCValAssign, 16> ArgLocs;
  CallingConv::ID CC = MF.getFunction()->getCallingConv();
  //bool hasStructRet = MF.getFunction()->hasStructRetAttr();

  CCState CCInfo(CC, isVarArg, DAG.getMachineFunction(),
                 getTargetMachine(), ArgLocs, *DAG.getContext());

  // When more calling conventions are added, they need to be chosen here
  CCInfo.AnalyzeFormalArguments(Ins, CC_AMDIL32);
  SDValue StackPtr;

  //unsigned int FirstStackArgLoc = 0;

  for (unsigned int i = 0, e = ArgLocs.size(); i != e; ++i) {
    CCValAssign &VA = ArgLocs[i];
    if (VA.isRegLoc()) {
      EVT RegVT = VA.getLocVT();
      const TargetRegisterClass *RC = getRegClassFromType(
          RegVT.getSimpleVT().SimpleTy);

      unsigned int Reg = MF.addLiveIn(VA.getLocReg(), RC);
      SDValue ArgValue = DAG.getCopyFromReg(
          Chain,
          dl,
          Reg,
          RegVT);
      // If this is an 8 or 16-bit value, it is really passed
      // promoted to 32 bits.  Insert an assert[sz]ext to capture
      // this, then truncate to the right size.

      if (VA.getLocInfo() == CCValAssign::SExt) {
        ArgValue = DAG.getNode(
            ISD::AssertSext,
            dl,
            RegVT,
            ArgValue,
            DAG.getValueType(VA.getValVT()));
      } else if (VA.getLocInfo() == CCValAssign::ZExt) {
        ArgValue = DAG.getNode(
            ISD::AssertZext,
            dl,
            RegVT,
            ArgValue,
            DAG.getValueType(VA.getValVT()));
      }
      if (VA.getLocInfo() != CCValAssign::Full) {
        ArgValue = DAG.getNode(
            ISD::TRUNCATE,
            dl,
            VA.getValVT(),
            ArgValue);
      }
      // Add the value to the list of arguments
      // to be passed in registers
      InVals.push_back(ArgValue);
      if (isVarArg) {
        assert(0 && "Variable arguments are not yet supported");
        // See MipsISelLowering.cpp for ideas on how to implement
      }
    } else if(VA.isMemLoc()) {
      InVals.push_back(LowerMemArgument(Chain, CallConv, Ins,
            dl, DAG, VA, MFI, i));
    } else {
      assert(0 && "found a Value Assign that is "
          "neither a register or a memory location");
    }
  }
  /*if (hasStructRet) {
    assert(0 && "Has struct return is not yet implemented");
  // See MipsISelLowering.cpp for ideas on how to implement
  }*/

  if (isVarArg) {
    assert(0 && "Variable arguments are not yet supported");
    // See X86/PPC/CellSPU ISelLowering.cpp for ideas on how to implement
  }
  // This needs to be changed to non-zero if the return function needs
  // to pop bytes
  return Chain;
}
/// CreateCopyOfByValArgument - Make a copy of an aggregate at address specified
/// by "Src" to address "Dst" with size and alignment information specified by
/// the specific parameter attribute. The copy will be passed as a byval
/// function parameter.
static SDValue
CreateCopyOfByValArgument(SDValue Src, SDValue Dst, SDValue Chain,
    ISD::ArgFlagsTy Flags, SelectionDAG &DAG) {
  assert(0 && "MemCopy does not exist yet");
  SDValue SizeNode     = DAG.getConstant(Flags.getByValSize(), MVT::i32);

  return DAG.getMemcpy(Chain,
      Src.getDebugLoc(),
      Dst, Src, SizeNode, Flags.getByValAlign(),
      /*IsVol=*/false, /*AlwaysInline=*/true, 
      MachinePointerInfo(), MachinePointerInfo());
}

SDValue
AMDILTargetLowering::LowerMemOpCallTo(SDValue Chain,
    SDValue StackPtr, SDValue Arg,
    DebugLoc dl, SelectionDAG &DAG,
    const CCValAssign &VA,
    ISD::ArgFlagsTy Flags) const
{
  unsigned int LocMemOffset = VA.getLocMemOffset();
  SDValue PtrOff = DAG.getIntPtrConstant(LocMemOffset);
  PtrOff = DAG.getNode(ISD::ADD,
      dl,
      getPointerTy(), StackPtr, PtrOff);
  if (Flags.isByVal()) {
    PtrOff = CreateCopyOfByValArgument(Arg, PtrOff, Chain, Flags, DAG);
  } else {
    PtrOff = DAG.getStore(Chain, dl, Arg, PtrOff,
        MachinePointerInfo::getStack(LocMemOffset),
        false, false, 0);
  }
  return PtrOff;
}
/// LowerCAL - functions arguments are copied from virtual
/// regs to (physical regs)/(stack frame), CALLSEQ_START and
/// CALLSEQ_END are emitted.
/// TODO: isVarArg, isTailCall, hasStructRet
SDValue
AMDILTargetLowering::LowerCall(SDValue Chain, SDValue Callee,
    CallingConv::ID CallConv, bool isVarArg, bool doesNotRet,
    bool& isTailCall,
    const SmallVectorImpl<ISD::OutputArg> &Outs,
    const SmallVectorImpl<SDValue> &OutVals,
    const SmallVectorImpl<ISD::InputArg> &Ins,
    DebugLoc dl, SelectionDAG &DAG,
    SmallVectorImpl<SDValue> &InVals)
const
{
  isTailCall = false;
  MachineFunction& MF = DAG.getMachineFunction();
  // FIXME: DO we need to handle fast calling conventions and tail call
  // optimizations?? X86/PPC ISelLowering
  /*bool hasStructRet = (TheCall->getNumArgs())
    ? TheCall->getArgFlags(0).device()->isSRet()
    : false;*/

  MachineFrameInfo *MFI = MF.getFrameInfo();

  // Analyze operands of the call, assigning locations to each operand
  SmallVector<CCValAssign, 16> ArgLocs;
  CCState CCInfo(CallConv, isVarArg, DAG.getMachineFunction(),
                 getTargetMachine(), ArgLocs, *DAG.getContext());
  // Analyize the calling operands, but need to change
  // if we have more than one calling convetion
  CCInfo.AnalyzeCallOperands(Outs, CCAssignFnForNode(CallConv));

  unsigned int NumBytes = CCInfo.getNextStackOffset();
  if (isTailCall) {
    assert(isTailCall && "Tail Call not handled yet!");
    // See X86/PPC ISelLowering
  }

  Chain = DAG.getCALLSEQ_START(Chain, DAG.getIntPtrConstant(NumBytes, true));

  SmallVector<std::pair<unsigned int, SDValue>, 8> RegsToPass;
  SmallVector<SDValue, 8> MemOpChains;
  SDValue StackPtr;
  //unsigned int FirstStacArgLoc = 0;
  //int LastArgStackLoc = 0;

  // Walk the register/memloc assignments, insert copies/loads
  for (unsigned int i = 0, e = ArgLocs.size(); i != e; ++i) {
    CCValAssign &VA = ArgLocs[i];
    //bool isByVal = Flags.isByVal(); // handle byval/bypointer registers
    // Arguments start after the 5 first operands of ISD::CALL
    SDValue Arg = OutVals[i];
    //Promote the value if needed
    switch(VA.getLocInfo()) {
      default: assert(0 && "Unknown loc info!");
      case CCValAssign::Full:
               break;
      case CCValAssign::SExt:
               Arg = DAG.getNode(ISD::SIGN_EXTEND,
                   dl,
                   VA.getLocVT(), Arg);
               break;
      case CCValAssign::ZExt:
               Arg = DAG.getNode(ISD::ZERO_EXTEND,
                   dl,
                   VA.getLocVT(), Arg);
               break;
      case CCValAssign::AExt:
               Arg = DAG.getNode(ISD::ANY_EXTEND,
                   dl,
                   VA.getLocVT(), Arg);
               break;
    }

    if (VA.isRegLoc()) {
      RegsToPass.push_back(std::make_pair(VA.getLocReg(), Arg));
    } else if (VA.isMemLoc()) {
      // Create the frame index object for this incoming parameter
      int FI = MFI->CreateFixedObject(VA.getValVT().getSizeInBits()/8,
          VA.getLocMemOffset(), true);
      SDValue PtrOff = DAG.getFrameIndex(FI,getPointerTy());

      // emit ISD::STORE whichs stores the
      // parameter value to a stack Location
      MemOpChains.push_back(DAG.getStore(Chain, dl, Arg, PtrOff,
            MachinePointerInfo::getFixedStack(FI),
            false, false, 0));
    } else {
      assert(0 && "Not a Reg/Mem Loc, major error!");
    }
  }
  if (!MemOpChains.empty()) {
    Chain = DAG.getNode(ISD::TokenFactor,
        dl,
        MVT::Other,
        &MemOpChains[0],
        MemOpChains.size());
  }
  SDValue InFlag;
  if (!isTailCall) {
    for (unsigned int i = 0, e = RegsToPass.size(); i != e; ++i) {
      Chain = DAG.getCopyToReg(Chain,
          dl,
          RegsToPass[i].first,
          RegsToPass[i].second,
          InFlag);
      InFlag = Chain.getValue(1);
    }
  }

  // If the callee is a GlobalAddress/ExternalSymbol node (quite common,
  // every direct call is) turn it into a TargetGlobalAddress/
  // TargetExternalSymbol
  // node so that legalize doesn't hack it.
  if (GlobalAddressSDNode *G = dyn_cast<GlobalAddressSDNode>(Callee))  {
    Callee = DAG.getTargetGlobalAddress(G->getGlobal(), dl, getPointerTy());
  }
  else if (ExternalSymbolSDNode *S = dyn_cast<ExternalSymbolSDNode>(Callee)) {
    Callee = DAG.getTargetExternalSymbol(S->getSymbol(), getPointerTy());
  }
  else if (isTailCall) {
    assert(0 && "Tail calls are not handled yet");
    // see X86 ISelLowering for ideas on implementation: 1708
  }

  SDVTList NodeTys = DAG.getVTList(MVT::Other, MVTGLUE);
  SmallVector<SDValue, 8> Ops;

  if (isTailCall) {
    assert(0 && "Tail calls are not handled yet");
    // see X86 ISelLowering for ideas on implementation: 1721
  }
  // If this is a direct call, pass the chain and the callee
  if (Callee.getNode()) {
    Ops.push_back(Chain);
    Ops.push_back(Callee);
  }

  if (isTailCall) {
    assert(0 && "Tail calls are not handled yet");
    // see X86 ISelLowering for ideas on implementation: 1739
  }

  // Add argument registers to the end of the list so that they are known
  // live into the call
  for (unsigned int i = 0, e = RegsToPass.size(); i != e; ++i) {
    Ops.push_back(DAG.getRegister(
          RegsToPass[i].first,
          RegsToPass[i].second.getValueType()));
  }
  if (InFlag.getNode()) {
    Ops.push_back(InFlag);
  }

  // Emit Tail Call
  if (isTailCall) {
    assert(0 && "Tail calls are not handled yet");
    // see X86 ISelLowering for ideas on implementation: 1762
  }

  Chain = DAG.getNode(AMDILISD::CALL,
      dl,
      NodeTys, &Ops[0], Ops.size());
  InFlag = Chain.getValue(1);

  // Create the CALLSEQ_END node
  Chain = DAG.getCALLSEQ_END(
      Chain,
      DAG.getIntPtrConstant(NumBytes, true),
      DAG.getIntPtrConstant(0, true),
      InFlag);
  InFlag = Chain.getValue(1);
  // Handle result values, copying them out of physregs into vregs that
  // we return
  return LowerCallResult(Chain, InFlag, CallConv, isVarArg, Ins, dl, DAG,
      InVals);
}

SDValue
AMDILTargetLowering::genCLZuN(SDValue Op, SelectionDAG &DAG,
    uint32_t bits) const
{
  DebugLoc DL = Op.getDebugLoc();
  EVT INTTY = Op.getValueType();
  EVT FPTY;
  if (INTTY.isVector()) {
    FPTY = EVT(MVT::getVectorVT(MVT::f32,
          INTTY.getVectorNumElements()));
  } else {
    FPTY = EVT(MVT::f32);
  }
  /* static inline uint
     __clz_Nbit(uint x)
     {
     int xor = 0x3f800000U | x;
     float tp = as_float(xor);
     float t = tp + -1.0f;
     uint tint = as_uint(t);
     int cmp = (x != 0);
     uint tsrc = tint >> 23;
     uint tmask = tsrc & 0xffU;
     uint cst = (103 + N)U - tmask;
     return cmp ? cst : N;
     }
     */
  assert(INTTY.getScalarType().getSimpleVT().SimpleTy == MVT::i32
      && "genCLZu16 only works on 32bit types");
  // uint x = Op
  SDValue x = Op;
  // xornode = 0x3f800000 | x
  SDValue xornode = DAG.getNode(ISD::OR, DL, INTTY,
      DAG.getConstant(0x3f800000, INTTY), x);
  // float tp = as_float(xornode)
  SDValue tp = DAG.getNode(ISDBITCAST, DL, FPTY, xornode);
  // float t = tp + -1.0f
  SDValue t = DAG.getNode(ISD::FADD, DL, FPTY, tp,
      DAG.getConstantFP(-1.0f, FPTY));
  // uint tint = as_uint(t)
  SDValue tint = DAG.getNode(ISDBITCAST, DL, INTTY, t);
  // int cmp = (x != 0)
  SDValue cmp = DAG.getNode(AMDILISD::CMP, DL, INTTY,
      DAG.getConstant(CondCCodeToCC(ISD::SETNE, MVT::i32), MVT::i32), x,
      DAG.getConstant(0, INTTY));
  // uint tsrc = tint >> 23
  SDValue tsrc = DAG.getNode(ISD::SRL, DL, INTTY, tint,
      DAG.getConstant(23, INTTY));
  // uint tmask = tsrc & 0xFF
  SDValue tmask = DAG.getNode(ISD::AND, DL, INTTY, tsrc,
      DAG.getConstant(0xFFU, INTTY));
  // uint cst = (103 + bits) - tmask
  SDValue cst = DAG.getNode(ISD::SUB, DL, INTTY,
      DAG.getConstant((103U + bits), INTTY), tmask);
  // return cmp ? cst : N
  cst = DAG.getNode(AMDILISD::CMOVLOG, DL, INTTY, cmp, cst,
      DAG.getConstant(bits, INTTY));
  return cst;
}

SDValue
AMDILTargetLowering::genCLZu32(SDValue Op, SelectionDAG &DAG) const
{
  SDValue DST = SDValue();
  DebugLoc DL = Op.getDebugLoc();
  EVT INTTY = Op.getValueType();
  const AMDILSubtarget *stm = reinterpret_cast<const AMDILTargetMachine*>(
      &this->getTargetMachine())->getSubtargetImpl();
  if (stm->device()->getGeneration() >= AMDILDeviceInfo::HD5XXX) {
    //__clz_32bit(uint u)
    //{
    // int z = __amdil_ffb_hi(u) ;
    // return z < 0 ? 32 : z;
    // }
    // uint u = op
    SDValue u = Op;
    // int z = __amdil_ffb_hi(u)
    SDValue z = DAG.getNode(AMDILISD::IFFB_HI, DL, INTTY, u);
    // int cmp = z < 0
    SDValue cmp = DAG.getNode(AMDILISD::CMP, DL, INTTY,
        DAG.getConstant(CondCCodeToCC(ISD::SETLT, MVT::i32), MVT::i32),
        z, DAG.getConstant(0, INTTY));
    // return cmp ? 32 : z
    DST = DAG.getNode(AMDILISD::CMOVLOG, DL, INTTY, cmp,
        DAG.getConstant(32, INTTY), z);
  } else if (stm->device()->getGeneration() == AMDILDeviceInfo::HD4XXX) {
    //  static inline uint
    //__clz_32bit(uint x)
    //{
    //    uint zh = __clz_16bit(x >> 16);
    //    uint zl = __clz_16bit(x & 0xffffU);
    //   return zh == 16U ? 16U + zl : zh;
    //}
    // uint x = Op
    SDValue x = Op;
    // uint xs16 = x >> 16
    SDValue xs16 = DAG.getNode(ISD::SRL, DL, INTTY, x,
        DAG.getConstant(16, INTTY));
    // uint zh = __clz_16bit(xs16)
    SDValue zh = genCLZuN(xs16, DAG, 16);
    // uint xa16 = x & 0xFFFF
    SDValue xa16 = DAG.getNode(ISD::AND, DL, INTTY, x,
        DAG.getConstant(0xFFFFU, INTTY));
    // uint zl = __clz_16bit(xa16)
    SDValue zl = genCLZuN(xa16, DAG, 16);
    // uint cmp = zh == 16U
    SDValue cmp = DAG.getNode(AMDILISD::CMP, DL, INTTY,
        DAG.getConstant(CondCCodeToCC(ISD::SETEQ, MVT::i32), MVT::i32),
        zh, DAG.getConstant(16U, INTTY));
    // uint zl16 = zl + 16
    SDValue zl16 = DAG.getNode(ISD::ADD, DL, INTTY,
        DAG.getConstant(16, INTTY), zl);
    // return cmp ? zl16 : zh
    DST = DAG.getNode(AMDILISD::CMOVLOG, DL, INTTY,
        cmp, zl16, zh);
  } else {
    assert(0 && "Attempting to generate a CLZ function with an"
        " unknown graphics card");
  }
  return DST;
}
SDValue
AMDILTargetLowering::genCLZu64(SDValue Op, SelectionDAG &DAG) const
{
  SDValue DST = SDValue();
  DebugLoc DL = Op.getDebugLoc();
  EVT INTTY;
  EVT LONGTY = Op.getValueType();
  bool isVec = LONGTY.isVector();
  if (isVec) {
    INTTY = EVT(MVT::getVectorVT(MVT::i32, Op.getValueType()
          .getVectorNumElements()));
  } else {
    INTTY = EVT(MVT::i32);
  }
  const AMDILSubtarget *stm = reinterpret_cast<const AMDILTargetMachine*>(
      &this->getTargetMachine())->getSubtargetImpl();
  if (stm->device()->getGeneration() >= AMDILDeviceInfo::HD5XXX) {
    // Evergreen:
    // static inline uint
    // __clz_u64(ulong x)
    // {
    //uint zhi = __clz_32bit((uint)(x >> 32));
    //uint zlo = __clz_32bit((uint)(x & 0xffffffffUL));
    //return zhi == 32U ? 32U + zlo : zhi;
    //}
    //ulong x = op
    SDValue x = Op;
    // uint xhi = x >> 32
    SDValue xlo = DAG.getNode((isVec) ? AMDILISD::LCOMPLO2 : AMDILISD::LCOMPLO, DL, INTTY, x);
    // uint xlo = x & 0xFFFFFFFF
    SDValue xhi = DAG.getNode((isVec) ? AMDILISD::LCOMPHI2 : AMDILISD::LCOMPHI, DL, INTTY, x);
    // uint zhi = __clz_32bit(xhi)
    SDValue zhi = genCLZu32(xhi, DAG);
    // uint zlo = __clz_32bit(xlo)
    SDValue zlo = genCLZu32(xlo, DAG);
    // uint cmp = zhi == 32
    SDValue cmp = DAG.getNode(AMDILISD::CMP, DL, INTTY,
        DAG.getConstant(CondCCodeToCC(ISD::SETEQ, MVT::i32), MVT::i32),
        zhi, DAG.getConstant(32U, INTTY));
    // uint zlop32 = 32 + zlo
    SDValue zlop32 = DAG.getNode(AMDILISD::ADD, DL, INTTY,
        DAG.getConstant(32U, INTTY), zlo);
    // return cmp ? zlop32: zhi
    DST = DAG.getNode(AMDILISD::CMOVLOG, DL, INTTY, cmp, zlop32, zhi);
  } else if (stm->device()->getGeneration() == AMDILDeviceInfo::HD4XXX) {
    // HD4XXX:
    //  static inline uint
    //__clz_64bit(ulong x)
    //{
    //uint zh = __clz_23bit((uint)(x >> 46)) - 5U;
    //uint zm = __clz_23bit((uint)(x >> 23) & 0x7fffffU);
    //uint zl = __clz_23bit((uint)x & 0x7fffffU);
    //uint r = zh == 18U ? 18U + zm : zh;
    //return zh + zm == 41U ? 41U + zl : r;
    //}
    //ulong x = Op
    SDValue x = Op;
    // ulong xs46 = x >> 46
    SDValue xs46 = DAG.getNode(ISD::SRL, DL, LONGTY, x,
        DAG.getConstant(46, LONGTY));
    // uint ixs46 = (uint)xs46
    SDValue ixs46 = DAG.getNode(ISD::TRUNCATE, DL, INTTY, xs46);
    // ulong xs23 = x >> 23
    SDValue xs23 = DAG.getNode(ISD::SRL, DL, LONGTY, x,
        DAG.getConstant(23, LONGTY));
    // uint ixs23 = (uint)xs23
    SDValue ixs23 = DAG.getNode(ISD::TRUNCATE, DL, INTTY, xs23);
    // uint xs23m23 = ixs23 & 0x7FFFFF
    SDValue xs23m23 = DAG.getNode(ISD::AND, DL, INTTY, ixs23,
        DAG.getConstant(0x7fffffU, INTTY));
    // uint ix = (uint)x
    SDValue ix = DAG.getNode((isVec) ? AMDILISD::LCOMPLO2 : AMDILISD::LCOMPLO, DL, INTTY, x);
    // uint xm23 = ix & 0x7FFFFF
    SDValue xm23 = DAG.getNode(ISD::AND, DL, INTTY, ix,
        DAG.getConstant(0x7fffffU, INTTY));
    // uint zh = __clz_23bit(ixs46)
    SDValue zh = genCLZuN(ixs46, DAG, 23);
    // uint zm = __clz_23bit(xs23m23)
    SDValue zm = genCLZuN(xs23m23, DAG, 23);
    // uint zl = __clz_23bit(xm23)
    SDValue zl = genCLZuN(xm23, DAG, 23);
    // uint zhm5 = zh - 5
    SDValue zhm5 = DAG.getNode(ISD::ADD, DL, INTTY, zh,
        DAG.getConstant(-5U, INTTY));
    SDValue const18 = DAG.getConstant(18, INTTY);
    SDValue const41 = DAG.getConstant(41, INTTY);
    // uint cmp1 = zh = 18
    SDValue cmp1 = DAG.getNode(AMDILISD::CMP, DL, INTTY,
        DAG.getConstant(CondCCodeToCC(ISD::SETEQ, MVT::i32), MVT::i32),
        zhm5, const18);
    // uint zhm5zm = zhm5 + zh
    SDValue zhm5zm = DAG.getNode(ISD::ADD, DL, INTTY, zhm5, zm);
    // uint cmp2 = zhm5zm == 41
    SDValue cmp2 = DAG.getNode(AMDILISD::CMP, DL, INTTY,
        DAG.getConstant(CondCCodeToCC(ISD::SETEQ, MVT::i32), MVT::i32),
        zhm5zm, const41);
    // uint zmp18 = zhm5 + 18
    SDValue zmp18 = DAG.getNode(ISD::ADD, DL, INTTY, zm, const18);
    // uint zlp41 = zl + 41
    SDValue zlp41 = DAG.getNode(ISD::ADD, DL, INTTY, zl, const41);
    // uint r = cmp1 ? zmp18 : zh
    SDValue r = DAG.getNode(AMDILISD::CMOVLOG, DL, INTTY,
        cmp1, zmp18, zhm5);
    // return cmp2 ? zlp41 : r
    DST = DAG.getNode(AMDILISD::CMOVLOG, DL, INTTY, cmp2, zlp41, r);
  } else {
    assert(0 && "Attempting to generate a CLZ function with an"
        " unknown graphics card");
  }
  return DST;
}
SDValue
AMDILTargetLowering::genf64toi64(SDValue RHS, SelectionDAG &DAG,
    bool includeSign) const
{
  EVT INTVT;
  EVT LONGVT;
  SDValue DST;
  DebugLoc DL = RHS.getDebugLoc();
  EVT RHSVT = RHS.getValueType();
  bool isVec = RHSVT.isVector();
  if (isVec) {
    LONGVT = EVT(MVT::getVectorVT(MVT::i64, RHSVT
          .getVectorNumElements()));
    INTVT = EVT(MVT::getVectorVT(MVT::i32, RHSVT
          .getVectorNumElements()));
  } else {
    LONGVT = EVT(MVT::i64);
    INTVT = EVT(MVT::i32);
  }
  const AMDILSubtarget *stm = reinterpret_cast<const AMDILTargetMachine*>(
      &this->getTargetMachine())->getSubtargetImpl();
  if (stm->device()->getGeneration() > AMDILDeviceInfo::HD6XXX) {
    // unsigned version:
    // uint uhi = (uint)(d * 0x1.0p-32);
    // uint ulo = (uint)(mad((double)uhi, -0x1.0p+32, d));
    // return as_ulong2((uint2)(ulo, uhi));
    //
    // signed version:
    // double ad = fabs(d);
    // long l = unsigned_version(ad);
    // long nl = -l;
    // return d == ad ? l : nl;
    SDValue d = RHS;
    if (includeSign) {
      d = DAG.getNode(ISD::FABS, DL, RHSVT, d);
    }
    SDValue uhid = DAG.getNode(ISD::FMUL, DL, RHSVT, d, 
        DAG.getConstantFP(0x2f800000, RHSVT));
    SDValue uhi = DAG.getNode(ISD::FP_TO_UINT, DL, INTVT, uhid);
    SDValue ulod = DAG.getNode(ISD::UINT_TO_FP, DL, RHSVT, uhi);
    ulod = DAG.getNode(AMDILISD::MAD, DL, RHSVT, ulod, 
        DAG.getConstantFP(0xcf800000, RHSVT), d);
    SDValue ulo = DAG.getNode(ISD::FP_TO_UINT, DL, INTVT, ulod);
    SDValue l = DAG.getNode((isVec) ? AMDILISD::LCREATE2 : AMDILISD::LCREATE, DL, LONGVT, ulo, uhi);
    if (includeSign) {
      SDValue nl = DAG.getNode(AMDILISD::INEGATE, DL, LONGVT, l);
      SDValue c = DAG.getNode(AMDILISD::CMP, DL, RHSVT,
          DAG.getConstant(CondCCodeToCC(ISD::SETEQ, MVT::f64), MVT::i32),
          RHS, d);
      l = DAG.getNode(AMDILISD::CMOVLOG, DL, LONGVT, c, l, nl);
    }
    DST = l;
  } else {
    /*
       __attribute__((always_inline)) long
       cast_f64_to_i64(double d)
       {
    // Convert d in to 32-bit components
    long x = as_long(d);
    xhi = LCOMPHI(x);
    xlo = LCOMPLO(x);

    // Generate 'normalized' mantissa
    mhi = xhi | 0x00100000; // hidden bit
    mhi <<= 11;
    temp = xlo >> (32 - 11);
    mhi |= temp
    mlo = xlo << 11;

    // Compute shift right count from exponent
    e = (xhi >> (52-32)) & 0x7ff;
    sr = 1023 + 63 - e;
    srge64 = sr >= 64;
    srge32 = sr >= 32;

    // Compute result for 0 <= sr < 32
    rhi0 = mhi >> (sr &31);
    rlo0 = mlo >> (sr &31);
    temp = mhi << (32 - sr);
    temp |= rlo0;
    rlo0 = sr ? temp : rlo0;

    // Compute result for 32 <= sr
    rhi1 = 0;
    rlo1 = srge64 ? 0 : rhi0;

    // Pick between the 2 results
    rhi = srge32 ? rhi1 : rhi0;
    rlo = srge32 ? rlo1 : rlo0;

    // Optional saturate on overflow
    srlt0 = sr < 0;
    rhi = srlt0 ? MAXVALUE : rhi;
    rlo = srlt0 ? MAXVALUE : rlo;

    // Create long
    res = LCREATE( rlo, rhi );

    // Deal with sign bit (ignoring whether result is signed or unsigned value)
    if (includeSign) {
    sign = ((signed int) xhi) >> 31; fill with sign bit
    sign = LCREATE( sign, sign );
    res += sign;
    res ^= sign;
    }

    return res;
    }
    */
    SDValue c11 = DAG.getConstant( 63 - 52, INTVT );
    SDValue c32 = DAG.getConstant( 32, INTVT );

    // Convert d in to 32-bit components
    SDValue d = RHS;
    SDValue x = DAG.getNode(ISDBITCAST, DL, LONGVT, d);
    SDValue xhi = DAG.getNode( (isVec) ? AMDILISD::LCOMPHI2 : AMDILISD::LCOMPHI, DL, INTVT, x );
    SDValue xlo = DAG.getNode( (isVec) ? AMDILISD::LCOMPLO2 : AMDILISD::LCOMPLO, DL, INTVT, x );

    // Generate 'normalized' mantissa
    SDValue mhi = DAG.getNode( ISD::OR, DL, INTVT,
        xhi, DAG.getConstant( 0x00100000, INTVT ) );
    mhi = DAG.getNode( ISD::SHL, DL, INTVT, mhi, c11 );
    SDValue temp = DAG.getNode( ISD::SRL, DL, INTVT,
        xlo, DAG.getConstant( 32 - (63 - 52), INTVT ) );
    mhi = DAG.getNode( ISD::OR, DL, INTVT, mhi, temp );
    SDValue mlo = DAG.getNode( ISD::SHL, DL, INTVT, xlo, c11 );

    // Compute shift right count from exponent
    SDValue e = DAG.getNode( ISD::SRL, DL, INTVT,
        xhi, DAG.getConstant( 52-32, INTVT ) );
    e = DAG.getNode( ISD::AND, DL, INTVT,
        e, DAG.getConstant( 0x7ff, INTVT ) );
    SDValue sr = DAG.getNode( ISD::SUB, DL, INTVT,
        DAG.getConstant( 1023 + 63, INTVT ), e );
    SDValue srge64 = DAG.getNode( AMDILISD::CMP, DL, INTVT,
        DAG.getConstant(CondCCodeToCC(ISD::SETGE, MVT::i32), MVT::i32),
        sr, DAG.getConstant(64, INTVT));
    SDValue srge32 = DAG.getNode( AMDILISD::CMP, DL, INTVT,
        DAG.getConstant(CondCCodeToCC(ISD::SETGE, MVT::i32), MVT::i32),
        sr, DAG.getConstant(32, INTVT));

    // Compute result for 0 <= sr < 32
    SDValue rhi0 = DAG.getNode( ISD::SRL, DL, INTVT, mhi, sr );
    SDValue rlo0 = DAG.getNode( ISD::SRL, DL, INTVT, mlo, sr );
    temp = DAG.getNode( ISD::SUB, DL, INTVT, c32, sr );
    temp = DAG.getNode( ISD::SHL, DL, INTVT, mhi, temp );
    temp = DAG.getNode( ISD::OR,  DL, INTVT, rlo0, temp );
    rlo0 = DAG.getNode( AMDILISD::CMOVLOG, DL, INTVT, sr, temp, rlo0 );

    // Compute result for 32 <= sr
    SDValue rhi1 = DAG.getConstant( 0, INTVT );
    SDValue rlo1 = DAG.getNode( AMDILISD::CMOVLOG, DL, INTVT,
        srge64, rhi1, rhi0 );

    // Pick between the 2 results
    SDValue rhi = DAG.getNode( AMDILISD::CMOVLOG, DL, INTVT,
        srge32, rhi1, rhi0 );
    SDValue rlo = DAG.getNode( AMDILISD::CMOVLOG, DL, INTVT,
        srge32, rlo1, rlo0 );

    // Create long
    SDValue res = DAG.getNode( (isVec) ? AMDILISD::LCREATE2 : AMDILISD::LCREATE, DL, LONGVT, rlo, rhi );

    // Deal with sign bit
    if (includeSign) {
      SDValue sign = DAG.getNode( ISD::SRA, DL, INTVT,
          xhi, DAG.getConstant( 31, INTVT ) );
      sign = DAG.getNode( (isVec) ? AMDILISD::LCREATE2 : AMDILISD::LCREATE, DL, LONGVT, sign, sign );
      res = DAG.getNode( ISD::ADD, DL, LONGVT, res, sign );
      res = DAG.getNode( ISD::XOR, DL, LONGVT, res, sign );
    }
    DST = res;
  }
  return DST;
}
SDValue
AMDILTargetLowering::genf64toi32(SDValue RHS, SelectionDAG &DAG,
    bool includeSign) const
{
  EVT INTVT;
  EVT LONGVT;
  DebugLoc DL = RHS.getDebugLoc();
  EVT RHSVT = RHS.getValueType();
  bool isVec = RHSVT.isVector();
  if (isVec) {
    LONGVT = EVT(MVT::getVectorVT(MVT::i64,
          RHSVT.getVectorNumElements()));
    INTVT = EVT(MVT::getVectorVT(MVT::i32,
          RHSVT.getVectorNumElements()));
  } else {
    LONGVT = EVT(MVT::i64);
    INTVT = EVT(MVT::i32);
  }
  /*
     __attribute__((always_inline)) int
     cast_f64_to_[u|i]32(double d)
     {
  // Convert d in to 32-bit components
  long x = as_long(d);
  xhi = LCOMPHI(x);
  xlo = LCOMPLO(x);

  // Generate 'normalized' mantissa
  mhi = xhi | 0x00100000; // hidden bit
  mhi <<= 11;
  temp = xlo >> (32 - 11);
  mhi |= temp

  // Compute shift right count from exponent
  e = (xhi >> (52-32)) & 0x7ff;
  sr = 1023 + 31 - e;
  srge32 = sr >= 32;

  // Compute result for 0 <= sr < 32
  res = mhi >> (sr &31);
  res = srge32 ? 0 : res;

  // Optional saturate on overflow
  srlt0 = sr < 0;
  res = srlt0 ? MAXVALUE : res;

  // Deal with sign bit (ignoring whether result is signed or unsigned value)
  if (includeSign) {
  sign = ((signed int) xhi) >> 31; fill with sign bit
  res += sign;
  res ^= sign;
  }

  return res;
  }
  */
  SDValue c11 = DAG.getConstant( 63 - 52, INTVT );

  // Convert d in to 32-bit components
  SDValue d = RHS;
  SDValue x = DAG.getNode(ISDBITCAST, DL, LONGVT, d);
  SDValue xhi = DAG.getNode( (isVec) ? AMDILISD::LCOMPHI2 : AMDILISD::LCOMPHI, DL, INTVT, x );
  SDValue xlo = DAG.getNode( (isVec) ? AMDILISD::LCOMPLO2 : AMDILISD::LCOMPLO, DL, INTVT, x );

  // Generate 'normalized' mantissa
  SDValue mhi = DAG.getNode( ISD::OR, DL, INTVT,
      xhi, DAG.getConstant( 0x00100000, INTVT ) );
  mhi = DAG.getNode( ISD::SHL, DL, INTVT, mhi, c11 );
  SDValue temp = DAG.getNode( ISD::SRL, DL, INTVT,
      xlo, DAG.getConstant( 32 - (63 - 52), INTVT ) );
  mhi = DAG.getNode( ISD::OR, DL, INTVT, mhi, temp );

  // Compute shift right count from exponent
  SDValue e = DAG.getNode( ISD::SRL, DL, INTVT,
      xhi, DAG.getConstant( 52-32, INTVT ) );
  e = DAG.getNode( ISD::AND, DL, INTVT,
      e, DAG.getConstant( 0x7ff, INTVT ) );
  SDValue sr = DAG.getNode( ISD::SUB, DL, INTVT,
      DAG.getConstant( 1023 + 31, INTVT ), e );
  SDValue srge32 = DAG.getNode( AMDILISD::CMP, DL, INTVT,
      DAG.getConstant(CondCCodeToCC(ISD::SETGE, MVT::i32), MVT::i32),
      sr, DAG.getConstant(32, INTVT));

  // Compute result for 0 <= sr < 32
  SDValue res = DAG.getNode( ISD::SRL, DL, INTVT, mhi, sr );
  res = DAG.getNode( AMDILISD::CMOVLOG, DL, INTVT,
      srge32, DAG.getConstant(0,INTVT), res );

  // Deal with sign bit
  if (includeSign) {
    SDValue sign = DAG.getNode( ISD::SRA, DL, INTVT,
        xhi, DAG.getConstant( 31, INTVT ) );
    res = DAG.getNode( ISD::ADD, DL, INTVT, res, sign );
    res = DAG.getNode( ISD::XOR, DL, INTVT, res, sign );
  }
  return res;
}

SDValue
AMDILTargetLowering::LowerFP_TO_UINT(SDValue Op, SelectionDAG &DAG) const
{
  SDValue DST;
  SDValue RHS = Op.getOperand(0);
  EVT RHSVT = RHS.getValueType();
  MVT RST = RHSVT.getScalarType().getSimpleVT();
  EVT LHSVT = Op.getValueType();
  MVT LST = LHSVT.getScalarType().getSimpleVT();
  DebugLoc DL = Op.getDebugLoc();
  const AMDILTargetMachine*
    amdtm = reinterpret_cast<const AMDILTargetMachine*>
    (&this->getTargetMachine());
  const AMDILSubtarget*
    stm = static_cast<const AMDILSubtarget*>(
        amdtm->getSubtargetImpl());
  if (RST == MVT::f64 && RHSVT.isVector()
      && stm->device()->getGeneration() > AMDILDeviceInfo::HD6XXX)  {
    // We dont support vector 64bit floating point convertions.
    for (unsigned x = 0, y = RHSVT.getVectorNumElements(); x < y; ++x) {
      SDValue op = DAG.getNode(ISD::EXTRACT_VECTOR_ELT,
          DL, RST, RHS, DAG.getTargetConstant(x, MVT::i32));
      op = DAG.getNode(ISD::FP_TO_SINT, DL, LST, op);
      if (!x) {
        DST = DAG.getNode(AMDILISD::VBUILD, DL, LHSVT, op);
      } else {
        DST = DAG.getNode(ISD::INSERT_VECTOR_ELT, DL, LHSVT,
            DST, op, DAG.getTargetConstant(x, MVT::i32));
      }

    }
  } else {
    if (RST == MVT::f64
        && LST == MVT::i32) {
      if (stm->device()->getGeneration() > AMDILDeviceInfo::HD6XXX) {
        DST = SDValue(Op.getNode(), 0);
      } else {
        DST = genf64toi32(RHS, DAG, false);
      }
    } else if (RST == MVT::f64
        && LST == MVT::i64) {
      DST = genf64toi64(RHS, DAG, false);
    } else if (RST == MVT::f64
        && (LST == MVT::i8 || LST == MVT::i16)) {
      if (stm->device()->getGeneration() > AMDILDeviceInfo::HD6XXX) {
        DST = DAG.getNode(ISD::TRUNCATE, DL, LHSVT, SDValue(Op.getNode(), 0));
      } else {
        SDValue ToInt = genf64toi32(RHS, DAG, false);
        DST = DAG.getNode(ISD::TRUNCATE, DL, LHSVT, ToInt);
      }

    } else {
      DST = SDValue(Op.getNode(), 0);
    }
  }
  return DST;
}
SDValue
AMDILTargetLowering::genu32tof64(SDValue RHS, EVT LHSVT,
    SelectionDAG &DAG) const
{
  EVT RHSVT = RHS.getValueType();
  DebugLoc DL = RHS.getDebugLoc();
  EVT INTVT;
  EVT LONGVT;
  bool isVec = RHSVT.isVector();
  if (isVec) {
    LONGVT = EVT(MVT::getVectorVT(MVT::i64,
          RHSVT.getVectorNumElements()));
    INTVT = EVT(MVT::getVectorVT(MVT::i32,
          RHSVT.getVectorNumElements()));
  } else {
    LONGVT = EVT(MVT::i64);
    INTVT = EVT(MVT::i32);
  }
  SDValue x = RHS;
  const AMDILTargetMachine*
    amdtm = reinterpret_cast<const AMDILTargetMachine*>
    (&this->getTargetMachine());
  const AMDILSubtarget*
    stm = static_cast<const AMDILSubtarget*>(
        amdtm->getSubtargetImpl());
  if (stm->calVersion() >= CAL_VERSION_SC_135) {
    // unsigned x = RHS;
    // ulong xd = (ulong)(0x4330_0000 << 32) | x;
    // double d = as_double( xd );
    // return d - 0x1.0p+52; // 0x1.0p+52 == 0x4330_0000_0000_0000
    SDValue xd = DAG.getNode( (isVec) ? AMDILISD::LCREATE2 : AMDILISD::LCREATE, DL, LONGVT, x,
        DAG.getConstant( 0x43300000, INTVT ) );
    SDValue d = DAG.getNode( ISDBITCAST, DL, LHSVT, xd );
    SDValue offsetd = DAG.getNode( ISDBITCAST, DL, LHSVT,
        DAG.getConstant( 0x4330000000000000ULL, LONGVT ) );
    return DAG.getNode( ISD::FSUB, DL, LHSVT, d, offsetd );
  } else {
    SDValue clz = genCLZu32(x, DAG);

    // Compute the exponent. 1023 is the bias, 31-clz the actual power of 2
    // Except for an input 0... which requires a 0 exponent
    SDValue exp = DAG.getNode( ISD::SUB, DL, INTVT,
        DAG.getConstant( (1023+31), INTVT), clz );
    exp = DAG.getNode( AMDILISD::CMOVLOG, DL, INTVT, x, exp, x );

    // Normalize frac
    SDValue rhi = DAG.getNode( ISD::SHL, DL, INTVT, x, clz );

    // Eliminate hidden bit
    rhi = DAG.getNode( ISD::AND, DL, INTVT,
        rhi, DAG.getConstant( 0x7fffffff, INTVT ) );

    // Pack exponent and frac
    SDValue rlo = DAG.getNode( ISD::SHL, DL, INTVT,
        rhi, DAG.getConstant( (32 - 11), INTVT ) );
    rhi = DAG.getNode( ISD::SRL, DL, INTVT,
        rhi, DAG.getConstant( 11, INTVT ) );
    exp = DAG.getNode( ISD::SHL, DL, INTVT,
        exp, DAG.getConstant( 20, INTVT ) );
    rhi = DAG.getNode( ISD::OR, DL, INTVT, rhi, exp );

    // Convert 2 x 32 in to 1 x 64, then to double precision float type
    SDValue res = DAG.getNode( (isVec) ? AMDILISD::LCREATE2 : AMDILISD::LCREATE, DL, LONGVT, rlo, rhi );
    return DAG.getNode(ISDBITCAST, DL, LHSVT, res);
  }
}
SDValue
AMDILTargetLowering::genu64tof64(SDValue RHS, EVT LHSVT,
    SelectionDAG &DAG) const
{
  EVT RHSVT = RHS.getValueType();
  DebugLoc DL = RHS.getDebugLoc();
  EVT INTVT;
  EVT LONGVT;
  bool isVec = RHSVT.isVector();
  if (isVec) {
    INTVT = EVT(MVT::getVectorVT(MVT::i32,
          RHSVT.getVectorNumElements()));
  } else {
    INTVT = EVT(MVT::i32);
  }
  LONGVT = RHSVT;
  SDValue x = RHS;
  const AMDILSubtarget *stm = reinterpret_cast<const AMDILTargetMachine*>(
      &this->getTargetMachine())->getSubtargetImpl();
  if (stm->device()->getGeneration() > AMDILDeviceInfo::HD6XXX) {
    // double dhi = (double)(as_uint2(x).y);
    // double dlo = (double)(as_uint2(x).x);
    // return mad(dhi, 0x1.0p+32, dlo)
    SDValue dhi = DAG.getNode((isVec) ? AMDILISD::LCOMPHI2 : AMDILISD::LCOMPHI, DL, INTVT, x);
    dhi = DAG.getNode(ISD::UINT_TO_FP, DL, LHSVT, dhi);
    SDValue dlo = DAG.getNode((isVec) ? AMDILISD::LCOMPLO2 : AMDILISD::LCOMPLO, DL, INTVT, x);
    dlo = DAG.getNode(ISD::UINT_TO_FP, DL, LHSVT, dlo);
    return DAG.getNode(AMDILISD::MAD, DL, LHSVT, dhi,
        DAG.getConstantFP(0x4f800000, LHSVT), dlo);
  } else if (stm->calVersion() >= CAL_VERSION_SC_135) {
    // double lo = as_double( as_ulong( 0x1.0p+52) | (u & 0xffff_ffffUL));
    // double hi = as_double( as_ulong( 0x1.0p+84) | (u >> 32));
    // return (hi - (0x1.0p+84 + 0x1.0p+52)) + lo;
    SDValue xlo = DAG.getNode( (isVec) ? AMDILISD::LCOMPLO2 : AMDILISD::LCOMPLO, DL, INTVT, x );  // x & 0xffff_ffffUL
    SDValue xd = DAG.getNode( (isVec) ? AMDILISD::LCREATE2 : AMDILISD::LCREATE, DL, LONGVT, xlo, DAG.getConstant( 0x43300000, INTVT ) );
    SDValue lo = DAG.getNode( ISDBITCAST, DL, LHSVT, xd );
    SDValue xhi = DAG.getNode((isVec) ? AMDILISD::LCOMPHI2 :  AMDILISD::LCOMPHI, DL, INTVT, x ); // x >> 32
    SDValue xe = DAG.getNode( (isVec) ? AMDILISD::LCREATE2 : AMDILISD::LCREATE, DL, LONGVT, xhi, DAG.getConstant( 0x45300000, INTVT ) );
    SDValue hi = DAG.getNode( ISDBITCAST, DL, LHSVT, xe );
    SDValue c = DAG.getNode( ISDBITCAST, DL, LHSVT,
        DAG.getConstant( 0x4530000000100000ULL, LONGVT ) );
    hi = DAG.getNode( ISD::FSUB, DL, LHSVT, hi, c );
    return DAG.getNode( ISD::FADD, DL, LHSVT, hi, lo );

  } else {
    SDValue clz = genCLZu64(x, DAG);
    SDValue xhi = DAG.getNode( (isVec) ? AMDILISD::LCOMPHI2 : AMDILISD::LCOMPHI, DL, INTVT, x );
    SDValue xlo = DAG.getNode( (isVec) ? AMDILISD::LCOMPLO2 : AMDILISD::LCOMPLO, DL, INTVT, x );

    // Compute the exponent. 1023 is the bias, 63-clz the actual power of 2
    SDValue exp = DAG.getNode( ISD::SUB, DL, INTVT,
        DAG.getConstant( (1023+63), INTVT), clz );
    SDValue mash = DAG.getNode( ISD::OR, DL, INTVT, xhi, xlo );
    exp = DAG.getNode( AMDILISD::CMOVLOG, DL, INTVT,
        mash, exp, mash );  // exp = exp, or 0 if input was 0

    // Normalize frac
    SDValue clz31 = DAG.getNode( ISD::AND, DL, INTVT,
        clz, DAG.getConstant( 31, INTVT ) );
    SDValue rshift = DAG.getNode( ISD::SUB, DL, INTVT,
        DAG.getConstant( 32, INTVT ), clz31 );
    SDValue t1 = DAG.getNode( ISD::SHL, DL, INTVT, xhi, clz31 );
    SDValue t2 = DAG.getNode( ISD::SRL, DL, INTVT, xlo, rshift );
    t2 = DAG.getNode( AMDILISD::CMOVLOG, DL, INTVT, clz31, t2, t1 );
    SDValue rhi1 = DAG.getNode( ISD::OR, DL, INTVT, t1, t2 );
    SDValue rlo1 = DAG.getNode( ISD::SHL, DL, INTVT, xlo, clz31 );
    SDValue rhi2 = DAG.getNode( ISD::SHL, DL, INTVT, xlo, clz31 );
    SDValue rlo2 = DAG.getConstant( 0, INTVT );
    SDValue clz32 = DAG.getNode( ISD::AND, DL, INTVT,
        clz, DAG.getConstant( 32, INTVT ) );
    SDValue rhi = DAG.getNode( AMDILISD::CMOVLOG, DL, INTVT,
        clz32, rhi2, rhi1 );
    SDValue rlo = DAG.getNode( AMDILISD::CMOVLOG, DL, INTVT,
        clz32, rlo2, rlo1 );

    // Eliminate hidden bit
    rhi = DAG.getNode( ISD::AND, DL, INTVT,
        rhi, DAG.getConstant( 0x7fffffff, INTVT ) );

    // Save bits needed to round properly
    SDValue round = DAG.getNode( ISD::AND, DL, INTVT,
        rlo, DAG.getConstant( 0x7ff, INTVT ) );

    // Pack exponent and frac
    rlo = DAG.getNode( ISD::SRL, DL, INTVT,
        rlo, DAG.getConstant( 11, INTVT ) );
    SDValue temp = DAG.getNode( ISD::SHL, DL, INTVT,
        rhi, DAG.getConstant( (32 - 11), INTVT ) );
    rlo = DAG.getNode( ISD::OR, DL, INTVT, rlo, temp );
    rhi = DAG.getNode( ISD::SRL, DL, INTVT,
        rhi, DAG.getConstant( 11, INTVT ) );
    exp = DAG.getNode( ISD::SHL, DL, INTVT,
        exp, DAG.getConstant( 20, INTVT ) );
    rhi = DAG.getNode( ISD::OR, DL, INTVT, rhi, exp );

    // Compute rounding bit
    SDValue even = DAG.getNode( ISD::AND, DL, INTVT,
        rlo, DAG.getConstant( 1, INTVT ) );
    SDValue grs = DAG.getNode( ISD::AND, DL, INTVT,
        round, DAG.getConstant( 0x3ff, INTVT ) );
    grs = DAG.getNode( AMDILISD::CMP, DL, INTVT,
        DAG.getConstant( CondCCodeToCC( ISD::SETNE, MVT::i32), MVT::i32),
        grs, DAG.getConstant( 0, INTVT ) ); // -1 if any GRS set, 0 if none
    grs = DAG.getNode( ISD::OR, DL, INTVT, grs, even );
    round = DAG.getNode( ISD::SRL, DL, INTVT,
        round, DAG.getConstant( 10, INTVT ) );
    round = DAG.getNode( ISD::AND, DL, INTVT, round, grs ); // 0 or 1

    // Add rounding bit
    SDValue lround = DAG.getNode( (isVec) ? AMDILISD::LCREATE2 : AMDILISD::LCREATE, DL, LONGVT,
        round, DAG.getConstant( 0, INTVT ) );
    SDValue res = DAG.getNode( (isVec) ? AMDILISD::LCREATE2 : AMDILISD::LCREATE, DL, LONGVT, rlo, rhi );
    res = DAG.getNode( ISD::ADD, DL, LONGVT, res, lround );
    return DAG.getNode(ISDBITCAST, DL, LHSVT, res);
  }
}
SDValue
AMDILTargetLowering::LowerUINT_TO_FP(SDValue Op, SelectionDAG &DAG) const
{
  SDValue RHS = Op.getOperand(0);
  EVT RHSVT = RHS.getValueType();
  MVT RST = RHSVT.getScalarType().getSimpleVT();
  EVT LHSVT = Op.getValueType();
  MVT LST = LHSVT.getScalarType().getSimpleVT();
  DebugLoc DL = Op.getDebugLoc();
  SDValue DST;
  EVT INTVT;
  EVT LONGVT;
  const AMDILTargetMachine*
    amdtm = reinterpret_cast<const AMDILTargetMachine*>
    (&this->getTargetMachine());
  const AMDILSubtarget*
    stm = static_cast<const AMDILSubtarget*>(
        amdtm->getSubtargetImpl());
  if (LST == MVT::f64 && LHSVT.isVector()
      && stm->device()->getGeneration() > AMDILDeviceInfo::HD6XXX)  {
    // We dont support vector 64bit floating point convertions.
    DST = Op;
    for (unsigned x = 0, y = LHSVT.getVectorNumElements(); x < y; ++x) {
      SDValue op = DAG.getNode(ISD::EXTRACT_VECTOR_ELT,
          DL, RST, RHS, DAG.getTargetConstant(x, MVT::i32));
      op = DAG.getNode(ISD::UINT_TO_FP, DL, LST, op);
      if (!x) {
        DST = DAG.getNode(AMDILISD::VBUILD, DL, LHSVT, op);
      } else {
        DST = DAG.getNode(ISD::INSERT_VECTOR_ELT, DL, LHSVT, DST,
            op, DAG.getTargetConstant(x, MVT::i32));
      }

    }
  } else {

    if (RST == MVT::i32
        && LST == MVT::f64) {
      if (stm->device()->getGeneration() > AMDILDeviceInfo::HD6XXX) {
        DST = SDValue(Op.getNode(), 0);
      } else {
        DST = genu32tof64(RHS, LHSVT, DAG);
      }
    } else if (RST == MVT::i64
        && LST == MVT::f64) {
      DST = genu64tof64(RHS, LHSVT, DAG);
    } else {
      DST = SDValue(Op.getNode(), 0);
    }
  }
  return DST;
}

SDValue
AMDILTargetLowering::LowerSUB(SDValue Op, SelectionDAG &DAG) const
{
  SDValue LHS = Op.getOperand(0);
  SDValue RHS = Op.getOperand(1);
  DebugLoc DL = Op.getDebugLoc();
  EVT OVT = Op.getValueType();
  SDValue DST;
  bool isVec = RHS.getValueType().isVector();
  if (OVT.getScalarType() == MVT::i64) {
    /*const AMDILTargetMachine*
      amdtm = reinterpret_cast<const AMDILTargetMachine*>
      (&this->getTargetMachine());
      const AMDILSubtarget*
      stm = dynamic_cast<const AMDILSubtarget*>(
      amdtm->getSubtargetImpl());*/
    MVT INTTY = MVT::i32;
    if (OVT == MVT::v2i64) {
      INTTY = MVT::v2i32;
    }
    SDValue LHSLO, LHSHI, RHSLO, RHSHI, INTLO, INTHI;
    // TODO: need to turn this into a bitcast of i64/v2i64 to v2i32/v4i32
    LHSLO = DAG.getNode((isVec) ? AMDILISD::LCOMPLO2 : AMDILISD::LCOMPLO, DL, INTTY, LHS);
    RHSLO = DAG.getNode((isVec) ? AMDILISD::LCOMPLO2 : AMDILISD::LCOMPLO, DL, INTTY, RHS);
    LHSHI = DAG.getNode((isVec) ? AMDILISD::LCOMPHI2 : AMDILISD::LCOMPHI, DL, INTTY, LHS);
    RHSHI = DAG.getNode((isVec) ? AMDILISD::LCOMPHI2 : AMDILISD::LCOMPHI, DL, INTTY, RHS);
    INTLO = DAG.getNode(ISD::SUB, DL, INTTY, LHSLO, RHSLO);
    INTHI = DAG.getNode(ISD::SUB, DL, INTTY, LHSHI, RHSHI);
    //TODO: need to use IBORROW on HD5XXX and later hardware
    SDValue cmp;
    if (OVT == MVT::i64) {
      cmp = DAG.getNode(AMDILISD::CMP, DL, INTTY,
          DAG.getConstant(CondCCodeToCC(ISD::SETULT, MVT::i32), MVT::i32),
          LHSLO, RHSLO);
    } else {
      SDValue cmplo;
      SDValue cmphi;
      SDValue LHSRLO = DAG.getNode(ISD::EXTRACT_VECTOR_ELT,
          DL, MVT::i32, LHSLO, DAG.getTargetConstant(0, MVT::i32));
      SDValue LHSRHI = DAG.getNode(ISD::EXTRACT_VECTOR_ELT,
          DL, MVT::i32, LHSLO, DAG.getTargetConstant(1, MVT::i32));
      SDValue RHSRLO = DAG.getNode(ISD::EXTRACT_VECTOR_ELT,
          DL, MVT::i32, RHSLO, DAG.getTargetConstant(0, MVT::i32));
      SDValue RHSRHI = DAG.getNode(ISD::EXTRACT_VECTOR_ELT,
          DL, MVT::i32, RHSLO, DAG.getTargetConstant(1, MVT::i32));
      cmplo = DAG.getNode(AMDILISD::CMP, DL, MVT::i32,
          DAG.getConstant(CondCCodeToCC(ISD::SETULT, MVT::i32), MVT::i32),
          LHSRLO, RHSRLO);
      cmphi = DAG.getNode(AMDILISD::CMP, DL, MVT::i32,
          DAG.getConstant(CondCCodeToCC(ISD::SETULT, MVT::i32), MVT::i32),
          LHSRHI, RHSRHI);
      cmp = DAG.getNode(AMDILISD::VBUILD, DL, MVT::v2i32, cmplo);
      cmp = DAG.getNode(ISD::INSERT_VECTOR_ELT, DL, MVT::v2i32,
          cmp, cmphi, DAG.getTargetConstant(1, MVT::i32));
    }
    INTHI = DAG.getNode(ISD::ADD, DL, INTTY, INTHI, cmp);
    DST = DAG.getNode((isVec) ? AMDILISD::LCREATE2 : AMDILISD::LCREATE, DL, OVT,
        INTLO, INTHI);
  } else {
    DST = SDValue(Op.getNode(), 0);
  }
  return DST;
}
SDValue
AMDILTargetLowering::LowerFDIV(SDValue Op, SelectionDAG &DAG) const
{
  EVT OVT = Op.getValueType();
  SDValue DST;
  if (OVT.getScalarType() == MVT::f64) {
    DST = LowerFDIV64(Op, DAG);
  } else if (OVT.getScalarType() == MVT::f32) {
    DST = LowerFDIV32(Op, DAG);
  } else {
    DST = SDValue(Op.getNode(), 0);
  }
  return DST;
}

SDValue
AMDILTargetLowering::LowerSDIV(SDValue Op, SelectionDAG &DAG) const
{
  EVT OVT = Op.getValueType();
  SDValue DST;
  if (OVT.getScalarType() == MVT::i64) {
    DST = LowerSDIV64(Op, DAG);
  } else if (OVT.getScalarType() == MVT::i32) {
    DST = LowerSDIV32(Op, DAG);
  } else if (OVT.getScalarType() == MVT::i16
      || OVT.getScalarType() == MVT::i8) {
    DST = LowerSDIV24(Op, DAG);
  } else {
    DST = SDValue(Op.getNode(), 0);
  }
  return DST;
}

SDValue
AMDILTargetLowering::LowerSREM(SDValue Op, SelectionDAG &DAG) const
{
  EVT OVT = Op.getValueType();
  SDValue DST;
  if (OVT.getScalarType() == MVT::i64) {
    DST = LowerSREM64(Op, DAG);
  } else if (OVT.getScalarType() == MVT::i32) {
    DST = LowerSREM32(Op, DAG);
  } else if (OVT.getScalarType() == MVT::i16) {
    DST = LowerSREM16(Op, DAG);
  } else if (OVT.getScalarType() == MVT::i8) {
    DST = LowerSREM8(Op, DAG);
  } else {
    DST = SDValue(Op.getNode(), 0);
  }
  return DST;
}

SDValue
AMDILTargetLowering::LowerUREM(SDValue Op, SelectionDAG &DAG) const
{
  EVT OVT = Op.getValueType();
  SDValue DST;
  if (OVT.getScalarType() == MVT::i64) {
    DST = LowerUREM64(Op, DAG);
  } else if (OVT.getScalarType() == MVT::i32) {
    DST = LowerUREM32(Op, DAG);
  } else if (OVT.getScalarType() == MVT::i16) {
    DST = LowerUREM16(Op, DAG);
  } else if (OVT.getScalarType() == MVT::i8) {
    DST = LowerUREM8(Op, DAG);
  } else {
    DST = SDValue(Op.getNode(), 0);
  }
  return DST;
}

SDValue
AMDILTargetLowering::LowerMUL(SDValue Op, SelectionDAG &DAG) const
{
  DebugLoc DL = Op.getDebugLoc();
  EVT OVT = Op.getValueType();
  SDValue DST;
  bool isVec = OVT.isVector();
  if (OVT.getScalarType() != MVT::i64)
  {
    DST = SDValue(Op.getNode(), 0);
  } else {
    assert(OVT.getScalarType() == MVT::i64 && "Only 64 bit mul should be lowered!");
    // TODO: This needs to be turned into a tablegen pattern
    SDValue LHS = Op.getOperand(0);
    SDValue RHS = Op.getOperand(1);

    MVT INTTY = MVT::i32;
    if (OVT == MVT::v2i64) {
      INTTY = MVT::v2i32;
    }
    // mul64(h1, l1, h0, l0)
    SDValue LHSLO = DAG.getNode((isVec) ? AMDILISD::LCOMPLO2 : AMDILISD::LCOMPLO,
        DL,
        INTTY, LHS);
    SDValue LHSHI = DAG.getNode((isVec) ? AMDILISD::LCOMPHI2 : AMDILISD::LCOMPHI,
        DL,
        INTTY, LHS);
    SDValue RHSLO = DAG.getNode((isVec) ? AMDILISD::LCOMPLO2 : AMDILISD::LCOMPLO,
        DL,
        INTTY, RHS);
    SDValue RHSHI = DAG.getNode((isVec) ? AMDILISD::LCOMPHI2 : AMDILISD::LCOMPHI,
        DL,
        INTTY, RHS);
    // MULLO_UINT_1 r1, h0, l1
    SDValue RHILLO = DAG.getNode(AMDILISD::UMUL,
        DL,
        INTTY, RHSHI, LHSLO);
    // MULLO_UINT_1 r2, h1, l0
    SDValue RLOHHI = DAG.getNode(AMDILISD::UMUL,
        DL,
        INTTY, RHSLO, LHSHI);
    // ADD_INT hr, r1, r2
    SDValue ADDHI = DAG.getNode(ISD::ADD,
        DL,
        INTTY, RHILLO, RLOHHI);
    // MULHI_UINT_1 r3, l1, l0
    SDValue RLOLLO = DAG.getNode(ISD::MULHU,
        DL,
        INTTY, RHSLO, LHSLO);
    // ADD_INT hr, hr, r3
    SDValue HIGH = DAG.getNode(ISD::ADD,
        DL,
        INTTY, ADDHI, RLOLLO);
    // MULLO_UINT_1 l3, l1, l0
    SDValue LOW = DAG.getNode(AMDILISD::UMUL,
        DL,
        INTTY, LHSLO, RHSLO);
    DST = DAG.getNode((isVec) ? AMDILISD::LCREATE2 : AMDILISD::LCREATE,
        DL,
        OVT, LOW, HIGH);
  }
  return DST;
}
SDValue
AMDILTargetLowering::LowerBUILD_VECTOR( SDValue Op, SelectionDAG &DAG ) const
{
  EVT VT = Op.getValueType();
  SDValue Nodes1;
  SDValue second;
  SDValue third;
  SDValue fourth;
  DebugLoc DL = Op.getDebugLoc();
  Nodes1 = DAG.getNode(AMDILISD::VBUILD,
      DL,
      VT, Op.getOperand(0));
#if 0
  bool allEqual = true;
  for (unsigned x = 1, y = Op.getNumOperands(); x < y; ++x) {
    if (Op.getOperand(0) != Op.getOperand(x)) {
      allEqual = false;
      break;
    }
  }
  if (allEqual) {
    return Nodes1;
  }
#endif
  switch(Op.getNumOperands()) {
    default:
    case 1:
      break;
    case 4:
      fourth = Op.getOperand(3);
      if (fourth.getOpcode() != ISD::UNDEF) {
        Nodes1 = DAG.getNode(
            ISD::INSERT_VECTOR_ELT,
            DL,
            Op.getValueType(),
            Nodes1,
            fourth,
            DAG.getConstant(7, MVT::i32));
      }
    case 3:
      third = Op.getOperand(2);
      if (third.getOpcode() != ISD::UNDEF) {
        Nodes1 = DAG.getNode(
            ISD::INSERT_VECTOR_ELT,
            DL,
            Op.getValueType(),
            Nodes1,
            third,
            DAG.getConstant(6, MVT::i32));
      }
    case 2:
      second = Op.getOperand(1);
      if (second.getOpcode() != ISD::UNDEF) {
        Nodes1 = DAG.getNode(
            ISD::INSERT_VECTOR_ELT,
            DL,
            Op.getValueType(),
            Nodes1,
            second,
            DAG.getConstant(5, MVT::i32));
      }
      break;
  };
  return Nodes1;
}

SDValue
AMDILTargetLowering::LowerINSERT_VECTOR_ELT(SDValue Op,
    SelectionDAG &DAG) const
{
  DebugLoc DL = Op.getDebugLoc();
  EVT VT = Op.getValueType();
  const SDValue *ptr = NULL;
  const ConstantSDNode *CSDN = dyn_cast<ConstantSDNode>(Op.getOperand(2));
  uint32_t swizzleNum = 0;
  SDValue DST;
  if (!VT.isVector()) {
    SDValue Res = Op.getOperand(0);
    return Res;
  }

  if (Op.getOperand(1).getOpcode() != ISD::UNDEF) {
    ptr = &Op.getOperand(1);
  } else {
    ptr = &Op.getOperand(0);
  }
  if (CSDN) {
    swizzleNum = (uint32_t)CSDN->getZExtValue();
    uint32_t mask2 = 0x04030201 & ~(0xFF << (swizzleNum * 8));
    uint32_t mask3 = 0x01010101 & (0xFF << (swizzleNum * 8));
    DST = DAG.getNode(AMDILISD::VINSERT,
        DL,
        VT,
        Op.getOperand(0),
        *ptr,
        DAG.getTargetConstant(mask2, MVT::i32),
        DAG.getTargetConstant(mask3, MVT::i32));
  } else {
    uint32_t mask2 = 0x04030201 & ~(0xFF << (swizzleNum * 8));
    uint32_t mask3 = 0x01010101 & (0xFF << (swizzleNum * 8));
    SDValue res = DAG.getNode(AMDILISD::VINSERT,
        DL, VT, Op.getOperand(0), *ptr,
        DAG.getTargetConstant(mask2, MVT::i32),
        DAG.getTargetConstant(mask3, MVT::i32));
    for (uint32_t x = 1; x < VT.getVectorNumElements(); ++x) {
      mask2 = 0x04030201 & ~(0xFF << (x * 8));
      mask3 = 0x01010101 & (0xFF << (x * 8));
      SDValue t = DAG.getNode(AMDILISD::VINSERT,
          DL, VT, Op.getOperand(0), *ptr,
          DAG.getTargetConstant(mask2, MVT::i32),
          DAG.getTargetConstant(mask3, MVT::i32));
      SDValue c = DAG.getNode(AMDILISD::CMP, DL, ptr->getValueType(),
          DAG.getConstant(AMDILCC::IL_CC_I_EQ, MVT::i32),
          Op.getOperand(2), DAG.getConstant(x, MVT::i32));
      c = DAG.getNode(AMDILISD::VBUILD, DL, Op.getValueType(), c);
      res = DAG.getNode(AMDILISD::CMOVLOG, DL, VT, c, t, res);
    }
    DST = res;
  }
  return DST;
}

SDValue
AMDILTargetLowering::LowerEXTRACT_VECTOR_ELT(SDValue Op,
    SelectionDAG &DAG) const
{
  EVT VT = Op.getValueType();
  const ConstantSDNode *CSDN = dyn_cast<ConstantSDNode>(Op.getOperand(1));
  uint64_t swizzleNum = 0;
  DebugLoc DL = Op.getDebugLoc();
  SDValue Res;
  if (!Op.getOperand(0).getValueType().isVector()) {
    Res = Op.getOperand(0);
    return Res;
  }
  if (CSDN) {
    // Static vector extraction
    swizzleNum = CSDN->getZExtValue() + 1;
    Res = DAG.getNode(AMDILISD::VEXTRACT,
        DL, VT,
        Op.getOperand(0),
        DAG.getTargetConstant(swizzleNum, MVT::i32));
  } else {
    SDValue Op1 = Op.getOperand(1);
    uint32_t vecSize = 4;
    SDValue Op0 = Op.getOperand(0);
    SDValue res = DAG.getNode(AMDILISD::VEXTRACT,
        DL, VT, Op0,
        DAG.getTargetConstant(1, MVT::i32));
    if (Op0.getValueType().isVector()) {
      vecSize = Op0.getValueType().getVectorNumElements();
    }
    for (uint32_t x = 2; x <= vecSize; ++x) {
      SDValue t = DAG.getNode(AMDILISD::VEXTRACT,
          DL, VT, Op0,
          DAG.getTargetConstant(x, MVT::i32));
      SDValue c = DAG.getNode(AMDILISD::CMP,
          DL, Op1.getValueType(),
          DAG.getConstant(AMDILCC::IL_CC_I_EQ, MVT::i32),
          Op1, DAG.getConstant(x, MVT::i32));
      res = DAG.getNode(AMDILISD::CMOVLOG, DL,
          VT, c, t, res);

    }
    Res = res;
  }
  return Res;
}

SDValue
AMDILTargetLowering::LowerEXTRACT_SUBVECTOR(SDValue Op,
    SelectionDAG &DAG) const
{
  uint32_t vecSize = Op.getValueType().getVectorNumElements();
  SDValue src = Op.getOperand(0);
  const ConstantSDNode *CSDN = dyn_cast<ConstantSDNode>(Op.getOperand(1));
  uint64_t offset = 0;
  EVT vecType = Op.getValueType().getVectorElementType();
  DebugLoc DL = Op.getDebugLoc();
  SDValue Result;
  if (CSDN) {
    offset = CSDN->getZExtValue();
    Result = DAG.getNode(ISD::EXTRACT_VECTOR_ELT,
        DL,vecType, src, DAG.getConstant(offset, MVT::i32));
    Result = DAG.getNode(AMDILISD::VBUILD, DL,
        Op.getValueType(), Result);
    for (uint32_t x = 1; x < vecSize; ++x) {
      SDValue elt = DAG.getNode(ISD::EXTRACT_VECTOR_ELT, DL, vecType,
          src, DAG.getConstant(offset + x, MVT::i32));
      if (elt.getOpcode() != ISD::UNDEF) {
        Result = DAG.getNode(ISD::INSERT_VECTOR_ELT, DL,
            Op.getValueType(), Result, elt,
            DAG.getConstant(x, MVT::i32));
      }
    }
  } else {
    SDValue idx = Op.getOperand(1);
    Result = DAG.getNode(ISD::EXTRACT_VECTOR_ELT,
        DL, vecType, src, idx);
    Result = DAG.getNode(AMDILISD::VBUILD, DL,
        Op.getValueType(), Result);
    for (uint32_t x = 1; x < vecSize; ++x) {
      idx = DAG.getNode(ISD::ADD, DL, vecType,
          idx, DAG.getConstant(1, MVT::i32));
      SDValue elt = DAG.getNode(ISD::EXTRACT_VECTOR_ELT, DL, vecType,
          src, idx);
      if (elt.getOpcode() != ISD::UNDEF) {
        Result = DAG.getNode(ISD::INSERT_VECTOR_ELT, DL,
            Op.getValueType(), Result, elt, idx);
      }
    }
  }
  return Result;
}
SDValue
AMDILTargetLowering::LowerSCALAR_TO_VECTOR(SDValue Op,
    SelectionDAG &DAG) const
{
  SDValue Res = DAG.getNode(AMDILISD::VBUILD,
      Op.getDebugLoc(),
      Op.getValueType(),
      Op.getOperand(0));
  return Res;
}
SDValue
AMDILTargetLowering::LowerSELECT(SDValue Op, SelectionDAG &DAG) const
{
  SDValue Cond = Op.getOperand(0);
  SDValue LHS = Op.getOperand(1);
  SDValue RHS = Op.getOperand(2);
  DebugLoc DL = Op.getDebugLoc();
  Cond = getConversionNode(DAG, Cond, Op, true);
  Cond = DAG.getNode(AMDILISD::CMOVLOG,
      DL,
      Op.getValueType(), Cond, LHS, RHS);
  return Cond;
}
SDValue
AMDILTargetLowering::LowerSETCC(SDValue Op, SelectionDAG &DAG) const
{
  SDValue Cond;
  SDValue LHS = Op.getOperand(0);
  SDValue RHS = Op.getOperand(1);
  SDValue CC  = Op.getOperand(2);
  DebugLoc DL = Op.getDebugLoc();
  ISD::CondCode SetCCOpcode = cast<CondCodeSDNode>(CC)->get();
  unsigned int AMDILCC = CondCCodeToCC(
      SetCCOpcode,
      LHS.getValueType().getSimpleVT().SimpleTy);
  assert((AMDILCC != AMDILCC::COND_ERROR) && "Invalid SetCC!");
  Cond = DAG.getNode(
      ISD::SELECT_CC,
      Op.getDebugLoc(),
      LHS.getValueType(),
      LHS, RHS,
      DAG.getConstant(-1, MVT::i32),
      DAG.getConstant(0, MVT::i32),
      CC);
  Cond = getConversionNode(DAG, Cond, Op, true);
  Cond = DAG.getNode(
      ISD::AND,
      DL,
      Cond.getValueType(),
      DAG.getConstant(1, Cond.getValueType()),
      Cond);
  return Cond;
}

SDValue
AMDILTargetLowering::LowerSIGN_EXTEND_INREG(SDValue Op, SelectionDAG &DAG) const
{
  SDValue Data = Op.getOperand(0);
  VTSDNode *BaseType = cast<VTSDNode>(Op.getOperand(1));
  DebugLoc DL = Op.getDebugLoc();
  EVT DVT = Data.getValueType();
  EVT BVT = BaseType->getVT();
  unsigned baseBits = BVT.getScalarType().getSizeInBits();
  unsigned srcBits = DVT.isSimple() ? DVT.getScalarType().getSizeInBits() : 1;
  unsigned shiftBits = srcBits - baseBits;
  if (srcBits < 32) {
    // If the op is less than 32 bits, then it needs to extend to 32bits
    // so it can properly keep the upper bits valid.
    EVT IVT = genIntType(32, DVT.isVector() ? DVT.getVectorNumElements() : 1);
    Data = DAG.getNode(ISD::ZERO_EXTEND, DL, IVT, Data);
    shiftBits = 32 - baseBits;
    DVT = IVT;
  }
  SDValue Shift = DAG.getConstant(shiftBits, DVT);
  // Shift left by 'Shift' bits.
  Data = DAG.getNode(ISD::SHL, DL, DVT, Data, Shift);
  // Signed shift Right by 'Shift' bits.
  Data = DAG.getNode(ISD::SRA, DL, DVT, Data, Shift);
  if (srcBits < 32) {
    // Once the sign extension is done, the op needs to be converted to
    // its original type.
    Data = DAG.getSExtOrTrunc(Data, DL, Op.getOperand(0).getValueType());
  }
  return Data;
}
EVT
AMDILTargetLowering::genIntType(uint32_t size, uint32_t numEle) const
{
  int iSize = (size * numEle);
  int vEle = (iSize >> ((size == 64) ? 6 : 5));
  if (!vEle) {
    vEle = 1;
  }
  if (size == 64) {
    if (vEle == 1) {
      return EVT(MVT::i64);
    } else {
      return EVT(MVT::getVectorVT(MVT::i64, vEle));
    }
  } else {
    if (vEle == 1) {
      return EVT(MVT::i32);
    } else {
      return EVT(MVT::getVectorVT(MVT::i32, vEle));
    }
  }
}

SDValue
AMDILTargetLowering::LowerDYNAMIC_STACKALLOC(SDValue Op,
    SelectionDAG &DAG) const
{
  SDValue Chain = Op.getOperand(0);
  SDValue Size = Op.getOperand(1);
  unsigned int SPReg = AMDIL::SP;
  DebugLoc DL = Op.getDebugLoc();
  SDValue SP = DAG.getCopyFromReg(Chain,
      DL,
      SPReg, MVT::i32);
  SDValue NewSP = DAG.getNode(ISD::ADD,
      DL,
      MVT::i32, SP, Size);
  Chain = DAG.getCopyToReg(SP.getValue(1),
      DL,
      SPReg, NewSP);
  SDValue Ops[2] = {NewSP, Chain};
  Chain = DAG.getMergeValues(Ops, 2 ,DL);
  return Chain;
}
SDValue
AMDILTargetLowering::LowerBRCOND(SDValue Op, SelectionDAG &DAG) const
{
  SDValue Chain = Op.getOperand(0);
  SDValue Cond  = Op.getOperand(1);
  SDValue Jump  = Op.getOperand(2);
  SDValue Result;
  Result = DAG.getNode(
      AMDILISD::BRANCH_COND,
      Op.getDebugLoc(),
      Op.getValueType(),
      Chain, Jump, Cond);
  return Result;
}

SDValue
AMDILTargetLowering::LowerBR_CC(SDValue Op, SelectionDAG &DAG) const
{
  SDValue Chain = Op.getOperand(0);
  SDValue CC = Op.getOperand(1);
  SDValue LHS   = Op.getOperand(2);
  SDValue RHS   = Op.getOperand(3);
  SDValue JumpT  = Op.getOperand(4);
  SDValue CmpValue;
  SDValue Result;
  CmpValue = DAG.getNode(
      ISD::SELECT_CC,
      Op.getDebugLoc(),
      LHS.getValueType(),
      LHS, RHS,
      DAG.getConstant(-1, MVT::i32),
      DAG.getConstant(0, MVT::i32),
      CC);
  Result = DAG.getNode(
      AMDILISD::BRANCH_COND,
      CmpValue.getDebugLoc(),
      MVT::Other, Chain,
      JumpT, CmpValue);
  return Result;
}

SDValue
AMDILTargetLowering::LowerFP_ROUND(SDValue Op, SelectionDAG &DAG) const
{
  SDValue Result = DAG.getNode(
      AMDILISD::DP_TO_FP,
      Op.getDebugLoc(),
      Op.getValueType(),
      Op.getOperand(0),
      Op.getOperand(1));
  return Result;
}

SDValue
AMDILTargetLowering::LowerCONCAT_VECTORS(SDValue Op, SelectionDAG &DAG) const
{
  SDValue Result = DAG.getNode(
      AMDILISD::VCONCAT,
      Op.getDebugLoc(),
      Op.getValueType(),
      Op.getOperand(0),
      Op.getOperand(1));
  return Result;
}
// LowerRET - Lower an ISD::RET node.
SDValue
AMDILTargetLowering::LowerReturn(SDValue Chain,
    CallingConv::ID CallConv, bool isVarArg,
    const SmallVectorImpl<ISD::OutputArg> &Outs,
    const SmallVectorImpl<SDValue> &OutVals,
    DebugLoc dl, SelectionDAG &DAG)
const
{
  //MachineFunction& MF = DAG.getMachineFunction();
  // CCValAssign - represent the assignment of the return value
  // to a location
  SmallVector<CCValAssign, 16> RVLocs;

  // CCState - Info about the registers and stack slot
  CCState CCInfo(CallConv, isVarArg, DAG.getMachineFunction(),
                 getTargetMachine(), RVLocs, *DAG.getContext());

  // Analyze return values of ISD::RET
  CCInfo.AnalyzeReturn(Outs, RetCC_AMDIL32);
  // If this is the first return lowered for this function, add
  // the regs to the liveout set for the function
  MachineRegisterInfo &MRI = DAG.getMachineFunction().getRegInfo();
  for (unsigned int i = 0, e = RVLocs.size(); i != e; ++i) {
    if (RVLocs[i].isRegLoc() && !MRI.isLiveOut(RVLocs[i].getLocReg())) {
      MRI.addLiveOut(RVLocs[i].getLocReg());
    }
  }
  // FIXME: implement this when tail call is implemented
  // Chain = GetPossiblePreceedingTailCall(Chain, AMDILISD::TAILCALL);
  // both x86 and ppc implement this in ISelLowering

  // Regular return here
  SDValue Flag;
  SmallVector<SDValue, 6> RetOps;
  RetOps.push_back(Chain);
  RetOps.push_back(DAG.getConstant(0/*getBytesToPopOnReturn()*/, MVT::i32));
  for (unsigned int i = 0, e = RVLocs.size(); i != e; ++i) {
    CCValAssign &VA = RVLocs[i];
    SDValue ValToCopy = OutVals[i];
    assert(VA.isRegLoc() && "Can only return in registers!");
    // ISD::Ret => ret chain, (regnum1, val1), ...
    // So i * 2 + 1 index only the regnums
    Chain = DAG.getCopyToReg(Chain,
        dl,
        VA.getLocReg(),
        ValToCopy,
        Flag);
    // guarantee that all emitted copies are stuck together
    // avoiding something bad
    Flag = Chain.getValue(1);
  }
  /*if (MF.getFunction()->hasStructRetAttr()) {
    assert(0 && "Struct returns are not yet implemented!");
  // Both MIPS and X86 have this
  }*/
  RetOps[0] = Chain;
  if (Flag.getNode())
    RetOps.push_back(Flag);

  Flag = DAG.getNode(AMDILISD::RET_FLAG,
      dl,
      MVT::Other, &RetOps[0], RetOps.size());
  return Flag;
}

unsigned int
AMDILTargetLowering::getFunctionAlignment(const Function *) const
{
  return 0;
}

void
AMDILTargetLowering::setPrivateData(MachineBasicBlock *BB,
    MachineBasicBlock::iterator &BBI,
    DebugLoc *DL, const TargetInstrInfo *TII) const
{
  mBB = BB;
  mBBI = BBI;
  mDL = DL;
  mTII = TII;
}
uint32_t
AMDILTargetLowering::genVReg(uint32_t regType) const
{
  return mBB->getParent()->getRegInfo().createVirtualRegister(
      getTargetMachine().getRegisterInfo()->getRegClass(regType));
}

MachineInstrBuilder
AMDILTargetLowering::generateMachineInst(uint32_t opcode, uint32_t dst) const
{
  return BuildMI(*mBB, mBBI, *mDL, mTII->get(opcode), dst);
}

MachineInstrBuilder
AMDILTargetLowering::generateMachineInst(uint32_t opcode, uint32_t dst,
    uint32_t src1) const
{
  return generateMachineInst(opcode, dst).addReg(src1);
}

MachineInstrBuilder
AMDILTargetLowering::generateMachineInst(uint32_t opcode, uint32_t dst,
    uint32_t src1, uint32_t src2) const
{
  return generateMachineInst(opcode, dst, src1).addReg(src2);
}

MachineInstrBuilder
AMDILTargetLowering::generateMachineInst(uint32_t opcode, uint32_t dst,
    uint32_t src1, uint32_t src2, uint32_t src3) const
{
  return generateMachineInst(opcode, dst, src1, src2).addReg(src3);
}


SDValue
AMDILTargetLowering::LowerSDIV24(SDValue Op, SelectionDAG &DAG) const
{
  DebugLoc DL = Op.getDebugLoc();
  EVT OVT = Op.getValueType();
  SDValue LHS = Op.getOperand(0);
  SDValue RHS = Op.getOperand(1);
  MVT INTTY;
  MVT FLTTY;
  if (!OVT.isVector()) {
    INTTY = MVT::i32;
    FLTTY = MVT::f32;
  } else if (OVT.getVectorNumElements() == 2) {
    INTTY = MVT::v2i32;
    FLTTY = MVT::v2f32;
  } else if (OVT.getVectorNumElements() == 4) {
    INTTY = MVT::v4i32;
    FLTTY = MVT::v4f32;
  }
  unsigned bitsize = OVT.getScalarType().getSizeInBits();
  // char|short jq = ia ^ ib;
  SDValue jq = DAG.getNode(ISD::XOR, DL, OVT, LHS, RHS);

  // jq = jq >> (bitsize - 2)
  jq = DAG.getNode(ISD::SRA, DL, OVT, jq, DAG.getConstant(bitsize - 2, OVT)); 

  // jq = jq | 0x1
  jq = DAG.getNode(ISD::OR, DL, OVT, jq, DAG.getConstant(1, OVT));

  // jq = (int)jq
  jq = DAG.getSExtOrTrunc(jq, DL, INTTY);

  // int ia = (int)LHS;
  SDValue ia = DAG.getSExtOrTrunc(LHS, DL, INTTY);

  // int ib, (int)RHS;
  SDValue ib = DAG.getSExtOrTrunc(RHS, DL, INTTY);

  // float fa = (float)ia;
  SDValue fa = DAG.getNode(ISD::SINT_TO_FP, DL, FLTTY, ia);

  // float fb = (float)ib;
  SDValue fb = DAG.getNode(ISD::SINT_TO_FP, DL, FLTTY, ib);

  // float fq = native_divide(fa, fb);
  SDValue fq = DAG.getNode(AMDILISD::DIV_INF, DL, FLTTY, fa, fb);

  // fq = trunc(fq);
  fq = DAG.getNode(ISD::FTRUNC, DL, FLTTY, fq);

  // float fqneg = -fq;
  SDValue fqneg = DAG.getNode(ISD::FNEG, DL, FLTTY, fq);

  // float fr = mad(fqneg, fb, fa);
  SDValue fr = DAG.getNode(AMDILISD::MAD, DL, FLTTY, fqneg, fb, fa);

  // int iq = (int)fq;
  SDValue iq = DAG.getNode(ISD::FP_TO_SINT, DL, INTTY, fq);

  // fr = fabs(fr);
  fr = DAG.getNode(ISD::FABS, DL, FLTTY, fr);

  // fb = fabs(fb);
  fb = DAG.getNode(ISD::FABS, DL, FLTTY, fb);

  // int cv = fr >= fb;
  SDValue cv;
  if (INTTY == MVT::i32) {
    cv = DAG.getSetCC(DL, INTTY, fr, fb, ISD::SETOGE);
  } else {
    cv = DAG.getSetCC(DL, INTTY, fr, fb, ISD::SETOGE);
  }
  // jq = (cv ? jq : 0);
  jq = DAG.getNode(AMDILISD::CMOVLOG, DL, OVT, cv, jq, 
      DAG.getConstant(0, OVT));
  // dst = iq + jq;
  iq = DAG.getSExtOrTrunc(iq, DL, OVT);
  iq = DAG.getNode(ISD::ADD, DL, OVT, iq, jq);
  return iq;
}

SDValue
AMDILTargetLowering::LowerSDIV32(SDValue Op, SelectionDAG &DAG) const
{
  DebugLoc DL = Op.getDebugLoc();
  EVT OVT = Op.getValueType();
  SDValue LHS = Op.getOperand(0);
  SDValue RHS = Op.getOperand(1);
  // The LowerSDIV32 function generates equivalent to the following IL.
  // mov r0, LHS
  // mov r1, RHS
  // ilt r10, r0, 0
  // ilt r11, r1, 0
  // iadd r0, r0, r10
  // iadd r1, r1, r11
  // ixor r0, r0, r10
  // ixor r1, r1, r11
  // udiv r0, r0, r1
  // ixor r10, r10, r11
  // iadd r0, r0, r10
  // ixor DST, r0, r10

  // mov r0, LHS
  SDValue r0 = LHS;

  // mov r1, RHS
  SDValue r1 = RHS;

  // ilt r10, r0, 0
  SDValue r10 = DAG.getSelectCC(DL,
      r0, DAG.getConstant(0, OVT),
      DAG.getConstant(-1, MVT::i32),
      DAG.getConstant(0, MVT::i32),
      ISD::SETLT);

  // ilt r11, r1, 0
  SDValue r11 = DAG.getSelectCC(DL,
      r1, DAG.getConstant(0, OVT),
      DAG.getConstant(-1, MVT::i32),
      DAG.getConstant(0, MVT::i32),
      ISD::SETLT);

  // iadd r0, r0, r10
  r0 = DAG.getNode(ISD::ADD, DL, OVT, r0, r10);

  // iadd r1, r1, r11
  r1 = DAG.getNode(ISD::ADD, DL, OVT, r1, r11);

  // ixor r0, r0, r10
  r0 = DAG.getNode(ISD::XOR, DL, OVT, r0, r10);

  // ixor r1, r1, r11
  r1 = DAG.getNode(ISD::XOR, DL, OVT, r1, r11);

  // udiv r0, r0, r1
  r0 = DAG.getNode(ISD::UDIV, DL, OVT, r0, r1);

  // ixor r10, r10, r11
  r10 = DAG.getNode(ISD::XOR, DL, OVT, r10, r11);

  // iadd r0, r0, r10
  r0 = DAG.getNode(ISD::ADD, DL, OVT, r0, r10);

  // ixor DST, r0, r10
  SDValue DST = DAG.getNode(ISD::XOR, DL, OVT, r0, r10); 
  return DST;
}

SDValue
AMDILTargetLowering::LowerSDIV64(SDValue Op, SelectionDAG &DAG) const
{
  return SDValue(Op.getNode(), 0);
}

SDValue
AMDILTargetLowering::LowerUDIV24(SDValue Op, SelectionDAG &DAG) const
{
  DebugLoc DL = Op.getDebugLoc();
  EVT OVT = Op.getValueType();
  SDValue LHS = Op.getOperand(0);
  SDValue RHS = Op.getOperand(1);
  MVT INTTY;
  MVT FLTTY;
  if (!OVT.isVector()) {
    INTTY = MVT::i32;
    FLTTY = MVT::f32;
  } else if (OVT.getVectorNumElements() == 2) {
    INTTY = MVT::v2i32;
    FLTTY = MVT::v2f32;
  } else if (OVT.getVectorNumElements() == 4) {
    INTTY = MVT::v4i32;
    FLTTY = MVT::v4f32;
  }

  // The LowerUDIV24 function implements the following CL.
  // int ia = (int)LHS
  // float fa = (float)ia
  // int ib = (int)RHS
  // float fb = (float)ib
  // float fq = native_divide(fa, fb)
  // fq = trunc(fq)
  // float t = mad(fq, fb, fb)
  // int iq = (int)fq - (t <= fa)
  // return (type)iq

  // int ia = (int)LHS
  SDValue ia = DAG.getZExtOrTrunc(LHS, DL, INTTY);

  // float fa = (float)ia
  SDValue fa = DAG.getNode(ISD::SINT_TO_FP, DL, FLTTY, ia);

  // int ib = (int)RHS
  SDValue ib = DAG.getZExtOrTrunc(RHS, DL, INTTY);

  // float fb = (float)ib
  SDValue fb = DAG.getNode(ISD::SINT_TO_FP, DL, FLTTY, ib);

  // float fq = native_divide(fa, fb)
  SDValue fq = DAG.getNode(AMDILISD::DIV_INF, DL, FLTTY, fa, fb);

  // fq = trunc(fq)
  fq = DAG.getNode(ISD::FTRUNC, DL, FLTTY, fq);

  // float t = mad(fq, fb, fb)
  SDValue t = DAG.getNode(AMDILISD::MAD, DL, FLTTY, fq, fb, fb);

  // int iq = (int)fq - (t <= fa) // This is sub and not add because GPU returns 0, -1
  SDValue iq;
  fq = DAG.getNode(ISD::FP_TO_SINT, DL, INTTY, fq);
  if (INTTY == MVT::i32) {
    iq = DAG.getSetCC(DL, INTTY, t, fa, ISD::SETOLE);
  } else {
    iq = DAG.getSetCC(DL, INTTY, t, fa, ISD::SETOLE);
  }
  iq = DAG.getNode(ISD::ADD, DL, INTTY, fq, iq);


  // return (type)iq
  iq = DAG.getZExtOrTrunc(iq, DL, OVT);
  return iq;

}

SDValue
AMDILTargetLowering::LowerSREM8(SDValue Op, SelectionDAG &DAG) const
{
  DebugLoc DL = Op.getDebugLoc();
  EVT OVT = Op.getValueType();
  MVT INTTY = MVT::i32;
  if (OVT == MVT::v2i8) {
    INTTY = MVT::v2i32;
  } else if (OVT == MVT::v4i8) {
    INTTY = MVT::v4i32;
  }
  SDValue LHS = DAG.getSExtOrTrunc(Op.getOperand(0), DL, INTTY);
  SDValue RHS = DAG.getSExtOrTrunc(Op.getOperand(1), DL, INTTY);
  LHS = DAG.getNode(ISD::SREM, DL, INTTY, LHS, RHS);
  LHS = DAG.getSExtOrTrunc(LHS, DL, OVT);
  return LHS;
}

SDValue
AMDILTargetLowering::LowerSREM16(SDValue Op, SelectionDAG &DAG) const
{
  DebugLoc DL = Op.getDebugLoc();
  EVT OVT = Op.getValueType();
  MVT INTTY = MVT::i32;
  if (OVT == MVT::v2i16) {
    INTTY = MVT::v2i32;
  } else if (OVT == MVT::v4i16) {
    INTTY = MVT::v4i32;
  }
  SDValue LHS = DAG.getSExtOrTrunc(Op.getOperand(0), DL, INTTY);
  SDValue RHS = DAG.getSExtOrTrunc(Op.getOperand(1), DL, INTTY);
  LHS = DAG.getNode(ISD::SREM, DL, INTTY, LHS, RHS);
  LHS = DAG.getSExtOrTrunc(LHS, DL, OVT);
  return LHS;
}

SDValue
AMDILTargetLowering::LowerSREM32(SDValue Op, SelectionDAG &DAG) const
{
  DebugLoc DL = Op.getDebugLoc();
  EVT OVT = Op.getValueType();
  SDValue LHS = Op.getOperand(0);
  SDValue RHS = Op.getOperand(1);
  // The LowerSREM32 function generates equivalent to the following IL.
  // mov r0, LHS
  // mov r1, RHS
  // ilt r10, r0, 0
  // ilt r11, r1, 0
  // iadd r0, r0, r10
  // iadd r1, r1, r11
  // ixor r0, r0, r10
  // ixor r1, r1, r11
  // udiv r20, r0, r1
  // umul r20, r20, r1
  // sub r0, r0, r20
  // iadd r0, r0, r10
  // ixor DST, r0, r10

  // mov r0, LHS
  SDValue r0 = LHS;

  // mov r1, RHS
  SDValue r1 = RHS;

  // ilt r10, r0, 0
  SDValue r10 = DAG.getNode(AMDILISD::CMP, DL, OVT,
      DAG.getConstant(CondCCodeToCC(ISD::SETLT, MVT::i32), MVT::i32),
      r0, DAG.getConstant(0, OVT));

  // ilt r11, r1, 0
  SDValue r11 = DAG.getNode(AMDILISD::CMP, DL, OVT, 
      DAG.getConstant(CondCCodeToCC(ISD::SETLT, MVT::i32), MVT::i32),
      r1, DAG.getConstant(0, OVT));

  // iadd r0, r0, r10
  r0 = DAG.getNode(ISD::ADD, DL, OVT, r0, r10);

  // iadd r1, r1, r11
  r1 = DAG.getNode(ISD::ADD, DL, OVT, r1, r11);

  // ixor r0, r0, r10
  r0 = DAG.getNode(ISD::XOR, DL, OVT, r0, r10);

  // ixor r1, r1, r11
  r1 = DAG.getNode(ISD::XOR, DL, OVT, r1, r11);

  // udiv r20, r0, r1
  SDValue r20 = DAG.getNode(ISD::UREM, DL, OVT, r0, r1);

  // umul r20, r20, r1
  r20 = DAG.getNode(AMDILISD::UMUL, DL, OVT, r20, r1);

  // sub r0, r0, r20
  r0 = DAG.getNode(ISD::SUB, DL, OVT, r0, r20);

  // iadd r0, r0, r10
  r0 = DAG.getNode(ISD::ADD, DL, OVT, r0, r10);

  // ixor DST, r0, r10
  SDValue DST = DAG.getNode(ISD::XOR, DL, OVT, r0, r10); 
  return DST;
}

SDValue
AMDILTargetLowering::LowerSREM64(SDValue Op, SelectionDAG &DAG) const
{
  return SDValue(Op.getNode(), 0);
}

SDValue
AMDILTargetLowering::LowerUREM8(SDValue Op, SelectionDAG &DAG) const
{
  DebugLoc DL = Op.getDebugLoc();
  EVT OVT = Op.getValueType();
  MVT INTTY = MVT::i32;
  if (OVT == MVT::v2i8) {
    INTTY = MVT::v2i32;
  } else if (OVT == MVT::v4i8) {
    INTTY = MVT::v4i32;
  }
  SDValue LHS = Op.getOperand(0);
  SDValue RHS = Op.getOperand(1);
  // The LowerUREM8 function generates equivalent to the following IL.
  // mov r0, as_u32(LHS)
  // mov r1, as_u32(RHS)
  // and r10, r0, 0xFF
  // and r11, r1, 0xFF
  // cmov_logical r3, r11, r11, 0x1
  // udiv r3, r10, r3
  // cmov_logical r3, r11, r3, 0
  // umul r3, r3, r11
  // sub r3, r10, r3
  // and as_u8(DST), r3, 0xFF

  // mov r0, as_u32(LHS)
  SDValue r0 = DAG.getSExtOrTrunc(LHS, DL, INTTY);

  // mov r1, as_u32(RHS)
  SDValue r1 = DAG.getSExtOrTrunc(RHS, DL, INTTY);

  // and r10, r0, 0xFF
  SDValue r10 = DAG.getNode(ISD::AND, DL, INTTY, r0, 
      DAG.getConstant(0xFF, INTTY));

  // and r11, r1, 0xFF
  SDValue r11 = DAG.getNode(ISD::AND, DL, INTTY, r1, 
      DAG.getConstant(0xFF, INTTY));

  // cmov_logical r3, r11, r11, 0x1
  SDValue r3 = DAG.getNode(AMDILISD::CMOVLOG, DL, INTTY, r11, r11,
      DAG.getConstant(0x01, INTTY));

  // udiv r3, r10, r3
  r3 = DAG.getNode(ISD::UREM, DL, INTTY, r10, r3);

  // cmov_logical r3, r11, r3, 0
  r3 = DAG.getNode(AMDILISD::CMOVLOG, DL, INTTY, r11, r3,
      DAG.getConstant(0, INTTY));

  // umul r3, r3, r11
  r3 = DAG.getNode(AMDILISD::UMUL, DL, INTTY, r3, r11);

  // sub r3, r10, r3
  r3 = DAG.getNode(ISD::SUB, DL, INTTY, r10, r3);

  // and as_u8(DST), r3, 0xFF
  SDValue DST = DAG.getNode(ISD::AND, DL, INTTY, r3,
      DAG.getConstant(0xFF, INTTY));
  DST = DAG.getZExtOrTrunc(DST, DL, OVT);
  return DST;
}

SDValue
AMDILTargetLowering::LowerUREM16(SDValue Op, SelectionDAG &DAG) const
{
  DebugLoc DL = Op.getDebugLoc();
  EVT OVT = Op.getValueType();
  MVT INTTY = MVT::i32;
  if (OVT == MVT::v2i16) {
    INTTY = MVT::v2i32;
  } else if (OVT == MVT::v4i16) {
    INTTY = MVT::v4i32;
  }
  SDValue LHS = Op.getOperand(0);
  SDValue RHS = Op.getOperand(1);
  // The LowerUREM16 function generatest equivalent to the following IL.
  // mov r0, LHS
  // mov r1, RHS
  // DIV = LowerUDIV16(LHS, RHS)
  // and r10, r0, 0xFFFF
  // and r11, r1, 0xFFFF
  // cmov_logical r3, r11, r11, 0x1
  // udiv as_u16(r3), as_u32(r10), as_u32(r3)
  // and r3, r3, 0xFFFF
  // cmov_logical r3, r11, r3, 0
  // umul r3, r3, r11
  // sub r3, r10, r3
  // and DST, r3, 0xFFFF

  // mov r0, LHS
  SDValue r0 = LHS;

  // mov r1, RHS
  SDValue r1 = RHS;

  // and r10, r0, 0xFFFF
  SDValue r10 = DAG.getNode(ISD::AND, DL, OVT, r0, 
      DAG.getConstant(0xFFFF, OVT));

  // and r11, r1, 0xFFFF
  SDValue r11 = DAG.getNode(ISD::AND, DL, OVT, r1, 
      DAG.getConstant(0xFFFF, OVT));

  // cmov_logical r3, r11, r11, 0x1
  SDValue r3 = DAG.getNode(AMDILISD::CMOVLOG, DL, OVT, r11, r11,
      DAG.getConstant(0x01, OVT));

  // udiv as_u16(r3), as_u32(r10), as_u32(r3)
  r10 = DAG.getZExtOrTrunc(r10, DL, INTTY);
  r3 = DAG.getZExtOrTrunc(r3, DL, INTTY);
  r3 = DAG.getNode(ISD::UREM, DL, INTTY, r10, r3);
  r3 = DAG.getZExtOrTrunc(r3, DL, OVT);
  r10 = DAG.getZExtOrTrunc(r10, DL, OVT);

  // and r3, r3, 0xFFFF
  r3 = DAG.getNode(ISD::AND, DL, OVT, r3, 
      DAG.getConstant(0xFFFF, OVT));

  // cmov_logical r3, r11, r3, 0
  r3 = DAG.getNode(AMDILISD::CMOVLOG, DL, OVT, r11, r3,
      DAG.getConstant(0, OVT));
  // umul r3, r3, r11
  r3 = DAG.getNode(AMDILISD::UMUL, DL, OVT, r3, r11);

  // sub r3, r10, r3
  r3 = DAG.getNode(ISD::SUB, DL, OVT, r10, r3);

  // and DST, r3, 0xFFFF
  SDValue DST = DAG.getNode(ISD::AND, DL, OVT, r3,
      DAG.getConstant(0xFFFF, OVT));
  return DST;
}

SDValue
AMDILTargetLowering::LowerUREM32(SDValue Op, SelectionDAG &DAG) const
{
  DebugLoc DL = Op.getDebugLoc();
  EVT OVT = Op.getValueType();
  SDValue LHS = Op.getOperand(0);
  SDValue RHS = Op.getOperand(1);
  // The LowerUREM32 function generates equivalent to the following IL.
  // udiv r20, LHS, RHS
  // umul r20, r20, RHS
  // sub DST, LHS, r20

  // udiv r20, LHS, RHS
  SDValue r20 = DAG.getNode(ISD::UDIV, DL, OVT, LHS, RHS);

  // umul r20, r20, RHS
  r20 = DAG.getNode(AMDILISD::UMUL, DL, OVT, r20, RHS);

  // sub DST, LHS, r20
  SDValue DST = DAG.getNode(ISD::SUB, DL, OVT, LHS, r20);
  return DST;
}

SDValue
AMDILTargetLowering::LowerUREM64(SDValue Op, SelectionDAG &DAG) const
{
  return SDValue(Op.getNode(), 0);
}


SDValue
AMDILTargetLowering::LowerFDIV32(SDValue Op, SelectionDAG &DAG) const
{
  DebugLoc DL = Op.getDebugLoc();
  EVT OVT = Op.getValueType();
  MVT INTTY = MVT::i32;
  if (OVT == MVT::v2f32) {
    INTTY = MVT::v2i32;
  } else if (OVT == MVT::v4f32) {
    INTTY = MVT::v4i32;
  }
  SDValue LHS = Op.getOperand(0);
  SDValue RHS = Op.getOperand(1);
  SDValue DST;
  const AMDILSubtarget *stm = reinterpret_cast<const AMDILTargetMachine*>(
      &this->getTargetMachine())->getSubtargetImpl();
  if (stm->device()->getGeneration() == AMDILDeviceInfo::HD4XXX) {
    // TODO: This doesn't work for vector types yet
    // The LowerFDIV32 function generates equivalent to the following
    // IL:
    // mov r20, as_int(LHS)
    // mov r21, as_int(RHS)
    // and r30, r20, 0x7f800000
    // and r31, r20, 0x807FFFFF
    // and r32, r21, 0x7f800000
    // and r33, r21, 0x807FFFFF
    // ieq r40, r30, 0x7F800000
    // ieq r41, r31, 0x7F800000
    // ieq r42, r32, 0
    // ieq r43, r33, 0
    // and r50, r20, 0x80000000
    // and r51, r21, 0x80000000
    // ior r32, r32, 0x3f800000
    // ior r33, r33, 0x3f800000
    // cmov_logical r32, r42, r50, r32
    // cmov_logical r33, r43, r51, r33
    // cmov_logical r32, r40, r20, r32
    // cmov_logical r33, r41, r21, r33
    // ior r50, r40, r41
    // ior r51, r42, r43
    // ior r50, r50, r51
    // inegate r52, r31
    // iadd r30, r30, r52
    // cmov_logical r30, r50, 0, r30
    // div_zeroop(infinity) r21, 1.0, r33
    // mul_ieee r20, r32, r21
    // and r22, r20, 0x7FFFFFFF
    // and r23, r20, 0x80000000
    // ishr r60, r22, 0x00000017
    // ishr r61, r30, 0x00000017
    // iadd r20, r20, r30
    // iadd r21, r22, r30
    // iadd r60, r60, r61
    // ige r42, 0, R60
    // ior r41, r23, 0x7F800000
    // ige r40, r60, 0x000000FF
    // cmov_logical r40, r50, 0, r40
    // cmov_logical r20, r42, r23, r20
    // cmov_logical DST, r40, r41, r20
    // as_float(DST)

    // mov r20, as_int(LHS)
    SDValue R20 = DAG.getNode(ISDBITCAST, DL, INTTY, LHS);

    // mov r21, as_int(RHS)
    SDValue R21 = DAG.getNode(ISDBITCAST, DL, INTTY, RHS);

    // and r30, r20, 0x7f800000
    SDValue R30 = DAG.getNode(ISD::AND, DL, INTTY, R20,
        DAG.getConstant(0x7F800000, INTTY));

    // and r31, r21, 0x7f800000
    SDValue R31 = DAG.getNode(ISD::AND, DL, INTTY, R21,
        DAG.getConstant(0x7f800000, INTTY));

    // and r32, r20, 0x807FFFFF
    SDValue R32 = DAG.getNode(ISD::AND, DL, INTTY, R20, 
        DAG.getConstant(0x807FFFFF, INTTY));

    // and r33, r21, 0x807FFFFF
    SDValue R33 = DAG.getNode(ISD::AND, DL, INTTY, R21, 
        DAG.getConstant(0x807FFFFF, INTTY));

    // ieq r40, r30, 0x7F800000
    SDValue R40 = DAG.getNode(AMDILISD::CMP, DL, INTTY, 
        DAG.getConstant(CondCCodeToCC(ISD::SETEQ, MVT::i32), MVT::i32),
        R30, DAG.getConstant(0x7F800000, INTTY));

    // ieq r41, r31, 0x7F800000
    SDValue R41 = DAG.getNode(AMDILISD::CMP, DL, INTTY, 
        DAG.getConstant(CondCCodeToCC(ISD::SETEQ, MVT::i32), MVT::i32),
        R31, DAG.getConstant(0x7F800000, INTTY));

    // ieq r42, r30, 0
    SDValue R42 = DAG.getNode(AMDILISD::CMP, DL, INTTY, 
        DAG.getConstant(CondCCodeToCC(ISD::SETEQ, MVT::i32), MVT::i32),
        R30, DAG.getConstant(0, INTTY));

    // ieq r43, r31, 0
    SDValue R43 = DAG.getNode(AMDILISD::CMP, DL, INTTY, 
        DAG.getConstant(CondCCodeToCC(ISD::SETEQ, MVT::i32), MVT::i32),
        R31, DAG.getConstant(0, INTTY));

    // and r50, r20, 0x80000000
    SDValue R50 = DAG.getNode(ISD::AND, DL, INTTY, R20,
        DAG.getConstant(0x80000000, INTTY));

    // and r51, r21, 0x80000000
    SDValue R51 = DAG.getNode(ISD::AND, DL, INTTY, R21,
        DAG.getConstant(0x80000000, INTTY));

    // ior r32, r32, 0x3f800000
    R32 = DAG.getNode(ISD::OR, DL, INTTY, R32, 
        DAG.getConstant(0x3F800000, INTTY));

    // ior r33, r33, 0x3f800000
    R33 = DAG.getNode(ISD::OR, DL, INTTY, R33, 
        DAG.getConstant(0x3F800000, INTTY));

    // cmov_logical r32, r42, r50, r32
    R32 = DAG.getNode(AMDILISD::CMOVLOG, DL, INTTY, R42, R50, R32);

    // cmov_logical r33, r43, r51, r33
    R33 = DAG.getNode(AMDILISD::CMOVLOG, DL, INTTY, R43, R51, R33);

    // cmov_logical r32, r40, r20, r32
    R32 = DAG.getNode(AMDILISD::CMOVLOG, DL, INTTY, R40, R20, R32);

    // cmov_logical r33, r41, r21, r33
    R33 = DAG.getNode(AMDILISD::CMOVLOG, DL, INTTY, R41, R21, R33);

    // ior r50, r40, r41
    R50 = DAG.getNode(ISD::OR, DL, INTTY, R40, R41);

    // ior r51, r42, r43
    R51 = DAG.getNode(ISD::OR, DL, INTTY, R42, R43);

    // ior r50, r50, r51
    R50 = DAG.getNode(ISD::OR, DL, INTTY, R50, R51);

    // inegate r52, r31
    SDValue R52 = DAG.getNode(AMDILISD::INEGATE, DL, INTTY, R31);

    // iadd r30, r30, r52
    R30 = DAG.getNode(ISD::ADD, DL, INTTY, R30, R52);

    // cmov_logical r30, r50, 0, r30
    R30 = DAG.getNode(AMDILISD::CMOVLOG, DL, INTTY, R50,
        DAG.getConstant(0, INTTY), R30);

    // div_zeroop(infinity) r21, 1.0, as_float(r33)
    R33 = DAG.getNode(ISDBITCAST, DL, OVT, R33);
    R21 = DAG.getNode(AMDILISD::DIV_INF, DL, OVT, 
        DAG.getConstantFP(1.0f, OVT), R33);

    // mul_ieee as_int(r20), as_float(r32), r21
    R32 = DAG.getNode(ISDBITCAST, DL, OVT, R32);
    R20 = DAG.getNode(ISD::FMUL, DL, OVT, R32, R21);
    R20 = DAG.getNode(ISDBITCAST, DL, INTTY, R20);

    // div_zeroop(infinity) r21, 1.0, as_float(r33)
    R33 = DAG.getNode(ISDBITCAST, DL, OVT, R33);
    R21 = DAG.getNode(AMDILISD::DIV_INF, DL, OVT, 
        DAG.getConstantFP(1.0f, OVT), R33);

    // mul_ieee as_int(r20), as_float(r32), r21
    R32 = DAG.getNode(ISDBITCAST, DL, OVT, R32);
    R20 = DAG.getNode(ISD::FMUL, DL, OVT, R32, R21);
    R20 = DAG.getNode(ISDBITCAST, DL, INTTY, R20);

    // and r22, r20, 0x7FFFFFFF
    SDValue R22 = DAG.getNode(ISD::AND, DL, INTTY, R20,
        DAG.getConstant(0x7FFFFFFF, INTTY));

    // and r23, r20, 0x80000000
    SDValue R23 = DAG.getNode(ISD::AND, DL, INTTY, R20,
        DAG.getConstant(0x80000000, INTTY));

    // ishr r60, r22, 0x00000017
    SDValue R60 = DAG.getNode(ISD::SRA, DL, INTTY, R22,
        DAG.getConstant(0x00000017, INTTY));

    // ishr r61, r30, 0x00000017
    SDValue R61 = DAG.getNode(ISD::SRA, DL, INTTY, R30,
        DAG.getConstant(0x00000017, INTTY));

    // iadd r20, r20, r30
    R20 = DAG.getNode(ISD::ADD, DL, INTTY, R20, R30);

    // iadd r21, r22, r30
    R21 = DAG.getNode(ISD::ADD, DL, INTTY, R22, R30);

    // iadd r60, r60, r61
    R60 = DAG.getNode(ISD::ADD, DL, INTTY, R60, R61);

    // ige r42, 0, R60
    R42 = DAG.getNode(AMDILISD::CMP, DL, INTTY,
        DAG.getConstant(CondCCodeToCC(ISD::SETGE, MVT::i32), MVT::i32),
        DAG.getConstant(0, INTTY),
        R60);

    // ior r41, r23, 0x7F800000
    R41 = DAG.getNode(ISD::OR, DL, INTTY, R23,
        DAG.getConstant(0x7F800000, INTTY));

    // ige r40, r60, 0x000000FF
    R40 = DAG.getNode(AMDILISD::CMP, DL, INTTY,
        DAG.getConstant(CondCCodeToCC(ISD::SETGE, MVT::i32), MVT::i32),
        R60,
        DAG.getConstant(0x0000000FF, INTTY));

    // cmov_logical r40, r50, 0, r40
    R40 = DAG.getNode(AMDILISD::CMOVLOG, DL, INTTY, R50,
        DAG.getConstant(0, INTTY),
        R40);

    // cmov_logical r20, r42, r23, r20
    R20 = DAG.getNode(AMDILISD::CMOVLOG, DL, INTTY, R42, R23, R20);

    // cmov_logical DST, r40, r41, r20
    DST = DAG.getNode(AMDILISD::CMOVLOG, DL, INTTY, R40, R41, R20);

    // as_float(DST)
    DST = DAG.getNode(ISDBITCAST, DL, OVT, DST);
  } else {
    // The following sequence of DAG nodes produce the following IL:
    // fabs r1, RHS
    // lt r2, 0x1.0p+96f, r1
    // cmov_logical r3, r2, 0x1.0p-23f, 1.0f
    // mul_ieee r1, RHS, r3
    // div_zeroop(infinity) r0, LHS, r1
    // mul_ieee DST, r0, r3

    // fabs r1, RHS
    SDValue r1 = DAG.getNode(ISD::FABS, DL, OVT, RHS);   
    // lt r2, 0x1.0p+96f, r1
    SDValue r2 = DAG.getNode(AMDILISD::CMP, DL, OVT,
        DAG.getConstant(CondCCodeToCC(ISD::SETLT, MVT::f32), MVT::i32),
        DAG.getConstant(0x6f800000, INTTY), r1);
    // cmov_logical r3, r2, 0x1.0p-23f, 1.0f
    SDValue r3 = DAG.getNode(AMDILISD::CMOVLOG, DL, OVT, r2,
        DAG.getConstant(0x2f800000, INTTY),
        DAG.getConstant(0x3f800000, INTTY));
    // mul_ieee r1, RHS, r3
    r1 = DAG.getNode(ISD::FMUL, DL, OVT, RHS, r3);
    // div_zeroop(infinity) r0, LHS, r1
    SDValue r0 = DAG.getNode(AMDILISD::DIV_INF, DL, OVT, LHS, r1);
    // mul_ieee DST, r0, r3
    DST = DAG.getNode(ISD::FMUL, DL, OVT, r0, r3);
  }
  return DST;
}

SDValue
AMDILTargetLowering::LowerFDIV64(SDValue Op, SelectionDAG &DAG) const
{
  return SDValue(Op.getNode(), 0);
}

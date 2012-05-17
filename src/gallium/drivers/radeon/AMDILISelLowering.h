//===-- AMDILISelLowering.h - AMDIL DAG Lowering Interface ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//
//
// This file defines the interfaces that AMDIL uses to lower LLVM code into a
// selection DAG.
//
//===----------------------------------------------------------------------===//

#ifndef AMDIL_ISELLOWERING_H_
#define AMDIL_ISELLOWERING_H_
#include "AMDIL.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/Target/TargetLowering.h"

namespace llvm
{
  namespace AMDILISD
  {
    enum
    {
      FIRST_NUMBER = ISD::BUILTIN_OP_END,
      INTTOANY,    // Dummy instruction that takes an int and goes to
      // any type converts the SDNode to an int
      DP_TO_FP,    // Conversion from 64bit FP to 32bit FP
      FP_TO_DP,    // Conversion from 32bit FP to 64bit FP
      BITCONV,     // instruction that converts from any type to any type
      CMOV,        // 32bit FP Conditional move instruction
      CMOVLOG,     // 32bit FP Conditional move logical instruction
      SELECT,      // 32bit FP Conditional move logical instruction
      SETCC,       // 32bit FP Conditional move logical instruction
      ISGN,        // 32bit Int Sign instruction
      INEGATE,     // 32bit Int Negation instruction
      MAD,         // 32bit Fused Multiply Add instruction
      ADD,         // 32/64 bit pseudo instruction
      AND,         // 128 bit and instruction
      OR,          // 128 bit or instruction
      NOT,         // 128 bit not instruction
      XOR,         // 128 bit xor instruction
      MOVE,        // generic mov instruction
      PHIMOVE,     // generic phi-node mov instruction
      VBUILD,      // scalar to vector mov instruction
      VEXTRACT,    // extract vector components
      VINSERT,     // insert vector components
      VCONCAT,     // concat a single vector to another vector
      UMAD,        // 32bit UInt Fused Multiply Add instruction
      CALL,        // Function call based on a single integer
      RET,         // Return from a function call
      SELECT_CC,   // Select the correct conditional instruction
      BRCC,        // Select the correct branch instruction
      CMPCC,       // Compare to GPR operands
      CMPICC,      // Compare two GPR operands, set icc.
      CMPFCC,      // Compare two FP operands, set fcc.
      BRICC,       // Branch to dest on icc condition
      BRFCC,       // Branch to dest on fcc condition
      SELECT_ICC,  // Select between two values using the current ICC
      //flags.
      SELECT_FCC,  // Select between two values using the current FCC
      //flags.
      LCREATE,     // Create a 64bit integer from two 32 bit integers
      LCOMPHI,     // Get the hi 32 bits from a 64 bit integer
      LCOMPLO,     // Get the lo 32 bits from a 64 bit integer
      DCREATE,     // Create a 64bit float from two 32 bit integers
      DCOMPHI,     // Get the hi 32 bits from a 64 bit float
      DCOMPLO,     // Get the lo 32 bits from a 64 bit float
      LCREATE2,     // Create a 64bit integer from two 32 bit integers
      LCOMPHI2,     // Get the hi 32 bits from a 64 bit integer
      LCOMPLO2,     // Get the lo 32 bits from a 64 bit integer
      DCREATE2,     // Create a 64bit float from two 32 bit integers
      DCOMPHI2,     // Get the hi 32 bits from a 64 bit float
      DCOMPLO2,     // Get the lo 32 bits from a 64 bit float
      UMUL,        // 32bit unsigned multiplication
      IFFB_HI,  // 32bit find first hi bit instruction
      IFFB_LO,  // 32bit find first low bit instruction
      DIV_INF,      // Divide with infinity returned on zero divisor
      SMAX,        // Signed integer max
      CMP,
      IL_CC_I_GT,
      IL_CC_I_LT,
      IL_CC_I_GE,
      IL_CC_I_LE,
      IL_CC_I_EQ,
      IL_CC_I_NE,
      RET_FLAG,
      BRANCH_COND,
      LOOP_NZERO,
      LOOP_ZERO,
      LOOP_CMP,
      ADDADDR,
      LAST_NON_MEMORY_OPCODE,
      // ATOMIC Operations
      // Global Memory
      ATOM_G_ADD = ISD::FIRST_TARGET_MEMORY_OPCODE,
      ATOM_G_AND,
      ATOM_G_CMPXCHG,
      ATOM_G_DEC,
      ATOM_G_INC,
      ATOM_G_MAX,
      ATOM_G_UMAX,
      ATOM_G_MIN,
      ATOM_G_UMIN,
      ATOM_G_OR,
      ATOM_G_SUB,
      ATOM_G_RSUB,
      ATOM_G_XCHG,
      ATOM_G_XOR,
      ATOM_G_ADD_NORET,
      ATOM_G_AND_NORET,
      ATOM_G_CMPXCHG_NORET,
      ATOM_G_DEC_NORET,
      ATOM_G_INC_NORET,
      ATOM_G_MAX_NORET,
      ATOM_G_UMAX_NORET,
      ATOM_G_MIN_NORET,
      ATOM_G_UMIN_NORET,
      ATOM_G_OR_NORET,
      ATOM_G_SUB_NORET,
      ATOM_G_RSUB_NORET,
      ATOM_G_XCHG_NORET,
      ATOM_G_XOR_NORET,
      // Local Memory
      ATOM_L_ADD,
      ATOM_L_AND,
      ATOM_L_CMPXCHG,
      ATOM_L_DEC,
      ATOM_L_INC,
      ATOM_L_MAX,
      ATOM_L_UMAX,
      ATOM_L_MIN,
      ATOM_L_UMIN,
      ATOM_L_OR,
      ATOM_L_MSKOR,
      ATOM_L_SUB,
      ATOM_L_RSUB,
      ATOM_L_XCHG,
      ATOM_L_XOR,
      ATOM_L_ADD_NORET,
      ATOM_L_AND_NORET,
      ATOM_L_CMPXCHG_NORET,
      ATOM_L_DEC_NORET,
      ATOM_L_INC_NORET,
      ATOM_L_MAX_NORET,
      ATOM_L_UMAX_NORET,
      ATOM_L_MIN_NORET,
      ATOM_L_UMIN_NORET,
      ATOM_L_OR_NORET,
      ATOM_L_MSKOR_NORET,
      ATOM_L_SUB_NORET,
      ATOM_L_RSUB_NORET,
      ATOM_L_XCHG_NORET,
      ATOM_L_XOR_NORET,
      // Region Memory
      ATOM_R_ADD,
      ATOM_R_AND,
      ATOM_R_CMPXCHG,
      ATOM_R_DEC,
      ATOM_R_INC,
      ATOM_R_MAX,
      ATOM_R_UMAX,
      ATOM_R_MIN,
      ATOM_R_UMIN,
      ATOM_R_OR,
      ATOM_R_MSKOR,
      ATOM_R_SUB,
      ATOM_R_RSUB,
      ATOM_R_XCHG,
      ATOM_R_XOR,
      ATOM_R_ADD_NORET,
      ATOM_R_AND_NORET,
      ATOM_R_CMPXCHG_NORET,
      ATOM_R_DEC_NORET,
      ATOM_R_INC_NORET,
      ATOM_R_MAX_NORET,
      ATOM_R_UMAX_NORET,
      ATOM_R_MIN_NORET,
      ATOM_R_UMIN_NORET,
      ATOM_R_OR_NORET,
      ATOM_R_MSKOR_NORET,
      ATOM_R_SUB_NORET,
      ATOM_R_RSUB_NORET,
      ATOM_R_XCHG_NORET,
      ATOM_R_XOR_NORET,
      // Append buffer
      APPEND_ALLOC,
      APPEND_ALLOC_NORET,
      APPEND_CONSUME,
      APPEND_CONSUME_NORET,
      // 2D Images
      IMAGE2D_READ,
      IMAGE2D_WRITE,
      IMAGE2D_INFO0,
      IMAGE2D_INFO1,
      // 3D Images
      IMAGE3D_READ,
      IMAGE3D_WRITE,
      IMAGE3D_INFO0,
      IMAGE3D_INFO1,

      LAST_ISD_NUMBER
    };
  } // AMDILISD

  class MachineBasicBlock;
  class MachineInstr;
  class DebugLoc;
  class TargetInstrInfo;

  class AMDILTargetLowering : public TargetLowering
  {
    private:
      int VarArgsFrameOffset;   // Frame offset to start of varargs area.
    public:
      AMDILTargetLowering(TargetMachine &TM);

      virtual SDValue
        LowerOperation(SDValue Op, SelectionDAG &DAG) const;

      int
        getVarArgsFrameOffset() const;

      /// computeMaskedBitsForTargetNode - Determine which of
      /// the bits specified
      /// in Mask are known to be either zero or one and return them in
      /// the
      /// KnownZero/KnownOne bitsets.
      virtual void
        computeMaskedBitsForTargetNode(
            const SDValue Op,
            APInt &KnownZero,
            APInt &KnownOne,
            const SelectionDAG &DAG,
            unsigned Depth = 0
            ) const;

      virtual MachineBasicBlock*
        EmitInstrWithCustomInserter(
            MachineInstr *MI,
            MachineBasicBlock *MBB) const;

      virtual bool 
        getTgtMemIntrinsic(IntrinsicInfo &Info,
                                  const CallInst &I, unsigned Intrinsic) const;
      virtual const char*
        getTargetNodeName(
            unsigned Opcode
            ) const;
      // We want to mark f32/f64 floating point values as
      // legal
      bool
        isFPImmLegal(const APFloat &Imm, EVT VT) const;
      // We don't want to shrink f64/f32 constants because
      // they both take up the same amount of space and
      // we don't want to use a f2d instruction.
      bool ShouldShrinkFPConstant(EVT VT) const;

      /// getFunctionAlignment - Return the Log2 alignment of this
      /// function.
      virtual unsigned int
        getFunctionAlignment(const Function *F) const;

    private:
      CCAssignFn*
        CCAssignFnForNode(unsigned int CC) const;

      SDValue LowerCallResult(SDValue Chain,
          SDValue InFlag,
          CallingConv::ID CallConv,
          bool isVarArg,
          const SmallVectorImpl<ISD::InputArg> &Ins,
          DebugLoc dl,
          SelectionDAG &DAG,
          SmallVectorImpl<SDValue> &InVals) const;

      SDValue LowerMemArgument(SDValue Chain,
          CallingConv::ID CallConv,
          const SmallVectorImpl<ISD::InputArg> &ArgInfo,
          DebugLoc dl, SelectionDAG &DAG,
          const CCValAssign &VA,  MachineFrameInfo *MFI,
          unsigned i) const;

      SDValue LowerMemOpCallTo(SDValue Chain, SDValue StackPtr,
          SDValue Arg,
          DebugLoc dl, SelectionDAG &DAG,
          const CCValAssign &VA,
          ISD::ArgFlagsTy Flags) const;

      virtual SDValue
        LowerFormalArguments(SDValue Chain,
            CallingConv::ID CallConv, bool isVarArg,
            const SmallVectorImpl<ISD::InputArg> &Ins,
            DebugLoc dl, SelectionDAG &DAG,
            SmallVectorImpl<SDValue> &InVals) const;

      virtual SDValue
        LowerCall(SDValue Chain, SDValue Callee,
            CallingConv::ID CallConv, bool isVarArg, bool doesNotRet,
            bool &isTailCall,
            const SmallVectorImpl<ISD::OutputArg> &Outs,
            const SmallVectorImpl<SDValue> &OutVals,
            const SmallVectorImpl<ISD::InputArg> &Ins,
            DebugLoc dl, SelectionDAG &DAG,
            SmallVectorImpl<SDValue> &InVals) const;

      virtual SDValue
        LowerReturn(SDValue Chain,
            CallingConv::ID CallConv, bool isVarArg,
            const SmallVectorImpl<ISD::OutputArg> &Outs,
            const SmallVectorImpl<SDValue> &OutVals,
            DebugLoc dl, SelectionDAG &DAG) const;

      //+++--- Function dealing with conversions between floating point and
      //integer types ---+++//
      SDValue
        genCLZu64(SDValue Op, SelectionDAG &DAG) const;
      SDValue
        genCLZuN(SDValue Op, SelectionDAG &DAG, uint32_t bits) const;
      SDValue
        genCLZu32(SDValue Op, SelectionDAG &DAG) const;
      SDValue
        genf64toi32(SDValue Op, SelectionDAG &DAG,
            bool includeSign) const;

      SDValue
        genf64toi64(SDValue Op, SelectionDAG &DAG,
            bool includeSign) const;

      SDValue
        genu32tof64(SDValue Op, EVT dblvt, SelectionDAG &DAG) const;

      SDValue
        genu64tof64(SDValue Op, EVT dblvt, SelectionDAG &DAG) const;

      SDValue
        LowerFP_TO_SINT(SDValue Op, SelectionDAG &DAG) const;

      SDValue
        LowerFP_TO_UINT(SDValue Op, SelectionDAG &DAG) const;

      SDValue
        LowerSINT_TO_FP(SDValue Op, SelectionDAG &DAG) const;

      SDValue
        LowerUINT_TO_FP(SDValue Op, SelectionDAG &DAG) const;

      SDValue
        LowerGlobalAddress(SDValue Op, SelectionDAG &DAG) const;

      SDValue
        LowerINTRINSIC_WO_CHAIN(SDValue Op, SelectionDAG& DAG) const;

      SDValue
        LowerINTRINSIC_W_CHAIN(SDValue Op, SelectionDAG& DAG) const;

      SDValue
        LowerINTRINSIC_VOID(SDValue Op, SelectionDAG& DAG) const;

      SDValue
        LowerJumpTable(SDValue Op, SelectionDAG &DAG) const;

      SDValue
        LowerConstantPool(SDValue Op, SelectionDAG &DAG) const;

      SDValue
        LowerExternalSymbol(SDValue Op, SelectionDAG &DAG) const;

      SDValue
        LowerADD(SDValue Op, SelectionDAG &DAG) const;

      SDValue
        LowerSUB(SDValue Op, SelectionDAG &DAG) const;

      SDValue
        LowerSREM(SDValue Op, SelectionDAG &DAG) const;
      SDValue
        LowerSREM8(SDValue Op, SelectionDAG &DAG) const;
      SDValue
        LowerSREM16(SDValue Op, SelectionDAG &DAG) const;
      SDValue
        LowerSREM32(SDValue Op, SelectionDAG &DAG) const;
      SDValue
        LowerSREM64(SDValue Op, SelectionDAG &DAG) const;

      SDValue
        LowerUREM(SDValue Op, SelectionDAG &DAG) const;
      SDValue
        LowerUREM8(SDValue Op, SelectionDAG &DAG) const;
      SDValue
        LowerUREM16(SDValue Op, SelectionDAG &DAG) const;
      SDValue
        LowerUREM32(SDValue Op, SelectionDAG &DAG) const;
      SDValue
        LowerUREM64(SDValue Op, SelectionDAG &DAG) const;

      SDValue
        LowerSDIV(SDValue Op, SelectionDAG &DAG) const;
      SDValue
        LowerSDIV24(SDValue Op, SelectionDAG &DAG) const;
      SDValue
        LowerSDIV32(SDValue Op, SelectionDAG &DAG) const;
      SDValue
        LowerSDIV64(SDValue Op, SelectionDAG &DAG) const;

      SDValue
        LowerUDIV(SDValue Op, SelectionDAG &DAG) const;
      SDValue
        LowerUDIV24(SDValue Op, SelectionDAG &DAG) const;
      SDValue
        LowerUDIV32(SDValue Op, SelectionDAG &DAG) const;
      SDValue
        LowerUDIV64(SDValue Op, SelectionDAG &DAG) const;

      SDValue
        LowerFDIV(SDValue Op, SelectionDAG &DAG) const;
      SDValue
        LowerFDIV32(SDValue Op, SelectionDAG &DAG) const;
      SDValue
        LowerFDIV64(SDValue Op, SelectionDAG &DAG) const;

      SDValue
        LowerMUL(SDValue Op, SelectionDAG &DAG) const;

      SDValue
        LowerBUILD_VECTOR(SDValue Op, SelectionDAG &DAG) const;

      SDValue
        LowerINSERT_VECTOR_ELT(SDValue Op, SelectionDAG &DAG) const;

      SDValue
        LowerEXTRACT_VECTOR_ELT(SDValue Op, SelectionDAG &DAG) const;

      SDValue
        LowerEXTRACT_SUBVECTOR(SDValue Op, SelectionDAG &DAG) const;

      SDValue
        LowerSCALAR_TO_VECTOR(SDValue Op, SelectionDAG &DAG) const;

      SDValue
        LowerCONCAT_VECTORS(SDValue Op, SelectionDAG &DAG) const;

      SDValue
        LowerAND(SDValue Op, SelectionDAG &DAG) const;

      SDValue
        LowerOR(SDValue Op, SelectionDAG &DAG) const;

      SDValue
        LowerSELECT(SDValue Op, SelectionDAG &DAG) const;

      SDValue
        LowerSELECT_CC(SDValue Op, SelectionDAG &DAG) const;

      SDValue
        LowerSETCC(SDValue Op, SelectionDAG &DAG) const;

      SDValue
        LowerSIGN_EXTEND_INREG(SDValue Op, SelectionDAG &DAG) const;

      EVT
        genIntType(uint32_t size = 32, uint32_t numEle = 1) const;

      SDValue
        LowerBITCAST(SDValue Op, SelectionDAG &DAG) const;

      SDValue
        LowerDYNAMIC_STACKALLOC(SDValue Op, SelectionDAG &DAG) const;

      SDValue
        LowerBRCOND(SDValue Op, SelectionDAG &DAG) const;

      SDValue
        LowerBR_CC(SDValue Op, SelectionDAG &DAG) const;
      SDValue
        LowerFP_ROUND(SDValue Op, SelectionDAG &DAG) const;
      void
        generateCMPInstr(MachineInstr*, MachineBasicBlock*,
            const TargetInstrInfo&) const;
      MachineOperand
        convertToReg(MachineOperand) const;

      // private members used by the set of instruction generation
      // functions, these are marked mutable as they are cached so
      // that they don't have to constantly be looked up when using the
      // generateMachineInst/genVReg instructions. This is to simplify
      // the code
      // and to make it cleaner. The object itself doesn't change as
      // only these functions use these three data types.
      mutable MachineBasicBlock *mBB;
      mutable DebugLoc *mDL;
      mutable const TargetInstrInfo *mTII;
      mutable MachineBasicBlock::iterator mBBI;
      void
        setPrivateData(MachineBasicBlock *BB, 
            MachineBasicBlock::iterator &BBI, 
            DebugLoc *DL,
          const TargetInstrInfo *TII) const;
      uint32_t genVReg(uint32_t regType) const;
      MachineInstrBuilder
        generateMachineInst(uint32_t opcode,
          uint32_t dst) const;
      MachineInstrBuilder
        generateMachineInst(uint32_t opcode,
          uint32_t dst, uint32_t src1) const;
      MachineInstrBuilder
        generateMachineInst(uint32_t opcode,
          uint32_t dst, uint32_t src1, uint32_t src2) const;
      MachineInstrBuilder
        generateMachineInst(uint32_t opcode,
          uint32_t dst, uint32_t src1, uint32_t src2,
          uint32_t src3) const;
      uint32_t
        addExtensionInstructions(
          uint32_t reg, bool signedShift,
          unsigned int simpleVT) const;
      void
        generateLongRelational(MachineInstr *MI,
          unsigned int opCode) const;

  }; // AMDILTargetLowering
} // end namespace llvm

#endif    // AMDIL_ISELLOWERING_H_

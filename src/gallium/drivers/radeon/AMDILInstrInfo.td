//===------------ AMDILInstrInfo.td - AMDIL Target ------*-tablegen-*------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//
//
// This file describes the AMDIL instructions in TableGen format.
//
//===----------------------------------------------------------------------===//
// AMDIL Instruction Predicate Definitions
// Predicate that is set to true if the hardware supports double precision
// divide
def HasHWDDiv                 : Predicate<"Subtarget.device()"
                           "->getGeneration() > AMDILDeviceInfo::HD4XXX && "
              "Subtarget.device()->usesHardware(AMDILDeviceInfo::DoubleOps)">;

// Predicate that is set to true if the hardware supports double, but not double
// precision divide in hardware
def HasSWDDiv             : Predicate<"Subtarget.device()"
                           "->getGeneration() == AMDILDeviceInfo::HD4XXX &&"
              "Subtarget.device()->usesHardware(AMDILDeviceInfo::DoubleOps)">;

// Predicate that is set to true if the hardware support 24bit signed
// math ops. Otherwise a software expansion to 32bit math ops is used instead.
def HasHWSign24Bit          : Predicate<"Subtarget.device()"
                            "->getGeneration() > AMDILDeviceInfo::HD5XXX">;

// Predicate that is set to true if 64bit operations are supported or not
def HasHW64Bit              : Predicate<"Subtarget.device()"
                            "->usesHardware(AMDILDeviceInfo::LongOps)">;
def HasSW64Bit              : Predicate<"Subtarget.device()"
                            "->usesSoftware(AMDILDeviceInfo::LongOps)">;

// Predicate that is set to true if the timer register is supported
def HasTmrRegister          : Predicate<"Subtarget.device()"
                            "->isSupported(AMDILDeviceInfo::TmrReg)">;
// Predicate that is true if we are at least evergreen series
def HasDeviceIDInst         : Predicate<"Subtarget.device()"
                            "->getGeneration() >= AMDILDeviceInfo::HD5XXX">;

// Predicate that is true if we have region address space.
def hasRegionAS             : Predicate<"Subtarget.device()"
                            "->usesHardware(AMDILDeviceInfo::RegionMem)">;

// Predicate that is false if we don't have region address space.
def noRegionAS             : Predicate<"!Subtarget.device()"
                            "->isSupported(AMDILDeviceInfo::RegionMem)">;


// Predicate that is set to true if 64bit Mul is supported in the IL or not
def HasHW64Mul              : Predicate<"Subtarget.calVersion()" 
                                          ">= CAL_VERSION_SC_139"
                                          "&& Subtarget.device()"
                                          "->getGeneration() >="
                                          "AMDILDeviceInfo::HD5XXX">;
def HasSW64Mul              : Predicate<"Subtarget.calVersion()" 
                                          "< CAL_VERSION_SC_139">;
// Predicate that is set to true if 64bit Div/Mod is supported in the IL or not
def HasHW64DivMod           : Predicate<"Subtarget.device()"
                            "->usesHardware(AMDILDeviceInfo::HW64BitDivMod)">;
def HasSW64DivMod           : Predicate<"Subtarget.device()"
                            "->usesSoftware(AMDILDeviceInfo::HW64BitDivMod)">;

// Predicate that is set to true if 64bit pointer are used.
def Has64BitPtr             : Predicate<"Subtarget.is64bit()">;
def Has32BitPtr             : Predicate<"!Subtarget.is64bit()">;
//===--------------------------------------------------------------------===//
// Custom Operands
//===--------------------------------------------------------------------===//
def brtarget   : Operand<OtherVT>;

//===--------------------------------------------------------------------===//
// Custom Selection DAG Type Profiles
//===--------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//
// Generic Profile Types
//===----------------------------------------------------------------------===//

def SDTIL_GenBinaryOp : SDTypeProfile<1, 2, [
    SDTCisSameAs<0, 1>, SDTCisSameAs<1, 2>
    ]>;
def SDTIL_GenTernaryOp : SDTypeProfile<1, 3, [
    SDTCisSameAs<0, 1>, SDTCisSameAs<1, 2>, SDTCisSameAs<2, 3>
    ]>;
def SDTIL_GenVecBuild : SDTypeProfile<1, 1, [
    SDTCisEltOfVec<1, 0>
    ]>;

//===----------------------------------------------------------------------===//
// Flow Control Profile Types
//===----------------------------------------------------------------------===//
// Branch instruction where second and third are basic blocks
def SDTIL_BRCond : SDTypeProfile<0, 2, [
    SDTCisVT<0, OtherVT>
    ]>;
// Comparison instruction
def SDTIL_Cmp  : SDTypeProfile<1, 3, [
    SDTCisSameAs<0, 2>, SDTCisSameAs<2,3>, SDTCisVT<1, i32>
    ]>;

//===--------------------------------------------------------------------===//
// Custom Selection DAG Nodes
//===--------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//
// Flow Control DAG Nodes
//===----------------------------------------------------------------------===//
def IL_brcond      : SDNode<"AMDILISD::BRANCH_COND", SDTIL_BRCond, [SDNPHasChain]>;

//===----------------------------------------------------------------------===//
// Comparison DAG Nodes
//===----------------------------------------------------------------------===//
def IL_cmp       : SDNode<"AMDILISD::CMP", SDTIL_Cmp>;

//===----------------------------------------------------------------------===//
// Call/Return DAG Nodes
//===----------------------------------------------------------------------===//
def IL_retflag       : SDNode<"AMDILISD::RET_FLAG", SDTNone,
    [SDNPHasChain, SDNPOptInGlue]>;

//===--------------------------------------------------------------------===//
// Instructions
//===--------------------------------------------------------------------===//
// Floating point math functions
def IL_cmov_logical : SDNode<"AMDILISD::CMOVLOG", SDTIL_GenTernaryOp>;
def IL_div_inf      : SDNode<"AMDILISD::DIV_INF", SDTIL_GenBinaryOp>;
def IL_mad          : SDNode<"AMDILISD::MAD", SDTIL_GenTernaryOp>;

//===----------------------------------------------------------------------===//
// Integer functions
//===----------------------------------------------------------------------===//
def IL_umul        : SDNode<"AMDILISD::UMUL"    , SDTIntBinOp,
    [SDNPCommutative, SDNPAssociative]>;

//===----------------------------------------------------------------------===//
// Vector functions
//===----------------------------------------------------------------------===//
def IL_vbuild     : SDNode<"AMDILISD::VBUILD", SDTIL_GenVecBuild,
    []>;

//===--------------------------------------------------------------------===//
// Custom Pattern DAG Nodes
//===--------------------------------------------------------------------===//
def global_store : PatFrag<(ops node:$val, node:$ptr),
    (store node:$val, node:$ptr), [{
        return isGlobalStore(dyn_cast<StoreSDNode>(N));
}]>;

//===----------------------------------------------------------------------===//
// Load pattern fragments
//===----------------------------------------------------------------------===//
// Global address space loads
def global_load : PatFrag<(ops node:$ptr), (load node:$ptr), [{
    return isGlobalLoad(dyn_cast<LoadSDNode>(N));
}]>;
// Constant address space loads
def constant_load : PatFrag<(ops node:$ptr), (load node:$ptr), [{
    return isConstantLoad(dyn_cast<LoadSDNode>(N), -1);
}]>;

//===----------------------------------------------------------------------===//
// Complex addressing mode patterns
//===----------------------------------------------------------------------===//
def ADDR : ComplexPattern<i32, 2, "SelectADDR", [], []>;
def ADDRF : ComplexPattern<i32, 2, "SelectADDR", [frameindex], []>;
def ADDR64 : ComplexPattern<i64, 2, "SelectADDR64", [], []>;
def ADDR64F : ComplexPattern<i64, 2, "SelectADDR64", [frameindex], []>;


//===----------------------------------------------------------------------===//
// Conditional Instruction Pattern Leafs
//===----------------------------------------------------------------------===//
class IL_CC_Op<int N> : PatLeaf<(i32 N)>;
def IL_CC_D_EQ  : IL_CC_Op<0>;
def IL_CC_D_GE  : IL_CC_Op<1>;
def IL_CC_D_LT  : IL_CC_Op<2>;
def IL_CC_D_NE  : IL_CC_Op<3>;
def IL_CC_F_EQ  : IL_CC_Op<4>;
def IL_CC_F_GE  : IL_CC_Op<5>;
def IL_CC_F_LT  : IL_CC_Op<6>;
def IL_CC_F_NE  : IL_CC_Op<7>;
def IL_CC_I_EQ  : IL_CC_Op<8>;
def IL_CC_I_GE  : IL_CC_Op<9>;
def IL_CC_I_LT  : IL_CC_Op<10>;
def IL_CC_I_NE  : IL_CC_Op<11>;
def IL_CC_U_GE  : IL_CC_Op<12>;
def IL_CC_U_LT  : IL_CC_Op<13>;
// Pseudo IL comparison instructions that aren't natively supported
def IL_CC_F_GT  : IL_CC_Op<14>;
def IL_CC_U_GT  : IL_CC_Op<15>;
def IL_CC_I_GT  : IL_CC_Op<16>;
def IL_CC_D_GT  : IL_CC_Op<17>;
def IL_CC_F_LE  : IL_CC_Op<18>;
def IL_CC_U_LE  : IL_CC_Op<19>;
def IL_CC_I_LE  : IL_CC_Op<20>;
def IL_CC_D_LE  : IL_CC_Op<21>;
def IL_CC_F_UNE : IL_CC_Op<22>;
def IL_CC_F_UEQ : IL_CC_Op<23>;
def IL_CC_F_ULT : IL_CC_Op<24>;
def IL_CC_F_UGT : IL_CC_Op<25>;
def IL_CC_F_ULE : IL_CC_Op<26>;
def IL_CC_F_UGE : IL_CC_Op<27>;
def IL_CC_F_ONE : IL_CC_Op<28>;
def IL_CC_F_OEQ : IL_CC_Op<29>;
def IL_CC_F_OLT : IL_CC_Op<30>;
def IL_CC_F_OGT : IL_CC_Op<31>;
def IL_CC_F_OLE : IL_CC_Op<32>;
def IL_CC_F_OGE : IL_CC_Op<33>;
def IL_CC_D_UNE : IL_CC_Op<34>;
def IL_CC_D_UEQ : IL_CC_Op<35>;
def IL_CC_D_ULT : IL_CC_Op<36>;
def IL_CC_D_UGT : IL_CC_Op<37>;
def IL_CC_D_ULE : IL_CC_Op<38>;
def IL_CC_D_UGE : IL_CC_Op<39>;
def IL_CC_D_ONE : IL_CC_Op<30>;
def IL_CC_D_OEQ : IL_CC_Op<41>;
def IL_CC_D_OLT : IL_CC_Op<42>;
def IL_CC_D_OGT : IL_CC_Op<43>;
def IL_CC_D_OLE : IL_CC_Op<44>;
def IL_CC_D_OGE : IL_CC_Op<45>;
def IL_CC_U_EQ  : IL_CC_Op<46>;
def IL_CC_U_NE  : IL_CC_Op<47>;
def IL_CC_F_O   : IL_CC_Op<48>;
def IL_CC_D_O   : IL_CC_Op<49>;
def IL_CC_F_UO  : IL_CC_Op<50>;
def IL_CC_D_UO  : IL_CC_Op<51>;
def IL_CC_L_LE  : IL_CC_Op<52>;
def IL_CC_L_GE  : IL_CC_Op<53>;
def IL_CC_L_EQ  : IL_CC_Op<54>;
def IL_CC_L_NE  : IL_CC_Op<55>;
def IL_CC_L_LT  : IL_CC_Op<56>;
def IL_CC_L_GT  : IL_CC_Op<57>;
def IL_CC_UL_LE  : IL_CC_Op<58>;
def IL_CC_UL_GE  : IL_CC_Op<59>;
def IL_CC_UL_EQ  : IL_CC_Op<60>;
def IL_CC_UL_NE  : IL_CC_Op<61>;
def IL_CC_UL_LT  : IL_CC_Op<62>;
def IL_CC_UL_GT  : IL_CC_Op<63>;

//===----------------------------------------------------------------------===//
// Instruction format classes
//===----------------------------------------------------------------------===//
class ILFormat<dag outs, dag ins, string asmstr, list<dag> pattern>
: Instruction {

     let Namespace = "AMDGPU";
     dag OutOperandList = outs;
     dag InOperandList = ins;
     let Pattern = pattern;
     let AsmString = !strconcat(asmstr, "\n");
     let isPseudo = 1;
     let Itinerary = NullALU;
     bit hasIEEEFlag = 0;
     bit hasZeroOpFlag = 0;
}

//===--------------------------------------------------------------------===//
// Multiclass Instruction formats
//===--------------------------------------------------------------------===//
// Multiclass that handles branch instructions
multiclass BranchConditional<SDNode Op> {
    def _i32 : ILFormat<(outs),
  (ins brtarget:$target, GPRI32:$src0),
        "; i32 Pseudo branch instruction",
  [(Op bb:$target, GPRI32:$src0)]>;
    def _f32 : ILFormat<(outs),
  (ins brtarget:$target, GPRF32:$src0),
        "; f32 Pseudo branch instruction",
  [(Op bb:$target, GPRF32:$src0)]>;
}

// Only scalar types should generate flow control
multiclass BranchInstr<string name> {
  def _i32 : ILFormat<(outs), (ins GPRI32:$src),
      !strconcat(name, " $src"), []>;
  def _f32 : ILFormat<(outs), (ins GPRF32:$src),
      !strconcat(name, " $src"), []>;
}
// Only scalar types should generate flow control
multiclass BranchInstr2<string name> {
  def _i32 : ILFormat<(outs), (ins GPRI32:$src0, GPRI32:$src1),
      !strconcat(name, " $src0, $src1"), []>;
  def _f32 : ILFormat<(outs), (ins GPRF32:$src0, GPRF32:$src1),
      !strconcat(name, " $src0, $src1"), []>;
}

//===--------------------------------------------------------------------===//
// Intrinsics support
//===--------------------------------------------------------------------===//
include "AMDILIntrinsics.td"

//===--------------------------------------------------------------------===//
// Instructions support
//===--------------------------------------------------------------------===//
//===---------------------------------------------------------------------===//
// Custom Inserter for Branches and returns, this eventually will be a
// seperate pass
//===---------------------------------------------------------------------===//
let isTerminator = 1 in {
  def BRANCH : ILFormat<(outs), (ins brtarget:$target),
      "; Pseudo unconditional branch instruction",
      [(br bb:$target)]>;
  defm BRANCH_COND : BranchConditional<IL_brcond>;
}
//===---------------------------------------------------------------------===//
// return instructions
//===---------------------------------------------------------------------===//
let isTerminator = 1, isReturn = 1, isBarrier = 1, hasCtrlDep = 1 in {
  def RETURN          : ILFormat<(outs), (ins variable_ops),
      "RETURN", [(IL_retflag)]>;
}

//===---------------------------------------------------------------------===//
// Flow and Program control Instructions
//===---------------------------------------------------------------------===//
let isTerminator=1 in {
  def SWITCH      : ILFormat< (outs), (ins GPRI32:$src),
  !strconcat("SWITCH", " $src"), []>;
  def CASE        : ILFormat< (outs), (ins GPRI32:$src),
      !strconcat("CASE", " $src"), []>;
  def BREAK       : ILFormat< (outs), (ins),
      "BREAK", []>;
  def CONTINUE    : ILFormat< (outs), (ins),
      "CONTINUE", []>;
  def DEFAULT     : ILFormat< (outs), (ins),
      "DEFAULT", []>;
  def ELSE        : ILFormat< (outs), (ins),
      "ELSE", []>;
  def ENDSWITCH   : ILFormat< (outs), (ins),
      "ENDSWITCH", []>;
  def ENDMAIN     : ILFormat< (outs), (ins),
      "ENDMAIN", []>;
  def END         : ILFormat< (outs), (ins),
      "END", []>;
  def ENDFUNC     : ILFormat< (outs), (ins),
      "ENDFUNC", []>;
  def ENDIF       : ILFormat< (outs), (ins),
      "ENDIF", []>;
  def WHILELOOP   : ILFormat< (outs), (ins),
      "WHILE", []>;
  def ENDLOOP     : ILFormat< (outs), (ins),
      "ENDLOOP", []>;
  def FUNC        : ILFormat< (outs), (ins),
      "FUNC", []>;
  def RETDYN      : ILFormat< (outs), (ins),
      "RET_DYN", []>;
  // This opcode has custom swizzle pattern encoded in Swizzle Encoder
  defm IF_LOGICALNZ  : BranchInstr<"IF_LOGICALNZ">;
  // This opcode has custom swizzle pattern encoded in Swizzle Encoder
  defm IF_LOGICALZ   : BranchInstr<"IF_LOGICALZ">;
  // This opcode has custom swizzle pattern encoded in Swizzle Encoder
  defm BREAK_LOGICALNZ : BranchInstr<"BREAK_LOGICALNZ">;
  // This opcode has custom swizzle pattern encoded in Swizzle Encoder
  defm BREAK_LOGICALZ : BranchInstr<"BREAK_LOGICALZ">;
  // This opcode has custom swizzle pattern encoded in Swizzle Encoder
  defm CONTINUE_LOGICALNZ : BranchInstr<"CONTINUE_LOGICALNZ">;
  // This opcode has custom swizzle pattern encoded in Swizzle Encoder
  defm CONTINUE_LOGICALZ : BranchInstr<"CONTINUE_LOGICALZ">;
  defm IFC         : BranchInstr2<"IFC">;
  defm BREAKC      : BranchInstr2<"BREAKC">;
  defm CONTINUEC   : BranchInstr2<"CONTINUEC">;
}

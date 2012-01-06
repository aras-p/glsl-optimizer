//==- AMDILFormats.td - AMDIL Instruction Formats ----*- tablegen -*-==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//
//
//===--------------------------------------------------------------------===//
include "AMDILTokenDesc.td"

//===--------------------------------------------------------------------===//
// The parent IL instruction class that inherits the Instruction class. This
// class sets the corresponding namespace, the out and input dag lists the
// pattern to match to and the string to print out for the assembly printer.
//===--------------------------------------------------------------------===//
class ILFormat<ILOpCode op, dag outs, dag ins, string asmstr, list<dag> pattern>
: Instruction {

     let Namespace = "AMDIL";
     dag OutOperandList = outs;
     dag InOperandList = ins;
     ILOpCode operation = op;
     let Pattern = pattern;
     let AsmString = !strconcat(asmstr, "\n");
     let isPseudo = 1;
     bit hasIEEEFlag = 0;
     bit hasZeroOpFlag = 0;
}

//===--------------------------------------------------------------------===//
// The base class for vector insert instructions. It is a single dest, quad
// source instruction where the last two source operands must be 32bit
// immediate values that are encoding the swizzle of the source register
// The src2 and src3 instructions must also be inversion of each other such
// that if src2 is 0x1000300(x0z0), src3 must be 0x20004(0y0w). The values
// are encoded as 32bit integer with each 8 char representing a swizzle value.
// The encoding is as follows for 32bit register types:
// 0x00 -> '_'
// 0x01 -> 'x'
// 0x02 -> 'y'
// 0x03 -> 'z'
// 0x04 -> 'w'
// 0x05 -> 'x'
// 0x06 -> 'y'
// 0x07 -> 'z'
// 0x08 -> 'w'
// 0x09 -> '0'
// The encoding is as follows for 64bit register types:
// 0x00 -> "__"
// 0x01 -> "xy"
// 0x02 -> "zw"
// 0x03 -> "xy"
// 0x04 -> "zw"
// 0x05 -> "00"
//===--------------------------------------------------------------------===//
class InsertVectorClass<ILOpCode op, RegisterClass DReg, RegisterClass SReg,
      SDNode OpNode, string asmstr> :
      ILFormat<op, (outs DReg:$dst),
      (ins DReg:$src0, SReg:$src1, i32imm:$src2, i32imm:$src3),
      !strconcat(asmstr, " $dst, $src0, $src1"),
      [(set DReg:$dst, (OpNode DReg:$src0, SReg:$src1,
                     timm:$src2, timm:$src3))]>;

//===--------------------------------------------------------------------===//
// Class that has one input parameters and one output parameter.
// The basic pattern for this class is "Opcode Dst, Src0" and
// handles the unary math operators.
// It sets the binary token ILSrc, ILSrcMod, ILRelAddr and ILSrc and ILSrcMod
// if the addressing is register relative for input and output register 0.
//===--------------------------------------------------------------------===//
class OneInOneOut<ILOpCode op, dag outs, dag ins,
      string asmstr, list<dag> pattern>
      : ILFormat<op, outs, ins, asmstr, pattern>
{
     ILDst       dst_reg;
     ILDstMod    dst_mod;
     ILRelAddr   dst_rel;
     ILSrc       dst_reg_rel;
     ILSrcMod    dst_reg_rel_mod;
     ILSrc       src0_reg;
     ILSrcMod    src0_mod;
     ILRelAddr   src0_rel;
     ILSrc       src0_reg_rel;
     ILSrcMod    src0_reg_rel_mod;
}

//===--------------------------------------------------------------------===//
// A simplified version of OneInOneOut class where the pattern is standard
// and does not need special cases. This requires that the pattern has
// a SDNode and takes a source and destination register that is of type
// RegisterClass. This is the standard unary op class.
//===--------------------------------------------------------------------===//
class UnaryOp<ILOpCode op, SDNode OpNode,
      RegisterClass dRegs, RegisterClass sRegs>
      : OneInOneOut<op, (outs dRegs:$dst), (ins sRegs:$src),
      !strconcat(op.Text, " $dst, $src"),
      [(set dRegs:$dst, (OpNode sRegs:$src))]>;

//===--------------------------------------------------------------------===//
// This class is similiar to the UnaryOp class, however, there is no
// result value to assign.
//===--------------------------------------------------------------------===//
class UnaryOpNoRet<ILOpCode op, dag outs, dag ins,
      string asmstr, list<dag> pattern>
      : ILFormat<op, outs, ins, asmstr, pattern>
{
     ILSrc       src0_reg;
     ILSrcMod    src0_mod;
     ILRelAddr   src0_rel;
     ILSrc       src0_reg_rel;
     ILSrcMod    src0_reg_rel_mod;
}

//===--------------------------------------------------------------------===//
// Set of classes that have two input parameters and one output parameter.
// The basic pattern for this class is "Opcode Dst, Src0, Src1" and
// handles the binary math operators and comparison operations.
// It sets the binary token ILSrc, ILSrcMod, ILRelAddr and ILSrc and ILSrcMod
// if the addressing is register relative for input register 1.
//===--------------------------------------------------------------------===//
class TwoInOneOut<ILOpCode op, dag outs, dag ins,
      string asmstr, list<dag> pattern>
      : OneInOneOut<op, outs, ins, asmstr, pattern>
{
     ILSrc       src1_reg;
     ILSrcMod    src1_mod;
     ILRelAddr   src1_rel;
     ILSrc       src1_reg_rel;
     ILSrcMod    src1_reg_rel_mod;
}
//===--------------------------------------------------------------------===//
// A simplification of the TwoInOneOut pattern for Binary Operations.
// This class is a helper class that assumes the simple pattern of
// $dst = op $src0 $src1.
// Other type of matching patterns need to use the TwoInOneOut class.
//===--------------------------------------------------------------------===//
class BinaryOp<ILOpCode op, SDNode OpNode, RegisterClass dReg,
      RegisterClass sReg0, RegisterClass sReg1>
      : TwoInOneOut<op, (outs dReg:$dst), (ins sReg0:$src0, sReg1:$src1),
      !strconcat(op.Text, " $dst, $src0, $src1"),
      [(set dReg:$dst, (OpNode sReg0:$src0, sReg1:$src1))]>;

//===--------------------------------------------------------------------===//
// The base class for vector extract instructions. The vector extract
// instructions take as an input value a source register and a 32bit integer
// with the same encoding as specified in InsertVectorClass and produces
// a result with only the swizzled component in the destination register.
//===--------------------------------------------------------------------===//
class ExtractVectorClass<RegisterClass DReg, RegisterClass SReg, SDNode OpNode>
: TwoInOneOut<IL_OP_MOV, (outs DReg:$dst), (ins SReg:$src0, i32imm:$src1),
     "mov $dst, $src0",
     [(set DReg:$dst, (OpNode SReg:$src0, timm:$src1))]>;

//===--------------------------------------------------------------------===//
// The base class for vector concatenation. This class creates either a vec2
// or a vec4 of 32bit data types or a vec2 of 64bit data types. This is done
// by swizzling either the 'x' or 'xy' components of the source operands
// into the destination register.
//===--------------------------------------------------------------------===//
class VectorConcatClass<RegisterClass Dst, RegisterClass Src, SDNode OpNode>
      : TwoInOneOut<IL_OP_I_ADD, (outs Dst:$dst), (ins Src:$src0, Src:$src1),
      "iadd $dst, $src0, $src1",
      [(set Dst:$dst, (OpNode Src:$src0, Src:$src1))]>;

//===--------------------------------------------------------------------===//
// Similiar to the UnaryOpNoRet class, but takes as arguments two input
// operands. Used mainly for barrier instructions on PC platform.
//===--------------------------------------------------------------------===//
class BinaryOpNoRet<ILOpCode op, dag outs, dag ins,
      string asmstr, list<dag> pattern>
      : UnaryOpNoRet<op, outs, ins, asmstr, pattern>
{
     ILSrc       src1_reg;
     ILSrcMod    src1_mod;
     ILRelAddr   src1_rel;
     ILSrc       src1_reg_rel;
     ILSrcMod    src1_reg_rel_mod;
}

//===--------------------------------------------------------------------===//
// Set of classes that have three input parameters and one output parameter.
// The basic pattern for this class is "Opcode Dst, Src0, Src1, Src2" and
// handles the mad and conditional mov instruction.
// It sets the binary token ILSrc, ILSrcMod, ILRelAddr and ILSrc and ILSrcMod
// if the addressing is register relative.
// This class is the parent class of TernaryOp
//===--------------------------------------------------------------------===//
class ThreeInOneOut<ILOpCode op, dag outs, dag ins,
      string asmstr, list<dag> pattern>
      : TwoInOneOut<op, outs, ins, asmstr, pattern> {
           ILSrc       src2_reg;
           ILSrcMod    src2_mod;
           ILRelAddr   src2_rel;
           ILSrc       src2_reg_rel;
           ILSrcMod    src2_reg_rel_mod;
      }

//===--------------------------------------------------------------------===//
// The g version of the Three Input pattern uses a standard pattern but
// but allows specification of the register to further generalize the class
// This class is mainly used in the generic multiclasses in AMDILMultiClass.td
//===--------------------------------------------------------------------===//
class TernaryOp<ILOpCode op, SDNode OpNode,
      RegisterClass dReg,
      RegisterClass sReg0,
      RegisterClass sReg1,
      RegisterClass sReg2>
      : ThreeInOneOut<op, (outs dReg:$dst),
      (ins sReg0:$src0, sReg1:$src1, sReg2:$src2),
      !strconcat(op.Text, " $dst, $src0, $src1, $src2"),
      [(set dReg:$dst,
                (OpNode sReg0:$src0, sReg1:$src1, sReg2:$src2))]>;

//===--------------------------------------------------------------------===//
// Set of classes that have three input parameters and one output parameter.
// The basic pattern for this class is "Opcode Dst, Src0, Src1, Src2" and
// handles the mad and conditional mov instruction.
// It sets the binary token ILSrc, ILSrcMod, ILRelAddr and ILSrc and ILSrcMod
// if the addressing is register relative.
// This class is the parent class of TernaryOp
//===--------------------------------------------------------------------===//
class FourInOneOut<ILOpCode op, dag outs, dag ins,
      string asmstr, list<dag> pattern>
      : ThreeInOneOut<op, outs, ins, asmstr, pattern> {
           ILSrc       src3_reg;
           ILSrcMod    src3_mod;
           ILRelAddr   src3_rel;
           ILSrc       src3_reg_rel;
           ILSrcMod    src3_reg_rel_mod;
      }


//===--------------------------------------------------------------------===//
// The macro class that is an extension of OneInOneOut but is tailored for
// macros only where all the register types are the same
//===--------------------------------------------------------------------===//
class UnaryMacro<RegisterClass Dst, RegisterClass Src0, SDNode OpNode>
: OneInOneOut<IL_OP_MACRO, (outs Dst:$dst),
     (ins Src0:$src0),
     "($dst),($src0)",
     [(set Dst:$dst, (OpNode Src0:$src0))]>;

//===--------------------------------------------------------------------===//
// The macro class is an extension of TwoInOneOut but is tailored for
// macros only where all the register types are the same
//===--------------------------------------------------------------------===//
class BinaryMacro<RegisterClass Dst,
      RegisterClass Src0,
      RegisterClass Src1,
      SDNode OpNode>
      : TwoInOneOut<IL_OP_MACRO, (outs Dst:$dst),
      (ins Src0: $src0, Src1:$src1),
      "($dst),($src0, $src1)",
      [(set Dst:$dst, (OpNode Src0:$src0, Src1:$src1))]>;

//===--------------------------------------------------------------------===//
// Classes for dealing with atomic instructions w/ 32bit pointers
//===--------------------------------------------------------------------===//
class Append<ILOpCode op, string idType, SDNode intr>
      : ILFormat<op, (outs GPRI32:$dst),
      (ins MEMI32:$id),
      !strconcat(op.Text, !strconcat(idType," $dst")),
      [(set GPRI32:$dst, (intr ADDR:$id))]>;


// TODO: Need to get this working without dst...
class AppendNoRet<ILOpCode op, string idType, SDNode intr>
      : ILFormat<op, (outs GPRI32:$dst),
      (ins MEMI32:$id),
      !strconcat(op.Text, !strconcat(idType," $dst")),
      [(set GPRI32:$dst, (intr ADDR:$id))]>;

class UniAtom<ILOpCode op, string idType, SDNode intr>
      : ILFormat<op, (outs GPRI32:$dst),
      (ins MEMI32:$ptr, i32imm:$id),
      !strconcat(op.Text, !strconcat(idType," $dst, $ptr")),
      [(set GPRI32:$dst, (intr ADDR:$ptr, timm:$id))]>;


// TODO: Need to get this working without dst...
class UniAtomNoRet<ILOpCode op, string idType, SDNode intr>
      : ILFormat<op, (outs GPRI32:$dst), (ins MEMI32:$ptr, i32imm:$id),
      !strconcat(op.Text, !strconcat(idType," $ptr")),
      [(set GPRI32:$dst, (intr ADDR:$ptr, timm:$id))]>;

class BinAtom<ILOpCode op, string idType, SDNode intr>
      : ILFormat<op, (outs GPRI32:$dst),
      (ins MEMI32:$ptr, GPRI32:$src, i32imm:$id),
      !strconcat(op.Text, !strconcat(idType," $dst, $ptr, $src")),
      [(set GPRI32:$dst, (intr ADDR:$ptr, GPRI32:$src, timm:$id))]>;


// TODO: Need to get this working without dst...
class BinAtomNoRet<ILOpCode op, string idType, SDNode intr>
      : ILFormat<op, (outs GPRI32:$dst), (ins MEMI32:$ptr, GPRI32:$src, i32imm:$id),
      !strconcat(op.Text, !strconcat(idType," $ptr, $src")),
      [(set GPRI32:$dst, (intr ADDR:$ptr, GPRI32:$src, timm:$id))]>;

class TriAtom<ILOpCode op, string idType, SDNode intr>
      : ILFormat<op, (outs GPRI32:$dst),
      (ins MEMI32:$ptr, GPRI32:$src, GPRI32:$src1, i32imm:$id),
      !strconcat(op.Text, !strconcat(idType," $dst, $ptr, $src, $src1")),
      [(set GPRI32:$dst, (intr ADDR:$ptr, GPRI32:$src, GPRI32:$src1, timm:$id))]>;

class CmpXChg<ILOpCode op, string idType, SDNode intr>
      : ILFormat<op, (outs GPRI32:$dst),
      (ins MEMI32:$ptr, GPRI32:$src, GPRI32:$src1, i32imm:$id),
      !strconcat(op.Text, !strconcat(idType," $dst, $ptr, $src1, $src")),
      [(set GPRI32:$dst, (intr ADDR:$ptr, GPRI32:$src, GPRI32:$src1, timm:$id))]>;

// TODO: Need to get this working without dst...
class TriAtomNoRet<ILOpCode op, string idType, SDNode intr>
      : ILFormat<op, (outs GPRI32:$dst),
      (ins MEMI32:$ptr, GPRI32:$src, GPRI32:$src1, i32imm:$id),
      !strconcat(op.Text, !strconcat(idType," $ptr, $src, $src1")),
      [(set GPRI32:$dst, (intr ADDR:$ptr, GPRI32:$src, GPRI32:$src1, timm:$id))]>;

// TODO: Need to get this working without dst...
class CmpXChgNoRet<ILOpCode op, string idType, SDNode intr>
      : ILFormat<op, (outs GPRI32:$dst),
      (ins MEMI32:$ptr, GPRI32:$src, GPRI32:$src1, i32imm:$id),
      !strconcat(op.Text, !strconcat(idType," $ptr, $src1, $src")),
      [(set GPRI32:$dst, (intr ADDR:$ptr, GPRI32:$src, GPRI32:$src1, timm:$id))]>;


//===--------------------------------------------------------------------===//
// Classes for dealing with atomic instructions w/ 64bit pointers
//===--------------------------------------------------------------------===//
class Append64<ILOpCode op, string idType, SDNode intr>
      : ILFormat<op, (outs GPRI32:$dst),
      (ins MEMI64:$id),
      !strconcat(op.Text, !strconcat(idType," $dst")),
      [(set GPRI32:$dst, (intr ADDR64:$id))]>;


// TODO: Need to get this working without dst...
class AppendNoRet64<ILOpCode op, string idType, SDNode intr>
      : ILFormat<op, (outs GPRI32:$dst),
      (ins MEMI64:$id),
      !strconcat(op.Text, !strconcat(idType," $dst")),
      [(set GPRI32:$dst, (intr ADDR64:$id))]>;

class UniAtom64<ILOpCode op, string idType, SDNode intr>
      : ILFormat<op, (outs GPRI32:$dst),
      (ins MEMI64:$ptr, i32imm:$id),
      !strconcat(op.Text, !strconcat(idType," $dst, $ptr")),
      [(set GPRI32:$dst, (intr ADDR64:$ptr, timm:$id))]>;


// TODO: Need to get this working without dst...
class UniAtomNoRet64<ILOpCode op, string idType, SDNode intr>
      : ILFormat<op, (outs GPRI32:$dst), (ins MEMI64:$ptr, i32imm:$id),
      !strconcat(op.Text, !strconcat(idType," $ptr")),
      [(set GPRI32:$dst, (intr ADDR64:$ptr, timm:$id))]>;

class BinAtom64<ILOpCode op, string idType, SDNode intr>
      : ILFormat<op, (outs GPRI32:$dst),
      (ins MEMI64:$ptr, GPRI32:$src, i32imm:$id),
      !strconcat(op.Text, !strconcat(idType," $dst, $ptr, $src")),
      [(set GPRI32:$dst, (intr ADDR64:$ptr, GPRI32:$src, timm:$id))]>;


// TODO: Need to get this working without dst...
class BinAtomNoRet64<ILOpCode op, string idType, SDNode intr>
      : ILFormat<op, (outs GPRI32:$dst), (ins MEMI64:$ptr, GPRI32:$src, i32imm:$id),
      !strconcat(op.Text, !strconcat(idType," $ptr, $src")),
      [(set GPRI32:$dst, (intr ADDR64:$ptr, GPRI32:$src, timm:$id))]>;

class TriAtom64<ILOpCode op, string idType, SDNode intr>
      : ILFormat<op, (outs GPRI32:$dst),
      (ins MEMI64:$ptr, GPRI32:$src, GPRI32:$src1, i32imm:$id),
      !strconcat(op.Text, !strconcat(idType," $dst, $ptr, $src, $src1")),
      [(set GPRI32:$dst, (intr ADDR64:$ptr, GPRI32:$src, GPRI32:$src1, timm:$id))]>;

class CmpXChg64<ILOpCode op, string idType, SDNode intr>
      : ILFormat<op, (outs GPRI32:$dst),
      (ins MEMI64:$ptr, GPRI32:$src, GPRI32:$src1, i32imm:$id),
      !strconcat(op.Text, !strconcat(idType," $dst, $ptr, $src1, $src")),
      [(set GPRI32:$dst, (intr ADDR64:$ptr, GPRI32:$src, GPRI32:$src1, timm:$id))]>;

// TODO: Need to get this working without dst...
class TriAtomNoRet64<ILOpCode op, string idType, SDNode intr>
      : ILFormat<op, (outs GPRI32:$dst),
      (ins MEMI64:$ptr, GPRI32:$src, GPRI32:$src1, i32imm:$id),
      !strconcat(op.Text, !strconcat(idType," $ptr, $src, $src1")),
      [(set GPRI32:$dst, (intr ADDR64:$ptr, GPRI32:$src, GPRI32:$src1, timm:$id))]>;

// TODO: Need to get this working without dst...
class CmpXChgNoRet64<ILOpCode op, string idType, SDNode intr>
      : ILFormat<op, (outs GPRI32:$dst),
      (ins MEMI64:$ptr, GPRI32:$src, GPRI32:$src1, i32imm:$id),
      !strconcat(op.Text, !strconcat(idType," $ptr, $src1, $src")),
      [(set GPRI32:$dst, (intr ADDR64:$ptr, GPRI32:$src, GPRI32:$src1, timm:$id))]>;

//===--------------------------------------------------------------------===//
// Intrinsic classes
// Generic versions of the above classes but for Target specific intrinsics
// instead of SDNode patterns.
//===--------------------------------------------------------------------===//
let TargetPrefix = "AMDIL", isTarget = 1 in {
     class VoidIntLong :
          Intrinsic<[llvm_i64_ty], [], []>;
     class VoidIntInt :
          Intrinsic<[llvm_i32_ty], [], []>;
     class VoidIntBool :
          Intrinsic<[llvm_i32_ty], [], []>;
     class UnaryIntInt :
          Intrinsic<[llvm_anyint_ty], [LLVMMatchType<0>], []>;
     class UnaryIntFloat :
          Intrinsic<[llvm_anyfloat_ty], [LLVMMatchType<0>], []>;
     class ConvertIntFTOI :
          Intrinsic<[llvm_anyint_ty], [llvm_anyfloat_ty], []>;
     class ConvertIntITOF :
          Intrinsic<[llvm_anyfloat_ty], [llvm_anyint_ty], []>;
     class UnaryIntNoRetInt :
          Intrinsic<[], [llvm_anyint_ty], []>;
     class UnaryIntNoRetFloat :
          Intrinsic<[], [llvm_anyfloat_ty], []>;
     class BinaryIntInt :
          Intrinsic<[llvm_anyint_ty], [LLVMMatchType<0>, LLVMMatchType<0>], []>;
     class BinaryIntFloat :
          Intrinsic<[llvm_anyfloat_ty], [LLVMMatchType<0>, LLVMMatchType<0>], []>;
     class BinaryIntNoRetInt :
          Intrinsic<[], [llvm_anyint_ty, LLVMMatchType<0>], []>;
     class BinaryIntNoRetFloat :
          Intrinsic<[], [llvm_anyfloat_ty, LLVMMatchType<0>], []>;
     class TernaryIntInt :
          Intrinsic<[llvm_anyint_ty], [LLVMMatchType<0>,
          LLVMMatchType<0>, LLVMMatchType<0>], []>;
     class TernaryIntFloat :
          Intrinsic<[llvm_anyfloat_ty], [LLVMMatchType<0>,
          LLVMMatchType<0>, LLVMMatchType<0>], []>;
     class QuaternaryIntInt :
          Intrinsic<[llvm_anyint_ty], [LLVMMatchType<0>,
          LLVMMatchType<0>, LLVMMatchType<0>, LLVMMatchType<0>], []>;
     class UnaryAtomicInt :
          Intrinsic<[llvm_i32_ty], [llvm_ptr_ty, llvm_i32_ty], [IntrReadWriteArgMem]>;
     class BinaryAtomicInt :
          Intrinsic<[llvm_i32_ty], [llvm_ptr_ty, llvm_i32_ty, llvm_i32_ty], [IntrReadWriteArgMem]>;
     class TernaryAtomicInt :
          Intrinsic<[llvm_i32_ty], [llvm_ptr_ty, llvm_i32_ty, llvm_i32_ty, llvm_i32_ty]>;
     class UnaryAtomicIntNoRet :
          Intrinsic<[], [llvm_ptr_ty, llvm_i32_ty], [IntrReadWriteArgMem]>;
     class BinaryAtomicIntNoRet :
          Intrinsic<[], [llvm_ptr_ty, llvm_i32_ty, llvm_i32_ty], [IntrReadWriteArgMem]>;
     class TernaryAtomicIntNoRet :
          Intrinsic<[], [llvm_ptr_ty, llvm_i32_ty, llvm_i32_ty, llvm_i32_ty], [IntrReadWriteArgMem]>;
}

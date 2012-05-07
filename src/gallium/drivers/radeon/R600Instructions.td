//===-- R600Instructions.td - TODO: Add brief description -------===//
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

include "R600Intrinsics.td"

class InstR600 <bits<32> inst, dag outs, dag ins, string asm, list<dag> pattern,
                InstrItinClass itin>
    : AMDGPUInst <outs, ins, asm, pattern> {

  field bits<32> Inst;
	bit Trig = 0;
  bit Op3 = 0;

  let Inst = inst;
  let Namespace = "AMDIL";
  let OutOperandList = outs;
  let InOperandList = ins;
  let AsmString = asm;
  let Pattern = pattern;
  let Itinerary = itin;

  let TSFlags{4} = Trig;
  let TSFlags{5} = Op3;
}

class InstR600ISA <dag outs, dag ins, string asm, list<dag> pattern> :
    AMDGPUInst <outs, ins, asm, pattern>
{
  field bits<64> Inst;

  let Namespace = "AMDIL";
}

def MEMri : Operand<iPTRAny> {
  let MIOperandInfo = (ops R600_Reg32:$ptr, R600_Reg32:$index);
}

def ADDRParam : ComplexPattern<i32, 2, "SelectADDRParam", [], []>;

class R600_ALU {

  bits<7> DST_GPR = 0;
  bits<9> SRC0_SEL = 0;
  bits<1> SRC0_NEG = 0;
  bits<9> SRC1_SEL = 0;
  bits<1> SRC1_NEG = 0;
  bits<1> CLAMP = 0;
  
}


class R600_1OP <bits<32> inst, string opName, list<dag> pattern,
                InstrItinClass itin = AnyALU> :
  InstR600 <inst,
          (outs R600_Reg32:$dst),
          (ins R600_Reg32:$src, variable_ops),
          !strconcat(opName, " $dst, $src"),
          pattern,
          itin
  >;

class R600_2OP <bits<32> inst, string opName, list<dag> pattern,
                InstrItinClass itin = AnyALU> :
  InstR600 <inst,
          (outs R600_Reg32:$dst),
          (ins R600_Reg32:$src0, R600_Reg32:$src1, variable_ops),
          !strconcat(opName, " $dst, $src0, $src1"),
          pattern,
          itin
  >;

class R600_3OP <bits<32> inst, string opName, list<dag> pattern,
                InstrItinClass itin = AnyALU> :
  InstR600 <inst,
          (outs R600_Reg32:$dst),
          (ins R600_Reg32:$src0, R600_Reg32:$src1, R600_Reg32:$src2, variable_ops),
          !strconcat(opName, " $dst, $src0, $src1, $src2"),
          pattern,
          itin>{

    let Op3 = 1;
  }

class R600_REDUCTION <bits<32> inst, dag ins, string asm, list<dag> pattern,
                      InstrItinClass itin = AnyALU> :
  InstR600 <inst,
          (outs R600_Reg32:$dst),
          ins,
          asm,
          pattern,
          itin

  >;

class R600_TEX <bits<32> inst, string opName, list<dag> pattern,
                InstrItinClass itin = AnyALU> :
  InstR600 <inst,
          (outs R600_Reg128:$dst),
          (ins R600_Reg128:$src0, i32imm:$src1, i32imm:$src2),
          !strconcat(opName, "$dst, $src0, $src1, $src2"),
          pattern,
          itin
  >;

def TEX_SHADOW : PatLeaf<
  (imm),
  [{uint32_t TType = (uint32_t)N->getZExtValue();
    return (TType >= 6 && TType <= 8) || TType == 11 || TType == 12;
  }]
>;

class EG_CF_RAT <bits <8> cf_inst, bits <6> rat_inst, dag outs, dag ins,
                 string asm> :
    InstR600ISA <outs, ins, asm, []>
{
  bits<7>  RW_GPR;
  bits<7>  INDEX_GPR;
  bits<4>  RAT_ID;

  bits<2>  RIM;
  bits<2>  TYPE;
  bits<1>  RW_REL;
  bits<2>  ELEM_SIZE;

  bits<12> ARRAY_SIZE;
  bits<4>  COMP_MASK;
  bits<4>  BURST_COUNT;
  bits<1>  VPM;
  bits<1>  EOP;
  bits<1>  MARK;
  bits<1>  BARRIER;

  /* CF_ALLOC_EXPORT_WORD0_RAT */
  let Inst{3-0}   = RAT_ID;
  let Inst{9-4}   = rat_inst;
  let Inst{10}    = 0; /* Reserved */
  let Inst{12-11} = RIM;
  let Inst{14-13} = TYPE;
  let Inst{21-15} = RW_GPR;
  let Inst{22}    = RW_REL;
  let Inst{29-23} = INDEX_GPR;
  let Inst{31-30} = ELEM_SIZE;

  /* CF_ALLOC_EXPORT_WORD1_BUF */
/* XXX: We can't have auto encoding of 64-bit instructions until LLVM 3.1 :( */
/*
  let Inst{43-32} = ARRAY_SIZE;
  let Inst{47-44} = COMP_MASK;
  let Inst{51-48} = BURST_COUNT;
  let Inst{52}    = VPM;
  let Inst{53}    = EOP;
  let Inst{61-54} = cf_inst;
  let Inst{62}    = MARK;
  let Inst{63}    = BARRIER;
*/
}

/*
def store_global : PatFrag<(ops node:$value, node:$ptr),
                           (store node:$value, node:$ptr),
                           [{
                            const Value *Src;
                            const PointerType *Type;
                            if ((src = cast<StoreSDNode>(N)->getSrcValue() &&
                                 PT = dyn_cast<PointerType>(Src->getType()))) {
                              return PT->getAddressSpace() == 1;
                            }
                            return false;
                           }]>;

*/

def load_param : PatFrag<(ops node:$ptr),
                         (load node:$ptr),
                          [{
                           return true;
                           const Value *Src = cast<LoadSDNode>(N)->getSrcValue();
                           if (Src) {
                                PointerType * PT = dyn_cast<PointerType>(Src->getType());
                                return PT && PT->getAddressSpace() == AMDILAS::PARAM_I_ADDRESS;
                           }
                           return false;
                          }]>;

//class EG_CF <bits<32> inst, string asm> :
//    InstR600 <inst, (outs), (ins), asm, []>;

/* XXX: We will use this when we emit the real ISA.
  bits<24> ADDR = 0;
  bits<3> JTS = 0;

  bits<3> PC = 0;
  bits<5> CF_CONS = 0;
  bits<2> COND = 0;
  bits<6> COUNT = 0;
  bits<1> VPM = 0;
  bits<1> EOP = 0;
  bits<8> CF_INST = 0;
  bits<1> WQM = 0;
  bits<1> B = 0;

  let Inst{23-0} = ADDR;
  let Inst{26-24} = JTS;
  let Inst{34-32} = PC;
  let Inst{39-35} = CF_CONST;
  let Inst{41-40} = COND;
  let Inst{47-42} = COUNT;
  let Inst{52} = VPM;
  let Inst{53} = EOP;
  let Inst{61-54} = CF_INST;
  let Inst{62} = WQM;
  let Inst{63} = B;
//}
*/
def isR600 : Predicate<"Subtarget.device()"
                            "->getGeneration() == AMDILDeviceInfo::HD4XXX">;
def isEG : Predicate<"Subtarget.device()"
                            "->getGeneration() >= AMDILDeviceInfo::HD5XXX && "
                            "Subtarget.device()->getDeviceFlag() != OCL_DEVICE_CAYMAN">;
def isCayman : Predicate<"Subtarget.device()"
                            "->getDeviceFlag() == OCL_DEVICE_CAYMAN">;
def isEGorCayman : Predicate<"Subtarget.device()"
                            "->getGeneration() >= AMDILDeviceInfo::HD5XXX">;

def isR600toCayman : Predicate<
                     "Subtarget.device()->getGeneration() <= AMDILDeviceInfo::HD6XXX">;


let Predicates = [isR600toCayman] in { 

/* ------------------------------------------- */
/* Common Instructions R600, R700, Evergreen, Cayman */
/* ------------------------------------------- */
let Gen = AMDGPUGen.R600_CAYMAN  in {

def ADD : R600_2OP <
  0x0, "ADD",
  [(set R600_Reg32:$dst, (fadd R600_Reg32:$src0, R600_Reg32:$src1))] > {
  let AMDILOp = AMDILInst.ADD_f32;
}
// Non-IEEE MUL: 0 * anything = 0
def MUL : R600_2OP <
  0x1, "MUL NON-IEEE",
  [(set R600_Reg32:$dst, (int_AMDGPU_mul R600_Reg32:$src0, R600_Reg32:$src1))]
>;

def MUL_IEEE : R600_2OP <
  0x2, "MUL_IEEE",
  [(set R600_Reg32:$dst, (fmul R600_Reg32:$src0, R600_Reg32:$src1))]> {
  let AMDILOp = AMDILInst.MUL_IEEE_f32;
}

def MAX : R600_2OP <
  0x3, "MAX",
  [(set R600_Reg32:$dst, (int_AMDIL_max R600_Reg32:$src0, R600_Reg32:$src1))]> {
  let AMDILOp = AMDILInst.MAX_f32;
}

def MIN : R600_2OP <
  0x4, "MIN",
  [(set R600_Reg32:$dst, (int_AMDIL_min R600_Reg32:$src0, R600_Reg32:$src1))]> {
  let AMDILOp = AMDILInst.MIN_f32;
}

/* For the SET* instructions there is a naming conflict in TargetSelectionDAG.td,
 * so some of the instruction names don't match the asm string.
 * XXX: Use the defs in TargetSelectionDAG.td instead of intrinsics.
 */

def SETE : R600_2OP <
  0x08, "SETE",
  [(set R600_Reg32:$dst, (int_AMDGPU_seq R600_Reg32:$src0, R600_Reg32:$src1))]> {
  let AMDILOp = AMDILInst.FEQ;
}

def SGT : R600_2OP <
  0x09, "SETGT",
  [(set R600_Reg32:$dst, (int_AMDGPU_sgt R600_Reg32:$src0, R600_Reg32:$src1))]
>;

def SGE : R600_2OP <
  0xA, "SETGE",
  [(set R600_Reg32:$dst, (int_AMDGPU_sge R600_Reg32:$src0, R600_Reg32:$src1))]> {
  let AMDILOp = AMDILInst.FGE;
}

def SNE : R600_2OP <
  0xB, "SETNE",
  [(set R600_Reg32:$dst, (int_AMDGPU_sne R600_Reg32:$src0, R600_Reg32:$src1))]> {
  let AMDILOp = AMDILInst.FNE;
}

def FRACT : R600_1OP <
  0x10, "FRACT",
  []> {
  let AMDILOp = AMDILInst.FRAC_f32;
}

def TRUNC : R600_1OP <
  0x11, "TRUNC",
  [(set R600_Reg32:$dst, (int_AMDGPU_trunc R600_Reg32:$src))]
>;

def CEIL : R600_1OP <
  0x12, "CEIL",
  [(set R600_Reg32:$dst, (int_AMDIL_round_neginf R600_Reg32:$src))]> {
  let AMDILOp = AMDILInst.ROUND_NEGINF_f32;
}

def RNDNE : R600_1OP <
  0x13, "RNDNE",
  [(set R600_Reg32:$dst, (int_AMDIL_round_nearest R600_Reg32:$src))]> {
  let AMDILOp = AMDILInst.ROUND_NEAREST_f32;
}

def FLOOR : R600_1OP <
  0x14, "FLOOR",
  [(set R600_Reg32:$dst, (int_AMDGPU_floor R600_Reg32:$src))]
>;

def MOV : R600_1OP <0x19, "MOV", []>;

def KILLGT : R600_2OP <
  0x2D, "KILLGT",
  []
>;

def AND_INT : R600_2OP <
  0x30, "AND_INT",
  []> {
  let AMDILOp = AMDILInst.AND_i32;
}

def OR_INT : R600_2OP <
  0x31, "OR_INT",
  []>{
  let AMDILOp = AMDILInst.BINARY_OR_i32;
}

def XOR_INT : R600_2OP <
  0x32, "XOR_INT",
  []
>;

def NOT_INT : R600_1OP <
  0x33, "NOT_INT",
  []>{
  let AMDILOp = AMDILInst.BINARY_NOT_i32;
}

def ADD_INT : R600_2OP <
  0x34, "ADD_INT",
  []>{
  let AMDILOp = AMDILInst.ADD_i32;
}

def SUB_INT : R600_2OP <
	0x35, "SUB_INT",
	[]
>;

def MAX_INT : R600_2OP <
  0x36, "MAX_INT",
  [(set R600_Reg32:$dst, (int_AMDGPU_imax R600_Reg32:$src0, R600_Reg32:$src1))]>;

def MIN_INT : R600_2OP <
  0x37, "MIN_INT",
  [(set R600_Reg32:$dst, (int_AMDGPU_imin R600_Reg32:$src0, R600_Reg32:$src1))]>;

def MAX_UINT : R600_2OP <
  0x38, "MAX_UINT",
  [(set R600_Reg32:$dst, (int_AMDGPU_umax R600_Reg32:$src0, R600_Reg32:$src1))]>;

def MIN_UINT : R600_2OP <
  0x39, "MIN_UINT",
  [(set R600_Reg32:$dst, (int_AMDGPU_umin R600_Reg32:$src0, R600_Reg32:$src1))]>;


def SETE_INT : R600_2OP <
  0x3A, "SETE_INT",
  []>{
  let AMDILOp = AMDILInst.IEQ;
}

def SETGT_INT : R600_2OP <
  0x3B, "SGT_INT",
  []
>;

def SETGE_INT : R600_2OP <
	0x3C, "SETGE_INT",
	[]>{
  let AMDILOp = AMDILInst.IGE;
}

def SETNE_INT : R600_2OP <
  0x3D, "SETNE_INT",
  []>{
  let AMDILOp = AMDILInst.INE;
}

def SETGT_UINT : R600_2OP <
  0x3E, "SETGT_UINT",
  []>{
  let AMDILOp = AMDILInst.UGT;
}

def SETGE_UINT : R600_2OP <
  0x3F, "SETGE_UINT",
  []>{
  let AMDILOp = AMDILInst.UGE;
}

def CNDE_INT : R600_3OP <
	0x1C, "CNDE_INT",
	[]
>;

/* Texture instructions */


def TEX_LD : R600_TEX <
  0x03, "TEX_LD",
  [(set R600_Reg128:$dst, (int_AMDGPU_txf R600_Reg128:$src0, imm:$src1, imm:$src2))]
>;

def TEX_GET_TEXTURE_RESINFO : R600_TEX <
  0x04, "TEX_GET_TEXTURE_RESINFO",
  [(set R600_Reg128:$dst, (int_AMDGPU_txq R600_Reg128:$src0, imm:$src1, imm:$src2))]
>;

def TEX_GET_GRADIENTS_H : R600_TEX <
  0x07, "TEX_GET_GRADIENTS_H",
  [(set R600_Reg128:$dst, (int_AMDGPU_ddx R600_Reg128:$src0, imm:$src1, imm:$src2))]
>;

def TEX_GET_GRADIENTS_V : R600_TEX <
  0x08, "TEX_GET_GRADIENTS_V",
  [(set R600_Reg128:$dst, (int_AMDGPU_ddy R600_Reg128:$src0, imm:$src1, imm:$src2))]
>;

def TEX_SAMPLE : R600_TEX <
  0x10, "TEX_SAMPLE",
  [(set R600_Reg128:$dst, (int_AMDGPU_tex R600_Reg128:$src0, imm:$src1, imm:$src2))]
>;

def TEX_SAMPLE_C : R600_TEX <
  0x18, "TEX_SAMPLE_C",
  [(set R600_Reg128:$dst, (int_AMDGPU_tex R600_Reg128:$src0, imm:$src1, TEX_SHADOW:$src2))]
>;

def TEX_SAMPLE_L : R600_TEX <
  0x11, "TEX_SAMPLE_L",
  [(set R600_Reg128:$dst, (int_AMDGPU_txl R600_Reg128:$src0, imm:$src1, imm:$src2))]
>;

def TEX_SAMPLE_C_L : R600_TEX <
  0x19, "TEX_SAMPLE_C_L",
  [(set R600_Reg128:$dst, (int_AMDGPU_txl R600_Reg128:$src0, imm:$src1, TEX_SHADOW:$src2))]
>;

def TEX_SAMPLE_LB : R600_TEX <
  0x12, "TEX_SAMPLE_LB",
  [(set R600_Reg128:$dst, (int_AMDGPU_txb R600_Reg128:$src0, imm:$src1, imm:$src2))]
>;

def TEX_SAMPLE_C_LB : R600_TEX <
  0x1A, "TEX_SAMPLE_C_LB",
  [(set R600_Reg128:$dst, (int_AMDGPU_txb R600_Reg128:$src0, imm:$src1, TEX_SHADOW:$src2))]
>;

def TEX_SAMPLE_G : R600_TEX <
  0x14, "TEX_SAMPLE_G",
  [(set R600_Reg128:$dst, (int_AMDGPU_txd R600_Reg128:$src0, imm:$src1, imm:$src2))]
>;

def TEX_SAMPLE_C_G : R600_TEX <
  0x1C, "TEX_SAMPLE_C_G",
  [(set R600_Reg128:$dst, (int_AMDGPU_txd R600_Reg128:$src0, imm:$src1, TEX_SHADOW:$src2))]
>;

} // End Gen R600_CAYMAN

def KILP : Pat <
  (int_AMDGPU_kilp),
  (MASK_WRITE (KILLGT (f32 ONE), (f32 ZERO)))
>;

def KIL : Pat <
  (int_AMDGPU_kill R600_Reg32:$src0),
  (MASK_WRITE (KILLGT (f32 ZERO), (f32 R600_Reg32:$src0)))
>;

/* Helper classes for common instructions */

class MUL_LIT_Common <bits<32> inst> : R600_3OP <
  inst, "MUL_LIT",
  []
>;

class MULADD_Common <bits<32> inst> : R600_3OP <
  inst, "MULADD",
  []> {
  let AMDILOp = AMDILInst.MAD_f32;
}

class CNDE_Common <bits<32> inst> : R600_3OP <
  inst, "CNDE",
  []> {
  let AMDILOp = AMDILInst.CMOVLOG_f32;
}

class CNDGT_Common <bits<32> inst> : R600_3OP <
  inst, "CNDGT",
  []
>;
  
class CNDGE_Common <bits<32> inst> : R600_3OP <
  inst, "CNDGE",
  [(set R600_Reg32:$dst, (int_AMDGPU_cndlt R600_Reg32:$src0, R600_Reg32:$src2, R600_Reg32:$src1))]
>;

class DOT4_Common <bits<32> inst> : R600_REDUCTION <
  inst,
  (ins R600_Reg128:$src0, R600_Reg128:$src1),
  "DOT4 $dst $src0, $src1",
  [(set R600_Reg32:$dst, (int_AMDGPU_dp4 R600_Reg128:$src0, R600_Reg128:$src1))]
>;

class EXP_IEEE_Common <bits<32> inst> : R600_1OP <
  inst, "EXP_IEEE",
  []> {
  let AMDILOp = AMDILInst.EXP_f32;
}

class FLT_TO_INT_Common <bits<32> inst> : R600_1OP <
  inst, "FLT_TO_INT", []> {
  let AMDILOp = AMDILInst.FTOI;
}

class INT_TO_FLT_Common <bits<32> inst> : R600_1OP <
  inst, "INT_TO_FLT", []> {
  let AMDILOp = AMDILInst.ITOF;
}

class LOG_CLAMPED_Common <bits<32> inst> : R600_1OP <
  inst, "LOG_CLAMPED",
  []
>;

class LOG_IEEE_Common <bits<32> inst> : R600_1OP <
  inst, "LOG_IEEE",
  []> {
  let AMDILOp = AMDILInst.LOG_f32;
}

class LSHL_Common <bits<32> inst> : R600_2OP <
  inst, "LSHL $dst, $src0, $src1",
  [] >{
  let AMDILOp = AMDILInst.SHL_i32;
}

class LSHR_Common <bits<32> inst> : R600_2OP <
  inst, "LSHR $dst, $src0, $src1",
  [] >{
  let AMDILOp = AMDILInst.USHR_i32;
}

class ASHR_Common <bits<32> inst> : R600_2OP <
  inst, "ASHR $dst, $src0, $src1",
  [] >{
  let AMDILOp = AMDILInst.SHR_i32;
}

class MULHI_INT_Common <bits<32> inst> : R600_2OP <
  inst, "MULHI_INT $dst, $src0, $src1",
  [] >{
  let AMDILOp = AMDILInst.SMULHI_i32;
}

class MULHI_UINT_Common <bits<32> inst> : R600_2OP <
	inst, "MULHI $dst, $src0, $src1",
	[]
>;

class MULLO_INT_Common <bits<32> inst> : R600_2OP <
  inst, "MULLO_INT $dst, $src0, $src1",
  [] >{
  let AMDILOp = AMDILInst.SMUL_i32;
}

class MULLO_UINT_Common <bits<32> inst> : R600_2OP <
  inst, "MULLO_UINT $dst, $src0, $src1",
  []
>;

class RECIP_CLAMPED_Common <bits<32> inst> : R600_1OP <
  inst, "RECIP_CLAMPED",
  []
>;

class RECIP_IEEE_Common <bits<32> inst> : R600_1OP <
  inst, "RECIP_IEEE",
  [(set R600_Reg32:$dst, (int_AMDGPU_rcp R600_Reg32:$src))]> {
  let AMDILOp = AMDILInst.RSQ_f32;
}

class RECIP_UINT_Common <bits<32> inst> : R600_1OP <
  inst, "RECIP_INT $dst, $src",
  []
>;

class RECIPSQRT_CLAMPED_Common <bits<32> inst> : R600_1OP <
  inst, "RECIPSQRT_CLAMPED",
  [(set R600_Reg32:$dst, (int_AMDGPU_rsq R600_Reg32:$src))]
>;

class RECIPSQRT_IEEE_Common <bits<32> inst> : R600_1OP <
  inst, "RECIPSQRT_IEEE",
  []
>;

class SIN_Common <bits<32> inst> : R600_1OP <
  inst, "SIN",
  []>{
  let AMDILOp = AMDILInst.SIN_f32;
  let Trig = 1;
}

class COS_Common <bits<32> inst> : R600_1OP <
  inst, "COS",
  []> {
  let AMDILOp = AMDILInst.COS_f32;
  let Trig = 1;
}

/* Helper patterns for complex intrinsics */
/* -------------------------------------- */

class DIV_Common <InstR600 recip_ieee> : Pat<
  (int_AMDGPU_div R600_Reg32:$src0, R600_Reg32:$src1),
  (MUL R600_Reg32:$src0, (recip_ieee R600_Reg32:$src1))
>;

class LRP_Common <InstR600 muladd> : Pat <
  (int_AMDGPU_lrp R600_Reg32:$src0, R600_Reg32:$src1, R600_Reg32:$src2),
  (muladd R600_Reg32:$src0, R600_Reg32:$src1, (MUL (SUB_f32 ONE, R600_Reg32:$src0), R600_Reg32:$src2))
>;

class SSG_Common <InstR600 cndgt, InstR600 cndge> : Pat <
  (int_AMDGPU_ssg R600_Reg32:$src),
  (cndgt R600_Reg32:$src, (f32 ONE), (cndge R600_Reg32:$src, (f32 ZERO), (f32 NEG_ONE)))
>;

class TGSI_LIT_Z_Common <InstR600 mul_lit, InstR600 log_clamped, InstR600 exp_ieee> : Pat <
  (int_TGSI_lit_z R600_Reg32:$src_x, R600_Reg32:$src_y, R600_Reg32:$src_w),
  (exp_ieee (mul_lit (log_clamped (MAX R600_Reg32:$src_y, (f32 ZERO))), R600_Reg32:$src_w, R600_Reg32:$src_x))
>;

/* ---------------------- */
/* R600 / R700 Only Instructions */
/* ---------------------- */

let Predicates = [isR600] in {

let Gen = AMDGPUGen.R600 in {

  def MUL_LIT_r600 : MUL_LIT_Common<0x0C>;
  def MULADD_r600 : MULADD_Common<0x10>;
  def CNDE_r600 : CNDE_Common<0x18>;
  def CNDGT_r600 : CNDGT_Common<0x19>;
  def CNDGE_r600 : CNDGE_Common<0x1A>;
  def DOT4_r600 : DOT4_Common<0x50>;
  def EXP_IEEE_r600 : EXP_IEEE_Common<0x61>;
  def LOG_CLAMPED_r600 : LOG_CLAMPED_Common<0x62>;
  def LOG_IEEE_r600 : LOG_IEEE_Common<0x63>;
  def RECIP_CLAMPED_r600 : RECIP_CLAMPED_Common<0x64>;
  def RECIP_IEEE_r600 : RECIP_IEEE_Common<0x66>;
  def RECIPSQRT_CLAMPED_r600 : RECIPSQRT_CLAMPED_Common<0x67>;
  def RECIPSQRT_IEEE_r600 : RECIPSQRT_IEEE_Common<0x69>;
  def FLT_TO_INT_r600 : FLT_TO_INT_Common<0x6b>;
  def INT_TO_FLT_r600 : INT_TO_FLT_Common<0x6c>;
  def SIN_r600 : SIN_Common<0x6E>;
  def COS_r600 : COS_Common<0x6F>;
  def ASHR_r600 : ASHR_Common<0x70>;
  def LSHR_r600 : LSHR_Common<0x71>;
  def LSHL_r600 : LSHL_Common<0x72>;
  def MULLO_INT_r600 : MULLO_INT_Common<0x73>;
  def MULHI_INT_r600 : MULHI_INT_Common<0x74>;
  def MULLO_UINT_r600 : MULLO_UINT_Common<0x75>;
  def MULHI_UINT_r600 : MULHI_UINT_Common<0x76>;
  def RECIP_UINT_r600 : RECIP_UINT_Common <0x77>;

} // End AMDGPUGen.R600

  def DIV_r600 : DIV_Common<RECIP_IEEE_r600>;
  def LRP_r600 : LRP_Common<MULADD_r600>;
  def POW_r600 : POW_Common<LOG_IEEE_r600, EXP_IEEE_r600, MUL, GPRF32>;
  def SSG_r600 : SSG_Common<CNDGT_r600, CNDGE_r600>;
  def TGSI_LIT_Z_r600 : TGSI_LIT_Z_Common<MUL_LIT_r600, LOG_CLAMPED_r600, EXP_IEEE_r600>;

}

/* ----------------- */
/* R700+ Trig helper */
/* ----------------- */

/*
class TRIG_HELPER_r700 <InstR600 trig_inst>: Pat <
  (trig_inst R600_Reg32:$src),
  (trig_inst (fmul R600_Reg32:$src, (PI))))
>;
*/

/* ---------------------- */
/* Evergreen Instructions */
/* ---------------------- */


let Predicates = [isEG] in {

let Gen = AMDGPUGen.EG in {

def RAT_WRITE_CACHELESS_eg :
    EG_CF_RAT <0x57, 0x2, (outs), (ins R600_TReg32_X:$rw_gpr,
                                   R600_TReg32_X:$index_gpr, i32imm:$rat_id), "">
{
/*
  let Inst{3-0}   = RAT_ID;
  let Inst{21-15} = RW_GPR;
  let Inst{29-23} = INDEX_GPR;
  /* Propery of the UAV */
  let Inst{31-30} = ELEM_SIZE;
*/
  let RIM         = 0;
  /* XXX: Have a separate instruction for non-indexed writes. */
  let TYPE        = 1;
  let RW_REL      = 0;
  let ELEM_SIZE   = 0;

/*
  let ARRAY_SIZE  = 0;
  let COMP_MASK   = 1;
  let BURST_COUNT = 0;
  let VPM         = 0;
  let EOP         = 0;
  let MARK        = 0;
  let BARRIER     = 1;
*/
}

def VTX_READ_eg : InstR600ISA < (outs R600_TReg32_X:$dst),
                                (ins R600_TReg32_X:$src, i32imm:$buffer_id),
                                "VTX_READ_eg $dst, $src", []>
{
/*
  bits<7> DST_GPR;
  bits<7> SRC_GPR;
  bits<8> BUFFER_ID;
*/
  /* If any of these field below need to be calculated at compile time, and
   * a ins operand for them and move them to the list of operands above. */

  /* XXX: This instruction is manual encoded, so none of these values are used.
   */
/*
  bits<5> VC_INST          = 0; //VC_INST_FETCH
  bits<2> FETCH_TYPE       = 2;
  bits<1> FETCH_WHOLE_QUAD = 1;
  bits<1> SRC_REL          = 0;
  bits<2> SRC_SEL_X        = 0;
  bits<6> MEGA_FETCH_COUNT = 4;
*/
/*

  bits<1> DST_REL          = 0;
  bits<3> DST_SEL_X        = 0;
  bits<3> DST_SEL_Y        = 7; //Masked
  bits<3> DST_SEL_Z        = 7; //Masked
  bits<3> DST_SEL_W        = 7; //Masked
  bits<1> USE_CONST_FIELDS = 1; //Masked
  bits<6> DATA_FORMAT      = 0;
  bits<2> NUM_FORMAT_ALL   = 0;
  bits<1> FORMAT_COMP_ALL  = 0;
  bits<1> SRF_MODE_ALL     = 0;
*/

/*
  let Inst{4-0}   = VC_INST;
  let Inst{6-5}   = FETCH_TYPE;
  let Inst{7}     = FETCH_WHOLE_QUAD;
  let Inst{15-8}  = BUFFER_ID;
  let Inst{22-16} = SRC_GPR;
  let Inst{23}    = SRC_REL;
  let Inst{25-24} = SRC_SEL_X;
  let Inst{31-26} = MEGA_FETCH_COUNT;
*/
  /* DST_GPR is OK to leave uncommented, because LLVM 3.0 only prevents you
   * from statically setting bits > 31.  This field will be set by
   * getMachineValueOp which can set bits > 31.
   */
//  let Inst{32-38} = DST_GPR;

  /* XXX: Uncomment for LLVM 3.1 which supports 64-bit instructions */

/*
  let Inst{39}    = DST_REL;
  let Inst{40}    = 0; //Reserved
  let Inst{43-41} = DST_SEL_X;
  let Inst{46-44} = DST_SEL_Y;
  let Inst{49-47} = DST_SEL_Z;
  let Inst{52-50} = DST_SEL_W;
  let Inst{53}    = USE_CONST_FIELDS;
  let Inst{59-54} = DATA_FORMAT;
  let Inst{61-60} = NUM_FORMAT_ALL;
  let Inst{62}    = FORMAT_COMP_ALL;
  let Inst{63}    = SRF_MODE_ALL;
*/
}



} // End AMDGPUGen.EG
/* XXX: Need to convert PTR to rat_id */
/*
def : Pat <(store_global (f32 R600_Reg32:$value), node:$ptr),
           (RAT_WRITE_CACHELESS_eg (INSERT_SUBREG (v4f32 (IMPLICIT_DEF)),
                                                  (f32 R600_Reg32:$value),
                                                   sel_x),
                                    (f32 ZERO), 0, R600_Reg32:$ptr)>;
*/

class VTX_Param_Read_Pattern <ValueType vt> : Pat <
    (vt (load_param ADDRParam:$mem)),
    (VTX_READ_eg (i32 R600_Reg32:$mem), 0)>;

def : VTX_Param_Read_Pattern <f32>;
def : VTX_Param_Read_Pattern <i32>;

} // End isEG Predicate

/* ------------------------------- */
/* Evergreen / Cayman Instructions */
/* ------------------------------- */

let Predicates = [isEGorCayman] in {
  
class TRIG_eg <InstR600 trig, Intrinsic intr> : Pat<
  (intr R600_Reg32:$src),
  (trig (MUL (MOV (LOADCONST_i32 CONST.TWO_PI_INV)), R600_Reg32:$src))
>;

let Gen = AMDGPUGen.EG_CAYMAN in {

  def MULADD_eg : MULADD_Common<0x14>;
  def ASHR_eg : ASHR_Common<0x15>;
  def LSHR_eg : LSHR_Common<0x16>;
  def LSHL_eg : LSHL_Common<0x17>;
  def CNDE_eg : CNDE_Common<0x19>;
  def CNDGT_eg : CNDGT_Common<0x1A>;
  def CNDGE_eg : CNDGE_Common<0x1B>;
  def MUL_LIT_eg : MUL_LIT_Common<0x1F>;
  def FLT_TO_INT_eg : FLT_TO_INT_Common<0x50>;
  def EXP_IEEE_eg : EXP_IEEE_Common<0x81>;
  def LOG_CLAMPED_eg : LOG_CLAMPED_Common<0x82>;
  def LOG_IEEE_eg : LOG_IEEE_Common<0x83>;
  def RECIP_CLAMPED_eg : RECIP_CLAMPED_Common<0x84>;
  def RECIP_IEEE_eg : RECIP_IEEE_Common<0x86>;
  def RECIPSQRT_CLAMPED_eg : RECIPSQRT_CLAMPED_Common<0x87>;
  def RECIPSQRT_IEEE_eg : RECIPSQRT_IEEE_Common<0x89>;
  def SIN_eg : SIN_Common<0x8D>;
  def COS_eg : COS_Common<0x8E>;
  def MULLO_INT_eg : MULLO_INT_Common<0x8F>;
  def MULHI_INT_eg : MULHI_INT_Common<0x90>;
  def MULLO_UINT_eg : MULLO_UINT_Common<0x91>;
  def MULHI_UINT_eg : MULHI_UINT_Common<0x92>;
  def RECIP_UINT_eg : RECIP_UINT_Common<0x94>;
  def INT_TO_FLT_eg : INT_TO_FLT_Common<0x9B>;
  def DOT4_eg : DOT4_Common<0xBE>;

} // End AMDGPUGen.EG_CAYMAN

  def DIV_eg : DIV_Common<RECIP_IEEE_eg>;
  def LRP_eg : LRP_Common<MULADD_eg>;
  def POW_eg : POW_Common<LOG_IEEE_eg, EXP_IEEE_eg, MUL, GPRF32>;
  def SSG_eg : SSG_Common<CNDGT_eg, CNDGE_eg>;
  def TGSI_LIT_Z_eg : TGSI_LIT_Z_Common<MUL_LIT_eg, LOG_CLAMPED_eg, EXP_IEEE_eg>;

  def : TRIG_eg <SIN_eg, int_AMDGPU_sin>;
  def : TRIG_eg <COS_eg, int_AMDGPU_cos>;

}

let Predicates = [isCayman] in {

let Gen = AMDGPUGen.CAYMAN in {

  /* XXX: I'm not sure if this opcode is correct. */
  def RECIP_UINT_cm : RECIP_UINT_Common<0x77>;

} // End AMDGPUGen.CAYMAN

} // End isCayman

/* Other Instructions */

let isCodeGenOnly = 1 in {
/*
  def SWIZZLE : AMDGPUShaderInst <
    (outs GPRV4F32:$dst),
    (ins GPRV4F32:$src0, i32imm:$src1),
    "SWIZZLE $dst, $src0, $src1",
    [(set GPRV4F32:$dst, (int_AMDGPU_swizzle GPRV4F32:$src0, imm:$src1))]
  >;
*/

  def LAST : AMDGPUShaderInst <
    (outs),
    (ins),
    "LAST",
    []
  >;

  def GET_CHAN : AMDGPUShaderInst <
    (outs R600_Reg32:$dst),
    (ins R600_Reg128:$src0, i32imm:$src1),
    "GET_CHAN $dst, $src0, $src1",
    []
  >;

  def MULLIT : AMDGPUShaderInst <
    (outs R600_Reg128:$dst),
    (ins R600_Reg32:$src0, R600_Reg32:$src1, R600_Reg32:$src2),
    "MULLIT $dst, $src0, $src1",
    [(set R600_Reg128:$dst, (int_AMDGPU_mullit R600_Reg32:$src0, R600_Reg32:$src1, R600_Reg32:$src2))]
  >;

let usesCustomInserter = 1, isPseudo = 1 in {

class R600PreloadInst <string asm, Intrinsic intr> : AMDGPUInst <
  (outs R600_TReg32:$dst),
  (ins),
  asm,
  [(set R600_TReg32:$dst, (intr))]
>;

def TGID_X : R600PreloadInst <"TGID_X", int_r600_read_tgid_x>;
def TGID_Y : R600PreloadInst <"TGID_Y", int_r600_read_tgid_y>;
def TGID_Z : R600PreloadInst <"TGID_Z", int_r600_read_tgid_z>;

def TIDIG_X : R600PreloadInst <"TIDIG_X", int_r600_read_tidig_x>;
def TIDIG_Y : R600PreloadInst <"TIDIG_Y", int_r600_read_tidig_y>;
def TIDIG_Z : R600PreloadInst <"TIDIG_Z", int_r600_read_tidig_z>;

def NGROUPS_X : R600PreloadInst <"NGROUPS_X", int_r600_read_ngroups_x>;
def NGROUPS_Y : R600PreloadInst <"NGROUPS_Y", int_r600_read_ngroups_y>;
def NGROUPS_Z : R600PreloadInst <"NGROUPS_Z", int_r600_read_ngroups_z>;

def GLOBAL_SIZE_X : R600PreloadInst <"GLOBAL_SIZE_X",
                                     int_r600_read_global_size_x>;
def GLOBAL_SIZE_Y : R600PreloadInst <"GLOBAL_SIZE_Y",
                                     int_r600_read_global_size_y>;
def GLOBAL_SIZE_Z : R600PreloadInst <"GLOBAL_SIZE_Z",
                                     int_r600_read_global_size_z>;

def LOCAL_SIZE_X : R600PreloadInst <"LOCAL_SIZE_X",
                                    int_r600_read_local_size_x>;
def LOCAL_SIZE_Y : R600PreloadInst <"LOCAL_SIZE_Y",
                                    int_r600_read_local_size_y>;
def LOCAL_SIZE_Z : R600PreloadInst <"LOCAL_SIZE_Z",
                                    int_r600_read_local_size_z>;

} // End usesCustomInserter = 1, isPseudo = 1

} // End isCodeGenOnly = 1



let isPseudo = 1 in {

def LOAD_VTX : AMDGPUShaderInst <
    (outs R600_Reg32:$dst),
    (ins MEMri:$mem),
    "LOAD_VTX",
    [(set (i32 R600_Reg32:$dst), (load_param ADDRParam:$mem))]
>;


} //End isPseudo

def : Extract_Element <f32, v4f32, R600_Reg128, 0, sel_x>;
def : Extract_Element <f32, v4f32, R600_Reg128, 1, sel_y>;
def : Extract_Element <f32, v4f32, R600_Reg128, 2, sel_z>;
def : Extract_Element <f32, v4f32, R600_Reg128, 3, sel_w>;

def : Insert_Element <f32, v4f32, R600_Reg32, R600_Reg128, 4, sel_x>;
def : Insert_Element <f32, v4f32, R600_Reg32, R600_Reg128, 5, sel_y>;
def : Insert_Element <f32, v4f32, R600_Reg32, R600_Reg128, 6, sel_z>;
def : Insert_Element <f32, v4f32, R600_Reg32, R600_Reg128, 7, sel_w>;

def : Extract_Element <i32, v4i32, R600_Reg128, 0, sel_x>;
def : Extract_Element <i32, v4i32, R600_Reg128, 1, sel_y>;
def : Extract_Element <i32, v4i32, R600_Reg128, 2, sel_z>;
def : Extract_Element <i32, v4i32, R600_Reg128, 3, sel_w>;

def : Insert_Element <i32, v4i32, R600_Reg32, R600_Reg128, 4, sel_x>;
def : Insert_Element <i32, v4i32, R600_Reg32, R600_Reg128, 5, sel_y>;
def : Insert_Element <i32, v4i32, R600_Reg32, R600_Reg128, 6, sel_z>;
def : Insert_Element <i32, v4i32, R600_Reg32, R600_Reg128, 7, sel_w>;


include "R600ShaderPatterns.td"

// We need this pattern to avoid having real registers in PHI nodes.
// For some reason this pattern only works when it comes after the other
// instruction defs.
def : Pat <
  (int_R600_load_input imm:$src),
  (LOAD_INPUT imm:$src)
>;

} // End isR600toCayman Predicate

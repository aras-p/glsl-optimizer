//===-- AMDGPUInstructions.td - TODO: Add brief description -------===//
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

include "AMDGPUInstrEnums.td"

class AMDGPUInst <dag outs, dag ins, string asm, list<dag> pattern> : Instruction {
  field bits<16> AMDILOp = 0;
  field bits<3> Gen = 0;
  field bit PreloadReg = 0;

  let Namespace = "AMDIL";
  let OutOperandList = outs;
  let InOperandList = ins;
  let AsmString = asm;
  let Pattern = pattern;
  let TSFlags{32} = PreloadReg;
  let TSFlags{42-40} = Gen;
  let TSFlags{63-48} = AMDILOp;
}

class AMDGPUShaderInst <dag outs, dag ins, string asm, list<dag> pattern>
    : AMDGPUInst<outs, ins, asm, pattern> {

  field bits<32> Inst = 0xffffffff;

}

let isCodeGenOnly = 1 in {

  def EXPORT_REG : AMDGPUShaderInst <
    (outs),
    (ins GPRF32:$src),
    "EXPORT_REG $src",
    [(int_AMDGPU_export_reg GPRF32:$src)]
  >;

  def LOAD_INPUT : AMDGPUShaderInst <
    (outs GPRF32:$dst),
    (ins i32imm:$src),
    "LOAD_INPUT $dst, $src",
    [] >{
    let PreloadReg = 1;
  }

  def MASK_WRITE : AMDGPUShaderInst <
    (outs),
    (ins GPRF32:$src),
    "MASK_WRITE $src",
    []
  >;

  def RESERVE_REG : AMDGPUShaderInst <
    (outs GPRF32:$dst),
    (ins i32imm:$src),
    "RESERVE_REG $dst, $src",
    [(set GPRF32:$dst, (int_AMDGPU_reserve_reg imm:$src))]> {
    let PreloadReg = 1;
  }

  def STORE_OUTPUT: AMDGPUShaderInst <
    (outs GPRF32:$dst),
    (ins GPRF32:$src0, i32imm:$src1),
    "STORE_OUTPUT $dst, $src0, $src1",
    [(set GPRF32:$dst, (int_AMDGPU_store_output GPRF32:$src0, imm:$src1))]
  >;
}

/* Generic helper patterns for intrinsics */
/* -------------------------------------- */

class POW_Common <AMDGPUInst log_ieee, AMDGPUInst exp_ieee, AMDGPUInst mul,
                  RegisterClass rc> : Pat <
  (int_AMDGPU_pow rc:$src0, rc:$src1),
  (exp_ieee (mul rc:$src1, (log_ieee rc:$src0)))
>;

include "R600Instructions.td"

include "SIInstrInfo.td"


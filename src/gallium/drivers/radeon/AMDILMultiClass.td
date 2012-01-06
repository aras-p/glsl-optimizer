//===-- AMDILMultiClass.td - TODO: Add brief description -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//
// Multiclass that handles branch instructions
multiclass BranchConditional<SDNode Op> {
    def _i8 : ILFormat<IL_OP_IFC, (outs),
  (ins brtarget:$target, GPRI8:$src0),
        "; i32 Pseudo branch instruction",
  [(Op bb:$target, GPRI8:$src0)]>;
    def _i16 : ILFormat<IL_OP_IFC, (outs),
  (ins brtarget:$target, GPRI16:$src0),
        "; i32 Pseudo branch instruction",
  [(Op bb:$target, GPRI16:$src0)]>;
    def _i32 : ILFormat<IL_OP_IFC, (outs),
  (ins brtarget:$target, GPRI32:$src0),
        "; i32 Pseudo branch instruction",
  [(Op bb:$target, GPRI32:$src0)]>;
    def _f32 : ILFormat<IL_OP_IFC, (outs),
  (ins brtarget:$target, GPRF32:$src0),
        "; f32 Pseudo branch instruction",
  [(Op bb:$target, GPRF32:$src0)]>;
    def _i64 : ILFormat<IL_OP_IFC, (outs),
  (ins brtarget:$target, GPRI64:$src0),
        "; f64 Pseudo branch instruction",
  [(Op bb:$target, (i64 GPRI64:$src0))]>;
    def _f64 : ILFormat<IL_OP_IFC, (outs),
  (ins brtarget:$target, GPRF64:$src0),
        "; f64 Pseudo branch instruction",
  [(Op bb:$target, (f64 GPRF64:$src0))]>;
}
// Multiclass that handles compare instructions
// When a definition is added here, a corrosponding defition
// needs to be added at:
// AMDILISelLowering.cpp@EmitInstrWithCustomInserter
multiclass Compare<string asm> {
  def _i8 : ILFormat<IL_OP_CMP, (outs GPRI8:$dst),
      (ins i32imm:$cc, GPRI8:$src0, GPRI8:$src1),
      !strconcat("; i8 ", asm),
      [(set GPRI8:$dst, (IL_cmp imm:$cc, GPRI8:$src0, GPRI8:$src1))]>;
  def _i16 : ILFormat<IL_OP_CMP, (outs GPRI16:$dst),
      (ins i32imm:$cc, GPRI16:$src0, GPRI16:$src1),
      !strconcat("; i16 ", asm),
      [(set GPRI16:$dst, (IL_cmp imm:$cc, GPRI16:$src0, GPRI16:$src1))]>;
  def _i32 : ILFormat<IL_OP_CMP, (outs GPRI32:$dst),
      (ins i32imm:$cc, GPRI32:$src0, GPRI32:$src1),
      !strconcat("; i32 ", asm),
      [(set GPRI32:$dst, (IL_cmp imm:$cc, GPRI32:$src0, GPRI32:$src1))]>;
  def _i64 : ILFormat<IL_OP_CMP, (outs GPRI64:$dst),
      (ins i32imm:$cc, GPRI64:$src0, GPRI64:$src1),
      !strconcat("; i64 ", asm),
      [(set GPRI64:$dst, (IL_cmp imm:$cc, GPRI64:$src0, GPRI64:$src1))]>;
  def _f32 : ILFormat<IL_OP_CMP, (outs GPRF32:$dst),
      (ins i32imm:$cc, GPRF32:$src0, GPRF32:$src1),
      !strconcat("; f32 ", asm),
      [(set GPRF32:$dst, (IL_cmp imm:$cc, GPRF32:$src0, GPRF32:$src1))]>;
  def _f64 : ILFormat<IL_OP_CMP, (outs GPRF64:$dst),
      (ins i32imm:$cc, GPRF64:$src0, GPRF64:$src1),
      !strconcat("; f64 ", asm),
      [(set GPRF64:$dst, (IL_cmp imm:$cc, GPRF64:$src0, GPRF64:$src1))]>;
  def _v2i8 : ILFormat<IL_OP_CMP, (outs GPRV2I8:$dst),
      (ins i32imm:$cc, GPRV2I8:$src0, GPRV2I8:$src1),
      !strconcat("; i8 ", asm),
      [(set GPRV2I8:$dst, (IL_cmp imm:$cc, GPRV2I8:$src0, GPRV2I8:$src1))]>;
  def _v2i16 : ILFormat<IL_OP_CMP, (outs GPRV2I16:$dst),
      (ins i32imm:$cc, GPRV2I16:$src0, GPRV2I16:$src1),
      !strconcat("; i16 ", asm),
      [(set GPRV2I16:$dst, (IL_cmp imm:$cc, GPRV2I16:$src0, GPRV2I16:$src1))]>;
  def _v2i32 : ILFormat<IL_OP_CMP, (outs GPRV2I32:$dst),
      (ins i32imm:$cc, GPRV2I32:$src0, GPRV2I32:$src1),
      !strconcat("; i32 ", asm),
      [(set GPRV2I32:$dst, (IL_cmp imm:$cc, GPRV2I32:$src0, GPRV2I32:$src1))]>;
  def _v2i64 : ILFormat<IL_OP_CMP, (outs GPRV2I64:$dst),
      (ins i32imm:$cc, GPRV2I64:$src0, GPRV2I64:$src1),
      !strconcat("; i64 ", asm),
      [(set GPRV2I64:$dst, (IL_cmp imm:$cc, GPRV2I64:$src0, GPRV2I64:$src1))]>;
  def _v2f32 : ILFormat<IL_OP_CMP, (outs GPRV2F32:$dst),
      (ins i32imm:$cc, GPRV2F32:$src0, GPRV2F32:$src1),
      !strconcat("; f32 ", asm),
      [(set GPRV2F32:$dst, (IL_cmp imm:$cc, GPRV2F32:$src0, GPRV2F32:$src1))]>;
  def _v2f64 : ILFormat<IL_OP_CMP, (outs GPRV2F64:$dst),
      (ins i32imm:$cc, GPRV2F64:$src0, GPRV2F64:$src1),
      !strconcat("; f64 ", asm),
      [(set GPRV2F64:$dst, (IL_cmp imm:$cc, GPRV2F64:$src0, GPRV2F64:$src1))]>;
  def _v4i8 : ILFormat<IL_OP_CMP, (outs GPRV4I8:$dst),
      (ins i32imm:$cc, GPRV4I8:$src0, GPRV4I8:$src1),
      !strconcat("; i8 ", asm),
      [(set GPRV4I8:$dst, (IL_cmp imm:$cc, GPRV4I8:$src0, GPRV4I8:$src1))]>;
  def _v4i16 : ILFormat<IL_OP_CMP, (outs GPRV4I16:$dst),
      (ins i32imm:$cc, GPRV4I16:$src0, GPRV4I16:$src1),
      !strconcat("; i16 ", asm),
      [(set GPRV4I16:$dst, (IL_cmp imm:$cc, GPRV4I16:$src0, GPRV4I16:$src1))]>;
  def _v4i32 : ILFormat<IL_OP_CMP, (outs GPRV4I32:$dst),
      (ins i32imm:$cc, GPRV4I32:$src0, GPRV4I32:$src1),
      !strconcat("; i32 ", asm),
      [(set GPRV4I32:$dst, (IL_cmp imm:$cc, GPRV4I32:$src0, GPRV4I32:$src1))]>;
  def _v4f32 : ILFormat<IL_OP_CMP, (outs GPRV4F32:$dst),
      (ins i32imm:$cc, GPRV4F32:$src0, GPRV4F32:$src1),
      !strconcat("; f32 ", asm),
      [(set GPRV4F32:$dst, (IL_cmp imm:$cc, GPRV4F32:$src0, GPRV4F32:$src1))]>;
}

// Multiclass that handles constant values
multiclass ILConstant<string asm> {
  def _i8 : ILFormat<IL_OP_MOV, (outs GPRI8:$dst),
      (ins i8imm:$val),
      asm, [(set GPRI8:$dst, imm:$val)]>;

  //  def _v2i8 : ILFormat<IL_OP_MOV, (outs GPRV2I8:$dst),
  //      (ins i8imm:$val),
  //      asm, [(set GPRV2I8:$dst, GPRV2I8:$val)]>;

  //def _v4i8 : ILFormat<IL_OP_MOV, (outs GPRV4I8:$dst),
  //(ins i8imm:$val),
  //asm, [(set GPRV4I8:$dst, GPRV4I8:$val)]>;

  def _i16 : ILFormat<IL_OP_MOV, (outs GPRI16:$dst),
      (ins i16imm:$val),
      asm, [(set GPRI16:$dst, imm:$val)]>;

  //  def _v2i16 : ILFormat<IL_OP_MOV, (outs GPRV2I16:$dst),
  //      (ins i16imm:$val),
  //      asm, [(set GPRV2I16:$dst, GPRV2I16:$val)]>;

  //  def _v4i16 : ILFormat<IL_OP_MOV, (outs GPRV4I16:$dst),
  //      (ins i16imm:$val),
  //      asm, [(set GPRV4I16:$dst, GPRV4I16:$val)]>;

  def _i32 : ILFormat<IL_OP_MOV, (outs GPRI32:$dst),
      (ins i32imm:$val),
      asm, [(set GPRI32:$dst, imm:$val)]>;

  //  def _v2i32 : ILFormat<IL_OP_MOV, (outs GPRV2I32:$dst),
  //      (ins i32imm:$val),
  //      asm, [(set GPRV2I32:$dst, GPRV2I32:$val)]>;

  //  def _v4i32 : ILFormat<IL_OP_MOV, (outs GPRV4I32:$dst),
  //      (ins GPRV4I32:$val),
  //      asm, [(set GPRV4I32:$dst, GPRV4I32:$val)]>;

  def _i64 : ILFormat<IL_OP_MOV, (outs GPRI64:$dst),
      (ins i64imm:$val),
      asm, [(set GPRI64:$dst, imm:$val)]>;

  //  def _v2i64 : ILFormat<IL_OP_MOV, (outs GPRV2I64:$dst),
  //      (ins i64imm:$val),
  //      asm, [(set GPRV2I64:$dst, GPRV2I64:$val)]>;

  def _f32 : ILFormat<IL_OP_MOV, (outs GPRF32:$dst),
      (ins f32imm:$val),
      asm, [(set GPRF32:$dst, fpimm:$val)]>;

  //  def _v2f32 : ILFormat<IL_OP_MOV, (outs GPRV2F32:$dst),
  //      (ins f32imm:$val),
  //      asm, [(set GPRV2F32:$dst, GPRV2F32:$val)]>;

  //  def _v4f32 : ILFormat<IL_OP_MOV, (outs GPRV4F32:$dst),
  //      (ins f32imm:$val),
  //      asm, [(set GPRV4F32:$dst, GPRV4F32:$val)]>;

  def _f64 : ILFormat<IL_OP_MOV, (outs GPRF64:$dst),
      (ins f64imm:$val),
      asm, [(set GPRF64:$dst, fpimm:$val)]>;

  //  def _v2f64 : ILFormat<IL_OP_MOV, (outs GPRV2F64:$dst),
  //      (ins f64imm:$val),
  //        asm, [(set GPRV2F64:$dst, GPRV2F64:$val)]>;

}

// Multiclass that handles memory store operations
multiclass GTRUNCSTORE<string asm> {
  def _i16i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI16:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(global_i8trunc_store GPRI16:$val, ADDR:$ptr)]>;
  def _i32i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI32:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(global_i8trunc_store GPRI32:$val, ADDR:$ptr)]>;
  def _i64i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI64:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(global_i8trunc_store GPRI64:$val, ADDR:$ptr)]>;
  def _i32i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI32:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(global_i16trunc_store GPRI32:$val, ADDR:$ptr)]>;
  def _i64i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI64:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(global_i16trunc_store GPRI64:$val, ADDR:$ptr)]>;
  def _i64i32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI64:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(global_i32trunc_store GPRI64:$val, ADDR:$ptr)]>;
  def _f64f32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRF64:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(global_f32trunc_store GPRF64:$val, ADDR:$ptr)]>;
  def _v2i32i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I32:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(global_v2i8trunc_store GPRV2I32:$val, ADDR:$ptr)]>;
  def _v4i32i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV4I32:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(global_v4i8trunc_store GPRV4I32:$val, ADDR:$ptr)]>;
  def _v2i16i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I16:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(global_v2i8trunc_store GPRV2I16:$val, ADDR:$ptr)]>;
  def _v4i16i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV4I16:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(global_v4i8trunc_store GPRV4I16:$val, ADDR:$ptr)]>;
  def _v2i32i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I32:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(global_v2i16trunc_store GPRV2I32:$val, ADDR:$ptr)]>;
  def _v4i32i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV4I32:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(global_v4i16trunc_store GPRV4I32:$val, ADDR:$ptr)]>;
  def _v2f64f32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2F64:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(global_v2f32trunc_store GPRV2F64:$val, ADDR:$ptr)]>;
  def _v2i64i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I64:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(global_v2i8trunc_store GPRV2I64:$val, ADDR:$ptr)]>;
  def _v2i64i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I64:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(global_v2i16trunc_store GPRV2I64:$val, ADDR:$ptr)]>;
  def _v2i64i32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I64:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(global_v2i32trunc_store GPRV2I64:$val, ADDR:$ptr)]>;
}

// Multiclass that handles memory store operations
multiclass LTRUNCSTORE<string asm> {
  def _i16i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI16:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(local_i8trunc_store GPRI16:$val, ADDR:$ptr)]>;
  def _i32i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI32:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(local_i8trunc_store GPRI32:$val, ADDR:$ptr)]>;
  def _i64i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI64:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(local_i8trunc_store GPRI64:$val, ADDR:$ptr)]>;
  def _i32i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI32:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(local_i16trunc_store GPRI32:$val, ADDR:$ptr)]>;
  def _i64i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI64:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(local_i16trunc_store GPRI64:$val, ADDR:$ptr)]>;
  def _i64i32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI64:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(local_i32trunc_store GPRI64:$val, ADDR:$ptr)]>;
  def _f64f32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRF64:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(local_f32trunc_store GPRF64:$val, ADDR:$ptr)]>;
  def _v2i32i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I32:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(local_v2i8trunc_store GPRV2I32:$val, ADDR:$ptr)]>;
  def _v4i32i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV4I32:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(local_v4i8trunc_store GPRV4I32:$val, ADDR:$ptr)]>;
  def _v2i16i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I16:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(local_v2i8trunc_store GPRV2I16:$val, ADDR:$ptr)]>;
  def _v4i16i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV4I16:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(local_v4i8trunc_store GPRV4I16:$val, ADDR:$ptr)]>;
  def _v2i32i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I32:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(local_v2i16trunc_store GPRV2I32:$val, ADDR:$ptr)]>;
  def _v4i32i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV4I32:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(local_v4i16trunc_store GPRV4I32:$val, ADDR:$ptr)]>;
  def _v2f64f32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2F64:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(local_v2f32trunc_store GPRV2F64:$val, ADDR:$ptr)]>;
  def _v2i64i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I64:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(local_v2i8trunc_store GPRV2I64:$val, ADDR:$ptr)]>;
  def _v2i64i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I64:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(local_v2i16trunc_store GPRV2I64:$val, ADDR:$ptr)]>;
  def _v2i64i32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I64:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(local_v2i32trunc_store GPRV2I64:$val, ADDR:$ptr)]>;
}

// Multiclass that handles memory store operations
multiclass PTRUNCSTORE<string asm> {
  def _i16i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI16:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(private_i8trunc_store GPRI16:$val, ADDR:$ptr)]>;
  def _i32i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI32:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(private_i8trunc_store GPRI32:$val, ADDR:$ptr)]>;
  def _i64i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI64:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(private_i8trunc_store GPRI64:$val, ADDR:$ptr)]>;
  def _i32i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI32:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(private_i16trunc_store GPRI32:$val, ADDR:$ptr)]>;
  def _i64i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI64:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(private_i16trunc_store GPRI64:$val, ADDR:$ptr)]>;
  def _i64i32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI64:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(private_i32trunc_store GPRI64:$val, ADDR:$ptr)]>;
  def _f64f32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRF64:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(private_f32trunc_store GPRF64:$val, ADDR:$ptr)]>;
  def _v2i32i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I32:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(private_v2i8trunc_store GPRV2I32:$val, ADDR:$ptr)]>;
  def _v4i32i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV4I32:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(private_v4i8trunc_store GPRV4I32:$val, ADDR:$ptr)]>;
  def _v2i16i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I16:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(private_v2i8trunc_store GPRV2I16:$val, ADDR:$ptr)]>;
  def _v4i16i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV4I16:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(private_v4i8trunc_store GPRV4I16:$val, ADDR:$ptr)]>;
  def _v2i32i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I32:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(private_v2i16trunc_store GPRV2I32:$val, ADDR:$ptr)]>;
  def _v4i32i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV4I32:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(private_v4i16trunc_store GPRV4I32:$val, ADDR:$ptr)]>;
  def _v2f64f32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2F64:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(private_v2f32trunc_store GPRV2F64:$val, ADDR:$ptr)]>;
  def _v2i64i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I64:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(private_v2i8trunc_store GPRV2I64:$val, ADDR:$ptr)]>;
  def _v2i64i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I64:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(private_v2i16trunc_store GPRV2I64:$val, ADDR:$ptr)]>;
  def _v2i64i32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I64:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(private_v2i32trunc_store GPRV2I64:$val, ADDR:$ptr)]>;
}

// Multiclass that handles memory store operations
multiclass RTRUNCSTORE<string asm> {
  def _i16i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI16:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(region_i8trunc_store GPRI16:$val, ADDR:$ptr)]>;
  def _i32i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI32:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(region_i8trunc_store GPRI32:$val, ADDR:$ptr)]>;
  def _i64i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI64:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(region_i8trunc_store GPRI64:$val, ADDR:$ptr)]>;
  def _i32i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI32:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(region_i16trunc_store GPRI32:$val, ADDR:$ptr)]>;
  def _i64i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI64:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(region_i16trunc_store GPRI64:$val, ADDR:$ptr)]>;
  def _i64i32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI64:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(region_i32trunc_store GPRI64:$val, ADDR:$ptr)]>;
  def _f64f32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRF64:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(region_f32trunc_store GPRF64:$val, ADDR:$ptr)]>;
  def _v2i32i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I32:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(region_v2i8trunc_store GPRV2I32:$val, ADDR:$ptr)]>;
  def _v4i32i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV4I32:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(region_v4i8trunc_store GPRV4I32:$val, ADDR:$ptr)]>;
  def _v2i16i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I16:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(region_v2i8trunc_store GPRV2I16:$val, ADDR:$ptr)]>;
  def _v4i16i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV4I16:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(region_v4i8trunc_store GPRV4I16:$val, ADDR:$ptr)]>;
  def _v2i32i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I32:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(region_v2i16trunc_store GPRV2I32:$val, ADDR:$ptr)]>;
  def _v4i32i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV4I32:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(region_v4i16trunc_store GPRV4I32:$val, ADDR:$ptr)]>;
  def _v2f64f32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2F64:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(region_v2f32trunc_store GPRV2F64:$val, ADDR:$ptr)]>;
  def _v2i64i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I64:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(region_v2i8trunc_store GPRV2I64:$val, ADDR:$ptr)]>;
  def _v2i64i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I64:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(region_v2i16trunc_store GPRV2I64:$val, ADDR:$ptr)]>;
  def _v2i64i32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I64:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(region_v2i32trunc_store GPRV2I64:$val, ADDR:$ptr)]>;
}


// Multiclass that handles memory store operations
multiclass STORE<string asm, PatFrag OpNode> {
  def _i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI8:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(OpNode GPRI8:$val, ADDR:$ptr)]>;
  def _i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI16:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(OpNode GPRI16:$val, ADDR:$ptr)]>;
  def _i32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI32:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(OpNode GPRI32:$val, ADDR:$ptr)]>;
  def _f32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRF32:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(OpNode GPRF32:$val, ADDR:$ptr)]>;
  def _i64 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI64:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(OpNode GPRI64:$val, ADDR:$ptr)]>;
  def _f64 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRF64:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(OpNode GPRF64:$val, ADDR:$ptr)]>;
  def _v4f32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV4F32:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(OpNode GPRV4F32:$val, ADDR:$ptr)]>;
  def _v2f32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2F32:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(OpNode GPRV2F32:$val, ADDR:$ptr)]>;
  def _v4i32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV4I32:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(OpNode GPRV4I32:$val, ADDR:$ptr)]>;
  def _v2i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I8:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(OpNode GPRV2I8:$val, ADDR:$ptr)]>;
  def _v2i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I16:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(OpNode GPRV2I16:$val, ADDR:$ptr)]>;
  def _v4i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV4I8:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(OpNode GPRV4I8:$val, ADDR:$ptr)]>;
  def _v4i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV4I16:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(OpNode GPRV4I16:$val, ADDR:$ptr)]>;
  def _v2i32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I32:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(OpNode GPRV2I32:$val, ADDR:$ptr)]>;
  def _v2f64 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2F64:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(OpNode GPRV2F64:$val, ADDR:$ptr)]>;
  def _v2i64 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I64:$val, MEMI32:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(OpNode GPRV2I64:$val, ADDR:$ptr)]>;
}

// Multiclass that handles load operations
multiclass LOAD<string asm, PatFrag OpNode> {
  def _i8 : OneInOneOut<IL_OP_MOV, (outs GPRI8:$dst), (ins MEMI32:$ptr),
      !strconcat(asm, " $dst $ptr"),
      [(set GPRI8:$dst, (OpNode ADDR:$ptr))]>;
  def _i16 : OneInOneOut<IL_OP_MOV, (outs GPRI16:$dst), (ins MEMI32:$ptr),
      !strconcat(asm, " $dst $ptr"),
      [(set GPRI16:$dst, (OpNode ADDR:$ptr))]>;
  def _i32 : OneInOneOut<IL_OP_MOV, (outs GPRI32:$dst), (ins MEMI32:$ptr),
      !strconcat(asm, " $dst $ptr"),
      [(set GPRI32:$dst, (OpNode ADDR:$ptr))]>;
  def _f32 : OneInOneOut<IL_OP_MOV, (outs GPRF32:$dst), (ins MEMI32:$ptr),
      !strconcat(asm, " $dst $ptr"),
      [(set GPRF32:$dst, (OpNode ADDR:$ptr))]>;
  def _i64 : OneInOneOut<IL_OP_MOV, (outs GPRI64:$dst), (ins MEMI32:$ptr),
      !strconcat(asm, " $dst $ptr"),
      [(set GPRI64:$dst, (OpNode ADDR:$ptr))]>;
  def _f64 : OneInOneOut<IL_OP_MOV, (outs GPRF64:$dst), (ins MEMI32:$ptr),
      !strconcat(asm, " $dst $ptr"),
      [(set GPRF64:$dst, (OpNode ADDR:$ptr))]>;
  def _v4f32 : OneInOneOut<IL_OP_MOV, (outs GPRV4F32:$dst), (ins MEMI32:$ptr),
      !strconcat(asm, " $dst $ptr"),
      [(set GPRV4F32:$dst, (OpNode ADDR:$ptr))]>;
  def _v2f32 : OneInOneOut<IL_OP_MOV, (outs GPRV2F32:$dst), (ins MEMI32:$ptr),
      !strconcat(asm, " $dst $ptr"),
      [(set GPRV2F32:$dst, (OpNode ADDR:$ptr))]>;
  def _v2f64 : OneInOneOut<IL_OP_MOV, (outs GPRV2F64:$dst), (ins MEMI32:$ptr),
      !strconcat(asm, " $dst $ptr"),
      [(set GPRV2F64:$dst, (OpNode ADDR:$ptr))]>;
  def _v4i32 : OneInOneOut<IL_OP_MOV, (outs GPRV4I32:$dst), (ins MEMI32:$ptr),
      !strconcat(asm, " $dst $ptr"),
      [(set GPRV4I32:$dst, (OpNode ADDR:$ptr))]>;
  def _v2i8 : OneInOneOut<IL_OP_MOV, (outs GPRV2I8:$dst), (ins MEMI32:$ptr),
      !strconcat(asm, " $dst $ptr"),
      [(set GPRV2I8:$dst, (OpNode ADDR:$ptr))]>;
  def _v2i16 : OneInOneOut<IL_OP_MOV, (outs GPRV2I16:$dst), (ins MEMI32:$ptr),
      !strconcat(asm, " $dst $ptr"),
      [(set GPRV2I16:$dst, (OpNode ADDR:$ptr))]>;
  def _v4i8 : OneInOneOut<IL_OP_MOV, (outs GPRV4I8:$dst), (ins MEMI32:$ptr),
      !strconcat(asm, " $dst $ptr"),
      [(set GPRV4I8:$dst, (OpNode ADDR:$ptr))]>;
  def _v4i16 : OneInOneOut<IL_OP_MOV, (outs GPRV4I16:$dst), (ins MEMI32:$ptr),
      !strconcat(asm, " $dst $ptr"),
      [(set GPRV4I16:$dst, (OpNode ADDR:$ptr))]>;
  def _v2i32 : OneInOneOut<IL_OP_MOV, (outs GPRV2I32:$dst), (ins MEMI32:$ptr),
      !strconcat(asm, " $dst $ptr"),
      [(set GPRV2I32:$dst, (OpNode ADDR:$ptr))]>;
  def _v2i64 : OneInOneOut<IL_OP_MOV, (outs GPRV2I64:$dst), (ins MEMI32:$ptr),
      !strconcat(asm, " $dst $ptr"),
      [(set GPRV2I64:$dst, (OpNode ADDR:$ptr))]>;
}

// Multiclass that handles memory store operations
multiclass GTRUNCSTORE64<string asm> {
  def _i16i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI16:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(global_i8trunc_store GPRI16:$val, ADDR64:$ptr)]>;
  def _i32i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI32:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(global_i8trunc_store GPRI32:$val, ADDR64:$ptr)]>;
  def _i64i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI64:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(global_i8trunc_store GPRI64:$val, ADDR64:$ptr)]>;
  def _i32i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI32:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(global_i16trunc_store GPRI32:$val, ADDR64:$ptr)]>;
  def _i64i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI64:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(global_i16trunc_store GPRI64:$val, ADDR64:$ptr)]>;
  def _i64i32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI64:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(global_i32trunc_store GPRI64:$val, ADDR64:$ptr)]>;
  def _f64f32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRF64:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(global_f32trunc_store GPRF64:$val, ADDR64:$ptr)]>;
  def _v2i32i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I32:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(global_v2i8trunc_store GPRV2I32:$val, ADDR64:$ptr)]>;
  def _v4i32i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV4I32:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(global_v4i8trunc_store GPRV4I32:$val, ADDR64:$ptr)]>;
  def _v2i16i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I16:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(global_v2i8trunc_store GPRV2I16:$val, ADDR64:$ptr)]>;
  def _v4i16i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV4I16:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(global_v4i8trunc_store GPRV4I16:$val, ADDR64:$ptr)]>;
  def _v2i32i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I32:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(global_v2i16trunc_store GPRV2I32:$val, ADDR64:$ptr)]>;
  def _v4i32i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV4I32:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(global_v4i16trunc_store GPRV4I32:$val, ADDR64:$ptr)]>;
  def _v2f64f32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2F64:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(global_v2f32trunc_store GPRV2F64:$val, ADDR64:$ptr)]>;
  def _v2i64i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I64:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(global_v2i8trunc_store GPRV2I64:$val, ADDR64:$ptr)]>;
  def _v2i64i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I64:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(global_v2i16trunc_store GPRV2I64:$val, ADDR64:$ptr)]>;
  def _v2i64i32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I64:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(global_v2i32trunc_store GPRV2I64:$val, ADDR64:$ptr)]>;
}

// Multiclass that handles memory store operations
multiclass LTRUNCSTORE64<string asm> {
  def _i16i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI16:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(local_i8trunc_store GPRI16:$val, ADDR64:$ptr)]>;
  def _i32i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI32:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(local_i8trunc_store GPRI32:$val, ADDR64:$ptr)]>;
  def _i64i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI64:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(local_i8trunc_store GPRI64:$val, ADDR64:$ptr)]>;
  def _i32i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI32:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(local_i16trunc_store GPRI32:$val, ADDR64:$ptr)]>;
  def _i64i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI64:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(local_i16trunc_store GPRI64:$val, ADDR64:$ptr)]>;
  def _i64i32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI64:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(local_i32trunc_store GPRI64:$val, ADDR64:$ptr)]>;
  def _f64f32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRF64:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(local_f32trunc_store GPRF64:$val, ADDR64:$ptr)]>;
  def _v2i32i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I32:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(local_v2i8trunc_store GPRV2I32:$val, ADDR64:$ptr)]>;
  def _v4i32i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV4I32:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(local_v4i8trunc_store GPRV4I32:$val, ADDR64:$ptr)]>;
  def _v2i16i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I16:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(local_v2i8trunc_store GPRV2I16:$val, ADDR64:$ptr)]>;
  def _v4i16i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV4I16:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(local_v4i8trunc_store GPRV4I16:$val, ADDR64:$ptr)]>;
  def _v2i32i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I32:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(local_v2i16trunc_store GPRV2I32:$val, ADDR64:$ptr)]>;
  def _v4i32i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV4I32:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(local_v4i16trunc_store GPRV4I32:$val, ADDR64:$ptr)]>;
  def _v2f64f32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2F64:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(local_v2f32trunc_store GPRV2F64:$val, ADDR64:$ptr)]>;
  def _v2i64i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I64:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(local_v2i8trunc_store GPRV2I64:$val, ADDR64:$ptr)]>;
  def _v2i64i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I64:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(local_v2i16trunc_store GPRV2I64:$val, ADDR64:$ptr)]>;
  def _v2i64i32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I64:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(local_v2i32trunc_store GPRV2I64:$val, ADDR64:$ptr)]>;
}

// Multiclass that handles memory store operations
multiclass PTRUNCSTORE64<string asm> {
  def _i16i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI16:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(private_i8trunc_store GPRI16:$val, ADDR64:$ptr)]>;
  def _i32i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI32:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(private_i8trunc_store GPRI32:$val, ADDR64:$ptr)]>;
  def _i64i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI64:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(private_i8trunc_store GPRI64:$val, ADDR64:$ptr)]>;
  def _i32i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI32:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(private_i16trunc_store GPRI32:$val, ADDR64:$ptr)]>;
  def _i64i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI64:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(private_i16trunc_store GPRI64:$val, ADDR64:$ptr)]>;
  def _i64i32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI64:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(private_i32trunc_store GPRI64:$val, ADDR64:$ptr)]>;
  def _f64f32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRF64:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(private_f32trunc_store GPRF64:$val, ADDR64:$ptr)]>;
  def _v2i32i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I32:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(private_v2i8trunc_store GPRV2I32:$val, ADDR64:$ptr)]>;
  def _v4i32i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV4I32:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(private_v4i8trunc_store GPRV4I32:$val, ADDR64:$ptr)]>;
  def _v2i16i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I16:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(private_v2i8trunc_store GPRV2I16:$val, ADDR64:$ptr)]>;
  def _v4i16i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV4I16:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(private_v4i8trunc_store GPRV4I16:$val, ADDR64:$ptr)]>;
  def _v2i32i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I32:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(private_v2i16trunc_store GPRV2I32:$val, ADDR64:$ptr)]>;
  def _v4i32i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV4I32:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(private_v4i16trunc_store GPRV4I32:$val, ADDR64:$ptr)]>;
  def _v2f64f32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2F64:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(private_v2f32trunc_store GPRV2F64:$val, ADDR64:$ptr)]>;
  def _v2i64i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I64:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(private_v2i8trunc_store GPRV2I64:$val, ADDR64:$ptr)]>;
  def _v2i64i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I64:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(private_v2i16trunc_store GPRV2I64:$val, ADDR64:$ptr)]>;
  def _v2i64i32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I64:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(private_v2i32trunc_store GPRV2I64:$val, ADDR64:$ptr)]>;
}

// Multiclass that handles memory store operations
multiclass RTRUNCSTORE64<string asm> {
  def _i16i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI16:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(region_i8trunc_store GPRI16:$val, ADDR64:$ptr)]>;
  def _i32i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI32:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(region_i8trunc_store GPRI32:$val, ADDR64:$ptr)]>;
  def _i64i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI64:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(region_i8trunc_store GPRI64:$val, ADDR64:$ptr)]>;
  def _i32i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI32:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(region_i16trunc_store GPRI32:$val, ADDR64:$ptr)]>;
  def _i64i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI64:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(region_i16trunc_store GPRI64:$val, ADDR64:$ptr)]>;
  def _i64i32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI64:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(region_i32trunc_store GPRI64:$val, ADDR64:$ptr)]>;
  def _f64f32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRF64:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(region_f32trunc_store GPRF64:$val, ADDR64:$ptr)]>;
  def _v2i32i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I32:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(region_v2i8trunc_store GPRV2I32:$val, ADDR64:$ptr)]>;
  def _v4i32i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV4I32:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(region_v4i8trunc_store GPRV4I32:$val, ADDR64:$ptr)]>;
  def _v2i16i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I16:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(region_v2i8trunc_store GPRV2I16:$val, ADDR64:$ptr)]>;
  def _v4i16i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV4I16:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(region_v4i8trunc_store GPRV4I16:$val, ADDR64:$ptr)]>;
  def _v2i32i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I32:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(region_v2i16trunc_store GPRV2I32:$val, ADDR64:$ptr)]>;
  def _v4i32i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV4I32:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(region_v4i16trunc_store GPRV4I32:$val, ADDR64:$ptr)]>;
  def _v2f64f32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2F64:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(region_v2f32trunc_store GPRV2F64:$val, ADDR64:$ptr)]>;
  def _v2i64i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I64:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(region_v2i8trunc_store GPRV2I64:$val, ADDR64:$ptr)]>;
  def _v2i64i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I64:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(region_v2i16trunc_store GPRV2I64:$val, ADDR64:$ptr)]>;
  def _v2i64i32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I64:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(region_v2i32trunc_store GPRV2I64:$val, ADDR64:$ptr)]>;
}


// Multiclass that handles memory store operations
multiclass STORE64<string asm, PatFrag OpNode> {
  def _i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI8:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(OpNode GPRI8:$val, ADDR64:$ptr)]>;
  def _i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI16:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(OpNode GPRI16:$val, ADDR64:$ptr)]>;
  def _i32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI32:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(OpNode GPRI32:$val, ADDR64:$ptr)]>;
  def _f32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRF32:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(OpNode GPRF32:$val, ADDR64:$ptr)]>;
  def _i64 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRI64:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(OpNode GPRI64:$val, ADDR64:$ptr)]>;
  def _f64 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRF64:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(OpNode GPRF64:$val, ADDR64:$ptr)]>;
  def _v4f32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV4F32:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(OpNode GPRV4F32:$val, ADDR64:$ptr)]>;
  def _v2f32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2F32:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(OpNode GPRV2F32:$val, ADDR64:$ptr)]>;
  def _v4i32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV4I32:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(OpNode GPRV4I32:$val, ADDR64:$ptr)]>;
  def _v2i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I8:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(OpNode GPRV2I8:$val, ADDR64:$ptr)]>;
  def _v2i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I16:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(OpNode GPRV2I16:$val, ADDR64:$ptr)]>;
  def _v4i8 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV4I8:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(OpNode GPRV4I8:$val, ADDR64:$ptr)]>;
  def _v4i16 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV4I16:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(OpNode GPRV4I16:$val, ADDR64:$ptr)]>;
  def _v2i32 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I32:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(OpNode GPRV2I32:$val, ADDR64:$ptr)]>;
  def _v2f64 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2F64:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(OpNode GPRV2F64:$val, ADDR64:$ptr)]>;
  def _v2i64 : OneInOneOut<IL_OP_MOV, (outs), (ins GPRV2I64:$val, MEMI64:$ptr),
      !strconcat(asm, " $val $ptr"),
      [(OpNode GPRV2I64:$val, ADDR64:$ptr)]>;
}

// Multiclass that handles load operations
multiclass LOAD64<string asm, PatFrag OpNode> {
  def _i8 : OneInOneOut<IL_OP_MOV, (outs GPRI8:$dst), (ins MEMI64:$ptr),
      !strconcat(asm, " $dst $ptr"),
      [(set GPRI8:$dst, (OpNode ADDR64:$ptr))]>;
  def _i16 : OneInOneOut<IL_OP_MOV, (outs GPRI16:$dst), (ins MEMI64:$ptr),
      !strconcat(asm, " $dst $ptr"),
      [(set GPRI16:$dst, (OpNode ADDR64:$ptr))]>;
  def _i32 : OneInOneOut<IL_OP_MOV, (outs GPRI32:$dst), (ins MEMI64:$ptr),
      !strconcat(asm, " $dst $ptr"),
      [(set GPRI32:$dst, (OpNode ADDR64:$ptr))]>;
  def _f32 : OneInOneOut<IL_OP_MOV, (outs GPRF32:$dst), (ins MEMI64:$ptr),
      !strconcat(asm, " $dst $ptr"),
      [(set GPRF32:$dst, (OpNode ADDR64:$ptr))]>;
  def _i64 : OneInOneOut<IL_OP_MOV, (outs GPRI64:$dst), (ins MEMI64:$ptr),
      !strconcat(asm, " $dst $ptr"),
      [(set GPRI64:$dst, (OpNode ADDR64:$ptr))]>;
  def _f64 : OneInOneOut<IL_OP_MOV, (outs GPRF64:$dst), (ins MEMI64:$ptr),
      !strconcat(asm, " $dst $ptr"),
      [(set GPRF64:$dst, (OpNode ADDR64:$ptr))]>;
  def _v4f32 : OneInOneOut<IL_OP_MOV, (outs GPRV4F32:$dst), (ins MEMI64:$ptr),
      !strconcat(asm, " $dst $ptr"),
      [(set GPRV4F32:$dst, (OpNode ADDR64:$ptr))]>;
  def _v2f32 : OneInOneOut<IL_OP_MOV, (outs GPRV2F32:$dst), (ins MEMI64:$ptr),
      !strconcat(asm, " $dst $ptr"),
      [(set GPRV2F32:$dst, (OpNode ADDR64:$ptr))]>;
  def _v2f64 : OneInOneOut<IL_OP_MOV, (outs GPRV2F64:$dst), (ins MEMI64:$ptr),
      !strconcat(asm, " $dst $ptr"),
      [(set GPRV2F64:$dst, (OpNode ADDR64:$ptr))]>;
  def _v4i32 : OneInOneOut<IL_OP_MOV, (outs GPRV4I32:$dst), (ins MEMI64:$ptr),
      !strconcat(asm, " $dst $ptr"),
      [(set GPRV4I32:$dst, (OpNode ADDR64:$ptr))]>;
  def _v2i8 : OneInOneOut<IL_OP_MOV, (outs GPRV2I8:$dst), (ins MEMI64:$ptr),
      !strconcat(asm, " $dst $ptr"),
      [(set GPRV2I8:$dst, (OpNode ADDR64:$ptr))]>;
  def _v2i16 : OneInOneOut<IL_OP_MOV, (outs GPRV2I16:$dst), (ins MEMI64:$ptr),
      !strconcat(asm, " $dst $ptr"),
      [(set GPRV2I16:$dst, (OpNode ADDR64:$ptr))]>;
  def _v4i8 : OneInOneOut<IL_OP_MOV, (outs GPRV4I8:$dst), (ins MEMI64:$ptr),
      !strconcat(asm, " $dst $ptr"),
      [(set GPRV4I8:$dst, (OpNode ADDR64:$ptr))]>;
  def _v4i16 : OneInOneOut<IL_OP_MOV, (outs GPRV4I16:$dst), (ins MEMI64:$ptr),
      !strconcat(asm, " $dst $ptr"),
      [(set GPRV4I16:$dst, (OpNode ADDR64:$ptr))]>;
  def _v2i32 : OneInOneOut<IL_OP_MOV, (outs GPRV2I32:$dst), (ins MEMI64:$ptr),
      !strconcat(asm, " $dst $ptr"),
      [(set GPRV2I32:$dst, (OpNode ADDR64:$ptr))]>;
  def _v2i64 : OneInOneOut<IL_OP_MOV, (outs GPRV2I64:$dst), (ins MEMI64:$ptr),
      !strconcat(asm, " $dst $ptr"),
      [(set GPRV2I64:$dst, (OpNode ADDR64:$ptr))]>;
}

// Only scalar types should generate flow control
multiclass BranchInstr<ILOpCode opc> {
  def _i8 : UnaryOpNoRet<opc, (outs), (ins GPRI8:$src),
      !strconcat(opc.Text, " $src"), []>;
  def _i16 : UnaryOpNoRet<opc, (outs), (ins GPRI16:$src),
      !strconcat(opc.Text, " $src"), []>;
  def _i32 : UnaryOpNoRet<opc, (outs), (ins GPRI32:$src),
      !strconcat(opc.Text, " $src"), []>;
  def _i64 : UnaryOpNoRet<opc, (outs), (ins GPRI64:$src),
      !strconcat(opc.Text, " $src"), []>;
  def _f32 : UnaryOpNoRet<opc, (outs), (ins GPRF32:$src),
      !strconcat(opc.Text, " $src"), []>;
  def _f64 : UnaryOpNoRet<opc, (outs), (ins GPRF64:$src),
      !strconcat(opc.Text, " $src"), []>;
}
// Only scalar types should generate flow control
multiclass BranchInstr2<ILOpCode opc> {
  def _i8 : BinaryOpNoRet<opc, (outs),  (ins  GPRI8:$src0,  GPRI8:$src1),
      !strconcat(opc.Text, " $src0, $src1"), []>;
  def _i16 : BinaryOpNoRet<opc, (outs), (ins GPRI16:$src0, GPRI16:$src1),
      !strconcat(opc.Text, " $src0, $src1"), []>;
  def _i32 : BinaryOpNoRet<opc, (outs), (ins GPRI32:$src0, GPRI32:$src1),
      !strconcat(opc.Text, " $src0, $src1"), []>;
  def _i64 : BinaryOpNoRet<opc, (outs), (ins GPRI64:$src0, GPRI64:$src1),
      !strconcat(opc.Text, " $src0, $src1"), []>;
  def _f32 : BinaryOpNoRet<opc, (outs), (ins GPRF32:$src0, GPRF32:$src1),
      !strconcat(opc.Text, " $src0, $src1"), []>;
  def _f64 : BinaryOpNoRet<opc, (outs), (ins GPRF64:$src0, GPRF64:$src1),
      !strconcat(opc.Text, " $src0, $src1"), []>;
}

// Class that handles the various vector extract patterns
multiclass VectorExtract<SDNode OpNode> {
  def _v2f64 : ExtractVectorClass<GPRF64, GPRV2F64, OpNode>;
  def _v4f32: ExtractVectorClass<GPRF32, GPRV4F32, OpNode>;
  def _v2f32 : ExtractVectorClass<GPRF32, GPRV2F32, OpNode>;
  def _v2i64 : ExtractVectorClass<GPRI64, GPRV2I64, OpNode>;
  def _v4i8 : ExtractVectorClass<GPRI8, GPRV4I8, OpNode>;
  def _v4i16 : ExtractVectorClass<GPRI16, GPRV4I16, OpNode>;
  def _v4i32 : ExtractVectorClass<GPRI32, GPRV4I32, OpNode>;
  def _v2i8 : ExtractVectorClass<GPRI8, GPRV2I8, OpNode>;
  def _v2i16 : ExtractVectorClass<GPRI16, GPRV2I16, OpNode>;
  def _v2i32 : ExtractVectorClass<GPRI32, GPRV2I32, OpNode>;
}

multiclass VectorConcat<SDNode OpNode> {
  def _v2f64 : VectorConcatClass<GPRV2F64, GPRF64, OpNode>;
  def _v2i64 : VectorConcatClass<GPRV2F64, GPRI64, OpNode>;
  def _v4f32 : VectorConcatClass<GPRV4F32, GPRV2F32, OpNode>;
  def _v4i32 : VectorConcatClass<GPRV4I32, GPRV2I32, OpNode>;
  def _v4i16 : VectorConcatClass<GPRV4I16, GPRV2I16, OpNode>;
  def _v4i8 : VectorConcatClass<GPRV4I8, GPRV2I8, OpNode>;
  def _v2f32 : VectorConcatClass<GPRV2F32, GPRF32, OpNode>;
  def _v2i32 : VectorConcatClass<GPRV2I32, GPRI32, OpNode>;
  def _v2i16 : VectorConcatClass<GPRV2I16, GPRI16, OpNode>;
  def _v2i8 : VectorConcatClass<GPRV2I8, GPRI8, OpNode>;
}

// Class that handles the various vector insert patterns
multiclass VectorInsert<SDNode OpNode> {
  def _v2f64 : InsertVectorClass<IL_OP_I_ADD, GPRV2F64,
      GPRF64, OpNode, "iadd">;
  def _v4f32: InsertVectorClass<IL_OP_I_ADD, GPRV4F32,
      GPRF32, OpNode, "iadd">;
  def _v2f32 : InsertVectorClass<IL_OP_I_ADD, GPRV2F32,
      GPRF32, OpNode, "iadd">;
  def _v2i64 : InsertVectorClass<IL_OP_I_ADD, GPRV2I64,
      GPRI64, OpNode, "iadd">;
  def _v4i8 : InsertVectorClass<IL_OP_I_ADD, GPRV4I8,
      GPRI8, OpNode, "iadd">;
  def _v4i16 : InsertVectorClass<IL_OP_I_ADD, GPRV4I16,
      GPRI16, OpNode, "iadd">;
  def _v4i32 : InsertVectorClass<IL_OP_I_ADD, GPRV4I32,
      GPRI32, OpNode, "iadd">;
  def _v2i8 : InsertVectorClass<IL_OP_I_ADD, GPRV2I8,
      GPRI8, OpNode, "iadd">;
  def _v2i16 : InsertVectorClass<IL_OP_I_ADD, GPRV2I16,
      GPRI16, OpNode, "iadd">;
  def _v2i32 : InsertVectorClass<IL_OP_I_ADD, GPRV2I32,
      GPRI32, OpNode, "iadd">;
}

// generic class that handles math instruction for OneInOneOut instruction
// patterns
multiclass UnaryOpMC<ILOpCode OpCode, SDNode OpNode> {
  def _i8    : UnaryOp<OpCode, OpNode, GPRI8, GPRI8>;
  def _i16    : UnaryOp<OpCode, OpNode, GPRI16, GPRI16>;
  def _i32    : UnaryOp<OpCode, OpNode, GPRI32, GPRI32>;
  def _f32    : UnaryOp<OpCode, OpNode, GPRF32, GPRF32>;
  def _f64    : UnaryOp<OpCode, OpNode, GPRF64, GPRF64>;
  def _i64    : UnaryOp<OpCode, OpNode, GPRI64, GPRI64>;
  def _v4f32: UnaryOp<OpCode, OpNode, GPRV4F32, GPRV4F32>;
  def _v4i16  : UnaryOp<OpCode, OpNode, GPRV4I16, GPRV4I16>;
  def _v4i8  : UnaryOp<OpCode, OpNode, GPRV4I8, GPRV4I8>;
  def _v4i32  : UnaryOp<OpCode, OpNode, GPRV4I32, GPRV4I32>;
  def _v2f32  : UnaryOp<OpCode, OpNode, GPRV2F32, GPRV2F32>;
  def _v2i16  : UnaryOp<OpCode, OpNode, GPRV2I16, GPRV2I16>;
  def _v2i8  : UnaryOp<OpCode, OpNode, GPRV2I8, GPRV2I8>;
  def _v2i32  : UnaryOp<OpCode, OpNode, GPRV2I32, GPRV2I32>;
  def _v2f64  : UnaryOp<OpCode, OpNode, GPRV2F64, GPRV2F64>;
  def _v2i64  : UnaryOp<OpCode, OpNode, GPRV2I64, GPRV2I64>;
}
multiclass UnaryOpMCVec<ILOpCode OpCode, SDNode OpNode> {
  def _v4f32: UnaryOp<OpCode, OpNode, GPRV4F32, GPRF32>;
  def _v4i16  : UnaryOp<OpCode, OpNode, GPRV4I16, GPRI16>;
  def _v4i8  : UnaryOp<OpCode, OpNode, GPRV4I8, GPRI8>;
  def _v4i32  : UnaryOp<OpCode, OpNode, GPRV4I32, GPRI32>;
  def _v2f32  : UnaryOp<OpCode, OpNode, GPRV2F32, GPRF32>;
  def _v2i16  : UnaryOp<OpCode, OpNode, GPRV2I16, GPRI16>;
  def _v2i8  : UnaryOp<OpCode, OpNode, GPRV2I8, GPRI8>;
  def _v2i32  : UnaryOp<OpCode, OpNode, GPRV2I32, GPRI32>;
  def _v2f64  : UnaryOp<OpCode, OpNode, GPRV2F64, GPRF64>;
  def _v2i64  : UnaryOp<OpCode, OpNode, GPRV2I64, GPRI64>;
}

multiclass UnaryOpMCf32<
ILOpCode f32OpCode,
         SDNode OpNode> {
           def _f32    : UnaryOp<f32OpCode, OpNode, GPRF32, GPRF32>;
           def _v4f32: UnaryOp<f32OpCode, OpNode, GPRV4F32, GPRV4F32>;
           def _v2f32  : UnaryOp<f32OpCode, OpNode, GPRV2F32, GPRV2F32>;
         }

multiclass UnaryOpMCi32<
ILOpCode i32OpCode,
         SDNode OpNode> {
           def _i8    : UnaryOp<i32OpCode, OpNode, GPRI8, GPRI8>;
           def _i16    : UnaryOp<i32OpCode, OpNode, GPRI16, GPRI16>;
           def _i32    : UnaryOp<i32OpCode, OpNode, GPRI32, GPRI32>;
           def _v4i16  : UnaryOp<i32OpCode, OpNode, GPRV4I16, GPRV4I16>;
           def _v4i8  : UnaryOp<i32OpCode, OpNode, GPRV4I8, GPRV4I8>;
           def _v4i32  : UnaryOp<i32OpCode, OpNode, GPRV4I32, GPRV4I32>;
           def _v2i16  : UnaryOp<i32OpCode, OpNode, GPRV2I16, GPRV2I16>;
           def _v2i8  : UnaryOp<i32OpCode, OpNode, GPRV2I8, GPRV2I8>;
           def _v2i32  : UnaryOp<i32OpCode, OpNode, GPRV2I32, GPRV2I32>;
         }


multiclass BinaryOpMC<ILOpCode OpCode, SDNode OpNode> {
  def _i8    : BinaryOp<OpCode, OpNode, GPRI8, GPRI8, GPRI8>;

  def _i16    : BinaryOp<OpCode, OpNode, GPRI16, GPRI16, GPRI16>;
  def _i32    : BinaryOp<OpCode, OpNode, GPRI32, GPRI32, GPRI32>;
  def _f32    : BinaryOp<OpCode, OpNode, GPRF32, GPRF32, GPRF32>;
  def _f64    : BinaryOp<OpCode, OpNode, GPRF64, GPRF64, GPRF64>;
  def _i64    : BinaryOp<OpCode, OpNode, GPRI64, GPRI64, GPRI64>;
  def _v4f32: BinaryOp<OpCode, OpNode, GPRV4F32, GPRV4F32, GPRV4F32>;
  def _v4i16  : BinaryOp<OpCode, OpNode, GPRV4I16, GPRV4I16, GPRV4I16>;
  def _v4i8  : BinaryOp<OpCode, OpNode, GPRV4I8, GPRV4I8, GPRV4I8>;
  def _v4i32  : BinaryOp<OpCode, OpNode, GPRV4I32, GPRV4I32, GPRV4I32>;
  def _v2f32  : BinaryOp<OpCode, OpNode, GPRV2F32, GPRV2F32, GPRV2F32>;
  def _v2i16  : BinaryOp<OpCode, OpNode, GPRV2I16, GPRV2I16, GPRV2I16>;
  def _v2i8  : BinaryOp<OpCode, OpNode, GPRV2I8, GPRV2I8, GPRV2I8>;
  def _v2i32  : BinaryOp<OpCode, OpNode, GPRV2I32, GPRV2I32, GPRV2I32>;
  def _v2f64  : BinaryOp<OpCode, OpNode, GPRV2F64, GPRV2F64, GPRV2F64>;
  def _v2i64  : BinaryOp<OpCode, OpNode, GPRV2I64, GPRV2I64, GPRV2I64>;
}

multiclass BinaryOpMCInt<ILOpCode OpCode, SDNode OpNode> {
  def _i8    : BinaryOp<OpCode, OpNode, GPRI8, GPRI8, GPRI8>;

  def _i16    : BinaryOp<OpCode, OpNode, GPRI16, GPRI16, GPRI16>;
  def _i32    : BinaryOp<OpCode, OpNode, GPRI32, GPRI32, GPRI32>;
  def _i64    : BinaryOp<OpCode, OpNode, GPRI64, GPRI64, GPRI64>;
  def _v4i16  : BinaryOp<OpCode, OpNode, GPRV4I16, GPRV4I16, GPRV4I16>;
  def _v4i8  : BinaryOp<OpCode, OpNode, GPRV4I8, GPRV4I8, GPRV4I8>;
  def _v4i32  : BinaryOp<OpCode, OpNode, GPRV4I32, GPRV4I32, GPRV4I32>;
  def _v2i16  : BinaryOp<OpCode, OpNode, GPRV2I16, GPRV2I16, GPRV2I16>;
  def _v2i8  : BinaryOp<OpCode, OpNode, GPRV2I8, GPRV2I8, GPRV2I8>;
  def _v2i32  : BinaryOp<OpCode, OpNode, GPRV2I32, GPRV2I32, GPRV2I32>;
  def _v2i64  : BinaryOp<OpCode, OpNode, GPRV2I64, GPRV2I64, GPRV2I64>;
}

// generic class that handles math instruction for ThreeInOneOut
// instruction patterns
multiclass TernaryOpMC<ILOpCode OpCode, SDNode OpNode> {
  def _i8    : TernaryOp<OpCode, OpNode, GPRI8, GPRI8, GPRI8, GPRI8>;
  def _i16    : TernaryOp<OpCode, OpNode, GPRI16, GPRI16, GPRI16, GPRI16>;
  def _i32    : TernaryOp<OpCode, OpNode, GPRI32, GPRI32, GPRI32, GPRI32>;
  def _f32    : TernaryOp<OpCode, OpNode, GPRF32, GPRF32, GPRF32, GPRF32>;
  def _f64    : TernaryOp<OpCode, OpNode, GPRF64, GPRF64, GPRF64, GPRF64>;
  def _i64    : TernaryOp<OpCode, OpNode, GPRI64, GPRI64, GPRI64, GPRI64>;
  def _v4f32: TernaryOp<OpCode, OpNode, GPRV4F32, GPRV4F32,
      GPRV4F32, GPRV4F32>;
  def _v4i8  : TernaryOp<OpCode, OpNode, GPRV4I8, GPRV4I8,
      GPRV4I8, GPRV4I8>;
  def _v4i16  : TernaryOp<OpCode, OpNode, GPRV4I16, GPRV4I16,
      GPRV4I16, GPRV4I16>;
  def _v4i32  : TernaryOp<OpCode, OpNode, GPRV4I32, GPRV4I32,
      GPRV4I32, GPRV4I32>;
  def _v2f32  : TernaryOp<OpCode, OpNode, GPRV2F32, GPRV2F32,
      GPRV2F32, GPRV2F32>;
  def _v2i8  : TernaryOp<OpCode, OpNode, GPRV2I8, GPRV2I8,
      GPRV2I8, GPRV2I8>;
  def _v2i16  : TernaryOp<OpCode, OpNode, GPRV2I16, GPRV2I16,
      GPRV2I16, GPRV2I16>;
  def _v2i32  : TernaryOp<OpCode, OpNode, GPRV2I32, GPRV2I32,
      GPRV2I32, GPRV2I32>;
  def _v2f64  : TernaryOp<OpCode, OpNode, GPRV2F64, GPRV2F64,
      GPRV2F64, GPRV2F64>;
  def _v2i64  : TernaryOp<OpCode, OpNode, GPRV2I64, GPRV2I64,
      GPRV2I64, GPRV2I64>;
}
multiclass BinaryOpMCi32<ILOpCode i32OpCode, SDNode OpNode> {
  def _i8    : BinaryOp<i32OpCode, OpNode, GPRI8, GPRI8, GPRI8>;
  def _i16   : BinaryOp<i32OpCode, OpNode, GPRI16, GPRI16, GPRI16>;
  def _i32   : BinaryOp<i32OpCode, OpNode, GPRI32, GPRI32, GPRI32>;
  def _v4i16 : BinaryOp<i32OpCode, OpNode, GPRV4I16,
      GPRV4I16, GPRV4I16>;
  def _v4i8  : BinaryOp<i32OpCode, OpNode, GPRV4I8,
      GPRV4I8, GPRV4I8>;
  def _v4i32 : BinaryOp<i32OpCode, OpNode, GPRV4I32,
      GPRV4I32, GPRV4I32>;
  def _v2i16 : BinaryOp<i32OpCode, OpNode, GPRV2I16,
      GPRV2I16, GPRV2I16>;
  def _v2i8  : BinaryOp<i32OpCode, OpNode, GPRV2I8,
      GPRV2I8, GPRV2I8>;
  def _v2i32 : BinaryOp<i32OpCode, OpNode, GPRV2I32,
      GPRV2I32, GPRV2I32>;
}
multiclass BinaryOpMCi64<ILOpCode i64OpCode, SDNode OpNode> {
  def _i64   : BinaryOp<i64OpCode, OpNode, GPRI64, GPRI64, GPRI64>;
  def _v2i64 : BinaryOp<i64OpCode, OpNode, GPRV2I64,
      GPRV2I64, GPRV2I64>;
}
multiclass BinaryOpMCi32Const<ILOpCode i32OpCode, SDNode OpNode> {
  def _i8    : BinaryOp<i32OpCode, OpNode, GPRI8, GPRI8, GPRI32>;
  def _i16   : BinaryOp<i32OpCode, OpNode, GPRI16, GPRI16, GPRI32>;
  def _i32   : BinaryOp<i32OpCode, OpNode, GPRI32, GPRI32, GPRI32>;
  def _v4i16 : BinaryOp<i32OpCode, OpNode, GPRV4I32,
      GPRV4I32, GPRI32>;
  def _v4i8  : BinaryOp<i32OpCode, OpNode, GPRV4I32,
      GPRV4I32, GPRI32>;
  def _v4i32 : BinaryOp<i32OpCode, OpNode, GPRV4I32,
      GPRV4I32, GPRI32>;
  def _v2i16 : BinaryOp<i32OpCode, OpNode, GPRV2I32,
      GPRV2I32, GPRI32>;
  def _v2i8  : BinaryOp<i32OpCode, OpNode, GPRV2I32,
      GPRV2I32, GPRI32>;
  def _v2i32 : BinaryOp<i32OpCode, OpNode, GPRV2I32,
      GPRV2I32, GPRI32>;
}
multiclass BinaryOpMCf32<ILOpCode f32OpCode, SDNode OpNode> {
  def _f32    : BinaryOp<f32OpCode, OpNode, GPRF32,
      GPRF32, GPRF32>;
  def _v4f32: BinaryOp<f32OpCode, OpNode, GPRV4F32,
      GPRV4F32, GPRV4F32>;
  def _v2f32  : BinaryOp<f32OpCode, OpNode, GPRV2F32,
      GPRV2F32, GPRV2F32>;
}

multiclass TernaryOpMCf64<ILOpCode f64OpCode, SDNode OpNode> {
  def _f64    : TernaryOp<f64OpCode, OpNode, GPRF64,
      GPRF64, GPRF64, GPRF64>;
}

multiclass TernaryOpMCf32<ILOpCode f32OpCode, SDNode OpNode> {
  def _f32    : TernaryOp<f32OpCode, OpNode, GPRF32,
      GPRF32, GPRF32, GPRF32>;
  def _v4f32: TernaryOp<f32OpCode, OpNode, GPRV4F32,
      GPRV4F32, GPRV4F32, GPRV4F32>;
  def _v2f32  : TernaryOp<f32OpCode, OpNode, GPRV2F32,
      GPRV2F32, GPRV2F32, GPRV2F32>;
}
multiclass BinaryOpMCFloat<ILOpCode f32OpCode, ILOpCode f64OpCode,
           SDNode OpNode> {
             def _f64    : BinaryOp<f64OpCode, OpNode, GPRF64,
             GPRF64, GPRF64>;
             def _v2f64  : BinaryOp<f64OpCode, OpNode, GPRV2F64,
                 GPRV2F64, GPRV2F64>;
             def _f32    : BinaryOp<f32OpCode, OpNode, GPRF32,
                 GPRF32, GPRF32>;
             def _v2f32  : BinaryOp<f32OpCode, OpNode, GPRV2F32,
                 GPRV2F32, GPRV2F32>;
             def _v4f32: BinaryOp<f32OpCode, OpNode, GPRV4F32,
                 GPRV4F32, GPRV4F32>;
           }

multiclass TernaryOpMCScalar<ILOpCode opcode, SDNode node>
{
    def _i8:  TernaryOp<opcode, node, GPRI8, GPRI8, GPRI8, GPRI8>;
    def _i16: TernaryOp<opcode, node, GPRI16, GPRI8, GPRI16, GPRI16>;
    def _i32: TernaryOp<opcode, node, GPRI32, GPRI8, GPRI32, GPRI32>;
    def _i64: TernaryOp<opcode, node, GPRI64, GPRI8, GPRI64, GPRI64>;
    def _f32: TernaryOp<opcode, node, GPRF32, GPRI8, GPRF32, GPRF32>;
    def _f64: TernaryOp<opcode, node, GPRF64, GPRI8, GPRF64, GPRF64>;
}


multiclass BitConversion<ILOpCode opcode, RegisterClass Regs, SDNode OpNode>
{
    def _i8    : UnaryOp<opcode, OpNode, Regs,    GPRI8>;
    def _i16   : UnaryOp<opcode, OpNode, Regs,   GPRI16>;
    def _i32   : UnaryOp<opcode, OpNode, Regs,   GPRI32>;
    def _f32   : UnaryOp<opcode, OpNode, Regs,   GPRF32>;
    def _i64   : UnaryOp<opcode, OpNode, Regs,   GPRI64>;
    def _f64   : UnaryOp<opcode, OpNode, Regs,   GPRF64>;
    def _v2i8  : UnaryOp<opcode, OpNode, Regs,  GPRV2I8>;
    def _v2i16 : UnaryOp<opcode, OpNode, Regs, GPRV2I16>;
    def _v2i32 : UnaryOp<opcode, OpNode, Regs, GPRV2I32>;
    def _v2f32 : UnaryOp<opcode, OpNode, Regs, GPRV2F32>;
    def _v2i64 : UnaryOp<opcode, OpNode, Regs, GPRV2I64>;
    def _v2f64 : UnaryOp<opcode, OpNode, Regs, GPRV2F64>;
    def _v4i8  : UnaryOp<opcode, OpNode, Regs,  GPRV4I8>;
    def _v4i16 : UnaryOp<opcode, OpNode, Regs, GPRV4I16>;
    def _v4i32 : UnaryOp<opcode, OpNode, Regs, GPRV4I32>;
    def _v4f32 : UnaryOp<opcode, OpNode, Regs, GPRV4F32>;
}


multiclass UnaryIntrinsicInt<ILOpCode opcode, Intrinsic intr>
{
def _i32 : OneInOneOut<opcode, (outs GPRI32:$dst),
      (ins GPRI32:$src),
      !strconcat(opcode.Text, " $dst, $src"),
      [(set GPRI32:$dst, (intr GPRI32:$src))]>;
def _v2i32 : OneInOneOut<opcode, (outs GPRV2I32:$dst),
      (ins GPRV2I32:$src),
      !strconcat(opcode.Text, " $dst, $src"),
      [(set GPRV2I32:$dst, (intr GPRV2I32:$src))]>;
def _v4i32 : OneInOneOut<opcode, (outs GPRV4I32:$dst),
      (ins GPRV4I32:$src),
      !strconcat(opcode.Text, " $dst, $src"),
      [(set GPRV4I32:$dst, (intr GPRV4I32:$src))]>;
}

multiclass IntrConvertF32TOF16<ILOpCode opcode, Intrinsic intr>
{
def _i16 : OneInOneOut<opcode, (outs GPRI16:$dst),
      (ins GPRF32:$src),
      !strconcat(opcode.Text, " $dst, $src"),
      [(set GPRI16:$dst, (intr GPRF32:$src))]>;
def _v2i16 : OneInOneOut<opcode, (outs GPRV2I16:$dst),
      (ins GPRV2F32:$src),
      !strconcat(opcode.Text, " $dst, $src"),
      [(set GPRV2I16:$dst, (intr GPRV2F32:$src))]>;
def _v4i16 : OneInOneOut<opcode, (outs GPRV4I16:$dst),
      (ins GPRV4F32:$src),
      !strconcat(opcode.Text, " $dst, $src"),
      [(set GPRV4I16:$dst, (intr GPRV4F32:$src))]>;
}


multiclass IntrConvertF32TOI32<ILOpCode opcode, Intrinsic intr>
{
def _i32 : OneInOneOut<opcode, (outs GPRI32:$dst),
      (ins GPRF32:$src),
      !strconcat(opcode.Text, " $dst, $src"),
      [(set GPRI32:$dst, (intr GPRF32:$src))]>;
def _v2i32 : OneInOneOut<opcode, (outs GPRV2I32:$dst),
      (ins GPRV2F32:$src),
      !strconcat(opcode.Text, " $dst, $src"),
      [(set GPRV2I32:$dst, (intr GPRV2F32:$src))]>;
def _v4i32 : OneInOneOut<opcode, (outs GPRV4I32:$dst),
      (ins GPRV4F32:$src),
      !strconcat(opcode.Text, " $dst, $src"),
      [(set GPRV4I32:$dst, (intr GPRV4F32:$src))]>;
}

multiclass IntrConvertF64TOI32<ILOpCode opcode, Intrinsic intr>
{
def _i32 : OneInOneOut<opcode, (outs GPRI32:$dst),
      (ins GPRF64:$src),
      !strconcat(opcode.Text, " $dst, $src"),
      [(set GPRI32:$dst, (intr GPRF64:$src))]>;
def _v2i32 : OneInOneOut<opcode, (outs GPRV2I32:$dst),
      (ins GPRV2F64:$src),
      !strconcat(opcode.Text, " $dst, $src"),
      [(set GPRV2I32:$dst, (intr GPRV2F64:$src))]>;
}

multiclass IntrConvertF16TOF32<ILOpCode opcode, Intrinsic intr>
{
def _f32 : OneInOneOut<opcode, (outs GPRF32:$dst),
      (ins GPRI16:$src),
      !strconcat(opcode.Text, " $dst, $src"),
      [(set GPRF32:$dst, (intr GPRI16:$src))]>;
def _v2f32 : OneInOneOut<opcode, (outs GPRV2F32:$dst),
      (ins GPRV2I16:$src),
      !strconcat(opcode.Text, " $dst, $src"),
      [(set GPRV2F32:$dst, (intr GPRV2I16:$src))]>;
def _v4f32 : OneInOneOut<opcode, (outs GPRV4F32:$dst),
      (ins GPRV4I16:$src),
      !strconcat(opcode.Text, " $dst, $src"),
      [(set GPRV4F32:$dst, (intr GPRV4I16:$src))]>;
}


multiclass IntrConvertI32TOF32<ILOpCode opcode, Intrinsic intr>
{
def _f32 : OneInOneOut<opcode, (outs GPRF32:$dst),
      (ins GPRI32:$src),
      !strconcat(opcode.Text, " $dst, $src"),
      [(set GPRF32:$dst, (intr GPRI32:$src))]>;
def _v2f32 : OneInOneOut<opcode, (outs GPRV2F32:$dst),
      (ins GPRV2I32:$src),
      !strconcat(opcode.Text, " $dst, $src"),
      [(set GPRV2F32:$dst, (intr GPRV2I32:$src))]>;
def _v4f32 : OneInOneOut<opcode, (outs GPRV4F32:$dst),
      (ins GPRV4I32:$src),
      !strconcat(opcode.Text, " $dst, $src"),
      [(set GPRV4F32:$dst, (intr GPRV4I32:$src))]>;
}

multiclass BinaryIntrinsicLong<ILOpCode opcode, Intrinsic intr>
{
def _i64 : TwoInOneOut<opcode, (outs GPRI64:$dst),
      (ins GPRI64:$src, GPRI64:$src2),
      !strconcat(opcode.Text, " $dst, $src, $src2"),
      [(set GPRI64:$dst,
      (intr GPRI64:$src, GPRI64:$src2))]>;
}


multiclass BinaryIntrinsicInt<ILOpCode opcode, Intrinsic intr>
{
def _i32 : TwoInOneOut<opcode, (outs GPRI32:$dst),
      (ins GPRI32:$src, GPRI32:$src2),
      !strconcat(opcode.Text, " $dst, $src, $src2"),
      [(set GPRI32:$dst,
      (intr GPRI32:$src, GPRI32:$src2))]>;
def _v2i32 : TwoInOneOut<opcode, (outs GPRV2I32:$dst),
      (ins GPRV2I32:$src, GPRV2I32:$src2),
      !strconcat(opcode.Text, " $dst, $src, $src2"),
      [(set GPRV2I32:$dst,
      (intr GPRV2I32:$src, GPRV2I32:$src2))]>;
def _v4i32 : TwoInOneOut<opcode, (outs GPRV4I32:$dst),
      (ins GPRV4I32:$src, GPRV4I32:$src2),
      !strconcat(opcode.Text, " $dst, $src, $src2"),
      [(set GPRV4I32:$dst,
      (intr GPRV4I32:$src, GPRV4I32:$src2))]>;
}

multiclass TernaryIntrinsicInt<ILOpCode opcode, Intrinsic intr>
{
def _i32 : ThreeInOneOut<opcode, (outs GPRI32:$dst),
      (ins GPRI32:$src, GPRI32:$src2, GPRI32:$src3),
      !strconcat(opcode.Text, " $dst, $src, $src2, $src3"),
      [(set GPRI32:$dst,
      (intr GPRI32:$src, GPRI32:$src2, GPRI32:$src3))]>;
def _v2i32 : ThreeInOneOut<opcode, (outs GPRV2I32:$dst),
      (ins GPRV2I32:$src, GPRV2I32:$src2, GPRV2I32:$src3),
      !strconcat(opcode.Text, " $dst, $src, $src2, $src3"),
      [(set GPRV2I32:$dst,
      (intr GPRV2I32:$src, GPRV2I32:$src2, GPRV2I32:$src3))]>;
def _v4i32 : ThreeInOneOut<opcode, (outs GPRV4I32:$dst),
      (ins GPRV4I32:$src, GPRV4I32:$src2, GPRV4I32:$src3),
      !strconcat(opcode.Text, " $dst, $src, $src2, $src3"),
      [(set GPRV4I32:$dst,
      (intr GPRV4I32:$src, GPRV4I32:$src2, GPRV4I32:$src3))]>;
}

multiclass TernaryIntrinsicFloat<ILOpCode opcode, Intrinsic intr>
{
def _f32 : ThreeInOneOut<opcode, (outs GPRF32:$dst),
      (ins GPRF32:$src, GPRF32:$src2, GPRF32:$src3),
      !strconcat(opcode.Text, " $dst, $src, $src2, $src3"),
      [(set GPRF32:$dst,
      (intr GPRF32:$src, GPRF32:$src2, GPRF32:$src3))]>;
def _v2f32 : ThreeInOneOut<opcode, (outs GPRV2F32:$dst),
      (ins GPRV2F32:$src, GPRV2F32:$src2, GPRV2F32:$src3),
      !strconcat(opcode.Text, " $dst, $src, $src2, $src3"),
      [(set GPRV2F32:$dst,
      (intr GPRV2F32:$src, GPRV2F32:$src2, GPRV2F32:$src3))]>;
def _v4f32 : ThreeInOneOut<opcode, (outs GPRV4F32:$dst),
      (ins GPRV4F32:$src, GPRV4F32:$src2, GPRV4F32:$src3),
      !strconcat(opcode.Text, " $dst, $src, $src2, $src3"),
      [(set GPRV4F32:$dst,
      (intr GPRV4F32:$src, GPRV4F32:$src2, GPRV4F32:$src3))]>;
}

multiclass BinaryIntrinsicDoubleScalar<ILOpCode opcode, Intrinsic intr>
{
def _f64 : ThreeInOneOut<opcode, (outs GPRF64:$dst),
      (ins GPRF64:$src, GPRF64:$src2),
      !strconcat(opcode.Text, " $dst, $src, $src2"),
      [(set GPRF64:$dst,
      (intr GPRF64:$src, GPRF64:$src2))]>;
}

multiclass TernaryIntrinsicDoubleScalar<ILOpCode opcode, Intrinsic intr>
{
def _f64 : ThreeInOneOut<opcode, (outs GPRF64:$dst),
      (ins GPRF64:$src, GPRF64:$src2, GPRF64:$src3),
      !strconcat(opcode.Text, " $dst, $src, $src2, $src3"),
      [(set GPRF64:$dst,
      (intr GPRF64:$src, GPRF64:$src2, GPRF64:$src3))]>;
}


multiclass TernaryIntrinsicLongScalar<ILOpCode opcode, Intrinsic intr>
{
def _i64 : ThreeInOneOut<opcode, (outs GPRI64:$dst),
      (ins GPRI64:$src, GPRI64:$src2, GPRI64:$src3),
      !strconcat(opcode.Text, " $dst, $src, $src2, $src3"),
      [(set GPRI64:$dst,
      (intr GPRI64:$src, GPRI64:$src2, GPRI64:$src3))]>;
}

multiclass QuaternaryIntrinsicInt<ILOpCode opcode, Intrinsic intr>
{
def _i32 : FourInOneOut<opcode, (outs GPRI32:$dst),
      (ins GPRI32:$src, GPRI32:$src2, GPRI32:$src3, GPRI32:$src4),
      !strconcat(opcode.Text, " $dst, $src, $src2, $src3, $src4"),
      [(set GPRI32:$dst,
      (intr GPRI32:$src, GPRI32:$src2, GPRI32:$src3, GPRI32:$src4))]>;
def _v2i32 : FourInOneOut<opcode, (outs GPRV2I32:$dst),
      (ins GPRV2I32:$src, GPRV2I32:$src2, GPRV2I32:$src3, GPRV2I32:$src4),
      !strconcat(opcode.Text, " $dst, $src, $src2, $src3, $src4"),
      [(set GPRV2I32:$dst,
      (intr GPRV2I32:$src, GPRV2I32:$src2, GPRV2I32:$src3, GPRV2I32:$src4))]>;
def _v4i32 : FourInOneOut<opcode, (outs GPRV4I32:$dst),
      (ins GPRV4I32:$src, GPRV4I32:$src2, GPRV4I32:$src3, GPRV4I32:$src4),
      !strconcat(opcode.Text, " $dst, $src, $src2, $src3, $src4"),
      [(set GPRV4I32:$dst,
      (intr GPRV4I32:$src, GPRV4I32:$src2, GPRV4I32:$src3, GPRV4I32:$src4))]>;
}

multiclass UnaryIntrinsicFloatScalar<ILOpCode opcode, Intrinsic intr>
{
def _f32 : OneInOneOut<opcode, (outs GPRF32:$dst),
      (ins GPRF32:$src),
      !strconcat(opcode.Text, " $dst, $src"),
      [(set GPRF32:$dst, (intr GPRF32:$src))]>;
}

multiclass UnaryIntrinsicFloat<ILOpCode opcode, Intrinsic intr>
{
def _f32 : OneInOneOut<opcode, (outs GPRF32:$dst),
      (ins GPRF32:$src),
      !strconcat(opcode.Text, " $dst, $src"),
      [(set GPRF32:$dst, (intr GPRF32:$src))]>;
def _v2f32 : OneInOneOut<opcode, (outs GPRV2F32:$dst),
      (ins GPRV2F32:$src),
      !strconcat(opcode.Text, " $dst, $src"),
      [(set GPRV2F32:$dst, (intr GPRV2F32:$src))]>;
def _v4f32 : OneInOneOut<opcode, (outs GPRV4F32:$dst),
      (ins GPRV4F32:$src),
      !strconcat(opcode.Text, " $dst, $src"),
      [(set GPRV4F32:$dst, (intr GPRV4F32:$src))]>;
}

multiclass BinaryIntrinsicFloatScalar<ILOpCode opcode, Intrinsic intr>
{
def _f32 : TwoInOneOut<opcode, (outs GPRF32:$dst),
      (ins GPRF32:$src, GPRF32:$src2),
      !strconcat(opcode.Text, " $dst, $src, $src2"),
      [(set GPRF32:$dst,
      (intr GPRF32:$src, GPRF32:$src2))]>;
}
multiclass BinaryIntrinsicFloat<ILOpCode opcode, Intrinsic intr>
{
def _f32 : TwoInOneOut<opcode, (outs GPRF32:$dst),
      (ins GPRF32:$src, GPRF32:$src2),
      !strconcat(opcode.Text, " $dst, $src, $src2"),
      [(set GPRF32:$dst,
      (intr GPRF32:$src, GPRF32:$src2))]>;
def _v2f32 : TwoInOneOut<opcode, (outs GPRV2F32:$dst),
      (ins GPRV2F32:$src, GPRV2F32:$src2),
      !strconcat(opcode.Text, " $dst, $src, $src2"),
      [(set GPRV2F32:$dst,
      (intr GPRV2F32:$src, GPRV2F32:$src2))]>;
def _v4f32 : TwoInOneOut<opcode, (outs GPRV4F32:$dst),
      (ins GPRV4F32:$src, GPRV4F32:$src2),
      !strconcat(opcode.Text, " $dst, $src, $src2"),
      [(set GPRV4F32:$dst,
      (intr GPRV4F32:$src, GPRV4F32:$src2))]>;
}

multiclass UnaryIntrinsicDoubleScalar<ILOpCode opcode, Intrinsic intr>
{
def _f64 : OneInOneOut<opcode, (outs GPRF64:$dst),
      (ins GPRF64:$src),
      !strconcat(opcode.Text, " $dst, $src"),
      [(set GPRF64:$dst, (intr GPRF64:$src))]>;
}

multiclass UnaryIntrinsicDouble<ILOpCode opcode, Intrinsic intr>
{
def _f64 : OneInOneOut<opcode, (outs GPRF64:$dst),
      (ins GPRF64:$src),
      !strconcat(opcode.Text, " $dst, $src"),
      [(set GPRF64:$dst, (intr GPRF64:$src))]>;
def _v2f64 : OneInOneOut<opcode, (outs GPRV2F64:$dst),
      (ins GPRV2F64:$src),
      !strconcat(opcode.Text, " $dst, $src"),
      [(set GPRV2F64:$dst, (intr GPRV2F64:$src))]>;
}

multiclass BinaryIntrinsicDouble<ILOpCode opcode, Intrinsic intr>
{
def _f64 : TwoInOneOut<opcode, (outs GPRF64:$dst),
      (ins GPRF64:$src, GPRF64:$src2),
      !strconcat(opcode.Text, " $dst, $src, $src2"),
      [(set GPRF64:$dst,
      (intr GPRF64:$src, GPRF64:$src2))]>;
def _v2f64 : TwoInOneOut<opcode, (outs GPRV2F64:$dst),
      (ins GPRV2F64:$src, GPRV2F64:$src2),
      !strconcat(opcode.Text, " $dst, $src, $src2"),
      [(set GPRV2F64:$dst,
      (intr GPRV2F64:$src, GPRV2F64:$src2))]>;
}

multiclass TernaryIntrinsicDouble<ILOpCode opcode, Intrinsic intr>
{
def _f64 : TwoInOneOut<opcode, (outs GPRF64:$dst),
      (ins GPRF64:$src, GPRF64:$src2, GPRF64:$src3),
      !strconcat(opcode.Text, " $dst, $src, $src2, $src3"),
      [(set GPRF64:$dst,
      (intr GPRF64:$src, GPRF64:$src2, GPRF64:$src3))]>;
def _v2f64 : TwoInOneOut<opcode, (outs GPRV2F64:$dst),
      (ins GPRV2F64:$src, GPRV2F64:$src2, GPRV2F64:$src3),
      !strconcat(opcode.Text, " $dst, $src, $src2, $src3"),
      [(set GPRV2F64:$dst,
      (intr GPRV2F64:$src, GPRV2F64:$src2, GPRV2F64:$src3))]>;
}

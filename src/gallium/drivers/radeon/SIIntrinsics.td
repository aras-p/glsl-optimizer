//===-- SIIntrinsics.td - TODO: Add brief description -------===//
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


let TargetPrefix = "SI", isTarget = 1 in {

  def int_SI_export : Intrinsic <[], [llvm_i32_ty, llvm_i32_ty, llvm_i32_ty, llvm_i32_ty, llvm_i32_ty, llvm_float_ty, llvm_float_ty, llvm_float_ty, llvm_float_ty], []>;
  /* XXX: We may need a seperate intrinsic here for loading integer values */
  def int_SI_load_const : Intrinsic <[llvm_float_ty], [llvm_i64_ty, llvm_i32_ty], []>;
  def int_SI_vs_load_buffer_index : Intrinsic <[llvm_i32_ty], [], []>;
  def int_SI_vs_load_input : Intrinsic <[llvm_v4f32_ty], [llvm_i64_ty, llvm_i32_ty, llvm_i16_ty, llvm_i32_ty], []> ;

  def int_SI_sample : Intrinsic <[llvm_v4f32_ty], [llvm_i32_ty, llvm_v4f32_ty, llvm_ptr_ty, llvm_i32_ty, llvm_ptr_ty, llvm_i32_ty]>;
  def int_SI_use_sgpr : Intrinsic <[llvm_anyint_ty], [llvm_i32_ty], [IntrNoMem]>;


  /* Interpolation Intrinsics */

  def int_SI_set_M0 : Intrinsic <[llvm_i32_ty], [llvm_i32_ty]>;
  class Interp : Intrinsic <[llvm_float_ty], [llvm_i32_ty, llvm_i32_ty, llvm_i32_ty], []>;

  def int_SI_fs_interp_linear_center : Interp;
  def int_SI_fs_interp_constant : Interp;
}

//===-- AMDGPURegisterInfo.td - TODO: Add brief description -------===//
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

let Namespace = "AMDIL" in {
  def sel_x : SubRegIndex;
  def sel_y : SubRegIndex;
  def sel_z : SubRegIndex;
  def sel_w : SubRegIndex;
}

include "R600RegisterInfo.td"
include "SIRegisterInfo.td"

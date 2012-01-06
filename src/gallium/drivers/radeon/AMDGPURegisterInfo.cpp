//===-- AMDGPURegisterInfo.cpp - TODO: Add brief description -------===//
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

#include "AMDGPURegisterInfo.h"
#include "AMDGPUTargetMachine.h"

using namespace llvm;

AMDGPURegisterInfo::AMDGPURegisterInfo(AMDGPUTargetMachine &tm,
    const TargetInstrInfo &tii)
: AMDILRegisterInfo(tm, tii),
  TM(tm),
  TII(tii)
  { }

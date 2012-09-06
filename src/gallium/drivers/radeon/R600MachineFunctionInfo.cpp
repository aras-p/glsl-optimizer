//===-- R600MachineFunctionInfo.cpp - R600 Machine Function Info-*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "R600MachineFunctionInfo.h"

using namespace llvm;

R600MachineFunctionInfo::R600MachineFunctionInfo(const MachineFunction &MF)
  : MachineFunctionInfo(),
    HasLinearInterpolation(false),
    HasPerspectiveInterpolation(false)
  { }

unsigned R600MachineFunctionInfo::GetIJPerspectiveIndex() const
{
  assert(HasPerspectiveInterpolation);
  return 0;
}

unsigned R600MachineFunctionInfo::GetIJLinearIndex() const
{
  assert(HasLinearInterpolation);
  if (HasPerspectiveInterpolation)
    return 1;
  else
    return 0;
}

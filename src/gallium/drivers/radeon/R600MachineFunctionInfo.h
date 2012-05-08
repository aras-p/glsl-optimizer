//===-- R600MachineFunctionInfo.h - TODO: Add brief description ---*- C++ -*-=//
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

#ifndef R600MACHINEFUNCTIONINFO_H
#define R600MACHINEFUNCTIONINFO_H

#include "llvm/CodeGen/MachineFunction.h"
#include <vector>

namespace llvm {

class R600MachineFunctionInfo : public MachineFunctionInfo {

public:
  R600MachineFunctionInfo(const MachineFunction &MF);
  std::vector<unsigned> ReservedRegs;

};

} // End llvm namespace

#endif //R600MACHINEFUNCTIONINFO_H

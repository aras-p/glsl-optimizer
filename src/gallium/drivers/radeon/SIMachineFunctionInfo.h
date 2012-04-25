//===-- SIMachineFunctionInfo.h - TODO: Add brief description -------===//
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


#ifndef _SIMACHINEFUNCTIONINFO_H_
#define _SIMACHINEFUNCTIONINFO_H_

#include "llvm/CodeGen/MachineFunction.h"

namespace llvm {

class SIMachineFunctionInfo : public MachineFunctionInfo {

  private:

  public:
    SIMachineFunctionInfo(const MachineFunction &MF);
    unsigned spi_ps_input_addr;

};

} // End namespace llvm


#endif //_SIMACHINEFUNCTIONINFO_H_

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

#include "AMDILMachineFunctionInfo.h"

namespace llvm {

class SIMachineFunctionInfo : public AMDILMachineFunctionInfo {

  private:

  public:
    SIMachineFunctionInfo();
    SIMachineFunctionInfo(MachineFunction &MF);
    unsigned spi_ps_input_addr;

};

} // End namespace llvm


#endif //_SIMACHINEFUNCTIONINFO_H_

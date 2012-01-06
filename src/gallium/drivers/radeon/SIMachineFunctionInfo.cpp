//===-- SIMachineFunctionInfo.cpp - TODO: Add brief description -------===//
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


#include "SIMachineFunctionInfo.h"
#include "AMDGPU.h"
#include "llvm/CodeGen/MachineFunctionPass.h"

using namespace llvm;


SIMachineFunctionInfo::SIMachineFunctionInfo()
  : AMDILMachineFunctionInfo(),
    spi_ps_input_addr(0)
  { }

SIMachineFunctionInfo::SIMachineFunctionInfo(MachineFunction &MF)
  : AMDILMachineFunctionInfo(MF),
    spi_ps_input_addr(0)
  { }


namespace {
  class SIInitMachineFunctionInfoPass : public MachineFunctionPass {

  private:
    static char ID;
    TargetMachine &TM;

  public:
    SIInitMachineFunctionInfoPass(TargetMachine &tm) :
      MachineFunctionPass(ID), TM(tm) { }
    virtual bool runOnMachineFunction(MachineFunction &MF);
  };
} // End anonymous namespace

char SIInitMachineFunctionInfoPass::ID = 0;

FunctionPass *llvm::createSIInitMachineFunctionInfoPass(TargetMachine &tm) {
  return new SIInitMachineFunctionInfoPass(tm);
}

/* A MachineFunction's MachineFunctionInfo is initialized in the first call to
 * getInfo().  We need to intialize it as an SIMachineFunctionInfo object
 * before any of the AMDIL passes otherwise it will be an
 * AMDILMachineFunctionInfo object and we won't be able to use it.
 */
bool SIInitMachineFunctionInfoPass::runOnMachineFunction(MachineFunction &MF)
{
  SIMachineFunctionInfo * MFI = MF.getInfo<SIMachineFunctionInfo>();
  return false;
}

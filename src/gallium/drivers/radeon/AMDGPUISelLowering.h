//===-- AMDGPUISelLowering.h - AMDGPU Lowering Interface --------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the interface defintiion of the TargetLowering class
// that is common to all AMD GPUs.
//
//===----------------------------------------------------------------------===//

#ifndef AMDGPUISELLOWERING_H
#define AMDGPUISELLOWERING_H

#include "AMDILISelLowering.h"

namespace llvm {

class AMDGPUTargetLowering : public AMDILTargetLowering
{
protected:

  /// addLiveIn - This functions adds reg to the live in list of the entry block
  /// and emits a copy from reg to MI.getOperand(0).
  ///
  //  Some registers are loaded with values before the program
  /// begins to execute.  The loading of these values is modeled with pseudo
  /// instructions which are lowered using this function. 
  void addLiveIn(MachineInstr * MI, MachineFunction * MF,
                 MachineRegisterInfo & MRI, const TargetInstrInfo * TII,
		 unsigned reg) const;

public:
  AMDGPUTargetLowering(TargetMachine &TM);

};

} // End namespace llvm

#endif // AMDGPUISELLOWERING_H

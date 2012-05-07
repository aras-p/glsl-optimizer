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
  void addLiveIn(MachineInstr * MI, MachineFunction * MF,
                 MachineRegisterInfo & MRI, const struct TargetInstrInfo * TII,
		 unsigned reg) const;

public:
  AMDGPUTargetLowering(TargetMachine &TM);

};

} /* End namespace llvm */

#endif /* AMDGPUISELLOWERING_H */

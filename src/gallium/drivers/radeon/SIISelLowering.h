//===-- SIISelLowering.h - SI DAG Lowering Interface ------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// SI DAG Lowering interface definition
//
//===----------------------------------------------------------------------===//

#ifndef SIISELLOWERING_H
#define SIISELLOWERING_H

#include "AMDGPUISelLowering.h"
#include "SIInstrInfo.h"

namespace llvm {

class SITargetLowering : public AMDGPUTargetLowering
{
  const SIInstrInfo * TII;

  void AppendS_WAITCNT(MachineInstr *MI, MachineBasicBlock &BB,
              MachineBasicBlock::iterator I) const;
  void LowerSI_INTERP(MachineInstr *MI, MachineBasicBlock &BB,
              MachineBasicBlock::iterator I, MachineRegisterInfo & MRI) const;
  void LowerSI_INTERP_CONST(MachineInstr *MI, MachineBasicBlock &BB,
              MachineBasicBlock::iterator I) const;
  void LowerSI_V_CNDLT(MachineInstr *MI, MachineBasicBlock &BB,
              MachineBasicBlock::iterator I, MachineRegisterInfo & MRI) const;
  void lowerUSE_SGPR(MachineInstr *MI, MachineFunction * MF,
                     MachineRegisterInfo & MRI) const;
public:
  SITargetLowering(TargetMachine &tm);
  virtual MachineBasicBlock * EmitInstrWithCustomInserter(MachineInstr * MI,
                                              MachineBasicBlock * BB) const;
};

} // End namespace llvm

#endif //SIISELLOWERING_H

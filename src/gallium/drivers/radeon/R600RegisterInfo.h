//===-- R600RegisterInfo.h - TODO: Add brief description -------===//
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

#ifndef R600REGISTERINFO_H_
#define R600REGISTERINFO_H_

#include "AMDGPUTargetMachine.h"
#include "AMDILRegisterInfo.h"

namespace llvm {

  class R600TargetMachine;
  class TargetInstrInfo;

  struct R600RegisterInfo : public AMDGPURegisterInfo
  {
    AMDGPUTargetMachine &TM;
    const TargetInstrInfo &TII;

    R600RegisterInfo(AMDGPUTargetMachine &tm, const TargetInstrInfo &tii);

    virtual BitVector getReservedRegs(const MachineFunction &MF) const;

    virtual const TargetRegisterClass *
    getISARegClass(const TargetRegisterClass * rc) const;
    unsigned getHWRegIndex(unsigned reg) const;
    unsigned getHWRegChan(unsigned reg) const;
private:
    unsigned getHWRegChanGen(unsigned reg) const;
    unsigned getHWRegIndexGen(unsigned reg) const;
  };
} // End namespace llvm

#endif // AMDIDSAREGISTERINFO_H_

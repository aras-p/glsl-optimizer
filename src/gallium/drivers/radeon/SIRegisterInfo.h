//===-- SIRegisterInfo.h - SI Register Info Interface ----------*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Interface definition for SIRegisterInfo
//
//===----------------------------------------------------------------------===//


#ifndef SIREGISTERINFO_H_
#define SIREGISTERINFO_H_

#include "AMDGPURegisterInfo.h"

namespace llvm {

  class AMDGPUTargetMachine;
  class TargetInstrInfo;

  struct SIRegisterInfo : public AMDGPURegisterInfo
  {
    AMDGPUTargetMachine &TM;
    const TargetInstrInfo &TII;

    SIRegisterInfo(AMDGPUTargetMachine &tm, const TargetInstrInfo &tii);

    virtual BitVector getReservedRegs(const MachineFunction &MF) const;
    virtual unsigned getBinaryCode(unsigned reg) const;

    virtual bool isBaseRegClass(unsigned regClassID) const;

    virtual const TargetRegisterClass *
    getISARegClass(const TargetRegisterClass * rc) const;

    unsigned getHWRegNum(unsigned reg) const;

  };

} // End namespace llvm

#endif // SIREGISTERINFO_H_

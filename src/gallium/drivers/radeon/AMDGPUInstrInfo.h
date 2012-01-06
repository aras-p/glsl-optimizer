//===-- AMDGPUInstrInfo.h - TODO: Add brief description -------===//
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

#ifndef AMDGPUINSTRUCTIONINFO_H_
#define AMDGPUINSTRUCTIONINFO_H_

#include "AMDGPURegisterInfo.h"
#include "AMDILInstrInfo.h"

#include <map>

namespace llvm {

  class AMDGPUTargetMachine;
  class MachineFunction;
  class MachineInstr;
  class MachineInstrBuilder;

  class AMDGPUInstrInfo : public AMDILInstrInfo {
  private:
  AMDGPUTargetMachine & TM;
  std::map<unsigned, unsigned> amdilToISA;

  public:
  explicit AMDGPUInstrInfo(AMDGPUTargetMachine &tm);

  virtual const AMDGPURegisterInfo &getRegisterInfo() const = 0;

  virtual unsigned getISAOpcode(unsigned AMDILopcode) const;

  virtual MachineInstr * convertToISA(MachineInstr & MI, MachineFunction &MF,
    DebugLoc DL) const;

  bool isRegPreload(const MachineInstr &MI) const;

  #include "AMDGPUInstrEnums.h.include"
  };

} // End llvm namespace

/* AMDGPU target flags are stored in bits 32-39 */
namespace AMDGPU_TFLAG_SHIFTS {
  enum TFLAGS {
    PRELOAD_REG = 32
  };
}


#endif // AMDGPUINSTRINFO_H_

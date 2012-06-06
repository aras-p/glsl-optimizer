//===-- R600InstrInfo.h - R600 Instruction Info Interface -------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Interface definition for R600InstrInfo
//
//===----------------------------------------------------------------------===//

#ifndef R600INSTRUCTIONINFO_H_
#define R600INSTRUCTIONINFO_H_

#include "AMDIL.h"
#include "AMDILInstrInfo.h"
#include "R600RegisterInfo.h"

#include <map>

namespace llvm {

  class AMDGPUTargetMachine;
  class MachineFunction;
  class MachineInstr;
  class MachineInstrBuilder;

  class R600InstrInfo : public AMDGPUInstrInfo {
  private:
  const R600RegisterInfo RI;
  AMDGPUTargetMachine &TM;

  public:
  explicit R600InstrInfo(AMDGPUTargetMachine &tm);

  const R600RegisterInfo &getRegisterInfo() const;
  virtual void copyPhysReg(MachineBasicBlock &MBB,
                           MachineBasicBlock::iterator MI, DebugLoc DL,
                           unsigned DestReg, unsigned SrcReg,
                           bool KillSrc) const;

  bool isTrig(const MachineInstr &MI) const;

  virtual MachineInstr * getMovImmInstr(MachineFunction *MF, unsigned DstReg,
                                        int64_t Imm) const;

  virtual unsigned getIEQOpcode() const;
  virtual bool isMov(unsigned Opcode) const;
};

} // End llvm namespace

namespace R600_InstFlag {
	enum TIF {
		TRANS_ONLY = (1 << 0),
		TEX = (1 << 1),
		REDUCTION = (1 << 2),
		FC = (1 << 3),
		TRIG = (1 << 4),
		OP3 = (1 << 5)
	};
}

#endif // R600INSTRINFO_H_

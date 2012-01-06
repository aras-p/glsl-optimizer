//===- AMDILInstrInfo.h - AMDIL Instruction Information ---------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//
//
// This file contains the AMDIL implementation of the TargetInstrInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef AMDILINSTRUCTIONINFO_H_
#define AMDILINSTRUCTIONINFO_H_

#include "AMDILRegisterInfo.h"
#include "llvm/Target/TargetInstrInfo.h"

#define GET_INSTRINFO_HEADER
#include "AMDILGenInstrInfo.inc"

namespace llvm {
  // AMDIL - This namespace holds all of the target specific flags that
  // instruction info tracks.
  //
  //class AMDILTargetMachine;
class AMDILInstrInfo : public AMDILGenInstrInfo {
private:
  const AMDILRegisterInfo RI;
  AMDILTargetMachine &TM;
  bool getNextBranchInstr(MachineBasicBlock::iterator &iter,
                          MachineBasicBlock &MBB) const;
  unsigned int getBranchInstr(const MachineOperand &op) const;
public:
  explicit AMDILInstrInfo(AMDILTargetMachine &tm);

  // getRegisterInfo - TargetInstrInfo is a superset of MRegister info.  As
  // such, whenever a client has an instance of instruction info, it should
  // always be able to get register info as well (through this method).
  const AMDILRegisterInfo &getRegisterInfo() const;

  // Return true if the instruction is a register to register move and leave the
  // source and dest operands in the passed parameters.
  bool isMoveInstr(const MachineInstr &MI, unsigned int &SrcReg,
                   unsigned int &DstReg, unsigned int &SrcSubIdx,
                   unsigned int &DstSubIdx) const;

  bool isCoalescableExtInstr(const MachineInstr &MI, unsigned &SrcReg,
                             unsigned &DstReg, unsigned &SubIdx) const;

  unsigned isLoadFromStackSlot(const MachineInstr *MI, int &FrameIndex) const;
  unsigned isLoadFromStackSlotPostFE(const MachineInstr *MI,
                                     int &FrameIndex) const;
  bool hasLoadFromStackSlot(const MachineInstr *MI,
                            const MachineMemOperand *&MMO,
                            int &FrameIndex) const;
  unsigned isStoreFromStackSlot(const MachineInstr *MI, int &FrameIndex) const;
  unsigned isStoreFromStackSlotPostFE(const MachineInstr *MI,
                                      int &FrameIndex) const;
  bool hasStoreFromStackSlot(const MachineInstr *MI,
                             const MachineMemOperand *&MMO,
                             int &FrameIndex) const;


#if 0
  void reMaterialize(MachineBasicBlock &MBB,
                     MachineBasicBlock::iterator MI,
                     unsigned DestReg, unsigned SubIdx,
                     const MachineInstr *Orig,
                     const TargetRegisterInfo *TRI) const;
  MachineInstr *duplicate(MachineInstr *Orig,
                          MachineFunction &MF) const;
#endif
  MachineInstr *
  convertToThreeAddress(MachineFunction::iterator &MFI,
                        MachineBasicBlock::iterator &MBBI,
                        LiveVariables *LV) const;
#if 0
  MachineInstr *commuteInstruction(MachineInstr *MI,
                                   bool NewMI = false) const;
  bool findCommutedOpIndices(MachineInstr *MI, unsigned &SrcOpIdx1,
                             unsigned &SrcOpIdx2) const;
  bool produceSameValue(const MachineInstr *MI0,
                        const MachineInstr *MI1) const;

#endif

  bool AnalyzeBranch(MachineBasicBlock &MBB, MachineBasicBlock *&TBB,
                     MachineBasicBlock *&FBB,
                     SmallVectorImpl<MachineOperand> &Cond,
                     bool AllowModify) const;

  unsigned RemoveBranch(MachineBasicBlock &MBB) const;

  unsigned
  InsertBranch(MachineBasicBlock &MBB, MachineBasicBlock *TBB,
               MachineBasicBlock *FBB,
               const SmallVectorImpl<MachineOperand> &Cond,
               DebugLoc DL) const;

  bool copyRegToReg(MachineBasicBlock &MBB,
                    MachineBasicBlock::iterator I,
                    unsigned DestReg, unsigned SrcReg,
                    const TargetRegisterClass *DestRC,
                    const TargetRegisterClass *SrcRC,
                    DebugLoc DL) const;
  virtual void copyPhysReg(MachineBasicBlock &MBB,
                           MachineBasicBlock::iterator MI, DebugLoc DL,
                           unsigned DestReg, unsigned SrcReg,
                           bool KillSrc) const;

  void storeRegToStackSlot(MachineBasicBlock &MBB,
                           MachineBasicBlock::iterator MI,
                           unsigned SrcReg, bool isKill, int FrameIndex,
                           const TargetRegisterClass *RC,
                           const TargetRegisterInfo *TRI) const;
  void loadRegFromStackSlot(MachineBasicBlock &MBB,
                            MachineBasicBlock::iterator MI,
                            unsigned DestReg, int FrameIndex,
                            const TargetRegisterClass *RC,
                            const TargetRegisterInfo *TRI) const;

protected:
  MachineInstr *foldMemoryOperandImpl(MachineFunction &MF,
                                      MachineInstr *MI,
                                      const SmallVectorImpl<unsigned> &Ops,
                                      int FrameIndex) const;
  MachineInstr *foldMemoryOperandImpl(MachineFunction &MF,
                                      MachineInstr *MI,
                                      const SmallVectorImpl<unsigned> &Ops,
                                      MachineInstr *LoadMI) const;
public:
  bool canFoldMemoryOperand(const MachineInstr *MI,
                            const SmallVectorImpl<unsigned> &Ops) const;
  bool unfoldMemoryOperand(MachineFunction &MF, MachineInstr *MI,
                           unsigned Reg, bool UnfoldLoad, bool UnfoldStore,
                           SmallVectorImpl<MachineInstr *> &NewMIs) const;
  bool unfoldMemoryOperand(SelectionDAG &DAG, SDNode *N,
                           SmallVectorImpl<SDNode *> &NewNodes) const;
  unsigned getOpcodeAfterMemoryUnfold(unsigned Opc,
                                      bool UnfoldLoad, bool UnfoldStore,
                                      unsigned *LoadRegIndex = 0) const;
  bool areLoadsFromSameBasePtr(SDNode *Load1, SDNode *Load2,
                               int64_t &Offset1, int64_t &Offset2) const;
  bool shouldScheduleLoadsNear(SDNode *Load1, SDNode *Load2,
                               int64_t Offset1, int64_t Offset2,
                               unsigned NumLoads) const;

  bool ReverseBranchCondition(SmallVectorImpl<MachineOperand> &Cond) const;
  void insertNoop(MachineBasicBlock &MBB,
                  MachineBasicBlock::iterator MI) const;
  bool isPredicated(const MachineInstr *MI) const;
#if 0
  bool isUnpredicatedTerminator(const MachineInstr *MI) const;
  bool PredicateInstruction(MachineInstr *MI,
                            const SmallVectorImpl<MachineOperand> &Pred) const;
#endif
  bool SubsumesPredicate(const SmallVectorImpl<MachineOperand> &Pred1,
                         const SmallVectorImpl<MachineOperand> &Pred2) const;
  bool DefinesPredicate(MachineInstr *MI,
                        std::vector<MachineOperand> &Pred) const;
  bool isPredicable(MachineInstr *MI) const;
  bool isSafeToMoveRegClassDefs(const TargetRegisterClass *RC) const;
  unsigned GetInstSizeInBytes(const MachineInstr *MI) const;
#if 0
  unsigned GetFunctionSizeInBytes(const MachineFunction &MF) const;
  unsigned getInlineAsmLength(const char *Str,
                              const MCAsmInfo &MAI) const;
#endif
  };

}

#endif // AMDILINSTRINFO_H_

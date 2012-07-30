//===-- AMDGPUInstrInfo.cpp - Base class for AMD GPU InstrInfo ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the implementation of the TargetInstrInfo class that is
// common to all AMD GPUs.
//
//===----------------------------------------------------------------------===//

#include "AMDGPUInstrInfo.h"
#include "AMDGPURegisterInfo.h"
#include "AMDGPUTargetMachine.h"
#include "AMDIL.h"
#include "AMDILUtilityFunctions.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"

#define GET_INSTRINFO_CTOR
#include "AMDGPUGenInstrInfo.inc"

using namespace llvm;

AMDGPUInstrInfo::AMDGPUInstrInfo(TargetMachine &tm)
  : AMDGPUGenInstrInfo(), RI(tm, *this), TM(tm) { }

const AMDGPURegisterInfo &AMDGPUInstrInfo::getRegisterInfo() const {
  return RI;
}

bool AMDGPUInstrInfo::isCoalescableExtInstr(const MachineInstr &MI,
                                           unsigned &SrcReg, unsigned &DstReg,
                                           unsigned &SubIdx) const {
// TODO: Implement this function
  return false;
}

unsigned AMDGPUInstrInfo::isLoadFromStackSlot(const MachineInstr *MI,
                                             int &FrameIndex) const {
// TODO: Implement this function
  return 0;
}

unsigned AMDGPUInstrInfo::isLoadFromStackSlotPostFE(const MachineInstr *MI,
                                                   int &FrameIndex) const {
// TODO: Implement this function
  return 0;
}

bool AMDGPUInstrInfo::hasLoadFromStackSlot(const MachineInstr *MI,
                                          const MachineMemOperand *&MMO,
                                          int &FrameIndex) const {
// TODO: Implement this function
  return false;
}
unsigned AMDGPUInstrInfo::isStoreFromStackSlot(const MachineInstr *MI,
                                              int &FrameIndex) const {
// TODO: Implement this function
  return 0;
}
unsigned AMDGPUInstrInfo::isStoreFromStackSlotPostFE(const MachineInstr *MI,
                                                    int &FrameIndex) const {
// TODO: Implement this function
  return 0;
}
bool AMDGPUInstrInfo::hasStoreFromStackSlot(const MachineInstr *MI,
                                           const MachineMemOperand *&MMO,
                                           int &FrameIndex) const {
// TODO: Implement this function
  return false;
}

MachineInstr *
AMDGPUInstrInfo::convertToThreeAddress(MachineFunction::iterator &MFI,
                                      MachineBasicBlock::iterator &MBBI,
                                      LiveVariables *LV) const {
// TODO: Implement this function
  return NULL;
}
bool AMDGPUInstrInfo::getNextBranchInstr(MachineBasicBlock::iterator &iter,
                                        MachineBasicBlock &MBB) const {
  while (iter != MBB.end()) {
    switch (iter->getOpcode()) {
    default:
      break;
      ExpandCaseToAllScalarTypes(AMDGPU::BRANCH_COND);
    case AMDGPU::BRANCH:
      return true;
    };
    ++iter;
  }
  return false;
}

bool AMDGPUInstrInfo::AnalyzeBranch(MachineBasicBlock &MBB,
                                   MachineBasicBlock *&TBB,
                                   MachineBasicBlock *&FBB,
                                   SmallVectorImpl<MachineOperand> &Cond,
                                   bool AllowModify) const {
  bool retVal = true;
  return retVal;
  MachineBasicBlock::iterator iter = MBB.begin();
  if (!getNextBranchInstr(iter, MBB)) {
    retVal = false;
  } else {
    MachineInstr *firstBranch = iter;
    if (!getNextBranchInstr(++iter, MBB)) {
      if (firstBranch->getOpcode() == AMDGPU::BRANCH) {
        TBB = firstBranch->getOperand(0).getMBB();
        firstBranch->eraseFromParent();
        retVal = false;
      } else {
        TBB = firstBranch->getOperand(0).getMBB();
        FBB = *(++MBB.succ_begin());
        if (FBB == TBB) {
          FBB = *(MBB.succ_begin());
        }
        Cond.push_back(firstBranch->getOperand(1));
        retVal = false;
      }
    } else {
      MachineInstr *secondBranch = iter;
      if (!getNextBranchInstr(++iter, MBB)) {
        if (secondBranch->getOpcode() == AMDGPU::BRANCH) {
          TBB = firstBranch->getOperand(0).getMBB();
          Cond.push_back(firstBranch->getOperand(1));
          FBB = secondBranch->getOperand(0).getMBB();
          secondBranch->eraseFromParent();
          retVal = false;
        } else {
          assert(0 && "Should not have two consecutive conditional branches");
        }
      } else {
        MBB.getParent()->viewCFG();
        assert(0 && "Should not have three branch instructions in"
               " a single basic block");
        retVal = false;
      }
    }
  }
  return retVal;
}

unsigned int AMDGPUInstrInfo::getBranchInstr(const MachineOperand &op) const {
  const MachineInstr *MI = op.getParent();
  
  switch (MI->getDesc().OpInfo->RegClass) {
  default: // FIXME: fallthrough??
  case AMDGPU::GPRI32RegClassID: return AMDGPU::BRANCH_COND_i32;
  case AMDGPU::GPRF32RegClassID: return AMDGPU::BRANCH_COND_f32;
  };
}

unsigned int
AMDGPUInstrInfo::InsertBranch(MachineBasicBlock &MBB,
                             MachineBasicBlock *TBB,
                             MachineBasicBlock *FBB,
                             const SmallVectorImpl<MachineOperand> &Cond,
                             DebugLoc DL) const
{
  assert(TBB && "InsertBranch must not be told to insert a fallthrough");
  for (unsigned int x = 0; x < Cond.size(); ++x) {
    Cond[x].getParent()->dump();
  }
  if (FBB == 0) {
    if (Cond.empty()) {
      BuildMI(&MBB, DL, get(AMDGPU::BRANCH)).addMBB(TBB);
    } else {
      BuildMI(&MBB, DL, get(getBranchInstr(Cond[0])))
        .addMBB(TBB).addReg(Cond[0].getReg());
    }
    return 1;
  } else {
    BuildMI(&MBB, DL, get(getBranchInstr(Cond[0])))
      .addMBB(TBB).addReg(Cond[0].getReg());
    BuildMI(&MBB, DL, get(AMDGPU::BRANCH)).addMBB(FBB);
  }
  assert(0 && "Inserting two branches not supported");
  return 0;
}

unsigned int AMDGPUInstrInfo::RemoveBranch(MachineBasicBlock &MBB) const {
  MachineBasicBlock::iterator I = MBB.end();
  if (I == MBB.begin()) {
    return 0;
  }
  --I;
  switch (I->getOpcode()) {
  default:
    return 0;
    ExpandCaseToAllScalarTypes(AMDGPU::BRANCH_COND);
  case AMDGPU::BRANCH:
    I->eraseFromParent();
    break;
  }
  I = MBB.end();
  
  if (I == MBB.begin()) {
    return 1;
  }
  --I;
  switch (I->getOpcode()) {
    // FIXME: only one case??
  default:
    return 1;
    ExpandCaseToAllScalarTypes(AMDGPU::BRANCH_COND);
    I->eraseFromParent();
    break;
  }
  return 2;
}

MachineBasicBlock::iterator skipFlowControl(MachineBasicBlock *MBB) {
  MachineBasicBlock::iterator tmp = MBB->end();
  if (!MBB->size()) {
    return MBB->end();
  }
  while (--tmp) {
    if (tmp->getOpcode() == AMDGPU::ENDLOOP
        || tmp->getOpcode() == AMDGPU::ENDIF
        || tmp->getOpcode() == AMDGPU::ELSE) {
      if (tmp == MBB->begin()) {
        return tmp;
      } else {
        continue;
      }
    }  else {
      return ++tmp;
    }
  }
  return MBB->end();
}

void
AMDGPUInstrInfo::storeRegToStackSlot(MachineBasicBlock &MBB,
                                    MachineBasicBlock::iterator MI,
                                    unsigned SrcReg, bool isKill,
                                    int FrameIndex,
                                    const TargetRegisterClass *RC,
                                    const TargetRegisterInfo *TRI) const {
  assert(!"Not Implemented");
}

void
AMDGPUInstrInfo::loadRegFromStackSlot(MachineBasicBlock &MBB,
                                     MachineBasicBlock::iterator MI,
                                     unsigned DestReg, int FrameIndex,
                                     const TargetRegisterClass *RC,
                                     const TargetRegisterInfo *TRI) const {
  assert(!"Not Implemented");
}

MachineInstr *
AMDGPUInstrInfo::foldMemoryOperandImpl(MachineFunction &MF,
                                      MachineInstr *MI,
                                      const SmallVectorImpl<unsigned> &Ops,
                                      int FrameIndex) const {
// TODO: Implement this function
  return 0;
}
MachineInstr*
AMDGPUInstrInfo::foldMemoryOperandImpl(MachineFunction &MF,
                                      MachineInstr *MI,
                                      const SmallVectorImpl<unsigned> &Ops,
                                      MachineInstr *LoadMI) const {
  // TODO: Implement this function
  return 0;
}
bool
AMDGPUInstrInfo::canFoldMemoryOperand(const MachineInstr *MI,
                                     const SmallVectorImpl<unsigned> &Ops) const
{
  // TODO: Implement this function
  return false;
}
bool
AMDGPUInstrInfo::unfoldMemoryOperand(MachineFunction &MF, MachineInstr *MI,
                                 unsigned Reg, bool UnfoldLoad,
                                 bool UnfoldStore,
                                 SmallVectorImpl<MachineInstr*> &NewMIs) const {
  // TODO: Implement this function
  return false;
}

bool
AMDGPUInstrInfo::unfoldMemoryOperand(SelectionDAG &DAG, SDNode *N,
                                    SmallVectorImpl<SDNode*> &NewNodes) const {
  // TODO: Implement this function
  return false;
}

unsigned
AMDGPUInstrInfo::getOpcodeAfterMemoryUnfold(unsigned Opc,
                                           bool UnfoldLoad, bool UnfoldStore,
                                           unsigned *LoadRegIndex) const {
  // TODO: Implement this function
  return 0;
}

bool AMDGPUInstrInfo::shouldScheduleLoadsNear(SDNode *Load1, SDNode *Load2,
                                             int64_t Offset1, int64_t Offset2,
                                             unsigned NumLoads) const {
  assert(Offset2 > Offset1
         && "Second offset should be larger than first offset!");
  // If we have less than 16 loads in a row, and the offsets are within 16,
  // then schedule together.
  // TODO: Make the loads schedule near if it fits in a cacheline
  return (NumLoads < 16 && (Offset2 - Offset1) < 16);
}

bool
AMDGPUInstrInfo::ReverseBranchCondition(SmallVectorImpl<MachineOperand> &Cond)
  const {
  // TODO: Implement this function
  return true;
}
void AMDGPUInstrInfo::insertNoop(MachineBasicBlock &MBB,
                                MachineBasicBlock::iterator MI) const {
  // TODO: Implement this function
}

bool AMDGPUInstrInfo::isPredicated(const MachineInstr *MI) const {
  // TODO: Implement this function
  return false;
}
bool
AMDGPUInstrInfo::SubsumesPredicate(const SmallVectorImpl<MachineOperand> &Pred1,
                                  const SmallVectorImpl<MachineOperand> &Pred2)
  const {
  // TODO: Implement this function
  return false;
}

bool AMDGPUInstrInfo::DefinesPredicate(MachineInstr *MI,
                                      std::vector<MachineOperand> &Pred) const {
  // TODO: Implement this function
  return false;
}

bool AMDGPUInstrInfo::isPredicable(MachineInstr *MI) const {
  // TODO: Implement this function
  return MI->getDesc().isPredicable();
}

bool
AMDGPUInstrInfo::isSafeToMoveRegClassDefs(const TargetRegisterClass *RC) const {
  // TODO: Implement this function
  return true;
}

MachineInstr * AMDGPUInstrInfo::convertToISA(MachineInstr & MI, MachineFunction &MF,
    DebugLoc DL) const
{
  MachineInstrBuilder newInstr;
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const AMDGPURegisterInfo & RI = getRegisterInfo();

  // Create the new instruction
  newInstr = BuildMI(MF, DL, TM.getInstrInfo()->get(MI.getOpcode()));

  for (unsigned i = 0; i < MI.getNumOperands(); i++) {
    MachineOperand &MO = MI.getOperand(i);
    // Convert dst regclass to one that is supported by the ISA
    if (MO.isReg() && MO.isDef()) {
      if (TargetRegisterInfo::isVirtualRegister(MO.getReg())) {
        const TargetRegisterClass * oldRegClass = MRI.getRegClass(MO.getReg());
        const TargetRegisterClass * newRegClass = RI.getISARegClass(oldRegClass);

        assert(newRegClass);

        MRI.setRegClass(MO.getReg(), newRegClass);
      }
    }
    // Add the operand to the new instruction
    newInstr.addOperand(MO);
  }

  return newInstr;
}

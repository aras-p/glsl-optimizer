//===-- SILowerFlowControl.cpp - Use predicates for flow control ----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This pass lowers the pseudo flow control instructions (SI_IF_NZ, ELSE, ENDIF)
// to predicated instructions.
//
// All flow control (except loops) is handled using predicated instructions and
// a predicate stack.  Each Scalar ALU controls the operations of 64 Vector
// ALUs.  The Scalar ALU can update the predicate for any of the Vector ALUs
// by writting to the 64-bit EXEC register (each bit corresponds to a
// single vector ALU).  Typically, for predicates, a vector ALU will write
// to its bit of the VCC register (like EXEC VCC is 64-bits, one for each
// Vector ALU) and then the ScalarALU will AND the VCC register with the
// EXEC to update the predicates.
//
// For example:
// %VCC = V_CMP_GT_F32 %VGPR1, %VGPR2
// SI_IF_NZ %VCC
//   %VGPR0 = V_ADD_F32 %VGPR0, %VGPR0
// ELSE
//   %VGPR0 = V_SUB_F32 %VGPR0, %VGPR0
// ENDIF
//
// becomes:
//
// %SGPR0 = S_MOV_B64 %EXEC          // Save the current exec mask
// %EXEC = S_AND_B64 %VCC, %EXEC     // Update the exec mask
// S_CBRANCH_EXECZ label0            // This instruction is an
//                                   // optimization which allows us to
//                                   // branch if all the bits of
//                                   // EXEC are zero.
// %VGPR0 = V_ADD_F32 %VGPR0, %VGPR0 // Do the IF block of the branch
//
// label0:
// %EXEC = S_NOT_B64 %EXEC            // Invert the exec mask for the
//                                    // Then block.
// %EXEC = S_AND_B64 %SGPR0, %EXEC
// S_BRANCH_EXECZ label1              // Use our branch optimization
//                                    // instruction again.
// %VGPR0 = V_SUB_F32 %VGPR0, %VGPR   // Do the THEN block
// label1:
// S_MOV_B64                          // Restore the old EXEC value
//===----------------------------------------------------------------------===//

#include "AMDGPU.h"
#include "SIInstrInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"

using namespace llvm;

namespace {

class SILowerFlowControlPass : public MachineFunctionPass {

private:
  static char ID;
  const TargetInstrInfo *TII;
  std::vector<unsigned> PredicateStack;
  std::vector<unsigned> UnusedRegisters;

  void pushExecMask(MachineBasicBlock &MBB, MachineBasicBlock::iterator I);
  void popExecMask(MachineBasicBlock &MBB, MachineBasicBlock::iterator I);

public:
  SILowerFlowControlPass(TargetMachine &tm) :
    MachineFunctionPass(ID), TII(tm.getInstrInfo()) { }

  virtual bool runOnMachineFunction(MachineFunction &MF);

  const char *getPassName() const {
    return "SI Lower flow control instructions";
  }

};

} // End anonymous namespace

char SILowerFlowControlPass::ID = 0;

FunctionPass *llvm::createSILowerFlowControlPass(TargetMachine &tm) {
  return new SILowerFlowControlPass(tm);
}

bool SILowerFlowControlPass::runOnMachineFunction(MachineFunction &MF) {

  // Find all the unused registers that can be used for the predicate stack.
  for (TargetRegisterClass::iterator S = AMDGPU::SReg_64RegClass.begin(),
                                     I = AMDGPU::SReg_64RegClass.end();
                                     I != S; --I) {
    unsigned Reg = *I;
    if (!MF.getRegInfo().isPhysRegOrOverlapUsed(Reg)) {
      UnusedRegisters.push_back(Reg);
    }
  }

  for (MachineFunction::iterator BB = MF.begin(), BB_E = MF.end();
                                                  BB != BB_E; ++BB) {
    MachineBasicBlock &MBB = *BB;
    for (MachineBasicBlock::iterator I = MBB.begin(), Next = llvm::next(I);
                               I != MBB.end(); I = Next, Next = llvm::next(I)) {
      MachineInstr &MI = *I;
      switch (MI.getOpcode()) {
        default: break;
        case AMDGPU::SI_IF_NZ:
          pushExecMask(MBB, I);
          BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(AMDGPU::S_AND_B64),
                  AMDGPU::EXEC)
                  .addOperand(MI.getOperand(0)) // VCC
                  .addReg(AMDGPU::EXEC);
          MI.eraseFromParent();
          break;
        case AMDGPU::ELSE:
          BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(AMDGPU::S_NOT_B64),
                  AMDGPU::EXEC)
                  .addReg(AMDGPU::EXEC);
          BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(AMDGPU::S_AND_B64),
                  AMDGPU::EXEC)
                  .addReg(PredicateStack.back())
                  .addReg(AMDGPU::EXEC);
          MI.eraseFromParent();
          break;
        case AMDGPU::ENDIF:
          popExecMask(MBB, I);
          MI.eraseFromParent();
          break;
      }
    }
  }
  return false;
}

void SILowerFlowControlPass::pushExecMask(MachineBasicBlock &MBB,
                                          MachineBasicBlock::iterator I) {

  assert(!UnusedRegisters.empty() && "Ran out of registers for predicate stack");
  unsigned StackReg = UnusedRegisters.back();
  UnusedRegisters.pop_back();
  PredicateStack.push_back(StackReg);
  BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(AMDGPU::S_MOV_B64),
          StackReg)
          .addReg(AMDGPU::EXEC);
}

void SILowerFlowControlPass::popExecMask(MachineBasicBlock &MBB,
                                        MachineBasicBlock::iterator I) {
  unsigned StackReg = PredicateStack.back();
  PredicateStack.pop_back();
  UnusedRegisters.push_back(StackReg);
  BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(AMDGPU::S_MOV_B64),
          AMDGPU::EXEC)
          .addReg(StackReg);
}

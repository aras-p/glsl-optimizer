//===--- AMDILLiteralManager.cpp - AMDIL Literal Manager Pass --*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//

#define DEBUG_TYPE "literal_manager"

#include "AMDIL.h"

#include "AMDILAlgorithms.tpp"
#include "AMDILKernelManager.h"
#include "AMDILMachineFunctionInfo.h"
#include "AMDILSubtarget.h"
#include "AMDILTargetMachine.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/Support/Debug.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;


// AMDIL Literal Manager traverses through all of the LOADCONST instructions and
// converts them from an immediate value to the literal index. The literal index
// is valid IL, but the immediate values are not. The Immediate values must be
// aggregated and declared for clarity and to reduce the number of literals that
// are used. It is also illegal to declare the same literal twice, so this keeps
// that from occuring.

namespace {
  class AMDILLiteralManager : public MachineFunctionPass {
  public:
    static char ID;
    AMDILLiteralManager(TargetMachine &tm AMDIL_OPT_LEVEL_DECL);
    virtual const char *getPassName() const;

    bool runOnMachineFunction(MachineFunction &MF);
  private:
    bool trackLiterals(MachineBasicBlock::iterator *bbb);
    TargetMachine &TM;
    const AMDILSubtarget *mSTM;
    AMDILKernelManager *mKM;
    AMDILMachineFunctionInfo *mMFI;
    int32_t mLitIdx;
    bool mChanged;
  };
  char AMDILLiteralManager::ID = 0;
}

namespace llvm {
  FunctionPass *
  createAMDILLiteralManager(TargetMachine &tm AMDIL_OPT_LEVEL_DECL) {
    return new AMDILLiteralManager(tm AMDIL_OPT_LEVEL_VAR);
  }
  
}

AMDILLiteralManager::AMDILLiteralManager(TargetMachine &tm
                                         AMDIL_OPT_LEVEL_DECL)
  : MachineFunctionPass(ID),
    TM(tm) {
}

bool AMDILLiteralManager::runOnMachineFunction(MachineFunction &MF) {
  mChanged = false;
  mMFI = MF.getInfo<AMDILMachineFunctionInfo>();
  const AMDILTargetMachine *amdtm =
    reinterpret_cast<const AMDILTargetMachine *>(&TM);
  mSTM = dynamic_cast<const AMDILSubtarget *>(amdtm->getSubtargetImpl());
  mKM = const_cast<AMDILKernelManager *>(mSTM->getKernelManager());
  safeNestedForEach(MF.begin(), MF.end(), MF.begin()->begin(),
      std::bind1st(std::mem_fun(&AMDILLiteralManager::trackLiterals), this));
  return mChanged;
}

bool AMDILLiteralManager::trackLiterals(MachineBasicBlock::iterator *bbb) {
  MachineInstr *MI = *bbb;
  uint32_t Opcode = MI->getOpcode();
  switch(Opcode) {
  default:
    return false;
  case AMDIL::LOADCONST_i8:
  case AMDIL::LOADCONST_i16:
  case AMDIL::LOADCONST_i32:
  case AMDIL::LOADCONST_i64:
  case AMDIL::LOADCONST_f32:
  case AMDIL::LOADCONST_f64:
    break;
  };
  MachineOperand &dstOp = MI->getOperand(0);
  MachineOperand &litOp = MI->getOperand(1);
  if (!litOp.isImm() && !litOp.isFPImm()) {
    return false;
  }
  if (!dstOp.isReg()) {
    return false;
  }
  // Change the literal to the correct index for each literal that is found.
  if (litOp.isImm()) {
    int64_t immVal = litOp.getImm();
    uint32_t idx = MI->getOpcode() == AMDIL::LOADCONST_i64 
                     ? mMFI->addi64Literal(immVal)
                     : mMFI->addi32Literal(static_cast<int>(immVal), Opcode);
    litOp.ChangeToImmediate(idx);
    return false;
  } 

  if (litOp.isFPImm()) {
    const ConstantFP *fpVal = litOp.getFPImm();
    uint32_t idx = MI->getOpcode() == AMDIL::LOADCONST_f64
                     ? mMFI->addf64Literal(fpVal)
                     : mMFI->addf32Literal(fpVal);
    litOp.ChangeToImmediate(idx);
    return false;
  }

  return false;
}

const char* AMDILLiteralManager::getPassName() const {
    return "AMDIL Constant Propagation";
}



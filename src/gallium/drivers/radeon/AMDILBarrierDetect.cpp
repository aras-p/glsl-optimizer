//===----- AMDILBarrierDetect.cpp - Barrier Detect pass -*- C++ -*- ------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//

#define DEBUG_TYPE "BarrierDetect"
#ifdef DEBUG
#define DEBUGME (DebugFlag && isCurrentDebugType(DEBUG_TYPE))
#else
#define DEBUGME 0
#endif
#include "AMDILAlgorithms.tpp"
#include "AMDILCompilerWarnings.h"
#include "AMDILDevices.h"
#include "AMDILMachineFunctionInfo.h"
#include "AMDILSubtarget.h"
#include "AMDILTargetMachine.h"
#include "llvm/BasicBlock.h"
#include "llvm/CodeGen/MachineFunctionAnalysis.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;

// The barrier detect pass determines if a barrier has been duplicated in the
// source program which can cause undefined behaviour if more than a single
// wavefront is executed in a group. This is because LLVM does not have an
// execution barrier and if this barrier function gets duplicated, undefined
// behaviour can occur. In order to work around this, we detect the duplicated
// barrier and then make the work-group execute in a single wavefront mode,
// essentially making the barrier a no-op.

namespace
{
  class LLVM_LIBRARY_VISIBILITY AMDILBarrierDetect : public FunctionPass
  {
    TargetMachine &TM;
    static char ID;
  public:
    AMDILBarrierDetect(TargetMachine &TM AMDIL_OPT_LEVEL_DECL);
    ~AMDILBarrierDetect();
    const char *getPassName() const;
    bool runOnFunction(Function &F);
    bool doInitialization(Module &M);
    bool doFinalization(Module &M);
    void getAnalysisUsage(AnalysisUsage &AU) const;
  private:
    bool detectBarrier(BasicBlock::iterator *BBI);
    bool detectMemFence(BasicBlock::iterator *BBI);
    bool mChanged;
    SmallVector<int64_t, DEFAULT_VEC_SLOTS> bVecMap;
    const AMDILSubtarget *mStm;

    // Constants used to define memory type.
    static const unsigned int LOCAL_MEM_FENCE = 1<<0;
    static const unsigned int GLOBAL_MEM_FENCE = 1<<1;
    static const unsigned int REGION_MEM_FENCE = 1<<2;
  };
  char AMDILBarrierDetect::ID = 0;
} // anonymouse namespace

namespace llvm
{
  FunctionPass *
  createAMDILBarrierDetect(TargetMachine &TM AMDIL_OPT_LEVEL_DECL)
  {
    return new AMDILBarrierDetect(TM  AMDIL_OPT_LEVEL_VAR);
  }
} // llvm namespace

AMDILBarrierDetect::AMDILBarrierDetect(TargetMachine &TM
                                       AMDIL_OPT_LEVEL_DECL)
  :
  FunctionPass(ID),
  TM(TM)
{
}

AMDILBarrierDetect::~AMDILBarrierDetect()
{
}

bool AMDILBarrierDetect::detectBarrier(BasicBlock::iterator *BBI)
{
  SmallVector<int64_t, DEFAULT_VEC_SLOTS>::iterator bIter;
  int64_t bID;
  Instruction *inst = (*BBI);
  CallInst *CI = dyn_cast<CallInst>(inst);

  if (!CI || !CI->getNumOperands()) {
    return false;
  }
  const Value *funcVal = CI->getOperand(CI->getNumOperands() - 1);
  if (funcVal && strncmp(funcVal->getName().data(), "__amd_barrier", 13)) {
    return false;
  }

  if (inst->getNumOperands() >= 3) {
    const Value *V = inst->getOperand(0);
    const ConstantInt *Cint = dyn_cast<ConstantInt>(V);
    bID = Cint->getSExtValue();
    bIter = std::find(bVecMap.begin(), bVecMap.end(), bID);
    if (bIter == bVecMap.end()) {
      bVecMap.push_back(bID);
    } else {
      if (mStm->device()->isSupported(AMDILDeviceInfo::BarrierDetect)) {
        AMDILMachineFunctionInfo *MFI =
          getAnalysis<MachineFunctionAnalysis>().getMF()
          .getInfo<AMDILMachineFunctionInfo>();
        MFI->addMetadata(";limitgroupsize");
        MFI->addErrorMsg(amd::CompilerWarningMessage[BAD_BARRIER_OPT]);
      }
    }
  }
  if (mStm->device()->getGeneration() == AMDILDeviceInfo::HD4XXX) {
    AMDILMachineFunctionInfo *MFI =
      getAnalysis<MachineFunctionAnalysis>().getMF()
          .getInfo<AMDILMachineFunctionInfo>();
    MFI->addErrorMsg(amd::CompilerWarningMessage[LIMIT_BARRIER]);
    MFI->addMetadata(";limitgroupsize");
    MFI->setUsesLocal();
  }
  const Value *V = inst->getOperand(inst->getNumOperands()-2);
  const ConstantInt *Cint = dyn_cast<ConstantInt>(V);
  Function *iF = dyn_cast<Function>(inst->getOperand(inst->getNumOperands()-1));
  Module *M = iF->getParent();
  bID = Cint->getSExtValue();
  if (bID > 0) {
    const char *name = "barrier";
    if (bID == GLOBAL_MEM_FENCE) {
      name = "barrierGlobal";
    } else if (bID == LOCAL_MEM_FENCE
        && mStm->device()->usesHardware(AMDILDeviceInfo::LocalMem)) {
      name = "barrierLocal";
    } else if (bID == REGION_MEM_FENCE
               && mStm->device()->usesHardware(AMDILDeviceInfo::RegionMem)) {
      name = "barrierRegion";
    }
    Function *nF =
      dyn_cast<Function>(M->getOrInsertFunction(name, iF->getFunctionType()));
    inst->setOperand(inst->getNumOperands()-1, nF);
    return false;
  }

  return false;
}

bool AMDILBarrierDetect::detectMemFence(BasicBlock::iterator *BBI)
{
  int64_t bID;
  Instruction *inst = (*BBI);
  CallInst *CI = dyn_cast<CallInst>(inst);

  if (!CI || CI->getNumOperands() != 2) {
    return false;
  }

  const Value *V = inst->getOperand(inst->getNumOperands()-2);
  const ConstantInt *Cint = dyn_cast<ConstantInt>(V);
  Function *iF = dyn_cast<Function>(inst->getOperand(inst->getNumOperands()-1));

  const char *fence_local_name;
  const char *fence_global_name;
  const char *fence_region_name;
  const char* fence_name = "mem_fence";
  if (!iF) {
    return false;
  }

  if (strncmp(iF->getName().data(), "mem_fence", 9) == 0) {
    fence_local_name = "mem_fence_local";
    fence_global_name = "mem_fence_global";
    fence_region_name = "mem_fence_region";
  } else if (strncmp(iF->getName().data(), "read_mem_fence", 14) == 0) {
    fence_local_name = "read_mem_fence_local";
    fence_global_name = "read_mem_fence_global";
    fence_region_name = "read_mem_fence_region";
  } else if (strncmp(iF->getName().data(), "write_mem_fence", 15) == 0) {
    fence_local_name = "write_mem_fence_local";
    fence_global_name = "write_mem_fence_global";
    fence_region_name = "write_mem_fence_region";
  } else {
    return false;
  }

  Module *M = iF->getParent();
  bID = Cint->getSExtValue();
  if (bID > 0) {
    const char *name = fence_name;
    if (bID == GLOBAL_MEM_FENCE) {
      name = fence_global_name;
    } else if (bID == LOCAL_MEM_FENCE
        && mStm->device()->usesHardware(AMDILDeviceInfo::LocalMem)) {
      name = fence_local_name;
    } else if (bID == REGION_MEM_FENCE
               && mStm->device()->usesHardware(AMDILDeviceInfo::RegionMem)) {
      name = fence_region_name;
    }
    Function *nF =
      dyn_cast<Function>(M->getOrInsertFunction(name, iF->getFunctionType()));
    inst->setOperand(inst->getNumOperands()-1, nF);
    return false;
  }

  return false;

}

bool AMDILBarrierDetect::runOnFunction(Function &MF)
{
  mChanged = false;
  bVecMap.clear();
  mStm = &TM.getSubtarget<AMDILSubtarget>();
  Function *F = &MF;
  safeNestedForEach(F->begin(), F->end(), F->begin()->begin(),
      std::bind1st(
        std::mem_fun(
          &AMDILBarrierDetect::detectBarrier), this));
  safeNestedForEach(F->begin(), F->end(), F->begin()->begin(),
      std::bind1st(
        std::mem_fun(
          &AMDILBarrierDetect::detectMemFence), this));
  return mChanged;
}

const char* AMDILBarrierDetect::getPassName() const
{
  return "AMDIL Barrier Detect Pass";
}

bool AMDILBarrierDetect::doInitialization(Module &M)
{
  return false;
}

bool AMDILBarrierDetect::doFinalization(Module &M)
{
  return false;
}

void AMDILBarrierDetect::getAnalysisUsage(AnalysisUsage &AU) const
{
  AU.addRequired<MachineFunctionAnalysis>();
  FunctionPass::getAnalysisUsage(AU);
  AU.setPreservesAll();
}

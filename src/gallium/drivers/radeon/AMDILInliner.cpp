//===-- AMDILInliner.cpp - TODO: Add brief description -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//

#define DEBUG_TYPE "amdilinline"
#include "AMDIL.h"
#include "AMDILCompilerErrors.h"
#include "AMDILMachineFunctionInfo.h"
#include "AMDILSubtarget.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionAnalysis.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/Local.h"

using namespace llvm;

namespace
{
  class LLVM_LIBRARY_VISIBILITY AMDILInlinePass: public FunctionPass

  {
    public:
      TargetMachine &TM;
      static char ID;
      AMDILInlinePass(TargetMachine &tm AMDIL_OPT_LEVEL_DECL);
      ~AMDILInlinePass();
      virtual const char* getPassName() const;
      virtual bool runOnFunction(Function &F);
      bool doInitialization(Module &M);
      bool doFinalization(Module &M);
      virtual void getAnalysisUsage(AnalysisUsage &AU) const;
    private:
      typedef DenseMap<const ArrayType*, SmallVector<AllocaInst*,
              DEFAULT_VEC_SLOTS> > InlinedArrayAllocasTy;
      bool
        AMDILInlineCallIfPossible(CallSite CS,
            const TargetData *TD,
            InlinedArrayAllocasTy &InlinedArrayAllocas);

      CodeGenOpt::Level OptLevel;
  };
  char AMDILInlinePass::ID = 0;
} // anonymouse namespace


namespace llvm
{
  FunctionPass*
    createAMDILInlinePass(TargetMachine &tm AMDIL_OPT_LEVEL_DECL)
    {
      return new AMDILInlinePass(tm AMDIL_OPT_LEVEL_VAR);
    }
} // llvm namespace

  AMDILInlinePass::AMDILInlinePass(TargetMachine &tm AMDIL_OPT_LEVEL_DECL)
: FunctionPass(ID), TM(tm)
{
  OptLevel = tm.getOptLevel();
}
AMDILInlinePass::~AMDILInlinePass()
{
}


bool
AMDILInlinePass::AMDILInlineCallIfPossible(CallSite CS,
    const TargetData *TD, InlinedArrayAllocasTy &InlinedArrayAllocas) {
  Function *Callee = CS.getCalledFunction();
  Function *Caller = CS.getCaller();

  // Try to inline the function.  Get the list of static allocas that were
  // inlined.
  SmallVector<AllocaInst*, 16> StaticAllocas;
  InlineFunctionInfo IFI;
  if (!InlineFunction(CS, IFI))
    return false;
  DEBUG(errs() << "<amdilinline> function " << Caller->getName()
      << ": inlined call to "<< Callee->getName() << "\n");

  // If the inlined function had a higher stack protection level than the
  // calling function, then bump up the caller's stack protection level.
  if (Callee->hasFnAttr(Attribute::StackProtectReq))
    Caller->addFnAttr(Attribute::StackProtectReq);
  else if (Callee->hasFnAttr(Attribute::StackProtect) &&
      !Caller->hasFnAttr(Attribute::StackProtectReq))
    Caller->addFnAttr(Attribute::StackProtect);


  // Look at all of the allocas that we inlined through this call site.  If we
  // have already inlined other allocas through other calls into this function,
  // then we know that they have disjoint lifetimes and that we can merge them.
  //
  // There are many heuristics possible for merging these allocas, and the
  // different options have different tradeoffs.  One thing that we *really*
  // don't want to hurt is SRoA: once inlining happens, often allocas are no
  // longer address taken and so they can be promoted.
  //
  // Our "solution" for that is to only merge allocas whose outermost type is an
  // array type.  These are usually not promoted because someone is using a
  // variable index into them.  These are also often the most important ones to
  // merge.
  //
  // A better solution would be to have real memory lifetime markers in the IR
  // and not have the inliner do any merging of allocas at all.  This would
  // allow the backend to do proper stack slot coloring of all allocas that
  // *actually make it to the backend*, which is really what we want.
  //
  // Because we don't have this information, we do this simple and useful hack.
  //
  SmallPtrSet<AllocaInst*, 16> UsedAllocas;

  // Loop over all the allocas we have so far and see if they can be merged with
  // a previously inlined alloca.  If not, remember that we had it.

  for (unsigned AllocaNo = 0,
      e = IFI.StaticAllocas.size();
      AllocaNo != e; ++AllocaNo) {

    AllocaInst *AI = IFI.StaticAllocas[AllocaNo];

    // Don't bother trying to merge array allocations (they will usually be
    // canonicalized to be an allocation *of* an array), or allocations whose
    // type is not itself an array (because we're afraid of pessimizing SRoA).
    const ArrayType *ATy = dyn_cast<ArrayType>(AI->getAllocatedType());
    if (ATy == 0 || AI->isArrayAllocation())
      continue;

    // Get the list of all available allocas for this array type.
    SmallVector<AllocaInst*, DEFAULT_VEC_SLOTS> &AllocasForType
      = InlinedArrayAllocas[ATy];

    // Loop over the allocas in AllocasForType to see if we can reuse one.  Note
    // that we have to be careful not to reuse the same "available" alloca for
    // multiple different allocas that we just inlined, we use the 'UsedAllocas'
    // set to keep track of which "available" allocas are being used by this
    // function.  Also, AllocasForType can be empty of course!
    bool MergedAwayAlloca = false;
    for (unsigned i = 0, e = AllocasForType.size(); i != e; ++i) {
      AllocaInst *AvailableAlloca = AllocasForType[i];

      // The available alloca has to be in the right function, not in some other
      // function in this SCC.
      if (AvailableAlloca->getParent() != AI->getParent())
        continue;

      // If the inlined function already uses this alloca then we can't reuse
      // it.
      if (!UsedAllocas.insert(AvailableAlloca))
        continue;

      // Otherwise, we *can* reuse it, RAUW AI into AvailableAlloca and declare
      // success!
      DEBUG(errs() << "    ***MERGED ALLOCA: " << *AI);

      AI->replaceAllUsesWith(AvailableAlloca);
      AI->eraseFromParent();
      MergedAwayAlloca = true;
      break;
    }

    // If we already nuked the alloca, we're done with it.
    if (MergedAwayAlloca)
      continue;

    // If we were unable to merge away the alloca either because there are no
    // allocas of the right type available or because we reused them all
    // already, remember that this alloca came from an inlined function and mark
    // it used so we don't reuse it for other allocas from this inline
    // operation.
    AllocasForType.push_back(AI);
    UsedAllocas.insert(AI);
  }

  return true;
}

  bool
AMDILInlinePass::runOnFunction(Function &MF)
{
  Function *F = &MF;
  const AMDILSubtarget &STM = TM.getSubtarget<AMDILSubtarget>();
  if (STM.device()->isSupported(AMDILDeviceInfo::NoInline)) {
    return false;
  }
  const TargetData *TD = getAnalysisIfAvailable<TargetData>();
  SmallVector<CallSite, 16> CallSites;
  for (Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB) {
    for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E; ++I) {
      CallSite CS = CallSite(cast<Value>(I));
      // If this isn't a call, or it is a call to an intrinsic, it can
      // never be inlined.
      if (CS.getInstruction() == 0 || isa<IntrinsicInst>(I))
        continue;

      // If this is a direct call to an external function, we can never inline
      // it.  If it is an indirect call, inlining may resolve it to be a
      // direct call, so we keep it.
      if (CS.getCalledFunction() && CS.getCalledFunction()->isDeclaration())
        continue;

      // We don't want to inline if we are recursive.
      if (CS.getCalledFunction() && CS.getCalledFunction()->getName() == MF.getName()) {
        AMDILMachineFunctionInfo *MFI =
          getAnalysis<MachineFunctionAnalysis>().getMF()
          .getInfo<AMDILMachineFunctionInfo>();
        MFI->addErrorMsg(amd::CompilerErrorMessage[RECURSIVE_FUNCTION]);
        continue;
      }

      CallSites.push_back(CS);
    }
  }

  InlinedArrayAllocasTy InlinedArrayAllocas;
  bool Changed = false;
  for (unsigned CSi = 0; CSi != CallSites.size(); ++CSi) {
    CallSite CS = CallSites[CSi];

    Function *Callee = CS.getCalledFunction();

    // We can only inline direct calls to non-declarations.
    if (Callee == 0 || Callee->isDeclaration()) continue;

    // Attempt to inline the function...
    if (!AMDILInlineCallIfPossible(CS, TD, InlinedArrayAllocas))
      continue;
    Changed = true;
  }
  return Changed;
}

const char*
AMDILInlinePass::getPassName() const
{
  return "AMDIL Inline Function Pass";
}
  bool
AMDILInlinePass::doInitialization(Module &M)
{
  return false;
}

  bool
AMDILInlinePass::doFinalization(Module &M)
{
  return false;
}

void
AMDILInlinePass::getAnalysisUsage(AnalysisUsage &AU) const
{
  AU.addRequired<MachineFunctionAnalysis>();
  FunctionPass::getAnalysisUsage(AU);
  AU.setPreservesAll();
}

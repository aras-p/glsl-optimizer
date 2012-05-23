//===-- AMDILTargetMachine.cpp - Define TargetMachine for AMDIL -----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//
//
//===----------------------------------------------------------------------===//

#include "AMDILTargetMachine.h"
#include "AMDGPUTargetMachine.h"
#include "AMDILDevices.h"
#include "AMDILFrameLowering.h"
#include "llvm/ADT/OwningPtr.h"
#include "llvm/CodeGen/MachineFunctionAnalysis.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/SchedulerRegistry.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Transforms/Scalar.h"

using namespace llvm;

extern "C" void LLVMInitializeAMDILTarget() {
  // Register the target
  RegisterTargetMachine<AMDILTargetMachine> X(TheAMDILTarget);
  RegisterTargetMachine<AMDGPUTargetMachine> Y(TheAMDGPUTarget);
}

/// AMDILTargetMachine ctor -
///
AMDILTargetMachine::AMDILTargetMachine(const Target &T,
    StringRef TT, StringRef CPU, StringRef FS,
    TargetOptions Options,
    Reloc::Model RM, CodeModel::Model CM,
    CodeGenOpt::Level OL
)
:
  LLVMTargetMachine(T, TT, CPU, FS, Options, RM, CM, OL),
  Subtarget(TT, CPU, FS),
  DataLayout(Subtarget.getDataLayout()),
  FrameLowering(TargetFrameLowering::StackGrowsUp,
      Subtarget.device()->getStackAlignment(), 0),
  InstrInfo(*this), //JITInfo(*this),
  TLInfo(*this), 
  IntrinsicInfo(this)
{
  setAsmVerbosityDefault(true);
  setMCUseLoc(false);
}

AMDILTargetLowering*
AMDILTargetMachine::getTargetLowering() const
{
  return const_cast<AMDILTargetLowering*>(&TLInfo);
}

const AMDILInstrInfo*
AMDILTargetMachine::getInstrInfo() const
{
  return &InstrInfo;
}
const AMDILFrameLowering*
AMDILTargetMachine::getFrameLowering() const
{
  return &FrameLowering;
}

const AMDILSubtarget*
AMDILTargetMachine::getSubtargetImpl() const
{
  return &Subtarget;
}

const AMDILRegisterInfo*
AMDILTargetMachine::getRegisterInfo() const
{
  return &InstrInfo.getRegisterInfo();
}

const TargetData*
AMDILTargetMachine::getTargetData() const
{
  return &DataLayout;
}

const AMDILIntrinsicInfo*
AMDILTargetMachine::getIntrinsicInfo() const
{
  return &IntrinsicInfo;
}

  void
AMDILTargetMachine::dump(llvm::raw_ostream &O)
{
  if (!mDebugMode) {
    return;
  }
  O << ";AMDIL Target Machine State Dump: \n";
}

  void
AMDILTargetMachine::setDebug(bool debugMode)
{
  mDebugMode = debugMode;
}

bool
AMDILTargetMachine::getDebug() const
{
  return mDebugMode;
}

namespace {
class AMDILPassConfig : public TargetPassConfig {

public:
  AMDILPassConfig(AMDILTargetMachine *TM, PassManagerBase &PM)
    : TargetPassConfig(TM, PM) {}

  AMDILTargetMachine &getAMDILTargetMachine() const {
    return getTM<AMDILTargetMachine>();
  }

  virtual bool addPreISel();
  virtual bool addInstSelector();
  virtual bool addPreRegAlloc();
  virtual bool addPostRegAlloc();
  virtual bool addPreEmitPass();
};
} // End of anonymous namespace

TargetPassConfig *AMDILTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new AMDILPassConfig(this, PM);
}

bool AMDILPassConfig::addPreISel()
{
  return false;
}

bool AMDILPassConfig::addInstSelector()
{
  PM->add(createAMDILPeepholeOpt(*TM));
  PM->add(createAMDILISelDag(getAMDILTargetMachine()));
  return false;
}

bool AMDILPassConfig::addPreRegAlloc()
{
  // If debugging, reduce code motion. Use less aggressive pre-RA scheduler
  if (TM->getOptLevel() == CodeGenOpt::None) {
    llvm::RegisterScheduler::setDefault(&llvm::createSourceListDAGScheduler);
  }
  return false;
}

bool AMDILPassConfig::addPostRegAlloc() {
  return false;  // -print-machineinstr should print after this.
}

/// addPreEmitPass - This pass may be implemented by targets that want to run
/// passes immediately before machine code is emitted.  This should return
/// true if -print-machineinstrs should print out the code after the passes.
bool AMDILPassConfig::addPreEmitPass()
{
  PM->add(createAMDILCFGPreparationPass(*TM));
  PM->add(createAMDILCFGStructurizerPass(*TM));
  return true;
}


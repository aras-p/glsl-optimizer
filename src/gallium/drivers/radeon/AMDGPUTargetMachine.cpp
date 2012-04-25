//===-- AMDGPUTargetMachine.cpp - TODO: Add brief description -------===//
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

#include "AMDGPUTargetMachine.h"
#include "AMDGPU.h"
#include "AMDILGlobalManager.h"
#include "AMDILKernelManager.h"
#include "AMDILTargetMachine.h"
#include "R600ISelLowering.h"
#include "R600InstrInfo.h"
#include "R600KernelParameters.h"
#include "SIISelLowering.h"
#include "SIInstrInfo.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/CodeGen/MachineFunctionAnalysis.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/PassManager.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/Scalar.h"

using namespace llvm;

AMDGPUTargetMachine::AMDGPUTargetMachine(const Target &T, StringRef TT,
    StringRef CPU, StringRef FS,
  TargetOptions Options,
  Reloc::Model RM, CodeModel::Model CM,
  CodeGenOpt::Level OptLevel
)
:
  AMDILTargetMachine(T, TT, CPU, FS, Options, RM, CM, OptLevel),
  Subtarget(TT, CPU, FS),
  mGM(new AMDILGlobalManager(0 /* Debug mode */)),
  mKM(new AMDILKernelManager(this, mGM)),
  mDump(false)

{
  /* XXX: Add these two initializations to fix a segfault, not sure if this
   * is correct.  These are normally initialized in the AsmPrinter, but AMDGPU
   * does not use the asm printer */
  Subtarget.setGlobalManager(mGM);
  Subtarget.setKernelManager(mKM);
  /* TLInfo uses InstrInfo so it must be initialized after. */
  if (Subtarget.device()->getGeneration() <= AMDILDeviceInfo::HD6XXX) {
    InstrInfo = new R600InstrInfo(*this);
    TLInfo = new R600TargetLowering(*this);
  } else {
    InstrInfo = new SIInstrInfo(*this);
    TLInfo = new SITargetLowering(*this);
  }
}

AMDGPUTargetMachine::~AMDGPUTargetMachine()
{
    delete mGM;
    delete mKM;
}

bool AMDGPUTargetMachine::addPassesToEmitFile(PassManagerBase &PM,
                                              formatted_raw_ostream &Out,
                                              CodeGenFileType FileType,
                                              bool DisableVerify) {
  /* XXX: Hack here addPassesToEmitFile will fail, but this is Ok since we are
   * only using it to access addPassesToGenerateCode() */
  bool fail = LLVMTargetMachine::addPassesToEmitFile(PM, Out, FileType,
                                                     DisableVerify);
  assert(fail);

  const AMDILSubtarget &STM = getSubtarget<AMDILSubtarget>();
  std::string gpu = STM.getDeviceName();
  if (gpu == "SI") {
    PM.add(createSICodeEmitterPass(Out));
  } else if (Subtarget.device()->getGeneration() <= AMDILDeviceInfo::HD6XXX) {
    PM.add(createR600CodeEmitterPass(Out));
  } else {
    abort();
    return true;
  }
  PM.add(createGCInfoDeleter());

  return false;
}

namespace {
class AMDGPUPassConfig : public TargetPassConfig {
public:
  AMDGPUPassConfig(AMDGPUTargetMachine *TM, PassManagerBase &PM)
    : TargetPassConfig(TM, PM) {}

  AMDGPUTargetMachine &getAMDGPUTargetMachine() const {
    return getTM<AMDGPUTargetMachine>();
  }

  virtual bool addPreISel();
  virtual bool addInstSelector();
  virtual bool addPreRegAlloc();
  virtual bool addPostRegAlloc();
  virtual bool addPreSched2();
  virtual bool addPreEmitPass();
};
} // End of anonymous namespace

TargetPassConfig *AMDGPUTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new AMDGPUPassConfig(this, PM);
}

bool
AMDGPUPassConfig::addPreISel()
{
  const AMDILSubtarget &ST = TM->getSubtarget<AMDILSubtarget>();
  if (ST.device()->getGeneration() <= AMDILDeviceInfo::HD6XXX) {
    PM.add(createR600KernelParametersPass(
                     getAMDGPUTargetMachine().getTargetData()));
  }
  return false;
}

bool AMDGPUPassConfig::addInstSelector() {
  const AMDILSubtarget &ST = TM->getSubtarget<AMDILSubtarget>();
  if (ST.device()->getGeneration() == AMDILDeviceInfo::HD7XXX) {
    PM.add(createSIInitMachineFunctionInfoPass(*TM));
  }
  PM.add(createAMDILBarrierDetect(*TM));
  PM.add(createAMDILPrintfConvert(*TM));
  PM.add(createAMDILInlinePass(*TM));
  PM.add(createAMDILPeepholeOpt(*TM));
  PM.add(createAMDILISelDag(getAMDGPUTargetMachine()));
  return false;
}

bool AMDGPUPassConfig::addPreRegAlloc() {
  const AMDILSubtarget &ST = TM->getSubtarget<AMDILSubtarget>();

  PM.add(createAMDGPUReorderPreloadInstructionsPass(*TM));
  if (ST.device()->getGeneration() <= AMDILDeviceInfo::HD6XXX) {
    PM.add(createR600LowerShaderInstructionsPass(*TM));
    PM.add(createR600LowerInstructionsPass(*TM));
  } else {
    PM.add(createSILowerShaderInstructionsPass(*TM));
    PM.add(createSIAssignInterpRegsPass(*TM));
  }
  PM.add(createAMDGPULowerInstructionsPass(*TM));
  PM.add(createAMDGPUConvertToISAPass(*TM));
  return false;
}

bool AMDGPUPassConfig::addPostRegAlloc() {
  return false;
}

bool AMDGPUPassConfig::addPreSched2() {
  return false;
}

bool AMDGPUPassConfig::addPreEmitPass() {
  const AMDILSubtarget &ST = TM->getSubtarget<AMDILSubtarget>();
  PM.add(createAMDILCFGPreparationPass(*TM));
  PM.add(createAMDILCFGStructurizerPass(*TM));
  if (ST.device()->getGeneration() == AMDILDeviceInfo::HD7XXX) {
    PM.add(createSIPropagateImmReadsPass(*TM));
  }

  PM.add(createAMDILIOExpansion(*TM));
  return false;
}


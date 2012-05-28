#include "AMDILMCTargetDesc.h"
#include "AMDILMCAsmInfo.h"
#include "llvm/MC/MachineLocation.h"
#include "llvm/MC/MCCodeGenInfo.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/TargetRegistry.h"

#define GET_INSTRINFO_MC_DESC
#include "AMDILGenInstrInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "AMDILGenSubtargetInfo.inc"

#define GET_REGINFO_MC_DESC
#include "AMDILGenRegisterInfo.inc"

using namespace llvm;

static MCInstrInfo *createAMDILMCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitAMDILMCInstrInfo(X);
  return X;
}

static MCRegisterInfo *createAMDILMCRegisterInfo(StringRef TT) {
  MCRegisterInfo *X = new MCRegisterInfo();
  InitAMDILMCRegisterInfo(X, 0);
  return X;
}

static MCSubtargetInfo *createAMDILMCSubtargetInfo(StringRef TT, StringRef CPU,
                                                   StringRef FS) {
  MCSubtargetInfo * X = new MCSubtargetInfo();
  InitAMDILMCSubtargetInfo(X, TT, CPU, FS);
  return X;
}

static MCCodeGenInfo *createAMDILMCCodeGenInfo(StringRef TT, Reloc::Model RM,
                                               CodeModel::Model CM,
                                               CodeGenOpt::Level OL) {
  MCCodeGenInfo *X = new MCCodeGenInfo();
  X->InitMCCodeGenInfo(RM, CM, OL);
  return X;
}

extern "C" void LLVMInitializeAMDGPUTargetMC() {

  RegisterMCAsmInfo<AMDILMCAsmInfo> Y(TheAMDGPUTarget);

  TargetRegistry::RegisterMCCodeGenInfo(TheAMDGPUTarget, createAMDILMCCodeGenInfo);

  TargetRegistry::RegisterMCInstrInfo(TheAMDGPUTarget, createAMDILMCInstrInfo);

  TargetRegistry::RegisterMCRegInfo(TheAMDGPUTarget, createAMDILMCRegisterInfo);

  TargetRegistry::RegisterMCSubtargetInfo(TheAMDGPUTarget, createAMDILMCSubtargetInfo);

}

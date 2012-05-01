//===-- AMDGPUTargetMachine.h - TODO: Add brief description -------===//
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

#ifndef AMDGPU_TARGET_MACHINE_H
#define AMDGPU_TARGET_MACHINE_H

#include "AMDGPUInstrInfo.h"
#include "AMDILTargetMachine.h"
#include "R600ISelLowering.h"
#include "llvm/ADT/OwningPtr.h"
#include "llvm/Target/TargetData.h"

namespace llvm {

MCAsmInfo* createMCAsmInfo(const Target &T, StringRef TT);

class AMDGPUTargetMachine : public AMDILTargetMachine {
  AMDILSubtarget Subtarget;
     const AMDGPUInstrInfo * InstrInfo;
     AMDGPUTargetLowering * TLInfo;
     AMDILGlobalManager *mGM;
     AMDILKernelManager *mKM;
     bool mDump;

public:
   AMDGPUTargetMachine(const Target &T, StringRef TT, StringRef FS,
                       StringRef CPU,
                       TargetOptions Options,
                       Reloc::Model RM, CodeModel::Model CM,
                       CodeGenOpt::Level OL);
   ~AMDGPUTargetMachine();
   virtual const AMDGPUInstrInfo *getInstrInfo() const {return InstrInfo;}
   virtual const AMDILSubtarget *getSubtargetImpl() const {return &Subtarget; }
   virtual const AMDGPURegisterInfo *getRegisterInfo() const {
      return &InstrInfo->getRegisterInfo();
   }
   virtual AMDGPUTargetLowering * getTargetLowering() const {
      return TLInfo;
   }
   virtual TargetPassConfig *createPassConfig(PassManagerBase &PM);
   virtual bool addPassesToEmitFile(PassManagerBase &PM,
                                              formatted_raw_ostream &Out,
                                              CodeGenFileType FileType,
                                              bool DisableVerify);
};

} /* End namespace llvm */

#endif /* AMDGPU_TARGET_MACHINE_H */

//===-- AMDILTargetMachine.h - Define TargetMachine for AMDIL ---*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//
//
// This file declares the AMDIL specific subclass of TargetMachine.
//
//===----------------------------------------------------------------------===//

#ifndef AMDILTARGETMACHINE_H_
#define AMDILTARGETMACHINE_H_

#include "AMDILELFWriterInfo.h"
#include "AMDILFrameLowering.h"
#include "AMDILISelLowering.h"
#include "AMDILInstrInfo.h"
#include "AMDILIntrinsicInfo.h"
#include "AMDILSubtarget.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm
{
    class raw_ostream;

    class AMDILTargetMachine : public LLVMTargetMachine
    {
        private:
        AMDILSubtarget Subtarget;
        const TargetData DataLayout;       // Calculates type size & alignment
        AMDILFrameLowering FrameLowering;
        AMDILInstrInfo InstrInfo;
        AMDILTargetLowering TLInfo;
        AMDILIntrinsicInfo IntrinsicInfo;
        AMDILELFWriterInfo ELFWriterInfo;
        bool mDebugMode;
        CodeGenOpt::Level mOptLevel;

        protected:

        public:
        AMDILTargetMachine(const Target &T,
             StringRef TT, StringRef CPU, StringRef FS,
             TargetOptions Options,
             Reloc::Model RM, CodeModel::Model CM,
             CodeGenOpt::Level OL);

        // Get Target/Subtarget specific information
        virtual AMDILTargetLowering* getTargetLowering() const;
        virtual const AMDILInstrInfo* getInstrInfo() const;
        virtual const AMDILFrameLowering* getFrameLowering() const;
        virtual const AMDILSubtarget* getSubtargetImpl() const;
        virtual const AMDILRegisterInfo* getRegisterInfo() const;
        virtual const TargetData* getTargetData() const;
        virtual const AMDILIntrinsicInfo *getIntrinsicInfo() const;
        virtual const AMDILELFWriterInfo *getELFWriterInfo() const;

        // Pass Pipeline Configuration
        virtual TargetPassConfig *createPassConfig(PassManagerBase &PM);

        void dump(llvm::raw_ostream &O);
        void setDebug(bool debugMode);
        bool getDebug() const;
        CodeGenOpt::Level getOptLevel() const { return mOptLevel; }


    }; // AMDILTargetMachine

} // end namespace llvm

#endif // AMDILTARGETMACHINE_H_

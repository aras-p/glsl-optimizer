//=====-- AMDGPUSubtarget.h - Define Subtarget for the AMDIL ---*- C++ -*-====//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//
//
// This file declares the AMDGPU specific subclass of TargetSubtarget.
//
//===----------------------------------------------------------------------===//

#ifndef _AMDGPUSUBTARGET_H_
#define _AMDGPUSUBTARGET_H_
#include "AMDILSubtarget.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringRef.h"

namespace llvm {

class AMDGPUSubtarget : public AMDILSubtarget
{
  InstrItineraryData InstrItins;

public:
  AMDGPUSubtarget(StringRef TT, StringRef CPU, StringRef FS);

  const InstrItineraryData &getInstrItineraryData() const { return InstrItins; }
  virtual void ParseSubtargetFeatures(llvm::StringRef CPU, llvm::StringRef FS);

};

} // End namespace llvm

#endif // AMDGPUSUBTARGET_H_

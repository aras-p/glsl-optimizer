//===-- MCTargetDesc/AMDILMCAsmInfo.h - TODO: Add brief description -------===//
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

#ifndef AMDILMCASMINFO_H_
#define AMDILMCASMINFO_H_

#include "llvm/MC/MCAsmInfo.h"
namespace llvm {
  class Target;
  class StringRef;

  class AMDILMCAsmInfo : public MCAsmInfo {
    public:
      explicit AMDILMCAsmInfo(const Target &T, StringRef &TT);
      const char*
        getDataASDirective(unsigned int Size, unsigned int AS) const;
      const MCSection* getNonexecutableStackSection(MCContext &CTX) const;
  };
} // namespace llvm
#endif // AMDILMCASMINFO_H_

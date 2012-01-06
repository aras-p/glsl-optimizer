//===-- AMDILELFWriterInfo.h - Elf Writer Info for AMDIL ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===//
//
//   This file implements ELF writer information for the AMDIL backend.
//
//===---------------------------------------------------------------------===//
#ifndef _AMDIL_ELF_WRITER_INFO_H_
#define _AMDIL_ELF_WRITER_INFO_H_
#include "llvm/Target/TargetELFWriterInfo.h"

namespace llvm {
  class AMDILELFWriterInfo : public TargetELFWriterInfo {
  public:
    AMDILELFWriterInfo(bool is64Bit_, bool isLittleEndian_);
    virtual ~AMDILELFWriterInfo();

    /// getRelocationType - Returns the target specific ELF Relocation type.
    /// 'MachineRelTy' contains the object code independent relocation type
    virtual unsigned getRelocationType(unsigned MachineRelTy) const;

    /// 'hasRelocationAddend - True if the target uses and addend in the
    /// ELF relocation entry.
    virtual bool hasRelocationAddend() const;

    /// getDefaultAddendForRelTy - Gets the default addend value for a
    /// relocation entry based on the target ELF relocation type.
    virtual long int getDefaultAddendForRelTy(unsigned RelTy,
                                              long int Modifier = 0) const;

    /// getRelTySize - Returns the size of relocatble field in bits
    virtual unsigned getRelocationTySize(unsigned RelTy) const;

    /// isPCRelativeRel - True if the relocation type is pc relative
    virtual bool isPCRelativeRel(unsigned RelTy) const;

    /// getJumpTableRelocationTy - Returns the machine relocation type used
    /// to reference a jumptable.
    virtual unsigned getAbsoluteLabelMachineRelTy() const;

    /// computeRelocation - Some relocatable fields could be relocated
    /// directly, avoiding the relocation symbol emission, compute the
    /// final relocation value for this symbol.
    virtual long int computeRelocation(unsigned SymOffset,
                                       unsigned RelOffset,
                                       unsigned RelTy) const;
  };
} // namespace llvm
#endif // _AMDIL_ELF_WRITER_INFO_H_

//===------ AMDILAsmBackend.cpp - AMDIL Assembly Backend ---===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//
//
//
#include "AMDILAsmBackend.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;
namespace llvm {
  ASM_BACKEND_CLASS* createAMDILAsmBackend(const ASM_BACKEND_CLASS &T,
                                          const std::string &TT)
  {
    return new AMDILAsmBackend(T);
  }
} // namespace llvm

//===--------------------- Default AMDIL Asm Backend ---------------------===//
AMDILAsmBackend::AMDILAsmBackend(const ASM_BACKEND_CLASS &T) 
  : ASM_BACKEND_CLASS()
{
}

MCObjectWriter *
AMDILAsmBackend::createObjectWriter(raw_ostream &OS) const
{
  return 0;
}

bool 
AMDILAsmBackend::doesSectionRequireSymbols(const MCSection &Section) const
{
  return false;
}

bool 
AMDILAsmBackend::isSectionAtomizable(const MCSection &Section) const
{
  return true;
}

bool 
AMDILAsmBackend::isVirtualSection(const MCSection &Section) const
{
  return false;
  //const MCSectionELF &SE = static_cast<const MCSectionELF&>(Section);
  //return SE.getType() == MCSectionELF::SHT_NOBITS;
}
void 
AMDILAsmBackend::ApplyFixup(const MCFixup &Fixup, char *Data, unsigned DataSize,
                          uint64_t Value) const
{
}

bool 
AMDILAsmBackend::MayNeedRelaxation(const MCInst &Inst) const
{
    return false;
}

void 
AMDILAsmBackend::RelaxInstruction(const MCInst &Inst,
                                       MCInst &Res) const
{
}

bool 
AMDILAsmBackend::WriteNopData(uint64_t Count, MCObjectWriter *OW) const
{
  return false;
}

unsigned
AMDILAsmBackend::getNumFixupKinds() const
{
  return 0;
}

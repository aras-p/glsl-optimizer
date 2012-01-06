//===-- AMDILELFWriterInfo.cpp - Elf Writer Info for AMDIL ----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//   This file implements ELF writer information for the AMDIL backend.
//
//===----------------------------------------------------------------------===//

#include "AMDILELFWriterInfo.h"
#include "AMDIL.h"
#include "llvm/Function.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Target/TargetELFWriterInfo.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;

//===----------------------------------------------------------------------===//
//  Implementation of the AMDILELFWriterInfo class
//===----------------------------------------------------------------------===//
AMDILELFWriterInfo::AMDILELFWriterInfo(bool is64bit, bool endian)
  : TargetELFWriterInfo(is64bit, endian)
{
}

AMDILELFWriterInfo::~AMDILELFWriterInfo() {
}

unsigned AMDILELFWriterInfo::getRelocationType(unsigned MachineRelTy) const {
  assert(0 && "What do we do here? Lets assert an analyze");
  return 0;
}

bool AMDILELFWriterInfo::hasRelocationAddend() const {
  assert(0 && "What do we do here? Lets assert an analyze");
  return false;
}

long int AMDILELFWriterInfo::getDefaultAddendForRelTy(unsigned RelTy,
                                                      long int Modifier) const {
  assert(0 && "What do we do here? Lets assert an analyze");
  return 0;
}

unsigned AMDILELFWriterInfo::getRelocationTySize(unsigned RelTy) const {
  assert(0 && "What do we do here? Lets assert an analyze");
  return 0;
}

bool AMDILELFWriterInfo::isPCRelativeRel(unsigned RelTy) const {
  assert(0 && "What do we do here? Lets assert an analyze");
  return false;
}

unsigned AMDILELFWriterInfo::getAbsoluteLabelMachineRelTy() const {
  assert(0 && "What do we do here? Lets assert an analyze");
  return 0;
}

long int AMDILELFWriterInfo::computeRelocation(unsigned SymOffset,
                                               unsigned RelOffset,
                                               unsigned RelTy) const {
  assert(0 && "What do we do here? Lets assert an analyze");
  return 0;
}

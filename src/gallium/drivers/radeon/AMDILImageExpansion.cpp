//===-- AMDILImageExpansion.cpp - TODO: Add brief description -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//
// @file AMDILImageExpansion.cpp
// @details Implementatino of the Image expansion class for image capable devices
//
#include "AMDILIOExpansion.h"
#include "AMDILKernelManager.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Support/DebugLoc.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/Value.h"

using namespace llvm;

AMDILImageExpansion::AMDILImageExpansion(TargetMachine &tm AMDIL_OPT_LEVEL_DECL)
  : AMDIL789IOExpansion(tm AMDIL_OPT_LEVEL_VAR)
{
}

AMDILImageExpansion::~AMDILImageExpansion()
{
}
void AMDILImageExpansion::expandInefficientImageLoad(
    MachineBasicBlock *mBB, MachineInstr *MI)
{
#if 0
  const llvm::StringRef &name = MI->getOperand(0).getGlobal()->getName();
  const char *tReg1, *tReg2, *tReg3, *tReg4;
  tReg1 = mASM->getRegisterName(MI->getOperand(1).getReg());
  if (MI->getOperand(2).isReg()) {
    tReg2 = mASM->getRegisterName(MI->getOperand(2).getReg());
  } else {
    tReg2 = mASM->getRegisterName(AMDIL::R1);
    O << "\tmov " << tReg2 << ", l" << MI->getOperand(2).getImm() << "\n";
  }
  if (MI->getOperand(3).isReg()) {
    tReg3 = mASM->getRegisterName(MI->getOperand(3).getReg());
  } else {
    tReg3 = mASM->getRegisterName(AMDIL::R2);
    O << "\tmov " << tReg3 << ", l" << MI->getOperand(3).getImm() << "\n";
  }
  if (MI->getOperand(4).isReg()) {
    tReg4 = mASM->getRegisterName(MI->getOperand(4).getReg());
  } else {
    tReg4 = mASM->getRegisterName(AMDIL::R3);
    O << "\tmov " << tReg2 << ", l" << MI->getOperand(4).getImm() << "\n";
  }
  bool internalSampler = false;
  //bool linear = true;
  unsigned ImageCount = 3; // OPENCL_MAX_READ_IMAGES
  unsigned SamplerCount = 3; // OPENCL_MAX_SAMPLERS
  if (ImageCount - 1) {
  O << "\tswitch " << mASM->getRegisterName(MI->getOperand(1).getReg())
    << "\n";
  }
  for (unsigned rID = 0; rID < ImageCount; ++rID) {
    if (ImageCount - 1)  {
    if (!rID) {
      O << "\tdefault\n";
    } else {
      O << "\tcase " << rID << "\n" ;
    }
    O << "\tswitch " << mASM->getRegisterName(MI->getOperand(2).getReg())
     << "\n";
    }
    for (unsigned sID = 0; sID < SamplerCount; ++sID) {
      if (SamplerCount - 1) {
      if (!sID) {
        O << "\tdefault\n";
      } else {
        O << "\tcase " << sID << "\n" ;
      }
      }
      if (internalSampler) {
        // Check if sampler has normalized setting.
        O << "\tand r0.x, " << tReg2 << ".x, l0.y\n"
          << "\tif_logicalz r0.x\n"
          << "\tflr " << tReg3 << ", " << tReg3 << "\n"
          << "\tsample_resource(" << rID << ")_sampler("
          << sID << ")_coordtype(unnormalized) "
          << tReg1 << ", " << tReg3 << " ; " << name.data() << "\n"
          << "\telse\n"
          << "\tiadd " << tReg1 << ".y, " << tReg1 << ".x, l0.y\n"
          << "\titof " << tReg2 << ", cb1[" << tReg1 << ".x].xyz\n"
          << "\tmul " << tReg3 << ", " << tReg3 << ", " << tReg2 << "\n"
          << "\tflr " << tReg3 << ", " << tReg3 << "\n"
          << "\tmul " << tReg3 << ", " << tReg3 << ", cb1[" 
          << tReg1 << ".y].xyz\n"
          << "\tsample_resource(" << rID << ")_sampler("
          << sID << ")_coordtype(normalized) "
          << tReg1 << ", " << tReg3 << " ; " << name.data() << "\n"
          << "\tendif\n";
      } else {
        O << "\tiadd " << tReg1 << ".y, " << tReg1 << ".x, l0.y\n"
          // Check if sampler has normalized setting.
          << "\tand r0, " << tReg2 << ".x, l0.y\n"
          // Convert image dimensions to float.
          << "\titof " << tReg4 << ", cb1[" << tReg1 << ".x].xyz\n"
          // Move into R0 1 if unnormalized or dimensions if normalized.
          << "\tcmov_logical r0, r0, " << tReg4 << ", r1.1111\n"
          // Make coordinates unnormalized.
          << "\tmul " << tReg3 << ", r0, " << tReg3 << "\n"
          // Get linear filtering if set.
          << "\tand " << tReg4 << ", " << tReg2 << ".x, l6.x\n"
          // Save unnormalized coordinates in R0.
          << "\tmov r0, " << tReg3 << "\n"
          // Floor the coordinates due to HW incompatibility with precision
          // requirements.
          << "\tflr " << tReg3 << ", " << tReg3 << "\n"
          // get Origianl coordinates (without floor) if linear filtering
          << "\tcmov_logical " << tReg3 << ", " << tReg4 
          << ".xxxx, r0, " << tReg3 << "\n"
          // Normalize the coordinates with multiplying by 1/dimensions
          << "\tmul " << tReg3 << ", " << tReg3 << ", cb1[" 
          << tReg1 << ".y].xyz\n"
          << "\tsample_resource(" << rID << ")_sampler("
          << sID << ")_coordtype(normalized) "
          << tReg1 << ", " << tReg3 << " ; " << name.data() << "\n";
      }
      if (SamplerCount - 1) {
      O << "\tbreak\n";
      }
    }
    if (SamplerCount - 1) {
      O << "\tendswitch\n";
    }
    if (ImageCount - 1) {
    O << "\tbreak\n";
    }
  }
  if (ImageCount - 1) {
    O << "\tendswitch\n";
  }
#endif
}
  void
AMDILImageExpansion::expandImageLoad(MachineBasicBlock *mBB, MachineInstr *MI)
{
  uint32_t imageID = getPointerID(MI);
  MI->getOperand(1).ChangeToImmediate(imageID);
  saveInst = true;
}
  void
AMDILImageExpansion::expandImageStore(MachineBasicBlock *mBB, MachineInstr *MI)
{
  uint32_t imageID = getPointerID(MI);
  mKM->setOutputInst();
  MI->getOperand(0).ChangeToImmediate(imageID);
  saveInst = true;
}
  void
AMDILImageExpansion::expandImageParam(MachineBasicBlock *mBB, MachineInstr *MI)
{
    MachineBasicBlock::iterator I = *MI;
    uint32_t ID = getPointerID(MI);
    DebugLoc DL = MI->getDebugLoc();
    BuildMI(*mBB, I, DL, mTII->get(AMDIL::CBLOAD), 
        MI->getOperand(0).getReg())
        .addImm(ID)
        .addImm(1);
}

//===-- AMDIL7XXIOExpansion.cpp - TODO: Add brief description -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//
// @file AMDIL7XXIOExpansion.cpp
// @details Implementation of the IO Printing class for 7XX devices
//
#include "AMDILCompilerErrors.h"
#include "AMDILCompilerWarnings.h"
#include "AMDILDevices.h"
#include "AMDILGlobalManager.h"
#include "AMDILIOExpansion.h"
#include "AMDILKernelManager.h"
#include "AMDILMachineFunctionInfo.h"
#include "AMDILTargetMachine.h"
#include "AMDILUtilityFunctions.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Support/DebugLoc.h"
#include "llvm/Value.h"

using namespace llvm;
AMDIL7XXIOExpansion::AMDIL7XXIOExpansion(TargetMachine &tm
    AMDIL_OPT_LEVEL_DECL) : AMDIL789IOExpansion(tm  AMDIL_OPT_LEVEL_VAR)
{
}

AMDIL7XXIOExpansion::~AMDIL7XXIOExpansion() {
}
const char *AMDIL7XXIOExpansion::getPassName() const
{
  return "AMDIL 7XX IO Expansion Pass";
}

  void
AMDIL7XXIOExpansion::expandGlobalLoad(MachineInstr *MI)
{
  DebugLoc DL;
  // These instructions go before the current MI.
  expandLoadStartCode(MI);
  uint32_t ID = getPointerID(MI);
  mKM->setOutputInst();
  switch(getMemorySize(MI)) {
    default:
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::UAVRAWLOAD_v4i32), AMDIL::R1011)
        .addReg(AMDIL::R1010)
        .addImm(ID);
      break;
    case 4:
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::UAVRAWLOAD_i32), AMDIL::R1011)
        .addReg(AMDIL::R1010)
        .addImm(ID);
      break;
    case 8:
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::UAVRAWLOAD_v2i32), AMDIL::R1011)
        .addReg(AMDIL::R1010)
        .addImm(ID);
      break;
    case 1:
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1008)
        .addReg(AMDIL::R1010)
        .addImm(mMFI->addi32Literal(3));
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1010)
        .addReg(AMDIL::R1010)
        .addImm(mMFI->addi32Literal(0xFFFFFFFC));
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::VCREATE_v4i32), AMDIL::R1008)
        .addReg(AMDIL::R1008);
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::ADD_v4i32), AMDIL::R1008)
        .addReg(AMDIL::R1008)
        .addImm(mMFI->addi128Literal(0xFFFFFFFFULL << 32, 
                (0xFFFFFFFEULL | (0xFFFFFFFDULL << 32))));
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::IEQ_v4i32), AMDIL::R1012)
        .addReg(AMDIL::R1008)
        .addImm(mMFI->addi32Literal(0));
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::CMOVLOG_i32), AMDIL::R1008)
        .addReg(AMDIL::R1012)
        .addImm(mMFI->addi32Literal(0))
        .addImm(mMFI->addi32Literal(24));
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::CMOVLOG_Y_i32), AMDIL::R1008)
        .addReg(AMDIL::R1012)
        .addImm(mMFI->addi32Literal(8))
        .addReg(AMDIL::R1008);
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::CMOVLOG_Z_i32), AMDIL::R1008)
        .addReg(AMDIL::R1012)
        .addImm(mMFI->addi32Literal(16))
        .addReg(AMDIL::R1008);
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::UAVRAWLOAD_i32), AMDIL::R1011)
        .addReg(AMDIL::R1010)
        .addImm(ID);
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::SHR_i8), AMDIL::R1011)
        .addReg(AMDIL::R1011)
        .addReg(AMDIL::R1008);
      break;
    case 2:
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1008)
        .addReg(AMDIL::R1010)
        .addImm(mMFI->addi32Literal(3));
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::SHR_i32), AMDIL::R1008)
        .addReg(AMDIL::R1008)
        .addImm(mMFI->addi32Literal(1));
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1010)
        .addReg(AMDIL::R1010)
        .addImm(mMFI->addi32Literal(0xFFFFFFFC));
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::CMOVLOG_i32), AMDIL::R1008)
        .addReg(AMDIL::R1008)
        .addImm(mMFI->addi32Literal(16))
        .addImm(mMFI->addi32Literal(0));
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::UAVRAWLOAD_i32), AMDIL::R1011)
        .addReg(AMDIL::R1010)
        .addImm(ID);
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::SHR_i16), AMDIL::R1011)
        .addReg(AMDIL::R1011)
        .addReg(AMDIL::R1008);
      break;
  }
  // These instructions go after the current MI.
  expandPackedData(MI);
  expandExtendLoad(MI);
  BuildMI(*mBB, MI, MI->getDebugLoc(),
      mTII->get(getMoveInstFromID(
          MI->getDesc().OpInfo[0].RegClass)))
    .addOperand(MI->getOperand(0))
    .addReg(AMDIL::R1011);
  MI->getOperand(0).setReg(AMDIL::R1011);
}

  void
AMDIL7XXIOExpansion::expandRegionLoad(MachineInstr *MI)
{
  bool HWRegion = mSTM->device()->usesHardware(AMDILDeviceInfo::RegionMem);
  if (!mSTM->device()->isSupported(AMDILDeviceInfo::RegionMem)) {
    mMFI->addErrorMsg(
        amd::CompilerErrorMessage[REGION_MEMORY_ERROR]);
    return;
  }
  if (!HWRegion || !isHardwareRegion(MI)) {
    return expandGlobalLoad(MI);
  }
  if (!mMFI->usesMem(AMDILDevice::GDS_ID)
      && mKM->isKernel()) {
    mMFI->addErrorMsg(amd::CompilerErrorMessage[MEMOP_NO_ALLOCATION]);
  }
  uint32_t gID = getPointerID(MI);
  assert(gID && "Found a GDS load that was incorrectly marked as zero ID!\n");
  if (!gID) {
    gID = mSTM->device()->getResourceID(AMDILDevice::GDS_ID);
    mMFI->addErrorMsg(amd::CompilerWarningMessage[RECOVERABLE_ERROR]);
  }
  
  DebugLoc DL;
  // These instructions go before the current MI.
  expandLoadStartCode(MI);
   switch (getMemorySize(MI)) {
    default:
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::VCREATE_v4i32), AMDIL::R1010)
        .addReg(AMDIL::R1010);
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::ADD_v4i32), AMDIL::R1010)
        .addReg(AMDIL::R1010)
        .addImm(mMFI->addi128Literal(1ULL << 32, 2ULL | (3ULL << 32)));
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::GDSLOAD), AMDIL::R1011)
        .addReg(AMDIL::R1010)
        .addImm(gID);
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::GDSLOAD_Y), AMDIL::R1011)
        .addReg(AMDIL::R1010)
        .addImm(gID);
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::GDSLOAD_Z), AMDIL::R1011)
        .addReg(AMDIL::R1010)
        .addImm(gID);
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::GDSLOAD_W), AMDIL::R1011)
        .addReg(AMDIL::R1010)
        .addImm(gID);
      break;
    case 1:
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1008)
        .addReg(AMDIL::R1010)
        .addImm(mMFI->addi32Literal(3));
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::UMUL_i32), AMDIL::R1008)
        .addReg(AMDIL::R1008)
        .addImm(mMFI->addi32Literal(8));
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1010)
        .addReg(AMDIL::R1010)
        .addImm(mMFI->addi32Literal(0xFFFFFFFC));
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::GDSLOAD), AMDIL::R1011)
        .addReg(AMDIL::R1010)
        .addImm(gID);
      // The instruction would normally fit in right here so everything created
      // after this point needs to go into the afterInst vector.
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::SHR_i32), AMDIL::R1011)
        .addReg(AMDIL::R1011)
        .addReg(AMDIL::R1008);
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::SHL_i32), AMDIL::R1011)
        .addReg(AMDIL::R1011)
        .addImm(mMFI->addi32Literal(24));
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::SHR_i32), AMDIL::R1011)
        .addReg(AMDIL::R1011)
        .addImm(mMFI->addi32Literal(24));
      break;
    case 2:
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1008)
        .addReg(AMDIL::R1010)
        .addImm(mMFI->addi32Literal(3));
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::UMUL_i32), AMDIL::R1008)
        .addReg(AMDIL::R1008)
        .addImm(mMFI->addi32Literal(8));
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1010)
        .addReg(AMDIL::R1010)
        .addImm(mMFI->addi32Literal(0xFFFFFFFC));
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::GDSLOAD), AMDIL::R1011)
        .addReg(AMDIL::R1010)
        .addImm(gID);
      // The instruction would normally fit in right here so everything created
      // after this point needs to go into the afterInst vector.
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::SHR_i32), AMDIL::R1011)
        .addReg(AMDIL::R1011)
        .addReg(AMDIL::R1008);
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::SHL_i32), AMDIL::R1011)
        .addReg(AMDIL::R1011)
        .addImm(mMFI->addi32Literal(16));
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::SHR_i32), AMDIL::R1011)
        .addReg(AMDIL::R1011)
        .addImm(mMFI->addi32Literal(16));
      break;
    case 4:
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::GDSLOAD), AMDIL::R1011)
        .addReg(AMDIL::R1010)
        .addImm(gID);
      break;
    case 8:
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::VCREATE_v2i32), AMDIL::R1010)
        .addReg(AMDIL::R1010);
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::ADD_v4i32), AMDIL::R1010)
        .addReg(AMDIL::R1010)
        .addImm(mMFI->addi64Literal(1ULL << 32));
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::GDSLOAD), AMDIL::R1011)
        .addReg(AMDIL::R1010)
        .addImm(gID);
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::GDSLOAD_Y), AMDIL::R1011)
        .addReg(AMDIL::R1010)
        .addImm(gID);
      break;
   }

  // These instructions go after the current MI.
  expandPackedData(MI);
  expandExtendLoad(MI);
  BuildMI(*mBB, MI, MI->getDebugLoc(),
      mTII->get(getMoveInstFromID(
          MI->getDesc().OpInfo[0].RegClass)))
    .addOperand(MI->getOperand(0))
    .addReg(AMDIL::R1011);
  MI->getOperand(0).setReg(AMDIL::R1011);
}
  void
AMDIL7XXIOExpansion::expandLocalLoad(MachineInstr *MI)
{
  bool HWLocal = mSTM->device()->usesHardware(AMDILDeviceInfo::LocalMem);
  if (!HWLocal || !isHardwareLocal(MI)) {
    return expandGlobalLoad(MI);
  }
  if (!mMFI->usesMem(AMDILDevice::LDS_ID)
      && mKM->isKernel()) {
    mMFI->addErrorMsg(amd::CompilerErrorMessage[MEMOP_NO_ALLOCATION]);
  }
  uint32_t lID = getPointerID(MI);
  assert(lID && "Found a LDS load that was incorrectly marked as zero ID!\n");
  if (!lID) {
    lID = mSTM->device()->getResourceID(AMDILDevice::LDS_ID);
    mMFI->addErrorMsg(amd::CompilerWarningMessage[RECOVERABLE_ERROR]);
  }
  DebugLoc DL;
  // These instructions go before the current MI.
  expandLoadStartCode(MI);
  switch (getMemorySize(MI)) {
    default:
    case 8:
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::LDSLOADVEC), AMDIL::R1011) 
        .addReg(AMDIL::R1010)
        .addImm(lID);
      break;
    case 4:
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::LDSLOAD), AMDIL::R1011) 
        .addReg(AMDIL::R1010)
        .addImm(lID);
      break;
    case 1:
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1008)
        .addReg(AMDIL::R1010)
        .addImm(mMFI->addi32Literal(3));
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::UMUL_i32), AMDIL::R1008)
        .addReg(AMDIL::R1008)
        .addImm(mMFI->addi32Literal(8));
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1010)
        .addReg(AMDIL::R1010)
        .addImm(mMFI->addi32Literal(0xFFFFFFFC));
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::LDSLOAD), AMDIL::R1011) 
        .addReg(AMDIL::R1010)
        .addImm(lID);
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::SHR_i32), AMDIL::R1011)
        .addReg(AMDIL::R1011)
        .addReg(AMDIL::R1008);
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::SHL_i32), AMDIL::R1011)
        .addReg(AMDIL::R1011)
        .addImm(mMFI->addi32Literal(24));
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::SHR_i32), AMDIL::R1011)
        .addReg(AMDIL::R1011)
        .addImm(mMFI->addi32Literal(24));
      break;
    case 2:
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1008)
        .addReg(AMDIL::R1010)
        .addImm(mMFI->addi32Literal(3));
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::UMUL_i32), AMDIL::R1008)
        .addReg(AMDIL::R1008)
        .addImm(mMFI->addi32Literal(8));
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1010)
        .addReg(AMDIL::R1010)
        .addImm(mMFI->addi32Literal(0xFFFFFFFC));
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::LDSLOAD), AMDIL::R1011) 
        .addReg(AMDIL::R1010)
        .addImm(lID);
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::SHR_i32), AMDIL::R1011)
        .addReg(AMDIL::R1011)
        .addReg(AMDIL::R1008);
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::SHL_i32), AMDIL::R1011)
        .addReg(AMDIL::R1011)
        .addImm(mMFI->addi32Literal(16));
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::SHR_i32), AMDIL::R1011)
        .addReg(AMDIL::R1011)
        .addImm(mMFI->addi32Literal(16));
      break;
   }

  // These instructions go after the current MI.
  expandPackedData(MI);
  expandExtendLoad(MI);
  BuildMI(*mBB, MI, MI->getDebugLoc(),
      mTII->get(getMoveInstFromID(
          MI->getDesc().OpInfo[0].RegClass)))
    .addOperand(MI->getOperand(0))
    .addReg(AMDIL::R1011);
  MI->getOperand(0).setReg(AMDIL::R1011);
}

  void
AMDIL7XXIOExpansion::expandGlobalStore(MachineInstr *MI)
{
  uint32_t ID = getPointerID(MI);
  mKM->setOutputInst();
  DebugLoc DL = MI->getDebugLoc();
  // These instructions go before the current MI.
  expandStoreSetupCode(MI);
  switch (getMemorySize(MI)) {
    default:
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::UAVRAWSTORE_v4i32), AMDIL::MEM)
        .addReg(AMDIL::R1010)
        .addReg(AMDIL::R1011)
        .addImm(ID);
      break;
    case 1:
      mMFI->addErrorMsg(
          amd::CompilerErrorMessage[BYTE_STORE_ERROR]);
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::UAVRAWSTORE_i32), AMDIL::MEM)
        .addReg(AMDIL::R1010)
        .addReg(AMDIL::R1011)
        .addImm(ID);
      break;
    case 2:
      mMFI->addErrorMsg(
          amd::CompilerErrorMessage[BYTE_STORE_ERROR]);
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::UAVRAWSTORE_i32), AMDIL::MEM)
        .addReg(AMDIL::R1010)
        .addReg(AMDIL::R1011)
        .addImm(ID);
      break;
    case 4:
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::UAVRAWSTORE_i32), AMDIL::MEM)
        .addReg(AMDIL::R1010)
        .addReg(AMDIL::R1011)
        .addImm(ID);
      break;
    case 8:
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::UAVRAWSTORE_v2i32), AMDIL::MEM)
        .addReg(AMDIL::R1010)
        .addReg(AMDIL::R1011)
        .addImm(ID);
      break;
  };
}

  void
AMDIL7XXIOExpansion::expandRegionStore(MachineInstr *MI)
{
  bool HWRegion = mSTM->device()->usesHardware(AMDILDeviceInfo::RegionMem);
  if (!mSTM->device()->isSupported(AMDILDeviceInfo::RegionMem)) {
    mMFI->addErrorMsg(
        amd::CompilerErrorMessage[REGION_MEMORY_ERROR]);
    return;
  }
  if (!HWRegion || !isHardwareRegion(MI)) {
    return expandGlobalStore(MI);
  }
  DebugLoc DL = MI->getDebugLoc();
  mKM->setOutputInst();
  if (!mMFI->usesMem(AMDILDevice::GDS_ID)
      && mKM->isKernel()) {
    mMFI->addErrorMsg(amd::CompilerErrorMessage[MEMOP_NO_ALLOCATION]);
  }
  uint32_t gID = getPointerID(MI);
  assert(gID && "Found a GDS store that was incorrectly marked as zero ID!\n");
  if (!gID) {
    gID = mSTM->device()->getResourceID(AMDILDevice::GDS_ID);
    mMFI->addErrorMsg(amd::CompilerWarningMessage[RECOVERABLE_ERROR]);
  }

  // These instructions go before the current MI.
  expandStoreSetupCode(MI);
  switch (getMemorySize(MI)) {
    default:
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::VCREATE_v4i32), AMDIL::R1010)
        .addReg(AMDIL::R1010);
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::ADD_v4i32), AMDIL::R1010)
        .addReg(AMDIL::R1010)
        .addImm(mMFI->addi128Literal(1ULL << 32, 2ULL | (3ULL << 32)));
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::GDSSTORE), AMDIL::R1010)
        .addReg(AMDIL::R1011)
        .addImm(gID);
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::GDSSTORE_Y), AMDIL::R1010)
        .addReg(AMDIL::R1011)
        .addImm(gID);
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::GDSSTORE_Z), AMDIL::R1010)
        .addReg(AMDIL::R1011)
        .addImm(gID);
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::GDSSTORE_W), AMDIL::R1010)
        .addReg(AMDIL::R1011)
        .addImm(gID);
      break;
    case 1:
      mMFI->addErrorMsg(
          amd::CompilerErrorMessage[BYTE_STORE_ERROR]);
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1011)
        .addReg(AMDIL::R1011)
        .addImm(mMFI->addi32Literal(0xFF));
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1012)
        .addReg(AMDIL::R1010)
        .addImm(mMFI->addi32Literal(3));
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::VCREATE_v4i32), AMDIL::R1008)
        .addReg(AMDIL::R1008);
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::ADD_v4i32), AMDIL::R1008)
        .addReg(AMDIL::R1008)
        .addImm(mMFI->addi128Literal(0xFFFFFFFFULL << 32, 
              (0xFFFFFFFEULL | (0xFFFFFFFDULL << 32))));
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::UMUL_i32), AMDIL::R1006)
        .addReg(AMDIL::R1008)
        .addImm(mMFI->addi32Literal(8));
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::CMOVLOG_i32), AMDIL::R1007)
        .addReg(AMDIL::R1008)
        .addImm(mMFI->addi32Literal(0xFFFFFF00))
        .addImm(mMFI->addi32Literal(0x00FFFFFF));
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::CMOVLOG_Y_i32), AMDIL::R1007)
        .addReg(AMDIL::R1008)
        .addReg(AMDIL::R1007)
        .addImm(mMFI->addi32Literal(0xFF00FFFF));
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::CMOVLOG_Z_i32), AMDIL::R1012)
        .addReg(AMDIL::R1008)
        .addReg(AMDIL::R1007)
        .addImm(mMFI->addi32Literal(0xFFFF00FF));
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::SHL_i32), AMDIL::R1011)
        .addReg(AMDIL::R1011)
        .addReg(AMDIL::R1007);
       BuildMI(*mBB, MI, DL, mTII->get(AMDIL::GDSSTORE), AMDIL::R1010)
        .addReg(AMDIL::R1011)
        .addImm(gID);
       break;
    case 2:
      mMFI->addErrorMsg(
          amd::CompilerErrorMessage[BYTE_STORE_ERROR]);
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1011)
        .addReg(AMDIL::R1011)
        .addImm(mMFI->addi32Literal(0x0000FFFF));
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1008)
        .addReg(AMDIL::R1010)
        .addImm(mMFI->addi32Literal(3));
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::SHR_i32), AMDIL::R1008)
        .addReg(AMDIL::R1008)
        .addImm(mMFI->addi32Literal(1));
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::CMOVLOG_i32), AMDIL::R1012)
        .addReg(AMDIL::R1008)
        .addImm(mMFI->addi32Literal(0x0000FFFF))
        .addImm(mMFI->addi32Literal(0xFFFF0000));
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::CMOVLOG_i32), AMDIL::R1008)
        .addReg(AMDIL::R1008)
        .addImm(mMFI->addi32Literal(16))
        .addImm(mMFI->addi32Literal(0));
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::SHL_i32), AMDIL::R1011)
        .addReg(AMDIL::R1011)
        .addReg(AMDIL::R1008);
       BuildMI(*mBB, MI, DL, mTII->get(AMDIL::GDSSTORE), AMDIL::R1010)
        .addReg(AMDIL::R1011)
        .addImm(gID);
       break;
    case 4:
       BuildMI(*mBB, MI, DL, mTII->get(AMDIL::GDSSTORE), AMDIL::R1010)
        .addReg(AMDIL::R1011)
        .addImm(gID);
      break;
    case 8:
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::VCREATE_v2i32), AMDIL::R1010)
        .addReg(AMDIL::R1010);
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::ADD_v4i32), AMDIL::R1010)
        .addReg(AMDIL::R1010)
        .addImm(mMFI->addi64Literal(1ULL << 32));
       BuildMI(*mBB, MI, DL, mTII->get(AMDIL::GDSSTORE), AMDIL::R1010)
        .addReg(AMDIL::R1011)
        .addImm(gID);
       BuildMI(*mBB, MI, DL, mTII->get(AMDIL::GDSSTORE_Y), AMDIL::R1010)
        .addReg(AMDIL::R1011)
        .addImm(gID);
      break;
   };
}

  void
AMDIL7XXIOExpansion::expandLocalStore(MachineInstr *MI)
{
  bool HWLocal = mSTM->device()->usesHardware(AMDILDeviceInfo::LocalMem);
  if (!HWLocal || !isHardwareLocal(MI)) {
    return expandGlobalStore(MI);
  }
  uint32_t lID = getPointerID(MI);
  assert(lID && "Found a LDS store that was incorrectly marked as zero ID!\n");
  if (!lID) {
    lID = mSTM->device()->getResourceID(AMDILDevice::LDS_ID);
    mMFI->addErrorMsg(amd::CompilerWarningMessage[RECOVERABLE_ERROR]);
  }
  DebugLoc DL = MI->getDebugLoc();
  // These instructions go before the current MI.
  expandStoreSetupCode(MI);
  BuildMI(*mBB, MI, DL, mTII->get(AMDIL::LDSSTOREVEC), AMDIL::MEM)
    .addReg(AMDIL::R1010)
    .addReg(AMDIL::R1011)
    .addImm(lID);
}

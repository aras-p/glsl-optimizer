//===-- AMDILEGIOExpansion.cpp - TODO: Add brief description -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//
// @file AMDILEGIOExpansion.cpp
// @details Implementation of IO expansion class for evergreen and NI devices.
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
#include "llvm/ADT/StringExtras.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Support/DebugLoc.h"
#include "llvm/Value.h"

using namespace llvm;
AMDILEGIOExpansion::AMDILEGIOExpansion(TargetMachine &tm
     AMDIL_OPT_LEVEL_DECL) : AMDILImageExpansion(tm AMDIL_OPT_LEVEL_VAR)
{
}

AMDILEGIOExpansion::~AMDILEGIOExpansion() {
}
const char *AMDILEGIOExpansion::getPassName() const
{
  return "AMDIL EG/NI IO Expansion Pass";
}
  bool
AMDILEGIOExpansion::isImageIO(MachineInstr *MI)
{
  if (!MI->getOperand(0).isGlobal()) {
    return false;
  }
  const llvm::StringRef& nameRef = MI->getOperand(0).getGlobal()->getName();
  const char *name = nameRef.data();
  if (nameRef.size() > 8 && !strncmp(name, "__amdil_", 8)) {
    name += 8;
    if (!strncmp(name, "sample_data", 11)
        || !strncmp(name, "write_image", 11)
        || !strncmp(name, "get_image2d_params", 18)
        || !strncmp(name, "get_image3d_params", 18)) {
      return true;
    }
  }
  return false;
}
bool
AMDILEGIOExpansion::isIOInstruction(MachineInstr *MI)
{
  if (!MI) {
    return false;
  }
  switch (MI->getOpcode()) {
    default:
      return AMDILIOExpansion::isIOInstruction(MI);
    case AMDIL::IMAGE2D_READ:
    case AMDIL::IMAGE2D_READ_UNNORM:
    case AMDIL::IMAGE2D_WRITE:
    case AMDIL::IMAGE2D_INFO0:
    case AMDIL::IMAGE2D_INFO1:
    case AMDIL::IMAGE3D_READ:
    case AMDIL::IMAGE3D_READ_UNNORM:
    case AMDIL::IMAGE3D_WRITE:
    case AMDIL::IMAGE3D_INFO0:
    case AMDIL::IMAGE3D_INFO1:
      return true;
  };
  return false;
}
void 
AMDILEGIOExpansion::expandIOInstruction(MachineInstr *MI)
{
  assert(isIOInstruction(MI) && "Must be an IO instruction to "
      "be passed to this function!");
  switch (MI->getOpcode()) {
    default:
      AMDILIOExpansion::expandIOInstruction(MI);
      break;
    case AMDIL::IMAGE2D_READ:
    case AMDIL::IMAGE3D_READ:
    case AMDIL::IMAGE2D_READ_UNNORM:
    case AMDIL::IMAGE3D_READ_UNNORM:
        expandImageLoad(mBB, MI);
        break;
    case AMDIL::IMAGE2D_WRITE:
    case AMDIL::IMAGE3D_WRITE:
        expandImageStore(mBB, MI);
        break;
    case AMDIL::IMAGE2D_INFO0:
    case AMDIL::IMAGE2D_INFO1:
    case AMDIL::IMAGE3D_INFO0:
    case AMDIL::IMAGE3D_INFO1:
        expandImageParam(mBB, MI);
        break;
  };
}
  bool
AMDILEGIOExpansion::isCacheableOp(MachineInstr *MI)
{
  AMDILAS::InstrResEnc curRes;
  getAsmPrinterFlags(MI, curRes);
  // We only support caching on UAV11 - JeffG
  if (curRes.bits.ResourceID == 11) {
    return curRes.bits.CacheableRead;
  } else {
    return false;
  }
}
  bool
AMDILEGIOExpansion::isArenaOp(MachineInstr *MI)
{
  AMDILAS::InstrResEnc curRes;
  getAsmPrinterFlags(MI, curRes);
  return curRes.bits.ResourceID 
    == mSTM->device()->getResourceID(AMDILDevice::ARENA_UAV_ID)
    || curRes.bits.ResourceID >= ARENA_SEGMENT_RESERVED_UAVS;
}
  void
AMDILEGIOExpansion::expandPackedData(MachineInstr *MI)
{
  MachineBasicBlock::iterator I = *MI;
  if (!isPackedData(MI)) {
    return;
  }
  // There is a bug in the CAL compiler that incorrectly
  // errors when the UBIT_INSERT instruction is 
  if (mSTM->calVersion() < CAL_VERSION_SC_137) {
    AMDIL789IOExpansion::expandPackedData(MI);
    return;
  }
  DebugLoc DL;
  // If we have packed data, then the shift size is no longer
  // the same as the load size and we need to adjust accordingly
  switch(getPackedID(MI)) {
    default:
      break;
    case PACK_V2I8:
      {
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::LHI), AMDIL::R1012)
          .addReg(AMDIL::R1011);
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::UBIT_INSERT_i32), AMDIL::R1011)
          .addImm(mMFI->addi32Literal(8)).addImm(mMFI->addi32Literal(8))
          .addReg(AMDIL::R1012).addReg(AMDIL::R1011);
      }
      break;
    case PACK_V4I8:
      {
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::LHI_v2i64), AMDIL::R1012)
          .addReg(AMDIL::R1011);
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::LLO_v2i64), AMDIL::R1011)
          .addReg(AMDIL::R1011);
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::UBIT_INSERT_v2i32), 
            AMDIL::R1011)
          .addImm(mMFI->addi64Literal(8ULL | (8ULL << 32)))
          .addImm(mMFI->addi64Literal(8ULL | (8ULL << 32)))
          .addReg(AMDIL::R1012).addReg(AMDIL::R1011);
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::LHI), AMDIL::R1012)
          .addReg(AMDIL::R1011);
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::UBIT_INSERT_i32), AMDIL::R1011)
          .addImm(mMFI->addi32Literal(16)).addImm(mMFI->addi32Literal(16))
          .addReg(AMDIL::R1012).addReg(AMDIL::R1011);
      }
      break;
    case PACK_V2I16:
      {
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::LHI), AMDIL::R1012)
          .addReg(AMDIL::R1011);
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::UBIT_INSERT_i32), AMDIL::R1011)
          .addImm(mMFI->addi32Literal(16)).addImm(mMFI->addi32Literal(16))
          .addReg(AMDIL::R1012).addReg(AMDIL::R1011);
      }
      break;
    case PACK_V4I16:
      {
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::LHI_v2i64), AMDIL::R1012)
          .addReg(AMDIL::R1011);
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::LLO_v2i64), AMDIL::R1011)
          .addReg(AMDIL::R1011);
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::UBIT_INSERT_v2i32), AMDIL::R1011)
          .addImm(mMFI->addi64Literal(16ULL | (16ULL << 32)))
          .addImm(mMFI->addi64Literal(16ULL | (16ULL << 32)))
          .addReg(AMDIL::R1012).addReg(AMDIL::R1011);
      }
      break;
    case UNPACK_V2I8:
    case UNPACK_V4I8:
    case UNPACK_V2I16:
    case UNPACK_V4I16:
      AMDIL789IOExpansion::expandPackedData(MI);
      break;
  };
}

  void
AMDILEGIOExpansion::expandGlobalLoad(MachineInstr *MI)
{
  MachineBasicBlock::iterator I = *MI;
  bool usesArena = isArenaOp(MI);
  bool cacheable = isCacheableOp(MI);
  uint32_t ID = getPointerID(MI);
  mKM->setOutputInst();
  if (!mMFI->usesMem(AMDILDevice::RAW_UAV_ID)
      && !mMFI->usesMem(AMDILDevice::ARENA_UAV_ID)
      && mKM->isKernel()) {
    mMFI->addErrorMsg(amd::CompilerErrorMessage[MEMOP_NO_ALLOCATION]);
  }
  // These instructions are generated before the current MI.
  expandLoadStartCode(MI);
  expandArenaSetup(MI);
  DebugLoc DL;
  if (getMemorySize(MI) == 1) {
      if (usesArena) {
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::UAVARENALOAD_i8), AMDIL::R1011)
          .addReg(AMDIL::R1010)
          .addImm(ID);
      } else {
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1008)
          .addReg(AMDIL::R1010)
          .addImm(mMFI->addi32Literal(3));
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1010)
          .addReg(AMDIL::R1010)
          .addImm(mMFI->addi32Literal(0xFFFFFFFC));
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::VCREATE_v4i32), AMDIL::R1008)
          .addReg(AMDIL::R1008);
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::ADD_v4i32), AMDIL::R1008)
          .addReg(AMDIL::R1008)
          .addImm(mMFI->addi128Literal(0xFFFFFFFFULL << 32, 
                (0xFFFFFFFEULL | (0xFFFFFFFDULL << 32))));
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::IEQ_v4i32), AMDIL::R1012)
          .addReg(AMDIL::R1008)
          .addImm(mMFI->addi32Literal(0));
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::CMOVLOG_i32), AMDIL::R1008)
          .addReg(AMDIL::R1012)
          .addImm(mMFI->addi32Literal(0))
          .addImm(mMFI->addi32Literal(24));
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::CMOVLOG_Y_i32), AMDIL::R1008)
          .addReg(AMDIL::R1012)
          .addImm(mMFI->addi32Literal(8))
          .addReg(AMDIL::R1008);
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::CMOVLOG_Z_i32), AMDIL::R1008)
          .addReg(AMDIL::R1012)
          .addImm(mMFI->addi32Literal(16))
          .addReg(AMDIL::R1008);
        if (cacheable) {
          BuildMI(*mBB, I, DL, mTII->get(AMDIL::UAVRAWLOADCACHED_i32),
              AMDIL::R1011).addReg(AMDIL::R1010).addImm(ID);
        } else {
          BuildMI(*mBB, I, DL, mTII->get(AMDIL::UAVRAWLOAD_i32),
              AMDIL::R1011).addReg(AMDIL::R1010).addImm(ID);
        }
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::SHR_v4i8), AMDIL::R1011)
          .addReg(AMDIL::R1011)
          .addReg(AMDIL::R1008);
      }
  } else if (getMemorySize(MI) == 2) {
      if (usesArena) {
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::UAVARENALOAD_i16), AMDIL::R1011)
          .addReg(AMDIL::R1010)
          .addImm(ID);
      } else {
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1008)
          .addReg(AMDIL::R1010)
          .addImm(mMFI->addi32Literal(3));
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::SHR_i32), AMDIL::R1008)
          .addReg(AMDIL::R1008)
          .addImm(mMFI->addi32Literal(1));
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1010)
          .addReg(AMDIL::R1010)
          .addImm(mMFI->addi32Literal(0xFFFFFFFC));
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::CMOVLOG_i32), AMDIL::R1008)
          .addReg(AMDIL::R1008)
          .addImm(mMFI->addi32Literal(16))
          .addImm(mMFI->addi32Literal(0));
        if (cacheable) {
          BuildMI(*mBB, I, DL, mTII->get(AMDIL::UAVRAWLOADCACHED_i32),
              AMDIL::R1011).addReg(AMDIL::R1010).addImm(ID);
        } else {
          BuildMI(*mBB, I, DL, mTII->get(AMDIL::UAVRAWLOAD_i32),
              AMDIL::R1011).addReg(AMDIL::R1010).addImm(ID);
        }
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::SHR_i16), AMDIL::R1011)
          .addReg(AMDIL::R1011)
          .addReg(AMDIL::R1008);
      }
  } else if (getMemorySize(MI) == 4) {
      if (usesArena) {
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::UAVARENALOAD_i32), AMDIL::R1011)
          .addReg(AMDIL::R1010)
          .addImm(ID);
      } else {
        if (cacheable) {
          BuildMI(*mBB, I, DL, mTII->get(AMDIL::UAVRAWLOADCACHED_i32),
              AMDIL::R1011).addReg(AMDIL::R1010).addImm(ID);
        } else {
          BuildMI(*mBB, I, DL, mTII->get(AMDIL::UAVRAWLOAD_i32),
              AMDIL::R1011).addReg(AMDIL::R1010).addImm(ID);
        }
      }
  } else if (getMemorySize(MI) == 8) {
    if (usesArena) {
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::UAVARENALOAD_i32), AMDIL::R1011)
          .addReg(AMDIL::R1010)
          .addImm(ID);
        if (mSTM->device()->usesHardware(AMDILDeviceInfo::ArenaVectors)) {
          BuildMI(*mBB, I, DL, mTII->get(AMDIL::UAVARENALOAD_Y_i32), AMDIL::R1011)
            .addReg(AMDIL::R1010)
            .addImm(ID);
        } else {
          BuildMI(*mBB, I, DL, mTII->get(AMDIL::VEXTRACT_v4i32), AMDIL::R1007)
            .addReg(AMDIL::R1010)
            .addImm(2);
          BuildMI(*mBB, I, DL, mTII->get(AMDIL::UAVARENALOAD_i32), AMDIL::R1008)
            .addReg(AMDIL::R1007)
            .addImm(ID);
          BuildMI(*mBB, I, DL, mTII->get(AMDIL::LCREATE), AMDIL::R1011)
            .addReg(AMDIL::R1011)
            .addReg(AMDIL::R1008);
        }
      } else {
        if (cacheable) {
          BuildMI(*mBB, I, DL, mTII->get(AMDIL::UAVRAWLOADCACHED_v2i32),
              AMDIL::R1011).addReg(AMDIL::R1010).addImm(ID);
        } else {
          BuildMI(*mBB, I, DL, mTII->get(AMDIL::UAVRAWLOAD_v2i32),
              AMDIL::R1011).addReg(AMDIL::R1010).addImm(ID);
        }
      }
  } else {
      if (usesArena) {
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::UAVARENALOAD_i32), AMDIL::R1011)
          .addReg(AMDIL::R1010)
          .addImm(ID);
        if (mSTM->device()->usesHardware(AMDILDeviceInfo::ArenaVectors)) {
          BuildMI(*mBB, I, DL, mTII->get(AMDIL::UAVARENALOAD_Y_i32), AMDIL::R1011)
            .addReg(AMDIL::R1010)
            .addImm(ID);
          BuildMI(*mBB, I, DL, mTII->get(AMDIL::UAVARENALOAD_Z_i32), AMDIL::R1011)
            .addReg(AMDIL::R1010)
            .addImm(ID);
          BuildMI(*mBB, I, DL, mTII->get(AMDIL::UAVARENALOAD_W_i32), AMDIL::R1011)
            .addReg(AMDIL::R1010)
            .addImm(ID);
        } else {
          BuildMI(*mBB, I, DL, mTII->get(AMDIL::VEXTRACT_v4i32), AMDIL::R1007)
            .addReg(AMDIL::R1010)
            .addImm(2);
          BuildMI(*mBB, I, DL, mTII->get(AMDIL::UAVARENALOAD_i32), AMDIL::R1008)
            .addReg(AMDIL::R1007)
            .addImm(ID);
          BuildMI(*mBB, I, DL, mTII->get(AMDIL::LCREATE), AMDIL::R1011)
            .addReg(AMDIL::R1011)
            .addReg(AMDIL::R1008);
          BuildMI(*mBB, I, DL, mTII->get(AMDIL::VEXTRACT_v4i32), AMDIL::R1007)
            .addReg(AMDIL::R1010)
            .addImm(3);
          BuildMI(*mBB, I, DL, mTII->get(AMDIL::UAVARENALOAD_i32), AMDIL::R1008)
            .addReg(AMDIL::R1007)
            .addImm(ID);
          BuildMI(*mBB, I, DL, mTII->get(AMDIL::VEXTRACT_v4i32), AMDIL::R1007)
            .addReg(AMDIL::R1010)
            .addImm(4);
          BuildMI(*mBB, I, DL, mTII->get(AMDIL::UAVARENALOAD_i32), AMDIL::R1006)
            .addReg(AMDIL::R1007)
            .addImm(ID);
          BuildMI(*mBB, I, DL, mTII->get(AMDIL::LCREATE), AMDIL::R1008)
            .addReg(AMDIL::R1006)
            .addReg(AMDIL::R1008);
          BuildMI(*mBB, I, DL, mTII->get(AMDIL::LCREATE_v2i64), AMDIL::R1011)
            .addReg(AMDIL::R1011)
            .addReg(AMDIL::R1008);
        }
      } else {
        if (cacheable) {
          BuildMI(*mBB, I, DL, mTII->get(AMDIL::UAVRAWLOADCACHED_v4i32),
              AMDIL::R1011).addReg(AMDIL::R1010).addImm(ID);
        } else {
          BuildMI(*mBB, I, DL, mTII->get(AMDIL::UAVRAWLOAD_v4i32),
              AMDIL::R1011).addReg(AMDIL::R1010).addImm(ID);
        }
      }
  }
  // These instructions are generated after the current MI.
  expandPackedData(MI);
  expandExtendLoad(MI);
  BuildMI(*mBB, I, MI->getDebugLoc(),
      mTII->get(getMoveInstFromID(
          MI->getDesc().OpInfo[0].RegClass)))
    .addOperand(MI->getOperand(0))
    .addReg(AMDIL::R1011);
  MI->getOperand(0).setReg(AMDIL::R1011);
}

  void
AMDILEGIOExpansion::expandRegionLoad(MachineInstr *MI)
{
  MachineBasicBlock::iterator I = *MI;
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
  DebugLoc DL;
  unsigned mulOp = 0;
  uint32_t gID = getPointerID(MI);
  assert(gID && "Found a GDS load that was incorrectly marked as zero ID!\n");
  if (!gID) {
    gID = mSTM->device()->getResourceID(AMDILDevice::GDS_ID);
    mMFI->addErrorMsg(amd::CompilerWarningMessage[RECOVERABLE_ERROR]);
  }
  // These instructions are generated before the current MI.
  expandLoadStartCode(MI);
  switch (getMemorySize(MI)) {
    default:
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::VCREATE_v4i32), AMDIL::R1010)
        .addReg(AMDIL::R1010);
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::ADD_v4i32), AMDIL::R1010)
        .addReg(AMDIL::R1010)
        .addImm(mMFI->addi128Literal(1ULL << 32, 2ULL | (3ULL << 32)));
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::GDSLOAD), AMDIL::R1011)
        .addReg(AMDIL::R1010)
        .addImm(gID);
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::GDSLOAD_Y), AMDIL::R1011)
        .addReg(AMDIL::R1010)
        .addImm(gID);
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::GDSLOAD_Z), AMDIL::R1011)
        .addReg(AMDIL::R1010)
        .addImm(gID);
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::GDSLOAD_W), AMDIL::R1011)
        .addReg(AMDIL::R1010)
        .addImm(gID);
      break;
    case 1:
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1008)
        .addReg(AMDIL::R1010)
        .addImm(mMFI->addi32Literal(3));
      mulOp = (mSTM->device()->usesSoftware(AMDILDeviceInfo::RegionMem))
        ? AMDIL::UMUL_i32 : AMDIL::UMUL24_i32;
      BuildMI(*mBB, I, DL, mTII->get(mulOp), AMDIL::R1008)
        .addReg(AMDIL::R1008)
        .addImm(mMFI->addi32Literal(8));
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1010)
        .addReg(AMDIL::R1010)
        .addImm(mMFI->addi32Literal(0xFFFFFFFC));
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::GDSLOAD), AMDIL::R1011)
        .addReg(AMDIL::R1010)
        .addImm(gID);
      // The instruction would normally fit in right here so everything created
      // after this point needs to go into the afterInst vector.
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::IBIT_EXTRACT_i32), AMDIL::R1011)
        .addImm(mMFI->addi32Literal(8))
        .addReg(AMDIL::R1008)
        .addReg(AMDIL::R1011);
      break;
    case 2:
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1008)
        .addReg(AMDIL::R1010)
        .addImm(mMFI->addi32Literal(3));
      mulOp = (mSTM->device()->usesSoftware(AMDILDeviceInfo::RegionMem))
        ? AMDIL::UMUL_i32 : AMDIL::UMUL24_i32;
      BuildMI(*mBB, I, DL, mTII->get(mulOp), AMDIL::R1008)
        .addReg(AMDIL::R1008)
        .addImm(mMFI->addi32Literal(8));
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1010)
        .addReg(AMDIL::R1010)
        .addImm(mMFI->addi32Literal(0xFFFFFFFC));
       BuildMI(*mBB, I, DL, mTII->get(AMDIL::GDSLOAD), AMDIL::R1011)
        .addReg(AMDIL::R1010)
        .addImm(gID);
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::IBIT_EXTRACT_i32), AMDIL::R1011)
        .addImm(mMFI->addi32Literal(16))
        .addReg(AMDIL::R1008)
        .addReg(AMDIL::R1011);
      break;
    case 4:
       BuildMI(*mBB, I, DL, mTII->get(AMDIL::GDSLOAD), AMDIL::R1011)
        .addReg(AMDIL::R1010)
        .addImm(gID);
      break;
    case 8:
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::VCREATE_v2i32), AMDIL::R1010)
        .addReg(AMDIL::R1010);
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::ADD_v4i32), AMDIL::R1010)
        .addReg(AMDIL::R1010)
        .addImm(mMFI->addi64Literal(1ULL << 32));
       BuildMI(*mBB, I, DL, mTII->get(AMDIL::GDSLOAD), AMDIL::R1011)
        .addReg(AMDIL::R1010)
        .addImm(gID);
       BuildMI(*mBB, I, DL, mTII->get(AMDIL::GDSLOAD_Y), AMDIL::R1011)
        .addReg(AMDIL::R1010)
        .addImm(gID);
      break;
   };

  // These instructions are generated after the current MI.
  expandPackedData(MI);
  expandExtendLoad(MI);
  BuildMI(*mBB, I, MI->getDebugLoc(),
      mTII->get(getMoveInstFromID(
          MI->getDesc().OpInfo[0].RegClass)))
    .addOperand(MI->getOperand(0))
    .addReg(AMDIL::R1011);
  MI->getOperand(0).setReg(AMDIL::R1011);
}
  void
AMDILEGIOExpansion::expandLocalLoad(MachineInstr *MI)
{
  MachineBasicBlock::iterator I = *MI;
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
  unsigned mulOp = 0;
  // These instructions are generated before the current MI.
  expandLoadStartCode(MI);
  switch (getMemorySize(MI)) {
    default:
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::LDSLOADVEC_v4i32), AMDIL::R1011) 
        .addReg(AMDIL::R1010)
        .addImm(lID);
      break;
    case 8:
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::LDSLOADVEC_v2i32), AMDIL::R1011) 
        .addReg(AMDIL::R1010)
        .addImm(lID);
      break;
    case 4:
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::LDSLOAD), AMDIL::R1011) 
        .addReg(AMDIL::R1010)
        .addImm(lID);
      break;
    case 1:
      if (!mSTM->device()->usesHardware(AMDILDeviceInfo::ByteLDSOps)) {
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1008)
          .addReg(AMDIL::R1010)
          .addImm(mMFI->addi32Literal(3));
        mulOp = (mSTM->device()->usesSoftware(AMDILDeviceInfo::LocalMem))
          ? AMDIL::UMUL_i32 : AMDIL::UMUL24_i32;
        BuildMI(*mBB, I, DL, mTII->get(mulOp), AMDIL::R1008)
          .addReg(AMDIL::R1008)
          .addImm(mMFI->addi32Literal(8));
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1010)
          .addReg(AMDIL::R1010)
          .addImm(mMFI->addi32Literal(0xFFFFFFFC));
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::LDSLOAD), AMDIL::R1011) 
          .addReg(AMDIL::R1010)
          .addImm(lID);
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::IBIT_EXTRACT_i32), AMDIL::R1011)
          .addImm(mMFI->addi32Literal(8))
          .addReg(AMDIL::R1008)
          .addReg(AMDIL::R1011);
      } else {
        if (isSWSExtLoadInst(MI)) { 
          BuildMI(*mBB, I, DL, mTII->get(AMDIL::LDSLOAD_i8), AMDIL::R1011) 
            .addReg(AMDIL::R1010)
            .addImm(lID);
        } else {
          BuildMI(*mBB, I, DL, mTII->get(AMDIL::LDSLOAD_u8), AMDIL::R1011) 
            .addReg(AMDIL::R1010)
            .addImm(lID);
        }
      }
      break;
    case 2:
      if (!mSTM->device()->usesHardware(AMDILDeviceInfo::ByteLDSOps)) {
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1008)
          .addReg(AMDIL::R1010)
          .addImm(mMFI->addi32Literal(3));
        mulOp = (mSTM->device()->usesSoftware(AMDILDeviceInfo::LocalMem))
          ? AMDIL::UMUL_i32 : AMDIL::UMUL24_i32;
        BuildMI(*mBB, I, DL, mTII->get(mulOp), AMDIL::R1008)
          .addReg(AMDIL::R1008)
          .addImm(mMFI->addi32Literal(8));
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1010)
          .addReg(AMDIL::R1010)
          .addImm(mMFI->addi32Literal(0xFFFFFFFC));
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::LDSLOAD), AMDIL::R1011) 
          .addReg(AMDIL::R1010)
          .addImm(lID);
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::IBIT_EXTRACT_i32), AMDIL::R1011)
          .addImm(mMFI->addi32Literal(16))
          .addReg(AMDIL::R1008)
          .addReg(AMDIL::R1011);
      } else {
        if (isSWSExtLoadInst(MI)) { 
           BuildMI(*mBB, I, DL, mTII->get(AMDIL::LDSLOAD_i16), AMDIL::R1011) 
        .addReg(AMDIL::R1010)
        .addImm(lID);
        } else {
           BuildMI(*mBB, I, DL, mTII->get(AMDIL::LDSLOAD_u16), AMDIL::R1011) 
        .addReg(AMDIL::R1010)
        .addImm(lID);
        }
      }
      break;
   }

  // These instructions are generated after the current MI.
  expandPackedData(MI);
  expandExtendLoad(MI);
  BuildMI(*mBB, I, MI->getDebugLoc(),
      mTII->get(getMoveInstFromID(
          MI->getDesc().OpInfo[0].RegClass)))
    .addOperand(MI->getOperand(0))
    .addReg(AMDIL::R1011);
  MI->getOperand(0).setReg(AMDIL::R1011);
}
  void
AMDILEGIOExpansion::expandGlobalStore(MachineInstr *MI)
{
  MachineBasicBlock::iterator I = *MI;
  bool usesArena = isArenaOp(MI);
  uint32_t ID = getPointerID(MI);
  mKM->setOutputInst();
  if (!mMFI->usesMem(AMDILDevice::RAW_UAV_ID)
      && !mMFI->usesMem(AMDILDevice::ARENA_UAV_ID)
      && mKM->isKernel()) {
    mMFI->addErrorMsg(amd::CompilerErrorMessage[MEMOP_NO_ALLOCATION]);
  }
  DebugLoc DL;
  // These instructions are expandted before the current MI.
  expandStoreSetupCode(MI);
  expandArenaSetup(MI);
  switch (getMemorySize(MI)) {
    default:
      if (usesArena) {
         BuildMI(*mBB, I, DL, mTII->get(AMDIL::UAVARENASTORE_i32), AMDIL::R1010)
          .addReg(AMDIL::R1011)
          .addImm(ID);
        if (mSTM->device()->usesHardware(AMDILDeviceInfo::ArenaVectors)) {
          BuildMI(*mBB, I, DL, mTII->get(AMDIL::UAVARENASTORE_Y_i32), AMDIL::R1010)
            .addReg(AMDIL::R1011)
            .addImm(ID);
          BuildMI(*mBB, I, DL, mTII->get(AMDIL::UAVARENASTORE_Z_i32), AMDIL::R1010)
            .addReg(AMDIL::R1011)
            .addImm(ID);
          BuildMI(*mBB, I, MI->getDebugLoc(), mTII->get(AMDIL::UAVARENASTORE_W_i32), AMDIL::R1010)
            .addReg(AMDIL::R1011)
            .addImm(ID);
        } else {
          BuildMI(*mBB, I, DL, mTII->get(AMDIL::VEXTRACT_v4i32), AMDIL::R1007)
            .addReg(AMDIL::R1010)
            .addImm(2);
          BuildMI(*mBB, I, DL, mTII->get(AMDIL::VEXTRACT_v4i32), AMDIL::R1008)
            .addReg(AMDIL::R1011)
            .addImm(2);
          BuildMI(*mBB, I, DL, mTII->get(AMDIL::UAVARENASTORE_i32), AMDIL::R1008)
            .addReg(AMDIL::R1008)
            .addImm(ID);
          BuildMI(*mBB, I, DL, mTII->get(AMDIL::VEXTRACT_v4i32), AMDIL::R1007)
            .addReg(AMDIL::R1010)
            .addImm(3);
          BuildMI(*mBB, I, DL, mTII->get(AMDIL::VEXTRACT_v4i32), AMDIL::R1008)
            .addReg(AMDIL::R1011)
            .addImm(3);
          BuildMI(*mBB, I, DL, mTII->get(AMDIL::UAVARENASTORE_i32), AMDIL::R1008)
            .addReg(AMDIL::R1008)
            .addImm(ID);
          BuildMI(*mBB, I, DL, mTII->get(AMDIL::VEXTRACT_v4i32), AMDIL::R1007)
            .addReg(AMDIL::R1010)
            .addImm(4);
          BuildMI(*mBB, I, DL, mTII->get(AMDIL::VEXTRACT_v4i32), AMDIL::R1008)
            .addReg(AMDIL::R1011)
            .addImm(4);
          BuildMI(*mBB, I, MI->getDebugLoc(), mTII->get(AMDIL::UAVARENASTORE_i32), AMDIL::R1008)
            .addReg(AMDIL::R1008)
            .addImm(ID);
        }
      } else {
        BuildMI(*mBB, I, MI->getDebugLoc(), mTII->get(AMDIL::UAVRAWSTORE_v4i32), AMDIL::MEM)
          .addReg(AMDIL::R1010)
          .addReg(AMDIL::R1011)
          .addImm(ID);
      }
      break;
    case 1:
      if (usesArena) {
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1011)
          .addReg(AMDIL::R1011)
          .addImm(mMFI->addi32Literal(0xFF));
        BuildMI(*mBB, I, MI->getDebugLoc(), mTII->get(AMDIL::UAVARENASTORE_i8), AMDIL::R1010)
          .addReg(AMDIL::R1011)
          .addImm(ID);
      } else {
        BuildMI(*mBB, I, MI->getDebugLoc(), mTII->get(AMDIL::UAVRAWSTORE_i32), AMDIL::MEM)
          .addReg(AMDIL::R1010)
          .addReg(AMDIL::R1011)
          .addImm(ID);
      }
      break;
    case 2:
      if (usesArena) {
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1011)
          .addReg(AMDIL::R1011)
          .addImm(mMFI->addi32Literal(0xFFFF));
        BuildMI(*mBB, I, MI->getDebugLoc(), mTII->get(AMDIL::UAVARENASTORE_i16), AMDIL::R1010)
          .addReg(AMDIL::R1011)
          .addImm(ID);
      } else {
        BuildMI(*mBB, I, MI->getDebugLoc(), mTII->get(AMDIL::UAVRAWSTORE_i32), AMDIL::MEM)
          .addReg(AMDIL::R1010)
          .addReg(AMDIL::R1011)
          .addImm(ID);
      }
      break;
    case 4:
      if (usesArena) {
        BuildMI(*mBB, I, MI->getDebugLoc(), mTII->get(AMDIL::UAVARENASTORE_i32), AMDIL::R1010)
          .addReg(AMDIL::R1011)
          .addImm(ID);
      } else {
        BuildMI(*mBB, I, MI->getDebugLoc(), mTII->get(AMDIL::UAVRAWSTORE_i32), AMDIL::MEM)
          .addReg(AMDIL::R1010)
          .addReg(AMDIL::R1011)
          .addImm(ID);
      }
      break;
    case 8:
      if (usesArena) {
        BuildMI(*mBB, I, MI->getDebugLoc(), mTII->get(AMDIL::UAVARENASTORE_i32), AMDIL::R1010)
          .addReg(AMDIL::R1011)
          .addImm(ID);
        if (mSTM->device()->usesHardware(AMDILDeviceInfo::ArenaVectors)) {
          BuildMI(*mBB, I, MI->getDebugLoc(), mTII->get(AMDIL::UAVARENASTORE_Y_i32), AMDIL::R1010)
            .addReg(AMDIL::R1011)
            .addImm(ID);
        } else {
           BuildMI(*mBB, I, DL, mTII->get(AMDIL::VEXTRACT_v4i32), AMDIL::R1007)
            .addReg(AMDIL::R1010)
            .addImm(2);
          BuildMI(*mBB, I, DL, mTII->get(AMDIL::VEXTRACT_v4i32), AMDIL::R1008)
            .addReg(AMDIL::R1011)
            .addImm(2);
          BuildMI(*mBB, I, MI->getDebugLoc(), mTII->get(AMDIL::UAVARENASTORE_i32), AMDIL::R1007)
            .addReg(AMDIL::R1008)
            .addImm(ID);
        }
      } else {
        BuildMI(*mBB, I, MI->getDebugLoc(), mTII->get(AMDIL::UAVRAWSTORE_v2i32), AMDIL::MEM)
          .addReg(AMDIL::R1010)
          .addReg(AMDIL::R1011)
          .addImm(ID);
      }
      break;
  };
}
  void
AMDILEGIOExpansion::expandRegionStore(MachineInstr *MI)
{
  MachineBasicBlock::iterator I = *MI;
  bool HWRegion = mSTM->device()->usesHardware(AMDILDeviceInfo::RegionMem);
  if (!HWRegion || !isHardwareRegion(MI)) {
    return expandGlobalStore(MI);
  }
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
  DebugLoc DL;
  unsigned mulOp = HWRegion ? AMDIL::UMUL24_i32 : AMDIL::UMUL24_i32;
  // These instructions are expandted before the current MI.
  expandStoreSetupCode(MI);
  expandArenaSetup(MI);
  switch (getMemorySize(MI)) {
    default:
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::VCREATE_v4i32), AMDIL::R1010)
        .addReg(AMDIL::R1010);
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::ADD_v4i32), AMDIL::R1010)
        .addReg(AMDIL::R1010)
        .addImm(mMFI->addi128Literal(1ULL << 32, 2ULL | (3ULL << 32)));
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::GDSSTORE), AMDIL::R1010)
        .addReg(AMDIL::R1011)
        .addImm(gID);
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::GDSSTORE_Y), AMDIL::R1010)
        .addReg(AMDIL::R1011)
        .addImm(gID);
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::GDSSTORE_Z), AMDIL::R1010)
        .addReg(AMDIL::R1011)
        .addImm(gID);
      BuildMI(*mBB, I, MI->getDebugLoc(), mTII->get(AMDIL::GDSSTORE_W), AMDIL::R1010)
        .addReg(AMDIL::R1011)
        .addImm(gID);
      break;
    case 1:
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1011)
        .addReg(AMDIL::R1011)
        .addImm(mMFI->addi32Literal(0xFF));
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1012)
        .addReg(AMDIL::R1010)
        .addImm(mMFI->addi32Literal(3));
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::VCREATE_v4i32), AMDIL::R1008)
        .addReg(AMDIL::R1008);
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::ADD_v4i32), AMDIL::R1008)
        .addReg(AMDIL::R1008)
        .addImm(mMFI->addi128Literal(0xFFFFFFFFULL << 32, 
              (0xFFFFFFFEULL | (0xFFFFFFFDULL << 32))));
      BuildMI(*mBB, I, DL, mTII->get(mulOp), AMDIL::R1006)
        .addReg(AMDIL::R1008)
        .addImm(mMFI->addi32Literal(8));
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::CMOVLOG_i32), AMDIL::R1007)
        .addReg(AMDIL::R1008)
        .addImm(mMFI->addi32Literal(0xFFFFFF00))
        .addImm(mMFI->addi32Literal(0x00FFFFFF));
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::CMOVLOG_Y_i32), AMDIL::R1007)
        .addReg(AMDIL::R1008)
        .addReg(AMDIL::R1007)
        .addImm(mMFI->addi32Literal(0xFF00FFFF));
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::CMOVLOG_Z_i32), AMDIL::R1012)
        .addReg(AMDIL::R1008)
        .addReg(AMDIL::R1007)
        .addImm(mMFI->addi32Literal(0xFFFF00FF));
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::SHL_i32), AMDIL::R1011)
        .addReg(AMDIL::R1011)
        .addReg(AMDIL::R1007);
      BuildMI(*mBB, I, MI->getDebugLoc(), mTII->get(AMDIL::ATOM_R_MSKOR), AMDIL::R1010)
        .addReg(AMDIL::R1012)
        .addReg(AMDIL::R1011)
        .addImm(gID);
      break;
    case 2:
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1011)
        .addReg(AMDIL::R1011)
        .addImm(mMFI->addi32Literal(0x0000FFFF));
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1008)
        .addReg(AMDIL::R1010)
        .addImm(mMFI->addi32Literal(3));
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::SHR_i32), AMDIL::R1008)
        .addReg(AMDIL::R1008)
        .addImm(mMFI->addi32Literal(1));
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::CMOVLOG_i32), AMDIL::R1012)
        .addReg(AMDIL::R1008)
        .addImm(mMFI->addi32Literal(0x0000FFFF))
        .addImm(mMFI->addi32Literal(0xFFFF0000));
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::CMOVLOG_i32), AMDIL::R1008)
        .addReg(AMDIL::R1008)
        .addImm(mMFI->addi32Literal(16))
        .addImm(mMFI->addi32Literal(0));
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::SHL_i32), AMDIL::R1011)
        .addReg(AMDIL::R1011)
        .addReg(AMDIL::R1008);
      BuildMI(*mBB, I, MI->getDebugLoc(), mTII->get(AMDIL::ATOM_R_MSKOR), AMDIL::R1010)
        .addReg(AMDIL::R1012)
        .addReg(AMDIL::R1011)
        .addImm(gID);
      break;
    case 4:
      BuildMI(*mBB, I, MI->getDebugLoc(), mTII->get(AMDIL::GDSSTORE), AMDIL::R1010)
        .addReg(AMDIL::R1011)
        .addImm(gID);
      break;
    case 8:
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::VCREATE_v2i32), AMDIL::R1010)
        .addReg(AMDIL::R1010);
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::ADD_v4i32), AMDIL::R1010)
        .addReg(AMDIL::R1010)
        .addImm(mMFI->addi64Literal(1ULL << 32));
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::GDSSTORE), AMDIL::R1010)
        .addReg(AMDIL::R1011)
        .addImm(gID);
      BuildMI(*mBB, I, MI->getDebugLoc(), mTII->get(AMDIL::GDSSTORE_Y), AMDIL::R1010)
        .addReg(AMDIL::R1011)
        .addImm(gID);
      break;
  };

}

  void
AMDILEGIOExpansion::expandLocalStore(MachineInstr *MI)
{
  MachineBasicBlock::iterator I = *MI;
  bool HWLocal = mSTM->device()->usesHardware(AMDILDeviceInfo::LocalMem);
  if (!HWLocal || !isHardwareLocal(MI)) {
    return expandGlobalStore(MI);
  }
  DebugLoc DL;
  if (!mMFI->usesMem(AMDILDevice::LDS_ID)
      && mKM->isKernel()) {
    mMFI->addErrorMsg(amd::CompilerErrorMessage[MEMOP_NO_ALLOCATION]);
  }
  uint32_t lID = getPointerID(MI);
  assert(lID && "Found a LDS store that was incorrectly marked as zero ID!\n");
  if (!lID) {
    lID = mSTM->device()->getResourceID(AMDILDevice::LDS_ID);
    mMFI->addErrorMsg(amd::CompilerWarningMessage[RECOVERABLE_ERROR]);
  }
  unsigned mulOp = HWLocal ? AMDIL::UMUL24_i32 : AMDIL::UMUL24_i32;
  // These instructions are expandted before the current MI.
  expandStoreSetupCode(MI);
  switch (getMemorySize(MI)) {
    default:
      BuildMI(*mBB, I, MI->getDebugLoc(), mTII->get(AMDIL::LDSSTOREVEC_v4i32), AMDIL::MEM)
        .addReg(AMDIL::R1010)
        .addReg(AMDIL::R1011)
        .addImm(lID);
      break;
    case 8:
      BuildMI(*mBB, I, MI->getDebugLoc(), mTII->get(AMDIL::LDSSTOREVEC_v2i32), AMDIL::MEM)
        .addReg(AMDIL::R1010)
        .addReg(AMDIL::R1011)
        .addImm(lID);
      break;
    case 4:
      BuildMI(*mBB, I, MI->getDebugLoc(), mTII->get(AMDIL::LDSSTORE), AMDIL::R1010)
        .addReg(AMDIL::R1011)
        .addImm(lID);
      break;
    case 1:
      if (!mSTM->device()->usesHardware(AMDILDeviceInfo::ByteLDSOps)) {
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1011)
          .addReg(AMDIL::R1011)
          .addImm(mMFI->addi32Literal(0xFF));
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1012)
          .addReg(AMDIL::R1010)
          .addImm(mMFI->addi32Literal(3));
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::VCREATE_v4i32), AMDIL::R1008)
          .addReg(AMDIL::R1008);
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::ADD_v4i32), AMDIL::R1008)
          .addReg(AMDIL::R1008)
          .addImm(mMFI->addi128Literal(0xFFFFFFFFULL << 32, 
                (0xFFFFFFFEULL | (0xFFFFFFFDULL << 32))));
        BuildMI(*mBB, I, DL, mTII->get(mulOp), AMDIL::R1006)
          .addReg(AMDIL::R1008)
          .addImm(mMFI->addi32Literal(8));
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::CMOVLOG_i32), AMDIL::R1007)
          .addReg(AMDIL::R1008)
          .addImm(mMFI->addi32Literal(0xFFFFFF00))
          .addImm(mMFI->addi32Literal(0x00FFFFFF));
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::CMOVLOG_Y_i32), AMDIL::R1007)
          .addReg(AMDIL::R1008)
          .addReg(AMDIL::R1007)
          .addImm(mMFI->addi32Literal(0xFF00FFFF));
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::CMOVLOG_Z_i32), AMDIL::R1012)
          .addReg(AMDIL::R1008)
          .addReg(AMDIL::R1007)
          .addImm(mMFI->addi32Literal(0xFFFF00FF));
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::SHL_i32), AMDIL::R1011)
          .addReg(AMDIL::R1011)
          .addReg(AMDIL::R1007);
        if (mSTM->calVersion() >= CAL_VERSION_SC_137) {
          BuildMI(*mBB, I, MI->getDebugLoc(), mTII->get(AMDIL::ATOM_L_MSKOR_NORET), 
              AMDIL::R1010)
            .addReg(AMDIL::R1012)
            .addReg(AMDIL::R1011)
            .addImm(lID);
        } else {
          BuildMI(*mBB, I, DL, mTII->get(AMDIL::ATOM_L_ADD_NORET), 
              AMDIL::R1010)
            .addReg(AMDIL::R1012)
            .addImm(lID);
          BuildMI(*mBB, I, MI->getDebugLoc(), mTII->get(AMDIL::ATOM_L_OR_NORET), 
              AMDIL::R1010)
            .addReg(AMDIL::R1011)
            .addImm(lID);
        }
      } else {
        BuildMI(*mBB, I, MI->getDebugLoc(), mTII->get(AMDIL::LDSSTORE_i8), AMDIL::R1010)
          .addReg(AMDIL::R1011)
          .addImm(lID);
      }
      break;
    case 2:
      if (!mSTM->device()->usesHardware(AMDILDeviceInfo::ByteLDSOps)) {
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1011)
          .addReg(AMDIL::R1011)
          .addImm(mMFI->addi32Literal(0x0000FFFF));
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1008)
          .addReg(AMDIL::R1010)
          .addImm(mMFI->addi32Literal(3));
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::SHR_i32), AMDIL::R1008)
          .addReg(AMDIL::R1008)
          .addImm(mMFI->addi32Literal(1));
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::CMOVLOG_i32), AMDIL::R1012)
          .addReg(AMDIL::R1008)
          .addImm(mMFI->addi32Literal(0x0000FFFF))
          .addImm(mMFI->addi32Literal(0xFFFF0000));
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::CMOVLOG_i32), AMDIL::R1008)
          .addReg(AMDIL::R1008)
          .addImm(mMFI->addi32Literal(16))
          .addImm(mMFI->addi32Literal(0));
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::SHL_i32), AMDIL::R1011)
          .addReg(AMDIL::R1011)
          .addReg(AMDIL::R1008);
        if (mSTM->calVersion() >= CAL_VERSION_SC_137) {
          BuildMI(*mBB, I, MI->getDebugLoc(), mTII->get(AMDIL::ATOM_L_MSKOR_NORET), 
              AMDIL::R1010)
            .addReg(AMDIL::R1012)
            .addReg(AMDIL::R1011)
            .addImm(lID);
        } else {
          BuildMI(*mBB, I, DL, mTII->get(AMDIL::ATOM_L_ADD_NORET), 
              AMDIL::R1010)
            .addReg(AMDIL::R1012)
            .addImm(lID);
          BuildMI(*mBB, I, MI->getDebugLoc(), mTII->get(AMDIL::ATOM_L_OR_NORET), 
              AMDIL::R1010)
            .addReg(AMDIL::R1011)
            .addImm(lID);
        }
      } else { 
        BuildMI(*mBB, I, MI->getDebugLoc(), mTII->get(AMDIL::LDSSTORE_i16), AMDIL::R1010)
          .addReg(AMDIL::R1011)
          .addImm(lID);
      }
      break;
  }
}


  void
AMDILEGIOExpansion::expandStoreSetupCode(MachineInstr *MI)
{
  AMDIL789IOExpansion::expandStoreSetupCode(MI);
}
  void
AMDILEGIOExpansion::expandArenaSetup(MachineInstr *MI)
{
  MachineBasicBlock::iterator I = *MI;
  if (!isArenaOp(MI)) {
    return;
  }
  const MCInstrDesc &TID = (MI->getDesc());
  const MCOperandInfo &TOI = TID.OpInfo[0];
  unsigned short RegClass = TOI.RegClass;
  DebugLoc DL;
  switch (RegClass) {
    case AMDIL::GPRV4I16RegClassID:
    case AMDIL::GPRI64RegClassID:
    case AMDIL::GPRF64RegClassID:
    case AMDIL::GPRV2I32RegClassID:
    case AMDIL::GPRV2F32RegClassID:
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::VCREATE_v2i32), AMDIL::R1010)
        .addReg(AMDIL::R1010);
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::ADD_v2i32), AMDIL::R1010)
        .addReg(AMDIL::R1010)
        .addImm(mMFI->addi64Literal(4ULL << 32));
      break;
    default:
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::VCREATE_v4i32), AMDIL::R1010)
        .addReg(AMDIL::R1010);
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::ADD_v4i32), AMDIL::R1010)
        .addReg(AMDIL::R1010)
        .addImm(mMFI->addi128Literal(4ULL << 32, 8ULL | (12ULL << 32)));
      break;
    case AMDIL::GPRI8RegClassID:
    case AMDIL::GPRV2I8RegClassID:
    case AMDIL::GPRI16RegClassID:
    case AMDIL::GPRV2I16RegClassID:
    case AMDIL::GPRV4I8RegClassID:
    case AMDIL::GPRI32RegClassID:
    case AMDIL::GPRF32RegClassID:
      break;
  };
}


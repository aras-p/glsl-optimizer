//===-- AMDIL789IOExpansion.cpp - TODO: Add brief description -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//
//
// @file AMDIL789IOExpansion.cpp
// @details Implementation of the IO expansion class for 789 devices.
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
AMDIL789IOExpansion::AMDIL789IOExpansion(TargetMachine &tm
    AMDIL_OPT_LEVEL_DECL) 
: AMDILIOExpansion(tm  AMDIL_OPT_LEVEL_VAR)
{
}

AMDIL789IOExpansion::~AMDIL789IOExpansion() {
}

const char *AMDIL789IOExpansion::getPassName() const
{
  return "AMDIL 789 IO Expansion Pass";
}
// This code produces the following pseudo-IL:
// mov r1007, $src.y000
// cmov_logical r1007.x___, $flag.yyyy, r1007.xxxx, $src.xxxx
// mov r1006, $src.z000
// cmov_logical r1007.x___, $flag.zzzz, r1006.xxxx, r1007.xxxx
// mov r1006, $src.w000
// cmov_logical $dst.x___, $flag.wwww, r1006.xxxx, r1007.xxxx
void
AMDIL789IOExpansion::emitComponentExtract(MachineInstr *MI, 
    unsigned flag, unsigned src, unsigned dst, bool before)
{
  MachineBasicBlock::iterator I = *MI;
  DebugLoc DL = MI->getDebugLoc();
  BuildMI(*mBB, I, DL, mTII->get(AMDIL::VEXTRACT_v4i32), AMDIL::R1007)
    .addReg(src)
    .addImm(2);
  BuildMI(*mBB, I, DL, mTII->get(AMDIL::CMOVLOG_Y_i32), AMDIL::R1007)
    .addReg(flag)
    .addReg(AMDIL::R1007)
    .addReg(src);
  BuildMI(*mBB, I, DL, mTII->get(AMDIL::VEXTRACT_v4i32), AMDIL::R1006)
    .addReg(src)
    .addImm(3);
  BuildMI(*mBB, I, DL, mTII->get(AMDIL::CMOVLOG_Z_i32), AMDIL::R1007)
    .addReg(flag)
    .addReg(AMDIL::R1006)
    .addReg(AMDIL::R1007);
  BuildMI(*mBB, I, DL, mTII->get(AMDIL::VEXTRACT_v4i32), AMDIL::R1006)
    .addReg(src)
    .addImm(4);
  BuildMI(*mBB, I, DL, mTII->get(AMDIL::CMOVLOG_W_i32), dst)
    .addReg(flag)
    .addReg(AMDIL::R1006)
    .addReg(AMDIL::R1007);

}
// We have a 128 bit load but a 8/16/32bit value, so we need to
// select the correct component and make sure that the correct
// bits are selected. For the 8 and 16 bit cases we need to 
// extract from the component the correct bits and for 32 bits
// we just need to select the correct component.
  void
AMDIL789IOExpansion::emitDataLoadSelect(MachineInstr *MI)
{
  MachineBasicBlock::iterator I = *MI;
  DebugLoc DL = MI->getDebugLoc();
  emitComponentExtract(MI, AMDIL::R1008, AMDIL::R1011, AMDIL::R1011, false);
  if (getMemorySize(MI) == 1) {
    // This produces the following pseudo-IL:
    // iand r1006.x___, r1010.xxxx, l14.xxxx
    // mov r1006, r1006.xxxx
    // iadd r1006, r1006, {0, -1, 2, 3}
    // ieq r1008, r1006, 0
    // mov r1011, r1011.xxxx
    // ishr r1011, r1011, {0, 8, 16, 24}
    // mov r1007, r1011.y000
    // cmov_logical r1007.x___, r1008.yyyy, r1007.xxxx, r1011.xxxx
    // mov r1006, r1011.z000
    // cmov_logical r1007.x___, r1008.zzzz, r1006.xxxx, r1007.xxxx
    // mov r1006, r1011.w000
    // cmov_logical r1011.x___, r1008.wwww, r1006.xxxx, r1007.xxxx
    BuildMI(*mBB, I, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1006)
      .addReg(AMDIL::R1010)
      .addImm(mMFI->addi32Literal(3));
    BuildMI(*mBB, I, DL, mTII->get(AMDIL::VCREATE_v4i32), AMDIL::R1006)
      .addReg(AMDIL::R1006);
    BuildMI(*mBB, I, DL, mTII->get(AMDIL::ADD_v4i32), AMDIL::R1006)
      .addReg(AMDIL::R1006)
      .addImm(mMFI->addi128Literal(0xFFFFFFFFULL << 32, 
            (0xFFFFFFFEULL | (0xFFFFFFFDULL << 32))));
    BuildMI(*mBB, I, DL, mTII->get(AMDIL::IEQ_v4i32), AMDIL::R1008)
      .addReg(AMDIL::R1006)
      .addImm(mMFI->addi32Literal(0));
    BuildMI(*mBB, I, DL, mTII->get(AMDIL::VCREATE_v4i32), AMDIL::R1011)
      .addReg(AMDIL::R1011);
    BuildMI(*mBB, I, DL, mTII->get(AMDIL::SHRVEC_v4i32), AMDIL::R1011)
      .addReg(AMDIL::R1011)
      .addImm(mMFI->addi128Literal(8ULL << 32, 16ULL | (24ULL << 32)));
    emitComponentExtract(MI, AMDIL::R1008, AMDIL::R1011, AMDIL::R1011, false);
  } else if (getMemorySize(MI) == 2) {
    // This produces the following pseudo-IL:
    // ishr r1007.x___, r1010.xxxx, 1
    // iand r1008.x___, r1007.xxxx, 1
    // ishr r1007.x___, r1011.xxxx, 16
    // cmov_logical r1011.x___, r1008.xxxx, r1007.xxxx, r1011.xxxx
    BuildMI(*mBB, I, DL, mTII->get(AMDIL::SHR_i32), AMDIL::R1007)
      .addReg(AMDIL::R1010)
      .addImm(mMFI->addi32Literal(1));
    BuildMI(*mBB, I, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1008)
      .addReg(AMDIL::R1007)
      .addImm(mMFI->addi32Literal(1));
    BuildMI(*mBB, I, DL, mTII->get(AMDIL::SHR_i32), AMDIL::R1007)
      .addReg(AMDIL::R1011)
      .addImm(mMFI->addi32Literal(16));
    BuildMI(*mBB, I, DL, mTII->get(AMDIL::CMOVLOG_i32), AMDIL::R1011)
      .addReg(AMDIL::R1008)
      .addReg(AMDIL::R1007)
      .addReg(AMDIL::R1011);
  }
}
// This function does address calculations modifications to load from a vector
// register type instead of a dword addressed load.
  void 
AMDIL789IOExpansion::emitVectorAddressCalc(MachineInstr *MI, bool is32bit, bool needsSelect)
{
  MachineBasicBlock::iterator I = *MI;
  DebugLoc DL = MI->getDebugLoc();
  // This produces the following pseudo-IL:
  // ishr r1007.x___, r1010.xxxx, (is32bit) ? 2 : 3
  // iand r1008.x___, r1007.xxxx, (is32bit) ? 3 : 1
  // ishr r1007.x___, r1007.xxxx, (is32bit) ? 2 : 1
  BuildMI(*mBB, I, DL, mTII->get(AMDIL::SHR_i32), AMDIL::R1007)
    .addReg(AMDIL::R1010)
    .addImm(mMFI->addi32Literal((is32bit) ? 0x2 : 3));
  BuildMI(*mBB, I, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1008)
    .addReg(AMDIL::R1007)
    .addImm(mMFI->addi32Literal((is32bit) ? 3 : 1));
  BuildMI(*mBB, I, DL, mTII->get(AMDIL::SHR_i32), AMDIL::R1007)
    .addReg(AMDIL::R1007)
    .addImm(mMFI->addi32Literal((is32bit) ? 2 : 1));
  if (needsSelect) {
    // If the component selection is required, the following 
    // pseudo-IL is produced.
    // mov r1008, r1008.xxxx
    // iadd r1008, r1008, (is32bit) ? {0, -1, -2, -3} : {0, 0, -1, -1}
    // ieq r1008, r1008, 0
    BuildMI(*mBB, I, DL, mTII->get(AMDIL::VCREATE_v4i32), AMDIL::R1008)
      .addReg(AMDIL::R1008);
    BuildMI(*mBB, I, DL, mTII->get(AMDIL::ADD_v4i32), AMDIL::R1008)
      .addReg(AMDIL::R1008)
      .addImm(mMFI->addi128Literal((is32bit) ? 0xFFFFFFFFULL << 32 : 0ULL,  
            (is32bit) ? 0xFFFFFFFEULL | (0xFFFFFFFDULL << 32) :
            -1ULL));
    BuildMI(*mBB, I, DL, mTII->get(AMDIL::IEQ_v4i32), AMDIL::R1008)
      .addReg(AMDIL::R1008)
      .addImm(mMFI->addi32Literal(0));
  }
}
// This function emits a switch statement and writes 32bit/64bit 
// value to a 128bit vector register type.
  void
AMDIL789IOExpansion::emitVectorSwitchWrite(MachineInstr *MI, bool is32bit) 
{
  MachineBasicBlock::iterator I = *MI;
  uint32_t xID = getPointerID(MI);
  assert(xID && "Found a scratch store that was incorrectly marked as zero ID!\n");
  // This section generates the following pseudo-IL:
  // switch r1008.x
  // default
  //   mov x1[r1007.x].(is32bit) ? x___ : xy__, r1011.x{y}
  // break
  // case 1
  //   mov x1[r1007.x].(is32bit) ? _y__ : __zw, r1011.x{yxy}
  // break
  // if is32bit is true, case 2 and 3 are emitted.
  // case 2
  //   mov x1[r1007.x].__z_, r1011.x
  // break
  // case 3
  //   mov x1[r1007.x].___w, r1011.x
  // break
  // endswitch
  DebugLoc DL;
  BuildMI(*mBB, I, MI->getDebugLoc(), mTII->get(AMDIL::SWITCH))
    .addReg(AMDIL::R1008);
  BuildMI(*mBB, I, DL, mTII->get(AMDIL::DEFAULT));
  BuildMI(*mBB, I, DL,
      mTII->get((is32bit) ? AMDIL::SCRATCHSTORE_X : AMDIL::SCRATCHSTORE_XY)
      , AMDIL::R1007)
    .addReg(AMDIL::R1011)
    .addImm(xID);
  BuildMI(*mBB, I, DL, mTII->get(AMDIL::BREAK));
  BuildMI(*mBB, I, DL, mTII->get(AMDIL::CASE)).addImm(1);
  BuildMI(*mBB, I, DL,
      mTII->get((is32bit) ? AMDIL::SCRATCHSTORE_Y : AMDIL::SCRATCHSTORE_ZW), AMDIL::R1007)
    .addReg(AMDIL::R1011)
    .addImm(xID);
  BuildMI(*mBB, I, DL, mTII->get(AMDIL::BREAK));
  if (is32bit) {
    BuildMI(*mBB, I, DL, mTII->get(AMDIL::CASE)).addImm(2);
    BuildMI(*mBB, I, DL,
        mTII->get(AMDIL::SCRATCHSTORE_Z), AMDIL::R1007)
      .addReg(AMDIL::R1011)
      .addImm(xID);
    BuildMI(*mBB, I, DL, mTII->get(AMDIL::BREAK));
    BuildMI(*mBB, I, DL, mTII->get(AMDIL::CASE)).addImm(3);
    BuildMI(*mBB, I, DL,
        mTII->get(AMDIL::SCRATCHSTORE_W), AMDIL::R1007)
      .addReg(AMDIL::R1011)
      .addImm(xID);
    BuildMI(*mBB, I, DL, mTII->get(AMDIL::BREAK));
  }
  BuildMI(*mBB, I, DL, mTII->get(AMDIL::ENDSWITCH));

}
  void
AMDIL789IOExpansion::expandPrivateLoad(MachineInstr *MI)
{
  MachineBasicBlock::iterator I = *MI;
  bool HWPrivate = mSTM->device()->usesHardware(AMDILDeviceInfo::PrivateMem);
  if (!HWPrivate || mSTM->device()->isSupported(AMDILDeviceInfo::PrivateUAV)) {
    return expandGlobalLoad(MI);
  }
  if (!mMFI->usesMem(AMDILDevice::SCRATCH_ID)
      && mKM->isKernel()) {
    mMFI->addErrorMsg(amd::CompilerErrorMessage[MEMOP_NO_ALLOCATION]);
  }
  uint32_t xID = getPointerID(MI);
  assert(xID && "Found a scratch load that was incorrectly marked as zero ID!\n");
  if (!xID) {
    xID = mSTM->device()->getResourceID(AMDILDevice::SCRATCH_ID);
    mMFI->addErrorMsg(amd::CompilerWarningMessage[RECOVERABLE_ERROR]);
  }
  DebugLoc DL;
  // These instructions go before the current MI.
  expandLoadStartCode(MI);
  switch (getMemorySize(MI)) {
    default:
      // Since the private register is a 128 bit aligned, we have to align the address
      // first, since our source address is 32bit aligned and then load the data.
      // This produces the following pseudo-IL:
      // ishr r1010.x___, r1010.xxxx, 4
	    // mov r1011, x1[r1010.x]
      BuildMI(*mBB, I, DL,
          mTII->get(AMDIL::SHR_i32), AMDIL::R1010)
        .addReg(AMDIL::R1010)
        .addImm(mMFI->addi32Literal(4));
      BuildMI(*mBB, I, DL,
          mTII->get(AMDIL::SCRATCHLOAD), AMDIL::R1011)
        .addReg(AMDIL::R1010)
        .addImm(xID);
      break;
    case 1:
    case 2:
    case 4:
      emitVectorAddressCalc(MI, true, true);
      // This produces the following pseudo-IL:
      // mov r1011, x1[r1007.x]
      BuildMI(*mBB, I, DL,
          mTII->get(AMDIL::SCRATCHLOAD), AMDIL::R1011)
        .addReg(AMDIL::R1007)
        .addImm(xID);
      // These instructions go after the current MI.
      emitDataLoadSelect(MI);
     break;
    case 8:
      emitVectorAddressCalc(MI, false, true);
      // This produces the following pseudo-IL:
      // mov r1011, x1[r1007.x]
      // mov r1007, r1011.zw00
      // cmov_logical r1011.xy__, r1008.xxxx, r1011.xy, r1007.zw
      BuildMI(*mBB, I, DL,
          mTII->get(AMDIL::SCRATCHLOAD), AMDIL::R1011)
        .addReg(AMDIL::R1007)
        .addImm(xID);
      // These instructions go after the current MI.
      BuildMI(*mBB, I, DL,
          mTII->get(AMDIL::VEXTRACT_v2i64), AMDIL::R1007)
        .addReg(AMDIL::R1011)
        .addImm(2);
      BuildMI(*mBB, I, DL,
          mTII->get(AMDIL::CMOVLOG_i64), AMDIL::R1011)
        .addReg(AMDIL::R1008)
        .addReg(AMDIL::R1011)
        .addReg(AMDIL::R1007);
     break;
  }
  expandPackedData(MI);
  expandExtendLoad(MI);
  BuildMI(*mBB, I, MI->getDebugLoc(),
      mTII->get(getMoveInstFromID(
          MI->getDesc().OpInfo[0].RegClass)),
      MI->getOperand(0).getReg())
    .addReg(AMDIL::R1011);
}


  void
AMDIL789IOExpansion::expandConstantLoad(MachineInstr *MI)
{
  MachineBasicBlock::iterator I = *MI;
  if (!isHardwareInst(MI) || MI->memoperands_empty()) {
    return expandGlobalLoad(MI);
  }
  uint32_t cID = getPointerID(MI);
  if (cID < 2) {
    return expandGlobalLoad(MI);
  }
  if (!mMFI->usesMem(AMDILDevice::CONSTANT_ID)
      && mKM->isKernel()) {
    mMFI->addErrorMsg(amd::CompilerErrorMessage[MEMOP_NO_ALLOCATION]);
  }

  DebugLoc DL;
  // These instructions go before the current MI.
  expandLoadStartCode(MI);
  switch (getMemorySize(MI)) {
    default:
      BuildMI(*mBB, I, DL,
          mTII->get(AMDIL::SHR_i32), AMDIL::R1010)
        .addReg(AMDIL::R1010)
        .addImm(mMFI->addi32Literal(4));
      BuildMI(*mBB, I, DL,
          mTII->get(AMDIL::CBLOAD), AMDIL::R1011)
        .addReg(AMDIL::R1010)
        .addImm(cID);
      break;
    case 1:
    case 2:
    case 4:
      emitVectorAddressCalc(MI, true, true);
      BuildMI(*mBB, I, DL,
          mTII->get(AMDIL::CBLOAD), AMDIL::R1011)
        .addReg(AMDIL::R1007)
        .addImm(cID);
      // These instructions go after the current MI.
      emitDataLoadSelect(MI);
      break;
    case 8:
      emitVectorAddressCalc(MI, false, true);
      BuildMI(*mBB, I, DL,
          mTII->get(AMDIL::CBLOAD), AMDIL::R1011)
        .addReg(AMDIL::R1007)
        .addImm(cID);
      // These instructions go after the current MI.
      BuildMI(*mBB, I, DL,
          mTII->get(AMDIL::VEXTRACT_v2i64), AMDIL::R1007)
        .addReg(AMDIL::R1011)
        .addImm(2);
      BuildMI(*mBB, I, DL,
          mTII->get(AMDIL::VCREATE_v2i32), AMDIL::R1008)
        .addReg(AMDIL::R1008);
      BuildMI(*mBB, I, DL,
          mTII->get(AMDIL::CMOVLOG_i64), AMDIL::R1011)
        .addReg(AMDIL::R1008)
        .addReg(AMDIL::R1011)
        .addReg(AMDIL::R1007);
      break;
  }
  expandPackedData(MI);
  expandExtendLoad(MI);
  BuildMI(*mBB, I, MI->getDebugLoc(),
      mTII->get(getMoveInstFromID(
          MI->getDesc().OpInfo[0].RegClass)),
      MI->getOperand(0).getReg())
    .addReg(AMDIL::R1011);
  MI->getOperand(0).setReg(AMDIL::R1011);
}

  void
AMDIL789IOExpansion::expandConstantPoolLoad(MachineInstr *MI)
{
  if (!isStaticCPLoad(MI)) {
    return expandConstantLoad(MI);
  } else {
    uint32_t idx = MI->getOperand(1).getIndex();
    const MachineConstantPool *MCP = MI->getParent()->getParent()
      ->getConstantPool();
    const std::vector<MachineConstantPoolEntry> &consts
      = MCP->getConstants();
    const Constant *C = consts[idx].Val.ConstVal;
    emitCPInst(MI, C, mKM, 0, isExtendLoad(MI));
  }
}

  void
AMDIL789IOExpansion::expandPrivateStore(MachineInstr *MI)
{
  MachineBasicBlock::iterator I = *MI;
  bool HWPrivate = mSTM->device()->usesHardware(AMDILDeviceInfo::PrivateMem);
  if (!HWPrivate || mSTM->device()->isSupported(AMDILDeviceInfo::PrivateUAV)) {
    return expandGlobalStore(MI);
  }
   if (!mMFI->usesMem(AMDILDevice::SCRATCH_ID)
      && mKM->isKernel()) {
    mMFI->addErrorMsg(amd::CompilerErrorMessage[MEMOP_NO_ALLOCATION]);
  }
  uint32_t xID = getPointerID(MI);
  assert(xID && "Found a scratch store that was incorrectly marked as zero ID!\n");
  if (!xID) {
    xID = mSTM->device()->getResourceID(AMDILDevice::SCRATCH_ID);
    mMFI->addErrorMsg(amd::CompilerWarningMessage[RECOVERABLE_ERROR]);
  }
  DebugLoc DL;
   // These instructions go before the current MI.
  expandStoreSetupCode(MI);
  switch (getMemorySize(MI)) {
    default:
      // This section generates the following pseudo-IL:
      // ishr r1010.x___, r1010.xxxx, 4
	    // mov x1[r1010.x], r1011
      BuildMI(*mBB, I, DL,
          mTII->get(AMDIL::SHR_i32), AMDIL::R1010)
        .addReg(AMDIL::R1010)
        .addImm(mMFI->addi32Literal(4));
      BuildMI(*mBB, I, MI->getDebugLoc(),
          mTII->get(AMDIL::SCRATCHSTORE), AMDIL::R1010)
        .addReg(AMDIL::R1011)
        .addImm(xID);
      break;
    case 1:
      emitVectorAddressCalc(MI, true, true);
      // This section generates the following pseudo-IL:
      // mov r1002, x1[r1007.x]
      BuildMI(*mBB, I, DL,
          mTII->get(AMDIL::SCRATCHLOAD), AMDIL::R1002)
        .addReg(AMDIL::R1007)
        .addImm(xID);
      emitComponentExtract(MI, AMDIL::R1008, AMDIL::R1002, AMDIL::R1002, true);
      // This section generates the following pseudo-IL:
      // iand r1003.x, r1010.x, 3
      // mov r1003, r1003.xxxx
      // iadd r1000, r1003, {0, -1, -2, -3}
      // ieq r1000, r1000, 0
      // mov r1002, r1002.xxxx
      // ishr r1002, r1002, {0, 8, 16, 24}
      // mov r1011, r1011.xxxx
      // cmov_logical r1002, r1000, r1011, r1002
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1003)
        .addReg(AMDIL::R1010)
        .addImm(mMFI->addi32Literal(3));
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::VCREATE_v4i32), AMDIL::R1003)
        .addReg(AMDIL::R1003);
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::ADD_v4i32), AMDIL::R1001)
        .addReg(AMDIL::R1003)
        .addImm(mMFI->addi128Literal(0xFFFFFFFFULL << 32, 
              (0xFFFFFFFEULL | (0xFFFFFFFDULL << 32))));
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::IEQ_v4i32), AMDIL::R1001)
        .addReg(AMDIL::R1001)
        .addImm(mMFI->addi32Literal(0));
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::VCREATE_v4i32), AMDIL::R1002)
        .addReg(AMDIL::R1002);
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::SHRVEC_v4i32), AMDIL::R1002)
      .addReg(AMDIL::R1002)
      .addImm(mMFI->addi128Literal(8ULL << 32, 16ULL | (24ULL << 32)));
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::VCREATE_v4i32), AMDIL::R1011)
        .addReg(AMDIL::R1011);
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::CMOVLOG_v4i32), AMDIL::R1002)
        .addReg(AMDIL::R1001)
        .addReg(AMDIL::R1011)
        .addReg(AMDIL::R1002);
      if (mSTM->device()->getGeneration() == AMDILDeviceInfo::HD4XXX) {
        // This section generates the following pseudo-IL:
        // iand r1002, r1002, 0xFF
        // ishl r1002, r1002, {0, 8, 16, 24}
        // ior r1002.xy, r1002.xy, r1002.zw
        // ior r1011.x, r1002.x, r1002.y
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::BINARY_AND_v4i32), AMDIL::R1002)
          .addReg(AMDIL::R1002)
          .addImm(mMFI->addi32Literal(0xFF));
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::SHL_v4i32), AMDIL::R1002)
          .addReg(AMDIL::R1002)
          .addImm(mMFI->addi128Literal(8ULL << 32, 16ULL | (24ULL << 32)));
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::HILO_BITOR_v2i64), AMDIL::R1002)
          .addReg(AMDIL::R1002).addReg(AMDIL::R1002);
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::HILO_BITOR_v2i32), AMDIL::R1011)
          .addReg(AMDIL::R1002).addReg(AMDIL::R1002);
      } else {
        // This section generates the following pseudo-IL:
        // mov r1001.xy, r1002.yw
        // mov r1002.xy, r1002.xz
        // ubit_insert r1002.xy, 8, 8, r1001.xy, r1002.xy
        // mov r1001.x, r1002.y
        // ubit_insert r1011.x, 16, 16, r1002.y, r1002.x
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::LHI_v2i64), AMDIL::R1001)
          .addReg(AMDIL::R1002);
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::LLO_v2i64), AMDIL::R1002)
          .addReg(AMDIL::R1002);
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::UBIT_INSERT_v2i32), AMDIL::R1002)
          .addImm(mMFI->addi32Literal(8))
          .addImm(mMFI->addi32Literal(8))
          .addReg(AMDIL::R1001)
          .addReg(AMDIL::R1002);
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::LHI), AMDIL::R1001)
          .addReg(AMDIL::R1002);
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::UBIT_INSERT_i32), AMDIL::R1011)
          .addImm(mMFI->addi32Literal(16))
          .addImm(mMFI->addi32Literal(16))
          .addReg(AMDIL::R1001)
          .addReg(AMDIL::R1002);
      }
      emitVectorAddressCalc(MI, true, false);
      emitVectorSwitchWrite(MI, true);
      break;
    case 2:
      emitVectorAddressCalc(MI, true, true);
      // This section generates the following pseudo-IL:
      // mov r1002, x1[r1007.x]
      BuildMI(*mBB, I, DL,
          mTII->get(AMDIL::SCRATCHLOAD), AMDIL::R1002)
        .addReg(AMDIL::R1007)
        .addImm(xID);
      emitComponentExtract(MI, AMDIL::R1008, AMDIL::R1002, AMDIL::R1002, true);
      // This section generates the following pseudo-IL:
      // ishr r1003.x, r1010.x, 1
      // iand r1003.x, r1003.x, 1
      // ishr r1001.x, r1002.x, 16
      // cmov_logical r1002.x, r1003.x, r1002.x, r1011.x
      // cmov_logical r1001.x, r1003.x, r1011.x, r1001.x
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::SHR_i32), AMDIL::R1003)
        .addReg(AMDIL::R1010)
        .addImm(mMFI->addi32Literal(1));
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1003)
        .addReg(AMDIL::R1003)
        .addImm(mMFI->addi32Literal(1));
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::SHR_i32), AMDIL::R1001)
        .addReg(AMDIL::R1002)
        .addImm(mMFI->addi32Literal(16));
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::CMOVLOG_i32), AMDIL::R1002)
        .addReg(AMDIL::R1003)
        .addReg(AMDIL::R1002)
        .addReg(AMDIL::R1011);
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::CMOVLOG_i32), AMDIL::R1001)
        .addReg(AMDIL::R1003)
        .addReg(AMDIL::R1011)
        .addReg(AMDIL::R1001);
      if (mSTM->device()->getGeneration() == AMDILDeviceInfo::HD4XXX) {
        // This section generates the following pseudo-IL:
        // iand r1002.x, r1002.x, 0xFFFF
        // iand r1001.x, r1001.x, 0xFFFF
        // ishl r1001.x, r1002.x, 16
        // ior r1011.x, r1002.x, r1001.x
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1002)
          .addReg(AMDIL::R1002)
          .addImm(mMFI->addi32Literal(0xFFFF));
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::BINARY_AND_i32), AMDIL::R1001)
          .addReg(AMDIL::R1001)
          .addImm(mMFI->addi32Literal(0xFFFF));
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::SHL_i32), AMDIL::R1001)
          .addReg(AMDIL::R1001)
          .addImm(mMFI->addi32Literal(16));
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::BINARY_OR_i32), AMDIL::R1011)
          .addReg(AMDIL::R1002).addReg(AMDIL::R1001);
      } else {
        // This section generates the following pseudo-IL:
        // ubit_insert r1011.x, 16, 16, r1001.y, r1002.x
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::UBIT_INSERT_i32), AMDIL::R1011)
          .addImm(mMFI->addi32Literal(16))
          .addImm(mMFI->addi32Literal(16))
          .addReg(AMDIL::R1001)
          .addReg(AMDIL::R1002);
      }
      emitVectorAddressCalc(MI, true, false);
      emitVectorSwitchWrite(MI, true);
      break;
    case 4:
      emitVectorAddressCalc(MI, true, false);
      emitVectorSwitchWrite(MI, true);
      break;
    case 8:
      emitVectorAddressCalc(MI, false, false);
      emitVectorSwitchWrite(MI, false);
      break;
  };
}
 void
AMDIL789IOExpansion::expandStoreSetupCode(MachineInstr *MI)
{
  MachineBasicBlock::iterator I = *MI;
  DebugLoc DL;
  if (MI->getOperand(0).isUndef()) {
  BuildMI(*mBB, I, DL, mTII->get(getMoveInstFromID(
          MI->getDesc().OpInfo[0].RegClass)), AMDIL::R1011)
      .addImm(mMFI->addi32Literal(0));
  } else {
  BuildMI(*mBB, I, DL, mTII->get(getMoveInstFromID(
          MI->getDesc().OpInfo[0].RegClass)), AMDIL::R1011)
      .addReg(MI->getOperand(0).getReg());
  }
  expandTruncData(MI);
  if (MI->getOperand(2).isReg()) {
    BuildMI(*mBB, I, DL, mTII->get(AMDIL::ADD_i32), AMDIL::R1010)
      .addReg(MI->getOperand(1).getReg())
      .addReg(MI->getOperand(2).getReg());
  } else {
    BuildMI(*mBB, I, DL, mTII->get(AMDIL::MOVE_i32), AMDIL::R1010)
      .addReg(MI->getOperand(1).getReg());
  }
  expandAddressCalc(MI);
  expandPackedData(MI);
}


void
AMDIL789IOExpansion::expandPackedData(MachineInstr *MI)
{
  MachineBasicBlock::iterator I = *MI;
  if (!isPackedData(MI)) {
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
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::BINARY_AND_v2i32), AMDIL::R1011)
          .addReg(AMDIL::R1011)
          .addImm(mMFI->addi64Literal(0xFFULL | (0xFFULL << 32)));
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::SHL_v2i32), AMDIL::R1011)
          .addReg(AMDIL::R1011).addImm(mMFI->addi64Literal(8ULL << 32));
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::HILO_BITOR_v2i32), AMDIL::R1011)
          .addReg(AMDIL::R1011).addReg(AMDIL::R1011);
      }
      break;
    case PACK_V4I8:
      {
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::BINARY_AND_v4i32), AMDIL::R1011)
          .addReg(AMDIL::R1011)
          .addImm(mMFI->addi32Literal(0xFF));
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::SHL_v4i32), AMDIL::R1011)
          .addReg(AMDIL::R1011)
          .addImm(mMFI->addi128Literal(8ULL << 32, (16ULL | (24ULL << 32))));
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::HILO_BITOR_v2i64), AMDIL::R1011)
          .addReg(AMDIL::R1011).addReg(AMDIL::R1011);
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::HILO_BITOR_v2i32), AMDIL::R1011)
          .addReg(AMDIL::R1011).addReg(AMDIL::R1011);
      }
      break;
    case PACK_V2I16:
      {
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::BINARY_AND_v2i32), AMDIL::R1011)
          .addReg(AMDIL::R1011)
          .addImm(mMFI->addi32Literal(0xFFFF));
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::SHL_v2i32), AMDIL::R1011)
          .addReg(AMDIL::R1011)
          .addImm(mMFI->addi64Literal(16ULL << 32));
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::HILO_BITOR_v2i32), AMDIL::R1011)
          .addReg(AMDIL::R1011).addReg(AMDIL::R1011);
      }
      break;
    case PACK_V4I16:
      {
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::BINARY_AND_v4i32), AMDIL::R1011)
          .addReg(AMDIL::R1011)
          .addImm(mMFI->addi32Literal(0xFFFF));
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::SHL_v4i32), AMDIL::R1011)
          .addReg(AMDIL::R1011)
          .addImm(mMFI->addi64Literal(16ULL << 32));
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::HILO_BITOR_v4i16), AMDIL::R1011)
          .addReg(AMDIL::R1011).addReg(AMDIL::R1011);
      }
      break;
    case UNPACK_V2I8:
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::USHRVEC_i32), AMDIL::R1012)
        .addReg(AMDIL::R1011)
        .addImm(mMFI->addi32Literal(8));
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::LCREATE), AMDIL::R1011)
        .addReg(AMDIL::R1011).addReg(AMDIL::R1012);
      break;
    case UNPACK_V4I8:
      {
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::VCREATE_v4i8), AMDIL::R1011)
          .addReg(AMDIL::R1011);
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::USHRVEC_v4i8), AMDIL::R1011)
          .addReg(AMDIL::R1011)
          .addImm(mMFI->addi128Literal(8ULL << 32, (16ULL | (24ULL << 32))));
      }
      break;
    case UNPACK_V2I16:
      {
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::USHRVEC_i32), AMDIL::R1012)
          .addReg(AMDIL::R1011)
          .addImm(mMFI->addi32Literal(16));
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::LCREATE), AMDIL::R1011)
          .addReg(AMDIL::R1011).addReg(AMDIL::R1012);
      }
      break;
    case UNPACK_V4I16:
      {
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::USHRVEC_v2i32), AMDIL::R1012)
          .addReg(AMDIL::R1011)
          .addImm(mMFI->addi32Literal(16));
        BuildMI(*mBB, I, DL, mTII->get(AMDIL::LCREATE_v2i64), AMDIL::R1011)
          .addReg(AMDIL::R1011).addReg(AMDIL::R1012);
      }
      break;
  };
}

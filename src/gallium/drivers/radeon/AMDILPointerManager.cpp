//===-------- AMDILPointerManager.cpp - Manage Pointers for HW-------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//
// Implementation for the AMDILPointerManager classes. See header file for
// more documentation of class.
// TODO: This fails when function calls are enabled, must always be inlined
//===----------------------------------------------------------------------===//
#include "AMDILPointerManager.h"
#include "AMDILCompilerErrors.h"
#include "AMDILDeviceInfo.h"
#include "AMDILGlobalManager.h"
#include "AMDILKernelManager.h"
#include "AMDILMachineFunctionInfo.h"
#include "AMDILTargetMachine.h"
#include "AMDILUtilityFunctions.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/ADT/Twine.h"
#include "llvm/ADT/ValueMap.h"
#include "llvm/CodeGen/MachineDominators.h"
#include "llvm/CodeGen/MachineFunctionAnalysis.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Function.h"
#include "llvm/GlobalValue.h"
#include "llvm/Instructions.h"
#include "llvm/Metadata.h"
#include "llvm/Module.h"
#include "llvm/Support/FormattedStream.h"

#include <stdio.h>
using namespace llvm;
char AMDILPointerManager::ID = 0;
namespace llvm {
  FunctionPass*
    createAMDILPointerManager(TargetMachine &tm AMDIL_OPT_LEVEL_DECL)
    {
      return tm.getSubtarget<AMDILSubtarget>()
        .device()->getPointerManager(tm AMDIL_OPT_LEVEL_VAR);
    }
}

AMDILPointerManager::AMDILPointerManager(
    TargetMachine &tm
    AMDIL_OPT_LEVEL_DECL) :
  MachineFunctionPass(ID),
  TM(tm)
{
  mDebug = DEBUGME;
  initializeMachineDominatorTreePass(*PassRegistry::getPassRegistry());
}

AMDILPointerManager::~AMDILPointerManager()
{
}

const char*
AMDILPointerManager::getPassName() const
{
  return "AMD IL Default Pointer Manager Pass";
}

void
AMDILPointerManager::getAnalysisUsage(AnalysisUsage &AU) const
{
  AU.setPreservesAll();
  AU.addRequiredID(MachineDominatorsID);
  MachineFunctionPass::getAnalysisUsage(AU);
}

AMDILEGPointerManager::AMDILEGPointerManager(
    TargetMachine &tm
    AMDIL_OPT_LEVEL_DECL) :
  AMDILPointerManager(tm AMDIL_OPT_LEVEL_VAR),
  TM(tm)
{
}

AMDILEGPointerManager::~AMDILEGPointerManager()
{
}
std::string
findSamplerName(MachineInstr* MI,
    FIPMap &FIToPtrMap,
    RVPVec &lookupTable,
    const TargetMachine *TM)
{
  std::string sampler = "unknown";
  assert(MI->getNumOperands() == 5 && "Only an "
      "image read instruction with 5 arguments can "
      "have a sampler.");
  assert(MI->getOperand(3).isReg() && 
      "Argument 3 must be a register to call this function");
  unsigned reg = MI->getOperand(3).getReg();
  // If this register points to an argument, then
  // we can return the argument name.
  if (lookupTable[reg].second && dyn_cast<Argument>(lookupTable[reg].second)) {
    return lookupTable[reg].second->getName();
  }
  // Otherwise the sampler is coming from memory somewhere.
  // If the sampler memory location can be tracked, then
  // we ascertain the sampler name that way.
  // The most common case is when optimizations are disabled
  // or mem2reg is not enabled, then the sampler when it is
  // an argument is passed through the frame index.

  // In the optimized case, the instruction that defined
  // register from operand #3 is a private load.
  MachineRegisterInfo &regInfo = MI->getParent()->getParent()->getRegInfo();
  assert(!regInfo.def_empty(reg) 
      && "We don't have any defs of this register, but we aren't an argument!");
  MachineOperand *defOp = regInfo.getRegUseDefListHead(reg);
  MachineInstr *defMI = defOp->getParent();
  if (isPrivateInst(TM->getInstrInfo(), defMI) && isLoadInst(TM->getInstrInfo(), defMI)) {
    if (defMI->getOperand(1).isFI()) {
      RegValPair &fiRVP = FIToPtrMap[reg];
      if (fiRVP.second && dyn_cast<Argument>(fiRVP.second)) {
        return fiRVP.second->getName();
      } else {
        // FIXME: Fix the case where the value stored is not a kernel argument.
        assert(!"Found a private load of a sampler where the value isn't an argument!");
      }
    } else {
      // FIXME: Fix the case where someone dynamically loads a sampler value
      // from private memory. This is problematic because we need to know the
      // sampler value at compile time and if it is dynamically loaded, we won't
      // know what sampler value to use.
      assert(!"Found a private load of a sampler that isn't from a frame index!");
    }
  } else {
    // FIXME: Handle the case where the def is neither a private instruction
    // and not a load instruction. This shouldn't occur, but putting an assertion
    // just to make sure that it doesn't.
    assert(!"Found a case which we don't handle.");
  }
  return sampler;
}

const char*
AMDILEGPointerManager::getPassName() const
{
  return "AMD IL EG Pointer Manager Pass";
}

// Helper function to determine if the current pointer is from the
// local, region or private address spaces.
  static bool
isLRPInst(MachineInstr *MI,
    const AMDILTargetMachine *ATM)
{
  const AMDILSubtarget *STM
    = ATM->getSubtargetImpl();
  if (!MI) {
    return false;
  }
  if ((isRegionInst(ATM->getInstrInfo(), MI) 
        && STM->device()->usesHardware(AMDILDeviceInfo::RegionMem))
      || (isLocalInst(ATM->getInstrInfo(), MI)
        && STM->device()->usesHardware(AMDILDeviceInfo::LocalMem))
      || (isPrivateInst(ATM->getInstrInfo(), MI)
        && STM->device()->usesHardware(AMDILDeviceInfo::PrivateMem))) {
    return true;
  }
  return false;
}

/// Helper function to determine if the I/O instruction uses
/// global device memory or not.
static bool
usesGlobal(
    const AMDILTargetMachine *ATM,
    MachineInstr *MI) {
  const AMDILSubtarget *STM
    = ATM->getSubtargetImpl();
  switch(MI->getOpcode()) {
    ExpandCaseToAllTypes(AMDIL::GLOBALSTORE);
    ExpandCaseToAllTruncTypes(AMDIL::GLOBALTRUNCSTORE);
    ExpandCaseToAllTypes(AMDIL::GLOBALLOAD);
    ExpandCaseToAllTypes(AMDIL::GLOBALSEXTLOAD);
    ExpandCaseToAllTypes(AMDIL::GLOBALZEXTLOAD);
    ExpandCaseToAllTypes(AMDIL::GLOBALAEXTLOAD);
    return true;
    ExpandCaseToAllTypes(AMDIL::REGIONLOAD);
    ExpandCaseToAllTypes(AMDIL::REGIONSEXTLOAD);
    ExpandCaseToAllTypes(AMDIL::REGIONZEXTLOAD);
    ExpandCaseToAllTypes(AMDIL::REGIONAEXTLOAD);
    ExpandCaseToAllTypes(AMDIL::REGIONSTORE);
    ExpandCaseToAllTruncTypes(AMDIL::REGIONTRUNCSTORE);
    return !STM->device()->usesHardware(AMDILDeviceInfo::RegionMem);
    ExpandCaseToAllTypes(AMDIL::LOCALLOAD);
    ExpandCaseToAllTypes(AMDIL::LOCALSEXTLOAD);
    ExpandCaseToAllTypes(AMDIL::LOCALZEXTLOAD);
    ExpandCaseToAllTypes(AMDIL::LOCALAEXTLOAD);
    ExpandCaseToAllTypes(AMDIL::LOCALSTORE);
    ExpandCaseToAllTruncTypes(AMDIL::LOCALTRUNCSTORE);
    return !STM->device()->usesHardware(AMDILDeviceInfo::LocalMem);
    ExpandCaseToAllTypes(AMDIL::CPOOLLOAD);
    ExpandCaseToAllTypes(AMDIL::CPOOLSEXTLOAD);
    ExpandCaseToAllTypes(AMDIL::CPOOLZEXTLOAD);
    ExpandCaseToAllTypes(AMDIL::CPOOLAEXTLOAD);
    ExpandCaseToAllTypes(AMDIL::CONSTANTLOAD);
    ExpandCaseToAllTypes(AMDIL::CONSTANTSEXTLOAD);
    ExpandCaseToAllTypes(AMDIL::CONSTANTAEXTLOAD);
    ExpandCaseToAllTypes(AMDIL::CONSTANTZEXTLOAD);
    return !STM->device()->usesHardware(AMDILDeviceInfo::ConstantMem);
    ExpandCaseToAllTypes(AMDIL::PRIVATELOAD);
    ExpandCaseToAllTypes(AMDIL::PRIVATESEXTLOAD);
    ExpandCaseToAllTypes(AMDIL::PRIVATEZEXTLOAD);
    ExpandCaseToAllTypes(AMDIL::PRIVATEAEXTLOAD);
    ExpandCaseToAllTypes(AMDIL::PRIVATESTORE);
    ExpandCaseToAllTruncTypes(AMDIL::PRIVATETRUNCSTORE);
    return !STM->device()->usesHardware(AMDILDeviceInfo::PrivateMem);
    default:
    return false;
  }
  return false;
}

// Helper function that allocates the default resource ID for the
// respective I/O types.
static void
allocateDefaultID(
    const AMDILTargetMachine *ATM,
    AMDILAS::InstrResEnc &curRes,
    MachineInstr *MI,
    bool mDebug)
{
  AMDILMachineFunctionInfo *mMFI =
    MI->getParent()->getParent()->getInfo<AMDILMachineFunctionInfo>();
  const AMDILSubtarget *STM
    = ATM->getSubtargetImpl();
  if (mDebug) {
    dbgs() << "Assigning instruction to default ID. Inst:";
    MI->dump();
  }
  // If we use global memory, lets set the Operand to
  // the ARENA_UAV_ID.
  if (usesGlobal(ATM, MI)) {
    curRes.bits.ResourceID =
      STM->device()->getResourceID(AMDILDevice::GLOBAL_ID);
    if (isAtomicInst(ATM->getInstrInfo(), MI)) {
      MI->getOperand(MI->getNumOperands()-1)
        .setImm(curRes.bits.ResourceID);
    }
    AMDILKernelManager *KM = STM->getKernelManager();
    if (curRes.bits.ResourceID == 8 
        && !STM->device()->isSupported(AMDILDeviceInfo::ArenaSegment)) {
        KM->setUAVID(NULL, curRes.bits.ResourceID);
        mMFI->uav_insert(curRes.bits.ResourceID);
    }
  } else if (isPrivateInst(ATM->getInstrInfo(), MI)) {
    curRes.bits.ResourceID =
      STM->device()->getResourceID(AMDILDevice::SCRATCH_ID);
  } else if (isLocalInst(ATM->getInstrInfo(), MI) || isLocalAtomic(ATM->getInstrInfo(), MI)) {
    curRes.bits.ResourceID =
      STM->device()->getResourceID(AMDILDevice::LDS_ID);
    AMDILMachineFunctionInfo *mMFI = 
    MI->getParent()->getParent()->getInfo<AMDILMachineFunctionInfo>();
    mMFI->setUsesLocal();
    if (isAtomicInst(ATM->getInstrInfo(), MI)) {
      assert(curRes.bits.ResourceID && "Atomic resource ID "
          "cannot be zero!");
      MI->getOperand(MI->getNumOperands()-1)
        .setImm(curRes.bits.ResourceID);
    }
  } else if (isRegionInst(ATM->getInstrInfo(), MI) || isRegionAtomic(ATM->getInstrInfo(), MI)) {
    curRes.bits.ResourceID =
      STM->device()->getResourceID(AMDILDevice::GDS_ID);
    AMDILMachineFunctionInfo *mMFI = 
    MI->getParent()->getParent()->getInfo<AMDILMachineFunctionInfo>();
    mMFI->setUsesRegion();
    if (isAtomicInst(ATM->getInstrInfo(), MI)) {
      assert(curRes.bits.ResourceID && "Atomic resource ID "
          "cannot be zero!");
      (MI)->getOperand((MI)->getNumOperands()-1)
        .setImm(curRes.bits.ResourceID);
    }
  } else if (isConstantInst(ATM->getInstrInfo(), MI)) {
    // If we are unknown constant instruction and the base pointer is known.
    // Set the resource ID accordingly, otherwise use the default constant ID.
    // FIXME: this should not require the base pointer to know what constant
    // it is from.
    AMDILGlobalManager *GM = STM->getGlobalManager();
    MachineFunction *MF = MI->getParent()->getParent();
    if (GM->isKernel(MF->getFunction()->getName())) {
      const kernel &krnl = GM->getKernel(MF->getFunction()->getName());
      const Value *V = getBasePointerValue(MI);
      if (V && !dyn_cast<AllocaInst>(V)) {
        curRes.bits.ResourceID = GM->getConstPtrCB(krnl, V->getName());
        curRes.bits.HardwareInst = 1;
      } else if (V && dyn_cast<AllocaInst>(V)) {
          // FIXME: Need a better way to fix this. Requires a rewrite of how
          // we lower global addresses to various address spaces.
          // So for now, lets assume that there is only a single 
          // constant buffer that can be accessed from a load instruction
          // that is derived from an alloca instruction.
          curRes.bits.ResourceID = 2;
          curRes.bits.HardwareInst = 1;
      } else {
        if (isStoreInst(ATM->getInstrInfo(), MI)) {
          if (mDebug) {
            dbgs() << __LINE__ << ": Setting byte store bit on instruction: ";
            MI->dump();
          }
          curRes.bits.ByteStore = 1;
        }
        curRes.bits.ResourceID = STM->device()->getResourceID(AMDILDevice::CONSTANT_ID);
      }
    } else {
      if (isStoreInst(ATM->getInstrInfo(), MI)) {
        if (mDebug) {
          dbgs() << __LINE__ << ": Setting byte store bit on instruction: ";
          MI->dump();
        }
        curRes.bits.ByteStore = 1;
      }
      curRes.bits.ResourceID = STM->device()->getResourceID(AMDILDevice::GLOBAL_ID);
      AMDILKernelManager *KM = STM->getKernelManager();
      KM->setUAVID(NULL, curRes.bits.ResourceID);
      mMFI->uav_insert(curRes.bits.ResourceID);
    }
  } else if (isAppendInst(ATM->getInstrInfo(), MI)) {
    unsigned opcode = MI->getOpcode();
    if (opcode == AMDIL::APPEND_ALLOC
        || opcode == AMDIL::APPEND_ALLOC_NORET) {
      curRes.bits.ResourceID = 1;
    } else {
      curRes.bits.ResourceID = 2;
    }
  }
  setAsmPrinterFlags(MI, curRes);
}

// Function that parses the arguments and updates the lookupTable with the
// pointer -> register mapping. This function also checks for cacheable
// pointers and updates the CacheableSet with the arguments that
// can be cached based on the readonlypointer annotation. The final
// purpose of this function is to update the imageSet and counterSet
// with all pointers that are either images or atomic counters.
uint32_t
parseArguments(MachineFunction &MF,
    RVPVec &lookupTable,
    const AMDILTargetMachine *ATM,
    CacheableSet &cacheablePtrs,
    ImageSet &imageSet,
    AppendSet &counterSet,
    bool mDebug)
{
  const AMDILSubtarget *STM
    = ATM->getSubtargetImpl();
  uint32_t writeOnlyImages = 0;
  uint32_t readOnlyImages = 0;
  std::string cachedKernelName = "llvm.readonlypointer.annotations.";
  cachedKernelName.append(MF.getFunction()->getName());
  GlobalVariable *GV = MF.getFunction()->getParent()
    ->getGlobalVariable(cachedKernelName);
  unsigned cbNum = 0;
  unsigned regNum = AMDIL::R1;
  AMDILMachineFunctionInfo *mMFI = MF.getInfo<AMDILMachineFunctionInfo>();
  for (Function::const_arg_iterator I = MF.getFunction()->arg_begin(),
      E = MF.getFunction()->arg_end(); I != E; ++I) {
    const Argument *curArg = I;
    if (mDebug) {
      dbgs() << "Argument: ";
      curArg->dump();
    }
    Type *curType = curArg->getType();
    // We are either a scalar or vector type that
    // is passed by value that is not a opaque/struct
    // type. We just need to increment regNum
    // the correct number of times to match the number
    // of registers that it takes up.
    if (curType->isFPOrFPVectorTy() ||
        curType->isIntOrIntVectorTy()) {
      // We are scalar, so increment once and
      // move on
      if (!curType->isVectorTy()) {
        lookupTable[regNum] = std::make_pair<unsigned, const Value*>(~0U, curArg);
        ++regNum;
        ++cbNum;
        continue;
      }
      VectorType *VT = dyn_cast<VectorType>(curType);
      // We are a vector type. If we are 64bit type, then
      // we increment length / 2 times, otherwise we
      // increment length / 4 times. The only corner case
      // is with vec3 where the vector gets scalarized and
      // therefor we need a loop count of 3.
      size_t loopCount = VT->getNumElements();
      if (loopCount != 3) {
        if (VT->getScalarSizeInBits() == 64) {
          loopCount = loopCount >> 1;
        } else {
          loopCount = (loopCount + 2) >> 2;
        }
        cbNum += loopCount;
      } else {
        cbNum++;
      }
      while (loopCount--) {
        lookupTable[regNum] = std::make_pair<unsigned, const Value*>(~0U, curArg);
        ++regNum;
      }
    } else if (curType->isPointerTy()) {
      Type *CT = dyn_cast<PointerType>(curType)->getElementType();
      const StructType *ST = dyn_cast<StructType>(CT);
      if (ST && ST->isOpaque()) {
        StringRef name = ST->getName();
        bool i1d_type  = name == "struct._image1d_t";
        bool i1da_type = name == "struct._image1d_array_t";
        bool i1db_type = name == "struct._image1d_buffer_t";
        bool i2d_type  = name == "struct._image2d_t";
        bool i2da_type = name == "struct._image2d_array_t";
        bool i3d_type  = name == "struct._image3d_t";
        bool c32_type  = name == "struct._counter32_t";
        bool c64_type  = name == "struct._counter64_t";
        if (i2d_type || i3d_type || i2da_type ||
            i1d_type || i1db_type || i1da_type) {
          imageSet.insert(I);
          uint32_t imageNum = readOnlyImages + writeOnlyImages;
          if (STM->getGlobalManager()
              ->isReadOnlyImage(MF.getFunction()->getName(), imageNum)) {
            if (mDebug) {
              dbgs() << "Pointer: '" << curArg->getName()
                << "' is a read only image # " << readOnlyImages << "!\n";
            }
            // We store the cbNum along with the image number so that we can
            // correctly encode the 'info' intrinsics.
            lookupTable[regNum] = std::make_pair<unsigned, const Value*>
              ((cbNum << 16 | readOnlyImages++), curArg);
          } else if (STM->getGlobalManager()
              ->isWriteOnlyImage(MF.getFunction()->getName(), imageNum)) {
            if (mDebug) {
              dbgs() << "Pointer: '" << curArg->getName()
                << "' is a write only image # " << writeOnlyImages << "!\n";
            }
            // We store the cbNum along with the image number so that we can
            // correctly encode the 'info' intrinsics.
            lookupTable[regNum] = std::make_pair<unsigned, const Value*>
              ((cbNum << 16 | writeOnlyImages++), curArg);
          } else {
            assert(!"Read/Write images are not supported!");
          }
          ++regNum;
          cbNum += 2;
          continue;
        } else if (c32_type || c64_type) {
          if (mDebug) {
            dbgs() << "Pointer: '" << curArg->getName()
              << "' is a " << (c32_type ? "32" : "64")
              << " bit atomic counter type!\n";
          }
          counterSet.push_back(I);
        }
      }

      if (STM->device()->isSupported(AMDILDeviceInfo::CachedMem) 
          && GV && GV->hasInitializer()) {
        const ConstantArray *nameArray 
          = dyn_cast_or_null<ConstantArray>(GV->getInitializer());
        if (nameArray) {
          for (unsigned x = 0, y = nameArray->getNumOperands(); x < y; ++x) {
            const GlobalVariable *gV= dyn_cast_or_null<GlobalVariable>(
                nameArray->getOperand(x)->getOperand(0));
            const ConstantDataArray *argName =
              dyn_cast_or_null<ConstantDataArray>(gV->getInitializer());
            if (!argName) {
              continue;
            }
            std::string argStr = argName->getAsString();
            std::string curStr = curArg->getName();
            if (!strcmp(argStr.data(), curStr.data())) {
              if (mDebug) {
                dbgs() << "Pointer: '" << curArg->getName() 
                  << "' is cacheable!\n";
              }
              cacheablePtrs.insert(curArg);
            }
          }
        }
      }
      uint32_t as = dyn_cast<PointerType>(curType)->getAddressSpace();
      // Handle the case where the kernel argument is a pointer
      if (mDebug) {
        dbgs() << "Pointer: " << curArg->getName() << " is assigned ";
        if (as == AMDILAS::GLOBAL_ADDRESS) {
          dbgs() << "uav " << STM->device()
            ->getResourceID(AMDILDevice::GLOBAL_ID);
        } else if (as == AMDILAS::PRIVATE_ADDRESS) {
          dbgs() << "scratch " << STM->device()
            ->getResourceID(AMDILDevice::SCRATCH_ID);
        } else if (as == AMDILAS::LOCAL_ADDRESS) {
          dbgs() << "lds " << STM->device()
            ->getResourceID(AMDILDevice::LDS_ID);
        } else if (as == AMDILAS::CONSTANT_ADDRESS) {
          dbgs() << "cb " << STM->device()
            ->getResourceID(AMDILDevice::CONSTANT_ID);
        } else if (as == AMDILAS::REGION_ADDRESS) {
          dbgs() << "gds " << STM->device()
            ->getResourceID(AMDILDevice::GDS_ID);
        } else {
          assert(!"Found an address space that we don't support!");
        }
        dbgs() << " @ register " << regNum << ". Inst: ";
        curArg->dump();
      }
      switch (as) {
        default:
          lookupTable[regNum] = std::make_pair<unsigned, const Value*>
            (STM->device()->getResourceID(AMDILDevice::GLOBAL_ID), curArg);
          break;
        case AMDILAS::LOCAL_ADDRESS:
          lookupTable[regNum] = std::make_pair<unsigned, const Value*>
            (STM->device()->getResourceID(AMDILDevice::LDS_ID), curArg);
          mMFI->setHasLocalArg();
          break;
        case AMDILAS::REGION_ADDRESS:
          lookupTable[regNum] = std::make_pair<unsigned, const Value*>
            (STM->device()->getResourceID(AMDILDevice::GDS_ID), curArg);
          mMFI->setHasRegionArg();
          break;
        case AMDILAS::CONSTANT_ADDRESS:
          lookupTable[regNum] = std::make_pair<unsigned, const Value*>
            (STM->device()->getResourceID(AMDILDevice::CONSTANT_ID), curArg);
          break;
        case AMDILAS::PRIVATE_ADDRESS:
          lookupTable[regNum] = std::make_pair<unsigned, const Value*>
            (STM->device()->getResourceID(AMDILDevice::SCRATCH_ID), curArg);
          break;
      }
      // In this case we need to increment it once.
      ++regNum;
      ++cbNum;
    } else {
      // Is anything missing that is legal in CL?
      assert(0 && "Current type is not supported!");
      lookupTable[regNum] = std::make_pair<unsigned, const Value*>
        (STM->device()->getResourceID(AMDILDevice::GLOBAL_ID), curArg);
      ++regNum;
      ++cbNum;
    }
  }
  return writeOnlyImages;
}
// The call stack is interesting in that even in SSA form, it assigns
// registers to the same value's over and over again. So we need to
// ignore the values that are assigned and just deal with the input
// and return registers.
static void
parseCall(
    const AMDILTargetMachine *ATM,
    InstPMap &InstToPtrMap,
    PtrIMap &PtrToInstMap,
    RVPVec &lookupTable,
    MachineBasicBlock::iterator &mBegin,
    MachineBasicBlock::iterator mEnd,
    bool mDebug)
{
  SmallVector<unsigned, 8> inputRegs;
  AMDILAS::InstrResEnc curRes;
  if (mDebug) {
    dbgs() << "Parsing Call Stack Start.\n";
  }
  MachineBasicBlock::iterator callInst = mBegin;
  MachineInstr *CallMI = callInst;
  getAsmPrinterFlags(CallMI, curRes);
  MachineInstr *MI = --mBegin;
  unsigned reg = AMDIL::R1;
  // First we need to check the input registers.
  do {
    // We stop if we hit the beginning of the call stack
    // adjustment.
    if (MI->getOpcode() == AMDIL::ADJCALLSTACKDOWN
        || MI->getOpcode() == AMDIL::ADJCALLSTACKUP
        || MI->getNumOperands() != 2
        || !MI->getOperand(0).isReg()) {
      break;
    }
    reg = MI->getOperand(0).getReg();
    if (MI->getOperand(1).isReg()) {
      unsigned reg1 = MI->getOperand(1).getReg();
      inputRegs.push_back(reg1);
      if (lookupTable[reg1].second) {
        curRes.bits.PointerPath = 1;
      }
    }
    lookupTable.erase(reg);
    if ((signed)reg < 0 
    || mBegin == CallMI->getParent()->begin()) {
      break;
    }
    MI = --mBegin;
  } while (1);
  mBegin = callInst;
  MI = ++mBegin;
  // If the next registers operand 1 is not a register or that register
  // is not R1, then we don't have any return values.
  if (MI->getNumOperands() == 2 
      && MI->getOperand(1).isReg() 
      && MI->getOperand(1).getReg() == AMDIL::R1) {
    // Next we check the output register.
    reg = MI->getOperand(0).getReg();
    // Now we link the inputs to the output.
    for (unsigned x = 0; x < inputRegs.size(); ++x) {
      if (lookupTable[inputRegs[x]].second) {
        curRes.bits.PointerPath = 1;
        lookupTable[reg] = lookupTable[inputRegs[x]];
        InstToPtrMap[CallMI].insert(
            lookupTable[reg].second);
        break;
      }
    }
    lookupTable.erase(MI->getOperand(1).getReg());
  }
  setAsmPrinterFlags(CallMI, curRes);
  if (mDebug) {
    dbgs() << "Parsing Call Stack End.\n";
  }
  return;
}

// Detect if the current instruction conflicts with another instruction
// and add the instruction to the correct location accordingly.
static void
detectConflictInst(
    MachineInstr *MI,
    AMDILAS::InstrResEnc &curRes,
    RVPVec &lookupTable,
    InstPMap &InstToPtrMap,
    bool isLoadStore,
    unsigned reg,
    unsigned dstReg,
    bool mDebug)
{
  // If the instruction does not have a point path flag
  // associated with it, then we know that no other pointer
  // hits this instruciton.
  if (!curRes.bits.PointerPath) {
    if (dyn_cast<PointerType>(lookupTable[reg].second->getType())) {
      curRes.bits.PointerPath = 1;
    }
    // We don't want to transfer to the register number
    // between load/store because the load dest can be completely
    // different pointer path and the store doesn't have a real
    // destination register.
    if (!isLoadStore) {
      if (mDebug) {
        if (dyn_cast<PointerType>(lookupTable[reg].second->getType())) {
          dbgs() << "Pointer: " << lookupTable[reg].second->getName();
          assert(dyn_cast<PointerType>(lookupTable[reg].second->getType())
              && "Must be a pointer type for an instruction!");
          switch (dyn_cast<PointerType>(
                lookupTable[reg].second->getType())->getAddressSpace())
          {
            case AMDILAS::GLOBAL_ADDRESS:  dbgs() << " UAV: "; break;
            case AMDILAS::LOCAL_ADDRESS: dbgs() << " LDS: "; break;
            case AMDILAS::REGION_ADDRESS: dbgs() << " GDS: "; break;
            case AMDILAS::PRIVATE_ADDRESS: dbgs() << " SCRATCH: "; break;
            case AMDILAS::CONSTANT_ADDRESS: dbgs() << " CB: "; break;

          }
          dbgs() << lookupTable[reg].first << " Reg: " << reg
            << " assigned to reg " << dstReg << ". Inst: ";
          MI->dump();
        }
      }
      // We don't want to do any copies if the register is not virtual
      // as it is the result of a CALL. ParseCallInst handles the
      // case where the input and output need to be linked up 
      // if it occurs. The easiest way to check for virtual
      // is to check the top bit.
      lookupTable[dstReg] = lookupTable[reg];
    }
  } else {
    if (dyn_cast<PointerType>(lookupTable[reg].second->getType())) {
      // Otherwise we have a conflict between two pointers somehow.
      curRes.bits.ConflictPtr = 1;
      if (mDebug) {
        dbgs() << "Pointer: " << lookupTable[reg].second->getName();
        assert(dyn_cast<PointerType>(lookupTable[reg].second->getType())
            && "Must be a pointer type for a conflict instruction!");
        switch (dyn_cast<PointerType>(
              lookupTable[reg].second->getType())->getAddressSpace())
        {
          case AMDILAS::GLOBAL_ADDRESS:  dbgs() << " UAV: "; break;
          case AMDILAS::LOCAL_ADDRESS: dbgs() << " LDS: "; break;
          case AMDILAS::REGION_ADDRESS: dbgs() << " GDS: "; break;
          case AMDILAS::PRIVATE_ADDRESS: dbgs() << " SCRATCH: "; break;
          case AMDILAS::CONSTANT_ADDRESS: dbgs() << " CB: "; break;

        }
        dbgs() << lookupTable[reg].first << " Reg: " << reg;
        if (InstToPtrMap[MI].size() > 1) {
          dbgs() << " conflicts with:\n ";
          for (PtrSet::iterator psib = InstToPtrMap[MI].begin(),
              psie = InstToPtrMap[MI].end(); psib != psie; ++psib) {
            dbgs() << "\t\tPointer: " << (*psib)->getName() << " ";
            assert(dyn_cast<PointerType>((*psib)->getType())
                && "Must be a pointer type for a conflict instruction!");
            (*psib)->dump();
          }
        } else {
          dbgs() << ".";
        }
        dbgs() << " Inst: ";
        MI->dump();
      }
    }
    // Add the conflicting values to the pointer set for the instruction
    InstToPtrMap[MI].insert(lookupTable[reg].second);
    // We don't want to add the destination register if
    // we are a load or store.
    if (!isLoadStore) {
      InstToPtrMap[MI].insert(lookupTable[dstReg].second);
    }
  }
  setAsmPrinterFlags(MI, curRes);
}

// In this case we want to handle a load instruction.
static void
parseLoadInst(
    const AMDILTargetMachine *ATM,
    InstPMap &InstToPtrMap,
    PtrIMap &PtrToInstMap,
    FIPMap &FIToPtrMap,
    RVPVec &lookupTable,
    CPoolSet &cpool,
    BlockCacheableInfo &bci,
    MachineInstr *MI,
    bool mDebug)
{
  assert(isLoadInst(ATM->getInstrInfo(), MI) && "Only a load instruction can be parsed by "
      "the parseLoadInst function.");
  AMDILAS::InstrResEnc curRes;
  getAsmPrinterFlags(MI, curRes);
  unsigned dstReg = MI->getOperand(0).getReg();
  unsigned idx = 0;
  const Value *basePtr = NULL;
  if (MI->getOperand(1).isReg()) {
    idx = MI->getOperand(1).getReg();
    basePtr = lookupTable[idx].second;
    // If we don't know what value the register
    // is assigned to, then we need to special case
    // this instruction.
  } else if (MI->getOperand(1).isFI()) {
    idx = MI->getOperand(1).getIndex();
    lookupTable[dstReg] = FIToPtrMap[idx];
  } else if (MI->getOperand(1).isCPI()) {
    cpool.insert(MI);
  } 
  // If we are a hardware local, then we don't need to track as there
  // is only one resource ID that we need to know about, so we
  // map it using allocateDefaultID, which maps it to the default.
  // This is also the case for REGION_ADDRESS and PRIVATE_ADDRESS.
  if (isLRPInst(MI, ATM) || !basePtr) {
    allocateDefaultID(ATM, curRes, MI, mDebug);
    return;
  }
  // We have a load instruction so we map this instruction
  // to the pointer and insert it into the set of known
  // load instructions.
  InstToPtrMap[MI].insert(basePtr);
  PtrToInstMap[basePtr].push_back(MI);

  if (isGlobalInst(ATM->getInstrInfo(), MI)) {
    // Add to the cacheable set for the block. If there was a store earlier
    // in the block, this call won't actually add it to the cacheable set.
    bci.addPossiblyCacheableInst(ATM, MI);
  }

  if (mDebug) {
    dbgs() << "Assigning instruction to pointer ";
    dbgs() << basePtr->getName() << ". Inst: ";
    MI->dump();
  }
  detectConflictInst(MI, curRes, lookupTable, InstToPtrMap, true,
      idx, dstReg, mDebug);
}

// In this case we want to handle a store instruction.
static void
parseStoreInst(
    const AMDILTargetMachine *ATM,
    InstPMap &InstToPtrMap,
    PtrIMap &PtrToInstMap,
    FIPMap &FIToPtrMap,
    RVPVec &lookupTable,
    CPoolSet &cpool,
    BlockCacheableInfo &bci,
    MachineInstr *MI,
    ByteSet &bytePtrs,
    ConflictSet &conflictPtrs,
    bool mDebug)
{
  assert(isStoreInst(ATM->getInstrInfo(), MI) && "Only a store instruction can be parsed by "
      "the parseStoreInst function.");
  AMDILAS::InstrResEnc curRes;
  getAsmPrinterFlags(MI, curRes);
  unsigned dstReg = MI->getOperand(0).getReg();

  // If the data part of the store instruction is known to
  // be a pointer, then we need to mark this pointer as being
  // a byte pointer. This is the conservative case that needs
  // to be handled correctly.
  if (lookupTable[dstReg].second && lookupTable[dstReg].first != ~0U) {
    curRes.bits.ConflictPtr = 1;
    if (mDebug) {
      dbgs() << "Found a case where the pointer is being stored!\n";
      MI->dump();
      dbgs() << "Pointer is ";
      lookupTable[dstReg].second->print(dbgs());
      dbgs() << "\n";
    }
    //PtrToInstMap[lookupTable[dstReg].second].push_back(MI);
    if (lookupTable[dstReg].second->getType()->isPointerTy()) {
      conflictPtrs.insert(lookupTable[dstReg].second);
    }
  }

  // Before we go through the special cases, for the cacheable information
  // all we care is if the store if global or not.
  if (!isLRPInst(MI, ATM)) {
    bci.setReachesExit();
  }

  // If the address is not a register address,
  // then we need to lower it as an unknown id.
  if (!MI->getOperand(1).isReg()) {
    if (MI->getOperand(1).isCPI()) {
      if (mDebug) {
        dbgs() << "Found an instruction with a CPI index #"
          << MI->getOperand(1).getIndex() << "!\n";
      }
      cpool.insert(MI);
    } else if (MI->getOperand(1).isFI()) {
      if (mDebug) {
        dbgs() << "Found an instruction with a frame index #"
          << MI->getOperand(1).getIndex() << "!\n";
      }
      // If we are a frame index and we are storing a pointer there, lets
      // go ahead and assign the pointer to the location within the frame
      // index map so that we can get the value out later.
      FIToPtrMap[MI->getOperand(1).getIndex()] = lookupTable[dstReg];
    }

    allocateDefaultID(ATM, curRes, MI, mDebug);
    return;
  }
  unsigned reg = MI->getOperand(1).getReg();
  // If we don't know what value the register
  // is assigned to, then we need to special case
  // this instruction.
  if (!lookupTable[reg].second) {
    allocateDefaultID(ATM, curRes, MI, mDebug);
    return;
  }
  // const Value *basePtr = lookupTable[reg].second;
  // If we are a hardware local, then we don't need to track as there
  // is only one resource ID that we need to know about, so we
  // map it using allocateDefaultID, which maps it to the default.
  // This is also the case for REGION_ADDRESS and PRIVATE_ADDRESS.
  if (isLRPInst(MI, ATM)) {
    allocateDefaultID(ATM, curRes, MI, mDebug);
    return;
  }

  // We have a store instruction so we map this instruction
  // to the pointer and insert it into the set of known
  // store instructions.
  InstToPtrMap[MI].insert(lookupTable[reg].second);
  PtrToInstMap[lookupTable[reg].second].push_back(MI);
  uint16_t RegClass = MI->getDesc().OpInfo[0].RegClass;
  switch (RegClass) {
    default:
      break;
    case AMDIL::GPRI8RegClassID:
    case AMDIL::GPRV2I8RegClassID:
    case AMDIL::GPRI16RegClassID:
      if (usesGlobal(ATM, MI)) {
        if (mDebug) {
          dbgs() << "Annotating instruction as Byte Store. Inst: ";
          MI->dump();
        }
        curRes.bits.ByteStore = 1;
        setAsmPrinterFlags(MI, curRes);
        const PointerType *PT = dyn_cast<PointerType>(
            lookupTable[reg].second->getType());
        if (PT) {
          bytePtrs.insert(lookupTable[reg].second);
        }
      }
      break;
  };
  // If we are a truncating store, then we need to determine the
  // size of the pointer that we are truncating to, and if we
  // are less than 32 bits, we need to mark the pointer as a
  // byte store pointer.
  switch (MI->getOpcode()) {
    case AMDIL::GLOBALTRUNCSTORE_i16i8:
    case AMDIL::GLOBALTRUNCSTORE_v2i16i8:
    case AMDIL::GLOBALTRUNCSTORE_i32i8:
    case AMDIL::GLOBALTRUNCSTORE_v2i32i8:
    case AMDIL::GLOBALTRUNCSTORE_i64i8:
    case AMDIL::GLOBALTRUNCSTORE_v2i64i8:
    case AMDIL::GLOBALTRUNCSTORE_i32i16:
    case AMDIL::GLOBALTRUNCSTORE_i64i16:
    case AMDIL::GLOBALSTORE_i8:
    case AMDIL::GLOBALSTORE_i16:
      curRes.bits.ByteStore = 1;
      setAsmPrinterFlags(MI, curRes);
      bytePtrs.insert(lookupTable[reg].second);
      break;
    default:
      break;
  }

  if (mDebug) {
    dbgs() << "Assigning instruction to pointer ";
    dbgs() << lookupTable[reg].second->getName() << ". Inst: ";
    MI->dump();
  }
  detectConflictInst(MI, curRes, lookupTable, InstToPtrMap, true,
      reg, dstReg, mDebug);
}

// In this case we want to handle an atomic instruction.
static void
parseAtomicInst(
    const AMDILTargetMachine *ATM,
    InstPMap &InstToPtrMap,
    PtrIMap &PtrToInstMap,
    RVPVec &lookupTable,
    BlockCacheableInfo &bci,
    MachineInstr *MI,
    ByteSet &bytePtrs,
    bool mDebug)
{
  assert(isAtomicInst(ATM->getInstrInfo(), MI) && "Only an atomic instruction can be parsed by "
      "the parseAtomicInst function.");
  AMDILAS::InstrResEnc curRes;
  unsigned dstReg = MI->getOperand(0).getReg();
  unsigned reg = 0;
  getAsmPrinterFlags(MI, curRes);
  unsigned numOps = MI->getNumOperands();
  bool found = false;
  while (--numOps) {
    MachineOperand &Op = MI->getOperand(numOps);
    if (!Op.isReg()) {
      continue;
    }
    reg = Op.getReg();
    // If the register is not known to be owned by a pointer
    // then we can ignore it
    if (!lookupTable[reg].second) {
      continue;
    }
    // if the pointer is known to be local, region or private, then we
    // can ignore it.  Although there are no private atomics, we still
    // do this check so we don't have to write a new function to check
    // for only local and region.
    if (isLRPInst(MI, ATM)) {
      continue;
    }
    found = true;
    InstToPtrMap[MI].insert(lookupTable[reg].second);
    PtrToInstMap[lookupTable[reg].second].push_back(MI);

	// We now know we have an atomic operation on global memory.
	// This is a store so must update the cacheable information.
    bci.setReachesExit();

	// Only do if have SC with arena atomic bug fix (EPR 326883).
	// TODO: enable once SC with EPR 326883 has been promoted to CAL.
    if (ATM->getSubtargetImpl()->calVersion() >= CAL_VERSION_SC_150) {
      // Force pointers that are used by atomics to be in the arena.
      // If they were allowed to be accessed as RAW they would cause
      // all access to use the slow complete path.
      if (mDebug) {
        dbgs() << __LINE__ << ": Setting byte store bit on atomic instruction: ";
        MI->dump();
      }
	  curRes.bits.ByteStore = 1;
      bytePtrs.insert(lookupTable[reg].second);
	}

    if (mDebug) {
      dbgs() << "Assigning instruction to pointer ";
      dbgs() << lookupTable[reg].second->getName() << ". Inst: ";
      MI->dump();
    }
    detectConflictInst(MI, curRes, lookupTable, InstToPtrMap, true,
        reg, dstReg, mDebug);
  }
  if (!found) {
    allocateDefaultID(ATM, curRes, MI, mDebug);
  }
}
// In this case we want to handle a counter instruction.
static void
parseAppendInst(
    const AMDILTargetMachine *ATM,
    InstPMap &InstToPtrMap,
    PtrIMap &PtrToInstMap,
    RVPVec &lookupTable,
    MachineInstr *MI,
    bool mDebug)
{
  assert(isAppendInst(ATM->getInstrInfo(), MI) && "Only an atomic counter instruction can be "
      "parsed by the parseAppendInst function.");
  AMDILAS::InstrResEnc curRes;
  unsigned dstReg = MI->getOperand(0).getReg();
  unsigned reg = MI->getOperand(1).getReg();
  getAsmPrinterFlags(MI, curRes);
  // If the register is not known to be owned by a pointer
  // then we set it to the default
  if (!lookupTable[reg].second) {
    allocateDefaultID(ATM, curRes, MI, mDebug);
    return;
  }
  InstToPtrMap[MI].insert(lookupTable[reg].second);
  PtrToInstMap[lookupTable[reg].second].push_back(MI);
  if (mDebug) {
    dbgs() << "Assigning instruction to pointer ";
    dbgs() << lookupTable[reg].second->getName() << ". Inst: ";
    MI->dump();
  }
  detectConflictInst(MI, curRes, lookupTable, InstToPtrMap, true,
      reg, dstReg, mDebug);
}
// In this case we want to handle an Image instruction.
static void
parseImageInst(
    const AMDILTargetMachine *ATM,
    InstPMap &InstToPtrMap,
    PtrIMap &PtrToInstMap,
    FIPMap &FIToPtrMap,
    RVPVec &lookupTable,
    MachineInstr *MI,
    bool mDebug)
{
  assert(isImageInst(ATM->getInstrInfo(), MI) && "Only an image instruction can be "
      "parsed by the parseImageInst function.");
  AMDILAS::InstrResEnc curRes;
  getAsmPrinterFlags(MI, curRes);
  // AMDILKernelManager *km = 
  //   (AMDILKernelManager *)ATM->getSubtargetImpl()->getKernelManager();
  AMDILMachineFunctionInfo *mMFI = MI->getParent()->getParent()
    ->getInfo<AMDILMachineFunctionInfo>();
  if (MI->getOpcode() == AMDIL::IMAGE2D_WRITE
      || MI->getOpcode() == AMDIL::IMAGE3D_WRITE) {
    unsigned dstReg = MI->getOperand(0).getReg();
    curRes.bits.ResourceID = lookupTable[dstReg].first & 0xFFFF;
    curRes.bits.isImage = 1;
    InstToPtrMap[MI].insert(lookupTable[dstReg].second);
    PtrToInstMap[lookupTable[dstReg].second].push_back(MI);
    if (mDebug) {
      dbgs() << "Assigning instruction to pointer ";
      dbgs() << lookupTable[dstReg].second->getName() << ". Inst: ";
      MI->dump();
    }
  } else {
    // unsigned dstReg = MI->getOperand(0).getReg();
    unsigned reg = MI->getOperand(1).getReg();

    // If the register is not known to be owned by a pointer
    // then we set it to the default
    if (!lookupTable[reg].second) {
      assert(!"This should not happen for images!");
      allocateDefaultID(ATM, curRes, MI, mDebug);
      return;
    }
    InstToPtrMap[MI].insert(lookupTable[reg].second);
    PtrToInstMap[lookupTable[reg].second].push_back(MI);
    if (mDebug) {
      dbgs() << "Assigning instruction to pointer ";
      dbgs() << lookupTable[reg].second->getName() << ". Inst: ";
      MI->dump();
    }    
    switch (MI->getOpcode()) {
      case AMDIL::IMAGE2D_READ:
      case AMDIL::IMAGE2D_READ_UNNORM:
      case AMDIL::IMAGE3D_READ:
      case AMDIL::IMAGE3D_READ_UNNORM:
        curRes.bits.ResourceID = lookupTable[reg].first & 0xFFFF;
        if (MI->getOperand(3).isReg()) {
          // Our sampler is not a literal value.
          char buffer[256];
          memset(buffer, 0, sizeof(buffer));
          std::string sampler_name = "";
          unsigned reg = MI->getOperand(3).getReg();
          if (lookupTable[reg].second) {
            sampler_name = lookupTable[reg].second->getName();
          }
          if (sampler_name.empty()) {
            sampler_name = findSamplerName(MI, lookupTable, FIToPtrMap, ATM);
          }
          uint32_t val = mMFI->addSampler(sampler_name, ~0U);
          if (mDebug) {
            dbgs() << "Mapping kernel sampler " << sampler_name
              << " to sampler number " << val << " for Inst:\n";
            MI->dump();
          }
          MI->getOperand(3).ChangeToImmediate(val);
        } else {
          // Our sampler is known at runtime as a literal, lets make sure
          // that the metadata for it is known.
          char buffer[256];
          memset(buffer, 0, sizeof(buffer));
          sprintf(buffer,"_%d", (int32_t)MI->getOperand(3).getImm());
          std::string sampler_name = std::string("unknown") + std::string(buffer);
          uint32_t val = mMFI->addSampler(sampler_name, MI->getOperand(3).getImm());
          if (mDebug) {
            dbgs() << "Mapping internal sampler " << sampler_name 
              << " to sampler number " << val << " for Inst:\n";
            MI->dump();
          }
          MI->getOperand(3).setImm(val);
        }
        break;
      case AMDIL::IMAGE2D_INFO0:
      case AMDIL::IMAGE3D_INFO0:
        curRes.bits.ResourceID = lookupTable[reg].first >> 16;
        break;
      case AMDIL::IMAGE2D_INFO1:
      case AMDIL::IMAGE2DA_INFO1:
        curRes.bits.ResourceID = (lookupTable[reg].first >> 16) + 1;
        break;
    };
    curRes.bits.isImage = 1;
  }
  setAsmPrinterFlags(MI, curRes);
}
// This case handles the rest of the instructions
static void
parseInstruction(
    const AMDILTargetMachine *ATM,
    InstPMap &InstToPtrMap,
    PtrIMap &PtrToInstMap,
    RVPVec &lookupTable,
    CPoolSet &cpool,
    MachineInstr *MI,
    bool mDebug)
{
  assert(!isAtomicInst(ATM->getInstrInfo(), MI) && !isStoreInst(ATM->getInstrInfo(), MI) && !isLoadInst(ATM->getInstrInfo(), MI) &&
      !isAppendInst(ATM->getInstrInfo(), MI) && !isImageInst(ATM->getInstrInfo(), MI) &&
      "Atomic/Load/Store/Append/Image insts should not be handled here!");
  unsigned numOps = MI->getNumOperands();
  // If we don't have any operands, we can skip this instruction
  if (!numOps) {
    return;
  }
  // if the dst operand is not a register, then we can skip
  // this instruction. That is because we are probably a branch
  // or jump instruction.
  if (!MI->getOperand(0).isReg()) {
    return;
  }
  // If we are a LOADCONST_i32, we might be a sampler, so we need
  // to propogate the LOADCONST to IMAGE[2|3]D_READ instructions.
  if (MI->getOpcode() == AMDIL::LOADCONST_i32) {
    uint32_t val = MI->getOperand(1).getImm();
    MachineOperand* oldPtr = &MI->getOperand(0);
    MachineOperand* moPtr = oldPtr->getNextOperandForReg();
    while (moPtr) {
      oldPtr = moPtr;
      moPtr = oldPtr->getNextOperandForReg();
      switch (oldPtr->getParent()->getOpcode()) {
        default:
          break;
        case AMDIL::IMAGE2D_READ:
        case AMDIL::IMAGE2D_READ_UNNORM:
        case AMDIL::IMAGE3D_READ:
        case AMDIL::IMAGE3D_READ_UNNORM:
          if (mDebug) {
            dbgs() << "Found a constant sampler for image read inst: ";
            oldPtr->getParent()->print(dbgs());
          }
          oldPtr->ChangeToImmediate(val);
          break;
      }
    }
  }
  AMDILAS::InstrResEnc curRes;
  getAsmPrinterFlags(MI, curRes);
  unsigned dstReg = MI->getOperand(0).getReg();
  unsigned reg = 0;
  while (--numOps) {
    MachineOperand &Op = MI->getOperand(numOps);
    // if the operand is not a register, then we can ignore it
    if (!Op.isReg()) {
      if (Op.isCPI()) {
        cpool.insert(MI);
      }
      continue;
    }
    reg = Op.getReg();
    // If the register is not known to be owned by a pointer
    // then we can ignore it
    if (!lookupTable[reg].second) {
      continue;
    }
    detectConflictInst(MI, curRes, lookupTable, InstToPtrMap, false,
        reg, dstReg, mDebug);

  }
}

// This function parses the basic block and based on the instruction type,
// calls the function to finish parsing the instruction.
static void
parseBasicBlock(
    const AMDILTargetMachine *ATM,
    MachineBasicBlock *MB,
    InstPMap &InstToPtrMap,
    PtrIMap &PtrToInstMap,
    FIPMap &FIToPtrMap,
    RVPVec &lookupTable,
    ByteSet &bytePtrs,
    ConflictSet &conflictPtrs, 
    CPoolSet &cpool,
    BlockCacheableInfo &bci,
    bool mDebug)
{
  for (MachineBasicBlock::iterator mbb = MB->begin(), mbe = MB->end();
      mbb != mbe; ++mbb) {
    MachineInstr *MI = mbb;
    if (MI->getOpcode() == AMDIL::CALL) {
      parseCall(ATM, InstToPtrMap, PtrToInstMap, lookupTable,
          mbb, mbe, mDebug);
    } 
    else if (isLoadInst(ATM->getInstrInfo(), MI)) {
      parseLoadInst(ATM, InstToPtrMap, PtrToInstMap,
          FIToPtrMap, lookupTable, cpool, bci, MI, mDebug);
    } else if (isStoreInst(ATM->getInstrInfo(), MI)) {
      parseStoreInst(ATM, InstToPtrMap, PtrToInstMap,
          FIToPtrMap, lookupTable, cpool, bci, MI, bytePtrs, conflictPtrs, mDebug);
    } else if (isAtomicInst(ATM->getInstrInfo(), MI)) {
      parseAtomicInst(ATM, InstToPtrMap, PtrToInstMap,
          lookupTable, bci, MI, bytePtrs, mDebug);
    } else if (isAppendInst(ATM->getInstrInfo(), MI)) {
      parseAppendInst(ATM, InstToPtrMap, PtrToInstMap,
          lookupTable, MI, mDebug);
    } else if (isImageInst(ATM->getInstrInfo(), MI)) {
      parseImageInst(ATM, InstToPtrMap, PtrToInstMap,
          FIToPtrMap, lookupTable, MI, mDebug);
    } else {
      parseInstruction(ATM, InstToPtrMap, PtrToInstMap,
          lookupTable, cpool, MI, mDebug);
    }
  }
}

// Follows the Reverse Post Order Traversal of the basic blocks to
// determine which order to parse basic blocks in.
void
parseFunction(
    const AMDILPointerManager *PM,
    const AMDILTargetMachine *ATM,
    MachineFunction &MF,
    InstPMap &InstToPtrMap,
    PtrIMap &PtrToInstMap,
    FIPMap &FIToPtrMap,
    RVPVec &lookupTable,
    ByteSet &bytePtrs,
    ConflictSet &conflictPtrs,
    CPoolSet &cpool,
    MBBCacheableMap &mbbCacheable,
    bool mDebug)
{
  if (mDebug) {
    MachineDominatorTree *dominatorTree = &PM
      ->getAnalysis<MachineDominatorTree>();
    dominatorTree->dump();
  }

  std::list<MachineBasicBlock*> prop_worklist;

  ReversePostOrderTraversal<MachineFunction*> RPOT(&MF);
  for (ReversePostOrderTraversal<MachineFunction*>::rpo_iterator
      curBlock = RPOT.begin(), endBlock = RPOT.end();
      curBlock != endBlock; ++curBlock) {
    MachineBasicBlock *MB = (*curBlock);
    BlockCacheableInfo &bci = mbbCacheable[MB];
    for (MachineBasicBlock::pred_iterator mbbit = MB->pred_begin(),
        mbbitend = MB->pred_end();
        mbbit != mbbitend;
        mbbit++) {
      MBBCacheableMap::const_iterator mbbcmit = mbbCacheable.find(*mbbit);
      if (mbbcmit != mbbCacheable.end() &&
          mbbcmit->second.storeReachesExit()) {
        bci.setReachesTop();
        break;
      }
    }

    if (mDebug) {
      dbgs() << "[BlockOrdering] Parsing CurrentBlock: "
        << MB->getNumber() << "\n";
    }
    parseBasicBlock(ATM, MB, InstToPtrMap, PtrToInstMap,
        FIToPtrMap, lookupTable, bytePtrs, conflictPtrs, cpool, bci, mDebug);

    if (bci.storeReachesExit())
      prop_worklist.push_back(MB);

    if (mDebug) {
      dbgs() << "BCI info: Top: " << bci.storeReachesTop() << " Exit: " 
        << bci.storeReachesExit() << "\n Instructions:\n";
      for (CacheableInstrSet::const_iterator cibit = bci.cacheableBegin(),
          cibitend = bci.cacheableEnd();
          cibit != cibitend;
          cibit++)
      {
        (*cibit)->dump();
      }
    }
  }

  // This loop pushes any "storeReachesExit" flags into successor
  // blocks until the flags have been fully propagated. This will
  // ensure that blocks that have reachable stores due to loops
  // are labeled appropriately.
  while (!prop_worklist.empty()) {
    MachineBasicBlock *wlb = prop_worklist.front();
    prop_worklist.pop_front();
    for (MachineBasicBlock::succ_iterator mbbit = wlb->succ_begin(),
        mbbitend = wlb->succ_end();
        mbbit != mbbitend;
        mbbit++)
    {
      BlockCacheableInfo &blockCache = mbbCacheable[*mbbit];
      if (!blockCache.storeReachesTop()) {
        blockCache.setReachesTop();
        prop_worklist.push_back(*mbbit);
      }
      if (mDebug) {
        dbgs() << "BCI Prop info: " << (*mbbit)->getNumber() << " Top: " 
          << blockCache.storeReachesTop() << " Exit: " 
          << blockCache.storeReachesExit()
          << "\n";
      }
    }
  }
}

// Helper function that dumps to dbgs() information about
// a pointer set.
  void
dumpPointers(AppendSet &Ptrs, const char *str)
{
  if (Ptrs.empty()) {
    return;
  }
  dbgs() << "[Dump]" << str << " found: " << "\n";
  for (AppendSet::iterator sb = Ptrs.begin();
      sb != Ptrs.end(); ++sb) {
    (*sb)->dump();
  }
  dbgs() << "\n";
}
// Helper function that dumps to dbgs() information about
// a pointer set.
  void
dumpPointers(PtrSet &Ptrs, const char *str)
{
  if (Ptrs.empty()) {
    return;
  }
  dbgs() << "[Dump]" << str << " found: " << "\n";
  for (PtrSet::iterator sb = Ptrs.begin();
      sb != Ptrs.end(); ++sb) {
    (*sb)->dump();
  }
  dbgs() << "\n";
}
// Function that detects all the conflicting pointers and adds
// the pointers that are detected to the conflict set, otherwise
// they are added to the raw or byte set based on their usage.
void
detectConflictingPointers(
    const AMDILTargetMachine *ATM,
    InstPMap &InstToPtrMap,
    ByteSet &bytePtrs,
    RawSet &rawPtrs,
    ConflictSet &conflictPtrs,
    bool mDebug)
{
  if (InstToPtrMap.empty()) {
    return;
  }
  PtrSet aliasedPtrs;
  const AMDILSubtarget *STM = ATM->getSubtargetImpl();
  for (InstPMap::iterator
      mapIter = InstToPtrMap.begin(), iterEnd = InstToPtrMap.end();
      mapIter != iterEnd; ++mapIter) {
    if (mDebug) {
      dbgs() << "Instruction: ";
      (mapIter)->first->dump();
    }
    MachineInstr* MI = mapIter->first;
    AMDILAS::InstrResEnc curRes;
    getAsmPrinterFlags(MI, curRes);
    if (curRes.bits.isImage) {
      continue;
    }
    bool byte = false;
    // We might have a case where more than 1 pointers is going to the same
    // I/O instruction
    if (mDebug) {
      dbgs() << "Base Pointer[s]:\n";
    }
    for (PtrSet::iterator cfIter = mapIter->second.begin(),
        cfEnd = mapIter->second.end(); cfIter != cfEnd; ++cfIter) {
      if (mDebug) {
        (*cfIter)->dump();
      }
      if (bytePtrs.count(*cfIter)) {
        if (mDebug) {
          dbgs() << "Byte pointer found!\n";
        }
        byte = true;
        break;
      }
    }
    if (byte) {
      for (PtrSet::iterator cfIter = mapIter->second.begin(),
          cfEnd = mapIter->second.end(); cfIter != cfEnd; ++cfIter) {
        const Value *ptr = (*cfIter);
        if (isLRPInst(mapIter->first, ATM)) {
          // We don't need to deal with pointers to local/region/private
          // memory regions
          continue;
        }
        if (mDebug) {
          dbgs() << "Adding pointer " << (ptr)->getName()
            << " to byte set!\n";
        }
        const PointerType *PT = dyn_cast<PointerType>(ptr->getType());
        if (PT) {
          bytePtrs.insert(ptr);
        }
      }
    } else {
      for (PtrSet::iterator cfIter = mapIter->second.begin(),
          cfEnd = mapIter->second.end(); cfIter != cfEnd; ++cfIter) {
        const Value *ptr = (*cfIter);
        // bool aliased = false;
        if (isLRPInst(mapIter->first, ATM)) {
          // We don't need to deal with pointers to local/region/private
          // memory regions
          continue;
        }
        const Argument *arg = dyn_cast_or_null<Argument>(*cfIter);
        if (!arg) {
          continue;
        }
        if (!STM->device()->isSupported(AMDILDeviceInfo::NoAlias) 
            && !arg->hasNoAliasAttr()) {
          if (mDebug) {
            dbgs() << "Possible aliased pointer found!\n";
          }
          aliasedPtrs.insert(ptr);
        }
        if (mapIter->second.size() > 1) {
          if (mDebug) {
            dbgs() << "Adding pointer " << ptr->getName()
              << " to conflict set!\n";
          }
          const PointerType *PT = dyn_cast<PointerType>(ptr->getType());
          if (PT) {
            conflictPtrs.insert(ptr);
          }
        }
        if (mDebug) {
          dbgs() << "Adding pointer " << ptr->getName()
            << " to raw set!\n";
        }
        const PointerType *PT = dyn_cast<PointerType>(ptr->getType());
        if (PT) {
          rawPtrs.insert(ptr);
        }
      }
    }
    if (mDebug) {
      dbgs() << "\n";
    }
  }
  // If we have any aliased pointers and byte pointers exist,
  // then make sure that all of the aliased pointers are 
  // part of the byte pointer set.
  if (!bytePtrs.empty()) {
    for (PtrSet::iterator aIter = aliasedPtrs.begin(),
        aEnd = aliasedPtrs.end(); aIter != aEnd; ++aIter) {
      if (mDebug) {
        dbgs() << "Moving " << (*aIter)->getName() 
          << " from raw to byte.\n";
      }
      bytePtrs.insert(*aIter);
      rawPtrs.erase(*aIter);
    }
  }
}
// Function that detects aliased constant pool operations.
void
detectAliasedCPoolOps(
    TargetMachine &TM,
    CPoolSet &cpool,
    bool mDebug
    )
{
  const AMDILSubtarget *STM = &TM.getSubtarget<AMDILSubtarget>();
  if (mDebug && !cpool.empty()) {
    dbgs() << "Instructions w/ CPool Ops: \n";
  }
  // The algorithm for detecting aliased cpool is as follows.
  // For each instruction that has a cpool argument
  // follow def-use chain
  //   if instruction is a load and load is a private load,
  //      switch to constant pool load
  for (CPoolSet::iterator cpb = cpool.begin(), cpe = cpool.end();
      cpb != cpe; ++cpb) {
    if (mDebug) {
      (*cpb)->dump();
    }
    std::queue<MachineInstr*> queue;
    std::set<MachineInstr*> visited;
    queue.push(*cpb);
    MachineInstr *cur;
    while (!queue.empty()) {
      cur = queue.front();
      queue.pop();
      if (visited.count(cur)) {
        continue;
      }
      if (isLoadInst(TM.getInstrInfo(), cur) && isPrivateInst(TM.getInstrInfo(), cur)) { 
        // If we are a private load and the register is
        // used in the address register, we need to
        // switch from private to constant pool load.
        if (mDebug) {
          dbgs() << "Found an instruction that is a private load "
            << "but should be a constant pool load.\n";
          cur->print(dbgs());
          dbgs() << "\n";
        }
        AMDILAS::InstrResEnc curRes;
        getAsmPrinterFlags(cur, curRes);
        curRes.bits.ResourceID = STM->device()->getResourceID(AMDILDevice::GLOBAL_ID);
        curRes.bits.ConflictPtr = 1;
        setAsmPrinterFlags(cur, curRes);
        cur->setDesc(TM.getInstrInfo()->get(
              (cur->getOpcode() - AMDIL::PRIVATEAEXTLOAD_f32) 
              + AMDIL::CPOOLAEXTLOAD_f32));
      } else {
        if (cur->getOperand(0).isReg()) {
          MachineOperand* ptr = cur->getOperand(0).getNextOperandForReg();
          while (ptr && !ptr->isDef() && ptr->isReg()) {
            queue.push(ptr->getParent());
            ptr = ptr->getNextOperandForReg();
          } 
        }
      }
      visited.insert(cur);
    }
  }
}
// Function that detects fully cacheable pointers. Fully cacheable pointers
// are pointers that have no writes to them and -fno-alias is specified.
void
detectFullyCacheablePointers(
    const AMDILTargetMachine *ATM,
    PtrIMap &PtrToInstMap,
    RawSet &rawPtrs,
    CacheableSet &cacheablePtrs,
    ConflictSet &conflictPtrs,
    bool mDebug
    )
{
  if (PtrToInstMap.empty()) {
    return;
  }
  const AMDILSubtarget *STM
    = ATM->getSubtargetImpl();
  // 4XXX hardware doesn't support cached uav opcodes and we assume
  // no aliasing for this to work. Also in debug mode we don't do
  // any caching.
  if (STM->device()->getGeneration() == AMDILDeviceInfo::HD4XXX
      || !STM->device()->isSupported(AMDILDeviceInfo::CachedMem)) {
    return;
  }
  if (STM->device()->isSupported(AMDILDeviceInfo::NoAlias)) {
    for (PtrIMap::iterator mapIter = PtrToInstMap.begin(), 
        iterEnd = PtrToInstMap.end(); mapIter != iterEnd; ++mapIter) {
      if (mDebug) {
        dbgs() << "Instruction: ";
        mapIter->first->dump();
      }
      // Skip the pointer if we have already detected it.
      if (cacheablePtrs.count(mapIter->first)) {
        continue;
      }
      bool cacheable = true;
      for (std::vector<MachineInstr*>::iterator 
          miBegin = mapIter->second.begin(),
          miEnd = mapIter->second.end(); miBegin != miEnd; ++miBegin) {
        if (isStoreInst(ATM->getInstrInfo(), *miBegin) ||
            isImageInst(ATM->getInstrInfo(), *miBegin) ||
            isAtomicInst(ATM->getInstrInfo(), *miBegin)) {
          cacheable = false;
          break;
        }
      }
      // we aren't cacheable, so lets move on to the next instruction
      if (!cacheable) {
        continue;
      }
      // If we are in the conflict set, lets move to the next instruction
      // FIXME: we need to check to see if the pointers that conflict with
      // the current pointer are also cacheable. If they are, then add them
      // to the cacheable list and not fail.
      if (conflictPtrs.count(mapIter->first)) {
        continue;
      }
      // Otherwise if we have no stores and no conflicting pointers, we can
      // be added to the cacheable set.
      if (mDebug) {
        dbgs() << "Adding pointer " << mapIter->first->getName();
        dbgs() << " to cached set!\n";
      }
      const PointerType *PT = dyn_cast<PointerType>(mapIter->first->getType());
      if (PT) {
        cacheablePtrs.insert(mapIter->first);
      }
    }
  }
}

// Are any of the pointers in PtrSet also in the BytePtrs or the CachePtrs?
static bool
ptrSetIntersectsByteOrCache(
    PtrSet &cacheSet,
    ByteSet &bytePtrs,
    CacheableSet &cacheablePtrs
    )
{
  for (PtrSet::const_iterator psit = cacheSet.begin(),
      psitend = cacheSet.end();
      psit != psitend;
      psit++) {
    if (bytePtrs.find(*psit) != bytePtrs.end() ||
        cacheablePtrs.find(*psit) != cacheablePtrs.end()) {
      return true;
    }
  }
  return false;
}

// Function that detects which instructions are cacheable even if
// all instructions of the pointer are not cacheable. The resulting
// set of instructions will not contain Ptrs that are in the cacheable
// ptr set (under the assumption they will get marked cacheable already)
// or pointers in the byte set, since they are not cacheable.
void
detectCacheableInstrs(
    MBBCacheableMap &bbCacheable,
    InstPMap &InstToPtrMap,
    CacheableSet &cacheablePtrs,
    ByteSet &bytePtrs,
    CacheableInstrSet &cacheableSet,
    bool mDebug
    )

{
  for (MBBCacheableMap::const_iterator mbbcit = bbCacheable.begin(),
      mbbcitend = bbCacheable.end();
      mbbcit != mbbcitend;
      mbbcit++) {
    for (CacheableInstrSet::const_iterator bciit 
        = mbbcit->second.cacheableBegin(),
        bciitend
        = mbbcit->second.cacheableEnd();
        bciit != bciitend;
        bciit++) {
      if (!ptrSetIntersectsByteOrCache(InstToPtrMap[*bciit],
            bytePtrs, 
            cacheablePtrs)) {
        cacheableSet.insert(*bciit);
      }
    }
  }
}
// This function annotates the cacheable pointers with the
// CacheableRead bit. The cacheable read bit is set
// when the number of write images is not equal to the max
// or if the default RAW_UAV_ID is equal to 11. The first
// condition means that there is a raw uav between 0 and 7
// that is available for cacheable reads and the second
// condition means that UAV 11 is available for cacheable
// reads.
void
annotateCacheablePtrs(
    TargetMachine &TM,
    PtrIMap &PtrToInstMap,
    CacheableSet &cacheablePtrs,
    ByteSet &bytePtrs,
    uint32_t numWriteImages,
    bool mDebug)
{
  const AMDILSubtarget *STM = &TM.getSubtarget<AMDILSubtarget>();
  // AMDILKernelManager *KM = (AMDILKernelManager*)STM->getKernelManager();
  PtrSet::iterator siBegin, siEnd;
  std::vector<MachineInstr*>::iterator miBegin, miEnd;
  AMDILMachineFunctionInfo *mMFI = NULL;
  // First we can check the cacheable pointers
  for (siBegin = cacheablePtrs.begin(), siEnd = cacheablePtrs.end();
      siBegin != siEnd; ++siBegin) {
    assert(!bytePtrs.count(*siBegin) && "Found a cacheable pointer "
        "that also exists as a byte pointer!");
    for (miBegin = PtrToInstMap[*siBegin].begin(),
        miEnd = PtrToInstMap[*siBegin].end();
        miBegin != miEnd; ++miBegin) {
      if (mDebug) {
        dbgs() << "Annotating pointer as cacheable. Inst: ";
        (*miBegin)->dump();
      }
      AMDILAS::InstrResEnc curRes;
      getAsmPrinterFlags(*miBegin, curRes);
      assert(!curRes.bits.ByteStore && "No cacheable pointers should have the "
          "byte Store flag set!");
      // If UAV11 is enabled, then we can enable cached reads.
      if (STM->device()->getResourceID(AMDILDevice::RAW_UAV_ID) == 11) {
        curRes.bits.CacheableRead = 1;
        curRes.bits.ResourceID = 11;
        setAsmPrinterFlags(*miBegin, curRes);
        if (!mMFI) {
          mMFI = (*miBegin)->getParent()->getParent()
            ->getInfo<AMDILMachineFunctionInfo>();
        }
        mMFI->uav_insert(curRes.bits.ResourceID);
      }
    }
  }
}

// A byte pointer is a pointer that along the pointer path has a
// byte store assigned to it.
void
annotateBytePtrs(
    TargetMachine &TM,
    PtrIMap &PtrToInstMap,
    ByteSet &bytePtrs,
    RawSet &rawPtrs,
    bool mDebug
    )
{
  const AMDILSubtarget *STM = &TM.getSubtarget<AMDILSubtarget>();
  AMDILKernelManager *KM = STM->getKernelManager();
  PtrSet::iterator siBegin, siEnd;
  std::vector<MachineInstr*>::iterator miBegin, miEnd;
  uint32_t arenaID = STM->device()
          ->getResourceID(AMDILDevice::ARENA_UAV_ID);
  if (STM->device()->isSupported(AMDILDeviceInfo::ArenaSegment)) {
    arenaID = ARENA_SEGMENT_RESERVED_UAVS + 1;
  }
  AMDILMachineFunctionInfo *mMFI = NULL;
  for (siBegin = bytePtrs.begin(), siEnd = bytePtrs.end();
      siBegin != siEnd; ++siBegin) {
          const Value* val = (*siBegin);
    const PointerType *PT = dyn_cast<PointerType>(val->getType());
    if (!PT) {
        continue;
    }
    const Argument *curArg = dyn_cast<Argument>(val);
    assert(!rawPtrs.count(*siBegin) && "Found a byte pointer "
        "that also exists as a raw pointer!");
    bool arenaInc = false;
    for (miBegin = PtrToInstMap[*siBegin].begin(),
        miEnd = PtrToInstMap[*siBegin].end();
        miBegin != miEnd; ++miBegin) {
      if (mDebug) {
        dbgs() << "Annotating pointer as arena. Inst: ";
        (*miBegin)->dump();
      }
      AMDILAS::InstrResEnc curRes;
      getAsmPrinterFlags(*miBegin, curRes);

      if (STM->device()->usesHardware(AMDILDeviceInfo::ConstantMem)
          && PT->getAddressSpace() == AMDILAS::CONSTANT_ADDRESS) {
        // If hardware constant mem is enabled, then we need to
        // get the constant pointer CB number and use that to specify
        // the resource ID.
        AMDILGlobalManager *GM = STM->getGlobalManager();
        const StringRef funcName = (*miBegin)->getParent()->getParent()
          ->getFunction()->getName();
        if (GM->isKernel(funcName)) {
          const kernel &krnl = GM->getKernel(funcName);
          curRes.bits.ResourceID = GM->getConstPtrCB(krnl,
              (*siBegin)->getName());
          curRes.bits.HardwareInst = 1;
        } else {
          curRes.bits.ResourceID = STM->device()
            ->getResourceID(AMDILDevice::CONSTANT_ID);
        }
      } else if (STM->device()->usesHardware(AMDILDeviceInfo::LocalMem)
          && PT->getAddressSpace() == AMDILAS::LOCAL_ADDRESS) {
        // If hardware local mem is enabled, get the local mem ID from
        // the device to use as the ResourceID
        curRes.bits.ResourceID = STM->device()
          ->getResourceID(AMDILDevice::LDS_ID);
        if (isAtomicInst(TM.getInstrInfo(), *miBegin)) {
          assert(curRes.bits.ResourceID && "Atomic resource ID "
              "cannot be non-zero!");
          (*miBegin)->getOperand((*miBegin)->getNumOperands()-1)
            .setImm(curRes.bits.ResourceID);
        }
      } else if (STM->device()->usesHardware(AMDILDeviceInfo::RegionMem)
          && PT->getAddressSpace() == AMDILAS::REGION_ADDRESS) {
        // If hardware region mem is enabled, get the gds mem ID from
        // the device to use as the ResourceID
        curRes.bits.ResourceID = STM->device()
          ->getResourceID(AMDILDevice::GDS_ID);
        if (isAtomicInst(TM.getInstrInfo(), *miBegin)) {
          assert(curRes.bits.ResourceID && "Atomic resource ID "
              "cannot be non-zero!");
          (*miBegin)->getOperand((*miBegin)->getNumOperands()-1)
            .setImm(curRes.bits.ResourceID);
        }
      } else if (STM->device()->usesHardware(AMDILDeviceInfo::PrivateMem)
          && PT->getAddressSpace() == AMDILAS::PRIVATE_ADDRESS) {
        curRes.bits.ResourceID = STM->device()
          ->getResourceID(AMDILDevice::SCRATCH_ID);
      } else { 
        if (mDebug) {
          dbgs() << __LINE__ << ": Setting byte store bit on instruction: ";
          (*miBegin)->print(dbgs());
        }
        curRes.bits.ByteStore = 1;
        curRes.bits.ResourceID = (curArg && curArg->hasNoAliasAttr()) ? arenaID
          : STM->device()->getResourceID(AMDILDevice::ARENA_UAV_ID);
        if (STM->device()->isSupported(AMDILDeviceInfo::ArenaSegment)) {
          arenaInc = true;
        }
        if (isAtomicInst(TM.getInstrInfo(), *miBegin) &&
            STM->device()->isSupported(AMDILDeviceInfo::ArenaUAV)) {
          (*miBegin)->getOperand((*miBegin)->getNumOperands()-1)
            .setImm(curRes.bits.ResourceID);
          // If we are an arena instruction, we need to switch the atomic opcode
          // from the global version to the arena version.
          MachineInstr *MI = *miBegin;
          MI->setDesc(
              TM.getInstrInfo()->get(
                (MI->getOpcode() - AMDIL::ATOM_G_ADD) + AMDIL::ATOM_A_ADD));
        }
        if (mDebug) {
          dbgs() << "Annotating pointer as arena. Inst: ";
          (*miBegin)->dump();
        }
      }
      setAsmPrinterFlags(*miBegin, curRes);
      KM->setUAVID(*siBegin, curRes.bits.ResourceID);
      if (!mMFI) {
        mMFI = (*miBegin)->getParent()->getParent()
          ->getInfo<AMDILMachineFunctionInfo>();
      }
      mMFI->uav_insert(curRes.bits.ResourceID);
    }
    if (arenaInc) {
      ++arenaID;
    }
  }
}
// An append pointer is a opaque object that has append instructions
// in its path.
void
annotateAppendPtrs(
    TargetMachine &TM,
    PtrIMap &PtrToInstMap,
    AppendSet &appendPtrs,
    bool mDebug)
{
  unsigned currentCounter = 0;
  // const AMDILSubtarget *STM = &TM.getSubtarget<AMDILSubtarget>();
  // AMDILKernelManager *KM = (AMDILKernelManager*)STM->getKernelManager();
  MachineFunction *MF = NULL;
  for (AppendSet::iterator asBegin = appendPtrs.begin(),
      asEnd = appendPtrs.end(); asBegin != asEnd; ++asBegin)
  {
    bool usesWrite = false;
    bool usesRead = false;
    const Value* curVal = *asBegin;
    if (mDebug) {
      dbgs() << "Counter: " << curVal->getName() 
        << " assigned the counter " << currentCounter << "\n";
    }
    for (std::vector<MachineInstr*>::iterator 
        miBegin = PtrToInstMap[curVal].begin(),
        miEnd = PtrToInstMap[curVal].end(); miBegin != miEnd; ++miBegin) {
      MachineInstr *MI = *miBegin;
      if (!MF) {
        MF = MI->getParent()->getParent();
      }
      unsigned opcode = MI->getOpcode();
      switch (opcode) {
        default:
          if (mDebug) {
            dbgs() << "Skipping instruction: ";
            MI->dump();
          }
          break;
        case AMDIL::APPEND_ALLOC:
        case AMDIL::APPEND_ALLOC_NORET:
          usesWrite = true;
          MI->getOperand(1).ChangeToImmediate(currentCounter);
          if (mDebug) {
            dbgs() << "Assing to counter " << currentCounter << " Inst: ";
            MI->dump();
          }
          break;
        case AMDIL::APPEND_CONSUME:
        case AMDIL::APPEND_CONSUME_NORET:
          usesRead = true;
          MI->getOperand(1).ChangeToImmediate(currentCounter);
          if (mDebug) {
            dbgs() << "Assing to counter " << currentCounter << " Inst: ";
            MI->dump();
          }
          break;
      };
    }
    if (usesWrite && usesRead && MF) {
      MF->getInfo<AMDILMachineFunctionInfo>()->addErrorMsg(
          amd::CompilerErrorMessage[INCORRECT_COUNTER_USAGE]);
    }
    ++currentCounter;
  }
}
// A raw pointer is any pointer that does not have byte store in its path.
static void
annotateRawPtrs(
    TargetMachine &TM,
    PtrIMap &PtrToInstMap,
    RawSet &rawPtrs,
    ByteSet &bytePtrs,
    uint32_t numWriteImages,
    bool mDebug
    )
{
  const AMDILSubtarget *STM = &TM.getSubtarget<AMDILSubtarget>();
  AMDILKernelManager *KM = STM->getKernelManager();
  PtrSet::iterator siBegin, siEnd;
  std::vector<MachineInstr*>::iterator miBegin, miEnd;
  AMDILMachineFunctionInfo *mMFI = NULL;

  // Now all of the raw pointers will go to the raw uav.
  for (siBegin = rawPtrs.begin(), siEnd = rawPtrs.end();
      siBegin != siEnd; ++siBegin) {
    const PointerType *PT = dyn_cast<PointerType>((*siBegin)->getType());
    if (!PT) {
      continue;
    }
    assert(!bytePtrs.count(*siBegin) && "Found a raw pointer "
        " that also exists as a byte pointers!");
    for (miBegin = PtrToInstMap[*siBegin].begin(),
        miEnd = PtrToInstMap[*siBegin].end();
        miBegin != miEnd; ++miBegin) {
      if (mDebug) {
        dbgs() << "Annotating pointer as raw. Inst: ";
        (*miBegin)->dump();
      }
      AMDILAS::InstrResEnc curRes;
      getAsmPrinterFlags(*miBegin, curRes);
      if (!curRes.bits.ConflictPtr) {
        assert(!curRes.bits.ByteStore 
            && "Found a instruction that is marked as "
            "raw but has a byte store bit set!");
      } else if (curRes.bits.ConflictPtr) {
        if (curRes.bits.ByteStore) {
          curRes.bits.ByteStore = 0;
        }
      }
      if (STM->device()->usesHardware(AMDILDeviceInfo::ConstantMem)
          && PT->getAddressSpace() == AMDILAS::CONSTANT_ADDRESS) {
        // If hardware constant mem is enabled, then we need to
        // get the constant pointer CB number and use that to specify
        // the resource ID.
        AMDILGlobalManager *GM = STM->getGlobalManager();
        const StringRef funcName = (*miBegin)->getParent()->getParent()
          ->getFunction()->getName();
        if (GM->isKernel(funcName)) {
          const kernel &krnl = GM->getKernel(funcName);
          curRes.bits.ResourceID = GM->getConstPtrCB(krnl,
              (*siBegin)->getName());
          curRes.bits.HardwareInst = 1;
        } else {
          curRes.bits.ResourceID = STM->device()
            ->getResourceID(AMDILDevice::CONSTANT_ID);
        }
      } else if (STM->device()->usesHardware(AMDILDeviceInfo::LocalMem)
          && PT->getAddressSpace() == AMDILAS::LOCAL_ADDRESS) {
        // If hardware local mem is enabled, get the local mem ID from
        // the device to use as the ResourceID
        curRes.bits.ResourceID = STM->device()
          ->getResourceID(AMDILDevice::LDS_ID);
        if (isAtomicInst(TM.getInstrInfo(), *miBegin)) {
          assert(curRes.bits.ResourceID && "Atomic resource ID "
              "cannot be non-zero!");
          (*miBegin)->getOperand((*miBegin)->getNumOperands()-1)
            .setImm(curRes.bits.ResourceID);
        }
      } else if (STM->device()->usesHardware(AMDILDeviceInfo::RegionMem)
          && PT->getAddressSpace() == AMDILAS::REGION_ADDRESS) {
        // If hardware region mem is enabled, get the gds mem ID from
        // the device to use as the ResourceID
        curRes.bits.ResourceID = STM->device()
          ->getResourceID(AMDILDevice::GDS_ID);
        if (isAtomicInst(TM.getInstrInfo(), *miBegin)) {
          assert(curRes.bits.ResourceID && "Atomic resource ID "
              "cannot be non-zero!");
          (*miBegin)->getOperand((*miBegin)->getNumOperands()-1)
            .setImm(curRes.bits.ResourceID);
        }
      } else if (STM->device()->usesHardware(AMDILDeviceInfo::PrivateMem)
          && PT->getAddressSpace() == AMDILAS::PRIVATE_ADDRESS) {
        curRes.bits.ResourceID = STM->device()
          ->getResourceID(AMDILDevice::SCRATCH_ID);
      } else if (!STM->device()->isSupported(AMDILDeviceInfo::MultiUAV)) {
        // If multi uav is enabled, then the resource ID is either the
        // number of write images that are available or the device
        // raw uav id if it is 11.
        if (STM->device()->getResourceID(AMDILDevice::RAW_UAV_ID) >
            STM->device()->getResourceID(AMDILDevice::ARENA_UAV_ID)) {
          curRes.bits.ResourceID = STM->device()
            ->getResourceID(AMDILDevice::RAW_UAV_ID);
        } else if (numWriteImages != OPENCL_MAX_WRITE_IMAGES) {
          if (STM->device()->getResourceID(AMDILDevice::RAW_UAV_ID)
              < numWriteImages) {
            curRes.bits.ResourceID = numWriteImages;
          } else {
            curRes.bits.ResourceID = STM->device()
              ->getResourceID(AMDILDevice::RAW_UAV_ID);
          }
        } else {
          if (mDebug) {
            dbgs() << __LINE__ << ": Setting byte store bit on instruction: ";
            (*miBegin)->print(dbgs());
          }
          curRes.bits.ByteStore = 1;
          curRes.bits.ResourceID = STM->device()
            ->getResourceID(AMDILDevice::ARENA_UAV_ID);
        }
        if (isAtomicInst(TM.getInstrInfo(), *miBegin)) {
          (*miBegin)->getOperand((*miBegin)->getNumOperands()-1)
            .setImm(curRes.bits.ResourceID);
          if (curRes.bits.ResourceID
              == STM->device()->getResourceID(AMDILDevice::ARENA_UAV_ID)) {
            assert(0 && "Found an atomic instruction that has "
                "an arena uav id!");
          }
        }
        KM->setUAVID(*siBegin, curRes.bits.ResourceID);
        if (!mMFI) {
          mMFI = (*miBegin)->getParent()->getParent()
            ->getInfo<AMDILMachineFunctionInfo>();
        }
        mMFI->uav_insert(curRes.bits.ResourceID);
      }
      setAsmPrinterFlags(*miBegin, curRes);
    }
  }

}

void
annotateCacheableInstrs(
    TargetMachine &TM,
    CacheableInstrSet &cacheableSet,
    bool mDebug)
{
  const AMDILSubtarget *STM = &TM.getSubtarget<AMDILSubtarget>();
  // AMDILKernelManager *KM = (AMDILKernelManager*)STM->getKernelManager();

  CacheableInstrSet::iterator miBegin, miEnd;

  for (miBegin = cacheableSet.begin(),
      miEnd = cacheableSet.end();
      miBegin != miEnd; ++miBegin) {
    if (mDebug) {
      dbgs() << "Annotating instr as cacheable. Inst: ";
      (*miBegin)->dump();
    }
    AMDILAS::InstrResEnc curRes;
    getAsmPrinterFlags(*miBegin, curRes);
    // If UAV11 is enabled, then we can enable cached reads.
    if (STM->device()->getResourceID(AMDILDevice::RAW_UAV_ID) == 11) {
      curRes.bits.CacheableRead = 1;
      curRes.bits.ResourceID = 11;
      setAsmPrinterFlags(*miBegin, curRes);
    }
  }
}

// Annotate the instructions along various pointer paths. The paths that
// are handled are the raw, byte and cacheable pointer paths.
static void
annotatePtrPath(
    TargetMachine &TM,
    PtrIMap &PtrToInstMap,
    RawSet &rawPtrs,
    ByteSet &bytePtrs,
    CacheableSet &cacheablePtrs,
    uint32_t numWriteImages,
    bool mDebug
    )
{
  if (PtrToInstMap.empty()) {
    return;
  }
  // First we can check the cacheable pointers
  annotateCacheablePtrs(TM, PtrToInstMap, cacheablePtrs,
      bytePtrs, numWriteImages, mDebug);

  // Next we annotate the byte pointers
  annotateBytePtrs(TM, PtrToInstMap, bytePtrs, rawPtrs, mDebug);

  // Next we annotate the raw pointers
  annotateRawPtrs(TM, PtrToInstMap, rawPtrs, bytePtrs,
      numWriteImages, mDebug);
}
// Allocate MultiUAV pointer ID's for the raw/conflict pointers.
static void
allocateMultiUAVPointers(
    MachineFunction &MF,
    const AMDILTargetMachine *ATM,
    PtrIMap &PtrToInstMap,
    RawSet &rawPtrs,
    ConflictSet &conflictPtrs,
    CacheableSet &cacheablePtrs,
    uint32_t numWriteImages,
    bool mDebug)
{
  if (PtrToInstMap.empty()) {
    return;
  }
  AMDILMachineFunctionInfo *mMFI = MF.getInfo<AMDILMachineFunctionInfo>();
  uint32_t curUAV = numWriteImages;
  bool increment = true;
  const AMDILSubtarget *STM
    = ATM->getSubtargetImpl();
  // If the RAW_UAV_ID is a value that is larger than the max number of write
  // images, then we use that UAV ID.
  if (numWriteImages >= OPENCL_MAX_WRITE_IMAGES) {
    curUAV = STM->device()->getResourceID(AMDILDevice::RAW_UAV_ID);
    increment = false;
  }
  AMDILKernelManager *KM = STM->getKernelManager();
  PtrSet::iterator siBegin, siEnd;
  std::vector<MachineInstr*>::iterator miBegin, miEnd;
  // First lets handle the raw pointers.
  for (siBegin = rawPtrs.begin(), siEnd = rawPtrs.end();
      siBegin != siEnd; ++siBegin) {
    assert((*siBegin)->getType()->isPointerTy() && "We must be a pointer type "
        "to be processed at this point!");
    const PointerType *PT = dyn_cast<PointerType>((*siBegin)->getType());
    if (conflictPtrs.count(*siBegin) || !PT) {
      continue;
    }
    // We only want to process global address space pointers
    if (PT->getAddressSpace() != AMDILAS::GLOBAL_ADDRESS) {
      if ((PT->getAddressSpace() == AMDILAS::LOCAL_ADDRESS
            && STM->device()->usesSoftware(AMDILDeviceInfo::LocalMem))
          || (PT->getAddressSpace() == AMDILAS::CONSTANT_ADDRESS
            && STM->device()->usesSoftware(AMDILDeviceInfo::ConstantMem))
          || (PT->getAddressSpace() == AMDILAS::REGION_ADDRESS
            && STM->device()->usesSoftware(AMDILDeviceInfo::RegionMem))) {
        // If we are using software emulated hardware features, then
        // we need to specify that they use the raw uav and not
        // zero-copy uav. The easiest way to do this is to assume they
        // conflict with another pointer. Any pointer that conflicts
        // with another pointer is assigned to the raw uav or the
        // arena uav if no raw uav exists.
        const PointerType *PT = dyn_cast<PointerType>((*siBegin)->getType());
        if (PT) {
          conflictPtrs.insert(*siBegin);
        }
      }
      if (PT->getAddressSpace() == AMDILAS::PRIVATE_ADDRESS) {
        if (STM->device()->usesSoftware(AMDILDeviceInfo::PrivateMem)) {
          const PointerType *PT = dyn_cast<PointerType>((*siBegin)->getType());
          if (PT) {
            conflictPtrs.insert(*siBegin);
          }
        } else {
          if (mDebug) {
            dbgs() << "Scratch Pointer '" << (*siBegin)->getName()
              << "' being assigned uav "<<
              STM->device()->getResourceID(AMDILDevice::SCRATCH_ID) << "\n";
          }
          for (miBegin = PtrToInstMap[*siBegin].begin(),
              miEnd = PtrToInstMap[*siBegin].end();
              miBegin != miEnd; ++miBegin) {
            AMDILAS::InstrResEnc curRes;
            getAsmPrinterFlags(*miBegin, curRes);
            curRes.bits.ResourceID = STM->device()
              ->getResourceID(AMDILDevice::SCRATCH_ID);
            if (mDebug) {
              dbgs() << "Updated instruction to bitmask ";
              dbgs().write_hex(curRes.u16all);
              dbgs() << " with ResID " << curRes.bits.ResourceID;
              dbgs() << ". Inst: ";
              (*miBegin)->dump();
            }
            setAsmPrinterFlags((*miBegin), curRes);
            KM->setUAVID(*siBegin, curRes.bits.ResourceID);
            mMFI->uav_insert(curRes.bits.ResourceID);
          }
        }
      }
      continue;
    }
    // If more than just UAV 11 is cacheable, then we can remove
    // this check.
    if (cacheablePtrs.count(*siBegin)) {
      if (mDebug) {
        dbgs() << "Raw Pointer '" << (*siBegin)->getName()
          << "' is cacheable, not allocating a multi-uav for it!\n";
      }
      continue;
    }
    if (mDebug) {
      dbgs() << "Raw Pointer '" << (*siBegin)->getName()
        << "' being assigned uav " << curUAV << "\n";
    }
    if (PtrToInstMap[*siBegin].empty()) {
      KM->setUAVID(*siBegin, curUAV);
      mMFI->uav_insert(curUAV);
    }
    // For all instructions here, we are going to set the new UAV to the curUAV
    // number and not the value that it currently is set to.
    for (miBegin = PtrToInstMap[*siBegin].begin(),
        miEnd = PtrToInstMap[*siBegin].end();
        miBegin != miEnd; ++miBegin) {
      AMDILAS::InstrResEnc curRes;
      getAsmPrinterFlags(*miBegin, curRes);
      curRes.bits.ResourceID = curUAV;
      if (isAtomicInst(ATM->getInstrInfo(), *miBegin)) {
        (*miBegin)->getOperand((*miBegin)->getNumOperands()-1)
          .setImm(curRes.bits.ResourceID);
        if (curRes.bits.ResourceID
            == STM->device()->getResourceID(AMDILDevice::ARENA_UAV_ID)) {
          assert(0 && "Found an atomic instruction that has "
              "an arena uav id!");
        }
      }
      if (curUAV == STM->device()->getResourceID(AMDILDevice::ARENA_UAV_ID)) {
        if (mDebug) {
          dbgs() << __LINE__ << ": Setting byte store bit on instruction: ";
          (*miBegin)->print(dbgs());
        }
        curRes.bits.ByteStore = 1;
        curRes.bits.CacheableRead = 0;
      }
      if (mDebug) {
        dbgs() << "Updated instruction to bitmask ";
        dbgs().write_hex(curRes.u16all);
        dbgs() << " with ResID " << curRes.bits.ResourceID;
        dbgs() << ". Inst: ";
        (*miBegin)->dump();
      }
      setAsmPrinterFlags(*miBegin, curRes);
      KM->setUAVID(*siBegin, curRes.bits.ResourceID);
      mMFI->uav_insert(curRes.bits.ResourceID);
    }
    // If we make it here, we can increment the uav counter if we are less
    // than the max write image count. Otherwise we set it to the default
    // UAV and leave it.
    if (increment && curUAV < (OPENCL_MAX_WRITE_IMAGES - 1)) {
      ++curUAV;
    } else {
      curUAV = STM->device()->getResourceID(AMDILDevice::RAW_UAV_ID);
      increment = false;
    }
  }
  if (numWriteImages == 8) {
    curUAV = STM->device()->getResourceID(AMDILDevice::RAW_UAV_ID);
  }
  // Now lets handle the conflict pointers
  for (siBegin = conflictPtrs.begin(), siEnd = conflictPtrs.end();
      siBegin != siEnd; ++siBegin) {
    assert((*siBegin)->getType()->isPointerTy() && "We must be a pointer type "
        "to be processed at this point!");
    const PointerType *PT = dyn_cast<PointerType>((*siBegin)->getType());
    // We only want to process global address space pointers
    if (!PT || PT->getAddressSpace() != AMDILAS::GLOBAL_ADDRESS) {
      continue;
    }
    if (mDebug) {
      dbgs() << "Conflict Pointer '" << (*siBegin)->getName()
        << "' being assigned uav " << curUAV << "\n";
    }
    if (PtrToInstMap[*siBegin].empty()) {
      KM->setUAVID(*siBegin, curUAV);
      mMFI->uav_insert(curUAV);
    }
    for (miBegin = PtrToInstMap[*siBegin].begin(),
        miEnd = PtrToInstMap[*siBegin].end();
        miBegin != miEnd; ++miBegin) {
      AMDILAS::InstrResEnc curRes;
      getAsmPrinterFlags(*miBegin, curRes);
      curRes.bits.ResourceID = curUAV;
      if (isAtomicInst(ATM->getInstrInfo(), *miBegin)) {
        (*miBegin)->getOperand((*miBegin)->getNumOperands()-1)
          .setImm(curRes.bits.ResourceID);
        if (curRes.bits.ResourceID
            == STM->device()->getResourceID(AMDILDevice::ARENA_UAV_ID)) {
          assert(0 && "Found an atomic instruction that has "
              "an arena uav id!");
        }
      }
      if (curUAV == STM->device()->getResourceID(AMDILDevice::ARENA_UAV_ID)) {
        if (mDebug) {
          dbgs() << __LINE__ << ": Setting byte store bit on instruction: ";
          (*miBegin)->print(dbgs());
        }
        curRes.bits.ByteStore = 1;
      }
      if (mDebug) {
        dbgs() << "Updated instruction to bitmask ";
        dbgs().write_hex(curRes.u16all);
        dbgs() << " with ResID " << curRes.bits.ResourceID;
        dbgs() << ". Inst: ";
        (*miBegin)->dump();
      }
      setAsmPrinterFlags(*miBegin, curRes);
      KM->setUAVID(*siBegin, curRes.bits.ResourceID);
      mMFI->uav_insert(curRes.bits.ResourceID);
    }
  }
}
// The first thing we should do is to allocate the default
// ID for each load/store/atomic instruction so that 
// it is correctly allocated. Everything else after this
// is just an optimization to more efficiently allocate
// resource ID's.
void
allocateDefaultIDs(
    const AMDILTargetMachine *ATM,
    MachineFunction &MF,
    bool mDebug)
{
  for (MachineFunction::iterator mfBegin = MF.begin(),
      mfEnd = MF.end(); mfBegin != mfEnd; ++mfBegin) {
    MachineBasicBlock *MB = mfBegin;
    for (MachineBasicBlock::iterator mbb = MB->begin(), mbe = MB->end();
        mbb != mbe; ++mbb) {
      MachineInstr *MI = mbb;
      if (isLoadInst(ATM->getInstrInfo(), MI) 
          || isStoreInst(ATM->getInstrInfo(), MI)
          || isAtomicInst(ATM->getInstrInfo(), MI)) {
        AMDILAS::InstrResEnc curRes;
        getAsmPrinterFlags(MI, curRes);
        allocateDefaultID(ATM, curRes, MI, mDebug);
      } 
    }
  }
}

  bool
AMDILEGPointerManager::runOnMachineFunction(MachineFunction &MF)
{
  bool changed = false;
  const AMDILTargetMachine *ATM
    = reinterpret_cast<const AMDILTargetMachine*>(&TM);
  AMDILMachineFunctionInfo *mMFI = 
    MF.getInfo<AMDILMachineFunctionInfo>();
  if (mDebug) {
    dbgs() << getPassName() << "\n";
    dbgs() << MF.getFunction()->getName() << "\n";
    MF.dump();
  }
  // Start out by allocating the default ID's to all instructions in the
  // function.
  allocateDefaultIDs(ATM, MF, mDebug);

  // A set of all pointers are tracked in this map and
  // if multiple pointers are detected, they go to the same
  // set.
  PtrIMap PtrToInstMap;

  // All of the instructions that are loads, stores or pointer
  // conflicts are tracked in the map with a set of all values
  // that reference the instruction stored.
  InstPMap InstToPtrMap;

  // In order to track across stack entries, we need a map between a 
  // frame index and a pointer. That way when we load from a frame
  // index, we know what pointer was stored to the frame index.
  FIPMap FIToPtrMap;

  // Set of all the pointers that are byte pointers. Byte pointers
  // are required to have their instructions go to the arena.
  ByteSet bytePtrs;

  // Set of all the pointers that are cacheable. All of the cache pointers
  // are required to go to a raw uav and cannot go to arena.
  CacheableSet cacheablePtrs;

  // Set of all the pointers that go into a raw buffer. A pointer can
  // exist in either rawPtrs or bytePtrs but not both.
  RawSet rawPtrs;

  // Set of all the pointers that end up having a conflicting instruction
  // somewhere in the pointer path.
  ConflictSet conflictPtrs;

  // Set of all pointers that are images
  ImageSet images;

  // Set of all pointers that are counters
  AppendSet counters;

  // Set of all pointers that load from a constant pool
  CPoolSet cpool;

  // Mapping from BB to infomation about the cacheability of the 
  // global load instructions in it.
  MBBCacheableMap bbCacheable;

  // A set of load instructions that are cacheable 
  // even if all the load instructions of the ptr are not. 
  CacheableInstrSet cacheableSet;

  // The lookup table holds all of the registers that
  // are used as we assign pointers values to them.
  // If two pointers collide on the lookup table, then
  // we assign them to the same UAV. If one of the
  // pointers is byte addressable, then we assign
  // them to arena, otherwise we assign them to raw.
  RVPVec lookupTable;

  // First we need to go through all of the arguments and assign the
  // live in registers to the lookup table and the pointer mapping.
  uint32_t numWriteImages = parseArguments(MF, lookupTable, ATM, 
      cacheablePtrs, images, counters, mDebug);

  // Lets do some error checking on the results of the parsing.
  if (counters.size() > OPENCL_MAX_NUM_ATOMIC_COUNTERS) {
    mMFI->addErrorMsg(
        amd::CompilerErrorMessage[INSUFFICIENT_COUNTER_RESOURCES]);
  }
  if (numWriteImages > OPENCL_MAX_WRITE_IMAGES
      || (images.size() - numWriteImages > OPENCL_MAX_READ_IMAGES)) {
    mMFI->addErrorMsg(
        amd::CompilerErrorMessage[INSUFFICIENT_IMAGE_RESOURCES]);
  }

  // Now lets parse all of the instructions and update our
  // lookup tables.
  parseFunction(this, ATM, MF, InstToPtrMap, PtrToInstMap,
      FIToPtrMap, lookupTable, bytePtrs, conflictPtrs, cpool, 
      bbCacheable, mDebug);

  // We need to go over our pointer map and find all the conflicting
  // pointers that have byte stores and put them in the bytePtr map.
  // All conflicting pointers that don't have byte stores go into
  // the rawPtr map.
  detectConflictingPointers(ATM, InstToPtrMap, bytePtrs, rawPtrs,
      conflictPtrs, mDebug);

  // The next step is to detect whether the pointer should be added to
  // the fully cacheable set or not. A pointer is marked as cacheable if
  // no store instruction exists.
  detectFullyCacheablePointers(ATM, PtrToInstMap, rawPtrs,
      cacheablePtrs, conflictPtrs, mDebug);

  // Disable partially cacheable for now when multiUAV is on.
  // SC versions before SC139 have a bug that generates incorrect
  // addressing for some cached accesses.
  if (!ATM->getSubtargetImpl()
      ->device()->isSupported(AMDILDeviceInfo::MultiUAV) &&
      ATM->getSubtargetImpl()->calVersion() >= CAL_VERSION_SC_139) {
    // Now we take the set of loads that have no reachable stores and
    // create a list of additional instructions (those that aren't already
    // in a cacheablePtr set) that are safe to mark as cacheable. 
    detectCacheableInstrs(bbCacheable, InstToPtrMap, cacheablePtrs,
        bytePtrs, cacheableSet, mDebug);

    // Annotate the additional instructions computed above as cacheable.
    // Note that this should not touch any instructions annotated in
    // annotatePtrPath.
    annotateCacheableInstrs(TM, cacheableSet, mDebug);
  }

  // Now that we have detected everything we need to detect, lets go through an
  // annotate the instructions along the pointer path for each of the
  // various pointer types.
  annotatePtrPath(TM, PtrToInstMap, rawPtrs, bytePtrs,
      cacheablePtrs, numWriteImages, mDebug);

  // Annotate the atomic counter path if any exists.
  annotateAppendPtrs(TM, PtrToInstMap, counters, mDebug);

  // If we support MultiUAV, then we need to determine how
  // many write images exist so that way we know how many UAV are
  // left to allocate to buffers.
  if (ATM->getSubtargetImpl()
      ->device()->isSupported(AMDILDeviceInfo::MultiUAV)) {
    // We now have (OPENCL_MAX_WRITE_IMAGES - numPtrs) buffers open for
    // multi-uav allocation.
    allocateMultiUAVPointers(MF, ATM, PtrToInstMap, rawPtrs,
        conflictPtrs, cacheablePtrs, numWriteImages, mDebug);
  }

  // The last step is to detect if we have any alias constant pool operations.
  // This is not likely, but does happen on occasion with double precision 
  // operations.
  detectAliasedCPoolOps(TM, cpool, mDebug);
  if (mDebug) {
    dumpPointers(bytePtrs, "Byte Store Ptrs");
    dumpPointers(rawPtrs, "Raw Ptrs");
    dumpPointers(cacheablePtrs, "Cache Load Ptrs");
    dumpPointers(counters, "Atomic Counters");
    dumpPointers(images, "Images");
  }
  return changed;
}

// The default pointer manager just assigns the default ID's to
// each load/store instruction and does nothing else. This is
// the pointer manager for the 7XX series of cards.
  bool
AMDILPointerManager::runOnMachineFunction(MachineFunction &MF)
{
  bool changed = false;
  const AMDILTargetMachine *ATM
    = reinterpret_cast<const AMDILTargetMachine*>(&TM);
  if (mDebug) {
    dbgs() << getPassName() << "\n";
    dbgs() << MF.getFunction()->getName() << "\n";
    MF.dump();
  }
  // On the 7XX we don't have to do any special processing, so we 
  // can just allocate the default ID and be done with it.
  allocateDefaultIDs(ATM, MF, mDebug);
  return changed;
}

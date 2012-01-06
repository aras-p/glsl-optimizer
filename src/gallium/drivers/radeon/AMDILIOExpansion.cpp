//===----------- AMDILIOExpansion.cpp - IO Expansion Pass -----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//
// The AMDIL IO Expansion class expands pseudo IO instructions into a sequence
// of instructions that produces the correct results. These instructions are
// not expanded earlier in the pass because any pass before this can assume to
// be able to generate a load/store instruction. So this pass can only have
// passes that execute after it if no load/store instructions can be generated.
//===----------------------------------------------------------------------===//
#include "AMDILIOExpansion.h"
#include "AMDIL.h"
#include "AMDILDevices.h"
#include "AMDILGlobalManager.h"
#include "AMDILKernelManager.h"
#include "AMDILMachineFunctionInfo.h"
#include "AMDILTargetMachine.h"
#include "AMDILUtilityFunctions.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Support/DebugLoc.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Value.h"

using namespace llvm;

char AMDILIOExpansion::ID = 0;
namespace llvm {
  FunctionPass*
    createAMDILIOExpansion(TargetMachine &TM AMDIL_OPT_LEVEL_DECL)
    {
      return TM.getSubtarget<AMDILSubtarget>()
        .device()->getIOExpansion(TM AMDIL_OPT_LEVEL_VAR);
    }
}

AMDILIOExpansion::AMDILIOExpansion(TargetMachine &tm
     AMDIL_OPT_LEVEL_DECL) :
  MachineFunctionPass(ID), TM(tm)
{
  mSTM = &tm.getSubtarget<AMDILSubtarget>();
  mDebug = DEBUGME;
  mTII = tm.getInstrInfo();
  mKM = NULL;
}

AMDILIOExpansion::~AMDILIOExpansion()
{
}
  bool
AMDILIOExpansion::runOnMachineFunction(MachineFunction &MF)
{
  mKM = const_cast<AMDILKernelManager*>(mSTM->getKernelManager());
  mMFI = MF.getInfo<AMDILMachineFunctionInfo>();
  for (MachineFunction::iterator MFI = MF.begin(), MFE = MF.end();
      MFI != MFE; ++MFI) {
    MachineBasicBlock *MBB = MFI;
    for (MachineBasicBlock::iterator MBI = MBB->begin(), MBE = MBB->end();
        MBI != MBE; ++MBI) {
      MachineInstr *MI = MBI;
      if (isIOInstruction(MI)) {
        mBB = MBB;
        saveInst = false;
        expandIOInstruction(MI);
        if (!saveInst) {
          // erase returns the instruction after
          // and we want the instruction before
          MBI = MBB->erase(MI);
          --MBI;
        }
      }
    }
  }
  return false;
}
const char *AMDILIOExpansion::getPassName() const
{
  return "AMDIL Generic IO Expansion Pass";
}
  bool
AMDILIOExpansion::isIOInstruction(MachineInstr *MI)
{
  if (!MI) {
    return false;
  }
  switch(MI->getOpcode()) {
    default:
      return false;
      ExpandCaseToAllTypes(AMDIL::CPOOLLOAD)
        ExpandCaseToAllTypes(AMDIL::CPOOLSEXTLOAD)
        ExpandCaseToAllTypes(AMDIL::CPOOLZEXTLOAD)
        ExpandCaseToAllTypes(AMDIL::CPOOLAEXTLOAD)
        ExpandCaseToAllTypes(AMDIL::CONSTANTLOAD)
        ExpandCaseToAllTypes(AMDIL::CONSTANTSEXTLOAD)
        ExpandCaseToAllTypes(AMDIL::CONSTANTZEXTLOAD)
        ExpandCaseToAllTypes(AMDIL::CONSTANTAEXTLOAD)
        ExpandCaseToAllTypes(AMDIL::PRIVATELOAD)
        ExpandCaseToAllTypes(AMDIL::PRIVATESEXTLOAD)
        ExpandCaseToAllTypes(AMDIL::PRIVATEZEXTLOAD)
        ExpandCaseToAllTypes(AMDIL::PRIVATEAEXTLOAD)
        ExpandCaseToAllTypes(AMDIL::PRIVATESTORE)
        ExpandCaseToAllTruncTypes(AMDIL::PRIVATETRUNCSTORE)
        ExpandCaseToAllTypes(AMDIL::REGIONSTORE)
        ExpandCaseToAllTruncTypes(AMDIL::REGIONTRUNCSTORE)
        ExpandCaseToAllTypes(AMDIL::REGIONLOAD)
        ExpandCaseToAllTypes(AMDIL::REGIONSEXTLOAD)
        ExpandCaseToAllTypes(AMDIL::REGIONZEXTLOAD)
        ExpandCaseToAllTypes(AMDIL::REGIONAEXTLOAD)
        ExpandCaseToAllTypes(AMDIL::LOCALSTORE)
        ExpandCaseToAllTruncTypes(AMDIL::LOCALTRUNCSTORE)
        ExpandCaseToAllTypes(AMDIL::LOCALLOAD)
        ExpandCaseToAllTypes(AMDIL::LOCALSEXTLOAD)
        ExpandCaseToAllTypes(AMDIL::LOCALZEXTLOAD)
        ExpandCaseToAllTypes(AMDIL::LOCALAEXTLOAD)
        ExpandCaseToAllTypes(AMDIL::GLOBALLOAD)
        ExpandCaseToAllTypes(AMDIL::GLOBALSEXTLOAD)
        ExpandCaseToAllTypes(AMDIL::GLOBALAEXTLOAD)
        ExpandCaseToAllTypes(AMDIL::GLOBALZEXTLOAD)
        ExpandCaseToAllTypes(AMDIL::GLOBALSTORE)
        ExpandCaseToAllTruncTypes(AMDIL::GLOBALTRUNCSTORE)
        return true;
  };
  return false;
}
void
AMDILIOExpansion::expandIOInstruction(MachineInstr *MI)
{
  assert(isIOInstruction(MI) && "Must be an IO instruction to "
      "be passed to this function!");
  switch (MI->getOpcode()) {
    default:
      assert(0 && "Not an IO Instruction!");
      ExpandCaseToAllTypes(AMDIL::GLOBALLOAD);
      ExpandCaseToAllTypes(AMDIL::GLOBALSEXTLOAD);
      ExpandCaseToAllTypes(AMDIL::GLOBALZEXTLOAD);
      ExpandCaseToAllTypes(AMDIL::GLOBALAEXTLOAD);
      expandGlobalLoad(MI);
      break;
      ExpandCaseToAllTypes(AMDIL::REGIONLOAD);
      ExpandCaseToAllTypes(AMDIL::REGIONSEXTLOAD);
      ExpandCaseToAllTypes(AMDIL::REGIONZEXTLOAD);
      ExpandCaseToAllTypes(AMDIL::REGIONAEXTLOAD);
      expandRegionLoad(MI);
      break;
      ExpandCaseToAllTypes(AMDIL::LOCALLOAD);
      ExpandCaseToAllTypes(AMDIL::LOCALSEXTLOAD);
      ExpandCaseToAllTypes(AMDIL::LOCALZEXTLOAD);
      ExpandCaseToAllTypes(AMDIL::LOCALAEXTLOAD);
      expandLocalLoad(MI);
      break;
      ExpandCaseToAllTypes(AMDIL::CONSTANTLOAD);
      ExpandCaseToAllTypes(AMDIL::CONSTANTSEXTLOAD);
      ExpandCaseToAllTypes(AMDIL::CONSTANTZEXTLOAD);
      ExpandCaseToAllTypes(AMDIL::CONSTANTAEXTLOAD);
      expandConstantLoad(MI);
      break;
      ExpandCaseToAllTypes(AMDIL::PRIVATELOAD);
      ExpandCaseToAllTypes(AMDIL::PRIVATESEXTLOAD);
      ExpandCaseToAllTypes(AMDIL::PRIVATEZEXTLOAD);
      ExpandCaseToAllTypes(AMDIL::PRIVATEAEXTLOAD);
      expandPrivateLoad(MI);
      break;
      ExpandCaseToAllTypes(AMDIL::CPOOLLOAD);
      ExpandCaseToAllTypes(AMDIL::CPOOLSEXTLOAD);
      ExpandCaseToAllTypes(AMDIL::CPOOLZEXTLOAD);
      ExpandCaseToAllTypes(AMDIL::CPOOLAEXTLOAD);
      expandConstantPoolLoad(MI);
      break;
      ExpandCaseToAllTruncTypes(AMDIL::GLOBALTRUNCSTORE)
      ExpandCaseToAllTypes(AMDIL::GLOBALSTORE);
      expandGlobalStore(MI);
      break;
      ExpandCaseToAllTruncTypes(AMDIL::PRIVATETRUNCSTORE);
      ExpandCaseToAllTypes(AMDIL::PRIVATESTORE);
      expandPrivateStore(MI);
      break;
      ExpandCaseToAllTruncTypes(AMDIL::REGIONTRUNCSTORE);
      ExpandCaseToAllTypes(AMDIL::REGIONSTORE);
      expandRegionStore(MI);
      break;
      ExpandCaseToAllTruncTypes(AMDIL::LOCALTRUNCSTORE);
      ExpandCaseToAllTypes(AMDIL::LOCALSTORE);
      expandLocalStore(MI);
      break;
  }
}
  bool
AMDILIOExpansion::isAddrCalcInstr(MachineInstr *MI)
{
  switch(MI->getOpcode()) {
    ExpandCaseToAllTypes(AMDIL::PRIVATELOAD)
      ExpandCaseToAllTypes(AMDIL::PRIVATESEXTLOAD)
      ExpandCaseToAllTypes(AMDIL::PRIVATEZEXTLOAD)
      ExpandCaseToAllTypes(AMDIL::PRIVATEAEXTLOAD)
      {
        // This section of code is a workaround for the problem of
        // globally scoped constant address variables. The problems
        // comes that although they are declared in the constant
        // address space, all variables must be allocated in the
        // private address space. So when there is a load from
        // the global address, it automatically goes into the private
        // address space. However, the data section is placed in the
        // constant address space so we need to check to see if our
        // load base address is a global variable or not. Only if it
        // is not a global variable can we do the address calculation
        // into the private memory ring.

        MachineMemOperand& memOp = (**MI->memoperands_begin());
        const Value *V = memOp.getValue();
        if (V) {
          const GlobalValue *GV = dyn_cast<GlobalVariable>(V);
          return mSTM->device()->usesSoftware(AMDILDeviceInfo::PrivateMem)
            && !(GV);
        } else {
          return false;
        }
      }
    ExpandCaseToAllTypes(AMDIL::CPOOLLOAD);
    ExpandCaseToAllTypes(AMDIL::CPOOLSEXTLOAD);
    ExpandCaseToAllTypes(AMDIL::CPOOLZEXTLOAD);
    ExpandCaseToAllTypes(AMDIL::CPOOLAEXTLOAD);
    return MI->getOperand(1).isReg();
    ExpandCaseToAllTruncTypes(AMDIL::PRIVATETRUNCSTORE);
    ExpandCaseToAllTypes(AMDIL::PRIVATESTORE);
    return mSTM->device()->usesSoftware(AMDILDeviceInfo::PrivateMem);
    ExpandCaseToAllTruncTypes(AMDIL::LOCALTRUNCSTORE);
    ExpandCaseToAllTypes(AMDIL::LOCALSTORE);
    ExpandCaseToAllTypes(AMDIL::LOCALLOAD);
    ExpandCaseToAllTypes(AMDIL::LOCALSEXTLOAD);
    ExpandCaseToAllTypes(AMDIL::LOCALZEXTLOAD);
    ExpandCaseToAllTypes(AMDIL::LOCALAEXTLOAD);
    return mSTM->device()->usesSoftware(AMDILDeviceInfo::LocalMem);
  };
  return false;

}
  bool
AMDILIOExpansion::isExtendLoad(MachineInstr *MI)
{
  return isSExtLoadInst(TM.getInstrInfo(), MI) ||
         isZExtLoadInst(TM.getInstrInfo(), MI) ||
         isAExtLoadInst(TM.getInstrInfo(), MI)
    || isSWSExtLoadInst(MI);
}

  bool
AMDILIOExpansion::isHardwareRegion(MachineInstr *MI)
{
  switch(MI->getOpcode()) {
    default:
      return false;
      break;
      ExpandCaseToAllTypes(AMDIL::REGIONLOAD)
        ExpandCaseToAllTypes(AMDIL::REGIONSEXTLOAD)
        ExpandCaseToAllTypes(AMDIL::REGIONZEXTLOAD)
        ExpandCaseToAllTypes(AMDIL::REGIONAEXTLOAD)
        ExpandCaseToAllTypes(AMDIL::REGIONSTORE)
        ExpandCaseToAllTruncTypes(AMDIL::REGIONTRUNCSTORE)
        return mSTM->device()->usesHardware(AMDILDeviceInfo::RegionMem);
  };
  return false;
}
  bool
AMDILIOExpansion::isHardwareLocal(MachineInstr *MI)
{
  switch(MI->getOpcode()) {
    default:
      return false;
      break;
      ExpandCaseToAllTypes(AMDIL::LOCALLOAD)
        ExpandCaseToAllTypes(AMDIL::LOCALSEXTLOAD)
        ExpandCaseToAllTypes(AMDIL::LOCALZEXTLOAD)
        ExpandCaseToAllTypes(AMDIL::LOCALAEXTLOAD)
        ExpandCaseToAllTypes(AMDIL::LOCALSTORE)
        ExpandCaseToAllTruncTypes(AMDIL::LOCALTRUNCSTORE)
        return mSTM->device()->usesHardware(AMDILDeviceInfo::LocalMem);
  };
  return false;
}
  bool
AMDILIOExpansion::isPackedData(MachineInstr *MI)
{
  switch(MI->getOpcode()) {
    default:
      if (isTruncStoreInst(TM.getInstrInfo(), MI)) {
        switch (MI->getDesc().OpInfo[0].RegClass) {
          default:
            break;
          case AMDIL::GPRV2I64RegClassID:
          case AMDIL::GPRV2I32RegClassID:
            switch (getMemorySize(MI)) {
              case 2:
              case 4:
                return true;
              default:
                break;
            }
            break;
          case AMDIL::GPRV4I32RegClassID:
            switch (getMemorySize(MI)) {
              case 4:
              case 8:
                return true;
              default:
                break;
            }
            break;
        }
      } 
      break;
      ExpandCaseToPackedTypes(AMDIL::CPOOLLOAD);
      ExpandCaseToPackedTypes(AMDIL::CPOOLSEXTLOAD);
      ExpandCaseToPackedTypes(AMDIL::CPOOLZEXTLOAD);
      ExpandCaseToPackedTypes(AMDIL::CPOOLAEXTLOAD);
      ExpandCaseToPackedTypes(AMDIL::GLOBALLOAD);
      ExpandCaseToPackedTypes(AMDIL::GLOBALSEXTLOAD);
      ExpandCaseToPackedTypes(AMDIL::GLOBALZEXTLOAD);
      ExpandCaseToPackedTypes(AMDIL::GLOBALAEXTLOAD);
      ExpandCaseToPackedTypes(AMDIL::LOCALLOAD);
      ExpandCaseToPackedTypes(AMDIL::LOCALSEXTLOAD);
      ExpandCaseToPackedTypes(AMDIL::LOCALZEXTLOAD);
      ExpandCaseToPackedTypes(AMDIL::LOCALAEXTLOAD);
      ExpandCaseToPackedTypes(AMDIL::REGIONLOAD);
      ExpandCaseToPackedTypes(AMDIL::REGIONSEXTLOAD);
      ExpandCaseToPackedTypes(AMDIL::REGIONZEXTLOAD);
      ExpandCaseToPackedTypes(AMDIL::REGIONAEXTLOAD);
      ExpandCaseToPackedTypes(AMDIL::PRIVATELOAD);
      ExpandCaseToPackedTypes(AMDIL::PRIVATESEXTLOAD);
      ExpandCaseToPackedTypes(AMDIL::PRIVATEZEXTLOAD);
      ExpandCaseToPackedTypes(AMDIL::PRIVATEAEXTLOAD);
      ExpandCaseToPackedTypes(AMDIL::CONSTANTLOAD);
      ExpandCaseToPackedTypes(AMDIL::CONSTANTSEXTLOAD);
      ExpandCaseToPackedTypes(AMDIL::CONSTANTAEXTLOAD);
      ExpandCaseToPackedTypes(AMDIL::CONSTANTZEXTLOAD);
      ExpandCaseToAllTruncTypes(AMDIL::GLOBALTRUNCSTORE)
      ExpandCaseToAllTruncTypes(AMDIL::PRIVATETRUNCSTORE);
      ExpandCaseToAllTruncTypes(AMDIL::LOCALTRUNCSTORE);
      ExpandCaseToAllTruncTypes(AMDIL::REGIONTRUNCSTORE);
      ExpandCaseToPackedTypes(AMDIL::GLOBALSTORE);
      ExpandCaseToPackedTypes(AMDIL::PRIVATESTORE);
      ExpandCaseToPackedTypes(AMDIL::LOCALSTORE);
      ExpandCaseToPackedTypes(AMDIL::REGIONSTORE);
      return true;
  }
  return false;
}

  bool
AMDILIOExpansion::isStaticCPLoad(MachineInstr *MI)
{
  switch(MI->getOpcode()) {
    ExpandCaseToAllTypes(AMDIL::CPOOLLOAD);
    ExpandCaseToAllTypes(AMDIL::CPOOLSEXTLOAD);
    ExpandCaseToAllTypes(AMDIL::CPOOLZEXTLOAD);
    ExpandCaseToAllTypes(AMDIL::CPOOLAEXTLOAD);
    {
      uint32_t x = 0;
      uint32_t num = MI->getNumOperands();
      for (x = 0; x < num; ++x) {
        if (MI->getOperand(x).isCPI()) {
          return true;
        }
      }
    }
    break;
    default:
    break;
  }
  return false;
}

  bool
AMDILIOExpansion::isNbitType(Type *mType, uint32_t nBits, bool isScalar)
{
  if (!mType) {
    return false;
  }
  if (dyn_cast<PointerType>(mType)) {
    PointerType *PT = dyn_cast<PointerType>(mType);
    return isNbitType(PT->getElementType(), nBits);
  } else if (dyn_cast<StructType>(mType)) {
    return getTypeSize(mType) == nBits;
  } else if (dyn_cast<VectorType>(mType)) {
    VectorType *VT = dyn_cast<VectorType>(mType);
    size_t size = VT->getScalarSizeInBits();
    return (isScalar ? 
        VT->getNumElements() * size == nBits : size == nBits);
  } else if (dyn_cast<ArrayType>(mType)) {
    ArrayType *AT = dyn_cast<ArrayType>(mType);
    size_t size = AT->getScalarSizeInBits();
    return (isScalar ? 
        AT->getNumElements() * size == nBits : size == nBits);
  } else if (mType->isSized()) {
    return mType->getScalarSizeInBits() == nBits;
  } else {
    assert(0 && "Found a type that we don't know how to handle!");
    return false;
  }
}

  bool
AMDILIOExpansion::isHardwareInst(MachineInstr *MI)
{
  AMDILAS::InstrResEnc curRes;
  curRes.u16all = MI->getAsmPrinterFlags();
  return curRes.bits.HardwareInst;
}

REG_PACKED_TYPE
AMDILIOExpansion::getPackedID(MachineInstr *MI)
{
  switch (MI->getOpcode()) {
    default:
      break;
    case AMDIL::GLOBALTRUNCSTORE_v2i64i8:
    case AMDIL::REGIONTRUNCSTORE_v2i64i8:
    case AMDIL::LOCALTRUNCSTORE_v2i64i8:
    case AMDIL::PRIVATETRUNCSTORE_v2i64i8:
    case AMDIL::GLOBALTRUNCSTORE_v2i32i8:
    case AMDIL::REGIONTRUNCSTORE_v2i32i8:
    case AMDIL::LOCALTRUNCSTORE_v2i32i8:
    case AMDIL::PRIVATETRUNCSTORE_v2i32i8:
    case AMDIL::GLOBALTRUNCSTORE_v2i16i8:
    case AMDIL::REGIONTRUNCSTORE_v2i16i8:
    case AMDIL::LOCALTRUNCSTORE_v2i16i8:
    case AMDIL::PRIVATETRUNCSTORE_v2i16i8:
    case AMDIL::GLOBALSTORE_v2i8:
    case AMDIL::LOCALSTORE_v2i8:
    case AMDIL::REGIONSTORE_v2i8:
    case AMDIL::PRIVATESTORE_v2i8:
      return PACK_V2I8;
    case AMDIL::GLOBALTRUNCSTORE_v4i32i8:
    case AMDIL::REGIONTRUNCSTORE_v4i32i8:
    case AMDIL::LOCALTRUNCSTORE_v4i32i8:
    case AMDIL::PRIVATETRUNCSTORE_v4i32i8:
    case AMDIL::GLOBALTRUNCSTORE_v4i16i8:
    case AMDIL::REGIONTRUNCSTORE_v4i16i8:
    case AMDIL::LOCALTRUNCSTORE_v4i16i8:
    case AMDIL::PRIVATETRUNCSTORE_v4i16i8:
    case AMDIL::GLOBALSTORE_v4i8:
    case AMDIL::LOCALSTORE_v4i8:
    case AMDIL::REGIONSTORE_v4i8:
    case AMDIL::PRIVATESTORE_v4i8:
      return PACK_V4I8;
    case AMDIL::GLOBALTRUNCSTORE_v2i64i16:
    case AMDIL::REGIONTRUNCSTORE_v2i64i16:
    case AMDIL::LOCALTRUNCSTORE_v2i64i16:
    case AMDIL::PRIVATETRUNCSTORE_v2i64i16:
    case AMDIL::GLOBALTRUNCSTORE_v2i32i16:
    case AMDIL::REGIONTRUNCSTORE_v2i32i16:
    case AMDIL::LOCALTRUNCSTORE_v2i32i16:
    case AMDIL::PRIVATETRUNCSTORE_v2i32i16:
    case AMDIL::GLOBALSTORE_v2i16:
    case AMDIL::LOCALSTORE_v2i16:
    case AMDIL::REGIONSTORE_v2i16:
    case AMDIL::PRIVATESTORE_v2i16:
      return PACK_V2I16;
    case AMDIL::GLOBALTRUNCSTORE_v4i32i16:
    case AMDIL::REGIONTRUNCSTORE_v4i32i16:
    case AMDIL::LOCALTRUNCSTORE_v4i32i16:
    case AMDIL::PRIVATETRUNCSTORE_v4i32i16:
    case AMDIL::GLOBALSTORE_v4i16:
    case AMDIL::LOCALSTORE_v4i16:
    case AMDIL::REGIONSTORE_v4i16:
    case AMDIL::PRIVATESTORE_v4i16:
      return PACK_V4I16;
    case AMDIL::GLOBALLOAD_v2i8:
    case AMDIL::GLOBALSEXTLOAD_v2i8:
    case AMDIL::GLOBALAEXTLOAD_v2i8:
    case AMDIL::GLOBALZEXTLOAD_v2i8:
    case AMDIL::LOCALLOAD_v2i8:
    case AMDIL::LOCALSEXTLOAD_v2i8:
    case AMDIL::LOCALAEXTLOAD_v2i8:
    case AMDIL::LOCALZEXTLOAD_v2i8:
    case AMDIL::REGIONLOAD_v2i8:
    case AMDIL::REGIONSEXTLOAD_v2i8:
    case AMDIL::REGIONAEXTLOAD_v2i8:
    case AMDIL::REGIONZEXTLOAD_v2i8:
    case AMDIL::PRIVATELOAD_v2i8:
    case AMDIL::PRIVATESEXTLOAD_v2i8:
    case AMDIL::PRIVATEAEXTLOAD_v2i8:
    case AMDIL::PRIVATEZEXTLOAD_v2i8:
    case AMDIL::CONSTANTLOAD_v2i8:
    case AMDIL::CONSTANTSEXTLOAD_v2i8:
    case AMDIL::CONSTANTAEXTLOAD_v2i8:
    case AMDIL::CONSTANTZEXTLOAD_v2i8:
      return UNPACK_V2I8;
    case AMDIL::GLOBALLOAD_v4i8:
    case AMDIL::GLOBALSEXTLOAD_v4i8:
    case AMDIL::GLOBALAEXTLOAD_v4i8:
    case AMDIL::GLOBALZEXTLOAD_v4i8:
    case AMDIL::LOCALLOAD_v4i8:
    case AMDIL::LOCALSEXTLOAD_v4i8:
    case AMDIL::LOCALAEXTLOAD_v4i8:
    case AMDIL::LOCALZEXTLOAD_v4i8:
    case AMDIL::REGIONLOAD_v4i8:
    case AMDIL::REGIONSEXTLOAD_v4i8:
    case AMDIL::REGIONAEXTLOAD_v4i8:
    case AMDIL::REGIONZEXTLOAD_v4i8:
    case AMDIL::PRIVATELOAD_v4i8:
    case AMDIL::PRIVATESEXTLOAD_v4i8:
    case AMDIL::PRIVATEAEXTLOAD_v4i8:
    case AMDIL::PRIVATEZEXTLOAD_v4i8:
    case AMDIL::CONSTANTLOAD_v4i8:
    case AMDIL::CONSTANTSEXTLOAD_v4i8:
    case AMDIL::CONSTANTAEXTLOAD_v4i8:
    case AMDIL::CONSTANTZEXTLOAD_v4i8:
      return UNPACK_V4I8;
    case AMDIL::GLOBALLOAD_v2i16:
    case AMDIL::GLOBALSEXTLOAD_v2i16:
    case AMDIL::GLOBALAEXTLOAD_v2i16:
    case AMDIL::GLOBALZEXTLOAD_v2i16:
    case AMDIL::LOCALLOAD_v2i16:
    case AMDIL::LOCALSEXTLOAD_v2i16:
    case AMDIL::LOCALAEXTLOAD_v2i16:
    case AMDIL::LOCALZEXTLOAD_v2i16:
    case AMDIL::REGIONLOAD_v2i16:
    case AMDIL::REGIONSEXTLOAD_v2i16:
    case AMDIL::REGIONAEXTLOAD_v2i16:
    case AMDIL::REGIONZEXTLOAD_v2i16:
    case AMDIL::PRIVATELOAD_v2i16:
    case AMDIL::PRIVATESEXTLOAD_v2i16:
    case AMDIL::PRIVATEAEXTLOAD_v2i16:
    case AMDIL::PRIVATEZEXTLOAD_v2i16:
    case AMDIL::CONSTANTLOAD_v2i16:
    case AMDIL::CONSTANTSEXTLOAD_v2i16:
    case AMDIL::CONSTANTAEXTLOAD_v2i16:
    case AMDIL::CONSTANTZEXTLOAD_v2i16:
      return UNPACK_V2I16;
    case AMDIL::GLOBALLOAD_v4i16:
    case AMDIL::GLOBALSEXTLOAD_v4i16:
    case AMDIL::GLOBALAEXTLOAD_v4i16:
    case AMDIL::GLOBALZEXTLOAD_v4i16:
    case AMDIL::LOCALLOAD_v4i16:
    case AMDIL::LOCALSEXTLOAD_v4i16:
    case AMDIL::LOCALAEXTLOAD_v4i16:
    case AMDIL::LOCALZEXTLOAD_v4i16:
    case AMDIL::REGIONLOAD_v4i16:
    case AMDIL::REGIONSEXTLOAD_v4i16:
    case AMDIL::REGIONAEXTLOAD_v4i16:
    case AMDIL::REGIONZEXTLOAD_v4i16:
    case AMDIL::PRIVATELOAD_v4i16:
    case AMDIL::PRIVATESEXTLOAD_v4i16:
    case AMDIL::PRIVATEAEXTLOAD_v4i16:
    case AMDIL::PRIVATEZEXTLOAD_v4i16:
    case AMDIL::CONSTANTLOAD_v4i16:
    case AMDIL::CONSTANTSEXTLOAD_v4i16:
    case AMDIL::CONSTANTAEXTLOAD_v4i16:
    case AMDIL::CONSTANTZEXTLOAD_v4i16:
      return UNPACK_V4I16;
  };
  return NO_PACKING;
}

  uint32_t
AMDILIOExpansion::getPointerID(MachineInstr *MI)
{
  AMDILAS::InstrResEnc curInst;
  getAsmPrinterFlags(MI, curInst);
  return curInst.bits.ResourceID;
}

  uint32_t
AMDILIOExpansion::getShiftSize(MachineInstr *MI)
{
  switch(getPackedID(MI)) {
    default:
      return 0;
    case PACK_V2I8:
    case PACK_V4I8:
    case UNPACK_V2I8:
    case UNPACK_V4I8:
      return 1;
    case PACK_V2I16:
    case PACK_V4I16:
    case UNPACK_V2I16:
    case UNPACK_V4I16:
      return 2;
  }
  return 0;
}
  uint32_t
AMDILIOExpansion::getMemorySize(MachineInstr *MI)
{
  if (MI->memoperands_empty()) {
    return 4;
  }
  return (uint32_t)((*MI->memoperands_begin())->getSize());
}

  void
AMDILIOExpansion::expandLongExtend(MachineInstr *MI,
    uint32_t numComps, uint32_t size, bool signedShift)
{
  DebugLoc DL = MI->getDebugLoc();
  switch(size) {
    default:
      assert(0 && "Found a case we don't handle!");
      break;
    case 8:
      if (numComps == 1) {
        expandLongExtendSub32(MI, AMDIL::SHL_i8, AMDIL::SHRVEC_v2i32, 
            AMDIL::USHRVEC_i8,
            24, (24ULL | (31ULL << 32)), 24, AMDIL::LCREATE, signedShift);
      } else if (numComps == 2) {
        expandLongExtendSub32(MI, AMDIL::SHL_v2i8, AMDIL::SHRVEC_v4i32, 
            AMDIL::USHRVEC_v2i8,
            24, (24ULL | (31ULL << 32)), 24, AMDIL::LCREATE_v2i64, signedShift);
      } else {
        assert(0 && "Found a case we don't handle!");
      }
      break;
    case 16:
      if (numComps == 1) {
        expandLongExtendSub32(MI, AMDIL::SHL_i16, AMDIL::SHRVEC_v2i32, 
            AMDIL::USHRVEC_i16,
            16, (16ULL | (31ULL << 32)), 16, AMDIL::LCREATE, signedShift);
      } else if (numComps == 2) {
        expandLongExtendSub32(MI, AMDIL::SHL_v2i16, AMDIL::SHRVEC_v4i32, 
            AMDIL::USHRVEC_v2i16,
            16, (16ULL | (31ULL << 32)), 16, AMDIL::LCREATE_v2i64, signedShift);
      } else {
        assert(0 && "Found a case we don't handle!");
      }
      break;
    case 32:
      if (numComps == 1) {
        if (signedShift) {
          BuildMI(*mBB, MI, DL, mTII->get(AMDIL::SHRVEC_i32), AMDIL::R1012)
            .addReg(AMDIL::R1011)
            .addImm(mMFI->addi32Literal(31));
          BuildMI(*mBB, MI, DL, mTII->get(AMDIL::LCREATE), AMDIL::R1011)
            .addReg(AMDIL::R1011).addReg(AMDIL::R1012);
        } else {
          BuildMI(*mBB, MI, DL, mTII->get(AMDIL::LCREATE), AMDIL::R1011)
            .addReg(AMDIL::R1011)
            .addImm(mMFI->addi32Literal(0));
        }
      } else if (numComps == 2) {
        if (signedShift) {
          BuildMI(*mBB, MI, DL, mTII->get(AMDIL::SHRVEC_v2i32), AMDIL::R1012)
            .addReg(AMDIL::R1011)
            .addImm(mMFI->addi32Literal(31));
          BuildMI(*mBB, MI, DL, mTII->get(AMDIL::LCREATE_v2i64), AMDIL::R1011)
            .addReg(AMDIL::R1011)
            .addReg(AMDIL::R1012);
        } else {
          BuildMI(*mBB, MI, DL, mTII->get(AMDIL::LCREATE_v2i64), AMDIL::R1011)
            .addReg(AMDIL::R1011)
            .addImm(mMFI->addi32Literal(0));
        }
      } else {
        assert(0 && "Found a case we don't handle!");
      }
  };
}
  void 
AMDILIOExpansion::expandLongExtendSub32(MachineInstr *MI, 
    unsigned SHLop, unsigned SHRop, unsigned USHRop, 
    unsigned SHLimm, uint64_t SHRimm, unsigned USHRimm, 
    unsigned LCRop, bool signedShift)
{
  DebugLoc DL = MI->getDebugLoc();
  BuildMI(*mBB, MI, DL, mTII->get(SHLop), AMDIL::R1011)
    .addReg(AMDIL::R1011)
    .addImm(mMFI->addi32Literal(SHLimm));
  if (signedShift) {
    BuildMI(*mBB, MI, DL, mTII->get(LCRop), AMDIL::R1011)
      .addReg(AMDIL::R1011).addReg(AMDIL::R1011);
    BuildMI(*mBB, MI, DL, mTII->get(SHRop), AMDIL::R1011)
      .addReg(AMDIL::R1011)
      .addImm(mMFI->addi64Literal(SHRimm));
  } else {
    BuildMI(*mBB, MI, DL, mTII->get(USHRop), AMDIL::R1011)
      .addReg(AMDIL::R1011)
      .addImm(mMFI->addi32Literal(USHRimm));
    BuildMI(*mBB, MI, MI->getDebugLoc(), mTII->get(LCRop), AMDIL::R1011)
      .addReg(AMDIL::R1011)
      .addImm(mMFI->addi32Literal(0));
  }
}

  void
AMDILIOExpansion::expandIntegerExtend(MachineInstr *MI, unsigned SHLop, 
    unsigned SHRop, unsigned offset)
{
  DebugLoc DL = MI->getDebugLoc();
  offset = mMFI->addi32Literal(offset);
  BuildMI(*mBB, MI, DL,
      mTII->get(SHLop), AMDIL::R1011)
    .addReg(AMDIL::R1011).addImm(offset);
  BuildMI(*mBB, MI, DL,
      mTII->get(SHRop), AMDIL::R1011)
    .addReg(AMDIL::R1011).addImm(offset);
}
  void
AMDILIOExpansion::expandExtendLoad(MachineInstr *MI)
{
  if (!isExtendLoad(MI)) {
    return;
  }
  Type *mType = NULL;
  if (!MI->memoperands_empty()) {
    MachineMemOperand *memOp = (*MI->memoperands_begin());
    const Value *moVal = (memOp) ? memOp->getValue() : NULL;
    mType = (moVal) ? moVal->getType() : NULL;
  }
  unsigned opcode = 0;
  DebugLoc DL = MI->getDebugLoc();
  if (isZExtLoadInst(TM.getInstrInfo(), MI) || isAExtLoadInst(TM.getInstrInfo(), MI) || isSExtLoadInst(TM.getInstrInfo(), MI)) {
    switch(MI->getDesc().OpInfo[0].RegClass) {
      default:
        assert(0 && "Found an extending load that we don't handle!");
        break;
      case AMDIL::GPRI16RegClassID:
        if (!isHardwareLocal(MI)
            || mSTM->device()->usesSoftware(AMDILDeviceInfo::ByteLDSOps)) {
          opcode = isSExtLoadInst(TM.getInstrInfo(), MI) ? AMDIL::SHRVEC_i16 : AMDIL::USHRVEC_i16;
          expandIntegerExtend(MI, AMDIL::SHL_i16, opcode, 24);
        }
        break;
      case AMDIL::GPRV2I16RegClassID:
        opcode = isSExtLoadInst(TM.getInstrInfo(), MI) ? AMDIL::SHRVEC_v2i16 : AMDIL::USHRVEC_v2i16;
        expandIntegerExtend(MI, AMDIL::SHL_v2i16, opcode, 24);
        break;
      case AMDIL::GPRV4I8RegClassID:        
        opcode = isSExtLoadInst(TM.getInstrInfo(), MI) ? AMDIL::SHRVEC_v4i8 : AMDIL::USHRVEC_v4i8;
        expandIntegerExtend(MI, AMDIL::SHL_v4i8, opcode, 24);
        break;
      case AMDIL::GPRV4I16RegClassID:
        opcode = isSExtLoadInst(TM.getInstrInfo(), MI) ? AMDIL::SHRVEC_v4i16 : AMDIL::USHRVEC_v4i16;
        expandIntegerExtend(MI, AMDIL::SHL_v4i16, opcode, 24);
        break;
      case AMDIL::GPRI32RegClassID:
        // We can be a i8 or i16 bit sign extended value
        if (isNbitType(mType, 8) || getMemorySize(MI) == 1) {
          opcode = isSExtLoadInst(TM.getInstrInfo(), MI) ? AMDIL::SHRVEC_i32 : AMDIL::USHRVEC_i32;
          expandIntegerExtend(MI, AMDIL::SHL_i32, opcode, 24);
        } else if (isNbitType(mType, 16) || getMemorySize(MI) == 2) {
          opcode = isSExtLoadInst(TM.getInstrInfo(), MI) ? AMDIL::SHRVEC_i32 : AMDIL::USHRVEC_i32;
          expandIntegerExtend(MI, AMDIL::SHL_i32, opcode, 16);
        } else {
          assert(0 && "Found an extending load that we don't handle!");
        }
        break;
      case AMDIL::GPRV2I32RegClassID:
        // We can be a v2i8 or v2i16 bit sign extended value
        if (isNbitType(mType, 8, false) || getMemorySize(MI) == 2) {
          opcode = isSExtLoadInst(TM.getInstrInfo(), MI) ? AMDIL::SHRVEC_v2i32 : AMDIL::USHRVEC_v2i32;
          expandIntegerExtend(MI, AMDIL::SHL_v2i32, opcode, 24);
        } else if (isNbitType(mType, 16, false) || getMemorySize(MI) == 4) {
          opcode = isSExtLoadInst(TM.getInstrInfo(), MI) ? AMDIL::SHRVEC_v2i32 : AMDIL::USHRVEC_v2i32;
          expandIntegerExtend(MI, AMDIL::SHL_v2i32, opcode, 16);
        } else {
          assert(0 && "Found an extending load that we don't handle!");
        }
        break;
      case AMDIL::GPRV4I32RegClassID:
        // We can be a v4i8 or v4i16 bit sign extended value
        if (isNbitType(mType, 8, false) || getMemorySize(MI) == 4) {
          opcode = isSExtLoadInst(TM.getInstrInfo(), MI) ? AMDIL::SHRVEC_v4i32 : AMDIL::USHRVEC_v4i32;
          expandIntegerExtend(MI, AMDIL::SHL_v4i32, opcode, 24);
        } else if (isNbitType(mType, 16, false) || getMemorySize(MI) == 8) {
          opcode = isSExtLoadInst(TM.getInstrInfo(), MI) ? AMDIL::SHRVEC_v4i32 : AMDIL::USHRVEC_v4i32;
          expandIntegerExtend(MI, AMDIL::SHL_v4i32, opcode, 16);
        } else {
          assert(0 && "Found an extending load that we don't handle!");
        }
        break;
      case AMDIL::GPRI64RegClassID:
        // We can be a i8, i16 or i32 bit sign extended value
        if (isNbitType(mType, 8) || getMemorySize(MI) == 1) {
          expandLongExtend(MI, 1, 8, isSExtLoadInst(TM.getInstrInfo(), MI));
        } else if (isNbitType(mType, 16) || getMemorySize(MI) == 2) {
          expandLongExtend(MI, 1, 16, isSExtLoadInst(TM.getInstrInfo(), MI));
        } else if (isNbitType(mType, 32) || getMemorySize(MI) == 4) {
          expandLongExtend(MI, 1, 32, isSExtLoadInst(TM.getInstrInfo(), MI));
        } else {
          assert(0 && "Found an extending load that we don't handle!");
        }
        break;
      case AMDIL::GPRV2I64RegClassID:
        // We can be a v2i8, v2i16 or v2i32 bit sign extended value
        if (isNbitType(mType, 8, false) || getMemorySize(MI) == 2) {
          expandLongExtend(MI, 2, 8, isSExtLoadInst(TM.getInstrInfo(), MI));
        } else if (isNbitType(mType, 16, false) || getMemorySize(MI) == 4) {
          expandLongExtend(MI, 2, 16, isSExtLoadInst(TM.getInstrInfo(), MI));
        } else if (isNbitType(mType, 32, false) || getMemorySize(MI) == 8) {
          expandLongExtend(MI, 2, 32, isSExtLoadInst(TM.getInstrInfo(), MI));
        } else {
          assert(0 && "Found an extending load that we don't handle!");
        }
        break;
      case AMDIL::GPRF32RegClassID:
        BuildMI(*mBB, MI, DL, 
            mTII->get(AMDIL::HTOF_f32), AMDIL::R1011)
          .addReg(AMDIL::R1011);
        break;
      case AMDIL::GPRV2F32RegClassID:
        BuildMI(*mBB, MI, DL, 
            mTII->get(AMDIL::HTOF_v2f32), AMDIL::R1011)
          .addReg(AMDIL::R1011);
        break;
      case AMDIL::GPRV4F32RegClassID:
        BuildMI(*mBB, MI, DL, 
            mTII->get(AMDIL::HTOF_v4f32), AMDIL::R1011)
          .addReg(AMDIL::R1011);
        break;
      case AMDIL::GPRF64RegClassID:
        BuildMI(*mBB, MI, DL, 
            mTII->get(AMDIL::FTOD), AMDIL::R1011)
          .addReg(AMDIL::R1011);
        break;
      case AMDIL::GPRV2F64RegClassID:
        BuildMI(*mBB, MI, DL, mTII->get(AMDIL::VEXTRACT_v2f32),
            AMDIL::R1012).addReg(AMDIL::R1011).addImm(2);
        BuildMI(*mBB, MI, DL, 
            mTII->get(AMDIL::FTOD), AMDIL::R1011)
          .addReg(AMDIL::R1011);
        BuildMI(*mBB, MI, DL, 
            mTII->get(AMDIL::FTOD), AMDIL::R1012)
          .addReg(AMDIL::R1012);
        BuildMI(*mBB, MI, DL,
            mTII->get(AMDIL::VINSERT_v2f64), AMDIL::R1011)
          .addReg(AMDIL::R1011).addReg(AMDIL::R1012)
          .addImm(1 << 8).addImm(1 << 8);
        break;
    };
  } else if (isSWSExtLoadInst(MI)) {
    switch(MI->getDesc().OpInfo[0].RegClass) {
      case AMDIL::GPRI8RegClassID:
        if (!isHardwareLocal(MI)
            || mSTM->device()->usesSoftware(AMDILDeviceInfo::ByteLDSOps)) {
          expandIntegerExtend(MI, AMDIL::SHL_i8, AMDIL::SHRVEC_i8, 24);
        }
        break;
      case AMDIL::GPRV2I8RegClassID:
        expandIntegerExtend(MI, AMDIL::SHL_v2i8, AMDIL::SHRVEC_v2i8, 24);
        break;
      case AMDIL::GPRV4I8RegClassID:
        expandIntegerExtend(MI, AMDIL::SHL_v4i8, AMDIL::SHRVEC_v4i8, 24);
        break;
      case AMDIL::GPRI16RegClassID:
        if (!isHardwareLocal(MI)
            || mSTM->device()->usesSoftware(AMDILDeviceInfo::ByteLDSOps)) {
          expandIntegerExtend(MI, AMDIL::SHL_i16, AMDIL::SHRVEC_i16, 16);
        }
        break;
      case AMDIL::GPRV2I16RegClassID:
        expandIntegerExtend(MI, AMDIL::SHL_v2i16, AMDIL::SHRVEC_v2i16, 16);
        break;
      case AMDIL::GPRV4I16RegClassID:
        expandIntegerExtend(MI, AMDIL::SHL_v4i16, AMDIL::SHRVEC_v4i16, 16);
        break;

    };
  }
}

  void
AMDILIOExpansion::expandTruncData(MachineInstr *MI)
{
  MachineBasicBlock::iterator I = *MI;
  if (!isTruncStoreInst(TM.getInstrInfo(), MI)) {
    return;
  }
  DebugLoc DL = MI->getDebugLoc();
  switch (MI->getOpcode()) {
    default: 
      MI->dump();
      assert(!"Found a trunc store instructions we don't handle!");
      break;
    case AMDIL::GLOBALTRUNCSTORE_i64i8:
    case AMDIL::GLOBALTRUNCSTORE_v2i64i8:
    case AMDIL::LOCALTRUNCSTORE_i64i8:
    case AMDIL::LOCALTRUNCSTORE_v2i64i8:
    case AMDIL::REGIONTRUNCSTORE_i64i8:
    case AMDIL::REGIONTRUNCSTORE_v2i64i8:
    case AMDIL::PRIVATETRUNCSTORE_i64i8:
    case AMDIL::PRIVATETRUNCSTORE_v2i64i8:
      BuildMI(*mBB, MI, DL,
          mTII->get(AMDIL::LLO_v2i64), AMDIL::R1011)
          .addReg(AMDIL::R1011);
    case AMDIL::GLOBALTRUNCSTORE_i16i8:
    case AMDIL::GLOBALTRUNCSTORE_v2i16i8:
    case AMDIL::GLOBALTRUNCSTORE_v4i16i8:
    case AMDIL::LOCALTRUNCSTORE_i16i8:
    case AMDIL::LOCALTRUNCSTORE_v2i16i8:
    case AMDIL::LOCALTRUNCSTORE_v4i16i8:
    case AMDIL::REGIONTRUNCSTORE_i16i8:
    case AMDIL::REGIONTRUNCSTORE_v2i16i8:
    case AMDIL::REGIONTRUNCSTORE_v4i16i8:
    case AMDIL::PRIVATETRUNCSTORE_i16i8:
    case AMDIL::PRIVATETRUNCSTORE_v2i16i8:
    case AMDIL::PRIVATETRUNCSTORE_v4i16i8:
    case AMDIL::GLOBALTRUNCSTORE_i32i8:
    case AMDIL::GLOBALTRUNCSTORE_v2i32i8:
    case AMDIL::GLOBALTRUNCSTORE_v4i32i8:
    case AMDIL::LOCALTRUNCSTORE_i32i8:
    case AMDIL::LOCALTRUNCSTORE_v2i32i8:
    case AMDIL::LOCALTRUNCSTORE_v4i32i8:
    case AMDIL::REGIONTRUNCSTORE_i32i8:
    case AMDIL::REGIONTRUNCSTORE_v2i32i8:
    case AMDIL::REGIONTRUNCSTORE_v4i32i8:
    case AMDIL::PRIVATETRUNCSTORE_i32i8:
    case AMDIL::PRIVATETRUNCSTORE_v2i32i8:
    case AMDIL::PRIVATETRUNCSTORE_v4i32i8:
      BuildMI(*mBB, MI, DL, 
          mTII->get(AMDIL::BINARY_AND_v4i32), AMDIL::R1011)
        .addReg(AMDIL::R1011)
        .addImm(mMFI->addi32Literal(0xFF));
      break;
    case AMDIL::GLOBALTRUNCSTORE_i64i16:
    case AMDIL::GLOBALTRUNCSTORE_v2i64i16:
    case AMDIL::LOCALTRUNCSTORE_i64i16:
    case AMDIL::LOCALTRUNCSTORE_v2i64i16:
    case AMDIL::REGIONTRUNCSTORE_i64i16:
    case AMDIL::REGIONTRUNCSTORE_v2i64i16:
    case AMDIL::PRIVATETRUNCSTORE_i64i16:
    case AMDIL::PRIVATETRUNCSTORE_v2i64i16:
      BuildMI(*mBB, MI, DL,
          mTII->get(AMDIL::LLO_v2i64), AMDIL::R1011)
          .addReg(AMDIL::R1011);
    case AMDIL::GLOBALTRUNCSTORE_i32i16:
    case AMDIL::GLOBALTRUNCSTORE_v2i32i16:
    case AMDIL::GLOBALTRUNCSTORE_v4i32i16:
    case AMDIL::LOCALTRUNCSTORE_i32i16:
    case AMDIL::LOCALTRUNCSTORE_v2i32i16:
    case AMDIL::LOCALTRUNCSTORE_v4i32i16:
    case AMDIL::REGIONTRUNCSTORE_i32i16:
    case AMDIL::REGIONTRUNCSTORE_v2i32i16:
    case AMDIL::REGIONTRUNCSTORE_v4i32i16:
    case AMDIL::PRIVATETRUNCSTORE_i32i16:
    case AMDIL::PRIVATETRUNCSTORE_v2i32i16:
    case AMDIL::PRIVATETRUNCSTORE_v4i32i16:
      BuildMI(*mBB, MI, DL, 
          mTII->get(AMDIL::BINARY_AND_v4i32), AMDIL::R1011)
        .addReg(AMDIL::R1011)
        .addImm(mMFI->addi32Literal(0xFFFF));
      break;
    case AMDIL::GLOBALTRUNCSTORE_i64i32:
    case AMDIL::LOCALTRUNCSTORE_i64i32:
    case AMDIL::REGIONTRUNCSTORE_i64i32:
    case AMDIL::PRIVATETRUNCSTORE_i64i32:
      BuildMI(*mBB, MI, DL,
          mTII->get(AMDIL::LLO), AMDIL::R1011)
          .addReg(AMDIL::R1011);
      break;
    case AMDIL::GLOBALTRUNCSTORE_v2i64i32:
    case AMDIL::LOCALTRUNCSTORE_v2i64i32:
    case AMDIL::REGIONTRUNCSTORE_v2i64i32:
    case AMDIL::PRIVATETRUNCSTORE_v2i64i32:
      BuildMI(*mBB, MI, DL,
          mTII->get(AMDIL::LLO_v2i64), AMDIL::R1011)
          .addReg(AMDIL::R1011);
      break;
    case AMDIL::GLOBALTRUNCSTORE_f64f32:
    case AMDIL::LOCALTRUNCSTORE_f64f32:
    case AMDIL::REGIONTRUNCSTORE_f64f32:
    case AMDIL::PRIVATETRUNCSTORE_f64f32:
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::DTOF),
          AMDIL::R1011).addReg(AMDIL::R1011);
      break;
    case AMDIL::GLOBALTRUNCSTORE_v2f64f32:
    case AMDIL::LOCALTRUNCSTORE_v2f64f32:
    case AMDIL::REGIONTRUNCSTORE_v2f64f32:
    case AMDIL::PRIVATETRUNCSTORE_v2f64f32:
      BuildMI(*mBB, I, DL, mTII->get(AMDIL::VEXTRACT_v2f64),
          AMDIL::R1012).addReg(AMDIL::R1011).addImm(2);
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::DTOF),
          AMDIL::R1011).addReg(AMDIL::R1011);
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::DTOF),
          AMDIL::R1012).addReg(AMDIL::R1012);
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::VINSERT_v2f32),
          AMDIL::R1011).addReg(AMDIL::R1011).addReg(AMDIL::R1012)
        .addImm(1 << 8).addImm(1 << 8);
      break;
  }
}
  void
AMDILIOExpansion::expandAddressCalc(MachineInstr *MI)
{
  if (!isAddrCalcInstr(MI)) {
    return;
  }
  DebugLoc DL = MI->getDebugLoc();
  switch(MI->getOpcode()) {
    ExpandCaseToAllTruncTypes(AMDIL::PRIVATETRUNCSTORE)
      ExpandCaseToAllTypes(AMDIL::PRIVATESTORE)
      ExpandCaseToAllTypes(AMDIL::PRIVATELOAD)
      ExpandCaseToAllTypes(AMDIL::PRIVATESEXTLOAD)
      ExpandCaseToAllTypes(AMDIL::PRIVATEZEXTLOAD)
      ExpandCaseToAllTypes(AMDIL::PRIVATEAEXTLOAD)
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::ADD_i32), 
          AMDIL::R1010).addReg(AMDIL::R1010).addReg(AMDIL::T1);
    break;
    ExpandCaseToAllTruncTypes(AMDIL::LOCALTRUNCSTORE)
      ExpandCaseToAllTypes(AMDIL::LOCALLOAD)
      ExpandCaseToAllTypes(AMDIL::LOCALSEXTLOAD)
      ExpandCaseToAllTypes(AMDIL::LOCALZEXTLOAD)
      ExpandCaseToAllTypes(AMDIL::LOCALAEXTLOAD)
      ExpandCaseToAllTypes(AMDIL::LOCALSTORE)
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::ADD_i32), 
          AMDIL::R1010).addReg(AMDIL::R1010).addReg(AMDIL::T2);
    break;
    ExpandCaseToAllTypes(AMDIL::CPOOLLOAD)
      ExpandCaseToAllTypes(AMDIL::CPOOLSEXTLOAD)
      ExpandCaseToAllTypes(AMDIL::CPOOLZEXTLOAD)
      ExpandCaseToAllTypes(AMDIL::CPOOLAEXTLOAD)
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::ADD_i32), 
          AMDIL::R1010).addReg(AMDIL::R1010).addReg(AMDIL::SDP);
    break;
    default:
    return;
  }
}
  void
AMDILIOExpansion::expandLoadStartCode(MachineInstr *MI)
{
  DebugLoc DL = MI->getDebugLoc();
  if (MI->getOperand(2).isReg()) {
    BuildMI(*mBB, MI, DL, mTII->get(AMDIL::ADD_i32),
        AMDIL::R1010).addReg(MI->getOperand(1).getReg())
      .addReg(MI->getOperand(2).getReg());
  } else {
    BuildMI(*mBB, MI, DL, mTII->get(AMDIL::MOVE_i32),
        AMDIL::R1010).addReg(MI->getOperand(1).getReg());
  }
  MI->getOperand(1).setReg(AMDIL::R1010);
  expandAddressCalc(MI);
}
  void
AMDILIOExpansion::emitStaticCPLoad(MachineInstr* MI, int swizzle, 
    int id, bool ExtFPLoad)
{
  DebugLoc DL = MI->getDebugLoc();
  switch(swizzle) {
    default:
      BuildMI(*mBB, MI, DL, mTII->get(ExtFPLoad 
            ? AMDIL::DTOF : AMDIL::MOVE_i32), 
          MI->getOperand(0).getReg())
        .addImm(id);
      break;
    case 1:
    case 2:
    case 3:
      BuildMI(*mBB, MI, DL, mTII->get(ExtFPLoad 
            ? AMDIL::DTOF : AMDIL::MOVE_i32), AMDIL::R1001)
        .addImm(id);
      BuildMI(*mBB, MI, DL, mTII->get(AMDIL::VINSERT_v4i32),
          MI->getOperand(0).getReg())
        .addReg(MI->getOperand(0).getReg())
        .addReg(AMDIL::R1001)
        .addImm(swizzle + 1);
      break;
  };
}
  void
AMDILIOExpansion::emitCPInst(MachineInstr* MI,
    const Constant* C, AMDILKernelManager* KM, int swizzle, bool ExtFPLoad)
{
  if (const ConstantFP* CFP = dyn_cast<ConstantFP>(C)) {
    if (CFP->getType()->isFloatTy()) {
      uint32_t val = (uint32_t)(CFP->getValueAPF().bitcastToAPInt()
          .getZExtValue());
      uint32_t id = mMFI->addi32Literal(val);
      if (!id) {
        const APFloat &APF = CFP->getValueAPF();
        union dtol_union {
          double d;
          uint64_t ul;
        } conv;
        if (&APF.getSemantics()
            == (const llvm::fltSemantics*)&APFloat::IEEEsingle) {
          float fval = APF.convertToFloat();
          conv.d = (double)fval;
        } else {
          conv.d = APF.convertToDouble();
        }
        id = mMFI->addi64Literal(conv.ul);
      }
      emitStaticCPLoad(MI, swizzle, id, ExtFPLoad);
    } else {
      const APFloat &APF = CFP->getValueAPF();
      union ftol_union {
        double d;
        uint64_t ul;
      } conv;
      if (&APF.getSemantics()
          == (const llvm::fltSemantics*)&APFloat::IEEEsingle) {
        float fval = APF.convertToFloat();
        conv.d = (double)fval;
      } else {
        conv.d = APF.convertToDouble();
      }
      uint32_t id = mMFI->getLongLits(conv.ul);
      if (!id) {
        id = mMFI->getIntLits((uint32_t)conv.ul);
      }
      emitStaticCPLoad(MI, swizzle, id, ExtFPLoad);
    }
  } else if (const ConstantInt* CI = dyn_cast<ConstantInt>(C)) {
    int64_t val = 0;
    if (CI) {
      val = CI->getSExtValue();
    }
    if (CI->getBitWidth() == 64) {
      emitStaticCPLoad(MI, swizzle, mMFI->addi64Literal(val), ExtFPLoad);
    } else {
      emitStaticCPLoad(MI, swizzle, mMFI->addi32Literal(val), ExtFPLoad);
    }
  } else if (const ConstantArray* CA = dyn_cast<ConstantArray>(C)) {
    uint32_t size = CA->getNumOperands();
    assert(size < 5 && "Cannot handle a constant array where size > 4");
    if (size > 4) {
      size = 4;
    }
    for (uint32_t x = 0; x < size; ++x) {
      emitCPInst(MI, CA->getOperand(0), KM, x, ExtFPLoad);
    }
  } else if (const ConstantAggregateZero* CAZ
      = dyn_cast<ConstantAggregateZero>(C)) {
    if (CAZ->isNullValue()) {
      emitStaticCPLoad(MI, swizzle, mMFI->addi32Literal(0), ExtFPLoad);
    }
  } else if (const ConstantStruct* CS = dyn_cast<ConstantStruct>(C)) {
    uint32_t size = CS->getNumOperands();
    assert(size < 5 && "Cannot handle a constant array where size > 4");
    if (size > 4) {
      size = 4;
    }
    for (uint32_t x = 0; x < size; ++x) {
      emitCPInst(MI, CS->getOperand(0), KM, x, ExtFPLoad);
    }
  } else if (const ConstantVector* CV = dyn_cast<ConstantVector>(C)) {
    // TODO: Make this handle vectors natively up to the correct
    // size
    uint32_t size = CV->getNumOperands();
    assert(size < 5 && "Cannot handle a constant array where size > 4");
    if (size > 4) {
      size = 4;
    }
    for (uint32_t x = 0; x < size; ++x) {
      emitCPInst(MI, CV->getOperand(0), KM, x, ExtFPLoad);
    }
  } else {
    // TODO: Do we really need to handle ConstantPointerNull?
    // What about BlockAddress, ConstantExpr and Undef?
    // How would these even be generated by a valid CL program?
    assert(0 && "Found a constant type that I don't know how to handle");
  }
}


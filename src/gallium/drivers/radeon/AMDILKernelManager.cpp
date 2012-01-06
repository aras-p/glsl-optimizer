//===-- AMDILKernelManager.cpp - TODO: Add brief description -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//
#include "AMDILKernelManager.h"

#include "AMDILAlgorithms.tpp"
#include "AMDILKernelManager.h"
#ifdef UPSTREAM_LLVM
#include "AMDILAsmPrinter.h"
#endif
#include "AMDILCompilerErrors.h"
#include "AMDILDeviceInfo.h"
#include "AMDILDevices.h"
#include "AMDILGlobalManager.h"
#include "AMDILMachineFunctionInfo.h"
#include "AMDILModuleInfo.h"
#include "AMDILSubtarget.h"
#include "AMDILTargetMachine.h"
#include "AMDILUtilityFunctions.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/MathExtras.h"

#include <stdio.h>

using namespace llvm;
#define NUM_EXTRA_SLOTS_PER_IMAGE 1

static bool errorPrint(const char *ptr, llvm::raw_ostream &O) {
  if (ptr[0] == 'E') {
    O << ";error:" << ptr << "\n";
  } else {
    O << ";warning:" << ptr << "\n";
  }
  return false;
}

#if 0
static bool
samplerPrint(StringMap<SamplerInfo>::iterator &data, llvm::raw_ostream &O) {
  O << ";sampler:" << (*data).second.name << ":" << (*data).second.idx
    << ":" << ((*data).second.val == (uint32_t)-1 ? 0 : 1) 
    << ":" << ((*data).second.val != (uint32_t)-1 ? (*data).second.val : 0)
    << "\n";
  return false;
}
#endif

static bool arenaPrint(uint32_t val, llvm::raw_ostream &O) {
  if (val >= ARENA_SEGMENT_RESERVED_UAVS) {
    O << "dcl_arena_uav_id(" << val << ")\n";
  }
  return false;
}

static bool uavPrint(uint32_t val, llvm::raw_ostream &O) {
  if (val < 8 || val == 11){
    O << "dcl_raw_uav_id(" << val << ")\n";
  }
  return false;
}

static bool uavPrintSI(uint32_t val, llvm::raw_ostream &O) {
  O << "dcl_typeless_uav_id(" << val << ")_stride(4)_length(4)_access(read_write)\n";
  return false;
}

static bool
printfPrint(std::pair<const std::string, PrintfInfo *> &data, llvm::raw_ostream &O) {
  O << ";printf_fmt:" << data.second->getPrintfID();
  // Number of operands
  O << ":" << data.second->getNumOperands();
  // Size of each operand
  for (size_t i = 0, e = data.second->getNumOperands(); i < e; ++i) {
    O << ":" << (data.second->getOperandID(i) >> 3);
  }
  const char *ptr = data.first.c_str();
  uint32_t size = data.first.size() - 1;
  // The format string size
  O << ":" << size << ":";
  for (size_t i = 0; i < size; ++i) {
    if (ptr[i] == '\r') {
      O << "\\r";
    } else if (ptr[i] == '\n') {
      O << "\\n";
    } else {
      O << ptr[i];
    }
  }
  O << ";\n";   // c_str() is cheap way to trim
  return false;
}


void AMDILKernelManager::updatePtrArg(Function::const_arg_iterator Ip,
                                      int numWriteImages, int raw_uav_buffer,
                                      int counter, bool isKernel,
                                      const Function *F) {
  assert(F && "Cannot pass a NULL Pointer to F!");
  assert(Ip->getType()->isPointerTy() &&
         "Argument must be a pointer to be passed into this function!\n");
  std::string ptrArg(";pointer:");
  const char *symTab = "NoSymTab";
  uint32_t ptrID = getUAVID(Ip);
  const PointerType *PT = cast<PointerType>(Ip->getType());
  uint32_t Align = 4;
  const char *MemType = "uav";
  if (PT->getElementType()->isSized()) {
    Align = NextPowerOf2((uint32_t)mTM->getTargetData()->
                            getTypeAllocSize(PT->getElementType()));
  }
  ptrArg += Ip->getName().str() + ":" + getTypeName(PT, symTab) + ":1:1:" +
            itostr(counter * 16) + ":";
  switch (PT->getAddressSpace()) {
  case AMDILAS::ADDRESS_NONE:
    //O << "No Address space qualifier!";
    mMFI->addErrorMsg(amd::CompilerErrorMessage[INTERNAL_ERROR]);
    assert(1);
    break;
  case AMDILAS::GLOBAL_ADDRESS:
    if (mSTM->device()->isSupported(AMDILDeviceInfo::ArenaSegment)) {
      if (ptrID >= ARENA_SEGMENT_RESERVED_UAVS) {
        ptrID = 8;
      }
    }
    mMFI->uav_insert(ptrID);
    break;
  case AMDILAS::CONSTANT_ADDRESS: {
    if (isKernel && mSTM->device()->usesHardware(AMDILDeviceInfo::ConstantMem)){
      const kernel t = mGM->getKernel(F->getName());
      if (mGM->usesHWConstant(t, Ip->getName())) {
        MemType = "hc\0";
        ptrID = mGM->getConstPtrCB(t, Ip->getName());
      } else {
        MemType = "c\0";
        mMFI->uav_insert(ptrID);
      }
    } else {
      MemType = "c\0";
      mMFI->uav_insert(ptrID);
    }
    break; 
  }
  default:
  case AMDILAS::PRIVATE_ADDRESS:
    if (mSTM->device()->usesHardware(AMDILDeviceInfo::PrivateMem)) {
      MemType = (mSTM->device()->isSupported(AMDILDeviceInfo::PrivateUAV)) 
        ? "up\0" : "hp\0";
    } else {
      MemType = "p\0";
      mMFI->uav_insert(ptrID);
    }
    break;
  case AMDILAS::REGION_ADDRESS:
    mMFI->setUsesRegion();
    if (mSTM->device()->usesHardware(AMDILDeviceInfo::RegionMem)) {
      MemType = "hr\0";
      ptrID = 0;
    } else {
      MemType = "r\0";
      mMFI->uav_insert(ptrID);
    }
    break;
  case AMDILAS::LOCAL_ADDRESS:
    mMFI->setUsesLocal();
    if (mSTM->device()->usesHardware(AMDILDeviceInfo::LocalMem)) {
      MemType = "hl\0";
      ptrID = 1;
    } else {
      MemType = "l\0";
      mMFI->uav_insert(ptrID);
    }
    break;
  };
  ptrArg += std::string(MemType) + ":";
  ptrArg += itostr(ptrID) + ":";
  ptrArg += itostr(Align);
  mMFI->addMetadata(ptrArg, true);
}

AMDILKernelManager::AMDILKernelManager(AMDILTargetMachine *TM,
                                       AMDILGlobalManager *GM)
{
  mTM = TM;
  mSTM = mTM->getSubtargetImpl();
  mGM = GM;
  clear();
}

AMDILKernelManager::~AMDILKernelManager() {
  clear();
}

void 
AMDILKernelManager::setMF(MachineFunction *MF)
{
  mMF = MF;
  mMFI = MF->getInfo<AMDILMachineFunctionInfo>();
}

void AMDILKernelManager::clear() {
  mUniqueID = 0;
  mIsKernel = false;
  mWasKernel = false;
  mHasImageWrite = false;
  mHasOutputInst = false;
}

bool AMDILKernelManager::useCompilerWrite(const MachineInstr *MI) {
  return (MI->getOpcode() == AMDIL::RETURN && wasKernel() && !mHasImageWrite
          && !mHasOutputInst);
}

void AMDILKernelManager::processArgMetadata(llvm::raw_ostream &O,
                                            uint32_t buf,
                                            bool isKernel) 
{
  const Function *F = mMF->getFunction();
  const char * symTab = "NoSymTab";
  Function::const_arg_iterator Ip = F->arg_begin();
  Function::const_arg_iterator Ep = F->arg_end();
  
  if (F->hasStructRetAttr()) {
    assert(Ip != Ep && "Invalid struct return fucntion!");
    mMFI->addErrorMsg(amd::CompilerErrorMessage[INTERNAL_ERROR]);
    ++Ip;
  }
  uint32_t mCBSize = 0;
  int raw_uav_buffer = mSTM->device()->getResourceID(AMDILDevice::RAW_UAV_ID);
  bool MultiUAV = mSTM->device()->isSupported(AMDILDeviceInfo::MultiUAV);
  bool ArenaSegment =
    mSTM->device()->isSupported(AMDILDeviceInfo::ArenaSegment);
  int numWriteImages =
    mSTM->getGlobalManager()->getNumWriteImages(F->getName());
  if (numWriteImages == OPENCL_MAX_WRITE_IMAGES || MultiUAV || ArenaSegment) {
    if (mSTM->device()->getGeneration() <= AMDILDeviceInfo::HD6XXX) {
      raw_uav_buffer = mSTM->device()->getResourceID(AMDILDevice::ARENA_UAV_ID);
    }
  }
  uint32_t CounterNum = 0;
  uint32_t ROArg = 0;
  uint32_t WOArg = 0;
  uint32_t NumArg = 0;
  while (Ip != Ep) {
    Type *cType = Ip->getType();
    if (cType->isIntOrIntVectorTy() || cType->isFPOrFPVectorTy()) {
      std::string argMeta(";value:");
      argMeta += Ip->getName().str() + ":" + getTypeName(cType, symTab) + ":";
      int bitsize = cType->getPrimitiveSizeInBits();
      int numEle = 1;
      if (cType->getTypeID() == Type::VectorTyID) {
        numEle = cast<VectorType>(cType)->getNumElements();
      }
      argMeta += itostr(numEle) + ":1:" + itostr(mCBSize << 4);
      mMFI->addMetadata(argMeta, true);

      // FIXME: simplify
      if ((bitsize / numEle) < 32) {
        bitsize = numEle >> 2;
      } else {
        bitsize >>= 7;
      }
      if (!bitsize) {
        bitsize = 1;
      }

      mCBSize += bitsize;
      ++NumArg;
    } else if (const PointerType *PT = dyn_cast<PointerType>(cType)) {
      Type *CT = PT->getElementType();
      const StructType *ST = dyn_cast<StructType>(CT);
      if (ST && ST->isOpaque()) {
        StringRef name = ST->getName();
        bool i1d  = name.equals( "struct._image1d_t" );
        bool i1da = name.equals( "struct._image1d_array_t" );
        bool i1db = name.equals( "struct._image1d_buffer_t" );
        bool i2d  = name.equals( "struct._image2d_t" );
        bool i2da = name.equals( "struct._image2d_array_t" );
        bool i3d  = name.equals( "struct._image3d_t" );
        bool c32  = name.equals( "struct._counter32_t" );
        bool c64  = name.equals( "struct._counter64_t" );
        if (i1d || i1da || i1db || i2d | i2da || i3d) {
          if (mSTM->device()->isSupported(AMDILDeviceInfo::Images)) {
            std::string imageArg(";image:");
            imageArg += Ip->getName().str() + ":";
            if (i1d)       imageArg += "1D:";
            else if (i1da) imageArg += "1DA:";
            else if (i1db) imageArg += "1DB:";
            else if (i2d)  imageArg += "2D:";
            else if (i2da) imageArg += "2DA:";
            else if (i3d)  imageArg += "3D:";

            if (isKernel) {
              if (mGM->isReadOnlyImage (mMF->getFunction()->getName(),
                                        (ROArg + WOArg))) {
                imageArg += "RO:" + itostr(ROArg);
                O << "dcl_resource_id(" << ROArg << ")_type(";
                if (i1d)       O << "1d";
                else if (i1da) O << "1darray";
                else if (i1db) O << "buffer";
                else if (i2d)  O << "2d";
                else if (i2da) O << "2darray";
                else if (i3d)  O << "3d";
                O << ")_fmtx(unknown)_fmty(unknown)"
                  << "_fmtz(unknown)_fmtw(unknown)\n";
                ++ROArg;
              } else if (mGM->isWriteOnlyImage(mMF->getFunction()->getName(),
                                               (ROArg + WOArg))) {
                uint32_t offset = 0;
                offset += WOArg;
                imageArg += "WO:" + itostr(offset & 0x7);
                O << "dcl_uav_id(" << ((offset) & 0x7) << ")_type(";
                if (i1d)       O << "1d";
                else if (i1da) O << "1darray";
                else if (i1db) O << "buffer";
                else if (i2d)  O << "2d";
                else if (i2da) O << "2darray";
                else if (i3d)  O << "3d";
                O << ")_fmtx(uint)\n";
                ++WOArg;
              } else {
                imageArg += "RW:" + itostr(ROArg + WOArg);
              }
            }
            imageArg += ":1:" + itostr(mCBSize * 16);
            mMFI->addMetadata(imageArg, true);
            mMFI->addi32Literal(mCBSize);
            mCBSize += NUM_EXTRA_SLOTS_PER_IMAGE + 1;
            ++NumArg;
          } else {
            mMFI->addErrorMsg(amd::CompilerErrorMessage[NO_IMAGE_SUPPORT]);
            ++NumArg;
          }
        } else if (c32 || c64) {
          std::string counterArg(";counter:");
          counterArg += Ip->getName().str() + ":"
            + itostr(c32 ? 32 : 64) + ":"
            + itostr(CounterNum++) + ":1:" + itostr(mCBSize * 16);
          mMFI->addMetadata(counterArg, true);
          ++NumArg;
          ++mCBSize;
        } else {
          updatePtrArg(Ip, numWriteImages, raw_uav_buffer, mCBSize, isKernel,
                       F);
          ++NumArg;
          ++mCBSize;
        }
      }
        else if (CT->getTypeID() == Type::StructTyID
                 && PT->getAddressSpace() == AMDILAS::PRIVATE_ADDRESS) {
        const TargetData *td = mTM->getTargetData();
        const StructLayout *sl = td->getStructLayout(dyn_cast<StructType>(CT));
        int bytesize = sl->getSizeInBytes();
        int reservedsize = (bytesize + 15) & ~15;
        int numSlots = reservedsize >> 4;
        if (!numSlots) {
          numSlots = 1;
        }
        std::string structArg(";value:");
        structArg += Ip->getName().str() + ":struct:"
          + itostr(bytesize) + ":1:" + itostr(mCBSize * 16);
        mMFI->addMetadata(structArg, true);
        mCBSize += numSlots;
        ++NumArg;
      } else if (CT->isIntOrIntVectorTy()
                 || CT->isFPOrFPVectorTy()
                 || CT->getTypeID() == Type::ArrayTyID
                 || CT->getTypeID() == Type::PointerTyID
                 || PT->getAddressSpace() != AMDILAS::PRIVATE_ADDRESS) {
        updatePtrArg(Ip, numWriteImages, raw_uav_buffer, mCBSize, isKernel, F);
        ++NumArg;
        ++mCBSize;
      } else {
        assert(0 && "Cannot process current pointer argument");
        mMFI->addErrorMsg(amd::CompilerErrorMessage[INTERNAL_ERROR]);
        ++NumArg;
      }
    } else {
      assert(0 && "Cannot process current kernel argument");
      mMFI->addErrorMsg(amd::CompilerErrorMessage[INTERNAL_ERROR]);
      ++NumArg;
    }
    ++Ip;
  }
}

void AMDILKernelManager::printHeader(AMDILAsmPrinter *AsmPrinter,
                                     llvm::raw_ostream &O,
                                     const std::string &name) {
#ifdef UPSTREAM_LLVM
  mName = name;
  std::string kernelName;
  kernelName = name;
  int kernelId = mGM->getOrCreateFunctionID(kernelName);
  O << "func " << kernelId << " ; " << kernelName << "\n";
  if (mSTM->is64bit()) {
    O << "mov " << AsmPrinter->getRegisterName(AMDIL::SDP) << ", cb0[8].xy\n";
  } else {
    O << "mov " << AsmPrinter->getRegisterName(AMDIL::SDP) << ", cb0[8].x\n";
  }
  O << "mov " << AsmPrinter->getRegisterName(AMDIL::SP) << ", l1.0\n";
#endif
}

void AMDILKernelManager::printGroupSize(llvm::raw_ostream& O) {
  // The HD4XXX generation of hardware does not support a 3D launch, so we need
  // to use dcl_num_thread_per_group to specify the launch size. If the launch
  // size is specified via a kernel attribute, we print it here. Otherwise we
  // use the the default size.
  if (mSTM->device()->getGeneration() == AMDILDeviceInfo::HD4XXX) {
    if (mGM->hasRWG(mName) 
        || !mMFI->usesLocal()) {
      // if the user has specified what the required workgroup size is then we
      // need to compile for that size and that size only.  Otherwise we compile
      // for the max workgroup size that is passed in as an option to the
      // backend.
      O << "dcl_num_thread_per_group ";
      O << mGM->getLocal(mName, 0) << ", ";
      O << mGM->getLocal(mName, 1) << ", ";
      O << mGM->getLocal(mName, 2) << "        \n";
    } else {
      // If the kernel uses local memory, then the kernel is being
      // compiled in single wavefront mode. So we have to generate code slightly
      // different.
      O << "dcl_num_thread_per_group "
        << mSTM->device()->getWavefrontSize()
        << ", 1, 1       \n";
    }
  } else {
    // Otherwise we generate for devices that support 3D launch natively.  If
    // the reqd_workgroup_size attribute was specified, then we can specify the
    // exact launch dimensions.
    if (mGM->hasRWG(mName)) {
      O << "dcl_num_thread_per_group ";
      O << mGM->getLocal(mName, 0) << ", ";
      O << mGM->getLocal(mName, 1) << ", ";
      O << mGM->getLocal(mName, 2) << "        \n";
    } else {
      // Otherwise we specify the largest workgroup size that can be launched.
      O << "dcl_max_thread_per_group " << mGM->getLocal(mName, 3) << " \n";
    }
  }
  // Now that we have specified the workgroup size, lets declare the local
  // memory size. If we are using hardware and we know the value at compile
  // time, then we need to declare the correct value. Otherwise we should just
  // declare the maximum size.
  if (mSTM->device()->usesHardware(AMDILDeviceInfo::LocalMem)) {
    size_t kernelLocalSize = (mGM->getHWLocalSize(mName) + 3) & ~3;
    if (kernelLocalSize > mSTM->device()->getMaxLDSSize()) {
      mMFI->addErrorMsg(amd::CompilerErrorMessage[INSUFFICIENT_LOCAL_RESOURCES]);
    }
    // If there is a local pointer as a kernel argument, we don't know the size
    // at compile time, so we reserve all of the space.
    if (mMFI->usesLocal() && (mMFI->hasLocalArg() || !kernelLocalSize)) {
      O << "dcl_lds_id(" << DEFAULT_LDS_ID << ") "
        << mSTM->device()->getMaxLDSSize() << "\n";
      mMFI->setUsesMem(AMDILDevice::LDS_ID);
    } else if (kernelLocalSize) {
      // We know the size, so lets declare it correctly.
      O << "dcl_lds_id(" << DEFAULT_LDS_ID << ") "
        << kernelLocalSize << "\n";
      mMFI->setUsesMem(AMDILDevice::LDS_ID);
    }
  }
  // If the device supports the region memory extension, which maps to our
  // hardware GDS memory, then lets declare it so we can use it later on.
  if (mSTM->device()->usesHardware(AMDILDeviceInfo::RegionMem)) {
    size_t kernelGDSSize = (mGM->getHWRegionSize(mName) + 3) & ~3;
    if (kernelGDSSize > mSTM->device()->getMaxGDSSize()) {
      mMFI->addErrorMsg(amd::CompilerErrorMessage[INSUFFICIENT_REGION_RESOURCES]);
    }
    // If there is a region pointer as a kernel argument, we don't know the size
    // at compile time, so we reserved all of the space.
    if (mMFI->usesRegion() && (mMFI->hasRegionArg() || !kernelGDSSize)) {
      O << "dcl_gds_id(" << DEFAULT_GDS_ID <<
        ") " << mSTM->device()->getMaxGDSSize() << "\n";
      mMFI->setUsesMem(AMDILDevice::GDS_ID);
    } else if (kernelGDSSize) {
      // We know the size, so lets declare it.
      O << "dcl_gds_id(" << DEFAULT_GDS_ID <<
        ") " << kernelGDSSize << "\n";
      mMFI->setUsesMem(AMDILDevice::GDS_ID);
    }
  }
}

void
AMDILKernelManager::printDecls(AMDILAsmPrinter *AsmPrinter, llvm::raw_ostream &O) {
  // If we are a HD4XXX generation device, then we only support a single uav
  // surface, so we declare it and leave
  if (mSTM->device()->getGeneration() == AMDILDeviceInfo::HD4XXX) {
    O << "dcl_raw_uav_id(" 
      << mSTM->device()->getResourceID(AMDILDevice::RAW_UAV_ID)
      << ")\n";
    mMFI->setUsesMem(AMDILDevice::RAW_UAV_ID);
    getIntrinsicSetup(AsmPrinter, O);
    return;
  }
  // If we are supporting multiple uav's view the MultiUAV capability, then we
  // need to print out the declarations here. MultiUAV conflicts with write
  // images, so they only use 8 - NumWriteImages uav's. Therefor only pointers
  // with ID's < 8 will get printed.
  if (mSTM->device()->isSupported(AMDILDeviceInfo::MultiUAV)) {
    binaryForEach(mMFI->uav_begin(), mMFI->uav_end(), uavPrint, O);
    mMFI->setUsesMem(AMDILDevice::RAW_UAV_ID);
  }
  // If arena segments are supported, then we should emit them now.  Arena
  // segments are similiar to MultiUAV, except ArenaSegments are virtual and up
  // to 1024 of them can coexist. These are more compiler hints for CAL and thus
  // cannot overlap in any form.  Each ID maps to a seperate piece of memory and
  // CAL determines whether the load/stores should go to the fast path/slow path
  // based on the usage and instruction.
  if (mSTM->device()->isSupported(AMDILDeviceInfo::ArenaSegment)) {
    binaryForEach(mMFI->uav_begin(), mMFI->uav_end(), arenaPrint, O);
  }
  // Now that we have printed out all of the arena and multi uav declaration,
  // now we must print out the default raw uav id. This always exists on HD5XXX
  // and HD6XXX hardware. The reason is that the hardware supports 12 UAV's and
  // 11 are taken up by MultiUAV/Write Images and Arena.  However, if we do not
  // have UAV 11 as the raw UAV and there are 8 write images, we must revert
  // everything to the arena and not print out the default raw uav id.
  if (mSTM->device()->getGeneration() == AMDILDeviceInfo::HD5XXX
      || mSTM->device()->getGeneration() == AMDILDeviceInfo::HD6XXX) {
    if ((mSTM->device()->getResourceID(AMDILDevice::RAW_UAV_ID) < 11 &&
         mSTM->getGlobalManager()->getNumWriteImages(mName)
         != OPENCL_MAX_WRITE_IMAGES
         && !mSTM->device()->isSupported(AMDILDeviceInfo::MultiUAV))
        || mSTM->device()->getResourceID(AMDILDevice::RAW_UAV_ID) == 11) {
      if (!mMFI->usesMem(AMDILDevice::RAW_UAV_ID)
          && mMFI->uav_count(mSTM->device()->
              getResourceID(AMDILDevice::RAW_UAV_ID))) {
        O << "dcl_raw_uav_id("
          << mSTM->device()->getResourceID(AMDILDevice::RAW_UAV_ID);
        O << ")\n";
        mMFI->setUsesMem(AMDILDevice::RAW_UAV_ID);
      }
    }
    // If we have not printed out the arena ID yet, then do so here.
      if (!mMFI->usesMem(AMDILDevice::ARENA_UAV_ID)
          && mSTM->device()->usesHardware(AMDILDeviceInfo::ArenaUAV)) {
        O << "dcl_arena_uav_id("
          << mSTM->device()->getResourceID(AMDILDevice::ARENA_UAV_ID) << ")\n";
        mMFI->setUsesMem(AMDILDevice::ARENA_UAV_ID);
      }
  } else if (mSTM->device()->getGeneration() > AMDILDeviceInfo::HD6XXX) {
    binaryForEach(mMFI->uav_begin(), mMFI->uav_end(), uavPrintSI, O);
    mMFI->setUsesMem(AMDILDevice::RAW_UAV_ID);
  }
  getIntrinsicSetup(AsmPrinter, O);
}

void AMDILKernelManager::getIntrinsicSetup(AMDILAsmPrinter *AsmPrinter,
                                           llvm::raw_ostream &O)
{
  O << "mov r0.z, vThreadGrpIdFlat.x\n"
    << "mov r1022.xyz0, vTidInGrp.xyz\n";
  if (mSTM->device()->getGeneration() > AMDILDeviceInfo::HD4XXX) {
    O << "mov r1023.xyz0, vThreadGrpId.xyz\n";
   } else {
    O << "imul r0.w, cb0[2].x, cb0[2].y\n"
      // Calculates the local id.
      // Calculates the group id.
      << "umod r1023.x, r0.z, cb0[2].x\n"
      << "udiv r1023.y, r0.z, cb0[2].x\n"
      << "umod r1023.y, r1023.y, cb0[2].y\n"
      << "udiv r1023.z, r0.z, r0.w\n";
  }
  // Calculates the global id.
  if (mGM->hasRWG(mName) && 0) {
    // Anytime we declare a literal, we need to reserve it, if it is not emitted
    // in emitLiterals.
    mMFI->addReservedLiterals(1);
    O << "dcl_literal l" << mMFI->getNumLiterals() + 1 << ", ";
    O << mGM->getLocal(mName, 0) << ", ";
    O << mGM->getLocal(mName, 1) << ", ";
    O << mGM->getLocal(mName, 2) << ", ";
    O << "0\n";
    O << "imad r1021.xyz0, r1023.xyz, l" << mMFI->getNumLiterals() + 1 << ".xyz, r1022.xyz\n";
    mMFI->addReservedLiterals(1);
  } else {
    O << "imad r1021.xyz0, r1023.xyz, cb0[1].xyz, r1022.xyz\n";
  }

  // Add the global/group offset for multi-launch support.
  O << "iadd r1021.xyz0, r1021.xyz0, cb0[6].xyz0\n"
    << "iadd r1023.xyz0, r1023.xyz0, cb0[7].xyz0\n"
    // moves the flat group id.
    << "mov r1023.w, r0.z\n";
#ifdef UPSTREAM_LLVM
  if (mSTM->device()->usesSoftware(AMDILDeviceInfo::LocalMem)) {
    if (mSTM->is64bit()) {
      O << "umul " << AsmPrinter->getRegisterName(AMDIL::T2) 
        << ".x0, r1023.w, cb0[4].z\n"
        << "i64add " << AsmPrinter->getRegisterName(AMDIL::T2)
        << ".xy, " << AsmPrinter->getRegisterName(AMDIL::T2)
        << ".xy, cb0[4].xy\n";

    } else {
      O << "imad " << AsmPrinter->getRegisterName(AMDIL::T2)
        << ".x, r1023.w, cb0[4].y, cb0[4].x\n";
    }
  }
  // Shift the flat group id to be in bytes instead of dwords.
  O << "ishl r1023.w, r1023.w, l0.z\n";
  if (mSTM->device()->usesSoftware(AMDILDeviceInfo::PrivateMem)) {
    if (mSTM->is64bit()) {
      O << "umul " << AsmPrinter->getRegisterName(AMDIL::T1) 
        << ".x0, vAbsTidFlat.x, cb0[3].z\n"
        << "i64add " << AsmPrinter->getRegisterName(AMDIL::T1)
        << ".xy, " << AsmPrinter->getRegisterName(AMDIL::T1)
        << ".xy, cb0[3].xy\n";

    } else {
      O << "imad " << AsmPrinter->getRegisterName(AMDIL::T1)
        << ".x, vAbsTidFlat.x, cb0[3].y, cb0[3].x\n";
    }
  } else {
    O << "mov " << AsmPrinter->getRegisterName(AMDIL::T1) << ".x, l0.0\n";
  }
#endif
  if (mSTM->device()->isSupported(AMDILDeviceInfo::RegionMem)) {
    O << "udiv r1024.xyz, r1021.xyz, cb0[10].xyz\n";
    if (mGM->hasRWR(mName) && 0) {
      // Anytime we declare a literal, we need to reserve it, if it is not emitted
      // in emitLiterals.
      mMFI->addReservedLiterals(1);
      O << "dcl_literal l" << mMFI->getNumLiterals() + 1 << ", ";
      O << mGM->getLocal(mName, 0) << ", ";
      O << mGM->getLocal(mName, 1) << ", ";
      O << mGM->getLocal(mName, 2) << ", ";
      O << "0\n";
      O << "imad r1025.xyz0, r1023.xyz, l" << mMFI->getNumLiterals() + 1 << ".xyz, r1022.xyz\n";
      mMFI->addReservedLiterals(1);
    } else {
      O << "imad r1025.xyz0, r1023.xyz, cb0[1].xyz, r1022.xyz\n";
    }
  }
}

void AMDILKernelManager::printFooter(llvm::raw_ostream &O) {
  O << "ret\n";
  O << "endfunc ; " << mName << "\n";
}

void
AMDILKernelManager::printMetaData(llvm::raw_ostream &O, uint32_t id, bool kernel) {
  if (kernel) {
    int kernelId = mGM->getOrCreateFunctionID(mName);
    mMFI->addCalledFunc(id);
    mUniqueID = kernelId;
    mIsKernel = true;
  }
  printKernelArgs(O);
  if (kernel) {
    mIsKernel = false;
    mMFI->eraseCalledFunc(id);
    mUniqueID = id;
  }
}

void AMDILKernelManager::setKernel(bool kernel) {
  mIsKernel = kernel;
  if (kernel) {
    mWasKernel = mIsKernel;
  }
}

void AMDILKernelManager::setID(uint32_t id)
{
  mUniqueID = id;
}

void AMDILKernelManager::setName(const std::string &name) {
  mName = name;
}

bool AMDILKernelManager::isKernel() {
  return mIsKernel;
}

bool AMDILKernelManager::wasKernel() {
  return mWasKernel;
}

void AMDILKernelManager::setImageWrite() {
  mHasImageWrite = true;
}

void AMDILKernelManager::setOutputInst() {
  mHasOutputInst = true;
}

void AMDILKernelManager::printConstantToRegMapping(
       AMDILAsmPrinter *RegNames,
       uint32_t &LII,
       llvm::raw_ostream &O,
       uint32_t &Counter,
       uint32_t Buffer,
       uint32_t n,
       const char *lit,
       uint32_t fcall,
       bool isImage,
       bool isHWCB)
{
#ifdef UPSTREAM_LLVM
  // TODO: This needs to be enabled or SC will never statically index into the
  // CB when a pointer is used.
  if (mSTM->device()->usesHardware(AMDILDeviceInfo::ConstantMem) && isHWCB) {
    const char *name = RegNames->getRegisterName(LII);
    O << "mov " << name << ", l5.x\n";
    ++LII;
    Counter++;
    return;
  }
  for (uint32_t x = 0; x < n; ++x) {
    const char *name = RegNames->getRegisterName(LII);
    if (isImage) {
      O << "mov " << name << ", l" << mMFI->getIntLits(Counter++) << "\n";
    } else {
      O << "mov " << name << ", cb" <<Buffer<< "[" <<Counter++<< "]\n";
    }
    switch(fcall) {
    case 1093:
      O << "ishr " << name << ", " << name << ".xxyy, l3.0y0y\n"
        "ishl " << name << ", " << name << ", l3.y\n"
        "ishr " << name << ", " << name << ", l3.y\n";
      break;
    case 1092:
      O << "ishr " << name << ", " << name << ".xx, l3.0y\n"
        "ishl " << name << ", " << name << ", l3.y\n"
        "ishr " << name << ", " << name << ", l3.y\n";
      break;
    case 1091:
      O << "ishr " << name << ", " << name << ".xxxx, l3.0zyx\n"
        "ishl " << name << ", " << name << ", l3.x\n"
        "ishr " << name << ", " << name << ", l3.x\n";
      break;
    case 1090:
      O << "ishr " << name << ", " << name << ".xx, l3.0z\n"
        "ishl " << name << ".xy__, " << name << ".xy, l3.x\n"
        "ishr " << name << ".xy__, " << name << ".xy, l3.x\n";
      break;
    default:
      break;
    };
    if (lit) {
      O << "ishl " << name << ", " << name
        << ", " << lit << "\n";
      O << "ishr " << name << ", " << name
        << ", " << lit << "\n";
    }
    if (isImage) {
      Counter += NUM_EXTRA_SLOTS_PER_IMAGE;
    }
    ++LII;
  }
#endif
}

void
AMDILKernelManager::printCopyStructPrivate(const StructType *ST,
                                           llvm::raw_ostream &O,
                                           size_t stackSize,
                                           uint32_t Buffer,
                                           uint32_t mLitIdx,
                                           uint32_t &Counter)
{
  size_t n = ((stackSize + 15) & ~15) >> 4;
  for (size_t x = 0; x < n; ++x) {
    O << "mov r2, cb" << Buffer << "[" << Counter++ << "]\n";
    O << "mov r1.x, r0.x\n";
    if (mSTM->device()->getGeneration() <= AMDILDeviceInfo::HD6XXX) {
    if (mSTM->device()->usesHardware(AMDILDeviceInfo::PrivateMem)) {
      O << "ishr r1.x, r1.x, l0.x\n";
      O << "mov x" << mSTM->device()->getResourceID(AMDILDevice::SCRATCH_ID)
        <<"[r1.x], r2\n";
    } else {
        O << "uav_raw_store_id(" <<
          mSTM->device()->getResourceID(AMDILDevice::GLOBAL_ID)
          << ") mem0, r1.x, r2\n";
    }
    } else {
      O << "uav_raw_store_id(" <<
        mSTM->device()->getResourceID(AMDILDevice::SCRATCH_ID)
        << ") mem0, r1.x, r2\n";
    }
    O << "iadd r0.x, r0.x, l" << mLitIdx << ".z\n";
  }
}

void AMDILKernelManager::printKernelArgs(llvm::raw_ostream &O) {
  std::string version(";version:");
  version += itostr(AMDIL_MAJOR_VERSION) + ":"
    + itostr(AMDIL_MINOR_VERSION) + ":" + itostr(AMDIL_REVISION_NUMBER);
  O << ";ARGSTART:" <<mName<< "\n";
  if (mIsKernel) {
    O << version << "\n";
    O << ";device:" <<mSTM->getDeviceName() << "\n";
  }
  O << ";uniqueid:" <<mUniqueID<< "\n";
  
  size_t local = mGM->getLocalSize(mName);
  size_t hwlocal = ((mGM->getHWLocalSize(mName) + 3) & (~0x3));
  size_t region = mGM->getRegionSize(mName);
  size_t hwregion = ((mGM->getHWRegionSize(mName) + 3) & (~0x3));
  bool usehwlocal = mSTM->device()->usesHardware(AMDILDeviceInfo::LocalMem);
  bool usehwprivate = mSTM->device()->usesHardware(AMDILDeviceInfo::PrivateMem);
  bool usehwregion = mSTM->device()->usesHardware(AMDILDeviceInfo::RegionMem);
  bool useuavprivate = mSTM->device()->isSupported(AMDILDeviceInfo::PrivateUAV);
  if (mIsKernel) {
    O << ";memory:" << ((usehwprivate) ? 
        (useuavprivate) ? "uav" : "hw" : "" ) << "private:"
      <<(((mMFI->getStackSize() + 15) & (~0xF)))<< "\n";
  }
  if (mSTM->device()->isSupported(AMDILDeviceInfo::RegionMem)) {
    O << ";memory:" << ((usehwregion) ? "hw" : "") << "region:"
      << ((usehwregion) ? hwregion : hwregion + region) << "\n";
  }
  O << ";memory:" << ((usehwlocal) ? "hw" : "") << "local:"
    << ((usehwlocal) ? hwlocal : hwlocal + local) << "\n";
  
  if (mIsKernel) {
    if (mGM->hasRWG(mName)) {
      O << ";cws:" << mGM->getLocal(mName, 0) << ":";
      O << mGM->getLocal(mName, 1) << ":";
      O << mGM->getLocal(mName, 2) << "\n";
    }
    if (mGM->hasRWR(mName)) {
      O << ";crs:" << mGM->getRegion(mName, 0) << ":";
      O << mGM->getRegion(mName, 1) << ":";
      O << mGM->getRegion(mName, 2) << "\n";
    }
  }
  if (mIsKernel) {
    for (std::vector<std::string>::iterator ib = mMFI->kernel_md_begin(),
           ie = mMFI->kernel_md_end(); ib != ie; ++ib) {
      O << (*ib) << "\n";
    }
  }
  for (std::set<std::string>::iterator ib = mMFI->func_md_begin(),
         ie = mMFI->func_md_end(); ib != ie; ++ib) {
    O << (*ib) << "\n";
  }
  if (!mMFI->func_empty()) {
    O << ";function:" << mMFI->func_size();
    binaryForEach(mMFI->func_begin(), mMFI->func_end(), commaPrint, O);
    O << "\n";
  }

  if (!mSTM->device()->isSupported(AMDILDeviceInfo::MacroDB)
      && !mMFI->intr_empty()) {
    O << ";intrinsic:" << mMFI->intr_size();
    binaryForEach(mMFI->intr_begin(), mMFI->intr_end(), commaPrint, O);
    O << "\n";
  }

  if (!mIsKernel) {
    binaryForEach(mMFI->printf_begin(), mMFI->printf_end(), printfPrint, O);
    mMF->getMMI().getObjFileInfo<AMDILModuleInfo>().add_printf_offset(
        mMFI->printf_size());
  } else {
    for (StringMap<SamplerInfo>::iterator 
        smb = mMFI->sampler_begin(),
        sme = mMFI->sampler_end(); smb != sme; ++ smb) {
      O << ";sampler:" << (*smb).second.name << ":" << (*smb).second.idx
        << ":" << ((*smb).second.val == (uint32_t)-1 ? 0 : 1) 
        << ":" << ((*smb).second.val != (uint32_t)-1 ? (*smb).second.val : 0)
        << "\n";
    }
  }
  if (mSTM->is64bit()) {
    O << ";memory:64bitABI\n";
  }

  if (mMFI->errors_empty()) {
    binaryForEach(mMFI->errors_begin(), mMFI->errors_end(), errorPrint, O);
  }
  // This has to come last
  if (mIsKernel 
      && mSTM->device()->getGeneration() <= AMDILDeviceInfo::HD6XXX) {
    if (mSTM->device()->getResourceID(AMDILDevice::RAW_UAV_ID) >
        mSTM->device()->getResourceID(AMDILDevice::ARENA_UAV_ID)) {
      if (mMFI->uav_size() == 1) {
        if (mSTM->device()->isSupported(AMDILDeviceInfo::ArenaSegment)
            && *(mMFI->uav_begin()) >= ARENA_SEGMENT_RESERVED_UAVS) {
          O << ";uavid:"
            << mSTM->device()->getResourceID(AMDILDevice::ARENA_UAV_ID);
          O << "\n";
        } else {
          O << ";uavid:" << *(mMFI->uav_begin()) << "\n";
        }
      } else if (mMFI->uav_count(mSTM->device()->
            getResourceID(AMDILDevice::RAW_UAV_ID))) {
        O << ";uavid:"
          << mSTM->device()->getResourceID(AMDILDevice::RAW_UAV_ID);
        O << "\n";
      } else {
        O << ";uavid:"
          << mSTM->device()->getResourceID(AMDILDevice::ARENA_UAV_ID);
        O << "\n";
      }
    } else if (mSTM->getGlobalManager()->getNumWriteImages(mName) !=
        OPENCL_MAX_WRITE_IMAGES
        && !mSTM->device()->isSupported(AMDILDeviceInfo::ArenaSegment)
        && mMFI->uav_count(mSTM->device()->
          getResourceID(AMDILDevice::RAW_UAV_ID))) {
      O << ";uavid:"
        << mSTM->device()->getResourceID(AMDILDevice::RAW_UAV_ID) << "\n";
    } else if (mMFI->uav_size() == 1) {
      O << ";uavid:" << *(mMFI->uav_begin()) << "\n";
    } else {
      O << ";uavid:"
        << mSTM->device()->getResourceID(AMDILDevice::ARENA_UAV_ID);
      O << "\n";
    }
  }
  O << ";ARGEND:" << mName << "\n";
}

void AMDILKernelManager::printArgCopies(llvm::raw_ostream &O,
    AMDILAsmPrinter *RegNames)
{
  Function::const_arg_iterator I = mMF->getFunction()->arg_begin();
  Function::const_arg_iterator Ie = mMF->getFunction()->arg_end();
  uint32_t Counter = 0;

  if (mMFI->getArgSize()) {
    O << "dcl_cb cb1";
    O << "[" << (mMFI->getArgSize() >> 4) << "]\n";
    mMFI->setUsesMem(AMDILDevice::CONSTANT_ID);
  }
  const Function *F = mMF->getFunction();
  // Get the stack size
  uint32_t stackSize = mMFI->getStackSize();
  uint32_t privateSize = mMFI->getScratchSize();
  uint32_t stackOffset = (privateSize + 15) & (~0xF);
  if (stackSize 
      && mSTM->device()->usesHardware(AMDILDeviceInfo::PrivateMem)) {
    // TODO: If the size is too large, we need to fall back to software emulated
    // instead of using the hardware capability.
    int size = (((stackSize + 15) & (~0xF)) >> 4);
    if (size > 4096) {
      mMFI->addErrorMsg(amd::CompilerErrorMessage[INSUFFICIENT_PRIVATE_RESOURCES]);
    }
    if (size) {
    // For any stack variables, we need to declare the literals for them so that
    // we can use them when we copy our data to the stack.
    mMFI->addReservedLiterals(1);
    // Anytime we declare a literal, we need to reserve it, if it is not emitted
    // in emitLiterals.
#ifdef UPSTREAM_LLVM
    O << "dcl_literal l" << mMFI->getNumLiterals() << ", " << stackSize << ", "
      << privateSize << ", 16, " << ((stackSize == privateSize) ? 0 : stackOffset) << "\n"
      << "iadd r0.x, " << RegNames->getRegisterName(AMDIL::T1) << ".x, l"
      << mMFI->getNumLiterals() << ".w\n";
    if (mSTM->device()->getGeneration() <= AMDILDeviceInfo::HD6XXX) {
    O << "dcl_indexed_temp_array x"
      << mSTM->device()->getResourceID(AMDILDevice::SCRATCH_ID) << "["
      << size << "]\n";
    } else {
      O << "dcl_typeless_uav_id("
        << mSTM->device()->getResourceID(AMDILDevice::SCRATCH_ID) 
        << ")_stride(4)_length(" << (size  << 4 )<< ")_access(private)\n";

    }
    O << "mov " << RegNames->getRegisterName(AMDIL::FP) 
      << ".x, l" << mMFI->getNumLiterals() << ".0\n";
#endif    
    mMFI->setUsesMem(AMDILDevice::SCRATCH_ID);
    }
  }
  I = mMF->getFunction()->arg_begin();
  int32_t count = 0;
  // uint32_t Image = 0;
  bool displaced1 = false;
  bool displaced2 = false;
  uint32_t curReg = AMDIL::R1;
  // TODO: We don't handle arguments that were pushed onto the stack!
  for (; I != Ie; ++I) {
    Type *curType = I->getType();
    unsigned int Buffer = 1;
    O << "; Kernel arg setup: " << I->getName() << "\n";
    if (curType->isIntegerTy() || curType->isFloatingPointTy()) {
      switch (curType->getPrimitiveSizeInBits()) {
        default:
          printConstantToRegMapping(RegNames, curReg, O, Counter, Buffer, 1);
          break;
        case 16:
          printConstantToRegMapping(RegNames, curReg, O, Counter, Buffer, 1,
              "l3.y" );
          break;
        case 8:
          printConstantToRegMapping(RegNames, curReg, O, Counter, Buffer, 1, "l3.x" );
          break;
      }
#ifdef UPSTREAM_LLVM
    } else if (const VectorType *VT = dyn_cast<VectorType>(curType)) {
      Type *ET = VT->getElementType();
      int numEle = VT->getNumElements();
      switch (ET->getPrimitiveSizeInBits()) {
        default:
          if (numEle == 3) {
            O << "mov " << RegNames->getRegisterName(curReg);
            O << ".x, cb" << Buffer << "[" << Counter << "].x\n";
            curReg++;
            O << "mov " << RegNames->getRegisterName(curReg);
            O << ".x, cb" << Buffer << "[" << Counter << "].y\n";
            curReg++;
            O << "mov " << RegNames->getRegisterName(curReg);
            O << ".x, cb" << Buffer << "[" << Counter << "].z\n";
            curReg++;
            Counter++;
          } else {
            printConstantToRegMapping(RegNames, curReg, O, Counter, Buffer,
                (numEle+2) >> 2);
          }
          break;
        case 64:
          if (numEle == 3) {
            O << "mov " << RegNames->getRegisterName(curReg);
            O << ".xy, cb" << Buffer << "[" << Counter << "].xy\n";
            curReg++;
            O << "mov " << RegNames->getRegisterName(curReg);
            O << ".xy, cb" << Buffer << "[" << Counter++ << "].zw\n";
            curReg++;
            O << "mov " << RegNames->getRegisterName(curReg);
            O << ".xy, cb" << Buffer << "[" << Counter << "].xy\n";
            curReg++;
            Counter++;
          } else {
            printConstantToRegMapping(RegNames, curReg, O, Counter, Buffer,
                (numEle) >> 1);
          }
          break;
        case 16: 
          {
                   switch (numEle) {
                     default:
                       printConstantToRegMapping(RegNames, curReg, O, Counter,
                           Buffer, (numEle+2) >> 2, "l3.y", 1093);
                       if (numEle == 3) {
                         O << "mov " << RegNames->getRegisterName(curReg) << ".x, ";
                         O << RegNames->getRegisterName(curReg) << ".y\n";
                         ++curReg;
                         O << "mov " << RegNames->getRegisterName(curReg) << ".x, ";
                         O << RegNames->getRegisterName(curReg) << ".z\n";
                         ++curReg;
                       }
                       break;
                     case 2:
                       printConstantToRegMapping(RegNames, curReg, O, Counter,
                           Buffer, 1, "l3.y", 1092);
                       break;
                   }
                   break;
                 }
        case 8: 
          {
                  switch (numEle) {
                    default:
                      printConstantToRegMapping(RegNames, curReg, O, Counter,
                          Buffer, (numEle+2) >> 2, "l3.x", 1091);
                      if (numEle == 3) {
                        O << "mov " << RegNames->getRegisterName(curReg) << ".x, ";
                        O << RegNames->getRegisterName(curReg) << ".y\n";
                        ++curReg;
                        O << "mov " << RegNames->getRegisterName(curReg) << ".x, ";
                        O << RegNames->getRegisterName(curReg) << ".z\n";
                        ++curReg;
                      }
                      break;
                    case 2:
                      printConstantToRegMapping(RegNames, curReg, O, Counter,
                          Buffer, 1, "l3.x", 1090);
                      break;
                  }
                  break;
                }
      }
#endif
    } else if (const PointerType *PT = dyn_cast<PointerType>(curType)) {
      Type *CT = PT->getElementType();
      const StructType *ST = dyn_cast<StructType>(CT);
      if (ST && ST->isOpaque()) {
        bool i1d  = ST->getName() == "struct._image1d_t";
        bool i1da = ST->getName() == "struct._image1d_array_t";
        bool i1db = ST->getName() == "struct._image1d_buffer_t";
        bool i2d  = ST->getName() == "struct._image2d_t";
        bool i2da = ST->getName() == "struct._image2d_array_t";
        bool i3d  = ST->getName() == "struct._image3d_t";
        bool is_image = i1d || i1da || i1db || i2d || i2da || i3d;
        if (is_image) {
          if (mSTM->device()->isSupported(AMDILDeviceInfo::Images)) {
            printConstantToRegMapping(RegNames, curReg, O, Counter, Buffer,
                1, NULL, 0, is_image);
          } else {
            mMFI->addErrorMsg(
                amd::CompilerErrorMessage[NO_IMAGE_SUPPORT]);
            ++curReg;
          }
        } else {
          printConstantToRegMapping(RegNames, curReg, O, Counter, Buffer, 1);
        }
      } else if (CT->isStructTy()
          && PT->getAddressSpace() == AMDILAS::PRIVATE_ADDRESS) {
        StructType *ST = dyn_cast<StructType>(CT);
        bool i1d  = ST->getName() == "struct._image1d_t";
        bool i1da = ST->getName() == "struct._image1d_array_t";
        bool i1db = ST->getName() == "struct._image1d_buffer_t";
        bool i2d  = ST->getName() == "struct._image2d_t";
        bool i2da = ST->getName() == "struct._image2d_array_t";
        bool i3d  = ST->getName() == "struct._image3d_t";
        bool is_image = i1d || i1da || i1db || i2d || i2da || i3d;
        if (is_image) {
          if (mSTM->device()->isSupported(AMDILDeviceInfo::Images)) {
            printConstantToRegMapping(RegNames, curReg, O, Counter, Buffer,
                1, NULL, 0, is_image);
          } else {
            mMFI->addErrorMsg(amd::CompilerErrorMessage[NO_IMAGE_SUPPORT]);
            ++curReg;
          }
        } else {
          if (count) {
            // Anytime we declare a literal, we need to reserve it, if it
            // is not emitted in emitLiterals.
            mMFI->addReservedLiterals(1);
            O << "dcl_literal l" << mMFI->getNumLiterals() << ", "
              << -stackSize << ", " << stackSize << ", 16, "
              << stackOffset << "\n";
          }
          ++count;
          size_t structSize;
          structSize = (getTypeSize(ST) + 15) & ~15;
          stackOffset += structSize;
#ifdef UPSTREAM_LLVM
          O << "mov " << RegNames->getRegisterName((curReg)) << ", l"
            << mMFI->getNumLiterals()<< ".w\n";
          if (!displaced1) {
            O << "mov r1011, r1\n";
            displaced1 = true;
          }
          if (!displaced2 && strcmp(RegNames->getRegisterName(curReg), "r1")) {
            O << "mov r1010, r2\n";
            displaced2 = true;
          }
#endif
          printCopyStructPrivate(ST, O, structSize, Buffer, mMFI->getNumLiterals(),
              Counter);
          ++curReg;
        }
      } else if (CT->isIntOrIntVectorTy()
          || CT->isFPOrFPVectorTy()
          || CT->isArrayTy()
          || CT->isPointerTy()
          || PT->getAddressSpace() != AMDILAS::PRIVATE_ADDRESS) {
        if (PT->getAddressSpace() == AMDILAS::CONSTANT_ADDRESS) {
          const kernel& krnl = mGM->getKernel(F->getName());
          printConstantToRegMapping(RegNames, curReg, O, Counter, Buffer,
              1, NULL, 0, false, 
              mGM->usesHWConstant(krnl, I->getName()));
        } else if (PT->getAddressSpace() == AMDILAS::REGION_ADDRESS) {
          // TODO: If we are region address space, the first region pointer, no
          // array pointers exist, and hardware RegionMem is enabled then we can
          // zero out register as the initial offset is zero.
          printConstantToRegMapping(RegNames, curReg, O, Counter, Buffer, 1);
        } else if (PT->getAddressSpace() == AMDILAS::LOCAL_ADDRESS) {
          // TODO: If we are local address space, the first local pointer, no
          // array pointers exist, and hardware LocalMem is enabled then we can
          // zero out register as the initial offset is zero.
          printConstantToRegMapping(RegNames, curReg, O, Counter, Buffer, 1);
        } else {
          printConstantToRegMapping(RegNames, curReg, O, Counter, Buffer, 1);
        }
      } else {
        assert(0 && "Current type is not supported!");
        mMFI->addErrorMsg(amd::CompilerErrorMessage[INTERNAL_ERROR]);
        ++curReg;
      }
    } else {
      assert(0 && "Current type is not supported!");
      mMFI->addErrorMsg(amd::CompilerErrorMessage[INTERNAL_ERROR]);
      ++curReg;
    }
  }
  if (displaced1) {
    O << "mov r1, r1011\n";
  }
  if (displaced2) {
    O << "mov r2, r1010\n";
  }
  if (mSTM->device()->usesHardware(AMDILDeviceInfo::ConstantMem)) {
    const kernel& krnl = mGM->getKernel(F->getName());
    uint32_t constNum = 0;
    for (uint32_t x = 0; x < mSTM->device()->getMaxNumCBs(); ++x) {
      if (krnl.constSizes[x]) {
        O << "dcl_cb cb" << x + CB_BASE_OFFSET;
        O << "[" << (((krnl.constSizes[x] + 15) & ~15) >> 4) << "]\n";
        ++constNum;
        mMFI->setUsesMem(AMDILDevice::CONSTANT_ID);
      }
    }
    // TODO: If we run out of constant resources, we need to push some of the
    // constant pointers to the software emulated section.
    if (constNum > mSTM->device()->getMaxNumCBs()) {
      assert(0 && "Max constant buffer limit passed!");
      mMFI->addErrorMsg(amd::CompilerErrorMessage[INSUFFICIENT_CONSTANT_RESOURCES]);
    }
  }
}

  const char *
AMDILKernelManager::getTypeName(const Type *ptr, const char *symTab)
{
  // symTab argument is ignored...
  LLVMContext& ctx = ptr->getContext();
  switch (ptr->getTypeID()) {
    case Type::StructTyID:
      {
        const StructType *ST = cast<StructType>(ptr);
        if (!ST->isOpaque())
          return "struct";
        // ptr is a pre-LLVM 3.0 "opaque" type.
        StringRef name = ST->getName();
        if (name.equals( "struct._event_t" ))         return "event";
        if (name.equals( "struct._image1d_t" ))       return "image1d";
        if (name.equals( "struct._image1d_array_t" )) return "image1d_array";
        if (name.equals( "struct._image2d_t" ))       return "image2d";
        if (name.equals( "struct._image2d_array_t" )) return "image2d_array";
        if (name.equals( "struct._image3d_t" ))       return "image3d";
        if (name.equals( "struct._counter32_t" ))     return "counter32";
        if (name.equals( "struct._counter64_t" ))     return "counter64";
        return "opaque";
        break;
      }
    case Type::FloatTyID:
      return "float";
    case Type::DoubleTyID: 
      {
        const AMDILSubtarget *mSTM= mTM->getSubtargetImpl();
        if (!mSTM->device()->usesHardware(AMDILDeviceInfo::DoubleOps)) {
          mMFI->addErrorMsg(amd::CompilerErrorMessage[DOUBLE_NOT_SUPPORTED]);
        }
        return "double";
      }
    case Type::IntegerTyID: 
      {
        if (ptr == Type::getInt8Ty(ctx)) {
          return "i8";
        } else if (ptr == Type::getInt16Ty(ctx)) {
          return "i16";
        } else if (ptr == Type::getInt32Ty(ctx)) {
          return "i32";
        } else if(ptr == Type::getInt64Ty(ctx)) {
          return "i64";
        }
        break;
      }
    default:
      break;
    case Type::ArrayTyID: 
      {
        const ArrayType *AT = cast<ArrayType>(ptr);
        const Type *name = AT->getElementType();
        return getTypeName(name, symTab);
        break;
      }
    case Type::VectorTyID: 
      {
        const VectorType *VT = cast<VectorType>(ptr);
        const Type *name = VT->getElementType();
        return getTypeName(name, symTab);
        break;
      }
    case Type::PointerTyID: 
      {
        const PointerType *PT = cast<PointerType>(ptr);
        const Type *name = PT->getElementType();
        return getTypeName(name, symTab);
        break;
      }
    case Type::FunctionTyID: 
      {
        const FunctionType *FT = cast<FunctionType>(ptr);
        const Type *name = FT->getReturnType();
        return getTypeName(name, symTab);
        break;
      }
  }
  ptr->dump();
  mMFI->addErrorMsg(amd::CompilerErrorMessage[UNKNOWN_TYPE_NAME]);
  return "unknown";
}

void AMDILKernelManager::emitLiterals(llvm::raw_ostream &O) {
  char buffer[256];
  std::map<uint32_t, uint32_t>::iterator ilb, ile;
  for (ilb = mMFI->begin_32(), ile = mMFI->end_32(); ilb != ile; ++ilb) {
    uint32_t a = ilb->first;
    O << "dcl_literal l" <<ilb->second<< ", ";
    sprintf(buffer, "0x%08x, 0x%08x, 0x%08x, 0x%08x", a, a, a, a);
    O << buffer << "; f32:i32 " << ilb->first << "\n";
  }
  std::map<uint64_t, uint32_t>::iterator llb, lle;
  for (llb = mMFI->begin_64(), lle = mMFI->end_64(); llb != lle; ++llb) {
    uint32_t v[2];
    uint64_t a = llb->first;
    memcpy(v, &a, sizeof(uint64_t));
    O << "dcl_literal l" <<llb->second<< ", ";
    sprintf(buffer, "0x%08x, 0x%08x, 0x%08x, 0x%08x; f64:i64 ",
        v[0], v[1], v[0], v[1]);
    O << buffer << llb->first << "\n";
  }
  std::map<std::pair<uint64_t, uint64_t>, uint32_t>::iterator vlb, vle;
  for (vlb = mMFI->begin_128(), vle = mMFI->end_128(); vlb != vle; ++vlb) {
    uint32_t v[2][2];
    uint64_t a = vlb->first.first;
    uint64_t b = vlb->first.second;
    memcpy(v[0], &a, sizeof(uint64_t));
    memcpy(v[1], &b, sizeof(uint64_t));
    O << "dcl_literal l" << vlb->second << ", ";
    sprintf(buffer, "0x%08x, 0x%08x, 0x%08x, 0x%08x; f128:i128 ",
        v[0][0], v[0][1], v[1][0], v[1][1]);
    O << buffer << vlb->first.first << vlb->first.second << "\n";
  }
}

// If the value is not known, then the uav is set, otherwise the mValueIDMap
// is used.
void AMDILKernelManager::setUAVID(const Value *value, uint32_t ID) {
  if (value) {
    mValueIDMap[value] = ID;
  }
}

uint32_t AMDILKernelManager::getUAVID(const Value *value) {
  if (mValueIDMap.find(value) != mValueIDMap.end()) {
    return mValueIDMap[value];
  }

  if (mSTM->device()->getGeneration() <= AMDILDeviceInfo::HD6XXX) {
    return mSTM->device()->getResourceID(AMDILDevice::ARENA_UAV_ID);
  } else {
    return mSTM->device()->getResourceID(AMDILDevice::RAW_UAV_ID);
  }
}


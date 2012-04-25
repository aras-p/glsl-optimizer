//===-- AMDILMachineFunctionInfo.cpp - TODO: Add brief description -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//
#include "AMDILMachineFunctionInfo.h"
#include "AMDILCompilerErrors.h"
#include "AMDILSubtarget.h"
#include "AMDILTargetMachine.h"
#include "AMDILUtilityFunctions.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/Support/FormattedStream.h"

using namespace llvm;

static const AMDILConstPtr *getConstPtr(const AMDILKernel *krnl, const std::string &arg) {
  llvm::SmallVector<AMDILConstPtr, DEFAULT_VEC_SLOTS>::const_iterator begin, end;
  for (begin = krnl->constPtr.begin(), end = krnl->constPtr.end();
       begin != end; ++begin) {
    if (!strcmp(begin->name.data(),arg.c_str())) {
      return &(*begin);
    }
  }
  return NULL;
}

void PrintfInfo::addOperand(size_t idx, uint32_t size) {
  mOperands.resize((unsigned)(idx + 1));
  mOperands[(unsigned)idx] = size;
}

uint32_t PrintfInfo::getPrintfID() {
  return mPrintfID;
}

void PrintfInfo::setPrintfID(uint32_t id) {
  mPrintfID = id;
}

size_t PrintfInfo::getNumOperands() {
  return mOperands.size();
}

uint32_t PrintfInfo::getOperandID(uint32_t idx) {
  return mOperands[idx];
}

AMDILMachineFunctionInfo::AMDILMachineFunctionInfo()
  : CalleeSavedFrameSize(0), BytesToPopOnReturn(0),
  DecorationStyle(None), ReturnAddrIndex(0),
  TailCallReturnAddrDelta(0),
  SRetReturnReg(0), UsesLDS(false), LDSArg(false),
  UsesGDS(false), GDSArg(false),
  mReservedLits(9)
{
  for (uint32_t x = 0; x < AMDILDevice::MAX_IDS; ++x) {
    mUsedMem[x] = false;
  }
  mMF = NULL;
  mKernel = NULL;
  mScratchSize = -1;
  mArgSize = -1;
  mStackSize = -1;
}

AMDILMachineFunctionInfo::AMDILMachineFunctionInfo(MachineFunction& MF)
  : CalleeSavedFrameSize(0), BytesToPopOnReturn(0),
  DecorationStyle(None), ReturnAddrIndex(0),
  TailCallReturnAddrDelta(0),
  SRetReturnReg(0), UsesLDS(false), LDSArg(false),
  UsesGDS(false), GDSArg(false),
  mReservedLits(9)
{
  for (uint32_t x = 0; x < AMDILDevice::MAX_IDS; ++x) {
    mUsedMem[x] = false;
  }
  mMF = &MF;
  const AMDILTargetMachine *TM = 
      reinterpret_cast<const AMDILTargetMachine*>(&MF.getTarget());
  mSTM = TM->getSubtargetImpl();

  mScratchSize = -1;
  mArgSize = -1;
  mStackSize = -1;
}

AMDILMachineFunctionInfo::~AMDILMachineFunctionInfo()
{
  for (std::map<std::string, PrintfInfo*>::iterator pfb = printf_begin(),
      pfe = printf_end(); pfb != pfe; ++pfb) {
    delete pfb->second;
  }
}
unsigned int
AMDILMachineFunctionInfo::getCalleeSavedFrameSize() const
{
  return CalleeSavedFrameSize;
}
void
AMDILMachineFunctionInfo::setCalleeSavedFrameSize(unsigned int bytes)
{
  CalleeSavedFrameSize = bytes;
}
unsigned int
AMDILMachineFunctionInfo::getBytesToPopOnReturn() const
{
  return BytesToPopOnReturn;
}
void
AMDILMachineFunctionInfo::setBytesToPopOnReturn(unsigned int bytes)
{
  BytesToPopOnReturn = bytes;
}
NameDecorationStyle
AMDILMachineFunctionInfo::getDecorationStyle() const
{
  return DecorationStyle;
}
void
AMDILMachineFunctionInfo::setDecorationStyle(NameDecorationStyle style)
{
  DecorationStyle = style;
}
int
AMDILMachineFunctionInfo::getRAIndex() const
{
  return ReturnAddrIndex;
}
void
AMDILMachineFunctionInfo::setRAIndex(int index)
{
  ReturnAddrIndex = index;
}
int
AMDILMachineFunctionInfo::getTCReturnAddrDelta() const
{
  return TailCallReturnAddrDelta;
}
void
AMDILMachineFunctionInfo::setTCReturnAddrDelta(int delta)
{
  TailCallReturnAddrDelta = delta;
}
unsigned int
AMDILMachineFunctionInfo::getSRetReturnReg() const
{
  return SRetReturnReg;
}
void
AMDILMachineFunctionInfo::setSRetReturnReg(unsigned int reg)
{
  SRetReturnReg = reg;
}

void 
AMDILMachineFunctionInfo::setUsesLocal()
{
  UsesLDS = true;
}

bool
AMDILMachineFunctionInfo::usesLocal() const
{
  return UsesLDS;
}

void 
AMDILMachineFunctionInfo::setHasLocalArg()
{
  LDSArg = true;
}

bool
AMDILMachineFunctionInfo::hasLocalArg() const
{
  return LDSArg;
}



void
AMDILMachineFunctionInfo::setUsesRegion()
{
  UsesGDS = true;
}

bool
AMDILMachineFunctionInfo::usesRegion() const
{
  return UsesGDS;
}

void 
AMDILMachineFunctionInfo::setHasRegionArg()
{
  GDSArg = true;
}

bool
AMDILMachineFunctionInfo::hasRegionArg() const
{
  return GDSArg;
}


bool
AMDILMachineFunctionInfo::usesHWConstant(std::string name) const
{
  const AMDILConstPtr *curConst = getConstPtr(mKernel, name);
  if (curConst) {
    return curConst->usesHardware;
  } else {
    return false;
  }
}

uint32_t
AMDILMachineFunctionInfo::getLocal(uint32_t dim)
{
  if (mKernel && mKernel->sgv) {
    AMDILKernelAttr *sgv = mKernel->sgv;
    switch (dim) {
    default: break;
    case 0:
    case 1:
    case 2:
      return sgv->reqGroupSize[dim];
      break;
    case 3:
      return sgv->reqGroupSize[0] * sgv->reqGroupSize[1] * sgv->reqGroupSize[2];
    };
  }
  switch (dim) {
  default:
    return 1;
  case 3:
    return mSTM->getDefaultSize(0) *
           mSTM->getDefaultSize(1) *
           mSTM->getDefaultSize(2);
  case 2:
  case 1:
  case 0:
    return mSTM->getDefaultSize(dim);
    break;
  };
  return 1;
}
bool
AMDILMachineFunctionInfo::isKernel() const
{
  return mKernel != NULL && mKernel->mKernel;
}

AMDILKernel*
AMDILMachineFunctionInfo::getKernel()
{
  return mKernel;
}

std::string
AMDILMachineFunctionInfo::getName()
{
  if (mMF) {
    return mMF->getFunction()->getName();
  } else {
    return "";
  }
}

uint32_t
AMDILMachineFunctionInfo::getArgSize()
{
  if (mArgSize == -1) {
    Function::const_arg_iterator I = mMF->getFunction()->arg_begin();
    Function::const_arg_iterator Ie = mMF->getFunction()->arg_end();
    uint32_t Counter = 0;
    while (I != Ie) {
      Type* curType = I->getType();
      if (curType->isIntegerTy() || curType->isFloatingPointTy()) {
        ++Counter;
      } else if (const VectorType *VT = dyn_cast<VectorType>(curType)) {
        Type *ET = VT->getElementType();
        int numEle = VT->getNumElements();
        switch (ET->getPrimitiveSizeInBits()) {
          default:
            if (numEle == 3) {
              Counter++;
            } else {
              Counter += ((numEle + 2) >> 2);
            }
            break;
          case 64:
            if (numEle == 3) {
              Counter += 2;
            } else {
              Counter += (numEle >> 1);
            }
            break;
          case 16:
          case 8:
            switch (numEle) {
              default:
                Counter += ((numEle + 2) >> 2);
              case 2:
                Counter++;
                break;
            }
            break;
        }
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
              Counter += 2;
            } else {
              addErrorMsg(amd::CompilerErrorMessage[NO_IMAGE_SUPPORT]);
            }
          } else {
            Counter++;
          }
        } else if (CT->isStructTy()
            && PT->getAddressSpace() == AMDILAS::PRIVATE_ADDRESS) {
          StructType *ST = dyn_cast<StructType>(CT);
          Counter += ((getTypeSize(ST) + 15) & ~15) >> 4;
        } else if (CT->isIntOrIntVectorTy()
            || CT->isFPOrFPVectorTy()
            || CT->isArrayTy()
            || CT->isPointerTy()
            || PT->getAddressSpace() != AMDILAS::PRIVATE_ADDRESS) {
          ++Counter;
        } else {
          assert(0 && "Current type is not supported!");
          addErrorMsg(amd::CompilerErrorMessage[INTERNAL_ERROR]);
        }
      } else {
        assert(0 && "Current type is not supported!");
        addErrorMsg(amd::CompilerErrorMessage[INTERNAL_ERROR]);
      }
      ++I;
    }
    // Convert from slots to bytes by multiplying by 16(shift by 4).
    mArgSize = Counter << 4;
  }
  return (uint32_t)mArgSize;
}
  uint32_t
AMDILMachineFunctionInfo::getScratchSize()
{
  if (mScratchSize == -1) {
    mScratchSize = 0;
    Function::const_arg_iterator I = mMF->getFunction()->arg_begin();
    Function::const_arg_iterator Ie = mMF->getFunction()->arg_end();
    while (I != Ie) {
      Type *curType = I->getType();
      mScratchSize += ((getTypeSize(curType) + 15) & ~15);
      ++I;
    }
    mScratchSize += ((mScratchSize + 15) & ~15);
  }
  return (uint32_t)mScratchSize;
}

  uint32_t
AMDILMachineFunctionInfo::getStackSize()
{
  if (mStackSize == -1) {
    uint32_t privSize = 0;
    const MachineFrameInfo *MFI = mMF->getFrameInfo();
    privSize = MFI->getOffsetAdjustment() + MFI->getStackSize();
    const AMDILTargetMachine *TM = 
      reinterpret_cast<const AMDILTargetMachine*>(&mMF->getTarget());
    bool addStackSize = TM->getOptLevel() == CodeGenOpt::None;
    Function::const_arg_iterator I = mMF->getFunction()->arg_begin();
    Function::const_arg_iterator Ie = mMF->getFunction()->arg_end();
    while (I != Ie) {
      Type *curType = I->getType();
      ++I;
      if (dyn_cast<PointerType>(curType)) {
        Type *CT = dyn_cast<PointerType>(curType)->getElementType();
        if (CT->isStructTy()
            && dyn_cast<PointerType>(curType)->getAddressSpace() 
            == AMDILAS::PRIVATE_ADDRESS) {
          addStackSize = true;
        }
      }
    }
    if (addStackSize) {
      privSize += getScratchSize();
    }
    mStackSize = privSize;
  }
  return (uint32_t)mStackSize;

}

uint32_t 
AMDILMachineFunctionInfo::addi32Literal(uint32_t val, int Opcode) {
  // Since we have emulated 16/8/1 bit register types with a 32bit real
  // register, we need to sign extend the constants to 32bits in order for
  // comparisons against the constants to work correctly, this fixes some issues
  // we had in conformance failing for saturation.
  if (Opcode == AMDIL::LOADCONST_i16) {
    val = (((int32_t)val << 16) >> 16);
  } else if (Opcode == AMDIL::LOADCONST_i8) {
    val = (((int32_t)val << 24) >> 24);
  }
  if (mIntLits.find(val) == mIntLits.end()) {
    mIntLits[val] = getNumLiterals();
  }
  return mIntLits[val];
}

uint32_t 
AMDILMachineFunctionInfo::addi64Literal(uint64_t val) {
  if (mLongLits.find(val) == mLongLits.end()) {
    mLongLits[val] = getNumLiterals();
  }
  return mLongLits[val];
}

uint32_t 
AMDILMachineFunctionInfo::addi128Literal(uint64_t val_lo, uint64_t val_hi) {
  std::pair<uint64_t, uint64_t> a;
  a.first = val_lo;
  a.second = val_hi;
  if (mVecLits.find(a) == mVecLits.end()) {
    mVecLits[a] = getNumLiterals();
  }
  return mVecLits[a];
}

uint32_t 
AMDILMachineFunctionInfo::addf32Literal(const ConstantFP *CFP) {
  uint32_t val = (uint32_t)CFP->getValueAPF().bitcastToAPInt().getZExtValue();
  if (mIntLits.find(val) == mIntLits.end()) {
    mIntLits[val] = getNumLiterals();
  }
  return mIntLits[val];
}

uint32_t 
AMDILMachineFunctionInfo::addf64Literal(const ConstantFP *CFP) {
  union dtol_union {
    double d;
    uint64_t ul;
  } dval;
  const APFloat &APF = CFP->getValueAPF();
  if (&APF.getSemantics() == (const llvm::fltSemantics *)&APFloat::IEEEsingle) {
    float fval = APF.convertToFloat();
    dval.d = (double)fval;
  } else {
    dval.d = APF.convertToDouble();
  }
  if (mLongLits.find(dval.ul) == mLongLits.end()) {
    mLongLits[dval.ul] = getNumLiterals();
  }
  return mLongLits[dval.ul];
}

  uint32_t 
AMDILMachineFunctionInfo::getIntLits(uint32_t offset) 
{
  return mIntLits[offset];
}

  uint32_t 
AMDILMachineFunctionInfo::getLongLits(uint64_t offset) 
{
  return mLongLits[offset];
}

  uint32_t
AMDILMachineFunctionInfo::getVecLits(uint64_t low64, uint64_t high64)
{
  return mVecLits[std::pair<uint64_t, uint64_t>(low64, high64)];
}

size_t 
AMDILMachineFunctionInfo::getNumLiterals() const {
  return mLongLits.size() + mIntLits.size() + mVecLits.size() + mReservedLits;
}

  void
AMDILMachineFunctionInfo::addReservedLiterals(uint32_t size)
{
  mReservedLits += size;
}

  uint32_t 
AMDILMachineFunctionInfo::addSampler(std::string name, uint32_t val)
{
  if (mSamplerMap.find(name) != mSamplerMap.end()) {
    SamplerInfo newVal = mSamplerMap[name];
    assert(newVal.val == val 
        && "Found a sampler with same name but different values!");
    return mSamplerMap[name].idx;
  } else {
    SamplerInfo curVal;
    curVal.name = name;
    curVal.val = val;
    curVal.idx = mSamplerMap.size();
    mSamplerMap[name] = curVal;
    return curVal.idx;
  }
}

void
AMDILMachineFunctionInfo::setUsesMem(unsigned id) {
  assert(id < AMDILDevice::MAX_IDS &&
      "Must set the ID to be less than MAX_IDS!");
  mUsedMem[id] = true;
}

bool 
AMDILMachineFunctionInfo::usesMem(unsigned id) {
  assert(id < AMDILDevice::MAX_IDS &&
      "Must set the ID to be less than MAX_IDS!");
  return mUsedMem[id];
}

  void 
AMDILMachineFunctionInfo::addErrorMsg(const char *msg, ErrorMsgEnum val) 
{
  if (val == DEBUG_ONLY) {
#if defined(DEBUG) || defined(_DEBUG)
    mErrors.insert(msg);
#endif
  }  else if (val == RELEASE_ONLY) {
#if !defined(DEBUG) && !defined(_DEBUG)
    mErrors.insert(msg);
#endif
  } else if (val == ALWAYS) {
    mErrors.insert(msg);
  }
}

  uint32_t 
AMDILMachineFunctionInfo::addPrintfString(std::string &name, unsigned offset) 
{
  if (mPrintfMap.find(name) != mPrintfMap.end()) {
    return mPrintfMap[name]->getPrintfID();
  } else {
    PrintfInfo *info = new PrintfInfo;
    info->setPrintfID(mPrintfMap.size() + offset);
    mPrintfMap[name] = info;
    return info->getPrintfID();
  }
}

  void 
AMDILMachineFunctionInfo::addPrintfOperand(std::string &name, 
    size_t idx,
    uint32_t size) 
{
  mPrintfMap[name]->addOperand(idx, size);
}

  void 
AMDILMachineFunctionInfo::addMetadata(const char *md, bool kernelOnly) 
{
  addMetadata(std::string(md), kernelOnly);
}

  void 
AMDILMachineFunctionInfo::addMetadata(std::string md, bool kernelOnly) 
{
  if (kernelOnly) {
    mMetadataKernel.push_back(md);
  } else {
    mMetadataFunc.insert(md);
  }
}


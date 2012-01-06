//===------------- AMDILKernel.h - AMDIL Kernel Class ----------*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//
// Definition of a AMDILKernel object and the various subclasses that 
// are used.
//===----------------------------------------------------------------------===//
#ifndef _AMDIL_KERNEL_H_
#define _AMDIL_KERNEL_H_
#include "AMDIL.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/Constant.h"
#include "llvm/Value.h"

namespace llvm {
  class AMDILSubtarget;
  class AMDILTargetMachine;
  /// structure that holds information for a single local/region address array
  typedef struct _AMDILArrayMemRec {
    uint32_t vecSize; // size of each vector
    uint32_t offset;  // offset into the memory section
    bool isHW;        // flag to specify if HW is used or SW is used
    bool isRegion;    // flag to specify if GDS is used or not
  } AMDILArrayMem;

  /// structure that holds information about a constant address
  /// space pointer that is a kernel argument
  typedef struct _AMDILConstPtrRec {
    const llvm::Value *base;
    uint32_t size;
    uint32_t offset;
    uint32_t cbNum; // value of 0 means that it does not use hw CB
    bool isArray;
    bool isArgument;
    bool usesHardware;
    std::string name;
  } AMDILConstPtr;
 
  /// Structure that holds information for all local/region address
  /// arrays in the kernel
  typedef struct _AMDILLocalArgRec {
    llvm::SmallVector<AMDILArrayMem *, DEFAULT_VEC_SLOTS> local;
    std::string name; // Kernel Name
  } AMDILLocalArg;

  /// Structure that holds information for each kernel argument
  typedef struct _AMDILkernelArgRec {
    uint32_t reqGroupSize[3];
    uint32_t reqRegionSize[3];
    llvm::SmallVector<uint32_t, DEFAULT_VEC_SLOTS> argInfo;
    bool mHasRWG;
    bool mHasRWR;
  } AMDILKernelAttr;

  /// Structure that holds information for each kernel
  class AMDILKernel {
    public:
      AMDILKernel() {}
      uint32_t curSize;
      uint32_t curRSize;
      uint32_t curHWSize;
      uint32_t curHWRSize;
      uint32_t constSize;
      bool mKernel;
      std::string mName;
      AMDILKernelAttr *sgv;
      AMDILLocalArg *lvgv;
      llvm::SmallVector<struct _AMDILConstPtrRec, DEFAULT_VEC_SLOTS> constPtr;
      uint32_t constSizes[HW_MAX_NUM_CB];
      llvm::SmallSet<uint32_t, OPENCL_MAX_READ_IMAGES> readOnly;
      llvm::SmallSet<uint32_t, OPENCL_MAX_WRITE_IMAGES> writeOnly;
      llvm::SmallVector<std::pair<uint32_t, const llvm::Constant *>,
        DEFAULT_VEC_SLOTS> CPOffsets;
      typedef llvm::SmallVector<struct _AMDILConstPtrRec, DEFAULT_VEC_SLOTS>::iterator constptr_iterator;
      typedef llvm::SmallVector<AMDILArrayMem *, DEFAULT_VEC_SLOTS>::iterator arraymem_iterator;
  }; // AMDILKernel
} // end llvm namespace
#endif // _AMDIL_KERNEL_H_

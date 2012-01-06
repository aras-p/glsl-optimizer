//===-- AMDILGlobalManager.h - TODO: Add brief description -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
// ==-----------------------------------------------------------------------===//
//
// Class that handles parsing and storing global variables that are relevant to
// the compilation of the module.
//
// ==-----------------------------------------------------------------------===//

#ifndef _AMDILGLOBALMANAGER_H_
#define _AMDILGLOBALMANAGER_H_

#include "AMDIL.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Module.h"
#include "llvm/Support/raw_ostream.h"

#include <set>
#include <string>

#define CB_BASE_OFFSET 2

namespace llvm {

class PointerType;
class AMDILKernelManager;
class AMDILSubtarget;
class TypeSymbolTable;
class Argument;
class GlobalValue;
class MachineFunction;

/// structure that holds information for a single local/region address array
typedef struct _arrayMemRec {
  uint32_t vecSize; // size of each vector
  uint32_t offset;  // offset into the memory section
  bool isHW;        // flag to specify if HW is used or SW is used
  bool isRegion;    // flag to specify if GDS is used or not
} arraymem;
 
/// Structure that holds information for all local/region address
/// arrays in the kernel
typedef struct _localArgRec {
  llvm::SmallVector<arraymem *, DEFAULT_VEC_SLOTS> local;
  std::string name; // Kernel Name
} localArg;

/// structure that holds information about a constant address
/// space pointer that is a kernel argument
typedef struct _constPtrRec {
  const Value *base;
  uint32_t size;
  uint32_t offset;
  uint32_t cbNum; // value of 0 means that it does not use hw CB
  bool isArray;
  bool isArgument;
  bool usesHardware;
  std::string name;
} constPtr;

/// Structure that holds information for each kernel argument
typedef struct _kernelArgRec {
  uint32_t reqGroupSize[3];
  uint32_t reqRegionSize[3];
  llvm::SmallVector<uint32_t, DEFAULT_VEC_SLOTS> argInfo;
  bool mHasRWG;
  bool mHasRWR;
} kernelArg;

/// Structure that holds information for each kernel
typedef struct _kernelRec {
  mutable uint32_t curSize;
  mutable uint32_t curRSize;
  mutable uint32_t curHWSize;
  mutable uint32_t curHWRSize;
  uint32_t constSize;
  kernelArg *sgv;
  localArg *lvgv;
  llvm::SmallVector<struct _constPtrRec, DEFAULT_VEC_SLOTS> constPtr;
  uint32_t constSizes[HW_MAX_NUM_CB];
  llvm::SmallSet<uint32_t, OPENCL_MAX_READ_IMAGES> readOnly;
  llvm::SmallSet<uint32_t, OPENCL_MAX_WRITE_IMAGES> writeOnly;
  llvm::SmallVector<std::pair<uint32_t, const Constant *>,
    DEFAULT_VEC_SLOTS> CPOffsets;
} kernel;

class AMDILGlobalManager {
public:
  AMDILGlobalManager(bool debugMode = false);
  ~AMDILGlobalManager();

  /// Process the given module and parse out the global variable metadata passed
  /// down from the frontend-compiler
  void processModule(const Module &MF, const AMDILTargetMachine* mTM);

  /// Returns whether the current name is the name of a kernel function or a
  /// normal function
  bool isKernel(const llvm::StringRef &name) const;

  /// Returns true if the image ID corresponds to a read only image.
  bool isReadOnlyImage(const llvm::StringRef &name, uint32_t iID) const;

  /// Returns true if the image ID corresponds to a write only image.
  bool isWriteOnlyImage(const llvm::StringRef &name, uint32_t iID) const;

  /// Returns the number of write only images for the kernel.
  uint32_t getNumWriteImages(const llvm::StringRef &name) const;

  /// Gets the group size of the kernel for the given dimension.
  uint32_t getLocal(const llvm::StringRef &name, uint32_t dim) const;

  /// Gets the region size of the kernel for the given dimension.
  uint32_t getRegion(const llvm::StringRef &name, uint32_t dim) const;

  /// Get the Region memory size in 1d for the given function/kernel.
  uint32_t getRegionSize(const llvm::StringRef &name) const;

  /// Get the region memory size in 1d for the given function/kernel.
  uint32_t getLocalSize(const llvm::StringRef &name) const;

  // Get the max group size in one 1D for the given function/kernel.
  uint32_t getMaxGroupSize(const llvm::StringRef &name) const;

  // Get the max region size in one 1D for the given function/kernel.
  uint32_t getMaxRegionSize(const llvm::StringRef &name) const;

  /// Get the constant memory size in 1d for the given function/kernel.
  uint32_t getConstSize(const llvm::StringRef &name) const;

  /// Get the HW local size in 1d for the given function/kernel We need to
  /// seperate SW local and HW local for the case where some local memory is
  /// emulated in global and some is using the hardware features. The main
  /// problem is that in OpenCL 1.0/1.1 cl_khr_byte_addressable_store allows
  /// these actions to happen on all memory spaces, but the hardware can only
  /// write byte address stores to UAV and LDS, not GDS or Stack.
  uint32_t getHWLocalSize(const llvm::StringRef &name) const;
  uint32_t getHWRegionSize(const llvm::StringRef &name) const;

  /// Get the offset of the array for the kernel.
  int32_t getArrayOffset(const llvm::StringRef &name) const;

  /// Get the offset of the const memory for the kernel.
  int32_t getConstOffset(const llvm::StringRef &name) const;

  /// Get the boolean value if this particular constant uses HW or not.
  bool getConstHWBit(const llvm::StringRef &name) const;

  /// Get a reference to the kernel metadata information for the given function
  /// name.
  const kernel &getKernel(const llvm::StringRef &name) const;

  /// Returns whether a reqd_workgroup_size attribute has been used or not.
  bool hasRWG(const llvm::StringRef &name) const;

  /// Returns whether a reqd_workregion_size attribute has been used or not.
  bool hasRWR(const llvm::StringRef &name) const;


  /// Dump the data section to the output stream for the given kernel.
  void dumpDataSection(llvm::raw_ostream &O, AMDILKernelManager *km);

  /// Iterate through the constants that are global to the compilation unit.
  StringMap<constPtr>::iterator consts_begin();
  StringMap<constPtr>::iterator consts_end();

  /// Query if the kernel has a byte store.
  bool byteStoreExists(llvm::StringRef S) const;

  /// Query if the kernel and argument uses hardware constant memory.
  bool usesHWConstant(const kernel &krnl, const llvm::StringRef &arg);

  /// Query if the constant pointer is an argument.
  bool isConstPtrArgument(const kernel &krnl, const llvm::StringRef &arg);

  /// Query if the constant pointer is an array that is globally scoped.
  bool isConstPtrArray(const kernel &krnl, const llvm::StringRef &arg);

  /// Query the size of the constant pointer.
  uint32_t getConstPtrSize(const kernel &krnl, const llvm::StringRef &arg);

  /// Query the offset of the constant pointer.
  uint32_t getConstPtrOff(const kernel &krnl, const llvm::StringRef &arg);

  /// Query the constant buffer number for a constant pointer.
  uint32_t getConstPtrCB(const kernel &krnl, const llvm::StringRef &arg);

  /// Query the Value* that the constant pointer originates from.
  const Value *getConstPtrValue(const kernel &krnl, const llvm::StringRef &arg);

  /// Get the ID of the argument.
  int32_t getArgID(const Argument *arg);

  /// Get the unique function ID for the specific function name and create a new
  /// unique ID if it is not found.
  uint32_t getOrCreateFunctionID(const GlobalValue* func);
  uint32_t getOrCreateFunctionID(const std::string& func);

  /// Calculate the offsets of the constant pool for the given kernel and
  /// machine function.
  void calculateCPOffsets(const MachineFunction *MF, kernel &krnl);

  /// Print the global manager to the output stream.
  void print(llvm::raw_ostream& O);

  /// Dump the global manager to the output stream - debug use.
  void dump();

private:
  /// Various functions that parse global value information and store them in
  /// the global manager. This approach is used instead of dynamic parsing as it
  /// might require more space, but should allow caching of data that gets
  /// requested multiple times.
  kernelArg parseSGV(const GlobalValue *GV);
  localArg  parseLVGV(const GlobalValue *GV);
  void parseGlobalAnnotate(const GlobalValue *G);
  void parseImageAnnotate(const GlobalValue *G);
  void parseConstantPtrAnnotate(const GlobalValue *G);
  void printConstantValue(const Constant *CAval,
                          llvm::raw_ostream& O,
                          bool asByte);
  void parseKernelInformation(const Value *V);
  void parseAutoArray(const GlobalValue *G, bool isRegion);
  void parseConstantPtr(const GlobalValue *G);
  void allocateGlobalCB();
  void dumpDataToCB(llvm::raw_ostream &O, AMDILKernelManager *km, uint32_t id);
  bool checkConstPtrsUseHW(Module::const_iterator *F);

  llvm::StringMap<arraymem> mArrayMems;
  llvm::StringMap<localArg> mLocalArgs;
  llvm::StringMap<kernelArg> mKernelArgs;
  llvm::StringMap<kernel> mKernels;
  llvm::StringMap<constPtr> mConstMems;
  llvm::StringMap<uint32_t> mFuncNames;
  llvm::DenseMap<const GlobalValue*, uint32_t> mFuncPtrNames;
  llvm::DenseMap<uint32_t, llvm::StringRef> mImageNameMap;
  std::set<llvm::StringRef> mByteStore;
  std::set<llvm::StringRef> mIgnoreStr;
  llvm::DenseMap<const Argument *, int32_t> mArgIDMap;
  const char *symTab;
  const AMDILSubtarget *mSTM;
  size_t mOffset;
  uint32_t mReservedBuffs;
  uint32_t mCurrentCPOffset;
  bool mDebugMode;
};
} // namespace llvm
#endif // __AMDILGLOBALMANAGER_H_

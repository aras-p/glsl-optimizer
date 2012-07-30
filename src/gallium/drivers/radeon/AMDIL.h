//===-- AMDIL.h - Top-level interface for AMDIL representation --*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//
//
// This file contains the entry points for global functions defined in the LLVM
// AMDIL back-end.
//
//===----------------------------------------------------------------------===//

#ifndef AMDIL_H_
#define AMDIL_H_

#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/Target/TargetMachine.h"

#define AMDIL_MAJOR_VERSION 2
#define AMDIL_MINOR_VERSION 0
#define AMDIL_REVISION_NUMBER 74
#define ARENA_SEGMENT_RESERVED_UAVS 12
#define DEFAULT_ARENA_UAV_ID 8
#define DEFAULT_RAW_UAV_ID 7
#define GLOBAL_RETURN_RAW_UAV_ID 11
#define HW_MAX_NUM_CB 8
#define MAX_NUM_UNIQUE_UAVS 8
#define OPENCL_MAX_NUM_ATOMIC_COUNTERS 8
#define OPENCL_MAX_READ_IMAGES 128
#define OPENCL_MAX_WRITE_IMAGES 8
#define OPENCL_MAX_SAMPLERS 16

// The next two values can never be zero, as zero is the ID that is
// used to assert against.
#define DEFAULT_LDS_ID     1
#define DEFAULT_GDS_ID     1
#define DEFAULT_SCRATCH_ID 1
#define DEFAULT_VEC_SLOTS  8

// SC->CAL version matchings.
#define CAL_VERSION_SC_150               1700
#define CAL_VERSION_SC_149               1700
#define CAL_VERSION_SC_148               1525
#define CAL_VERSION_SC_147               1525
#define CAL_VERSION_SC_146               1525
#define CAL_VERSION_SC_145               1451
#define CAL_VERSION_SC_144               1451
#define CAL_VERSION_SC_143               1441
#define CAL_VERSION_SC_142               1441
#define CAL_VERSION_SC_141               1420
#define CAL_VERSION_SC_140               1400
#define CAL_VERSION_SC_139               1387
#define CAL_VERSION_SC_138               1387
#define CAL_APPEND_BUFFER_SUPPORT        1340
#define CAL_VERSION_SC_137               1331
#define CAL_VERSION_SC_136                982
#define CAL_VERSION_SC_135                950
#define CAL_VERSION_GLOBAL_RETURN_BUFFER  990

#define OCL_DEVICE_RV710        0x0001
#define OCL_DEVICE_RV730        0x0002
#define OCL_DEVICE_RV770        0x0004
#define OCL_DEVICE_CEDAR        0x0008
#define OCL_DEVICE_REDWOOD      0x0010
#define OCL_DEVICE_JUNIPER      0x0020
#define OCL_DEVICE_CYPRESS      0x0040
#define OCL_DEVICE_CAICOS       0x0080
#define OCL_DEVICE_TURKS        0x0100
#define OCL_DEVICE_BARTS        0x0200
#define OCL_DEVICE_CAYMAN       0x0400
#define OCL_DEVICE_ALL          0x3FFF

/// The number of function ID's that are reserved for 
/// internal compiler usage.
const unsigned int RESERVED_FUNCS = 1024;

#define AMDIL_OPT_LEVEL_DECL
#define  AMDIL_OPT_LEVEL_VAR
#define AMDIL_OPT_LEVEL_VAR_NO_COMMA

namespace llvm {
class AMDILInstrPrinter;
class FunctionPass;
class MCAsmInfo;
class raw_ostream;
class Target;
class TargetMachine;

/// Instruction selection passes.
FunctionPass*
  createAMDILISelDag(TargetMachine &TM AMDIL_OPT_LEVEL_DECL);
FunctionPass*
  createAMDILPeepholeOpt(TargetMachine &TM AMDIL_OPT_LEVEL_DECL);

/// Pre emit passes.
FunctionPass*
  createAMDILCFGPreparationPass(TargetMachine &TM AMDIL_OPT_LEVEL_DECL);
FunctionPass*
  createAMDILCFGStructurizerPass(TargetMachine &TM AMDIL_OPT_LEVEL_DECL);

extern Target TheAMDILTarget;
extern Target TheAMDGPUTarget;
} // end namespace llvm;

/// Include device information enumerations
#include "AMDILDeviceInfo.h"

namespace llvm {
/// OpenCL uses address spaces to differentiate between
/// various memory regions on the hardware. On the CPU
/// all of the address spaces point to the same memory,
/// however on the GPU, each address space points to
/// a seperate piece of memory that is unique from other
/// memory locations.
namespace AMDILAS {
enum AddressSpaces {
  PRIVATE_ADDRESS  = 0, // Address space for private memory.
  GLOBAL_ADDRESS   = 1, // Address space for global memory (RAT0, VTX0).
  CONSTANT_ADDRESS = 2, // Address space for constant memory.
  LOCAL_ADDRESS    = 3, // Address space for local memory.
  REGION_ADDRESS   = 4, // Address space for region memory.
  ADDRESS_NONE     = 5, // Address space for unknown memory.
  PARAM_D_ADDRESS  = 6, // Address space for direct addressible parameter memory (CONST0)
  PARAM_I_ADDRESS  = 7, // Address space for indirect addressible parameter memory (VTX1)
  USER_SGPR_ADDRESS = 8, // Address space for USER_SGPRS on SI
  LAST_ADDRESS     = 9
};

// This union/struct combination is an easy way to read out the
// exact bits that are needed.
typedef union ResourceRec {
  struct {
#ifdef __BIG_ENDIAN__
    unsigned short isImage       : 1;  // Reserved for future use/llvm.
    unsigned short ResourceID    : 10; // Flag to specify the resourece ID for
                                       // the op.
    unsigned short HardwareInst  : 1;  // Flag to specify that this instruction
                                       // is a hardware instruction.
    unsigned short ConflictPtr   : 1;  // Flag to specify that the pointer has a
                                       // conflict.
    unsigned short ByteStore     : 1;  // Flag to specify if the op is a byte
                                       // store op.
    unsigned short PointerPath   : 1;  // Flag to specify if the op is on the
                                       // pointer path.
    unsigned short CacheableRead : 1;  // Flag to specify if the read is
                                       // cacheable.
#else
    unsigned short CacheableRead : 1;  // Flag to specify if the read is
                                       // cacheable.
    unsigned short PointerPath   : 1;  // Flag to specify if the op is on the
                                       // pointer path.
    unsigned short ByteStore     : 1;  // Flag to specify if the op is byte
                                       // store op.
    unsigned short ConflictPtr   : 1;  // Flag to specify that the pointer has
                                       // a conflict.
    unsigned short HardwareInst  : 1;  // Flag to specify that this instruction
                                       // is a hardware instruction.
    unsigned short ResourceID    : 10; // Flag to specify the resource ID for
                                       // the op.
    unsigned short isImage       : 1;  // Reserved for future use.
#endif
  } bits;
  unsigned short u16all;
} InstrResEnc;

} // namespace AMDILAS

} // end namespace llvm
#endif // AMDIL_H_

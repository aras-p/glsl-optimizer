//===----------- AMDILIOExpansion.h - IO Expansion Pass -------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//
// The AMDIL IO Expansion class expands pseudo IO instructions into a sequence
// of instructions that produces the correct results. These instructions are
// not expanded earlier in the backend because any pass before this can assume to
// be able to generate a load/store instruction. So this pass can only have
// passes that execute after it if no load/store instructions can be generated
// in those passes.
//===----------------------------------------------------------------------===//
#ifndef _AMDILIOEXPANSION_H_
#define _AMDILIOEXPANSION_H_
#undef DEBUG_TYPE
#undef DEBUGME
#define DEBUG_TYPE "IOExpansion"
#if !defined(NDEBUG)
#define DEBUGME (DebugFlag && isCurrentDebugType(DEBUG_TYPE))
#else
#define DEBUGME (false)
#endif
#include "AMDIL.h"
#include "llvm/CodeGen/MachineFunctionAnalysis.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {
  class MachineFunction;
  class AMDILKernelManager;
  class AMDILMachineFunctionInfo;
  class AMDILSubtarget;
  class MachineInstr;
  class Constant;
  class TargetInstrInfo;
  class Type;
  typedef enum {
    NO_PACKING = 0,
    PACK_V2I8,
    PACK_V4I8,
    PACK_V2I16,
    PACK_V4I16,
    UNPACK_V2I8,
    UNPACK_V4I8,
    UNPACK_V2I16,
    UNPACK_V4I16,
    UNPACK_LAST
  } REG_PACKED_TYPE;
  class AMDILIOExpansion : public MachineFunctionPass
  {
    public:
      virtual ~AMDILIOExpansion();
      virtual const char* getPassName() const;
      bool runOnMachineFunction(MachineFunction &MF);
      static char ID;
    protected:
      AMDILIOExpansion(TargetMachine &tm AMDIL_OPT_LEVEL_DECL);
      TargetMachine &TM;
      //
      // @param MI Machine instruction to check.
      // @brief checks to see if the machine instruction
      // is an I/O instruction or not.
      //
      // @return true if I/O, false otherwise.
      //
      virtual bool
        isIOInstruction(MachineInstr *MI);
      // Wrapper function that calls the appropriate I/O 
      // expansion function based on the instruction type.
      virtual void
        expandIOInstruction(MachineInstr *MI);
       virtual void
        expandGlobalStore(MachineInstr *MI) = 0;
      virtual void
        expandLocalStore(MachineInstr *MI) = 0;
      virtual void
        expandRegionStore(MachineInstr *MI) = 0;
      virtual void
        expandPrivateStore(MachineInstr *MI) = 0;
      virtual void
        expandGlobalLoad(MachineInstr *MI) = 0;
      virtual void
        expandRegionLoad(MachineInstr *MI) = 0;
      virtual void
        expandLocalLoad(MachineInstr *MI) = 0;
      virtual void
        expandPrivateLoad(MachineInstr *MI) = 0;
      virtual void
        expandConstantLoad(MachineInstr *MI) = 0;
      virtual void
        expandConstantPoolLoad(MachineInstr *MI) = 0;
      bool
        isAddrCalcInstr(MachineInstr *MI);
      bool
        isExtendLoad(MachineInstr *MI);
      bool
        isHardwareRegion(MachineInstr *MI);
      bool
        isHardwareLocal(MachineInstr *MI);
      bool
        isPackedData(MachineInstr *MI);
      bool
        isStaticCPLoad(MachineInstr *MI);
      bool
        isNbitType(Type *MI, uint32_t nBits, bool isScalar = true);
      bool
        isHardwareInst(MachineInstr *MI);
      uint32_t
        getMemorySize(MachineInstr *MI);
      REG_PACKED_TYPE
        getPackedID(MachineInstr *MI);
      uint32_t
        getShiftSize(MachineInstr *MI);
      uint32_t
        getPointerID(MachineInstr *MI);
      void
        expandTruncData(MachineInstr *MI);
      void
        expandLoadStartCode(MachineInstr *MI);
      virtual void
        expandStoreSetupCode(MachineInstr *MI) = 0;
      void
        expandAddressCalc(MachineInstr *MI);
      void
        expandLongExtend(MachineInstr *MI, 
            uint32_t numComponents, uint32_t size, bool signedShift);
      void 
        expandLongExtendSub32(MachineInstr *MI, 
            unsigned SHLop, unsigned SHRop, unsigned USHRop, 
            unsigned SHLimm, uint64_t SHRimm, unsigned USHRimm, 
            unsigned LCRop, bool signedShift);
      void
        expandIntegerExtend(MachineInstr *MI, unsigned, unsigned, unsigned);
      void
        expandExtendLoad(MachineInstr *MI);
      virtual void
        expandPackedData(MachineInstr *MI) = 0;
       void
         emitCPInst(MachineInstr* MI, const Constant* C, 
             AMDILKernelManager* KM, int swizzle, bool ExtFPLoad);

      bool mDebug;
      const AMDILSubtarget *mSTM;
      AMDILKernelManager *mKM;
      MachineBasicBlock *mBB;
      AMDILMachineFunctionInfo *mMFI;
      const TargetInstrInfo *mTII;
      bool saveInst;
    private:
      void
        emitStaticCPLoad(MachineInstr* MI, int swizzle, int id,
            bool ExtFPLoad);
  }; // class AMDILIOExpansion

  // Intermediate class that holds I/O code expansion that is common to the
  // 7XX, Evergreen and Northern Island family of chips.
  class AMDIL789IOExpansion : public AMDILIOExpansion  {
    public:
      virtual ~AMDIL789IOExpansion();
      virtual const char* getPassName() const;
    protected:
      AMDIL789IOExpansion(TargetMachine &tm AMDIL_OPT_LEVEL_DECL);
       virtual void
        expandGlobalStore(MachineInstr *MI) = 0;
      virtual void
        expandLocalStore(MachineInstr *MI) = 0;
      virtual void
        expandRegionStore(MachineInstr *MI) = 0;
      virtual void
        expandGlobalLoad(MachineInstr *MI) = 0;
      virtual void
        expandRegionLoad(MachineInstr *MI) = 0;
      virtual void
        expandLocalLoad(MachineInstr *MI) = 0;
      virtual void
        expandPrivateStore(MachineInstr *MI);
      virtual void
        expandConstantLoad(MachineInstr *MI);
      virtual void
        expandPrivateLoad(MachineInstr *MI) ;
      virtual void
        expandConstantPoolLoad(MachineInstr *MI);
      void
        expandStoreSetupCode(MachineInstr *MI);
      virtual void
        expandPackedData(MachineInstr *MI);
    private:
      void emitVectorAddressCalc(MachineInstr *MI, bool is32bit, 
          bool needsSelect);
      void emitVectorSwitchWrite(MachineInstr *MI, bool is32bit);
      void emitComponentExtract(MachineInstr *MI, unsigned flag, unsigned src, 
          unsigned dst, bool beforeInst);
      void emitDataLoadSelect(MachineInstr *MI);
  }; // class AMDIL789IOExpansion
  // Class that handles I/O emission for the 7XX family of devices.
  class AMDIL7XXIOExpansion : public AMDIL789IOExpansion {
    public:
      AMDIL7XXIOExpansion(TargetMachine &tm AMDIL_OPT_LEVEL_DECL);

      ~AMDIL7XXIOExpansion();
      const char* getPassName() const;
    protected:
      void
        expandGlobalStore(MachineInstr *MI);
      void
        expandLocalStore(MachineInstr *MI);
      void
        expandRegionStore(MachineInstr *MI);
      void
        expandGlobalLoad(MachineInstr *MI);
      void
        expandRegionLoad(MachineInstr *MI);
      void
        expandLocalLoad(MachineInstr *MI);
  }; // class AMDIL7XXIOExpansion

  // Class that handles image functions to expand them into the
  // correct set of I/O instructions.
  class AMDILImageExpansion : public AMDIL789IOExpansion {
    public:
      AMDILImageExpansion(TargetMachine &tm AMDIL_OPT_LEVEL_DECL);

      virtual ~AMDILImageExpansion();
    protected:
      //
      // @param MI Instruction iterator that has the sample instruction
      // that needs to be taken care of.
      // @brief transforms the __amdil_sample_data function call into a
      // sample instruction in IL.
      //
      // @warning This function only works correctly if all functions get
      // inlined
      //
      virtual void
        expandImageLoad(MachineBasicBlock *BB, MachineInstr *MI);
      //
      // @param MI Instruction iterator that has the write instruction that
      // needs to be taken care of.
      // @brief transforms the __amdil_write_data function call into a
      // simple UAV write instruction in IL.
      //
      // @warning This function only works correctly if all functions get
      // inlined
      //
      virtual void
        expandImageStore(MachineBasicBlock *BB, MachineInstr *MI);
      //
      // @param MI Instruction interator that has the image parameter
      // instruction
      // @brief transforms the __amdil_get_image_params function call into
      // a copy of data from a specific constant buffer to the register
      //
      // @warning This function only works correctly if all functions get
      // inlined
      //
      virtual void
        expandImageParam(MachineBasicBlock *BB, MachineInstr *MI);

      //
      // @param MI Insturction that points to the image
      // @brief transforms __amdil_sample_data into a sequence of
      // if/else that selects the correct sample instruction.
      //
      // @warning This function is inefficient and works with no
      // inlining.
      //
      virtual void
        expandInefficientImageLoad(MachineBasicBlock *BB, MachineInstr *MI);
    private:
      AMDILImageExpansion(); // Do not implement.

  }; // class AMDILImageExpansion

  // Class that expands IO instructions for Evergreen and Northern
  // Island family of devices.
  class AMDILEGIOExpansion : public AMDILImageExpansion  {
    public:
      AMDILEGIOExpansion(TargetMachine &tm AMDIL_OPT_LEVEL_DECL);

      virtual ~AMDILEGIOExpansion();
      const char* getPassName() const;
    protected:
      virtual bool
        isIOInstruction(MachineInstr *MI);
      virtual void
        expandIOInstruction(MachineInstr *MI);
      bool
        isImageIO(MachineInstr *MI);
      virtual void
        expandGlobalStore(MachineInstr *MI);
      void
        expandLocalStore(MachineInstr *MI);
      void
        expandRegionStore(MachineInstr *MI);
      virtual void
        expandGlobalLoad(MachineInstr *MI);
      void
        expandRegionLoad(MachineInstr *MI);
      void
        expandLocalLoad(MachineInstr *MI);
      virtual bool
        isCacheableOp(MachineInstr *MI);
      void
        expandStoreSetupCode(MachineInstr *MI);
      void
        expandPackedData(MachineInstr *MI);
    private:
      bool
        isArenaOp(MachineInstr *MI);
      void
        expandArenaSetup(MachineInstr *MI);
  }; // class AMDILEGIOExpansion
} // namespace llvm
#endif // _AMDILIOEXPANSION_H_

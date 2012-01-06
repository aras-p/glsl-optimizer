//===-------- AMDILPointerManager.h - Manage Pointers for HW ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//
// The AMDIL Pointer Manager is a class that does all the checking for
// different pointer characteristics. Pointers have attributes that need
// to be attached to them in order to correctly codegen them efficiently.
// This class will analyze the pointers of a function and then traverse the uses
// of the pointers and determine if a pointer can be cached, should belong in
// the arena, and what UAV it should belong to. There are seperate classes for
// each unique generation of devices. This pass only works in SSA form.
//===----------------------------------------------------------------------===//
#ifndef _AMDIL_POINTER_MANAGER_H_
#define _AMDIL_POINTER_MANAGER_H_
#undef DEBUG_TYPE
#undef DEBUGME
#define DEBUG_TYPE "PointerManager"
#if !defined(NDEBUG)
#define DEBUGME (DebugFlag && isCurrentDebugType(DEBUG_TYPE))
#else
#define DEBUGME (false)
#endif
#include "AMDIL.h"
#include "AMDILUtilityFunctions.h"
#include "llvm/CodeGen/MachineFunctionAnalysis.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"
#include "llvm/Target/TargetMachine.h"

#include <list>
#include <map>
#include <queue>
#include <set>

namespace llvm {
  class Value;
  class MachineBasicBlock;
  // Typedefing the multiple different set types to that it is
  // easier to read what each set is supposed to handle. This
  // also allows it easier to track which set goes to which
  // argument in a function call.
  typedef std::set<const Value*> PtrSet;

  // A Byte set is the set of all base pointers that must
  // be allocated to the arena path.
  typedef PtrSet ByteSet;

  // A Raw set is the set of all base pointers that can be
  // allocated to the raw path.
  typedef PtrSet RawSet;

  // A cacheable set is the set of all base pointers that
  // are deamed cacheable based on annotations or
  // compiler options.
  typedef PtrSet CacheableSet;

  // A conflict set is a set of all base pointers whose 
  // use/def chains conflict with another base pointer.
  typedef PtrSet ConflictSet;

  // An image set is a set of all read/write only image pointers.
  typedef PtrSet ImageSet;

  // An append set is a set of atomic counter base pointers
  typedef std::vector<const Value*> AppendSet;

  // A ConstantSet is a set of constant pool instructions
  typedef std::set<MachineInstr*> CPoolSet;

  // A CacheableInstSet set is a set of instructions that are cachable
  // even if the pointer is not generally cacheable.
  typedef std::set<MachineInstr*> CacheableInstrSet;

  // A pair that maps a virtual register to the equivalent base
  // pointer value that it was derived from.
  typedef std::pair<unsigned, const Value*> RegValPair;

  // A map that maps between the base pointe rvalue and an array
  // of instructions that are part of the pointer chain. A pointer
  // chain is a recursive def/use chain of all instructions that don't
  // store data to memory unless the pointer is the data being stored.
  typedef std::map<const Value*, std::vector<MachineInstr*> > PtrIMap;

  // A map that holds a set of all base pointers that are used in a machine
  // instruction. This helps to detect when conflict pointers are found
  // such as when pointer subtraction occurs.
  typedef std::map<MachineInstr*, PtrSet> InstPMap;

  // A map that holds the frame index to RegValPair so that writes of 
  // pointers to the stack can be tracked.
  typedef std::map<unsigned, RegValPair > FIPMap;

  // A small vector impl that holds all of the register to base pointer 
  // mappings for a given function.
  typedef std::map<unsigned, RegValPair> RVPVec;



  // The default pointer manager. This handles pointer 
  // resource allocation for default ID's only. 
  // There is no special processing.
  class AMDILPointerManager : public MachineFunctionPass
  {
    public:
      AMDILPointerManager(
          TargetMachine &tm
          AMDIL_OPT_LEVEL_DECL);
      virtual ~AMDILPointerManager();
      virtual const char*
        getPassName() const;
      virtual bool
        runOnMachineFunction(MachineFunction &F);
      virtual void
        getAnalysisUsage(AnalysisUsage &AU) const;
      static char ID;
    protected:
      bool mDebug;
    private:
      TargetMachine &TM;
  }; // class AMDILPointerManager

  // The pointer manager for Evergreen and Northern Island
  // devices. This pointer manager allocates and trackes
  // cached memory, arena resources, raw resources and
  // whether multi-uav is utilized or not.
  class AMDILEGPointerManager : public AMDILPointerManager
  {
    public:
      AMDILEGPointerManager(
          TargetMachine &tm
          AMDIL_OPT_LEVEL_DECL);
      virtual ~AMDILEGPointerManager();
      virtual const char*
        getPassName() const;
      virtual bool
        runOnMachineFunction(MachineFunction &F);
    private:
      TargetMachine &TM;
  }; // class AMDILEGPointerManager

  // Information related to the cacheability of instructions in a basic block.
  // This is used during the parse phase of the pointer algorithm to track
  // the reachability of stores within a basic block.
  class BlockCacheableInfo {
    public:
      BlockCacheableInfo() :
        mStoreReachesTop(false),
        mStoreReachesExit(false),
        mCacheableSet()
    {};

      bool storeReachesTop() const  { return mStoreReachesTop; }
      bool storeReachesExit() const { return mStoreReachesExit; }
      CacheableInstrSet::const_iterator 
        cacheableBegin() const { return mCacheableSet.begin(); }
      CacheableInstrSet::const_iterator 
        cacheableEnd()   const { return mCacheableSet.end(); }

      // mark the block as having a global store that reaches it. This
      // will also set the store reaches exit flag, and clear the list
      // of loads (since they are now reachable by a store.)
      bool setReachesTop() {
        bool changedExit = !mStoreReachesExit;

        if (!mStoreReachesTop)
          mCacheableSet.clear();

        mStoreReachesTop = true;
        mStoreReachesExit = true;
        return changedExit;
      }

      // Mark the block as having a store that reaches the exit of the 
      // block.
      void setReachesExit() {
        mStoreReachesExit = true;
      }

      // If the top or the exit of the block are not marked as reachable
      // by a store, add the load to the list of cacheable loads.
      void addPossiblyCacheableInst(const TargetMachine * tm, MachineInstr *load) {
        // By definition, if store reaches top, then store reaches exit.
        // So, we only test for exit here.
        // If we have a volatile load we cannot cache it.
        if (mStoreReachesExit || isVolatileInst(tm->getInstrInfo(), load)) {
          return;
        }

        mCacheableSet.insert(load);
      }

    private:
      bool mStoreReachesTop; // Does a global store reach the top of this block?
      bool mStoreReachesExit;// Does a global store reach the exit of this block?
      CacheableInstrSet mCacheableSet; // The set of loads in the block not 
      // reachable by a global store.
  };
  // Map from MachineBasicBlock to it's cacheable load info.
  typedef std::map<MachineBasicBlock*, BlockCacheableInfo> MBBCacheableMap;
} // end llvm namespace
#endif // _AMDIL_POINTER_MANAGER_H_

//===-- AMDILMachinePeephole.cpp - AMDIL Machine Peephole Pass -*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//


#define DEBUG_TYPE "machine_peephole"
#if !defined(NDEBUG)
#define DEBUGME (DebugFlag && isCurrentDebugType(DEBUG_TYPE))
#else
#define DEBUGME (false)
#endif

#include "AMDIL.h"
#include "AMDILSubtarget.h"
#include "AMDILUtilityFunctions.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/Support/Debug.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;
namespace
{
  class AMDILMachinePeephole : public MachineFunctionPass
  {
    public:
      static char ID;
      AMDILMachinePeephole(TargetMachine &tm AMDIL_OPT_LEVEL_DECL);
      //virtual ~AMDILMachinePeephole();
      virtual const char*
        getPassName() const;
      virtual bool
        runOnMachineFunction(MachineFunction &MF);
    private:
      void insertFence(MachineBasicBlock::iterator &MIB);
      TargetMachine &TM;
      bool mDebug;
  }; // AMDILMachinePeephole
  char AMDILMachinePeephole::ID = 0;
} // anonymous namespace

namespace llvm
{
  FunctionPass*
    createAMDILMachinePeephole(TargetMachine &tm AMDIL_OPT_LEVEL_DECL)
    {
      return new AMDILMachinePeephole(tm AMDIL_OPT_LEVEL_VAR);
    }
} // llvm namespace

AMDILMachinePeephole::AMDILMachinePeephole(TargetMachine &tm AMDIL_OPT_LEVEL_DECL)
  : MachineFunctionPass(ID), TM(tm)
{
  mDebug = DEBUGME;
}

bool
AMDILMachinePeephole::runOnMachineFunction(MachineFunction &MF)
{
  bool Changed = false;
  const AMDILSubtarget *STM = &TM.getSubtarget<AMDILSubtarget>();
  for (MachineFunction::iterator MBB = MF.begin(), MBE = MF.end();
      MBB != MBE; ++MBB) {
    MachineBasicBlock *mb = MBB;
    for (MachineBasicBlock::iterator MIB = mb->begin(), MIE = mb->end();
        MIB != MIE; ++MIB) {
      MachineInstr *mi = MIB;
      const char * name;
      name = TM.getInstrInfo()->getName(mi->getOpcode());
      switch (mi->getOpcode()) {
        default:
          if (isAtomicInst(TM.getInstrInfo(), mi)) {
            // If we don't support the hardware accellerated address spaces,
            // then the atomic needs to be transformed to the global atomic.
            if (strstr(name, "_L_")
                && STM->device()->usesSoftware(AMDILDeviceInfo::LocalMem)) {
              BuildMI(*mb, MIB, mi->getDebugLoc(), 
                  TM.getInstrInfo()->get(AMDIL::ADD_i32), AMDIL::R1011)
                .addReg(mi->getOperand(1).getReg())
                .addReg(AMDIL::T2);
              mi->getOperand(1).setReg(AMDIL::R1011);
              mi->setDesc(
                  TM.getInstrInfo()->get(
                    (mi->getOpcode() - AMDIL::ATOM_L_ADD) + AMDIL::ATOM_G_ADD));
            } else if (strstr(name, "_R_")
                && STM->device()->usesSoftware(AMDILDeviceInfo::RegionMem)) {
              assert(!"Software region memory is not supported!");
              mi->setDesc(
                  TM.getInstrInfo()->get(
                    (mi->getOpcode() - AMDIL::ATOM_R_ADD) + AMDIL::ATOM_G_ADD));
            }
          } else if ((isLoadInst(TM.getInstrInfo(), mi) || isStoreInst(TM.getInstrInfo(), mi)) && isVolatileInst(TM.getInstrInfo(), mi)) {
            insertFence(MIB);
          }
          continue;
          break;
        case AMDIL::USHR_i16:
        case AMDIL::USHR_v2i16:
        case AMDIL::USHR_v4i16:
        case AMDIL::USHRVEC_i16:
        case AMDIL::USHRVEC_v2i16:
        case AMDIL::USHRVEC_v4i16:
          if (TM.getSubtarget<AMDILSubtarget>()
              .device()->usesSoftware(AMDILDeviceInfo::ShortOps)) {
            unsigned lReg = MF.getRegInfo()
              .createVirtualRegister(&AMDIL::GPRI32RegClass);
            unsigned Reg = MF.getRegInfo()
              .createVirtualRegister(&AMDIL::GPRV4I32RegClass);
            BuildMI(*mb, MIB, mi->getDebugLoc(),
                TM.getInstrInfo()->get(AMDIL::LOADCONST_i32),
                lReg).addImm(0xFFFF);
            BuildMI(*mb, MIB, mi->getDebugLoc(),
                TM.getInstrInfo()->get(AMDIL::BINARY_AND_v4i32),
                Reg)
              .addReg(mi->getOperand(1).getReg())
              .addReg(lReg);
            mi->getOperand(1).setReg(Reg);
          }
          break;
        case AMDIL::USHR_i8:
        case AMDIL::USHR_v2i8:
        case AMDIL::USHR_v4i8:
        case AMDIL::USHRVEC_i8:
        case AMDIL::USHRVEC_v2i8:
        case AMDIL::USHRVEC_v4i8:
          if (TM.getSubtarget<AMDILSubtarget>()
              .device()->usesSoftware(AMDILDeviceInfo::ByteOps)) {
            unsigned lReg = MF.getRegInfo()
              .createVirtualRegister(&AMDIL::GPRI32RegClass);
            unsigned Reg = MF.getRegInfo()
              .createVirtualRegister(&AMDIL::GPRV4I32RegClass);
            BuildMI(*mb, MIB, mi->getDebugLoc(),
                TM.getInstrInfo()->get(AMDIL::LOADCONST_i32),
                lReg).addImm(0xFF);
            BuildMI(*mb, MIB, mi->getDebugLoc(),
                TM.getInstrInfo()->get(AMDIL::BINARY_AND_v4i32),
                Reg)
              .addReg(mi->getOperand(1).getReg())
              .addReg(lReg);
            mi->getOperand(1).setReg(Reg);
          }
          break;
      }
    }
  }
  return Changed;
}

const char*
AMDILMachinePeephole::getPassName() const
{
  return "AMDIL Generic Machine Peephole Optimization Pass";
}

void
AMDILMachinePeephole::insertFence(MachineBasicBlock::iterator &MIB)
{
  MachineInstr *MI = MIB;
  MachineInstr *fence = BuildMI(*(MI->getParent()->getParent()),
        MI->getDebugLoc(),
        TM.getInstrInfo()->get(AMDIL::FENCE)).addReg(1);

  MI->getParent()->insert(MIB, fence);
  fence = BuildMI(*(MI->getParent()->getParent()),
        MI->getDebugLoc(),
        TM.getInstrInfo()->get(AMDIL::FENCE)).addReg(1);
  MIB = MI->getParent()->insertAfter(MIB, fence);
}

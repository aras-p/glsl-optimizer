//===-- R600LowerInstructions.cpp - TODO: Add brief description -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// TODO: Add full description
//
//===----------------------------------------------------------------------===//

#include "AMDGPU.h"
#include "AMDGPUInstrInfo.h"
#include "AMDGPUUtil.h"
#include "AMDIL.h"
#include "AMDILRegisterInfo.h"
#include "R600InstrInfo.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/Constants.h"
#include "llvm/Target/TargetInstrInfo.h"

#include <stdio.h>

using namespace llvm;

namespace {
  class R600LowerInstructionsPass : public MachineFunctionPass {

  private:
    static char ID;
    TargetMachine &TM;
    const R600InstrInfo * TII;
    MachineRegisterInfo * MRI;

    void lowerFLT(MachineInstr &MI);

    void calcAddress(const MachineOperand &ptrOp,
                     const MachineOperand &indexOp,
                     unsigned indexReg,
                     MachineBasicBlock &MBB,
                     MachineBasicBlock::iterator I) const;

    void divMod(MachineInstr &MI,
                  MachineBasicBlock &MBB,
                  MachineBasicBlock::iterator I,
                  bool div = true) const;

  public:
    R600LowerInstructionsPass(TargetMachine &tm) :
      MachineFunctionPass(ID), TM(tm),
      TII(static_cast<const R600InstrInfo*>(tm.getInstrInfo())),
      MRI(NULL)
      { }

    const char *getPassName() const { return "R600 Lower Instructions"; }
    virtual bool runOnMachineFunction(MachineFunction &MF);

  };
} /* End anonymous namespace */

char R600LowerInstructionsPass::ID = 0;

FunctionPass *llvm::createR600LowerInstructionsPass(TargetMachine &tm) {
  return new R600LowerInstructionsPass(tm);
}

bool R600LowerInstructionsPass::runOnMachineFunction(MachineFunction &MF)
{
  MRI = &MF.getRegInfo();

  for (MachineFunction::iterator BB = MF.begin(), BB_E = MF.end();
                                                  BB != BB_E; ++BB) {
    MachineBasicBlock &MBB = *BB;
    for (MachineBasicBlock::iterator I = MBB.begin(), Next = llvm::next(I);
         I != MBB.end(); I = Next, Next = llvm::next(I) ) {

      MachineInstr &MI = *I;
      switch(MI.getOpcode()) {
      case AMDIL::FLT:
        BuildMI(MBB, I, MBB.findDebugLoc(I), TM.getInstrInfo()->get(AMDIL::FGE))
                .addOperand(MI.getOperand(0))
                .addOperand(MI.getOperand(2))
                .addOperand(MI.getOperand(1));
        break;

      case AMDIL::ABS_i32:
        {
          unsigned setgt = MRI->createVirtualRegister(
                           &AMDIL::R600_TReg32RegClass);
          BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(AMDIL::SETGE_INT),
                  setgt)
                  .addReg(AMDIL::ZERO)
                  .addOperand(MI.getOperand(1));

          unsigned add_int = MRI->createVirtualRegister(
                             &AMDIL::R600_TReg32RegClass);
          BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(AMDIL::ADD_INT),
                  add_int)
                  .addReg(setgt)
                  .addOperand(MI.getOperand(1));

          BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(AMDIL::XOR_INT))
                  .addOperand(MI.getOperand(0))
                  .addReg(setgt)
                  .addReg(add_int);

          break;
        }

      /* XXX: We could propagate the ABS flag to all of the uses of Operand0 and
       * remove the ABS instruction.*/
      case AMDIL::FABS_f32:
      case AMDIL::ABS_f32:
        MI.getOperand(1).addTargetFlag(MO_FLAG_ABS);
        BuildMI(MBB, I, MBB.findDebugLoc(I), TM.getInstrInfo()->get(AMDIL::MOVE_f32))
                .addOperand(MI.getOperand(0))
                .addOperand(MI.getOperand(1));
        break;

      case AMDIL::BINARY_OR_f32:
        {
        unsigned tmp0 = MRI->createVirtualRegister(&AMDIL::GPRI32RegClass);
        BuildMI(MBB, I, MBB.findDebugLoc(I), TM.getInstrInfo()->get(AMDIL::FTOI), tmp0)
                .addOperand(MI.getOperand(1));
        unsigned tmp1 = MRI->createVirtualRegister(&AMDIL::GPRI32RegClass);
        BuildMI(MBB, I, MBB.findDebugLoc(I), TM.getInstrInfo()->get(AMDIL::FTOI), tmp1)
                .addOperand(MI.getOperand(2));
        unsigned tmp2 = MRI->createVirtualRegister(&AMDIL::GPRI32RegClass);
        BuildMI(MBB, I, MBB.findDebugLoc(I), TM.getInstrInfo()->get(AMDIL::BINARY_OR_i32), tmp2)
                .addReg(tmp0)
                .addReg(tmp1);
        BuildMI(MBB, I, MBB.findDebugLoc(I), TM.getInstrInfo()->get(AMDIL::ITOF), MI.getOperand(0).getReg())
                .addReg(tmp2);
        break;
        }
      case AMDIL::CMOVLOG_f32:
        BuildMI(MBB, I, MBB.findDebugLoc(I), TM.getInstrInfo()->get(MI.getOpcode()))
                .addOperand(MI.getOperand(0))
                .addOperand(MI.getOperand(1))
                .addOperand(MI.getOperand(3))
                .addOperand(MI.getOperand(2));
        break;

      case AMDIL::CMOVLOG_i32:
        BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(AMDIL::CNDE_INT))
                .addOperand(MI.getOperand(0))
                .addOperand(MI.getOperand(1))
                .addOperand(MI.getOperand(3))
                .addOperand(MI.getOperand(2));
        break;

      case AMDIL::CLAMP_f32:
        {
          MachineOperand lowOp = MI.getOperand(2);
          MachineOperand highOp = MI.getOperand(3);
        if (lowOp.isReg() && highOp.isReg()
            && lowOp.getReg() == AMDIL::ZERO && highOp.getReg() == AMDIL::ONE) {
          MI.getOperand(0).addTargetFlag(MO_FLAG_CLAMP);
          BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(AMDIL::MOV))
                  .addOperand(MI.getOperand(0))
                  .addOperand(MI.getOperand(1));
        } else {
          /* XXX: Handle other cases */
          abort();
        }
        break;
        }

      case AMDIL::UDIV_i32:
        divMod(MI, MBB, I);
        break;

      /* XXX: Figure out the semantics of DIV_INF_f32 and make sure this is OK */
/*      case AMDIL::DIV_INF_f32:
        {
          unsigned tmp0 = MRI->createVirtualRegister(&AMDIL::GPRF32RegClass);
          BuildMI(MBB, I, MBB.findDebugLoc(I),
                          TM.getInstrInfo()->get(AMDIL::RECIP_CLAMPED), tmp0)
                  .addOperand(MI.getOperand(2));
          BuildMI(MBB, I, MBB.findDebugLoc(I),
                          TM.getInstrInfo()->get(AMDIL::MUL_IEEE_f32))
                  .addOperand(MI.getOperand(0))
                  .addReg(tmp0)
                  .addOperand(MI.getOperand(1));
          break;
        }
*/        /* XXX: This is an optimization */

      case AMDIL::GLOBALLOAD_f32:
      case AMDIL::GLOBALLOAD_i32:
        {
          MachineOperand &ptrOperand = MI.getOperand(1);
          MachineOperand &indexOperand = MI.getOperand(2);
          unsigned indexReg =
                   MRI->createVirtualRegister(&AMDIL::R600_TReg32_XRegClass);

          /* Calculate the address with in the VTX buffer */
          calcAddress(ptrOperand, indexOperand, indexReg, MBB, I);

          /* Make sure the VTX_READ_eg writes to the X chan */
          MRI->setRegClass(MI.getOperand(0).getReg(),
                          &AMDIL::R600_TReg32_XRegClass);

          /* Add the VTX_READ_eg instruction */
          BuildMI(MBB, I, MBB.findDebugLoc(I),
                          TII->get(AMDIL::VTX_READ_eg))
                  .addOperand(MI.getOperand(0))
                  .addReg(indexReg)
                  .addImm(1);
          break;
        }

      case AMDIL::GLOBALSTORE_i32:
      case AMDIL::GLOBALSTORE_f32:
        {
          MachineOperand &ptrOperand = MI.getOperand(1);
          MachineOperand &indexOperand = MI.getOperand(2);
          unsigned rwReg =
                   MRI->createVirtualRegister(&AMDIL::R600_TReg32_XRegClass);
          unsigned byteIndexReg =
                   MRI->createVirtualRegister(&AMDIL::R600_TReg32RegClass);
          unsigned shiftReg =
                   MRI->createVirtualRegister(&AMDIL::R600_TReg32RegClass);
          unsigned indexReg =
                   MRI->createVirtualRegister(&AMDIL::R600_TReg32_XRegClass);

          /* Move the store value to the correct register class */
          BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(AMDIL::COPY), rwReg)
                  .addOperand(MI.getOperand(0));

          /* Calculate the address in the RAT */
          calcAddress(ptrOperand, indexOperand, byteIndexReg, MBB, I);


          BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(AMDIL::MOV), shiftReg)
                  .addReg(AMDIL::ALU_LITERAL_X)
                  .addImm(2);

          /* XXX: Check GPU family */
          BuildMI(MBB, I, MBB.findDebugLoc(I),
                          TII->get(AMDIL::LSHR_eg), indexReg)
                 .addReg(byteIndexReg)
                 .addReg(shiftReg);

          /* XXX: Check GPU Family */
          BuildMI(MBB, I, MBB.findDebugLoc(I),
                          TII->get(AMDIL::RAT_WRITE_CACHELESS_eg))
                  .addReg(rwReg)
                  .addReg(indexReg)
                  .addImm(0);
          break;
        }
      case AMDIL::ILT:
        BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(AMDIL::SETGT_INT))
                .addOperand(MI.getOperand(0))
                .addOperand(MI.getOperand(2))
                .addOperand(MI.getOperand(1));
        break;
      case AMDIL::LOADCONST_f32:
      case AMDIL::LOADCONST_i32:
        {
          bool canInline = false;
          unsigned inlineReg;
          MachineOperand & dstOp = MI.getOperand(0);
          MachineOperand & immOp = MI.getOperand(1);
          if (immOp.isFPImm()) {
            const ConstantFP * cfp = immOp.getFPImm();
            if (cfp->isZero()) {
              canInline = true;
              inlineReg = AMDIL::ZERO;
            } else if (cfp->isExactlyValue(1.0f)) {
              canInline = true;
              inlineReg = AMDIL::ONE;
            } else if (cfp->isExactlyValue(0.5f)) {
              canInline = true;
              inlineReg = AMDIL::HALF;
            }
          }

          if (canInline) {
            MachineOperand * use = dstOp.getNextOperandForReg();
            /* The lowering operation for CLAMP needs to have the immediates
             * as operands, so we must propagate them. */
            while (use) {
              MachineOperand * next = use->getNextOperandForReg();
              if (use->getParent()->getOpcode() == AMDIL::CLAMP_f32) {
                use->setReg(inlineReg);
              }
              use = next;
            }
            BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(AMDIL::COPY))
                    .addOperand(dstOp)
                    .addReg(inlineReg);
          } else {
            BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(AMDIL::MOV))
                    .addOperand(dstOp)
                    .addReg(AMDIL::ALU_LITERAL_X)
                    .addOperand(immOp);
          }
          break;
        }

      case AMDIL::MASK_WRITE:
      {
        unsigned maskedRegister = MI.getOperand(0).getReg();
        assert(TargetRegisterInfo::isVirtualRegister(maskedRegister));
        MachineInstr * defInstr = MRI->getVRegDef(maskedRegister);
        MachineOperand * def = defInstr->findRegisterDefOperand(maskedRegister);
        def->addTargetFlag(MO_FLAG_MASK);
        /* Continue so the instruction is not erased */
        continue;
      }

      case AMDIL::NEGATE_i32:
        BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(AMDIL::SUB_INT))
                .addOperand(MI.getOperand(0))
                .addReg(AMDIL::ZERO)
                .addOperand(MI.getOperand(1));
        break;

      case AMDIL::NEG_f32:
        {
            MI.getOperand(1).addTargetFlag(MO_FLAG_NEG);
            BuildMI(MBB, I, MBB.findDebugLoc(I),
                    TII->get(TII->getISAOpcode(AMDIL::MOV)))
            .addOperand(MI.getOperand(0))
            .addOperand(MI.getOperand(1));
          break;
        }

      case AMDIL::SUB_f32:
        {
          MI.getOperand(2).addTargetFlag(MO_FLAG_NEG);
          BuildMI(MBB, I, MBB.findDebugLoc(I),
                          TII->get(TII->getISAOpcode(AMDIL::ADD_f32)))
                  .addOperand(MI.getOperand(0))
                  .addOperand(MI.getOperand(1))
                  .addOperand(MI.getOperand(2));
          break;
        }

      case AMDIL::ULT:
        BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(AMDIL::SETGT_UINT))
                .addOperand(MI.getOperand(0))
                .addOperand(MI.getOperand(2))
                .addOperand(MI.getOperand(1));
        break;

      default:
        continue;
      }
      MI.eraseFromParent();
    }
  }
  return false;
}

void R600LowerInstructionsPass::calcAddress(const MachineOperand &ptrOp,
                                            const MachineOperand &indexOp,
                                            unsigned indexReg,
                                            MachineBasicBlock &MBB,
                                            MachineBasicBlock::iterator I) const
{
  /* Optimize the case where the indexOperand is 0 */
  if (indexOp.isImm() && indexOp.getImm() == 0) {
    assert(ptrOp.isReg());
    BuildMI(MBB, I, MBB.findDebugLoc(I),
                    TII->get(AMDIL::COPY), indexReg)
            .addOperand(ptrOp);
  } else {
    BuildMI(MBB, I, MBB.findDebugLoc(I),
                    TII->get(AMDIL::ADD_INT), indexReg)
            .addOperand(indexOp)
            .addOperand(ptrOp);
  }
}

/* Mostly copied from tgsi_divmod() in r600_shader.c */
void R600LowerInstructionsPass::divMod(MachineInstr &MI,
                                       MachineBasicBlock &MBB,
                                       MachineBasicBlock::iterator I,
                                       bool div) const
{
  unsigned dst = MI.getOperand(0).getReg();
  MachineOperand &numerator = MI.getOperand(1);
  MachineOperand &denominator = MI.getOperand(2);
  /* rcp = RECIP(denominator) = 2^32 / denominator + e
   * e is rounding error */
  unsigned rcp = MRI->createVirtualRegister(&AMDIL::R600_TReg32RegClass);
  BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(TII->getRECIP_UINT()), rcp)
          .addOperand(denominator);

  /* rcp_lo = lo(rcp * denominator) */
  unsigned rcp_lo = MRI->createVirtualRegister(&AMDIL::R600_TReg32RegClass);
  BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(TII->getMULLO_UINT()), rcp_lo)
          .addReg(rcp)
          .addOperand(denominator);

  /* rcp_hi = HI (rcp * denominator) */
  unsigned rcp_hi = MRI->createVirtualRegister(&AMDIL::R600_TReg32RegClass);
  BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(TII->getMULHI_UINT()), rcp_hi)
          .addReg(rcp)
          .addOperand(denominator);

  unsigned neg_rcp_lo = MRI->createVirtualRegister(&AMDIL::R600_TReg32RegClass);
  BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(AMDIL::SUB_INT), neg_rcp_lo)
          .addReg(AMDIL::ZERO)
          .addReg(rcp_lo);

  unsigned abs_rcp_lo = MRI->createVirtualRegister(&AMDIL::R600_TReg32RegClass);
  BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(AMDIL::CNDE_INT), abs_rcp_lo)
          .addReg(rcp_hi)
          .addReg(neg_rcp_lo)
          .addReg(rcp_lo);

  unsigned e = MRI->createVirtualRegister(&AMDIL::R600_TReg32RegClass);
  BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(TII->getMULHI_UINT()), e)
          .addReg(abs_rcp_lo)
          .addReg(rcp);

  unsigned rcp_plus_e = MRI->createVirtualRegister(&AMDIL::R600_TReg32RegClass);
  BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(AMDIL::ADD_INT), rcp_plus_e)
          .addReg(rcp)
          .addReg(e);

  unsigned rcp_sub_e = MRI->createVirtualRegister(&AMDIL::R600_TReg32RegClass);
  BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(AMDIL::SUB_INT), rcp_sub_e)
          .addReg(rcp)
          .addReg(e);

  /* tmp0 = rcp_hi == 0 ? rcp_plus_e : rcp_sub_e */
  unsigned tmp0 = MRI->createVirtualRegister(&AMDIL::R600_TReg32RegClass);
  BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(AMDIL::CNDE_INT), tmp0)
          .addReg(rcp_hi)
          .addReg(rcp_plus_e)
          .addReg(rcp_sub_e);

  unsigned q = MRI->createVirtualRegister(&AMDIL::R600_TReg32RegClass);
  BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(TII->getMULHI_UINT()), q)
          .addReg(tmp0)
          .addOperand(numerator);

  /* num_sub_r = q * denominator */
  unsigned num_sub_r = MRI->createVirtualRegister(&AMDIL::R600_TReg32RegClass);
  BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(TII->getMULLO_UINT()),
          num_sub_r)
          .addReg(q)
          .addOperand(denominator);

  unsigned r = MRI->createVirtualRegister(&AMDIL::R600_TReg32RegClass);
  BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(AMDIL::SUB_INT), r)
          .addOperand(numerator)
          .addReg(num_sub_r);

  unsigned r_ge_den = MRI->createVirtualRegister(&AMDIL::R600_TReg32RegClass);
  BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(AMDIL::SETGE_INT), r_ge_den)
          .addReg(r)
          .addOperand(denominator);

  unsigned r_ge_zero = MRI->createVirtualRegister(&AMDIL::R600_TReg32RegClass);
  BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(AMDIL::SETGE_INT), r_ge_zero)
          .addOperand(numerator)
          .addReg(num_sub_r);

  unsigned tmp1 = MRI->createVirtualRegister(&AMDIL::R600_TReg32RegClass);
  BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(AMDIL::AND_INT), tmp1)
          .addReg(r_ge_den)
          .addReg(r_ge_zero);

  unsigned val0 = MRI->createVirtualRegister(&AMDIL::R600_TReg32RegClass);
  unsigned val1 = MRI->createVirtualRegister(&AMDIL::R600_TReg32RegClass);
  unsigned result = MRI->createVirtualRegister(&AMDIL::R600_TReg32RegClass);
  if (div) {
    BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(AMDIL::ADD_INT), val0)
            .addReg(q)
            .addReg(AMDIL::ONE_INT);

    BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(AMDIL::SUB_INT), val1)
            .addReg(q)
            .addReg(AMDIL::ONE_INT);

    BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(AMDIL::CNDE_INT), result)
            .addReg(tmp1)
            .addReg(q)
            .addReg(val0);
  } else {
    BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(AMDIL::SUB_INT), val0)
            .addReg(r)
            .addOperand(denominator);

    BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(AMDIL::ADD_INT), val1)
            .addReg(r)
            .addOperand(denominator);

    BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(AMDIL::CNDE_INT), result)
            .addReg(tmp1)
            .addReg(r)
            .addReg(val0);
  }

  /* XXX: Do we need to set to MAX_INT if denominator is 0? */
  BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(AMDIL::CNDE_INT), dst)
          .addReg(r_ge_zero)
          .addReg(val1)
          .addReg(result);
}

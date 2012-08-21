
#include "AMDGPUMCInstLower.h"
#include "AMDGPUAsmPrinter.h"
#include "R600InstrInfo.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/Constants.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

AMDGPUMCInstLower::AMDGPUMCInstLower() { }

void AMDGPUMCInstLower::Lower(const MachineInstr *MI, MCInst &OutMI) const {
  OutMI.setOpcode(MI->getOpcode());

  for (unsigned i = 0, e = MI->getNumExplicitOperands(); i != e; ++i) {
    const MachineOperand &MO = MI->getOperand(i);

    MCOperand MCOp;
    switch (MO.getType()) {
    default:
      llvm_unreachable("unknown operand type");
    case MachineOperand::MO_FPImmediate: {
      const APFloat &FloatValue = MO.getFPImm()->getValueAPF();
      assert(&FloatValue.getSemantics() == &APFloat::IEEEsingle &&
             "Only floating point immediates are supported at the moment.");
      MCOp = MCOperand::CreateFPImm(FloatValue.convertToFloat());
      break;
    }
    case MachineOperand::MO_Immediate:
      MCOp = MCOperand::CreateImm(MO.getImm());
      break;
    case MachineOperand::MO_Register:
      MCOp = MCOperand::CreateReg(MO.getReg());
      break;
    }
    OutMI.addOperand(MCOp);
  }
}

void AMDGPUAsmPrinter::EmitInstruction(const MachineInstr *MI) {
  AMDGPUMCInstLower MCInstLowering;

  // Ignore placeholder instructions:
  if (MI->getOpcode() == AMDGPU::MASK_WRITE) {
    return;
  }

  if (MI->isBundle()) {
    const MachineBasicBlock *MBB = MI->getParent();
    MachineBasicBlock::const_instr_iterator I = MI;
    ++I;
    while (I != MBB->end() && I->isInsideBundle()) {
      MCInst MCBundleInst;
      const MachineInstr *BundledInst = I;
      MCInstLowering.Lower(BundledInst, MCBundleInst);
      OutStreamer.EmitInstruction(MCBundleInst);
      ++I;
    }
  } else {
    MCInst TmpInst;
    MCInstLowering.Lower(MI, TmpInst);
    OutStreamer.EmitInstruction(TmpInst);
  }
}

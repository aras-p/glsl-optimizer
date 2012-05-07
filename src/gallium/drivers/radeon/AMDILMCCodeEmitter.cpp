//===---- AMDILMCCodeEmitter.cpp - Convert AMDIL text to AMDIL binary ----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//
//
//===---------------------------------------------------------------------===//

#include "AMDIL.h"
#include "AMDILInstrInfo.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
#if 0
namespace {
  class AMDILMCCodeEmitter : public MCCodeEmitter {
    AMDILMCCodeEmitter(const AMDILMCCodeEmitter &);// DO NOT IMPLEMENT
    void operator=(const AMDILMCCodeEmitter &); // DO NOT IMPLEMENT
    const TargetMachine &TM;
    const TargetInstrInfo &TII;
    MCContext &Ctx;
    bool Is64BitMode;
    public:
    AMDILMCCodeEmitter(TargetMachine &tm, MCContext &ctx, bool is64Bit);
    ~AMDILMCCodeEmitter();
    unsigned getNumFixupKinds() const;
    const MCFixupKindInfo& getFixupKindInfo(MCFixupKind Kind) const;
    static unsigned GetAMDILRegNum(const MCOperand &MO);
    void EmitByte(unsigned char C, unsigned &CurByte, raw_ostream &OS) const;
    void EmitConstant(uint64_t Val, unsigned Size, unsigned &CurByte,
        raw_ostream &OS) const;
    void EmitImmediate(const MCOperand &Disp, unsigned ImmSize,
        MCFixupKind FixupKind, unsigned &CurByte, raw_ostream &os,
        SmallVectorImpl<MCFixup> &Fixups, int ImmOffset = 0) const;

    void EncodeInstruction(const MCInst &MI, raw_ostream &OS,
        SmallVectorImpl<MCFixup> &Fixups) const;

  }; // class AMDILMCCodeEmitter
}; // anonymous namespace

namespace llvm {
  MCCodeEmitter *createAMDILMCCodeEmitter(const Target &,
      TargetMachine &TM, MCContext &Ctx)
  {
    return new AMDILMCCodeEmitter(TM, Ctx, false);
  }
}

AMDILMCCodeEmitter::AMDILMCCodeEmitter(TargetMachine &tm, MCContext &ctx
    , bool is64Bit)
: TM(tm), TII(*TM.getInstrInfo()), Ctx(ctx)
{
  Is64BitMode = is64Bit;
}

AMDILMCCodeEmitter::~AMDILMCCodeEmitter()
{
}

unsigned
AMDILMCCodeEmitter::getNumFixupKinds() const
{
  return 0;
}

const MCFixupKindInfo &
AMDILMCCodeEmitter::getFixupKindInfo(MCFixupKind Kind) const
{
//  const static MCFixupKindInfo Infos[] = {};
  if (Kind < FirstTargetFixupKind) {
    return MCCodeEmitter::getFixupKindInfo(Kind);
  }
  assert(unsigned(Kind - FirstTargetFixupKind) < getNumFixupKinds() &&
      "Invalid kind!");
  return MCCodeEmitter::getFixupKindInfo(Kind);
 // return Infos[Kind - FirstTargetFixupKind];

}

void
AMDILMCCodeEmitter::EmitByte(unsigned char C, unsigned &CurByte,
    raw_ostream &OS) const
{
  OS << (char) C;
  ++CurByte;
}
void
AMDILMCCodeEmitter::EmitConstant(uint64_t Val, unsigned Size, unsigned &CurByte,
    raw_ostream &OS) const
{
  // Output the constant in little endian byte order
  for (unsigned i = 0; i != Size; ++i) {
    EmitByte(Val & 255, CurByte, OS);
    Val >>= 8;
  }
}
void
AMDILMCCodeEmitter::EmitImmediate(const MCOperand &DispOp, unsigned ImmSize,
    MCFixupKind FixupKind, unsigned &CurByte, raw_ostream &OS,
    SmallVectorImpl<MCFixup> &Fixups, int ImmOffset) const
{
  // If this is a simple integer displacement that doesn't require a relocation
  // emit it now.
  if (DispOp.isImm()) {
    EmitConstant(DispOp.getImm() + ImmOffset, ImmSize, CurByte, OS);
  }

  // If we have an immoffset, add it to the expression
  const MCExpr *Expr = DispOp.getExpr();

  if (ImmOffset) {
    Expr = MCBinaryExpr::CreateAdd(Expr,
        MCConstantExpr::Create(ImmOffset, Ctx), Ctx);
  }
  // Emit a symbolic constant as a fixup and 4 zeros.
  Fixups.push_back(MCFixup::Create(CurByte, Expr, FixupKind));
  // TODO: Why the 4 zeros?
  EmitConstant(0, ImmSize, CurByte, OS);
}

void
AMDILMCCodeEmitter::EncodeInstruction(const MCInst &MI, raw_ostream &OS,
    SmallVectorImpl<MCFixup> &Fixups) const
{
#if 0
  unsigned Opcode = MI.getOpcode();
  const TargetInstrDesc &Desc = TII.get(Opcode);
  unsigned TSFlags = Desc.TSFlags;

  // Keep track of the current byte being emitted.
  unsigned CurByte = 0;

  unsigned NumOps = Desc.getNumOperands();
  unsigned CurOp = 0;

  unsigned char BaseOpcode = 0;
#ifndef NDEBUG
  // FIXME: Verify.
  if (// !Desc.isVariadic() &&
      CurOp != NumOps) {
    errs() << "Cannot encode all operands of: ";
    MI.dump();
    errs() << '\n';
    abort();
  }
#endif
#endif
}
#endif

/*
 * Copyright 2011 Christoph Bumiller
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "nv50_ir.h"
#include "nv50_ir_target.h"

namespace nv50_ir {

class CodeEmitterNV50 : public CodeEmitter
{
public:
   CodeEmitterNV50(const Target *);

   virtual bool emitInstruction(Instruction *);

   virtual uint32_t getMinEncodingSize(const Instruction *) const;

   inline void setProgramType(Program::Type pType) { progType = pType; }

private:
   const Target *targ;

   Program::Type progType;

private:
   inline void defId(const ValueDef&, const int pos);
   inline void srcId(const ValueRef&, const int pos);
   inline void srcId(const ValueRef *, const int pos);

   inline void srcAddr16(const ValueRef&, const int pos);
   inline void srcAddr8(const ValueRef&, const int pos);

   void emitFlagsRd(const Instruction *);
   void emitFlagsWr(const Instruction *);

   void emitCondCode(CondCode cc, int pos);

   inline void setARegBits(unsigned int);

   void setAReg16(const Instruction *, int s);
   void setImmediate(const Instruction *, int s);

   void setDst(const Value *);
   void setDst(const Instruction *, int d);
   void emitSrc0(const ValueRef&);
   void emitSrc1(const ValueRef&);
   void emitSrc2(const ValueRef&);

   void emitForm_MAD(const Instruction *);
   void emitForm_ADD(const Instruction *);
   void emitForm_MUL(const Instruction *);
   void emitForm_IMM(const Instruction *);

   void emitLoadStoreSize(DataType ty, int pos);

   void roundMode_MAD(const Instruction *);
   void roundMode_CVT(RoundMode);

   void emitMNeg12(const Instruction *);

   void emitLOAD(const Instruction *);
   void emitSTORE(const Instruction *);
   void emitMOV(const Instruction *);
   void emitNOP();
   void emitINTERP(const Instruction *);
   void emitPFETCH(const Instruction *);
   void emitOUT(const Instruction *);

   void emitUADD(const Instruction *);
   void emitAADD(const Instruction *);
   void emitFADD(const Instruction *);
   void emitUMUL(const Instruction *);
   void emitFMUL(const Instruction *);
   void emitFMAD(const Instruction *);

   void emitMINMAX(const Instruction *);

   void emitPreOp(const Instruction *);
   void emitSFnOp(const Instruction *, uint8_t subOp);

   void emitShift(const Instruction *);
   void emitARL(const Instruction *);
   void emitLogicOp(const Instruction *);

   void emitCVT(const Instruction *);
   void emitSET(const Instruction *);

   void emitTEX(const TexInstruction *);

   void emitQUADOP(const Instruction *, uint8_t lane, uint8_t quOp);

   void emitFlow(const Instruction *, uint8_t flowOp);
};

#define SDATA(a) ((a).rep()->reg.data)
#define DDATA(a) ((a).rep()->reg.data)

void CodeEmitterNV50::srcId(const ValueRef& src, const int pos)
{
   assert(src.get());
   code[pos / 32] |= SDATA(src).id << (pos % 32);
}

void CodeEmitterNV50::srcId(const ValueRef *src, const int pos)
{
   assert(src->get());
   code[pos / 32] |= SDATA(*src).id << (pos % 32);
}

void CodeEmitterNV50::srcAddr16(const ValueRef& src, const int pos)
{
   assert(src.get());

   uint32_t offset = SDATA(src).offset;

   assert(offset <= 0xffff && (pos % 32) <= 16);

   code[pos / 32] |= offset << (pos % 32);
}

void CodeEmitterNV50::srcAddr8(const ValueRef& src, const int pos)
{
   assert(src.get());

   uint32_t offset = SDATA(src).offset;

   assert(offset <= 0x1fc && !(offset & 0x3));

   code[pos / 32] |= (offset >> 2) << (pos % 32);
}

void CodeEmitterNV50::defId(const ValueDef& def, const int pos)
{
   assert(def.get());
   code[pos / 32] |= DDATA(def).id << (pos % 32);
}

void
CodeEmitterNV50::roundMode_MAD(const Instruction *insn)
{
   switch (insn->rnd) {
   case ROUND_M: code[1] |= 1 << 22; break;
   case ROUND_P: code[1] |= 2 << 22; break;
   case ROUND_Z: code[1] |= 3 << 22; break;
   default:
      assert(insn->rnd == ROUND_N);
      break;
   }
}

void
CodeEmitterNV50::emitMNeg12(const Instruction *i)
{
   code[1] |= i->src[0].mod.neg() << 26;
   code[1] |= i->src[1].mod.neg() << 27;
}

void CodeEmitterNV50::emitCondCode(CondCode cc, int pos)
{
   uint8_t enc;

   assert(pos >= 32 || pos <= 27);

   switch (cc) {
   case CC_LT:  enc = 0x1; break;
   case CC_LTU: enc = 0x9; break;
   case CC_EQ:  enc = 0x2; break;
   case CC_EQU: enc = 0xa; break;
   case CC_LE:  enc = 0x3; break;
   case CC_LEU: enc = 0xb; break;
   case CC_GT:  enc = 0x4; break;
   case CC_GTU: enc = 0xc; break;
   case CC_NE:  enc = 0x5; break;
   case CC_NEU: enc = 0xd; break;
   case CC_GE:  enc = 0x6; break;
   case CC_GEU: enc = 0xe; break;
   case CC_TR:  enc = 0xf; break;
   case CC_FL:  enc = 0x0; break;

   case CC_O:  enc = 0x10; break;
   case CC_C:  enc = 0x11; break;
   case CC_A:  enc = 0x12; break;
   case CC_S:  enc = 0x13; break;
   case CC_NS: enc = 0x1c; break;
   case CC_NA: enc = 0x1d; break;
   case CC_NC: enc = 0x1e; break;
   case CC_NO: enc = 0x1f; break;

   default:
      enc = 0;
      assert(!"invalid condition code");
      break;
   }
   code[pos / 32] |= enc << (pos % 32);
}

void
CodeEmitterNV50::emitFlagsRd(const Instruction *i)
{
   int s = (i->flagsSrc >= 0) ? i->flagsSrc : i->predSrc;

   assert(!(code[1] & 0x00003f80));

   if (s >= 0) {
      assert(i->getSrc(s)->reg.file == FILE_FLAGS);
      emitCondCode(i->cc, 32 + 7);
      srcId(i->src[s], 32 + 12);
   } else {
      code[1] |= 0x0780;
   }
}

void
CodeEmitterNV50::emitFlagsWr(const Instruction *i)
{
   assert(!(code[1] & 0x70));

   if (i->flagsDef >= 0)
      code[1] |= (DDATA(i->def[i->flagsDef]).id << 4) | 0x40;
}

void
CodeEmitterNV50::setARegBits(unsigned int u)
{
   code[0] |= (u & 3) << 26;
   code[1] |= (u & 4);
}

void
CodeEmitterNV50::setAReg16(const Instruction *i, int s)
{
   s = i->src[s].indirect[0];
   if (s >= 0)
      setARegBits(SDATA(i->src[s]).id + 1);
}

void
CodeEmitterNV50::setImmediate(const Instruction *i, int s)
{
   const ImmediateValue *imm = i->src[s].get()->asImm();
   assert(imm);

   code[1] |= 3;
   code[0] |= (imm->reg.data.u32 & 0x3f) << 16;
   code[1] |= (imm->reg.data.u32 >> 6) << 2;
}

void
CodeEmitterNV50::setDst(const Value *dst)
{
   const Storage *reg = &dst->join->reg;

   assert(reg->file != FILE_ADDRESS);

   if (reg->data.id < 0) {
      code[0] |= (127 << 2) | 1;
      code[1] |= 8;
   } else {
      if (reg->file == FILE_SHADER_OUTPUT)
         code[1] |= 8;
      code[0] |= reg->data.id << 2;
   }
}

void
CodeEmitterNV50::setDst(const Instruction *i, int d)
{
   if (i->defExists(d)) {
      setDst(i->getDef(d));
   } else
   if (!d) {
      code[0] |= 0x01fc; // bit bucket
      code[1] |= 0x0008;
   }
}

void
CodeEmitterNV50::emitSrc0(const ValueRef& ref)
{
   const Storage *reg = &ref.rep()->reg;

   if (reg->file == FILE_SHADER_INPUT)
      code[1] |= 0x00200000;
   else
   if (reg->file != FILE_GPR)
      ERROR("invalid src0 register file: %d\n", reg->file);

   assert(reg->data.id < 128);
   code[0] |= reg->data.id << 9;
}

void
CodeEmitterNV50::emitSrc1(const ValueRef& ref)
{
   const Storage *reg = &ref.rep()->reg;

   if (reg->file == FILE_MEMORY_CONST) {
      assert(!(code[1] & 0x01800000));
      code[0] |= 1 << 23;
      code[1] |= reg->fileIndex << 22;
   } else
   if (reg->file != FILE_GPR) {
      ERROR("invalid src1 register file: %d\n", reg->file);
   }

   assert(reg->data.id < 128);
   code[0] |= reg->data.id << 16;
}

void
CodeEmitterNV50::emitSrc2(const ValueRef& ref)
{
   const Storage *reg = &ref.rep()->reg;

   if (reg->file == FILE_MEMORY_CONST) {
      assert(!(code[1] & 0x01800000));
      code[0] |= 1 << 24;
      code[1] |= reg->fileIndex << 22;
   } else
   if (reg->file != FILE_GPR) {
      ERROR("invalid src1 register file: %d\n", reg->file);
   }

   assert(reg->data.id < 128);
   code[1] |= reg->data.id << 14;
}

// the default form:
//  - long instruction
//  - 1 to 3 sources in slots 0, 1, 2
//  - address & flags
void
CodeEmitterNV50::emitForm_MAD(const Instruction *i)
{
   assert(i->encSize == 8);
   code[0] |= 1;

   emitFlagsRd(i);
   emitFlagsWr(i);

   setDst(i, 0);

   if (i->srcExists(0))
      emitSrc0(i->src[0]);

   if (i->srcExists(1))
      emitSrc1(i->src[1]);

   if (i->srcExists(2))
      emitSrc2(i->src[2]);

   setAReg16(i, 1);
}

// like default form, but 2nd source in slot 2, and no 3rd source
void
CodeEmitterNV50::emitForm_ADD(const Instruction *i)
{
   assert(i->encSize == 8);
   code[0] |= 1;

   emitFlagsRd(i);
   emitFlagsWr(i);

   setDst(i, 0);

   if (i->srcExists(0))
      emitSrc0(i->src[0]);

   if (i->srcExists(1))
      emitSrc2(i->src[1]);

   setAReg16(i, 1);
}

// default short form
void
CodeEmitterNV50::emitForm_MUL(const Instruction *i)
{
   assert(i->encSize == 4 && !(code[0] & 1));
   assert(i->defExists(0));
   assert(!i->getPredicate());

   setDst(i, 0);

   if (i->srcExists(0))
      emitSrc0(i->src[0]);

   if (i->srcExists(1))
      emitSrc1(i->src[1]);
}

// usual immediate form
// - 1 to 3 sources where last is immediate
// - no address or predicate possible
void
CodeEmitterNV50::emitForm_IMM(const Instruction *i)
{
   assert(i->encSize == 8);
   code[0] |= 1;

   assert(i->defExists(0) && i->srcExists(0));

   setDst(i, 0);

   if (i->srcExists(2)) {
      emitSrc0(i->src[0]);
      emitSrc1(i->src[1]);
      setImmediate(i, 2);
   } else
   if (i->srcExists(1)) {
      emitSrc0(i->src[0]);
      setImmediate(i, 1);
   } else {
      setImmediate(i, 0);
   }
}

void
CodeEmitterNV50::emitLoadStoreSize(DataType ty, int pos)
{
   uint8_t enc;

   switch (ty) {
   case TYPE_F32: // fall through
   case TYPE_S32: // fall through
   case TYPE_U32:  enc = 0x6; break;
   case TYPE_B128: enc = 0x5; break;
   case TYPE_F64:  enc = 0x4; break;
   case TYPE_S16:  enc = 0x3; break;
   case TYPE_U16:  enc = 0x2; break;
   case TYPE_S8:   enc = 0x1; break;
   case TYPE_U8:   enc = 0x0; break;
   default:
      enc = 0;
      assert(!"invalid load/store type");
      break;
   }
   code[pos / 32] |= enc << (pos % 32);
}

void
CodeEmitterNV50::emitLOAD(const Instruction *i)
{
   DataFile sf = i->src[0].getFile();

   switch (sf) {
   case FILE_SHADER_INPUT:
      code[0] = 0x10000001;
      code[1] = 0x04200000 | (i->lanes << 14);
      break;
   case FILE_MEMORY_CONST:
      code[0] = 0x10000001;
      code[1] = 0x24000000 | (i->getSrc(0)->reg.fileIndex << 22);
      break;
   case FILE_MEMORY_LOCAL:
      code[0] = 0xd0000001;
      code[1] = 0x40000000;
      break;
   case FILE_MEMORY_GLOBAL:
      code[0] = 0xd0000001 | (i->getSrc(0)->reg.fileIndex << 16);
      code[1] = 0x80000000;
      break;
   default:
      assert(!"invalid load source file");
      break;
   }
   if (sf == FILE_MEMORY_LOCAL ||
       sf == FILE_MEMORY_GLOBAL)
      emitLoadStoreSize(i->sType, 21 + 32);

   setDst(i, 0);

   emitFlagsRd(i);
   emitFlagsWr(i);

   if (i->src[0].getFile() == FILE_MEMORY_GLOBAL) {
      srcId(*i->src[0].getIndirect(0), 9);
   } else {
      setAReg16(i, 0);
      srcAddr16(i->src[0], 9);
   }
}

void
CodeEmitterNV50::emitSTORE(const Instruction *i)
{
   DataFile f = i->getSrc(0)->reg.file;
   int32_t offset = i->getSrc(0)->reg.data.offset;

   switch (f) {
   case FILE_SHADER_OUTPUT:
      code[0] = 0x00000001 | ((offset >> 2) << 2);
      code[1] = 0x80c00000;
      srcId(i->src[1], 32 + 15);
      break;
   case FILE_MEMORY_GLOBAL:
      code[0] = 0xd0000000;
      code[1] = 0xa0000000;
      emitLoadStoreSize(i->dType, 21 + 32);
      break;
   case FILE_MEMORY_LOCAL:
      code[0] = 0xd0000001;
      code[1] = 0x60000000;
      emitLoadStoreSize(i->dType, 21 + 32);
      break;
   case FILE_MEMORY_SHARED:
      code[0] = 0x00000001;
      code[1] = 0xe0000000;
      switch (typeSizeof(i->dType)) {
      case 1:
         code[0] |= offset << 9;
         code[1] |= 0x00400000;
         break;
      case 2:
         code[0] |= (offset >> 1) << 9;
         break;
      case 4:
         code[0] |= (offset >> 2) << 9;
         code[1] |= 0x04000000;
         break;
      default:
         assert(0);
         break;
      }
      break;
   default:
      assert(!"invalid store destination file");
      break;
   }

   if (f != FILE_SHADER_OUTPUT) {
      srcId(i->src[1], 2);
      if (f == FILE_MEMORY_GLOBAL)
         srcId(*i->src[0].getIndirect(0), 9);
      if (f == FILE_MEMORY_LOCAL)
         srcAddr16(i->src[0], 9);
   }
   if (f != FILE_MEMORY_GLOBAL)
      setAReg16(i, 0);

   emitFlagsRd(i);
}

void
CodeEmitterNV50::emitMOV(const Instruction *i)
{
   DataFile sf = i->getSrc(0)->reg.file;
   DataFile df = i->getDef(0)->reg.file;

   assert(sf == FILE_GPR || df == FILE_GPR);

   if (sf == FILE_FLAGS) {
      code[0] = 0x00000001;
      code[1] = 0x20000000;
      defId(i->def[0], 2);
      srcId(i->src[0], 12);
      emitFlagsRd(i);
   } else
   if (sf == FILE_ADDRESS) {
      code[0] = 0x00000001;
      code[1] = 0x40000000;
      defId(i->def[0], 2);
      setARegBits(SDATA(i->src[0]).id + 1);
   } else
   if (df == FILE_FLAGS) {
      code[0] = 0x00000001;
      code[1] = 0xa0000000;
      defId(i->def[0], 4);
      srcId(i->src[0], 9);
      emitFlagsRd(i);
   } else
   if (sf == FILE_IMMEDIATE) {
      code[0] = 0x10008001;
      code[1] = 0x00000003;
      emitForm_IMM(i);
   } else {
      if (i->encSize == 4) {
         code[0] = 0x10008000;
      } else {
         code[0] = 0x10000001;
         code[1] = 0x04000000 | (i->lanes << 14);
      }
      defId(i->def[0], 2);
      srcId(i->src[0], 9);
   }
   if (df == FILE_SHADER_OUTPUT) {
      assert(i->encSize == 8);
      code[1] |= 0x8;
   }
}

void
CodeEmitterNV50::emitNOP()
{
   code[0] = 0xf0000001;
   code[1] = 0xe0000000;
}

void
CodeEmitterNV50::emitQUADOP(const Instruction *i, uint8_t lane, uint8_t quOp)
{
   code[0] = 0xc0000000 | (lane << 16);
   code[1] = 0x80000000;

   code[0] |= (quOp & 0x03) << 20;
   code[1] |= (quOp & 0xfc) << 20;

   emitForm_ADD(i);

   if (!i->srcExists(1))
      srcId(i->src[0], 32 + 14);
}

void
CodeEmitterNV50::emitPFETCH(const Instruction *i)
{
   code[0] = 0x11800001;
   code[1] = 0x04200000 | (0xf << 14);

   defId(i->def[0], 2);
   srcAddr8(i->src[0], 9);
   setAReg16(i, 0);
}

void
CodeEmitterNV50::emitINTERP(const Instruction *i)
{
   code[0] = 0x80000000;

   defId(i->def[0], 2);
   srcAddr8(i->src[0], 16);

   if (i->getInterpMode() == NV50_IR_INTERP_FLAT) {
      code[0] |= 1 << 8;
   } else {
      if (i->op == OP_PINTERP) {
         code[0] |= 1 << 25;
         srcId(i->src[1], 9);
      }
      if (i->getSampleMode() == NV50_IR_INTERP_CENTROID)
         code[0] |= 1 << 24;
   }

   if (i->encSize == 8) {
      emitFlagsRd(i);
      code[1] |=
         (code[0] & (3 << 24)) >> (24 - 16) |
         (code[0] & (1 <<  8)) >> (18 -  8);
      code[0] &= ~0x03000100;
      code[0] |= 1;
   }
}

void
CodeEmitterNV50::emitMINMAX(const Instruction *i)
{
   if (i->dType == TYPE_F64) {
      code[0] = 0xe0000000;
      code[1] = (i->op == OP_MIN) ? 0xa0000000 : 0xc0000000;
   } else {
      code[0] = 0x30000000;
      code[1] = 0x80000000;
      if (i->op == OP_MIN)
         code[1] |= 0x20000000;

      switch (i->dType) {
      case TYPE_F32: code[0] |= 0x80000000; break;
      case TYPE_S32: code[1] |= 0x8c000000; break;
      case TYPE_U32: code[1] |= 0x84000000; break;
      case TYPE_S16: code[1] |= 0x80000000; break;
      case TYPE_U16: break;
      default:
         assert(0);
         break;
      }
      code[1] |= i->src[0].mod.abs() << 20;
      code[1] |= i->src[1].mod.abs() << 19;
   }
   emitForm_MAD(i);
}

void
CodeEmitterNV50::emitFMAD(const Instruction *i)
{
   const int neg_mul = i->src[0].mod.neg() ^ i->src[1].mod.neg();
   const int neg_add = i->src[2].mod.neg();

   code[0] = 0xe0000000;

   if (i->encSize == 4) {
      emitForm_MUL(i);
      assert(!neg_mul && !neg_add);
   } else {
      emitForm_MAD(i);
      code[1] |= neg_mul << 26;
      code[1] |= neg_add << 27;
      if (i->saturate)
         code[1] |= 1 << 29;
   }
}

void
CodeEmitterNV50::emitFADD(const Instruction *i)
{
   const int neg0 = i->src[0].mod.neg();
   const int neg1 = i->src[1].mod.neg() ^ ((i->op == OP_SUB) ? 1 : 0);

   code[0] = 0xb0000000;

   assert(!(i->src[0].mod | i->src[1].mod).abs());

   if (i->src[1].getFile() == FILE_IMMEDIATE) {
      emitForm_IMM(i);
      code[0] |= neg0 << 15;
      code[0] |= neg1 << 22;
   } else
   if (i->encSize == 8) {
      emitForm_ADD(i);
      code[1] |= neg0 << 26;
      code[1] |= neg1 << 27;
      if (i->saturate)
         code[1] |= 1 << 29;
   } else {
      emitForm_MUL(i);
      code[0] |= neg0 << 15;
      code[0] |= neg1 << 22;
   }
}

void
CodeEmitterNV50::emitUADD(const Instruction *i)
{
   code[0] = 0x20008000;

   if (i->src[0].getFile() == FILE_IMMEDIATE) {
      emitForm_IMM(i);
   } else
   if (i->encSize == 8) {
      code[0] = 0x20000000;
      code[1] = 0x04000000;
      emitForm_ADD(i);
   } else {
      emitForm_MUL(i);
   }
   assert(!(i->src[0].mod.neg() && i->src[1].mod.neg()));
   code[0] |= i->src[0].mod.neg() << 28;
   code[0] |= i->src[1].mod.neg() << 22;
}

void
CodeEmitterNV50::emitAADD(const Instruction *i)
{
   const int s = (i->op == OP_MOV) ? 0 : 1;

   code[0] = 0xd0000001 | (i->getSrc(s)->reg.data.u16 << 9);
   code[1] = 0x20000000;

   code[0] |= (DDATA(i->def[0]).id + 1) << 2;

   emitFlagsRd(i);

   if (s && i->srcExists(0))
      setARegBits(SDATA(i->src[0]).id + 1);
}

void
CodeEmitterNV50::emitFMUL(const Instruction *i)
{
   const int neg = (i->src[0].mod ^ i->src[1].mod).neg();

   code[0] = 0xc0000000;

   if (i->src[0].getFile() == FILE_IMMEDIATE) {
      emitForm_IMM(i);
      if (neg)
         code[0] |= 0x8000;
   } else
   if (i->encSize == 8) {
      emitForm_MAD(i);
      if (neg)
         code[1] |= 0x08000000;
   } else {
      emitForm_MUL(i);
      if (neg)
         code[0] |= 0x8000;
   }
}

void
CodeEmitterNV50::emitSET(const Instruction *i)
{
   code[0] = 0x30000000;
   code[1] = 0x60000000;

   emitCondCode(i->asCmp()->setCond, 32 + 14);

   switch (i->sType) {
   case TYPE_F32: code[0] |= 0x80000000; break;
   case TYPE_S32: code[1] |= 0x0c000000; break;
   case TYPE_U32: code[1] |= 0x04000000; break;
   case TYPE_S16: code[1] |= 0x08000000; break;
   case TYPE_U16: break;
   default:
      assert(0);
      break;
   }
   emitForm_MAD(i);
}

void
CodeEmitterNV50::roundMode_CVT(RoundMode rnd)
{
   switch (rnd) {
   case ROUND_NI: code[1] |= 0x08000000; break;
   case ROUND_M:  code[1] |= 0x00020000; break;
   case ROUND_MI: code[1] |= 0x08020000; break;
   case ROUND_P:  code[1] |= 0x00040000; break;
   case ROUND_PI: code[1] |= 0x08040000; break;
   case ROUND_Z:  code[1] |= 0x00060000; break;
   case ROUND_ZI: code[1] |= 0x08060000; break;
   default:
      assert(rnd == ROUND_N);
      break;
   }
}

void
CodeEmitterNV50::emitCVT(const Instruction *i)
{
   const bool f2f = isFloatType(i->dType) && isFloatType(i->sType);
   RoundMode rnd;

   switch (i->op) {
   case OP_CEIL:  rnd = f2f ? ROUND_PI : ROUND_P; break;
   case OP_FLOOR: rnd = f2f ? ROUND_MI : ROUND_M; break;
   case OP_TRUNC: rnd = f2f ? ROUND_ZI : ROUND_Z; break;
   default:
      rnd = i->rnd;
      break;
   }

   code[0] = 0xa0000000;

   switch (i->dType) {
   case TYPE_F64:
      switch (i->sType) {
      case TYPE_F64: code[1] = 0xc4404000; break;
      case TYPE_S64: code[1] = 0x44414000; break;
      case TYPE_U64: code[1] = 0x44404000; break;
      case TYPE_F32: code[1] = 0xc4400000; break;
      case TYPE_S32: code[1] = 0x44410000; break;
      case TYPE_U32: code[1] = 0x44400000; break;
      default:
         assert(0);
         break;
      }
      break;
   case TYPE_S64:
      switch (i->sType) {
      case TYPE_F64: code[1] = 0x8c404000; break;
      case TYPE_F32: code[1] = 0x8c400000; break;
      default:
         assert(0);
         break;
      }
      break;
   case TYPE_U64:
      switch (i->sType) {
      case TYPE_F64: code[1] = 0x84404000; break;
      case TYPE_F32: code[1] = 0x84400000; break;
      default:
         assert(0);
         break;
      }
      break;
   case TYPE_F32:
      switch (i->sType) {
      case TYPE_F64: code[1] = 0xc0404000; break;
      case TYPE_S64: code[1] = 0x40414000; break;
      case TYPE_U64: code[1] = 0x40404000; break;
      case TYPE_F32: code[1] = 0xc4004000; break;
      case TYPE_S32: code[1] = 0x44014000; break;
      case TYPE_U32: code[1] = 0x44004000; break;
      case TYPE_F16: code[1] = 0xc4000000; break;
      default:
         assert(0);
         break;
      }
      break;
   case TYPE_S32:
      switch (i->sType) {
      case TYPE_F64: code[1] = 0x88404000; break;
      case TYPE_F32: code[1] = 0x8c004000; break;
      case TYPE_S32: code[1] = 0x0c014000; break;
      case TYPE_U32: code[1] = 0x0c004000; break;
      case TYPE_F16: code[1] = 0x8c000000; break;
      case TYPE_S16: code[1] = 0x0c010000; break;
      case TYPE_U16: code[1] = 0x0c000000; break;
      case TYPE_S8:  code[1] = 0x0c018000; break;
      case TYPE_U8:  code[1] = 0x0c008000; break;
      default:
         assert(0);
         break;
      }
      break;
   case TYPE_U32:
      switch (i->sType) {
      case TYPE_F64: code[1] = 0x80404000; break;
      case TYPE_F32: code[1] = 0x84004000; break;
      case TYPE_S32: code[1] = 0x04014000; break;
      case TYPE_U32: code[1] = 0x04004000; break;
      case TYPE_F16: code[1] = 0x84000000; break;
      case TYPE_S16: code[1] = 0x04010000; break;
      case TYPE_U16: code[1] = 0x04000000; break;
      case TYPE_S8:  code[1] = 0x04018000; break;
      case TYPE_U8:  code[1] = 0x04008000; break;
      default:
         assert(0);
         break;
      }
   case TYPE_S16:
   case TYPE_U16:
   case TYPE_S8:
   case TYPE_U8:
   default:
      assert(0);
      break;
   }
   if (typeSizeof(i->sType) == 1 && i->getSrc(0)->reg.size == 4)
      code[1] |= 0x00004000;

   roundMode_CVT(rnd);

   switch (i->op) {
   case OP_ABS: code[1] |= 1 << 20; break;
   case OP_SAT: code[1] |= 1 << 19; break;
   case OP_NEG: code[1] |= 1 << 29; break;
   default:
      break;
   }
   code[1] ^= i->src[0].mod.neg() << 29;
   code[1] |= i->src[0].mod.abs() << 20;
   if (i->saturate)
      code[1] |= 1 << 19;

   assert(i->op != OP_ABS || !i->src[0].mod.neg());

   emitForm_MAD(i);
}

void
CodeEmitterNV50::emitPreOp(const Instruction *i)
{
   code[0] = 0xb0000000;
   code[1] = (i->op == OP_PREEX2) ? 0xc0004000 : 0xc0000000;

   code[1] |= i->src[0].mod.abs() << 20;
   code[1] |= i->src[0].mod.neg() << 26;

   emitForm_MAD(i);
}

void
CodeEmitterNV50::emitSFnOp(const Instruction *i, uint8_t subOp)
{
   code[0] = 0x90000000;

   if (i->encSize == 4) {
      assert(i->op == OP_RCP);
      emitForm_MUL(i);
   } else {
      code[1] = subOp << 29;
      code[1] |= i->src[0].mod.abs() << 20;
      code[1] |= i->src[0].mod.neg() << 26;
      emitForm_MAD(i);
   }
}

void
CodeEmitterNV50::emitLogicOp(const Instruction *i)
{
   code[0] = 0xd0000000;

   if (i->src[1].getFile() == FILE_IMMEDIATE) {
      switch (i->op) {
      case OP_OR:  code[0] |= 0x0100; break;
      case OP_XOR: code[0] |= 0x8000; break;
      default:
         assert(i->op == OP_AND);
         break;
      }
      emitForm_IMM(i);
   } else {
      switch (i->op) {
      case OP_AND: code[1] = 0x04000000; break;
      case OP_OR:  code[1] = 0x04004000; break;
      case OP_XOR: code[1] = 0x04008000; break;
      default:
         assert(0);
         break;
      }
      emitForm_MAD(i);
   }
}

void
CodeEmitterNV50::emitARL(const Instruction *i)
{
   assert(i->src[1].getFile() == FILE_IMMEDIATE);

   code[0] = 0x00000001 | (i->getSrc(1)->reg.data.u32 & 0x3f) << 16;
   code[1] = 0xc0000000;

   code[0] |= (DDATA(i->def[0]).id + 1) << 2;
   emitSrc0(i->src[0]);
   emitFlagsRd(i);
}

void
CodeEmitterNV50::emitShift(const Instruction *i)
{
   if (i->def[0].getFile() == FILE_ADDRESS) {
      emitARL(i);
   } else {
      code[0] = 0x30000001;
      code[1] = (i->op == OP_SHR) ? 0xe4000000 : 0xc4000000;
      if (isSignedType(i->sType))
          code[1] |= 1 << 27;

      if (i->src[1].getFile() == FILE_IMMEDIATE) {
         code[1] |= 1 << 20;
         code[0] |= (i->getSrc(1)->reg.data.u32 & 0x7f) << 16;
         emitFlagsRd(i);
      } else {
         emitForm_MAD(i);
      }
   }
}

void
CodeEmitterNV50::emitOUT(const Instruction *i)
{
   code[0] = (i->op == OP_EMIT) ? 0xf0000200 : 0xf0000400;
   code[1] = 0xc0000001;

   emitFlagsRd(i);
}

void
CodeEmitterNV50::emitTEX(const TexInstruction *i)
{
   code[0] = 0xf0000001;
   code[1] = 0x00000000;

   switch (i->op) {
   case OP_TXB:
      code[1] = 0x20000000;
      break;
   case OP_TXL:
      code[1] = 0x40000000;
      break;
   case OP_TXF:
      code[0] = 0x01000000;
      break;
   case OP_TXG:
      code[0] = 0x01000000;
      code[1] = 0x80000000;
      break;
   default:
      assert(i->op == OP_TEX);
      break;
   }

   code[0] |= i->tex.r << 9;
   code[0] |= i->tex.s << 17;

   int argc = i->tex.target.getArgCount();

   if (i->op == OP_TXB || i->op == OP_TXL)
      argc += 1;
   if (i->tex.target.isShadow())
      argc += 1;
   assert(argc <= 4);

   code[0] |= (argc - 1) << 22;

   if (i->tex.target.isCube()) {
      code[0] |= 0x08000000;
   } else
   if (i->tex.useOffsets) {
      code[1] |= (i->tex.offset[0][0] & 0xf) << 16;
      code[1] |= (i->tex.offset[0][1] & 0xf) << 20;
      code[1] |= (i->tex.offset[0][2] & 0xf) << 24;
   }

   code[0] |= (i->tex.mask & 0x3) << 25;
   code[1] |= (i->tex.mask & 0xc) << 12;

   if (i->tex.liveOnly)
      code[1] |= 4;

   defId(i->def[0], 2);

   emitFlagsRd(i);
}

void
CodeEmitterNV50::emitFlow(const Instruction *i, uint8_t flowOp)
{
   const FlowInstruction *f = i->asFlow();

   code[0] = 0x00000003 | (flowOp << 28);
   code[1] = 0x00000000;

   emitFlagsRd(i);

   if (f && f->target.bb) {
      uint32_t pos;

      if (f->op == OP_CALL) {
         if (f->builtin) {
            pos = 0; // XXX: TODO
         } else {
            pos = f->target.fn->binPos;
         }
      } else {
         pos = f->target.bb->binPos;
      }

      code[0] |= ((pos >>  2) & 0xffff) << 11;
      code[1] |= ((pos >> 18) & 0x003f) << 14;
   }
}

bool
CodeEmitterNV50::emitInstruction(Instruction *insn)
{
   if (!insn->encSize) {
      ERROR("skipping unencodable instruction: "); insn->print();
      return false;
   } else
   if (codeSize + insn->encSize > codeSizeLimit) {
      ERROR("code emitter output buffer too small\n");
      return false;
   }

   switch (insn->op) {
   case OP_MOV:
      emitMOV(insn);
      break;
   case OP_NOP:
   case OP_JOIN:
      emitNOP();
      break;
   case OP_VFETCH:
   case OP_LOAD:
      emitLOAD(insn);
      break;
   case OP_EXPORT:
   case OP_STORE:
      emitSTORE(insn);
      break;
   case OP_PFETCH:
      emitPFETCH(insn);
      break;
   case OP_LINTERP:
   case OP_PINTERP:
      emitINTERP(insn);
      break;
   case OP_ADD:
   case OP_SUB:
      if (isFloatType(insn->dType))
         emitFADD(insn);
      else
         emitUADD(insn);
      break;
   case OP_MUL:
      if (isFloatType(insn->dType))
         emitFMUL(insn);
      else
         emitUMUL(insn);
      break;
   case OP_MAD:
   case OP_FMA:
      emitFMAD(insn);
      break;
      break;
   case OP_AND:
   case OP_OR:
   case OP_XOR:
      emitLogicOp(insn);
      break;
   case OP_MIN:
   case OP_MAX:
      emitMINMAX(insn);
      break;
   case OP_CEIL:
   case OP_FLOOR:
   case OP_TRUNC:
   case OP_CVT:
      emitCVT(insn);
      break;
   case OP_RCP:
      emitSFnOp(insn, 0);
      break;
   case OP_RSQ:
      emitSFnOp(insn, 2);
      break;
   case OP_LG2:
      emitSFnOp(insn, 3);
      break;
   case OP_SIN:
      emitSFnOp(insn, 4);
      break;
   case OP_COS:
      emitSFnOp(insn, 5);
      break;
   case OP_EX2:
      emitSFnOp(insn, 6);
      break;
   case OP_PRESIN:
   case OP_PREEX2:
      emitPreOp(insn);
      break;
   case OP_TEX:
   case OP_TXB:
   case OP_TXL:
      emitTEX(insn->asTex());
      break;
   case OP_EMIT:
   case OP_RESTART:
      emitOUT(insn);
      break;
   case OP_DISCARD:
      emitFlow(insn, 0x0);
      break;
   case OP_BRA:
      emitFlow(insn, 0x1);
      break;
   case OP_CALL:
      emitFlow(insn, 0x2);
      break;
   case OP_RET:
      emitFlow(insn, 0x3);
      break;
   case OP_PREBREAK:
      emitFlow(insn, 0x4);
      break;
   case OP_BREAK:
      emitFlow(insn, 0x5);
      break;
   case OP_QUADON:
      emitFlow(insn, 0x6);
      break;
   case OP_QUADPOP:
      emitFlow(insn, 0x7);
      break;
   case OP_JOINAT:
      emitFlow(insn, 0xa);
      break;
   case OP_PRERET:
      emitFlow(insn, 0xd);
      break;
   case OP_QUADOP:
      emitQUADOP(insn, insn->lanes, insn->subOp);
      break;
   case OP_DFDX:
      emitQUADOP(insn, 4, insn->src[0].mod.neg() ? 0x66 : 0x99);
      break;
   case OP_DFDY:
      emitQUADOP(insn, 5, insn->src[0].mod.neg() ? 0x5a : 0xa5);
      break;
   case OP_PHI:
   case OP_UNION:
   case OP_CONSTRAINT:
      ERROR("operation should have been eliminated");
      return false;
   case OP_EXP:
   case OP_LOG:
   case OP_SQRT:
   case OP_POW:
   case OP_SELP:
   case OP_SLCT:
   case OP_TXD:
   case OP_PRECONT:
   case OP_CONT:
   case OP_POPCNT:
   case OP_INSBF:
   case OP_EXTBF:
      ERROR("operation should have been lowered\n");
      return false;
   default:
      ERROR("unknow op\n");
      return false;
   }
   if (insn->join)
      code[1] |= 0x2;
   else
   if (insn->exit)
      code[1] |= 0x1;

   assert((insn->encSize == 8) == (code[1] & 1));

   code += insn->encSize / 4;
   codeSize += insn->encSize;
   return true;
}

uint32_t
CodeEmitterNV50::getMinEncodingSize(const Instruction *i) const
{
   const Target::OpInfo &info = targ->getOpInfo(i);

   if (info.minEncSize == 8)
      return 8;

   return 4;
}

CodeEmitterNV50::CodeEmitterNV50(const Target *target) : targ(target)
{
   code = NULL;
   codeSize = codeSizeLimit = 0;
}

CodeEmitter *
Target::getCodeEmitter(Program::Type type)
{
   CodeEmitterNV50 *emit = new CodeEmitterNV50(this);
   emit->setProgramType(type);
   return emit;
}

} // namespace nv50_ir

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
#include "nv50_ir_build_util.h"

namespace nv50_ir {

BuildUtil::BuildUtil()
{
   prog = NULL;
   func = NULL;
   bb = NULL;
   pos = NULL;

   memset(imms, 0, sizeof(imms));
   immCount = 0;
}

void
BuildUtil::addImmediate(ImmediateValue *imm)
{
   if (immCount > (NV50_IR_BUILD_IMM_HT_SIZE * 3) / 4)
      return;

   unsigned int pos = u32Hash(imm->reg.data.u32);

   while (imms[pos])
      pos = (pos + 1) % NV50_IR_BUILD_IMM_HT_SIZE;
   imms[pos] = imm;
   immCount++;
}

Instruction *
BuildUtil::mkOp1(operation op, DataType ty, Value *dst, Value *src)
{
   Instruction *insn = new_Instruction(func, op, ty);

   insn->setDef(0, dst);
   insn->setSrc(0, src);

   insert(insn);
   return insn;
}

Instruction *
BuildUtil::mkOp2(operation op, DataType ty, Value *dst,
                 Value *src0, Value *src1)
{
   Instruction *insn = new_Instruction(func, op, ty);

   insn->setDef(0, dst);
   insn->setSrc(0, src0);
   insn->setSrc(1, src1);

   insert(insn);
   return insn;
}

Instruction *
BuildUtil::mkOp3(operation op, DataType ty, Value *dst,
                 Value *src0, Value *src1, Value *src2)
{
   Instruction *insn = new_Instruction(func, op, ty);

   insn->setDef(0, dst);
   insn->setSrc(0, src0);
   insn->setSrc(1, src1);
   insn->setSrc(2, src2);

   insert(insn);
   return insn;
}

LValue *
BuildUtil::mkLoad(DataType ty, Symbol *mem, Value *ptr)
{
   Instruction *insn = new_Instruction(func, OP_LOAD, ty);
   LValue *def = getScratch();

   insn->setDef(0, def);
   insn->setSrc(0, mem);
   if (ptr)
      insn->setIndirect(0, 0, ptr);

   insert(insn);
   return def;
}

Instruction *
BuildUtil::mkStore(operation op, DataType ty, Symbol *mem, Value *ptr,
                   Value *stVal)
{
   Instruction *insn = new_Instruction(func, op, ty);

   insn->setSrc(0, mem);
   insn->setSrc(1, stVal);
   if (ptr)
      insn->setIndirect(0, 0, ptr);

   insert(insn);
   return insn;
}

Instruction *
BuildUtil::mkFetch(Value *dst, DataType ty, DataFile file, int32_t offset,
                   Value *attrRel, Value *primRel)
{
   Symbol *sym = mkSymbol(file, 0, ty, offset);

   Instruction *insn = mkOp1(OP_VFETCH, ty, dst, sym);

   insn->setIndirect(0, 0, attrRel);
   insn->setIndirect(0, 1, primRel);

   // already inserted
   return insn;
}

Instruction *
BuildUtil::mkInterp(unsigned mode, Value *dst, int32_t offset, Value *rel)
{
   operation op = OP_LINTERP;
   DataType ty = TYPE_F32;

   if ((mode & NV50_IR_INTERP_MODE_MASK) == NV50_IR_INTERP_FLAT)
      ty = TYPE_U32;
   else
   if ((mode & NV50_IR_INTERP_MODE_MASK) == NV50_IR_INTERP_PERSPECTIVE)
      op = OP_PINTERP;

   Symbol *sym = mkSymbol(FILE_SHADER_INPUT, 0, ty, offset);

   Instruction *insn = mkOp1(op, ty, dst, sym);
   insn->setIndirect(0, 0, rel);
   return insn;
}

Instruction *
BuildUtil::mkMov(Value *dst, Value *src, DataType ty)
{
   Instruction *insn = new_Instruction(func, OP_MOV, ty);

   insn->setDef(0, dst);
   insn->setSrc(0, src);

   insert(insn);
   return insn;
}

Instruction *
BuildUtil::mkMovToReg(int id, Value *src)
{
   Instruction *insn = new_Instruction(func, OP_MOV, typeOfSize(src->reg.size));

   insn->setDef(0, new_LValue(func, FILE_GPR));
   insn->getDef(0)->reg.data.id = id;
   insn->setSrc(0, src);

   insert(insn);
   return insn;
}

Instruction *
BuildUtil::mkMovFromReg(Value *dst, int id)
{
   Instruction *insn = new_Instruction(func, OP_MOV, typeOfSize(dst->reg.size));

   insn->setDef(0, dst);
   insn->setSrc(0, new_LValue(func, FILE_GPR));
   insn->getSrc(0)->reg.data.id = id;

   insert(insn);
   return insn;
}

Instruction *
BuildUtil::mkCvt(operation op,
                 DataType dstTy, Value *dst, DataType srcTy, Value *src)
{
   Instruction *insn = new_Instruction(func, op, dstTy);

   insn->setType(dstTy, srcTy);
   insn->setDef(0, dst);
   insn->setSrc(0, src);

   insert(insn);
   return insn;
}

CmpInstruction *
BuildUtil::mkCmp(operation op, CondCode cc, DataType ty, Value *dst,
                 Value *src0, Value *src1, Value *src2)
{
   CmpInstruction *insn = new_CmpInstruction(func, op);

   insn->setType(dst->reg.file == FILE_PREDICATE ? TYPE_U8 : ty, ty);
   insn->setCondition(cc);
   insn->setDef(0, dst);
   insn->setSrc(0, src0);
   insn->setSrc(1, src1);
   if (src2)
      insn->setSrc(2, src2);

   if (dst->reg.file == FILE_FLAGS)
      insn->flagsDef = 0;

   insert(insn);
   return insn;
}

Instruction *
BuildUtil::mkTex(operation op, TexTarget targ, uint8_t tic, uint8_t tsc,
                 Value **def, Value **src)
{
   TexInstruction *tex = new_TexInstruction(func, op);

   for (int d = 0; d < 4 && def[d]; ++d)
      tex->setDef(d, def[d]);
   for (int s = 0; s < 4 && src[s]; ++s)
      tex->setSrc(s, src[s]);

   tex->setTexture(targ, tic, tsc);

   return tex;
}

Instruction *
BuildUtil::mkQuadop(uint8_t q, Value *def, uint8_t l, Value *src0, Value *src1)
{
   Instruction *quadop = mkOp2(OP_QUADOP, TYPE_F32, def, src0, src1);
   quadop->subOp = q;
   quadop->lanes = l;
   return quadop;
}

Instruction *
BuildUtil::mkSelect(Value *pred, Value *dst, Value *trSrc, Value *flSrc)
{
   Instruction *insn;
   LValue *def0 = getSSA();
   LValue *def1 = getSSA();

   mkMov(def0, trSrc)->setPredicate(CC_P, pred);
   mkMov(def1, flSrc)->setPredicate(CC_NOT_P, pred);

   insn = mkOp2(OP_UNION, typeOfSize(dst->reg.size), dst, def0, def1);

   insert(insn);
   return insn;
}

FlowInstruction *
BuildUtil::mkFlow(operation op, BasicBlock *targ, CondCode cc, Value *pred)
{
   FlowInstruction *insn = new_FlowInstruction(func, op, targ);

   if (pred)
      insn->setPredicate(cc, pred);

   insert(insn);
   return insn;
}

void
BuildUtil::mkClobber(DataFile f, uint32_t rMask, int unit)
{
   static const uint16_t baseSize2[16] =
   {
      0x0000, 0x0010, 0x0011, 0x0020, 0x0012, 0x1210, 0x1211, 0x1220,
      0x0013, 0x1310, 0x1311, 0x0020, 0x1320, 0x0022, 0x2210, 0x0040,
   };

   int base = 0;

   for (; rMask; rMask >>= 4, base += 4) {
      const uint32_t mask = rMask & 0xf;
      if (!mask)
         continue;
      int base1 = (baseSize2[mask] >>  0) & 0xf;
      int size1 = (baseSize2[mask] >>  4) & 0xf;
      int base2 = (baseSize2[mask] >>  8) & 0xf;
      int size2 = (baseSize2[mask] >> 12) & 0xf;
      Instruction *insn = mkOp(OP_NOP, TYPE_NONE, NULL);
      if (1) { // size1 can't be 0
         LValue *reg = new_LValue(func, f);
         reg->reg.size = size1 << unit;
         reg->reg.data.id = base + base1;
         insn->setDef(0, reg);
      }
      if (size2) {
         LValue *reg = new_LValue(func, f);
         reg->reg.size = size2 << unit;
         reg->reg.data.id = base + base2;
         insn->setDef(1, reg);
      }
   }
}

ImmediateValue *
BuildUtil::mkImm(uint32_t u)
{
   unsigned int pos = u32Hash(u);

   while (imms[pos] && imms[pos]->reg.data.u32 != u)
      pos = (pos + 1) % NV50_IR_BUILD_IMM_HT_SIZE;

   ImmediateValue *imm = imms[pos];
   if (!imm) {
      imm = new_ImmediateValue(prog, u);
      addImmediate(imm);
   }
   return imm;
}

ImmediateValue *
BuildUtil::mkImm(uint64_t u)
{
   ImmediateValue *imm = new_ImmediateValue(prog, (uint32_t)0);

   imm->reg.size = 8;
   imm->reg.type = TYPE_U64;
   imm->reg.data.u64 = u;

   return imm;
}

ImmediateValue *
BuildUtil::mkImm(float f)
{
   union {
      float f32;
      uint32_t u32;
   } u;
   u.f32 = f;
   return mkImm(u.u32);
}

Value *
BuildUtil::loadImm(Value *dst, float f)
{
   return mkOp1v(OP_MOV, TYPE_F32, dst ? dst : getScratch(), mkImm(f));
}

Value *
BuildUtil::loadImm(Value *dst, uint32_t u)
{
   return mkOp1v(OP_MOV, TYPE_U32, dst ? dst : getScratch(), mkImm(u));
}

Value *
BuildUtil::loadImm(Value *dst, uint64_t u)
{
   return mkOp1v(OP_MOV, TYPE_U64, dst ? dst : getScratch(8), mkImm(u));
}

Symbol *
BuildUtil::mkSymbol(DataFile file, int8_t fileIndex, DataType ty,
                    uint32_t baseAddr)
{
   Symbol *sym = new_Symbol(prog, file, fileIndex);

   sym->setOffset(baseAddr);
   sym->reg.type = ty;
   sym->reg.size = typeSizeof(ty);

   return sym;
}

Symbol *
BuildUtil::mkSysVal(SVSemantic svName, uint32_t svIndex)
{
   Symbol *sym = new_Symbol(prog, FILE_SYSTEM_VALUE, 0);

   assert(svIndex < 4 ||
          (svName == SV_CLIP_DISTANCE || svName == SV_TESS_FACTOR));

   switch (svName) {
   case SV_POSITION:
   case SV_FACE:
   case SV_YDIR:
   case SV_POINT_SIZE:
   case SV_POINT_COORD:
   case SV_CLIP_DISTANCE:
   case SV_TESS_FACTOR:
      sym->reg.type = TYPE_F32;
      break;
   default:
      sym->reg.type = TYPE_U32;
      break;
   }
   sym->reg.size = typeSizeof(sym->reg.type);

   sym->reg.data.sv.sv = svName;
   sym->reg.data.sv.index = svIndex;

   return sym;
}

void
BuildUtil::DataArray::init()
{
   values = NULL;
   baseAddr = 0;
   arrayLen = 0;

   vecDim = 4;
   eltSize = 2;

   file = FILE_GPR;
   regOnly = true;
}

BuildUtil::DataArray::DataArray()
{
   init();
}

BuildUtil::DataArray::DataArray(BuildUtil *bld) : up(bld)
{
   init();
}

BuildUtil::DataArray::~DataArray()
{
   if (values)
      delete[] values;
}

void
BuildUtil::DataArray::setup(uint32_t base, int len, int v, int size,
                            DataFile f, int8_t fileIndex)
{
   baseAddr = base;
   arrayLen = len;

   vecDim = v;
   eltSize = size;

   file = f;
   regOnly = !isMemoryFile(f);

   values = new Value * [arrayLen * vecDim];
   if (values)
      memset(values, 0, arrayLen * vecDim * sizeof(Value *));

   if (!regOnly) {
      baseSym = new_Symbol(up->getProgram(), file, fileIndex);
      baseSym->setOffset(baseAddr);
      baseSym->reg.size = size;
   }
}

Value *
BuildUtil::DataArray::acquire(int i, int c)
{
   const unsigned int idx = i * vecDim + c;

   assert(idx < arrayLen * vecDim);

   if (regOnly) {
      const unsigned int idx = i * 4 + c; // vecDim always 4 if regOnly
      if (!values[idx])
         values[idx] = new_LValue(up->getFunction(), file);
      return values[idx];
   } else {
      return up->getScratch();
   }
}

Value *
BuildUtil::DataArray::load(int i, int c, Value *ptr)
{
   const unsigned int idx = i * vecDim + c;

   assert(idx < arrayLen * vecDim);

   if (regOnly) {
      if (!values[idx])
         values[idx] = new_LValue(up->getFunction(), file);
      return values[idx];
   } else {
      Symbol *sym = reinterpret_cast<Symbol *>(values[idx]);
      if (!sym)
         values[idx] = sym = this->mkSymbol(i, c, baseSym);
      return up->mkLoad(typeOfSize(eltSize), sym, ptr);
   }
}

void
BuildUtil::DataArray::store(int i, int c, Value *ptr, Value *value)
{
   const unsigned int idx = i * vecDim + c;

   assert(idx < arrayLen * vecDim);

   if (regOnly) {
      assert(!ptr);
      assert(!values[idx] || values[idx] == value);
      values[idx] = value;
   } else {
      Symbol *sym = reinterpret_cast<Symbol *>(values[idx]);
      if (!sym)
         values[idx] = sym = this->mkSymbol(i, c, baseSym);
      up->mkStore(OP_STORE, typeOfSize(value->reg.size), sym, ptr, value);
   }
}

Symbol *
BuildUtil::DataArray::mkSymbol(int i, int c, Symbol *base)
{
   const unsigned int idx = i * vecDim + c;

   Symbol *sym = new_Symbol(up->getProgram(), file, 0);

   assert(base || (idx < arrayLen && c < vecDim));

   sym->reg.size = eltSize;
   sym->reg.type = typeOfSize(eltSize);

   sym->setAddress(base, baseAddr + idx * eltSize);
   return sym;
}

} // namespace nv50_ir

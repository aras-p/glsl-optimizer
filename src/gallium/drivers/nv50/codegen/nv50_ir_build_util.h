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

#ifndef __NV50_IR_BUILD_UTIL__
#define __NV50_IR_BUILD_UTIL__

namespace nv50_ir {

class BuildUtil
{
public:
   BuildUtil();

   inline void setProgram(Program *);
   inline Program *getProgram() const { return prog; }
   inline Function *getFunction() const { return func; }

   // keeps inserting at head/tail of block
   inline void setPosition(BasicBlock *, bool tail);
   // position advances only if @after is true
   inline void setPosition(Instruction *, bool after);

   inline BasicBlock *getBB() { return bb; }

   inline void insert(Instruction *);
   inline void remove(Instruction *i) { assert(i->bb == bb); bb->remove(i); }

   inline LValue *getScratch(int size = 4, DataFile = FILE_GPR);
   inline LValue *getSSA(int size = 4); // scratch value for a single assignment

   inline Instruction *mkOp(operation, DataType, Value *);
   Instruction *mkOp1(operation, DataType, Value *, Value *);
   Instruction *mkOp2(operation, DataType, Value *, Value *, Value *);
   Instruction *mkOp3(operation, DataType, Value *, Value *, Value *, Value *);

   LValue *mkOp1v(operation, DataType, Value *, Value *);
   LValue *mkOp2v(operation, DataType, Value *, Value *, Value *);
   LValue *mkOp3v(operation, DataType, Value *, Value *, Value *, Value *);

   LValue *mkLoad(DataType, Symbol *, Value *ptr);
   Instruction *mkStore(operation, DataType, Symbol *, Value *ptr, Value *val);

   Instruction *mkMov(Value *, Value *, DataType = TYPE_U32);
   Instruction *mkMovToReg(int id, Value *);
   Instruction *mkMovFromReg(Value *, int id);

   Instruction *mkInterp(unsigned mode, Value *, int32_t offset, Value *rel);
   Instruction *mkFetch(Value *, DataType, DataFile, int32_t offset,
                        Value *attrRel, Value *primRel);

   Instruction *mkCvt(operation, DataType, Value *, DataType, Value *);
   CmpInstruction *mkCmp(operation, CondCode, DataType,
			 Value *,
			 Value *, Value *, Value * = NULL);
   Instruction *mkTex(operation, TexTarget, uint8_t tic, uint8_t tsc,
                      Value **def, Value **src);
   Instruction *mkQuadop(uint8_t qop, Value *, uint8_t l, Value *, Value *);

   FlowInstruction *mkFlow(operation, BasicBlock *target,
                           CondCode, Value *pred);

   Instruction *mkSelect(Value *pred, Value *dst, Value *trSrc, Value *flSrc);

   void mkClobber(DataFile file, uint32_t regMask, int regUnitLog2);

   ImmediateValue *mkImm(float);
   ImmediateValue *mkImm(uint32_t);
   ImmediateValue *mkImm(uint64_t);

   ImmediateValue *mkImm(int i) { return mkImm((uint32_t)i); }

   Value *loadImm(Value *dst, float);
   Value *loadImm(Value *dst, uint32_t);
   Value *loadImm(Value *dst, uint64_t);

   Value *loadImm(Value *dst, int i) { return loadImm(dst, (uint32_t)i); }

   class DataArray
   {
   public:
      DataArray();
      DataArray(BuildUtil *);
      ~DataArray();

      inline void setParent(BuildUtil *bld) { assert(!up); up = bld; }

      void setup(uint32_t base, int len, int vecDim, int size,
                 DataFile, int8_t fileIndex = 0);

      inline bool exists(unsigned int i, unsigned int c);

      Value *load(int i, int c, Value *ptr);
      void store(int i, int c, Value *ptr, Value *value);
      Value *acquire(int i, int c);

   private:
      Symbol *mkSymbol(int i, int c, Symbol *base);

   private:
      Value **values;
      uint32_t baseAddr;
      uint32_t arrayLen;
      Symbol *baseSym;

      uint8_t vecDim;
      uint8_t eltSize; // in bytes

      DataFile file;
      bool regOnly;

      BuildUtil *up;

      void init();
   };

   Symbol *mkSymbol(DataFile file, int8_t fileIndex,
                    DataType ty, uint32_t baseAddress);

   Symbol *mkSysVal(SVSemantic svName, uint32_t svIndex);

private:
   void addImmediate(ImmediateValue *);
   inline unsigned int u32Hash(uint32_t);

protected:
   Program *prog;
   Function *func;
   Instruction *pos;
   BasicBlock *bb;
   bool tail;

#define NV50_IR_BUILD_IMM_HT_SIZE 256

   ImmediateValue *imms[NV50_IR_BUILD_IMM_HT_SIZE];
   unsigned int immCount;
};

unsigned int BuildUtil::u32Hash(uint32_t u)
{
   return (u % 273) % NV50_IR_BUILD_IMM_HT_SIZE;
}

void BuildUtil::setProgram(Program *program)
{
   prog = program;
}

void
BuildUtil::setPosition(BasicBlock *block, bool atTail)
{
   bb = block;
   prog = bb->getProgram();
   func = bb->getFunction();
   pos = NULL;
   tail = atTail;
}

void
BuildUtil::setPosition(Instruction *i, bool after)
{
   bb = i->bb;
   prog = bb->getProgram();
   func = bb->getFunction();
   pos = i;
   tail = after;
   assert(bb);
}

LValue *
BuildUtil::getScratch(int size, DataFile f)
{
   LValue *lval = new_LValue(func, f);
   if (size != 4)
      lval->reg.size = size;
   return lval;
}

LValue *
BuildUtil::getSSA(int size)
{
   LValue *lval = new_LValue(func, FILE_GPR);
   lval->ssa = 1;
   if (size != 4)
      lval->reg.size = size;
   return lval;
}

void BuildUtil::insert(Instruction *i)
{
   if (!pos) {
      tail ? bb->insertTail(i) : bb->insertHead(i);
   } else {
      if (tail) {
         bb->insertAfter(pos, i);
         pos = i;
      } else {
         bb->insertBefore(pos, i);
      }
   }
}

Instruction *
BuildUtil::mkOp(operation op, DataType ty, Value *dst)
{
   Instruction *insn = new_Instruction(func, op, ty);
   insn->setDef(0, dst);
   insert(insn);
   if (op == OP_DISCARD || op == OP_EXIT ||
       op == OP_JOIN ||
       op == OP_QUADON || op == OP_QUADPOP ||
       op == OP_EMIT || op == OP_RESTART)
      insn->fixed = 1;
   return insn;
}

inline LValue *
BuildUtil::mkOp1v(operation op, DataType ty, Value *dst, Value *src)
{
   mkOp1(op, ty, dst, src);
   return dst->asLValue();
}

inline LValue *
BuildUtil::mkOp2v(operation op, DataType ty, Value *dst,
                  Value *src0, Value *src1)
{
   mkOp2(op, ty, dst, src0, src1);
   return dst->asLValue();
}

inline LValue *
BuildUtil::mkOp3v(operation op, DataType ty, Value *dst,
                  Value *src0, Value *src1, Value *src2)
{
   mkOp3(op, ty, dst, src0, src1, src2);
   return dst->asLValue();
}

bool
BuildUtil::DataArray::exists(unsigned int i, unsigned int c)
{
   assert(i < arrayLen && c < vecDim);
   return !regOnly || values[i * vecDim + c];
}

} // namespace nv50_ir

#endif // __NV50_IR_BUILD_UTIL_H__

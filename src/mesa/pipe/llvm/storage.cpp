/**************************************************************************
 *
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

 /*
  * Authors:
  *   Zack Rusin zack@tungstengraphics.com
  */

#include "storage.h"

#include "pipe/tgsi/exec/tgsi_token.h"
#include <llvm/BasicBlock.h>
#include <llvm/Module.h>
#include <llvm/Value.h>

#include <llvm/CallingConv.h>
#include <llvm/Constants.h>
#include <llvm/DerivedTypes.h>
#include <llvm/InstrTypes.h>
#include <llvm/Instructions.h>

using namespace llvm;

Storage::Storage(llvm::BasicBlock *block, llvm::Value *out,
                                         llvm::Value *in, llvm::Value *consts)
   : m_block(block), m_OUT(out),
     m_IN(in), m_CONST(consts),
     m_temps(32), m_addrs(32),
     m_dstCache(32),
     m_idx(0)
{
   m_floatVecType = VectorType::get(Type::FloatTy, 4);
   m_intVecType   = VectorType::get(IntegerType::get(32), 4);

   m_undefFloatVec = UndefValue::get(m_floatVecType);
   m_undefIntVec   = UndefValue::get(m_intVecType);
   m_extSwizzleVec = 0;

   m_numConsts = 0;
}

//can only build vectors with all members in the [0, 9] range
llvm::Constant *Storage::shuffleMask(int vec)
{
   if (!m_extSwizzleVec) {
      Constant *const_vec = Constant::getNullValue(m_floatVecType);
      InsertElementInst *res = new InsertElementInst(const_vec,
                                                     ConstantFP::get(Type::FloatTy, APFloat(0.f)),
                                                     unsigned(0),
                                                     name("extswx"), m_block);
      res = new InsertElementInst(res, ConstantFP::get(Type::FloatTy, APFloat(1.f)),
                                  unsigned(1),
                                  name("extswy"),
                                  m_block);
      res = new InsertElementInst(res, ConstantFP::get(Type::FloatTy, APFloat(0.f)),
                                  unsigned(2),
                                  name("extswz"),
                                  m_block);
      res = new InsertElementInst(res, ConstantFP::get(Type::FloatTy, APFloat(1.f)),
                                  unsigned(3),
                                  name("extsww"),
                                  m_block);
      m_extSwizzleVec = res;
   }

   if (m_intVecs.find(vec) != m_intVecs.end()) {
      return m_intVecs[vec];
   }
   int origVec = vec;
   Constant* const_vec = 0;
   if (origVec == 0) {
      const_vec = Constant::getNullValue(m_intVecType);
   } else {
      int x = vec / 1000; vec -= x * 1000;
      int y = vec / 100;  vec -= y * 100;
      int z = vec / 10;   vec -= z * 10;
      int w = vec;
      std::vector<Constant*> elems;
      elems.push_back(constantInt(x));
      elems.push_back(constantInt(y));
      elems.push_back(constantInt(z));
      elems.push_back(constantInt(w));
      const_vec = ConstantVector::get(m_intVecType, elems);
   }

   m_intVecs[origVec] = const_vec;
   return const_vec;
}

llvm::ConstantInt *Storage::constantInt(int idx)
{
   if (m_constInts.find(idx) != m_constInts.end()) {
      return m_constInts[idx];
   }
   ConstantInt *const_int = ConstantInt::get(APInt(32,  idx));
   m_constInts[idx] = const_int;
   return const_int;
}

llvm::Value *Storage::inputElement(int idx, llvm::Value *indIdx)
{
   if (!indIdx && m_inputs.find(idx) != m_inputs.end()) {
      return m_inputs[idx];
   }
   GetElementPtrInst *getElem = 0;

   if (indIdx) {
      getElem = new GetElementPtrInst(m_IN,
                                      BinaryOperator::create(Instruction::Add,
                                                             indIdx,
                                                             constantInt(idx),
                                                             name("add"),
                                                             m_block),
                                      name("input_ptr"),
                                      m_block);
   } else {
      getElem = new GetElementPtrInst(m_IN,
                                      constantInt(idx),
                                      name("input_ptr"),
                                      m_block);
   }

   LoadInst *load = new LoadInst(getElem, name("input"),
                                 false, m_block);
   load->setAlignment(8);
   m_inputs[idx] = load;

   return load;
}

llvm::Value *Storage::constElement(int idx, llvm::Value *indIdx)
{
   m_numConsts = ((idx + 1) > m_numConsts) ? (idx + 1) : m_numConsts;
   if (!indIdx && m_consts.find(idx) != m_consts.end()) {
      return m_consts[idx];
   }

   GetElementPtrInst *getElem = 0;

   if (indIdx)
      getElem = new GetElementPtrInst(m_CONST,
                                      BinaryOperator::create(Instruction::Add,
                                                             indIdx,
                                                             constantInt(idx),
                                                             name("add"),
                                                             m_block),
                                      name("const_ptr"),
                                      m_block);
   else
      getElem = new GetElementPtrInst(m_CONST,
                                      constantInt(idx),
                                      name("const_ptr"),
                                      m_block);
   LoadInst *load = new LoadInst(getElem, name("const"),
                                 false, m_block);
   load->setAlignment(8);
   m_consts[idx] = load;
   return load;
}

llvm::Value *Storage::shuffleVector(llvm::Value *vec, int shuffle)
{
   Constant *mask = shuffleMask(shuffle);
   ShuffleVectorInst *res =
      new ShuffleVectorInst(vec, m_extSwizzleVec, mask,
                            name("shuffle"), m_block);
   return res;
}


llvm::Value *Storage::tempElement(int idx)
{
   Value *ptr = m_temps[idx];
   if (!ptr)
      return m_undefFloatVec;
   llvm::LoadInst *li = new LoadInst(ptr, name("temp"),
                                     false, m_block);
   li->setAlignment(8);
   return li;
}

void Storage::setTempElement(int idx, llvm::Value *val, int mask)
{
   if (mask != TGSI_WRITEMASK_XYZW) {
      llvm::Value *templ = 0;
      if (m_temps[idx])
         templ = tempElement(idx);
      val = maskWrite(val, mask, templ);
   }
   llvm::Value *ptr = m_temps[idx];
   assert(ptr);
   StoreInst *st = new StoreInst(val, ptr, false, m_block);
   st->setAlignment(8);
}

void Storage::store(int dstIdx, llvm::Value *val, int mask)
{
   if (mask != TGSI_WRITEMASK_XYZW) {
      llvm::Value *templ = m_dstCache[dstIdx];
      val = maskWrite(val, mask, templ);
   }

   GetElementPtrInst *getElem = new GetElementPtrInst(m_OUT,
                                                      constantInt(dstIdx),
                                                      name("out_ptr"),
                                                      m_block);
   StoreInst *st = new StoreInst(val, getElem, false, m_block);
   st->setAlignment(8);
   //m_dstCache[dstIdx] = st;
}

llvm::Value *Storage::maskWrite(llvm::Value *src, int mask, llvm::Value *templ)
{
   llvm::Value *dst = templ;
   if (!dst)
      dst = Constant::getNullValue(m_floatVecType);
   if ((mask & TGSI_WRITEMASK_X)) {
      llvm::Value *x = new ExtractElementInst(src, unsigned(0),
                                              name("x"), m_block);
      dst = new InsertElementInst(dst, x, unsigned(0),
                                  name("dstx"), m_block);
   }
   if ((mask & TGSI_WRITEMASK_Y)) {
      llvm::Value *y = new ExtractElementInst(src, unsigned(1),
                                              name("y"), m_block);
      dst = new InsertElementInst(dst, y, unsigned(1),
                                  name("dsty"), m_block);
   }
   if ((mask & TGSI_WRITEMASK_Z)) {
      llvm::Value *z = new ExtractElementInst(src, unsigned(2),
                                              name("z"), m_block);
      dst = new InsertElementInst(dst, z, unsigned(2),
                                  name("dstz"), m_block);
   }
   if ((mask & TGSI_WRITEMASK_W)) {
      llvm::Value *w = new ExtractElementInst(src, unsigned(3),
                                              name("w"), m_block);
      dst = new InsertElementInst(dst, w, unsigned(3),
                                  name("dstw"), m_block);
   }
   return dst;
}

const char * Storage::name(const char *prefix)
{
   ++m_idx;
   snprintf(m_name, 32, "%s%d", prefix, m_idx);
   return m_name;
}

int Storage::numConsts() const
{
   return m_numConsts;
}

llvm::Value * Storage::addrElement(int idx) const
{
   Value *ret = m_addrs[idx];
   if (!ret)
      return m_undefFloatVec;
   return ret;
}

void Storage::setAddrElement(int idx, llvm::Value *val, int mask)
{
   if (mask != TGSI_WRITEMASK_XYZW) {
      llvm::Value *templ = m_addrs[idx];
      val = maskWrite(val, mask, templ);
   }
   m_addrs[idx] = val;
}

llvm::Value * Storage::extractIndex(llvm::Value *vec)
{
   llvm::Value *x = new ExtractElementInst(vec, unsigned(0),
                                           name("x"), m_block);
   return new FPToSIInst(x, IntegerType::get(32), name("intidx"), m_block);
}

void Storage::setCurrentBlock(llvm::BasicBlock *block)
{
   m_block = block;
}

void Storage::declareTemp(int idx)
{
    m_temps[idx] = new AllocaInst(m_floatVecType, name("temp"), m_block);
}

llvm::Value * Storage::outputElement(int idx, llvm::Value *indIdx )
{
    GetElementPtrInst *getElem = 0;

   if (indIdx) {
      getElem = new GetElementPtrInst(m_IN,
                                      BinaryOperator::create(Instruction::Add,
                                                             indIdx,
                                                             constantInt(idx),
                                                             name("add"),
                                                             m_block),
                                      name("output_ptr"),
                                      m_block);
   } else {
      getElem = new GetElementPtrInst(m_IN,
                                      constantInt(idx),
                                      name("output_ptr"),
                                      m_block);
   }

   LoadInst *load = new LoadInst(getElem, name("output"),
                                 false, m_block);
   load->setAlignment(8);
   m_inputs[idx] = load;

   return load;
}

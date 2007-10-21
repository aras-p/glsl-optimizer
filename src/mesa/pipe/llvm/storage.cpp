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
     m_temps(32), m_dstCache(32),
     m_idx(0)
{
   m_floatVecType = VectorType::get(Type::FloatTy, 4);
   m_intVecType   = VectorType::get(IntegerType::get(32), 4);

   m_undefFloatVec = UndefValue::get(m_floatVecType);
   m_undefIntVec   = UndefValue::get(m_intVecType);

   m_numConsts = 0;
}

//can only build vectors with all members in the [0, 9] range
llvm::Constant *Storage::shuffleMask(int vec)
{
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

llvm::Value *Storage::inputElement(int idx)
{
   if (m_inputs.find(idx) != m_inputs.end()) {
      return m_inputs[idx];
   }
   GetElementPtrInst *getElem = new GetElementPtrInst(m_IN,
                                                      constantInt(idx),
                                                      name("input_ptr"),
                                                      m_block);
   LoadInst *load = new LoadInst(getElem, name("input"),
                                 false, m_block);
   m_inputs[idx] = load;
   return load;
}

llvm::Value *Storage::constElement(int idx)
{
   m_numConsts = ((idx + 1) > m_numConsts) ? (idx + 1) : m_numConsts;
   if (m_consts.find(idx) != m_consts.end()) {
      return m_consts[idx];
   }
   GetElementPtrInst *getElem = new GetElementPtrInst(m_CONST,
                                                      constantInt(idx),
                                                      name("const_ptr"),
                                                      m_block);
   LoadInst *load = new LoadInst(getElem, name("const"),
                                 false, m_block);
   m_consts[idx] = load;
   return load;
}

llvm::Value *Storage::shuffleVector(llvm::Value *vec, int shuffle)
{
   Constant *mask = shuffleMask(shuffle);
   ShuffleVectorInst *res =
      new ShuffleVectorInst(vec, m_undefFloatVec, mask,
                            name("shuffle"), m_block);
   return res;
}


llvm::Value *Storage::tempElement(int idx) const
{
   Value *ret = m_temps[idx];
   if (!ret)
      return m_undefFloatVec;
   return ret;
}

void Storage::setTempElement(int idx, llvm::Value *val, int mask)
{
   if (mask != TGSI_WRITEMASK_XYZW) {
      llvm::Value *templ = m_temps[idx];
      val = maskWrite(val, mask, templ);
   }
   m_temps[idx] = val;
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

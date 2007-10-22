#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include <llvm/BasicBlock.h>
#include <llvm/Module.h>
#include <llvm/Value.h>

namespace llvm {
   class VectorType;
   class Function;
}

class Instructions
{
public:
   Instructions(llvm::Module *mod, llvm::Function *func, llvm::BasicBlock *block);

   llvm::Value *add(llvm::Value *in1, llvm::Value *in2);
   llvm::Value *dp3(llvm::Value *in1, llvm::Value *in2);
   llvm::Value *dp4(llvm::Value *in1, llvm::Value *in2);
   llvm::Value *dph(llvm::Value *in1, llvm::Value *in2);
   llvm::Value *dst(llvm::Value *in1, llvm::Value *in2);
   llvm::Value *ex2(llvm::Value *in);
   llvm::Value *floor(llvm::Value *in);
   llvm::Value *frc(llvm::Value *in);
   llvm::Value *lit(llvm::Value *in);
   llvm::Value *lg2(llvm::Value *in);
   llvm::Value *madd(llvm::Value *in1, llvm::Value *in2,
                     llvm::Value *in2);
   llvm::Value *min(llvm::Value *in1, llvm::Value *in2);
   llvm::Value *max(llvm::Value *in1, llvm::Value *in2);
   llvm::Value *mul(llvm::Value *in1, llvm::Value *in2);
   llvm::Value *pow(llvm::Value *in1, llvm::Value *in2);
   llvm::Value *rcp(llvm::Value *in);
   llvm::Value *rsq(llvm::Value *in);
   llvm::Value *sge(llvm::Value *in1, llvm::Value *in2);
   llvm::Value *slt(llvm::Value *in1, llvm::Value *in2);
   llvm::Value *sub(llvm::Value *in1, llvm::Value *in2);

   void printVector(llvm::Value *val);
private:
   const char *name(const char *prefix);

   llvm::Value *callFAbs(llvm::Value *val);
   llvm::Value *callFloor(llvm::Value *val);
   llvm::Value *callFSqrt(llvm::Value *val);
   llvm::Value *callFLog(llvm::Value *val);
   llvm::Value *callPow(llvm::Value *val1, llvm::Value *val2);

   llvm::Value *vectorFromVals(llvm::Value *x, llvm::Value *y,
                               llvm::Value *z, llvm::Value *w=0);

   llvm::Function *declarePrintf();
private:
   llvm::Module *m_mod;
   llvm::Function *m_func;
   char        m_name[32];
   llvm::BasicBlock *m_block;
   int               m_idx;

   llvm::VectorType *m_floatVecType;

   llvm::Function   *m_llvmFSqrt;
   llvm::Function   *m_llvmFAbs;
   llvm::Function   *m_llvmPow;
   llvm::Function   *m_llvmFloor;
   llvm::Function   *m_llvmFlog;
   llvm::Function   *m_llvmLit;

   llvm::Constant   *m_fmtPtr;
};

#endif

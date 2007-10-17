#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include <llvm/BasicBlock.h>
#include <llvm/Module.h>
#include <llvm/Value.h>

namespace llvm {
   class VectorType;
}

class Instructions
{
public:
   Instructions(llvm::Module *mod, llvm::BasicBlock *block);

   llvm::Value *add(llvm::Value *in1, llvm::Value *in2);
   llvm::Value *dp3(llvm::Value *in1, llvm::Value *in2);
   llvm::Value *lit(llvm::Value *in1);
   llvm::Value *madd(llvm::Value *in1, llvm::Value *in2,
                     llvm::Value *in2);
   llvm::Value *mul(llvm::Value *in1, llvm::Value *in2);
   llvm::Value *rsq(llvm::Value *in1);
private:
   const char *name(const char *prefix);

   llvm::Value *callFSqrt(llvm::Value *val);
   llvm::Value *callFAbs(llvm::Value *val);

   llvm::Value *vectorFromVals(llvm::Value *x, llvm::Value *y,
                               llvm::Value *z, llvm::Value *w=0);
private:
   llvm::Module *m_mod;
   char        m_name[32];
   llvm::BasicBlock *m_block;
   int               m_idx;
   llvm::Function   *m_llvmFSqrt;
   llvm::Function   *m_llvmFAbs;

   llvm::VectorType *m_floatVecType;
};

#endif

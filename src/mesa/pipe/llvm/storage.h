#ifndef STORAGE_H
#define STORAGE_H

#include <map>
#include <vector>

namespace llvm {
   class BasicBlock;
   class Constant;
   class ConstantInt;
   class LoadInst;
   class Value;
   class VectorType;
}

class Storage
{
   typedef std::map<int, llvm::LoadInst*> LoadMap;
public:
   Storage(llvm::BasicBlock *block,
           llvm::Value *out,
           llvm::Value *in, llvm::Value *consts);

   llvm::ConstantInt *constantInt(int);
   llvm::Constant *shuffleMask(int vec);
   llvm::Value *inputElement(int idx);
   llvm::Value *constElement(int idx);

   llvm::Value *tempElement(int idx) const;
   void setTempElement(int idx, llvm::Value *val);

   llvm::Value *shuffleVector(llvm::Value *vec, int shuffle);


   void store(int dstIdx, llvm::Value *val);
private:
   llvm::BasicBlock *m_block;
   llvm::Value *m_OUT;
   llvm::Value *m_IN;
   llvm::Value *m_CONST;

   std::map<int, llvm::ConstantInt*> m_constInts;
   std::map<int, llvm::Constant*>    m_intVecs;
   std::vector<llvm::Value*>         m_temps;
   LoadMap                           m_inputs;
   LoadMap                           m_consts;

   llvm::VectorType *m_floatVecType;
   llvm::VectorType *m_intVecType;

   llvm::Value      *m_undefFloatVec;
   llvm::Value      *m_undefIntVec;

   int         m_shuffleId;
};

#endif

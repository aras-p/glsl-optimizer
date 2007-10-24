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

   void setCurrentBlock(llvm::BasicBlock *block);

   llvm::ConstantInt *constantInt(int);
   llvm::Constant *shuffleMask(int vec);
   llvm::Value *inputElement(int idx, llvm::Value *indIdx =0);
   llvm::Value *constElement(int idx, llvm::Value *indIdx =0);

   llvm::Value *tempElement(int idx) const;
   void setTempElement(int idx, llvm::Value *val, int mask);

   llvm::Value *addrElement(int idx) const;
   void setAddrElement(int idx, llvm::Value *val, int mask);

   llvm::Value *shuffleVector(llvm::Value *vec, int shuffle);

   llvm::Value *extractIndex(llvm::Value *vec);

   void store(int dstIdx, llvm::Value *val, int mask);

   void popPhiNode();

   int numConsts() const;
private:
   llvm::Value *maskWrite(llvm::Value *src, int mask, llvm::Value *templ);
   const char *name(const char *prefix);
private:
   llvm::BasicBlock *m_block;
   llvm::Value *m_OUT;
   llvm::Value *m_IN;
   llvm::Value *m_CONST;

   std::map<int, llvm::ConstantInt*> m_constInts;
   std::map<int, llvm::Constant*>    m_intVecs;
   std::vector<llvm::Value*>         m_temps;
   std::vector<llvm::Value*>         m_addrs;
   std::vector<llvm::Value*>         m_dstCache;
   LoadMap                           m_inputs;
   LoadMap                           m_consts;

   llvm::VectorType *m_floatVecType;
   llvm::VectorType *m_intVecType;

   llvm::Value      *m_undefFloatVec;
   llvm::Value      *m_undefIntVec;

   llvm::Value      *m_extSwizzleVec;

   char        m_name[32];
   int         m_idx;

   int         m_numConsts;

   void addPhiNode(int, llvm::Value*, llvm::BasicBlock*,
                   llvm::Value*, llvm::BasicBlock*);
   void updatePhiNode(int, llvm::Value*);
   struct PhiNode {
      llvm::Value      *val1;
      llvm::BasicBlock *block1;
      llvm::Value      *val2;
      llvm::BasicBlock *block2;
   };

   std::map<llvm::Value*, llvm::BasicBlock*> m_varBlocks;
   std::map<int, PhiNode> m_phiNodes;
};

#endif

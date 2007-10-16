
#include <map>

class VertexShaderBuilder
{
   typedef std::map<int, llvm::LoadInst*> LoadMap;
public:
   VertexShaderBuilder(llvm::BasicBlock *block, llvm::Value *in, llvm::Value *consts);

   llvm::ConstantInt *constantInt(int);
   llvm::Constant *shuffleMask(int vec);
   llvm::Value *inputElement(int idx);
   llvm::Value *constElement(int idx);

   llvm::Value *shuffleVector(llvm::Value *vec, int shuffle);


private:
   llvm::BasicBlock *m_block;
   llvm::Value *m_IN;
   llvm::Value *m_CONST;

   std::map<int, llvm::ConstantInt*> m_constInts;
   std::map<int, llvm::Constant*> m_intVecs;
   LoadMap    m_inputs;
   LoadMap    m_consts;

   VectorType *m_floatVecType;
   VectorType *m_intVecType;

   Value      *m_undefFloatVec;
   Value      *m_undefIntVec;

   int         m_shuffleId;
};

VertexShaderBuilder::VertexShaderBuilder(llvm::BasicBlock *block, llvm::Value *in, llvm::Value *consts)
   : m_block(block), m_IN(in), m_CONST(consts)
{
   m_floatVecType = VectorType::get(Type::FloatTy, 4);
   m_intVecType   = VectorType::get(IntegerType::get(32), 4);

   m_undefFloatVec = UndefValue::get(m_floatVecType);
   m_undefIntVec   = UndefValue::get(m_intVecType);

   m_shuffleId = 0;
}

//can only build vectors with all members in the [0, 9] range
llvm::Constant *VertexShaderBuilder::shuffleMask(int vec)
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

llvm::ConstantInt *VertexShaderBuilder::constantInt(int idx)
{
   if (m_constInts.find(idx) != m_constInts.end()) {
      return m_constInts[idx];
   }
   ConstantInt *const_int = ConstantInt::get(APInt(32,  idx));
   m_constInts[idx] = const_int;
   return const_int;
}

llvm::Value *VertexShaderBuilder::inputElement(int idx)
{
   if (m_inputs.find(idx) != m_inputs.end()) {
      return m_inputs[idx];
   }
   char ptrName[13];
   char name[9];
   snprintf(ptrName, 13, "input_ptr%d", idx);
   snprintf(name, 9, "input%d", idx);
   GetElementPtrInst *getElem = new GetElementPtrInst(m_IN,
                                                      constantInt(idx),
                                                      ptrName,
                                                      m_block);
   LoadInst *load = new LoadInst(getElem, name,
                                 false, m_block);
   m_inputs[idx] = load;
   return load;
}

llvm::Value *VertexShaderBuilder::constElement(int idx)
{
   if (m_consts.find(idx) != m_consts.end()) {
      return m_consts[idx];
   }
   char ptrName[13];
   char name[9];
   snprintf(ptrName, 13, "const_ptr%d", idx);
   snprintf(name, 9, "const%d", idx);
   GetElementPtrInst *getElem = new GetElementPtrInst(m_CONST,
                                                      constantInt(idx),
                                                      ptrName,
                                                      m_block);
   LoadInst *load = new LoadInst(getElem, name,
                                 false, m_block);
   m_consts[idx] = load;
   return load;
}

llvm::Value *VertexShaderBuilder::shuffleVector(llvm::Value *vec, int shuffle)
{
   Constant *mask = shuffleMask(shuffle);
   ++m_shuffleId;
   char name[11];
   snprintf(name, 11, "shuffle%d", m_shuffleId);
   ShuffleVectorInst *res =
      new ShuffleVectorInst(vec, m_undefFloatVec, mask,
                            name, m_block);
   return res;
}

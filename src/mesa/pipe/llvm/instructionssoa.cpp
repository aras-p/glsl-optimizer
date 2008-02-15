#include "instructionssoa.h"

#include "storagesoa.h"

#include <llvm/Constants.h>

using namespace llvm;

InstructionsSoa::InstructionsSoa(llvm::Module *mod, llvm::Function *func,
                                 llvm::BasicBlock *block, StorageSoa *storage)
   : m_builder(block),
     m_storage(storage),
     m_idx(0)
{
}

const char * InstructionsSoa::name(const char *prefix) const
{
   ++m_idx;
   snprintf(m_name, 32, "%s%d", prefix, m_idx);
   return m_name;
}

llvm::Value * InstructionsSoa::vectorFromVals(llvm::Value *x, llvm::Value *y,
                                              llvm::Value *z, llvm::Value *w)
{
   VectorType  *vectorType = VectorType::get(Type::FloatTy, 4);
   Constant *constVector = Constant::getNullValue(vectorType);
   Value *res = m_builder.CreateInsertElement(constVector, x,
                                              m_storage->constantInt(0),
                                              name("vecx"));
   res = m_builder.CreateInsertElement(res, y, m_storage->constantInt(1),
                               name("vecxy"));
   res = m_builder.CreateInsertElement(res, z, m_storage->constantInt(2),
                               name("vecxyz"));
   if (w)
      res = m_builder.CreateInsertElement(res, w, m_storage->constantInt(3),
                                          name("vecxyzw"));
   return res;
}

std::vector<llvm::Value*> InstructionsSoa::arl(const std::vector<llvm::Value*> in)
{
   std::vector<llvm::Value*> res(4);

   //Extract the first x (all 4 should be the same)
   llvm::Value *x = m_builder.CreateExtractElement(in[0],
                                                   m_storage->constantInt(0),
                                                   name("extractX"));
   //cast it to an unsigned int
   x = m_builder.CreateFPToUI(x, IntegerType::get(32), name("xIntCast"));

   res[0] = x;
   //only x is valid. the others shouldn't be necessary
   /*
   res[1] = Constant::getNullValue(m_floatVecType);
   res[2] = Constant::getNullValue(m_floatVecType);
   res[3] = Constant::getNullValue(m_floatVecType);
   */

   return res;
}


std::vector<llvm::Value*> InstructionsSoa::add(const std::vector<llvm::Value*> in1,
                                               const std::vector<llvm::Value*> in2)
{
   std::vector<llvm::Value*> res(4);

   res[0] = m_builder.CreateAdd(in1[0], in2[0], name("addx"));
   res[1] = m_builder.CreateAdd(in1[1], in2[1], name("addy"));
   res[2] = m_builder.CreateAdd(in1[2], in2[2], name("addz"));
   res[3] = m_builder.CreateAdd(in1[3], in2[3], name("addw"));

   return res;
}

std::vector<llvm::Value*> InstructionsSoa::mul(const std::vector<llvm::Value*> in1,
                                               const std::vector<llvm::Value*> in2)
{
   std::vector<llvm::Value*> res(4);

   res[0] = m_builder.CreateMul(in1[0], in2[0], name("mulx"));
   res[1] = m_builder.CreateMul(in1[1], in2[1], name("muly"));
   res[2] = m_builder.CreateMul(in1[2], in2[2], name("mulz"));
   res[3] = m_builder.CreateMul(in1[3], in2[3], name("mulw"));

   return res;
}

void InstructionsSoa::end()
{
   m_builder.CreateRetVoid();
}

std::vector<llvm::Value*> InstructionsSoa::madd(const std::vector<llvm::Value*> in1,
                                                const std::vector<llvm::Value*> in2,
                                                const std::vector<llvm::Value*> in3)
{
   std::vector<llvm::Value*> res = mul(in1, in2);
   return add(res, in3);
}

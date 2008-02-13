#include "instructionssoa.h"

InstructionsSoa::InstructionsSoa(llvm::Module *mod, llvm::Function *func,
                                 llvm::BasicBlock *block, StorageSoa *storage)
   : m_builder(block),
     m_idx(0)
{
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

const char * InstructionsSoa::name(const char *prefix) const
{
   ++m_idx;
   snprintf(m_name, 32, "%s%d", prefix, m_idx);
   return m_name;
}

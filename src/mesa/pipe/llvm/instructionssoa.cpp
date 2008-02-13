#include "instructionssoa.h"

InstructionsSoa::InstructionsSoa(llvm::Module *mod, llvm::Function *func,
                                 llvm::BasicBlock *block, StorageSoa *storage)
   : m_builder(block)
{
}

std::vector<llvm::Value*> InstructionsSoa::add(const std::vector<llvm::Value*> in1,
                                               const std::vector<llvm::Value*> in2)
{
   std::vector<llvm::Value*> res(4);

   return res;
}

std::vector<llvm::Value*> InstructionsSoa::mul(const std::vector<llvm::Value*> in1,
                                               const std::vector<llvm::Value*> in2)
{
   std::vector<llvm::Value*> res(4);

   return res;
}

void InstructionsSoa::end()
{
   m_builder.CreateRetVoid();
}

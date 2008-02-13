#include "loweringpass.h"

using namespace llvm;

char LoweringPass::ID = 0;
RegisterPass<LoweringPass> X("lowering", "Lowering Pass");

LoweringPass::LoweringPass()
   :  ModulePass((intptr_t)&ID)
{
}

bool LoweringPass::runOnModule(Module &m)
{
   llvm::cerr << "Hello: " << m.getModuleIdentifier() << "\n";
   return false;
}

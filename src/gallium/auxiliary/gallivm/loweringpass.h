#ifndef LOWERINGPASS_H
#define LOWERINGPASS_H

#include "llvm/Pass.h"
#include "llvm/Module.h"

struct LoweringPass : public llvm::ModulePass
{
   static char ID;
   LoweringPass();

   virtual bool runOnModule(llvm::Module &m);
};

#endif

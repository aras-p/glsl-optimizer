//===-- OpenCLUtils.h - TODO: Add brief description -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// TODO: Add full description
//
//===----------------------------------------------------------------------===//
#ifndef OPENCLUTILS_H
#define OPENCLUTILS_H

#include "llvm/Function.h"

#include <llvm/Module.h>

static bool isOpenCLKernel(const llvm::Function* fun)
{
  llvm::Module *mod = const_cast<llvm::Function*>(fun)->getParent();
  llvm::NamedMDNode * md = mod->getOrInsertNamedMetadata("opencl.kernels");

  if (!md or !md->getNumOperands())
  {
    return false;
  }

  for (int i = 0; i < int(md->getNumOperands()); i++)
  {
    if (!md->getOperand(i) or !md->getOperand(i)->getOperand(0))
    {
      continue;
    }
    
    assert(md->getOperand(i)->getNumOperands() == 1);

    if (md->getOperand(i)->getOperand(0)->getName() == fun->getName())
    {
      return true;
    }
  }

  return false;
}


#endif

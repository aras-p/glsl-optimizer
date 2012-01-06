//===-- R600KernelParameters.h - TODO: Add brief description -------===//
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

#ifndef KERNELPARAMETERS_H
#define KERNELPARAMETERS_H

#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/Function.h"
#include "llvm/Pass.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Value.h"

#include <vector>

llvm::FunctionPass* createR600KernelParametersPass(const llvm::TargetData* TD);


#endif

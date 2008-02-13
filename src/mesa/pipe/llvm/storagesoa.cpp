/**************************************************************************
 *
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "storagesoa.h"


#include "pipe/p_shader_tokens.h"
#include <llvm/BasicBlock.h>
#include <llvm/Module.h>
#include <llvm/Value.h>

#include <llvm/CallingConv.h>
#include <llvm/Constants.h>
#include <llvm/DerivedTypes.h>
#include <llvm/InstrTypes.h>
#include <llvm/Instructions.h>

using namespace llvm;

StorageSoa::StorageSoa(llvm::BasicBlock *block,
                       llvm::Value *input,
                       llvm::Value *output,
                       llvm::Value *consts)
{
}

void StorageSoa::addImmediate(float *vec)
{
}

llvm::Value *StorageSoa::addrElement(int idx) const
{
   return 0;
}

std::vector<llvm::Value*> StorageSoa::inputElement(int idx, int swizzle,
                                                   llvm::Value *indIdx)
{
   std::vector<llvm::Value*> res(4);

   return res;
}

std::vector<llvm::Value*> StorageSoa::constElement(int idx, int swizzle,
                                                   llvm::Value *indIdx)
{
   std::vector<llvm::Value*> res(4);

   return res;
}

std::vector<llvm::Value*> StorageSoa::outputElement(int idx, int swizzle,
                                                    llvm::Value *indIdx)
{
   std::vector<llvm::Value*> res(4);

   return res;
}

std::vector<llvm::Value*> StorageSoa::tempElement(int idx, int swizzle,
                                                  llvm::Value *indIdx)
{
   std::vector<llvm::Value*> res(4);

   return res;
}

std::vector<llvm::Value*> StorageSoa::immediateElement(int idx, int swizzle)
{
   std::vector<llvm::Value*> res(4);

   return res;
}

llvm::Value * StorageSoa::extractIndex(llvm::Value *vec)
{
   return 0;
}

void StorageSoa::storeOutput(int dstIdx, const std::vector<llvm::Value*> &val,
                             int mask)
{
}

void StorageSoa::storeTemp(int idx, const std::vector<llvm::Value*> &val,
                           int mask)
{
}

void StorageSoa::storeAddress(int idx, const std::vector<llvm::Value*> &val,
                              int mask)
{
}

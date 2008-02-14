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

#ifndef STORAGESOA_H
#define STORAGESOA_H

#include <vector>
#include <map>

namespace llvm {
   class BasicBlock;
   class Constant;
   class ConstantInt;
   class LoadInst;
   class Value;
   class VectorType;
   class Module;
}

class StorageSoa
{
public:
   enum Argument {
      Input,
      Output,
      Temp,
      Const,
      Immediate,
      Address
   };
public:
   StorageSoa(llvm::BasicBlock *block,
              llvm::Value *input,
              llvm::Value *output,
              llvm::Value *consts,
              llvm::Value *temps);


   std::vector<llvm::Value*> load(Argument type, int idx, int swizzle, 
                                  llvm::Value *indIdx =0);
   void store(Argument type, int idx, const std::vector<llvm::Value*> &val,
              int mask);

   void addImmediate(float *vec);

   llvm::Value  * addrElement(int idx) const;

   llvm::Value *extractIndex(llvm::Value *vec);
private:
   llvm::Value *elementPointer(llvm::Value *ptr, int index,
                               int channel) const;
   llvm::Value *element(llvm::Value *ptr, int index,
                        int channel) const;
   const char *name(const char *prefix) const;
   llvm::ConstantInt *constantInt(int) const;
   llvm::Value  *alignedArrayLoad(llvm::Value *val);
   llvm::Module *currentModule() const;
   llvm::Value  *createConstGlobalVector(float *vec);

   std::vector<llvm::Value*> inputElement(int idx, llvm::Value *indIdx =0);
   std::vector<llvm::Value*> constElement(int idx, llvm::Value *indIdx =0);
   std::vector<llvm::Value*> outputElement(int idx, llvm::Value *indIdx =0);
   std::vector<llvm::Value*> tempElement(int idx, llvm::Value *indIdx =0);
   std::vector<llvm::Value*> immediateElement(int idx);
private:
   llvm::BasicBlock *m_block;

   llvm::Value *m_input;
   llvm::Value *m_output;
   llvm::Value *m_consts;
   llvm::Value *m_temps;

   std::map<int, std::vector<llvm::Value*> > m_immediates;

   mutable std::map<int, llvm::ConstantInt*> m_constInts;
   mutable char        m_name[32];
   mutable int         m_idx;
};

#endif

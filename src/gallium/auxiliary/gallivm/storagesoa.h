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

#include <pipe/p_shader_tokens.h>

#include <vector>
#include <list>
#include <map>

namespace llvm {
   class BasicBlock;
   class Constant;
   class ConstantInt;
   class GlobalVariable;
   class LoadInst;
   class Value;
   class VectorType;
   class Module;
}

class StorageSoa
{
public:
   StorageSoa(llvm::BasicBlock *block,
              llvm::Value *input,
              llvm::Value *output,
              llvm::Value *consts,
              llvm::Value *temps);


   std::vector<llvm::Value*> load(enum tgsi_file_type type, int idx, int swizzle, 
                                  llvm::Value *indIdx =0);
   void store(enum tgsi_file_type type, int idx, const std::vector<llvm::Value*> &val,
              int mask);

   void addImmediate(float *vec);
   void declareImmediates();

   void addAddress(int idx);

   llvm::Value  * addrElement(int idx) const;

   llvm::ConstantInt *constantInt(int) const;
private:
   llvm::Value *elementPointer(llvm::Value *ptr, llvm::Value *indIdx,
                               int channel) const;
   llvm::Value *element(llvm::Value *ptr, llvm::Value *idx,
                        int channel) const;
   const char *name(const char *prefix) const;
   llvm::Value  *alignedArrayLoad(llvm::Value *val);
   llvm::Module *currentModule() const;
   llvm::Constant  *createConstGlobalVector(const std::vector<float> &vec);

   std::vector<llvm::Value*> inputElement(llvm::Value *indIdx);
   std::vector<llvm::Value*> constElement(llvm::Value *indIdx);
   std::vector<llvm::Value*> outputElement(llvm::Value *indIdx);
   std::vector<llvm::Value*> tempElement(llvm::Value *indIdx);
   std::vector<llvm::Value*> immediateElement(llvm::Value *indIdx);
private:
   llvm::BasicBlock *m_block;

   llvm::Value *m_input;
   llvm::Value *m_output;
   llvm::Value *m_consts;
   llvm::Value *m_temps;
   llvm::GlobalVariable *m_immediates;

   std::map<int, llvm::Value*> m_addresses;

   std::vector<std::vector<float> > m_immediatesToFlush;

   mutable std::map<int, llvm::ConstantInt*> m_constInts;
   mutable char        m_name[32];
   mutable int         m_idx;
};

#endif

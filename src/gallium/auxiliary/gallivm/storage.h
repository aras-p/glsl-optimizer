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

 /*
  * Authors:
  *   Zack Rusin zack@tungstengraphics.com
  */

#ifndef STORAGE_H
#define STORAGE_H

#include <map>
#include <set>
#include <stack>
#include <vector>

namespace llvm {
   class BasicBlock;
   class Constant;
   class ConstantInt;
   class LoadInst;
   class Value;
   class VectorType;
}

class Storage
{
public:
   Storage(llvm::BasicBlock *block,
           llvm::Value *input);

   llvm::Value *inputPtr() const;

   void setCurrentBlock(llvm::BasicBlock *block);

   llvm::ConstantInt *constantInt(int);
   llvm::Constant *shuffleMask(int vec);
   llvm::Value *inputElement(int idx, llvm::Value *indIdx =0);
   llvm::Value *constElement(int idx, llvm::Value *indIdx =0);
   llvm::Value *outputElement(int idx, llvm::Value *indIdx =0);
   llvm::Value *tempElement(int idx, llvm::Value *indIdx =0);
   llvm::Value *immediateElement(int idx);

   void setOutputElement(int dstIdx, llvm::Value *val, int mask);
   void setTempElement(int idx, llvm::Value *val, int mask);

   llvm::Value *addrElement(int idx) const;
   void setAddrElement(int idx, llvm::Value *val, int mask);

   void setKilElement(llvm::Value *val);

   llvm::Value *shuffleVector(llvm::Value *vec, int shuffle);

   llvm::Value *extractIndex(llvm::Value *vec);

   int numConsts() const;

   void pushArguments(llvm::Value *input);
   void popArguments();
   void pushTemps();
   void popTemps();

   void addImmediate(float *val);

private:
   llvm::Value *maskWrite(llvm::Value *src, int mask, llvm::Value *templ);
   const char *name(const char *prefix);

   enum Args {
      DestsArg   = 0,
      InputsArg  = 1,
      TempsArg   = 2,
      ConstsArg  = 3,
      KilArg     = 4
   };
   llvm::Value *elemPtr(Args arg);
   llvm::Value *elemIdx(llvm::Value *ptr, int idx,
                        llvm::Value *indIdx = 0);
   llvm::Value *element(Args arg, int idx, llvm::Value *indIdx = 0);

private:
   llvm::BasicBlock *m_block;
   llvm::Value *m_INPUT;

   std::map<int, llvm::ConstantInt*> m_constInts;
   std::map<int, llvm::Constant*>    m_intVecs;
   std::vector<llvm::Value*>         m_addrs;
   std::vector<llvm::Constant*>      m_immediates;

   llvm::VectorType *m_floatVecType;
   llvm::VectorType *m_intVecType;

   char        m_name[32];
   int         m_idx;

   int         m_numConsts;

   std::map<int, bool > m_destWriteMap;
   std::map<int, bool > m_tempWriteMap;

   llvm::Value      *m_undefFloatVec;
   llvm::Value      *m_undefIntVec;
   llvm::Value      *m_extSwizzleVec;

   std::stack<llvm::Value*> m_argStack;
   std::stack<std::vector<llvm::Value*> > m_tempStack;
};

#endif

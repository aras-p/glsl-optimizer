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

#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include <llvm/BasicBlock.h>
#include <llvm/Module.h>
#include <llvm/Value.h>
#include <llvm/Support/IRBuilder.h>

#include <map>
#include <stack>

namespace llvm {
   class VectorType;
   class Function;
}

class Storage;

class Instructions
{
public:
   Instructions(llvm::Module *mod, llvm::Function *func, llvm::BasicBlock *block,
                Storage *storage);

   llvm::BasicBlock *currentBlock() const;

   llvm::Value *abs(llvm::Value *in1);
   llvm::Value *arl(llvm::Value *in1);
   llvm::Value *add(llvm::Value *in1, llvm::Value *in2);
   void         beginLoop();
   void         bgnSub(unsigned);
   void         brk();
   void         cal(int label, llvm::Value *input);
   llvm::Value *cmp(llvm::Value *in1, llvm::Value *in2, llvm::Value *in3);
   llvm::Value *cos(llvm::Value *in);
   llvm::Value *cross(llvm::Value *in1, llvm::Value *in2);
   llvm::Value *dp3(llvm::Value *in1, llvm::Value *in2);
   llvm::Value *dp4(llvm::Value *in1, llvm::Value *in2);
   llvm::Value *dph(llvm::Value *in1, llvm::Value *in2);
   llvm::Value *dst(llvm::Value *in1, llvm::Value *in2);
   void         elseop();
   void         endif();
   void         endLoop();
   void         end();
   void         endSub();
   llvm::Value *ex2(llvm::Value *in);
   llvm::Value *floor(llvm::Value *in);
   llvm::Value *frc(llvm::Value *in);
   void         ifop(llvm::Value *in);
   llvm::Value *kil(llvm::Value *in);
   llvm::Value *lerp(llvm::Value *in1, llvm::Value *in2,
                     llvm::Value *in3);
   llvm::Value *lit(llvm::Value *in);
   llvm::Value *lg2(llvm::Value *in);
   llvm::Value *madd(llvm::Value *in1, llvm::Value *in2,
                     llvm::Value *in3);
   llvm::Value *min(llvm::Value *in1, llvm::Value *in2);
   llvm::Value *max(llvm::Value *in1, llvm::Value *in2);
   llvm::Value *mul(llvm::Value *in1, llvm::Value *in2);
   llvm::Value *pow(llvm::Value *in1, llvm::Value *in2);
   llvm::Value *rcp(llvm::Value *in);
   llvm::Value *rsq(llvm::Value *in);
   llvm::Value *scs(llvm::Value *in);
   llvm::Value *sge(llvm::Value *in1, llvm::Value *in2);
   llvm::Value *sgt(llvm::Value *in1, llvm::Value *in2);
   llvm::Value *sin(llvm::Value *in);
   llvm::Value *slt(llvm::Value *in1, llvm::Value *in2);
   llvm::Value *sub(llvm::Value *in1, llvm::Value *in2);
   llvm::Value *trunc(llvm::Value *in);

   void printVector(llvm::Value *val);
private:
   const char *name(const char *prefix);

   llvm::Value *callFAbs(llvm::Value *val);
   llvm::Value *callFloor(llvm::Value *val);
   llvm::Value *callFSqrt(llvm::Value *val);
   llvm::Value *callFLog(llvm::Value *val);
   llvm::Value *callPow(llvm::Value *val1, llvm::Value *val2);

   llvm::Value *vectorFromVals(llvm::Value *x, llvm::Value *y,
                               llvm::Value *z, llvm::Value *w=0);

   llvm::Value *constVector(float x, float y, float z, float w);

   llvm::Function *declarePrintf();
   llvm::Function *declareFunc(int label);

   llvm::Function *findFunction(int label);

   std::vector<llvm::Value*> extractVector(llvm::Value *vec);
private:
   llvm::Module             *m_mod;
   llvm::Function           *m_func;
   char                      m_name[32];
   llvm::IRBuilder           m_builder;
   int                       m_idx;

   llvm::VectorType *m_floatVecType;

   llvm::Function   *m_llvmFSqrt;
   llvm::Function   *m_llvmFAbs;
   llvm::Function   *m_llvmPow;
   llvm::Function   *m_llvmFloor;
   llvm::Function   *m_llvmFlog;
   llvm::Function   *m_llvmLit;

   llvm::Constant   *m_fmtPtr;

   std::stack<llvm::BasicBlock*> m_ifStack;
   struct Loop {
      llvm::BasicBlock *begin;
      llvm::BasicBlock *end;
   };
   std::stack<Loop> m_loopStack;
   std::map<int, llvm::Function*> m_functions;
   Storage *m_storage;
};

#endif

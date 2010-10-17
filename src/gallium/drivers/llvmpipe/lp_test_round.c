/**************************************************************************
 *
 * Copyright 2010 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


#include <stdlib.h>
#include <stdio.h>

#include "util/u_pointer.h"
#include "gallivm/lp_bld.h"
#include "gallivm/lp_bld_init.h"
#include "gallivm/lp_bld_arit.h"

#include <llvm-c/Analysis.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Transforms/Scalar.h>

#include "lp_test.h"


void
write_tsv_header(FILE *fp)
{
   fprintf(fp,
           "result\t"
           "format\n");

   fflush(fp);
}


#ifdef PIPE_ARCH_SSE

#define USE_SSE2
#include "sse_mathfun.h"

typedef __m128 (*test_round_t)(__m128);

typedef LLVMValueRef (*lp_func_t)(struct lp_build_context *, LLVMValueRef);


static LLVMValueRef
add_test(LLVMModuleRef module, const char *name, lp_func_t lp_func)
{
   LLVMTypeRef v4sf = LLVMVectorType(LLVMFloatType(), 4);
   LLVMTypeRef args[1] = { v4sf };
   LLVMValueRef func = LLVMAddFunction(module, name, LLVMFunctionType(v4sf, args, 1, 0));
   LLVMValueRef arg1 = LLVMGetParam(func, 0);
   LLVMBuilderRef builder = LLVMCreateBuilder();
   LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
   LLVMValueRef ret;
   struct lp_build_context bld;

   lp_build_context_init(&bld, builder, lp_float32_vec4_type());

   LLVMSetFunctionCallConv(func, LLVMCCallConv);

   LLVMPositionBuilderAtEnd(builder, block);

   ret = lp_func(&bld, arg1);

   LLVMBuildRet(builder, ret);
   LLVMDisposeBuilder(builder);
   return func;
}

static void
printv(char* string, v4sf value)
{
   v4sf v = value;
   float *f = (float *)&v;
   printf("%s: %10f %10f %10f %10f\n", string,
           f[0], f[1], f[2], f[3]);
}

static boolean
compare(v4sf x, v4sf y)
{
   boolean success = TRUE;
   float *xp = (float *) &x;
   float *yp = (float *) &y;
   if (xp[0] != yp[0] ||
       xp[1] != yp[1] ||
       xp[2] != yp[2] ||
       xp[3] != yp[3]) {
      printf(" Incorrect result! ^^^^^^^^^^^^^^^^^^^^^^^^^^^^ \n");
      success = FALSE;
   }
   return success;
}



PIPE_ALIGN_STACK
static boolean
test_round(unsigned verbose, FILE *fp)
{
   LLVMModuleRef module = NULL;
   LLVMValueRef test_round = NULL, test_trunc, test_floor, test_ceil;
   LLVMExecutionEngineRef engine = lp_build_engine;
   LLVMPassManagerRef pass = NULL;
   char *error = NULL;
   test_round_t round_func, trunc_func, floor_func, ceil_func;
   float unpacked[4];
   unsigned packed;
   boolean success = TRUE;
   int i;

   module = LLVMModuleCreateWithName("test");

   test_round = add_test(module, "round", lp_build_round);
   test_trunc = add_test(module, "trunc", lp_build_trunc);
   test_floor = add_test(module, "floor", lp_build_floor);
   test_ceil = add_test(module, "ceil", lp_build_ceil);

   if(LLVMVerifyModule(module, LLVMPrintMessageAction, &error)) {
      printf("LLVMVerifyModule: %s\n", error);
      LLVMDumpModule(module);
      abort();
   }
   LLVMDisposeMessage(error);

#if 0
   pass = LLVMCreatePassManager();
   LLVMAddTargetData(LLVMGetExecutionEngineTargetData(engine), pass);
   /* These are the passes currently listed in llvm-c/Transforms/Scalar.h,
    * but there are more on SVN. */
   LLVMAddConstantPropagationPass(pass);
   LLVMAddInstructionCombiningPass(pass);
   LLVMAddPromoteMemoryToRegisterPass(pass);
   LLVMAddGVNPass(pass);
   LLVMAddCFGSimplificationPass(pass);
   LLVMRunPassManager(pass, module);
#else
   (void)pass;
#endif

   round_func = (test_round_t) pointer_to_func(LLVMGetPointerToGlobal(engine, test_round));
   trunc_func = (test_round_t) pointer_to_func(LLVMGetPointerToGlobal(engine, test_trunc));
   floor_func = (test_round_t) pointer_to_func(LLVMGetPointerToGlobal(engine, test_floor));
   ceil_func = (test_round_t) pointer_to_func(LLVMGetPointerToGlobal(engine, test_ceil));

   memset(unpacked, 0, sizeof unpacked);
   packed = 0;

   if (0)
      LLVMDumpModule(module);

   for (i = 0; i < 3; i++) {
      v4sf xvals[3] = {
         {-10.0, -1, 0, 12.0},
         {-1.5, -0.25, 1.25, 2.5},
         {-0.99, -0.01, 0.01, 0.99}
      };
      v4sf x = xvals[i];
      v4sf y, ref;
      float *xp = (float *) &x;
      float *refp = (float *) &ref;

      printf("\n");
      printv("x            ", x);

      refp[0] = round(xp[0]);
      refp[1] = round(xp[1]);
      refp[2] = round(xp[2]);
      refp[3] = round(xp[3]);
      y = round_func(x);
      printv("C round(x)   ", ref);
      printv("LLVM round(x)", y);
      success = success && compare(ref, y);

      refp[0] = trunc(xp[0]);
      refp[1] = trunc(xp[1]);
      refp[2] = trunc(xp[2]);
      refp[3] = trunc(xp[3]);
      y = trunc_func(x);
      printv("C trunc(x)   ", ref);
      printv("LLVM trunc(x)", y);
      success = success && compare(ref, y);

      refp[0] = floor(xp[0]);
      refp[1] = floor(xp[1]);
      refp[2] = floor(xp[2]);
      refp[3] = floor(xp[3]);
      y = floor_func(x);
      printv("C floor(x)   ", ref);
      printv("LLVM floor(x)", y);
      success = success && compare(ref, y);

      refp[0] = ceil(xp[0]);
      refp[1] = ceil(xp[1]);
      refp[2] = ceil(xp[2]);
      refp[3] = ceil(xp[3]);
      y = ceil_func(x);
      printv("C ceil(x)    ", ref);
      printv("LLVM ceil(x) ", y);
      success = success && compare(ref, y);
   }

   LLVMFreeMachineCodeForFunction(engine, test_round);
   LLVMFreeMachineCodeForFunction(engine, test_trunc);
   LLVMFreeMachineCodeForFunction(engine, test_floor);
   LLVMFreeMachineCodeForFunction(engine, test_ceil);

   LLVMDisposeExecutionEngine(engine);
   if(pass)
      LLVMDisposePassManager(pass);

   return success;
}

#else /* !PIPE_ARCH_SSE */

static boolean
test_round(unsigned verbose, FILE *fp)
{
   return TRUE;
}

#endif /* !PIPE_ARCH_SSE */


boolean
test_all(unsigned verbose, FILE *fp)
{
   return test_round(verbose, fp);
}


boolean
test_some(unsigned verbose, FILE *fp, unsigned long n)
{
   return test_all(verbose, fp);
}

boolean
test_single(unsigned verbose, FILE *fp)
{
   printf("no test_single()");
   return TRUE;
}

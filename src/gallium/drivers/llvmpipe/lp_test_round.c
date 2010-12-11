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
add_test(struct gallivm_state *gallivm, const char *name, lp_func_t lp_func)
{
   LLVMModuleRef module = gallivm->module;
   LLVMContextRef context = gallivm->context;
   LLVMBuilderRef builder = gallivm->builder;

   LLVMTypeRef v4sf = LLVMVectorType(LLVMFloatTypeInContext(context), 4);
   LLVMTypeRef args[1] = { v4sf };
   LLVMValueRef func = LLVMAddFunction(module, name, LLVMFunctionType(v4sf, args, 1, 0));
   LLVMValueRef arg1 = LLVMGetParam(func, 0);
   LLVMBasicBlockRef block = LLVMAppendBasicBlockInContext(context, func, "entry");
   LLVMValueRef ret;
   struct lp_build_context bld;

   lp_build_context_init(&bld, gallivm, lp_float32_vec4_type());

   LLVMSetFunctionCallConv(func, LLVMCCallConv);

   LLVMPositionBuilderAtEnd(builder, block);

   ret = lp_func(&bld, arg1);

   LLVMBuildRet(builder, ret);

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
test_round(struct gallivm_state *gallivm, unsigned verbose, FILE *fp)
{
   LLVMModuleRef module = gallivm->module;
   LLVMValueRef test_round = NULL, test_trunc, test_floor, test_ceil;
   LLVMExecutionEngineRef engine = gallivm->engine;
   char *error = NULL;
   test_round_t round_func, trunc_func, floor_func, ceil_func;
   float unpacked[4];
   unsigned packed;
   boolean success = TRUE;
   int i;

   test_round = add_test(gallivm, "round", lp_build_round);
   test_trunc = add_test(gallivm, "trunc", lp_build_trunc);
   test_floor = add_test(gallivm, "floor", lp_build_floor);
   test_ceil = add_test(gallivm, "ceil", lp_build_ceil);

   if(LLVMVerifyModule(module, LLVMPrintMessageAction, &error)) {
      printf("LLVMVerifyModule: %s\n", error);
      LLVMDumpModule(module);
      abort();
   }
   LLVMDisposeMessage(error);

   round_func = (test_round_t) pointer_to_func(LLVMGetPointerToGlobal(engine, test_round));
   trunc_func = (test_round_t) pointer_to_func(LLVMGetPointerToGlobal(engine, test_trunc));
   floor_func = (test_round_t) pointer_to_func(LLVMGetPointerToGlobal(engine, test_floor));
   ceil_func = (test_round_t) pointer_to_func(LLVMGetPointerToGlobal(engine, test_ceil));

   memset(unpacked, 0, sizeof unpacked);
   packed = 0;

   if (0)
      LLVMDumpModule(module);

   for (i = 0; i < 3; i++) {
      /* NOTE: There are several acceptable rules for x.5 rounding: ceiling,
       * nearest even, etc. So we avoid testing such corner cases here.
       */
      v4sf xvals[3] = {
         {-10.0, -1, 0, 12.0},
         {-1.49, -0.25, 1.25, 2.51},
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

   return success;
}

#else /* !PIPE_ARCH_SSE */

static boolean
test_round(struct gallivm_state *gallivm, unsigned verbose, FILE *fp)
{
   return TRUE;
}

#endif /* !PIPE_ARCH_SSE */


boolean
test_all(struct gallivm_state *gallivm, unsigned verbose, FILE *fp)
{
   return test_round(gallivm, verbose, fp);
}


boolean
test_some(struct gallivm_state *gallivm, unsigned verbose, FILE *fp,
          unsigned long n)
{
   return test_all(gallivm, verbose, fp);
}

boolean
test_single(struct gallivm_state *gallivm, unsigned verbose, FILE *fp)
{
   printf("no test_single()");
   return TRUE;
}

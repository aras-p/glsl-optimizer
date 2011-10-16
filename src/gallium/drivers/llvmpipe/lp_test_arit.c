/**************************************************************************
 *
 * Copyright 2011 VMware, Inc.
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


#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "util/u_pointer.h"
#include "util/u_memory.h"
#include "util/u_math.h"

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


typedef float (*unary_func_t)(float);


/**
 * Describe a test case of one unary function.
 */
struct unary_test_t
{
   /*
    * Test name -- name of the mathematical function under test.
    */

   const char *name;

   LLVMValueRef
   (*builder)(struct lp_build_context *bld, LLVMValueRef a);

   /*
    * Reference (pure-C) function.
    */
   float
   (*ref)(float a);

   /*
    * Test values.
    */
   const float *values;
   unsigned num_values;
};


const float exp2_values[] = {
   -60,
   -4,
   -2,
   -1,
   -1e-007,
   0,
   1e-007,
   0.01,
   0.1,
   0.9,
   0.99,
   1, 
   2, 
   4, 
   60
};


const float log2_values[] = {
#if 0
   /* 
    * Smallest denormalized number; meant just for experimentation, but not
    * validation.
    */
   1.4012984643248171e-45,
#endif
   1e-007,
   0.1,
   0.5,
   0.99,
   1,
   1.01,
   1.1,
   1.9,
   1.99,
   2,
   4,
   100000,
   1e+018
};


static float rsqrtf(float x)
{
   return 1.0/sqrt(x);
}


const float rsqrt_values[] = {
   -1, -1e-007,
   1e-007, 1,
   -4, -1,
   1, 4,
   -1e+035, -100000,
   100000, 1e+035,
};


const float sincos_values[] = {
   -5*M_PI/4,
   -4*M_PI/4,
   -4*M_PI/4,
   -3*M_PI/4,
   -2*M_PI/4,
   -1*M_PI/4,
    1*M_PI/4,
    2*M_PI/4,
    3*M_PI/4,
    4*M_PI/4,
    5*M_PI/4,
};


/*
 * Unary test cases.
 */

static const struct unary_test_t unary_tests[] = {
   {"exp2", &lp_build_exp2, &exp2f, exp2_values, Elements(exp2_values)},
   {"log2", &lp_build_log2, &log2f, log2_values, Elements(log2_values)},
   {"exp", &lp_build_exp, &expf, exp2_values, Elements(exp2_values)},
   {"log", &lp_build_log, &logf, log2_values, Elements(log2_values)},
   {"rsqrt", &lp_build_rsqrt, &rsqrtf, rsqrt_values, Elements(rsqrt_values)},
   {"sin", &lp_build_sin, &sinf, sincos_values, Elements(sincos_values)},
   {"cos", &lp_build_cos, &cosf, sincos_values, Elements(sincos_values)},
};


/*
 * Build LLVM function that exercises the unary operator builder.
 */
static LLVMValueRef
build_unary_test_func(struct gallivm_state *gallivm,
                      LLVMModuleRef module,
                      LLVMContextRef context,
                      const struct unary_test_t *test)
{
   LLVMTypeRef i32t = LLVMInt32TypeInContext(context);
   LLVMTypeRef f32t = LLVMFloatTypeInContext(context);
   LLVMTypeRef v4f32t = LLVMVectorType(f32t, 4);
   LLVMTypeRef args[1] = { f32t };
   LLVMValueRef func = LLVMAddFunction(module, test->name, LLVMFunctionType(f32t, args, Elements(args), 0));
   LLVMValueRef arg1 = LLVMGetParam(func, 0);
   LLVMBuilderRef builder = gallivm->builder;
   LLVMBasicBlockRef block = LLVMAppendBasicBlockInContext(context, func, "entry");
   LLVMValueRef index0 = LLVMConstInt(i32t, 0, 0);
   LLVMValueRef ret;

   struct lp_build_context bld;

   lp_build_context_init(&bld, gallivm, lp_float32_vec4_type());

   LLVMSetFunctionCallConv(func, LLVMCCallConv);

   LLVMPositionBuilderAtEnd(builder, block);
   
   /* scalar to vector */
   arg1 = LLVMBuildInsertElement(builder, LLVMGetUndef(v4f32t), arg1, index0, "");

   ret = test->builder(&bld, arg1);
   
   /* vector to scalar */
   ret = LLVMBuildExtractElement(builder, ret, index0, "");

   LLVMBuildRet(builder, ret);
   return func;
}


/*
 * Test one LLVM unary arithmetic builder function.
 */
static boolean
test_unary(struct gallivm_state *gallivm, unsigned verbose, FILE *fp, const struct unary_test_t *test)
{
   LLVMModuleRef module = gallivm->module;
   LLVMValueRef test_func;
   LLVMExecutionEngineRef engine = gallivm->engine;
   LLVMContextRef context = gallivm->context;
   char *error = NULL;
   unary_func_t test_func_jit;
   boolean success = TRUE;
   int i;

   test_func = build_unary_test_func(gallivm, module, context, test);

   if (LLVMVerifyModule(module, LLVMPrintMessageAction, &error)) {
      printf("LLVMVerifyModule: %s\n", error);
      LLVMDumpModule(module);
      abort();
   }
   LLVMDisposeMessage(error);

   test_func_jit = (unary_func_t) pointer_to_func(LLVMGetPointerToGlobal(engine, test_func));

   for (i = 0; i < test->num_values; ++i) {
      float value = test->values[i];
      float ref = test->ref(value);
      float src = test_func_jit(value);

      double error = fabs(src - ref);
      double precision = error ? -log2(error/fabs(ref)) : FLT_MANT_DIG;

      bool pass = precision >= 20.0;

      if (isnan(ref)) {
         continue;
      }

      if (!pass || verbose) {
         printf("%s(%.9g): ref = %.9g, src = %.9g, precision = %f bits, %s\n",
               test->name, value, ref, src, precision,
               pass ? "PASS" : "FAIL");
      }

      if (!pass) {
         success = FALSE;
      }
   }

   LLVMFreeMachineCodeForFunction(engine, test_func);

   return success;
}


boolean
test_all(struct gallivm_state *gallivm, unsigned verbose, FILE *fp)
{
   boolean success = TRUE;
   int i;

   for (i = 0; i < Elements(unary_tests); ++i) {
      if (!test_unary(gallivm, verbose, fp, &unary_tests[i])) {
         success = FALSE;
      }
   }

   return success;
}


boolean
test_some(struct gallivm_state *gallivm, unsigned verbose, FILE *fp,
          unsigned long n)
{
   /*
    * Not randomly generated test cases, so test all.
    */

   return test_all(gallivm, verbose, fp);
}


boolean
test_single(struct gallivm_state *gallivm, unsigned verbose, FILE *fp)
{
   return TRUE;
}

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
#include "gallivm/lp_bld_debug.h"
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


typedef void (*unary_func_t)(float *out, const float *in);


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

   /*
    * Required precision in bits.
    */
   double precision;
};


static float negf(float x)
{
   return -x;
}


static float sgnf(float x)
{
   if (x > 0.0f) {
      return 1.0f;
   }
   if (x < 0.0f) {
      return -1.0f;
   }
   return 0.0f;
}


const float exp2_values[] = {
   -INFINITY,
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
   60,
   INFINITY,
   NAN
};


const float log2_values[] = {
#if 0
   /* 
    * Smallest denormalized number; meant just for experimentation, but not
    * validation.
    */
   1.4012984643248171e-45,
#endif
   -INFINITY,
   0,
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
   1e+018,
   INFINITY,
   NAN
};


static float rcpf(float x)
{
   return 1.0/x;
}


const float rcp_values[] = {
   -0.0, 0.0,
   -1.0, 1.0,
   -1e-007, 1e-007,
   -4.0, 4.0,
   -1e+035, -100000,
   100000, 1e+035,
   5.88e-39f, // denormal
#if (__STDC_VERSION__ >= 199901L)
   INFINITY, -INFINITY,
#endif
};


static float rsqrtf(float x)
{
   return 1.0/(float)sqrt(x);
}


const float rsqrt_values[] = {
   // http://msdn.microsoft.com/en-us/library/windows/desktop/bb147346.aspx
   0.0, // must yield infinity
   1.0, // must yield 1.0
   1e-007, 4.0,
   100000, 1e+035,
   5.88e-39f, // denormal
#if (__STDC_VERSION__ >= 199901L)
   INFINITY,
#endif
};


const float sincos_values[] = {
   -INFINITY,
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
   INFINITY,
   NAN
};

const float round_values[] = {
      -10.0, -1, 0.0, 12.0,
      -1.49, -0.25, 1.25, 2.51,
      -0.99, -0.01, 0.01, 0.99,
      1.401298464324817e-45f, // smallest denormal
      -1.401298464324817e-45f,
      1.62981451e-08f,
      -1.62981451e-08f,
      1.62981451e15f, // large number not representable as 32bit int
      -1.62981451e15f,
      FLT_EPSILON,
      -FLT_EPSILON,
      1.0f - 0.5f*FLT_EPSILON,
      -1.0f + FLT_EPSILON,
      FLT_MAX,
      -FLT_MAX
};

static float fractf(float x)
{
   x -= floorf(x);
   if (x >= 1.0f) {
      // clamp to the largest number smaller than one
      x = 1.0f - 0.5f*FLT_EPSILON;
   }
   return x;
}


const float fract_values[] = {
   // http://en.wikipedia.org/wiki/IEEE_754-1985#Examples
   0.0f,
   -0.0f,
   1.0f,
   -1.0f,
   0.5f,
   -0.5f,
   1.401298464324817e-45f, // smallest denormal
   -1.401298464324817e-45f,
   5.88e-39f, // middle denormal
   1.18e-38f, // largest denormal
   -1.18e-38f,
   -1.62981451e-08f,
   FLT_EPSILON,
   -FLT_EPSILON,
   1.0f - 0.5f*FLT_EPSILON,
   -1.0f + FLT_EPSILON,
   FLT_MAX,
   -FLT_MAX
};


/*
 * Unary test cases.
 */

static const struct unary_test_t
unary_tests[] = {
   {"neg", &lp_build_negate, &negf, exp2_values, Elements(exp2_values), 20.0 },
   {"exp2", &lp_build_exp2, &exp2f, exp2_values, Elements(exp2_values), 20.0 },
   {"log2", &lp_build_log2_safe, &log2f, log2_values, Elements(log2_values), 20.0 },
   {"exp", &lp_build_exp, &expf, exp2_values, Elements(exp2_values), 18.0 },
   {"log", &lp_build_log_safe, &logf, log2_values, Elements(log2_values), 20.0 },
   {"rcp", &lp_build_rcp, &rcpf, rcp_values, Elements(rcp_values), 20.0 },
   {"rsqrt", &lp_build_rsqrt, &rsqrtf, rsqrt_values, Elements(rsqrt_values), 20.0 },
   {"sin", &lp_build_sin, &sinf, sincos_values, Elements(sincos_values), 20.0 },
   {"cos", &lp_build_cos, &cosf, sincos_values, Elements(sincos_values), 20.0 },
   {"sgn", &lp_build_sgn, &sgnf, exp2_values, Elements(exp2_values), 20.0 },
   {"round", &lp_build_round, &roundf, round_values, Elements(round_values), 24.0 },
   {"trunc", &lp_build_trunc, &truncf, round_values, Elements(round_values), 24.0 },
   {"floor", &lp_build_floor, &floorf, round_values, Elements(round_values), 24.0 },
   {"ceil", &lp_build_ceil, &ceilf, round_values, Elements(round_values), 24.0 },
   {"fract", &lp_build_fract_safe, &fractf, fract_values, Elements(fract_values), 24.0 },
};


/*
 * Build LLVM function that exercises the unary operator builder.
 */
static LLVMValueRef
build_unary_test_func(struct gallivm_state *gallivm,
                      const struct unary_test_t *test)
{
   struct lp_type type = lp_type_float_vec(32, lp_native_vector_width);
   LLVMContextRef context = gallivm->context;
   LLVMModuleRef module = gallivm->module;
   LLVMTypeRef vf32t = lp_build_vec_type(gallivm, type);
   LLVMTypeRef args[2] = { LLVMPointerType(vf32t, 0), LLVMPointerType(vf32t, 0) };
   LLVMValueRef func = LLVMAddFunction(module, test->name,
                                       LLVMFunctionType(LLVMVoidTypeInContext(context),
                                                        args, Elements(args), 0));
   LLVMValueRef arg0 = LLVMGetParam(func, 0);
   LLVMValueRef arg1 = LLVMGetParam(func, 1);
   LLVMBuilderRef builder = gallivm->builder;
   LLVMBasicBlockRef block = LLVMAppendBasicBlockInContext(context, func, "entry");
   LLVMValueRef ret;

   struct lp_build_context bld;

   lp_build_context_init(&bld, gallivm, type);

   LLVMSetFunctionCallConv(func, LLVMCCallConv);

   LLVMPositionBuilderAtEnd(builder, block);
   
   arg1 = LLVMBuildLoad(builder, arg1, "");

   ret = test->builder(&bld, arg1);
   
   LLVMBuildStore(builder, ret, arg0);

   LLVMBuildRetVoid(builder);

   gallivm_verify_function(gallivm, func);

   return func;
}


/*
 * Test one LLVM unary arithmetic builder function.
 */
static boolean
test_unary(unsigned verbose, FILE *fp, const struct unary_test_t *test)
{
   struct gallivm_state *gallivm;
   LLVMValueRef test_func;
   unary_func_t test_func_jit;
   boolean success = TRUE;
   int i, j;
   int length = lp_native_vector_width / 32;
   float *in, *out;

   in = align_malloc(length * 4, length * 4);
   out = align_malloc(length * 4, length * 4);

   /* random NaNs or 0s could wreak havoc */
   for (i = 0; i < length; i++) {
      in[i] = 1.0;
   }

   gallivm = gallivm_create();

   test_func = build_unary_test_func(gallivm, test);

   gallivm_compile_module(gallivm);

   test_func_jit = (unary_func_t) gallivm_jit_function(gallivm, test_func);

   for (j = 0; j < (test->num_values + length - 1) / length; j++) {
      int num_vals = ((j + 1) * length <= test->num_values) ? length :
                                                              test->num_values % length;

      for (i = 0; i < num_vals; ++i) {
         in[i] = test->values[i+j*length];
      }

      test_func_jit(out, in);
      for (i = 0; i < num_vals; ++i) {
         float ref = test->ref(in[i]);
         double error, precision;
         bool pass;

         if (util_inf_sign(ref) && util_inf_sign(out[i]) == util_inf_sign(ref)) {
            error = 0;
         } else {
            error = fabs(out[i] - ref);
         }
         precision = error ? -log2(error/fabs(ref)) : FLT_MANT_DIG;

         pass = precision >= test->precision;

         if (isnan(ref)) {
            continue;
         }

         if (!pass || verbose) {
            printf("%s(%.9g): ref = %.9g, out = %.9g, precision = %f bits, %s\n",
                  test->name, in[i], ref, out[i], precision,
                  pass ? "PASS" : "FAIL");
         }

         if (!pass) {
            success = FALSE;
         }
      }
   }

   gallivm_free_function(gallivm, test_func, test_func_jit);

   gallivm_destroy(gallivm);

   align_free(in);
   align_free(out);

   return success;
}


boolean
test_all(unsigned verbose, FILE *fp)
{
   boolean success = TRUE;
   int i;

   for (i = 0; i < Elements(unary_tests); ++i) {
      if (!test_unary(verbose, fp, &unary_tests[i])) {
         success = FALSE;
      }
   }

   return success;
}


boolean
test_some(unsigned verbose, FILE *fp,
          unsigned long n)
{
   /*
    * Not randomly generated test cases, so test all.
    */

   return test_all(verbose, fp);
}


boolean
test_single(unsigned verbose, FILE *fp)
{
   return TRUE;
}

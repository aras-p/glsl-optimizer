/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.
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


/**
 * @file
 * Unit tests for blend LLVM IR generation
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 *
 * Blend computation code derived from code written by
 * @author Brian Paul <brian@vmware.com>
 */


#include <stdlib.h>
#include <stdio.h>
#include <float.h>

#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Transforms/Scalar.h>

#include "pipe/p_state.h"
#include "util/u_format.h"
#include "util/u_math.h"

#include "lp_bld.h"
#include "lp_bld_type.h"
#include "lp_bld_arit.h"


unsigned verbose = 0;


typedef int64_t (*blend_test_ptr_t)(const void *src, const void *dst, const void *con, void *res);


static LLVMValueRef
read_cycle_counter(LLVMBuilderRef builder)
{
   const char *name = "llvm.readcyclecounter";
   LLVMModuleRef module = LLVMGetGlobalParent(LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)));
   LLVMValueRef function;

   function = LLVMGetNamedFunction(module, name);
   if(!function) {
      LLVMTypeRef type = LLVMInt64Type();
      function = LLVMAddFunction(module, name, LLVMFunctionType(type, NULL, 0, 0));
      LLVMSetFunctionCallConv(function, LLVMCCallConv);
      LLVMSetLinkage(function, LLVMExternalLinkage);
   }
   assert(LLVMIsDeclaration(function));

   return LLVMBuildCall(builder, function, NULL, 0, "");
}


static LLVMValueRef
add_blend_test(LLVMModuleRef module,
               const struct pipe_blend_state *blend,
               union lp_type type)
{
   LLVMTypeRef ret_type;
   LLVMTypeRef vec_type;
   LLVMTypeRef args[4];
   LLVMValueRef func;
   LLVMValueRef src_ptr;
   LLVMValueRef dst_ptr;
   LLVMValueRef const_ptr;
   LLVMValueRef res_ptr;
   LLVMBasicBlockRef block;
   LLVMBuilderRef builder;
   LLVMValueRef src;
   LLVMValueRef dst;
   LLVMValueRef con;
   LLVMValueRef res;
   LLVMValueRef start_counter;
   LLVMValueRef end_counter;

   ret_type = LLVMInt64Type();
   vec_type = lp_build_vec_type(type);

   args[3] = args[2] = args[1] = args[0] = LLVMPointerType(vec_type, 0);
   func = LLVMAddFunction(module, "test", LLVMFunctionType(LLVMInt64Type(), args, 4, 0));
   LLVMSetFunctionCallConv(func, LLVMCCallConv);
   src_ptr = LLVMGetParam(func, 0);
   dst_ptr = LLVMGetParam(func, 1);
   const_ptr = LLVMGetParam(func, 2);
   res_ptr = LLVMGetParam(func, 3);

   block = LLVMAppendBasicBlock(func, "entry");
   builder = LLVMCreateBuilder();
   LLVMPositionBuilderAtEnd(builder, block);

   src = LLVMBuildLoad(builder, src_ptr, "src");
   dst = LLVMBuildLoad(builder, dst_ptr, "dst");
   con = LLVMBuildLoad(builder, const_ptr, "const");

   start_counter = read_cycle_counter(builder);

   res = lp_build_blend(builder, blend, type, src, dst, con, 3);

   LLVMSetValueName(res, "res");

   end_counter = read_cycle_counter(builder);

   LLVMBuildStore(builder, res, res_ptr);

   LLVMBuildRet(builder, LLVMBuildSub(builder, end_counter, start_counter, "cycles"));;

   LLVMDisposeBuilder(builder);
   return func;
}


static float
random_float(void)
{
    return (float)((double)random()/(double)RAND_MAX);
}


/** Add and limit result to ceiling of 1.0 */
#define ADD_SAT(R, A, B) \
do { \
   R = (A) + (B);  if (R > 1.0f) R = 1.0f; \
} while (0)

/** Subtract and limit result to floor of 0.0 */
#define SUB_SAT(R, A, B) \
do { \
   R = (A) - (B);  if (R < 0.0f) R = 0.0f; \
} while (0)


static void
compute_blend_ref_term(unsigned rgb_factor,
                       unsigned alpha_factor,
                       const float *factor,
                       const float *src, 
                       const float *dst, 
                       const float *con, 
                       float *term)
{
   float temp;

   switch (rgb_factor) {
   case PIPE_BLENDFACTOR_ONE:
      term[0] = factor[0]; /* R */
      term[1] = factor[1]; /* G */
      term[2] = factor[2]; /* B */
      break;
   case PIPE_BLENDFACTOR_SRC_COLOR:
      term[0] = factor[0] * src[0]; /* R */
      term[1] = factor[1] * src[1]; /* G */
      term[2] = factor[2] * src[2]; /* B */
      break;
   case PIPE_BLENDFACTOR_SRC_ALPHA:
      term[0] = factor[0] * src[3]; /* R */
      term[1] = factor[1] * src[3]; /* G */
      term[2] = factor[2] * src[3]; /* B */
      break;
   case PIPE_BLENDFACTOR_DST_COLOR:
      term[0] = factor[0] * dst[0]; /* R */
      term[1] = factor[1] * dst[1]; /* G */
      term[2] = factor[2] * dst[2]; /* B */
      break;
   case PIPE_BLENDFACTOR_DST_ALPHA:
      term[0] = factor[0] * dst[3]; /* R */
      term[1] = factor[1] * dst[3]; /* G */
      term[2] = factor[2] * dst[3]; /* B */
      break;
   case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
      temp = MIN2(src[3], 1.0f - dst[3]);
      term[0] = factor[0] * temp; /* R */
      term[1] = factor[1] * temp; /* G */
      term[2] = factor[2] * temp; /* B */
      break;
   case PIPE_BLENDFACTOR_CONST_COLOR:
      term[0] = factor[0] * con[0]; /* R */
      term[1] = factor[1] * con[1]; /* G */
      term[2] = factor[2] * con[2]; /* B */
      break;
   case PIPE_BLENDFACTOR_CONST_ALPHA:
      term[0] = factor[0] * con[3]; /* R */
      term[1] = factor[1] * con[3]; /* G */
      term[2] = factor[2] * con[3]; /* B */
      break;
   case PIPE_BLENDFACTOR_SRC1_COLOR:
      assert(0); /* to do */
      break;
   case PIPE_BLENDFACTOR_SRC1_ALPHA:
      assert(0); /* to do */
      break;
   case PIPE_BLENDFACTOR_ZERO:
      term[0] = 0.0f; /* R */
      term[1] = 0.0f; /* G */
      term[2] = 0.0f; /* B */
      break;
   case PIPE_BLENDFACTOR_INV_SRC_COLOR:
      term[0] = factor[0] * (1.0f - src[0]); /* R */
      term[1] = factor[1] * (1.0f - src[1]); /* G */
      term[2] = factor[2] * (1.0f - src[2]); /* B */
      break;
   case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
      term[0] = factor[0] * (1.0f - src[3]); /* R */
      term[1] = factor[1] * (1.0f - src[3]); /* G */
      term[2] = factor[2] * (1.0f - src[3]); /* B */
      break;
   case PIPE_BLENDFACTOR_INV_DST_ALPHA:
      term[0] = factor[0] * (1.0f - dst[3]); /* R */
      term[1] = factor[1] * (1.0f - dst[3]); /* G */
      term[2] = factor[2] * (1.0f - dst[3]); /* B */
      break;
   case PIPE_BLENDFACTOR_INV_DST_COLOR:
      term[0] = factor[0] * (1.0f - dst[0]); /* R */
      term[1] = factor[1] * (1.0f - dst[1]); /* G */
      term[2] = factor[2] * (1.0f - dst[2]); /* B */
      break;
   case PIPE_BLENDFACTOR_INV_CONST_COLOR:
      term[0] = factor[0] * (1.0f - con[0]); /* R */
      term[1] = factor[1] * (1.0f - con[1]); /* G */
      term[2] = factor[2] * (1.0f - con[2]); /* B */
      break;
   case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
      term[0] = factor[0] * (1.0f - con[3]); /* R */
      term[1] = factor[1] * (1.0f - con[3]); /* G */
      term[2] = factor[2] * (1.0f - con[3]); /* B */
      break;
   case PIPE_BLENDFACTOR_INV_SRC1_COLOR:
      assert(0); /* to do */
      break;
   case PIPE_BLENDFACTOR_INV_SRC1_ALPHA:
      assert(0); /* to do */
      break;
   default:
      assert(0);
   }

   /*
    * Compute src/first term A
    */
   switch (alpha_factor) {
   case PIPE_BLENDFACTOR_ONE:
      term[3] = factor[3]; /* A */
      break;
   case PIPE_BLENDFACTOR_SRC_COLOR:
   case PIPE_BLENDFACTOR_SRC_ALPHA:
      term[3] = factor[3] * src[3]; /* A */
      break;
   case PIPE_BLENDFACTOR_DST_COLOR:
   case PIPE_BLENDFACTOR_DST_ALPHA:
      term[3] = factor[3] * dst[3]; /* A */
      break;
   case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
      term[3] = src[3]; /* A */
      break;
   case PIPE_BLENDFACTOR_CONST_COLOR:
   case PIPE_BLENDFACTOR_CONST_ALPHA:
      term[3] = factor[3] * con[3]; /* A */
      break;
   case PIPE_BLENDFACTOR_ZERO:
      term[3] = 0.0f; /* A */
      break;
   case PIPE_BLENDFACTOR_INV_SRC_COLOR:
   case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
      term[3] = factor[3] * (1.0f - src[3]); /* A */
      break;
   case PIPE_BLENDFACTOR_INV_DST_COLOR:
   case PIPE_BLENDFACTOR_INV_DST_ALPHA:
      term[3] = factor[3] * (1.0f - dst[3]); /* A */
      break;
   case PIPE_BLENDFACTOR_INV_CONST_COLOR:
   case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
      term[3] = factor[3] * (1.0f - con[3]);
      break;
   default:
      assert(0);
   }
}


static void
compute_blend_ref(const struct pipe_blend_state *blend,
                  const float *src, 
                  const float *dst, 
                  const float *con, 
                  float *res)
{
   float src_term[4];
   float dst_term[4];

   compute_blend_ref_term(blend->rgb_src_factor, blend->alpha_src_factor, src, src, dst, con, src_term);
   compute_blend_ref_term(blend->rgb_dst_factor, blend->alpha_dst_factor, dst, src, dst, con, dst_term);

   /*
    * Combine RGB terms
    */
   switch (blend->rgb_func) {
   case PIPE_BLEND_ADD:
      ADD_SAT(res[0], src_term[0], dst_term[0]); /* R */
      ADD_SAT(res[1], src_term[1], dst_term[1]); /* G */
      ADD_SAT(res[2], src_term[2], dst_term[2]); /* B */
      break;
   case PIPE_BLEND_SUBTRACT:
      SUB_SAT(res[0], src_term[0], dst_term[0]); /* R */
      SUB_SAT(res[1], src_term[1], dst_term[1]); /* G */
      SUB_SAT(res[2], src_term[2], dst_term[2]); /* B */
      break;
   case PIPE_BLEND_REVERSE_SUBTRACT:
      SUB_SAT(res[0], dst_term[0], src_term[0]); /* R */
      SUB_SAT(res[1], dst_term[1], src_term[1]); /* G */
      SUB_SAT(res[2], dst_term[2], src_term[2]); /* B */
      break;
   case PIPE_BLEND_MIN:
      res[0] = MIN2(src_term[0], dst_term[0]); /* R */
      res[1] = MIN2(src_term[1], dst_term[1]); /* G */
      res[2] = MIN2(src_term[2], dst_term[2]); /* B */
      break;
   case PIPE_BLEND_MAX:
      res[0] = MAX2(src_term[0], dst_term[0]); /* R */
      res[1] = MAX2(src_term[1], dst_term[1]); /* G */
      res[2] = MAX2(src_term[2], dst_term[2]); /* B */
      break;
   default:
      assert(0);
   }

   /*
    * Combine A terms
    */
   switch (blend->alpha_func) {
   case PIPE_BLEND_ADD:
      ADD_SAT(res[3], src_term[3], dst_term[3]); /* A */
      break;
   case PIPE_BLEND_SUBTRACT:
      SUB_SAT(res[3], src_term[3], dst_term[3]); /* A */
      break;
   case PIPE_BLEND_REVERSE_SUBTRACT:
      SUB_SAT(res[3], dst_term[3], src_term[3]); /* A */
      break;
   case PIPE_BLEND_MIN:
      res[3] = MIN2(src_term[3], dst_term[3]); /* A */
      break;
   case PIPE_BLEND_MAX:
      res[3] = MAX2(src_term[3], dst_term[3]); /* A */
      break;
   default:
      assert(0);
   }
}


static boolean
test_one(const struct pipe_blend_state *blend,
         union lp_type type)
{
   LLVMModuleRef module = NULL;
   LLVMValueRef func = NULL;
   LLVMExecutionEngineRef engine = NULL;
   LLVMModuleProviderRef provider = NULL;
   LLVMPassManagerRef pass = NULL;
   char *error = NULL;
   blend_test_ptr_t blend_test_ptr;
   boolean success;
   const unsigned n = 32;
   int64_t cycles[n];
   unsigned i, j, k;

   module = LLVMModuleCreateWithName("test");

   func = add_blend_test(module, blend, type);

   if(LLVMVerifyModule(module, LLVMPrintMessageAction, &error)) {
      LLVMDumpModule(module);
      abort();
   }
   LLVMDisposeMessage(error);

   provider = LLVMCreateModuleProviderForExistingModule(module);
   if (LLVMCreateJITCompiler(&engine, provider, 1, &error)) {
      fprintf(stderr, "%s\n", error);
      LLVMDisposeMessage(error);
      abort();
   }

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

   blend_test_ptr = (blend_test_ptr_t)LLVMGetPointerToGlobal(engine, func);

   if(verbose >= 2)
      LLVMDumpModule(module);

   success = TRUE;
   for(i = 0; i < n && success; ++i) {
      if(type.floating && type.width == 32) {
         float src[LP_MAX_VECTOR_LENGTH];
         float dst[LP_MAX_VECTOR_LENGTH];
         float con[LP_MAX_VECTOR_LENGTH];
         float ref[LP_MAX_VECTOR_LENGTH];
         float res[LP_MAX_VECTOR_LENGTH];

         for(j = 0; j < type.length; ++j) {
            src[j] = random_float();
            dst[j] = random_float();
            con[j] = random_float();
         }

         for(j = 0; j < type.length; j += 4)
            compute_blend_ref(blend, src + j, dst + j, con + j, ref + j);

         cycles[i] = blend_test_ptr(src, dst, con, res);

         for(j = 0; j < type.length; ++j)
            if(fabs(res[j] - ref[j]) > FLT_EPSILON)
               success = FALSE;

         if (!success) {
            fprintf(stderr, "MISMATCH\n");
            fprintf(stderr, "  Result:   ");
            for(j = 0; j < type.length; ++j)
               fprintf(stderr, " %f", res[j]);
            fprintf(stderr, "\n");
            fprintf(stderr, "  Expected: ");
            for(j = 0; j < type.length; ++j)
               fprintf(stderr, " %f", ref[j]);
            fprintf(stderr, "\n");
         }
      }
      else if(!type.floating && !type.fixed && !type.sign && type.norm && type.width == 8) {
         uint8_t src[LP_MAX_VECTOR_LENGTH];
         uint8_t dst[LP_MAX_VECTOR_LENGTH];
         uint8_t con[LP_MAX_VECTOR_LENGTH];
         uint8_t ref[LP_MAX_VECTOR_LENGTH];
         uint8_t res[LP_MAX_VECTOR_LENGTH];

         for(j = 0; j < type.length; ++j) {
            src[j] = random() & 0xff;
            dst[j] = random() & 0xff;
            con[j] = random() & 0xff;
         }

         for(j = 0; j < type.length; j += 4) {
            float srcf[4];
            float dstf[4];
            float conf[4];
            float reff[4];

            for(k = 0; k < 4; ++k) {
               srcf[k] = (1.0f/255.0f)*src[j + k];
               dstf[k] = (1.0f/255.0f)*dst[j + k];
               conf[k] = (1.0f/255.0f)*con[j + k];
            }

            compute_blend_ref(blend, srcf, dstf, conf, reff);

            for(k = 0; k < 4; ++k)
               ref[j + k] = (uint8_t)(reff[k]*255.0f + 0.5f);
         }

         cycles[i] = blend_test_ptr(src, dst, con, res);

         for(j = 0; j < type.length; ++j) {
            int delta = (int)res[j] - (int)ref[j];
            if (delta < 0)
               delta = -delta;
            if(delta > 1)
               success = FALSE;
         }

         if (!success) {
            fprintf(stderr, "MISMATCH\n");
            fprintf(stderr, "  Result:   ");
            for(j = 0; j < type.length; ++j)
               fprintf(stderr, " %3u", res[j]);
            fprintf(stderr, "\n");
            fprintf(stderr, "  Expected: ");
            for(j = 0; j < type.length; ++j)
               fprintf(stderr, " %3u", ref[j]);
            fprintf(stderr, "\n");
         }
      }
      else
         assert(0);
   }

   /*
    * Unfortunately the output of cycle counter is not very reliable as it comes
    * -- sometimes we get outliers (due IRQs perhaps?) which are
    * better removed to avoid random or biased data.
    */
   if(verbose >=1 && success) {
      double sum = 0.0, sum2 = 0.0;
      double avg, std;
      unsigned m;

      for(i = 0; i < n; ++i) {
         sum += cycles[i];
         sum2 += cycles[i]*cycles[i];
      }

      avg = sum/n;
      std = sqrtf((sum2 - n*avg*avg)/n);

      m = 0;
      sum = 0.0;
      for(i = 0; i < n; ++i) {
         if(fabs(cycles[i] - avg) <= 4.0*std) {
            sum += cycles[i];
            ++m;
         }
      }

      avg = sum/m;

      fprintf(stdout, " cycles=%.1f", avg);
   }

   if(verbose >= 1) {
      fprintf(stdout, " result=%s\n", success ? "pass" : "fail");
      fflush(stdout);
   }

   if (!success) {
      LLVMDumpModule(module);
      LLVMWriteBitcodeToFile(module, "blend.bc");
      fprintf(stderr, "blend.bc written\n");
      abort();
   }

   LLVMFreeMachineCodeForFunction(engine, func);

   LLVMDisposeExecutionEngine(engine);
   if(pass)
      LLVMDisposePassManager(pass);

   return success;
}


struct value_name_pair
{
   unsigned value;
   const char *name;
};


const struct value_name_pair
blend_factors[] = {
   {PIPE_BLENDFACTOR_ZERO                , "zero"},
   {PIPE_BLENDFACTOR_ONE                 , "one"},
   {PIPE_BLENDFACTOR_SRC_COLOR           , "src_color"},
   {PIPE_BLENDFACTOR_SRC_ALPHA           , "src_alpha"},
   {PIPE_BLENDFACTOR_DST_COLOR           , "dst_color"},
   {PIPE_BLENDFACTOR_DST_ALPHA           , "dst_alpha"},
   {PIPE_BLENDFACTOR_CONST_COLOR         , "const_color"},
   {PIPE_BLENDFACTOR_CONST_ALPHA         , "const_alpha"},
#if 0
   {PIPE_BLENDFACTOR_SRC1_COLOR          , "src1_color"},
   {PIPE_BLENDFACTOR_SRC1_ALPHA          , "src1_alpha"},
#endif
   {PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE  , "src_alpha_saturate"},
   {PIPE_BLENDFACTOR_INV_SRC_COLOR       , "inv_src_color"},
   {PIPE_BLENDFACTOR_INV_SRC_ALPHA       , "inv_src_alpha"},
   {PIPE_BLENDFACTOR_INV_DST_COLOR       , "inv_dst_color"},
   {PIPE_BLENDFACTOR_INV_DST_ALPHA       , "inv_dst_alpha"},
   {PIPE_BLENDFACTOR_INV_CONST_COLOR     , "inv_const_color"},
   {PIPE_BLENDFACTOR_INV_CONST_ALPHA     , "inv_const_alpha"},
#if 0
   {PIPE_BLENDFACTOR_INV_SRC1_COLOR      , "inv_src1_color"},
   {PIPE_BLENDFACTOR_INV_SRC1_ALPHA      , "inv_src1_alpha"}
#endif
};


const struct value_name_pair
blend_funcs[] = {
   {PIPE_BLEND_ADD               , "add"},
   {PIPE_BLEND_SUBTRACT          , "sub"},
   {PIPE_BLEND_REVERSE_SUBTRACT  , "rev_sub"},
   {PIPE_BLEND_MIN               , "min"},
   {PIPE_BLEND_MAX               , "max"}
};


const union lp_type blend_types[] = {
   /* float, fixed,  sign,  norm, width, len */
   {{  TRUE, FALSE,  TRUE,  TRUE,    32,   4 }}, /* f32 x 4 */
   {{ FALSE, FALSE, FALSE,  TRUE,     8,  16 }}, /* u8n x 16 */
};


const unsigned num_funcs = sizeof(blend_funcs)/sizeof(blend_funcs[0]);
const unsigned num_factors = sizeof(blend_factors)/sizeof(blend_factors[0]);
const unsigned num_types = sizeof(blend_types)/sizeof(blend_types[0]);


static boolean 
test_all(void)
{
   const struct value_name_pair *rgb_func;
   const struct value_name_pair *rgb_src_factor;
   const struct value_name_pair *rgb_dst_factor;
   const struct value_name_pair *alpha_func;
   const struct value_name_pair *alpha_src_factor;
   const struct value_name_pair *alpha_dst_factor;
   struct pipe_blend_state blend;
   const union lp_type *type;
   bool success = TRUE;

   for(rgb_func = blend_funcs; rgb_func < &blend_funcs[num_funcs]; ++rgb_func) {
      for(alpha_func = blend_funcs; alpha_func < &blend_funcs[num_funcs]; ++alpha_func) {
         for(rgb_src_factor = blend_factors; rgb_src_factor < &blend_factors[num_factors]; ++rgb_src_factor) {
            for(rgb_dst_factor = blend_factors; rgb_dst_factor <= rgb_src_factor; ++rgb_dst_factor) {
               for(alpha_src_factor = blend_factors; alpha_src_factor < &blend_factors[num_factors]; ++alpha_src_factor) {
                  for(alpha_dst_factor = blend_factors; alpha_dst_factor <= alpha_src_factor; ++alpha_dst_factor) {
                     for(type = blend_types; type < &blend_types[num_types]; ++type) {

                        if(rgb_dst_factor->value == PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE ||
                           alpha_dst_factor->value == PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE)
                           continue;

                        if(verbose >= 1) {
                           fprintf(stdout,
                                   "%s=%s %s=%s %s=%s %s=%s %s=%s %s=%s",
                                   "rgb_func",         rgb_func->name,
                                   "rgb_src_factor",   rgb_src_factor->name,
                                   "rgb_dst_factor",   rgb_dst_factor->name,
                                   "alpha_func",       alpha_func->name,
                                   "alpha_src_factor", alpha_src_factor->name,
                                   "alpha_dst_factor", alpha_dst_factor->name);
                           fflush(stdout);
                        }

                        memset(&blend, 0, sizeof blend);
                        blend.blend_enable      = 1;
                        blend.rgb_func          = rgb_func->value;
                        blend.rgb_src_factor    = rgb_src_factor->value;
                        blend.rgb_dst_factor    = rgb_dst_factor->value;
                        blend.alpha_func        = alpha_func->value;
                        blend.alpha_src_factor  = alpha_src_factor->value;
                        blend.alpha_dst_factor  = alpha_dst_factor->value;

                        if(!test_one(&blend, *type))
                          success = FALSE;

                     }
                  }
               }
            }
         }
      }
   }

   return success;
}


static boolean 
test_some(unsigned long n)
{
   const struct value_name_pair *rgb_func;
   const struct value_name_pair *rgb_src_factor;
   const struct value_name_pair *rgb_dst_factor;
   const struct value_name_pair *alpha_func;
   const struct value_name_pair *alpha_src_factor;
   const struct value_name_pair *alpha_dst_factor;
   struct pipe_blend_state blend;
   const union lp_type *type;
   unsigned long i;
   bool success = TRUE;

   for(i = 0; i < n; ++i) {
      rgb_func = &blend_funcs[random() % num_funcs];
      alpha_func = &blend_funcs[random() % num_funcs];
      rgb_src_factor = &blend_factors[random() % num_factors];
      alpha_src_factor = &blend_factors[random() % num_factors];
      
      do {
         rgb_dst_factor = &blend_factors[random() % num_factors];
      } while(rgb_dst_factor->value == PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE);

      do {
         alpha_dst_factor = &blend_factors[random() % num_factors];
      } while(alpha_dst_factor->value == PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE);

      for(type = blend_types; type < &blend_types[num_types]; ++type) {

         if(verbose >= 1) {
            fprintf(stdout,
                    "%s=%s %s=%s %s=%s %s=%s %s=%s %s=%s",
                    "rgb_func",         rgb_func->name,
                    "rgb_src_factor",   rgb_src_factor->name,
                    "rgb_dst_factor",   rgb_dst_factor->name,
                    "alpha_func",       alpha_func->name,
                    "alpha_src_factor", alpha_src_factor->name,
                    "alpha_dst_factor", alpha_dst_factor->name);
            fflush(stdout);
         }

         memset(&blend, 0, sizeof blend);
         blend.blend_enable      = 1;
         blend.rgb_func          = rgb_func->value;
         blend.rgb_src_factor    = rgb_src_factor->value;
         blend.rgb_dst_factor    = rgb_dst_factor->value;
         blend.alpha_func        = alpha_func->value;
         blend.alpha_src_factor  = alpha_src_factor->value;
         blend.alpha_dst_factor  = alpha_dst_factor->value;

         if(!test_one(&blend, *type))
           success = FALSE;

      }
   }

   return success;
}


int main(int argc, char **argv)
{
   unsigned long n = 1000;
   unsigned i;
   boolean success;

   for(i = 1; i < argc; ++i) {
      if(strcmp(argv[i], "-v") == 0)
         ++verbose;
      else
         n = atoi(argv[i]);
   }
      
   if(n)
      success = test_some(n);
   else
      success = test_all();

   return success ? 0 : 1;
}

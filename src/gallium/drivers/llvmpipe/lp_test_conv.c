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
 * Unit tests for type conversion.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */


#include "gallivm/lp_bld_type.h"
#include "gallivm/lp_bld_const.h"
#include "gallivm/lp_bld_conv.h"
#include "gallivm/lp_bld_debug.h"
#include "lp_test.h"


typedef void (*conv_test_ptr_t)(const void *src, const void *dst);


void
write_tsv_header(FILE *fp)
{
   fprintf(fp,
           "result\t"
           "cycles_per_channel\t"
           "src_type\t"
           "dst_type\n");

   fflush(fp);
}


static void
write_tsv_row(FILE *fp,
              struct lp_type src_type,
              struct lp_type dst_type,
              double cycles,
              boolean success)
{
   fprintf(fp, "%s\t", success ? "pass" : "fail");

   fprintf(fp, "%.1f\t", cycles / MAX2(src_type.length, dst_type.length));

   dump_type(fp, src_type);
   fprintf(fp, "\t");

   dump_type(fp, dst_type);
   fprintf(fp, "\n");

   fflush(fp);
}


static void
dump_conv_types(FILE *fp,
               struct lp_type src_type,
               struct lp_type dst_type)
{
   fprintf(fp, "src_type=");
   dump_type(fp, src_type);

   fprintf(fp, " dst_type=");
   dump_type(fp, dst_type);

   fprintf(fp, " ...\n");
   fflush(fp);
}


static LLVMValueRef
add_conv_test(LLVMModuleRef module,
              struct lp_type src_type, unsigned num_srcs,
              struct lp_type dst_type, unsigned num_dsts)
{
   LLVMTypeRef args[2];
   LLVMValueRef func;
   LLVMValueRef src_ptr;
   LLVMValueRef dst_ptr;
   LLVMBasicBlockRef block;
   LLVMBuilderRef builder;
   LLVMValueRef src[LP_MAX_VECTOR_LENGTH];
   LLVMValueRef dst[LP_MAX_VECTOR_LENGTH];
   unsigned i;

   args[0] = LLVMPointerType(lp_build_vec_type(src_type), 0);
   args[1] = LLVMPointerType(lp_build_vec_type(dst_type), 0);

   func = LLVMAddFunction(module, "test", LLVMFunctionType(LLVMVoidType(), args, 2, 0));
   LLVMSetFunctionCallConv(func, LLVMCCallConv);
   src_ptr = LLVMGetParam(func, 0);
   dst_ptr = LLVMGetParam(func, 1);

   block = LLVMAppendBasicBlock(func, "entry");
   builder = LLVMCreateBuilder();
   LLVMPositionBuilderAtEnd(builder, block);

   for(i = 0; i < num_srcs; ++i) {
      LLVMValueRef index = LLVMConstInt(LLVMInt32Type(), i, 0);
      LLVMValueRef ptr = LLVMBuildGEP(builder, src_ptr, &index, 1, "");
      src[i] = LLVMBuildLoad(builder, ptr, "");
   }

   lp_build_conv(builder, src_type, dst_type, src, num_srcs, dst, num_dsts);

   for(i = 0; i < num_dsts; ++i) {
      LLVMValueRef index = LLVMConstInt(LLVMInt32Type(), i, 0);
      LLVMValueRef ptr = LLVMBuildGEP(builder, dst_ptr, &index, 1, "");
      LLVMBuildStore(builder, dst[i], ptr);
   }

   LLVMBuildRetVoid(builder);;

   LLVMDisposeBuilder(builder);
   return func;
}


PIPE_ALIGN_STACK
static boolean
test_one(unsigned verbose,
         FILE *fp,
         struct lp_type src_type,
         struct lp_type dst_type)
{
   LLVMModuleRef module = NULL;
   LLVMValueRef func = NULL;
   LLVMExecutionEngineRef engine = NULL;
   LLVMModuleProviderRef provider = NULL;
   LLVMPassManagerRef pass = NULL;
   char *error = NULL;
   conv_test_ptr_t conv_test_ptr;
   boolean success;
   const unsigned n = LP_TEST_NUM_SAMPLES;
   int64_t cycles[LP_TEST_NUM_SAMPLES];
   double cycles_avg = 0.0;
   unsigned num_srcs;
   unsigned num_dsts;
   double eps;
   unsigned i, j;

   if(verbose >= 1)
      dump_conv_types(stdout, src_type, dst_type);

   if(src_type.length > dst_type.length) {
      num_srcs = 1;
      num_dsts = src_type.length/dst_type.length;
   }
   else  {
      num_dsts = 1;
      num_srcs = dst_type.length/src_type.length;
   }

   assert(src_type.width * src_type.length == dst_type.width * dst_type.length);

   /* We must not loose or gain channels. Only precision */
   assert(src_type.length * num_srcs == dst_type.length * num_dsts);

   eps = MAX2(lp_const_eps(src_type), lp_const_eps(dst_type));

   module = LLVMModuleCreateWithName("test");

   func = add_conv_test(module, src_type, num_srcs, dst_type, num_dsts);

   if(LLVMVerifyModule(module, LLVMPrintMessageAction, &error)) {
      LLVMDumpModule(module);
      abort();
   }
   LLVMDisposeMessage(error);

   provider = LLVMCreateModuleProviderForExistingModule(module);
   if (LLVMCreateJITCompiler(&engine, provider, 1, &error)) {
      if(verbose < 1)
         dump_conv_types(stderr, src_type, dst_type);
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

   if(verbose >= 2)
      LLVMDumpModule(module);

   conv_test_ptr = (conv_test_ptr_t)LLVMGetPointerToGlobal(engine, func);

   if(verbose >= 2)
      lp_disassemble(conv_test_ptr);

   success = TRUE;
   for(i = 0; i < n && success; ++i) {
      unsigned src_stride = src_type.length*src_type.width/8;
      unsigned dst_stride = dst_type.length*dst_type.width/8;
      PIPE_ALIGN_VAR(16) uint8_t src[LP_MAX_VECTOR_LENGTH*LP_MAX_VECTOR_LENGTH];
      PIPE_ALIGN_VAR(16) uint8_t dst[LP_MAX_VECTOR_LENGTH*LP_MAX_VECTOR_LENGTH];
      double fref[LP_MAX_VECTOR_LENGTH*LP_MAX_VECTOR_LENGTH];
      uint8_t ref[LP_MAX_VECTOR_LENGTH*LP_MAX_VECTOR_LENGTH];
      int64_t start_counter = 0;
      int64_t end_counter = 0;

      for(j = 0; j < num_srcs; ++j) {
         random_vec(src_type, src + j*src_stride);
         read_vec(src_type, src + j*src_stride, fref + j*src_type.length);
      }

      for(j = 0; j < num_dsts; ++j) {
         write_vec(dst_type, ref + j*dst_stride, fref + j*dst_type.length);
      }

      start_counter = rdtsc();
      conv_test_ptr(src, dst);
      end_counter = rdtsc();

      cycles[i] = end_counter - start_counter;

      for(j = 0; j < num_dsts; ++j) {
         if(!compare_vec_with_eps(dst_type, dst + j*dst_stride, ref + j*dst_stride, eps))
            success = FALSE;
      }

      if (!success) {
         if(verbose < 1)
            dump_conv_types(stderr, src_type, dst_type);
         fprintf(stderr, "MISMATCH\n");

         for(j = 0; j < num_srcs; ++j) {
            fprintf(stderr, "  Src%u: ", j);
            dump_vec(stderr, src_type, src + j*src_stride);
            fprintf(stderr, "\n");
         }

#if 1
         fprintf(stderr, "  Ref: ");
         for(j = 0; j < src_type.length*num_srcs; ++j)
            fprintf(stderr, " %f", fref[j]);
         fprintf(stderr, "\n");
#endif

         for(j = 0; j < num_dsts; ++j) {
            fprintf(stderr, "  Dst%u: ", j);
            dump_vec(stderr, dst_type, dst + j*dst_stride);
            fprintf(stderr, "\n");

            fprintf(stderr, "  Ref%u: ", j);
            dump_vec(stderr, dst_type, ref + j*dst_stride);
            fprintf(stderr, "\n");
         }
      }
   }

   /*
    * Unfortunately the output of cycle counter is not very reliable as it comes
    * -- sometimes we get outliers (due IRQs perhaps?) which are
    * better removed to avoid random or biased data.
    */
   {
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

      cycles_avg = sum/m;

   }

   if(fp)
      write_tsv_row(fp, src_type, dst_type, cycles_avg, success);

   if (!success) {
      static boolean firsttime = TRUE;
      if(firsttime) {
         if(verbose < 2)
            LLVMDumpModule(module);
         LLVMWriteBitcodeToFile(module, "conv.bc");
         fprintf(stderr, "conv.bc written\n");
         fprintf(stderr, "Invoke as \"llc -o - conv.bc\"\n");
         firsttime = FALSE;
         /* abort(); */
      }
   }

   LLVMFreeMachineCodeForFunction(engine, func);

   LLVMDisposeExecutionEngine(engine);
   if(pass)
      LLVMDisposePassManager(pass);

   return success;
}


const struct lp_type conv_types[] = {
   /* float, fixed,  sign,  norm, width, len */

   {   TRUE, FALSE,  TRUE,  TRUE,    32,   4 },
   {   TRUE, FALSE,  TRUE, FALSE,    32,   4 },
   {   TRUE, FALSE, FALSE,  TRUE,    32,   4 },
   {   TRUE, FALSE, FALSE, FALSE,    32,   4 },

   /* TODO: test fixed formats too */

   {  FALSE, FALSE,  TRUE,  TRUE,    16,   8 },
   {  FALSE, FALSE,  TRUE, FALSE,    16,   8 },
   {  FALSE, FALSE, FALSE,  TRUE,    16,   8 },
   {  FALSE, FALSE, FALSE, FALSE,    16,   8 },

   {  FALSE, FALSE,  TRUE,  TRUE,    32,   4 },
   {  FALSE, FALSE,  TRUE, FALSE,    32,   4 },
   {  FALSE, FALSE, FALSE,  TRUE,    32,   4 },
   {  FALSE, FALSE, FALSE, FALSE,    32,   4 },

   {  FALSE, FALSE,  TRUE,  TRUE,    16,   8 },
   {  FALSE, FALSE,  TRUE, FALSE,    16,   8 },
   {  FALSE, FALSE, FALSE,  TRUE,    16,   8 },
   {  FALSE, FALSE, FALSE, FALSE,    16,   8 },

   {  FALSE, FALSE,  TRUE,  TRUE,     8,  16 },
   {  FALSE, FALSE,  TRUE, FALSE,     8,  16 },
   {  FALSE, FALSE, FALSE,  TRUE,     8,  16 },
   {  FALSE, FALSE, FALSE, FALSE,     8,  16 },
};


const unsigned num_types = sizeof(conv_types)/sizeof(conv_types[0]);


boolean
test_all(unsigned verbose, FILE *fp)
{
   const struct lp_type *src_type;
   const struct lp_type *dst_type;
   bool success = TRUE;

   for(src_type = conv_types; src_type < &conv_types[num_types]; ++src_type) {
      for(dst_type = conv_types; dst_type < &conv_types[num_types]; ++dst_type) {

         if(src_type == dst_type)
            continue;

         if(src_type->norm != dst_type->norm)
            continue;

         if(!test_one(verbose, fp, *src_type, *dst_type))
           success = FALSE;

      }
   }

   return success;
}


boolean
test_some(unsigned verbose, FILE *fp, unsigned long n)
{
   const struct lp_type *src_type;
   const struct lp_type *dst_type;
   unsigned long i;
   bool success = TRUE;

   for(i = 0; i < n; ++i) {
      src_type = &conv_types[rand() % num_types];
      
      do {
         dst_type = &conv_types[rand() % num_types];
      } while (src_type == dst_type || src_type->norm != dst_type->norm);

      if(!test_one(verbose, fp, *src_type, *dst_type))
        success = FALSE;
   }

   return success;
}

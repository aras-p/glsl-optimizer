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


#include <stdlib.h>
#include <stdio.h>
#include <float.h>

#include "gallivm/lp_bld.h"
#include "gallivm/lp_bld_init.h"
#include <llvm-c/Analysis.h>
#include <llvm-c/Target.h>
#include <llvm-c/Transforms/Scalar.h>

#include "util/u_memory.h"
#include "util/u_format.h"
#include "util/u_format_tests.h"
#include "util/u_format_s3tc.h"

#include "gallivm/lp_bld_format.h"
#include "lp_test.h"


void
write_tsv_header(FILE *fp)
{
   fprintf(fp,
           "result\t"
           "format\n");

   fflush(fp);
}


static void
write_tsv_row(FILE *fp,
              const struct util_format_description *desc,
              boolean success)
{
   fprintf(fp, "%s\t", success ? "pass" : "fail");

   fprintf(fp, "%s\n", desc->name);

   fflush(fp);
}


typedef void
(*fetch_ptr_t)(float *, const void *packed,
               unsigned i, unsigned j);


static LLVMValueRef
add_fetch_rgba_test(LLVMModuleRef lp_build_module,
                    const struct util_format_description *desc)
{
   LLVMTypeRef args[4];
   LLVMValueRef func;
   LLVMValueRef packed_ptr;
   LLVMValueRef rgba_ptr;
   LLVMValueRef i;
   LLVMValueRef j;
   LLVMBasicBlockRef block;
   LLVMBuilderRef builder;
   LLVMValueRef rgba;

   args[0] = LLVMPointerType(LLVMVectorType(LLVMFloatType(), 4), 0);
   args[1] = LLVMPointerType(LLVMInt8Type(), 0);
   args[3] = args[2] = LLVMInt32Type();

   func = LLVMAddFunction(lp_build_module, "fetch", LLVMFunctionType(LLVMVoidType(), args, Elements(args), 0));
   LLVMSetFunctionCallConv(func, LLVMCCallConv);
   rgba_ptr = LLVMGetParam(func, 0);
   packed_ptr = LLVMGetParam(func, 1);
   i = LLVMGetParam(func, 2);
   j = LLVMGetParam(func, 3);

   block = LLVMAppendBasicBlock(func, "entry");
   builder = LLVMCreateBuilder();
   LLVMPositionBuilderAtEnd(builder, block);

   rgba = lp_build_fetch_rgba_aos(builder, desc, packed_ptr, i, j);

   LLVMBuildStore(builder, rgba, rgba_ptr);

   LLVMBuildRetVoid(builder);

   LLVMDisposeBuilder(builder);
   return func;
}


PIPE_ALIGN_STACK
static boolean
test_format(unsigned verbose, FILE *fp,
            const struct util_format_description *desc,
            const struct util_format_test_case *test)
{
   LLVMValueRef fetch = NULL;
   LLVMPassManagerRef pass = NULL;
   fetch_ptr_t fetch_ptr;
   float unpacked[4];
   boolean success;
   unsigned i, j, k;

   fetch = add_fetch_rgba_test(lp_build_module, desc);

   if (LLVMVerifyFunction(fetch, LLVMPrintMessageAction)) {
      LLVMDumpValue(fetch);
      abort();
   }

#if 0
   pass = LLVMCreatePassManager();
   LLVMAddTargetData(LLVMGetExecutionEngineTargetData(lp_build_engine), pass);
   /* These are the passes currently listed in llvm-c/Transforms/Scalar.h,
    * but there are more on SVN. */
   LLVMAddConstantPropagationPass(pass);
   LLVMAddInstructionCombiningPass(pass);
   LLVMAddPromoteMemoryToRegisterPass(pass);
   LLVMAddGVNPass(pass);
   LLVMAddCFGSimplificationPass(pass);
   LLVMRunPassManager(pass, lp_build_module);
#else
   (void)pass;
#endif

   fetch_ptr = (fetch_ptr_t) LLVMGetPointerToGlobal(lp_build_engine, fetch);

   for (i = 0; i < desc->block.height; ++i) {
      for (j = 0; j < desc->block.width; ++j) {

         memset(unpacked, 0, sizeof unpacked);

         fetch_ptr(unpacked, test->packed, j, i);

         success = TRUE;
         for(k = 0; k < 4; ++k)
            if (fabs((float)test->unpacked[i][j][k] - unpacked[k]) > FLT_EPSILON)
               success = FALSE;

         if (!success) {
            printf("FAILED\n");
            printf("  Packed: %02x %02x %02x %02x\n",
                   test->packed[0], test->packed[1], test->packed[2], test->packed[3]);
            printf("  Unpacked (%u,%u): %f %f %f %f obtained\n",
                   j, i,
                   unpacked[0], unpacked[1], unpacked[2], unpacked[3]);
            printf("                  %f %f %f %f expected\n",
                   test->unpacked[i][j][0],
                   test->unpacked[i][j][1],
                   test->unpacked[i][j][2],
                   test->unpacked[i][j][3]);
         }
      }
   }

   if (!success)
      LLVMDumpValue(fetch);

   LLVMFreeMachineCodeForFunction(lp_build_engine, fetch);
   LLVMDeleteFunction(fetch);

   if(pass)
      LLVMDisposePassManager(pass);

   if(fp)
      write_tsv_row(fp, desc, success);

   return success;
}



static boolean
test_one(unsigned verbose, FILE *fp,
         const struct util_format_description *format_desc)
{
   unsigned i;
   bool first = TRUE;
   bool success = TRUE;

   for (i = 0; i < util_format_nr_test_cases; ++i) {
      const struct util_format_test_case *test = &util_format_test_cases[i];

      if (test->format == format_desc->format) {

         if (first) {
            printf("Testing %s ...\n",
                   format_desc->name);
            first = FALSE;
         }

         if (!test_format(verbose, fp, format_desc, test)) {
           success = FALSE;
         }
      }
   }

   return success;
}


boolean
test_all(unsigned verbose, FILE *fp)
{
   enum pipe_format format;
   bool success = TRUE;

   for (format = 1; format < PIPE_FORMAT_COUNT; ++format) {
      const struct util_format_description *format_desc;

      format_desc = util_format_description(format);
      if (!format_desc) {
         continue;
      }

      /*
       * TODO: test more
       */

      if (format_desc->colorspace == UTIL_FORMAT_COLORSPACE_ZS) {
         continue;
      }

      if (format_desc->layout == UTIL_FORMAT_LAYOUT_S3TC &&
          !util_format_s3tc_enabled) {
         continue;
      }

      if (!test_one(verbose, fp, format_desc)) {
           success = FALSE;
      }
   }

   return success;
}


boolean
test_some(unsigned verbose, FILE *fp, unsigned long n)
{
   return test_all(verbose, fp);
}

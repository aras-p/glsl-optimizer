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

#include "gallivm/lp_bld.h"
#include <llvm-c/Analysis.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Transforms/Scalar.h>

#include "util/u_cpu_detect.h"
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


typedef void (*fetch_ptr_t)(const void *packed, float *);


static LLVMValueRef
add_fetch_rgba_test(LLVMModuleRef module,
                    const struct util_format_description *desc)
{
   LLVMTypeRef args[2];
   LLVMValueRef func;
   LLVMValueRef packed_ptr;
   LLVMValueRef rgba_ptr;
   LLVMBasicBlockRef block;
   LLVMBuilderRef builder;
   LLVMValueRef rgba;

   args[0] = LLVMPointerType(LLVMInt8Type(), 0);
   args[1] = LLVMPointerType(LLVMVectorType(LLVMFloatType(), 4), 0);

   func = LLVMAddFunction(module, "fetch", LLVMFunctionType(LLVMVoidType(), args, 2, 0));
   LLVMSetFunctionCallConv(func, LLVMCCallConv);
   packed_ptr = LLVMGetParam(func, 0);
   rgba_ptr = LLVMGetParam(func, 1);

   block = LLVMAppendBasicBlock(func, "entry");
   builder = LLVMCreateBuilder();
   LLVMPositionBuilderAtEnd(builder, block);

   rgba = lp_build_fetch_rgba_aos(builder, desc, packed_ptr);

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
   LLVMModuleRef module = NULL;
   LLVMValueRef fetch = NULL;
   LLVMExecutionEngineRef engine = NULL;
   LLVMModuleProviderRef provider = NULL;
   LLVMPassManagerRef pass = NULL;
   char *error = NULL;
   fetch_ptr_t fetch_ptr;
   float unpacked[4];
   boolean success;
   unsigned i;

   module = LLVMModuleCreateWithName("test");

   fetch = add_fetch_rgba_test(module, desc);

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

   fetch_ptr  = (fetch_ptr_t) LLVMGetPointerToGlobal(engine, fetch);

   memset(unpacked, 0, sizeof unpacked);

   fetch_ptr(test->packed, unpacked);

   success = TRUE;
   for(i = 0; i < 4; ++i)
      if(test->unpacked[0][0][i] != unpacked[i])
         success = FALSE;

   if (!success) {
      printf("FAILED\n");
      printf("  Packed: %02x %02x %02x %02x\n",
             test->packed[0], test->packed[1], test->packed[2], test->packed[3]);
      printf("  Unpacked: %f %f %f %f obtained\n",
             unpacked[0], unpacked[1], unpacked[2], unpacked[3]);
      printf("            %f %f %f %f expected\n",
             test->unpacked[0][0][0],
             test->unpacked[0][0][1],
             test->unpacked[0][0][2],
             test->unpacked[0][0][3]);
      LLVMDumpModule(module);
   }

   LLVMFreeMachineCodeForFunction(engine, fetch);

   LLVMDisposeExecutionEngine(engine);
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
   bool success = TRUE;

   printf("Testing %s ...\n",
          format_desc->name);

   for (i = 0; i < util_format_nr_test_cases; ++i) {
      const struct util_format_test_case *test = &util_format_test_cases[i];

      if (test->format == format_desc->format) {

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
       * XXX: copied from lp_build_fetch_rgba_aos()
       * TODO: test more
       */

      if (!(format_desc->layout == UTIL_FORMAT_LAYOUT_PLAIN &&
            format_desc->colorspace == UTIL_FORMAT_COLORSPACE_RGB &&
            format_desc->block.width == 1 &&
            format_desc->block.height == 1 &&
            util_is_pot(format_desc->block.bits) &&
            format_desc->block.bits <= 32 &&
            format_desc->is_bitmask &&
            !format_desc->is_mixed &&
            (format_desc->channel[0].type == UTIL_FORMAT_TYPE_UNSIGNED ||
             format_desc->channel[1].type == UTIL_FORMAT_TYPE_UNSIGNED))) {
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

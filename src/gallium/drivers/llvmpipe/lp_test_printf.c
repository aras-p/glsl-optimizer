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
#include "gallivm/lp_bld_assert.h"
#include "gallivm/lp_bld_printf.h"

#include <llvm-c/Analysis.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Transforms/Scalar.h>

#include "lp_test.h"


struct printf_test_case {
   int foo;
};

void
write_tsv_header(FILE *fp)
{
   fprintf(fp,
           "result\t"
           "format\n");

   fflush(fp);
}



typedef void (*test_printf_t)(int i);


static LLVMValueRef
add_printf_test(LLVMModuleRef module)
{
   LLVMTypeRef args[1] = { LLVMIntType(32) };
   LLVMValueRef func = LLVMAddFunction(module, "test_printf", LLVMFunctionType(LLVMVoidType(), args, 1, 0));
   LLVMBuilderRef builder = LLVMCreateBuilder();
   LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");

   LLVMSetFunctionCallConv(func, LLVMCCallConv);

   LLVMPositionBuilderAtEnd(builder, block);
   lp_build_printf(builder, "hello, world\n");
   lp_build_printf(builder, "print 5 6: %d %d\n", LLVMConstInt(LLVMInt32Type(), 5, 0),
				LLVMConstInt(LLVMInt32Type(), 6, 0));

   /* Also test lp_build_assert().  This should not fail. */
   lp_build_assert(builder, LLVMConstInt(LLVMInt32Type(), 1, 0), "assert(1)");

   LLVMBuildRetVoid(builder);
   LLVMDisposeBuilder(builder);
   return func;
}


PIPE_ALIGN_STACK
static boolean
test_printf(unsigned verbose, FILE *fp, const struct printf_test_case *testcase)
{
   LLVMModuleRef module = NULL;
   LLVMValueRef test = NULL;
   LLVMExecutionEngineRef engine = NULL;
   LLVMModuleProviderRef provider = NULL;
   LLVMPassManagerRef pass = NULL;
   char *error = NULL;
   test_printf_t test_printf;
   float unpacked[4];
   unsigned packed;
   boolean success = TRUE;
   void *code;

   module = LLVMModuleCreateWithName("test");

   test = add_printf_test(module);

   if(LLVMVerifyModule(module, LLVMPrintMessageAction, &error)) {
      LLVMDumpModule(module);
      abort();
   }
   LLVMDisposeMessage(error);

   provider = LLVMCreateModuleProviderForExistingModule(module);
#if 0
   if (LLVMCreateJITCompiler(&engine, provider, 1, &error)) {
      fprintf(stderr, "%s\n", error);
      LLVMDisposeMessage(error);
      abort();
   }
#else
   (void) provider;
   engine = lp_build_engine;
#endif

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

   code = LLVMGetPointerToGlobal(engine, test);
   test_printf = (test_printf_t)pointer_to_func(code);

   memset(unpacked, 0, sizeof unpacked);
   packed = 0;


   // LLVMDumpModule(module);

   test_printf(0);

   LLVMFreeMachineCodeForFunction(engine, test);

   LLVMDisposeExecutionEngine(engine);
   if(pass)
      LLVMDisposePassManager(pass);

   return success;
}


boolean
test_all(unsigned verbose, FILE *fp)
{
   boolean success = TRUE;

   test_printf(verbose, fp, NULL);

   return success;
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

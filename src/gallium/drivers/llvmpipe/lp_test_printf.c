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
add_printf_test(struct gallivm_state *gallivm)
{
   LLVMModuleRef module = gallivm->module;
   LLVMTypeRef args[1] = { LLVMIntTypeInContext(gallivm->context, 32) };
   LLVMValueRef func = LLVMAddFunction(module, "test_printf", LLVMFunctionType(LLVMVoidTypeInContext(gallivm->context), args, 1, 0));
   LLVMBuilderRef builder = gallivm->builder;
   LLVMBasicBlockRef block = LLVMAppendBasicBlockInContext(gallivm->context, func, "entry");

   LLVMSetFunctionCallConv(func, LLVMCCallConv);

   LLVMPositionBuilderAtEnd(builder, block);
   lp_build_printf(gallivm, "hello, world\n");
   lp_build_printf(gallivm, "print 5 6: %d %d\n", LLVMConstInt(LLVMInt32TypeInContext(gallivm->context), 5, 0),
				LLVMConstInt(LLVMInt32TypeInContext(gallivm->context), 6, 0));

   /* Also test lp_build_assert().  This should not fail. */
   lp_build_assert(gallivm, LLVMConstInt(LLVMInt32TypeInContext(gallivm->context), 1, 0), "assert(1)");

   LLVMBuildRetVoid(builder);

   return func;
}


PIPE_ALIGN_STACK
static boolean
test_printf(struct gallivm_state *gallivm,
            unsigned verbose, FILE *fp,
            const struct printf_test_case *testcase)
{
   LLVMExecutionEngineRef engine = gallivm->engine;
   LLVMModuleRef module = gallivm->module;
   LLVMValueRef test;
   char *error = NULL;
   test_printf_t test_printf_func;
   boolean success = TRUE;
   void *code;

   test = add_printf_test(gallivm);

   if(LLVMVerifyModule(module, LLVMPrintMessageAction, &error)) {
      LLVMDumpModule(module);
      abort();
   }
   LLVMDisposeMessage(error);

   code = LLVMGetPointerToGlobal(engine, test);
   test_printf_func = (test_printf_t) pointer_to_func(code);

   // LLVMDumpModule(module);

   test_printf_func(0);

   LLVMFreeMachineCodeForFunction(engine, test);

   return success;
}


boolean
test_all(struct gallivm_state *gallivm, unsigned verbose, FILE *fp)
{
   boolean success = TRUE;

   test_printf(gallivm, verbose, fp, NULL);

   return success;
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

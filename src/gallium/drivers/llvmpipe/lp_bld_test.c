/**************************************************************************
 *
 * Copyright 2009 Vmware, Inc.
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

#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Transforms/Scalar.h>

#include "lp_bld.h"


static LLVMValueRef
add_unpack_rgba_test(LLVMModuleRef module,
                     enum pipe_format format)
{
   LLVMTypeRef args[] = {
      LLVMPointerType(LLVMInt8Type(), 0),
      LLVMPointerType(LLVMVectorType(LLVMFloatType(), 4), 0)
   };
   LLVMValueRef func = LLVMAddFunction(module, "unpack", LLVMFunctionType(LLVMVoidType(), args, 2, 0));
   LLVMSetFunctionCallConv(func, LLVMCCallConv);
   LLVMValueRef ptr = LLVMGetParam(func, 0);
   LLVMValueRef rgba_ptr = LLVMGetParam(func, 1);

   LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
   LLVMBuilderRef builder = LLVMCreateBuilder();
   LLVMPositionBuilderAtEnd(builder, block);

   LLVMValueRef rgba;

   struct lp_build_loop_state loop;

   lp_build_loop_begin(builder, LLVMConstInt(LLVMInt32Type(), 1, 0), &loop);

   rgba = lp_build_unpack_rgba(builder, format, ptr);
   LLVMBuildStore(builder, rgba, rgba_ptr);

   lp_build_loop_end(builder, LLVMConstInt(LLVMInt32Type(), 4, 0), NULL, &loop);

   LLVMBuildRetVoid(builder);

   LLVMDisposeBuilder(builder);
   return func;
}


static LLVMValueRef
add_pack_rgba_test(LLVMModuleRef module,
                   enum pipe_format format)
{
   LLVMTypeRef args[] = {
      LLVMPointerType(LLVMInt8Type(), 0),
      LLVMPointerType(LLVMVectorType(LLVMFloatType(), 4), 0)
   };
   LLVMValueRef func = LLVMAddFunction(module, "pack", LLVMFunctionType(LLVMVoidType(), args, 2, 0));
   LLVMSetFunctionCallConv(func, LLVMCCallConv);
   LLVMValueRef ptr = LLVMGetParam(func, 0);
   LLVMValueRef rgba_ptr = LLVMGetParam(func, 1);

   LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
   LLVMBuilderRef builder = LLVMCreateBuilder();
   LLVMPositionBuilderAtEnd(builder, block);

   LLVMValueRef rgba;

   rgba = LLVMBuildLoad(builder, rgba_ptr, "");

   lp_build_pack_rgba(builder, format, ptr, rgba);

   LLVMBuildRetVoid(builder);

   LLVMDisposeBuilder(builder);
   return func;
}


int main(int argc, char **argv)
{
   char *error = NULL;
   int n;

   if (argc > 1)
      sscanf(argv[1], "%x", &n);
   else
      n = 0x0000f0f0;

   LLVMModuleRef module = LLVMModuleCreateWithName("test");

   enum pipe_format format;
   format = PIPE_FORMAT_R5G6B5_UNORM;
   LLVMValueRef unpack = add_unpack_rgba_test(module, format);
   LLVMValueRef pack = add_pack_rgba_test(module, format);

   LLVMVerifyModule(module, LLVMAbortProcessAction, &error);
   LLVMDisposeMessage(error);

   LLVMExecutionEngineRef engine;
   LLVMModuleProviderRef provider = LLVMCreateModuleProviderForExistingModule(module);
   error = NULL;
   LLVMCreateJITCompiler(&engine, provider, 1, &error);
   if (error) {
      fprintf(stderr, "%s\n", error);
      LLVMDisposeMessage(error);
      abort();
   }

   LLVMPassManagerRef pass = LLVMCreatePassManager();
#if 0
   LLVMAddTargetData(LLVMGetExecutionEngineTargetData(engine), pass);
   /* These are the passes currently listed in llvm-c/Transforms/Scalar.h,
    * but there are more on SVN. */
   LLVMAddConstantPropagationPass(pass);
   LLVMAddInstructionCombiningPass(pass);
   LLVMAddPromoteMemoryToRegisterPass(pass);
   LLVMAddDemoteMemoryToRegisterPass(pass);
   LLVMAddGVNPass(pass);
   LLVMAddCFGSimplificationPass(pass);
   LLVMRunPassManager(pass, module);
#endif
   LLVMDumpModule(module);

   printf("Packed: %08x\n", n);

   float rgba[4] = {0, 0, 0, 0};

   {
#if 1
      typedef void (*unpack_ptr_t)(void *, float *);
      unpack_ptr_t unpack_ptr = (unpack_ptr_t)LLVMGetPointerToGlobal(engine, unpack);

      unpack_ptr(&n, rgba);
#else
      LLVMGenericValueRef exec_args[] = {
         LLVMCreateGenericValueOfPointer(n),
         LLVMCreateGenericValueOfPointer(rgba)
      };
      LLVMGenericValueRef exec_res = LLVMRunFunction(engine, unpack, 2, exec_args);
#endif

      printf("Unpacked: %f %f %f %f\n",
             rgba[0],
             rgba[1],
             rgba[2],
             rgba[3]);
   }

   n = 0;

   {
#if 1
      typedef void (*pack_ptr_t)(void *, float *);
      pack_ptr_t pack_ptr = (pack_ptr_t)LLVMGetPointerToGlobal(engine, pack);

      pack_ptr(&n, rgba);
#else
      LLVMGenericValueRef exec_args[] = {
         LLVMCreateGenericValueOfPointer(n),
         LLVMCreateGenericValueOfPointer(rgba)
      };
      LLVMGenericValueRef exec_res = LLVMRunFunction(engine, pack, 2, exec_args);
#endif

      printf("Packed: %08x\n", n);
   }

   LLVMDisposePassManager(pass);
   LLVMDisposeExecutionEngine(engine);

   return 0;
}

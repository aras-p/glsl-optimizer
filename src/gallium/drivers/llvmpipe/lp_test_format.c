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

#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Transforms/Scalar.h>

#include "util/u_format.h"

#include "lp_bld_flow.h"
#include "lp_bld_format.h"


struct pixel_test_case
{
   enum pipe_format format;
   uint32_t packed;
   double unpacked[4];
};


struct pixel_test_case test_cases[] =
{
   {PIPE_FORMAT_R5G6B5_UNORM,   0x0000, {0.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_R5G6B5_UNORM,   0x001f, {0.0, 0.0, 1.0, 1.0}},
   {PIPE_FORMAT_R5G6B5_UNORM,   0x07e0, {0.0, 1.0, 0.0, 1.0}},
   {PIPE_FORMAT_R5G6B5_UNORM,   0xf800, {1.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_R5G6B5_UNORM,   0xffff, {1.0, 1.0, 1.0, 1.0}},

   {PIPE_FORMAT_A1R5G5B5_UNORM, 0x0000, {0.0, 0.0, 0.0, 0.0}},
   {PIPE_FORMAT_A1R5G5B5_UNORM, 0x001f, {0.0, 0.0, 1.0, 0.0}},
   {PIPE_FORMAT_A1R5G5B5_UNORM, 0x03e0, {0.0, 1.0, 0.0, 0.0}},
   {PIPE_FORMAT_A1R5G5B5_UNORM, 0x7c00, {1.0, 0.0, 0.0, 0.0}},
   {PIPE_FORMAT_A1R5G5B5_UNORM, 0x8000, {0.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_A1R5G5B5_UNORM, 0xffff, {1.0, 1.0, 1.0, 1.0}},

   {PIPE_FORMAT_A8R8G8B8_UNORM, 0x00000000, {0.0, 0.0, 0.0, 0.0}},
   {PIPE_FORMAT_A8R8G8B8_UNORM, 0x000000ff, {0.0, 0.0, 1.0, 0.0}},
   {PIPE_FORMAT_A8R8G8B8_UNORM, 0x0000ff00, {0.0, 1.0, 0.0, 0.0}},
   {PIPE_FORMAT_A8R8G8B8_UNORM, 0x00ff0000, {1.0, 0.0, 0.0, 0.0}},
   {PIPE_FORMAT_A8R8G8B8_UNORM, 0xff000000, {0.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_A8R8G8B8_UNORM, 0xffffffff, {1.0, 1.0, 1.0, 1.0}},

#if 0
   {PIPE_FORMAT_R8G8B8A8_UNORM, 0x00000000, {0.0, 0.0, 0.0, 0.0}},
   {PIPE_FORMAT_R8G8B8A8_UNORM, 0x000000ff, {0.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_R8G8B8A8_UNORM, 0x0000ff00, {0.0, 0.0, 1.0, 0.0}},
   {PIPE_FORMAT_R8G8B8A8_UNORM, 0x00ff0000, {0.0, 1.0, 0.0, 0.0}},
   {PIPE_FORMAT_R8G8B8A8_UNORM, 0xff000000, {1.0, 0.0, 0.0, 0.0}},
   {PIPE_FORMAT_R8G8B8A8_UNORM, 0xffffffff, {1.0, 1.0, 1.0, 1.0}},
#endif

   {PIPE_FORMAT_B8G8R8A8_UNORM, 0x00000000, {0.0, 0.0, 0.0, 0.0}},
   {PIPE_FORMAT_B8G8R8A8_UNORM, 0x000000ff, {0.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_B8G8R8A8_UNORM, 0x0000ff00, {1.0, 0.0, 0.0, 0.0}},
   {PIPE_FORMAT_B8G8R8A8_UNORM, 0x00ff0000, {0.0, 1.0, 0.0, 0.0}},
   {PIPE_FORMAT_B8G8R8A8_UNORM, 0xff000000, {0.0, 0.0, 1.0, 0.0}},
   {PIPE_FORMAT_B8G8R8A8_UNORM, 0xffffffff, {1.0, 1.0, 1.0, 1.0}},
};


typedef void (*load_ptr_t)(const void *, float *);


static LLVMValueRef
add_load_rgba_test(LLVMModuleRef module,
                   enum pipe_format format)
{
   LLVMTypeRef args[2];
   LLVMValueRef func;
   LLVMValueRef ptr;
   LLVMValueRef rgba_ptr;
   LLVMBasicBlockRef block;
   LLVMBuilderRef builder;
   LLVMValueRef rgba;
   struct lp_build_loop_state loop;

   args[0] = LLVMPointerType(LLVMInt8Type(), 0);
   args[1] = LLVMPointerType(LLVMVectorType(LLVMFloatType(), 4), 0);

   func = LLVMAddFunction(module, "load", LLVMFunctionType(LLVMVoidType(), args, 2, 0));
   LLVMSetFunctionCallConv(func, LLVMCCallConv);
   ptr = LLVMGetParam(func, 0);
   rgba_ptr = LLVMGetParam(func, 1);

   block = LLVMAppendBasicBlock(func, "entry");
   builder = LLVMCreateBuilder();
   LLVMPositionBuilderAtEnd(builder, block);

   lp_build_loop_begin(builder, LLVMConstInt(LLVMInt32Type(), 1, 0), &loop);

   rgba = lp_build_load_rgba(builder, format, ptr);
   LLVMBuildStore(builder, rgba, rgba_ptr);

   lp_build_loop_end(builder, LLVMConstInt(LLVMInt32Type(), 4, 0), NULL, &loop);

   LLVMBuildRetVoid(builder);

   LLVMDisposeBuilder(builder);
   return func;
}


typedef void (*store_ptr_t)(void *, const float *);


static LLVMValueRef
add_store_rgba_test(LLVMModuleRef module,
                    enum pipe_format format)
{
   LLVMTypeRef args[2];
   LLVMValueRef func;
   LLVMValueRef ptr;
   LLVMValueRef rgba_ptr;
   LLVMBasicBlockRef block;
   LLVMBuilderRef builder;
   LLVMValueRef rgba;

   args[0] = LLVMPointerType(LLVMInt8Type(), 0);
   args[1] = LLVMPointerType(LLVMVectorType(LLVMFloatType(), 4), 0);

   func = LLVMAddFunction(module, "store", LLVMFunctionType(LLVMVoidType(), args, 2, 0));
   LLVMSetFunctionCallConv(func, LLVMCCallConv);
   ptr = LLVMGetParam(func, 0);
   rgba_ptr = LLVMGetParam(func, 1);

   block = LLVMAppendBasicBlock(func, "entry");
   builder = LLVMCreateBuilder();
   LLVMPositionBuilderAtEnd(builder, block);

   rgba = LLVMBuildLoad(builder, rgba_ptr, "");

   lp_build_store_rgba(builder, format, ptr, rgba);

   LLVMBuildRetVoid(builder);

   LLVMDisposeBuilder(builder);
   return func;
}


static boolean
test_format(const struct pixel_test_case *test)
{
   LLVMModuleRef module = NULL;
   LLVMValueRef load = NULL;
   LLVMValueRef store = NULL;
   LLVMExecutionEngineRef engine = NULL;
   LLVMModuleProviderRef provider = NULL;
   LLVMPassManagerRef pass = NULL;
   char *error = NULL;
   const struct util_format_description *desc;
   load_ptr_t load_ptr;
   store_ptr_t store_ptr;
   float unpacked[4];
   unsigned packed;
   boolean success;
   unsigned i;

   desc = util_format_description(test->format);
   fprintf(stderr, "%s\n", desc->name);

   module = LLVMModuleCreateWithName("test");

   load = add_load_rgba_test(module, test->format);
   store = add_store_rgba_test(module, test->format);

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

   load_ptr  = (load_ptr_t) LLVMGetPointerToGlobal(engine, load);
   store_ptr = (store_ptr_t)LLVMGetPointerToGlobal(engine, store);

   memset(unpacked, 0, sizeof unpacked);
   packed = 0;

   load_ptr(&test->packed, unpacked);
   store_ptr(&packed, unpacked);

   success = TRUE;
   if(test->packed != packed)
      success = FALSE;
   for(i = 0; i < 4; ++i)
      if(test->unpacked[i] != unpacked[i])
         success = FALSE;

   if (!success) {
      printf("FAILED\n");
      printf("  Packed: %08x\n", test->packed);
      printf("          %08x\n", packed);
      printf("  Unpacked: %f %f %f %f\n", unpacked[0], unpacked[1], unpacked[2], unpacked[3]);
      printf("            %f %f %f %f\n", test->unpacked[0], test->unpacked[1], test->unpacked[2], test->unpacked[3]);
      LLVMDumpModule(module);
   }

   LLVMFreeMachineCodeForFunction(engine, store);
   LLVMFreeMachineCodeForFunction(engine, load);

   LLVMDisposeExecutionEngine(engine);
   if(pass)
      LLVMDisposePassManager(pass);

   return success;
}


int main(int argc, char **argv)
{
   unsigned i;
   int ret;

   for (i = 0; i < sizeof(test_cases)/sizeof(test_cases[0]); ++i)
      if(!test_format(&test_cases[i]))
        ret = 1;

   return ret;
}

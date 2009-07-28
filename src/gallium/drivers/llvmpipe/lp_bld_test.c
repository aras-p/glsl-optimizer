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

#include "lp_bld.h"


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

   {PIPE_FORMAT_R8G8B8A8_UNORM, 0x00000000, {0.0, 0.0, 0.0, 0.0}},
   {PIPE_FORMAT_R8G8B8A8_UNORM, 0x000000ff, {0.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_R8G8B8A8_UNORM, 0x0000ff00, {0.0, 0.0, 1.0, 0.0}},
   {PIPE_FORMAT_R8G8B8A8_UNORM, 0x00ff0000, {0.0, 1.0, 0.0, 0.0}},
   {PIPE_FORMAT_R8G8B8A8_UNORM, 0xff000000, {1.0, 0.0, 0.0, 0.0}},
   {PIPE_FORMAT_R8G8B8A8_UNORM, 0xffffffff, {1.0, 1.0, 1.0, 1.0}},

   {PIPE_FORMAT_B8G8R8A8_UNORM, 0x00000000, {0.0, 0.0, 0.0, 0.0}},
   {PIPE_FORMAT_B8G8R8A8_UNORM, 0x000000ff, {0.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_B8G8R8A8_UNORM, 0x0000ff00, {1.0, 0.0, 0.0, 0.0}},
   {PIPE_FORMAT_B8G8R8A8_UNORM, 0x00ff0000, {0.0, 1.0, 0.0, 0.0}},
   {PIPE_FORMAT_B8G8R8A8_UNORM, 0xff000000, {0.0, 0.0, 1.0, 0.0}},
   {PIPE_FORMAT_B8G8R8A8_UNORM, 0xffffffff, {1.0, 1.0, 1.0, 1.0}},
};


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


static boolean
test_format(const struct pixel_test_case *test)
{
   char *error = NULL;
   const struct util_format_description *desc;
   
   desc = util_format_description(test->format);
   fprintf(stderr, "%s\n", desc->name);

   LLVMModuleRef module = LLVMModuleCreateWithName("test");

   LLVMValueRef unpack = add_unpack_rgba_test(module, test->format);
   LLVMValueRef pack = add_pack_rgba_test(module, test->format);

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
   LLVMDumpModule(module);
#endif


   float unpacked[4] = {0, 0, 0, 0};
   unsigned packed = 0;

   {
      typedef void (*unpack_ptr_t)(const void *, float *);
      unpack_ptr_t unpack_ptr = (unpack_ptr_t)LLVMGetPointerToGlobal(engine, unpack);

      unpack_ptr(&test->packed, unpacked);

   }


   {
      typedef void (*pack_ptr_t)(void *, const float *);
      pack_ptr_t pack_ptr = (pack_ptr_t)LLVMGetPointerToGlobal(engine, pack);

      pack_ptr(&packed, unpacked);

   }

   boolean success = TRUE;
   unsigned i;
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

   LLVMDisposePassManager(pass);
   LLVMDisposeExecutionEngine(engine);
   //LLVMDisposeModule(module);

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

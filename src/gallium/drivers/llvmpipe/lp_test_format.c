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

#include "util/u_cpu_detect.h"
#include "util/u_format.h"

#include "gallivm/lp_bld_format.h"
#include "lp_test.h"


struct pixel_test_case
{
   enum pipe_format format;
   uint32_t packed;
   double unpacked[4];
};


struct pixel_test_case test_cases[] =
{
   {PIPE_FORMAT_B5G6R5_UNORM,   0x0000, {0.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_B5G6R5_UNORM,   0x001f, {0.0, 0.0, 1.0, 1.0}},
   {PIPE_FORMAT_B5G6R5_UNORM,   0x07e0, {0.0, 1.0, 0.0, 1.0}},
   {PIPE_FORMAT_B5G6R5_UNORM,   0xf800, {1.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_B5G6R5_UNORM,   0xffff, {1.0, 1.0, 1.0, 1.0}},

   {PIPE_FORMAT_B5G5R5A1_UNORM, 0x0000, {0.0, 0.0, 0.0, 0.0}},
   {PIPE_FORMAT_B5G5R5A1_UNORM, 0x001f, {0.0, 0.0, 1.0, 0.0}},
   {PIPE_FORMAT_B5G5R5A1_UNORM, 0x03e0, {0.0, 1.0, 0.0, 0.0}},
   {PIPE_FORMAT_B5G5R5A1_UNORM, 0x7c00, {1.0, 0.0, 0.0, 0.0}},
   {PIPE_FORMAT_B5G5R5A1_UNORM, 0x8000, {0.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_B5G5R5A1_UNORM, 0xffff, {1.0, 1.0, 1.0, 1.0}},

   {PIPE_FORMAT_B8G8R8A8_UNORM, 0x00000000, {0.0, 0.0, 0.0, 0.0}},
   {PIPE_FORMAT_B8G8R8A8_UNORM, 0x000000ff, {0.0, 0.0, 1.0, 0.0}},
   {PIPE_FORMAT_B8G8R8A8_UNORM, 0x0000ff00, {0.0, 1.0, 0.0, 0.0}},
   {PIPE_FORMAT_B8G8R8A8_UNORM, 0x00ff0000, {1.0, 0.0, 0.0, 0.0}},
   {PIPE_FORMAT_B8G8R8A8_UNORM, 0xff000000, {0.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_B8G8R8A8_UNORM, 0xffffffff, {1.0, 1.0, 1.0, 1.0}},

#if 0
   {PIPE_FORMAT_R8G8B8A8_UNORM, 0x00000000, {0.0, 0.0, 0.0, 0.0}},
   {PIPE_FORMAT_R8G8B8A8_UNORM, 0x000000ff, {0.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_R8G8B8A8_UNORM, 0x0000ff00, {0.0, 0.0, 1.0, 0.0}},
   {PIPE_FORMAT_R8G8B8A8_UNORM, 0x00ff0000, {0.0, 1.0, 0.0, 0.0}},
   {PIPE_FORMAT_R8G8B8A8_UNORM, 0xff000000, {1.0, 0.0, 0.0, 0.0}},
   {PIPE_FORMAT_R8G8B8A8_UNORM, 0xffffffff, {1.0, 1.0, 1.0, 1.0}},
#endif

   {PIPE_FORMAT_A8R8G8B8_UNORM, 0x00000000, {0.0, 0.0, 0.0, 0.0}},
   {PIPE_FORMAT_A8R8G8B8_UNORM, 0x000000ff, {0.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_A8R8G8B8_UNORM, 0x0000ff00, {1.0, 0.0, 0.0, 0.0}},
   {PIPE_FORMAT_A8R8G8B8_UNORM, 0x00ff0000, {0.0, 1.0, 0.0, 0.0}},
   {PIPE_FORMAT_A8R8G8B8_UNORM, 0xff000000, {0.0, 0.0, 1.0, 0.0}},
   {PIPE_FORMAT_A8R8G8B8_UNORM, 0xffffffff, {1.0, 1.0, 1.0, 1.0}},
};


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


typedef void (*load_ptr_t)(const uint32_t packed, float *);


static LLVMValueRef
add_load_rgba_test(LLVMModuleRef module,
                   const struct util_format_description *desc)
{
   LLVMTypeRef args[2];
   LLVMValueRef func;
   LLVMValueRef packed;
   LLVMValueRef rgba_ptr;
   LLVMBasicBlockRef block;
   LLVMBuilderRef builder;
   LLVMValueRef rgba;

   args[0] = LLVMInt32Type();
   args[1] = LLVMPointerType(LLVMVectorType(LLVMFloatType(), 4), 0);

   func = LLVMAddFunction(module, "load", LLVMFunctionType(LLVMVoidType(), args, 2, 0));
   LLVMSetFunctionCallConv(func, LLVMCCallConv);
   packed = LLVMGetParam(func, 0);
   rgba_ptr = LLVMGetParam(func, 1);

   block = LLVMAppendBasicBlock(func, "entry");
   builder = LLVMCreateBuilder();
   LLVMPositionBuilderAtEnd(builder, block);

   if(desc->block.bits < 32)
      packed = LLVMBuildTrunc(builder, packed, LLVMIntType(desc->block.bits), "");

   rgba = lp_build_unpack_rgba_aos(builder, desc, packed);

   LLVMBuildStore(builder, rgba, rgba_ptr);

   LLVMBuildRetVoid(builder);

   LLVMDisposeBuilder(builder);
   return func;
}


typedef void (*store_ptr_t)(uint32_t *, const float *);


static LLVMValueRef
add_store_rgba_test(LLVMModuleRef module,
                    const struct util_format_description *desc)
{
   LLVMTypeRef args[2];
   LLVMValueRef func;
   LLVMValueRef packed_ptr;
   LLVMValueRef rgba_ptr;
   LLVMBasicBlockRef block;
   LLVMBuilderRef builder;
   LLVMValueRef rgba;
   LLVMValueRef packed;

   args[0] = LLVMPointerType(LLVMInt32Type(), 0);
   args[1] = LLVMPointerType(LLVMVectorType(LLVMFloatType(), 4), 0);

   func = LLVMAddFunction(module, "store", LLVMFunctionType(LLVMVoidType(), args, 2, 0));
   LLVMSetFunctionCallConv(func, LLVMCCallConv);
   packed_ptr = LLVMGetParam(func, 0);
   rgba_ptr = LLVMGetParam(func, 1);

   block = LLVMAppendBasicBlock(func, "entry");
   builder = LLVMCreateBuilder();
   LLVMPositionBuilderAtEnd(builder, block);

   rgba = LLVMBuildLoad(builder, rgba_ptr, "");

   packed = lp_build_pack_rgba_aos(builder, desc, rgba);

   if(desc->block.bits < 32)
      packed = LLVMBuildZExt(builder, packed, LLVMInt32Type(), "");

   LLVMBuildStore(builder, packed, packed_ptr);

   LLVMBuildRetVoid(builder);

   LLVMDisposeBuilder(builder);
   return func;
}


PIPE_ALIGN_STACK
static boolean
test_format(unsigned verbose, FILE *fp, const struct pixel_test_case *test)
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

   load = add_load_rgba_test(module, desc);
   store = add_store_rgba_test(module, desc);

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

   load_ptr(test->packed, unpacked);
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

   if(fp)
      write_tsv_row(fp, desc, success);

   return success;
}


boolean
test_all(unsigned verbose, FILE *fp)
{
   unsigned i;
   bool success = TRUE;

   for (i = 0; i < sizeof(test_cases)/sizeof(test_cases[0]); ++i)
      if(!test_format(verbose, fp, &test_cases[i]))
        success = FALSE;

   return success;
}


boolean
test_some(unsigned verbose, FILE *fp, unsigned long n)
{
   return test_all(verbose, fp);
}

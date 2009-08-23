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
 * C - JIT interfaces
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */


#include <llvm-c/Transforms/Scalar.h>

#include "util/u_memory.h"
#include "lp_screen.h"
#include "lp_jit.h"


static void
lp_jit_init_types(struct llvmpipe_screen *screen)
{
   /* struct lp_jit_context */
   {
      LLVMTypeRef elem_types[4];
      LLVMTypeRef context_type;

      elem_types[0] = LLVMPointerType(LLVMFloatType(), 0); /* constants */
      elem_types[1] = LLVMPointerType(LLVMInt8Type(), 0);  /* samplers */
      elem_types[2] = LLVMFloatType();                     /* alpha_ref_value */
      elem_types[3] = LLVMPointerType(LLVMInt8Type(), 0);  /* blend_color */

      context_type = LLVMStructType(elem_types, Elements(elem_types), 0);

      LP_CHECK_MEMBER_OFFSET(struct lp_jit_context, constants,
                             screen->target, context_type, 0);
      LP_CHECK_MEMBER_OFFSET(struct lp_jit_context, samplers,
                             screen->target, context_type, 1);
      LP_CHECK_MEMBER_OFFSET(struct lp_jit_context, alpha_ref_value,
                             screen->target, context_type, 2);
      LP_CHECK_MEMBER_OFFSET(struct lp_jit_context, blend_color,
                             screen->target, context_type, 3);
      LP_CHECK_STRUCT_SIZE(struct lp_jit_context,
                           screen->target, context_type);

      LLVMAddTypeName(screen->module, "context", context_type);

      screen->context_ptr_type = LLVMPointerType(context_type, 0);
   }

#ifdef DEBUG
   LLVMDumpModule(screen->module);
#endif
}


void
lp_jit_screen_cleanup(struct llvmpipe_screen *screen)
{
   if(screen->engine)
      LLVMDisposeExecutionEngine(screen->engine);

   if(screen->pass)
      LLVMDisposePassManager(screen->pass);
}


void
lp_jit_screen_init(struct llvmpipe_screen *screen)
{
   char *error = NULL;

   screen->module = LLVMModuleCreateWithName("llvmpipe");

   screen->provider = LLVMCreateModuleProviderForExistingModule(screen->module);

   if (LLVMCreateJITCompiler(&screen->engine, screen->provider, 1, &error)) {
      fprintf(stderr, "%s\n", error);
      LLVMDisposeMessage(error);
      abort();
   }

   screen->target = LLVMGetExecutionEngineTargetData(screen->engine);

   screen->pass = LLVMCreateFunctionPassManager(screen->provider);
   LLVMAddTargetData(screen->target, screen->pass);
   /* These are the passes currently listed in llvm-c/Transforms/Scalar.h,
    * but there are more on SVN. */
   LLVMAddConstantPropagationPass(screen->pass);
   LLVMAddInstructionCombiningPass(screen->pass);
   LLVMAddPromoteMemoryToRegisterPass(screen->pass);
   LLVMAddGVNPass(screen->pass);
   LLVMAddCFGSimplificationPass(screen->pass);

   lp_jit_init_types(screen);
}

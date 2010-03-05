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
#include "util/u_cpu_detect.h"
#include "gallivm/lp_bld_init.h"
#include "lp_debug.h"
#include "lp_screen.h"
#include "gallivm/lp_bld_intr.h"
#include "lp_jit.h"


static void
lp_jit_init_globals(struct llvmpipe_screen *screen)
{
   LLVMTypeRef texture_type;

   /* struct lp_jit_texture */
   {
      LLVMTypeRef elem_types[6];

      elem_types[LP_JIT_TEXTURE_WIDTH]  = LLVMInt32Type();
      elem_types[LP_JIT_TEXTURE_HEIGHT] = LLVMInt32Type();
      elem_types[LP_JIT_TEXTURE_DEPTH] = LLVMInt32Type();
      elem_types[LP_JIT_TEXTURE_LAST_LEVEL] = LLVMInt32Type();
      elem_types[LP_JIT_TEXTURE_STRIDE] = LLVMInt32Type();
      elem_types[LP_JIT_TEXTURE_DATA]   = LLVMPointerType(LLVMInt8Type(), 0);

      texture_type = LLVMStructType(elem_types, Elements(elem_types), 0);

      LP_CHECK_MEMBER_OFFSET(struct lp_jit_texture, width,
                             screen->target, texture_type,
                             LP_JIT_TEXTURE_WIDTH);
      LP_CHECK_MEMBER_OFFSET(struct lp_jit_texture, height,
                             screen->target, texture_type,
                             LP_JIT_TEXTURE_HEIGHT);
      LP_CHECK_MEMBER_OFFSET(struct lp_jit_texture, depth,
                             screen->target, texture_type,
                             LP_JIT_TEXTURE_DEPTH);
      LP_CHECK_MEMBER_OFFSET(struct lp_jit_texture, last_level,
                             screen->target, texture_type,
                             LP_JIT_TEXTURE_LAST_LEVEL);
      LP_CHECK_MEMBER_OFFSET(struct lp_jit_texture, stride,
                             screen->target, texture_type,
                             LP_JIT_TEXTURE_STRIDE);
      LP_CHECK_MEMBER_OFFSET(struct lp_jit_texture, data,
                             screen->target, texture_type,
                             LP_JIT_TEXTURE_DATA);
      LP_CHECK_STRUCT_SIZE(struct lp_jit_texture,
                           screen->target, texture_type);

      LLVMAddTypeName(screen->module, "texture", texture_type);
   }

   /* struct lp_jit_context */
   {
      LLVMTypeRef elem_types[8];
      LLVMTypeRef context_type;

      elem_types[0] = LLVMPointerType(LLVMFloatType(), 0); /* constants */
      elem_types[1] = LLVMFloatType();                     /* alpha_ref_value */      elem_types[2] = LLVMFloatType();                     /* scissor_xmin */
      elem_types[3] = LLVMFloatType();                     /* scissor_ymin */
      elem_types[4] = LLVMFloatType();                     /* scissor_xmax */
      elem_types[5] = LLVMFloatType();                     /* scissor_ymax */
      elem_types[6] = LLVMPointerType(LLVMInt8Type(), 0);  /* blend_color */
      elem_types[7] = LLVMArrayType(texture_type, PIPE_MAX_SAMPLERS); /* textures */

      context_type = LLVMStructType(elem_types, Elements(elem_types), 0);

      LP_CHECK_MEMBER_OFFSET(struct lp_jit_context, constants,
                             screen->target, context_type, 0);
      LP_CHECK_MEMBER_OFFSET(struct lp_jit_context, alpha_ref_value,
                             screen->target, context_type, 1);
      LP_CHECK_MEMBER_OFFSET(struct lp_jit_context, scissor_xmin,
                             screen->target, context_type, 2);
      LP_CHECK_MEMBER_OFFSET(struct lp_jit_context, scissor_ymin,
                             screen->target, context_type, 3);
      LP_CHECK_MEMBER_OFFSET(struct lp_jit_context, scissor_xmax,
                             screen->target, context_type, 4);
      LP_CHECK_MEMBER_OFFSET(struct lp_jit_context, scissor_ymax,
                             screen->target, context_type, 5);
      LP_CHECK_MEMBER_OFFSET(struct lp_jit_context, blend_color,
                             screen->target, context_type, 6);
      LP_CHECK_MEMBER_OFFSET(struct lp_jit_context, textures,
                             screen->target, context_type,
                             LP_JIT_CONTEXT_TEXTURES_INDEX);
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

   util_cpu_detect();

#if 0
   /* For simulating less capable machines */
   util_cpu_caps.has_sse3 = 0;
   util_cpu_caps.has_ssse3 = 0;
   util_cpu_caps.has_sse4_1 = 0;
#endif

   lp_build_init();

   screen->module = LLVMModuleCreateWithName("llvmpipe");

   screen->provider = LLVMCreateModuleProviderForExistingModule(screen->module);

   if (LLVMCreateJITCompiler(&screen->engine, screen->provider, 1, &error)) {
      _debug_printf("%s\n", error);
      LLVMDisposeMessage(error);
      assert(0);
   }

   screen->target = LLVMGetExecutionEngineTargetData(screen->engine);

   screen->pass = LLVMCreateFunctionPassManager(screen->provider);
   LLVMAddTargetData(screen->target, screen->pass);

   if ((LP_DEBUG & DEBUG_NO_LLVM_OPT) == 0) {
      /* These are the passes currently listed in llvm-c/Transforms/Scalar.h,
       * but there are more on SVN. */
      /* TODO: Add more passes */
      LLVMAddConstantPropagationPass(screen->pass);
      if(util_cpu_caps.has_sse4_1) {
         /* FIXME: There is a bug in this pass, whereby the combination of fptosi
          * and sitofp (necessary for trunc/floor/ceil/round implementation)
          * somehow becomes invalid code.
          */
         LLVMAddInstructionCombiningPass(screen->pass);
      }
      LLVMAddPromoteMemoryToRegisterPass(screen->pass);
      LLVMAddGVNPass(screen->pass);
      LLVMAddCFGSimplificationPass(screen->pass);
   }

   lp_jit_init_globals(screen);
}

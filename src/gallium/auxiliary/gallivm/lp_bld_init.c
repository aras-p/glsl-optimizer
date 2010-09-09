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


#include "pipe/p_compiler.h"
#include "util/u_cpu_detect.h"
#include "util/u_debug.h"
#include "lp_bld_debug.h"
#include "lp_bld_init.h"

#include <llvm-c/Transforms/Scalar.h>


#ifdef DEBUG
unsigned gallivm_debug = 0;

static const struct debug_named_value lp_bld_debug_flags[] = {
   { "tgsi",   GALLIVM_DEBUG_TGSI, NULL },
   { "ir",     GALLIVM_DEBUG_IR, NULL },
   { "asm",    GALLIVM_DEBUG_ASM, NULL },
   { "nopt",   GALLIVM_DEBUG_NO_OPT, NULL },
   { "perf",   GALLIVM_DEBUG_PERF, NULL },
   DEBUG_NAMED_VALUE_END
};

DEBUG_GET_ONCE_FLAGS_OPTION(gallivm_debug, "GALLIVM_DEBUG", lp_bld_debug_flags, 0)
#endif


LLVMModuleRef lp_build_module = NULL;
LLVMExecutionEngineRef lp_build_engine = NULL;
LLVMModuleProviderRef lp_build_provider = NULL;
LLVMTargetDataRef lp_build_target = NULL;
LLVMPassManagerRef lp_build_pass = NULL;


/*
 * Optimization values are:
 * - 0: None (-O0)
 * - 1: Less (-O1)
 * - 2: Default (-O2, -Os)
 * - 3: Aggressive (-O3)
 *
 * See also CodeGenOpt::Level in llvm/Target/TargetMachine.h
 */
enum LLVM_CodeGenOpt_Level {
#if HAVE_LLVM >= 0x207
   None,        // -O0
   Less,        // -O1
   Default,     // -O2, -Os
   Aggressive   // -O3
#else
   Default,
   None,
   Aggressive
#endif
};


extern void
lp_register_oprofile_jit_event_listener(LLVMExecutionEngineRef EE);

extern void
lp_set_target_options(void);


void
lp_build_init(void)
{
#ifdef DEBUG
   gallivm_debug = debug_get_option_gallivm_debug();
#endif

   lp_set_target_options();

   LLVMInitializeNativeTarget();

   LLVMLinkInJIT();

   if (!lp_build_module)
      lp_build_module = LLVMModuleCreateWithName("gallivm");

   if (!lp_build_provider)
      lp_build_provider = LLVMCreateModuleProviderForExistingModule(lp_build_module);

   if (!lp_build_engine) {
      enum LLVM_CodeGenOpt_Level optlevel;
      char *error = NULL;

      if (gallivm_debug & GALLIVM_DEBUG_NO_OPT) {
         optlevel = None;
      }
      else {
         optlevel = Default;
      }

      if (LLVMCreateJITCompiler(&lp_build_engine, lp_build_provider,
                                (unsigned)optlevel, &error)) {
         _debug_printf("%s\n", error);
         LLVMDisposeMessage(error);
         assert(0);
      }

#if defined(DEBUG) || defined(PROFILE)
      lp_register_oprofile_jit_event_listener(lp_build_engine);
#endif
   }

   if (!lp_build_target)
      lp_build_target = LLVMGetExecutionEngineTargetData(lp_build_engine);

   if (!lp_build_pass) {
      lp_build_pass = LLVMCreateFunctionPassManager(lp_build_provider);
      LLVMAddTargetData(lp_build_target, lp_build_pass);

      if ((gallivm_debug & GALLIVM_DEBUG_NO_OPT) == 0) {
         /* These are the passes currently listed in llvm-c/Transforms/Scalar.h,
          * but there are more on SVN. */
         /* TODO: Add more passes */
         LLVMAddCFGSimplificationPass(lp_build_pass);
         LLVMAddPromoteMemoryToRegisterPass(lp_build_pass);
         LLVMAddConstantPropagationPass(lp_build_pass);
         if(util_cpu_caps.has_sse4_1) {
            /* FIXME: There is a bug in this pass, whereby the combination of fptosi
             * and sitofp (necessary for trunc/floor/ceil/round implementation)
             * somehow becomes invalid code.
             */
            LLVMAddInstructionCombiningPass(lp_build_pass);
         }
         LLVMAddGVNPass(lp_build_pass);
      } else {
         /* We need at least this pass to prevent the backends to fail in
          * unexpected ways.
          */
         LLVMAddPromoteMemoryToRegisterPass(lp_build_pass);
      }
   }

   util_cpu_detect();

#if 0
   /* For simulating less capable machines */
   util_cpu_caps.has_sse3 = 0;
   util_cpu_caps.has_ssse3 = 0;
   util_cpu_caps.has_sse4_1 = 0;
#endif
}


/* 
 * Hack to allow the linking of release LLVM static libraries on a debug build.
 *
 * See also:
 * - http://social.msdn.microsoft.com/Forums/en-US/vclanguage/thread/7234ea2b-0042-42ed-b4e2-5d8644dfb57d
 */
#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdefs.h>
_CRTIMP void __cdecl
_invalid_parameter_noinfo(void) {}
#endif

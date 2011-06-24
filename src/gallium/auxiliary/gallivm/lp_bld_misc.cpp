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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 **************************************************************************/


#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif

#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JITEventListener.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/PrettyStackTrace.h>

#include "pipe/p_config.h"
#include "util/u_debug.h"


/**
 * Register the engine with oprofile.
 *
 * This allows to see the LLVM IR function names in oprofile output.
 *
 * To actually work LLVM needs to be built with the --with-oprofile configure
 * option.
 *
 * Also a oprofile:oprofile user:group is necessary. Which is not created by
 * default on some distributions.
 */
extern "C" void
lp_register_oprofile_jit_event_listener(LLVMExecutionEngineRef EE)
{
   llvm::unwrap(EE)->RegisterJITEventListener(llvm::createOProfileJITEventListener());
}


extern "C" void
lp_set_target_options(void)
{
#if defined(DEBUG)
#if HAVE_LLVM >= 0x0207
   llvm::JITEmitDebugInfo = true;
#endif
#endif

   /*
    * LLVM revision 123367 switched the default stack alignment to 16 bytes on
    * Linux (and several other Unices in later revisions), to match recent gcc
    * versions.
    *
    * However our drivers can be loaded by old binary applications, still
    * maintaining a 4 bytes stack alignment.  Therefore we must tell LLVM here
    * to only assume a 4 bytes alignment for backwards compatibility.
    */
#if defined(PIPE_ARCH_X86)
#if HAVE_LLVM >= 0x0300
   llvm::StackAlignmentOverride = 4;
#else
   llvm::StackAlignment = 4;
#endif
#endif

#if defined(DEBUG) || defined(PROFILE)
   llvm::NoFramePointerElim = true;
#endif

   llvm::NoExcessFPPrecision = false;

   /* XXX: Investigate this */
#if 0
   llvm::UnsafeFPMath = true;
#endif

#if HAVE_LLVM < 0x0209
   /*
    * LLVM will generate MMX instructions for vectors <= 64 bits, leading to
    * innefficient code, and in 32bit systems, to the corruption of the FPU
    * stack given that it expects the user to generate the EMMS instructions.
    *
    * See also:
    * - http://llvm.org/bugs/show_bug.cgi?id=3287
    * - http://l4.me.uk/post/2009/06/07/llvm-wrinkle-3-configuration-what-configuration/
    *
    * The -disable-mmx global option can be specified only once  since we
    * dynamically link against LLVM it will reside in a separate shared object,
    * which may or not be delete when this shared object is, so we use the
    * llvm::DisablePrettyStackTrace variable (which we set below and should
    * reside in the same shared library) to determine whether the -disable-mmx
    * option has been set or not.
    *
    * Thankfully this ugly hack is not necessary on LLVM 2.9 onwards.
    */
   if (!llvm::DisablePrettyStackTrace) {
      static boolean first = TRUE;
      static const char* options[] = {
         "prog",
         "-disable-mmx"
      };
      assert(first);
      llvm::cl::ParseCommandLineOptions(2, const_cast<char**>(options));
      first = FALSE;
   }
#endif

   /*
    * By default LLVM adds a signal handler to output a pretty stack trace.
    * This signal handler is never removed, causing problems when unloading the
    * shared object where the gallium driver resides.
    */
   llvm::DisablePrettyStackTrace = true;
}


extern "C" void
lp_func_delete_body(LLVMValueRef FF)
{
   llvm::Function *func = llvm::unwrap<llvm::Function>(FF);
   func->deleteBody();
}


extern "C"
LLVMValueRef
lp_build_load_volatile(LLVMBuilderRef B, LLVMValueRef PointerVal,
                       const char *Name)
{
   return llvm::wrap(llvm::unwrap(B)->CreateLoad(llvm::unwrap(PointerVal), true, Name));
}


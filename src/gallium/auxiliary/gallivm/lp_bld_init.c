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
#include "util/u_memory.h"
#include "util/u_simple_list.h"
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
   { "no_brilinear", GALLIVM_DEBUG_NO_BRILINEAR, NULL },
   { "gc",     GALLIVM_DEBUG_GC, NULL },
   DEBUG_NAMED_VALUE_END
};

DEBUG_GET_ONCE_FLAGS_OPTION(gallivm_debug, "GALLIVM_DEBUG", lp_bld_debug_flags, 0)
#endif


static boolean gallivm_initialized = FALSE;


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


/**
 * LLVM 2.6 permits only one ExecutionEngine to be created.  This is it.
 */
static LLVMExecutionEngineRef GlobalEngine = NULL;

/**
 * Same gallivm state shared by all contexts.
 */
static struct gallivm_state *GlobalGallivm = NULL;




extern void
lp_register_oprofile_jit_event_listener(LLVMExecutionEngineRef EE);

extern void
lp_set_target_options(void);



/**
 * Create the LLVM (optimization) pass manager and install
 * relevant optimization passes.
 * \return  TRUE for success, FALSE for failure
 */
static boolean
create_pass_manager(struct gallivm_state *gallivm)
{
   assert(!gallivm->passmgr);

   gallivm->passmgr = LLVMCreateFunctionPassManager(gallivm->provider);
   if (!gallivm->passmgr)
      return FALSE;

   LLVMAddTargetData(gallivm->target, gallivm->passmgr);

   if ((gallivm_debug & GALLIVM_DEBUG_NO_OPT) == 0) {
      /* These are the passes currently listed in llvm-c/Transforms/Scalar.h,
       * but there are more on SVN.
       * TODO: Add more passes.
       */
      LLVMAddCFGSimplificationPass(gallivm->passmgr);

      if (HAVE_LLVM >= 0x207 && sizeof(void*) == 4) {
         /* For LLVM >= 2.7 and 32-bit build, use this order of passes to
          * avoid generating bad code.
          * Test with piglit glsl-vs-sqrt-zero test.
          */
         LLVMAddConstantPropagationPass(gallivm->passmgr);
         LLVMAddPromoteMemoryToRegisterPass(gallivm->passmgr);
      }
      else {
         LLVMAddPromoteMemoryToRegisterPass(gallivm->passmgr);
         LLVMAddConstantPropagationPass(gallivm->passmgr);
      }

      if (util_cpu_caps.has_sse4_1) {
         /* FIXME: There is a bug in this pass, whereby the combination
          * of fptosi and sitofp (necessary for trunc/floor/ceil/round
          * implementation) somehow becomes invalid code.
          */
         LLVMAddInstructionCombiningPass(gallivm->passmgr);
      }
      LLVMAddGVNPass(gallivm->passmgr);
   }
   else {
      /* We need at least this pass to prevent the backends to fail in
       * unexpected ways.
       */
      LLVMAddPromoteMemoryToRegisterPass(gallivm->passmgr);
   }

   return TRUE;
}


/**
 * Free gallivm object's LLVM allocations, but not the gallivm object itself.
 */
static void
free_gallivm_state(struct gallivm_state *gallivm)
{
#if HAVE_LLVM >= 0x207 /* XXX or 0x208? */
   /* This leads to crashes w/ some versions of LLVM */
   LLVMModuleRef mod;
   char *error;

   if (gallivm->engine && gallivm->provider)
      LLVMRemoveModuleProvider(gallivm->engine, gallivm->provider,
                               &mod, &error);
#endif

#if 0
   /* XXX this seems to crash with all versions of LLVM */
   if (gallivm->provider)
      LLVMDisposeModuleProvider(gallivm->provider);
#endif

   if (gallivm->passmgr)
      LLVMDisposePassManager(gallivm->passmgr);

#if HAVE_LLVM >= 0x207
   if (gallivm->module)
      LLVMDisposeModule(gallivm->module);
#endif

#if 0
   /* Don't free the exec engine, it's a global/singleton */
   if (gallivm->engine)
      LLVMDisposeExecutionEngine(gallivm->engine);
#endif

#if 0
   /* Don't free the TargetData, it's owned by the exec engine */
   LLVMDisposeTargetData(gallivm->target);
#endif

   if (gallivm->context)
      LLVMContextDispose(gallivm->context);

   if (gallivm->builder)
      LLVMDisposeBuilder(gallivm->builder);

   gallivm->engine = NULL;
   gallivm->target = NULL;
   gallivm->module = NULL;
   gallivm->provider = NULL;
   gallivm->passmgr = NULL;
   gallivm->context = NULL;
   gallivm->builder = NULL;
}


/**
 * Allocate gallivm LLVM objects.
 * \return  TRUE for success, FALSE for failure
 */
static boolean
init_gallivm_state(struct gallivm_state *gallivm)
{
   assert(!gallivm->context);
   assert(!gallivm->module);
   assert(!gallivm->provider);

   lp_build_init();

   gallivm->context = LLVMContextCreate();
   if (!gallivm->context)
      goto fail;

   gallivm->module = LLVMModuleCreateWithNameInContext("gallivm",
                                                       gallivm->context);
   if (!gallivm->module)
      goto fail;

   gallivm->provider =
      LLVMCreateModuleProviderForExistingModule(gallivm->module);
   if (!gallivm->provider)
      goto fail;

   if (!GlobalEngine) {
      /* We can only create one LLVMExecutionEngine (w/ LLVM 2.6 anyway) */
      enum LLVM_CodeGenOpt_Level optlevel;
      char *error = NULL;

      if (gallivm_debug & GALLIVM_DEBUG_NO_OPT) {
         optlevel = None;
      }
      else {
         optlevel = Default;
      }

      if (LLVMCreateJITCompiler(&GlobalEngine, gallivm->provider,
                                (unsigned) optlevel, &error)) {
         _debug_printf("%s\n", error);
         LLVMDisposeMessage(error);
         goto fail;
      }

#if defined(DEBUG) || defined(PROFILE)
      lp_register_oprofile_jit_event_listener(GlobalEngine);
#endif
   }

   gallivm->engine = GlobalEngine;

   LLVMAddModuleProvider(gallivm->engine, gallivm->provider);//new

   gallivm->target = LLVMGetExecutionEngineTargetData(gallivm->engine);
   if (!gallivm->target)
      goto fail;

   if (!create_pass_manager(gallivm))
      goto fail;

   gallivm->builder = LLVMCreateBuilderInContext(gallivm->context);
   if (!gallivm->builder)
      goto fail;

   return TRUE;

fail:
   free_gallivm_state(gallivm);
   return FALSE;
}


struct callback
{
   garbage_collect_callback_func func;
   void *cb_data;
   struct callback *prev, *next;
};


/** list of all garbage collector callbacks */
static struct callback callback_list = {NULL, NULL, NULL, NULL};


/**
 * Register a function with gallivm which will be called when we
 * do garbage collection.
 */
void
gallivm_register_garbage_collector_callback(garbage_collect_callback_func func,
                                            void *cb_data)
{
   struct callback *cb;

   if (!callback_list.prev) {
      make_empty_list(&callback_list);
   }

   /* see if already in list */
   foreach(cb, &callback_list) {
      if (cb->func == func && cb->cb_data == cb_data)
         return;
   }

   /* add to list */
   cb = CALLOC_STRUCT(callback);
   if (cb) {
      cb->func = func;
      cb->cb_data = cb_data;
      insert_at_head(&callback_list, cb);
   }
}


/**
 * Remove a callback.
 */
void
gallivm_remove_garbage_collector_callback(garbage_collect_callback_func func,
                                          void *cb_data)
{
   struct callback *cb;

   /* search list */
   foreach(cb, &callback_list) {
      if (cb->func == func && cb->cb_data == cb_data) {
         /* found, remove it */
         remove_from_list(cb);
         FREE(cb);
         return;
      }
   }
}


/**
 * Call the callback functions (which are typically in the
 * draw module and llvmpipe driver.
 */
static void
call_garbage_collector_callbacks(void)
{
   struct callback *cb;
   foreach(cb, &callback_list) {
      cb->func(cb->cb_data);
   }
}



/**
 * Other gallium components using gallivm should call this periodically
 * to let us do garbage collection (or at least try to free memory
 * accumulated by the LLVM libraries).
 */
void
gallivm_garbage_collect(struct gallivm_state *gallivm)
{
   if (gallivm->context) {
      if (gallivm_debug & GALLIVM_DEBUG_GC)
         debug_printf("***** Doing LLVM garbage collection\n");

      call_garbage_collector_callbacks();
      free_gallivm_state(gallivm);
      init_gallivm_state(gallivm);
   }
}


void
lp_build_init(void)
{
   if (gallivm_initialized)
      return;

#ifdef DEBUG
   gallivm_debug = debug_get_option_gallivm_debug();
#endif

   lp_set_target_options();

   LLVMInitializeNativeTarget();

   LLVMLinkInJIT();

   util_cpu_detect();
 
   gallivm_initialized = TRUE;

#if 0
   /* For simulating less capable machines */
   util_cpu_caps.has_sse3 = 0;
   util_cpu_caps.has_ssse3 = 0;
   util_cpu_caps.has_sse4_1 = 0;
#endif
}



/**
 * Create a new gallivm_state object.
 * Note that we return a singleton.
 */
struct gallivm_state *
gallivm_create(void)
{
   if (!GlobalGallivm) {
      GlobalGallivm = CALLOC_STRUCT(gallivm_state);
      if (GlobalGallivm) {
         if (!init_gallivm_state(GlobalGallivm)) {
            FREE(GlobalGallivm);
            GlobalGallivm = NULL;
         }
      }
   }
   return GlobalGallivm;
}


/**
 * Destroy a gallivm_state object.
 */
void
gallivm_destroy(struct gallivm_state *gallivm)
{
   /* No-op: don't destroy the singleton */
   (void) gallivm;
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

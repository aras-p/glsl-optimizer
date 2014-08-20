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


/**
 * The purpose of this module is to expose LLVM functionality not available
 * through the C++ bindings.
 */


#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif

// Undef these vars just to silence warnings
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION


#include <stddef.h>

#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ADT/Triple.h>
#include <llvm/ExecutionEngine/JITMemoryManager.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/PrettyStackTrace.h>

#include <llvm/Support/TargetSelect.h>

#if HAVE_LLVM >= 0x0303
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/CBindingWrapping.h>
#endif

#include "pipe/p_config.h"
#include "util/u_debug.h"
#include "util/u_cpu_detect.h"

#include "lp_bld_misc.h"

namespace {

class LLVMEnsureMultithreaded {
public:
   LLVMEnsureMultithreaded()
   {
#if HAVE_LLVM < 0x0303
      if (!llvm::llvm_is_multithreaded()) {
         llvm::llvm_start_multithreaded();
      }
#else
      if (!LLVMIsMultithreaded()) {
         LLVMStartMultithreaded();
      }
#endif
   }
};

static LLVMEnsureMultithreaded lLVMEnsureMultithreaded;

}

extern "C" void
lp_set_target_options(void)
{
#if HAVE_LLVM < 0x0304
   /*
    * By default LLVM adds a signal handler to output a pretty stack trace.
    * This signal handler is never removed, causing problems when unloading the
    * shared object where the gallium driver resides.
    */
   llvm::DisablePrettyStackTrace = true;
#endif

   // If we have a native target, initialize it to ensure it is linked in and
   // usable by the JIT.
   llvm::InitializeNativeTarget();

   llvm::InitializeNativeTargetAsmPrinter();

   llvm::InitializeNativeTargetDisassembler();
}


extern "C"
LLVMValueRef
lp_build_load_volatile(LLVMBuilderRef B, LLVMValueRef PointerVal,
                       const char *Name)
{
   return llvm::wrap(llvm::unwrap(B)->CreateLoad(llvm::unwrap(PointerVal), true, Name));
}


extern "C"
void
lp_set_load_alignment(LLVMValueRef Inst,
                       unsigned Align)
{
   llvm::unwrap<llvm::LoadInst>(Inst)->setAlignment(Align);
}

extern "C"
void
lp_set_store_alignment(LLVMValueRef Inst,
		       unsigned Align)
{
   llvm::unwrap<llvm::StoreInst>(Inst)->setAlignment(Align);
}


/*
 * Delegating is tedious but the default manager class is hidden in an
 * anonymous namespace in LLVM, so we cannot just derive from it to change
 * its behavior.
 */
class DelegatingJITMemoryManager : public llvm::JITMemoryManager {

   protected:
      virtual llvm::JITMemoryManager *mgr() const = 0;

   public:
      /*
       * From JITMemoryManager
       */
      virtual void setMemoryWritable() {
         mgr()->setMemoryWritable();
      }
      virtual void setMemoryExecutable() {
         mgr()->setMemoryExecutable();
      }
      virtual void setPoisonMemory(bool poison) {
         mgr()->setPoisonMemory(poison);
      }
      virtual void AllocateGOT() {
         mgr()->AllocateGOT();
         /*
          * isManagingGOT() is not virtual in base class so we can't delegate.
          * Instead we mirror the value of HasGOT in our instance.
          */
         HasGOT = mgr()->isManagingGOT();
      }
      virtual uint8_t *getGOTBase() const {
         return mgr()->getGOTBase();
      }
      virtual uint8_t *startFunctionBody(const llvm::Function *F,
                                         uintptr_t &ActualSize) {
         return mgr()->startFunctionBody(F, ActualSize);
      }
      virtual uint8_t *allocateStub(const llvm::GlobalValue *F,
                                    unsigned StubSize,
                                    unsigned Alignment) {
         return mgr()->allocateStub(F, StubSize, Alignment);
      }
      virtual void endFunctionBody(const llvm::Function *F,
                                   uint8_t *FunctionStart,
                                   uint8_t *FunctionEnd) {
         mgr()->endFunctionBody(F, FunctionStart, FunctionEnd);
      }
      virtual uint8_t *allocateSpace(intptr_t Size, unsigned Alignment) {
         return mgr()->allocateSpace(Size, Alignment);
      }
      virtual uint8_t *allocateGlobal(uintptr_t Size, unsigned Alignment) {
         return mgr()->allocateGlobal(Size, Alignment);
      }
      virtual void deallocateFunctionBody(void *Body) {
         mgr()->deallocateFunctionBody(Body);
      }
#if HAVE_LLVM < 0x0304
      virtual uint8_t *startExceptionTable(const llvm::Function *F,
                                           uintptr_t &ActualSize) {
         return mgr()->startExceptionTable(F, ActualSize);
      }
      virtual void endExceptionTable(const llvm::Function *F,
                                     uint8_t *TableStart,
                                     uint8_t *TableEnd,
                                     uint8_t *FrameRegister) {
         mgr()->endExceptionTable(F, TableStart, TableEnd,
                                  FrameRegister);
      }
      virtual void deallocateExceptionTable(void *ET) {
         mgr()->deallocateExceptionTable(ET);
      }
#endif
      virtual bool CheckInvariants(std::string &s) {
         return mgr()->CheckInvariants(s);
      }
      virtual size_t GetDefaultCodeSlabSize() {
         return mgr()->GetDefaultCodeSlabSize();
      }
      virtual size_t GetDefaultDataSlabSize() {
         return mgr()->GetDefaultDataSlabSize();
      }
      virtual size_t GetDefaultStubSlabSize() {
         return mgr()->GetDefaultStubSlabSize();
      }
      virtual unsigned GetNumCodeSlabs() {
         return mgr()->GetNumCodeSlabs();
      }
      virtual unsigned GetNumDataSlabs() {
         return mgr()->GetNumDataSlabs();
      }
      virtual unsigned GetNumStubSlabs() {
         return mgr()->GetNumStubSlabs();
      }

      /*
       * From RTDyldMemoryManager
       */
#if HAVE_LLVM >= 0x0304
      virtual uint8_t *allocateCodeSection(uintptr_t Size,
                                           unsigned Alignment,
                                           unsigned SectionID,
                                           llvm::StringRef SectionName) {
         return mgr()->allocateCodeSection(Size, Alignment, SectionID,
                                           SectionName);
      }
#else
      virtual uint8_t *allocateCodeSection(uintptr_t Size,
                                           unsigned Alignment,
                                           unsigned SectionID) {
         return mgr()->allocateCodeSection(Size, Alignment, SectionID);
      }
#endif
#if HAVE_LLVM >= 0x0303
      virtual uint8_t *allocateDataSection(uintptr_t Size,
                                           unsigned Alignment,
                                           unsigned SectionID,
#if HAVE_LLVM >= 0x0304
                                           llvm::StringRef SectionName,
#endif
                                           bool IsReadOnly) {
         return mgr()->allocateDataSection(Size, Alignment, SectionID,
#if HAVE_LLVM >= 0x0304
                                           SectionName,
#endif
                                           IsReadOnly);
      }
#if HAVE_LLVM >= 0x0304
      virtual void registerEHFrames(uint8_t *Addr, uint64_t LoadAddr, size_t Size) {
         mgr()->registerEHFrames(Addr, LoadAddr, Size);
      }
      virtual void deregisterEHFrames(uint8_t *Addr, uint64_t LoadAddr, size_t Size) {
         mgr()->deregisterEHFrames(Addr, LoadAddr, Size);
      }
#else
      virtual void registerEHFrames(llvm::StringRef SectionData) {
         mgr()->registerEHFrames(SectionData);
      }
#endif
#else
      virtual uint8_t *allocateDataSection(uintptr_t Size,
                                           unsigned Alignment,
                                           unsigned SectionID) {
         return mgr()->allocateDataSection(Size, Alignment, SectionID);
      }
#endif
      virtual void *getPointerToNamedFunction(const std::string &Name,
                                              bool AbortOnFailure=true) {
         return mgr()->getPointerToNamedFunction(Name, AbortOnFailure);
      }
#if HAVE_LLVM == 0x0303
      virtual bool applyPermissions(std::string *ErrMsg = 0) {
         return mgr()->applyPermissions(ErrMsg);
      }
#elif HAVE_LLVM > 0x0303
      virtual bool finalizeMemory(std::string *ErrMsg = 0) {
         return mgr()->finalizeMemory(ErrMsg);
      }
#endif
};


/*
 * Delegate memory management to one shared manager for more efficient use
 * of memory than creating a separate pool for each LLVM engine.
 * Keep generated code until freeGeneratedCode() is called, instead of when
 * memory manager is destroyed, which happens during engine destruction.
 * This allows additional memory savings as we don't have to keep the engine
 * around in order to use the code.
 * All methods are delegated to the shared manager except destruction and
 * deallocating code.  For the latter we just remember what needs to be
 * deallocated later.  The shared manager is deleted once it is empty.
 */
class ShaderMemoryManager : public DelegatingJITMemoryManager {

   static llvm::JITMemoryManager *TheMM;
   static unsigned NumUsers;

   struct GeneratedCode {
      typedef std::vector<void *> Vec;
      Vec FunctionBody, ExceptionTable;

      GeneratedCode() {
         ++NumUsers;
      }

      ~GeneratedCode() {
         /*
          * Deallocate things as previously requested and
          * free shared manager when no longer used.
          */
	 Vec::iterator i;

	 assert(TheMM);
	 for ( i = FunctionBody.begin(); i != FunctionBody.end(); ++i )
	    TheMM->deallocateFunctionBody(*i);
#if HAVE_LLVM < 0x0304
	 for ( i = ExceptionTable.begin(); i != ExceptionTable.end(); ++i )
	    TheMM->deallocateExceptionTable(*i);
#endif
         --NumUsers;
         if (NumUsers == 0) {
            delete TheMM;
            TheMM = 0;
         }
      }
   };

   GeneratedCode *code;

   llvm::JITMemoryManager *mgr() const {
      if (!TheMM) {
         TheMM = CreateDefaultMemManager();
      }
      return TheMM;
   }

   public:

      ShaderMemoryManager() {
         code = new GeneratedCode;
      }

      virtual ~ShaderMemoryManager() {
         /*
          * 'code' is purposely not deleted.  It is the user's responsibility
          * to call getGeneratedCode() and freeGeneratedCode().
          */
      }

      struct lp_generated_code *getGeneratedCode() {
         return (struct lp_generated_code *) code;
      }

      static void freeGeneratedCode(struct lp_generated_code *code) {
         delete (GeneratedCode *) code;
      }

#if HAVE_LLVM < 0x0304
      virtual void deallocateExceptionTable(void *ET) {
         // remember for later deallocation
         code->ExceptionTable.push_back(ET);
      }
#endif

      virtual void deallocateFunctionBody(void *Body) {
         // remember for later deallocation
         code->FunctionBody.push_back(Body);
      }
};

llvm::JITMemoryManager *ShaderMemoryManager::TheMM = 0;
unsigned ShaderMemoryManager::NumUsers = 0;


/**
 * Same as LLVMCreateJITCompilerForModule, but:
 * - allows using MCJIT and enabling AVX feature where available.
 * - set target options
 *
 * See also:
 * - llvm/lib/ExecutionEngine/ExecutionEngineBindings.cpp
 * - llvm/tools/lli/lli.cpp
 * - http://markmail.org/message/ttkuhvgj4cxxy2on#query:+page:1+mid:aju2dggerju3ivd3+state:results
 */
extern "C"
LLVMBool
lp_build_create_jit_compiler_for_module(LLVMExecutionEngineRef *OutJIT,
                                        lp_generated_code **OutCode,
                                        LLVMModuleRef M,
                                        unsigned OptLevel,
                                        int useMCJIT,
                                        char **OutError)
{
   using namespace llvm;

   std::string Error;
#if HAVE_LLVM >= 0x0306
   EngineBuilder builder(std::unique_ptr<Module>(unwrap(M)));
#else
   EngineBuilder builder(unwrap(M));
#endif

   /**
    * LLVM 3.1+ haven't more "extern unsigned llvm::StackAlignmentOverride" and
    * friends for configuring code generation options, like stack alignment.
    */
   TargetOptions options;
#if defined(PIPE_ARCH_X86)
   options.StackAlignmentOverride = 4;
#if HAVE_LLVM < 0x0304
   options.RealignStack = true;
#endif
#endif

#if defined(DEBUG)
   options.JITEmitDebugInfo = true;
#endif

#if defined(DEBUG) || defined(PROFILE)
#if HAVE_LLVM < 0x0304
   options.NoFramePointerElimNonLeaf = true;
#endif
   options.NoFramePointerElim = true;
#endif

   builder.setEngineKind(EngineKind::JIT)
          .setErrorStr(&Error)
          .setTargetOptions(options)
          .setOptLevel((CodeGenOpt::Level)OptLevel);

   if (useMCJIT) {
       builder.setUseMCJIT(true);
#ifdef _WIN32
       /*
        * MCJIT works on Windows, but currently only through ELF object format.
        */
       std::string targetTriple = llvm::sys::getProcessTriple();
       targetTriple.append("-elf");
       unwrap(M)->setTargetTriple(targetTriple);
#endif
   }

   llvm::SmallVector<std::string, 1> MAttrs;
   if (util_cpu_caps.has_avx) {
      /*
       * AVX feature is not automatically detected from CPUID by the X86 target
       * yet, because the old (yet default) JIT engine is not capable of
       * emitting the opcodes. On newer llvm versions it is and at least some
       * versions (tested with 3.3) will emit avx opcodes without this anyway.
       */
      MAttrs.push_back("+avx");
      if (util_cpu_caps.has_f16c) {
         MAttrs.push_back("+f16c");
      }
      builder.setMAttrs(MAttrs);
   }

#if HAVE_LLVM >= 0x0305
   StringRef MCPU = llvm::sys::getHostCPUName();
   /*
    * The cpu bits are no longer set automatically, so need to set mcpu manually.
    * Note that the MAttrs set above will be sort of ignored (since we should
    * not set any which would not be set by specifying the cpu anyway).
    * It ought to be safe though since getHostCPUName() should include bits
    * not only from the cpu but environment as well (for instance if it's safe
    * to use avx instructions which need OS support). According to
    * http://llvm.org/bugs/show_bug.cgi?id=19429 however if I understand this
    * right it may be necessary to specify older cpu (or disable mattrs) though
    * when not using MCJIT so no instructions are generated which the old JIT
    * can't handle. Not entirely sure if we really need to do anything yet.
    */
   builder.setMCPU(MCPU);
#endif

   ShaderMemoryManager *MM = new ShaderMemoryManager();
   *OutCode = MM->getGeneratedCode();

   builder.setJITMemoryManager(MM);

   ExecutionEngine *JIT;

#if HAVE_LLVM >= 0x0302
   JIT = builder.create();
#else
   /*
    * Workaround http://llvm.org/PR12833
    */
   StringRef MArch = "";
   StringRef MCPU = "";
   Triple TT(unwrap(M)->getTargetTriple());
   JIT = builder.create(builder.selectTarget(TT, MArch, MCPU, MAttrs));
#endif
   if (JIT) {
      *OutJIT = wrap(JIT);
      return 0;
   }
   lp_free_generated_code(*OutCode);
   *OutCode = 0;
   delete MM;
   *OutError = strdup(Error.c_str());
   return 1;
}


extern "C"
void
lp_free_generated_code(struct lp_generated_code *code)
{
   ShaderMemoryManager::freeGeneratedCode(code);
}

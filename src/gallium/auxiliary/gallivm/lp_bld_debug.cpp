/**************************************************************************
 *
 * Copyright 2009-2011 VMware, Inc.
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

#include <stddef.h>

#include <llvm-c/Core.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetInstrInfo.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/Format.h>
#include <llvm/Support/MemoryObject.h>

#if HAVE_LLVM >= 0x0306
#include <llvm/Target/TargetSubtargetInfo.h>
#endif

#include <llvm/Support/TargetRegistry.h>
#include <llvm/MC/MCSubtargetInfo.h>

#include <llvm/Support/Host.h>

#if HAVE_LLVM >= 0x0303
#include <llvm/IR/Module.h>
#else
#include <llvm/Module.h>
#endif

#include <llvm/MC/MCDisassembler.h>
#include <llvm/MC/MCAsmInfo.h>
#include <llvm/MC/MCInst.h>
#include <llvm/MC/MCInstPrinter.h>
#include <llvm/MC/MCRegisterInfo.h>

#if HAVE_LLVM >= 0x0305
#define OwningPtr std::unique_ptr
#elif HAVE_LLVM >= 0x0303
#include <llvm/ADT/OwningPtr.h>
#endif

#if HAVE_LLVM >= 0x0305
#include <llvm/MC/MCContext.h>
#endif

#include "util/u_math.h"
#include "util/u_debug.h"

#include "lp_bld_debug.h"

#ifdef __linux__
#include <sys/stat.h>
#include <fcntl.h>
#endif



/**
 * Check alignment.
 *
 * It is important that this check is not implemented as a macro or inlined
 * function, as the compiler assumptions in respect to alignment of global
 * and stack variables would often make the check a no op, defeating the
 * whole purpose of the exercise.
 */
extern "C" boolean
lp_check_alignment(const void *ptr, unsigned alignment)
{
   assert(util_is_power_of_two(alignment));
   return ((uintptr_t)ptr & (alignment - 1)) == 0;
}


class raw_debug_ostream :
   public llvm::raw_ostream
{
private:
   uint64_t pos;

public:
   raw_debug_ostream() : pos(0) { }

   void write_impl(const char *Ptr, size_t Size);

   uint64_t current_pos() const { return pos; }
   size_t preferred_buffer_size() const { return 512; }
};


void
raw_debug_ostream::write_impl(const char *Ptr, size_t Size)
{
   if (Size > 0) {
      char *lastPtr = (char *)&Ptr[Size];
      char last = *lastPtr;
      *lastPtr = 0;
      _debug_printf("%*s", Size, Ptr);
      *lastPtr = last;
      pos += Size;
   }
}


extern "C" const char *
lp_get_module_id(LLVMModuleRef module)
{
   return llvm::unwrap(module)->getModuleIdentifier().c_str();
}


/**
 * Same as LLVMDumpValue, but through our debugging channels.
 */
extern "C" void
lp_debug_dump_value(LLVMValueRef value)
{
#if (defined(PIPE_OS_WINDOWS) && !defined(PIPE_CC_MSVC)) || defined(PIPE_OS_EMBDDED)
   raw_debug_ostream os;
   llvm::unwrap(value)->print(os);
   os.flush();
#else
   LLVMDumpValue(value);
#endif
}


/*
 * MemoryObject wrapper around a buffer of memory, to be used by MC
 * disassembler.
 */
class BufferMemoryObject:
   public llvm::MemoryObject
{
private:
   const uint8_t *Bytes;
   uint64_t Length;
public:
   BufferMemoryObject(const uint8_t *bytes, uint64_t length) :
      Bytes(bytes), Length(length)
   {
   }

   uint64_t getBase() const
   {
      return 0;
   }

   uint64_t getExtent() const
   {
      return Length;
   }

   int readByte(uint64_t addr, uint8_t *byte) const
   {
      if (addr > getExtent())
         return -1;
      *byte = Bytes[addr];
      return 0;
   }
};


/*
 * Disassemble a function, using the LLVM MC disassembler.
 *
 * See also:
 * - http://blog.llvm.org/2010/01/x86-disassembler.html
 * - http://blog.llvm.org/2010/04/intro-to-llvm-mc-project.html
 */
static size_t
disassemble(const void* func, llvm::raw_ostream & Out)
{
   using namespace llvm;

   const uint8_t *bytes = (const uint8_t *)func;

   /*
    * Limit disassembly to this extent
    */
   const uint64_t extent = 96 * 1024;

   uint64_t max_pc = 0;

   /*
    * Initialize all used objects.
    */

   std::string Triple = sys::getDefaultTargetTriple();

   std::string Error;
   const Target *T = TargetRegistry::lookupTarget(Triple, Error);

#if HAVE_LLVM >= 0x0304
   OwningPtr<const MCAsmInfo> AsmInfo(T->createMCAsmInfo(*T->createMCRegInfo(Triple), Triple));
#else
   OwningPtr<const MCAsmInfo> AsmInfo(T->createMCAsmInfo(Triple));
#endif

   if (!AsmInfo) {
      Out << "error: no assembly info for target " << Triple << "\n";
      Out.flush();
      return 0;
   }

   unsigned int AsmPrinterVariant = AsmInfo->getAssemblerDialect();

   OwningPtr<const MCRegisterInfo> MRI(T->createMCRegInfo(Triple));
   if (!MRI) {
      Out << "error: no register info for target " << Triple.c_str() << "\n";
      Out.flush();
      return 0;
   }

   OwningPtr<const MCInstrInfo> MII(T->createMCInstrInfo());
   if (!MII) {
      Out << "error: no instruction info for target " << Triple.c_str() << "\n";
      Out.flush();
      return 0;
   }

#if HAVE_LLVM >= 0x0305
   OwningPtr<const MCSubtargetInfo> STI(T->createMCSubtargetInfo(Triple, sys::getHostCPUName(), ""));
   OwningPtr<MCContext> MCCtx(new MCContext(AsmInfo.get(), MRI.get(), 0));
   OwningPtr<const MCDisassembler> DisAsm(T->createMCDisassembler(*STI, *MCCtx));
#else
   OwningPtr<const MCSubtargetInfo> STI(T->createMCSubtargetInfo(Triple, sys::getHostCPUName(), ""));
   OwningPtr<const MCDisassembler> DisAsm(T->createMCDisassembler(*STI));
#endif
   if (!DisAsm) {
      Out << "error: no disassembler for target " << Triple << "\n";
      Out.flush();
      return 0;
   }


   OwningPtr<MCInstPrinter> Printer(
         T->createMCInstPrinter(AsmPrinterVariant, *AsmInfo, *MII, *MRI, *STI));
   if (!Printer) {
      Out << "error: no instruction printer for target " << Triple.c_str() << "\n";
      Out.flush();
      return 0;
   }

   TargetOptions options;
#if defined(DEBUG)
   options.JITEmitDebugInfo = true;
#endif
#if defined(PIPE_ARCH_X86)
   options.StackAlignmentOverride = 4;
#endif
#if defined(DEBUG) || defined(PROFILE)
   options.NoFramePointerElim = true;
#endif
   OwningPtr<TargetMachine> TM(T->createTargetMachine(Triple, sys::getHostCPUName(), "", options));

#if HAVE_LLVM >= 0x0306
   const TargetInstrInfo *TII = TM->getSubtargetImpl()->getInstrInfo();
#else
   const TargetInstrInfo *TII = TM->getInstrInfo();
#endif

   /*
    * Wrap the data in a MemoryObject
    */
   BufferMemoryObject memoryObject((const uint8_t *)bytes, extent);

   uint64_t pc;
   pc = 0;
   while (true) {
      MCInst Inst;
      uint64_t Size;

      /*
       * Print address.  We use addresses relative to the start of the function,
       * so that between runs.
       */

      Out << llvm::format("%6lu:\t", (unsigned long)pc);

      if (!DisAsm->getInstruction(Inst, Size, memoryObject,
                                 pc,
				  nulls(), nulls())) {
         Out << "invalid";
         pc += 1;
      }

      /*
       * Output the bytes in hexidecimal format.
       */

      if (0) {
         unsigned i;
         for (i = 0; i < Size; ++i) {
            Out << llvm::format("%02x ", ((const uint8_t*)bytes)[pc + i]);
         }
         for (; i < 16; ++i) {
            Out << "   ";
         }
      }

      /*
       * Print the instruction.
       */
      Printer->printInst(&Inst, Out, "");

      /*
       * Advance.
       */

      pc += Size;

      const MCInstrDesc &TID = TII->get(Inst.getOpcode());

      /*
       * Keep track of forward jumps to a nearby address.
       */

      if (TID.isBranch()) {
         for (unsigned i = 0; i < Inst.getNumOperands(); ++i) {
            const MCOperand &operand = Inst.getOperand(i);
            if (operand.isImm()) {
               uint64_t jump;

               /*
                * FIXME: Handle both relative and absolute addresses correctly.
                * EDInstInfo actually has this info, but operandTypes and
                * operandFlags enums are not exposed in the public interface.
                */

               if (1) {
                  /*
                   * PC relative addr.
                   */

                  jump = pc + operand.getImm();
               } else {
                  /*
                   * Absolute addr.
                   */

                  jump = (uint64_t)operand.getImm();
               }

               /*
                * Output the address relative to the function start, given
                * that MC will print the addresses relative the current pc.
                */
               Out << "\t\t; " << jump;

               /*
                * Ignore far jumps given it could be actually a tail return to
                * a random address.
                */

               if (jump > max_pc &&
                   jump < extent) {
                  max_pc = jump;
               }
            }
         }
      }

      Out << "\n";

      /*
       * Stop disassembling on return statements, if there is no record of a
       * jump to a successive address.
       */

      if (TID.isReturn()) {
         if (pc > max_pc) {
            break;
         }
      }
   }

   /*
    * Print GDB command, useful to verify output.
    */

   if (0) {
      _debug_printf("disassemble %p %p\n", bytes, bytes + pc);
   }

   Out << "\n";
   Out.flush();

   return pc;
}


extern "C" void
lp_disassemble(LLVMValueRef func, const void *code) {
   raw_debug_ostream Out;
   disassemble(code, Out);
}


/*
 * Linux perf profiler integration.
 *
 * See also:
 * - http://penberg.blogspot.co.uk/2009/06/jato-has-profiler.html
 * - https://github.com/penberg/jato/commit/73ad86847329d99d51b386f5aba692580d1f8fdc
 * - http://git.kernel.org/?p=linux/kernel/git/torvalds/linux.git;a=commitdiff;h=80d496be89ed7dede5abee5c057634e80a31c82d
 */
extern "C" void
lp_profile(LLVMValueRef func, const void *code)
{
#if defined(__linux__) && (defined(DEBUG) || defined(PROFILE))
   static boolean first_time = TRUE;
   static FILE *perf_map_file = NULL;
   static int perf_asm_fd = -1;
   if (first_time) {
      /*
       * We rely on the disassembler for determining a function's size, but
       * the disassembly is a leaky and slow operation, so avoid running
       * this except when running inside linux perf, which can be inferred
       * by the PERF_BUILDID_DIR environment variable.
       */
      if (getenv("PERF_BUILDID_DIR")) {
         pid_t pid = getpid();
         char filename[256];
         util_snprintf(filename, sizeof filename, "/tmp/perf-%llu.map", (unsigned long long)pid);
         perf_map_file = fopen(filename, "wt");
         util_snprintf(filename, sizeof filename, "/tmp/perf-%llu.map.asm", (unsigned long long)pid);
         mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
         perf_asm_fd = open(filename, O_WRONLY | O_CREAT, mode);
      }
      first_time = FALSE;
   }
   if (perf_map_file) {
      const char *symbol = LLVMGetValueName(func);
      unsigned long addr = (uintptr_t)code;
      llvm::raw_fd_ostream Out(perf_asm_fd, false);
      Out << symbol << ":\n";
      unsigned long size = disassemble(code, Out);
      fprintf(perf_map_file, "%lx %lx %s\n", addr, size, symbol);
      fflush(perf_map_file);
   }
#else
   (void)func;
   (void)code;
#endif
}



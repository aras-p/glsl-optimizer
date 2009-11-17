/**************************************************************************
 * 
 * Copyright 2008 Dennis Smit
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * AUTHORS, COPYRIGHT HOLDERS, AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

/**
 * @file
 * CPU feature detection.
 *
 * @author Dennis Smit
 * @author Based on the work of Eric Anholt <anholt@FreeBSD.org>
 */

#include "pipe/p_config.h"

#include "u_debug.h"
#include "u_cpu_detect.h"

#if defined(PIPE_ARCH_PPC)
#if defined(PIPE_OS_DARWIN)
#include <sys/sysctl.h>
#else
#include <signal.h>
#include <setjmp.h>
#endif
#endif

#if defined(PIPE_OS_NETBSD) || defined(PIPE_OS_OPENBSD)
#include <sys/param.h>
#include <sys/sysctl.h>
#include <machine/cpu.h>
#endif

#if defined(PIPE_OS_FREEBSD)
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#if defined(PIPE_OS_LINUX)
#include <signal.h>
#endif

#ifdef PIPE_OS_UNIX
#include <unistd.h>
#endif

#if defined(PIPE_OS_WINDOWS)
#include <windows.h>
#if defined(MSVC)
#include <intrin.h>
#endif
#endif


struct util_cpu_caps util_cpu_caps;

static int has_cpuid(void);

#if defined(PIPE_ARCH_X86)

/* The sigill handlers */
#if defined(PIPE_OS_LINUX) /*&& defined(_POSIX_SOURCE) && defined(X86_FXSR_MAGIC)*/
static void
sigill_handler_sse(int signal, struct sigcontext sc)
{
   /* Both the "xorps %%xmm0,%%xmm0" and "divps %xmm0,%%xmm1"
    * instructions are 3 bytes long.  We must increment the instruction
    * pointer manually to avoid repeated execution of the offending
    * instruction.
    *
    * If the SIGILL is caused by a divide-by-zero when unmasked
    * exceptions aren't supported, the SIMD FPU status and control
    * word will be restored at the end of the test, so we don't need
    * to worry about doing it here.  Besides, we may not be able to...
    */
   sc.eip += 3;

   util_cpu_caps.has_sse=0;
}

static void
sigfpe_handler_sse(int signal, struct sigcontext sc)
{
   if (sc.fpstate->magic != 0xffff) {
      /* Our signal context has the extended FPU state, so reset the
       * divide-by-zero exception mask and clear the divide-by-zero
       * exception bit.
       */
      sc.fpstate->mxcsr |= 0x00000200;
      sc.fpstate->mxcsr &= 0xfffffffb;
   } else {
      /* If we ever get here, we're completely hosed.
      */
   }
}
#endif /* PIPE_OS_LINUX && _POSIX_SOURCE && X86_FXSR_MAGIC */

#if defined(PIPE_OS_WINDOWS)
static LONG CALLBACK
win32_sig_handler_sse(EXCEPTION_POINTERS* ep)
{
   if(ep->ExceptionRecord->ExceptionCode==EXCEPTION_ILLEGAL_INSTRUCTION){
      ep->ContextRecord->Eip +=3;
      util_cpu_caps.has_sse=0;
      return EXCEPTION_CONTINUE_EXECUTION;
   }
   return EXCEPTION_CONTINUE_SEARCH;
}
#endif /* PIPE_OS_WINDOWS */

#endif /* PIPE_ARCH_X86 */


#if defined(PIPE_ARCH_PPC) && !defined(PIPE_OS_DARWIN)
static jmp_buf  __lv_powerpc_jmpbuf;
static volatile sig_atomic_t __lv_powerpc_canjump = 0;

static void
sigill_handler(int sig)
{
   if (!__lv_powerpc_canjump) {
      signal (sig, SIG_DFL);
      raise (sig);
   }

   __lv_powerpc_canjump = 0;
   longjmp(__lv_powerpc_jmpbuf, 1);
}
#endif

#if defined(PIPE_ARCH_PPC)
static void
check_os_altivec_support(void)
{
#if defined(PIPE_OS_DARWIN)
   int sels[2] = {CTL_HW, HW_VECTORUNIT};
   int has_vu = 0;
   int len = sizeof (has_vu);
   int err;

   err = sysctl(sels, 2, &has_vu, &len, NULL, 0);

   if (err == 0) {
      if (has_vu != 0) {
         util_cpu_caps.has_altivec = 1;
      }
   }
#else /* !PIPE_OS_DARWIN */
   /* no Darwin, do it the brute-force way */
   /* this is borrowed from the libmpeg2 library */
   signal(SIGILL, sigill_handler);
   if (setjmp(__lv_powerpc_jmpbuf)) {
      signal(SIGILL, SIG_DFL);
   } else {
      __lv_powerpc_canjump = 1;

      __asm __volatile
         ("mtspr 256, %0\n\t"
          "vand %%v0, %%v0, %%v0"
          :
          : "r" (-1));

      signal(SIGILL, SIG_DFL);
      util_cpu_caps.has_altivec = 1;
   }
#endif /* PIPE_OS_DARWIN */
}
#endif /* PIPE_ARCH_PPC */

/* If we're running on a processor that can do SSE, let's see if we
 * are allowed to or not.  This will catch 2.4.0 or later kernels that
 * haven't been configured for a Pentium III but are running on one,
 * and RedHat patched 2.2 kernels that have broken exception handling
 * support for user space apps that do SSE.
 */
#if defined(PIPE_ARCH_X86) || defined (PIPE_ARCH_X86_64)
static void
check_os_katmai_support(void)
{
#if defined(PIPE_ARCH_X86)
#if defined(PIPE_OS_FREEBSD)
   int has_sse=0, ret;
   int len = sizeof (has_sse);

   ret = sysctlbyname("hw.instruction_sse", &has_sse, &len, NULL, 0);
   if (ret || !has_sse)
      util_cpu_caps.has_sse=0;

#elif defined(PIPE_OS_NETBSD) || defined(PIPE_OS_OPENBSD)
   int has_sse, has_sse2, ret, mib[2];
   int varlen;

   mib[0] = CTL_MACHDEP;
   mib[1] = CPU_SSE;
   varlen = sizeof (has_sse);

   ret = sysctl(mib, 2, &has_sse, &varlen, NULL, 0);
   if (ret < 0 || !has_sse) {
      util_cpu_caps.has_sse = 0;
   } else {
      util_cpu_caps.has_sse = 1;
   }

   mib[1] = CPU_SSE2;
   varlen = sizeof (has_sse2);
   ret = sysctl(mib, 2, &has_sse2, &varlen, NULL, 0);
   if (ret < 0 || !has_sse2) {
      util_cpu_caps.has_sse2 = 0;
   } else {
      util_cpu_caps.has_sse2 = 1;
   }
   util_cpu_caps.has_sse = 0; /* FIXME ?!?!? */

#elif defined(PIPE_OS_WINDOWS)
   LPTOP_LEVEL_EXCEPTION_FILTER exc_fil;
   if (util_cpu_caps.has_sse) {
      exc_fil = SetUnhandledExceptionFilter(win32_sig_handler_sse);
#if defined(PIPE_CC_GCC)
      __asm __volatile ("xorps %xmm0, %xmm0");
#elif defined(PIPE_CC_MSVC)
      __asm {
          xorps xmm0, xmm0        /* executing SSE instruction */
      }
#else
#error Unsupported compiler
#endif
      SetUnhandledExceptionFilter(exc_fil);
   }
#elif defined(PIPE_OS_LINUX)
   struct sigaction saved_sigill;
   struct sigaction saved_sigfpe;

   /* Save the original signal handlers.
   */
   sigaction(SIGILL, NULL, &saved_sigill);
   sigaction(SIGFPE, NULL, &saved_sigfpe);

   signal(SIGILL, (void (*)(int))sigill_handler_sse);
   signal(SIGFPE, (void (*)(int))sigfpe_handler_sse);

   /* Emulate test for OSFXSR in CR4.  The OS will set this bit if it
    * supports the extended FPU save and restore required for SSE.  If
    * we execute an SSE instruction on a PIII and get a SIGILL, the OS
    * doesn't support Streaming SIMD Exceptions, even if the processor
    * does.
    */
   if (util_cpu_caps.has_sse) {
      __asm __volatile ("xorps %xmm1, %xmm0");
   }

   /* Emulate test for OSXMMEXCPT in CR4.  The OS will set this bit if
    * it supports unmasked SIMD FPU exceptions.  If we unmask the
    * exceptions, do a SIMD divide-by-zero and get a SIGILL, the OS
    * doesn't support unmasked SIMD FPU exceptions.  If we get a SIGFPE
    * as expected, we're okay but we need to clean up after it.
    *
    * Are we being too stringent in our requirement that the OS support
    * unmasked exceptions?  Certain RedHat 2.2 kernels enable SSE by
    * setting CR4.OSFXSR but don't support unmasked exceptions.  Win98
    * doesn't even support them.  We at least know the user-space SSE
    * support is good in kernels that do support unmasked exceptions,
    * and therefore to be safe I'm going to leave this test in here.
    */
   if (util_cpu_caps.has_sse) {
      /* test_os_katmai_exception_support(); */
   }

   /* Restore the original signal handlers.
   */
   sigaction(SIGILL, &saved_sigill, NULL);
   sigaction(SIGFPE, &saved_sigfpe, NULL);

#else
   /* We can't use POSIX signal handling to test the availability of
    * SSE, so we disable it by default.
    */
   util_cpu_caps.has_sse = 0;
#endif /* __linux__ */
#endif

#if defined(PIPE_ARCH_X86_64)
   util_cpu_caps.has_sse = 1;
#endif
}


static int has_cpuid(void)
{
#if defined(PIPE_ARCH_X86)
#if defined(PIPE_OS_GCC)
   int a, c;

   __asm __volatile
      ("pushf\n"
       "popl %0\n"
       "movl %0, %1\n"
       "xorl $0x200000, %0\n"
       "push %0\n"
       "popf\n"
       "pushf\n"
       "popl %0\n"
       : "=a" (a), "=c" (c)
       :
       : "cc");

   return a != c;
#else
   /* FIXME */
   return 1;
#endif
#elif defined(PIPE_ARCH_X86_64)
   return 1;
#else
   return 0;
#endif
}


/**
 * @sa cpuid.h included in gcc-4.3 onwards.
 * @sa http://msdn.microsoft.com/en-us/library/hskdteyh.aspx
 */
static INLINE void
cpuid(uint32_t ax, uint32_t *p)
{
#if defined(PIPE_CC_GCC) && defined(PIPE_ARCH_X86)
   __asm __volatile (
     "xchgl %%ebx, %1\n\t"
     "cpuid\n\t"
     "xchgl %%ebx, %1"
     : "=a" (p[0]),
       "=S" (p[1]),
       "=c" (p[2]),
       "=d" (p[3])
     : "0" (ax)
   );
#elif defined(PIPE_CC_GCC) && defined(PIPE_ARCH_X86_64)
   __asm __volatile (
     "cpuid\n\t"
     : "=a" (p[0]),
       "=b" (p[1]),
       "=c" (p[2]),
       "=d" (p[3])
     : "0" (ax)
   );
#elif defined(PIPE_CC_MSVC)
   __cpuid(p, ax);
#else
   p[0] = 0;
   p[1] = 0;
   p[2] = 0;
   p[3] = 0;
#endif
}
#endif /* X86 or X86_64 */

void
util_cpu_detect(void)
{
   static boolean util_cpu_detect_initialized = FALSE;

   if(util_cpu_detect_initialized)
      return;

   memset(&util_cpu_caps, 0, sizeof util_cpu_caps);

   /* Check for arch type */
#if defined(PIPE_ARCH_MIPS)
   util_cpu_caps.arch = UTIL_CPU_ARCH_MIPS;
#elif defined(PIPE_ARCH_ALPHA)
   util_cpu_caps.arch = UTIL_CPU_ARCH_ALPHA;
#elif defined(PIPE_ARCH_SPARC)
   util_cpu_caps.arch = UTIL_CPU_ARCH_SPARC;
#elif defined(PIPE_ARCH_X86) || defined(PIPE_ARCH_X86_64)
   util_cpu_caps.arch = UTIL_CPU_ARCH_X86;
   util_cpu_caps.little_endian = 1;
#elif defined(PIPE_ARCH_PPC)
   util_cpu_caps.arch = UTIL_CPU_ARCH_POWERPC;
   util_cpu_caps.little_endian = 0;
#else
   util_cpu_caps.arch = UTIL_CPU_ARCH_UNKNOWN;
#endif

   /* Count the number of CPUs in system */
#if defined(PIPE_OS_WINDOWS)
   {
      SYSTEM_INFO system_info;
      GetSystemInfo(&system_info);
      util_cpu_caps.nr_cpus = system_info.dwNumberOfProcessors;
   }
#elif defined(PIPE_OS_UNIX) && defined(_SC_NPROCESSORS_ONLN)
   util_cpu_caps.nr_cpus = sysconf(_SC_NPROCESSORS_ONLN);
   if (util_cpu_caps.nr_cpus == -1)
      util_cpu_caps.nr_cpus = 1;
#elif defined(PIPE_OS_BSD)
   {
      int mib[2], ncpu;
      int len;

      mib[0] = CTL_HW;
      mib[1] = HW_NCPU;

      len = sizeof (ncpu);
      sysctl(mib, 2, &ncpu, &len, NULL, 0);
      util_cpu_caps.nr_cpus = ncpu;
   }
#else
   util_cpu_caps.nr_cpus = 1;
#endif

#if defined(PIPE_ARCH_X86) || defined(PIPE_ARCH_X86_64)
   if (has_cpuid()) {
      uint32_t regs[4];
      uint32_t regs2[4];

      util_cpu_caps.cacheline = 32;

      /* Get max cpuid level */
      cpuid(0x00000000, regs);

      if (regs[0] >= 0x00000001) {
         unsigned int cacheline;

         cpuid (0x00000001, regs2);

         util_cpu_caps.x86_cpu_type = (regs2[0] >> 8) & 0xf;
         if (util_cpu_caps.x86_cpu_type == 0xf)
             util_cpu_caps.x86_cpu_type = 8 + ((regs2[0] >> 20) & 255); /* use extended family (P4, IA64) */

         /* general feature flags */
         util_cpu_caps.has_tsc    = (regs2[3] & (1 << 8  )) >>  8; /* 0x0000010 */
         util_cpu_caps.has_mmx    = (regs2[3] & (1 << 23 )) >> 23; /* 0x0800000 */
         util_cpu_caps.has_sse    = (regs2[3] & (1 << 25 )) >> 25; /* 0x2000000 */
         util_cpu_caps.has_sse2   = (regs2[3] & (1 << 26 )) >> 26; /* 0x4000000 */
         util_cpu_caps.has_sse3   = (regs2[2] & (1));          /* 0x0000001 */
         util_cpu_caps.has_ssse3  = (regs2[2] & (1 << 9 )) >> 9;   /* 0x0000020 */
         util_cpu_caps.has_sse4_1 = (regs2[2] & (1 << 19)) >> 19;
         util_cpu_caps.has_mmx2   = util_cpu_caps.has_sse; /* SSE cpus supports mmxext too */

         cacheline = ((regs2[1] >> 8) & 0xFF) * 8;
         if (cacheline > 0)
            util_cpu_caps.cacheline = cacheline;
      }

      cpuid(0x80000000, regs);

      if (regs[0] >= 0x80000001) {

         cpuid(0x80000001, regs2);

         util_cpu_caps.has_mmx  |= (regs2[3] & (1 << 23 )) >> 23; /* 0x0800000 */
         util_cpu_caps.has_mmx2 |= (regs2[3] & (1 << 22 )) >> 22; /* 0x400000 */
         util_cpu_caps.has_3dnow    = (regs2[3] & (1 << 31 )) >> 31; /* 0x80000000 */
         util_cpu_caps.has_3dnow_ext = (regs2[3] & (1 << 30 )) >> 30;
      }

      if (regs[0] >= 0x80000006) {
         cpuid(0x80000006, regs2);
         util_cpu_caps.cacheline = regs2[2] & 0xFF;
      }

      if (util_cpu_caps.has_sse)
         check_os_katmai_support();

      if (!util_cpu_caps.has_sse) {
         util_cpu_caps.has_sse2 = 0;
         util_cpu_caps.has_sse3 = 0;
         util_cpu_caps.has_ssse3 = 0;
         util_cpu_caps.has_sse4_1 = 0;
      }
   }
#endif /* PIPE_ARCH_X86 || PIPE_ARCH_X86_64 */

#if defined(PIPE_ARCH_PPC)
   check_os_altivec_support();
#endif /* PIPE_ARCH_PPC */

#ifdef DEBUG
   debug_printf("util_cpu_caps.arch = %i\n", util_cpu_caps.arch);
   debug_printf("util_cpu_caps.nr_cpus = %u\n", util_cpu_caps.nr_cpus);

   debug_printf("util_cpu_caps.x86_cpu_type = %u\n", util_cpu_caps.x86_cpu_type);
   debug_printf("util_cpu_caps.cacheline = %u\n", util_cpu_caps.cacheline);

   debug_printf("util_cpu_caps.has_tsc = %u\n", util_cpu_caps.has_tsc);
   debug_printf("util_cpu_caps.has_mmx = %u\n", util_cpu_caps.has_mmx);
   debug_printf("util_cpu_caps.has_mmx2 = %u\n", util_cpu_caps.has_mmx2);
   debug_printf("util_cpu_caps.has_sse = %u\n", util_cpu_caps.has_sse);
   debug_printf("util_cpu_caps.has_sse2 = %u\n", util_cpu_caps.has_sse2);
   debug_printf("util_cpu_caps.has_sse3 = %u\n", util_cpu_caps.has_sse3);
   debug_printf("util_cpu_caps.has_ssse3 = %u\n", util_cpu_caps.has_ssse3);
   debug_printf("util_cpu_caps.has_sse4_1 = %u\n", util_cpu_caps.has_sse4_1);
   debug_printf("util_cpu_caps.has_3dnow = %u\n", util_cpu_caps.has_3dnow);
   debug_printf("util_cpu_caps.has_3dnow_ext = %u\n", util_cpu_caps.has_3dnow_ext);
   debug_printf("util_cpu_caps.has_altivec = %u\n", util_cpu_caps.has_altivec);
#endif

   util_cpu_detect_initialized = TRUE;
}

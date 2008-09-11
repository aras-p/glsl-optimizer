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

/*
 * Based on the work of Eric Anholt <anholt@FreeBSD.org>
 */

/* FIXME: clean this entire file up */

#include "u_cpu_detect.h"

#ifdef __linux__
#define OS_LINUX
#endif
#ifdef WIN32
#define OS_WIN32
#endif

#if defined(ARCH_POWERPC)
#if defined(OS_DARWIN)
#include <sys/sysctl.h>
#else
#include <signal.h>
#include <setjmp.h>
#endif
#endif

#if defined(OS_NETBSD) || defined(OS_OPENBSD)
#include <sys/param.h>
#include <sys/sysctl.h>
#include <machine/cpu.h>
#endif

#if defined(OS_FREEBSD)
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#if defined(OS_LINUX)
#include <signal.h>
#endif

#if defined(OS_WIN32)
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


static struct cpu_detect_caps __cpu_detect_caps;
static int __cpu_detect_initialized = 0;

static int has_cpuid(void);
static int cpuid(unsigned int ax, unsigned int *p);

/* The sigill handlers */
#if defined(ARCH_X86) /* x86 (linux katmai handler check thing) */
#if defined(OS_LINUX) && defined(_POSIX_SOURCE) && defined(X86_FXSR_MAGIC)
static void sigill_handler_sse(int signal, struct sigcontext sc)
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

	__cpu_detect_caps.hasSSE=0;
}

static void sigfpe_handler_sse(int signal, struct sigcontext sc)
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
#endif
#endif /* OS_LINUX && _POSIX_SOURCE && X86_FXSR_MAGIC */

#if defined(OS_WIN32)
LONG CALLBACK win32_sig_handler_sse(EXCEPTION_POINTERS* ep)
{
	if(ep->ExceptionRecord->ExceptionCode==EXCEPTION_ILLEGAL_INSTRUCTION){
		ep->ContextRecord->Eip +=3;
		__cpu_detect_caps.hasSSE=0;
		return EXCEPTION_CONTINUE_EXECUTION;
	}
	return EXCEPTION_CONTINUE_SEARCH;
}
#endif /* OS_WIN32 */


#if defined(ARCH_POWERPC) && !defined(OS_DARWIN)
static sigjmp_buf __lv_powerpc_jmpbuf;
static volatile sig_atomic_t __lv_powerpc_canjump = 0;

static void sigill_handler (int sig);

static void sigill_handler (int sig)
{
	if (!__lv_powerpc_canjump) {
		signal (sig, SIG_DFL);
		raise (sig);
	}

	__lv_powerpc_canjump = 0;
	siglongjmp(__lv_powerpc_jmpbuf, 1);
}

static void check_os_altivec_support(void)
{
#if defined(OS_DARWIN)
	int sels[2] = {CTL_HW, HW_VECTORUNIT};
	int has_vu = 0;
	int len = sizeof (has_vu);
	int err;

	err = sysctl(sels, 2, &has_vu, &len, NULL, 0);

	if (err == 0) {
		if (has_vu != 0) {
			__cpu_detect_caps.hasAltiVec = 1;
		}
	}
#else /* !OS_DARWIN */
	/* no Darwin, do it the brute-force way */
	/* this is borrowed from the libmpeg2 library */
	signal(SIGILL, sigill_handler);
	if (sigsetjmp(__lv_powerpc_jmpbuf, 1)) {
		signal(SIGILL, SIG_DFL);
	} else {
		__lv_powerpc_canjump = 1;

		__asm __volatile
			("mtspr 256, %0\n\t"
			 "vand %%v0, %%v0, %%v0"
			 :
			 : "r" (-1));

		signal(SIGILL, SIG_DFL);
		__cpu_detect_caps.hasAltiVec = 1;
	}
#endif
}
#endif

/* If we're running on a processor that can do SSE, let's see if we
 * are allowed to or not.  This will catch 2.4.0 or later kernels that
 * haven't been configured for a Pentium III but are running on one,
 * and RedHat patched 2.2 kernels that have broken exception handling
 * support for user space apps that do SSE.
 */
static void check_os_katmai_support(void)
{
#if defined(ARCH_X86)
#if defined(OS_FREEBSD)
	int has_sse=0, ret;
	int len = sizeof (has_sse);

	ret = sysctlbyname("hw.instruction_sse", &has_sse, &len, NULL, 0);
	if (ret || !has_sse)
		__cpu_detect_caps.hasSSE=0;

#elif defined(OS_NETBSD) || defined(OS_OPENBSD)
	int has_sse, has_sse2, ret, mib[2];
	int varlen;

	mib[0] = CTL_MACHDEP;
	mib[1] = CPU_SSE;
	varlen = sizeof (has_sse);

	ret = sysctl(mib, 2, &has_sse, &varlen, NULL, 0);
	if (ret < 0 || !has_sse) {
		__cpu_detect_caps.hasSSE = 0;
	} else {
		__cpu_detect_caps.hasSSE = 1;
	}

	mib[1] = CPU_SSE2;
	varlen = sizeof (has_sse2);
	ret = sysctl(mib, 2, &has_sse2, &varlen, NULL, 0);
	if (ret < 0 || !has_sse2) {
		__cpu_detect_caps.hasSSE2 = 0;
	} else {
		__cpu_detect_caps.hasSSE2 = 1;
	}
	__cpu_detect_caps.hasSSE = 0; /* FIXME ?!?!? */

#elif defined(OS_WIN32)
	LPTOP_LEVEL_EXCEPTION_FILTER exc_fil;
	if (__cpu_detect_caps.hasSSE) {
		exc_fil = SetUnhandledExceptionFilter(win32_sig_handler_sse);
		__asm __volatile ("xorps %xmm0, %xmm0");
		SetUnhandledExceptionFilter(exc_fil);
	}
#elif defined(OS_LINUX)
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
	if (__cpu_detect_caps.hasSSE) {
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
	if (__cpu_detect_caps.hasSSE) {
		//      test_os_katmai_exception_support();
	}

	/* Restore the original signal handlers.
	*/
	sigaction(SIGILL, &saved_sigill, NULL);
	sigaction(SIGFPE, &saved_sigfpe, NULL);

#else
	/* We can't use POSIX signal handling to test the availability of
	 * SSE, so we disable it by default.
	 */
	__cpu_detect_caps.hasSSE = 0;
#endif /* __linux__ */
#endif
}


static int has_cpuid(void)
{
#if defined(ARCH_X86)
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
	return 0;
#endif
}

static int cpuid(unsigned int ax, unsigned int *p)
{
#if defined(ARCH_X86)
	unsigned int flags;

	__asm __volatile
		("movl %%ebx, %%esi\n\t"
		 "cpuid\n\t"
		 "xchgl %%ebx, %%esi"
		 : "=a" (p[0]), "=S" (p[1]),
		 "=c" (p[2]), "=d" (p[3])
		 : "0" (ax));

	return 0;
#else
	return -1;
#endif
}

void cpu_detect_initialize()
{
	unsigned int regs[4];
	unsigned int regs2[4];

	int mib[2], ncpu;
	int len;

	memset(&__cpu_detect_caps, 0, sizeof (struct cpu_detect_caps));

	/* Check for arch type */
#if defined(ARCH_MIPS)
	__cpu_detect_caps.type = CPU_DETECT_TYPE_MIPS;
#elif defined(ARCH_ALPHA)
	__cpu_detect_caps.type = CPU_DETECT_TYPE_ALPHA;
#elif defined(ARCH_SPARC)
	__cpu_detect_caps.type = CPU_DETECT_TYPE_SPARC;
#elif defined(ARCH_X86)
	__cpu_detect_caps.type = CPU_DETECT_TYPE_X86;
#elif defined(ARCH_POWERPC)
	__cpu_detect_caps.type = CPU_DETECT_TYPE_POWERPC;
#else
	__cpu_detect_caps.type = CPU_DETECT_TYPE_OTHER;
#endif

	/* Count the number of CPUs in system */
#if !defined(OS_WIN32) && !defined(OS_UNKNOWN) && defined(_SC_NPROCESSORS_ONLN)
	__cpu_detect_caps.nrcpu = sysconf(_SC_NPROCESSORS_ONLN);
	if (__cpu_detect_caps.nrcpu == -1)
		__cpu_detect_caps.nrcpu = 1;

#elif defined(OS_NETBSD) || defined(OS_FREEBSD) || defined(OS_OPENBSD)

	mib[0] = CTL_HW;
	mib[1] = HW_NCPU;

	len = sizeof (ncpu);
	sysctl(mib, 2, &ncpu, &len, NULL, 0);
	__cpu_detect_caps.nrcpu = ncpu;

#else
	__cpu_detect_caps.nrcpu = 1;
#endif

#if defined(ARCH_X86)
	/* No cpuid, old 486 or lower */
	if (has_cpuid() == 0)
		return;

	__cpu_detect_caps.cacheline = 32;

	/* Get max cpuid level */
	cpuid(0x00000000, regs);

	if (regs[0] >= 0x00000001) {
		unsigned int cacheline;

		cpuid (0x00000001, regs2);

		__cpu_detect_caps.x86cpuType = (regs2[0] >> 8) & 0xf;
		if (__cpu_detect_caps.x86cpuType == 0xf)
		    __cpu_detect_caps.x86cpuType = 8 + ((regs2[0] >> 20) & 255); /* use extended family (P4, IA64) */

		/* general feature flags */
		__cpu_detect_caps.hasTSC  = (regs2[3] & (1 << 8  )) >>  8; /* 0x0000010 */
		__cpu_detect_caps.hasMMX  = (regs2[3] & (1 << 23 )) >> 23; /* 0x0800000 */
		__cpu_detect_caps.hasSSE  = (regs2[3] & (1 << 25 )) >> 25; /* 0x2000000 */
		__cpu_detect_caps.hasSSE2 = (regs2[3] & (1 << 26 )) >> 26; /* 0x4000000 */
		__cpu_detect_caps.hasSSE3 = (regs2[2] & (1));	       /* 0x0000001 */
		__cpu_detect_caps.hasSSSE3 = (regs2[2] & (1 << 9 )) >> 9;   /* 0x0000020 */
		__cpu_detect_caps.hasMMX2 = __cpu_detect_caps.hasSSE; /* SSE cpus supports mmxext too */

		cacheline = ((regs2[1] >> 8) & 0xFF) * 8;
		if (cacheline > 0)
			__cpu_detect_caps.cacheline = cacheline;
	}

	cpuid(0x80000000, regs);

	if (regs[0] >= 0x80000001) {

		cpuid(0x80000001, regs2);

		__cpu_detect_caps.hasMMX  |= (regs2[3] & (1 << 23 )) >> 23; /* 0x0800000 */
		__cpu_detect_caps.hasMMX2 |= (regs2[3] & (1 << 22 )) >> 22; /* 0x400000 */
		__cpu_detect_caps.has3DNow    = (regs2[3] & (1 << 31 )) >> 31; /* 0x80000000 */
		__cpu_detect_caps.has3DNowExt = (regs2[3] & (1 << 30 )) >> 30;
	}

	if (regs[0] >= 0x80000006) {
		cpuid(0x80000006, regs2);
		__cpu_detect_caps.cacheline = regs2[2] & 0xFF;
	}


#if defined(OS_LINUX) || defined(OS_FREEBSD) || defined(OS_NETBSD) || defined(OS_CYGWIN) || defined(OS_OPENBSD)
	if (__cpu_detect_caps.hasSSE)
		check_os_katmai_support();

	if (!__cpu_detect_caps.hasSSE) {
		__cpu_detect_caps.hasSSE2 = 0;
		__cpu_detect_caps.hasSSE3 = 0;
		__cpu_detect_caps.hasSSSE3 = 0;
	}
#else
	__cpu_detect_caps.hasSSE = 0;
	__cpu_detect_caps.hasSSE2 = 0;
	__cpu_detect_caps.hasSSE3 = 0;
	__cpu_detect_caps.hasSSSE3 = 0;
#endif
#endif /* ARCH_X86 */

#if defined(ARCH_POWERPC)
	check_os_altivec_support();
#endif /* ARCH_POWERPC */

	__cpu_detect_initialized = 1;
}

struct cpu_detect_caps *cpu_detect_get_caps()
{
	return &__cpu_detect_caps;
}

/* The getters and setters for feature flags */
int cpu_detect_get_tsc()
{
	return __cpu_detect_caps.hasTSC;
}

int cpu_detect_get_mmx()
{
	return __cpu_detect_caps.hasMMX;
}

int cpu_detect_get_mmx2()
{
	return __cpu_detect_caps.hasMMX2;
}

int cpu_detect_get_sse()
{
	return __cpu_detect_caps.hasSSE;
}

int cpu_detect_get_sse2()
{
	return __cpu_detect_caps.hasSSE2;
}

int cpu_detect_get_sse3()
{
	return __cpu_detect_caps.hasSSE3;
}

int cpu_detect_get_ssse3()
{
	return __cpu_detect_caps.hasSSSE3;
}

int cpu_detect_get_3dnow()
{
	return __cpu_detect_caps.has3DNow;
}

int cpu_detect_get_3dnow2()
{
	return __cpu_detect_caps.has3DNowExt;
}

int cpu_detect_get_altivec()
{
	return __cpu_detect_caps.hasAltiVec;
}


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
 ***************************************************************************/

/*
 * Based on the work of Eric Anholt <anholt@FreeBSD.org>
 */

#ifndef _CPU_DETECT_H
#define _CPU_DETECT_H

typedef enum {
	CPU_DETECT_TYPE_MIPS,
	CPU_DETECT_TYPE_ALPHA,
	CPU_DETECT_TYPE_SPARC,
	CPU_DETECT_TYPE_X86,
	CPU_DETECT_TYPE_POWERPC,
	CPU_DETECT_TYPE_OTHER
} cpu_detect_type;

struct cpu_detect_caps {
	cpu_detect_type	type;
	int		nrcpu;

	/* Feature flags */
	int		x86cpuType;
	int		cacheline;

	int		hasTSC;
	int		hasMMX;
	int		hasMMX2;
	int		hasSSE;
	int		hasSSE2;
	int		hasSSE3;
	int		hasSSSE3;
	int		has3DNow;
	int		has3DNowExt;
	int		hasAltiVec;
};

/* prototypes */
void cpu_detect_initialize(void);
struct cpu_detect_caps *cpu_detect_get_caps(void);

int cpu_detect_get_tsc(void);
int cpu_detect_get_mmx(void);
int cpu_detect_get_mmx2(void);
int cpu_detect_get_sse(void);
int cpu_detect_get_sse2(void);
int cpu_detect_get_sse3(void);
int cpu_detect_get_ssse3(void);
int cpu_detect_get_3dnow(void);
int cpu_detect_get_3dnow2(void);
int cpu_detect_get_altivec(void);

#endif /* _CPU_DETECT_H */

/* $Id: common_x86_features.h,v 1.4 2001/03/28 20:44:44 gareth Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * x86 CPUID feature information.  The raw data is returned by
 * _mesa_identify_x86_cpu_features() and interpreted with the cpu_has_*
 * helper macros.
 *
 * Gareth Hughes <gareth@valinux.com>
 */

#ifndef __COMMON_X86_FEATURES_H__
#define __COMMON_X86_FEATURES_H__

/* Capabilities of CPUs
 */
#define X86_FEATURE_FPU		0x00000001
#define X86_FEATURE_VME		0x00000002
#define X86_FEATURE_DE		0x00000004
#define X86_FEATURE_PSE		0x00000008
#define X86_FEATURE_TSC		0x00000010
#define X86_FEATURE_MSR		0x00000020
#define X86_FEATURE_PAE		0x00000040
#define X86_FEATURE_MCE		0x00000080
#define X86_FEATURE_CX8		0x00000100
#define X86_FEATURE_APIC	0x00000200
#define X86_FEATURE_10		0x00000400
#define X86_FEATURE_SEP		0x00000800
#define X86_FEATURE_MTRR	0x00001000
#define X86_FEATURE_PGE		0x00002000
#define X86_FEATURE_MCA		0x00004000
#define X86_FEATURE_CMOV	0x00008000
#define X86_FEATURE_PAT		0x00010000
#define X86_FEATURE_PSE36	0x00020000
#define X86_FEATURE_18		0x00040000
#define X86_FEATURE_19		0x00080000
#define X86_FEATURE_20		0x00100000
#define X86_FEATURE_21		0x00200000
#define X86_FEATURE_MMXEXT	0x00400000
#define X86_FEATURE_MMX		0x00800000
#define X86_FEATURE_FXSR	0x01000000
#define X86_FEATURE_XMM		0x02000000
#define X86_FEATURE_XMM2	0x04000000
#define X86_FEATURE_27		0x08000000
#define X86_FEATURE_28		0x10000000
#define X86_FEATURE_29		0x20000000
#define X86_FEATURE_3DNOWEXT	0x40000000
#define X86_FEATURE_3DNOW	0x80000000

#define cpu_has_mmx		(_mesa_x86_cpu_features & X86_FEATURE_MMX)
#define cpu_has_mmxext		(_mesa_x86_cpu_features & X86_FEATURE_MMXEXT)
#define cpu_has_xmm		(_mesa_x86_cpu_features & X86_FEATURE_XMM)
#define cpu_has_xmm2		(_mesa_x86_cpu_features & X86_FEATURE_XMM2)
#define cpu_has_3dnow		(_mesa_x86_cpu_features & X86_FEATURE_3DNOW)
#define cpu_has_3dnowext	(_mesa_x86_cpu_features & X86_FEATURE_3DNOWEXT)

#endif

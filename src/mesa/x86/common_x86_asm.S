/* $Id: common_x86_asm.S,v 1.2 2000/10/23 00:16:28 gareth Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
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
 * Check extended CPU capabilities.  Now justs returns the raw CPUID
 * feature information, allowing the higher level code to interpret the
 * results.
 *
 * Written by Holger Waechtler <holger@akaflieg.extern.tu-berlin.de>
 *
 * Cleaned up and simplified by Gareth Hughes <gareth@valinux.com>
 */

#include "assyntax.h"
#include "common_x86_features.h"


/* Intel vendor string
 */
#define GENU	0x756e6547	/* "Genu" */
#define INEI	0x49656e69	/* "ineI" */
#define NTEL	0x6c65746e	/* "ntel" */

/* AMD vendor string
 */
#define AUTH	0x68747541	/* "Auth" */
#define ENTI	0x69746e65	/* "enti" */
#define CAMD	0x444d4163	/* "cAMD" */


	SEG_DATA

/* We might want to print out some useful messages.
 */
LLBL( found_intel ):	STRING( "Genuine Intel processor found\n\0" )
LLBL( found_amd ):	STRING( "Authentic AMD processor found\n\0" )


	SEG_TEXT

ALIGNTEXT4
GLOBL GLNAME( gl_identify_x86_cpu_features )
GLNAME( gl_identify_x86_cpu_features ):

	PUSH_L	( EBX )

	/* Test for the CPUID command.  If the ID Flag bit in EFLAGS
	 * (bit 21) is writable, the CPUID command is present.
	 */
	PUSHF_L
	POP_L	( EAX )
	MOV_L	( EAX, ECX )
	XOR_L	( CONST(0x00200000), EAX )
	PUSH_L	( EAX )
	POPF_L
	PUSHF_L
	POP_L	( EAX )

	/* Verify the ID Flag bit has been written.
	 */
	CMP_L	( ECX, EAX )
	JZ	( LLBL ( cpuid_done ) )

	/* Get the CPU vendor info.
	 */
	XOR_L	( EAX, EAX )
	CPUID

	/* Test for Intel processors.  We must look for the
	 * "GenuineIntel" string in EBX, ECX and EDX.
	 */
	CMP_L	( CONST(GENU), EBX )
	JNE	( LLBL( cpuid_amd ) )
	CMP_L	( CONST(INEI), EDX )
	JNE	( LLBL( cpuid_amd ) )
	CMP_L	( CONST(NTEL), ECX )
	JNE	( LLBL( cpuid_amd ) )

	/* We have an Intel processor, so we can get the feature
	 * information with an CPUID input value of 1.
	 */
	MOV_L	( CONST(0x1), EAX )
	CPUID
	MOV_L	( EDX, EAX )
	JMP	( LLBL( cpuid_done ) )

LLBL( cpuid_amd ):

	/* Test for AMD processors.  We must look for the
	 * "AuthenticAMD" string in EBX, ECX and EDX.
	 */
	CMP_L	( CONST(AUTH), EBX )
	JNE	( LLBL( cpuid_other ) )
	CMP_L	( CONST(ENTI), EDX )
	JNE	( LLBL( cpuid_other ) )
	CMP_L	( CONST(CAMD), ECX )
	JNE	( LLBL( cpuid_other ) )

	/* We have an AMD processor, so we can get the feature
	 * information after we verify that the extended functions are
	 * supported.
	 */
	MOV_L	( CONST(0x80000000), EAX )
	CPUID
	TEST_L	( EAX, EAX )
	JZ	( LLBL ( cpuid_failed ) )

	MOV_L	( CONST(0x80000001), EAX )
	CPUID
	MOV_L	( EDX, EAX )
	JMP	( LLBL ( cpuid_done ) )

LLBL( cpuid_other ):

	/* Test for other processors here when required.
	 */

LLBL( cpuid_failed ):

	/* If we can't determine the feature information, we must
	 * return zero to indicate that no platform-specific
	 * optimizations can be used.
	 */
	MOV_L	( CONST(0), EAX )

LLBL ( cpuid_done ):

	POP_L	( EBX )
	RET

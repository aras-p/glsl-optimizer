/* $Id: x86_cliptest.S,v 1.6 2001/03/28 20:44:44 gareth Exp $ */

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

#include "matypes.h"
#include "clip_args.h"

#define FP_ONE		1065353216
#define FP_ZERO		0

#define SRC(i)		REGOFF(i * 4, ESI)
#define DST(i)		REGOFF(i * 4, EDI)
#define MAT(i)		REGOFF(i * 4, EDX)


/*
 * Table for clip test.
 *
 * 	bit6 = SRC(3) < 0
 * 	bit5 = SRC(2) < 0
 * 	bit4 = abs(S(2)) > abs(S(3))
 * 	bit3 = SRC(1) < 0
 * 	bit2 = abs(S(1)) > abs(S(3))
 * 	bit1 = SRC(0) < 0
 * 	bit0 = abs(S(0)) > abs(S(3))
 */

	SEG_DATA

clip_table:
	D_BYTE 0, 1, 0, 2, 4, 5, 4, 6
	D_BYTE 0, 1, 0, 2, 8, 9, 8, 10
	D_BYTE 32, 33, 32, 34, 36, 37, 36, 38
	D_BYTE 32, 33, 32, 34, 40, 41, 40, 42
	D_BYTE 0, 1, 0, 2, 4, 5, 4, 6
	D_BYTE 0, 1, 0, 2, 8, 9, 8, 10
	D_BYTE 16, 17, 16, 18, 20, 21, 20, 22
	D_BYTE 16, 17, 16, 18, 24, 25, 24, 26
	D_BYTE 63, 61, 63, 62, 55, 53, 55, 54
	D_BYTE 63, 61, 63, 62, 59, 57, 59, 58
	D_BYTE 47, 45, 47, 46, 39, 37, 39, 38
	D_BYTE 47, 45, 47, 46, 43, 41, 43, 42
	D_BYTE 63, 61, 63, 62, 55, 53, 55, 54
	D_BYTE 63, 61, 63, 62, 59, 57, 59, 58
	D_BYTE 31, 29, 31, 30, 23, 21, 23, 22
	D_BYTE 31, 29, 31, 30, 27, 25, 27, 26


	SEG_TEXT

/*
 * _mesa_x86_cliptest_points4
 *
 *   AL:  ormask
 *   AH:  andmask
 *   EBX: temp0
 *   ECX: temp1
 *   EDX: clipmask[]
 *   ESI: clip[]
 *   EDI: proj[]
 *   EBP: temp2
 */

#if defined(__ELF__) && defined(__PIC__) && !defined(ELFPIC)
#define ELFPIC
#endif

ALIGNTEXT16
GLOBL GLNAME( _mesa_x86_cliptest_points4 )
GLNAME( _mesa_x86_cliptest_points4 ):

#ifdef ELFPIC
#define FRAME_OFFSET 20
#else
#define FRAME_OFFSET 16
#endif
	PUSH_L( ESI )
	PUSH_L( EDI )
	PUSH_L( EBP )
	PUSH_L( EBX )

#ifdef ELFPIC
	/* store pointer to clip_table on stack */
	CALL( LLBL( ctp4_get_eip ) )
	ADD_L( CONST(_GLOBAL_OFFSET_TABLE_), EBX )
	MOV_L( REGOFF(clip_table@GOT, EBX), EBX )
	PUSH_L( EBX )
	JMP( LLBL( ctp4_clip_table_ready ) )

LLBL( ctp4_get_eip ):
	/* store eip in ebx */
	MOV_L( REGIND(ESP), EBX )
	RET

LLBL( ctp4_clip_table_ready ):
#endif

	MOV_L( ARG_SOURCE, ESI )
	MOV_L( ARG_DEST, EDI )

	MOV_L( ARG_CLIP, EDX )
	MOV_L( ARG_OR, EBX )

	MOV_L( ARG_AND, EBP )
	MOV_L( REGOFF(V4F_STRIDE, ESI), EAX )

	MOV_L( REGOFF(V4F_COUNT, ESI), ECX )
	MOV_L( REGOFF(V4F_START, ESI), ESI )

	OR_L( CONST(VEC_SIZE_4), REGOFF(V4F_FLAGS, EDI) )
	MOV_L( EAX, ARG_SOURCE )	/* put stride in ARG_SOURCE */

	MOV_L( CONST(4), REGOFF(V4F_SIZE, EDI) )
	MOV_L( ECX, REGOFF(V4F_COUNT, EDI) )

	MOV_L( REGOFF(V4F_START, EDI), EDI )
	ADD_L( EDX, ECX )

	MOV_L( ECX, ARG_CLIP )		/* put clipmask + count in ARG_CLIP */
	CMP_L( ECX, EDX )

	MOV_B( REGIND(EBX), AL )
	MOV_B( REGIND(EBP), AH )

	JZ( LLBL( ctp4_finish ) )

ALIGNTEXT16
LLBL( ctp4_top ):

	FLD1			/* F3 */
	FDIV_S( SRC(3) )

	MOV_L( SRC(3), EBP )
	MOV_L( SRC(2), EBX )

	XOR_L( ECX, ECX )
	ADD_L( EBP, EBP )	/* ebp = abs(S(3))*2 ; carry = sign of S(3) */

	ADC_L( ECX, ECX )
	ADD_L( EBX, EBX )	/* ebx = abs(S(2))*2 ; carry = sign of S(2) */

	ADC_L( ECX, ECX )
	CMP_L( EBX, EBP )	/* carry = abs(S(2))*2 > abs(S(3))*2 */

	ADC_L( ECX, ECX )
	MOV_L( SRC(1), EBX )

	ADD_L( EBX, EBX )	/* ebx = abs(S(1))*2 ; carry = sign of S(1) */

	ADC_L( ECX, ECX )
	CMP_L( EBX, EBP )	/* carry = abs(S(1))*2 > abs(S(3))*2 */

	ADC_L( ECX, ECX )
	MOV_L( SRC(0), EBX )

	ADD_L( EBX, EBX )	/* ebx = abs(S(0))*2 ; carry = sign of S(0) */

	ADC_L( ECX, ECX )
	CMP_L( EBX, EBP )	/* carry = abs(S(0))*2 > abs(S(3))*2 */

	ADC_L( ECX, ECX )

#ifdef ELFPIC
	MOV_L( REGIND(ESP), EBP )	/* clip_table */

	MOV_B( REGBI(EBP, ECX), CL )
#else
	MOV_B( REGOFF(clip_table,ECX), CL )
#endif

	OR_B( CL, AL )
	AND_B( CL, AH )

	TEST_B( CL, CL )
	MOV_B( CL, REGIND(EDX) )

	JZ( LLBL( ctp4_proj ) )

	FSTP( ST(0) )			/* */
	JMP( LLBL( ctp4_next ) )

LLBL( ctp4_proj ):

	FLD_S( SRC(0) )			/* F0 F3 */
	FMUL2( ST(1), ST(0) )

	FLD_S( SRC(1) )			/* F1 F0 F3 */
	FMUL2( ST(2), ST(0) )

	FLD_S( SRC(2) )			/* F2 F1 F0 F3 */
	FMUL2( ST(3), ST(0) )

	FXCH( ST(2) )			/* F0 F1 F2 F3 */
	FSTP_S( DST(0) )		/* F1 F2 F3 */
	FSTP_S( DST(1) )		/* F2 F3 */
	FSTP_S( DST(2) )		/* F3 */
	FSTP_S( DST(3) )		/* */

LLBL( ctp4_next ):

	INC_L( EDX )
	ADD_L( CONST(16), EDI )

	ADD_L( ARG_SOURCE, ESI )
	CMP_L( EDX, ARG_CLIP )

	JNZ( LLBL( ctp4_top ) )

	MOV_L( ARG_OR, ECX )
	MOV_L( ARG_AND, EDX )

	MOV_B( AL, REGIND(ECX) )
	MOV_B( AH, REGIND(EDX) )

LLBL( ctp4_finish ):

	MOV_L( ARG_DEST, EAX )
#ifdef ELFPIC
	POP_L( ESI )			/* discard ptr to clip_table */
#endif
	POP_L( EBX )
	POP_L( EBP )
	POP_L( EDI )
	POP_L( ESI )

	RET







ALIGNTEXT16
GLOBL GLNAME( _mesa_x86_cliptest_points4_np )
GLNAME( _mesa_x86_cliptest_points4_np ):

#ifdef ELFPIC
#define FRAME_OFFSET 20
#else
#define FRAME_OFFSET 16
#endif
	PUSH_L( ESI )
	PUSH_L( EDI )
	PUSH_L( EBP )
	PUSH_L( EBX )

#ifdef ELFPIC
	/* store pointer to clip_table on stack */
	CALL( LLBL( ctp4_np_get_eip ) )
	ADD_L( CONST(_GLOBAL_OFFSET_TABLE_), EBX )
	MOV_L( REGOFF(clip_table@GOT, EBX), EBX )
	PUSH_L( EBX )
	JMP( LLBL( ctp4_np_clip_table_ready ) )

LLBL( ctp4_np_get_eip ):
	/* store eip in ebx */
	MOV_L( REGIND(ESP), EBX )
	RET

LLBL( ctp4_np_clip_table_ready ):
#endif

	MOV_L( ARG_SOURCE, ESI )
	/* slot */

	MOV_L( ARG_CLIP, EDX )
	MOV_L( ARG_OR, EBX )

	MOV_L( ARG_AND, EBP )
	MOV_L( REGOFF(V4F_STRIDE, ESI), EAX )

	MOV_L( REGOFF(V4F_COUNT, ESI), ECX )
	MOV_L( REGOFF(V4F_START, ESI), ESI )

	MOV_L( EAX, ARG_DEST )   	/* put stride in ARG_DEST */
	ADD_L( EDX, ECX )

	MOV_L( ECX, EDI )		/* put clipmask + count in EDI */
	CMP_L( ECX, EDX )

	MOV_B( REGIND(EBX), AL )
	MOV_B( REGIND(EBP), AH )

	JZ( LLBL( ctp4_np_finish ) )

ALIGNTEXT16
LLBL( ctp4_np_top ):

	MOV_L( SRC(3), EBP )
	MOV_L( SRC(2), EBX )

	XOR_L( ECX, ECX )
	ADD_L( EBP, EBP )	/* ebp = abs(S(3))*2 ; carry = sign of S(3) */

	ADC_L( ECX, ECX )
	ADD_L( EBX, EBX )	/* ebx = abs(S(2))*2 ; carry = sign of S(2) */

	ADC_L( ECX, ECX )
	CMP_L( EBX, EBP )	/* carry = abs(S(2))*2 > abs(S(3))*2 */

	ADC_L( ECX, ECX )
	MOV_L( SRC(1), EBX )

	ADD_L( EBX, EBX )	/* ebx = abs(S(1))*2 ; carry = sign of S(1) */

	ADC_L( ECX, ECX )
	CMP_L( EBX, EBP )	/* carry = abs(S(1))*2 > abs(S(3))*2 */

	ADC_L( ECX, ECX )
	MOV_L( SRC(0), EBX )

	ADD_L( EBX, EBX )	/* ebx = abs(S(0))*2 ; carry = sign of S(0) */

	ADC_L( ECX, ECX )
	CMP_L( EBX, EBP )	/* carry = abs(S(0))*2 > abs(S(3))*2 */

	ADC_L( ECX, ECX )

#ifdef ELFPIC
	MOV_L( REGIND(ESP), EBP )	/* clip_table */

	MOV_B( REGBI(EBP, ECX), CL )
#else
	MOV_B( REGOFF(clip_table,ECX), CL )
#endif

	OR_B( CL, AL )
	AND_B( CL, AH )

	TEST_B( CL, CL )
	MOV_B( CL, REGIND(EDX) )

	INC_L( EDX )
	/* slot */

	ADD_L( ARG_DEST, ESI )
	CMP_L( EDX, EDI )

	JNZ( LLBL( ctp4_np_top ) )

	MOV_L( ARG_OR, ECX )
	MOV_L( ARG_AND, EDX )

	MOV_B( AL, REGIND(ECX) )
	MOV_B( AH, REGIND(EDX) )

LLBL( ctp4_np_finish ):

	MOV_L( ARG_SOURCE, EAX )
#ifdef ELFPIC
	POP_L( ESI )			/* discard ptr to clip_table */
#endif
	POP_L( EBX )
	POP_L( EBP )
	POP_L( EDI )
	POP_L( ESI )

	RET

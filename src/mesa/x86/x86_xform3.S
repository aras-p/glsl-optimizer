/* $Id: x86_xform3.S,v 1.1 2001/03/29 06:46:27 gareth Exp $ */

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
#include "xform_args.h"

	SEG_TEXT

#define FP_ONE		1065353216
#define FP_ZERO		0

#define SRC(i)		REGOFF(i * 4, ESI)
#define DST(i)		REGOFF(i * 4, EDI)
#define MAT(i)		REGOFF(i * 4, EDX)


ALIGNTEXT16
GLOBL GLNAME( _mesa_x86_transform_points3_general )
GLNAME( _mesa_x86_transform_points3_general ):

#define FRAME_OFFSET 8
	PUSH_L( ESI )
	PUSH_L( EDI )

	MOV_L( ARG_SOURCE, ESI )
	MOV_L( ARG_DEST, EDI )

	MOV_L( ARG_MATRIX, EDX )
	MOV_L( REGOFF(V4F_COUNT, ESI), ECX )

	TEST_L( ECX, ECX )
	JZ( LLBL( x86_p3_gr_done ) )

	MOV_L( REGOFF(V4F_STRIDE, ESI), EAX )
	OR_L( CONST(VEC_SIZE_4), REGOFF(V4F_FLAGS, EDI) )

	MOV_L( ECX, REGOFF(V4F_COUNT, EDI) )
	MOV_L( CONST(4), REGOFF(V4F_SIZE, EDI) )

	SHL_L( CONST(4), ECX )
	MOV_L( REGOFF(V4F_START, ESI), ESI )

	MOV_L( REGOFF(V4F_START, EDI), EDI )
	ADD_L( EDI, ECX )

ALIGNTEXT16
LLBL( x86_p3_gr_loop ):

	FLD_S( SRC(0) )			/* F4 */
	FMUL_S( MAT(0) )
	FLD_S( SRC(0) )			/* F5 F4 */
	FMUL_S( MAT(1) )
	FLD_S( SRC(0) )			/* F6 F5 F4 */
	FMUL_S( MAT(2) )
	FLD_S( SRC(0) )			/* F7 F6 F5 F4 */
	FMUL_S( MAT(3) )

	FLD_S( SRC(1) )			/* F0 F7 F6 F5 F4 */
	FMUL_S( MAT(4) )
	FLD_S( SRC(1) )			/* F1 F0 F7 F6 F5 F4 */
	FMUL_S( MAT(5) )
	FLD_S( SRC(1) )			/* F2 F1 F0 F7 F6 F5 F4 */
	FMUL_S( MAT(6) )
	FLD_S( SRC(1) )			/* F3 F2 F1 F0 F7 F6 F5 F4 */
	FMUL_S( MAT(7) )

	FXCH( ST(3) )			/* F0 F2 F1 F3 F7 F6 F5 F4 */
	FADDP( ST(0), ST(7) )		/* F2 F1 F3 F7 F6 F5 F4 */
	FXCH( ST(1) )			/* F1 F2 F3 F7 F6 F5 F4 */
	FADDP( ST(0), ST(5) )		/* F2 F3 F7 F6 F5 F4 */
	FADDP( ST(0), ST(3) )		/* F3 F7 F6 F5 F4 */
	FADDP( ST(0), ST(1) )		/* F7 F6 F5 F4 */

	FLD_S( SRC(2) )			/* F0 F7 F6 F5 F4 */
	FMUL_S( MAT(8) )
	FLD_S( SRC(2) )			/* F1 F0 F7 F6 F5 F4 */
	FMUL_S( MAT(9) )
	FLD_S( SRC(2) )			/* F2 F1 F0 F7 F6 F5 F4 */
	FMUL_S( MAT(10) )
	FLD_S( SRC(2) )			/* F3 F2 F1 F0 F7 F6 F5 F4 */
	FMUL_S( MAT(11) )

	FXCH( ST(3) )			/* F0 F2 F1 F3 F7 F6 F5 F4 */
	FADDP( ST(0), ST(7) )		/* F2 F1 F3 F7 F6 F5 F4 */
	FXCH( ST(1) )			/* F1 F2 F3 F7 F6 F5 F4 */
	FADDP( ST(0), ST(5) )		/* F2 F3 F7 F6 F5 F4 */
	FADDP( ST(0), ST(3) )		/* F3 F7 F6 F5 F4 */
	FADDP( ST(0), ST(1) )		/* F7 F6 F5 F4 */

	FXCH( ST(3) )			/* F4 F6 F5 F7 */
	FADD_S( MAT(12) )
	FXCH( ST(2) )			/* F5 F6 F4 F7 */
	FADD_S( MAT(13) )
	FXCH( ST(1) )			/* F6 F5 F4 F7 */
	FADD_S( MAT(14) )
	FXCH( ST(3) )			/* F7 F5 F4 F6 */
	FADD_S( MAT(15) )

	FXCH( ST(2) )			/* F4 F5 F7 F6 */
	FSTP_S( DST(0) )		/* F5 F7 F6 */
	FSTP_S( DST(1) )		/* F7 F6 */
	FXCH( ST(1) )			/* F6 F7 */
	FSTP_S( DST(2) )		/* F7 */
	FSTP_S( DST(3) )		/* */

LLBL( x86_p3_gr_skip ):

	ADD_L( CONST(16), EDI )
	ADD_L( EAX, ESI )
	CMP_L( ECX, EDI )
	JNE( LLBL( x86_p3_gr_loop ) )

LLBL( x86_p3_gr_done ):

	POP_L( EDI )
	POP_L( ESI )
	RET
#undef FRAME_OFFSET




ALIGNTEXT16
GLOBL GLNAME( _mesa_x86_transform_points3_perspective )
GLNAME( _mesa_x86_transform_points3_perspective ):

#define FRAME_OFFSET 12
	PUSH_L( ESI )
	PUSH_L( EDI )
	PUSH_L( EBX )

	MOV_L( ARG_SOURCE, ESI )
	MOV_L( ARG_DEST, EDI )

	MOV_L( ARG_MATRIX, EDX )
	MOV_L( REGOFF(V4F_COUNT, ESI), ECX )

	TEST_L( ECX, ECX )
	JZ( LLBL( x86_p3_pr_done ) )

	MOV_L( REGOFF(V4F_STRIDE, ESI), EAX )
	OR_L( CONST(VEC_SIZE_4), REGOFF(V4F_FLAGS, EDI) )

	MOV_L( ECX, REGOFF(V4F_COUNT, EDI) )
	MOV_L( CONST(4), REGOFF(V4F_SIZE, EDI) )

	SHL_L( CONST(4), ECX )
	MOV_L( REGOFF(V4F_START, ESI), ESI )

	MOV_L( REGOFF(V4F_START, EDI), EDI )
	ADD_L( EDI, ECX )

ALIGNTEXT16
LLBL( x86_p3_pr_loop ):

	FLD_S( SRC(0) )			/* F4 */
	FMUL_S( MAT(0) )

	FLD_S( SRC(1) )			/* F5 F4 */
	FMUL_S( MAT(5) )

	FLD_S( SRC(2) )			/* F0 F5 F4 */
	FMUL_S( MAT(8) )
	FLD_S( SRC(2) )			/* F1 F0 F5 F4 */
	FMUL_S( MAT(9) )
	FLD_S( SRC(2) )			/* F2 F1 F0 F5 F4 */
	FMUL_S( MAT(10) )

	FXCH( ST(2) )			/* F0 F1 F2 F5 F4 */
	FADDP( ST(0), ST(4) )		/* F1 F2 F5 F4 */
	FADDP( ST(0), ST(2) )		/* F2 F5 F4 */
	FLD_S( MAT(14) )		/* F6 F2 F5 F4 */
	FXCH( ST(1) )			/* F2 F6 F5 F4 */
	FADDP( ST(0), ST(1) )		/* F6 F5 F4 */

	MOV_L( SRC(2), EBX )
	XOR_L( CONST(-2147483648), EBX )/* change sign */

	FXCH( ST(2) )			/* F4 F5 F6 */
	FSTP_S( DST(0) )		/* F5 F6 */
	FSTP_S( DST(1) )		/* F6 */
	FSTP_S( DST(2) )		/* */
	MOV_L( EBX, DST(3) )

LLBL( x86_p3_pr_skip ):

	ADD_L( CONST(16), EDI )
	ADD_L( EAX, ESI )
	CMP_L( ECX, EDI )
	JNE( LLBL( x86_p3_pr_loop ) )

LLBL( x86_p3_pr_done ):

	POP_L( EBX )
	POP_L( EDI )
	POP_L( ESI )
	RET
#undef FRAME_OFFSET




ALIGNTEXT16
GLOBL GLNAME( _mesa_x86_transform_points3_3d )
GLNAME( _mesa_x86_transform_points3_3d ):

#define FRAME_OFFSET 8
	PUSH_L( ESI )
	PUSH_L( EDI )

	MOV_L( ARG_SOURCE, ESI )
	MOV_L( ARG_DEST, EDI )

	MOV_L( ARG_MATRIX, EDX )
	MOV_L( REGOFF(V4F_COUNT, ESI), ECX )

	TEST_L( ECX, ECX )
	JZ( LLBL( x86_p3_3dr_done ) )

	MOV_L( REGOFF(V4F_STRIDE, ESI), EAX )
	OR_L( CONST(VEC_SIZE_3), REGOFF(V4F_FLAGS, EDI) )

	MOV_L( ECX, REGOFF(V4F_COUNT, EDI) )
	MOV_L( CONST(3), REGOFF(V4F_SIZE, EDI) )

	SHL_L( CONST(4), ECX )
	MOV_L( REGOFF(V4F_START, ESI), ESI )

	MOV_L( REGOFF(V4F_START, EDI), EDI )
	ADD_L( EDI, ECX )

ALIGNTEXT16
LLBL( x86_p3_3dr_loop ):

	FLD_S( SRC(0) )			/* F4 */
	FMUL_S( MAT(0) )
	FLD_S( SRC(0) )			/* F5 F4 */
	FMUL_S( MAT(1) )
	FLD_S( SRC(0) )			/* F6 F5 F4 */
	FMUL_S( MAT(2) )

	FLD_S( SRC(1) )			/* F0 F6 F5 F4 */
	FMUL_S( MAT(4) )
	FLD_S( SRC(1) )			/* F1 F0 F6 F5 F4 */
	FMUL_S( MAT(5) )
	FLD_S( SRC(1) )			/* F2 F1 F0 F6 F5 F4 */
	FMUL_S( MAT(6) )

	FXCH( ST(2) )			/* F0 F1 F2 F6 F5 F4 */
	FADDP( ST(0), ST(5) )		/* F1 F2 F6 F5 F4 */
	FADDP( ST(0), ST(3) )		/* F2 F6 F5 F4 */
	FADDP( ST(0), ST(1) )		/* F6 F5 F4 */

	FLD_S( SRC(2) )			/* F0 F6 F5 F4 */
	FMUL_S( MAT(8) )
	FLD_S( SRC(2) )			/* F1 F0 F6 F5 F4 */
	FMUL_S( MAT(9) )
	FLD_S( SRC(2) )			/* F2 F1 F0 F6 F5 F4 */
	FMUL_S( MAT(10) )

	FXCH( ST(2) )			/* F0 F1 F2 F6 F5 F4 */
	FADDP( ST(0), ST(5) )		/* F1 F2 F6 F5 F4 */
	FADDP( ST(0), ST(3) )		/* F2 F6 F5 F4 */
	FADDP( ST(0), ST(1) )		/* F6 F5 F4 */

	FXCH( ST(2) )			/* F4 F5 F6 */
	FADD_S( MAT(12) )
	FXCH( ST(1) )			/* F5 F4 F6 */
	FADD_S( MAT(13) )
	FXCH( ST(2) )			/* F6 F4 F5 */
	FADD_S( MAT(14) )

	FXCH( ST(1) )			/* F4 F6 F5 */
	FSTP_S( DST(0)   )		/* F6 F5 */
	FXCH( ST(1) )			/* F5 F6 */
	FSTP_S( DST(1)   )		/* F6 */
	FSTP_S( DST(2)   )		/* */

LLBL( x86_p3_3dr_skip ):

	ADD_L( CONST(16), EDI )
	ADD_L( EAX, ESI )
	CMP_L( ECX, EDI )
	JNE( LLBL( x86_p3_3dr_loop ) )

LLBL( x86_p3_3dr_done ):

	POP_L( EDI )
	POP_L( ESI )
	RET
#undef FRAME_OFFSET




ALIGNTEXT16
GLOBL GLNAME( _mesa_x86_transform_points3_3d_no_rot )
GLNAME( _mesa_x86_transform_points3_3d_no_rot ):

#define FRAME_OFFSET 8
	PUSH_L( ESI )
	PUSH_L( EDI )

	MOV_L( ARG_SOURCE, ESI )
	MOV_L( ARG_DEST, EDI )


	MOV_L( ARG_MATRIX, EDX )
	MOV_L( REGOFF(V4F_COUNT, ESI), ECX )

	TEST_L( ECX, ECX )
	JZ( LLBL( x86_p3_3dnrr_done ) )

	MOV_L( REGOFF(V4F_STRIDE, ESI), EAX )
	OR_L( CONST(VEC_SIZE_3), REGOFF(V4F_FLAGS, EDI) )

	MOV_L( ECX, REGOFF(V4F_COUNT, EDI) )
	MOV_L( CONST(3), REGOFF(V4F_SIZE, EDI) )

	SHL_L( CONST(4), ECX )
	MOV_L( REGOFF(V4F_START, ESI), ESI )

	MOV_L( REGOFF(V4F_START, EDI), EDI )
	ADD_L( EDI, ECX )

ALIGNTEXT16
LLBL( x86_p3_3dnrr_loop ):

	FLD_S( SRC(0) )			/* F4 */
	FMUL_S( MAT(0) )

	FLD_S( SRC(1) )			/* F1 F4 */
	FMUL_S( MAT(5) )

	FLD_S( SRC(2) )			/* F2 F1 F4 */
	FMUL_S( MAT(10) )

	FXCH( ST(2) )			/* F4 F1 F2 */
	FADD_S( MAT(12) )
	FLD_S( MAT(13) )		/* F5 F4 F1 F2 */
	FXCH( ST(2) )			/* F1 F4 F5 F2 */
	FADDP( ST(0), ST(2) )		/* F4 F5 F2 */
	FLD_S( MAT(14) )		/* F6 F4 F5 F2 */
	FXCH( ST(3) )			/* F2 F4 F5 F6 */
	FADDP( ST(0), ST(3) )		/* F4 F5 F6 */

	FSTP_S( DST(0)   )		/* F5 F6 */
	FSTP_S( DST(1)   )		/* F6 */
	FSTP_S( DST(2)   )		/* */

LLBL( x86_p3_3dnrr_skip ):

	ADD_L( CONST(16), EDI )
	ADD_L( EAX, ESI )
	CMP_L( ECX, EDI )
	JNE( LLBL( x86_p3_3dnrr_loop ) )

LLBL( x86_p3_3dnrr_done ):

	POP_L( EDI )
	POP_L( ESI )
	RET
#undef FRAME_OFFSET




ALIGNTEXT16
GLOBL GLNAME( _mesa_x86_transform_points3_2d )
GLNAME( _mesa_x86_transform_points3_2d ):

#define FRAME_OFFSET 12
	PUSH_L( ESI )
	PUSH_L( EDI )
	PUSH_L( EBX )

	MOV_L( ARG_SOURCE, ESI )
	MOV_L( ARG_DEST, EDI )

	MOV_L( ARG_MATRIX, EDX )
	MOV_L( REGOFF(V4F_COUNT, ESI), ECX )

	TEST_L( ECX, ECX )
	JZ( LLBL( x86_p3_2dr_done ) )

	MOV_L( REGOFF(V4F_STRIDE, ESI), EAX )
	OR_L( CONST(VEC_SIZE_3), REGOFF(V4F_FLAGS, EDI) )

	MOV_L( ECX, REGOFF(V4F_COUNT, EDI) )
	MOV_L( CONST(3), REGOFF(V4F_SIZE, EDI) )

	SHL_L( CONST(4), ECX )
	MOV_L( REGOFF(V4F_START, ESI), ESI )

	MOV_L( REGOFF(V4F_START, EDI), EDI )
	ADD_L( EDI, ECX )

ALIGNTEXT16
LLBL( x86_p3_2dr_loop ):

	FLD_S( SRC(0) )			/* F4 */
	FMUL_S( MAT(0) )
	FLD_S( SRC(0) )			/* F5 F4 */
	FMUL_S( MAT(1) )

	FLD_S( SRC(1) )			/* F0 F5 F4 */
	FMUL_S( MAT(4) )
	FLD_S( SRC(1) )			/* F1 F0 F5 F4 */
	FMUL_S( MAT(5) )

	FXCH( ST(1) )			/* F0 F1 F5 F4 */
	FADDP( ST(0), ST(3) )		/* F1 F5 F4 */
	FADDP( ST(0), ST(1) )		/* F5 F4 */

	FXCH( ST(1) )			/* F4 F5 */
	FADD_S( MAT(12) )
	FXCH( ST(1) )			/* F5 F4 */
	FADD_S( MAT(13) )

	MOV_L( SRC(2), EBX )

	FXCH( ST(1) )			/* F4 F5 */
	FSTP_S( DST(0)   )		/* F5 */
	FSTP_S( DST(1)   )		/* */
	MOV_L( EBX, DST(2) )

LLBL( x86_p3_2dr_skip ):

	ADD_L( CONST(16), EDI )
	ADD_L( EAX, ESI )
	CMP_L( ECX, EDI )
	JNE( LLBL( x86_p3_2dr_loop ) )

LLBL( x86_p3_2dr_done ):

	POP_L( EBX )
	POP_L( EDI )
	POP_L( ESI )
	RET
#undef FRAME_OFFSET




ALIGNTEXT16
GLOBL GLNAME( _mesa_x86_transform_points3_2d_no_rot )
GLNAME( _mesa_x86_transform_points3_2d_no_rot ):

#define FRAME_OFFSET 12
	PUSH_L( ESI )
	PUSH_L( EDI )
	PUSH_L( EBX )

	MOV_L( ARG_SOURCE, ESI )
	MOV_L( ARG_DEST, EDI )

	MOV_L( ARG_MATRIX, EDX )
	MOV_L( REGOFF(V4F_COUNT, ESI), ECX )

	TEST_L( ECX, ECX )
	JZ( LLBL( x86_p3_2dnrr_done ) )

	MOV_L( REGOFF(V4F_STRIDE, ESI), EAX )
	OR_L( CONST(VEC_SIZE_3), REGOFF(V4F_FLAGS, EDI) )

	MOV_L( ECX, REGOFF(V4F_COUNT, EDI) )
	MOV_L( CONST(3), REGOFF(V4F_SIZE, EDI) )

	SHL_L( CONST(4), ECX )
	MOV_L( REGOFF(V4F_START, ESI), ESI )

	MOV_L( REGOFF(V4F_START, EDI), EDI )
	ADD_L( EDI, ECX )

ALIGNTEXT16
LLBL( x86_p3_2dnrr_loop ):

	FLD_S( SRC(0) )			/* F4 */
	FMUL_S( MAT(0) )

	FLD_S( SRC(1) )			/* F1 F4 */
	FMUL_S( MAT(5) )

	FXCH( ST(1) )			/* F4 F1 */
	FADD_S( MAT(12) )
	FLD_S( MAT(13) )		/* F5 F4 F1 */

	FXCH( ST(2) )			/* F1 F4 F5 */
	FADDP( ST(0), ST(2) )		/* F4 F5 */

	MOV_L( SRC(2), EBX )

	FSTP_S( DST(0) )		/* F5 */
	FSTP_S( DST(1) )		/* */
	MOV_L( EBX, DST(2) )

LLBL( x86_p3_2dnrr_skip ):

	ADD_L( CONST(16), EDI )
	ADD_L( EAX, ESI )
	CMP_L( ECX, EDI )
	JNE( LLBL( x86_p3_2dnrr_loop ) )

LLBL( x86_p3_2dnrr_done ):

	POP_L( EBX )
	POP_L( EDI )
	POP_L( ESI )
	RET
#undef FRAME_OFFSET




ALIGNTEXT16
GLOBL GLNAME( _mesa_x86_transform_points3_identity )
GLNAME(_mesa_x86_transform_points3_identity ):

#define FRAME_OFFSET 16
	PUSH_L( ESI )
	PUSH_L( EDI )
	PUSH_L( EBX )
	PUSH_L( EBP )

	MOV_L( ARG_SOURCE, ESI )
	MOV_L( ARG_DEST, EDI )

	MOV_L( ARG_MATRIX, EDX )
	MOV_L( REGOFF(V4F_COUNT, ESI), ECX )

	TEST_L( ECX, ECX )
	JZ( LLBL( x86_p3_ir_done ) )

	MOV_L( REGOFF(V4F_STRIDE, ESI), EAX )
	OR_L( CONST(VEC_SIZE_3), REGOFF(V4F_FLAGS, EDI) )

	MOV_L( ECX, REGOFF(V4F_COUNT, EDI) )
	MOV_L( CONST(3), REGOFF(V4F_SIZE, EDI) )

	SHL_L( CONST(4), ECX )
	MOV_L( REGOFF(V4F_START, ESI), ESI )

	MOV_L( REGOFF(V4F_START, EDI), EDI )
	ADD_L( EDI, ECX )

	CMP_L( ESI, EDI )
	JE( LLBL( x86_p3_ir_done ) )

ALIGNTEXT16
LLBL( x86_p3_ir_loop ):

#if 1
	MOV_L( SRC(0), EBX )
	MOV_L( SRC(1), EBP )
	MOV_L( SRC(2), EDX )

	MOV_L( EBX, DST(0) )
	MOV_L( EBP, DST(1) )
	MOV_L( EDX, DST(2) )
#else
	FLD_S( SRC(0) )
	FLD_S( SRC(1) )
	FLD_S( SRC(2) )

	FSTP_S( DST(2) )
	FSTP_S( DST(1) )
	FSTP_S( DST(0) )
#endif

LLBL( x86_p3_ir_skip ):

	ADD_L( CONST(16), EDI )
	ADD_L( EAX, ESI )
	CMP_L( ECX, EDI )
	JNE( LLBL( x86_p3_ir_loop ) )

LLBL( x86_p3_ir_done ):

	POP_L( EBP )
	POP_L( EBX )
	POP_L( EDI )
	POP_L( ESI )
	RET

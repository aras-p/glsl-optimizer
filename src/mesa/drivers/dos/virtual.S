/*
 * Mesa 3-D graphics library
 * Version:  4.0
 * 
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
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
 * DOS/DJGPP device driver v1.1 for Mesa 4.0
 *
 *  Copyright (C) 2002 - Borca Daniel
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */


		.file	"virtual.S"

/*
 * extern word32 vl_hw_granularity;
 * extern int vl_video_selector;
 * extern int vl_current_offset, vl_current_delta;
 */

		.text

/*
 * void v_clear16 (void *buffer, int bytes, int color);
 */
		.balign	4
		.global	_v_clear16
_v_clear16:
		movl	12(%esp), %eax
		pushw	%ax
		pushw	%ax
		popl	%eax
		jmp	_v_clear_common

/*
 * void v_clear32 (void *buffer, int bytes, int color);
 */
		.balign	4
		.global	_v_clear32
_v_clear32:
		movl	12(%esp), %eax
		.balign	4
_v_clear_common:
		movl	8(%esp), %ecx
		movl	4(%esp), %edx
		shrl	$2, %ecx
	0:
		.balign	4
		movl	%eax, (%edx)
		addl	$4, %edx
		decl	%ecx
		jnz	0b
		ret

/*
 * void v_clear24 (void *buffer, int bytes, int color);
 */
		.balign	4
		.global	_v_clear24
_v_clear24:
		movl	8(%esp), %edx
		movl	$0xaaaaaaab, %eax
		mull	%edx
		movl	12(%esp), %eax
		movl	%edx, %ecx
		movl	4(%esp), %edx
		pushl	%ebx
		shrl	%ecx
		movb	18(%esp), %bl
		.balign	4
	0:
		movw	%ax, (%edx)
		movb	%bl, 2(%edx)
		addl	$3, %edx
		decl	%ecx
		jnz	0b
		popl	%ebx
		ret

/*
 * void b_dump_virtual (void *buffer, int stride, int height);
 */
		.balign	4
		.global	_b_dump_virtual
_b_dump_virtual:
		pushl	%ebx
		pushl	%esi
		pushl	%edi
		pushl	%ebp
		movl	_vl_video_selector, %fs
		movl	4*4+4+0(%esp), %esi
		movl	_vl_hw_granularity, %ebp
		xorl	%edx, %edx
		movl	_vl_current_offset, %eax
		divl	%ebp
		movl	%edx, %edi
		pushl	%eax
		movl	%eax, %edx
		xorl	%ebx, %ebx
		movw	$0x4f05, %ax
		int	$0x10
		movl	_vl_current_delta, %ebx
		movl	5*4+4+4(%esp), %ecx
		movl	5*4+4+8(%esp), %edx
		shrl	$2, %ecx
		.balign	4
	0:
		pushl	%ecx
		.balign	4
	1:
		cmpl	%ebp, %edi
		jb	2f
		pushl	%ebx
		pushl	%edx
		incl	12(%esp)
		movw	$0x4f05, %ax
		movl	12(%esp), %edx
		xorl	%ebx, %ebx
		int	$0x10
		popl	%edx
		popl	%ebx
		subl	%ebp, %edi
	2:
		movl	(%esi), %eax
		addl	$4, %esi
		movl	%eax, %fs:(%edi)
		addl	$4, %edi
		decl	%ecx
		jnz	1b
		popl	%ecx
		addl	%ebx, %edi
		decl	%edx
		jnz	0b
		popl	%eax
		popl	%ebp
		popl	%edi
		popl	%esi
		popl	%ebx
		ret

/*
 * void l_dump_virtual (void *buffer, int stride, int height);
 */
		.balign	4
		.global	_l_dump_virtual
_l_dump_virtual:
		pushl	%ebx
		pushl	%esi
		pushl	%edi
		movl	_vl_video_selector, %fs
		movl	3*4+4+0(%esp), %esi
		movl	_vl_current_offset, %edi
		movl	3*4+4+4(%esp), %ecx
		movl	3*4+4+8(%esp), %edx
		movl	_vl_current_delta, %ebx
		shrl	$2, %ecx
		.balign	4
	0:
		pushl	%ecx
		.balign	4
	1:
		movl	(%esi), %eax
		addl	$4, %esi
		movl	%eax, %fs:(%edi)
		addl	$4, %edi
		decl	%ecx
		jnz	1b
		popl	%ecx
		addl	%ebx, %edi
		decl	%edx
		jnz	0b
		popl	%edi
		popl	%esi
		popl	%ebx
		ret

/*
 * void v_putpixel16 (void *buffer, int offset, int color);
 */
		.balign	4
		.global	_v_putpixel16
_v_putpixel16:
		movl	8(%esp), %edx
		shll	%edx
		movl	12(%esp), %eax
		addl	4(%esp), %edx
		movw	%ax, (%edx)
		ret

/*
 * void v_putpixel24 (void *buffer, int offset, int color);
 */
		.balign	4
		.global	_v_putpixel24
_v_putpixel24:
		movl	8(%esp), %edx
		leal	(%edx, %edx, 2), %edx
		movl	12(%esp), %eax
		addl	4(%esp), %edx
		movw	%ax, (%edx)
		shrl	$16, %eax
		movb	%al, 2(%edx)
		ret

/*
 * void v_putpixel32 (void *buffer, int offset, int color);
 */
		.balign	4
		.global	_v_putpixel32
_v_putpixel32:
		movl	8(%esp), %edx
		shll	$2, %edx
		movl	12(%esp), %eax
		addl	4(%esp), %edx
		movl	%eax, (%edx)
		ret

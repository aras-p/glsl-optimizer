/**************************************************************************

Copyright 2004 Tungsten Graphics Inc., Cedar Park, Texas.

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ATI, TUNGSTEN GRAPHICS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

	

	
			
#define GLOBL( x )	\
.globl x;		\
x:

.data
.align 4

	
	
GLOBL ( _x86_Vertex1fv )
	;; v already in eax
	push	%edi
	push	%esi
	movl	(0x0), %edi	; load vbptr
	movl	(%eax), %edx	; load v[0]
	movl	%edx, (%edi)	; vbptr[0] = v[0]
	addl	$4, %edi	; vbptr += 1
	movl	$0x0, %ecx	; vertex_size - 1
	movl	$0x0, %esi	; tnl->vtx.vertex + 1
	repz
	movsl %ds:(%esi), %es:(%edi)
	movl	%edi, (0)	; save vbptr
	movl	(0), %edx	; load counter
	pop	%esi
	pop	%edi
	dec	%edx		; counter--
	movl	%edx, (0)	; save counter
	je	.5		; if (counter != 0)
	ret			;    return
.5:	jmp    *0		; else notify();
GLOBL ( _x86_Vertex1fv_end )


GLOBL ( _x86_Vertex2fv )
	;; v already in eax
	push	%edi
	push	%esi
	movl	(0x0), %edi	; load vbptr
	movl	(%eax), %edx	; load v[0]
	movl	4(%eax), %ecx	; load v[1]
	movl	%edx, (%edi)	; vbptr[0] = v[0]
	movl	%ecx, 4(%edi)	; vbptr[1] = v[1]
	addl	$8, %edi	; vbptr += 2
	movl	$0x0, %ecx	; vertex_size - 2
	movl	$0x0, %esi	; tnl->vtx.vertex + 2
	repz
	movsl %ds:(%esi), %es:(%edi)
	movl	%edi, (0)	; save vbptr
	movl	(0), %edx	; load counter
	pop	%esi
	pop	%edi
	dec	%edx		; counter--
	movl	%edx, (0)	; save counter
	je	.6		; if (counter != 0)
	ret			;    return
.6:	jmp    *0		; else notify();
GLOBL ( _x86_Vertex3fv_end )

GLOBL ( _x86_Vertex3fv )
	;; v already in eax
	push	%edi
	push	%esi
	movl	(0x0), %edi	; load vbptr
	movl	(%eax), %edx	; load v[0]
	movl	4(%eax), %ecx	; load v[1]
	movl	8(%eax), %esi	; load v[2]
	movl	%edx, (%edi)	; vbptr[0] = v[0]
	movl	%ecx, 4(%edi)	; vbptr[1] = v[1]
	movl	%esi, 8(%edi)	; vbptr[2] = v[2]
	addl	$12, %edi	; vbptr += 3
	movl	$0x0, %ecx	; vertex_size - 3
	movl	$0x0, %esi	; tnl->vtx.vertex + 3
	repz
	movsl %ds:(%esi), %es:(%edi)
	movl	%edi, (0)	; save vbptr
	movl	(0), %edx	; load counter
	pop	%esi
	pop	%edi
	dec	%edx		; counter--
	movl	%edx, (0)	; save counter
	je	.7		; if (counter != 0)
	ret			;    return
.7:	jmp    *0		; else notify();
GLOBL ( _x86_Vertex3fv_end )

			
GLOBL ( _x86_Vertex4fv )
	;; v already in eax
	push	%edi
	push	%esi
	movl	(0x0), %edi	; load vbptr
	movl	(%eax), %edx	; load v[0]
	movl	4(%eax), %ecx	; load v[1]
	movl	8(%eax), %esi	; load v[2]
	movl	%edx, (%edi)	; vbptr[0] = v[0]
	movl	%ecx, 4(%edi)	; vbptr[1] = v[1]
	movl	%esi, 8(%edi)	; vbptr[2] = v[2]
	movl	12(%eax), %esi	; load v[3]
	movl	%esi, 12(%edi)	; vbptr[3] = v[3]
	addl	$16, %edi	; vbptr += 4
	movl	$0x0, %ecx	; vertex_size - 4
	movl	$0x0, %esi	; tnl->vtx.vertex + 3
	repz
	movsl %ds:(%esi), %es:(%edi)
	movl	%edi, (0)	; save vbptr
	movl	(0), %edx	; load counter
	pop	%esi
	pop	%edi
	dec	%edx		; counter--
	movl	%edx, (0)	; save counter
	je	.6		; if (counter != 0)
	ret			;    return
.6:	jmp    *0		; else notify();
GLOBL ( _x86_Vertex3fv_end )


	
/**
 * Generic handlers for vector format data. 
 */

GLOBL( _x86_Attribute1fv)
	/* 'v' is already in eax */
	movl (%eax), %ecx       /* load v[0] */
	movl %ecx, 0      	/* store v[0] to current vertex */
	ret
GLOBL ( _x86_Attribute2fv_end )

GLOBL( _x86_Attribute2fv)
	/* 'v' is already in eax */
	movl (%eax), %ecx       /* load v[0] */
	movl 4(%eax), %eax      /* load v[1] */
	movl %ecx, 0      	/* store v[0] to current vertex */
	movl %eax, 4      	/* store v[1] to current vertex */
	ret
GLOBL ( _x86_Attribute2fv_end )


GLOBL( _x86_Attribute3fv)
	/* 'v' is already in eax */
	movl (%eax), %ecx       /* load v[0] */
	movl 4(%eax), %edx      /* load v[1] */
	movl 8(%eax), %eax      /* load v[2] */
	movl %ecx, 0      	/* store v[0] to current vertex */
	movl %edx, 4      	/* store v[1] to current vertex */
	movl %eax, 8      	/* store v[2] to current vertex */
	ret
GLOBL ( _x86_Attribute3fv_end )

GLOBL( _x86_Attribute4fv)
	/* 'v' is already in eax */
	movl (%eax), %ecx       /* load v[0] */
	movl 4(%eax), %edx      /* load v[1] */
	movl %ecx, 0      	/* store v[0] to current vertex */
	movl %edx, 4      	/* store v[1] to current vertex */
	movl 8(%eax), %ecx      /* load v[2] */
	movl 12(%eax), %edx      /* load v[3] */
	movl %ecx, 8      	/* store v[2] to current vertex */
	movl %edx, 12      	/* store v[3] to current vertex */
	ret
GLOBL ( _x86_Attribute3fv_end )


;;; In the 1st level dispatch functions, switch to a different
;;; calling convention -- (const GLfloat *v) in %eax.
;;; 
;;; As with regular (x86) dispatch, don't create a new stack frame -
;;; just let the 'ret' in the dispatched function return straight
;;; back to the original caller.


	
;;; Vertex/Normal/Color, etc: the address of the function pointer
;;; is known at codegen time.
	
GLOBL( _x86_dispatch_attrf )
	leal	4(%esp), %eax
	jmp	*foo
GLOBL( _x86_dispatch_attrf_end )

GLOBL( _x86_dispatch_attrfv )
	movl	4(%esp), %eax
	jmp	*foo
GLOBL( _x86_dispatch_attr1f_end )

;;; MultiTexcoord: the address of the function pointer must be
;;; calculated.
	
GLOBL( _x86_dispatch_multitexcoordf )
	leal	4(%esp), %eax
	jmp	*foo
GLOBL( _x86_dispatch_multitexcoordf_end )

GLOBL( _x86_dispatch_multitexcoordfv )
	movl	4(%esp), %eax
	jmp	*foo
GLOBL( _x86_dispatch_multitexcoordfv_end )
				
;;; VertexAttrib: the address of the function pointer must be
;;; calculated.

GLOBL( _x86_dispatch_vertexattribf )
	leal	4(%esp), %eax
	jmp	*foo
GLOBL( _x86_dispatch_vertexattribf_end )

GLOBL( _x86_dispatch_vertexattribfv )
	movl	4(%esp), %eax
	jmp	*foo
GLOBL( _x86_dispatch_vertexattribfv_end )
	
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

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#if defined(USE_X86_ASM) && !defined(HAVE_NONSTANDARD_GLAPIENTRY)
			
#define GLOBL( x )	\
.globl x;		\
x:

.data
.align 4

// Someone who knew a lot about this sort of thing would use this
// macro to note current offsets, etc in a special region of the
// object file & just make everything work out neat.  I don't know
// enough to do that...
	
#define SUBST( x ) (0x10101010 + x)	
	


GLOBL ( _x86_Vertex1fv )
	movl    4(%esp), %ecx	
	push	%edi
	push	%esi
	movl	SUBST(0), %edi	// 0x0 --> tnl->vtx.vbptr
	movl	(%ecx), %edx	// load v[0]
	movl	%edx, (%edi)	// tnl->vtx.vbptr[0] = v[0]
	addl	$4, %edi	// tnl->vtx.vbptr += 1
	movl	$SUBST(1), %ecx	// 0x1 --> (tnl->vtx.vertex_size - 1)
	movl	$SUBST(2), %esi	// 0x2 --> (tnl->vtx.vertex + 1)
	repz
	movsl   %ds:(%esi), %es:(%edi)
	movl	%edi, SUBST(0)	// 0x0 --> tnl->vtx.vbptr
	movl	SUBST(3), %edx	// 0x3 --> counter
	pop	%esi
	pop	%edi
	dec	%edx		// counter--
	movl	%edx, SUBST(3)	// 0x3 --> counter 
	.short	0x840f		// if (counter == 0)
	.long	SUBST(4)	//    notify()
	ret			// return
GLOBL ( _x86_Vertex1fv_end )


.align 4
GLOBL ( _x86_Vertex2fv )
	movl    4(%esp), %ecx	
	push	%edi
	push	%esi
	movl	SUBST(0), %edi	// load tnl->vtx.vbptr
	movl	(%ecx), %edx	// load v[0]
	movl	4(%ecx), %eax	// load v[1]
	movl	%edx, (%edi)	// tnl->vtx.vbptr[0] = v[0]
	movl	%eax, 4(%edi)	// tnl->vtx.vbptr[1] = v[1]
	addl	$8, %edi	// tnl->vtx.vbptr += 2
	movl	$SUBST(1), %ecx	// vertex_size - 2
	movl	$SUBST(2), %esi	// tnl->vtx.vertex + 2
	repz
	movsl %ds:(%esi), %es:(%edi)
	movl	%edi, SUBST(0)	// save tnl->vtx.vbptr
	movl	SUBST(3), %edx	// load counter
	pop	%esi
	pop	%edi
	dec	%edx		// counter--
	movl	%edx, SUBST(3)	// save counter
	.short	0x840f		// if (counter == 0)
	.long	SUBST(4)	//    notify()
	ret			// return
GLOBL ( _x86_Vertex2fv_end )

.align 4
GLOBL ( _x86_Vertex3fv )
	movl    4(%esp), %ecx	
	push	%edi
	push	%esi
	movl	SUBST(0), %edi	// load tnl->vtx.vbptr
	movl	(%ecx), %edx	// load v[0]
	movl	4(%ecx), %eax	// load v[1]
	movl	8(%ecx), %esi	// load v[2]
	movl	%edx, (%edi)	// tnl->vtx.vbptr[0] = v[0]
	movl	%eax, 4(%edi)	// tnl->vtx.vbptr[1] = v[1]
	movl	%esi, 8(%edi)	// tnl->vtx.vbptr[2] = v[2]
	addl	$12, %edi	// tnl->vtx.vbptr += 3
	movl	$SUBST(1), %ecx	// vertex_size - 3
	movl	$SUBST(2), %esi	// tnl->vtx.vertex + 3
	repz
	movsl %ds:(%esi), %es:(%edi)
	movl	%edi, SUBST(0)	// save tnl->vtx.vbptr
	movl	SUBST(3), %edx	// load counter
	pop	%esi
	pop	%edi
	dec	%edx		// counter--
	movl	%edx, SUBST(3)	// save counter
	.short	0x840f		// if (counter == 0)
	.long	SUBST(4)	//    notify()
	ret			// return
GLOBL ( _x86_Vertex3fv_end )

			
.align 4
GLOBL ( _x86_Vertex4fv )
	movl    4(%esp), %ecx	
	push	%edi
	push	%esi
	movl	SUBST(0), %edi	// load tnl->vtx.vbptr
	movl	(%ecx), %edx	// load v[0]
	movl	4(%ecx), %eax	// load v[1]
	movl	8(%ecx), %esi	// load v[2]
	movl	12(%ecx), %ecx	// load v[3]
	movl	%edx, (%edi)	// tnl->vtx.vbptr[0] = v[0]
	movl	%eax, 4(%edi)	// tnl->vtx.vbptr[1] = v[1]
	movl	%esi, 8(%edi)	// tnl->vtx.vbptr[2] = v[2]
	movl	%ecx, 12(%edi)	// tnl->vtx.vbptr[3] = v[3]
	addl	$16, %edi	// tnl->vtx.vbptr += 4
	movl	$SUBST(1), %ecx	// vertex_size - 4
	movl	$SUBST(2), %esi	// tnl->vtx.vertex + 3
	repz
	movsl   %ds:(%esi), %es:(%edi)
	movl	%edi, SUBST(0)	// save tnl->vtx.vbptr
	movl	SUBST(3), %edx	// load counter
	pop	%esi
	pop	%edi
	dec	%edx		// counter--
	movl	%edx, SUBST(3)	// save counter
	.short	0x840f		// if (counter == 0)
	.long	SUBST(4)	//    notify()
	ret			// return
GLOBL ( _x86_Vertex4fv_end )


	
/**
 * Generic handlers for vector format data. 
 */

GLOBL( _x86_Attribute1fv)
	movl 4(%esp), %ecx	
	movl (%ecx), %eax       /* load v[0] */
	movl %eax, SUBST(0)    	/* store v[0] to current vertex */
	ret
GLOBL ( _x86_Attribute1fv_end )

GLOBL( _x86_Attribute2fv)
	movl 4(%esp), %ecx	
	movl (%ecx), %eax       /* load v[0] */
	movl 4(%ecx), %edx      /* load v[1] */
	movl %eax, SUBST(0)    	/* store v[0] to current vertex */
	movl %edx, SUBST(1)    	/* store v[1] to current vertex */
	ret
GLOBL ( _x86_Attribute2fv_end )


GLOBL( _x86_Attribute3fv)
	movl 4(%esp), %ecx	
	movl (%ecx), %eax       /* load v[0] */
	movl 4(%ecx), %edx      /* load v[1] */
	movl 8(%ecx), %ecx      /* load v[2] */
	movl %eax, SUBST(0)    	/* store v[0] to current vertex */
	movl %edx, SUBST(1)    	/* store v[1] to current vertex */
	movl %ecx, SUBST(2)   	/* store v[2] to current vertex */
	ret
GLOBL ( _x86_Attribute3fv_end )

GLOBL( _x86_Attribute4fv)
	movl 4(%esp), %ecx	
	movl (%ecx), %eax       /* load v[0] */
	movl 4(%ecx), %edx      /* load v[1] */
	movl %eax, SUBST(0)    	/* store v[0] to current vertex */
	movl %edx, SUBST(1)    	/* store v[1] to current vertex */
	movl 8(%ecx), %eax      /* load v[2] */
	movl 12(%ecx), %edx     /* load v[3] */
	movl %eax, SUBST(2)    	/* store v[2] to current vertex */
	movl %edx, SUBST(3)    	/* store v[3] to current vertex */
	ret
GLOBL ( _x86_Attribute4fv_end )


// Choosers:

// Must generate all of these ahead of first usage.  Generate at
// compile-time?  
	
// NOT CURRENTLY USED


GLOBL( _x86_choose_fv)
	subl	$12, %esp	// gcc does 16 byte alignment of stack frames?
	movl	$SUBST(0), (%esp)	// arg 0 - attrib
	movl	$SUBST(1), 4(%esp)	// arg 1 - N
	call    _do_choose	// new function returned in %eax
	add     $12, %esp	// tear down stack frame
	jmp     *%eax		// jump to new func
GLOBL ( _x86_choosefv_end )
	
	


// FIRST LEVEL FUNCTIONS -- these are plugged directly into GL dispatch.
	

// NOT CURRENTLY USED

	
		
// In the 1st level dispatch functions, switch to a different
// calling convention -- (const GLfloat *v) in %ecx.
// 
// As with regular (x86) dispatch, don't create a new stack frame -
// just let the 'ret' in the dispatched function return straight
// back to the original caller.


	
// Vertex/Normal/Color, etc: the address of the function pointer
// is known at codegen time.


// Unfortunately, have to play with the stack in the non-fv case:
// 
GLOBL( _x86_dispatch_attrf )
	subl	$12, %esp	// gcc does 16 byte alignment of stack frames?
	leal	16(%esp), %edx	// address of first float on stack
	movl	%edx, (%esp)	// save as 'v'
	call	SUBST(0)	// 0x0 --> tabfv[attr][n]
	addl	$12, %esp	// tear down frame
	ret			// return
GLOBL( _x86_dispatch_attrf_end )

// The fv case is simpler:
// 
GLOBL( _x86_dispatch_attrfv )
	jmp	SUBST(0)	// 0x0 --> tabfv[attr][n]
GLOBL( _x86_dispatch_attrfv_end )


// MultiTexcoord: the address of the function pointer must be
// calculated, but can use the index argument slot to hold 'v', and
// avoid setting up a new stack frame.

// Also, will only need a maximum of four of each of these per context:
// 
GLOBL( _x86_dispatch_multitexcoordf )
	movl	4(%esp), %ecx
	leal	8(%esp), %edx
	andl	$7, %ecx
	movl	%edx, 4(%esp)
	sall	$4, %ecx
	jmp	*SUBST(0)(%ecx)	// 0x0 - tabfv[tex0][n]
GLOBL( _x86_dispatch_multitexcoordf_end )

GLOBL( _x86_dispatch_multitexcoordfv )
	movl	4(%esp), %ecx
	movl	8(%esp), %edx
	andl	$7, %ecx
	movl	%edx, 4(%esp)
	sall	$4, %ecx
	jmp	*SUBST(0)(%ecx)	// 0x0 - tabfv[tex0][n]
GLOBL( _x86_dispatch_multitexcoordfv_end )
				
// VertexAttrib: the address of the function pointer must be
// calculated.

GLOBL( _x86_dispatch_vertexattribf )
	movl	$16, %ecx
	movl	4(%esp), %eax
	cmpl	$16, %eax
	cmovge	%ecx, %eax
	leal	8(%esp), %ecx	// calculate 'v'
	movl	%ecx, 4(%esp)	// save in 1st arg slot
	sall	$4, %eax
	jmp	*SUBST(0)(%eax)	// 0x0 - tabfv[0][n]
GLOBL( _x86_dispatch_vertexattribf_end )

GLOBL( _x86_dispatch_vertexattribfv )
	movl	$16, %ecx
	movl	4(%esp), %eax
	cmpl	$16, %eax
	cmovge	%ecx, %eax
	movl	8(%esp), %ecx	// load 'v'
	movl	%ecx, 4(%esp)	// save in 1st arg slot
	sall	$4, %eax
	jmp	*SUBST(0)(%eax)	// 0x0 - tabfv[0][n]
GLOBL( _x86_dispatch_vertexattribfv_end )

#endif

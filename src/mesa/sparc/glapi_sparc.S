/* DO NOT EDIT - This file generated automatically with glsparcasm.py script */
#include "glapioffsets.h"

/* The _glapi_Dispatch symbol addresses get relocated into the
 * sethi/or instruction sequences below at library init time.
 */


.text
.align 32
.globl __glapi_sparc_icache_flush
__glapi_sparc_icache_flush: /* %o0 = insn_addr */
	flush	%o0
	retl
	nop

.data
.align 64

.globl _mesa_sparc_glapi_begin
.type _mesa_sparc_glapi_begin,#function
_mesa_sparc_glapi_begin:

.globl glNewList
.type glNewList,#function
glNewList:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_NewList), %g2
	or	%g2, %lo(8 * _gloffset_NewList), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_NewList)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glEndList
.type glEndList,#function
glEndList:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_EndList), %g2
	or	%g2, %lo(8 * _gloffset_EndList), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_EndList)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glCallList
.type glCallList,#function
glCallList:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_CallList), %g2
	or	%g2, %lo(8 * _gloffset_CallList), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_CallList)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glCallLists
.type glCallLists,#function
glCallLists:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_CallLists), %g2
	or	%g2, %lo(8 * _gloffset_CallLists), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_CallLists)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glDeleteLists
.type glDeleteLists,#function
glDeleteLists:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_DeleteLists), %g2
	or	%g2, %lo(8 * _gloffset_DeleteLists), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_DeleteLists)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGenLists
.type glGenLists,#function
glGenLists:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GenLists), %g2
	or	%g2, %lo(8 * _gloffset_GenLists), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GenLists)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glListBase
.type glListBase,#function
glListBase:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ListBase), %g2
	or	%g2, %lo(8 * _gloffset_ListBase), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ListBase)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glBegin
.type glBegin,#function
glBegin:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Begin), %g2
	or	%g2, %lo(8 * _gloffset_Begin), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Begin)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glBitmap
.type glBitmap,#function
glBitmap:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Bitmap), %g2
	or	%g2, %lo(8 * _gloffset_Bitmap), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Bitmap)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColor3b
.type glColor3b,#function
glColor3b:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Color3b), %g2
	or	%g2, %lo(8 * _gloffset_Color3b), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Color3b)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColor3bv
.type glColor3bv,#function
glColor3bv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Color3bv), %g2
	or	%g2, %lo(8 * _gloffset_Color3bv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Color3bv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColor3d
.type glColor3d,#function
glColor3d:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Color3d), %g2
	or	%g2, %lo(8 * _gloffset_Color3d), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Color3d)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColor3dv
.type glColor3dv,#function
glColor3dv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Color3dv), %g2
	or	%g2, %lo(8 * _gloffset_Color3dv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Color3dv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColor3f
.type glColor3f,#function
glColor3f:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Color3f), %g2
	or	%g2, %lo(8 * _gloffset_Color3f), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Color3f)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColor3fv
.type glColor3fv,#function
glColor3fv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Color3fv), %g2
	or	%g2, %lo(8 * _gloffset_Color3fv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Color3fv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColor3i
.type glColor3i,#function
glColor3i:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Color3i), %g2
	or	%g2, %lo(8 * _gloffset_Color3i), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Color3i)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColor3iv
.type glColor3iv,#function
glColor3iv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Color3iv), %g2
	or	%g2, %lo(8 * _gloffset_Color3iv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Color3iv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColor3s
.type glColor3s,#function
glColor3s:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Color3s), %g2
	or	%g2, %lo(8 * _gloffset_Color3s), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Color3s)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColor3sv
.type glColor3sv,#function
glColor3sv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Color3sv), %g2
	or	%g2, %lo(8 * _gloffset_Color3sv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Color3sv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColor3ub
.type glColor3ub,#function
glColor3ub:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Color3ub), %g2
	or	%g2, %lo(8 * _gloffset_Color3ub), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Color3ub)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColor3ubv
.type glColor3ubv,#function
glColor3ubv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Color3ubv), %g2
	or	%g2, %lo(8 * _gloffset_Color3ubv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Color3ubv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColor3ui
.type glColor3ui,#function
glColor3ui:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Color3ui), %g2
	or	%g2, %lo(8 * _gloffset_Color3ui), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Color3ui)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColor3uiv
.type glColor3uiv,#function
glColor3uiv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Color3uiv), %g2
	or	%g2, %lo(8 * _gloffset_Color3uiv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Color3uiv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColor3us
.type glColor3us,#function
glColor3us:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Color3us), %g2
	or	%g2, %lo(8 * _gloffset_Color3us), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Color3us)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColor3usv
.type glColor3usv,#function
glColor3usv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Color3usv), %g2
	or	%g2, %lo(8 * _gloffset_Color3usv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Color3usv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColor4b
.type glColor4b,#function
glColor4b:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Color4b), %g2
	or	%g2, %lo(8 * _gloffset_Color4b), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Color4b)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColor4bv
.type glColor4bv,#function
glColor4bv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Color4bv), %g2
	or	%g2, %lo(8 * _gloffset_Color4bv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Color4bv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColor4d
.type glColor4d,#function
glColor4d:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Color4d), %g2
	or	%g2, %lo(8 * _gloffset_Color4d), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Color4d)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColor4dv
.type glColor4dv,#function
glColor4dv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Color4dv), %g2
	or	%g2, %lo(8 * _gloffset_Color4dv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Color4dv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColor4f
.type glColor4f,#function
glColor4f:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Color4f), %g2
	or	%g2, %lo(8 * _gloffset_Color4f), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Color4f)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColor4fv
.type glColor4fv,#function
glColor4fv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Color4fv), %g2
	or	%g2, %lo(8 * _gloffset_Color4fv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Color4fv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColor4i
.type glColor4i,#function
glColor4i:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Color4i), %g2
	or	%g2, %lo(8 * _gloffset_Color4i), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Color4i)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColor4iv
.type glColor4iv,#function
glColor4iv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Color4iv), %g2
	or	%g2, %lo(8 * _gloffset_Color4iv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Color4iv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColor4s
.type glColor4s,#function
glColor4s:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Color4s), %g2
	or	%g2, %lo(8 * _gloffset_Color4s), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Color4s)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColor4sv
.type glColor4sv,#function
glColor4sv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Color4sv), %g2
	or	%g2, %lo(8 * _gloffset_Color4sv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Color4sv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColor4ub
.type glColor4ub,#function
glColor4ub:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Color4ub), %g2
	or	%g2, %lo(8 * _gloffset_Color4ub), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Color4ub)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColor4ubv
.type glColor4ubv,#function
glColor4ubv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Color4ubv), %g2
	or	%g2, %lo(8 * _gloffset_Color4ubv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Color4ubv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColor4ui
.type glColor4ui,#function
glColor4ui:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Color4ui), %g2
	or	%g2, %lo(8 * _gloffset_Color4ui), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Color4ui)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColor4uiv
.type glColor4uiv,#function
glColor4uiv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Color4uiv), %g2
	or	%g2, %lo(8 * _gloffset_Color4uiv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Color4uiv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColor4us
.type glColor4us,#function
glColor4us:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Color4us), %g2
	or	%g2, %lo(8 * _gloffset_Color4us), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Color4us)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColor4usv
.type glColor4usv,#function
glColor4usv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Color4usv), %g2
	or	%g2, %lo(8 * _gloffset_Color4usv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Color4usv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glEdgeFlag
.type glEdgeFlag,#function
glEdgeFlag:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_EdgeFlag), %g2
	or	%g2, %lo(8 * _gloffset_EdgeFlag), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_EdgeFlag)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glEdgeFlagv
.type glEdgeFlagv,#function
glEdgeFlagv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_EdgeFlagv), %g2
	or	%g2, %lo(8 * _gloffset_EdgeFlagv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_EdgeFlagv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glEnd
.type glEnd,#function
glEnd:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_End), %g2
	or	%g2, %lo(8 * _gloffset_End), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_End)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glIndexd
.type glIndexd,#function
glIndexd:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Indexd), %g2
	or	%g2, %lo(8 * _gloffset_Indexd), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Indexd)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glIndexdv
.type glIndexdv,#function
glIndexdv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Indexdv), %g2
	or	%g2, %lo(8 * _gloffset_Indexdv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Indexdv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glIndexf
.type glIndexf,#function
glIndexf:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Indexf), %g2
	or	%g2, %lo(8 * _gloffset_Indexf), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Indexf)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glIndexfv
.type glIndexfv,#function
glIndexfv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Indexfv), %g2
	or	%g2, %lo(8 * _gloffset_Indexfv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Indexfv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glIndexi
.type glIndexi,#function
glIndexi:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Indexi), %g2
	or	%g2, %lo(8 * _gloffset_Indexi), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Indexi)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glIndexiv
.type glIndexiv,#function
glIndexiv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Indexiv), %g2
	or	%g2, %lo(8 * _gloffset_Indexiv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Indexiv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glIndexs
.type glIndexs,#function
glIndexs:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Indexs), %g2
	or	%g2, %lo(8 * _gloffset_Indexs), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Indexs)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glIndexsv
.type glIndexsv,#function
glIndexsv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Indexsv), %g2
	or	%g2, %lo(8 * _gloffset_Indexsv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Indexsv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glNormal3b
.type glNormal3b,#function
glNormal3b:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Normal3b), %g2
	or	%g2, %lo(8 * _gloffset_Normal3b), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Normal3b)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glNormal3bv
.type glNormal3bv,#function
glNormal3bv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Normal3bv), %g2
	or	%g2, %lo(8 * _gloffset_Normal3bv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Normal3bv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glNormal3d
.type glNormal3d,#function
glNormal3d:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Normal3d), %g2
	or	%g2, %lo(8 * _gloffset_Normal3d), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Normal3d)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glNormal3dv
.type glNormal3dv,#function
glNormal3dv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Normal3dv), %g2
	or	%g2, %lo(8 * _gloffset_Normal3dv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Normal3dv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glNormal3f
.type glNormal3f,#function
glNormal3f:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Normal3f), %g2
	or	%g2, %lo(8 * _gloffset_Normal3f), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Normal3f)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glNormal3fv
.type glNormal3fv,#function
glNormal3fv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Normal3fv), %g2
	or	%g2, %lo(8 * _gloffset_Normal3fv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Normal3fv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glNormal3i
.type glNormal3i,#function
glNormal3i:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Normal3i), %g2
	or	%g2, %lo(8 * _gloffset_Normal3i), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Normal3i)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glNormal3iv
.type glNormal3iv,#function
glNormal3iv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Normal3iv), %g2
	or	%g2, %lo(8 * _gloffset_Normal3iv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Normal3iv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glNormal3s
.type glNormal3s,#function
glNormal3s:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Normal3s), %g2
	or	%g2, %lo(8 * _gloffset_Normal3s), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Normal3s)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glNormal3sv
.type glNormal3sv,#function
glNormal3sv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Normal3sv), %g2
	or	%g2, %lo(8 * _gloffset_Normal3sv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Normal3sv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glRasterPos2d
.type glRasterPos2d,#function
glRasterPos2d:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_RasterPos2d), %g2
	or	%g2, %lo(8 * _gloffset_RasterPos2d), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_RasterPos2d)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glRasterPos2dv
.type glRasterPos2dv,#function
glRasterPos2dv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_RasterPos2dv), %g2
	or	%g2, %lo(8 * _gloffset_RasterPos2dv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_RasterPos2dv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glRasterPos2f
.type glRasterPos2f,#function
glRasterPos2f:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_RasterPos2f), %g2
	or	%g2, %lo(8 * _gloffset_RasterPos2f), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_RasterPos2f)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glRasterPos2fv
.type glRasterPos2fv,#function
glRasterPos2fv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_RasterPos2fv), %g2
	or	%g2, %lo(8 * _gloffset_RasterPos2fv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_RasterPos2fv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glRasterPos2i
.type glRasterPos2i,#function
glRasterPos2i:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_RasterPos2i), %g2
	or	%g2, %lo(8 * _gloffset_RasterPos2i), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_RasterPos2i)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glRasterPos2iv
.type glRasterPos2iv,#function
glRasterPos2iv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_RasterPos2iv), %g2
	or	%g2, %lo(8 * _gloffset_RasterPos2iv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_RasterPos2iv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glRasterPos2s
.type glRasterPos2s,#function
glRasterPos2s:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_RasterPos2s), %g2
	or	%g2, %lo(8 * _gloffset_RasterPos2s), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_RasterPos2s)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glRasterPos2sv
.type glRasterPos2sv,#function
glRasterPos2sv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_RasterPos2sv), %g2
	or	%g2, %lo(8 * _gloffset_RasterPos2sv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_RasterPos2sv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glRasterPos3d
.type glRasterPos3d,#function
glRasterPos3d:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_RasterPos3d), %g2
	or	%g2, %lo(8 * _gloffset_RasterPos3d), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_RasterPos3d)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glRasterPos3dv
.type glRasterPos3dv,#function
glRasterPos3dv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_RasterPos3dv), %g2
	or	%g2, %lo(8 * _gloffset_RasterPos3dv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_RasterPos3dv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glRasterPos3f
.type glRasterPos3f,#function
glRasterPos3f:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_RasterPos3f), %g2
	or	%g2, %lo(8 * _gloffset_RasterPos3f), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_RasterPos3f)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glRasterPos3fv
.type glRasterPos3fv,#function
glRasterPos3fv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_RasterPos3fv), %g2
	or	%g2, %lo(8 * _gloffset_RasterPos3fv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_RasterPos3fv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glRasterPos3i
.type glRasterPos3i,#function
glRasterPos3i:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_RasterPos3i), %g2
	or	%g2, %lo(8 * _gloffset_RasterPos3i), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_RasterPos3i)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glRasterPos3iv
.type glRasterPos3iv,#function
glRasterPos3iv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_RasterPos3iv), %g2
	or	%g2, %lo(8 * _gloffset_RasterPos3iv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_RasterPos3iv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glRasterPos3s
.type glRasterPos3s,#function
glRasterPos3s:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_RasterPos3s), %g2
	or	%g2, %lo(8 * _gloffset_RasterPos3s), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_RasterPos3s)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glRasterPos3sv
.type glRasterPos3sv,#function
glRasterPos3sv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_RasterPos3sv), %g2
	or	%g2, %lo(8 * _gloffset_RasterPos3sv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_RasterPos3sv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glRasterPos4d
.type glRasterPos4d,#function
glRasterPos4d:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_RasterPos4d), %g2
	or	%g2, %lo(8 * _gloffset_RasterPos4d), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_RasterPos4d)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glRasterPos4dv
.type glRasterPos4dv,#function
glRasterPos4dv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_RasterPos4dv), %g2
	or	%g2, %lo(8 * _gloffset_RasterPos4dv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_RasterPos4dv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glRasterPos4f
.type glRasterPos4f,#function
glRasterPos4f:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_RasterPos4f), %g2
	or	%g2, %lo(8 * _gloffset_RasterPos4f), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_RasterPos4f)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glRasterPos4fv
.type glRasterPos4fv,#function
glRasterPos4fv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_RasterPos4fv), %g2
	or	%g2, %lo(8 * _gloffset_RasterPos4fv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_RasterPos4fv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glRasterPos4i
.type glRasterPos4i,#function
glRasterPos4i:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_RasterPos4i), %g2
	or	%g2, %lo(8 * _gloffset_RasterPos4i), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_RasterPos4i)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glRasterPos4iv
.type glRasterPos4iv,#function
glRasterPos4iv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_RasterPos4iv), %g2
	or	%g2, %lo(8 * _gloffset_RasterPos4iv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_RasterPos4iv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glRasterPos4s
.type glRasterPos4s,#function
glRasterPos4s:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_RasterPos4s), %g2
	or	%g2, %lo(8 * _gloffset_RasterPos4s), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_RasterPos4s)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glRasterPos4sv
.type glRasterPos4sv,#function
glRasterPos4sv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_RasterPos4sv), %g2
	or	%g2, %lo(8 * _gloffset_RasterPos4sv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_RasterPos4sv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glRectd
.type glRectd,#function
glRectd:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Rectd), %g2
	or	%g2, %lo(8 * _gloffset_Rectd), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Rectd)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glRectdv
.type glRectdv,#function
glRectdv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Rectdv), %g2
	or	%g2, %lo(8 * _gloffset_Rectdv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Rectdv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glRectf
.type glRectf,#function
glRectf:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Rectf), %g2
	or	%g2, %lo(8 * _gloffset_Rectf), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Rectf)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glRectfv
.type glRectfv,#function
glRectfv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Rectfv), %g2
	or	%g2, %lo(8 * _gloffset_Rectfv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Rectfv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glRecti
.type glRecti,#function
glRecti:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Recti), %g2
	or	%g2, %lo(8 * _gloffset_Recti), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Recti)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glRectiv
.type glRectiv,#function
glRectiv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Rectiv), %g2
	or	%g2, %lo(8 * _gloffset_Rectiv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Rectiv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glRects
.type glRects,#function
glRects:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Rects), %g2
	or	%g2, %lo(8 * _gloffset_Rects), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Rects)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glRectsv
.type glRectsv,#function
glRectsv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Rectsv), %g2
	or	%g2, %lo(8 * _gloffset_Rectsv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Rectsv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexCoord1d
.type glTexCoord1d,#function
glTexCoord1d:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexCoord1d), %g2
	or	%g2, %lo(8 * _gloffset_TexCoord1d), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexCoord1d)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexCoord1dv
.type glTexCoord1dv,#function
glTexCoord1dv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexCoord1dv), %g2
	or	%g2, %lo(8 * _gloffset_TexCoord1dv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexCoord1dv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexCoord1f
.type glTexCoord1f,#function
glTexCoord1f:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexCoord1f), %g2
	or	%g2, %lo(8 * _gloffset_TexCoord1f), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexCoord1f)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexCoord1fv
.type glTexCoord1fv,#function
glTexCoord1fv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexCoord1fv), %g2
	or	%g2, %lo(8 * _gloffset_TexCoord1fv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexCoord1fv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexCoord1i
.type glTexCoord1i,#function
glTexCoord1i:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexCoord1i), %g2
	or	%g2, %lo(8 * _gloffset_TexCoord1i), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexCoord1i)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexCoord1iv
.type glTexCoord1iv,#function
glTexCoord1iv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexCoord1iv), %g2
	or	%g2, %lo(8 * _gloffset_TexCoord1iv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexCoord1iv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexCoord1s
.type glTexCoord1s,#function
glTexCoord1s:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexCoord1s), %g2
	or	%g2, %lo(8 * _gloffset_TexCoord1s), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexCoord1s)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexCoord1sv
.type glTexCoord1sv,#function
glTexCoord1sv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexCoord1sv), %g2
	or	%g2, %lo(8 * _gloffset_TexCoord1sv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexCoord1sv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexCoord2d
.type glTexCoord2d,#function
glTexCoord2d:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexCoord2d), %g2
	or	%g2, %lo(8 * _gloffset_TexCoord2d), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexCoord2d)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexCoord2dv
.type glTexCoord2dv,#function
glTexCoord2dv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexCoord2dv), %g2
	or	%g2, %lo(8 * _gloffset_TexCoord2dv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexCoord2dv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexCoord2f
.type glTexCoord2f,#function
glTexCoord2f:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexCoord2f), %g2
	or	%g2, %lo(8 * _gloffset_TexCoord2f), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexCoord2f)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexCoord2fv
.type glTexCoord2fv,#function
glTexCoord2fv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexCoord2fv), %g2
	or	%g2, %lo(8 * _gloffset_TexCoord2fv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexCoord2fv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexCoord2i
.type glTexCoord2i,#function
glTexCoord2i:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexCoord2i), %g2
	or	%g2, %lo(8 * _gloffset_TexCoord2i), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexCoord2i)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexCoord2iv
.type glTexCoord2iv,#function
glTexCoord2iv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexCoord2iv), %g2
	or	%g2, %lo(8 * _gloffset_TexCoord2iv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexCoord2iv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexCoord2s
.type glTexCoord2s,#function
glTexCoord2s:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexCoord2s), %g2
	or	%g2, %lo(8 * _gloffset_TexCoord2s), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexCoord2s)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexCoord2sv
.type glTexCoord2sv,#function
glTexCoord2sv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexCoord2sv), %g2
	or	%g2, %lo(8 * _gloffset_TexCoord2sv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexCoord2sv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexCoord3d
.type glTexCoord3d,#function
glTexCoord3d:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexCoord3d), %g2
	or	%g2, %lo(8 * _gloffset_TexCoord3d), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexCoord3d)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexCoord3dv
.type glTexCoord3dv,#function
glTexCoord3dv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexCoord3dv), %g2
	or	%g2, %lo(8 * _gloffset_TexCoord3dv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexCoord3dv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexCoord3f
.type glTexCoord3f,#function
glTexCoord3f:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexCoord3f), %g2
	or	%g2, %lo(8 * _gloffset_TexCoord3f), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexCoord3f)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexCoord3fv
.type glTexCoord3fv,#function
glTexCoord3fv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexCoord3fv), %g2
	or	%g2, %lo(8 * _gloffset_TexCoord3fv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexCoord3fv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexCoord3i
.type glTexCoord3i,#function
glTexCoord3i:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexCoord3i), %g2
	or	%g2, %lo(8 * _gloffset_TexCoord3i), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexCoord3i)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexCoord3iv
.type glTexCoord3iv,#function
glTexCoord3iv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexCoord3iv), %g2
	or	%g2, %lo(8 * _gloffset_TexCoord3iv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexCoord3iv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexCoord3s
.type glTexCoord3s,#function
glTexCoord3s:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexCoord3s), %g2
	or	%g2, %lo(8 * _gloffset_TexCoord3s), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexCoord3s)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexCoord3sv
.type glTexCoord3sv,#function
glTexCoord3sv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexCoord3sv), %g2
	or	%g2, %lo(8 * _gloffset_TexCoord3sv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexCoord3sv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexCoord4d
.type glTexCoord4d,#function
glTexCoord4d:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexCoord4d), %g2
	or	%g2, %lo(8 * _gloffset_TexCoord4d), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexCoord4d)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexCoord4dv
.type glTexCoord4dv,#function
glTexCoord4dv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexCoord4dv), %g2
	or	%g2, %lo(8 * _gloffset_TexCoord4dv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexCoord4dv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexCoord4f
.type glTexCoord4f,#function
glTexCoord4f:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexCoord4f), %g2
	or	%g2, %lo(8 * _gloffset_TexCoord4f), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexCoord4f)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexCoord4fv
.type glTexCoord4fv,#function
glTexCoord4fv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexCoord4fv), %g2
	or	%g2, %lo(8 * _gloffset_TexCoord4fv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexCoord4fv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexCoord4i
.type glTexCoord4i,#function
glTexCoord4i:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexCoord4i), %g2
	or	%g2, %lo(8 * _gloffset_TexCoord4i), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexCoord4i)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexCoord4iv
.type glTexCoord4iv,#function
glTexCoord4iv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexCoord4iv), %g2
	or	%g2, %lo(8 * _gloffset_TexCoord4iv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexCoord4iv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexCoord4s
.type glTexCoord4s,#function
glTexCoord4s:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexCoord4s), %g2
	or	%g2, %lo(8 * _gloffset_TexCoord4s), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexCoord4s)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexCoord4sv
.type glTexCoord4sv,#function
glTexCoord4sv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexCoord4sv), %g2
	or	%g2, %lo(8 * _gloffset_TexCoord4sv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexCoord4sv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertex2d
.type glVertex2d,#function
glVertex2d:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Vertex2d), %g2
	or	%g2, %lo(8 * _gloffset_Vertex2d), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Vertex2d)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertex2dv
.type glVertex2dv,#function
glVertex2dv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Vertex2dv), %g2
	or	%g2, %lo(8 * _gloffset_Vertex2dv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Vertex2dv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertex2f
.type glVertex2f,#function
glVertex2f:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Vertex2f), %g2
	or	%g2, %lo(8 * _gloffset_Vertex2f), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Vertex2f)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertex2fv
.type glVertex2fv,#function
glVertex2fv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Vertex2fv), %g2
	or	%g2, %lo(8 * _gloffset_Vertex2fv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Vertex2fv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertex2i
.type glVertex2i,#function
glVertex2i:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Vertex2i), %g2
	or	%g2, %lo(8 * _gloffset_Vertex2i), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Vertex2i)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertex2iv
.type glVertex2iv,#function
glVertex2iv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Vertex2iv), %g2
	or	%g2, %lo(8 * _gloffset_Vertex2iv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Vertex2iv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertex2s
.type glVertex2s,#function
glVertex2s:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Vertex2s), %g2
	or	%g2, %lo(8 * _gloffset_Vertex2s), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Vertex2s)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertex2sv
.type glVertex2sv,#function
glVertex2sv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Vertex2sv), %g2
	or	%g2, %lo(8 * _gloffset_Vertex2sv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Vertex2sv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertex3d
.type glVertex3d,#function
glVertex3d:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Vertex3d), %g2
	or	%g2, %lo(8 * _gloffset_Vertex3d), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Vertex3d)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertex3dv
.type glVertex3dv,#function
glVertex3dv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Vertex3dv), %g2
	or	%g2, %lo(8 * _gloffset_Vertex3dv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Vertex3dv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertex3f
.type glVertex3f,#function
glVertex3f:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Vertex3f), %g2
	or	%g2, %lo(8 * _gloffset_Vertex3f), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Vertex3f)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertex3fv
.type glVertex3fv,#function
glVertex3fv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Vertex3fv), %g2
	or	%g2, %lo(8 * _gloffset_Vertex3fv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Vertex3fv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertex3i
.type glVertex3i,#function
glVertex3i:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Vertex3i), %g2
	or	%g2, %lo(8 * _gloffset_Vertex3i), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Vertex3i)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertex3iv
.type glVertex3iv,#function
glVertex3iv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Vertex3iv), %g2
	or	%g2, %lo(8 * _gloffset_Vertex3iv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Vertex3iv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertex3s
.type glVertex3s,#function
glVertex3s:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Vertex3s), %g2
	or	%g2, %lo(8 * _gloffset_Vertex3s), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Vertex3s)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertex3sv
.type glVertex3sv,#function
glVertex3sv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Vertex3sv), %g2
	or	%g2, %lo(8 * _gloffset_Vertex3sv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Vertex3sv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertex4d
.type glVertex4d,#function
glVertex4d:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Vertex4d), %g2
	or	%g2, %lo(8 * _gloffset_Vertex4d), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Vertex4d)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertex4dv
.type glVertex4dv,#function
glVertex4dv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Vertex4dv), %g2
	or	%g2, %lo(8 * _gloffset_Vertex4dv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Vertex4dv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertex4f
.type glVertex4f,#function
glVertex4f:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Vertex4f), %g2
	or	%g2, %lo(8 * _gloffset_Vertex4f), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Vertex4f)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertex4fv
.type glVertex4fv,#function
glVertex4fv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Vertex4fv), %g2
	or	%g2, %lo(8 * _gloffset_Vertex4fv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Vertex4fv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertex4i
.type glVertex4i,#function
glVertex4i:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Vertex4i), %g2
	or	%g2, %lo(8 * _gloffset_Vertex4i), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Vertex4i)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertex4iv
.type glVertex4iv,#function
glVertex4iv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Vertex4iv), %g2
	or	%g2, %lo(8 * _gloffset_Vertex4iv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Vertex4iv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertex4s
.type glVertex4s,#function
glVertex4s:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Vertex4s), %g2
	or	%g2, %lo(8 * _gloffset_Vertex4s), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Vertex4s)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertex4sv
.type glVertex4sv,#function
glVertex4sv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Vertex4sv), %g2
	or	%g2, %lo(8 * _gloffset_Vertex4sv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Vertex4sv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glClipPlane
.type glClipPlane,#function
glClipPlane:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ClipPlane), %g2
	or	%g2, %lo(8 * _gloffset_ClipPlane), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ClipPlane)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColorMaterial
.type glColorMaterial,#function
glColorMaterial:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ColorMaterial), %g2
	or	%g2, %lo(8 * _gloffset_ColorMaterial), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ColorMaterial)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glCullFace
.type glCullFace,#function
glCullFace:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_CullFace), %g2
	or	%g2, %lo(8 * _gloffset_CullFace), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_CullFace)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glFogf
.type glFogf,#function
glFogf:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Fogf), %g2
	or	%g2, %lo(8 * _gloffset_Fogf), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Fogf)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glFogfv
.type glFogfv,#function
glFogfv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Fogfv), %g2
	or	%g2, %lo(8 * _gloffset_Fogfv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Fogfv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glFogi
.type glFogi,#function
glFogi:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Fogi), %g2
	or	%g2, %lo(8 * _gloffset_Fogi), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Fogi)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glFogiv
.type glFogiv,#function
glFogiv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Fogiv), %g2
	or	%g2, %lo(8 * _gloffset_Fogiv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Fogiv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glFrontFace
.type glFrontFace,#function
glFrontFace:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_FrontFace), %g2
	or	%g2, %lo(8 * _gloffset_FrontFace), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_FrontFace)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glHint
.type glHint,#function
glHint:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Hint), %g2
	or	%g2, %lo(8 * _gloffset_Hint), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Hint)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glLightf
.type glLightf,#function
glLightf:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Lightf), %g2
	or	%g2, %lo(8 * _gloffset_Lightf), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Lightf)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glLightfv
.type glLightfv,#function
glLightfv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Lightfv), %g2
	or	%g2, %lo(8 * _gloffset_Lightfv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Lightfv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glLighti
.type glLighti,#function
glLighti:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Lighti), %g2
	or	%g2, %lo(8 * _gloffset_Lighti), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Lighti)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glLightiv
.type glLightiv,#function
glLightiv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Lightiv), %g2
	or	%g2, %lo(8 * _gloffset_Lightiv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Lightiv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glLightModelf
.type glLightModelf,#function
glLightModelf:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_LightModelf), %g2
	or	%g2, %lo(8 * _gloffset_LightModelf), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_LightModelf)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glLightModelfv
.type glLightModelfv,#function
glLightModelfv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_LightModelfv), %g2
	or	%g2, %lo(8 * _gloffset_LightModelfv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_LightModelfv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glLightModeli
.type glLightModeli,#function
glLightModeli:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_LightModeli), %g2
	or	%g2, %lo(8 * _gloffset_LightModeli), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_LightModeli)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glLightModeliv
.type glLightModeliv,#function
glLightModeliv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_LightModeliv), %g2
	or	%g2, %lo(8 * _gloffset_LightModeliv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_LightModeliv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glLineStipple
.type glLineStipple,#function
glLineStipple:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_LineStipple), %g2
	or	%g2, %lo(8 * _gloffset_LineStipple), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_LineStipple)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glLineWidth
.type glLineWidth,#function
glLineWidth:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_LineWidth), %g2
	or	%g2, %lo(8 * _gloffset_LineWidth), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_LineWidth)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMaterialf
.type glMaterialf,#function
glMaterialf:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Materialf), %g2
	or	%g2, %lo(8 * _gloffset_Materialf), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Materialf)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMaterialfv
.type glMaterialfv,#function
glMaterialfv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Materialfv), %g2
	or	%g2, %lo(8 * _gloffset_Materialfv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Materialfv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMateriali
.type glMateriali,#function
glMateriali:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Materiali), %g2
	or	%g2, %lo(8 * _gloffset_Materiali), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Materiali)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMaterialiv
.type glMaterialiv,#function
glMaterialiv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Materialiv), %g2
	or	%g2, %lo(8 * _gloffset_Materialiv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Materialiv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glPointSize
.type glPointSize,#function
glPointSize:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_PointSize), %g2
	or	%g2, %lo(8 * _gloffset_PointSize), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_PointSize)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glPolygonMode
.type glPolygonMode,#function
glPolygonMode:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_PolygonMode), %g2
	or	%g2, %lo(8 * _gloffset_PolygonMode), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_PolygonMode)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glPolygonStipple
.type glPolygonStipple,#function
glPolygonStipple:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_PolygonStipple), %g2
	or	%g2, %lo(8 * _gloffset_PolygonStipple), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_PolygonStipple)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glScissor
.type glScissor,#function
glScissor:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Scissor), %g2
	or	%g2, %lo(8 * _gloffset_Scissor), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Scissor)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glShadeModel
.type glShadeModel,#function
glShadeModel:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ShadeModel), %g2
	or	%g2, %lo(8 * _gloffset_ShadeModel), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ShadeModel)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexParameterf
.type glTexParameterf,#function
glTexParameterf:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexParameterf), %g2
	or	%g2, %lo(8 * _gloffset_TexParameterf), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexParameterf)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexParameterfv
.type glTexParameterfv,#function
glTexParameterfv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexParameterfv), %g2
	or	%g2, %lo(8 * _gloffset_TexParameterfv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexParameterfv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexParameteri
.type glTexParameteri,#function
glTexParameteri:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexParameteri), %g2
	or	%g2, %lo(8 * _gloffset_TexParameteri), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexParameteri)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexParameteriv
.type glTexParameteriv,#function
glTexParameteriv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexParameteriv), %g2
	or	%g2, %lo(8 * _gloffset_TexParameteriv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexParameteriv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexImage1D
.type glTexImage1D,#function
glTexImage1D:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexImage1D), %g2
	or	%g2, %lo(8 * _gloffset_TexImage1D), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexImage1D)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexImage2D
.type glTexImage2D,#function
glTexImage2D:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexImage2D), %g2
	or	%g2, %lo(8 * _gloffset_TexImage2D), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexImage2D)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexEnvf
.type glTexEnvf,#function
glTexEnvf:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexEnvf), %g2
	or	%g2, %lo(8 * _gloffset_TexEnvf), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexEnvf)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexEnvfv
.type glTexEnvfv,#function
glTexEnvfv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexEnvfv), %g2
	or	%g2, %lo(8 * _gloffset_TexEnvfv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexEnvfv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexEnvi
.type glTexEnvi,#function
glTexEnvi:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexEnvi), %g2
	or	%g2, %lo(8 * _gloffset_TexEnvi), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexEnvi)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexEnviv
.type glTexEnviv,#function
glTexEnviv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexEnviv), %g2
	or	%g2, %lo(8 * _gloffset_TexEnviv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexEnviv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexGend
.type glTexGend,#function
glTexGend:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexGend), %g2
	or	%g2, %lo(8 * _gloffset_TexGend), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexGend)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexGendv
.type glTexGendv,#function
glTexGendv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexGendv), %g2
	or	%g2, %lo(8 * _gloffset_TexGendv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexGendv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexGenf
.type glTexGenf,#function
glTexGenf:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexGenf), %g2
	or	%g2, %lo(8 * _gloffset_TexGenf), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexGenf)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexGenfv
.type glTexGenfv,#function
glTexGenfv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexGenfv), %g2
	or	%g2, %lo(8 * _gloffset_TexGenfv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexGenfv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexGeni
.type glTexGeni,#function
glTexGeni:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexGeni), %g2
	or	%g2, %lo(8 * _gloffset_TexGeni), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexGeni)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexGeniv
.type glTexGeniv,#function
glTexGeniv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexGeniv), %g2
	or	%g2, %lo(8 * _gloffset_TexGeniv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexGeniv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glFeedbackBuffer
.type glFeedbackBuffer,#function
glFeedbackBuffer:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_FeedbackBuffer), %g2
	or	%g2, %lo(8 * _gloffset_FeedbackBuffer), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_FeedbackBuffer)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSelectBuffer
.type glSelectBuffer,#function
glSelectBuffer:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SelectBuffer), %g2
	or	%g2, %lo(8 * _gloffset_SelectBuffer), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SelectBuffer)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glRenderMode
.type glRenderMode,#function
glRenderMode:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_RenderMode), %g2
	or	%g2, %lo(8 * _gloffset_RenderMode), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_RenderMode)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glInitNames
.type glInitNames,#function
glInitNames:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_InitNames), %g2
	or	%g2, %lo(8 * _gloffset_InitNames), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_InitNames)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glLoadName
.type glLoadName,#function
glLoadName:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_LoadName), %g2
	or	%g2, %lo(8 * _gloffset_LoadName), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_LoadName)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glPassThrough
.type glPassThrough,#function
glPassThrough:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_PassThrough), %g2
	or	%g2, %lo(8 * _gloffset_PassThrough), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_PassThrough)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glPopName
.type glPopName,#function
glPopName:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_PopName), %g2
	or	%g2, %lo(8 * _gloffset_PopName), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_PopName)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glPushName
.type glPushName,#function
glPushName:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_PushName), %g2
	or	%g2, %lo(8 * _gloffset_PushName), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_PushName)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glDrawBuffer
.type glDrawBuffer,#function
glDrawBuffer:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_DrawBuffer), %g2
	or	%g2, %lo(8 * _gloffset_DrawBuffer), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_DrawBuffer)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glClear
.type glClear,#function
glClear:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Clear), %g2
	or	%g2, %lo(8 * _gloffset_Clear), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Clear)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glClearAccum
.type glClearAccum,#function
glClearAccum:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ClearAccum), %g2
	or	%g2, %lo(8 * _gloffset_ClearAccum), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ClearAccum)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glClearIndex
.type glClearIndex,#function
glClearIndex:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ClearIndex), %g2
	or	%g2, %lo(8 * _gloffset_ClearIndex), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ClearIndex)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glClearColor
.type glClearColor,#function
glClearColor:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ClearColor), %g2
	or	%g2, %lo(8 * _gloffset_ClearColor), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ClearColor)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glClearStencil
.type glClearStencil,#function
glClearStencil:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ClearStencil), %g2
	or	%g2, %lo(8 * _gloffset_ClearStencil), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ClearStencil)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glClearDepth
.type glClearDepth,#function
glClearDepth:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ClearDepth), %g2
	or	%g2, %lo(8 * _gloffset_ClearDepth), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ClearDepth)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glStencilMask
.type glStencilMask,#function
glStencilMask:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_StencilMask), %g2
	or	%g2, %lo(8 * _gloffset_StencilMask), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_StencilMask)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColorMask
.type glColorMask,#function
glColorMask:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ColorMask), %g2
	or	%g2, %lo(8 * _gloffset_ColorMask), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ColorMask)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glDepthMask
.type glDepthMask,#function
glDepthMask:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_DepthMask), %g2
	or	%g2, %lo(8 * _gloffset_DepthMask), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_DepthMask)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glIndexMask
.type glIndexMask,#function
glIndexMask:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_IndexMask), %g2
	or	%g2, %lo(8 * _gloffset_IndexMask), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_IndexMask)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glAccum
.type glAccum,#function
glAccum:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Accum), %g2
	or	%g2, %lo(8 * _gloffset_Accum), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Accum)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glDisable
.type glDisable,#function
glDisable:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Disable), %g2
	or	%g2, %lo(8 * _gloffset_Disable), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Disable)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glEnable
.type glEnable,#function
glEnable:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Enable), %g2
	or	%g2, %lo(8 * _gloffset_Enable), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Enable)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glFinish
.type glFinish,#function
glFinish:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Finish), %g2
	or	%g2, %lo(8 * _gloffset_Finish), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Finish)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glFlush
.type glFlush,#function
glFlush:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Flush), %g2
	or	%g2, %lo(8 * _gloffset_Flush), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Flush)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glPopAttrib
.type glPopAttrib,#function
glPopAttrib:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_PopAttrib), %g2
	or	%g2, %lo(8 * _gloffset_PopAttrib), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_PopAttrib)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glPushAttrib
.type glPushAttrib,#function
glPushAttrib:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_PushAttrib), %g2
	or	%g2, %lo(8 * _gloffset_PushAttrib), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_PushAttrib)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMap1d
.type glMap1d,#function
glMap1d:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Map1d), %g2
	or	%g2, %lo(8 * _gloffset_Map1d), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Map1d)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMap1f
.type glMap1f,#function
glMap1f:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Map1f), %g2
	or	%g2, %lo(8 * _gloffset_Map1f), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Map1f)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMap2d
.type glMap2d,#function
glMap2d:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Map2d), %g2
	or	%g2, %lo(8 * _gloffset_Map2d), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Map2d)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMap2f
.type glMap2f,#function
glMap2f:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Map2f), %g2
	or	%g2, %lo(8 * _gloffset_Map2f), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Map2f)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMapGrid1d
.type glMapGrid1d,#function
glMapGrid1d:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MapGrid1d), %g2
	or	%g2, %lo(8 * _gloffset_MapGrid1d), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MapGrid1d)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMapGrid1f
.type glMapGrid1f,#function
glMapGrid1f:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MapGrid1f), %g2
	or	%g2, %lo(8 * _gloffset_MapGrid1f), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MapGrid1f)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMapGrid2d
.type glMapGrid2d,#function
glMapGrid2d:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MapGrid2d), %g2
	or	%g2, %lo(8 * _gloffset_MapGrid2d), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MapGrid2d)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMapGrid2f
.type glMapGrid2f,#function
glMapGrid2f:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MapGrid2f), %g2
	or	%g2, %lo(8 * _gloffset_MapGrid2f), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MapGrid2f)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glEvalCoord1d
.type glEvalCoord1d,#function
glEvalCoord1d:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_EvalCoord1d), %g2
	or	%g2, %lo(8 * _gloffset_EvalCoord1d), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_EvalCoord1d)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glEvalCoord1dv
.type glEvalCoord1dv,#function
glEvalCoord1dv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_EvalCoord1dv), %g2
	or	%g2, %lo(8 * _gloffset_EvalCoord1dv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_EvalCoord1dv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glEvalCoord1f
.type glEvalCoord1f,#function
glEvalCoord1f:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_EvalCoord1f), %g2
	or	%g2, %lo(8 * _gloffset_EvalCoord1f), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_EvalCoord1f)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glEvalCoord1fv
.type glEvalCoord1fv,#function
glEvalCoord1fv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_EvalCoord1fv), %g2
	or	%g2, %lo(8 * _gloffset_EvalCoord1fv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_EvalCoord1fv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glEvalCoord2d
.type glEvalCoord2d,#function
glEvalCoord2d:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_EvalCoord2d), %g2
	or	%g2, %lo(8 * _gloffset_EvalCoord2d), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_EvalCoord2d)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glEvalCoord2dv
.type glEvalCoord2dv,#function
glEvalCoord2dv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_EvalCoord2dv), %g2
	or	%g2, %lo(8 * _gloffset_EvalCoord2dv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_EvalCoord2dv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glEvalCoord2f
.type glEvalCoord2f,#function
glEvalCoord2f:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_EvalCoord2f), %g2
	or	%g2, %lo(8 * _gloffset_EvalCoord2f), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_EvalCoord2f)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glEvalCoord2fv
.type glEvalCoord2fv,#function
glEvalCoord2fv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_EvalCoord2fv), %g2
	or	%g2, %lo(8 * _gloffset_EvalCoord2fv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_EvalCoord2fv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glEvalMesh1
.type glEvalMesh1,#function
glEvalMesh1:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_EvalMesh1), %g2
	or	%g2, %lo(8 * _gloffset_EvalMesh1), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_EvalMesh1)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glEvalPoint1
.type glEvalPoint1,#function
glEvalPoint1:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_EvalPoint1), %g2
	or	%g2, %lo(8 * _gloffset_EvalPoint1), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_EvalPoint1)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glEvalMesh2
.type glEvalMesh2,#function
glEvalMesh2:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_EvalMesh2), %g2
	or	%g2, %lo(8 * _gloffset_EvalMesh2), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_EvalMesh2)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glEvalPoint2
.type glEvalPoint2,#function
glEvalPoint2:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_EvalPoint2), %g2
	or	%g2, %lo(8 * _gloffset_EvalPoint2), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_EvalPoint2)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glAlphaFunc
.type glAlphaFunc,#function
glAlphaFunc:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_AlphaFunc), %g2
	or	%g2, %lo(8 * _gloffset_AlphaFunc), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_AlphaFunc)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glBlendFunc
.type glBlendFunc,#function
glBlendFunc:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_BlendFunc), %g2
	or	%g2, %lo(8 * _gloffset_BlendFunc), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_BlendFunc)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glLogicOp
.type glLogicOp,#function
glLogicOp:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_LogicOp), %g2
	or	%g2, %lo(8 * _gloffset_LogicOp), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_LogicOp)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glStencilFunc
.type glStencilFunc,#function
glStencilFunc:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_StencilFunc), %g2
	or	%g2, %lo(8 * _gloffset_StencilFunc), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_StencilFunc)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glStencilOp
.type glStencilOp,#function
glStencilOp:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_StencilOp), %g2
	or	%g2, %lo(8 * _gloffset_StencilOp), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_StencilOp)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glDepthFunc
.type glDepthFunc,#function
glDepthFunc:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_DepthFunc), %g2
	or	%g2, %lo(8 * _gloffset_DepthFunc), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_DepthFunc)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glPixelZoom
.type glPixelZoom,#function
glPixelZoom:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_PixelZoom), %g2
	or	%g2, %lo(8 * _gloffset_PixelZoom), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_PixelZoom)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glPixelTransferf
.type glPixelTransferf,#function
glPixelTransferf:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_PixelTransferf), %g2
	or	%g2, %lo(8 * _gloffset_PixelTransferf), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_PixelTransferf)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glPixelTransferi
.type glPixelTransferi,#function
glPixelTransferi:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_PixelTransferi), %g2
	or	%g2, %lo(8 * _gloffset_PixelTransferi), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_PixelTransferi)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glPixelStoref
.type glPixelStoref,#function
glPixelStoref:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_PixelStoref), %g2
	or	%g2, %lo(8 * _gloffset_PixelStoref), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_PixelStoref)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glPixelStorei
.type glPixelStorei,#function
glPixelStorei:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_PixelStorei), %g2
	or	%g2, %lo(8 * _gloffset_PixelStorei), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_PixelStorei)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glPixelMapfv
.type glPixelMapfv,#function
glPixelMapfv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_PixelMapfv), %g2
	or	%g2, %lo(8 * _gloffset_PixelMapfv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_PixelMapfv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glPixelMapuiv
.type glPixelMapuiv,#function
glPixelMapuiv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_PixelMapuiv), %g2
	or	%g2, %lo(8 * _gloffset_PixelMapuiv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_PixelMapuiv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glPixelMapusv
.type glPixelMapusv,#function
glPixelMapusv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_PixelMapusv), %g2
	or	%g2, %lo(8 * _gloffset_PixelMapusv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_PixelMapusv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glReadBuffer
.type glReadBuffer,#function
glReadBuffer:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ReadBuffer), %g2
	or	%g2, %lo(8 * _gloffset_ReadBuffer), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ReadBuffer)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glCopyPixels
.type glCopyPixels,#function
glCopyPixels:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_CopyPixels), %g2
	or	%g2, %lo(8 * _gloffset_CopyPixels), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_CopyPixels)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glReadPixels
.type glReadPixels,#function
glReadPixels:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ReadPixels), %g2
	or	%g2, %lo(8 * _gloffset_ReadPixels), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ReadPixels)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glDrawPixels
.type glDrawPixels,#function
glDrawPixels:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_DrawPixels), %g2
	or	%g2, %lo(8 * _gloffset_DrawPixels), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_DrawPixels)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetBooleanv
.type glGetBooleanv,#function
glGetBooleanv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetBooleanv), %g2
	or	%g2, %lo(8 * _gloffset_GetBooleanv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetBooleanv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetClipPlane
.type glGetClipPlane,#function
glGetClipPlane:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetClipPlane), %g2
	or	%g2, %lo(8 * _gloffset_GetClipPlane), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetClipPlane)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetDoublev
.type glGetDoublev,#function
glGetDoublev:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetDoublev), %g2
	or	%g2, %lo(8 * _gloffset_GetDoublev), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetDoublev)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetError
.type glGetError,#function
glGetError:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetError), %g2
	or	%g2, %lo(8 * _gloffset_GetError), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetError)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetFloatv
.type glGetFloatv,#function
glGetFloatv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetFloatv), %g2
	or	%g2, %lo(8 * _gloffset_GetFloatv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetFloatv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetIntegerv
.type glGetIntegerv,#function
glGetIntegerv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetIntegerv), %g2
	or	%g2, %lo(8 * _gloffset_GetIntegerv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetIntegerv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetLightfv
.type glGetLightfv,#function
glGetLightfv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetLightfv), %g2
	or	%g2, %lo(8 * _gloffset_GetLightfv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetLightfv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetLightiv
.type glGetLightiv,#function
glGetLightiv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetLightiv), %g2
	or	%g2, %lo(8 * _gloffset_GetLightiv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetLightiv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetMapdv
.type glGetMapdv,#function
glGetMapdv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetMapdv), %g2
	or	%g2, %lo(8 * _gloffset_GetMapdv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetMapdv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetMapfv
.type glGetMapfv,#function
glGetMapfv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetMapfv), %g2
	or	%g2, %lo(8 * _gloffset_GetMapfv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetMapfv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetMapiv
.type glGetMapiv,#function
glGetMapiv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetMapiv), %g2
	or	%g2, %lo(8 * _gloffset_GetMapiv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetMapiv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetMaterialfv
.type glGetMaterialfv,#function
glGetMaterialfv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetMaterialfv), %g2
	or	%g2, %lo(8 * _gloffset_GetMaterialfv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetMaterialfv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetMaterialiv
.type glGetMaterialiv,#function
glGetMaterialiv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetMaterialiv), %g2
	or	%g2, %lo(8 * _gloffset_GetMaterialiv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetMaterialiv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetPixelMapfv
.type glGetPixelMapfv,#function
glGetPixelMapfv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetPixelMapfv), %g2
	or	%g2, %lo(8 * _gloffset_GetPixelMapfv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetPixelMapfv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetPixelMapuiv
.type glGetPixelMapuiv,#function
glGetPixelMapuiv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetPixelMapuiv), %g2
	or	%g2, %lo(8 * _gloffset_GetPixelMapuiv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetPixelMapuiv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetPixelMapusv
.type glGetPixelMapusv,#function
glGetPixelMapusv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetPixelMapusv), %g2
	or	%g2, %lo(8 * _gloffset_GetPixelMapusv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetPixelMapusv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetPolygonStipple
.type glGetPolygonStipple,#function
glGetPolygonStipple:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetPolygonStipple), %g2
	or	%g2, %lo(8 * _gloffset_GetPolygonStipple), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetPolygonStipple)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetString
.type glGetString,#function
glGetString:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetString), %g2
	or	%g2, %lo(8 * _gloffset_GetString), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetString)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetTexEnvfv
.type glGetTexEnvfv,#function
glGetTexEnvfv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetTexEnvfv), %g2
	or	%g2, %lo(8 * _gloffset_GetTexEnvfv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetTexEnvfv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetTexEnviv
.type glGetTexEnviv,#function
glGetTexEnviv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetTexEnviv), %g2
	or	%g2, %lo(8 * _gloffset_GetTexEnviv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetTexEnviv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetTexGendv
.type glGetTexGendv,#function
glGetTexGendv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetTexGendv), %g2
	or	%g2, %lo(8 * _gloffset_GetTexGendv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetTexGendv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetTexGenfv
.type glGetTexGenfv,#function
glGetTexGenfv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetTexGenfv), %g2
	or	%g2, %lo(8 * _gloffset_GetTexGenfv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetTexGenfv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetTexGeniv
.type glGetTexGeniv,#function
glGetTexGeniv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetTexGeniv), %g2
	or	%g2, %lo(8 * _gloffset_GetTexGeniv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetTexGeniv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetTexImage
.type glGetTexImage,#function
glGetTexImage:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetTexImage), %g2
	or	%g2, %lo(8 * _gloffset_GetTexImage), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetTexImage)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetTexParameterfv
.type glGetTexParameterfv,#function
glGetTexParameterfv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetTexParameterfv), %g2
	or	%g2, %lo(8 * _gloffset_GetTexParameterfv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetTexParameterfv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetTexParameteriv
.type glGetTexParameteriv,#function
glGetTexParameteriv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetTexParameteriv), %g2
	or	%g2, %lo(8 * _gloffset_GetTexParameteriv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetTexParameteriv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetTexLevelParameterfv
.type glGetTexLevelParameterfv,#function
glGetTexLevelParameterfv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetTexLevelParameterfv), %g2
	or	%g2, %lo(8 * _gloffset_GetTexLevelParameterfv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetTexLevelParameterfv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetTexLevelParameteriv
.type glGetTexLevelParameteriv,#function
glGetTexLevelParameteriv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetTexLevelParameteriv), %g2
	or	%g2, %lo(8 * _gloffset_GetTexLevelParameteriv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetTexLevelParameteriv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glIsEnabled
.type glIsEnabled,#function
glIsEnabled:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_IsEnabled), %g2
	or	%g2, %lo(8 * _gloffset_IsEnabled), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_IsEnabled)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glIsList
.type glIsList,#function
glIsList:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_IsList), %g2
	or	%g2, %lo(8 * _gloffset_IsList), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_IsList)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glDepthRange
.type glDepthRange,#function
glDepthRange:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_DepthRange), %g2
	or	%g2, %lo(8 * _gloffset_DepthRange), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_DepthRange)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glFrustum
.type glFrustum,#function
glFrustum:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Frustum), %g2
	or	%g2, %lo(8 * _gloffset_Frustum), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Frustum)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glLoadIdentity
.type glLoadIdentity,#function
glLoadIdentity:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_LoadIdentity), %g2
	or	%g2, %lo(8 * _gloffset_LoadIdentity), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_LoadIdentity)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glLoadMatrixf
.type glLoadMatrixf,#function
glLoadMatrixf:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_LoadMatrixf), %g2
	or	%g2, %lo(8 * _gloffset_LoadMatrixf), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_LoadMatrixf)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glLoadMatrixd
.type glLoadMatrixd,#function
glLoadMatrixd:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_LoadMatrixd), %g2
	or	%g2, %lo(8 * _gloffset_LoadMatrixd), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_LoadMatrixd)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMatrixMode
.type glMatrixMode,#function
glMatrixMode:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MatrixMode), %g2
	or	%g2, %lo(8 * _gloffset_MatrixMode), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MatrixMode)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultMatrixf
.type glMultMatrixf,#function
glMultMatrixf:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultMatrixf), %g2
	or	%g2, %lo(8 * _gloffset_MultMatrixf), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultMatrixf)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultMatrixd
.type glMultMatrixd,#function
glMultMatrixd:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultMatrixd), %g2
	or	%g2, %lo(8 * _gloffset_MultMatrixd), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultMatrixd)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glOrtho
.type glOrtho,#function
glOrtho:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Ortho), %g2
	or	%g2, %lo(8 * _gloffset_Ortho), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Ortho)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glPopMatrix
.type glPopMatrix,#function
glPopMatrix:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_PopMatrix), %g2
	or	%g2, %lo(8 * _gloffset_PopMatrix), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_PopMatrix)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glPushMatrix
.type glPushMatrix,#function
glPushMatrix:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_PushMatrix), %g2
	or	%g2, %lo(8 * _gloffset_PushMatrix), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_PushMatrix)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glRotated
.type glRotated,#function
glRotated:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Rotated), %g2
	or	%g2, %lo(8 * _gloffset_Rotated), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Rotated)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glRotatef
.type glRotatef,#function
glRotatef:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Rotatef), %g2
	or	%g2, %lo(8 * _gloffset_Rotatef), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Rotatef)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glScaled
.type glScaled,#function
glScaled:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Scaled), %g2
	or	%g2, %lo(8 * _gloffset_Scaled), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Scaled)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glScalef
.type glScalef,#function
glScalef:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Scalef), %g2
	or	%g2, %lo(8 * _gloffset_Scalef), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Scalef)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTranslated
.type glTranslated,#function
glTranslated:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Translated), %g2
	or	%g2, %lo(8 * _gloffset_Translated), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Translated)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTranslatef
.type glTranslatef,#function
glTranslatef:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Translatef), %g2
	or	%g2, %lo(8 * _gloffset_Translatef), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Translatef)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glViewport
.type glViewport,#function
glViewport:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Viewport), %g2
	or	%g2, %lo(8 * _gloffset_Viewport), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Viewport)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glArrayElement
.type glArrayElement,#function
glArrayElement:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ArrayElement), %g2
	or	%g2, %lo(8 * _gloffset_ArrayElement), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ArrayElement)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColorPointer
.type glColorPointer,#function
glColorPointer:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ColorPointer), %g2
	or	%g2, %lo(8 * _gloffset_ColorPointer), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ColorPointer)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glDisableClientState
.type glDisableClientState,#function
glDisableClientState:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_DisableClientState), %g2
	or	%g2, %lo(8 * _gloffset_DisableClientState), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_DisableClientState)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glDrawArrays
.type glDrawArrays,#function
glDrawArrays:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_DrawArrays), %g2
	or	%g2, %lo(8 * _gloffset_DrawArrays), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_DrawArrays)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glDrawElements
.type glDrawElements,#function
glDrawElements:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_DrawElements), %g2
	or	%g2, %lo(8 * _gloffset_DrawElements), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_DrawElements)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glEdgeFlagPointer
.type glEdgeFlagPointer,#function
glEdgeFlagPointer:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_EdgeFlagPointer), %g2
	or	%g2, %lo(8 * _gloffset_EdgeFlagPointer), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_EdgeFlagPointer)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glEnableClientState
.type glEnableClientState,#function
glEnableClientState:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_EnableClientState), %g2
	or	%g2, %lo(8 * _gloffset_EnableClientState), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_EnableClientState)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetPointerv
.type glGetPointerv,#function
glGetPointerv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetPointerv), %g2
	or	%g2, %lo(8 * _gloffset_GetPointerv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetPointerv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glIndexPointer
.type glIndexPointer,#function
glIndexPointer:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_IndexPointer), %g2
	or	%g2, %lo(8 * _gloffset_IndexPointer), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_IndexPointer)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glInterleavedArrays
.type glInterleavedArrays,#function
glInterleavedArrays:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_InterleavedArrays), %g2
	or	%g2, %lo(8 * _gloffset_InterleavedArrays), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_InterleavedArrays)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glNormalPointer
.type glNormalPointer,#function
glNormalPointer:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_NormalPointer), %g2
	or	%g2, %lo(8 * _gloffset_NormalPointer), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_NormalPointer)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexCoordPointer
.type glTexCoordPointer,#function
glTexCoordPointer:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexCoordPointer), %g2
	or	%g2, %lo(8 * _gloffset_TexCoordPointer), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexCoordPointer)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexPointer
.type glVertexPointer,#function
glVertexPointer:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexPointer), %g2
	or	%g2, %lo(8 * _gloffset_VertexPointer), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexPointer)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glPolygonOffset
.type glPolygonOffset,#function
glPolygonOffset:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_PolygonOffset), %g2
	or	%g2, %lo(8 * _gloffset_PolygonOffset), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_PolygonOffset)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glCopyTexImage1D
.type glCopyTexImage1D,#function
glCopyTexImage1D:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_CopyTexImage1D), %g2
	or	%g2, %lo(8 * _gloffset_CopyTexImage1D), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_CopyTexImage1D)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glCopyTexImage2D
.type glCopyTexImage2D,#function
glCopyTexImage2D:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_CopyTexImage2D), %g2
	or	%g2, %lo(8 * _gloffset_CopyTexImage2D), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_CopyTexImage2D)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glCopyTexSubImage1D
.type glCopyTexSubImage1D,#function
glCopyTexSubImage1D:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_CopyTexSubImage1D), %g2
	or	%g2, %lo(8 * _gloffset_CopyTexSubImage1D), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_CopyTexSubImage1D)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glCopyTexSubImage2D
.type glCopyTexSubImage2D,#function
glCopyTexSubImage2D:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_CopyTexSubImage2D), %g2
	or	%g2, %lo(8 * _gloffset_CopyTexSubImage2D), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_CopyTexSubImage2D)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexSubImage1D
.type glTexSubImage1D,#function
glTexSubImage1D:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexSubImage1D), %g2
	or	%g2, %lo(8 * _gloffset_TexSubImage1D), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexSubImage1D)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexSubImage2D
.type glTexSubImage2D,#function
glTexSubImage2D:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexSubImage2D), %g2
	or	%g2, %lo(8 * _gloffset_TexSubImage2D), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexSubImage2D)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glAreTexturesResident
.type glAreTexturesResident,#function
glAreTexturesResident:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_AreTexturesResident), %g2
	or	%g2, %lo(8 * _gloffset_AreTexturesResident), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_AreTexturesResident)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glBindTexture
.type glBindTexture,#function
glBindTexture:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_BindTexture), %g2
	or	%g2, %lo(8 * _gloffset_BindTexture), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_BindTexture)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glDeleteTextures
.type glDeleteTextures,#function
glDeleteTextures:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_DeleteTextures), %g2
	or	%g2, %lo(8 * _gloffset_DeleteTextures), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_DeleteTextures)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGenTextures
.type glGenTextures,#function
glGenTextures:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GenTextures), %g2
	or	%g2, %lo(8 * _gloffset_GenTextures), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GenTextures)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glIsTexture
.type glIsTexture,#function
glIsTexture:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_IsTexture), %g2
	or	%g2, %lo(8 * _gloffset_IsTexture), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_IsTexture)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glPrioritizeTextures
.type glPrioritizeTextures,#function
glPrioritizeTextures:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_PrioritizeTextures), %g2
	or	%g2, %lo(8 * _gloffset_PrioritizeTextures), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_PrioritizeTextures)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glIndexub
.type glIndexub,#function
glIndexub:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Indexub), %g2
	or	%g2, %lo(8 * _gloffset_Indexub), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Indexub)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glIndexubv
.type glIndexubv,#function
glIndexubv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Indexubv), %g2
	or	%g2, %lo(8 * _gloffset_Indexubv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Indexubv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glPopClientAttrib
.type glPopClientAttrib,#function
glPopClientAttrib:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_PopClientAttrib), %g2
	or	%g2, %lo(8 * _gloffset_PopClientAttrib), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_PopClientAttrib)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glPushClientAttrib
.type glPushClientAttrib,#function
glPushClientAttrib:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_PushClientAttrib), %g2
	or	%g2, %lo(8 * _gloffset_PushClientAttrib), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_PushClientAttrib)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glBlendColor
.type glBlendColor,#function
glBlendColor:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_BlendColor), %g2
	or	%g2, %lo(8 * _gloffset_BlendColor), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_BlendColor)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glBlendEquation
.type glBlendEquation,#function
glBlendEquation:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_BlendEquation), %g2
	or	%g2, %lo(8 * _gloffset_BlendEquation), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_BlendEquation)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glDrawRangeElements
.type glDrawRangeElements,#function
glDrawRangeElements:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_DrawRangeElements), %g2
	or	%g2, %lo(8 * _gloffset_DrawRangeElements), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_DrawRangeElements)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColorTable
.type glColorTable,#function
glColorTable:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ColorTable), %g2
	or	%g2, %lo(8 * _gloffset_ColorTable), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ColorTable)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColorTableParameterfv
.type glColorTableParameterfv,#function
glColorTableParameterfv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ColorTableParameterfv), %g2
	or	%g2, %lo(8 * _gloffset_ColorTableParameterfv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ColorTableParameterfv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColorTableParameteriv
.type glColorTableParameteriv,#function
glColorTableParameteriv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ColorTableParameteriv), %g2
	or	%g2, %lo(8 * _gloffset_ColorTableParameteriv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ColorTableParameteriv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glCopyColorTable
.type glCopyColorTable,#function
glCopyColorTable:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_CopyColorTable), %g2
	or	%g2, %lo(8 * _gloffset_CopyColorTable), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_CopyColorTable)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetColorTable
.type glGetColorTable,#function
glGetColorTable:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetColorTable), %g2
	or	%g2, %lo(8 * _gloffset_GetColorTable), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetColorTable)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetColorTableParameterfv
.type glGetColorTableParameterfv,#function
glGetColorTableParameterfv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetColorTableParameterfv), %g2
	or	%g2, %lo(8 * _gloffset_GetColorTableParameterfv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetColorTableParameterfv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetColorTableParameteriv
.type glGetColorTableParameteriv,#function
glGetColorTableParameteriv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetColorTableParameteriv), %g2
	or	%g2, %lo(8 * _gloffset_GetColorTableParameteriv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetColorTableParameteriv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColorSubTable
.type glColorSubTable,#function
glColorSubTable:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ColorSubTable), %g2
	or	%g2, %lo(8 * _gloffset_ColorSubTable), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ColorSubTable)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glCopyColorSubTable
.type glCopyColorSubTable,#function
glCopyColorSubTable:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_CopyColorSubTable), %g2
	or	%g2, %lo(8 * _gloffset_CopyColorSubTable), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_CopyColorSubTable)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glConvolutionFilter1D
.type glConvolutionFilter1D,#function
glConvolutionFilter1D:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ConvolutionFilter1D), %g2
	or	%g2, %lo(8 * _gloffset_ConvolutionFilter1D), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ConvolutionFilter1D)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glConvolutionFilter2D
.type glConvolutionFilter2D,#function
glConvolutionFilter2D:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ConvolutionFilter2D), %g2
	or	%g2, %lo(8 * _gloffset_ConvolutionFilter2D), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ConvolutionFilter2D)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glConvolutionParameterf
.type glConvolutionParameterf,#function
glConvolutionParameterf:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ConvolutionParameterf), %g2
	or	%g2, %lo(8 * _gloffset_ConvolutionParameterf), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ConvolutionParameterf)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glConvolutionParameterfv
.type glConvolutionParameterfv,#function
glConvolutionParameterfv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ConvolutionParameterfv), %g2
	or	%g2, %lo(8 * _gloffset_ConvolutionParameterfv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ConvolutionParameterfv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glConvolutionParameteri
.type glConvolutionParameteri,#function
glConvolutionParameteri:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ConvolutionParameteri), %g2
	or	%g2, %lo(8 * _gloffset_ConvolutionParameteri), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ConvolutionParameteri)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glConvolutionParameteriv
.type glConvolutionParameteriv,#function
glConvolutionParameteriv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ConvolutionParameteriv), %g2
	or	%g2, %lo(8 * _gloffset_ConvolutionParameteriv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ConvolutionParameteriv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glCopyConvolutionFilter1D
.type glCopyConvolutionFilter1D,#function
glCopyConvolutionFilter1D:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_CopyConvolutionFilter1D), %g2
	or	%g2, %lo(8 * _gloffset_CopyConvolutionFilter1D), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_CopyConvolutionFilter1D)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glCopyConvolutionFilter2D
.type glCopyConvolutionFilter2D,#function
glCopyConvolutionFilter2D:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_CopyConvolutionFilter2D), %g2
	or	%g2, %lo(8 * _gloffset_CopyConvolutionFilter2D), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_CopyConvolutionFilter2D)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetConvolutionFilter
.type glGetConvolutionFilter,#function
glGetConvolutionFilter:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetConvolutionFilter), %g2
	or	%g2, %lo(8 * _gloffset_GetConvolutionFilter), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetConvolutionFilter)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetConvolutionParameterfv
.type glGetConvolutionParameterfv,#function
glGetConvolutionParameterfv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetConvolutionParameterfv), %g2
	or	%g2, %lo(8 * _gloffset_GetConvolutionParameterfv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetConvolutionParameterfv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetConvolutionParameteriv
.type glGetConvolutionParameteriv,#function
glGetConvolutionParameteriv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetConvolutionParameteriv), %g2
	or	%g2, %lo(8 * _gloffset_GetConvolutionParameteriv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetConvolutionParameteriv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetSeparableFilter
.type glGetSeparableFilter,#function
glGetSeparableFilter:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetSeparableFilter), %g2
	or	%g2, %lo(8 * _gloffset_GetSeparableFilter), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetSeparableFilter)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSeparableFilter2D
.type glSeparableFilter2D,#function
glSeparableFilter2D:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SeparableFilter2D), %g2
	or	%g2, %lo(8 * _gloffset_SeparableFilter2D), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SeparableFilter2D)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetHistogram
.type glGetHistogram,#function
glGetHistogram:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetHistogram), %g2
	or	%g2, %lo(8 * _gloffset_GetHistogram), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetHistogram)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetHistogramParameterfv
.type glGetHistogramParameterfv,#function
glGetHistogramParameterfv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetHistogramParameterfv), %g2
	or	%g2, %lo(8 * _gloffset_GetHistogramParameterfv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetHistogramParameterfv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetHistogramParameteriv
.type glGetHistogramParameteriv,#function
glGetHistogramParameteriv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetHistogramParameteriv), %g2
	or	%g2, %lo(8 * _gloffset_GetHistogramParameteriv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetHistogramParameteriv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetMinmax
.type glGetMinmax,#function
glGetMinmax:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetMinmax), %g2
	or	%g2, %lo(8 * _gloffset_GetMinmax), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetMinmax)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetMinmaxParameterfv
.type glGetMinmaxParameterfv,#function
glGetMinmaxParameterfv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetMinmaxParameterfv), %g2
	or	%g2, %lo(8 * _gloffset_GetMinmaxParameterfv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetMinmaxParameterfv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetMinmaxParameteriv
.type glGetMinmaxParameteriv,#function
glGetMinmaxParameteriv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetMinmaxParameteriv), %g2
	or	%g2, %lo(8 * _gloffset_GetMinmaxParameteriv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetMinmaxParameteriv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glHistogram
.type glHistogram,#function
glHistogram:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Histogram), %g2
	or	%g2, %lo(8 * _gloffset_Histogram), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Histogram)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMinmax
.type glMinmax,#function
glMinmax:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Minmax), %g2
	or	%g2, %lo(8 * _gloffset_Minmax), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Minmax)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glResetHistogram
.type glResetHistogram,#function
glResetHistogram:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ResetHistogram), %g2
	or	%g2, %lo(8 * _gloffset_ResetHistogram), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ResetHistogram)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glResetMinmax
.type glResetMinmax,#function
glResetMinmax:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ResetMinmax), %g2
	or	%g2, %lo(8 * _gloffset_ResetMinmax), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ResetMinmax)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexImage3D
.type glTexImage3D,#function
glTexImage3D:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexImage3D), %g2
	or	%g2, %lo(8 * _gloffset_TexImage3D), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexImage3D)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexSubImage3D
.type glTexSubImage3D,#function
glTexSubImage3D:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexSubImage3D), %g2
	or	%g2, %lo(8 * _gloffset_TexSubImage3D), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexSubImage3D)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glCopyTexSubImage3D
.type glCopyTexSubImage3D,#function
glCopyTexSubImage3D:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_CopyTexSubImage3D), %g2
	or	%g2, %lo(8 * _gloffset_CopyTexSubImage3D), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_CopyTexSubImage3D)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glActiveTextureARB
.type glActiveTextureARB,#function
glActiveTextureARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ActiveTextureARB), %g2
	or	%g2, %lo(8 * _gloffset_ActiveTextureARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ActiveTextureARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glClientActiveTextureARB
.type glClientActiveTextureARB,#function
glClientActiveTextureARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ClientActiveTextureARB), %g2
	or	%g2, %lo(8 * _gloffset_ClientActiveTextureARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ClientActiveTextureARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord1dARB
.type glMultiTexCoord1dARB,#function
glMultiTexCoord1dARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord1dARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord1dARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord1dARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord1dvARB
.type glMultiTexCoord1dvARB,#function
glMultiTexCoord1dvARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord1dvARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord1dvARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord1dvARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord1fARB
.type glMultiTexCoord1fARB,#function
glMultiTexCoord1fARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord1fARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord1fARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord1fARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord1fvARB
.type glMultiTexCoord1fvARB,#function
glMultiTexCoord1fvARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord1fvARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord1fvARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord1fvARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord1iARB
.type glMultiTexCoord1iARB,#function
glMultiTexCoord1iARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord1iARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord1iARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord1iARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord1ivARB
.type glMultiTexCoord1ivARB,#function
glMultiTexCoord1ivARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord1ivARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord1ivARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord1ivARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord1sARB
.type glMultiTexCoord1sARB,#function
glMultiTexCoord1sARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord1sARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord1sARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord1sARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord1svARB
.type glMultiTexCoord1svARB,#function
glMultiTexCoord1svARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord1svARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord1svARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord1svARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord2dARB
.type glMultiTexCoord2dARB,#function
glMultiTexCoord2dARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord2dARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord2dARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord2dARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord2dvARB
.type glMultiTexCoord2dvARB,#function
glMultiTexCoord2dvARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord2dvARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord2dvARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord2dvARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord2fARB
.type glMultiTexCoord2fARB,#function
glMultiTexCoord2fARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord2fARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord2fARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord2fARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord2fvARB
.type glMultiTexCoord2fvARB,#function
glMultiTexCoord2fvARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord2fvARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord2fvARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord2fvARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord2iARB
.type glMultiTexCoord2iARB,#function
glMultiTexCoord2iARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord2iARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord2iARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord2iARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord2ivARB
.type glMultiTexCoord2ivARB,#function
glMultiTexCoord2ivARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord2ivARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord2ivARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord2ivARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord2sARB
.type glMultiTexCoord2sARB,#function
glMultiTexCoord2sARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord2sARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord2sARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord2sARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord2svARB
.type glMultiTexCoord2svARB,#function
glMultiTexCoord2svARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord2svARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord2svARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord2svARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord3dARB
.type glMultiTexCoord3dARB,#function
glMultiTexCoord3dARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord3dARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord3dARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord3dARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord3dvARB
.type glMultiTexCoord3dvARB,#function
glMultiTexCoord3dvARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord3dvARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord3dvARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord3dvARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord3fARB
.type glMultiTexCoord3fARB,#function
glMultiTexCoord3fARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord3fARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord3fARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord3fARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord3fvARB
.type glMultiTexCoord3fvARB,#function
glMultiTexCoord3fvARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord3fvARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord3fvARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord3fvARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord3iARB
.type glMultiTexCoord3iARB,#function
glMultiTexCoord3iARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord3iARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord3iARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord3iARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord3ivARB
.type glMultiTexCoord3ivARB,#function
glMultiTexCoord3ivARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord3ivARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord3ivARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord3ivARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord3sARB
.type glMultiTexCoord3sARB,#function
glMultiTexCoord3sARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord3sARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord3sARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord3sARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord3svARB
.type glMultiTexCoord3svARB,#function
glMultiTexCoord3svARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord3svARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord3svARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord3svARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord4dARB
.type glMultiTexCoord4dARB,#function
glMultiTexCoord4dARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord4dARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord4dARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord4dARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord4dvARB
.type glMultiTexCoord4dvARB,#function
glMultiTexCoord4dvARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord4dvARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord4dvARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord4dvARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord4fARB
.type glMultiTexCoord4fARB,#function
glMultiTexCoord4fARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord4fARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord4fARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord4fARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord4fvARB
.type glMultiTexCoord4fvARB,#function
glMultiTexCoord4fvARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord4fvARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord4fvARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord4fvARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord4iARB
.type glMultiTexCoord4iARB,#function
glMultiTexCoord4iARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord4iARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord4iARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord4iARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord4ivARB
.type glMultiTexCoord4ivARB,#function
glMultiTexCoord4ivARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord4ivARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord4ivARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord4ivARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord4sARB
.type glMultiTexCoord4sARB,#function
glMultiTexCoord4sARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord4sARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord4sARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord4sARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord4svARB
.type glMultiTexCoord4svARB,#function
glMultiTexCoord4svARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord4svARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord4svARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord4svARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glLoadTransposeMatrixfARB
.type glLoadTransposeMatrixfARB,#function
glLoadTransposeMatrixfARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_LoadTransposeMatrixfARB), %g2
	or	%g2, %lo(8 * _gloffset_LoadTransposeMatrixfARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_LoadTransposeMatrixfARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glLoadTransposeMatrixdARB
.type glLoadTransposeMatrixdARB,#function
glLoadTransposeMatrixdARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_LoadTransposeMatrixdARB), %g2
	or	%g2, %lo(8 * _gloffset_LoadTransposeMatrixdARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_LoadTransposeMatrixdARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultTransposeMatrixfARB
.type glMultTransposeMatrixfARB,#function
glMultTransposeMatrixfARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultTransposeMatrixfARB), %g2
	or	%g2, %lo(8 * _gloffset_MultTransposeMatrixfARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultTransposeMatrixfARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultTransposeMatrixdARB
.type glMultTransposeMatrixdARB,#function
glMultTransposeMatrixdARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultTransposeMatrixdARB), %g2
	or	%g2, %lo(8 * _gloffset_MultTransposeMatrixdARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultTransposeMatrixdARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSampleCoverageARB
.type glSampleCoverageARB,#function
glSampleCoverageARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SampleCoverageARB), %g2
	or	%g2, %lo(8 * _gloffset_SampleCoverageARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SampleCoverageARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl gl__unused413
.type gl__unused413,#function
gl__unused413:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset___unused413), %g2
	or	%g2, %lo(8 * _gloffset___unused413), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset___unused413)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glCompressedTexImage3DARB
.type glCompressedTexImage3DARB,#function
glCompressedTexImage3DARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_CompressedTexImage3DARB), %g2
	or	%g2, %lo(8 * _gloffset_CompressedTexImage3DARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_CompressedTexImage3DARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glCompressedTexImage2DARB
.type glCompressedTexImage2DARB,#function
glCompressedTexImage2DARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_CompressedTexImage2DARB), %g2
	or	%g2, %lo(8 * _gloffset_CompressedTexImage2DARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_CompressedTexImage2DARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glCompressedTexImage1DARB
.type glCompressedTexImage1DARB,#function
glCompressedTexImage1DARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_CompressedTexImage1DARB), %g2
	or	%g2, %lo(8 * _gloffset_CompressedTexImage1DARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_CompressedTexImage1DARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glCompressedTexSubImage3DARB
.type glCompressedTexSubImage3DARB,#function
glCompressedTexSubImage3DARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_CompressedTexSubImage3DARB), %g2
	or	%g2, %lo(8 * _gloffset_CompressedTexSubImage3DARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_CompressedTexSubImage3DARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glCompressedTexSubImage2DARB
.type glCompressedTexSubImage2DARB,#function
glCompressedTexSubImage2DARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_CompressedTexSubImage2DARB), %g2
	or	%g2, %lo(8 * _gloffset_CompressedTexSubImage2DARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_CompressedTexSubImage2DARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glCompressedTexSubImage1DARB
.type glCompressedTexSubImage1DARB,#function
glCompressedTexSubImage1DARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_CompressedTexSubImage1DARB), %g2
	or	%g2, %lo(8 * _gloffset_CompressedTexSubImage1DARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_CompressedTexSubImage1DARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetCompressedTexImageARB
.type glGetCompressedTexImageARB,#function
glGetCompressedTexImageARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetCompressedTexImageARB), %g2
	or	%g2, %lo(8 * _gloffset_GetCompressedTexImageARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetCompressedTexImageARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glActiveTexture
.type glActiveTexture,#function
glActiveTexture:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ActiveTextureARB), %g2
	or	%g2, %lo(8 * _gloffset_ActiveTextureARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ActiveTextureARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glClientActiveTexture
.type glClientActiveTexture,#function
glClientActiveTexture:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ClientActiveTextureARB), %g2
	or	%g2, %lo(8 * _gloffset_ClientActiveTextureARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ClientActiveTextureARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord1d
.type glMultiTexCoord1d,#function
glMultiTexCoord1d:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord1dARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord1dARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord1dARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord1dv
.type glMultiTexCoord1dv,#function
glMultiTexCoord1dv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord1dvARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord1dvARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord1dvARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord1f
.type glMultiTexCoord1f,#function
glMultiTexCoord1f:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord1fARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord1fARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord1fARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord1fv
.type glMultiTexCoord1fv,#function
glMultiTexCoord1fv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord1fvARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord1fvARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord1fvARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord1i
.type glMultiTexCoord1i,#function
glMultiTexCoord1i:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord1iARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord1iARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord1iARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord1iv
.type glMultiTexCoord1iv,#function
glMultiTexCoord1iv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord1ivARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord1ivARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord1ivARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord1s
.type glMultiTexCoord1s,#function
glMultiTexCoord1s:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord1sARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord1sARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord1sARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord1sv
.type glMultiTexCoord1sv,#function
glMultiTexCoord1sv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord1svARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord1svARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord1svARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord2d
.type glMultiTexCoord2d,#function
glMultiTexCoord2d:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord2dARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord2dARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord2dARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord2dv
.type glMultiTexCoord2dv,#function
glMultiTexCoord2dv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord2dvARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord2dvARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord2dvARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord2f
.type glMultiTexCoord2f,#function
glMultiTexCoord2f:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord2fARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord2fARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord2fARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord2fv
.type glMultiTexCoord2fv,#function
glMultiTexCoord2fv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord2fvARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord2fvARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord2fvARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord2i
.type glMultiTexCoord2i,#function
glMultiTexCoord2i:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord2iARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord2iARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord2iARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord2iv
.type glMultiTexCoord2iv,#function
glMultiTexCoord2iv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord2ivARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord2ivARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord2ivARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord2s
.type glMultiTexCoord2s,#function
glMultiTexCoord2s:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord2sARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord2sARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord2sARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord2sv
.type glMultiTexCoord2sv,#function
glMultiTexCoord2sv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord2svARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord2svARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord2svARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord3d
.type glMultiTexCoord3d,#function
glMultiTexCoord3d:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord3dARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord3dARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord3dARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord3dv
.type glMultiTexCoord3dv,#function
glMultiTexCoord3dv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord3dvARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord3dvARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord3dvARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord3f
.type glMultiTexCoord3f,#function
glMultiTexCoord3f:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord3fARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord3fARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord3fARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord3fv
.type glMultiTexCoord3fv,#function
glMultiTexCoord3fv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord3fvARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord3fvARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord3fvARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord3i
.type glMultiTexCoord3i,#function
glMultiTexCoord3i:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord3iARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord3iARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord3iARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord3iv
.type glMultiTexCoord3iv,#function
glMultiTexCoord3iv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord3ivARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord3ivARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord3ivARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord3s
.type glMultiTexCoord3s,#function
glMultiTexCoord3s:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord3sARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord3sARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord3sARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord3sv
.type glMultiTexCoord3sv,#function
glMultiTexCoord3sv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord3svARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord3svARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord3svARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord4d
.type glMultiTexCoord4d,#function
glMultiTexCoord4d:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord4dARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord4dARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord4dARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord4dv
.type glMultiTexCoord4dv,#function
glMultiTexCoord4dv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord4dvARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord4dvARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord4dvARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord4f
.type glMultiTexCoord4f,#function
glMultiTexCoord4f:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord4fARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord4fARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord4fARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord4fv
.type glMultiTexCoord4fv,#function
glMultiTexCoord4fv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord4fvARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord4fvARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord4fvARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord4i
.type glMultiTexCoord4i,#function
glMultiTexCoord4i:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord4iARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord4iARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord4iARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord4iv
.type glMultiTexCoord4iv,#function
glMultiTexCoord4iv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord4ivARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord4ivARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord4ivARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord4s
.type glMultiTexCoord4s,#function
glMultiTexCoord4s:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord4sARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord4sARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord4sARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiTexCoord4sv
.type glMultiTexCoord4sv,#function
glMultiTexCoord4sv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiTexCoord4svARB), %g2
	or	%g2, %lo(8 * _gloffset_MultiTexCoord4svARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiTexCoord4svARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glLoadTransposeMatrixf
.type glLoadTransposeMatrixf,#function
glLoadTransposeMatrixf:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_LoadTransposeMatrixfARB), %g2
	or	%g2, %lo(8 * _gloffset_LoadTransposeMatrixfARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_LoadTransposeMatrixfARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glLoadTransposeMatrixd
.type glLoadTransposeMatrixd,#function
glLoadTransposeMatrixd:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_LoadTransposeMatrixdARB), %g2
	or	%g2, %lo(8 * _gloffset_LoadTransposeMatrixdARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_LoadTransposeMatrixdARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultTransposeMatrixf
.type glMultTransposeMatrixf,#function
glMultTransposeMatrixf:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultTransposeMatrixfARB), %g2
	or	%g2, %lo(8 * _gloffset_MultTransposeMatrixfARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultTransposeMatrixfARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultTransposeMatrixd
.type glMultTransposeMatrixd,#function
glMultTransposeMatrixd:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultTransposeMatrixdARB), %g2
	or	%g2, %lo(8 * _gloffset_MultTransposeMatrixdARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultTransposeMatrixdARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSampleCoverage
.type glSampleCoverage,#function
glSampleCoverage:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SampleCoverageARB), %g2
	or	%g2, %lo(8 * _gloffset_SampleCoverageARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SampleCoverageARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glCompressedTexImage3D
.type glCompressedTexImage3D,#function
glCompressedTexImage3D:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_CompressedTexImage3DARB), %g2
	or	%g2, %lo(8 * _gloffset_CompressedTexImage3DARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_CompressedTexImage3DARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glCompressedTexImage2D
.type glCompressedTexImage2D,#function
glCompressedTexImage2D:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_CompressedTexImage2DARB), %g2
	or	%g2, %lo(8 * _gloffset_CompressedTexImage2DARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_CompressedTexImage2DARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glCompressedTexImage1D
.type glCompressedTexImage1D,#function
glCompressedTexImage1D:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_CompressedTexImage1DARB), %g2
	or	%g2, %lo(8 * _gloffset_CompressedTexImage1DARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_CompressedTexImage1DARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glCompressedTexSubImage3D
.type glCompressedTexSubImage3D,#function
glCompressedTexSubImage3D:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_CompressedTexSubImage3DARB), %g2
	or	%g2, %lo(8 * _gloffset_CompressedTexSubImage3DARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_CompressedTexSubImage3DARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glCompressedTexSubImage2D
.type glCompressedTexSubImage2D,#function
glCompressedTexSubImage2D:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_CompressedTexSubImage2DARB), %g2
	or	%g2, %lo(8 * _gloffset_CompressedTexSubImage2DARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_CompressedTexSubImage2DARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glCompressedTexSubImage1D
.type glCompressedTexSubImage1D,#function
glCompressedTexSubImage1D:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_CompressedTexSubImage1DARB), %g2
	or	%g2, %lo(8 * _gloffset_CompressedTexSubImage1DARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_CompressedTexSubImage1DARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetCompressedTexImage
.type glGetCompressedTexImage,#function
glGetCompressedTexImage:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetCompressedTexImageARB), %g2
	or	%g2, %lo(8 * _gloffset_GetCompressedTexImageARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetCompressedTexImageARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glBlendColorEXT
.type glBlendColorEXT,#function
glBlendColorEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_BlendColor), %g2
	or	%g2, %lo(8 * _gloffset_BlendColor), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_BlendColor)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glPolygonOffsetEXT
.type glPolygonOffsetEXT,#function
glPolygonOffsetEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_PolygonOffsetEXT), %g2
	or	%g2, %lo(8 * _gloffset_PolygonOffsetEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_PolygonOffsetEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexImage3DEXT
.type glTexImage3DEXT,#function
glTexImage3DEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexImage3D), %g2
	or	%g2, %lo(8 * _gloffset_TexImage3D), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexImage3D)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexSubImage3DEXT
.type glTexSubImage3DEXT,#function
glTexSubImage3DEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexSubImage3D), %g2
	or	%g2, %lo(8 * _gloffset_TexSubImage3D), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexSubImage3D)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetTexFilterFuncSGIS
.type glGetTexFilterFuncSGIS,#function
glGetTexFilterFuncSGIS:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetTexFilterFuncSGIS), %g2
	or	%g2, %lo(8 * _gloffset_GetTexFilterFuncSGIS), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetTexFilterFuncSGIS)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexFilterFuncSGIS
.type glTexFilterFuncSGIS,#function
glTexFilterFuncSGIS:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexFilterFuncSGIS), %g2
	or	%g2, %lo(8 * _gloffset_TexFilterFuncSGIS), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexFilterFuncSGIS)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexSubImage1DEXT
.type glTexSubImage1DEXT,#function
glTexSubImage1DEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexSubImage1D), %g2
	or	%g2, %lo(8 * _gloffset_TexSubImage1D), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexSubImage1D)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexSubImage2DEXT
.type glTexSubImage2DEXT,#function
glTexSubImage2DEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexSubImage2D), %g2
	or	%g2, %lo(8 * _gloffset_TexSubImage2D), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexSubImage2D)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glCopyTexImage1DEXT
.type glCopyTexImage1DEXT,#function
glCopyTexImage1DEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_CopyTexImage1D), %g2
	or	%g2, %lo(8 * _gloffset_CopyTexImage1D), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_CopyTexImage1D)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glCopyTexImage2DEXT
.type glCopyTexImage2DEXT,#function
glCopyTexImage2DEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_CopyTexImage2D), %g2
	or	%g2, %lo(8 * _gloffset_CopyTexImage2D), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_CopyTexImage2D)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glCopyTexSubImage1DEXT
.type glCopyTexSubImage1DEXT,#function
glCopyTexSubImage1DEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_CopyTexSubImage1D), %g2
	or	%g2, %lo(8 * _gloffset_CopyTexSubImage1D), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_CopyTexSubImage1D)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glCopyTexSubImage2DEXT
.type glCopyTexSubImage2DEXT,#function
glCopyTexSubImage2DEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_CopyTexSubImage2D), %g2
	or	%g2, %lo(8 * _gloffset_CopyTexSubImage2D), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_CopyTexSubImage2D)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glCopyTexSubImage3DEXT
.type glCopyTexSubImage3DEXT,#function
glCopyTexSubImage3DEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_CopyTexSubImage3D), %g2
	or	%g2, %lo(8 * _gloffset_CopyTexSubImage3D), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_CopyTexSubImage3D)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetHistogramEXT
.type glGetHistogramEXT,#function
glGetHistogramEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetHistogramEXT), %g2
	or	%g2, %lo(8 * _gloffset_GetHistogramEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetHistogramEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetHistogramParameterfvEXT
.type glGetHistogramParameterfvEXT,#function
glGetHistogramParameterfvEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetHistogramParameterfvEXT), %g2
	or	%g2, %lo(8 * _gloffset_GetHistogramParameterfvEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetHistogramParameterfvEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetHistogramParameterivEXT
.type glGetHistogramParameterivEXT,#function
glGetHistogramParameterivEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetHistogramParameterivEXT), %g2
	or	%g2, %lo(8 * _gloffset_GetHistogramParameterivEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetHistogramParameterivEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetMinmaxEXT
.type glGetMinmaxEXT,#function
glGetMinmaxEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetMinmaxEXT), %g2
	or	%g2, %lo(8 * _gloffset_GetMinmaxEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetMinmaxEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetMinmaxParameterfvEXT
.type glGetMinmaxParameterfvEXT,#function
glGetMinmaxParameterfvEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetMinmaxParameterfvEXT), %g2
	or	%g2, %lo(8 * _gloffset_GetMinmaxParameterfvEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetMinmaxParameterfvEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetMinmaxParameterivEXT
.type glGetMinmaxParameterivEXT,#function
glGetMinmaxParameterivEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetMinmaxParameterivEXT), %g2
	or	%g2, %lo(8 * _gloffset_GetMinmaxParameterivEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetMinmaxParameterivEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glHistogramEXT
.type glHistogramEXT,#function
glHistogramEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Histogram), %g2
	or	%g2, %lo(8 * _gloffset_Histogram), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Histogram)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMinmaxEXT
.type glMinmaxEXT,#function
glMinmaxEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_Minmax), %g2
	or	%g2, %lo(8 * _gloffset_Minmax), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_Minmax)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glResetHistogramEXT
.type glResetHistogramEXT,#function
glResetHistogramEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ResetHistogram), %g2
	or	%g2, %lo(8 * _gloffset_ResetHistogram), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ResetHistogram)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glResetMinmaxEXT
.type glResetMinmaxEXT,#function
glResetMinmaxEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ResetMinmax), %g2
	or	%g2, %lo(8 * _gloffset_ResetMinmax), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ResetMinmax)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glConvolutionFilter1DEXT
.type glConvolutionFilter1DEXT,#function
glConvolutionFilter1DEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ConvolutionFilter1D), %g2
	or	%g2, %lo(8 * _gloffset_ConvolutionFilter1D), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ConvolutionFilter1D)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glConvolutionFilter2DEXT
.type glConvolutionFilter2DEXT,#function
glConvolutionFilter2DEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ConvolutionFilter2D), %g2
	or	%g2, %lo(8 * _gloffset_ConvolutionFilter2D), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ConvolutionFilter2D)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glConvolutionParameterfEXT
.type glConvolutionParameterfEXT,#function
glConvolutionParameterfEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ConvolutionParameterf), %g2
	or	%g2, %lo(8 * _gloffset_ConvolutionParameterf), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ConvolutionParameterf)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glConvolutionParameterfvEXT
.type glConvolutionParameterfvEXT,#function
glConvolutionParameterfvEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ConvolutionParameterfv), %g2
	or	%g2, %lo(8 * _gloffset_ConvolutionParameterfv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ConvolutionParameterfv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glConvolutionParameteriEXT
.type glConvolutionParameteriEXT,#function
glConvolutionParameteriEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ConvolutionParameteri), %g2
	or	%g2, %lo(8 * _gloffset_ConvolutionParameteri), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ConvolutionParameteri)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glConvolutionParameterivEXT
.type glConvolutionParameterivEXT,#function
glConvolutionParameterivEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ConvolutionParameteriv), %g2
	or	%g2, %lo(8 * _gloffset_ConvolutionParameteriv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ConvolutionParameteriv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glCopyConvolutionFilter1DEXT
.type glCopyConvolutionFilter1DEXT,#function
glCopyConvolutionFilter1DEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_CopyConvolutionFilter1D), %g2
	or	%g2, %lo(8 * _gloffset_CopyConvolutionFilter1D), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_CopyConvolutionFilter1D)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glCopyConvolutionFilter2DEXT
.type glCopyConvolutionFilter2DEXT,#function
glCopyConvolutionFilter2DEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_CopyConvolutionFilter2D), %g2
	or	%g2, %lo(8 * _gloffset_CopyConvolutionFilter2D), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_CopyConvolutionFilter2D)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetConvolutionFilterEXT
.type glGetConvolutionFilterEXT,#function
glGetConvolutionFilterEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetConvolutionFilterEXT), %g2
	or	%g2, %lo(8 * _gloffset_GetConvolutionFilterEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetConvolutionFilterEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetConvolutionParameterfvEXT
.type glGetConvolutionParameterfvEXT,#function
glGetConvolutionParameterfvEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetConvolutionParameterfvEXT), %g2
	or	%g2, %lo(8 * _gloffset_GetConvolutionParameterfvEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetConvolutionParameterfvEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetConvolutionParameterivEXT
.type glGetConvolutionParameterivEXT,#function
glGetConvolutionParameterivEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetConvolutionParameterivEXT), %g2
	or	%g2, %lo(8 * _gloffset_GetConvolutionParameterivEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetConvolutionParameterivEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetSeparableFilterEXT
.type glGetSeparableFilterEXT,#function
glGetSeparableFilterEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetSeparableFilterEXT), %g2
	or	%g2, %lo(8 * _gloffset_GetSeparableFilterEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetSeparableFilterEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSeparableFilter2DEXT
.type glSeparableFilter2DEXT,#function
glSeparableFilter2DEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SeparableFilter2D), %g2
	or	%g2, %lo(8 * _gloffset_SeparableFilter2D), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SeparableFilter2D)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColorTableSGI
.type glColorTableSGI,#function
glColorTableSGI:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ColorTable), %g2
	or	%g2, %lo(8 * _gloffset_ColorTable), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ColorTable)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColorTableParameterfvSGI
.type glColorTableParameterfvSGI,#function
glColorTableParameterfvSGI:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ColorTableParameterfv), %g2
	or	%g2, %lo(8 * _gloffset_ColorTableParameterfv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ColorTableParameterfv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColorTableParameterivSGI
.type glColorTableParameterivSGI,#function
glColorTableParameterivSGI:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ColorTableParameteriv), %g2
	or	%g2, %lo(8 * _gloffset_ColorTableParameteriv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ColorTableParameteriv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glCopyColorTableSGI
.type glCopyColorTableSGI,#function
glCopyColorTableSGI:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_CopyColorTable), %g2
	or	%g2, %lo(8 * _gloffset_CopyColorTable), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_CopyColorTable)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetColorTableSGI
.type glGetColorTableSGI,#function
glGetColorTableSGI:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetColorTableSGI), %g2
	or	%g2, %lo(8 * _gloffset_GetColorTableSGI), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetColorTableSGI)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetColorTableParameterfvSGI
.type glGetColorTableParameterfvSGI,#function
glGetColorTableParameterfvSGI:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetColorTableParameterfvSGI), %g2
	or	%g2, %lo(8 * _gloffset_GetColorTableParameterfvSGI), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetColorTableParameterfvSGI)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetColorTableParameterivSGI
.type glGetColorTableParameterivSGI,#function
glGetColorTableParameterivSGI:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetColorTableParameterivSGI), %g2
	or	%g2, %lo(8 * _gloffset_GetColorTableParameterivSGI), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetColorTableParameterivSGI)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glPixelTexGenSGIX
.type glPixelTexGenSGIX,#function
glPixelTexGenSGIX:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_PixelTexGenSGIX), %g2
	or	%g2, %lo(8 * _gloffset_PixelTexGenSGIX), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_PixelTexGenSGIX)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glPixelTexGenParameteriSGIS
.type glPixelTexGenParameteriSGIS,#function
glPixelTexGenParameteriSGIS:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_PixelTexGenParameteriSGIS), %g2
	or	%g2, %lo(8 * _gloffset_PixelTexGenParameteriSGIS), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_PixelTexGenParameteriSGIS)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glPixelTexGenParameterivSGIS
.type glPixelTexGenParameterivSGIS,#function
glPixelTexGenParameterivSGIS:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_PixelTexGenParameterivSGIS), %g2
	or	%g2, %lo(8 * _gloffset_PixelTexGenParameterivSGIS), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_PixelTexGenParameterivSGIS)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glPixelTexGenParameterfSGIS
.type glPixelTexGenParameterfSGIS,#function
glPixelTexGenParameterfSGIS:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_PixelTexGenParameterfSGIS), %g2
	or	%g2, %lo(8 * _gloffset_PixelTexGenParameterfSGIS), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_PixelTexGenParameterfSGIS)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glPixelTexGenParameterfvSGIS
.type glPixelTexGenParameterfvSGIS,#function
glPixelTexGenParameterfvSGIS:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_PixelTexGenParameterfvSGIS), %g2
	or	%g2, %lo(8 * _gloffset_PixelTexGenParameterfvSGIS), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_PixelTexGenParameterfvSGIS)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetPixelTexGenParameterivSGIS
.type glGetPixelTexGenParameterivSGIS,#function
glGetPixelTexGenParameterivSGIS:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetPixelTexGenParameterivSGIS), %g2
	or	%g2, %lo(8 * _gloffset_GetPixelTexGenParameterivSGIS), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetPixelTexGenParameterivSGIS)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetPixelTexGenParameterfvSGIS
.type glGetPixelTexGenParameterfvSGIS,#function
glGetPixelTexGenParameterfvSGIS:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetPixelTexGenParameterfvSGIS), %g2
	or	%g2, %lo(8 * _gloffset_GetPixelTexGenParameterfvSGIS), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetPixelTexGenParameterfvSGIS)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexImage4DSGIS
.type glTexImage4DSGIS,#function
glTexImage4DSGIS:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexImage4DSGIS), %g2
	or	%g2, %lo(8 * _gloffset_TexImage4DSGIS), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexImage4DSGIS)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexSubImage4DSGIS
.type glTexSubImage4DSGIS,#function
glTexSubImage4DSGIS:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexSubImage4DSGIS), %g2
	or	%g2, %lo(8 * _gloffset_TexSubImage4DSGIS), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexSubImage4DSGIS)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glAreTexturesResidentEXT
.type glAreTexturesResidentEXT,#function
glAreTexturesResidentEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_AreTexturesResidentEXT), %g2
	or	%g2, %lo(8 * _gloffset_AreTexturesResidentEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_AreTexturesResidentEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glBindTextureEXT
.type glBindTextureEXT,#function
glBindTextureEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_BindTexture), %g2
	or	%g2, %lo(8 * _gloffset_BindTexture), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_BindTexture)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glDeleteTexturesEXT
.type glDeleteTexturesEXT,#function
glDeleteTexturesEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_DeleteTextures), %g2
	or	%g2, %lo(8 * _gloffset_DeleteTextures), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_DeleteTextures)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGenTexturesEXT
.type glGenTexturesEXT,#function
glGenTexturesEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GenTexturesEXT), %g2
	or	%g2, %lo(8 * _gloffset_GenTexturesEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GenTexturesEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glIsTextureEXT
.type glIsTextureEXT,#function
glIsTextureEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_IsTextureEXT), %g2
	or	%g2, %lo(8 * _gloffset_IsTextureEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_IsTextureEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glPrioritizeTexturesEXT
.type glPrioritizeTexturesEXT,#function
glPrioritizeTexturesEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_PrioritizeTextures), %g2
	or	%g2, %lo(8 * _gloffset_PrioritizeTextures), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_PrioritizeTextures)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glDetailTexFuncSGIS
.type glDetailTexFuncSGIS,#function
glDetailTexFuncSGIS:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_DetailTexFuncSGIS), %g2
	or	%g2, %lo(8 * _gloffset_DetailTexFuncSGIS), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_DetailTexFuncSGIS)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetDetailTexFuncSGIS
.type glGetDetailTexFuncSGIS,#function
glGetDetailTexFuncSGIS:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetDetailTexFuncSGIS), %g2
	or	%g2, %lo(8 * _gloffset_GetDetailTexFuncSGIS), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetDetailTexFuncSGIS)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSharpenTexFuncSGIS
.type glSharpenTexFuncSGIS,#function
glSharpenTexFuncSGIS:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SharpenTexFuncSGIS), %g2
	or	%g2, %lo(8 * _gloffset_SharpenTexFuncSGIS), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SharpenTexFuncSGIS)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetSharpenTexFuncSGIS
.type glGetSharpenTexFuncSGIS,#function
glGetSharpenTexFuncSGIS:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetSharpenTexFuncSGIS), %g2
	or	%g2, %lo(8 * _gloffset_GetSharpenTexFuncSGIS), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetSharpenTexFuncSGIS)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSampleMaskSGIS
.type glSampleMaskSGIS,#function
glSampleMaskSGIS:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SampleMaskSGIS), %g2
	or	%g2, %lo(8 * _gloffset_SampleMaskSGIS), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SampleMaskSGIS)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSamplePatternSGIS
.type glSamplePatternSGIS,#function
glSamplePatternSGIS:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SamplePatternSGIS), %g2
	or	%g2, %lo(8 * _gloffset_SamplePatternSGIS), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SamplePatternSGIS)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glArrayElementEXT
.type glArrayElementEXT,#function
glArrayElementEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ArrayElement), %g2
	or	%g2, %lo(8 * _gloffset_ArrayElement), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ArrayElement)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColorPointerEXT
.type glColorPointerEXT,#function
glColorPointerEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ColorPointerEXT), %g2
	or	%g2, %lo(8 * _gloffset_ColorPointerEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ColorPointerEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glDrawArraysEXT
.type glDrawArraysEXT,#function
glDrawArraysEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_DrawArrays), %g2
	or	%g2, %lo(8 * _gloffset_DrawArrays), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_DrawArrays)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glEdgeFlagPointerEXT
.type glEdgeFlagPointerEXT,#function
glEdgeFlagPointerEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_EdgeFlagPointerEXT), %g2
	or	%g2, %lo(8 * _gloffset_EdgeFlagPointerEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_EdgeFlagPointerEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetPointervEXT
.type glGetPointervEXT,#function
glGetPointervEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetPointerv), %g2
	or	%g2, %lo(8 * _gloffset_GetPointerv), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetPointerv)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glIndexPointerEXT
.type glIndexPointerEXT,#function
glIndexPointerEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_IndexPointerEXT), %g2
	or	%g2, %lo(8 * _gloffset_IndexPointerEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_IndexPointerEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glNormalPointerEXT
.type glNormalPointerEXT,#function
glNormalPointerEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_NormalPointerEXT), %g2
	or	%g2, %lo(8 * _gloffset_NormalPointerEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_NormalPointerEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTexCoordPointerEXT
.type glTexCoordPointerEXT,#function
glTexCoordPointerEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TexCoordPointerEXT), %g2
	or	%g2, %lo(8 * _gloffset_TexCoordPointerEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TexCoordPointerEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexPointerEXT
.type glVertexPointerEXT,#function
glVertexPointerEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexPointerEXT), %g2
	or	%g2, %lo(8 * _gloffset_VertexPointerEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexPointerEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glBlendEquationEXT
.type glBlendEquationEXT,#function
glBlendEquationEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_BlendEquation), %g2
	or	%g2, %lo(8 * _gloffset_BlendEquation), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_BlendEquation)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSpriteParameterfSGIX
.type glSpriteParameterfSGIX,#function
glSpriteParameterfSGIX:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SpriteParameterfSGIX), %g2
	or	%g2, %lo(8 * _gloffset_SpriteParameterfSGIX), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SpriteParameterfSGIX)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSpriteParameterfvSGIX
.type glSpriteParameterfvSGIX,#function
glSpriteParameterfvSGIX:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SpriteParameterfvSGIX), %g2
	or	%g2, %lo(8 * _gloffset_SpriteParameterfvSGIX), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SpriteParameterfvSGIX)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSpriteParameteriSGIX
.type glSpriteParameteriSGIX,#function
glSpriteParameteriSGIX:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SpriteParameteriSGIX), %g2
	or	%g2, %lo(8 * _gloffset_SpriteParameteriSGIX), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SpriteParameteriSGIX)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSpriteParameterivSGIX
.type glSpriteParameterivSGIX,#function
glSpriteParameterivSGIX:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SpriteParameterivSGIX), %g2
	or	%g2, %lo(8 * _gloffset_SpriteParameterivSGIX), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SpriteParameterivSGIX)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glPointParameterfEXT
.type glPointParameterfEXT,#function
glPointParameterfEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_PointParameterfEXT), %g2
	or	%g2, %lo(8 * _gloffset_PointParameterfEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_PointParameterfEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glPointParameterfvEXT
.type glPointParameterfvEXT,#function
glPointParameterfvEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_PointParameterfvEXT), %g2
	or	%g2, %lo(8 * _gloffset_PointParameterfvEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_PointParameterfvEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glPointParameterfARB
.type glPointParameterfARB,#function
glPointParameterfARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_PointParameterfEXT), %g2
	or	%g2, %lo(8 * _gloffset_PointParameterfEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_PointParameterfEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glPointParameterfvARB
.type glPointParameterfvARB,#function
glPointParameterfvARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_PointParameterfvEXT), %g2
	or	%g2, %lo(8 * _gloffset_PointParameterfvEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_PointParameterfvEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glPointParameterfSGIS
.type glPointParameterfSGIS,#function
glPointParameterfSGIS:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_PointParameterfEXT), %g2
	or	%g2, %lo(8 * _gloffset_PointParameterfEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_PointParameterfEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glPointParameterfvSGIS
.type glPointParameterfvSGIS,#function
glPointParameterfvSGIS:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_PointParameterfvEXT), %g2
	or	%g2, %lo(8 * _gloffset_PointParameterfvEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_PointParameterfvEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetInstrumentsSGIX
.type glGetInstrumentsSGIX,#function
glGetInstrumentsSGIX:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetInstrumentsSGIX), %g2
	or	%g2, %lo(8 * _gloffset_GetInstrumentsSGIX), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetInstrumentsSGIX)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glInstrumentsBufferSGIX
.type glInstrumentsBufferSGIX,#function
glInstrumentsBufferSGIX:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_InstrumentsBufferSGIX), %g2
	or	%g2, %lo(8 * _gloffset_InstrumentsBufferSGIX), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_InstrumentsBufferSGIX)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glPollInstrumentsSGIX
.type glPollInstrumentsSGIX,#function
glPollInstrumentsSGIX:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_PollInstrumentsSGIX), %g2
	or	%g2, %lo(8 * _gloffset_PollInstrumentsSGIX), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_PollInstrumentsSGIX)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glReadInstrumentsSGIX
.type glReadInstrumentsSGIX,#function
glReadInstrumentsSGIX:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ReadInstrumentsSGIX), %g2
	or	%g2, %lo(8 * _gloffset_ReadInstrumentsSGIX), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ReadInstrumentsSGIX)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glStartInstrumentsSGIX
.type glStartInstrumentsSGIX,#function
glStartInstrumentsSGIX:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_StartInstrumentsSGIX), %g2
	or	%g2, %lo(8 * _gloffset_StartInstrumentsSGIX), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_StartInstrumentsSGIX)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glStopInstrumentsSGIX
.type glStopInstrumentsSGIX,#function
glStopInstrumentsSGIX:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_StopInstrumentsSGIX), %g2
	or	%g2, %lo(8 * _gloffset_StopInstrumentsSGIX), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_StopInstrumentsSGIX)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glFrameZoomSGIX
.type glFrameZoomSGIX,#function
glFrameZoomSGIX:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_FrameZoomSGIX), %g2
	or	%g2, %lo(8 * _gloffset_FrameZoomSGIX), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_FrameZoomSGIX)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTagSampleBufferSGIX
.type glTagSampleBufferSGIX,#function
glTagSampleBufferSGIX:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TagSampleBufferSGIX), %g2
	or	%g2, %lo(8 * _gloffset_TagSampleBufferSGIX), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TagSampleBufferSGIX)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glReferencePlaneSGIX
.type glReferencePlaneSGIX,#function
glReferencePlaneSGIX:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ReferencePlaneSGIX), %g2
	or	%g2, %lo(8 * _gloffset_ReferencePlaneSGIX), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ReferencePlaneSGIX)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glFlushRasterSGIX
.type glFlushRasterSGIX,#function
glFlushRasterSGIX:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_FlushRasterSGIX), %g2
	or	%g2, %lo(8 * _gloffset_FlushRasterSGIX), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_FlushRasterSGIX)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColorSubTableEXT
.type glColorSubTableEXT,#function
glColorSubTableEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ColorSubTable), %g2
	or	%g2, %lo(8 * _gloffset_ColorSubTable), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ColorSubTable)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glCopyColorSubTableEXT
.type glCopyColorSubTableEXT,#function
glCopyColorSubTableEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_CopyColorSubTable), %g2
	or	%g2, %lo(8 * _gloffset_CopyColorSubTable), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_CopyColorSubTable)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glHintPGI
.type glHintPGI,#function
glHintPGI:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_HintPGI), %g2
	or	%g2, %lo(8 * _gloffset_HintPGI), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_HintPGI)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glColorTableEXT
.type glColorTableEXT,#function
glColorTableEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ColorTable), %g2
	or	%g2, %lo(8 * _gloffset_ColorTable), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ColorTable)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetColorTableEXT
.type glGetColorTableEXT,#function
glGetColorTableEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetColorTableEXT), %g2
	or	%g2, %lo(8 * _gloffset_GetColorTableEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetColorTableEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetColorTableParameterivEXT
.type glGetColorTableParameterivEXT,#function
glGetColorTableParameterivEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetColorTableParameterivEXT), %g2
	or	%g2, %lo(8 * _gloffset_GetColorTableParameterivEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetColorTableParameterivEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetColorTableParameterfvEXT
.type glGetColorTableParameterfvEXT,#function
glGetColorTableParameterfvEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetColorTableParameterfvEXT), %g2
	or	%g2, %lo(8 * _gloffset_GetColorTableParameterfvEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetColorTableParameterfvEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetListParameterfvSGIX
.type glGetListParameterfvSGIX,#function
glGetListParameterfvSGIX:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetListParameterfvSGIX), %g2
	or	%g2, %lo(8 * _gloffset_GetListParameterfvSGIX), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetListParameterfvSGIX)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetListParameterivSGIX
.type glGetListParameterivSGIX,#function
glGetListParameterivSGIX:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetListParameterivSGIX), %g2
	or	%g2, %lo(8 * _gloffset_GetListParameterivSGIX), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetListParameterivSGIX)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glListParameterfSGIX
.type glListParameterfSGIX,#function
glListParameterfSGIX:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ListParameterfSGIX), %g2
	or	%g2, %lo(8 * _gloffset_ListParameterfSGIX), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ListParameterfSGIX)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glListParameterfvSGIX
.type glListParameterfvSGIX,#function
glListParameterfvSGIX:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ListParameterfvSGIX), %g2
	or	%g2, %lo(8 * _gloffset_ListParameterfvSGIX), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ListParameterfvSGIX)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glListParameteriSGIX
.type glListParameteriSGIX,#function
glListParameteriSGIX:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ListParameteriSGIX), %g2
	or	%g2, %lo(8 * _gloffset_ListParameteriSGIX), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ListParameteriSGIX)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glListParameterivSGIX
.type glListParameterivSGIX,#function
glListParameterivSGIX:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ListParameterivSGIX), %g2
	or	%g2, %lo(8 * _gloffset_ListParameterivSGIX), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ListParameterivSGIX)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glIndexMaterialEXT
.type glIndexMaterialEXT,#function
glIndexMaterialEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_IndexMaterialEXT), %g2
	or	%g2, %lo(8 * _gloffset_IndexMaterialEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_IndexMaterialEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glIndexFuncEXT
.type glIndexFuncEXT,#function
glIndexFuncEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_IndexFuncEXT), %g2
	or	%g2, %lo(8 * _gloffset_IndexFuncEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_IndexFuncEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glLockArraysEXT
.type glLockArraysEXT,#function
glLockArraysEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_LockArraysEXT), %g2
	or	%g2, %lo(8 * _gloffset_LockArraysEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_LockArraysEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glUnlockArraysEXT
.type glUnlockArraysEXT,#function
glUnlockArraysEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_UnlockArraysEXT), %g2
	or	%g2, %lo(8 * _gloffset_UnlockArraysEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_UnlockArraysEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glCullParameterdvEXT
.type glCullParameterdvEXT,#function
glCullParameterdvEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_CullParameterdvEXT), %g2
	or	%g2, %lo(8 * _gloffset_CullParameterdvEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_CullParameterdvEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glCullParameterfvEXT
.type glCullParameterfvEXT,#function
glCullParameterfvEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_CullParameterfvEXT), %g2
	or	%g2, %lo(8 * _gloffset_CullParameterfvEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_CullParameterfvEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glFragmentColorMaterialSGIX
.type glFragmentColorMaterialSGIX,#function
glFragmentColorMaterialSGIX:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_FragmentColorMaterialSGIX), %g2
	or	%g2, %lo(8 * _gloffset_FragmentColorMaterialSGIX), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_FragmentColorMaterialSGIX)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glFragmentLightfSGIX
.type glFragmentLightfSGIX,#function
glFragmentLightfSGIX:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_FragmentLightfSGIX), %g2
	or	%g2, %lo(8 * _gloffset_FragmentLightfSGIX), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_FragmentLightfSGIX)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glFragmentLightfvSGIX
.type glFragmentLightfvSGIX,#function
glFragmentLightfvSGIX:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_FragmentLightfvSGIX), %g2
	or	%g2, %lo(8 * _gloffset_FragmentLightfvSGIX), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_FragmentLightfvSGIX)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glFragmentLightiSGIX
.type glFragmentLightiSGIX,#function
glFragmentLightiSGIX:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_FragmentLightiSGIX), %g2
	or	%g2, %lo(8 * _gloffset_FragmentLightiSGIX), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_FragmentLightiSGIX)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glFragmentLightivSGIX
.type glFragmentLightivSGIX,#function
glFragmentLightivSGIX:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_FragmentLightivSGIX), %g2
	or	%g2, %lo(8 * _gloffset_FragmentLightivSGIX), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_FragmentLightivSGIX)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glFragmentLightModelfSGIX
.type glFragmentLightModelfSGIX,#function
glFragmentLightModelfSGIX:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_FragmentLightModelfSGIX), %g2
	or	%g2, %lo(8 * _gloffset_FragmentLightModelfSGIX), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_FragmentLightModelfSGIX)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glFragmentLightModelfvSGIX
.type glFragmentLightModelfvSGIX,#function
glFragmentLightModelfvSGIX:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_FragmentLightModelfvSGIX), %g2
	or	%g2, %lo(8 * _gloffset_FragmentLightModelfvSGIX), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_FragmentLightModelfvSGIX)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glFragmentLightModeliSGIX
.type glFragmentLightModeliSGIX,#function
glFragmentLightModeliSGIX:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_FragmentLightModeliSGIX), %g2
	or	%g2, %lo(8 * _gloffset_FragmentLightModeliSGIX), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_FragmentLightModeliSGIX)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glFragmentLightModelivSGIX
.type glFragmentLightModelivSGIX,#function
glFragmentLightModelivSGIX:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_FragmentLightModelivSGIX), %g2
	or	%g2, %lo(8 * _gloffset_FragmentLightModelivSGIX), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_FragmentLightModelivSGIX)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glFragmentMaterialfSGIX
.type glFragmentMaterialfSGIX,#function
glFragmentMaterialfSGIX:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_FragmentMaterialfSGIX), %g2
	or	%g2, %lo(8 * _gloffset_FragmentMaterialfSGIX), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_FragmentMaterialfSGIX)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glFragmentMaterialfvSGIX
.type glFragmentMaterialfvSGIX,#function
glFragmentMaterialfvSGIX:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_FragmentMaterialfvSGIX), %g2
	or	%g2, %lo(8 * _gloffset_FragmentMaterialfvSGIX), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_FragmentMaterialfvSGIX)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glFragmentMaterialiSGIX
.type glFragmentMaterialiSGIX,#function
glFragmentMaterialiSGIX:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_FragmentMaterialiSGIX), %g2
	or	%g2, %lo(8 * _gloffset_FragmentMaterialiSGIX), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_FragmentMaterialiSGIX)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glFragmentMaterialivSGIX
.type glFragmentMaterialivSGIX,#function
glFragmentMaterialivSGIX:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_FragmentMaterialivSGIX), %g2
	or	%g2, %lo(8 * _gloffset_FragmentMaterialivSGIX), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_FragmentMaterialivSGIX)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetFragmentLightfvSGIX
.type glGetFragmentLightfvSGIX,#function
glGetFragmentLightfvSGIX:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetFragmentLightfvSGIX), %g2
	or	%g2, %lo(8 * _gloffset_GetFragmentLightfvSGIX), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetFragmentLightfvSGIX)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetFragmentLightivSGIX
.type glGetFragmentLightivSGIX,#function
glGetFragmentLightivSGIX:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetFragmentLightivSGIX), %g2
	or	%g2, %lo(8 * _gloffset_GetFragmentLightivSGIX), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetFragmentLightivSGIX)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetFragmentMaterialfvSGIX
.type glGetFragmentMaterialfvSGIX,#function
glGetFragmentMaterialfvSGIX:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetFragmentMaterialfvSGIX), %g2
	or	%g2, %lo(8 * _gloffset_GetFragmentMaterialfvSGIX), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetFragmentMaterialfvSGIX)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetFragmentMaterialivSGIX
.type glGetFragmentMaterialivSGIX,#function
glGetFragmentMaterialivSGIX:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetFragmentMaterialivSGIX), %g2
	or	%g2, %lo(8 * _gloffset_GetFragmentMaterialivSGIX), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetFragmentMaterialivSGIX)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glLightEnviSGIX
.type glLightEnviSGIX,#function
glLightEnviSGIX:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_LightEnviSGIX), %g2
	or	%g2, %lo(8 * _gloffset_LightEnviSGIX), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_LightEnviSGIX)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glDrawRangeElementsEXT
.type glDrawRangeElementsEXT,#function
glDrawRangeElementsEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_DrawRangeElements), %g2
	or	%g2, %lo(8 * _gloffset_DrawRangeElements), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_DrawRangeElements)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSecondaryColor3bEXT
.type glSecondaryColor3bEXT,#function
glSecondaryColor3bEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SecondaryColor3bEXT), %g2
	or	%g2, %lo(8 * _gloffset_SecondaryColor3bEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SecondaryColor3bEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSecondaryColor3bvEXT
.type glSecondaryColor3bvEXT,#function
glSecondaryColor3bvEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SecondaryColor3bvEXT), %g2
	or	%g2, %lo(8 * _gloffset_SecondaryColor3bvEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SecondaryColor3bvEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSecondaryColor3dEXT
.type glSecondaryColor3dEXT,#function
glSecondaryColor3dEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SecondaryColor3dEXT), %g2
	or	%g2, %lo(8 * _gloffset_SecondaryColor3dEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SecondaryColor3dEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSecondaryColor3dvEXT
.type glSecondaryColor3dvEXT,#function
glSecondaryColor3dvEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SecondaryColor3dvEXT), %g2
	or	%g2, %lo(8 * _gloffset_SecondaryColor3dvEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SecondaryColor3dvEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSecondaryColor3fEXT
.type glSecondaryColor3fEXT,#function
glSecondaryColor3fEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SecondaryColor3fEXT), %g2
	or	%g2, %lo(8 * _gloffset_SecondaryColor3fEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SecondaryColor3fEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSecondaryColor3fvEXT
.type glSecondaryColor3fvEXT,#function
glSecondaryColor3fvEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SecondaryColor3fvEXT), %g2
	or	%g2, %lo(8 * _gloffset_SecondaryColor3fvEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SecondaryColor3fvEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSecondaryColor3iEXT
.type glSecondaryColor3iEXT,#function
glSecondaryColor3iEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SecondaryColor3iEXT), %g2
	or	%g2, %lo(8 * _gloffset_SecondaryColor3iEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SecondaryColor3iEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSecondaryColor3ivEXT
.type glSecondaryColor3ivEXT,#function
glSecondaryColor3ivEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SecondaryColor3ivEXT), %g2
	or	%g2, %lo(8 * _gloffset_SecondaryColor3ivEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SecondaryColor3ivEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSecondaryColor3sEXT
.type glSecondaryColor3sEXT,#function
glSecondaryColor3sEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SecondaryColor3sEXT), %g2
	or	%g2, %lo(8 * _gloffset_SecondaryColor3sEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SecondaryColor3sEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSecondaryColor3svEXT
.type glSecondaryColor3svEXT,#function
glSecondaryColor3svEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SecondaryColor3svEXT), %g2
	or	%g2, %lo(8 * _gloffset_SecondaryColor3svEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SecondaryColor3svEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSecondaryColor3ubEXT
.type glSecondaryColor3ubEXT,#function
glSecondaryColor3ubEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SecondaryColor3ubEXT), %g2
	or	%g2, %lo(8 * _gloffset_SecondaryColor3ubEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SecondaryColor3ubEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSecondaryColor3ubvEXT
.type glSecondaryColor3ubvEXT,#function
glSecondaryColor3ubvEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SecondaryColor3ubvEXT), %g2
	or	%g2, %lo(8 * _gloffset_SecondaryColor3ubvEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SecondaryColor3ubvEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSecondaryColor3uiEXT
.type glSecondaryColor3uiEXT,#function
glSecondaryColor3uiEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SecondaryColor3uiEXT), %g2
	or	%g2, %lo(8 * _gloffset_SecondaryColor3uiEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SecondaryColor3uiEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSecondaryColor3uivEXT
.type glSecondaryColor3uivEXT,#function
glSecondaryColor3uivEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SecondaryColor3uivEXT), %g2
	or	%g2, %lo(8 * _gloffset_SecondaryColor3uivEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SecondaryColor3uivEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSecondaryColor3usEXT
.type glSecondaryColor3usEXT,#function
glSecondaryColor3usEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SecondaryColor3usEXT), %g2
	or	%g2, %lo(8 * _gloffset_SecondaryColor3usEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SecondaryColor3usEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSecondaryColor3usvEXT
.type glSecondaryColor3usvEXT,#function
glSecondaryColor3usvEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SecondaryColor3usvEXT), %g2
	or	%g2, %lo(8 * _gloffset_SecondaryColor3usvEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SecondaryColor3usvEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSecondaryColorPointerEXT
.type glSecondaryColorPointerEXT,#function
glSecondaryColorPointerEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SecondaryColorPointerEXT), %g2
	or	%g2, %lo(8 * _gloffset_SecondaryColorPointerEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SecondaryColorPointerEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiDrawArraysEXT
.type glMultiDrawArraysEXT,#function
glMultiDrawArraysEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiDrawArraysEXT), %g2
	or	%g2, %lo(8 * _gloffset_MultiDrawArraysEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiDrawArraysEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glMultiDrawElementsEXT
.type glMultiDrawElementsEXT,#function
glMultiDrawElementsEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_MultiDrawElementsEXT), %g2
	or	%g2, %lo(8 * _gloffset_MultiDrawElementsEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_MultiDrawElementsEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glFogCoordfEXT
.type glFogCoordfEXT,#function
glFogCoordfEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_FogCoordfEXT), %g2
	or	%g2, %lo(8 * _gloffset_FogCoordfEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_FogCoordfEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glFogCoordfvEXT
.type glFogCoordfvEXT,#function
glFogCoordfvEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_FogCoordfvEXT), %g2
	or	%g2, %lo(8 * _gloffset_FogCoordfvEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_FogCoordfvEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glFogCoorddEXT
.type glFogCoorddEXT,#function
glFogCoorddEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_FogCoorddEXT), %g2
	or	%g2, %lo(8 * _gloffset_FogCoorddEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_FogCoorddEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glFogCoorddvEXT
.type glFogCoorddvEXT,#function
glFogCoorddvEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_FogCoorddvEXT), %g2
	or	%g2, %lo(8 * _gloffset_FogCoorddvEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_FogCoorddvEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glFogCoordPointerEXT
.type glFogCoordPointerEXT,#function
glFogCoordPointerEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_FogCoordPointerEXT), %g2
	or	%g2, %lo(8 * _gloffset_FogCoordPointerEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_FogCoordPointerEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glBlendFuncSeparateEXT
.type glBlendFuncSeparateEXT,#function
glBlendFuncSeparateEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_BlendFuncSeparateEXT), %g2
	or	%g2, %lo(8 * _gloffset_BlendFuncSeparateEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_BlendFuncSeparateEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glBlendFuncSeparateINGR
.type glBlendFuncSeparateINGR,#function
glBlendFuncSeparateINGR:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_BlendFuncSeparateEXT), %g2
	or	%g2, %lo(8 * _gloffset_BlendFuncSeparateEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_BlendFuncSeparateEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexWeightfEXT
.type glVertexWeightfEXT,#function
glVertexWeightfEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexWeightfEXT), %g2
	or	%g2, %lo(8 * _gloffset_VertexWeightfEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexWeightfEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexWeightfvEXT
.type glVertexWeightfvEXT,#function
glVertexWeightfvEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexWeightfvEXT), %g2
	or	%g2, %lo(8 * _gloffset_VertexWeightfvEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexWeightfvEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexWeightPointerEXT
.type glVertexWeightPointerEXT,#function
glVertexWeightPointerEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexWeightPointerEXT), %g2
	or	%g2, %lo(8 * _gloffset_VertexWeightPointerEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexWeightPointerEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glFlushVertexArrayRangeNV
.type glFlushVertexArrayRangeNV,#function
glFlushVertexArrayRangeNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_FlushVertexArrayRangeNV), %g2
	or	%g2, %lo(8 * _gloffset_FlushVertexArrayRangeNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_FlushVertexArrayRangeNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexArrayRangeNV
.type glVertexArrayRangeNV,#function
glVertexArrayRangeNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexArrayRangeNV), %g2
	or	%g2, %lo(8 * _gloffset_VertexArrayRangeNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexArrayRangeNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glCombinerParameterfvNV
.type glCombinerParameterfvNV,#function
glCombinerParameterfvNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_CombinerParameterfvNV), %g2
	or	%g2, %lo(8 * _gloffset_CombinerParameterfvNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_CombinerParameterfvNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glCombinerParameterfNV
.type glCombinerParameterfNV,#function
glCombinerParameterfNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_CombinerParameterfNV), %g2
	or	%g2, %lo(8 * _gloffset_CombinerParameterfNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_CombinerParameterfNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glCombinerParameterivNV
.type glCombinerParameterivNV,#function
glCombinerParameterivNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_CombinerParameterivNV), %g2
	or	%g2, %lo(8 * _gloffset_CombinerParameterivNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_CombinerParameterivNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glCombinerParameteriNV
.type glCombinerParameteriNV,#function
glCombinerParameteriNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_CombinerParameteriNV), %g2
	or	%g2, %lo(8 * _gloffset_CombinerParameteriNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_CombinerParameteriNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glCombinerInputNV
.type glCombinerInputNV,#function
glCombinerInputNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_CombinerInputNV), %g2
	or	%g2, %lo(8 * _gloffset_CombinerInputNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_CombinerInputNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glCombinerOutputNV
.type glCombinerOutputNV,#function
glCombinerOutputNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_CombinerOutputNV), %g2
	or	%g2, %lo(8 * _gloffset_CombinerOutputNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_CombinerOutputNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glFinalCombinerInputNV
.type glFinalCombinerInputNV,#function
glFinalCombinerInputNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_FinalCombinerInputNV), %g2
	or	%g2, %lo(8 * _gloffset_FinalCombinerInputNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_FinalCombinerInputNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetCombinerInputParameterfvNV
.type glGetCombinerInputParameterfvNV,#function
glGetCombinerInputParameterfvNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetCombinerInputParameterfvNV), %g2
	or	%g2, %lo(8 * _gloffset_GetCombinerInputParameterfvNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetCombinerInputParameterfvNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetCombinerInputParameterivNV
.type glGetCombinerInputParameterivNV,#function
glGetCombinerInputParameterivNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetCombinerInputParameterivNV), %g2
	or	%g2, %lo(8 * _gloffset_GetCombinerInputParameterivNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetCombinerInputParameterivNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetCombinerOutputParameterfvNV
.type glGetCombinerOutputParameterfvNV,#function
glGetCombinerOutputParameterfvNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetCombinerOutputParameterfvNV), %g2
	or	%g2, %lo(8 * _gloffset_GetCombinerOutputParameterfvNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetCombinerOutputParameterfvNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetCombinerOutputParameterivNV
.type glGetCombinerOutputParameterivNV,#function
glGetCombinerOutputParameterivNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetCombinerOutputParameterivNV), %g2
	or	%g2, %lo(8 * _gloffset_GetCombinerOutputParameterivNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetCombinerOutputParameterivNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetFinalCombinerInputParameterfvNV
.type glGetFinalCombinerInputParameterfvNV,#function
glGetFinalCombinerInputParameterfvNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetFinalCombinerInputParameterfvNV), %g2
	or	%g2, %lo(8 * _gloffset_GetFinalCombinerInputParameterfvNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetFinalCombinerInputParameterfvNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetFinalCombinerInputParameterivNV
.type glGetFinalCombinerInputParameterivNV,#function
glGetFinalCombinerInputParameterivNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetFinalCombinerInputParameterivNV), %g2
	or	%g2, %lo(8 * _gloffset_GetFinalCombinerInputParameterivNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetFinalCombinerInputParameterivNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glResizeBuffersMESA
.type glResizeBuffersMESA,#function
glResizeBuffersMESA:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ResizeBuffersMESA), %g2
	or	%g2, %lo(8 * _gloffset_ResizeBuffersMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ResizeBuffersMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos2dMESA
.type glWindowPos2dMESA,#function
glWindowPos2dMESA:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos2dMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos2dMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos2dMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos2dvMESA
.type glWindowPos2dvMESA,#function
glWindowPos2dvMESA:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos2dvMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos2dvMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos2dvMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos2fMESA
.type glWindowPos2fMESA,#function
glWindowPos2fMESA:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos2fMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos2fMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos2fMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos2fvMESA
.type glWindowPos2fvMESA,#function
glWindowPos2fvMESA:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos2fvMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos2fvMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos2fvMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos2iMESA
.type glWindowPos2iMESA,#function
glWindowPos2iMESA:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos2iMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos2iMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos2iMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos2ivMESA
.type glWindowPos2ivMESA,#function
glWindowPos2ivMESA:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos2ivMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos2ivMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos2ivMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos2sMESA
.type glWindowPos2sMESA,#function
glWindowPos2sMESA:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos2sMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos2sMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos2sMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos2svMESA
.type glWindowPos2svMESA,#function
glWindowPos2svMESA:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos2svMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos2svMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos2svMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos3dMESA
.type glWindowPos3dMESA,#function
glWindowPos3dMESA:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos3dMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos3dMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos3dMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos3dvMESA
.type glWindowPos3dvMESA,#function
glWindowPos3dvMESA:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos3dvMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos3dvMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos3dvMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos3fMESA
.type glWindowPos3fMESA,#function
glWindowPos3fMESA:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos3fMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos3fMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos3fMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos3fvMESA
.type glWindowPos3fvMESA,#function
glWindowPos3fvMESA:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos3fvMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos3fvMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos3fvMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos3iMESA
.type glWindowPos3iMESA,#function
glWindowPos3iMESA:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos3iMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos3iMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos3iMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos3ivMESA
.type glWindowPos3ivMESA,#function
glWindowPos3ivMESA:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos3ivMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos3ivMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos3ivMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos3sMESA
.type glWindowPos3sMESA,#function
glWindowPos3sMESA:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos3sMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos3sMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos3sMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos3svMESA
.type glWindowPos3svMESA,#function
glWindowPos3svMESA:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos3svMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos3svMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos3svMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos4dMESA
.type glWindowPos4dMESA,#function
glWindowPos4dMESA:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos4dMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos4dMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos4dMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos4dvMESA
.type glWindowPos4dvMESA,#function
glWindowPos4dvMESA:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos4dvMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos4dvMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos4dvMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos4fMESA
.type glWindowPos4fMESA,#function
glWindowPos4fMESA:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos4fMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos4fMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos4fMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos4fvMESA
.type glWindowPos4fvMESA,#function
glWindowPos4fvMESA:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos4fvMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos4fvMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos4fvMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos4iMESA
.type glWindowPos4iMESA,#function
glWindowPos4iMESA:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos4iMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos4iMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos4iMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos4ivMESA
.type glWindowPos4ivMESA,#function
glWindowPos4ivMESA:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos4ivMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos4ivMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos4ivMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos4sMESA
.type glWindowPos4sMESA,#function
glWindowPos4sMESA:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos4sMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos4sMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos4sMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos4svMESA
.type glWindowPos4svMESA,#function
glWindowPos4svMESA:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos4svMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos4svMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos4svMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTbufferMask3DFX
.type glTbufferMask3DFX,#function
glTbufferMask3DFX:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TbufferMask3DFX), %g2
	or	%g2, %lo(8 * _gloffset_TbufferMask3DFX), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TbufferMask3DFX)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSampleMaskEXT
.type glSampleMaskEXT,#function
glSampleMaskEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SampleMaskSGIS), %g2
	or	%g2, %lo(8 * _gloffset_SampleMaskSGIS), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SampleMaskSGIS)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSamplePatternEXT
.type glSamplePatternEXT,#function
glSamplePatternEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SamplePatternSGIS), %g2
	or	%g2, %lo(8 * _gloffset_SamplePatternSGIS), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SamplePatternSGIS)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glDeleteFencesNV
.type glDeleteFencesNV,#function
glDeleteFencesNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_DeleteFencesNV), %g2
	or	%g2, %lo(8 * _gloffset_DeleteFencesNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_DeleteFencesNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGenFencesNV
.type glGenFencesNV,#function
glGenFencesNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GenFencesNV), %g2
	or	%g2, %lo(8 * _gloffset_GenFencesNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GenFencesNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glIsFenceNV
.type glIsFenceNV,#function
glIsFenceNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_IsFenceNV), %g2
	or	%g2, %lo(8 * _gloffset_IsFenceNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_IsFenceNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTestFenceNV
.type glTestFenceNV,#function
glTestFenceNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TestFenceNV), %g2
	or	%g2, %lo(8 * _gloffset_TestFenceNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TestFenceNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetFenceivNV
.type glGetFenceivNV,#function
glGetFenceivNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetFenceivNV), %g2
	or	%g2, %lo(8 * _gloffset_GetFenceivNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetFenceivNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glFinishFenceNV
.type glFinishFenceNV,#function
glFinishFenceNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_FinishFenceNV), %g2
	or	%g2, %lo(8 * _gloffset_FinishFenceNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_FinishFenceNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSetFenceNV
.type glSetFenceNV,#function
glSetFenceNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SetFenceNV), %g2
	or	%g2, %lo(8 * _gloffset_SetFenceNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SetFenceNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos2dARB
.type glWindowPos2dARB,#function
glWindowPos2dARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos2dMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos2dMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos2dMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos2fARB
.type glWindowPos2fARB,#function
glWindowPos2fARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos2fMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos2fMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos2fMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos2iARB
.type glWindowPos2iARB,#function
glWindowPos2iARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos2iMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos2iMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos2iMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos2sARB
.type glWindowPos2sARB,#function
glWindowPos2sARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos2sMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos2sMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos2sMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos2dvARB
.type glWindowPos2dvARB,#function
glWindowPos2dvARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos2dvMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos2dvMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos2dvMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos2fvARB
.type glWindowPos2fvARB,#function
glWindowPos2fvARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos2fvMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos2fvMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos2fvMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos2ivARB
.type glWindowPos2ivARB,#function
glWindowPos2ivARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos2ivMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos2ivMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos2ivMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos2svARB
.type glWindowPos2svARB,#function
glWindowPos2svARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos2svMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos2svMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos2svMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos3dARB
.type glWindowPos3dARB,#function
glWindowPos3dARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos3dMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos3dMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos3dMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos3fARB
.type glWindowPos3fARB,#function
glWindowPos3fARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos3fMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos3fMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos3fMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos3iARB
.type glWindowPos3iARB,#function
glWindowPos3iARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos3iMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos3iMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos3iMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos3sARB
.type glWindowPos3sARB,#function
glWindowPos3sARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos3sMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos3sMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos3sMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos3dvARB
.type glWindowPos3dvARB,#function
glWindowPos3dvARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos3dvMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos3dvMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos3dvMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos3fvARB
.type glWindowPos3fvARB,#function
glWindowPos3fvARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos3fvMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos3fvMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos3fvMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos3ivARB
.type glWindowPos3ivARB,#function
glWindowPos3ivARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos3ivMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos3ivMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos3ivMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos3svARB
.type glWindowPos3svARB,#function
glWindowPos3svARB:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos3svMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos3svMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos3svMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glAreProgramsResidentNV
.type glAreProgramsResidentNV,#function
glAreProgramsResidentNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_AreProgramsResidentNV), %g2
	or	%g2, %lo(8 * _gloffset_AreProgramsResidentNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_AreProgramsResidentNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glBindProgramNV
.type glBindProgramNV,#function
glBindProgramNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_BindProgramNV), %g2
	or	%g2, %lo(8 * _gloffset_BindProgramNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_BindProgramNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glDeleteProgramsNV
.type glDeleteProgramsNV,#function
glDeleteProgramsNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_DeleteProgramsNV), %g2
	or	%g2, %lo(8 * _gloffset_DeleteProgramsNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_DeleteProgramsNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glExecuteProgramNV
.type glExecuteProgramNV,#function
glExecuteProgramNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ExecuteProgramNV), %g2
	or	%g2, %lo(8 * _gloffset_ExecuteProgramNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ExecuteProgramNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGenProgramsNV
.type glGenProgramsNV,#function
glGenProgramsNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GenProgramsNV), %g2
	or	%g2, %lo(8 * _gloffset_GenProgramsNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GenProgramsNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetProgramParameterdvNV
.type glGetProgramParameterdvNV,#function
glGetProgramParameterdvNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetProgramParameterdvNV), %g2
	or	%g2, %lo(8 * _gloffset_GetProgramParameterdvNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetProgramParameterdvNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetProgramParameterfvNV
.type glGetProgramParameterfvNV,#function
glGetProgramParameterfvNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetProgramParameterfvNV), %g2
	or	%g2, %lo(8 * _gloffset_GetProgramParameterfvNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetProgramParameterfvNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetProgramivNV
.type glGetProgramivNV,#function
glGetProgramivNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetProgramivNV), %g2
	or	%g2, %lo(8 * _gloffset_GetProgramivNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetProgramivNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetProgramStringNV
.type glGetProgramStringNV,#function
glGetProgramStringNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetProgramStringNV), %g2
	or	%g2, %lo(8 * _gloffset_GetProgramStringNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetProgramStringNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetTrackMatrixivNV
.type glGetTrackMatrixivNV,#function
glGetTrackMatrixivNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetTrackMatrixivNV), %g2
	or	%g2, %lo(8 * _gloffset_GetTrackMatrixivNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetTrackMatrixivNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetVertexAttribdvNV
.type glGetVertexAttribdvNV,#function
glGetVertexAttribdvNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetVertexAttribdvNV), %g2
	or	%g2, %lo(8 * _gloffset_GetVertexAttribdvNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetVertexAttribdvNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetVertexAttribfvNV
.type glGetVertexAttribfvNV,#function
glGetVertexAttribfvNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetVertexAttribfvNV), %g2
	or	%g2, %lo(8 * _gloffset_GetVertexAttribfvNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetVertexAttribfvNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetVertexAttribivNV
.type glGetVertexAttribivNV,#function
glGetVertexAttribivNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetVertexAttribivNV), %g2
	or	%g2, %lo(8 * _gloffset_GetVertexAttribivNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetVertexAttribivNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glGetVertexAttribPointervNV
.type glGetVertexAttribPointervNV,#function
glGetVertexAttribPointervNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_GetVertexAttribPointervNV), %g2
	or	%g2, %lo(8 * _gloffset_GetVertexAttribPointervNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_GetVertexAttribPointervNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glIsProgramNV
.type glIsProgramNV,#function
glIsProgramNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_IsProgramNV), %g2
	or	%g2, %lo(8 * _gloffset_IsProgramNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_IsProgramNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glLoadProgramNV
.type glLoadProgramNV,#function
glLoadProgramNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_LoadProgramNV), %g2
	or	%g2, %lo(8 * _gloffset_LoadProgramNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_LoadProgramNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glProgramParameter4dNV
.type glProgramParameter4dNV,#function
glProgramParameter4dNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ProgramParameter4dNV), %g2
	or	%g2, %lo(8 * _gloffset_ProgramParameter4dNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ProgramParameter4dNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glProgramParameter4dvNV
.type glProgramParameter4dvNV,#function
glProgramParameter4dvNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ProgramParameter4dvNV), %g2
	or	%g2, %lo(8 * _gloffset_ProgramParameter4dvNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ProgramParameter4dvNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glProgramParameter4fNV
.type glProgramParameter4fNV,#function
glProgramParameter4fNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ProgramParameter4fNV), %g2
	or	%g2, %lo(8 * _gloffset_ProgramParameter4fNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ProgramParameter4fNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glProgramParameter4fvNV
.type glProgramParameter4fvNV,#function
glProgramParameter4fvNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ProgramParameter4fvNV), %g2
	or	%g2, %lo(8 * _gloffset_ProgramParameter4fvNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ProgramParameter4fvNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glProgramParameters4dvNV
.type glProgramParameters4dvNV,#function
glProgramParameters4dvNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ProgramParameters4dvNV), %g2
	or	%g2, %lo(8 * _gloffset_ProgramParameters4dvNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ProgramParameters4dvNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glProgramParameters4fvNV
.type glProgramParameters4fvNV,#function
glProgramParameters4fvNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ProgramParameters4fvNV), %g2
	or	%g2, %lo(8 * _gloffset_ProgramParameters4fvNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ProgramParameters4fvNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glRequestResidentProgramsNV
.type glRequestResidentProgramsNV,#function
glRequestResidentProgramsNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_RequestResidentProgramsNV), %g2
	or	%g2, %lo(8 * _gloffset_RequestResidentProgramsNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_RequestResidentProgramsNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glTrackMatrixNV
.type glTrackMatrixNV,#function
glTrackMatrixNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_TrackMatrixNV), %g2
	or	%g2, %lo(8 * _gloffset_TrackMatrixNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_TrackMatrixNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexAttribPointerNV
.type glVertexAttribPointerNV,#function
glVertexAttribPointerNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexAttribPointerNV), %g2
	or	%g2, %lo(8 * _gloffset_VertexAttribPointerNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexAttribPointerNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexAttrib1dNV
.type glVertexAttrib1dNV,#function
glVertexAttrib1dNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexAttrib1dNV), %g2
	or	%g2, %lo(8 * _gloffset_VertexAttrib1dNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexAttrib1dNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexAttrib1dvNV
.type glVertexAttrib1dvNV,#function
glVertexAttrib1dvNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexAttrib1dvNV), %g2
	or	%g2, %lo(8 * _gloffset_VertexAttrib1dvNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexAttrib1dvNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexAttrib1fNV
.type glVertexAttrib1fNV,#function
glVertexAttrib1fNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexAttrib1fNV), %g2
	or	%g2, %lo(8 * _gloffset_VertexAttrib1fNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexAttrib1fNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexAttrib1fvNV
.type glVertexAttrib1fvNV,#function
glVertexAttrib1fvNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexAttrib1fvNV), %g2
	or	%g2, %lo(8 * _gloffset_VertexAttrib1fvNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexAttrib1fvNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexAttrib1sNV
.type glVertexAttrib1sNV,#function
glVertexAttrib1sNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexAttrib1sNV), %g2
	or	%g2, %lo(8 * _gloffset_VertexAttrib1sNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexAttrib1sNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexAttrib1svNV
.type glVertexAttrib1svNV,#function
glVertexAttrib1svNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexAttrib1svNV), %g2
	or	%g2, %lo(8 * _gloffset_VertexAttrib1svNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexAttrib1svNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexAttrib2dNV
.type glVertexAttrib2dNV,#function
glVertexAttrib2dNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexAttrib2dNV), %g2
	or	%g2, %lo(8 * _gloffset_VertexAttrib2dNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexAttrib2dNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexAttrib2dvNV
.type glVertexAttrib2dvNV,#function
glVertexAttrib2dvNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexAttrib2dvNV), %g2
	or	%g2, %lo(8 * _gloffset_VertexAttrib2dvNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexAttrib2dvNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexAttrib2fNV
.type glVertexAttrib2fNV,#function
glVertexAttrib2fNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexAttrib2fNV), %g2
	or	%g2, %lo(8 * _gloffset_VertexAttrib2fNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexAttrib2fNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexAttrib2fvNV
.type glVertexAttrib2fvNV,#function
glVertexAttrib2fvNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexAttrib2fvNV), %g2
	or	%g2, %lo(8 * _gloffset_VertexAttrib2fvNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexAttrib2fvNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexAttrib2sNV
.type glVertexAttrib2sNV,#function
glVertexAttrib2sNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexAttrib2sNV), %g2
	or	%g2, %lo(8 * _gloffset_VertexAttrib2sNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexAttrib2sNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexAttrib2svNV
.type glVertexAttrib2svNV,#function
glVertexAttrib2svNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexAttrib2svNV), %g2
	or	%g2, %lo(8 * _gloffset_VertexAttrib2svNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexAttrib2svNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexAttrib3dNV
.type glVertexAttrib3dNV,#function
glVertexAttrib3dNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexAttrib3dNV), %g2
	or	%g2, %lo(8 * _gloffset_VertexAttrib3dNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexAttrib3dNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexAttrib3dvNV
.type glVertexAttrib3dvNV,#function
glVertexAttrib3dvNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexAttrib3dvNV), %g2
	or	%g2, %lo(8 * _gloffset_VertexAttrib3dvNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexAttrib3dvNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexAttrib3fNV
.type glVertexAttrib3fNV,#function
glVertexAttrib3fNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexAttrib3fNV), %g2
	or	%g2, %lo(8 * _gloffset_VertexAttrib3fNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexAttrib3fNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexAttrib3fvNV
.type glVertexAttrib3fvNV,#function
glVertexAttrib3fvNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexAttrib3fvNV), %g2
	or	%g2, %lo(8 * _gloffset_VertexAttrib3fvNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexAttrib3fvNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexAttrib3sNV
.type glVertexAttrib3sNV,#function
glVertexAttrib3sNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexAttrib3sNV), %g2
	or	%g2, %lo(8 * _gloffset_VertexAttrib3sNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexAttrib3sNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexAttrib3svNV
.type glVertexAttrib3svNV,#function
glVertexAttrib3svNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexAttrib3svNV), %g2
	or	%g2, %lo(8 * _gloffset_VertexAttrib3svNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexAttrib3svNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexAttrib4dNV
.type glVertexAttrib4dNV,#function
glVertexAttrib4dNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexAttrib4dNV), %g2
	or	%g2, %lo(8 * _gloffset_VertexAttrib4dNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexAttrib4dNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexAttrib4dvNV
.type glVertexAttrib4dvNV,#function
glVertexAttrib4dvNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexAttrib4dvNV), %g2
	or	%g2, %lo(8 * _gloffset_VertexAttrib4dvNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexAttrib4dvNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexAttrib4fNV
.type glVertexAttrib4fNV,#function
glVertexAttrib4fNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexAttrib4fNV), %g2
	or	%g2, %lo(8 * _gloffset_VertexAttrib4fNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexAttrib4fNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexAttrib4fvNV
.type glVertexAttrib4fvNV,#function
glVertexAttrib4fvNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexAttrib4fvNV), %g2
	or	%g2, %lo(8 * _gloffset_VertexAttrib4fvNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexAttrib4fvNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexAttrib4sNV
.type glVertexAttrib4sNV,#function
glVertexAttrib4sNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexAttrib4sNV), %g2
	or	%g2, %lo(8 * _gloffset_VertexAttrib4sNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexAttrib4sNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexAttrib4svNV
.type glVertexAttrib4svNV,#function
glVertexAttrib4svNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexAttrib4svNV), %g2
	or	%g2, %lo(8 * _gloffset_VertexAttrib4svNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexAttrib4svNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexAttrib4ubNV
.type glVertexAttrib4ubNV,#function
glVertexAttrib4ubNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexAttrib4ubNV), %g2
	or	%g2, %lo(8 * _gloffset_VertexAttrib4ubNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexAttrib4ubNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexAttrib4ubvNV
.type glVertexAttrib4ubvNV,#function
glVertexAttrib4ubvNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexAttrib4ubvNV), %g2
	or	%g2, %lo(8 * _gloffset_VertexAttrib4ubvNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexAttrib4ubvNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexAttribs1dvNV
.type glVertexAttribs1dvNV,#function
glVertexAttribs1dvNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexAttribs1dvNV), %g2
	or	%g2, %lo(8 * _gloffset_VertexAttribs1dvNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexAttribs1dvNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexAttribs1fvNV
.type glVertexAttribs1fvNV,#function
glVertexAttribs1fvNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexAttribs1fvNV), %g2
	or	%g2, %lo(8 * _gloffset_VertexAttribs1fvNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexAttribs1fvNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexAttribs1svNV
.type glVertexAttribs1svNV,#function
glVertexAttribs1svNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexAttribs1svNV), %g2
	or	%g2, %lo(8 * _gloffset_VertexAttribs1svNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexAttribs1svNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexAttribs2dvNV
.type glVertexAttribs2dvNV,#function
glVertexAttribs2dvNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexAttribs2dvNV), %g2
	or	%g2, %lo(8 * _gloffset_VertexAttribs2dvNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexAttribs2dvNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexAttribs2fvNV
.type glVertexAttribs2fvNV,#function
glVertexAttribs2fvNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexAttribs2fvNV), %g2
	or	%g2, %lo(8 * _gloffset_VertexAttribs2fvNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexAttribs2fvNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexAttribs2svNV
.type glVertexAttribs2svNV,#function
glVertexAttribs2svNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexAttribs2svNV), %g2
	or	%g2, %lo(8 * _gloffset_VertexAttribs2svNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexAttribs2svNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexAttribs3dvNV
.type glVertexAttribs3dvNV,#function
glVertexAttribs3dvNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexAttribs3dvNV), %g2
	or	%g2, %lo(8 * _gloffset_VertexAttribs3dvNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexAttribs3dvNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexAttribs3fvNV
.type glVertexAttribs3fvNV,#function
glVertexAttribs3fvNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexAttribs3fvNV), %g2
	or	%g2, %lo(8 * _gloffset_VertexAttribs3fvNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexAttribs3fvNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexAttribs3svNV
.type glVertexAttribs3svNV,#function
glVertexAttribs3svNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexAttribs3svNV), %g2
	or	%g2, %lo(8 * _gloffset_VertexAttribs3svNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexAttribs3svNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexAttribs4dvNV
.type glVertexAttribs4dvNV,#function
glVertexAttribs4dvNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexAttribs4dvNV), %g2
	or	%g2, %lo(8 * _gloffset_VertexAttribs4dvNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexAttribs4dvNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexAttribs4fvNV
.type glVertexAttribs4fvNV,#function
glVertexAttribs4fvNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexAttribs4fvNV), %g2
	or	%g2, %lo(8 * _gloffset_VertexAttribs4fvNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexAttribs4fvNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexAttribs4svNV
.type glVertexAttribs4svNV,#function
glVertexAttribs4svNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexAttribs4svNV), %g2
	or	%g2, %lo(8 * _gloffset_VertexAttribs4svNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexAttribs4svNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glVertexAttribs4ubvNV
.type glVertexAttribs4ubvNV,#function
glVertexAttribs4ubvNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_VertexAttribs4ubvNV), %g2
	or	%g2, %lo(8 * _gloffset_VertexAttribs4ubvNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_VertexAttribs4ubvNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glPointParameteriNV
.type glPointParameteriNV,#function
glPointParameteriNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_PointParameteriNV), %g2
	or	%g2, %lo(8 * _gloffset_PointParameteriNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_PointParameteriNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glPointParameterivNV
.type glPointParameterivNV,#function
glPointParameterivNV:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_PointParameterivNV), %g2
	or	%g2, %lo(8 * _gloffset_PointParameterivNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_PointParameterivNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glPointParameterf
.type glPointParameterf,#function
glPointParameterf:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_PointParameterfEXT), %g2
	or	%g2, %lo(8 * _gloffset_PointParameterfEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_PointParameterfEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glPointParameterfv
.type glPointParameterfv,#function
glPointParameterfv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_PointParameterfvEXT), %g2
	or	%g2, %lo(8 * _gloffset_PointParameterfvEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_PointParameterfvEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glPointParameteri
.type glPointParameteri,#function
glPointParameteri:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_PointParameteriNV), %g2
	or	%g2, %lo(8 * _gloffset_PointParameteriNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_PointParameteriNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glPointParameteriv
.type glPointParameteriv,#function
glPointParameteriv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_PointParameterivNV), %g2
	or	%g2, %lo(8 * _gloffset_PointParameterivNV), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_PointParameterivNV)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSecondaryColor3b
.type glSecondaryColor3b,#function
glSecondaryColor3b:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SecondaryColor3bEXT), %g2
	or	%g2, %lo(8 * _gloffset_SecondaryColor3bEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SecondaryColor3bEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSecondaryColor3bv
.type glSecondaryColor3bv,#function
glSecondaryColor3bv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SecondaryColor3bvEXT), %g2
	or	%g2, %lo(8 * _gloffset_SecondaryColor3bvEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SecondaryColor3bvEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSecondaryColor3d
.type glSecondaryColor3d,#function
glSecondaryColor3d:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SecondaryColor3dEXT), %g2
	or	%g2, %lo(8 * _gloffset_SecondaryColor3dEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SecondaryColor3dEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSecondaryColor3dv
.type glSecondaryColor3dv,#function
glSecondaryColor3dv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SecondaryColor3dvEXT), %g2
	or	%g2, %lo(8 * _gloffset_SecondaryColor3dvEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SecondaryColor3dvEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSecondaryColor3f
.type glSecondaryColor3f,#function
glSecondaryColor3f:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SecondaryColor3fEXT), %g2
	or	%g2, %lo(8 * _gloffset_SecondaryColor3fEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SecondaryColor3fEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSecondaryColor3fv
.type glSecondaryColor3fv,#function
glSecondaryColor3fv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SecondaryColor3fvEXT), %g2
	or	%g2, %lo(8 * _gloffset_SecondaryColor3fvEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SecondaryColor3fvEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSecondaryColor3i
.type glSecondaryColor3i,#function
glSecondaryColor3i:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SecondaryColor3iEXT), %g2
	or	%g2, %lo(8 * _gloffset_SecondaryColor3iEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SecondaryColor3iEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSecondaryColor3iv
.type glSecondaryColor3iv,#function
glSecondaryColor3iv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SecondaryColor3ivEXT), %g2
	or	%g2, %lo(8 * _gloffset_SecondaryColor3ivEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SecondaryColor3ivEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSecondaryColor3s
.type glSecondaryColor3s,#function
glSecondaryColor3s:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SecondaryColor3sEXT), %g2
	or	%g2, %lo(8 * _gloffset_SecondaryColor3sEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SecondaryColor3sEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSecondaryColor3sv
.type glSecondaryColor3sv,#function
glSecondaryColor3sv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SecondaryColor3svEXT), %g2
	or	%g2, %lo(8 * _gloffset_SecondaryColor3svEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SecondaryColor3svEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSecondaryColor3ub
.type glSecondaryColor3ub,#function
glSecondaryColor3ub:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SecondaryColor3ubEXT), %g2
	or	%g2, %lo(8 * _gloffset_SecondaryColor3ubEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SecondaryColor3ubEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSecondaryColor3ubv
.type glSecondaryColor3ubv,#function
glSecondaryColor3ubv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SecondaryColor3ubvEXT), %g2
	or	%g2, %lo(8 * _gloffset_SecondaryColor3ubvEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SecondaryColor3ubvEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSecondaryColor3ui
.type glSecondaryColor3ui,#function
glSecondaryColor3ui:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SecondaryColor3uiEXT), %g2
	or	%g2, %lo(8 * _gloffset_SecondaryColor3uiEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SecondaryColor3uiEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSecondaryColor3uiv
.type glSecondaryColor3uiv,#function
glSecondaryColor3uiv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SecondaryColor3uivEXT), %g2
	or	%g2, %lo(8 * _gloffset_SecondaryColor3uivEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SecondaryColor3uivEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSecondaryColor3us
.type glSecondaryColor3us,#function
glSecondaryColor3us:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SecondaryColor3usEXT), %g2
	or	%g2, %lo(8 * _gloffset_SecondaryColor3usEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SecondaryColor3usEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSecondaryColor3usv
.type glSecondaryColor3usv,#function
glSecondaryColor3usv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SecondaryColor3usvEXT), %g2
	or	%g2, %lo(8 * _gloffset_SecondaryColor3usvEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SecondaryColor3usvEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glSecondaryColorPointer
.type glSecondaryColorPointer,#function
glSecondaryColorPointer:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_SecondaryColorPointerEXT), %g2
	or	%g2, %lo(8 * _gloffset_SecondaryColorPointerEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_SecondaryColorPointerEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos2d
.type glWindowPos2d,#function
glWindowPos2d:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos2dMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos2dMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos2dMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos2dv
.type glWindowPos2dv,#function
glWindowPos2dv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos2dvMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos2dvMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos2dvMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos2f
.type glWindowPos2f,#function
glWindowPos2f:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos2fMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos2fMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos2fMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos2fv
.type glWindowPos2fv,#function
glWindowPos2fv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos2fvMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos2fvMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos2fvMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos2i
.type glWindowPos2i,#function
glWindowPos2i:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos2iMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos2iMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos2iMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos2iv
.type glWindowPos2iv,#function
glWindowPos2iv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos2ivMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos2ivMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos2ivMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos2s
.type glWindowPos2s,#function
glWindowPos2s:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos2sMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos2sMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos2sMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos2sv
.type glWindowPos2sv,#function
glWindowPos2sv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos2svMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos2svMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos2svMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos3d
.type glWindowPos3d,#function
glWindowPos3d:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos3dMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos3dMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos3dMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos3dv
.type glWindowPos3dv,#function
glWindowPos3dv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos3dvMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos3dvMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos3dvMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos3f
.type glWindowPos3f,#function
glWindowPos3f:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos3fMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos3fMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos3fMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos3fv
.type glWindowPos3fv,#function
glWindowPos3fv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos3fvMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos3fvMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos3fvMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos3i
.type glWindowPos3i,#function
glWindowPos3i:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos3iMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos3iMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos3iMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos3iv
.type glWindowPos3iv,#function
glWindowPos3iv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos3ivMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos3ivMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos3ivMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos3s
.type glWindowPos3s,#function
glWindowPos3s:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos3sMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos3sMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos3sMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glWindowPos3sv
.type glWindowPos3sv,#function
glWindowPos3sv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos3svMESA), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos3svMESA), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos3svMESA)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glFogCoordf
.type glFogCoordf,#function
glFogCoordf:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_FogCoordfEXT), %g2
	or	%g2, %lo(8 * _gloffset_FogCoordfEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_FogCoordfEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glFogCoordfv
.type glFogCoordfv,#function
glFogCoordfv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_FogCoordfvEXT), %g2
	or	%g2, %lo(8 * _gloffset_FogCoordfvEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_FogCoordfvEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glFogCoordd
.type glFogCoordd,#function
glFogCoordd:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_FogCoorddEXT), %g2
	or	%g2, %lo(8 * _gloffset_FogCoorddEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_FogCoorddEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glFogCoorddv
.type glFogCoorddv,#function
glFogCoorddv:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_FogCoorddvEXT), %g2
	or	%g2, %lo(8 * _gloffset_FogCoorddvEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_FogCoorddvEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glFogCoordPointer
.type glFogCoordPointer,#function
glFogCoordPointer:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_FogCoordPointerEXT), %g2
	or	%g2, %lo(8 * _gloffset_FogCoordPointerEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_FogCoordPointerEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop

.globl glActiveStencilFaceEXT
.type glActiveStencilFaceEXT,#function
glActiveStencilFaceEXT:
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_ActiveStencilFaceEXT), %g2
	or	%g2, %lo(8 * _gloffset_ActiveStencilFaceEXT), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_ActiveStencilFaceEXT)], %g3
#endif
	jmpl	%g3, %g0
	nop
	 nop

.globl _mesa_sparc_glapi_end
.type _mesa_sparc_glapi_end,#function
_mesa_sparc_glapi_end:


/* DO NOT EDIT - This file generated automatically with glsparcasm.py script */
#include "glapioffsets.h"

#define GL_PREFIX(n) gl##n
#define GLOBL_FN(x) .globl x

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

GLOBL_FN(GL_PREFIX(NewList))
.type glNewList,#function
GL_PREFIX(NewList):
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

GLOBL_FN(GL_PREFIX(EndList))
.type glEndList,#function
GL_PREFIX(EndList):
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

GLOBL_FN(GL_PREFIX(CallList))
.type glCallList,#function
GL_PREFIX(CallList):
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

GLOBL_FN(GL_PREFIX(CallLists))
.type glCallLists,#function
GL_PREFIX(CallLists):
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

GLOBL_FN(GL_PREFIX(DeleteLists))
.type glDeleteLists,#function
GL_PREFIX(DeleteLists):
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

GLOBL_FN(GL_PREFIX(GenLists))
.type glGenLists,#function
GL_PREFIX(GenLists):
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

GLOBL_FN(GL_PREFIX(ListBase))
.type glListBase,#function
GL_PREFIX(ListBase):
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

GLOBL_FN(GL_PREFIX(Begin))
.type glBegin,#function
GL_PREFIX(Begin):
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

GLOBL_FN(GL_PREFIX(Bitmap))
.type glBitmap,#function
GL_PREFIX(Bitmap):
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

GLOBL_FN(GL_PREFIX(Color3b))
.type glColor3b,#function
GL_PREFIX(Color3b):
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

GLOBL_FN(GL_PREFIX(Color3bv))
.type glColor3bv,#function
GL_PREFIX(Color3bv):
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

GLOBL_FN(GL_PREFIX(Color3d))
.type glColor3d,#function
GL_PREFIX(Color3d):
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

GLOBL_FN(GL_PREFIX(Color3dv))
.type glColor3dv,#function
GL_PREFIX(Color3dv):
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

GLOBL_FN(GL_PREFIX(Color3f))
.type glColor3f,#function
GL_PREFIX(Color3f):
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

GLOBL_FN(GL_PREFIX(Color3fv))
.type glColor3fv,#function
GL_PREFIX(Color3fv):
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

GLOBL_FN(GL_PREFIX(Color3i))
.type glColor3i,#function
GL_PREFIX(Color3i):
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

GLOBL_FN(GL_PREFIX(Color3iv))
.type glColor3iv,#function
GL_PREFIX(Color3iv):
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

GLOBL_FN(GL_PREFIX(Color3s))
.type glColor3s,#function
GL_PREFIX(Color3s):
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

GLOBL_FN(GL_PREFIX(Color3sv))
.type glColor3sv,#function
GL_PREFIX(Color3sv):
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

GLOBL_FN(GL_PREFIX(Color3ub))
.type glColor3ub,#function
GL_PREFIX(Color3ub):
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

GLOBL_FN(GL_PREFIX(Color3ubv))
.type glColor3ubv,#function
GL_PREFIX(Color3ubv):
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

GLOBL_FN(GL_PREFIX(Color3ui))
.type glColor3ui,#function
GL_PREFIX(Color3ui):
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

GLOBL_FN(GL_PREFIX(Color3uiv))
.type glColor3uiv,#function
GL_PREFIX(Color3uiv):
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

GLOBL_FN(GL_PREFIX(Color3us))
.type glColor3us,#function
GL_PREFIX(Color3us):
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

GLOBL_FN(GL_PREFIX(Color3usv))
.type glColor3usv,#function
GL_PREFIX(Color3usv):
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

GLOBL_FN(GL_PREFIX(Color4b))
.type glColor4b,#function
GL_PREFIX(Color4b):
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

GLOBL_FN(GL_PREFIX(Color4bv))
.type glColor4bv,#function
GL_PREFIX(Color4bv):
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

GLOBL_FN(GL_PREFIX(Color4d))
.type glColor4d,#function
GL_PREFIX(Color4d):
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

GLOBL_FN(GL_PREFIX(Color4dv))
.type glColor4dv,#function
GL_PREFIX(Color4dv):
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

GLOBL_FN(GL_PREFIX(Color4f))
.type glColor4f,#function
GL_PREFIX(Color4f):
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

GLOBL_FN(GL_PREFIX(Color4fv))
.type glColor4fv,#function
GL_PREFIX(Color4fv):
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

GLOBL_FN(GL_PREFIX(Color4i))
.type glColor4i,#function
GL_PREFIX(Color4i):
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

GLOBL_FN(GL_PREFIX(Color4iv))
.type glColor4iv,#function
GL_PREFIX(Color4iv):
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

GLOBL_FN(GL_PREFIX(Color4s))
.type glColor4s,#function
GL_PREFIX(Color4s):
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

GLOBL_FN(GL_PREFIX(Color4sv))
.type glColor4sv,#function
GL_PREFIX(Color4sv):
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

GLOBL_FN(GL_PREFIX(Color4ub))
.type glColor4ub,#function
GL_PREFIX(Color4ub):
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

GLOBL_FN(GL_PREFIX(Color4ubv))
.type glColor4ubv,#function
GL_PREFIX(Color4ubv):
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

GLOBL_FN(GL_PREFIX(Color4ui))
.type glColor4ui,#function
GL_PREFIX(Color4ui):
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

GLOBL_FN(GL_PREFIX(Color4uiv))
.type glColor4uiv,#function
GL_PREFIX(Color4uiv):
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

GLOBL_FN(GL_PREFIX(Color4us))
.type glColor4us,#function
GL_PREFIX(Color4us):
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

GLOBL_FN(GL_PREFIX(Color4usv))
.type glColor4usv,#function
GL_PREFIX(Color4usv):
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

GLOBL_FN(GL_PREFIX(EdgeFlag))
.type glEdgeFlag,#function
GL_PREFIX(EdgeFlag):
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

GLOBL_FN(GL_PREFIX(EdgeFlagv))
.type glEdgeFlagv,#function
GL_PREFIX(EdgeFlagv):
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

GLOBL_FN(GL_PREFIX(End))
.type glEnd,#function
GL_PREFIX(End):
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

GLOBL_FN(GL_PREFIX(Indexd))
.type glIndexd,#function
GL_PREFIX(Indexd):
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

GLOBL_FN(GL_PREFIX(Indexdv))
.type glIndexdv,#function
GL_PREFIX(Indexdv):
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

GLOBL_FN(GL_PREFIX(Indexf))
.type glIndexf,#function
GL_PREFIX(Indexf):
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

GLOBL_FN(GL_PREFIX(Indexfv))
.type glIndexfv,#function
GL_PREFIX(Indexfv):
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

GLOBL_FN(GL_PREFIX(Indexi))
.type glIndexi,#function
GL_PREFIX(Indexi):
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

GLOBL_FN(GL_PREFIX(Indexiv))
.type glIndexiv,#function
GL_PREFIX(Indexiv):
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

GLOBL_FN(GL_PREFIX(Indexs))
.type glIndexs,#function
GL_PREFIX(Indexs):
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

GLOBL_FN(GL_PREFIX(Indexsv))
.type glIndexsv,#function
GL_PREFIX(Indexsv):
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

GLOBL_FN(GL_PREFIX(Normal3b))
.type glNormal3b,#function
GL_PREFIX(Normal3b):
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

GLOBL_FN(GL_PREFIX(Normal3bv))
.type glNormal3bv,#function
GL_PREFIX(Normal3bv):
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

GLOBL_FN(GL_PREFIX(Normal3d))
.type glNormal3d,#function
GL_PREFIX(Normal3d):
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

GLOBL_FN(GL_PREFIX(Normal3dv))
.type glNormal3dv,#function
GL_PREFIX(Normal3dv):
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

GLOBL_FN(GL_PREFIX(Normal3f))
.type glNormal3f,#function
GL_PREFIX(Normal3f):
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

GLOBL_FN(GL_PREFIX(Normal3fv))
.type glNormal3fv,#function
GL_PREFIX(Normal3fv):
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

GLOBL_FN(GL_PREFIX(Normal3i))
.type glNormal3i,#function
GL_PREFIX(Normal3i):
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

GLOBL_FN(GL_PREFIX(Normal3iv))
.type glNormal3iv,#function
GL_PREFIX(Normal3iv):
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

GLOBL_FN(GL_PREFIX(Normal3s))
.type glNormal3s,#function
GL_PREFIX(Normal3s):
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

GLOBL_FN(GL_PREFIX(Normal3sv))
.type glNormal3sv,#function
GL_PREFIX(Normal3sv):
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

GLOBL_FN(GL_PREFIX(RasterPos2d))
.type glRasterPos2d,#function
GL_PREFIX(RasterPos2d):
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

GLOBL_FN(GL_PREFIX(RasterPos2dv))
.type glRasterPos2dv,#function
GL_PREFIX(RasterPos2dv):
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

GLOBL_FN(GL_PREFIX(RasterPos2f))
.type glRasterPos2f,#function
GL_PREFIX(RasterPos2f):
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

GLOBL_FN(GL_PREFIX(RasterPos2fv))
.type glRasterPos2fv,#function
GL_PREFIX(RasterPos2fv):
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

GLOBL_FN(GL_PREFIX(RasterPos2i))
.type glRasterPos2i,#function
GL_PREFIX(RasterPos2i):
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

GLOBL_FN(GL_PREFIX(RasterPos2iv))
.type glRasterPos2iv,#function
GL_PREFIX(RasterPos2iv):
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

GLOBL_FN(GL_PREFIX(RasterPos2s))
.type glRasterPos2s,#function
GL_PREFIX(RasterPos2s):
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

GLOBL_FN(GL_PREFIX(RasterPos2sv))
.type glRasterPos2sv,#function
GL_PREFIX(RasterPos2sv):
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

GLOBL_FN(GL_PREFIX(RasterPos3d))
.type glRasterPos3d,#function
GL_PREFIX(RasterPos3d):
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

GLOBL_FN(GL_PREFIX(RasterPos3dv))
.type glRasterPos3dv,#function
GL_PREFIX(RasterPos3dv):
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

GLOBL_FN(GL_PREFIX(RasterPos3f))
.type glRasterPos3f,#function
GL_PREFIX(RasterPos3f):
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

GLOBL_FN(GL_PREFIX(RasterPos3fv))
.type glRasterPos3fv,#function
GL_PREFIX(RasterPos3fv):
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

GLOBL_FN(GL_PREFIX(RasterPos3i))
.type glRasterPos3i,#function
GL_PREFIX(RasterPos3i):
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

GLOBL_FN(GL_PREFIX(RasterPos3iv))
.type glRasterPos3iv,#function
GL_PREFIX(RasterPos3iv):
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

GLOBL_FN(GL_PREFIX(RasterPos3s))
.type glRasterPos3s,#function
GL_PREFIX(RasterPos3s):
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

GLOBL_FN(GL_PREFIX(RasterPos3sv))
.type glRasterPos3sv,#function
GL_PREFIX(RasterPos3sv):
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

GLOBL_FN(GL_PREFIX(RasterPos4d))
.type glRasterPos4d,#function
GL_PREFIX(RasterPos4d):
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

GLOBL_FN(GL_PREFIX(RasterPos4dv))
.type glRasterPos4dv,#function
GL_PREFIX(RasterPos4dv):
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

GLOBL_FN(GL_PREFIX(RasterPos4f))
.type glRasterPos4f,#function
GL_PREFIX(RasterPos4f):
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

GLOBL_FN(GL_PREFIX(RasterPos4fv))
.type glRasterPos4fv,#function
GL_PREFIX(RasterPos4fv):
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

GLOBL_FN(GL_PREFIX(RasterPos4i))
.type glRasterPos4i,#function
GL_PREFIX(RasterPos4i):
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

GLOBL_FN(GL_PREFIX(RasterPos4iv))
.type glRasterPos4iv,#function
GL_PREFIX(RasterPos4iv):
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

GLOBL_FN(GL_PREFIX(RasterPos4s))
.type glRasterPos4s,#function
GL_PREFIX(RasterPos4s):
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

GLOBL_FN(GL_PREFIX(RasterPos4sv))
.type glRasterPos4sv,#function
GL_PREFIX(RasterPos4sv):
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

GLOBL_FN(GL_PREFIX(Rectd))
.type glRectd,#function
GL_PREFIX(Rectd):
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

GLOBL_FN(GL_PREFIX(Rectdv))
.type glRectdv,#function
GL_PREFIX(Rectdv):
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

GLOBL_FN(GL_PREFIX(Rectf))
.type glRectf,#function
GL_PREFIX(Rectf):
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

GLOBL_FN(GL_PREFIX(Rectfv))
.type glRectfv,#function
GL_PREFIX(Rectfv):
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

GLOBL_FN(GL_PREFIX(Recti))
.type glRecti,#function
GL_PREFIX(Recti):
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

GLOBL_FN(GL_PREFIX(Rectiv))
.type glRectiv,#function
GL_PREFIX(Rectiv):
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

GLOBL_FN(GL_PREFIX(Rects))
.type glRects,#function
GL_PREFIX(Rects):
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

GLOBL_FN(GL_PREFIX(Rectsv))
.type glRectsv,#function
GL_PREFIX(Rectsv):
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

GLOBL_FN(GL_PREFIX(TexCoord1d))
.type glTexCoord1d,#function
GL_PREFIX(TexCoord1d):
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

GLOBL_FN(GL_PREFIX(TexCoord1dv))
.type glTexCoord1dv,#function
GL_PREFIX(TexCoord1dv):
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

GLOBL_FN(GL_PREFIX(TexCoord1f))
.type glTexCoord1f,#function
GL_PREFIX(TexCoord1f):
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

GLOBL_FN(GL_PREFIX(TexCoord1fv))
.type glTexCoord1fv,#function
GL_PREFIX(TexCoord1fv):
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

GLOBL_FN(GL_PREFIX(TexCoord1i))
.type glTexCoord1i,#function
GL_PREFIX(TexCoord1i):
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

GLOBL_FN(GL_PREFIX(TexCoord1iv))
.type glTexCoord1iv,#function
GL_PREFIX(TexCoord1iv):
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

GLOBL_FN(GL_PREFIX(TexCoord1s))
.type glTexCoord1s,#function
GL_PREFIX(TexCoord1s):
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

GLOBL_FN(GL_PREFIX(TexCoord1sv))
.type glTexCoord1sv,#function
GL_PREFIX(TexCoord1sv):
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

GLOBL_FN(GL_PREFIX(TexCoord2d))
.type glTexCoord2d,#function
GL_PREFIX(TexCoord2d):
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

GLOBL_FN(GL_PREFIX(TexCoord2dv))
.type glTexCoord2dv,#function
GL_PREFIX(TexCoord2dv):
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

GLOBL_FN(GL_PREFIX(TexCoord2f))
.type glTexCoord2f,#function
GL_PREFIX(TexCoord2f):
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

GLOBL_FN(GL_PREFIX(TexCoord2fv))
.type glTexCoord2fv,#function
GL_PREFIX(TexCoord2fv):
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

GLOBL_FN(GL_PREFIX(TexCoord2i))
.type glTexCoord2i,#function
GL_PREFIX(TexCoord2i):
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

GLOBL_FN(GL_PREFIX(TexCoord2iv))
.type glTexCoord2iv,#function
GL_PREFIX(TexCoord2iv):
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

GLOBL_FN(GL_PREFIX(TexCoord2s))
.type glTexCoord2s,#function
GL_PREFIX(TexCoord2s):
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

GLOBL_FN(GL_PREFIX(TexCoord2sv))
.type glTexCoord2sv,#function
GL_PREFIX(TexCoord2sv):
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

GLOBL_FN(GL_PREFIX(TexCoord3d))
.type glTexCoord3d,#function
GL_PREFIX(TexCoord3d):
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

GLOBL_FN(GL_PREFIX(TexCoord3dv))
.type glTexCoord3dv,#function
GL_PREFIX(TexCoord3dv):
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

GLOBL_FN(GL_PREFIX(TexCoord3f))
.type glTexCoord3f,#function
GL_PREFIX(TexCoord3f):
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

GLOBL_FN(GL_PREFIX(TexCoord3fv))
.type glTexCoord3fv,#function
GL_PREFIX(TexCoord3fv):
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

GLOBL_FN(GL_PREFIX(TexCoord3i))
.type glTexCoord3i,#function
GL_PREFIX(TexCoord3i):
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

GLOBL_FN(GL_PREFIX(TexCoord3iv))
.type glTexCoord3iv,#function
GL_PREFIX(TexCoord3iv):
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

GLOBL_FN(GL_PREFIX(TexCoord3s))
.type glTexCoord3s,#function
GL_PREFIX(TexCoord3s):
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

GLOBL_FN(GL_PREFIX(TexCoord3sv))
.type glTexCoord3sv,#function
GL_PREFIX(TexCoord3sv):
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

GLOBL_FN(GL_PREFIX(TexCoord4d))
.type glTexCoord4d,#function
GL_PREFIX(TexCoord4d):
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

GLOBL_FN(GL_PREFIX(TexCoord4dv))
.type glTexCoord4dv,#function
GL_PREFIX(TexCoord4dv):
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

GLOBL_FN(GL_PREFIX(TexCoord4f))
.type glTexCoord4f,#function
GL_PREFIX(TexCoord4f):
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

GLOBL_FN(GL_PREFIX(TexCoord4fv))
.type glTexCoord4fv,#function
GL_PREFIX(TexCoord4fv):
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

GLOBL_FN(GL_PREFIX(TexCoord4i))
.type glTexCoord4i,#function
GL_PREFIX(TexCoord4i):
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

GLOBL_FN(GL_PREFIX(TexCoord4iv))
.type glTexCoord4iv,#function
GL_PREFIX(TexCoord4iv):
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

GLOBL_FN(GL_PREFIX(TexCoord4s))
.type glTexCoord4s,#function
GL_PREFIX(TexCoord4s):
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

GLOBL_FN(GL_PREFIX(TexCoord4sv))
.type glTexCoord4sv,#function
GL_PREFIX(TexCoord4sv):
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

GLOBL_FN(GL_PREFIX(Vertex2d))
.type glVertex2d,#function
GL_PREFIX(Vertex2d):
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

GLOBL_FN(GL_PREFIX(Vertex2dv))
.type glVertex2dv,#function
GL_PREFIX(Vertex2dv):
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

GLOBL_FN(GL_PREFIX(Vertex2f))
.type glVertex2f,#function
GL_PREFIX(Vertex2f):
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

GLOBL_FN(GL_PREFIX(Vertex2fv))
.type glVertex2fv,#function
GL_PREFIX(Vertex2fv):
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

GLOBL_FN(GL_PREFIX(Vertex2i))
.type glVertex2i,#function
GL_PREFIX(Vertex2i):
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

GLOBL_FN(GL_PREFIX(Vertex2iv))
.type glVertex2iv,#function
GL_PREFIX(Vertex2iv):
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

GLOBL_FN(GL_PREFIX(Vertex2s))
.type glVertex2s,#function
GL_PREFIX(Vertex2s):
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

GLOBL_FN(GL_PREFIX(Vertex2sv))
.type glVertex2sv,#function
GL_PREFIX(Vertex2sv):
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

GLOBL_FN(GL_PREFIX(Vertex3d))
.type glVertex3d,#function
GL_PREFIX(Vertex3d):
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

GLOBL_FN(GL_PREFIX(Vertex3dv))
.type glVertex3dv,#function
GL_PREFIX(Vertex3dv):
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

GLOBL_FN(GL_PREFIX(Vertex3f))
.type glVertex3f,#function
GL_PREFIX(Vertex3f):
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

GLOBL_FN(GL_PREFIX(Vertex3fv))
.type glVertex3fv,#function
GL_PREFIX(Vertex3fv):
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

GLOBL_FN(GL_PREFIX(Vertex3i))
.type glVertex3i,#function
GL_PREFIX(Vertex3i):
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

GLOBL_FN(GL_PREFIX(Vertex3iv))
.type glVertex3iv,#function
GL_PREFIX(Vertex3iv):
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

GLOBL_FN(GL_PREFIX(Vertex3s))
.type glVertex3s,#function
GL_PREFIX(Vertex3s):
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

GLOBL_FN(GL_PREFIX(Vertex3sv))
.type glVertex3sv,#function
GL_PREFIX(Vertex3sv):
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

GLOBL_FN(GL_PREFIX(Vertex4d))
.type glVertex4d,#function
GL_PREFIX(Vertex4d):
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

GLOBL_FN(GL_PREFIX(Vertex4dv))
.type glVertex4dv,#function
GL_PREFIX(Vertex4dv):
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

GLOBL_FN(GL_PREFIX(Vertex4f))
.type glVertex4f,#function
GL_PREFIX(Vertex4f):
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

GLOBL_FN(GL_PREFIX(Vertex4fv))
.type glVertex4fv,#function
GL_PREFIX(Vertex4fv):
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

GLOBL_FN(GL_PREFIX(Vertex4i))
.type glVertex4i,#function
GL_PREFIX(Vertex4i):
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

GLOBL_FN(GL_PREFIX(Vertex4iv))
.type glVertex4iv,#function
GL_PREFIX(Vertex4iv):
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

GLOBL_FN(GL_PREFIX(Vertex4s))
.type glVertex4s,#function
GL_PREFIX(Vertex4s):
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

GLOBL_FN(GL_PREFIX(Vertex4sv))
.type glVertex4sv,#function
GL_PREFIX(Vertex4sv):
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

GLOBL_FN(GL_PREFIX(ClipPlane))
.type glClipPlane,#function
GL_PREFIX(ClipPlane):
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

GLOBL_FN(GL_PREFIX(ColorMaterial))
.type glColorMaterial,#function
GL_PREFIX(ColorMaterial):
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

GLOBL_FN(GL_PREFIX(CullFace))
.type glCullFace,#function
GL_PREFIX(CullFace):
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

GLOBL_FN(GL_PREFIX(Fogf))
.type glFogf,#function
GL_PREFIX(Fogf):
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

GLOBL_FN(GL_PREFIX(Fogfv))
.type glFogfv,#function
GL_PREFIX(Fogfv):
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

GLOBL_FN(GL_PREFIX(Fogi))
.type glFogi,#function
GL_PREFIX(Fogi):
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

GLOBL_FN(GL_PREFIX(Fogiv))
.type glFogiv,#function
GL_PREFIX(Fogiv):
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

GLOBL_FN(GL_PREFIX(FrontFace))
.type glFrontFace,#function
GL_PREFIX(FrontFace):
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

GLOBL_FN(GL_PREFIX(Hint))
.type glHint,#function
GL_PREFIX(Hint):
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

GLOBL_FN(GL_PREFIX(Lightf))
.type glLightf,#function
GL_PREFIX(Lightf):
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

GLOBL_FN(GL_PREFIX(Lightfv))
.type glLightfv,#function
GL_PREFIX(Lightfv):
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

GLOBL_FN(GL_PREFIX(Lighti))
.type glLighti,#function
GL_PREFIX(Lighti):
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

GLOBL_FN(GL_PREFIX(Lightiv))
.type glLightiv,#function
GL_PREFIX(Lightiv):
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

GLOBL_FN(GL_PREFIX(LightModelf))
.type glLightModelf,#function
GL_PREFIX(LightModelf):
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

GLOBL_FN(GL_PREFIX(LightModelfv))
.type glLightModelfv,#function
GL_PREFIX(LightModelfv):
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

GLOBL_FN(GL_PREFIX(LightModeli))
.type glLightModeli,#function
GL_PREFIX(LightModeli):
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

GLOBL_FN(GL_PREFIX(LightModeliv))
.type glLightModeliv,#function
GL_PREFIX(LightModeliv):
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

GLOBL_FN(GL_PREFIX(LineStipple))
.type glLineStipple,#function
GL_PREFIX(LineStipple):
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

GLOBL_FN(GL_PREFIX(LineWidth))
.type glLineWidth,#function
GL_PREFIX(LineWidth):
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

GLOBL_FN(GL_PREFIX(Materialf))
.type glMaterialf,#function
GL_PREFIX(Materialf):
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

GLOBL_FN(GL_PREFIX(Materialfv))
.type glMaterialfv,#function
GL_PREFIX(Materialfv):
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

GLOBL_FN(GL_PREFIX(Materiali))
.type glMateriali,#function
GL_PREFIX(Materiali):
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

GLOBL_FN(GL_PREFIX(Materialiv))
.type glMaterialiv,#function
GL_PREFIX(Materialiv):
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

GLOBL_FN(GL_PREFIX(PointSize))
.type glPointSize,#function
GL_PREFIX(PointSize):
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

GLOBL_FN(GL_PREFIX(PolygonMode))
.type glPolygonMode,#function
GL_PREFIX(PolygonMode):
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

GLOBL_FN(GL_PREFIX(PolygonStipple))
.type glPolygonStipple,#function
GL_PREFIX(PolygonStipple):
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

GLOBL_FN(GL_PREFIX(Scissor))
.type glScissor,#function
GL_PREFIX(Scissor):
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

GLOBL_FN(GL_PREFIX(ShadeModel))
.type glShadeModel,#function
GL_PREFIX(ShadeModel):
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

GLOBL_FN(GL_PREFIX(TexParameterf))
.type glTexParameterf,#function
GL_PREFIX(TexParameterf):
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

GLOBL_FN(GL_PREFIX(TexParameterfv))
.type glTexParameterfv,#function
GL_PREFIX(TexParameterfv):
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

GLOBL_FN(GL_PREFIX(TexParameteri))
.type glTexParameteri,#function
GL_PREFIX(TexParameteri):
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

GLOBL_FN(GL_PREFIX(TexParameteriv))
.type glTexParameteriv,#function
GL_PREFIX(TexParameteriv):
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

GLOBL_FN(GL_PREFIX(TexImage1D))
.type glTexImage1D,#function
GL_PREFIX(TexImage1D):
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

GLOBL_FN(GL_PREFIX(TexImage2D))
.type glTexImage2D,#function
GL_PREFIX(TexImage2D):
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

GLOBL_FN(GL_PREFIX(TexEnvf))
.type glTexEnvf,#function
GL_PREFIX(TexEnvf):
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

GLOBL_FN(GL_PREFIX(TexEnvfv))
.type glTexEnvfv,#function
GL_PREFIX(TexEnvfv):
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

GLOBL_FN(GL_PREFIX(TexEnvi))
.type glTexEnvi,#function
GL_PREFIX(TexEnvi):
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

GLOBL_FN(GL_PREFIX(TexEnviv))
.type glTexEnviv,#function
GL_PREFIX(TexEnviv):
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

GLOBL_FN(GL_PREFIX(TexGend))
.type glTexGend,#function
GL_PREFIX(TexGend):
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

GLOBL_FN(GL_PREFIX(TexGendv))
.type glTexGendv,#function
GL_PREFIX(TexGendv):
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

GLOBL_FN(GL_PREFIX(TexGenf))
.type glTexGenf,#function
GL_PREFIX(TexGenf):
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

GLOBL_FN(GL_PREFIX(TexGenfv))
.type glTexGenfv,#function
GL_PREFIX(TexGenfv):
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

GLOBL_FN(GL_PREFIX(TexGeni))
.type glTexGeni,#function
GL_PREFIX(TexGeni):
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

GLOBL_FN(GL_PREFIX(TexGeniv))
.type glTexGeniv,#function
GL_PREFIX(TexGeniv):
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

GLOBL_FN(GL_PREFIX(FeedbackBuffer))
.type glFeedbackBuffer,#function
GL_PREFIX(FeedbackBuffer):
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

GLOBL_FN(GL_PREFIX(SelectBuffer))
.type glSelectBuffer,#function
GL_PREFIX(SelectBuffer):
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

GLOBL_FN(GL_PREFIX(RenderMode))
.type glRenderMode,#function
GL_PREFIX(RenderMode):
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

GLOBL_FN(GL_PREFIX(InitNames))
.type glInitNames,#function
GL_PREFIX(InitNames):
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

GLOBL_FN(GL_PREFIX(LoadName))
.type glLoadName,#function
GL_PREFIX(LoadName):
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

GLOBL_FN(GL_PREFIX(PassThrough))
.type glPassThrough,#function
GL_PREFIX(PassThrough):
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

GLOBL_FN(GL_PREFIX(PopName))
.type glPopName,#function
GL_PREFIX(PopName):
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

GLOBL_FN(GL_PREFIX(PushName))
.type glPushName,#function
GL_PREFIX(PushName):
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

GLOBL_FN(GL_PREFIX(DrawBuffer))
.type glDrawBuffer,#function
GL_PREFIX(DrawBuffer):
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

GLOBL_FN(GL_PREFIX(Clear))
.type glClear,#function
GL_PREFIX(Clear):
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

GLOBL_FN(GL_PREFIX(ClearAccum))
.type glClearAccum,#function
GL_PREFIX(ClearAccum):
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

GLOBL_FN(GL_PREFIX(ClearIndex))
.type glClearIndex,#function
GL_PREFIX(ClearIndex):
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

GLOBL_FN(GL_PREFIX(ClearColor))
.type glClearColor,#function
GL_PREFIX(ClearColor):
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

GLOBL_FN(GL_PREFIX(ClearStencil))
.type glClearStencil,#function
GL_PREFIX(ClearStencil):
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

GLOBL_FN(GL_PREFIX(ClearDepth))
.type glClearDepth,#function
GL_PREFIX(ClearDepth):
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

GLOBL_FN(GL_PREFIX(StencilMask))
.type glStencilMask,#function
GL_PREFIX(StencilMask):
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

GLOBL_FN(GL_PREFIX(ColorMask))
.type glColorMask,#function
GL_PREFIX(ColorMask):
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

GLOBL_FN(GL_PREFIX(DepthMask))
.type glDepthMask,#function
GL_PREFIX(DepthMask):
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

GLOBL_FN(GL_PREFIX(IndexMask))
.type glIndexMask,#function
GL_PREFIX(IndexMask):
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

GLOBL_FN(GL_PREFIX(Accum))
.type glAccum,#function
GL_PREFIX(Accum):
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

GLOBL_FN(GL_PREFIX(Disable))
.type glDisable,#function
GL_PREFIX(Disable):
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

GLOBL_FN(GL_PREFIX(Enable))
.type glEnable,#function
GL_PREFIX(Enable):
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

GLOBL_FN(GL_PREFIX(Finish))
.type glFinish,#function
GL_PREFIX(Finish):
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

GLOBL_FN(GL_PREFIX(Flush))
.type glFlush,#function
GL_PREFIX(Flush):
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

GLOBL_FN(GL_PREFIX(PopAttrib))
.type glPopAttrib,#function
GL_PREFIX(PopAttrib):
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

GLOBL_FN(GL_PREFIX(PushAttrib))
.type glPushAttrib,#function
GL_PREFIX(PushAttrib):
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

GLOBL_FN(GL_PREFIX(Map1d))
.type glMap1d,#function
GL_PREFIX(Map1d):
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

GLOBL_FN(GL_PREFIX(Map1f))
.type glMap1f,#function
GL_PREFIX(Map1f):
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

GLOBL_FN(GL_PREFIX(Map2d))
.type glMap2d,#function
GL_PREFIX(Map2d):
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

GLOBL_FN(GL_PREFIX(Map2f))
.type glMap2f,#function
GL_PREFIX(Map2f):
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

GLOBL_FN(GL_PREFIX(MapGrid1d))
.type glMapGrid1d,#function
GL_PREFIX(MapGrid1d):
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

GLOBL_FN(GL_PREFIX(MapGrid1f))
.type glMapGrid1f,#function
GL_PREFIX(MapGrid1f):
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

GLOBL_FN(GL_PREFIX(MapGrid2d))
.type glMapGrid2d,#function
GL_PREFIX(MapGrid2d):
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

GLOBL_FN(GL_PREFIX(MapGrid2f))
.type glMapGrid2f,#function
GL_PREFIX(MapGrid2f):
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

GLOBL_FN(GL_PREFIX(EvalCoord1d))
.type glEvalCoord1d,#function
GL_PREFIX(EvalCoord1d):
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

GLOBL_FN(GL_PREFIX(EvalCoord1dv))
.type glEvalCoord1dv,#function
GL_PREFIX(EvalCoord1dv):
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

GLOBL_FN(GL_PREFIX(EvalCoord1f))
.type glEvalCoord1f,#function
GL_PREFIX(EvalCoord1f):
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

GLOBL_FN(GL_PREFIX(EvalCoord1fv))
.type glEvalCoord1fv,#function
GL_PREFIX(EvalCoord1fv):
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

GLOBL_FN(GL_PREFIX(EvalCoord2d))
.type glEvalCoord2d,#function
GL_PREFIX(EvalCoord2d):
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

GLOBL_FN(GL_PREFIX(EvalCoord2dv))
.type glEvalCoord2dv,#function
GL_PREFIX(EvalCoord2dv):
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

GLOBL_FN(GL_PREFIX(EvalCoord2f))
.type glEvalCoord2f,#function
GL_PREFIX(EvalCoord2f):
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

GLOBL_FN(GL_PREFIX(EvalCoord2fv))
.type glEvalCoord2fv,#function
GL_PREFIX(EvalCoord2fv):
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

GLOBL_FN(GL_PREFIX(EvalMesh1))
.type glEvalMesh1,#function
GL_PREFIX(EvalMesh1):
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

GLOBL_FN(GL_PREFIX(EvalPoint1))
.type glEvalPoint1,#function
GL_PREFIX(EvalPoint1):
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

GLOBL_FN(GL_PREFIX(EvalMesh2))
.type glEvalMesh2,#function
GL_PREFIX(EvalMesh2):
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

GLOBL_FN(GL_PREFIX(EvalPoint2))
.type glEvalPoint2,#function
GL_PREFIX(EvalPoint2):
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

GLOBL_FN(GL_PREFIX(AlphaFunc))
.type glAlphaFunc,#function
GL_PREFIX(AlphaFunc):
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

GLOBL_FN(GL_PREFIX(BlendFunc))
.type glBlendFunc,#function
GL_PREFIX(BlendFunc):
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

GLOBL_FN(GL_PREFIX(LogicOp))
.type glLogicOp,#function
GL_PREFIX(LogicOp):
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

GLOBL_FN(GL_PREFIX(StencilFunc))
.type glStencilFunc,#function
GL_PREFIX(StencilFunc):
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

GLOBL_FN(GL_PREFIX(StencilOp))
.type glStencilOp,#function
GL_PREFIX(StencilOp):
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

GLOBL_FN(GL_PREFIX(DepthFunc))
.type glDepthFunc,#function
GL_PREFIX(DepthFunc):
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

GLOBL_FN(GL_PREFIX(PixelZoom))
.type glPixelZoom,#function
GL_PREFIX(PixelZoom):
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

GLOBL_FN(GL_PREFIX(PixelTransferf))
.type glPixelTransferf,#function
GL_PREFIX(PixelTransferf):
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

GLOBL_FN(GL_PREFIX(PixelTransferi))
.type glPixelTransferi,#function
GL_PREFIX(PixelTransferi):
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

GLOBL_FN(GL_PREFIX(PixelStoref))
.type glPixelStoref,#function
GL_PREFIX(PixelStoref):
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

GLOBL_FN(GL_PREFIX(PixelStorei))
.type glPixelStorei,#function
GL_PREFIX(PixelStorei):
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

GLOBL_FN(GL_PREFIX(PixelMapfv))
.type glPixelMapfv,#function
GL_PREFIX(PixelMapfv):
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

GLOBL_FN(GL_PREFIX(PixelMapuiv))
.type glPixelMapuiv,#function
GL_PREFIX(PixelMapuiv):
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

GLOBL_FN(GL_PREFIX(PixelMapusv))
.type glPixelMapusv,#function
GL_PREFIX(PixelMapusv):
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

GLOBL_FN(GL_PREFIX(ReadBuffer))
.type glReadBuffer,#function
GL_PREFIX(ReadBuffer):
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

GLOBL_FN(GL_PREFIX(CopyPixels))
.type glCopyPixels,#function
GL_PREFIX(CopyPixels):
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

GLOBL_FN(GL_PREFIX(ReadPixels))
.type glReadPixels,#function
GL_PREFIX(ReadPixels):
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

GLOBL_FN(GL_PREFIX(DrawPixels))
.type glDrawPixels,#function
GL_PREFIX(DrawPixels):
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

GLOBL_FN(GL_PREFIX(GetBooleanv))
.type glGetBooleanv,#function
GL_PREFIX(GetBooleanv):
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

GLOBL_FN(GL_PREFIX(GetClipPlane))
.type glGetClipPlane,#function
GL_PREFIX(GetClipPlane):
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

GLOBL_FN(GL_PREFIX(GetDoublev))
.type glGetDoublev,#function
GL_PREFIX(GetDoublev):
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

GLOBL_FN(GL_PREFIX(GetError))
.type glGetError,#function
GL_PREFIX(GetError):
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

GLOBL_FN(GL_PREFIX(GetFloatv))
.type glGetFloatv,#function
GL_PREFIX(GetFloatv):
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

GLOBL_FN(GL_PREFIX(GetIntegerv))
.type glGetIntegerv,#function
GL_PREFIX(GetIntegerv):
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

GLOBL_FN(GL_PREFIX(GetLightfv))
.type glGetLightfv,#function
GL_PREFIX(GetLightfv):
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

GLOBL_FN(GL_PREFIX(GetLightiv))
.type glGetLightiv,#function
GL_PREFIX(GetLightiv):
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

GLOBL_FN(GL_PREFIX(GetMapdv))
.type glGetMapdv,#function
GL_PREFIX(GetMapdv):
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

GLOBL_FN(GL_PREFIX(GetMapfv))
.type glGetMapfv,#function
GL_PREFIX(GetMapfv):
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

GLOBL_FN(GL_PREFIX(GetMapiv))
.type glGetMapiv,#function
GL_PREFIX(GetMapiv):
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

GLOBL_FN(GL_PREFIX(GetMaterialfv))
.type glGetMaterialfv,#function
GL_PREFIX(GetMaterialfv):
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

GLOBL_FN(GL_PREFIX(GetMaterialiv))
.type glGetMaterialiv,#function
GL_PREFIX(GetMaterialiv):
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

GLOBL_FN(GL_PREFIX(GetPixelMapfv))
.type glGetPixelMapfv,#function
GL_PREFIX(GetPixelMapfv):
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

GLOBL_FN(GL_PREFIX(GetPixelMapuiv))
.type glGetPixelMapuiv,#function
GL_PREFIX(GetPixelMapuiv):
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

GLOBL_FN(GL_PREFIX(GetPixelMapusv))
.type glGetPixelMapusv,#function
GL_PREFIX(GetPixelMapusv):
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

GLOBL_FN(GL_PREFIX(GetPolygonStipple))
.type glGetPolygonStipple,#function
GL_PREFIX(GetPolygonStipple):
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

GLOBL_FN(GL_PREFIX(GetString))
.type glGetString,#function
GL_PREFIX(GetString):
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

GLOBL_FN(GL_PREFIX(GetTexEnvfv))
.type glGetTexEnvfv,#function
GL_PREFIX(GetTexEnvfv):
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

GLOBL_FN(GL_PREFIX(GetTexEnviv))
.type glGetTexEnviv,#function
GL_PREFIX(GetTexEnviv):
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

GLOBL_FN(GL_PREFIX(GetTexGendv))
.type glGetTexGendv,#function
GL_PREFIX(GetTexGendv):
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

GLOBL_FN(GL_PREFIX(GetTexGenfv))
.type glGetTexGenfv,#function
GL_PREFIX(GetTexGenfv):
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

GLOBL_FN(GL_PREFIX(GetTexGeniv))
.type glGetTexGeniv,#function
GL_PREFIX(GetTexGeniv):
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

GLOBL_FN(GL_PREFIX(GetTexImage))
.type glGetTexImage,#function
GL_PREFIX(GetTexImage):
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

GLOBL_FN(GL_PREFIX(GetTexParameterfv))
.type glGetTexParameterfv,#function
GL_PREFIX(GetTexParameterfv):
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

GLOBL_FN(GL_PREFIX(GetTexParameteriv))
.type glGetTexParameteriv,#function
GL_PREFIX(GetTexParameteriv):
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

GLOBL_FN(GL_PREFIX(GetTexLevelParameterfv))
.type glGetTexLevelParameterfv,#function
GL_PREFIX(GetTexLevelParameterfv):
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

GLOBL_FN(GL_PREFIX(GetTexLevelParameteriv))
.type glGetTexLevelParameteriv,#function
GL_PREFIX(GetTexLevelParameteriv):
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

GLOBL_FN(GL_PREFIX(IsEnabled))
.type glIsEnabled,#function
GL_PREFIX(IsEnabled):
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

GLOBL_FN(GL_PREFIX(IsList))
.type glIsList,#function
GL_PREFIX(IsList):
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

GLOBL_FN(GL_PREFIX(DepthRange))
.type glDepthRange,#function
GL_PREFIX(DepthRange):
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

GLOBL_FN(GL_PREFIX(Frustum))
.type glFrustum,#function
GL_PREFIX(Frustum):
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

GLOBL_FN(GL_PREFIX(LoadIdentity))
.type glLoadIdentity,#function
GL_PREFIX(LoadIdentity):
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

GLOBL_FN(GL_PREFIX(LoadMatrixf))
.type glLoadMatrixf,#function
GL_PREFIX(LoadMatrixf):
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

GLOBL_FN(GL_PREFIX(LoadMatrixd))
.type glLoadMatrixd,#function
GL_PREFIX(LoadMatrixd):
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

GLOBL_FN(GL_PREFIX(MatrixMode))
.type glMatrixMode,#function
GL_PREFIX(MatrixMode):
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

GLOBL_FN(GL_PREFIX(MultMatrixf))
.type glMultMatrixf,#function
GL_PREFIX(MultMatrixf):
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

GLOBL_FN(GL_PREFIX(MultMatrixd))
.type glMultMatrixd,#function
GL_PREFIX(MultMatrixd):
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

GLOBL_FN(GL_PREFIX(Ortho))
.type glOrtho,#function
GL_PREFIX(Ortho):
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

GLOBL_FN(GL_PREFIX(PopMatrix))
.type glPopMatrix,#function
GL_PREFIX(PopMatrix):
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

GLOBL_FN(GL_PREFIX(PushMatrix))
.type glPushMatrix,#function
GL_PREFIX(PushMatrix):
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

GLOBL_FN(GL_PREFIX(Rotated))
.type glRotated,#function
GL_PREFIX(Rotated):
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

GLOBL_FN(GL_PREFIX(Rotatef))
.type glRotatef,#function
GL_PREFIX(Rotatef):
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

GLOBL_FN(GL_PREFIX(Scaled))
.type glScaled,#function
GL_PREFIX(Scaled):
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

GLOBL_FN(GL_PREFIX(Scalef))
.type glScalef,#function
GL_PREFIX(Scalef):
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

GLOBL_FN(GL_PREFIX(Translated))
.type glTranslated,#function
GL_PREFIX(Translated):
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

GLOBL_FN(GL_PREFIX(Translatef))
.type glTranslatef,#function
GL_PREFIX(Translatef):
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

GLOBL_FN(GL_PREFIX(Viewport))
.type glViewport,#function
GL_PREFIX(Viewport):
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

GLOBL_FN(GL_PREFIX(ArrayElement))
.type glArrayElement,#function
GL_PREFIX(ArrayElement):
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

GLOBL_FN(GL_PREFIX(ColorPointer))
.type glColorPointer,#function
GL_PREFIX(ColorPointer):
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

GLOBL_FN(GL_PREFIX(DisableClientState))
.type glDisableClientState,#function
GL_PREFIX(DisableClientState):
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

GLOBL_FN(GL_PREFIX(DrawArrays))
.type glDrawArrays,#function
GL_PREFIX(DrawArrays):
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

GLOBL_FN(GL_PREFIX(DrawElements))
.type glDrawElements,#function
GL_PREFIX(DrawElements):
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

GLOBL_FN(GL_PREFIX(EdgeFlagPointer))
.type glEdgeFlagPointer,#function
GL_PREFIX(EdgeFlagPointer):
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

GLOBL_FN(GL_PREFIX(EnableClientState))
.type glEnableClientState,#function
GL_PREFIX(EnableClientState):
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

GLOBL_FN(GL_PREFIX(GetPointerv))
.type glGetPointerv,#function
GL_PREFIX(GetPointerv):
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

GLOBL_FN(GL_PREFIX(IndexPointer))
.type glIndexPointer,#function
GL_PREFIX(IndexPointer):
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

GLOBL_FN(GL_PREFIX(InterleavedArrays))
.type glInterleavedArrays,#function
GL_PREFIX(InterleavedArrays):
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

GLOBL_FN(GL_PREFIX(NormalPointer))
.type glNormalPointer,#function
GL_PREFIX(NormalPointer):
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

GLOBL_FN(GL_PREFIX(TexCoordPointer))
.type glTexCoordPointer,#function
GL_PREFIX(TexCoordPointer):
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

GLOBL_FN(GL_PREFIX(VertexPointer))
.type glVertexPointer,#function
GL_PREFIX(VertexPointer):
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

GLOBL_FN(GL_PREFIX(PolygonOffset))
.type glPolygonOffset,#function
GL_PREFIX(PolygonOffset):
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

GLOBL_FN(GL_PREFIX(CopyTexImage1D))
.type glCopyTexImage1D,#function
GL_PREFIX(CopyTexImage1D):
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

GLOBL_FN(GL_PREFIX(CopyTexImage2D))
.type glCopyTexImage2D,#function
GL_PREFIX(CopyTexImage2D):
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

GLOBL_FN(GL_PREFIX(CopyTexSubImage1D))
.type glCopyTexSubImage1D,#function
GL_PREFIX(CopyTexSubImage1D):
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

GLOBL_FN(GL_PREFIX(CopyTexSubImage2D))
.type glCopyTexSubImage2D,#function
GL_PREFIX(CopyTexSubImage2D):
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

GLOBL_FN(GL_PREFIX(TexSubImage1D))
.type glTexSubImage1D,#function
GL_PREFIX(TexSubImage1D):
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

GLOBL_FN(GL_PREFIX(TexSubImage2D))
.type glTexSubImage2D,#function
GL_PREFIX(TexSubImage2D):
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

GLOBL_FN(GL_PREFIX(AreTexturesResident))
.type glAreTexturesResident,#function
GL_PREFIX(AreTexturesResident):
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

GLOBL_FN(GL_PREFIX(BindTexture))
.type glBindTexture,#function
GL_PREFIX(BindTexture):
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

GLOBL_FN(GL_PREFIX(DeleteTextures))
.type glDeleteTextures,#function
GL_PREFIX(DeleteTextures):
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

GLOBL_FN(GL_PREFIX(GenTextures))
.type glGenTextures,#function
GL_PREFIX(GenTextures):
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

GLOBL_FN(GL_PREFIX(IsTexture))
.type glIsTexture,#function
GL_PREFIX(IsTexture):
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

GLOBL_FN(GL_PREFIX(PrioritizeTextures))
.type glPrioritizeTextures,#function
GL_PREFIX(PrioritizeTextures):
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

GLOBL_FN(GL_PREFIX(Indexub))
.type glIndexub,#function
GL_PREFIX(Indexub):
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

GLOBL_FN(GL_PREFIX(Indexubv))
.type glIndexubv,#function
GL_PREFIX(Indexubv):
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

GLOBL_FN(GL_PREFIX(PopClientAttrib))
.type glPopClientAttrib,#function
GL_PREFIX(PopClientAttrib):
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

GLOBL_FN(GL_PREFIX(PushClientAttrib))
.type glPushClientAttrib,#function
GL_PREFIX(PushClientAttrib):
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

GLOBL_FN(GL_PREFIX(BlendColor))
.type glBlendColor,#function
GL_PREFIX(BlendColor):
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

GLOBL_FN(GL_PREFIX(BlendEquation))
.type glBlendEquation,#function
GL_PREFIX(BlendEquation):
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

GLOBL_FN(GL_PREFIX(DrawRangeElements))
.type glDrawRangeElements,#function
GL_PREFIX(DrawRangeElements):
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

GLOBL_FN(GL_PREFIX(ColorTable))
.type glColorTable,#function
GL_PREFIX(ColorTable):
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

GLOBL_FN(GL_PREFIX(ColorTableParameterfv))
.type glColorTableParameterfv,#function
GL_PREFIX(ColorTableParameterfv):
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

GLOBL_FN(GL_PREFIX(ColorTableParameteriv))
.type glColorTableParameteriv,#function
GL_PREFIX(ColorTableParameteriv):
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

GLOBL_FN(GL_PREFIX(CopyColorTable))
.type glCopyColorTable,#function
GL_PREFIX(CopyColorTable):
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

GLOBL_FN(GL_PREFIX(GetColorTable))
.type glGetColorTable,#function
GL_PREFIX(GetColorTable):
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

GLOBL_FN(GL_PREFIX(GetColorTableParameterfv))
.type glGetColorTableParameterfv,#function
GL_PREFIX(GetColorTableParameterfv):
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

GLOBL_FN(GL_PREFIX(GetColorTableParameteriv))
.type glGetColorTableParameteriv,#function
GL_PREFIX(GetColorTableParameteriv):
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

GLOBL_FN(GL_PREFIX(ColorSubTable))
.type glColorSubTable,#function
GL_PREFIX(ColorSubTable):
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

GLOBL_FN(GL_PREFIX(CopyColorSubTable))
.type glCopyColorSubTable,#function
GL_PREFIX(CopyColorSubTable):
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

GLOBL_FN(GL_PREFIX(ConvolutionFilter1D))
.type glConvolutionFilter1D,#function
GL_PREFIX(ConvolutionFilter1D):
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

GLOBL_FN(GL_PREFIX(ConvolutionFilter2D))
.type glConvolutionFilter2D,#function
GL_PREFIX(ConvolutionFilter2D):
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

GLOBL_FN(GL_PREFIX(ConvolutionParameterf))
.type glConvolutionParameterf,#function
GL_PREFIX(ConvolutionParameterf):
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

GLOBL_FN(GL_PREFIX(ConvolutionParameterfv))
.type glConvolutionParameterfv,#function
GL_PREFIX(ConvolutionParameterfv):
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

GLOBL_FN(GL_PREFIX(ConvolutionParameteri))
.type glConvolutionParameteri,#function
GL_PREFIX(ConvolutionParameteri):
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

GLOBL_FN(GL_PREFIX(ConvolutionParameteriv))
.type glConvolutionParameteriv,#function
GL_PREFIX(ConvolutionParameteriv):
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

GLOBL_FN(GL_PREFIX(CopyConvolutionFilter1D))
.type glCopyConvolutionFilter1D,#function
GL_PREFIX(CopyConvolutionFilter1D):
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

GLOBL_FN(GL_PREFIX(CopyConvolutionFilter2D))
.type glCopyConvolutionFilter2D,#function
GL_PREFIX(CopyConvolutionFilter2D):
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

GLOBL_FN(GL_PREFIX(GetConvolutionFilter))
.type glGetConvolutionFilter,#function
GL_PREFIX(GetConvolutionFilter):
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

GLOBL_FN(GL_PREFIX(GetConvolutionParameterfv))
.type glGetConvolutionParameterfv,#function
GL_PREFIX(GetConvolutionParameterfv):
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

GLOBL_FN(GL_PREFIX(GetConvolutionParameteriv))
.type glGetConvolutionParameteriv,#function
GL_PREFIX(GetConvolutionParameteriv):
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

GLOBL_FN(GL_PREFIX(GetSeparableFilter))
.type glGetSeparableFilter,#function
GL_PREFIX(GetSeparableFilter):
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

GLOBL_FN(GL_PREFIX(SeparableFilter2D))
.type glSeparableFilter2D,#function
GL_PREFIX(SeparableFilter2D):
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

GLOBL_FN(GL_PREFIX(GetHistogram))
.type glGetHistogram,#function
GL_PREFIX(GetHistogram):
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

GLOBL_FN(GL_PREFIX(GetHistogramParameterfv))
.type glGetHistogramParameterfv,#function
GL_PREFIX(GetHistogramParameterfv):
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

GLOBL_FN(GL_PREFIX(GetHistogramParameteriv))
.type glGetHistogramParameteriv,#function
GL_PREFIX(GetHistogramParameteriv):
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

GLOBL_FN(GL_PREFIX(GetMinmax))
.type glGetMinmax,#function
GL_PREFIX(GetMinmax):
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

GLOBL_FN(GL_PREFIX(GetMinmaxParameterfv))
.type glGetMinmaxParameterfv,#function
GL_PREFIX(GetMinmaxParameterfv):
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

GLOBL_FN(GL_PREFIX(GetMinmaxParameteriv))
.type glGetMinmaxParameteriv,#function
GL_PREFIX(GetMinmaxParameteriv):
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

GLOBL_FN(GL_PREFIX(Histogram))
.type glHistogram,#function
GL_PREFIX(Histogram):
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

GLOBL_FN(GL_PREFIX(Minmax))
.type glMinmax,#function
GL_PREFIX(Minmax):
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

GLOBL_FN(GL_PREFIX(ResetHistogram))
.type glResetHistogram,#function
GL_PREFIX(ResetHistogram):
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

GLOBL_FN(GL_PREFIX(ResetMinmax))
.type glResetMinmax,#function
GL_PREFIX(ResetMinmax):
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

GLOBL_FN(GL_PREFIX(TexImage3D))
.type glTexImage3D,#function
GL_PREFIX(TexImage3D):
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

GLOBL_FN(GL_PREFIX(TexSubImage3D))
.type glTexSubImage3D,#function
GL_PREFIX(TexSubImage3D):
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

GLOBL_FN(GL_PREFIX(CopyTexSubImage3D))
.type glCopyTexSubImage3D,#function
GL_PREFIX(CopyTexSubImage3D):
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

GLOBL_FN(GL_PREFIX(ActiveTextureARB))
.type glActiveTextureARB,#function
GL_PREFIX(ActiveTextureARB):
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

GLOBL_FN(GL_PREFIX(ClientActiveTextureARB))
.type glClientActiveTextureARB,#function
GL_PREFIX(ClientActiveTextureARB):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord1dARB))
.type glMultiTexCoord1dARB,#function
GL_PREFIX(MultiTexCoord1dARB):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord1dvARB))
.type glMultiTexCoord1dvARB,#function
GL_PREFIX(MultiTexCoord1dvARB):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord1fARB))
.type glMultiTexCoord1fARB,#function
GL_PREFIX(MultiTexCoord1fARB):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord1fvARB))
.type glMultiTexCoord1fvARB,#function
GL_PREFIX(MultiTexCoord1fvARB):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord1iARB))
.type glMultiTexCoord1iARB,#function
GL_PREFIX(MultiTexCoord1iARB):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord1ivARB))
.type glMultiTexCoord1ivARB,#function
GL_PREFIX(MultiTexCoord1ivARB):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord1sARB))
.type glMultiTexCoord1sARB,#function
GL_PREFIX(MultiTexCoord1sARB):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord1svARB))
.type glMultiTexCoord1svARB,#function
GL_PREFIX(MultiTexCoord1svARB):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord2dARB))
.type glMultiTexCoord2dARB,#function
GL_PREFIX(MultiTexCoord2dARB):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord2dvARB))
.type glMultiTexCoord2dvARB,#function
GL_PREFIX(MultiTexCoord2dvARB):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord2fARB))
.type glMultiTexCoord2fARB,#function
GL_PREFIX(MultiTexCoord2fARB):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord2fvARB))
.type glMultiTexCoord2fvARB,#function
GL_PREFIX(MultiTexCoord2fvARB):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord2iARB))
.type glMultiTexCoord2iARB,#function
GL_PREFIX(MultiTexCoord2iARB):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord2ivARB))
.type glMultiTexCoord2ivARB,#function
GL_PREFIX(MultiTexCoord2ivARB):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord2sARB))
.type glMultiTexCoord2sARB,#function
GL_PREFIX(MultiTexCoord2sARB):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord2svARB))
.type glMultiTexCoord2svARB,#function
GL_PREFIX(MultiTexCoord2svARB):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord3dARB))
.type glMultiTexCoord3dARB,#function
GL_PREFIX(MultiTexCoord3dARB):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord3dvARB))
.type glMultiTexCoord3dvARB,#function
GL_PREFIX(MultiTexCoord3dvARB):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord3fARB))
.type glMultiTexCoord3fARB,#function
GL_PREFIX(MultiTexCoord3fARB):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord3fvARB))
.type glMultiTexCoord3fvARB,#function
GL_PREFIX(MultiTexCoord3fvARB):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord3iARB))
.type glMultiTexCoord3iARB,#function
GL_PREFIX(MultiTexCoord3iARB):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord3ivARB))
.type glMultiTexCoord3ivARB,#function
GL_PREFIX(MultiTexCoord3ivARB):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord3sARB))
.type glMultiTexCoord3sARB,#function
GL_PREFIX(MultiTexCoord3sARB):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord3svARB))
.type glMultiTexCoord3svARB,#function
GL_PREFIX(MultiTexCoord3svARB):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord4dARB))
.type glMultiTexCoord4dARB,#function
GL_PREFIX(MultiTexCoord4dARB):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord4dvARB))
.type glMultiTexCoord4dvARB,#function
GL_PREFIX(MultiTexCoord4dvARB):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord4fARB))
.type glMultiTexCoord4fARB,#function
GL_PREFIX(MultiTexCoord4fARB):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord4fvARB))
.type glMultiTexCoord4fvARB,#function
GL_PREFIX(MultiTexCoord4fvARB):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord4iARB))
.type glMultiTexCoord4iARB,#function
GL_PREFIX(MultiTexCoord4iARB):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord4ivARB))
.type glMultiTexCoord4ivARB,#function
GL_PREFIX(MultiTexCoord4ivARB):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord4sARB))
.type glMultiTexCoord4sARB,#function
GL_PREFIX(MultiTexCoord4sARB):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord4svARB))
.type glMultiTexCoord4svARB,#function
GL_PREFIX(MultiTexCoord4svARB):
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

GLOBL_FN(GL_PREFIX(LoadTransposeMatrixfARB))
.type glLoadTransposeMatrixfARB,#function
GL_PREFIX(LoadTransposeMatrixfARB):
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

GLOBL_FN(GL_PREFIX(LoadTransposeMatrixdARB))
.type glLoadTransposeMatrixdARB,#function
GL_PREFIX(LoadTransposeMatrixdARB):
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

GLOBL_FN(GL_PREFIX(MultTransposeMatrixfARB))
.type glMultTransposeMatrixfARB,#function
GL_PREFIX(MultTransposeMatrixfARB):
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

GLOBL_FN(GL_PREFIX(MultTransposeMatrixdARB))
.type glMultTransposeMatrixdARB,#function
GL_PREFIX(MultTransposeMatrixdARB):
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

GLOBL_FN(GL_PREFIX(SampleCoverageARB))
.type glSampleCoverageARB,#function
GL_PREFIX(SampleCoverageARB):
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

GLOBL_FN(GL_PREFIX(__unused413))
.type gl__unused413,#function
GL_PREFIX(__unused413):
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

GLOBL_FN(GL_PREFIX(CompressedTexImage3DARB))
.type glCompressedTexImage3DARB,#function
GL_PREFIX(CompressedTexImage3DARB):
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

GLOBL_FN(GL_PREFIX(CompressedTexImage2DARB))
.type glCompressedTexImage2DARB,#function
GL_PREFIX(CompressedTexImage2DARB):
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

GLOBL_FN(GL_PREFIX(CompressedTexImage1DARB))
.type glCompressedTexImage1DARB,#function
GL_PREFIX(CompressedTexImage1DARB):
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

GLOBL_FN(GL_PREFIX(CompressedTexSubImage3DARB))
.type glCompressedTexSubImage3DARB,#function
GL_PREFIX(CompressedTexSubImage3DARB):
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

GLOBL_FN(GL_PREFIX(CompressedTexSubImage2DARB))
.type glCompressedTexSubImage2DARB,#function
GL_PREFIX(CompressedTexSubImage2DARB):
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

GLOBL_FN(GL_PREFIX(CompressedTexSubImage1DARB))
.type glCompressedTexSubImage1DARB,#function
GL_PREFIX(CompressedTexSubImage1DARB):
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

GLOBL_FN(GL_PREFIX(GetCompressedTexImageARB))
.type glGetCompressedTexImageARB,#function
GL_PREFIX(GetCompressedTexImageARB):
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

GLOBL_FN(GL_PREFIX(ActiveTexture))
.type glActiveTexture,#function
GL_PREFIX(ActiveTexture):
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

GLOBL_FN(GL_PREFIX(ClientActiveTexture))
.type glClientActiveTexture,#function
GL_PREFIX(ClientActiveTexture):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord1d))
.type glMultiTexCoord1d,#function
GL_PREFIX(MultiTexCoord1d):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord1dv))
.type glMultiTexCoord1dv,#function
GL_PREFIX(MultiTexCoord1dv):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord1f))
.type glMultiTexCoord1f,#function
GL_PREFIX(MultiTexCoord1f):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord1fv))
.type glMultiTexCoord1fv,#function
GL_PREFIX(MultiTexCoord1fv):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord1i))
.type glMultiTexCoord1i,#function
GL_PREFIX(MultiTexCoord1i):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord1iv))
.type glMultiTexCoord1iv,#function
GL_PREFIX(MultiTexCoord1iv):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord1s))
.type glMultiTexCoord1s,#function
GL_PREFIX(MultiTexCoord1s):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord1sv))
.type glMultiTexCoord1sv,#function
GL_PREFIX(MultiTexCoord1sv):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord2d))
.type glMultiTexCoord2d,#function
GL_PREFIX(MultiTexCoord2d):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord2dv))
.type glMultiTexCoord2dv,#function
GL_PREFIX(MultiTexCoord2dv):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord2f))
.type glMultiTexCoord2f,#function
GL_PREFIX(MultiTexCoord2f):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord2fv))
.type glMultiTexCoord2fv,#function
GL_PREFIX(MultiTexCoord2fv):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord2i))
.type glMultiTexCoord2i,#function
GL_PREFIX(MultiTexCoord2i):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord2iv))
.type glMultiTexCoord2iv,#function
GL_PREFIX(MultiTexCoord2iv):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord2s))
.type glMultiTexCoord2s,#function
GL_PREFIX(MultiTexCoord2s):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord2sv))
.type glMultiTexCoord2sv,#function
GL_PREFIX(MultiTexCoord2sv):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord3d))
.type glMultiTexCoord3d,#function
GL_PREFIX(MultiTexCoord3d):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord3dv))
.type glMultiTexCoord3dv,#function
GL_PREFIX(MultiTexCoord3dv):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord3f))
.type glMultiTexCoord3f,#function
GL_PREFIX(MultiTexCoord3f):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord3fv))
.type glMultiTexCoord3fv,#function
GL_PREFIX(MultiTexCoord3fv):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord3i))
.type glMultiTexCoord3i,#function
GL_PREFIX(MultiTexCoord3i):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord3iv))
.type glMultiTexCoord3iv,#function
GL_PREFIX(MultiTexCoord3iv):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord3s))
.type glMultiTexCoord3s,#function
GL_PREFIX(MultiTexCoord3s):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord3sv))
.type glMultiTexCoord3sv,#function
GL_PREFIX(MultiTexCoord3sv):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord4d))
.type glMultiTexCoord4d,#function
GL_PREFIX(MultiTexCoord4d):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord4dv))
.type glMultiTexCoord4dv,#function
GL_PREFIX(MultiTexCoord4dv):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord4f))
.type glMultiTexCoord4f,#function
GL_PREFIX(MultiTexCoord4f):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord4fv))
.type glMultiTexCoord4fv,#function
GL_PREFIX(MultiTexCoord4fv):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord4i))
.type glMultiTexCoord4i,#function
GL_PREFIX(MultiTexCoord4i):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord4iv))
.type glMultiTexCoord4iv,#function
GL_PREFIX(MultiTexCoord4iv):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord4s))
.type glMultiTexCoord4s,#function
GL_PREFIX(MultiTexCoord4s):
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

GLOBL_FN(GL_PREFIX(MultiTexCoord4sv))
.type glMultiTexCoord4sv,#function
GL_PREFIX(MultiTexCoord4sv):
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

GLOBL_FN(GL_PREFIX(LoadTransposeMatrixf))
.type glLoadTransposeMatrixf,#function
GL_PREFIX(LoadTransposeMatrixf):
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

GLOBL_FN(GL_PREFIX(LoadTransposeMatrixd))
.type glLoadTransposeMatrixd,#function
GL_PREFIX(LoadTransposeMatrixd):
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

GLOBL_FN(GL_PREFIX(MultTransposeMatrixf))
.type glMultTransposeMatrixf,#function
GL_PREFIX(MultTransposeMatrixf):
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

GLOBL_FN(GL_PREFIX(MultTransposeMatrixd))
.type glMultTransposeMatrixd,#function
GL_PREFIX(MultTransposeMatrixd):
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

GLOBL_FN(GL_PREFIX(CompressedTexImage3D))
.type glCompressedTexImage3D,#function
GL_PREFIX(CompressedTexImage3D):
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

GLOBL_FN(GL_PREFIX(CompressedTexImage2D))
.type glCompressedTexImage2D,#function
GL_PREFIX(CompressedTexImage2D):
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

GLOBL_FN(GL_PREFIX(CompressedTexImage1D))
.type glCompressedTexImage1D,#function
GL_PREFIX(CompressedTexImage1D):
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

GLOBL_FN(GL_PREFIX(CompressedTexSubImage3D))
.type glCompressedTexSubImage3D,#function
GL_PREFIX(CompressedTexSubImage3D):
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

GLOBL_FN(GL_PREFIX(CompressedTexSubImage2D))
.type glCompressedTexSubImage2D,#function
GL_PREFIX(CompressedTexSubImage2D):
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

GLOBL_FN(GL_PREFIX(CompressedTexSubImage1D))
.type glCompressedTexSubImage1D,#function
GL_PREFIX(CompressedTexSubImage1D):
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

GLOBL_FN(GL_PREFIX(GetCompressedTexImage))
.type glGetCompressedTexImage,#function
GL_PREFIX(GetCompressedTexImage):
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

GLOBL_FN(GL_PREFIX(BlendColorEXT))
.type glBlendColorEXT,#function
GL_PREFIX(BlendColorEXT):
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

GLOBL_FN(GL_PREFIX(PolygonOffsetEXT))
.type glPolygonOffsetEXT,#function
GL_PREFIX(PolygonOffsetEXT):
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

GLOBL_FN(GL_PREFIX(TexImage3DEXT))
.type glTexImage3DEXT,#function
GL_PREFIX(TexImage3DEXT):
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

GLOBL_FN(GL_PREFIX(TexSubImage3DEXT))
.type glTexSubImage3DEXT,#function
GL_PREFIX(TexSubImage3DEXT):
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

GLOBL_FN(GL_PREFIX(GetTexFilterFuncSGIS))
.type glGetTexFilterFuncSGIS,#function
GL_PREFIX(GetTexFilterFuncSGIS):
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

GLOBL_FN(GL_PREFIX(TexFilterFuncSGIS))
.type glTexFilterFuncSGIS,#function
GL_PREFIX(TexFilterFuncSGIS):
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

GLOBL_FN(GL_PREFIX(TexSubImage1DEXT))
.type glTexSubImage1DEXT,#function
GL_PREFIX(TexSubImage1DEXT):
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

GLOBL_FN(GL_PREFIX(TexSubImage2DEXT))
.type glTexSubImage2DEXT,#function
GL_PREFIX(TexSubImage2DEXT):
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

GLOBL_FN(GL_PREFIX(CopyTexImage1DEXT))
.type glCopyTexImage1DEXT,#function
GL_PREFIX(CopyTexImage1DEXT):
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

GLOBL_FN(GL_PREFIX(CopyTexImage2DEXT))
.type glCopyTexImage2DEXT,#function
GL_PREFIX(CopyTexImage2DEXT):
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

GLOBL_FN(GL_PREFIX(CopyTexSubImage1DEXT))
.type glCopyTexSubImage1DEXT,#function
GL_PREFIX(CopyTexSubImage1DEXT):
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

GLOBL_FN(GL_PREFIX(CopyTexSubImage2DEXT))
.type glCopyTexSubImage2DEXT,#function
GL_PREFIX(CopyTexSubImage2DEXT):
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

GLOBL_FN(GL_PREFIX(CopyTexSubImage3DEXT))
.type glCopyTexSubImage3DEXT,#function
GL_PREFIX(CopyTexSubImage3DEXT):
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

GLOBL_FN(GL_PREFIX(GetHistogramEXT))
.type glGetHistogramEXT,#function
GL_PREFIX(GetHistogramEXT):
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

GLOBL_FN(GL_PREFIX(GetHistogramParameterfvEXT))
.type glGetHistogramParameterfvEXT,#function
GL_PREFIX(GetHistogramParameterfvEXT):
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

GLOBL_FN(GL_PREFIX(GetHistogramParameterivEXT))
.type glGetHistogramParameterivEXT,#function
GL_PREFIX(GetHistogramParameterivEXT):
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

GLOBL_FN(GL_PREFIX(GetMinmaxEXT))
.type glGetMinmaxEXT,#function
GL_PREFIX(GetMinmaxEXT):
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

GLOBL_FN(GL_PREFIX(GetMinmaxParameterfvEXT))
.type glGetMinmaxParameterfvEXT,#function
GL_PREFIX(GetMinmaxParameterfvEXT):
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

GLOBL_FN(GL_PREFIX(GetMinmaxParameterivEXT))
.type glGetMinmaxParameterivEXT,#function
GL_PREFIX(GetMinmaxParameterivEXT):
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

GLOBL_FN(GL_PREFIX(HistogramEXT))
.type glHistogramEXT,#function
GL_PREFIX(HistogramEXT):
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

GLOBL_FN(GL_PREFIX(MinmaxEXT))
.type glMinmaxEXT,#function
GL_PREFIX(MinmaxEXT):
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

GLOBL_FN(GL_PREFIX(ResetHistogramEXT))
.type glResetHistogramEXT,#function
GL_PREFIX(ResetHistogramEXT):
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

GLOBL_FN(GL_PREFIX(ResetMinmaxEXT))
.type glResetMinmaxEXT,#function
GL_PREFIX(ResetMinmaxEXT):
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

GLOBL_FN(GL_PREFIX(ConvolutionFilter1DEXT))
.type glConvolutionFilter1DEXT,#function
GL_PREFIX(ConvolutionFilter1DEXT):
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

GLOBL_FN(GL_PREFIX(ConvolutionFilter2DEXT))
.type glConvolutionFilter2DEXT,#function
GL_PREFIX(ConvolutionFilter2DEXT):
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

GLOBL_FN(GL_PREFIX(ConvolutionParameterfEXT))
.type glConvolutionParameterfEXT,#function
GL_PREFIX(ConvolutionParameterfEXT):
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

GLOBL_FN(GL_PREFIX(ConvolutionParameterfvEXT))
.type glConvolutionParameterfvEXT,#function
GL_PREFIX(ConvolutionParameterfvEXT):
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

GLOBL_FN(GL_PREFIX(ConvolutionParameteriEXT))
.type glConvolutionParameteriEXT,#function
GL_PREFIX(ConvolutionParameteriEXT):
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

GLOBL_FN(GL_PREFIX(ConvolutionParameterivEXT))
.type glConvolutionParameterivEXT,#function
GL_PREFIX(ConvolutionParameterivEXT):
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

GLOBL_FN(GL_PREFIX(CopyConvolutionFilter1DEXT))
.type glCopyConvolutionFilter1DEXT,#function
GL_PREFIX(CopyConvolutionFilter1DEXT):
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

GLOBL_FN(GL_PREFIX(CopyConvolutionFilter2DEXT))
.type glCopyConvolutionFilter2DEXT,#function
GL_PREFIX(CopyConvolutionFilter2DEXT):
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

GLOBL_FN(GL_PREFIX(GetConvolutionFilterEXT))
.type glGetConvolutionFilterEXT,#function
GL_PREFIX(GetConvolutionFilterEXT):
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

GLOBL_FN(GL_PREFIX(GetConvolutionParameterfvEXT))
.type glGetConvolutionParameterfvEXT,#function
GL_PREFIX(GetConvolutionParameterfvEXT):
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

GLOBL_FN(GL_PREFIX(GetConvolutionParameterivEXT))
.type glGetConvolutionParameterivEXT,#function
GL_PREFIX(GetConvolutionParameterivEXT):
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

GLOBL_FN(GL_PREFIX(GetSeparableFilterEXT))
.type glGetSeparableFilterEXT,#function
GL_PREFIX(GetSeparableFilterEXT):
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

GLOBL_FN(GL_PREFIX(SeparableFilter2DEXT))
.type glSeparableFilter2DEXT,#function
GL_PREFIX(SeparableFilter2DEXT):
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

GLOBL_FN(GL_PREFIX(ColorTableSGI))
.type glColorTableSGI,#function
GL_PREFIX(ColorTableSGI):
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

GLOBL_FN(GL_PREFIX(ColorTableParameterfvSGI))
.type glColorTableParameterfvSGI,#function
GL_PREFIX(ColorTableParameterfvSGI):
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

GLOBL_FN(GL_PREFIX(ColorTableParameterivSGI))
.type glColorTableParameterivSGI,#function
GL_PREFIX(ColorTableParameterivSGI):
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

GLOBL_FN(GL_PREFIX(CopyColorTableSGI))
.type glCopyColorTableSGI,#function
GL_PREFIX(CopyColorTableSGI):
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

GLOBL_FN(GL_PREFIX(GetColorTableSGI))
.type glGetColorTableSGI,#function
GL_PREFIX(GetColorTableSGI):
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

GLOBL_FN(GL_PREFIX(GetColorTableParameterfvSGI))
.type glGetColorTableParameterfvSGI,#function
GL_PREFIX(GetColorTableParameterfvSGI):
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

GLOBL_FN(GL_PREFIX(GetColorTableParameterivSGI))
.type glGetColorTableParameterivSGI,#function
GL_PREFIX(GetColorTableParameterivSGI):
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

GLOBL_FN(GL_PREFIX(PixelTexGenSGIX))
.type glPixelTexGenSGIX,#function
GL_PREFIX(PixelTexGenSGIX):
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

GLOBL_FN(GL_PREFIX(PixelTexGenParameteriSGIS))
.type glPixelTexGenParameteriSGIS,#function
GL_PREFIX(PixelTexGenParameteriSGIS):
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

GLOBL_FN(GL_PREFIX(PixelTexGenParameterivSGIS))
.type glPixelTexGenParameterivSGIS,#function
GL_PREFIX(PixelTexGenParameterivSGIS):
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

GLOBL_FN(GL_PREFIX(PixelTexGenParameterfSGIS))
.type glPixelTexGenParameterfSGIS,#function
GL_PREFIX(PixelTexGenParameterfSGIS):
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

GLOBL_FN(GL_PREFIX(PixelTexGenParameterfvSGIS))
.type glPixelTexGenParameterfvSGIS,#function
GL_PREFIX(PixelTexGenParameterfvSGIS):
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

GLOBL_FN(GL_PREFIX(GetPixelTexGenParameterivSGIS))
.type glGetPixelTexGenParameterivSGIS,#function
GL_PREFIX(GetPixelTexGenParameterivSGIS):
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

GLOBL_FN(GL_PREFIX(GetPixelTexGenParameterfvSGIS))
.type glGetPixelTexGenParameterfvSGIS,#function
GL_PREFIX(GetPixelTexGenParameterfvSGIS):
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

GLOBL_FN(GL_PREFIX(TexImage4DSGIS))
.type glTexImage4DSGIS,#function
GL_PREFIX(TexImage4DSGIS):
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

GLOBL_FN(GL_PREFIX(TexSubImage4DSGIS))
.type glTexSubImage4DSGIS,#function
GL_PREFIX(TexSubImage4DSGIS):
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

GLOBL_FN(GL_PREFIX(AreTexturesResidentEXT))
.type glAreTexturesResidentEXT,#function
GL_PREFIX(AreTexturesResidentEXT):
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

GLOBL_FN(GL_PREFIX(BindTextureEXT))
.type glBindTextureEXT,#function
GL_PREFIX(BindTextureEXT):
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

GLOBL_FN(GL_PREFIX(DeleteTexturesEXT))
.type glDeleteTexturesEXT,#function
GL_PREFIX(DeleteTexturesEXT):
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

GLOBL_FN(GL_PREFIX(GenTexturesEXT))
.type glGenTexturesEXT,#function
GL_PREFIX(GenTexturesEXT):
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

GLOBL_FN(GL_PREFIX(IsTextureEXT))
.type glIsTextureEXT,#function
GL_PREFIX(IsTextureEXT):
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

GLOBL_FN(GL_PREFIX(PrioritizeTexturesEXT))
.type glPrioritizeTexturesEXT,#function
GL_PREFIX(PrioritizeTexturesEXT):
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

GLOBL_FN(GL_PREFIX(DetailTexFuncSGIS))
.type glDetailTexFuncSGIS,#function
GL_PREFIX(DetailTexFuncSGIS):
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

GLOBL_FN(GL_PREFIX(GetDetailTexFuncSGIS))
.type glGetDetailTexFuncSGIS,#function
GL_PREFIX(GetDetailTexFuncSGIS):
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

GLOBL_FN(GL_PREFIX(SharpenTexFuncSGIS))
.type glSharpenTexFuncSGIS,#function
GL_PREFIX(SharpenTexFuncSGIS):
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

GLOBL_FN(GL_PREFIX(GetSharpenTexFuncSGIS))
.type glGetSharpenTexFuncSGIS,#function
GL_PREFIX(GetSharpenTexFuncSGIS):
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

GLOBL_FN(GL_PREFIX(SampleMaskSGIS))
.type glSampleMaskSGIS,#function
GL_PREFIX(SampleMaskSGIS):
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

GLOBL_FN(GL_PREFIX(SamplePatternSGIS))
.type glSamplePatternSGIS,#function
GL_PREFIX(SamplePatternSGIS):
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

GLOBL_FN(GL_PREFIX(ArrayElementEXT))
.type glArrayElementEXT,#function
GL_PREFIX(ArrayElementEXT):
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

GLOBL_FN(GL_PREFIX(ColorPointerEXT))
.type glColorPointerEXT,#function
GL_PREFIX(ColorPointerEXT):
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

GLOBL_FN(GL_PREFIX(DrawArraysEXT))
.type glDrawArraysEXT,#function
GL_PREFIX(DrawArraysEXT):
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

GLOBL_FN(GL_PREFIX(EdgeFlagPointerEXT))
.type glEdgeFlagPointerEXT,#function
GL_PREFIX(EdgeFlagPointerEXT):
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

GLOBL_FN(GL_PREFIX(GetPointervEXT))
.type glGetPointervEXT,#function
GL_PREFIX(GetPointervEXT):
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

GLOBL_FN(GL_PREFIX(IndexPointerEXT))
.type glIndexPointerEXT,#function
GL_PREFIX(IndexPointerEXT):
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

GLOBL_FN(GL_PREFIX(NormalPointerEXT))
.type glNormalPointerEXT,#function
GL_PREFIX(NormalPointerEXT):
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

GLOBL_FN(GL_PREFIX(TexCoordPointerEXT))
.type glTexCoordPointerEXT,#function
GL_PREFIX(TexCoordPointerEXT):
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

GLOBL_FN(GL_PREFIX(VertexPointerEXT))
.type glVertexPointerEXT,#function
GL_PREFIX(VertexPointerEXT):
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

GLOBL_FN(GL_PREFIX(BlendEquationEXT))
.type glBlendEquationEXT,#function
GL_PREFIX(BlendEquationEXT):
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

GLOBL_FN(GL_PREFIX(SpriteParameterfSGIX))
.type glSpriteParameterfSGIX,#function
GL_PREFIX(SpriteParameterfSGIX):
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

GLOBL_FN(GL_PREFIX(SpriteParameterfvSGIX))
.type glSpriteParameterfvSGIX,#function
GL_PREFIX(SpriteParameterfvSGIX):
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

GLOBL_FN(GL_PREFIX(SpriteParameteriSGIX))
.type glSpriteParameteriSGIX,#function
GL_PREFIX(SpriteParameteriSGIX):
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

GLOBL_FN(GL_PREFIX(SpriteParameterivSGIX))
.type glSpriteParameterivSGIX,#function
GL_PREFIX(SpriteParameterivSGIX):
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

GLOBL_FN(GL_PREFIX(PointParameterfEXT))
.type glPointParameterfEXT,#function
GL_PREFIX(PointParameterfEXT):
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

GLOBL_FN(GL_PREFIX(PointParameterfvEXT))
.type glPointParameterfvEXT,#function
GL_PREFIX(PointParameterfvEXT):
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

GLOBL_FN(GL_PREFIX(PointParameterfARB))
.type glPointParameterfARB,#function
GL_PREFIX(PointParameterfARB):
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

GLOBL_FN(GL_PREFIX(PointParameterfvARB))
.type glPointParameterfvARB,#function
GL_PREFIX(PointParameterfvARB):
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

GLOBL_FN(GL_PREFIX(PointParameterfSGIS))
.type glPointParameterfSGIS,#function
GL_PREFIX(PointParameterfSGIS):
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

GLOBL_FN(GL_PREFIX(PointParameterfvSGIS))
.type glPointParameterfvSGIS,#function
GL_PREFIX(PointParameterfvSGIS):
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

GLOBL_FN(GL_PREFIX(GetInstrumentsSGIX))
.type glGetInstrumentsSGIX,#function
GL_PREFIX(GetInstrumentsSGIX):
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

GLOBL_FN(GL_PREFIX(InstrumentsBufferSGIX))
.type glInstrumentsBufferSGIX,#function
GL_PREFIX(InstrumentsBufferSGIX):
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

GLOBL_FN(GL_PREFIX(PollInstrumentsSGIX))
.type glPollInstrumentsSGIX,#function
GL_PREFIX(PollInstrumentsSGIX):
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

GLOBL_FN(GL_PREFIX(ReadInstrumentsSGIX))
.type glReadInstrumentsSGIX,#function
GL_PREFIX(ReadInstrumentsSGIX):
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

GLOBL_FN(GL_PREFIX(StartInstrumentsSGIX))
.type glStartInstrumentsSGIX,#function
GL_PREFIX(StartInstrumentsSGIX):
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

GLOBL_FN(GL_PREFIX(StopInstrumentsSGIX))
.type glStopInstrumentsSGIX,#function
GL_PREFIX(StopInstrumentsSGIX):
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

GLOBL_FN(GL_PREFIX(FrameZoomSGIX))
.type glFrameZoomSGIX,#function
GL_PREFIX(FrameZoomSGIX):
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

GLOBL_FN(GL_PREFIX(TagSampleBufferSGIX))
.type glTagSampleBufferSGIX,#function
GL_PREFIX(TagSampleBufferSGIX):
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

GLOBL_FN(GL_PREFIX(ReferencePlaneSGIX))
.type glReferencePlaneSGIX,#function
GL_PREFIX(ReferencePlaneSGIX):
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

GLOBL_FN(GL_PREFIX(FlushRasterSGIX))
.type glFlushRasterSGIX,#function
GL_PREFIX(FlushRasterSGIX):
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

GLOBL_FN(GL_PREFIX(ColorSubTableEXT))
.type glColorSubTableEXT,#function
GL_PREFIX(ColorSubTableEXT):
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

GLOBL_FN(GL_PREFIX(CopyColorSubTableEXT))
.type glCopyColorSubTableEXT,#function
GL_PREFIX(CopyColorSubTableEXT):
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

GLOBL_FN(GL_PREFIX(HintPGI))
.type glHintPGI,#function
GL_PREFIX(HintPGI):
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

GLOBL_FN(GL_PREFIX(ColorTableEXT))
.type glColorTableEXT,#function
GL_PREFIX(ColorTableEXT):
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

GLOBL_FN(GL_PREFIX(GetColorTableEXT))
.type glGetColorTableEXT,#function
GL_PREFIX(GetColorTableEXT):
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

GLOBL_FN(GL_PREFIX(GetColorTableParameterivEXT))
.type glGetColorTableParameterivEXT,#function
GL_PREFIX(GetColorTableParameterivEXT):
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

GLOBL_FN(GL_PREFIX(GetColorTableParameterfvEXT))
.type glGetColorTableParameterfvEXT,#function
GL_PREFIX(GetColorTableParameterfvEXT):
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

GLOBL_FN(GL_PREFIX(GetListParameterfvSGIX))
.type glGetListParameterfvSGIX,#function
GL_PREFIX(GetListParameterfvSGIX):
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

GLOBL_FN(GL_PREFIX(GetListParameterivSGIX))
.type glGetListParameterivSGIX,#function
GL_PREFIX(GetListParameterivSGIX):
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

GLOBL_FN(GL_PREFIX(ListParameterfSGIX))
.type glListParameterfSGIX,#function
GL_PREFIX(ListParameterfSGIX):
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

GLOBL_FN(GL_PREFIX(ListParameterfvSGIX))
.type glListParameterfvSGIX,#function
GL_PREFIX(ListParameterfvSGIX):
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

GLOBL_FN(GL_PREFIX(ListParameteriSGIX))
.type glListParameteriSGIX,#function
GL_PREFIX(ListParameteriSGIX):
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

GLOBL_FN(GL_PREFIX(ListParameterivSGIX))
.type glListParameterivSGIX,#function
GL_PREFIX(ListParameterivSGIX):
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

GLOBL_FN(GL_PREFIX(IndexMaterialEXT))
.type glIndexMaterialEXT,#function
GL_PREFIX(IndexMaterialEXT):
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

GLOBL_FN(GL_PREFIX(IndexFuncEXT))
.type glIndexFuncEXT,#function
GL_PREFIX(IndexFuncEXT):
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

GLOBL_FN(GL_PREFIX(LockArraysEXT))
.type glLockArraysEXT,#function
GL_PREFIX(LockArraysEXT):
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

GLOBL_FN(GL_PREFIX(UnlockArraysEXT))
.type glUnlockArraysEXT,#function
GL_PREFIX(UnlockArraysEXT):
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

GLOBL_FN(GL_PREFIX(CullParameterdvEXT))
.type glCullParameterdvEXT,#function
GL_PREFIX(CullParameterdvEXT):
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

GLOBL_FN(GL_PREFIX(CullParameterfvEXT))
.type glCullParameterfvEXT,#function
GL_PREFIX(CullParameterfvEXT):
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

GLOBL_FN(GL_PREFIX(FragmentColorMaterialSGIX))
.type glFragmentColorMaterialSGIX,#function
GL_PREFIX(FragmentColorMaterialSGIX):
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

GLOBL_FN(GL_PREFIX(FragmentLightfSGIX))
.type glFragmentLightfSGIX,#function
GL_PREFIX(FragmentLightfSGIX):
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

GLOBL_FN(GL_PREFIX(FragmentLightfvSGIX))
.type glFragmentLightfvSGIX,#function
GL_PREFIX(FragmentLightfvSGIX):
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

GLOBL_FN(GL_PREFIX(FragmentLightiSGIX))
.type glFragmentLightiSGIX,#function
GL_PREFIX(FragmentLightiSGIX):
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

GLOBL_FN(GL_PREFIX(FragmentLightivSGIX))
.type glFragmentLightivSGIX,#function
GL_PREFIX(FragmentLightivSGIX):
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

GLOBL_FN(GL_PREFIX(FragmentLightModelfSGIX))
.type glFragmentLightModelfSGIX,#function
GL_PREFIX(FragmentLightModelfSGIX):
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

GLOBL_FN(GL_PREFIX(FragmentLightModelfvSGIX))
.type glFragmentLightModelfvSGIX,#function
GL_PREFIX(FragmentLightModelfvSGIX):
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

GLOBL_FN(GL_PREFIX(FragmentLightModeliSGIX))
.type glFragmentLightModeliSGIX,#function
GL_PREFIX(FragmentLightModeliSGIX):
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

GLOBL_FN(GL_PREFIX(FragmentLightModelivSGIX))
.type glFragmentLightModelivSGIX,#function
GL_PREFIX(FragmentLightModelivSGIX):
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

GLOBL_FN(GL_PREFIX(FragmentMaterialfSGIX))
.type glFragmentMaterialfSGIX,#function
GL_PREFIX(FragmentMaterialfSGIX):
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

GLOBL_FN(GL_PREFIX(FragmentMaterialfvSGIX))
.type glFragmentMaterialfvSGIX,#function
GL_PREFIX(FragmentMaterialfvSGIX):
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

GLOBL_FN(GL_PREFIX(FragmentMaterialiSGIX))
.type glFragmentMaterialiSGIX,#function
GL_PREFIX(FragmentMaterialiSGIX):
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

GLOBL_FN(GL_PREFIX(FragmentMaterialivSGIX))
.type glFragmentMaterialivSGIX,#function
GL_PREFIX(FragmentMaterialivSGIX):
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

GLOBL_FN(GL_PREFIX(GetFragmentLightfvSGIX))
.type glGetFragmentLightfvSGIX,#function
GL_PREFIX(GetFragmentLightfvSGIX):
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

GLOBL_FN(GL_PREFIX(GetFragmentLightivSGIX))
.type glGetFragmentLightivSGIX,#function
GL_PREFIX(GetFragmentLightivSGIX):
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

GLOBL_FN(GL_PREFIX(GetFragmentMaterialfvSGIX))
.type glGetFragmentMaterialfvSGIX,#function
GL_PREFIX(GetFragmentMaterialfvSGIX):
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

GLOBL_FN(GL_PREFIX(GetFragmentMaterialivSGIX))
.type glGetFragmentMaterialivSGIX,#function
GL_PREFIX(GetFragmentMaterialivSGIX):
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

GLOBL_FN(GL_PREFIX(LightEnviSGIX))
.type glLightEnviSGIX,#function
GL_PREFIX(LightEnviSGIX):
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

GLOBL_FN(GL_PREFIX(DrawRangeElementsEXT))
.type glDrawRangeElementsEXT,#function
GL_PREFIX(DrawRangeElementsEXT):
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

GLOBL_FN(GL_PREFIX(SecondaryColor3bEXT))
.type glSecondaryColor3bEXT,#function
GL_PREFIX(SecondaryColor3bEXT):
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

GLOBL_FN(GL_PREFIX(SecondaryColor3bvEXT))
.type glSecondaryColor3bvEXT,#function
GL_PREFIX(SecondaryColor3bvEXT):
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

GLOBL_FN(GL_PREFIX(SecondaryColor3dEXT))
.type glSecondaryColor3dEXT,#function
GL_PREFIX(SecondaryColor3dEXT):
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

GLOBL_FN(GL_PREFIX(SecondaryColor3dvEXT))
.type glSecondaryColor3dvEXT,#function
GL_PREFIX(SecondaryColor3dvEXT):
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

GLOBL_FN(GL_PREFIX(SecondaryColor3fEXT))
.type glSecondaryColor3fEXT,#function
GL_PREFIX(SecondaryColor3fEXT):
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

GLOBL_FN(GL_PREFIX(SecondaryColor3fvEXT))
.type glSecondaryColor3fvEXT,#function
GL_PREFIX(SecondaryColor3fvEXT):
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

GLOBL_FN(GL_PREFIX(SecondaryColor3iEXT))
.type glSecondaryColor3iEXT,#function
GL_PREFIX(SecondaryColor3iEXT):
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

GLOBL_FN(GL_PREFIX(SecondaryColor3ivEXT))
.type glSecondaryColor3ivEXT,#function
GL_PREFIX(SecondaryColor3ivEXT):
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

GLOBL_FN(GL_PREFIX(SecondaryColor3sEXT))
.type glSecondaryColor3sEXT,#function
GL_PREFIX(SecondaryColor3sEXT):
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

GLOBL_FN(GL_PREFIX(SecondaryColor3svEXT))
.type glSecondaryColor3svEXT,#function
GL_PREFIX(SecondaryColor3svEXT):
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

GLOBL_FN(GL_PREFIX(SecondaryColor3ubEXT))
.type glSecondaryColor3ubEXT,#function
GL_PREFIX(SecondaryColor3ubEXT):
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

GLOBL_FN(GL_PREFIX(SecondaryColor3ubvEXT))
.type glSecondaryColor3ubvEXT,#function
GL_PREFIX(SecondaryColor3ubvEXT):
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

GLOBL_FN(GL_PREFIX(SecondaryColor3uiEXT))
.type glSecondaryColor3uiEXT,#function
GL_PREFIX(SecondaryColor3uiEXT):
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

GLOBL_FN(GL_PREFIX(SecondaryColor3uivEXT))
.type glSecondaryColor3uivEXT,#function
GL_PREFIX(SecondaryColor3uivEXT):
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

GLOBL_FN(GL_PREFIX(SecondaryColor3usEXT))
.type glSecondaryColor3usEXT,#function
GL_PREFIX(SecondaryColor3usEXT):
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

GLOBL_FN(GL_PREFIX(SecondaryColor3usvEXT))
.type glSecondaryColor3usvEXT,#function
GL_PREFIX(SecondaryColor3usvEXT):
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

GLOBL_FN(GL_PREFIX(SecondaryColorPointerEXT))
.type glSecondaryColorPointerEXT,#function
GL_PREFIX(SecondaryColorPointerEXT):
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

GLOBL_FN(GL_PREFIX(FogCoordfEXT))
.type glFogCoordfEXT,#function
GL_PREFIX(FogCoordfEXT):
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

GLOBL_FN(GL_PREFIX(FogCoordfvEXT))
.type glFogCoordfvEXT,#function
GL_PREFIX(FogCoordfvEXT):
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

GLOBL_FN(GL_PREFIX(FogCoorddEXT))
.type glFogCoorddEXT,#function
GL_PREFIX(FogCoorddEXT):
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

GLOBL_FN(GL_PREFIX(FogCoorddvEXT))
.type glFogCoorddvEXT,#function
GL_PREFIX(FogCoorddvEXT):
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

GLOBL_FN(GL_PREFIX(FogCoordPointerEXT))
.type glFogCoordPointerEXT,#function
GL_PREFIX(FogCoordPointerEXT):
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

GLOBL_FN(GL_PREFIX(BlendFuncSeparateEXT))
.type glBlendFuncSeparateEXT,#function
GL_PREFIX(BlendFuncSeparateEXT):
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

GLOBL_FN(GL_PREFIX(BlendFuncSeparateINGR))
.type glBlendFuncSeparateINGR,#function
GL_PREFIX(BlendFuncSeparateINGR):
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

GLOBL_FN(GL_PREFIX(VertexWeightfEXT))
.type glVertexWeightfEXT,#function
GL_PREFIX(VertexWeightfEXT):
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

GLOBL_FN(GL_PREFIX(VertexWeightfvEXT))
.type glVertexWeightfvEXT,#function
GL_PREFIX(VertexWeightfvEXT):
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

GLOBL_FN(GL_PREFIX(VertexWeightPointerEXT))
.type glVertexWeightPointerEXT,#function
GL_PREFIX(VertexWeightPointerEXT):
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

GLOBL_FN(GL_PREFIX(FlushVertexArrayRangeNV))
.type glFlushVertexArrayRangeNV,#function
GL_PREFIX(FlushVertexArrayRangeNV):
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

GLOBL_FN(GL_PREFIX(VertexArrayRangeNV))
.type glVertexArrayRangeNV,#function
GL_PREFIX(VertexArrayRangeNV):
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

GLOBL_FN(GL_PREFIX(CombinerParameterfvNV))
.type glCombinerParameterfvNV,#function
GL_PREFIX(CombinerParameterfvNV):
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

GLOBL_FN(GL_PREFIX(CombinerParameterfNV))
.type glCombinerParameterfNV,#function
GL_PREFIX(CombinerParameterfNV):
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

GLOBL_FN(GL_PREFIX(CombinerParameterivNV))
.type glCombinerParameterivNV,#function
GL_PREFIX(CombinerParameterivNV):
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

GLOBL_FN(GL_PREFIX(CombinerParameteriNV))
.type glCombinerParameteriNV,#function
GL_PREFIX(CombinerParameteriNV):
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

GLOBL_FN(GL_PREFIX(CombinerInputNV))
.type glCombinerInputNV,#function
GL_PREFIX(CombinerInputNV):
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

GLOBL_FN(GL_PREFIX(CombinerOutputNV))
.type glCombinerOutputNV,#function
GL_PREFIX(CombinerOutputNV):
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

GLOBL_FN(GL_PREFIX(FinalCombinerInputNV))
.type glFinalCombinerInputNV,#function
GL_PREFIX(FinalCombinerInputNV):
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

GLOBL_FN(GL_PREFIX(GetCombinerInputParameterfvNV))
.type glGetCombinerInputParameterfvNV,#function
GL_PREFIX(GetCombinerInputParameterfvNV):
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

GLOBL_FN(GL_PREFIX(GetCombinerInputParameterivNV))
.type glGetCombinerInputParameterivNV,#function
GL_PREFIX(GetCombinerInputParameterivNV):
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

GLOBL_FN(GL_PREFIX(GetCombinerOutputParameterfvNV))
.type glGetCombinerOutputParameterfvNV,#function
GL_PREFIX(GetCombinerOutputParameterfvNV):
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

GLOBL_FN(GL_PREFIX(GetCombinerOutputParameterivNV))
.type glGetCombinerOutputParameterivNV,#function
GL_PREFIX(GetCombinerOutputParameterivNV):
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

GLOBL_FN(GL_PREFIX(GetFinalCombinerInputParameterfvNV))
.type glGetFinalCombinerInputParameterfvNV,#function
GL_PREFIX(GetFinalCombinerInputParameterfvNV):
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

GLOBL_FN(GL_PREFIX(GetFinalCombinerInputParameterivNV))
.type glGetFinalCombinerInputParameterivNV,#function
GL_PREFIX(GetFinalCombinerInputParameterivNV):
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

GLOBL_FN(GL_PREFIX(ResizeBuffersMESA))
.type glResizeBuffersMESA,#function
GL_PREFIX(ResizeBuffersMESA):
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

GLOBL_FN(GL_PREFIX(WindowPos2dMESA))
.type glWindowPos2dMESA,#function
GL_PREFIX(WindowPos2dMESA):
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

GLOBL_FN(GL_PREFIX(WindowPos2dvMESA))
.type glWindowPos2dvMESA,#function
GL_PREFIX(WindowPos2dvMESA):
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

GLOBL_FN(GL_PREFIX(WindowPos2fMESA))
.type glWindowPos2fMESA,#function
GL_PREFIX(WindowPos2fMESA):
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

GLOBL_FN(GL_PREFIX(WindowPos2fvMESA))
.type glWindowPos2fvMESA,#function
GL_PREFIX(WindowPos2fvMESA):
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

GLOBL_FN(GL_PREFIX(WindowPos2iMESA))
.type glWindowPos2iMESA,#function
GL_PREFIX(WindowPos2iMESA):
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

GLOBL_FN(GL_PREFIX(WindowPos2ivMESA))
.type glWindowPos2ivMESA,#function
GL_PREFIX(WindowPos2ivMESA):
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

GLOBL_FN(GL_PREFIX(WindowPos2sMESA))
.type glWindowPos2sMESA,#function
GL_PREFIX(WindowPos2sMESA):
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

GLOBL_FN(GL_PREFIX(WindowPos2svMESA))
.type glWindowPos2svMESA,#function
GL_PREFIX(WindowPos2svMESA):
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

GLOBL_FN(GL_PREFIX(WindowPos3dMESA))
.type glWindowPos3dMESA,#function
GL_PREFIX(WindowPos3dMESA):
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

GLOBL_FN(GL_PREFIX(WindowPos3dvMESA))
.type glWindowPos3dvMESA,#function
GL_PREFIX(WindowPos3dvMESA):
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

GLOBL_FN(GL_PREFIX(WindowPos3fMESA))
.type glWindowPos3fMESA,#function
GL_PREFIX(WindowPos3fMESA):
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

GLOBL_FN(GL_PREFIX(WindowPos3fvMESA))
.type glWindowPos3fvMESA,#function
GL_PREFIX(WindowPos3fvMESA):
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

GLOBL_FN(GL_PREFIX(WindowPos3iMESA))
.type glWindowPos3iMESA,#function
GL_PREFIX(WindowPos3iMESA):
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

GLOBL_FN(GL_PREFIX(WindowPos3ivMESA))
.type glWindowPos3ivMESA,#function
GL_PREFIX(WindowPos3ivMESA):
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

GLOBL_FN(GL_PREFIX(WindowPos3sMESA))
.type glWindowPos3sMESA,#function
GL_PREFIX(WindowPos3sMESA):
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

GLOBL_FN(GL_PREFIX(WindowPos3svMESA))
.type glWindowPos3svMESA,#function
GL_PREFIX(WindowPos3svMESA):
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

GLOBL_FN(GL_PREFIX(WindowPos4dMESA))
.type glWindowPos4dMESA,#function
GL_PREFIX(WindowPos4dMESA):
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

GLOBL_FN(GL_PREFIX(WindowPos4dvMESA))
.type glWindowPos4dvMESA,#function
GL_PREFIX(WindowPos4dvMESA):
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

GLOBL_FN(GL_PREFIX(WindowPos4fMESA))
.type glWindowPos4fMESA,#function
GL_PREFIX(WindowPos4fMESA):
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

GLOBL_FN(GL_PREFIX(WindowPos4fvMESA))
.type glWindowPos4fvMESA,#function
GL_PREFIX(WindowPos4fvMESA):
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

GLOBL_FN(GL_PREFIX(WindowPos4iMESA))
.type glWindowPos4iMESA,#function
GL_PREFIX(WindowPos4iMESA):
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

GLOBL_FN(GL_PREFIX(WindowPos4ivMESA))
.type glWindowPos4ivMESA,#function
GL_PREFIX(WindowPos4ivMESA):
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

GLOBL_FN(GL_PREFIX(WindowPos4sMESA))
.type glWindowPos4sMESA,#function
GL_PREFIX(WindowPos4sMESA):
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

GLOBL_FN(GL_PREFIX(WindowPos4svMESA))
.type glWindowPos4svMESA,#function
GL_PREFIX(WindowPos4svMESA):
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

GLOBL_FN(GL_PREFIX(TbufferMask3DFX))
.type glTbufferMask3DFX,#function
GL_PREFIX(TbufferMask3DFX):
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

GLOBL_FN(GL_PREFIX(SampleMaskEXT))
.type glSampleMaskEXT,#function
GL_PREFIX(SampleMaskEXT):
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

GLOBL_FN(GL_PREFIX(SamplePatternEXT))
.type glSamplePatternEXT,#function
GL_PREFIX(SamplePatternEXT):
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

GLOBL_FN(GL_PREFIX(WindowPos2dARB))
.type glWindowPos2dARB,#function
GL_PREFIX(WindowPos2dARB):
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos2dARB), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos2dARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos2dARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

GLOBL_FN(GL_PREFIX(WindowPos2fARB))
.type glWindowPos2fARB,#function
GL_PREFIX(WindowPos2fARB):
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos2fARB), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos2fARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos2fARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

GLOBL_FN(GL_PREFIX(WindowPos2iARB))
.type glWindowPos2iARB,#function
GL_PREFIX(WindowPos2iARB):
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos2iARB), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos2iARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos2iARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

GLOBL_FN(GL_PREFIX(WindowPos2sARB))
.type glWindowPos2sARB,#function
GL_PREFIX(WindowPos2sARB):
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos2sARB), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos2sARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos2sARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

GLOBL_FN(GL_PREFIX(WindowPos2dvARB))
.type glWindowPos2dvARB,#function
GL_PREFIX(WindowPos2dvARB):
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos2dvARB), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos2dvARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos2dvARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

GLOBL_FN(GL_PREFIX(WindowPos2fvARB))
.type glWindowPos2fvARB,#function
GL_PREFIX(WindowPos2fvARB):
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos2fvARB), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos2fvARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos2fvARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

GLOBL_FN(GL_PREFIX(WindowPos2ivARB))
.type glWindowPos2ivARB,#function
GL_PREFIX(WindowPos2ivARB):
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos2ivARB), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos2ivARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos2ivARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

GLOBL_FN(GL_PREFIX(WindowPos2svARB))
.type glWindowPos2svARB,#function
GL_PREFIX(WindowPos2svARB):
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos2svARB), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos2svARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos2svARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

GLOBL_FN(GL_PREFIX(WindowPos3dARB))
.type glWindowPos3dARB,#function
GL_PREFIX(WindowPos3dARB):
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos3dARB), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos3dARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos3dARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

GLOBL_FN(GL_PREFIX(WindowPos3fARB))
.type glWindowPos3fARB,#function
GL_PREFIX(WindowPos3fARB):
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos3fARB), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos3fARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos3fARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

GLOBL_FN(GL_PREFIX(WindowPos3iARB))
.type glWindowPos3iARB,#function
GL_PREFIX(WindowPos3iARB):
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos3iARB), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos3iARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos3iARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

GLOBL_FN(GL_PREFIX(WindowPos3sARB))
.type glWindowPos3sARB,#function
GL_PREFIX(WindowPos3sARB):
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos3sARB), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos3sARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos3sARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

GLOBL_FN(GL_PREFIX(WindowPos3dvARB))
.type glWindowPos3dvARB,#function
GL_PREFIX(WindowPos3dvARB):
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos3dvARB), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos3dvARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos3dvARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

GLOBL_FN(GL_PREFIX(WindowPos3fvARB))
.type glWindowPos3fvARB,#function
GL_PREFIX(WindowPos3fvARB):
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos3fvARB), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos3fvARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos3fvARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

GLOBL_FN(GL_PREFIX(WindowPos3ivARB))
.type glWindowPos3ivARB,#function
GL_PREFIX(WindowPos3ivARB):
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos3ivARB), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos3ivARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos3ivARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

GLOBL_FN(GL_PREFIX(WindowPos3svARB))
.type glWindowPos3svARB,#function
GL_PREFIX(WindowPos3svARB):
#ifdef __sparc_v9__
	sethi	%hi(0x00000000), %g2
	sethi	%hi(0x00000000), %g1
	or	%g2, %lo(0x00000000), %g2
	or	%g1, %lo(0x00000000), %g1
	sllx	%g2, 32, %g2
	ldx	[%g1 + %g2], %g1
	sethi	%hi(8 * _gloffset_WindowPos3svARB), %g2
	or	%g2, %lo(8 * _gloffset_WindowPos3svARB), %g2
	ldx	[%g1 + %g2], %g3
#else
	sethi	%hi(0x00000000), %g1
	ld	[%g1 + %lo(0x00000000)], %g1
	ld	[%g1 + (4 * _gloffset_WindowPos3svARB)], %g3
#endif
	jmpl	%g3, %g0
	nop

GLOBL_FN(GL_PREFIX(BindProgramNV))
.type glBindProgramNV,#function
GL_PREFIX(BindProgramNV):
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

GLOBL_FN(GL_PREFIX(DeleteProgramsNV))
.type glDeleteProgramsNV,#function
GL_PREFIX(DeleteProgramsNV):
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

GLOBL_FN(GL_PREFIX(ExecuteProgramNV))
.type glExecuteProgramNV,#function
GL_PREFIX(ExecuteProgramNV):
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

GLOBL_FN(GL_PREFIX(GenProgramsNV))
.type glGenProgramsNV,#function
GL_PREFIX(GenProgramsNV):
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

GLOBL_FN(GL_PREFIX(AreProgramsResidentNV))
.type glAreProgramsResidentNV,#function
GL_PREFIX(AreProgramsResidentNV):
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

GLOBL_FN(GL_PREFIX(RequestResidentProgramsNV))
.type glRequestResidentProgramsNV,#function
GL_PREFIX(RequestResidentProgramsNV):
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

GLOBL_FN(GL_PREFIX(GetProgramParameterfvNV))
.type glGetProgramParameterfvNV,#function
GL_PREFIX(GetProgramParameterfvNV):
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

GLOBL_FN(GL_PREFIX(GetProgramParameterdvNV))
.type glGetProgramParameterdvNV,#function
GL_PREFIX(GetProgramParameterdvNV):
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

GLOBL_FN(GL_PREFIX(GetProgramivNV))
.type glGetProgramivNV,#function
GL_PREFIX(GetProgramivNV):
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

GLOBL_FN(GL_PREFIX(GetProgramStringNV))
.type glGetProgramStringNV,#function
GL_PREFIX(GetProgramStringNV):
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

GLOBL_FN(GL_PREFIX(GetTrackMatrixivNV))
.type glGetTrackMatrixivNV,#function
GL_PREFIX(GetTrackMatrixivNV):
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

GLOBL_FN(GL_PREFIX(GetVertexAttribdvNV))
.type glGetVertexAttribdvNV,#function
GL_PREFIX(GetVertexAttribdvNV):
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

GLOBL_FN(GL_PREFIX(GetVertexAttribfvNV))
.type glGetVertexAttribfvNV,#function
GL_PREFIX(GetVertexAttribfvNV):
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

GLOBL_FN(GL_PREFIX(GetVertexAttribivNV))
.type glGetVertexAttribivNV,#function
GL_PREFIX(GetVertexAttribivNV):
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

GLOBL_FN(GL_PREFIX(GetVertexAttribPointervNV))
.type glGetVertexAttribPointervNV,#function
GL_PREFIX(GetVertexAttribPointervNV):
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

GLOBL_FN(GL_PREFIX(IsProgramNV))
.type glIsProgramNV,#function
GL_PREFIX(IsProgramNV):
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

GLOBL_FN(GL_PREFIX(LoadProgramNV))
.type glLoadProgramNV,#function
GL_PREFIX(LoadProgramNV):
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

GLOBL_FN(GL_PREFIX(ProgramParameter4fNV))
.type glProgramParameter4fNV,#function
GL_PREFIX(ProgramParameter4fNV):
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

GLOBL_FN(GL_PREFIX(ProgramParameter4dNV))
.type glProgramParameter4dNV,#function
GL_PREFIX(ProgramParameter4dNV):
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

GLOBL_FN(GL_PREFIX(ProgramParameter4dvNV))
.type glProgramParameter4dvNV,#function
GL_PREFIX(ProgramParameter4dvNV):
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

GLOBL_FN(GL_PREFIX(ProgramParameter4fvNV))
.type glProgramParameter4fvNV,#function
GL_PREFIX(ProgramParameter4fvNV):
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

GLOBL_FN(GL_PREFIX(ProgramParameters4dvNV))
.type glProgramParameters4dvNV,#function
GL_PREFIX(ProgramParameters4dvNV):
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

GLOBL_FN(GL_PREFIX(ProgramParameters4fvNV))
.type glProgramParameters4fvNV,#function
GL_PREFIX(ProgramParameters4fvNV):
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

GLOBL_FN(GL_PREFIX(TrackMatrixNV))
.type glTrackMatrixNV,#function
GL_PREFIX(TrackMatrixNV):
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

GLOBL_FN(GL_PREFIX(VertexAttribPointerNV))
.type glVertexAttribPointerNV,#function
GL_PREFIX(VertexAttribPointerNV):
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

GLOBL_FN(GL_PREFIX(VertexAttrib1sNV))
.type glVertexAttrib1sNV,#function
GL_PREFIX(VertexAttrib1sNV):
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

GLOBL_FN(GL_PREFIX(VertexAttrib1fNV))
.type glVertexAttrib1fNV,#function
GL_PREFIX(VertexAttrib1fNV):
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

GLOBL_FN(GL_PREFIX(VertexAttrib1dNV))
.type glVertexAttrib1dNV,#function
GL_PREFIX(VertexAttrib1dNV):
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

GLOBL_FN(GL_PREFIX(VertexAttrib2sNV))
.type glVertexAttrib2sNV,#function
GL_PREFIX(VertexAttrib2sNV):
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

GLOBL_FN(GL_PREFIX(VertexAttrib2fNV))
.type glVertexAttrib2fNV,#function
GL_PREFIX(VertexAttrib2fNV):
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

GLOBL_FN(GL_PREFIX(VertexAttrib2dNV))
.type glVertexAttrib2dNV,#function
GL_PREFIX(VertexAttrib2dNV):
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

GLOBL_FN(GL_PREFIX(VertexAttrib3sNV))
.type glVertexAttrib3sNV,#function
GL_PREFIX(VertexAttrib3sNV):
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

GLOBL_FN(GL_PREFIX(VertexAttrib3fNV))
.type glVertexAttrib3fNV,#function
GL_PREFIX(VertexAttrib3fNV):
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

GLOBL_FN(GL_PREFIX(VertexAttrib3dNV))
.type glVertexAttrib3dNV,#function
GL_PREFIX(VertexAttrib3dNV):
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

GLOBL_FN(GL_PREFIX(VertexAttrib4sNV))
.type glVertexAttrib4sNV,#function
GL_PREFIX(VertexAttrib4sNV):
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

GLOBL_FN(GL_PREFIX(VertexAttrib4fNV))
.type glVertexAttrib4fNV,#function
GL_PREFIX(VertexAttrib4fNV):
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

GLOBL_FN(GL_PREFIX(VertexAttrib4dNV))
.type glVertexAttrib4dNV,#function
GL_PREFIX(VertexAttrib4dNV):
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

GLOBL_FN(GL_PREFIX(VertexAttrib4ubNV))
.type glVertexAttrib4ubNV,#function
GL_PREFIX(VertexAttrib4ubNV):
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

GLOBL_FN(GL_PREFIX(VertexAttrib1svNV))
.type glVertexAttrib1svNV,#function
GL_PREFIX(VertexAttrib1svNV):
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

GLOBL_FN(GL_PREFIX(VertexAttrib1fvNV))
.type glVertexAttrib1fvNV,#function
GL_PREFIX(VertexAttrib1fvNV):
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

GLOBL_FN(GL_PREFIX(VertexAttrib1dvNV))
.type glVertexAttrib1dvNV,#function
GL_PREFIX(VertexAttrib1dvNV):
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

GLOBL_FN(GL_PREFIX(VertexAttrib2svNV))
.type glVertexAttrib2svNV,#function
GL_PREFIX(VertexAttrib2svNV):
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

GLOBL_FN(GL_PREFIX(VertexAttrib2fvNV))
.type glVertexAttrib2fvNV,#function
GL_PREFIX(VertexAttrib2fvNV):
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

GLOBL_FN(GL_PREFIX(VertexAttrib2dvNV))
.type glVertexAttrib2dvNV,#function
GL_PREFIX(VertexAttrib2dvNV):
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

GLOBL_FN(GL_PREFIX(VertexAttrib3svNV))
.type glVertexAttrib3svNV,#function
GL_PREFIX(VertexAttrib3svNV):
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

GLOBL_FN(GL_PREFIX(VertexAttrib3fvNV))
.type glVertexAttrib3fvNV,#function
GL_PREFIX(VertexAttrib3fvNV):
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

GLOBL_FN(GL_PREFIX(VertexAttrib3dvNV))
.type glVertexAttrib3dvNV,#function
GL_PREFIX(VertexAttrib3dvNV):
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

GLOBL_FN(GL_PREFIX(VertexAttrib4svNV))
.type glVertexAttrib4svNV,#function
GL_PREFIX(VertexAttrib4svNV):
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

GLOBL_FN(GL_PREFIX(VertexAttrib4fvNV))
.type glVertexAttrib4fvNV,#function
GL_PREFIX(VertexAttrib4fvNV):
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

GLOBL_FN(GL_PREFIX(VertexAttrib4dvNV))
.type glVertexAttrib4dvNV,#function
GL_PREFIX(VertexAttrib4dvNV):
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

GLOBL_FN(GL_PREFIX(VertexAttrib4ubvNV))
.type glVertexAttrib4ubvNV,#function
GL_PREFIX(VertexAttrib4ubvNV):
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

GLOBL_FN(GL_PREFIX(VertexAttribs1svNV))
.type glVertexAttribs1svNV,#function
GL_PREFIX(VertexAttribs1svNV):
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

GLOBL_FN(GL_PREFIX(VertexAttribs1fvNV))
.type glVertexAttribs1fvNV,#function
GL_PREFIX(VertexAttribs1fvNV):
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

GLOBL_FN(GL_PREFIX(VertexAttribs1dvNV))
.type glVertexAttribs1dvNV,#function
GL_PREFIX(VertexAttribs1dvNV):
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

GLOBL_FN(GL_PREFIX(VertexAttribs2svNV))
.type glVertexAttribs2svNV,#function
GL_PREFIX(VertexAttribs2svNV):
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

GLOBL_FN(GL_PREFIX(VertexAttribs2fvNV))
.type glVertexAttribs2fvNV,#function
GL_PREFIX(VertexAttribs2fvNV):
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

GLOBL_FN(GL_PREFIX(VertexAttribs2dvNV))
.type glVertexAttribs2dvNV,#function
GL_PREFIX(VertexAttribs2dvNV):
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

GLOBL_FN(GL_PREFIX(VertexAttribs3svNV))
.type glVertexAttribs3svNV,#function
GL_PREFIX(VertexAttribs3svNV):
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

GLOBL_FN(GL_PREFIX(VertexAttribs3fvNV))
.type glVertexAttribs3fvNV,#function
GL_PREFIX(VertexAttribs3fvNV):
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

GLOBL_FN(GL_PREFIX(VertexAttribs3dvNV))
.type glVertexAttribs3dvNV,#function
GL_PREFIX(VertexAttribs3dvNV):
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

GLOBL_FN(GL_PREFIX(VertexAttribs4svNV))
.type glVertexAttribs4svNV,#function
GL_PREFIX(VertexAttribs4svNV):
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

GLOBL_FN(GL_PREFIX(VertexAttribs4fvNV))
.type glVertexAttribs4fvNV,#function
GL_PREFIX(VertexAttribs4fvNV):
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

GLOBL_FN(GL_PREFIX(VertexAttribs4dvNV))
.type glVertexAttribs4dvNV,#function
GL_PREFIX(VertexAttribs4dvNV):
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

GLOBL_FN(GL_PREFIX(VertexAttribs4ubvNV))
.type glVertexAttribs4ubvNV,#function
GL_PREFIX(VertexAttribs4ubvNV):
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
	 nop

.globl _mesa_sparc_glapi_end
.type _mesa_sparc_glapi_end,#function
_mesa_sparc_glapi_end:


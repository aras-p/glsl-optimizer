/* DO NOT EDIT - This file generated automatically with glsparcasm.py script */
#include "glapioffsets.h"

#define GL_PREFIX(n) gl##n
#define GLOBL_FN(x) .globl x ; .type x,##function

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

GLOBL_FN(GL_PREFIX(EndList))
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

GLOBL_FN(GL_PREFIX(CallList))
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

GLOBL_FN(GL_PREFIX(CallLists))
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

GLOBL_FN(GL_PREFIX(DeleteLists))
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

GLOBL_FN(GL_PREFIX(GenLists))
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

GLOBL_FN(GL_PREFIX(ListBase))
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

GLOBL_FN(GL_PREFIX(Begin))
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

GLOBL_FN(GL_PREFIX(Bitmap))
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

GLOBL_FN(GL_PREFIX(Color3b))
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

GLOBL_FN(GL_PREFIX(Color3bv))
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

GLOBL_FN(GL_PREFIX(Color3d))
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

GLOBL_FN(GL_PREFIX(Color3dv))
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

GLOBL_FN(GL_PREFIX(Color3f))
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

GLOBL_FN(GL_PREFIX(Color3fv))
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

GLOBL_FN(GL_PREFIX(Color3i))
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

GLOBL_FN(GL_PREFIX(Color3iv))
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

GLOBL_FN(GL_PREFIX(Color3s))
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

GLOBL_FN(GL_PREFIX(Color3sv))
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

GLOBL_FN(GL_PREFIX(Color3ub))
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

GLOBL_FN(GL_PREFIX(Color3ubv))
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

GLOBL_FN(GL_PREFIX(Color3ui))
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

GLOBL_FN(GL_PREFIX(Color3uiv))
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

GLOBL_FN(GL_PREFIX(Color3us))
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

GLOBL_FN(GL_PREFIX(Color3usv))
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

GLOBL_FN(GL_PREFIX(Color4b))
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

GLOBL_FN(GL_PREFIX(Color4bv))
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

GLOBL_FN(GL_PREFIX(Color4d))
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

GLOBL_FN(GL_PREFIX(Color4dv))
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

GLOBL_FN(GL_PREFIX(Color4f))
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

GLOBL_FN(GL_PREFIX(Color4fv))
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

GLOBL_FN(GL_PREFIX(Color4i))
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

GLOBL_FN(GL_PREFIX(Color4iv))
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

GLOBL_FN(GL_PREFIX(Color4s))
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

GLOBL_FN(GL_PREFIX(Color4sv))
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

GLOBL_FN(GL_PREFIX(Color4ub))
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

GLOBL_FN(GL_PREFIX(Color4ubv))
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

GLOBL_FN(GL_PREFIX(Color4ui))
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

GLOBL_FN(GL_PREFIX(Color4uiv))
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

GLOBL_FN(GL_PREFIX(Color4us))
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

GLOBL_FN(GL_PREFIX(Color4usv))
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

GLOBL_FN(GL_PREFIX(EdgeFlag))
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

GLOBL_FN(GL_PREFIX(EdgeFlagv))
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

GLOBL_FN(GL_PREFIX(End))
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

GLOBL_FN(GL_PREFIX(Indexd))
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

GLOBL_FN(GL_PREFIX(Indexdv))
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

GLOBL_FN(GL_PREFIX(Indexf))
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

GLOBL_FN(GL_PREFIX(Indexfv))
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

GLOBL_FN(GL_PREFIX(Indexi))
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

GLOBL_FN(GL_PREFIX(Indexiv))
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

GLOBL_FN(GL_PREFIX(Indexs))
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

GLOBL_FN(GL_PREFIX(Indexsv))
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

GLOBL_FN(GL_PREFIX(Normal3b))
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

GLOBL_FN(GL_PREFIX(Normal3bv))
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

GLOBL_FN(GL_PREFIX(Normal3d))
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

GLOBL_FN(GL_PREFIX(Normal3dv))
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

GLOBL_FN(GL_PREFIX(Normal3f))
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

GLOBL_FN(GL_PREFIX(Normal3fv))
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

GLOBL_FN(GL_PREFIX(Normal3i))
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

GLOBL_FN(GL_PREFIX(Normal3iv))
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

GLOBL_FN(GL_PREFIX(Normal3s))
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

GLOBL_FN(GL_PREFIX(Normal3sv))
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

GLOBL_FN(GL_PREFIX(RasterPos2d))
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

GLOBL_FN(GL_PREFIX(RasterPos2dv))
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

GLOBL_FN(GL_PREFIX(RasterPos2f))
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

GLOBL_FN(GL_PREFIX(RasterPos2fv))
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

GLOBL_FN(GL_PREFIX(RasterPos2i))
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

GLOBL_FN(GL_PREFIX(RasterPos2iv))
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

GLOBL_FN(GL_PREFIX(RasterPos2s))
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

GLOBL_FN(GL_PREFIX(RasterPos2sv))
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

GLOBL_FN(GL_PREFIX(RasterPos3d))
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

GLOBL_FN(GL_PREFIX(RasterPos3dv))
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

GLOBL_FN(GL_PREFIX(RasterPos3f))
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

GLOBL_FN(GL_PREFIX(RasterPos3fv))
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

GLOBL_FN(GL_PREFIX(RasterPos3i))
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

GLOBL_FN(GL_PREFIX(RasterPos3iv))
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

GLOBL_FN(GL_PREFIX(RasterPos3s))
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

GLOBL_FN(GL_PREFIX(RasterPos3sv))
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

GLOBL_FN(GL_PREFIX(RasterPos4d))
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

GLOBL_FN(GL_PREFIX(RasterPos4dv))
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

GLOBL_FN(GL_PREFIX(RasterPos4f))
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

GLOBL_FN(GL_PREFIX(RasterPos4fv))
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

GLOBL_FN(GL_PREFIX(RasterPos4i))
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

GLOBL_FN(GL_PREFIX(RasterPos4iv))
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

GLOBL_FN(GL_PREFIX(RasterPos4s))
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

GLOBL_FN(GL_PREFIX(RasterPos4sv))
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

GLOBL_FN(GL_PREFIX(Rectd))
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

GLOBL_FN(GL_PREFIX(Rectdv))
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

GLOBL_FN(GL_PREFIX(Rectf))
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

GLOBL_FN(GL_PREFIX(Rectfv))
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

GLOBL_FN(GL_PREFIX(Recti))
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

GLOBL_FN(GL_PREFIX(Rectiv))
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

GLOBL_FN(GL_PREFIX(Rects))
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

GLOBL_FN(GL_PREFIX(Rectsv))
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

GLOBL_FN(GL_PREFIX(TexCoord1d))
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

GLOBL_FN(GL_PREFIX(TexCoord1dv))
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

GLOBL_FN(GL_PREFIX(TexCoord1f))
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

GLOBL_FN(GL_PREFIX(TexCoord1fv))
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

GLOBL_FN(GL_PREFIX(TexCoord1i))
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

GLOBL_FN(GL_PREFIX(TexCoord1iv))
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

GLOBL_FN(GL_PREFIX(TexCoord1s))
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

GLOBL_FN(GL_PREFIX(TexCoord1sv))
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

GLOBL_FN(GL_PREFIX(TexCoord2d))
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

GLOBL_FN(GL_PREFIX(TexCoord2dv))
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

GLOBL_FN(GL_PREFIX(TexCoord2f))
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

GLOBL_FN(GL_PREFIX(TexCoord2fv))
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

GLOBL_FN(GL_PREFIX(TexCoord2i))
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

GLOBL_FN(GL_PREFIX(TexCoord2iv))
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

GLOBL_FN(GL_PREFIX(TexCoord2s))
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

GLOBL_FN(GL_PREFIX(TexCoord2sv))
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

GLOBL_FN(GL_PREFIX(TexCoord3d))
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

GLOBL_FN(GL_PREFIX(TexCoord3dv))
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

GLOBL_FN(GL_PREFIX(TexCoord3f))
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

GLOBL_FN(GL_PREFIX(TexCoord3fv))
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

GLOBL_FN(GL_PREFIX(TexCoord3i))
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

GLOBL_FN(GL_PREFIX(TexCoord3iv))
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

GLOBL_FN(GL_PREFIX(TexCoord3s))
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

GLOBL_FN(GL_PREFIX(TexCoord3sv))
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

GLOBL_FN(GL_PREFIX(TexCoord4d))
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

GLOBL_FN(GL_PREFIX(TexCoord4dv))
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

GLOBL_FN(GL_PREFIX(TexCoord4f))
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

GLOBL_FN(GL_PREFIX(TexCoord4fv))
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

GLOBL_FN(GL_PREFIX(TexCoord4i))
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

GLOBL_FN(GL_PREFIX(TexCoord4iv))
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

GLOBL_FN(GL_PREFIX(TexCoord4s))
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

GLOBL_FN(GL_PREFIX(TexCoord4sv))
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

GLOBL_FN(GL_PREFIX(Vertex2d))
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

GLOBL_FN(GL_PREFIX(Vertex2dv))
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

GLOBL_FN(GL_PREFIX(Vertex2f))
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

GLOBL_FN(GL_PREFIX(Vertex2fv))
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

GLOBL_FN(GL_PREFIX(Vertex2i))
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

GLOBL_FN(GL_PREFIX(Vertex2iv))
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

GLOBL_FN(GL_PREFIX(Vertex2s))
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

GLOBL_FN(GL_PREFIX(Vertex2sv))
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

GLOBL_FN(GL_PREFIX(Vertex3d))
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

GLOBL_FN(GL_PREFIX(Vertex3dv))
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

GLOBL_FN(GL_PREFIX(Vertex3f))
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

GLOBL_FN(GL_PREFIX(Vertex3fv))
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

GLOBL_FN(GL_PREFIX(Vertex3i))
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

GLOBL_FN(GL_PREFIX(Vertex3iv))
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

GLOBL_FN(GL_PREFIX(Vertex3s))
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

GLOBL_FN(GL_PREFIX(Vertex3sv))
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

GLOBL_FN(GL_PREFIX(Vertex4d))
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

GLOBL_FN(GL_PREFIX(Vertex4dv))
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

GLOBL_FN(GL_PREFIX(Vertex4f))
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

GLOBL_FN(GL_PREFIX(Vertex4fv))
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

GLOBL_FN(GL_PREFIX(Vertex4i))
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

GLOBL_FN(GL_PREFIX(Vertex4iv))
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

GLOBL_FN(GL_PREFIX(Vertex4s))
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

GLOBL_FN(GL_PREFIX(Vertex4sv))
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

GLOBL_FN(GL_PREFIX(ClipPlane))
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

GLOBL_FN(GL_PREFIX(ColorMaterial))
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

GLOBL_FN(GL_PREFIX(CullFace))
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

GLOBL_FN(GL_PREFIX(Fogf))
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

GLOBL_FN(GL_PREFIX(Fogfv))
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

GLOBL_FN(GL_PREFIX(Fogi))
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

GLOBL_FN(GL_PREFIX(Fogiv))
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

GLOBL_FN(GL_PREFIX(FrontFace))
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

GLOBL_FN(GL_PREFIX(Hint))
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

GLOBL_FN(GL_PREFIX(Lightf))
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

GLOBL_FN(GL_PREFIX(Lightfv))
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

GLOBL_FN(GL_PREFIX(Lighti))
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

GLOBL_FN(GL_PREFIX(Lightiv))
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

GLOBL_FN(GL_PREFIX(LightModelf))
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

GLOBL_FN(GL_PREFIX(LightModelfv))
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

GLOBL_FN(GL_PREFIX(LightModeli))
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

GLOBL_FN(GL_PREFIX(LightModeliv))
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

GLOBL_FN(GL_PREFIX(LineStipple))
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

GLOBL_FN(GL_PREFIX(LineWidth))
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

GLOBL_FN(GL_PREFIX(Materialf))
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

GLOBL_FN(GL_PREFIX(Materialfv))
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

GLOBL_FN(GL_PREFIX(Materiali))
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

GLOBL_FN(GL_PREFIX(Materialiv))
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

GLOBL_FN(GL_PREFIX(PointSize))
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

GLOBL_FN(GL_PREFIX(PolygonMode))
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

GLOBL_FN(GL_PREFIX(PolygonStipple))
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

GLOBL_FN(GL_PREFIX(Scissor))
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

GLOBL_FN(GL_PREFIX(ShadeModel))
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

GLOBL_FN(GL_PREFIX(TexParameterf))
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

GLOBL_FN(GL_PREFIX(TexParameterfv))
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

GLOBL_FN(GL_PREFIX(TexParameteri))
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

GLOBL_FN(GL_PREFIX(TexParameteriv))
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

GLOBL_FN(GL_PREFIX(TexImage1D))
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

GLOBL_FN(GL_PREFIX(TexImage2D))
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

GLOBL_FN(GL_PREFIX(TexEnvf))
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

GLOBL_FN(GL_PREFIX(TexEnvfv))
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

GLOBL_FN(GL_PREFIX(TexEnvi))
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

GLOBL_FN(GL_PREFIX(TexEnviv))
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

GLOBL_FN(GL_PREFIX(TexGend))
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

GLOBL_FN(GL_PREFIX(TexGendv))
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

GLOBL_FN(GL_PREFIX(TexGenf))
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

GLOBL_FN(GL_PREFIX(TexGenfv))
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

GLOBL_FN(GL_PREFIX(TexGeni))
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

GLOBL_FN(GL_PREFIX(TexGeniv))
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

GLOBL_FN(GL_PREFIX(FeedbackBuffer))
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

GLOBL_FN(GL_PREFIX(SelectBuffer))
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

GLOBL_FN(GL_PREFIX(RenderMode))
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

GLOBL_FN(GL_PREFIX(InitNames))
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

GLOBL_FN(GL_PREFIX(LoadName))
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

GLOBL_FN(GL_PREFIX(PassThrough))
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

GLOBL_FN(GL_PREFIX(PopName))
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

GLOBL_FN(GL_PREFIX(PushName))
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

GLOBL_FN(GL_PREFIX(DrawBuffer))
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

GLOBL_FN(GL_PREFIX(Clear))
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

GLOBL_FN(GL_PREFIX(ClearAccum))
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

GLOBL_FN(GL_PREFIX(ClearIndex))
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

GLOBL_FN(GL_PREFIX(ClearColor))
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

GLOBL_FN(GL_PREFIX(ClearStencil))
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

GLOBL_FN(GL_PREFIX(ClearDepth))
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

GLOBL_FN(GL_PREFIX(StencilMask))
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

GLOBL_FN(GL_PREFIX(ColorMask))
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

GLOBL_FN(GL_PREFIX(DepthMask))
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

GLOBL_FN(GL_PREFIX(IndexMask))
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

GLOBL_FN(GL_PREFIX(Accum))
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

GLOBL_FN(GL_PREFIX(Disable))
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

GLOBL_FN(GL_PREFIX(Enable))
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

GLOBL_FN(GL_PREFIX(Finish))
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

GLOBL_FN(GL_PREFIX(Flush))
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

GLOBL_FN(GL_PREFIX(PopAttrib))
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

GLOBL_FN(GL_PREFIX(PushAttrib))
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

GLOBL_FN(GL_PREFIX(Map1d))
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

GLOBL_FN(GL_PREFIX(Map1f))
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

GLOBL_FN(GL_PREFIX(Map2d))
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

GLOBL_FN(GL_PREFIX(Map2f))
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

GLOBL_FN(GL_PREFIX(MapGrid1d))
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

GLOBL_FN(GL_PREFIX(MapGrid1f))
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

GLOBL_FN(GL_PREFIX(MapGrid2d))
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

GLOBL_FN(GL_PREFIX(MapGrid2f))
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

GLOBL_FN(GL_PREFIX(EvalCoord1d))
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

GLOBL_FN(GL_PREFIX(EvalCoord1dv))
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

GLOBL_FN(GL_PREFIX(EvalCoord1f))
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

GLOBL_FN(GL_PREFIX(EvalCoord1fv))
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

GLOBL_FN(GL_PREFIX(EvalCoord2d))
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

GLOBL_FN(GL_PREFIX(EvalCoord2dv))
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

GLOBL_FN(GL_PREFIX(EvalCoord2f))
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

GLOBL_FN(GL_PREFIX(EvalCoord2fv))
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

GLOBL_FN(GL_PREFIX(EvalMesh1))
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

GLOBL_FN(GL_PREFIX(EvalPoint1))
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

GLOBL_FN(GL_PREFIX(EvalMesh2))
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

GLOBL_FN(GL_PREFIX(EvalPoint2))
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

GLOBL_FN(GL_PREFIX(AlphaFunc))
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

GLOBL_FN(GL_PREFIX(BlendFunc))
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

GLOBL_FN(GL_PREFIX(LogicOp))
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

GLOBL_FN(GL_PREFIX(StencilFunc))
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

GLOBL_FN(GL_PREFIX(StencilOp))
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

GLOBL_FN(GL_PREFIX(DepthFunc))
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

GLOBL_FN(GL_PREFIX(PixelZoom))
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

GLOBL_FN(GL_PREFIX(PixelTransferf))
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

GLOBL_FN(GL_PREFIX(PixelTransferi))
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

GLOBL_FN(GL_PREFIX(PixelStoref))
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

GLOBL_FN(GL_PREFIX(PixelStorei))
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

GLOBL_FN(GL_PREFIX(PixelMapfv))
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

GLOBL_FN(GL_PREFIX(PixelMapuiv))
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

GLOBL_FN(GL_PREFIX(PixelMapusv))
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

GLOBL_FN(GL_PREFIX(ReadBuffer))
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

GLOBL_FN(GL_PREFIX(CopyPixels))
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

GLOBL_FN(GL_PREFIX(ReadPixels))
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

GLOBL_FN(GL_PREFIX(DrawPixels))
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

GLOBL_FN(GL_PREFIX(GetBooleanv))
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

GLOBL_FN(GL_PREFIX(GetClipPlane))
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

GLOBL_FN(GL_PREFIX(GetDoublev))
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

GLOBL_FN(GL_PREFIX(GetError))
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

GLOBL_FN(GL_PREFIX(GetFloatv))
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

GLOBL_FN(GL_PREFIX(GetIntegerv))
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

GLOBL_FN(GL_PREFIX(GetLightfv))
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

GLOBL_FN(GL_PREFIX(GetLightiv))
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

GLOBL_FN(GL_PREFIX(GetMapdv))
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

GLOBL_FN(GL_PREFIX(GetMapfv))
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

GLOBL_FN(GL_PREFIX(GetMapiv))
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

GLOBL_FN(GL_PREFIX(GetMaterialfv))
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

GLOBL_FN(GL_PREFIX(GetMaterialiv))
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

GLOBL_FN(GL_PREFIX(GetPixelMapfv))
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

GLOBL_FN(GL_PREFIX(GetPixelMapuiv))
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

GLOBL_FN(GL_PREFIX(GetPixelMapusv))
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

GLOBL_FN(GL_PREFIX(GetPolygonStipple))
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

GLOBL_FN(GL_PREFIX(GetString))
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

GLOBL_FN(GL_PREFIX(GetTexEnvfv))
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

GLOBL_FN(GL_PREFIX(GetTexEnviv))
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

GLOBL_FN(GL_PREFIX(GetTexGendv))
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

GLOBL_FN(GL_PREFIX(GetTexGenfv))
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

GLOBL_FN(GL_PREFIX(GetTexGeniv))
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

GLOBL_FN(GL_PREFIX(GetTexImage))
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

GLOBL_FN(GL_PREFIX(GetTexParameterfv))
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

GLOBL_FN(GL_PREFIX(GetTexParameteriv))
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

GLOBL_FN(GL_PREFIX(GetTexLevelParameterfv))
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

GLOBL_FN(GL_PREFIX(GetTexLevelParameteriv))
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

GLOBL_FN(GL_PREFIX(IsEnabled))
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

GLOBL_FN(GL_PREFIX(IsList))
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

GLOBL_FN(GL_PREFIX(DepthRange))
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

GLOBL_FN(GL_PREFIX(Frustum))
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

GLOBL_FN(GL_PREFIX(LoadIdentity))
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

GLOBL_FN(GL_PREFIX(LoadMatrixf))
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

GLOBL_FN(GL_PREFIX(LoadMatrixd))
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

GLOBL_FN(GL_PREFIX(MatrixMode))
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

GLOBL_FN(GL_PREFIX(MultMatrixf))
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

GLOBL_FN(GL_PREFIX(MultMatrixd))
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

GLOBL_FN(GL_PREFIX(Ortho))
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

GLOBL_FN(GL_PREFIX(PopMatrix))
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

GLOBL_FN(GL_PREFIX(PushMatrix))
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

GLOBL_FN(GL_PREFIX(Rotated))
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

GLOBL_FN(GL_PREFIX(Rotatef))
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

GLOBL_FN(GL_PREFIX(Scaled))
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

GLOBL_FN(GL_PREFIX(Scalef))
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

GLOBL_FN(GL_PREFIX(Translated))
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

GLOBL_FN(GL_PREFIX(Translatef))
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

GLOBL_FN(GL_PREFIX(Viewport))
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

GLOBL_FN(GL_PREFIX(ArrayElement))
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

GLOBL_FN(GL_PREFIX(ColorPointer))
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

GLOBL_FN(GL_PREFIX(DisableClientState))
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

GLOBL_FN(GL_PREFIX(DrawArrays))
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

GLOBL_FN(GL_PREFIX(DrawElements))
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

GLOBL_FN(GL_PREFIX(EdgeFlagPointer))
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

GLOBL_FN(GL_PREFIX(EnableClientState))
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

GLOBL_FN(GL_PREFIX(GetPointerv))
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

GLOBL_FN(GL_PREFIX(IndexPointer))
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

GLOBL_FN(GL_PREFIX(InterleavedArrays))
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

GLOBL_FN(GL_PREFIX(NormalPointer))
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

GLOBL_FN(GL_PREFIX(TexCoordPointer))
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

GLOBL_FN(GL_PREFIX(VertexPointer))
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

GLOBL_FN(GL_PREFIX(PolygonOffset))
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

GLOBL_FN(GL_PREFIX(CopyTexImage1D))
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

GLOBL_FN(GL_PREFIX(CopyTexImage2D))
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

GLOBL_FN(GL_PREFIX(CopyTexSubImage1D))
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

GLOBL_FN(GL_PREFIX(CopyTexSubImage2D))
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

GLOBL_FN(GL_PREFIX(TexSubImage1D))
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

GLOBL_FN(GL_PREFIX(TexSubImage2D))
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

GLOBL_FN(GL_PREFIX(AreTexturesResident))
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

GLOBL_FN(GL_PREFIX(BindTexture))
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

GLOBL_FN(GL_PREFIX(DeleteTextures))
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

GLOBL_FN(GL_PREFIX(GenTextures))
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

GLOBL_FN(GL_PREFIX(IsTexture))
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

GLOBL_FN(GL_PREFIX(PrioritizeTextures))
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

GLOBL_FN(GL_PREFIX(Indexub))
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

GLOBL_FN(GL_PREFIX(Indexubv))
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

GLOBL_FN(GL_PREFIX(PopClientAttrib))
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

GLOBL_FN(GL_PREFIX(PushClientAttrib))
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

GLOBL_FN(GL_PREFIX(BlendColor))
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

GLOBL_FN(GL_PREFIX(BlendEquation))
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

GLOBL_FN(GL_PREFIX(DrawRangeElements))
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

GLOBL_FN(GL_PREFIX(ColorTable))
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

GLOBL_FN(GL_PREFIX(ColorTableParameterfv))
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

GLOBL_FN(GL_PREFIX(ColorTableParameteriv))
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

GLOBL_FN(GL_PREFIX(CopyColorTable))
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

GLOBL_FN(GL_PREFIX(GetColorTable))
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

GLOBL_FN(GL_PREFIX(GetColorTableParameterfv))
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

GLOBL_FN(GL_PREFIX(GetColorTableParameteriv))
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

GLOBL_FN(GL_PREFIX(ColorSubTable))
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

GLOBL_FN(GL_PREFIX(CopyColorSubTable))
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

GLOBL_FN(GL_PREFIX(ConvolutionFilter1D))
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

GLOBL_FN(GL_PREFIX(ConvolutionFilter2D))
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

GLOBL_FN(GL_PREFIX(ConvolutionParameterf))
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

GLOBL_FN(GL_PREFIX(ConvolutionParameterfv))
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

GLOBL_FN(GL_PREFIX(ConvolutionParameteri))
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

GLOBL_FN(GL_PREFIX(ConvolutionParameteriv))
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

GLOBL_FN(GL_PREFIX(CopyConvolutionFilter1D))
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

GLOBL_FN(GL_PREFIX(CopyConvolutionFilter2D))
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

GLOBL_FN(GL_PREFIX(GetConvolutionFilter))
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

GLOBL_FN(GL_PREFIX(GetConvolutionParameterfv))
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

GLOBL_FN(GL_PREFIX(GetConvolutionParameteriv))
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

GLOBL_FN(GL_PREFIX(GetSeparableFilter))
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

GLOBL_FN(GL_PREFIX(SeparableFilter2D))
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

GLOBL_FN(GL_PREFIX(GetHistogram))
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

GLOBL_FN(GL_PREFIX(GetHistogramParameterfv))
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

GLOBL_FN(GL_PREFIX(GetHistogramParameteriv))
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

GLOBL_FN(GL_PREFIX(GetMinmax))
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

GLOBL_FN(GL_PREFIX(GetMinmaxParameterfv))
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

GLOBL_FN(GL_PREFIX(GetMinmaxParameteriv))
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

GLOBL_FN(GL_PREFIX(Histogram))
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

GLOBL_FN(GL_PREFIX(Minmax))
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

GLOBL_FN(GL_PREFIX(ResetHistogram))
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

GLOBL_FN(GL_PREFIX(ResetMinmax))
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

GLOBL_FN(GL_PREFIX(TexImage3D))
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

GLOBL_FN(GL_PREFIX(TexSubImage3D))
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

GLOBL_FN(GL_PREFIX(CopyTexSubImage3D))
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

GLOBL_FN(GL_PREFIX(ActiveTextureARB))
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

GLOBL_FN(GL_PREFIX(ClientActiveTextureARB))
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

GLOBL_FN(GL_PREFIX(MultiTexCoord1dARB))
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

GLOBL_FN(GL_PREFIX(MultiTexCoord1dvARB))
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

GLOBL_FN(GL_PREFIX(MultiTexCoord1fARB))
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

GLOBL_FN(GL_PREFIX(MultiTexCoord1fvARB))
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

GLOBL_FN(GL_PREFIX(MultiTexCoord1iARB))
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

GLOBL_FN(GL_PREFIX(MultiTexCoord1ivARB))
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

GLOBL_FN(GL_PREFIX(MultiTexCoord1sARB))
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

GLOBL_FN(GL_PREFIX(MultiTexCoord1svARB))
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

GLOBL_FN(GL_PREFIX(MultiTexCoord2dARB))
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

GLOBL_FN(GL_PREFIX(MultiTexCoord2dvARB))
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

GLOBL_FN(GL_PREFIX(MultiTexCoord2fARB))
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

GLOBL_FN(GL_PREFIX(MultiTexCoord2fvARB))
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

GLOBL_FN(GL_PREFIX(MultiTexCoord2iARB))
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

GLOBL_FN(GL_PREFIX(MultiTexCoord2ivARB))
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

GLOBL_FN(GL_PREFIX(MultiTexCoord2sARB))
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

GLOBL_FN(GL_PREFIX(MultiTexCoord2svARB))
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

GLOBL_FN(GL_PREFIX(MultiTexCoord3dARB))
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

GLOBL_FN(GL_PREFIX(MultiTexCoord3dvARB))
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

GLOBL_FN(GL_PREFIX(MultiTexCoord3fARB))
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

GLOBL_FN(GL_PREFIX(MultiTexCoord3fvARB))
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

GLOBL_FN(GL_PREFIX(MultiTexCoord3iARB))
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

GLOBL_FN(GL_PREFIX(MultiTexCoord3ivARB))
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

GLOBL_FN(GL_PREFIX(MultiTexCoord3sARB))
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

GLOBL_FN(GL_PREFIX(MultiTexCoord3svARB))
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

GLOBL_FN(GL_PREFIX(MultiTexCoord4dARB))
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

GLOBL_FN(GL_PREFIX(MultiTexCoord4dvARB))
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

GLOBL_FN(GL_PREFIX(MultiTexCoord4fARB))
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

GLOBL_FN(GL_PREFIX(MultiTexCoord4fvARB))
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

GLOBL_FN(GL_PREFIX(MultiTexCoord4iARB))
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

GLOBL_FN(GL_PREFIX(MultiTexCoord4ivARB))
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

GLOBL_FN(GL_PREFIX(MultiTexCoord4sARB))
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

GLOBL_FN(GL_PREFIX(MultiTexCoord4svARB))
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

GLOBL_FN(GL_PREFIX(LoadTransposeMatrixfARB))
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

GLOBL_FN(GL_PREFIX(LoadTransposeMatrixdARB))
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

GLOBL_FN(GL_PREFIX(MultTransposeMatrixfARB))
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

GLOBL_FN(GL_PREFIX(MultTransposeMatrixdARB))
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

GLOBL_FN(GL_PREFIX(SampleCoverageARB))
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

GLOBL_FN(GL_PREFIX(CompressedTexImage3DARB))
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

GLOBL_FN(GL_PREFIX(CompressedTexImage2DARB))
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

GLOBL_FN(GL_PREFIX(CompressedTexImage1DARB))
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

GLOBL_FN(GL_PREFIX(CompressedTexSubImage3DARB))
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

GLOBL_FN(GL_PREFIX(CompressedTexSubImage2DARB))
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

GLOBL_FN(GL_PREFIX(CompressedTexSubImage1DARB))
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

GLOBL_FN(GL_PREFIX(GetCompressedTexImageARB))
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

GLOBL_FN(GL_PREFIX(BlendColorEXT))
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

GLOBL_FN(GL_PREFIX(PolygonOffsetEXT))
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

GLOBL_FN(GL_PREFIX(TexImage3DEXT))
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

GLOBL_FN(GL_PREFIX(TexSubImage3DEXT))
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

GLOBL_FN(GL_PREFIX(GetTexFilterFuncSGIS))
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

GLOBL_FN(GL_PREFIX(TexFilterFuncSGIS))
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

GLOBL_FN(GL_PREFIX(TexSubImage1DEXT))
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

GLOBL_FN(GL_PREFIX(TexSubImage2DEXT))
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

GLOBL_FN(GL_PREFIX(CopyTexImage1DEXT))
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

GLOBL_FN(GL_PREFIX(CopyTexImage2DEXT))
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

GLOBL_FN(GL_PREFIX(CopyTexSubImage1DEXT))
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

GLOBL_FN(GL_PREFIX(CopyTexSubImage2DEXT))
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

GLOBL_FN(GL_PREFIX(CopyTexSubImage3DEXT))
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

GLOBL_FN(GL_PREFIX(GetHistogramEXT))
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

GLOBL_FN(GL_PREFIX(GetHistogramParameterfvEXT))
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

GLOBL_FN(GL_PREFIX(GetHistogramParameterivEXT))
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

GLOBL_FN(GL_PREFIX(GetMinmaxEXT))
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

GLOBL_FN(GL_PREFIX(GetMinmaxParameterfvEXT))
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

GLOBL_FN(GL_PREFIX(GetMinmaxParameterivEXT))
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

GLOBL_FN(GL_PREFIX(HistogramEXT))
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

GLOBL_FN(GL_PREFIX(MinmaxEXT))
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

GLOBL_FN(GL_PREFIX(ResetHistogramEXT))
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

GLOBL_FN(GL_PREFIX(ResetMinmaxEXT))
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

GLOBL_FN(GL_PREFIX(ConvolutionFilter1DEXT))
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

GLOBL_FN(GL_PREFIX(ConvolutionFilter2DEXT))
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

GLOBL_FN(GL_PREFIX(ConvolutionParameterfEXT))
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

GLOBL_FN(GL_PREFIX(ConvolutionParameterfvEXT))
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

GLOBL_FN(GL_PREFIX(ConvolutionParameteriEXT))
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

GLOBL_FN(GL_PREFIX(ConvolutionParameterivEXT))
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

GLOBL_FN(GL_PREFIX(CopyConvolutionFilter1DEXT))
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

GLOBL_FN(GL_PREFIX(CopyConvolutionFilter2DEXT))
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

GLOBL_FN(GL_PREFIX(GetConvolutionFilterEXT))
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

GLOBL_FN(GL_PREFIX(GetConvolutionParameterfvEXT))
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

GLOBL_FN(GL_PREFIX(GetConvolutionParameterivEXT))
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

GLOBL_FN(GL_PREFIX(GetSeparableFilterEXT))
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

GLOBL_FN(GL_PREFIX(SeparableFilter2DEXT))
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

GLOBL_FN(GL_PREFIX(ColorTableSGI))
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

GLOBL_FN(GL_PREFIX(ColorTableParameterfvSGI))
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

GLOBL_FN(GL_PREFIX(ColorTableParameterivSGI))
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

GLOBL_FN(GL_PREFIX(CopyColorTableSGI))
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

GLOBL_FN(GL_PREFIX(GetColorTableSGI))
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

GLOBL_FN(GL_PREFIX(GetColorTableParameterfvSGI))
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

GLOBL_FN(GL_PREFIX(GetColorTableParameterivSGI))
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

GLOBL_FN(GL_PREFIX(PixelTexGenSGIX))
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

GLOBL_FN(GL_PREFIX(PixelTexGenParameteriSGIS))
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

GLOBL_FN(GL_PREFIX(PixelTexGenParameterivSGIS))
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

GLOBL_FN(GL_PREFIX(PixelTexGenParameterfSGIS))
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

GLOBL_FN(GL_PREFIX(PixelTexGenParameterfvSGIS))
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

GLOBL_FN(GL_PREFIX(GetPixelTexGenParameterivSGIS))
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

GLOBL_FN(GL_PREFIX(GetPixelTexGenParameterfvSGIS))
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

GLOBL_FN(GL_PREFIX(TexImage4DSGIS))
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

GLOBL_FN(GL_PREFIX(TexSubImage4DSGIS))
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

GLOBL_FN(GL_PREFIX(AreTexturesResidentEXT))
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

GLOBL_FN(GL_PREFIX(BindTextureEXT))
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

GLOBL_FN(GL_PREFIX(DeleteTexturesEXT))
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

GLOBL_FN(GL_PREFIX(GenTexturesEXT))
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

GLOBL_FN(GL_PREFIX(IsTextureEXT))
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

GLOBL_FN(GL_PREFIX(PrioritizeTexturesEXT))
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

GLOBL_FN(GL_PREFIX(DetailTexFuncSGIS))
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

GLOBL_FN(GL_PREFIX(GetDetailTexFuncSGIS))
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

GLOBL_FN(GL_PREFIX(SharpenTexFuncSGIS))
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

GLOBL_FN(GL_PREFIX(GetSharpenTexFuncSGIS))
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

GLOBL_FN(GL_PREFIX(ArrayElementEXT))
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

GLOBL_FN(GL_PREFIX(ColorPointerEXT))
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

GLOBL_FN(GL_PREFIX(DrawArraysEXT))
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

GLOBL_FN(GL_PREFIX(EdgeFlagPointerEXT))
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

GLOBL_FN(GL_PREFIX(GetPointervEXT))
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

GLOBL_FN(GL_PREFIX(IndexPointerEXT))
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

GLOBL_FN(GL_PREFIX(NormalPointerEXT))
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

GLOBL_FN(GL_PREFIX(TexCoordPointerEXT))
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

GLOBL_FN(GL_PREFIX(VertexPointerEXT))
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

GLOBL_FN(GL_PREFIX(BlendEquationEXT))
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

GLOBL_FN(GL_PREFIX(SpriteParameterfSGIX))
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

GLOBL_FN(GL_PREFIX(SpriteParameterfvSGIX))
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

GLOBL_FN(GL_PREFIX(SpriteParameteriSGIX))
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

GLOBL_FN(GL_PREFIX(SpriteParameterivSGIX))
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

GLOBL_FN(GL_PREFIX(PointParameterfEXT))
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

GLOBL_FN(GL_PREFIX(PointParameterfvEXT))
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

GLOBL_FN(GL_PREFIX(PointParameterfSGIS))
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

GLOBL_FN(GL_PREFIX(PointParameterfvSGIS))
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

GLOBL_FN(GL_PREFIX(GetInstrumentsSGIX))
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

GLOBL_FN(GL_PREFIX(InstrumentsBufferSGIX))
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

GLOBL_FN(GL_PREFIX(PollInstrumentsSGIX))
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

GLOBL_FN(GL_PREFIX(ReadInstrumentsSGIX))
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

GLOBL_FN(GL_PREFIX(StartInstrumentsSGIX))
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

GLOBL_FN(GL_PREFIX(StopInstrumentsSGIX))
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

GLOBL_FN(GL_PREFIX(FrameZoomSGIX))
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

GLOBL_FN(GL_PREFIX(TagSampleBufferSGIX))
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

GLOBL_FN(GL_PREFIX(ReferencePlaneSGIX))
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

GLOBL_FN(GL_PREFIX(FlushRasterSGIX))
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

GLOBL_FN(GL_PREFIX(ColorSubTableEXT))
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

GLOBL_FN(GL_PREFIX(CopyColorSubTableEXT))
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

GLOBL_FN(GL_PREFIX(HintPGI))
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

GLOBL_FN(GL_PREFIX(ColorTableEXT))
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

GLOBL_FN(GL_PREFIX(GetColorTableEXT))
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

GLOBL_FN(GL_PREFIX(GetColorTableParameterivEXT))
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

GLOBL_FN(GL_PREFIX(GetColorTableParameterfvEXT))
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

GLOBL_FN(GL_PREFIX(GetListParameterfvSGIX))
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

GLOBL_FN(GL_PREFIX(GetListParameterivSGIX))
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

GLOBL_FN(GL_PREFIX(ListParameterfSGIX))
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

GLOBL_FN(GL_PREFIX(ListParameterfvSGIX))
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

GLOBL_FN(GL_PREFIX(ListParameteriSGIX))
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

GLOBL_FN(GL_PREFIX(ListParameterivSGIX))
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

GLOBL_FN(GL_PREFIX(IndexMaterialEXT))
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

GLOBL_FN(GL_PREFIX(IndexFuncEXT))
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

GLOBL_FN(GL_PREFIX(LockArraysEXT))
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

GLOBL_FN(GL_PREFIX(UnlockArraysEXT))
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

GLOBL_FN(GL_PREFIX(CullParameterdvEXT))
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

GLOBL_FN(GL_PREFIX(CullParameterfvEXT))
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

GLOBL_FN(GL_PREFIX(FragmentColorMaterialSGIX))
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

GLOBL_FN(GL_PREFIX(FragmentLightfSGIX))
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

GLOBL_FN(GL_PREFIX(FragmentLightfvSGIX))
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

GLOBL_FN(GL_PREFIX(FragmentLightiSGIX))
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

GLOBL_FN(GL_PREFIX(FragmentLightivSGIX))
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

GLOBL_FN(GL_PREFIX(FragmentLightModelfSGIX))
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

GLOBL_FN(GL_PREFIX(FragmentLightModelfvSGIX))
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

GLOBL_FN(GL_PREFIX(FragmentLightModeliSGIX))
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

GLOBL_FN(GL_PREFIX(FragmentLightModelivSGIX))
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

GLOBL_FN(GL_PREFIX(FragmentMaterialfSGIX))
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

GLOBL_FN(GL_PREFIX(FragmentMaterialfvSGIX))
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

GLOBL_FN(GL_PREFIX(FragmentMaterialiSGIX))
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

GLOBL_FN(GL_PREFIX(FragmentMaterialivSGIX))
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

GLOBL_FN(GL_PREFIX(GetFragmentLightfvSGIX))
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

GLOBL_FN(GL_PREFIX(GetFragmentLightivSGIX))
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

GLOBL_FN(GL_PREFIX(GetFragmentMaterialfvSGIX))
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

GLOBL_FN(GL_PREFIX(GetFragmentMaterialivSGIX))
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

GLOBL_FN(GL_PREFIX(LightEnviSGIX))
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

GLOBL_FN(GL_PREFIX(DrawRangeElementsEXT))
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

GLOBL_FN(GL_PREFIX(SecondaryColor3bEXT))
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

GLOBL_FN(GL_PREFIX(SecondaryColor3bvEXT))
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

GLOBL_FN(GL_PREFIX(SecondaryColor3dEXT))
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

GLOBL_FN(GL_PREFIX(SecondaryColor3dvEXT))
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

GLOBL_FN(GL_PREFIX(SecondaryColor3fEXT))
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

GLOBL_FN(GL_PREFIX(SecondaryColor3fvEXT))
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

GLOBL_FN(GL_PREFIX(SecondaryColor3iEXT))
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

GLOBL_FN(GL_PREFIX(SecondaryColor3ivEXT))
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

GLOBL_FN(GL_PREFIX(SecondaryColor3sEXT))
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

GLOBL_FN(GL_PREFIX(SecondaryColor3svEXT))
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

GLOBL_FN(GL_PREFIX(SecondaryColor3ubEXT))
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

GLOBL_FN(GL_PREFIX(SecondaryColor3ubvEXT))
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

GLOBL_FN(GL_PREFIX(SecondaryColor3uiEXT))
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

GLOBL_FN(GL_PREFIX(SecondaryColor3uivEXT))
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

GLOBL_FN(GL_PREFIX(SecondaryColor3usEXT))
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

GLOBL_FN(GL_PREFIX(SecondaryColor3usvEXT))
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

GLOBL_FN(GL_PREFIX(SecondaryColorPointerEXT))
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

GLOBL_FN(GL_PREFIX(FogCoordfEXT))
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

GLOBL_FN(GL_PREFIX(FogCoordfvEXT))
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

GLOBL_FN(GL_PREFIX(FogCoorddEXT))
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

GLOBL_FN(GL_PREFIX(FogCoorddvEXT))
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

GLOBL_FN(GL_PREFIX(FogCoordPointerEXT))
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

GLOBL_FN(GL_PREFIX(BlendFuncSeparateEXT))
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

GLOBL_FN(GL_PREFIX(BlendFuncSeparateINGR))
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

GLOBL_FN(GL_PREFIX(VertexWeightfEXT))
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

GLOBL_FN(GL_PREFIX(VertexWeightfvEXT))
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

GLOBL_FN(GL_PREFIX(VertexWeightPointerEXT))
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

GLOBL_FN(GL_PREFIX(FlushVertexArrayRangeNV))
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

GLOBL_FN(GL_PREFIX(VertexArrayRangeNV))
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

GLOBL_FN(GL_PREFIX(CombinerParameterfvNV))
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

GLOBL_FN(GL_PREFIX(CombinerParameterfNV))
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

GLOBL_FN(GL_PREFIX(CombinerParameterivNV))
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

GLOBL_FN(GL_PREFIX(CombinerParameteriNV))
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

GLOBL_FN(GL_PREFIX(CombinerInputNV))
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

GLOBL_FN(GL_PREFIX(CombinerOutputNV))
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

GLOBL_FN(GL_PREFIX(FinalCombinerInputNV))
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

GLOBL_FN(GL_PREFIX(GetCombinerInputParameterfvNV))
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

GLOBL_FN(GL_PREFIX(GetCombinerInputParameterivNV))
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

GLOBL_FN(GL_PREFIX(GetCombinerOutputParameterfvNV))
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

GLOBL_FN(GL_PREFIX(GetCombinerOutputParameterivNV))
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

GLOBL_FN(GL_PREFIX(GetFinalCombinerInputParameterfvNV))
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

GLOBL_FN(GL_PREFIX(GetFinalCombinerInputParameterivNV))
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

GLOBL_FN(GL_PREFIX(ResizeBuffersMESA))
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

GLOBL_FN(GL_PREFIX(WindowPos2dMESA))
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

GLOBL_FN(GL_PREFIX(WindowPos2dvMESA))
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

GLOBL_FN(GL_PREFIX(WindowPos2fMESA))
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

GLOBL_FN(GL_PREFIX(WindowPos2fvMESA))
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

GLOBL_FN(GL_PREFIX(WindowPos2iMESA))
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

GLOBL_FN(GL_PREFIX(WindowPos2ivMESA))
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

GLOBL_FN(GL_PREFIX(WindowPos2sMESA))
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

GLOBL_FN(GL_PREFIX(WindowPos2svMESA))
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

GLOBL_FN(GL_PREFIX(WindowPos3dMESA))
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

GLOBL_FN(GL_PREFIX(WindowPos3dvMESA))
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

GLOBL_FN(GL_PREFIX(WindowPos3fMESA))
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

GLOBL_FN(GL_PREFIX(WindowPos3fvMESA))
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

GLOBL_FN(GL_PREFIX(WindowPos3iMESA))
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

GLOBL_FN(GL_PREFIX(WindowPos3ivMESA))
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

GLOBL_FN(GL_PREFIX(WindowPos3sMESA))
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

GLOBL_FN(GL_PREFIX(WindowPos3svMESA))
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

GLOBL_FN(GL_PREFIX(WindowPos4dMESA))
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

GLOBL_FN(GL_PREFIX(WindowPos4dvMESA))
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

GLOBL_FN(GL_PREFIX(WindowPos4fMESA))
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

GLOBL_FN(GL_PREFIX(WindowPos4fvMESA))
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

GLOBL_FN(GL_PREFIX(WindowPos4iMESA))
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

GLOBL_FN(GL_PREFIX(WindowPos4ivMESA))
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

GLOBL_FN(GL_PREFIX(WindowPos4sMESA))
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

GLOBL_FN(GL_PREFIX(WindowPos4svMESA))
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

GLOBL_FN(GL_PREFIX(TbufferMask3DFX))
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

GLOBL_FN(GL_PREFIX(SampleMaskSGIS))
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

GLOBL_FN(GL_PREFIX(SamplePatternSGIS))
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

.globl _mesa_sparc_glapi_end
.type _mesa_sparc_glapi_end,#function
_mesa_sparc_glapi_end:


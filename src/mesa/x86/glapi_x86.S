/* DO NOT EDIT - This file generated automatically with glx86asm.py script */
#include "assyntax.h"
#include "glapioffsets.h"

#ifndef __WIN32__

#if defined(USE_MGL_NAMESPACE)
#define GL_PREFIX(n) GLNAME(CONCAT(mgl,n))
#else
#define GL_PREFIX(n) GLNAME(CONCAT(gl,n))
#endif

#define GL_OFFSET(x) CODEPTR(REGOFF(4 * x, EAX))

#if defined(GNU_ASSEMBLER) && !defined(DJGPP)
#define GLOBL_FN(x) GLOBL x ; .type x,@function
#else
#define GLOBL_FN(x) GLOBL x
#endif


EXTERN GLNAME(_glapi_Dispatch)

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(NewList))
GL_PREFIX(NewList):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_NewList))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EndList))
GL_PREFIX(EndList):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_EndList))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CallList))
GL_PREFIX(CallList):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CallList))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CallLists))
GL_PREFIX(CallLists):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CallLists))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DeleteLists))
GL_PREFIX(DeleteLists):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_DeleteLists))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GenLists))
GL_PREFIX(GenLists):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GenLists))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ListBase))
GL_PREFIX(ListBase):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ListBase))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Begin))
GL_PREFIX(Begin):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Begin))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Bitmap))
GL_PREFIX(Bitmap):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Bitmap))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3b))
GL_PREFIX(Color3b):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color3b))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3bv))
GL_PREFIX(Color3bv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color3bv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3d))
GL_PREFIX(Color3d):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color3d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3dv))
GL_PREFIX(Color3dv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color3dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3f))
GL_PREFIX(Color3f):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color3f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3fv))
GL_PREFIX(Color3fv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color3fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3i))
GL_PREFIX(Color3i):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color3i))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3iv))
GL_PREFIX(Color3iv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color3iv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3s))
GL_PREFIX(Color3s):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color3s))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3sv))
GL_PREFIX(Color3sv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color3sv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3ub))
GL_PREFIX(Color3ub):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color3ub))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3ubv))
GL_PREFIX(Color3ubv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color3ubv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3ui))
GL_PREFIX(Color3ui):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color3ui))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3uiv))
GL_PREFIX(Color3uiv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color3uiv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3us))
GL_PREFIX(Color3us):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color3us))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3usv))
GL_PREFIX(Color3usv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color3usv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4b))
GL_PREFIX(Color4b):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color4b))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4bv))
GL_PREFIX(Color4bv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color4bv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4d))
GL_PREFIX(Color4d):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color4d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4dv))
GL_PREFIX(Color4dv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color4dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4f))
GL_PREFIX(Color4f):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color4f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4fv))
GL_PREFIX(Color4fv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color4fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4i))
GL_PREFIX(Color4i):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color4i))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4iv))
GL_PREFIX(Color4iv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color4iv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4s))
GL_PREFIX(Color4s):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color4s))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4sv))
GL_PREFIX(Color4sv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color4sv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4ub))
GL_PREFIX(Color4ub):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color4ub))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4ubv))
GL_PREFIX(Color4ubv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color4ubv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4ui))
GL_PREFIX(Color4ui):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color4ui))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4uiv))
GL_PREFIX(Color4uiv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color4uiv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4us))
GL_PREFIX(Color4us):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color4us))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4usv))
GL_PREFIX(Color4usv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color4usv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EdgeFlag))
GL_PREFIX(EdgeFlag):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_EdgeFlag))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EdgeFlagv))
GL_PREFIX(EdgeFlagv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_EdgeFlagv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(End))
GL_PREFIX(End):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_End))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Indexd))
GL_PREFIX(Indexd):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Indexd))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Indexdv))
GL_PREFIX(Indexdv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Indexdv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Indexf))
GL_PREFIX(Indexf):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Indexf))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Indexfv))
GL_PREFIX(Indexfv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Indexfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Indexi))
GL_PREFIX(Indexi):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Indexi))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Indexiv))
GL_PREFIX(Indexiv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Indexiv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Indexs))
GL_PREFIX(Indexs):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Indexs))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Indexsv))
GL_PREFIX(Indexsv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Indexsv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Normal3b))
GL_PREFIX(Normal3b):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Normal3b))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Normal3bv))
GL_PREFIX(Normal3bv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Normal3bv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Normal3d))
GL_PREFIX(Normal3d):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Normal3d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Normal3dv))
GL_PREFIX(Normal3dv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Normal3dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Normal3f))
GL_PREFIX(Normal3f):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Normal3f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Normal3fv))
GL_PREFIX(Normal3fv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Normal3fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Normal3i))
GL_PREFIX(Normal3i):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Normal3i))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Normal3iv))
GL_PREFIX(Normal3iv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Normal3iv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Normal3s))
GL_PREFIX(Normal3s):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Normal3s))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Normal3sv))
GL_PREFIX(Normal3sv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Normal3sv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos2d))
GL_PREFIX(RasterPos2d):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos2d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos2dv))
GL_PREFIX(RasterPos2dv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos2dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos2f))
GL_PREFIX(RasterPos2f):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos2f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos2fv))
GL_PREFIX(RasterPos2fv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos2fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos2i))
GL_PREFIX(RasterPos2i):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos2i))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos2iv))
GL_PREFIX(RasterPos2iv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos2iv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos2s))
GL_PREFIX(RasterPos2s):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos2s))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos2sv))
GL_PREFIX(RasterPos2sv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos2sv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos3d))
GL_PREFIX(RasterPos3d):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos3d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos3dv))
GL_PREFIX(RasterPos3dv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos3dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos3f))
GL_PREFIX(RasterPos3f):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos3f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos3fv))
GL_PREFIX(RasterPos3fv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos3fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos3i))
GL_PREFIX(RasterPos3i):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos3i))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos3iv))
GL_PREFIX(RasterPos3iv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos3iv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos3s))
GL_PREFIX(RasterPos3s):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos3s))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos3sv))
GL_PREFIX(RasterPos3sv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos3sv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos4d))
GL_PREFIX(RasterPos4d):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos4d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos4dv))
GL_PREFIX(RasterPos4dv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos4dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos4f))
GL_PREFIX(RasterPos4f):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos4f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos4fv))
GL_PREFIX(RasterPos4fv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos4fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos4i))
GL_PREFIX(RasterPos4i):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos4i))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos4iv))
GL_PREFIX(RasterPos4iv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos4iv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos4s))
GL_PREFIX(RasterPos4s):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos4s))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos4sv))
GL_PREFIX(RasterPos4sv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos4sv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Rectd))
GL_PREFIX(Rectd):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Rectd))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Rectdv))
GL_PREFIX(Rectdv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Rectdv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Rectf))
GL_PREFIX(Rectf):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Rectf))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Rectfv))
GL_PREFIX(Rectfv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Rectfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Recti))
GL_PREFIX(Recti):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Recti))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Rectiv))
GL_PREFIX(Rectiv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Rectiv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Rects))
GL_PREFIX(Rects):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Rects))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Rectsv))
GL_PREFIX(Rectsv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Rectsv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord1d))
GL_PREFIX(TexCoord1d):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord1d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord1dv))
GL_PREFIX(TexCoord1dv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord1dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord1f))
GL_PREFIX(TexCoord1f):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord1f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord1fv))
GL_PREFIX(TexCoord1fv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord1fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord1i))
GL_PREFIX(TexCoord1i):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord1i))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord1iv))
GL_PREFIX(TexCoord1iv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord1iv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord1s))
GL_PREFIX(TexCoord1s):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord1s))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord1sv))
GL_PREFIX(TexCoord1sv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord1sv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord2d))
GL_PREFIX(TexCoord2d):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord2d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord2dv))
GL_PREFIX(TexCoord2dv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord2dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord2f))
GL_PREFIX(TexCoord2f):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord2f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord2fv))
GL_PREFIX(TexCoord2fv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord2fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord2i))
GL_PREFIX(TexCoord2i):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord2i))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord2iv))
GL_PREFIX(TexCoord2iv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord2iv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord2s))
GL_PREFIX(TexCoord2s):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord2s))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord2sv))
GL_PREFIX(TexCoord2sv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord2sv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord3d))
GL_PREFIX(TexCoord3d):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord3d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord3dv))
GL_PREFIX(TexCoord3dv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord3dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord3f))
GL_PREFIX(TexCoord3f):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord3f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord3fv))
GL_PREFIX(TexCoord3fv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord3fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord3i))
GL_PREFIX(TexCoord3i):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord3i))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord3iv))
GL_PREFIX(TexCoord3iv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord3iv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord3s))
GL_PREFIX(TexCoord3s):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord3s))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord3sv))
GL_PREFIX(TexCoord3sv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord3sv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord4d))
GL_PREFIX(TexCoord4d):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord4d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord4dv))
GL_PREFIX(TexCoord4dv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord4dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord4f))
GL_PREFIX(TexCoord4f):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord4f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord4fv))
GL_PREFIX(TexCoord4fv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord4fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord4i))
GL_PREFIX(TexCoord4i):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord4i))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord4iv))
GL_PREFIX(TexCoord4iv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord4iv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord4s))
GL_PREFIX(TexCoord4s):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord4s))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord4sv))
GL_PREFIX(TexCoord4sv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord4sv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex2d))
GL_PREFIX(Vertex2d):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex2d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex2dv))
GL_PREFIX(Vertex2dv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex2dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex2f))
GL_PREFIX(Vertex2f):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex2f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex2fv))
GL_PREFIX(Vertex2fv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex2fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex2i))
GL_PREFIX(Vertex2i):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex2i))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex2iv))
GL_PREFIX(Vertex2iv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex2iv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex2s))
GL_PREFIX(Vertex2s):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex2s))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex2sv))
GL_PREFIX(Vertex2sv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex2sv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex3d))
GL_PREFIX(Vertex3d):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex3d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex3dv))
GL_PREFIX(Vertex3dv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex3dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex3f))
GL_PREFIX(Vertex3f):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex3f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex3fv))
GL_PREFIX(Vertex3fv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex3fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex3i))
GL_PREFIX(Vertex3i):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex3i))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex3iv))
GL_PREFIX(Vertex3iv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex3iv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex3s))
GL_PREFIX(Vertex3s):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex3s))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex3sv))
GL_PREFIX(Vertex3sv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex3sv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex4d))
GL_PREFIX(Vertex4d):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex4d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex4dv))
GL_PREFIX(Vertex4dv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex4dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex4f))
GL_PREFIX(Vertex4f):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex4f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex4fv))
GL_PREFIX(Vertex4fv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex4fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex4i))
GL_PREFIX(Vertex4i):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex4i))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex4iv))
GL_PREFIX(Vertex4iv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex4iv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex4s))
GL_PREFIX(Vertex4s):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex4s))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex4sv))
GL_PREFIX(Vertex4sv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex4sv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ClipPlane))
GL_PREFIX(ClipPlane):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ClipPlane))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ColorMaterial))
GL_PREFIX(ColorMaterial):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ColorMaterial))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CullFace))
GL_PREFIX(CullFace):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CullFace))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Fogf))
GL_PREFIX(Fogf):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Fogf))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Fogfv))
GL_PREFIX(Fogfv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Fogfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Fogi))
GL_PREFIX(Fogi):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Fogi))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Fogiv))
GL_PREFIX(Fogiv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Fogiv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FrontFace))
GL_PREFIX(FrontFace):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FrontFace))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Hint))
GL_PREFIX(Hint):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Hint))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Lightf))
GL_PREFIX(Lightf):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Lightf))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Lightfv))
GL_PREFIX(Lightfv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Lightfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Lighti))
GL_PREFIX(Lighti):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Lighti))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Lightiv))
GL_PREFIX(Lightiv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Lightiv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LightModelf))
GL_PREFIX(LightModelf):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_LightModelf))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LightModelfv))
GL_PREFIX(LightModelfv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_LightModelfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LightModeli))
GL_PREFIX(LightModeli):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_LightModeli))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LightModeliv))
GL_PREFIX(LightModeliv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_LightModeliv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LineStipple))
GL_PREFIX(LineStipple):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_LineStipple))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LineWidth))
GL_PREFIX(LineWidth):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_LineWidth))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Materialf))
GL_PREFIX(Materialf):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Materialf))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Materialfv))
GL_PREFIX(Materialfv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Materialfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Materiali))
GL_PREFIX(Materiali):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Materiali))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Materialiv))
GL_PREFIX(Materialiv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Materialiv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PointSize))
GL_PREFIX(PointSize):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PointSize))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PolygonMode))
GL_PREFIX(PolygonMode):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PolygonMode))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PolygonStipple))
GL_PREFIX(PolygonStipple):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PolygonStipple))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Scissor))
GL_PREFIX(Scissor):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Scissor))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ShadeModel))
GL_PREFIX(ShadeModel):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ShadeModel))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexParameterf))
GL_PREFIX(TexParameterf):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexParameterf))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexParameterfv))
GL_PREFIX(TexParameterfv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexParameterfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexParameteri))
GL_PREFIX(TexParameteri):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexParameteri))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexParameteriv))
GL_PREFIX(TexParameteriv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexParameteriv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexImage1D))
GL_PREFIX(TexImage1D):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexImage1D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexImage2D))
GL_PREFIX(TexImage2D):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexImage2D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexEnvf))
GL_PREFIX(TexEnvf):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexEnvf))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexEnvfv))
GL_PREFIX(TexEnvfv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexEnvfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexEnvi))
GL_PREFIX(TexEnvi):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexEnvi))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexEnviv))
GL_PREFIX(TexEnviv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexEnviv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexGend))
GL_PREFIX(TexGend):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexGend))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexGendv))
GL_PREFIX(TexGendv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexGendv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexGenf))
GL_PREFIX(TexGenf):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexGenf))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexGenfv))
GL_PREFIX(TexGenfv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexGenfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexGeni))
GL_PREFIX(TexGeni):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexGeni))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexGeniv))
GL_PREFIX(TexGeniv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexGeniv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FeedbackBuffer))
GL_PREFIX(FeedbackBuffer):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FeedbackBuffer))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SelectBuffer))
GL_PREFIX(SelectBuffer):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SelectBuffer))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RenderMode))
GL_PREFIX(RenderMode):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RenderMode))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(InitNames))
GL_PREFIX(InitNames):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_InitNames))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LoadName))
GL_PREFIX(LoadName):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_LoadName))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PassThrough))
GL_PREFIX(PassThrough):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PassThrough))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PopName))
GL_PREFIX(PopName):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PopName))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PushName))
GL_PREFIX(PushName):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PushName))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DrawBuffer))
GL_PREFIX(DrawBuffer):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_DrawBuffer))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Clear))
GL_PREFIX(Clear):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Clear))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ClearAccum))
GL_PREFIX(ClearAccum):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ClearAccum))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ClearIndex))
GL_PREFIX(ClearIndex):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ClearIndex))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ClearColor))
GL_PREFIX(ClearColor):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ClearColor))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ClearStencil))
GL_PREFIX(ClearStencil):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ClearStencil))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ClearDepth))
GL_PREFIX(ClearDepth):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ClearDepth))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(StencilMask))
GL_PREFIX(StencilMask):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_StencilMask))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ColorMask))
GL_PREFIX(ColorMask):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ColorMask))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DepthMask))
GL_PREFIX(DepthMask):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_DepthMask))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(IndexMask))
GL_PREFIX(IndexMask):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_IndexMask))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Accum))
GL_PREFIX(Accum):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Accum))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Disable))
GL_PREFIX(Disable):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Disable))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Enable))
GL_PREFIX(Enable):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Enable))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Finish))
GL_PREFIX(Finish):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Finish))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Flush))
GL_PREFIX(Flush):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Flush))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PopAttrib))
GL_PREFIX(PopAttrib):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PopAttrib))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PushAttrib))
GL_PREFIX(PushAttrib):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PushAttrib))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Map1d))
GL_PREFIX(Map1d):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Map1d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Map1f))
GL_PREFIX(Map1f):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Map1f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Map2d))
GL_PREFIX(Map2d):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Map2d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Map2f))
GL_PREFIX(Map2f):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Map2f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MapGrid1d))
GL_PREFIX(MapGrid1d):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MapGrid1d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MapGrid1f))
GL_PREFIX(MapGrid1f):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MapGrid1f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MapGrid2d))
GL_PREFIX(MapGrid2d):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MapGrid2d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MapGrid2f))
GL_PREFIX(MapGrid2f):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MapGrid2f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EvalCoord1d))
GL_PREFIX(EvalCoord1d):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_EvalCoord1d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EvalCoord1dv))
GL_PREFIX(EvalCoord1dv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_EvalCoord1dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EvalCoord1f))
GL_PREFIX(EvalCoord1f):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_EvalCoord1f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EvalCoord1fv))
GL_PREFIX(EvalCoord1fv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_EvalCoord1fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EvalCoord2d))
GL_PREFIX(EvalCoord2d):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_EvalCoord2d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EvalCoord2dv))
GL_PREFIX(EvalCoord2dv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_EvalCoord2dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EvalCoord2f))
GL_PREFIX(EvalCoord2f):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_EvalCoord2f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EvalCoord2fv))
GL_PREFIX(EvalCoord2fv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_EvalCoord2fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EvalMesh1))
GL_PREFIX(EvalMesh1):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_EvalMesh1))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EvalPoint1))
GL_PREFIX(EvalPoint1):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_EvalPoint1))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EvalMesh2))
GL_PREFIX(EvalMesh2):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_EvalMesh2))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EvalPoint2))
GL_PREFIX(EvalPoint2):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_EvalPoint2))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(AlphaFunc))
GL_PREFIX(AlphaFunc):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_AlphaFunc))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(BlendFunc))
GL_PREFIX(BlendFunc):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_BlendFunc))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LogicOp))
GL_PREFIX(LogicOp):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_LogicOp))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(StencilFunc))
GL_PREFIX(StencilFunc):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_StencilFunc))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(StencilOp))
GL_PREFIX(StencilOp):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_StencilOp))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DepthFunc))
GL_PREFIX(DepthFunc):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_DepthFunc))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PixelZoom))
GL_PREFIX(PixelZoom):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PixelZoom))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PixelTransferf))
GL_PREFIX(PixelTransferf):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PixelTransferf))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PixelTransferi))
GL_PREFIX(PixelTransferi):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PixelTransferi))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PixelStoref))
GL_PREFIX(PixelStoref):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PixelStoref))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PixelStorei))
GL_PREFIX(PixelStorei):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PixelStorei))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PixelMapfv))
GL_PREFIX(PixelMapfv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PixelMapfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PixelMapuiv))
GL_PREFIX(PixelMapuiv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PixelMapuiv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PixelMapusv))
GL_PREFIX(PixelMapusv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PixelMapusv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ReadBuffer))
GL_PREFIX(ReadBuffer):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ReadBuffer))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyPixels))
GL_PREFIX(CopyPixels):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CopyPixels))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ReadPixels))
GL_PREFIX(ReadPixels):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ReadPixels))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DrawPixels))
GL_PREFIX(DrawPixels):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_DrawPixels))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetBooleanv))
GL_PREFIX(GetBooleanv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetBooleanv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetClipPlane))
GL_PREFIX(GetClipPlane):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetClipPlane))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetDoublev))
GL_PREFIX(GetDoublev):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetDoublev))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetError))
GL_PREFIX(GetError):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetError))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetFloatv))
GL_PREFIX(GetFloatv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetFloatv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetIntegerv))
GL_PREFIX(GetIntegerv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetIntegerv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetLightfv))
GL_PREFIX(GetLightfv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetLightfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetLightiv))
GL_PREFIX(GetLightiv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetLightiv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetMapdv))
GL_PREFIX(GetMapdv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetMapdv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetMapfv))
GL_PREFIX(GetMapfv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetMapfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetMapiv))
GL_PREFIX(GetMapiv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetMapiv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetMaterialfv))
GL_PREFIX(GetMaterialfv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetMaterialfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetMaterialiv))
GL_PREFIX(GetMaterialiv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetMaterialiv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetPixelMapfv))
GL_PREFIX(GetPixelMapfv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetPixelMapfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetPixelMapuiv))
GL_PREFIX(GetPixelMapuiv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetPixelMapuiv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetPixelMapusv))
GL_PREFIX(GetPixelMapusv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetPixelMapusv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetPolygonStipple))
GL_PREFIX(GetPolygonStipple):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetPolygonStipple))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetString))
GL_PREFIX(GetString):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetString))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetTexEnvfv))
GL_PREFIX(GetTexEnvfv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetTexEnvfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetTexEnviv))
GL_PREFIX(GetTexEnviv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetTexEnviv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetTexGendv))
GL_PREFIX(GetTexGendv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetTexGendv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetTexGenfv))
GL_PREFIX(GetTexGenfv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetTexGenfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetTexGeniv))
GL_PREFIX(GetTexGeniv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetTexGeniv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetTexImage))
GL_PREFIX(GetTexImage):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetTexImage))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetTexParameterfv))
GL_PREFIX(GetTexParameterfv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetTexParameterfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetTexParameteriv))
GL_PREFIX(GetTexParameteriv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetTexParameteriv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetTexLevelParameterfv))
GL_PREFIX(GetTexLevelParameterfv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetTexLevelParameterfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetTexLevelParameteriv))
GL_PREFIX(GetTexLevelParameteriv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetTexLevelParameteriv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(IsEnabled))
GL_PREFIX(IsEnabled):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_IsEnabled))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(IsList))
GL_PREFIX(IsList):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_IsList))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DepthRange))
GL_PREFIX(DepthRange):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_DepthRange))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Frustum))
GL_PREFIX(Frustum):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Frustum))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LoadIdentity))
GL_PREFIX(LoadIdentity):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_LoadIdentity))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LoadMatrixf))
GL_PREFIX(LoadMatrixf):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_LoadMatrixf))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LoadMatrixd))
GL_PREFIX(LoadMatrixd):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_LoadMatrixd))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MatrixMode))
GL_PREFIX(MatrixMode):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MatrixMode))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultMatrixf))
GL_PREFIX(MultMatrixf):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultMatrixf))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultMatrixd))
GL_PREFIX(MultMatrixd):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultMatrixd))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Ortho))
GL_PREFIX(Ortho):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Ortho))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PopMatrix))
GL_PREFIX(PopMatrix):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PopMatrix))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PushMatrix))
GL_PREFIX(PushMatrix):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PushMatrix))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Rotated))
GL_PREFIX(Rotated):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Rotated))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Rotatef))
GL_PREFIX(Rotatef):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Rotatef))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Scaled))
GL_PREFIX(Scaled):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Scaled))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Scalef))
GL_PREFIX(Scalef):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Scalef))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Translated))
GL_PREFIX(Translated):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Translated))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Translatef))
GL_PREFIX(Translatef):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Translatef))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Viewport))
GL_PREFIX(Viewport):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Viewport))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ArrayElement))
GL_PREFIX(ArrayElement):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ArrayElement))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ColorPointer))
GL_PREFIX(ColorPointer):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ColorPointer))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DisableClientState))
GL_PREFIX(DisableClientState):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_DisableClientState))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DrawArrays))
GL_PREFIX(DrawArrays):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_DrawArrays))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DrawElements))
GL_PREFIX(DrawElements):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_DrawElements))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EdgeFlagPointer))
GL_PREFIX(EdgeFlagPointer):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_EdgeFlagPointer))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EnableClientState))
GL_PREFIX(EnableClientState):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_EnableClientState))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetPointerv))
GL_PREFIX(GetPointerv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetPointerv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(IndexPointer))
GL_PREFIX(IndexPointer):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_IndexPointer))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(InterleavedArrays))
GL_PREFIX(InterleavedArrays):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_InterleavedArrays))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(NormalPointer))
GL_PREFIX(NormalPointer):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_NormalPointer))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoordPointer))
GL_PREFIX(TexCoordPointer):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoordPointer))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexPointer))
GL_PREFIX(VertexPointer):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexPointer))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PolygonOffset))
GL_PREFIX(PolygonOffset):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PolygonOffset))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyTexImage1D))
GL_PREFIX(CopyTexImage1D):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CopyTexImage1D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyTexImage2D))
GL_PREFIX(CopyTexImage2D):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CopyTexImage2D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyTexSubImage1D))
GL_PREFIX(CopyTexSubImage1D):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CopyTexSubImage1D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyTexSubImage2D))
GL_PREFIX(CopyTexSubImage2D):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CopyTexSubImage2D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexSubImage1D))
GL_PREFIX(TexSubImage1D):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexSubImage1D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexSubImage2D))
GL_PREFIX(TexSubImage2D):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexSubImage2D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(AreTexturesResident))
GL_PREFIX(AreTexturesResident):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_AreTexturesResident))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(BindTexture))
GL_PREFIX(BindTexture):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_BindTexture))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DeleteTextures))
GL_PREFIX(DeleteTextures):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_DeleteTextures))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GenTextures))
GL_PREFIX(GenTextures):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GenTextures))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(IsTexture))
GL_PREFIX(IsTexture):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_IsTexture))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PrioritizeTextures))
GL_PREFIX(PrioritizeTextures):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PrioritizeTextures))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Indexub))
GL_PREFIX(Indexub):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Indexub))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Indexubv))
GL_PREFIX(Indexubv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Indexubv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PopClientAttrib))
GL_PREFIX(PopClientAttrib):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PopClientAttrib))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PushClientAttrib))
GL_PREFIX(PushClientAttrib):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PushClientAttrib))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(BlendColor))
GL_PREFIX(BlendColor):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_BlendColor))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(BlendEquation))
GL_PREFIX(BlendEquation):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_BlendEquation))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DrawRangeElements))
GL_PREFIX(DrawRangeElements):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_DrawRangeElements))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ColorTable))
GL_PREFIX(ColorTable):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ColorTable))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ColorTableParameterfv))
GL_PREFIX(ColorTableParameterfv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ColorTableParameterfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ColorTableParameteriv))
GL_PREFIX(ColorTableParameteriv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ColorTableParameteriv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyColorTable))
GL_PREFIX(CopyColorTable):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CopyColorTable))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetColorTable))
GL_PREFIX(GetColorTable):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetColorTable))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetColorTableParameterfv))
GL_PREFIX(GetColorTableParameterfv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetColorTableParameterfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetColorTableParameteriv))
GL_PREFIX(GetColorTableParameteriv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetColorTableParameteriv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ColorSubTable))
GL_PREFIX(ColorSubTable):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ColorSubTable))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyColorSubTable))
GL_PREFIX(CopyColorSubTable):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CopyColorSubTable))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ConvolutionFilter1D))
GL_PREFIX(ConvolutionFilter1D):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ConvolutionFilter1D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ConvolutionFilter2D))
GL_PREFIX(ConvolutionFilter2D):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ConvolutionFilter2D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ConvolutionParameterf))
GL_PREFIX(ConvolutionParameterf):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ConvolutionParameterf))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ConvolutionParameterfv))
GL_PREFIX(ConvolutionParameterfv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ConvolutionParameterfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ConvolutionParameteri))
GL_PREFIX(ConvolutionParameteri):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ConvolutionParameteri))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ConvolutionParameteriv))
GL_PREFIX(ConvolutionParameteriv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ConvolutionParameteriv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyConvolutionFilter1D))
GL_PREFIX(CopyConvolutionFilter1D):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CopyConvolutionFilter1D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyConvolutionFilter2D))
GL_PREFIX(CopyConvolutionFilter2D):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CopyConvolutionFilter2D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetConvolutionFilter))
GL_PREFIX(GetConvolutionFilter):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetConvolutionFilter))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetConvolutionParameterfv))
GL_PREFIX(GetConvolutionParameterfv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetConvolutionParameterfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetConvolutionParameteriv))
GL_PREFIX(GetConvolutionParameteriv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetConvolutionParameteriv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetSeparableFilter))
GL_PREFIX(GetSeparableFilter):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetSeparableFilter))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SeparableFilter2D))
GL_PREFIX(SeparableFilter2D):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SeparableFilter2D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetHistogram))
GL_PREFIX(GetHistogram):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetHistogram))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetHistogramParameterfv))
GL_PREFIX(GetHistogramParameterfv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetHistogramParameterfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetHistogramParameteriv))
GL_PREFIX(GetHistogramParameteriv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetHistogramParameteriv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetMinmax))
GL_PREFIX(GetMinmax):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetMinmax))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetMinmaxParameterfv))
GL_PREFIX(GetMinmaxParameterfv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetMinmaxParameterfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetMinmaxParameteriv))
GL_PREFIX(GetMinmaxParameteriv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetMinmaxParameteriv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Histogram))
GL_PREFIX(Histogram):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Histogram))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Minmax))
GL_PREFIX(Minmax):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Minmax))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ResetHistogram))
GL_PREFIX(ResetHistogram):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ResetHistogram))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ResetMinmax))
GL_PREFIX(ResetMinmax):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ResetMinmax))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexImage3D))
GL_PREFIX(TexImage3D):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexImage3D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexSubImage3D))
GL_PREFIX(TexSubImage3D):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexSubImage3D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyTexSubImage3D))
GL_PREFIX(CopyTexSubImage3D):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CopyTexSubImage3D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ActiveTextureARB))
GL_PREFIX(ActiveTextureARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ActiveTextureARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ClientActiveTextureARB))
GL_PREFIX(ClientActiveTextureARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ClientActiveTextureARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord1dARB))
GL_PREFIX(MultiTexCoord1dARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord1dARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord1dvARB))
GL_PREFIX(MultiTexCoord1dvARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord1dvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord1fARB))
GL_PREFIX(MultiTexCoord1fARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord1fARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord1fvARB))
GL_PREFIX(MultiTexCoord1fvARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord1fvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord1iARB))
GL_PREFIX(MultiTexCoord1iARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord1iARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord1ivARB))
GL_PREFIX(MultiTexCoord1ivARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord1ivARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord1sARB))
GL_PREFIX(MultiTexCoord1sARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord1sARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord1svARB))
GL_PREFIX(MultiTexCoord1svARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord1svARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord2dARB))
GL_PREFIX(MultiTexCoord2dARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord2dARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord2dvARB))
GL_PREFIX(MultiTexCoord2dvARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord2dvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord2fARB))
GL_PREFIX(MultiTexCoord2fARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord2fARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord2fvARB))
GL_PREFIX(MultiTexCoord2fvARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord2fvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord2iARB))
GL_PREFIX(MultiTexCoord2iARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord2iARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord2ivARB))
GL_PREFIX(MultiTexCoord2ivARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord2ivARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord2sARB))
GL_PREFIX(MultiTexCoord2sARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord2sARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord2svARB))
GL_PREFIX(MultiTexCoord2svARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord2svARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord3dARB))
GL_PREFIX(MultiTexCoord3dARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord3dARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord3dvARB))
GL_PREFIX(MultiTexCoord3dvARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord3dvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord3fARB))
GL_PREFIX(MultiTexCoord3fARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord3fARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord3fvARB))
GL_PREFIX(MultiTexCoord3fvARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord3fvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord3iARB))
GL_PREFIX(MultiTexCoord3iARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord3iARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord3ivARB))
GL_PREFIX(MultiTexCoord3ivARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord3ivARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord3sARB))
GL_PREFIX(MultiTexCoord3sARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord3sARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord3svARB))
GL_PREFIX(MultiTexCoord3svARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord3svARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord4dARB))
GL_PREFIX(MultiTexCoord4dARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord4dARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord4dvARB))
GL_PREFIX(MultiTexCoord4dvARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord4dvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord4fARB))
GL_PREFIX(MultiTexCoord4fARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord4fARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord4fvARB))
GL_PREFIX(MultiTexCoord4fvARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord4fvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord4iARB))
GL_PREFIX(MultiTexCoord4iARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord4iARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord4ivARB))
GL_PREFIX(MultiTexCoord4ivARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord4ivARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord4sARB))
GL_PREFIX(MultiTexCoord4sARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord4sARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord4svARB))
GL_PREFIX(MultiTexCoord4svARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord4svARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LoadTransposeMatrixfARB))
GL_PREFIX(LoadTransposeMatrixfARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_LoadTransposeMatrixfARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LoadTransposeMatrixdARB))
GL_PREFIX(LoadTransposeMatrixdARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_LoadTransposeMatrixdARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultTransposeMatrixfARB))
GL_PREFIX(MultTransposeMatrixfARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultTransposeMatrixfARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultTransposeMatrixdARB))
GL_PREFIX(MultTransposeMatrixdARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultTransposeMatrixdARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SampleCoverageARB))
GL_PREFIX(SampleCoverageARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SampleCoverageARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(__unused413))
GL_PREFIX(__unused413):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset___unused413))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CompressedTexImage3DARB))
GL_PREFIX(CompressedTexImage3DARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CompressedTexImage3DARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CompressedTexImage2DARB))
GL_PREFIX(CompressedTexImage2DARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CompressedTexImage2DARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CompressedTexImage1DARB))
GL_PREFIX(CompressedTexImage1DARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CompressedTexImage1DARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CompressedTexSubImage3DARB))
GL_PREFIX(CompressedTexSubImage3DARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CompressedTexSubImage3DARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CompressedTexSubImage2DARB))
GL_PREFIX(CompressedTexSubImage2DARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CompressedTexSubImage2DARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CompressedTexSubImage1DARB))
GL_PREFIX(CompressedTexSubImage1DARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CompressedTexSubImage1DARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetCompressedTexImageARB))
GL_PREFIX(GetCompressedTexImageARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetCompressedTexImageARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ActiveTexture))
GL_PREFIX(ActiveTexture):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ActiveTextureARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ClientActiveTexture))
GL_PREFIX(ClientActiveTexture):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ClientActiveTextureARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord1d))
GL_PREFIX(MultiTexCoord1d):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord1dARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord1dv))
GL_PREFIX(MultiTexCoord1dv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord1dvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord1f))
GL_PREFIX(MultiTexCoord1f):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord1fARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord1fv))
GL_PREFIX(MultiTexCoord1fv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord1fvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord1i))
GL_PREFIX(MultiTexCoord1i):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord1iARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord1iv))
GL_PREFIX(MultiTexCoord1iv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord1ivARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord1s))
GL_PREFIX(MultiTexCoord1s):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord1sARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord1sv))
GL_PREFIX(MultiTexCoord1sv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord1svARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord2d))
GL_PREFIX(MultiTexCoord2d):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord2dARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord2dv))
GL_PREFIX(MultiTexCoord2dv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord2dvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord2f))
GL_PREFIX(MultiTexCoord2f):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord2fARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord2fv))
GL_PREFIX(MultiTexCoord2fv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord2fvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord2i))
GL_PREFIX(MultiTexCoord2i):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord2iARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord2iv))
GL_PREFIX(MultiTexCoord2iv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord2ivARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord2s))
GL_PREFIX(MultiTexCoord2s):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord2sARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord2sv))
GL_PREFIX(MultiTexCoord2sv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord2svARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord3d))
GL_PREFIX(MultiTexCoord3d):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord3dARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord3dv))
GL_PREFIX(MultiTexCoord3dv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord3dvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord3f))
GL_PREFIX(MultiTexCoord3f):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord3fARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord3fv))
GL_PREFIX(MultiTexCoord3fv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord3fvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord3i))
GL_PREFIX(MultiTexCoord3i):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord3iARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord3iv))
GL_PREFIX(MultiTexCoord3iv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord3ivARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord3s))
GL_PREFIX(MultiTexCoord3s):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord3sARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord3sv))
GL_PREFIX(MultiTexCoord3sv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord3svARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord4d))
GL_PREFIX(MultiTexCoord4d):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord4dARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord4dv))
GL_PREFIX(MultiTexCoord4dv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord4dvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord4f))
GL_PREFIX(MultiTexCoord4f):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord4fARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord4fv))
GL_PREFIX(MultiTexCoord4fv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord4fvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord4i))
GL_PREFIX(MultiTexCoord4i):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord4iARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord4iv))
GL_PREFIX(MultiTexCoord4iv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord4ivARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord4s))
GL_PREFIX(MultiTexCoord4s):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord4sARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord4sv))
GL_PREFIX(MultiTexCoord4sv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord4svARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LoadTransposeMatrixf))
GL_PREFIX(LoadTransposeMatrixf):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_LoadTransposeMatrixfARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LoadTransposeMatrixd))
GL_PREFIX(LoadTransposeMatrixd):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_LoadTransposeMatrixdARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultTransposeMatrixf))
GL_PREFIX(MultTransposeMatrixf):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultTransposeMatrixfARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultTransposeMatrixd))
GL_PREFIX(MultTransposeMatrixd):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultTransposeMatrixdARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CompressedTexImage3D))
GL_PREFIX(CompressedTexImage3D):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CompressedTexImage3DARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CompressedTexImage2D))
GL_PREFIX(CompressedTexImage2D):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CompressedTexImage2DARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CompressedTexImage1D))
GL_PREFIX(CompressedTexImage1D):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CompressedTexImage1DARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CompressedTexSubImage3D))
GL_PREFIX(CompressedTexSubImage3D):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CompressedTexSubImage3DARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CompressedTexSubImage2D))
GL_PREFIX(CompressedTexSubImage2D):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CompressedTexSubImage2DARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CompressedTexSubImage1D))
GL_PREFIX(CompressedTexSubImage1D):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CompressedTexSubImage1DARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetCompressedTexImage))
GL_PREFIX(GetCompressedTexImage):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetCompressedTexImageARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(BlendColorEXT))
GL_PREFIX(BlendColorEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_BlendColor))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PolygonOffsetEXT))
GL_PREFIX(PolygonOffsetEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PolygonOffsetEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexImage3DEXT))
GL_PREFIX(TexImage3DEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexImage3D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexSubImage3DEXT))
GL_PREFIX(TexSubImage3DEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexSubImage3D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetTexFilterFuncSGIS))
GL_PREFIX(GetTexFilterFuncSGIS):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetTexFilterFuncSGIS))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexFilterFuncSGIS))
GL_PREFIX(TexFilterFuncSGIS):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexFilterFuncSGIS))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexSubImage1DEXT))
GL_PREFIX(TexSubImage1DEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexSubImage1D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexSubImage2DEXT))
GL_PREFIX(TexSubImage2DEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexSubImage2D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyTexImage1DEXT))
GL_PREFIX(CopyTexImage1DEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CopyTexImage1D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyTexImage2DEXT))
GL_PREFIX(CopyTexImage2DEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CopyTexImage2D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyTexSubImage1DEXT))
GL_PREFIX(CopyTexSubImage1DEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CopyTexSubImage1D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyTexSubImage2DEXT))
GL_PREFIX(CopyTexSubImage2DEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CopyTexSubImage2D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyTexSubImage3DEXT))
GL_PREFIX(CopyTexSubImage3DEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CopyTexSubImage3D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetHistogramEXT))
GL_PREFIX(GetHistogramEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetHistogramEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetHistogramParameterfvEXT))
GL_PREFIX(GetHistogramParameterfvEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetHistogramParameterfvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetHistogramParameterivEXT))
GL_PREFIX(GetHistogramParameterivEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetHistogramParameterivEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetMinmaxEXT))
GL_PREFIX(GetMinmaxEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetMinmaxEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetMinmaxParameterfvEXT))
GL_PREFIX(GetMinmaxParameterfvEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetMinmaxParameterfvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetMinmaxParameterivEXT))
GL_PREFIX(GetMinmaxParameterivEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetMinmaxParameterivEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(HistogramEXT))
GL_PREFIX(HistogramEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Histogram))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MinmaxEXT))
GL_PREFIX(MinmaxEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Minmax))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ResetHistogramEXT))
GL_PREFIX(ResetHistogramEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ResetHistogram))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ResetMinmaxEXT))
GL_PREFIX(ResetMinmaxEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ResetMinmax))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ConvolutionFilter1DEXT))
GL_PREFIX(ConvolutionFilter1DEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ConvolutionFilter1D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ConvolutionFilter2DEXT))
GL_PREFIX(ConvolutionFilter2DEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ConvolutionFilter2D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ConvolutionParameterfEXT))
GL_PREFIX(ConvolutionParameterfEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ConvolutionParameterf))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ConvolutionParameterfvEXT))
GL_PREFIX(ConvolutionParameterfvEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ConvolutionParameterfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ConvolutionParameteriEXT))
GL_PREFIX(ConvolutionParameteriEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ConvolutionParameteri))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ConvolutionParameterivEXT))
GL_PREFIX(ConvolutionParameterivEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ConvolutionParameteriv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyConvolutionFilter1DEXT))
GL_PREFIX(CopyConvolutionFilter1DEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CopyConvolutionFilter1D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyConvolutionFilter2DEXT))
GL_PREFIX(CopyConvolutionFilter2DEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CopyConvolutionFilter2D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetConvolutionFilterEXT))
GL_PREFIX(GetConvolutionFilterEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetConvolutionFilterEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetConvolutionParameterfvEXT))
GL_PREFIX(GetConvolutionParameterfvEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetConvolutionParameterfvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetConvolutionParameterivEXT))
GL_PREFIX(GetConvolutionParameterivEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetConvolutionParameterivEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetSeparableFilterEXT))
GL_PREFIX(GetSeparableFilterEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetSeparableFilterEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SeparableFilter2DEXT))
GL_PREFIX(SeparableFilter2DEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SeparableFilter2D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ColorTableSGI))
GL_PREFIX(ColorTableSGI):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ColorTable))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ColorTableParameterfvSGI))
GL_PREFIX(ColorTableParameterfvSGI):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ColorTableParameterfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ColorTableParameterivSGI))
GL_PREFIX(ColorTableParameterivSGI):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ColorTableParameteriv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyColorTableSGI))
GL_PREFIX(CopyColorTableSGI):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CopyColorTable))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetColorTableSGI))
GL_PREFIX(GetColorTableSGI):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetColorTableSGI))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetColorTableParameterfvSGI))
GL_PREFIX(GetColorTableParameterfvSGI):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetColorTableParameterfvSGI))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetColorTableParameterivSGI))
GL_PREFIX(GetColorTableParameterivSGI):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetColorTableParameterivSGI))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PixelTexGenSGIX))
GL_PREFIX(PixelTexGenSGIX):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PixelTexGenSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PixelTexGenParameteriSGIS))
GL_PREFIX(PixelTexGenParameteriSGIS):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PixelTexGenParameteriSGIS))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PixelTexGenParameterivSGIS))
GL_PREFIX(PixelTexGenParameterivSGIS):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PixelTexGenParameterivSGIS))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PixelTexGenParameterfSGIS))
GL_PREFIX(PixelTexGenParameterfSGIS):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PixelTexGenParameterfSGIS))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PixelTexGenParameterfvSGIS))
GL_PREFIX(PixelTexGenParameterfvSGIS):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PixelTexGenParameterfvSGIS))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetPixelTexGenParameterivSGIS))
GL_PREFIX(GetPixelTexGenParameterivSGIS):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetPixelTexGenParameterivSGIS))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetPixelTexGenParameterfvSGIS))
GL_PREFIX(GetPixelTexGenParameterfvSGIS):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetPixelTexGenParameterfvSGIS))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexImage4DSGIS))
GL_PREFIX(TexImage4DSGIS):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexImage4DSGIS))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexSubImage4DSGIS))
GL_PREFIX(TexSubImage4DSGIS):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexSubImage4DSGIS))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(AreTexturesResidentEXT))
GL_PREFIX(AreTexturesResidentEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_AreTexturesResidentEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(BindTextureEXT))
GL_PREFIX(BindTextureEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_BindTexture))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DeleteTexturesEXT))
GL_PREFIX(DeleteTexturesEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_DeleteTextures))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GenTexturesEXT))
GL_PREFIX(GenTexturesEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GenTexturesEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(IsTextureEXT))
GL_PREFIX(IsTextureEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_IsTextureEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PrioritizeTexturesEXT))
GL_PREFIX(PrioritizeTexturesEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PrioritizeTextures))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DetailTexFuncSGIS))
GL_PREFIX(DetailTexFuncSGIS):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_DetailTexFuncSGIS))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetDetailTexFuncSGIS))
GL_PREFIX(GetDetailTexFuncSGIS):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetDetailTexFuncSGIS))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SharpenTexFuncSGIS))
GL_PREFIX(SharpenTexFuncSGIS):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SharpenTexFuncSGIS))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetSharpenTexFuncSGIS))
GL_PREFIX(GetSharpenTexFuncSGIS):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetSharpenTexFuncSGIS))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SampleMaskSGIS))
GL_PREFIX(SampleMaskSGIS):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SampleMaskSGIS))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SamplePatternSGIS))
GL_PREFIX(SamplePatternSGIS):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SamplePatternSGIS))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ArrayElementEXT))
GL_PREFIX(ArrayElementEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ArrayElement))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ColorPointerEXT))
GL_PREFIX(ColorPointerEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ColorPointerEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DrawArraysEXT))
GL_PREFIX(DrawArraysEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_DrawArrays))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EdgeFlagPointerEXT))
GL_PREFIX(EdgeFlagPointerEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_EdgeFlagPointerEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetPointervEXT))
GL_PREFIX(GetPointervEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetPointerv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(IndexPointerEXT))
GL_PREFIX(IndexPointerEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_IndexPointerEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(NormalPointerEXT))
GL_PREFIX(NormalPointerEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_NormalPointerEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoordPointerEXT))
GL_PREFIX(TexCoordPointerEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoordPointerEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexPointerEXT))
GL_PREFIX(VertexPointerEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexPointerEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(BlendEquationEXT))
GL_PREFIX(BlendEquationEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_BlendEquation))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SpriteParameterfSGIX))
GL_PREFIX(SpriteParameterfSGIX):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SpriteParameterfSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SpriteParameterfvSGIX))
GL_PREFIX(SpriteParameterfvSGIX):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SpriteParameterfvSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SpriteParameteriSGIX))
GL_PREFIX(SpriteParameteriSGIX):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SpriteParameteriSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SpriteParameterivSGIX))
GL_PREFIX(SpriteParameterivSGIX):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SpriteParameterivSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PointParameterfEXT))
GL_PREFIX(PointParameterfEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PointParameterfEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PointParameterfvEXT))
GL_PREFIX(PointParameterfvEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PointParameterfvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PointParameterfARB))
GL_PREFIX(PointParameterfARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PointParameterfEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PointParameterfvARB))
GL_PREFIX(PointParameterfvARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PointParameterfvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PointParameterfSGIS))
GL_PREFIX(PointParameterfSGIS):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PointParameterfEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PointParameterfvSGIS))
GL_PREFIX(PointParameterfvSGIS):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PointParameterfvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetInstrumentsSGIX))
GL_PREFIX(GetInstrumentsSGIX):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetInstrumentsSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(InstrumentsBufferSGIX))
GL_PREFIX(InstrumentsBufferSGIX):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_InstrumentsBufferSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PollInstrumentsSGIX))
GL_PREFIX(PollInstrumentsSGIX):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PollInstrumentsSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ReadInstrumentsSGIX))
GL_PREFIX(ReadInstrumentsSGIX):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ReadInstrumentsSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(StartInstrumentsSGIX))
GL_PREFIX(StartInstrumentsSGIX):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_StartInstrumentsSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(StopInstrumentsSGIX))
GL_PREFIX(StopInstrumentsSGIX):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_StopInstrumentsSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FrameZoomSGIX))
GL_PREFIX(FrameZoomSGIX):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FrameZoomSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TagSampleBufferSGIX))
GL_PREFIX(TagSampleBufferSGIX):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TagSampleBufferSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ReferencePlaneSGIX))
GL_PREFIX(ReferencePlaneSGIX):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ReferencePlaneSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FlushRasterSGIX))
GL_PREFIX(FlushRasterSGIX):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FlushRasterSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ColorSubTableEXT))
GL_PREFIX(ColorSubTableEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ColorSubTable))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyColorSubTableEXT))
GL_PREFIX(CopyColorSubTableEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CopyColorSubTable))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(HintPGI))
GL_PREFIX(HintPGI):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_HintPGI))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ColorTableEXT))
GL_PREFIX(ColorTableEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ColorTable))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetColorTableEXT))
GL_PREFIX(GetColorTableEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetColorTableEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetColorTableParameterivEXT))
GL_PREFIX(GetColorTableParameterivEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetColorTableParameterivEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetColorTableParameterfvEXT))
GL_PREFIX(GetColorTableParameterfvEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetColorTableParameterfvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetListParameterfvSGIX))
GL_PREFIX(GetListParameterfvSGIX):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetListParameterfvSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetListParameterivSGIX))
GL_PREFIX(GetListParameterivSGIX):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetListParameterivSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ListParameterfSGIX))
GL_PREFIX(ListParameterfSGIX):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ListParameterfSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ListParameterfvSGIX))
GL_PREFIX(ListParameterfvSGIX):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ListParameterfvSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ListParameteriSGIX))
GL_PREFIX(ListParameteriSGIX):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ListParameteriSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ListParameterivSGIX))
GL_PREFIX(ListParameterivSGIX):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ListParameterivSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(IndexMaterialEXT))
GL_PREFIX(IndexMaterialEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_IndexMaterialEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(IndexFuncEXT))
GL_PREFIX(IndexFuncEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_IndexFuncEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LockArraysEXT))
GL_PREFIX(LockArraysEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_LockArraysEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(UnlockArraysEXT))
GL_PREFIX(UnlockArraysEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_UnlockArraysEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CullParameterdvEXT))
GL_PREFIX(CullParameterdvEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CullParameterdvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CullParameterfvEXT))
GL_PREFIX(CullParameterfvEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CullParameterfvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FragmentColorMaterialSGIX))
GL_PREFIX(FragmentColorMaterialSGIX):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FragmentColorMaterialSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FragmentLightfSGIX))
GL_PREFIX(FragmentLightfSGIX):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FragmentLightfSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FragmentLightfvSGIX))
GL_PREFIX(FragmentLightfvSGIX):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FragmentLightfvSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FragmentLightiSGIX))
GL_PREFIX(FragmentLightiSGIX):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FragmentLightiSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FragmentLightivSGIX))
GL_PREFIX(FragmentLightivSGIX):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FragmentLightivSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FragmentLightModelfSGIX))
GL_PREFIX(FragmentLightModelfSGIX):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FragmentLightModelfSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FragmentLightModelfvSGIX))
GL_PREFIX(FragmentLightModelfvSGIX):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FragmentLightModelfvSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FragmentLightModeliSGIX))
GL_PREFIX(FragmentLightModeliSGIX):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FragmentLightModeliSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FragmentLightModelivSGIX))
GL_PREFIX(FragmentLightModelivSGIX):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FragmentLightModelivSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FragmentMaterialfSGIX))
GL_PREFIX(FragmentMaterialfSGIX):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FragmentMaterialfSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FragmentMaterialfvSGIX))
GL_PREFIX(FragmentMaterialfvSGIX):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FragmentMaterialfvSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FragmentMaterialiSGIX))
GL_PREFIX(FragmentMaterialiSGIX):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FragmentMaterialiSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FragmentMaterialivSGIX))
GL_PREFIX(FragmentMaterialivSGIX):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FragmentMaterialivSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetFragmentLightfvSGIX))
GL_PREFIX(GetFragmentLightfvSGIX):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetFragmentLightfvSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetFragmentLightivSGIX))
GL_PREFIX(GetFragmentLightivSGIX):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetFragmentLightivSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetFragmentMaterialfvSGIX))
GL_PREFIX(GetFragmentMaterialfvSGIX):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetFragmentMaterialfvSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetFragmentMaterialivSGIX))
GL_PREFIX(GetFragmentMaterialivSGIX):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetFragmentMaterialivSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LightEnviSGIX))
GL_PREFIX(LightEnviSGIX):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_LightEnviSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DrawRangeElementsEXT))
GL_PREFIX(DrawRangeElementsEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_DrawRangeElements))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3bEXT))
GL_PREFIX(SecondaryColor3bEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3bEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3bvEXT))
GL_PREFIX(SecondaryColor3bvEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3bvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3dEXT))
GL_PREFIX(SecondaryColor3dEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3dEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3dvEXT))
GL_PREFIX(SecondaryColor3dvEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3dvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3fEXT))
GL_PREFIX(SecondaryColor3fEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3fEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3fvEXT))
GL_PREFIX(SecondaryColor3fvEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3fvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3iEXT))
GL_PREFIX(SecondaryColor3iEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3iEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3ivEXT))
GL_PREFIX(SecondaryColor3ivEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3ivEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3sEXT))
GL_PREFIX(SecondaryColor3sEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3sEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3svEXT))
GL_PREFIX(SecondaryColor3svEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3svEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3ubEXT))
GL_PREFIX(SecondaryColor3ubEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3ubEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3ubvEXT))
GL_PREFIX(SecondaryColor3ubvEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3ubvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3uiEXT))
GL_PREFIX(SecondaryColor3uiEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3uiEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3uivEXT))
GL_PREFIX(SecondaryColor3uivEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3uivEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3usEXT))
GL_PREFIX(SecondaryColor3usEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3usEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3usvEXT))
GL_PREFIX(SecondaryColor3usvEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3usvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColorPointerEXT))
GL_PREFIX(SecondaryColorPointerEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColorPointerEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiDrawArraysEXT))
GL_PREFIX(MultiDrawArraysEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiDrawArraysEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiDrawElementsEXT))
GL_PREFIX(MultiDrawElementsEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiDrawElementsEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FogCoordfEXT))
GL_PREFIX(FogCoordfEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FogCoordfEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FogCoordfvEXT))
GL_PREFIX(FogCoordfvEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FogCoordfvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FogCoorddEXT))
GL_PREFIX(FogCoorddEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FogCoorddEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FogCoorddvEXT))
GL_PREFIX(FogCoorddvEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FogCoorddvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FogCoordPointerEXT))
GL_PREFIX(FogCoordPointerEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FogCoordPointerEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(BlendFuncSeparateEXT))
GL_PREFIX(BlendFuncSeparateEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_BlendFuncSeparateEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(BlendFuncSeparateINGR))
GL_PREFIX(BlendFuncSeparateINGR):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_BlendFuncSeparateEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexWeightfEXT))
GL_PREFIX(VertexWeightfEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexWeightfEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexWeightfvEXT))
GL_PREFIX(VertexWeightfvEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexWeightfvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexWeightPointerEXT))
GL_PREFIX(VertexWeightPointerEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexWeightPointerEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FlushVertexArrayRangeNV))
GL_PREFIX(FlushVertexArrayRangeNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FlushVertexArrayRangeNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexArrayRangeNV))
GL_PREFIX(VertexArrayRangeNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexArrayRangeNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CombinerParameterfvNV))
GL_PREFIX(CombinerParameterfvNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CombinerParameterfvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CombinerParameterfNV))
GL_PREFIX(CombinerParameterfNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CombinerParameterfNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CombinerParameterivNV))
GL_PREFIX(CombinerParameterivNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CombinerParameterivNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CombinerParameteriNV))
GL_PREFIX(CombinerParameteriNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CombinerParameteriNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CombinerInputNV))
GL_PREFIX(CombinerInputNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CombinerInputNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CombinerOutputNV))
GL_PREFIX(CombinerOutputNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CombinerOutputNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FinalCombinerInputNV))
GL_PREFIX(FinalCombinerInputNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FinalCombinerInputNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetCombinerInputParameterfvNV))
GL_PREFIX(GetCombinerInputParameterfvNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetCombinerInputParameterfvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetCombinerInputParameterivNV))
GL_PREFIX(GetCombinerInputParameterivNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetCombinerInputParameterivNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetCombinerOutputParameterfvNV))
GL_PREFIX(GetCombinerOutputParameterfvNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetCombinerOutputParameterfvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetCombinerOutputParameterivNV))
GL_PREFIX(GetCombinerOutputParameterivNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetCombinerOutputParameterivNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetFinalCombinerInputParameterfvNV))
GL_PREFIX(GetFinalCombinerInputParameterfvNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetFinalCombinerInputParameterfvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetFinalCombinerInputParameterivNV))
GL_PREFIX(GetFinalCombinerInputParameterivNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetFinalCombinerInputParameterivNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ResizeBuffersMESA))
GL_PREFIX(ResizeBuffersMESA):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ResizeBuffersMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2dMESA))
GL_PREFIX(WindowPos2dMESA):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2dMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2dvMESA))
GL_PREFIX(WindowPos2dvMESA):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2dvMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2fMESA))
GL_PREFIX(WindowPos2fMESA):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2fMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2fvMESA))
GL_PREFIX(WindowPos2fvMESA):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2fvMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2iMESA))
GL_PREFIX(WindowPos2iMESA):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2iMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2ivMESA))
GL_PREFIX(WindowPos2ivMESA):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2ivMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2sMESA))
GL_PREFIX(WindowPos2sMESA):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2sMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2svMESA))
GL_PREFIX(WindowPos2svMESA):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2svMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3dMESA))
GL_PREFIX(WindowPos3dMESA):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3dMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3dvMESA))
GL_PREFIX(WindowPos3dvMESA):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3dvMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3fMESA))
GL_PREFIX(WindowPos3fMESA):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3fMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3fvMESA))
GL_PREFIX(WindowPos3fvMESA):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3fvMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3iMESA))
GL_PREFIX(WindowPos3iMESA):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3iMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3ivMESA))
GL_PREFIX(WindowPos3ivMESA):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3ivMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3sMESA))
GL_PREFIX(WindowPos3sMESA):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3sMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3svMESA))
GL_PREFIX(WindowPos3svMESA):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3svMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos4dMESA))
GL_PREFIX(WindowPos4dMESA):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos4dMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos4dvMESA))
GL_PREFIX(WindowPos4dvMESA):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos4dvMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos4fMESA))
GL_PREFIX(WindowPos4fMESA):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos4fMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos4fvMESA))
GL_PREFIX(WindowPos4fvMESA):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos4fvMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos4iMESA))
GL_PREFIX(WindowPos4iMESA):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos4iMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos4ivMESA))
GL_PREFIX(WindowPos4ivMESA):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos4ivMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos4sMESA))
GL_PREFIX(WindowPos4sMESA):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos4sMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos4svMESA))
GL_PREFIX(WindowPos4svMESA):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos4svMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TbufferMask3DFX))
GL_PREFIX(TbufferMask3DFX):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TbufferMask3DFX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SampleMaskEXT))
GL_PREFIX(SampleMaskEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SampleMaskSGIS))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SamplePatternEXT))
GL_PREFIX(SamplePatternEXT):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SamplePatternSGIS))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2dARB))
GL_PREFIX(WindowPos2dARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2dMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2fARB))
GL_PREFIX(WindowPos2fARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2fMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2iARB))
GL_PREFIX(WindowPos2iARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2iMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2sARB))
GL_PREFIX(WindowPos2sARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2sMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2dvARB))
GL_PREFIX(WindowPos2dvARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2dvMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2fvARB))
GL_PREFIX(WindowPos2fvARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2fvMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2ivARB))
GL_PREFIX(WindowPos2ivARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2ivMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2svARB))
GL_PREFIX(WindowPos2svARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2svMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3dARB))
GL_PREFIX(WindowPos3dARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3dMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3fARB))
GL_PREFIX(WindowPos3fARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3fMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3iARB))
GL_PREFIX(WindowPos3iARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3iMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3sARB))
GL_PREFIX(WindowPos3sARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3sMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3dvARB))
GL_PREFIX(WindowPos3dvARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3dvMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3fvARB))
GL_PREFIX(WindowPos3fvARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3fvMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3ivARB))
GL_PREFIX(WindowPos3ivARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3ivMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3svARB))
GL_PREFIX(WindowPos3svARB):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3svMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(AreProgramsResidentNV))
GL_PREFIX(AreProgramsResidentNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_AreProgramsResidentNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(BindProgramNV))
GL_PREFIX(BindProgramNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_BindProgramNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DeleteProgramsNV))
GL_PREFIX(DeleteProgramsNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_DeleteProgramsNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ExecuteProgramNV))
GL_PREFIX(ExecuteProgramNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ExecuteProgramNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GenProgramsNV))
GL_PREFIX(GenProgramsNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GenProgramsNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetProgramParameterdvNV))
GL_PREFIX(GetProgramParameterdvNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetProgramParameterdvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetProgramParameterfvNV))
GL_PREFIX(GetProgramParameterfvNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetProgramParameterfvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetProgramivNV))
GL_PREFIX(GetProgramivNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetProgramivNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetProgramStringNV))
GL_PREFIX(GetProgramStringNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetProgramStringNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetTrackMatrixivNV))
GL_PREFIX(GetTrackMatrixivNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetTrackMatrixivNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetVertexAttribdvNV))
GL_PREFIX(GetVertexAttribdvNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetVertexAttribdvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetVertexAttribfvNV))
GL_PREFIX(GetVertexAttribfvNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetVertexAttribfvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetVertexAttribivNV))
GL_PREFIX(GetVertexAttribivNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetVertexAttribivNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetVertexAttribPointervNV))
GL_PREFIX(GetVertexAttribPointervNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetVertexAttribPointervNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(IsProgramNV))
GL_PREFIX(IsProgramNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_IsProgramNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LoadProgramNV))
GL_PREFIX(LoadProgramNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_LoadProgramNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ProgramParameter4dNV))
GL_PREFIX(ProgramParameter4dNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ProgramParameter4dNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ProgramParameter4dvNV))
GL_PREFIX(ProgramParameter4dvNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ProgramParameter4dvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ProgramParameter4fNV))
GL_PREFIX(ProgramParameter4fNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ProgramParameter4fNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ProgramParameter4fvNV))
GL_PREFIX(ProgramParameter4fvNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ProgramParameter4fvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ProgramParameters4dvNV))
GL_PREFIX(ProgramParameters4dvNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ProgramParameters4dvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ProgramParameters4fvNV))
GL_PREFIX(ProgramParameters4fvNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ProgramParameters4fvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RequestResidentProgramsNV))
GL_PREFIX(RequestResidentProgramsNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RequestResidentProgramsNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TrackMatrixNV))
GL_PREFIX(TrackMatrixNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TrackMatrixNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttribPointerNV))
GL_PREFIX(VertexAttribPointerNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttribPointerNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib1dNV))
GL_PREFIX(VertexAttrib1dNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib1dNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib1dvNV))
GL_PREFIX(VertexAttrib1dvNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib1dvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib1fNV))
GL_PREFIX(VertexAttrib1fNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib1fNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib1fvNV))
GL_PREFIX(VertexAttrib1fvNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib1fvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib1sNV))
GL_PREFIX(VertexAttrib1sNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib1sNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib1svNV))
GL_PREFIX(VertexAttrib1svNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib1svNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib2dNV))
GL_PREFIX(VertexAttrib2dNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib2dNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib2dvNV))
GL_PREFIX(VertexAttrib2dvNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib2dvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib2fNV))
GL_PREFIX(VertexAttrib2fNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib2fNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib2fvNV))
GL_PREFIX(VertexAttrib2fvNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib2fvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib2sNV))
GL_PREFIX(VertexAttrib2sNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib2sNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib2svNV))
GL_PREFIX(VertexAttrib2svNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib2svNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib3dNV))
GL_PREFIX(VertexAttrib3dNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib3dNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib3dvNV))
GL_PREFIX(VertexAttrib3dvNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib3dvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib3fNV))
GL_PREFIX(VertexAttrib3fNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib3fNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib3fvNV))
GL_PREFIX(VertexAttrib3fvNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib3fvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib3sNV))
GL_PREFIX(VertexAttrib3sNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib3sNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib3svNV))
GL_PREFIX(VertexAttrib3svNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib3svNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib4dNV))
GL_PREFIX(VertexAttrib4dNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib4dNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib4dvNV))
GL_PREFIX(VertexAttrib4dvNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib4dvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib4fNV))
GL_PREFIX(VertexAttrib4fNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib4fNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib4fvNV))
GL_PREFIX(VertexAttrib4fvNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib4fvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib4sNV))
GL_PREFIX(VertexAttrib4sNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib4sNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib4svNV))
GL_PREFIX(VertexAttrib4svNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib4svNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib4ubNV))
GL_PREFIX(VertexAttrib4ubNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib4ubNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib4ubvNV))
GL_PREFIX(VertexAttrib4ubvNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib4ubvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttribs1dvNV))
GL_PREFIX(VertexAttribs1dvNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttribs1dvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttribs1fvNV))
GL_PREFIX(VertexAttribs1fvNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttribs1fvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttribs1svNV))
GL_PREFIX(VertexAttribs1svNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttribs1svNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttribs2dvNV))
GL_PREFIX(VertexAttribs2dvNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttribs2dvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttribs2fvNV))
GL_PREFIX(VertexAttribs2fvNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttribs2fvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttribs2svNV))
GL_PREFIX(VertexAttribs2svNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttribs2svNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttribs3dvNV))
GL_PREFIX(VertexAttribs3dvNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttribs3dvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttribs3fvNV))
GL_PREFIX(VertexAttribs3fvNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttribs3fvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttribs3svNV))
GL_PREFIX(VertexAttribs3svNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttribs3svNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttribs4dvNV))
GL_PREFIX(VertexAttribs4dvNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttribs4dvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttribs4fvNV))
GL_PREFIX(VertexAttribs4fvNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttribs4fvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttribs4svNV))
GL_PREFIX(VertexAttribs4svNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttribs4svNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttribs4ubvNV))
GL_PREFIX(VertexAttribs4ubvNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttribs4ubvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PointParameteriNV))
GL_PREFIX(PointParameteriNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PointParameteriNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PointParameterivNV))
GL_PREFIX(PointParameterivNV):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PointParameterivNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PointParameterf))
GL_PREFIX(PointParameterf):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PointParameterfEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PointParameterfv))
GL_PREFIX(PointParameterfv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PointParameterfvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PointParameteri))
GL_PREFIX(PointParameteri):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PointParameteriNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PointParameteriv))
GL_PREFIX(PointParameteriv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PointParameterivNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3b))
GL_PREFIX(SecondaryColor3b):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3bEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3bv))
GL_PREFIX(SecondaryColor3bv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3bvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3d))
GL_PREFIX(SecondaryColor3d):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3dEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3dv))
GL_PREFIX(SecondaryColor3dv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3dvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3f))
GL_PREFIX(SecondaryColor3f):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3fEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3fv))
GL_PREFIX(SecondaryColor3fv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3fvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3i))
GL_PREFIX(SecondaryColor3i):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3iEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3iv))
GL_PREFIX(SecondaryColor3iv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3ivEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3s))
GL_PREFIX(SecondaryColor3s):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3sEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3sv))
GL_PREFIX(SecondaryColor3sv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3svEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3ub))
GL_PREFIX(SecondaryColor3ub):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3ubEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3ubv))
GL_PREFIX(SecondaryColor3ubv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3ubvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3ui))
GL_PREFIX(SecondaryColor3ui):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3uiEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3uiv))
GL_PREFIX(SecondaryColor3uiv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3uivEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3us))
GL_PREFIX(SecondaryColor3us):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3usEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3usv))
GL_PREFIX(SecondaryColor3usv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3usvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColorPointer))
GL_PREFIX(SecondaryColorPointer):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColorPointerEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2d))
GL_PREFIX(WindowPos2d):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2dMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2dv))
GL_PREFIX(WindowPos2dv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2dvMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2f))
GL_PREFIX(WindowPos2f):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2fMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2fv))
GL_PREFIX(WindowPos2fv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2fvMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2i))
GL_PREFIX(WindowPos2i):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2iMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2iv))
GL_PREFIX(WindowPos2iv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2ivMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2s))
GL_PREFIX(WindowPos2s):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2sMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2sv))
GL_PREFIX(WindowPos2sv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2svMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3d))
GL_PREFIX(WindowPos3d):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3dMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3dv))
GL_PREFIX(WindowPos3dv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3dvMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3f))
GL_PREFIX(WindowPos3f):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3fMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3fv))
GL_PREFIX(WindowPos3fv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3fvMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3i))
GL_PREFIX(WindowPos3i):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3iMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3iv))
GL_PREFIX(WindowPos3iv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3ivMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3s))
GL_PREFIX(WindowPos3s):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3sMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3sv))
GL_PREFIX(WindowPos3sv):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3svMESA))


#endif  /* __WIN32__ */

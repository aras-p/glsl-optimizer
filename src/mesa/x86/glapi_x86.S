/* $Id: glapi_x86.S,v 1.2 2000/01/17 18:09:11 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.3
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
 * X86-optimized dispatch code contibuted by Josh Vanderhoof
 */


#include "assyntax.h"

#include "../glapioffsets.h"

#ifndef __WIN32__
#define GL_PREFIX(n) GLNAME(gl ## n)

#define GL_OFFSET(x) CODEPTR(REGOFF(4 * x, EAX))

#ifdef GNU_ASSEMBLER
#define GLOBL_FN(x) GLOBL x ; .type x,@function
#else
#define GLOBL_FN(x) GLOBL x
#endif

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Accum))
GL_PREFIX(Accum):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Accum))
	JMP(GL_OFFSET(_gloffset_Accum))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(AlphaFunc))
GL_PREFIX(AlphaFunc):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_AlphaFunc))
	JMP(GL_OFFSET(_gloffset_AlphaFunc))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Begin))
GL_PREFIX(Begin):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Begin))
	JMP(GL_OFFSET(_gloffset_Begin))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Bitmap))
GL_PREFIX(Bitmap):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Bitmap))
	JMP(GL_OFFSET(_gloffset_Bitmap))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(BlendFunc))
GL_PREFIX(BlendFunc):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_BlendFunc))
	JMP(GL_OFFSET(_gloffset_BlendFunc))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CallList))
GL_PREFIX(CallList):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_CallList))
	JMP(GL_OFFSET(_gloffset_CallList))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CallLists))
GL_PREFIX(CallLists):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_CallLists))
	JMP(GL_OFFSET(_gloffset_CallLists))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Clear))
GL_PREFIX(Clear):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Clear))
	JMP(GL_OFFSET(_gloffset_Clear))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ClearAccum))
GL_PREFIX(ClearAccum):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ClearAccum))
	JMP(GL_OFFSET(_gloffset_ClearAccum))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ClearColor))
GL_PREFIX(ClearColor):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ClearColor))
	JMP(GL_OFFSET(_gloffset_ClearColor))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ClearDepth))
GL_PREFIX(ClearDepth):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ClearDepth))
	JMP(GL_OFFSET(_gloffset_ClearDepth))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ClearIndex))
GL_PREFIX(ClearIndex):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ClearIndex))
	JMP(GL_OFFSET(_gloffset_ClearIndex))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ClearStencil))
GL_PREFIX(ClearStencil):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ClearStencil))
	JMP(GL_OFFSET(_gloffset_ClearStencil))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ClipPlane))
GL_PREFIX(ClipPlane):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ClipPlane))
	JMP(GL_OFFSET(_gloffset_ClipPlane))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3b))
GL_PREFIX(Color3b):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Color3b))
	JMP(GL_OFFSET(_gloffset_Color3b))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3d))
GL_PREFIX(Color3d):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Color3d))
	JMP(GL_OFFSET(_gloffset_Color3d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3f))
GL_PREFIX(Color3f):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Color3f))
	JMP(GL_OFFSET(_gloffset_Color3f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3i))
GL_PREFIX(Color3i):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Color3i))
	JMP(GL_OFFSET(_gloffset_Color3i))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3s))
GL_PREFIX(Color3s):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Color3s))
	JMP(GL_OFFSET(_gloffset_Color3s))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3ub))
GL_PREFIX(Color3ub):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Color3ub))
	JMP(GL_OFFSET(_gloffset_Color3ub))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3ui))
GL_PREFIX(Color3ui):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Color3ui))
	JMP(GL_OFFSET(_gloffset_Color3ui))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3us))
GL_PREFIX(Color3us):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Color3us))
	JMP(GL_OFFSET(_gloffset_Color3us))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4b))
GL_PREFIX(Color4b):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Color4b))
	JMP(GL_OFFSET(_gloffset_Color4b))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4d))
GL_PREFIX(Color4d):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Color4d))
	JMP(GL_OFFSET(_gloffset_Color4d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4f))
GL_PREFIX(Color4f):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Color4f))
	JMP(GL_OFFSET(_gloffset_Color4f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4i))
GL_PREFIX(Color4i):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Color4i))
	JMP(GL_OFFSET(_gloffset_Color4i))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4s))
GL_PREFIX(Color4s):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Color4s))
	JMP(GL_OFFSET(_gloffset_Color4s))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4ub))
GL_PREFIX(Color4ub):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Color4ub))
	JMP(GL_OFFSET(_gloffset_Color4ub))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4ui))
GL_PREFIX(Color4ui):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Color4ui))
	JMP(GL_OFFSET(_gloffset_Color4ui))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4us))
GL_PREFIX(Color4us):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Color4us))
	JMP(GL_OFFSET(_gloffset_Color4us))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3bv))
GL_PREFIX(Color3bv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Color3bv))
	JMP(GL_OFFSET(_gloffset_Color3bv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3dv))
GL_PREFIX(Color3dv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Color3dv))
	JMP(GL_OFFSET(_gloffset_Color3dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3fv))
GL_PREFIX(Color3fv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Color3fv))
	JMP(GL_OFFSET(_gloffset_Color3fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3iv))
GL_PREFIX(Color3iv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Color3iv))
	JMP(GL_OFFSET(_gloffset_Color3iv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3sv))
GL_PREFIX(Color3sv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Color3sv))
	JMP(GL_OFFSET(_gloffset_Color3sv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3ubv))
GL_PREFIX(Color3ubv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Color3ubv))
	JMP(GL_OFFSET(_gloffset_Color3ubv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3uiv))
GL_PREFIX(Color3uiv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Color3uiv))
	JMP(GL_OFFSET(_gloffset_Color3uiv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3usv))
GL_PREFIX(Color3usv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Color3usv))
	JMP(GL_OFFSET(_gloffset_Color3usv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4bv))
GL_PREFIX(Color4bv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Color4bv))
	JMP(GL_OFFSET(_gloffset_Color4bv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4dv))
GL_PREFIX(Color4dv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Color4dv))
	JMP(GL_OFFSET(_gloffset_Color4dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4fv))
GL_PREFIX(Color4fv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Color4fv))
	JMP(GL_OFFSET(_gloffset_Color4fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4iv))
GL_PREFIX(Color4iv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Color4iv))
	JMP(GL_OFFSET(_gloffset_Color4iv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4sv))
GL_PREFIX(Color4sv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Color4sv))
	JMP(GL_OFFSET(_gloffset_Color4sv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4ubv))
GL_PREFIX(Color4ubv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Color4ubv))
	JMP(GL_OFFSET(_gloffset_Color4ubv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4uiv))
GL_PREFIX(Color4uiv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Color4uiv))
	JMP(GL_OFFSET(_gloffset_Color4uiv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4usv))
GL_PREFIX(Color4usv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Color4usv))
	JMP(GL_OFFSET(_gloffset_Color4usv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ColorMask))
GL_PREFIX(ColorMask):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ColorMask))
	JMP(GL_OFFSET(_gloffset_ColorMask))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ColorMaterial))
GL_PREFIX(ColorMaterial):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ColorMaterial))
	JMP(GL_OFFSET(_gloffset_ColorMaterial))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyPixels))
GL_PREFIX(CopyPixels):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_CopyPixels))
	JMP(GL_OFFSET(_gloffset_CopyPixels))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CullFace))
GL_PREFIX(CullFace):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_CullFace))
	JMP(GL_OFFSET(_gloffset_CullFace))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DepthFunc))
GL_PREFIX(DepthFunc):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_DepthFunc))
	JMP(GL_OFFSET(_gloffset_DepthFunc))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DepthMask))
GL_PREFIX(DepthMask):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_DepthMask))
	JMP(GL_OFFSET(_gloffset_DepthMask))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DepthRange))
GL_PREFIX(DepthRange):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_DepthRange))
	JMP(GL_OFFSET(_gloffset_DepthRange))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DeleteLists))
GL_PREFIX(DeleteLists):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_DeleteLists))
	JMP(GL_OFFSET(_gloffset_DeleteLists))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Disable))
GL_PREFIX(Disable):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Disable))
	JMP(GL_OFFSET(_gloffset_Disable))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DrawBuffer))
GL_PREFIX(DrawBuffer):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_DrawBuffer))
	JMP(GL_OFFSET(_gloffset_DrawBuffer))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DrawElements))
GL_PREFIX(DrawElements):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_DrawElements))
	JMP(GL_OFFSET(_gloffset_DrawElements))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DrawPixels))
GL_PREFIX(DrawPixels):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_DrawPixels))
	JMP(GL_OFFSET(_gloffset_DrawPixels))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Enable))
GL_PREFIX(Enable):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Enable))
	JMP(GL_OFFSET(_gloffset_Enable))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(End))
GL_PREFIX(End):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_End))
	JMP(GL_OFFSET(_gloffset_End))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EndList))
GL_PREFIX(EndList):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_EndList))
	JMP(GL_OFFSET(_gloffset_EndList))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EvalCoord1d))
GL_PREFIX(EvalCoord1d):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_EvalCoord1d))
	JMP(GL_OFFSET(_gloffset_EvalCoord1d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EvalCoord1f))
GL_PREFIX(EvalCoord1f):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_EvalCoord1f))
	JMP(GL_OFFSET(_gloffset_EvalCoord1f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EvalCoord1dv))
GL_PREFIX(EvalCoord1dv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_EvalCoord1dv))
	JMP(GL_OFFSET(_gloffset_EvalCoord1dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EvalCoord1fv))
GL_PREFIX(EvalCoord1fv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_EvalCoord1fv))
	JMP(GL_OFFSET(_gloffset_EvalCoord1fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EvalCoord2d))
GL_PREFIX(EvalCoord2d):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_EvalCoord2d))
	JMP(GL_OFFSET(_gloffset_EvalCoord2d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EvalCoord2f))
GL_PREFIX(EvalCoord2f):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_EvalCoord2f))
	JMP(GL_OFFSET(_gloffset_EvalCoord2f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EvalCoord2dv))
GL_PREFIX(EvalCoord2dv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_EvalCoord2dv))
	JMP(GL_OFFSET(_gloffset_EvalCoord2dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EvalCoord2fv))
GL_PREFIX(EvalCoord2fv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_EvalCoord2fv))
	JMP(GL_OFFSET(_gloffset_EvalCoord2fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EvalPoint1))
GL_PREFIX(EvalPoint1):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_EvalPoint1))
	JMP(GL_OFFSET(_gloffset_EvalPoint1))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EvalPoint2))
GL_PREFIX(EvalPoint2):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_EvalPoint2))
	JMP(GL_OFFSET(_gloffset_EvalPoint2))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EvalMesh1))
GL_PREFIX(EvalMesh1):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_EvalMesh1))
	JMP(GL_OFFSET(_gloffset_EvalMesh1))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EdgeFlag))
GL_PREFIX(EdgeFlag):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_EdgeFlag))
	JMP(GL_OFFSET(_gloffset_EdgeFlag))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EdgeFlagv))
GL_PREFIX(EdgeFlagv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_EdgeFlagv))
	JMP(GL_OFFSET(_gloffset_EdgeFlagv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EvalMesh2))
GL_PREFIX(EvalMesh2):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_EvalMesh2))
	JMP(GL_OFFSET(_gloffset_EvalMesh2))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FeedbackBuffer))
GL_PREFIX(FeedbackBuffer):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_FeedbackBuffer))
	JMP(GL_OFFSET(_gloffset_FeedbackBuffer))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Finish))
GL_PREFIX(Finish):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Finish))
	JMP(GL_OFFSET(_gloffset_Finish))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Flush))
GL_PREFIX(Flush):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Flush))
	JMP(GL_OFFSET(_gloffset_Flush))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Fogf))
GL_PREFIX(Fogf):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Fogf))
	JMP(GL_OFFSET(_gloffset_Fogf))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Fogi))
GL_PREFIX(Fogi):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Fogi))
	JMP(GL_OFFSET(_gloffset_Fogi))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Fogfv))
GL_PREFIX(Fogfv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Fogfv))
	JMP(GL_OFFSET(_gloffset_Fogfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Fogiv))
GL_PREFIX(Fogiv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Fogiv))
	JMP(GL_OFFSET(_gloffset_Fogiv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FrontFace))
GL_PREFIX(FrontFace):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_FrontFace))
	JMP(GL_OFFSET(_gloffset_FrontFace))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Frustum))
GL_PREFIX(Frustum):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Frustum))
	JMP(GL_OFFSET(_gloffset_Frustum))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GenLists))
GL_PREFIX(GenLists):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GenLists))
	JMP(GL_OFFSET(_gloffset_GenLists))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetBooleanv))
GL_PREFIX(GetBooleanv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetBooleanv))
	JMP(GL_OFFSET(_gloffset_GetBooleanv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetClipPlane))
GL_PREFIX(GetClipPlane):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetClipPlane))
	JMP(GL_OFFSET(_gloffset_GetClipPlane))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetDoublev))
GL_PREFIX(GetDoublev):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetDoublev))
	JMP(GL_OFFSET(_gloffset_GetDoublev))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetError))
GL_PREFIX(GetError):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetError))
	JMP(GL_OFFSET(_gloffset_GetError))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetFloatv))
GL_PREFIX(GetFloatv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetFloatv))
	JMP(GL_OFFSET(_gloffset_GetFloatv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetIntegerv))
GL_PREFIX(GetIntegerv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetIntegerv))
	JMP(GL_OFFSET(_gloffset_GetIntegerv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetLightfv))
GL_PREFIX(GetLightfv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetLightfv))
	JMP(GL_OFFSET(_gloffset_GetLightfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetLightiv))
GL_PREFIX(GetLightiv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetLightiv))
	JMP(GL_OFFSET(_gloffset_GetLightiv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetMapdv))
GL_PREFIX(GetMapdv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetMapdv))
	JMP(GL_OFFSET(_gloffset_GetMapdv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetMapfv))
GL_PREFIX(GetMapfv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetMapfv))
	JMP(GL_OFFSET(_gloffset_GetMapfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetMapiv))
GL_PREFIX(GetMapiv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetMapiv))
	JMP(GL_OFFSET(_gloffset_GetMapiv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetMaterialfv))
GL_PREFIX(GetMaterialfv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetMaterialfv))
	JMP(GL_OFFSET(_gloffset_GetMaterialfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetMaterialiv))
GL_PREFIX(GetMaterialiv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetMaterialiv))
	JMP(GL_OFFSET(_gloffset_GetMaterialiv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetPixelMapfv))
GL_PREFIX(GetPixelMapfv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetPixelMapfv))
	JMP(GL_OFFSET(_gloffset_GetPixelMapfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetPixelMapuiv))
GL_PREFIX(GetPixelMapuiv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetPixelMapuiv))
	JMP(GL_OFFSET(_gloffset_GetPixelMapuiv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetPixelMapusv))
GL_PREFIX(GetPixelMapusv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetPixelMapusv))
	JMP(GL_OFFSET(_gloffset_GetPixelMapusv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetPolygonStipple))
GL_PREFIX(GetPolygonStipple):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetPolygonStipple))
	JMP(GL_OFFSET(_gloffset_GetPolygonStipple))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetString))
GL_PREFIX(GetString):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetString))
	JMP(GL_OFFSET(_gloffset_GetString))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetTexEnvfv))
GL_PREFIX(GetTexEnvfv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetTexEnvfv))
	JMP(GL_OFFSET(_gloffset_GetTexEnvfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetTexEnviv))
GL_PREFIX(GetTexEnviv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetTexEnviv))
	JMP(GL_OFFSET(_gloffset_GetTexEnviv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetTexGeniv))
GL_PREFIX(GetTexGeniv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetTexGeniv))
	JMP(GL_OFFSET(_gloffset_GetTexGeniv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetTexGendv))
GL_PREFIX(GetTexGendv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetTexGendv))
	JMP(GL_OFFSET(_gloffset_GetTexGendv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetTexGenfv))
GL_PREFIX(GetTexGenfv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetTexGenfv))
	JMP(GL_OFFSET(_gloffset_GetTexGenfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetTexImage))
GL_PREFIX(GetTexImage):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetTexImage))
	JMP(GL_OFFSET(_gloffset_GetTexImage))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetTexLevelParameterfv))
GL_PREFIX(GetTexLevelParameterfv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetTexLevelParameterfv))
	JMP(GL_OFFSET(_gloffset_GetTexLevelParameterfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetTexLevelParameteriv))
GL_PREFIX(GetTexLevelParameteriv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetTexLevelParameteriv))
	JMP(GL_OFFSET(_gloffset_GetTexLevelParameteriv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetTexParameterfv))
GL_PREFIX(GetTexParameterfv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetTexParameterfv))
	JMP(GL_OFFSET(_gloffset_GetTexParameterfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetTexParameteriv))
GL_PREFIX(GetTexParameteriv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetTexParameteriv))
	JMP(GL_OFFSET(_gloffset_GetTexParameteriv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Hint))
GL_PREFIX(Hint):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Hint))
	JMP(GL_OFFSET(_gloffset_Hint))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Indexd))
GL_PREFIX(Indexd):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Indexd))
	JMP(GL_OFFSET(_gloffset_Indexd))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Indexdv))
GL_PREFIX(Indexdv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Indexdv))
	JMP(GL_OFFSET(_gloffset_Indexdv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Indexf))
GL_PREFIX(Indexf):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Indexf))
	JMP(GL_OFFSET(_gloffset_Indexf))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Indexfv))
GL_PREFIX(Indexfv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Indexfv))
	JMP(GL_OFFSET(_gloffset_Indexfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Indexi))
GL_PREFIX(Indexi):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Indexi))
	JMP(GL_OFFSET(_gloffset_Indexi))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Indexiv))
GL_PREFIX(Indexiv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Indexiv))
	JMP(GL_OFFSET(_gloffset_Indexiv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Indexs))
GL_PREFIX(Indexs):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Indexs))
	JMP(GL_OFFSET(_gloffset_Indexs))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Indexsv))
GL_PREFIX(Indexsv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Indexsv))
	JMP(GL_OFFSET(_gloffset_Indexsv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(IndexMask))
GL_PREFIX(IndexMask):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_IndexMask))
	JMP(GL_OFFSET(_gloffset_IndexMask))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(InitNames))
GL_PREFIX(InitNames):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_InitNames))
	JMP(GL_OFFSET(_gloffset_InitNames))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(IsEnabled))
GL_PREFIX(IsEnabled):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_IsEnabled))
	JMP(GL_OFFSET(_gloffset_IsEnabled))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(IsList))
GL_PREFIX(IsList):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_IsList))
	JMP(GL_OFFSET(_gloffset_IsList))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Lightf))
GL_PREFIX(Lightf):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Lightf))
	JMP(GL_OFFSET(_gloffset_Lightf))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Lighti))
GL_PREFIX(Lighti):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Lighti))
	JMP(GL_OFFSET(_gloffset_Lighti))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Lightfv))
GL_PREFIX(Lightfv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Lightfv))
	JMP(GL_OFFSET(_gloffset_Lightfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Lightiv))
GL_PREFIX(Lightiv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Lightiv))
	JMP(GL_OFFSET(_gloffset_Lightiv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LightModelf))
GL_PREFIX(LightModelf):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_LightModelf))
	JMP(GL_OFFSET(_gloffset_LightModelf))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LightModeli))
GL_PREFIX(LightModeli):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_LightModeli))
	JMP(GL_OFFSET(_gloffset_LightModeli))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LightModelfv))
GL_PREFIX(LightModelfv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_LightModelfv))
	JMP(GL_OFFSET(_gloffset_LightModelfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LightModeliv))
GL_PREFIX(LightModeliv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_LightModeliv))
	JMP(GL_OFFSET(_gloffset_LightModeliv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LineWidth))
GL_PREFIX(LineWidth):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_LineWidth))
	JMP(GL_OFFSET(_gloffset_LineWidth))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LineStipple))
GL_PREFIX(LineStipple):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_LineStipple))
	JMP(GL_OFFSET(_gloffset_LineStipple))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ListBase))
GL_PREFIX(ListBase):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ListBase))
	JMP(GL_OFFSET(_gloffset_ListBase))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LoadIdentity))
GL_PREFIX(LoadIdentity):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_LoadIdentity))
	JMP(GL_OFFSET(_gloffset_LoadIdentity))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LoadMatrixd))
GL_PREFIX(LoadMatrixd):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_LoadMatrixd))
	JMP(GL_OFFSET(_gloffset_LoadMatrixd))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LoadMatrixf))
GL_PREFIX(LoadMatrixf):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_LoadMatrixf))
	JMP(GL_OFFSET(_gloffset_LoadMatrixf))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LoadName))
GL_PREFIX(LoadName):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_LoadName))
	JMP(GL_OFFSET(_gloffset_LoadName))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LogicOp))
GL_PREFIX(LogicOp):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_LogicOp))
	JMP(GL_OFFSET(_gloffset_LogicOp))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Map1d))
GL_PREFIX(Map1d):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Map1d))
	JMP(GL_OFFSET(_gloffset_Map1d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Map1f))
GL_PREFIX(Map1f):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Map1f))
	JMP(GL_OFFSET(_gloffset_Map1f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Map2d))
GL_PREFIX(Map2d):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Map2d))
	JMP(GL_OFFSET(_gloffset_Map2d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Map2f))
GL_PREFIX(Map2f):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Map2f))
	JMP(GL_OFFSET(_gloffset_Map2f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MapGrid1d))
GL_PREFIX(MapGrid1d):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_MapGrid1d))
	JMP(GL_OFFSET(_gloffset_MapGrid1d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MapGrid1f))
GL_PREFIX(MapGrid1f):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_MapGrid1f))
	JMP(GL_OFFSET(_gloffset_MapGrid1f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MapGrid2d))
GL_PREFIX(MapGrid2d):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_MapGrid2d))
	JMP(GL_OFFSET(_gloffset_MapGrid2d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MapGrid2f))
GL_PREFIX(MapGrid2f):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_MapGrid2f))
	JMP(GL_OFFSET(_gloffset_MapGrid2f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Materialf))
GL_PREFIX(Materialf):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Materialf))
	JMP(GL_OFFSET(_gloffset_Materialf))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Materiali))
GL_PREFIX(Materiali):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Materiali))
	JMP(GL_OFFSET(_gloffset_Materiali))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Materialfv))
GL_PREFIX(Materialfv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Materialfv))
	JMP(GL_OFFSET(_gloffset_Materialfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Materialiv))
GL_PREFIX(Materialiv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Materialiv))
	JMP(GL_OFFSET(_gloffset_Materialiv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MatrixMode))
GL_PREFIX(MatrixMode):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_MatrixMode))
	JMP(GL_OFFSET(_gloffset_MatrixMode))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultMatrixd))
GL_PREFIX(MultMatrixd):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_MultMatrixd))
	JMP(GL_OFFSET(_gloffset_MultMatrixd))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultMatrixf))
GL_PREFIX(MultMatrixf):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_MultMatrixf))
	JMP(GL_OFFSET(_gloffset_MultMatrixf))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(NewList))
GL_PREFIX(NewList):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_NewList))
	JMP(GL_OFFSET(_gloffset_NewList))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Normal3b))
GL_PREFIX(Normal3b):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Normal3b))
	JMP(GL_OFFSET(_gloffset_Normal3b))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Normal3bv))
GL_PREFIX(Normal3bv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Normal3bv))
	JMP(GL_OFFSET(_gloffset_Normal3bv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Normal3d))
GL_PREFIX(Normal3d):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Normal3d))
	JMP(GL_OFFSET(_gloffset_Normal3d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Normal3dv))
GL_PREFIX(Normal3dv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Normal3dv))
	JMP(GL_OFFSET(_gloffset_Normal3dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Normal3f))
GL_PREFIX(Normal3f):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Normal3f))
	JMP(GL_OFFSET(_gloffset_Normal3f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Normal3fv))
GL_PREFIX(Normal3fv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Normal3fv))
	JMP(GL_OFFSET(_gloffset_Normal3fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Normal3i))
GL_PREFIX(Normal3i):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Normal3i))
	JMP(GL_OFFSET(_gloffset_Normal3i))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Normal3iv))
GL_PREFIX(Normal3iv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Normal3iv))
	JMP(GL_OFFSET(_gloffset_Normal3iv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Normal3s))
GL_PREFIX(Normal3s):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Normal3s))
	JMP(GL_OFFSET(_gloffset_Normal3s))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Normal3sv))
GL_PREFIX(Normal3sv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Normal3sv))
	JMP(GL_OFFSET(_gloffset_Normal3sv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Ortho))
GL_PREFIX(Ortho):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Ortho))
	JMP(GL_OFFSET(_gloffset_Ortho))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PassThrough))
GL_PREFIX(PassThrough):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_PassThrough))
	JMP(GL_OFFSET(_gloffset_PassThrough))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PixelMapfv))
GL_PREFIX(PixelMapfv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_PixelMapfv))
	JMP(GL_OFFSET(_gloffset_PixelMapfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PixelMapuiv))
GL_PREFIX(PixelMapuiv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_PixelMapuiv))
	JMP(GL_OFFSET(_gloffset_PixelMapuiv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PixelMapusv))
GL_PREFIX(PixelMapusv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_PixelMapusv))
	JMP(GL_OFFSET(_gloffset_PixelMapusv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PixelStoref))
GL_PREFIX(PixelStoref):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_PixelStoref))
	JMP(GL_OFFSET(_gloffset_PixelStoref))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PixelStorei))
GL_PREFIX(PixelStorei):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_PixelStorei))
	JMP(GL_OFFSET(_gloffset_PixelStorei))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PixelTransferf))
GL_PREFIX(PixelTransferf):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_PixelTransferf))
	JMP(GL_OFFSET(_gloffset_PixelTransferf))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PixelTransferi))
GL_PREFIX(PixelTransferi):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_PixelTransferi))
	JMP(GL_OFFSET(_gloffset_PixelTransferi))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PixelZoom))
GL_PREFIX(PixelZoom):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_PixelZoom))
	JMP(GL_OFFSET(_gloffset_PixelZoom))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PointSize))
GL_PREFIX(PointSize):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_PointSize))
	JMP(GL_OFFSET(_gloffset_PointSize))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PolygonMode))
GL_PREFIX(PolygonMode):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_PolygonMode))
	JMP(GL_OFFSET(_gloffset_PolygonMode))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PolygonStipple))
GL_PREFIX(PolygonStipple):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_PolygonStipple))
	JMP(GL_OFFSET(_gloffset_PolygonStipple))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PopAttrib))
GL_PREFIX(PopAttrib):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_PopAttrib))
	JMP(GL_OFFSET(_gloffset_PopAttrib))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PopMatrix))
GL_PREFIX(PopMatrix):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_PopMatrix))
	JMP(GL_OFFSET(_gloffset_PopMatrix))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PopName))
GL_PREFIX(PopName):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_PopName))
	JMP(GL_OFFSET(_gloffset_PopName))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PushAttrib))
GL_PREFIX(PushAttrib):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_PushAttrib))
	JMP(GL_OFFSET(_gloffset_PushAttrib))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PushMatrix))
GL_PREFIX(PushMatrix):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_PushMatrix))
	JMP(GL_OFFSET(_gloffset_PushMatrix))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PushName))
GL_PREFIX(PushName):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_PushName))
	JMP(GL_OFFSET(_gloffset_PushName))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos2d))
GL_PREFIX(RasterPos2d):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_RasterPos2d))
	JMP(GL_OFFSET(_gloffset_RasterPos2d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos2f))
GL_PREFIX(RasterPos2f):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_RasterPos2f))
	JMP(GL_OFFSET(_gloffset_RasterPos2f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos2i))
GL_PREFIX(RasterPos2i):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_RasterPos2i))
	JMP(GL_OFFSET(_gloffset_RasterPos2i))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos2s))
GL_PREFIX(RasterPos2s):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_RasterPos2s))
	JMP(GL_OFFSET(_gloffset_RasterPos2s))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos3d))
GL_PREFIX(RasterPos3d):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_RasterPos3d))
	JMP(GL_OFFSET(_gloffset_RasterPos3d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos3f))
GL_PREFIX(RasterPos3f):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_RasterPos3f))
	JMP(GL_OFFSET(_gloffset_RasterPos3f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos3i))
GL_PREFIX(RasterPos3i):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_RasterPos3i))
	JMP(GL_OFFSET(_gloffset_RasterPos3i))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos3s))
GL_PREFIX(RasterPos3s):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_RasterPos3s))
	JMP(GL_OFFSET(_gloffset_RasterPos3s))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos4d))
GL_PREFIX(RasterPos4d):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_RasterPos4d))
	JMP(GL_OFFSET(_gloffset_RasterPos4d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos4f))
GL_PREFIX(RasterPos4f):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_RasterPos4f))
	JMP(GL_OFFSET(_gloffset_RasterPos4f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos4i))
GL_PREFIX(RasterPos4i):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_RasterPos4i))
	JMP(GL_OFFSET(_gloffset_RasterPos4i))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos4s))
GL_PREFIX(RasterPos4s):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_RasterPos4s))
	JMP(GL_OFFSET(_gloffset_RasterPos4s))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos2dv))
GL_PREFIX(RasterPos2dv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_RasterPos2dv))
	JMP(GL_OFFSET(_gloffset_RasterPos2dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos2fv))
GL_PREFIX(RasterPos2fv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_RasterPos2fv))
	JMP(GL_OFFSET(_gloffset_RasterPos2fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos2iv))
GL_PREFIX(RasterPos2iv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_RasterPos2iv))
	JMP(GL_OFFSET(_gloffset_RasterPos2iv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos2sv))
GL_PREFIX(RasterPos2sv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_RasterPos2sv))
	JMP(GL_OFFSET(_gloffset_RasterPos2sv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos3dv))
GL_PREFIX(RasterPos3dv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_RasterPos3dv))
	JMP(GL_OFFSET(_gloffset_RasterPos3dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos3fv))
GL_PREFIX(RasterPos3fv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_RasterPos3fv))
	JMP(GL_OFFSET(_gloffset_RasterPos3fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos3iv))
GL_PREFIX(RasterPos3iv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_RasterPos3iv))
	JMP(GL_OFFSET(_gloffset_RasterPos3iv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos3sv))
GL_PREFIX(RasterPos3sv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_RasterPos3sv))
	JMP(GL_OFFSET(_gloffset_RasterPos3sv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos4dv))
GL_PREFIX(RasterPos4dv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_RasterPos4dv))
	JMP(GL_OFFSET(_gloffset_RasterPos4dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos4fv))
GL_PREFIX(RasterPos4fv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_RasterPos4fv))
	JMP(GL_OFFSET(_gloffset_RasterPos4fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos4iv))
GL_PREFIX(RasterPos4iv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_RasterPos4iv))
	JMP(GL_OFFSET(_gloffset_RasterPos4iv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos4sv))
GL_PREFIX(RasterPos4sv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_RasterPos4sv))
	JMP(GL_OFFSET(_gloffset_RasterPos4sv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ReadBuffer))
GL_PREFIX(ReadBuffer):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ReadBuffer))
	JMP(GL_OFFSET(_gloffset_ReadBuffer))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ReadPixels))
GL_PREFIX(ReadPixels):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ReadPixels))
	JMP(GL_OFFSET(_gloffset_ReadPixels))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Rectd))
GL_PREFIX(Rectd):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Rectd))
	JMP(GL_OFFSET(_gloffset_Rectd))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Rectdv))
GL_PREFIX(Rectdv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Rectdv))
	JMP(GL_OFFSET(_gloffset_Rectdv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Rectf))
GL_PREFIX(Rectf):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Rectf))
	JMP(GL_OFFSET(_gloffset_Rectf))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Rectfv))
GL_PREFIX(Rectfv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Rectfv))
	JMP(GL_OFFSET(_gloffset_Rectfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Recti))
GL_PREFIX(Recti):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Recti))
	JMP(GL_OFFSET(_gloffset_Recti))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Rectiv))
GL_PREFIX(Rectiv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Rectiv))
	JMP(GL_OFFSET(_gloffset_Rectiv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Rects))
GL_PREFIX(Rects):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Rects))
	JMP(GL_OFFSET(_gloffset_Rects))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Rectsv))
GL_PREFIX(Rectsv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Rectsv))
	JMP(GL_OFFSET(_gloffset_Rectsv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RenderMode))
GL_PREFIX(RenderMode):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_RenderMode))
	JMP(GL_OFFSET(_gloffset_RenderMode))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Rotated))
GL_PREFIX(Rotated):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Rotated))
	JMP(GL_OFFSET(_gloffset_Rotated))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Rotatef))
GL_PREFIX(Rotatef):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Rotatef))
	JMP(GL_OFFSET(_gloffset_Rotatef))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SelectBuffer))
GL_PREFIX(SelectBuffer):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_SelectBuffer))
	JMP(GL_OFFSET(_gloffset_SelectBuffer))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Scaled))
GL_PREFIX(Scaled):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Scaled))
	JMP(GL_OFFSET(_gloffset_Scaled))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Scalef))
GL_PREFIX(Scalef):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Scalef))
	JMP(GL_OFFSET(_gloffset_Scalef))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Scissor))
GL_PREFIX(Scissor):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Scissor))
	JMP(GL_OFFSET(_gloffset_Scissor))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ShadeModel))
GL_PREFIX(ShadeModel):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ShadeModel))
	JMP(GL_OFFSET(_gloffset_ShadeModel))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(StencilFunc))
GL_PREFIX(StencilFunc):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_StencilFunc))
	JMP(GL_OFFSET(_gloffset_StencilFunc))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(StencilMask))
GL_PREFIX(StencilMask):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_StencilMask))
	JMP(GL_OFFSET(_gloffset_StencilMask))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(StencilOp))
GL_PREFIX(StencilOp):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_StencilOp))
	JMP(GL_OFFSET(_gloffset_StencilOp))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord1d))
GL_PREFIX(TexCoord1d):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexCoord1d))
	JMP(GL_OFFSET(_gloffset_TexCoord1d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord1f))
GL_PREFIX(TexCoord1f):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexCoord1f))
	JMP(GL_OFFSET(_gloffset_TexCoord1f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord1i))
GL_PREFIX(TexCoord1i):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexCoord1i))
	JMP(GL_OFFSET(_gloffset_TexCoord1i))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord1s))
GL_PREFIX(TexCoord1s):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexCoord1s))
	JMP(GL_OFFSET(_gloffset_TexCoord1s))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord2d))
GL_PREFIX(TexCoord2d):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexCoord2d))
	JMP(GL_OFFSET(_gloffset_TexCoord2d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord2f))
GL_PREFIX(TexCoord2f):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexCoord2f))
	JMP(GL_OFFSET(_gloffset_TexCoord2f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord2s))
GL_PREFIX(TexCoord2s):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexCoord2s))
	JMP(GL_OFFSET(_gloffset_TexCoord2s))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord2i))
GL_PREFIX(TexCoord2i):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexCoord2i))
	JMP(GL_OFFSET(_gloffset_TexCoord2i))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord3d))
GL_PREFIX(TexCoord3d):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexCoord3d))
	JMP(GL_OFFSET(_gloffset_TexCoord3d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord3f))
GL_PREFIX(TexCoord3f):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexCoord3f))
	JMP(GL_OFFSET(_gloffset_TexCoord3f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord3i))
GL_PREFIX(TexCoord3i):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexCoord3i))
	JMP(GL_OFFSET(_gloffset_TexCoord3i))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord3s))
GL_PREFIX(TexCoord3s):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexCoord3s))
	JMP(GL_OFFSET(_gloffset_TexCoord3s))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord4d))
GL_PREFIX(TexCoord4d):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexCoord4d))
	JMP(GL_OFFSET(_gloffset_TexCoord4d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord4f))
GL_PREFIX(TexCoord4f):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexCoord4f))
	JMP(GL_OFFSET(_gloffset_TexCoord4f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord4i))
GL_PREFIX(TexCoord4i):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexCoord4i))
	JMP(GL_OFFSET(_gloffset_TexCoord4i))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord4s))
GL_PREFIX(TexCoord4s):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexCoord4s))
	JMP(GL_OFFSET(_gloffset_TexCoord4s))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord1dv))
GL_PREFIX(TexCoord1dv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexCoord1dv))
	JMP(GL_OFFSET(_gloffset_TexCoord1dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord1fv))
GL_PREFIX(TexCoord1fv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexCoord1fv))
	JMP(GL_OFFSET(_gloffset_TexCoord1fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord1iv))
GL_PREFIX(TexCoord1iv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexCoord1iv))
	JMP(GL_OFFSET(_gloffset_TexCoord1iv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord1sv))
GL_PREFIX(TexCoord1sv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexCoord1sv))
	JMP(GL_OFFSET(_gloffset_TexCoord1sv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord2dv))
GL_PREFIX(TexCoord2dv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexCoord2dv))
	JMP(GL_OFFSET(_gloffset_TexCoord2dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord2fv))
GL_PREFIX(TexCoord2fv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexCoord2fv))
	JMP(GL_OFFSET(_gloffset_TexCoord2fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord2iv))
GL_PREFIX(TexCoord2iv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexCoord2iv))
	JMP(GL_OFFSET(_gloffset_TexCoord2iv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord2sv))
GL_PREFIX(TexCoord2sv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexCoord2sv))
	JMP(GL_OFFSET(_gloffset_TexCoord2sv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord3dv))
GL_PREFIX(TexCoord3dv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexCoord3dv))
	JMP(GL_OFFSET(_gloffset_TexCoord3dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord3fv))
GL_PREFIX(TexCoord3fv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexCoord3fv))
	JMP(GL_OFFSET(_gloffset_TexCoord3fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord3iv))
GL_PREFIX(TexCoord3iv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexCoord3iv))
	JMP(GL_OFFSET(_gloffset_TexCoord3iv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord3sv))
GL_PREFIX(TexCoord3sv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexCoord3sv))
	JMP(GL_OFFSET(_gloffset_TexCoord3sv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord4dv))
GL_PREFIX(TexCoord4dv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexCoord4dv))
	JMP(GL_OFFSET(_gloffset_TexCoord4dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord4fv))
GL_PREFIX(TexCoord4fv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexCoord4fv))
	JMP(GL_OFFSET(_gloffset_TexCoord4fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord4iv))
GL_PREFIX(TexCoord4iv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexCoord4iv))
	JMP(GL_OFFSET(_gloffset_TexCoord4iv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord4sv))
GL_PREFIX(TexCoord4sv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexCoord4sv))
	JMP(GL_OFFSET(_gloffset_TexCoord4sv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexGend))
GL_PREFIX(TexGend):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexGend))
	JMP(GL_OFFSET(_gloffset_TexGend))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexGendv))
GL_PREFIX(TexGendv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexGendv))
	JMP(GL_OFFSET(_gloffset_TexGendv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexGenf))
GL_PREFIX(TexGenf):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexGenf))
	JMP(GL_OFFSET(_gloffset_TexGenf))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexGenfv))
GL_PREFIX(TexGenfv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexGenfv))
	JMP(GL_OFFSET(_gloffset_TexGenfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexGeni))
GL_PREFIX(TexGeni):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexGeni))
	JMP(GL_OFFSET(_gloffset_TexGeni))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexGeniv))
GL_PREFIX(TexGeniv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexGeniv))
	JMP(GL_OFFSET(_gloffset_TexGeniv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexEnvf))
GL_PREFIX(TexEnvf):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexEnvf))
	JMP(GL_OFFSET(_gloffset_TexEnvf))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexEnvfv))
GL_PREFIX(TexEnvfv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexEnvfv))
	JMP(GL_OFFSET(_gloffset_TexEnvfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexEnvi))
GL_PREFIX(TexEnvi):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexEnvi))
	JMP(GL_OFFSET(_gloffset_TexEnvi))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexEnviv))
GL_PREFIX(TexEnviv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexEnviv))
	JMP(GL_OFFSET(_gloffset_TexEnviv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexImage1D))
GL_PREFIX(TexImage1D):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexImage1D))
	JMP(GL_OFFSET(_gloffset_TexImage1D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexImage2D))
GL_PREFIX(TexImage2D):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexImage2D))
	JMP(GL_OFFSET(_gloffset_TexImage2D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexParameterf))
GL_PREFIX(TexParameterf):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexParameterf))
	JMP(GL_OFFSET(_gloffset_TexParameterf))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexParameterfv))
GL_PREFIX(TexParameterfv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexParameterfv))
	JMP(GL_OFFSET(_gloffset_TexParameterfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexParameteri))
GL_PREFIX(TexParameteri):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexParameteri))
	JMP(GL_OFFSET(_gloffset_TexParameteri))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexParameteriv))
GL_PREFIX(TexParameteriv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexParameteriv))
	JMP(GL_OFFSET(_gloffset_TexParameteriv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Translated))
GL_PREFIX(Translated):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Translated))
	JMP(GL_OFFSET(_gloffset_Translated))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Translatef))
GL_PREFIX(Translatef):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Translatef))
	JMP(GL_OFFSET(_gloffset_Translatef))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex2d))
GL_PREFIX(Vertex2d):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Vertex2d))
	JMP(GL_OFFSET(_gloffset_Vertex2d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex2dv))
GL_PREFIX(Vertex2dv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Vertex2dv))
	JMP(GL_OFFSET(_gloffset_Vertex2dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex2f))
GL_PREFIX(Vertex2f):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Vertex2f))
	JMP(GL_OFFSET(_gloffset_Vertex2f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex2fv))
GL_PREFIX(Vertex2fv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Vertex2fv))
	JMP(GL_OFFSET(_gloffset_Vertex2fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex2i))
GL_PREFIX(Vertex2i):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Vertex2i))
	JMP(GL_OFFSET(_gloffset_Vertex2i))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex2iv))
GL_PREFIX(Vertex2iv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Vertex2iv))
	JMP(GL_OFFSET(_gloffset_Vertex2iv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex2s))
GL_PREFIX(Vertex2s):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Vertex2s))
	JMP(GL_OFFSET(_gloffset_Vertex2s))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex2sv))
GL_PREFIX(Vertex2sv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Vertex2sv))
	JMP(GL_OFFSET(_gloffset_Vertex2sv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex3d))
GL_PREFIX(Vertex3d):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Vertex3d))
	JMP(GL_OFFSET(_gloffset_Vertex3d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex3dv))
GL_PREFIX(Vertex3dv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Vertex3dv))
	JMP(GL_OFFSET(_gloffset_Vertex3dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex3f))
GL_PREFIX(Vertex3f):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Vertex3f))
	JMP(GL_OFFSET(_gloffset_Vertex3f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex3fv))
GL_PREFIX(Vertex3fv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Vertex3fv))
	JMP(GL_OFFSET(_gloffset_Vertex3fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex3i))
GL_PREFIX(Vertex3i):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Vertex3i))
	JMP(GL_OFFSET(_gloffset_Vertex3i))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex3iv))
GL_PREFIX(Vertex3iv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Vertex3iv))
	JMP(GL_OFFSET(_gloffset_Vertex3iv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex3s))
GL_PREFIX(Vertex3s):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Vertex3s))
	JMP(GL_OFFSET(_gloffset_Vertex3s))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex3sv))
GL_PREFIX(Vertex3sv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Vertex3sv))
	JMP(GL_OFFSET(_gloffset_Vertex3sv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex4d))
GL_PREFIX(Vertex4d):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Vertex4d))
	JMP(GL_OFFSET(_gloffset_Vertex4d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex4dv))
GL_PREFIX(Vertex4dv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Vertex4dv))
	JMP(GL_OFFSET(_gloffset_Vertex4dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex4f))
GL_PREFIX(Vertex4f):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Vertex4f))
	JMP(GL_OFFSET(_gloffset_Vertex4f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex4fv))
GL_PREFIX(Vertex4fv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Vertex4fv))
	JMP(GL_OFFSET(_gloffset_Vertex4fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex4i))
GL_PREFIX(Vertex4i):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Vertex4i))
	JMP(GL_OFFSET(_gloffset_Vertex4i))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex4iv))
GL_PREFIX(Vertex4iv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Vertex4iv))
	JMP(GL_OFFSET(_gloffset_Vertex4iv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex4s))
GL_PREFIX(Vertex4s):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Vertex4s))
	JMP(GL_OFFSET(_gloffset_Vertex4s))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex4sv))
GL_PREFIX(Vertex4sv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Vertex4sv))
	JMP(GL_OFFSET(_gloffset_Vertex4sv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Viewport))
GL_PREFIX(Viewport):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Viewport))
	JMP(GL_OFFSET(_gloffset_Viewport))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(AreTexturesResident))
GL_PREFIX(AreTexturesResident):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_AreTexturesResident))
	JMP(GL_OFFSET(_gloffset_AreTexturesResident))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ArrayElement))
GL_PREFIX(ArrayElement):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ArrayElement))
	JMP(GL_OFFSET(_gloffset_ArrayElement))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(BindTexture))
GL_PREFIX(BindTexture):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_BindTexture))
	JMP(GL_OFFSET(_gloffset_BindTexture))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ColorPointer))
GL_PREFIX(ColorPointer):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ColorPointer))
	JMP(GL_OFFSET(_gloffset_ColorPointer))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyTexImage1D))
GL_PREFIX(CopyTexImage1D):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_CopyTexImage1D))
	JMP(GL_OFFSET(_gloffset_CopyTexImage1D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyTexImage2D))
GL_PREFIX(CopyTexImage2D):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_CopyTexImage2D))
	JMP(GL_OFFSET(_gloffset_CopyTexImage2D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyTexSubImage1D))
GL_PREFIX(CopyTexSubImage1D):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_CopyTexSubImage1D))
	JMP(GL_OFFSET(_gloffset_CopyTexSubImage1D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyTexSubImage2D))
GL_PREFIX(CopyTexSubImage2D):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_CopyTexSubImage2D))
	JMP(GL_OFFSET(_gloffset_CopyTexSubImage2D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DeleteTextures))
GL_PREFIX(DeleteTextures):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_DeleteTextures))
	JMP(GL_OFFSET(_gloffset_DeleteTextures))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DisableClientState))
GL_PREFIX(DisableClientState):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_DisableClientState))
	JMP(GL_OFFSET(_gloffset_DisableClientState))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DrawArrays))
GL_PREFIX(DrawArrays):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_DrawArrays))
	JMP(GL_OFFSET(_gloffset_DrawArrays))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EdgeFlagPointer))
GL_PREFIX(EdgeFlagPointer):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_EdgeFlagPointer))
	JMP(GL_OFFSET(_gloffset_EdgeFlagPointer))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EnableClientState))
GL_PREFIX(EnableClientState):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_EnableClientState))
	JMP(GL_OFFSET(_gloffset_EnableClientState))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GenTextures))
GL_PREFIX(GenTextures):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GenTextures))
	JMP(GL_OFFSET(_gloffset_GenTextures))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetPointerv))
GL_PREFIX(GetPointerv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetPointerv))
	JMP(GL_OFFSET(_gloffset_GetPointerv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(IndexPointer))
GL_PREFIX(IndexPointer):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_IndexPointer))
	JMP(GL_OFFSET(_gloffset_IndexPointer))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Indexub))
GL_PREFIX(Indexub):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Indexub))
	JMP(GL_OFFSET(_gloffset_Indexub))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Indexubv))
GL_PREFIX(Indexubv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Indexubv))
	JMP(GL_OFFSET(_gloffset_Indexubv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(InterleavedArrays))
GL_PREFIX(InterleavedArrays):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_InterleavedArrays))
	JMP(GL_OFFSET(_gloffset_InterleavedArrays))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(IsTexture))
GL_PREFIX(IsTexture):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_IsTexture))
	JMP(GL_OFFSET(_gloffset_IsTexture))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(NormalPointer))
GL_PREFIX(NormalPointer):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_NormalPointer))
	JMP(GL_OFFSET(_gloffset_NormalPointer))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PolygonOffset))
GL_PREFIX(PolygonOffset):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_PolygonOffset))
	JMP(GL_OFFSET(_gloffset_PolygonOffset))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PopClientAttrib))
GL_PREFIX(PopClientAttrib):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_PopClientAttrib))
	JMP(GL_OFFSET(_gloffset_PopClientAttrib))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PrioritizeTextures))
GL_PREFIX(PrioritizeTextures):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_PrioritizeTextures))
	JMP(GL_OFFSET(_gloffset_PrioritizeTextures))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PushClientAttrib))
GL_PREFIX(PushClientAttrib):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_PushClientAttrib))
	JMP(GL_OFFSET(_gloffset_PushClientAttrib))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoordPointer))
GL_PREFIX(TexCoordPointer):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexCoordPointer))
	JMP(GL_OFFSET(_gloffset_TexCoordPointer))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexSubImage1D))
GL_PREFIX(TexSubImage1D):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexSubImage1D))
	JMP(GL_OFFSET(_gloffset_TexSubImage1D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexSubImage2D))
GL_PREFIX(TexSubImage2D):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexSubImage2D))
	JMP(GL_OFFSET(_gloffset_TexSubImage2D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexPointer))
GL_PREFIX(VertexPointer):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_VertexPointer))
	JMP(GL_OFFSET(_gloffset_VertexPointer))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyTexSubImage3D))
GL_PREFIX(CopyTexSubImage3D):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_CopyTexSubImage3D))
	JMP(GL_OFFSET(_gloffset_CopyTexSubImage3D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DrawRangeElements))
GL_PREFIX(DrawRangeElements):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_DrawRangeElements))
	JMP(GL_OFFSET(_gloffset_DrawRangeElements))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexImage3D))
GL_PREFIX(TexImage3D):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexImage3D))
	JMP(GL_OFFSET(_gloffset_TexImage3D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexSubImage3D))
GL_PREFIX(TexSubImage3D):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexSubImage3D))
	JMP(GL_OFFSET(_gloffset_TexSubImage3D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(BlendColor))
GL_PREFIX(BlendColor):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_BlendColor))
	JMP(GL_OFFSET(_gloffset_BlendColor))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(BlendEquation))
GL_PREFIX(BlendEquation):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_BlendEquation))
	JMP(GL_OFFSET(_gloffset_BlendEquation))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ColorSubTable))
GL_PREFIX(ColorSubTable):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ColorSubTable))
	JMP(GL_OFFSET(_gloffset_ColorSubTable))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ColorTable))
GL_PREFIX(ColorTable):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ColorTable))
	JMP(GL_OFFSET(_gloffset_ColorTable))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ColorTableParameterfv))
GL_PREFIX(ColorTableParameterfv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ColorTableParameterfv))
	JMP(GL_OFFSET(_gloffset_ColorTableParameterfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ColorTableParameteriv))
GL_PREFIX(ColorTableParameteriv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ColorTableParameteriv))
	JMP(GL_OFFSET(_gloffset_ColorTableParameteriv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ConvolutionFilter1D))
GL_PREFIX(ConvolutionFilter1D):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ConvolutionFilter1D))
	JMP(GL_OFFSET(_gloffset_ConvolutionFilter1D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ConvolutionFilter2D))
GL_PREFIX(ConvolutionFilter2D):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ConvolutionFilter2D))
	JMP(GL_OFFSET(_gloffset_ConvolutionFilter2D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ConvolutionParameterf))
GL_PREFIX(ConvolutionParameterf):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ConvolutionParameterf))
	JMP(GL_OFFSET(_gloffset_ConvolutionParameterf))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ConvolutionParameterfv))
GL_PREFIX(ConvolutionParameterfv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ConvolutionParameterfv))
	JMP(GL_OFFSET(_gloffset_ConvolutionParameterfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ConvolutionParameteri))
GL_PREFIX(ConvolutionParameteri):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ConvolutionParameteri))
	JMP(GL_OFFSET(_gloffset_ConvolutionParameteri))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ConvolutionParameteriv))
GL_PREFIX(ConvolutionParameteriv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ConvolutionParameteriv))
	JMP(GL_OFFSET(_gloffset_ConvolutionParameteriv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyColorSubTable))
GL_PREFIX(CopyColorSubTable):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_CopyColorSubTable))
	JMP(GL_OFFSET(_gloffset_CopyColorSubTable))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyColorTable))
GL_PREFIX(CopyColorTable):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_CopyColorTable))
	JMP(GL_OFFSET(_gloffset_CopyColorTable))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyConvolutionFilter1D))
GL_PREFIX(CopyConvolutionFilter1D):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_CopyConvolutionFilter1D))
	JMP(GL_OFFSET(_gloffset_CopyConvolutionFilter1D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyConvolutionFilter2D))
GL_PREFIX(CopyConvolutionFilter2D):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_CopyConvolutionFilter2D))
	JMP(GL_OFFSET(_gloffset_CopyConvolutionFilter2D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetColorTable))
GL_PREFIX(GetColorTable):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetColorTable))
	JMP(GL_OFFSET(_gloffset_GetColorTable))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetColorTableParameterfv))
GL_PREFIX(GetColorTableParameterfv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetColorTableParameterfv))
	JMP(GL_OFFSET(_gloffset_GetColorTableParameterfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetColorTableParameteriv))
GL_PREFIX(GetColorTableParameteriv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetColorTableParameteriv))
	JMP(GL_OFFSET(_gloffset_GetColorTableParameteriv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetConvolutionFilter))
GL_PREFIX(GetConvolutionFilter):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetConvolutionFilter))
	JMP(GL_OFFSET(_gloffset_GetConvolutionFilter))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetConvolutionParameterfv))
GL_PREFIX(GetConvolutionParameterfv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetConvolutionParameterfv))
	JMP(GL_OFFSET(_gloffset_GetConvolutionParameterfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetConvolutionParameteriv))
GL_PREFIX(GetConvolutionParameteriv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetConvolutionParameteriv))
	JMP(GL_OFFSET(_gloffset_GetConvolutionParameteriv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetHistogram))
GL_PREFIX(GetHistogram):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetHistogram))
	JMP(GL_OFFSET(_gloffset_GetHistogram))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetHistogramParameterfv))
GL_PREFIX(GetHistogramParameterfv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetHistogramParameterfv))
	JMP(GL_OFFSET(_gloffset_GetHistogramParameterfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetHistogramParameteriv))
GL_PREFIX(GetHistogramParameteriv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetHistogramParameteriv))
	JMP(GL_OFFSET(_gloffset_GetHistogramParameteriv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetMinmax))
GL_PREFIX(GetMinmax):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetMinmax))
	JMP(GL_OFFSET(_gloffset_GetMinmax))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetMinmaxParameterfv))
GL_PREFIX(GetMinmaxParameterfv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetMinmaxParameterfv))
	JMP(GL_OFFSET(_gloffset_GetMinmaxParameterfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetMinmaxParameteriv))
GL_PREFIX(GetMinmaxParameteriv):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetMinmaxParameteriv))
	JMP(GL_OFFSET(_gloffset_GetMinmaxParameteriv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetSeparableFilter))
GL_PREFIX(GetSeparableFilter):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetSeparableFilter))
	JMP(GL_OFFSET(_gloffset_GetSeparableFilter))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Histogram))
GL_PREFIX(Histogram):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Histogram))
	JMP(GL_OFFSET(_gloffset_Histogram))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Minmax))
GL_PREFIX(Minmax):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_Minmax))
	JMP(GL_OFFSET(_gloffset_Minmax))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ResetMinmax))
GL_PREFIX(ResetMinmax):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ResetMinmax))
	JMP(GL_OFFSET(_gloffset_ResetMinmax))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ResetHistogram))
GL_PREFIX(ResetHistogram):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ResetHistogram))
	JMP(GL_OFFSET(_gloffset_ResetHistogram))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SeparableFilter2D))
GL_PREFIX(SeparableFilter2D):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_SeparableFilter2D))
	JMP(GL_OFFSET(_gloffset_SeparableFilter2D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ActiveTextureARB))
GL_PREFIX(ActiveTextureARB):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ActiveTextureARB))
	JMP(GL_OFFSET(_gloffset_ActiveTextureARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ClientActiveTextureARB))
GL_PREFIX(ClientActiveTextureARB):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ClientActiveTextureARB))
	JMP(GL_OFFSET(_gloffset_ClientActiveTextureARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord1dARB))
GL_PREFIX(MultiTexCoord1dARB):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_MultiTexCoord1dARB))
	JMP(GL_OFFSET(_gloffset_MultiTexCoord1dARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord1dvARB))
GL_PREFIX(MultiTexCoord1dvARB):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_MultiTexCoord1dvARB))
	JMP(GL_OFFSET(_gloffset_MultiTexCoord1dvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord1fARB))
GL_PREFIX(MultiTexCoord1fARB):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_MultiTexCoord1fARB))
	JMP(GL_OFFSET(_gloffset_MultiTexCoord1fARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord1fvARB))
GL_PREFIX(MultiTexCoord1fvARB):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_MultiTexCoord1fvARB))
	JMP(GL_OFFSET(_gloffset_MultiTexCoord1fvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord1iARB))
GL_PREFIX(MultiTexCoord1iARB):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_MultiTexCoord1iARB))
	JMP(GL_OFFSET(_gloffset_MultiTexCoord1iARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord1ivARB))
GL_PREFIX(MultiTexCoord1ivARB):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_MultiTexCoord1ivARB))
	JMP(GL_OFFSET(_gloffset_MultiTexCoord1ivARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord1sARB))
GL_PREFIX(MultiTexCoord1sARB):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_MultiTexCoord1sARB))
	JMP(GL_OFFSET(_gloffset_MultiTexCoord1sARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord1svARB))
GL_PREFIX(MultiTexCoord1svARB):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_MultiTexCoord1svARB))
	JMP(GL_OFFSET(_gloffset_MultiTexCoord1svARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord2dARB))
GL_PREFIX(MultiTexCoord2dARB):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_MultiTexCoord2dARB))
	JMP(GL_OFFSET(_gloffset_MultiTexCoord2dARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord2dvARB))
GL_PREFIX(MultiTexCoord2dvARB):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_MultiTexCoord2dvARB))
	JMP(GL_OFFSET(_gloffset_MultiTexCoord2dvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord2fARB))
GL_PREFIX(MultiTexCoord2fARB):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_MultiTexCoord2fARB))
	JMP(GL_OFFSET(_gloffset_MultiTexCoord2fARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord2fvARB))
GL_PREFIX(MultiTexCoord2fvARB):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_MultiTexCoord2fvARB))
	JMP(GL_OFFSET(_gloffset_MultiTexCoord2fvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord2iARB))
GL_PREFIX(MultiTexCoord2iARB):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_MultiTexCoord2iARB))
	JMP(GL_OFFSET(_gloffset_MultiTexCoord2iARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord2ivARB))
GL_PREFIX(MultiTexCoord2ivARB):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_MultiTexCoord2ivARB))
	JMP(GL_OFFSET(_gloffset_MultiTexCoord2ivARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord2sARB))
GL_PREFIX(MultiTexCoord2sARB):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_MultiTexCoord2sARB))
	JMP(GL_OFFSET(_gloffset_MultiTexCoord2sARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord2svARB))
GL_PREFIX(MultiTexCoord2svARB):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_MultiTexCoord2svARB))
	JMP(GL_OFFSET(_gloffset_MultiTexCoord2svARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord3dARB))
GL_PREFIX(MultiTexCoord3dARB):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_MultiTexCoord3dARB))
	JMP(GL_OFFSET(_gloffset_MultiTexCoord3dARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord3dvARB))
GL_PREFIX(MultiTexCoord3dvARB):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_MultiTexCoord3dvARB))
	JMP(GL_OFFSET(_gloffset_MultiTexCoord3dvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord3fARB))
GL_PREFIX(MultiTexCoord3fARB):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_MultiTexCoord3fARB))
	JMP(GL_OFFSET(_gloffset_MultiTexCoord3fARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord3fvARB))
GL_PREFIX(MultiTexCoord3fvARB):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_MultiTexCoord3fvARB))
	JMP(GL_OFFSET(_gloffset_MultiTexCoord3fvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord3iARB))
GL_PREFIX(MultiTexCoord3iARB):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_MultiTexCoord3iARB))
	JMP(GL_OFFSET(_gloffset_MultiTexCoord3iARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord3ivARB))
GL_PREFIX(MultiTexCoord3ivARB):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_MultiTexCoord3ivARB))
	JMP(GL_OFFSET(_gloffset_MultiTexCoord3ivARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord3sARB))
GL_PREFIX(MultiTexCoord3sARB):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_MultiTexCoord3sARB))
	JMP(GL_OFFSET(_gloffset_MultiTexCoord3sARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord3svARB))
GL_PREFIX(MultiTexCoord3svARB):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_MultiTexCoord3svARB))
	JMP(GL_OFFSET(_gloffset_MultiTexCoord3svARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord4dARB))
GL_PREFIX(MultiTexCoord4dARB):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_MultiTexCoord4dARB))
	JMP(GL_OFFSET(_gloffset_MultiTexCoord4dARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord4dvARB))
GL_PREFIX(MultiTexCoord4dvARB):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_MultiTexCoord4dvARB))
	JMP(GL_OFFSET(_gloffset_MultiTexCoord4dvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord4fARB))
GL_PREFIX(MultiTexCoord4fARB):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_MultiTexCoord4fARB))
	JMP(GL_OFFSET(_gloffset_MultiTexCoord4fARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord4fvARB))
GL_PREFIX(MultiTexCoord4fvARB):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_MultiTexCoord4fvARB))
	JMP(GL_OFFSET(_gloffset_MultiTexCoord4fvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord4iARB))
GL_PREFIX(MultiTexCoord4iARB):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_MultiTexCoord4iARB))
	JMP(GL_OFFSET(_gloffset_MultiTexCoord4iARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord4ivARB))
GL_PREFIX(MultiTexCoord4ivARB):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_MultiTexCoord4ivARB))
	JMP(GL_OFFSET(_gloffset_MultiTexCoord4ivARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord4sARB))
GL_PREFIX(MultiTexCoord4sARB):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_MultiTexCoord4sARB))
	JMP(GL_OFFSET(_gloffset_MultiTexCoord4sARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord4svARB))
GL_PREFIX(MultiTexCoord4svARB):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_MultiTexCoord4svARB))
	JMP(GL_OFFSET(_gloffset_MultiTexCoord4svARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(BlendColorEXT))
GL_PREFIX(BlendColorEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_BlendColorEXT))
	JMP(GL_OFFSET(_gloffset_BlendColorEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PolygonOffsetEXT))
GL_PREFIX(PolygonOffsetEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_PolygonOffsetEXT))
	JMP(GL_OFFSET(_gloffset_PolygonOffsetEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexImage3DEXT))
GL_PREFIX(TexImage3DEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexImage3DEXT))
	JMP(GL_OFFSET(_gloffset_TexImage3DEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexSubImage3DEXT))
GL_PREFIX(TexSubImage3DEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexSubImage3DEXT))
	JMP(GL_OFFSET(_gloffset_TexSubImage3DEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyTexSubImage3DEXT))
GL_PREFIX(CopyTexSubImage3DEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_CopyTexSubImage3DEXT))
	JMP(GL_OFFSET(_gloffset_CopyTexSubImage3DEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetTexFilterFuncSGIS))
GL_PREFIX(GetTexFilterFuncSGIS):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetTexFilterFuncSGIS))
	JMP(GL_OFFSET(_gloffset_GetTexFilterFuncSGIS))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexFilterFuncSGIS))
GL_PREFIX(TexFilterFuncSGIS):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexFilterFuncSGIS))
	JMP(GL_OFFSET(_gloffset_TexFilterFuncSGIS))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyTexSubImage1DEXT))
GL_PREFIX(CopyTexSubImage1DEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_CopyTexSubImage1DEXT))
	JMP(GL_OFFSET(_gloffset_CopyTexSubImage1DEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexSubImage1DEXT))
GL_PREFIX(TexSubImage1DEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexSubImage1DEXT))
	JMP(GL_OFFSET(_gloffset_TexSubImage1DEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexSubImage2DEXT))
GL_PREFIX(TexSubImage2DEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_TexSubImage2DEXT))
	JMP(GL_OFFSET(_gloffset_TexSubImage2DEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyTexImage1DEXT))
GL_PREFIX(CopyTexImage1DEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_CopyTexImage1DEXT))
	JMP(GL_OFFSET(_gloffset_CopyTexImage1DEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyTexImage2DEXT))
GL_PREFIX(CopyTexImage2DEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_CopyTexImage2DEXT))
	JMP(GL_OFFSET(_gloffset_CopyTexImage2DEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyTexSubImage2DEXT))
GL_PREFIX(CopyTexSubImage2DEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_CopyTexSubImage2DEXT))
	JMP(GL_OFFSET(_gloffset_CopyTexSubImage2DEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetHistogramEXT))
GL_PREFIX(GetHistogramEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetHistogramEXT))
	JMP(GL_OFFSET(_gloffset_GetHistogramEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetHistogramParameterfvEXT))
GL_PREFIX(GetHistogramParameterfvEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetHistogramParameterfvEXT))
	JMP(GL_OFFSET(_gloffset_GetHistogramParameterfvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetHistogramParameterivEXT))
GL_PREFIX(GetHistogramParameterivEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetHistogramParameterivEXT))
	JMP(GL_OFFSET(_gloffset_GetHistogramParameterivEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetMinmaxEXT))
GL_PREFIX(GetMinmaxEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetMinmaxEXT))
	JMP(GL_OFFSET(_gloffset_GetMinmaxEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetMinmaxParameterfvEXT))
GL_PREFIX(GetMinmaxParameterfvEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetMinmaxParameterfvEXT))
	JMP(GL_OFFSET(_gloffset_GetMinmaxParameterfvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetMinmaxParameterivEXT))
GL_PREFIX(GetMinmaxParameterivEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetMinmaxParameterivEXT))
	JMP(GL_OFFSET(_gloffset_GetMinmaxParameterivEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(HistogramEXT))
GL_PREFIX(HistogramEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_HistogramEXT))
	JMP(GL_OFFSET(_gloffset_HistogramEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MinmaxEXT))
GL_PREFIX(MinmaxEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_MinmaxEXT))
	JMP(GL_OFFSET(_gloffset_MinmaxEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ResetHistogramEXT))
GL_PREFIX(ResetHistogramEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ResetHistogramEXT))
	JMP(GL_OFFSET(_gloffset_ResetHistogramEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ResetMinmaxEXT))
GL_PREFIX(ResetMinmaxEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ResetMinmaxEXT))
	JMP(GL_OFFSET(_gloffset_ResetMinmaxEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ConvolutionFilter1DEXT))
GL_PREFIX(ConvolutionFilter1DEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ConvolutionFilter1DEXT))
	JMP(GL_OFFSET(_gloffset_ConvolutionFilter1DEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ConvolutionFilter2DEXT))
GL_PREFIX(ConvolutionFilter2DEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ConvolutionFilter2DEXT))
	JMP(GL_OFFSET(_gloffset_ConvolutionFilter2DEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ConvolutionParameterfEXT))
GL_PREFIX(ConvolutionParameterfEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ConvolutionParameterfEXT))
	JMP(GL_OFFSET(_gloffset_ConvolutionParameterfEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ConvolutionParameterfvEXT))
GL_PREFIX(ConvolutionParameterfvEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ConvolutionParameterfvEXT))
	JMP(GL_OFFSET(_gloffset_ConvolutionParameterfvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ConvolutionParameteriEXT))
GL_PREFIX(ConvolutionParameteriEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ConvolutionParameteriEXT))
	JMP(GL_OFFSET(_gloffset_ConvolutionParameteriEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ConvolutionParameterivEXT))
GL_PREFIX(ConvolutionParameterivEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ConvolutionParameterivEXT))
	JMP(GL_OFFSET(_gloffset_ConvolutionParameterivEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyConvolutionFilter1DEXT))
GL_PREFIX(CopyConvolutionFilter1DEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_CopyConvolutionFilter1DEXT))
	JMP(GL_OFFSET(_gloffset_CopyConvolutionFilter1DEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyConvolutionFilter2DEXT))
GL_PREFIX(CopyConvolutionFilter2DEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_CopyConvolutionFilter2DEXT))
	JMP(GL_OFFSET(_gloffset_CopyConvolutionFilter2DEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetConvolutionFilterEXT))
GL_PREFIX(GetConvolutionFilterEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetConvolutionFilterEXT))
	JMP(GL_OFFSET(_gloffset_GetConvolutionFilterEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetConvolutionParameterfvEXT))
GL_PREFIX(GetConvolutionParameterfvEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetConvolutionParameterfvEXT))
	JMP(GL_OFFSET(_gloffset_GetConvolutionParameterfvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetConvolutionParameterivEXT))
GL_PREFIX(GetConvolutionParameterivEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetConvolutionParameterivEXT))
	JMP(GL_OFFSET(_gloffset_GetConvolutionParameterivEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetSeparableFilterEXT))
GL_PREFIX(GetSeparableFilterEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetSeparableFilterEXT))
	JMP(GL_OFFSET(_gloffset_GetSeparableFilterEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SeparableFilter2DEXT))
GL_PREFIX(SeparableFilter2DEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_SeparableFilter2DEXT))
	JMP(GL_OFFSET(_gloffset_SeparableFilter2DEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ColorTableParameterfvSGI))
GL_PREFIX(ColorTableParameterfvSGI):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ColorTableParameterfvSGI))
	JMP(GL_OFFSET(_gloffset_ColorTableParameterfvSGI))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ColorTableParameterivSGI))
GL_PREFIX(ColorTableParameterivSGI):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ColorTableParameterivSGI))
	JMP(GL_OFFSET(_gloffset_ColorTableParameterivSGI))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ColorTableSGI))
GL_PREFIX(ColorTableSGI):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ColorTableSGI))
	JMP(GL_OFFSET(_gloffset_ColorTableSGI))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyColorTableSGI))
GL_PREFIX(CopyColorTableSGI):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_CopyColorTableSGI))
	JMP(GL_OFFSET(_gloffset_CopyColorTableSGI))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetColorTableSGI))
GL_PREFIX(GetColorTableSGI):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetColorTableSGI))
	JMP(GL_OFFSET(_gloffset_GetColorTableSGI))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetColorTableParameterfvSGI))
GL_PREFIX(GetColorTableParameterfvSGI):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetColorTableParameterfvSGI))
	JMP(GL_OFFSET(_gloffset_GetColorTableParameterfvSGI))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetColorTableParameterivSGI))
GL_PREFIX(GetColorTableParameterivSGI):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetColorTableParameterivSGI))
	JMP(GL_OFFSET(_gloffset_GetColorTableParameterivSGI))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GenTexturesEXT))
GL_PREFIX(GenTexturesEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GenTexturesEXT))
	JMP(GL_OFFSET(_gloffset_GenTexturesEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DeleteTexturesEXT))
GL_PREFIX(DeleteTexturesEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_DeleteTexturesEXT))
	JMP(GL_OFFSET(_gloffset_DeleteTexturesEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(BindTextureEXT))
GL_PREFIX(BindTextureEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_BindTextureEXT))
	JMP(GL_OFFSET(_gloffset_BindTextureEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PrioritizeTexturesEXT))
GL_PREFIX(PrioritizeTexturesEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_PrioritizeTexturesEXT))
	JMP(GL_OFFSET(_gloffset_PrioritizeTexturesEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(AreTexturesResidentEXT))
GL_PREFIX(AreTexturesResidentEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_AreTexturesResidentEXT))
	JMP(GL_OFFSET(_gloffset_AreTexturesResidentEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(IsTextureEXT))
GL_PREFIX(IsTextureEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_IsTextureEXT))
	JMP(GL_OFFSET(_gloffset_IsTextureEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(BlendEquationEXT))
GL_PREFIX(BlendEquationEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_BlendEquationEXT))
	JMP(GL_OFFSET(_gloffset_BlendEquationEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexPointerEXT))
GL_PREFIX(VertexPointerEXT):
	JMP(GLNAME(_mesa_fallback_VertexPointerEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(NormalPointerEXT))
GL_PREFIX(NormalPointerEXT):
	JMP(GLNAME(_mesa_fallback_NormalPointerEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ColorPointerEXT))
GL_PREFIX(ColorPointerEXT):
	JMP(GLNAME(_mesa_fallback_ColorPointerEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(IndexPointerEXT))
GL_PREFIX(IndexPointerEXT):
	JMP(GLNAME(_mesa_fallback_IndexPointerEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoordPointerEXT))
GL_PREFIX(TexCoordPointerEXT):
	JMP(GLNAME(_mesa_fallback_TexCoordPointerEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EdgeFlagPointerEXT))
GL_PREFIX(EdgeFlagPointerEXT):
	JMP(GLNAME(_mesa_fallback_EdgeFlagPointerEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetPointervEXT))
GL_PREFIX(GetPointervEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetPointervEXT))
	JMP(GL_OFFSET(_gloffset_GetPointervEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ArrayElementEXT))
GL_PREFIX(ArrayElementEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ArrayElementEXT))
	JMP(GL_OFFSET(_gloffset_ArrayElementEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DrawArraysEXT))
GL_PREFIX(DrawArraysEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_DrawArraysEXT))
	JMP(GL_OFFSET(_gloffset_DrawArraysEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PointParameterfEXT))
GL_PREFIX(PointParameterfEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_PointParameterfEXT))
	JMP(GL_OFFSET(_gloffset_PointParameterfEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PointParameterfvEXT))
GL_PREFIX(PointParameterfvEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_PointParameterfvEXT))
	JMP(GL_OFFSET(_gloffset_PointParameterfvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(HintPGI))
GL_PREFIX(HintPGI):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_HintPGI))
	JMP(GL_OFFSET(_gloffset_HintPGI))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ColorTableEXT))
GL_PREFIX(ColorTableEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ColorTableEXT))
	JMP(GL_OFFSET(_gloffset_ColorTableEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ColorSubTableEXT))
GL_PREFIX(ColorSubTableEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ColorSubTableEXT))
	JMP(GL_OFFSET(_gloffset_ColorSubTableEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetColorTableEXT))
GL_PREFIX(GetColorTableEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetColorTableEXT))
	JMP(GL_OFFSET(_gloffset_GetColorTableEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetColorTableParameterfvEXT))
GL_PREFIX(GetColorTableParameterfvEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetColorTableParameterfvEXT))
	JMP(GL_OFFSET(_gloffset_GetColorTableParameterfvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetColorTableParameterivEXT))
GL_PREFIX(GetColorTableParameterivEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_GetColorTableParameterivEXT))
	JMP(GL_OFFSET(_gloffset_GetColorTableParameterivEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LockArraysEXT))
GL_PREFIX(LockArraysEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_LockArraysEXT))
	JMP(GL_OFFSET(_gloffset_LockArraysEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(UnlockArraysEXT))
GL_PREFIX(UnlockArraysEXT):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_UnlockArraysEXT))
	JMP(GL_OFFSET(_gloffset_UnlockArraysEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(BlendFuncSeparateINGR))
GL_PREFIX(BlendFuncSeparateINGR):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_BlendFuncSeparateINGR))
	JMP(GL_OFFSET(_gloffset_BlendFuncSeparateINGR))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2iMESA))
GL_PREFIX(WindowPos2iMESA):
	JMP(GLNAME(_mesa_fallback_WindowPos2iMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2sMESA))
GL_PREFIX(WindowPos2sMESA):
	JMP(GLNAME(_mesa_fallback_WindowPos2sMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2fMESA))
GL_PREFIX(WindowPos2fMESA):
	JMP(GLNAME(_mesa_fallback_WindowPos2fMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2dMESA))
GL_PREFIX(WindowPos2dMESA):
	JMP(GLNAME(_mesa_fallback_WindowPos2dMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2ivMESA))
GL_PREFIX(WindowPos2ivMESA):
	JMP(GLNAME(_mesa_fallback_WindowPos2ivMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2svMESA))
GL_PREFIX(WindowPos2svMESA):
	JMP(GLNAME(_mesa_fallback_WindowPos2svMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2fvMESA))
GL_PREFIX(WindowPos2fvMESA):
	JMP(GLNAME(_mesa_fallback_WindowPos2fvMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2dvMESA))
GL_PREFIX(WindowPos2dvMESA):
	JMP(GLNAME(_mesa_fallback_WindowPos2dvMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3iMESA))
GL_PREFIX(WindowPos3iMESA):
	JMP(GLNAME(_mesa_fallback_WindowPos3iMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3sMESA))
GL_PREFIX(WindowPos3sMESA):
	JMP(GLNAME(_mesa_fallback_WindowPos3sMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3fMESA))
GL_PREFIX(WindowPos3fMESA):
	JMP(GLNAME(_mesa_fallback_WindowPos3fMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3dMESA))
GL_PREFIX(WindowPos3dMESA):
	JMP(GLNAME(_mesa_fallback_WindowPos3dMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3ivMESA))
GL_PREFIX(WindowPos3ivMESA):
	JMP(GLNAME(_mesa_fallback_WindowPos3ivMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3svMESA))
GL_PREFIX(WindowPos3svMESA):
	JMP(GLNAME(_mesa_fallback_WindowPos3svMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3fvMESA))
GL_PREFIX(WindowPos3fvMESA):
	JMP(GLNAME(_mesa_fallback_WindowPos3fvMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3dvMESA))
GL_PREFIX(WindowPos3dvMESA):
	JMP(GLNAME(_mesa_fallback_WindowPos3dvMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos4iMESA))
GL_PREFIX(WindowPos4iMESA):
	JMP(GLNAME(_mesa_fallback_WindowPos4iMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos4sMESA))
GL_PREFIX(WindowPos4sMESA):
	JMP(GLNAME(_mesa_fallback_WindowPos4sMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos4fMESA))
GL_PREFIX(WindowPos4fMESA):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_WindowPos4fMESA))
	JMP(GL_OFFSET(_gloffset_WindowPos4fMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos4dMESA))
GL_PREFIX(WindowPos4dMESA):
	JMP(GLNAME(_mesa_fallback_WindowPos4dMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos4ivMESA))
GL_PREFIX(WindowPos4ivMESA):
	JMP(GLNAME(_mesa_fallback_WindowPos4ivMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos4svMESA))
GL_PREFIX(WindowPos4svMESA):
	JMP(GLNAME(_mesa_fallback_WindowPos4svMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos4fvMESA))
GL_PREFIX(WindowPos4fvMESA):
	JMP(GLNAME(_mesa_fallback_WindowPos4fvMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos4dvMESA))
GL_PREFIX(WindowPos4dvMESA):
	JMP(GLNAME(_mesa_fallback_WindowPos4dvMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ResizeBuffersMESA))
GL_PREFIX(ResizeBuffersMESA):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_ResizeBuffersMESA))
	JMP(GL_OFFSET(_gloffset_ResizeBuffersMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LoadTransposeMatrixdARB))
GL_PREFIX(LoadTransposeMatrixdARB):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_LoadTransposeMatrixdARB))
	JMP(GL_OFFSET(_gloffset_LoadTransposeMatrixdARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LoadTransposeMatrixfARB))
GL_PREFIX(LoadTransposeMatrixfARB):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_LoadTransposeMatrixfARB))
	JMP(GL_OFFSET(_gloffset_LoadTransposeMatrixfARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultTransposeMatrixdARB))
GL_PREFIX(MultTransposeMatrixdARB):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_MultTransposeMatrixdARB))
	JMP(GL_OFFSET(_gloffset_MultTransposeMatrixdARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultTransposeMatrixfARB))
GL_PREFIX(MultTransposeMatrixfARB):
	MOV_L(GLNAME(_mesa_Dispatch), EAX)
	TEST_L(EAX, EAX)
	JZ(GLNAME(_mesa_fallback_MultTransposeMatrixfARB))
	JMP(GL_OFFSET(_gloffset_MultTransposeMatrixfARB))

#endif

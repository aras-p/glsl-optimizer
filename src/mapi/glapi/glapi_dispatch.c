/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */


/**
 * \file glapi_dispatch.c
 *
 * This file generates all the gl* function entrypoints.  This code is not
 * used if optimized assembly stubs are available (e.g., using
 * glapi/glapi_x86.S on IA32 or glapi/glapi_sparc.S on SPARC).
 *
 * \note
 * This file is also used to build the client-side libGL that loads DRI-based
 * device drivers.  At build-time it is symlinked to src/glx.
 *
 * \author Brian Paul <brian@precisioninsight.com>
 */

#include "glapi/glapi_priv.h"
#include "glapi/glapitable.h"


#if !(defined(USE_X86_ASM) || defined(USE_X86_64_ASM) || defined(USE_SPARC_ASM))

#if defined(_WIN32)
#define KEYWORD1 GLAPI
#else
#define KEYWORD1 PUBLIC
#endif

#define KEYWORD2 GLAPIENTRY

#if defined(USE_MGL_NAMESPACE)
#define NAME(func)  mgl##func
#else
#define NAME(func)  gl##func
#endif

#if 0  /* Use this to log GL calls to stdout (for DEBUG only!) */

#define F stdout
#define DISPATCH(FUNC, ARGS, MESSAGE)		\
   fprintf MESSAGE;				\
   GET_DISPATCH()->FUNC ARGS

#define RETURN_DISPATCH(FUNC, ARGS, MESSAGE) 	\
   fprintf MESSAGE;				\
   return GET_DISPATCH()->FUNC ARGS

#else

#define DISPATCH(FUNC, ARGS, MESSAGE)		\
   GET_DISPATCH()->FUNC ARGS

#define RETURN_DISPATCH(FUNC, ARGS, MESSAGE) 	\
   return GET_DISPATCH()->FUNC ARGS

#endif /* logging */


#ifndef GLAPIENTRY
#define GLAPIENTRY
#endif

#ifdef GLX_INDIRECT_RENDERING
/* those link to libglapi.a should provide the entry points */
#define _GLAPI_SKIP_PROTO_ENTRY_POINTS
#endif

/* These prototypes are necessary because GLES1 library builds will create
 * dispatch functions for them.  We can't directly include GLES/gl.h because
 * it would conflict the previously-included GL/gl.h.  Since GLES1 ABI is not
 * expected to every add more functions, the path of least resistance is to
 * just duplicate the prototypes for the functions that aren't already in
 * desktop OpenGL.
 */
#include <GLES/glplatform.h>

GL_API void GL_APIENTRY glClearDepthf (GLclampf depth);
GL_API void GL_APIENTRY glClipPlanef (GLenum plane, const GLfloat *equation);
GL_API void GL_APIENTRY glFrustumf (GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar);
GL_API void GL_APIENTRY glGetClipPlanef (GLenum pname, GLfloat eqn[4]);
GL_API void GL_APIENTRY glOrthof (GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar);

GL_API void GL_APIENTRY glAlphaFuncx (GLenum func, GLclampx ref);
GL_API void GL_APIENTRY glClearColorx (GLclampx red, GLclampx green, GLclampx blue, GLclampx alpha);
GL_API void GL_APIENTRY glClearDepthx (GLclampx depth);
GL_API void GL_APIENTRY glClipPlanex (GLenum plane, const GLfixed *equation);
GL_API void GL_APIENTRY glColor4x (GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha);
GL_API void GL_APIENTRY glDepthRangex (GLclampx zNear, GLclampx zFar);
GL_API void GL_APIENTRY glFogx (GLenum pname, GLfixed param);
GL_API void GL_APIENTRY glFogxv (GLenum pname, const GLfixed *params);
GL_API void GL_APIENTRY glFrustumx (GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed zNear, GLfixed zFar);
GL_API void GL_APIENTRY glGetClipPlanex (GLenum pname, GLfixed eqn[4]);
GL_API void GL_APIENTRY glGetFixedv (GLenum pname, GLfixed *params);
GL_API void GL_APIENTRY glGetLightxv (GLenum light, GLenum pname, GLfixed *params);
GL_API void GL_APIENTRY glGetMaterialxv (GLenum face, GLenum pname, GLfixed *params);
GL_API void GL_APIENTRY glGetTexEnvxv (GLenum env, GLenum pname, GLfixed *params);
GL_API void GL_APIENTRY glGetTexParameterxv (GLenum target, GLenum pname, GLfixed *params);
GL_API void GL_APIENTRY glLightModelx (GLenum pname, GLfixed param);
GL_API void GL_APIENTRY glLightModelxv (GLenum pname, const GLfixed *params);
GL_API void GL_APIENTRY glLightx (GLenum light, GLenum pname, GLfixed param);
GL_API void GL_APIENTRY glLightxv (GLenum light, GLenum pname, const GLfixed *params);
GL_API void GL_APIENTRY glLineWidthx (GLfixed width);
GL_API void GL_APIENTRY glLoadMatrixx (const GLfixed *m);
GL_API void GL_APIENTRY glMaterialx (GLenum face, GLenum pname, GLfixed param);
GL_API void GL_APIENTRY glMaterialxv (GLenum face, GLenum pname, const GLfixed *params);
GL_API void GL_APIENTRY glMultMatrixx (const GLfixed *m);
GL_API void GL_APIENTRY glMultiTexCoord4x (GLenum target, GLfixed s, GLfixed t, GLfixed r, GLfixed q);
GL_API void GL_APIENTRY glNormal3x (GLfixed nx, GLfixed ny, GLfixed nz);
GL_API void GL_APIENTRY glOrthox (GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed zNear, GLfixed zFar);
GL_API void GL_APIENTRY glPointParameterx (GLenum pname, GLfixed param);
GL_API void GL_APIENTRY glPointParameterxv (GLenum pname, const GLfixed *params);
GL_API void GL_APIENTRY glPointSizex (GLfixed size);
GL_API void GL_APIENTRY glPolygonOffsetx (GLfixed factor, GLfixed units);
GL_API void GL_APIENTRY glRotatex (GLfixed angle, GLfixed x, GLfixed y, GLfixed z);
GL_API void GL_APIENTRY glSampleCoveragex (GLclampx value, GLboolean invert);
GL_API void GL_APIENTRY glScalex (GLfixed x, GLfixed y, GLfixed z);
GL_API void GL_APIENTRY glTexEnvx (GLenum target, GLenum pname, GLfixed param);
GL_API void GL_APIENTRY glTexEnvxv (GLenum target, GLenum pname, const GLfixed *params);
GL_API void GL_APIENTRY glTexParameterx (GLenum target, GLenum pname, GLfixed param);
GL_API void GL_APIENTRY glTexParameterxv (GLenum target, GLenum pname, const GLfixed *params);
GL_API void GL_APIENTRY glTranslatex (GLfixed x, GLfixed y, GLfixed z);
GL_API void GL_APIENTRY glPointSizePointerOES (GLenum type, GLsizei stride, const GLvoid *pointer);

#include "glapi/glapitemp.h"

#endif /* USE_X86_ASM */

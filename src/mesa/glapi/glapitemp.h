/* $Id: glapitemp.h,v 1.29 2001/06/05 19:29:41 brianp Exp $ */

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


/*
 * This file is a template which generates the OpenGL API entry point
 * functions.  It should be included by a .c file which first defines
 * the following macros:
 *   KEYWORD1 - usually nothing, but might be __declspec(dllexport) on Win32
 *   KEYWORD2 - usually nothing, but might be __stdcall on Win32
 *   NAME(n)  - builds the final function name (usually add "gl" prefix)
 *   DISPATCH(func, args, msg) - code to do dispatch of named function.
 *                               msg is a printf-style debug message.
 *   RETURN_DISPATCH(func, args, msg) - code to do dispatch with a return value
 *
 * Here's an example which generates the usual OpenGL functions:
 *   #define KEYWORD1
 *   #define KEYWORD2
 *   #define NAME(func)  gl##func
 *   #define DISPATCH(func, args, msg)                           \
 *          struct _glapi_table *dispatch = CurrentDispatch;     \
 *          (*dispatch->func) args
 *   #define RETURN DISPATCH(func, args, msg)                    \
 *          struct _glapi_table *dispatch = CurrentDispatch;     \
 *          return (*dispatch->func) args
 *
 */


#ifndef KEYWORD1
#define KEYWORD1
#endif

#ifndef KEYWORD2
#define KEYWORD2
#endif

#ifndef NAME
#error NAME must be defined
#endif

#ifndef DISPATCH
#error DISPATCH must be defined
#endif

#ifndef RETURN_DISPATCH
#error RETURN_DISPATCH must be defined
#endif


/*
 * XXX
 * Most functions need the msg (printf-message) parameter to be finished.
 * I.e. replace ";" with the real info.
 */

/*
 * XXX
 * Someday this code should be automatically generated from a spec file
 * like that used in the SGI OpenGL SI.
 */



/* GL 1.0 */

KEYWORD1 void KEYWORD2 NAME(Accum)(GLenum op, GLfloat value)
{
   DISPATCH(Accum, (op, value), (F, "glAccum(0x%x, %g);", op, value));
}

KEYWORD1 void KEYWORD2 NAME(AlphaFunc)(GLenum func, GLclampf ref)
{
   DISPATCH(AlphaFunc, (func, ref), (F, "glAlphaFunc(0x%x, %g);", func, ref));
}

KEYWORD1 void KEYWORD2 NAME(Bitmap)(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap)
{
   DISPATCH(Bitmap, (width, height, xorig, yorig, xmove, ymove, bitmap), (F, "glBitmap(%d %d %g %g %g %g %p;", width, height, xorig, yorig, xmove, ymove, bitmap));
}

KEYWORD1 void KEYWORD2 NAME(BlendFunc)(GLenum sfactor, GLenum dfactor)
{
   DISPATCH(BlendFunc, (sfactor, dfactor), (F, "glBlendFunc(0x%x, 0x%x);", sfactor, dfactor));
}

KEYWORD1 void KEYWORD2 NAME(Clear)(GLbitfield mask)
{
   DISPATCH(Clear, (mask), (F, "glClear(0x%x);", mask));
}

KEYWORD1 void KEYWORD2 NAME(ClearAccum)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
   DISPATCH(ClearAccum, (red, green, blue, alpha), (F, "glClearAccum(%g, %g, %g, %g);", red, green, blue, alpha));
}

KEYWORD1 void KEYWORD2 NAME(ClearColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
   DISPATCH(ClearColor, (red, green, blue, alpha), (F, "glClearColor(%g, %g, %g, %g);", red, green, blue, alpha));
}

KEYWORD1 void KEYWORD2 NAME(ClearDepth)(GLclampd depth)
{
   DISPATCH(ClearDepth, (depth), (F, "glClearDepth(%g);", depth));
}

KEYWORD1 void KEYWORD2 NAME(ClearIndex)(GLfloat c)
{
   DISPATCH(ClearIndex, (c), (F, "glClearIndex(%g);", c));
}

KEYWORD1 void KEYWORD2 NAME(ClearStencil)(GLint s)
{
   DISPATCH(ClearStencil, (s), (F, "glClearStencil(%d);", s));
}

KEYWORD1 void KEYWORD2 NAME(ClipPlane)(GLenum plane, const GLdouble *equation)
{
   DISPATCH(ClipPlane, (plane, equation), (F, "glClipPlane(%p);", equation));
}

KEYWORD1 void KEYWORD2 NAME(ColorMask)(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
   DISPATCH(ColorMask, (red, green, blue, alpha), (F, "glColorMask(%d, %d, %d, %d);", red, green, blue, alpha));
}

KEYWORD1 void KEYWORD2 NAME(ColorMaterial)(GLenum face, GLenum mode)
{
   DISPATCH(ColorMaterial, (face, mode), (F, "glColorMaterial(0x%x, 0x%x);", face, mode));
}

KEYWORD1 void KEYWORD2 NAME(CopyPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
{
   DISPATCH(CopyPixels, (x, y, width, height, type), (F, "glCopyPixels(%d, %d, %d, %d, 0x%x;", x, y, width, height, type));
}

KEYWORD1 void KEYWORD2 NAME(CullFace)(GLenum mode)
{
   DISPATCH(CullFace, (mode), (F, "glCullFace(0x%x);", mode));
}

KEYWORD1 void KEYWORD2 NAME(DepthFunc)(GLenum func)
{
   DISPATCH(DepthFunc, (func), (F, "glDepthFunc(0x%x);", func));
}

KEYWORD1 void KEYWORD2 NAME(DepthMask)(GLboolean flag)
{
   DISPATCH(DepthMask, (flag), (F, "glDepthMask(%d);", flag));
}

KEYWORD1 void KEYWORD2 NAME(DepthRange)(GLclampd nearVal, GLclampd farVal)
{
   DISPATCH(DepthRange, (nearVal, farVal), (F, "glDepthRange(%g, %g;", nearVal, farVal));
}

KEYWORD1 void KEYWORD2 NAME(DeleteLists)(GLuint list, GLsizei range)
{
   DISPATCH(DeleteLists, (list, range), (F, "glDeleteLists(%u, %d);", list, range));
}

KEYWORD1 void KEYWORD2 NAME(Disable)(GLenum cap)
{
   DISPATCH(Disable, (cap), (F, "glDisable(0x%x);", cap));
}

KEYWORD1 void KEYWORD2 NAME(DrawBuffer)(GLenum mode)
{
   DISPATCH(DrawBuffer, (mode), (F, "glDrawBuffer(0x%x);", mode));
}

KEYWORD1 void KEYWORD2 NAME(DrawElements)(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)
{
   DISPATCH(DrawElements, (mode, count, type, indices), (F, "glDrawElements(0x%x, %d, 0x%x, %p;", mode, count, type, indices));
}

KEYWORD1 void KEYWORD2 NAME(DrawPixels)(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
   DISPATCH(DrawPixels, (width, height, format, type, pixels), (F, "glDrawPixels(%d, %d, 0x%x, 0x%x, %p);", width, height, format, type, pixels));
}

KEYWORD1 void KEYWORD2 NAME(Enable)(GLenum cap)
{
   DISPATCH(Enable, (cap), (F, "glEnable(0x%x);", cap));
}

KEYWORD1 void KEYWORD2 NAME(EndList)(void)
{
   DISPATCH(EndList, (), (F, "glEndList();"));
}

KEYWORD1 void KEYWORD2 NAME(EvalMesh1)(GLenum mode, GLint i1, GLint i2)
{
   DISPATCH(EvalMesh1, (mode, i1, i2), (F, "glEvalMesh(0x%x, %d, %d);", mode, i1, i2));
}

KEYWORD1 void KEYWORD2 NAME(EvalMesh2)(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2)
{
   DISPATCH(EvalMesh2, (mode, i1, i2, j1, j2), (F, "glEvalMesh2(0x%x, %d, %d, %d, %d);", mode, i1, i2, j1, j2));
}

KEYWORD1 void KEYWORD2 NAME(FeedbackBuffer)(GLsizei size, GLenum type, GLfloat *buffer)
{
   DISPATCH(FeedbackBuffer, (size, type, buffer), (F, "glFeedbackBuffer(%d, 0x%x, %p);", size, type, buffer));
}

KEYWORD1 void KEYWORD2 NAME(Finish)(void)
{
   DISPATCH(Finish, (), (F, "glFinish();"));
}

KEYWORD1 void KEYWORD2 NAME(Flush)(void)
{
   DISPATCH(Flush, (), (F, "glFlush();"));
}

KEYWORD1 void KEYWORD2 NAME(Fogf)(GLenum pname, GLfloat param)
{
   DISPATCH(Fogf, (pname, param), (F, "glFogf(0x%x, %g);", pname, param));
}

KEYWORD1 void KEYWORD2 NAME(Fogi)(GLenum pname, GLint param)
{
   DISPATCH(Fogi, (pname, param), (F, "glFogi(0x%x, %d);", pname, param));
}

KEYWORD1 void KEYWORD2 NAME(Fogfv)(GLenum pname, const GLfloat *params)
{
   DISPATCH(Fogfv, (pname, params), (F, "glFogfv(0x%x, %p);", pname, params));
}

KEYWORD1 void KEYWORD2 NAME(Fogiv)(GLenum pname, const GLint *params)
{
   DISPATCH(Fogiv, (pname, params), (F, "glFogiv(0x%x, %p);", pname, params));
}

KEYWORD1 void KEYWORD2 NAME(FrontFace)(GLenum mode)
{
   DISPATCH(FrontFace, (mode), (F, "glFrontFace(0x%x);", mode));
}

KEYWORD1 void KEYWORD2 NAME(Frustum)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble nearval, GLdouble farval)
{
   DISPATCH(Frustum, (left, right, bottom, top, nearval, farval), (F, "glFrustum(%f, %f, %f, %f, %f, %f);", left, right, bottom, top, nearval, farval));
}

KEYWORD1 GLuint KEYWORD2 NAME(GenLists)(GLsizei range)
{
   RETURN_DISPATCH(GenLists, (range), (F, "glGenLists(%d);", range));
}

KEYWORD1 void KEYWORD2 NAME(GetBooleanv)(GLenum pname, GLboolean *params)
{
   DISPATCH(GetBooleanv, (pname, params), (F, "glGetBooleanv(0x%x, %p);", pname, params));
}

KEYWORD1 void KEYWORD2 NAME(GetClipPlane)(GLenum plane, GLdouble *equation)
{
   DISPATCH(GetClipPlane, (plane, equation), (F, "glGetClipPlane(x0%x, %p);", plane, equation));
}

KEYWORD1 void KEYWORD2 NAME(GetDoublev)(GLenum pname, GLdouble *params)
{
   DISPATCH(GetDoublev, (pname, params), (F, "glGetDoublev(0x%x, %p);", pname, params));
}

KEYWORD1 GLenum KEYWORD2 NAME(GetError)(void)
{
   RETURN_DISPATCH(GetError, (), (F, "glGetError();"));
}

KEYWORD1 void KEYWORD2 NAME(GetFloatv)(GLenum pname, GLfloat *params)
{
   DISPATCH(GetFloatv, (pname, params), (F, "glGetFloatv(0x%x, %p);", pname, params));
}

KEYWORD1 void KEYWORD2 NAME(GetIntegerv)(GLenum pname, GLint *params)
{
   DISPATCH(GetIntegerv, (pname, params), (F, "glGetIntegerv(0x%x, %p);", pname, params));
}

KEYWORD1 void KEYWORD2 NAME(GetLightfv)(GLenum light, GLenum pname, GLfloat *params)
{
   DISPATCH(GetLightfv, (light, pname, params), (F, "glGetLightfv(0x%x, 0x%x, %p);", light, pname, params));
}

KEYWORD1 void KEYWORD2 NAME(GetLightiv)(GLenum light, GLenum pname, GLint *params)
{
   DISPATCH(GetLightiv, (light, pname, params), (F, "glGetLightiv(0x%x, 0x%x, %p);", light, pname, params));
}

KEYWORD1 void KEYWORD2 NAME(GetMapdv)(GLenum target, GLenum query, GLdouble *v)
{
   DISPATCH(GetMapdv, (target, query, v), (F, "glGetMapdv(0x%x, 0x%x, %p);", target, query, v));
}

KEYWORD1 void KEYWORD2 NAME(GetMapfv)(GLenum target, GLenum query, GLfloat *v)
{
   DISPATCH(GetMapfv, (target, query, v), (F, "glGetMapfv(0x%x, 0x%x, %p);", target, query, v));
}

KEYWORD1 void KEYWORD2 NAME(GetMapiv)(GLenum target, GLenum query, GLint *v)
{
   DISPATCH(GetMapiv, (target, query, v), (F, "glGetMapiv(0x%x, 0x%x, %p);", target, query, v));
}

KEYWORD1 void KEYWORD2 NAME(GetMaterialfv)(GLenum face, GLenum pname, GLfloat *params)
{
   DISPATCH(GetMaterialfv, (face, pname, params), (F, "glGetMaterialfv(0x%x, 0x%x, %p;", face, pname, params));
}

KEYWORD1 void KEYWORD2 NAME(GetMaterialiv)(GLenum face, GLenum pname, GLint *params)
{
   DISPATCH(GetMaterialiv, (face, pname, params), (F, "glGetMaterialiv(0x%x, 0x%x, %p);", face, pname, params));
}

KEYWORD1 void KEYWORD2 NAME(GetPixelMapfv)(GLenum map, GLfloat *values)
{
   DISPATCH(GetPixelMapfv, (map, values), (F, "glGetPixelMapfv(0x%x, %p);", map, values));
}

KEYWORD1 void KEYWORD2 NAME(GetPixelMapuiv)(GLenum map, GLuint *values)
{
   DISPATCH(GetPixelMapuiv, (map, values), (F, "glGetPixelMapuiv(0x%x, %p);", map, values));
}

KEYWORD1 void KEYWORD2 NAME(GetPixelMapusv)(GLenum map, GLushort *values)
{
   DISPATCH(GetPixelMapusv, (map, values), (F, "glGetPixelMapusv(0x%x, %p);", map, values));
}

KEYWORD1 void KEYWORD2 NAME(GetPolygonStipple)(GLubyte *mask)
{
   DISPATCH(GetPolygonStipple, (mask), (F, "glGetPolygonStipple(%p);", mask));
}

KEYWORD1 const GLubyte * KEYWORD2 NAME(GetString)(GLenum name)
{
   RETURN_DISPATCH(GetString, (name), (F, "glGetString(0x%x);", name));
}

KEYWORD1 void KEYWORD2 NAME(GetTexEnvfv)(GLenum target, GLenum pname, GLfloat *params)
{
   DISPATCH(GetTexEnvfv, (target, pname, params), (F, "glGettexEnvfv(0x%x, 0x%x, %p);", target, pname, params));
}

KEYWORD1 void KEYWORD2 NAME(GetTexEnviv)(GLenum target, GLenum pname, GLint *params)
{
   DISPATCH(GetTexEnviv, (target, pname, params), (F, "glGetTexEnviv(0x%x, 0x%x, %p);", target, pname, params));
}

KEYWORD1 void KEYWORD2 NAME(GetTexGeniv)(GLenum target, GLenum pname, GLint *params)
{
   DISPATCH(GetTexGeniv, (target, pname, params), (F, "glGetTexGeniv(0x%x, 0x%x, %p);", target, pname, params));
}

KEYWORD1 void KEYWORD2 NAME(GetTexGendv)(GLenum target, GLenum pname, GLdouble *params)
{
   DISPATCH(GetTexGendv, (target, pname, params), (F, "glGetTexGendv(0x%x, 0x%x, %p;", target, pname, params));
}

KEYWORD1 void KEYWORD2 NAME(GetTexGenfv)(GLenum target, GLenum pname, GLfloat *params)
{
   DISPATCH(GetTexGenfv, (target, pname, params), (F, "glGetTexGenfv(0x%x, 0x%x, %p);", target, pname, params));
}

KEYWORD1 void KEYWORD2 NAME(GetTexImage)(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels)
{
   DISPATCH(GetTexImage, (target, level, format, type, pixels), (F, "glGetTexImage(0x%x, %d, 0x%x, 0x%x, %p);", target, level, format, type, pixels));
}

KEYWORD1 void KEYWORD2 NAME(GetTexLevelParameterfv)(GLenum target, GLint level, GLenum pname, GLfloat *params)
{
   DISPATCH(GetTexLevelParameterfv, (target, level, pname, params), (F, "glGetTexLevelParameterfv(0x%x, %d, 0x%x, %p);", target, level, pname, params));
}

KEYWORD1 void KEYWORD2 NAME(GetTexLevelParameteriv)(GLenum target, GLint level, GLenum pname, GLint *params)
{
   DISPATCH(GetTexLevelParameteriv, (target, level, pname, params), (F, "glGetTexLevelParameteriv(0x%x, %d, 0x%x, %p);", target, level, pname, params));
}

KEYWORD1 void KEYWORD2 NAME(GetTexParameterfv)(GLenum target, GLenum pname, GLfloat *params)
{
   DISPATCH(GetTexParameterfv, (target, pname, params), (F, "glGetTexParameterfv(0x%x, 0x%x, %p);", target, pname, params));
}

KEYWORD1 void KEYWORD2 NAME(GetTexParameteriv)(GLenum target, GLenum pname, GLint *params)
{
   DISPATCH(GetTexParameteriv, (target, pname, params), (F, "glGetTexParameteriv(0x%x, 0x%x, %p);", target, pname, params));
}

KEYWORD1 void KEYWORD2 NAME(Hint)(GLenum target, GLenum mode)
{
   DISPATCH(Hint, (target, mode), (F, "glHint(0x%x, 0x%x);", target, mode));
}

KEYWORD1 void KEYWORD2 NAME(IndexMask)(GLuint mask)
{
   DISPATCH(IndexMask, (mask), (F, "glIndexMask(%u);", mask));
}

KEYWORD1 void KEYWORD2 NAME(InitNames)(void)
{
   DISPATCH(InitNames, (), (F, "glInitNames();"));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsEnabled)(GLenum cap)
{
   RETURN_DISPATCH(IsEnabled, (cap), (F, "glIsEnabled(0x%x);", cap));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsList)(GLuint list)
{
   RETURN_DISPATCH(IsList, (list), (F, "glIsList(%u);", list));
}

KEYWORD1 void KEYWORD2 NAME(Lightf)(GLenum light, GLenum pname, GLfloat param)
{
   DISPATCH(Lightf, (light, pname, param), (F, "glLightfv(0x%x, 0x%x, %g);", light, pname, param));
}

KEYWORD1 void KEYWORD2 NAME(Lighti)(GLenum light, GLenum pname, GLint param)
{
   DISPATCH(Lighti, (light, pname, param), (F, "glLighti(0x%x, 0x%x, %d);", light, pname, param));
}

KEYWORD1 void KEYWORD2 NAME(Lightfv)(GLenum light, GLenum pname, const GLfloat *params)
{
   DISPATCH(Lightfv, (light, pname, params), (F, "glLightfv(0x%x, 0x%x, %p);", light, pname, params));
}

KEYWORD1 void KEYWORD2 NAME(Lightiv)(GLenum light, GLenum pname, const GLint *params)
{
   DISPATCH(Lightiv, (light, pname, params), (F, "glLightiv(0x%x, 0x%x, %p);", light, pname, params));
}

KEYWORD1 void KEYWORD2 NAME(LightModelf)(GLenum pname, GLfloat param)
{
   DISPATCH(LightModelf, (pname, param), (F, "glLightModelf(0x%x, %f);", pname, param));
}

KEYWORD1 void KEYWORD2 NAME(LightModeli)(GLenum pname, GLint param)
{
   DISPATCH(LightModeli, (pname, param), (F, "glLightModeli(0x%x, %d);", pname, param));
}

KEYWORD1 void KEYWORD2 NAME(LightModelfv)(GLenum pname, const GLfloat *params)
{
   DISPATCH(LightModelfv, (pname, params), (F, "glLightModelfv(0x%x, %p);", pname, params));
}

KEYWORD1 void KEYWORD2 NAME(LightModeliv)(GLenum pname, const GLint *params)
{
   DISPATCH(LightModeliv, (pname, params), (F, "glLightModeliv(0x%x, %p);", pname, params));
}

KEYWORD1 void KEYWORD2 NAME(LineWidth)(GLfloat width)
{
   DISPATCH(LineWidth, (width), (F, "glLineWidth(%g);", width));
}

KEYWORD1 void KEYWORD2 NAME(LineStipple)(GLint factor, GLushort pattern)
{
   DISPATCH(LineStipple, (factor, pattern), (F, "glLineStipple(%d, 0x%x);", factor, pattern));
}

KEYWORD1 void KEYWORD2 NAME(ListBase)(GLuint base)
{
   DISPATCH(ListBase, (base), (F, "glListbase(%d);", base));
}

KEYWORD1 void KEYWORD2 NAME(LoadIdentity)(void)
{
   DISPATCH(LoadIdentity, (), (F, "glLoadIdentity();"));
}

KEYWORD1 void KEYWORD2 NAME(LoadMatrixd)(const GLdouble *m)
{
   DISPATCH(LoadMatrixd, (m), (F, "glLoadMatirxd(%p);", m));
}

KEYWORD1 void KEYWORD2 NAME(LoadMatrixf)(const GLfloat *m)
{
   DISPATCH(LoadMatrixf, (m), (F, "glLoadMatrixf(%p);", m));
}

KEYWORD1 void KEYWORD2 NAME(LoadName)(GLuint name)
{
   DISPATCH(LoadName, (name), (F, "glLoadName(%u);", name));
}

KEYWORD1 void KEYWORD2 NAME(LogicOp)(GLenum opcode)
{
   DISPATCH(LogicOp, (opcode), (F, "glLogicOp(0x%x);", opcode));
}

KEYWORD1 void KEYWORD2 NAME(Map1d)(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points)
{
   DISPATCH(Map1d, (target, u1, u2, stride, order, points), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(Map1f)(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points)
{
   DISPATCH(Map1f, (target, u1, u2, stride, order, points), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(Map2d)(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points)
{
   DISPATCH(Map2d, (target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(Map2f)(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points)
{
   DISPATCH(Map2f, (target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(MapGrid1d)(GLint un, GLdouble u1, GLdouble u2)
{
   DISPATCH(MapGrid1d, (un, u1, u2), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(MapGrid1f)(GLint un, GLfloat u1, GLfloat u2)
{
   DISPATCH(MapGrid1f, (un, u1, u2), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(MapGrid2d)(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2)
{
   DISPATCH(MapGrid2d, (un, u1, u2, vn, v1, v2), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(MapGrid2f)(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2)
{
   DISPATCH(MapGrid2f, (un, u1, u2, vn, v1, v2), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(MatrixMode)(GLenum mode)
{
   DISPATCH(MatrixMode, (mode), (F, "glMatrixMode(0x%x);", mode));
}

KEYWORD1 void KEYWORD2 NAME(MultMatrixd)(const GLdouble *m)
{
   DISPATCH(MultMatrixd, (m), (F, "glMultMatrixd(%p);", m));
}

KEYWORD1 void KEYWORD2 NAME(MultMatrixf)(const GLfloat *m)
{
   DISPATCH(MultMatrixf, (m), (F, "glMultMatrixf(%p);", m));
}

KEYWORD1 void KEYWORD2 NAME(NewList)(GLuint list, GLenum mode)
{
   DISPATCH(NewList, (list, mode), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(Ortho)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble nearval, GLdouble farval)
{
   DISPATCH(Ortho, (left, right, bottom, top, nearval, farval), (F, "glOrtho(%f, %f, %f, %f, %f, %f);", left, right, bottom, top, nearval, farval));
}

KEYWORD1 void KEYWORD2 NAME(PassThrough)(GLfloat token)
{
   DISPATCH(PassThrough, (token), (F, "glPassThrough(%f);", token));
}

KEYWORD1 void KEYWORD2 NAME(PixelMapfv)(GLenum map, GLint mapsize, const GLfloat *values)
{
   DISPATCH(PixelMapfv, (map, mapsize, values), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(PixelMapuiv)(GLenum map, GLint mapsize, const GLuint *values)
{
   DISPATCH(PixelMapuiv, (map, mapsize, values), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(PixelMapusv)(GLenum map, GLint mapsize, const GLushort *values)
{
   DISPATCH(PixelMapusv, (map, mapsize, values), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(PixelStoref)(GLenum pname, GLfloat param)
{
   DISPATCH(PixelStoref, (pname, param), (F, "glPixelStoref(0x%x, %f);", pname, param));
}

KEYWORD1 void KEYWORD2 NAME(PixelStorei)(GLenum pname, GLint param)
{
   DISPATCH(PixelStorei, (pname, param), (F, "glPixelStorei(0x%x, %d);", pname, param));
}

KEYWORD1 void KEYWORD2 NAME(PixelTransferf)(GLenum pname, GLfloat param)
{
   DISPATCH(PixelTransferf, (pname, param), (F, "glPixelTransferf(0x%x, %f);", pname, param));
}

KEYWORD1 void KEYWORD2 NAME(PixelTransferi)(GLenum pname, GLint param)
{
   DISPATCH(PixelTransferi, (pname, param), (F, "glPixelTransferi(0x%x, %d);", pname, param));
}

KEYWORD1 void KEYWORD2 NAME(PixelZoom)(GLfloat xfactor, GLfloat yfactor)
{
   DISPATCH(PixelZoom, (xfactor, yfactor), (F, "glPixelZoom(%f, %f);", xfactor, yfactor));
}

KEYWORD1 void KEYWORD2 NAME(PointSize)(GLfloat size)
{
   DISPATCH(PointSize, (size), (F, "glPointSize(%f);", size));
}

KEYWORD1 void KEYWORD2 NAME(PolygonMode)(GLenum face, GLenum mode)
{
   DISPATCH(PolygonMode, (face, mode), (F, "glPolygonMode(0x%x, 0x%x);", face, mode));
}

KEYWORD1 void KEYWORD2 NAME(PolygonStipple)(const GLubyte *pattern)
{
   DISPATCH(PolygonStipple, (pattern), (F, "glPolygonStipple(%p);", pattern));
}

KEYWORD1 void KEYWORD2 NAME(PopAttrib)(void)
{
   DISPATCH(PopAttrib, (), (F, "glPopAttrib();"));
}

KEYWORD1 void KEYWORD2 NAME(PopMatrix)(void)
{
   DISPATCH(PopMatrix, (), (F, "glPopMatrix();"));
}

KEYWORD1 void KEYWORD2 NAME(PopName)(void)
{
   DISPATCH(PopName, (), (F, "glPopName();"));
}

KEYWORD1 void KEYWORD2 NAME(PushAttrib)(GLbitfield mask)
{
   DISPATCH(PushAttrib, (mask), (F, "glPushAttrib(0x%x)", mask));
}

KEYWORD1 void KEYWORD2 NAME(PushMatrix)(void)
{
   DISPATCH(PushMatrix, (), (F, "glPushMatrix();"));
}

KEYWORD1 void KEYWORD2 NAME(PushName)(GLuint name)
{
   DISPATCH(PushName, (name), (F, "glPushName(%u)", name));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos2d)(GLdouble x, GLdouble y)
{
   DISPATCH(RasterPos2d, (x, y), (F, "glRasterPos2d(%g, %g);", x, y));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos2f)(GLfloat x, GLfloat y)
{
   DISPATCH(RasterPos2f, (x, y), (F, "glRasterPos2f(%g, %g);", x, y));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos2i)(GLint x, GLint y)
{
   DISPATCH(RasterPos2i, (x, y), (F, "glRasterPos2i(%d, %d);", x, y));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos2s)(GLshort x, GLshort y)
{
   DISPATCH(RasterPos2s, (x, y), (F, "glRasterPos2s(%d, %d);", x, y));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos3d)(GLdouble x, GLdouble y, GLdouble z)
{
   DISPATCH(RasterPos3d, (x, y, z), (F, "glRasterPos3d(%g, %g, %g);", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos3f)(GLfloat x, GLfloat y, GLfloat z)
{
   DISPATCH(RasterPos3f, (x, y, z), (F, "glRasterPos3f(%g, %g, %g);", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos3i)(GLint x, GLint y, GLint z)
{
   DISPATCH(RasterPos3i, (x, y, z), (F, "glRasterPos3i(%d, %d, %d);", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos3s)(GLshort x, GLshort y, GLshort z)
{
   DISPATCH(RasterPos3s, (x, y, z), (F, "glRasterPos3s(%d, %d, %d);", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos4d)(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   DISPATCH(RasterPos4d, (x, y, z, w), (F, "glRasterPos4d(%g, %g, %g, %g);", x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   DISPATCH(RasterPos4f, (x, y, z, w), (F, "glRasterPos4f(%g, %g, %g, %g);", x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos4i)(GLint x, GLint y, GLint z, GLint w)
{
   DISPATCH(RasterPos4i, (x, y, z, w), (F, "glRasterPos4i(%d, %d, %d, %d);", x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos4s)(GLshort x, GLshort y, GLshort z, GLshort w)
{
   DISPATCH(RasterPos4s, (x, y, z, w), (F, "glRasterPos4s(%d, %d, %d, %d);", x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos2dv)(const GLdouble *v)
{
   DISPATCH(RasterPos2dv, (v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos2fv)(const GLfloat *v)
{
   DISPATCH(RasterPos2fv, (v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos2iv)(const GLint *v)
{
   DISPATCH(RasterPos2iv, (v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos2sv)(const GLshort *v)
{
   DISPATCH(RasterPos2sv, (v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos3dv)(const GLdouble *v)
{
   DISPATCH(RasterPos3dv, (v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos3fv)(const GLfloat *v)
{
   DISPATCH(RasterPos3fv, (v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos3iv)(const GLint *v)
{
   DISPATCH(RasterPos3iv, (v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos3sv)(const GLshort *v)
{
   DISPATCH(RasterPos3sv, (v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos4dv)(const GLdouble *v)
{
   DISPATCH(RasterPos4dv, (v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos4fv)(const GLfloat *v)
{
   DISPATCH(RasterPos4fv, (v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos4iv)(const GLint *v)
{
   DISPATCH(RasterPos4iv, (v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos4sv)(const GLshort *v)
{
   DISPATCH(RasterPos4sv, (v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(ReadBuffer)(GLenum mode)
{
   DISPATCH(ReadBuffer, (mode), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(ReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels)
{
   DISPATCH(ReadPixels, (x, y, width, height, format, type, pixels), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(Rectd)(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2)
{
   DISPATCH(Rectd, (x1, y1, x2, y2), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(Rectdv)(const GLdouble *v1, const GLdouble *v2)
{
   DISPATCH(Rectdv, (v1, v2), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(Rectf)(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
   DISPATCH(Rectf, (x1, y1, x2, y2), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(Rectfv)(const GLfloat *v1, const GLfloat *v2)
{
   DISPATCH(Rectfv, (v1, v2), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(Recti)(GLint x1, GLint y1, GLint x2, GLint y2)
{
   DISPATCH(Recti, (x1, y1, x2, y2), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(Rectiv)(const GLint *v1, const GLint *v2)
{
   DISPATCH(Rectiv, (v1, v2), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(Rects)(GLshort x1, GLshort y1, GLshort x2, GLshort y2)
{
   DISPATCH(Rects, (x1, y1, x2, y2), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(Rectsv)(const GLshort *v1, const GLshort *v2)
{
   DISPATCH(Rectsv, (v1, v2), (F, ";"));
}

KEYWORD1 GLint KEYWORD2 NAME(RenderMode)(GLenum mode)
{
   RETURN_DISPATCH(RenderMode, (mode), (F, "glRenderMode(0x%x);", mode));
}

KEYWORD1 void KEYWORD2 NAME(Rotated)(GLdouble angle, GLdouble x, GLdouble y, GLdouble z)
{
   DISPATCH(Rotated, (angle, x, y, z), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(Rotatef)(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
   DISPATCH(Rotatef, (angle, x, y, z), (F, "glRotatef(%g, %g, %g, %g);", angle, x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(SelectBuffer)(GLsizei size, GLuint *buffer)
{
   DISPATCH(SelectBuffer, (size, buffer), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(Scaled)(GLdouble x, GLdouble y, GLdouble z)
{
   DISPATCH(Scaled, (x, y, z), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(Scalef)(GLfloat x, GLfloat y, GLfloat z)
{
   DISPATCH(Scalef, (x, y, z), (F, "glScalef(%g, %g, %g);", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(Scissor)(GLint x, GLint y, GLsizei width, GLsizei height)
{
   DISPATCH(Scissor, (x, y, width, height), (F, "glScissor(%d, %d, %d, %d);", x, y, width, height));
}

KEYWORD1 void KEYWORD2 NAME(ShadeModel)(GLenum mode)
{
   DISPATCH(ShadeModel, (mode), (F, "glShadeModel(0x%x);", mode));
}

KEYWORD1 void KEYWORD2 NAME(StencilFunc)(GLenum func, GLint ref, GLuint mask)
{
   DISPATCH(StencilFunc, (func, ref, mask), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(StencilMask)(GLuint mask)
{
   DISPATCH(StencilMask, (mask), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(StencilOp)(GLenum fail, GLenum zfail, GLenum zpass)
{
   DISPATCH(StencilOp, (fail, zfail, zpass), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexGend)(GLenum coord, GLenum pname, GLdouble param)
{
   DISPATCH(TexGend, (coord, pname, param), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexGendv)(GLenum coord, GLenum pname, const GLdouble *params)
{
   DISPATCH(TexGendv, (coord, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexGenf)(GLenum coord, GLenum pname, GLfloat param)
{
   DISPATCH(TexGenf, (coord, pname, param), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexGenfv)(GLenum coord, GLenum pname, const GLfloat *params)
{
   DISPATCH(TexGenfv, (coord, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexGeni)(GLenum coord, GLenum pname, GLint param)
{
   DISPATCH(TexGeni, (coord, pname, param), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexGeniv)(GLenum coord, GLenum pname, const GLint *params)
{
   DISPATCH(TexGeniv, (coord, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexEnvf)(GLenum target, GLenum pname, GLfloat param)
{
   DISPATCH(TexEnvf, (target, pname, param), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexEnvfv)(GLenum target, GLenum pname, const GLfloat *param)
{
   DISPATCH(TexEnvfv, (target, pname, param), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexEnvi)(GLenum target, GLenum pname, GLint param)
{
   DISPATCH(TexEnvi, (target, pname, param), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexEnviv)(GLenum target, GLenum pname, const GLint *param)
{
   DISPATCH(TexEnviv, (target, pname, param), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexImage1D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
   DISPATCH(TexImage1D, (target, level, internalformat, width, border, format, type, pixels), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
   DISPATCH(TexImage2D, (target, level, internalformat, width, height, border, format, type, pixels), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexParameterf)(GLenum target, GLenum pname, GLfloat param)
{
   DISPATCH(TexParameterf, (target, pname, param), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexParameterfv)(GLenum target, GLenum pname, const GLfloat *params)
{
   DISPATCH(TexParameterfv, (target, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexParameteri)(GLenum target, GLenum pname, GLint param)
{
   DISPATCH(TexParameteri, (target, pname, param), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexParameteriv)(GLenum target, GLenum pname, const GLint *params)
{
   DISPATCH(TexParameteriv, (target, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(Translated)(GLdouble x, GLdouble y, GLdouble z)
{
   DISPATCH(Translated, (x, y, z), (F, "glTranslated(%g, %g, %g);", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(Translatef)(GLfloat x, GLfloat y, GLfloat z)
{
   DISPATCH(Translatef, (x, y, z), (F, "glTranslatef(%g, %g, %g);", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(Viewport)(GLint x, GLint y, GLsizei width, GLsizei height)
{
   DISPATCH(Viewport, (x, y, width, height), (F, "glViewport(%d, %d, %d, %d);", x, y, width, height));
}



/* GL 1.1 */

KEYWORD1 GLboolean KEYWORD2 NAME(AreTexturesResident)(GLsizei n, const GLuint *textures, GLboolean *residences)
{
   RETURN_DISPATCH(AreTexturesResident, (n, textures, residences), (F, "glAreTexturesResident(%d, %p, %p);", n, textures, residences));
}

KEYWORD1 void KEYWORD2 NAME(ArrayElement)(GLint i)
{
   DISPATCH(ArrayElement, (i), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(BindTexture)(GLenum target, GLuint texture)
{
   DISPATCH(BindTexture, (target, texture), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(ColorPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr)
{
   DISPATCH(ColorPointer, (size, type, stride, ptr), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(CopyTexImage1D)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border)
{
   DISPATCH(CopyTexImage1D, (target, level, internalformat, x, y, width, border), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(CopyTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
   DISPATCH(CopyTexImage2D, (target, level, internalformat, x, y, width, height, border), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(CopyTexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)
{
   DISPATCH(CopyTexSubImage1D, (target, level, xoffset, x, y, width), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(CopyTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
   DISPATCH(CopyTexSubImage2D, (target, level, xoffset, yoffset, x, y, width, height), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(DeleteTextures)(GLsizei n, const GLuint *textures)
{
   DISPATCH(DeleteTextures, (n, textures), (F, "glDeleteTextures(%d, %p);", n, textures));
}

KEYWORD1 void KEYWORD2 NAME(DisableClientState)(GLenum cap)
{
   DISPATCH(DisableClientState, (cap), (F, "glDisableClientState(0x%x);", cap));
}

KEYWORD1 void KEYWORD2 NAME(DrawArrays)(GLenum mode, GLint first, GLsizei count)
{
   DISPATCH(DrawArrays, (mode, first, count), (F, "glDrawArrays(0x%x, %d, %d);", mode, first, count));
}

KEYWORD1 void KEYWORD2 NAME(EdgeFlagPointer)(GLsizei stride, const GLvoid *ptr)
{
   DISPATCH(EdgeFlagPointer, (stride, ptr), (F, "glEdgeFlagPointer(%d, %p);", stride, ptr));
}

KEYWORD1 void KEYWORD2 NAME(EnableClientState)(GLenum cap)
{
   DISPATCH(EnableClientState, (cap), (F, "glEnableClientState(0x%x)", cap));
}

KEYWORD1 void KEYWORD2 NAME(GenTextures)(GLsizei n, GLuint *textures)
{
   DISPATCH(GenTextures, (n, textures), (F, "glGenTextures(%d, %p);", n, textures));
}

KEYWORD1 void KEYWORD2 NAME(GetPointerv)(GLenum pname, GLvoid **params)
{
   DISPATCH(GetPointerv, (pname, params), (F, "glGetPointerv(0x%x, %p);", pname, params));
}

KEYWORD1 void KEYWORD2 NAME(IndexPointer)(GLenum type, GLsizei stride, const GLvoid *ptr)
{
   DISPATCH(IndexPointer, (type, stride, ptr), (F, "glIndexPointer(0x%x, %d, %p);", type, stride, ptr));
}

KEYWORD1 void KEYWORD2 NAME(InterleavedArrays)(GLenum format, GLsizei stride, const GLvoid *pointer)
{
   DISPATCH(InterleavedArrays, (format, stride, pointer), (F, "glInterleavedArrays(0x%x, %d, %p);", format, stride, pointer));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsTexture)(GLuint texture)
{
   RETURN_DISPATCH(IsTexture, (texture), (F, "glIsTexture(%u);", texture));
}

KEYWORD1 void KEYWORD2 NAME(NormalPointer)(GLenum type, GLsizei stride, const GLvoid *ptr)
{
   DISPATCH(NormalPointer, (type, stride, ptr), (F, "glNormalPointer(0x%x, %d, %p);", type, stride, ptr));
}

KEYWORD1 void KEYWORD2 NAME(PolygonOffset)(GLfloat factor, GLfloat units)
{
   DISPATCH(PolygonOffset, (factor, units), (F, "glPolygonOffset(%g, %g);", factor, units));
}

KEYWORD1 void KEYWORD2 NAME(PopClientAttrib)(void)
{
   DISPATCH(PopClientAttrib, (), (F, "glPopClientAttrib();"));
}

KEYWORD1 void KEYWORD2 NAME(PrioritizeTextures)(GLsizei n, const GLuint *textures, const GLclampf *priorities)
{
   DISPATCH(PrioritizeTextures, (n, textures, priorities), (F, "glPrioritizeTextures(%d, %p, %p);", n, textures, priorities));
}

KEYWORD1 void KEYWORD2 NAME(PushClientAttrib)(GLbitfield mask)
{
   DISPATCH(PushClientAttrib, (mask), (F, "glPushClientAttrib(0x%x)", mask));
}

KEYWORD1 void KEYWORD2 NAME(TexCoordPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr)
{
   DISPATCH(TexCoordPointer, (size, type, stride, ptr), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels)
{
   DISPATCH(TexSubImage1D, (target, level, xoffset, width, format, type, pixels), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
   DISPATCH(TexSubImage2D, (target, level, xoffset, yoffset, width, height, format, type, pixels), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(VertexPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr)
{
   DISPATCH(VertexPointer, (size, type, stride, ptr), (F, ";"));
}


/* GL 1.2 */

KEYWORD1 void KEYWORD2 NAME(CopyTexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
   DISPATCH(CopyTexSubImage3D, (target, level, xoffset, yoffset, zoffset, x, y, width, height), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(DrawRangeElements)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices)
{
   DISPATCH(DrawRangeElements, (mode, start, end, count, type, indices), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexImage3D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
   DISPATCH(TexImage3D, (target, level, internalformat, width, height, depth, border, format, type, pixels), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels)
{
   DISPATCH(TexSubImage3D, (target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels), (F, ";"));
}

/* GL_ARB_imaging */

KEYWORD1 void KEYWORD2 NAME(BlendColor)(GLclampf r, GLclampf g, GLclampf b, GLclampf a)
{
   DISPATCH(BlendColor, (r, g, b, a), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(BlendEquation)(GLenum mode)
{
   DISPATCH(BlendEquation, (mode), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(ColorSubTable)(GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid *data)
{
   DISPATCH(ColorSubTable, (target, start, count, format, type, data), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(ColorTable)(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *table)
{
   DISPATCH(ColorTable, (target, internalformat, width, format, type, table), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(ColorTableParameterfv)(GLenum target, GLenum pname, const GLfloat *params)
{
   DISPATCH(ColorTableParameterfv, (target, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(ColorTableParameteriv)(GLenum target, GLenum pname, const GLint *params)
{
   DISPATCH(ColorTableParameteriv, (target, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(ConvolutionFilter1D)(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *image)
{
   DISPATCH(ConvolutionFilter1D, (target, internalformat, width, format, type, image), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(ConvolutionFilter2D)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *image)
{
   DISPATCH(ConvolutionFilter2D, (target, internalformat, width, height, format, type, image), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(ConvolutionParameterf)(GLenum target, GLenum pname, GLfloat params)
{
   DISPATCH(ConvolutionParameterf, (target, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(ConvolutionParameterfv)(GLenum target, GLenum pname, const GLfloat *params)
{
   DISPATCH(ConvolutionParameterfv, (target, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(ConvolutionParameteri)(GLenum target, GLenum pname, GLint params)
{
   DISPATCH(ConvolutionParameteri, (target, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(ConvolutionParameteriv)(GLenum target, GLenum pname, const GLint *params)
{
   DISPATCH(ConvolutionParameteriv, (target, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(CopyColorSubTable)(GLenum target, GLsizei start, GLint x, GLint y, GLsizei width)
{
   DISPATCH(CopyColorSubTable, (target, start, x, y, width), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(CopyColorTable)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width)
{
   DISPATCH(CopyColorTable, (target, internalformat, x, y, width), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(CopyConvolutionFilter1D)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width)
{
   DISPATCH(CopyConvolutionFilter1D, (target, internalformat, x, y, width), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(CopyConvolutionFilter2D)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height)
{
   DISPATCH(CopyConvolutionFilter2D, (target, internalformat, x, y, width, height), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetColorTable)(GLenum target, GLenum format, GLenum type, GLvoid *table)
{
   DISPATCH(GetColorTable, (target, format, type, table), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetColorTableParameterfv)(GLenum target, GLenum pname, GLfloat *params)
{
   DISPATCH(GetColorTableParameterfv, (target, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetColorTableParameteriv)(GLenum target, GLenum pname, GLint *params)
{
   DISPATCH(GetColorTableParameteriv, (target, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetConvolutionFilter)(GLenum target, GLenum format, GLenum type, GLvoid *image)
{
   DISPATCH(GetConvolutionFilter, (target, format, type, image), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetConvolutionParameterfv)(GLenum target, GLenum pname, GLfloat *params)
{
   DISPATCH(GetConvolutionParameterfv, (target, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetConvolutionParameteriv)(GLenum target, GLenum pname, GLint *params)
{
   DISPATCH(GetConvolutionParameteriv, (target, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetHistogram)(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values)
{
   DISPATCH(GetHistogram, (target, reset, format, type, values), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetHistogramParameterfv)(GLenum target, GLenum pname, GLfloat *params)
{
   DISPATCH(GetHistogramParameterfv, (target, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetHistogramParameteriv)(GLenum target, GLenum pname, GLint *params)
{
   DISPATCH(GetHistogramParameteriv, (target, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetMinmax)(GLenum target, GLboolean reset, GLenum format, GLenum types, GLvoid *values)
{
   DISPATCH(GetMinmax, (target, reset, format, types, values), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetMinmaxParameterfv)(GLenum target, GLenum pname, GLfloat *params)
{
   DISPATCH(GetMinmaxParameterfv, (target, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetMinmaxParameteriv)(GLenum target, GLenum pname, GLint *params)
{
   DISPATCH(GetMinmaxParameteriv, (target, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetSeparableFilter)(GLenum target, GLenum format, GLenum type, GLvoid *row, GLvoid *column, GLvoid *span)
{
   DISPATCH(GetSeparableFilter, (target, format, type, row, column, span), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(Histogram)(GLenum target, GLsizei width, GLenum internalformat, GLboolean sink)
{
   DISPATCH(Histogram, (target, width, internalformat, sink), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(Minmax)(GLenum target, GLenum internalformat, GLboolean sink)
{
   DISPATCH(Minmax, (target, internalformat, sink), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(ResetMinmax)(GLenum target)
{
   DISPATCH(ResetMinmax, (target), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(ResetHistogram)(GLenum target)
{
   DISPATCH(ResetHistogram, (target), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(SeparableFilter2D)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *row, const GLvoid *column)
{
   DISPATCH(SeparableFilter2D, (target, internalformat, width, height, format, type, row, column), (F, ";"));
}

/***
 *** Extension functions
 ***/

/* 2. GL_EXT_blend_color */
KEYWORD1 void KEYWORD2 NAME(BlendColorEXT)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
   DISPATCH(BlendColor, (red, green, blue, alpha), (F, ";"));
}

/* 3. GL_EXT_polygon_offset */
KEYWORD1 void KEYWORD2 NAME(PolygonOffsetEXT)(GLfloat factor, GLfloat bias)
{
   DISPATCH(PolygonOffsetEXT, (factor, bias), (F, ";"));
}

/* 6. GL_EXT_texture3D */

KEYWORD1 void KEYWORD2 NAME(CopyTexSubImage3DEXT)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
   DISPATCH(CopyTexSubImage3D, (target, level, xoffset, yoffset, zoffset, x, y, width, height), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexImage3DEXT)(GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
   DISPATCH(TexImage3D, (target, level, internalFormat, width, height, depth, border, format, type, pixels), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexSubImage3DEXT)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels)
{
   DISPATCH(TexSubImage3D, (target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels), (F, ";"));
}

/* 7. GL_SGI_texture_filter4 */

KEYWORD1 void KEYWORD2 NAME(GetTexFilterFuncSGIS)(GLenum target, GLenum filter, GLfloat *weights)
{
   DISPATCH(GetTexFilterFuncSGIS, (target, filter, weights), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexFilterFuncSGIS)(GLenum target, GLenum filter, GLsizei n, const GLfloat *weights)
{
   DISPATCH(TexFilterFuncSGIS, (target, filter, n, weights), (F, ";"));
}

/* 9. GL_EXT_subtexture */

KEYWORD1 void KEYWORD2 NAME(CopyTexSubImage1DEXT)(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)
{
   DISPATCH(CopyTexSubImage1D, (target, level, xoffset, x, y, width), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexSubImage1DEXT)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels)
{
   DISPATCH(TexSubImage1D, (target, level, xoffset, width, format, type, pixels), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexSubImage2DEXT)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
   DISPATCH(TexSubImage2D, (target, level, xoffset, yoffset, width, height, format, type, pixels), (F, ";"));
}

/* 10. GL_EXT_copy_texture */

KEYWORD1 void KEYWORD2 NAME(CopyTexImage1DEXT)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border)
{
   DISPATCH(CopyTexImage1D, (target, level, internalformat, x, y, width, border), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(CopyTexImage2DEXT)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
   DISPATCH(CopyTexImage2D, (target, level, internalformat, x, y, width, height, border), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(CopyTexSubImage2DEXT)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
   DISPATCH(CopyTexSubImage2D, (target, level, xoffset, yoffset, x, y, width, height), (F, ";"));
}

/* 11. GL_EXT_histogram */
KEYWORD1 void KEYWORD2 NAME(GetHistogramEXT)(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values)
{
   DISPATCH(GetHistogramEXT, (target, reset, format, type, values), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetHistogramParameterfvEXT)(GLenum target, GLenum pname, GLfloat *params)
{
   DISPATCH(GetHistogramParameterfvEXT, (target, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetHistogramParameterivEXT)(GLenum target, GLenum pname, GLint *params)
{
   DISPATCH(GetHistogramParameterivEXT, (target, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetMinmaxEXT)(GLenum target, GLboolean reset, GLenum format, GLenum types, GLvoid *values)
{
   DISPATCH(GetMinmaxEXT, (target, reset, format, types, values), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetMinmaxParameterfvEXT)(GLenum target, GLenum pname, GLfloat *params)
{
   DISPATCH(GetMinmaxParameterfvEXT, (target, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetMinmaxParameterivEXT)(GLenum target, GLenum pname, GLint *params)
{
   DISPATCH(GetMinmaxParameterivEXT, (target, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(HistogramEXT)(GLenum target, GLsizei width, GLenum internalformat, GLboolean sink)
{
   DISPATCH(Histogram, (target, width, internalformat, sink), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(MinmaxEXT)(GLenum target, GLenum internalformat, GLboolean sink)
{
   DISPATCH(Minmax, (target, internalformat, sink), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(ResetHistogramEXT)(GLenum target)
{
   DISPATCH(ResetHistogram, (target), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(ResetMinmaxEXT)(GLenum target)
{
   DISPATCH(ResetMinmax, (target), (F, ";"));
}

/* 12. GL_EXT_convolution */

KEYWORD1 void KEYWORD2 NAME(ConvolutionFilter1DEXT)(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *image)
{
   DISPATCH(ConvolutionFilter1D, (target, internalformat, width, format, type, image), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(ConvolutionFilter2DEXT)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *image)
{
   DISPATCH(ConvolutionFilter2D, (target, internalformat, width, height, format, type, image), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(ConvolutionParameterfEXT)(GLenum target, GLenum pname, GLfloat params)
{
   DISPATCH(ConvolutionParameterf, (target, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(ConvolutionParameterfvEXT)(GLenum target, GLenum pname, const GLfloat *params)
{
   DISPATCH(ConvolutionParameterfv, (target, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(ConvolutionParameteriEXT)(GLenum target, GLenum pname, GLint params)
{
   DISPATCH(ConvolutionParameteri, (target, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(ConvolutionParameterivEXT)(GLenum target, GLenum pname, const GLint *params)
{
   DISPATCH(ConvolutionParameteriv, (target, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(CopyConvolutionFilter1DEXT)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width)
{
   DISPATCH(CopyConvolutionFilter1D, (target, internalformat, x, y, width), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(CopyConvolutionFilter2DEXT)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height)
{
   DISPATCH(CopyConvolutionFilter2D, (target, internalformat, x, y, width, height), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetConvolutionFilterEXT)(GLenum target, GLenum format, GLenum type, GLvoid *image)
{
   DISPATCH(GetConvolutionFilterEXT, (target, format, type, image), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetConvolutionParameterfvEXT)(GLenum target, GLenum pname, GLfloat *params)
{
   DISPATCH(GetConvolutionParameterfvEXT, (target, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetConvolutionParameterivEXT)(GLenum target, GLenum pname, GLint *params)
{
   DISPATCH(GetConvolutionParameterivEXT, (target, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetSeparableFilterEXT)(GLenum target, GLenum format, GLenum type, GLvoid *row, GLvoid *column, GLvoid *span)
{
   DISPATCH(GetSeparableFilterEXT, (target, format, type, row, column, span), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(SeparableFilter2DEXT)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *row, const GLvoid *column)
{
   DISPATCH(SeparableFilter2D, (target, internalformat, width, height, format, type, row, column), (F, ";"));
}

/* 14. GL_SGI_color_table */

KEYWORD1 void KEYWORD2 NAME(ColorTableParameterfvSGI)(GLenum target, GLenum pname, const GLfloat *params)
{
   DISPATCH(ColorTableParameterfv, (target, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(ColorTableParameterivSGI)(GLenum target, GLenum pname, const GLint *params)
{
   DISPATCH(ColorTableParameteriv, (target, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(ColorTableSGI)(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *table)
{
   DISPATCH(ColorTable, (target, internalformat, width, format, type, table), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(CopyColorTableSGI)(GLenum target, GLenum internalFormat, GLint x, GLint y, GLsizei width)
{
   DISPATCH(CopyColorTable, (target, internalFormat, x, y, width), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetColorTableSGI)(GLenum target, GLenum format, GLenum type, GLvoid *table)
{
   DISPATCH(GetColorTableSGI, (target, format, type, table), (F, ";"));
}
KEYWORD1 void KEYWORD2 NAME(GetColorTableParameterfvSGI)(GLenum target, GLenum pname, GLfloat *params)
{
   DISPATCH(GetColorTableParameterfvSGI, (target, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetColorTableParameterivSGI)(GLenum target, GLenum pname, GLint *params)
{
   DISPATCH(GetColorTableParameterivSGI, (target, pname, params), (F, ";"));
}

/* ??. GL_SGIX_pixel_texture */

KEYWORD1 void KEYWORD2 NAME(PixelTexGenSGIX)(GLenum mode)
{
   DISPATCH(PixelTexGenSGIX, (mode), (F, ";"));
}

/* 15. GL_SGIS_pixel_texture */

KEYWORD1 void KEYWORD2 NAME(PixelTexGenParameterfSGIS)(GLenum target, GLfloat value)
{
   DISPATCH(PixelTexGenParameterfSGIS, (target, value), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(PixelTexGenParameterfvSGIS)(GLenum target, const GLfloat *value)
{
   DISPATCH(PixelTexGenParameterfvSGIS, (target, value), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(PixelTexGenParameteriSGIS)(GLenum target, GLint value)
{
   DISPATCH(PixelTexGenParameteriSGIS, (target, value), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(PixelTexGenParameterivSGIS)(GLenum target, const GLint *value)
{
   DISPATCH(PixelTexGenParameterivSGIS, (target, value), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetPixelTexGenParameterfvSGIS)(GLenum target, GLfloat *value)
{
   DISPATCH(GetPixelTexGenParameterfvSGIS, (target, value), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetPixelTexGenParameterivSGIS)(GLenum target, GLint *value)
{
   DISPATCH(GetPixelTexGenParameterivSGIS, (target, value), (F, ";"));
}

/* 16. GL_SGIS_texture4D */

KEYWORD1 void KEYWORD2 NAME(TexImage4DSGIS)(GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLsizei extent, GLint border, GLenum format, GLenum type, const void *pixels)
{
   DISPATCH(TexImage4DSGIS, (target, level, internalFormat, width, height, depth, extent, border, format, type, pixels), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexSubImage4DSGIS)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint woffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei extent, GLenum format, GLenum type, const void *pixels)
{
   DISPATCH(TexSubImage4DSGIS, (target, level, xoffset, yoffset, zoffset, woffset, width, height, depth, extent, format, type, pixels), (F, ";"));
}

/* 20. GL_EXT_texture_object */

KEYWORD1 void KEYWORD2 NAME(GenTexturesEXT)(GLsizei n, GLuint *textures)
{
   DISPATCH(GenTextures, (n, textures), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(DeleteTexturesEXT)(GLsizei n, const GLuint *texture)
{
   DISPATCH(DeleteTextures, (n, texture), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(BindTextureEXT)(GLenum target, GLuint texture)
{
   DISPATCH(BindTexture, (target, texture), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(PrioritizeTexturesEXT)(GLsizei n, const GLuint *textures, const GLclampf *priorities)
{
   DISPATCH(PrioritizeTextures, (n, textures, priorities), (F, ";"));
}

KEYWORD1 GLboolean KEYWORD2 NAME(AreTexturesResidentEXT)(GLsizei n, const GLuint *textures, GLboolean *residences)
{
   RETURN_DISPATCH(AreTexturesResident, (n, textures, residences), (F, "glAreTexturesResidentEXT(%d %p %p);", n, textures, residences));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsTextureEXT)(GLuint texture)
{
   RETURN_DISPATCH(IsTexture, (texture), (F, "glIsTextureEXT(%u);", texture));
}

/* 21. GL_SGIS_detail_texture */

KEYWORD1 void KEYWORD2 NAME(DetailTexFuncSGIS)(GLenum target, GLsizei n, const GLfloat *points)
{
   DISPATCH(DetailTexFuncSGIS, (target, n, points), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetDetailTexFuncSGIS)(GLenum target, GLfloat *points)
{
   DISPATCH(GetDetailTexFuncSGIS, (target, points), (F, ";"));
}

/* 22. GL_SGIS_sharpen_texture */

KEYWORD1 void KEYWORD2 NAME(GetSharpenTexFuncSGIS)(GLenum target, GLfloat *points)
{
   DISPATCH(GetSharpenTexFuncSGIS, (target, points), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(SharpenTexFuncSGIS)(GLenum target, GLsizei n, const GLfloat *points)
{
   DISPATCH(SharpenTexFuncSGIS, (target, n, points), (F, ";"));
}

/* 25. GL_SGIS_multisample */

KEYWORD1 void KEYWORD2 NAME(SampleMaskSGIS)(GLclampf value, GLboolean invert)
{
   DISPATCH(SampleMaskSGIS, (value, invert), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(SamplePatternSGIS)(GLenum pattern)
{
   DISPATCH(SamplePatternSGIS, (pattern), (F, ";"));
}

/* 30. GL_EXT_vertex_array */

KEYWORD1 void KEYWORD2 NAME(VertexPointerEXT)(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *ptr)
{
   DISPATCH(VertexPointerEXT, (size, type, stride, count, ptr), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(NormalPointerEXT)(GLenum type, GLsizei stride, GLsizei count, const GLvoid *ptr)
{
   DISPATCH(NormalPointerEXT, (type, stride, count, ptr), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(ColorPointerEXT)(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *ptr)
{
   DISPATCH(ColorPointerEXT, (size, type, stride, count, ptr), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(IndexPointerEXT)(GLenum type, GLsizei stride, GLsizei count, const GLvoid *ptr)
{
   DISPATCH(IndexPointerEXT, (type, stride, count, ptr), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoordPointerEXT)(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *ptr)
{
   DISPATCH(ColorPointerEXT, (size, type, stride, count, ptr), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(EdgeFlagPointerEXT)(GLsizei stride, GLsizei count, const GLboolean *ptr)
{
   DISPATCH(EdgeFlagPointerEXT, (stride, count, ptr), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetPointervEXT)(GLenum pname, void **params)
{
   DISPATCH(GetPointerv, (pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(DrawArraysEXT)(GLenum mode, GLint first, GLsizei count)
{
   DISPATCH(DrawArrays, (mode, first, count), (F, ";"));
}

/* 37. GL_EXT_blend_minmax */
KEYWORD1 void KEYWORD2 NAME(BlendEquationEXT)(GLenum mode)
{
   DISPATCH(BlendEquation, (mode), (F, "glBlendEquationEXT(0x%x);", mode));
}

/* 52. GL_SGIX_sprite */

KEYWORD1 void KEYWORD2 NAME(SpriteParameterfSGIX)(GLenum pname, GLfloat param)
{
   DISPATCH(SpriteParameterfSGIX, (pname, param), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(SpriteParameterfvSGIX)(GLenum pname, const GLfloat *param)
{
   DISPATCH(SpriteParameterfvSGIX, (pname, param), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(SpriteParameteriSGIX)(GLenum pname, GLint param)
{
   DISPATCH(SpriteParameteriSGIX, (pname, param), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(SpriteParameterivSGIX)(GLenum pname, const GLint *param)
{
   DISPATCH(SpriteParameterivSGIX, (pname, param), (F, ";"));
}

/* 54. GL_EXT_point_parameters */

KEYWORD1 void KEYWORD2 NAME(PointParameterfEXT)(GLenum target, GLfloat param)
{
   DISPATCH(PointParameterfEXT, (target, param), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(PointParameterfvEXT)(GLenum target, const GLfloat *param)
{
   DISPATCH(PointParameterfvEXT, (target, param), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(PointParameterfSGIS)(GLenum target, GLfloat param)
{
   DISPATCH(PointParameterfEXT, (target, param), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(PointParameterfvSGIS)(GLenum target, const GLfloat *param)
{
   DISPATCH(PointParameterfvEXT, (target, param), (F, ";"));
}

/* 55. GL_SGIX_instruments */
KEYWORD1 GLint KEYWORD2 NAME(GetInstrumentsSGIX)(void)
{
   RETURN_DISPATCH(GetInstrumentsSGIX, (), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(InstrumentsBufferSGIX)(GLsizei size, GLint *buf)
{
   DISPATCH(InstrumentsBufferSGIX, (size, buf), (F, ";"));
}

KEYWORD1 GLint KEYWORD2 NAME(PollInstrumentsSGIX)(GLint *markerp)
{
   RETURN_DISPATCH(PollInstrumentsSGIX, (markerp), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(ReadInstrumentsSGIX)(GLint marker)
{
   DISPATCH(ReadInstrumentsSGIX, (marker), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(StartInstrumentsSGIX)(void)
{
   DISPATCH(StartInstrumentsSGIX, (), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(StopInstrumentsSGIX)(GLint marker)
{
   DISPATCH(StopInstrumentsSGIX, (marker), (F, ";"));
}

/* 57. GL_SGIX_framezoom */
KEYWORD1 void KEYWORD2 NAME(FrameZoomSGIX)(GLint factor)
{
   DISPATCH(FrameZoomSGIX, (factor), (F, ";"));
}

/* 58. GL_SGIX_tag_sample_buffer */
KEYWORD1 void KEYWORD2 NAME(TagSampleBufferSGIX)(void)
{
   DISPATCH(TagSampleBufferSGIX, (), (F, ";"));
}

/* 60. GL_SGIX_reference_plane */
KEYWORD1 void KEYWORD2 NAME(ReferencePlaneSGIX)(const GLdouble *plane)
{
   DISPATCH(ReferencePlaneSGIX, (plane), (F, ";"));
}

/* 61. GL_SGIX_flush_raster */
KEYWORD1 void KEYWORD2 NAME(FlushRasterSGIX)(void)
{
   DISPATCH(FlushRasterSGIX, (), (F, ";"));
}

/* 66. GL_HP_image_transform */
#if 00
KEYWORD1 void KEYWORD2 NAME(GetImageTransformParameterfvHP)(GLenum target, GLenum pname, GLfloat *param)
{
   DISPATCH(GetImageTransformParameterfvHP, (target, pname, param), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetImageTransformParameterivHP)(GLenum target, GLenum pname, GLint *param)
{
   DISPATCH(GetImageTransformParameterivHP, (target, pname, param), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(ImageTransformParameterfHP)(GLenum target, GLenum pname, const GLfloat param)
{
   DISPATCH(ImageTransformParameterfHP, (target, pname, param), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(ImageTransformParameterfvHP)(GLenum target, GLenum pname, const GLfloat *param)
{
   DISPATCH(ImageTransformParameterfvHP, (target, pname, param), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(ImageTransformParameteriHP)(GLenum target, GLenum pname, const GLint param)
{
   DISPATCH(ImageTransformParameteriHP, (target, pname, param), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(ImageTransformParameterivHP)(GLenum target, GLenum pname, const GLint *param)
{
   DISPATCH(ImageTransformParameterivHP, (target, pname, param), (F, ";"));
}
#endif

/* 74. GL_EXT_color_subtable */
KEYWORD1 void KEYWORD2 NAME(ColorSubTableEXT)(GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const void *data)
{
   DISPATCH(ColorSubTable, (target, start, count, format, type, data), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(CopyColorSubTableEXT)(GLenum target, GLsizei start, GLint x, GLint y, GLsizei width)
{
   DISPATCH(CopyColorSubTable, (target, start, x, y, width), (F, ";"));
}

/* 77. GL_PGI_misc_hints */
KEYWORD1 void KEYWORD2 NAME(HintPGI)(GLenum target, GLint mode)
{
   DISPATCH(HintPGI, (target, mode), (F, ";"));
}

/* 78. GL_EXT_paletted_texture */

KEYWORD1 void KEYWORD2 NAME(ColorTableEXT)(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *table)
{
   DISPATCH(ColorTable, (target, internalformat, width, format, type, table), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetColorTableEXT)(GLenum target, GLenum format, GLenum type, GLvoid *table)
{
   DISPATCH(GetColorTableEXT, (target, format, type, table), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetColorTableParameterfvEXT)(GLenum target, GLenum pname, GLfloat *params)
{
   DISPATCH(GetColorTableParameterfvEXT, (target, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetColorTableParameterivEXT)(GLenum target, GLenum pname, GLint *params)
{
   DISPATCH(GetColorTableParameterivEXT, (target, pname, params), (F, ";"));
}

/* 80. GL_SGIX_list_priority */

KEYWORD1 void KEYWORD2 NAME(GetListParameterfvSGIX)(GLuint list, GLenum name, GLfloat *param)
{
   DISPATCH(GetListParameterfvSGIX, (list, name, param), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetListParameterivSGIX)(GLuint list, GLenum name, GLint *param)
{
   DISPATCH(GetListParameterivSGIX, (list, name, param), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(ListParameterfSGIX)(GLuint list, GLenum name, GLfloat param)
{
   DISPATCH(ListParameterfSGIX, (list, name, param), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(ListParameterfvSGIX)(GLuint list, GLenum name, const GLfloat *param)
{
   DISPATCH(ListParameterfvSGIX, (list, name, param), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(ListParameteriSGIX)(GLuint list, GLenum name, GLint param)
{
   DISPATCH(ListParameteriSGIX, (list, name, param), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(ListParameterivSGIX)(GLuint list, GLenum name, const GLint *param)
{
   DISPATCH(ListParameterivSGIX, (list, name, param), (F, ";"));
}

/* 94. GL_EXT_index_material */
KEYWORD1 void KEYWORD2 NAME(IndexMaterialEXT)(GLenum face, GLenum mode)
{
   DISPATCH(IndexMaterialEXT, (face, mode), (F, ";"));
}

/* 95. GL_EXT_index_func */
KEYWORD1 void KEYWORD2 NAME(IndexFuncEXT)(GLenum func, GLfloat ref)
{
   DISPATCH(IndexFuncEXT, (func, ref), (F, ";"));
}

/* 97. GL_EXT_compiled_vertex_array */
KEYWORD1 void KEYWORD2 NAME(LockArraysEXT)(GLint first, GLsizei count)
{
   DISPATCH(LockArraysEXT, (first, count), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(UnlockArraysEXT)(void)
{
   DISPATCH(UnlockArraysEXT, (), (F, ";"));
}

/* 98. GL_EXT_cull_vertex */
KEYWORD1 void KEYWORD2 NAME(CullParameterfvEXT)(GLenum pname, GLfloat *params)
{
   DISPATCH(CullParameterfvEXT, (pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(CullParameterdvEXT)(GLenum pname, GLdouble *params)
{
   DISPATCH(CullParameterdvEXT, (pname, params), (F, ";"));
}

/* 102. GL_SGIX_fragment_lighting */
KEYWORD1 void KEYWORD2 NAME(FragmentColorMaterialSGIX)(GLenum face, GLenum mode)
{
   DISPATCH(FragmentColorMaterialSGIX, (face, mode), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(FragmentLightfSGIX)(GLenum light, GLenum pname, GLfloat param)
{
   DISPATCH(FragmentLightfSGIX, (light, pname, param), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(FragmentLightfvSGIX)(GLenum light, GLenum pname, const GLfloat * params)
{
   DISPATCH(FragmentLightfvSGIX, (light, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(FragmentLightiSGIX)(GLenum light, GLenum pname, GLint param)
{
   DISPATCH(FragmentLightiSGIX, (light, pname, param), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(FragmentLightivSGIX)(GLenum light, GLenum pname, const GLint * params)
{
   DISPATCH(FragmentLightivSGIX, (light, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(FragmentLightModelfSGIX)(GLenum pname, GLfloat param)
{
   DISPATCH(FragmentLightModelfSGIX, (pname, param), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(FragmentLightModelfvSGIX)(GLenum pname, const GLfloat * params)
{
   DISPATCH(FragmentLightModelfvSGIX, (pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(FragmentLightModeliSGIX)(GLenum pname, GLint param)
{
   DISPATCH(FragmentLightModeliSGIX, (pname, param), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(FragmentLightModelivSGIX)(GLenum pname, const GLint * params)
{
   DISPATCH(FragmentLightModelivSGIX, (pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(FragmentMaterialfSGIX)(GLenum face, GLenum pname, GLfloat param)
{
   DISPATCH(FragmentMaterialfSGIX, (face, pname, param), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(FragmentMaterialfvSGIX)(GLenum face, GLenum pname, const GLfloat * params)
{
   DISPATCH(FragmentMaterialfvSGIX, (face, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(FragmentMaterialiSGIX)(GLenum face, GLenum pname, GLint param)
{
   DISPATCH(FragmentMaterialiSGIX, (face, pname, param), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(FragmentMaterialivSGIX)(GLenum face, GLenum pname, const GLint * params)
{
   DISPATCH(FragmentMaterialivSGIX, (face, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetFragmentLightfvSGIX)(GLenum light, GLenum pname, GLfloat * params)
{
   DISPATCH(FragmentLightfvSGIX, (light, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetFragmentLightivSGIX)(GLenum light, GLenum pname, GLint * params)
{
   DISPATCH(FragmentLightivSGIX, (light, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetFragmentMaterialfvSGIX)(GLenum face, GLenum pname, GLfloat * params)
{
   DISPATCH(FragmentMaterialfvSGIX, (face, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetFragmentMaterialivSGIX)(GLenum face, GLenum pname, GLint * params)
{
   DISPATCH(FragmentMaterialivSGIX, (face, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(LightEnviSGIX)(GLenum pname, GLint param)
{
   DISPATCH(LightEnviSGIX, (pname, param), (F, ";"));
}

/* 112. GL_EXT_draw_range_elements */
KEYWORD1 void KEYWORD2 NAME(DrawRangeElementsEXT)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices)
{
   DISPATCH(DrawRangeElements, (mode, start, end, count, type, indices), (F, "glDrawRangeElementsEXT(0x%x, %u %u %d 0x%x %p);", mode, start, end, count, type, indices));
}

/* 117. GL_EXT_light_texture */
#if 00
KEYWORD1 void KEYWORD2 NAME(ApplyTextureEXT)(GLenum mode)
{
   DISPATCH(ApplyTextureEXT, (mode), (F, "glApplyTextureEXT(0x%x);", mode));
}

KEYWORD1 void KEYWORD2 NAME(TextureLightEXT)(GLenum pname)
{
   DISPATCH(TextureLightEXT, (pname), (F, "glTextureLightEXT(0x%x);", pname));
}

KEYWORD1 void KEYWORD2 NAME(TextureMaterialEXT)(GLenum face, GLenum mode)
{
   DISPATCH(TextureMaterialEXT, (face, mode), (F, "glTextureMaterialEXT(0x%x, 0x%x);", face, mode));
}
#endif

/* 135. GL_INTEL_texture_scissor */
#if 00
KEYWORD1 void KEYWORD2 NAME(TexScissorINTEL)(GLenum target, GLclampf tlow, GLclampf thigh)
{
   DISPATCH(TexScissorINTEL, (target, tlow, thigh), (F, "glTexScissorINTEL(0x%x %g %g);", target, tlow, thigh));
}

KEYWORD1 void KEYWORD2 NAME(TexScissorFuncINTEL)(GLenum target, GLenum lfunc, GLenum hfunc)
{
   DISPATCH(TexScissorFuncINTEL, (target, lfunc, hfunc), (F, "glTexScissorFuncINTEL(0x%x 0x%x 0x%x);", target, tlow, thigh));
}
#endif

/* 136. GL_INTEL_parallel_arrays */
#if 00
KEYWORD1 void KEYWORD2 NAME(VertexPointervINTEL)(GLint size, GLenum type, const void ** pointer)
{
   DISPATCH(VertexPointervINTEL, (size, type, pointer), (F, "glVertexPointervINTEL(%d, 0x%x, %p);", size, type, pointer));
}

KEYWORD1 void KEYWORD2 NAME(NormalPointervINTEL)(GLenum type, const void** pointer)
{
   DISPATCH(NormalPointervINTEL, (size, pointer), (F, "glNormalPointervINTEL(%d, %p);", size, pointer));
}

KEYWORD1 void KEYWORD2 NAME(ColorPointervINTEL)(GLint size, GLenum type, const void** pointer)
{
   DISPATCH(ColorPointervINTEL, (size, type, pointer), (F, "glColorPointervINTEL(%d, 0x%x, %p);", size, type, pointer));
}

KEYWORD1 void KEYWORD2 NAME(TexCoordPointervINTEL)(GLint size, GLenum type, const void** pointer)
{
   DISPATCH(TexCoordPointervINTEL, (size, type, pointer), (F, "glTexCoordPointervINTEL(%d, 0x%x, %p);", size, type, pointer));
}
#endif

/* 138. GL_EXT_pixel_transform */
#if 00
KEYWORD1 void KEYWORD2 NAME(PixelTransformParameteriEXT)(GLenum target, GLenum pname, const GLint param)
{
   DISPATCH(PixelTransformParameteriEXT, (target, pname, param), (F, "glPixelTransformParameteriEXT(0x%x, 0x%x, %d);", target, pname, param));
}

KEYWORD1 void KEYWORD2 NAME(PixelTransformParameterfEXT)(GLenum target, GLenum pname, const GLfloat param)
{
   DISPATCH(PixelTransformParameterfEXT, (target, pname, param), (F, "glPixelTransformParameterfEXT(0x%x, 0x%x, %f);", target, pname, param));
}

KEYWORD1 void KEYWORD2 NAME(PixelTransformParameterivEXT)(GLenum target, GLenum pname, const GLint *params)
{
   DISPATCH(PixelTransformParameterivEXT, (target, pname, params), (F, "glPixelTransformParameterivEXT(0x%x, 0x%x, %p);", target, pname, params));
}

KEYWORD1 void KEYWORD2 NAME(PixelTransformParameterfvEXT)(GLenum target, GLenum pname, const GLfloat *params)
{
   DISPATCH(PixelTransformParameterfvEXT, (target, pname, params), (F, "glPixelTransformParameterfvEXT(0x%x, 0x%x, %p);", target, pname, params));
}

KEYWORD1 void KEYWORD2 NAME(GetPixelTransformParameterivEXT)(GLenum target, GLenum pname, const GLint *params)
{
   DISPATCH(GetPixelTransformParameterivEXT, (target, pname, params), (F, "glGetPixelTransformParameterivEXT(0x%x, 0x%x, %p);", target, pname, params));
}

KEYWORD1 void KEYWORD2 NAME(GetPixelTransformParameterfvEXT)(GLenum target, GLenum pname, const GLfloat *params)
{
   DISPATCH(GetPixelTransformParameterfvEXT, (target, pname, params), (F, "glGetPixelTransformParameterfvEXT(0x%x, 0x%x, %p);", target, pname, params));
}
#endif

/* 145. GL_EXT_secondary_color */
KEYWORD1 void KEYWORD2 NAME(SecondaryColorPointerEXT)(GLint size, GLenum type, GLsizei stride, GLvoid * pointer)
{
   DISPATCH(SecondaryColorPointerEXT, (size, type, stride, pointer), (F, "glSecondaryColorPointerEXT(%d, 0x%x, %d, %p);", size, type, stride, pointer));
}

/* 149. GL_EXT_fog_coord */
KEYWORD1 void KEYWORD2 NAME(FogCoordPointerEXT)(GLenum type, GLsizei stride, const GLvoid * pointer)
{
   DISPATCH(FogCoordPointerEXT, (type, stride, pointer), (F, "glFogCoordPointerEXT(0x%x, %d, %p);", type, stride, pointer));
}

/* 173. GL_EXT/INGR_blend_func_separate */
KEYWORD1 void KEYWORD2 NAME(BlendFuncSeparateEXT)(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha)
{
   DISPATCH(BlendFuncSeparateEXT, (sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(BlendFuncSeparateINGR)(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha)
{
   DISPATCH(BlendFuncSeparateEXT, (sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha), (F, ";"));
}

/* 190. GL_NV_vertex_array_range */
KEYWORD1 void KEYWORD2 NAME(FlushVertexArrayRangeNV)(void)
{
   DISPATCH(FlushVertexArrayRangeNV, (), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(VertexArrayRangeNV)(GLsizei size, const GLvoid * pointer)
{
   DISPATCH(VertexArrayRangeNV, (size, pointer), (F, ";"));
}

/* 191. GL_NV_register_combiners */
KEYWORD1 void KEYWORD2 NAME(CombinerParameterfvNV)(GLenum pname, const GLfloat * params)
{
   DISPATCH(CombinerParameterfvNV, (pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(CombinerParameterfNV)(GLenum pname, GLfloat param)
{
   DISPATCH(CombinerParameterfNV, (pname, param), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(CombinerParameterivNV)(GLenum pname, const GLint * params)
{
   DISPATCH(CombinerParameterivNV, (pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(CombinerParameteriNV)(GLenum pname, GLint param)
{
   DISPATCH(CombinerParameteriNV, (pname, param), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(CombinerInputNV)(GLenum stage, GLenum portion, GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage)
{
   DISPATCH(CombinerInputNV, (stage, portion, variable, input, mapping, componentUsage), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(CombinerOutputNV)(GLenum stage, GLenum portion, GLenum abOutput, GLenum cdOutput, GLenum sumOutput, GLenum scale, GLenum bias, GLboolean abDotProduct, GLboolean cdDotProduct, GLboolean muxSum)
{
   DISPATCH(CombinerOutputNV, (stage, portion, abOutput, cdOutput, sumOutput, scale, bias, abDotProduct, cdDotProduct, muxSum), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(FinalCombinerInputNV)(GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage)
{
   DISPATCH(FinalCombinerInputNV, (variable, input, mapping, componentUsage), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetCombinerInputParameterfvNV)(GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLfloat * params)
{
   DISPATCH(GetCombinerInputParameterfvNV, (stage, portion, variable, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetCombinerInputParameterivNV)(GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLint * params)
{
   DISPATCH(GetCombinerInputParameterivNV, (stage, portion, variable, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetCombinerOutputParameterfvNV)(GLenum stage, GLenum portion, GLenum pname, GLfloat * params)
{
   DISPATCH(GetCombinerOutputParameterfvNV, (stage, portion, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetCombinerOutputParameterivNV)(GLenum stage, GLenum portion, GLenum pname, GLint * params)
{
   DISPATCH(GetCombinerOutputParameterivNV, (stage, portion, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetFinalCombinerInputParameterfvNV)(GLenum variable, GLenum pname, GLfloat * params)
{
DISPATCH(GetFinalCombinerInputParameterfvNV, (variable, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(GetFinalCombinerInputParameterivNV)(GLenum variable, GLenum pname, GLint * params)
{
DISPATCH(GetFinalCombinerInputParameterivNV, (variable, pname, params), (F, ";"));
}

/* 194. GL_EXT_vertex_weighting */
KEYWORD1 void KEYWORD2 NAME(VertexWeightfEXT)(GLfloat weight)
{
   DISPATCH(VertexWeightfEXT, (weight), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(VertexWeightfvEXT)(const GLfloat * weight)
{
   DISPATCH(VertexWeightfvEXT, (weight), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(VertexWeightPointerEXT)(GLsizei size, GLenum type, GLsizei stride, const GLvoid * pointer)
{
   DISPATCH(VertexWeightPointerEXT, (size, type, stride, pointer), (F, ";"));
}

/* 196. GL_MESA_resize_buffers */
KEYWORD1 void KEYWORD2 NAME(ResizeBuffersMESA)(void)
{
   DISPATCH(ResizeBuffersMESA, (), (F, "glResizeBuffersMESA();"));
}

/* 197. GL_MESA_window_pos */
KEYWORD1 void KEYWORD2 NAME(WindowPos2iMESA)(GLint x, GLint y)
{
   DISPATCH(WindowPos2iMESA, (x, y), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2sMESA)(GLshort x, GLshort y)
{
   DISPATCH(WindowPos2sMESA, (x, y), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2fMESA)(GLfloat x, GLfloat y)
{
   DISPATCH(WindowPos2fMESA, (x, y), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2dMESA)(GLdouble x, GLdouble y)
{
   DISPATCH(WindowPos2dMESA, (x, y), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2ivMESA)(const GLint *p)
{
   DISPATCH(WindowPos2ivMESA, (p), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2svMESA)(const GLshort *p)
{
   DISPATCH(WindowPos2svMESA, (p), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2fvMESA)(const GLfloat *p)
{
   DISPATCH(WindowPos2fvMESA, (p), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2dvMESA)(const GLdouble *p)
{
   DISPATCH(WindowPos2dvMESA, (p), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3iMESA)(GLint x, GLint y, GLint z)
{
   DISPATCH(WindowPos3iMESA, (x, y, z), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3sMESA)(GLshort x, GLshort y, GLshort z)
{
   DISPATCH(WindowPos3sMESA, (x, y, z), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3fMESA)(GLfloat x, GLfloat y, GLfloat z)
{
   DISPATCH(WindowPos3fMESA, (x, y, z), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3dMESA)(GLdouble x, GLdouble y, GLdouble z)
{
   DISPATCH(WindowPos3dMESA, (x, y, z), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3ivMESA)(const GLint *p)
{
   DISPATCH(WindowPos3ivMESA, (p), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3svMESA)(const GLshort *p)
{
   DISPATCH(WindowPos3svMESA, (p), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3fvMESA)(const GLfloat *p)
{
   DISPATCH(WindowPos3fvMESA, (p), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3dvMESA)(const GLdouble *p)
{
   DISPATCH(WindowPos3dvMESA, (p), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos4iMESA)(GLint x, GLint y, GLint z, GLint w)
{
   DISPATCH(WindowPos4iMESA, (x, y, z, w), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos4sMESA)(GLshort x, GLshort y, GLshort z, GLshort w)
{
   DISPATCH(WindowPos4sMESA, (x, y, z, w), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos4fMESA)(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   DISPATCH(WindowPos4fMESA, (x, y, z, w), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos4dMESA)(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   DISPATCH(WindowPos4dMESA, (x, y, z, w), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos4ivMESA)(const GLint *p)
{
   DISPATCH(WindowPos4ivMESA, (p), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos4svMESA)(const GLshort *p)
{
   DISPATCH(WindowPos4svMESA, (p), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos4fvMESA)(const GLfloat *p)
{
   DISPATCH(WindowPos4fvMESA, (p), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos4dvMESA)(const GLdouble *p)
{
   DISPATCH(WindowPos4dvMESA, (p), (F, ";"));
}

/* 208. GL_3DFX_tbuffer */
KEYWORD1 void KEYWORD2 NAME(TbufferMask3DFX)(GLuint mask)
{
   DISPATCH(TbufferMask3DFX, (mask), (F, "glTbufferMask3DFX(0x%x);", mask));
}

/* 209. WGL_EXT_multisample */

KEYWORD1 void KEYWORD2 NAME(SampleMaskEXT)(GLclampf value, GLboolean invert)
{
   DISPATCH(SampleMaskSGIS, (value, invert), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(SamplePatternEXT)(GLenum pattern)
{
   DISPATCH(SamplePatternSGIS, (pattern), (F, ";"));
}

/* ARB 1. GL_ARB_multitexture */

KEYWORD1 void KEYWORD2 NAME(ActiveTextureARB)(GLenum texture)
{
   DISPATCH(ActiveTextureARB, (texture), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(ClientActiveTextureARB)(GLenum texture)
{
   DISPATCH(ClientActiveTextureARB, (texture), (F, ";"));
}


/* ARB 3. GL_ARB_transpose_matrix */
KEYWORD1 void KEYWORD2 NAME(LoadTransposeMatrixdARB)(const GLdouble m[16])
{
   DISPATCH(LoadTransposeMatrixdARB, (m), (F, "glLoadTransposeMatrixARB(%p);", m));
}

KEYWORD1 void KEYWORD2 NAME(LoadTransposeMatrixfARB)(const GLfloat m[16])
{
   DISPATCH(LoadTransposeMatrixfARB, (m), (F, "glLoadTransposeMatrixfARB(%p)", m));
}

KEYWORD1 void KEYWORD2 NAME(MultTransposeMatrixdARB)(const GLdouble m[16])
{
   DISPATCH(MultTransposeMatrixdARB, (m), (F, "glMultTransposeMatrixfARB(%p)", m));
}

KEYWORD1 void KEYWORD2 NAME(MultTransposeMatrixfARB)(const GLfloat m[16])
{
   DISPATCH(MultTransposeMatrixfARB, (m), (F, "glLoadTransposeMatrixfARB(%p)", m));
}

/* ARB 5. GL_ARB_multisample */
KEYWORD1 void KEYWORD2 NAME(SampleCoverageARB)(GLclampf value, GLboolean invert)
{
   DISPATCH(SampleCoverageARB, (value, invert), (F, "glSampleCoverageARB(%f, %d);", value, invert));
}

KEYWORD1 void KEYWORD2 NAME(SamplePassARB)(GLenum pass)
{
   DISPATCH(SamplePassARB, (pass), (F, "glSamplePassARB(0x%x);", pass));
}

/* ARB 12. GL_ARB_texture_compression */
KEYWORD1 void KEYWORD2 NAME(CompressedTexImage3DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid *data)
{
   DISPATCH(CompressedTexImage3DARB, (target, level, internalformat, width, height, depth, border, imageSize, data), (F, "glCompressedTexImage3DARB();"));
}

KEYWORD1 void KEYWORD2 NAME(CompressedTexImage2DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data)
{
   DISPATCH(CompressedTexImage2DARB, (target, level, internalformat, width, height, border, imageSize, data), (F, "glCompressedTexImage2DARB();"));
}

KEYWORD1 void KEYWORD2 NAME(CompressedTexImage1DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid *data)
{
   DISPATCH(CompressedTexImage1DARB, (target, level, internalformat, width, border, imageSize, data), (F, "glCompressedTexImage1DARB();"));
}

KEYWORD1 void KEYWORD2 NAME(CompressedTexSubImage3DARB)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid *data)
{
   DISPATCH(CompressedTexSubImage3DARB, (target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data), (F, "glCompressedTexSubImage3DARB();"));
}

KEYWORD1 void KEYWORD2 NAME(CompressedTexSubImage2DARB)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *data)
{
   DISPATCH(CompressedTexSubImage2DARB, (target, level, xoffset, yoffset, width, height, format, imageSize, data), (F, "glCompressedTexSubImage2DARB();"));
}

KEYWORD1 void KEYWORD2 NAME(CompressedTexSubImage1DARB)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid *data)
{
   DISPATCH(CompressedTexSubImage1DARB, (target, level, xoffset, width, format, imageSize, data), (F, "glCompressedTexSubImage1DARB();"));
}

KEYWORD1 void KEYWORD2 NAME(GetCompressedTexImageARB)(GLenum target, GLint lod, GLvoid *img)
{
   DISPATCH(GetCompressedTexImageARB, (target, lod, img), (F, "glGetCompressedTexImageARB();"));
}




KEYWORD1 void KEYWORD2 NAME(Begin)(GLenum mode)
{
   DISPATCH(Begin, (mode), (F, "glBegin(0x%x);", mode));
}

KEYWORD1 void KEYWORD2 NAME(CallList)(GLuint list)
{
   DISPATCH(CallList, (list), (F, "glCallList(%u);", list));
}

KEYWORD1 void KEYWORD2 NAME(CallLists)(GLsizei n, GLenum type, const GLvoid *lists)
{
   DISPATCH(CallLists, (n, type, lists), (F, "glCallLists(%d, 0x%x, %p);", n, type, lists));
}

KEYWORD1 void KEYWORD2 NAME(Color3b)(GLbyte red, GLbyte green, GLbyte blue)
{
   DISPATCH(Color3b, (red, green, blue), (F, "glColor3b(%d, %d, %d);", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(Color3d)(GLdouble red, GLdouble green, GLdouble blue)
{
   DISPATCH(Color3d, (red, green, blue), (F, "glColor3d(%g, %g, %g);", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(Color3f)(GLfloat red, GLfloat green, GLfloat blue)
{
   DISPATCH(Color3f, (red, green, blue), (F, "glColor3f(%g, %g, %g);", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(Color3i)(GLint red, GLint green, GLint blue)
{
   DISPATCH(Color3i, (red, green, blue), (F, "glColor3i(%d, %d, %d);", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(Color3s)(GLshort red, GLshort green, GLshort blue)
{
   DISPATCH(Color3s, (red, green, blue), (F, "glColor3s(%d, %d, %d);", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(Color3ub)(GLubyte red, GLubyte green, GLubyte blue)
{
   DISPATCH(Color3ub, (red, green, blue), (F, "glColor3ub(%u, %u, %u);", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(Color3ui)(GLuint red, GLuint green, GLuint blue)
{
   DISPATCH(Color3ui, (red, green, blue), (F, "glColor3ui(%u, %u, %u);", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(Color3us)(GLushort red, GLushort green, GLushort blue)
{
   DISPATCH(Color3us, (red, green, blue), (F, "glColor3us(%u, %u, %u);", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(Color4b)(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha)
{
   DISPATCH(Color4b, (red, green, blue, alpha), (F, "glColor4b(%d, %d, %d, %d);", red, green, blue, alpha));
}

KEYWORD1 void KEYWORD2 NAME(Color4d)(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha)
{
   DISPATCH(Color4d, (red, green, blue, alpha), (F, "glColor4d(%g, %g, %g, %g);", red, green, blue, alpha));
}

KEYWORD1 void KEYWORD2 NAME(Color4f)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
   DISPATCH(Color4f, (red, green, blue, alpha), (F, "glColor4b(%g, %g, %g, %g);", red, green, blue, alpha));
}

KEYWORD1 void KEYWORD2 NAME(Color4i)(GLint red, GLint green, GLint blue, GLint alpha)
{
   DISPATCH(Color4i, (red, green, blue, alpha), (F, "glColor4i(%d, %d, %d);", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(Color4s)(GLshort red, GLshort green, GLshort blue, GLshort alpha)
{
   DISPATCH(Color4s, (red, green, blue, alpha), (F, "glColor4s(%d, %d, %d, %d);", red, green, blue, alpha));
}

KEYWORD1 void KEYWORD2 NAME(Color4ub)(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
   DISPATCH(Color4ub, (red, green, blue, alpha), (F, "glColor4ub(%u, %u, %u, %u);", red, green, blue, alpha));
}

KEYWORD1 void KEYWORD2 NAME(Color4ui)(GLuint red, GLuint green, GLuint blue, GLuint alpha)
{
   DISPATCH(Color4ui, (red, green, blue, alpha), (F, "glColor4ui(%u, %u, %u, %u);", red, green, blue, alpha));
}

KEYWORD1 void KEYWORD2 NAME(Color4us)(GLushort red, GLushort green, GLushort blue, GLushort alpha)
{
   DISPATCH(Color4us, (red, green, blue, alpha), (F, "glColor4us(%u, %u, %u, %u);", red, green, blue, alpha));
}

KEYWORD1 void KEYWORD2 NAME(Color3bv)(const GLbyte *v)
{
   DISPATCH(Color3bv, (v), (F, "glColor3bf(%p);", v));
}

KEYWORD1 void KEYWORD2 NAME(Color3dv)(const GLdouble *v)
{
   DISPATCH(Color3dv, (v), (F, "glColor3dv(%p);", v));
}

KEYWORD1 void KEYWORD2 NAME(Color3fv)(const GLfloat *v)
{
   DISPATCH(Color3fv, (v), (F, "glColor3fv(%p);", v));
}

KEYWORD1 void KEYWORD2 NAME(Color3iv)(const GLint *v)
{
   DISPATCH(Color3iv, (v), (F, "glColor3iv(%p);", v));
}

KEYWORD1 void KEYWORD2 NAME(Color3sv)(const GLshort *v)
{
   DISPATCH(Color3sv, (v), (F, "glColor3sv(%p);", v));
}

KEYWORD1 void KEYWORD2 NAME(Color3ubv)(const GLubyte *v)
{
   DISPATCH(Color3ubv, (v), (F, "glColor3ubv(%p);", v));
}

KEYWORD1 void KEYWORD2 NAME(Color3uiv)(const GLuint *v)
{
   DISPATCH(Color3uiv, (v), (F, "glColor3uiv(%p);", v));
}

KEYWORD1 void KEYWORD2 NAME(Color3usv)(const GLushort *v)
{
   DISPATCH(Color3usv, (v), (F, "glColor3usv(%p);", v));
}

KEYWORD1 void KEYWORD2 NAME(Color4bv)(const GLbyte *v)
{
   DISPATCH(Color4bv, (v), (F, "glColor3bv(%p);", v));
}

KEYWORD1 void KEYWORD2 NAME(Color4dv)(const GLdouble *v)
{
   DISPATCH(Color4dv, (v), (F, "glColor4dv(%p);", v));
}

KEYWORD1 void KEYWORD2 NAME(Color4fv)(const GLfloat *v)
{
   DISPATCH(Color4fv, (v), (F, "glColor4fv(%p);", v));
}

KEYWORD1 void KEYWORD2 NAME(Color4iv)(const GLint *v)
{
   DISPATCH(Color4iv, (v), (F, "glColor4iv(%p);", v));
}

KEYWORD1 void KEYWORD2 NAME(Color4sv)(const GLshort *v)
{
   DISPATCH(Color4sv, (v), (F, "glColor4sv(%p);", v));
}

KEYWORD1 void KEYWORD2 NAME(Color4ubv)(const GLubyte *v)
{
   DISPATCH(Color4ubv, (v), (F, "glColor4ubv(%p);", v));
}

KEYWORD1 void KEYWORD2 NAME(Color4uiv)(const GLuint *v)
{
   DISPATCH(Color4uiv, (v), (F, "glColor4uiv(%p);", v));
}

KEYWORD1 void KEYWORD2 NAME(Color4usv)(const GLushort *v)
{
   DISPATCH(Color4usv, (v), (F, "glColor4usv(%p);", v));
}

KEYWORD1 void KEYWORD2 NAME(End)(void)
{
   DISPATCH(End, (), (F, "glEnd();"));
}

KEYWORD1 void KEYWORD2 NAME(EvalCoord1d)(GLdouble u)
{
   DISPATCH(EvalCoord1d, (u), (F, "glEvalCoord1d(%g);", u));
}

KEYWORD1 void KEYWORD2 NAME(EvalCoord1f)(GLfloat u)
{
   DISPATCH(EvalCoord1f, (u), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(EvalCoord1dv)(const GLdouble *u)
{
   DISPATCH(EvalCoord1dv, (u), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(EvalCoord1fv)(const GLfloat *u)
{
   DISPATCH(EvalCoord1fv, (u), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(EvalCoord2d)(GLdouble u, GLdouble v)
{
   DISPATCH(EvalCoord2d, (u, v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(EvalCoord2f)(GLfloat u, GLfloat v)
{
   DISPATCH(EvalCoord2f, (u, v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(EvalCoord2dv)(const GLdouble *u)
{
   DISPATCH(EvalCoord2dv, (u), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(EvalCoord2fv)(const GLfloat *u)
{
   DISPATCH(EvalCoord2fv, (u), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(EvalPoint1)(GLint i)
{
   DISPATCH(EvalPoint1, (i), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(EvalPoint2)(GLint i, GLint j)
{
   DISPATCH(EvalPoint2, (i, j), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(EdgeFlag)(GLboolean flag)
{
   DISPATCH(EdgeFlag, (flag), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(EdgeFlagv)(const GLboolean *flag)
{
   DISPATCH(EdgeFlagv, (flag), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(Indexd)(GLdouble c)
{
   DISPATCH(Indexd, (c), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(Indexdv)(const GLdouble *c)
{
   DISPATCH(Indexdv, (c), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(Indexf)(GLfloat c)
{
   DISPATCH(Indexf, (c), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(Indexfv)(const GLfloat *c)
{
   DISPATCH(Indexfv, (c), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(Indexi)(GLint c)
{
   DISPATCH(Indexi, (c), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(Indexiv)(const GLint *c)
{
   DISPATCH(Indexiv, (c), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(Indexs)(GLshort c)
{
   DISPATCH(Indexs, (c), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(Indexsv)(const GLshort *c)
{
   DISPATCH(Indexsv, (c), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(Materialf)(GLenum face, GLenum pname, GLfloat param)
{
   DISPATCH(Materialf, (face, pname, param), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(Materiali)(GLenum face, GLenum pname, GLint param)
{
   DISPATCH(Materiali, (face, pname, param), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(Materialfv)(GLenum face, GLenum pname, const GLfloat *params)
{
   DISPATCH(Materialfv, (face, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(Materialiv)(GLenum face, GLenum pname, const GLint *params)
{
   DISPATCH(Materialiv, (face, pname, params), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(Normal3b)(GLbyte nx, GLbyte ny, GLbyte nz)
{
   DISPATCH(Normal3b, (nx, ny, nz), (F, "glNormal3b(%d, %d, %d);", nx, ny, nz));
}

KEYWORD1 void KEYWORD2 NAME(Normal3bv)(const GLbyte *v)
{
   DISPATCH(Normal3bv, (v), (F, "glNormal3bv(%p);", v));
}

KEYWORD1 void KEYWORD2 NAME(Normal3d)(GLdouble nx, GLdouble ny, GLdouble nz)
{
   DISPATCH(Normal3d, (nx, ny, nz), (F, "glNormal3d(%f, %f, %f);", nx, ny, nz));
}

KEYWORD1 void KEYWORD2 NAME(Normal3dv)(const GLdouble *v)
{
   DISPATCH(Normal3dv, (v), (F, "glNormal3dv(%p);", v));
}

KEYWORD1 void KEYWORD2 NAME(Normal3f)(GLfloat nx, GLfloat ny, GLfloat nz)
{
   DISPATCH(Normal3f, (nx, ny, nz), (F, "glNormal3f(%g, %g, %g);", nx, ny, nz));
}

KEYWORD1 void KEYWORD2 NAME(Normal3fv)(const GLfloat *v)
{
   DISPATCH(Normal3fv, (v), (F, "glNormal3fv(%p);", v));
}

KEYWORD1 void KEYWORD2 NAME(Normal3i)(GLint nx, GLint ny, GLint nz)
{
   DISPATCH(Normal3i, (nx, ny, nz), (F, "glNormal3i(%d, %d, %d);", nx, ny, nz));
}

KEYWORD1 void KEYWORD2 NAME(Normal3iv)(const GLint *v)
{
   DISPATCH(Normal3iv, (v), (F, "glNormal3iv(%p);", v));
}

KEYWORD1 void KEYWORD2 NAME(Normal3s)(GLshort nx, GLshort ny, GLshort nz)
{
   DISPATCH(Normal3s, (nx, ny, nz), (F, "glNormal3s(%d, %d, %d);", nx, ny, nz));
}

KEYWORD1 void KEYWORD2 NAME(Normal3sv)(const GLshort *v)
{
   DISPATCH(Normal3sv, (v), (F, "glNormal3sv(%p);", v));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord1d)(GLdouble s)
{
   DISPATCH(TexCoord1d, (s), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord1f)(GLfloat s)
{
   DISPATCH(TexCoord1f, (s), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord1i)(GLint s)
{
   DISPATCH(TexCoord1i, (s), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord1s)(GLshort s)
{
   DISPATCH(TexCoord1s, (s), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord2d)(GLdouble s, GLdouble t)
{
   DISPATCH(TexCoord2d, (s, t), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord2f)(GLfloat s, GLfloat t)
{
   DISPATCH(TexCoord2f, (s, t), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord2s)(GLshort s, GLshort t)
{
   DISPATCH(TexCoord2s, (s, t), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord2i)(GLint s, GLint t)
{
   DISPATCH(TexCoord2i, (s, t), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord3d)(GLdouble s, GLdouble t, GLdouble r)
{
   DISPATCH(TexCoord3d, (s, t, r), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord3f)(GLfloat s, GLfloat t, GLfloat r)
{
   DISPATCH(TexCoord3f, (s, t, r), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord3i)(GLint s, GLint t, GLint r)
{
   DISPATCH(TexCoord3i, (s, t, r), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord3s)(GLshort s, GLshort t, GLshort r)
{
   DISPATCH(TexCoord3s, (s, t, r), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord4d)(GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
   DISPATCH(TexCoord4d, (s, t, r, q), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord4f)(GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
   DISPATCH(TexCoord4f, (s, t, r, q), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord4i)(GLint s, GLint t, GLint r, GLint q)
{
   DISPATCH(TexCoord4i, (s, t, r, q), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord4s)(GLshort s, GLshort t, GLshort r, GLshort q)
{
   DISPATCH(TexCoord4s, (s, t, r, q), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord1dv)(const GLdouble *v)
{
   DISPATCH(TexCoord1dv, (v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord1fv)(const GLfloat *v)
{
   DISPATCH(TexCoord1fv, (v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord1iv)(const GLint *v)
{
   DISPATCH(TexCoord1iv, (v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord1sv)(const GLshort *v)
{
   DISPATCH(TexCoord1sv, (v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord2dv)(const GLdouble *v)
{
   DISPATCH(TexCoord2dv, (v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord2fv)(const GLfloat *v)
{
   DISPATCH(TexCoord2fv, (v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord2iv)(const GLint *v)
{
   DISPATCH(TexCoord2iv, (v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord2sv)(const GLshort *v)
{
   DISPATCH(TexCoord2sv, (v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord3dv)(const GLdouble *v)
{
   DISPATCH(TexCoord3dv, (v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord3fv)(const GLfloat *v)
{
   DISPATCH(TexCoord3fv, (v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord3iv)(const GLint *v)
{
   DISPATCH(TexCoord3iv, (v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord3sv)(const GLshort *v)
{
   DISPATCH(TexCoord3sv, (v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord4dv)(const GLdouble *v)
{
   DISPATCH(TexCoord4dv, (v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord4fv)(const GLfloat *v)
{
   DISPATCH(TexCoord4fv, (v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord4iv)(const GLint *v)
{
   DISPATCH(TexCoord4iv, (v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord4sv)(const GLshort *v)
{
   DISPATCH(TexCoord4sv, (v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(Vertex2d)(GLdouble x, GLdouble y)
{
   DISPATCH(Vertex2d, (x, y), (F, "glVertex2d(%f, %f);", x, y));
}

KEYWORD1 void KEYWORD2 NAME(Vertex2dv)(const GLdouble *v)
{
   DISPATCH(Vertex2dv, (v), (F, "glVertex2dv(%p);", v));
}

KEYWORD1 void KEYWORD2 NAME(Vertex2f)(GLfloat x, GLfloat y)
{
   DISPATCH(Vertex2f, (x, y), (F, "glVertex2f(%g, %g);", x, y));
}

KEYWORD1 void KEYWORD2 NAME(Vertex2fv)(const GLfloat *v)
{
   DISPATCH(Vertex2fv, (v), (F, "glVertex2fv(%p);", v));
}

KEYWORD1 void KEYWORD2 NAME(Vertex2i)(GLint x, GLint y)
{
   DISPATCH(Vertex2i, (x, y), (F, "glVertex2i(%d, %d);", x, y));
}

KEYWORD1 void KEYWORD2 NAME(Vertex2iv)(const GLint *v)
{
   DISPATCH(Vertex2iv, (v), (F, "glVertex2iv(%p);", v));
}

KEYWORD1 void KEYWORD2 NAME(Vertex2s)(GLshort x, GLshort y)
{
   DISPATCH(Vertex2s, (x, y), (F, "glVertex2s(%d, %d);", x, y));
}

KEYWORD1 void KEYWORD2 NAME(Vertex2sv)(const GLshort *v)
{
   DISPATCH(Vertex2sv, (v), (F, "glVertex2sv(%p);", v));
}

KEYWORD1 void KEYWORD2 NAME(Vertex3d)(GLdouble x, GLdouble y, GLdouble z)
{
   DISPATCH(Vertex3d, (x, y, z), (F, "glVertex3d(%f, %f, %f);", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(Vertex3dv)(const GLdouble *v)
{
   DISPATCH(Vertex3dv, (v), (F, "glVertex3dv(%p);", v));
}

KEYWORD1 void KEYWORD2 NAME(Vertex3f)(GLfloat x, GLfloat y, GLfloat z)
{
   DISPATCH(Vertex3f, (x, y, z), (F, "glVertex3f(%g, %g, %g);", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(Vertex3fv)(const GLfloat *v)
{
   DISPATCH(Vertex3fv, (v), (F, "glVertex3fv(%p);", v));
}

KEYWORD1 void KEYWORD2 NAME(Vertex3i)(GLint x, GLint y, GLint z)
{
   DISPATCH(Vertex3i, (x, y, z), (F, "glVertex3i(%d, %d, %d);", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(Vertex3iv)(const GLint *v)
{
   DISPATCH(Vertex3iv, (v), (F, "glVertex3iv(%p);", v));
}

KEYWORD1 void KEYWORD2 NAME(Vertex3s)(GLshort x, GLshort y, GLshort z)
{
   DISPATCH(Vertex3s, (x, y, z), (F, "glVertex3s(%d, %d, %d);", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(Vertex3sv)(const GLshort *v)
{
   DISPATCH(Vertex3sv, (v), (F, "glVertex3sv(%p);", v));
}

KEYWORD1 void KEYWORD2 NAME(Vertex4d)(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   DISPATCH(Vertex4d, (x, y, z, w), (F, "glVertex4d(%f, %f, %f, %f);", x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(Vertex4dv)(const GLdouble *v)
{
   DISPATCH(Vertex4dv, (v), (F, "glVertex4dv(%p);", v));
}

KEYWORD1 void KEYWORD2 NAME(Vertex4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   DISPATCH(Vertex4f, (x, y, z, w), (F, "glVertex4f(%f, %f, %f, %f);", x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(Vertex4fv)(const GLfloat *v)
{
   DISPATCH(Vertex4fv, (v), (F, "glVertex4fv(%p);", v));
}

KEYWORD1 void KEYWORD2 NAME(Vertex4i)(GLint x, GLint y, GLint z, GLint w)
{
   DISPATCH(Vertex4i, (x, y, z, w), (F, "glVertex4i(%d, %d, %d, %d);", x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(Vertex4iv)(const GLint *v)
{
   DISPATCH(Vertex4iv, (v), (F, "glVertex4iv(%p);", v));
}

KEYWORD1 void KEYWORD2 NAME(Vertex4s)(GLshort x, GLshort y, GLshort z, GLshort w)
{
   DISPATCH(Vertex4s, (x, y, z, w), (F, "glVertex4s(%d, %d, %d, %d);", x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(Vertex4sv)(const GLshort *v)
{
   DISPATCH(Vertex4sv, (v), (F, "glVertex4sv(%p);", v));
}

KEYWORD1 void KEYWORD2 NAME(Indexub)(GLubyte c)
{
   DISPATCH(Indexub, (c), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(Indexubv)(const GLubyte *c)
{
   DISPATCH(Indexubv, (c), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(ArrayElementEXT)(GLint i)
{
   DISPATCH(ArrayElement, (i), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3bEXT)(GLbyte red, GLbyte green, GLbyte blue)
{
   DISPATCH(SecondaryColor3bEXT, (red, green, blue), (F, "glSecondaryColor3bEXT(%d, %d, %d);", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3bvEXT)(const GLbyte *v)
{
   DISPATCH(SecondaryColor3bvEXT, (v), (F, "glSecondaryColor3bvEXT(%d, %d, %d);", v[0], v[1], v[2]));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3dEXT)(GLdouble red, GLdouble green, GLdouble blue)
{
   DISPATCH(SecondaryColor3dEXT, (red, green, blue), (F, "glSecondaryColor3dEXT(%g, %g, %g);", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3dvEXT)(const GLdouble * v)
{
   DISPATCH(SecondaryColor3dvEXT, (v), (F, "glSecondaryColor3dvEXT(%g, %g, %g);", v[0], v[1], v[2]));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3fEXT)(GLfloat red, GLfloat green, GLfloat blue)
{
   DISPATCH(SecondaryColor3fEXT, (red, green, blue), (F, "glSecondaryColor3fEXT(%g, %g, %g);", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3fvEXT)(const GLfloat * v)
{
   DISPATCH(SecondaryColor3fvEXT, (v), (F, "glSecondaryColor3fvEXT(%g, %g, %g);", v[0], v[1], v[2]));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3iEXT)(GLint red, GLint green, GLint blue)
{
   DISPATCH(SecondaryColor3iEXT, (red, green, blue), (F, "glSecondaryColor3iEXT(%d, %d, %d);", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3ivEXT)(const GLint * v)
{
   DISPATCH(SecondaryColor3ivEXT, (v), (F, "glSecondaryColor3ivEXT(%d, %d, %d);", v[0], v[1], v[2]));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3sEXT)(GLshort red, GLshort green, GLshort blue)
{
   DISPATCH(SecondaryColor3sEXT, (red, green, blue), (F, "glSecondaryColor3sEXT(%d, %d, %d);", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3svEXT)(const GLshort * v)
{
   DISPATCH(SecondaryColor3svEXT, (v), (F, "glSecondaryColor3svEXT(%d, %d, %d);", v[0], v[1], v[2]));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3ubEXT)(GLubyte red, GLubyte green, GLubyte blue)
{
   DISPATCH(SecondaryColor3ubEXT, (red, green, blue), (F, "glSecondaryColor3ubEXT(%d, %d, %d);", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3ubvEXT)(const GLubyte * v)
{
   DISPATCH(SecondaryColor3ubvEXT, (v), (F, "glSecondaryColor3ubvEXT(%d, %d, %d);", v[0], v[1], v[2]));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3uiEXT)(GLuint red, GLuint green, GLuint blue)
{
   DISPATCH(SecondaryColor3uiEXT, (red, green, blue), (F, "glSecondaryColor3uiEXT(%d, %d, %d);", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3uivEXT)(const GLuint * v)
{
   DISPATCH(SecondaryColor3uivEXT, (v), (F, "glSecondaryColor3uivEXT(%d, %d, %d);", v[0], v[1], v[2]));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3usEXT)(GLushort red, GLushort green, GLushort blue)
{
   DISPATCH(SecondaryColor3usEXT, (red, green, blue), (F, "glSecondaryColor3usEXT(%d, %d, %d);", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3usvEXT)(const GLushort * v)
{
   DISPATCH(SecondaryColor3usvEXT, (v), (F, "glSecondaryColor3usvEXT(%d, %d, %d);", v[0], v[1], v[2]));
}

KEYWORD1 void KEYWORD2 NAME(FogCoordfEXT)(GLfloat coord)
{
   DISPATCH(FogCoordfEXT, (coord), (F, "glFogCoordfEXT(%g);", coord));
}

KEYWORD1 void KEYWORD2 NAME(FogCoordfvEXT)(const GLfloat * coord)
{
   DISPATCH(FogCoordfvEXT, (coord), (F, "glFogCoordfvEXT(%p);", coord));
}

KEYWORD1 void KEYWORD2 NAME(FogCoorddEXT)(GLdouble coord)
{
   DISPATCH(FogCoorddEXT, (coord), (F, "glFogCoorddEXT(%g);", coord));
}

KEYWORD1 void KEYWORD2 NAME(FogCoorddvEXT)(const GLdouble * coord)
{
   DISPATCH(FogCoorddvEXT, (coord), (F, "glFogCoorddvEXT(%p);", coord));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1dARB)(GLenum target, GLdouble s)
{
   DISPATCH(MultiTexCoord1dARB, (target, s), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1dvARB)(GLenum target, const GLdouble *v)
{
   DISPATCH(MultiTexCoord1dvARB, (target, v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1fARB)(GLenum target, GLfloat s)
{
   DISPATCH(MultiTexCoord1fARB, (target, s), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1fvARB)(GLenum target, const GLfloat *v)
{
   DISPATCH(MultiTexCoord1fvARB, (target, v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1iARB)(GLenum target, GLint s)
{
   DISPATCH(MultiTexCoord1iARB, (target, s), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1ivARB)(GLenum target, const GLint *v)
{
   DISPATCH(MultiTexCoord1ivARB, (target, v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1sARB)(GLenum target, GLshort s)
{
   DISPATCH(MultiTexCoord1sARB, (target, s), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1svARB)(GLenum target, const GLshort *v)
{
   DISPATCH(MultiTexCoord1svARB, (target, v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2dARB)(GLenum target, GLdouble s, GLdouble t)
{
   DISPATCH(MultiTexCoord2dARB, (target, s, t), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2dvARB)(GLenum target, const GLdouble *v)
{
   DISPATCH(MultiTexCoord2dvARB, (target, v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2fARB)(GLenum target, GLfloat s, GLfloat t)
{
   DISPATCH(MultiTexCoord2fARB, (target, s, t), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2fvARB)(GLenum target, const GLfloat *v)
{
   DISPATCH(MultiTexCoord2fvARB, (target, v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2iARB)(GLenum target, GLint s, GLint t)
{
   DISPATCH(MultiTexCoord2iARB, (target, s, t), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2ivARB)(GLenum target, const GLint *v)
{
   DISPATCH(MultiTexCoord2ivARB, (target, v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2sARB)(GLenum target, GLshort s, GLshort t)
{
   DISPATCH(MultiTexCoord2sARB, (target, s, t), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2svARB)(GLenum target, const GLshort *v)
{
   DISPATCH(MultiTexCoord2svARB, (target, v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3dARB)(GLenum target, GLdouble s, GLdouble t, GLdouble r)
{
   DISPATCH(MultiTexCoord3dARB, (target, s, t, r), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3dvARB)(GLenum target, const GLdouble *v)
{
   DISPATCH(MultiTexCoord3dvARB, (target, v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3fARB)(GLenum target, GLfloat s, GLfloat t, GLfloat r)
{
   DISPATCH(MultiTexCoord3fARB, (target, s, t, r), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3fvARB)(GLenum target, const GLfloat *v)
{
   DISPATCH(MultiTexCoord3fvARB, (target, v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3iARB)(GLenum target, GLint s, GLint t, GLint r)
{
   DISPATCH(MultiTexCoord3iARB, (target, s, t, r), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3ivARB)(GLenum target, const GLint *v)
{
   DISPATCH(MultiTexCoord3ivARB, (target, v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3sARB)(GLenum target, GLshort s, GLshort t, GLshort r)
{
   DISPATCH(MultiTexCoord3sARB, (target, s, t, r), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3svARB)(GLenum target, const GLshort *v)
{
   DISPATCH(MultiTexCoord3svARB, (target, v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4dARB)(GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
   DISPATCH(MultiTexCoord4dARB, (target, s, t, r, q), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4dvARB)(GLenum target, const GLdouble *v)
{
   DISPATCH(MultiTexCoord4dvARB, (target, v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4fARB)(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
   DISPATCH(MultiTexCoord4fARB, (target, s, t, r, q), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4fvARB)(GLenum target, const GLfloat *v)
{
   DISPATCH(MultiTexCoord4fvARB, (target, v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4iARB)(GLenum target, GLint s, GLint t, GLint r, GLint q)
{
   DISPATCH(MultiTexCoord4iARB, (target, s, t, r, q), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4ivARB)(GLenum target, const GLint *v)
{
   DISPATCH(MultiTexCoord4ivARB, (target, v), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4sARB)(GLenum target, GLshort s, GLshort t, GLshort r, GLshort q)
{
   DISPATCH(MultiTexCoord4sARB, (target, s, t, r, q), (F, ";"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4svARB)(GLenum target, const GLshort *v)
{
   DISPATCH(MultiTexCoord4svARB, (target, v), (F, ";"));
}



#ifdef DISPATCH_TABLE_NAME

#ifndef TABLE_ENTRY
#error TABLE_ENTRY must be defined
#endif

void *DISPATCH_TABLE_NAME[] = {
   TABLE_ENTRY(NewList),
   TABLE_ENTRY(EndList),
   TABLE_ENTRY(CallList),
   TABLE_ENTRY(CallLists),
   TABLE_ENTRY(DeleteLists),
   TABLE_ENTRY(GenLists),
   TABLE_ENTRY(ListBase),
   TABLE_ENTRY(Begin),
   TABLE_ENTRY(Bitmap),
   TABLE_ENTRY(Color3b),
   TABLE_ENTRY(Color3bv),
   TABLE_ENTRY(Color3d),
   TABLE_ENTRY(Color3dv),
   TABLE_ENTRY(Color3f),
   TABLE_ENTRY(Color3fv),
   TABLE_ENTRY(Color3i),
   TABLE_ENTRY(Color3iv),
   TABLE_ENTRY(Color3s),
   TABLE_ENTRY(Color3sv),
   TABLE_ENTRY(Color3ub),
   TABLE_ENTRY(Color3ubv),
   TABLE_ENTRY(Color3ui),
   TABLE_ENTRY(Color3uiv),
   TABLE_ENTRY(Color3us),
   TABLE_ENTRY(Color3usv),
   TABLE_ENTRY(Color4b),
   TABLE_ENTRY(Color4bv),
   TABLE_ENTRY(Color4d),
   TABLE_ENTRY(Color4dv),
   TABLE_ENTRY(Color4f),
   TABLE_ENTRY(Color4fv),
   TABLE_ENTRY(Color4i),
   TABLE_ENTRY(Color4iv),
   TABLE_ENTRY(Color4s),
   TABLE_ENTRY(Color4sv),
   TABLE_ENTRY(Color4ub),
   TABLE_ENTRY(Color4ubv),
   TABLE_ENTRY(Color4ui),
   TABLE_ENTRY(Color4uiv),
   TABLE_ENTRY(Color4us),
   TABLE_ENTRY(Color4usv),
   TABLE_ENTRY(EdgeFlag),
   TABLE_ENTRY(EdgeFlagv),
   TABLE_ENTRY(End),
   TABLE_ENTRY(Indexd),
   TABLE_ENTRY(Indexdv),
   TABLE_ENTRY(Indexf),
   TABLE_ENTRY(Indexfv),
   TABLE_ENTRY(Indexi),
   TABLE_ENTRY(Indexiv),
   TABLE_ENTRY(Indexs),
   TABLE_ENTRY(Indexsv),
   TABLE_ENTRY(Normal3b),
   TABLE_ENTRY(Normal3bv),
   TABLE_ENTRY(Normal3d),
   TABLE_ENTRY(Normal3dv),
   TABLE_ENTRY(Normal3f),
   TABLE_ENTRY(Normal3fv),
   TABLE_ENTRY(Normal3i),
   TABLE_ENTRY(Normal3iv),
   TABLE_ENTRY(Normal3s),
   TABLE_ENTRY(Normal3sv),
   TABLE_ENTRY(RasterPos2d),
   TABLE_ENTRY(RasterPos2dv),
   TABLE_ENTRY(RasterPos2f),
   TABLE_ENTRY(RasterPos2fv),
   TABLE_ENTRY(RasterPos2i),
   TABLE_ENTRY(RasterPos2iv),
   TABLE_ENTRY(RasterPos2s),
   TABLE_ENTRY(RasterPos2sv),
   TABLE_ENTRY(RasterPos3d),
   TABLE_ENTRY(RasterPos3dv),
   TABLE_ENTRY(RasterPos3f),
   TABLE_ENTRY(RasterPos3fv),
   TABLE_ENTRY(RasterPos3i),
   TABLE_ENTRY(RasterPos3iv),
   TABLE_ENTRY(RasterPos3s),
   TABLE_ENTRY(RasterPos3sv),
   TABLE_ENTRY(RasterPos4d),
   TABLE_ENTRY(RasterPos4dv),
   TABLE_ENTRY(RasterPos4f),
   TABLE_ENTRY(RasterPos4fv),
   TABLE_ENTRY(RasterPos4i),
   TABLE_ENTRY(RasterPos4iv),
   TABLE_ENTRY(RasterPos4s),
   TABLE_ENTRY(RasterPos4sv),
   TABLE_ENTRY(Rectd),
   TABLE_ENTRY(Rectdv),
   TABLE_ENTRY(Rectf),
   TABLE_ENTRY(Rectfv),
   TABLE_ENTRY(Recti),
   TABLE_ENTRY(Rectiv),
   TABLE_ENTRY(Rects),
   TABLE_ENTRY(Rectsv),
   TABLE_ENTRY(TexCoord1d),
   TABLE_ENTRY(TexCoord1dv),
   TABLE_ENTRY(TexCoord1f),
   TABLE_ENTRY(TexCoord1fv),
   TABLE_ENTRY(TexCoord1i),
   TABLE_ENTRY(TexCoord1iv),
   TABLE_ENTRY(TexCoord1s),
   TABLE_ENTRY(TexCoord1sv),
   TABLE_ENTRY(TexCoord2d),
   TABLE_ENTRY(TexCoord2dv),
   TABLE_ENTRY(TexCoord2f),
   TABLE_ENTRY(TexCoord2fv),
   TABLE_ENTRY(TexCoord2i),
   TABLE_ENTRY(TexCoord2iv),
   TABLE_ENTRY(TexCoord2s),
   TABLE_ENTRY(TexCoord2sv),
   TABLE_ENTRY(TexCoord3d),
   TABLE_ENTRY(TexCoord3dv),
   TABLE_ENTRY(TexCoord3f),
   TABLE_ENTRY(TexCoord3fv),
   TABLE_ENTRY(TexCoord3i),
   TABLE_ENTRY(TexCoord3iv),
   TABLE_ENTRY(TexCoord3s),
   TABLE_ENTRY(TexCoord3sv),
   TABLE_ENTRY(TexCoord4d),
   TABLE_ENTRY(TexCoord4dv),
   TABLE_ENTRY(TexCoord4f),
   TABLE_ENTRY(TexCoord4fv),
   TABLE_ENTRY(TexCoord4i),
   TABLE_ENTRY(TexCoord4iv),
   TABLE_ENTRY(TexCoord4s),
   TABLE_ENTRY(TexCoord4sv),
   TABLE_ENTRY(Vertex2d),
   TABLE_ENTRY(Vertex2dv),
   TABLE_ENTRY(Vertex2f),
   TABLE_ENTRY(Vertex2fv),
   TABLE_ENTRY(Vertex2i),
   TABLE_ENTRY(Vertex2iv),
   TABLE_ENTRY(Vertex2s),
   TABLE_ENTRY(Vertex2sv),
   TABLE_ENTRY(Vertex3d),
   TABLE_ENTRY(Vertex3dv),
   TABLE_ENTRY(Vertex3f),
   TABLE_ENTRY(Vertex3fv),
   TABLE_ENTRY(Vertex3i),
   TABLE_ENTRY(Vertex3iv),
   TABLE_ENTRY(Vertex3s),
   TABLE_ENTRY(Vertex3sv),
   TABLE_ENTRY(Vertex4d),
   TABLE_ENTRY(Vertex4dv),
   TABLE_ENTRY(Vertex4f),
   TABLE_ENTRY(Vertex4fv),
   TABLE_ENTRY(Vertex4i),
   TABLE_ENTRY(Vertex4iv),
   TABLE_ENTRY(Vertex4s),
   TABLE_ENTRY(Vertex4sv),
   TABLE_ENTRY(ClipPlane),
   TABLE_ENTRY(ColorMaterial),
   TABLE_ENTRY(CullFace),
   TABLE_ENTRY(Fogf),
   TABLE_ENTRY(Fogfv),
   TABLE_ENTRY(Fogi),
   TABLE_ENTRY(Fogiv),
   TABLE_ENTRY(FrontFace),
   TABLE_ENTRY(Hint),
   TABLE_ENTRY(Lightf),
   TABLE_ENTRY(Lightfv),
   TABLE_ENTRY(Lighti),
   TABLE_ENTRY(Lightiv),
   TABLE_ENTRY(LightModelf),
   TABLE_ENTRY(LightModelfv),
   TABLE_ENTRY(LightModeli),
   TABLE_ENTRY(LightModeliv),
   TABLE_ENTRY(LineStipple),
   TABLE_ENTRY(LineWidth),
   TABLE_ENTRY(Materialf),
   TABLE_ENTRY(Materialfv),
   TABLE_ENTRY(Materiali),
   TABLE_ENTRY(Materialiv),
   TABLE_ENTRY(PointSize),
   TABLE_ENTRY(PolygonMode),
   TABLE_ENTRY(PolygonStipple),
   TABLE_ENTRY(Scissor),
   TABLE_ENTRY(ShadeModel),
   TABLE_ENTRY(TexParameterf),
   TABLE_ENTRY(TexParameterfv),
   TABLE_ENTRY(TexParameteri),
   TABLE_ENTRY(TexParameteriv),
   TABLE_ENTRY(TexImage1D),
   TABLE_ENTRY(TexImage2D),
   TABLE_ENTRY(TexEnvf),
   TABLE_ENTRY(TexEnvfv),
   TABLE_ENTRY(TexEnvi),
   TABLE_ENTRY(TexEnviv),
   TABLE_ENTRY(TexGend),
   TABLE_ENTRY(TexGendv),
   TABLE_ENTRY(TexGenf),
   TABLE_ENTRY(TexGenfv),
   TABLE_ENTRY(TexGeni),
   TABLE_ENTRY(TexGeniv),
   TABLE_ENTRY(FeedbackBuffer),
   TABLE_ENTRY(SelectBuffer),
   TABLE_ENTRY(RenderMode),
   TABLE_ENTRY(InitNames),
   TABLE_ENTRY(LoadName),
   TABLE_ENTRY(PassThrough),
   TABLE_ENTRY(PopName),
   TABLE_ENTRY(PushName),
   TABLE_ENTRY(DrawBuffer),
   TABLE_ENTRY(Clear),
   TABLE_ENTRY(ClearAccum),
   TABLE_ENTRY(ClearIndex),
   TABLE_ENTRY(ClearColor),
   TABLE_ENTRY(ClearStencil),
   TABLE_ENTRY(ClearDepth),
   TABLE_ENTRY(StencilMask),
   TABLE_ENTRY(ColorMask),
   TABLE_ENTRY(DepthMask),
   TABLE_ENTRY(IndexMask),
   TABLE_ENTRY(Accum),
   TABLE_ENTRY(Disable),
   TABLE_ENTRY(Enable),
   TABLE_ENTRY(Finish),
   TABLE_ENTRY(Flush),
   TABLE_ENTRY(PopAttrib),
   TABLE_ENTRY(PushAttrib),
   TABLE_ENTRY(Map1d),
   TABLE_ENTRY(Map1f),
   TABLE_ENTRY(Map2d),
   TABLE_ENTRY(Map2f),
   TABLE_ENTRY(MapGrid1d),
   TABLE_ENTRY(MapGrid1f),
   TABLE_ENTRY(MapGrid2d),
   TABLE_ENTRY(MapGrid2f),
   TABLE_ENTRY(EvalCoord1d),
   TABLE_ENTRY(EvalCoord1dv),
   TABLE_ENTRY(EvalCoord1f),
   TABLE_ENTRY(EvalCoord1fv),
   TABLE_ENTRY(EvalCoord2d),
   TABLE_ENTRY(EvalCoord2dv),
   TABLE_ENTRY(EvalCoord2f),
   TABLE_ENTRY(EvalCoord2fv),
   TABLE_ENTRY(EvalMesh1),
   TABLE_ENTRY(EvalPoint1),
   TABLE_ENTRY(EvalMesh2),
   TABLE_ENTRY(EvalPoint2),
   TABLE_ENTRY(AlphaFunc),
   TABLE_ENTRY(BlendFunc),
   TABLE_ENTRY(LogicOp),
   TABLE_ENTRY(StencilFunc),
   TABLE_ENTRY(StencilOp),
   TABLE_ENTRY(DepthFunc),
   TABLE_ENTRY(PixelZoom),
   TABLE_ENTRY(PixelTransferf),
   TABLE_ENTRY(PixelTransferi),
   TABLE_ENTRY(PixelStoref),
   TABLE_ENTRY(PixelStorei),
   TABLE_ENTRY(PixelMapfv),
   TABLE_ENTRY(PixelMapuiv),
   TABLE_ENTRY(PixelMapusv),
   TABLE_ENTRY(ReadBuffer),
   TABLE_ENTRY(CopyPixels),
   TABLE_ENTRY(ReadPixels),
   TABLE_ENTRY(DrawPixels),
   TABLE_ENTRY(GetBooleanv),
   TABLE_ENTRY(GetClipPlane),
   TABLE_ENTRY(GetDoublev),
   TABLE_ENTRY(GetError),
   TABLE_ENTRY(GetFloatv),
   TABLE_ENTRY(GetIntegerv),
   TABLE_ENTRY(GetLightfv),
   TABLE_ENTRY(GetLightiv),
   TABLE_ENTRY(GetMapdv),
   TABLE_ENTRY(GetMapfv),
   TABLE_ENTRY(GetMapiv),
   TABLE_ENTRY(GetMaterialfv),
   TABLE_ENTRY(GetMaterialiv),
   TABLE_ENTRY(GetPixelMapfv),
   TABLE_ENTRY(GetPixelMapuiv),
   TABLE_ENTRY(GetPixelMapusv),
   TABLE_ENTRY(GetPolygonStipple),
   TABLE_ENTRY(GetString),
   TABLE_ENTRY(GetTexEnvfv),
   TABLE_ENTRY(GetTexEnviv),
   TABLE_ENTRY(GetTexGendv),
   TABLE_ENTRY(GetTexGenfv),
   TABLE_ENTRY(GetTexGeniv),
   TABLE_ENTRY(GetTexImage),
   TABLE_ENTRY(GetTexParameterfv),
   TABLE_ENTRY(GetTexParameteriv),
   TABLE_ENTRY(GetTexLevelParameterfv),
   TABLE_ENTRY(GetTexLevelParameteriv),
   TABLE_ENTRY(IsEnabled),
   TABLE_ENTRY(IsList),
   TABLE_ENTRY(DepthRange),
   TABLE_ENTRY(Frustum),
   TABLE_ENTRY(LoadIdentity),
   TABLE_ENTRY(LoadMatrixf),
   TABLE_ENTRY(LoadMatrixd),
   TABLE_ENTRY(MatrixMode),
   TABLE_ENTRY(MultMatrixf),
   TABLE_ENTRY(MultMatrixd),
   TABLE_ENTRY(Ortho),
   TABLE_ENTRY(PopMatrix),
   TABLE_ENTRY(PushMatrix),
   TABLE_ENTRY(Rotated),
   TABLE_ENTRY(Rotatef),
   TABLE_ENTRY(Scaled),
   TABLE_ENTRY(Scalef),
   TABLE_ENTRY(Translated),
   TABLE_ENTRY(Translatef),
   TABLE_ENTRY(Viewport),
   /* 1.1 */
   TABLE_ENTRY(ArrayElement),
   TABLE_ENTRY(BindTexture),
   TABLE_ENTRY(ColorPointer),
   TABLE_ENTRY(DisableClientState),
   TABLE_ENTRY(DrawArrays),
   TABLE_ENTRY(DrawElements),
   TABLE_ENTRY(EdgeFlagPointer),
   TABLE_ENTRY(EnableClientState),
   TABLE_ENTRY(IndexPointer),
   TABLE_ENTRY(Indexub),
   TABLE_ENTRY(Indexubv),
   TABLE_ENTRY(InterleavedArrays),
   TABLE_ENTRY(NormalPointer),
   TABLE_ENTRY(PolygonOffset),
   TABLE_ENTRY(TexCoordPointer),
   TABLE_ENTRY(VertexPointer),
   TABLE_ENTRY(AreTexturesResident),
   TABLE_ENTRY(CopyTexImage1D),
   TABLE_ENTRY(CopyTexImage2D),
   TABLE_ENTRY(CopyTexSubImage1D),
   TABLE_ENTRY(CopyTexSubImage2D),
   TABLE_ENTRY(DeleteTextures),
   TABLE_ENTRY(GenTextures),
   TABLE_ENTRY(GetPointerv),
   TABLE_ENTRY(IsTexture),
   TABLE_ENTRY(PrioritizeTextures),
   TABLE_ENTRY(TexSubImage1D),
   TABLE_ENTRY(TexSubImage2D),
   TABLE_ENTRY(PopClientAttrib),
   TABLE_ENTRY(PushClientAttrib),
   /* 1.2 */
   TABLE_ENTRY(BlendColor),
   TABLE_ENTRY(BlendEquation),
   TABLE_ENTRY(DrawRangeElements),
   TABLE_ENTRY(ColorTable),
   TABLE_ENTRY(ColorTableParameterfv),
   TABLE_ENTRY(ColorTableParameteriv),
   TABLE_ENTRY(CopyColorTable),
   TABLE_ENTRY(GetColorTable),
   TABLE_ENTRY(GetColorTableParameterfv),
   TABLE_ENTRY(GetColorTableParameteriv),
   TABLE_ENTRY(ColorSubTable),
   TABLE_ENTRY(CopyColorSubTable),
   TABLE_ENTRY(ConvolutionFilter1D),
   TABLE_ENTRY(ConvolutionFilter2D),
   TABLE_ENTRY(ConvolutionParameterf),
   TABLE_ENTRY(ConvolutionParameterfv),
   TABLE_ENTRY(ConvolutionParameteri),
   TABLE_ENTRY(ConvolutionParameteriv),
   TABLE_ENTRY(CopyConvolutionFilter1D),
   TABLE_ENTRY(CopyConvolutionFilter2D),
   TABLE_ENTRY(GetConvolutionFilter),
   TABLE_ENTRY(GetConvolutionParameterfv),
   TABLE_ENTRY(GetConvolutionParameteriv),
   TABLE_ENTRY(GetSeparableFilter),
   TABLE_ENTRY(SeparableFilter2D),
   TABLE_ENTRY(GetHistogram),
   TABLE_ENTRY(GetHistogramParameterfv),
   TABLE_ENTRY(GetHistogramParameteriv),
   TABLE_ENTRY(GetMinmax),
   TABLE_ENTRY(GetMinmaxParameterfv),
   TABLE_ENTRY(GetMinmaxParameteriv),
   TABLE_ENTRY(Histogram),
   TABLE_ENTRY(Minmax),
   TABLE_ENTRY(ResetHistogram),
   TABLE_ENTRY(ResetMinmax),
   TABLE_ENTRY(TexImage3D),
   TABLE_ENTRY(TexSubImage3D),
   TABLE_ENTRY(CopyTexSubImage3D),
   /* GL_ARB_multitexture */
   TABLE_ENTRY(ActiveTextureARB),
   TABLE_ENTRY(ClientActiveTextureARB),
   TABLE_ENTRY(MultiTexCoord1dARB),
   TABLE_ENTRY(MultiTexCoord1dvARB),
   TABLE_ENTRY(MultiTexCoord1fARB),
   TABLE_ENTRY(MultiTexCoord1fvARB),
   TABLE_ENTRY(MultiTexCoord1iARB),
   TABLE_ENTRY(MultiTexCoord1ivARB),
   TABLE_ENTRY(MultiTexCoord1sARB),
   TABLE_ENTRY(MultiTexCoord1svARB),
   TABLE_ENTRY(MultiTexCoord2dARB),
   TABLE_ENTRY(MultiTexCoord2dvARB),
   TABLE_ENTRY(MultiTexCoord2fARB),
   TABLE_ENTRY(MultiTexCoord2fvARB),
   TABLE_ENTRY(MultiTexCoord2iARB),
   TABLE_ENTRY(MultiTexCoord2ivARB),
   TABLE_ENTRY(MultiTexCoord2sARB),
   TABLE_ENTRY(MultiTexCoord2svARB),
   TABLE_ENTRY(MultiTexCoord3dARB),
   TABLE_ENTRY(MultiTexCoord3dvARB),
   TABLE_ENTRY(MultiTexCoord3fARB),
   TABLE_ENTRY(MultiTexCoord3fvARB),
   TABLE_ENTRY(MultiTexCoord3iARB),
   TABLE_ENTRY(MultiTexCoord3ivARB),
   TABLE_ENTRY(MultiTexCoord3sARB),
   TABLE_ENTRY(MultiTexCoord3svARB),
   TABLE_ENTRY(MultiTexCoord4dARB),
   TABLE_ENTRY(MultiTexCoord4dvARB),
   TABLE_ENTRY(MultiTexCoord4fARB),
   TABLE_ENTRY(MultiTexCoord4fvARB),
   TABLE_ENTRY(MultiTexCoord4iARB),
   TABLE_ENTRY(MultiTexCoord4ivARB),
   TABLE_ENTRY(MultiTexCoord4sARB),
   TABLE_ENTRY(MultiTexCoord4svARB),
   /* GL_ARB_transpose_matrix */
   TABLE_ENTRY(LoadTransposeMatrixfARB),
   TABLE_ENTRY(LoadTransposeMatrixdARB),
   TABLE_ENTRY(MultTransposeMatrixfARB),
   TABLE_ENTRY(MultTransposeMatrixdARB),
   /* GL_ARB_multisample */
   TABLE_ENTRY(SampleCoverageARB),
   TABLE_ENTRY(SamplePassARB),
   /* GL_EXT_blend_color */
   /* GL_EXT_polygon_offset */
   TABLE_ENTRY(PolygonOffsetEXT),
   /* GL_EXT_texture3D */
   /* GL_EXT_subtexture */
   /* GL_SGIS_texture_filter4 */
   TABLE_ENTRY(GetTexFilterFuncSGIS),
   TABLE_ENTRY(TexFilterFuncSGIS),
   /* GL_EXT_subtexture */
   /* GL_EXT_copy_texture */
   /* GL_EXT_histogram */
   TABLE_ENTRY(GetHistogramEXT),
   TABLE_ENTRY(GetHistogramParameterfvEXT),
   TABLE_ENTRY(GetHistogramParameterivEXT),
   TABLE_ENTRY(GetMinmaxEXT),
   TABLE_ENTRY(GetMinmaxParameterfvEXT),
   TABLE_ENTRY(GetMinmaxParameterivEXT),
   /* GL_EXT_convolution */
   TABLE_ENTRY(GetConvolutionFilterEXT),
   TABLE_ENTRY(GetConvolutionParameterfvEXT),
   TABLE_ENTRY(GetConvolutionParameterivEXT),
   TABLE_ENTRY(GetSeparableFilterEXT),
   /* GL_SGI_color_table */
   TABLE_ENTRY(GetColorTableSGI),
   TABLE_ENTRY(GetColorTableParameterfvSGI),
   TABLE_ENTRY(GetColorTableParameterivSGI),
   /* GL_SGIX_pixel_texture */
   TABLE_ENTRY(PixelTexGenSGIX),
   /* GL_SGIS_pixel_texture */
   TABLE_ENTRY(PixelTexGenParameteriSGIS),
   TABLE_ENTRY(PixelTexGenParameterivSGIS),
   TABLE_ENTRY(PixelTexGenParameterfSGIS),
   TABLE_ENTRY(PixelTexGenParameterfvSGIS),
   TABLE_ENTRY(GetPixelTexGenParameterivSGIS),
   TABLE_ENTRY(GetPixelTexGenParameterfvSGIS),
   /* GL_SGIS_texture4D */
   TABLE_ENTRY(TexImage4DSGIS),
   TABLE_ENTRY(TexSubImage4DSGIS),
   /* GL_EXT_texture_object */
   TABLE_ENTRY(AreTexturesResidentEXT),
   TABLE_ENTRY(GenTexturesEXT),
   TABLE_ENTRY(IsTextureEXT),
   /* GL_SGIS_detail_texture */
   TABLE_ENTRY(DetailTexFuncSGIS),
   TABLE_ENTRY(GetDetailTexFuncSGIS),
   /* GL_SGIS_sharpen_texture */
   TABLE_ENTRY(SharpenTexFuncSGIS),
   TABLE_ENTRY(GetSharpenTexFuncSGIS),
   /* GL_SGIS_multisample */
   TABLE_ENTRY(SampleMaskSGIS),
   TABLE_ENTRY(SamplePatternSGIS),
   /* GL_EXT_vertex_array */
   TABLE_ENTRY(ColorPointerEXT),
   TABLE_ENTRY(EdgeFlagPointerEXT),
   TABLE_ENTRY(IndexPointerEXT),
   TABLE_ENTRY(NormalPointerEXT),
   TABLE_ENTRY(TexCoordPointerEXT),
   TABLE_ENTRY(VertexPointerEXT),
   /* GL_EXT_blend_minmax */
   /* GL_SGIX_sprite */
   TABLE_ENTRY(SpriteParameterfSGIX),
   TABLE_ENTRY(SpriteParameterfvSGIX),
   TABLE_ENTRY(SpriteParameteriSGIX),
   TABLE_ENTRY(SpriteParameterivSGIX),
   /* GL_EXT_point_parameters */
   TABLE_ENTRY(PointParameterfEXT),
   TABLE_ENTRY(PointParameterfvEXT),
   /* GL_SGIX_instruments */
   TABLE_ENTRY(GetInstrumentsSGIX),
   TABLE_ENTRY(InstrumentsBufferSGIX),
   TABLE_ENTRY(PollInstrumentsSGIX),
   TABLE_ENTRY(ReadInstrumentsSGIX),
   TABLE_ENTRY(StartInstrumentsSGIX),
   TABLE_ENTRY(StopInstrumentsSGIX),
   /* GL_SGIX_framezoom */
   TABLE_ENTRY(FrameZoomSGIX),
   /* GL_SGIX_tag_sample_buffer */
   TABLE_ENTRY(TagSampleBufferSGIX),
   /* GL_SGIX_reference_plane */
   TABLE_ENTRY(ReferencePlaneSGIX),
   /* GL_SGIX_flush_raster */
   TABLE_ENTRY(FlushRasterSGIX),
   /* GL_SGIX_list_priority */
   TABLE_ENTRY(GetListParameterfvSGIX),
   TABLE_ENTRY(GetListParameterivSGIX),
   TABLE_ENTRY(ListParameterfSGIX),
   TABLE_ENTRY(ListParameterfvSGIX),
   TABLE_ENTRY(ListParameteriSGIX),
   TABLE_ENTRY(ListParameterivSGIX),
   /* GL_SGIX_fragment_lighting */
   TABLE_ENTRY(FragmentColorMaterialSGIX),
   TABLE_ENTRY(FragmentLightfSGIX),
   TABLE_ENTRY(FragmentLightfvSGIX),
   TABLE_ENTRY(FragmentLightiSGIX),
   TABLE_ENTRY(FragmentLightivSGIX),
   TABLE_ENTRY(FragmentLightModelfSGIX),
   TABLE_ENTRY(FragmentLightModelfvSGIX),
   TABLE_ENTRY(FragmentLightModeliSGIX),
   TABLE_ENTRY(FragmentLightModelivSGIX),
   TABLE_ENTRY(FragmentMaterialfSGIX),
   TABLE_ENTRY(FragmentMaterialfvSGIX),
   TABLE_ENTRY(FragmentMaterialiSGIX),
   TABLE_ENTRY(FragmentMaterialivSGIX),
   TABLE_ENTRY(GetFragmentLightfvSGIX),
   TABLE_ENTRY(GetFragmentLightivSGIX),
   TABLE_ENTRY(GetFragmentMaterialfvSGIX),
   TABLE_ENTRY(GetFragmentMaterialivSGIX),
   TABLE_ENTRY(LightEnviSGIX),
   /* GL_EXT_vertex_weighting */
   TABLE_ENTRY(VertexWeightfEXT),
   TABLE_ENTRY(VertexWeightfvEXT),
   TABLE_ENTRY(VertexWeightPointerEXT),
   /* GL_NV_vertex_array_range */
   TABLE_ENTRY(FlushVertexArrayRangeNV),
   TABLE_ENTRY(VertexArrayRangeNV),
   /* GL_NV_register_combiners */
   TABLE_ENTRY(CombinerParameterfvNV),
   TABLE_ENTRY(CombinerParameterfNV),
   TABLE_ENTRY(CombinerParameterivNV),
   TABLE_ENTRY(CombinerParameteriNV),
   TABLE_ENTRY(CombinerInputNV),
   TABLE_ENTRY(CombinerOutputNV),
   TABLE_ENTRY(FinalCombinerInputNV),
   TABLE_ENTRY(GetCombinerInputParameterfvNV),
   TABLE_ENTRY(GetCombinerInputParameterivNV),
   TABLE_ENTRY(GetCombinerOutputParameterfvNV),
   TABLE_ENTRY(GetCombinerOutputParameterivNV),
   TABLE_ENTRY(GetFinalCombinerInputParameterfvNV),
   TABLE_ENTRY(GetFinalCombinerInputParameterivNV),
   /* GL_MESA_resize_buffers */
   TABLE_ENTRY(ResizeBuffersMESA),
   /* GL_MESA_window_pos */
   TABLE_ENTRY(WindowPos2dMESA),
   TABLE_ENTRY(WindowPos2dvMESA),
   TABLE_ENTRY(WindowPos2fMESA),
   TABLE_ENTRY(WindowPos2fvMESA),
   TABLE_ENTRY(WindowPos2iMESA),
   TABLE_ENTRY(WindowPos2ivMESA),
   TABLE_ENTRY(WindowPos2sMESA),
   TABLE_ENTRY(WindowPos2svMESA),
   TABLE_ENTRY(WindowPos3dMESA),
   TABLE_ENTRY(WindowPos3dvMESA),
   TABLE_ENTRY(WindowPos3fMESA),
   TABLE_ENTRY(WindowPos3fvMESA),
   TABLE_ENTRY(WindowPos3iMESA),
   TABLE_ENTRY(WindowPos3ivMESA),
   TABLE_ENTRY(WindowPos3sMESA),
   TABLE_ENTRY(WindowPos3svMESA),
   TABLE_ENTRY(WindowPos4dMESA),
   TABLE_ENTRY(WindowPos4dvMESA),
   TABLE_ENTRY(WindowPos4fMESA),
   TABLE_ENTRY(WindowPos4fvMESA),
   TABLE_ENTRY(WindowPos4iMESA),
   TABLE_ENTRY(WindowPos4ivMESA),
   TABLE_ENTRY(WindowPos4sMESA),
   TABLE_ENTRY(WindowPos4svMESA),
   /* GL_EXT_draw_range_elements */
   TABLE_ENTRY(BlendFuncSeparateEXT),
   /* GL_EXT_index_material */
   TABLE_ENTRY(IndexMaterialEXT),
   /* GL_EXT_index_func */
   TABLE_ENTRY(IndexFuncEXT),
   /* GL_EXT_compiled_vertex_array */
   TABLE_ENTRY(LockArraysEXT),
   TABLE_ENTRY(UnlockArraysEXT),
   /* GL_EXT_cull_vertex */
   TABLE_ENTRY(CullParameterdvEXT),
   TABLE_ENTRY(CullParameterfvEXT),
   /* GL_PGI_misc_hints */
   TABLE_ENTRY(HintPGI),
   /* GL_EXT_fog_coord */
   TABLE_ENTRY(FogCoordfEXT),
   TABLE_ENTRY(FogCoordfvEXT),
   TABLE_ENTRY(FogCoorddEXT),
   TABLE_ENTRY(FogCoorddvEXT),
   TABLE_ENTRY(FogCoordPointerEXT),
   /* GL_EXT_color_table */
   TABLE_ENTRY(GetColorTableEXT),
   TABLE_ENTRY(GetColorTableParameterivEXT),
   TABLE_ENTRY(GetColorTableParameterfvEXT),
   /* GL_3DFX_tbuffer */
   TABLE_ENTRY(TbufferMask3DFX),
   /* GL_ARB_texture_compression */
   TABLE_ENTRY(CompressedTexImage3DARB),
   TABLE_ENTRY(CompressedTexImage2DARB),
   TABLE_ENTRY(CompressedTexImage1DARB),
   TABLE_ENTRY(CompressedTexSubImage3DARB),
   TABLE_ENTRY(CompressedTexSubImage2DARB),
   TABLE_ENTRY(CompressedTexSubImage1DARB),
   TABLE_ENTRY(GetCompressedTexImageARB),
   /* GL_EXT_secondary_color */
   TABLE_ENTRY(SecondaryColor3bEXT),
   TABLE_ENTRY(SecondaryColor3bvEXT),
   TABLE_ENTRY(SecondaryColor3dEXT),
   TABLE_ENTRY(SecondaryColor3dvEXT),
   TABLE_ENTRY(SecondaryColor3fEXT),
   TABLE_ENTRY(SecondaryColor3fvEXT),
   TABLE_ENTRY(SecondaryColor3iEXT),
   TABLE_ENTRY(SecondaryColor3ivEXT),
   TABLE_ENTRY(SecondaryColor3sEXT),
   TABLE_ENTRY(SecondaryColor3svEXT),
   TABLE_ENTRY(SecondaryColor3ubEXT),
   TABLE_ENTRY(SecondaryColor3ubvEXT),
   TABLE_ENTRY(SecondaryColor3uiEXT),
   TABLE_ENTRY(SecondaryColor3uivEXT),
   TABLE_ENTRY(SecondaryColor3usEXT),
   TABLE_ENTRY(SecondaryColor3usvEXT),
   TABLE_ENTRY(SecondaryColorPointerEXT),
   /* A whole bunch of no-op functions.  These might be called
    * when someone tries to call a dynamically-registered extension
    * function without a current rendering context.
    */
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused)
};
#endif /* DISPATCH_TABLE_NAME */



/*
 * This is just used to silence compiler warnings.
 * We list the functions which aren't otherwise used.
 */
#ifdef UNUSED_TABLE_NAME
void *UNUSED_TABLE_NAME[] = {
   TABLE_ENTRY(BlendColorEXT),
   TABLE_ENTRY(CopyTexSubImage3DEXT),
   TABLE_ENTRY(TexImage3DEXT),
   TABLE_ENTRY(TexSubImage3DEXT),
   TABLE_ENTRY(CopyTexSubImage1DEXT),
   TABLE_ENTRY(TexSubImage1DEXT),
   TABLE_ENTRY(TexSubImage2DEXT),
   TABLE_ENTRY(CopyTexImage1DEXT),
   TABLE_ENTRY(CopyTexImage2DEXT),
   TABLE_ENTRY(CopyTexSubImage2DEXT),
   TABLE_ENTRY(HistogramEXT),
   TABLE_ENTRY(MinmaxEXT),
   TABLE_ENTRY(ResetHistogramEXT),
   TABLE_ENTRY(ResetMinmaxEXT),
   TABLE_ENTRY(ConvolutionFilter1DEXT),
   TABLE_ENTRY(ConvolutionFilter2DEXT),
   TABLE_ENTRY(ConvolutionParameterfEXT),
   TABLE_ENTRY(ConvolutionParameterfvEXT),
   TABLE_ENTRY(ConvolutionParameteriEXT),
   TABLE_ENTRY(ConvolutionParameterivEXT),
   TABLE_ENTRY(CopyConvolutionFilter1DEXT),
   TABLE_ENTRY(CopyConvolutionFilter2DEXT),
   TABLE_ENTRY(SeparableFilter2DEXT),
   TABLE_ENTRY(ColorTableParameterfvSGI),
   TABLE_ENTRY(ColorTableParameterivSGI),
   TABLE_ENTRY(ColorTableSGI),
   TABLE_ENTRY(CopyColorTableSGI),
   TABLE_ENTRY(DeleteTexturesEXT),
   TABLE_ENTRY(BindTextureEXT),
   TABLE_ENTRY(PrioritizeTexturesEXT),
   TABLE_ENTRY(GetPointervEXT),
   TABLE_ENTRY(ArrayElementEXT),
   TABLE_ENTRY(DrawArraysEXT),
   TABLE_ENTRY(BlendEquationEXT),
   TABLE_ENTRY(PointParameterfSGIS),
   TABLE_ENTRY(PointParameterfvSGIS),
   TABLE_ENTRY(ColorSubTableEXT),
   TABLE_ENTRY(CopyColorSubTableEXT),
   TABLE_ENTRY(ColorTableEXT),
   TABLE_ENTRY(DrawRangeElementsEXT),
   TABLE_ENTRY(BlendFuncSeparateINGR),
   TABLE_ENTRY(SampleMaskEXT),
   TABLE_ENTRY(SamplePatternEXT)
};
#endif /*UNUSED_TABLE_NAME*/


#undef KEYWORD1
#undef KEYWORD2
#undef NAME
#undef DISPATCH
#undef RETURN_DISPATCH
#undef DISPATCH_TABLE_NAME
#undef UNUSED_TABLE_NAME
#undef TABLE_ENTRY

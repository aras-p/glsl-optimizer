/* $Id: glapi.c,v 1.7 1999/11/12 18:57:50 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.3
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


#include <assert.h>
#include <stdlib.h>  /* to get NULL */
#include "glapi.h"
#include "glapinoop.h"



#if defined(MULTI_THREAD)

/* XXX to do */

#else

static struct _glapi_table *Dispatch = &__glapi_noop_table;
#define DISPATCH(FUNC) (Dispatch->FUNC)

#endif


void GLAPIENTRY glAccum(GLenum op, GLfloat value)
{
   DISPATCH(Accum)(op, value);
}

void GLAPIENTRY glAlphaFunc(GLenum func, GLclampf ref)
{
   DISPATCH(AlphaFunc)(func, ref);
}

void GLAPIENTRY glBegin(GLenum mode)
{
   DISPATCH(Begin)(mode);
}

void GLAPIENTRY glBitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap)
{
   DISPATCH(Bitmap)(width, height, xorig, yorig, xmove, ymove, bitmap);
}

void GLAPIENTRY glBlendFunc(GLenum sfactor, GLenum dfactor)
{
   DISPATCH(BlendFunc)(sfactor, dfactor);
}

void GLAPIENTRY glCallList(GLuint list)
{
   DISPATCH(CallList)(list);
}

void GLAPIENTRY glCallLists(GLsizei n, GLenum type, const GLvoid *lists)
{
   DISPATCH(CallLists)(n, type, lists);
}

void GLAPIENTRY glClear(GLbitfield mask)
{
   DISPATCH(Clear)(mask);
}

void GLAPIENTRY glClearAccum(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
   DISPATCH(ClearAccum)(red, green, blue, alpha);
}

void GLAPIENTRY glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
   DISPATCH(ClearColor)(red, green, blue, alpha);
}

void GLAPIENTRY glClearDepth(GLclampd depth)
{
   DISPATCH(ClearDepth)(depth);
}

void GLAPIENTRY glClearIndex(GLfloat c)
{
   DISPATCH(ClearIndex)(c);
}

void GLAPIENTRY glClearStencil(GLint s)
{
   DISPATCH(ClearStencil)(s);
}

void GLAPIENTRY glClipPlane(GLenum plane, const GLdouble *equation)
{
   DISPATCH(ClipPlane)(plane, equation);
}

void GLAPIENTRY glColor3b(GLbyte red, GLbyte green, GLbyte blue)
{
   DISPATCH(Color3b)(red, green, blue);
}

void GLAPIENTRY glColor3d(GLdouble red, GLdouble green, GLdouble blue)
{
   DISPATCH(Color3d)(red, green, blue);
}

void GLAPIENTRY glColor3f(GLfloat red, GLfloat green, GLfloat blue)
{
   DISPATCH(Color3f)(red, green, blue);
}

void GLAPIENTRY glColor3i(GLint red, GLint green, GLint blue)
{
   DISPATCH(Color3i)(red, green, blue);
}

void GLAPIENTRY glColor3s(GLshort red, GLshort green, GLshort blue)
{
   DISPATCH(Color3s)(red, green, blue);
}

void GLAPIENTRY glColor3ub(GLubyte red, GLubyte green, GLubyte blue)
{
   DISPATCH(Color3ub)(red, green, blue);
}

void GLAPIENTRY glColor3ui(GLuint red, GLuint green, GLuint blue)
{
   DISPATCH(Color3ui)(red, green, blue);
}

void GLAPIENTRY glColor3us(GLushort red, GLushort green, GLushort blue)
{
   DISPATCH(Color3us)(red, green, blue);
}

void GLAPIENTRY glColor4b(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha)
{
   DISPATCH(Color4b)(red, green, blue, alpha);
}

void GLAPIENTRY glColor4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha)
{
   DISPATCH(Color4d)(red, green, blue, alpha);
}

void GLAPIENTRY glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
   DISPATCH(Color4f)(red, green, blue, alpha);
}

void GLAPIENTRY glColor4i(GLint red, GLint green, GLint blue, GLint alpha)
{
   DISPATCH(Color4i)(red, green, blue, alpha);
}

void GLAPIENTRY glColor4s(GLshort red, GLshort green, GLshort blue, GLshort alpha)
{
   DISPATCH(Color4s)(red, green, blue, alpha);
}

void GLAPIENTRY glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
   DISPATCH(Color4ub)(red, green, blue, alpha);
}

void GLAPIENTRY glColor4ui(GLuint red, GLuint green, GLuint blue, GLuint alpha)
{
   DISPATCH(Color4ui)(red, green, blue, alpha);
}

void GLAPIENTRY glColor4us(GLushort red, GLushort green, GLushort blue, GLushort alpha)
{
   DISPATCH(Color4us)(red, green, blue, alpha);
}

void GLAPIENTRY glColor3bv(const GLbyte *v)
{
   DISPATCH(Color3bv)(v);
}

void GLAPIENTRY glColor3dv(const GLdouble *v)
{
   DISPATCH(Color3dv)(v);
}

void GLAPIENTRY glColor3fv(const GLfloat *v)
{
   DISPATCH(Color3fv)(v);
}

void GLAPIENTRY glColor3iv(const GLint *v)
{
   DISPATCH(Color3iv)(v);
}

void GLAPIENTRY glColor3sv(const GLshort *v)
{
   DISPATCH(Color3sv)(v);
}

void GLAPIENTRY glColor3ubv(const GLubyte *v)
{
   DISPATCH(Color3ubv)(v);
}

void GLAPIENTRY glColor3uiv(const GLuint *v)
{
   DISPATCH(Color3uiv)(v);
}

void GLAPIENTRY glColor3usv(const GLushort *v)
{
   DISPATCH(Color3usv)(v);
}

void GLAPIENTRY glColor4bv(const GLbyte *v)
{
   DISPATCH(Color4bv)(v);
}

void GLAPIENTRY glColor4dv(const GLdouble *v)
{
   DISPATCH(Color4dv)(v);
}

void GLAPIENTRY glColor4fv(const GLfloat *v)
{
   DISPATCH(Color4fv)(v);
}

void GLAPIENTRY glColor4iv(const GLint *v)
{
   DISPATCH(Color4iv)(v);
}

void GLAPIENTRY glColor4sv(const GLshort *v)
{
   DISPATCH(Color4sv)(v);
}

void GLAPIENTRY glColor4ubv(const GLubyte *v)
{
   DISPATCH(Color4ubv)(v);
}

void GLAPIENTRY glColor4uiv(const GLuint *v)
{
   DISPATCH(Color4uiv)(v);
}

void GLAPIENTRY glColor4usv(const GLushort *v)
{
   DISPATCH(Color4usv)(v);
}

void GLAPIENTRY glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
   DISPATCH(ColorMask)(red, green, blue, alpha);
}

void GLAPIENTRY glColorMaterial(GLenum face, GLenum mode)
{
   DISPATCH(ColorMaterial)(face, mode);
}

void GLAPIENTRY glCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
{
   DISPATCH(CopyPixels)(x, y, width, height, type);
}

void GLAPIENTRY glCullFace(GLenum mode)
{
   DISPATCH(CullFace)(mode);
}

void GLAPIENTRY glDepthFunc(GLenum func)
{
   DISPATCH(DepthFunc)(func);
}

void GLAPIENTRY glDepthMask(GLboolean flag)
{
   DISPATCH(DepthMask)(flag);
}

void GLAPIENTRY glDepthRange(GLclampd nearVal, GLclampd farVal)
{
   DISPATCH(DepthRange)(nearVal, farVal);
}

void GLAPIENTRY glDeleteLists(GLuint list, GLsizei range)
{
   DISPATCH(DeleteLists)(list, range);
}

void GLAPIENTRY glDisable(GLenum cap)
{
   DISPATCH(Disable)(cap);
}

void GLAPIENTRY glDrawBuffer(GLenum mode)
{
   DISPATCH(DrawBuffer)(mode);
}

void GLAPIENTRY glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)
{
   DISPATCH(DrawElements)(mode, count, type, indices);
}

void GLAPIENTRY glDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
   DISPATCH(DrawPixels)(width, height, format, type, pixels);
}

void GLAPIENTRY glEnable(GLenum mode)
{
   DISPATCH(Enable)(mode);
}

void GLAPIENTRY glEnd(void)
{
   DISPATCH(End)();
}

void GLAPIENTRY glEndList(void)
{
   DISPATCH(EndList)();
}

void GLAPIENTRY glEvalCoord1d(GLdouble u)
{
   DISPATCH(EvalCoord1d)(u);
}

void GLAPIENTRY glEvalCoord1f(GLfloat u)
{
   DISPATCH(EvalCoord1f)(u);
}

void GLAPIENTRY glEvalCoord1dv(const GLdouble *u)
{
   DISPATCH(EvalCoord1dv)(u);
}

void GLAPIENTRY glEvalCoord1fv(const GLfloat *u)
{
   DISPATCH(EvalCoord1fv)(u);
}

void GLAPIENTRY glEvalCoord2d(GLdouble u, GLdouble v)
{
   DISPATCH(EvalCoord2d)(u, v);
}

void GLAPIENTRY glEvalCoord2f(GLfloat u, GLfloat v)
{
   DISPATCH(EvalCoord2f)(u, v);
}

void GLAPIENTRY glEvalCoord2dv(const GLdouble *u)
{
   DISPATCH(EvalCoord2dv)(u);
}

void GLAPIENTRY glEvalCoord2fv(const GLfloat *u)
{
   DISPATCH(EvalCoord2fv)(u);
}

void GLAPIENTRY glEvalPoint1(GLint i)
{
   DISPATCH(EvalPoint1)(i);
}

void GLAPIENTRY glEvalPoint2(GLint i, GLint j)
{
   DISPATCH(EvalPoint2)(i, j);
}

void GLAPIENTRY glEvalMesh1(GLenum mode, GLint i1, GLint i2)
{
   DISPATCH(EvalMesh1)(mode, i1, i2);
}

void GLAPIENTRY glEdgeFlag(GLboolean flag)
{
   DISPATCH(EdgeFlag)(flag);
}

void GLAPIENTRY glEdgeFlagv(const GLboolean *flag)
{
   DISPATCH(EdgeFlagv)(flag);
}

void GLAPIENTRY glEvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2)
{
   DISPATCH(EvalMesh2)(mode, i1, i2, j1, j2);
}

void GLAPIENTRY glFeedbackBuffer(GLsizei size, GLenum type, GLfloat *buffer)
{
   DISPATCH(FeedbackBuffer)(size, type, buffer);
}

void GLAPIENTRY glFinish(void)
{
   DISPATCH(Finish)();
}

void GLAPIENTRY glFlush(void)
{
   DISPATCH(Flush)();
}

void GLAPIENTRY glFogf(GLenum pname, GLfloat param)
{
   DISPATCH(Fogf)(pname, param);
}

void GLAPIENTRY glFogi(GLenum pname, GLint param)
{
   DISPATCH(Fogi)(pname, param);
}

void GLAPIENTRY glFogfv(GLenum pname, const GLfloat *params)
{
   DISPATCH(Fogfv)(pname, params);
}

void GLAPIENTRY glFogiv(GLenum pname, const GLint *params)
{
   DISPATCH(Fogiv)(pname, params);
}

void GLAPIENTRY glFrontFace(GLenum mode)
{
   DISPATCH(FrontFace)(mode);
}

void GLAPIENTRY glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble nearval, GLdouble farval)
{
   DISPATCH(Frustum)(left, right, bottom, top, nearval, farval);
}

GLuint GLAPIENTRY glGenLists(GLsizei range)
{
   return DISPATCH(GenLists)(range);
}

void GLAPIENTRY glGetBooleanv(GLenum pname, GLboolean *params)
{
   DISPATCH(GetBooleanv)(pname, params);
}

void GLAPIENTRY glGetClipPlane(GLenum plane, GLdouble *equation)
{
   DISPATCH(GetClipPlane)(plane, equation);
}

void GLAPIENTRY glGetDoublev(GLenum pname, GLdouble *params)
{
   DISPATCH(GetDoublev)(pname, params);
}

GLenum GLAPIENTRY glGetError(void)
{
   return DISPATCH(GetError)();
}

void GLAPIENTRY glGetFloatv(GLenum pname, GLfloat *params)
{
   DISPATCH(GetFloatv)(pname, params);
}

void GLAPIENTRY glGetIntegerv(GLenum pname, GLint *params)
{
   DISPATCH(GetIntegerv)(pname, params);
}

void GLAPIENTRY glGetLightfv(GLenum light, GLenum pname, GLfloat *params)
{
   DISPATCH(GetLightfv)(light, pname, params);
}

void GLAPIENTRY glGetLightiv(GLenum light, GLenum pname, GLint *params)
{
   DISPATCH(GetLightiv)(light, pname, params);
}

void GLAPIENTRY glGetMapdv(GLenum target, GLenum query, GLdouble *v)
{
   DISPATCH(GetMapdv)(target, query, v);
}

void GLAPIENTRY glGetMapfv(GLenum target, GLenum query, GLfloat *v)
{
   DISPATCH(GetMapfv)(target, query, v);
}

void GLAPIENTRY glGetMapiv(GLenum target, GLenum query, GLint *v)
{
   DISPATCH(GetMapiv)(target, query, v);
}

void GLAPIENTRY glGetMaterialfv(GLenum face, GLenum pname, GLfloat *params)
{
   DISPATCH(GetMaterialfv)(face, pname, params);
}

void GLAPIENTRY glGetMaterialiv(GLenum face, GLenum pname, GLint *params)
{
   DISPATCH(GetMaterialiv)(face, pname, params);
}

void GLAPIENTRY glGetPixelMapfv(GLenum map, GLfloat *values)
{
   DISPATCH(GetPixelMapfv)(map, values);
}

void GLAPIENTRY glGetPixelMapuiv(GLenum map, GLuint *values)
{
   DISPATCH(GetPixelMapuiv)(map, values);
}

void GLAPIENTRY glGetPixelMapusv(GLenum map, GLushort *values)
{
   DISPATCH(GetPixelMapusv)(map, values);
}

void GLAPIENTRY glGetPolygonStipple(GLubyte *mask)
{
   DISPATCH(GetPolygonStipple)(mask);
}

const GLubyte * GLAPIENTRY glGetString(GLenum name)
{
   return DISPATCH(GetString)(name);
}

void GLAPIENTRY glGetTexEnvfv(GLenum target, GLenum pname, GLfloat *params)
{
   DISPATCH(GetTexEnvfv)(target, pname, params);
}

void GLAPIENTRY glGetTexEnviv(GLenum target, GLenum pname, GLint *params)
{
   DISPATCH(GetTexEnviv)(target, pname, params);
}

void GLAPIENTRY glGetTexGeniv(GLenum target, GLenum pname, GLint *params)
{
   DISPATCH(GetTexGeniv)(target, pname, params);
}

void GLAPIENTRY glGetTexGendv(GLenum target, GLenum pname, GLdouble *params)
{
   DISPATCH(GetTexGendv)(target, pname, params);
}

void GLAPIENTRY glGetTexGenfv(GLenum target, GLenum pname, GLfloat *params)
{
   DISPATCH(GetTexGenfv)(target, pname, params);
}

void GLAPIENTRY glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels)
{
   DISPATCH(GetTexImage)(target, level, format, type, pixels);
}

void GLAPIENTRY glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat *params)
{
   DISPATCH(GetTexLevelParameterfv)(target, level, pname, params);
}

void GLAPIENTRY glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint *params)
{
   DISPATCH(GetTexLevelParameteriv)(target, level, pname, params);
}

void GLAPIENTRY glGetTexParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
   DISPATCH(GetTexParameterfv)(target, pname, params);
}

void GLAPIENTRY glGetTexParameteriv(GLenum target, GLenum pname, GLint *params)
{
   DISPATCH(GetTexParameteriv)(target, pname, params);
}

void GLAPIENTRY glHint(GLenum target, GLenum mode)
{
   DISPATCH(Hint)(target, mode);
}

void GLAPIENTRY glIndexd(GLdouble c)
{
   DISPATCH(Indexd)(c);
}

void GLAPIENTRY glIndexdv(const GLdouble *c)
{
   DISPATCH(Indexdv)(c);
}

void GLAPIENTRY glIndexf(GLfloat c)
{
   DISPATCH(Indexf)(c);
}

void GLAPIENTRY glIndexfv(const GLfloat *c)
{
   DISPATCH(Indexfv)(c);
}

void GLAPIENTRY glIndexi(GLint c)
{
   DISPATCH(Indexi)(c);
}

void GLAPIENTRY glIndexiv(const GLint *c)
{
   DISPATCH(Indexiv)(c);
}

void GLAPIENTRY glIndexs(GLshort c)
{
   DISPATCH(Indexs)(c);
}

void GLAPIENTRY glIndexsv(const GLshort *c)
{
   DISPATCH(Indexsv)(c);
}

void GLAPIENTRY glIndexub(GLubyte c)
{
   DISPATCH(Indexub)(c);
}

void GLAPIENTRY glIndexubv(const GLubyte *c)
{
   DISPATCH(Indexubv)(c);
}

void GLAPIENTRY glIndexMask(GLuint mask)
{
   DISPATCH(IndexMask)(mask);
}

void GLAPIENTRY glInitNames(void)
{
   DISPATCH(InitNames)();
}

void GLAPIENTRY glInterleavedArrays(GLenum format, GLsizei stride, const GLvoid *pointer)
{
   DISPATCH(InterleavedArrays)(format, stride, pointer);
}

GLboolean GLAPIENTRY glIsEnabled(GLenum cap)
{
   return DISPATCH(IsEnabled)(cap);
}

GLboolean GLAPIENTRY glIsList(GLuint list)
{
   return DISPATCH(IsList)(list);
}

GLboolean GLAPIENTRY glIsTexture(GLuint texture)
{
   return DISPATCH(IsTexture)(texture);
}

void GLAPIENTRY glLightf(GLenum light, GLenum pname, GLfloat param)
{
   DISPATCH(Lightf)(light, pname, param);
}

void GLAPIENTRY glLighti(GLenum light, GLenum pname, GLint param)
{
   DISPATCH(Lighti)(light, pname, param);
}

void GLAPIENTRY glLightfv(GLenum light, GLenum pname, const GLfloat *params)
{
   DISPATCH(Lightfv)(light, pname, params);
}

void GLAPIENTRY glLightiv(GLenum light, GLenum pname, const GLint *params)
{
   DISPATCH(Lightiv)(light, pname, params);
}

void GLAPIENTRY glLightModelf(GLenum pname, GLfloat param)
{
   DISPATCH(LightModelf)(pname, param);
}

void GLAPIENTRY glLightModeli(GLenum pname, GLint param)
{
   DISPATCH(LightModeli)(pname, param);
}

void GLAPIENTRY glLightModelfv(GLenum pname, const GLfloat *params)
{
   DISPATCH(LightModelfv)(pname, params);
}

void GLAPIENTRY glLightModeliv(GLenum pname, const GLint *params)
{
   DISPATCH(LightModeliv)(pname, params);
}

void GLAPIENTRY glLineWidth(GLfloat width)
{
   DISPATCH(LineWidth)(width);
}

void GLAPIENTRY glLineStipple(GLint factor, GLushort pattern)
{
   DISPATCH(LineStipple)(factor, pattern);
}

void GLAPIENTRY glListBase(GLuint base)
{
   DISPATCH(ListBase)(base);
}

void GLAPIENTRY glLoadIdentity(void)
{
   DISPATCH(LoadIdentity)();
}

void GLAPIENTRY glLoadMatrixd(const GLdouble *m)
{
   DISPATCH(LoadMatrixd)(m);
}

void GLAPIENTRY glLoadMatrixf(const GLfloat *m)
{
   DISPATCH(LoadMatrixf)(m);
}

void GLAPIENTRY glLoadName(GLuint name)
{
   DISPATCH(LoadName)(name);
}

void GLAPIENTRY glLogicOp(GLenum opcode)
{
   DISPATCH(LogicOp)(opcode);
}

void GLAPIENTRY glMap1d(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points)
{
   DISPATCH(Map1d)(target, u1, u2, stride, order, points);
}

void GLAPIENTRY glMap1f(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points)
{
   DISPATCH(Map1f)(target, u1, u2, stride, order, points);
}

void GLAPIENTRY glMap2d(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points)
{
   DISPATCH(Map2d)(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);
}

void GLAPIENTRY glMap2f(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points)
{
   DISPATCH(Map2f)(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);
}

void GLAPIENTRY glMapGrid1d(GLint un, GLdouble u1, GLdouble u2)
{
   DISPATCH(MapGrid1d)(un, u1, u2);
}

void GLAPIENTRY glMapGrid1f(GLint un, GLfloat u1, GLfloat u2)
{
   DISPATCH(MapGrid1f)(un, u1, u2);
}

void GLAPIENTRY glMapGrid2d(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2)
{
   DISPATCH(MapGrid2d)(un, u1, u2, vn, v1, v2);
}

void GLAPIENTRY glMapGrid2f(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2)
{
   DISPATCH(MapGrid2f)(un, u1, u2, vn, v1, v2);
}

void GLAPIENTRY glMaterialf(GLenum face, GLenum pname, GLfloat param)
{
   DISPATCH(Materialf)(face, pname, param);
}

void GLAPIENTRY glMateriali(GLenum face, GLenum pname, GLint param)
{
   DISPATCH(Materiali)(face, pname, param);
}

void GLAPIENTRY glMaterialfv(GLenum face, GLenum pname, const GLfloat *params)
{
   DISPATCH(Materialfv)(face, pname, params);
}

void GLAPIENTRY glMaterialiv(GLenum face, GLenum pname, const GLint *params)
{
   DISPATCH(Materialiv)(face, pname, params);
}

void GLAPIENTRY glMatrixMode(GLenum mode)
{
   DISPATCH(MatrixMode)(mode);
}

void GLAPIENTRY glMultMatrixd(const GLdouble *m)
{
   DISPATCH(MultMatrixd)(m);
}

void GLAPIENTRY glMultMatrixf(const GLfloat *m)
{
   DISPATCH(MultMatrixf)(m);
}

void GLAPIENTRY glNewList(GLuint list, GLenum mode)
{
   DISPATCH(NewList)(list, mode);
}

void GLAPIENTRY glNormal3b(GLbyte nx, GLbyte ny, GLbyte nz)
{
   DISPATCH(Normal3b)(nx, ny, nz);
}

void GLAPIENTRY glNormal3bv(const GLbyte *v)
{
   DISPATCH(Normal3bv)(v);
}

void GLAPIENTRY glNormal3d(GLdouble nx, GLdouble ny, GLdouble nz)
{
   DISPATCH(Normal3d)(nx, ny, nz);
}

void GLAPIENTRY glNormal3dv(const GLdouble *v)
{
   DISPATCH(Normal3dv)(v);
}

void GLAPIENTRY glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz)
{
   DISPATCH(Normal3f)(nx, ny, nz);
}

void GLAPIENTRY glNormal3fv(const GLfloat *v)
{
   DISPATCH(Normal3fv)(v);
}

void GLAPIENTRY glNormal3i(GLint nx, GLint ny, GLint nz)
{
   DISPATCH(Normal3i)(nx, ny, nz);
}

void GLAPIENTRY glNormal3iv(const GLint *v)
{
   DISPATCH(Normal3iv)(v);
}

void GLAPIENTRY glNormal3s(GLshort nx, GLshort ny, GLshort nz)
{
   DISPATCH(Normal3s)(nx, ny, nz);
}

void GLAPIENTRY glNormal3sv(const GLshort *v)
{
   DISPATCH(Normal3sv)(v);
}

void GLAPIENTRY glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble nearval, GLdouble farval)
{
   DISPATCH(Ortho)(left, right, bottom, top, nearval, farval);
}

void GLAPIENTRY glPassThrough(GLfloat token)
{
   DISPATCH(PassThrough)(token);
}

void GLAPIENTRY glPixelMapfv(GLenum map, GLint mapsize, const GLfloat *values)
{
   DISPATCH(PixelMapfv)(map, mapsize, values);
}

void GLAPIENTRY glPixelMapuiv(GLenum map, GLint mapsize, const GLuint *values)
{
   DISPATCH(PixelMapuiv)(map, mapsize, values);
}

void GLAPIENTRY glPixelMapusv(GLenum map, GLint mapsize, const GLushort *values)
{
   DISPATCH(PixelMapusv)(map, mapsize, values);
}

void GLAPIENTRY glPixelStoref(GLenum pname, GLfloat param)
{
   DISPATCH(PixelStoref)(pname, param);
}

void GLAPIENTRY glPixelStorei(GLenum pname, GLint param)
{
   DISPATCH(PixelStorei)(pname, param);
}

void GLAPIENTRY glPixelTransferf(GLenum pname, GLfloat param)
{
   DISPATCH(PixelTransferf)(pname, param);
}

void GLAPIENTRY glPixelTransferi(GLenum pname, GLint param)
{
   DISPATCH(PixelTransferi)(pname, param);
}

void GLAPIENTRY glPixelZoom(GLfloat xfactor, GLfloat yfactor)
{
   DISPATCH(PixelZoom)(xfactor, yfactor);
}

void GLAPIENTRY glPointSize(GLfloat size)
{
   DISPATCH(PointSize)(size);
}

void GLAPIENTRY glPolygonMode(GLenum face, GLenum mode)
{
   DISPATCH(PolygonMode)(face, mode);
}

void GLAPIENTRY glPolygonStipple(const GLubyte *pattern)
{
   DISPATCH(PolygonStipple)(pattern);
}

void GLAPIENTRY glPopAttrib(void)
{
   DISPATCH(PopAttrib)();
}

void GLAPIENTRY glPopMatrix(void)
{
   DISPATCH(PopMatrix)();
}

void GLAPIENTRY glPopName(void)
{
   DISPATCH(PopName)();
}

void GLAPIENTRY glPushAttrib(GLbitfield mask)
{
   DISPATCH(PushAttrib)(mask);
}

void GLAPIENTRY glPushMatrix(void)
{
   DISPATCH(PushMatrix)();
}

void GLAPIENTRY glPushName(GLuint name)
{
   DISPATCH(PushName)(name);
}

void GLAPIENTRY glRasterPos2d(GLdouble x, GLdouble y)
{
   DISPATCH(RasterPos2d)(x, y);
}

void GLAPIENTRY glRasterPos2f(GLfloat x, GLfloat y)
{
   DISPATCH(RasterPos2f)(x, y);
}

void GLAPIENTRY glRasterPos2i(GLint x, GLint y)
{
   DISPATCH(RasterPos2i)(x, y);
}

void GLAPIENTRY glRasterPos2s(GLshort x, GLshort y)
{
   DISPATCH(RasterPos2s)(x, y);
}

void GLAPIENTRY glRasterPos3d(GLdouble x, GLdouble y, GLdouble z)
{
   DISPATCH(RasterPos3d)(x, y, z);
}

void GLAPIENTRY glRasterPos3f(GLfloat x, GLfloat y, GLfloat z)
{
   DISPATCH(RasterPos3f)(x, y, z);
}

void GLAPIENTRY glRasterPos3i(GLint x, GLint y, GLint z)
{
   DISPATCH(RasterPos3i)(x, y, z);
}

void GLAPIENTRY glRasterPos3s(GLshort x, GLshort y, GLshort z)
{
   DISPATCH(RasterPos3s)(x, y, z);
}

void GLAPIENTRY glRasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   DISPATCH(RasterPos4d)(x, y, z, w);
}

void GLAPIENTRY glRasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   DISPATCH(RasterPos4f)(x, y, z, w);
}

void GLAPIENTRY glRasterPos4i(GLint x, GLint y, GLint z, GLint w)
{
   DISPATCH(RasterPos4i)(x, y, z, w);
}

void GLAPIENTRY glRasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
   DISPATCH(RasterPos4s)(x, y, z, w);
}

void GLAPIENTRY glRasterPos2dv(const GLdouble *v)
{
   DISPATCH(RasterPos2dv)(v);
}

void GLAPIENTRY glRasterPos2fv(const GLfloat *v)
{
   DISPATCH(RasterPos2fv)(v);
}

void GLAPIENTRY glRasterPos2iv(const GLint *v)
{
   DISPATCH(RasterPos2iv)(v);
}

void GLAPIENTRY glRasterPos2sv(const GLshort *v)
{
   DISPATCH(RasterPos2sv)(v);
}

void GLAPIENTRY glRasterPos3dv(const GLdouble *v)
{
   DISPATCH(RasterPos3dv)(v);
}

void GLAPIENTRY glRasterPos3fv(const GLfloat *v)
{
   DISPATCH(RasterPos3fv)(v);
}

void GLAPIENTRY glRasterPos3iv(const GLint *v)
{
   DISPATCH(RasterPos3iv)(v);
}

void GLAPIENTRY glRasterPos3sv(const GLshort *v)
{
   DISPATCH(RasterPos3sv)(v);
}

void GLAPIENTRY glRasterPos4dv(const GLdouble *v)
{
   DISPATCH(RasterPos4dv)(v);
}

void GLAPIENTRY glRasterPos4fv(const GLfloat *v)
{
   DISPATCH(RasterPos4fv)(v);
}

void GLAPIENTRY glRasterPos4iv(const GLint *v)
{
   DISPATCH(RasterPos4iv)(v);
}

void GLAPIENTRY glRasterPos4sv(const GLshort *v)
{
   DISPATCH(RasterPos4sv)(v);
}

void GLAPIENTRY glReadBuffer(GLenum mode)
{
   DISPATCH(ReadBuffer)(mode);
}

void GLAPIENTRY glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels)
{
   DISPATCH(ReadPixels)(x, y, width, height, format, type, pixels);
}

void GLAPIENTRY glRectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2)
{
   DISPATCH(Rectd)(x1, y1, x2, y2);
}

void GLAPIENTRY glRectdv(const GLdouble *v1, const GLdouble *v2)
{
   DISPATCH(Rectdv)(v1, v2);
}

void GLAPIENTRY glRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
   DISPATCH(Rectf)(x1, y1, x2, y2);
}

void GLAPIENTRY glRectfv(const GLfloat *v1, const GLfloat *v2)
{
   DISPATCH(Rectfv)(v1, v2);
}

void GLAPIENTRY glRecti(GLint x1, GLint y1, GLint x2, GLint y2)
{
   DISPATCH(Recti)(x1, y1, x2, y2);
}

void GLAPIENTRY glRectiv(const GLint *v1, const GLint *v2)
{
   DISPATCH(Rectiv)(v1, v2);
}

void GLAPIENTRY glRects(GLshort x1, GLshort y1, GLshort x2, GLshort y2)
{
   DISPATCH(Rects)(x1, y1, x2, y2);
}

void GLAPIENTRY glRectsv(const GLshort *v1, const GLshort *v2)
{
   DISPATCH(Rectsv)(v1, v2);
}

GLint GLAPIENTRY glRenderMode(GLenum mode)
{
   return DISPATCH(RenderMode)(mode);
}

void GLAPIENTRY glRotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z)
{
   DISPATCH(Rotated)(angle, x, y, z);
}

void GLAPIENTRY glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
   DISPATCH(Rotatef)(angle, x, y, z);
}

void GLAPIENTRY glSelectBuffer(GLsizei size, GLuint *buffer)
{
   DISPATCH(SelectBuffer)(size, buffer);
}

void GLAPIENTRY glScaled(GLdouble x, GLdouble y, GLdouble z)
{
   DISPATCH(Scaled)(x, y, z);
}

void GLAPIENTRY glScalef(GLfloat x, GLfloat y, GLfloat z)
{
   DISPATCH(Scalef)(x, y, z);
}

void GLAPIENTRY glScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
   DISPATCH(Scissor)(x, y, width, height);
}

void GLAPIENTRY glShadeModel(GLenum mode)
{
   DISPATCH(ShadeModel)(mode);
}

void GLAPIENTRY glStencilFunc(GLenum func, GLint ref, GLuint mask)
{
   DISPATCH(StencilFunc)(func, ref, mask);
}

void GLAPIENTRY glStencilMask(GLuint mask)
{
   DISPATCH(StencilMask)(mask);
}

void GLAPIENTRY glStencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
   DISPATCH(StencilOp)(fail, zfail, zpass);
}

void GLAPIENTRY glTexCoord1d(GLdouble s)
{
   DISPATCH(TexCoord1d)(s);
}

void GLAPIENTRY glTexCoord1f(GLfloat s)
{
   DISPATCH(TexCoord1f)(s);
}

void GLAPIENTRY glTexCoord1i(GLint s)
{
   DISPATCH(TexCoord1i)(s);
}

void GLAPIENTRY glTexCoord1s(GLshort s)
{
   DISPATCH(TexCoord1s)(s);
}

void GLAPIENTRY glTexCoord2d(GLdouble s, GLdouble t)
{
   DISPATCH(TexCoord2d)(s, t);
}

void GLAPIENTRY glTexCoord2f(GLfloat s, GLfloat t)
{
   DISPATCH(TexCoord2f)(s, t);
}

void GLAPIENTRY glTexCoord2s(GLshort s, GLshort t)
{
   DISPATCH(TexCoord2s)(s, t);
}

void GLAPIENTRY glTexCoord2i(GLint s, GLint t)
{
   DISPATCH(TexCoord2i)(s, t);
}

void GLAPIENTRY glTexCoord3d(GLdouble s, GLdouble t, GLdouble r)
{
   DISPATCH(TexCoord3d)(s, t, r);
}

void GLAPIENTRY glTexCoord3f(GLfloat s, GLfloat t, GLfloat r)
{
   DISPATCH(TexCoord3f)(s, t, r);
}

void GLAPIENTRY glTexCoord3i(GLint s, GLint t, GLint r)
{
   DISPATCH(TexCoord3i)(s, t, r);
}

void GLAPIENTRY glTexCoord3s(GLshort s, GLshort t, GLshort r)
{
   DISPATCH(TexCoord3s)(s, t, r);
}

void GLAPIENTRY glTexCoord4d(GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
   DISPATCH(TexCoord4d)(s, t, r, q);
}

void GLAPIENTRY glTexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
   DISPATCH(TexCoord4f)(s, t, r, q);
}

void GLAPIENTRY glTexCoord4i(GLint s, GLint t, GLint r, GLint q)
{
   DISPATCH(TexCoord4i)(s, t, r, q);
}

void GLAPIENTRY glTexCoord4s(GLshort s, GLshort t, GLshort r, GLshort q)
{
   DISPATCH(TexCoord4s)(s, t, r, q);
}

void GLAPIENTRY glTexCoord1dv(const GLdouble *v)
{
   DISPATCH(TexCoord1dv)(v);
}

void GLAPIENTRY glTexCoord1fv(const GLfloat *v)
{
   DISPATCH(TexCoord1fv)(v);
}

void GLAPIENTRY glTexCoord1iv(const GLint *v)
{
   DISPATCH(TexCoord1iv)(v);
}

void GLAPIENTRY glTexCoord1sv(const GLshort *v)
{
   DISPATCH(TexCoord1sv)(v);
}

void GLAPIENTRY glTexCoord2dv(const GLdouble *v)
{
   DISPATCH(TexCoord2dv)(v);
}

void GLAPIENTRY glTexCoord2fv(const GLfloat *v)
{
   DISPATCH(TexCoord2fv)(v);
}

void GLAPIENTRY glTexCoord2iv(const GLint *v)
{
   DISPATCH(TexCoord2iv)(v);
}

void GLAPIENTRY glTexCoord2sv(const GLshort *v)
{
   DISPATCH(TexCoord2sv)(v);
}

void GLAPIENTRY glTexCoord3dv(const GLdouble *v)
{
   DISPATCH(TexCoord3dv)(v);
}

void GLAPIENTRY glTexCoord3fv(const GLfloat *v)
{
   DISPATCH(TexCoord3fv)(v);
}

void GLAPIENTRY glTexCoord3iv(const GLint *v)
{
   DISPATCH(TexCoord3iv)(v);
}

void GLAPIENTRY glTexCoord3sv(const GLshort *v)
{
   DISPATCH(TexCoord3sv)(v);
}

void GLAPIENTRY glTexCoord4dv(const GLdouble *v)
{
   DISPATCH(TexCoord4dv)(v);
}

void GLAPIENTRY glTexCoord4fv(const GLfloat *v)
{
   DISPATCH(TexCoord4fv)(v);
}

void GLAPIENTRY glTexCoord4iv(const GLint *v)
{
   DISPATCH(TexCoord4iv)(v);
}

void GLAPIENTRY glTexCoord4sv(const GLshort *v)
{
   DISPATCH(TexCoord4sv)(v);
}

void GLAPIENTRY glTexGend(GLenum coord, GLenum pname, GLdouble param)
{
   DISPATCH(TexGend)(coord, pname, param);
}

void GLAPIENTRY glTexGendv(GLenum coord, GLenum pname, const GLdouble *params)
{
   DISPATCH(TexGendv)(coord, pname, params);
}

void GLAPIENTRY glTexGenf(GLenum coord, GLenum pname, GLfloat param)
{
   DISPATCH(TexGenf)(coord, pname, param);
}

void GLAPIENTRY glTexGenfv(GLenum coord, GLenum pname, const GLfloat *params)
{
   DISPATCH(TexGenfv)(coord, pname, params);
}

void GLAPIENTRY glTexGeni(GLenum coord, GLenum pname, GLint param)
{
   DISPATCH(TexGeni)(coord, pname, param);
}

void GLAPIENTRY glTexGeniv(GLenum coord, GLenum pname, const GLint *params)
{
   DISPATCH(TexGeniv)(coord, pname, params);
}

void GLAPIENTRY glTexEnvf(GLenum target, GLenum pname, GLfloat param)
{
   DISPATCH(TexEnvf)(target, pname, param);
}

void GLAPIENTRY glTexEnvfv(GLenum target, GLenum pname, const GLfloat *param)
{
   DISPATCH(TexEnvfv)(target, pname, param);
}

void GLAPIENTRY glTexEnvi(GLenum target, GLenum pname, GLint param)
{
   DISPATCH(TexEnvi)(target, pname, param);
}

void GLAPIENTRY glTexEnviv(GLenum target, GLenum pname, const GLint *param)
{
   DISPATCH(TexEnviv)(target, pname, param);
}

void GLAPIENTRY glTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
   DISPATCH(TexImage1D)(target, level, internalformat, width, border, format, type, pixels);
}

void GLAPIENTRY glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
   DISPATCH(TexImage2D)(target, level, internalformat, width, height, border, format, type, pixels);
}

void GLAPIENTRY glTexParameterf(GLenum target, GLenum pname, GLfloat param)
{
   DISPATCH(TexParameterf)(target, pname, param);
}

void GLAPIENTRY glTexParameterfv(GLenum target, GLenum pname, const GLfloat *params)
{
   DISPATCH(TexParameterfv)(target, pname, params);
}

void GLAPIENTRY glTexParameteri(GLenum target, GLenum pname, GLint param)
{
   DISPATCH(TexParameteri)(target, pname, param);
}

void GLAPIENTRY glTexParameteriv(GLenum target, GLenum pname, const GLint *params)
{
   DISPATCH(TexParameteriv)(target, pname, params);
}

void GLAPIENTRY glTranslated(GLdouble x, GLdouble y, GLdouble z)
{
   DISPATCH(Translated)(x, y, z);
}

void GLAPIENTRY glTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
   DISPATCH(Translatef)(x, y, z);
}

void GLAPIENTRY glVertex2d(GLdouble x, GLdouble y)
{
   DISPATCH(Vertex2d)(x, y);
}

void GLAPIENTRY glVertex2dv(const GLdouble *v)
{
   DISPATCH(Vertex2dv)(v);
}

void GLAPIENTRY glVertex2f(GLfloat x, GLfloat y)
{
   DISPATCH(Vertex2f)(x, y);
}

void GLAPIENTRY glVertex2fv(const GLfloat *v)
{
   DISPATCH(Vertex2fv)(v);
}

void GLAPIENTRY glVertex2i(GLint x, GLint y)
{
   DISPATCH(Vertex2i)(x, y);
}

void GLAPIENTRY glVertex2iv(const GLint *v)
{
   DISPATCH(Vertex2iv)(v);
}

void GLAPIENTRY glVertex2s(GLshort x, GLshort y)
{
   DISPATCH(Vertex2s)(x, y);
}

void GLAPIENTRY glVertex2sv(const GLshort *v)
{
   DISPATCH(Vertex2sv)(v);
}

void GLAPIENTRY glVertex3d(GLdouble x, GLdouble y, GLdouble z)
{
   DISPATCH(Vertex3d)(x, y, z);
}

void GLAPIENTRY glVertex3dv(const GLdouble *v)
{
   DISPATCH(Vertex3dv)(v);
}

void GLAPIENTRY glVertex3f(GLfloat x, GLfloat y, GLfloat z)
{
   DISPATCH(Vertex3f)(x, y, z);
}

void GLAPIENTRY glVertex3fv(const GLfloat *v)
{
   DISPATCH(Vertex3fv)(v);
}

void GLAPIENTRY glVertex3i(GLint x, GLint y, GLint z)
{
   DISPATCH(Vertex3i)(x, y, z);
}

void GLAPIENTRY glVertex3iv(const GLint *v)
{
   DISPATCH(Vertex3iv)(v);
}

void GLAPIENTRY glVertex3s(GLshort x, GLshort y, GLshort z)
{
   DISPATCH(Vertex3s)(x, y, z);
}

void GLAPIENTRY glVertex3sv(const GLshort *v)
{
   DISPATCH(Vertex3sv)(v);
}

void GLAPIENTRY glVertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   DISPATCH(Vertex4d)(x, y, z, w);
}

void GLAPIENTRY glVertex4dv(const GLdouble *v)
{
   DISPATCH(Vertex4dv)(v);
}

void GLAPIENTRY glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   DISPATCH(Vertex4f)(x, y, z, w);
}

void GLAPIENTRY glVertex4fv(const GLfloat *v)
{
   DISPATCH(Vertex4fv)(v);
}

void GLAPIENTRY glVertex4i(GLint x, GLint y, GLint z, GLint w)
{
   DISPATCH(Vertex4i)(x, y, z, w);
}

void GLAPIENTRY glVertex4iv(const GLint *v)
{
   DISPATCH(Vertex4iv)(v);
}

void GLAPIENTRY glVertex4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
   DISPATCH(Vertex4s)(x, y, z, w);
}

void GLAPIENTRY glVertex4sv(const GLshort *v)
{
   DISPATCH(Vertex4sv)(v);
}

void GLAPIENTRY glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
   DISPATCH(Viewport)(x, y, width, height);
}




#ifdef _GLAPI_VERSION_1_1

GLboolean GLAPIENTRY glAreTexturesResident(GLsizei n, const GLuint *textures, GLboolean *residences)
{
   return DISPATCH(AreTexturesResident)(n, textures, residences);
}

void GLAPIENTRY glArrayElement(GLint i)
{
   DISPATCH(ArrayElement)(i);
}

void GLAPIENTRY glBindTexture(GLenum target, GLuint texture)
{
   DISPATCH(BindTexture)(target, texture);
}

void GLAPIENTRY glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr)
{
   DISPATCH(ColorPointer)(size, type, stride, ptr);
}

void GLAPIENTRY glCopyTexImage1D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border)
{
   DISPATCH(CopyTexImage1D)(target, level, internalformat, x, y, width, border);
}

void GLAPIENTRY glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
   DISPATCH(CopyTexImage2D)(target, level, internalformat, x, y, width, height, border);
}

void GLAPIENTRY glCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)
{
   DISPATCH(CopyTexSubImage1D)(target, level, xoffset, x, y, width);
}

void GLAPIENTRY glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
   DISPATCH(CopyTexSubImage2D)(target, level, xoffset, yoffset, x, y, width, height);
}

void GLAPIENTRY glDeleteTextures(GLsizei n, const GLuint *textures)
{
   DISPATCH(DeleteTextures)(n, textures);
}

void GLAPIENTRY glDisableClientState(GLenum cap)
{
   DISPATCH(DisableClientState)(cap);
}

void GLAPIENTRY glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
   DISPATCH(DrawArrays)(mode, first, count);
}

void GLAPIENTRY glEdgeFlagPointer(GLsizei stride, const GLvoid *ptr)
{
   DISPATCH(EdgeFlagPointer)(stride, ptr);
}

void GLAPIENTRY glEnableClientState(GLenum cap)
{
   DISPATCH(EnableClientState)(cap);
}

void GLAPIENTRY glGenTextures(GLsizei n, GLuint *textures)
{
   DISPATCH(GenTextures)(n, textures);
}

void GLAPIENTRY glGetPointerv(GLenum pname, GLvoid **params)
{
   DISPATCH(GetPointerv)(pname, params);
}

void GLAPIENTRY glIndexPointer(GLenum type, GLsizei stride, const GLvoid *ptr)
{
   DISPATCH(IndexPointer)(type, stride, ptr);
}

void GLAPIENTRY glNormalPointer(GLenum type, GLsizei stride, const GLvoid *ptr)
{
   DISPATCH(NormalPointer)(type, stride, ptr);
}

void GLAPIENTRY glPolygonOffset(GLfloat factor, GLfloat units)
{
   DISPATCH(PolygonOffset)(factor, units);
}

void GLAPIENTRY glPopClientAttrib(void)
{
   DISPATCH(PopClientAttrib)();
}

void GLAPIENTRY glPrioritizeTextures(GLsizei n, const GLuint *textures, const GLclampf *priorities)
{
   DISPATCH(PrioritizeTextures)(n, textures, priorities);
}

void GLAPIENTRY glPushClientAttrib(GLbitfield mask)
{
   DISPATCH(PushClientAttrib)(mask);
}

void GLAPIENTRY glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr)
{
   DISPATCH(TexCoordPointer)(size, type, stride, ptr);
}

void GLAPIENTRY glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels)
{
   DISPATCH(TexSubImage1D)(target, level, xoffset, width, format, type, pixels);
}

void GLAPIENTRY glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
   DISPATCH(TexSubImage2D)(target, level, xoffset, yoffset, width, height, format, type, pixels);
}

void GLAPIENTRY glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr)
{
   DISPATCH(VertexPointer)(size, type, stride, ptr);
}

#endif  /*_GLAPI_VERSION_1_1*/



#ifdef _GLAPI_VERSION_1_2

void GLAPIENTRY glCopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
   DISPATCH(CopyTexSubImage3D)(target, level, xoffset, yoffset, zoffset, x, y, width, height);
}

void GLAPIENTRY glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices)
{
   DISPATCH(DrawRangeElements)(mode, start, end, count, type, indices);
}

void GLAPIENTRY glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
   DISPATCH(TexImage3D)(target, level, internalformat, width, height, depth, border, format, type, pixels);
}

void GLAPIENTRY glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels)
{
   DISPATCH(TexSubImage3D)(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
}


#ifdef _GLAPI_ARB_imaging

void GLAPIENTRY glBlendColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a)
{
   DISPATCH(BlendColor)(r, g, b, a);
}

void GLAPIENTRY glBlendEquation(GLenum mode)
{
   DISPATCH(BlendEquation)(mode);
}

void GLAPIENTRY glColorSubTable(GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid *data)
{
   DISPATCH(ColorSubTable)(target, start, count, format, type, data);
}

void GLAPIENTRY glColorTable(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *table)
{
   DISPATCH(ColorTable)(target, internalformat, width, format, type, table);
}

void GLAPIENTRY glColorTableParameterfv(GLenum target, GLenum pname, const GLfloat *params)
{
   DISPATCH(ColorTableParameterfv)(target, pname, params);
}

void GLAPIENTRY glColorTableParameteriv(GLenum target, GLenum pname, const GLint *params)
{
   DISPATCH(ColorTableParameteriv)(target, pname, params);
}

void GLAPIENTRY glConvolutionFilter1D(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *image)
{
   DISPATCH(ConvolutionFilter1D)(target, internalformat, width, format, type, image);
}

void GLAPIENTRY glConvolutionFilter2D(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *image)
{
   DISPATCH(ConvolutionFilter2D)(target, internalformat, width, height, format, type, image);
}

void GLAPIENTRY glConvolutionParameterf(GLenum target, GLenum pname, GLfloat params)
{
   DISPATCH(ConvolutionParameterf)(target, pname, params);
}

void GLAPIENTRY glConvolutionParameterfv(GLenum target, GLenum pname, const GLfloat *params)
{
   DISPATCH(ConvolutionParameterfv)(target, pname, params);
}

void GLAPIENTRY glConvolutionParameteri(GLenum target, GLenum pname, GLint params)
{
   DISPATCH(ConvolutionParameteri)(target, pname, params);
}

void GLAPIENTRY glConvolutionParameteriv(GLenum target, GLenum pname, const GLint *params)
{
   DISPATCH(ConvolutionParameteriv)(target, pname, params);
}

void GLAPIENTRY glCopyColorSubTable(GLenum target, GLsizei start, GLint x, GLint y, GLsizei width)
{
   DISPATCH(CopyColorSubTable)(target, start, x, y, width);
}

void GLAPIENTRY glCopyColorTable(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width)
{
   DISPATCH(CopyColorTable)(target, internalformat, x, y, width);
}

void GLAPIENTRY glCopyConvolutionFilter1D(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width)
{
   DISPATCH(CopyConvolutionFilter1D)(target, internalformat, x, y, width);
}

void GLAPIENTRY glCopyConvolutionFilter2D(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height)
{
   DISPATCH(CopyConvolutionFilter2D)(target, internalformat, x, y, width, height);
}

void GLAPIENTRY glGetColorTable(GLenum target, GLenum format, GLenum type, GLvoid *table)
{
   DISPATCH(GetColorTable)(target, format, type, table);
}

void GLAPIENTRY glGetColorTableParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
   DISPATCH(GetColorTableParameterfv)(target, pname, params);
}

void GLAPIENTRY glGetColorTableParameteriv(GLenum target, GLenum pname, GLint *params)
{
   DISPATCH(GetColorTableParameteriv)(target, pname, params);
}

void GLAPIENTRY glGetConvolutionFilter(GLenum target, GLenum format, GLenum type, GLvoid *image)
{
   DISPATCH(GetConvolutionFilter)(target, format, type, image);
}

void GLAPIENTRY glGetConvolutionParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
   DISPATCH(GetConvolutionParameterfv)(target, pname, params);
}

void GLAPIENTRY glGetConvolutionParameteriv(GLenum target, GLenum pname, GLint *params)
{
   DISPATCH(GetConvolutionParameteriv)(target, pname, params);
}

void GLAPIENTRY glGetHistogram(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values)
{
   DISPATCH(GetHistogram)(target, reset, format, type, values);
}

void GLAPIENTRY glGetHistogramParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
   DISPATCH(GetHistogramParameterfv)(target, pname, params);
}

void GLAPIENTRY glGetHistogramParameteriv(GLenum target, GLenum pname, GLint *params)
{
   DISPATCH(GetHistogramParameteriv)(target, pname, params);
}

void GLAPIENTRY glGetMinmax(GLenum target, GLboolean reset, GLenum format, GLenum types, GLvoid *values)
{
   DISPATCH(GetMinmax)(target, reset, format, types, values);
}

void GLAPIENTRY glGetMinmaxParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
   DISPATCH(GetMinmaxParameterfv)(target, pname, params);
}

void GLAPIENTRY glGetMinmaxParameteriv(GLenum target, GLenum pname, GLint *params)
{
   DISPATCH(GetMinmaxParameteriv)(target, pname, params);
}

void GLAPIENTRY glGetSeparableFilter(GLenum target, GLenum format, GLenum type, GLvoid *row, GLvoid *column, GLvoid *span)
{
   DISPATCH(GetSeparableFilter)(target, format, type, row, column, span);
}

void GLAPIENTRY glHistogram(GLenum target, GLsizei width, GLenum internalformat, GLboolean sink)
{
   DISPATCH(Histogram)(target, width, internalformat, sink);
}

void GLAPIENTRY glMinmax(GLenum target, GLenum internalformat, GLboolean sink)
{
   DISPATCH(Minmax)(target, internalformat, sink);
}

void GLAPIENTRY glResetMinmax(GLenum target)
{
   DISPATCH(ResetMinmax)(target);
}

void GLAPIENTRY glResetHistogram(GLenum target)
{
   DISPATCH(ResetHistogram)(target);
}

void GLAPIENTRY glSeparableFilter2D(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *row, const GLvoid *column)
{
   DISPATCH(SeparableFilter2D)(target, internalformat, width, height, format, type, row, column);
}


#endif  /*_GLAPI_ARB_imaging*/
#endif  /*_GLAPI_VERSION_1_2*/



/***
 *** Extension functions
 ***/

#ifdef _GLAPI_EXT_blend_minmax
void GLAPIENTRY glBlendEquationEXT(GLenum mode)
{
   DISPATCH(BlendEquationEXT)(mode);
}
#endif


#ifdef _GLAPI_EXT_blend_color
void GLAPIENTRY glBlendColorEXT(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
   DISPATCH(BlendColorEXT)(red, green, blue, alpha);
}
#endif


#ifdef _GLAPI_EXT_polygon_offset
void GLAPIENTRY glPolygonOffsetEXT(GLfloat factor, GLfloat bias)
{
   DISPATCH(PolygonOffsetEXT)(factor, bias);
}
#endif



#ifdef _GLAPI_EXT_vertex_array

void GLAPIENTRY glVertexPointerEXT(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *ptr)
{
   (void) count;
   DISPATCH(VertexPointer)(size, type, stride, ptr);
}

void GLAPIENTRY glNormalPointerEXT(GLenum type, GLsizei stride, GLsizei count, const GLvoid *ptr)
{
   (void) count;
   DISPATCH(NormalPointer)(type, stride, ptr);
}

void GLAPIENTRY glColorPointerEXT(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *ptr)
{
   (void) count;
   DISPATCH(ColorPointer)(size, type, stride, ptr);
}

void GLAPIENTRY glIndexPointerEXT(GLenum type, GLsizei stride, GLsizei count, const GLvoid *ptr)
{
   (void) count;
   DISPATCH(IndexPointer)(type, stride, ptr);
}

void GLAPIENTRY glTexCoordPointerEXT(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *ptr)
{
   (void) count;
   DISPATCH(ColorPointer)(size, type, stride, ptr);
}

void GLAPIENTRY glEdgeFlagPointerEXT(GLsizei stride, GLsizei count, const GLboolean *ptr)
{
   (void) count;
   DISPATCH(EdgeFlagPointer)(stride, ptr);
}

void GLAPIENTRY glGetPointervEXT(GLenum pname, void **params)
{
   DISPATCH(GetPointerv)(pname, params);
}

void GLAPIENTRY glArrayElementEXT(GLint i)
{
   DISPATCH(ArrayElement)(i);
}

void GLAPIENTRY glDrawArraysEXT(GLenum mode, GLint first, GLsizei count)
{
   DISPATCH(DrawArrays)(mode, first, count);
}

#endif  /* GL_EXT_vertex_arrays */



#ifdef _GLAPI_EXT_texture_object

void GLAPIENTRY glGenTexturesEXT(GLsizei n, GLuint *textures)
{
   DISPATCH(GenTextures)(n, textures);
}

void GLAPIENTRY glDeleteTexturesEXT(GLsizei n, const GLuint *texture)
{
   DISPATCH(DeleteTextures)(n, texture);
}

void GLAPIENTRY glBindTextureEXT(GLenum target, GLuint texture)
{
   DISPATCH(BindTexture)(target, texture);
}

void GLAPIENTRY glPrioritizeTexturesEXT(GLsizei n, const GLuint *textures, const GLclampf *priorities)
{
   DISPATCH(PrioritizeTextures)(n, textures, priorities);
}

GLboolean GLAPIENTRY glAreTexturesResidentEXT(GLsizei n, const GLuint *textures, GLboolean *residences)
{
   DISPATCH(AreTexturesResident)(n, textures, residences);
   return GL_FALSE;
}

GLboolean GLAPIENTRY glIsTextureEXT(GLuint texture)
{
   DISPATCH(IsTexture)(texture);
   return GL_FALSE;
}
#endif  /* GL_EXT_texture_object */



#ifdef _GLAPI_EXT_texture3D

void GLAPIENTRY glTexImage3DEXT(GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
   DISPATCH(TexImage3D)(target, level, internalFormat, width, height, depth, border, format, type, pixels);
}

void GLAPIENTRY glTexSubImage3DEXT(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels)
{
   DISPATCH(TexSubImage3D)(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
}

void GLAPIENTRY glCopyTexSubImage3DEXT(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
   DISPATCH(CopyTexSubImage3D)(target, level, xoffset, yoffset, zoffset, x, y, width, height);
}

#endif  /* GL_EXT_texture3D*/



#ifdef _GLAPI_EXT_paletted_texture

void GLAPIENTRY glColorTableEXT(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *table)
{
   DISPATCH(ColorTableEXT)(target, internalformat, width, format, type, table);
}

void GLAPIENTRY glColorSubTableEXT(GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid *data)
{
   DISPATCH(ColorSubTableEXT)(target, start, count, format, type, data);
}

void GLAPIENTRY glGetColorTableEXT(GLenum target, GLenum format, GLenum type, GLvoid *table)
{
   DISPATCH(GetColorTableEXT)(target, format, type, table);
}

void GLAPIENTRY glGetColorTableParameterfvEXT(GLenum target, GLenum pname, GLfloat *params)
{
   DISPATCH(GetColorTableParameterfvEXT)(target, pname, params);
}

void GLAPIENTRY glGetColorTableParameterivEXT(GLenum target, GLenum pname, GLint *params)
{
   DISPATCH(GetColorTableParameterivEXT)(target, pname, params);
}

#endif  /* GL_EXT_paletted_texture */



#ifdef _GLAPI_EXT_compiled_vertex_array

void GLAPIENTRY glLockArraysEXT(GLint first, GLsizei count)
{
   DISPATCH(LockArraysEXT)(first, count);
}

void GLAPIENTRY glUnlockArraysEXT(void)
{
   DISPATCH(UnlockArraysEXT)();
}

#endif  /* GL_EXT_compiled_vertex_array */



#ifdef _GLAPI_EXT_point_parameters

void GLAPIENTRY glPointParameterfEXT(GLenum target, GLfloat param)
{
   DISPATCH(PointParameterfEXT)(target, param);
}

void GLAPIENTRY glPointParameterfvEXT(GLenum target, const GLfloat *param)
{
   DISPATCH(PointParameterfvEXT)(target, param);
}

#endif  /* GL_EXT_point_parameters */



#ifdef _GLAPI_ARB_multitexture

void GLAPIENTRY glActiveTextureARB(GLenum texture)
{
   DISPATCH(ActiveTextureARB)(texture);
}

void GLAPIENTRY glClientActiveTextureARB(GLenum texture)
{
   DISPATCH(ClientActiveTextureARB)(texture);
}

void GLAPIENTRY glMultiTexCoord1dARB(GLenum target, GLdouble s)
{
   DISPATCH(MultiTexCoord1dARB)(target, s);
}

void GLAPIENTRY glMultiTexCoord1dvARB(GLenum target, const GLdouble *v)
{
   DISPATCH(MultiTexCoord1dvARB)(target, v);
}

void GLAPIENTRY glMultiTexCoord1fARB(GLenum target, GLfloat s)
{
   DISPATCH(MultiTexCoord1fARB)(target, s);
}

void GLAPIENTRY glMultiTexCoord1fvARB(GLenum target, const GLfloat *v)
{
   DISPATCH(MultiTexCoord1fvARB)(target, v);
}

void GLAPIENTRY glMultiTexCoord1iARB(GLenum target, GLint s)
{
   DISPATCH(MultiTexCoord1iARB)(target, s);
}

void GLAPIENTRY glMultiTexCoord1ivARB(GLenum target, const GLint *v)
{
   DISPATCH(MultiTexCoord1ivARB)(target, v);
}

void GLAPIENTRY glMultiTexCoord1sARB(GLenum target, GLshort s)
{
   DISPATCH(MultiTexCoord1sARB)(target, s);
}

void GLAPIENTRY glMultiTexCoord1svARB(GLenum target, const GLshort *v)
{
   DISPATCH(MultiTexCoord1svARB)(target, v);
}

void GLAPIENTRY glMultiTexCoord2dARB(GLenum target, GLdouble s, GLdouble t)
{
   DISPATCH(MultiTexCoord2dARB)(target, s, t);
}

void GLAPIENTRY glMultiTexCoord2dvARB(GLenum target, const GLdouble *v)
{
   DISPATCH(MultiTexCoord2dvARB)(target, v);
}

void GLAPIENTRY glMultiTexCoord2fARB(GLenum target, GLfloat s, GLfloat t)
{
   DISPATCH(MultiTexCoord2fARB)(target, s, t);
}

void GLAPIENTRY glMultiTexCoord2fvARB(GLenum target, const GLfloat *v)
{
   DISPATCH(MultiTexCoord2fvARB)(target, v);
}

void GLAPIENTRY glMultiTexCoord2iARB(GLenum target, GLint s, GLint t)
{
   DISPATCH(MultiTexCoord2iARB)(target, s, t);
}

void GLAPIENTRY glMultiTexCoord2ivARB(GLenum target, const GLint *v)
{
   DISPATCH(MultiTexCoord2ivARB)(target, v);
}

void GLAPIENTRY glMultiTexCoord2sARB(GLenum target, GLshort s, GLshort t)
{
   DISPATCH(MultiTexCoord2sARB)(target, s, t);
}

void GLAPIENTRY glMultiTexCoord2svARB(GLenum target, const GLshort *v)
{
   DISPATCH(MultiTexCoord2svARB)(target, v);
}

void GLAPIENTRY glMultiTexCoord3dARB(GLenum target, GLdouble s, GLdouble t, GLdouble r)
{
   DISPATCH(MultiTexCoord3dARB)(target, s, t, r);
}

void GLAPIENTRY glMultiTexCoord3dvARB(GLenum target, const GLdouble *v)
{
   DISPATCH(MultiTexCoord3dvARB)(target, v);
}

void GLAPIENTRY glMultiTexCoord3fARB(GLenum target, GLfloat s, GLfloat t, GLfloat r)
{
   DISPATCH(MultiTexCoord3fARB)(target, s, t, r);
}

void GLAPIENTRY glMultiTexCoord3fvARB(GLenum target, const GLfloat *v)
{
   DISPATCH(MultiTexCoord3fvARB)(target, v);
}

void GLAPIENTRY glMultiTexCoord3iARB(GLenum target, GLint s, GLint t, GLint r)
{
   DISPATCH(MultiTexCoord3iARB)(target, s, t, r);
}

void GLAPIENTRY glMultiTexCoord3ivARB(GLenum target, const GLint *v)
{
   DISPATCH(MultiTexCoord3ivARB)(target, v);
}

void GLAPIENTRY glMultiTexCoord3sARB(GLenum target, GLshort s, GLshort t, GLshort r)
{
   DISPATCH(MultiTexCoord3sARB)(target, s, t, r);
}

void GLAPIENTRY glMultiTexCoord3svARB(GLenum target, const GLshort *v)
{
   DISPATCH(MultiTexCoord3svARB)(target, v);
}

void GLAPIENTRY glMultiTexCoord4dARB(GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
   DISPATCH(MultiTexCoord4dARB)(target, s, t, r, q);
}

void GLAPIENTRY glMultiTexCoord4dvARB(GLenum target, const GLdouble *v)
{
   DISPATCH(MultiTexCoord4dvARB)(target, v);
}

void GLAPIENTRY glMultiTexCoord4fARB(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
   DISPATCH(MultiTexCoord4fARB)(target, s, t, r, q);
}

void GLAPIENTRY glMultiTexCoord4fvARB(GLenum target, const GLfloat *v)
{
   DISPATCH(MultiTexCoord4fvARB)(target, v);
}

void GLAPIENTRY glMultiTexCoord4iARB(GLenum target, GLint s, GLint t, GLint r, GLint q)
{
   DISPATCH(MultiTexCoord4iARB)(target, s, t, r, q);
}

void GLAPIENTRY glMultiTexCoord4ivARB(GLenum target, const GLint *v)
{
   DISPATCH(MultiTexCoord4ivARB)(target, v);
}

void GLAPIENTRY glMultiTexCoord4sARB(GLenum target, GLshort s, GLshort t, GLshort r, GLshort q)
{
   DISPATCH(MultiTexCoord4sARB)(target, s, t, r, q);
}

void GLAPIENTRY glMultiTexCoord4svARB(GLenum target, const GLshort *v)
{
   DISPATCH(MultiTexCoord4svARB)(target, v);
}

#endif  /* GL_ARB_multitexture */



#ifdef _GLAPI_INGR_blend_func_separate
void GLAPIENTRY glBlendFuncSeparateINGR(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha)
{
   DISPATCH(BlendFuncSeparateINGR)(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);
}
#endif  /* GL_INGR_blend_func_separate */



#ifdef _GLAPI_MESA_window_pos

void GLAPIENTRY glWindowPos2iMESA(GLint x, GLint y)
{
   DISPATCH(WindowPos4fMESA)(x, y, 0, 1);
}

void GLAPIENTRY glWindowPos2sMESA(GLshort x, GLshort y)
{
   DISPATCH(WindowPos4fMESA)(x, y, 0, 1);
}

void GLAPIENTRY glWindowPos2fMESA(GLfloat x, GLfloat y)
{
   DISPATCH(WindowPos4fMESA)(x, y, 0, 1);
}

void GLAPIENTRY glWindowPos2dMESA(GLdouble x, GLdouble y)
{
   DISPATCH(WindowPos4fMESA)(x, y, 0, 1);
}

void GLAPIENTRY glWindowPos2ivMESA(const GLint *p)
{
   DISPATCH(WindowPos4fMESA)(p[0], p[1], 0, 1);
}

void GLAPIENTRY glWindowPos2svMESA(const GLshort *p)
{
   DISPATCH(WindowPos4fMESA)(p[0], p[1], 0, 1);
}

void GLAPIENTRY glWindowPos2fvMESA(const GLfloat *p)
{
   DISPATCH(WindowPos4fMESA)(p[0], p[1], 0, 1);
}

void GLAPIENTRY glWindowPos2dvMESA(const GLdouble *p)
{
   DISPATCH(WindowPos4fMESA)(p[0], p[1], 0, 1);
}

void GLAPIENTRY glWindowPos3iMESA(GLint x, GLint y, GLint z)
{
   DISPATCH(WindowPos4fMESA)(x, y, z, 1);
}

void GLAPIENTRY glWindowPos3sMESA(GLshort x, GLshort y, GLshort z)
{
   DISPATCH(WindowPos4fMESA)(x, y, z, 1);
}

void GLAPIENTRY glWindowPos3fMESA(GLfloat x, GLfloat y, GLfloat z)
{
   DISPATCH(WindowPos4fMESA)(x, y, z, 1);
}

void GLAPIENTRY glWindowPos3dMESA(GLdouble x, GLdouble y, GLdouble z)
{
   DISPATCH(WindowPos4fMESA)(x, y, z, 1);
}

void GLAPIENTRY glWindowPos3ivMESA(const GLint *p)
{
   DISPATCH(WindowPos4fMESA)(p[0], p[1], p[2], 1.0);
}

void GLAPIENTRY glWindowPos3svMESA(const GLshort *p)
{
   DISPATCH(WindowPos4fMESA)(p[0], p[1], p[2], 1.0);
}

void GLAPIENTRY glWindowPos3fvMESA(const GLfloat *p)
{
   DISPATCH(WindowPos4fMESA)(p[0], p[1], p[2], 1.0);
}

void GLAPIENTRY glWindowPos3dvMESA(const GLdouble *p)
{
   DISPATCH(WindowPos4fMESA)(p[0], p[1], p[2], 1.0);
}

void GLAPIENTRY glWindowPos4iMESA(GLint x, GLint y, GLint z, GLint w)
{
   DISPATCH(WindowPos4fMESA)(x, y, z, w);
}

void GLAPIENTRY glWindowPos4sMESA(GLshort x, GLshort y, GLshort z, GLshort w)
{
   DISPATCH(WindowPos4fMESA)(x, y, z, w);
}

void GLAPIENTRY glWindowPos4fMESA(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   DISPATCH(WindowPos4fMESA)(x, y, z, w);
}

void GLAPIENTRY glWindowPos4dMESA(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   DISPATCH(WindowPos4fMESA)(x, y, z, w);
}

void GLAPIENTRY glWindowPos4ivMESA(const GLint *p)
{
   DISPATCH(WindowPos4fMESA)(p[0], p[1], p[2], p[3]);
}

void GLAPIENTRY glWindowPos4svMESA(const GLshort *p)
{
   DISPATCH(WindowPos4fMESA)(p[0], p[1], p[2], p[3]);
}

void GLAPIENTRY glWindowPos4fvMESA(const GLfloat *p)
{
   DISPATCH(WindowPos4fMESA)(p[0], p[1], p[2], p[3]);
}

void GLAPIENTRY glWindowPos4dvMESA(const GLdouble *p)
{
   DISPATCH(WindowPos4fMESA)(p[0], p[1], p[2], p[3]);
}

#endif  /* GL_MESA_window_pos */



#ifdef _GLAPI_MESA_resize_buffers
void GLAPIENTRY glResizeBuffersMESA(void)
{
   DISPATCH(ResizeBuffersMESA)();
}
#endif  /* GL_MESA_resize_buffers */


#ifdef DEBUG
/*
 * This struct is just used to be sure we've defined all the API functions.
 */
static struct _glapi_table completeness_test = {
   glAccum,
   glAlphaFunc,
   glBegin,
   glBitmap,
   glBlendFunc,
   glCallList,
   glCallLists,
   glClear,
   glClearAccum,
   glClearColor,
   glClearDepth,
   glClearIndex,
   glClearStencil,
   glClipPlane,
   glColor3b,
   glColor3bv,
   glColor3d,
   glColor3dv,
   glColor3f,
   glColor3fv,
   glColor3i,
   glColor3iv,
   glColor3s,
   glColor3sv,
   glColor3ub,
   glColor3ubv,
   glColor3ui,
   glColor3uiv,
   glColor3us,
   glColor3usv,
   glColor4b,
   glColor4bv,
   glColor4d,
   glColor4dv,
   glColor4f,
   glColor4fv,
   glColor4i,
   glColor4iv,
   glColor4s,
   glColor4sv,
   glColor4ub,
   glColor4ubv,
   glColor4ui,
   glColor4uiv,
   glColor4us,
   glColor4usv,
   glColorMask,
   glColorMaterial,
   glCopyPixels,
   glCullFace,
   glDeleteLists,
   glDepthFunc,
   glDepthMask,
   glDepthRange,
   glDisable,
   glDrawBuffer,
   glDrawPixels,
   glEdgeFlag,
   glEdgeFlagv,
   glEnable,
   glEnd,
   glEndList,
   glEvalCoord1d,
   glEvalCoord1dv,
   glEvalCoord1f,
   glEvalCoord1fv,
   glEvalCoord2d,
   glEvalCoord2dv,
   glEvalCoord2f,
   glEvalCoord2fv,
   glEvalMesh1,
   glEvalMesh2,
   glEvalPoint1,
   glEvalPoint2,
   glFeedbackBuffer,
   glFinish,
   glFlush,
   glFogf,
   glFogfv,
   glFogi,
   glFogiv,
   glFrontFace,
   glFrustum,
   glGenLists,
   glGetBooleanv,
   glGetClipPlane,
   glGetDoublev,
   glGetError,
   glGetFloatv,
   glGetIntegerv,
   glGetLightfv,
   glGetLightiv,
   glGetMapdv,
   glGetMapfv,
   glGetMapiv,
   glGetMaterialfv,
   glGetMaterialiv,
   glGetPixelMapfv,
   glGetPixelMapuiv,
   glGetPixelMapusv,
   glGetPolygonStipple,
   glGetString,
   glGetTexEnvfv,
   glGetTexEnviv,
   glGetTexGendv,
   glGetTexGenfv,
   glGetTexGeniv,
   glGetTexImage,
   glGetTexLevelParameterfv,
   glGetTexLevelParameteriv,
   glGetTexParameterfv,
   glGetTexParameteriv,
   glHint,
   glIndexMask,
   glIndexd,
   glIndexdv,
   glIndexf,
   glIndexfv,
   glIndexi,
   glIndexiv,
   glIndexs,
   glIndexsv,
   glInitNames,
   glIsEnabled,
   glIsList,
   glLightModelf,
   glLightModelfv,
   glLightModeli,
   glLightModeliv,
   glLightf,
   glLightfv,
   glLighti,
   glLightiv,
   glLineStipple,
   glLineWidth,
   glListBase,
   glLoadIdentity,
   glLoadMatrixd,
   glLoadMatrixf,
   glLoadName,
   glLogicOp,
   glMap1d,
   glMap1f,
   glMap2d,
   glMap2f,
   glMapGrid1d,
   glMapGrid1f,
   glMapGrid2d,
   glMapGrid2f,
   glMaterialf,
   glMaterialfv,
   glMateriali,
   glMaterialiv,
   glMatrixMode,
   glMultMatrixd,
   glMultMatrixf,
   glNewList,
   glNormal3b,
   glNormal3bv,
   glNormal3d,
   glNormal3dv,
   glNormal3f,
   glNormal3fv,
   glNormal3i,
   glNormal3iv,
   glNormal3s,
   glNormal3sv,
   glOrtho,
   glPassThrough,
   glPixelMapfv,
   glPixelMapuiv,
   glPixelMapusv,
   glPixelStoref,
   glPixelStorei,
   glPixelTransferf,
   glPixelTransferi,
   glPixelZoom,
   glPointSize,
   glPolygonMode,
   glPolygonOffset,
   glPolygonStipple,
   glPopAttrib,
   glPopMatrix,
   glPopName,
   glPushAttrib,
   glPushMatrix,
   glPushName,
   glRasterPos2d,
   glRasterPos2dv,
   glRasterPos2f,
   glRasterPos2fv,
   glRasterPos2i,
   glRasterPos2iv,
   glRasterPos2s,
   glRasterPos2sv,
   glRasterPos3d,
   glRasterPos3dv,
   glRasterPos3f,
   glRasterPos3fv,
   glRasterPos3i,
   glRasterPos3iv,
   glRasterPos3s,
   glRasterPos3sv,
   glRasterPos4d,
   glRasterPos4dv,
   glRasterPos4f,
   glRasterPos4fv,
   glRasterPos4i,
   glRasterPos4iv,
   glRasterPos4s,
   glRasterPos4sv,
   glReadBuffer,
   glReadPixels,
   glRectd,
   glRectdv,
   glRectf,
   glRectfv,
   glRecti,
   glRectiv,
   glRects,
   glRectsv,
   glRenderMode,
   glRotated,
   glRotatef,
   glScaled,
   glScalef,
   glScissor,
   glSelectBuffer,
   glShadeModel,
   glStencilFunc,
   glStencilMask,
   glStencilOp,
   glTexCoord1d,
   glTexCoord1dv,
   glTexCoord1f,
   glTexCoord1fv,
   glTexCoord1i,
   glTexCoord1iv,
   glTexCoord1s,
   glTexCoord1sv,
   glTexCoord2d,
   glTexCoord2dv,
   glTexCoord2f,
   glTexCoord2fv,
   glTexCoord2i,
   glTexCoord2iv,
   glTexCoord2s,
   glTexCoord2sv,
   glTexCoord3d,
   glTexCoord3dv,
   glTexCoord3f,
   glTexCoord3fv,
   glTexCoord3i,
   glTexCoord3iv,
   glTexCoord3s,
   glTexCoord3sv,
   glTexCoord4d,
   glTexCoord4dv,
   glTexCoord4f,
   glTexCoord4fv,
   glTexCoord4i,
   glTexCoord4iv,
   glTexCoord4s,
   glTexCoord4sv,
   glTexEnvf,
   glTexEnvfv,
   glTexEnvi,
   glTexEnviv,
   glTexGend,
   glTexGendv,
   glTexGenf,
   glTexGenfv,
   glTexGeni,
   glTexGeniv,
   glTexImage1D,
   glTexImage2D,
   glTexParameterf,
   glTexParameterfv,
   glTexParameteri,
   glTexParameteriv,
   glTranslated,
   glTranslatef,
   glVertex2d,
   glVertex2dv,
   glVertex2f,
   glVertex2fv,
   glVertex2i,
   glVertex2iv,
   glVertex2s,
   glVertex2sv,
   glVertex3d,
   glVertex3dv,
   glVertex3f,
   glVertex3fv,
   glVertex3i,
   glVertex3iv,
   glVertex3s,
   glVertex3sv,
   glVertex4d,
   glVertex4dv,
   glVertex4f,
   glVertex4fv,
   glVertex4i,
   glVertex4iv,
   glVertex4s,
   glVertex4sv,
   glViewport,

#ifdef _GLAPI_VERSION_1_1
   glAreTexturesResident,
   glArrayElement,
   glBindTexture,
   glColorPointer,
   glCopyTexImage1D,
   glCopyTexImage2D,
   glCopyTexSubImage1D,
   glCopyTexSubImage2D,
   glDeleteTextures,
   glDisableClientState,
   glDrawArrays,
   glDrawElements,
   glEdgeFlagPointer,
   glEnableClientState,
   glGenTextures,
   glGetPointerv,
   glIndexPointer,
   glIndexub,
   glIndexubv,
   glInterleavedArrays,
   glIsTexture,
   glNormalPointer,
   glPopClientAttrib,
   glPrioritizeTextures,
   glPushClientAttrib,
   glTexCoordPointer,
   glTexSubImage1D,
   glTexSubImage2D,
   glVertexPointer,
#endif

#ifdef _GLAPI_VERSION_1_2
   glCopyTexSubImage3D,
   glDrawRangeElements,
   glTexImage3D,
   glTexSubImage3D,

#ifdef _GLAPI_ARB_imaging
   glBlendColor,
   glBlendEquation,
   glColorSubTable,
   glColorTable,
   glColorTableParameterfv,
   glColorTableParameteriv,
   glConvolutionFilter1D,
   glConvolutionFilter2D,
   glConvolutionParameterf,
   glConvolutionParameterfv,
   glConvolutionParameteri,
   glConvolutionParameteriv,
   glCopyColorSubTable,
   glCopyColorTable,
   glCopyConvolutionFilter1D,
   glCopyConvolutionFilter2D,
   glGetColorTable,
   glGetColorTableParameterfv,
   glGetColorTableParameteriv,
   glGetConvolutionFilter,
   glGetConvolutionParameterfv,
   glGetConvolutionParameteriv,
   glGetHistogram,
   glGetHistogramParameterfv,
   glGetHistogramParameteriv,
   glGetMinmax,
   glGetMinmaxParameterfv,
   glGetMinmaxParameteriv,
   glGetSeparableFilter,
   glHistogram,
   glMinmax,
   glResetHistogram,
   glResetMinmax,
   glSeparableFilter2D,
#endif
#endif


   /*
    * Extensions
    */

#ifdef _GLAPI_EXT_paletted_texture
   glColorTableEXT,
   glColorSubTableEXT,
   glGetColorTableEXT,
   glGetColorTableParameterfvEXT,
   glGetColorTableParameterivEXT,
#endif

#ifdef _GLAPI_EXT_compiled_vertex_array
   glLockArraysEXT,
   glUnlockArraysEXT,
#endif

#ifdef _GLAPI_EXT_point_parameters
   glPointParameterfEXT,
   glPointParameterfvEXT,
#endif

#ifdef _GLAPI_EXT_polygon_offset
   glPolygonOffsetEXT,
#endif

#ifdef _GLAPI_EXT_blend_minmax
   glBlendEquationEXT,
#endif

#ifdef _GLAPI_EXT_blend_color
   glBlendColorEXT,
#endif

#ifdef _GLAPI_ARB_multitexture
   glActiveTextureARB,
   glClientActiveTextureARB,
   glMultiTexCoord1dARB,
   glMultiTexCoord1dvARB,
   glMultiTexCoord1fARB,
   glMultiTexCoord1fvARB,
   glMultiTexCoord1iARB,
   glMultiTexCoord1ivARB,
   glMultiTexCoord1sARB,
   glMultiTexCoord1svARB,
   glMultiTexCoord2dARB,
   glMultiTexCoord2dvARB,
   glMultiTexCoord2fARB,
   glMultiTexCoord2fvARB,
   glMultiTexCoord2iARB,
   glMultiTexCoord2ivARB,
   glMultiTexCoord2sARB,
   glMultiTexCoord2svARB,
   glMultiTexCoord3dARB,
   glMultiTexCoord3dvARB,
   glMultiTexCoord3fARB,
   glMultiTexCoord3fvARB,
   glMultiTexCoord3iARB,
   glMultiTexCoord3ivARB,
   glMultiTexCoord3sARB,
   glMultiTexCoord3svARB,
   glMultiTexCoord4dARB,
   glMultiTexCoord4dvARB,
   glMultiTexCoord4fARB,
   glMultiTexCoord4fvARB,
   glMultiTexCoord4iARB,
   glMultiTexCoord4ivARB,
   glMultiTexCoord4sARB,
   glMultiTexCoord4svARB,
#endif

#ifdef _GLAPI_INGR_blend_func_separate
   glBlendFuncSeparateINGR,
#endif

#ifdef _GLAPI_MESA_window_pos
   glWindowPos4fMESA,
#endif

#ifdef _GLAPI_MESA_resize_buffers
   glResizeBuffersMESA
#endif

};

#endif /*DEBUG*/




/*
 * Set the global or per-thread dispatch table pointer.
 */
void
_glapi_set_dispatch(struct _glapi_table *dispatch)
{
#ifdef DEBUG
   (void) completeness_test;  /* to silence compiler warnings */
#endif

   if (dispatch) {
#ifdef DEBUG
      _glapi_check_table(dispatch);
#endif
#if defined(MULTI_THREAD)
      /* set this thread's dispatch pointer */
      /* XXX To Do */
#else
      Dispatch = dispatch;
#endif
   }
   else {
      /* no current context, each function is a no-op */
#if defined(MULTI_THREAD)
      /* XXX To Do */
#else
      Dispatch = &__glapi_noop_table;
#endif
   }
}


/*
 * Get the global or per-thread dispatch table pointer.
 */
struct _glapi_table *
_glapi_get_dispatch(void)
{
#if defined(MULTI_THREAD)
   /* return this thread's dispatch pointer */
   return NULL;
#else
   return Dispatch;
#endif
}


/*
 * Get API dispatcher version string.
 */
const char *
_glapi_get_version(void)
{
   return "1.2";   /* XXX this isn't well defined yet */
}


/*
 * Return list of hard-coded extension entrypoints in the dispatch table.
 */
const char *
_glapi_get_extensions(void)
{
   return "GL_EXT_paletted_texture GL_EXT_compiled_vertex_array GL_EXT_point_parameters GL_EXT_polygon_offset GL_EXT_blend_minmax GL_EXT_blend_color GL_ARB_multitexture GL_INGR_blend_func_separate GL_MESA_window_pos GL_MESA_resize_buffers";
}



/*
 * Dynamically allocate an extension entry point.  Return a slot number.
 */
GLint
_glapi_alloc_entrypoint(const char *funcName)
{
   /* XXX To Do */
   return -1;
}



/*
 * Find the dynamic entry point for the named function.
 */
GLint
_glapi_get_entrypoint(const char *funcName)
{
   /* XXX To Do */
   return -1;
}


/*
 * Return entrypoint for named function.
 */
const GLvoid *
_glapi_get_proc_address(const char *funcName)
{
   /* XXX To Do */
   return NULL;
}



/*
 * Make sure there are no NULL pointers in the given dispatch table.
 * Intented for debugging purposes.
 */
void
_glapi_check_table(const struct _glapi_table *table)
{
   assert(table->Accum);
   assert(table->AlphaFunc);
   assert(table->Begin);
   assert(table->Bitmap);
   assert(table->BlendFunc);
   assert(table->CallList);
   assert(table->CallLists);
   assert(table->Clear);
   assert(table->ClearAccum);
   assert(table->ClearColor);
   assert(table->ClearDepth);
   assert(table->ClearIndex);
   assert(table->ClearStencil);
   assert(table->ClipPlane);
   assert(table->Color3b);
   assert(table->Color3bv);
   assert(table->Color3d);
   assert(table->Color3dv);
   assert(table->Color3f);
   assert(table->Color3fv);
   assert(table->Color3i);
   assert(table->Color3iv);
   assert(table->Color3s);
   assert(table->Color3sv);
   assert(table->Color3ub);
   assert(table->Color3ubv);
   assert(table->Color3ui);
   assert(table->Color3uiv);
   assert(table->Color3us);
   assert(table->Color3usv);
   assert(table->Color4b);
   assert(table->Color4bv);
   assert(table->Color4d);
   assert(table->Color4dv);
   assert(table->Color4f);
   assert(table->Color4fv);
   assert(table->Color4i);
   assert(table->Color4iv);
   assert(table->Color4s);
   assert(table->Color4sv);
   assert(table->Color4ub);
   assert(table->Color4ubv);
   assert(table->Color4ui);
   assert(table->Color4uiv);
   assert(table->Color4us);
   assert(table->Color4usv);
   assert(table->ColorMask);
   assert(table->ColorMaterial);
   assert(table->CopyPixels);
   assert(table->CullFace);
   assert(table->DeleteLists);
   assert(table->DepthFunc);
   assert(table->DepthMask);
   assert(table->DepthRange);
   assert(table->Disable);
   assert(table->DrawBuffer);
   assert(table->DrawElements);
   assert(table->DrawPixels);
   assert(table->EdgeFlag);
   assert(table->EdgeFlagv);
   assert(table->Enable);
   assert(table->End);
   assert(table->EndList);
   assert(table->EvalCoord1d);
   assert(table->EvalCoord1dv);
   assert(table->EvalCoord1f);
   assert(table->EvalCoord1fv);
   assert(table->EvalCoord2d);
   assert(table->EvalCoord2dv);
   assert(table->EvalCoord2f);
   assert(table->EvalCoord2fv);
   assert(table->EvalMesh1);
   assert(table->EvalMesh2);
   assert(table->EvalPoint1);
   assert(table->EvalPoint2);
   assert(table->FeedbackBuffer);
   assert(table->Finish);
   assert(table->Flush);
   assert(table->Fogf);
   assert(table->Fogfv);
   assert(table->Fogi);
   assert(table->Fogiv);
   assert(table->FrontFace);
   assert(table->Frustum);
   assert(table->GenLists);
   assert(table->GetBooleanv);
   assert(table->GetClipPlane);
   assert(table->GetDoublev);
   assert(table->GetError);
   assert(table->GetFloatv);
   assert(table->GetIntegerv);
   assert(table->GetLightfv);
   assert(table->GetLightiv);
   assert(table->GetMapdv);
   assert(table->GetMapfv);
   assert(table->GetMapiv);
   assert(table->GetMaterialfv);
   assert(table->GetMaterialiv);
   assert(table->GetPixelMapfv);
   assert(table->GetPixelMapuiv);
   assert(table->GetPixelMapusv);
   assert(table->GetPolygonStipple);
   assert(table->GetString);
   assert(table->GetTexEnvfv);
   assert(table->GetTexEnviv);
   assert(table->GetTexGendv);
   assert(table->GetTexGenfv);
   assert(table->GetTexGeniv);
   assert(table->GetTexImage);
   assert(table->GetTexLevelParameterfv);
   assert(table->GetTexLevelParameteriv);
   assert(table->GetTexParameterfv);
   assert(table->GetTexParameteriv);
   assert(table->Hint);
   assert(table->IndexMask);
   assert(table->Indexd);
   assert(table->Indexdv);
   assert(table->Indexf);
   assert(table->Indexfv);
   assert(table->Indexi);
   assert(table->Indexiv);
   assert(table->Indexs);
   assert(table->Indexsv);
   assert(table->InitNames);
   assert(table->IsEnabled);
   assert(table->IsList);
   assert(table->LightModelf);
   assert(table->LightModelfv);
   assert(table->LightModeli);
   assert(table->LightModeliv);
   assert(table->Lightf);
   assert(table->Lightfv);
   assert(table->Lighti);
   assert(table->Lightiv);
   assert(table->LineStipple);
   assert(table->LineWidth);
   assert(table->ListBase);
   assert(table->LoadIdentity);
   assert(table->LoadMatrixd);
   assert(table->LoadMatrixf);
   assert(table->LoadName);
   assert(table->LogicOp);
   assert(table->Map1d);
   assert(table->Map1f);
   assert(table->Map2d);
   assert(table->Map2f);
   assert(table->MapGrid1d);
   assert(table->MapGrid1f);
   assert(table->MapGrid2d);
   assert(table->MapGrid2f);
   assert(table->Materialf);
   assert(table->Materialfv);
   assert(table->Materiali);
   assert(table->Materialiv);
   assert(table->MatrixMode);
   assert(table->MultMatrixd);
   assert(table->MultMatrixf);
   assert(table->NewList);
   assert(table->Normal3b);
   assert(table->Normal3bv);
   assert(table->Normal3d);
   assert(table->Normal3dv);
   assert(table->Normal3f);
   assert(table->Normal3fv);
   assert(table->Normal3i);
   assert(table->Normal3iv);
   assert(table->Normal3s);
   assert(table->Normal3sv);
   assert(table->Ortho);
   assert(table->PassThrough);
   assert(table->PixelMapfv);
   assert(table->PixelMapuiv);
   assert(table->PixelMapusv);
   assert(table->PixelStoref);
   assert(table->PixelStorei);
   assert(table->PixelTransferf);
   assert(table->PixelTransferi);
   assert(table->PixelZoom);
   assert(table->PointSize);
   assert(table->PolygonMode);
   assert(table->PolygonOffset);
   assert(table->PolygonStipple);
   assert(table->PopAttrib);
   assert(table->PopMatrix);
   assert(table->PopName);
   assert(table->PushAttrib);
   assert(table->PushMatrix);
   assert(table->PushName);
   assert(table->RasterPos2d);
   assert(table->RasterPos2dv);
   assert(table->RasterPos2f);
   assert(table->RasterPos2fv);
   assert(table->RasterPos2i);
   assert(table->RasterPos2iv);
   assert(table->RasterPos2s);
   assert(table->RasterPos2sv);
   assert(table->RasterPos3d);
   assert(table->RasterPos3dv);
   assert(table->RasterPos3f);
   assert(table->RasterPos3fv);
   assert(table->RasterPos3i);
   assert(table->RasterPos3iv);
   assert(table->RasterPos3s);
   assert(table->RasterPos3sv);
   assert(table->RasterPos4d);
   assert(table->RasterPos4dv);
   assert(table->RasterPos4f);
   assert(table->RasterPos4fv);
   assert(table->RasterPos4i);
   assert(table->RasterPos4iv);
   assert(table->RasterPos4s);
   assert(table->RasterPos4sv);
   assert(table->ReadBuffer);
   assert(table->ReadPixels);
   assert(table->Rectd);
   assert(table->Rectdv);
   assert(table->Rectf);
   assert(table->Rectfv);
   assert(table->Recti);
   assert(table->Rectiv);
   assert(table->Rects);
   assert(table->Rectsv);
   assert(table->RenderMode);
   assert(table->Rotated);
   assert(table->Rotatef);
   assert(table->Scaled);
   assert(table->Scalef);
   assert(table->Scissor);
   assert(table->SelectBuffer);
   assert(table->ShadeModel);
   assert(table->StencilFunc);
   assert(table->StencilMask);
   assert(table->StencilOp);
   assert(table->TexCoord1d);
   assert(table->TexCoord1dv);
   assert(table->TexCoord1f);
   assert(table->TexCoord1fv);
   assert(table->TexCoord1i);
   assert(table->TexCoord1iv);
   assert(table->TexCoord1s);
   assert(table->TexCoord1sv);
   assert(table->TexCoord2d);
   assert(table->TexCoord2dv);
   assert(table->TexCoord2f);
   assert(table->TexCoord2fv);
   assert(table->TexCoord2i);
   assert(table->TexCoord2iv);
   assert(table->TexCoord2s);
   assert(table->TexCoord2sv);
   assert(table->TexCoord3d);
   assert(table->TexCoord3dv);
   assert(table->TexCoord3f);
   assert(table->TexCoord3fv);
   assert(table->TexCoord3i);
   assert(table->TexCoord3iv);
   assert(table->TexCoord3s);
   assert(table->TexCoord3sv);
   assert(table->TexCoord4d);
   assert(table->TexCoord4dv);
   assert(table->TexCoord4f);
   assert(table->TexCoord4fv);
   assert(table->TexCoord4i);
   assert(table->TexCoord4iv);
   assert(table->TexCoord4s);
   assert(table->TexCoord4sv);
   assert(table->TexEnvf);
   assert(table->TexEnvfv);
   assert(table->TexEnvi);
   assert(table->TexEnviv);
   assert(table->TexGend);
   assert(table->TexGendv);
   assert(table->TexGenf);
   assert(table->TexGenfv);
   assert(table->TexGeni);
   assert(table->TexGeniv);
   assert(table->TexImage1D);
   assert(table->TexImage2D);
   assert(table->TexParameterf);
   assert(table->TexParameterfv);
   assert(table->TexParameteri);
   assert(table->TexParameteriv);
   assert(table->Translated);
   assert(table->Translatef);
   assert(table->Vertex2d);
   assert(table->Vertex2dv);
   assert(table->Vertex2f);
   assert(table->Vertex2fv);
   assert(table->Vertex2i);
   assert(table->Vertex2iv);
   assert(table->Vertex2s);
   assert(table->Vertex2sv);
   assert(table->Vertex3d);
   assert(table->Vertex3dv);
   assert(table->Vertex3f);
   assert(table->Vertex3fv);
   assert(table->Vertex3i);
   assert(table->Vertex3iv);
   assert(table->Vertex3s);
   assert(table->Vertex3sv);
   assert(table->Vertex4d);
   assert(table->Vertex4dv);
   assert(table->Vertex4f);
   assert(table->Vertex4fv);
   assert(table->Vertex4i);
   assert(table->Vertex4iv);
   assert(table->Vertex4s);
   assert(table->Vertex4sv);
   assert(table->Viewport);

#ifdef _GLAPI_VERSION_1_1
   assert(table->AreTexturesResident);
   assert(table->ArrayElement);
   assert(table->BindTexture);
   assert(table->ColorPointer);
   assert(table->CopyTexImage1D);
   assert(table->CopyTexImage2D);
   assert(table->CopyTexSubImage1D);
   assert(table->CopyTexSubImage2D);
   assert(table->DeleteTextures);
   assert(table->DisableClientState);
   assert(table->DrawArrays);
   assert(table->EdgeFlagPointer);
   assert(table->EnableClientState);
   assert(table->GenTextures);
   assert(table->GetPointerv);
   assert(table->IndexPointer);
   assert(table->Indexub);
   assert(table->Indexubv);
   assert(table->InterleavedArrays);
   assert(table->IsTexture);
   assert(table->NormalPointer);
   assert(table->PopClientAttrib);
   assert(table->PrioritizeTextures);
   assert(table->PushClientAttrib);
   assert(table->TexCoordPointer);
   assert(table->TexSubImage1D);
   assert(table->TexSubImage2D);
   assert(table->VertexPointer);
#endif

#ifdef _GLAPI_VERSION_1_2
   assert(table->CopyTexSubImage3D);
   assert(table->DrawRangeElements);
   assert(table->TexImage3D);
   assert(table->TexSubImage3D);
#ifdef _GLAPI_ARB_imaging
   assert(table->BlendColor);
   assert(table->BlendEquation);
   assert(table->ColorSubTable);
   assert(table->ColorTable);
   assert(table->ColorTableParameterfv);
   assert(table->ColorTableParameteriv);
   assert(table->ConvolutionFilter1D);
   assert(table->ConvolutionFilter2D);
   assert(table->ConvolutionParameterf);
   assert(table->ConvolutionParameterfv);
   assert(table->ConvolutionParameteri);
   assert(table->ConvolutionParameteriv);
   assert(table->CopyColorSubTable);
   assert(table->CopyColorTable);
   assert(table->CopyConvolutionFilter1D);
   assert(table->CopyConvolutionFilter2D);
   assert(table->GetColorTable);
   assert(table->GetColorTableParameterfv);
   assert(table->GetColorTableParameteriv);
   assert(table->GetConvolutionFilter);
   assert(table->GetConvolutionParameterfv);
   assert(table->GetConvolutionParameteriv);
   assert(table->GetHistogram);
   assert(table->GetHistogramParameterfv);
   assert(table->GetHistogramParameteriv);
   assert(table->GetMinmax);
   assert(table->GetMinmaxParameterfv);
   assert(table->GetMinmaxParameteriv);
   assert(table->Histogram);
   assert(table->Minmax);
   assert(table->ResetHistogram);
   assert(table->ResetMinmax);
   assert(table->SeparableFilter2D);
#endif
#endif


#ifdef _GLAPI_EXT_paletted_texture
   assert(table->ColorTableEXT);
   assert(table->ColorSubTableEXT);
   assert(table->GetColorTableEXT);
   assert(table->GetColorTableParameterfvEXT);
   assert(table->GetColorTableParameterivEXT);
#endif

#ifdef _GLAPI_EXT_compiled_vertex_array
   assert(table->LockArraysEXT);
   assert(table->UnlockArraysEXT);
#endif

#ifdef _GLAPI_EXT_point_parameter
   assert(table->PointParameterfEXT);
   assert(table->PointParameterfvEXT);
#endif

#ifdef _GLAPI_EXT_polygon_offset
   assert(table->PolygonOffsetEXT);
#endif

#ifdef _GLAPI_ARB_multitexture
   assert(table->ActiveTextureARB);
   assert(table->ClientActiveTextureARB);
   assert(table->MultiTexCoord1dARB);
   assert(table->MultiTexCoord1dvARB);
   assert(table->MultiTexCoord1fARB);
   assert(table->MultiTexCoord1fvARB);
   assert(table->MultiTexCoord1iARB);
   assert(table->MultiTexCoord1ivARB);
   assert(table->MultiTexCoord1sARB);
   assert(table->MultiTexCoord1svARB);
   assert(table->MultiTexCoord2dARB);
   assert(table->MultiTexCoord2dvARB);
   assert(table->MultiTexCoord2fARB);
   assert(table->MultiTexCoord2fvARB);
   assert(table->MultiTexCoord2iARB);
   assert(table->MultiTexCoord2ivARB);
   assert(table->MultiTexCoord2sARB);
   assert(table->MultiTexCoord2svARB);
   assert(table->MultiTexCoord3dARB);
   assert(table->MultiTexCoord3dvARB);
   assert(table->MultiTexCoord3fARB);
   assert(table->MultiTexCoord3fvARB);
   assert(table->MultiTexCoord3iARB);
   assert(table->MultiTexCoord3ivARB);
   assert(table->MultiTexCoord3sARB);
   assert(table->MultiTexCoord3svARB);
   assert(table->MultiTexCoord4dARB);
   assert(table->MultiTexCoord4dvARB);
   assert(table->MultiTexCoord4fARB);
   assert(table->MultiTexCoord4fvARB);
   assert(table->MultiTexCoord4iARB);
   assert(table->MultiTexCoord4ivARB);
   assert(table->MultiTexCoord4sARB);
   assert(table->MultiTexCoord4svARB);
#endif

#ifdef _GLAPI_INGR_blend_func_separate
   assert(table->BlendFuncSeparateINGR);
#endif

#ifdef _GLAPI_MESA_window_pos
   assert(table->WindowPos4fMESA);
#endif

#ifdef _GLAPI_MESA_resize_buffers
   assert(table->ResizeBuffersMESA);
#endif
}


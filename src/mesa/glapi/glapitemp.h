/* $Id: glapitemp.h,v 1.8 2000/01/10 04:29:10 brianp Exp $ */

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



/*
 * XXX
 * Most functions need the msg (printf-message) parameter to be finished.
 * I.e. replace ";\n" with the real info.
 */


/* GL 1.0 */

KEYWORD1 void KEYWORD2 NAME(Accum)(GLenum op, GLfloat value)
{
   DISPATCH(Accum, (op, value), ("glAccum(0x%x, %g);\n", op, value));
}

KEYWORD1 void KEYWORD2 NAME(AlphaFunc)(GLenum func, GLclampf ref)
{
   DISPATCH(AlphaFunc, (func, ref), ("glAlphaFunc(0x%x, %g);\n", func, ref));
}

KEYWORD1 void KEYWORD2 NAME(Begin)(GLenum mode)
{
   DISPATCH(Begin, (mode), ("glBegin(0x%x);\n", mode));
}

KEYWORD1 void KEYWORD2 NAME(Bitmap)(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap)
{
   DISPATCH(Bitmap, (width, height, xorig, yorig, xmove, ymove, bitmap), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(BlendFunc)(GLenum sfactor, GLenum dfactor)
{
   DISPATCH(BlendFunc, (sfactor, dfactor), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(CallList)(GLuint list)
{
   DISPATCH(CallList, (list), ("glCallList(%u);\n", list));
}

KEYWORD1 void KEYWORD2 NAME(CallLists)(GLsizei n, GLenum type, const GLvoid *lists)
{
   DISPATCH(CallLists, (n, type, lists), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Clear)(GLbitfield mask)
{
   DISPATCH(Clear, (mask), ("glClear(0x%x);\n", mask));
}

KEYWORD1 void KEYWORD2 NAME(ClearAccum)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
   DISPATCH(ClearAccum, (red, green, blue, alpha), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(ClearColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
   DISPATCH(ClearColor, (red, green, blue, alpha), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(ClearDepth)(GLclampd depth)
{
   DISPATCH(ClearDepth, (depth), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(ClearIndex)(GLfloat c)
{
   DISPATCH(ClearIndex, (c), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(ClearStencil)(GLint s)
{
   DISPATCH(ClearStencil, (s), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(ClipPlane)(GLenum plane, const GLdouble *equation)
{
   DISPATCH(ClipPlane, (plane, equation), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Color3b)(GLbyte red, GLbyte green, GLbyte blue)
{
   DISPATCH(Color3b, (red, green, blue), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Color3d)(GLdouble red, GLdouble green, GLdouble blue)
{
   DISPATCH(Color3d, (red, green, blue), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Color3f)(GLfloat red, GLfloat green, GLfloat blue)
{
   DISPATCH(Color3f, (red, green, blue), ("glColor3f(%g, %g, %g);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(Color3i)(GLint red, GLint green, GLint blue)
{
   DISPATCH(Color3i, (red, green, blue), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Color3s)(GLshort red, GLshort green, GLshort blue)
{
   DISPATCH(Color3s, (red, green, blue), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Color3ub)(GLubyte red, GLubyte green, GLubyte blue)
{
   DISPATCH(Color3ub, (red, green, blue), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Color3ui)(GLuint red, GLuint green, GLuint blue)
{
   DISPATCH(Color3ui, (red, green, blue), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Color3us)(GLushort red, GLushort green, GLushort blue)
{
   DISPATCH(Color3us, (red, green, blue), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Color4b)(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha)
{
   DISPATCH(Color4b, (red, green, blue, alpha), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Color4d)(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha)
{
   DISPATCH(Color4d, (red, green, blue, alpha), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Color4f)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
   DISPATCH(Color4f, (red, green, blue, alpha), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Color4i)(GLint red, GLint green, GLint blue, GLint alpha)
{
   DISPATCH(Color4i, (red, green, blue, alpha), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Color4s)(GLshort red, GLshort green, GLshort blue, GLshort alpha)
{
   DISPATCH(Color4s, (red, green, blue, alpha), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Color4ub)(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
   DISPATCH(Color4ub, (red, green, blue, alpha), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Color4ui)(GLuint red, GLuint green, GLuint blue, GLuint alpha)
{
   DISPATCH(Color4ui, (red, green, blue, alpha), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Color4us)(GLushort red, GLushort green, GLushort blue, GLushort alpha)
{
   DISPATCH(Color4us, (red, green, blue, alpha), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Color3bv)(const GLbyte *v)
{
   DISPATCH(Color3bv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Color3dv)(const GLdouble *v)
{
   DISPATCH(Color3dv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Color3fv)(const GLfloat *v)
{
   DISPATCH(Color3fv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Color3iv)(const GLint *v)
{
   DISPATCH(Color3iv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Color3sv)(const GLshort *v)
{
   DISPATCH(Color3sv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Color3ubv)(const GLubyte *v)
{
   DISPATCH(Color3ubv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Color3uiv)(const GLuint *v)
{
   DISPATCH(Color3uiv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Color3usv)(const GLushort *v)
{
   DISPATCH(Color3usv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Color4bv)(const GLbyte *v)
{
   DISPATCH(Color4bv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Color4dv)(const GLdouble *v)
{
   DISPATCH(Color4dv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Color4fv)(const GLfloat *v)
{
   DISPATCH(Color4fv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Color4iv)(const GLint *v)
{
   DISPATCH(Color4iv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Color4sv)(const GLshort *v)
{
   DISPATCH(Color4sv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Color4ubv)(const GLubyte *v)
{
   DISPATCH(Color4ubv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Color4uiv)(const GLuint *v)
{
   DISPATCH(Color4uiv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Color4usv)(const GLushort *v)
{
   DISPATCH(Color4usv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(ColorMask)(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
   DISPATCH(ColorMask, (red, green, blue, alpha), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(ColorMaterial)(GLenum face, GLenum mode)
{
   DISPATCH(ColorMaterial, (face, mode), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(CopyPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
{
   DISPATCH(CopyPixels, (x, y, width, height, type), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(CullFace)(GLenum mode)
{
   DISPATCH(CullFace, (mode), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(DepthFunc)(GLenum func)
{
   DISPATCH(DepthFunc, (func), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(DepthMask)(GLboolean flag)
{
   DISPATCH(DepthMask, (flag), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(DepthRange)(GLclampd nearVal, GLclampd farVal)
{
   DISPATCH(DepthRange, (nearVal, farVal), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(DeleteLists)(GLuint list, GLsizei range)
{
   DISPATCH(DeleteLists, (list, range), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Disable)(GLenum cap)
{
   DISPATCH(Disable, (cap), ("glDisable(0x%x);\n", cap));
}

KEYWORD1 void KEYWORD2 NAME(DrawBuffer)(GLenum mode)
{
   DISPATCH(DrawBuffer, (mode), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(DrawElements)(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)
{
   DISPATCH(DrawElements, (mode, count, type, indices), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(DrawPixels)(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
   DISPATCH(DrawPixels, (width, height, format, type, pixels), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Enable)(GLenum cap)
{
   DISPATCH(Enable, (cap), ("glEnable(0x%x);\n", cap));
}

KEYWORD1 void KEYWORD2 NAME(End)(void)
{
   DISPATCH(End, (), ("glEnd();\n"));
}

KEYWORD1 void KEYWORD2 NAME(EndList)(void)
{
   DISPATCH(EndList, (), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(EvalCoord1d)(GLdouble u)
{
   DISPATCH(EvalCoord1d, (u), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(EvalCoord1f)(GLfloat u)
{
   DISPATCH(EvalCoord1f, (u), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(EvalCoord1dv)(const GLdouble *u)
{
   DISPATCH(EvalCoord1dv, (u), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(EvalCoord1fv)(const GLfloat *u)
{
   DISPATCH(EvalCoord1fv, (u), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(EvalCoord2d)(GLdouble u, GLdouble v)
{
   DISPATCH(EvalCoord2d, (u, v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(EvalCoord2f)(GLfloat u, GLfloat v)
{
   DISPATCH(EvalCoord2f, (u, v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(EvalCoord2dv)(const GLdouble *u)
{
   DISPATCH(EvalCoord2dv, (u), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(EvalCoord2fv)(const GLfloat *u)
{
   DISPATCH(EvalCoord2fv, (u), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(EvalPoint1)(GLint i)
{
   DISPATCH(EvalPoint1, (i), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(EvalPoint2)(GLint i, GLint j)
{
   DISPATCH(EvalPoint2, (i, j), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(EvalMesh1)(GLenum mode, GLint i1, GLint i2)
{
   DISPATCH(EvalMesh1, (mode, i1, i2), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(EdgeFlag)(GLboolean flag)
{
   DISPATCH(EdgeFlag, (flag), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(EdgeFlagv)(const GLboolean *flag)
{
   DISPATCH(EdgeFlagv, (flag), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(EvalMesh2)(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2)
{
   DISPATCH(EvalMesh2, (mode, i1, i2, j1, j2), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(FeedbackBuffer)(GLsizei size, GLenum type, GLfloat *buffer)
{
   DISPATCH(FeedbackBuffer, (size, type, buffer), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Finish)(void)
{
   DISPATCH(Finish, (), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Flush)(void)
{
   DISPATCH(Flush, (), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Fogf)(GLenum pname, GLfloat param)
{
   DISPATCH(Fogf, (pname, param), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Fogi)(GLenum pname, GLint param)
{
   DISPATCH(Fogi, (pname, param), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Fogfv)(GLenum pname, const GLfloat *params)
{
   DISPATCH(Fogfv, (pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Fogiv)(GLenum pname, const GLint *params)
{
   DISPATCH(Fogiv, (pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(FrontFace)(GLenum mode)
{
   DISPATCH(FrontFace, (mode), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Frustum)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble nearval, GLdouble farval)
{
   DISPATCH(Frustum, (left, right, bottom, top, nearval, farval), (";\n"));
}

KEYWORD1 GLuint KEYWORD2 NAME(GenLists)(GLsizei range)
{
   RETURN_DISPATCH(GenLists, (range), ("glGenLists(%d);\n", range));
}

KEYWORD1 void KEYWORD2 NAME(GetBooleanv)(GLenum pname, GLboolean *params)
{
   DISPATCH(GetBooleanv, (pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetClipPlane)(GLenum plane, GLdouble *equation)
{
   DISPATCH(GetClipPlane, (plane, equation), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetDoublev)(GLenum pname, GLdouble *params)
{
   DISPATCH(GetDoublev, (pname, params), (";\n"));
}

KEYWORD1 GLenum KEYWORD2 NAME(GetError)(void)
{
   RETURN_DISPATCH(GetError, (), ("glGetError();\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetFloatv)(GLenum pname, GLfloat *params)
{
   DISPATCH(GetFloatv, (pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetIntegerv)(GLenum pname, GLint *params)
{
   DISPATCH(GetIntegerv, (pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetLightfv)(GLenum light, GLenum pname, GLfloat *params)
{
   DISPATCH(GetLightfv, (light, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetLightiv)(GLenum light, GLenum pname, GLint *params)
{
   DISPATCH(GetLightiv, (light, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetMapdv)(GLenum target, GLenum query, GLdouble *v)
{
   DISPATCH(GetMapdv, (target, query, v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetMapfv)(GLenum target, GLenum query, GLfloat *v)
{
   DISPATCH(GetMapfv, (target, query, v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetMapiv)(GLenum target, GLenum query, GLint *v)
{
   DISPATCH(GetMapiv, (target, query, v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetMaterialfv)(GLenum face, GLenum pname, GLfloat *params)
{
   DISPATCH(GetMaterialfv, (face, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetMaterialiv)(GLenum face, GLenum pname, GLint *params)
{
   DISPATCH(GetMaterialiv, (face, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetPixelMapfv)(GLenum map, GLfloat *values)
{
   DISPATCH(GetPixelMapfv, (map, values), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetPixelMapuiv)(GLenum map, GLuint *values)
{
   DISPATCH(GetPixelMapuiv, (map, values), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetPixelMapusv)(GLenum map, GLushort *values)
{
   DISPATCH(GetPixelMapusv, (map, values), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetPolygonStipple)(GLubyte *mask)
{
   DISPATCH(GetPolygonStipple, (mask), (";\n"));
}

KEYWORD1 const GLubyte * KEYWORD2 NAME(GetString)(GLenum name)
{
   RETURN_DISPATCH(GetString, (name), ("glGetString(0x%x);\n", name));
}

KEYWORD1 void KEYWORD2 NAME(GetTexEnvfv)(GLenum target, GLenum pname, GLfloat *params)
{
   DISPATCH(GetTexEnvfv, (target, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetTexEnviv)(GLenum target, GLenum pname, GLint *params)
{
   DISPATCH(GetTexEnviv, (target, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetTexGeniv)(GLenum target, GLenum pname, GLint *params)
{
   DISPATCH(GetTexGeniv, (target, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetTexGendv)(GLenum target, GLenum pname, GLdouble *params)
{
   DISPATCH(GetTexGendv, (target, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetTexGenfv)(GLenum target, GLenum pname, GLfloat *params)
{
   DISPATCH(GetTexGenfv, (target, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetTexImage)(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels)
{
   DISPATCH(GetTexImage, (target, level, format, type, pixels), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetTexLevelParameterfv)(GLenum target, GLint level, GLenum pname, GLfloat *params)
{
   DISPATCH(GetTexLevelParameterfv, (target, level, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetTexLevelParameteriv)(GLenum target, GLint level, GLenum pname, GLint *params)
{
   DISPATCH(GetTexLevelParameteriv, (target, level, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetTexParameterfv)(GLenum target, GLenum pname, GLfloat *params)
{
   DISPATCH(GetTexParameterfv, (target, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetTexParameteriv)(GLenum target, GLenum pname, GLint *params)
{
   DISPATCH(GetTexParameteriv, (target, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Hint)(GLenum target, GLenum mode)
{
   DISPATCH(Hint, (target, mode), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Indexd)(GLdouble c)
{
   DISPATCH(Indexd, (c), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Indexdv)(const GLdouble *c)
{
   DISPATCH(Indexdv, (c), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Indexf)(GLfloat c)
{
   DISPATCH(Indexf, (c), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Indexfv)(const GLfloat *c)
{
   DISPATCH(Indexfv, (c), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Indexi)(GLint c)
{
   DISPATCH(Indexi, (c), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Indexiv)(const GLint *c)
{
   DISPATCH(Indexiv, (c), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Indexs)(GLshort c)
{
   DISPATCH(Indexs, (c), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Indexsv)(const GLshort *c)
{
   DISPATCH(Indexsv, (c), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(IndexMask)(GLuint mask)
{
   DISPATCH(IndexMask, (mask), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(InitNames)(void)
{
   DISPATCH(InitNames, (), (";\n"));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsEnabled)(GLenum cap)
{
   RETURN_DISPATCH(IsEnabled, (cap), ("glIsEnabled(0x%x);\n", cap));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsList)(GLuint list)
{
   RETURN_DISPATCH(IsList, (list), ("glIsList(%u);\n", list));
}

KEYWORD1 void KEYWORD2 NAME(Lightf)(GLenum light, GLenum pname, GLfloat param)
{
   DISPATCH(Lightf, (light, pname, param), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Lighti)(GLenum light, GLenum pname, GLint param)
{
   DISPATCH(Lighti, (light, pname, param), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Lightfv)(GLenum light, GLenum pname, const GLfloat *params)
{
   DISPATCH(Lightfv, (light, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Lightiv)(GLenum light, GLenum pname, const GLint *params)
{
   DISPATCH(Lightiv, (light, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(LightModelf)(GLenum pname, GLfloat param)
{
   DISPATCH(LightModelf, (pname, param), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(LightModeli)(GLenum pname, GLint param)
{
   DISPATCH(LightModeli, (pname, param), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(LightModelfv)(GLenum pname, const GLfloat *params)
{
   DISPATCH(LightModelfv, (pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(LightModeliv)(GLenum pname, const GLint *params)
{
   DISPATCH(LightModeliv, (pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(LineWidth)(GLfloat width)
{
   DISPATCH(LineWidth, (width), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(LineStipple)(GLint factor, GLushort pattern)
{
   DISPATCH(LineStipple, (factor, pattern), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(ListBase)(GLuint base)
{
   DISPATCH(ListBase, (base), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(LoadIdentity)(void)
{
   DISPATCH(LoadIdentity, (), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(LoadMatrixd)(const GLdouble *m)
{
   DISPATCH(LoadMatrixd, (m), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(LoadMatrixf)(const GLfloat *m)
{
   DISPATCH(LoadMatrixf, (m), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(LoadName)(GLuint name)
{
   DISPATCH(LoadName, (name), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(LogicOp)(GLenum opcode)
{
   DISPATCH(LogicOp, (opcode), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Map1d)(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points)
{
   DISPATCH(Map1d, (target, u1, u2, stride, order, points), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Map1f)(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points)
{
   DISPATCH(Map1f, (target, u1, u2, stride, order, points), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Map2d)(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points)
{
   DISPATCH(Map2d, (target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Map2f)(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points)
{
   DISPATCH(Map2f, (target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(MapGrid1d)(GLint un, GLdouble u1, GLdouble u2)
{
   DISPATCH(MapGrid1d, (un, u1, u2), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(MapGrid1f)(GLint un, GLfloat u1, GLfloat u2)
{
   DISPATCH(MapGrid1f, (un, u1, u2), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(MapGrid2d)(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2)
{
   DISPATCH(MapGrid2d, (un, u1, u2, vn, v1, v2), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(MapGrid2f)(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2)
{
   DISPATCH(MapGrid2f, (un, u1, u2, vn, v1, v2), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Materialf)(GLenum face, GLenum pname, GLfloat param)
{
   DISPATCH(Materialf, (face, pname, param), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Materiali)(GLenum face, GLenum pname, GLint param)
{
   DISPATCH(Materiali, (face, pname, param), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Materialfv)(GLenum face, GLenum pname, const GLfloat *params)
{
   DISPATCH(Materialfv, (face, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Materialiv)(GLenum face, GLenum pname, const GLint *params)
{
   DISPATCH(Materialiv, (face, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(MatrixMode)(GLenum mode)
{
   DISPATCH(MatrixMode, (mode), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(MultMatrixd)(const GLdouble *m)
{
   DISPATCH(MultMatrixd, (m), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(MultMatrixf)(const GLfloat *m)
{
   DISPATCH(MultMatrixf, (m), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(NewList)(GLuint list, GLenum mode)
{
   DISPATCH(NewList, (list, mode), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Normal3b)(GLbyte nx, GLbyte ny, GLbyte nz)
{
   DISPATCH(Normal3b, (nx, ny, nz), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Normal3bv)(const GLbyte *v)
{
   DISPATCH(Normal3bv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Normal3d)(GLdouble nx, GLdouble ny, GLdouble nz)
{
   DISPATCH(Normal3d, (nx, ny, nz), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Normal3dv)(const GLdouble *v)
{
   DISPATCH(Normal3dv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Normal3f)(GLfloat nx, GLfloat ny, GLfloat nz)
{
   DISPATCH(Normal3f, (nx, ny, nz), ("glNormal3f(%g, %g, %g);\n", nx, ny, nz));
}

KEYWORD1 void KEYWORD2 NAME(Normal3fv)(const GLfloat *v)
{
   DISPATCH(Normal3fv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Normal3i)(GLint nx, GLint ny, GLint nz)
{
   DISPATCH(Normal3i, (nx, ny, nz), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Normal3iv)(const GLint *v)
{
   DISPATCH(Normal3iv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Normal3s)(GLshort nx, GLshort ny, GLshort nz)
{
   DISPATCH(Normal3s, (nx, ny, nz), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Normal3sv)(const GLshort *v)
{
   DISPATCH(Normal3sv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Ortho)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble nearval, GLdouble farval)
{
   DISPATCH(Ortho, (left, right, bottom, top, nearval, farval), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(PassThrough)(GLfloat token)
{
   DISPATCH(PassThrough, (token), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(PixelMapfv)(GLenum map, GLint mapsize, const GLfloat *values)
{
   DISPATCH(PixelMapfv, (map, mapsize, values), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(PixelMapuiv)(GLenum map, GLint mapsize, const GLuint *values)
{
   DISPATCH(PixelMapuiv, (map, mapsize, values), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(PixelMapusv)(GLenum map, GLint mapsize, const GLushort *values)
{
   DISPATCH(PixelMapusv, (map, mapsize, values), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(PixelStoref)(GLenum pname, GLfloat param)
{
   DISPATCH(PixelStoref, (pname, param), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(PixelStorei)(GLenum pname, GLint param)
{
   DISPATCH(PixelStorei, (pname, param), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(PixelTransferf)(GLenum pname, GLfloat param)
{
   DISPATCH(PixelTransferf, (pname, param), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(PixelTransferi)(GLenum pname, GLint param)
{
   DISPATCH(PixelTransferi, (pname, param), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(PixelZoom)(GLfloat xfactor, GLfloat yfactor)
{
   DISPATCH(PixelZoom, (xfactor, yfactor), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(PointSize)(GLfloat size)
{
   DISPATCH(PointSize, (size), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(PolygonMode)(GLenum face, GLenum mode)
{
   DISPATCH(PolygonMode, (face, mode), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(PolygonStipple)(const GLubyte *pattern)
{
   DISPATCH(PolygonStipple, (pattern), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(PopAttrib)(void)
{
   DISPATCH(PopAttrib, (), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(PopMatrix)(void)
{
   DISPATCH(PopMatrix, (), ("glPopMatrix();\n"));
}

KEYWORD1 void KEYWORD2 NAME(PopName)(void)
{
   DISPATCH(PopName, (), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(PushAttrib)(GLbitfield mask)
{
   DISPATCH(PushAttrib, (mask), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(PushMatrix)(void)
{
   DISPATCH(PushMatrix, (), ("glPushMatrix();\n"));
}

KEYWORD1 void KEYWORD2 NAME(PushName)(GLuint name)
{
   DISPATCH(PushName, (name), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos2d)(GLdouble x, GLdouble y)
{
   DISPATCH(RasterPos2d, (x, y), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos2f)(GLfloat x, GLfloat y)
{
   DISPATCH(RasterPos2f, (x, y), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos2i)(GLint x, GLint y)
{
   DISPATCH(RasterPos2i, (x, y), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos2s)(GLshort x, GLshort y)
{
   DISPATCH(RasterPos2s, (x, y), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos3d)(GLdouble x, GLdouble y, GLdouble z)
{
   DISPATCH(RasterPos3d, (x, y, z), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos3f)(GLfloat x, GLfloat y, GLfloat z)
{
   DISPATCH(RasterPos3f, (x, y, z), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos3i)(GLint x, GLint y, GLint z)
{
   DISPATCH(RasterPos3i, (x, y, z), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos3s)(GLshort x, GLshort y, GLshort z)
{
   DISPATCH(RasterPos3s, (x, y, z), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos4d)(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   DISPATCH(RasterPos4d, (x, y, z, w), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   DISPATCH(RasterPos4f, (x, y, z, w), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos4i)(GLint x, GLint y, GLint z, GLint w)
{
   DISPATCH(RasterPos4i, (x, y, z, w), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos4s)(GLshort x, GLshort y, GLshort z, GLshort w)
{
   DISPATCH(RasterPos4s, (x, y, z, w), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos2dv)(const GLdouble *v)
{
   DISPATCH(RasterPos2dv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos2fv)(const GLfloat *v)
{
   DISPATCH(RasterPos2fv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos2iv)(const GLint *v)
{
   DISPATCH(RasterPos2iv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos2sv)(const GLshort *v)
{
   DISPATCH(RasterPos2sv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos3dv)(const GLdouble *v)
{
   DISPATCH(RasterPos3dv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos3fv)(const GLfloat *v)
{
   DISPATCH(RasterPos3fv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos3iv)(const GLint *v)
{
   DISPATCH(RasterPos3iv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos3sv)(const GLshort *v)
{
   DISPATCH(RasterPos3sv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos4dv)(const GLdouble *v)
{
   DISPATCH(RasterPos4dv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos4fv)(const GLfloat *v)
{
   DISPATCH(RasterPos4fv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos4iv)(const GLint *v)
{
   DISPATCH(RasterPos4iv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos4sv)(const GLshort *v)
{
   DISPATCH(RasterPos4sv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(ReadBuffer)(GLenum mode)
{
   DISPATCH(ReadBuffer, (mode), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(ReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels)
{
   DISPATCH(ReadPixels, (x, y, width, height, format, type, pixels), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Rectd)(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2)
{
   DISPATCH(Rectd, (x1, y1, x2, y2), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Rectdv)(const GLdouble *v1, const GLdouble *v2)
{
   DISPATCH(Rectdv, (v1, v2), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Rectf)(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
   DISPATCH(Rectf, (x1, y1, x2, y2), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Rectfv)(const GLfloat *v1, const GLfloat *v2)
{
   DISPATCH(Rectfv, (v1, v2), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Recti)(GLint x1, GLint y1, GLint x2, GLint y2)
{
   DISPATCH(Recti, (x1, y1, x2, y2), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Rectiv)(const GLint *v1, const GLint *v2)
{
   DISPATCH(Rectiv, (v1, v2), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Rects)(GLshort x1, GLshort y1, GLshort x2, GLshort y2)
{
   DISPATCH(Rects, (x1, y1, x2, y2), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Rectsv)(const GLshort *v1, const GLshort *v2)
{
   DISPATCH(Rectsv, (v1, v2), (";\n"));
}

KEYWORD1 GLint KEYWORD2 NAME(RenderMode)(GLenum mode)
{
   RETURN_DISPATCH(RenderMode, (mode), ("glRenderMode(0x%x);\n", mode));
}

KEYWORD1 void KEYWORD2 NAME(Rotated)(GLdouble angle, GLdouble x, GLdouble y, GLdouble z)
{
   DISPATCH(Rotated, (angle, x, y, z), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Rotatef)(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
   DISPATCH(Rotatef, (angle, x, y, z), ("glRotatef(%g, %g, %g, %g);\n", angle, x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(SelectBuffer)(GLsizei size, GLuint *buffer)
{
   DISPATCH(SelectBuffer, (size, buffer), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Scaled)(GLdouble x, GLdouble y, GLdouble z)
{
   DISPATCH(Scaled, (x, y, z), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Scalef)(GLfloat x, GLfloat y, GLfloat z)
{
   DISPATCH(Scalef, (x, y, z), ("glScalef(%g, %g, %g);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(Scissor)(GLint x, GLint y, GLsizei width, GLsizei height)
{
   DISPATCH(Scissor, (x, y, width, height), ("glScissor(%d, %d, %d, %d);\n", x, y, width, height));
}

KEYWORD1 void KEYWORD2 NAME(ShadeModel)(GLenum mode)
{
   DISPATCH(ShadeModel, (mode), ("glShadeModel(0x%x);\n", mode));
}

KEYWORD1 void KEYWORD2 NAME(StencilFunc)(GLenum func, GLint ref, GLuint mask)
{
   DISPATCH(StencilFunc, (func, ref, mask), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(StencilMask)(GLuint mask)
{
   DISPATCH(StencilMask, (mask), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(StencilOp)(GLenum fail, GLenum zfail, GLenum zpass)
{
   DISPATCH(StencilOp, (fail, zfail, zpass), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord1d)(GLdouble s)
{
   DISPATCH(TexCoord1d, (s), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord1f)(GLfloat s)
{
   DISPATCH(TexCoord1f, (s), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord1i)(GLint s)
{
   DISPATCH(TexCoord1i, (s), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord1s)(GLshort s)
{
   DISPATCH(TexCoord1s, (s), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord2d)(GLdouble s, GLdouble t)
{
   DISPATCH(TexCoord2d, (s, t), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord2f)(GLfloat s, GLfloat t)
{
   DISPATCH(TexCoord2f, (s, t), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord2s)(GLshort s, GLshort t)
{
   DISPATCH(TexCoord2s, (s, t), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord2i)(GLint s, GLint t)
{
   DISPATCH(TexCoord2i, (s, t), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord3d)(GLdouble s, GLdouble t, GLdouble r)
{
   DISPATCH(TexCoord3d, (s, t, r), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord3f)(GLfloat s, GLfloat t, GLfloat r)
{
   DISPATCH(TexCoord3f, (s, t, r), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord3i)(GLint s, GLint t, GLint r)
{
   DISPATCH(TexCoord3i, (s, t, r), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord3s)(GLshort s, GLshort t, GLshort r)
{
   DISPATCH(TexCoord3s, (s, t, r), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord4d)(GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
   DISPATCH(TexCoord4d, (s, t, r, q), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord4f)(GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
   DISPATCH(TexCoord4f, (s, t, r, q), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord4i)(GLint s, GLint t, GLint r, GLint q)
{
   DISPATCH(TexCoord4i, (s, t, r, q), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord4s)(GLshort s, GLshort t, GLshort r, GLshort q)
{
   DISPATCH(TexCoord4s, (s, t, r, q), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord1dv)(const GLdouble *v)
{
   DISPATCH(TexCoord1dv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord1fv)(const GLfloat *v)
{
   DISPATCH(TexCoord1fv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord1iv)(const GLint *v)
{
   DISPATCH(TexCoord1iv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord1sv)(const GLshort *v)
{
   DISPATCH(TexCoord1sv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord2dv)(const GLdouble *v)
{
   DISPATCH(TexCoord2dv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord2fv)(const GLfloat *v)
{
   DISPATCH(TexCoord2fv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord2iv)(const GLint *v)
{
   DISPATCH(TexCoord2iv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord2sv)(const GLshort *v)
{
   DISPATCH(TexCoord2sv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord3dv)(const GLdouble *v)
{
   DISPATCH(TexCoord3dv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord3fv)(const GLfloat *v)
{
   DISPATCH(TexCoord3fv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord3iv)(const GLint *v)
{
   DISPATCH(TexCoord3iv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord3sv)(const GLshort *v)
{
   DISPATCH(TexCoord3sv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord4dv)(const GLdouble *v)
{
   DISPATCH(TexCoord4dv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord4fv)(const GLfloat *v)
{
   DISPATCH(TexCoord4fv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord4iv)(const GLint *v)
{
   DISPATCH(TexCoord4iv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord4sv)(const GLshort *v)
{
   DISPATCH(TexCoord4sv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexGend)(GLenum coord, GLenum pname, GLdouble param)
{
   DISPATCH(TexGend, (coord, pname, param), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexGendv)(GLenum coord, GLenum pname, const GLdouble *params)
{
   DISPATCH(TexGendv, (coord, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexGenf)(GLenum coord, GLenum pname, GLfloat param)
{
   DISPATCH(TexGenf, (coord, pname, param), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexGenfv)(GLenum coord, GLenum pname, const GLfloat *params)
{
   DISPATCH(TexGenfv, (coord, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexGeni)(GLenum coord, GLenum pname, GLint param)
{
   DISPATCH(TexGeni, (coord, pname, param), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexGeniv)(GLenum coord, GLenum pname, const GLint *params)
{
   DISPATCH(TexGeniv, (coord, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexEnvf)(GLenum target, GLenum pname, GLfloat param)
{
   DISPATCH(TexEnvf, (target, pname, param), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexEnvfv)(GLenum target, GLenum pname, const GLfloat *param)
{
   DISPATCH(TexEnvfv, (target, pname, param), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexEnvi)(GLenum target, GLenum pname, GLint param)
{
   DISPATCH(TexEnvi, (target, pname, param), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexEnviv)(GLenum target, GLenum pname, const GLint *param)
{
   DISPATCH(TexEnviv, (target, pname, param), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexImage1D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
   DISPATCH(TexImage1D, (target, level, internalformat, width, border, format, type, pixels), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
   DISPATCH(TexImage2D, (target, level, internalformat, width, height, border, format, type, pixels), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexParameterf)(GLenum target, GLenum pname, GLfloat param)
{
   DISPATCH(TexParameterf, (target, pname, param), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexParameterfv)(GLenum target, GLenum pname, const GLfloat *params)
{
   DISPATCH(TexParameterfv, (target, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexParameteri)(GLenum target, GLenum pname, GLint param)
{
   DISPATCH(TexParameteri, (target, pname, param), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexParameteriv)(GLenum target, GLenum pname, const GLint *params)
{
   DISPATCH(TexParameteriv, (target, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Translated)(GLdouble x, GLdouble y, GLdouble z)
{
   DISPATCH(Translated, (x, y, z), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Translatef)(GLfloat x, GLfloat y, GLfloat z)
{
   DISPATCH(Translatef, (x, y, z), ("glTranslatef(%g, %g, %g);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(Vertex2d)(GLdouble x, GLdouble y)
{
   DISPATCH(Vertex2d, (x, y), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Vertex2dv)(const GLdouble *v)
{
   DISPATCH(Vertex2dv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Vertex2f)(GLfloat x, GLfloat y)
{
   DISPATCH(Vertex2f, (x, y), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Vertex2fv)(const GLfloat *v)
{
   DISPATCH(Vertex2fv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Vertex2i)(GLint x, GLint y)
{
   DISPATCH(Vertex2i, (x, y), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Vertex2iv)(const GLint *v)
{
   DISPATCH(Vertex2iv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Vertex2s)(GLshort x, GLshort y)
{
   DISPATCH(Vertex2s, (x, y), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Vertex2sv)(const GLshort *v)
{
   DISPATCH(Vertex2sv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Vertex3d)(GLdouble x, GLdouble y, GLdouble z)
{
   DISPATCH(Vertex3d, (x, y, z), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Vertex3dv)(const GLdouble *v)
{
   DISPATCH(Vertex3dv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Vertex3f)(GLfloat x, GLfloat y, GLfloat z)
{
   DISPATCH(Vertex3f, (x, y, z), ("glVertex3f(%g, %g, %g);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(Vertex3fv)(const GLfloat *v)
{
   DISPATCH(Vertex3fv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Vertex3i)(GLint x, GLint y, GLint z)
{
   DISPATCH(Vertex3i, (x, y, z), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Vertex3iv)(const GLint *v)
{
   DISPATCH(Vertex3iv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Vertex3s)(GLshort x, GLshort y, GLshort z)
{
   DISPATCH(Vertex3s, (x, y, z), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Vertex3sv)(const GLshort *v)
{
   DISPATCH(Vertex3sv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Vertex4d)(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   DISPATCH(Vertex4d, (x, y, z, w), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Vertex4dv)(const GLdouble *v)
{
   DISPATCH(Vertex4dv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Vertex4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   DISPATCH(Vertex4f, (x, y, z, w), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Vertex4fv)(const GLfloat *v)
{
   DISPATCH(Vertex4fv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Vertex4i)(GLint x, GLint y, GLint z, GLint w)
{
   DISPATCH(Vertex4i, (x, y, z, w), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Vertex4iv)(const GLint *v)
{
   DISPATCH(Vertex4iv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Vertex4s)(GLshort x, GLshort y, GLshort z, GLshort w)
{
   DISPATCH(Vertex4s, (x, y, z, w), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Vertex4sv)(const GLshort *v)
{
   DISPATCH(Vertex4sv, (v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Viewport)(GLint x, GLint y, GLsizei width, GLsizei height)
{
   DISPATCH(Viewport, (x, y, width, height), (";\n"));
}




/* GL 1.1 */

KEYWORD1 GLboolean KEYWORD2 NAME(AreTexturesResident)(GLsizei n, const GLuint *textures, GLboolean *residences)
{
   RETURN_DISPATCH(AreTexturesResident, (n, textures, residences), ("glAreTexturesResident(%d, %p, %p);\n", n, textures, residences));
}

KEYWORD1 void KEYWORD2 NAME(ArrayElement)(GLint i)
{
   DISPATCH(ArrayElement, (i), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(BindTexture)(GLenum target, GLuint texture)
{
   DISPATCH(BindTexture, (target, texture), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(ColorPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr)
{
   DISPATCH(ColorPointer, (size, type, stride, ptr), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(CopyTexImage1D)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border)
{
   DISPATCH(CopyTexImage1D, (target, level, internalformat, x, y, width, border), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(CopyTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
   DISPATCH(CopyTexImage2D, (target, level, internalformat, x, y, width, height, border), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(CopyTexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)
{
   DISPATCH(CopyTexSubImage1D, (target, level, xoffset, x, y, width), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(CopyTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
   DISPATCH(CopyTexSubImage2D, (target, level, xoffset, yoffset, x, y, width, height), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(DeleteTextures)(GLsizei n, const GLuint *textures)
{
   DISPATCH(DeleteTextures, (n, textures), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(DisableClientState)(GLenum cap)
{
   DISPATCH(DisableClientState, (cap), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(DrawArrays)(GLenum mode, GLint first, GLsizei count)
{
   DISPATCH(DrawArrays, (mode, first, count), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(EdgeFlagPointer)(GLsizei stride, const GLvoid *ptr)
{
   DISPATCH(EdgeFlagPointer, (stride, ptr), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(EnableClientState)(GLenum cap)
{
   DISPATCH(EnableClientState, (cap), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GenTextures)(GLsizei n, GLuint *textures)
{
   DISPATCH(GenTextures, (n, textures), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetPointerv)(GLenum pname, GLvoid **params)
{
   DISPATCH(GetPointerv, (pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(IndexPointer)(GLenum type, GLsizei stride, const GLvoid *ptr)
{
   DISPATCH(IndexPointer, (type, stride, ptr), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Indexub)(GLubyte c)
{
   DISPATCH(Indexub, (c), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Indexubv)(const GLubyte *c)
{
   DISPATCH(Indexubv, (c), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(InterleavedArrays)(GLenum format, GLsizei stride, const GLvoid *pointer)
{
   DISPATCH(InterleavedArrays, (format, stride, pointer), (";\n"));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsTexture)(GLuint texture)
{
   RETURN_DISPATCH(IsTexture, (texture), ("glIsTexture(%u);\n", texture));
}

KEYWORD1 void KEYWORD2 NAME(NormalPointer)(GLenum type, GLsizei stride, const GLvoid *ptr)
{
   DISPATCH(NormalPointer, (type, stride, ptr), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(PolygonOffset)(GLfloat factor, GLfloat units)
{
   DISPATCH(PolygonOffset, (factor, units), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(PopClientAttrib)(void)
{
   DISPATCH(PopClientAttrib, (), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(PrioritizeTextures)(GLsizei n, const GLuint *textures, const GLclampf *priorities)
{
   DISPATCH(PrioritizeTextures, (n, textures, priorities), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(PushClientAttrib)(GLbitfield mask)
{
   DISPATCH(PushClientAttrib, (mask), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoordPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr)
{
   DISPATCH(TexCoordPointer, (size, type, stride, ptr), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels)
{
   DISPATCH(TexSubImage1D, (target, level, xoffset, width, format, type, pixels), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
   DISPATCH(TexSubImage2D, (target, level, xoffset, yoffset, width, height, format, type, pixels), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(VertexPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr)
{
   DISPATCH(VertexPointer, (size, type, stride, ptr), (";\n"));
}




/* GL 1.2 */

KEYWORD1 void KEYWORD2 NAME(CopyTexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
   DISPATCH(CopyTexSubImage3D, (target, level, xoffset, yoffset, zoffset, x, y, width, height), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(DrawRangeElements)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices)
{
   DISPATCH(DrawRangeElements, (mode, start, end, count, type, indices), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexImage3D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
   DISPATCH(TexImage3D, (target, level, internalformat, width, height, depth, border, format, type, pixels), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels)
{
   DISPATCH(TexSubImage3D, (target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels), (";\n"));
}



/* GL_ARB_imaging */

KEYWORD1 void KEYWORD2 NAME(BlendColor)(GLclampf r, GLclampf g, GLclampf b, GLclampf a)
{
   DISPATCH(BlendColor, (r, g, b, a), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(BlendEquation)(GLenum mode)
{
   DISPATCH(BlendEquation, (mode), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(ColorSubTable)(GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid *data)
{
   DISPATCH(ColorSubTable, (target, start, count, format, type, data), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(ColorTable)(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *table)
{
   DISPATCH(ColorTable, (target, internalformat, width, format, type, table), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(ColorTableParameterfv)(GLenum target, GLenum pname, const GLfloat *params)
{
   DISPATCH(ColorTableParameterfv, (target, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(ColorTableParameteriv)(GLenum target, GLenum pname, const GLint *params)
{
   DISPATCH(ColorTableParameteriv, (target, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(ConvolutionFilter1D)(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *image)
{
   DISPATCH(ConvolutionFilter1D, (target, internalformat, width, format, type, image), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(ConvolutionFilter2D)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *image)
{
   DISPATCH(ConvolutionFilter2D, (target, internalformat, width, height, format, type, image), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(ConvolutionParameterf)(GLenum target, GLenum pname, GLfloat params)
{
   DISPATCH(ConvolutionParameterf, (target, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(ConvolutionParameterfv)(GLenum target, GLenum pname, const GLfloat *params)
{
   DISPATCH(ConvolutionParameterfv, (target, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(ConvolutionParameteri)(GLenum target, GLenum pname, GLint params)
{
   DISPATCH(ConvolutionParameteri, (target, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(ConvolutionParameteriv)(GLenum target, GLenum pname, const GLint *params)
{
   DISPATCH(ConvolutionParameteriv, (target, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(CopyColorSubTable)(GLenum target, GLsizei start, GLint x, GLint y, GLsizei width)
{
   DISPATCH(CopyColorSubTable, (target, start, x, y, width), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(CopyColorTable)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width)
{
   DISPATCH(CopyColorTable, (target, internalformat, x, y, width), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(CopyConvolutionFilter1D)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width)
{
   DISPATCH(CopyConvolutionFilter1D, (target, internalformat, x, y, width), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(CopyConvolutionFilter2D)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height)
{
   DISPATCH(CopyConvolutionFilter2D, (target, internalformat, x, y, width, height), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetColorTable)(GLenum target, GLenum format, GLenum type, GLvoid *table)
{
   DISPATCH(GetColorTable, (target, format, type, table), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetColorTableParameterfv)(GLenum target, GLenum pname, GLfloat *params)
{
   DISPATCH(GetColorTableParameterfv, (target, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetColorTableParameteriv)(GLenum target, GLenum pname, GLint *params)
{
   DISPATCH(GetColorTableParameteriv, (target, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetConvolutionFilter)(GLenum target, GLenum format, GLenum type, GLvoid *image)
{
   DISPATCH(GetConvolutionFilter, (target, format, type, image), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetConvolutionParameterfv)(GLenum target, GLenum pname, GLfloat *params)
{
   DISPATCH(GetConvolutionParameterfv, (target, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetConvolutionParameteriv)(GLenum target, GLenum pname, GLint *params)
{
   DISPATCH(GetConvolutionParameteriv, (target, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetHistogram)(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values)
{
   DISPATCH(GetHistogram, (target, reset, format, type, values), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetHistogramParameterfv)(GLenum target, GLenum pname, GLfloat *params)
{
   DISPATCH(GetHistogramParameterfv, (target, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetHistogramParameteriv)(GLenum target, GLenum pname, GLint *params)
{
   DISPATCH(GetHistogramParameteriv, (target, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetMinmax)(GLenum target, GLboolean reset, GLenum format, GLenum types, GLvoid *values)
{
   DISPATCH(GetMinmax, (target, reset, format, types, values), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetMinmaxParameterfv)(GLenum target, GLenum pname, GLfloat *params)
{
   DISPATCH(GetMinmaxParameterfv, (target, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetMinmaxParameteriv)(GLenum target, GLenum pname, GLint *params)
{
   DISPATCH(GetMinmaxParameteriv, (target, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetSeparableFilter)(GLenum target, GLenum format, GLenum type, GLvoid *row, GLvoid *column, GLvoid *span)
{
   DISPATCH(GetSeparableFilter, (target, format, type, row, column, span), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Histogram)(GLenum target, GLsizei width, GLenum internalformat, GLboolean sink)
{
   DISPATCH(Histogram, (target, width, internalformat, sink), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(Minmax)(GLenum target, GLenum internalformat, GLboolean sink)
{
   DISPATCH(Minmax, (target, internalformat, sink), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(ResetMinmax)(GLenum target)
{
   DISPATCH(ResetMinmax, (target), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(ResetHistogram)(GLenum target)
{
   DISPATCH(ResetHistogram, (target), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(SeparableFilter2D)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *row, const GLvoid *column)
{
   DISPATCH(SeparableFilter2D, (target, internalformat, width, height, format, type, row, column), (";\n"));
}



/* GL_ARB_multitexture */

KEYWORD1 void KEYWORD2 NAME(ActiveTextureARB)(GLenum texture)
{
   DISPATCH(ActiveTextureARB, (texture), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(ClientActiveTextureARB)(GLenum texture)
{
   DISPATCH(ClientActiveTextureARB, (texture), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1dARB)(GLenum target, GLdouble s)
{
   DISPATCH(MultiTexCoord1dARB, (target, s), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1dvARB)(GLenum target, const GLdouble *v)
{
   DISPATCH(MultiTexCoord1dvARB, (target, v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1fARB)(GLenum target, GLfloat s)
{
   DISPATCH(MultiTexCoord1fARB, (target, s), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1fvARB)(GLenum target, const GLfloat *v)
{
   DISPATCH(MultiTexCoord1fvARB, (target, v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1iARB)(GLenum target, GLint s)
{
   DISPATCH(MultiTexCoord1iARB, (target, s), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1ivARB)(GLenum target, const GLint *v)
{
   DISPATCH(MultiTexCoord1ivARB, (target, v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1sARB)(GLenum target, GLshort s)
{
   DISPATCH(MultiTexCoord1sARB, (target, s), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1svARB)(GLenum target, const GLshort *v)
{
   DISPATCH(MultiTexCoord1svARB, (target, v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2dARB)(GLenum target, GLdouble s, GLdouble t)
{
   DISPATCH(MultiTexCoord2dARB, (target, s, t), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2dvARB)(GLenum target, const GLdouble *v)
{
   DISPATCH(MultiTexCoord2dvARB, (target, v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2fARB)(GLenum target, GLfloat s, GLfloat t)
{
   DISPATCH(MultiTexCoord2fARB, (target, s, t), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2fvARB)(GLenum target, const GLfloat *v)
{
   DISPATCH(MultiTexCoord2fvARB, (target, v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2iARB)(GLenum target, GLint s, GLint t)
{
   DISPATCH(MultiTexCoord2iARB, (target, s, t), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2ivARB)(GLenum target, const GLint *v)
{
   DISPATCH(MultiTexCoord2ivARB, (target, v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2sARB)(GLenum target, GLshort s, GLshort t)
{
   DISPATCH(MultiTexCoord2sARB, (target, s, t), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2svARB)(GLenum target, const GLshort *v)
{
   DISPATCH(MultiTexCoord2svARB, (target, v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3dARB)(GLenum target, GLdouble s, GLdouble t, GLdouble r)
{
   DISPATCH(MultiTexCoord3dARB, (target, s, t, r), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3dvARB)(GLenum target, const GLdouble *v)
{
   DISPATCH(MultiTexCoord3dvARB, (target, v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3fARB)(GLenum target, GLfloat s, GLfloat t, GLfloat r)
{
   DISPATCH(MultiTexCoord3fARB, (target, s, t, r), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3fvARB)(GLenum target, const GLfloat *v)
{
   DISPATCH(MultiTexCoord3fvARB, (target, v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3iARB)(GLenum target, GLint s, GLint t, GLint r)
{
   DISPATCH(MultiTexCoord3iARB, (target, s, t, r), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3ivARB)(GLenum target, const GLint *v)
{
   DISPATCH(MultiTexCoord3ivARB, (target, v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3sARB)(GLenum target, GLshort s, GLshort t, GLshort r)
{
   DISPATCH(MultiTexCoord3sARB, (target, s, t, r), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3svARB)(GLenum target, const GLshort *v)
{
   DISPATCH(MultiTexCoord3svARB, (target, v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4dARB)(GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
   DISPATCH(MultiTexCoord4dARB, (target, s, t, r, q), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4dvARB)(GLenum target, const GLdouble *v)
{
   DISPATCH(MultiTexCoord4dvARB, (target, v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4fARB)(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
   DISPATCH(MultiTexCoord4fARB, (target, s, t, r, q), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4fvARB)(GLenum target, const GLfloat *v)
{
   DISPATCH(MultiTexCoord4fvARB, (target, v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4iARB)(GLenum target, GLint s, GLint t, GLint r, GLint q)
{
   DISPATCH(MultiTexCoord4iARB, (target, s, t, r, q), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4ivARB)(GLenum target, const GLint *v)
{
   DISPATCH(MultiTexCoord4ivARB, (target, v), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4sARB)(GLenum target, GLshort s, GLshort t, GLshort r, GLshort q)
{
   DISPATCH(MultiTexCoord4sARB, (target, s, t, r, q), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4svARB)(GLenum target, const GLshort *v)
{
   DISPATCH(MultiTexCoord4svARB, (target, v), (";\n"));
}



/***
 *** Extension functions
 ***/


/* 2. GL_EXT_blend_color */
KEYWORD1 void KEYWORD2 NAME(BlendColorEXT)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
   DISPATCH(BlendColorEXT, (red, green, blue, alpha), (";\n"));
}



/* 3. GL_EXT_polygon_offset */
KEYWORD1 void KEYWORD2 NAME(PolygonOffsetEXT)(GLfloat factor, GLfloat bias)
{
   DISPATCH(PolygonOffsetEXT, (factor, bias), (";\n"));
}



/* 6. GL_EXT_texture3D */

KEYWORD1 void KEYWORD2 NAME(TexImage3DEXT)(GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
   DISPATCH(TexImage3D, (target, level, internalFormat, width, height, depth, border, format, type, pixels), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexSubImage3DEXT)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels)
{
   DISPATCH(TexSubImage3D, (target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(CopyTexSubImage3DEXT)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
   DISPATCH(CopyTexSubImage3D, (target, level, xoffset, yoffset, zoffset, x, y, width, height), (";\n"));
}



/* 7. GL_SGI_texture_filter4 */

KEYWORD1 void KEYWORD2 NAME(GetTexFilterFuncSGIS)(GLenum target, GLenum filter, GLsizei n, const GLfloat *weights)
{
   DISPATCH(GetTexFilterFuncSGIS, (target, filter, n, weights), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexFilterFuncSGIS)(GLenum target, GLenum filter, GLfloat *weights)
{
   DISPATCH(TexFilterFuncSGIS, (target, filter, weights), (";\n"));
}



/* 9. GL_EXT_subtexture */

KEYWORD1 void KEYWORD2 NAME(CopyTexSubImage1DEXT)(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)
{
   DISPATCH(CopyTexSubImage1DEXT, (target, level, xoffset, x, y, width), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexSubImage1DEXT)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels)
{
   DISPATCH(TexSubImage1DEXT, (target, level, xoffset, width, format, type, pixels), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexSubImage2DEXT)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
   DISPATCH(TexSubImage2DEXT, (target, level, xoffset, yoffset, width, height, format, type, pixels), (";\n"));
}


/* 10. GL_EXT_copy_texture */

KEYWORD1 void KEYWORD2 NAME(CopyTexImage1DEXT)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border)
{
   DISPATCH(CopyTexImage1DEXT, (target, level, internalformat, x, y, width, border), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(CopyTexImage2DEXT)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
   DISPATCH(CopyTexImage2DEXT, (target, level, internalformat, x, y, width, height, border), (";\n"));
}


KEYWORD1 void KEYWORD2 NAME(CopyTexSubImage2DEXT)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
   DISPATCH(CopyTexSubImage2DEXT, (target, level, xoffset, yoffset, x, y, width, height), (";\n"));
}



/* 11. GL_EXT_histogram */
KEYWORD1 void KEYWORD2 NAME(GetHistogramEXT)(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values)
{
   DISPATCH(GetHistogramEXT, (target, reset, format, type, values), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetHistogramParameterfvEXT)(GLenum target, GLenum pname, GLfloat *params)
{
   DISPATCH(GetHistogramParameterfvEXT, (target, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetHistogramParameterivEXT)(GLenum target, GLenum pname, GLint *params)
{
   DISPATCH(GetHistogramParameterivEXT, (target, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetMinmaxEXT)(GLenum target, GLboolean reset, GLenum format, GLenum types, GLvoid *values)
{
   DISPATCH(GetMinmaxEXT, (target, reset, format, types, values), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetMinmaxParameterfvEXT)(GLenum target, GLenum pname, GLfloat *params)
{
   DISPATCH(GetMinmaxParameterfvEXT, (target, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetMinmaxParameterivEXT)(GLenum target, GLenum pname, GLint *params)
{
   DISPATCH(GetMinmaxParameterivEXT, (target, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(HistogramEXT)(GLenum target, GLsizei width, GLenum internalformat, GLboolean sink)
{
   DISPATCH(HistogramEXT, (target, width, internalformat, sink), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(MinmaxEXT)(GLenum target, GLenum internalformat, GLboolean sink)
{
   DISPATCH(MinmaxEXT, (target, internalformat, sink), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(ResetHistogramEXT)(GLenum target)
{
   DISPATCH(ResetHistogramEXT, (target), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(ResetMinmaxEXT)(GLenum target)
{
   DISPATCH(ResetMinmaxEXT, (target), (";\n"));
}



/* 12. GL_EXT_convolution */

KEYWORD1 void KEYWORD2 NAME(ConvolutionFilter1DEXT)(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *image)
{
   DISPATCH(ConvolutionFilter1DEXT, (target, internalformat, width, format, type, image), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(ConvolutionFilter2DEXT)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *image)
{
   DISPATCH(ConvolutionFilter2DEXT, (target, internalformat, width, height, format, type, image), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(ConvolutionParameterfEXT)(GLenum target, GLenum pname, GLfloat params)
{
   DISPATCH(ConvolutionParameterfEXT, (target, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(ConvolutionParameterfvEXT)(GLenum target, GLenum pname, const GLfloat *params)
{
   DISPATCH(ConvolutionParameterfvEXT, (target, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(ConvolutionParameteriEXT)(GLenum target, GLenum pname, GLint params)
{
   DISPATCH(ConvolutionParameteriEXT, (target, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(ConvolutionParameterivEXT)(GLenum target, GLenum pname, const GLint *params)
{
   DISPATCH(ConvolutionParameterivEXT, (target, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(CopyConvolutionFilter1DEXT)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width)
{
   DISPATCH(CopyConvolutionFilter1DEXT, (target, internalformat, x, y, width), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(CopyConvolutionFilter2DEXT)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height)
{
   DISPATCH(CopyConvolutionFilter2DEXT, (target, internalformat, x, y, width, height), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetConvolutionFilterEXT)(GLenum target, GLenum format, GLenum type, GLvoid *image)
{
   DISPATCH(GetConvolutionFilterEXT, (target, format, type, image), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetConvolutionParameterfvEXT)(GLenum target, GLenum pname, GLfloat *params)
{
   DISPATCH(GetConvolutionParameterfvEXT, (target, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetConvolutionParameterivEXT)(GLenum target, GLenum pname, GLint *params)
{
   DISPATCH(GetConvolutionParameterivEXT, (target, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetSeparableFilterEXT)(GLenum target, GLenum format, GLenum type, GLvoid *row, GLvoid *column, GLvoid *span)
{
   DISPATCH(GetSeparableFilterEXT, (target, format, type, row, column, span), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(SeparableFilter2DEXT)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *row, const GLvoid *column)
{
   DISPATCH(SeparableFilter2DEXT, (target, internalformat, width, height, format, type, row, column), (";\n"));
}



/* 14. GL_SGI_color_table */

KEYWORD1 void KEYWORD2 NAME(ColorTableParameterfvSGI)(GLenum target, GLenum pname, const GLfloat *params)
{
   DISPATCH(ColorTableParameterfvSGI, (target, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(ColorTableParameterivSGI)(GLenum target, GLenum pname, const GLint *params)
{
   DISPATCH(ColorTableParameterivSGI, (target, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(ColorTableSGI)(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *table)
{
   DISPATCH(ColorTableSGI, (target, internalformat, width, format, type, table), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(CopyColorTableSGI)(GLenum target, GLenum internalFormat, GLint x, GLint y, GLsizei width)
{
   DISPATCH(CopyColorTableSGI, (target, internalFormat, x, y, width), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetColorTableSGI)(GLenum target, GLenum format, GLenum type, GLvoid *table)
{
   DISPATCH(GetColorTableSGI, (target, format, type, table), (";\n"));
}
KEYWORD1 void KEYWORD2 NAME(GetColorTableParameterfvSGI)(GLenum target, GLenum pname, GLfloat *params)
{
   DISPATCH(GetColorTableParameterfvSGI, (target, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetColorTableParameterivSGI)(GLenum target, GLenum pname, GLint *params)
{
   DISPATCH(GetColorTableParameterivSGI, (target, pname, params), (";\n"));
}



/* 20. GL_EXT_texture_object */

KEYWORD1 void KEYWORD2 NAME(GenTexturesEXT)(GLsizei n, GLuint *textures)
{
   DISPATCH(GenTextures, (n, textures), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(DeleteTexturesEXT)(GLsizei n, const GLuint *texture)
{
   DISPATCH(DeleteTextures, (n, texture), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(BindTextureEXT)(GLenum target, GLuint texture)
{
   DISPATCH(BindTexture, (target, texture), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(PrioritizeTexturesEXT)(GLsizei n, const GLuint *textures, const GLclampf *priorities)
{
   DISPATCH(PrioritizeTextures, (n, textures, priorities), (";\n"));
}

KEYWORD1 GLboolean KEYWORD2 NAME(AreTexturesResidentEXT)(GLsizei n, const GLuint *textures, GLboolean *residences)
{
   RETURN_DISPATCH(AreTexturesResident, (n, textures, residences), ("glAreTexturesResidentEXT(%d %p %p);\n", n, textures, residences));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsTextureEXT)(GLuint texture)
{
   RETURN_DISPATCH(IsTexture, (texture), ("glIsTextureEXT(%u);\n", texture));
}



/* 37. GL_EXT_blend_minmax */
KEYWORD1 void KEYWORD2 NAME(BlendEquationEXT)(GLenum mode)
{
   DISPATCH(BlendEquationEXT, (mode), (";\n"));
}



/* 30. GL_EXT_vertex_array */

KEYWORD1 void KEYWORD2 NAME(VertexPointerEXT)(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *ptr)
{
   DISPATCH(VertexPointer, (size, type, stride, ptr), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(NormalPointerEXT)(GLenum type, GLsizei stride, GLsizei count, const GLvoid *ptr)
{
   DISPATCH(NormalPointer, (type, stride, ptr), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(ColorPointerEXT)(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *ptr)
{
   DISPATCH(ColorPointer, (size, type, stride, ptr), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(IndexPointerEXT)(GLenum type, GLsizei stride, GLsizei count, const GLvoid *ptr)
{
   DISPATCH(IndexPointer, (type, stride, ptr), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(TexCoordPointerEXT)(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *ptr)
{
   DISPATCH(ColorPointer, (size, type, stride, ptr), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(EdgeFlagPointerEXT)(GLsizei stride, GLsizei count, const GLboolean *ptr)
{
   DISPATCH(EdgeFlagPointer, (stride, ptr), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetPointervEXT)(GLenum pname, void **params)
{
   DISPATCH(GetPointerv, (pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(ArrayElementEXT)(GLint i)
{
   DISPATCH(ArrayElement, (i), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(DrawArraysEXT)(GLenum mode, GLint first, GLsizei count)
{
   DISPATCH(DrawArrays, (mode, first, count), (";\n"));
}



/* 54. GL_EXT_point_parameters */

KEYWORD1 void KEYWORD2 NAME(PointParameterfEXT)(GLenum target, GLfloat param)
{
   DISPATCH(PointParameterfEXT, (target, param), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(PointParameterfvEXT)(GLenum target, const GLfloat *param)
{
   DISPATCH(PointParameterfvEXT, (target, param), (";\n"));
}


/* 77. GL_PGI_misc_hints */
KEYWORD1 void KEYWORD2 NAME(HintPGI)(GLenum target, GLint mode)
{
   DISPATCH(HintPGI, (target, mode), (";\n"));
}


/* 78. GL_EXT_paletted_texture */

KEYWORD1 void KEYWORD2 NAME(ColorTableEXT)(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *table)
{
   DISPATCH(ColorTableEXT, (target, internalformat, width, format, type, table), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(ColorSubTableEXT)(GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid *data)
{
   DISPATCH(ColorSubTableEXT, (target, start, count, format, type, data), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetColorTableEXT)(GLenum target, GLenum format, GLenum type, GLvoid *table)
{
   DISPATCH(GetColorTableEXT, (target, format, type, table), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetColorTableParameterfvEXT)(GLenum target, GLenum pname, GLfloat *params)
{
   DISPATCH(GetColorTableParameterfvEXT, (target, pname, params), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetColorTableParameterivEXT)(GLenum target, GLenum pname, GLint *params)
{
   DISPATCH(GetColorTableParameterivEXT, (target, pname, params), (";\n"));
}




/* 97. GL_EXT_compiled_vertex_array */

KEYWORD1 void KEYWORD2 NAME(LockArraysEXT)(GLint first, GLsizei count)
{
   DISPATCH(LockArraysEXT, (first, count), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(UnlockArraysEXT)(void)
{
   DISPATCH(UnlockArraysEXT, (), (";\n"));
}



/* 173. GL_EXT/INGR_blend_func_separate */
KEYWORD1 void KEYWORD2 NAME(BlendFuncSeparateINGR)(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha)
{
   DISPATCH(BlendFuncSeparateINGR, (sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha), (";\n"));
}



/* GL_MESA_window_pos */

KEYWORD1 void KEYWORD2 NAME(WindowPos2iMESA)(GLint x, GLint y)
{
   DISPATCH(WindowPos4fMESA, (x, y, 0, 1), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2sMESA)(GLshort x, GLshort y)
{
   DISPATCH(WindowPos4fMESA, (x, y, 0, 1), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2fMESA)(GLfloat x, GLfloat y)
{
   DISPATCH(WindowPos4fMESA, (x, y, 0, 1), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2dMESA)(GLdouble x, GLdouble y)
{
   DISPATCH(WindowPos4fMESA, (x, y, 0, 1), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2ivMESA)(const GLint *p)
{
   DISPATCH(WindowPos4fMESA, (p[0], p[1], 0, 1), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2svMESA)(const GLshort *p)
{
   DISPATCH(WindowPos4fMESA, (p[0], p[1], 0, 1), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2fvMESA)(const GLfloat *p)
{
   DISPATCH(WindowPos4fMESA, (p[0], p[1], 0, 1), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2dvMESA)(const GLdouble *p)
{
   DISPATCH(WindowPos4fMESA, (p[0], p[1], 0, 1), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3iMESA)(GLint x, GLint y, GLint z)
{
   DISPATCH(WindowPos4fMESA, (x, y, z, 1), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3sMESA)(GLshort x, GLshort y, GLshort z)
{
   DISPATCH(WindowPos4fMESA, (x, y, z, 1), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3fMESA)(GLfloat x, GLfloat y, GLfloat z)
{
   DISPATCH(WindowPos4fMESA, (x, y, z, 1), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3dMESA)(GLdouble x, GLdouble y, GLdouble z)
{
   DISPATCH(WindowPos4fMESA, (x, y, z, 1), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3ivMESA)(const GLint *p)
{
   DISPATCH(WindowPos4fMESA, (p[0], p[1], p[2], 1.0), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3svMESA)(const GLshort *p)
{
   DISPATCH(WindowPos4fMESA, (p[0], p[1], p[2], 1.0), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3fvMESA)(const GLfloat *p)
{
   DISPATCH(WindowPos4fMESA, (p[0], p[1], p[2], 1.0), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3dvMESA)(const GLdouble *p)
{
   DISPATCH(WindowPos4fMESA, (p[0], p[1], p[2], 1.0), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos4iMESA)(GLint x, GLint y, GLint z, GLint w)
{
   DISPATCH(WindowPos4fMESA, (x, y, z, w), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos4sMESA)(GLshort x, GLshort y, GLshort z, GLshort w)
{
   DISPATCH(WindowPos4fMESA, (x, y, z, w), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos4fMESA)(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   DISPATCH(WindowPos4fMESA, (x, y, z, w), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos4dMESA)(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   DISPATCH(WindowPos4fMESA, (x, y, z, w), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos4ivMESA)(const GLint *p)
{
   DISPATCH(WindowPos4fMESA, (p[0], p[1], p[2], p[3]), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos4svMESA)(const GLshort *p)
{
   DISPATCH(WindowPos4fMESA, (p[0], p[1], p[2], p[3]), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos4fvMESA)(const GLfloat *p)
{
   DISPATCH(WindowPos4fMESA, (p[0], p[1], p[2], p[3]), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos4dvMESA)(const GLdouble *p)
{
   DISPATCH(WindowPos4fMESA, (p[0], p[1], p[2], p[3]), (";\n"));
}



/* GL_MESA_resize_buffers */
KEYWORD1 void KEYWORD2 NAME(ResizeBuffersMESA)(void)
{
   DISPATCH(ResizeBuffersMESA, (), (";\n"));
}



/* GL_ARB_transpose_matrix */
KEYWORD1 void KEYWORD2 NAME(LoadTransposeMatrixdARB)(const GLdouble m[16])
{
   DISPATCH(LoadTransposeMatrixdARB, (m), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(LoadTransposeMatrixfARB)(const GLfloat m[16])
{
   DISPATCH(LoadTransposeMatrixfARB, (m), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(MultTransposeMatrixdARB)(const GLdouble m[16])
{
   DISPATCH(MultTransposeMatrixdARB, (m), (";\n"));
}

KEYWORD1 void KEYWORD2 NAME(MultTransposeMatrixfARB)(const GLfloat m[16])
{
   DISPATCH(MultTransposeMatrixfARB, (m), (";\n"));
}



#undef KEYWORD1
#undef KEYWORD2
#undef NAME
#undef DISPATCH
#undef RETURN_DISPATCH

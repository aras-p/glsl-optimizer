/*
 * Mesa 3-D graphics library
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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */



#ifndef API_LOOPBACK_H
#define API_LOOPBACK_H

#include "main/compiler.h"
#include "main/glheader.h" // ?
#include "main/macros.h" // ?
#include "main/mtypes.h" // ?
#include "glapi/glapi.h" // ?
#include "main/dispatch.h" // ?
#include "main/context.h" // ?

struct _glapi_table;
struct gl_context;

extern void
_mesa_loopback_init_api_table(const struct gl_context *ctx,
                              struct _glapi_table *dest);
void GLAPIENTRY
_mesa_Color3b( GLbyte red, GLbyte green, GLbyte blue );
void GLAPIENTRY
_mesa_Color3d( GLdouble red, GLdouble green, GLdouble blue );
void GLAPIENTRY
_mesa_Color3i( GLint red, GLint green, GLint blue );
void GLAPIENTRY
_mesa_Color3s( GLshort red, GLshort green, GLshort blue );
void GLAPIENTRY
_mesa_Color3ui( GLuint red, GLuint green, GLuint blue );
void GLAPIENTRY
_mesa_Color3us( GLushort red, GLushort green, GLushort blue );
void GLAPIENTRY
_mesa_Color3ub( GLubyte red, GLubyte green, GLubyte blue );
void GLAPIENTRY
_mesa_Color3bv( const GLbyte *v );
void GLAPIENTRY
_mesa_Color3dv( const GLdouble *v );
void GLAPIENTRY
_mesa_Color3iv( const GLint *v );
void GLAPIENTRY
_mesa_Color3sv( const GLshort *v );
void GLAPIENTRY
_mesa_Color3uiv( const GLuint *v );
void GLAPIENTRY
_mesa_Color3usv( const GLushort *v );
void GLAPIENTRY
_mesa_Color3ubv( const GLubyte *v );
void GLAPIENTRY
_mesa_Color4b( GLbyte red, GLbyte green, GLbyte blue,
                    GLbyte alpha );
void GLAPIENTRY
_mesa_Color4d( GLdouble red, GLdouble green, GLdouble blue,
                    GLdouble alpha );
void GLAPIENTRY
_mesa_Color4i( GLint red, GLint green, GLint blue, GLint alpha );
void GLAPIENTRY
_mesa_Color4s( GLshort red, GLshort green, GLshort blue,
                    GLshort alpha );
void GLAPIENTRY
_mesa_Color4ui( GLuint red, GLuint green, GLuint blue, GLuint alpha );
void GLAPIENTRY
_mesa_Color4us( GLushort red, GLushort green, GLushort blue,
                     GLushort alpha );
void GLAPIENTRY
_mesa_Color4ub( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha );
void GLAPIENTRY
_mesa_Color4iv( const GLint *v );
void GLAPIENTRY
_mesa_Color4bv( const GLbyte *v );
void GLAPIENTRY
_mesa_Color4dv( const GLdouble *v );
void GLAPIENTRY
_mesa_Color4sv( const GLshort *v);
void GLAPIENTRY
_mesa_Color4uiv( const GLuint *v);
void GLAPIENTRY
_mesa_Color4usv( const GLushort *v);
void GLAPIENTRY
_mesa_Color4ubv( const GLubyte *v);
void GLAPIENTRY
_mesa_FogCoordd( GLdouble d );
void GLAPIENTRY
_mesa_FogCoorddv( const GLdouble *v );
void GLAPIENTRY
_mesa_Indexd( GLdouble c );
void GLAPIENTRY
_mesa_Indexi( GLint c );
void GLAPIENTRY
_mesa_Indexs( GLshort c );
void GLAPIENTRY
_mesa_Indexub( GLubyte c );
void GLAPIENTRY
_mesa_Indexdv( const GLdouble *c );
void GLAPIENTRY
_mesa_Indexiv( const GLint *c );
void GLAPIENTRY
_mesa_Indexsv( const GLshort *c );
void GLAPIENTRY
_mesa_Indexubv( const GLubyte *c );
void GLAPIENTRY
_mesa_EdgeFlagv(const GLboolean *flag);
void GLAPIENTRY
_mesa_Normal3b( GLbyte nx, GLbyte ny, GLbyte nz );
void GLAPIENTRY
_mesa_Normal3d( GLdouble nx, GLdouble ny, GLdouble nz );
void GLAPIENTRY
_mesa_Normal3i( GLint nx, GLint ny, GLint nz );
void GLAPIENTRY
_mesa_Normal3s( GLshort nx, GLshort ny, GLshort nz );
void GLAPIENTRY
_mesa_Normal3bv( const GLbyte *v );
void GLAPIENTRY
_mesa_Normal3dv( const GLdouble *v );
void GLAPIENTRY
_mesa_Normal3iv( const GLint *v );
void GLAPIENTRY
_mesa_Normal3sv( const GLshort *v );
void GLAPIENTRY
_mesa_TexCoord1d( GLdouble s );
void GLAPIENTRY
_mesa_TexCoord1i( GLint s );
void GLAPIENTRY
_mesa_TexCoord1s( GLshort s );
void GLAPIENTRY
_mesa_TexCoord2d( GLdouble s, GLdouble t );
void GLAPIENTRY
_mesa_TexCoord2s( GLshort s, GLshort t );
void GLAPIENTRY
_mesa_TexCoord2i( GLint s, GLint t );
void GLAPIENTRY
_mesa_TexCoord3d( GLdouble s, GLdouble t, GLdouble r );
void GLAPIENTRY
_mesa_TexCoord3i( GLint s, GLint t, GLint r );
void GLAPIENTRY
_mesa_TexCoord3s( GLshort s, GLshort t, GLshort r );
void GLAPIENTRY
_mesa_TexCoord4d( GLdouble s, GLdouble t, GLdouble r, GLdouble q );
void GLAPIENTRY
_mesa_TexCoord4i( GLint s, GLint t, GLint r, GLint q );
void GLAPIENTRY
_mesa_TexCoord4s( GLshort s, GLshort t, GLshort r, GLshort q );
void GLAPIENTRY
_mesa_TexCoord1dv( const GLdouble *v );
void GLAPIENTRY
_mesa_TexCoord1iv( const GLint *v );
void GLAPIENTRY
_mesa_TexCoord1sv( const GLshort *v );
void GLAPIENTRY
_mesa_TexCoord2dv( const GLdouble *v );
void GLAPIENTRY
_mesa_TexCoord2iv( const GLint *v );
void GLAPIENTRY
_mesa_TexCoord2sv( const GLshort *v );
void GLAPIENTRY
_mesa_TexCoord3dv( const GLdouble *v );
void GLAPIENTRY
_mesa_TexCoord3iv( const GLint *v );
void GLAPIENTRY
_mesa_TexCoord3sv( const GLshort *v );
void GLAPIENTRY
_mesa_TexCoord4dv( const GLdouble *v );
void GLAPIENTRY
_mesa_TexCoord4iv( const GLint *v );
void GLAPIENTRY
_mesa_TexCoord4sv( const GLshort *v );
void GLAPIENTRY
_mesa_Vertex2d( GLdouble x, GLdouble y );
void GLAPIENTRY
_mesa_Vertex2i( GLint x, GLint y );
void GLAPIENTRY
_mesa_Vertex2s( GLshort x, GLshort y );
void GLAPIENTRY
_mesa_Vertex3d( GLdouble x, GLdouble y, GLdouble z );
void GLAPIENTRY
_mesa_Vertex3i( GLint x, GLint y, GLint z );
void GLAPIENTRY
_mesa_Vertex3s( GLshort x, GLshort y, GLshort z );
void GLAPIENTRY
_mesa_Vertex4d( GLdouble x, GLdouble y, GLdouble z, GLdouble w );
void GLAPIENTRY
_mesa_Vertex4i( GLint x, GLint y, GLint z, GLint w );
void GLAPIENTRY
_mesa_Vertex4s( GLshort x, GLshort y, GLshort z, GLshort w );
void GLAPIENTRY
_mesa_Vertex2dv( const GLdouble *v );
void GLAPIENTRY
_mesa_Vertex2iv( const GLint *v );
void GLAPIENTRY
_mesa_Vertex2sv( const GLshort *v );
void GLAPIENTRY
_mesa_Vertex3dv( const GLdouble *v );
void GLAPIENTRY
_mesa_Vertex3iv( const GLint *v );
void GLAPIENTRY
_mesa_Vertex3sv( const GLshort *v );
void GLAPIENTRY
_mesa_Vertex4dv( const GLdouble *v );
void GLAPIENTRY
_mesa_Vertex4iv( const GLint *v );
void GLAPIENTRY
_mesa_Vertex4sv( const GLshort *v );
void GLAPIENTRY
_mesa_MultiTexCoord1d(GLenum target, GLdouble s);
void GLAPIENTRY
_mesa_MultiTexCoord1dv(GLenum target, const GLdouble *v);
void GLAPIENTRY
_mesa_MultiTexCoord1i(GLenum target, GLint s);
void GLAPIENTRY
_mesa_MultiTexCoord1iv(GLenum target, const GLint *v);
void GLAPIENTRY
_mesa_MultiTexCoord1s(GLenum target, GLshort s);
void GLAPIENTRY
_mesa_MultiTexCoord1sv(GLenum target, const GLshort *v);
void GLAPIENTRY
_mesa_MultiTexCoord2d(GLenum target, GLdouble s, GLdouble t);
void GLAPIENTRY
_mesa_MultiTexCoord2dv(GLenum target, const GLdouble *v);
void GLAPIENTRY
_mesa_MultiTexCoord2i(GLenum target, GLint s, GLint t);
void GLAPIENTRY
_mesa_MultiTexCoord2iv(GLenum target, const GLint *v);
void GLAPIENTRY
_mesa_MultiTexCoord2s(GLenum target, GLshort s, GLshort t);
void GLAPIENTRY
_mesa_MultiTexCoord2sv(GLenum target, const GLshort *v);
void GLAPIENTRY
_mesa_MultiTexCoord3d(GLenum target, GLdouble s, GLdouble t, GLdouble r);
void GLAPIENTRY
_mesa_MultiTexCoord3dv(GLenum target, const GLdouble *v);
void GLAPIENTRY
_mesa_MultiTexCoord3i(GLenum target, GLint s, GLint t, GLint r);
void GLAPIENTRY
_mesa_MultiTexCoord3iv(GLenum target, const GLint *v);
void GLAPIENTRY
_mesa_MultiTexCoord3s(GLenum target, GLshort s, GLshort t, GLshort r);
void GLAPIENTRY
_mesa_MultiTexCoord3sv(GLenum target, const GLshort *v);
void GLAPIENTRY
_mesa_MultiTexCoord4d(GLenum target, GLdouble s, GLdouble t, GLdouble r,
                            GLdouble q);
void GLAPIENTRY
_mesa_MultiTexCoord4dv(GLenum target, const GLdouble *v);
void GLAPIENTRY
_mesa_MultiTexCoord4i(GLenum target, GLint s, GLint t, GLint r, GLint q);
void GLAPIENTRY
_mesa_MultiTexCoord4iv(GLenum target, const GLint *v);
void GLAPIENTRY
_mesa_MultiTexCoord4s(GLenum target, GLshort s, GLshort t, GLshort r,
                            GLshort q);
void GLAPIENTRY
_mesa_MultiTexCoord4sv(GLenum target, const GLshort *v);
void GLAPIENTRY
_mesa_EvalCoord2dv( const GLdouble *u );
void GLAPIENTRY
_mesa_EvalCoord2fv( const GLfloat *u );
void GLAPIENTRY
_mesa_EvalCoord2d( GLdouble u, GLdouble v );
void GLAPIENTRY
_mesa_EvalCoord1dv( const GLdouble *u );
void GLAPIENTRY
_mesa_EvalCoord1fv( const GLfloat *u );
void GLAPIENTRY
_mesa_EvalCoord1d( GLdouble u );
void GLAPIENTRY
_mesa_Materialf( GLenum face, GLenum pname, GLfloat param );
void GLAPIENTRY
_mesa_Materiali(GLenum face, GLenum pname, GLint param );
void GLAPIENTRY
_mesa_Materialiv(GLenum face, GLenum pname, const GLint *params );
void GLAPIENTRY
_mesa_Rectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
void GLAPIENTRY
_mesa_Rectdv(const GLdouble *v1, const GLdouble *v2);
void GLAPIENTRY
_mesa_Rectfv(const GLfloat *v1, const GLfloat *v2);
void GLAPIENTRY
_mesa_Recti(GLint x1, GLint y1, GLint x2, GLint y2);
void GLAPIENTRY
_mesa_Rectiv(const GLint *v1, const GLint *v2);
void GLAPIENTRY
_mesa_Rects(GLshort x1, GLshort y1, GLshort x2, GLshort y2);
void GLAPIENTRY
_mesa_Rectsv(const GLshort *v1, const GLshort *v2);
void GLAPIENTRY
_mesa_SecondaryColor3b( GLbyte red, GLbyte green, GLbyte blue );
void GLAPIENTRY
_mesa_SecondaryColor3d( GLdouble red, GLdouble green, GLdouble blue );
void GLAPIENTRY
_mesa_SecondaryColor3i( GLint red, GLint green, GLint blue );
void GLAPIENTRY
_mesa_SecondaryColor3s( GLshort red, GLshort green, GLshort blue );
void GLAPIENTRY
_mesa_SecondaryColor3ui( GLuint red, GLuint green, GLuint blue );
void GLAPIENTRY
_mesa_SecondaryColor3us( GLushort red, GLushort green, GLushort blue );
void GLAPIENTRY
_mesa_SecondaryColor3ub( GLubyte red, GLubyte green, GLubyte blue );
void GLAPIENTRY
_mesa_SecondaryColor3bv( const GLbyte *v );
void GLAPIENTRY
_mesa_SecondaryColor3dv( const GLdouble *v );
void GLAPIENTRY
_mesa_SecondaryColor3iv( const GLint *v );
void GLAPIENTRY
_mesa_SecondaryColor3sv( const GLshort *v );
void GLAPIENTRY
_mesa_SecondaryColor3uiv( const GLuint *v );
void GLAPIENTRY
_mesa_SecondaryColor3usv( const GLushort *v );
void GLAPIENTRY
_mesa_SecondaryColor3ubv( const GLubyte *v );
void GLAPIENTRY
_mesa_VertexAttrib1sNV(GLuint index, GLshort x);
void GLAPIENTRY
_mesa_VertexAttrib1dNV(GLuint index, GLdouble x);
void GLAPIENTRY
_mesa_VertexAttrib2sNV(GLuint index, GLshort x, GLshort y);
void GLAPIENTRY
_mesa_VertexAttrib2dNV(GLuint index, GLdouble x, GLdouble y);
void GLAPIENTRY
_mesa_VertexAttrib3sNV(GLuint index, GLshort x, GLshort y, GLshort z);
void GLAPIENTRY
_mesa_VertexAttrib3dNV(GLuint index, GLdouble x, GLdouble y, GLdouble z);
void GLAPIENTRY
_mesa_VertexAttrib4sNV(GLuint index, GLshort x, GLshort y, GLshort z,
                          GLshort w);
void GLAPIENTRY
_mesa_VertexAttrib4dNV(GLuint index, GLdouble x, GLdouble y, GLdouble z,
                          GLdouble w);
void GLAPIENTRY
_mesa_VertexAttrib4ubNV(GLuint index, GLubyte x, GLubyte y, GLubyte z,
                           GLubyte w);
void GLAPIENTRY
_mesa_VertexAttrib1svNV(GLuint index, const GLshort *v);
void GLAPIENTRY
_mesa_VertexAttrib1dvNV(GLuint index, const GLdouble *v);
void GLAPIENTRY
_mesa_VertexAttrib2svNV(GLuint index, const GLshort *v);
void GLAPIENTRY
_mesa_VertexAttrib2dvNV(GLuint index, const GLdouble *v);
void GLAPIENTRY
_mesa_VertexAttrib3svNV(GLuint index, const GLshort *v);
void GLAPIENTRY
_mesa_VertexAttrib3dvNV(GLuint index, const GLdouble *v);
void GLAPIENTRY
_mesa_VertexAttrib4svNV(GLuint index, const GLshort *v);
void GLAPIENTRY
_mesa_VertexAttrib4dvNV(GLuint index, const GLdouble *v);
void GLAPIENTRY
_mesa_VertexAttrib4ubvNV(GLuint index, const GLubyte *v);
void GLAPIENTRY
_mesa_VertexAttribs1svNV(GLuint index, GLsizei n, const GLshort *v);
void GLAPIENTRY
_mesa_VertexAttribs1fvNV(GLuint index, GLsizei n, const GLfloat *v);
void GLAPIENTRY
_mesa_VertexAttribs1dvNV(GLuint index, GLsizei n, const GLdouble *v);
void GLAPIENTRY
_mesa_VertexAttribs2svNV(GLuint index, GLsizei n, const GLshort *v);
void GLAPIENTRY
_mesa_VertexAttribs2fvNV(GLuint index, GLsizei n, const GLfloat *v);
void GLAPIENTRY
_mesa_VertexAttribs2dvNV(GLuint index, GLsizei n, const GLdouble *v);
void GLAPIENTRY
_mesa_VertexAttribs3svNV(GLuint index, GLsizei n, const GLshort *v);
void GLAPIENTRY
_mesa_VertexAttribs3fvNV(GLuint index, GLsizei n, const GLfloat *v);
void GLAPIENTRY
_mesa_VertexAttribs3dvNV(GLuint index, GLsizei n, const GLdouble *v);
void GLAPIENTRY
_mesa_VertexAttribs4svNV(GLuint index, GLsizei n, const GLshort *v);
void GLAPIENTRY
_mesa_VertexAttribs4fvNV(GLuint index, GLsizei n, const GLfloat *v);
void GLAPIENTRY
_mesa_VertexAttribs4dvNV(GLuint index, GLsizei n, const GLdouble *v);
void GLAPIENTRY
_mesa_VertexAttribs4ubvNV(GLuint index, GLsizei n, const GLubyte *v);
void GLAPIENTRY
_mesa_VertexAttrib1s(GLuint index, GLshort x);
void GLAPIENTRY
_mesa_VertexAttrib1d(GLuint index, GLdouble x);
void GLAPIENTRY
_mesa_VertexAttrib2s(GLuint index, GLshort x, GLshort y);
void GLAPIENTRY
_mesa_VertexAttrib2d(GLuint index, GLdouble x, GLdouble y);
void GLAPIENTRY
_mesa_VertexAttrib3s(GLuint index, GLshort x, GLshort y, GLshort z);
void GLAPIENTRY
_mesa_VertexAttrib3d(GLuint index, GLdouble x, GLdouble y, GLdouble z);
void GLAPIENTRY
_mesa_VertexAttrib4s(GLuint index, GLshort x, GLshort y, GLshort z,
                           GLshort w);
void GLAPIENTRY
_mesa_VertexAttrib4d(GLuint index, GLdouble x, GLdouble y, GLdouble z,
                           GLdouble w);
void GLAPIENTRY
_mesa_VertexAttrib1sv(GLuint index, const GLshort *v);
void GLAPIENTRY
_mesa_VertexAttrib1dv(GLuint index, const GLdouble *v);
void GLAPIENTRY
_mesa_VertexAttrib2sv(GLuint index, const GLshort *v);
void GLAPIENTRY
_mesa_VertexAttrib2dv(GLuint index, const GLdouble *v);
void GLAPIENTRY
_mesa_VertexAttrib3sv(GLuint index, const GLshort *v);
void GLAPIENTRY
_mesa_VertexAttrib3dv(GLuint index, const GLdouble *v);
void GLAPIENTRY
_mesa_VertexAttrib4sv(GLuint index, const GLshort *v);
void GLAPIENTRY
_mesa_VertexAttrib4dv(GLuint index, const GLdouble *v);
void GLAPIENTRY
_mesa_VertexAttrib4bv(GLuint index, const GLbyte * v);
void GLAPIENTRY
_mesa_VertexAttrib4iv(GLuint index, const GLint * v);
void GLAPIENTRY
_mesa_VertexAttrib4ubv(GLuint index, const GLubyte * v);
void GLAPIENTRY
_mesa_VertexAttrib4usv(GLuint index, const GLushort * v);
void GLAPIENTRY
_mesa_VertexAttrib4uiv(GLuint index, const GLuint * v);
void GLAPIENTRY
_mesa_VertexAttrib4Nbv(GLuint index, const GLbyte * v);
void GLAPIENTRY
_mesa_VertexAttrib4Nsv(GLuint index, const GLshort * v);
void GLAPIENTRY
_mesa_VertexAttrib4Niv(GLuint index, const GLint * v);
void GLAPIENTRY
_mesa_VertexAttrib4Nub(GLuint index, GLubyte x, GLubyte y, GLubyte z,
                             GLubyte w);
void GLAPIENTRY
_mesa_VertexAttrib4Nubv(GLuint index, const GLubyte * v);
void GLAPIENTRY
_mesa_VertexAttrib4Nusv(GLuint index, const GLushort * v);
void GLAPIENTRY
_mesa_VertexAttrib4Nuiv(GLuint index, const GLuint * v);
void GLAPIENTRY
_mesa_VertexAttribI1iv(GLuint index, const GLint *v);
void GLAPIENTRY
_mesa_VertexAttribI1uiv(GLuint index, const GLuint *v);
void GLAPIENTRY
_mesa_VertexAttribI4bv(GLuint index, const GLbyte *v);
void GLAPIENTRY
_mesa_VertexAttribI4sv(GLuint index, const GLshort *v);
void GLAPIENTRY
_mesa_VertexAttribI4ubv(GLuint index, const GLubyte *v);
void GLAPIENTRY
_mesa_VertexAttribI4usv(GLuint index, const GLushort *v);


#endif /* API_LOOPBACK_H */

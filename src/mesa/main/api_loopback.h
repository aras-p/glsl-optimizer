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



#ifndef API_LOOPBACK_H
#define API_LOOPBACK_H

#include "main/compiler.h"
#include "main/mfeatures.h"
#include "main/glheader.h" // ?
#include "main/macros.h" // ?
#include "main/mtypes.h" // ?
#include "glapi/glapi.h" // ?
#include "glapi/glthread.h" // ?
#include "main/dispatch.h" // ?
#include "main/mfeatures.h" // ?
#include "main/context.h" // ?

struct _glapi_table;
struct gl_context;

extern void
_mesa_loopback_init_api_table(const struct gl_context *ctx,
                              struct _glapi_table *dest);
void GLAPIENTRY
loopback_Color3b_f( GLbyte red, GLbyte green, GLbyte blue );
void GLAPIENTRY
loopback_Color3d_f( GLdouble red, GLdouble green, GLdouble blue );
void GLAPIENTRY
loopback_Color3i_f( GLint red, GLint green, GLint blue );
void GLAPIENTRY
loopback_Color3s_f( GLshort red, GLshort green, GLshort blue );
void GLAPIENTRY
loopback_Color3ui_f( GLuint red, GLuint green, GLuint blue );
void GLAPIENTRY
loopback_Color3us_f( GLushort red, GLushort green, GLushort blue );
void GLAPIENTRY
loopback_Color3ub_f( GLubyte red, GLubyte green, GLubyte blue );
void GLAPIENTRY
loopback_Color3bv_f( const GLbyte *v );
void GLAPIENTRY
loopback_Color3dv_f( const GLdouble *v );
void GLAPIENTRY
loopback_Color3iv_f( const GLint *v );
void GLAPIENTRY
loopback_Color3sv_f( const GLshort *v );
void GLAPIENTRY
loopback_Color3uiv_f( const GLuint *v );
void GLAPIENTRY
loopback_Color3usv_f( const GLushort *v );
void GLAPIENTRY
loopback_Color3ubv_f( const GLubyte *v );
void GLAPIENTRY
loopback_Color4b_f( GLbyte red, GLbyte green, GLbyte blue,
                    GLbyte alpha );
void GLAPIENTRY
loopback_Color4d_f( GLdouble red, GLdouble green, GLdouble blue,
                    GLdouble alpha );
void GLAPIENTRY
loopback_Color4i_f( GLint red, GLint green, GLint blue, GLint alpha );
void GLAPIENTRY
loopback_Color4s_f( GLshort red, GLshort green, GLshort blue,
                    GLshort alpha );
void GLAPIENTRY
loopback_Color4ui_f( GLuint red, GLuint green, GLuint blue, GLuint alpha );
void GLAPIENTRY
loopback_Color4us_f( GLushort red, GLushort green, GLushort blue,
                     GLushort alpha );
void GLAPIENTRY
loopback_Color4ub_f( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha );
void GLAPIENTRY
loopback_Color4iv_f( const GLint *v );
void GLAPIENTRY
loopback_Color4bv_f( const GLbyte *v );
void GLAPIENTRY
loopback_Color4dv_f( const GLdouble *v );
void GLAPIENTRY
loopback_Color4sv_f( const GLshort *v);
void GLAPIENTRY
loopback_Color4uiv_f( const GLuint *v);
void GLAPIENTRY
loopback_Color4usv_f( const GLushort *v);
void GLAPIENTRY
loopback_Color4ubv_f( const GLubyte *v);
void GLAPIENTRY
loopback_FogCoorddEXT( GLdouble d );
void GLAPIENTRY
loopback_FogCoorddvEXT( const GLdouble *v );
void GLAPIENTRY
loopback_Indexd( GLdouble c );
void GLAPIENTRY
loopback_Indexi( GLint c );
void GLAPIENTRY
loopback_Indexs( GLshort c );
void GLAPIENTRY
loopback_Indexub( GLubyte c );
void GLAPIENTRY
loopback_Indexdv( const GLdouble *c );
void GLAPIENTRY
loopback_Indexiv( const GLint *c );
void GLAPIENTRY
loopback_Indexsv( const GLshort *c );
void GLAPIENTRY
loopback_Indexubv( const GLubyte *c );
void GLAPIENTRY
loopback_EdgeFlagv(const GLboolean *flag);
void GLAPIENTRY
loopback_Normal3b( GLbyte nx, GLbyte ny, GLbyte nz );
void GLAPIENTRY
loopback_Normal3d( GLdouble nx, GLdouble ny, GLdouble nz );
void GLAPIENTRY
loopback_Normal3i( GLint nx, GLint ny, GLint nz );
void GLAPIENTRY
loopback_Normal3s( GLshort nx, GLshort ny, GLshort nz );
void GLAPIENTRY
loopback_Normal3bv( const GLbyte *v );
void GLAPIENTRY
loopback_Normal3dv( const GLdouble *v );
void GLAPIENTRY
loopback_Normal3iv( const GLint *v );
void GLAPIENTRY
loopback_Normal3sv( const GLshort *v );
void GLAPIENTRY
loopback_TexCoord1d( GLdouble s );
void GLAPIENTRY
loopback_TexCoord1i( GLint s );
void GLAPIENTRY
loopback_TexCoord1s( GLshort s );
void GLAPIENTRY
loopback_TexCoord2d( GLdouble s, GLdouble t );
void GLAPIENTRY
loopback_TexCoord2s( GLshort s, GLshort t );
void GLAPIENTRY
loopback_TexCoord2i( GLint s, GLint t );
void GLAPIENTRY
loopback_TexCoord3d( GLdouble s, GLdouble t, GLdouble r );
void GLAPIENTRY
loopback_TexCoord3i( GLint s, GLint t, GLint r );
void GLAPIENTRY
loopback_TexCoord3s( GLshort s, GLshort t, GLshort r );
void GLAPIENTRY
loopback_TexCoord4d( GLdouble s, GLdouble t, GLdouble r, GLdouble q );
void GLAPIENTRY
loopback_TexCoord4i( GLint s, GLint t, GLint r, GLint q );
void GLAPIENTRY
loopback_TexCoord4s( GLshort s, GLshort t, GLshort r, GLshort q );
void GLAPIENTRY
loopback_TexCoord1dv( const GLdouble *v );
void GLAPIENTRY
loopback_TexCoord1iv( const GLint *v );
void GLAPIENTRY
loopback_TexCoord1sv( const GLshort *v );
void GLAPIENTRY
loopback_TexCoord2dv( const GLdouble *v );
void GLAPIENTRY
loopback_TexCoord2iv( const GLint *v );
void GLAPIENTRY
loopback_TexCoord2sv( const GLshort *v );
void GLAPIENTRY
loopback_TexCoord3dv( const GLdouble *v );
void GLAPIENTRY
loopback_TexCoord3iv( const GLint *v );
void GLAPIENTRY
loopback_TexCoord3sv( const GLshort *v );
void GLAPIENTRY
loopback_TexCoord4dv( const GLdouble *v );
void GLAPIENTRY
loopback_TexCoord4iv( const GLint *v );
void GLAPIENTRY
loopback_TexCoord4sv( const GLshort *v );
void GLAPIENTRY
loopback_Vertex2d( GLdouble x, GLdouble y );
void GLAPIENTRY
loopback_Vertex2i( GLint x, GLint y );
void GLAPIENTRY
loopback_Vertex2s( GLshort x, GLshort y );
void GLAPIENTRY
loopback_Vertex3d( GLdouble x, GLdouble y, GLdouble z );
void GLAPIENTRY
loopback_Vertex3i( GLint x, GLint y, GLint z );
void GLAPIENTRY
loopback_Vertex3s( GLshort x, GLshort y, GLshort z );
void GLAPIENTRY
loopback_Vertex4d( GLdouble x, GLdouble y, GLdouble z, GLdouble w );
void GLAPIENTRY
loopback_Vertex4i( GLint x, GLint y, GLint z, GLint w );
void GLAPIENTRY
loopback_Vertex4s( GLshort x, GLshort y, GLshort z, GLshort w );
void GLAPIENTRY
loopback_Vertex2dv( const GLdouble *v );
void GLAPIENTRY
loopback_Vertex2iv( const GLint *v );
void GLAPIENTRY
loopback_Vertex2sv( const GLshort *v );
void GLAPIENTRY
loopback_Vertex3dv( const GLdouble *v );
void GLAPIENTRY
loopback_Vertex3iv( const GLint *v );
void GLAPIENTRY
loopback_Vertex3sv( const GLshort *v );
void GLAPIENTRY
loopback_Vertex4dv( const GLdouble *v );
void GLAPIENTRY
loopback_Vertex4iv( const GLint *v );
void GLAPIENTRY
loopback_Vertex4sv( const GLshort *v );
void GLAPIENTRY
loopback_MultiTexCoord1dARB(GLenum target, GLdouble s);
void GLAPIENTRY
loopback_MultiTexCoord1dvARB(GLenum target, const GLdouble *v);
void GLAPIENTRY
loopback_MultiTexCoord1iARB(GLenum target, GLint s);
void GLAPIENTRY
loopback_MultiTexCoord1ivARB(GLenum target, const GLint *v);
void GLAPIENTRY
loopback_MultiTexCoord1sARB(GLenum target, GLshort s);
void GLAPIENTRY
loopback_MultiTexCoord1svARB(GLenum target, const GLshort *v);
void GLAPIENTRY
loopback_MultiTexCoord2dARB(GLenum target, GLdouble s, GLdouble t);
void GLAPIENTRY
loopback_MultiTexCoord2dvARB(GLenum target, const GLdouble *v);
void GLAPIENTRY
loopback_MultiTexCoord2iARB(GLenum target, GLint s, GLint t);
void GLAPIENTRY
loopback_MultiTexCoord2ivARB(GLenum target, const GLint *v);
void GLAPIENTRY
loopback_MultiTexCoord2sARB(GLenum target, GLshort s, GLshort t);
void GLAPIENTRY
loopback_MultiTexCoord2svARB(GLenum target, const GLshort *v);
void GLAPIENTRY
loopback_MultiTexCoord3dARB(GLenum target, GLdouble s, GLdouble t, GLdouble r);
void GLAPIENTRY
loopback_MultiTexCoord3dvARB(GLenum target, const GLdouble *v);
void GLAPIENTRY
loopback_MultiTexCoord3iARB(GLenum target, GLint s, GLint t, GLint r);
void GLAPIENTRY
loopback_MultiTexCoord3ivARB(GLenum target, const GLint *v);
void GLAPIENTRY
loopback_MultiTexCoord3sARB(GLenum target, GLshort s, GLshort t, GLshort r);
void GLAPIENTRY
loopback_MultiTexCoord3svARB(GLenum target, const GLshort *v);
void GLAPIENTRY
loopback_MultiTexCoord4dARB(GLenum target, GLdouble s, GLdouble t, GLdouble r,
                            GLdouble q);
void GLAPIENTRY
loopback_MultiTexCoord4dvARB(GLenum target, const GLdouble *v);
void GLAPIENTRY
loopback_MultiTexCoord4iARB(GLenum target, GLint s, GLint t, GLint r, GLint q);
void GLAPIENTRY
loopback_MultiTexCoord4ivARB(GLenum target, const GLint *v);
void GLAPIENTRY
loopback_MultiTexCoord4sARB(GLenum target, GLshort s, GLshort t, GLshort r,
                            GLshort q);
void GLAPIENTRY
loopback_MultiTexCoord4svARB(GLenum target, const GLshort *v);
void GLAPIENTRY
loopback_EvalCoord2dv( const GLdouble *u );
void GLAPIENTRY
loopback_EvalCoord2fv( const GLfloat *u );
void GLAPIENTRY
loopback_EvalCoord2d( GLdouble u, GLdouble v );
void GLAPIENTRY
loopback_EvalCoord1dv( const GLdouble *u );
void GLAPIENTRY
loopback_EvalCoord1fv( const GLfloat *u );
void GLAPIENTRY
loopback_EvalCoord1d( GLdouble u );
void GLAPIENTRY
loopback_Materialf( GLenum face, GLenum pname, GLfloat param );
void GLAPIENTRY
loopback_Materiali(GLenum face, GLenum pname, GLint param );
void GLAPIENTRY
loopback_Materialiv(GLenum face, GLenum pname, const GLint *params );
void GLAPIENTRY
loopback_Rectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
void GLAPIENTRY
loopback_Rectdv(const GLdouble *v1, const GLdouble *v2);
void GLAPIENTRY
loopback_Rectfv(const GLfloat *v1, const GLfloat *v2);
void GLAPIENTRY
loopback_Recti(GLint x1, GLint y1, GLint x2, GLint y2);
void GLAPIENTRY
loopback_Rectiv(const GLint *v1, const GLint *v2);
void GLAPIENTRY
loopback_Rects(GLshort x1, GLshort y1, GLshort x2, GLshort y2);
void GLAPIENTRY
loopback_Rectsv(const GLshort *v1, const GLshort *v2);
void GLAPIENTRY
loopback_SecondaryColor3bEXT_f( GLbyte red, GLbyte green, GLbyte blue );
void GLAPIENTRY
loopback_SecondaryColor3dEXT_f( GLdouble red, GLdouble green, GLdouble blue );
void GLAPIENTRY
loopback_SecondaryColor3iEXT_f( GLint red, GLint green, GLint blue );
void GLAPIENTRY
loopback_SecondaryColor3sEXT_f( GLshort red, GLshort green, GLshort blue );
void GLAPIENTRY
loopback_SecondaryColor3uiEXT_f( GLuint red, GLuint green, GLuint blue );
void GLAPIENTRY
loopback_SecondaryColor3usEXT_f( GLushort red, GLushort green, GLushort blue );
void GLAPIENTRY
loopback_SecondaryColor3ubEXT_f( GLubyte red, GLubyte green, GLubyte blue );
void GLAPIENTRY
loopback_SecondaryColor3bvEXT_f( const GLbyte *v );
void GLAPIENTRY
loopback_SecondaryColor3dvEXT_f( const GLdouble *v );
void GLAPIENTRY
loopback_SecondaryColor3ivEXT_f( const GLint *v );
void GLAPIENTRY
loopback_SecondaryColor3svEXT_f( const GLshort *v );
void GLAPIENTRY
loopback_SecondaryColor3uivEXT_f( const GLuint *v );
void GLAPIENTRY
loopback_SecondaryColor3usvEXT_f( const GLushort *v );
void GLAPIENTRY
loopback_SecondaryColor3ubvEXT_f( const GLubyte *v );
void GLAPIENTRY
loopback_VertexAttrib1sNV(GLuint index, GLshort x);
void GLAPIENTRY
loopback_VertexAttrib1dNV(GLuint index, GLdouble x);
void GLAPIENTRY
loopback_VertexAttrib2sNV(GLuint index, GLshort x, GLshort y);
void GLAPIENTRY
loopback_VertexAttrib2dNV(GLuint index, GLdouble x, GLdouble y);
void GLAPIENTRY
loopback_VertexAttrib3sNV(GLuint index, GLshort x, GLshort y, GLshort z);
void GLAPIENTRY
loopback_VertexAttrib3dNV(GLuint index, GLdouble x, GLdouble y, GLdouble z);
void GLAPIENTRY
loopback_VertexAttrib4sNV(GLuint index, GLshort x, GLshort y, GLshort z,
                          GLshort w);
void GLAPIENTRY
loopback_VertexAttrib4dNV(GLuint index, GLdouble x, GLdouble y, GLdouble z,
                          GLdouble w);
void GLAPIENTRY
loopback_VertexAttrib4ubNV(GLuint index, GLubyte x, GLubyte y, GLubyte z,
                           GLubyte w);
void GLAPIENTRY
loopback_VertexAttrib1svNV(GLuint index, const GLshort *v);
void GLAPIENTRY
loopback_VertexAttrib1dvNV(GLuint index, const GLdouble *v);
void GLAPIENTRY
loopback_VertexAttrib2svNV(GLuint index, const GLshort *v);
void GLAPIENTRY
loopback_VertexAttrib2dvNV(GLuint index, const GLdouble *v);
void GLAPIENTRY
loopback_VertexAttrib3svNV(GLuint index, const GLshort *v);
void GLAPIENTRY
loopback_VertexAttrib3dvNV(GLuint index, const GLdouble *v);
void GLAPIENTRY
loopback_VertexAttrib4svNV(GLuint index, const GLshort *v);
void GLAPIENTRY
loopback_VertexAttrib4dvNV(GLuint index, const GLdouble *v);
void GLAPIENTRY
loopback_VertexAttrib4ubvNV(GLuint index, const GLubyte *v);
void GLAPIENTRY
loopback_VertexAttribs1svNV(GLuint index, GLsizei n, const GLshort *v);
void GLAPIENTRY
loopback_VertexAttribs1fvNV(GLuint index, GLsizei n, const GLfloat *v);
void GLAPIENTRY
loopback_VertexAttribs1dvNV(GLuint index, GLsizei n, const GLdouble *v);
void GLAPIENTRY
loopback_VertexAttribs2svNV(GLuint index, GLsizei n, const GLshort *v);
void GLAPIENTRY
loopback_VertexAttribs2fvNV(GLuint index, GLsizei n, const GLfloat *v);
void GLAPIENTRY
loopback_VertexAttribs2dvNV(GLuint index, GLsizei n, const GLdouble *v);
void GLAPIENTRY
loopback_VertexAttribs3svNV(GLuint index, GLsizei n, const GLshort *v);
void GLAPIENTRY
loopback_VertexAttribs3fvNV(GLuint index, GLsizei n, const GLfloat *v);
void GLAPIENTRY
loopback_VertexAttribs3dvNV(GLuint index, GLsizei n, const GLdouble *v);
void GLAPIENTRY
loopback_VertexAttribs4svNV(GLuint index, GLsizei n, const GLshort *v);
void GLAPIENTRY
loopback_VertexAttribs4fvNV(GLuint index, GLsizei n, const GLfloat *v);
void GLAPIENTRY
loopback_VertexAttribs4dvNV(GLuint index, GLsizei n, const GLdouble *v);
void GLAPIENTRY
loopback_VertexAttribs4ubvNV(GLuint index, GLsizei n, const GLubyte *v);
void GLAPIENTRY
loopback_VertexAttrib1sARB(GLuint index, GLshort x);
void GLAPIENTRY
loopback_VertexAttrib1dARB(GLuint index, GLdouble x);
void GLAPIENTRY
loopback_VertexAttrib2sARB(GLuint index, GLshort x, GLshort y);
void GLAPIENTRY
loopback_VertexAttrib2dARB(GLuint index, GLdouble x, GLdouble y);
void GLAPIENTRY
loopback_VertexAttrib3sARB(GLuint index, GLshort x, GLshort y, GLshort z);
void GLAPIENTRY
loopback_VertexAttrib3dARB(GLuint index, GLdouble x, GLdouble y, GLdouble z);
void GLAPIENTRY
loopback_VertexAttrib4sARB(GLuint index, GLshort x, GLshort y, GLshort z,
                           GLshort w);
void GLAPIENTRY
loopback_VertexAttrib4dARB(GLuint index, GLdouble x, GLdouble y, GLdouble z,
                           GLdouble w);
void GLAPIENTRY
loopback_VertexAttrib1svARB(GLuint index, const GLshort *v);
void GLAPIENTRY
loopback_VertexAttrib1dvARB(GLuint index, const GLdouble *v);
void GLAPIENTRY
loopback_VertexAttrib2svARB(GLuint index, const GLshort *v);
void GLAPIENTRY
loopback_VertexAttrib2dvARB(GLuint index, const GLdouble *v);
void GLAPIENTRY
loopback_VertexAttrib3svARB(GLuint index, const GLshort *v);
void GLAPIENTRY
loopback_VertexAttrib3dvARB(GLuint index, const GLdouble *v);
void GLAPIENTRY
loopback_VertexAttrib4svARB(GLuint index, const GLshort *v);
void GLAPIENTRY
loopback_VertexAttrib4dvARB(GLuint index, const GLdouble *v);
void GLAPIENTRY
loopback_VertexAttrib4bvARB(GLuint index, const GLbyte * v);
void GLAPIENTRY
loopback_VertexAttrib4ivARB(GLuint index, const GLint * v);
void GLAPIENTRY
loopback_VertexAttrib4ubvARB(GLuint index, const GLubyte * v);
void GLAPIENTRY
loopback_VertexAttrib4usvARB(GLuint index, const GLushort * v);
void GLAPIENTRY
loopback_VertexAttrib4uivARB(GLuint index, const GLuint * v);
void GLAPIENTRY
loopback_VertexAttrib4NbvARB(GLuint index, const GLbyte * v);
void GLAPIENTRY
loopback_VertexAttrib4NsvARB(GLuint index, const GLshort * v);
void GLAPIENTRY
loopback_VertexAttrib4NivARB(GLuint index, const GLint * v);
void GLAPIENTRY
loopback_VertexAttrib4NubARB(GLuint index, GLubyte x, GLubyte y, GLubyte z,
                             GLubyte w);
void GLAPIENTRY
loopback_VertexAttrib4NubvARB(GLuint index, const GLubyte * v);
void GLAPIENTRY
loopback_VertexAttrib4NusvARB(GLuint index, const GLushort * v);
void GLAPIENTRY
loopback_VertexAttrib4NuivARB(GLuint index, const GLuint * v);
void GLAPIENTRY
loopback_VertexAttribI1iv(GLuint index, const GLint *v);
void GLAPIENTRY
loopback_VertexAttribI1uiv(GLuint index, const GLuint *v);
void GLAPIENTRY
loopback_VertexAttribI4bv(GLuint index, const GLbyte *v);
void GLAPIENTRY
loopback_VertexAttribI4sv(GLuint index, const GLshort *v);
void GLAPIENTRY
loopback_VertexAttribI4ubv(GLuint index, const GLubyte *v);
void GLAPIENTRY
loopback_VertexAttribI4usv(GLuint index, const GLushort *v);


#endif /* API_LOOPBACK_H */

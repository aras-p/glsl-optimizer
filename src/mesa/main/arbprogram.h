/*
 * Mesa 3-D graphics library
 * Version:  5.1
 *
 * Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
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


#ifndef ARBPROGRAM_H
#define ARBPROGRAM_H

extern void
_mesa_VertexAttrib1sARB(GLuint index, GLshort x);

extern void
_mesa_VertexAttrib1fARB(GLuint index, GLfloat x);

extern void
_mesa_VertexAttrib1dARB(GLuint index, GLdouble x);

extern void
_mesa_VertexAttrib2sARB(GLuint index, GLshort x, GLshort y);

extern void
_mesa_VertexAttrib2fARB(GLuint index, GLfloat x, GLfloat y);

extern void
_mesa_VertexAttrib2dARB(GLuint index, GLdouble x, GLdouble y);

extern void
_mesa_VertexAttrib3sARB(GLuint index, GLshort x, GLshort y, GLshort z);

extern void
_mesa_VertexAttrib3fARB(GLuint index, GLfloat x, GLfloat y, GLfloat z);

extern void
_mesa_VertexAttrib3dARB(GLuint index, GLdouble x, GLdouble y, GLdouble z);

extern void
_mesa_VertexAttrib4sARB(GLuint index, GLshort x, GLshort y, GLshort z, GLshort w);

extern void
_mesa_VertexAttrib4fARB(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);

extern void
_mesa_VertexAttrib4dARB(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);

extern void
_mesa_VertexAttrib4NubARB(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w);

extern void
_mesa_VertexAttrib1svARB(GLuint index, const GLshort *v);

extern void
_mesa_VertexAttrib1fvARB(GLuint index, const GLfloat *v);

extern void
_mesa_VertexAttrib1dvARB(GLuint index, const GLdouble *v);

extern void
_mesa_VertexAttrib2svARB(GLuint index, const GLshort *v);

extern void
_mesa_VertexAttrib2fvARB(GLuint index, const GLfloat *v);

extern void
_mesa_VertexAttrib2dvARB(GLuint index, const GLdouble *v);

extern void
_mesa_VertexAttrib3svARB(GLuint index, const GLshort *v);

extern void
_mesa_VertexAttrib3fvARB(GLuint index, const GLfloat *v);

extern void
_mesa_VertexAttrib3dvARB(GLuint index, const GLdouble *v);

extern void
_mesa_VertexAttrib4bvARB(GLuint index, const GLbyte *v);

extern void
_mesa_VertexAttrib4svARB(GLuint index, const GLshort *v);

extern void
_mesa_VertexAttrib4ivARB(GLuint index, const GLint *v);

extern void
_mesa_VertexAttrib4ubvARB(GLuint index, const GLubyte *v);

extern void
_mesa_VertexAttrib4usvARB(GLuint index, const GLushort *v);

extern void
_mesa_VertexAttrib4uivARB(GLuint index, const GLuint *v);

extern void
_mesa_VertexAttrib4fvARB(GLuint index, const GLfloat *v);

extern void
_mesa_VertexAttrib4dvARB(GLuint index, const GLdouble *v);

extern void
_mesa_VertexAttrib4NbvARB(GLuint index, const GLbyte *v);

extern void
_mesa_VertexAttrib4NsvARB(GLuint index, const GLshort *v);

extern void
_mesa_VertexAttrib4NivARB(GLuint index, const GLint *v);

extern void
_mesa_VertexAttrib4NubvARB(GLuint index, const GLubyte *v);

extern void
_mesa_VertexAttrib4NusvARB(GLuint index, const GLushort *v);

extern void
_mesa_VertexAttrib4NuivARB(GLuint index, const GLuint *v);


extern void
_mesa_VertexAttribPointerARB(GLuint index, GLint size, GLenum type,
                             GLboolean normalized, GLsizei stride,
                             const GLvoid *pointer);


extern void
_mesa_EnableVertexAttribArrayARB(GLuint index);


extern void
_mesa_DisableVertexAttribArrayARB(GLuint index);


extern void
_mesa_GetVertexAttribdvARB(GLuint index, GLenum pname, GLdouble *params);


extern void
_mesa_GetVertexAttribfvARB(GLuint index, GLenum pname, GLfloat *params);


extern void
_mesa_GetVertexAttribivARB(GLuint index, GLenum pname, GLint *params);


extern void
_mesa_GetVertexAttribPointervARB(GLuint index, GLenum pname, GLvoid **pointer);


extern void
_mesa_ProgramStringARB(GLenum target, GLenum format, GLsizei len,
                       const GLvoid *string);


extern void
_mesa_BindProgramARB(GLenum target, GLuint program);


extern void
_mesa_DeleteProgramsARB(GLsizei n, const GLuint *programs);


extern void
_mesa_GenProgramsARB(GLsizei n, GLuint *programs);


extern void
_mesa_ProgramEnvParameter4dARB(GLenum target, GLuint index,
                               GLdouble x, GLdouble y, GLdouble z, GLdouble w);


extern void
_mesa_ProgramEnvParameter4dvARB(GLenum target, GLuint index,
                                const GLdouble *params);


extern void
_mesa_ProgramEnvParameter4fARB(GLenum target, GLuint index,
                               GLfloat x, GLfloat y, GLfloat z, GLfloat w);


extern void
_mesa_ProgramEnvParameter4fvARB(GLenum target, GLuint index,
                                const GLfloat *params);


extern void
_mesa_ProgramLocalParameter4dARB(GLenum target, GLuint index,
                                 GLdouble x, GLdouble y,
                                 GLdouble z, GLdouble w);


extern void
_mesa_ProgramLocalParameter4dvARB(GLenum target, GLuint index,
                                  const GLdouble *params);


extern void
_mesa_ProgramLocalParameter4fARB(GLenum target, GLuint index,
                                 GLfloat x, GLfloat y, GLfloat z, GLfloat w);


extern void
_mesa_ProgramLocalParameter4fvARB(GLenum target, GLuint index,
                                  const GLfloat *params);


extern void
_mesa_GetProgramEnvParameterdvARB(GLenum target, GLuint index,
                                  GLdouble *params);


extern void
_mesa_GetProgramEnvParameterfvARB(GLenum target, GLuint index, 
                                  GLfloat *params);


extern void
_mesa_GetProgramLocalParameterdvARB(GLenum target, GLuint index,
                                    GLdouble *params);


extern void
_mesa_GetProgramLocalParameterfvARB(GLenum target, GLuint index, 
                                    GLfloat *params);


extern void
_mesa_GetProgramivARB(GLenum target, GLenum pname, GLint *params);


extern void
_mesa_GetProgramStringARB(GLenum target, GLenum pname, GLvoid *string);


extern GLboolean
_mesa_IsProgramARB(GLuint program);

#endif


/* $Id: nvprogram.h,v 1.7 2003/04/17 01:48:20 brianp Exp $ */

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
 *
 * Authors:
 *    Brian Paul
 */


#ifndef NVPROGRAM_H
#define NVPROGRAM_H


/** Internal functions **/
extern void
_mesa_set_program_error(GLcontext *ctx, GLint pos, const char *string);

extern const GLubyte *
_mesa_find_line_column(const GLubyte *string, const GLubyte *pos,
                       GLint *line, GLint *col);

extern struct program *
_mesa_alloc_program(GLcontext *ctx, GLenum target, GLuint id);

extern void
_mesa_delete_program(GLcontext *ctx, struct program *prog);



/** API functions **/

extern void
_mesa_BindProgramNV(GLenum target, GLuint id);

extern void
_mesa_DeleteProgramsNV(GLsizei n, const GLuint *ids);

extern void
_mesa_ExecuteProgramNV(GLenum target, GLuint id, const GLfloat *params);

extern void
_mesa_GenProgramsNV(GLsizei n, GLuint *ids);

extern GLboolean
_mesa_AreProgramsResidentNV(GLsizei n, const GLuint *ids, GLboolean *residences);

extern void
_mesa_RequestResidentProgramsNV(GLsizei n, const GLuint *ids);

extern void
_mesa_GetProgramParameterfvNV(GLenum target, GLuint index, GLenum pname, GLfloat *params);

extern void
_mesa_GetProgramParameterdvNV(GLenum target, GLuint index, GLenum pname, GLdouble *params);

extern void
_mesa_GetProgramivNV(GLuint id, GLenum pname, GLint *params);

extern void
_mesa_GetProgramStringNV(GLuint id, GLenum pname, GLubyte *program);

extern void
_mesa_GetTrackMatrixivNV(GLenum target, GLuint address, GLenum pname, GLint *params);

extern void
_mesa_GetVertexAttribdvNV(GLuint index, GLenum pname, GLdouble *params);

extern void
_mesa_GetVertexAttribfvNV(GLuint index, GLenum pname, GLfloat *params);

extern void
_mesa_GetVertexAttribivNV(GLuint index, GLenum pname, GLint *params);

extern void
_mesa_GetVertexAttribPointervNV(GLuint index, GLenum pname, GLvoid **pointer);

extern GLboolean
_mesa_IsProgramNV(GLuint id);

extern void
_mesa_LoadProgramNV(GLenum target, GLuint id, GLsizei len, const GLubyte *program);

extern void
_mesa_ProgramParameter4dNV(GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);

extern void
_mesa_ProgramParameter4dvNV(GLenum target, GLuint index, const GLdouble *params);

extern void
_mesa_ProgramParameter4fNV(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);

extern void
_mesa_ProgramParameter4fvNV(GLenum target, GLuint index, const GLfloat *params);

extern void
_mesa_ProgramParameters4dvNV(GLenum target, GLuint index, GLuint num, const GLdouble *params);

extern void
_mesa_ProgramParameters4fvNV(GLenum target, GLuint index, GLuint num, const GLfloat *params);

extern void
_mesa_TrackMatrixNV(GLenum target, GLuint address, GLenum matrix, GLenum transform);


extern void
_mesa_ProgramNamedParameter4fNV(GLuint id, GLsizei len, const GLubyte *name,
                                GLfloat x, GLfloat y, GLfloat z, GLfloat w);

extern void
_mesa_ProgramNamedParameter4fvNV(GLuint id, GLsizei len, const GLubyte *name,
                                 const float v[]);

extern void
_mesa_ProgramNamedParameter4dNV(GLuint id, GLsizei len, const GLubyte *name,
                                GLdouble x, GLdouble y, GLdouble z, GLdouble w);

extern void
_mesa_ProgramNamedParameter4dvNV(GLuint id, GLsizei len, const GLubyte *name,
                                 const double v[]);

extern void
_mesa_GetProgramNamedParameterfvNV(GLuint id, GLsizei len, const GLubyte *name,
                                   GLfloat *params);

extern void
_mesa_GetProgramNamedParameterdvNV(GLuint id, GLsizei len, const GLubyte *name,
                                   GLdouble *params);

extern void
_mesa_ProgramLocalParameter4fARB(GLenum target, GLuint index,
                                 GLfloat x, GLfloat y, GLfloat z, GLfloat w);

extern void
_mesa_ProgramLocalParameter4fvARB(GLenum target, GLuint index,
                                  const GLfloat *params);

extern void
_mesa_ProgramLocalParameter4dARB(GLenum target, GLuint index,
                                 GLdouble x, GLdouble y,
                                 GLdouble z, GLdouble w);

extern void
_mesa_ProgramLocalParameter4dvARB(GLenum target, GLuint index,
                                  const GLdouble *params);

extern void
_mesa_GetProgramLocalParameterfvARB(GLenum target, GLuint index,
                                    GLfloat *params);

extern void
_mesa_GetProgramLocalParameterdvARB(GLenum target, GLuint index,
                                    GLdouble *params);


#endif

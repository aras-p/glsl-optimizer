/* $Id: varray.h,v 1.5 1999/11/11 01:22:28 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.1
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


#ifndef VARRAY_H
#define VARRAY_H


#include "types.h"


extern void
_mesa_VertexPointer(GLint size, GLenum type, GLsizei stride,
                    const GLvoid *ptr);


extern void
_mesa_NormalPointer(GLenum type, GLsizei stride, const GLvoid *ptr);


extern void
_mesa_ColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr);


extern void
_mesa_IndexPointer(GLenum type, GLsizei stride, const GLvoid *ptr);


extern void
_mesa_TexCoordPointer(GLint size, GLenum type, GLsizei stride,
                      const GLvoid *ptr);


extern void
_mesa_EdgeFlagPointer(GLsizei stride, const GLvoid *ptr);


extern void
_mesa_DrawArrays(GLenum mode, GLint first, GLsizei count);


extern void
_mesa_save_DrawArrays(GLenum mode, GLint first, GLsizei count);


extern void
_mesa_DrawElements(GLenum mode, GLsizei count, GLenum type,
                   const GLvoid *indices);


extern void
_mesa_save_DrawElements(GLenum mode, GLsizei count,
                        GLenum type, const GLvoid *indices);


extern void
_mesa_InterleavedArrays(GLenum format, GLsizei stride, const GLvoid *pointer);

extern void
_mesa_save_InterleavedArrays(GLenum format, GLsizei stride,
                             const GLvoid *pointer);


extern void
_mesa_DrawRangeElements(GLenum mode, GLuint start,
                        GLuint end, GLsizei count, GLenum type,
                        const GLvoid *indices);

extern void
_mesa_save_DrawRangeElements(GLenum mode,
                             GLuint start, GLuint end, GLsizei count,
                             GLenum type, const GLvoid *indices );


extern void gl_exec_array_elements( GLcontext *ctx, 
				    struct immediate *IM,
				    GLuint start, 
				    GLuint end );

extern void gl_update_client_state( GLcontext *ctx );


#endif

/* $Id: varray.h,v 1.4 1999/10/19 18:37:05 keithw Exp $ */

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


extern void gl_VertexPointer( GLcontext *ctx,
                              GLint size, GLenum type, GLsizei stride,
                              const GLvoid *ptr );


extern void gl_NormalPointer( GLcontext *ctx,
                              GLenum type, GLsizei stride, const GLvoid *ptr );


extern void gl_ColorPointer( GLcontext *ctx,
                             GLint size, GLenum type, GLsizei stride,
                             const GLvoid *ptr );


extern void gl_IndexPointer( GLcontext *ctx,
                                GLenum type, GLsizei stride,
                                const GLvoid *ptr );


extern void gl_TexCoordPointer( GLcontext *ctx,
                                GLint size, GLenum type, GLsizei stride,
                                const GLvoid *ptr );


extern void gl_EdgeFlagPointer( GLcontext *ctx,
                                GLsizei stride, const GLboolean *ptr );


extern void gl_GetPointerv( GLcontext *ctx, GLenum pname, GLvoid **params );



extern void gl_DrawArrays( GLcontext *ctx,
                           GLenum mode, GLint first, GLsizei count );

extern void gl_save_DrawArrays( GLcontext *ctx,
                                GLenum mode, GLint first, GLsizei count );


extern void gl_DrawElements( GLcontext *ctx,
                             GLenum mode, GLsizei count,
                             GLenum type, const GLvoid *indices );

extern void gl_save_DrawElements( GLcontext *ctx,
                                  GLenum mode, GLsizei count,
                                  GLenum type, const GLvoid *indices );


extern void gl_InterleavedArrays( GLcontext *ctx,
                                  GLenum format, GLsizei stride,
                                  const GLvoid *pointer );

extern void gl_save_InterleavedArrays( GLcontext *ctx,
                                       GLenum format, GLsizei stride,
                                       const GLvoid *pointer );


extern void gl_DrawRangeElements( GLcontext *ctx, GLenum mode, GLuint start,
                                  GLuint end, GLsizei count, GLenum type,
                                  const GLvoid *indices );

extern void gl_save_DrawRangeElements( GLcontext *ctx, GLenum mode,
                                       GLuint start, GLuint end, GLsizei count,
                                       GLenum type, const GLvoid *indices );


extern void gl_exec_array_elements( GLcontext *ctx, 
				    struct immediate *IM,
				    GLuint start, 
				    GLuint end );

extern void gl_update_client_state( GLcontext *ctx );

#endif

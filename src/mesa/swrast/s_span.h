/* $Id: s_span.h,v 1.3 2001/03/03 20:33:30 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
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





#ifndef S_SPAN_H
#define S_SPAN_H


#include "mtypes.h"
#include "swrast.h"


extern void _mesa_write_index_span( GLcontext *ctx,
                                 GLuint n, GLint x, GLint y, const GLdepth z[],
				 const GLfixed fog[],
				 GLuint index[], GLenum primitive );


extern void _mesa_write_monoindex_span( GLcontext *ctx,
                                     GLuint n, GLint x, GLint y,
                                     const GLdepth z[],
				     const GLfixed fog[],
				     GLuint index, GLenum primitive );


extern void _mesa_write_rgba_span( GLcontext *ctx,
                                GLuint n, GLint x, GLint y, const GLdepth z[],
				const GLfixed fog[],
                                GLchan rgba[][4], GLenum primitive );


extern void _mesa_write_monocolor_span( GLcontext *ctx,
                                     GLuint n, GLint x, GLint y,
                                     const GLdepth z[],
				     const GLfixed fog[],
				     const GLchan color[4],
                                     GLenum primitive );


extern void _mesa_write_texture_span( GLcontext *ctx,
                                   GLuint n, GLint x, GLint y,
                                   const GLdepth z[],
				   const GLfixed fog[],
				   const GLfloat s[], const GLfloat t[],
                                   const GLfloat u[], GLfloat lambda[],
				   GLchan rgba[][4], CONST GLchan spec[][4],
                                   GLenum primitive );


extern void
_mesa_write_multitexture_span( GLcontext *ctx,
                            GLuint n, GLint x, GLint y,
                            const GLdepth z[],
			    const GLfixed fog[],
                            CONST GLfloat s[MAX_TEXTURE_UNITS][MAX_WIDTH],
                            CONST GLfloat t[MAX_TEXTURE_UNITS][MAX_WIDTH],
                            CONST GLfloat u[MAX_TEXTURE_UNITS][MAX_WIDTH],
                            GLfloat lambda[MAX_TEXTURE_UNITS][MAX_WIDTH],
                            GLchan rgba[][4],
                            CONST GLchan spec[][4],
                            GLenum primitive );


extern void _mesa_read_rgba_span( GLcontext *ctx, GLframebuffer *buffer,
                               GLuint n, GLint x, GLint y,
                               GLchan rgba[][4] );


extern void _mesa_read_index_span( GLcontext *ctx, GLframebuffer *buffer,
                                GLuint n, GLint x, GLint y, GLuint indx[] );


#endif

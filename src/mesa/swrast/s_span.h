/* $Id: s_span.h,v 1.9 2002/01/10 16:54:29 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  4.1
 *
 * Copyright (C) 1999-2002  Brian Paul   All Rights Reserved.
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


extern void
_old_write_index_span( GLcontext *ctx, GLuint n, GLint x, GLint y,
                        const GLdepth z[], const GLfloat fog[],
                        GLuint index[], const GLint coverage[],
                        GLenum primitive );

extern void
_old_write_rgba_span( GLcontext *ctx, GLuint n, GLint x, GLint y,
                       const GLdepth z[], const GLfloat fog[],
                       GLchan rgba[][4], const GLfloat coverage[],
                       GLenum primitive );

void
_mesa_write_index_span( GLcontext *ctx,	struct sw_span *span,
			GLenum primitive);

extern void
_mesa_write_monoindex_span( GLcontext *ctx, struct sw_span *span,
                            GLuint index, GLenum primitive );

extern void
_mesa_write_rgba_span( GLcontext *ctx, struct sw_span *span,
                       GLenum primitive );

extern void
_mesa_write_monocolor_span( GLcontext *ctx, struct sw_span *span,
			    const GLchan color[4], GLenum primitive );


extern void
_mesa_rasterize_span(GLcontext *ctx, struct sw_span *span);


extern void
_old_write_texture_span( GLcontext *ctx, GLuint n, GLint x, GLint y,
			 const GLdepth z[], const GLfloat fog[],
			 GLfloat texcoord[][4], GLfloat lambda[],
			 GLchan rgba[][4], GLchan spec[][4],
			 const GLfloat coverage[], GLenum primitive );


extern void
_old_write_multitexture_span( GLcontext *ctx, GLuint n, GLint x, GLint y,
                              const GLdepth z[], const GLfloat fog[],
                              GLfloat texcoord[MAX_TEXTURE_UNITS][MAX_WIDTH][4],
                              GLfloat lambda[MAX_TEXTURE_UNITS][MAX_WIDTH],
                              GLchan rgba[][4], GLchan spec[][4],
                              const GLfloat coverage[],  GLenum primitive );


extern void
_mesa_read_rgba_span( GLcontext *ctx, GLframebuffer *buffer,
                      GLuint n, GLint x, GLint y, GLchan rgba[][4] );


extern void
_mesa_read_index_span( GLcontext *ctx, GLframebuffer *buffer,
                       GLuint n, GLint x, GLint y, GLuint indx[] );


#endif

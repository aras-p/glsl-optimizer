/* $Id: stencil.h,v 1.1 1999/08/19 00:55:41 jtg Exp $ */

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





#ifndef STENCIL_H
#define STENCIL_H


#include "types.h"


extern void gl_ClearStencil( GLcontext *ctx, GLint s );


extern void gl_StencilFunc( GLcontext *ctx, GLenum func,
                            GLint ref, GLuint mask );


extern void gl_StencilMask( GLcontext *ctx, GLuint mask );


extern void gl_StencilOp( GLcontext *ctx, GLenum fail,
                          GLenum zfail, GLenum zpass );



extern GLint gl_stencil_span( GLcontext *ctx,
                              GLuint n, GLint x, GLint y, GLubyte mask[] );


extern void gl_depth_stencil_span( GLcontext *ctx, GLuint n, GLint x, GLint y,
				   const GLdepth z[], GLubyte mask[] );


extern GLint gl_stencil_pixels( GLcontext *ctx,
                                GLuint n, const GLint x[], const GLint y[],
			        GLubyte mask[] );


extern void gl_depth_stencil_pixels( GLcontext *ctx,
                                     GLuint n, const GLint x[],
				     const GLint y[], const GLdepth z[],
				     GLubyte mask[] );


extern void gl_read_stencil_span( GLcontext *ctx,
                                  GLuint n, GLint x, GLint y,
				  GLstencil stencil[] );


extern void gl_write_stencil_span( GLcontext *ctx,
                                   GLuint n, GLint x, GLint y,
				   const GLstencil stencil[] );


extern void gl_alloc_stencil_buffer( GLcontext *ctx );


extern void gl_clear_stencil_buffer( GLcontext *ctx );


#endif

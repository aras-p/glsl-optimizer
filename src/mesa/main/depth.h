
/* $Id: depth.h,v 1.2 1999/10/08 09:27:10 keithw Exp $ */

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





#ifndef DEPTH_H
#define DEPTH_H


#include "types.h"


/*
 * Return the address of the Z-buffer value for window coordinate (x,y):
 */
#define Z_ADDRESS( CTX, X, Y )  \
            ((CTX)->Buffer->Depth + (CTX)->Buffer->Width * (Y) + (X))




extern GLuint
gl_depth_test_span_generic( GLcontext* ctx, GLuint n, GLint x, GLint y,
                            const GLdepth z[], GLubyte mask[] );

extern GLuint
gl_depth_test_span_less( GLcontext* ctx, GLuint n, GLint x, GLint y,
                         const GLdepth z[], GLubyte mask[] );

extern GLuint
gl_depth_test_span_greater( GLcontext* ctx, GLuint n, GLint x, GLint y,
                            const GLdepth z[], GLubyte mask[] );



extern void
gl_depth_test_pixels_generic( GLcontext* ctx,
                              GLuint n, const GLint x[], const GLint y[],
                              const GLdepth z[], GLubyte mask[] );

extern void
gl_depth_test_pixels_less( GLcontext* ctx,
                           GLuint n, const GLint x[], const GLint y[],
                           const GLdepth z[], GLubyte mask[] );

extern void
gl_depth_test_pixels_greater( GLcontext* ctx,
                              GLuint n, const GLint x[], const GLint y[],
                              const GLdepth z[], GLubyte mask[] );


extern void gl_read_depth_span_float( GLcontext* ctx,
                                      GLuint n, GLint x, GLint y,
                                      GLfloat depth[] );


extern void gl_read_depth_span_int( GLcontext* ctx, GLuint n, GLint x, GLint y,
                                    GLdepth depth[] );


extern void gl_alloc_depth_buffer( GLcontext* ctx );


extern void gl_clear_depth_buffer( GLcontext* ctx );


extern void gl_ClearDepth( GLcontext* ctx, GLclampd depth );

extern void gl_DepthFunc( GLcontext* ctx, GLenum func );

extern void gl_DepthMask( GLcontext* ctx, GLboolean flag );

#endif

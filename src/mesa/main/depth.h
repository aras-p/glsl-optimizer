/* $Id: depth.h,v 1.8 2000/04/11 20:42:22 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.3
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


#ifndef DEPTH_H
#define DEPTH_H


#include "types.h"


/*
 * Immediate-mode API entrpoints
 */

extern void
_mesa_ClearDepth( GLclampd depth );


extern void
_mesa_DepthFunc( GLenum func );


extern void
_mesa_DepthMask( GLboolean flag );



/*
 * Internal functions
 */

extern GLvoid *
_mesa_zbuffer_address(GLcontext *ctx, GLint x, GLint y);


extern GLuint
_mesa_depth_test_span( GLcontext *ctx, GLuint n, GLint x, GLint y,
                       const GLdepth z[], GLubyte mask[] );

extern void
_mesa_depth_test_pixels( GLcontext *ctx,
                         GLuint n, const GLint x[], const GLint y[],
                         const GLdepth z[], GLubyte mask[] );


extern void
_mesa_read_depth_span( GLcontext *ctx,
                       GLint n, GLint x, GLint y, GLdepth depth[] );


extern void
_mesa_read_depth_span_float( GLcontext *ctx, GLint n, GLint x, GLint y,
                             GLfloat depth[] );


extern void
_mesa_alloc_depth_buffer( GLcontext* ctx );


extern void
_mesa_clear_depth_buffer( GLcontext* ctx );



#endif

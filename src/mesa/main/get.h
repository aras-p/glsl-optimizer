/* $Id: get.h,v 1.2 1999/09/09 23:47:09 brianp Exp $ */

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





#ifndef GET_H
#define GET_H


#include "types.h"


extern void gl_GetBooleanv( GLcontext *ctx, GLenum pname, GLboolean *params );

extern void gl_GetDoublev( GLcontext *ctx, GLenum pname, GLdouble *params );

extern void gl_GetFloatv( GLcontext *ctx, GLenum pname, GLfloat *params );

extern void gl_GetIntegerv( GLcontext *ctx, GLenum pname, GLint *params );

extern void gl_GetPointerv( GLcontext *ctx, GLenum pname, GLvoid **params );

extern const GLubyte *gl_GetString( GLcontext *ctx, GLenum name );


#endif


/* $Id: blend.h,v 1.4 2000/02/24 22:04:03 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.3
 * 
 * Copyright (C) 2000  Brian Paul   All Rights Reserved.
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


#ifndef BLEND_H
#define BLEND_H


#include "types.h"



extern void
_mesa_blend_span( GLcontext *ctx, GLuint n, GLint x, GLint y,
                  GLubyte rgba[][4], const GLubyte mask[] );


extern void
_mesa_blend_pixels( GLcontext *ctx,
                    GLuint n, const GLint x[], const GLint y[],
                    GLubyte rgba[][4], const GLubyte mask[] );


extern void
_mesa_BlendFunc( GLenum sfactor, GLenum dfactor );


extern void
_mesa_BlendFuncSeparateEXT( GLenum sfactorRGB, GLenum dfactorRGB,
                            GLenum sfactorA, GLenum dfactorA );


extern void
_mesa_BlendEquation( GLenum mode );


extern void
_mesa_BlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);


#endif

/* $Id: s_texture.h,v 1.6 2001/03/12 00:48:42 gareth Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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


#ifndef S_TEXTURE_H
#define S_TEXTURE_H


#include "mtypes.h"
#include "swrast.h"


extern void
_swrast_choose_texture_sample_func( GLcontext *ctx,
				    GLuint texUnit,
				    const struct gl_texture_object *tObj );


extern void
_swrast_texture_fragments( GLcontext *ctx, GLuint texSet, GLuint n,
                           const GLfloat s[], const GLfloat t[],
                           const GLfloat r[], GLfloat lambda[],
                           CONST GLchan primary_rgba[][4], GLchan rgba[][4] );


#endif

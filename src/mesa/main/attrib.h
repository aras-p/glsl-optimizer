/* $Id: attrib.h,v 1.2 1999/11/11 01:22:25 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.3
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


#ifndef ATTRIB_H
#define ATTRIB_h


#include "types.h"


extern void gl_PushAttrib( GLcontext* ctx, GLbitfield mask );

extern void gl_PopAttrib( GLcontext* ctx );

extern void gl_PushClientAttrib( GLcontext *ctx, GLbitfield mask );

extern void gl_PopClientAttrib( GLcontext *ctx );


extern void
_mesa_PushAttrib( GLbitfield mask );

extern void
_mesa_PopAttrib( void );

extern void
_mesa_PushClientAttrib( GLbitfield mask );

extern void
_mesa_PopClientAttrib( void );


#endif

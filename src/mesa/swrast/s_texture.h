/* $Id: s_texture.h,v 1.10 2002/01/28 00:07:33 brianp Exp $ */

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


#ifndef S_TEXTURE_H
#define S_TEXTURE_H


#include "mtypes.h"
#include "swrast.h"


extern void
_swrast_choose_texture_sample_func( GLcontext *ctx,
				    GLuint texUnit,
				    const struct gl_texture_object *tObj );


extern void
_swrast_texture_fragments( GLcontext *ctx, GLuint texSet,
                           struct sw_span *span,
			   GLchan rgba[][4] );

extern void
_old_swrast_texture_fragments( GLcontext *ctx, GLuint texSet, GLuint n,
			       GLfloat texcoords[][4], GLfloat lambda[],
			       CONST GLchan primary_rgba[][4],
                               GLchan rgba[][4] );

extern void
_swrast_multitexture_fragments( GLcontext *ctx, struct sw_span *span );

#endif

/*
 * GLX Hardware Device Driver for Intel i810
 * Copyright (C) 1999 Keith Whitwell
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
 * KEITH WHITWELL, OR ANY OTHER CONTRIBUTORS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 */

#ifndef I810VB_INC
#define I810VB_INC

#include "main/mtypes.h"
#include "swrast/swrast.h"

#define _I810_NEW_VERTEX (_NEW_TEXTURE |			\
			  _DD_NEW_SEPARATE_SPECULAR |		\
			  _DD_NEW_TRI_UNFILLED |		\
			  _DD_NEW_TRI_LIGHT_TWOSIDE |		\
			  _NEW_FOG)


extern void i810ChooseVertexState( struct gl_context *ctx );
extern void i810CheckTexSizes( struct gl_context *ctx );
extern void i810BuildVertices( struct gl_context *ctx,
			       GLuint start,
			       GLuint count,
			       GLuint newinputs );


extern void *i810_emit_contiguous_verts( struct gl_context *ctx,
					 GLuint start,
					 GLuint count,
					 void *dest );

extern void i810_translate_vertex( struct gl_context *ctx,
				   const i810Vertex *src,
				   SWvertex *dst );

extern void i810InitVB( struct gl_context *ctx );
extern void i810FreeVB( struct gl_context *ctx );

#endif

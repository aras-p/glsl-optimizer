/* $Id: s_masking.h,v 1.4 2002/02/02 17:24:11 brianp Exp $ */

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


#ifndef S_MASKING_H
#define S_MASKING_H


#include "mtypes.h"
#include "swrast.h"


/*
 * Implement glColorMask for a span of RGBA pixels.
 */
extern void
_mesa_mask_rgba_span( GLcontext *ctx, const struct sw_span *span,
                      GLchan rgba[][4] );


extern void
_mesa_mask_rgba_array( GLcontext *ctx, GLuint n, GLint x, GLint y,
                       GLchan rgba[][4] );


/*
 * Implement glColorMask for an array of RGBA pixels.
 */
extern void
_mesa_mask_rgba_pixels( GLcontext *ctx,
                        GLuint n, const GLint x[], const GLint y[],
                        GLchan rgba[][4], const GLubyte mask[] );



/*
 * Implement glIndexMask for a span of CI pixels.
 */
extern void
_mesa_mask_index_span( GLcontext *ctx, const struct sw_span *span,
                       GLuint index[] );



/*
 * Implement glIndexMask for a span of CI pixels.
 */
extern void
_mesa_mask_index_array( GLcontext *ctx,
                        GLuint n, GLint x, GLint y, GLuint index[] );



/*
 * Implement glIndexMask for an array of CI pixels.
 */
extern void
_mesa_mask_index_pixels( GLcontext *ctx,
                         GLuint n, const GLint x[], const GLint y[],
                         GLuint index[], const GLubyte mask[] );



#endif

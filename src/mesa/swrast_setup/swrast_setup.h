/* $Id: swrast_setup.h,v 1.8 2001/03/12 00:48:43 gareth Exp $ */

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
 *
 * Authors:
 *    Keith Whitwell <keithw@valinux.com>
 */

/* Public interface to the swrast_setup module.  This module provides
 * an implementation of the driver interface to t_vb_render.c, and uses
 * the software rasterizer (swrast) to perform actual rasterization.
 */

#ifndef SWRAST_SETUP_H
#define SWRAST_SETUP_H

extern GLboolean
_swsetup_CreateContext( GLcontext *ctx );

extern void
_swsetup_DestroyContext( GLcontext *ctx );

extern void
_swsetup_InvalidateState( GLcontext *ctx, GLuint new_state );

extern void
_swsetup_BuildProjectedVertices( GLcontext *ctx,
				 GLuint start,
				 GLuint end,
				 GLuint new_inputs );

extern void
_swsetup_Quad( GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2, GLuint v3 );

extern void
_swsetup_Triangle( GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2 );

extern void
_swsetup_Line( GLcontext *ctx, GLuint v0, GLuint v1 );

extern void
_swsetup_Points( GLcontext *ctx, GLuint first, GLuint last );

extern void
_swsetup_RenderPrimitive( GLcontext *ctx, GLenum mode );

extern void
_swsetup_RenderStart( GLcontext *ctx );

extern void
_swsetup_RenderFinish( GLcontext *ctx );

extern void
_swsetup_RenderProjectInterpVerts( GLcontext *ctx );

extern void
_swsetup_RenderInterp( GLcontext *ctx, GLfloat t,
		       GLuint dst, GLuint out, GLuint in,
		       GLboolean force_boundary );
extern void
_swsetup_RenderCopyPV( GLcontext *ctx, GLuint dst, GLuint src );

extern void
_swsetup_RenderClippedPolygon( GLcontext *ctx, const GLuint *elts, GLuint n );

extern void
_swsetup_RenderClippedLine( GLcontext *ctx, GLuint ii, GLuint jj );



#endif

/*
 * Mesa 3-D graphics library
 * Version:  3.5
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
 *
 * Authors:
 *    Keith Whitwell <keithw@valinux.com>
 */

/* Public interface to the swrast_setup module.
 */

#ifndef SWRAST_SETUP_H
#define SWRAST_SETUP_H

extern GLboolean 
_swsetup_CreateContext( GLcontext *ctx );

extern void 
_swsetup_DestroyContext( GLcontext *ctx );

extern void
_swsetup_RegisterVB( struct vertex_buffer *VB );

extern void 
_swsetup_UnregisterVB( struct vertex_buffer *VB );

extern void 
_swsetup_InvalidateState( GLcontext *ctx, GLuint new_state );

extern void 
_swsetup_RasterSetup( struct vertex_buffer *VB, 
		     GLuint start, GLuint end );

extern void 
_swsetup_Quad( GLcontext *ctx, GLuint v0, GLuint v1, 
	       GLuint v2, GLuint v3, GLuint pv );

extern void 
_swsetup_Triangle( GLcontext *ctx, GLuint v0, GLuint v1, 
		   GLuint v2, GLuint pv );


extern void 
_swsetup_Line( GLcontext *ctx, GLuint v0, GLuint v1, GLuint pv );


extern void 
_swsetup_Points( GLcontext *ctx, GLuint first, GLuint last );


#endif

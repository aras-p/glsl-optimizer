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

#ifndef SWRAST_H
#define SWRAST_H

#include "mtypes.h"



/* The software rasterizer now uses this format for vertices.  Thus a
 * 'RasterSetup' stage or other translation is required between the
 * tnl module and the swrast rasterization functions.  This serves to
 * isolate the swrast module from the internals of the tnl module, and
 * improve its usefulness as a fallback mechanism for hardware
 * drivers.
 *
 * Full software drivers:
 *   - Register the rastersetup and triangle functions from
 *     utils/software_helper.
 *   - On statechange, update the rasterization pointers in that module.
 *
 * Rasterization hardware drivers:
 *   - Keep native rastersetup.
 *   - Implement native twoside,offset and unfilled triangle setup.
 *   - Implement a translator from native vertices to swrast vertices.
 *   - On partial fallback (mix of accelerated and unaccelerated
 *   prims), call a pass-through function which translates native
 *   vertices to SWvertices and calls the appropriate swrast function.
 *   - On total fallback (vertex format insufficient for state or all
 *     primitives unaccelerated), hook in swrast_setup instead.
 */
typedef struct {
   GLfloat win[4];
   GLfloat eye[4];		/* for GL_EXT_point_param only */
   GLfloat texcoord[MAX_TEXTURE_UNITS][4];
   GLchan color[4];
   GLchan specular[4];
   GLfloat fog;
   GLuint index;
} SWvertex;




/* These are the public-access functions exported from swrast.
 */
extern void
_swrast_alloc_buffers( GLcontext *ctx );

extern GLboolean
_swrast_CreateContext( GLcontext *ctx );

extern void
_swrast_DestroyContext( GLcontext *ctx );



extern void
_swrast_Bitmap( GLcontext *ctx,
		GLint px, GLint py,
		GLsizei width, GLsizei height,
		const struct gl_pixelstore_attrib *unpack,
		const GLubyte *bitmap );

extern void
_swrast_CopyPixels( GLcontext *ctx,
		    GLint srcx, GLint srcy,
		    GLint destx, GLint desty,
		    GLsizei width, GLsizei height,
		    GLenum type );

extern void
_swrast_DrawPixels( GLcontext *ctx,
		    GLint x, GLint y,
		    GLsizei width, GLsizei height,
		    GLenum format, GLenum type,
		    const struct gl_pixelstore_attrib *unpack,
		    const GLvoid *pixels );

extern void
_swrast_ReadPixels( GLcontext *ctx,
		    GLint x, GLint y, GLsizei width, GLsizei height,
		    GLenum format, GLenum type,
		    const struct gl_pixelstore_attrib *unpack,
		    GLvoid *pixels );

extern void
_swrast_Clear( GLcontext *ctx, GLbitfield mask, GLboolean all,
	       GLint x, GLint y, GLint width, GLint height );

extern void
_swrast_Accum( GLcontext *ctx, GLenum op,
	       GLfloat value, GLint xpos, GLint ypos,
	       GLint width, GLint height );


/* Get a pointer to the stipple counter.
 */
extern GLuint *
_swrast_get_stipple_counter_ref( GLcontext *ctx );


/* These will always render the correct point/line/triangle for the
 * current state.
 */
extern void
_swrast_Point( GLcontext *ctx, const SWvertex *v );

extern void
_swrast_Line( GLcontext *ctx, const SWvertex *v0, const SWvertex *v1 );

extern void
_swrast_Triangle( GLcontext *ctx, const SWvertex *v0,
                  const SWvertex *v1, const SWvertex *v2 );

extern void
_swrast_Quad( GLcontext *ctx,
              const SWvertex *v0, const SWvertex *v1,
	      const SWvertex *v2,  const SWvertex *v3);

extern void
_swrast_flush( GLcontext *ctx );


/* Tell the software rasterizer about core state changes.
 */
extern void
_swrast_InvalidateState( GLcontext *ctx, GLuint new_state );

/* Configure software rasterizer to match hardware rasterizer characteristics:
 */
extern void
_swrast_allow_vertex_fog( GLcontext *ctx, GLboolean value );

extern void
_swrast_allow_pixel_fog( GLcontext *ctx, GLboolean value );

#endif

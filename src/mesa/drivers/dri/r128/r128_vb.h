/* $XFree86: xc/lib/GL/mesa/src/drv/r128/r128_vb.h,v 1.8 2002/10/30 12:51:46 alanh Exp $ */
/**************************************************************************

Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     VA Linux Systems Inc., Fremont, California.

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ATI, VA LINUX SYSTEMS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 *
 */

#ifndef R128VB_INC
#define R128VB_INC

#include "mtypes.h"
#include "swrast/swrast.h"
#include "r128_context.h"

#define _R128_NEW_VERTEX_STATE (_DD_NEW_SEPARATE_SPECULAR |		\
				_DD_NEW_TRI_LIGHT_TWOSIDE |		\
				_DD_NEW_TRI_UNFILLED |			\
				_NEW_TEXTURE |				\
				_NEW_FOG)

extern void r128CheckTexSizes( GLcontext *ctx );
extern void r128ChooseVertexState( GLcontext *ctx );

extern void r128BuildVertices( GLcontext *ctx, GLuint start, GLuint count,
				 GLuint newinputs );

extern void r128PrintSetupFlags(char *msg, GLuint flags );

extern void r128InitVB( GLcontext *ctx );
extern void r128FreeVB( GLcontext *ctx );

extern void r128_emit_contiguous_verts( GLcontext *ctx,
					  GLuint start,
					  GLuint count );

extern void r128_emit_indexed_verts( GLcontext *ctx,
				       GLuint start,
				       GLuint count );

extern void r128_translate_vertex( GLcontext *ctx,
				     const r128Vertex *src,
				     SWvertex *dst );

extern void r128_print_vertex( GLcontext *ctx, const r128Vertex *v );

#endif

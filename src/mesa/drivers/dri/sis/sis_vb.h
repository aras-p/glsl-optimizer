/* $XFree86: xc/lib/GL/mesa/src/drv/r128/r128_vb.h,v 1.8 2002/10/30 12:51:46 alanh Exp $ */
/**************************************************************************

Copyright 2003 Eric Anholt
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
ERIC ANHOLT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *    Eric Anholt <anholt@FreeBSD.org>
 */

#ifndef __SIS_VB_H__
#define __SIS_VB_H__

#include "mtypes.h"
#include "swrast/swrast.h"
#include "sis_context.h"

#define _SIS_NEW_VERTEX_STATE (_DD_NEW_SEPARATE_SPECULAR |		\
				_DD_NEW_TRI_LIGHT_TWOSIDE |		\
				_DD_NEW_TRI_UNFILLED |			\
				_NEW_TEXTURE |				\
				_NEW_FOG)

extern void sisCheckTexSizes( GLcontext *ctx );
extern void sisChooseVertexState( GLcontext *ctx );

extern void sisBuildVertices( GLcontext *ctx, GLuint start, GLuint count,
			      GLuint newinputs );

extern void sisPrintSetupFlags( char *msg, GLuint flags );

extern void sisInitVB( GLcontext *ctx );
extern void sisFreeVB( GLcontext *ctx );

extern void sis_translate_vertex( GLcontext *ctx,
				  const sisVertex *src,
				  SWvertex *dst );

extern void sis_print_vertex( GLcontext *ctx, const sisVertex *v );

#endif

/* $XFree86: xc/lib/GL/mesa/src/drv/mga/mgavb.h,v 1.6 2001/04/10 16:07:51 dawes Exp $ */
/*
 * Copyright 2000-2001 VA Linux Systems, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Keith Whitwell <keithw@valinux.com>
 *    Felix Kuehling <fxkuehl@gmx.de>
 */

#ifndef SAVAGEVB_INC
#define SAVAGEVB_INC

#include "mtypes.h"
#include "savagecontext.h"
#include "swrast/swrast.h"

#define _SAVAGE_NEW_VERTEX_STATE (_NEW_TEXTURE |		\
			          _DD_NEW_SEPARATE_SPECULAR |	\
			          _DD_NEW_TRI_UNFILLED |	\
			          _DD_NEW_TRI_LIGHT_TWOSIDE |	\
			          _NEW_FOG)


extern void savageChooseVertexState( GLcontext *ctx );
extern void savageCheckTexSizes( GLcontext *ctx );
extern void savageBuildVertices( GLcontext *ctx, 
				 GLuint start, 
				 GLuint count,
				 GLuint newinputs );

extern void savagePrintSetupFlags(char *msg, GLuint flags );

extern void savageInitVB( GLcontext *ctx );
extern void savageFreeVB( GLcontext *ctx );

extern void savage_translate_vertex(GLcontext *ctx, 
				    const savageVertex *src, 
				    SWvertex *dst);

extern void savage_print_vertex( GLcontext *ctx, const savageVertex *v );

#endif

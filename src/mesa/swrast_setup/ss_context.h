/* $Id: ss_context.h,v 1.7 2001/03/12 00:48:43 gareth Exp $ */

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

#ifndef SS_CONTEXT_H
#define SS_CONTEXT_H

#include "mtypes.h"
#include "swrast/swrast.h"
#include "swrast_setup.h"

typedef struct {
   GLuint NewState;
   GLuint StateChanges;

   /* Function hooks, trigger lazy state updates.
    */
   void (*InvalidateState)( GLcontext *ctx, GLuint new_state );

   void (*BuildProjVerts)( GLcontext *ctx,
			   GLuint start, GLuint end, GLuint new_inputs );

   void (*Quad)( GLcontext *ctx, GLuint v0, GLuint v1,
		 GLuint v2, GLuint v3 );

   void (*Triangle)( GLcontext *ctx, GLuint v0, GLuint v1,
		     GLuint v2 );

   void (*Line)( GLcontext *ctx, GLuint v0, GLuint v1 );

   void (*Points)( GLcontext *ctx, GLuint first, GLuint last );

   void (*RenderCopyPV)( GLcontext *ctx, GLuint dst, GLuint src );

   void (*RenderInterp)( GLcontext *ctx, GLfloat t,
			 GLuint dst, GLuint out, GLuint in,
			 GLboolean force_boundary );


   SWvertex *verts;
   GLenum render_prim;

} SScontext;

#define SWSETUP_CONTEXT(ctx) ((SScontext *)ctx->swsetup_context)

#endif

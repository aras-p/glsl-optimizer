/* $Id: ss_context.c,v 1.13 2001/03/12 00:48:43 gareth Exp $ */

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

#include "glheader.h"
#include "mem.h"
#include "ss_context.h"
#include "ss_triangle.h"
#include "ss_vb.h"
#include "ss_interp.h"
#include "swrast_setup.h"
#include "tnl/t_context.h"


#define _SWSETUP_NEW_VERTS (_NEW_RENDERMODE|	\
                            _NEW_LIGHT|         \
			    _NEW_TEXTURE|	\
			    _NEW_COLOR|		\
			    _NEW_FOG|		\
			    _NEW_POINT)

#define _SWSETUP_NEW_RENDERINDEX (_NEW_POLYGON|_NEW_LIGHT)


/* Dispatch from these fixed entrypoints to the state-dependent
 * functions.
 *
 * The design of swsetup suggests that we could really program
 * ctx->Driver.TriangleFunc directly from _swsetup_RenderStart, and
 * avoid this second level of indirection.  However, this is more
 * convient for fallback cases in hardware rasterization drivers.
 */
void
_swsetup_Quad( GLcontext *ctx, GLuint v0, GLuint v1,
	       GLuint v2, GLuint v3 )
{
   SWSETUP_CONTEXT(ctx)->Quad( ctx, v0, v1, v2, v3 );
}

void
_swsetup_Triangle( GLcontext *ctx, GLuint v0, GLuint v1,
		   GLuint v2 )
{
   SWSETUP_CONTEXT(ctx)->Triangle( ctx, v0, v1, v2 );
}

void
_swsetup_Line( GLcontext *ctx, GLuint v0, GLuint v1 )
{
   SWSETUP_CONTEXT(ctx)->Line( ctx, v0, v1 );
}

void
_swsetup_Points( GLcontext *ctx, GLuint first, GLuint last )
{
   SWSETUP_CONTEXT(ctx)->Points( ctx, first, last );
}

void
_swsetup_BuildProjectedVertices( GLcontext *ctx, GLuint start, GLuint end,
				 GLuint new_inputs )
{
   SWSETUP_CONTEXT(ctx)->BuildProjVerts( ctx, start, end, new_inputs );
}


GLboolean
_swsetup_CreateContext( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   SScontext *swsetup = (SScontext *)CALLOC(sizeof(SScontext));

   if (!swsetup)
      return GL_FALSE;

   swsetup->verts = (SWvertex *) ALIGN_MALLOC( sizeof(SWvertex) * tnl->vb.Size, 32);
   if (!swsetup->verts) {
      FREE(swsetup);
      return GL_FALSE;
   }

   ctx->swsetup_context = swsetup;

   swsetup->NewState = ~0;
   _swsetup_vb_init( ctx );
   _swsetup_interp_init( ctx );
   _swsetup_trifuncs_init( ctx );

   return GL_TRUE;
}

void
_swsetup_DestroyContext( GLcontext *ctx )
{
   if (SWSETUP_CONTEXT(ctx)) {
      if (SWSETUP_CONTEXT(ctx)->verts)
	 ALIGN_FREE(SWSETUP_CONTEXT(ctx)->verts);

      FREE(SWSETUP_CONTEXT(ctx));
      ctx->swsetup_context = 0;
   }
}

void
_swsetup_RenderPrimitive( GLcontext *ctx, GLenum mode )
{
   SWSETUP_CONTEXT(ctx)->render_prim = mode;
}

void
_swsetup_RenderStart( GLcontext *ctx )
{
   SScontext *swsetup = SWSETUP_CONTEXT(ctx);
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   GLuint new_state = swsetup->NewState;

   if (new_state & _SWSETUP_NEW_RENDERINDEX) {
      _swsetup_choose_trifuncs( ctx );
   }

   if (new_state & _SWSETUP_NEW_VERTS) {
      _swsetup_choose_rastersetup_func( ctx );
   }

   swsetup->NewState = 0;

   if (VB->ClipMask && VB->importable_data)
      VB->import_data( ctx,
		       VB->importable_data,
		       VEC_NOT_WRITEABLE|VEC_BAD_STRIDE);
}

void
_swsetup_RenderFinish( GLcontext *ctx )
{
   _swrast_flush( ctx );
}

void
_swsetup_InvalidateState( GLcontext *ctx, GLuint new_state )
{
   SScontext *swsetup = SWSETUP_CONTEXT(ctx);
   swsetup->NewState |= new_state;

   if (new_state & _SWSETUP_NEW_INTERP) {
      swsetup->RenderInterp = _swsetup_validate_interp;
      swsetup->RenderCopyPV = _swsetup_validate_copypv;
   }
}

void
_swsetup_RenderInterp( GLcontext *ctx, GLfloat t,
		       GLuint dst, GLuint out, GLuint in,
		       GLboolean force_boundary )
{
   SWSETUP_CONTEXT(ctx)->RenderInterp( ctx, t, dst, out, in, force_boundary );
}

void
_swsetup_RenderCopyPV( GLcontext *ctx, GLuint dst, GLuint src )
{
   SWSETUP_CONTEXT(ctx)->RenderCopyPV( ctx, dst, src );
}

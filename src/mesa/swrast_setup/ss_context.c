/* $Id: ss_context.c,v 1.1 2000/11/05 18:20:18 keithw Exp $ */

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


#include "glheader.h"
#include "mem.h"


#include "ss_context.h"
#include "ss_triangle.h"
#include "ss_vb.h"

#include "swrast_setup.h"


/* Stub for swsetup->Triangle to select a true triangle function 
 * after a state change.
 */
static void 
_swsetup_validate_quad( GLcontext *ctx, GLuint v0, GLuint v1, 
			GLuint v2, GLuint v3, GLuint pv )
{
   _swsetup_choose_trifuncs( ctx );
   SWSETUP_CONTEXT(ctx)->Quad( ctx, v0, v1, v2, v3, pv );
}

static void 
_swsetup_validate_triangle( GLcontext *ctx, GLuint v0, GLuint v1, 
			    GLuint v2, GLuint pv )
{
   _swsetup_choose_trifuncs( ctx );
   SWSETUP_CONTEXT(ctx)->Triangle( ctx, v0, v1, v2, pv );
}

static void 
_swsetup_validate_line( GLcontext *ctx, GLuint v0, GLuint v1, GLuint pv )
{
   _swsetup_choose_trifuncs( ctx );
   SWSETUP_CONTEXT(ctx)->Line( ctx, v0, v1, pv );
}


static void 
_swsetup_validate_points( GLcontext *ctx, GLuint first, GLuint last )
{
   _swsetup_choose_trifuncs( ctx );
   SWSETUP_CONTEXT(ctx)->Points( ctx, first, last );
}



static void 
_swsetup_validate_rastersetup( struct vertex_buffer *VB,
			       GLuint start, GLuint end )
{
   GLcontext *ctx = VB->ctx;
   _swsetup_choose_rastersetup_func( ctx );
   SWSETUP_CONTEXT(ctx)->RasterSetup( VB, start, end );
}


#define _SWSETUP_NEW_RASTERSETUP (_NEW_RENDERMODE|	\
			          _NEW_TEXTURE|		\
			          _NEW_COLOR|		\
			          _NEW_FOG|		\
			          _NEW_POINT)

#define _SWSETUP_NEW_RENDERINDEX (_NEW_POLYGON|_NEW_LIGHT)


#if 0
/* TODO: sleep/wakeup mechanism
 */
static void
_swsetup_sleep( GLcontext *ctx, GLuint new_state )
{
}
#endif

static void
_swsetup_invalidate_state( GLcontext *ctx, GLuint new_state )
{
   SScontext *swsetup = SWSETUP_CONTEXT(ctx);

   swsetup->NewState |= new_state;

   
   if (new_state & _SWSETUP_NEW_RENDERINDEX) {
      swsetup->Triangle = _swsetup_validate_triangle;
      swsetup->Line = _swsetup_validate_line;
      swsetup->Points = _swsetup_validate_points;
      swsetup->Quad = _swsetup_validate_quad;
   }

   if (new_state & _SWSETUP_NEW_RASTERSETUP) {
      swsetup->RasterSetup = _swsetup_validate_rastersetup;
   }
}



/* Dispatch from these fixed entrypoints to the state-dependent
 * functions:
 */
void 
_swsetup_Quad( GLcontext *ctx, GLuint v0, GLuint v1, 
	       GLuint v2, GLuint v3, GLuint pv )
{
   SWSETUP_CONTEXT(ctx)->Quad( ctx, v0, v1, v2, v3, pv );
}

void 
_swsetup_Triangle( GLcontext *ctx, GLuint v0, GLuint v1, 
		   GLuint v2, GLuint pv )
{
   SWSETUP_CONTEXT(ctx)->Triangle( ctx, v0, v1, v2, pv );
}

void 
_swsetup_Line( GLcontext *ctx, GLuint v0, GLuint v1, GLuint pv )
{
   SWSETUP_CONTEXT(ctx)->Line( ctx, v0, v1, pv );
}


void 
_swsetup_Points( GLcontext *ctx, GLuint first, GLuint last )
{
   SWSETUP_CONTEXT(ctx)->Points( ctx, first, last );
}

void 
_swsetup_RasterSetup( struct vertex_buffer *VB, GLuint start, GLuint end )
{
   SWSETUP_CONTEXT(VB->ctx)->RasterSetup( VB, start, end );
}

void
_swsetup_InvalidateState( GLcontext *ctx, GLuint new_state )
{
   SWSETUP_CONTEXT(ctx)->InvalidateState( ctx, new_state );
}


GLboolean
_swsetup_CreateContext( GLcontext *ctx )
{
   SScontext *swsetup = (SScontext *)CALLOC(sizeof(SScontext));

   if (!swsetup) 
      return GL_FALSE;

   ctx->swsetup_context = swsetup;

   swsetup->NewState = ~0;
   swsetup->InvalidateState = _swsetup_invalidate_state;
   swsetup->Quad = _swsetup_validate_quad;
   swsetup->Triangle = _swsetup_validate_triangle;
   swsetup->Line = _swsetup_validate_line;
   swsetup->Points = _swsetup_validate_points;
   swsetup->RasterSetup = _swsetup_validate_rastersetup;
   
   _swsetup_vb_init( ctx );
   _swsetup_trifuncs_init( ctx );
   
   return GL_TRUE;
}

void
_swsetup_DestroyContext( GLcontext *ctx )
{
   if (SWSETUP_CONTEXT(ctx)) {
      FREE(SWSETUP_CONTEXT(ctx));
      ctx->swsetup_context = 0;
   }
}

GLboolean 
_swsetup_RegisterVB( struct vertex_buffer *VB )
{
   SSvertexbuffer *ssvb = (SSvertexbuffer *)CALLOC(sizeof(SSvertexbuffer) );

   ssvb->verts = ALIGN_MALLOC( sizeof(SWvertex) * VB->Size, 32);
   if (!ssvb->verts) {
      FREE(ssvb);
      return GL_FALSE;
   }

   VB->swsetup_vb = ssvb;
   return GL_TRUE;
}


void 
_swsetup_UnregisterVB( struct vertex_buffer *VB )
{
   SSvertexbuffer *ssvb = SWSETUP_VB(VB);
   
   if (ssvb) {
      if (ssvb->verts) ALIGN_FREE(ssvb->verts);
      FREE(ssvb);
      VB->swsetup_vb = 0;
   }      
}

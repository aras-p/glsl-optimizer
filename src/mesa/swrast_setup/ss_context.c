
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
#include "swrast_setup.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"


#define _SWSETUP_NEW_VERTS (_NEW_RENDERMODE|	\
                            _NEW_POLYGON|       \
                            _NEW_LIGHT|         \
			    _NEW_TEXTURE|	\
			    _NEW_COLOR|		\
			    _NEW_FOG|		\
			    _NEW_POINT)

#define _SWSETUP_NEW_RENDERINDEX (_NEW_POLYGON|_NEW_LIGHT)


GLboolean
_swsetup_CreateContext( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   SScontext *swsetup = (SScontext *)CALLOC(sizeof(SScontext));

   if (!swsetup)
      return GL_FALSE;

   swsetup->verts = (SWvertex *) ALIGN_CALLOC( sizeof(SWvertex) * tnl->vb.Size,
					       32);
   if (!swsetup->verts) {
      FREE(swsetup);
      return GL_FALSE;
   }

   ctx->swsetup_context = swsetup;

   swsetup->NewState = ~0;
   _swsetup_vb_init( ctx );
   _swsetup_trifuncs_init( ctx );

   return GL_TRUE;
}

void
_swsetup_DestroyContext( GLcontext *ctx )
{
   SScontext *swsetup = SWSETUP_CONTEXT(ctx);

   if (swsetup) {
      if (swsetup->verts)
	 ALIGN_FREE(swsetup->verts);

      if (swsetup->ChanSecondaryColor.Ptr) 
	 ALIGN_FREE(swsetup->ChanSecondaryColor.Ptr);

      if (swsetup->ChanColor.Ptr) 
	 ALIGN_FREE(swsetup->ChanColor.Ptr);

      FREE(swsetup);
      ctx->swsetup_context = 0;
   }
}

static void
_swsetup_RenderPrimitive( GLcontext *ctx, GLenum mode )
{
   SWSETUP_CONTEXT(ctx)->render_prim = mode;
}

static void
_swsetup_RenderStart( GLcontext *ctx )
{
   SScontext *swsetup = SWSETUP_CONTEXT(ctx);
   GLuint new_state = swsetup->NewState;

   if (new_state & _SWSETUP_NEW_RENDERINDEX) {
      _swsetup_choose_trifuncs( ctx );
   }

   if (new_state & _SWSETUP_NEW_VERTS) {
      _swsetup_choose_rastersetup_func( ctx );
   }

   swsetup->NewState = 0;

   _swrast_render_start( ctx );
}

static void
_swsetup_RenderFinish( GLcontext *ctx )
{
   _swrast_render_finish( ctx );
}

void
_swsetup_InvalidateState( GLcontext *ctx, GLuint new_state )
{
   SScontext *swsetup = SWSETUP_CONTEXT(ctx);
   swsetup->NewState |= new_state;
}


void
_swsetup_Wakeup( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   tnl->Driver.Render.Start = _swsetup_RenderStart;
   tnl->Driver.Render.Finish = _swsetup_RenderFinish;
   tnl->Driver.Render.PrimitiveNotify = _swsetup_RenderPrimitive;
   /* interp */
   /* copypv */
   tnl->Driver.Render.ClippedPolygon = _tnl_RenderClippedPolygon; /* new */
   tnl->Driver.Render.ClippedLine = _tnl_RenderClippedLine; /* new */
   /* points */
   /* line */
   /* triangle */
   /* quad */
   tnl->Driver.Render.PrimTabVerts = _tnl_render_tab_verts;
   tnl->Driver.Render.PrimTabElts = _tnl_render_tab_elts;
   tnl->Driver.Render.ResetLineStipple = _swrast_ResetLineStipple;
   /* buildvertices */
   tnl->Driver.Render.Multipass = 0;
   _tnl_need_projected_coords( ctx, GL_TRUE );
   _swsetup_InvalidateState( ctx, ~0 );
}

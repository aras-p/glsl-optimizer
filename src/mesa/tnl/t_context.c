/* $Id: t_context.c,v 1.19 2001/06/04 16:09:28 keithw Exp $ */

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
#include "macros.h"
#include "mtypes.h"
#include "mem.h"
#include "dlist.h"
#include "vtxfmt.h"

#include "t_context.h"
#include "t_array_api.h"
#include "t_eval_api.h"
#include "t_imm_alloc.h"
#include "t_imm_api.h"
#include "t_imm_exec.h"
#include "t_imm_dlist.h"
#include "t_pipeline.h"
#include "tnl.h"

#ifndef THREADS
struct immediate *_tnl_CurrentInput = NULL;
#endif


void
_tnl_MakeCurrent( GLcontext *ctx,
		  GLframebuffer *drawBuffer,
		  GLframebuffer *readBuffer )
{
#ifndef THREADS
   SET_IMMEDIATE( ctx, TNL_CURRENT_IM(ctx) );
#endif
}


static void
install_driver_callbacks( GLcontext *ctx )
{
   ctx->Driver.NewList = _tnl_NewList;
   ctx->Driver.EndList = _tnl_EndList;
   ctx->Driver.FlushVertices = _tnl_flush_vertices;
   ctx->Driver.MakeCurrent = _tnl_MakeCurrent;
   ctx->Driver.BeginCallList = _tnl_BeginCallList;
   ctx->Driver.EndCallList = _tnl_EndCallList;
}



GLboolean
_tnl_CreateContext( GLcontext *ctx )
{
   TNLcontext *tnl;

   /* Create the TNLcontext structure
    */
   ctx->swtnl_context = tnl = (TNLcontext *) CALLOC( sizeof(TNLcontext) );

   if (!tnl) {
      return GL_FALSE;
   }

   /* Initialize the VB.
    */
   tnl->vb.Size = MAX2( IMM_SIZE,
			ctx->Const.MaxArrayLockSize + MAX_CLIPPED_VERTICES);


   /* Initialize tnl state and tnl->vtxfmt.
    */
   _tnl_dlist_init( ctx );
   _tnl_array_init( ctx );
   _tnl_imm_init( ctx );
   _tnl_eval_init( ctx );
   _tnl_install_pipeline( ctx, _tnl_default_pipeline );


   tnl->NeedProjCoords = GL_TRUE;
   tnl->LoopbackDListCassettes = GL_FALSE;

   /* Hook our functions into exec and compile dispatch tables.
    */
   _mesa_install_exec_vtxfmt( ctx, &tnl->vtxfmt );
   _mesa_install_save_vtxfmt( ctx, &tnl->vtxfmt );
   ctx->Save->CallList = _mesa_save_CallList;	
   ctx->Save->CallLists = _mesa_save_CallLists;
   ctx->Save->EvalMesh1 = _mesa_save_EvalMesh1;	
   ctx->Save->EvalMesh2 = _mesa_save_EvalMesh2;
   ctx->Save->Begin = _tnl_save_Begin;

   /* Set a few default values in the driver struct.
    */
   install_driver_callbacks(ctx);
   ctx->Driver.NeedFlush = FLUSH_UPDATE_CURRENT;
   ctx->Driver.CurrentExecPrimitive = PRIM_OUTSIDE_BEGIN_END;
   ctx->Driver.CurrentSavePrimitive = PRIM_UNKNOWN;

   tnl->Driver.RenderTabElts = _tnl_render_tab_elts;
   tnl->Driver.RenderTabVerts = _tnl_render_tab_verts;

   
   return GL_TRUE;
}


void
_tnl_DestroyContext( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   _tnl_array_destroy( ctx );
   _tnl_imm_destroy( ctx );
   _tnl_destroy_pipeline( ctx );

   FREE(tnl);
   ctx->swtnl_context = 0;
}


void
_tnl_InvalidateState( GLcontext *ctx, GLuint new_state )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   if (new_state & _NEW_ARRAY) {
      struct immediate *IM = TNL_CURRENT_IM(ctx);
      IM->ArrayEltFlags = ~ctx->Array._Enabled;
      IM->ArrayEltFlush = (ctx->Array.LockCount 
			   ? FLUSH_ELT_LAZY : FLUSH_ELT_EAGER);
      IM->ArrayEltIncr = ctx->Array.Vertex.Enabled ? 1 : 0;
      tnl->pipeline.run_input_changes |= ctx->Array.NewState; /* overkill */
   }

   tnl->pipeline.run_state_changes |= new_state;
   tnl->pipeline.build_state_changes |= (new_state &
					 tnl->pipeline.build_state_trigger);

   tnl->eval.EvalNewState |= new_state;
}


void
_tnl_wakeup_exec( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   install_driver_callbacks(ctx);
   ctx->Driver.NeedFlush |= FLUSH_UPDATE_CURRENT;

   /* Hook our functions into exec and compile dispatch tables.
    */
   _mesa_install_exec_vtxfmt( ctx, &tnl->vtxfmt );

   /* Call all appropriate driver callbacks to revive state.
    */
   _tnl_MakeCurrent( ctx, ctx->DrawBuffer, ctx->ReadBuffer );

   /* Assume we haven't been getting state updates either:
    */
   _tnl_InvalidateState( ctx, ~0 );
   tnl->pipeline.run_input_changes = ~0;
}


void
_tnl_wakeup_save_exec( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   _tnl_wakeup_exec( ctx );
   _mesa_install_save_vtxfmt( ctx, &tnl->vtxfmt );
   ctx->Save->CallList = _mesa_save_CallList;	/* fixme */
   ctx->Save->CallLists = _mesa_save_CallLists;
   ctx->Save->EvalMesh1 = _mesa_save_EvalMesh1;	/* fixme */
   ctx->Save->EvalMesh2 = _mesa_save_EvalMesh2;
   ctx->Save->Begin = _tnl_save_Begin;
}


void
_tnl_need_projected_coords( GLcontext *ctx, GLboolean mode )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   if (tnl->NeedProjCoords != mode) {
      tnl->NeedProjCoords = mode;
      _tnl_InvalidateState( ctx, _NEW_PROJECTION );
   }
}

void
_tnl_need_dlist_loopback( GLcontext *ctx, GLboolean mode )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   if (tnl->LoopbackDListCassettes != mode) {
      tnl->LoopbackDListCassettes = mode;
   }
}

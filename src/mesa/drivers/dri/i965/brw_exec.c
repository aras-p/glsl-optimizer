/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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
 *    Keith Whitwell <keith@tungstengraphics.com>
 */


#include "api_arrayelt.h"
#include "glheader.h"
#include "imports.h"
#include "context.h"
#include "macros.h"
#include "mtypes.h"
#include "dlist.h"
#include "vtxfmt.h"

#include "brw_exec.h"


void brw_exec_init( GLcontext *ctx )
{
   struct brw_exec_context *exec = CALLOC_STRUCT(brw_exec_context);

   if (ctx->swtnl_im == NULL) {
      ctx->swtnl_im = CALLOC_STRUCT(brw_exec_save);
   }

   exec->ctx = ctx;
   IMM_CONTEXT(ctx)->exec = exec;

   /* Initialize the arrayelt helper
    */
   if (!ctx->aelt_context &&
       !_ae_create_context( ctx )) 
      return;

   brw_exec_vtx_init( exec );
   brw_exec_array_init( exec );

   ctx->Driver.NeedFlush = 0;
   ctx->Driver.CurrentExecPrimitive = PRIM_OUTSIDE_BEGIN_END;
   ctx->Driver.FlushVertices = brw_exec_FlushVertices;

   exec->eval.recalculate_maps = 1;
}


void brw_exec_destroy( GLcontext *ctx )
{
   struct brw_exec_context *exec = IMM_CONTEXT(ctx)->exec;

   if (ctx->aelt_context) {
      _ae_destroy_context( ctx );
      ctx->aelt_context = NULL;
   }

   brw_exec_vtx_destroy( exec );
   brw_exec_array_destroy( exec );

   if (exec) {
      FREE(exec);
      IMM_CONTEXT(ctx)->exec = NULL;
   }

   if (IMM_CONTEXT(ctx)->exec == NULL &&
       IMM_CONTEXT(ctx)->save == NULL) {
      FREE(IMM_CONTEXT(ctx));
      ctx->swtnl_im = NULL;
   }
}

/* Really want to install these callbacks to a central facility to be
 * invoked according to the state flags.  That will have to wait for a
 * mesa rework:
 */ 
void brw_exec_invalidate_state( GLcontext *ctx, GLuint new_state )
{
   struct brw_exec_context *exec = IMM_CONTEXT(ctx)->exec;

   if (new_state & (_NEW_PROGRAM|_NEW_EVAL))
      exec->eval.recalculate_maps = 1;

   _ae_invalidate_state(ctx, new_state);
}


void brw_exec_wakeup( GLcontext *ctx )
{
   struct brw_exec_context *exec = IMM_CONTEXT(ctx)->exec;

   ctx->Driver.FlushVertices = brw_exec_FlushVertices;
   ctx->Driver.NeedFlush |= FLUSH_UPDATE_CURRENT;

   /* Hook our functions into exec and compile dispatch tables.
    */
   _mesa_install_exec_vtxfmt( ctx, &exec->vtxfmt );

   /* Assume we haven't been getting state updates either:
    */
   brw_exec_invalidate_state( ctx, ~0 );
}




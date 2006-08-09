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


#include "mtypes.h"
#include "api_arrayelt.h"
#include "dlist.h"
#include "vtxfmt.h"
#include "imports.h"

#include "brw_save.h"



void brw_save_init( GLcontext *ctx )
{
   struct brw_save_context *save = CALLOC_STRUCT(brw_save_context);

   if (ctx->swtnl_im == NULL) {
      ctx->swtnl_im = CALLOC_STRUCT(brw_exec_save);
   }

   save->ctx = ctx;
   IMM_CONTEXT(ctx)->save = save;

   /* Initialize the arrayelt helper
    */
   if (!ctx->aelt_context &&
       !_ae_create_context( ctx )) 
      return;

   brw_save_api_init( save );
   brw_save_wakeup(ctx);

   ctx->Driver.CurrentSavePrimitive = PRIM_UNKNOWN;
}


void brw_save_destroy( GLcontext *ctx )
{
   struct brw_save_context *save = IMM_CONTEXT(ctx)->save;
   if (save) {
      FREE(save);
      IMM_CONTEXT(ctx)->save = NULL;
   }

   if (ctx->aelt_context) {
      _ae_destroy_context( ctx );
      ctx->aelt_context = NULL;
   }

   if (IMM_CONTEXT(ctx)->exec == NULL &&
       IMM_CONTEXT(ctx)->save == NULL) {
      FREE(IMM_CONTEXT(ctx));
      ctx->swtnl_im = NULL;
   }
}


void brw_save_invalidate_state( GLcontext *ctx, GLuint new_state )
{
   _ae_invalidate_state(ctx, new_state);
}


/* Note that this can occur during the playback of a display list:
 */
void brw_save_fallback( GLcontext *ctx, GLboolean fallback )
{
   struct brw_save_context *save = IMM_CONTEXT(ctx)->save;

   if (fallback)
      save->replay_flags |= BRW_SAVE_FALLBACK;
   else
      save->replay_flags &= ~BRW_SAVE_FALLBACK;
}


/* I don't see any reason to swap this code out on fallbacks.  It
 * wouldn't really mean anything to do so anyway as the old lists are
 * still around from pre-fallback.  Instead, the above code ensures
 * that vertices are routed back through immediate mode dispatch on
 * fallback.
 *
 * The below can be moved into init or removed:
 */
void brw_save_wakeup( GLcontext *ctx )
{
   ctx->Driver.NewList = brw_save_NewList;
   ctx->Driver.EndList = brw_save_EndList;
   ctx->Driver.SaveFlushVertices = brw_save_SaveFlushVertices;
   ctx->Driver.BeginCallList = brw_save_BeginCallList;
   ctx->Driver.EndCallList = brw_save_EndCallList;
   ctx->Driver.NotifySaveBegin = brw_save_NotifyBegin;

   /* Assume we haven't been getting state updates either:
    */
   brw_save_invalidate_state( ctx, ~0 );
}




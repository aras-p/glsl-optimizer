
#include "brw_context.h"
#include "brw_batchbuffer.h"

#include "util/u_upload_mgr.h"




/* All batchbuffer flushes must go through this function.
 */
void brw_context_flush( struct brw_context *brw )
{
   /*
    * 
    */
   brw_emit_query_end(brw);

   /* Move to the end of the current upload buffer so that we'll force choosing
    * a new buffer next time.
    */
   u_upload_flush( brw->vb.upload_vertex );
   u_upload_flush( brw->vb.upload_index );

   _brw_batchbuffer_flush( brw->batch, __FILE__, __LINE__ );

   /* Mark all context state as needing to be re-emitted.
    * This is probably not as severe as on 915, since almost all of our state
    * is just in referenced buffers.
    */
   brw->state.dirty.brw |= BRW_NEW_CONTEXT;
   brw->state.dirty.mesa |= ~0;
   brw->state.dirty.brw |= ~0;
   brw->state.dirty.cache |= ~0;

   brw->curbe.need_new_bo = GL_TRUE;
}

static void
brw_flush( struct pipe_context *pipe,
           struct pipe_fence_handle **fence )
{
   brw_context_flush( brw_context( pipe ) );
   if (fence)
      *fence = NULL;
}

void brw_pipe_flush_init( struct brw_context *brw )
{
   brw->base.flush = brw_flush;
}


void brw_pipe_flush_cleanup( struct brw_context *brw )
{
}

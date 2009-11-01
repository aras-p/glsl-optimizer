
#include "util/u_upload_mgr.h"

#include "brw_context.h"


/**
 * called from brw_batchbuffer_flush and children before sending a
 * batchbuffer off.
 */
static void brw_finish_batch(struct brw_context *brw)
{
   brw_emit_query_end(brw);
}


/**
 * called from intelFlushBatchLocked
 */
static void brw_new_batch( struct brw_context *brw )
{
   brw->curbe.need_new_bo = GL_TRUE;

   /* Mark all context state as needing to be re-emitted.
    * This is probably not as severe as on 915, since almost all of our state
    * is just in referenced buffers.
    */
   brw->state.dirty.brw |= BRW_NEW_CONTEXT;

   brw->state.dirty.mesa |= ~0;
   brw->state.dirty.brw |= ~0;
   brw->state.dirty.cache |= ~0;

   /* Move to the end of the current upload buffer so that we'll force choosing
    * a new buffer next time.
    */
   u_upload_flush( brw->vb.upload_vertex );
   u_upload_flush( brw->vb.upload_index );

}

/* called from intelWaitForIdle() and intelFlush()
 *
 * For now, just flush everything.  Could be smarter later.
 */
static GLuint brw_flush_cmd( void )
{
   return ((MI_FLUSH << 16) | BRW_FLUSH_STATE_CACHE);
}



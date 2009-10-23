
/**
 * called from intel_batchbuffer_flush and children before sending a
 * batchbuffer off.
 */
static void brw_finish_batch(struct intel_context *intel)
{
   struct brw_context *brw = brw_context(&intel->ctx);
   brw_emit_query_end(brw);
}


/**
 * called from intelFlushBatchLocked
 */
static void brw_new_batch( struct intel_context *intel )
{
   struct brw_context *brw = brw_context(&intel->ctx);

   /* Check that we didn't just wrap our batchbuffer at a bad time. */
   assert(!brw->no_batch_wrap);

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
   if (brw->vb.upload.bo != NULL) {
      dri_bo_unreference(brw->vb.upload.bo);
      brw->vb.upload.bo = NULL;
      brw->vb.upload.offset = 0;
   }
}


static void brw_note_fence( struct intel_context *intel, GLuint fence )
{
   brw_context(&intel->ctx)->state.dirty.brw |= BRW_NEW_FENCE;
}

/* called from intelWaitForIdle() and intelFlush()
 *
 * For now, just flush everything.  Could be smarter later.
 */
static GLuint brw_flush_cmd( void )
{
   struct brw_mi_flush flush;
   flush.opcode = CMD_MI_FLUSH;
   flush.pad = 0;
   flush.flags = BRW_FLUSH_STATE_CACHE;
   return *(GLuint *)&flush;
}



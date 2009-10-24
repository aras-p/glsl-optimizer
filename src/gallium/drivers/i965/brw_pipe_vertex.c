


void 
brw_pipe_vertex_cleanup( struct brw_context *brw )
{
   for (i = 0; i < VERT_ATTRIB_MAX; i++) {
      brw->sws->bo_unreference(brw->vb.inputs[i].bo);
      brw->vb.inputs[i].bo = NULL;
   }
}

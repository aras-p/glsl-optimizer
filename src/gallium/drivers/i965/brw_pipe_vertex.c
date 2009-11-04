#include "brw_context.h"


void 
brw_pipe_vertex_init( struct brw_context *brw )
{
}


void 
brw_pipe_vertex_cleanup( struct brw_context *brw )
{

   /* Release bound pipe vertex_buffers
    */

   /* Release some other stuff
    */
#if 0
   for (i = 0; i < PIPE_MAX_ATTRIBS; i++) {
      brw->sws->bo_unreference(brw->vb.inputs[i].bo);
      brw->vb.inputs[i].bo = NULL;
   }
#endif
}

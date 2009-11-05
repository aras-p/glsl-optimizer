#include "brw_context.h"


static void brw_set_vertex_elements( struct pipe_context *pipe,
				     unsigned count,
				     const struct pipe_vertex_element *elements )
{
   struct brw_context *brw = brw_context(pipe);

   memcpy(brw->curr.vertex_element, elements, count * sizeof(elements[0]));
   brw->curr.num_vertex_elements = count;

   brw->state.dirty.mesa |= PIPE_NEW_VERTEX_ELEMENT;
}


static void brw_set_vertex_buffers(struct pipe_context *pipe,
				   unsigned count,
				   const struct pipe_vertex_buffer *buffers)
{
   struct brw_context *brw = brw_context(pipe);

   /* XXX: don't we need to take some references here?  It's a bit
    * awkward to do so, though.
    */
   memcpy(brw->curr.vertex_buffer, buffers, count * sizeof(buffers[0]));
   brw->curr.num_vertex_buffers = count;

   brw->state.dirty.mesa |= PIPE_NEW_VERTEX_BUFFER;
}

static void brw_set_edgeflags( struct pipe_context *pipe,
			       const unsigned *bitfield )
{
   /* XXX */
}


void 
brw_pipe_vertex_init( struct brw_context *brw )
{
   brw->base.set_vertex_buffers = brw_set_vertex_buffers;
   brw->base.set_vertex_elements = brw_set_vertex_elements;
   brw->base.set_edgeflags = brw_set_edgeflags;
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
      bo_reference(&brw->vb.inputs[i].bo, NULL);
      brw->vb.inputs[i].bo = NULL;
   }
#endif
}

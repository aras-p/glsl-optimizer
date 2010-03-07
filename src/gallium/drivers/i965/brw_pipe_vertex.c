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
   unsigned i;

   /* Check for no change */
   if (count == brw->curr.num_vertex_buffers &&
       memcmp(brw->curr.vertex_buffer,
              buffers,
              count * sizeof buffers[0]) == 0)
      return;

   /* Adjust refcounts */
   for (i = 0; i < count; i++) 
      pipe_buffer_reference(&brw->curr.vertex_buffer[i].buffer, 
                            buffers[i].buffer);

   for ( ; i < brw->curr.num_vertex_buffers; i++)
      pipe_buffer_reference(&brw->curr.vertex_buffer[i].buffer,
                            NULL);

   /* Copy remaining data */
   memcpy(brw->curr.vertex_buffer, buffers, count * sizeof buffers[0]);
   brw->curr.num_vertex_buffers = count;

   brw->state.dirty.mesa |= PIPE_NEW_VERTEX_BUFFER;
}


void 
brw_pipe_vertex_init( struct brw_context *brw )
{
   brw->base.set_vertex_buffers = brw_set_vertex_buffers;
   brw->base.set_vertex_elements = brw_set_vertex_elements;
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

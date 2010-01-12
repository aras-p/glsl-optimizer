
#include "brw_context.h"
#include "brw_structs.h"
#include "brw_defines.h"

static void brw_set_polygon_stipple( struct pipe_context *pipe,
				     const struct pipe_poly_stipple *stip )
{
   struct brw_context *brw = brw_context(pipe);
   struct brw_polygon_stipple *bps = &brw->curr.bps;
   GLuint i;

   memset(bps, 0, sizeof *bps);
   bps->header.opcode = CMD_POLY_STIPPLE_PATTERN;
   bps->header.length = sizeof *bps/4-2;

   for (i = 0; i < 32; i++)
      bps->stipple[i] = stip->stipple[i]; /* don't invert */

   brw->state.dirty.mesa |= PIPE_NEW_POLYGON_STIPPLE;
}


static void brw_set_scissor_state( struct pipe_context *pipe,
                                   const struct pipe_scissor_state *scissor )
{
   struct brw_context *brw = brw_context(pipe);

   brw->curr.scissor =  *scissor;
   brw->state.dirty.mesa |= PIPE_NEW_SCISSOR;
}


static void brw_set_clip_state( struct pipe_context *pipe,
                                const struct pipe_clip_state *clip )
{
   struct brw_context *brw = brw_context(pipe);

   brw->curr.ucp = *clip;
   brw->state.dirty.mesa |= PIPE_NEW_CLIP;
}


void brw_pipe_misc_init( struct brw_context *brw )
{
   brw->base.set_polygon_stipple = brw_set_polygon_stipple;
   brw->base.set_scissor_state = brw_set_scissor_state;
   brw->base.set_clip_state = brw_set_clip_state;
}


void brw_pipe_misc_cleanup( struct brw_context *brw )
{
}

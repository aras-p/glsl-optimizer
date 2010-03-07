#include "util/u_math.h"
#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "util/u_inlines.h"

#include "brw_context.h"

/**
 * called from intelDrawBuffer()
 */
static void brw_set_framebuffer_state( struct pipe_context *pipe, 
				       const struct pipe_framebuffer_state *fb )
{
   struct brw_context *brw = brw_context(pipe);
   unsigned i;

   /* Dimensions:
    */
   if (brw->curr.fb.width != fb->width ||
       brw->curr.fb.height != fb->height) {
      brw->curr.fb.width = fb->width;
      brw->curr.fb.height = fb->height;
      brw->state.dirty.mesa |= PIPE_NEW_FRAMEBUFFER_DIMENSIONS;
   }
   
   /* Z/Stencil
    */
   if (brw->curr.fb.zsbuf != fb->zsbuf) {
      pipe_surface_reference(&brw->curr.fb.zsbuf, fb->zsbuf);
      brw->state.dirty.mesa |= PIPE_NEW_DEPTH_BUFFER;
   }

   /* Color buffers:
    */
   for (i = 0; i < PIPE_MAX_COLOR_BUFS; i++) {
      if (brw->curr.fb.cbufs[i] != fb->cbufs[i]) {
	 brw->state.dirty.mesa |= PIPE_NEW_COLOR_BUFFERS;
	 pipe_surface_reference(&brw->curr.fb.cbufs[i], fb->cbufs[i]);
      }
   }
   
   if (brw->curr.fb.nr_cbufs != fb->nr_cbufs) {
      brw->curr.fb.nr_cbufs = MIN2(BRW_MAX_DRAW_BUFFERS, fb->nr_cbufs);
      brw->state.dirty.mesa |= PIPE_NEW_NR_CBUFS;
   }
}


static void brw_set_viewport_state( struct pipe_context *pipe,
				    const struct pipe_viewport_state *viewport )
{
   struct brw_context *brw = brw_context(pipe);

   brw->curr.viewport = *viewport;
   brw->curr.ccv.min_depth = viewport->scale[2] * -1.0 + viewport->translate[2];
   brw->curr.ccv.max_depth = viewport->scale[2] *  1.0 + viewport->translate[2];

   if (0)
      debug_printf("%s depth range %f .. %f\n",
                   __FUNCTION__,
                   brw->curr.ccv.min_depth,
                   brw->curr.ccv.max_depth);

   brw->state.dirty.mesa |= PIPE_NEW_VIEWPORT;
}


void brw_pipe_framebuffer_init( struct brw_context *brw )
{
   brw->base.set_framebuffer_state = brw_set_framebuffer_state;
   brw->base.set_viewport_state = brw_set_viewport_state;
}

void brw_pipe_framebuffer_cleanup( struct brw_context *brw )
{
   struct pipe_framebuffer_state *fb = &brw->curr.fb;
   int i;

   for (i = 0; i < PIPE_MAX_COLOR_BUFS; i++) {
      pipe_surface_reference(&fb->cbufs[i], NULL);
   }

   pipe_surface_reference(&fb->zsbuf, NULL);
}

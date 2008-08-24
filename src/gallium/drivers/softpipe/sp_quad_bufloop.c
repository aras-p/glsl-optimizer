
#include "util/u_memory.h"
#include "sp_context.h"
#include "sp_headers.h"
#include "sp_surface.h"
#include "sp_quad.h"


/**
 * Loop over colorbuffers, passing quad to next stage each time.
 */
static void
cbuf_loop_quad(struct quad_stage *qs, struct quad_header *quad)
{
   struct softpipe_context *softpipe = qs->softpipe;
   float tmp[PIPE_MAX_COLOR_BUFS][4][QUAD_SIZE];
   unsigned i;

   assert(sizeof(quad->outputs.color) == sizeof(tmp));
   assert(softpipe->framebuffer.num_cbufs <= PIPE_MAX_COLOR_BUFS);

   /* make copy of original colors since they can get modified
    * by blending and masking.
    * XXX we won't have to do this if the fragment program actually emits
    * N separate colors and we're drawing to N color buffers (MRT).
    * But if we emitted one color and glDrawBuffer(GL_FRONT_AND_BACK) is
    * in effect, we need to save/restore colors like this.
    */
   memcpy(tmp, quad->outputs.color, sizeof(tmp));

   for (i = 0; i < softpipe->framebuffer.num_cbufs; i++) {
      /* set current cbuffer */
#if 0 /* obsolete & going away */
      softpipe->current_cbuf = i;
#endif

      /* pass blended quad to next stage */
      qs->next->run(qs->next, quad);

      /* restore quad's colors for next buffer */
      memcpy(quad->outputs.color, tmp, sizeof(tmp));
   }
}


static void cbuf_loop_begin(struct quad_stage *qs)
{
   qs->next->begin(qs->next);
}


static void cbuf_loop_destroy(struct quad_stage *qs)
{
   FREE( qs );
}


/**
 * Create the colorbuffer loop stage.
 * This is used to implement multiple render targets and GL_FRONT_AND_BACK
 * rendering.
 */
struct quad_stage *sp_quad_bufloop_stage( struct softpipe_context *softpipe )
{
   struct quad_stage *stage = CALLOC_STRUCT(quad_stage);

   stage->softpipe = softpipe;
   stage->begin = cbuf_loop_begin;
   stage->run = cbuf_loop_quad;
   stage->destroy = cbuf_loop_destroy;

   return stage;
}



/**
 * quad polygon stipple stage
 */

#include "sp_context.h"
#include "sp_headers.h"
#include "sp_quad.h"
#include "pipe/p_defines.h"
#include "util/u_memory.h"


/**
 * Apply polygon stipple to quads produced by triangle rasterization
 */
static void
stipple_quad(struct quad_stage *qs, struct quad_header *quad)
{
   static const uint bit31 = 1 << 31;
   static const uint bit30 = 1 << 30;

   if (quad->input.prim == PRIM_TRI) {
      struct softpipe_context *softpipe = qs->softpipe;
      /* need to invert Y to index into OpenGL's stipple pattern */
      int y0, y1;
      uint stipple0, stipple1;
      if (softpipe->rasterizer->origin_lower_left) {
         y0 = softpipe->framebuffer.height - 1 - quad->input.y0;
         y1 = y0 - 1;
      }
      else {
         y0 = quad->input.y0;
         y1 = y0 + 1;
      }
      stipple0 = softpipe->poly_stipple.stipple[y0 % 32];
      stipple1 = softpipe->poly_stipple.stipple[y1 % 32];

#if 1
      {
      const int col0 = quad->input.x0 % 32;
      if ((stipple0 & (bit31 >> col0)) == 0)
         quad->inout.mask &= ~MASK_TOP_LEFT;

      if ((stipple0 & (bit30 >> col0)) == 0)
         quad->inout.mask &= ~MASK_TOP_RIGHT;

      if ((stipple1 & (bit31 >> col0)) == 0)
         quad->inout.mask &= ~MASK_BOTTOM_LEFT;

      if ((stipple1 & (bit30 >> col0)) == 0)
         quad->inout.mask &= ~MASK_BOTTOM_RIGHT;
      }
#else
      /* We'd like to use this code, but we'd need to redefine
       * MASK_TOP_LEFT to be (1 << 1) and MASK_TOP_RIGHT to be (1 << 0),
       * and similarly for the BOTTOM bits.  But that may have undesirable
       * side effects elsewhere.
       */
      const int col0 = 30 - (quad->input.x0 % 32);
      quad->inout.mask &= (((stipple0 >> col0) & 0x3) | 
                     (((stipple1 >> col0) & 0x3) << 2));
#endif
      if (!quad->inout.mask)
         return;
   }

   qs->next->run(qs->next, quad);
}


static void stipple_begin(struct quad_stage *qs)
{
   qs->next->begin(qs->next);
}


static void stipple_destroy(struct quad_stage *qs)
{
   FREE( qs );
}


struct quad_stage *
sp_quad_polygon_stipple_stage( struct softpipe_context *softpipe )
{
   struct quad_stage *stage = CALLOC_STRUCT(quad_stage);

   stage->softpipe = softpipe;
   stage->begin = stipple_begin;
   stage->run = stipple_quad;
   stage->destroy = stipple_destroy;

   return stage;
}

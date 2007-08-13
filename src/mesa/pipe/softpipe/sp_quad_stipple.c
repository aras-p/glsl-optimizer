
/**
 * quad polygon stipple stage
 */

#include "sp_context.h"
#include "sp_headers.h"
#include "sp_quad.h"
#include "pipe/p_defines.h"
#include "pipe/p_util.h"


/**
 * Apply polygon stipple to quads produced by triangle rasterization
 */
static void
stipple_quad(struct quad_stage *qs, struct quad_header *quad)
{
   if (quad->prim == PRIM_TRI) {
      struct softpipe_context *softpipe = qs->softpipe;
      const int col0 = quad->x0 % 32;
      const int row0 = quad->y0 % 32;
      const unsigned stipple0 = softpipe->poly_stipple.stipple[row0];
      const unsigned stipple1 = softpipe->poly_stipple.stipple[row0 + 1];

      /* XXX there may be a better way to lay out the stored stipple
       * values to further simplify this computation.
       */
      quad->mask &= (((stipple0 >> col0) & 0x3) | 
                     (((stipple1 >> col0) & 0x3) << 2));

      if (quad->mask)
         qs->next->run(qs->next, quad);
   }
}


static void stipple_begin(struct quad_stage *qs)
{
   if (qs->next)
      qs->next->begin(qs->next);
}


struct quad_stage *
sp_quad_polygon_stipple_stage( struct softpipe_context *softpipe )
{
   struct quad_stage *stage = CALLOC_STRUCT(quad_stage);

   stage->softpipe = softpipe;
   stage->begin = stipple_begin;
   stage->run = stipple_quad;

   return stage;
}

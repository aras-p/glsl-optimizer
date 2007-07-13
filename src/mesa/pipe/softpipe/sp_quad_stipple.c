
/**
 * quad polygon stipple stage
 */

#include "glheader.h"
#include "imports.h"
#include "sp_context.h"
#include "sp_headers.h"
#include "sp_quad.h"
#include "pipe/p_defines.h"


/**
 * Apply polygon stipple to quads produced by triangle rasterization
 */
static void
stipple_quad(struct quad_stage *qs, struct quad_header *quad)
{
   if (quad->prim == PRIM_TRI) {
      struct softpipe_context *softpipe = qs->softpipe;
      const GLint col0 = quad->x0 % 32;
      const GLint row0 = quad->y0 % 32;
      const GLuint stipple0 = softpipe->poly_stipple.stipple[row0];
      const GLuint stipple1 = softpipe->poly_stipple.stipple[row0 + 1];

      /* XXX this should be acheivable without conditionals */
#if 1
      GLbitfield mask = 0x0;

      if ((1 << col0) & stipple0)
         mask |= MASK_BOTTOM_LEFT;

      if ((2 << col0) & stipple0)	/* note: col0 <= 30 */
         mask |= MASK_BOTTOM_RIGHT;

      if ((1 << col0) & stipple1)
         mask |= MASK_TOP_LEFT;

      if ((2 << col0) & stipple1)
         mask |= MASK_TOP_RIGHT;

      quad->mask &= mask;
#else
      /* XXX there may be a better way to lay out the stored stipple
       * values to further simplify this computation.
       */
      quad->mask &= (((stipple0 >> col0) & 0x3) | 
                     (((stipple1 >> col0) & 0x3) << 2));
#endif

      if (quad->mask)
         qs->next->run(qs->next, quad);
   }
}


struct quad_stage *
sp_quad_polygon_stipple_stage( struct softpipe_context *softpipe )
{
   struct quad_stage *stage = CALLOC_STRUCT(quad_stage);

   stage->softpipe = softpipe;
   stage->run = stipple_quad;

   return stage;
}

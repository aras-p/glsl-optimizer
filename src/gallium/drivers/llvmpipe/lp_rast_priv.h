#ifndef LP_RAST_PRIV_H
#define LP_RAST_PRIV_H

#include "lp_rast.h"


/* We can choose whatever layout for the internal tile storage we
 * prefer:
 */
struct lp_rast_tile
{
   uint8_t *color;

   uint8_t *depth;
};


struct lp_rasterizer {

   /* We can choose whatever layout for the internal tile storage we
    * prefer:
    */
   struct lp_rast_tile tile;

      
   unsigned x;
   unsigned y;

   
   struct {
      struct pipe_surface *color;
      struct pipe_surface *zstencil;
      unsigned clear_color;
      unsigned clear_depth;
      char clear_stencil;
   } state;
};

#endif

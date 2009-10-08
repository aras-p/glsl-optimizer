#ifndef LP_RAST_PRIV_H
#define LP_RAST_PRIV_H

#include "lp_rast.h"

struct lp_rasterizer {

   /* We can choose whatever layout for the internal tile storage we
    * prefer:
    */
   struct {
      unsigned color[TILESIZE][TILESIZE];
      unsigned depth[TILESIZE][TILESIZE];
      char stencil[TILESIZE][TILESIZE];
   } tile;

      
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

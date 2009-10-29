#ifndef BRW_PIPE_RAST_H
#define BRW_PIPE_RAST_H

#include "brw_clip.h"

struct brw_rasterizer_state {
   struct pipe_rasterizer_state templ; /* for draw module */

   /* Precalculated hardware state:
    */
   struct brw_clip_prog_key clip_key;
   struct brw_line_stipple bls;
   unsigned unfilled_aa_line;
};

#endif

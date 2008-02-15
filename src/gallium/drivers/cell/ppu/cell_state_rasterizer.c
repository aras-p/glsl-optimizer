/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include "pipe/p_defines.h"
#include "pipe/p_util.h"
#include "pipe/draw/draw_context.h"
#include "cell_context.h"
#include "cell_state.h"



struct spu_rasterizer_state
{
   unsigned flatshade:1;
#if 0
   unsigned light_twoside:1;
   unsigned front_winding:2;  /**< PIPE_WINDING_x */
   unsigned cull_mode:2;      /**< PIPE_WINDING_x */
   unsigned fill_cw:2;        /**< PIPE_POLYGON_MODE_x */
   unsigned fill_ccw:2;       /**< PIPE_POLYGON_MODE_x */
   unsigned offset_cw:1;
   unsigned offset_ccw:1;
#endif
   unsigned scissor:1;
   unsigned poly_smooth:1;
   unsigned poly_stipple_enable:1;
   unsigned point_smooth:1;
#if 0
   unsigned point_sprite:1;
   unsigned point_size_per_vertex:1; /**< size computed in vertex shader */
#endif
   unsigned multisample:1;         /* XXX maybe more ms state in future */
   unsigned line_smooth:1;
   unsigned line_stipple_enable:1;
   unsigned line_stipple_factor:8;  /**< [1..256] actually */
   unsigned line_stipple_pattern:16;
#if 0
   unsigned bypass_clipping:1;
#endif
   unsigned origin_lower_left:1;  /**< Is (0,0) the lower-left corner? */

   float line_width;
   float point_size;           /**< used when no per-vertex size */
#if 0
   float offset_units;
   float offset_scale;
   ubyte sprite_coord_mode[PIPE_MAX_SHADER_OUTPUTS]; /**< PIPE_SPRITE_COORD_ */
#endif
};



void *
cell_create_rasterizer_state(struct pipe_context *pipe,
                             const struct pipe_rasterizer_state *setup)
{
   struct pipe_rasterizer_state *state
      = MALLOC(sizeof(struct pipe_rasterizer_state));
   memcpy(state, setup, sizeof(struct pipe_rasterizer_state));
   return state;
}


void
cell_bind_rasterizer_state(struct pipe_context *pipe, void *setup)
{
   struct cell_context *cell = cell_context(pipe);

   /* pass-through to draw module */
   draw_set_rasterizer_state(cell->draw, setup);

   cell->rasterizer = (struct pipe_rasterizer_state *)setup;

   cell->dirty |= CELL_NEW_RASTERIZER;
}


void
cell_delete_rasterizer_state(struct pipe_context *pipe, void *rasterizer)
{
   FREE(rasterizer);
}

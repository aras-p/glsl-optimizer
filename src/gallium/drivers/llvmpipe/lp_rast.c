/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "util/u_memory.h"

#include "lp_state.h"
#include "lp_quad.h"
#include "lp_rast.h"


struct lp_rasterizer *lp_rast_create( void )
{
   return CALLOC_STRUCT(lp_rasterizer);
}

void lp_rast_bind_surfaces( struct lp_rasterizer *,
			    struct pipe_surface *color,
			    struct pipe_surface *zstencil,
			    const float *clear_color,
			    double clear_depth,
			    unsigned clear_stencil)
{
   pipe_surface_reference(&rast->state.color, color);
   pipe_surface_reference(&rast->state.depth, depth);
   rast->state.clear_color = util_pack_8888(clear_color);
   rast->state.clear_depth = clear_depth * 0xffffffff;
   rast->state.clear_stencil = clear_stencil;
}

/* Begining of each tile:
 */
void lp_rast_start_tile( struct lp_rasterizer *,
			 unsigned x,
			 unsigned y )
{
   rast->x = x;
   rast->y = y;
}

void lp_rast_clear_color( struct lp_rasterizer *rast )
{
   const unsigned clear_color = rast->state.clear_color;
   unsigned i, j;
   
   for (i = 0; i < TILESIZE; i++)
      for (j = 0; j < TILESIZE; j++)
	 rast->tile[i][j] = clear_color;
}

void lp_rast_clear_depth( struct lp_rasterizer *rast )
{
   const unsigned clear_depth = rast->state.clear_depth;
   unsigned i, j;
   
   for (i = 0; i < TILESIZE; i++)
      for (j = 0; j < TILESIZE; j++)
	 rast->tile[i][j] = clear_depth;
}

void lp_rast_clear_stencil( struct lp_rasterizer *rast )
{
   const unsigned clear_stencil = rast->state.clear_stencil;

   memset(rast->tile.stencil, clear_stencil, sizeof rast->tile.stencil );
}

void lp_rast_load_color( struct lp_rasterizer *rast )
{
   /* call u_tile func to load colors from surface */
}

void lp_rast_load_zstencil( struct lp_rasterizer *rast )
{
   /* call u_tile func to load depth (and stencil?) from surface */
}

/* Within a tile:
 */
void lp_rast_set_state( struct lp_rasterizer *rast,
		       const struct lp_rast_state *state )
{
   rast->shader_state = state;
   lp->quad.first->begin( lp->quad.first );

}


void lp_rast_shade_tile( struct lp_rasterizer *rast,
			 const struct lp_rast_shader_inputs *inputs )
{
   /* Set up the silly quad coef pointers
    */
   for (i = 0; i < 4; i++) {
      rast->quads[i].posCoef = &inputs->posCoef;
      rast->quads[i].coef = inputs->coef;
   }

   /* Use the existing preference for 8x2 (four quads) shading:
    */
   for (i = 0; i < TILESIZE; i += 8) {
      for (j = 0; j < TILESIZE; j += 2) {
	 rast->shader_state.shade( inputs->jc,
				   rast->x + i,
				   rast->y + j,
				   rast->quads, 4 );
      }
   }
}


void lp_rast_shade_quads( const struct lp_rast_state *state,
                          struct lp_rast_tile *tile,
                          struct quad_header **quads,
                          unsigned nr )
{
   struct lp_fragment_shader *fs = llvmpipe->fs;
   struct quad_header *quad = quads[0];
   const unsigned x = quad->input.x0;
   const unsigned y = quad->input.y0;
   uint8_t *color;
   uint8_t *depth;
   uint32_t ALIGN16_ATTRIB mask[4][NUM_CHANNELS];
   unsigned chan_index;
   unsigned q;

   /* Sanity checks */
   assert(nr * QUAD_SIZE == TILE_VECTOR_HEIGHT * TILE_VECTOR_WIDTH);
   assert(x % TILE_VECTOR_WIDTH == 0);
   assert(y % TILE_VECTOR_HEIGHT == 0);
   for (q = 0; q < nr; ++q) {
      assert(quads[q]->input.x0 == x + q*2);
      assert(quads[q]->input.y0 == y);
   }

   /* mask */
   for (q = 0; q < 4; ++q)
      for (chan_index = 0; chan_index < NUM_CHANNELS; ++chan_index)
         mask[q][chan_index] = quads[q]->inout.mask & (1 << chan_index) ? ~0 : 0;

   /* color buffer */
   color = &TILE_PIXEL(tile->color, x, y, 0);

   /* depth buffer */
   assert((x % 2) == 0);
   assert((y % 2) == 0);
   depth = (uint8_t)*tile->depth + y*TILE_SIZE*4 + 2*x*4;

   /* XXX: This will most likely fail on 32bit x86 without -mstackrealign */
   assert(lp_check_alignment(mask, 16));

   assert(lp_check_alignment(depth, 16));
   assert(lp_check_alignment(color, 16));
   assert(lp_check_alignment(state->jc.blend_color, 16));

   /* run shader */
   state->jit_function( &state->jc,
                        x, y,
                        quad->coef->a0,
                        quad->coef->dadx,
                        quad->coef->dady,
                        &mask[0][0],
                        color,
                        depth);

}


/* End of tile:
 */
void lp_rast_store_color( struct lp_rasterizer *rast )
{
   /* call u_tile func to store colors to surface */
}

void lp_rast_store_zstencil( struct lp_rasterizer *rast )
{
   /* call u_tile func to store depth/stencil to surface */
}

/* Shutdown:
 */
void lp_rast_destroy( struct lp_rasterizer *rast )
{
   FREE(rast);
}


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
#include "lp_rast.h"
#include "lp_rast_priv.h"
#include "lp_tile_soa.h"
#include "lp_bld_debug.h"


struct lp_rasterizer *lp_rast_create( void )
{
   struct lp_rasterizer *rast;

   rast = CALLOC_STRUCT(lp_rasterizer);
   if(!rast)
      return NULL;

   rast->tile.color = align_malloc( TILE_SIZE*TILE_SIZE*4, 16 );
   rast->tile.depth = align_malloc( TILE_SIZE*TILE_SIZE*4, 16 );

   return rast;
}


void lp_rast_begin( struct lp_rasterizer *rast,
                    unsigned width,
                    unsigned height )
{
   rast->width = width;
   rast->height = height;
   rast->check_for_clipped_tiles = (width % TILESIZE != 0 ||
                                    height % TILESIZE != 0);
}

void lp_rast_bind_color( struct lp_rasterizer *rast,
                         struct pipe_surface *cbuf,
                         boolean write_color )
{
   pipe_surface_reference(&rast->state.cbuf, cbuf);
}

void lp_rast_bind_zstencil( struct lp_rasterizer *rast,
                            struct pipe_surface *zsbuf,
                            boolean write_zstencil )
{
   pipe_surface_reference(&rast->state.zsbuf, zsbuf);
}


/* Begining of each tile:
 */
void lp_rast_start_tile( struct lp_rasterizer *rast,
			 unsigned x,
			 unsigned y )
{
   rast->x = x;
   rast->y = y;
}

void lp_rast_clear_color( struct lp_rasterizer *rast,
                          const union lp_rast_cmd_arg *arg )
{
   const uint8_t *clear_color = arg->clear_color;
   
   if (clear_color[0] == clear_color[1] &&
       clear_color[1] == clear_color[2] &&
       clear_color[2] == clear_color[3]) {
      memset(rast->tile.color, clear_color[0], TILE_SIZE * TILE_SIZE * 4);
   }
   else {
      unsigned x, y, chan;
      for (y = 0; y < TILE_SIZE; y++)
         for (x = 0; x < TILE_SIZE; x++)
            for (chan = 0; chan < 4; ++chan)
               TILE_PIXEL(rast->tile.color, x, y, chan) = clear_color[chan];
   }
}

void lp_rast_clear_zstencil( struct lp_rasterizer *rast,
                             const union lp_rast_cmd_arg *arg)
{
   const unsigned clear_zstencil = arg->clear_zstencil;
   unsigned i, j;
   
   for (i = 0; i < TILE_SIZE; i++)
      for (j = 0; j < TILE_SIZE; j++)
	 rast->tile.depth[i*TILE_SIZE + j] = clear_zstencil;
}


void lp_rast_load_color( struct lp_rasterizer *rast,
                         const union lp_rast_cmd_arg *arg)
{
   /* call u_tile func to load colors from surface */
}

void lp_rast_load_zstencil( struct lp_rasterizer *rast,
                            const union lp_rast_cmd_arg *arg )
{
   /* call u_tile func to load depth (and stencil?) from surface */
}

/* Within a tile:
 */
void lp_rast_set_state( struct lp_rasterizer *rast,
                        const union lp_rast_cmd_arg *arg )
{
   rast->shader_state = arg->set_state;

}


void lp_rast_shade_tile( struct lp_rasterizer *rast,
                         const union lp_rast_cmd_arg *arg,
			 const struct lp_rast_shader_inputs *inputs )
{
   const unsigned masks[4] = {~0, ~0, ~0, ~0};
   unsigned x, y;

   /* Use the existing preference for 8x2 (four quads) shading:
    */
   for (y = 0; y < TILE_SIZE; y += 2)
      for (x = 0; x < TILE_SIZE; x += 8)
         lp_rast_shade_quads( rast, inputs, x, y, masks);
}


void lp_rast_shade_quads( struct lp_rasterizer *rast,
                          const struct lp_rast_shader_inputs *inputs,
                          unsigned x, unsigned y,
                          const unsigned *masks)
{
   const struct lp_rast_state *state = rast->shader_state;
   struct lp_rast_tile *tile = &rast->tile;
   void *color;
   void *depth;
   uint32_t ALIGN16_ATTRIB mask[4][NUM_CHANNELS];
   unsigned chan_index;
   unsigned q;

   /* Sanity checks */
   assert(x % TILE_VECTOR_WIDTH == 0);
   assert(y % TILE_VECTOR_HEIGHT == 0);

   /* mask */
   for (q = 0; q < 4; ++q)
      for (chan_index = 0; chan_index < NUM_CHANNELS; ++chan_index)
         mask[q][chan_index] = masks[q] & (1 << chan_index) ? ~0 : 0;

   /* color buffer */
   color = &TILE_PIXEL(tile->color, x, y, 0);

   /* depth buffer */
   assert((x % 2) == 0);
   assert((y % 2) == 0);
   depth = tile->depth + y*TILE_SIZE + 2*x;

   /* XXX: This will most likely fail on 32bit x86 without -mstackrealign */
   assert(lp_check_alignment(mask, 16));

   assert(lp_check_alignment(depth, 16));
   assert(lp_check_alignment(color, 16));
   assert(lp_check_alignment(state->jc.blend_color, 16));

   /* run shader */
   state->shader( &state->jc,
                  x, y,
                  inputs->a0,
                  inputs->dadx,
                  inputs->dady,
                  &mask[0][0],
                  color,
                  depth);

}


/* End of tile:
 */


void lp_rast_end_tile( struct lp_rasterizer *rast )
{
   struct pipe_surface *surface;
   struct pipe_screen *screen;
   struct pipe_transfer *transfer;
   const unsigned x = rast->x;
   const unsigned y = rast->y;
   unsigned w = TILE_SIZE;
   unsigned h = TILE_SIZE;
   void *map;

   surface = rast->state.cbuf;
   if(!surface)
      return;

   screen = surface->texture->screen;

   if(x + w > rast->width)
      w = rast->width - x;
   if(y + h > rast->height)
      h = rast->height - y;

   transfer = screen->get_tex_transfer(screen,
                                       surface->texture,
                                       surface->face,
                                       surface->level,
                                       surface->zslice,
                                       PIPE_TRANSFER_READ_WRITE,
                                       x, y, w, h);
   if(!transfer)
      return;

   map = screen->transfer_map(screen, transfer);
   if(map) {
      lp_tile_write_4ub(transfer->format,
                        rast->tile.color,
                        map, transfer->stride,
                        x, y, w, h);

      screen->transfer_unmap(screen, transfer);
   }

   screen->tex_transfer_destroy(transfer);

   if (0) {
      /* FIXME: call u_tile func to store depth/stencil to surface */
   }
}

/* Shutdown:
 */
void lp_rast_destroy( struct lp_rasterizer *rast )
{
   pipe_surface_reference(&rast->state.cbuf, NULL);
   pipe_surface_reference(&rast->state.zsbuf, NULL);
   align_free(rast->tile.depth);
   align_free(rast->tile.color);
   FREE(rast);
}


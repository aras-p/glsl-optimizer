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

/**
 * Quad blending.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 * @author Brian Paul
 */

#include "pipe/p_defines.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_debug_dump.h"
#include "lp_context.h"
#include "lp_state.h"
#include "lp_quad.h"
#include "lp_surface.h"
#include "lp_tile_cache.h"
#include "lp_tile_soa.h"
#include "lp_quad_pipe.h"


static void blend_begin(struct quad_stage *qs)
{
}


static void
blend_run(struct quad_stage *qs,
          struct quad_header *quads[],
          unsigned nr)
{
   struct llvmpipe_context *llvmpipe = qs->llvmpipe;
   struct lp_blend_state *blend = llvmpipe->blend;
   unsigned cbuf;
   uint q, i, j;

   for (cbuf = 0; cbuf < llvmpipe->framebuffer.nr_cbufs; cbuf++) 
   {
      unsigned x0 = quads[0]->input.x0;
      unsigned y0 = quads[0]->input.y0;
      uint8_t ALIGN16_ATTRIB src[NUM_CHANNELS][TILE_VECTOR_HEIGHT*TILE_VECTOR_WIDTH];
      uint8_t ALIGN16_ATTRIB mask[16];
      uint8_t *dst;
      struct llvmpipe_cached_tile *tile
         = lp_get_cached_tile(llvmpipe->cbuf_cache[cbuf], x0, y0);

      assert(nr * QUAD_SIZE == TILE_VECTOR_HEIGHT * TILE_VECTOR_WIDTH);

      assert(x0 % TILE_VECTOR_WIDTH == 0);
      assert(y0 % TILE_VECTOR_HEIGHT == 0);

      dst = &TILE_PIXEL(tile->data.color, x0 & (TILE_SIZE-1), y0 & (TILE_SIZE-1), 0);

      for (q = 0; q < nr; ++q) {
         struct quad_header *quad = quads[q];
         const int itx = (quad->input.x0 & (TILE_SIZE-1));
         const int ity = (quad->input.y0 & (TILE_SIZE-1));

         assert(quad->input.x0 == x0 + q*2);
         assert(quad->input.y0 == y0);

         /* get/swizzle src/dest colors
          */
         for (j = 0; j < QUAD_SIZE; j++) {
            int x = itx + (j & 1);
            int y = ity + (j >> 1);

            assert(x < TILE_SIZE);
            assert(y < TILE_SIZE);

            for (i = 0; i < 4; i++) {
               src[i][4*q + j] = float_to_ubyte(quad->output.color[cbuf][i][j]);
            }
            mask[4*q + j] = quad->inout.mask & (1 << j) ? ~0 : 0;
         }
      }

      assert(blend->jit_function);
      assert((((uintptr_t)src) & 0xf) == 0);
      assert((((uintptr_t)dst) & 0xf) == 0);
      assert((((uintptr_t)llvmpipe->blend_color) & 0xf) == 0);
      if(blend->jit_function)
         blend->jit_function( mask,
                              &src[0][0],
                              &llvmpipe->blend_color[0][0],
                              dst );
   }
}


static void blend_destroy(struct quad_stage *qs)
{
   FREE( qs );
}


struct quad_stage *lp_quad_blend_stage( struct llvmpipe_context *llvmpipe )
{
   struct quad_stage *stage = CALLOC_STRUCT(quad_stage);

   stage->llvmpipe = llvmpipe;
   stage->begin = blend_begin;
   stage->run = blend_run;
   stage->destroy = blend_destroy;

   return stage;
}

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


static void
logicop_quad(struct quad_stage *qs, 
             uint8_t src[][16],
             uint8_t dst[][16])
{
   struct llvmpipe_context *llvmpipe = qs->llvmpipe;
   uint32_t *src4 = (uint32_t *) src;
   uint32_t *dst4 = (uint32_t *) dst;
   uint32_t *res4 = (uint32_t *) src;
   uint j;

   switch (llvmpipe->blend->base.logicop_func) {
   case PIPE_LOGICOP_CLEAR:
      for (j = 0; j < 4; j++)
         res4[j] = 0;
      break;
   case PIPE_LOGICOP_NOR:
      for (j = 0; j < 4; j++)
         res4[j] = ~(src4[j] | dst4[j]);
      break;
   case PIPE_LOGICOP_AND_INVERTED:
      for (j = 0; j < 4; j++)
         res4[j] = ~src4[j] & dst4[j];
      break;
   case PIPE_LOGICOP_COPY_INVERTED:
      for (j = 0; j < 4; j++)
         res4[j] = ~src4[j];
      break;
   case PIPE_LOGICOP_AND_REVERSE:
      for (j = 0; j < 4; j++)
         res4[j] = src4[j] & ~dst4[j];
      break;
   case PIPE_LOGICOP_INVERT:
      for (j = 0; j < 4; j++)
         res4[j] = ~dst4[j];
      break;
   case PIPE_LOGICOP_XOR:
      for (j = 0; j < 4; j++)
         res4[j] = dst4[j] ^ src4[j];
      break;
   case PIPE_LOGICOP_NAND:
      for (j = 0; j < 4; j++)
         res4[j] = ~(src4[j] & dst4[j]);
      break;
   case PIPE_LOGICOP_AND:
      for (j = 0; j < 4; j++)
         res4[j] = src4[j] & dst4[j];
      break;
   case PIPE_LOGICOP_EQUIV:
      for (j = 0; j < 4; j++)
         res4[j] = ~(src4[j] ^ dst4[j]);
      break;
   case PIPE_LOGICOP_NOOP:
      for (j = 0; j < 4; j++)
         res4[j] = dst4[j];
      break;
   case PIPE_LOGICOP_OR_INVERTED:
      for (j = 0; j < 4; j++)
         res4[j] = ~src4[j] | dst4[j];
      break;
   case PIPE_LOGICOP_COPY:
      for (j = 0; j < 4; j++)
         res4[j] = src4[j];
      break;
   case PIPE_LOGICOP_OR_REVERSE:
      for (j = 0; j < 4; j++)
         res4[j] = src4[j] | ~dst4[j];
      break;
   case PIPE_LOGICOP_OR:
      for (j = 0; j < 4; j++)
         res4[j] = src4[j] | dst4[j];
      break;
   case PIPE_LOGICOP_SET:
      for (j = 0; j < 4; j++)
         res4[j] = ~0;
      break;
   default:
      assert(0);
   }
}


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
   uint q, i, j, k;

   for (cbuf = 0; cbuf < llvmpipe->framebuffer.nr_cbufs; cbuf++) 
   {
      uint8_t ALIGN16_ATTRIB src[4][16];
      uint8_t ALIGN16_ATTRIB dst[4][16];
      struct llvmpipe_cached_tile *tile
         = lp_get_cached_tile(llvmpipe->cbuf_cache[cbuf],
                              quads[0]->input.x0, 
                              quads[0]->input.y0);

      for (q = 0; q < nr; q += 4) {
         for (k = 0; k < 4 && q + k < nr; ++k) {
            struct quad_header *quad = quads[q + k];
            const int itx = (quad->input.x0 & (TILE_SIZE-1));
            const int ity = (quad->input.y0 & (TILE_SIZE-1));

            /* get/swizzle src/dest colors
             */
            for (j = 0; j < QUAD_SIZE; j++) {
               int x = itx + (j & 1);
               int y = ity + (j >> 1);
               for (i = 0; i < 4; i++) {
                  src[i][4*k + j] = float_to_ubyte(quad->output.color[cbuf][i][j]);
                  dst[i][4*k + j] = TILE_PIXEL(tile->data.color, x, y, i);
               }
            }
         }


         if (blend->base.logicop_enable) {
            logicop_quad( qs, src, dst );
         }
         else {
            assert(blend->jit_function);
            assert((((uintptr_t)src) & 0xf) == 0);
            assert((((uintptr_t)dst) & 0xf) == 0);
            assert((((uintptr_t)llvmpipe->blend_color) & 0xf) == 0);
            if(blend->jit_function)
               blend->jit_function( src, dst, llvmpipe->blend_color, src );
         }

         /* Output color values
          */
         for (k = 0; k < 4 && q + k < nr; ++k) {
            struct quad_header *quad = quads[q + k];
            const int itx = (quad->input.x0 & (TILE_SIZE-1));
            const int ity = (quad->input.y0 & (TILE_SIZE-1));

            for (j = 0; j < QUAD_SIZE; j++) {
               if (quad->inout.mask & (1 << j)) {
                  int x = itx + (j & 1);
                  int y = ity + (j >> 1);
                  for (i = 0; i < 4; i++) { /* loop over color chans */
                     TILE_PIXEL(tile->data.color, x, y, i) = src[i][4*k + j];
                  }
               }
            }
         }
      }
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

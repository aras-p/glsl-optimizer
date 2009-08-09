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
#include "lp_quad_pipe.h"


static void
logicop_quad(struct quad_stage *qs, 
             float (*quadColor)[4],
             float (*dest)[4])
{
   struct llvmpipe_context *llvmpipe = qs->llvmpipe;
   ubyte src[4][4], dst[4][4], res[4][4];
   uint *src4 = (uint *) src;
   uint *dst4 = (uint *) dst;
   uint *res4 = (uint *) res;
   uint j;


   /* convert to ubyte */
   for (j = 0; j < 4; j++) { /* loop over R,G,B,A channels */
      dst[j][0] = float_to_ubyte(dest[j][0]); /* P0 */
      dst[j][1] = float_to_ubyte(dest[j][1]); /* P1 */
      dst[j][2] = float_to_ubyte(dest[j][2]); /* P2 */
      dst[j][3] = float_to_ubyte(dest[j][3]); /* P3 */

      src[j][0] = float_to_ubyte(quadColor[j][0]); /* P0 */
      src[j][1] = float_to_ubyte(quadColor[j][1]); /* P1 */
      src[j][2] = float_to_ubyte(quadColor[j][2]); /* P2 */
      src[j][3] = float_to_ubyte(quadColor[j][3]); /* P3 */
   }

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

   for (j = 0; j < 4; j++) {
      quadColor[j][0] = ubyte_to_float(res[j][0]);
      quadColor[j][1] = ubyte_to_float(res[j][1]);
      quadColor[j][2] = ubyte_to_float(res[j][2]);
      quadColor[j][3] = ubyte_to_float(res[j][3]);
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
   uint q, i, j;

   for (cbuf = 0; cbuf < llvmpipe->framebuffer.nr_cbufs; cbuf++) 
   {
      float ALIGN16_ATTRIB dest[4][QUAD_SIZE];
      struct llvmpipe_cached_tile *tile
         = lp_get_cached_tile(llvmpipe->cbuf_cache[cbuf],
                              quads[0]->input.x0, 
                              quads[0]->input.y0);

      for (q = 0; q < nr; q++) {
         struct quad_header *quad = quads[q];
         float (*quadColor)[4] = quad->output.color[cbuf];
         const int itx = (quad->input.x0 & (TILE_SIZE-1));
         const int ity = (quad->input.y0 & (TILE_SIZE-1));

         /* get/swizzle dest colors 
          */
         for (j = 0; j < QUAD_SIZE; j++) {
            int x = itx + (j & 1);
            int y = ity + (j >> 1);
            for (i = 0; i < 4; i++) {
               dest[i][j] = tile->data.color[y][x][i];
            }
         }


         if (blend->base.logicop_enable) {
            logicop_quad( qs, quadColor, dest );
         }
         else {
            assert(blend->jit_function);
            assert((((uintptr_t)quadColor) & 0xf) == 0);
            assert((((uintptr_t)dest) & 0xf) == 0);
            assert((((uintptr_t)llvmpipe->blend_color) & 0xf) == 0);
            if(blend->jit_function)
               blend->jit_function( quadColor, dest, llvmpipe->blend_color, quadColor );
         }

         /* Output color values
          */
         for (j = 0; j < QUAD_SIZE; j++) {
            if (quad->inout.mask & (1 << j)) {
               int x = itx + (j & 1);
               int y = ity + (j >> 1);
               for (i = 0; i < 4; i++) { /* loop over color chans */
                  tile->data.color[y][x][i] = quadColor[i][j];
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

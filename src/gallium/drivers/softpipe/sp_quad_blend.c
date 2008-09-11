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
 * quad blending
 * \author Brian Paul
 */

#include "pipe/p_defines.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "sp_context.h"
#include "sp_headers.h"
#include "sp_surface.h"
#include "sp_tile_cache.h"
#include "sp_quad.h"


#define VEC4_COPY(DST, SRC) \
do { \
    DST[0] = SRC[0]; \
    DST[1] = SRC[1]; \
    DST[2] = SRC[2]; \
    DST[3] = SRC[3]; \
} while(0)

#define VEC4_SCALAR(DST, SRC) \
do { \
    DST[0] = SRC; \
    DST[1] = SRC; \
    DST[2] = SRC; \
    DST[3] = SRC; \
} while(0)

#define VEC4_ADD(R, A, B) \
do { \
   R[0] = A[0] + B[0]; \
   R[1] = A[1] + B[1]; \
   R[2] = A[2] + B[2]; \
   R[3] = A[3] + B[3]; \
} while (0)

#define VEC4_SUB(R, A, B) \
do { \
   R[0] = A[0] - B[0]; \
   R[1] = A[1] - B[1]; \
   R[2] = A[2] - B[2]; \
   R[3] = A[3] - B[3]; \
} while (0)

#define VEC4_MUL(R, A, B) \
do { \
   R[0] = A[0] * B[0]; \
   R[1] = A[1] * B[1]; \
   R[2] = A[2] * B[2]; \
   R[3] = A[3] * B[3]; \
} while (0)

#define VEC4_MIN(R, A, B) \
do { \
   R[0] = (A[0] < B[0]) ? A[0] : B[0]; \
   R[1] = (A[1] < B[1]) ? A[1] : B[1]; \
   R[2] = (A[2] < B[2]) ? A[2] : B[2]; \
   R[3] = (A[3] < B[3]) ? A[3] : B[3]; \
} while (0)

#define VEC4_MAX(R, A, B) \
do { \
   R[0] = (A[0] > B[0]) ? A[0] : B[0]; \
   R[1] = (A[1] > B[1]) ? A[1] : B[1]; \
   R[2] = (A[2] > B[2]) ? A[2] : B[2]; \
   R[3] = (A[3] > B[3]) ? A[3] : B[3]; \
} while (0)



static void
logicop_quad(struct quad_stage *qs, struct quad_header *quad)
{
   struct softpipe_context *softpipe = qs->softpipe;
   uint cbuf;

   /* loop over colorbuffer outputs */
   for (cbuf = 0; cbuf < softpipe->framebuffer.num_cbufs; cbuf++) {
      float dest[4][QUAD_SIZE];
      ubyte src[4][4], dst[4][4], res[4][4];
      uint *src4 = (uint *) src;
      uint *dst4 = (uint *) dst;
      uint *res4 = (uint *) res;
      struct softpipe_cached_tile *
         tile = sp_get_cached_tile(softpipe,
                                   softpipe->cbuf_cache[cbuf],
                                   quad->input.x0, quad->input.y0);
      float (*quadColor)[4] = quad->output.color[cbuf];
      uint i, j;

      /* get/swizzle dest colors */
      for (j = 0; j < QUAD_SIZE; j++) {
         int x = (quad->input.x0 & (TILE_SIZE-1)) + (j & 1);
         int y = (quad->input.y0 & (TILE_SIZE-1)) + (j >> 1);
         for (i = 0; i < 4; i++) {
            dest[i][j] = tile->data.color[y][x][i];
         }
      }

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

      switch (softpipe->blend->logicop_func) {
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

   /* pass quad to next stage */
   qs->next->run(qs->next, quad);
}




static void
blend_quad(struct quad_stage *qs, struct quad_header *quad)
{
   static const float zero[4] = { 0, 0, 0, 0 };
   static const float one[4] = { 1, 1, 1, 1 };

   struct softpipe_context *softpipe = qs->softpipe;
   uint cbuf;

   if (softpipe->blend->logicop_enable) {
      logicop_quad(qs, quad);
      return;
   }

   /* loop over colorbuffer outputs */
   for (cbuf = 0; cbuf < softpipe->framebuffer.num_cbufs; cbuf++) {
      float source[4][QUAD_SIZE], dest[4][QUAD_SIZE];
      struct softpipe_cached_tile *tile
         = sp_get_cached_tile(softpipe,
                              softpipe->cbuf_cache[cbuf],
                              quad->input.x0, quad->input.y0);
      float (*quadColor)[4] = quad->output.color[cbuf];
      uint i, j;

      /* get/swizzle dest colors */
      for (j = 0; j < QUAD_SIZE; j++) {
         int x = (quad->input.x0 & (TILE_SIZE-1)) + (j & 1);
         int y = (quad->input.y0 & (TILE_SIZE-1)) + (j >> 1);
         for (i = 0; i < 4; i++) {
            dest[i][j] = tile->data.color[y][x][i];
         }
      }

      /*
       * Compute src/first term RGB
       */
      switch (softpipe->blend->rgb_src_factor) {
      case PIPE_BLENDFACTOR_ONE:
         VEC4_COPY(source[0], quadColor[0]); /* R */
         VEC4_COPY(source[1], quadColor[1]); /* G */
         VEC4_COPY(source[2], quadColor[2]); /* B */
         break;
      case PIPE_BLENDFACTOR_SRC_COLOR:
         VEC4_MUL(source[0], quadColor[0], quadColor[0]); /* R */
         VEC4_MUL(source[1], quadColor[1], quadColor[1]); /* G */
         VEC4_MUL(source[2], quadColor[2], quadColor[2]); /* B */
         break;
      case PIPE_BLENDFACTOR_SRC_ALPHA:
         {
            const float *alpha = quadColor[3];
            VEC4_MUL(source[0], quadColor[0], alpha); /* R */
            VEC4_MUL(source[1], quadColor[1], alpha); /* G */
            VEC4_MUL(source[2], quadColor[2], alpha); /* B */
         }
         break;
      case PIPE_BLENDFACTOR_DST_COLOR:
         VEC4_MUL(source[0], quadColor[0], dest[0]); /* R */
         VEC4_MUL(source[1], quadColor[1], dest[1]); /* G */
         VEC4_MUL(source[2], quadColor[2], dest[2]); /* B */
         break;
      case PIPE_BLENDFACTOR_DST_ALPHA:
         {
            const float *alpha = dest[3];
            VEC4_MUL(source[0], quadColor[0], alpha); /* R */
            VEC4_MUL(source[1], quadColor[1], alpha); /* G */
            VEC4_MUL(source[2], quadColor[2], alpha); /* B */
         }
         break;
      case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
         {
            const float *alpha = quadColor[3];
            float diff[4], temp[4];
            VEC4_SUB(diff, one, dest[3]);
            VEC4_MIN(temp, alpha, diff);
            VEC4_MUL(source[0], quadColor[0], temp); /* R */
            VEC4_MUL(source[1], quadColor[1], temp); /* G */
            VEC4_MUL(source[2], quadColor[2], temp); /* B */
         }
         break;
      case PIPE_BLENDFACTOR_CONST_COLOR:
         {
            float comp[4];
            VEC4_SCALAR(comp, softpipe->blend_color.color[0]); /* R */
            VEC4_MUL(source[0], quadColor[0], comp); /* R */
            VEC4_SCALAR(comp, softpipe->blend_color.color[1]); /* G */
            VEC4_MUL(source[1], quadColor[1], comp); /* G */
            VEC4_SCALAR(comp, softpipe->blend_color.color[2]); /* B */
            VEC4_MUL(source[2], quadColor[2], comp); /* B */
         }
         break;
      case PIPE_BLENDFACTOR_CONST_ALPHA:
         {
            float alpha[4];
            VEC4_SCALAR(alpha, softpipe->blend_color.color[3]);
            VEC4_MUL(source[0], quadColor[0], alpha); /* R */
            VEC4_MUL(source[1], quadColor[1], alpha); /* G */
            VEC4_MUL(source[2], quadColor[2], alpha); /* B */
         }
         break;
      case PIPE_BLENDFACTOR_SRC1_COLOR:
         assert(0); /* to do */
         break;
      case PIPE_BLENDFACTOR_SRC1_ALPHA:
         assert(0); /* to do */
         break;
      case PIPE_BLENDFACTOR_ZERO:
         VEC4_COPY(source[0], zero); /* R */
         VEC4_COPY(source[1], zero); /* G */
         VEC4_COPY(source[2], zero); /* B */
         break;
      case PIPE_BLENDFACTOR_INV_SRC_COLOR:
         {
            float inv_comp[4];
            VEC4_SUB(inv_comp, one, quadColor[0]); /* R */
            VEC4_MUL(source[0], quadColor[0], inv_comp); /* R */
            VEC4_SUB(inv_comp, one, quadColor[1]); /* G */
            VEC4_MUL(source[1], quadColor[1], inv_comp); /* G */
            VEC4_SUB(inv_comp, one, quadColor[2]); /* B */
            VEC4_MUL(source[2], quadColor[2], inv_comp); /* B */
         }
         break;
      case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
         {
            float inv_alpha[4];
            VEC4_SUB(inv_alpha, one, quadColor[3]);
            VEC4_MUL(source[0], quadColor[0], inv_alpha); /* R */
            VEC4_MUL(source[1], quadColor[1], inv_alpha); /* G */
            VEC4_MUL(source[2], quadColor[2], inv_alpha); /* B */
         }
         break;
      case PIPE_BLENDFACTOR_INV_DST_ALPHA:
         {
            float inv_alpha[4];
            VEC4_SUB(inv_alpha, one, dest[3]);
            VEC4_MUL(source[0], quadColor[0], inv_alpha); /* R */
            VEC4_MUL(source[1], quadColor[1], inv_alpha); /* G */
            VEC4_MUL(source[2], quadColor[2], inv_alpha); /* B */
         }
         break;
      case PIPE_BLENDFACTOR_INV_DST_COLOR:
         {
            float inv_comp[4];
            VEC4_SUB(inv_comp, one, dest[0]); /* R */
            VEC4_MUL(source[0], quadColor[0], inv_comp); /* R */
            VEC4_SUB(inv_comp, one, dest[1]); /* G */
            VEC4_MUL(source[1], quadColor[1], inv_comp); /* G */
            VEC4_SUB(inv_comp, one, dest[2]); /* B */
            VEC4_MUL(source[2], quadColor[2], inv_comp); /* B */
         }
         break;
      case PIPE_BLENDFACTOR_INV_CONST_COLOR:
         {
            float inv_comp[4];
            /* R */
            VEC4_SCALAR(inv_comp, 1.0f - softpipe->blend_color.color[0]);
            VEC4_MUL(source[0], quadColor[0], inv_comp);
            /* G */
            VEC4_SCALAR(inv_comp, 1.0f - softpipe->blend_color.color[1]);
            VEC4_MUL(source[1], quadColor[1], inv_comp);
            /* B */
            VEC4_SCALAR(inv_comp, 1.0f - softpipe->blend_color.color[2]);
            VEC4_MUL(source[2], quadColor[2], inv_comp);
         }
         break;
      case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
         {
            float inv_alpha[4];
            VEC4_SCALAR(inv_alpha, 1.0f - softpipe->blend_color.color[3]);
            VEC4_MUL(source[0], quadColor[0], inv_alpha); /* R */
            VEC4_MUL(source[1], quadColor[1], inv_alpha); /* G */
            VEC4_MUL(source[2], quadColor[2], inv_alpha); /* B */
         }
         break;
      case PIPE_BLENDFACTOR_INV_SRC1_COLOR:
         assert(0); /* to do */
         break;
      case PIPE_BLENDFACTOR_INV_SRC1_ALPHA:
         assert(0); /* to do */
         break;
      default:
         assert(0);
      }

      /*
       * Compute src/first term A
       */
      switch (softpipe->blend->alpha_src_factor) {
      case PIPE_BLENDFACTOR_ONE:
         VEC4_COPY(source[3], quadColor[3]); /* A */
         break;
      case PIPE_BLENDFACTOR_SRC_COLOR:
         /* fall-through */
      case PIPE_BLENDFACTOR_SRC_ALPHA:
         {
            const float *alpha = quadColor[3];
            VEC4_MUL(source[3], quadColor[3], alpha); /* A */
         }
         break;
      case PIPE_BLENDFACTOR_DST_COLOR:
         /* fall-through */
      case PIPE_BLENDFACTOR_DST_ALPHA:
         VEC4_MUL(source[3], quadColor[3], dest[3]); /* A */
         break;
      case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
         /* multiply alpha by 1.0 */
         VEC4_COPY(source[3], quadColor[3]); /* A */
         break;
      case PIPE_BLENDFACTOR_CONST_COLOR:
         /* fall-through */
      case PIPE_BLENDFACTOR_CONST_ALPHA:
         {
            float comp[4];
            VEC4_SCALAR(comp, softpipe->blend_color.color[3]); /* A */
            VEC4_MUL(source[3], quadColor[3], comp); /* A */
         }
         break;
      case PIPE_BLENDFACTOR_ZERO:
         VEC4_COPY(source[3], zero); /* A */
         break;
      case PIPE_BLENDFACTOR_INV_SRC_COLOR:
         /* fall-through */
      case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
         {
            float inv_alpha[4];
            VEC4_SUB(inv_alpha, one, quadColor[3]);
            VEC4_MUL(source[3], quadColor[3], inv_alpha); /* A */
         }
         break;
      case PIPE_BLENDFACTOR_INV_DST_COLOR:
         /* fall-through */
      case PIPE_BLENDFACTOR_INV_DST_ALPHA:
         {
            float inv_alpha[4];
            VEC4_SUB(inv_alpha, one, dest[3]);
            VEC4_MUL(source[3], quadColor[3], inv_alpha); /* A */
         }
         break;
      case PIPE_BLENDFACTOR_INV_CONST_COLOR:
         /* fall-through */
      case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
         {
            float inv_comp[4];
            /* A */
            VEC4_SCALAR(inv_comp, 1.0f - softpipe->blend_color.color[3]);
            VEC4_MUL(source[3], quadColor[3], inv_comp);
         }
         break;
      default:
         assert(0);
      }


      /*
       * Compute dest/second term RGB
       */
      switch (softpipe->blend->rgb_dst_factor) {
      case PIPE_BLENDFACTOR_ONE:
         /* dest = dest * 1   NO-OP, leave dest as-is */
         break;
      case PIPE_BLENDFACTOR_SRC_COLOR:
         VEC4_MUL(dest[0], dest[0], quadColor[0]); /* R */
         VEC4_MUL(dest[1], dest[1], quadColor[1]); /* G */
         VEC4_MUL(dest[2], dest[2], quadColor[2]); /* B */
         break;
      case PIPE_BLENDFACTOR_SRC_ALPHA:
         VEC4_MUL(dest[0], dest[0], quadColor[3]); /* R * A */
         VEC4_MUL(dest[1], dest[1], quadColor[3]); /* G * A */
         VEC4_MUL(dest[2], dest[2], quadColor[3]); /* B * A */
         break;
      case PIPE_BLENDFACTOR_DST_ALPHA:
         VEC4_MUL(dest[0], dest[0], dest[3]); /* R * A */
         VEC4_MUL(dest[1], dest[1], dest[3]); /* G * A */
         VEC4_MUL(dest[2], dest[2], dest[3]); /* B * A */
         break;
      case PIPE_BLENDFACTOR_DST_COLOR:
         VEC4_MUL(dest[0], dest[0], dest[0]); /* R */
         VEC4_MUL(dest[1], dest[1], dest[1]); /* G */
         VEC4_MUL(dest[2], dest[2], dest[2]); /* B */
         break;
      case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
         assert(0); /* illegal */
         break;
      case PIPE_BLENDFACTOR_CONST_COLOR:
         {
            float comp[4];
            VEC4_SCALAR(comp, softpipe->blend_color.color[0]); /* R */
            VEC4_MUL(dest[0], dest[0], comp); /* R */
            VEC4_SCALAR(comp, softpipe->blend_color.color[1]); /* G */
            VEC4_MUL(dest[1], dest[1], comp); /* G */
            VEC4_SCALAR(comp, softpipe->blend_color.color[2]); /* B */
            VEC4_MUL(dest[2], dest[2], comp); /* B */
         }
         break;
      case PIPE_BLENDFACTOR_CONST_ALPHA:
         {
            float comp[4];
            VEC4_SCALAR(comp, softpipe->blend_color.color[3]); /* A */
            VEC4_MUL(dest[0], dest[0], comp); /* R */
            VEC4_MUL(dest[1], dest[1], comp); /* G */
            VEC4_MUL(dest[2], dest[2], comp); /* B */
         }
         break;
      case PIPE_BLENDFACTOR_ZERO:
         VEC4_COPY(dest[0], zero); /* R */
         VEC4_COPY(dest[1], zero); /* G */
         VEC4_COPY(dest[2], zero); /* B */
         break;
      case PIPE_BLENDFACTOR_SRC1_COLOR:
      case PIPE_BLENDFACTOR_SRC1_ALPHA:
         /* XXX what are these? */
         assert(0);
         break;
      case PIPE_BLENDFACTOR_INV_SRC_COLOR:
         {
            float inv_comp[4];
            VEC4_SUB(inv_comp, one, quadColor[0]); /* R */
            VEC4_MUL(dest[0], inv_comp, dest[0]); /* R */
            VEC4_SUB(inv_comp, one, quadColor[1]); /* G */
            VEC4_MUL(dest[1], inv_comp, dest[1]); /* G */
            VEC4_SUB(inv_comp, one, quadColor[2]); /* B */
            VEC4_MUL(dest[2], inv_comp, dest[2]); /* B */
         }
         break;
      case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
         {
            float one_minus_alpha[QUAD_SIZE];
            VEC4_SUB(one_minus_alpha, one, quadColor[3]);
            VEC4_MUL(dest[0], dest[0], one_minus_alpha); /* R */
            VEC4_MUL(dest[1], dest[1], one_minus_alpha); /* G */
            VEC4_MUL(dest[2], dest[2], one_minus_alpha); /* B */
         }
         break;
      case PIPE_BLENDFACTOR_INV_DST_ALPHA:
         {
            float inv_comp[4];
            VEC4_SUB(inv_comp, one, dest[3]); /* A */
            VEC4_MUL(dest[0], inv_comp, dest[0]); /* R */
            VEC4_MUL(dest[1], inv_comp, dest[1]); /* G */
            VEC4_MUL(dest[2], inv_comp, dest[2]); /* B */
         }
         break;
      case PIPE_BLENDFACTOR_INV_DST_COLOR:
         {
            float inv_comp[4];
            VEC4_SUB(inv_comp, one, dest[0]); /* R */
            VEC4_MUL(dest[0], dest[0], inv_comp); /* R */
            VEC4_SUB(inv_comp, one, dest[1]); /* G */
            VEC4_MUL(dest[1], dest[1], inv_comp); /* G */
            VEC4_SUB(inv_comp, one, dest[2]); /* B */
            VEC4_MUL(dest[2], dest[2], inv_comp); /* B */
         }
         break;
      case PIPE_BLENDFACTOR_INV_CONST_COLOR:
         {
            float inv_comp[4];
            /* R */
            VEC4_SCALAR(inv_comp, 1.0f - softpipe->blend_color.color[0]);
            VEC4_MUL(dest[0], dest[0], inv_comp);
            /* G */
            VEC4_SCALAR(inv_comp, 1.0f - softpipe->blend_color.color[1]);
            VEC4_MUL(dest[1], dest[1], inv_comp);
            /* B */
            VEC4_SCALAR(inv_comp, 1.0f - softpipe->blend_color.color[2]);
            VEC4_MUL(dest[2], dest[2], inv_comp);
         }
         break;
      case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
         {
            float inv_comp[4];
            VEC4_SCALAR(inv_comp, 1.0f - softpipe->blend_color.color[3]);
            VEC4_MUL(dest[0], dest[0], inv_comp);
            VEC4_MUL(dest[1], dest[1], inv_comp);
            VEC4_MUL(dest[2], dest[2], inv_comp);
         }
         break;
      case PIPE_BLENDFACTOR_INV_SRC1_COLOR:
      case PIPE_BLENDFACTOR_INV_SRC1_ALPHA:
         /* XXX what are these? */
         assert(0);
         break;
      default:
         assert(0);
      }

      /*
       * Compute dest/second term A
       */
      switch (softpipe->blend->alpha_dst_factor) {
      case PIPE_BLENDFACTOR_ONE:
         /* dest = dest * 1   NO-OP, leave dest as-is */
         break;
      case PIPE_BLENDFACTOR_SRC_COLOR:
         /* fall-through */
      case PIPE_BLENDFACTOR_SRC_ALPHA:
         VEC4_MUL(dest[3], dest[3], quadColor[3]); /* A * A */
         break;
      case PIPE_BLENDFACTOR_DST_COLOR:
         /* fall-through */
      case PIPE_BLENDFACTOR_DST_ALPHA:
         VEC4_MUL(dest[3], dest[3], dest[3]); /* A */
         break;
      case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
         assert(0); /* illegal */
         break;
      case PIPE_BLENDFACTOR_CONST_COLOR:
         /* fall-through */
      case PIPE_BLENDFACTOR_CONST_ALPHA:
         {
            float comp[4];
            VEC4_SCALAR(comp, softpipe->blend_color.color[3]); /* A */
            VEC4_MUL(dest[3], dest[3], comp); /* A */
         }
         break;
      case PIPE_BLENDFACTOR_ZERO:
         VEC4_COPY(dest[3], zero); /* A */
         break;
      case PIPE_BLENDFACTOR_INV_SRC_COLOR:
         /* fall-through */
      case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
         {
            float one_minus_alpha[QUAD_SIZE];
            VEC4_SUB(one_minus_alpha, one, quadColor[3]);
            VEC4_MUL(dest[3], dest[3], one_minus_alpha); /* A */
         }
         break;
      case PIPE_BLENDFACTOR_INV_DST_COLOR:
         /* fall-through */
      case PIPE_BLENDFACTOR_INV_DST_ALPHA:
         {
            float inv_comp[4];
            VEC4_SUB(inv_comp, one, dest[3]); /* A */
            VEC4_MUL(dest[3], inv_comp, dest[3]); /* A */
         }
         break;
      case PIPE_BLENDFACTOR_INV_CONST_COLOR:
         /* fall-through */
      case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
         {
            float inv_comp[4];
            VEC4_SCALAR(inv_comp, 1.0f - softpipe->blend_color.color[3]);
            VEC4_MUL(dest[3], dest[3], inv_comp);
         }
         break;
      default:
         assert(0);
      }

      /*
       * Combine RGB terms
       */
      switch (softpipe->blend->rgb_func) {
      case PIPE_BLEND_ADD:
         VEC4_ADD(quadColor[0], source[0], dest[0]); /* R */
         VEC4_ADD(quadColor[1], source[1], dest[1]); /* G */
         VEC4_ADD(quadColor[2], source[2], dest[2]); /* B */
         break;
      case PIPE_BLEND_SUBTRACT:
         VEC4_SUB(quadColor[0], source[0], dest[0]); /* R */
         VEC4_SUB(quadColor[1], source[1], dest[1]); /* G */
         VEC4_SUB(quadColor[2], source[2], dest[2]); /* B */
         break;
      case PIPE_BLEND_REVERSE_SUBTRACT:
         VEC4_SUB(quadColor[0], dest[0], source[0]); /* R */
         VEC4_SUB(quadColor[1], dest[1], source[1]); /* G */
         VEC4_SUB(quadColor[2], dest[2], source[2]); /* B */
         break;
      case PIPE_BLEND_MIN:
         VEC4_MIN(quadColor[0], source[0], dest[0]); /* R */
         VEC4_MIN(quadColor[1], source[1], dest[1]); /* G */
         VEC4_MIN(quadColor[2], source[2], dest[2]); /* B */
         break;
      case PIPE_BLEND_MAX:
         VEC4_MAX(quadColor[0], source[0], dest[0]); /* R */
         VEC4_MAX(quadColor[1], source[1], dest[1]); /* G */
         VEC4_MAX(quadColor[2], source[2], dest[2]); /* B */
         break;
      default:
         assert(0);
      }

      /*
       * Combine A terms
       */
      switch (softpipe->blend->alpha_func) {
      case PIPE_BLEND_ADD:
         VEC4_ADD(quadColor[3], source[3], dest[3]); /* A */
         break;
      case PIPE_BLEND_SUBTRACT:
         VEC4_SUB(quadColor[3], source[3], dest[3]); /* A */
         break;
      case PIPE_BLEND_REVERSE_SUBTRACT:
         VEC4_SUB(quadColor[3], dest[3], source[3]); /* A */
         break;
      case PIPE_BLEND_MIN:
         VEC4_MIN(quadColor[3], source[3], dest[3]); /* A */
         break;
      case PIPE_BLEND_MAX:
         VEC4_MAX(quadColor[3], source[3], dest[3]); /* A */
         break;
      default:
         assert(0);
      }

   } /* cbuf loop */

   /* pass blended quad to next stage */
   qs->next->run(qs->next, quad);
}


static void blend_begin(struct quad_stage *qs)
{
   qs->next->begin(qs->next);
}


static void blend_destroy(struct quad_stage *qs)
{
   FREE( qs );
}


struct quad_stage *sp_quad_blend_stage( struct softpipe_context *softpipe )
{
   struct quad_stage *stage = CALLOC_STRUCT(quad_stage);

   stage->softpipe = softpipe;
   stage->begin = blend_begin;
   stage->run = blend_quad;
   stage->destroy = blend_destroy;

   return stage;
}

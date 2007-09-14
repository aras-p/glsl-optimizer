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
#include "pipe/p_util.h"
#include "sp_context.h"
#include "sp_headers.h"
#include "sp_surface.h"
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

#define VEC4_ADD(SUM, A, B) \
do { \
   SUM[0] = A[0] + B[0]; \
   SUM[1] = A[1] + B[1]; \
   SUM[2] = A[2] + B[2]; \
   SUM[3] = A[3] + B[3]; \
} while (0)

#define VEC4_SUB(SUM, A, B) \
do { \
   SUM[0] = A[0] - B[0]; \
   SUM[1] = A[1] - B[1]; \
   SUM[2] = A[2] - B[2]; \
   SUM[3] = A[3] - B[3]; \
} while (0)

#define VEC4_MUL(SUM, A, B) \
do { \
   SUM[0] = A[0] * B[0]; \
   SUM[1] = A[1] * B[1]; \
   SUM[2] = A[2] * B[2]; \
   SUM[3] = A[3] * B[3]; \
} while (0)

#define VEC4_MIN(SUM, A, B) \
do { \
   SUM[0] = (A[0] < B[0]) ? A[0] : B[0]; \
   SUM[1] = (A[1] < B[1]) ? A[1] : B[1]; \
   SUM[2] = (A[2] < B[2]) ? A[2] : B[2]; \
   SUM[3] = (A[3] < B[3]) ? A[3] : B[3]; \
} while (0)

#define VEC4_MAX(SUM, A, B) \
do { \
   SUM[0] = (A[0] > B[0]) ? A[0] : B[0]; \
   SUM[1] = (A[1] > B[1]) ? A[1] : B[1]; \
   SUM[2] = (A[2] > B[2]) ? A[2] : B[2]; \
   SUM[3] = (A[3] > B[3]) ? A[3] : B[3]; \
} while (0)



static void
blend_quad(struct quad_stage *qs, struct quad_header *quad)
{
   static const float zero[4] = { 0, 0, 0, 0 };
   static const float one[4] = { 1, 1, 1, 1 };
   struct softpipe_context *softpipe = qs->softpipe;
   struct softpipe_surface *sps = softpipe_surface(softpipe->cbuf);
   float source[4][QUAD_SIZE], dest[4][QUAD_SIZE];
   
   /* get colors from framebuffer */
   sps->read_quad_f_swz(sps, quad->x0, quad->y0, dest);

   /*
    * Compute src/first term RGB
    */
   switch (softpipe->blend->rgb_src_factor) {
   case PIPE_BLENDFACTOR_ONE:
      VEC4_COPY(source[0], quad->outputs.color[0]); /* R */
      VEC4_COPY(source[1], quad->outputs.color[1]); /* G */
      VEC4_COPY(source[2], quad->outputs.color[2]); /* B */
      break;
   case PIPE_BLENDFACTOR_SRC_COLOR:
      VEC4_MUL(source[0], quad->outputs.color[0], quad->outputs.color[0]); /* R */
      VEC4_MUL(source[1], quad->outputs.color[1], quad->outputs.color[1]); /* G */
      VEC4_MUL(source[2], quad->outputs.color[2], quad->outputs.color[2]); /* B */
      break;
   case PIPE_BLENDFACTOR_SRC_ALPHA:
      {
         const float *alpha = quad->outputs.color[3];
         VEC4_MUL(source[0], quad->outputs.color[0], alpha); /* R */
         VEC4_MUL(source[1], quad->outputs.color[1], alpha); /* G */
         VEC4_MUL(source[2], quad->outputs.color[2], alpha); /* B */
      }
      break;
   case PIPE_BLENDFACTOR_DST_COLOR:
      VEC4_MUL(source[0], quad->outputs.color[0], dest[0]); /* R */
      VEC4_MUL(source[1], quad->outputs.color[1], dest[1]); /* G */
      VEC4_MUL(source[2], quad->outputs.color[2], dest[2]); /* B */
      break;
   case PIPE_BLENDFACTOR_DST_ALPHA:
      {
         const float *alpha = dest[3];
         VEC4_MUL(source[0], quad->outputs.color[0], alpha); /* R */
         VEC4_MUL(source[1], quad->outputs.color[1], alpha); /* G */
         VEC4_MUL(source[2], quad->outputs.color[2], alpha); /* B */
      }
      break;
   case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
      assert(0); /* to do */
      break;
   case PIPE_BLENDFACTOR_CONST_COLOR:
      {
         float comp[4];
         VEC4_SCALAR(comp, softpipe->blend_color.color[0]); /* R */
         VEC4_MUL(source[0], quad->outputs.color[0], comp); /* R */
         VEC4_SCALAR(comp, softpipe->blend_color.color[1]); /* G */
         VEC4_MUL(source[1], quad->outputs.color[1], comp); /* G */
         VEC4_SCALAR(comp, softpipe->blend_color.color[2]); /* B */
         VEC4_MUL(source[2], quad->outputs.color[2], comp); /* B */
      }
      break;
   case PIPE_BLENDFACTOR_CONST_ALPHA:
      {
         float alpha[4];
         VEC4_SCALAR(alpha, softpipe->blend_color.color[3]);
         VEC4_MUL(source[0], quad->outputs.color[0], alpha); /* R */
         VEC4_MUL(source[1], quad->outputs.color[1], alpha); /* G */
         VEC4_MUL(source[2], quad->outputs.color[2], alpha); /* B */
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
         VEC4_SUB(inv_comp, one, quad->outputs.color[0]); /* R */
         VEC4_MUL(source[0], quad->outputs.color[0], inv_comp); /* R */
         VEC4_SUB(inv_comp, one, quad->outputs.color[1]); /* G */
         VEC4_MUL(source[1], quad->outputs.color[1], inv_comp); /* G */
         VEC4_SUB(inv_comp, one, quad->outputs.color[2]); /* B */
         VEC4_MUL(source[2], quad->outputs.color[2], inv_comp); /* B */
      }
      break;
   case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
      {
         float inv_alpha[4];
         VEC4_SUB(inv_alpha, one, quad->outputs.color[3]);
         VEC4_MUL(source[0], quad->outputs.color[0], inv_alpha); /* R */
         VEC4_MUL(source[1], quad->outputs.color[1], inv_alpha); /* G */
         VEC4_MUL(source[2], quad->outputs.color[2], inv_alpha); /* B */
      }
      break;
   case PIPE_BLENDFACTOR_INV_DST_ALPHA:
      {
         float inv_alpha[4];
         VEC4_SUB(inv_alpha, one, dest[3]);
         VEC4_MUL(source[0], quad->outputs.color[0], inv_alpha); /* R */
         VEC4_MUL(source[1], quad->outputs.color[1], inv_alpha); /* G */
         VEC4_MUL(source[2], quad->outputs.color[2], inv_alpha); /* B */
      }
      break;
   case PIPE_BLENDFACTOR_INV_DST_COLOR:
      {
         float inv_comp[4];
         VEC4_SUB(inv_comp, one, dest[0]); /* R */
         VEC4_MUL(source[0], quad->outputs.color[0], inv_comp); /* R */
         VEC4_SUB(inv_comp, one, dest[1]); /* G */
         VEC4_MUL(source[1], quad->outputs.color[1], inv_comp); /* G */
         VEC4_SUB(inv_comp, one, dest[2]); /* B */
         VEC4_MUL(source[2], quad->outputs.color[2], inv_comp); /* B */
      }
      break;
   case PIPE_BLENDFACTOR_INV_CONST_COLOR:
      {
         float inv_comp[4];
         /* R */
         VEC4_SCALAR(inv_comp, 1.0f - softpipe->blend_color.color[0]);
         VEC4_MUL(source[0], quad->outputs.color[0], inv_comp);
         /* G */
         VEC4_SCALAR(inv_comp, 1.0f - softpipe->blend_color.color[1]);
         VEC4_MUL(source[1], quad->outputs.color[1], inv_comp);
         /* B */
         VEC4_SCALAR(inv_comp, 1.0f - softpipe->blend_color.color[2]);
         VEC4_MUL(source[2], quad->outputs.color[2], inv_comp);
      }
      break;
   case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
      {
         float inv_alpha[4];
         VEC4_SCALAR(inv_alpha, 1.0f - softpipe->blend_color.color[3]);
         VEC4_MUL(source[0], quad->outputs.color[0], inv_alpha); /* R */
         VEC4_MUL(source[1], quad->outputs.color[1], inv_alpha); /* G */
         VEC4_MUL(source[2], quad->outputs.color[2], inv_alpha); /* B */
      }
      break;
   case PIPE_BLENDFACTOR_INV_SRC1_COLOR:
      assert(0); /* to do */
      break;
   case PIPE_BLENDFACTOR_INV_SRC1_ALPHA:
      assert(0); /* to do */
      break;
   default:
      abort();
   }
   
   /*
    * Compute src/first term A
    */
   switch (softpipe->blend->alpha_src_factor) {
   case PIPE_BLENDFACTOR_ONE:
      VEC4_COPY(source[3], quad->outputs.color[3]); /* A */
      break;
   case PIPE_BLENDFACTOR_SRC_ALPHA:
      {
         const float *alpha = quad->outputs.color[3];
         VEC4_MUL(source[3], quad->outputs.color[3], alpha); /* A */
      }
      break;
   case PIPE_BLENDFACTOR_ZERO:
      VEC4_COPY(source[3], zero); /* A */
      break;
      /* XXX fill in remaining terms */
   default:
      abort();
   }
   
   
   /*
    * Compute dest/second term RGB
    */
   switch (softpipe->blend->rgb_dst_factor) {
   case PIPE_BLENDFACTOR_ONE:
      /* dest = dest * 1   NO-OP, leave dest as-is */
      break;
   case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
      {
         float one_minus_alpha[QUAD_SIZE];
         VEC4_SUB(one_minus_alpha, one, quad->outputs.color[3]);
         VEC4_MUL(dest[0], dest[0], one_minus_alpha); /* R */
         VEC4_MUL(dest[1], dest[1], one_minus_alpha); /* G */
         VEC4_MUL(dest[2], dest[2], one_minus_alpha); /* B */
      }
      break;
   case PIPE_BLENDFACTOR_ZERO:
      VEC4_COPY(dest[0], zero); /* R */
      VEC4_COPY(dest[1], zero); /* G */
      VEC4_COPY(dest[2], zero); /* B */
      break;
      /* XXX fill in remaining terms */
   default:
      abort();
   }
   
   /*
    * Compute dest/second term A
    */
   switch (softpipe->blend->alpha_dst_factor) {
   case PIPE_BLENDFACTOR_ONE:
      /* dest = dest * 1   NO-OP, leave dest as-is */
      break;
   case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
      {
         float one_minus_alpha[QUAD_SIZE];
         VEC4_SUB(one_minus_alpha, one, quad->outputs.color[3]);
         VEC4_MUL(dest[3], dest[3], one_minus_alpha); /* A */
      }
      break;
   case PIPE_BLENDFACTOR_ZERO:
      VEC4_COPY(dest[3], zero); /* A */
      break;
      /* XXX fill in remaining terms */
   default:
      abort();
   }
   
   /*
    * Combine RGB terms
    */
   switch (softpipe->blend->rgb_func) {
   case PIPE_BLEND_ADD:
      VEC4_ADD(quad->outputs.color[0], source[0], dest[0]); /* R */
      VEC4_ADD(quad->outputs.color[1], source[1], dest[1]); /* G */
      VEC4_ADD(quad->outputs.color[2], source[2], dest[2]); /* B */
      break;
   case PIPE_BLEND_SUBTRACT:
      VEC4_SUB(quad->outputs.color[0], source[0], dest[0]); /* R */
      VEC4_SUB(quad->outputs.color[1], source[1], dest[1]); /* G */
      VEC4_SUB(quad->outputs.color[2], source[2], dest[2]); /* B */
      break;
   case PIPE_BLEND_REVERSE_SUBTRACT:
      VEC4_SUB(quad->outputs.color[0], dest[0], source[0]); /* R */
      VEC4_SUB(quad->outputs.color[1], dest[1], source[1]); /* G */
      VEC4_SUB(quad->outputs.color[2], dest[2], source[2]); /* B */
      break;
   case PIPE_BLEND_MIN:
      VEC4_MIN(quad->outputs.color[0], source[0], dest[0]); /* R */
      VEC4_MIN(quad->outputs.color[1], source[1], dest[1]); /* G */
      VEC4_MIN(quad->outputs.color[2], source[2], dest[2]); /* B */
      break;
   case PIPE_BLEND_MAX:
      VEC4_MAX(quad->outputs.color[0], source[0], dest[0]); /* R */
      VEC4_MAX(quad->outputs.color[1], source[1], dest[1]); /* G */
      VEC4_MAX(quad->outputs.color[2], source[2], dest[2]); /* B */
      break;
   default:
      abort();
   }
   
   /*
    * Combine A terms
    */
   switch (softpipe->blend->alpha_func) {
   case PIPE_BLEND_ADD:
      VEC4_ADD(quad->outputs.color[3], source[3], dest[3]); /* A */
      break;
   case PIPE_BLEND_SUBTRACT:
      VEC4_SUB(quad->outputs.color[3], source[3], dest[3]); /* A */
      break;
   case PIPE_BLEND_REVERSE_SUBTRACT:
      VEC4_SUB(quad->outputs.color[3], dest[3], source[3]); /* A */
      break;
   case PIPE_BLEND_MIN:
      VEC4_MIN(quad->outputs.color[3], source[3], dest[3]); /* A */
      break;
   case PIPE_BLEND_MAX:
      VEC4_MAX(quad->outputs.color[3], source[3], dest[3]); /* A */
   default:
      abort();
   }
   
   /* pass blended quad to next stage */
   qs->next->run(qs->next, quad);
}


static void blend_begin(struct quad_stage *qs)
{
   if (qs->next)
      qs->next->begin(qs->next);
}


struct quad_stage *sp_quad_blend_stage( struct softpipe_context *softpipe )
{
   struct quad_stage *stage = CALLOC_STRUCT(quad_stage);

   stage->softpipe = softpipe;
   stage->begin = blend_begin;
   stage->run = blend_quad;

   return stage;
}

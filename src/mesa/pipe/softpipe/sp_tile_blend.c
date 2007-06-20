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

#include "glheader.h"
#include "imports.h"
#include "macros.h"
#include "pipe/p_defines.h"
#include "sp_context.h"
#include "sp_headers.h"
#include "sp_surface.h"
#include "sp_tile.h"



static void
blend_quad(struct quad_stage *qs, struct quad_header *quad)
{
   struct softpipe_context *softpipe = qs->softpipe;
   GLfloat source[4][QUAD_SIZE], dest[4][QUAD_SIZE];
   GLuint i, j;

   /* XXX we're also looping in output_quad() !?! */

   /* copy quad's colors since we'll modify them in the loop */
   memcpy(source, quad->outputs.color, 4 * 4 * sizeof(GLfloat));

   for (i = 0; i < softpipe->framebuffer.num_cbufs; i++) {
      GLfloat srcTerm[4], dstTerm[4];
      struct softpipe_surface *sps
         = softpipe_surface(softpipe->framebuffer.cbufs[i]);
   
      sps->read_quad_f_swz(sps, quad->x0, quad->y0, dest);

      /* XXX this loop could be factored out - we'd compute the src/dstTerm
       * for all four pixels in the quad at once.
       */
      for (j = 0; j < QUAD_SIZE; j++) {
         switch (softpipe->blend.rgb_src_factor) {
         case PIPE_BLENDFACTOR_ONE:
            srcTerm[0] = srcTerm[1] = srcTerm[2] = 1.0;
            break;
         case PIPE_BLENDFACTOR_SRC_ALPHA:
            srcTerm[0] = srcTerm[1] = srcTerm[2] = quad->outputs.color[3][j];
            break;
         case PIPE_BLENDFACTOR_ZERO:
            srcTerm[0] = srcTerm[1] = srcTerm[2] = 0.0;
            break;
            /* XXX fill in remaining terms */
         default:
            abort();
         }

         switch (softpipe->blend.alpha_src_factor) {
         case PIPE_BLENDFACTOR_ONE:
            srcTerm[3] = 1.0;
            break;
         case PIPE_BLENDFACTOR_SRC_ALPHA:
            srcTerm[3] = quad->outputs.color[3][j];
            break;
         case PIPE_BLENDFACTOR_ZERO:
            srcTerm[3] = 0.0;
            break;
            /* XXX fill in remaining terms */
         default:
            abort();
         }

         switch (softpipe->blend.rgb_dst_factor) {
         case PIPE_BLENDFACTOR_ONE:
            dstTerm[0] = dstTerm[1] = dstTerm[2] = 1.0;
            break;
         case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
            dstTerm[0] = dstTerm[1] = dstTerm[2] = 1.0 - quad->outputs.color[3][j];
            break;
         case PIPE_BLENDFACTOR_ZERO:
            dstTerm[0] = dstTerm[1] = dstTerm[2] = 0.0;
            break;
            /* XXX fill in remaining terms */
         default:
            abort();
         }

         switch (softpipe->blend.alpha_dst_factor) {
         case PIPE_BLENDFACTOR_ONE:
            dstTerm[3] = 1.0;
            break;
         case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
            dstTerm[3] = 1.0 - quad->outputs.color[3][j];
            break;
         case PIPE_BLENDFACTOR_ZERO:
            dstTerm[3] = 0.0;
            break;
            /* XXX fill in remaining terms */
         default:
            abort();
         }

         switch (softpipe->blend.rgb_func) {
         case PIPE_BLEND_ADD:
            quad->outputs.color[0][j] = source[0][j] * srcTerm[0] + dest[0][j] * dstTerm[0];
            quad->outputs.color[1][j] = source[1][j] * srcTerm[1] + dest[1][j] * dstTerm[1];
            quad->outputs.color[2][j] = source[2][j] * srcTerm[2] + dest[2][j] * dstTerm[2];
            quad->outputs.color[3][j] = source[3][j] * srcTerm[3] + dest[3][j] * dstTerm[3];
            break;
         case PIPE_BLEND_SUBTRACT:
            quad->outputs.color[0][j] = source[0][j] * srcTerm[0] - dest[0][j] * dstTerm[0];
            quad->outputs.color[1][j] = source[1][j] * srcTerm[1] - dest[1][j] * dstTerm[1];
            quad->outputs.color[2][j] = source[2][j] * srcTerm[2] - dest[2][j] * dstTerm[2];
            quad->outputs.color[3][j] = source[3][j] * srcTerm[3] - dest[3][j] * dstTerm[3];
            break;
         case PIPE_BLEND_REVERSE_SUBTRACT:
            quad->outputs.color[0][j] = dest[0][j] * dstTerm[0] - source[0][j] * srcTerm[0];
            quad->outputs.color[1][j] = dest[1][j] * dstTerm[1] - source[1][j] * srcTerm[1];
            quad->outputs.color[2][j] = dest[2][j] * dstTerm[2] - source[2][j] * srcTerm[2];
            quad->outputs.color[3][j] = dest[3][j] * dstTerm[3] - source[3][j] * srcTerm[3];
            break;
         case PIPE_BLEND_MIN:
            quad->outputs.color[0][j] = MIN2(dest[0][j], source[0][j]);
            quad->outputs.color[1][j] = MIN2(dest[1][j], source[1][j]);
            quad->outputs.color[2][j] = MIN2(dest[2][j], source[2][j]);
            quad->outputs.color[3][j] = MIN2(dest[3][j], source[3][j]);
            break;
         case PIPE_BLEND_MAX:
            quad->outputs.color[0][j] = MAX2(dest[0][j], source[0][j]);
            quad->outputs.color[1][j] = MAX2(dest[1][j], source[1][j]);
            quad->outputs.color[2][j] = MAX2(dest[2][j], source[2][j]);
            quad->outputs.color[3][j] = MAX2(dest[3][j], source[3][j]);
            break;
         default:
            abort();
         }
      } /* loop over quads */

      /* pass blended quad to next stage */
      qs->next->run(qs->next, quad);
   }
}




struct quad_stage *sp_quad_blend_stage( struct softpipe_context *softpipe )
{
   struct quad_stage *stage = CALLOC_STRUCT(quad_stage);

   stage->softpipe = softpipe;
   stage->run = blend_quad;

   return stage;
}

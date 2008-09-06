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
 * \brief  Apply AA coverage to quad alpha valus
 * \author  Brian Paul
 */


#include "pipe/p_defines.h"
#include "util/u_memory.h"
#include "sp_context.h"
#include "sp_headers.h"
#include "sp_quad.h"


/**
 * Multiply quad's alpha values by the fragment coverage.
 */
static void
coverage_quad(struct quad_stage *qs, struct quad_header *quad)
{
   struct softpipe_context *softpipe = qs->softpipe;

   if ((softpipe->rasterizer->poly_smooth && quad->input.prim == PRIM_TRI) ||
       (softpipe->rasterizer->line_smooth && quad->input.prim == PRIM_LINE) ||
       (softpipe->rasterizer->point_smooth && quad->input.prim == PRIM_POINT)) {
      uint cbuf;

      /* loop over colorbuffer outputs */
      for (cbuf = 0; cbuf < softpipe->framebuffer.num_cbufs; cbuf++) {
         float (*quadColor)[4] = quad->output.color[cbuf];
         unsigned j;
         for (j = 0; j < QUAD_SIZE; j++) {
            assert(quad->input.coverage[j] >= 0.0);
            assert(quad->input.coverage[j] <= 1.0);
         quadColor[3][j] *= quad->input.coverage[j];
         }
      }
   }

   qs->next->run(qs->next, quad);
}


static void coverage_begin(struct quad_stage *qs)
{
   qs->next->begin(qs->next);
}


static void coverage_destroy(struct quad_stage *qs)
{
   FREE( qs );
}


struct quad_stage *sp_quad_coverage_stage( struct softpipe_context *softpipe )
{
   struct quad_stage *stage = CALLOC_STRUCT(quad_stage);

   stage->softpipe = softpipe;
   stage->begin = coverage_begin;
   stage->run = coverage_quad;
   stage->destroy = coverage_destroy;

   return stage;
}

/**************************************************************************
 * 
 * Copyright 2008-2009 VMware, Inc.
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

/* Vertices are just an array of floats, with all the attributes
 * packed.  We currently assume a layout like:
 *
 * attr[0][0..3] - window position
 * attr[1..n][0..3] - remaining attributes.
 *
 * Attributes are assumed to be 4 floats wide but are packed so that
 * all the enabled attributes run contiguously.
 */

#include "util/u_math.h"
#include "util/u_memory.h"
#include "pipe/p_defines.h"
#include "pipe/p_screen.h"

#include "lp_context.h"
#include "lp_state.h"
#include "lp_quad.h"
#include "lp_quad_pipe.h"
#include "lp_texture.h"


struct quad_shade_stage
{
   struct quad_stage stage;  /**< base class */

   struct pipe_transfer *transfer;
   uint8_t *map;
};


/** cast wrapper */
static INLINE struct quad_shade_stage *
quad_shade_stage(struct quad_stage *qs)
{
   return (struct quad_shade_stage *) qs;
}



/**
 * Execute fragment shader for the four fragments in the quad.
 */
static boolean
shade_quad(struct quad_stage *qs, struct quad_header *quad)
{
   struct quad_shade_stage *qss = quad_shade_stage( qs );
   struct llvmpipe_context *llvmpipe = qs->llvmpipe;
   struct lp_fragment_shader *fs = llvmpipe->fs;
   void *constants;
   struct tgsi_sampler **samplers;
   const unsigned x = quad->input.x0;
   const unsigned y = quad->input.y0;
   void *depth;
   uint32_t ALIGN16_ATTRIB mask[NUM_CHANNELS];
   unsigned chan_index;

   assert(fs->current);
   if(!fs->current)
      return FALSE;

   constants = llvmpipe->mapped_constants[PIPE_SHADER_FRAGMENT];
   samplers = (struct tgsi_sampler **)llvmpipe->tgsi.frag_samplers_list;

   for (chan_index = 0; chan_index < NUM_CHANNELS; ++chan_index)
      mask[chan_index] = quad->inout.mask & (1 << chan_index) ? ~0 : 0;

   if(qss->map) {
      assert((x % 2) == 0);
      assert((y % 2) == 0);
      depth = qss->map +
              y*qss->transfer->stride +
              2*x*qss->transfer->block.size;
   }
   else
      depth = NULL;

   assert((((uintptr_t)mask) & 0xf) == 0);
   assert((((uintptr_t)quad->output.color) & 0xf) == 0);
   assert((((uintptr_t)depth) & 0xf) == 0);

   /* run shader */
   fs->current->jit_function( x,
                              y,
                              quad->coef->a0,
                              quad->coef->dadx,
                              quad->coef->dady,
                              constants,
                              mask,
                              quad->output.color,
                              depth,
                              samplers);

   for (chan_index = 0; chan_index < NUM_CHANNELS; ++chan_index)
      if(!mask[chan_index])
         quad->inout.mask &= ~(1 << chan_index);

   if (quad->inout.mask == 0)
      return FALSE;

   return TRUE;
}



static void
coverage_quad(struct quad_stage *qs, struct quad_header *quad)
{
   struct llvmpipe_context *llvmpipe = qs->llvmpipe;
   uint cbuf;

   /* loop over colorbuffer outputs */
   for (cbuf = 0; cbuf < llvmpipe->framebuffer.nr_cbufs; cbuf++) {
      float (*quadColor)[4] = quad->output.color[cbuf];
      unsigned j;
      for (j = 0; j < QUAD_SIZE; j++) {
         assert(quad->input.coverage[j] >= 0.0);
         assert(quad->input.coverage[j] <= 1.0);
         quadColor[3][j] *= quad->input.coverage[j];
      }
   }
}



static void
shade_quads(struct quad_stage *qs, 
                 struct quad_header *quads[],
                 unsigned nr)
{
   unsigned i, pass = 0;
   
   for (i = 0; i < nr; i++) {
      if(!quads[i]->inout.mask)
         continue;

      if (!shade_quad(qs, quads[i]))
         continue;

      if (/*do_coverage*/ 0)
         coverage_quad( qs, quads[i] );

      ++pass;
   }
   
   if (pass)
      qs->next->run(qs->next, quads, nr);
}
   




/**
 * Per-primitive (or per-begin?) setup
 */
static void
shade_begin(struct quad_stage *qs)
{
   struct quad_shade_stage *qss = quad_shade_stage( qs );
   struct llvmpipe_context *llvmpipe = qs->llvmpipe;
   struct pipe_screen *screen = llvmpipe->pipe.screen;
   struct pipe_surface *zsbuf = llvmpipe->framebuffer.zsbuf;

   if(qss->transfer) {
      if(qss->map) {
         screen->transfer_unmap(screen, qss->transfer);
         qss->map = NULL;
      }

      screen->tex_transfer_destroy(qss->transfer);
      qss->transfer = NULL;
   }

   if(zsbuf) {
      qss->transfer = screen->get_tex_transfer(screen, zsbuf->texture,
                                               zsbuf->face, zsbuf->level, zsbuf->zslice,
                                               PIPE_TRANSFER_READ_WRITE,
                                               0, 0, zsbuf->width, zsbuf->height);
      if(qss->transfer)
         qss->map = screen->transfer_map(screen, qss->transfer);

   }

   qs->next->begin(qs->next);
}


static void
shade_destroy(struct quad_stage *qs)
{
   struct quad_shade_stage *qss = quad_shade_stage( qs );
   struct llvmpipe_context *llvmpipe = qs->llvmpipe;
   struct pipe_screen *screen = llvmpipe->pipe.screen;

   if(qss->transfer) {
      if(qss->map) {
         screen->transfer_unmap(screen, qss->transfer);
         qss->map = NULL;
      }

      screen->tex_transfer_destroy(qss->transfer);
      qss->transfer = NULL;
   }

   align_free( qs );
}


struct quad_stage *
lp_quad_shade_stage( struct llvmpipe_context *llvmpipe )
{
   struct quad_shade_stage *qss;

   qss = align_malloc(sizeof(struct quad_shade_stage), 16);
   if (!qss)
      return NULL;

   memset(qss, 0, sizeof *qss);

   qss->stage.llvmpipe = llvmpipe;
   qss->stage.begin = shade_begin;
   qss->stage.run = shade_quads;
   qss->stage.destroy = shade_destroy;

   return &qss->stage;
}

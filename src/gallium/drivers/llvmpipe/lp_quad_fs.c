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
#include "pipe/p_shader_tokens.h"

#include "lp_context.h"
#include "lp_state.h"
#include "lp_quad.h"
#include "lp_quad_pipe.h"
#include "lp_texture.h"
#include "lp_tex_sample.h"


struct quad_shade_stage
{
   struct quad_stage stage;  /**< base class */

   union tgsi_exec_channel ALIGN16_ATTRIB pos[NUM_CHANNELS];
   union tgsi_exec_channel ALIGN16_ATTRIB a0[PIPE_MAX_SHADER_INPUTS][NUM_CHANNELS];
   union tgsi_exec_channel ALIGN16_ATTRIB dadx[PIPE_MAX_SHADER_INPUTS][NUM_CHANNELS];
   union tgsi_exec_channel ALIGN16_ATTRIB dady[PIPE_MAX_SHADER_INPUTS][NUM_CHANNELS];

   struct tgsi_exec_vector ALIGN16_ATTRIB outputs[PIPE_MAX_ATTRIBS];
};


/** cast wrapper */
static INLINE struct quad_shade_stage *
quad_shade_stage(struct quad_stage *qs)
{
   return (struct quad_shade_stage *) qs;
}


static void
setup_pos_vector(struct quad_shade_stage *qss,
                 const struct tgsi_interp_coef *coef,
                 float x, float y)
{
   uint chan;

   /* do X */
   qss->pos[0].f[0] = x;
   qss->pos[0].f[1] = x + 1;
   qss->pos[0].f[2] = x;
   qss->pos[0].f[3] = x + 1;

   /* do Y */
   qss->pos[1].f[0] = y;
   qss->pos[1].f[1] = y;
   qss->pos[1].f[2] = y + 1;
   qss->pos[1].f[3] = y + 1;

   /* do Z and W for all fragments in the quad */
   for (chan = 2; chan < 4; chan++) {
      const float dadx = coef->dadx[chan];
      const float dady = coef->dady[chan];
      const float a0 = coef->a0[chan] + dadx * x + dady * y;
      qss->pos[chan].f[0] = a0;
      qss->pos[chan].f[1] = a0 + dadx;
      qss->pos[chan].f[2] = a0 + dady;
      qss->pos[chan].f[3] = a0 + dadx + dady;
   }
}


static void
setup_coef_vector(struct quad_shade_stage *qss,
                  const struct tgsi_interp_coef *coef)
{
   unsigned num_inputs = qss->stage.llvmpipe->fs->info.num_inputs;
   unsigned attrib, chan, i;

   for (attrib = 0; attrib < num_inputs; ++attrib) {
      for (chan = 0; chan < NUM_CHANNELS; ++chan) {
         for( i = 0; i < QUAD_SIZE; ++i ) {
            qss->a0[attrib][chan].f[i] = coef[attrib].a0[chan];
            qss->dadx[attrib][chan].f[i] = coef[attrib].dadx[chan];
            qss->dady[attrib][chan].f[i] = coef[attrib].dady[chan];
         }
      }
   }
}


/**
 * Execute fragment shader for the four fragments in the quad.
 */
static boolean
shade_quad(struct quad_stage *qs, struct quad_header *quad)
{
   struct quad_shade_stage *qss = quad_shade_stage( qs );
   struct llvmpipe_context *llvmpipe = qs->llvmpipe;
   void *constants;
   struct tgsi_sampler **samplers;
   boolean z_written;

   /* Compute X, Y, Z, W vals for this quad */
   setup_pos_vector(qss,
                    quad->posCoef,
                    (float)quad->input.x0, (float)quad->input.y0);


   constants = llvmpipe->mapped_constants[PIPE_SHADER_FRAGMENT];
   samplers = (struct tgsi_sampler **)llvmpipe->tgsi.frag_samplers_list;

   /* run shader */
   llvmpipe->fs->jit_function( qss->pos,
                               qss->a0, qss->dadx, qss->dady,
                               constants,
                               qss->outputs,
                               samplers);

   /* FIXME */
#if 0
   quad->inout.mask &= ... ;
   if (quad->inout.mask == 0)
      return FALSE;
#endif

   /* store outputs */
   z_written = FALSE;
   {
      const ubyte *sem_name = llvmpipe->fs->info.output_semantic_name;
      const ubyte *sem_index = llvmpipe->fs->info.output_semantic_index;
      const uint n = qss->stage.llvmpipe->fs->info.num_outputs;
      uint i;
      for (i = 0; i < n; i++) {
         switch (sem_name[i]) {
         case TGSI_SEMANTIC_COLOR:
            {
               uint cbuf = sem_index[i];
               memcpy(quad->output.color[cbuf],
                      &qss->outputs[i].xyzw[0].f[0],
                      sizeof(quad->output.color[0]) );
            }
            break;
         case TGSI_SEMANTIC_POSITION:
            {
               uint j;
               for (j = 0; j < 4; j++) {
                  quad->output.depth[j] = qss->outputs[0].xyzw[2].f[j];
               }
               z_written = TRUE;
            }
            break;
         }
      }
   }

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
   struct quad_shade_stage *qss = quad_shade_stage( qs );
   unsigned i, pass = 0;
   
   setup_coef_vector(qss,
                     quads[0]->coef);

   for (i = 0; i < nr; i++) {
      if (!shade_quad(qs, quads[i]))
         continue;

      if (/*do_coverage*/ 0)
         coverage_quad( qs, quads[i] );

      quads[pass++] = quads[i];
   }
   
   if (pass)
      qs->next->run(qs->next, quads, pass);
}
   




/**
 * Per-primitive (or per-begin?) setup
 */
static void
shade_begin(struct quad_stage *qs)
{
   qs->next->begin(qs->next);
}


static void
shade_destroy(struct quad_stage *qs)
{
   FREE( qs );
}


struct quad_stage *
lp_quad_shade_stage( struct llvmpipe_context *llvmpipe )
{
   struct quad_shade_stage *qss;

   qss = CALLOC_STRUCT(quad_shade_stage);
   if (!qss)
      return NULL;

   qss->stage.llvmpipe = llvmpipe;
   qss->stage.begin = shade_begin;
   qss->stage.run = shade_quads;
   qss->stage.destroy = shade_destroy;

   return &qss->stage;
}

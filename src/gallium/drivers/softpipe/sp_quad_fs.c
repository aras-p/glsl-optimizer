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

#include "sp_context.h"
#include "sp_state.h"
#include "sp_headers.h"
#include "sp_quad.h"
#include "sp_texture.h"
#include "sp_tex_sample.h"


struct quad_shade_stage
{
   struct quad_stage stage;
   struct tgsi_sampler samplers[PIPE_MAX_SAMPLERS];
   struct tgsi_exec_machine machine;
   struct tgsi_exec_vector *inputs, *outputs;
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
static void
shade_quad(
   struct quad_stage *qs,
   struct quad_header *quad )
{
   struct quad_shade_stage *qss = quad_shade_stage( qs );
   struct softpipe_context *softpipe = qs->softpipe;
   struct tgsi_exec_machine *machine = &qss->machine;
   boolean z_written;
   
   /* Consts do not require 16 byte alignment. */
   machine->Consts = softpipe->mapped_constants[PIPE_SHADER_FRAGMENT];

   machine->InterpCoefs = quad->coef;

   /* run shader */
   quad->inout.mask &= softpipe->fs->run( softpipe->fs, 
				    &qss->machine,
				    quad );

   /* store outputs */
   z_written = FALSE;
   {
      const ubyte *sem_name = softpipe->fs->info.output_semantic_name;
      const ubyte *sem_index = softpipe->fs->info.output_semantic_index;
      const uint n = qss->stage.softpipe->fs->info.num_outputs;
      uint i;
      for (i = 0; i < n; i++) {
         switch (sem_name[i]) {
         case TGSI_SEMANTIC_COLOR:
            {
               uint cbuf = sem_index[i];
               memcpy(quad->output.color[cbuf],
                      &machine->Outputs[i].xyzw[0].f[0],
                      sizeof(quad->output.color[0]) );
            }
            break;
         case TGSI_SEMANTIC_POSITION:
            {
               uint j;
               for (j = 0; j < 4; j++) {
                  quad->output.depth[j] = machine->Outputs[0].xyzw[2].f[j];
               }
               z_written = TRUE;
            }
            break;
         }
      }
   }

   if (!z_written) {
      /* compute Z values now, as in the quad earlyz stage */
      /* XXX we should really only do this if the earlyz stage is not used */
      const float fx = (float) quad->input.x0;
      const float fy = (float) quad->input.y0;
      const float dzdx = quad->posCoef->dadx[2];
      const float dzdy = quad->posCoef->dady[2];
      const float z0 = quad->posCoef->a0[2] + dzdx * fx + dzdy * fy;

      quad->output.depth[0] = z0;
      quad->output.depth[1] = z0 + dzdx;
      quad->output.depth[2] = z0 + dzdy;
      quad->output.depth[3] = z0 + dzdx + dzdy;
   }

   /* shader may cull fragments */
   if( quad->inout.mask ) {
      qs->next->run( qs->next, quad );
   }
}

/**
 * Per-primitive (or per-begin?) setup
 */
static void shade_begin(struct quad_stage *qs)
{
   struct quad_shade_stage *qss = quad_shade_stage(qs);
   struct softpipe_context *softpipe = qs->softpipe;
   unsigned i;
   unsigned num = MAX2(softpipe->num_textures, softpipe->num_samplers);

   /* set TGSI sampler state that varies */
   for (i = 0; i < num; i++) {
      qss->samplers[i].state = softpipe->sampler[i];
      qss->samplers[i].texture = softpipe->texture[i];
   }

   softpipe->fs->prepare( softpipe->fs, 
			  &qss->machine,
			  qss->samplers );

   qs->next->begin(qs->next);
}


static void shade_destroy(struct quad_stage *qs)
{
   struct quad_shade_stage *qss = (struct quad_shade_stage *) qs;

   tgsi_exec_machine_free_data(&qss->machine);
   FREE( qss->inputs );
   FREE( qss->outputs );
   FREE( qs );
}


struct quad_stage *sp_quad_shade_stage( struct softpipe_context *softpipe )
{
   struct quad_shade_stage *qss = CALLOC_STRUCT(quad_shade_stage);
   uint i;

   /* allocate storage for program inputs/outputs, aligned to 16 bytes */
   qss->inputs = MALLOC(PIPE_MAX_ATTRIBS * sizeof(*qss->inputs) + 16);
   qss->outputs = MALLOC(PIPE_MAX_ATTRIBS * sizeof(*qss->outputs) + 16);
   qss->machine.Inputs = align16(qss->inputs);
   qss->machine.Outputs = align16(qss->outputs);

   qss->stage.softpipe = softpipe;
   qss->stage.begin = shade_begin;
   qss->stage.run = shade_quad;
   qss->stage.destroy = shade_destroy;

   /* set TGSI sampler state that's constant */
   for (i = 0; i < PIPE_MAX_SAMPLERS; i++) {
      assert(softpipe->tex_cache[i]);
      qss->samplers[i].get_samples = sp_get_samples;
      qss->samplers[i].pipe = &softpipe->pipe;
      qss->samplers[i].cache = softpipe->tex_cache[i];
   }

   tgsi_exec_machine_init( &qss->machine );

   return &qss->stage;
}

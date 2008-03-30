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

#include "pipe/p_util.h"
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
   int colorOutSlot, depthOutSlot;
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

   /* Consts do not require 16 byte alignment. */
   machine->Consts = softpipe->mapped_constants[PIPE_SHADER_FRAGMENT];

   machine->InterpCoefs = quad->coef;

   /* run shader */
   quad->mask &= softpipe->fs->run( softpipe->fs, 
				    &qss->machine,
				    quad );

   /* store result color */
   if (qss->colorOutSlot >= 0) {
      /* XXX need to handle multiple color outputs someday */
      assert(qss->stage.softpipe->fs->info.output_semantic_name[qss->colorOutSlot]
             == TGSI_SEMANTIC_COLOR);
      memcpy(
             quad->outputs.color,
             &machine->Outputs[qss->colorOutSlot].xyzw[0].f[0],
             sizeof( quad->outputs.color ) );
   }

   /*
    * XXX the following code for updating quad->outputs.depth
    * isn't really needed if we did early z testing.
    */

   /* store result Z */
   if (qss->depthOutSlot >= 0) {
      /* output[slot] is new Z */
      uint i;
      for (i = 0; i < 4; i++) {
         quad->outputs.depth[i] = machine->Outputs[0].xyzw[2].f[i];
      }
   }
   else {
      /* compute Z values now, as in the quad earlyz stage */
      /* XXX we should really only do this if the earlyz stage is not used */
      const float fx = (float) quad->x0;
      const float fy = (float) quad->y0;
      const float dzdx = quad->posCoef->dadx[2];
      const float dzdy = quad->posCoef->dady[2];
      const float z0 = quad->posCoef->a0[2] + dzdx * fx + dzdy * fy;

      quad->outputs.depth[0] = z0;
      quad->outputs.depth[1] = z0 + dzdx;
      quad->outputs.depth[2] = z0 + dzdy;
      quad->outputs.depth[3] = z0 + dzdx + dzdy;
   }

   /* shader may cull fragments */
   if( quad->mask ) {
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

   /* find output slots for depth, color */
   qss->colorOutSlot = -1;
   qss->depthOutSlot = -1;
   for (i = 0; i < qss->stage.softpipe->fs->info.num_outputs; i++) {
      switch (qss->stage.softpipe->fs->info.output_semantic_name[i]) {
      case TGSI_SEMANTIC_POSITION:
         qss->depthOutSlot = i;
         break;
      case TGSI_SEMANTIC_COLOR:
         qss->colorOutSlot = i;
         break;
      }
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

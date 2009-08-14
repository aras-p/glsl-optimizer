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
   struct tgsi_exec_machine *machine;
   struct tgsi_exec_vector *inputs, *outputs;
};


/** cast wrapper */
static INLINE struct quad_shade_stage *
quad_shade_stage(struct quad_stage *qs)
{
   return (struct quad_shade_stage *) qs;
}


static void
shader_prepare( const struct lp_fragment_shader *shader,
                struct tgsi_exec_machine *machine,
                struct tgsi_sampler **samplers )
{
   /*
    * Bind tokens/shader to the interpreter's machine state.
    * Avoid redundant binding.
    */
   if (machine->Tokens != shader->base.tokens) {
      tgsi_exec_machine_bind_shader( machine,
                                     shader->base.tokens,
                                     PIPE_MAX_SAMPLERS,
                                     samplers );
   }
}



static void
setup_pos_vector(struct lp_fragment_shader *shader,
                 const struct tgsi_interp_coef *coef,
                 float x, float y)
{
   uint chan;

   /* do X */
   shader->pos[0].f[0] = x;
   shader->pos[0].f[1] = x + 1;
   shader->pos[0].f[2] = x;
   shader->pos[0].f[3] = x + 1;

   /* do Y */
   shader->pos[1].f[0] = y;
   shader->pos[1].f[1] = y;
   shader->pos[1].f[2] = y + 1;
   shader->pos[1].f[3] = y + 1;

   /* do Z and W for all fragments in the quad */
   for (chan = 2; chan < 4; chan++) {
      const float dadx = coef->dadx[chan];
      const float dady = coef->dady[chan];
      const float a0 = coef->a0[chan] + dadx * x + dady * y;
      shader->pos[chan].f[0] = a0;
      shader->pos[chan].f[1] = a0 + dadx;
      shader->pos[chan].f[2] = a0 + dady;
      shader->pos[chan].f[3] = a0 + dadx + dady;
   }
}


static void
setup_coef_vector(struct lp_fragment_shader *shader,
                  const struct tgsi_interp_coef *coef)
{
   unsigned attrib, chan, i;

   for (attrib = 0; attrib < PIPE_MAX_SHADER_INPUTS; ++attrib) {
      for (chan = 0; chan < NUM_CHANNELS; ++chan) {
         for( i = 0; i < QUAD_SIZE; ++i ) {
            shader->a0[attrib][chan].f[i] = coef[attrib].a0[chan];
            shader->dadx[attrib][chan].f[i] = coef[attrib].dadx[chan];
            shader->dady[attrib][chan].f[i] = coef[attrib].dady[chan];
         }
      }
   }
}


/* TODO: codegenerate the whole run function, skip this wrapper.
 * TODO: break dependency on tgsi_exec_machine struct
 * TODO: push Position calculation into the generated shader
 * TODO: process >1 quad at a time
 */
static unsigned
shader_run( struct lp_fragment_shader *shader,
            struct tgsi_exec_machine *machine,
            struct quad_header *quad )
{
   unsigned mask;

   /* Compute X, Y, Z, W vals for this quad */
   setup_pos_vector(shader,
                    quad->posCoef,
                    (float)quad->input.x0, (float)quad->input.y0);

   setup_coef_vector(shader,
                     quad->coef);

   /* init kill mask */
   tgsi_set_kill_mask(machine, 0x0);
   tgsi_set_exec_mask(machine, 1, 1, 1, 1);

   memset(machine->Outputs, 0, sizeof machine->Outputs);

   shader->jit_function( shader->pos,
                         shader->a0, shader->dadx, shader->dady,
                         machine->Consts,
                         machine->Outputs,
                         machine->Samplers);

   /* FIXME */
   mask = ~0;

   return mask;
}


/**
 * Execute fragment shader for the four fragments in the quad.
 */
static boolean
shade_quad(struct quad_stage *qs, struct quad_header *quad)
{
   struct quad_shade_stage *qss = quad_shade_stage( qs );
   struct llvmpipe_context *llvmpipe = qs->llvmpipe;
   struct tgsi_exec_machine *machine = qss->machine;
   boolean z_written;

   /* run shader */
   quad->inout.mask &= shader_run( llvmpipe->fs, machine, quad );
   if (quad->inout.mask == 0)
      return FALSE;

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
   struct llvmpipe_context *llvmpipe = qs->llvmpipe;
   struct tgsi_exec_machine *machine = qss->machine;

   unsigned i, pass = 0;
   
   machine->Consts = llvmpipe->mapped_constants[PIPE_SHADER_FRAGMENT];
   machine->InterpCoefs = quads[0]->coef;

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
   struct quad_shade_stage *qss = quad_shade_stage(qs);
   struct llvmpipe_context *llvmpipe = qs->llvmpipe;

   shader_prepare( llvmpipe->fs,
                   qss->machine,
                   (struct tgsi_sampler **)llvmpipe->tgsi.frag_samplers_list );

   qs->next->begin(qs->next);
}


static void
shade_destroy(struct quad_stage *qs)
{
   struct quad_shade_stage *qss = (struct quad_shade_stage *) qs;

   tgsi_exec_machine_destroy(qss->machine);

   FREE( qs );
}


struct quad_stage *
lp_quad_shade_stage( struct llvmpipe_context *llvmpipe )
{
   struct quad_shade_stage *qss = CALLOC_STRUCT(quad_shade_stage);
   if (!qss)
      goto fail;

   qss->stage.llvmpipe = llvmpipe;
   qss->stage.begin = shade_begin;
   qss->stage.run = shade_quads;
   qss->stage.destroy = shade_destroy;

   qss->machine = tgsi_exec_machine_create();
   if (!qss->machine)
      goto fail;

   return &qss->stage;

fail:
   if (qss && qss->machine)
      tgsi_exec_machine_destroy(qss->machine);

   FREE(qss);
   return NULL;
}

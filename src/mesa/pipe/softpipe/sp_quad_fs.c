/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

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

#include "sp_context.h"
#include "sp_headers.h"
#include "sp_quad.h"
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


#if !defined(XSTDCALL) 
#if defined(WIN32)
#define XSTDCALL __stdcall
#else
#define XSTDCALL
#endif
#endif

typedef void (XSTDCALL *codegen_function)(
   const struct tgsi_exec_vector *input,
   struct tgsi_exec_vector *output,
   float (*constant)[4],
   struct tgsi_exec_vector *temporary,
   const struct tgsi_interp_coef *coef );

/* This should be done by the fragment shader execution unit (code
 * generated from the decl instructions).  Do it here for now.
 */
static void
shade_quad(
   struct quad_stage *qs,
   struct quad_header *quad )
{
   struct quad_shade_stage *qss = quad_shade_stage( qs );
   struct softpipe_context *softpipe = qs->softpipe;
   const float fx = (float) quad->x0;
   const float fy = (float) quad->y0;
   struct tgsi_exec_machine *machine = &qss->machine;

   /* Consts does not require 16 byte alignment. */
   machine->Consts = softpipe->mapped_constants[PIPE_SHADER_FRAGMENT];

   machine->InterpCoefs = quad->coef;

   machine->Inputs[0].xyzw[0].f[0] = fx;
   machine->Inputs[0].xyzw[0].f[1] = fx + 1.0f;
   machine->Inputs[0].xyzw[0].f[2] = fx;
   machine->Inputs[0].xyzw[0].f[3] = fx + 1.0f;

   machine->Inputs[0].xyzw[1].f[0] = fy;
   machine->Inputs[0].xyzw[1].f[1] = fy;
   machine->Inputs[0].xyzw[1].f[2] = fy + 1.0f;
   machine->Inputs[0].xyzw[1].f[3] = fy + 1.0f;

   /* run shader */
   /* XXX: Generated code effectively unusable until it handles quad->mask */
   if( !quad->mask && softpipe->fs->executable != NULL ) {
      codegen_function func = (codegen_function) softpipe->fs->executable;
      func(
         machine->Inputs,
         machine->Outputs,
         machine->Consts,
         machine->Temps,
         machine->InterpCoefs );
   }
   else {
      quad->mask &= tgsi_exec_machine_run( machine );
   }

   /* store result color (always in output[1]) */
   memcpy(
      quad->outputs.color,
      &machine->Outputs[1].xyzw[0].f[0],
      sizeof( quad->outputs.color ) );

   /* Z */
   if (qss->stage.softpipe->fs->output_semantic_name[0]
       == TGSI_SEMANTIC_POSITION) {
      /* output[0] is new Z */
      uint i;
      for (i = 0; i < 4; i++) {
         quad->outputs.depth[i] = machine->Outputs[0].xyzw[2].f[i];
      }
   }
   else {
      /* pass input Z to output Z */
      uint i;
      for (i = 0; i < 4; i++) {
         quad->outputs.depth[i] = machine->Inputs[0].xyzw[2].f[i];
      }
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
   unsigned i, entry;

   for (i = 0; i < PIPE_MAX_SAMPLERS; i++) {
      qss->samplers[i].state = softpipe->sampler[i];
      qss->samplers[i].texture = softpipe->texture[i];
      qss->samplers[i].get_samples = sp_get_samples;
      qss->samplers[i].pipe = &softpipe->pipe;
      /* init cache info here */
      for (entry = 0; entry < TEX_CACHE_NUM_ENTRIES; entry++) {
         qss->samplers[i].cache[entry].x = -1;
         qss->samplers[i].cache[entry].y = -1;
         qss->samplers[i].cache[entry].level = -1;
         qss->samplers[i].cache[entry].face = -1;
         qss->samplers[i].cache[entry].zslice = -1;
      }
   }

   /* XXX only do this if the fragment shader changes... */
   tgsi_exec_machine_init(&qss->machine,
                          softpipe->fs->tokens,
                          PIPE_MAX_SAMPLERS,
                          qss->samplers );

   if (qs->next)
      qs->next->begin(qs->next);
}


struct quad_stage *sp_quad_shade_stage( struct softpipe_context *softpipe )
{
   struct quad_shade_stage *qss = CALLOC_STRUCT(quad_shade_stage);

   /* allocate storage for program inputs/outputs, aligned to 16 bytes */
   qss->inputs = malloc(PIPE_ATTRIB_MAX * sizeof(*qss->inputs) + 16);
   qss->outputs = malloc(PIPE_ATTRIB_MAX * sizeof(*qss->outputs) + 16);
   qss->machine.Inputs = align16(qss->inputs);
   qss->machine.Outputs = align16(qss->outputs);

   qss->stage.softpipe = softpipe;
   qss->stage.begin = shade_begin;
   qss->stage.run = shade_quad;

   return &qss->stage;
}

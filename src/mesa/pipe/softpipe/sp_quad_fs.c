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

#include "sp_context.h"
#include "sp_headers.h"
#include "sp_quad.h"
#include "sp_tex_sample.h"


#if defined __GNUC__
#define USE_ALIGNED_ATTRIBS   1
#define ALIGN16_SUFFIX        __attribute__(( aligned( 16 ) ))
#else
#define USE_ALIGNED_ATTRIBS   0
#define ALIGN16_SUFFIX
#endif


struct quad_shade_stage
{
   struct quad_stage stage;
   struct tgsi_sampler samplers[PIPE_MAX_SAMPLERS];
};


/** cast wrapper */
static INLINE struct quad_shade_stage *
quad_shade_stage(struct quad_stage *qs)
{
   return (struct quad_shade_stage *) qs;
}

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
   struct tgsi_exec_machine machine;

#if USE_ALIGNED_ATTRIBS
   struct tgsi_exec_vector inputs[PIPE_ATTRIB_MAX] ALIGN16_SUFFIX;
   struct tgsi_exec_vector outputs[PIPE_ATTRIB_MAX] ALIGN16_SUFFIX;
#else
   struct tgsi_exec_vector inputs[PIPE_ATTRIB_MAX + 1];
   struct tgsi_exec_vector outputs[PIPE_ATTRIB_MAX + 1];
#endif

#ifdef DEBUG
   memset( &machine, 0, sizeof( machine ) );
#endif

   /* init machine state */
   tgsi_exec_machine_init(
      &machine,
      softpipe->fs.tokens,
      PIPE_MAX_SAMPLERS,
      qss->samplers );

   /* Consts does not require 16 byte alignment. */
   machine.Consts = softpipe->fs.constants->constant;

#if USE_ALIGNED_ATTRIBS
   machine.Inputs = inputs;
   machine.Outputs = outputs;
#else
   machine.Inputs = (struct tgsi_exec_vector *) tgsi_align_128bit( inputs );
   machine.Outputs = (struct tgsi_exec_vector *) tgsi_align_128bit( outputs );
#endif

   machine.InterpCoefs = quad->coef;

   machine.Inputs[0].xyzw[0].f[0] = fx;
   machine.Inputs[0].xyzw[0].f[1] = fx + 1.0f;
   machine.Inputs[0].xyzw[0].f[2] = fx;
   machine.Inputs[0].xyzw[0].f[3] = fx + 1.0f;

   machine.Inputs[0].xyzw[1].f[0] = fy;
   machine.Inputs[0].xyzw[1].f[1] = fy;
   machine.Inputs[0].xyzw[1].f[2] = fy + 1.0f;
   machine.Inputs[0].xyzw[1].f[3] = fy + 1.0f;

   /* run shader */
   tgsi_exec_machine_run( &machine );

   /* store result color */
   memcpy(
      quad->outputs.color,
      &machine.Outputs[1].xyzw[0].f[0],
      sizeof( quad->outputs.color ) );

   if( softpipe->need_z ) {
      /* XXX temporary */
      memcpy(
         quad->outputs.depth,
         &machine.Outputs[0].xyzw[2],
         sizeof( quad->outputs.depth ) );
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
      qss->samplers[i].state = &softpipe->sampler[i];
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

   if (qs->next)
      qs->next->begin(qs->next);
}


struct quad_stage *sp_quad_shade_stage( struct softpipe_context *softpipe )
{
   struct quad_shade_stage *stage = CALLOC_STRUCT(quad_shade_stage);

   stage->stage.softpipe = softpipe;
   stage->stage.begin = shade_begin;
   stage->stage.run = shade_quad;

   return &stage->stage;
}

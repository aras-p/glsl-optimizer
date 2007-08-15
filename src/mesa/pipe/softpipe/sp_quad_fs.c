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
#include "tgsi/core/tgsi_core.h"

#include "sp_context.h"
#include "sp_headers.h"
#include "sp_quad.h"
#include "sp_tex_sample.h"

#include "main/mtypes.h"


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



struct exec_machine {
   const struct setup_coefficient *coef; /**< will point to quad->coef */
   float attr[PIPE_ATTRIB_MAX][NUM_CHANNELS][QUAD_SIZE] ALIGN16_SUFFIX;
};


/**
 * Compute quad's attributes values, as constants (GL_FLAT shading).
 */
static INLINE void cinterp( struct exec_machine *exec,
			    unsigned attrib,
			    unsigned i )
{
   unsigned j;

   for (j = 0; j < QUAD_SIZE; j++) {
      exec->attr[attrib][i][j] = exec->coef[attrib].a0[i];
   }
}


/**
 * Compute quad's attribute values by linear interpolation.
 *
 * Push into the fp:
 * 
 *   INPUT[attr] = MAD COEF_A0[attr], COEF_DADX[attr], INPUT_WPOS.xxxx
 *   INPUT[attr] = MAD INPUT[attr],   COEF_DADY[attr], INPUT_WPOS.yyyy
 */
static INLINE void linterp( struct exec_machine *exec,
			    unsigned attrib,
			    unsigned i )
{
   unsigned j;

   for (j = 0; j < QUAD_SIZE; j++) {
      const float x = exec->attr[FRAG_ATTRIB_WPOS][0][j];
      const float y = exec->attr[FRAG_ATTRIB_WPOS][1][j];
      exec->attr[attrib][i][j] = (exec->coef[attrib].a0[i] +
				  exec->coef[attrib].dadx[i] * x + 
				  exec->coef[attrib].dady[i] * y);
   }
}


/**
 * Compute quad's attribute values by linear interpolation with
 * perspective correction.
 *
 * Push into the fp:
 * 
 *   INPUT[attr] = MAD COEF_DADX[attr], INPUT_WPOS.xxxx, COEF_A0[attr]
 *   INPUT[attr] = MAD COEF_DADY[attr], INPUT_WPOS.yyyy, INPUT[attr]
 *   TMP         = RCP INPUT_WPOS.w
 *   INPUT[attr] = MUL INPUT[attr], TMP.xxxx
 *
 */
static INLINE void pinterp( struct exec_machine *exec,
			    unsigned attrib,
			    unsigned i )
{
   unsigned j;

   for (j = 0; j < QUAD_SIZE; j++) {
      const float x = exec->attr[FRAG_ATTRIB_WPOS][0][j];
      const float y = exec->attr[FRAG_ATTRIB_WPOS][1][j];
      /* FRAG_ATTRIB_WPOS.w here is really 1/w */
      const float w = 1.0 / exec->attr[FRAG_ATTRIB_WPOS][3][j];
      exec->attr[attrib][i][j] = ((exec->coef[attrib].a0[i] +
				   exec->coef[attrib].dadx[i] * x + 
				   exec->coef[attrib].dady[i] * y) * w);
   }
}


/* This should be done by the fragment shader execution unit (code
 * generated from the decl instructions).  Do it here for now.
 */
static void
shade_quad( struct quad_stage *qs, struct quad_header *quad )
{
   struct quad_shade_stage *qss = quad_shade_stage(qs);
   struct softpipe_context *softpipe = qs->softpipe;
   struct exec_machine exec;
   const float fx = quad->x0;
   const float fy = quad->y0;
   unsigned attr, i;
   struct tgsi_exec_machine machine;

#if USE_ALIGNED_ATTRIBS
   struct tgsi_exec_vector outputs[FRAG_ATTRIB_MAX] ALIGN16_SUFFIX;
#else
   struct tgsi_exec_vector inputs[FRAG_ATTRIB_MAX + 1];
   struct tgsi_exec_vector outputs[FRAG_ATTRIB_MAX + 1];
#endif

   exec.coef = quad->coef;

   /* Position:
    */
   exec.attr[FRAG_ATTRIB_WPOS][0][0] = fx;
   exec.attr[FRAG_ATTRIB_WPOS][0][1] = fx + 1.0;
   exec.attr[FRAG_ATTRIB_WPOS][0][2] = fx;
   exec.attr[FRAG_ATTRIB_WPOS][0][3] = fx + 1.0;

   exec.attr[FRAG_ATTRIB_WPOS][1][0] = fy;
   exec.attr[FRAG_ATTRIB_WPOS][1][1] = fy;
   exec.attr[FRAG_ATTRIB_WPOS][1][2] = fy + 1.0;
   exec.attr[FRAG_ATTRIB_WPOS][1][3] = fy + 1.0;

   /* Z and W are done by linear interpolation */
   if (softpipe->need_z) {
      linterp(&exec, 0, 2);   /* attr[0].z */
   }

   if (softpipe->need_w) {
      linterp(&exec, 0, 3);  /* attr[0].w */
      /*invert(&exec, 0, 3);*/
   }

   /* Interpolate all the remaining attributes.  This will get pushed
    * into the fragment program's responsibilities at some point.
    * Start at 1 to skip fragment position attribute (computed above).
    */
   for (attr = 1; attr < quad->nr_attrs; attr++) {
      switch (softpipe->interp[attr]) {
      case INTERP_CONSTANT:
	 for (i = 0; i < NUM_CHANNELS; i++)
	    cinterp(&exec, attr, i);
	 break;

      case INTERP_LINEAR:
	 for (i = 0; i < NUM_CHANNELS; i++)
	    linterp(&exec, attr, i);
	 break;

      case INTERP_PERSPECTIVE:
	 for (i = 0; i < NUM_CHANNELS; i++)
	    pinterp(&exec, attr, i);
	 break;
      }
   }

#ifdef DEBUG
   memset( &machine, 0, sizeof( machine ) );
#endif

   assert( sizeof( struct tgsi_exec_vector ) == sizeof( exec.attr[0] ) );

   /* init machine state */
   tgsi_exec_machine_init(
      &machine,
      softpipe->fs.tokens,
      PIPE_MAX_SAMPLERS,
      qss->samplers );

   /* Consts does not require 16 byte alignment. */
   machine.Consts = softpipe->fs.constants->constant;

#if USE_ALIGNED_ATTRIBS
   machine.Inputs = (struct tgsi_exec_vector *) exec.attr;
   machine.Outputs = outputs;
#else
   machine.Inputs = (struct tgsi_exec_vector *) tgsi_align_128bit( inputs );
   machine.Outputs = (struct tgsi_exec_vector *) tgsi_align_128bit( outputs );

   memcpy(
      machine.Inputs,
      exec.attr,
      softpipe->nr_attrs * sizeof( struct tgsi_exec_vector ) );
#endif

   /* run shader */
   tgsi_exec_machine_run( &machine );

   /* store result color */
   memcpy(
      quad->outputs.color,
      &machine.Outputs[FRAG_ATTRIB_COL0].xyzw[0].f[0],
      sizeof( quad->outputs.color ) );

   if( softpipe->need_z ) {
      /* XXX temporary */
      quad->outputs.depth[0] = exec.attr[0][2][0];
      quad->outputs.depth[1] = exec.attr[0][2][1];
      quad->outputs.depth[2] = exec.attr[0][2][2];
      quad->outputs.depth[3] = exec.attr[0][2][3];
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

/* $Id: t_pipeline.c,v 1.2 2000/11/20 18:06:13 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 * 
 * Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
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

/* Dynamic pipelines, support for CVA.
 * Copyright (C) 1999 Keith Whitwell.
 */

#include "glheader.h"
#include "context.h"
#include "mem.h"
#include "mmath.h"
#include "state.h"
#include "types.h"

#include "math/m_translate.h"
#include "math/m_xform.h"

#include "t_bbox.h"
#include "t_clip.h"
#include "t_cva.h"
#include "t_fog.h"
#include "t_light.h"
#include "t_pipeline.h"
#include "t_shade.h"
#include "t_stages.h"
#include "t_vbcull.h"
#include "t_vbindirect.h"
#include "t_vbrender.h"
#include "t_vbxform.h"





void gl_print_pipe_ops( const char *msg, GLuint flags )
{
   fprintf(stderr, 
	   "%s: (0x%x) %s%s%s%s%s%s%s%s%s%s%s%s\n",
	   msg,
	   flags,
	   (flags & PIPE_OP_CVA_PREPARE)   ? "cva-prepare, " : "",
	   (flags & PIPE_OP_VERT_XFORM)    ? "vert-xform, " : "",
	   (flags & PIPE_OP_NORM_XFORM)    ? "norm-xform, " : "",
	   (flags & PIPE_OP_LIGHT)         ? "light, " : "",
	   (flags & PIPE_OP_FOG)           ? "fog, " : "",
	   (flags & PIPE_OP_TEX0)          ? "tex-0, " : "",
	   (flags & PIPE_OP_TEX1)          ? "tex-1, " : "",
	   (flags & PIPE_OP_TEX2)          ? "tex-2, " : "",
	   (flags & PIPE_OP_TEX3)          ? "tex-3, " : "",
	   (flags & PIPE_OP_RAST_SETUP_0)  ? "rast-0, " : "",
	   (flags & PIPE_OP_RAST_SETUP_1)  ? "rast-1, " : "",
	   (flags & PIPE_OP_RENDER)        ? "render, " : "");

}



/* Have to reset only those parts of the vb which are being recalculated.
 */
void gl_reset_cva_vb( struct vertex_buffer *VB, GLuint stages )
{
   GLcontext *ctx = VB->ctx;
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   if (MESA_VERBOSE&VERBOSE_PIPELINE)
      gl_print_pipe_ops( "reset cva vb", stages ); 

   if (stages & PIPE_OP_VERT_XFORM) 
   {
      if (VB->ClipOrMask & CLIP_USER_BIT)
	 MEMSET(VB->UserClipMask, 0, VB->Count);

      VB->ClipOrMask = 0;
      VB->ClipAndMask = CLIP_ALL_BITS;
      VB->CullMode = 0;
      VB->CullFlag[0] = VB->CullFlag[1] = 0;
      VB->Culled = 0;
   }

   if (stages & PIPE_OP_NORM_XFORM) {
      VB->NormalPtr = &tnl->CVA.v.Normal;
   }

   if (stages & PIPE_OP_LIGHT) 
   {
      VB->ColorPtr = VB->Color[0] = VB->Color[1] = &tnl->CVA.v.Color;
      VB->IndexPtr = VB->Index[0] = VB->Index[1] = &tnl->CVA.v.Index;
   }
   else if (stages & PIPE_OP_FOG) 
   {
      if (ctx->Light.Enabled) {
	 VB->Color[0] = VB->LitColor[0];
	 VB->Color[1] = VB->LitColor[1];      
	 VB->Index[0] = VB->LitIndex[0];
	 VB->Index[1] = VB->LitIndex[1];      
      } else {
	 VB->Color[0] = VB->Color[1] = &tnl->CVA.v.Color;
	 VB->Index[0] = VB->Index[1] = &tnl->CVA.v.Index;
      }
      VB->ColorPtr = VB->Color[0];
      VB->IndexPtr = VB->Index[0];
   }
}






static void pipeline_ctr( struct gl_pipeline *p, GLcontext *ctx, GLuint type )
{
   GLuint i;
   (void) ctx;

   p->state_change = 0;
   p->cva_state_change = 0;
   p->inputs = 0;
   p->outputs = 0;
   p->type = type;
   p->ops = 0;

   for (i = 0 ; i < gl_default_nr_stages ; i++) 
      p->state_change |= gl_default_pipeline[i].state_change;
}


void _tnl_pipeline_init( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   MEMCPY( tnl->PipelineStage, 
	   gl_default_pipeline, 
	   sizeof(*gl_default_pipeline) * gl_default_nr_stages );
   
   tnl->NrPipelineStages = gl_default_nr_stages;
      
   pipeline_ctr( &tnl->CVA.elt, ctx, PIPE_IMMEDIATE);
   pipeline_ctr( &tnl->CVA.pre, ctx, PIPE_PRECALC );
}



#define MINIMAL_VERT_DATA (VERT_DATA & ~(VERT_TEX0_4 | \
                                         VERT_TEX1_4 | \
                                         VERT_TEX2_4 | \
                                         VERT_TEX3_4 | \
                                         VERT_EVAL_ANY))

#define VERT_CURRENT_DATA (VERT_TEX0_1234 | \
                           VERT_TEX1_1234 | \
                           VERT_TEX2_1234 | \
                           VERT_TEX3_1234 | \
                           VERT_RGBA | \
                           VERT_SPEC_RGB | \
                           VERT_FOG_COORD | \
			   VERT_INDEX | \
                           VERT_EDGE | \
                           VERT_NORM | \
	                   VERT_MATERIAL)

/* Called prior to every recomputation of the CVA precalc data, except where
 * the driver is able to calculate the pipeline unassisted.
 */
static void build_full_precalc_pipeline( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct gl_pipeline_stage *pipeline = tnl->PipelineStage;
   struct gl_cva *cva = &tnl->CVA;
   struct gl_pipeline *pre = &cva->pre;   
   struct gl_pipeline_stage **stages = pre->stages;
   GLuint i;
   GLuint newstate = pre->new_state;
   GLuint changed_ops = 0;
   GLuint oldoutputs = pre->outputs;
   GLuint oldinputs = pre->inputs;
   GLuint fallback = (VERT_CURRENT_DATA & tnl->_CurrentFlag & 
		      ~tnl->_ArraySummary);
   GLuint changed_outputs = (tnl->_ArrayNewState | 
			     (fallback & cva->orflag));
   GLuint available = fallback | tnl->_ArrayFlags;

   pre->cva_state_change = 0;
   pre->ops = 0;
   pre->outputs = 0;
   pre->inputs = 0;
   pre->forbidden_inputs = 0;
   pre->fallback = 0;

   /* KW: Disable data reuse during Mesa reorg.  Make this more readable...
    */
   newstate = ~0;

   if (tnl->_ArraySummary & VERT_ELT) 
      cva->orflag &= VERT_MATERIAL;
  
   cva->orflag &= ~(tnl->_ArraySummary & ~VERT_OBJ_ANY);
   available &= ~cva->orflag;
   
   pre->outputs = available;
   pre->inputs = available;

   if (MESA_VERBOSE & VERBOSE_PIPELINE) {
      fprintf(stderr, ": Rebuild pipeline\n");
      gl_print_vert_flags("orflag", cva->orflag);
   }
      
   

   /* If something changes in the pipeline, tag all subsequent stages
    * using this value for recalcuation.  Also used to build the full
    * pipeline by setting newstate and newinputs to ~0.
    *
    * Because all intermediate values are buffered, the new inputs
    * are enough to fully specify what needs to be calculated, and a
    * single pass identifies all stages requiring recalculation.
    */
   for (i = 0 ; i < tnl->NrPipelineStages ; i++) 
   {
      pipeline[i].check(ctx, &pipeline[i]);

      if (pipeline[i].type & PIPE_PRECALC) 
      {
	 if ((newstate & pipeline[i].cva_state_change) ||
	     (changed_outputs & pipeline[i].inputs) ||
	     !pipeline[i].inputs)
	 {	    
	    changed_ops |= pipeline[i].ops;
	    changed_outputs |= pipeline[i].outputs;
	    pipeline[i].active &= ~PIPE_PRECALC;

	    if ((pipeline[i].inputs & ~available) == 0 &&
		(pipeline[i].ops & pre->ops) == 0)
	    {
	       pipeline[i].active |= PIPE_PRECALC;
	       *stages++ = &pipeline[i];
	    } 
	 }
      
	 /* Incompatible with multiple stages structs implementing
	  * the same stage.
	  */
	 available &= ~pipeline[i].outputs;
	 pre->outputs &= ~pipeline[i].outputs;

	 if (pipeline[i].active & PIPE_PRECALC) {
	    pre->ops |= pipeline[i].ops;
	    pre->outputs |= pipeline[i].outputs;
	    available |= pipeline[i].outputs;
	    pre->forbidden_inputs |= pipeline[i].pre_forbidden_inputs;
	 }
      } 
      else if (pipeline[i].active & PIPE_PRECALC) 
      {
	 pipeline[i].active &= ~PIPE_PRECALC;
	 changed_outputs |= pipeline[i].outputs;
	 changed_ops |= pipeline[i].ops;
      }
   }

   *stages = 0;

   pre->new_outputs = pre->outputs & (changed_outputs | ~oldoutputs);
   pre->new_inputs = pre->inputs & ~oldinputs;
   pre->fallback = pre->inputs & fallback;
   pre->forbidden_inputs |= pre->inputs & fallback;

   pre->changed_ops = changed_ops;
}

void gl_build_precalc_pipeline( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct gl_pipeline *pre = &tnl->CVA.pre;   
   struct gl_pipeline *elt = &tnl->CVA.elt;   

   if (!ctx->Driver.BuildPrecalcPipeline ||
       !ctx->Driver.BuildPrecalcPipeline( ctx ))
      build_full_precalc_pipeline( ctx );

   pre->data_valid = 0;
   pre->pipeline_valid = 1;
   elt->pipeline_valid = 0;
   
   tnl->CVA.orflag = 0;
   
   if (MESA_VERBOSE&VERBOSE_PIPELINE)
      gl_print_pipeline( ctx, pre ); 
}


static void build_full_immediate_pipeline( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct gl_pipeline_stage *pipeline = tnl->PipelineStage;
   struct gl_cva *cva = &tnl->CVA;
   struct gl_pipeline *pre = &cva->pre;   
   struct gl_pipeline *elt = &cva->elt;
   struct gl_pipeline_stage **stages = elt->stages;
   GLuint i;
   GLuint newstate = elt->new_state;
   GLuint active_ops = 0;
   GLuint available = cva->orflag | MINIMAL_VERT_DATA;
   GLuint generated = 0;
   GLuint is_elt = 0;

   if (pre->data_valid && tnl->CompileCVAFlag) {
      is_elt = 1;
      active_ops = cva->pre.ops;
      available |= pre->outputs | VERT_PRECALC_DATA;
   }


   elt->outputs = 0;		/* not used */
   elt->inputs = 0;

   for (i = 0 ; i < tnl->NrPipelineStages ; i++) {
      pipeline[i].active &= ~PIPE_IMMEDIATE;

      if ((pipeline[i].state_change & newstate) ||
  	  (pipeline[i].elt_forbidden_inputs & available)) 
      {
	 pipeline[i].check(ctx, &pipeline[i]);
      }

      if ((pipeline[i].type & PIPE_IMMEDIATE) &&
	  (pipeline[i].ops & active_ops) == 0 && 
	  (pipeline[i].elt_forbidden_inputs & available) == 0
	 )
      {
	 if (pipeline[i].inputs & ~available) 
	    elt->forbidden_inputs |= pipeline[i].inputs & ~available;
	 else
	 {
	    elt->inputs |= pipeline[i].inputs & ~generated;
	    elt->forbidden_inputs |= pipeline[i].elt_forbidden_inputs;
	    pipeline[i].active |= PIPE_IMMEDIATE;
	    *stages++ = &pipeline[i];
	    generated |= pipeline[i].outputs;
	    available |= pipeline[i].outputs;
	    active_ops |= pipeline[i].ops;
	 }
      }
   }

   *stages = 0;
   
   elt->copy_transformed_data = 1;
   elt->replay_copied_vertices = 0;

   if (is_elt) {
      cva->merge = elt->inputs & pre->outputs;
      elt->ops = active_ops & ~pre->ops;
   }
}



void gl_build_immediate_pipeline( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct gl_pipeline *elt = &tnl->CVA.elt;   

   if (!ctx->Driver.BuildEltPipeline ||
       !ctx->Driver.BuildEltPipeline( ctx )) {
      build_full_immediate_pipeline( ctx );
   }

   elt->pipeline_valid = 1;
   tnl->CVA.orflag = 0;
   
   if (MESA_VERBOSE&VERBOSE_PIPELINE)
      gl_print_pipeline( ctx, elt ); 
}
   
#define INTERESTED ~0

void gl_update_pipelines( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint newstate = ctx->NewState;
   struct gl_cva *cva = &tnl->CVA;

   newstate &= INTERESTED;

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_STATE))
      gl_print_enable_flags("enabled", ctx->_Enabled);

   if (newstate ||
       cva->lock_changed ||
       cva->orflag != cva->last_orflag ||
       tnl->_ArrayFlags != cva->last_array_flags)
   {   
      GLuint flags = VERT_WIN;

      if (ctx->Visual.RGBAflag) {
	 flags |= VERT_RGBA;
	 if (ctx->_TriangleCaps && DD_SEPERATE_SPECULAR)
 	    flags |= VERT_SPEC_RGB;
      } else 
	 flags |= VERT_INDEX;

      if (ctx->Texture._ReallyEnabled & TEXTURE0_ANY) 
	 flags |= VERT_TEX0_ANY;

      if (ctx->Texture._ReallyEnabled & TEXTURE1_ANY)
	 flags |= VERT_TEX1_ANY;

#if MAX_TEXTURE_UNITS > 2
      if (ctx->Texture._ReallyEnabled & TEXTURE2_ANY)
	 flags |= VERT_TEX2_ANY;
#endif
#if MAX_TEXTURE_UNITS > 3
      if (ctx->Texture._ReallyEnabled & TEXTURE3_ANY)
	 flags |= VERT_TEX3_ANY;
#endif
   
      if (ctx->Polygon._Unfilled) 
	 flags |= VERT_EDGE;
 
      if (ctx->Fog.FogCoordinateSource == GL_FOG_COORDINATE_EXT)
 	 flags |= VERT_FOG_COORD;

      if (ctx->RenderMode==GL_FEEDBACK) {
	 flags = (VERT_WIN | VERT_RGBA | VERT_INDEX | VERT_NORM | VERT_EDGE
		  | VERT_TEX0_ANY
                  | VERT_TEX1_ANY
#if MAX_TEXTURE_UNITS > 2
                  | VERT_TEX2_ANY
#endif
#if MAX_TEXTURE_UNITS > 3
                  | VERT_TEX3_ANY
#endif
                  );
      }

      tnl->_RenderFlags = flags;

      cva->elt.new_state |= newstate;
      cva->elt.pipeline_valid = 0;

      cva->pre.new_state |= newstate;
      cva->pre.forbidden_inputs = 0;
      cva->pre.pipeline_valid = 0;
      cva->lock_changed = 0;
   }

   if (tnl->_ArrayNewState != cva->last_array_new_state)
      cva->pre.pipeline_valid = 0;

   cva->pre.data_valid = 0;
   cva->last_array_new_state = tnl->_ArrayNewState;
   cva->last_orflag = cva->orflag;
   cva->last_array_flags = tnl->_ArrayFlags;
}

void gl_run_pipeline( struct vertex_buffer *VB )
{
   struct gl_pipeline *pipe = VB->pipeline;
   struct gl_pipeline_stage **stages = pipe->stages;
   unsigned short x;

   pipe->data_valid = 1;	/* optimized stages might want to reset this. */

   if (0) gl_print_pipeline( VB->ctx, pipe );
   
   START_FAST_MATH(x);
   
   for ( VB->Culled = 0; *stages && !VB->Culled ; stages++ ) 
      (*stages)->run( VB );

   END_FAST_MATH(x);

   pipe->new_state = 0;
}


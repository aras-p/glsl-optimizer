/* $Id: t_vb_fog.c,v 1.2 2001/01/03 22:56:23 brianp Exp $ */

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
 *
 * Author:
 *    Keith Whitwell <keithw@valinux.com>
 */


#include "glheader.h"
#include "colormac.h"
#include "context.h"
#include "macros.h"
#include "mem.h"
#include "mmath.h"
#include "mtypes.h"

#include "math/m_xform.h"

#include "t_context.h"
#include "t_pipeline.h"


struct fog_stage_data {
   GLvector1f fogcoord;		/* has actual storage allocated */
   GLvector1f input;		/* points into VB->EyePtr Z values */
};

#define FOG_STAGE_DATA(stage) ((struct fog_stage_data *)stage->private)



/* Use lookup table & interpolation?
 */
static void make_win_fog_coords( GLcontext *ctx, GLvector1f *out, 
				 const GLvector1f *in )
{
   GLfloat end  = ctx->Fog.End;
   GLfloat *v = in->start;
   GLuint stride = in->stride;
   GLuint n = in->count;
   GLfloat *data = out->data;		
   GLfloat d;
   GLuint i;

   out->count = in->count;

   switch (ctx->Fog.Mode) {
   case GL_LINEAR:
      if (ctx->Fog.Start == ctx->Fog.End)
         d = 1.0F;
      else
         d = 1.0F / (ctx->Fog.End - ctx->Fog.Start);
      for ( i = 0 ; i < n ; i++, STRIDE_F(v, stride)) 
	 data[i] = (end - ABSF(*v)) * d;
      break;
   case GL_EXP:
      d = -ctx->Fog.Density;
      for ( i = 0 ; i < n ; i++, STRIDE_F(v,stride)) 
	 data[i] = exp( d*ABSF(*v) );
      break;
   case GL_EXP2:
      d = -(ctx->Fog.Density*ctx->Fog.Density);
      for ( i = 0 ; i < n ; i++, STRIDE_F(v, stride)) {
	 GLfloat z = *v;
	 data[i] = exp( d*z*z );
      }
      break;
   default:
      gl_problem(ctx, "Bad fog mode in make_fog_coord");
      return;
   }
}


static GLboolean run_fog_stage( GLcontext *ctx, 
				struct gl_pipeline_stage *stage )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   struct fog_stage_data *store = FOG_STAGE_DATA(stage);
   GLvector1f *input;

   VB->FogCoordPtr = &store->fogcoord;

   if (stage->changed_inputs == 0)
      return GL_TRUE;

   if (ctx->Fog.FogCoordinateSource == GL_FRAGMENT_DEPTH_EXT) {
      if (!ctx->_NeedEyeCoords) {
	 GLfloat *m = ctx->ModelView.m;
	 GLfloat plane[4];
	 
	 /* Use this to store calculated eye z values:
	  */
	 input = &store->fogcoord;

	 plane[0] = m[2];
	 plane[1] = m[6];
	 plane[2] = m[10];
	 plane[3] = m[14];

	 /* Full eye coords weren't required, just calculate the
	  * eye Z values.
	  */
	 gl_dotprod_tab[0][VB->ObjPtr->size](input->data, sizeof(GLfloat),
					     VB->ObjPtr, plane, 0 );

	 input->count = VB->ObjPtr->count;
      }
      else
      {
	 input = &store->input;

	 if (VB->EyePtr->size < 2)
	    gl_vector4f_clean_elem( VB->EyePtr, VB->Count, 2 );

	 input->data = &(VB->EyePtr->data[0][2]);
	 input->start = VB->EyePtr->start+2;
	 input->stride = VB->EyePtr->stride;
	 input->count = VB->EyePtr->count;
      }
   } else
      input = VB->FogCoordPtr;

   make_win_fog_coords( ctx, VB->FogCoordPtr, input );
   return GL_TRUE;
}

static void check_fog_stage( GLcontext *ctx, struct gl_pipeline_stage *stage )
{
   stage->active = ctx->Fog.Enabled;

   if (ctx->Fog.FogCoordinateSource == GL_FRAGMENT_DEPTH_EXT)
      stage->inputs = VERT_EYE;
   else
      stage->inputs = VERT_FOG_COORD;
}


/* Called the first time stage->run() is invoked.
 */
static GLboolean alloc_fog_data( GLcontext *ctx, 
				 struct gl_pipeline_stage *stage )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct fog_stage_data *store;
   stage->private = MALLOC(sizeof(*store));
   store = FOG_STAGE_DATA(stage);
   if (!store)
      return GL_FALSE;

   gl_vector1f_alloc( &store->fogcoord, 0, tnl->vb.Size, 32 );
   gl_vector1f_init( &store->input, 0, 0 );

   /* Now run the stage.
    */
   stage->run = run_fog_stage;
   return stage->run( ctx, stage );
}


static void free_fog_data( struct gl_pipeline_stage *stage )
{
   struct fog_stage_data *store = FOG_STAGE_DATA(stage);
   if (store) {
      gl_vector1f_free( &store->fogcoord );
      FREE( store );
      stage->private = 0;
   }
}


const struct gl_pipeline_stage _tnl_fog_coordinate_stage = 
{ 
   "build fog coordinates",
   _NEW_FOG,
   _NEW_FOG,			
   0, 0, VERT_FOG_COORD,	/* active, inputs, outputs */
   0, 0,			/* changed_inputs, private_data */
   free_fog_data,		/* dtr */
   check_fog_stage,		/* check */
   alloc_fog_data		/* run -- initially set to init. */
};

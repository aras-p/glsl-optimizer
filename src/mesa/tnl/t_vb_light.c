/* $Id: t_vb_light.c,v 1.14 2001/04/28 08:39:18 keithw Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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



#include "glheader.h"
#include "colormac.h"
#include "light.h"
#include "macros.h"
#include "mem.h"
#include "mmath.h"
#include "simple_list.h"
#include "mtypes.h"

#include "t_context.h"
#include "t_pipeline.h"

#define LIGHT_FLAGS         0x1	/* must be first */
#define LIGHT_TWOSIDE       0x2
#define LIGHT_COLORMATERIAL 0x4
#define MAX_LIGHT_FUNC      0x8

typedef void (*light_func)( GLcontext *ctx,
			    struct vertex_buffer *VB,
			    struct gl_pipeline_stage *stage,
			    GLvector4f *input );

struct light_stage_data {
   struct gl_client_array LitColor[2];
   struct gl_client_array LitSecondary[2];
   GLvector1ui LitIndex[2];
   light_func *light_func_tab;
};

#define LIGHT_STAGE_DATA(stage) ((struct light_stage_data *)(stage->privatePtr))

/* Tables for all the shading functions.
 */
static light_func _tnl_light_tab[MAX_LIGHT_FUNC];
static light_func _tnl_light_fast_tab[MAX_LIGHT_FUNC];
static light_func _tnl_light_fast_single_tab[MAX_LIGHT_FUNC];
static light_func _tnl_light_spec_tab[MAX_LIGHT_FUNC];
static light_func _tnl_light_ci_tab[MAX_LIGHT_FUNC];

#define TAG(x)           x
#define IDX              (0)
#include "t_vb_lighttmp.h"

#define TAG(x)           x##_tw
#define IDX              (LIGHT_TWOSIDE)
#include "t_vb_lighttmp.h"

#define TAG(x)           x##_fl
#define IDX              (LIGHT_FLAGS)
#include "t_vb_lighttmp.h"

#define TAG(x)           x##_tw_fl
#define IDX              (LIGHT_FLAGS|LIGHT_TWOSIDE)
#include "t_vb_lighttmp.h"

#define TAG(x)           x##_cm
#define IDX              (LIGHT_COLORMATERIAL)
#include "t_vb_lighttmp.h"

#define TAG(x)           x##_tw_cm
#define IDX              (LIGHT_TWOSIDE|LIGHT_COLORMATERIAL)
#include "t_vb_lighttmp.h"

#define TAG(x)           x##_fl_cm
#define IDX              (LIGHT_FLAGS|LIGHT_COLORMATERIAL)
#include "t_vb_lighttmp.h"

#define TAG(x)           x##_tw_fl_cm
#define IDX              (LIGHT_FLAGS|LIGHT_TWOSIDE|LIGHT_COLORMATERIAL)
#include "t_vb_lighttmp.h"


static void init_lighting( void )
{
   static int done;

   if (!done) {
      init_light_tab();
      init_light_tab_tw();
      init_light_tab_fl();
      init_light_tab_tw_fl();
      init_light_tab_cm();
      init_light_tab_tw_cm();
      init_light_tab_fl_cm();
      init_light_tab_tw_fl_cm();
      done = 1;
   }
}


static GLboolean run_lighting( GLcontext *ctx, struct gl_pipeline_stage *stage )
{
   struct light_stage_data *store = LIGHT_STAGE_DATA(stage);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLvector4f *input = ctx->_NeedEyeCoords ? VB->EyePtr : VB->ObjPtr;
   GLuint ind;

/*     _tnl_print_vert_flags( __FUNCTION__, stage->changed_inputs ); */

   /* Make sure we can talk about elements 0..2 in the vector we are
    * lighting.
    */
   if (stage->changed_inputs & (VERT_EYE|VERT_OBJ)) {
      if (input->size <= 2) {
	 if (input->flags & VEC_NOT_WRITEABLE) {
	    ASSERT(VB->importable_data & VERT_OBJ);

	    VB->import_data( ctx, VERT_OBJ, VEC_NOT_WRITEABLE );
	    input = ctx->_NeedEyeCoords ? VB->EyePtr : VB->ObjPtr;

	    ASSERT((input->flags & VEC_NOT_WRITEABLE) == 0);
	 }

	 _mesa_vector4f_clean_elem(input, VB->Count, 2);
      }
   }

   if (VB->Flag)
      ind = LIGHT_FLAGS;
   else
      ind = 0;

   /* The individual functions know about replaying side-effects
    * vs. full re-execution. 
    */
   store->light_func_tab[ind]( ctx, VB, stage, input );

   return GL_TRUE;
}


/* Called in place of do_lighting when the light table may have changed.
 */
static GLboolean run_validate_lighting( GLcontext *ctx,
					struct gl_pipeline_stage *stage )
{
   GLuint ind = 0;
   light_func *tab;

   if (ctx->Visual.rgbMode) {
      if (ctx->Light._NeedVertices) {
	 if (ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR)
	    tab = _tnl_light_spec_tab;
	 else
	    tab = _tnl_light_tab;
      }
      else {
	 if (ctx->Light.EnabledList.next == ctx->Light.EnabledList.prev)
	    tab = _tnl_light_fast_single_tab;
	 else
	    tab = _tnl_light_fast_tab;
      }
   }
   else
      tab = _tnl_light_ci_tab;

   if (ctx->Light.ColorMaterialEnabled)
      ind |= LIGHT_COLORMATERIAL;

   if (ctx->Light.Model.TwoSide)
      ind |= LIGHT_TWOSIDE;

   LIGHT_STAGE_DATA(stage)->light_func_tab = &tab[ind];

   /* This and the above should only be done on _NEW_LIGHT:
    */
   _mesa_validate_all_lighting_tables( ctx );

   /* Now run the stage...
    */
   stage->run = run_lighting;
   return stage->run( ctx, stage );
}

static void alloc_4f( struct gl_client_array *a, GLuint sz )
{
   a->Ptr = ALIGN_MALLOC( sz * sizeof(GLfloat) * 4, 32 );
   a->Size = 4;
   a->Type = GL_FLOAT;
   a->Stride = 0;
   a->StrideB = sizeof(GLfloat) * 4;
   a->Enabled = 0;
   a->Flags = 0;
}

static void free_4f( struct gl_client_array *a )
{
   ALIGN_FREE( a->Ptr );
}

/* Called the first time stage->run is called.  In effect, don't
 * allocate data until the first time the stage is run.
 */
static GLboolean run_init_lighting( GLcontext *ctx,
				    struct gl_pipeline_stage *stage )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct light_stage_data *store;
   GLuint size = tnl->vb.Size;

   stage->privatePtr = MALLOC(sizeof(*store));
   store = LIGHT_STAGE_DATA(stage);
   if (!store)
      return GL_FALSE;

   /* Do onetime init.
    */
   init_lighting();

   alloc_4f( &store->LitColor[0], size );
   alloc_4f( &store->LitColor[1], size );
   alloc_4f( &store->LitSecondary[0], size );
   alloc_4f( &store->LitSecondary[1], size );

   _mesa_vector1ui_alloc( &store->LitIndex[0], 0, size, 32 );
   _mesa_vector1ui_alloc( &store->LitIndex[1], 0, size, 32 );

   /* Now validate the stage derived data...
    */
   stage->run = run_validate_lighting;
   return stage->run( ctx, stage );
}



/*
 * Check if lighting is enabled.  If so, configure the pipeline stage's
 * type, inputs, and outputs.
 */
static void check_lighting( GLcontext *ctx, struct gl_pipeline_stage *stage )
{
   stage->active = ctx->Light.Enabled;
   if (stage->active) {
      if (stage->privatePtr)
	 stage->run = run_validate_lighting;
      stage->inputs = VERT_NORM|VERT_MATERIAL;
      if (ctx->Light._NeedVertices)
	 stage->inputs |= VERT_EYE; /* effectively, even when lighting in obj */
      if (ctx->Light.ColorMaterialEnabled)
	 stage->inputs |= VERT_RGBA;

      stage->outputs = VERT_RGBA;
      if (ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR)
	 stage->outputs |= VERT_SPEC_RGB;
   }
}


static void dtr( struct gl_pipeline_stage *stage )
{
   struct light_stage_data *store = LIGHT_STAGE_DATA(stage);

   if (store) {
      free_4f( &store->LitColor[0] );
      free_4f( &store->LitColor[1] );
      free_4f( &store->LitSecondary[0] );
      free_4f( &store->LitSecondary[1] );
      _mesa_vector1ui_free( &store->LitIndex[0] );
      _mesa_vector1ui_free( &store->LitIndex[1] );
      FREE( store );
      stage->privatePtr = 0;
   }
}

const struct gl_pipeline_stage _tnl_lighting_stage =
{
   "lighting",
   _NEW_LIGHT,			/* recheck */
   _NEW_LIGHT|_NEW_MODELVIEW,	/* recalc -- modelview dependency
				 * otherwise not captured by inputs
				 * (which may be VERT_OBJ) */
   0,0,0,			/* active, inputs, outputs */
   0,0,				/* changed_inputs, private_data */
   dtr,				/* destroy */
   check_lighting,		/* check */
   run_init_lighting		/* run -- initially set to ctr */
};

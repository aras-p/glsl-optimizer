
/*
 * Mesa 3-D graphics library
 * Version:  3.3
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
 *
 * Original Mesa / 3Dfx device driver (C) 1999 David Bucciarelli, by the
 * terms stated above.
 *
 * Author:
 *   Keith Whitwell <keith@precisioninsight.com>
 */


/* fxvsetup.c - 3Dfx VooDoo vertices setup functions */


#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

#if defined(FX)

#include "fxdrv.h"
#include "mmath.h"
#include "swrast_setup/swrast_setup.h"

#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"


void fxPrintSetupFlags( const char *msg, GLuint flags )
{
   fprintf(stderr, "%s: %d %s%s%s%s%s\n",
	  msg,
	  flags,
	  (flags & SETUP_XYZW) ? " xyzw," : "", 
	  (flags & SETUP_SNAP) ? " snap," : "", 
	  (flags & SETUP_RGBA) ? " rgba," : "",
	  (flags & SETUP_TMU0)  ? " tmu0," : "",
	  (flags & SETUP_TMU1)  ? " tmu1," : "");
}

static void project_texcoords( fxVertex *v,
			       struct vertex_buffer *VB,
			       GLuint tmu_nr, GLuint tc_nr,
			       GLuint start, GLuint count )
{			       
   GrTmuVertex *tmu = &(v->v.tmuvtx[tmu_nr]);
   GLvector4f *vec = VB->TexCoordPtr[tc_nr];

   GLuint i;
   GLuint stride = vec->stride;
   GLfloat *data = VEC_ELT(vec, GLfloat, start);

   for (i = start ; i < count ; i++, STRIDE_F(data, stride), v++) {
      tmu->oow = v->v.oow * data[3];
      tmu = (GrTmuVertex *)((char *)tmu + sizeof(fxVertex));
   }      
}


static void copy_w( fxVertex *v,
		    struct vertex_buffer *VB,
		    GLuint tmu_nr, 
		    GLuint start, GLuint count )
{			       
   GrTmuVertex *tmu = &(v->v.tmuvtx[tmu_nr]);
   GLuint i;

   for (i = start ; i < count ; i++, v++) {
      tmu->oow = v->v.oow;
      tmu = (GrTmuVertex *)((char *)tmu + sizeof(fxVertex));
   }      
}

/* need to compute W values for fogging purposes 
 */
static void fx_fake_fog_w( GLcontext *ctx,
			   fxVertex *verts,
			   struct vertex_buffer *VB, 
			   GLuint start, GLuint end )
{
   const GLfloat m10 = ctx->ProjectionMatrix.m[10];
   const GLfloat m14 = ctx->ProjectionMatrix.m[14];
   GLfloat (*clip)[4] = VB->ClipPtr->data; 
   GLubyte *clipmask = VB->ClipMask;
   GLuint i;

   for ( i = start ; i < end ; i++) {
      if (clipmask[i] == 0) {
	 verts[i].v.oow = - m10 / (clip[i][2] - m14); /* -1/zEye */
      }
   }
}



static tfxSetupFunc setupfuncs[MAX_SETUP];


#define IND (SETUP_XYZW)
#define INPUTS (VERT_CLIP)
#define NAME fxsetupXYZW
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_RGBA)
#define INPUTS (VERT_CLIP|VERT_RGBA)
#define NAME fxsetupXYZWRGBA
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_TMU0)
#define INPUTS (VERT_CLIP|VERT_TEX_ANY)
#define NAME fxsetupXYZWT0
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_TMU1)
#define INPUTS (VERT_CLIP|VERT_TEX_ANY)
#define NAME fxsetupXYZWT1
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_TMU1|SETUP_TMU0)
#define INPUTS (VERT_CLIP|VERT_TEX_ANY)
#define NAME fxsetupXYZWT0T1
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_TMU0|SETUP_RGBA)
#define INPUTS (VERT_CLIP|VERT_RGBA|VERT_TEX_ANY)
#define NAME fxsetupXYZWRGBAT0
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_TMU1|SETUP_RGBA)
#define INPUTS (VERT_CLIP|VERT_RGBA|VERT_TEX_ANY)
#define NAME fxsetupXYZWRGBAT1
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_TMU1|SETUP_TMU0|SETUP_RGBA)
#define INPUTS (VERT_CLIP|VERT_RGBA|VERT_TEX_ANY)
#define NAME fxsetupXYZWRGBAT0T1
#include "fxvbtmp.h"


#define IND (SETUP_XYZW|SETUP_SNAP)
#define INPUTS (VERT_CLIP)
#define NAME fxsetupXYZW_SNAP
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA)
#define INPUTS (VERT_CLIP|VERT_RGBA)
#define NAME fxsetupXYZW_SNAP_RGBA
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_TMU0)
#define INPUTS (VERT_CLIP|VERT_TEX_ANY)
#define NAME fxsetupXYZW_SNAP_T0
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_TMU1)
#define INPUTS (VERT_CLIP|VERT_TEX_ANY)
#define NAME fxsetupXYZW_SNAP_T1
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_TMU1|SETUP_TMU0)
#define INPUTS (VERT_CLIP|VERT_TEX_ANY)
#define NAME fxsetupXYZW_SNAP_T0T1
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_TMU0|SETUP_RGBA)
#define INPUTS (VERT_CLIP|VERT_RGBA|VERT_TEX_ANY)
#define NAME fxsetupXYZW_SNAP_RGBAT0
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_TMU1|SETUP_RGBA)
#define INPUTS (VERT_CLIP|VERT_RGBA|VERT_TEX_ANY)
#define NAME fxsetupXYZW_SNAP_RGBAT1
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_TMU1|SETUP_TMU0|SETUP_RGBA)
#define INPUTS (VERT_CLIP|VERT_RGBA|VERT_TEX_ANY)
#define NAME fxsetupXYZW_SNAP_RGBAT0T1
#include "fxvbtmp.h"



#define IND (SETUP_RGBA)
#define INPUTS (VERT_RGBA)
#define NAME fxsetupRGBA
#include "fxvbtmp.h"

#define IND (SETUP_TMU0)
#define INPUTS (VERT_TEX_ANY)
#define NAME fxsetupT0
#include "fxvbtmp.h"

#define IND (SETUP_TMU1)
#define INPUTS (VERT_TEX_ANY)
#define NAME fxsetupT1
#include "fxvbtmp.h"

#define IND (SETUP_TMU1|SETUP_TMU0)
#define INPUTS (VERT_TEX_ANY)
#define NAME fxsetupT0T1
#include "fxvbtmp.h"

#define IND (SETUP_TMU0|SETUP_RGBA)
#define INPUTS (VERT_RGBA|VERT_TEX_ANY)
#define NAME fxsetupRGBAT0
#include "fxvbtmp.h"

#define IND (SETUP_TMU1|SETUP_RGBA)
#define INPUTS (VERT_RGBA|VERT_TEX_ANY)
#define NAME fxsetupRGBAT1
#include "fxvbtmp.h"

#define IND (SETUP_TMU1|SETUP_TMU0|SETUP_RGBA)
#define INPUTS (VERT_RGBA|VERT_TEX_ANY)
#define NAME fxsetupRGBAT0T1
#include "fxvbtmp.h"


static void
fxsetup_invalid( GLcontext *ctx, GLuint start, GLuint end )
{
   fprintf(stderr, "fxMesa: invalid setup function\n");
   (void) (ctx && start && end);
}


void fxDDSetupInit( void )
{
   GLuint i;
   for (i = 0 ; i < Elements(setupfuncs) ; i++)
      setupfuncs[i] = fxsetup_invalid;

   setupfuncs[SETUP_XYZW] = fxsetupXYZW;
   setupfuncs[SETUP_XYZW|SETUP_RGBA] = fxsetupXYZWRGBA;
   setupfuncs[SETUP_XYZW|SETUP_TMU0] = fxsetupXYZWT0;
   setupfuncs[SETUP_XYZW|SETUP_TMU1] = fxsetupXYZWT1;
   setupfuncs[SETUP_XYZW|SETUP_TMU0|SETUP_RGBA] = fxsetupXYZWRGBAT0;
   setupfuncs[SETUP_XYZW|SETUP_TMU1|SETUP_RGBA] = fxsetupXYZWRGBAT1;
   setupfuncs[SETUP_XYZW|SETUP_TMU1|SETUP_TMU0] = fxsetupXYZWT0T1;
   setupfuncs[SETUP_XYZW|SETUP_TMU1|SETUP_TMU0|SETUP_RGBA] = 
      fxsetupXYZWRGBAT0T1;

   setupfuncs[SETUP_XYZW|SETUP_SNAP] = fxsetupXYZW_SNAP;
   setupfuncs[SETUP_XYZW|SETUP_SNAP|SETUP_RGBA] = fxsetupXYZW_SNAP_RGBA;
   setupfuncs[SETUP_XYZW|SETUP_SNAP|SETUP_TMU0] = fxsetupXYZW_SNAP_T0;
   setupfuncs[SETUP_XYZW|SETUP_SNAP|SETUP_TMU1] = fxsetupXYZW_SNAP_T1;
   setupfuncs[SETUP_XYZW|SETUP_SNAP|SETUP_TMU0|SETUP_RGBA] = 
      fxsetupXYZW_SNAP_RGBAT0;
   setupfuncs[SETUP_XYZW|SETUP_SNAP|SETUP_TMU1|SETUP_RGBA] = 
      fxsetupXYZW_SNAP_RGBAT1;
   setupfuncs[SETUP_XYZW|SETUP_SNAP|SETUP_TMU1|SETUP_TMU0] = 
      fxsetupXYZW_SNAP_T0T1;
   setupfuncs[SETUP_XYZW|SETUP_SNAP|SETUP_TMU1|SETUP_TMU0|SETUP_RGBA] = 
      fxsetupXYZW_SNAP_RGBAT0T1;

   setupfuncs[SETUP_RGBA] = fxsetupRGBA;
   setupfuncs[SETUP_TMU0] = fxsetupT0;
   setupfuncs[SETUP_TMU1] = fxsetupT1;
   setupfuncs[SETUP_TMU1|SETUP_TMU0] = fxsetupT0T1;
   setupfuncs[SETUP_TMU0|SETUP_RGBA] = fxsetupRGBAT0;
   setupfuncs[SETUP_TMU1|SETUP_RGBA] = fxsetupRGBAT1;
   setupfuncs[SETUP_TMU1|SETUP_TMU0|SETUP_RGBA] = fxsetupRGBAT0T1;
}



void fx_validate_BuildProjVerts(GLcontext *ctx, GLuint start, GLuint count,
				GLuint newinputs )
{
   fxMesaContext fxMesa = (fxMesaContext)ctx->DriverCtx;

   if (!fxMesa->is_in_hardware) 
      ctx->Driver.BuildProjectedVertices = _swsetup_BuildProjectedVertices;
   else {
      GLuint setupindex = SETUP_XYZW;

      fxMesa->tmu_source[0] = 0;
      fxMesa->tmu_source[1] = 1;
      fxMesa->tex_dest[0] = SETUP_TMU0;
      fxMesa->tex_dest[1] = SETUP_TMU1;
   
      /* For flat and two-side-lit triangles, colors will always be added
       * to vertices in the triangle functions.  Vertices will *always*
       * have rbga values, but only sometimes will they come from here.
       */
      if ((ctx->_TriangleCaps & (DD_FLATSHADE|DD_TRI_LIGHT_TWOSIDE)) == 0)
	 setupindex |= SETUP_RGBA;

      if (ctx->Texture._ReallyEnabled & TEXTURE0_2D) 
	 setupindex |= SETUP_TMU0;

      if (ctx->Texture._ReallyEnabled & TEXTURE1_2D) {
	 if ((ctx->Texture._ReallyEnabled & TEXTURE0_2D) == 0) {
	    fxMesa->tmu_source[0] = 1; fxMesa->tex_dest[0] = SETUP_TMU1;
	    fxMesa->tmu_source[1] = 0; fxMesa->tex_dest[1] = SETUP_TMU0;
	    setupindex |= SETUP_TMU0;
	 } else {
	    setupindex |= SETUP_TMU1;
	 }
      }

      if (MESA_VERBOSE & (VERBOSE_DRIVER|VERBOSE_PIPELINE|VERBOSE_STATE))
	 fxPrintSetupFlags("fxmesa: vertex setup function", setupindex); 
      
      fxMesa->setupindex = setupindex;
      ctx->Driver.BuildProjectedVertices = fx_BuildProjVerts;
   }
   ctx->Driver.BuildProjectedVertices( ctx, start, count, newinputs );
}


void fx_BuildProjVerts( GLcontext *ctx, GLuint start, GLuint count, 
			GLuint newinputs )
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;

   if (newinputs == ~0) {
      /* build interpolated vertices */
      setupfuncs[fxMesa->setupindex]( ctx, start, count );   
   } else {
      GLuint ind = fxMesa->setup_gone;
      fxMesa->setup_gone = 0;
   
      if (newinputs & VERT_CLIP) 
	 ind = fxMesa->setupindex;	/* clipmask has potentially changed */
      else {
	 if (newinputs & VERT_TEX0)
	    ind |= fxMesa->tex_dest[0];
      
	 if (newinputs & VERT_TEX1)
	    ind |= fxMesa->tex_dest[1];

	 if (newinputs & VERT_RGBA)
	    ind |= SETUP_RGBA;

	 ind &= fxMesa->setupindex;
      }

      if (ind) {
	 if (fxMesa->new_state) 
	    fxSetupFXUnits( ctx ); /* why? */
	 
	 if (VB->importable_data & newinputs)
	    VB->import_data( ctx, VB->importable_data & newinputs,
			     VEC_BAD_STRIDE );
      
	 setupfuncs[ind]( ctx, start, count );   
      }
   }
}

void fxAllocVB( GLcontext *ctx )
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   fxMesa->verts = ALIGN_MALLOC( tnl->vb.Size * sizeof(fxMesa->verts[0]), 32 );
}

void fxFreeVB( GLcontext *ctx )
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   if (fxMesa->verts)
      ALIGN_FREE( fxMesa->verts );
   fxMesa->verts = 0;
}


#else


/*
 * Need this to provide at least one external definition.
 */

extern int gl_fx_dummy_function_vsetup(void);
int gl_fx_dummy_function_vsetup(void)
{
  return 0;
}

#endif  /* FX */

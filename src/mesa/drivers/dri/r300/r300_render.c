/**************************************************************************

Copyright (C) 2004 Nicolai Haehnle.

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ATI, VA LINUX SYSTEMS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Nicolai Haehnle <prefect_@gmx.net>
 */

#include "glheader.h"
#include "state.h"
#include "imports.h"
#include "enums.h"
#include "macros.h"
#include "context.h"
#include "dd.h"
#include "simple_list.h"

#include "api_arrayelt.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "array_cache/acache.h"
#include "tnl/tnl.h"

#include "radeon_ioctl.h"
#include "radeon_state.h"
#include "r300_context.h"
#include "r300_ioctl.h"
#include "r300_state.h"
#include "r300_reg.h"
#include "r300_program.h"

#include "r300_lib.h"


/**********************************************************************
*                     Hardware rasterization
*
* When we fell back to software TCL, we still try to use the
* rasterization hardware for rendering.
**********************************************************************/

static void r300_render_flat_primitive(r300ContextPtr rmesa, 
	GLcontext *ctx,
	int start,
	int end,
	int type)
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint i;
   int k;
   ADAPTOR adaptor;
   AOS_DATA vb_arrays[2];
   LOCAL_VARS
       	
   if(end<=start)return; /* do we need to watch for this ? */
   
   /* setup array of structures data */

   /* Note: immediate vertex data includes all coordinates.
     To save bandwidth use either VBUF or state-based vertex generation */
    /* xyz */
   vb_arrays[0].element_size=4;
   vb_arrays[0].stride=4;
   vb_arrays[0].offset=0; /* Not used */
   vb_arrays[0].format=AOS_FORMAT_FLOAT;
   vb_arrays[0].ncomponents=4;

    /* color */
   vb_arrays[1].element_size=4;
   vb_arrays[1].stride=4;
   vb_arrays[1].offset=0; /* Not used */
   vb_arrays[1].format=AOS_FORMAT_FLOAT_COLOR;
   vb_arrays[1].ncomponents=4;

   adaptor=TWO_PIPE_ADAPTOR;
   
   adaptor.color_offset[0]=rmesa->radeon.radeonScreen->backOffset+rmesa->radeon.radeonScreen->fbLocation;
   adaptor.color_pitch[0]=(rmesa->radeon.radeonScreen->backPitch) | (0xc0<<16);

   adaptor.depth_offset=rmesa->radeon.radeonScreen->depthOffset;
   adaptor.depth_pitch=rmesa->radeon.radeonScreen->depthPitch | (0x2 << 16);
   
   init_3d(PASS_PREFIX &adaptor);
   init_flat_primitive(PASS_PREFIX &adaptor);
   
   set_scissors(PASS_PREFIX 0, 0, 2647, 1941);

   set_cliprect(PASS_PREFIX 0, 0, 0, 2647,1941);
   set_cliprect(PASS_PREFIX 1, 0, 0, 2647,1941);
   set_cliprect(PASS_PREFIX 2, 0, 0, 2647,1941);
   set_cliprect(PASS_PREFIX 3, 0, 0, 2647,1941);
   
   reg_start(R300_RE_OCCLUSION_CNTL, 0);
	     e32(R300_OCCLUSION_ON);

   set_quad0(PASS_PREFIX 1.0,1.0,1.0,1.0);
   set_init21(PASS_PREFIX 0.0,1.0);

   /* We need LOAD_VBPNTR to setup AOS_ATTR fields.. the offsets are irrelevant */
   setup_AOS(PASS_PREFIX vb_arrays, 2);


   start_immediate_packet(end-start, type, 8);

	for(i=start;i<end;i++){
		#if 1
		fprintf(stderr, "* (%f %f %f %f) (%f %f %f %f)\n", 
			VEC_ELT(VB->ObjPtr, GLfloat, i)[0],
			VEC_ELT(VB->ObjPtr, GLfloat, i)[1],
			VEC_ELT(VB->ObjPtr, GLfloat, i)[2],
			VEC_ELT(VB->ObjPtr, GLfloat, i)[3],
			
			VEC_ELT(VB->ColorPtr[0], GLfloat, i)[0],
			VEC_ELT(VB->ColorPtr[0], GLfloat, i)[1],
			VEC_ELT(VB->ColorPtr[0], GLfloat, i)[2],
			VEC_ELT(VB->ColorPtr[0], GLfloat, i)[3]
			);
		#endif
		
		/* coordinates */
		efloat(VEC_ELT(VB->ObjPtr, GLfloat, i)[0]);
		efloat(VEC_ELT(VB->ObjPtr, GLfloat, i)[1]);
		efloat(VEC_ELT(VB->ObjPtr, GLfloat, i)[2]);
		#if 0
		efloat(VEC_ELT(VB->ObjPtr, GLfloat, i)[3]);
		#else
		efloat(1.0);
		#endif
		
		/* color components */
		efloat(VEC_ELT(VB->ColorPtr[0], GLfloat, i)[0]);
		efloat(VEC_ELT(VB->ColorPtr[0], GLfloat, i)[1]);
		efloat(VEC_ELT(VB->ColorPtr[0], GLfloat, i)[2]);
		#if 0
		efloat(VEC_ELT(VB->ColorPtr[0], GLfloat, i)[3]);
		#else
		efloat(0.0);
		#endif
		}

   end_3d(PASS_PREFIX_VOID);
   
   start_packet3(RADEON_CP_PACKET3_NOP, 0);
   e32(0x0);
}

static void r300_render_primitive(r300ContextPtr rmesa, 
	GLcontext *ctx,
	int start,
	int end,
	int prim)
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint i;
   int type=-1;
   
   if(end<=start)return; /* do we need to watch for this ? */
   
	fprintf(stderr, "[%d-%d]", start, end);
	switch (prim & PRIM_MODE_MASK) {
		case GL_LINES:
   		fprintf(stderr, "L ");
	        type=R300_VAP_VF_CNTL__PRIM_LINES;
		if(end<start+2){
			fprintf(stderr, "Not enough vertices\n");
			return; /* need enough vertices for Q */
			}
      		break;
		case GL_LINE_STRIP:
   		fprintf(stderr, "LS ");
	        type=R300_VAP_VF_CNTL__PRIM_LINE_STRIP;
		if(end<start+2){
			fprintf(stderr, "Not enough vertices\n");
			return; /* need enough vertices for Q */
			}
      		break;
		case GL_LINE_LOOP:
   		fprintf(stderr, "LL ");
		return;
		if(end<start+2){
			fprintf(stderr, "Not enough vertices\n");
			return; /* need enough vertices for Q */
			}
      		break;
    		case GL_TRIANGLES:
   		fprintf(stderr, "T ");
	        type=R300_VAP_VF_CNTL__PRIM_TRIANGLES;
		if(end<start+3){
			fprintf(stderr, "Not enough vertices\n");
			return; /* need enough vertices for Q */
			}
      		break;
   		case GL_TRIANGLE_STRIP:
   		fprintf(stderr, "TS ");
	        type=R300_VAP_VF_CNTL__PRIM_TRIANGLE_STRIP;
		if(end<start+3){
			fprintf(stderr, "Not enough vertices\n");
			return; /* need enough vertices for Q */
			}
      		break;
   		case GL_TRIANGLE_FAN:
   		fprintf(stderr, "TF ");
	        type=R300_VAP_VF_CNTL__PRIM_TRIANGLE_FAN;
		if(end<start+3){
			fprintf(stderr, "Not enough vertices\n");
			return; /* need enough vertices for Q */
			}
      		break;
		case GL_QUADS:
   		fprintf(stderr, "Q ");
	        type=R300_VAP_VF_CNTL__PRIM_QUADS;
		if(end<start+4){
			fprintf(stderr, "Not enough vertices\n");
			return; /* need enough vertices for Q */
			}
      		break;
		case GL_QUAD_STRIP:
   		fprintf(stderr, "QS ");
	        type=R300_VAP_VF_CNTL__PRIM_QUAD_STRIP;
		if(end<start+4){
			fprintf(stderr, "Not enough vertices\n");
			return; /* need enough vertices for Q */
			}
      		break;
   		default:
   		fprintf(stderr, "Cannot handle primitive %02x ", prim & PRIM_MODE_MASK);
		return;
         	break;
   		}
   r300_render_flat_primitive(rmesa, ctx, start, end, type);
	
}


/**
 * Called by the pipeline manager to render a batch of primitives.
 * We can return true to pass on to the next stage (i.e. software
 * rasterization) or false to indicate that the pipeline has finished
 * after we render something.
 */
static GLboolean r300_run_render(GLcontext *ctx,
				 struct tnl_pipeline_stage *stage)
{
   r300ContextPtr rmesa = R300_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint i;
	
	if (RADEON_DEBUG == DEBUG_PRIMS)
		fprintf(stderr, "%s\n", __FUNCTION__);

   for(i=0; i < VB->PrimitiveCount; i++){
       GLuint prim = VB->Primitive[i].mode;
       GLuint start = VB->Primitive[i].start;
       GLuint length = VB->Primitive[i].count;
	r300_render_primitive(rmesa, ctx, start, start + length, prim);
   	}
	
   
   fprintf(stderr, "\n");
   #if 0
	return GL_TRUE;
   #else
        return GL_FALSE;
   #endif

#if 0
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint i;

   /* Don't handle clipping or indexed vertices or vertex manipulations.
    */
   if (mmesa->RenderIndex != 0 ||
       !mga_validate_render( ctx, VB )) {
      return GL_TRUE;
   }

   tnl->Driver.Render.Start( ctx );
   mmesa->SetupNewInputs = ~0;

   for (i = 0 ; i < VB->PrimitiveCount ; i++)
   {
      GLuint prim = VB->Primitive[i].mode;
      GLuint start = VB->Primitive[i].start;
      GLuint length = VB->Primitive[i].count;

      if (!length)
	 continue;

      mga_render_tab_verts[prim & PRIM_MODE_MASK]( ctx, start, start + length,
						   prim);
   }

   tnl->Driver.Render.Finish( ctx );

   return GL_FALSE;		/* finished the pipe */
#endif
}


/**
 * Called by the pipeline manager once before rendering.
 * We check the GL state here to
 *  a) decide whether we can do the current state in hardware and
 *  b) update hardware registers
 */
#define FALLBACK_IF(expr) \
do {										\
	if (expr) {								\
		if (RADEON_DEBUG & DEBUG_FALLBACKS)				\
			fprintf(stderr, "%s: fallback:%s\n",			\
				__FUNCTION__, #expr);				\
		stage->active = GL_FALSE;					\
		return;								\
	}									\
} while(0)

static void r300_check_render(GLcontext *ctx, struct tnl_pipeline_stage *stage)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	int i;

	if (RADEON_DEBUG & DEBUG_STATE)
		fprintf(stderr, "%s\n", __FUNCTION__);

	/* We only support rendering in hardware for now */
	if (ctx->RenderMode != GL_RENDER) {
		stage->active = GL_FALSE;
		return;
	}

	// I failed to figure out how dither works in hardware,
	// let's just ignore it for now
	//FALLBACK_IF(ctx->Color.DitherFlag);

	/* I'm almost certain I forgot something here */
	FALLBACK_IF(ctx->Color.AlphaEnabled); // GL_ALPHA_TEST
	FALLBACK_IF(ctx->Color.BlendEnabled); // GL_BLEND
	FALLBACK_IF(ctx->Fog.Enabled); // GL_FOG
	FALLBACK_IF(ctx->Line.SmoothFlag); // GL_LINE_SMOOTH
	FALLBACK_IF(ctx->Line.StippleFlag); // GL_LINE_STIPPLE
	FALLBACK_IF(ctx->Point.SmoothFlag); // GL_POINT_SMOOTH
	if (ctx->Extensions.NV_point_sprite || ctx->Extensions.ARB_point_sprite)
		FALLBACK_IF(ctx->Point.PointSprite); // GL_POINT_SPRITE_NV
	FALLBACK_IF(ctx->Polygon.OffsetPoint); // GL_POLYGON_OFFSET_POINT
	FALLBACK_IF(ctx->Polygon.OffsetLine); // GL_POLYGON_OFFSET_LINE
	FALLBACK_IF(ctx->Polygon.OffsetFill); // GL_POLYGON_OFFSET_FILL
	FALLBACK_IF(ctx->Polygon.SmoothFlag); // GL_POLYGON_SMOOTH
	FALLBACK_IF(ctx->Polygon.StippleFlag); // GL_POLYGON_STIPPLE
	FALLBACK_IF(ctx->Stencil.Enabled); // GL_STENCIL_TEST
	FALLBACK_IF(ctx->Multisample.Enabled); // GL_MULTISAMPLE_ARB

	for (i = 0; i < ctx->Const.MaxTextureUnits; i++)
		FALLBACK_IF(ctx->Texture.Unit[i].Enabled);


	/* let r300_run_render do its job */
	#if 0  
	stage->active = GL_FALSE;
	#endif
}


static void dtr(struct tnl_pipeline_stage *stage)
{
	(void)stage;
}

const struct tnl_pipeline_stage _r300_render_stage = {
	"r300 hw rasterize",
	_NEW_ALL,		/* re-check (always re-check for now) */
	0,			/* re-run (always runs) */
	GL_TRUE,		/* active */
	0, 0,			/* inputs (set in check_render), outputs */
	0, 0,			/* changed_inputs, private */
	dtr,			/* destructor */
	r300_check_render,	/* check */
	r300_run_render		/* run */
};

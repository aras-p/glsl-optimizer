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

#include "radeon_reg.h"
#include "radeon_macros.h"
#include "radeon_ioctl.h"
#include "radeon_state.h"
#include "r300_context.h"
#include "r300_ioctl.h"
#include "r300_state.h"
#include "r300_reg.h"
#include "r300_program.h"
#include "r300_tex.h"

#include "r300_emit.h"

/**********************************************************************
*                     Hardware rasterization
*
* When we fell back to software TCL, we still try to use the
* rasterization hardware for rendering.
**********************************************************************/

static int r300_get_primitive_type(r300ContextPtr rmesa, 
	GLcontext *ctx,
	int start,
	int end,
	int prim)
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint i;
   int type=-1, min_vertices=0;
   char *name="UNKNOWN";
   
   if(end<=start)return -1; /* do we need to watch for this ? */
   
	switch (prim & PRIM_MODE_MASK) {
	case GL_POINTS:
   		name="P";
	        type=R300_VAP_VF_CNTL__PRIM_POINTS;
		min_vertices=1;
      		break;		
	case GL_LINES:
   		name="L";
	        type=R300_VAP_VF_CNTL__PRIM_LINES;
		min_vertices=2;
      		break;		
	case GL_LINE_STRIP:
   		name="LS";
	        type=R300_VAP_VF_CNTL__PRIM_LINE_STRIP;
		min_vertices=2;
      		break;
	case GL_LINE_LOOP:
   		name="LL";
		min_vertices=2;
		return -1;
      		break;
    	case GL_TRIANGLES:
   		name="T";
	        type=R300_VAP_VF_CNTL__PRIM_TRIANGLES;
		min_vertices=3;
      		break;
   	case GL_TRIANGLE_STRIP:
   		name="TS";
	        type=R300_VAP_VF_CNTL__PRIM_TRIANGLE_STRIP;
		min_vertices=3;
      		break;
   	case GL_TRIANGLE_FAN:
   		name="TF";
	        type=R300_VAP_VF_CNTL__PRIM_TRIANGLE_FAN;
		min_vertices=3;
      		break;
	case GL_QUADS:
   		name="Q";
	        type=R300_VAP_VF_CNTL__PRIM_QUADS;
		min_vertices=4;
      		break;
	case GL_QUAD_STRIP:
   		name="QS";
	        type=R300_VAP_VF_CNTL__PRIM_QUAD_STRIP;
		min_vertices=4;
      		break;
	case GL_POLYGON:
		name="P";
			type=R300_VAP_VF_CNTL__PRIM_POLYGON;
		min_vertices=3;
		break;
   	default:
 		fprintf(stderr, "%s:%s Do not know how to handle primitive %02x - help me !\n",
			__FILE__, __FUNCTION__,
			prim & PRIM_MODE_MASK);
		return -1;
         	break;
   	}
   #if 0
   fprintf(stderr, "[%d-%d]%s ", start, end, name);
   #endif
   if(start+min_vertices>end){
	static int warn_once=1;
	if(warn_once){
		fprintf(stderr, "%s:%s Not enough vertices to draw primitive %02x - help me !\n",
				__FILE__, __FUNCTION__,
				prim & PRIM_MODE_MASK);
			warn_once=0;
			}
	return -1;
   	}
   return type;
}

/* This function compiles GL context into state registers that 
   describe data routing inside of R300 pipeline.
   
   In particular, it programs input_route, output_vtx_fmt, texture
   unit configuration and gb_output_vtx_fmt
   
   This function encompasses setup_AOS() from r300_lib.c
*/




/* Immediate implementation - vertex data is sent via command stream */

static GLfloat default_vector[4]={0.0, 0.0, 0.0, 1.0};

#define output_vector(v, i) \
	{ \
	int _i; \
	for(_i=0;_i<v->size;_i++){ \
		efloat(VEC_ELT(v, GLfloat, i)[_i]); \
		} \
	for(_i=v->size;_i<4;_i++){ \
			efloat(default_vector[_i]); \
			} \
	}

/* Immediate implementation - vertex data is sent via command stream */

static void r300_render_immediate_primitive(r300ContextPtr rmesa, 
	GLcontext *ctx,
	int start,
	int end,
	int prim)
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint i;
   int k, type;
   LOCAL_VARS
		   
   type=r300_get_primitive_type(rmesa, ctx, start, end, prim);
		
   		#if 0
		fprintf(stderr,"ObjPtr: size=%d stride=%d\n", 
			VB->ObjPtr->size, VB->ObjPtr->stride);
		fprintf(stderr,"ColorPtr[0]: size=%d stride=%d\n", 
			VB->ColorPtr[0]->size, VB->ColorPtr[0]->stride);
		fprintf(stderr,"TexCoordPtr[0]: size=%d stride=%d\n", 
			VB->TexCoordPtr[0]->size, VB->TexCoordPtr[0]->stride);
		#endif
   
   if(type<0)return;

   /* A packet cannot have more than 16383 data words.. */
   if(((end-start)*8+4*rmesa->state.texture.tc_count)>16380){
   	fprintf(stderr, "%s:%s: Too many vertices to paint. Fix me !\n");
	return;
   	}

   start_immediate_packet(end-start, type, 8+4*rmesa->state.texture.tc_count);

	for(i=start;i<end;i++){
		#if 0
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
		output_vector(VB->ObjPtr, i);
		
		/* color components */
		output_vector(VB->ColorPtr[0], i);
		
		/* texture coordinates */
		for(k=0;k < ctx->Const.MaxTextureUnits;k++)
			if(ctx->Texture.Unit[k].Enabled)
				output_vector(VB->TexCoordPtr[k], i);
		}

}


static GLboolean r300_run_immediate_render(GLcontext *ctx,
				 struct tnl_pipeline_stage *stage)
{
   r300ContextPtr rmesa = R300_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint i;
   /* Only do 2d textures */
   struct gl_texture_object *to=ctx->Texture.Unit[0].Current2D;
   r300TexObjPtr t=to->DriverData;
   LOCAL_VARS
	
   
   /* Update texture state - needs to be done only when actually changed..
      All the time for now.. */


	if (RADEON_DEBUG == DEBUG_PRIMS)
		fprintf(stderr, "%s\n", __FUNCTION__);

   #if 1 /* we need this, somehow */
   /* Flush state - make sure command buffer is nice and large */
   r300Flush(ctx);
   /* Make sure we have enough space */
   #else 0
   /* Count is very imprecize, but should be good upper bound */
   r300EnsureCmdBufSpace(rmesa, rmesa->hw.max_state_size + 4+2+30
   	+VB->PrimitiveCount*(1+8)+VB->Count*4*rmesa->state.texture.tc_count+4, __FUNCTION__);
   #endif
     
   /* needed before starting 3d operation .. */
   reg_start(R300_RB3D_DSTCACHE_CTLSTAT,0);
	e32(0x0000000a);
   
   reg_start(0x4f18,0);
	e32(0x00000003);
	
   
	#if 0 /* looks like the Z offset issue got fixed */
   rmesa->hw.vte.cmd[1] = R300_VPORT_X_SCALE_ENA
				| R300_VPORT_X_OFFSET_ENA
				| R300_VPORT_Y_SCALE_ENA
				| R300_VPORT_Y_OFFSET_ENA
				| R300_VTX_W0_FMT;
   R300_STATECHANGE(rmesa, vte);
	#endif
   
	
      
   /* Magic register - note it is right after 20b0 */

   
   if(rmesa->state.texture.tc_count>0){
   	reg_start(0x20b4,0);
		e32(0x0000000c);
   
	}
		
   r300EmitState(rmesa);
   
   #if 0
   reg_start(R300_RB3D_COLORMASK, 0);
   	e32(0xf);
   
   vsf_start_fragment(0x406, 4);
   efloat(0.0);
   efloat(0.0);
   efloat(0.0);
   efloat(1.0);

   vsf_start_fragment(0x400, 4);
   efloat(0.0);
   efloat(0.0);
   efloat(0.0);
   efloat(1.0);
   #endif
   
   /* We need LOAD_VBPNTR to setup AOS_ATTR fields.. the offsets are irrelevant */
   r300EmitLOAD_VBPNTR(rmesa, 0);
   
   for(i=0; i < VB->PrimitiveCount; i++){
       GLuint prim = VB->Primitive[i].mode;
       GLuint start = VB->Primitive[i].start;
       GLuint length = VB->Primitive[i].count;
	r300_render_immediate_primitive(rmesa, ctx, start, start + length, prim);
   	}
	
    /* This sequence is required after any 3d drawing packet
      I suspect it work arounds a bug (or deficiency) in hardware */
  
   reg_start(R300_RB3D_DSTCACHE_CTLSTAT,0);
	e32(0x0000000a);
   
   reg_start(0x4f18,0);
	e32(0x00000003);
         
   return GL_FALSE;
}


/* vertex buffer implementation */

/* We use the start part of GART texture buffer for vertices */


static void upload_vertex_buffer(r300ContextPtr rmesa, GLcontext *ctx)
{
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	struct vertex_buffer *VB = &tnl->vb;
	int idx=0;
	int i,j,k;
	radeonScreenPtr rsp=rmesa->radeon.radeonScreen;
	
	/* A hack - we don't want to overwrite vertex buffers, so we
	just use AGP space for them.. Fix me ! */
	static int offset=0;
	if(offset>2*1024*1024){
		//fprintf(stderr, "Wrapping agp vertex buffer offset\n");
		offset=0;
		}
	/* Not the most efficient implementation, but, for now, I just want something that
	works */
	/* to do - make single memcpy per column (is it possible ?) */
	/* to do - use dirty flags to avoid redundant copies */
	#define UPLOAD_VECTOR(v)\
		{ \
		/* Is the data dirty ? */ \
		if (v->flags & ((1<<v->size)-1)) { \
			/* fprintf(stderr, "size=%d vs stride=%d\n", v->size, v->stride); */ \
			if(v->size*4==v->stride){\
				/* fast path */  \
				memcpy(rsp->gartTextures.map+offset, v->data, v->stride*VB->Count); \
				} else { \
				for(i=0;i<VB->Count;i++){ \
					/* copy one vertex at a time*/ \
					memcpy(rsp->gartTextures.map+offset+i*v->size*4, VEC_ELT(v, GLfloat, i), v->size*4); \
					} \
				} \
			/* v->flags &= ~((1<<v->size)-1);*/ \
			} \
		rmesa->state.aos[idx].offset=rsp->gartTextures.handle+offset; \
		offset+=v->size*4*VB->Count; \
		idx++; \
		}
		
	UPLOAD_VECTOR(VB->ObjPtr);
	UPLOAD_VECTOR(VB->ColorPtr[0]);
	/* texture coordinates */
	for(k=0;k < ctx->Const.MaxTextureUnits;k++)
		if(ctx->Texture.Unit[k].Enabled)
			UPLOAD_VECTOR(VB->TexCoordPtr[k]);

	if(idx>=R300_MAX_AOS_ARRAYS){
		fprintf(stderr, "Aieee ! Maximum AOS arrays count exceeded.. \n");
		exit(-1);
		}
}

static void r300_render_vb_primitive(r300ContextPtr rmesa, 
	GLcontext *ctx,
	int start,
	int end,
	int prim)
{
   int type;
   LOCAL_VARS
       	
   if(end<=start)return; /* do we need to watch for this ? */
   
   type=r300_get_primitive_type(rmesa, ctx, start, end, prim);
   if(type<0)return;

   fire_AOS(PASS_PREFIX end-start, type);
}

static GLboolean r300_run_vb_render(GLcontext *ctx,
				 struct tnl_pipeline_stage *stage)
{
   r300ContextPtr rmesa = R300_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   int i, j;
   LOCAL_VARS
	
	if (RADEON_DEBUG == DEBUG_PRIMS)
		fprintf(stderr, "%s\n", __FUNCTION__);

   
   reg_start(R300_RB3D_DSTCACHE_CTLSTAT,0);
	e32(0x0000000a);
   
   reg_start(0x4f18,0);
	e32(0x00000003);
   
   r300_setup_routing(ctx, GL_FALSE);
	
   r300EmitState(rmesa);

   /* setup array of structures data */
   LOCK_HARDWARE(&(rmesa->radeon));

   upload_vertex_buffer(rmesa, ctx);
   //fprintf(stderr, "Using %d AOS arrays\n", n_arrays);
   
   for(i=0; i < VB->PrimitiveCount; i++){
       GLuint prim = VB->Primitive[i].mode;
       GLuint start = VB->Primitive[i].start;
       GLuint length = VB->Primitive[i].count;
       		
	   /* We need LOAD_VBPNTR to setup AOS_ATTR fields.. */
        r300EmitLOAD_VBPNTR(rmesa, start);
       
	r300_render_vb_primitive(rmesa, ctx, start, start + length, prim);
   	}
	
    /* This sequence is required after any 3d drawing packet
      I suspect it works around a bug (or deficiency) in hardware */
  
  reg_start(R300_RB3D_DSTCACHE_CTLSTAT,0);
	e32(0x0000000a);
   
   reg_start(0x4f18,0);
	e32(0x00000003);
   
   end_3d(PASS_PREFIX_VOID);
   
   /* Flush state - we are done drawing.. */
   r300FlushCmdBufLocked(ctx, __FUNCTION__);
   radeonWaitForIdleLocked(&(rmesa->radeon));
   
   UNLOCK_HARDWARE(&(rmesa->radeon));
   return GL_FALSE;
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

		
   #if 1
	
   	#if 1
        return r300_run_immediate_render(ctx, stage);
	#else 
        return r300_run_vb_render(ctx, stage);
	#endif
   #else
	return GL_TRUE;
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
		if (1 || RADEON_DEBUG & DEBUG_FALLBACKS)				\
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
	#if 0 /* This should work now.. */
	FALLBACK_IF(ctx->Color.AlphaEnabled); // GL_ALPHA_TEST
	#endif
	//FALLBACK_IF(ctx->Color.BlendEnabled); // GL_BLEND
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

	/* One step at a time - let one texture pass.. */
	for (i = 2; i < ctx->Const.MaxTextureUnits; i++)
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

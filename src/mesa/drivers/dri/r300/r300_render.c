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
   	default:
   		fprintf(stderr, "Cannot handle primitive %02x ", prim & PRIM_MODE_MASK);
		return -1;
         	break;
   	}
   #if 1
   fprintf(stderr, "[%d-%d]%s ", start, end, name);
   #endif
   if(start+min_vertices>=end){
	fprintf(stderr, "Not enough vertices\n");
	return -1;
   	}
   return type;
}



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

static void r300_render_flat_primitive(r300ContextPtr rmesa, 
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
		
		fprintf(stderr,"ObjPtr: size=%d stride=%d\n", 
			VB->ObjPtr->size, VB->ObjPtr->stride);
		fprintf(stderr,"ColorPtr[0]: size=%d stride=%d\n", 
			VB->ColorPtr[0]->size, VB->ColorPtr[0]->stride);
   
   if(type<0)return;


   start_immediate_packet(end-start, type, 8);

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
		}

}

static GLboolean r300_run_flat_render(GLcontext *ctx,
				 struct tnl_pipeline_stage *stage)
{
   r300ContextPtr rmesa = R300_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint i;
   AOS_DATA vb_arrays[2];
   LOCAL_VARS
	
	if (RADEON_DEBUG == DEBUG_PRIMS)
		fprintf(stderr, "%s\n", __FUNCTION__);

   /* setup array of structures data */

   /* Note: immediate vertex data includes all coordinates.
     To save bandwidth use either VBUF or state-based vertex generation */
    /* xyz */
   vb_arrays[0].element_size=4;
   vb_arrays[0].stride=4;
   vb_arrays[0].offset=0; /* Not used */
   vb_arrays[0].format=AOS_FORMAT_FLOAT;
   vb_arrays[0].ncomponents=4;
   vb_arrays[0].reg=REG_COORDS;

    /* color */
   vb_arrays[1].element_size=4;
   vb_arrays[1].stride=4;
   vb_arrays[1].offset=0; /* Not used */
   vb_arrays[1].format=AOS_FORMAT_FLOAT_COLOR;
   vb_arrays[1].ncomponents=4;
   vb_arrays[1].reg=REG_COLOR0;

   
   /* needed before starting 3d operation .. */
   reg_start(R300_RB3D_DSTCACHE_CTLSTAT,0);
	e32(0x0000000a);
   
   reg_start(0x4f18,0);
	e32(0x00000003);
	
   rmesa->hw.vte.cmd[1] = R300_VPORT_X_SCALE_ENA
				| R300_VPORT_X_OFFSET_ENA
				| R300_VPORT_Y_SCALE_ENA
				| R300_VPORT_Y_OFFSET_ENA
				| R300_VTX_W0_FMT;
   R300_STATECHANGE(rmesa, vte);
   
   r300EmitState(rmesa);
   
   FLAT_COLOR_PIPELINE.vertex_shader.matrix[0].length=16;
   memcpy(FLAT_COLOR_PIPELINE.vertex_shader.matrix[0].body.f, ctx->_ModelProjectMatrix.m, 16*4);

   FLAT_COLOR_PIPELINE.vertex_shader.unknown2.length=4;
   FLAT_COLOR_PIPELINE.vertex_shader.unknown2.body.f[0]=0.0;
   FLAT_COLOR_PIPELINE.vertex_shader.unknown2.body.f[1]=0.0;
   FLAT_COLOR_PIPELINE.vertex_shader.unknown2.body.f[2]=1.0;
   FLAT_COLOR_PIPELINE.vertex_shader.unknown2.body.f[3]=0.0;
   
   program_pipeline(PASS_PREFIX &FLAT_COLOR_PIPELINE);
   
   /* We need LOAD_VBPNTR to setup AOS_ATTR fields.. the offsets are irrelevant */
   setup_AOS(PASS_PREFIX vb_arrays, 2);
   
   for(i=0; i < VB->PrimitiveCount; i++){
       GLuint prim = VB->Primitive[i].mode;
       GLuint start = VB->Primitive[i].start;
       GLuint length = VB->Primitive[i].count;
	r300_render_flat_primitive(rmesa, ctx, start, start + length, prim);
   	}
	
   end_3d(PASS_PREFIX_VOID);
   
   fprintf(stderr, "\n");
   return GL_FALSE;
}

/* vertex buffer implementation */

/* We use the start part of GART texture buffer for vertices */

/* 8 is somewhat bogus... it is probably something like 24 */
#define R300_MAX_AOS_ARRAYS		8

static void upload_vertex_buffer(r300ContextPtr rmesa, 
	GLcontext *ctx, AOS_DATA *array, int *n_arrays)
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   int offset=0, idx=0;
   int i,j;
   radeonScreenPtr rsp=rmesa->radeon.radeonScreen;
   /* Not the most efficient implementation, but, for now, I just want something that
      works */
      /* to do - make single memcpy per column (is it possible ?) */
      /* to do - use dirty flags to avoid redundant copies */
#define UPLOAD_VECTOR(v, r, f)\
	{ \
	 /* Is the data dirty ? */ \
	if (v->flags & ((1<<v->size)-1)) { \
		fprintf(stderr, "size=%d vs stride=%d\n", v->size, v->stride); \
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
	array[idx].element_size=v->size; \
	array[idx].stride=v->size; \
	array[idx].format=(f); \
	array[idx].ncomponents=v->size; \
	array[idx].offset=rsp->gartTextures.handle+offset; \
	array[idx].reg=r; \
	offset+=v->size*4*VB->Count; \
	idx++; \
	}
	
UPLOAD_VECTOR(VB->ObjPtr, REG_COORDS, AOS_FORMAT_FLOAT);
UPLOAD_VECTOR(VB->ColorPtr[0], REG_COLOR0, AOS_FORMAT_FLOAT_COLOR);

*n_arrays=idx;
if(idx>=R300_MAX_AOS_ARRAYS){
	fprintf(stderr, "Aieee ! Maximum AOS arrays count exceeded.. \n");
	exit(-1);
	}
}

static void r300_render_vb_flat_primitive(r300ContextPtr rmesa, 
	GLcontext *ctx,
	int start,
	int end,
	int prim)
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint i;
   int k, type, n_arrays;
   LOCAL_VARS
       	
   if(end<=start)return; /* do we need to watch for this ? */
   
   type=r300_get_primitive_type(rmesa, ctx, start, end, prim);
   if(type<0)return;

   fire_AOS(PASS_PREFIX end-start, type);
}

static VERTEX_SHADER_FRAGMENT default_vector_vsf={
	length: 4,
	body: { 
		f: {0.0, 0.0, 0.0, 1.0}
		}
	};

static GLboolean r300_run_vb_flat_render(GLcontext *ctx,
				 struct tnl_pipeline_stage *stage)
{
   r300ContextPtr rmesa = R300_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   int i, j, n_arrays;
   AOS_DATA vb_arrays[R300_MAX_AOS_ARRAYS];
   AOS_DATA vb_arrays2[R300_MAX_AOS_ARRAYS];
   LOCAL_VARS
	
	if (RADEON_DEBUG == DEBUG_PRIMS)
		fprintf(stderr, "%s\n", __FUNCTION__);

   /* setup array of structures data */

   upload_vertex_buffer(rmesa, ctx, vb_arrays, &n_arrays);
   fprintf(stderr, "Using %d AOS arrays\n", n_arrays);
   
   reg_start(R300_RB3D_DSTCACHE_CTLSTAT,0);
	e32(0x0000000a);
   
   reg_start(0x4f18,0);
	e32(0x00000003);
   
   r300EmitState(rmesa);
   
   FLAT_COLOR_PIPELINE.vertex_shader.matrix[0].length=16;
   memcpy(FLAT_COLOR_PIPELINE.vertex_shader.matrix[0].body.f, ctx->_ModelProjectMatrix.m, 16*4);

   FLAT_COLOR_PIPELINE.vertex_shader.unknown2.length=4;
   FLAT_COLOR_PIPELINE.vertex_shader.unknown2.body.f[0]=0.0;
   FLAT_COLOR_PIPELINE.vertex_shader.unknown2.body.f[1]=0.0;
   FLAT_COLOR_PIPELINE.vertex_shader.unknown2.body.f[2]=1.0;
   FLAT_COLOR_PIPELINE.vertex_shader.unknown2.body.f[3]=0.0;

   program_pipeline(PASS_PREFIX &FLAT_COLOR_PIPELINE);
      
   set_quad0(PASS_PREFIX 1.0,1.0,1.0,1.0);
   set_init21(PASS_PREFIX 0.0,1.0);
   
   for(i=0; i < VB->PrimitiveCount; i++){
       GLuint prim = VB->Primitive[i].mode;
       GLuint start = VB->Primitive[i].start;
       GLuint length = VB->Primitive[i].count;
       
        /* copy arrays */
        memcpy(vb_arrays2, vb_arrays, sizeof(AOS_DATA)*n_arrays);
	for(j=0;j<n_arrays;j++){
		vb_arrays2[j].offset+=vb_arrays2[j].stride*start*4;
		}
		
        setup_AOS(PASS_PREFIX vb_arrays2, n_arrays);
       
	r300_render_vb_flat_primitive(rmesa, ctx, start, start + length, prim);
   	}
	
   end_3d(PASS_PREFIX_VOID);
   
   /* Flush state - we are done drawing.. */
   r300Flush(ctx);
   fprintf(stderr, "\n");
   return GL_FALSE;
}

/* Textures... */

/* Immediate implementation - vertex data is sent via command stream */

static void r300_render_tex_primitive(r300ContextPtr rmesa, 
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
		
		fprintf(stderr,"ObjPtr: size=%d stride=%d\n", 
			VB->ObjPtr->size, VB->ObjPtr->stride);
		fprintf(stderr,"ColorPtr[0]: size=%d stride=%d\n", 
			VB->ColorPtr[0]->size, VB->ColorPtr[0]->stride);
   
   if(type<0)return;


   start_immediate_packet(end-start, type, 8);

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
		output_vector(VB->TexCoordPtr[0], i);
		}

}

static GLboolean r300_run_tex_render(GLcontext *ctx,
				 struct tnl_pipeline_stage *stage)
{
   r300ContextPtr rmesa = R300_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint i;
   AOS_DATA vb_arrays[2];
   /* Only do 2d textures */
   struct gl_texture_object *to=ctx->Texture.Unit[0].Current2D;
   radeonScreenPtr rsp=rmesa->radeon.radeonScreen;
   LOCAL_VARS
	
   
   fprintf(stderr, "%s Fixme ! I am broken\n", __FUNCTION__);
   return GL_TRUE;
   
	if (RADEON_DEBUG == DEBUG_PRIMS)
		fprintf(stderr, "%s\n", __FUNCTION__);

   /* setup array of structures data */

   /* Note: immediate vertex data includes all coordinates.
     To save bandwidth use either VBUF or state-based vertex generation */
    /* xyz */
   vb_arrays[0].element_size=4;
   vb_arrays[0].stride=4;
   vb_arrays[0].offset=0; /* Not used */
   vb_arrays[0].format=AOS_FORMAT_FLOAT;
   vb_arrays[0].ncomponents=4;
   vb_arrays[0].reg=REG_COORDS;

    /* color */
   vb_arrays[1].element_size=4;
   vb_arrays[1].stride=4;
   vb_arrays[1].offset=0; /* Not used */
   vb_arrays[1].format=AOS_FORMAT_FLOAT;
   vb_arrays[1].ncomponents=4;
   vb_arrays[1].reg=REG_TEX0;

   
   /* needed before starting 3d operation .. */
   reg_start(R300_RB3D_DSTCACHE_CTLSTAT,0);
	e32(0x0000000a);
   
   reg_start(0x4f18,0);
	e32(0x00000003);
	
   
   r300EmitState(rmesa);
   
   SINGLE_TEXTURE_PIPELINE.vertex_shader.matrix[0].length=16;
   memcpy(SINGLE_TEXTURE_PIPELINE.vertex_shader.matrix[0].body.f, ctx->_ModelProjectMatrix.m, 16*4);

   SINGLE_TEXTURE_PIPELINE.vertex_shader.unknown2.length=4;
   SINGLE_TEXTURE_PIPELINE.vertex_shader.unknown2.body.f[0]=0.0;
   SINGLE_TEXTURE_PIPELINE.vertex_shader.unknown2.body.f[1]=0.0;
   SINGLE_TEXTURE_PIPELINE.vertex_shader.unknown2.body.f[2]=1.0;
   SINGLE_TEXTURE_PIPELINE.vertex_shader.unknown2.body.f[3]=0.0;

   /* Put it in the beginning of texture memory */
   SINGLE_TEXTURE_PIPELINE.texture_unit[0].offset=rsp->gartTextures.handle;
   
   /* Upload texture, a hack, really  we can do a lot better */
   #if 0
   memcpy(rsp->gartTextures.map, to->Image[0][0]->Data, to->Image[0][0]->RowStride*to->Image[0][0]->Height*4);
   #endif

reg_start(0x4e0c,0);
	e32(0x0000000f);

reg_start(0x427c,1);
	/* XG_427c(427c) */
	e32(0x00000000);
	/* XG_4280(4280) */
	e32(0x00000000);

reg_start(0x4e04,1);
	/* XG_4e04(4e04) */
	e32(0x20220000);
	/* XG_4e08(4e08) */
	e32(0x00000000);

reg_start(0x4f14,0);
	e32(0x00000001);

reg_start(0x4f1c,0);
	e32(0x00000000);

/* gap */
sync_VAP(PASS_PREFIX_VOID);

reg_start(R300_RS_CNTL_0,1);
	/* R300_RS_CNTL_0(4300) */
	e32(0x00040084);
	/* RS_INST_COUNT(4304) */
	e32(0x000000c0);

reg_start(R300_RS_ROUTE_0,0);
	e32(0x00024008);

reg_start(R300_RS_INTERP_0,7);
	/* X_MEM0_0(4310) */
	e32(0x00d10000);
	/* X_MEM0_1(4314) */
	e32(0x00d10044);
	/* X_MEM0_2(4318) */
	e32(0x00d10084);
	/* X_MEM0_3(431c) */
	e32(0x00d100c4);
	/* X_MEM0_4(4320) */
	e32(0x00d10004);
	/* X_MEM0_5(4324) */
	e32(0x00d10004);
	/* X_MEM0_6(4328) */
	e32(0x00d10004);
	/* X_MEM0_7(432c) */
	e32(0x00d10004);

reg_start(0x221c,0);
	e32(0x00000000);

reg_start(0x20b0,0);
	e32(0x0000043f);

reg_start(0x4bd8,0);
	e32(0x00000000);

reg_start(0x4e04,0);
	e32(0x20220000);

reg_start(0x20b4,0);
	e32(0x0000000c);

reg_start(0x4288,0);
	e32(0x00000000);

reg_start(0x4e0c,0);
	e32(0x0000000f);
  
  reg_start(R300_RS_CNTL_0,0);
	e32(0x00040084);
   
   program_pipeline(PASS_PREFIX &SINGLE_TEXTURE_PIPELINE);
   
   /* We need LOAD_VBPNTR to setup AOS_ATTR fields.. the offsets are irrelevant */
   setup_AOS(PASS_PREFIX vb_arrays, 2);
   
   for(i=0; i < VB->PrimitiveCount; i++){
       GLuint prim = VB->Primitive[i].mode;
       GLuint start = VB->Primitive[i].start;
       GLuint length = VB->Primitive[i].count;
	r300_render_tex_primitive(rmesa, ctx, start, start + length, prim);
   	}
	
   end_3d(PASS_PREFIX_VOID);
   
   fprintf(stderr, "\n");
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
   	/* Just switch between pipelines.. We could possibly do better.. (?) */
        if(ctx->Texture.Unit[0].Enabled)
        	return r300_run_tex_render(ctx, stage);
		else
        	return r300_run_flat_render(ctx, stage);
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

	/* One step at a time - let one texture pass.. */
	for (i = 1; i < ctx->Const.MaxTextureUnits; i++)
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

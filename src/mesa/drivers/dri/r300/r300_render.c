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
   #if 0
   fprintf(stderr, "[%d-%d]%s ", start, end, name);
   #endif
   if(start+min_vertices>=end){
	fprintf(stderr, "Not enough vertices\n");
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



static void inline r300_setup_routing(r300ContextPtr r300, GLcontext *ctx, GLboolean immediate)
{
int i, count=0,reg=0;
GLuint dw, mask;
TNLcontext *tnl = TNL_CONTEXT(ctx);
struct vertex_buffer *VB = &tnl->vb;


/* Stage 1 - input to VAP */

/* Assign register number automatically, retaining it in rmesa->state.reg */

   /* Note: immediate vertex data includes all coordinates.
     To save bandwidth use either VBUF or state-based vertex generation */
   
#define CONFIGURE_AOS(v, o, r, f) \
	{\
	if(immediate){ \
		r300->state.aos[count].element_size=4; \
		r300->state.aos[count].stride=4; \
		r300->state.aos[count].ncomponents=4; \
		} else { \
		r300->state.aos[count].element_size=v->size; \
		r300->state.aos[count].stride=v->size; \
		r300->state.aos[count].ncomponents=v->size; \
		} \
	r300->state.aos[count].offset=o; \
	r300->state.aos[count].reg=reg; \
	r300->state.aos[count].format=(f); \
	r300->state.vap_reg.r=reg; \
	count++; \
	reg++; \
	}

	/* All offsets are 0 - for use by immediate mode. 
	   Should change later to handle vertex buffers */
CONFIGURE_AOS(VB->ObjPtr, 0, i_coords, AOS_FORMAT_FLOAT);
CONFIGURE_AOS(VB->ColorPtr[0], 0, i_color[0], AOS_FORMAT_FLOAT_COLOR);
for(i=0;i < ctx->Const.MaxTextureUnits;i++)
	if(ctx->Texture.Unit[i].Enabled)
		CONFIGURE_AOS(VB->TexCoordPtr[i], 0, i_tex[i], AOS_FORMAT_FLOAT);
		
r300->state.aos_count=count;

if(count>R300_MAX_AOS_ARRAYS){
	fprintf(stderr, "Aieee ! AOS array count exceeded !\n");
	exit(-1);
	}
		
/* Implement AOS */


/* setup INPUT_ROUTE */

R300_STATECHANGE(r300, vir[0]);
for(i=0;i+1<count;i+=2){
	dw=(r300->state.aos[i].ncomponents-1) 
	   | ((r300->state.aos[i].reg)<<8)
	   | (r300->state.aos[i].format<<14)
	   | (((r300->state.aos[i+1].ncomponents-1) 
	   | ((r300->state.aos[i+1].reg)<<8)
	   | (r300->state.aos[i+1].format<<14))<<16);
	   
	if(i+2==count){
		dw|=(1<<(13+16));
		}
	r300->hw.vir[0].cmd[R300_VIR_CNTL_0+(i>>1)]=dw;
	}
if(count & 1){
	dw=(r300->state.aos[count-1].ncomponents-1)
	   | (r300->state.aos[count-1].format<<14)
	   | ((r300->state.aos[count-1].reg)<<8)
	   | (1<<13);
	r300->hw.vir[0].cmd[R300_VIR_CNTL_0+(count>>1)]=dw;
	}
/* Set the rest of INPUT_ROUTE_0 to 0 */
for(i=((count+1)>>1); i<8; i++)r300->hw.vir[0].cmd[R300_VIR_CNTL_0+i]=(0x0);

/* Mesa assumes that all missing components are from (0, 0, 0, 1) */
#define ALL_COMPONENTS ((R300_INPUT_ROUTE_SELECT_X<<R300_INPUT_ROUTE_X_SHIFT) \
	| (R300_INPUT_ROUTE_SELECT_Y<<R300_INPUT_ROUTE_Y_SHIFT) \
	| (R300_INPUT_ROUTE_SELECT_Z<<R300_INPUT_ROUTE_Z_SHIFT) \
	| (R300_INPUT_ROUTE_SELECT_W<<R300_INPUT_ROUTE_W_SHIFT))

#define ALL_DEFAULT ((R300_INPUT_ROUTE_SELECT_ZERO<<R300_INPUT_ROUTE_X_SHIFT) \
	| (R300_INPUT_ROUTE_SELECT_ZERO<<R300_INPUT_ROUTE_Y_SHIFT) \
	| (R300_INPUT_ROUTE_SELECT_ZERO<<R300_INPUT_ROUTE_Z_SHIFT) \
	| (R300_INPUT_ROUTE_SELECT_ONE<<R300_INPUT_ROUTE_W_SHIFT))

R300_STATECHANGE(r300, vir[1]);
	
for(i=0;i+1<count;i+=2){
	/* do i first.. */
	mask=(1<<(r300->state.aos[i].ncomponents*3))-1;
	dw=(ALL_COMPONENTS & mask)
	 | (ALL_DEFAULT & ~mask)
	 | R300_INPUT_ROUTE_ENABLE;
	 
	/* i+1 */
	mask=(1<<(r300->state.aos[i+1].ncomponents*3))-1;
	dw|=( 
	   (ALL_COMPONENTS & mask)
	 | (ALL_DEFAULT & ~mask)
	 | R300_INPUT_ROUTE_ENABLE
	    )<<16;

	r300->hw.vir[1].cmd[R300_VIR_CNTL_0+(i>>1)]=dw;
	}
if(count & 1){
	mask=(1<<(r300->state.aos[count-1].ncomponents*3))-1;
	dw=(ALL_COMPONENTS & mask)
	 | (ALL_DEFAULT & ~mask)
	 | R300_INPUT_ROUTE_ENABLE;
	r300->hw.vir[1].cmd[R300_VIR_CNTL_0+(count>>1)]=dw;
	}
/* Set the rest of INPUT_ROUTE_1 to 0 */
for(i=((count+1)>>1); i<8; i++)r300->hw.vir[1].cmd[R300_VIR_CNTL_0+i]=(0x0);

/* Set up input_cntl */

R300_STATECHANGE(r300, vic);
r300->hw.vic.cmd[R300_VIC_CNTL_0]=0x5555;  /* Hard coded value, no idea what it means */

r300->hw.vic.cmd[R300_VIC_CNTL_1]=R300_INPUT_CNTL_POS
				| R300_INPUT_CNTL_COLOR;

for(i=0;i < ctx->Const.MaxTextureUnits;i++)
	if(ctx->Texture.Unit[i].Enabled)
		r300->hw.vic.cmd[R300_VIC_CNTL_1]|=(R300_INPUT_CNTL_TC0<<i);

/* Stage 3: VAP output */
R300_STATECHANGE(r300, vof);
r300->hw.vof.cmd[R300_VOF_CNTL_0]=R300_VAP_OUTPUT_VTX_FMT_0__POS_PRESENT
				| R300_VAP_OUTPUT_VTX_FMT_0__COLOR_PRESENT;

r300->hw.vof.cmd[R300_VOF_CNTL_1]=0;
for(i=0;i < ctx->Const.MaxTextureUnits;i++)
	if(ctx->Texture.Unit[i].Enabled)
		r300->hw.vof.cmd[R300_VOF_CNTL_1]|=(4<<(3*i));
}

static inline void r300_setup_textures(r300ContextPtr r300, GLcontext *ctx)
{
int i;
struct r300_tex_obj *t;

R300_STATECHANGE(r300, txe);
R300_STATECHANGE(r300, tex.filter);
R300_STATECHANGE(r300, tex.unknown1);
R300_STATECHANGE(r300, tex.size);
R300_STATECHANGE(r300, tex.format);
R300_STATECHANGE(r300, tex.offset);
R300_STATECHANGE(r300, tex.unknown4);
R300_STATECHANGE(r300, tex.unknown5);

r300->hw.txe.cmd[R300_TXE_ENABLE]=0x0;

for(i=0;i<R300_MAX_TEXTURE_UNITS;i++){
	if((t=r300->state.texture.unit[i].texobj)!=NULL){
		r300->hw.txe.cmd[R300_TXE_ENABLE]|=(1<<i);
		
		r300->hw.tex.filter.cmd[R300_TEX_CMD_0+i]=t->filter;
		r300->hw.tex.unknown1.cmd[R300_TEX_CMD_0+i]=t->pitch;
		r300->hw.tex.size.cmd[R300_TEX_CMD_0+i]=t->size;
		r300->hw.tex.format.cmd[R300_TEX_CMD_0+i]=t->format;
		r300->hw.tex.offset.cmd[R300_TEX_CMD_0+i]=r300->radeon.radeonScreen->fbLocation+t->offset;
		r300->hw.tex.unknown4.cmd[R300_TEX_CMD_0+i]=0x0;
		r300->hw.tex.unknown5.cmd[R300_TEX_CMD_0+i]=0x0;
		
		/* We don't know how to set this yet */
		r300->hw.tex.format.cmd[R300_TEX_CMD_0+i]=0x88a0c;
		}
	}
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

static void assign_pipeline(r300ContextPtr rmesa, R300_PIPELINE *p)
{
   /* Watch out ! This is buggy .. but will do for now */
   
   /* At least one sanity check is in order */
   if(sizeof(rmesa->state.vertex_shader) != sizeof(p->vertex_shader)){
   	fprintf(stderr, "Aieee ! vertex_shader sizes don't match.\n");
	exit(-1);
   	}
   if(sizeof(rmesa->state.pixel_shader) != sizeof(p->pixel_shader)){
   	fprintf(stderr, "Aieee ! vertex_shader sizes don't match.\n");
	exit(-1);
   	}
   
   memcpy(&rmesa->state.vertex_shader, &(p->vertex_shader), sizeof(rmesa->state.vertex_shader));
   memcpy(&rmesa->state.pixel_shader, &(p->pixel_shader), sizeof(rmesa->state.pixel_shader));

}

static GLboolean r300_run_flat_render(GLcontext *ctx,
				 struct tnl_pipeline_stage *stage)
{
   r300ContextPtr rmesa = R300_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint i;
   LOCAL_VARS
	
   /* Flush state - make sure command buffer is nice and large */
   r300Flush(ctx);
	
	if (RADEON_DEBUG == DEBUG_PRIMS)
		fprintf(stderr, "%s\n", __FUNCTION__);

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

   r300_setup_routing(rmesa, ctx, GL_TRUE);
   r300_setup_textures(rmesa, ctx);
      
   r300EmitState(rmesa);
   
   assign_pipeline(rmesa, &FLAT_COLOR_PIPELINE);
   
   rmesa->state.vertex_shader.matrix[0].length=16;
   memcpy(rmesa->state.vertex_shader.matrix[0].body.f, ctx->_ModelProjectMatrix.m, 16*4);

   rmesa->state.vertex_shader.unknown2.length=4;
   rmesa->state.vertex_shader.unknown2.body.f[0]=0.0;
   rmesa->state.vertex_shader.unknown2.body.f[1]=0.0;
   rmesa->state.vertex_shader.unknown2.body.f[2]=1.0;
   rmesa->state.vertex_shader.unknown2.body.f[3]=0.0;
   
   r300EmitVertexShader(rmesa);
   r300EmitPixelShader(rmesa);
         
   /* We need LOAD_VBPNTR to setup AOS_ATTR fields.. the offsets are irrelevant */   
   r300EmitLOAD_VBPNTR(rmesa, 0);
   
   for(i=0; i < VB->PrimitiveCount; i++){
       GLuint prim = VB->Primitive[i].mode;
       GLuint start = VB->Primitive[i].start;
       GLuint length = VB->Primitive[i].count;
	r300_render_flat_primitive(rmesa, ctx, start, start + length, prim);
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
   
   r300_setup_routing(rmesa, ctx, GL_FALSE);
   
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
	
    /* This sequence is required after any 3d drawing packet
      I suspect it work arounds a bug (or deficiency) in hardware */
  
  reg_start(R300_RB3D_DSTCACHE_CTLSTAT,0);
	e32(0x0000000a);
   
   reg_start(0x4f18,0);
	e32(0x00000003);
   
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
		
   		#if 1
		fprintf(stderr,"ObjPtr: size=%d stride=%d\n", 
			VB->ObjPtr->size, VB->ObjPtr->stride);
		fprintf(stderr,"ColorPtr[0]: size=%d stride=%d\n", 
			VB->ColorPtr[0]->size, VB->ColorPtr[0]->stride);
		fprintf(stderr,"TexCoordPtr[0]: size=%d stride=%d\n", 
			VB->TexCoordPtr[0]->size, VB->TexCoordPtr[0]->stride);
		#endif
   
   if(type<0)return;


   start_immediate_packet(end-start, type, 12);

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
   /* Only do 2d textures */
   struct gl_texture_object *to=ctx->Texture.Unit[0].Current2D;
   r300TexObjPtr t=to->DriverData;
   LOCAL_VARS
	
   
   /* Update texture state - needs to be done only when actually changed..
      All the time for now.. */
   r300UpdateTextureState(ctx);
   
   r300_setup_routing(rmesa, ctx, GL_TRUE);
   r300_setup_textures(rmesa, ctx);
   exit(-1);
      
   /* Flush state - make sure command buffer is nice and large */
   r300Flush(ctx);
   
   //fprintf(stderr, "You can enable texture drawing in %s:%s \n", __FILE__, __FUNCTION__);
   //return GL_TRUE;


	if (RADEON_DEBUG == DEBUG_PRIMS)
		fprintf(stderr, "%s\n", __FUNCTION__);

     
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
   
   SINGLE_TEXTURE_PIPELINE.vertex_shader.matrix[0].length=16;
   memcpy(SINGLE_TEXTURE_PIPELINE.vertex_shader.matrix[0].body.f, ctx->_ModelProjectMatrix.m, 16*4);

   SINGLE_TEXTURE_PIPELINE.vertex_shader.unknown2.length=4;
   SINGLE_TEXTURE_PIPELINE.vertex_shader.unknown2.body.f[0]=0.0;
   SINGLE_TEXTURE_PIPELINE.vertex_shader.unknown2.body.f[1]=0.0;
   SINGLE_TEXTURE_PIPELINE.vertex_shader.unknown2.body.f[2]=1.0;
   SINGLE_TEXTURE_PIPELINE.vertex_shader.unknown2.body.f[3]=0.0;

   /* Use actual texture offset */
   
   fprintf(stderr,"pp_border_color=%08x pp_cubic_faces=%08x format=%08x size=%08x format_x=%08x\n", 
   	t->pp_border_color, t->pp_cubic_faces, t->format, t->size, t->format_x);
   
   SINGLE_TEXTURE_PIPELINE.texture_unit[0].offset=rmesa->radeon.radeonScreen->fbLocation+t->offset;
   #if 0
   SINGLE_TEXTURE_PIPELINE.texture_unit[0].format=t->format;
   #endif
   SINGLE_TEXTURE_PIPELINE.texture_unit[0].size=t->size;
   SINGLE_TEXTURE_PIPELINE.texture_unit[0].filter=t->filter;
   SINGLE_TEXTURE_PIPELINE.texture_unit[0].unknown1=t->pitch; /* Unknown 1 is pitch ! */
   SINGLE_TEXTURE_PIPELINE.texture_unit[0].filter=t->filter;
   
   
   /* Program RS unit. This needs to be moved into R300 pipeline */   
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

  reg_start(R300_RS_CNTL_0,0);
	e32(0x00040084);

   /* Magic register - note it is right after 20b0 */
   
   reg_start(0x20b4,0);
	e32(0x0000000c);
   
   program_pipeline(PASS_PREFIX &SINGLE_TEXTURE_PIPELINE);
         
   /* We need LOAD_VBPNTR to setup AOS_ATTR fields.. the offsets are irrelevant */
   r300EmitLOAD_VBPNTR(rmesa, 0);
   
   for(i=0; i < VB->PrimitiveCount; i++){
       GLuint prim = VB->Primitive[i].mode;
       GLuint start = VB->Primitive[i].start;
       GLuint length = VB->Primitive[i].count;
	r300_render_tex_primitive(rmesa, ctx, start, start + length, prim);
   	}
	
    /* This sequence is required after any 3d drawing packet
      I suspect it work arounds a bug (or deficiency) in hardware */
  
   reg_start(R300_RB3D_DSTCACHE_CTLSTAT,0);
	e32(0x0000000a);
   
   reg_start(0x4f18,0);
	e32(0x00000003);
         
//   exit(-1);
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
	#if 0 /* This should work now.. */
	FALLBACK_IF(ctx->Color.AlphaEnabled); // GL_ALPHA_TEST
	#endif
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

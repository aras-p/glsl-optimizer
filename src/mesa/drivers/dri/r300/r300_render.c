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
#include "tnl/t_vp_build.h"

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
#include "r300_maos.h"
#include "r300_emit.h"

extern int future_hw_tcl_on;

/**********************************************************************
*                     Hardware rasterization
*
* When we fell back to software TCL, we still try to use the
* rasterization hardware for rendering.
**********************************************************************/

static int r300_get_primitive_type(r300ContextPtr rmesa, GLcontext *ctx, int prim)
{
	int type=-1;

	switch (prim & PRIM_MODE_MASK) {
	case GL_POINTS:
	        type=R300_VAP_VF_CNTL__PRIM_POINTS;
      		break;
	case GL_LINES:
	        type=R300_VAP_VF_CNTL__PRIM_LINES;
      		break;
	case GL_LINE_STRIP:
	        type=R300_VAP_VF_CNTL__PRIM_LINE_STRIP;
      		break;
	case GL_LINE_LOOP:
		type=R300_VAP_VF_CNTL__PRIM_LINE_LOOP;
      		break;
    	case GL_TRIANGLES:
	        type=R300_VAP_VF_CNTL__PRIM_TRIANGLES;
      		break;
   	case GL_TRIANGLE_STRIP:
	        type=R300_VAP_VF_CNTL__PRIM_TRIANGLE_STRIP;
      		break;
   	case GL_TRIANGLE_FAN:
	        type=R300_VAP_VF_CNTL__PRIM_TRIANGLE_FAN;
      		break;
	case GL_QUADS:
	        type=R300_VAP_VF_CNTL__PRIM_QUADS;
      		break;
	case GL_QUAD_STRIP:
	        type=R300_VAP_VF_CNTL__PRIM_QUAD_STRIP;
      		break;
	case GL_POLYGON:
		type=R300_VAP_VF_CNTL__PRIM_POLYGON;
		break;
   	default:
 		fprintf(stderr, "%s:%s Do not know how to handle primitive %02x - help me !\n",
			__FILE__, __FUNCTION__,
			prim & PRIM_MODE_MASK);
		return -1;
         	break;
   	}
   return type;
}

int r300_get_num_verts(r300ContextPtr rmesa, int num_verts, int prim)
{
	int verts_off=0;
	char *name="UNKNOWN";

	switch (prim & PRIM_MODE_MASK) {
	case GL_POINTS:
   		name="P";
		verts_off = 0;
      		break;
	case GL_LINES:
   		name="L";
		verts_off = num_verts % 2;
      		break;
	case GL_LINE_STRIP:
   		name="LS";
		if(num_verts < 2)
			verts_off = num_verts;
      		break;
	case GL_LINE_LOOP:
   		name="LL";
		if(num_verts < 2)
			verts_off = num_verts;
      		break;
    	case GL_TRIANGLES:
   		name="T";
		verts_off = num_verts % 3;
      		break;
   	case GL_TRIANGLE_STRIP:
   		name="TS";
		if(num_verts < 3)
			verts_off = num_verts;
      		break;
   	case GL_TRIANGLE_FAN:
   		name="TF";
		if(num_verts < 3)
			verts_off = num_verts;
      		break;
	case GL_QUADS:
   		name="Q";
		verts_off = num_verts % 4;
      		break;
	case GL_QUAD_STRIP:
   		name="QS";
		if(num_verts < 4)
			verts_off = num_verts;
		else
			verts_off = num_verts % 2;
      		break;
	case GL_POLYGON:
		name="P";
		if(num_verts < 3)
			verts_off = num_verts;
		break;
   	default:
 		fprintf(stderr, "%s:%s Do not know how to handle primitive %02x - help me !\n",
			__FILE__, __FUNCTION__,
			prim & PRIM_MODE_MASK);
		return -1;
         	break;
   	}

	if (RADEON_DEBUG & DEBUG_VERTS) {
		if (num_verts - verts_off == 0) {
			WARN_ONCE("user error: Need more than %d vertices to draw primitive %s !\n", num_verts, name);
			return 0;
		}

		if (verts_off > 0) {
			WARN_ONCE("user error: %d is not a valid number of vertices for primitive %s !\n", num_verts, name);
		}
	}

	return num_verts - verts_off;
}

/* Immediate implementation has been removed from CVS. */

/* vertex buffer implementation */

static void inline fire_EB(r300ContextPtr rmesa, unsigned long addr, int vertex_count, int type, int elt_size)
{
	int cmd_reserved = 0;
	int cmd_written = 0;
	drm_radeon_cmd_header_t *cmd = NULL;
	unsigned long addr_a;
	unsigned long t_addr;
	unsigned long magic_1, magic_2;
	GLcontext *ctx;
	ctx = rmesa->radeon.glCtx; 
	
	assert(elt_size == 2 || elt_size == 4);
	
	if(addr & (elt_size-1)){
		WARN_ONCE("Badly aligned buffer\n");
		return ;
	}
#ifdef OPTIMIZE_ELTS
	addr_a = 0;
	
	magic_1 = (addr % 32) / 4;
	t_addr = addr & (~0x1d);
	magic_2 = (vertex_count + 1 + (t_addr & 0x2)) / 2 + magic_1;
	
	check_space(6);
	
	start_packet3(RADEON_CP_PACKET3_3D_DRAW_INDX_2, 0);
	if(elt_size == 4){
		e32(R300_VAP_VF_CNTL__PRIM_WALK_INDICES | (vertex_count<<16) | type | R300_VAP_VF_CNTL__INDEX_SIZE_32bit);
	} else {
		e32(R300_VAP_VF_CNTL__PRIM_WALK_INDICES | (vertex_count<<16) | type);
	}

	start_packet3(RADEON_CP_PACKET3_INDX_BUFFER, 2);
	if(elt_size == 4){
		e32(R300_EB_UNK1 | (0 << 16) | R300_EB_UNK2);
		e32(addr /*& 0xffffffe3*/);
	} else {
		e32(R300_EB_UNK1 | (magic_1 << 16) | R300_EB_UNK2);
		e32(t_addr);
	}
	
	if(elt_size == 4){
		e32(vertex_count /*+ addr_a/4*/); /* Total number of dwords needed? */
	} else {
		e32(magic_2); /* Total number of dwords needed? */
	}
	//cp_delay(rmesa, 1);
#if 0
	fprintf(stderr, "magic_1 %d\n", magic_1);
	fprintf(stderr, "t_addr %x\n", t_addr);
	fprintf(stderr, "magic_2 %d\n", magic_2);
	exit(1);
#endif
#else
	(void)magic_2, (void)magic_1, (void)t_addr;
	
	addr_a = 0;
	
	check_space(6);
	
	start_packet3(RADEON_CP_PACKET3_3D_DRAW_INDX_2, 0);
	if(elt_size == 4){
		e32(R300_VAP_VF_CNTL__PRIM_WALK_INDICES | (vertex_count<<16) | type | R300_VAP_VF_CNTL__INDEX_SIZE_32bit);
	} else {
		e32(R300_VAP_VF_CNTL__PRIM_WALK_INDICES | (vertex_count<<16) | type);
	}

	start_packet3(RADEON_CP_PACKET3_INDX_BUFFER, 2);
	e32(R300_EB_UNK1 | (0 << 16) | R300_EB_UNK2);
	e32(addr /*& 0xffffffe3*/);
	
	if(elt_size == 4){
		e32(vertex_count /*+ addr_a/4*/); /* Total number of dwords needed? */
	} else {
		e32((vertex_count+1)/2 /*+ addr_a/4*/); /* Total number of dwords needed? */
	}
	//cp_delay(rmesa, 1);
#endif	
}

static void r300_render_vb_primitive(r300ContextPtr rmesa,
	GLcontext *ctx,
	int start,
	int end,
	int prim)
{
   int type, num_verts;

   type=r300_get_primitive_type(rmesa, ctx, prim);
   num_verts=r300_get_num_verts(rmesa, end-start, prim);

   if(type<0 || num_verts <= 0)return;

   if(rmesa->state.VB.Elts){
	r300EmitAOS(rmesa, rmesa->state.aos_count, /*0*/start);
#if 0
	int cmd_reserved = 0;
	int cmd_written = 0;
	drm_radeon_cmd_header_t *cmd = NULL;
	int i;
	start_index32_packet(num_verts, type);
	for(i=0; i < num_verts; i++)
		e32(((unsigned long *)rmesa->state.VB.Elts)[i]/*rmesa->state.Elts[start+i]*/); /* start ? */
#else
	if(num_verts == 1){
		//start_index32_packet(num_verts, type);
		//e32(rmesa->state.Elts[start]);
		return;
	}
	
	if(num_verts > 65535){ /* not implemented yet */
		WARN_ONCE("Too many elts\n");
		return;
	}
	
	r300EmitElts(ctx, rmesa->state.VB.Elts, num_verts, rmesa->state.VB.elt_size);
	fire_EB(rmesa, rmesa->state.elt_dma.aos_offset, num_verts, type, rmesa->state.VB.elt_size);
#endif
   }else{
	   r300EmitAOS(rmesa, rmesa->state.aos_count, start);
	   fire_AOS(rmesa, num_verts, type);
   }
}

GLboolean r300_run_vb_render(GLcontext *ctx,
				 struct tnl_pipeline_stage *stage)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	struct radeon_vertex_buffer *VB = &rmesa->state.VB;
	int i;
	int cmd_reserved = 0;
	int cmd_written = 0;
	drm_radeon_cmd_header_t *cmd = NULL;

   
	if (RADEON_DEBUG & DEBUG_PRIMS)
		fprintf(stderr, "%s\n", __FUNCTION__);

	if (stage) {
 		TNLcontext *tnl = TNL_CONTEXT(ctx);
		radeon_vb_to_rvb(rmesa, VB, &tnl->vb);
	}
	
	r300UpdateShaders(rmesa);
	if (r300EmitArrays(ctx))
		return GL_TRUE;

	r300UpdateShaderStates(rmesa);
	
	reg_start(R300_RB3D_DSTCACHE_CTLSTAT,0);
	e32(0x0000000a);

	reg_start(0x4f18,0);
	e32(0x00000003);
	
	r300EmitState(rmesa);
	
	for(i=0; i < VB->PrimitiveCount; i++){
		GLuint prim = VB->Primitive[i].mode;
		GLuint start = VB->Primitive[i].start;
		GLuint length = VB->Primitive[i].count;
		
		r300_render_vb_primitive(rmesa, ctx, start, start + length, prim);
	}

	reg_start(R300_RB3D_DSTCACHE_CTLSTAT,0);
	e32(0x0000000a/*0x2*/);

	reg_start(0x4f18,0);
	e32(0x00000003/*0x1*/);

#ifdef USER_BUFFERS
	r300UseArrays(ctx);
#endif
	r300ReleaseArrays(ctx);
	return GL_FALSE;
}

#define FALLBACK_IF(expr)						\
	do {								\
		if (expr) {						\
			if (1 || RADEON_DEBUG & DEBUG_FALLBACKS)	\
				WARN_ONCE("Software fallback:%s\n",	\
					  #expr);			\
			return R300_FALLBACK_RAST;			\
		}							\
	} while(0)

int r300Fallback(GLcontext *ctx)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	int i;

	/* We do not do SELECT or FEEDBACK (yet ?)
	 * Is it worth doing them ?
	 */
	FALLBACK_IF(ctx->RenderMode != GL_RENDER);

#if 0
	/* These should work now.. */
	FALLBACK_IF(ctx->Color.DitherFlag);
	/* GL_ALPHA_TEST */
	FALLBACK_IF(ctx->Color.AlphaEnabled);
	/* GL_BLEND */
	FALLBACK_IF(ctx->Color.BlendEnabled);
	/* GL_POLYGON_OFFSET_FILL */
	FALLBACK_IF(ctx->Polygon.OffsetFill);
	/* FOG seems to trigger an unknown output
	 *  in vertex program.
	 */
	FALLBACK_IF(ctx->Fog.Enabled);
#endif

	if(!r300->disable_lowimpact_fallback){
		/* GL_POLYGON_OFFSET_POINT */
		FALLBACK_IF(ctx->Polygon.OffsetPoint);
		/* GL_POLYGON_OFFSET_LINE */
		FALLBACK_IF(ctx->Polygon.OffsetLine);
#if 0
		/* GL_STENCIL_TEST */
		FALLBACK_IF(ctx->Stencil.Enabled);
		/* GL_POLYGON_SMOOTH disabling to get blender going */
		FALLBACK_IF(ctx->Polygon.SmoothFlag);
#endif
		/* GL_POLYGON_STIPPLE */
		FALLBACK_IF(ctx->Polygon.StippleFlag);
		/* GL_MULTISAMPLE_ARB */
		FALLBACK_IF(ctx->Multisample.Enabled);
		/* blender ? */
		FALLBACK_IF(ctx->Line.StippleFlag);
		/* GL_LINE_SMOOTH */
		FALLBACK_IF(ctx->Line.SmoothFlag);
		/* GL_POINT_SMOOTH */
		FALLBACK_IF(ctx->Point.SmoothFlag);
	}

	/* Fallback for LOGICOP */
	FALLBACK_IF(ctx->Color.ColorLogicOpEnabled);

	/* Rest could be done with vertex fragments */
	if (ctx->Extensions.NV_point_sprite ||
	    ctx->Extensions.ARB_point_sprite)
		/* GL_POINT_SPRITE_NV */
		FALLBACK_IF(ctx->Point.PointSprite);

	/* Fallback for rectangular texture */
	for (i = 0; i < ctx->Const.MaxTextureUnits; i++)
		if (ctx->Texture.Unit[i]._ReallyEnabled & TEXTURE_RECT_BIT)
			return R300_FALLBACK_TCL;

	return R300_FALLBACK_NONE;
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

	if (RADEON_DEBUG & DEBUG_PRIMS)
		fprintf(stderr, "%s\n", __FUNCTION__);

	if (r300Fallback(ctx) >= R300_FALLBACK_RAST)
		return GL_TRUE;

	return r300_run_vb_render(ctx, stage);
}

const struct tnl_pipeline_stage _r300_render_stage = {
	"r300 hw rasterize",
	NULL,
	NULL,
	NULL,
	NULL,
	r300_run_render		/* run */
};

static GLboolean r300_run_tcl_render(GLcontext *ctx,
				 struct tnl_pipeline_stage *stage)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	struct r300_vertex_program *vp;
   
   	hw_tcl_on=future_hw_tcl_on;
   
	if (RADEON_DEBUG & DEBUG_PRIMS)
		fprintf(stderr, "%s\n", __FUNCTION__);
	if(hw_tcl_on == GL_FALSE)
		return GL_TRUE;
	
	if (r300Fallback(ctx) >= R300_FALLBACK_TCL) {
		hw_tcl_on = GL_FALSE;
		return GL_TRUE;
	}
	
	r300UpdateShaders(rmesa);

	vp = (struct r300_vertex_program *)CURRENT_VERTEX_SHADER(ctx);
#if 0 /* Draw every second request with software arb vp */
	vp->native++;
	vp->native &= 1;
	//vp->native = GL_FALSE;
#endif

#if 0 /* You dont want to know what this does... */
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	struct tnl_cache *cache;
	struct tnl_cache_item *c;
	
	cache = tnl->vp_cache;
	c = cache->items[0xc000cc0e % cache->size];
	
	if(c && c->data == vp)
		vp->native = GL_FALSE;
	
#endif
#if 0
	vp->native = GL_FALSE;
#endif
	if (vp->native == GL_FALSE) {
		hw_tcl_on = GL_FALSE;
		return GL_TRUE;
	}
	//r300UpdateShaderStates(rmesa);
	
	return r300_run_vb_render(ctx, stage);
}

const struct tnl_pipeline_stage _r300_tcl_stage = {
	"r300 tcl",
	NULL,
	NULL,
	NULL,
	NULL,
	r300_run_tcl_render	/* run */
};

/* R300 texture rectangle expects coords in 0..1 range, not 0..dimension
 * as in the extension spec.  Need to translate here.
 *
 * Note that swrast expects 0..dimension, so if a fallback is active,
 * don't do anything.  (Maybe need to configure swrast to match hw)
 */
struct texrect_stage_data {
   GLvector4f texcoord[MAX_TEXTURE_UNITS];
};

#define TEXRECT_STAGE_DATA(stage) ((struct texrect_stage_data *)stage->privatePtr)


static GLboolean run_texrect_stage( GLcontext *ctx,
				    struct tnl_pipeline_stage *stage )
{
   struct texrect_stage_data *store = TEXRECT_STAGE_DATA(stage);
   r300ContextPtr rmesa = R300_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint i;

   if (rmesa->radeon.Fallback)
      return GL_TRUE;

   for (i = 0 ; i < ctx->Const.MaxTextureUnits ; i++) {
      if (ctx->Texture.Unit[i]._ReallyEnabled & TEXTURE_RECT_BIT) {
	 struct gl_texture_object *texObj = ctx->Texture.Unit[i].CurrentRect;
	 struct gl_texture_image *texImage = texObj->Image[0][texObj->BaseLevel];
	 const GLfloat iw = 1.0/texImage->Width;
	 const GLfloat ih = 1.0/texImage->Height;
	 GLfloat *in = (GLfloat *)VB->TexCoordPtr[i]->data;
	 GLint instride = VB->TexCoordPtr[i]->stride;
	 GLfloat (*out)[4] = store->texcoord[i].data;
	 GLint j;

	 store->texcoord[i].size = VB->TexCoordPtr[i]->size;
	 for (j = 0 ; j < VB->Count ; j++) {
	    switch (VB->TexCoordPtr[i]->size) {
	    case 4:
	       out[j][3] = in[3];
	    /* fallthrough */
	    case 3:
	       out[j][2] = in[2];
	    /* fallthrough */
	    default:
	       out[j][0] = in[0] * iw;
	       out[j][1] = in[1] * ih;
	    }
	    in = (GLfloat *)((GLubyte *)in + instride);
	 }

	 VB->AttribPtr[VERT_ATTRIB_TEX0+i] = VB->TexCoordPtr[i] = &store->texcoord[i];
      }
   }

   return GL_TRUE;
}


/* Called the first time stage->run() is invoked.
 */
static GLboolean alloc_texrect_data( GLcontext *ctx,
				     struct tnl_pipeline_stage *stage )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   struct texrect_stage_data *store;
   GLuint i;

   stage->privatePtr = CALLOC(sizeof(*store));
   store = TEXRECT_STAGE_DATA(stage);
   if (!store)
      return GL_FALSE;

   for (i = 0 ; i < ctx->Const.MaxTextureUnits ; i++)
      _mesa_vector4f_alloc( &store->texcoord[i], 0, VB->Size, 32 );

   return GL_TRUE;
}

static void free_texrect_data( struct tnl_pipeline_stage *stage )
{
   struct texrect_stage_data *store = TEXRECT_STAGE_DATA(stage);
   GLuint i;

   if (store) {
      for (i = 0 ; i < MAX_TEXTURE_UNITS ; i++)
	 if (store->texcoord[i].data)
	    _mesa_vector4f_free( &store->texcoord[i] );
      FREE( store );
      stage->privatePtr = NULL;
   }
}

const struct tnl_pipeline_stage _r300_texrect_stage =
{
   "r300 texrect stage",			/* name */
   NULL,
   alloc_texrect_data,
   free_texrect_data,
   NULL,
   run_texrect_stage
};


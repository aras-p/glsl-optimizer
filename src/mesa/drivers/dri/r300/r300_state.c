/*
Copyright (C) The Weather Channel, Inc.  2002.
Copyright (C) 2004 Nicolai Haehnle.
All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

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
#include "r300_fixed_pipelines.h"

static void r300AlphaFunc(GLcontext * ctx, GLenum func, GLfloat ref)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	int pp_misc = rmesa->hw.at.cmd[R300_AT_ALPHA_TEST];
	GLubyte refByte;

	CLAMPED_FLOAT_TO_UBYTE(refByte, ref);

	R300_STATECHANGE(rmesa, at);

	pp_misc &= ~(R300_ALPHA_TEST_OP_MASK | R300_REF_ALPHA_MASK);
	pp_misc |= (refByte & R300_REF_ALPHA_MASK);

	switch (func) {
	case GL_NEVER:
		pp_misc |= R300_ALPHA_TEST_FAIL;
		break;
	case GL_LESS:
		pp_misc |= R300_ALPHA_TEST_LESS;
		break;
	case GL_EQUAL:
		pp_misc |= R300_ALPHA_TEST_EQUAL;
		break;
	case GL_LEQUAL:
		pp_misc |= R300_ALPHA_TEST_LEQUAL;
		break;
	case GL_GREATER:
		pp_misc |= R300_ALPHA_TEST_GREATER;
		break;
	case GL_NOTEQUAL:
		pp_misc |= R300_ALPHA_TEST_NEQUAL;
		break;
	case GL_GEQUAL:
		pp_misc |= R300_ALPHA_TEST_GEQUAL;
		break;
	case GL_ALWAYS:
		pp_misc |= R300_ALPHA_TEST_PASS;
		break;
	}

	rmesa->hw.at.cmd[R300_AT_ALPHA_TEST] = pp_misc;
}

static void r300BlendColor(GLcontext * ctx, const GLfloat cf[4])
{
	GLubyte color[4];
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	fprintf(stderr, "%s:%s is not implemented yet. Fixme !\n", __FILE__, __FUNCTION__);
	#if 0
	R200_STATECHANGE(rmesa, ctx);
	CLAMPED_FLOAT_TO_UBYTE(color[0], cf[0]);
	CLAMPED_FLOAT_TO_UBYTE(color[1], cf[1]);
	CLAMPED_FLOAT_TO_UBYTE(color[2], cf[2]);
	CLAMPED_FLOAT_TO_UBYTE(color[3], cf[3]);
	if (rmesa->radeon.radeonScreen->drmSupportsBlendColor)
		rmesa->hw.ctx.cmd[CTX_RB3D_BLENDCOLOR] =
		    radeonPackColor(4, color[0], color[1], color[2], color[3]);
	#endif
}

/**
 * Calculate the hardware blend factor setting.  This same function is used
 * for source and destination of both alpha and RGB.
 *
 * \returns
 * The hardware register value for the specified blend factor.  This value
 * will need to be shifted into the correct position for either source or
 * destination factor.
 *
 * \todo
 * Since the two cases where source and destination are handled differently
 * are essentially error cases, they should never happen.  Determine if these
 * cases can be removed.
 */
static int blend_factor(GLenum factor, GLboolean is_src)
{
	int func;

	switch (factor) {
	case GL_ZERO:
		func = R200_BLEND_GL_ZERO;
		break;
	case GL_ONE:
		func = R200_BLEND_GL_ONE;
		break;
	case GL_DST_COLOR:
		func = R200_BLEND_GL_DST_COLOR;
		break;
	case GL_ONE_MINUS_DST_COLOR:
		func = R200_BLEND_GL_ONE_MINUS_DST_COLOR;
		break;
	case GL_SRC_COLOR:
		func = R200_BLEND_GL_SRC_COLOR;
		break;
	case GL_ONE_MINUS_SRC_COLOR:
		func = R200_BLEND_GL_ONE_MINUS_SRC_COLOR;
		break;
	case GL_SRC_ALPHA:
		func = R200_BLEND_GL_SRC_ALPHA;
		break;
	case GL_ONE_MINUS_SRC_ALPHA:
		func = R200_BLEND_GL_ONE_MINUS_SRC_ALPHA;
		break;
	case GL_DST_ALPHA:
		func = R200_BLEND_GL_DST_ALPHA;
		break;
	case GL_ONE_MINUS_DST_ALPHA:
		func = R200_BLEND_GL_ONE_MINUS_DST_ALPHA;
		break;
	case GL_SRC_ALPHA_SATURATE:
		func =
		    (is_src) ? R200_BLEND_GL_SRC_ALPHA_SATURATE :
		    R200_BLEND_GL_ZERO;
		break;
	case GL_CONSTANT_COLOR:
		func = R200_BLEND_GL_CONST_COLOR;
		break;
	case GL_ONE_MINUS_CONSTANT_COLOR:
		func = R200_BLEND_GL_ONE_MINUS_CONST_COLOR;
		break;
	case GL_CONSTANT_ALPHA:
		func = R200_BLEND_GL_CONST_ALPHA;
		break;
	case GL_ONE_MINUS_CONSTANT_ALPHA:
		func = R200_BLEND_GL_ONE_MINUS_CONST_ALPHA;
		break;
	default:
		func = (is_src) ? R200_BLEND_GL_ONE : R200_BLEND_GL_ZERO;
	}
	return func;
}

/**
 * Sets both the blend equation and the blend function.
 * This is done in a single
 * function because some blend equations (i.e., \c GL_MIN and \c GL_MAX)
 * change the interpretation of the blend function.
 * Also, make sure that blend function and blend equation are set to their default
 * value if color blending is not enabled, since at least blend equations GL_MIN
 * and GL_FUNC_REVERSE_SUBTRACT will cause wrong results otherwise for
 * unknown reasons.
 */
static void r300_set_blend_state(GLcontext * ctx)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	#if 0
	GLuint cntl = rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] &
	    ~(R300_ROP_ENABLE | R300_ALPHA_BLEND_ENABLE |
	      R300_SEPARATE_ALPHA_ENABLE);
	#endif

	int func = (R200_BLEND_GL_ONE << R200_SRC_BLEND_SHIFT) |
	    (R200_BLEND_GL_ZERO << R200_DST_BLEND_SHIFT);
	int eqn = R200_COMB_FCN_ADD_CLAMP;
	int funcA = (R200_BLEND_GL_ONE << R200_SRC_BLEND_SHIFT) |
	    (R200_BLEND_GL_ZERO << R200_DST_BLEND_SHIFT);
	int eqnA = R200_COMB_FCN_ADD_CLAMP;

	R300_STATECHANGE(rmesa, bld);

	if (rmesa->radeon.radeonScreen->drmSupportsBlendColor) {
		if (ctx->Color._LogicOpEnabled) {
			#if 0
			rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] =
			    cntl | R300_ROP_ENABLE;
			#endif
			rmesa->hw.bld.cmd[R300_BLD_ABLEND] = eqn | func;
			rmesa->hw.bld.cmd[R300_BLD_CBLEND] = eqn | func;
			return;
		} else if (ctx->Color.BlendEnabled) {
			#if 0
			rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] =
			    cntl | R300_ALPHA_BLEND_ENABLE |
			    R300_SEPARATE_ALPHA_ENABLE;
			#endif
		} else {
			#if 0
			rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] = cntl;
			#endif
			rmesa->hw.bld.cmd[R300_BLD_ABLEND] = eqn | func;
			rmesa->hw.bld.cmd[R300_BLD_CBLEND] = eqn | func;
			return;
		}
	} else {
		if (ctx->Color._LogicOpEnabled) {
			#if 0
			rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] =
			    cntl | R300_ROP_ENABLE;
			rmesa->hw.ctx.cmd[CTX_RB3D_BLENDCNTL] = eqn | func;
			#endif
			return;
		} else if (ctx->Color.BlendEnabled) {
			#if 0
			rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] =
			    cntl | R300_ALPHA_BLEND_ENABLE;
			#endif
		} else {
			#if 0
			rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] = cntl;
			rmesa->hw.ctx.cmd[CTX_RB3D_BLENDCNTL] = eqn | func;
			#endif
			return;
		}
	}

	func =
	    (blend_factor(ctx->Color.BlendSrcRGB, GL_TRUE) <<
	     R200_SRC_BLEND_SHIFT) | (blend_factor(ctx->Color.BlendDstRGB,
						   GL_FALSE) <<
				      R200_DST_BLEND_SHIFT);

	switch (ctx->Color.BlendEquationRGB) {
	case GL_FUNC_ADD:
		eqn = R300_COMB_FCN_ADD_CLAMP;
		break;

	case GL_FUNC_SUBTRACT:
		eqn = R300_COMB_FCN_SUB_CLAMP;
		break;

	case GL_FUNC_REVERSE_SUBTRACT:
		eqn = R200_COMB_FCN_RSUB_CLAMP;
		break;

	case GL_MIN:
		eqn = R200_COMB_FCN_MIN;
		func = (R200_BLEND_GL_ONE << R200_SRC_BLEND_SHIFT) |
		    (R200_BLEND_GL_ONE << R200_DST_BLEND_SHIFT);
		break;

	case GL_MAX:
		eqn = R200_COMB_FCN_MAX;
		func = (R200_BLEND_GL_ONE << R200_SRC_BLEND_SHIFT) |
		    (R200_BLEND_GL_ONE << R200_DST_BLEND_SHIFT);
		break;

	default:
		fprintf(stderr,
			"[%s:%u] Invalid RGB blend equation (0x%04x).\n",
			__func__, __LINE__, ctx->Color.BlendEquationRGB);
		return;
	}

	if (!rmesa->radeon.radeonScreen->drmSupportsBlendColor) {
		#if 0
		rmesa->hw.ctx.cmd[CTX_RB3D_BLENDCNTL] = eqn | func;
		#endif
		return;
	}

	funcA =
	    (blend_factor(ctx->Color.BlendSrcA, GL_TRUE) <<
	     R200_SRC_BLEND_SHIFT) | (blend_factor(ctx->Color.BlendDstA,
						   GL_FALSE) <<
				      R200_DST_BLEND_SHIFT);

	switch (ctx->Color.BlendEquationA) {
	case GL_FUNC_ADD:
		eqnA = R300_COMB_FCN_ADD_CLAMP;
		break;

	case GL_FUNC_SUBTRACT:
		eqnA = R300_COMB_FCN_SUB_CLAMP;
		break;

	case GL_FUNC_REVERSE_SUBTRACT:
		eqnA = R200_COMB_FCN_RSUB_CLAMP;
		break;

	case GL_MIN:
		eqnA = R200_COMB_FCN_MIN;
		funcA = (R200_BLEND_GL_ONE << R200_SRC_BLEND_SHIFT) |
		    (R200_BLEND_GL_ONE << R200_DST_BLEND_SHIFT);
		break;

	case GL_MAX:
		eqnA = R200_COMB_FCN_MAX;
		funcA = (R200_BLEND_GL_ONE << R200_SRC_BLEND_SHIFT) |
		    (R200_BLEND_GL_ONE << R200_DST_BLEND_SHIFT);
		break;

	default:
		fprintf(stderr, "[%s:%u] Invalid A blend equation (0x%04x).\n",
			__func__, __LINE__, ctx->Color.BlendEquationA);
		return;
	}

	rmesa->hw.bld.cmd[R300_BLD_ABLEND] = eqnA | funcA;
	rmesa->hw.bld.cmd[R300_BLD_CBLEND] = eqn | func ;
	if(rmesa->hw.bld.cmd[R300_BLD_ABLEND] == rmesa->hw.bld.cmd[R300_BLD_CBLEND]){
		rmesa->hw.bld.cmd[R300_BLD_CBLEND] |= R300_BLEND_UNKNOWN | R300_BLEND_ENABLE | R300_BLEND_NO_SEPARATE;
		} else {
		rmesa->hw.bld.cmd[R300_BLD_CBLEND] |= R300_BLEND_UNKNOWN | R300_BLEND_ENABLE;
		}

}

static void r300BlendEquationSeparate(GLcontext * ctx,
				      GLenum modeRGB, GLenum modeA)
{
	r300_set_blend_state(ctx);
}

static void r300BlendFuncSeparate(GLcontext * ctx,
				  GLenum sfactorRGB, GLenum dfactorRGB,
				  GLenum sfactorA, GLenum dfactorA)
{
	r300_set_blend_state(ctx);
}

/**
 * Update our tracked culling state based on Mesa's state.
 */
static void r300UpdateCulling(GLcontext* ctx)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	uint32_t val = 0;

	R300_STATECHANGE(r300, cul);
	if (ctx->Polygon.CullFlag) {
		if (ctx->Polygon.CullFaceMode == GL_FRONT_AND_BACK)
			val = R300_CULL_FRONT|R300_CULL_BACK;
		else if (ctx->Polygon.CullFaceMode == GL_FRONT)
			val = R300_CULL_FRONT;
		else
			val = R300_CULL_BACK;

		if (ctx->Polygon.FrontFace == GL_CW)
			val |= R300_FRONT_FACE_CW;
		else
			val |= R300_FRONT_FACE_CCW;
	}

	r300->hw.cul.cmd[R300_CUL_CULL] = val;
}


/**
 * Handle glEnable()/glDisable().
 *
 * \note Mesa already filters redundant calls to glEnable/glDisable.
 */
static void r300Enable(GLcontext* ctx, GLenum cap, GLboolean state)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	uint32_t newval;

	if (RADEON_DEBUG & DEBUG_STATE)
		fprintf(stderr, "%s( %s = %s )\n", __FUNCTION__,
			_mesa_lookup_enum_by_nr(cap),
			state ? "GL_TRUE" : "GL_FALSE");

	switch (cap) {
	case GL_DEPTH_TEST:
		R300_STATECHANGE(r300, zc);

		if (state) {
			if (ctx->Depth.Mask)
				newval = R300_RB3D_Z_TEST_AND_WRITE;
			else
				newval = R300_RB3D_Z_TEST;
		} else
			newval = 0;

		r300->hw.zc.cmd[R300_ZC_CNTL_0] = newval;
		break;

	case GL_CULL_FACE:
		r300UpdateCulling(ctx);
		break;

	default:
		radeonEnable(ctx, cap, state);
		return;
	}
}


/**
 * Change the culling mode.
 *
 * \note Mesa already filters redundant calls to this function.
 */
static void r300CullFace(GLcontext* ctx, GLenum mode)
{
	(void)mode;

	r300UpdateCulling(ctx);
}


/**
 * Change the polygon orientation.
 *
 * \note Mesa already filters redundant calls to this function.
 */
static void r300FrontFace(GLcontext* ctx, GLenum mode)
{
	(void)mode;

	r300UpdateCulling(ctx);
}


/**
 * Change the depth testing function.
 *
 * \note Mesa already filters redundant calls to this function.
 */
static void r300DepthFunc(GLcontext* ctx, GLenum func)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);

	R300_STATECHANGE(r300, zc);

	switch(func) {
	case GL_NEVER:
		r300->hw.zc.cmd[R300_ZC_CNTL_1] = R300_Z_TEST_NEVER;
		break;
	case GL_LESS:
		r300->hw.zc.cmd[R300_ZC_CNTL_1] = R300_Z_TEST_LESS;
		break;
	case GL_EQUAL:
		r300->hw.zc.cmd[R300_ZC_CNTL_1] = R300_Z_TEST_EQUAL;
		break;
	case GL_LEQUAL:
		r300->hw.zc.cmd[R300_ZC_CNTL_1] = R300_Z_TEST_LEQUAL;
		break;
	case GL_GREATER:
		r300->hw.zc.cmd[R300_ZC_CNTL_1] = R300_Z_TEST_GREATER;
		break;
	case GL_NOTEQUAL:
		r300->hw.zc.cmd[R300_ZC_CNTL_1] = R300_Z_TEST_NEQUAL;
		break;
	case GL_GEQUAL:
		r300->hw.zc.cmd[R300_ZC_CNTL_1] = R300_Z_TEST_GEQUAL;
		break;
	case GL_ALWAYS:
		r300->hw.zc.cmd[R300_ZC_CNTL_1] = R300_Z_TEST_ALWAYS;
		break;
	}
}


/**
 * Enable/Disable depth writing.
 *
 * \note Mesa already filters redundant calls to this function.
 */
static void r300DepthMask(GLcontext* ctx, GLboolean mask)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);

	if (!ctx->Depth.Test)
		return;

	R300_STATECHANGE(r300, zc);
	r300->hw.zc.cmd[R300_ZC_CNTL_0] = mask
		? R300_RB3D_Z_TEST_AND_WRITE : R300_RB3D_Z_TEST;
}


/**
 * Handle glColorMask()
 */
static void r300ColorMask(GLcontext* ctx,
			  GLboolean r, GLboolean g, GLboolean b, GLboolean a)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	int mask = (b << 0) | (g << 1) | (r << 2) | (a << 3);

	if (mask != r300->hw.cmk.cmd[R300_CMK_COLORMASK]) {
		R300_STATECHANGE(r300, cmk);
		r300->hw.cmk.cmd[R300_CMK_COLORMASK] = mask;
	}
}

/* =============================================================
 * Point state
 */
static void r300PointSize(GLcontext * ctx, GLfloat size)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	
	/* This might need fixing later */
	R300_STATECHANGE(r300, vps);
	r300->hw.vps.cmd[R300_VPS_POINTSIZE] = r300PackFloat32(1.0);
}

/* =============================================================
 * Window position and viewport transformation
 */

/*
 * To correctly position primitives:
 */
#define SUBPIXEL_X 0.125
#define SUBPIXEL_Y 0.125

void r300UpdateWindow(GLcontext * ctx)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	__DRIdrawablePrivate *dPriv = rmesa->radeon.dri.drawable;
	GLfloat xoffset = dPriv ? (GLfloat) dPriv->x : 0;
	GLfloat yoffset = dPriv ? (GLfloat) dPriv->y + dPriv->h : 0;
	const GLfloat *v = ctx->Viewport._WindowMap.m;

	GLfloat sx = v[MAT_SX];
	GLfloat tx = v[MAT_TX] + xoffset + SUBPIXEL_X;
	GLfloat sy = -v[MAT_SY];
	GLfloat ty = (-v[MAT_TY]) + yoffset + SUBPIXEL_Y;
	GLfloat sz = v[MAT_SZ] * rmesa->state.depth.scale;
	GLfloat tz = v[MAT_TZ] * rmesa->state.depth.scale;

	R300_FIREVERTICES(rmesa);
	R300_STATECHANGE(rmesa, vpt);

	rmesa->hw.vpt.cmd[R300_VPT_XSCALE]  = r300PackFloat32(sx);
	rmesa->hw.vpt.cmd[R300_VPT_XOFFSET] = r300PackFloat32(tx);
	rmesa->hw.vpt.cmd[R300_VPT_YSCALE]  = r300PackFloat32(sy);
	rmesa->hw.vpt.cmd[R300_VPT_YOFFSET] = r300PackFloat32(ty);
	rmesa->hw.vpt.cmd[R300_VPT_ZSCALE]  = r300PackFloat32(sz);
	rmesa->hw.vpt.cmd[R300_VPT_ZOFFSET] = r300PackFloat32(tz);
}

static void r300Viewport(GLcontext * ctx, GLint x, GLint y,
			 GLsizei width, GLsizei height)
{
	/* Don't pipeline viewport changes, conflict with window offset
	 * setting below.  Could apply deltas to rescue pipelined viewport
	 * values, or keep the originals hanging around.
	 */
	R200_FIREVERTICES(R200_CONTEXT(ctx));
	r300UpdateWindow(ctx);
}

static void r300DepthRange(GLcontext * ctx, GLclampd nearval, GLclampd farval)
{
	r300UpdateWindow(ctx);
}

/* Routing and texture-related */

void r300_setup_routing(GLcontext *ctx, GLboolean immediate)
{
	int i, count=0,reg=0;
	GLuint dw, mask;
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	struct vertex_buffer *VB = &tnl->vb;
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	
	
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
	
	if (RADEON_DEBUG & DEBUG_STATE)
		fprintf(stderr, "aos_count=%d\n", count);
	
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
		//fprintf(stderr, "vir0 dw=%08x\n", dw);
		}
	/* Set the rest of INPUT_ROUTE_0 to 0 */
	//for(i=((count+1)>>1); i<8; i++)r300->hw.vir[0].cmd[R300_VIR_CNTL_0+i]=(0x0);
	((drm_r300_cmd_header_t*)r300->hw.vir[0].cmd)->unchecked_state.count = (count+1)>>1;
	
	
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
		//fprintf(stderr, "vir1 dw=%08x\n", dw);
		}
	/* Set the rest of INPUT_ROUTE_1 to 0 */
	//for(i=((count+1)>>1); i<8; i++)r300->hw.vir[1].cmd[R300_VIR_CNTL_0+i]=0x0;
	((drm_r300_cmd_header_t*)r300->hw.vir[1].cmd)->unchecked_state.count = (count+1)>>1;
	
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

static r300TexObj default_tex_obj={
	filter:R300_TX_MAG_FILTER_LINEAR | R300_TX_MIN_FILTER_LINEAR,
	pitch: 0x8000,
	size: (0xff << R300_TX_WIDTHMASK_SHIFT) 
	      | (0xff << R300_TX_HEIGHTMASK_SHIFT)
	      | (0x8 << R300_TX_SIZE_SHIFT),
	format: 0x88a0c,
	offset: 0x0,
	unknown4: 0x0,
	unknown5: 0x0
	};

	/* there is probably a system to these value, but, for now, 
	   we just try by hand */

static int inline translate_src(int src)
{
	switch (src) {
	case GL_TEXTURE:
		return 1;
		break;
	case GL_CONSTANT:
		return 2;
		break;
	case GL_PRIMARY_COLOR:
		return 3;
		break;
	case GL_PREVIOUS:
		return 4;
		break;
	case GL_ZERO:
		return 5;
		break;
	case GL_ONE:
		return 6;
		break;
	default:
		return 0;
	}
}
	   
/* I think 357 and 457 are prime numbers.. wiggle them if you get coincidences */
#define FORMAT_HASH(opRGB, srcRGB, modeRGB, opA, srcA, modeA, format, intFormat)	( \
	(\
	((opRGB)<<30) | ((opA)<<28) | \
	((srcRGB)<< 25) | ((srcA)<<22) | \
	((modeRGB)) \
	) \
	^ ((modeA)*357) \
	^ (((format)) *457) \
	^ ((intFormat) * 7) \
	)
	   
static GLuint translate_texture_format(GLcontext *ctx, GLint tex_unit, GLuint format, GLint IntFormat)
{
	const struct gl_texture_unit *texUnit= &ctx->Texture.Unit[tex_unit];
	int i=0; /* number of alpha args .. */
	
	/* Size field in format specific first */
	switch(FORMAT_HASH(							
		texUnit->_CurrentCombine->OperandRGB[i] -GL_SRC_COLOR,
		translate_src(texUnit->_CurrentCombine->SourceRGB[i]),
		texUnit->_CurrentCombine->ModeRGB,
		texUnit->_CurrentCombine->OperandA[i] -GL_SRC_ALPHA,
		translate_src(texUnit->_CurrentCombine->SourceA[i]),
		texUnit->_CurrentCombine->ModeA,
		format,
		IntFormat
		)){
	case FORMAT_HASH(0, 1, 0x2100, 0, 1, 0x2100, 0x00088047, 0x1908):
		/* tested with:
			kfiresaver.kss 
			*/
		return 0x7a0c; /* kfiresaver.kss */
	case FORMAT_HASH(0, 1, 0x2100, 0, 1, 0x2100, 0x00088047, 0x8058):
		/* tested with:
			Quake3demo 
			*/
		return 0x8a0c; /* Quake3demo -small font on the bottom */
	case FORMAT_HASH(0, 1, 0x2100, 0, 1, 0x2100, 0x00077047, 4):
		/* tested with:
			kfiresaver.kss 
			*/
		return 0x4ba0c;
	case FORMAT_HASH(0, 1, 0x2100, 0, 1, 0x2100, 0x00055047, 4):
		/* tested with:
			kfiresaver.kss
			kfountain.kss
			*/
		return 0x51a0c;	
	case FORMAT_HASH(0, 1, 0x2100, 0, 4, 0x1e01, 0x00088047, 3):
		/* tested with
			lesson 06
			lesson 07
			*/
		return 0x53a0c;
	//case FORMAT_HASH(0, 1, 0x2100, 0, 4, 0x1e01, 0x00055047, 0):
		/* Can't remember what I tested this with.. 
		   try putting return 0 of you see broken textures which 
		   are not being complained about */
		return 0x53a0c;	
	case FORMAT_HASH(0, 1, 0x2100, 0, 4, 0x1e01, 0x00099047, 0x8051):
	//case FORMAT_HASH(0, 1, 0x2100, 0, 4, 0x1e01, 0x00089047, 0x8058):
	case FORMAT_HASH(0, 1, 0x2100, 0, 4, 0x1e01, 0x00077047, 0x8051):
		/* Tested with:
			Quake3demo
			*/
		return 0x53a0c;	
	case  FORMAT_HASH(0, 1, 0x2100, 0, 1, 0x2100, 0x00055045, 0x00008056):
	case  FORMAT_HASH(0, 1, 0x2100, 0, 1, 0x2100, 0x00088045, 0x00008056):
		return 0x53a23;
	case FORMAT_HASH(0, 1, 0x2100, 0, 4, 0x1e01, 0x00099004, 0x00008050):
		return 0;
		return 0x2a0b;
	}
	//	return 0x53a0c;	
	
	/* Just key on internal format  - useful for quick tryouts*/
	
		return 0;
	#if 1
	switch(IntFormat
		){
	case 0x3:
		return 0x53a0c;
	case 0x8050:
		return 0x2a0b;
	case 0x8056:
	
		return 0x53a23;
	case 0x8058:
		return 0;
		return 0x8a0c;
		//fprintf(stderr, "%08x\n", format);
		/* Tested with:
			Quake3demo
			*/
		return 0x53a0c;	
	default:
		return 0x53a0c;
	}
	#endif
	
	
	
	{
		static int warn_once=1;
		if(warn_once){
			fprintf(stderr, "%s:%s Do not know how to translate texture format - help me !\n",
				__FILE__, __FUNCTION__);
			warn_once=0;
			}
	}
		return 0;
}
	
void r300_setup_textures(GLcontext *ctx)
{
	int i, mtu;
	struct r300_tex_obj *t;
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	int max_texture_unit=-1; /* -1 translates into no setup costs for fields */
	struct gl_texture_unit *texUnit;

	R300_STATECHANGE(r300, txe);
	R300_STATECHANGE(r300, tex.filter);
	R300_STATECHANGE(r300, tex.unknown1);
	R300_STATECHANGE(r300, tex.size);
	R300_STATECHANGE(r300, tex.format);
	R300_STATECHANGE(r300, tex.offset);
	R300_STATECHANGE(r300, tex.unknown4);
	R300_STATECHANGE(r300, tex.unknown5);
	
	r300->state.texture.tc_count=0;
	
	r300->hw.txe.cmd[R300_TXE_ENABLE]=0x0;
	
	mtu = r300->radeon.glCtx->Const.MaxTextureUnits;
	if (RADEON_DEBUG & DEBUG_STATE)
		fprintf(stderr, "mtu=%d\n", mtu);
	
	if(mtu>R300_MAX_TEXTURE_UNITS){
		fprintf(stderr, "Aiiee ! mtu=%d is greater than R300_MAX_TEXTURE_UNITS=%d\n", 
			mtu, R300_MAX_TEXTURE_UNITS);
		exit(-1);
		}
	for(i=0;i<mtu;i++){
		if(ctx->Texture.Unit[i].Enabled){
			t=r300->state.texture.unit[i].texobj;
			r300->state.texture.tc_count++;
			if(t==NULL){
				fprintf(stderr, "Texture unit %d enabled, but corresponding texobj is NULL, using default object.\n", i);
				//exit(-1);
				t=&default_tex_obj;
				}
			if (RADEON_DEBUG & DEBUG_STATE)
				fprintf(stderr, "Activating texture unit %d\n", i);
			max_texture_unit=i;
			r300->hw.txe.cmd[R300_TXE_ENABLE]|=(1<<i);
			
			r300->hw.tex.filter.cmd[R300_TEX_VALUE_0+i]=t->filter;
			r300->hw.tex.unknown1.cmd[R300_TEX_VALUE_0+i]=t->pitch;
			r300->hw.tex.size.cmd[R300_TEX_VALUE_0+i]=t->size;
			r300->hw.tex.format.cmd[R300_TEX_VALUE_0+i]=t->format;
			r300->hw.tex.offset.cmd[R300_TEX_VALUE_0+i]=r300->radeon.radeonScreen->fbLocation+t->offset;
			r300->hw.tex.unknown4.cmd[R300_TEX_VALUE_0+i]=0x0;
			r300->hw.tex.unknown5.cmd[R300_TEX_VALUE_0+i]=0x0;
			
			
			
			/* We don't know how to set this yet */
			//value from r300_lib.c for RGB24
			//r300->hw.tex.format.cmd[R300_TEX_VALUE_0+i]=0x88a0c; 
			r300->hw.tex.format.cmd[R300_TEX_VALUE_0+i]=translate_texture_format(ctx, i, t->format, t->base.tObj->Image[0][0]->IntFormat);
			/* Use the code below to quickly find matching texture
			   formats. Requires an app that displays the same texture
			   repeatedly  */
			      #if 1
				if(r300->hw.tex.format.cmd[R300_TEX_VALUE_0+i]==0){ 
					static int fmt=0x0;
					static int k=0;
					k++;
					if(k>400){
						k=0;
						fmt++;
						texUnit = &ctx->Texture.Unit[i];
						fprintf(stderr, "Want to set FORMAT_HASH(%d, %d, 0x%04x, %d, %d, 0x%04x, 0x%08x, 0x%08x)\n",
							texUnit->_CurrentCombine->OperandRGB[0] -GL_SRC_COLOR,
							translate_src(texUnit->_CurrentCombine->SourceRGB[0]),
							texUnit->_CurrentCombine->ModeRGB,
							texUnit->_CurrentCombine->OperandA[0] -GL_SRC_ALPHA,
							translate_src(texUnit->_CurrentCombine->SourceA[0]),
							texUnit->_CurrentCombine->ModeA,
							t->format,
							t->base.tObj->Image[0][0]->IntFormat
							);
						fprintf(stderr, "Also known: format_x=%08x border_color=%08x cubic_faces=%08x\n", t->format_x, t->pp_border_color, t->pp_cubic_faces);
						fprintf(stderr, "\t_ReallyEnabled=%08x EnvMode=%08x IntFormat=%08x\n", texUnit->_ReallyEnabled, texUnit->EnvMode,  t->base.tObj->Image[0][0]->IntFormat);
						if(fmt>0xff){
							//exit(-1);
							fmt=0;
							}
						//sleep(1);
						fprintf(stderr, "Now trying format %08x\n", 
							0x00a0c | (fmt<<12));
						fprintf(stderr, "size=%08x\n", t->size);
						}
					r300->hw.tex.format.cmd[R300_TEX_VALUE_0+i]=0x00a0b | (fmt<<12);
					//r300->hw.tex.format.cmd[R300_TEX_VALUE_0+i]=0x51a00 | (fmt);
					//r300->hw.tex.format.cmd[R300_TEX_VALUE_0+i]=0x53a0c | (fmt<<24);
					}
			      #endif
			
			}
			
		}
	((drm_r300_cmd_header_t*)r300->hw.tex.filter.cmd)->unchecked_state.count = max_texture_unit+1;
	((drm_r300_cmd_header_t*)r300->hw.tex.unknown1.cmd)->unchecked_state.count = max_texture_unit+1;
	((drm_r300_cmd_header_t*)r300->hw.tex.size.cmd)->unchecked_state.count = max_texture_unit+1;
	((drm_r300_cmd_header_t*)r300->hw.tex.format.cmd)->unchecked_state.count = max_texture_unit+1;
	((drm_r300_cmd_header_t*)r300->hw.tex.offset.cmd)->unchecked_state.count = max_texture_unit+1;
	((drm_r300_cmd_header_t*)r300->hw.tex.unknown4.cmd)->unchecked_state.count = max_texture_unit+1;
	((drm_r300_cmd_header_t*)r300->hw.tex.unknown5.cmd)->unchecked_state.count = max_texture_unit+1;
	
	if (RADEON_DEBUG & DEBUG_STATE)
		fprintf(stderr, "TX_ENABLE: %08x  max_texture_unit=%d\n", r300->hw.txe.cmd[R300_TXE_ENABLE], max_texture_unit);
}

void r300_setup_rs_unit(GLcontext *ctx)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	int i;
	
	/* This needs to be rewritten - it is a hack at best */
	
	R300_STATECHANGE(r300, ri);
	R300_STATECHANGE(r300, rc);
	R300_STATECHANGE(r300, rr);
	
	for(i = 1; i <= 8; ++i)
		r300->hw.ri.cmd[i] = 0x00d10000;
	r300->hw.ri.cmd[R300_RI_INTERP_1] |= R300_RS_INTERP_1_UNKNOWN;
	r300->hw.ri.cmd[R300_RI_INTERP_2] |= R300_RS_INTERP_2_UNKNOWN;
	r300->hw.ri.cmd[R300_RI_INTERP_3] |= R300_RS_INTERP_3_UNKNOWN;
	
	for(i = 1; i <= 8; ++i)
		r300->hw.rr.cmd[i] = 0;
	/* textures enabled ? */
	if(r300->state.texture.tc_count>0){
	
		/* This code only really works with one set of texture coordinates */
		
		/* The second constant is needed to get glxgears display anything .. */
		r300->hw.rc.cmd[1] = R300_RS_CNTL_0_UNKNOWN_7 
				| R300_RS_CNTL_0_UNKNOWN_18 
				| (r300->state.texture.tc_count<<R300_RS_CNTL_TC_CNT_SHIFT);
		r300->hw.rc.cmd[2] = 0xc0;
	
	
		((drm_r300_cmd_header_t*)r300->hw.rr.cmd)->unchecked_state.count = 1;
		r300->hw.rr.cmd[R300_RR_ROUTE_0] = 0x24008;
		
		} else {
		
		/* The second constant is needed to get glxgears display anything .. */
		r300->hw.rc.cmd[1] = R300_RS_CNTL_0_UNKNOWN_7 | R300_RS_CNTL_0_UNKNOWN_18;
		r300->hw.rc.cmd[2] = 0;
		
		((drm_r300_cmd_header_t*)r300->hw.rr.cmd)->unchecked_state.count = 1;
		r300->hw.rr.cmd[R300_RR_ROUTE_0] = 0x4000;
		
		}
}

#define vpucount(ptr) (((drm_r300_cmd_header_t*)(ptr))->vpu.count)

#define bump_vpu_count(ptr, new_count)   do{\
	drm_r300_cmd_header_t* _p=((drm_r300_cmd_header_t*)(ptr));\
	int _nc=(new_count)/4; \
	if(_nc>_p->vpu.count)_p->vpu.count=_nc;\
	}while(0)

void static inline setup_vertex_shader_fragment(r300ContextPtr r300, int dest, struct r300_vertex_shader_fragment *vsf)
{
	int i;
	
	if(vsf->length==0)return;
	
	if(vsf->length & 0x3){
		fprintf(stderr,"VERTEX_SHADER_FRAGMENT must have length divisible by 4\n");
		exit(-1);
		}
	
	switch((dest>>8) & 0xf){
	case 0:
		R300_STATECHANGE(r300, vpi);
		for(i=0;i<vsf->length;i++)
			r300->hw.vpi.cmd[R300_VPI_INSTR_0+i+4*(dest & 0xff)]=(vsf->body.d[i]);
		bump_vpu_count(r300->hw.vpi.cmd, vsf->length+4*(dest & 0xff));
		break;
		
	case 2:
		R300_STATECHANGE(r300, vpp);
		for(i=0;i<vsf->length;i++)
			r300->hw.vpp.cmd[R300_VPP_PARAM_0+i+4*(dest & 0xff)]=(vsf->body.d[i]);
		bump_vpu_count(r300->hw.vpp.cmd, vsf->length+4*(dest & 0xff));
		break;
	case 4:	
		R300_STATECHANGE(r300, vps);
		for(i=0;i<vsf->length;i++)
			r300->hw.vps.cmd[1+i+4*(dest & 0xff)]=(vsf->body.d[i]);
		bump_vpu_count(r300->hw.vps.cmd, vsf->length+4*(dest & 0xff));
		break;
	default:
		fprintf(stderr, "%s:%s don't know how to handle dest %04x\n", __FILE__, __FUNCTION__, dest);
		exit(-1);
	}
}


void r300SetupVertexShader(r300ContextPtr rmesa)
{
	GLcontext* ctx = rmesa->radeon.glCtx;
	
	/* Reset state, in case we don't use something */
	((drm_r300_cmd_header_t*)rmesa->hw.vpp.cmd)->vpu.count = 0;
	((drm_r300_cmd_header_t*)rmesa->hw.vpi.cmd)->vpu.count = 0;
	((drm_r300_cmd_header_t*)rmesa->hw.vps.cmd)->vpu.count = 0;


/* This needs to be replaced by vertex shader generation code */

	
	/* textures enabled ? */
	if(rmesa->state.texture.tc_count>0){
		rmesa->state.vertex_shader=SINGLE_TEXTURE_VERTEX_SHADER;
		} else {
		rmesa->state.vertex_shader=FLAT_COLOR_VERTEX_SHADER;
		}


        rmesa->state.vertex_shader.matrix[0].length=16;
        memcpy(rmesa->state.vertex_shader.matrix[0].body.f, ctx->_ModelProjectMatrix.m, 16*4);
	
	setup_vertex_shader_fragment(rmesa, VSF_DEST_PROGRAM, &(rmesa->state.vertex_shader.program));
	
	setup_vertex_shader_fragment(rmesa, VSF_DEST_MATRIX0, &(rmesa->state.vertex_shader.matrix[0]));
	#if 0
	setup_vertex_shader_fragment(rmesa, VSF_DEST_MATRIX1, &(rmesa->state.vertex_shader.matrix[0]));
	setup_vertex_shader_fragment(rmesa, VSF_DEST_MATRIX2, &(rmesa->state.vertex_shader.matrix[0]));
	
	setup_vertex_shader_fragment(rmesa, VSF_DEST_VECTOR0, &(rmesa->state.vertex_shader.vector[0]));
	setup_vertex_shader_fragment(rmesa, VSF_DEST_VECTOR1, &(rmesa->state.vertex_shader.vector[1]));
	#endif
	
	#if 0
	setup_vertex_shader_fragment(rmesa, VSF_DEST_UNKNOWN1, &(rmesa->state.vertex_shader.unknown1));
	setup_vertex_shader_fragment(rmesa, VSF_DEST_UNKNOWN2, &(rmesa->state.vertex_shader.unknown2));
	#endif
	
	R300_STATECHANGE(rmesa, pvs);
	rmesa->hw.pvs.cmd[R300_PVS_CNTL_1]=(rmesa->state.vertex_shader.program_start << R300_PVS_CNTL_1_PROGRAM_START_SHIFT)
		| (rmesa->state.vertex_shader.unknown_ptr1 << R300_PVS_CNTL_1_UNKNOWN_SHIFT)
		| (rmesa->state.vertex_shader.program_end << R300_PVS_CNTL_1_PROGRAM_END_SHIFT);
	rmesa->hw.pvs.cmd[R300_PVS_CNTL_2]=(rmesa->state.vertex_shader.param_offset << R300_PVS_CNTL_2_PARAM_OFFSET_SHIFT)
		| (rmesa->state.vertex_shader.param_count << R300_PVS_CNTL_2_PARAM_COUNT_SHIFT);
	rmesa->hw.pvs.cmd[R300_PVS_CNTL_3]=(rmesa->state.vertex_shader.unknown_ptr2 << R300_PVS_CNTL_3_PROGRAM_UNKNOWN_SHIFT)
	| (rmesa->state.vertex_shader.unknown_ptr3 << 0);
	
	/* This is done for vertex shader fragments, but also needs to be done for vap_pvs, 
	so I leave it as a reminder */
	#if 0
	reg_start(R300_VAP_PVS_WAITIDLE,0);
		e32(0x00000000);
	#endif
}

void r300SetupPixelShader(r300ContextPtr rmesa)
{
int i,k;

	/* This needs to be replaced by pixel shader generation code */

	/* textures enabled ? */
	if(rmesa->state.texture.tc_count>0){
		rmesa->state.pixel_shader=SINGLE_TEXTURE_PIXEL_SHADER;
		} else {
		rmesa->state.pixel_shader=FLAT_COLOR_PIXEL_SHADER;
		}

	R300_STATECHANGE(rmesa, fpt);
	for(i=0;i<rmesa->state.pixel_shader.program.tex.length;i++)
		rmesa->hw.fpt.cmd[R300_FPT_INSTR_0+i]=rmesa->state.pixel_shader.program.tex.inst[i];
	rmesa->hw.fpt.cmd[R300_FPT_CMD_0]=cmducs(R300_PFS_TEXI_0, rmesa->state.pixel_shader.program.tex.length);

	#define OUTPUT_FIELD(st, reg, field)  \
		R300_STATECHANGE(rmesa, st); \
		for(i=0;i<rmesa->state.pixel_shader.program.alu.length;i++) \
			rmesa->hw.st.cmd[R300_FPI_INSTR_0+i]=rmesa->state.pixel_shader.program.alu.inst[i].field;\
		rmesa->hw.st.cmd[R300_FPI_CMD_0]=cmducs(reg, rmesa->state.pixel_shader.program.alu.length);
	
	OUTPUT_FIELD(fpi[0], R300_PFS_INSTR0_0, inst0);
	OUTPUT_FIELD(fpi[1], R300_PFS_INSTR1_0, inst1);
	OUTPUT_FIELD(fpi[2], R300_PFS_INSTR2_0, inst2);
	OUTPUT_FIELD(fpi[3], R300_PFS_INSTR3_0, inst3);
	#undef OUTPUT_FIELD

	R300_STATECHANGE(rmesa, fp);
	for(i=0;i<4;i++){
		rmesa->hw.fp.cmd[R300_FP_NODE0+i]=
		(rmesa->state.pixel_shader.program.node[i].alu_offset << R300_PFS_NODE_ALU_OFFSET_SHIFT)
		| (rmesa->state.pixel_shader.program.node[i].alu_end  << R300_PFS_NODE_ALU_END_SHIFT)
		| (rmesa->state.pixel_shader.program.node[i].tex_offset << R300_PFS_NODE_TEX_OFFSET_SHIFT)
		| (rmesa->state.pixel_shader.program.node[i].tex_end  << R300_PFS_NODE_TEX_END_SHIFT)
		| ( (i==3) ? R300_PFS_NODE_LAST_NODE : 0);
		}

		/*  PFS_CNTL_0 */
	rmesa->hw.fp.cmd[R300_FP_CNTL0]=
		(rmesa->state.pixel_shader.program.active_nodes-1) 
		| (rmesa->state.pixel_shader.program.first_node_has_tex<<3);
		/* PFS_CNTL_1 */
	rmesa->hw.fp.cmd[R300_FP_CNTL1]=rmesa->state.pixel_shader.program.temp_register_count;
		/* PFS_CNTL_2 */
	rmesa->hw.fp.cmd[R300_FP_CNTL2]=
		(rmesa->state.pixel_shader.program.alu_offset << R300_PFS_CNTL_ALU_OFFSET_SHIFT)
		| (rmesa->state.pixel_shader.program.alu_end << R300_PFS_CNTL_ALU_END_SHIFT)
		| (rmesa->state.pixel_shader.program.tex_offset << R300_PFS_CNTL_TEX_OFFSET_SHIFT)
		| (rmesa->state.pixel_shader.program.tex_end << R300_PFS_CNTL_TEX_END_SHIFT);
	
	R300_STATECHANGE(rmesa, fpp);
	for(i=0;i<rmesa->state.pixel_shader.param_length;i++){
		rmesa->hw.fpp.cmd[R300_FPP_PARAM_0+4*i+0]=r300PackFloat32(rmesa->state.pixel_shader.param[i].x);
		rmesa->hw.fpp.cmd[R300_FPP_PARAM_0+4*i+1]=r300PackFloat32(rmesa->state.pixel_shader.param[i].y);
		rmesa->hw.fpp.cmd[R300_FPP_PARAM_0+4*i+2]=r300PackFloat32(rmesa->state.pixel_shader.param[i].z);
		rmesa->hw.fpp.cmd[R300_FPP_PARAM_0+4*i+3]=r300PackFloat32(rmesa->state.pixel_shader.param[i].w);
		}
	rmesa->hw.fpp.cmd[R300_FPP_CMD_0]=cmducs(R300_PFS_PARAM_0_X, rmesa->state.pixel_shader.param_length);

}

/**
 * Called by Mesa after an internal state update.
 */
static void r300InvalidateState(GLcontext * ctx, GLuint new_state)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);

	_swrast_InvalidateState(ctx, new_state);
	_swsetup_InvalidateState(ctx, new_state);
	_ac_InvalidateState(ctx, new_state);
	_tnl_InvalidateState(ctx, new_state);
	_ae_invalidate_state(ctx, new_state);

	/* Go inefficiency! */
	r300ResetHwState(r300);
}


/**
 * Completely recalculates hardware state based on the Mesa state.
 */
void r300ResetHwState(r300ContextPtr r300)
{
	GLcontext* ctx = r300->radeon.glCtx;
	int i;

	if (RADEON_DEBUG & DEBUG_STATE)
		fprintf(stderr, "%s\n", __FUNCTION__);

	r300UpdateWindow(ctx);
	
	r300ColorMask(ctx,
		ctx->Color.ColorMask[RCOMP],
		ctx->Color.ColorMask[GCOMP],
		ctx->Color.ColorMask[BCOMP],
		ctx->Color.ColorMask[ACOMP]);

	r300Enable(ctx, GL_DEPTH_TEST, ctx->Depth.Test);
	r300DepthMask(ctx, ctx->Depth.Mask);
	r300DepthFunc(ctx, ctx->Depth.Func);

	r300UpdateCulling(ctx);
        
	r300_setup_routing(ctx, GL_TRUE);
	
	r300UpdateTextureState(ctx);
	r300_setup_textures(ctx);
	r300_setup_rs_unit(ctx);
	
	r300SetupVertexShader(r300);
	r300SetupPixelShader(r300);
	
	r300_set_blend_state(ctx);
	r300AlphaFunc(ctx, ctx->Color.AlphaFunc, ctx->Color.AlphaRef);

//BEGIN: TODO
	r300->hw.unk2080.cmd[1] = 0x0030045A;

	r300->hw.ovf.cmd[R300_OVF_FMT_0] = 0x00000003;
	r300->hw.ovf.cmd[R300_OVF_FMT_1] = 0x00000000;

	r300->hw.vte.cmd[1] = R300_VPORT_X_SCALE_ENA
				| R300_VPORT_X_OFFSET_ENA
				| R300_VPORT_Y_SCALE_ENA
				| R300_VPORT_Y_OFFSET_ENA
				| R300_VPORT_Z_SCALE_ENA
				| R300_VPORT_Z_OFFSET_ENA
				| R300_VTX_W0_FMT;
	r300->hw.vte.cmd[2] = 0x00000008;

	r300->hw.unk2134.cmd[1] = 0x00FFFFFF;
	r300->hw.unk2134.cmd[2] = 0x00000000;

	r300->hw.unk2140.cmd[1] = 0x00000000;

	#if 0 /* Done in setup routing */
	((drm_r300_cmd_header_t*)r300->hw.vir[0].cmd)->unchecked_state.count = 1;
	r300->hw.vir[0].cmd[1] = 0x21030003;

	((drm_r300_cmd_header_t*)r300->hw.vir[1].cmd)->unchecked_state.count = 1;
	r300->hw.vir[1].cmd[1] = 0xF688F688;

	r300->hw.vic.cmd[R300_VIR_CNTL_0] = 0x00000001;
	r300->hw.vic.cmd[R300_VIR_CNTL_1] = 0x00000405;
	#endif
	
	r300->hw.unk21DC.cmd[1] = 0xAAAAAAAA;

	r300->hw.unk221C.cmd[1] = R300_221C_NORMAL;

	r300->hw.unk2220.cmd[1] = r300PackFloat32(1.0);
	r300->hw.unk2220.cmd[2] = r300PackFloat32(1.0);
	r300->hw.unk2220.cmd[3] = r300PackFloat32(1.0);
	r300->hw.unk2220.cmd[4] = r300PackFloat32(1.0);

	if (GET_CHIP(r300->radeon.radeonScreen) == RADEON_CHIP_R300)
		r300->hw.unk2288.cmd[1] = R300_2288_R300;
	else
		r300->hw.unk2288.cmd[1] = R300_2288_RV350;

	#if 0
	r300->hw.vof.cmd[R300_VOF_CNTL_0] = R300_VAP_OUTPUT_VTX_FMT_0__POS_PRESENT
				| R300_VAP_OUTPUT_VTX_FMT_0__COLOR_PRESENT;
	r300->hw.vof.cmd[R300_VOF_CNTL_1] = 0; /* no textures */
	
	
	r300->hw.pvs.cmd[R300_PVS_CNTL_1] = 0;
	r300->hw.pvs.cmd[R300_PVS_CNTL_2] = 0;
	r300->hw.pvs.cmd[R300_PVS_CNTL_3] = 0;
	#endif	

	r300->hw.gb_enable.cmd[1] = R300_GB_POINT_STUFF_ENABLE
		| R300_GB_LINE_STUFF_ENABLE
		| R300_GB_TRIANGLE_STUFF_ENABLE;

	r300->hw.gb_misc.cmd[R300_GB_MISC_MSPOS_0] = 0x66666666;
	r300->hw.gb_misc.cmd[R300_GB_MISC_MSPOS_1] = 0x06666666;
	if (GET_CHIP(r300->radeon.radeonScreen) == RADEON_CHIP_R300)
		r300->hw.gb_misc.cmd[R300_GB_MISC_TILE_CONFIG] = R300_GB_TILE_ENABLE
							| R300_GB_TILE_PIPE_COUNT_R300
							| R300_GB_TILE_SIZE_16;
	else
		r300->hw.gb_misc.cmd[R300_GB_MISC_TILE_CONFIG] = R300_GB_TILE_ENABLE
							| R300_GB_TILE_PIPE_COUNT_RV300
							| R300_GB_TILE_SIZE_16;
	r300->hw.gb_misc.cmd[R300_GB_MISC_SELECT] = 0x00000000;
	r300->hw.gb_misc.cmd[R300_GB_MISC_AA_CONFIG] = 0x00000000; /* No antialiasing */

	//r300->hw.txe.cmd[R300_TXE_ENABLE] = 0;

	r300->hw.unk4200.cmd[1] = r300PackFloat32(0.0);
	r300->hw.unk4200.cmd[2] = r300PackFloat32(0.0);
	r300->hw.unk4200.cmd[3] = r300PackFloat32(1.0);
	r300->hw.unk4200.cmd[4] = r300PackFloat32(1.0);

	r300->hw.unk4214.cmd[1] = 0x00050005;

	r300->hw.ps.cmd[R300_PS_POINTSIZE] = (6 << R300_POINTSIZE_X_SHIFT) |
					     (6 << R300_POINTSIZE_Y_SHIFT);

	r300->hw.unk4230.cmd[1] = 0x01800000;
	r300->hw.unk4230.cmd[2] = 0x00020006;
	r300->hw.unk4230.cmd[3] = r300PackFloat32(1.0 / 192.0);

	r300->hw.unk4260.cmd[1] = 0;
	r300->hw.unk4260.cmd[2] = r300PackFloat32(0.0);
	r300->hw.unk4260.cmd[3] = r300PackFloat32(1.0);

	r300->hw.unk4274.cmd[1] = 0x00000002;
	r300->hw.unk4274.cmd[2] = 0x0003AAAA;
	r300->hw.unk4274.cmd[3] = 0x00000000;
	r300->hw.unk4274.cmd[4] = 0x00000000;

	r300->hw.unk4288.cmd[1] = 0x00000000;
	r300->hw.unk4288.cmd[2] = 0x00000001;
	r300->hw.unk4288.cmd[3] = 0x00000000;
	r300->hw.unk4288.cmd[4] = 0x00000000;
	r300->hw.unk4288.cmd[5] = 0x00000000;

	r300->hw.unk42A0.cmd[1] = 0x00000000;

	r300->hw.unk42B4.cmd[1] = 0x00000000;

	r300->hw.unk42C0.cmd[1] = 0x4B7FFFFF;
	r300->hw.unk42C0.cmd[2] = 0x00000000;


	r300->hw.unk43A4.cmd[1] = 0x0000001C;
	r300->hw.unk43A4.cmd[2] = 0x2DA49525;

	r300->hw.unk43E8.cmd[1] = 0x00FFFFFF;

	#if 0
	r300->hw.fp.cmd[R300_FP_CNTL0] = 0;
	r300->hw.fp.cmd[R300_FP_CNTL1] = 0;
	r300->hw.fp.cmd[R300_FP_CNTL2] = 0;
	r300->hw.fp.cmd[R300_FP_NODE0] = 0;
	r300->hw.fp.cmd[R300_FP_NODE1] = 0;
	r300->hw.fp.cmd[R300_FP_NODE2] = 0;
	r300->hw.fp.cmd[R300_FP_NODE3] = 0;
	#endif
	
	r300->hw.unk46A4.cmd[1] = 0x00001B01;
	r300->hw.unk46A4.cmd[2] = 0x00001B0F;
	r300->hw.unk46A4.cmd[3] = 0x00001B0F;
	r300->hw.unk46A4.cmd[4] = 0x00001B0F;
	r300->hw.unk46A4.cmd[5] = 0x00000001;

	#if 0
	for(i = 1; i <= 64; ++i) {
		/* create NOP instructions */
		r300->hw.fpi[0].cmd[i] = FP_INSTRC(MAD, FP_ARGC(SRC0C_XYZ), FP_ARGC(ONE), FP_ARGC(ZERO));
		r300->hw.fpi[1].cmd[i] = FP_SELC(0,XYZ,NO,FP_TMP(0),0,0);
		r300->hw.fpi[2].cmd[i] = FP_INSTRA(MAD, FP_ARGA(SRC0A), FP_ARGA(ONE), FP_ARGA(ZERO));
		r300->hw.fpi[3].cmd[i] = FP_SELA(0,W,NO,FP_TMP(0),0,0);
	}
	#endif
	
	r300->hw.unk4BC0.cmd[1] = 0;

	r300->hw.unk4BC8.cmd[1] = 0;
	r300->hw.unk4BC8.cmd[2] = 0;
	r300->hw.unk4BC8.cmd[3] = 0;

	#if 0
	r300->hw.at.cmd[R300_AT_ALPHA_TEST] = 0;
	#endif

	r300->hw.unk4BD8.cmd[1] = 0;

	r300->hw.unk4E00.cmd[1] = 0;

	#if 0
	r300->hw.bld.cmd[R300_BLD_CBLEND] = 0;
	r300->hw.bld.cmd[R300_BLD_ABLEND] = 0;
	#endif

	r300->hw.unk4E10.cmd[1] = 0;
	r300->hw.unk4E10.cmd[2] = 0;
	r300->hw.unk4E10.cmd[3] = 0;

	r300->hw.cb.cmd[R300_CB_OFFSET] =
		r300->radeon.radeonScreen->backOffset +
		r300->radeon.radeonScreen->fbLocation;
	r300->hw.cb.cmd[R300_CB_PITCH] = r300->radeon.radeonScreen->backPitch
		| R300_COLOR_UNKNOWN_22_23;

	r300->hw.unk4E50.cmd[1] = 0;
	r300->hw.unk4E50.cmd[2] = 0;
	r300->hw.unk4E50.cmd[3] = 0;
	r300->hw.unk4E50.cmd[4] = 0;
	r300->hw.unk4E50.cmd[5] = 0;
	r300->hw.unk4E50.cmd[6] = 0;
	r300->hw.unk4E50.cmd[7] = 0;
	r300->hw.unk4E50.cmd[8] = 0;
	r300->hw.unk4E50.cmd[9] = 0;

	r300->hw.unk4E88.cmd[1] = 0;

	r300->hw.unk4EA0.cmd[1] = 0x00000000;
	r300->hw.unk4EA0.cmd[2] = 0xffffffff;

	r300->hw.unk4F08.cmd[1] = 0x00FFFF00;

	r300->hw.unk4F10.cmd[1] = 0x00000002; // depthbuffer format?
	r300->hw.unk4F10.cmd[2] = 0x00000000;
	r300->hw.unk4F10.cmd[3] = 0x00000003;
	r300->hw.unk4F10.cmd[4] = 0x00000000;

	r300->hw.zb.cmd[R300_ZB_OFFSET] =
		r300->radeon.radeonScreen->depthOffset +
		r300->radeon.radeonScreen->fbLocation;
	r300->hw.zb.cmd[R300_ZB_PITCH] = r300->radeon.radeonScreen->depthPitch;

	r300->hw.unk4F28.cmd[1] = 0;

	r300->hw.unk4F30.cmd[1] = 0;
	r300->hw.unk4F30.cmd[2] = 0;

	r300->hw.unk4F44.cmd[1] = 0;

	r300->hw.unk4F54.cmd[1] = 0;

	#if 0
	((drm_r300_cmd_header_t*)r300->hw.vpi.cmd)->vpu.count = 0;
	for(i = 1; i < R300_VPI_CMDSIZE; i += 4) {
		/* MOV t0, t0 */
		r300->hw.vpi.cmd[i+0] = VP_OUT(ADD,TMP,0,XYZW);
		r300->hw.vpi.cmd[i+1] = VP_IN(TMP,0);
		r300->hw.vpi.cmd[i+2] = VP_ZERO();
		r300->hw.vpi.cmd[i+3] = VP_ZERO();
	}

	((drm_r300_cmd_header_t*)r300->hw.vpp.cmd)->vpu.count = 0;
	for(i = 1; i < R300_VPP_CMDSIZE; ++i)
		r300->hw.vpp.cmd[i] = 0;
	#endif
		
	r300->hw.vps.cmd[R300_VPS_ZERO_0] = 0;
	r300->hw.vps.cmd[R300_VPS_ZERO_1] = 0;
	r300->hw.vps.cmd[R300_VPS_POINTSIZE] = r300PackFloat32(1.0);
	r300->hw.vps.cmd[R300_VPS_ZERO_3] = 0;
	
//END: TODO
	
	r300->hw.all_dirty = GL_TRUE;
}



/**
 * Calculate initial hardware state and register state functions.
 * Assumes that the command buffer and state atoms have been
 * initialized already.
 */
void r300InitState(r300ContextPtr r300)
{
	GLcontext *ctx = r300->radeon.glCtx;
	GLuint depth_fmt;

	radeonInitState(&r300->radeon);
	
	switch (ctx->Visual.depthBits) {
	case 16:
		r300->state.depth.scale = 1.0 / (GLfloat) 0xffff;
		depth_fmt = R200_DEPTH_FORMAT_16BIT_INT_Z;
		//r300->state.stencil.clear = 0x00000000;
		break;
	case 24:
		r300->state.depth.scale = 1.0 / (GLfloat) 0xffffff;
		depth_fmt = R200_DEPTH_FORMAT_24BIT_INT_Z;
		//r300->state.stencil.clear = 0xff000000;
		break;
	default:
		fprintf(stderr, "Error: Unsupported depth %d... exiting\n",
			ctx->Visual.depthBits);
		exit(-1);
	}
	
	memset(&(r300->state.texture), 0, sizeof(r300->state.texture));

	r300ResetHwState(r300);
}



/**
 * Initialize driver's state callback functions
 */
void r300InitStateFuncs(struct dd_function_table* functions)
{
	radeonInitStateFuncs(functions);

	functions->UpdateState = r300InvalidateState;
	//functions->AlphaFunc = r300AlphaFunc;
	functions->BlendColor = r300BlendColor;
	functions->BlendEquationSeparate = r300BlendEquationSeparate;
	functions->BlendFuncSeparate = r300BlendFuncSeparate;
	functions->Enable = r300Enable;
	functions->ColorMask = r300ColorMask;
	functions->DepthFunc = r300DepthFunc;
	functions->DepthMask = r300DepthMask;
	functions->CullFace = r300CullFace;
	functions->FrontFace = r300FrontFace;

	/* Viewport related */
	functions->Viewport = r300Viewport;
	functions->DepthRange = r300DepthRange;
	functions->PointSize = r300PointSize;
}


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
#include "texformat.h"

#include "radeon_ioctl.h"
#include "radeon_state.h"
#include "r300_context.h"
#include "r300_ioctl.h"
#include "r300_state.h"
#include "r300_reg.h"
#include "r300_program.h"
#include "r300_emit.h"
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

/* helper function */
static void r300_set_blend_cntl(r300ContextPtr rmesa, int func, int eqn, int cbits, int funcA, int eqnA)
{
	GLuint new_ablend, new_cblend;

	#if 0
	fprintf(stderr, "eqnA=%08x funcA=%08x eqn=%08x func=%08x cbits=%08x\n", eqnA, funcA, eqn, func, cbits);
	#endif
	new_ablend = eqnA | funcA;
	new_cblend = eqn | func;
	if(funcA == func){
		new_cblend |=  R300_BLEND_NO_SEPARATE;
		}
	new_cblend |= cbits;
	
	if((new_ablend != rmesa->hw.bld.cmd[R300_BLD_ABLEND])
		|| (new_cblend != rmesa->hw.bld.cmd[R300_BLD_CBLEND])){
		R300_STATECHANGE(rmesa, bld);
		rmesa->hw.bld.cmd[R300_BLD_ABLEND]=new_ablend;
		rmesa->hw.bld.cmd[R300_BLD_CBLEND]=new_cblend;
		}
}

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


	if (rmesa->radeon.radeonScreen->drmSupportsBlendColor) {
		if (ctx->Color._LogicOpEnabled) {
			#if 0
			rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] =
			    cntl | R300_ROP_ENABLE;
			#endif
			r300_set_blend_cntl(rmesa,
				func, eqn, 0,
				func, eqn);
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
			r300_set_blend_cntl(rmesa,
				func, eqn, 0,
				func, eqn);
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
			r300_set_blend_cntl(rmesa,
				func, eqn, 0,
				func, eqn);
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

	r300_set_blend_cntl(rmesa,
		func, eqn, R300_BLEND_UNKNOWN | R300_BLEND_ENABLE,
		funcA, eqnA);
	r300_set_blend_cntl(rmesa,
		func, eqn, R300_BLEND_UNKNOWN | R300_BLEND_ENABLE,
		funcA, eqnA);
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
		/* Fast track this one...
		 */
	case GL_TEXTURE_1D:
	case GL_TEXTURE_2D:
	case GL_TEXTURE_3D:
		break;

	case GL_ALPHA_TEST:
		R200_STATECHANGE(r300, at);
		if (state) {
			r300->hw.at.cmd[R300_AT_ALPHA_TEST] |=
			    R300_ALPHA_TEST_ENABLE;
		} else {
			r300->hw.at.cmd[R300_AT_ALPHA_TEST] |=
			    ~R300_ALPHA_TEST_ENABLE;
		}
		break;

	case GL_BLEND:
	case GL_COLOR_LOGIC_OP:
		r300_set_blend_state(ctx);
		break;

	case GL_DEPTH_TEST:
		R300_STATECHANGE(r300, zs);

		if (state) {
			if (ctx->Depth.Mask)
				newval = R300_RB3D_Z_TEST_AND_WRITE;
			else
				newval = R300_RB3D_Z_TEST;
		} else
			newval = 0;

		r300->hw.zs.cmd[R300_ZS_CNTL_0] = newval;
		break;

	case GL_STENCIL_TEST:

		WARN_ONCE("Do not know how to enable stencil. Help me !\n");
		
		if (r300->state.hw_stencil) {
			//fprintf(stderr, "Stencil %s\n", state ? "enabled" : "disabled");
			R300_STATECHANGE(r300, zs);
			if (state) {
				r300->hw.zs.cmd[R300_ZS_CNTL_0] |=
				    R300_RB3D_STENCIL_ENABLE;
			} else {
				r300->hw.zs.cmd[R300_ZS_CNTL_0] &=
				    ~R300_RB3D_STENCIL_ENABLE;
			}
		} else {
			FALLBACK(&r300->radeon, RADEON_FALLBACK_STENCIL, state);
		}
		break;

	case GL_CULL_FACE:
		r300UpdateCulling(ctx);
		break;
		
	case GL_POLYGON_OFFSET_POINT:
	case GL_POLYGON_OFFSET_LINE:
		WARN_ONCE("Don't know how to enable polygon offset point/line. Help me !\n");
		break;
		
	case GL_POLYGON_OFFSET_FILL:
		R300_STATECHANGE(r300, unk42B4);
		if(state){
			r300->hw.unk42B4.cmd[1] |= 3;
			} else {
			r300->hw.unk42B4.cmd[1] &= ~3;
			}
		break;
		
	case GL_VERTEX_PROGRAM_ARB:
		//TCL_FALLBACK(rmesa->glCtx, R200_TCL_FALLBACK_TCL_DISABLE, state);
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

	R300_STATECHANGE(r300, zs);

	r300->hw.zs.cmd[R300_ZS_CNTL_1] &= ~(R300_ZS_MASK << R300_RB3D_ZS1_DEPTH_FUNC_SHIFT);

	switch(func) {
	case GL_NEVER:
		r300->hw.zs.cmd[R300_ZS_CNTL_1] |= R300_ZS_NEVER << R300_RB3D_ZS1_DEPTH_FUNC_SHIFT;
		break;
	case GL_LESS:
		r300->hw.zs.cmd[R300_ZS_CNTL_1] |= R300_ZS_LESS << R300_RB3D_ZS1_DEPTH_FUNC_SHIFT;
		break;
	case GL_EQUAL:
		r300->hw.zs.cmd[R300_ZS_CNTL_1] |= R300_ZS_EQUAL << R300_RB3D_ZS1_DEPTH_FUNC_SHIFT;
		break;
	case GL_LEQUAL:
		r300->hw.zs.cmd[R300_ZS_CNTL_1] |= R300_ZS_LEQUAL << R300_RB3D_ZS1_DEPTH_FUNC_SHIFT;
		break;
	case GL_GREATER:
		r300->hw.zs.cmd[R300_ZS_CNTL_1] |= R300_ZS_GREATER << R300_RB3D_ZS1_DEPTH_FUNC_SHIFT;
		break;
	case GL_NOTEQUAL:
		r300->hw.zs.cmd[R300_ZS_CNTL_1] |= R300_ZS_NOTEQUAL << R300_RB3D_ZS1_DEPTH_FUNC_SHIFT;
		break;
	case GL_GEQUAL:
		r300->hw.zs.cmd[R300_ZS_CNTL_1] |= R300_ZS_GEQUAL << R300_RB3D_ZS1_DEPTH_FUNC_SHIFT;
		break;
	case GL_ALWAYS:
		r300->hw.zs.cmd[R300_ZS_CNTL_1] |= R300_ZS_ALWAYS << R300_RB3D_ZS1_DEPTH_FUNC_SHIFT;
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

	R300_STATECHANGE(r300, zs);
	r300->hw.zs.cmd[R300_ZS_CNTL_0] = mask
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
 * Stencil
 */

 static int translate_stencil_func(int func)
 {
	switch (func) {
	case GL_NEVER:
		    return R300_ZS_NEVER;
		break;
	case GL_LESS:
		    return R300_ZS_LESS;
		break;
	case GL_EQUAL:
		    return R300_ZS_EQUAL;
		break;
	case GL_LEQUAL:
		    return R300_ZS_LEQUAL;
		break;
	case GL_GREATER:
		    return R300_ZS_GREATER;
		break;
	case GL_NOTEQUAL:
		    return R300_ZS_NOTEQUAL;
		break;
	case GL_GEQUAL:
		    return R300_ZS_GEQUAL;
		break;
	case GL_ALWAYS:
		    return R300_ZS_ALWAYS;
		break;
	}
 return 0;
 }

 static int translate_stencil_op(int op)
{
	switch (op) {
	case GL_KEEP:
		    return R300_ZS_KEEP;
	case GL_ZERO:
		    return R300_ZS_ZERO;
	case GL_REPLACE:
		    return R300_ZS_REPLACE;
	case GL_INCR:
		    return R300_ZS_INCR;
	case GL_DECR:
		    return R300_ZS_DECR;
	case GL_INCR_WRAP_EXT:
		    return R300_ZS_INCR_WRAP;
	case GL_DECR_WRAP_EXT:
		    return R300_ZS_DECR_WRAP;
	case GL_INVERT:
		    return R300_ZS_INVERT;
	default:
		WARN_ONCE("Do not know how to translate stencil op");
		return R300_ZS_KEEP;
	}
}

static void r300StencilFunc(GLcontext * ctx, GLenum func,
			    GLint ref, GLuint mask)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	GLuint refmask = ((ctx->Stencil.Ref[0] << R300_RB3D_ZS2_STENCIL_REF_SHIFT) |
			  (ctx->Stencil.
			   ValueMask[0] << R300_RB3D_ZS2_STENCIL_MASK_SHIFT));
	GLuint flag;

	R200_STATECHANGE(rmesa, zs);

	rmesa->hw.zs.cmd[R300_ZS_CNTL_1] &= ~(
		(R300_ZS_MASK << R300_RB3D_ZS1_FRONT_FUNC_SHIFT)
		| (R300_ZS_MASK << R300_RB3D_ZS1_BACK_FUNC_SHIFT));
	rmesa->hw.zs.cmd[R300_ZS_CNTL_2] &=  ~((R300_ZS_MASK << R300_RB3D_ZS2_STENCIL_REF_SHIFT) |
						(R300_ZS_MASK << R300_RB3D_ZS2_STENCIL_MASK_SHIFT));

	flag = translate_stencil_func(ctx->Stencil.Function[0]);

	rmesa->hw.zs.cmd[R300_ZS_CNTL_1] |= (flag << R300_RB3D_ZS1_FRONT_FUNC_SHIFT)
					  | (flag << R300_RB3D_ZS1_BACK_FUNC_SHIFT);
	rmesa->hw.zs.cmd[R300_ZS_CNTL_2] |= refmask;
}

static void r300StencilMask(GLcontext * ctx, GLuint mask)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);

	R200_STATECHANGE(rmesa, zs);
	rmesa->hw.zs.cmd[R300_ZS_CNTL_2]  &= ~(R300_ZS_MASK << R300_RB3D_ZS2_STENCIL_WRITE_MASK_SHIFT);
	rmesa->hw.zs.cmd[R300_ZS_CNTL_2] |= ctx->Stencil.WriteMask[0] << R300_RB3D_ZS2_STENCIL_WRITE_MASK_SHIFT;
}


static void r300StencilOp(GLcontext * ctx, GLenum fail,
			  GLenum zfail, GLenum zpass)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);

	R200_STATECHANGE(rmesa, zs);
		/* It is easier to mask what's left.. */
	rmesa->hw.zs.cmd[R300_ZS_CNTL_1] &= (R300_ZS_MASK << R300_RB3D_ZS1_DEPTH_FUNC_SHIFT);

	rmesa->hw.zs.cmd[R300_ZS_CNTL_1] |=
		 (translate_stencil_op(ctx->Stencil.FailFunc[0]) << R300_RB3D_ZS1_FRONT_FAIL_OP_SHIFT)
		|(translate_stencil_op(ctx->Stencil.ZFailFunc[0]) << R300_RB3D_ZS1_FRONT_ZFAIL_OP_SHIFT)
		|(translate_stencil_op(ctx->Stencil.ZPassFunc[0]) << R300_RB3D_ZS1_FRONT_ZPASS_OP_SHIFT)
		|(translate_stencil_op(ctx->Stencil.FailFunc[0]) << R300_RB3D_ZS1_BACK_FAIL_OP_SHIFT)
		|(translate_stencil_op(ctx->Stencil.ZFailFunc[0]) << R300_RB3D_ZS1_BACK_ZFAIL_OP_SHIFT)
		|(translate_stencil_op(ctx->Stencil.ZPassFunc[0]) << R300_RB3D_ZS1_BACK_ZPASS_OP_SHIFT);

}

static void r300ClearStencil(GLcontext * ctx, GLint s)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);

	/* Not sure whether this is correct.. */
	R200_STATECHANGE(rmesa, zs);
	rmesa->hw.zs.cmd[R300_ZS_CNTL_2] =
	    ((GLuint) ctx->Stencil.Clear |
	     (0xff << R200_STENCIL_MASK_SHIFT) |
	     (ctx->Stencil.WriteMask[0] << R200_STENCIL_WRITEMASK_SHIFT));
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

/* =============================================================
 * Polygon state
 */
#ifdef HAVE_ZBS
static void r300PolygonOffset(GLcontext * ctx, GLfloat factor, GLfloat units)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	GLfloat constant = units * /*rmesa->state.depth.scale*/4;
	
	factor *= 12; 

/*    fprintf(stderr, "%s f:%f u:%f\n", __FUNCTION__, factor, constant); */

	R300_STATECHANGE(rmesa, zbs);
	rmesa->hw.zbs.cmd[R300_ZBS_T_FACTOR] = r300PackFloat32(factor);
	rmesa->hw.zbs.cmd[R300_ZBS_T_CONSTANT] = r300PackFloat32(constant);
	rmesa->hw.zbs.cmd[R300_ZBS_W_FACTOR] = r300PackFloat32(factor);
	rmesa->hw.zbs.cmd[R300_ZBS_W_CONSTANT] = r300PackFloat32(constant);
}
#endif

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
		if (RADEON_DEBUG & DEBUG_STATE)fprintf(stderr, "Enabling "#r "\n"); \
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
	if(r300->current_vp!=NULL){

	/* VERT_ATTRIB_WEIGHT, VERT_ATTRIB_SIX, VERT_ATTRIB_SEVEN, VERT_ATTRIB_GENERIC0,
	   VERT_ATTRIB_GENERIC1, VERT_ATTRIB_GENERIC2, VERT_ATTRIB_GENERIC3 */
	r300->state.render_inputs = 0;
	   
	if(r300->current_vp->inputs[VERT_ATTRIB_POS] != -1){
		reg=r300->current_vp->inputs[VERT_ATTRIB_POS];
		CONFIGURE_AOS(VB->ObjPtr, 0, i_coords, AOS_FORMAT_FLOAT);
		r300->state.render_inputs |= _TNL_BIT_POS;
	}
	if(r300->current_vp->inputs[VERT_ATTRIB_NORMAL] != -1){
		reg=r300->current_vp->inputs[VERT_ATTRIB_NORMAL];
		CONFIGURE_AOS(VB->NormalPtr, 0, i_normal, AOS_FORMAT_FLOAT);
		r300->state.render_inputs |= _TNL_BIT_NORMAL;
	}
	if(r300->current_vp->inputs[VERT_ATTRIB_COLOR0] != -1){
		reg=r300->current_vp->inputs[VERT_ATTRIB_COLOR0];
		CONFIGURE_AOS(VB->ColorPtr[0], 0, i_color[0], AOS_FORMAT_FLOAT_COLOR);
		r300->state.render_inputs |= _TNL_BIT_COLOR0;
	}
	if(r300->current_vp->inputs[VERT_ATTRIB_COLOR1] != -1){
		reg=r300->current_vp->inputs[VERT_ATTRIB_COLOR1];
		CONFIGURE_AOS(VB->SecondaryColorPtr[0], 0, i_color[1], AOS_FORMAT_FLOAT_COLOR);
		r300->state.render_inputs |= _TNL_BIT_COLOR1;
	}
	if(r300->current_vp->inputs[VERT_ATTRIB_FOG] != -1){
		reg=r300->current_vp->inputs[VERT_ATTRIB_FOG];
		CONFIGURE_AOS(VB->FogCoordPtr, 0, i_fog, AOS_FORMAT_FLOAT);
		r300->state.render_inputs |= _TNL_BIT_FOG;
	}
	for(i=0;i < ctx->Const.MaxTextureUnits;i++) // tex 7 is last 
		if(r300->current_vp->inputs[VERT_ATTRIB_TEX0+i] != -1){
			reg=r300->current_vp->inputs[VERT_ATTRIB_TEX0+i];
			CONFIGURE_AOS(VB->TexCoordPtr[i], 0, i_tex[i], AOS_FORMAT_FLOAT);
			r300->state.render_inputs |= _TNL_BIT_TEX0<<i;
		}
#if 0
	if((tnl->render_inputs & _TNL_BIT_INDEX))
		CONFIGURE_AOS(VB->IndexPtr[0], 0, i_index, AOS_FORMAT_FLOAT);
	
	if((tnl->render_inputs & _TNL_BIT_POINTSIZE))
		CONFIGURE_AOS(VB->PointSizePtr, 0, i_pointsize, AOS_FORMAT_FLOAT);
#endif	
	}else{
	
	r300->state.render_inputs = tnl->render_inputs;
	
	if(tnl->render_inputs & _TNL_BIT_POS)
		CONFIGURE_AOS(VB->ObjPtr, 0, i_coords, AOS_FORMAT_FLOAT);
	if(tnl->render_inputs & _TNL_BIT_NORMAL)
		CONFIGURE_AOS(VB->NormalPtr, 0, i_normal, AOS_FORMAT_FLOAT);
	
	if(tnl->render_inputs & _TNL_BIT_COLOR0)
		CONFIGURE_AOS(VB->ColorPtr[0], 0, i_color[0], AOS_FORMAT_FLOAT_COLOR);
	if(tnl->render_inputs & _TNL_BIT_COLOR1)
		CONFIGURE_AOS(VB->SecondaryColorPtr[0], 0, i_color[1], AOS_FORMAT_FLOAT_COLOR);
	
	/*if(tnl->render_inputs & _TNL_BIT_FOG) // Causes lock ups when immediate mode is on
		CONFIGURE_AOS(VB->FogCoordPtr, 0, i_fog, AOS_FORMAT_FLOAT);*/
	
	for(i=0;i < ctx->Const.MaxTextureUnits;i++)
		if(tnl->render_inputs & (_TNL_BIT_TEX0<<i))
			CONFIGURE_AOS(VB->TexCoordPtr[i], 0, i_tex[i], AOS_FORMAT_FLOAT);
	
	if(tnl->render_inputs & _TNL_BIT_INDEX)
		CONFIGURE_AOS(VB->IndexPtr[0], 0, i_index, AOS_FORMAT_FLOAT);
	if(tnl->render_inputs & _TNL_BIT_POINTSIZE)
		CONFIGURE_AOS(VB->PointSizePtr, 0, i_pointsize, AOS_FORMAT_FLOAT);
	}
	
	r300->state.aos_count=count;

	if (RADEON_DEBUG & DEBUG_STATE)
		fprintf(stderr, "aos_count=%d render_inputs=%08x\n", count, r300->state.render_inputs);
		

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

	r300->hw.vic.cmd[R300_VIC_CNTL_1]=0;
	
	if(r300->state.render_inputs & _TNL_BIT_POS)
		r300->hw.vic.cmd[R300_VIC_CNTL_1]|=R300_INPUT_CNTL_POS;
	
	if(r300->state.render_inputs & _TNL_BIT_NORMAL)
		r300->hw.vic.cmd[R300_VIC_CNTL_1]|=R300_INPUT_CNTL_NORMAL;
	
	if(r300->state.render_inputs & _TNL_BIT_COLOR0)
		r300->hw.vic.cmd[R300_VIC_CNTL_1]|=R300_INPUT_CNTL_COLOR;

	for(i=0;i < ctx->Const.MaxTextureUnits;i++)
		if(r300->state.render_inputs & (_TNL_BIT_TEX0<<i))
			r300->hw.vic.cmd[R300_VIC_CNTL_1]|=(R300_INPUT_CNTL_TC0<<i);

	/* Stage 3: VAP output */
	R300_STATECHANGE(r300, vof);
	r300->hw.vof.cmd[R300_VOF_CNTL_0]=R300_VAP_OUTPUT_VTX_FMT_0__POS_PRESENT
					| R300_VAP_OUTPUT_VTX_FMT_0__COLOR_PRESENT;

	r300->hw.vof.cmd[R300_VOF_CNTL_1]=0;
	for(i=0;i < ctx->Const.MaxTextureUnits;i++)
		if(r300->state.render_inputs & (_TNL_BIT_TEX0<<i))
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

/* r300 doesnt handle GL_CLAMP and GL_MIRROR_CLAMP_EXT correctly when filter is NEAREST.
 * Since texwrap produces same results for GL_CLAMP and GL_CLAMP_TO_EDGE we use them instead.
 * We need to recalculate wrap modes whenever filter mode is changed because someone might do:
 * glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
 * glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
 * glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
 * Since r300 completely ignores R300_TX_CLAMP when either min or mag is nearest it cant handle
 * combinations where only one of them is nearest.
 */
static unsigned long gen_fixed_filter(unsigned long f)
{
	unsigned long mag, min, needs_fixing=0;
	//return f;
	
	/* We ignore MIRROR bit so we dont have to do everything twice */
	if((f & ((7-1) << R300_TX_WRAP_S_SHIFT)) == (R300_TX_CLAMP << R300_TX_WRAP_S_SHIFT)){
		needs_fixing |= 1;
	}
	if((f & ((7-1) << R300_TX_WRAP_T_SHIFT)) == (R300_TX_CLAMP << R300_TX_WRAP_T_SHIFT)){
		needs_fixing |= 2;
	}
	if((f & ((7-1) << R300_TX_WRAP_Q_SHIFT)) == (R300_TX_CLAMP << R300_TX_WRAP_Q_SHIFT)){
		needs_fixing |= 4;
	}
	
	if(!needs_fixing)
		return f;
	
	mag=f & R300_TX_MAG_FILTER_MASK;
	min=f & R300_TX_MIN_FILTER_MASK;
	
	/* TODO: Check for anisto filters too */
	if((mag != R300_TX_MAG_FILTER_NEAREST) && (min != R300_TX_MIN_FILTER_NEAREST))
		return f;
	
	/* r300 cant handle these modes hence we force nearest to linear */
	if((mag == R300_TX_MAG_FILTER_NEAREST) && (min != R300_TX_MIN_FILTER_NEAREST)){
		f &= ~R300_TX_MAG_FILTER_NEAREST;
		f |= R300_TX_MAG_FILTER_LINEAR;
		return f;
	}
	
	if((min == R300_TX_MIN_FILTER_NEAREST) && (mag != R300_TX_MAG_FILTER_NEAREST)){
		f &= ~R300_TX_MIN_FILTER_NEAREST;
		f |= R300_TX_MIN_FILTER_LINEAR;
		return f;
	}
	
	/* Both are nearest */
	if(needs_fixing & 1){
		f &= ~((7-1) << R300_TX_WRAP_S_SHIFT);
		f |= R300_TX_CLAMP_TO_EDGE << R300_TX_WRAP_S_SHIFT;
	}
	if(needs_fixing & 2){
		f &= ~((7-1) << R300_TX_WRAP_T_SHIFT);
		f |= R300_TX_CLAMP_TO_EDGE << R300_TX_WRAP_T_SHIFT;
	}
	if(needs_fixing & 4){
		f &= ~((7-1) << R300_TX_WRAP_Q_SHIFT);
		f |= R300_TX_CLAMP_TO_EDGE << R300_TX_WRAP_Q_SHIFT;
	}
	return f;
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
	//R300_STATECHANGE(r300, tex.border_color);

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
		if( ((r300->state.render_inputs & (_TNL_BIT_TEX0<<i))!=0) != ((ctx->Texture.Unit[i].Enabled)!=0) ) {
			WARN_ONCE("Mismatch between render_inputs and ctx->Texture.Unit[i].Enabled value.\n");
			}
		if(r300->state.render_inputs & (_TNL_BIT_TEX0<<i)){
			t=r300->state.texture.unit[i].texobj;
			//fprintf(stderr, "format=%08x\n", r300->state.texture.unit[i].format);
			r300->state.texture.tc_count++;
			if(t==NULL){
				fprintf(stderr, "Texture unit %d enabled, but corresponding texobj is NULL, using default object.\n", i);
				//exit(-1);
				t=&default_tex_obj;
				}
			//fprintf(stderr, "t->format=%08x\n", t->format);
			if((t->format & 0xffffff00)==0xffffff00){
				WARN_ONCE("unknown texture format encountered. Help me !\n");
				//fprintf(stderr, "t->format=%08x\n", t->format);
				}
			if (RADEON_DEBUG & DEBUG_STATE)
				fprintf(stderr, "Activating texture unit %d\n", i);
			max_texture_unit=i;
			r300->hw.txe.cmd[R300_TXE_ENABLE]|=(1<<i);

			r300->hw.tex.filter.cmd[R300_TEX_VALUE_0+i]=gen_fixed_filter(t->filter);
			/* No idea why linear filtered textures shake when puting random data */
			/*r300->hw.tex.unknown1.cmd[R300_TEX_VALUE_0+i]=(rand()%0xffffffff) & (~0x1fff);*/
			r300->hw.tex.size.cmd[R300_TEX_VALUE_0+i]=t->size;
			r300->hw.tex.format.cmd[R300_TEX_VALUE_0+i]=t->format;
			//fprintf(stderr, "t->format=%08x\n", t->format);
			r300->hw.tex.offset.cmd[R300_TEX_VALUE_0+i]=r300->radeon.radeonScreen->fbLocation+t->offset;
			r300->hw.tex.unknown4.cmd[R300_TEX_VALUE_0+i]=0x0;
			r300->hw.tex.unknown5.cmd[R300_TEX_VALUE_0+i]=0x0;
			//r300->hw.tex.border_color.cmd[R300_TEX_VALUE_0+i]=t->pp_border_color;
			}
		}
	((drm_r300_cmd_header_t*)r300->hw.tex.filter.cmd)->unchecked_state.count = max_texture_unit+1;
	((drm_r300_cmd_header_t*)r300->hw.tex.unknown1.cmd)->unchecked_state.count = max_texture_unit+1;
	((drm_r300_cmd_header_t*)r300->hw.tex.size.cmd)->unchecked_state.count = max_texture_unit+1;
	((drm_r300_cmd_header_t*)r300->hw.tex.format.cmd)->unchecked_state.count = max_texture_unit+1;
	((drm_r300_cmd_header_t*)r300->hw.tex.offset.cmd)->unchecked_state.count = max_texture_unit+1;
	((drm_r300_cmd_header_t*)r300->hw.tex.unknown4.cmd)->unchecked_state.count = max_texture_unit+1;
	((drm_r300_cmd_header_t*)r300->hw.tex.unknown5.cmd)->unchecked_state.count = max_texture_unit+1;
	//((drm_r300_cmd_header_t*)r300->hw.tex.border_color.cmd)->unchecked_state.count = max_texture_unit+1;

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

	#if 1
	for(i = 2; i <= 8; ++i)
		r300->hw.ri.cmd[i] |= 4;
	#endif

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

void r300SetupVertexProgram(r300ContextPtr rmesa);

void r300SetupVertexShader(r300ContextPtr rmesa)
{
	GLcontext* ctx = rmesa->radeon.glCtx;
	
	if(rmesa->current_vp != NULL){
		r300SetupVertexProgram(rmesa);
		return ;
	}	
	
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

void r300SetupVertexProgram(r300ContextPtr rmesa)
{
	GLcontext* ctx = rmesa->radeon.glCtx;
	int inst_count;
	int param_count;

	/* Reset state, in case we don't use something */
	((drm_r300_cmd_header_t*)rmesa->hw.vpp.cmd)->vpu.count = 0;
	((drm_r300_cmd_header_t*)rmesa->hw.vpi.cmd)->vpu.count = 0;
	((drm_r300_cmd_header_t*)rmesa->hw.vps.cmd)->vpu.count = 0;

	r300VertexProgUpdateParams(ctx, rmesa->current_vp);
	
	setup_vertex_shader_fragment(rmesa, VSF_DEST_PROGRAM, &(rmesa->current_vp->program));

	setup_vertex_shader_fragment(rmesa, VSF_DEST_MATRIX0, &(rmesa->current_vp->params));
	
	#if 0
	setup_vertex_shader_fragment(rmesa, VSF_DEST_UNKNOWN1, &(rmesa->state.vertex_shader.unknown1));
	setup_vertex_shader_fragment(rmesa, VSF_DEST_UNKNOWN2, &(rmesa->state.vertex_shader.unknown2));
	#endif
	
	inst_count=rmesa->current_vp->program.length/4 - 1;
	param_count=rmesa->current_vp->params.length/4;
					
	R300_STATECHANGE(rmesa, pvs);
	rmesa->hw.pvs.cmd[R300_PVS_CNTL_1]=(0 << R300_PVS_CNTL_1_PROGRAM_START_SHIFT)
		| (inst_count/*0*/ << R300_PVS_CNTL_1_UNKNOWN_SHIFT)
		| (inst_count << R300_PVS_CNTL_1_PROGRAM_END_SHIFT);
	rmesa->hw.pvs.cmd[R300_PVS_CNTL_2]=(0 << R300_PVS_CNTL_2_PARAM_OFFSET_SHIFT)
		| (param_count << R300_PVS_CNTL_2_PARAM_COUNT_SHIFT);
	rmesa->hw.pvs.cmd[R300_PVS_CNTL_3]=(0/*rmesa->state.vertex_shader.unknown_ptr2*/ << R300_PVS_CNTL_3_PROGRAM_UNKNOWN_SHIFT)
	| ((inst_count-rmesa->current_vp->t2rs) /*rmesa->state.vertex_shader.unknown_ptr3*/ << 0);

	/* This is done for vertex shader fragments, but also needs to be done for vap_pvs,
	so I leave it as a reminder */
	#if 0
	reg_start(R300_VAP_PVS_WAITIDLE,0);
		e32(0x00000000);
	#endif
}


/* just a skeleton for now.. */
void r300GenerateTexturePixelShader(r300ContextPtr r300)
{
	int i, mtu;
	mtu = r300->radeon.glCtx->Const.MaxTextureUnits;
	GLenum envMode;
	
	int tex_inst=0, alu_inst=0;
	
	for(i=0;i<mtu;i++){
		/* No need to proliferate {} */
		if(! (r300->state.render_inputs & (_TNL_BIT_TEX0<<i)))continue;
		
		envMode = r300->radeon.glCtx->Texture.Unit[i].EnvMode;
		//fprintf(stderr, "envMode=%s\n", _mesa_lookup_enum_by_nr(envMode));
		
		/* Fetch textured pixel */
		
		r300->state.pixel_shader.program.tex.inst[tex_inst]=0x00018000;
		tex_inst++;
		
		switch(r300->radeon.glCtx->Texture.Unit[i]._CurrentCombine->ModeRGB){
			case GL_REPLACE:
				WARN_ONCE("ModeA==GL_REPLACE is possibly broken.\n");
				r300->state.pixel_shader.program.alu.inst[alu_inst].inst0=
					EASY_PFS_INSTR0(MAD, SRC0C_XYZ, ONE, ZERO);
			
				r300->state.pixel_shader.program.alu.inst[alu_inst].inst1=
					EASY_PFS_INSTR1(0, 0, 0 | PFS_FLAG_CONST, 0 | PFS_FLAG_CONST, NONE, ALL);
				break;
			case GL_MODULATE:
				WARN_ONCE("ModeRGB==GL_MODULATE is possibly broken.\n");
				r300->state.pixel_shader.program.alu.inst[alu_inst].inst0=
					EASY_PFS_INSTR0(MAD, SRC0C_XYZ, SRC1C_XYZ, ZERO);
				
				r300->state.pixel_shader.program.alu.inst[alu_inst].inst1=
					EASY_PFS_INSTR1(0, 0, 1, 0 | PFS_FLAG_CONST, NONE, ALL);
				
				break;
			default:
				fprintf(stderr, "ModeRGB=%s is not implemented yet !\n",
					 _mesa_lookup_enum_by_nr(r300->radeon.glCtx->Texture.Unit[i]._CurrentCombine->ModeRGB));
				/* PFS_NOP */
				r300->state.pixel_shader.program.alu.inst[alu_inst].inst0=
					EASY_PFS_INSTR0(MAD, SRC0C_XYZ, ONE, ZERO);
			
				r300->state.pixel_shader.program.alu.inst[alu_inst].inst1=
					EASY_PFS_INSTR1(0, 0, 0 | PFS_FLAG_CONST, 0 | PFS_FLAG_CONST, NONE, ALL);
			}
		switch(r300->radeon.glCtx->Texture.Unit[i]._CurrentCombine->ModeA){
			case GL_REPLACE:
				WARN_ONCE("ModeA==GL_REPLACE is possibly broken.\n");
				r300->state.pixel_shader.program.alu.inst[alu_inst].inst2=
					EASY_PFS_INSTR2(MAD, SRC0A, ONE, ZERO);
				
				r300->state.pixel_shader.program.alu.inst[alu_inst].inst3=
					EASY_PFS_INSTR3(0, 0, 0| PFS_FLAG_CONST, 0 | PFS_FLAG_CONST, OUTPUT);

				#if 0
				fprintf(stderr, "numArgsA=%d sourceA[0]=%s op=%d\n",
					 r300->radeon.glCtx->Texture.Unit[i]._CurrentCombine->_NumArgsA,
					 _mesa_lookup_enum_by_nr(r300->radeon.glCtx->Texture.Unit[i]._CurrentCombine->SourceA[0]),
					 r300->radeon.glCtx->Texture.Unit[i]._CurrentCombine->OperandA[0]-GL_SRC_ALPHA);
				#endif
				break;				
			case GL_MODULATE:
				WARN_ONCE("ModeA==GL_MODULATE is possibly broken.\n");
				
				r300->state.pixel_shader.program.alu.inst[alu_inst].inst2=
					EASY_PFS_INSTR2(MAD, SRC0A, SRC1A, ZERO);
				
				r300->state.pixel_shader.program.alu.inst[alu_inst].inst3=
					EASY_PFS_INSTR3(0, 0, 1, 0 | PFS_FLAG_CONST, OUTPUT);
				
				break;
			default:
				fprintf(stderr, "ModeA=%s is not implemented yet !\n",
					 _mesa_lookup_enum_by_nr(r300->radeon.glCtx->Texture.Unit[i]._CurrentCombine->ModeA));
				/* PFS_NOP */
				r300->state.pixel_shader.program.alu.inst[alu_inst].inst2=
					EASY_PFS_INSTR2(MAD, SRC0A, ONE, ZERO);
				
				r300->state.pixel_shader.program.alu.inst[alu_inst].inst3=
					EASY_PFS_INSTR3(0, 0, 0 | PFS_FLAG_CONST, 0 | PFS_FLAG_CONST, OUTPUT);

			}
					
		alu_inst++;				
		}
		
	r300->state.pixel_shader.program.tex.length=tex_inst;
	r300->state.pixel_shader.program.tex_offset=0;
	r300->state.pixel_shader.program.tex_end=tex_inst-1;

	#if 0
	/* saturate last instruction, like i915 driver does */
	r300->state.pixel_shader.program.alu.inst[alu_inst-1].inst0|=R300_FPI0_OUTC_SAT;
	r300->state.pixel_shader.program.alu.inst[alu_inst-1].inst2|=R300_FPI2_OUTA_SAT;
	#endif
		
	r300->state.pixel_shader.program.alu.length=alu_inst;
	r300->state.pixel_shader.program.alu_offset=0;
	r300->state.pixel_shader.program.alu_end=alu_inst-1;
}

void r300SetupPixelShader(r300ContextPtr rmesa)
{
int i,k;

	/* This needs to be replaced by pixel shader generation code */

	/* textures enabled ? */
	if(rmesa->state.texture.tc_count>0){
		rmesa->state.pixel_shader=SINGLE_TEXTURE_PIXEL_SHADER;
		r300GenerateTexturePixelShader(rmesa);
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

void update_zbias(GLcontext * ctx, int prim);

/**
 * Completely recalculates hardware state based on the Mesa state.
 */
void r300ResetHwState(r300ContextPtr r300)
{
	GLcontext* ctx = r300->radeon.glCtx;
	int i;

	if (RADEON_DEBUG & DEBUG_STATE)
		fprintf(stderr, "%s\n", __FUNCTION__);

		/* This is a place to initialize registers which
		   have bitfields accessed by different functions
		   and not all bits are used */
	#if 0
	r300->hw.zs.cmd[R300_ZS_CNTL_0] = 0;
	r300->hw.zs.cmd[R300_ZS_CNTL_1] = 0;
	r300->hw.zs.cmd[R300_ZS_CNTL_2] = 0xffff00;
	#endif

		/* go and compute register values from GL state */

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

	r300UpdateTextureState(ctx);
	
	r300_setup_routing(ctx, GL_TRUE);
	r300_setup_textures(ctx);
	r300_setup_rs_unit(ctx);

	r300SetupVertexShader(r300);
	r300SetupPixelShader(r300);

	r300_set_blend_state(ctx);
	r300AlphaFunc(ctx, ctx->Color.AlphaFunc, ctx->Color.AlphaRef);

		/* Initialize magic registers
		 TODO : learn what they really do, or get rid of
		 those we don't have to touch */
	r300->hw.unk2080.cmd[1] = 0x0030045A;

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
#ifdef MESA_BIG_ENDIAN
	r300->hw.unk2140.cmd[1] = 0x00000002;
#else
	r300->hw.unk2140.cmd[1] = 0x00000000;
#endif

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

	update_zbias(ctx, GL_TRIANGLES);/* FIXME */
#if 0	
	r300->hw.unk42B4.cmd[1] = 0x00000000;
#endif	
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

	r300->hw.at.cmd[R300_AT_UNKNOWN] = 0;
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

	r300->hw.unk4F10.cmd[1] = 0x00000002; // depthbuffer format?
	r300->hw.unk4F10.cmd[2] = 0x00000000;
	r300->hw.unk4F10.cmd[3] = 0x00000003;
	r300->hw.unk4F10.cmd[4] = 0x00000000;

	/* experiment a bit */
	r300->hw.unk4F10.cmd[2] = 0x00000001; // depthbuffer format?

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

	/* Only have hw stencil when depth buffer is 24 bits deep */
	r300->state.hw_stencil = (ctx->Visual.stencilBits > 0 &&
					 ctx->Visual.depthBits == 24);

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
	functions->AlphaFunc = r300AlphaFunc;
	functions->BlendColor = r300BlendColor;
	functions->BlendEquationSeparate = r300BlendEquationSeparate;
	functions->BlendFuncSeparate = r300BlendFuncSeparate;
	functions->Enable = r300Enable;
	functions->ColorMask = r300ColorMask;
	functions->DepthFunc = r300DepthFunc;
	functions->DepthMask = r300DepthMask;
	functions->CullFace = r300CullFace;
	functions->FrontFace = r300FrontFace;

	/* Stencil related */
	functions->ClearStencil = r300ClearStencil;
	functions->StencilFunc = r300StencilFunc;
	functions->StencilMask = r300StencilMask;
	functions->StencilOp = r300StencilOp;

	/* Viewport related */
	functions->Viewport = r300Viewport;
	functions->DepthRange = r300DepthRange;
	functions->PointSize = r300PointSize;
	
	functions->PolygonOffset = r300PolygonOffset;
}

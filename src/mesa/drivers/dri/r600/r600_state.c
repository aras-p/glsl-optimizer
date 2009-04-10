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

/**
 * \file
 *
 * \author Nicolai Haehnle <prefect_@gmx.net>
 */

#include "main/glheader.h"
#include "main/state.h"
#include "main/imports.h"
#include "main/enums.h"
#include "main/macros.h"
#include "main/context.h"
#include "main/dd.h"
#include "main/framebuffer.h"
#include "main/simple_list.h"
#include "main/api_arrayelt.h"
#include "main/texformat.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "shader/prog_parameter.h"
#include "shader/prog_statevars.h"
#include "vbo/vbo.h"
#include "tnl/tnl.h"

#include "r600_context.h"
#include "r600_ioctl.h"
#include "r600_state.h"
#include "r600_reg.h"
#include "r600_emit.h"
#include "r600_fragprog.h"
#include "r600_tex.h"

#include "drirenderbuffer.h"

extern int future_hw_tcl_on;
extern void _tnl_UpdateFixedFunctionProgram(GLcontext * ctx);

static void r600BlendColor(GLcontext * ctx, const GLfloat cf[4])
{
	r600ContextPtr rmesa = R600_CONTEXT(ctx);
	GLubyte color[4];

	R600_STATECHANGE(rmesa, blend_color);

	CLAMPED_FLOAT_TO_UBYTE(color[0], cf[0]);
	CLAMPED_FLOAT_TO_UBYTE(color[1], cf[1]);
	CLAMPED_FLOAT_TO_UBYTE(color[2], cf[2]);
	CLAMPED_FLOAT_TO_UBYTE(color[3], cf[3]);

	rmesa->hw.blend_color.cmd[1] = PACK_COLOR_8888(color[3], color[0],
						       color[1], color[2]);
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
	switch (factor) {
	case GL_ZERO:
		return R600_BLEND_GL_ZERO;
		break;
	case GL_ONE:
		return R600_BLEND_GL_ONE;
		break;
	case GL_DST_COLOR:
		return R600_BLEND_GL_DST_COLOR;
		break;
	case GL_ONE_MINUS_DST_COLOR:
		return R600_BLEND_GL_ONE_MINUS_DST_COLOR;
		break;
	case GL_SRC_COLOR:
		return R600_BLEND_GL_SRC_COLOR;
		break;
	case GL_ONE_MINUS_SRC_COLOR:
		return R600_BLEND_GL_ONE_MINUS_SRC_COLOR;
		break;
	case GL_SRC_ALPHA:
		return R600_BLEND_GL_SRC_ALPHA;
		break;
	case GL_ONE_MINUS_SRC_ALPHA:
		return R600_BLEND_GL_ONE_MINUS_SRC_ALPHA;
		break;
	case GL_DST_ALPHA:
		return R600_BLEND_GL_DST_ALPHA;
		break;
	case GL_ONE_MINUS_DST_ALPHA:
		return R600_BLEND_GL_ONE_MINUS_DST_ALPHA;
		break;
	case GL_SRC_ALPHA_SATURATE:
		return (is_src) ? R600_BLEND_GL_SRC_ALPHA_SATURATE :
		    R600_BLEND_GL_ZERO;
		break;
	case GL_CONSTANT_COLOR:
		return R600_BLEND_GL_CONST_COLOR;
		break;
	case GL_ONE_MINUS_CONSTANT_COLOR:
		return R600_BLEND_GL_ONE_MINUS_CONST_COLOR;
		break;
	case GL_CONSTANT_ALPHA:
		return R600_BLEND_GL_CONST_ALPHA;
		break;
	case GL_ONE_MINUS_CONSTANT_ALPHA:
		return R600_BLEND_GL_ONE_MINUS_CONST_ALPHA;
		break;
	default:
		fprintf(stderr, "unknown blend factor %x\n", factor);
		return (is_src) ? R600_BLEND_GL_ONE : R600_BLEND_GL_ZERO;
		break;
	}
}

/**
 * Sets both the blend equation and the blend function.
 * This is done in a single
 * function because some blend equations (i.e., \c GL_MIN and \c GL_MAX)
 * change the interpretation of the blend function.
 * Also, make sure that blend function and blend equation are set to their
 * default value if color blending is not enabled, since at least blend
 * equations GL_MIN and GL_FUNC_REVERSE_SUBTRACT will cause wrong results
 * otherwise for unknown reasons.
 */

/* helper function */
static void r600SetBlendCntl(r600ContextPtr r600, int func, int eqn,
			     int cbits, int funcA, int eqnA)
{
	GLuint new_ablend, new_cblend;

#if 0
	fprintf(stderr,
		"eqnA=%08x funcA=%08x eqn=%08x func=%08x cbits=%08x\n",
		eqnA, funcA, eqn, func, cbits);
#endif
	new_ablend = eqnA | funcA;
	new_cblend = eqn | func;

	/* Some blend factor combinations don't seem to work when the
	 * BLEND_NO_SEPARATE bit is set.
	 *
	 * Especially problematic candidates are the ONE_MINUS_* flags,
	 * but I can't see a real pattern.
	 */
#if 0
	if (new_ablend == new_cblend) {
		new_cblend |= R600_DISCARD_SRC_PIXELS_SRC_ALPHA_0;
	}
#endif
	new_cblend |= cbits;

	if ((new_ablend != r600->hw.bld.cmd[R600_BLD_ABLEND]) ||
	    (new_cblend != r600->hw.bld.cmd[R600_BLD_CBLEND])) {
		R600_STATECHANGE(r600, bld);
		r600->hw.bld.cmd[R600_BLD_ABLEND] = new_ablend;
		r600->hw.bld.cmd[R600_BLD_CBLEND] = new_cblend;
	}
}

static void r600SetBlendState(GLcontext * ctx)
{
	r600ContextPtr r600 = R600_CONTEXT(ctx);
	int func = (R600_BLEND_GL_ONE << R600_SRC_BLEND_SHIFT) |
	    (R600_BLEND_GL_ZERO << R600_DST_BLEND_SHIFT);
	int eqn = R600_COMB_FCN_ADD_CLAMP;
	int funcA = (R600_BLEND_GL_ONE << R600_SRC_BLEND_SHIFT) |
	    (R600_BLEND_GL_ZERO << R600_DST_BLEND_SHIFT);
	int eqnA = R600_COMB_FCN_ADD_CLAMP;

	if (RGBA_LOGICOP_ENABLED(ctx) || !ctx->Color.BlendEnabled) {
		r600SetBlendCntl(r600, func, eqn, 0, func, eqn);
		return;
	}

	func =
	    (blend_factor(ctx->Color.BlendSrcRGB, GL_TRUE) <<
	     R600_SRC_BLEND_SHIFT) | (blend_factor(ctx->Color.BlendDstRGB,
						   GL_FALSE) <<
				      R600_DST_BLEND_SHIFT);

	switch (ctx->Color.BlendEquationRGB) {
	case GL_FUNC_ADD:
		eqn = R600_COMB_FCN_ADD_CLAMP;
		break;

	case GL_FUNC_SUBTRACT:
		eqn = R600_COMB_FCN_SUB_CLAMP;
		break;

	case GL_FUNC_REVERSE_SUBTRACT:
		eqn = R600_COMB_FCN_RSUB_CLAMP;
		break;

	case GL_MIN:
		eqn = R600_COMB_FCN_MIN;
		func = (R600_BLEND_GL_ONE << R600_SRC_BLEND_SHIFT) |
		    (R600_BLEND_GL_ONE << R600_DST_BLEND_SHIFT);
		break;

	case GL_MAX:
		eqn = R600_COMB_FCN_MAX;
		func = (R600_BLEND_GL_ONE << R600_SRC_BLEND_SHIFT) |
		    (R600_BLEND_GL_ONE << R600_DST_BLEND_SHIFT);
		break;

	default:
		fprintf(stderr,
			"[%s:%u] Invalid RGB blend equation (0x%04x).\n",
			__FUNCTION__, __LINE__, ctx->Color.BlendEquationRGB);
		return;
	}

	funcA =
	    (blend_factor(ctx->Color.BlendSrcA, GL_TRUE) <<
	     R600_SRC_BLEND_SHIFT) | (blend_factor(ctx->Color.BlendDstA,
						   GL_FALSE) <<
				      R600_DST_BLEND_SHIFT);

	switch (ctx->Color.BlendEquationA) {
	case GL_FUNC_ADD:
		eqnA = R600_COMB_FCN_ADD_CLAMP;
		break;

	case GL_FUNC_SUBTRACT:
		eqnA = R600_COMB_FCN_SUB_CLAMP;
		break;

	case GL_FUNC_REVERSE_SUBTRACT:
		eqnA = R600_COMB_FCN_RSUB_CLAMP;
		break;

	case GL_MIN:
		eqnA = R600_COMB_FCN_MIN;
		funcA = (R600_BLEND_GL_ONE << R600_SRC_BLEND_SHIFT) |
		    (R600_BLEND_GL_ONE << R600_DST_BLEND_SHIFT);
		break;

	case GL_MAX:
		eqnA = R600_COMB_FCN_MAX;
		funcA = (R600_BLEND_GL_ONE << R600_SRC_BLEND_SHIFT) |
		    (R600_BLEND_GL_ONE << R600_DST_BLEND_SHIFT);
		break;

	default:
		fprintf(stderr,
			"[%s:%u] Invalid A blend equation (0x%04x).\n",
			__FUNCTION__, __LINE__, ctx->Color.BlendEquationA);
		return;
	}

	r600SetBlendCntl(r600,
			 func, eqn,
			 (R600_SEPARATE_ALPHA_ENABLE |
			  R600_READ_ENABLE |
			  R600_ALPHA_BLEND_ENABLE), funcA, eqnA);
}

static void r600BlendEquationSeparate(GLcontext * ctx,
				      GLenum modeRGB, GLenum modeA)
{
	r600SetBlendState(ctx);
}

static void r600BlendFuncSeparate(GLcontext * ctx,
				  GLenum sfactorRGB, GLenum dfactorRGB,
				  GLenum sfactorA, GLenum dfactorA)
{
	r600SetBlendState(ctx);
}

/**
 * Translate LogicOp enums into hardware representation.
 * Both use a very logical bit-wise layout, but unfortunately the order
 * of bits is reversed.
 */
static GLuint translate_logicop(GLenum logicop)
{
	GLuint bits = logicop - GL_CLEAR;
	bits = ((bits & 1) << 3) | ((bits & 2) << 1) | ((bits & 4) >> 1) | ((bits & 8) >> 3);
	return bits << R600_RB3D_ROPCNTL_ROP_SHIFT;
}

/**
 * Used internally to update the r600->hw hardware state to match the
 * current OpenGL state.
 */
static void r600SetLogicOpState(GLcontext *ctx)
{
	r600ContextPtr r600 = R600_CONTEXT(ctx);
	R600_STATECHANGE(r600, rop);
	if (RGBA_LOGICOP_ENABLED(ctx)) {
		r600->hw.rop.cmd[1] = R600_RB3D_ROPCNTL_ROP_ENABLE |
			translate_logicop(ctx->Color.LogicOp);
	} else {
		r600->hw.rop.cmd[1] = 0;
	}
}

/**
 * Called by Mesa when an application program changes the LogicOp state
 * via glLogicOp.
 */
static void r600LogicOpcode(GLcontext *ctx, GLenum logicop)
{
	if (RGBA_LOGICOP_ENABLED(ctx))
		r600SetLogicOpState(ctx);
}

static void r600ClipPlane( GLcontext *ctx, GLenum plane, const GLfloat *eq )
{
	r600ContextPtr rmesa = R600_CONTEXT(ctx);
	GLint p;
	GLint *ip;

	p = (GLint) plane - (GLint) GL_CLIP_PLANE0;
	ip = (GLint *)ctx->Transform._ClipUserPlane[p];

	R600_STATECHANGE( rmesa, vpucp[p] );
	rmesa->hw.vpucp[p].cmd[R600_VPUCP_X] = ip[0];
	rmesa->hw.vpucp[p].cmd[R600_VPUCP_Y] = ip[1];
	rmesa->hw.vpucp[p].cmd[R600_VPUCP_Z] = ip[2];
	rmesa->hw.vpucp[p].cmd[R600_VPUCP_W] = ip[3];
}

static void r600SetClipPlaneState(GLcontext * ctx, GLenum cap, GLboolean state)
{
	r600ContextPtr r600 = R600_CONTEXT(ctx);
	GLuint p;

	p = cap - GL_CLIP_PLANE0;
	R600_STATECHANGE(r600, vap_clip_cntl);
	if (state) {
		r600->hw.vap_clip_cntl.cmd[1] |= (R600_VAP_UCP_ENABLE_0 << p);
		r600ClipPlane(ctx, cap, NULL);
	} else {
		r600->hw.vap_clip_cntl.cmd[1] &= ~(R600_VAP_UCP_ENABLE_0 << p);
	}
}

/**
 * Update our tracked culling state based on Mesa's state.
 */
static void r600UpdateCulling(GLcontext * ctx)
{
	r600ContextPtr r600 = R600_CONTEXT(ctx);
	uint32_t val = 0;

	if (ctx->Polygon.CullFlag) {
		switch (ctx->Polygon.CullFaceMode) {
		case GL_FRONT:
			val = R600_CULL_FRONT;
			break;
		case GL_BACK:
			val = R600_CULL_BACK;
			break;
		case GL_FRONT_AND_BACK:
			val = R600_CULL_FRONT | R600_CULL_BACK;
			break;
		default:
			break;
		}
	}

	switch (ctx->Polygon.FrontFace) {
	case GL_CW:
		val |= R600_FRONT_FACE_CW;
		break;
	case GL_CCW:
		val |= R600_FRONT_FACE_CCW;
		break;
	default:
		break;
	}

	R600_STATECHANGE(r600, cul);
	r600->hw.cul.cmd[R600_CUL_CULL] = val;
}

static void r600SetPolygonOffsetState(GLcontext * ctx, GLboolean state)
{
	r600ContextPtr r600 = R600_CONTEXT(ctx);

	R600_STATECHANGE(r600, occlusion_cntl);
	if (state) {
		r600->hw.occlusion_cntl.cmd[1] |= (3 << 0);
	} else {
		r600->hw.occlusion_cntl.cmd[1] &= ~(3 << 0);
	}
}

static GLboolean current_fragment_program_writes_depth(GLcontext* ctx)
{
	r600ContextPtr r600 = R600_CONTEXT(ctx);

	struct r600_fragment_program *fp = (struct r600_fragment_program *)
		(char *)ctx->FragmentProgram._Current;
	return (fp && fp->WritesDepth);

}

static void r600SetEarlyZState(GLcontext * ctx)
{
	r600ContextPtr r600 = R600_CONTEXT(ctx);
	GLuint topZ = R600_ZTOP_ENABLE;

	if (ctx->Color.AlphaEnabled && ctx->Color.AlphaFunc != GL_ALWAYS)
		topZ = R600_ZTOP_DISABLE;
	if (current_fragment_program_writes_depth(ctx))
		topZ = R600_ZTOP_DISABLE;

	if (topZ != r600->hw.zstencil_format.cmd[2]) {
		/* Note: This completely reemits the stencil format.
		 * I have not tested whether this is strictly necessary,
		 * or if emitting a write to ZB_ZTOP is enough.
		 */
		R600_STATECHANGE(r600, zstencil_format);
		r600->hw.zstencil_format.cmd[2] = topZ;
	}
}

static void r600SetAlphaState(GLcontext * ctx)
{
	r600ContextPtr r600 = R600_CONTEXT(ctx);
	GLubyte refByte;
	uint32_t pp_misc = 0x0;
	GLboolean really_enabled = ctx->Color.AlphaEnabled;

	CLAMPED_FLOAT_TO_UBYTE(refByte, ctx->Color.AlphaRef);

	switch (ctx->Color.AlphaFunc) {
	case GL_NEVER:
		pp_misc |= R600_FG_ALPHA_FUNC_NEVER;
		break;
	case GL_LESS:
		pp_misc |= R600_FG_ALPHA_FUNC_LESS;
		break;
	case GL_EQUAL:
		pp_misc |= R600_FG_ALPHA_FUNC_EQUAL;
		break;
	case GL_LEQUAL:
		pp_misc |= R600_FG_ALPHA_FUNC_LE;
		break;
	case GL_GREATER:
		pp_misc |= R600_FG_ALPHA_FUNC_GREATER;
		break;
	case GL_NOTEQUAL:
		pp_misc |= R600_FG_ALPHA_FUNC_NOTEQUAL;
		break;
	case GL_GEQUAL:
		pp_misc |= R600_FG_ALPHA_FUNC_GE;
		break;
	case GL_ALWAYS:
		/*pp_misc |= FG_ALPHA_FUNC_ALWAYS; */
		really_enabled = GL_FALSE;
		break;
	}

	if (really_enabled) {
		pp_misc |= R600_FG_ALPHA_FUNC_ENABLE;
		pp_misc |= (refByte & R600_FG_ALPHA_FUNC_VAL_MASK);
	} else {
		pp_misc = 0x0;
	}

	R600_STATECHANGE(r600, at);
	r600->hw.at.cmd[R600_AT_ALPHA_TEST] = pp_misc;
	r600->hw.at.cmd[R600_AT_UNKNOWN] = 0;

	r600SetEarlyZState(ctx);
}

static void r600AlphaFunc(GLcontext * ctx, GLenum func, GLfloat ref)
{
	(void)func;
	(void)ref;
	r600SetAlphaState(ctx);
}

static int translate_func(int func)
{
	switch (func) {
	case GL_NEVER:
		return R600_ZS_NEVER;
	case GL_LESS:
		return R600_ZS_LESS;
	case GL_EQUAL:
		return R600_ZS_EQUAL;
	case GL_LEQUAL:
		return R600_ZS_LEQUAL;
	case GL_GREATER:
		return R600_ZS_GREATER;
	case GL_NOTEQUAL:
		return R600_ZS_NOTEQUAL;
	case GL_GEQUAL:
		return R600_ZS_GEQUAL;
	case GL_ALWAYS:
		return R600_ZS_ALWAYS;
	}
	return 0;
}

static void r600SetDepthState(GLcontext * ctx)
{
	r600ContextPtr r600 = R600_CONTEXT(ctx);

	R600_STATECHANGE(r600, zs);
	r600->hw.zs.cmd[R600_ZS_CNTL_0] &= R600_STENCIL_ENABLE|R600_STENCIL_FRONT_BACK;
	r600->hw.zs.cmd[R600_ZS_CNTL_1] &= ~(R600_ZS_MASK << R600_Z_FUNC_SHIFT);

	if (ctx->Depth.Test) {
		r600->hw.zs.cmd[R600_ZS_CNTL_0] |= R600_Z_ENABLE;
		if (ctx->Depth.Mask)
			r600->hw.zs.cmd[R600_ZS_CNTL_0] |= R600_Z_WRITE_ENABLE;
		r600->hw.zs.cmd[R600_ZS_CNTL_1] |=
		    translate_func(ctx->Depth.Func) << R600_Z_FUNC_SHIFT;
	}

	r600SetEarlyZState(ctx);
}

static void r600SetStencilState(GLcontext * ctx, GLboolean state)
{
	r600ContextPtr r600 = R600_CONTEXT(ctx);
	GLboolean hw_stencil = GL_FALSE;
	if (ctx->DrawBuffer) {
		struct radeon_renderbuffer *rrbStencil
			= radeon_get_renderbuffer(ctx->DrawBuffer, BUFFER_STENCIL);
		hw_stencil = (rrbStencil && rrbStencil->bo);
	}

	if (hw_stencil) {
		R600_STATECHANGE(r600, zs);
		if (state) {
			r600->hw.zs.cmd[R600_ZS_CNTL_0] |=
			    R600_STENCIL_ENABLE;
		} else {
			r600->hw.zs.cmd[R600_ZS_CNTL_0] &=
			    ~R600_STENCIL_ENABLE;
		}
	} else {
#if R200_MERGED
		FALLBACK(&r600->radeon, RADEON_FALLBACK_STENCIL, state);
#endif
	}
}

static void r600UpdatePolygonMode(GLcontext * ctx)
{
	r600ContextPtr r600 = R600_CONTEXT(ctx);
	uint32_t hw_mode = R600_GA_POLY_MODE_DISABLE;

	/* Only do something if a polygon mode is wanted, default is GL_FILL */
	if (ctx->Polygon.FrontMode != GL_FILL ||
	    ctx->Polygon.BackMode != GL_FILL) {
		GLenum f, b;

		/* Handle GL_CW (clock wise and GL_CCW (counter clock wise)
		 * correctly by selecting the correct front and back face
		 */
		if (ctx->Polygon.FrontFace == GL_CCW) {
			f = ctx->Polygon.FrontMode;
			b = ctx->Polygon.BackMode;
		} else {
			f = ctx->Polygon.BackMode;
			b = ctx->Polygon.FrontMode;
		}

		/* Enable polygon mode */
		hw_mode |= R600_GA_POLY_MODE_DUAL;

		switch (f) {
		case GL_LINE:
			hw_mode |= R600_GA_POLY_MODE_FRONT_PTYPE_LINE;
			break;
		case GL_POINT:
			hw_mode |= R600_GA_POLY_MODE_FRONT_PTYPE_POINT;
			break;
		case GL_FILL:
			hw_mode |= R600_GA_POLY_MODE_FRONT_PTYPE_TRI;
			break;
		}

		switch (b) {
		case GL_LINE:
			hw_mode |= R600_GA_POLY_MODE_BACK_PTYPE_LINE;
			break;
		case GL_POINT:
			hw_mode |= R600_GA_POLY_MODE_BACK_PTYPE_POINT;
			break;
		case GL_FILL:
			hw_mode |= R600_GA_POLY_MODE_BACK_PTYPE_TRI;
			break;
		}
	}

	if (r600->hw.polygon_mode.cmd[1] != hw_mode) {
		R600_STATECHANGE(r600, polygon_mode);
		r600->hw.polygon_mode.cmd[1] = hw_mode;
	}

	r600->hw.polygon_mode.cmd[2] = 0x00000001;
	r600->hw.polygon_mode.cmd[3] = 0x00000000;
}

/**
 * Change the culling mode.
 *
 * \note Mesa already filters redundant calls to this function.
 */
static void r600CullFace(GLcontext * ctx, GLenum mode)
{
	(void)mode;

	r600UpdateCulling(ctx);
}

/**
 * Change the polygon orientation.
 *
 * \note Mesa already filters redundant calls to this function.
 */
static void r600FrontFace(GLcontext * ctx, GLenum mode)
{
	(void)mode;

	r600UpdateCulling(ctx);
	r600UpdatePolygonMode(ctx);
}

/**
 * Change the depth testing function.
 *
 * \note Mesa already filters redundant calls to this function.
 */
static void r600DepthFunc(GLcontext * ctx, GLenum func)
{
	(void)func;
	r600SetDepthState(ctx);
}

/**
 * Enable/Disable depth writing.
 *
 * \note Mesa already filters redundant calls to this function.
 */
static void r600DepthMask(GLcontext * ctx, GLboolean mask)
{
	(void)mask;
	r600SetDepthState(ctx);
}

/**
 * Handle glColorMask()
 */
static void r600ColorMask(GLcontext * ctx,
			  GLboolean r, GLboolean g, GLboolean b, GLboolean a)
{
	r600ContextPtr r600 = R600_CONTEXT(ctx);
	int mask = (r ? RB3D_COLOR_CHANNEL_MASK_RED_MASK0 : 0) |
	    (g ? RB3D_COLOR_CHANNEL_MASK_GREEN_MASK0 : 0) |
	    (b ? RB3D_COLOR_CHANNEL_MASK_BLUE_MASK0 : 0) |
	    (a ? RB3D_COLOR_CHANNEL_MASK_ALPHA_MASK0 : 0);

	if (mask != r600->hw.cmk.cmd[R600_CMK_COLORMASK]) {
		R600_STATECHANGE(r600, cmk);
		r600->hw.cmk.cmd[R600_CMK_COLORMASK] = mask;
	}
}

/* =============================================================
 * Point state
 */
static void r600PointSize(GLcontext * ctx, GLfloat size)
{
	r600ContextPtr r600 = R600_CONTEXT(ctx);
        /* same size limits for AA, non-AA points */
	size = CLAMP(size, ctx->Const.MinPointSize, ctx->Const.MaxPointSize);

	R600_STATECHANGE(r600, ps);
	r600->hw.ps.cmd[R600_PS_POINTSIZE] =
	    ((int)(size * 6) << R600_POINTSIZE_X_SHIFT) |
	    ((int)(size * 6) << R600_POINTSIZE_Y_SHIFT);
}

static void r600PointParameter(GLcontext * ctx, GLenum pname, const GLfloat * param)
{
	r600ContextPtr r600 = R600_CONTEXT(ctx);

	switch (pname) {
	case GL_POINT_SIZE_MIN:
		R600_STATECHANGE(r600, ga_point_minmax);
		r600->hw.ga_point_minmax.cmd[1] &= ~R600_GA_POINT_MINMAX_MIN_MASK;
		r600->hw.ga_point_minmax.cmd[1] |= (GLuint)(ctx->Point.MinSize * 6.0);
		break;
	case GL_POINT_SIZE_MAX:
		R600_STATECHANGE(r600, ga_point_minmax);
		r600->hw.ga_point_minmax.cmd[1] &= ~R600_GA_POINT_MINMAX_MAX_MASK;
		r600->hw.ga_point_minmax.cmd[1] |= (GLuint)(ctx->Point.MaxSize * 6.0)
			<< R600_GA_POINT_MINMAX_MAX_SHIFT;
		break;
	case GL_POINT_DISTANCE_ATTENUATION:
		break;
	case GL_POINT_FADE_THRESHOLD_SIZE:
		break;
	default:
		break;
	}
}

/* =============================================================
 * Line state
 */
static void r600LineWidth(GLcontext * ctx, GLfloat widthf)
{
	r600ContextPtr r600 = R600_CONTEXT(ctx);

	widthf = CLAMP(widthf,
                       ctx->Const.MinPointSize,
                       ctx->Const.MaxPointSize);
	R600_STATECHANGE(r600, lcntl);
	r600->hw.lcntl.cmd[1] =
	    R600_LINE_CNT_HO | R600_LINE_CNT_VE | (int)(widthf * 6.0);
}

static void r600PolygonMode(GLcontext * ctx, GLenum face, GLenum mode)
{
	(void)face;
	(void)mode;

	r600UpdatePolygonMode(ctx);
}

/* =============================================================
 * Stencil
 */

static int translate_stencil_op(int op)
{
	switch (op) {
	case GL_KEEP:
		return R600_ZS_KEEP;
	case GL_ZERO:
		return R600_ZS_ZERO;
	case GL_REPLACE:
		return R600_ZS_REPLACE;
	case GL_INCR:
		return R600_ZS_INCR;
	case GL_DECR:
		return R600_ZS_DECR;
	case GL_INCR_WRAP_EXT:
		return R600_ZS_INCR_WRAP;
	case GL_DECR_WRAP_EXT:
		return R600_ZS_DECR_WRAP;
	case GL_INVERT:
		return R600_ZS_INVERT;
	default:
		WARN_ONCE("Do not know how to translate stencil op");
		return R600_ZS_KEEP;
	}
	return 0;
}

static void r600ShadeModel(GLcontext * ctx, GLenum mode)
{
	r600ContextPtr rmesa = R600_CONTEXT(ctx);

	R600_STATECHANGE(rmesa, shade);
	rmesa->hw.shade.cmd[1] = 0x00000002;
	switch (mode) {
	case GL_FLAT:
		rmesa->hw.shade.cmd[2] = R600_RE_SHADE_MODEL_FLAT;
		break;
	case GL_SMOOTH:
		rmesa->hw.shade.cmd[2] = R600_RE_SHADE_MODEL_SMOOTH;
		break;
	default:
		return;
	}
	rmesa->hw.shade.cmd[3] = 0x00000000;
	rmesa->hw.shade.cmd[4] = 0x00000000;
}

static void r600StencilFuncSeparate(GLcontext * ctx, GLenum face,
				    GLenum func, GLint ref, GLuint mask)
{
	r600ContextPtr rmesa = R600_CONTEXT(ctx);
	GLuint refmask =
	    ((ctx->Stencil.Ref[0] & 0xff) << R600_STENCILREF_SHIFT)
	     | ((ctx->Stencil.ValueMask[0] & 0xff) << R600_STENCILMASK_SHIFT);
	const unsigned back = ctx->Stencil._BackFace;
	GLuint flag;

	R600_STATECHANGE(rmesa, zs);
	rmesa->hw.zs.cmd[R600_ZS_CNTL_0] |= R600_STENCIL_FRONT_BACK;
	rmesa->hw.zs.cmd[R600_ZS_CNTL_1] &= ~((R600_ZS_MASK <<
					       R600_S_FRONT_FUNC_SHIFT)
					      | (R600_ZS_MASK <<
						 R600_S_BACK_FUNC_SHIFT));

	rmesa->hw.zs.cmd[R600_ZS_CNTL_2] &=
	    ~((R600_STENCILREF_MASK << R600_STENCILREF_SHIFT) |
	      (R600_STENCILREF_MASK << R600_STENCILMASK_SHIFT));

	flag = translate_func(ctx->Stencil.Function[0]);
	rmesa->hw.zs.cmd[R600_ZS_CNTL_1] |=
	    (flag << R600_S_FRONT_FUNC_SHIFT);

	flag = translate_func(ctx->Stencil.Function[back]);

	rmesa->hw.zs.cmd[R600_ZS_CNTL_1] |=
	    (flag << R600_S_BACK_FUNC_SHIFT);
	rmesa->hw.zs.cmd[R600_ZS_CNTL_2] |= refmask;
}

static void r600StencilMaskSeparate(GLcontext * ctx, GLenum face, GLuint mask)
{
	r600ContextPtr rmesa = R600_CONTEXT(ctx);

	R600_STATECHANGE(rmesa, zs);
	rmesa->hw.zs.cmd[R600_ZS_CNTL_2] &=
	    ~(R600_STENCILREF_MASK <<
	      R600_STENCILWRITEMASK_SHIFT);
	rmesa->hw.zs.cmd[R600_ZS_CNTL_2] |=
	    (ctx->Stencil.
	     WriteMask[0] & R600_STENCILREF_MASK) <<
	     R600_STENCILWRITEMASK_SHIFT;
}

static void r600StencilOpSeparate(GLcontext * ctx, GLenum face,
				  GLenum fail, GLenum zfail, GLenum zpass)
{
	r600ContextPtr rmesa = R600_CONTEXT(ctx);
	const unsigned back = ctx->Stencil._BackFace;

	R600_STATECHANGE(rmesa, zs);
	/* It is easier to mask what's left.. */
	rmesa->hw.zs.cmd[R600_ZS_CNTL_1] &=
	    (R600_ZS_MASK << R600_Z_FUNC_SHIFT) |
	    (R600_ZS_MASK << R600_S_FRONT_FUNC_SHIFT) |
	    (R600_ZS_MASK << R600_S_BACK_FUNC_SHIFT);

	rmesa->hw.zs.cmd[R600_ZS_CNTL_1] |=
	    (translate_stencil_op(ctx->Stencil.FailFunc[0]) <<
	     R600_S_FRONT_SFAIL_OP_SHIFT)
	    | (translate_stencil_op(ctx->Stencil.ZFailFunc[0]) <<
	       R600_S_FRONT_ZFAIL_OP_SHIFT)
	    | (translate_stencil_op(ctx->Stencil.ZPassFunc[0]) <<
	       R600_S_FRONT_ZPASS_OP_SHIFT);

	rmesa->hw.zs.cmd[R600_ZS_CNTL_1] |=
	    (translate_stencil_op(ctx->Stencil.FailFunc[back]) <<
	     R600_S_BACK_SFAIL_OP_SHIFT)
	    | (translate_stencil_op(ctx->Stencil.ZFailFunc[back]) <<
	       R600_S_BACK_ZFAIL_OP_SHIFT)
	    | (translate_stencil_op(ctx->Stencil.ZPassFunc[back]) <<
	       R600_S_BACK_ZPASS_OP_SHIFT);
}

/* =============================================================
 * Window position and viewport transformation
 */

/*
 * To correctly position primitives:
 */
#define SUBPIXEL_X 0.125
#define SUBPIXEL_Y 0.125

static void r600UpdateWindow(GLcontext * ctx)
{
	r600ContextPtr rmesa = R600_CONTEXT(ctx);
	__DRIdrawablePrivate *dPriv = rmesa->radeon.dri.drawable;
	GLfloat xoffset = dPriv ? (GLfloat) dPriv->x : 0;
	GLfloat yoffset = dPriv ? (GLfloat) dPriv->y + dPriv->h : 0;
	const GLfloat *v = ctx->Viewport._WindowMap.m;
	const GLfloat depthScale = 1.0F / ctx->DrawBuffer->_DepthMaxF;
	const GLboolean render_to_fbo = (ctx->DrawBuffer->Name != 0);
	GLfloat y_scale, y_bias;

	if (render_to_fbo) {
		y_scale = 1.0;
		y_bias = 0;
	} else {
		y_scale = -1.0;
		y_bias = yoffset;
	}

	GLfloat sx = v[MAT_SX];
	GLfloat tx = v[MAT_TX] + xoffset + SUBPIXEL_X;
	GLfloat sy = v[MAT_SY] * y_scale;
	GLfloat ty = (v[MAT_TY] * y_scale) + y_bias + SUBPIXEL_Y;
	GLfloat sz = v[MAT_SZ] * depthScale;
	GLfloat tz = v[MAT_TZ] * depthScale;

	R600_STATECHANGE(rmesa, vpt);

	rmesa->hw.vpt.cmd[R600_VPT_XSCALE] = r600PackFloat32(sx);
	rmesa->hw.vpt.cmd[R600_VPT_XOFFSET] = r600PackFloat32(tx);
	rmesa->hw.vpt.cmd[R600_VPT_YSCALE] = r600PackFloat32(sy);
	rmesa->hw.vpt.cmd[R600_VPT_YOFFSET] = r600PackFloat32(ty);
	rmesa->hw.vpt.cmd[R600_VPT_ZSCALE] = r600PackFloat32(sz);
	rmesa->hw.vpt.cmd[R600_VPT_ZOFFSET] = r600PackFloat32(tz);
}

static void r600Viewport(GLcontext * ctx, GLint x, GLint y,
			 GLsizei width, GLsizei height)
{
	/* Don't pipeline viewport changes, conflict with window offset
	 * setting below.  Could apply deltas to rescue pipelined viewport
	 * values, or keep the originals hanging around.
	 */
	r600UpdateWindow(ctx);

	radeon_viewport(ctx, x, y, width, height);
}

static void r600DepthRange(GLcontext * ctx, GLclampd nearval, GLclampd farval)
{
	r600UpdateWindow(ctx);
}

void r600UpdateViewportOffset(GLcontext * ctx)
{
	r600ContextPtr rmesa = R600_CONTEXT(ctx);
	__DRIdrawablePrivate *dPriv = ((radeonContextPtr) rmesa)->dri.drawable;
	GLfloat xoffset = (GLfloat) dPriv->x;
	GLfloat yoffset = (GLfloat) dPriv->y + dPriv->h;
	const GLfloat *v = ctx->Viewport._WindowMap.m;

	GLfloat tx = v[MAT_TX] + xoffset + SUBPIXEL_X;
	GLfloat ty = (-v[MAT_TY]) + yoffset + SUBPIXEL_Y;

	if (rmesa->hw.vpt.cmd[R600_VPT_XOFFSET] != r600PackFloat32(tx) ||
	    rmesa->hw.vpt.cmd[R600_VPT_YOFFSET] != r600PackFloat32(ty)) {
		/* Note: this should also modify whatever data the context reset
		 * code uses...
		 */
		R600_STATECHANGE(rmesa, vpt);
		rmesa->hw.vpt.cmd[R600_VPT_XOFFSET] = r600PackFloat32(tx);
		rmesa->hw.vpt.cmd[R600_VPT_YOFFSET] = r600PackFloat32(ty);

	}

	radeonUpdateScissor(ctx);
}

static void
r600FetchStateParameter(GLcontext * ctx,
			const gl_state_index state[STATE_LENGTH],
			GLfloat * value)
{
	r600ContextPtr r600 = R600_CONTEXT(ctx);

	switch (state[0]) {
	case STATE_INTERNAL:
		switch (state[1]) {
		case STATE_R600_WINDOW_DIMENSION:
			value[0] = r600->radeon.dri.drawable->w * 0.5f;	/* width*0.5 */
			value[1] = r600->radeon.dri.drawable->h * 0.5f;	/* height*0.5 */
			value[2] = 0.5F;	/* for moving range [-1 1] -> [0 1] */
			value[3] = 1.0F;	/* not used */
			break;

		case STATE_R600_TEXRECT_FACTOR:{
				struct gl_texture_object *t =
				    ctx->Texture.Unit[state[2]].CurrentTex[TEXTURE_RECT_INDEX];

				if (t && t->Image[0][t->BaseLevel]) {
					struct gl_texture_image *image =
					    t->Image[0][t->BaseLevel];
					value[0] = 1.0 / image->Width2;
					value[1] = 1.0 / image->Height2;
				} else {
					value[0] = 1.0;
					value[1] = 1.0;
				}
				value[2] = 1.0;
				value[3] = 1.0;
				break;
			}

		default:
			break;
		}
		break;

	default:
		break;
	}
}

/**
 * Update R600's own internal state parameters.
 * For now just STATE_R600_WINDOW_DIMENSION
 */
void r600UpdateStateParameters(GLcontext * ctx, GLuint new_state)
{
	struct r600_fragment_program *fp;
	struct gl_program_parameter_list *paramList;
	GLuint i;

	if (!(new_state & (_NEW_BUFFERS | _NEW_PROGRAM)))
		return;

	fp = (struct r600_fragment_program *)ctx->FragmentProgram._Current;
	if (!fp)
		return;

	paramList = fp->mesa_program.Base.Parameters;

	if (!paramList)
		return;

	for (i = 0; i < paramList->NumParameters; i++) {
		if (paramList->Parameters[i].Type == PROGRAM_STATE_VAR) {
			r600FetchStateParameter(ctx,
						paramList->Parameters[i].
						StateIndexes,
						paramList->ParameterValues[i]);
		}
	}
}

/* =============================================================
 * Polygon state
 */
static void r600PolygonOffset(GLcontext * ctx, GLfloat factor, GLfloat units)
{
	r600ContextPtr rmesa = R600_CONTEXT(ctx);
	GLfloat constant = units;

	switch (ctx->Visual.depthBits) {
	case 16:
		constant *= 4.0;
		break;
	case 24:
		constant *= 2.0;
		break;
	}

	factor *= 12.0;

/*    fprintf(stderr, "%s f:%f u:%f\n", __FUNCTION__, factor, constant); */

	R600_STATECHANGE(rmesa, zbs);
	rmesa->hw.zbs.cmd[R600_ZBS_T_FACTOR] = r600PackFloat32(factor);
	rmesa->hw.zbs.cmd[R600_ZBS_T_CONSTANT] = r600PackFloat32(constant);
	rmesa->hw.zbs.cmd[R600_ZBS_W_FACTOR] = r600PackFloat32(factor);
	rmesa->hw.zbs.cmd[R600_ZBS_W_CONSTANT] = r600PackFloat32(constant);
}

/* Routing and texture-related */

/* r600 doesnt handle GL_CLAMP and GL_MIRROR_CLAMP_EXT correctly when filter is NEAREST.
 * Since texwrap produces same results for GL_CLAMP and GL_CLAMP_TO_EDGE we use them instead.
 * We need to recalculate wrap modes whenever filter mode is changed because someone might do:
 * glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
 * glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
 * glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
 * Since r600 completely ignores R600_TX_CLAMP when either min or mag is nearest it cant handle
 * combinations where only one of them is nearest.
 */
static unsigned long gen_fixed_filter(unsigned long f)
{
	unsigned long mag, min, needs_fixing = 0;
	//return f;

	/* We ignore MIRROR bit so we dont have to do everything twice */
	if ((f & ((7 - 1) << R600_TX_WRAP_S_SHIFT)) ==
	    (R600_TX_CLAMP << R600_TX_WRAP_S_SHIFT)) {
		needs_fixing |= 1;
	}
	if ((f & ((7 - 1) << R600_TX_WRAP_T_SHIFT)) ==
	    (R600_TX_CLAMP << R600_TX_WRAP_T_SHIFT)) {
		needs_fixing |= 2;
	}
	if ((f & ((7 - 1) << R600_TX_WRAP_R_SHIFT)) ==
	    (R600_TX_CLAMP << R600_TX_WRAP_R_SHIFT)) {
		needs_fixing |= 4;
	}

	if (!needs_fixing)
		return f;

	mag = f & R600_TX_MAG_FILTER_MASK;
	min = f & (R600_TX_MIN_FILTER_MASK|R600_TX_MIN_FILTER_MIP_MASK);

	/* TODO: Check for anisto filters too */
	if ((mag != R600_TX_MAG_FILTER_NEAREST)
	    && (min != R600_TX_MIN_FILTER_NEAREST))
		return f;

	/* r600 cant handle these modes hence we force nearest to linear */
	if ((mag == R600_TX_MAG_FILTER_NEAREST)
	    && (min != R600_TX_MIN_FILTER_NEAREST)) {
		f &= ~R600_TX_MAG_FILTER_NEAREST;
		f |= R600_TX_MAG_FILTER_LINEAR;
		return f;
	}

	if ((min == R600_TX_MIN_FILTER_NEAREST)
	    && (mag != R600_TX_MAG_FILTER_NEAREST)) {
		f &= ~R600_TX_MIN_FILTER_NEAREST;
		f |= R600_TX_MIN_FILTER_LINEAR;
		return f;
	}

	/* Both are nearest */
	if (needs_fixing & 1) {
		f &= ~((7 - 1) << R600_TX_WRAP_S_SHIFT);
		f |= R600_TX_CLAMP_TO_EDGE << R600_TX_WRAP_S_SHIFT;
	}
	if (needs_fixing & 2) {
		f &= ~((7 - 1) << R600_TX_WRAP_T_SHIFT);
		f |= R600_TX_CLAMP_TO_EDGE << R600_TX_WRAP_T_SHIFT;
	}
	if (needs_fixing & 4) {
		f &= ~((7 - 1) << R600_TX_WRAP_R_SHIFT);
		f |= R600_TX_CLAMP_TO_EDGE << R600_TX_WRAP_R_SHIFT;
	}
	return f;
}

static void r600SetupFragmentShaderTextures(GLcontext *ctx, int *tmu_mappings)
{
	r600ContextPtr r600 = R600_CONTEXT(ctx);
	int i;
	struct r600_fragment_program *fp = (struct r600_fragment_program *)
	    (char *)ctx->FragmentProgram._Current;
	struct r600_fragment_program_code *code = &fp->code;

	R600_STATECHANGE(r600, fpt);

	for (i = 0; i < code->tex.length; i++) {
		int unit;
		int opcode;
		unsigned long val;

		unit = code->tex.inst[i] >> R600_TEX_ID_SHIFT;
		unit &= 15;

		val = code->tex.inst[i];
		val &= ~R600_TEX_ID_MASK;

		opcode =
			(val & R600_TEX_INST_MASK) >> R600_TEX_INST_SHIFT;
		if (opcode == R600_TEX_OP_KIL) {
			r600->hw.fpt.cmd[R600_FPT_INSTR_0 + i] = val;
		} else {
			if (tmu_mappings[unit] >= 0) {
				val |=
					tmu_mappings[unit] <<
					R600_TEX_ID_SHIFT;
				r600->hw.fpt.cmd[R600_FPT_INSTR_0 + i] = val;
			} else {
				// We get here when the corresponding texture image is incomplete
				// (e.g. incomplete mipmaps etc.)
				r600->hw.fpt.cmd[R600_FPT_INSTR_0 + i] = val;
			}
		}
	}

	r600->hw.fpt.cmd[R600_FPT_CMD_0] =
		cmdpacket0(r600->radeon.radeonScreen,
                   R600_US_TEX_INST_0, code->tex.length);
}

static GLuint translate_lod_bias(GLfloat bias)
{
	GLint b = (int)(bias*32);
	if (b >= (1 << 9))
		b = (1 << 9)-1;
	else if (b < -(1 << 9))
		b = -(1 << 9);
	return (((GLuint)b) << R600_LOD_BIAS_SHIFT) & R600_LOD_BIAS_MASK;
}

static void r600SetupTextures(GLcontext * ctx)
{
	int i, mtu;
	struct radeon_tex_obj *t;
	r600ContextPtr r600 = R600_CONTEXT(ctx);
	int hw_tmu = 0;
	int last_hw_tmu = -1;	/* -1 translates into no setup costs for fields */
	int tmu_mappings[R600_MAX_TEXTURE_UNITS] = { -1, };
	struct r600_fragment_program *fp = (struct r600_fragment_program *)
	    (char *)ctx->FragmentProgram._Current;

	R600_STATECHANGE(r600, txe);
	R600_STATECHANGE(r600, tex.filter);
	R600_STATECHANGE(r600, tex.filter_1);
	R600_STATECHANGE(r600, tex.size);
	R600_STATECHANGE(r600, tex.format);
	R600_STATECHANGE(r600, tex.pitch);
	R600_STATECHANGE(r600, tex.offset);
	R600_STATECHANGE(r600, tex.chroma_key);
	R600_STATECHANGE(r600, tex.border_color);

	r600->hw.txe.cmd[R600_TXE_ENABLE] = 0x0;

	mtu = r600->radeon.glCtx->Const.MaxTextureUnits;
	if (RADEON_DEBUG & DEBUG_STATE)
		fprintf(stderr, "mtu=%d\n", mtu);

	if (mtu > R600_MAX_TEXTURE_UNITS) {
		fprintf(stderr,
			"Aiiee ! mtu=%d is greater than R600_MAX_TEXTURE_UNITS=%d\n",
			mtu, R600_MAX_TEXTURE_UNITS);
		_mesa_exit(-1);
	}

	/* We cannot let disabled tmu offsets pass DRM */
	for (i = 0; i < mtu; i++) {
		if (ctx->Texture.Unit[i]._ReallyEnabled) {
			tmu_mappings[i] = hw_tmu;

			t = radeon_tex_obj(ctx->Texture.Unit[i]._Current);
			if (!t)
				continue;

			if ((t->pp_txformat & 0xffffff00) == 0xffffff00) {
				WARN_ONCE
				    ("unknown texture format (entry %x) encountered. Help me !\n",
				     t->pp_txformat & 0xff);
			}

			if (RADEON_DEBUG & DEBUG_STATE)
				fprintf(stderr,
					"Activating texture unit %d\n", i);

			r600->hw.txe.cmd[R600_TXE_ENABLE] |= (1 << hw_tmu);

			r600->hw.tex.filter.cmd[R600_TEX_VALUE_0 +
						hw_tmu] =
			    gen_fixed_filter(t->pp_txfilter) | (hw_tmu << 28);
			/* Note: There is a LOD bias per texture unit and a LOD bias
			 * per texture object. We add them here to get the correct behaviour.
			 * (The per-texture object LOD bias was introduced in OpenGL 1.4
			 * and is not present in the EXT_texture_object extension).
			 */
			r600->hw.tex.filter_1.cmd[R600_TEX_VALUE_0 + hw_tmu] =
				t->pp_txfilter_1 |
				translate_lod_bias(ctx->Texture.Unit[i].LodBias + t->base.LodBias);
			r600->hw.tex.size.cmd[R600_TEX_VALUE_0 + hw_tmu] =
			    t->pp_txsize;
			r600->hw.tex.format.cmd[R600_TEX_VALUE_0 +
						hw_tmu] = t->pp_txformat;
			r600->hw.tex.pitch.cmd[R600_TEX_VALUE_0 + hw_tmu] =
			  t->pp_txpitch;
			r600->hw.textures[hw_tmu] = t;

			if (t->tile_bits & R600_TXO_MACRO_TILE) {
				WARN_ONCE("macro tiling enabled!\n");
			}

			if (t->tile_bits & R600_TXO_MICRO_TILE) {
				WARN_ONCE("micro tiling enabled!\n");
			}

			r600->hw.tex.chroma_key.cmd[R600_TEX_VALUE_0 +
						    hw_tmu] = 0x0;
			r600->hw.tex.border_color.cmd[R600_TEX_VALUE_0 +
						      hw_tmu] =
			    t->pp_border_color;

			last_hw_tmu = hw_tmu;

			hw_tmu++;
		}
	}

	r600->hw.tex.filter.cmd[R600_TEX_CMD_0] =
	    cmdpacket0(r600->radeon.radeonScreen, R600_TX_FILTER0_0, last_hw_tmu + 1);
	r600->hw.tex.filter_1.cmd[R600_TEX_CMD_0] =
	    cmdpacket0(r600->radeon.radeonScreen, R600_TX_FILTER1_0, last_hw_tmu + 1);
	r600->hw.tex.size.cmd[R600_TEX_CMD_0] =
	    cmdpacket0(r600->radeon.radeonScreen, R600_TX_SIZE_0, last_hw_tmu + 1);
	r600->hw.tex.format.cmd[R600_TEX_CMD_0] =
	    cmdpacket0(r600->radeon.radeonScreen, R600_TX_FORMAT_0, last_hw_tmu + 1);
	r600->hw.tex.pitch.cmd[R600_TEX_CMD_0] =
	    cmdpacket0(r600->radeon.radeonScreen, R600_TX_FORMAT2_0, last_hw_tmu + 1);
	r600->hw.tex.offset.cmd[R600_TEX_CMD_0] =
	    cmdpacket0(r600->radeon.radeonScreen, R600_TX_OFFSET_0, last_hw_tmu + 1);
	r600->hw.tex.chroma_key.cmd[R600_TEX_CMD_0] =
	    cmdpacket0(r600->radeon.radeonScreen, R600_TX_CHROMA_KEY_0, last_hw_tmu + 1);
	r600->hw.tex.border_color.cmd[R600_TEX_CMD_0] =
	    cmdpacket0(r600->radeon.radeonScreen, R600_TX_BORDER_COLOR_0, last_hw_tmu + 1);

	if (!fp)		/* should only happenen once, just after context is created */
		return;

	if (fp->mesa_program.UsesKill && last_hw_tmu < 0) {
		// The KILL operation requires the first texture unit
		// to be enabled.
		r600->hw.txe.cmd[R600_TXE_ENABLE] |= 1;
		r600->hw.tex.filter.cmd[R600_TEX_VALUE_0] = 0;
		r600->hw.tex.filter.cmd[R600_TEX_CMD_0] =
			cmdpacket0(r600->radeon.radeonScreen, R600_TX_FILTER0_0, 1);
	}
	r600SetupFragmentShaderTextures(ctx, tmu_mappings);

	if (RADEON_DEBUG & DEBUG_STATE)
		fprintf(stderr, "TX_ENABLE: %08x  last_hw_tmu=%d\n",
			r600->hw.txe.cmd[R600_TXE_ENABLE], last_hw_tmu);
}

union r600_outputs_written {
	GLuint vp_outputs;	/* hw_tcl_on */
	 DECLARE_RENDERINPUTS(index_bitset);	/* !hw_tcl_on */
};

#define R600_OUTPUTS_WRITTEN_TEST(ow, vp_result, tnl_attrib) \
	((hw_tcl_on) ? (ow).vp_outputs & (1 << (vp_result)) : \
	RENDERINPUTS_TEST( (ow.index_bitset), (tnl_attrib) ))

static void r600SetupRSUnit(GLcontext * ctx)
{
	r600ContextPtr r600 = R600_CONTEXT(ctx);
        TNLcontext *tnl = TNL_CONTEXT(ctx);
	struct vertex_buffer *VB = &tnl->vb;
	union r600_outputs_written OutputsWritten;
	GLuint InputsRead;
	int fp_reg, high_rr;
	int col_ip, tex_ip;
	int rs_tex_count = 0;
	int i, count, col_fmt;

	if (hw_tcl_on)
		OutputsWritten.vp_outputs = CURRENT_VERTEX_SHADER(ctx)->key.OutputsWritten;
	else
		RENDERINPUTS_COPY(OutputsWritten.index_bitset, r600->state.render_inputs_bitset);

	if (ctx->FragmentProgram._Current)
		InputsRead = ctx->FragmentProgram._Current->Base.InputsRead;
	else {
		fprintf(stderr, "No ctx->FragmentProgram._Current!!\n");
		return;		/* This should only ever happen once.. */
	}

	R600_STATECHANGE(r600, ri);
	R600_STATECHANGE(r600, rc);
	R600_STATECHANGE(r600, rr);

	fp_reg = col_ip = tex_ip = col_fmt = 0;

	r600->hw.rc.cmd[1] = 0;
	r600->hw.rc.cmd[2] = 0;
	for (i=0; i<R600_RR_CMDSIZE-1; ++i)
		r600->hw.rr.cmd[R600_RR_INST_0 + i] = 0;

	for (i=0; i<R600_RI_CMDSIZE-1; ++i)
		r600->hw.ri.cmd[R600_RI_INTERP_0 + i] = 0;


	if (InputsRead & FRAG_BIT_COL0) {
		if (R600_OUTPUTS_WRITTEN_TEST(OutputsWritten, VERT_RESULT_COL0, _TNL_ATTRIB_COLOR0)) {
			count = VB->AttribPtr[_TNL_ATTRIB_COLOR0]->size;
			if (count == 4)
			    col_fmt = R600_RS_COL_FMT_RGBA;
			else if (count == 3)
			    col_fmt = R600_RS_COL_FMT_RGB1;
			else
			    col_fmt = R600_RS_COL_FMT_0001;

			r600->hw.ri.cmd[R600_RI_INTERP_0 + col_ip] = R600_RS_COL_PTR(col_ip) | R600_RS_COL_FMT(col_fmt);
			r600->hw.rr.cmd[R600_RR_INST_0 + col_ip] = R600_RS_INST_COL_ID(col_ip) | R600_RS_INST_COL_CN_WRITE | R600_RS_INST_COL_ADDR(fp_reg);
			InputsRead &= ~FRAG_BIT_COL0;
			++col_ip;
			++fp_reg;
		} else {
			WARN_ONCE("fragprog wants col0, vp doesn't provide it\n");
		}
	}

	if (InputsRead & FRAG_BIT_COL1) {
		if (R600_OUTPUTS_WRITTEN_TEST(OutputsWritten, VERT_RESULT_COL1, _TNL_ATTRIB_COLOR1)) {
			count = VB->AttribPtr[_TNL_ATTRIB_COLOR1]->size;
			if (count == 4)
			    col_fmt = R600_RS_COL_FMT_RGBA;
			else if (count == 3)
			    col_fmt = R600_RS_COL_FMT_RGB1;
			else
			    col_fmt = R600_RS_COL_FMT_0001;

			r600->hw.ri.cmd[R600_RI_INTERP_0 + col_ip] = R600_RS_COL_PTR(col_ip) | R600_RS_COL_FMT(col_fmt);
			r600->hw.rr.cmd[R600_RR_INST_0 + col_ip] = R600_RS_INST_COL_ID(col_ip) | R600_RS_INST_COL_CN_WRITE | R600_RS_INST_COL_ADDR(fp_reg);
			InputsRead &= ~FRAG_BIT_COL1;
			++col_ip;
			++fp_reg;
		} else {
			WARN_ONCE("fragprog wants col1, vp doesn't provide it\n");
		}
	}

	for (i = 0; i < ctx->Const.MaxTextureUnits; i++) {
		if (! ( InputsRead & FRAG_BIT_TEX(i) ) )
		    continue;

		if (!R600_OUTPUTS_WRITTEN_TEST(OutputsWritten, VERT_RESULT_TEX0 + i, _TNL_ATTRIB_TEX(i))) {
		    WARN_ONCE("fragprog wants coords for tex%d, vp doesn't provide them!\n", i);
		    continue;
		}

		int swiz;

		/* with TCL we always seem to route 4 components */
		if (hw_tcl_on)
		  count = 4;
		else
		  count = VB->AttribPtr[_TNL_ATTRIB_TEX(i)]->size;

		switch(count) {
		case 4: swiz = R600_RS_SEL_S(0) | R600_RS_SEL_T(1) | R600_RS_SEL_R(2) | R600_RS_SEL_Q(3); break;
		case 3: swiz = R600_RS_SEL_S(0) | R600_RS_SEL_T(1) | R600_RS_SEL_R(2) | R600_RS_SEL_Q(R600_RS_SEL_K1); break;
		default:
		case 1:
		case 2: swiz = R600_RS_SEL_S(0) | R600_RS_SEL_T(1) | R600_RS_SEL_R(R600_RS_SEL_K0) | R600_RS_SEL_Q(R600_RS_SEL_K1); break;
		};

		r600->hw.ri.cmd[R600_RI_INTERP_0 + tex_ip] |= swiz | R600_RS_TEX_PTR(rs_tex_count);
		r600->hw.rr.cmd[R600_RR_INST_0 + tex_ip] |= R600_RS_INST_TEX_ID(tex_ip) | R600_RS_INST_TEX_CN_WRITE | R600_RS_INST_TEX_ADDR(fp_reg);
		InputsRead &= ~(FRAG_BIT_TEX0 << i);
		rs_tex_count += count;
		++tex_ip;
		++fp_reg;
	}

	if (InputsRead & FRAG_BIT_FOGC) {
		if (R600_OUTPUTS_WRITTEN_TEST(OutputsWritten, VERT_RESULT_FOGC, _TNL_ATTRIB_FOG)) {
			r600->hw.ri.cmd[R600_RI_INTERP_0 + tex_ip] |=  R600_RS_SEL_S(0) | R600_RS_SEL_T(1) | R600_RS_SEL_R(2) | R600_RS_SEL_Q(3) |  R600_RS_TEX_PTR(rs_tex_count);
			r600->hw.rr.cmd[R600_RR_INST_0 + tex_ip] |= R600_RS_INST_TEX_ID(tex_ip) | R600_RS_INST_TEX_CN_WRITE | R600_RS_INST_TEX_ADDR(fp_reg);
			InputsRead &= ~FRAG_BIT_FOGC;
			rs_tex_count += 4;
			++tex_ip;
			++fp_reg;
		} else {
			WARN_ONCE("fragprog wants fogc, vp doesn't provide it\n");
		}
	}

	if (InputsRead & FRAG_BIT_WPOS) {
		r600->hw.ri.cmd[R600_RI_INTERP_0 + tex_ip] |=  R600_RS_SEL_S(0) | R600_RS_SEL_T(1) | R600_RS_SEL_R(2) | R600_RS_SEL_Q(3) |  R600_RS_TEX_PTR(rs_tex_count);
		r600->hw.rr.cmd[R600_RR_INST_0 + tex_ip] |= R600_RS_INST_TEX_ID(tex_ip) | R600_RS_INST_TEX_CN_WRITE | R600_RS_INST_TEX_ADDR(fp_reg);
		InputsRead &= ~FRAG_BIT_WPOS;
		rs_tex_count += 4;
		++tex_ip;
		++fp_reg;
	}
	InputsRead &= ~FRAG_BIT_WPOS;

	/* Setup default color if no color or tex was set */
	if (rs_tex_count == 0 && col_ip == 0) {
		r600->hw.rr.cmd[R600_RR_INST_0] = R600_RS_INST_COL_ID(0) | R600_RS_INST_COL_CN_WRITE | R600_RS_INST_COL_ADDR(0) | R600_RS_COL_FMT(R600_RS_COL_FMT_0001);
		++col_ip;
	}

	high_rr = (col_ip > tex_ip) ? col_ip : tex_ip;
	r600->hw.rc.cmd[1] |= (rs_tex_count << R600_IT_COUNT_SHIFT)  | (col_ip << R600_IC_COUNT_SHIFT) | R600_HIRES_EN;
	r600->hw.rc.cmd[2] |= high_rr - 1;

	 r600->hw.rr.cmd[R600_RR_CMD_0] = cmdpacket0(r600->radeon.radeonScreen, R600_RS_INST_0, high_rr);

	if (InputsRead)
		WARN_ONCE("Don't know how to satisfy InputsRead=0x%08x\n", InputsRead);
}

#define bump_vpu_count(ptr, new_count)   do{\
	drm_r300_cmd_header_t* _p=((drm_r300_cmd_header_t*)(ptr));\
	int _nc=(new_count)/4; \
	assert(_nc < 256); \
	if(_nc>_p->vpu.count)_p->vpu.count=_nc;\
	}while(0)

static INLINE void r600SetupVertexProgramFragment(r600ContextPtr r600, int dest, struct r600_vertex_shader_fragment *vsf)
{
	int i;

	if (vsf->length == 0)
		return;

	if (vsf->length & 0x3) {
		fprintf(stderr, "VERTEX_SHADER_FRAGMENT must have length divisible by 4\n");
		_mesa_exit(-1);
	}

	switch ((dest >> 8) & 0xf) {
	case 0:
		R600_STATECHANGE(r600, vpi);
		for (i = 0; i < vsf->length; i++)
			r600->hw.vpi.cmd[R600_VPI_INSTR_0 + i + 4 * (dest & 0xff)] = (vsf->body.d[i]);
		bump_vpu_count(r600->hw.vpi.cmd, vsf->length + 4 * (dest & 0xff));
		break;

	case 2:
		R600_STATECHANGE(r600, vpp);
		for (i = 0; i < vsf->length; i++)
			r600->hw.vpp.cmd[R600_VPP_PARAM_0 + i + 4 * (dest & 0xff)] = (vsf->body.d[i]);
		bump_vpu_count(r600->hw.vpp.cmd, vsf->length + 4 * (dest & 0xff));
		break;
	case 4:
		R600_STATECHANGE(r600, vps);
		for (i = 0; i < vsf->length; i++)
			r600->hw.vps.cmd[1 + i + 4 * (dest & 0xff)] = (vsf->body.d[i]);
		bump_vpu_count(r600->hw.vps.cmd, vsf->length + 4 * (dest & 0xff));
		break;
	default:
		fprintf(stderr, "%s:%s don't know how to handle dest %04x\n", __FILE__, __FUNCTION__, dest);
		_mesa_exit(-1);
	}
}

#define MIN3(a, b, c)	((a) < (b) ? MIN2(a, c) : MIN2(b, c))


static void r600VapCntl(r600ContextPtr rmesa, GLuint input_count,
			GLuint output_count, GLuint temp_count)
{
    int vtx_mem_size;
    int pvs_num_slots;
    int pvs_num_cntrls;

    /* Flush PVS engine before changing PVS_NUM_SLOTS, PVS_NUM_CNTRLS.
     * See r500 docs 6.5.2 - done in emit */

    /* avoid division by zero */
    if (input_count == 0) input_count = 1;
    if (output_count == 0) output_count = 1;
    if (temp_count == 0) temp_count = 1;

    if (rmesa->radeon.radeonScreen->chip_family >= CHIP_FAMILY_RV515)
	vtx_mem_size = 128;
    else
	vtx_mem_size = 72;

    pvs_num_slots = MIN3(10, vtx_mem_size/input_count, vtx_mem_size/output_count);
    pvs_num_cntrls = MIN2(6, vtx_mem_size/temp_count);

    R600_STATECHANGE(rmesa, vap_cntl);
    if (rmesa->radeon.radeonScreen->chip_flags & RADEON_CHIPSET_TCL) {
	rmesa->hw.vap_cntl.cmd[R600_VAP_CNTL_INSTR] =
	    (pvs_num_slots << R600_PVS_NUM_SLOTS_SHIFT) |
	    (pvs_num_cntrls << R600_PVS_NUM_CNTLRS_SHIFT) |
	    (12 << R600_VF_MAX_VTX_NUM_SHIFT);
    } else
	/* not sure about non-tcl */
	rmesa->hw.vap_cntl.cmd[R600_VAP_CNTL_INSTR] = ((10 << R600_PVS_NUM_SLOTS_SHIFT) |
				    (5 << R600_PVS_NUM_CNTLRS_SHIFT) |
				    (5 << R600_VF_MAX_VTX_NUM_SHIFT));

    if (rmesa->radeon.radeonScreen->chip_family == CHIP_FAMILY_RV515)
	rmesa->hw.vap_cntl.cmd[R600_VAP_CNTL_INSTR] |= (2 << R600_PVS_NUM_FPUS_SHIFT);
    else if ((rmesa->radeon.radeonScreen->chip_family == CHIP_FAMILY_RV530) ||
	     (rmesa->radeon.radeonScreen->chip_family == CHIP_FAMILY_RV560) ||
	     (rmesa->radeon.radeonScreen->chip_family == CHIP_FAMILY_RV570))
	rmesa->hw.vap_cntl.cmd[R600_VAP_CNTL_INSTR] |= (5 << R600_PVS_NUM_FPUS_SHIFT);
    else if ((rmesa->radeon.radeonScreen->chip_family == CHIP_FAMILY_RV410) ||
	     (rmesa->radeon.radeonScreen->chip_family == CHIP_FAMILY_R420))
	rmesa->hw.vap_cntl.cmd[R600_VAP_CNTL_INSTR] |= (6 << R600_PVS_NUM_FPUS_SHIFT);
    else if ((rmesa->radeon.radeonScreen->chip_family == CHIP_FAMILY_R520) ||
	     (rmesa->radeon.radeonScreen->chip_family == CHIP_FAMILY_R580))
	rmesa->hw.vap_cntl.cmd[R600_VAP_CNTL_INSTR] |= (8 << R600_PVS_NUM_FPUS_SHIFT);
    else
	rmesa->hw.vap_cntl.cmd[R600_VAP_CNTL_INSTR] |= (4 << R600_PVS_NUM_FPUS_SHIFT);

}

static void r600SetupDefaultVertexProgram(r600ContextPtr rmesa)
{
	struct r600_vertex_shader_state *prog = &(rmesa->state.vertex_shader);
	GLuint o_reg = 0;
	GLuint i_reg = 0;
	int i;
	int inst_count = 0;
	int param_count = 0;
	int program_end = 0;

	for (i = VERT_ATTRIB_POS; i < VERT_ATTRIB_MAX; i++) {
		if (rmesa->state.sw_tcl_inputs[i] != -1) {
			prog->program.body.i[program_end + 0] = PVS_OP_DST_OPERAND(VE_MULTIPLY, GL_FALSE, GL_FALSE, o_reg++, VSF_FLAG_ALL, PVS_DST_REG_OUT);
			prog->program.body.i[program_end + 1] = PVS_SRC_OPERAND(rmesa->state.sw_tcl_inputs[i], PVS_SRC_SELECT_X, PVS_SRC_SELECT_Y, PVS_SRC_SELECT_Z, PVS_SRC_SELECT_W, PVS_SRC_REG_INPUT, VSF_FLAG_NONE);
			prog->program.body.i[program_end + 2] = PVS_SRC_OPERAND(rmesa->state.sw_tcl_inputs[i], PVS_SRC_SELECT_FORCE_1, PVS_SRC_SELECT_FORCE_1, PVS_SRC_SELECT_FORCE_1, PVS_SRC_SELECT_FORCE_1, PVS_SRC_REG_INPUT, VSF_FLAG_NONE);
			prog->program.body.i[program_end + 3] = PVS_SRC_OPERAND(rmesa->state.sw_tcl_inputs[i], PVS_SRC_SELECT_FORCE_1, PVS_SRC_SELECT_FORCE_1, PVS_SRC_SELECT_FORCE_1, PVS_SRC_SELECT_FORCE_1, PVS_SRC_REG_INPUT, VSF_FLAG_NONE);
			program_end += 4;
			i_reg++;
		}
	}

	prog->program.length = program_end;

	r600SetupVertexProgramFragment(rmesa, R600_PVS_CODE_START,
				       &(prog->program));
	inst_count = (prog->program.length / 4) - 1;

	r600VapCntl(rmesa, i_reg, o_reg, 0);

	R600_STATECHANGE(rmesa, pvs);
	rmesa->hw.pvs.cmd[R600_PVS_CNTL_1] =
	    (0 << R600_PVS_FIRST_INST_SHIFT) |
	    (inst_count << R600_PVS_XYZW_VALID_INST_SHIFT) |
	    (inst_count << R600_PVS_LAST_INST_SHIFT);
	rmesa->hw.pvs.cmd[R600_PVS_CNTL_2] =
	    (0 << R600_PVS_CONST_BASE_OFFSET_SHIFT) |
	    (param_count << R600_PVS_MAX_CONST_ADDR_SHIFT);
	rmesa->hw.pvs.cmd[R600_PVS_CNTL_3] =
	    (inst_count << R600_PVS_LAST_VTX_SRC_INST_SHIFT);
}

static int bit_count (int x)
{
    x = ((x & 0xaaaaaaaaU) >> 1) + (x & 0x55555555U);
    x = ((x & 0xccccccccU) >> 2) + (x & 0x33333333U);
    x = (x >> 16) + (x & 0xffff);
    x = ((x & 0xf0f0) >> 4) + (x & 0x0f0f);
    return (x >> 8) + (x & 0x00ff);
}

static void r600SetupRealVertexProgram(r600ContextPtr rmesa)
{
	GLcontext *ctx = rmesa->radeon.glCtx;
	struct r600_vertex_program *prog = (struct r600_vertex_program *)CURRENT_VERTEX_SHADER(ctx);
	int inst_count = 0;
	int param_count = 0;

	/* FIXME: r600SetupVertexProgramFragment */
	R600_STATECHANGE(rmesa, vpp);
	param_count =
	    r600VertexProgUpdateParams(ctx,
				       (struct r600_vertex_program_cont *)
				       ctx->VertexProgram._Current,
				       (float *)&rmesa->hw.vpp.
				       cmd[R600_VPP_PARAM_0]);
	bump_vpu_count(rmesa->hw.vpp.cmd, param_count);
	param_count /= 4;

	r600SetupVertexProgramFragment(rmesa, R600_PVS_CODE_START, &(prog->program));
	inst_count = (prog->program.length / 4) - 1;

	r600VapCntl(rmesa, bit_count(prog->key.InputsRead),
		    bit_count(prog->key.OutputsWritten), prog->num_temporaries);

	R600_STATECHANGE(rmesa, pvs);
	rmesa->hw.pvs.cmd[R600_PVS_CNTL_1] =
	  (0 << R600_PVS_FIRST_INST_SHIFT) |
	  (inst_count << R600_PVS_XYZW_VALID_INST_SHIFT) |
	  (inst_count << R600_PVS_LAST_INST_SHIFT);
	rmesa->hw.pvs.cmd[R600_PVS_CNTL_2] =
	  (0 << R600_PVS_CONST_BASE_OFFSET_SHIFT) |
	  (param_count << R600_PVS_MAX_CONST_ADDR_SHIFT);
	rmesa->hw.pvs.cmd[R600_PVS_CNTL_3] =
	  (inst_count << R600_PVS_LAST_VTX_SRC_INST_SHIFT);
}


static void r600SetupVertexProgram(r600ContextPtr rmesa)
{
	GLcontext *ctx = rmesa->radeon.glCtx;

	/* Reset state, in case we don't use something */
	((drm_r300_cmd_header_t *) rmesa->hw.vpp.cmd)->vpu.count = 0;
	((drm_r300_cmd_header_t *) rmesa->hw.vpi.cmd)->vpu.count = 0;
	((drm_r300_cmd_header_t *) rmesa->hw.vps.cmd)->vpu.count = 0;

	/* Not sure why this doesnt work...
	   0x400 area might have something to do with pixel shaders as it appears right after pfs programming.
	   0x406 is set to { 0.0, 0.0, 1.0, 0.0 } most of the time but should change with smooth points and in other rare cases. */
	//setup_vertex_shader_fragment(rmesa, 0x406, &unk4);
	if (hw_tcl_on && ((struct r600_vertex_program *)CURRENT_VERTEX_SHADER(ctx))->translated) {
		r600SetupRealVertexProgram(rmesa);
	} else {
		/* FIXME: This needs to be replaced by vertex shader generation code. */
		r600SetupDefaultVertexProgram(rmesa);
	}

}

/**
 * Enable/Disable states.
 *
 * \note Mesa already filters redundant calls to this function.
 */
static void r600Enable(GLcontext * ctx, GLenum cap, GLboolean state)
{
	r600ContextPtr rmesa = R600_CONTEXT(ctx);
	if (RADEON_DEBUG & DEBUG_STATE)
		fprintf(stderr, "%s( %s = %s )\n", __FUNCTION__,
			_mesa_lookup_enum_by_nr(cap),
			state ? "GL_TRUE" : "GL_FALSE");

	switch (cap) {
	case GL_TEXTURE_1D:
	case GL_TEXTURE_2D:
	case GL_TEXTURE_3D:
		/* empty */
		break;
	case GL_FOG:
		/* empty */
		break;
	case GL_ALPHA_TEST:
		r600SetAlphaState(ctx);
		break;
	case GL_COLOR_LOGIC_OP:
		r600SetLogicOpState(ctx);
		/* fall-through, because logic op overrides blending */
	case GL_BLEND:
		r600SetBlendState(ctx);
		break;
	case GL_CLIP_PLANE0:
	case GL_CLIP_PLANE1:
	case GL_CLIP_PLANE2:
	case GL_CLIP_PLANE3:
	case GL_CLIP_PLANE4:
	case GL_CLIP_PLANE5:
		r600SetClipPlaneState(ctx, cap, state);
		break;
	case GL_DEPTH_TEST:
		r600SetDepthState(ctx);
		break;
	case GL_STENCIL_TEST:
		r600SetStencilState(ctx, state);
		break;
	case GL_CULL_FACE:
		r600UpdateCulling(ctx);
		break;
	case GL_POLYGON_OFFSET_POINT:
	case GL_POLYGON_OFFSET_LINE:
	case GL_POLYGON_OFFSET_FILL:
		r600SetPolygonOffsetState(ctx, state);
		break;
	case GL_SCISSOR_TEST:
		radeon_firevertices(&rmesa->radeon);
		rmesa->radeon.state.scissor.enabled = state;
		radeonUpdateScissor( ctx );
		break;
	default:
		break;
	}
}

/**
 * Completely recalculates hardware state based on the Mesa state.
 */
static void r600ResetHwState(r600ContextPtr r600)
{
	GLcontext *ctx = r600->radeon.glCtx;

	if (RADEON_DEBUG & DEBUG_STATE)
		fprintf(stderr, "%s\n", __FUNCTION__);

	radeon_firevertices(&r600->radeon);

	r600ColorMask(ctx,
		      ctx->Color.ColorMask[RCOMP],
		      ctx->Color.ColorMask[GCOMP],
		      ctx->Color.ColorMask[BCOMP], ctx->Color.ColorMask[ACOMP]);

	r600Enable(ctx, GL_DEPTH_TEST, ctx->Depth.Test);
	r600DepthMask(ctx, ctx->Depth.Mask);
	r600DepthFunc(ctx, ctx->Depth.Func);

	/* stencil */
	r600Enable(ctx, GL_STENCIL_TEST, ctx->Stencil._Enabled);
	r600StencilMaskSeparate(ctx, 0, ctx->Stencil.WriteMask[0]);
	r600StencilFuncSeparate(ctx, 0, ctx->Stencil.Function[0],
				ctx->Stencil.Ref[0], ctx->Stencil.ValueMask[0]);
	r600StencilOpSeparate(ctx, 0, ctx->Stencil.FailFunc[0],
			      ctx->Stencil.ZFailFunc[0],
			      ctx->Stencil.ZPassFunc[0]);

	r600UpdateCulling(ctx);

	r600SetBlendState(ctx);
	r600SetLogicOpState(ctx);

	r600AlphaFunc(ctx, ctx->Color.AlphaFunc, ctx->Color.AlphaRef);
	r600Enable(ctx, GL_ALPHA_TEST, ctx->Color.AlphaEnabled);

	r600->hw.vte.cmd[1] = R600_VPORT_X_SCALE_ENA
	    | R600_VPORT_X_OFFSET_ENA
	    | R600_VPORT_Y_SCALE_ENA
	    | R600_VPORT_Y_OFFSET_ENA
	    | R600_VPORT_Z_SCALE_ENA
	    | R600_VPORT_Z_OFFSET_ENA | R600_VTX_W0_FMT;
	r600->hw.vte.cmd[2] = 0x00000008;

	r600->hw.vap_vf_max_vtx_indx.cmd[1] = 0x00FFFFFF;
	r600->hw.vap_vf_max_vtx_indx.cmd[2] = 0x00000000;

#ifdef MESA_LITTLE_ENDIAN
	r600->hw.vap_cntl_status.cmd[1] = R600_VC_NO_SWAP;
#else
	r600->hw.vap_cntl_status.cmd[1] = R600_VC_32BIT_SWAP;
#endif

	r600->hw.vap_psc_sgn_norm_cntl.cmd[1] = 0xAAAAAAAA;

	r600->hw.vap_clip_cntl.cmd[1] = R600_PS_UCP_MODE_DIST_COP;

	r600->hw.vap_clip.cmd[1] = r600PackFloat32(1.0); /* X */
	r600->hw.vap_clip.cmd[2] = r600PackFloat32(1.0); /* X */
	r600->hw.vap_clip.cmd[3] = r600PackFloat32(1.0); /* Y */
	r600->hw.vap_clip.cmd[4] = r600PackFloat32(1.0); /* Y */

	switch (r600->radeon.radeonScreen->chip_family) {
	case CHIP_FAMILY_R600:
		r600->hw.vap_pvs_vtx_timeout_reg.cmd[1] = R600_2288_R600;
		break;
	default:
		r600->hw.vap_pvs_vtx_timeout_reg.cmd[1] = R600_2288_RV350;
		break;
	}

	r600->hw.gb_enable.cmd[1] = R600_GB_POINT_STUFF_ENABLE
	    | R600_GB_LINE_STUFF_ENABLE
	    | R600_GB_TRIANGLE_STUFF_ENABLE;

	r600->hw.gb_misc.cmd[R600_GB_MISC_MSPOS_0] = 0x66666666;
	r600->hw.gb_misc.cmd[R600_GB_MISC_MSPOS_1] = 0x06666666;

	r600->hw.gb_misc.cmd[R600_GB_MISC_TILE_CONFIG] =
	    R600_GB_TILE_ENABLE | R600_GB_TILE_SIZE_16 /*| R600_GB_SUBPIXEL_1_16*/;
	switch (r600->radeon.radeonScreen->num_gb_pipes) {
	case 1:
	default:
		r600->hw.gb_misc.cmd[R600_GB_MISC_TILE_CONFIG] |=
		    R600_GB_TILE_PIPE_COUNT_RV300;
		break;
	case 2:
		r600->hw.gb_misc.cmd[R600_GB_MISC_TILE_CONFIG] |=
		    R600_GB_TILE_PIPE_COUNT_R600;
		break;
	case 3:
		r600->hw.gb_misc.cmd[R600_GB_MISC_TILE_CONFIG] |=
		    R600_GB_TILE_PIPE_COUNT_R420_3P;
		break;
	case 4:
		r600->hw.gb_misc.cmd[R600_GB_MISC_TILE_CONFIG] |=
		    R600_GB_TILE_PIPE_COUNT_R420;
		break;
	}

	/* XXX: Enable anti-aliasing? */
	r600->hw.gb_misc.cmd[R600_GB_MISC_AA_CONFIG] = GB_AA_CONFIG_AA_DISABLE;
	r600->hw.gb_misc.cmd[R600_GB_MISC_SELECT] = 0;

	r600->hw.ga_point_s0.cmd[1] = r600PackFloat32(0.0);
	r600->hw.ga_point_s0.cmd[2] = r600PackFloat32(0.0);
	r600->hw.ga_point_s0.cmd[3] = r600PackFloat32(1.0);
	r600->hw.ga_point_s0.cmd[4] = r600PackFloat32(1.0);

	r600->hw.ga_triangle_stipple.cmd[1] = 0x00050005;

	r600PointSize(ctx, 1.0);

	r600->hw.ga_point_minmax.cmd[1] = 0x18000006;
	r600->hw.ga_point_minmax.cmd[2] = 0x00020006;
	r600->hw.ga_point_minmax.cmd[3] = r600PackFloat32(1.0 / 192.0);

	r600LineWidth(ctx, 1.0);

	r600->hw.ga_line_stipple.cmd[1] = 0;
	r600->hw.ga_line_stipple.cmd[2] = r600PackFloat32(0.0);
	r600->hw.ga_line_stipple.cmd[3] = r600PackFloat32(1.0);

	r600ShadeModel(ctx, ctx->Light.ShadeModel);

	r600PolygonMode(ctx, GL_FRONT, ctx->Polygon.FrontMode);
	r600PolygonMode(ctx, GL_BACK, ctx->Polygon.BackMode);
	r600->hw.zbias_cntl.cmd[1] = 0x00000000;

	r600PolygonOffset(ctx, ctx->Polygon.OffsetFactor,
			  ctx->Polygon.OffsetUnits);
	r600Enable(ctx, GL_POLYGON_OFFSET_POINT, ctx->Polygon.OffsetPoint);
	r600Enable(ctx, GL_POLYGON_OFFSET_LINE, ctx->Polygon.OffsetLine);
	r600Enable(ctx, GL_POLYGON_OFFSET_FILL, ctx->Polygon.OffsetFill);

	r600->hw.su_depth_scale.cmd[1] = 0x4B7FFFFF;
	r600->hw.su_depth_scale.cmd[2] = 0x00000000;

	r600->hw.sc_hyperz.cmd[1] = 0x0000001C;
	r600->hw.sc_hyperz.cmd[2] = 0x2DA49525;

	r600->hw.sc_screendoor.cmd[1] = 0x00FFFFFF;

	r600->hw.us_out_fmt.cmd[5] = R600_W_FMT_W0 | R600_W_SRC_US;

	/* disable fog unit */
	r600->hw.fogs.cmd[R600_FOGS_STATE] = 0;
	r600->hw.fg_depth_src.cmd[1] = R600_FG_DEPTH_SRC_SCAN;

	r600->hw.rb3d_cctl.cmd[1] = 0;

	r600BlendColor(ctx, ctx->Color.BlendColor);

	r600->hw.rb3d_dither_ctl.cmd[1] = 0;
	r600->hw.rb3d_dither_ctl.cmd[2] = 0;
	r600->hw.rb3d_dither_ctl.cmd[3] = 0;
	r600->hw.rb3d_dither_ctl.cmd[4] = 0;
	r600->hw.rb3d_dither_ctl.cmd[5] = 0;
	r600->hw.rb3d_dither_ctl.cmd[6] = 0;
	r600->hw.rb3d_dither_ctl.cmd[7] = 0;
	r600->hw.rb3d_dither_ctl.cmd[8] = 0;
	r600->hw.rb3d_dither_ctl.cmd[9] = 0;

	r600->hw.rb3d_aaresolve_ctl.cmd[1] = 0;

	r600->hw.zb_depthclearvalue.cmd[1] = 0;

	r600->hw.zstencil_format.cmd[2] = R600_ZTOP_DISABLE;
	r600->hw.zstencil_format.cmd[3] = 0x00000003;
	r600->hw.zstencil_format.cmd[4] = 0x00000000;
	r600SetEarlyZState(ctx);

	r600->hw.unk4F30.cmd[1] = 0;
	r600->hw.unk4F30.cmd[2] = 0;

	r600->hw.zb_hiz_offset.cmd[1] = 0;

	r600->hw.zb_hiz_pitch.cmd[1] = 0;

	r600VapCntl(r600, 0, 0, 0);

	r600->hw.vps.cmd[R600_VPS_ZERO_0] = 0;
	r600->hw.vps.cmd[R600_VPS_ZERO_1] = 0;
	r600->hw.vps.cmd[R600_VPS_POINTSIZE] = r600PackFloat32(1.0);
	r600->hw.vps.cmd[R600_VPS_ZERO_3] = 0;

	r600->radeon.hw.all_dirty = GL_TRUE;
}

void r600UpdateShaders(r600ContextPtr rmesa)
{
	GLcontext *ctx;
	struct r600_vertex_program *vp;
	int i;

	ctx = rmesa->radeon.glCtx;

	if (rmesa->radeon.NewGLState && hw_tcl_on) {
		rmesa->radeon.NewGLState = 0;

		for (i = _TNL_FIRST_MAT; i <= _TNL_LAST_MAT; i++) {
			rmesa->temp_attrib[i] =
			    TNL_CONTEXT(ctx)->vb.AttribPtr[i];
			TNL_CONTEXT(ctx)->vb.AttribPtr[i] =
			    &rmesa->dummy_attrib[i];
		}

		_tnl_UpdateFixedFunctionProgram(ctx);

		for (i = _TNL_FIRST_MAT; i <= _TNL_LAST_MAT; i++) {
			TNL_CONTEXT(ctx)->vb.AttribPtr[i] =
			    rmesa->temp_attrib[i];
		}

		r600SelectVertexShader(rmesa);
		vp = (struct r600_vertex_program *)
		    CURRENT_VERTEX_SHADER(ctx);
		/*if (vp->translated == GL_FALSE)
		   r600TranslateVertexShader(vp); */
		if (vp->translated == GL_FALSE) {
			fprintf(stderr, "Failing back to sw-tcl\n");
			hw_tcl_on = future_hw_tcl_on = 0;
			r600ResetHwState(rmesa);

			r600UpdateStateParameters(ctx, _NEW_PROGRAM);
			return;
		}
	}
	r600UpdateStateParameters(ctx, _NEW_PROGRAM);
}

static const GLfloat *get_fragmentprogram_constant(GLcontext *ctx,
	struct gl_program *program, struct prog_src_register srcreg)
{
	static const GLfloat dummy[4] = { 0, 0, 0, 0 };

	switch(srcreg.File) {
	case PROGRAM_LOCAL_PARAM:
		return program->LocalParams[srcreg.Index];
	case PROGRAM_ENV_PARAM:
		return ctx->FragmentProgram.Parameters[srcreg.Index];
	case PROGRAM_STATE_VAR:
	case PROGRAM_NAMED_PARAM:
	case PROGRAM_CONSTANT:
		return program->Parameters->ParameterValues[srcreg.Index];
	default:
		_mesa_problem(ctx, "get_fragmentprogram_constant: Unknown\n");
		return dummy;
	}
}


static void r600SetupPixelShader(r600ContextPtr rmesa)
{
	GLcontext *ctx = rmesa->radeon.glCtx;
	struct r600_fragment_program *fp = (struct r600_fragment_program *)
	    (char *)ctx->FragmentProgram._Current;
	struct r600_fragment_program_code *code;
	int i, k;

	if (!fp)		/* should only happenen once, just after context is created */
		return;

	r600TranslateFragmentShader(rmesa, fp);
	if (!fp->translated) {
		fprintf(stderr, "%s: No valid fragment shader, exiting\n",
			__FUNCTION__);
		return;
	}
	code = &fp->code;

	r600SetupTextures(ctx);

	R600_STATECHANGE(rmesa, fpi[0]);
	R600_STATECHANGE(rmesa, fpi[1]);
	R600_STATECHANGE(rmesa, fpi[2]);
	R600_STATECHANGE(rmesa, fpi[3]);
	rmesa->hw.fpi[0].cmd[R600_FPI_CMD_0] = cmdpacket0(rmesa->radeon.radeonScreen, R600_US_ALU_RGB_INST_0, code->alu.length);
	rmesa->hw.fpi[1].cmd[R600_FPI_CMD_0] = cmdpacket0(rmesa->radeon.radeonScreen, R600_US_ALU_RGB_ADDR_0, code->alu.length);
	rmesa->hw.fpi[2].cmd[R600_FPI_CMD_0] = cmdpacket0(rmesa->radeon.radeonScreen, R600_US_ALU_ALPHA_INST_0, code->alu.length);
	rmesa->hw.fpi[3].cmd[R600_FPI_CMD_0] = cmdpacket0(rmesa->radeon.radeonScreen, R600_US_ALU_ALPHA_ADDR_0, code->alu.length);
	for (i = 0; i < code->alu.length; i++) {
		rmesa->hw.fpi[0].cmd[R600_FPI_INSTR_0 + i] = code->alu.inst[i].inst0;
		rmesa->hw.fpi[1].cmd[R600_FPI_INSTR_0 + i] = code->alu.inst[i].inst1;
		rmesa->hw.fpi[2].cmd[R600_FPI_INSTR_0 + i] = code->alu.inst[i].inst2;
		rmesa->hw.fpi[3].cmd[R600_FPI_INSTR_0 + i] = code->alu.inst[i].inst3;
	}

	R600_STATECHANGE(rmesa, fp);
	rmesa->hw.fp.cmd[R600_FP_CNTL0] = code->cur_node | (code->first_node_has_tex << 3);
	rmesa->hw.fp.cmd[R600_FP_CNTL1] = code->max_temp_idx;
	rmesa->hw.fp.cmd[R600_FP_CNTL2] =
	  (0 << R600_PFS_CNTL_ALU_OFFSET_SHIFT) |
	  ((code->alu.length-1) << R600_PFS_CNTL_ALU_END_SHIFT) |
	  (0 << R600_PFS_CNTL_TEX_OFFSET_SHIFT) |
	  ((code->tex.length ? code->tex.length-1 : 0) << R600_PFS_CNTL_TEX_END_SHIFT);
	/* I just want to say, the way these nodes are stored.. weird.. */
	for (i = 0, k = (4 - (code->cur_node + 1)); i < 4; i++, k++) {
		if (i < (code->cur_node + 1)) {
			rmesa->hw.fp.cmd[R600_FP_NODE0 + k] =
			  (code->node[i].alu_offset << R600_ALU_START_SHIFT) |
			  (code->node[i].alu_end << R600_ALU_SIZE_SHIFT) |
			  (code->node[i].tex_offset << R600_TEX_START_SHIFT) |
			  (code->node[i].tex_end << R600_TEX_SIZE_SHIFT) |
			  code->node[i].flags;
		} else {
			rmesa->hw.fp.cmd[R600_FP_NODE0 + (3 - i)] = 0;
		}
	}

	R600_STATECHANGE(rmesa, fpp);
	rmesa->hw.fpp.cmd[R600_FPP_CMD_0] = cmdpacket0(rmesa->radeon.radeonScreen, R600_PFS_PARAM_0_X, code->const_nr * 4);
	for (i = 0; i < code->const_nr; i++) {
		const GLfloat *constant = get_fragmentprogram_constant(ctx,
			&fp->mesa_program.Base, code->constant[i]);
		rmesa->hw.fpp.cmd[R600_FPP_PARAM_0 + 4 * i + 0] = r600PackFloat24(constant[0]);
		rmesa->hw.fpp.cmd[R600_FPP_PARAM_0 + 4 * i + 1] = r600PackFloat24(constant[1]);
		rmesa->hw.fpp.cmd[R600_FPP_PARAM_0 + 4 * i + 2] = r600PackFloat24(constant[2]);
		rmesa->hw.fpp.cmd[R600_FPP_PARAM_0 + 4 * i + 3] = r600PackFloat24(constant[3]);
	}
}

void r600UpdateShaderStates(r600ContextPtr rmesa)
{
	GLcontext *ctx;
	ctx = rmesa->radeon.glCtx;

	r600SetEarlyZState(ctx);

	/* w_fmt value is set to get best performance
	 * see p.130 R5xx 3D acceleration guide v1.3 */
	GLuint w_fmt, fgdepthsrc;
	if (current_fragment_program_writes_depth(ctx)) {
		fgdepthsrc = R600_FG_DEPTH_SRC_SHADER;
		w_fmt = R600_W_FMT_W24 | R600_W_SRC_US;
	} else {
		fgdepthsrc = R600_FG_DEPTH_SRC_SCAN;
		w_fmt = R600_W_FMT_W0 | R600_W_SRC_US;
	}

	if (w_fmt != rmesa->hw.us_out_fmt.cmd[5]) {
		R600_STATECHANGE(rmesa, us_out_fmt);
		rmesa->hw.us_out_fmt.cmd[5] = w_fmt;
	}

	if (fgdepthsrc != rmesa->hw.fg_depth_src.cmd[1]) {
		R600_STATECHANGE(rmesa, fg_depth_src);
		rmesa->hw.fg_depth_src.cmd[1] = fgdepthsrc;
	}

	r600SetupPixelShader(rmesa);

	r600SetupRSUnit(ctx);

	r600SetupVertexProgram(rmesa);

}

/**
 * Called by Mesa after an internal state update.
 */
static void r600InvalidateState(GLcontext * ctx, GLuint new_state)
{
	r600ContextPtr r600 = R600_CONTEXT(ctx);

	_swrast_InvalidateState(ctx, new_state);
	_swsetup_InvalidateState(ctx, new_state);
	_vbo_InvalidateState(ctx, new_state);
	_tnl_InvalidateState(ctx, new_state);
	_ae_invalidate_state(ctx, new_state);

	if (new_state & (_NEW_BUFFERS | _NEW_COLOR | _NEW_PIXEL)) {
		_mesa_update_framebuffer(ctx);
		/* this updates the DrawBuffer's Width/Height if it's a FBO */
		_mesa_update_draw_buffer_bounds(ctx);

		R600_STATECHANGE(r600, cb);
	}

	r600UpdateStateParameters(ctx, new_state);

	r600->radeon.NewGLState |= new_state;
}

/**
 * Calculate initial hardware state and register state functions.
 * Assumes that the command buffer and state atoms have been
 * initialized already.
 */
void r600InitState(r600ContextPtr r600)
{
	memset(&(r600->state.texture), 0, sizeof(r600->state.texture));

	r600ResetHwState(r600);
}

static void r600RenderMode(GLcontext * ctx, GLenum mode)
{
	r600ContextPtr rmesa = R600_CONTEXT(ctx);
	(void)rmesa;
	(void)mode;
}

void r600UpdateClipPlanes( GLcontext *ctx )
{
	r600ContextPtr rmesa = R600_CONTEXT(ctx);
	GLuint p;

	for (p = 0; p < ctx->Const.MaxClipPlanes; p++) {
		if (ctx->Transform.ClipPlanesEnabled & (1 << p)) {
			GLint *ip = (GLint *)ctx->Transform._ClipUserPlane[p];

			R600_STATECHANGE( rmesa, vpucp[p] );
			rmesa->hw.vpucp[p].cmd[R600_VPUCP_X] = ip[0];
			rmesa->hw.vpucp[p].cmd[R600_VPUCP_Y] = ip[1];
			rmesa->hw.vpucp[p].cmd[R600_VPUCP_Z] = ip[2];
			rmesa->hw.vpucp[p].cmd[R600_VPUCP_W] = ip[3];
		}
	}
}

/**
 * Initialize driver's state callback functions
 */
void r600InitStateFuncs(struct dd_function_table *functions)
{

	functions->UpdateState = r600InvalidateState;
	functions->AlphaFunc = r600AlphaFunc;
	functions->BlendColor = r600BlendColor;
	functions->BlendEquationSeparate = r600BlendEquationSeparate;
	functions->BlendFuncSeparate = r600BlendFuncSeparate;
	functions->Enable = r600Enable;
	functions->ColorMask = r600ColorMask;
	functions->DepthFunc = r600DepthFunc;
	functions->DepthMask = r600DepthMask;
	functions->CullFace = r600CullFace;
	functions->FrontFace = r600FrontFace;
	functions->ShadeModel = r600ShadeModel;
	functions->LogicOpcode = r600LogicOpcode;

	/* ARB_point_parameters */
	functions->PointParameterfv = r600PointParameter;

	/* Stencil related */
	functions->StencilFuncSeparate = r600StencilFuncSeparate;
	functions->StencilMaskSeparate = r600StencilMaskSeparate;
	functions->StencilOpSeparate = r600StencilOpSeparate;

	/* Viewport related */
	functions->Viewport = r600Viewport;
	functions->DepthRange = r600DepthRange;
	functions->PointSize = r600PointSize;
	functions->LineWidth = r600LineWidth;

	functions->PolygonOffset = r600PolygonOffset;
	functions->PolygonMode = r600PolygonMode;

	functions->RenderMode = r600RenderMode;

	functions->ClipPlane = r600ClipPlane;
	functions->Scissor = radeonScissor;

	functions->DrawBuffer		= radeonDrawBuffer;
	functions->ReadBuffer		= radeonReadBuffer;
}

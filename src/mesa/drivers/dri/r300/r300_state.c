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

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "shader/prog_parameter.h"
#include "shader/prog_statevars.h"
#include "vbo/vbo.h"
#include "tnl/tnl.h"
#include "tnl/t_vp_build.h"

#include "r300_context.h"
#include "r300_state.h"
#include "r300_reg.h"
#include "r300_emit.h"
#include "r300_fragprog_common.h"
#include "r300_render.h"
#include "r300_vertprog.h"

static void r300BlendColor(GLcontext * ctx, const GLfloat cf[4])
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);

	R300_STATECHANGE(rmesa, blend_color);

	if (rmesa->radeon.radeonScreen->chip_family >= CHIP_FAMILY_RV515) {
		GLuint r = IROUND(cf[0]*1023.0f);
		GLuint g = IROUND(cf[1]*1023.0f);
		GLuint b = IROUND(cf[2]*1023.0f);
		GLuint a = IROUND(cf[3]*1023.0f);

		rmesa->hw.blend_color.cmd[1] = r | (a << 16);
		rmesa->hw.blend_color.cmd[2] = b | (g << 16);
	} else {
		GLubyte color[4];
		CLAMPED_FLOAT_TO_UBYTE(color[0], cf[0]);
		CLAMPED_FLOAT_TO_UBYTE(color[1], cf[1]);
		CLAMPED_FLOAT_TO_UBYTE(color[2], cf[2]);
		CLAMPED_FLOAT_TO_UBYTE(color[3], cf[3]);

		rmesa->hw.blend_color.cmd[1] = PACK_COLOR_8888(color[3], color[0],
							color[1], color[2]);
	}
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
		return R300_BLEND_GL_ZERO;
		break;
	case GL_ONE:
		return R300_BLEND_GL_ONE;
		break;
	case GL_DST_COLOR:
		return R300_BLEND_GL_DST_COLOR;
		break;
	case GL_ONE_MINUS_DST_COLOR:
		return R300_BLEND_GL_ONE_MINUS_DST_COLOR;
		break;
	case GL_SRC_COLOR:
		return R300_BLEND_GL_SRC_COLOR;
		break;
	case GL_ONE_MINUS_SRC_COLOR:
		return R300_BLEND_GL_ONE_MINUS_SRC_COLOR;
		break;
	case GL_SRC_ALPHA:
		return R300_BLEND_GL_SRC_ALPHA;
		break;
	case GL_ONE_MINUS_SRC_ALPHA:
		return R300_BLEND_GL_ONE_MINUS_SRC_ALPHA;
		break;
	case GL_DST_ALPHA:
		return R300_BLEND_GL_DST_ALPHA;
		break;
	case GL_ONE_MINUS_DST_ALPHA:
		return R300_BLEND_GL_ONE_MINUS_DST_ALPHA;
		break;
	case GL_SRC_ALPHA_SATURATE:
		return (is_src) ? R300_BLEND_GL_SRC_ALPHA_SATURATE :
		    R300_BLEND_GL_ZERO;
		break;
	case GL_CONSTANT_COLOR:
		return R300_BLEND_GL_CONST_COLOR;
		break;
	case GL_ONE_MINUS_CONSTANT_COLOR:
		return R300_BLEND_GL_ONE_MINUS_CONST_COLOR;
		break;
	case GL_CONSTANT_ALPHA:
		return R300_BLEND_GL_CONST_ALPHA;
		break;
	case GL_ONE_MINUS_CONSTANT_ALPHA:
		return R300_BLEND_GL_ONE_MINUS_CONST_ALPHA;
		break;
	default:
		fprintf(stderr, "unknown blend factor %x\n", factor);
		return (is_src) ? R300_BLEND_GL_ONE : R300_BLEND_GL_ZERO;
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
static void r300SetBlendCntl(r300ContextPtr r300, int func, int eqn,
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
		new_cblend |= R300_DISCARD_SRC_PIXELS_SRC_ALPHA_0;
	}
#endif
	new_cblend |= cbits;

	if ((new_ablend != r300->hw.bld.cmd[R300_BLD_ABLEND]) ||
	    (new_cblend != r300->hw.bld.cmd[R300_BLD_CBLEND])) {
		R300_STATECHANGE(r300, bld);
		r300->hw.bld.cmd[R300_BLD_ABLEND] = new_ablend;
		r300->hw.bld.cmd[R300_BLD_CBLEND] = new_cblend;
	}
}

static void r300SetBlendState(GLcontext * ctx)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	int func = (R300_BLEND_GL_ONE << R300_SRC_BLEND_SHIFT) |
	    (R300_BLEND_GL_ZERO << R300_DST_BLEND_SHIFT);
	int eqn = R300_COMB_FCN_ADD_CLAMP;
	int funcA = (R300_BLEND_GL_ONE << R300_SRC_BLEND_SHIFT) |
	    (R300_BLEND_GL_ZERO << R300_DST_BLEND_SHIFT);
	int eqnA = R300_COMB_FCN_ADD_CLAMP;

	if (RGBA_LOGICOP_ENABLED(ctx) || !ctx->Color.BlendEnabled) {
		r300SetBlendCntl(r300, func, eqn, 0, func, eqn);
		return;
	}

	func =
	    (blend_factor(ctx->Color.BlendSrcRGB, GL_TRUE) <<
	     R300_SRC_BLEND_SHIFT) | (blend_factor(ctx->Color.BlendDstRGB,
						   GL_FALSE) <<
				      R300_DST_BLEND_SHIFT);

	switch (ctx->Color.BlendEquationRGB) {
	case GL_FUNC_ADD:
		eqn = R300_COMB_FCN_ADD_CLAMP;
		break;

	case GL_FUNC_SUBTRACT:
		eqn = R300_COMB_FCN_SUB_CLAMP;
		break;

	case GL_FUNC_REVERSE_SUBTRACT:
		eqn = R300_COMB_FCN_RSUB_CLAMP;
		break;

	case GL_MIN:
		eqn = R300_COMB_FCN_MIN;
		func = (R300_BLEND_GL_ONE << R300_SRC_BLEND_SHIFT) |
		    (R300_BLEND_GL_ONE << R300_DST_BLEND_SHIFT);
		break;

	case GL_MAX:
		eqn = R300_COMB_FCN_MAX;
		func = (R300_BLEND_GL_ONE << R300_SRC_BLEND_SHIFT) |
		    (R300_BLEND_GL_ONE << R300_DST_BLEND_SHIFT);
		break;

	default:
		fprintf(stderr,
			"[%s:%u] Invalid RGB blend equation (0x%04x).\n",
			__FUNCTION__, __LINE__, ctx->Color.BlendEquationRGB);
		return;
	}

	funcA =
	    (blend_factor(ctx->Color.BlendSrcA, GL_TRUE) <<
	     R300_SRC_BLEND_SHIFT) | (blend_factor(ctx->Color.BlendDstA,
						   GL_FALSE) <<
				      R300_DST_BLEND_SHIFT);

	switch (ctx->Color.BlendEquationA) {
	case GL_FUNC_ADD:
		eqnA = R300_COMB_FCN_ADD_CLAMP;
		break;

	case GL_FUNC_SUBTRACT:
		eqnA = R300_COMB_FCN_SUB_CLAMP;
		break;

	case GL_FUNC_REVERSE_SUBTRACT:
		eqnA = R300_COMB_FCN_RSUB_CLAMP;
		break;

	case GL_MIN:
		eqnA = R300_COMB_FCN_MIN;
		funcA = (R300_BLEND_GL_ONE << R300_SRC_BLEND_SHIFT) |
		    (R300_BLEND_GL_ONE << R300_DST_BLEND_SHIFT);
		break;

	case GL_MAX:
		eqnA = R300_COMB_FCN_MAX;
		funcA = (R300_BLEND_GL_ONE << R300_SRC_BLEND_SHIFT) |
		    (R300_BLEND_GL_ONE << R300_DST_BLEND_SHIFT);
		break;

	default:
		fprintf(stderr,
			"[%s:%u] Invalid A blend equation (0x%04x).\n",
			__FUNCTION__, __LINE__, ctx->Color.BlendEquationA);
		return;
	}

	r300SetBlendCntl(r300,
			 func, eqn,
			 (R300_SEPARATE_ALPHA_ENABLE |
			  R300_READ_ENABLE |
			  R300_ALPHA_BLEND_ENABLE), funcA, eqnA);
}

static void r300BlendEquationSeparate(GLcontext * ctx,
				      GLenum modeRGB, GLenum modeA)
{
	r300SetBlendState(ctx);
}

static void r300BlendFuncSeparate(GLcontext * ctx,
				  GLenum sfactorRGB, GLenum dfactorRGB,
				  GLenum sfactorA, GLenum dfactorA)
{
	r300SetBlendState(ctx);
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
	return bits << R300_RB3D_ROPCNTL_ROP_SHIFT;
}

/**
 * Used internally to update the r300->hw hardware state to match the
 * current OpenGL state.
 */
static void r300SetLogicOpState(GLcontext *ctx)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	R300_STATECHANGE(r300, rop);
	if (RGBA_LOGICOP_ENABLED(ctx)) {
		r300->hw.rop.cmd[1] = R300_RB3D_ROPCNTL_ROP_ENABLE |
			translate_logicop(ctx->Color.LogicOp);
	} else {
		r300->hw.rop.cmd[1] = 0;
	}
}

/**
 * Called by Mesa when an application program changes the LogicOp state
 * via glLogicOp.
 */
static void r300LogicOpcode(GLcontext *ctx, GLenum logicop)
{
	if (RGBA_LOGICOP_ENABLED(ctx))
		r300SetLogicOpState(ctx);
}

static void r300ClipPlane( GLcontext *ctx, GLenum plane, const GLfloat *eq )
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	GLint p;
	GLint *ip;

	/* no VAP UCP on non-TCL chipsets */
	if (!rmesa->options.hw_tcl_enabled)
			return;

	p = (GLint) plane - (GLint) GL_CLIP_PLANE0;
	ip = (GLint *)ctx->Transform._ClipUserPlane[p];

	R300_STATECHANGE( rmesa, vpucp[p] );
	rmesa->hw.vpucp[p].cmd[R300_VPUCP_X] = ip[0];
	rmesa->hw.vpucp[p].cmd[R300_VPUCP_Y] = ip[1];
	rmesa->hw.vpucp[p].cmd[R300_VPUCP_Z] = ip[2];
	rmesa->hw.vpucp[p].cmd[R300_VPUCP_W] = ip[3];
}

static void r300SetClipPlaneState(GLcontext * ctx, GLenum cap, GLboolean state)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	GLuint p;

	/* no VAP UCP on non-TCL chipsets */
	if (!r300->options.hw_tcl_enabled)
		return;

	p = cap - GL_CLIP_PLANE0;
	R300_STATECHANGE(r300, vap_clip_cntl);
	if (state) {
		r300->hw.vap_clip_cntl.cmd[1] |= (R300_VAP_UCP_ENABLE_0 << p);
		r300ClipPlane(ctx, cap, NULL);
	} else {
		r300->hw.vap_clip_cntl.cmd[1] &= ~(R300_VAP_UCP_ENABLE_0 << p);
	}
}

/**
 * Update our tracked culling state based on Mesa's state.
 */
static void r300UpdateCulling(GLcontext * ctx)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	uint32_t val = 0;

	if (ctx->Polygon.CullFlag) {
		switch (ctx->Polygon.CullFaceMode) {
		case GL_FRONT:
			val = R300_CULL_FRONT;
			break;
		case GL_BACK:
			val = R300_CULL_BACK;
			break;
		case GL_FRONT_AND_BACK:
			val = R300_CULL_FRONT | R300_CULL_BACK;
			break;
		default:
			break;
		}
	}

	switch (ctx->Polygon.FrontFace) {
	case GL_CW:
		val |= R300_FRONT_FACE_CW;
		break;
	case GL_CCW:
		val |= R300_FRONT_FACE_CCW;
		break;
	default:
		break;
	}

	/* Winding is inverted when rendering to FBO */
	if (ctx->DrawBuffer && ctx->DrawBuffer->Name)
		val ^= R300_FRONT_FACE_CW;

	R300_STATECHANGE(r300, cul);
	r300->hw.cul.cmd[R300_CUL_CULL] = val;
}

static void r300SetPolygonOffsetState(GLcontext * ctx, GLboolean state)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);

	R300_STATECHANGE(r300, occlusion_cntl);
	if (state) {
		r300->hw.occlusion_cntl.cmd[1] |= (3 << 0);
	} else {
		r300->hw.occlusion_cntl.cmd[1] &= ~(3 << 0);
	}
}

static GLboolean current_fragment_program_writes_depth(GLcontext* ctx)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);

	return ctx->FragmentProgram._Current && r300->selected_fp->code.writes_depth;
}

static void r300SetEarlyZState(GLcontext * ctx)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	GLuint topZ = R300_ZTOP_ENABLE;
	GLuint w_fmt, fgdepthsrc;

	if (ctx->Color.AlphaEnabled && ctx->Color.AlphaFunc != GL_ALWAYS)
		topZ = R300_ZTOP_DISABLE;
	else if (current_fragment_program_writes_depth(ctx))
		topZ = R300_ZTOP_DISABLE;
	else if (ctx->FragmentProgram._Current && ctx->FragmentProgram._Current->UsesKill)
		topZ = R300_ZTOP_DISABLE;
	else if (r300->radeon.query.current)
		topZ = R300_ZTOP_DISABLE;

	if (topZ != r300->hw.zstencil_format.cmd[2]) {
		/* Note: This completely reemits the stencil format.
		 * I have not tested whether this is strictly necessary,
		 * or if emitting a write to ZB_ZTOP is enough.
		 */
		R300_STATECHANGE(r300, zstencil_format);
		r300->hw.zstencil_format.cmd[2] = topZ;
	}

	/* w_fmt value is set to get best performance
	* see p.130 R5xx 3D acceleration guide v1.3 */
	if (current_fragment_program_writes_depth(ctx)) {
		fgdepthsrc = R300_FG_DEPTH_SRC_SHADER;
		w_fmt = R300_W_FMT_W24 | R300_W_SRC_US;
	} else {
		fgdepthsrc = R300_FG_DEPTH_SRC_SCAN;
		w_fmt = R300_W_FMT_W0 | R300_W_SRC_US;
	}

	if (w_fmt != r300->hw.us_out_fmt.cmd[5]) {
		R300_STATECHANGE(r300, us_out_fmt);
		r300->hw.us_out_fmt.cmd[5] = w_fmt;
	}

	if (fgdepthsrc != r300->hw.fg_depth_src.cmd[1]) {
		R300_STATECHANGE(r300, fg_depth_src);
		r300->hw.fg_depth_src.cmd[1] = fgdepthsrc;
	}
}

static void r300SetAlphaState(GLcontext * ctx)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	GLubyte refByte;
	uint32_t pp_misc = 0x0;
	GLboolean really_enabled = ctx->Color.AlphaEnabled;

	CLAMPED_FLOAT_TO_UBYTE(refByte, ctx->Color.AlphaRef);

	switch (ctx->Color.AlphaFunc) {
	case GL_NEVER:
		pp_misc |= R300_FG_ALPHA_FUNC_NEVER;
		break;
	case GL_LESS:
		pp_misc |= R300_FG_ALPHA_FUNC_LESS;
		break;
	case GL_EQUAL:
		pp_misc |= R300_FG_ALPHA_FUNC_EQUAL;
		break;
	case GL_LEQUAL:
		pp_misc |= R300_FG_ALPHA_FUNC_LE;
		break;
	case GL_GREATER:
		pp_misc |= R300_FG_ALPHA_FUNC_GREATER;
		break;
	case GL_NOTEQUAL:
		pp_misc |= R300_FG_ALPHA_FUNC_NOTEQUAL;
		break;
	case GL_GEQUAL:
		pp_misc |= R300_FG_ALPHA_FUNC_GE;
		break;
	case GL_ALWAYS:
		/*pp_misc |= FG_ALPHA_FUNC_ALWAYS; */
		really_enabled = GL_FALSE;
		break;
	}

	if (really_enabled) {
		pp_misc |= R300_FG_ALPHA_FUNC_ENABLE;
		pp_misc |= R500_FG_ALPHA_FUNC_8BIT;
		pp_misc |= (refByte & R300_FG_ALPHA_FUNC_VAL_MASK);
	} else {
		pp_misc = 0x0;
	}

	R300_STATECHANGE(r300, at);
	r300->hw.at.cmd[R300_AT_ALPHA_TEST] = pp_misc;
	r300->hw.at.cmd[R300_AT_UNKNOWN] = 0;
}

static void r300AlphaFunc(GLcontext * ctx, GLenum func, GLfloat ref)
{
	(void)func;
	(void)ref;
	r300SetAlphaState(ctx);
}

static int translate_func(int func)
{
	switch (func) {
	case GL_NEVER:
		return R300_ZS_NEVER;
	case GL_LESS:
		return R300_ZS_LESS;
	case GL_EQUAL:
		return R300_ZS_EQUAL;
	case GL_LEQUAL:
		return R300_ZS_LEQUAL;
	case GL_GREATER:
		return R300_ZS_GREATER;
	case GL_NOTEQUAL:
		return R300_ZS_NOTEQUAL;
	case GL_GEQUAL:
		return R300_ZS_GEQUAL;
	case GL_ALWAYS:
		return R300_ZS_ALWAYS;
	}
	return 0;
}

static void r300SetDepthState(GLcontext * ctx)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);

	R300_STATECHANGE(r300, zs);
	r300->hw.zs.cmd[R300_ZS_CNTL_0] &= (R300_STENCIL_ENABLE |
					    R300_STENCIL_FRONT_BACK |
					    R500_STENCIL_REFMASK_FRONT_BACK);
	r300->hw.zs.cmd[R300_ZS_CNTL_1] &= ~(R300_ZS_MASK << R300_Z_FUNC_SHIFT);

	if (ctx->Depth.Test) {
		r300->hw.zs.cmd[R300_ZS_CNTL_0] |= R300_Z_ENABLE;
		if (ctx->Depth.Mask)
			r300->hw.zs.cmd[R300_ZS_CNTL_0] |= R300_Z_WRITE_ENABLE;
		r300->hw.zs.cmd[R300_ZS_CNTL_1] |=
		    translate_func(ctx->Depth.Func) << R300_Z_FUNC_SHIFT;
	}
}

static void r300CatchStencilFallback(GLcontext *ctx)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	const unsigned back = ctx->Stencil._BackFace;

	if (rmesa->radeon.radeonScreen->kernel_mm &&
	    (rmesa->radeon.radeonScreen->chip_family >= CHIP_FAMILY_RV515)) {
		r300SwitchFallback(ctx, R300_FALLBACK_STENCIL_TWOSIDE, GL_FALSE);
	} else if (ctx->Stencil._Enabled &&
		   (ctx->Stencil.Ref[0] != ctx->Stencil.Ref[back]
		    || ctx->Stencil.ValueMask[0] != ctx->Stencil.ValueMask[back]
		    || ctx->Stencil.WriteMask[0] != ctx->Stencil.WriteMask[back])) {
		r300SwitchFallback(ctx, R300_FALLBACK_STENCIL_TWOSIDE, GL_TRUE);
	} else {
		r300SwitchFallback(ctx, R300_FALLBACK_STENCIL_TWOSIDE, GL_FALSE);
	}
}

static void r300SetStencilState(GLcontext * ctx, GLboolean state)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	GLboolean hw_stencil = GL_FALSE;

	r300CatchStencilFallback(ctx);

	if (ctx->DrawBuffer) {
		struct radeon_renderbuffer *rrbStencil
			= radeon_get_renderbuffer(ctx->DrawBuffer, BUFFER_STENCIL);
		hw_stencil = (rrbStencil && rrbStencil->bo);
	}

	if (hw_stencil) {
		R300_STATECHANGE(r300, zs);
		if (state) {
			r300->hw.zs.cmd[R300_ZS_CNTL_0] |=
			    R300_STENCIL_ENABLE;
		} else {
			r300->hw.zs.cmd[R300_ZS_CNTL_0] &=
			    ~R300_STENCIL_ENABLE;
		}
	}
}

static void r300UpdatePolygonMode(GLcontext * ctx)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	uint32_t hw_mode = R300_GA_POLY_MODE_DISABLE;

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
		hw_mode |= R300_GA_POLY_MODE_DUAL;

		switch (f) {
		case GL_LINE:
			hw_mode |= R300_GA_POLY_MODE_FRONT_PTYPE_LINE;
			break;
		case GL_POINT:
			hw_mode |= R300_GA_POLY_MODE_FRONT_PTYPE_POINT;
			break;
		case GL_FILL:
			hw_mode |= R300_GA_POLY_MODE_FRONT_PTYPE_TRI;
			break;
		}

		switch (b) {
		case GL_LINE:
			hw_mode |= R300_GA_POLY_MODE_BACK_PTYPE_LINE;
			break;
		case GL_POINT:
			hw_mode |= R300_GA_POLY_MODE_BACK_PTYPE_POINT;
			break;
		case GL_FILL:
			hw_mode |= R300_GA_POLY_MODE_BACK_PTYPE_TRI;
			break;
		}
	}

	if (r300->hw.polygon_mode.cmd[1] != hw_mode) {
		R300_STATECHANGE(r300, polygon_mode);
		r300->hw.polygon_mode.cmd[1] = hw_mode;
	}

	r300->hw.polygon_mode.cmd[2] = 0x00000001;
	r300->hw.polygon_mode.cmd[3] = 0x00000000;
}

/**
 * Change the culling mode.
 *
 * \note Mesa already filters redundant calls to this function.
 */
static void r300CullFace(GLcontext * ctx, GLenum mode)
{
	(void)mode;

	r300UpdateCulling(ctx);
}

/**
 * Change the polygon orientation.
 *
 * \note Mesa already filters redundant calls to this function.
 */
static void r300FrontFace(GLcontext * ctx, GLenum mode)
{
	(void)mode;

	r300UpdateCulling(ctx);
	r300UpdatePolygonMode(ctx);
}

/**
 * Change the depth testing function.
 *
 * \note Mesa already filters redundant calls to this function.
 */
static void r300DepthFunc(GLcontext * ctx, GLenum func)
{
	(void)func;
	r300SetDepthState(ctx);
}

/**
 * Enable/Disable depth writing.
 *
 * \note Mesa already filters redundant calls to this function.
 */
static void r300DepthMask(GLcontext * ctx, GLboolean mask)
{
	(void)mask;
	r300SetDepthState(ctx);
}

/**
 * Handle glColorMask()
 */
static void r300ColorMask(GLcontext * ctx,
			  GLboolean r, GLboolean g, GLboolean b, GLboolean a)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	int mask = (r ? RB3D_COLOR_CHANNEL_MASK_RED_MASK0 : 0) |
	    (g ? RB3D_COLOR_CHANNEL_MASK_GREEN_MASK0 : 0) |
	    (b ? RB3D_COLOR_CHANNEL_MASK_BLUE_MASK0 : 0) |
	    (a ? RB3D_COLOR_CHANNEL_MASK_ALPHA_MASK0 : 0);

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

	/* We need to clamp to user defined range here, because
	 * the HW clamping happens only for per vertex point size. */
	size = CLAMP(size, ctx->Point.MinSize, ctx->Point.MaxSize);

	/* same size limits for AA, non-AA points */
	size = CLAMP(size, ctx->Const.MinPointSize, ctx->Const.MaxPointSize);

	R300_STATECHANGE(r300, ps);
	r300->hw.ps.cmd[R300_PS_POINTSIZE] =
	    ((int)(size * 6) << R300_POINTSIZE_X_SHIFT) |
	    ((int)(size * 6) << R300_POINTSIZE_Y_SHIFT);
}

static void r300PointParameter(GLcontext * ctx, GLenum pname, const GLfloat * param)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);

	switch (pname) {
	case GL_POINT_SIZE_MIN:
		R300_STATECHANGE(r300, ga_point_minmax);
		r300->hw.ga_point_minmax.cmd[1] &= ~R300_GA_POINT_MINMAX_MIN_MASK;
		r300->hw.ga_point_minmax.cmd[1] |= (GLuint)(ctx->Point.MinSize * 6.0);
		r300PointSize(ctx, ctx->Point.Size);
		break;
	case GL_POINT_SIZE_MAX:
		R300_STATECHANGE(r300, ga_point_minmax);
		r300->hw.ga_point_minmax.cmd[1] &= ~R300_GA_POINT_MINMAX_MAX_MASK;
		r300->hw.ga_point_minmax.cmd[1] |= (GLuint)(ctx->Point.MaxSize * 6.0)
			<< R300_GA_POINT_MINMAX_MAX_SHIFT;
		r300PointSize(ctx, ctx->Point.Size);
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
static void r300LineWidth(GLcontext * ctx, GLfloat widthf)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);

	widthf = CLAMP(widthf,
                       ctx->Const.MinPointSize,
                       ctx->Const.MaxPointSize);
	R300_STATECHANGE(r300, lcntl);
	r300->hw.lcntl.cmd[1] =
	    R300_LINE_CNT_HO | R300_LINE_CNT_VE | (int)(widthf * 6.0);
}

static void r300PolygonMode(GLcontext * ctx, GLenum face, GLenum mode)
{
	(void)face;
	(void)mode;

	r300UpdatePolygonMode(ctx);
}

/* =============================================================
 * Stencil
 */

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
	return 0;
}

static void r300ShadeModel(GLcontext * ctx, GLenum mode)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);

	R300_STATECHANGE(rmesa, shade);
	rmesa->hw.shade.cmd[1] = 0x00000002;
	R300_STATECHANGE(rmesa, shade2);
	switch (mode) {
	case GL_FLAT:
		rmesa->hw.shade2.cmd[1] = R300_RE_SHADE_MODEL_FLAT;
		break;
	case GL_SMOOTH:
		rmesa->hw.shade2.cmd[1] = R300_RE_SHADE_MODEL_SMOOTH;
		break;
	default:
		return;
	}
	rmesa->hw.shade2.cmd[2] = 0x00000000;
	rmesa->hw.shade2.cmd[3] = 0x00000000;
}

static void r300StencilFuncSeparate(GLcontext * ctx, GLenum face,
				    GLenum func, GLint ref, GLuint mask)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	GLuint refmask;
	GLuint flag;
	const unsigned back = ctx->Stencil._BackFace;

	r300CatchStencilFallback(ctx);

	refmask = ((ctx->Stencil.Ref[0] & 0xff) << R300_STENCILREF_SHIFT)
	     | ((ctx->Stencil.ValueMask[0] & 0xff) << R300_STENCILMASK_SHIFT);

	R300_STATECHANGE(rmesa, zs);
	rmesa->hw.zs.cmd[R300_ZS_CNTL_0] |= R300_STENCIL_FRONT_BACK;
	rmesa->hw.zs.cmd[R300_ZS_CNTL_1] &= ~((R300_ZS_MASK <<
					       R300_S_FRONT_FUNC_SHIFT)
					      | (R300_ZS_MASK <<
						 R300_S_BACK_FUNC_SHIFT));

	rmesa->hw.zs.cmd[R300_ZS_CNTL_2] &=
	    ~((R300_STENCILREF_MASK << R300_STENCILREF_SHIFT) |
	      (R300_STENCILREF_MASK << R300_STENCILMASK_SHIFT));

	flag = translate_func(ctx->Stencil.Function[0]);
	rmesa->hw.zs.cmd[R300_ZS_CNTL_1] |=
	    (flag << R300_S_FRONT_FUNC_SHIFT);

	flag = translate_func(ctx->Stencil.Function[back]);

	rmesa->hw.zs.cmd[R300_ZS_CNTL_1] |=
	    (flag << R300_S_BACK_FUNC_SHIFT);
	rmesa->hw.zs.cmd[R300_ZS_CNTL_2] |= refmask;

	if (rmesa->radeon.radeonScreen->chip_family >= CHIP_FAMILY_RV515) {
		rmesa->hw.zs.cmd[R300_ZS_CNTL_0] |= R500_STENCIL_REFMASK_FRONT_BACK;
		R300_STATECHANGE(rmesa, zsb);
		refmask = ((ctx->Stencil.Ref[back] & 0xff) << R300_STENCILREF_SHIFT)
			| ((ctx->Stencil.ValueMask[back] & 0xff) << R300_STENCILMASK_SHIFT);

		rmesa->hw.zsb.cmd[R300_ZSB_CNTL_0] &=
			~((R300_STENCILREF_MASK << R300_STENCILREF_SHIFT) |
			  (R300_STENCILREF_MASK << R300_STENCILMASK_SHIFT));
		rmesa->hw.zsb.cmd[R300_ZSB_CNTL_0] |= refmask;
	}
}

static void r300StencilMaskSeparate(GLcontext * ctx, GLenum face, GLuint mask)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	const unsigned back = ctx->Stencil._BackFace;

	r300CatchStencilFallback(ctx);

	R300_STATECHANGE(rmesa, zs);
	rmesa->hw.zs.cmd[R300_ZS_CNTL_2] &=
	    ~(R300_STENCILREF_MASK <<
	      R300_STENCILWRITEMASK_SHIFT);
	rmesa->hw.zs.cmd[R300_ZS_CNTL_2] |=
	    (ctx->Stencil.
	     WriteMask[0] & R300_STENCILREF_MASK) <<
	     R300_STENCILWRITEMASK_SHIFT;
	if (rmesa->radeon.radeonScreen->chip_family >= CHIP_FAMILY_RV515) {
		R300_STATECHANGE(rmesa, zsb);
		rmesa->hw.zsb.cmd[R300_ZSB_CNTL_0] |=
			(ctx->Stencil.
			 WriteMask[back] & R300_STENCILREF_MASK) <<
			R300_STENCILWRITEMASK_SHIFT;
	}
}

static void r300StencilOpSeparate(GLcontext * ctx, GLenum face,
				  GLenum fail, GLenum zfail, GLenum zpass)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	const unsigned back = ctx->Stencil._BackFace;

	r300CatchStencilFallback(ctx);

	R300_STATECHANGE(rmesa, zs);
	/* It is easier to mask what's left.. */
	rmesa->hw.zs.cmd[R300_ZS_CNTL_1] &=
	    (R300_ZS_MASK << R300_Z_FUNC_SHIFT) |
	    (R300_ZS_MASK << R300_S_FRONT_FUNC_SHIFT) |
	    (R300_ZS_MASK << R300_S_BACK_FUNC_SHIFT);

	rmesa->hw.zs.cmd[R300_ZS_CNTL_1] |=
	    (translate_stencil_op(ctx->Stencil.FailFunc[0]) <<
	     R300_S_FRONT_SFAIL_OP_SHIFT)
	    | (translate_stencil_op(ctx->Stencil.ZFailFunc[0]) <<
	       R300_S_FRONT_ZFAIL_OP_SHIFT)
	    | (translate_stencil_op(ctx->Stencil.ZPassFunc[0]) <<
	       R300_S_FRONT_ZPASS_OP_SHIFT);

	rmesa->hw.zs.cmd[R300_ZS_CNTL_1] |=
	    (translate_stencil_op(ctx->Stencil.FailFunc[back]) <<
	     R300_S_BACK_SFAIL_OP_SHIFT)
	    | (translate_stencil_op(ctx->Stencil.ZFailFunc[back]) <<
	       R300_S_BACK_ZFAIL_OP_SHIFT)
	    | (translate_stencil_op(ctx->Stencil.ZPassFunc[back]) <<
	       R300_S_BACK_ZPASS_OP_SHIFT);
}

/* =============================================================
 * Window position and viewport transformation
 */

static void r300UpdateWindow(GLcontext * ctx)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	__DRIdrawable *dPriv = radeon_get_drawable(&rmesa->radeon);
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
	GLfloat tx = v[MAT_TX] + xoffset;
	GLfloat sy = v[MAT_SY] * y_scale;
	GLfloat ty = (v[MAT_TY] * y_scale) + y_bias;
	GLfloat sz = v[MAT_SZ] * depthScale;
	GLfloat tz = v[MAT_TZ] * depthScale;

	R300_STATECHANGE(rmesa, vpt);

	rmesa->hw.vpt.cmd[R300_VPT_XSCALE] = r300PackFloat32(sx);
	rmesa->hw.vpt.cmd[R300_VPT_XOFFSET] = r300PackFloat32(tx);
	rmesa->hw.vpt.cmd[R300_VPT_YSCALE] = r300PackFloat32(sy);
	rmesa->hw.vpt.cmd[R300_VPT_YOFFSET] = r300PackFloat32(ty);
	rmesa->hw.vpt.cmd[R300_VPT_ZSCALE] = r300PackFloat32(sz);
	rmesa->hw.vpt.cmd[R300_VPT_ZOFFSET] = r300PackFloat32(tz);
}

static void r300Viewport(GLcontext * ctx, GLint x, GLint y,
			 GLsizei width, GLsizei height)
{
	/* Don't pipeline viewport changes, conflict with window offset
	 * setting below.  Could apply deltas to rescue pipelined viewport
	 * values, or keep the originals hanging around.
	 */
	r300UpdateWindow(ctx);

	radeon_viewport(ctx, x, y, width, height);
}

static void r300DepthRange(GLcontext * ctx, GLclampd nearval, GLclampd farval)
{
	r300UpdateWindow(ctx);
}

void r300UpdateViewportOffset(GLcontext * ctx)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	__DRIdrawable *dPriv = radeon_get_drawable(&rmesa->radeon);
	GLfloat xoffset = (GLfloat) dPriv->x;
	GLfloat yoffset = (GLfloat) dPriv->y + dPriv->h;
	const GLfloat *v = ctx->Viewport._WindowMap.m;

	GLfloat tx = v[MAT_TX] + xoffset;
	GLfloat ty = (-v[MAT_TY]) + yoffset;

	if (rmesa->hw.vpt.cmd[R300_VPT_XOFFSET] != r300PackFloat32(tx) ||
	    rmesa->hw.vpt.cmd[R300_VPT_YOFFSET] != r300PackFloat32(ty)) {
		/* Note: this should also modify whatever data the context reset
		 * code uses...
		 */
		R300_STATECHANGE(rmesa, vpt);
		rmesa->hw.vpt.cmd[R300_VPT_XOFFSET] = r300PackFloat32(tx);
		rmesa->hw.vpt.cmd[R300_VPT_YOFFSET] = r300PackFloat32(ty);

	}

	radeonUpdateScissor(ctx);
}

/**
 * Update R300's own internal state parameters.
 * For now just STATE_R300_WINDOW_DIMENSION
 */
static void r300UpdateStateParameters(GLcontext * ctx, GLuint new_state)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	struct gl_program_parameter_list *paramList;

	if (!(new_state & (_NEW_BUFFERS | _NEW_PROGRAM | _NEW_PROGRAM_CONSTANTS)))
		return;

	if (!ctx->FragmentProgram._Current || !rmesa->selected_fp)
		return;

	paramList = ctx->FragmentProgram._Current->Base.Parameters;

	if (!paramList)
		return;

	_mesa_load_state_parameters(ctx, paramList);
}

/* =============================================================
 * Polygon state
 */
static void r300PolygonOffset(GLcontext * ctx, GLfloat factor, GLfloat units)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
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

	R300_STATECHANGE(rmesa, zbs);
	rmesa->hw.zbs.cmd[R300_ZBS_T_FACTOR] = r300PackFloat32(factor);
	rmesa->hw.zbs.cmd[R300_ZBS_T_CONSTANT] = r300PackFloat32(constant);
	rmesa->hw.zbs.cmd[R300_ZBS_W_FACTOR] = r300PackFloat32(factor);
	rmesa->hw.zbs.cmd[R300_ZBS_W_CONSTANT] = r300PackFloat32(constant);
}

/* Routing and texture-related */

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
	unsigned long mag, min, needs_fixing = 0;
	//return f;

	/* We ignore MIRROR bit so we dont have to do everything twice */
	if ((f & ((7 - 1) << R300_TX_WRAP_S_SHIFT)) ==
	    (R300_TX_CLAMP << R300_TX_WRAP_S_SHIFT)) {
		needs_fixing |= 1;
	}
	if ((f & ((7 - 1) << R300_TX_WRAP_T_SHIFT)) ==
	    (R300_TX_CLAMP << R300_TX_WRAP_T_SHIFT)) {
		needs_fixing |= 2;
	}
	if ((f & ((7 - 1) << R300_TX_WRAP_R_SHIFT)) ==
	    (R300_TX_CLAMP << R300_TX_WRAP_R_SHIFT)) {
		needs_fixing |= 4;
	}

	if (!needs_fixing)
		return f;

	mag = f & R300_TX_MAG_FILTER_MASK;
	min = f & (R300_TX_MIN_FILTER_MASK|R300_TX_MIN_FILTER_MIP_MASK);

	/* TODO: Check for anisto filters too */
	if ((mag != R300_TX_MAG_FILTER_NEAREST)
	    && (min != R300_TX_MIN_FILTER_NEAREST))
		return f;

	/* r300 cant handle these modes hence we force nearest to linear */
	if ((mag == R300_TX_MAG_FILTER_NEAREST)
	    && (min != R300_TX_MIN_FILTER_NEAREST)) {
		f &= ~R300_TX_MAG_FILTER_NEAREST;
		f |= R300_TX_MAG_FILTER_LINEAR;
		return f;
	}

	if ((min == R300_TX_MIN_FILTER_NEAREST)
	    && (mag != R300_TX_MAG_FILTER_NEAREST)) {
		f &= ~R300_TX_MIN_FILTER_NEAREST;
		f |= R300_TX_MIN_FILTER_LINEAR;
		return f;
	}

	/* Both are nearest */
	if (needs_fixing & 1) {
		f &= ~((7 - 1) << R300_TX_WRAP_S_SHIFT);
		f |= R300_TX_CLAMP_TO_EDGE << R300_TX_WRAP_S_SHIFT;
	}
	if (needs_fixing & 2) {
		f &= ~((7 - 1) << R300_TX_WRAP_T_SHIFT);
		f |= R300_TX_CLAMP_TO_EDGE << R300_TX_WRAP_T_SHIFT;
	}
	if (needs_fixing & 4) {
		f &= ~((7 - 1) << R300_TX_WRAP_R_SHIFT);
		f |= R300_TX_CLAMP_TO_EDGE << R300_TX_WRAP_R_SHIFT;
	}
	return f;
}

static void r300SetupFragmentShaderTextures(GLcontext *ctx, int *tmu_mappings)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	int i;
	struct r300_fragment_program_code *code = &r300->selected_fp->code.code.r300;

	R300_STATECHANGE(r300, fpt);

	for (i = 0; i < code->tex.length; i++) {
		int unit;
		int opcode;
		unsigned long val;

		unit = code->tex.inst[i] >> R300_TEX_ID_SHIFT;
		unit &= 15;

		val = code->tex.inst[i];
		val &= ~R300_TEX_ID_MASK;

		opcode =
			(val & R300_TEX_INST_MASK) >> R300_TEX_INST_SHIFT;
		if (opcode == R300_TEX_OP_KIL) {
			r300->hw.fpt.cmd[R300_FPT_INSTR_0 + i] = val;
		} else {
			if (tmu_mappings[unit] >= 0) {
				val |=
					tmu_mappings[unit] <<
					R300_TEX_ID_SHIFT;
				r300->hw.fpt.cmd[R300_FPT_INSTR_0 + i] = val;
			} else {
				// We get here when the corresponding texture image is incomplete
				// (e.g. incomplete mipmaps etc.)
				r300->hw.fpt.cmd[R300_FPT_INSTR_0 + i] = val;
			}
		}
	}

	r300->hw.fpt.cmd[R300_FPT_CMD_0] =
		cmdpacket0(r300->radeon.radeonScreen,
                   R300_US_TEX_INST_0, code->tex.length);
}

static void r500SetupFragmentShaderTextures(GLcontext *ctx, int *tmu_mappings)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	int i;
	struct r500_fragment_program_code *code = &r300->selected_fp->code.code.r500;

	/* find all the texture instructions and relocate the texture units */
	for (i = 0; i < code->inst_end + 1; i++) {
		if ((code->inst[i].inst0 & 0x3) == R500_INST_TYPE_TEX) {
			uint32_t val;
			int unit, opcode, new_unit;

			val = code->inst[i].inst1;

			unit = (val >> 16) & 0xf;

			val &= ~(0xf << 16);

			opcode = val & (0x7 << 22);
			if (opcode == R500_TEX_INST_TEXKILL) {
				new_unit = 0;
			} else {
				if (tmu_mappings[unit] >= 0) {
					new_unit = tmu_mappings[unit];
				} else {
					new_unit = 0;
				}
			}
			val |= R500_TEX_ID(new_unit);
			code->inst[i].inst1 = val;
		}
	}
}

static GLuint translate_lod_bias(GLfloat bias)
{
	GLint b = (int)(bias*32);
	if (b >= (1 << 9))
		b = (1 << 9)-1;
	else if (b < -(1 << 9))
		b = -(1 << 9);
	return (((GLuint)b) << R300_LOD_BIAS_SHIFT) & R300_LOD_BIAS_MASK;
}


static void r300SetupTextures(GLcontext * ctx)
{
	int i, mtu;
	struct radeon_tex_obj *t;
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	int hw_tmu = 0;
	int last_hw_tmu = -1;	/* -1 translates into no setup costs for fields */
	int tmu_mappings[R300_MAX_TEXTURE_UNITS] = { -1, };

	R300_STATECHANGE(r300, txe);
	R300_STATECHANGE(r300, tex.filter);
	R300_STATECHANGE(r300, tex.filter_1);
	R300_STATECHANGE(r300, tex.size);
	R300_STATECHANGE(r300, tex.format);
	R300_STATECHANGE(r300, tex.pitch);
	R300_STATECHANGE(r300, tex.offset);
	R300_STATECHANGE(r300, tex.chroma_key);
	R300_STATECHANGE(r300, tex.border_color);

	r300->hw.txe.cmd[R300_TXE_ENABLE] = 0x0;

	mtu = r300->radeon.glCtx->Const.MaxTextureUnits;
	if (RADEON_DEBUG & RADEON_STATE)
		fprintf(stderr, "mtu=%d\n", mtu);

	if (mtu > R300_MAX_TEXTURE_UNITS) {
		fprintf(stderr,
			"Aiiee ! mtu=%d is greater than R300_MAX_TEXTURE_UNITS=%d\n",
			mtu, R300_MAX_TEXTURE_UNITS);
		exit(-1);
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

			if (RADEON_DEBUG & RADEON_STATE)
				fprintf(stderr,
					"Activating texture unit %d\n", i);

			r300->hw.txe.cmd[R300_TXE_ENABLE] |= (1 << hw_tmu);

			r300->hw.tex.filter.cmd[R300_TEX_VALUE_0 +
						hw_tmu] =
			    gen_fixed_filter(t->pp_txfilter) | (hw_tmu << 28);
			/* Note: There is a LOD bias per texture unit and a LOD bias
			 * per texture object. We add them here to get the correct behaviour.
			 * (The per-texture object LOD bias was introduced in OpenGL 1.4
			 * and is not present in the EXT_texture_object extension).
			 */
			r300->hw.tex.filter_1.cmd[R300_TEX_VALUE_0 + hw_tmu] =
				t->pp_txfilter_1 |
				translate_lod_bias(ctx->Texture.Unit[i].LodBias + t->base.LodBias);
			r300->hw.tex.size.cmd[R300_TEX_VALUE_0 + hw_tmu] =
			    t->pp_txsize;
			r300->hw.tex.format.cmd[R300_TEX_VALUE_0 +
						hw_tmu] = t->pp_txformat;
			r300->hw.tex.pitch.cmd[R300_TEX_VALUE_0 + hw_tmu] =
			  t->pp_txpitch;
			r300->hw.textures[hw_tmu] = t;

			if (t->tile_bits & R300_TXO_MACRO_TILE) {
				WARN_ONCE("macro tiling enabled!\n");
			}

			if (t->tile_bits & R300_TXO_MICRO_TILE) {
				WARN_ONCE("micro tiling enabled!\n");
			}

			r300->hw.tex.chroma_key.cmd[R300_TEX_VALUE_0 +
						    hw_tmu] = 0x0;
			r300->hw.tex.border_color.cmd[R300_TEX_VALUE_0 +
						      hw_tmu] =
			    t->pp_border_color;

			last_hw_tmu = hw_tmu;

			hw_tmu++;
		}
	}

	/* R3xx and R4xx chips require that the texture unit corresponding to
	 * KIL instructions is really enabled.
	 *
	 * We do some fakery here and in the state atom emit logic to enable
	 * the texture without tripping up the CS checker in the kernel.
	 */
	if (r300->radeon.radeonScreen->chip_family < CHIP_FAMILY_RV515) {
		if (ctx->FragmentProgram._Current->UsesKill && last_hw_tmu < 0) {
			last_hw_tmu++;

			r300->hw.txe.cmd[R300_TXE_ENABLE] |= 1;

			r300->hw.tex.border_color.cmd[R300_TEX_VALUE_0] = 0;
			r300->hw.tex.chroma_key.cmd[R300_TEX_VALUE_0] = 0;
			r300->hw.tex.filter.cmd[R300_TEX_VALUE_0] = 0;
			r300->hw.tex.filter_1.cmd[R300_TEX_VALUE_0] = 0;
			r300->hw.tex.size.cmd[R300_TEX_VALUE_0] = 0; /* 1x1 texture */
			r300->hw.tex.format.cmd[R300_TEX_VALUE_0] = 0; /* A8 format */
			r300->hw.tex.pitch.cmd[R300_TEX_VALUE_0] = 0;
		}
	}

	r300->hw.tex.filter.cmd[R300_TEX_CMD_0] =
	    cmdpacket0(r300->radeon.radeonScreen, R300_TX_FILTER0_0, last_hw_tmu + 1);
	r300->hw.tex.filter_1.cmd[R300_TEX_CMD_0] =
	    cmdpacket0(r300->radeon.radeonScreen, R300_TX_FILTER1_0, last_hw_tmu + 1);
	r300->hw.tex.size.cmd[R300_TEX_CMD_0] =
	    cmdpacket0(r300->radeon.radeonScreen, R300_TX_SIZE_0, last_hw_tmu + 1);
	r300->hw.tex.format.cmd[R300_TEX_CMD_0] =
	    cmdpacket0(r300->radeon.radeonScreen, R300_TX_FORMAT_0, last_hw_tmu + 1);
	r300->hw.tex.pitch.cmd[R300_TEX_CMD_0] =
	    cmdpacket0(r300->radeon.radeonScreen, R300_TX_FORMAT2_0, last_hw_tmu + 1);
	r300->hw.tex.offset.cmd[R300_TEX_CMD_0] =
	    cmdpacket0(r300->radeon.radeonScreen, R300_TX_OFFSET_0, last_hw_tmu + 1);
	r300->hw.tex.chroma_key.cmd[R300_TEX_CMD_0] =
	    cmdpacket0(r300->radeon.radeonScreen, R300_TX_CHROMA_KEY_0, last_hw_tmu + 1);
	r300->hw.tex.border_color.cmd[R300_TEX_CMD_0] =
	    cmdpacket0(r300->radeon.radeonScreen, R300_TX_BORDER_COLOR_0, last_hw_tmu + 1);

	r300->vtbl.SetupFragmentShaderTextures(ctx, tmu_mappings);

	if (RADEON_DEBUG & RADEON_STATE)
		fprintf(stderr, "TX_ENABLE: %08x  last_hw_tmu=%d\n",
			r300->hw.txe.cmd[R300_TXE_ENABLE], last_hw_tmu);
}

union r300_outputs_written {
	GLuint vp_outputs;	/* hw_tcl_on */
	 DECLARE_RENDERINPUTS(index_bitset);	/* !hw_tcl_on */
};

#define R300_OUTPUTS_WRITTEN_TEST(ow, vp_result, tnl_attrib) \
	((hw_tcl_on) ? (ow).vp_outputs & (1 << (vp_result)) : \
	RENDERINPUTS_TEST( (ow.index_bitset), (tnl_attrib) ))

static void r300SetupRSUnit(GLcontext * ctx)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	union r300_outputs_written OutputsWritten;
	GLuint InputsRead;
	int fp_reg, high_rr;
	int col_ip, tex_ip;
	int rs_tex_count = 0;
	int i, col_fmt, hw_tcl_on;

	hw_tcl_on = r300->options.hw_tcl_enabled;

	if (hw_tcl_on)
		OutputsWritten.vp_outputs = r300->selected_vp->code.OutputsWritten;
	else
		RENDERINPUTS_COPY(OutputsWritten.index_bitset, r300->render_inputs_bitset);

	InputsRead = r300->selected_fp->InputsRead;

	R300_STATECHANGE(r300, ri);
	R300_STATECHANGE(r300, rc);
	R300_STATECHANGE(r300, rr);

	fp_reg = col_ip = tex_ip = col_fmt = 0;

	r300->hw.rc.cmd[1] = 0;
	r300->hw.rc.cmd[2] = 0;
	for (i=0; i<R300_RR_CMDSIZE-1; ++i)
		r300->hw.rr.cmd[R300_RR_INST_0 + i] = 0;

	for (i=0; i<R300_RI_CMDSIZE-1; ++i)
		r300->hw.ri.cmd[R300_RI_INTERP_0 + i] = 0;


	if (InputsRead & FRAG_BIT_COL0) {
		if (R300_OUTPUTS_WRITTEN_TEST(OutputsWritten, VERT_RESULT_COL0, _TNL_ATTRIB_COLOR0)) {
			r300->hw.ri.cmd[R300_RI_INTERP_0 + col_ip] = R300_RS_COL_PTR(col_ip) | R300_RS_COL_FMT(R300_RS_COL_FMT_RGBA);
			r300->hw.rr.cmd[R300_RR_INST_0 + col_ip] = R300_RS_INST_COL_ID(col_ip) | R300_RS_INST_COL_CN_WRITE | R300_RS_INST_COL_ADDR(fp_reg);
			InputsRead &= ~FRAG_BIT_COL0;
			++col_ip;
			++fp_reg;
		} else {
			WARN_ONCE("fragprog wants col0, vp doesn't provide it\n");
		}
	}

	if (InputsRead & FRAG_BIT_COL1) {
		if (R300_OUTPUTS_WRITTEN_TEST(OutputsWritten, VERT_RESULT_COL1, _TNL_ATTRIB_COLOR1)) {
			r300->hw.ri.cmd[R300_RI_INTERP_0 + col_ip] = R300_RS_COL_PTR(col_ip) | R300_RS_COL_FMT(R300_RS_COL_FMT_RGBA);
			r300->hw.rr.cmd[R300_RR_INST_0 + col_ip] = R300_RS_INST_COL_ID(col_ip) | R300_RS_INST_COL_CN_WRITE | R300_RS_INST_COL_ADDR(fp_reg);
			InputsRead &= ~FRAG_BIT_COL1;
			++col_ip;
			++fp_reg;
		} else {
			WARN_ONCE("fragprog wants col1, vp doesn't provide it\n");
		}
	}

	/* We always route 4 texcoord components */
	for (i = 0; i < ctx->Const.MaxTextureUnits; i++) {
		if (! ( InputsRead & FRAG_BIT_TEX(i) ) )
		    continue;

		if (!R300_OUTPUTS_WRITTEN_TEST(OutputsWritten, VERT_RESULT_TEX0 + i, _TNL_ATTRIB_TEX(i))) {
		    WARN_ONCE("fragprog wants coords for tex%d, vp doesn't provide them!\n", i);
		    continue;
		}

		r300->hw.ri.cmd[R300_RI_INTERP_0 + tex_ip] |= R300_RS_SEL_S(0) | R300_RS_SEL_T(1) | R300_RS_SEL_R(2) | R300_RS_SEL_Q(3) | R300_RS_TEX_PTR(rs_tex_count);
		r300->hw.rr.cmd[R300_RR_INST_0 + tex_ip] |= R300_RS_INST_TEX_ID(tex_ip) | R300_RS_INST_TEX_CN_WRITE | R300_RS_INST_TEX_ADDR(fp_reg);
		InputsRead &= ~(FRAG_BIT_TEX0 << i);
		rs_tex_count += 4;
		++tex_ip;
		++fp_reg;
	}

	/* Setup default color if no color or tex was set */
	if (rs_tex_count == 0 && col_ip == 0) {
		r300->hw.rr.cmd[R300_RR_INST_0] = R300_RS_INST_COL_ID(0) | R300_RS_INST_COL_ADDR(0);
		r300->hw.ri.cmd[R300_RI_INTERP_0] = R300_RS_COL_PTR(0) | R300_RS_COL_FMT(R300_RS_COL_FMT_0001);
		++col_ip;
	}

	high_rr = (col_ip > tex_ip) ? col_ip : tex_ip;
	r300->hw.rc.cmd[1] |= (rs_tex_count << R300_IT_COUNT_SHIFT) | (col_ip << R300_IC_COUNT_SHIFT) | R300_HIRES_EN;
	r300->hw.rc.cmd[2] |= high_rr - 1;

	r300->hw.rr.cmd[R300_RR_CMD_0] = cmdpacket0(r300->radeon.radeonScreen, R300_RS_INST_0, high_rr);
	r300->hw.ri.cmd[R300_RI_CMD_0] = cmdpacket0(r300->radeon.radeonScreen, R300_RS_IP_0, high_rr);

	if (InputsRead)
		WARN_ONCE("Don't know how to satisfy InputsRead=0x%08x\n", InputsRead);
}

static void r500SetupRSUnit(GLcontext * ctx)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	union r300_outputs_written OutputsWritten;
	GLuint InputsRead;
	int fp_reg, high_rr;
	int col_ip, tex_ip;
	int rs_tex_count = 0;
	int i, col_fmt, hw_tcl_on;

	hw_tcl_on = r300->options.hw_tcl_enabled;

	if (hw_tcl_on)
		OutputsWritten.vp_outputs = r300->selected_vp->code.OutputsWritten;
	else
		RENDERINPUTS_COPY(OutputsWritten.index_bitset, r300->render_inputs_bitset);

	InputsRead = r300->selected_fp->InputsRead;

	R300_STATECHANGE(r300, ri);
	R300_STATECHANGE(r300, rc);
	R300_STATECHANGE(r300, rr);

	fp_reg = col_ip = tex_ip = col_fmt = 0;

	r300->hw.rc.cmd[1] = 0;
	r300->hw.rc.cmd[2] = 0;
	for (i=0; i<R300_RR_CMDSIZE-1; ++i)
		r300->hw.rr.cmd[R300_RR_INST_0 + i] = 0;

	for (i=0; i<R500_RI_CMDSIZE-1; ++i)
		r300->hw.ri.cmd[R300_RI_INTERP_0 + i] = 0;


	if (InputsRead & FRAG_BIT_COL0) {
		if (R300_OUTPUTS_WRITTEN_TEST(OutputsWritten, VERT_RESULT_COL0, _TNL_ATTRIB_COLOR0)) {
			r300->hw.ri.cmd[R300_RI_INTERP_0 + col_ip] = R500_RS_COL_PTR(col_ip) | R500_RS_COL_FMT(R300_RS_COL_FMT_RGBA);
			r300->hw.rr.cmd[R300_RR_INST_0 + col_ip] = R500_RS_INST_COL_ID(col_ip) | R500_RS_INST_COL_CN_WRITE | R500_RS_INST_COL_ADDR(fp_reg);
			InputsRead &= ~FRAG_BIT_COL0;
			++col_ip;
			++fp_reg;
		} else {
			WARN_ONCE("fragprog wants col0, vp doesn't provide it\n");
		}
	}

	if (InputsRead & FRAG_BIT_COL1) {
		if (R300_OUTPUTS_WRITTEN_TEST(OutputsWritten, VERT_RESULT_COL1, _TNL_ATTRIB_COLOR1)) {
			r300->hw.ri.cmd[R300_RI_INTERP_0 + col_ip] = R500_RS_COL_PTR(col_ip) | R500_RS_COL_FMT(R300_RS_COL_FMT_RGBA);
			r300->hw.rr.cmd[R300_RR_INST_0 + col_ip] = R500_RS_INST_COL_ID(col_ip) | R500_RS_INST_COL_CN_WRITE | R500_RS_INST_COL_ADDR(fp_reg);
			InputsRead &= ~FRAG_BIT_COL1;
			++col_ip;
			++fp_reg;
		} else {
			WARN_ONCE("fragprog wants col1, vp doesn't provide it\n");
		}
	}

	/* We always route 4 texcoord components */
	for (i = 0; i < ctx->Const.MaxTextureUnits; i++) {
		if (! ( InputsRead & FRAG_BIT_TEX(i) ) )
		    continue;

		if (!R300_OUTPUTS_WRITTEN_TEST(OutputsWritten, VERT_RESULT_TEX0 + i, _TNL_ATTRIB_TEX(i))) {
		    WARN_ONCE("fragprog wants coords for tex%d, vp doesn't provide them!\n", i);
		    continue;
		}

		r300->hw.ri.cmd[R300_RI_INTERP_0 + tex_ip] |= ((rs_tex_count + 0) << R500_RS_IP_TEX_PTR_S_SHIFT) |
			((rs_tex_count + 1) << R500_RS_IP_TEX_PTR_T_SHIFT) |
			((rs_tex_count + 2) << R500_RS_IP_TEX_PTR_R_SHIFT) |
			((rs_tex_count + 3) << R500_RS_IP_TEX_PTR_Q_SHIFT);

		r300->hw.rr.cmd[R300_RR_INST_0 + tex_ip] |= R500_RS_INST_TEX_ID(tex_ip) | R500_RS_INST_TEX_CN_WRITE | R500_RS_INST_TEX_ADDR(fp_reg);
		InputsRead &= ~(FRAG_BIT_TEX0 << i);
		rs_tex_count += 4;
		++tex_ip;
		++fp_reg;
	}

	/* Setup default color if no color or tex was set */
	if (rs_tex_count == 0 && col_ip == 0) {
		r300->hw.rr.cmd[R300_RR_INST_0] = R500_RS_INST_COL_ID(0) | R500_RS_INST_COL_ADDR(0);
		r300->hw.ri.cmd[R300_RI_INTERP_0] = R500_RS_COL_PTR(0) | R500_RS_COL_FMT(R300_RS_COL_FMT_0001);
		++col_ip;
	}

	high_rr = (col_ip > tex_ip) ? col_ip : tex_ip;
	r300->hw.rc.cmd[1] = (rs_tex_count << R300_IT_COUNT_SHIFT) | (col_ip << R300_IC_COUNT_SHIFT) | R300_HIRES_EN;
	r300->hw.rc.cmd[2] = 0xC0 | (high_rr - 1);

	r300->hw.rr.cmd[R300_RR_CMD_0] = cmdpacket0(r300->radeon.radeonScreen, R500_RS_INST_0, high_rr);
	r300->hw.ri.cmd[R300_RI_CMD_0] = cmdpacket0(r300->radeon.radeonScreen, R500_RS_IP_0, high_rr);

	if (InputsRead)
		WARN_ONCE("Don't know how to satisfy InputsRead=0x%08x\n", InputsRead);
}

#define MIN3(a, b, c)	((a) < (b) ? MIN2(a, c) : MIN2(b, c))

void r300VapCntl(r300ContextPtr rmesa, GLuint input_count,
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

    R300_STATECHANGE(rmesa, vap_cntl);
    if (rmesa->options.hw_tcl_enabled) {
	rmesa->hw.vap_cntl.cmd[R300_VAP_CNTL_INSTR] =
	    (pvs_num_slots << R300_PVS_NUM_SLOTS_SHIFT) |
	    (pvs_num_cntrls << R300_PVS_NUM_CNTLRS_SHIFT) |
	    (12 << R300_VF_MAX_VTX_NUM_SHIFT);
	if (rmesa->radeon.radeonScreen->chip_family >= CHIP_FAMILY_RV515)
	    rmesa->hw.vap_cntl.cmd[R300_VAP_CNTL_INSTR] |= R500_TCL_STATE_OPTIMIZATION;
    } else
	/* not sure about non-tcl */
	rmesa->hw.vap_cntl.cmd[R300_VAP_CNTL_INSTR] = ((10 << R300_PVS_NUM_SLOTS_SHIFT) |
				    (5 << R300_PVS_NUM_CNTLRS_SHIFT) |
				    (5 << R300_VF_MAX_VTX_NUM_SHIFT));

    if ((rmesa->radeon.radeonScreen->chip_family == CHIP_FAMILY_R300) ||
	(rmesa->radeon.radeonScreen->chip_family == CHIP_FAMILY_R350))
	rmesa->hw.vap_cntl.cmd[R300_VAP_CNTL_INSTR] |= (4 << R300_PVS_NUM_FPUS_SHIFT);
    else if (rmesa->radeon.radeonScreen->chip_family == CHIP_FAMILY_RV530)
	rmesa->hw.vap_cntl.cmd[R300_VAP_CNTL_INSTR] |= (5 << R300_PVS_NUM_FPUS_SHIFT);
    else if ((rmesa->radeon.radeonScreen->chip_family == CHIP_FAMILY_RV410) ||
	     (rmesa->radeon.radeonScreen->chip_family == CHIP_FAMILY_R420))
	rmesa->hw.vap_cntl.cmd[R300_VAP_CNTL_INSTR] |= (6 << R300_PVS_NUM_FPUS_SHIFT);
    else if ((rmesa->radeon.radeonScreen->chip_family == CHIP_FAMILY_R520) ||
	     (rmesa->radeon.radeonScreen->chip_family == CHIP_FAMILY_R580) ||
	     (rmesa->radeon.radeonScreen->chip_family == CHIP_FAMILY_RV560) ||
	     (rmesa->radeon.radeonScreen->chip_family == CHIP_FAMILY_RV570))
	rmesa->hw.vap_cntl.cmd[R300_VAP_CNTL_INSTR] |= (8 << R300_PVS_NUM_FPUS_SHIFT);
    else
	rmesa->hw.vap_cntl.cmd[R300_VAP_CNTL_INSTR] |= (2 << R300_PVS_NUM_FPUS_SHIFT);

}

/**
 * Enable/Disable states.
 *
 * \note Mesa already filters redundant calls to this function.
 */
static void r300Enable(GLcontext * ctx, GLenum cap, GLboolean state)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	if (RADEON_DEBUG & RADEON_STATE)
		fprintf(stderr, "%s( %s = %s )\n", __FUNCTION__,
			_mesa_lookup_enum_by_nr(cap),
			state ? "GL_TRUE" : "GL_FALSE");

	switch (cap) {
	case GL_ALPHA_TEST:
		r300SetAlphaState(ctx);
		break;
	case GL_COLOR_LOGIC_OP:
		r300SetLogicOpState(ctx);
		/* fall-through, because logic op overrides blending */
	case GL_BLEND:
		r300SetBlendState(ctx);
		break;
	case GL_CLIP_PLANE0:
	case GL_CLIP_PLANE1:
	case GL_CLIP_PLANE2:
	case GL_CLIP_PLANE3:
	case GL_CLIP_PLANE4:
	case GL_CLIP_PLANE5:
		r300SetClipPlaneState(ctx, cap, state);
		break;
	case GL_CULL_FACE:
		r300UpdateCulling(ctx);
		break;
	case GL_DEPTH_TEST:
		r300SetDepthState(ctx);
		break;
	case GL_LINE_SMOOTH:
		if (rmesa->options.conformance_mode)
			r300SwitchFallback(ctx, R300_FALLBACK_LINE_SMOOTH, ctx->Line.SmoothFlag);
		break;
	case GL_LINE_STIPPLE:
		if (rmesa->options.conformance_mode)
			r300SwitchFallback(ctx, R300_FALLBACK_LINE_STIPPLE, ctx->Line.StippleFlag);
		break;
	case GL_POINT_SMOOTH:
		if (rmesa->options.conformance_mode)
			r300SwitchFallback(ctx, R300_FALLBACK_POINT_SMOOTH, ctx->Point.SmoothFlag);
		break;
	case GL_POLYGON_SMOOTH:
		if (rmesa->options.conformance_mode)
			r300SwitchFallback(ctx, R300_FALLBACK_POLYGON_SMOOTH, ctx->Polygon.SmoothFlag);
		break;
	case GL_POLYGON_STIPPLE:
		if (rmesa->options.conformance_mode)
			r300SwitchFallback(ctx, R300_FALLBACK_POLYGON_STIPPLE, ctx->Polygon.StippleFlag);
		break;
	case GL_POLYGON_OFFSET_POINT:
	case GL_POLYGON_OFFSET_LINE:
	case GL_POLYGON_OFFSET_FILL:
		r300SetPolygonOffsetState(ctx, state);
		break;
	case GL_SCISSOR_TEST:
		radeon_firevertices(&rmesa->radeon);
		rmesa->radeon.state.scissor.enabled = state;
		radeonUpdateScissor( ctx );
		break;
	case GL_STENCIL_TEST:
		r300SetStencilState(ctx, state);
		break;
	default:
		break;
	}
}

/**
 * Completely recalculates hardware state based on the Mesa state.
 */
static void r300ResetHwState(r300ContextPtr r300)
{
	GLcontext *ctx = r300->radeon.glCtx;
	int has_tcl;

	has_tcl = r300->options.hw_tcl_enabled;

	if (RADEON_DEBUG & RADEON_STATE)
		fprintf(stderr, "%s\n", __FUNCTION__);

	r300ColorMask(ctx,
		      ctx->Color.ColorMask[0][RCOMP],
		      ctx->Color.ColorMask[0][GCOMP],
		      ctx->Color.ColorMask[0][BCOMP],
                      ctx->Color.ColorMask[0][ACOMP]);

	r300Enable(ctx, GL_DEPTH_TEST, ctx->Depth.Test);
	r300DepthMask(ctx, ctx->Depth.Mask);
	r300DepthFunc(ctx, ctx->Depth.Func);

	/* stencil */
	r300Enable(ctx, GL_STENCIL_TEST, ctx->Stencil._Enabled);
	r300StencilMaskSeparate(ctx, 0, ctx->Stencil.WriteMask[0]);
	r300StencilFuncSeparate(ctx, 0, ctx->Stencil.Function[0],
				ctx->Stencil.Ref[0], ctx->Stencil.ValueMask[0]);
	r300StencilOpSeparate(ctx, 0, ctx->Stencil.FailFunc[0],
			      ctx->Stencil.ZFailFunc[0],
			      ctx->Stencil.ZPassFunc[0]);

	r300UpdateCulling(ctx);

	r300SetBlendState(ctx);
	r300SetLogicOpState(ctx);

	r300AlphaFunc(ctx, ctx->Color.AlphaFunc, ctx->Color.AlphaRef);
	r300Enable(ctx, GL_ALPHA_TEST, ctx->Color.AlphaEnabled);

	r300->hw.vte.cmd[1] = R300_VPORT_X_SCALE_ENA
	    | R300_VPORT_X_OFFSET_ENA
	    | R300_VPORT_Y_SCALE_ENA
	    | R300_VPORT_Y_OFFSET_ENA
	    | R300_VPORT_Z_SCALE_ENA
	    | R300_VPORT_Z_OFFSET_ENA | R300_VTX_W0_FMT;
	r300->hw.vte.cmd[2] = 0x00000008;

	r300->hw.vap_vf_max_vtx_indx.cmd[1] = 0x00FFFFFF;
	r300->hw.vap_vf_max_vtx_indx.cmd[2] = 0x00000000;

#ifdef MESA_LITTLE_ENDIAN
	r300->hw.vap_cntl_status.cmd[1] = R300_VC_NO_SWAP;
#else
	r300->hw.vap_cntl_status.cmd[1] = R300_VC_32BIT_SWAP;
#endif

	/* disable VAP/TCL on non-TCL capable chips */
	if (!has_tcl)
		r300->hw.vap_cntl_status.cmd[1] |= R300_VAP_TCL_BYPASS;

	r300->hw.vap_psc_sgn_norm_cntl.cmd[1] = 0xAAAAAAAA;

	/* XXX: Other families? */
	if (has_tcl) {
		r300->hw.vap_clip_cntl.cmd[1] = R300_PS_UCP_MODE_DIST_COP;

		r300->hw.vap_clip.cmd[1] = r300PackFloat32(1.0); /* X */
		r300->hw.vap_clip.cmd[2] = r300PackFloat32(1.0); /* X */
		r300->hw.vap_clip.cmd[3] = r300PackFloat32(1.0); /* Y */
		r300->hw.vap_clip.cmd[4] = r300PackFloat32(1.0); /* Y */

		switch (r300->radeon.radeonScreen->chip_family) {
		case CHIP_FAMILY_R300:
			r300->hw.vap_pvs_vtx_timeout_reg.cmd[1] = R300_2288_R300;
			break;
		default:
			r300->hw.vap_pvs_vtx_timeout_reg.cmd[1] = R300_2288_RV350;
			break;
		}
	}

	r300->hw.gb_enable.cmd[1] = R300_GB_POINT_STUFF_ENABLE
	    | R300_GB_LINE_STUFF_ENABLE
	    | R300_GB_TRIANGLE_STUFF_ENABLE;

	r300->hw.gb_misc.cmd[R300_GB_MISC_MSPOS_0] = 0x66666666;
	r300->hw.gb_misc.cmd[R300_GB_MISC_MSPOS_1] = 0x06666666;

	r300->hw.gb_misc.cmd[R300_GB_MISC_TILE_CONFIG] =
	    R300_GB_TILE_ENABLE | R300_GB_TILE_SIZE_16 /*| R300_GB_SUBPIXEL_1_16*/;
	switch (r300->radeon.radeonScreen->num_gb_pipes) {
	case 1:
	default:
		r300->hw.gb_misc.cmd[R300_GB_MISC_TILE_CONFIG] |=
		    R300_GB_TILE_PIPE_COUNT_RV300;
		break;
	case 2:
		r300->hw.gb_misc.cmd[R300_GB_MISC_TILE_CONFIG] |=
		    R300_GB_TILE_PIPE_COUNT_R300;
		break;
	case 3:
		r300->hw.gb_misc.cmd[R300_GB_MISC_TILE_CONFIG] |=
		    R300_GB_TILE_PIPE_COUNT_R420_3P;
		break;
	case 4:
		r300->hw.gb_misc.cmd[R300_GB_MISC_TILE_CONFIG] |=
		    R300_GB_TILE_PIPE_COUNT_R420;
		break;
	}

	/* XXX: Enable anti-aliasing? */
	r300->hw.gb_misc2.cmd[R300_GB_MISC2_AA_CONFIG] = GB_AA_CONFIG_AA_DISABLE;
	r300->hw.gb_misc2.cmd[R300_GB_MISC2_SELECT] = 0;

	r300->hw.ga_point_s0.cmd[1] = r300PackFloat32(0.0);
	r300->hw.ga_point_s0.cmd[2] = r300PackFloat32(0.0);
	r300->hw.ga_point_s0.cmd[3] = r300PackFloat32(1.0);
	r300->hw.ga_point_s0.cmd[4] = r300PackFloat32(1.0);

	r300->hw.ga_triangle_stipple.cmd[1] = 0x00050005;

	r300PointSize(ctx, 1.0);

	r300->hw.ga_point_minmax.cmd[1] = 0x18000006;
	r300->hw.ga_point_minmax.cmd[2] = 0x00020006;
	r300->hw.ga_point_minmax.cmd[3] = r300PackFloat32(1.0 / 192.0);

	r300LineWidth(ctx, 1.0);

	r300->hw.ga_line_stipple.cmd[1] = 0;
	r300->hw.ga_line_stipple.cmd[2] = r300PackFloat32(0.0);
	r300->hw.ga_line_stipple.cmd[3] = r300PackFloat32(1.0);

	r300ShadeModel(ctx, ctx->Light.ShadeModel);

	r300PolygonMode(ctx, GL_FRONT, ctx->Polygon.FrontMode);
	r300PolygonMode(ctx, GL_BACK, ctx->Polygon.BackMode);
	r300->hw.zbias_cntl.cmd[1] = 0x00000000;

	r300PolygonOffset(ctx, ctx->Polygon.OffsetFactor,
			  ctx->Polygon.OffsetUnits);
	r300Enable(ctx, GL_POLYGON_OFFSET_POINT, ctx->Polygon.OffsetPoint);
	r300Enable(ctx, GL_POLYGON_OFFSET_LINE, ctx->Polygon.OffsetLine);
	r300Enable(ctx, GL_POLYGON_OFFSET_FILL, ctx->Polygon.OffsetFill);

	r300->hw.su_depth_scale.cmd[1] = 0x4B7FFFFF;
	r300->hw.su_depth_scale.cmd[2] = 0x00000000;

	r300->hw.sc_hyperz.cmd[1] = 0x0000001C;
	r300->hw.sc_hyperz.cmd[2] = 0x2DA49525;

	r300->hw.sc_screendoor.cmd[1] = 0x00FFFFFF;

	r300->hw.us_out_fmt.cmd[1] = R500_OUT_FMT_C4_8  |
	  R500_C0_SEL_B | R500_C1_SEL_G | R500_C2_SEL_R | R500_C3_SEL_A;
	r300->hw.us_out_fmt.cmd[2] = R500_OUT_FMT_UNUSED |
	  R500_C0_SEL_B | R500_C1_SEL_G | R500_C2_SEL_R | R500_C3_SEL_A;
	r300->hw.us_out_fmt.cmd[3] = R500_OUT_FMT_UNUSED |
	  R500_C0_SEL_B | R500_C1_SEL_G | R500_C2_SEL_R | R500_C3_SEL_A;
	r300->hw.us_out_fmt.cmd[4] = R500_OUT_FMT_UNUSED |
	  R500_C0_SEL_B | R500_C1_SEL_G | R500_C2_SEL_R | R500_C3_SEL_A;
	r300->hw.us_out_fmt.cmd[5] = R300_W_FMT_W0 | R300_W_SRC_US;

	/* disable fog unit */
	r300->hw.fogs.cmd[R300_FOGS_STATE] = 0;
	r300->hw.fg_depth_src.cmd[1] = R300_FG_DEPTH_SRC_SCAN;

	r300->hw.rb3d_cctl.cmd[1] = 0;

	r300BlendColor(ctx, ctx->Color.BlendColor);

	r300->hw.rb3d_dither_ctl.cmd[1] = 0;
	r300->hw.rb3d_dither_ctl.cmd[2] = 0;
	r300->hw.rb3d_dither_ctl.cmd[3] = 0;
	r300->hw.rb3d_dither_ctl.cmd[4] = 0;
	r300->hw.rb3d_dither_ctl.cmd[5] = 0;
	r300->hw.rb3d_dither_ctl.cmd[6] = 0;
	r300->hw.rb3d_dither_ctl.cmd[7] = 0;
	r300->hw.rb3d_dither_ctl.cmd[8] = 0;
	r300->hw.rb3d_dither_ctl.cmd[9] = 0;

	r300->hw.rb3d_aaresolve_ctl.cmd[1] = 0;

    r300->hw.rb3d_discard_src_pixel_lte_threshold.cmd[1] = 0x00000000;
    r300->hw.rb3d_discard_src_pixel_lte_threshold.cmd[2] = 0xffffffff;

	r300->hw.zb_depthclearvalue.cmd[1] = 0;

	r300->hw.zstencil_format.cmd[2] = R300_ZTOP_DISABLE;
	r300->hw.zstencil_format.cmd[3] = 0x00000003;
	r300->hw.zstencil_format.cmd[4] = 0x00000000;
	r300SetEarlyZState(ctx);

	r300->hw.zb_zmask.cmd[1] = 0;
	r300->hw.zb_zmask.cmd[2] = 0;

	r300->hw.zb_hiz_offset.cmd[1] = 0;

	r300->hw.zb_hiz_pitch.cmd[1] = 0;

	r300VapCntl(r300, 0, 0, 0);
	if (has_tcl) {
		r300->hw.vps.cmd[R300_VPS_ZERO_0] = 0;
		r300->hw.vps.cmd[R300_VPS_ZERO_1] = 0;
		r300->hw.vps.cmd[R300_VPS_POINTSIZE] = r300PackFloat32(1.0);
		r300->hw.vps.cmd[R300_VPS_ZERO_3] = 0;
	}

	r300->radeon.hw.all_dirty = GL_TRUE;
}

void r300UpdateShaders(r300ContextPtr rmesa)
{
	GLcontext *ctx = rmesa->radeon.glCtx;

	/* should only happenen once, just after context is created */
	/* TODO: shouldn't we fallback to sw here? */
	if (!ctx->FragmentProgram._Current) {
		fprintf(stderr, "No ctx->FragmentProgram._Current!!\n");
		return;
	}

	{
		struct r300_fragment_program *fp;

		fp = r300SelectAndTranslateFragmentShader(ctx);

		r300SwitchFallback(ctx, R300_FALLBACK_FRAGMENT_PROGRAM, fp->error);
	}

	if (rmesa->options.hw_tcl_enabled) {
		struct r300_vertex_program *vp;

		vp = r300SelectAndTranslateVertexShader(ctx);

		r300SwitchFallback(ctx, R300_FALLBACK_VERTEX_PROGRAM, vp->error);
	}

	r300UpdateStateParameters(ctx, _NEW_PROGRAM | _NEW_PROGRAM_CONSTANTS);
	rmesa->radeon.NewGLState = 0;
}

static const GLfloat *get_fragmentprogram_constant(GLcontext *ctx, GLuint index, GLfloat * buffer)
{
	static const GLfloat dummy[4] = { 0, 0, 0, 0 };
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	struct rc_constant * rcc = &rmesa->selected_fp->code.constants.Constants[index];

	switch(rcc->Type) {
	case RC_CONSTANT_EXTERNAL:
		return ctx->FragmentProgram._Current->Base.Parameters->ParameterValues[rcc->u.External];
	case RC_CONSTANT_IMMEDIATE:
		return rcc->u.Immediate;
	case RC_CONSTANT_STATE:
		switch(rcc->u.State[0]) {
		case RC_STATE_SHADOW_AMBIENT: {
			const int unit = (int) rcc->u.State[1];
			const struct gl_texture_object *texObj = ctx->Texture.Unit[unit]._Current;
			if (texObj) {
				buffer[0] =
				buffer[1] =
				buffer[2] =
				buffer[3] = texObj->CompareFailValue;
			}
			return buffer;
		}

		case RC_STATE_R300_WINDOW_DIMENSION: {
			__DRIdrawable * drawable = radeon_get_drawable(&rmesa->radeon);
			buffer[0] = drawable->w * 0.5f;	/* width*0.5 */
			buffer[1] = drawable->h * 0.5f;	/* height*0.5 */
			buffer[2] = 0.5F;	/* for moving range [-1 1] -> [0 1] */
			buffer[3] = 1.0F;	/* not used */
			return buffer;
		}

		case RC_STATE_R300_TEXRECT_FACTOR: {
			struct gl_texture_object *t =
				ctx->Texture.Unit[rcc->u.State[1]].CurrentTex[TEXTURE_RECT_INDEX];

			if (t && t->Image[0][t->BaseLevel]) {
				struct gl_texture_image *image =
					t->Image[0][t->BaseLevel];
				buffer[0] = 1.0 / image->Width2;
				buffer[1] = 1.0 / image->Height2;
			} else {
				buffer[0] = 1.0;
				buffer[1] = 1.0;
			}
			buffer[2] = 1.0;
			buffer[3] = 1.0;
			return buffer;
		}
		}
	}

	return dummy;
}


static void r300SetupPixelShader(GLcontext *ctx)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	struct r300_fragment_program *fp = rmesa->selected_fp;
	struct r300_fragment_program_code *code;
	int i;

	code = &fp->code.code.r300;

	R300_STATECHANGE(rmesa, fpi[0]);
	R300_STATECHANGE(rmesa, fpi[1]);
	R300_STATECHANGE(rmesa, fpi[2]);
	R300_STATECHANGE(rmesa, fpi[3]);
	rmesa->hw.fpi[0].cmd[R300_FPI_CMD_0] = cmdpacket0(rmesa->radeon.radeonScreen, R300_US_ALU_RGB_INST_0, code->alu.length);
	rmesa->hw.fpi[1].cmd[R300_FPI_CMD_0] = cmdpacket0(rmesa->radeon.radeonScreen, R300_US_ALU_RGB_ADDR_0, code->alu.length);
	rmesa->hw.fpi[2].cmd[R300_FPI_CMD_0] = cmdpacket0(rmesa->radeon.radeonScreen, R300_US_ALU_ALPHA_INST_0, code->alu.length);
	rmesa->hw.fpi[3].cmd[R300_FPI_CMD_0] = cmdpacket0(rmesa->radeon.radeonScreen, R300_US_ALU_ALPHA_ADDR_0, code->alu.length);
	for (i = 0; i < code->alu.length; i++) {
		rmesa->hw.fpi[0].cmd[R300_FPI_INSTR_0 + i] = code->alu.inst[i].rgb_inst;
		rmesa->hw.fpi[1].cmd[R300_FPI_INSTR_0 + i] = code->alu.inst[i].rgb_addr;
		rmesa->hw.fpi[2].cmd[R300_FPI_INSTR_0 + i] = code->alu.inst[i].alpha_inst;
		rmesa->hw.fpi[3].cmd[R300_FPI_INSTR_0 + i] = code->alu.inst[i].alpha_addr;
	}

	R300_STATECHANGE(rmesa, fp);
	rmesa->hw.fp.cmd[R300_FP_CNTL0] = code->config;
	rmesa->hw.fp.cmd[R300_FP_CNTL1] = code->pixsize;
	rmesa->hw.fp.cmd[R300_FP_CNTL2] = code->code_offset;
	for (i = 0; i < 4; i++)
		rmesa->hw.fp.cmd[R300_FP_NODE0 + i] = code->code_addr[i];

	R300_STATECHANGE(rmesa, fpp);
	rmesa->hw.fpp.cmd[R300_FPP_CMD_0] = cmdpacket0(rmesa->radeon.radeonScreen, R300_PFS_PARAM_0_X, fp->code.constants.Count * 4);
	for (i = 0; i < fp->code.constants.Count; i++) {
		GLfloat buffer[4];
		const GLfloat *constant = get_fragmentprogram_constant(ctx, i, buffer);
		rmesa->hw.fpp.cmd[R300_FPP_PARAM_0 + 4 * i + 0] = r300PackFloat24(constant[0]);
		rmesa->hw.fpp.cmd[R300_FPP_PARAM_0 + 4 * i + 1] = r300PackFloat24(constant[1]);
		rmesa->hw.fpp.cmd[R300_FPP_PARAM_0 + 4 * i + 2] = r300PackFloat24(constant[2]);
		rmesa->hw.fpp.cmd[R300_FPP_PARAM_0 + 4 * i + 3] = r300PackFloat24(constant[3]);
	}
}

#define bump_r500fp_count(ptr, new_count)   do{\
	drm_r300_cmd_header_t* _p=((drm_r300_cmd_header_t*)(ptr));\
	int _nc=(new_count)/6; \
	assert(_nc < 256); \
	if(_nc>_p->r500fp.count)_p->r500fp.count=_nc;\
} while(0)

#define bump_r500fp_const_count(ptr, new_count)   do{\
	drm_r300_cmd_header_t* _p=((drm_r300_cmd_header_t*)(ptr));\
	int _nc=(new_count)/4; \
	assert(_nc < 256); \
	if(_nc>_p->r500fp.count)_p->r500fp.count=_nc;\
} while(0)

static void r500SetupPixelShader(GLcontext *ctx)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	struct r300_fragment_program *fp = rmesa->selected_fp;
	int i;
	struct r500_fragment_program_code *code;

	((drm_r300_cmd_header_t *) rmesa->hw.r500fp.cmd)->r500fp.count = 0;
	((drm_r300_cmd_header_t *) rmesa->hw.r500fp_const.cmd)->r500fp.count = 0;

	code = &fp->code.code.r500;

	R300_STATECHANGE(rmesa, fp);
	rmesa->hw.fp.cmd[R500_FP_PIXSIZE] = code->max_temp_idx;

	rmesa->hw.fp.cmd[R500_FP_CODE_ADDR] =
	    R500_US_CODE_START_ADDR(0) |
	    R500_US_CODE_END_ADDR(code->inst_end);
	rmesa->hw.fp.cmd[R500_FP_CODE_RANGE] =
	    R500_US_CODE_RANGE_ADDR(0) |
	    R500_US_CODE_RANGE_SIZE(code->inst_end);
	rmesa->hw.fp.cmd[R500_FP_CODE_OFFSET] =
	    R500_US_CODE_OFFSET_ADDR(0);

	R300_STATECHANGE(rmesa, r500fp);
	/* Emit our shader... */
	for (i = 0; i < code->inst_end+1; i++) {
		rmesa->hw.r500fp.cmd[i*6+1] = code->inst[i].inst0;
		rmesa->hw.r500fp.cmd[i*6+2] = code->inst[i].inst1;
		rmesa->hw.r500fp.cmd[i*6+3] = code->inst[i].inst2;
		rmesa->hw.r500fp.cmd[i*6+4] = code->inst[i].inst3;
		rmesa->hw.r500fp.cmd[i*6+5] = code->inst[i].inst4;
		rmesa->hw.r500fp.cmd[i*6+6] = code->inst[i].inst5;
	}

	bump_r500fp_count(rmesa->hw.r500fp.cmd, (code->inst_end + 1) * 6);

	R300_STATECHANGE(rmesa, r500fp_const);
	for (i = 0; i < fp->code.constants.Count; i++) {
		GLfloat buffer[4];
		const GLfloat *constant = get_fragmentprogram_constant(ctx, i, buffer);
		rmesa->hw.r500fp_const.cmd[R300_FPP_PARAM_0 + 4 * i + 0] = r300PackFloat32(constant[0]);
		rmesa->hw.r500fp_const.cmd[R300_FPP_PARAM_0 + 4 * i + 1] = r300PackFloat32(constant[1]);
		rmesa->hw.r500fp_const.cmd[R300_FPP_PARAM_0 + 4 * i + 2] = r300PackFloat32(constant[2]);
		rmesa->hw.r500fp_const.cmd[R300_FPP_PARAM_0 + 4 * i + 3] = r300PackFloat32(constant[3]);
	}
	bump_r500fp_const_count(rmesa->hw.r500fp_const.cmd, fp->code.constants.Count * 4);
}

void r300SetupVAP(GLcontext *ctx, GLuint InputsRead, GLuint OutputsWritten)
{
	r300ContextPtr rmesa = R300_CONTEXT( ctx );
	struct vertex_attribute *attrs = rmesa->vbuf.attribs;
	int i, j, reg_count;
	uint32_t *vir0 = &rmesa->hw.vir[0].cmd[1];
	uint32_t *vir1 = &rmesa->hw.vir[1].cmd[1];

	for (i = 0; i < R300_VIR_CMDSIZE-1; ++i)
		vir0[i] = vir1[i] = 0;

	for (i = 0, j = 0; i < rmesa->vbuf.num_attribs; ++i) {
		int tmp;

		tmp = attrs[i].data_type | (attrs[i].dst_loc << R300_DST_VEC_LOC_SHIFT);
		if (attrs[i]._signed)
			tmp |= R300_SIGNED;
		if (attrs[i].normalize)
			tmp |= R300_NORMALIZE;

		if (i % 2 == 0) {
			vir0[j] = tmp << R300_DATA_TYPE_0_SHIFT;
			vir1[j] = attrs[i].swizzle | (attrs[i].write_mask << R300_WRITE_ENA_SHIFT);
		} else {
			vir0[j] |= tmp << R300_DATA_TYPE_1_SHIFT;
			vir1[j] |= (attrs[i].swizzle | (attrs[i].write_mask << R300_WRITE_ENA_SHIFT)) << R300_SWIZZLE1_SHIFT;
			++j;
		}
	}

	reg_count = (rmesa->vbuf.num_attribs + 1) >> 1;
	if (rmesa->vbuf.num_attribs % 2 != 0) {
		vir0[reg_count-1] |= R300_LAST_VEC << R300_DATA_TYPE_0_SHIFT;
	} else {
		vir0[reg_count-1] |= R300_LAST_VEC << R300_DATA_TYPE_1_SHIFT;
	}

	R300_STATECHANGE(rmesa, vir[0]);
	R300_STATECHANGE(rmesa, vir[1]);
	R300_STATECHANGE(rmesa, vof);
	R300_STATECHANGE(rmesa, vic);

	if (rmesa->radeon.radeonScreen->kernel_mm) {
		rmesa->hw.vir[0].cmd[0] &= 0xC000FFFF;
		rmesa->hw.vir[1].cmd[0] &= 0xC000FFFF;
		rmesa->hw.vir[0].cmd[0] |= (reg_count & 0x3FFF) << 16;
		rmesa->hw.vir[1].cmd[0] |= (reg_count & 0x3FFF) << 16;
	} else {
		((drm_r300_cmd_header_t *) rmesa->hw.vir[0].cmd)->packet0.count = reg_count;
		((drm_r300_cmd_header_t *) rmesa->hw.vir[1].cmd)->packet0.count = reg_count;
	}

	rmesa->hw.vic.cmd[R300_VIC_CNTL_0] = r300VAPInputCntl0(ctx, InputsRead);
	rmesa->hw.vic.cmd[R300_VIC_CNTL_1] = r300VAPInputCntl1(ctx, InputsRead);
	rmesa->hw.vof.cmd[R300_VOF_CNTL_0] = r300VAPOutputCntl0(ctx, OutputsWritten);
	rmesa->hw.vof.cmd[R300_VOF_CNTL_1] = r300VAPOutputCntl1(ctx, OutputsWritten);
}

void r300UpdateShaderStates(r300ContextPtr rmesa)
{
	GLcontext *ctx;
	ctx = rmesa->radeon.glCtx;

	/* should only happenen once, just after context is created */
	if (!ctx->FragmentProgram._Current)
		return;

	r300SetEarlyZState(ctx);

	r300SetupTextures(ctx);

	rmesa->vtbl.SetupPixelShader(ctx);

	rmesa->vtbl.SetupRSUnit(ctx);

	if (rmesa->options.hw_tcl_enabled) {
		r300SetupVertexProgram(rmesa);
	}
}

/**
 * Called by Mesa after an internal state update.
 */
static void r300InvalidateState(GLcontext * ctx, GLuint new_state)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);

	_swrast_InvalidateState(ctx, new_state);
	_swsetup_InvalidateState(ctx, new_state);
	_vbo_InvalidateState(ctx, new_state);
	_tnl_InvalidateState(ctx, new_state);

	if (new_state & _NEW_BUFFERS) {
		_mesa_update_framebuffer(ctx);
		/* this updates the DrawBuffer's Width/Height if it's a FBO */
		_mesa_update_draw_buffer_bounds(ctx);

		R300_STATECHANGE(r300, cb);
		R300_STATECHANGE(r300, zb);
	}

	if (new_state & (_NEW_LIGHT)) {
		R300_STATECHANGE(r300, shade2);
		if (ctx->Light.ProvokingVertex == GL_LAST_VERTEX_CONVENTION)
			r300->hw.shade2.cmd[1] |= R300_GA_COLOR_CONTROL_PROVOKING_VERTEX_LAST;
		else
			r300->hw.shade2.cmd[1] &= ~R300_GA_COLOR_CONTROL_PROVOKING_VERTEX_LAST;
	}

	r300->radeon.NewGLState |= new_state;
}

/**
 * Calculate initial hardware state and register state functions.
 * Assumes that the command buffer and state atoms have been
 * initialized already.
 */
void r300InitState(r300ContextPtr r300)
{
	r300ResetHwState(r300);
}

static void r300RenderMode(GLcontext * ctx, GLenum mode)
{
	r300SwitchFallback(ctx, R300_FALLBACK_RENDER_MODE, ctx->RenderMode != GL_RENDER);
}

/**
 * Initialize driver's state callback functions
 */
void r300InitStateFuncs(struct dd_function_table *functions)
{

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
	functions->ShadeModel = r300ShadeModel;
	functions->LogicOpcode = r300LogicOpcode;

	/* ARB_point_parameters */
	functions->PointParameterfv = r300PointParameter;

	/* Stencil related */
	functions->StencilFuncSeparate = r300StencilFuncSeparate;
	functions->StencilMaskSeparate = r300StencilMaskSeparate;
	functions->StencilOpSeparate = r300StencilOpSeparate;

	/* Viewport related */
	functions->Viewport = r300Viewport;
	functions->DepthRange = r300DepthRange;
	functions->PointSize = r300PointSize;
	functions->LineWidth = r300LineWidth;

	functions->PolygonOffset = r300PolygonOffset;
	functions->PolygonMode = r300PolygonMode;

	functions->RenderMode = r300RenderMode;

	functions->ClipPlane = r300ClipPlane;
	functions->Scissor = radeonScissor;

	functions->DrawBuffer		= radeonDrawBuffer;
	functions->ReadBuffer		= radeonReadBuffer;
}

void r300InitShaderFunctions(r300ContextPtr r300)
{
	if (r300->radeon.radeonScreen->chip_family >= CHIP_FAMILY_RV515) {
		r300->vtbl.SetupRSUnit = r500SetupRSUnit;
		r300->vtbl.SetupPixelShader = r500SetupPixelShader;
		r300->vtbl.SetupFragmentShaderTextures = r500SetupFragmentShaderTextures;
	} else {
		r300->vtbl.SetupRSUnit = r300SetupRSUnit;
		r300->vtbl.SetupPixelShader = r300SetupPixelShader;
		r300->vtbl.SetupFragmentShaderTextures = r300SetupFragmentShaderTextures;
	}
}

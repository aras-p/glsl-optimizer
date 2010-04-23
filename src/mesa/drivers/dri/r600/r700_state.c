/*
 * Copyright (C) 2008-2009  Advanced Micro Devices, Inc.
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
 * THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Authors:
 *   Richard Li <RichardZ.Li@amd.com>, <richardradeon@gmail.com>
 */

#include "main/glheader.h"
#include "main/mtypes.h"
#include "main/imports.h"
#include "main/enums.h"
#include "main/macros.h"
#include "main/context.h"
#include "main/dd.h"
#include "main/simple_list.h"

#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "main/api_arrayelt.h"
#include "main/framebuffer.h"

#include "shader/prog_parameter.h"
#include "shader/prog_statevars.h"
#include "vbo/vbo.h"

#include "r600_context.h"

#include "r700_state.h"

#include "r700_fragprog.h"
#include "r700_vertprog.h"

void r600UpdateTextureState(GLcontext * ctx);
static void r700SetClipPlaneState(GLcontext * ctx, GLenum cap, GLboolean state);
static void r700UpdatePolygonMode(GLcontext * ctx);
static void r700SetPolygonOffsetState(GLcontext * ctx, GLboolean state);
static void r700SetStencilState(GLcontext * ctx, GLboolean state);
static void r700UpdateWindow(GLcontext * ctx, int id);

void r700UpdateShaders(GLcontext * ctx)
{
    context_t *context = R700_CONTEXT(ctx);

    /* should only happenen once, just after context is created */
    /* TODO: shouldn't we fallback to sw here? */
    if (!ctx->FragmentProgram._Current) {
	    fprintf(stderr, "No ctx->FragmentProgram._Current!!\n");
	    return;
    }

    r700SelectFragmentShader(ctx);

    r700SelectVertexShader(ctx);
    r700UpdateStateParameters(ctx, _NEW_PROGRAM | _NEW_PROGRAM_CONSTANTS);
    context->radeon.NewGLState = 0;
}

/*
 * To correctly position primitives:
 */
void r700UpdateViewportOffset(GLcontext * ctx) //------------------
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(&context->hw);
	__DRIdrawable *dPriv = radeon_get_drawable(&context->radeon);
	GLfloat xoffset = (GLfloat) dPriv->x;
	GLfloat yoffset = (GLfloat) dPriv->y + dPriv->h;
	const GLfloat *v = ctx->Viewport._WindowMap.m;
	int id = 0;

	GLfloat tx = v[MAT_TX] + xoffset;
	GLfloat ty = (-v[MAT_TY]) + yoffset;

	if (r700->viewport[id].PA_CL_VPORT_XOFFSET.f32All != tx ||
	    r700->viewport[id].PA_CL_VPORT_YOFFSET.f32All != ty) {
		/* Note: this should also modify whatever data the context reset
		 * code uses...
		 */
		R600_STATECHANGE(context, vpt);
		r700->viewport[id].PA_CL_VPORT_XOFFSET.f32All = tx;
		r700->viewport[id].PA_CL_VPORT_YOFFSET.f32All = ty;
	}

	radeonUpdateScissor(ctx);
}

void r700UpdateStateParameters(GLcontext * ctx, GLuint new_state) //--------------------
{
	struct r700_fragment_program *fp =
		(struct r700_fragment_program *)ctx->FragmentProgram._Current;
	struct gl_program_parameter_list *paramList;

	if (!(new_state & (_NEW_BUFFERS | _NEW_PROGRAM | _NEW_PROGRAM_CONSTANTS)))
		return;

	if (!ctx->FragmentProgram._Current || !fp)
		return;

	paramList = ctx->FragmentProgram._Current->Base.Parameters;

	if (!paramList)
		return;

	_mesa_load_state_parameters(ctx, paramList);

}

/**
 * Called by Mesa after an internal state update.
 */
static void r700InvalidateState(GLcontext * ctx, GLuint new_state) //-------------------
{
    context_t *context = R700_CONTEXT(ctx);

    R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(&context->hw);

    _swrast_InvalidateState(ctx, new_state);
    _swsetup_InvalidateState(ctx, new_state);
    _vbo_InvalidateState(ctx, new_state);
    _tnl_InvalidateState(ctx, new_state);
    _ae_invalidate_state(ctx, new_state);

    if (new_state & _NEW_BUFFERS) {
	    _mesa_update_framebuffer(ctx);
	    /* this updates the DrawBuffer's Width/Height if it's a FBO */
	    _mesa_update_draw_buffer_bounds(ctx);

	    R600_STATECHANGE(context, cb_target);
	    R600_STATECHANGE(context, db_target);
    }

    if (new_state & (_NEW_LIGHT)) {
	    R600_STATECHANGE(context, su);
	    if (ctx->Light.ProvokingVertex == GL_LAST_VERTEX_CONVENTION)
		    SETbit(r700->PA_SU_SC_MODE_CNTL.u32All, PROVOKING_VTX_LAST_bit);
	    else
		    CLEARbit(r700->PA_SU_SC_MODE_CNTL.u32All, PROVOKING_VTX_LAST_bit);
    }

    r700UpdateStateParameters(ctx, new_state);

    R600_STATECHANGE(context, cl);
    R600_STATECHANGE(context, spi);

    if(GL_TRUE == r700->bEnablePerspective)
    {
        /* Do scale XY and Z by 1/W0 for perspective correction on pos. For orthogonal case, set both to one. */
        CLEARbit(r700->PA_CL_VTE_CNTL.u32All, VTX_XY_FMT_bit);
        CLEARbit(r700->PA_CL_VTE_CNTL.u32All, VTX_Z_FMT_bit);

        SETbit(r700->PA_CL_VTE_CNTL.u32All, VTX_W0_FMT_bit);

        SETbit(r700->SPI_PS_IN_CONTROL_0.u32All, PERSP_GRADIENT_ENA_bit);
        CLEARbit(r700->SPI_PS_IN_CONTROL_0.u32All, LINEAR_GRADIENT_ENA_bit);
    }
    else
    {
        /* For orthogonal case. */
        SETbit(r700->PA_CL_VTE_CNTL.u32All, VTX_XY_FMT_bit);
        SETbit(r700->PA_CL_VTE_CNTL.u32All, VTX_Z_FMT_bit);

        SETbit(r700->PA_CL_VTE_CNTL.u32All, VTX_W0_FMT_bit);

        CLEARbit(r700->SPI_PS_IN_CONTROL_0.u32All, PERSP_GRADIENT_ENA_bit);
        SETbit(r700->SPI_PS_IN_CONTROL_0.u32All, LINEAR_GRADIENT_ENA_bit);
    }

    context->radeon.NewGLState |= new_state;
}

static void r700SetDBRenderState(GLcontext * ctx)
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(&context->hw);
	struct r700_fragment_program *fp = (struct r700_fragment_program *)
		(ctx->FragmentProgram._Current);

	R600_STATECHANGE(context, db);

	SETbit(r700->DB_SHADER_CONTROL.u32All, DUAL_EXPORT_ENABLE_bit);
	SETfield(r700->DB_SHADER_CONTROL.u32All, EARLY_Z_THEN_LATE_Z, Z_ORDER_shift, Z_ORDER_mask);
	/* XXX need to enable htile for hiz/s */
	SETfield(r700->DB_RENDER_OVERRIDE.u32All, FORCE_DISABLE, FORCE_HIZ_ENABLE_shift, FORCE_HIZ_ENABLE_mask);
	SETfield(r700->DB_RENDER_OVERRIDE.u32All, FORCE_DISABLE, FORCE_HIS_ENABLE0_shift, FORCE_HIS_ENABLE0_mask);
	SETfield(r700->DB_RENDER_OVERRIDE.u32All, FORCE_DISABLE, FORCE_HIS_ENABLE1_shift, FORCE_HIS_ENABLE1_mask);

	if (context->radeon.query.current)
	{
		SETbit(r700->DB_RENDER_OVERRIDE.u32All, NOOP_CULL_DISABLE_bit);
		if (context->radeon.radeonScreen->chip_family >= CHIP_FAMILY_RV770)
		{
			SETbit(r700->DB_RENDER_CONTROL.u32All, PERFECT_ZPASS_COUNTS_bit);
		}
	}
	else
	{
		CLEARbit(r700->DB_RENDER_OVERRIDE.u32All, NOOP_CULL_DISABLE_bit);
		if (context->radeon.radeonScreen->chip_family >= CHIP_FAMILY_RV770)
		{
			CLEARbit(r700->DB_RENDER_CONTROL.u32All, PERFECT_ZPASS_COUNTS_bit);
		}
	}

	if (fp)
	{
		if (fp->r700Shader.killIsUsed)
		{
			SETbit(r700->DB_SHADER_CONTROL.u32All, KILL_ENABLE_bit);
		}
		else
		{
			CLEARbit(r700->DB_SHADER_CONTROL.u32All, KILL_ENABLE_bit);
		}

		if (fp->r700Shader.depthIsExported)
		{
			SETbit(r700->DB_SHADER_CONTROL.u32All, Z_EXPORT_ENABLE_bit);
		}
		else
		{
			CLEARbit(r700->DB_SHADER_CONTROL.u32All, Z_EXPORT_ENABLE_bit);
		}
	}
}

void r700UpdateShaderStates(GLcontext * ctx)
{
	r700SetDBRenderState(ctx);
	r600UpdateTextureState(ctx);
}

static void r700SetDepthState(GLcontext * ctx)
{
	struct radeon_renderbuffer *rrb;
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(&context->hw);

	R600_STATECHANGE(context, db);

	rrb = radeon_get_depthbuffer(&context->radeon);

    if (ctx->Depth.Test && rrb && rrb->bo)
    {
        SETbit(r700->DB_DEPTH_CONTROL.u32All, Z_ENABLE_bit);
        if (ctx->Depth.Mask)
        {
            SETbit(r700->DB_DEPTH_CONTROL.u32All, Z_WRITE_ENABLE_bit);
        }
        else
        {
            CLEARbit(r700->DB_DEPTH_CONTROL.u32All, Z_WRITE_ENABLE_bit);
        }

        switch (ctx->Depth.Func)
        {
        case GL_NEVER:
            SETfield(r700->DB_DEPTH_CONTROL.u32All, FRAG_NEVER,
                     ZFUNC_shift, ZFUNC_mask);
            break;
        case GL_LESS:
            SETfield(r700->DB_DEPTH_CONTROL.u32All, FRAG_LESS,
                     ZFUNC_shift, ZFUNC_mask);
            break;
        case GL_EQUAL:
            SETfield(r700->DB_DEPTH_CONTROL.u32All, FRAG_EQUAL,
                     ZFUNC_shift, ZFUNC_mask);
            break;
        case GL_LEQUAL:
            SETfield(r700->DB_DEPTH_CONTROL.u32All, FRAG_LEQUAL,
                     ZFUNC_shift, ZFUNC_mask);
            break;
        case GL_GREATER:
            SETfield(r700->DB_DEPTH_CONTROL.u32All, FRAG_GREATER,
                     ZFUNC_shift, ZFUNC_mask);
            break;
        case GL_NOTEQUAL:
            SETfield(r700->DB_DEPTH_CONTROL.u32All, FRAG_NOTEQUAL,
                     ZFUNC_shift, ZFUNC_mask);
            break;
        case GL_GEQUAL:
            SETfield(r700->DB_DEPTH_CONTROL.u32All, FRAG_GEQUAL,
                     ZFUNC_shift, ZFUNC_mask);
            break;
        case GL_ALWAYS:
            SETfield(r700->DB_DEPTH_CONTROL.u32All, FRAG_ALWAYS,
                     ZFUNC_shift, ZFUNC_mask);
            break;
        default:
            SETfield(r700->DB_DEPTH_CONTROL.u32All, FRAG_ALWAYS,
                     ZFUNC_shift, ZFUNC_mask);
            break;
        }
    }
    else
    {
        CLEARbit(r700->DB_DEPTH_CONTROL.u32All, Z_ENABLE_bit);
        CLEARbit(r700->DB_DEPTH_CONTROL.u32All, Z_WRITE_ENABLE_bit);
    }
}

static void r700SetAlphaState(GLcontext * ctx)
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(&context->hw);
	uint32_t alpha_func = REF_ALWAYS;
	GLboolean really_enabled = ctx->Color.AlphaEnabled;

	R600_STATECHANGE(context, sx);

	switch (ctx->Color.AlphaFunc) {
	case GL_NEVER:
		alpha_func = REF_NEVER;
		break;
	case GL_LESS:
		alpha_func = REF_LESS;
		break;
	case GL_EQUAL:
		alpha_func = REF_EQUAL;
		break;
	case GL_LEQUAL:
		alpha_func = REF_LEQUAL;
		break;
	case GL_GREATER:
		alpha_func = REF_GREATER;
		break;
	case GL_NOTEQUAL:
		alpha_func = REF_NOTEQUAL;
		break;
	case GL_GEQUAL:
		alpha_func = REF_GEQUAL;
		break;
	case GL_ALWAYS:
		/*alpha_func = REF_ALWAYS; */
		really_enabled = GL_FALSE;
		break;
	}

	if (really_enabled) {
		SETfield(r700->SX_ALPHA_TEST_CONTROL.u32All, alpha_func,
			 ALPHA_FUNC_shift, ALPHA_FUNC_mask);
		SETbit(r700->SX_ALPHA_TEST_CONTROL.u32All, ALPHA_TEST_ENABLE_bit);
		r700->SX_ALPHA_REF.f32All = ctx->Color.AlphaRef;
	} else {
		CLEARbit(r700->SX_ALPHA_TEST_CONTROL.u32All, ALPHA_TEST_ENABLE_bit);
	}

}

static void r700AlphaFunc(GLcontext * ctx, GLenum func, GLfloat ref) //---------------
{
	(void)func;
	(void)ref;
	r700SetAlphaState(ctx);
}


static void r700BlendColor(GLcontext * ctx, const GLfloat cf[4]) //----------------
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(&context->hw);

	R600_STATECHANGE(context, blnd_clr);

	r700->CB_BLEND_RED.f32All = cf[0];
	r700->CB_BLEND_GREEN.f32All = cf[1];
	r700->CB_BLEND_BLUE.f32All = cf[2];
	r700->CB_BLEND_ALPHA.f32All = cf[3];
}

static int blend_factor(GLenum factor, GLboolean is_src)
{
	switch (factor) {
	case GL_ZERO:
		return BLEND_ZERO;
		break;
	case GL_ONE:
		return BLEND_ONE;
		break;
	case GL_DST_COLOR:
		return BLEND_DST_COLOR;
		break;
	case GL_ONE_MINUS_DST_COLOR:
		return BLEND_ONE_MINUS_DST_COLOR;
		break;
	case GL_SRC_COLOR:
		return BLEND_SRC_COLOR;
		break;
	case GL_ONE_MINUS_SRC_COLOR:
		return BLEND_ONE_MINUS_SRC_COLOR;
		break;
	case GL_SRC_ALPHA:
		return BLEND_SRC_ALPHA;
		break;
	case GL_ONE_MINUS_SRC_ALPHA:
		return BLEND_ONE_MINUS_SRC_ALPHA;
		break;
	case GL_DST_ALPHA:
		return BLEND_DST_ALPHA;
		break;
	case GL_ONE_MINUS_DST_ALPHA:
		return BLEND_ONE_MINUS_DST_ALPHA;
		break;
	case GL_SRC_ALPHA_SATURATE:
		return (is_src) ? BLEND_SRC_ALPHA_SATURATE : BLEND_ZERO;
		break;
	case GL_CONSTANT_COLOR:
		return BLEND_CONSTANT_COLOR;
		break;
	case GL_ONE_MINUS_CONSTANT_COLOR:
		return BLEND_ONE_MINUS_CONSTANT_COLOR;
		break;
	case GL_CONSTANT_ALPHA:
		return BLEND_CONSTANT_ALPHA;
		break;
	case GL_ONE_MINUS_CONSTANT_ALPHA:
		return BLEND_ONE_MINUS_CONSTANT_ALPHA;
		break;
	default:
		fprintf(stderr, "unknown blend factor %x\n", factor);
		return (is_src) ? BLEND_ONE : BLEND_ZERO;
		break;
	}
}

static void r700SetBlendState(GLcontext * ctx)
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(&context->hw);
	int id = 0;
	uint32_t blend_reg = 0, eqn, eqnA;

	R600_STATECHANGE(context, blnd);

	if (RGBA_LOGICOP_ENABLED(ctx) || !ctx->Color.BlendEnabled) {
		SETfield(blend_reg,
			 BLEND_ONE, COLOR_SRCBLEND_shift, COLOR_SRCBLEND_mask);
		SETfield(blend_reg,
			 BLEND_ZERO, COLOR_DESTBLEND_shift, COLOR_DESTBLEND_mask);
		SETfield(blend_reg,
			 COMB_DST_PLUS_SRC, COLOR_COMB_FCN_shift, COLOR_COMB_FCN_mask);
		SETfield(blend_reg,
			 BLEND_ONE, ALPHA_SRCBLEND_shift, ALPHA_SRCBLEND_mask);
		SETfield(blend_reg,
			 BLEND_ZERO, ALPHA_DESTBLEND_shift, ALPHA_DESTBLEND_mask);
		SETfield(blend_reg,
			 COMB_DST_PLUS_SRC, ALPHA_COMB_FCN_shift, ALPHA_COMB_FCN_mask);
		if (context->radeon.radeonScreen->chip_family == CHIP_FAMILY_R600)
			r700->CB_BLEND_CONTROL.u32All = blend_reg;
		else
			r700->render_target[id].CB_BLEND0_CONTROL.u32All = blend_reg;
		return;
	}

	SETfield(blend_reg,
		 blend_factor(ctx->Color.BlendSrcRGB, GL_TRUE),
		 COLOR_SRCBLEND_shift, COLOR_SRCBLEND_mask);
	SETfield(blend_reg,
		 blend_factor(ctx->Color.BlendDstRGB, GL_FALSE),
		 COLOR_DESTBLEND_shift, COLOR_DESTBLEND_mask);

	switch (ctx->Color.BlendEquationRGB) {
	case GL_FUNC_ADD:
		eqn = COMB_DST_PLUS_SRC;
		break;
	case GL_FUNC_SUBTRACT:
		eqn = COMB_SRC_MINUS_DST;
		break;
	case GL_FUNC_REVERSE_SUBTRACT:
		eqn = COMB_DST_MINUS_SRC;
		break;
	case GL_MIN:
		eqn = COMB_MIN_DST_SRC;
		SETfield(blend_reg,
			 BLEND_ONE,
			 COLOR_SRCBLEND_shift, COLOR_SRCBLEND_mask);
		SETfield(blend_reg,
			 BLEND_ONE,
			 COLOR_DESTBLEND_shift, COLOR_DESTBLEND_mask);
		break;
	case GL_MAX:
		eqn = COMB_MAX_DST_SRC;
		SETfield(blend_reg,
			 BLEND_ONE,
			 COLOR_SRCBLEND_shift, COLOR_SRCBLEND_mask);
		SETfield(blend_reg,
			 BLEND_ONE,
			 COLOR_DESTBLEND_shift, COLOR_DESTBLEND_mask);
		break;

	default:
		fprintf(stderr,
			"[%s:%u] Invalid RGB blend equation (0x%04x).\n",
			__FUNCTION__, __LINE__, ctx->Color.BlendEquationRGB);
		return;
	}
	SETfield(blend_reg,
		 eqn, COLOR_COMB_FCN_shift, COLOR_COMB_FCN_mask);

	SETfield(blend_reg,
		 blend_factor(ctx->Color.BlendSrcA, GL_TRUE),
		 ALPHA_SRCBLEND_shift, ALPHA_SRCBLEND_mask);
	SETfield(blend_reg,
		 blend_factor(ctx->Color.BlendDstA, GL_FALSE),
		 ALPHA_DESTBLEND_shift, ALPHA_DESTBLEND_mask);

	switch (ctx->Color.BlendEquationA) {
	case GL_FUNC_ADD:
		eqnA = COMB_DST_PLUS_SRC;
		break;
	case GL_FUNC_SUBTRACT:
		eqnA = COMB_SRC_MINUS_DST;
		break;
	case GL_FUNC_REVERSE_SUBTRACT:
		eqnA = COMB_DST_MINUS_SRC;
		break;
	case GL_MIN:
		eqnA = COMB_MIN_DST_SRC;
		SETfield(blend_reg,
			 BLEND_ONE,
			 ALPHA_SRCBLEND_shift, ALPHA_SRCBLEND_mask);
		SETfield(blend_reg,
			 BLEND_ONE,
			 ALPHA_DESTBLEND_shift, ALPHA_DESTBLEND_mask);
		break;
	case GL_MAX:
		eqnA = COMB_MAX_DST_SRC;
		SETfield(blend_reg,
			 BLEND_ONE,
			 ALPHA_SRCBLEND_shift, ALPHA_SRCBLEND_mask);
		SETfield(blend_reg,
			 BLEND_ONE,
			 ALPHA_DESTBLEND_shift, ALPHA_DESTBLEND_mask);
		break;
	default:
		fprintf(stderr,
			"[%s:%u] Invalid A blend equation (0x%04x).\n",
			__FUNCTION__, __LINE__, ctx->Color.BlendEquationA);
		return;
	}

	SETfield(blend_reg,
		 eqnA, ALPHA_COMB_FCN_shift, ALPHA_COMB_FCN_mask);

	SETbit(blend_reg, SEPARATE_ALPHA_BLEND_bit);

	if (context->radeon.radeonScreen->chip_family == CHIP_FAMILY_R600)
		r700->CB_BLEND_CONTROL.u32All = blend_reg;
	else {
		r700->render_target[id].CB_BLEND0_CONTROL.u32All = blend_reg;
		SETbit(r700->CB_COLOR_CONTROL.u32All, PER_MRT_BLEND_bit);
	}
	SETfield(r700->CB_COLOR_CONTROL.u32All, (1 << id),
		 TARGET_BLEND_ENABLE_shift, TARGET_BLEND_ENABLE_mask);

}

static void r700BlendEquationSeparate(GLcontext * ctx,
				                      GLenum modeRGB, GLenum modeA) //-----------------
{
	r700SetBlendState(ctx);
}

static void r700BlendFuncSeparate(GLcontext * ctx,
				  GLenum sfactorRGB, GLenum dfactorRGB,
				  GLenum sfactorA, GLenum dfactorA) //------------------------
{
	r700SetBlendState(ctx);
}

/**
 * Translate LogicOp enums into hardware representation.
 */
static GLuint translate_logicop(GLenum logicop)
{
	switch (logicop) {
	case GL_CLEAR:
		return 0x00;
	case GL_SET:
		return 0xff;
	case GL_COPY:
		return 0xcc;
	case GL_COPY_INVERTED:
		return 0x33;
	case GL_NOOP:
		return 0xaa;
	case GL_INVERT:
		return 0x55;
	case GL_AND:
		return 0x88;
	case GL_NAND:
		return 0x77;
	case GL_OR:
		return 0xee;
	case GL_NOR:
		return 0x11;
	case GL_XOR:
		return 0x66;
	case GL_EQUIV:
		return 0x99;
	case GL_AND_REVERSE:
		return 0x44;
	case GL_AND_INVERTED:
		return 0x22;
	case GL_OR_REVERSE:
		return 0xdd;
	case GL_OR_INVERTED:
		return 0xbb;
	default:
		fprintf(stderr, "unknown blend logic operation %x\n", logicop);
		return 0xcc;
	}
}

/**
 * Used internally to update the r300->hw hardware state to match the
 * current OpenGL state.
 */
static void r700SetLogicOpState(GLcontext *ctx)
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(&R700_CONTEXT(ctx)->hw);

	R600_STATECHANGE(context, blnd);

	if (RGBA_LOGICOP_ENABLED(ctx))
		SETfield(r700->CB_COLOR_CONTROL.u32All,
			 translate_logicop(ctx->Color.LogicOp), ROP3_shift, ROP3_mask);
	else
		SETfield(r700->CB_COLOR_CONTROL.u32All, 0xCC, ROP3_shift, ROP3_mask);
}

/**
 * Called by Mesa when an application program changes the LogicOp state
 * via glLogicOp.
 */
static void r700LogicOpcode(GLcontext *ctx, GLenum logicop)
{
	if (RGBA_LOGICOP_ENABLED(ctx))
		r700SetLogicOpState(ctx);
}

static void r700UpdateCulling(GLcontext * ctx)
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(&R700_CONTEXT(ctx)->hw);

    R600_STATECHANGE(context, su);

    CLEARbit(r700->PA_SU_SC_MODE_CNTL.u32All, FACE_bit);
    CLEARbit(r700->PA_SU_SC_MODE_CNTL.u32All, CULL_FRONT_bit);
    CLEARbit(r700->PA_SU_SC_MODE_CNTL.u32All, CULL_BACK_bit);

    if (ctx->Polygon.CullFlag)
    {
        switch (ctx->Polygon.CullFaceMode)
        {
        case GL_FRONT:
            SETbit(r700->PA_SU_SC_MODE_CNTL.u32All, CULL_FRONT_bit);
            CLEARbit(r700->PA_SU_SC_MODE_CNTL.u32All, CULL_BACK_bit);
            break;
        case GL_BACK:
            CLEARbit(r700->PA_SU_SC_MODE_CNTL.u32All, CULL_FRONT_bit);
            SETbit(r700->PA_SU_SC_MODE_CNTL.u32All, CULL_BACK_bit);
            break;
        case GL_FRONT_AND_BACK:
            SETbit(r700->PA_SU_SC_MODE_CNTL.u32All, CULL_FRONT_bit);
            SETbit(r700->PA_SU_SC_MODE_CNTL.u32All, CULL_BACK_bit);
            break;
        default:
            CLEARbit(r700->PA_SU_SC_MODE_CNTL.u32All, CULL_FRONT_bit);
            CLEARbit(r700->PA_SU_SC_MODE_CNTL.u32All, CULL_BACK_bit);
            break;
        }
    }

    switch (ctx->Polygon.FrontFace)
    {
        case GL_CW:
            SETbit(r700->PA_SU_SC_MODE_CNTL.u32All, FACE_bit);
            break;
        case GL_CCW:
            CLEARbit(r700->PA_SU_SC_MODE_CNTL.u32All, FACE_bit);
            break;
        default:
            CLEARbit(r700->PA_SU_SC_MODE_CNTL.u32All, FACE_bit); /* default: ccw */
            break;
    }

    /* Winding is inverted when rendering to FBO */
    if (ctx->DrawBuffer && ctx->DrawBuffer->Name)
	    r700->PA_SU_SC_MODE_CNTL.u32All ^= FACE_bit;
}

static void r700UpdateLineStipple(GLcontext * ctx)
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(&R700_CONTEXT(ctx)->hw);

    R600_STATECHANGE(context, sc);

    if (ctx->Line.StippleFlag)
    {
	SETbit(r700->PA_SC_MODE_CNTL.u32All, LINE_STIPPLE_ENABLE_bit);
    }
    else
    {
	CLEARbit(r700->PA_SC_MODE_CNTL.u32All, LINE_STIPPLE_ENABLE_bit);
    }
}

static void r700Enable(GLcontext * ctx, GLenum cap, GLboolean state) //------------------
{
	context_t *context = R700_CONTEXT(ctx);

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
		r700SetAlphaState(ctx);
		break;
	case GL_COLOR_LOGIC_OP:
		r700SetLogicOpState(ctx);
		/* fall-through, because logic op overrides blending */
	case GL_BLEND:
		r700SetBlendState(ctx);
		break;
	case GL_CLIP_PLANE0:
	case GL_CLIP_PLANE1:
	case GL_CLIP_PLANE2:
	case GL_CLIP_PLANE3:
	case GL_CLIP_PLANE4:
	case GL_CLIP_PLANE5:
		r700SetClipPlaneState(ctx, cap, state);
		break;
	case GL_DEPTH_TEST:
		r700SetDepthState(ctx);
		break;
	case GL_STENCIL_TEST:
		r700SetStencilState(ctx, state);
		break;
	case GL_CULL_FACE:
		r700UpdateCulling(ctx);
		break;
	case GL_POLYGON_OFFSET_POINT:
	case GL_POLYGON_OFFSET_LINE:
	case GL_POLYGON_OFFSET_FILL:
		r700SetPolygonOffsetState(ctx, state);
		break;
	case GL_SCISSOR_TEST:
		radeon_firevertices(&context->radeon);
		context->radeon.state.scissor.enabled = state;
		radeonUpdateScissor(ctx);
		break;
	case GL_LINE_STIPPLE:
		r700UpdateLineStipple(ctx);
		break;
	case GL_DEPTH_CLAMP:
		r700UpdateWindow(ctx, 0);
		break;
	default:
		break;
	}

}

/**
 * Handle glColorMask()
 */
static void r700ColorMask(GLcontext * ctx,
			  GLboolean r, GLboolean g, GLboolean b, GLboolean a) //------------------
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(&R700_CONTEXT(ctx)->hw);
	unsigned int mask = ((r ? 1 : 0) |
			     (g ? 2 : 0) |
			     (b ? 4 : 0) |
			     (a ? 8 : 0));

	if (mask != r700->CB_TARGET_MASK.u32All) {
		R600_STATECHANGE(context, cb);
		SETfield(r700->CB_TARGET_MASK.u32All, mask, TARGET0_ENABLE_shift, TARGET0_ENABLE_mask);
	}
}

/**
 * Change the depth testing function.
 *
 * \note Mesa already filters redundant calls to this function.
 */
static void r700DepthFunc(GLcontext * ctx, GLenum func) //--------------------
{
    r700SetDepthState(ctx);
}

/**
 * Enable/Disable depth writing.
 *
 * \note Mesa already filters redundant calls to this function.
 */
static void r700DepthMask(GLcontext * ctx, GLboolean mask) //------------------
{
    r700SetDepthState(ctx);
}

/**
 * Change the culling mode.
 *
 * \note Mesa already filters redundant calls to this function.
 */
static void r700CullFace(GLcontext * ctx, GLenum mode) //-----------------
{
    r700UpdateCulling(ctx);
}

/* =============================================================
 * Fog
 */
static void r700Fogfv(GLcontext * ctx, GLenum pname, const GLfloat * param) //--------------
{
}

/**
 * Change the polygon orientation.
 *
 * \note Mesa already filters redundant calls to this function.
 */
static void r700FrontFace(GLcontext * ctx, GLenum mode) //------------------
{
    r700UpdateCulling(ctx);
    r700UpdatePolygonMode(ctx);
}

static void r700ShadeModel(GLcontext * ctx, GLenum mode) //--------------------
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(&context->hw);

	R600_STATECHANGE(context, spi);

	/* also need to set/clear FLAT_SHADE bit per param in SPI_PS_INPUT_CNTL_[0-31] */
	switch (mode) {
	case GL_FLAT:
		SETbit(r700->SPI_INTERP_CONTROL_0.u32All, FLAT_SHADE_ENA_bit);
		break;
	case GL_SMOOTH:
		CLEARbit(r700->SPI_INTERP_CONTROL_0.u32All, FLAT_SHADE_ENA_bit);
		break;
	default:
		return;
	}
}

/* =============================================================
 * Point state
 */
static void r700PointSize(GLcontext * ctx, GLfloat size)
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(&context->hw);

	R600_STATECHANGE(context, su);

	/* We need to clamp to user defined range here, because
	 * the HW clamping happens only for per vertex point size. */
	size = CLAMP(size, ctx->Point.MinSize, ctx->Point.MaxSize);

	/* same size limits for AA, non-AA points */
	size = CLAMP(size, ctx->Const.MinPointSize, ctx->Const.MaxPointSize);

	/* format is 12.4 fixed point */
	SETfield(r700->PA_SU_POINT_SIZE.u32All, (int)(size * 8.0),
		 PA_SU_POINT_SIZE__HEIGHT_shift, PA_SU_POINT_SIZE__HEIGHT_mask);
	SETfield(r700->PA_SU_POINT_SIZE.u32All, (int)(size * 8.0),
		 PA_SU_POINT_SIZE__WIDTH_shift, PA_SU_POINT_SIZE__WIDTH_mask);

}

static void r700PointParameter(GLcontext * ctx, GLenum pname, const GLfloat * param) //---------------
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(&context->hw);

	R600_STATECHANGE(context, su);

	/* format is 12.4 fixed point */
	switch (pname) {
	case GL_POINT_SIZE_MIN:
		SETfield(r700->PA_SU_POINT_MINMAX.u32All, (int)(ctx->Point.MinSize * 8.0),
			 MIN_SIZE_shift, MIN_SIZE_mask);
		r700PointSize(ctx, ctx->Point.Size);
		break;
	case GL_POINT_SIZE_MAX:
		SETfield(r700->PA_SU_POINT_MINMAX.u32All, (int)(ctx->Point.MaxSize * 8.0),
			 MAX_SIZE_shift, MAX_SIZE_mask);
		r700PointSize(ctx, ctx->Point.Size);
		break;
	case GL_POINT_DISTANCE_ATTENUATION:
		break;
	case GL_POINT_FADE_THRESHOLD_SIZE:
		break;
	default:
		break;
	}
}

static int translate_stencil_func(int func)
{
	switch (func) {
	case GL_NEVER:
		return REF_NEVER;
	case GL_LESS:
		return REF_LESS;
	case GL_EQUAL:
		return REF_EQUAL;
	case GL_LEQUAL:
		return REF_LEQUAL;
	case GL_GREATER:
		return REF_GREATER;
	case GL_NOTEQUAL:
		return REF_NOTEQUAL;
	case GL_GEQUAL:
		return REF_GEQUAL;
	case GL_ALWAYS:
		return REF_ALWAYS;
	}
	return 0;
}

static int translate_stencil_op(int op)
{
	switch (op) {
	case GL_KEEP:
		return STENCIL_KEEP;
	case GL_ZERO:
		return STENCIL_ZERO;
	case GL_REPLACE:
		return STENCIL_REPLACE;
	case GL_INCR:
		return STENCIL_INCR_CLAMP;
	case GL_DECR:
		return STENCIL_DECR_CLAMP;
	case GL_INCR_WRAP_EXT:
		return STENCIL_INCR_WRAP;
	case GL_DECR_WRAP_EXT:
		return STENCIL_DECR_WRAP;
	case GL_INVERT:
		return STENCIL_INVERT;
	default:
		WARN_ONCE("Do not know how to translate stencil op");
		return STENCIL_KEEP;
	}
	return 0;
}

static void r700SetStencilState(GLcontext * ctx, GLboolean state)
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(&context->hw);
	GLboolean hw_stencil = GL_FALSE;

	if (ctx->DrawBuffer) {
		struct radeon_renderbuffer *rrbStencil
			= radeon_get_renderbuffer(ctx->DrawBuffer, BUFFER_STENCIL);
		hw_stencil = (rrbStencil && rrbStencil->bo);
	}

	if (hw_stencil) {
		R600_STATECHANGE(context, db);
		if (state) {
			SETbit(r700->DB_DEPTH_CONTROL.u32All, STENCIL_ENABLE_bit);
			SETbit(r700->DB_DEPTH_CONTROL.u32All, BACKFACE_ENABLE_bit);
		} else
			CLEARbit(r700->DB_DEPTH_CONTROL.u32All, STENCIL_ENABLE_bit);
	}
}

static void r700StencilFuncSeparate(GLcontext * ctx, GLenum face,
				    GLenum func, GLint ref, GLuint mask) //---------------------
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(&context->hw);
	const unsigned back = ctx->Stencil._BackFace;

	R600_STATECHANGE(context, stencil);
	R600_STATECHANGE(context, db);

	//front
	SETfield(r700->DB_STENCILREFMASK.u32All, ctx->Stencil.Ref[0],
		 STENCILREF_shift, STENCILREF_mask);
	SETfield(r700->DB_STENCILREFMASK.u32All, ctx->Stencil.ValueMask[0],
		 STENCILMASK_shift, STENCILMASK_mask);

	SETfield(r700->DB_DEPTH_CONTROL.u32All, translate_stencil_func(ctx->Stencil.Function[0]),
		 STENCILFUNC_shift, STENCILFUNC_mask);

	//back
	SETfield(r700->DB_STENCILREFMASK_BF.u32All, ctx->Stencil.Ref[back],
		 STENCILREF_BF_shift, STENCILREF_BF_mask);
	SETfield(r700->DB_STENCILREFMASK_BF.u32All, ctx->Stencil.ValueMask[back],
		 STENCILMASK_BF_shift, STENCILMASK_BF_mask);

	SETfield(r700->DB_DEPTH_CONTROL.u32All, translate_stencil_func(ctx->Stencil.Function[back]),
		 STENCILFUNC_BF_shift, STENCILFUNC_BF_mask);

}

static void r700StencilMaskSeparate(GLcontext * ctx, GLenum face, GLuint mask) //--------------
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(&context->hw);
	const unsigned back = ctx->Stencil._BackFace;

	R600_STATECHANGE(context, stencil);

	// front
	SETfield(r700->DB_STENCILREFMASK.u32All, ctx->Stencil.WriteMask[0],
		 STENCILWRITEMASK_shift, STENCILWRITEMASK_mask);

	// back
	SETfield(r700->DB_STENCILREFMASK_BF.u32All, ctx->Stencil.WriteMask[back],
		 STENCILWRITEMASK_BF_shift, STENCILWRITEMASK_BF_mask);

}

static void r700StencilOpSeparate(GLcontext * ctx, GLenum face,
				  GLenum fail, GLenum zfail, GLenum zpass) //--------------------
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(&context->hw);
	const unsigned back = ctx->Stencil._BackFace;

	R600_STATECHANGE(context, db);

	SETfield(r700->DB_DEPTH_CONTROL.u32All, translate_stencil_op(ctx->Stencil.FailFunc[0]),
		 STENCILFAIL_shift, STENCILFAIL_mask);
	SETfield(r700->DB_DEPTH_CONTROL.u32All, translate_stencil_op(ctx->Stencil.ZFailFunc[0]),
		 STENCILZFAIL_shift, STENCILZFAIL_mask);
	SETfield(r700->DB_DEPTH_CONTROL.u32All, translate_stencil_op(ctx->Stencil.ZPassFunc[0]),
		 STENCILZPASS_shift, STENCILZPASS_mask);

	SETfield(r700->DB_DEPTH_CONTROL.u32All, translate_stencil_op(ctx->Stencil.FailFunc[back]),
		 STENCILFAIL_BF_shift, STENCILFAIL_BF_mask);
	SETfield(r700->DB_DEPTH_CONTROL.u32All, translate_stencil_op(ctx->Stencil.ZFailFunc[back]),
		 STENCILZFAIL_BF_shift, STENCILZFAIL_BF_mask);
	SETfield(r700->DB_DEPTH_CONTROL.u32All, translate_stencil_op(ctx->Stencil.ZPassFunc[back]),
		 STENCILZPASS_BF_shift, STENCILZPASS_BF_mask);
}

static void r700UpdateWindow(GLcontext * ctx, int id) //--------------------
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(&context->hw);
	__DRIdrawable *dPriv = radeon_get_drawable(&context->radeon);
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

	R600_STATECHANGE(context, vpt);
	R600_STATECHANGE(context, cl);

	r700->viewport[id].PA_CL_VPORT_XSCALE.f32All  = sx;
	r700->viewport[id].PA_CL_VPORT_XOFFSET.f32All = tx;

	r700->viewport[id].PA_CL_VPORT_YSCALE.f32All  = sy;
	r700->viewport[id].PA_CL_VPORT_YOFFSET.f32All = ty;

	r700->viewport[id].PA_CL_VPORT_ZSCALE.f32All  = sz;
	r700->viewport[id].PA_CL_VPORT_ZOFFSET.f32All = tz;

	if (ctx->Transform.DepthClamp) {
		r700->viewport[id].PA_SC_VPORT_ZMIN_0.f32All = MIN2(ctx->Viewport.Near, ctx->Viewport.Far);
		r700->viewport[id].PA_SC_VPORT_ZMAX_0.f32All = MAX2(ctx->Viewport.Near, ctx->Viewport.Far);
		SETbit(r700->PA_CL_CLIP_CNTL.u32All, ZCLIP_NEAR_DISABLE_bit);
		SETbit(r700->PA_CL_CLIP_CNTL.u32All, ZCLIP_FAR_DISABLE_bit);
	} else {
		r700->viewport[id].PA_SC_VPORT_ZMIN_0.f32All = 0.0;
		r700->viewport[id].PA_SC_VPORT_ZMAX_0.f32All = 1.0;
		CLEARbit(r700->PA_CL_CLIP_CNTL.u32All, ZCLIP_NEAR_DISABLE_bit);
		CLEARbit(r700->PA_CL_CLIP_CNTL.u32All, ZCLIP_FAR_DISABLE_bit);
	}

	r700->viewport[id].enabled = GL_TRUE;

	r700SetScissor(context);
}


static void r700Viewport(GLcontext * ctx,
                         GLint x,
                         GLint y,
			 GLsizei width,
                         GLsizei height) //--------------------
{
	r700UpdateWindow(ctx, 0);

	radeon_viewport(ctx, x, y, width, height);
}

static void r700DepthRange(GLcontext * ctx, GLclampd nearval, GLclampd farval) //-------------
{
	r700UpdateWindow(ctx, 0);
}

static void r700LineWidth(GLcontext * ctx, GLfloat widthf) //---------------
{
    context_t *context = R700_CONTEXT(ctx);
    R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(&context->hw);
    uint32_t lineWidth = (uint32_t)((widthf * 0.5) * (1 << 4));

    R600_STATECHANGE(context, su);

    if (lineWidth > 0xFFFF)
	    lineWidth = 0xFFFF;
    SETfield(r700->PA_SU_LINE_CNTL.u32All,(uint16_t)lineWidth,
	     PA_SU_LINE_CNTL__WIDTH_shift, PA_SU_LINE_CNTL__WIDTH_mask);
}

static void r700LineStipple(GLcontext *ctx, GLint factor, GLushort pattern)
{
    context_t *context = R700_CONTEXT(ctx);
    R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(&context->hw);

    R600_STATECHANGE(context, sc);

    SETfield(r700->PA_SC_LINE_STIPPLE.u32All, pattern, LINE_PATTERN_shift, LINE_PATTERN_mask);
    SETfield(r700->PA_SC_LINE_STIPPLE.u32All, (factor-1), REPEAT_COUNT_shift, REPEAT_COUNT_mask);
    SETfield(r700->PA_SC_LINE_STIPPLE.u32All, 1, AUTO_RESET_CNTL_shift, AUTO_RESET_CNTL_mask);
}

static void r700SetPolygonOffsetState(GLcontext * ctx, GLboolean state)
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(&context->hw);

	R600_STATECHANGE(context, su);

	if (state) {
		SETbit(r700->PA_SU_SC_MODE_CNTL.u32All, POLY_OFFSET_FRONT_ENABLE_bit);
		SETbit(r700->PA_SU_SC_MODE_CNTL.u32All, POLY_OFFSET_BACK_ENABLE_bit);
		SETbit(r700->PA_SU_SC_MODE_CNTL.u32All, POLY_OFFSET_PARA_ENABLE_bit);
	} else {
		CLEARbit(r700->PA_SU_SC_MODE_CNTL.u32All, POLY_OFFSET_FRONT_ENABLE_bit);
		CLEARbit(r700->PA_SU_SC_MODE_CNTL.u32All, POLY_OFFSET_BACK_ENABLE_bit);
		CLEARbit(r700->PA_SU_SC_MODE_CNTL.u32All, POLY_OFFSET_PARA_ENABLE_bit);
	}
}

static void r700PolygonOffset(GLcontext * ctx, GLfloat factor, GLfloat units) //--------------
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(&context->hw);
	GLfloat constant = units;
	GLchar depth = 0;

	R600_STATECHANGE(context, poly);

	switch (ctx->Visual.depthBits) {
	case 16:
		constant *= 4.0;
		depth = -16;
		break;
	case 24:
		constant *= 2.0;
		depth = -24;
		break;
	}

	factor *= 12.0;
	SETfield(r700->PA_SU_POLY_OFFSET_DB_FMT_CNTL.u32All, depth,
		 POLY_OFFSET_NEG_NUM_DB_BITS_shift, POLY_OFFSET_NEG_NUM_DB_BITS_mask);
	//r700->PA_SU_POLY_OFFSET_CLAMP.f32All = constant; //???
	r700->PA_SU_POLY_OFFSET_FRONT_SCALE.f32All = factor;
	r700->PA_SU_POLY_OFFSET_FRONT_OFFSET.f32All = constant;
	r700->PA_SU_POLY_OFFSET_BACK_SCALE.f32All = factor;
	r700->PA_SU_POLY_OFFSET_BACK_OFFSET.f32All = constant;
}

static void r700UpdatePolygonMode(GLcontext * ctx)
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(&context->hw);

	R600_STATECHANGE(context, su);

	SETfield(r700->PA_SU_SC_MODE_CNTL.u32All, X_DISABLE_POLY_MODE, POLY_MODE_shift, POLY_MODE_mask);

	/* Only do something if a polygon mode is wanted, default is GL_FILL */
	if (ctx->Polygon.FrontMode != GL_FILL ||
	    ctx->Polygon.BackMode != GL_FILL) {
		GLenum f, b;

		/* Handle GL_CW (clock wise and GL_CCW (counter clock wise)
		 * correctly by selecting the correct front and back face
		 */
		f = ctx->Polygon.FrontMode;
		b = ctx->Polygon.BackMode;

		/* Enable polygon mode */
		SETfield(r700->PA_SU_SC_MODE_CNTL.u32All, X_DUAL_MODE, POLY_MODE_shift, POLY_MODE_mask);

		switch (f) {
		case GL_LINE:
			SETfield(r700->PA_SU_SC_MODE_CNTL.u32All, X_DRAW_LINES,
				 POLYMODE_FRONT_PTYPE_shift, POLYMODE_FRONT_PTYPE_mask);
			break;
		case GL_POINT:
			SETfield(r700->PA_SU_SC_MODE_CNTL.u32All, X_DRAW_POINTS,
				 POLYMODE_FRONT_PTYPE_shift, POLYMODE_FRONT_PTYPE_mask);
			break;
		case GL_FILL:
			SETfield(r700->PA_SU_SC_MODE_CNTL.u32All, X_DRAW_TRIANGLES,
				 POLYMODE_FRONT_PTYPE_shift, POLYMODE_FRONT_PTYPE_mask);
			break;
		}

		switch (b) {
		case GL_LINE:
			SETfield(r700->PA_SU_SC_MODE_CNTL.u32All, X_DRAW_LINES,
				 POLYMODE_BACK_PTYPE_shift, POLYMODE_BACK_PTYPE_mask);
			break;
		case GL_POINT:
			SETfield(r700->PA_SU_SC_MODE_CNTL.u32All, X_DRAW_POINTS,
				 POLYMODE_BACK_PTYPE_shift, POLYMODE_BACK_PTYPE_mask);
			break;
		case GL_FILL:
			SETfield(r700->PA_SU_SC_MODE_CNTL.u32All, X_DRAW_TRIANGLES,
				 POLYMODE_BACK_PTYPE_shift, POLYMODE_BACK_PTYPE_mask);
			break;
		}
	}
}

static void r700PolygonMode(GLcontext * ctx, GLenum face, GLenum mode) //------------------
{
	(void)face;
	(void)mode;

	r700UpdatePolygonMode(ctx);
}

static void r700RenderMode(GLcontext * ctx, GLenum mode) //---------------------
{
}

static void r700ClipPlane( GLcontext *ctx, GLenum plane, const GLfloat *eq )
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(&context->hw);
	GLint p;
	GLint *ip;

	p = (GLint) plane - (GLint) GL_CLIP_PLANE0;
	ip = (GLint *)ctx->Transform._ClipUserPlane[p];

	R600_STATECHANGE(context, ucp);

	r700->ucp[p].PA_CL_UCP_0_X.u32All = ip[0];
	r700->ucp[p].PA_CL_UCP_0_Y.u32All = ip[1];
	r700->ucp[p].PA_CL_UCP_0_Z.u32All = ip[2];
	r700->ucp[p].PA_CL_UCP_0_W.u32All = ip[3];
}

static void r700SetClipPlaneState(GLcontext * ctx, GLenum cap, GLboolean state)
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(&context->hw);
	GLuint p;

	p = cap - GL_CLIP_PLANE0;

	R600_STATECHANGE(context, cl);

	if (state) {
		r700->PA_CL_CLIP_CNTL.u32All |= (UCP_ENA_0_bit << p);
		r700->ucp[p].enabled = GL_TRUE;
		r700ClipPlane(ctx, cap, NULL);
	} else {
		r700->PA_CL_CLIP_CNTL.u32All &= ~(UCP_ENA_0_bit << p);
		r700->ucp[p].enabled = GL_FALSE;
	}
}

void r700SetScissor(context_t *context) //---------------
{
	R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(&context->hw);
	unsigned x1, y1, x2, y2;
	int id = 0;
	struct radeon_renderbuffer *rrb;

	rrb = radeon_get_colorbuffer(&context->radeon);
	if (!rrb || !rrb->bo) {
		return;
	}
	if (context->radeon.state.scissor.enabled) {
		x1 = context->radeon.state.scissor.rect.x1;
		y1 = context->radeon.state.scissor.rect.y1;
		x2 = context->radeon.state.scissor.rect.x2;
		y2 = context->radeon.state.scissor.rect.y2;
		/* r600 has exclusive BR scissors */
		if (context->radeon.radeonScreen->kernel_mm) {
			x2++;
			y2++;
		}
	} else {
		if (context->radeon.radeonScreen->driScreen->dri2.enabled) {
			x1 = 0;
			y1 = 0;
			x2 = rrb->base.Width;
			y2 = rrb->base.Height;
		} else {
			x1 = rrb->dPriv->x;
			y1 = rrb->dPriv->y;
			x2 = rrb->dPriv->x + rrb->dPriv->w;
			y2 = rrb->dPriv->y + rrb->dPriv->h;
		}
	}

	R600_STATECHANGE(context, scissor);

	/* screen */
	SETbit(r700->PA_SC_SCREEN_SCISSOR_TL.u32All, WINDOW_OFFSET_DISABLE_bit);
	SETfield(r700->PA_SC_SCREEN_SCISSOR_TL.u32All, x1,
		 PA_SC_SCREEN_SCISSOR_TL__TL_X_shift, PA_SC_SCREEN_SCISSOR_TL__TL_X_mask);
	SETfield(r700->PA_SC_SCREEN_SCISSOR_TL.u32All, y1,
		 PA_SC_SCREEN_SCISSOR_TL__TL_Y_shift, PA_SC_SCREEN_SCISSOR_TL__TL_Y_mask);

	SETfield(r700->PA_SC_SCREEN_SCISSOR_BR.u32All, x2,
		 PA_SC_SCREEN_SCISSOR_BR__BR_X_shift, PA_SC_SCREEN_SCISSOR_BR__BR_X_mask);
	SETfield(r700->PA_SC_SCREEN_SCISSOR_BR.u32All, y2,
		 PA_SC_SCREEN_SCISSOR_BR__BR_Y_shift, PA_SC_SCREEN_SCISSOR_BR__BR_Y_mask);

	/* window */
	SETbit(r700->PA_SC_WINDOW_SCISSOR_TL.u32All, WINDOW_OFFSET_DISABLE_bit);
	SETfield(r700->PA_SC_WINDOW_SCISSOR_TL.u32All, x1,
		 PA_SC_WINDOW_SCISSOR_TL__TL_X_shift, PA_SC_WINDOW_SCISSOR_TL__TL_X_mask);
	SETfield(r700->PA_SC_WINDOW_SCISSOR_TL.u32All, y1,
		 PA_SC_WINDOW_SCISSOR_TL__TL_Y_shift, PA_SC_WINDOW_SCISSOR_TL__TL_Y_mask);

	SETfield(r700->PA_SC_WINDOW_SCISSOR_BR.u32All, x2,
		 PA_SC_WINDOW_SCISSOR_BR__BR_X_shift, PA_SC_WINDOW_SCISSOR_BR__BR_X_mask);
	SETfield(r700->PA_SC_WINDOW_SCISSOR_BR.u32All, y2,
		 PA_SC_WINDOW_SCISSOR_BR__BR_Y_shift, PA_SC_WINDOW_SCISSOR_BR__BR_Y_mask);


	SETfield(r700->PA_SC_CLIPRECT_0_TL.u32All, x1,
		 PA_SC_CLIPRECT_0_TL__TL_X_shift, PA_SC_CLIPRECT_0_TL__TL_X_mask);
	SETfield(r700->PA_SC_CLIPRECT_0_TL.u32All, y1,
		 PA_SC_CLIPRECT_0_TL__TL_Y_shift, PA_SC_CLIPRECT_0_TL__TL_Y_mask);
	SETfield(r700->PA_SC_CLIPRECT_0_BR.u32All, x2,
		 PA_SC_CLIPRECT_0_BR__BR_X_shift, PA_SC_CLIPRECT_0_BR__BR_X_mask);
	SETfield(r700->PA_SC_CLIPRECT_0_BR.u32All, y2,
		 PA_SC_CLIPRECT_0_BR__BR_Y_shift, PA_SC_CLIPRECT_0_BR__BR_Y_mask);

	r700->PA_SC_CLIPRECT_1_TL.u32All = r700->PA_SC_CLIPRECT_0_TL.u32All;
	r700->PA_SC_CLIPRECT_1_BR.u32All = r700->PA_SC_CLIPRECT_0_BR.u32All;
	r700->PA_SC_CLIPRECT_2_TL.u32All = r700->PA_SC_CLIPRECT_0_TL.u32All;
	r700->PA_SC_CLIPRECT_2_BR.u32All = r700->PA_SC_CLIPRECT_0_BR.u32All;
	r700->PA_SC_CLIPRECT_3_TL.u32All = r700->PA_SC_CLIPRECT_0_TL.u32All;
	r700->PA_SC_CLIPRECT_3_BR.u32All = r700->PA_SC_CLIPRECT_0_BR.u32All;

	/* more....2d clip */
	SETbit(r700->PA_SC_GENERIC_SCISSOR_TL.u32All, WINDOW_OFFSET_DISABLE_bit);
	SETfield(r700->PA_SC_GENERIC_SCISSOR_TL.u32All, x1,
		 PA_SC_GENERIC_SCISSOR_TL__TL_X_shift, PA_SC_GENERIC_SCISSOR_TL__TL_X_mask);
	SETfield(r700->PA_SC_GENERIC_SCISSOR_TL.u32All, y1,
		 PA_SC_GENERIC_SCISSOR_TL__TL_Y_shift, PA_SC_GENERIC_SCISSOR_TL__TL_Y_mask);
	SETfield(r700->PA_SC_GENERIC_SCISSOR_BR.u32All, x2,
		 PA_SC_GENERIC_SCISSOR_BR__BR_X_shift, PA_SC_GENERIC_SCISSOR_BR__BR_X_mask);
	SETfield(r700->PA_SC_GENERIC_SCISSOR_BR.u32All, y2,
		 PA_SC_GENERIC_SCISSOR_BR__BR_Y_shift, PA_SC_GENERIC_SCISSOR_BR__BR_Y_mask);

	SETbit(r700->viewport[id].PA_SC_VPORT_SCISSOR_0_TL.u32All, WINDOW_OFFSET_DISABLE_bit);
	SETfield(r700->viewport[id].PA_SC_VPORT_SCISSOR_0_TL.u32All, x1,
		 PA_SC_VPORT_SCISSOR_0_TL__TL_X_shift, PA_SC_VPORT_SCISSOR_0_TL__TL_X_mask);
	SETfield(r700->viewport[id].PA_SC_VPORT_SCISSOR_0_TL.u32All, y1,
		 PA_SC_VPORT_SCISSOR_0_TL__TL_Y_shift, PA_SC_VPORT_SCISSOR_0_TL__TL_Y_mask);
	SETfield(r700->viewport[id].PA_SC_VPORT_SCISSOR_0_BR.u32All, x2,
		 PA_SC_VPORT_SCISSOR_0_BR__BR_X_shift, PA_SC_VPORT_SCISSOR_0_BR__BR_X_mask);
	SETfield(r700->viewport[id].PA_SC_VPORT_SCISSOR_0_BR.u32All, y2,
		 PA_SC_VPORT_SCISSOR_0_BR__BR_Y_shift, PA_SC_VPORT_SCISSOR_0_BR__BR_Y_mask);

	r700->viewport[id].enabled = GL_TRUE;
}

static void r700InitSQConfig(GLcontext * ctx)
{
    context_t *context = R700_CONTEXT(ctx);
    R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(&context->hw);
    int ps_prio;
    int vs_prio;
    int gs_prio;
    int es_prio;
    int num_ps_gprs;
    int num_vs_gprs;
    int num_gs_gprs;
    int num_es_gprs;
    int num_temp_gprs;
    int num_ps_threads;
    int num_vs_threads;
    int num_gs_threads;
    int num_es_threads;
    int num_ps_stack_entries;
    int num_vs_stack_entries;
    int num_gs_stack_entries;
    int num_es_stack_entries;

    R600_STATECHANGE(context, sq);

    // SQ
    ps_prio = 0;
    vs_prio = 1;
    gs_prio = 2;
    es_prio = 3;
    switch (context->radeon.radeonScreen->chip_family) {
    case CHIP_FAMILY_R600:
	    num_ps_gprs = 192;
	    num_vs_gprs = 56;
	    num_temp_gprs = 4;
	    num_gs_gprs = 0;
	    num_es_gprs = 0;
	    num_ps_threads = 136;
	    num_vs_threads = 48;
	    num_gs_threads = 4;
	    num_es_threads = 4;
	    num_ps_stack_entries = 128;
	    num_vs_stack_entries = 128;
	    num_gs_stack_entries = 0;
	    num_es_stack_entries = 0;
	    break;
    case CHIP_FAMILY_RV630:
    case CHIP_FAMILY_RV635:
	    num_ps_gprs = 84;
	    num_vs_gprs = 36;
	    num_temp_gprs = 4;
	    num_gs_gprs = 0;
	    num_es_gprs = 0;
	    num_ps_threads = 144;
	    num_vs_threads = 40;
	    num_gs_threads = 4;
	    num_es_threads = 4;
	    num_ps_stack_entries = 40;
	    num_vs_stack_entries = 40;
	    num_gs_stack_entries = 32;
	    num_es_stack_entries = 16;
	    break;
    case CHIP_FAMILY_RV610:
    case CHIP_FAMILY_RV620:
    case CHIP_FAMILY_RS780:
    case CHIP_FAMILY_RS880:
    default:
	    num_ps_gprs = 84;
	    num_vs_gprs = 36;
	    num_temp_gprs = 4;
	    num_gs_gprs = 0;
	    num_es_gprs = 0;
	    num_ps_threads = 136;
	    num_vs_threads = 48;
	    num_gs_threads = 4;
	    num_es_threads = 4;
	    num_ps_stack_entries = 40;
	    num_vs_stack_entries = 40;
	    num_gs_stack_entries = 32;
	    num_es_stack_entries = 16;
	    break;
    case CHIP_FAMILY_RV670:
	    num_ps_gprs = 144;
	    num_vs_gprs = 40;
	    num_temp_gprs = 4;
	    num_gs_gprs = 0;
	    num_es_gprs = 0;
	    num_ps_threads = 136;
	    num_vs_threads = 48;
	    num_gs_threads = 4;
	    num_es_threads = 4;
	    num_ps_stack_entries = 40;
	    num_vs_stack_entries = 40;
	    num_gs_stack_entries = 32;
	    num_es_stack_entries = 16;
	    break;
    case CHIP_FAMILY_RV770:
	    num_ps_gprs = 192;
	    num_vs_gprs = 56;
	    num_temp_gprs = 4;
	    num_gs_gprs = 0;
	    num_es_gprs = 0;
	    num_ps_threads = 188;
	    num_vs_threads = 60;
	    num_gs_threads = 0;
	    num_es_threads = 0;
	    num_ps_stack_entries = 256;
	    num_vs_stack_entries = 256;
	    num_gs_stack_entries = 0;
	    num_es_stack_entries = 0;
	    break;
    case CHIP_FAMILY_RV730:
    case CHIP_FAMILY_RV740:
	    num_ps_gprs = 84;
	    num_vs_gprs = 36;
	    num_temp_gprs = 4;
	    num_gs_gprs = 0;
	    num_es_gprs = 0;
	    num_ps_threads = 188;
	    num_vs_threads = 60;
	    num_gs_threads = 0;
	    num_es_threads = 0;
	    num_ps_stack_entries = 128;
	    num_vs_stack_entries = 128;
	    num_gs_stack_entries = 0;
	    num_es_stack_entries = 0;
	    break;
    case CHIP_FAMILY_RV710:
	    num_ps_gprs = 192;
	    num_vs_gprs = 56;
	    num_temp_gprs = 4;
	    num_gs_gprs = 0;
	    num_es_gprs = 0;
	    num_ps_threads = 144;
	    num_vs_threads = 48;
	    num_gs_threads = 0;
	    num_es_threads = 0;
	    num_ps_stack_entries = 128;
	    num_vs_stack_entries = 128;
	    num_gs_stack_entries = 0;
	    num_es_stack_entries = 0;
	    break;
    }

    r700->sq_config.SQ_CONFIG.u32All = 0;
    if ((context->radeon.radeonScreen->chip_family == CHIP_FAMILY_RV610) ||
        (context->radeon.radeonScreen->chip_family == CHIP_FAMILY_RV620) ||
	(context->radeon.radeonScreen->chip_family == CHIP_FAMILY_RS780) ||
	(context->radeon.radeonScreen->chip_family == CHIP_FAMILY_RS880) ||
        (context->radeon.radeonScreen->chip_family == CHIP_FAMILY_RV710))
	    CLEARbit(r700->sq_config.SQ_CONFIG.u32All, VC_ENABLE_bit);
    else
	    SETbit(r700->sq_config.SQ_CONFIG.u32All, VC_ENABLE_bit);
    SETbit(r700->sq_config.SQ_CONFIG.u32All, DX9_CONSTS_bit);
    SETbit(r700->sq_config.SQ_CONFIG.u32All, ALU_INST_PREFER_VECTOR_bit);
    SETfield(r700->sq_config.SQ_CONFIG.u32All, ps_prio, PS_PRIO_shift, PS_PRIO_mask);
    SETfield(r700->sq_config.SQ_CONFIG.u32All, vs_prio, VS_PRIO_shift, VS_PRIO_mask);
    SETfield(r700->sq_config.SQ_CONFIG.u32All, gs_prio, GS_PRIO_shift, GS_PRIO_mask);
    SETfield(r700->sq_config.SQ_CONFIG.u32All, es_prio, ES_PRIO_shift, ES_PRIO_mask);

    r700->sq_config.SQ_GPR_RESOURCE_MGMT_1.u32All = 0;
    SETfield(r700->sq_config.SQ_GPR_RESOURCE_MGMT_1.u32All, num_ps_gprs, NUM_PS_GPRS_shift, NUM_PS_GPRS_mask);
    SETfield(r700->sq_config.SQ_GPR_RESOURCE_MGMT_1.u32All, num_vs_gprs, NUM_VS_GPRS_shift, NUM_VS_GPRS_mask);
    SETfield(r700->sq_config.SQ_GPR_RESOURCE_MGMT_1.u32All, num_temp_gprs,
	     NUM_CLAUSE_TEMP_GPRS_shift, NUM_CLAUSE_TEMP_GPRS_mask);

    r700->sq_config.SQ_GPR_RESOURCE_MGMT_2.u32All = 0;
    SETfield(r700->sq_config.SQ_GPR_RESOURCE_MGMT_2.u32All, num_gs_gprs, NUM_GS_GPRS_shift, NUM_GS_GPRS_mask);
    SETfield(r700->sq_config.SQ_GPR_RESOURCE_MGMT_2.u32All, num_es_gprs, NUM_ES_GPRS_shift, NUM_ES_GPRS_mask);

    r700->sq_config.SQ_THREAD_RESOURCE_MGMT.u32All = 0;
    SETfield(r700->sq_config.SQ_THREAD_RESOURCE_MGMT.u32All, num_ps_threads,
	     NUM_PS_THREADS_shift, NUM_PS_THREADS_mask);
    SETfield(r700->sq_config.SQ_THREAD_RESOURCE_MGMT.u32All, num_vs_threads,
	     NUM_VS_THREADS_shift, NUM_VS_THREADS_mask);
    SETfield(r700->sq_config.SQ_THREAD_RESOURCE_MGMT.u32All, num_gs_threads,
	     NUM_GS_THREADS_shift, NUM_GS_THREADS_mask);
    SETfield(r700->sq_config.SQ_THREAD_RESOURCE_MGMT.u32All, num_es_threads,
	     NUM_ES_THREADS_shift, NUM_ES_THREADS_mask);

    r700->sq_config.SQ_STACK_RESOURCE_MGMT_1.u32All = 0;
    SETfield(r700->sq_config.SQ_STACK_RESOURCE_MGMT_1.u32All, num_ps_stack_entries,
	     NUM_PS_STACK_ENTRIES_shift, NUM_PS_STACK_ENTRIES_mask);
    SETfield(r700->sq_config.SQ_STACK_RESOURCE_MGMT_1.u32All, num_vs_stack_entries,
	     NUM_VS_STACK_ENTRIES_shift, NUM_VS_STACK_ENTRIES_mask);

    r700->sq_config.SQ_STACK_RESOURCE_MGMT_2.u32All = 0;
    SETfield(r700->sq_config.SQ_STACK_RESOURCE_MGMT_2.u32All, num_gs_stack_entries,
	     NUM_GS_STACK_ENTRIES_shift, NUM_GS_STACK_ENTRIES_mask);
    SETfield(r700->sq_config.SQ_STACK_RESOURCE_MGMT_2.u32All, num_es_stack_entries,
	     NUM_ES_STACK_ENTRIES_shift, NUM_ES_STACK_ENTRIES_mask);

}

/**
 * Calculate initial hardware state and register state functions.
 * Assumes that the command buffer and state atoms have been
 * initialized already.
 */
void r700InitState(GLcontext * ctx) //-------------------
{
    context_t *context = R700_CONTEXT(ctx);
    R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(&context->hw);
    int id = 0;

    r700->TA_CNTL_AUX.u32All = 0;
    SETfield(r700->TA_CNTL_AUX.u32All, 28, TD_FIFO_CREDIT_shift, TD_FIFO_CREDIT_mask);
    r700->VC_ENHANCE.u32All = 0;
    r700->DB_WATERMARKS.u32All = 0;
    SETfield(r700->DB_WATERMARKS.u32All, 4, DEPTH_FREE_shift, DEPTH_FREE_mask);
    SETfield(r700->DB_WATERMARKS.u32All, 16, DEPTH_FLUSH_shift, DEPTH_FLUSH_mask);
    SETfield(r700->DB_WATERMARKS.u32All, 0, FORCE_SUMMARIZE_shift, FORCE_SUMMARIZE_mask);
    SETfield(r700->DB_WATERMARKS.u32All, 4, DEPTH_PENDING_FREE_shift, DEPTH_PENDING_FREE_mask);
    r700->SQ_DYN_GPR_CNTL_PS_FLUSH_REQ.u32All = 0;
    if (context->radeon.radeonScreen->chip_family < CHIP_FAMILY_RV770) {
	    SETfield(r700->TA_CNTL_AUX.u32All, 3, GRADIENT_CREDIT_shift, GRADIENT_CREDIT_mask);
	    r700->DB_DEBUG.u32All = 0x82000000;
	    SETfield(r700->DB_WATERMARKS.u32All, 16, DEPTH_CACHELINE_FREE_shift, DEPTH_CACHELINE_FREE_mask);
    } else {
	    SETfield(r700->TA_CNTL_AUX.u32All, 2, GRADIENT_CREDIT_shift, GRADIENT_CREDIT_mask);
	    SETfield(r700->DB_WATERMARKS.u32All, 4, DEPTH_CACHELINE_FREE_shift, DEPTH_CACHELINE_FREE_mask);
	    SETbit(r700->SQ_DYN_GPR_CNTL_PS_FLUSH_REQ.u32All, VS_PC_LIMIT_ENABLE_bit);
    }

    /* Turn off vgt reuse */
    r700->VGT_REUSE_OFF.u32All = 0;
    SETbit(r700->VGT_REUSE_OFF.u32All, REUSE_OFF_bit);

    /* Specify offsetting and clamp values for vertices */
    r700->VGT_MAX_VTX_INDX.u32All      = 0xFFFFFF;
    r700->VGT_MIN_VTX_INDX.u32All      = 0;
    r700->VGT_INDX_OFFSET.u32All    = 0;

    /* default shader connections. */
    r700->SPI_VS_OUT_ID_0.u32All  = 0x03020100;
    r700->SPI_VS_OUT_ID_1.u32All  = 0x07060504;
    r700->SPI_VS_OUT_ID_2.u32All  = 0x0b0a0908;
    r700->SPI_VS_OUT_ID_3.u32All  = 0x0f0e0d0c;

    r700->SPI_THREAD_GROUPING.u32All = 0;
    if (context->radeon.radeonScreen->chip_family >= CHIP_FAMILY_RV770)
	    SETfield(r700->SPI_THREAD_GROUPING.u32All, 1, PS_GROUPING_shift, PS_GROUPING_mask);

    /* 4 clip rectangles */ /* TODO : set these clip rects according to context->currentDraw->numClipRects */
    r700->PA_SC_CLIPRECT_RULE.u32All = 0;
    SETfield(r700->PA_SC_CLIPRECT_RULE.u32All, CLIP_RULE_mask, CLIP_RULE_shift, CLIP_RULE_mask);

    if (context->radeon.radeonScreen->chip_family < CHIP_FAMILY_RV770)
	    r700->PA_SC_EDGERULE.u32All = 0;
    else
	    r700->PA_SC_EDGERULE.u32All = 0xAAAAAAAA;

    if (context->radeon.radeonScreen->chip_family < CHIP_FAMILY_RV770) {
	    r700->PA_SC_MODE_CNTL.u32All = 0;
	    SETbit(r700->PA_SC_MODE_CNTL.u32All, WALK_ORDER_ENABLE_bit);
	    SETbit(r700->PA_SC_MODE_CNTL.u32All, FORCE_EOV_CNTDWN_ENABLE_bit);
    } else {
	    r700->PA_SC_MODE_CNTL.u32All = 0x00500000;
	    SETbit(r700->PA_SC_MODE_CNTL.u32All, FORCE_EOV_REZ_ENABLE_bit);
	    SETbit(r700->PA_SC_MODE_CNTL.u32All, FORCE_EOV_CNTDWN_ENABLE_bit);
    }

    /* Do scale XY and Z by 1/W0. */
    r700->bEnablePerspective = GL_TRUE;
    CLEARbit(r700->PA_CL_VTE_CNTL.u32All, VTX_XY_FMT_bit);
    CLEARbit(r700->PA_CL_VTE_CNTL.u32All, VTX_Z_FMT_bit);
    SETbit(r700->PA_CL_VTE_CNTL.u32All, VTX_W0_FMT_bit);

    /* Enable viewport scaling for all three axis */
    SETbit(r700->PA_CL_VTE_CNTL.u32All, VPORT_X_SCALE_ENA_bit);
    SETbit(r700->PA_CL_VTE_CNTL.u32All, VPORT_X_OFFSET_ENA_bit);
    SETbit(r700->PA_CL_VTE_CNTL.u32All, VPORT_Y_SCALE_ENA_bit);
    SETbit(r700->PA_CL_VTE_CNTL.u32All, VPORT_Y_OFFSET_ENA_bit);
    SETbit(r700->PA_CL_VTE_CNTL.u32All, VPORT_Z_SCALE_ENA_bit);
    SETbit(r700->PA_CL_VTE_CNTL.u32All, VPORT_Z_OFFSET_ENA_bit);

    /* GL uses last vtx for flat shading components */
    SETbit(r700->PA_SU_SC_MODE_CNTL.u32All, PROVOKING_VTX_LAST_bit);

    /* Set up vertex control */
    r700->PA_SU_VTX_CNTL.u32All = 0;
    CLEARfield(r700->PA_SU_VTX_CNTL.u32All, QUANT_MODE_mask);
    SETbit(r700->PA_SU_VTX_CNTL.u32All, PIX_CENTER_bit);
    SETfield(r700->PA_SU_VTX_CNTL.u32All, X_ROUND_TO_EVEN,
             PA_SU_VTX_CNTL__ROUND_MODE_shift, PA_SU_VTX_CNTL__ROUND_MODE_mask);

    /* to 1.0 = no guard band */
    r700->PA_CL_GB_VERT_CLIP_ADJ.u32All  = 0x3F800000;  /* 1.0 */
    r700->PA_CL_GB_VERT_DISC_ADJ.u32All  = 0x3F800000;
    r700->PA_CL_GB_HORZ_CLIP_ADJ.u32All  = 0x3F800000;
    r700->PA_CL_GB_HORZ_DISC_ADJ.u32All  = 0x3F800000;

    /* Enable all samples for multi-sample anti-aliasing */
    r700->PA_SC_AA_MASK.u32All = 0xFFFFFFFF;
    /* Turn off AA */
    r700->PA_SC_AA_CONFIG.u32All = 0;

    r700->SX_MISC.u32All = 0;

    r700InitSQConfig(ctx);

    r700ColorMask(ctx,
		  ctx->Color.ColorMask[0][RCOMP],
		  ctx->Color.ColorMask[0][GCOMP],
		  ctx->Color.ColorMask[0][BCOMP],
		  ctx->Color.ColorMask[0][ACOMP]);

    r700Enable(ctx, GL_DEPTH_TEST, ctx->Depth.Test);
    r700DepthMask(ctx, ctx->Depth.Mask);
    r700DepthFunc(ctx, ctx->Depth.Func);
    r700->DB_DEPTH_CLEAR.u32All     = 0x3F800000;
    SETbit(r700->DB_RENDER_CONTROL.u32All, STENCIL_COMPRESS_DISABLE_bit);
    SETbit(r700->DB_RENDER_CONTROL.u32All, DEPTH_COMPRESS_DISABLE_bit);
    r700SetDBRenderState(ctx);

    r700->DB_ALPHA_TO_MASK.u32All = 0;
    SETfield(r700->DB_ALPHA_TO_MASK.u32All, 2, ALPHA_TO_MASK_OFFSET0_shift, ALPHA_TO_MASK_OFFSET0_mask);
    SETfield(r700->DB_ALPHA_TO_MASK.u32All, 2, ALPHA_TO_MASK_OFFSET1_shift, ALPHA_TO_MASK_OFFSET1_mask);
    SETfield(r700->DB_ALPHA_TO_MASK.u32All, 2, ALPHA_TO_MASK_OFFSET2_shift, ALPHA_TO_MASK_OFFSET2_mask);
    SETfield(r700->DB_ALPHA_TO_MASK.u32All, 2, ALPHA_TO_MASK_OFFSET3_shift, ALPHA_TO_MASK_OFFSET3_mask);

    /* stencil */
    r700Enable(ctx, GL_STENCIL_TEST, ctx->Stencil._Enabled);
    r700StencilMaskSeparate(ctx, 0, ctx->Stencil.WriteMask[0]);
    r700StencilFuncSeparate(ctx, 0, ctx->Stencil.Function[0],
			    ctx->Stencil.Ref[0], ctx->Stencil.ValueMask[0]);
    r700StencilOpSeparate(ctx, 0, ctx->Stencil.FailFunc[0],
			  ctx->Stencil.ZFailFunc[0],
			  ctx->Stencil.ZPassFunc[0]);

    r700UpdateCulling(ctx);

    r700SetBlendState(ctx);
    r700SetLogicOpState(ctx);

    r700AlphaFunc(ctx, ctx->Color.AlphaFunc, ctx->Color.AlphaRef);
    r700Enable(ctx, GL_ALPHA_TEST, ctx->Color.AlphaEnabled);

    r700PointSize(ctx, 1.0);

    CLEARfield(r700->PA_SU_POINT_MINMAX.u32All, MIN_SIZE_mask);
    SETfield(r700->PA_SU_POINT_MINMAX.u32All, 0x8000, MAX_SIZE_shift, MAX_SIZE_mask);

    r700LineWidth(ctx, 1.0);

    r700->PA_SC_LINE_CNTL.u32All = 0;
    CLEARbit(r700->PA_SC_LINE_CNTL.u32All, EXPAND_LINE_WIDTH_bit);
    SETbit(r700->PA_SC_LINE_CNTL.u32All, LAST_PIXEL_bit);

    r700ShadeModel(ctx, ctx->Light.ShadeModel);
    r700PolygonMode(ctx, GL_FRONT, ctx->Polygon.FrontMode);
    r700PolygonMode(ctx, GL_BACK, ctx->Polygon.BackMode);
    r700PolygonOffset(ctx, ctx->Polygon.OffsetFactor,
		      ctx->Polygon.OffsetUnits);
    r700Enable(ctx, GL_POLYGON_OFFSET_POINT, ctx->Polygon.OffsetPoint);
    r700Enable(ctx, GL_POLYGON_OFFSET_LINE, ctx->Polygon.OffsetLine);
    r700Enable(ctx, GL_POLYGON_OFFSET_FILL, ctx->Polygon.OffsetFill);

    /* CB */
    r700BlendColor(ctx, ctx->Color.BlendColor);

    r700->CB_CLEAR_RED_R6XX.f32All = 1.0; //r6xx only
    r700->CB_CLEAR_GREEN_R6XX.f32All = 0.0; //r6xx only
    r700->CB_CLEAR_BLUE_R6XX.f32All = 1.0; //r6xx only
    r700->CB_CLEAR_ALPHA_R6XX.f32All = 1.0; //r6xx only
    r700->CB_FOG_RED_R6XX.u32All = 0; //r6xx only
    r700->CB_FOG_GREEN_R6XX.u32All = 0; //r6xx only
    r700->CB_FOG_BLUE_R6XX.u32All = 0; //r6xx only

    /* Disable color compares */
    SETfield(r700->CB_CLRCMP_CONTROL.u32All, CLRCMP_DRAW_ALWAYS,
             CLRCMP_FCN_SRC_shift, CLRCMP_FCN_SRC_mask);
    SETfield(r700->CB_CLRCMP_CONTROL.u32All, CLRCMP_DRAW_ALWAYS,
             CLRCMP_FCN_DST_shift, CLRCMP_FCN_DST_mask);
    SETfield(r700->CB_CLRCMP_CONTROL.u32All, CLRCMP_SEL_SRC,
             CLRCMP_FCN_SEL_shift, CLRCMP_FCN_SEL_mask);

    /* Zero out source */
    r700->CB_CLRCMP_SRC.u32All = 0x00000000;

    /* Put a compare color in for error checking */
    r700->CB_CLRCMP_DST.u32All = 0x000000FF;

    /* Set up color compare mask */
    r700->CB_CLRCMP_MSK.u32All = 0xFFFFFFFF;

    /* screen/window/view */
    SETfield(r700->CB_SHADER_MASK.u32All, 0xF, (4 * id), OUTPUT0_ENABLE_mask);

    context->radeon.hw.all_dirty = GL_TRUE;

}

void r700InitStateFuncs(struct dd_function_table *functions) //-----------------
{
	functions->UpdateState = r700InvalidateState;
	functions->AlphaFunc = r700AlphaFunc;
	functions->BlendColor = r700BlendColor;
	functions->BlendEquationSeparate = r700BlendEquationSeparate;
	functions->BlendFuncSeparate = r700BlendFuncSeparate;
	functions->Enable = r700Enable;
	functions->ColorMask = r700ColorMask;
	functions->DepthFunc = r700DepthFunc;
	functions->DepthMask = r700DepthMask;
	functions->CullFace = r700CullFace;
	functions->Fogfv = r700Fogfv;
	functions->FrontFace = r700FrontFace;
	functions->ShadeModel = r700ShadeModel;
	functions->LogicOpcode = r700LogicOpcode;

	/* ARB_point_parameters */
	functions->PointParameterfv = r700PointParameter;

	/* Stencil related */
	functions->StencilFuncSeparate = r700StencilFuncSeparate;
	functions->StencilMaskSeparate = r700StencilMaskSeparate;
	functions->StencilOpSeparate = r700StencilOpSeparate;

	/* Viewport related */
	functions->Viewport = r700Viewport;
	functions->DepthRange = r700DepthRange;
	functions->PointSize = r700PointSize;
	functions->LineWidth = r700LineWidth;
	functions->LineStipple = r700LineStipple;

	functions->PolygonOffset = r700PolygonOffset;
	functions->PolygonMode = r700PolygonMode;

	functions->RenderMode = r700RenderMode;

	functions->ClipPlane = r700ClipPlane;

	functions->Scissor = radeonScissor;

	functions->DrawBuffer		= radeonDrawBuffer;
	functions->ReadBuffer		= radeonReadBuffer;

}


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
#include "main/state.h"

#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "main/api_arrayelt.h"
#include "main/framebuffer.h"
#include "drivers/common/meta.h"
#include "program/prog_parameter.h"
#include "program/prog_statevars.h"

#include "vbo/vbo.h"

#include "r600_context.h"

#include "evergreen_state.h"
#include "evergreen_diff.h"
#include "evergreen_vertprog.h"
#include "evergreen_fragprog.h"
#include "evergreen_tex.h"

void evergreenUpdateStateParameters(struct gl_context * ctx, GLuint new_state); //same

void evergreenUpdateShaders(struct gl_context * ctx)
{
    context_t *context = EVERGREEN_CONTEXT(ctx);

    /* should only happenen once, just after context is created */
    /* TODO: shouldn't we fallback to sw here? */
    if (!ctx->FragmentProgram._Current) {
	    fprintf(stderr, "No ctx->FragmentProgram._Current!!\n");
	    return;
    }

    evergreenSelectFragmentShader(ctx);

    evergreenSelectVertexShader(ctx);
    evergreenUpdateStateParameters(ctx, _NEW_PROGRAM | _NEW_PROGRAM_CONSTANTS);
    context->radeon.NewGLState = 0;
}

void evergreeUpdateShaders(struct gl_context * ctx)
{
    context_t *context = R700_CONTEXT(ctx);

    /* should only happenen once, just after context is created */
    /* TODO: shouldn't we fallback to sw here? */
    if (!ctx->FragmentProgram._Current) {
	    fprintf(stderr, "No ctx->FragmentProgram._Current!!\n");
	    return;
    }

    evergreenSelectFragmentShader(ctx);

    evergreenSelectVertexShader(ctx);
    evergreenUpdateStateParameters(ctx, _NEW_PROGRAM | _NEW_PROGRAM_CONSTANTS);
    context->radeon.NewGLState = 0;
}

/*
 * To correctly position primitives:
 */
void evergreenUpdateViewportOffset(struct gl_context * ctx) //------------------
{
	context_t *context = R700_CONTEXT(ctx);
	EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);
	__DRIdrawable *dPriv = radeon_get_drawable(&context->radeon);
	GLfloat xoffset = (GLfloat) dPriv->x;
	GLfloat yoffset = (GLfloat) dPriv->y + dPriv->h;
	const GLfloat *v = ctx->Viewport._WindowMap.m;
	int id = 0;

	GLfloat tx = v[MAT_TX] + xoffset;
	GLfloat ty = (-v[MAT_TY]) + yoffset;

	if (evergreen->viewport[id].PA_CL_VPORT_XOFFSET.f32All != tx ||
	    evergreen->viewport[id].PA_CL_VPORT_YOFFSET.f32All != ty) {
		/* Note: this should also modify whatever data the context reset
		 * code uses...
		 */
		EVERGREEN_STATECHANGE(context, pa);
		evergreen->viewport[id].PA_CL_VPORT_XOFFSET.f32All = tx;
		evergreen->viewport[id].PA_CL_VPORT_YOFFSET.f32All = ty;
	}

	radeonUpdateScissor(ctx);
}

void evergreenUpdateStateParameters(struct gl_context * ctx, GLuint new_state) //same
{
	struct evergreen_fragment_program *fp =
		(struct evergreen_fragment_program *)ctx->FragmentProgram._Current;
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
static void evergreenInvalidateState(struct gl_context * ctx, GLuint new_state) //same
{
    context_t *context = EVERGREEN_CONTEXT(ctx);

    EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);

    _swrast_InvalidateState(ctx, new_state);
    _swsetup_InvalidateState(ctx, new_state);
    _vbo_InvalidateState(ctx, new_state);
    _tnl_InvalidateState(ctx, new_state);
    _ae_invalidate_state(ctx, new_state);

    if (new_state & _NEW_BUFFERS) {
	    _mesa_update_framebuffer(ctx);
	    /* this updates the DrawBuffer's Width/Height if it's a FBO */
	    _mesa_update_draw_buffer_bounds(ctx);

	    EVERGREEN_STATECHANGE(context, cb);
	    EVERGREEN_STATECHANGE(context, db);
    }

    if (new_state & (_NEW_LIGHT)) {
	    EVERGREEN_STATECHANGE(context, pa);
	    if (ctx->Light.ProvokingVertex == GL_LAST_VERTEX_CONVENTION)
		    SETbit(evergreen->PA_SU_SC_MODE_CNTL.u32All, PROVOKING_VTX_LAST_bit);
	    else
		    CLEARbit(evergreen->PA_SU_SC_MODE_CNTL.u32All, PROVOKING_VTX_LAST_bit);
    }

    evergreenUpdateStateParameters(ctx, new_state);

    EVERGREEN_STATECHANGE(context, pa);
    EVERGREEN_STATECHANGE(context, spi);

    if(GL_TRUE == evergreen->bEnablePerspective)
    {
        /* Do scale XY and Z by 1/W0 for perspective correction on pos. For orthogonal case, set both to one. */
        CLEARbit(evergreen->PA_CL_VTE_CNTL.u32All, VTX_XY_FMT_bit);
        CLEARbit(evergreen->PA_CL_VTE_CNTL.u32All, VTX_Z_FMT_bit);

        SETbit(evergreen->PA_CL_VTE_CNTL.u32All, VTX_W0_FMT_bit);

        SETbit(evergreen->SPI_PS_IN_CONTROL_0.u32All, PERSP_GRADIENT_ENA_bit);
        CLEARbit(evergreen->SPI_PS_IN_CONTROL_0.u32All, LINEAR_GRADIENT_ENA_bit);

        SETfield(evergreen->SPI_BARYC_CNTL.u32All, 1,
		         EG_SPI_BARYC_CNTL__PERSP_CENTROID_ENA_shift, 
                 EG_SPI_BARYC_CNTL__PERSP_CENTROID_ENA_mask);        
    }
    else
    {
        /* For orthogonal case. */
        SETbit(evergreen->PA_CL_VTE_CNTL.u32All, VTX_XY_FMT_bit);
        SETbit(evergreen->PA_CL_VTE_CNTL.u32All, VTX_Z_FMT_bit);

        SETbit(evergreen->PA_CL_VTE_CNTL.u32All, VTX_W0_FMT_bit);
       
        CLEARbit(evergreen->SPI_PS_IN_CONTROL_0.u32All, PERSP_GRADIENT_ENA_bit);
        SETbit(evergreen->SPI_PS_IN_CONTROL_0.u32All, LINEAR_GRADIENT_ENA_bit);
        
        SETfield(evergreen->SPI_BARYC_CNTL.u32All, 1,
		         EG_SPI_BARYC_CNTL__LINEAR_CENTROID_ENA_shift, 
                 EG_SPI_BARYC_CNTL__LINEAR_CENTROID_ENA_mask);        
    }

    context->radeon.NewGLState |= new_state;
}

static void evergreenSetAlphaState(struct gl_context * ctx)  //same
{
	context_t *context = EVERGREEN_CONTEXT(ctx);
	EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);
	uint32_t alpha_func = REF_ALWAYS;
	GLboolean really_enabled = ctx->Color.AlphaEnabled;

	EVERGREEN_STATECHANGE(context, sx);

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
		SETfield(evergreen->SX_ALPHA_TEST_CONTROL.u32All, alpha_func,
			 ALPHA_FUNC_shift, ALPHA_FUNC_mask);
		SETbit(evergreen->SX_ALPHA_TEST_CONTROL.u32All, ALPHA_TEST_ENABLE_bit);
		evergreen->SX_ALPHA_REF.f32All = ctx->Color.AlphaRef;
	} else {
		CLEARbit(evergreen->SX_ALPHA_TEST_CONTROL.u32All, ALPHA_TEST_ENABLE_bit);
	}
}

static void evergreenAlphaFunc(struct gl_context * ctx, GLenum func, GLfloat ref) //same
{
	(void)func;
	(void)ref;
	evergreenSetAlphaState(ctx);
}

static void evergreenBlendColor(struct gl_context * ctx, const GLfloat cf[4]) //same
{
	context_t *context = EVERGREEN_CONTEXT(ctx);
	EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);

	EVERGREEN_STATECHANGE(context, cb);

	evergreen->CB_BLEND_RED.f32All = cf[0];
	evergreen->CB_BLEND_GREEN.f32All = cf[1];
	evergreen->CB_BLEND_BLUE.f32All = cf[2];
	evergreen->CB_BLEND_ALPHA.f32All = cf[3];
}

static int evergreenblend_factor(GLenum factor, GLboolean is_src) //same
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

static void evergreenSetBlendState(struct gl_context * ctx) //diff : CB_COLOR_CONTROL, CB_BLEND0_CONTROL bits
{
	context_t *context = EVERGREEN_CONTEXT(ctx);
	EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);
	uint32_t blend_reg = 0, eqn, eqnA;

	EVERGREEN_STATECHANGE(context, cb);

	if (_mesa_rgba_logicop_enabled(ctx) || !ctx->Color.BlendEnabled) {
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
		//if (context->radeon.radeonScreen->chip_family == CHIP_FAMILY_R600)
		//	evergreen->CB_BLEND_CONTROL.u32All = blend_reg;
		//else
			evergreen->CB_BLEND0_CONTROL.u32All = blend_reg;
		return;
	}

	SETfield(blend_reg,
		 evergreenblend_factor(ctx->Color.Blend[0].SrcRGB, GL_TRUE),
		 COLOR_SRCBLEND_shift, COLOR_SRCBLEND_mask);
	SETfield(blend_reg,
		 evergreenblend_factor(ctx->Color.Blend[0].DstRGB, GL_FALSE),
		 COLOR_DESTBLEND_shift, COLOR_DESTBLEND_mask);

	switch (ctx->Color.Blend[0].EquationRGB) {
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
			__FUNCTION__, __LINE__, ctx->Color.Blend[0].EquationRGB);
		return;
	}
	SETfield(blend_reg,
		 eqn, COLOR_COMB_FCN_shift, COLOR_COMB_FCN_mask);

	SETfield(blend_reg,
		 evergreenblend_factor(ctx->Color.Blend[0].SrcA, GL_TRUE),
		 ALPHA_SRCBLEND_shift, ALPHA_SRCBLEND_mask);
	SETfield(blend_reg,
		 evergreenblend_factor(ctx->Color.Blend[0].DstA, GL_FALSE),
		 ALPHA_DESTBLEND_shift, ALPHA_DESTBLEND_mask);

	switch (ctx->Color.Blend[0].EquationA) {
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
			__FUNCTION__, __LINE__, ctx->Color.Blend[0].EquationA);
		return;
	}

	SETfield(blend_reg,
		 eqnA, ALPHA_COMB_FCN_shift, ALPHA_COMB_FCN_mask);

	SETbit(blend_reg, SEPARATE_ALPHA_BLEND_bit);

    SETbit(blend_reg, EG_CB_BLENDX_CONTROL_ENABLE_bit);
	
    evergreen->CB_BLEND0_CONTROL.u32All = blend_reg;	
}

static void evergreenBlendEquationSeparate(struct gl_context * ctx,
				                      GLenum modeRGB, GLenum modeA) //same
{
	evergreenSetBlendState(ctx);
}

static void evergreenBlendFuncSeparate(struct gl_context * ctx,
				  GLenum sfactorRGB, GLenum dfactorRGB,
				  GLenum sfactorA, GLenum dfactorA) //same
{
	evergreenSetBlendState(ctx);
}

static GLuint evergreen_translate_logicop(GLenum logicop) //same
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

static void evergreenSetLogicOpState(struct gl_context *ctx) //diff : CB_COLOR_CONTROL.ROP3 is actually same bits.
{
	context_t *context = EVERGREEN_CONTEXT(ctx);
	EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);

	EVERGREEN_STATECHANGE(context, cb);

	if (_mesa_rgba_logicop_enabled(ctx))
		SETfield(evergreen->CB_COLOR_CONTROL.u32All,
			 evergreen_translate_logicop(ctx->Color.LogicOp), 
             EG_CB_COLOR_CONTROL__ROP3_shift, 
             EG_CB_COLOR_CONTROL__ROP3_mask);
	else
		SETfield(evergreen->CB_COLOR_CONTROL.u32All, 0xCC, 
             EG_CB_COLOR_CONTROL__ROP3_shift, 
             EG_CB_COLOR_CONTROL__ROP3_mask);
}

static void evergreenClipPlane( struct gl_context *ctx, GLenum plane, const GLfloat *eq ) //same , but PA_CL_UCP_0_ offset diff 
{
	context_t *context = EVERGREEN_CONTEXT(ctx);
	EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);
	GLint p;
	GLint *ip;

	p = (GLint) plane - (GLint) GL_CLIP_PLANE0;
	ip = (GLint *)ctx->Transform._ClipUserPlane[p];

	EVERGREEN_STATECHANGE(context, pa);

	evergreen->ucp[p].PA_CL_UCP_0_X.u32All = ip[0];
	evergreen->ucp[p].PA_CL_UCP_0_Y.u32All = ip[1];
	evergreen->ucp[p].PA_CL_UCP_0_Z.u32All = ip[2];
	evergreen->ucp[p].PA_CL_UCP_0_W.u32All = ip[3];
}

static void evergreenSetClipPlaneState(struct gl_context * ctx, GLenum cap, GLboolean state) //diff in func calls
{
	context_t *context = EVERGREEN_CONTEXT(ctx);
	EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);
	GLuint p;

	p = cap - GL_CLIP_PLANE0;

	EVERGREEN_STATECHANGE(context, pa);

	if (state) {
		evergreen->PA_CL_CLIP_CNTL.u32All |= (UCP_ENA_0_bit << p);
		evergreen->ucp[p].enabled = GL_TRUE;
		evergreenClipPlane(ctx, cap, NULL);
	} else {
		evergreen->PA_CL_CLIP_CNTL.u32All &= ~(UCP_ENA_0_bit << p);
		evergreen->ucp[p].enabled = GL_FALSE;
	}
}

static void evergreenSetDBRenderState(struct gl_context * ctx)
{    
    context_t *context = EVERGREEN_CONTEXT(ctx);
    EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);
    struct evergreen_fragment_program *fp =
            (struct evergreen_fragment_program *)(ctx->FragmentProgram._Current);
	
    EVERGREEN_STATECHANGE(context, db);

    SETbit(evergreen->DB_SHADER_CONTROL.u32All, 
           DUAL_EXPORT_ENABLE_bit);
    SETfield(evergreen->DB_SHADER_CONTROL.u32All, EARLY_Z_THEN_LATE_Z, 
             Z_ORDER_shift, 
             Z_ORDER_mask);
	/* XXX need to enable htile for hiz/s */
    SETfield(evergreen->DB_RENDER_OVERRIDE.u32All, FORCE_DISABLE, 
             FORCE_HIZ_ENABLE_shift, 
             FORCE_HIZ_ENABLE_mask);
    SETfield(evergreen->DB_RENDER_OVERRIDE.u32All, FORCE_DISABLE, 
             FORCE_HIS_ENABLE0_shift, 
             FORCE_HIS_ENABLE0_mask);
    SETfield(evergreen->DB_RENDER_OVERRIDE.u32All, FORCE_DISABLE, 
             FORCE_HIS_ENABLE1_shift, 
             FORCE_HIS_ENABLE1_mask);

    if (context->radeon.query.current)
    {
        SETbit(evergreen->DB_RENDER_OVERRIDE.u32All, NOOP_CULL_DISABLE_bit);        
        SETbit(evergreen->DB_COUNT_CONTROL.u32All, 
               EG_DB_COUNT_CONTROL__PERFECT_ZPASS_COUNTS_bit);        
    }
    else
    {
        CLEARbit(evergreen->DB_RENDER_OVERRIDE.u32All, NOOP_CULL_DISABLE_bit);        
        CLEARbit(evergreen->DB_COUNT_CONTROL.u32All, 
                 EG_DB_COUNT_CONTROL__PERFECT_ZPASS_COUNTS_bit);        
    }

    if (fp)
    {
        if (fp->r700Shader.killIsUsed)
        {
            SETbit(evergreen->DB_SHADER_CONTROL.u32All, KILL_ENABLE_bit);
        }
        else
        {
            CLEARbit(evergreen->DB_SHADER_CONTROL.u32All, KILL_ENABLE_bit);
        }

        if (fp->r700Shader.depthIsExported)
        {
            SETbit(evergreen->DB_SHADER_CONTROL.u32All, Z_EXPORT_ENABLE_bit);
        }
        else
        {
            CLEARbit(evergreen->DB_SHADER_CONTROL.u32All, Z_EXPORT_ENABLE_bit);
        }
    }    
}

void evergreenUpdateShaderStates(struct gl_context * ctx)
{
	evergreenSetDBRenderState(ctx);
	evergreenUpdateTextureState(ctx);
}

static void evergreenSetDepthState(struct gl_context * ctx) //same
{
	context_t *context = EVERGREEN_CONTEXT(ctx);
	EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);

	EVERGREEN_STATECHANGE(context, db);

    if (ctx->Depth.Test)
    {
        SETbit(evergreen->DB_DEPTH_CONTROL.u32All, Z_ENABLE_bit);
        if (ctx->Depth.Mask)
        {
            SETbit(evergreen->DB_DEPTH_CONTROL.u32All, Z_WRITE_ENABLE_bit);
        }
        else
        {
            CLEARbit(evergreen->DB_DEPTH_CONTROL.u32All, Z_WRITE_ENABLE_bit);
        }

        switch (ctx->Depth.Func)
        {
        case GL_NEVER:
            SETfield(evergreen->DB_DEPTH_CONTROL.u32All, FRAG_NEVER,
                     ZFUNC_shift, ZFUNC_mask);
            break;
        case GL_LESS:
            SETfield(evergreen->DB_DEPTH_CONTROL.u32All, FRAG_LESS,
                     ZFUNC_shift, ZFUNC_mask);
            break;
        case GL_EQUAL:
            SETfield(evergreen->DB_DEPTH_CONTROL.u32All, FRAG_EQUAL,
                     ZFUNC_shift, ZFUNC_mask);
            break;
        case GL_LEQUAL:
            SETfield(evergreen->DB_DEPTH_CONTROL.u32All, FRAG_LEQUAL,
                     ZFUNC_shift, ZFUNC_mask);
            break;
        case GL_GREATER:
            SETfield(evergreen->DB_DEPTH_CONTROL.u32All, FRAG_GREATER,
                     ZFUNC_shift, ZFUNC_mask);
            break;
        case GL_NOTEQUAL:
            SETfield(evergreen->DB_DEPTH_CONTROL.u32All, FRAG_NOTEQUAL,
                     ZFUNC_shift, ZFUNC_mask);
            break;
        case GL_GEQUAL:
            SETfield(evergreen->DB_DEPTH_CONTROL.u32All, FRAG_GEQUAL,
                     ZFUNC_shift, ZFUNC_mask);
            break;
        case GL_ALWAYS:
            SETfield(evergreen->DB_DEPTH_CONTROL.u32All, FRAG_ALWAYS,
                     ZFUNC_shift, ZFUNC_mask);
            break;
        default:
            SETfield(evergreen->DB_DEPTH_CONTROL.u32All, FRAG_ALWAYS,
                     ZFUNC_shift, ZFUNC_mask);
            break;
        }
    }
    else    
    {
        CLEARbit(evergreen->DB_DEPTH_CONTROL.u32All, Z_ENABLE_bit);
        CLEARbit(evergreen->DB_DEPTH_CONTROL.u32All, Z_WRITE_ENABLE_bit);
    }
}

static void evergreenSetStencilState(struct gl_context * ctx, GLboolean state)  //same
{
    context_t *context = EVERGREEN_CONTEXT(ctx);
    EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);
    GLboolean hw_stencil = GL_FALSE;

    if (ctx->DrawBuffer) {
        struct radeon_renderbuffer *rrbStencil
            = radeon_get_renderbuffer(ctx->DrawBuffer, BUFFER_STENCIL);
        hw_stencil = (rrbStencil && rrbStencil->bo);
    }

    if (hw_stencil) {
        EVERGREEN_STATECHANGE(context, db);
        if (state) {
            SETbit(evergreen->DB_DEPTH_CONTROL.u32All, STENCIL_ENABLE_bit);
            SETbit(evergreen->DB_DEPTH_CONTROL.u32All, BACKFACE_ENABLE_bit);
            SETbit(evergreen->DB_STENCIL_INFO.u32All,  EG_DB_STENCIL_INFO__FORMAT_bit);
        } else
            CLEARbit(evergreen->DB_DEPTH_CONTROL.u32All, STENCIL_ENABLE_bit);
    }
}

static void evergreenUpdateCulling(struct gl_context * ctx) //same
{
	context_t *context = EVERGREEN_CONTEXT(ctx);
	EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);

    EVERGREEN_STATECHANGE(context, pa);

    CLEARbit(evergreen->PA_SU_SC_MODE_CNTL.u32All, FACE_bit);
    CLEARbit(evergreen->PA_SU_SC_MODE_CNTL.u32All, CULL_FRONT_bit);
    CLEARbit(evergreen->PA_SU_SC_MODE_CNTL.u32All, CULL_BACK_bit);

    if (ctx->Polygon.CullFlag)
    {
        switch (ctx->Polygon.CullFaceMode)
        {
        case GL_FRONT:
            SETbit(evergreen->PA_SU_SC_MODE_CNTL.u32All, CULL_FRONT_bit);
            CLEARbit(evergreen->PA_SU_SC_MODE_CNTL.u32All, CULL_BACK_bit);
            break;
        case GL_BACK:
            CLEARbit(evergreen->PA_SU_SC_MODE_CNTL.u32All, CULL_FRONT_bit);
            SETbit(evergreen->PA_SU_SC_MODE_CNTL.u32All, CULL_BACK_bit);
            break;
        case GL_FRONT_AND_BACK:
            SETbit(evergreen->PA_SU_SC_MODE_CNTL.u32All, CULL_FRONT_bit);
            SETbit(evergreen->PA_SU_SC_MODE_CNTL.u32All, CULL_BACK_bit);
            break;
        default:
            CLEARbit(evergreen->PA_SU_SC_MODE_CNTL.u32All, CULL_FRONT_bit);
            CLEARbit(evergreen->PA_SU_SC_MODE_CNTL.u32All, CULL_BACK_bit);
            break;
        }
    }

    switch (ctx->Polygon.FrontFace)
    {
        case GL_CW:
            SETbit(evergreen->PA_SU_SC_MODE_CNTL.u32All, FACE_bit);
            break;
        case GL_CCW:
            CLEARbit(evergreen->PA_SU_SC_MODE_CNTL.u32All, FACE_bit);
            break;
        default:
            CLEARbit(evergreen->PA_SU_SC_MODE_CNTL.u32All, FACE_bit); /* default: ccw */
            break;
    }

    /* Winding is inverted when rendering to FBO */
    if (ctx->DrawBuffer && ctx->DrawBuffer->Name)
	    evergreen->PA_SU_SC_MODE_CNTL.u32All ^= FACE_bit;
}

static void evergreenSetPolygonOffsetState(struct gl_context * ctx, GLboolean state) //same
{
	context_t *context = EVERGREEN_CONTEXT(ctx);
	EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);

	EVERGREEN_STATECHANGE(context, pa);

	if (state) {
		SETbit(evergreen->PA_SU_SC_MODE_CNTL.u32All, POLY_OFFSET_FRONT_ENABLE_bit);
		SETbit(evergreen->PA_SU_SC_MODE_CNTL.u32All, POLY_OFFSET_BACK_ENABLE_bit);
		SETbit(evergreen->PA_SU_SC_MODE_CNTL.u32All, POLY_OFFSET_PARA_ENABLE_bit);
	} else {
		CLEARbit(evergreen->PA_SU_SC_MODE_CNTL.u32All, POLY_OFFSET_FRONT_ENABLE_bit);
		CLEARbit(evergreen->PA_SU_SC_MODE_CNTL.u32All, POLY_OFFSET_BACK_ENABLE_bit);
		CLEARbit(evergreen->PA_SU_SC_MODE_CNTL.u32All, POLY_OFFSET_PARA_ENABLE_bit);
	}
}

static void evergreenUpdateLineStipple(struct gl_context * ctx) //diff
{
	/* TODO */
}

void evergreenSetScissor(context_t *context) //diff
{
	EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);
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

	EVERGREEN_STATECHANGE(context, pa);

	/* screen */
    /* TODO : check WINDOW_OFFSET_DISABLE */
	//SETbit(evergreen->PA_SC_SCREEN_SCISSOR_TL.u32All, WINDOW_OFFSET_DISABLE_bit);
	SETfield(evergreen->PA_SC_SCREEN_SCISSOR_TL.u32All, x1,
		 PA_SC_SCREEN_SCISSOR_TL__TL_X_shift, EG_PA_SC_SCREEN_SCISSOR_TL__TL_X_mask);
	SETfield(evergreen->PA_SC_SCREEN_SCISSOR_TL.u32All, y1,
		 PA_SC_SCREEN_SCISSOR_TL__TL_Y_shift, EG_PA_SC_SCREEN_SCISSOR_TL__TL_Y_mask);

	SETfield(evergreen->PA_SC_SCREEN_SCISSOR_BR.u32All, x2,
		 PA_SC_SCREEN_SCISSOR_BR__BR_X_shift, EG_PA_SC_SCREEN_SCISSOR_BR__BR_X_mask);
	SETfield(evergreen->PA_SC_SCREEN_SCISSOR_BR.u32All, y2,
		 PA_SC_SCREEN_SCISSOR_BR__BR_Y_shift, EG_PA_SC_SCREEN_SCISSOR_BR__BR_Y_mask);

	/* window */
	SETbit(evergreen->PA_SC_WINDOW_SCISSOR_TL.u32All, WINDOW_OFFSET_DISABLE_bit);
	SETfield(evergreen->PA_SC_WINDOW_SCISSOR_TL.u32All, x1,
		 PA_SC_WINDOW_SCISSOR_TL__TL_X_shift, EG_PA_SC_WINDOW_SCISSOR_TL__TL_X_mask);
	SETfield(evergreen->PA_SC_WINDOW_SCISSOR_TL.u32All, y1,
		 PA_SC_WINDOW_SCISSOR_TL__TL_Y_shift, EG_PA_SC_WINDOW_SCISSOR_TL__TL_Y_mask);

	SETfield(evergreen->PA_SC_WINDOW_SCISSOR_BR.u32All, x2,
		 PA_SC_WINDOW_SCISSOR_BR__BR_X_shift, EG_PA_SC_WINDOW_SCISSOR_BR__BR_X_mask);
	SETfield(evergreen->PA_SC_WINDOW_SCISSOR_BR.u32All, y2,
		 PA_SC_WINDOW_SCISSOR_BR__BR_Y_shift, EG_PA_SC_WINDOW_SCISSOR_BR__BR_Y_mask);


	SETfield(evergreen->PA_SC_CLIPRECT_0_TL.u32All, x1,
		 PA_SC_CLIPRECT_0_TL__TL_X_shift, EG_PA_SC_CLIPRECT_0_TL__TL_X_mask);
	SETfield(evergreen->PA_SC_CLIPRECT_0_TL.u32All, y1,
		 PA_SC_CLIPRECT_0_TL__TL_Y_shift, EG_PA_SC_CLIPRECT_0_TL__TL_Y_mask);
	SETfield(evergreen->PA_SC_CLIPRECT_0_BR.u32All, x2,
		 PA_SC_CLIPRECT_0_BR__BR_X_shift, EG_PA_SC_CLIPRECT_0_BR__BR_X_mask);
	SETfield(evergreen->PA_SC_CLIPRECT_0_BR.u32All, y2,
		 PA_SC_CLIPRECT_0_BR__BR_Y_shift, EG_PA_SC_CLIPRECT_0_BR__BR_Y_mask);

	evergreen->PA_SC_CLIPRECT_1_TL.u32All = evergreen->PA_SC_CLIPRECT_0_TL.u32All;
	evergreen->PA_SC_CLIPRECT_1_BR.u32All = evergreen->PA_SC_CLIPRECT_0_BR.u32All;
	evergreen->PA_SC_CLIPRECT_2_TL.u32All = evergreen->PA_SC_CLIPRECT_0_TL.u32All;
	evergreen->PA_SC_CLIPRECT_2_BR.u32All = evergreen->PA_SC_CLIPRECT_0_BR.u32All;
	evergreen->PA_SC_CLIPRECT_3_TL.u32All = evergreen->PA_SC_CLIPRECT_0_TL.u32All;
	evergreen->PA_SC_CLIPRECT_3_BR.u32All = evergreen->PA_SC_CLIPRECT_0_BR.u32All;

	/* more....2d clip */
	SETbit(evergreen->PA_SC_GENERIC_SCISSOR_TL.u32All, WINDOW_OFFSET_DISABLE_bit);
	SETfield(evergreen->PA_SC_GENERIC_SCISSOR_TL.u32All, x1,
		 PA_SC_GENERIC_SCISSOR_TL__TL_X_shift, EG_PA_SC_GENERIC_SCISSOR_TL__TL_X_mask);
	SETfield(evergreen->PA_SC_GENERIC_SCISSOR_TL.u32All, y1,
		 PA_SC_GENERIC_SCISSOR_TL__TL_Y_shift, EG_PA_SC_GENERIC_SCISSOR_TL__TL_Y_mask);
	SETfield(evergreen->PA_SC_GENERIC_SCISSOR_BR.u32All, x2,
		 PA_SC_GENERIC_SCISSOR_BR__BR_X_shift, EG_PA_SC_GENERIC_SCISSOR_BR__BR_X_mask);
	SETfield(evergreen->PA_SC_GENERIC_SCISSOR_BR.u32All, y2,
		 PA_SC_GENERIC_SCISSOR_BR__BR_Y_shift, EG_PA_SC_GENERIC_SCISSOR_BR__BR_Y_mask);

	SETbit(evergreen->viewport[id].PA_SC_VPORT_SCISSOR_0_TL.u32All, WINDOW_OFFSET_DISABLE_bit);
	SETfield(evergreen->viewport[id].PA_SC_VPORT_SCISSOR_0_TL.u32All, x1,
		 PA_SC_VPORT_SCISSOR_0_TL__TL_X_shift, EG_PA_SC_VPORT_SCISSOR_0_TL__TL_X_mask);
	SETfield(evergreen->viewport[id].PA_SC_VPORT_SCISSOR_0_TL.u32All, y1,
		 PA_SC_VPORT_SCISSOR_0_TL__TL_Y_shift, EG_PA_SC_VPORT_SCISSOR_0_TL__TL_Y_mask);
	SETfield(evergreen->viewport[id].PA_SC_VPORT_SCISSOR_0_BR.u32All, x2,
		 PA_SC_VPORT_SCISSOR_0_BR__BR_X_shift, EG_PA_SC_VPORT_SCISSOR_0_BR__BR_X_mask);
	SETfield(evergreen->viewport[id].PA_SC_VPORT_SCISSOR_0_BR.u32All, y2,
		 PA_SC_VPORT_SCISSOR_0_BR__BR_Y_shift, EG_PA_SC_VPORT_SCISSOR_0_BR__BR_Y_mask);

    id = 1;
    SETbit(evergreen->viewport[id].PA_SC_VPORT_SCISSOR_0_TL.u32All, WINDOW_OFFSET_DISABLE_bit);
	SETfield(evergreen->viewport[id].PA_SC_VPORT_SCISSOR_0_TL.u32All, x1,
		 PA_SC_VPORT_SCISSOR_0_TL__TL_X_shift, EG_PA_SC_VPORT_SCISSOR_0_TL__TL_X_mask);
	SETfield(evergreen->viewport[id].PA_SC_VPORT_SCISSOR_0_TL.u32All, y1,
		 PA_SC_VPORT_SCISSOR_0_TL__TL_Y_shift, EG_PA_SC_VPORT_SCISSOR_0_TL__TL_Y_mask);
	SETfield(evergreen->viewport[id].PA_SC_VPORT_SCISSOR_0_BR.u32All, x2,
		 PA_SC_VPORT_SCISSOR_0_BR__BR_X_shift, EG_PA_SC_VPORT_SCISSOR_0_BR__BR_X_mask);
	SETfield(evergreen->viewport[id].PA_SC_VPORT_SCISSOR_0_BR.u32All, y2,
		 PA_SC_VPORT_SCISSOR_0_BR__BR_Y_shift, EG_PA_SC_VPORT_SCISSOR_0_BR__BR_Y_mask);

	evergreen->viewport[id].enabled = GL_TRUE;
}

static void evergreenUpdateWindow(struct gl_context * ctx, int id) //diff in calling evergreenSetScissor
{
	context_t *context = EVERGREEN_CONTEXT(ctx);
	EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);
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

	EVERGREEN_STATECHANGE(context, pa);
	

	evergreen->viewport[id].PA_CL_VPORT_XSCALE.f32All  = sx;
	evergreen->viewport[id].PA_CL_VPORT_XOFFSET.f32All = tx;

	evergreen->viewport[id].PA_CL_VPORT_YSCALE.f32All  = sy;
	evergreen->viewport[id].PA_CL_VPORT_YOFFSET.f32All = ty;

	evergreen->viewport[id].PA_CL_VPORT_ZSCALE.f32All  = sz;
	evergreen->viewport[id].PA_CL_VPORT_ZOFFSET.f32All = tz;

	if (ctx->Transform.DepthClamp) {
		evergreen->viewport[id].PA_SC_VPORT_ZMIN_0.f32All = MIN2(ctx->Viewport.Near, ctx->Viewport.Far);
		evergreen->viewport[id].PA_SC_VPORT_ZMAX_0.f32All = MAX2(ctx->Viewport.Near, ctx->Viewport.Far);
		SETbit(evergreen->PA_CL_CLIP_CNTL.u32All, ZCLIP_NEAR_DISABLE_bit);
		SETbit(evergreen->PA_CL_CLIP_CNTL.u32All, ZCLIP_FAR_DISABLE_bit);
	} else {
		evergreen->viewport[id].PA_SC_VPORT_ZMIN_0.f32All = 0.0;
		evergreen->viewport[id].PA_SC_VPORT_ZMAX_0.f32All = 1.0;
		CLEARbit(evergreen->PA_CL_CLIP_CNTL.u32All, ZCLIP_NEAR_DISABLE_bit);
		CLEARbit(evergreen->PA_CL_CLIP_CNTL.u32All, ZCLIP_FAR_DISABLE_bit);
	}

	evergreen->viewport[id].enabled = GL_TRUE;

	evergreenSetScissor(context);
}

static void evergreenEnable(struct gl_context * ctx, GLenum cap, GLboolean state) //diff in func calls
{
	context_t *context = EVERGREEN_CONTEXT(ctx);

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
		evergreenSetAlphaState(ctx);
		break;
	case GL_COLOR_LOGIC_OP:
		evergreenSetLogicOpState(ctx);
		/* fall-through, because logic op overrides blending */
	case GL_BLEND:
		evergreenSetBlendState(ctx);
		break;
	case GL_CLIP_PLANE0:
	case GL_CLIP_PLANE1:
	case GL_CLIP_PLANE2:
	case GL_CLIP_PLANE3:
	case GL_CLIP_PLANE4:
	case GL_CLIP_PLANE5:
		evergreenSetClipPlaneState(ctx, cap, state);
		break;
	case GL_DEPTH_TEST:
		evergreenSetDepthState(ctx);
		break;
	case GL_STENCIL_TEST:
		evergreenSetStencilState(ctx, state);
		break;
	case GL_CULL_FACE:
		evergreenUpdateCulling(ctx);
		break;
	case GL_POLYGON_OFFSET_POINT:
	case GL_POLYGON_OFFSET_LINE:
	case GL_POLYGON_OFFSET_FILL:
		evergreenSetPolygonOffsetState(ctx, state);
		break;
	case GL_SCISSOR_TEST:
		radeon_firevertices(&context->radeon);
		context->radeon.state.scissor.enabled = state;
		radeonUpdateScissor(ctx);
		break;
	case GL_LINE_STIPPLE:
		evergreenUpdateLineStipple(ctx);
		break;
	case GL_DEPTH_CLAMP:
		evergreenUpdateWindow(ctx, 0);
		break;
	default:
		break;
	}

}

static void evergreenColorMask(struct gl_context * ctx,
			  GLboolean r, GLboolean g, GLboolean b, GLboolean a) //same
{
	context_t *context = EVERGREEN_CONTEXT(ctx);
	EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);
	unsigned int mask = ((r ? 1 : 0) |
			     (g ? 2 : 0) |
			     (b ? 4 : 0) |
			     (a ? 8 : 0));

	if (mask != evergreen->CB_TARGET_MASK.u32All) {
		EVERGREEN_STATECHANGE(context, cb);
		SETfield(evergreen->CB_TARGET_MASK.u32All, mask, TARGET0_ENABLE_shift, TARGET0_ENABLE_mask);
	}
}

static void evergreenDepthFunc(struct gl_context * ctx, GLenum func) //same
{
    evergreenSetDepthState(ctx);
}

static void evergreenDepthMask(struct gl_context * ctx, GLboolean mask) //same
{
    evergreenSetDepthState(ctx);
}

static void evergreenCullFace(struct gl_context * ctx, GLenum mode) //same
{
    evergreenUpdateCulling(ctx);
}

static void evergreenFogfv(struct gl_context * ctx, GLenum pname, const GLfloat * param) //same
{
}

static void evergreenUpdatePolygonMode(struct gl_context * ctx) //same
{
	context_t *context = EVERGREEN_CONTEXT(ctx);
	EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);

	EVERGREEN_STATECHANGE(context, pa);

	SETfield(evergreen->PA_SU_SC_MODE_CNTL.u32All, X_DISABLE_POLY_MODE, POLY_MODE_shift, POLY_MODE_mask);

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
		SETfield(evergreen->PA_SU_SC_MODE_CNTL.u32All, X_DUAL_MODE, POLY_MODE_shift, POLY_MODE_mask);

		switch (f) {
		case GL_LINE:
			SETfield(evergreen->PA_SU_SC_MODE_CNTL.u32All, X_DRAW_LINES,
				 POLYMODE_FRONT_PTYPE_shift, POLYMODE_FRONT_PTYPE_mask);
			break;
		case GL_POINT:
			SETfield(evergreen->PA_SU_SC_MODE_CNTL.u32All, X_DRAW_POINTS,
				 POLYMODE_FRONT_PTYPE_shift, POLYMODE_FRONT_PTYPE_mask);
			break;
		case GL_FILL:
			SETfield(evergreen->PA_SU_SC_MODE_CNTL.u32All, X_DRAW_TRIANGLES,
				 POLYMODE_FRONT_PTYPE_shift, POLYMODE_FRONT_PTYPE_mask);
			break;
		}

		switch (b) {
		case GL_LINE:
			SETfield(evergreen->PA_SU_SC_MODE_CNTL.u32All, X_DRAW_LINES,
				 POLYMODE_BACK_PTYPE_shift, POLYMODE_BACK_PTYPE_mask);
			break;
		case GL_POINT:
			SETfield(evergreen->PA_SU_SC_MODE_CNTL.u32All, X_DRAW_POINTS,
				 POLYMODE_BACK_PTYPE_shift, POLYMODE_BACK_PTYPE_mask);
			break;
		case GL_FILL:
			SETfield(evergreen->PA_SU_SC_MODE_CNTL.u32All, X_DRAW_TRIANGLES,
				 POLYMODE_BACK_PTYPE_shift, POLYMODE_BACK_PTYPE_mask);
			break;
		}
	}
}

static void evergreenFrontFace(struct gl_context * ctx, GLenum mode) //same
{
    evergreenUpdateCulling(ctx);
    evergreenUpdatePolygonMode(ctx);
}

static void evergreenShadeModel(struct gl_context * ctx, GLenum mode) //same
{
	context_t *context = EVERGREEN_CONTEXT(ctx);
	EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);

	EVERGREEN_STATECHANGE(context, spi);

	/* also need to set/clear FLAT_SHADE bit per param in SPI_PS_INPUT_CNTL_[0-31] */
	switch (mode) {
	case GL_FLAT:
		SETbit(evergreen->SPI_INTERP_CONTROL_0.u32All, FLAT_SHADE_ENA_bit);
		break;
	case GL_SMOOTH:
		CLEARbit(evergreen->SPI_INTERP_CONTROL_0.u32All, FLAT_SHADE_ENA_bit);
		break;
	default:
		return;
	}
}

static void evergreenLogicOpcode(struct gl_context *ctx, GLenum logicop) //diff
{
	if (_mesa_rgba_logicop_enabled(ctx))
		evergreenSetLogicOpState(ctx);
}

static void evergreenPointSize(struct gl_context * ctx, GLfloat size) //same
{
	context_t *context = EVERGREEN_CONTEXT(ctx);
	EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);

	EVERGREEN_STATECHANGE(context, pa);

	/* We need to clamp to user defined range here, because
	 * the HW clamping happens only for per vertex point size. */
	size = CLAMP(size, ctx->Point.MinSize, ctx->Point.MaxSize);

	/* same size limits for AA, non-AA points */
	size = CLAMP(size, ctx->Const.MinPointSize, ctx->Const.MaxPointSize);

	/* format is 12.4 fixed point */
	SETfield(evergreen->PA_SU_POINT_SIZE.u32All, (int)(size * 8.0),
		 PA_SU_POINT_SIZE__HEIGHT_shift, PA_SU_POINT_SIZE__HEIGHT_mask);
	SETfield(evergreen->PA_SU_POINT_SIZE.u32All, (int)(size * 8.0),
		 PA_SU_POINT_SIZE__WIDTH_shift, PA_SU_POINT_SIZE__WIDTH_mask);

}

static void evergreenPointParameter(struct gl_context * ctx, GLenum pname, const GLfloat * param) //same
{
	context_t *context = EVERGREEN_CONTEXT(ctx);
	EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);

	EVERGREEN_STATECHANGE(context, pa);

	/* format is 12.4 fixed point */
	switch (pname) {
	case GL_POINT_SIZE_MIN:
		SETfield(evergreen->PA_SU_POINT_MINMAX.u32All, (int)(ctx->Point.MinSize * 8.0),
			 MIN_SIZE_shift, MIN_SIZE_mask);
		evergreenPointSize(ctx, ctx->Point.Size);
		break;
	case GL_POINT_SIZE_MAX:
		SETfield(evergreen->PA_SU_POINT_MINMAX.u32All, (int)(ctx->Point.MaxSize * 8.0),
			 MAX_SIZE_shift, MAX_SIZE_mask);
		evergreenPointSize(ctx, ctx->Point.Size);
		break;
	case GL_POINT_DISTANCE_ATTENUATION:
		break;
	case GL_POINT_FADE_THRESHOLD_SIZE:
		break;
	default:
		break;
	}
}

static int evergreen_translate_stencil_func(int func) //same
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

static void evergreenStencilFuncSeparate(struct gl_context * ctx, GLenum face,
				    GLenum func, GLint ref, GLuint mask) //same
{
	context_t *context = EVERGREEN_CONTEXT(ctx);
	EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);
	const unsigned back = ctx->Stencil._BackFace;

	
	EVERGREEN_STATECHANGE(context, db);

	//front
	SETfield(evergreen->DB_STENCILREFMASK.u32All, ctx->Stencil.Ref[0],
		 STENCILREF_shift, STENCILREF_mask);
	SETfield(evergreen->DB_STENCILREFMASK.u32All, ctx->Stencil.ValueMask[0],
		 STENCILMASK_shift, STENCILMASK_mask);

	SETfield(evergreen->DB_DEPTH_CONTROL.u32All, evergreen_translate_stencil_func(ctx->Stencil.Function[0]),
		 STENCILFUNC_shift, STENCILFUNC_mask);

	//back
	SETfield(evergreen->DB_STENCILREFMASK_BF.u32All, ctx->Stencil.Ref[back],
		 STENCILREF_BF_shift, STENCILREF_BF_mask);
	SETfield(evergreen->DB_STENCILREFMASK_BF.u32All, ctx->Stencil.ValueMask[back],
		 STENCILMASK_BF_shift, STENCILMASK_BF_mask);

	SETfield(evergreen->DB_DEPTH_CONTROL.u32All, evergreen_translate_stencil_func(ctx->Stencil.Function[back]),
		 STENCILFUNC_BF_shift, STENCILFUNC_BF_mask);
}

static void evergreenStencilMaskSeparate(struct gl_context * ctx, GLenum face, GLuint mask) //same
{
	context_t *context = EVERGREEN_CONTEXT(ctx);
	EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);
	const unsigned back = ctx->Stencil._BackFace;

	EVERGREEN_STATECHANGE(context, db);

	// front
	SETfield(evergreen->DB_STENCILREFMASK.u32All, ctx->Stencil.WriteMask[0],
		 STENCILWRITEMASK_shift, STENCILWRITEMASK_mask);

	// back
	SETfield(evergreen->DB_STENCILREFMASK_BF.u32All, ctx->Stencil.WriteMask[back],
		 STENCILWRITEMASK_BF_shift, STENCILWRITEMASK_BF_mask);

}

static int evergreen_translate_stencil_op(int op) //same
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

static void evergreenStencilOpSeparate(struct gl_context * ctx, GLenum face,
				  GLenum fail, GLenum zfail, GLenum zpass) //same
{
	context_t *context = EVERGREEN_CONTEXT(ctx);
	EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);
	const unsigned back = ctx->Stencil._BackFace;

	EVERGREEN_STATECHANGE(context, db);

	SETfield(evergreen->DB_DEPTH_CONTROL.u32All, evergreen_translate_stencil_op(ctx->Stencil.FailFunc[0]),
		 STENCILFAIL_shift, STENCILFAIL_mask);
	SETfield(evergreen->DB_DEPTH_CONTROL.u32All, evergreen_translate_stencil_op(ctx->Stencil.ZFailFunc[0]),
		 STENCILZFAIL_shift, STENCILZFAIL_mask);
	SETfield(evergreen->DB_DEPTH_CONTROL.u32All, evergreen_translate_stencil_op(ctx->Stencil.ZPassFunc[0]),
		 STENCILZPASS_shift, STENCILZPASS_mask);

	SETfield(evergreen->DB_DEPTH_CONTROL.u32All, evergreen_translate_stencil_op(ctx->Stencil.FailFunc[back]),
		 STENCILFAIL_BF_shift, STENCILFAIL_BF_mask);
	SETfield(evergreen->DB_DEPTH_CONTROL.u32All, evergreen_translate_stencil_op(ctx->Stencil.ZFailFunc[back]),
		 STENCILZFAIL_BF_shift, STENCILZFAIL_BF_mask);
	SETfield(evergreen->DB_DEPTH_CONTROL.u32All, evergreen_translate_stencil_op(ctx->Stencil.ZPassFunc[back]),
		 STENCILZPASS_BF_shift, STENCILZPASS_BF_mask);
}

static void evergreenViewport(struct gl_context * ctx,
                         GLint x,
                         GLint y,
			 GLsizei width,
                         GLsizei height) //diff in evergreenUpdateWindow
{
	evergreenUpdateWindow(ctx, 0);

	radeon_viewport(ctx, x, y, width, height);
}

static void evergreenDepthRange(struct gl_context * ctx, GLclampd nearval, GLclampd farval) //diff in evergreenUpdateWindow
{
	evergreenUpdateWindow(ctx, 0);
}

static void evergreenLineWidth(struct gl_context * ctx, GLfloat widthf) //same
{
    context_t *context = EVERGREEN_CONTEXT(ctx);
    EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);
    uint32_t lineWidth = (uint32_t)((widthf * 0.5) * (1 << 4));

    EVERGREEN_STATECHANGE(context, pa);

    if (lineWidth > 0xFFFF)
	    lineWidth = 0xFFFF;
    SETfield(evergreen->PA_SU_LINE_CNTL.u32All,(uint16_t)lineWidth,
	     PA_SU_LINE_CNTL__WIDTH_shift, PA_SU_LINE_CNTL__WIDTH_mask);
}

static void evergreenLineStipple(struct gl_context *ctx, GLint factor, GLushort pattern) //same
{
    context_t *context = EVERGREEN_CONTEXT(ctx);
    EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);

    EVERGREEN_STATECHANGE(context, pa);

    SETfield(evergreen->PA_SC_LINE_STIPPLE.u32All, pattern, LINE_PATTERN_shift, LINE_PATTERN_mask);
    SETfield(evergreen->PA_SC_LINE_STIPPLE.u32All, (factor-1), REPEAT_COUNT_shift, REPEAT_COUNT_mask);
    SETfield(evergreen->PA_SC_LINE_STIPPLE.u32All, 1, AUTO_RESET_CNTL_shift, AUTO_RESET_CNTL_mask);
}

static void evergreenPolygonOffset(struct gl_context * ctx, GLfloat factor, GLfloat units) //diff :
                                                                    //all register here offset diff, bits same
{
	context_t *context = EVERGREEN_CONTEXT(ctx);
	EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);
	GLfloat constant = units;
	GLchar depth = 0;

	EVERGREEN_STATECHANGE(context, pa);

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
	SETfield(evergreen->PA_SU_POLY_OFFSET_DB_FMT_CNTL.u32All, depth,
		 POLY_OFFSET_NEG_NUM_DB_BITS_shift, POLY_OFFSET_NEG_NUM_DB_BITS_mask);
	//evergreen->PA_SU_POLY_OFFSET_CLAMP.f32All = constant; //???
	evergreen->PA_SU_POLY_OFFSET_FRONT_SCALE.f32All = factor;
	evergreen->PA_SU_POLY_OFFSET_FRONT_OFFSET.f32All = constant;
	evergreen->PA_SU_POLY_OFFSET_BACK_SCALE.f32All = factor;
	evergreen->PA_SU_POLY_OFFSET_BACK_OFFSET.f32All = constant;
}

static void evergreenPolygonMode(struct gl_context * ctx, GLenum face, GLenum mode) //same
{
	(void)face;
	(void)mode;

	evergreenUpdatePolygonMode(ctx);
}

static void evergreenRenderMode(struct gl_context * ctx, GLenum mode) //same
{
}

//TODO : move to kernel.
static void evergreenInitSQConfig(struct gl_context * ctx)
{
    context_t *context = EVERGREEN_CONTEXT(ctx);
    EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context); 
    
    uint32_t  uSqNumCfInsts, uMaxGPRs, uMaxThreads, uMaxStackEntries, uPSThreadCount, uOtherThreadCount;
    uint32_t  NUM_PS_GPRS, NUM_VS_GPRS, NUM_GS_GPRS, NUM_ES_GPRS, NUM_HS_GPRS, NUM_LS_GPRS, NUM_CLAUSE_TEMP_GPRS;
    GLboolean bVC_ENABLE = GL_TRUE;

    R600_STATECHANGE(context, sq);

    switch (context->radeon.radeonScreen->chip_family) 
    {
    case CHIP_FAMILY_CEDAR:
	    uSqNumCfInsts       = 1;
        bVC_ENABLE = GL_FALSE;
        uMaxGPRs = 256;
        uPSThreadCount = 96;
        uMaxThreads = 192;
        uMaxStackEntries = 256;
	    break;
    case CHIP_FAMILY_REDWOOD:
	    uSqNumCfInsts       = 2;
        bVC_ENABLE = GL_TRUE;
        uMaxGPRs = 256;
        uPSThreadCount = 128;
        uMaxThreads = 248;
        uMaxStackEntries = 256;
	    break;
    case CHIP_FAMILY_JUNIPER:
        uSqNumCfInsts       = 2;
        bVC_ENABLE = GL_TRUE;
        uMaxGPRs = 256;
        uPSThreadCount = 128;
        uMaxThreads = 248;
        uMaxStackEntries = 512;
	    break;
    case CHIP_FAMILY_CYPRESS:
	    uSqNumCfInsts       = 2;
        bVC_ENABLE = GL_TRUE;
        uMaxGPRs = 256;
        uPSThreadCount = 128;
        uMaxThreads = 248;
        uMaxStackEntries = 512;
	    break;
    case CHIP_FAMILY_HEMLOCK:
	    uSqNumCfInsts       = 2;//?
        bVC_ENABLE = GL_TRUE;
        uMaxGPRs = 256;
        uPSThreadCount = 128;
        uMaxThreads = 248;
        uMaxStackEntries = 512;
	    break;
    case CHIP_FAMILY_PALM:
	    uSqNumCfInsts       = 1;
        bVC_ENABLE = GL_FALSE;
        uMaxGPRs = 256;
        uPSThreadCount = 96;
        uMaxThreads = 192;
        uMaxStackEntries = 256;
	    break;
    case CHIP_FAMILY_SUMO:
	    uSqNumCfInsts       = 2;
        bVC_ENABLE = GL_FALSE;
        uMaxGPRs = 256;
        uPSThreadCount = 96;
        uMaxThreads = 248;
        uMaxStackEntries = 256;
	    break;
    case CHIP_FAMILY_SUMO2:
	    uSqNumCfInsts       = 2;
        bVC_ENABLE = GL_FALSE;
        uMaxGPRs = 256;
        uPSThreadCount = 96;
        uMaxThreads = 248;
        uMaxStackEntries = 512;
	    break;
    case CHIP_FAMILY_BARTS:
	    uSqNumCfInsts       = 2;
        bVC_ENABLE = GL_TRUE;
        uMaxGPRs = 256;
        uPSThreadCount = 128;
        uMaxThreads = 248;
        uMaxStackEntries = 512;
	    break;
    case CHIP_FAMILY_TURKS:
	    uSqNumCfInsts       = 2;
        bVC_ENABLE = GL_TRUE;
        uMaxGPRs = 256;
        uPSThreadCount = 128;
        uMaxThreads = 248;
        uMaxStackEntries = 256;
	    break;
    case CHIP_FAMILY_CAICOS:
	    uSqNumCfInsts       = 1;
        bVC_ENABLE = GL_FALSE;
        uMaxGPRs = 256;
        uPSThreadCount = 128;
        uMaxThreads = 192;
        uMaxStackEntries = 256;
	    break;
    default:
        uSqNumCfInsts       = 2;
        bVC_ENABLE = GL_TRUE;
        uMaxGPRs = 256;
        uPSThreadCount = 128;
        uMaxThreads = 248;
        uMaxStackEntries = 512;
	    break;
    }

    evergreen->evergreen_config.SQ_DYN_GPR_CNTL_PS_FLUSH_REQ.u32All   = 0;

    evergreen->evergreen_config.SPI_CONFIG_CNTL.u32All   = 0;
    evergreen->evergreen_config.SPI_CONFIG_CNTL_1.u32All = 0;
    SETfield(evergreen->evergreen_config.SPI_CONFIG_CNTL_1.u32All, 4, 
             EG_SPI_CONFIG_CNTL_1__VTX_DONE_DELAY_shift, 
             EG_SPI_CONFIG_CNTL_1__VTX_DONE_DELAY_mask);

    evergreen->evergreen_config.CP_PERFMON_CNTL.u32All = 0;
    
    evergreen->evergreen_config.SQ_MS_FIFO_SIZES.u32All = 0;
    SETfield(evergreen->evergreen_config.SQ_MS_FIFO_SIZES.u32All, 16 * uSqNumCfInsts, 
             EG_SQ_MS_FIFO_SIZES__CACHE_FIFO_SIZE_shift, 
             EG_SQ_MS_FIFO_SIZES__CACHE_FIFO_SIZE_mask);
    SETfield(evergreen->evergreen_config.SQ_MS_FIFO_SIZES.u32All, 0x4, 
             EG_SQ_MS_FIFO_SIZES__FETCH_FIFO_HIWATER_shift, 
             EG_SQ_MS_FIFO_SIZES__FETCH_FIFO_HIWATER_mask);
    SETfield(evergreen->evergreen_config.SQ_MS_FIFO_SIZES.u32All, 0xE0, 
             EG_SQ_MS_FIFO_SIZES__DONE_FIFO_HIWATER_shift, 
             EG_SQ_MS_FIFO_SIZES__DONE_FIFO_HIWATER_mask);
    SETfield(evergreen->evergreen_config.SQ_MS_FIFO_SIZES.u32All, 0x8, 
             EG_SQ_MS_FIFO_SIZES__ALU_UPDATE_FIFO_HIWATER_shift, 
             EG_SQ_MS_FIFO_SIZES__ALU_UPDATE_FIFO_HIWATER_mask);
    
    if(bVC_ENABLE == GL_TRUE)
    {
        SETbit(evergreen->evergreen_config.SQ_CONFIG.u32All, 
               EG_SQ_CONFIG__VC_ENABLE_bit);
    }
    else
    {
        CLEARbit(evergreen->evergreen_config.SQ_CONFIG.u32All, 
                 EG_SQ_CONFIG__VC_ENABLE_bit);
    }
    SETbit(evergreen->evergreen_config.SQ_CONFIG.u32All, 
           EG_SQ_CONFIG__EXPORT_SRC_C_bit);
    SETfield(evergreen->evergreen_config.SQ_CONFIG.u32All, 0, 
             EG_SQ_CONFIG__PS_PRIO_shift, 
             EG_SQ_CONFIG__PS_PRIO_mask);
    SETfield(evergreen->evergreen_config.SQ_CONFIG.u32All, 1, 
             EG_SQ_CONFIG__VS_PRIO_shift, 
             EG_SQ_CONFIG__VS_PRIO_mask);
    SETfield(evergreen->evergreen_config.SQ_CONFIG.u32All, 2, 
             EG_SQ_CONFIG__GS_PRIO_shift, 
             EG_SQ_CONFIG__GS_PRIO_mask);
    SETfield(evergreen->evergreen_config.SQ_CONFIG.u32All, 3, 
             EG_SQ_CONFIG__ES_PRIO_shift, 
             EG_SQ_CONFIG__ES_PRIO_mask);

    NUM_CLAUSE_TEMP_GPRS = 4; 
    NUM_PS_GPRS = ((uMaxGPRs-(4*2))*12/32); // 93
    NUM_VS_GPRS = ((uMaxGPRs-(4*2))*6/32);  // 46    
    NUM_GS_GPRS = ((uMaxGPRs-(4*2))*4/32);  // 31
    NUM_ES_GPRS = ((uMaxGPRs-(4*2))*4/32);  // 31
    NUM_HS_GPRS = ((uMaxGPRs-(4*2))*3/32);  // 23
    NUM_LS_GPRS = ((uMaxGPRs-(4*2))*3/32);  // 23

    evergreen->evergreen_config.SQ_GPR_RESOURCE_MGMT_1.u32All = 0;
    evergreen->evergreen_config.SQ_GPR_RESOURCE_MGMT_2.u32All = 0;
    evergreen->evergreen_config.SQ_GPR_RESOURCE_MGMT_3.u32All = 0;

    SETfield(evergreen->evergreen_config.SQ_GPR_RESOURCE_MGMT_1.u32All, NUM_PS_GPRS, 
             NUM_PS_GPRS_shift, NUM_PS_GPRS_mask);
    SETfield(evergreen->evergreen_config.SQ_GPR_RESOURCE_MGMT_1.u32All, NUM_VS_GPRS, 
             NUM_VS_GPRS_shift, NUM_VS_GPRS_mask);
    SETfield(evergreen->evergreen_config.SQ_GPR_RESOURCE_MGMT_1.u32All, NUM_CLAUSE_TEMP_GPRS,
	         NUM_CLAUSE_TEMP_GPRS_shift, NUM_CLAUSE_TEMP_GPRS_mask);
    SETfield(evergreen->evergreen_config.SQ_GPR_RESOURCE_MGMT_2.u32All, NUM_GS_GPRS, 
             NUM_GS_GPRS_shift, NUM_GS_GPRS_mask);
    SETfield(evergreen->evergreen_config.SQ_GPR_RESOURCE_MGMT_2.u32All, NUM_ES_GPRS, 
             NUM_ES_GPRS_shift, NUM_ES_GPRS_mask);
    SETfield(evergreen->evergreen_config.SQ_GPR_RESOURCE_MGMT_3.u32All, NUM_HS_GPRS, 
             NUM_PS_GPRS_shift, NUM_PS_GPRS_mask);
    SETfield(evergreen->evergreen_config.SQ_GPR_RESOURCE_MGMT_3.u32All, NUM_LS_GPRS, 
             NUM_VS_GPRS_shift, NUM_VS_GPRS_mask);

    uOtherThreadCount = (((uMaxThreads-uPSThreadCount)/6)/8)*8;
    evergreen->evergreen_config.SQ_THREAD_RESOURCE_MGMT.u32All = 0;
    evergreen->evergreen_config.SQ_THREAD_RESOURCE_MGMT_2.u32All = 0;
    SETfield(evergreen->evergreen_config.SQ_THREAD_RESOURCE_MGMT.u32All, uPSThreadCount,
	     NUM_PS_THREADS_shift, NUM_PS_THREADS_mask);
    SETfield(evergreen->evergreen_config.SQ_THREAD_RESOURCE_MGMT.u32All, uOtherThreadCount,
	     NUM_VS_THREADS_shift, NUM_VS_THREADS_mask);
    SETfield(evergreen->evergreen_config.SQ_THREAD_RESOURCE_MGMT.u32All, uOtherThreadCount,
	     NUM_GS_THREADS_shift, NUM_GS_THREADS_mask);
    SETfield(evergreen->evergreen_config.SQ_THREAD_RESOURCE_MGMT.u32All, uOtherThreadCount,
	     NUM_ES_THREADS_shift, NUM_ES_THREADS_mask);
    SETfield(evergreen->evergreen_config.SQ_THREAD_RESOURCE_MGMT_2.u32All, uOtherThreadCount,
	     NUM_PS_THREADS_shift, NUM_PS_THREADS_mask);
    SETfield(evergreen->evergreen_config.SQ_THREAD_RESOURCE_MGMT_2.u32All, uOtherThreadCount,
	     NUM_VS_THREADS_shift, NUM_VS_THREADS_mask);

    uMaxStackEntries = ((uMaxStackEntries*1)/6);
    evergreen->evergreen_config.SQ_STACK_RESOURCE_MGMT_1.u32All = 0;
    evergreen->evergreen_config.SQ_STACK_RESOURCE_MGMT_2.u32All = 0;
    evergreen->evergreen_config.SQ_STACK_RESOURCE_MGMT_3.u32All = 0;
    SETfield(evergreen->evergreen_config.SQ_STACK_RESOURCE_MGMT_1.u32All, uMaxStackEntries,
	         NUM_PS_STACK_ENTRIES_shift, NUM_PS_STACK_ENTRIES_mask);
    SETfield(evergreen->evergreen_config.SQ_STACK_RESOURCE_MGMT_1.u32All, uMaxStackEntries,
	         NUM_VS_STACK_ENTRIES_shift, NUM_VS_STACK_ENTRIES_mask);    
    SETfield(evergreen->evergreen_config.SQ_STACK_RESOURCE_MGMT_2.u32All, uMaxStackEntries,
	         NUM_GS_STACK_ENTRIES_shift, NUM_GS_STACK_ENTRIES_mask);
    SETfield(evergreen->evergreen_config.SQ_STACK_RESOURCE_MGMT_2.u32All, uMaxStackEntries,
	         NUM_ES_STACK_ENTRIES_shift, NUM_ES_STACK_ENTRIES_mask);
    SETfield(evergreen->evergreen_config.SQ_STACK_RESOURCE_MGMT_3.u32All, uMaxStackEntries,
	         NUM_PS_STACK_ENTRIES_shift, NUM_PS_STACK_ENTRIES_mask);
    SETfield(evergreen->evergreen_config.SQ_STACK_RESOURCE_MGMT_3.u32All, uMaxStackEntries,
	         NUM_VS_STACK_ENTRIES_shift, NUM_VS_STACK_ENTRIES_mask); 

    evergreen->evergreen_config.PA_SC_FORCE_EOV_MAX_CNTS.u32All = 0;
    SETfield(evergreen->evergreen_config.PA_SC_FORCE_EOV_MAX_CNTS.u32All, 4095,
	         EG_PA_SC_FORCE_EOV_MAX_CNTS__FORCE_EOV_MAX_CLK_CNT_shift, 
             EG_PA_SC_FORCE_EOV_MAX_CNTS__FORCE_EOV_MAX_CLK_CNT_mask); 
    SETfield(evergreen->evergreen_config.PA_SC_FORCE_EOV_MAX_CNTS.u32All, 255,
	         EG_PA_SC_FORCE_EOV_MAX_CNTS__FORCE_EOV_MAX_REZ_CNT_shift, 
             EG_PA_SC_FORCE_EOV_MAX_CNTS__FORCE_EOV_MAX_REZ_CNT_mask); 
    
    evergreen->evergreen_config.VGT_CACHE_INVALIDATION.u32All = 0;
    SETfield(evergreen->evergreen_config.VGT_CACHE_INVALIDATION.u32All, 2,
	         EG_VGT_CACHE_INVALIDATION__CACHE_INVALIDATION_shift, 
             EG_VGT_CACHE_INVALIDATION__CACHE_INVALIDATION_mask); 
   
    evergreen->evergreen_config.VGT_GS_VERTEX_REUSE.u32All = 0;
    SETfield(evergreen->evergreen_config.VGT_GS_VERTEX_REUSE.u32All, 16,
	         VERT_REUSE_shift, 
             VERT_REUSE_mask); 

    evergreen->evergreen_config.PA_SC_LINE_STIPPLE_STATE.u32All = 0;

    evergreen->evergreen_config.PA_CL_ENHANCE.u32All = 0;
    SETbit(evergreen->evergreen_config.PA_CL_ENHANCE.u32All, 
           CLIP_VTX_REORDER_ENA_bit);
    SETfield(evergreen->evergreen_config.PA_CL_ENHANCE.u32All, 3,
	         NUM_CLIP_SEQ_shift, 
             NUM_CLIP_SEQ_mask);     
}

void evergreenInitState(struct gl_context * ctx) //diff
{
    context_t *context = R700_CONTEXT(ctx);
    EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);

    int id = 0;
    
    //calloc should have done this
    memset(evergreen, 0, sizeof(EVERGREEN_CHIP_CONTEXT));
    
    // Disable window clipping and offset:
    SETfield(evergreen->PA_SC_WINDOW_OFFSET.u32All, 0,
		 EG_PA_SC_WINDOW_OFFSET__WINDOW_X_OFFSET_shift, EG_PA_SC_WINDOW_OFFSET__WINDOW_X_OFFSET_mask);
    SETfield(evergreen->PA_SC_WINDOW_OFFSET.u32All, 0,
		 EG_PA_SC_WINDOW_OFFSET__WINDOW_Y_OFFSET_shift, EG_PA_SC_WINDOW_OFFSET__WINDOW_Y_OFFSET_mask);

    SETbit(evergreen->PA_SC_WINDOW_SCISSOR_TL.u32All, WINDOW_OFFSET_DISABLE_bit);    
        
    evergreen->PA_SC_CLIPRECT_RULE.u32All = 0x0000FFFF;

    evergreen->PA_SC_EDGERULE.u32All = 0xAAAAAAAA;

    // Set up Z min/max:
    evergreen->viewport[id].PA_SC_VPORT_ZMIN_0.f32All = 0.0; 
    evergreen->viewport[id].PA_SC_VPORT_ZMAX_0.f32All = 1.0;

    SETfield(evergreen->CB_TARGET_MASK.u32All, 0xF, TARGET0_ENABLE_shift, TARGET0_ENABLE_mask);
    SETfield(evergreen->CB_SHADER_MASK.u32All, 0xF, OUTPUT0_ENABLE_shift, OUTPUT0_ENABLE_mask);
    
    SETfield(evergreen->SPI_BARYC_CNTL.u32All, 1,
		 EG_SPI_BARYC_CNTL__PERSP_CENTROID_ENA_shift, 
         EG_SPI_BARYC_CNTL__PERSP_CENTROID_ENA_mask);
    SETfield(evergreen->SPI_BARYC_CNTL.u32All, 1,
		 EG_SPI_BARYC_CNTL__LINEAR_CENTROID_ENA_shift, 
         EG_SPI_BARYC_CNTL__LINEAR_CENTROID_ENA_mask);
    
    // Turn off vgt reuse:
    evergreen->VGT_REUSE_OFF.u32All = 0;
    SETbit(evergreen->VGT_REUSE_OFF.u32All, REUSE_OFF_bit);    

    // Specify offsetting and clamp values for vertices:
    evergreen->VGT_MAX_VTX_INDX.u32All      = 0xFFFFFF;
    evergreen->VGT_MIN_VTX_INDX.u32All      = 0;
    evergreen->VGT_INDX_OFFSET.u32All       = 0;

    evergreen->VGT_DMA_NUM_INSTANCES.u32All = 1;

    // Do not alpha blend:
    SETfield(evergreen->SX_ALPHA_TEST_CONTROL.u32All, REF_NEVER,
			 ALPHA_FUNC_shift, ALPHA_FUNC_mask);
    CLEARbit(evergreen->SX_ALPHA_TEST_CONTROL.u32All, ALPHA_TEST_ENABLE_bit);    

    evergreen->SPI_VS_OUT_ID_0.u32All  = 0x03020100;
    evergreen->SPI_VS_OUT_ID_1.u32All  = 0x07060504;
    
    evergreen->SPI_PS_INPUT_CNTL[0].u32All  = 0x00000800;
    evergreen->SPI_PS_INPUT_CNTL[1].u32All  = 0x00000801;
    evergreen->SPI_PS_INPUT_CNTL[2].u32All  = 0x00000802;


    // Depth buffer currently disabled:    
    evergreen->DB_DEPTH_CONTROL.u32All = 0;
    SETbit(evergreen->DB_DEPTH_CONTROL.u32All, Z_WRITE_ENABLE_bit);
    SETfield(evergreen->DB_DEPTH_CONTROL.u32All, FRAG_ALWAYS,
                     ZFUNC_shift, ZFUNC_mask);    

    evergreen->DB_Z_READ_BASE.u32All = 0;
    evergreen->DB_Z_WRITE_BASE.u32All = 0;

    evergreen->DB_DEPTH_CLEAR.f32All = 1.0;

    evergreen->DB_DEPTH_VIEW.u32All = 0;

    evergreen->DB_SHADER_CONTROL.u32All = 0;
    SETbit(evergreen->DB_SHADER_CONTROL.u32All, EG_DB_SHADER_CONTROL__DUAL_EXPORT_ENABLE_bit);    

    evergreen->DB_Z_INFO.u32All = 0;
    SETfield(evergreen->DB_Z_INFO.u32All   , ARRAY_1D_TILED_THIN1,
        EG_DB_Z_INFO__ARRAY_MODE_shift, EG_DB_Z_INFO__ARRAY_MODE_mask);
    SETfield(evergreen->DB_Z_INFO.u32All       , EG_Z_24,
        EG_DB_Z_INFO__FORMAT_shift, EG_DB_Z_INFO__FORMAT_mask);
    SETfield(evergreen->DB_Z_INFO.u32All   , EG_ADDR_SURF_TILE_SPLIT_256B,
        EG_DB_Z_INFO__TILE_SPLIT_shift, EG_DB_Z_INFO__TILE_SPLIT_mask);
    SETfield(evergreen->DB_Z_INFO.u32All    , EG_ADDR_SURF_8_BANK,
        EG_DB_Z_INFO__NUM_BANKS_shift, EG_DB_Z_INFO__NUM_BANKS_mask);
    SETfield(evergreen->DB_Z_INFO.u32All   , EG_ADDR_SURF_BANK_WIDTH_1,
        EG_DB_Z_INFO__BANK_WIDTH_shift, EG_DB_Z_INFO__BANK_WIDTH_mask);
    SETfield(evergreen->DB_Z_INFO.u32All  , EG_ADDR_SURF_BANK_HEIGHT_1,
        EG_DB_Z_INFO__BANK_HEIGHT_shift, EG_DB_Z_INFO__BANK_HEIGHT_mask);

    evergreen->DB_STENCIL_INFO.u32All = 0;
    CLEARbit(evergreen->DB_STENCIL_INFO.u32All, EG_DB_STENCIL_INFO__FORMAT_bit);
    SETfield(evergreen->DB_STENCIL_INFO.u32All, 0,
        EG_DB_STENCIL_INFO__TILE_SPLIT_shift, EG_DB_STENCIL_INFO__TILE_SPLIT_mask);

    evergreen->DB_RENDER_CONTROL.u32All = 0;

    evergreen->DB_RENDER_OVERRIDE.u32All = 0;
    SETfield(evergreen->DB_RENDER_OVERRIDE.u32All, FORCE_DISABLE, FORCE_HIZ_ENABLE_shift, FORCE_HIZ_ENABLE_mask);
	SETfield(evergreen->DB_RENDER_OVERRIDE.u32All, FORCE_DISABLE, FORCE_HIS_ENABLE0_shift, FORCE_HIS_ENABLE0_mask);
	SETfield(evergreen->DB_RENDER_OVERRIDE.u32All, FORCE_DISABLE, FORCE_HIS_ENABLE1_shift, FORCE_HIS_ENABLE1_mask);

    /* stencil */
    evergreenEnable(ctx, GL_STENCIL_TEST, ctx->Stencil._Enabled);
    evergreenStencilMaskSeparate(ctx, 0, ctx->Stencil.WriteMask[0]);
    evergreenStencilFuncSeparate(ctx, 0, ctx->Stencil.Function[0],
			                     ctx->Stencil.Ref[0], ctx->Stencil.ValueMask[0]);
    evergreenStencilOpSeparate(ctx, 0, ctx->Stencil.FailFunc[0],
			                   ctx->Stencil.ZFailFunc[0],
			                   ctx->Stencil.ZPassFunc[0]);

    // Disable ROP3 modes by setting src to dst copy:
    SETfield(evergreen->CB_COLOR_CONTROL.u32All, 0xCC, 
             EG_CB_COLOR_CONTROL__ROP3_shift, 
             EG_CB_COLOR_CONTROL__ROP3_mask);    
    SETfield(evergreen->CB_COLOR_CONTROL.u32All, EG_CB_NORMAL,
             EG_CB_COLOR_CONTROL__MODE_shift,
             EG_CB_COLOR_CONTROL__MODE_mask);

    SETfield(evergreen->CB_BLEND0_CONTROL.u32All,
			 BLEND_ONE, COLOR_SRCBLEND_shift, COLOR_SRCBLEND_mask);
	
	SETfield(evergreen->CB_BLEND0_CONTROL.u32All,
		     BLEND_ONE, ALPHA_SRCBLEND_shift, ALPHA_SRCBLEND_mask);

    //evergreen->PA_CL_CLIP_CNTL.CLIP_DISABLE = 1;
    
    SETbit(evergreen->PA_CL_CLIP_CNTL.u32All, DX_LINEAR_ATTR_CLIP_ENA_bit);

    // Set up the culling control register:
    SETfield(evergreen->PA_SU_SC_MODE_CNTL.u32All, 2,
             POLYMODE_FRONT_PTYPE_shift, POLYMODE_FRONT_PTYPE_mask); // draw using triangles
    SETfield(evergreen->PA_SU_SC_MODE_CNTL.u32All, 2,
				 POLYMODE_BACK_PTYPE_shift, POLYMODE_BACK_PTYPE_mask); // draw using triangles
        
    // Do scale XY or X by 1/W0. eg:    
    evergreen->bEnablePerspective = GL_TRUE;
    
    CLEARbit(evergreen->PA_CL_VTE_CNTL.u32All, VTX_XY_FMT_bit);
    CLEARbit(evergreen->PA_CL_VTE_CNTL.u32All, VTX_Z_FMT_bit);
    SETbit(evergreen->PA_CL_VTE_CNTL.u32All, VTX_W0_FMT_bit);

    // Enable viewport scaling for all three axis:
    SETbit(evergreen->PA_CL_VTE_CNTL.u32All, VPORT_X_SCALE_ENA_bit);
    SETbit(evergreen->PA_CL_VTE_CNTL.u32All, VPORT_X_OFFSET_ENA_bit);
    SETbit(evergreen->PA_CL_VTE_CNTL.u32All, VPORT_Y_SCALE_ENA_bit);
    SETbit(evergreen->PA_CL_VTE_CNTL.u32All, VPORT_Y_OFFSET_ENA_bit);
    SETbit(evergreen->PA_CL_VTE_CNTL.u32All, VPORT_Z_SCALE_ENA_bit);
    SETbit(evergreen->PA_CL_VTE_CNTL.u32All, VPORT_Z_OFFSET_ENA_bit);

    // Set up point sizes and min/max values:
    SETfield(evergreen->PA_SU_POINT_SIZE.u32All, 0x8,
		 PA_SU_POINT_SIZE__HEIGHT_shift, PA_SU_POINT_SIZE__HEIGHT_mask);
	SETfield(evergreen->PA_SU_POINT_SIZE.u32All, 0x8,
		 PA_SU_POINT_SIZE__WIDTH_shift, PA_SU_POINT_SIZE__WIDTH_mask);
    CLEARfield(evergreen->PA_SU_POINT_MINMAX.u32All, MIN_SIZE_mask);
    SETfield(evergreen->PA_SU_POINT_MINMAX.u32All, 0x8000, MAX_SIZE_shift, MAX_SIZE_mask);
    SETfield(evergreen->PA_SU_LINE_CNTL.u32All,0x8,
	     PA_SU_LINE_CNTL__WIDTH_shift, PA_SU_LINE_CNTL__WIDTH_mask);   

    // Set up line control:
    evergreen->PA_SC_LINE_CNTL.u32All = 0;
    CLEARbit(evergreen->PA_SC_LINE_CNTL.u32All, EXPAND_LINE_WIDTH_bit);
    SETbit(evergreen->PA_SC_LINE_CNTL.u32All, LAST_PIXEL_bit);    

    // Set up vertex control:
    evergreen->PA_SU_VTX_CNTL.u32All               = 0;
    CLEARfield(evergreen->PA_SU_VTX_CNTL.u32All, QUANT_MODE_mask);  
    SETbit(evergreen->PA_SU_VTX_CNTL.u32All, PIX_CENTER_bit);
    SETfield(evergreen->PA_SU_VTX_CNTL.u32All, X_ROUND_TO_EVEN,
             PA_SU_VTX_CNTL__ROUND_MODE_shift, PA_SU_VTX_CNTL__ROUND_MODE_mask);
        
    // to 1.0 = no guard band:
    evergreen->PA_CL_GB_VERT_CLIP_ADJ.u32All  = 0x3F800000;  // 1.0
    evergreen->PA_CL_GB_VERT_DISC_ADJ.u32All  = 0x3F800000;  // 1.0
    evergreen->PA_CL_GB_HORZ_CLIP_ADJ.u32All  = 0x3F800000;  // 1.0
    evergreen->PA_CL_GB_HORZ_DISC_ADJ.u32All  = 0x3F800000;  // 1.0

    // Diable color compares:    
    SETfield(evergreen->CB_CLRCMP_CONTROL.u32All, CLRCMP_DRAW_ALWAYS,
             CLRCMP_FCN_SRC_shift, CLRCMP_FCN_SRC_mask);
    SETfield(evergreen->CB_CLRCMP_CONTROL.u32All, CLRCMP_DRAW_ALWAYS,
             CLRCMP_FCN_DST_shift, CLRCMP_FCN_DST_mask);
    SETfield(evergreen->CB_CLRCMP_CONTROL.u32All, CLRCMP_SEL_SRC,
             CLRCMP_FCN_SEL_shift, CLRCMP_FCN_SEL_mask);

    // Zero out source:
    evergreen->CB_CLRCMP_SRC.u32All = 0x00000000;

    // Put a compare color in for error checking:
    evergreen->CB_CLRCMP_DST.u32All = 0x000000FF;

    // Set up color compare mask:
    evergreen->CB_CLRCMP_MSK.u32All = 0xFFFFFFFF;

    // Enable all samples for multi-sample anti-aliasing:
    evergreen->PA_SC_AA_MASK.u32All = 0xFFFFFFFF;
    // Turn off AA:
    evergreen->PA_SC_AA_CONFIG.u32All = 0;
    
    SETfield(evergreen->VGT_OUT_DEALLOC_CNTL.u32All, 16,
             DEALLOC_DIST_shift, DEALLOC_DIST_mask);
    SETfield(evergreen->VGT_VERTEX_REUSE_BLOCK_CNTL.u32All, 14,
             VTX_REUSE_DEPTH_shift, VTX_REUSE_DEPTH_mask);
        
    evergreen->SX_MISC.u32All = 0;

    SETfield(evergreen->render_target[id].CB_COLOR0_INFO.u32All, 1,
             EG_CB_COLOR0_INFO__SOURCE_FORMAT_shift, EG_CB_COLOR0_INFO__SOURCE_FORMAT_mask);
    SETbit(evergreen->render_target[id].CB_COLOR0_INFO.u32All, EG_CB_COLOR0_INFO__BLEND_CLAMP_bit);
    SETfield(evergreen->render_target[id].CB_COLOR0_INFO.u32All, 0,
             EG_CB_COLOR0_INFO__NUMBER_TYPE_shift, EG_CB_COLOR0_INFO__NUMBER_TYPE_mask);
    
    SETfield(evergreen->render_target[id].CB_COLOR0_INFO.u32All, SWAP_STD,
             EG_CB_COLOR0_INFO__COMP_SWAP_shift, EG_CB_COLOR0_INFO__COMP_SWAP_mask);

    evergreen->render_target[id].CB_COLOR0_VIEW.u32All   = 0;
    evergreen->render_target[id].CB_COLOR0_CMASK.u32All   = 0;
    evergreen->render_target[id].CB_COLOR0_FMASK.u32All  = 0;
    evergreen->render_target[id].CB_COLOR0_FMASK_SLICE.u32All   = 0; 

    evergreenInitSQConfig(ctx);

    context->radeon.hw.all_dirty = GL_TRUE;
}

void evergreenInitStateFuncs(radeonContextPtr radeon, struct dd_function_table *functions)
{
	functions->UpdateState = evergreenInvalidateState;
	functions->AlphaFunc = evergreenAlphaFunc;
	functions->BlendColor = evergreenBlendColor;
	functions->BlendEquationSeparate = evergreenBlendEquationSeparate;
	functions->BlendFuncSeparate = evergreenBlendFuncSeparate;
	functions->Enable = evergreenEnable;
	functions->ColorMask = evergreenColorMask;
	functions->DepthFunc = evergreenDepthFunc;
	functions->DepthMask = evergreenDepthMask;
	functions->CullFace = evergreenCullFace;
	functions->Fogfv = evergreenFogfv;
	functions->FrontFace = evergreenFrontFace;
	functions->ShadeModel = evergreenShadeModel;
	functions->LogicOpcode = evergreenLogicOpcode;

	/* ARB_point_parameters */
	functions->PointParameterfv = evergreenPointParameter;

	/* Stencil related */
	functions->StencilFuncSeparate = evergreenStencilFuncSeparate;
	functions->StencilMaskSeparate = evergreenStencilMaskSeparate;
	functions->StencilOpSeparate = evergreenStencilOpSeparate;

	/* Viewport related */
	functions->Viewport = evergreenViewport;
	functions->DepthRange = evergreenDepthRange;
	functions->PointSize = evergreenPointSize;
	functions->LineWidth = evergreenLineWidth;
	functions->LineStipple = evergreenLineStipple;

	functions->PolygonOffset = evergreenPolygonOffset;
	functions->PolygonMode = evergreenPolygonMode;

	functions->RenderMode = evergreenRenderMode;

	functions->ClipPlane = evergreenClipPlane;

	functions->Scissor = radeonScissor;

	functions->DrawBuffer = radeonDrawBuffer;
	functions->ReadBuffer = radeonReadBuffer;

	if (radeon->radeonScreen->kernel_mm) {
		functions->CopyPixels = _mesa_meta_CopyPixels;
		functions->DrawPixels = _mesa_meta_DrawPixels;
		functions->ReadPixels = radeonReadPixels;
	}
}



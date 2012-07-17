/*
 * Copyright 2012 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *      Christian König <christian.koenig@amd.com>
 */

#include "util/u_memory.h"
#include "radeonsi_pipe.h"
#include "si_state.h"
#include "sid.h"

/*
 * Blender functions
 */

static uint32_t si_translate_blend_function(int blend_func)
{
	switch (blend_func) {
	case PIPE_BLEND_ADD:
		return V_028780_COMB_DST_PLUS_SRC;
	case PIPE_BLEND_SUBTRACT:
		return V_028780_COMB_SRC_MINUS_DST;
	case PIPE_BLEND_REVERSE_SUBTRACT:
		return V_028780_COMB_DST_MINUS_SRC;
	case PIPE_BLEND_MIN:
		return V_028780_COMB_MIN_DST_SRC;
	case PIPE_BLEND_MAX:
		return V_028780_COMB_MAX_DST_SRC;
	default:
		R600_ERR("Unknown blend function %d\n", blend_func);
		assert(0);
		break;
	}
	return 0;
}

static uint32_t si_translate_blend_factor(int blend_fact)
{
	switch (blend_fact) {
	case PIPE_BLENDFACTOR_ONE:
		return V_028780_BLEND_ONE;
	case PIPE_BLENDFACTOR_SRC_COLOR:
		return V_028780_BLEND_SRC_COLOR;
	case PIPE_BLENDFACTOR_SRC_ALPHA:
		return V_028780_BLEND_SRC_ALPHA;
	case PIPE_BLENDFACTOR_DST_ALPHA:
		return V_028780_BLEND_DST_ALPHA;
	case PIPE_BLENDFACTOR_DST_COLOR:
		return V_028780_BLEND_DST_COLOR;
	case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
		return V_028780_BLEND_SRC_ALPHA_SATURATE;
	case PIPE_BLENDFACTOR_CONST_COLOR:
		return V_028780_BLEND_CONSTANT_COLOR;
	case PIPE_BLENDFACTOR_CONST_ALPHA:
		return V_028780_BLEND_CONSTANT_ALPHA;
	case PIPE_BLENDFACTOR_ZERO:
		return V_028780_BLEND_ZERO;
	case PIPE_BLENDFACTOR_INV_SRC_COLOR:
		return V_028780_BLEND_ONE_MINUS_SRC_COLOR;
	case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
		return V_028780_BLEND_ONE_MINUS_SRC_ALPHA;
	case PIPE_BLENDFACTOR_INV_DST_ALPHA:
		return V_028780_BLEND_ONE_MINUS_DST_ALPHA;
	case PIPE_BLENDFACTOR_INV_DST_COLOR:
		return V_028780_BLEND_ONE_MINUS_DST_COLOR;
	case PIPE_BLENDFACTOR_INV_CONST_COLOR:
		return V_028780_BLEND_ONE_MINUS_CONSTANT_COLOR;
	case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
		return V_028780_BLEND_ONE_MINUS_CONSTANT_ALPHA;
	case PIPE_BLENDFACTOR_SRC1_COLOR:
		return V_028780_BLEND_SRC1_COLOR;
	case PIPE_BLENDFACTOR_SRC1_ALPHA:
		return V_028780_BLEND_SRC1_ALPHA;
	case PIPE_BLENDFACTOR_INV_SRC1_COLOR:
		return V_028780_BLEND_INV_SRC1_COLOR;
	case PIPE_BLENDFACTOR_INV_SRC1_ALPHA:
		return V_028780_BLEND_INV_SRC1_ALPHA;
	default:
		R600_ERR("Bad blend factor %d not supported!\n", blend_fact);
		assert(0);
		break;
	}
	return 0;
}

static void *si_create_blend_state(struct pipe_context *ctx,
				   const struct pipe_blend_state *state)
{
	struct si_state_blend *blend = CALLOC_STRUCT(si_state_blend);
	struct si_pm4_state *pm4 = &blend->pm4;

	uint32_t color_control;

	if (blend == NULL)
		return NULL;

	color_control = S_028808_MODE(V_028808_CB_NORMAL);
	if (state->logicop_enable) {
		color_control |= S_028808_ROP3(state->logicop_func | (state->logicop_func << 4));
	} else {
		color_control |= S_028808_ROP3(0xcc);
	}
	si_pm4_set_reg(pm4, R_028808_CB_COLOR_CONTROL, color_control);

	si_pm4_set_reg(pm4, R_028C38_PA_SC_AA_MASK_X0Y0_X1Y0, ~0);
	si_pm4_set_reg(pm4, R_028C3C_PA_SC_AA_MASK_X0Y1_X1Y1, ~0);

	blend->cb_target_mask = 0;
	for (int i = 0; i < 8; i++) {
		/* state->rt entries > 0 only written if independent blending */
		const int j = state->independent_blend_enable ? i : 0;

		unsigned eqRGB = state->rt[j].rgb_func;
		unsigned srcRGB = state->rt[j].rgb_src_factor;
		unsigned dstRGB = state->rt[j].rgb_dst_factor;
		unsigned eqA = state->rt[j].alpha_func;
		unsigned srcA = state->rt[j].alpha_src_factor;
		unsigned dstA = state->rt[j].alpha_dst_factor;

		unsigned blend_cntl = 0;

		/* we pretend 8 buffer are used, CB_SHADER_MASK will disable unused one */
		blend->cb_target_mask |= state->rt[j].colormask << (4 * i);

		if (!state->rt[j].blend_enable) {
			si_pm4_set_reg(pm4, R_028780_CB_BLEND0_CONTROL + i * 4, blend_cntl);
			continue;
		}

		blend_cntl |= S_028780_ENABLE(1);
		blend_cntl |= S_028780_COLOR_COMB_FCN(si_translate_blend_function(eqRGB));
		blend_cntl |= S_028780_COLOR_SRCBLEND(si_translate_blend_factor(srcRGB));
		blend_cntl |= S_028780_COLOR_DESTBLEND(si_translate_blend_factor(dstRGB));

		if (srcA != srcRGB || dstA != dstRGB || eqA != eqRGB) {
			blend_cntl |= S_028780_SEPARATE_ALPHA_BLEND(1);
			blend_cntl |= S_028780_ALPHA_COMB_FCN(si_translate_blend_function(eqA));
			blend_cntl |= S_028780_ALPHA_SRCBLEND(si_translate_blend_factor(srcA));
			blend_cntl |= S_028780_ALPHA_DESTBLEND(si_translate_blend_factor(dstA));
		}
		si_pm4_set_reg(pm4, R_028780_CB_BLEND0_CONTROL + i * 4, blend_cntl);
	}

	return blend;
}

static void si_bind_blend_state(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	si_pm4_bind_state(rctx, blend, (struct si_state_blend *)state);
}

static void si_delete_blend_state(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	si_pm4_delete_state(rctx, blend, (struct si_state_blend *)state);
}

static void si_set_blend_color(struct pipe_context *ctx,
			       const struct pipe_blend_color *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct si_pm4_state *pm4 = CALLOC_STRUCT(si_pm4_state);

        if (pm4 == NULL)
                return;

	si_pm4_set_reg(pm4, R_028414_CB_BLEND_RED, fui(state->color[0]));
	si_pm4_set_reg(pm4, R_028418_CB_BLEND_GREEN, fui(state->color[1]));
	si_pm4_set_reg(pm4, R_02841C_CB_BLEND_BLUE, fui(state->color[2]));
	si_pm4_set_reg(pm4, R_028420_CB_BLEND_ALPHA, fui(state->color[3]));

	si_pm4_set_state(rctx, blend_color, pm4);
}

static void si_set_clip_state(struct pipe_context *ctx,
			      const struct pipe_clip_state *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct si_pm4_state *pm4 = CALLOC_STRUCT(si_pm4_state);

	if (pm4 == NULL)
		return;

	for (int i = 0; i < 6; i++) {
		si_pm4_set_reg(pm4, R_0285BC_PA_CL_UCP_0_X + i * 16,
			       fui(state->ucp[i][0]));
		si_pm4_set_reg(pm4, R_0285C0_PA_CL_UCP_0_Y + i * 16,
			       fui(state->ucp[i][1]));
		si_pm4_set_reg(pm4, R_0285C4_PA_CL_UCP_0_Z + i * 16,
			       fui(state->ucp[i][2]));
		si_pm4_set_reg(pm4, R_0285C8_PA_CL_UCP_0_W + i * 16,
			       fui(state->ucp[i][3]));
        }

	si_pm4_set_state(rctx, clip, pm4);
}

static void si_set_scissor_state(struct pipe_context *ctx,
				 const struct pipe_scissor_state *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct si_pm4_state *pm4 = CALLOC_STRUCT(si_pm4_state);
	uint32_t tl, br;

	if (pm4 == NULL)
		return;

	tl = S_028240_TL_X(state->minx) | S_028240_TL_Y(state->miny);
	br = S_028244_BR_X(state->maxx) | S_028244_BR_Y(state->maxy);
	si_pm4_set_reg(pm4, R_028210_PA_SC_CLIPRECT_0_TL, tl);
	si_pm4_set_reg(pm4, R_028214_PA_SC_CLIPRECT_0_BR, br);
	si_pm4_set_reg(pm4, R_028218_PA_SC_CLIPRECT_1_TL, tl);
	si_pm4_set_reg(pm4, R_02821C_PA_SC_CLIPRECT_1_BR, br);
	si_pm4_set_reg(pm4, R_028220_PA_SC_CLIPRECT_2_TL, tl);
	si_pm4_set_reg(pm4, R_028224_PA_SC_CLIPRECT_2_BR, br);
	si_pm4_set_reg(pm4, R_028228_PA_SC_CLIPRECT_3_TL, tl);
	si_pm4_set_reg(pm4, R_02822C_PA_SC_CLIPRECT_3_BR, br);

	si_pm4_set_state(rctx, scissor, pm4);
}

static void si_set_viewport_state(struct pipe_context *ctx,
				  const struct pipe_viewport_state *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct si_state_viewport *viewport = CALLOC_STRUCT(si_state_viewport);
	struct si_pm4_state *pm4 = &viewport->pm4;

	if (viewport == NULL)
		return;

	viewport->viewport = *state;
	si_pm4_set_reg(pm4, R_0282D0_PA_SC_VPORT_ZMIN_0, 0x00000000);
	si_pm4_set_reg(pm4, R_0282D4_PA_SC_VPORT_ZMAX_0, 0x3F800000);
	si_pm4_set_reg(pm4, R_028350_PA_SC_RASTER_CONFIG, 0x00000000);
	si_pm4_set_reg(pm4, R_02843C_PA_CL_VPORT_XSCALE_0, fui(state->scale[0]));
	si_pm4_set_reg(pm4, R_028440_PA_CL_VPORT_XOFFSET_0, fui(state->translate[0]));
	si_pm4_set_reg(pm4, R_028444_PA_CL_VPORT_YSCALE_0, fui(state->scale[1]));
	si_pm4_set_reg(pm4, R_028448_PA_CL_VPORT_YOFFSET_0, fui(state->translate[1]));
	si_pm4_set_reg(pm4, R_02844C_PA_CL_VPORT_ZSCALE_0, fui(state->scale[2]));
	si_pm4_set_reg(pm4, R_028450_PA_CL_VPORT_ZOFFSET_0, fui(state->translate[2]));
	si_pm4_set_reg(pm4, R_028818_PA_CL_VTE_CNTL, 0x0000043F);

	si_pm4_set_state(rctx, viewport, viewport);
}

void si_init_state_functions(struct r600_context *rctx)
{
	rctx->context.create_blend_state = si_create_blend_state;
	rctx->context.bind_blend_state = si_bind_blend_state;
	rctx->context.delete_blend_state = si_delete_blend_state;
	rctx->context.set_blend_color = si_set_blend_color;

	rctx->context.set_clip_state = si_set_clip_state;
	rctx->context.set_scissor_state = si_set_scissor_state;
	rctx->context.set_viewport_state = si_set_viewport_state;
}

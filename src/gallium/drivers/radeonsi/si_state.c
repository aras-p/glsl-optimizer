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
#include "util/u_framebuffer.h"
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

/*
 * Clipping, scissors and viewport
 */

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

/*
 * inferred state between framebuffer and rasterizer
 */
static void si_update_fb_rs_state(struct r600_context *rctx)
{
	struct si_state_rasterizer *rs = rctx->queued.named.rasterizer;
	struct si_pm4_state *pm4 = CALLOC_STRUCT(si_pm4_state);
	unsigned offset_db_fmt_cntl = 0, depth;
	float offset_units;

	if (!rs || !rctx->framebuffer.zsbuf) {
		FREE(pm4);
		return;
	}

	offset_units = rctx->queued.named.rasterizer->offset_units;
	switch (rctx->framebuffer.zsbuf->texture->format) {
	case PIPE_FORMAT_Z24X8_UNORM:
	case PIPE_FORMAT_Z24_UNORM_S8_UINT:
		depth = -24;
		offset_units *= 2.0f;
		break;
	case PIPE_FORMAT_Z32_FLOAT:
	case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
		depth = -23;
		offset_units *= 1.0f;
		offset_db_fmt_cntl |= S_028B78_POLY_OFFSET_DB_IS_FLOAT_FMT(1);
		break;
	case PIPE_FORMAT_Z16_UNORM:
		depth = -16;
		offset_units *= 4.0f;
		break;
	default:
		return;
	}

	/* FIXME some of those reg can be computed with cso */
	offset_db_fmt_cntl |= S_028B78_POLY_OFFSET_NEG_NUM_DB_BITS(depth);
	si_pm4_set_reg(pm4, R_028B80_PA_SU_POLY_OFFSET_FRONT_SCALE,
		       fui(rctx->queued.named.rasterizer->offset_scale));
	si_pm4_set_reg(pm4, R_028B84_PA_SU_POLY_OFFSET_FRONT_OFFSET, fui(offset_units));
	si_pm4_set_reg(pm4, R_028B88_PA_SU_POLY_OFFSET_BACK_SCALE,
		       fui(rctx->queued.named.rasterizer->offset_scale));
	si_pm4_set_reg(pm4, R_028B8C_PA_SU_POLY_OFFSET_BACK_OFFSET, fui(offset_units));
	si_pm4_set_reg(pm4, R_028B78_PA_SU_POLY_OFFSET_DB_FMT_CNTL, offset_db_fmt_cntl);

	si_pm4_set_state(rctx, fb_rs, pm4);
}

/*
 * Rasterizer
 */

static uint32_t si_translate_fill(uint32_t func)
{
	switch(func) {
	case PIPE_POLYGON_MODE_FILL:
		return V_028814_X_DRAW_TRIANGLES;
	case PIPE_POLYGON_MODE_LINE:
		return V_028814_X_DRAW_LINES;
	case PIPE_POLYGON_MODE_POINT:
		return V_028814_X_DRAW_POINTS;
	default:
		assert(0);
		return V_028814_X_DRAW_POINTS;
	}
}

static void *si_create_rs_state(struct pipe_context *ctx,
				const struct pipe_rasterizer_state *state)
{
	struct si_state_rasterizer *rs = CALLOC_STRUCT(si_state_rasterizer);
	struct si_pm4_state *pm4 = &rs->pm4;
	unsigned tmp;
	unsigned prov_vtx = 1, polygon_dual_mode;
	unsigned clip_rule;
	float psize_min, psize_max;

	if (rs == NULL) {
		return NULL;
	}

	polygon_dual_mode = (state->fill_front != PIPE_POLYGON_MODE_FILL ||
				state->fill_back != PIPE_POLYGON_MODE_FILL);

	if (state->flatshade_first)
		prov_vtx = 0;

	rs->flatshade = state->flatshade;
	rs->sprite_coord_enable = state->sprite_coord_enable;
	rs->pa_sc_line_stipple = state->line_stipple_enable ?
				S_028A0C_LINE_PATTERN(state->line_stipple_pattern) |
				S_028A0C_REPEAT_COUNT(state->line_stipple_factor) : 0;
	rs->pa_su_sc_mode_cntl =
		S_028814_PROVOKING_VTX_LAST(prov_vtx) |
		S_028814_CULL_FRONT(state->rasterizer_discard || (state->cull_face & PIPE_FACE_FRONT) ? 1 : 0) |
		S_028814_CULL_BACK(state->rasterizer_discard || (state->cull_face & PIPE_FACE_BACK) ? 1 : 0) |
		S_028814_FACE(!state->front_ccw) |
		S_028814_POLY_OFFSET_FRONT_ENABLE(state->offset_tri) |
		S_028814_POLY_OFFSET_BACK_ENABLE(state->offset_tri) |
		S_028814_POLY_OFFSET_PARA_ENABLE(state->offset_tri) |
		S_028814_POLY_MODE(polygon_dual_mode) |
		S_028814_POLYMODE_FRONT_PTYPE(si_translate_fill(state->fill_front)) |
		S_028814_POLYMODE_BACK_PTYPE(si_translate_fill(state->fill_back));
	rs->pa_cl_clip_cntl =
		S_028810_PS_UCP_MODE(3) |
		S_028810_ZCLIP_NEAR_DISABLE(!state->depth_clip) |
		S_028810_ZCLIP_FAR_DISABLE(!state->depth_clip) |
		S_028810_DX_LINEAR_ATTR_CLIP_ENA(1);
	rs->pa_cl_vs_out_cntl =
		S_02881C_USE_VTX_POINT_SIZE(state->point_size_per_vertex) |
		S_02881C_VS_OUT_MISC_VEC_ENA(state->point_size_per_vertex);

	clip_rule = state->scissor ? 0xAAAA : 0xFFFF;

	/* offset */
	rs->offset_units = state->offset_units;
	rs->offset_scale = state->offset_scale * 12.0f;

	/* XXX: Flat shading hangs the GPU */
	tmp = S_0286D4_FLAT_SHADE_ENA(0);
	if (state->sprite_coord_enable) {
		tmp |= S_0286D4_PNT_SPRITE_ENA(1) |
			S_0286D4_PNT_SPRITE_OVRD_X(V_0286D4_SPI_PNT_SPRITE_SEL_S) |
			S_0286D4_PNT_SPRITE_OVRD_Y(V_0286D4_SPI_PNT_SPRITE_SEL_T) |
			S_0286D4_PNT_SPRITE_OVRD_Z(V_0286D4_SPI_PNT_SPRITE_SEL_0) |
			S_0286D4_PNT_SPRITE_OVRD_W(V_0286D4_SPI_PNT_SPRITE_SEL_1);
		if (state->sprite_coord_mode != PIPE_SPRITE_COORD_UPPER_LEFT) {
			tmp |= S_0286D4_PNT_SPRITE_TOP_1(1);
		}
	}
	si_pm4_set_reg(pm4, R_0286D4_SPI_INTERP_CONTROL_0, tmp);

	si_pm4_set_reg(pm4, R_028820_PA_CL_NANINF_CNTL, 0x00000000);
	/* point size 12.4 fixed point */
	tmp = (unsigned)(state->point_size * 8.0);
	si_pm4_set_reg(pm4, R_028A00_PA_SU_POINT_SIZE, S_028A00_HEIGHT(tmp) | S_028A00_WIDTH(tmp));

	if (state->point_size_per_vertex) {
		psize_min = util_get_min_point_size(state);
		psize_max = 8192;
	} else {
		/* Force the point size to be as if the vertex output was disabled. */
		psize_min = state->point_size;
		psize_max = state->point_size;
	}
	/* Divide by two, because 0.5 = 1 pixel. */
	si_pm4_set_reg(pm4, R_028A04_PA_SU_POINT_MINMAX,
			S_028A04_MIN_SIZE(r600_pack_float_12p4(psize_min/2)) |
			S_028A04_MAX_SIZE(r600_pack_float_12p4(psize_max/2)));

	tmp = (unsigned)state->line_width * 8;
	si_pm4_set_reg(pm4, R_028A08_PA_SU_LINE_CNTL, S_028A08_WIDTH(tmp));
	si_pm4_set_reg(pm4, R_028A48_PA_SC_MODE_CNTL_0,
			S_028A48_LINE_STIPPLE_ENABLE(state->line_stipple_enable));

	si_pm4_set_reg(pm4, R_028BDC_PA_SC_LINE_CNTL, 0x00000400);
	si_pm4_set_reg(pm4, R_028BE4_PA_SU_VTX_CNTL,
			S_028BE4_PIX_CENTER(state->gl_rasterization_rules));
	si_pm4_set_reg(pm4, R_028BE8_PA_CL_GB_VERT_CLIP_ADJ, 0x3F800000);
	si_pm4_set_reg(pm4, R_028BEC_PA_CL_GB_VERT_DISC_ADJ, 0x3F800000);
	si_pm4_set_reg(pm4, R_028BF0_PA_CL_GB_HORZ_CLIP_ADJ, 0x3F800000);
	si_pm4_set_reg(pm4, R_028BF4_PA_CL_GB_HORZ_DISC_ADJ, 0x3F800000);

	si_pm4_set_reg(pm4, R_028B7C_PA_SU_POLY_OFFSET_CLAMP, fui(state->offset_clamp));
	si_pm4_set_reg(pm4, R_02820C_PA_SC_CLIPRECT_RULE, clip_rule);

	return rs;
}

static void si_bind_rs_state(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct si_state_rasterizer *rs = (struct si_state_rasterizer *)state;

	if (state == NULL)
		return;

	// TODO
	rctx->sprite_coord_enable = rs->sprite_coord_enable;
	rctx->pa_sc_line_stipple = rs->pa_sc_line_stipple;
	rctx->pa_su_sc_mode_cntl = rs->pa_su_sc_mode_cntl;
	rctx->pa_cl_clip_cntl = rs->pa_cl_clip_cntl;
	rctx->pa_cl_vs_out_cntl = rs->pa_cl_vs_out_cntl;

	si_pm4_bind_state(rctx, rasterizer, rs);
	si_update_fb_rs_state(rctx);
}

static void si_delete_rs_state(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	si_pm4_delete_state(rctx, rasterizer, (struct si_state_rasterizer *)state);
}

/*
 * DSA
 */

/* transnates straight */
static uint32_t si_translate_ds_func(int func)
{
        return func;
}

static void *si_create_dsa_state(struct pipe_context *ctx,
				 const struct pipe_depth_stencil_alpha_state *state)
{
	struct si_state_dsa *dsa = CALLOC_STRUCT(si_state_dsa);
	struct si_pm4_state *pm4 = &dsa->pm4;
	unsigned db_depth_control, /* alpha_test_control, */ alpha_ref;
	unsigned db_render_override, db_render_control;

	if (dsa == NULL) {
		return NULL;
	}

	dsa->valuemask[0] = state->stencil[0].valuemask;
	dsa->valuemask[1] = state->stencil[1].valuemask;
	dsa->writemask[0] = state->stencil[0].writemask;
	dsa->writemask[1] = state->stencil[1].writemask;

	db_depth_control = S_028800_Z_ENABLE(state->depth.enabled) |
		S_028800_Z_WRITE_ENABLE(state->depth.writemask) |
		S_028800_ZFUNC(state->depth.func);

	/* stencil */
	if (state->stencil[0].enabled) {
		db_depth_control |= S_028800_STENCIL_ENABLE(1);
		db_depth_control |= S_028800_STENCILFUNC(si_translate_ds_func(state->stencil[0].func));
		//db_depth_control |= S_028800_STENCILFAIL(r600_translate_stencil_op(state->stencil[0].fail_op));
		//db_depth_control |= S_028800_STENCILZPASS(r600_translate_stencil_op(state->stencil[0].zpass_op));
		//db_depth_control |= S_028800_STENCILZFAIL(r600_translate_stencil_op(state->stencil[0].zfail_op));

		if (state->stencil[1].enabled) {
			db_depth_control |= S_028800_BACKFACE_ENABLE(1);
			db_depth_control |= S_028800_STENCILFUNC_BF(si_translate_ds_func(state->stencil[1].func));
			//db_depth_control |= S_028800_STENCILFAIL_BF(r600_translate_stencil_op(state->stencil[1].fail_op));
			//db_depth_control |= S_028800_STENCILZPASS_BF(r600_translate_stencil_op(state->stencil[1].zpass_op));
			//db_depth_control |= S_028800_STENCILZFAIL_BF(r600_translate_stencil_op(state->stencil[1].zfail_op));
		}
	}

	/* alpha */
	//alpha_test_control = 0;
	alpha_ref = 0;
	if (state->alpha.enabled) {
		//alpha_test_control = S_028410_ALPHA_FUNC(state->alpha.func);
		//alpha_test_control |= S_028410_ALPHA_TEST_ENABLE(1);
		alpha_ref = fui(state->alpha.ref_value);
	}
	dsa->alpha_ref = alpha_ref;

	/* misc */
	db_render_control = 0;
	db_render_override = S_02800C_FORCE_HIZ_ENABLE(V_02800C_FORCE_DISABLE) |
		S_02800C_FORCE_HIS_ENABLE0(V_02800C_FORCE_DISABLE) |
		S_02800C_FORCE_HIS_ENABLE1(V_02800C_FORCE_DISABLE);
	/* TODO db_render_override depends on query */
	si_pm4_set_reg(pm4, R_028020_DB_DEPTH_BOUNDS_MIN, 0x00000000);
	si_pm4_set_reg(pm4, R_028024_DB_DEPTH_BOUNDS_MAX, 0x00000000);
	si_pm4_set_reg(pm4, R_028028_DB_STENCIL_CLEAR, 0x00000000);
	si_pm4_set_reg(pm4, R_02802C_DB_DEPTH_CLEAR, 0x3F800000);
	//si_pm4_set_reg(pm4, R_028410_SX_ALPHA_TEST_CONTROL, alpha_test_control);
	si_pm4_set_reg(pm4, R_028800_DB_DEPTH_CONTROL, db_depth_control);
	si_pm4_set_reg(pm4, R_028000_DB_RENDER_CONTROL, db_render_control);
	si_pm4_set_reg(pm4, R_02800C_DB_RENDER_OVERRIDE, db_render_override);
	si_pm4_set_reg(pm4, R_028AC0_DB_SRESULTS_COMPARE_STATE0, 0x0);
	si_pm4_set_reg(pm4, R_028AC4_DB_SRESULTS_COMPARE_STATE1, 0x0);
	si_pm4_set_reg(pm4, R_028AC8_DB_PRELOAD_CONTROL, 0x0);
	si_pm4_set_reg(pm4, R_028B70_DB_ALPHA_TO_MASK, 0x0000AA00);
	dsa->db_render_override = db_render_override;

	return dsa;
}

static void si_bind_dsa_state(struct pipe_context *ctx, void *state)
{
        struct r600_context *rctx = (struct r600_context *)ctx;
        struct si_state_dsa *dsa = state;
        struct r600_stencil_ref ref;

        if (state == NULL)
                return;

	si_pm4_bind_state(rctx, dsa, dsa);

	// TODO
        rctx->alpha_ref = dsa->alpha_ref;
        rctx->alpha_ref_dirty = true;

        ref.ref_value[0] = rctx->stencil_ref.ref_value[0];
        ref.ref_value[1] = rctx->stencil_ref.ref_value[1];
        ref.valuemask[0] = dsa->valuemask[0];
        ref.valuemask[1] = dsa->valuemask[1];
        ref.writemask[0] = dsa->writemask[0];
        ref.writemask[1] = dsa->writemask[1];

        r600_set_stencil_ref(ctx, &ref);
}

static void si_delete_dsa_state(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	si_pm4_delete_state(rctx, dsa, (struct si_state_dsa *)state);
}

static void *si_create_db_flush_dsa(struct r600_context *rctx)
{
	struct pipe_depth_stencil_alpha_state dsa;
        struct si_state_dsa *state;

	memset(&dsa, 0, sizeof(dsa));

	state = rctx->context.create_depth_stencil_alpha_state(&rctx->context, &dsa);
	si_pm4_set_reg(&state->pm4, R_028000_DB_RENDER_CONTROL,
		       S_028000_DEPTH_COPY(1) |
		       S_028000_STENCIL_COPY(1) |
		       S_028000_COPY_CENTROID(1));
        return state;
}

/*
 * format translation
 */
static uint32_t si_translate_colorformat(enum pipe_format format)
{
	switch (format) {
	/* 8-bit buffers. */
	case PIPE_FORMAT_A8_UNORM:
	case PIPE_FORMAT_A8_UINT:
	case PIPE_FORMAT_A8_SINT:
	case PIPE_FORMAT_I8_UNORM:
	case PIPE_FORMAT_I8_UINT:
	case PIPE_FORMAT_I8_SINT:
	case PIPE_FORMAT_L8_UNORM:
	case PIPE_FORMAT_L8_UINT:
	case PIPE_FORMAT_L8_SINT:
	case PIPE_FORMAT_L8_SRGB:
	case PIPE_FORMAT_R8_UNORM:
	case PIPE_FORMAT_R8_SNORM:
	case PIPE_FORMAT_R8_UINT:
	case PIPE_FORMAT_R8_SINT:
		return V_028C70_COLOR_8;

	/* 16-bit buffers. */
	case PIPE_FORMAT_B5G6R5_UNORM:
		return V_028C70_COLOR_5_6_5;

	case PIPE_FORMAT_B5G5R5A1_UNORM:
	case PIPE_FORMAT_B5G5R5X1_UNORM:
		return V_028C70_COLOR_1_5_5_5;

	case PIPE_FORMAT_B4G4R4A4_UNORM:
	case PIPE_FORMAT_B4G4R4X4_UNORM:
		return V_028C70_COLOR_4_4_4_4;

	case PIPE_FORMAT_L8A8_UNORM:
	case PIPE_FORMAT_L8A8_UINT:
	case PIPE_FORMAT_L8A8_SINT:
	case PIPE_FORMAT_L8A8_SRGB:
	case PIPE_FORMAT_R8G8_UNORM:
	case PIPE_FORMAT_R8G8_UINT:
	case PIPE_FORMAT_R8G8_SINT:
		return V_028C70_COLOR_8_8;

	case PIPE_FORMAT_Z16_UNORM:
	case PIPE_FORMAT_R16_UNORM:
	case PIPE_FORMAT_R16_UINT:
	case PIPE_FORMAT_R16_SINT:
	case PIPE_FORMAT_R16_FLOAT:
	case PIPE_FORMAT_R16G16_FLOAT:
		return V_028C70_COLOR_16;

	/* 32-bit buffers. */
	case PIPE_FORMAT_A8B8G8R8_SRGB:
	case PIPE_FORMAT_A8B8G8R8_UNORM:
	case PIPE_FORMAT_A8R8G8B8_UNORM:
	case PIPE_FORMAT_B8G8R8A8_SRGB:
	case PIPE_FORMAT_B8G8R8A8_UNORM:
	case PIPE_FORMAT_B8G8R8X8_UNORM:
	case PIPE_FORMAT_R8G8B8A8_SNORM:
	case PIPE_FORMAT_R8G8B8A8_UNORM:
	case PIPE_FORMAT_R8G8B8X8_UNORM:
	case PIPE_FORMAT_R8SG8SB8UX8U_NORM:
	case PIPE_FORMAT_X8B8G8R8_UNORM:
	case PIPE_FORMAT_X8R8G8B8_UNORM:
	case PIPE_FORMAT_R8G8B8_UNORM:
	case PIPE_FORMAT_R8G8B8A8_SSCALED:
	case PIPE_FORMAT_R8G8B8A8_USCALED:
	case PIPE_FORMAT_R8G8B8A8_SINT:
	case PIPE_FORMAT_R8G8B8A8_UINT:
		return V_028C70_COLOR_8_8_8_8;

	case PIPE_FORMAT_R10G10B10A2_UNORM:
	case PIPE_FORMAT_R10G10B10X2_SNORM:
	case PIPE_FORMAT_B10G10R10A2_UNORM:
	case PIPE_FORMAT_B10G10R10A2_UINT:
	case PIPE_FORMAT_R10SG10SB10SA2U_NORM:
		return V_028C70_COLOR_2_10_10_10;

	case PIPE_FORMAT_Z24X8_UNORM:
	case PIPE_FORMAT_Z24_UNORM_S8_UINT:
		return V_028C70_COLOR_8_24;

	case PIPE_FORMAT_X8Z24_UNORM:
	case PIPE_FORMAT_S8_UINT_Z24_UNORM:
		return V_028C70_COLOR_24_8;

	case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
		return V_028C70_COLOR_X24_8_32_FLOAT;

	case PIPE_FORMAT_R32_FLOAT:
	case PIPE_FORMAT_Z32_FLOAT:
		return V_028C70_COLOR_32;

	case PIPE_FORMAT_R16G16_SSCALED:
	case PIPE_FORMAT_R16G16_UNORM:
	case PIPE_FORMAT_R16G16_UINT:
	case PIPE_FORMAT_R16G16_SINT:
		return V_028C70_COLOR_16_16;

	case PIPE_FORMAT_R11G11B10_FLOAT:
		return V_028C70_COLOR_10_11_11;

	/* 64-bit buffers. */
	case PIPE_FORMAT_R16G16B16_USCALED:
	case PIPE_FORMAT_R16G16B16_SSCALED:
	case PIPE_FORMAT_R16G16B16A16_UINT:
	case PIPE_FORMAT_R16G16B16A16_SINT:
	case PIPE_FORMAT_R16G16B16A16_USCALED:
	case PIPE_FORMAT_R16G16B16A16_SSCALED:
	case PIPE_FORMAT_R16G16B16A16_UNORM:
	case PIPE_FORMAT_R16G16B16A16_SNORM:
	case PIPE_FORMAT_R16G16B16_FLOAT:
	case PIPE_FORMAT_R16G16B16A16_FLOAT:
		return V_028C70_COLOR_16_16_16_16;

	case PIPE_FORMAT_R32G32_FLOAT:
	case PIPE_FORMAT_R32G32_USCALED:
	case PIPE_FORMAT_R32G32_SSCALED:
	case PIPE_FORMAT_R32G32_SINT:
	case PIPE_FORMAT_R32G32_UINT:
		return V_028C70_COLOR_32_32;

	/* 128-bit buffers. */
	case PIPE_FORMAT_R32G32B32A32_SNORM:
	case PIPE_FORMAT_R32G32B32A32_UNORM:
	case PIPE_FORMAT_R32G32B32A32_SSCALED:
	case PIPE_FORMAT_R32G32B32A32_USCALED:
	case PIPE_FORMAT_R32G32B32A32_SINT:
	case PIPE_FORMAT_R32G32B32A32_UINT:
	case PIPE_FORMAT_R32G32B32A32_FLOAT:
		return V_028C70_COLOR_32_32_32_32;

	/* YUV buffers. */
	case PIPE_FORMAT_UYVY:
	case PIPE_FORMAT_YUYV:
	/* 96-bit buffers. */
	case PIPE_FORMAT_R32G32B32_FLOAT:
	/* 8-bit buffers. */
	case PIPE_FORMAT_L4A4_UNORM:
	case PIPE_FORMAT_R4A4_UNORM:
	case PIPE_FORMAT_A4R4_UNORM:
	default:
		return ~0U; /* Unsupported. */
	}
}

static uint32_t si_translate_colorswap(enum pipe_format format)
{
	switch (format) {
	/* 8-bit buffers. */
	case PIPE_FORMAT_L4A4_UNORM:
	case PIPE_FORMAT_A4R4_UNORM:
		return V_028C70_SWAP_ALT;

	case PIPE_FORMAT_A8_UNORM:
	case PIPE_FORMAT_A8_UINT:
	case PIPE_FORMAT_A8_SINT:
	case PIPE_FORMAT_R4A4_UNORM:
		return V_028C70_SWAP_ALT_REV;
	case PIPE_FORMAT_I8_UNORM:
	case PIPE_FORMAT_L8_UNORM:
	case PIPE_FORMAT_I8_UINT:
	case PIPE_FORMAT_I8_SINT:
	case PIPE_FORMAT_L8_UINT:
	case PIPE_FORMAT_L8_SINT:
	case PIPE_FORMAT_L8_SRGB:
	case PIPE_FORMAT_R8_UNORM:
	case PIPE_FORMAT_R8_SNORM:
	case PIPE_FORMAT_R8_UINT:
	case PIPE_FORMAT_R8_SINT:
		return V_028C70_SWAP_STD;

	/* 16-bit buffers. */
	case PIPE_FORMAT_B5G6R5_UNORM:
		return V_028C70_SWAP_STD_REV;

	case PIPE_FORMAT_B5G5R5A1_UNORM:
	case PIPE_FORMAT_B5G5R5X1_UNORM:
		return V_028C70_SWAP_ALT;

	case PIPE_FORMAT_B4G4R4A4_UNORM:
	case PIPE_FORMAT_B4G4R4X4_UNORM:
		return V_028C70_SWAP_ALT;

	case PIPE_FORMAT_Z16_UNORM:
		return V_028C70_SWAP_STD;

	case PIPE_FORMAT_L8A8_UNORM:
	case PIPE_FORMAT_L8A8_UINT:
	case PIPE_FORMAT_L8A8_SINT:
	case PIPE_FORMAT_L8A8_SRGB:
		return V_028C70_SWAP_ALT;
	case PIPE_FORMAT_R8G8_UNORM:
	case PIPE_FORMAT_R8G8_UINT:
	case PIPE_FORMAT_R8G8_SINT:
		return V_028C70_SWAP_STD;

	case PIPE_FORMAT_R16_UNORM:
	case PIPE_FORMAT_R16_UINT:
	case PIPE_FORMAT_R16_SINT:
	case PIPE_FORMAT_R16_FLOAT:
		return V_028C70_SWAP_STD;

	/* 32-bit buffers. */
	case PIPE_FORMAT_A8B8G8R8_SRGB:
		return V_028C70_SWAP_STD_REV;
	case PIPE_FORMAT_B8G8R8A8_SRGB:
		return V_028C70_SWAP_ALT;

	case PIPE_FORMAT_B8G8R8A8_UNORM:
	case PIPE_FORMAT_B8G8R8X8_UNORM:
		return V_028C70_SWAP_ALT;

	case PIPE_FORMAT_A8R8G8B8_UNORM:
	case PIPE_FORMAT_X8R8G8B8_UNORM:
		return V_028C70_SWAP_ALT_REV;
	case PIPE_FORMAT_R8G8B8A8_SNORM:
	case PIPE_FORMAT_R8G8B8A8_UNORM:
	case PIPE_FORMAT_R8G8B8A8_SSCALED:
	case PIPE_FORMAT_R8G8B8A8_USCALED:
	case PIPE_FORMAT_R8G8B8A8_SINT:
	case PIPE_FORMAT_R8G8B8A8_UINT:
	case PIPE_FORMAT_R8G8B8X8_UNORM:
		return V_028C70_SWAP_STD;

	case PIPE_FORMAT_A8B8G8R8_UNORM:
	case PIPE_FORMAT_X8B8G8R8_UNORM:
	/* case PIPE_FORMAT_R8SG8SB8UX8U_NORM: */
		return V_028C70_SWAP_STD_REV;

	case PIPE_FORMAT_Z24X8_UNORM:
	case PIPE_FORMAT_Z24_UNORM_S8_UINT:
		return V_028C70_SWAP_STD;

	case PIPE_FORMAT_X8Z24_UNORM:
	case PIPE_FORMAT_S8_UINT_Z24_UNORM:
		return V_028C70_SWAP_STD;

	case PIPE_FORMAT_R10G10B10A2_UNORM:
	case PIPE_FORMAT_R10G10B10X2_SNORM:
	case PIPE_FORMAT_R10SG10SB10SA2U_NORM:
		return V_028C70_SWAP_STD;

	case PIPE_FORMAT_B10G10R10A2_UNORM:
	case PIPE_FORMAT_B10G10R10A2_UINT:
		return V_028C70_SWAP_ALT;

	case PIPE_FORMAT_R11G11B10_FLOAT:
	case PIPE_FORMAT_R32_FLOAT:
	case PIPE_FORMAT_R32_UINT:
	case PIPE_FORMAT_R32_SINT:
	case PIPE_FORMAT_Z32_FLOAT:
	case PIPE_FORMAT_R16G16_FLOAT:
	case PIPE_FORMAT_R16G16_UNORM:
	case PIPE_FORMAT_R16G16_UINT:
	case PIPE_FORMAT_R16G16_SINT:
		return V_028C70_SWAP_STD;

	/* 64-bit buffers. */
	case PIPE_FORMAT_R32G32_FLOAT:
	case PIPE_FORMAT_R32G32_UINT:
	case PIPE_FORMAT_R32G32_SINT:
	case PIPE_FORMAT_R16G16B16A16_UNORM:
	case PIPE_FORMAT_R16G16B16A16_SNORM:
	case PIPE_FORMAT_R16G16B16A16_USCALED:
	case PIPE_FORMAT_R16G16B16A16_SSCALED:
	case PIPE_FORMAT_R16G16B16A16_UINT:
	case PIPE_FORMAT_R16G16B16A16_SINT:
	case PIPE_FORMAT_R16G16B16A16_FLOAT:
	case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:

	/* 128-bit buffers. */
	case PIPE_FORMAT_R32G32B32A32_FLOAT:
	case PIPE_FORMAT_R32G32B32A32_SNORM:
	case PIPE_FORMAT_R32G32B32A32_UNORM:
	case PIPE_FORMAT_R32G32B32A32_SSCALED:
	case PIPE_FORMAT_R32G32B32A32_USCALED:
	case PIPE_FORMAT_R32G32B32A32_SINT:
	case PIPE_FORMAT_R32G32B32A32_UINT:
		return V_028C70_SWAP_STD;
	default:
		R600_ERR("unsupported colorswap format %d\n", format);
		return ~0U;
	}
	return ~0U;
}

static uint32_t si_colorformat_endian_swap(uint32_t colorformat)
{
	if (R600_BIG_ENDIAN) {
		switch(colorformat) {
		/* 8-bit buffers. */
		case V_028C70_COLOR_8:
			return V_028C70_ENDIAN_NONE;

		/* 16-bit buffers. */
		case V_028C70_COLOR_5_6_5:
		case V_028C70_COLOR_1_5_5_5:
		case V_028C70_COLOR_4_4_4_4:
		case V_028C70_COLOR_16:
		case V_028C70_COLOR_8_8:
			return V_028C70_ENDIAN_8IN16;

		/* 32-bit buffers. */
		case V_028C70_COLOR_8_8_8_8:
		case V_028C70_COLOR_2_10_10_10:
		case V_028C70_COLOR_8_24:
		case V_028C70_COLOR_24_8:
		case V_028C70_COLOR_16_16:
			return V_028C70_ENDIAN_8IN32;

		/* 64-bit buffers. */
		case V_028C70_COLOR_16_16_16_16:
			return V_028C70_ENDIAN_8IN16;

		case V_028C70_COLOR_32_32:
			return V_028C70_ENDIAN_8IN32;

		/* 128-bit buffers. */
		case V_028C70_COLOR_32_32_32_32:
			return V_028C70_ENDIAN_8IN32;
		default:
			return V_028C70_ENDIAN_NONE; /* Unsupported. */
		}
	} else {
		return V_028C70_ENDIAN_NONE;
	}
}

static uint32_t si_translate_dbformat(enum pipe_format format)
{
	switch (format) {
	case PIPE_FORMAT_Z16_UNORM:
		return V_028040_Z_16;
	case PIPE_FORMAT_Z24X8_UNORM:
	case PIPE_FORMAT_Z24_UNORM_S8_UINT:
		return V_028040_Z_24; /* XXX no longer supported on SI */
	case PIPE_FORMAT_Z32_FLOAT:
	case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
		return V_028040_Z_32_FLOAT;
	default:
		return ~0U;
	}
}

/*
 * framebuffer handling
 */

static void si_cb(struct r600_context *rctx, struct si_pm4_state *pm4,
		  const struct pipe_framebuffer_state *state, int cb)
{
	struct r600_resource_texture *rtex;
	struct r600_surface *surf;
	unsigned level = state->cbufs[cb]->u.tex.level;
	unsigned pitch, slice;
	unsigned color_info, color_attrib;
	unsigned format, swap, ntype, endian;
	uint64_t offset;
	unsigned blocksize;
	const struct util_format_description *desc;
	int i;
	unsigned blend_clamp = 0, blend_bypass = 0;

	surf = (struct r600_surface *)state->cbufs[cb];
	rtex = (struct r600_resource_texture*)state->cbufs[cb]->texture;
	blocksize = util_format_get_blocksize(rtex->real_format);

	if (rtex->depth)
		rctx->have_depth_fb = TRUE;

	if (rtex->depth && !rtex->is_flushing_texture) {
	        r600_texture_depth_flush(&rctx->context, state->cbufs[cb]->texture, TRUE);
		rtex = rtex->flushed_depth_texture;
	}

	offset = rtex->surface.level[level].offset;
	if (rtex->surface.level[level].mode < RADEON_SURF_MODE_1D) {
		offset += rtex->surface.level[level].slice_size *
			  state->cbufs[cb]->u.tex.first_layer;
	}
	pitch = (rtex->surface.level[level].nblk_x) / 8 - 1;
	slice = (rtex->surface.level[level].nblk_x * rtex->surface.level[level].nblk_y) / 64;
	if (slice) {
		slice = slice - 1;
	}

	color_attrib = S_028C74_TILE_MODE_INDEX(8);
	switch (rtex->surface.level[level].mode) {
	case RADEON_SURF_MODE_LINEAR_ALIGNED:
		color_attrib = S_028C74_TILE_MODE_INDEX(8);
		break;
	case RADEON_SURF_MODE_1D:
		color_attrib = S_028C74_TILE_MODE_INDEX(9);
		break;
	case RADEON_SURF_MODE_2D:
		if (rtex->resource.b.b.bind & PIPE_BIND_SCANOUT) {
			switch (blocksize) {
			case 1:
				color_attrib = S_028C74_TILE_MODE_INDEX(10);
				break;
			case 2:
				color_attrib = S_028C74_TILE_MODE_INDEX(11);
				break;
			case 4:
				color_attrib = S_028C74_TILE_MODE_INDEX(12);
				break;
			}
			break;
		} else switch (blocksize) {
		case 1:
			color_attrib = S_028C74_TILE_MODE_INDEX(14);
			break;
		case 2:
			color_attrib = S_028C74_TILE_MODE_INDEX(15);
			break;
		case 4:
			color_attrib = S_028C74_TILE_MODE_INDEX(16);
			break;
		case 8:
			color_attrib = S_028C74_TILE_MODE_INDEX(17);
			break;
		default:
			color_attrib = S_028C74_TILE_MODE_INDEX(13);
		}
		break;
	}

	desc = util_format_description(surf->base.format);
	for (i = 0; i < 4; i++) {
		if (desc->channel[i].type != UTIL_FORMAT_TYPE_VOID) {
			break;
		}
	}
	if (desc->channel[i].type == UTIL_FORMAT_TYPE_FLOAT) {
		ntype = V_028C70_NUMBER_FLOAT;
	} else {
		ntype = V_028C70_NUMBER_UNORM;
		if (desc->colorspace == UTIL_FORMAT_COLORSPACE_SRGB)
			ntype = V_028C70_NUMBER_SRGB;
		else if (desc->channel[i].type == UTIL_FORMAT_TYPE_SIGNED) {
			if (desc->channel[i].normalized)
				ntype = V_028C70_NUMBER_SNORM;
			else if (desc->channel[i].pure_integer)
				ntype = V_028C70_NUMBER_SINT;
		} else if (desc->channel[i].type == UTIL_FORMAT_TYPE_UNSIGNED) {
			if (desc->channel[i].normalized)
				ntype = V_028C70_NUMBER_UNORM;
			else if (desc->channel[i].pure_integer)
				ntype = V_028C70_NUMBER_UINT;
		}
	}

	format = si_translate_colorformat(surf->base.format);
	swap = si_translate_colorswap(surf->base.format);
	if (rtex->resource.b.b.usage == PIPE_USAGE_STAGING) {
		endian = V_028C70_ENDIAN_NONE;
	} else {
		endian = si_colorformat_endian_swap(format);
	}

	/* blend clamp should be set for all NORM/SRGB types */
	if (ntype == V_028C70_NUMBER_UNORM ||
	    ntype == V_028C70_NUMBER_SNORM ||
	    ntype == V_028C70_NUMBER_SRGB)
		blend_clamp = 1;

	/* set blend bypass according to docs if SINT/UINT or
	   8/24 COLOR variants */
	if (ntype == V_028C70_NUMBER_UINT || ntype == V_028C70_NUMBER_SINT ||
	    format == V_028C70_COLOR_8_24 || format == V_028C70_COLOR_24_8 ||
	    format == V_028C70_COLOR_X24_8_32_FLOAT) {
		blend_clamp = 0;
		blend_bypass = 1;
	}

	color_info = S_028C70_FORMAT(format) |
		S_028C70_COMP_SWAP(swap) |
		S_028C70_BLEND_CLAMP(blend_clamp) |
		S_028C70_BLEND_BYPASS(blend_bypass) |
		S_028C70_NUMBER_TYPE(ntype) |
		S_028C70_ENDIAN(endian);

	rctx->alpha_ref_dirty = true;

	offset += r600_resource_va(rctx->context.screen, state->cbufs[cb]->texture);
	offset >>= 8;

	/* FIXME handle enabling of CB beyond BASE8 which has different offset */
	si_pm4_add_bo(pm4, &rtex->resource, RADEON_USAGE_READWRITE);
	si_pm4_set_reg(pm4, R_028C60_CB_COLOR0_BASE + cb * 0x3C, offset);
	si_pm4_set_reg(pm4, R_028C64_CB_COLOR0_PITCH + cb * 0x3C, S_028C64_TILE_MAX(pitch));
	si_pm4_set_reg(pm4, R_028C68_CB_COLOR0_SLICE + cb * 0x3C, S_028C68_TILE_MAX(slice));

	if (rtex->surface.level[level].mode < RADEON_SURF_MODE_1D) {
		si_pm4_set_reg(pm4, R_028C6C_CB_COLOR0_VIEW + cb * 0x3C, 0x00000000);
	} else {
		si_pm4_set_reg(pm4, R_028C6C_CB_COLOR0_VIEW + cb * 0x3C,
			       S_028C6C_SLICE_START(state->cbufs[cb]->u.tex.first_layer) |
			       S_028C6C_SLICE_MAX(state->cbufs[cb]->u.tex.last_layer));
	}
	si_pm4_set_reg(pm4, R_028C70_CB_COLOR0_INFO + cb * 0x3C, color_info);
	si_pm4_set_reg(pm4, R_028C74_CB_COLOR0_ATTRIB + cb * 0x3C, color_attrib);
}

static void si_db(struct r600_context *rctx, struct si_pm4_state *pm4,
		  const struct pipe_framebuffer_state *state)
{
	struct r600_resource_texture *rtex;
	struct r600_surface *surf;
	unsigned level, first_layer, pitch, slice, format;
	uint32_t db_z_info, stencil_info;
	uint64_t offset;

	if (state->zsbuf == NULL) {
		si_pm4_set_reg(pm4, R_028040_DB_Z_INFO, 0);
		si_pm4_set_reg(pm4, R_028044_DB_STENCIL_INFO, 0);
		return;
	}

	surf = (struct r600_surface *)state->zsbuf;
	level = surf->base.u.tex.level;
	rtex = (struct r600_resource_texture*)surf->base.texture;

	first_layer = surf->base.u.tex.first_layer;
	format = si_translate_dbformat(rtex->real_format);

	offset = r600_resource_va(rctx->context.screen, surf->base.texture);
	offset += rtex->surface.level[level].offset;
	pitch = (rtex->surface.level[level].nblk_x / 8) - 1;
	slice = (rtex->surface.level[level].nblk_x * rtex->surface.level[level].nblk_y) / 64;
	if (slice) {
		slice = slice - 1;
	}
	offset >>= 8;

	si_pm4_add_bo(pm4, &rtex->resource, RADEON_USAGE_READWRITE);
	si_pm4_set_reg(pm4, R_028048_DB_Z_READ_BASE, offset);
	si_pm4_set_reg(pm4, R_028050_DB_Z_WRITE_BASE, offset);
	si_pm4_set_reg(pm4, R_028008_DB_DEPTH_VIEW,
		       S_028008_SLICE_START(state->zsbuf->u.tex.first_layer) |
		       S_028008_SLICE_MAX(state->zsbuf->u.tex.last_layer));

	db_z_info = S_028040_FORMAT(format);
	stencil_info = S_028044_FORMAT(rtex->stencil != 0);

	switch (format) {
	case V_028040_Z_16:
		db_z_info |= S_028040_TILE_MODE_INDEX(5);
		stencil_info |= S_028044_TILE_MODE_INDEX(5);
		break;
	case V_028040_Z_24:
	case V_028040_Z_32_FLOAT:
		db_z_info |= S_028040_TILE_MODE_INDEX(6);
		stencil_info |= S_028044_TILE_MODE_INDEX(6);
		break;
	default:
		db_z_info |= S_028040_TILE_MODE_INDEX(7);
		stencil_info |= S_028044_TILE_MODE_INDEX(7);
	}

	if (rtex->stencil) {
		uint64_t stencil_offset =
			r600_texture_get_offset(rtex->stencil, level, first_layer);

		stencil_offset += r600_resource_va(rctx->context.screen, (void*)rtex->stencil);
		stencil_offset >>= 8;

		si_pm4_add_bo(pm4, &rtex->stencil->resource, RADEON_USAGE_READWRITE);
		si_pm4_set_reg(pm4, R_02804C_DB_STENCIL_READ_BASE, stencil_offset);
		si_pm4_set_reg(pm4, R_028054_DB_STENCIL_WRITE_BASE, stencil_offset);
		si_pm4_set_reg(pm4, R_028044_DB_STENCIL_INFO, stencil_info);
	} else {
		si_pm4_set_reg(pm4, R_028044_DB_STENCIL_INFO, 0);
	}

	if (format != ~0U) {
		si_pm4_set_reg(pm4, R_02803C_DB_DEPTH_INFO, 0x1);
		si_pm4_set_reg(pm4, R_028040_DB_Z_INFO, db_z_info);
		si_pm4_set_reg(pm4, R_028058_DB_DEPTH_SIZE, S_028058_PITCH_TILE_MAX(pitch));
		si_pm4_set_reg(pm4, R_02805C_DB_DEPTH_SLICE, S_02805C_SLICE_TILE_MAX(slice));

	} else {
		si_pm4_set_reg(pm4, R_028040_DB_Z_INFO, 0);
	}
}

static void si_set_framebuffer_state(struct pipe_context *ctx,
				     const struct pipe_framebuffer_state *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct si_pm4_state *pm4 = CALLOC_STRUCT(si_pm4_state);
	uint32_t shader_mask, tl, br;
	int tl_x, tl_y, br_x, br_y;

	if (pm4 == NULL)
		return;

	si_pm4_inval_fb_cache(pm4, state->nr_cbufs);

	if (state->zsbuf)
		si_pm4_inval_zsbuf_cache(pm4);

	util_copy_framebuffer_state(&rctx->framebuffer, state);

	/* build states */
	rctx->have_depth_fb = 0;
	for (int i = 0; i < state->nr_cbufs; i++) {
		si_cb(rctx, pm4, state, i);
	}
	si_db(rctx, pm4, state);

	shader_mask = 0;
	for (int i = 0; i < state->nr_cbufs; i++) {
		shader_mask |= 0xf << (i * 4);
	}
	tl_x = 0;
	tl_y = 0;
	br_x = state->width;
	br_y = state->height;
#if 0 /* These shouldn't be necessary on SI, see PA_SC_ENHANCE register */
	/* EG hw workaround */
	if (br_x == 0)
		tl_x = 1;
	if (br_y == 0)
		tl_y = 1;
	/* cayman hw workaround */
	if (rctx->chip_class == CAYMAN) {
		if (br_x == 1 && br_y == 1)
			br_x = 2;
	}
#endif
	tl = S_028240_TL_X(tl_x) | S_028240_TL_Y(tl_y);
	br = S_028244_BR_X(br_x) | S_028244_BR_Y(br_y);

	si_pm4_set_reg(pm4, R_028240_PA_SC_GENERIC_SCISSOR_TL, tl);
	si_pm4_set_reg(pm4, R_028244_PA_SC_GENERIC_SCISSOR_BR, br);
	si_pm4_set_reg(pm4, R_028250_PA_SC_VPORT_SCISSOR_0_TL, tl);
	si_pm4_set_reg(pm4, R_028254_PA_SC_VPORT_SCISSOR_0_BR, br);
	si_pm4_set_reg(pm4, R_028030_PA_SC_SCREEN_SCISSOR_TL, tl);
	si_pm4_set_reg(pm4, R_028034_PA_SC_SCREEN_SCISSOR_BR, br);
	si_pm4_set_reg(pm4, R_028204_PA_SC_WINDOW_SCISSOR_TL, tl);
	si_pm4_set_reg(pm4, R_028208_PA_SC_WINDOW_SCISSOR_BR, br);
	si_pm4_set_reg(pm4, R_028200_PA_SC_WINDOW_OFFSET, 0x00000000);
	si_pm4_set_reg(pm4, R_028230_PA_SC_EDGERULE, 0xAAAAAAAA);
	si_pm4_set_reg(pm4, R_02823C_CB_SHADER_MASK, shader_mask);
	si_pm4_set_reg(pm4, R_028BE0_PA_SC_AA_CONFIG, 0x00000000);

	si_pm4_set_state(rctx, framebuffer, pm4);
	si_update_fb_rs_state(rctx);
}

void si_init_state_functions(struct r600_context *rctx)
{
	rctx->context.create_blend_state = si_create_blend_state;
	rctx->context.bind_blend_state = si_bind_blend_state;
	rctx->context.delete_blend_state = si_delete_blend_state;
	rctx->context.set_blend_color = si_set_blend_color;

	rctx->context.create_rasterizer_state = si_create_rs_state;
	rctx->context.bind_rasterizer_state = si_bind_rs_state;
	rctx->context.delete_rasterizer_state = si_delete_rs_state;

	rctx->context.create_depth_stencil_alpha_state = si_create_dsa_state;
	rctx->context.bind_depth_stencil_alpha_state = si_bind_dsa_state;
	rctx->context.delete_depth_stencil_alpha_state = si_delete_dsa_state;
	rctx->custom_dsa_flush = si_create_db_flush_dsa(rctx);

	rctx->context.set_clip_state = si_set_clip_state;
	rctx->context.set_scissor_state = si_set_scissor_state;
	rctx->context.set_viewport_state = si_set_viewport_state;

	rctx->context.set_framebuffer_state = si_set_framebuffer_state;
}

/*
 * Copyright 2010 Jerome Glisse <glisse@freedesktop.org>
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
 */

/* TODO:
 *	- fix mask for depth control & cull for query
 */
#include <stdio.h>
#include <errno.h>
#include <pipe/p_defines.h>
#include <pipe/p_state.h>
#include <pipe/p_context.h>
#include <tgsi/tgsi_scan.h>
#include <tgsi/tgsi_parse.h>
#include <tgsi/tgsi_util.h>
#include <util/u_double_list.h>
#include <util/u_pack_color.h>
#include <util/u_memory.h>
#include <util/u_inlines.h>
#include <util/u_framebuffer.h>
#include "util/u_transfer.h"
#include <pipebuffer/pb_buffer.h>
#include "r600.h"
#include "r600d.h"
#include "r600_resource.h"
#include "r600_shader.h"
#include "r600_pipe.h"
#include "r600_state_inlines.h"

void r600_polygon_offset_update(struct r600_pipe_context *rctx)
{
	struct r600_pipe_state state;

	state.id = R600_PIPE_STATE_POLYGON_OFFSET;
	state.nregs = 0;
	if (rctx->rasterizer && rctx->framebuffer.zsbuf) {
		float offset_units = rctx->rasterizer->offset_units;
		unsigned offset_db_fmt_cntl = 0, depth;

		switch (rctx->framebuffer.zsbuf->texture->format) {
		case PIPE_FORMAT_Z24X8_UNORM:
		case PIPE_FORMAT_Z24_UNORM_S8_USCALED:
			depth = -24;
			offset_units *= 2.0f;
			break;
		case PIPE_FORMAT_Z32_FLOAT:
			depth = -23;
			offset_units *= 1.0f;
			offset_db_fmt_cntl |= S_028DF8_POLY_OFFSET_DB_IS_FLOAT_FMT(1);
			break;
		case PIPE_FORMAT_Z16_UNORM:
			depth = -16;
			offset_units *= 4.0f;
			break;
		default:
			return;
		}
		/* FIXME some of those reg can be computed with cso */
		offset_db_fmt_cntl |= S_028DF8_POLY_OFFSET_NEG_NUM_DB_BITS(depth);
		r600_pipe_state_add_reg(&state,
				R_028E00_PA_SU_POLY_OFFSET_FRONT_SCALE,
				fui(rctx->rasterizer->offset_scale), 0xFFFFFFFF, NULL);
		r600_pipe_state_add_reg(&state,
				R_028E04_PA_SU_POLY_OFFSET_FRONT_OFFSET,
				fui(offset_units), 0xFFFFFFFF, NULL);
		r600_pipe_state_add_reg(&state,
				R_028E08_PA_SU_POLY_OFFSET_BACK_SCALE,
				fui(rctx->rasterizer->offset_scale), 0xFFFFFFFF, NULL);
		r600_pipe_state_add_reg(&state,
				R_028E0C_PA_SU_POLY_OFFSET_BACK_OFFSET,
				fui(offset_units), 0xFFFFFFFF, NULL);
		r600_pipe_state_add_reg(&state,
				R_028DF8_PA_SU_POLY_OFFSET_DB_FMT_CNTL,
				offset_db_fmt_cntl, 0xFFFFFFFF, NULL);
		r600_context_pipe_state_set(&rctx->ctx, &state);
	}
}

static void r600_set_blend_color(struct pipe_context *ctx,
					const struct pipe_blend_color *state)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct r600_pipe_state *rstate = CALLOC_STRUCT(r600_pipe_state);

	if (rstate == NULL)
		return;

	rstate->id = R600_PIPE_STATE_BLEND_COLOR;
	r600_pipe_state_add_reg(rstate, R_028414_CB_BLEND_RED, fui(state->color[0]), 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028418_CB_BLEND_GREEN, fui(state->color[1]), 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_02841C_CB_BLEND_BLUE, fui(state->color[2]), 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028420_CB_BLEND_ALPHA, fui(state->color[3]), 0xFFFFFFFF, NULL);
	free(rctx->states[R600_PIPE_STATE_BLEND_COLOR]);
	rctx->states[R600_PIPE_STATE_BLEND_COLOR] = rstate;
	r600_context_pipe_state_set(&rctx->ctx, rstate);
}

static void *r600_create_blend_state(struct pipe_context *ctx,
					const struct pipe_blend_state *state)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct r600_pipe_blend *blend = CALLOC_STRUCT(r600_pipe_blend);
	struct r600_pipe_state *rstate;
	u32 color_control = 0, target_mask;

	if (blend == NULL) {
		return NULL;
	}
	rstate = &blend->rstate;

	rstate->id = R600_PIPE_STATE_BLEND;

	target_mask = 0;

	/* R600 does not support per-MRT blends */
	if (rctx->family > CHIP_R600)
		color_control |= S_028808_PER_MRT_BLEND(1);
	if (state->logicop_enable) {
		color_control |= (state->logicop_func << 16) | (state->logicop_func << 20);
	} else {
		color_control |= (0xcc << 16);
	}
	/* we pretend 8 buffer are used, CB_SHADER_MASK will disable unused one */
	if (state->independent_blend_enable) {
		for (int i = 0; i < 8; i++) {
			if (state->rt[i].blend_enable) {
				color_control |= S_028808_TARGET_BLEND_ENABLE(1 << i);
			}
			target_mask |= (state->rt[i].colormask << (4 * i));
		}
	} else {
		for (int i = 0; i < 8; i++) {
			if (state->rt[0].blend_enable) {
				color_control |= S_028808_TARGET_BLEND_ENABLE(1 << i);
			}
			target_mask |= (state->rt[0].colormask << (4 * i));
		}
	}
	blend->cb_target_mask = target_mask;
	/* MULTIWRITE_ENABLE is controlled by r600_pipe_shader_ps(). */
	r600_pipe_state_add_reg(rstate, R_028808_CB_COLOR_CONTROL,
				color_control, 0xFFFFFFFD, NULL);

	for (int i = 0; i < 8; i++) {
		/* state->rt entries > 0 only written if independent blending */
		const int j = state->independent_blend_enable ? i : 0;

		unsigned eqRGB = state->rt[j].rgb_func;
		unsigned srcRGB = state->rt[j].rgb_src_factor;
		unsigned dstRGB = state->rt[j].rgb_dst_factor;

		unsigned eqA = state->rt[j].alpha_func;
		unsigned srcA = state->rt[j].alpha_src_factor;
		unsigned dstA = state->rt[j].alpha_dst_factor;
		uint32_t bc = 0;

		if (!state->rt[j].blend_enable)
			continue;

		bc |= S_028804_COLOR_COMB_FCN(r600_translate_blend_function(eqRGB));
		bc |= S_028804_COLOR_SRCBLEND(r600_translate_blend_factor(srcRGB));
		bc |= S_028804_COLOR_DESTBLEND(r600_translate_blend_factor(dstRGB));

		if (srcA != srcRGB || dstA != dstRGB || eqA != eqRGB) {
			bc |= S_028804_SEPARATE_ALPHA_BLEND(1);
			bc |= S_028804_ALPHA_COMB_FCN(r600_translate_blend_function(eqA));
			bc |= S_028804_ALPHA_SRCBLEND(r600_translate_blend_factor(srcA));
			bc |= S_028804_ALPHA_DESTBLEND(r600_translate_blend_factor(dstA));
		}

		/* R600 does not support per-MRT blends */
		if (rctx->family > CHIP_R600)
			r600_pipe_state_add_reg(rstate, R_028780_CB_BLEND0_CONTROL + i * 4, bc, 0xFFFFFFFF, NULL);
		if (i == 0)
			r600_pipe_state_add_reg(rstate, R_028804_CB_BLEND_CONTROL, bc, 0xFFFFFFFF, NULL);
	}
	return rstate;
}

static void *r600_create_dsa_state(struct pipe_context *ctx,
				   const struct pipe_depth_stencil_alpha_state *state)
{
	struct r600_pipe_state *rstate = CALLOC_STRUCT(r600_pipe_state);
	unsigned db_depth_control, alpha_test_control, alpha_ref, db_shader_control;
	unsigned stencil_ref_mask, stencil_ref_mask_bf, db_render_override, db_render_control;

	if (rstate == NULL) {
		return NULL;
	}

	rstate->id = R600_PIPE_STATE_DSA;
	/* depth TODO some of those db_shader_control field depend on shader adjust mask & add it to shader */
	db_shader_control = S_02880C_Z_ORDER(V_02880C_EARLY_Z_THEN_LATE_Z);
	stencil_ref_mask = 0;
	stencil_ref_mask_bf = 0;
	db_depth_control = S_028800_Z_ENABLE(state->depth.enabled) |
		S_028800_Z_WRITE_ENABLE(state->depth.writemask) |
		S_028800_ZFUNC(state->depth.func);

	/* stencil */
	if (state->stencil[0].enabled) {
		db_depth_control |= S_028800_STENCIL_ENABLE(1);
		db_depth_control |= S_028800_STENCILFUNC(r600_translate_ds_func(state->stencil[0].func));
		db_depth_control |= S_028800_STENCILFAIL(r600_translate_stencil_op(state->stencil[0].fail_op));
		db_depth_control |= S_028800_STENCILZPASS(r600_translate_stencil_op(state->stencil[0].zpass_op));
		db_depth_control |= S_028800_STENCILZFAIL(r600_translate_stencil_op(state->stencil[0].zfail_op));


		stencil_ref_mask = S_028430_STENCILMASK(state->stencil[0].valuemask) |
			S_028430_STENCILWRITEMASK(state->stencil[0].writemask);
		if (state->stencil[1].enabled) {
			db_depth_control |= S_028800_BACKFACE_ENABLE(1);
			db_depth_control |= S_028800_STENCILFUNC_BF(r600_translate_ds_func(state->stencil[1].func));
			db_depth_control |= S_028800_STENCILFAIL_BF(r600_translate_stencil_op(state->stencil[1].fail_op));
			db_depth_control |= S_028800_STENCILZPASS_BF(r600_translate_stencil_op(state->stencil[1].zpass_op));
			db_depth_control |= S_028800_STENCILZFAIL_BF(r600_translate_stencil_op(state->stencil[1].zfail_op));
			stencil_ref_mask_bf = S_028434_STENCILMASK_BF(state->stencil[1].valuemask) |
				S_028434_STENCILWRITEMASK_BF(state->stencil[1].writemask);
		}
	}

	/* alpha */
	alpha_test_control = 0;
	alpha_ref = 0;
	if (state->alpha.enabled) {
		alpha_test_control = S_028410_ALPHA_FUNC(state->alpha.func);
		alpha_test_control |= S_028410_ALPHA_TEST_ENABLE(1);
		alpha_ref = fui(state->alpha.ref_value);
	}

	/* misc */
	db_render_control = 0;
	db_render_override = S_028D10_FORCE_HIZ_ENABLE(V_028D10_FORCE_DISABLE) |
		S_028D10_FORCE_HIS_ENABLE0(V_028D10_FORCE_DISABLE) |
		S_028D10_FORCE_HIS_ENABLE1(V_028D10_FORCE_DISABLE);
	/* TODO db_render_override depends on query */
	r600_pipe_state_add_reg(rstate, R_028028_DB_STENCIL_CLEAR, 0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_02802C_DB_DEPTH_CLEAR, 0x3F800000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028410_SX_ALPHA_TEST_CONTROL, alpha_test_control, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate,
				R_028430_DB_STENCILREFMASK, stencil_ref_mask,
				0xFFFFFFFF & C_028430_STENCILREF, NULL);
	r600_pipe_state_add_reg(rstate,
				R_028434_DB_STENCILREFMASK_BF, stencil_ref_mask_bf,
				0xFFFFFFFF & C_028434_STENCILREF_BF, NULL);
	r600_pipe_state_add_reg(rstate, R_028438_SX_ALPHA_REF, alpha_ref, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_0286E0_SPI_FOG_FUNC_SCALE, 0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_0286E4_SPI_FOG_FUNC_BIAS, 0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_0286DC_SPI_FOG_CNTL, 0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028800_DB_DEPTH_CONTROL, db_depth_control, 0xFFFFFFFF, NULL);
	/* The DB_SHADER_CONTROL mask is 0xFFFFFFBC since Z_EXPORT_ENABLE,
	 * STENCIL_EXPORT_ENABLE and KILL_ENABLE are controlled by
	 * r600_pipe_shader_ps().*/
	r600_pipe_state_add_reg(rstate, R_02880C_DB_SHADER_CONTROL, db_shader_control, 0xFFFFFFBC, NULL);
	r600_pipe_state_add_reg(rstate, R_028D0C_DB_RENDER_CONTROL, db_render_control, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028D10_DB_RENDER_OVERRIDE, db_render_override, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028D2C_DB_SRESULTS_COMPARE_STATE1, 0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028D30_DB_PRELOAD_CONTROL, 0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028D44_DB_ALPHA_TO_MASK, 0x0000AA00, 0xFFFFFFFF, NULL);

	return rstate;
}

static void *r600_create_rs_state(struct pipe_context *ctx,
					const struct pipe_rasterizer_state *state)
{
	struct r600_pipe_rasterizer *rs = CALLOC_STRUCT(r600_pipe_rasterizer);
	struct r600_pipe_state *rstate;
	unsigned tmp;
	unsigned prov_vtx = 1, polygon_dual_mode;
	unsigned clip_rule;

	if (rs == NULL) {
		return NULL;
	}

	rstate = &rs->rstate;
	rs->flatshade = state->flatshade;
	rs->sprite_coord_enable = state->sprite_coord_enable;

	clip_rule = state->scissor ? 0xAAAA : 0xFFFF;
	/* offset */
	rs->offset_units = state->offset_units;
	rs->offset_scale = state->offset_scale * 12.0f;

	rstate->id = R600_PIPE_STATE_RASTERIZER;
	if (state->flatshade_first)
		prov_vtx = 0;
	tmp = S_0286D4_FLAT_SHADE_ENA(1);
	if (state->sprite_coord_enable) {
		tmp |= S_0286D4_PNT_SPRITE_ENA(1) |
			S_0286D4_PNT_SPRITE_OVRD_X(2) |
			S_0286D4_PNT_SPRITE_OVRD_Y(3) |
			S_0286D4_PNT_SPRITE_OVRD_Z(0) |
			S_0286D4_PNT_SPRITE_OVRD_W(1);
		if (state->sprite_coord_mode != PIPE_SPRITE_COORD_UPPER_LEFT) {
			tmp |= S_0286D4_PNT_SPRITE_TOP_1(1);
		}
	}
	r600_pipe_state_add_reg(rstate, R_0286D4_SPI_INTERP_CONTROL_0, tmp, 0xFFFFFFFF, NULL);

	polygon_dual_mode = (state->fill_front != PIPE_POLYGON_MODE_FILL ||
				state->fill_back != PIPE_POLYGON_MODE_FILL);
	r600_pipe_state_add_reg(rstate, R_028814_PA_SU_SC_MODE_CNTL,
		S_028814_PROVOKING_VTX_LAST(prov_vtx) |
		S_028814_CULL_FRONT((state->cull_face & PIPE_FACE_FRONT) ? 1 : 0) |
		S_028814_CULL_BACK((state->cull_face & PIPE_FACE_BACK) ? 1 : 0) |
		S_028814_FACE(!state->front_ccw) |
		S_028814_POLY_OFFSET_FRONT_ENABLE(state->offset_tri) |
		S_028814_POLY_OFFSET_BACK_ENABLE(state->offset_tri) |
		S_028814_POLY_OFFSET_PARA_ENABLE(state->offset_tri) |
		S_028814_POLY_MODE(polygon_dual_mode) |
		S_028814_POLYMODE_FRONT_PTYPE(r600_translate_fill(state->fill_front)) |
		S_028814_POLYMODE_BACK_PTYPE(r600_translate_fill(state->fill_back)), 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_02881C_PA_CL_VS_OUT_CNTL,
			S_02881C_USE_VTX_POINT_SIZE(state->point_size_per_vertex) |
			S_02881C_VS_OUT_MISC_VEC_ENA(state->point_size_per_vertex), 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028820_PA_CL_NANINF_CNTL, 0x00000000, 0xFFFFFFFF, NULL);
	/* point size 12.4 fixed point */
	tmp = (unsigned)(state->point_size * 8.0);
	r600_pipe_state_add_reg(rstate, R_028A00_PA_SU_POINT_SIZE, S_028A00_HEIGHT(tmp) | S_028A00_WIDTH(tmp), 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028A04_PA_SU_POINT_MINMAX, 0x80000000, 0xFFFFFFFF, NULL);

	tmp = (unsigned)state->line_width * 8;
	r600_pipe_state_add_reg(rstate, R_028A08_PA_SU_LINE_CNTL, S_028A08_WIDTH(tmp), 0xFFFFFFFF, NULL);

	r600_pipe_state_add_reg(rstate, R_028A0C_PA_SC_LINE_STIPPLE, 0x00000005, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028A48_PA_SC_MPASS_PS_CNTL, 0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028C00_PA_SC_LINE_CNTL, 0x00000400, 0xFFFFFFFF, NULL);

	r600_pipe_state_add_reg(rstate, R_028C08_PA_SU_VTX_CNTL,
				S_028C08_PIX_CENTER_HALF(state->gl_rasterization_rules),
				0xFFFFFFFF, NULL);

	r600_pipe_state_add_reg(rstate, R_028C0C_PA_CL_GB_VERT_CLIP_ADJ, 0x3F800000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028C10_PA_CL_GB_VERT_DISC_ADJ, 0x3F800000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028C14_PA_CL_GB_HORZ_CLIP_ADJ, 0x3F800000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028C18_PA_CL_GB_HORZ_DISC_ADJ, 0x3F800000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028DFC_PA_SU_POLY_OFFSET_CLAMP, 0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_02820C_PA_SC_CLIPRECT_RULE, clip_rule, 0xFFFFFFFF, NULL);

	return rstate;
}

static void *r600_create_sampler_state(struct pipe_context *ctx,
					const struct pipe_sampler_state *state)
{
	struct r600_pipe_state *rstate = CALLOC_STRUCT(r600_pipe_state);
	union util_color uc;

	if (rstate == NULL) {
		return NULL;
	}

	rstate->id = R600_PIPE_STATE_SAMPLER;
	util_pack_color(state->border_color, PIPE_FORMAT_B8G8R8A8_UNORM, &uc);
	r600_pipe_state_add_reg(rstate, R_03C000_SQ_TEX_SAMPLER_WORD0_0,
			S_03C000_CLAMP_X(r600_tex_wrap(state->wrap_s)) |
			S_03C000_CLAMP_Y(r600_tex_wrap(state->wrap_t)) |
			S_03C000_CLAMP_Z(r600_tex_wrap(state->wrap_r)) |
			S_03C000_XY_MAG_FILTER(r600_tex_filter(state->mag_img_filter)) |
			S_03C000_XY_MIN_FILTER(r600_tex_filter(state->min_img_filter)) |
			S_03C000_MIP_FILTER(r600_tex_mipfilter(state->min_mip_filter)) |
			S_03C000_DEPTH_COMPARE_FUNCTION(r600_tex_compare(state->compare_func)) |
			S_03C000_BORDER_COLOR_TYPE(uc.ui ? V_03C000_SQ_TEX_BORDER_COLOR_REGISTER : 0), 0xFFFFFFFF, NULL);
	/* FIXME LOD it depends on texture base level ... */
	r600_pipe_state_add_reg(rstate, R_03C004_SQ_TEX_SAMPLER_WORD1_0,
			S_03C004_MIN_LOD(S_FIXED(CLAMP(state->min_lod, 0, 15), 6)) |
			S_03C004_MAX_LOD(S_FIXED(CLAMP(state->max_lod, 0, 15), 6)) |
			S_03C004_LOD_BIAS(S_FIXED(CLAMP(state->lod_bias, -16, 16), 6)), 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_03C008_SQ_TEX_SAMPLER_WORD2_0, S_03C008_TYPE(1), 0xFFFFFFFF, NULL);
	if (uc.ui) {
		r600_pipe_state_add_reg(rstate, R_00A400_TD_PS_SAMPLER0_BORDER_RED, fui(state->border_color[0]), 0xFFFFFFFF, NULL);
		r600_pipe_state_add_reg(rstate, R_00A404_TD_PS_SAMPLER0_BORDER_GREEN, fui(state->border_color[1]), 0xFFFFFFFF, NULL);
		r600_pipe_state_add_reg(rstate, R_00A408_TD_PS_SAMPLER0_BORDER_BLUE, fui(state->border_color[2]), 0xFFFFFFFF, NULL);
		r600_pipe_state_add_reg(rstate, R_00A40C_TD_PS_SAMPLER0_BORDER_ALPHA, fui(state->border_color[3]), 0xFFFFFFFF, NULL);
	}
	return rstate;
}

static struct pipe_sampler_view *r600_create_sampler_view(struct pipe_context *ctx,
							struct pipe_resource *texture,
							const struct pipe_sampler_view *state)
{
	struct r600_pipe_sampler_view *resource = CALLOC_STRUCT(r600_pipe_sampler_view);
	struct r600_pipe_state *rstate;
	const struct util_format_description *desc;
	struct r600_resource_texture *tmp;
	struct r600_resource *rbuffer;
	unsigned format, endian;
	uint32_t word4 = 0, yuv_format = 0, pitch = 0;
	unsigned char swizzle[4], array_mode = 0, tile_type = 0;
	struct r600_bo *bo[2];
	unsigned height, depth;

	if (resource == NULL)
		return NULL;
	rstate = &resource->state;

	/* initialize base object */
	resource->base = *state;
	resource->base.texture = NULL;
	pipe_reference(NULL, &texture->reference);
	resource->base.texture = texture;
	resource->base.reference.count = 1;
	resource->base.context = ctx;

	swizzle[0] = state->swizzle_r;
	swizzle[1] = state->swizzle_g;
	swizzle[2] = state->swizzle_b;
	swizzle[3] = state->swizzle_a;
	format = r600_translate_texformat(ctx->screen, state->format,
					  swizzle,
					  &word4, &yuv_format);
	if (format == ~0) {
		format = 0;
	}
	desc = util_format_description(state->format);
	if (desc == NULL) {
		R600_ERR("unknow format %d\n", state->format);
	}
	tmp = (struct r600_resource_texture *)texture;
	if (tmp->depth && !tmp->is_flushing_texture) {
	        r600_texture_depth_flush(ctx, texture, TRUE);
		tmp = tmp->flushed_depth_texture;
	}
	endian = r600_colorformat_endian_swap(format);

	if (tmp->force_int_type) {
		word4 &= C_038010_NUM_FORMAT_ALL;
		word4 |= S_038010_NUM_FORMAT_ALL(V_038010_SQ_NUM_FORMAT_INT);
	}
	rbuffer = &tmp->resource;
	bo[0] = rbuffer->bo;
	bo[1] = rbuffer->bo;
	pitch = align(tmp->pitch_in_blocks[0] * util_format_get_blockwidth(state->format), 8);
	array_mode = tmp->array_mode[0];
	tile_type = tmp->tile_type;

	height = texture->height0;
	depth = texture->depth0;
	if (texture->target == PIPE_TEXTURE_1D_ARRAY) {
	        height = 1;
		depth = texture->array_size;
	} else if (texture->target == PIPE_TEXTURE_2D_ARRAY) {
		depth = texture->array_size;
	}

	/* FIXME properly handle first level != 0 */
	r600_pipe_state_add_reg(rstate, R_038000_RESOURCE0_WORD0,
				S_038000_DIM(r600_tex_dim(texture->target)) |
				S_038000_TILE_MODE(array_mode) |
				S_038000_TILE_TYPE(tile_type) |
				S_038000_PITCH((pitch / 8) - 1) |
				S_038000_TEX_WIDTH(texture->width0 - 1), 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_038004_RESOURCE0_WORD1,
				S_038004_TEX_HEIGHT(height - 1) |
				S_038004_TEX_DEPTH(depth - 1) |
				S_038004_DATA_FORMAT(format), 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_038008_RESOURCE0_WORD2,
				(tmp->offset[0] + r600_bo_offset(bo[0])) >> 8, 0xFFFFFFFF, bo[0]);
	r600_pipe_state_add_reg(rstate, R_03800C_RESOURCE0_WORD3,
				(tmp->offset[1] + r600_bo_offset(bo[1])) >> 8, 0xFFFFFFFF, bo[1]);
	r600_pipe_state_add_reg(rstate, R_038010_RESOURCE0_WORD4,
				word4 |
				S_038010_SRF_MODE_ALL(V_038010_SRF_MODE_NO_ZERO) |
				S_038010_REQUEST_SIZE(1) |
				S_038010_ENDIAN_SWAP(endian) |
				S_038010_BASE_LEVEL(state->u.tex.first_level), 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_038014_RESOURCE0_WORD5,
				S_038014_LAST_LEVEL(state->u.tex.last_level) |
				S_038014_BASE_ARRAY(state->u.tex.first_layer) |
				S_038014_LAST_ARRAY(state->u.tex.last_layer), 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_038018_RESOURCE0_WORD6,
				S_038018_TYPE(V_038010_SQ_TEX_VTX_VALID_TEXTURE), 0xFFFFFFFF, NULL);

	return &resource->base;
}

static void r600_set_vs_sampler_view(struct pipe_context *ctx, unsigned count,
					struct pipe_sampler_view **views)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct r600_pipe_sampler_view **resource = (struct r600_pipe_sampler_view **)views;

	for (int i = 0; i < count; i++) {
		if (resource[i]) {
			r600_context_pipe_state_set_vs_resource(&rctx->ctx, &resource[i]->state,
								i + R600_MAX_CONST_BUFFERS);
		}
	}
}

static void r600_set_ps_sampler_view(struct pipe_context *ctx, unsigned count,
					struct pipe_sampler_view **views)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct r600_pipe_sampler_view **resource = (struct r600_pipe_sampler_view **)views;
	int i;

	for (i = 0; i < count; i++) {
		if (&rctx->ps_samplers.views[i]->base != views[i]) {
			if (resource[i])
				r600_context_pipe_state_set_ps_resource(&rctx->ctx, &resource[i]->state,
									i + R600_MAX_CONST_BUFFERS);
			else
				r600_context_pipe_state_set_ps_resource(&rctx->ctx, NULL,
									i + R600_MAX_CONST_BUFFERS);

			pipe_sampler_view_reference(
				(struct pipe_sampler_view **)&rctx->ps_samplers.views[i],
				views[i]);

		}
	}
	for (i = count; i < NUM_TEX_UNITS; i++) {
		if (rctx->ps_samplers.views[i]) {
			r600_context_pipe_state_set_ps_resource(&rctx->ctx, NULL,
								i + R600_MAX_CONST_BUFFERS);
			pipe_sampler_view_reference((struct pipe_sampler_view **)&rctx->ps_samplers.views[i], NULL);
		}
	}
	rctx->ps_samplers.n_views = count;
}

static void r600_bind_ps_sampler(struct pipe_context *ctx, unsigned count, void **states)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct r600_pipe_state **rstates = (struct r600_pipe_state **)states;

	memcpy(rctx->ps_samplers.samplers, states, sizeof(void*) * count);
	rctx->ps_samplers.n_samplers = count;

	for (int i = 0; i < count; i++) {
		r600_context_pipe_state_set_ps_sampler(&rctx->ctx, rstates[i], i);
	}
}

static void r600_bind_vs_sampler(struct pipe_context *ctx, unsigned count, void **states)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct r600_pipe_state **rstates = (struct r600_pipe_state **)states;

	for (int i = 0; i < count; i++) {
		r600_context_pipe_state_set_vs_sampler(&rctx->ctx, rstates[i], i);
	}
}

static void r600_set_clip_state(struct pipe_context *ctx,
				const struct pipe_clip_state *state)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct r600_pipe_state *rstate = CALLOC_STRUCT(r600_pipe_state);

	if (rstate == NULL)
		return;

	rctx->clip = *state;
	rstate->id = R600_PIPE_STATE_CLIP;
	for (int i = 0; i < state->nr; i++) {
		r600_pipe_state_add_reg(rstate,
					R_028E20_PA_CL_UCP0_X + i * 16,
					fui(state->ucp[i][0]), 0xFFFFFFFF, NULL);
		r600_pipe_state_add_reg(rstate,
					R_028E24_PA_CL_UCP0_Y + i * 16,
					fui(state->ucp[i][1]) , 0xFFFFFFFF, NULL);
		r600_pipe_state_add_reg(rstate,
					R_028E28_PA_CL_UCP0_Z + i * 16,
					fui(state->ucp[i][2]), 0xFFFFFFFF, NULL);
		r600_pipe_state_add_reg(rstate,
					R_028E2C_PA_CL_UCP0_W + i * 16,
					fui(state->ucp[i][3]), 0xFFFFFFFF, NULL);
	}
	r600_pipe_state_add_reg(rstate, R_028810_PA_CL_CLIP_CNTL,
			S_028810_PS_UCP_MODE(3) | ((1 << state->nr) - 1) |
			S_028810_ZCLIP_NEAR_DISABLE(state->depth_clamp) |
			S_028810_ZCLIP_FAR_DISABLE(state->depth_clamp), 0xFFFFFFFF, NULL);

	free(rctx->states[R600_PIPE_STATE_CLIP]);
	rctx->states[R600_PIPE_STATE_CLIP] = rstate;
	r600_context_pipe_state_set(&rctx->ctx, rstate);
}

static void r600_set_polygon_stipple(struct pipe_context *ctx,
					 const struct pipe_poly_stipple *state)
{
}

static void r600_set_sample_mask(struct pipe_context *pipe, unsigned sample_mask)
{
}

static void r600_set_scissor_state(struct pipe_context *ctx,
					const struct pipe_scissor_state *state)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct r600_pipe_state *rstate = CALLOC_STRUCT(r600_pipe_state);
	u32 tl, br;

	if (rstate == NULL)
		return;

	rstate->id = R600_PIPE_STATE_SCISSOR;
	tl = S_028240_TL_X(state->minx) | S_028240_TL_Y(state->miny) | S_028240_WINDOW_OFFSET_DISABLE(1);
	br = S_028244_BR_X(state->maxx) | S_028244_BR_Y(state->maxy);
	r600_pipe_state_add_reg(rstate,
				R_028210_PA_SC_CLIPRECT_0_TL, tl,
				0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate,
				R_028214_PA_SC_CLIPRECT_0_BR, br,
				0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate,
				R_028218_PA_SC_CLIPRECT_1_TL, tl,
				0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate,
				R_02821C_PA_SC_CLIPRECT_1_BR, br,
				0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate,
				R_028220_PA_SC_CLIPRECT_2_TL, tl,
				0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate,
				R_028224_PA_SC_CLIPRECT_2_BR, br,
				0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate,
				R_028228_PA_SC_CLIPRECT_3_TL, tl,
				0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate,
				R_02822C_PA_SC_CLIPRECT_3_BR, br,
				0xFFFFFFFF, NULL);

	free(rctx->states[R600_PIPE_STATE_SCISSOR]);
	rctx->states[R600_PIPE_STATE_SCISSOR] = rstate;
	r600_context_pipe_state_set(&rctx->ctx, rstate);
}

static void r600_set_stencil_ref(struct pipe_context *ctx,
				const struct pipe_stencil_ref *state)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct r600_pipe_state *rstate = CALLOC_STRUCT(r600_pipe_state);
	u32 tmp;

	if (rstate == NULL)
		return;

	rctx->stencil_ref = *state;
	rstate->id = R600_PIPE_STATE_STENCIL_REF;
	tmp = S_028430_STENCILREF(state->ref_value[0]);
	r600_pipe_state_add_reg(rstate,
				R_028430_DB_STENCILREFMASK, tmp,
				~C_028430_STENCILREF, NULL);
	tmp = S_028434_STENCILREF_BF(state->ref_value[1]);
	r600_pipe_state_add_reg(rstate,
				R_028434_DB_STENCILREFMASK_BF, tmp,
				~C_028434_STENCILREF_BF, NULL);

	free(rctx->states[R600_PIPE_STATE_STENCIL_REF]);
	rctx->states[R600_PIPE_STATE_STENCIL_REF] = rstate;
	r600_context_pipe_state_set(&rctx->ctx, rstate);
}

static void r600_set_viewport_state(struct pipe_context *ctx,
					const struct pipe_viewport_state *state)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct r600_pipe_state *rstate = CALLOC_STRUCT(r600_pipe_state);

	if (rstate == NULL)
		return;

	rctx->viewport = *state;
	rstate->id = R600_PIPE_STATE_VIEWPORT;
	r600_pipe_state_add_reg(rstate, R_0282D0_PA_SC_VPORT_ZMIN_0, 0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_0282D4_PA_SC_VPORT_ZMAX_0, 0x3F800000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_02843C_PA_CL_VPORT_XSCALE_0, fui(state->scale[0]), 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028444_PA_CL_VPORT_YSCALE_0, fui(state->scale[1]), 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_02844C_PA_CL_VPORT_ZSCALE_0, fui(state->scale[2]), 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028440_PA_CL_VPORT_XOFFSET_0, fui(state->translate[0]), 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028448_PA_CL_VPORT_YOFFSET_0, fui(state->translate[1]), 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028450_PA_CL_VPORT_ZOFFSET_0, fui(state->translate[2]), 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028818_PA_CL_VTE_CNTL, 0x0000043F, 0xFFFFFFFF, NULL);

	free(rctx->states[R600_PIPE_STATE_VIEWPORT]);
	rctx->states[R600_PIPE_STATE_VIEWPORT] = rstate;
	r600_context_pipe_state_set(&rctx->ctx, rstate);
}

static void r600_cb(struct r600_pipe_context *rctx, struct r600_pipe_state *rstate,
			const struct pipe_framebuffer_state *state, int cb)
{
	struct r600_resource_texture *rtex;
	struct r600_resource *rbuffer;
	struct r600_surface *surf;
	unsigned level = state->cbufs[cb]->u.tex.level;
	unsigned pitch, slice;
	unsigned color_info;
	unsigned format, swap, ntype, endian;
	unsigned offset;
	const struct util_format_description *desc;
	struct r600_bo *bo[3];
	int i;

	surf = (struct r600_surface *)state->cbufs[cb];
	rtex = (struct r600_resource_texture*)state->cbufs[cb]->texture;

	if (rtex->depth && !rtex->is_flushing_texture) {
	        r600_texture_depth_flush(&rctx->context, state->cbufs[cb]->texture, TRUE);
		rtex = rtex->flushed_depth_texture;
	}

	rbuffer = &rtex->resource;
	bo[0] = rbuffer->bo;
	bo[1] = rbuffer->bo;
	bo[2] = rbuffer->bo;

	/* XXX quite sure for dx10+ hw don't need any offset hacks */
	offset = r600_texture_get_offset(rtex,
					 level, state->cbufs[cb]->u.tex.first_layer);
	pitch = rtex->pitch_in_blocks[level] / 8 - 1;
	slice = rtex->pitch_in_blocks[level] * surf->aligned_height / 64 - 1;
	desc = util_format_description(surf->base.format);

	for (i = 0; i < 4; i++) {
		if (desc->channel[i].type != UTIL_FORMAT_TYPE_VOID) {
			break;
		}
	}
	ntype = V_0280A0_NUMBER_UNORM;
	if (desc->colorspace == UTIL_FORMAT_COLORSPACE_SRGB)
		ntype = V_0280A0_NUMBER_SRGB;
	else if (desc->channel[i].type == UTIL_FORMAT_TYPE_SIGNED)
		ntype = V_0280A0_NUMBER_SNORM;

	format = r600_translate_colorformat(surf->base.format);
	swap = r600_translate_colorswap(surf->base.format);
	if(rbuffer->b.b.b.usage == PIPE_USAGE_STAGING) {
		endian = ENDIAN_NONE;
	} else {
		endian = r600_colorformat_endian_swap(format);
	}

	/* disable when gallium grows int textures */
	if ((format == FMT_32_32_32_32 || format == FMT_16_16_16_16) && rtex->force_int_type)
		ntype = V_0280A0_NUMBER_UINT;

	color_info = S_0280A0_FORMAT(format) |
		S_0280A0_COMP_SWAP(swap) |
		S_0280A0_ARRAY_MODE(rtex->array_mode[level]) |
		S_0280A0_BLEND_CLAMP(1) |
		S_0280A0_NUMBER_TYPE(ntype) |
		S_0280A0_ENDIAN(endian);

	/* on R600 this can't be set if BLEND_CLAMP isn't set,
	   if BLEND_FLOAT32 is set of > 11 bits in a UNORM or SNORM */
	if (desc->colorspace != UTIL_FORMAT_COLORSPACE_ZS &&
	    desc->channel[i].size < 12)
		color_info |= S_0280A0_SOURCE_FORMAT(V_0280A0_EXPORT_NORM);

	r600_pipe_state_add_reg(rstate,
				R_028040_CB_COLOR0_BASE + cb * 4,
				(offset + r600_bo_offset(bo[0])) >> 8, 0xFFFFFFFF, bo[0]);
	r600_pipe_state_add_reg(rstate,
				R_0280A0_CB_COLOR0_INFO + cb * 4,
				color_info, 0xFFFFFFFF, bo[0]);
	r600_pipe_state_add_reg(rstate,
				R_028060_CB_COLOR0_SIZE + cb * 4,
				S_028060_PITCH_TILE_MAX(pitch) |
				S_028060_SLICE_TILE_MAX(slice),
				0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate,
				R_028080_CB_COLOR0_VIEW + cb * 4,
				0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate,
				R_0280E0_CB_COLOR0_FRAG + cb * 4,
				r600_bo_offset(bo[1]) >> 8, 0xFFFFFFFF, bo[1]);
	r600_pipe_state_add_reg(rstate,
				R_0280C0_CB_COLOR0_TILE + cb * 4,
				r600_bo_offset(bo[2]) >> 8, 0xFFFFFFFF, bo[2]);
	r600_pipe_state_add_reg(rstate,
				R_028100_CB_COLOR0_MASK + cb * 4,
				0x00000000, 0xFFFFFFFF, NULL);
}

static void r600_db(struct r600_pipe_context *rctx, struct r600_pipe_state *rstate,
			const struct pipe_framebuffer_state *state)
{
	struct r600_resource_texture *rtex;
	struct r600_resource *rbuffer;
	struct r600_surface *surf;
	unsigned level;
	unsigned pitch, slice, format;
	unsigned offset;

	if (state->zsbuf == NULL)
		return;

	level = state->zsbuf->u.tex.level;

	surf = (struct r600_surface *)state->zsbuf;
	rtex = (struct r600_resource_texture*)state->zsbuf->texture;

	rbuffer = &rtex->resource;

	/* XXX quite sure for dx10+ hw don't need any offset hacks */
	offset = r600_texture_get_offset((struct r600_resource_texture *)state->zsbuf->texture,
					 level, state->zsbuf->u.tex.first_layer);
	pitch = rtex->pitch_in_blocks[level] / 8 - 1;
	slice = rtex->pitch_in_blocks[level] * surf->aligned_height / 64 - 1;
	format = r600_translate_dbformat(state->zsbuf->texture->format);

	r600_pipe_state_add_reg(rstate, R_02800C_DB_DEPTH_BASE,
				(offset + r600_bo_offset(rbuffer->bo)) >> 8, 0xFFFFFFFF, rbuffer->bo);
	r600_pipe_state_add_reg(rstate, R_028000_DB_DEPTH_SIZE,
				S_028000_PITCH_TILE_MAX(pitch) | S_028000_SLICE_TILE_MAX(slice),
				0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028004_DB_DEPTH_VIEW, 0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028010_DB_DEPTH_INFO,
				S_028010_ARRAY_MODE(rtex->array_mode[level]) | S_028010_FORMAT(format),
				0xFFFFFFFF, rbuffer->bo);
	r600_pipe_state_add_reg(rstate, R_028D34_DB_PREFETCH_LIMIT,
				(surf->aligned_height / 8) - 1, 0xFFFFFFFF, NULL);
}

static void r600_set_framebuffer_state(struct pipe_context *ctx,
					const struct pipe_framebuffer_state *state)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct r600_pipe_state *rstate = CALLOC_STRUCT(r600_pipe_state);
	u32 shader_mask, tl, br, shader_control, target_mask;

	if (rstate == NULL)
		return;

	r600_context_flush_dest_caches(&rctx->ctx);
	rctx->ctx.num_dest_buffers = state->nr_cbufs;

	/* unreference old buffer and reference new one */
	rstate->id = R600_PIPE_STATE_FRAMEBUFFER;

	util_copy_framebuffer_state(&rctx->framebuffer, state);

	/* build states */
	for (int i = 0; i < state->nr_cbufs; i++) {
		r600_cb(rctx, rstate, state, i);
	}
	if (state->zsbuf) {
		r600_db(rctx, rstate, state);
		rctx->ctx.num_dest_buffers++;
	}

	target_mask = 0x00000000;
	target_mask = 0xFFFFFFFF;
	shader_mask = 0;
	shader_control = 0;
	for (int i = 0; i < state->nr_cbufs; i++) {
		target_mask ^= 0xf << (i * 4);
		shader_mask |= 0xf << (i * 4);
		shader_control |= 1 << i;
	}
	tl = S_028240_TL_X(0) | S_028240_TL_Y(0) | S_028240_WINDOW_OFFSET_DISABLE(1);
	br = S_028244_BR_X(state->width) | S_028244_BR_Y(state->height);

	r600_pipe_state_add_reg(rstate,
				R_028030_PA_SC_SCREEN_SCISSOR_TL, tl,
				0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate,
				R_028034_PA_SC_SCREEN_SCISSOR_BR, br,
				0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate,
				R_028204_PA_SC_WINDOW_SCISSOR_TL, tl,
				0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate,
				R_028208_PA_SC_WINDOW_SCISSOR_BR, br,
				0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate,
				R_028240_PA_SC_GENERIC_SCISSOR_TL, tl,
				0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate,
				R_028244_PA_SC_GENERIC_SCISSOR_BR, br,
				0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate,
				R_028250_PA_SC_VPORT_SCISSOR_0_TL, tl,
				0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate,
				R_028254_PA_SC_VPORT_SCISSOR_0_BR, br,
				0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate,
				R_028200_PA_SC_WINDOW_OFFSET, 0x00000000,
				0xFFFFFFFF, NULL);
	if (rctx->family >= CHIP_RV770) {
		r600_pipe_state_add_reg(rstate,
					R_028230_PA_SC_EDGERULE, 0xAAAAAAAA,
					0xFFFFFFFF, NULL);
	}

	r600_pipe_state_add_reg(rstate, R_0287A0_CB_SHADER_CONTROL,
				shader_control, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028238_CB_TARGET_MASK,
				0x00000000, target_mask, NULL);
	r600_pipe_state_add_reg(rstate, R_02823C_CB_SHADER_MASK,
				shader_mask, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028C04_PA_SC_AA_CONFIG,
				0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028C1C_PA_SC_AA_SAMPLE_LOCS_MCTX,
				0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028C20_PA_SC_AA_SAMPLE_LOCS_8S_WD1_MCTX,
				0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028C30_CB_CLRCMP_CONTROL,
				0x01000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028C34_CB_CLRCMP_SRC,
				0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028C38_CB_CLRCMP_DST,
				0x000000FF, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028C3C_CB_CLRCMP_MSK,
				0xFFFFFFFF, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028C48_PA_SC_AA_MASK,
				0xFFFFFFFF, 0xFFFFFFFF, NULL);

	free(rctx->states[R600_PIPE_STATE_FRAMEBUFFER]);
	rctx->states[R600_PIPE_STATE_FRAMEBUFFER] = rstate;
	r600_context_pipe_state_set(&rctx->ctx, rstate);

	if (state->zsbuf) {
		r600_polygon_offset_update(rctx);
	}
}

static void r600_texture_barrier(struct pipe_context *ctx)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;

	r600_context_flush_all(&rctx->ctx, S_0085F0_TC_ACTION_ENA(1) | S_0085F0_CB_ACTION_ENA(1) |
			S_0085F0_CB0_DEST_BASE_ENA(1) | S_0085F0_CB1_DEST_BASE_ENA(1) |
			S_0085F0_CB2_DEST_BASE_ENA(1) | S_0085F0_CB3_DEST_BASE_ENA(1) |
			S_0085F0_CB4_DEST_BASE_ENA(1) | S_0085F0_CB5_DEST_BASE_ENA(1) |
			S_0085F0_CB6_DEST_BASE_ENA(1) | S_0085F0_CB7_DEST_BASE_ENA(1));
}

void r600_init_state_functions(struct r600_pipe_context *rctx)
{
	rctx->context.create_blend_state = r600_create_blend_state;
	rctx->context.create_depth_stencil_alpha_state = r600_create_dsa_state;
	rctx->context.create_fs_state = r600_create_shader_state;
	rctx->context.create_rasterizer_state = r600_create_rs_state;
	rctx->context.create_sampler_state = r600_create_sampler_state;
	rctx->context.create_sampler_view = r600_create_sampler_view;
	rctx->context.create_vertex_elements_state = r600_create_vertex_elements;
	rctx->context.create_vs_state = r600_create_shader_state;
	rctx->context.bind_blend_state = r600_bind_blend_state;
	rctx->context.bind_depth_stencil_alpha_state = r600_bind_state;
	rctx->context.bind_fragment_sampler_states = r600_bind_ps_sampler;
	rctx->context.bind_fs_state = r600_bind_ps_shader;
	rctx->context.bind_rasterizer_state = r600_bind_rs_state;
	rctx->context.bind_vertex_elements_state = r600_bind_vertex_elements;
	rctx->context.bind_vertex_sampler_states = r600_bind_vs_sampler;
	rctx->context.bind_vs_state = r600_bind_vs_shader;
	rctx->context.delete_blend_state = r600_delete_state;
	rctx->context.delete_depth_stencil_alpha_state = r600_delete_state;
	rctx->context.delete_fs_state = r600_delete_ps_shader;
	rctx->context.delete_rasterizer_state = r600_delete_rs_state;
	rctx->context.delete_sampler_state = r600_delete_state;
	rctx->context.delete_vertex_elements_state = r600_delete_vertex_element;
	rctx->context.delete_vs_state = r600_delete_vs_shader;
	rctx->context.set_blend_color = r600_set_blend_color;
	rctx->context.set_clip_state = r600_set_clip_state;
	rctx->context.set_constant_buffer = r600_set_constant_buffer;
	rctx->context.set_fragment_sampler_views = r600_set_ps_sampler_view;
	rctx->context.set_framebuffer_state = r600_set_framebuffer_state;
	rctx->context.set_polygon_stipple = r600_set_polygon_stipple;
	rctx->context.set_sample_mask = r600_set_sample_mask;
	rctx->context.set_scissor_state = r600_set_scissor_state;
	rctx->context.set_stencil_ref = r600_set_stencil_ref;
	rctx->context.set_vertex_buffers = r600_set_vertex_buffers;
	rctx->context.set_index_buffer = r600_set_index_buffer;
	rctx->context.set_vertex_sampler_views = r600_set_vs_sampler_view;
	rctx->context.set_viewport_state = r600_set_viewport_state;
	rctx->context.sampler_view_destroy = r600_sampler_view_destroy;
	rctx->context.redefine_user_buffer = u_default_redefine_user_buffer;
	rctx->context.texture_barrier = r600_texture_barrier;
}

void r600_init_config(struct r600_pipe_context *rctx)
{
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
	enum radeon_family family;
	struct r600_pipe_state *rstate = &rctx->config;
	u32 tmp;

	family = r600_get_family(rctx->radeon);
	ps_prio = 0;
	vs_prio = 1;
	gs_prio = 2;
	es_prio = 3;
	switch (family) {
	case CHIP_R600:
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
	case CHIP_RV630:
	case CHIP_RV635:
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
	case CHIP_RV610:
	case CHIP_RV620:
	case CHIP_RS780:
	case CHIP_RS880:
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
	case CHIP_RV670:
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
	case CHIP_RV770:
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
	case CHIP_RV730:
	case CHIP_RV740:
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
	case CHIP_RV710:
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

	rstate->id = R600_PIPE_STATE_CONFIG;

	/* SQ_CONFIG */
	tmp = 0;
	switch (family) {
	case CHIP_RV610:
	case CHIP_RV620:
	case CHIP_RS780:
	case CHIP_RS880:
	case CHIP_RV710:
		break;
	default:
		tmp |= S_008C00_VC_ENABLE(1);
		break;
	}
	tmp |= S_008C00_DX9_CONSTS(0);
	tmp |= S_008C00_ALU_INST_PREFER_VECTOR(1);
	tmp |= S_008C00_PS_PRIO(ps_prio);
	tmp |= S_008C00_VS_PRIO(vs_prio);
	tmp |= S_008C00_GS_PRIO(gs_prio);
	tmp |= S_008C00_ES_PRIO(es_prio);
	r600_pipe_state_add_reg(rstate, R_008C00_SQ_CONFIG, tmp, 0xFFFFFFFF, NULL);

	/* SQ_GPR_RESOURCE_MGMT_1 */
	tmp = 0;
	tmp |= S_008C04_NUM_PS_GPRS(num_ps_gprs);
	tmp |= S_008C04_NUM_VS_GPRS(num_vs_gprs);
	tmp |= S_008C04_NUM_CLAUSE_TEMP_GPRS(num_temp_gprs);
	r600_pipe_state_add_reg(rstate, R_008C04_SQ_GPR_RESOURCE_MGMT_1, tmp, 0xFFFFFFFF, NULL);

	/* SQ_GPR_RESOURCE_MGMT_2 */
	tmp = 0;
	tmp |= S_008C08_NUM_GS_GPRS(num_gs_gprs);
	tmp |= S_008C08_NUM_GS_GPRS(num_es_gprs);
	r600_pipe_state_add_reg(rstate, R_008C08_SQ_GPR_RESOURCE_MGMT_2, tmp, 0xFFFFFFFF, NULL);

	/* SQ_THREAD_RESOURCE_MGMT */
	tmp = 0;
	tmp |= S_008C0C_NUM_PS_THREADS(num_ps_threads);
	tmp |= S_008C0C_NUM_VS_THREADS(num_vs_threads);
	tmp |= S_008C0C_NUM_GS_THREADS(num_gs_threads);
	tmp |= S_008C0C_NUM_ES_THREADS(num_es_threads);
	r600_pipe_state_add_reg(rstate, R_008C0C_SQ_THREAD_RESOURCE_MGMT, tmp, 0xFFFFFFFF, NULL);

	/* SQ_STACK_RESOURCE_MGMT_1 */
	tmp = 0;
	tmp |= S_008C10_NUM_PS_STACK_ENTRIES(num_ps_stack_entries);
	tmp |= S_008C10_NUM_VS_STACK_ENTRIES(num_vs_stack_entries);
	r600_pipe_state_add_reg(rstate, R_008C10_SQ_STACK_RESOURCE_MGMT_1, tmp, 0xFFFFFFFF, NULL);

	/* SQ_STACK_RESOURCE_MGMT_2 */
	tmp = 0;
	tmp |= S_008C14_NUM_GS_STACK_ENTRIES(num_gs_stack_entries);
	tmp |= S_008C14_NUM_ES_STACK_ENTRIES(num_es_stack_entries);
	r600_pipe_state_add_reg(rstate, R_008C14_SQ_STACK_RESOURCE_MGMT_2, tmp, 0xFFFFFFFF, NULL);

	r600_pipe_state_add_reg(rstate, R_009714_VC_ENHANCE, 0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028350_SX_MISC, 0x00000000, 0xFFFFFFFF, NULL);

	if (family >= CHIP_RV770) {
		r600_pipe_state_add_reg(rstate, R_008D8C_SQ_DYN_GPR_CNTL_PS_FLUSH_REQ, 0x00004000, 0xFFFFFFFF, NULL);
		r600_pipe_state_add_reg(rstate, R_009508_TA_CNTL_AUX, 0x07000002, 0xFFFFFFFF, NULL);
		r600_pipe_state_add_reg(rstate, R_009830_DB_DEBUG, 0x00000000, 0xFFFFFFFF, NULL);
		r600_pipe_state_add_reg(rstate, R_009838_DB_WATERMARKS, 0x00420204, 0xFFFFFFFF, NULL);
		r600_pipe_state_add_reg(rstate, R_0286C8_SPI_THREAD_GROUPING, 0x00000000, 0xFFFFFFFF, NULL);
		r600_pipe_state_add_reg(rstate, R_028A4C_PA_SC_MODE_CNTL, 0x00514002, 0xFFFFFFFF, NULL);
	} else {
		r600_pipe_state_add_reg(rstate, R_008D8C_SQ_DYN_GPR_CNTL_PS_FLUSH_REQ, 0x00000000, 0xFFFFFFFF, NULL);
		r600_pipe_state_add_reg(rstate, R_009508_TA_CNTL_AUX, 0x07000003, 0xFFFFFFFF, NULL);
		r600_pipe_state_add_reg(rstate, R_009830_DB_DEBUG, 0x82000000, 0xFFFFFFFF, NULL);
		r600_pipe_state_add_reg(rstate, R_009838_DB_WATERMARKS, 0x01020204, 0xFFFFFFFF, NULL);
		r600_pipe_state_add_reg(rstate, R_0286C8_SPI_THREAD_GROUPING, 0x00000001, 0xFFFFFFFF, NULL);
		r600_pipe_state_add_reg(rstate, R_028A4C_PA_SC_MODE_CNTL, 0x00004012, 0xFFFFFFFF, NULL);
	}
	r600_pipe_state_add_reg(rstate, R_0288A8_SQ_ESGS_RING_ITEMSIZE, 0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_0288AC_SQ_GSVS_RING_ITEMSIZE, 0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_0288B0_SQ_ESTMP_RING_ITEMSIZE, 0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_0288B4_SQ_GSTMP_RING_ITEMSIZE, 0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_0288B8_SQ_VSTMP_RING_ITEMSIZE, 0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_0288BC_SQ_PSTMP_RING_ITEMSIZE, 0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_0288C0_SQ_FBUF_RING_ITEMSIZE, 0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_0288C4_SQ_REDUC_RING_ITEMSIZE, 0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_0288C8_SQ_GS_VERT_ITEMSIZE, 0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028A10_VGT_OUTPUT_PATH_CNTL, 0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028A14_VGT_HOS_CNTL, 0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028A18_VGT_HOS_MAX_TESS_LEVEL, 0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028A1C_VGT_HOS_MIN_TESS_LEVEL, 0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028A20_VGT_HOS_REUSE_DEPTH, 0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028A24_VGT_GROUP_PRIM_TYPE, 0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028A28_VGT_GROUP_FIRST_DECR, 0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028A2C_VGT_GROUP_DECR, 0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028A30_VGT_GROUP_VECT_0_CNTL, 0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028A34_VGT_GROUP_VECT_1_CNTL, 0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028A38_VGT_GROUP_VECT_0_FMT_CNTL, 0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028A3C_VGT_GROUP_VECT_1_FMT_CNTL, 0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028A40_VGT_GS_MODE, 0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028AB0_VGT_STRMOUT_EN, 0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028AB4_VGT_REUSE_OFF, 0x00000001, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028AB8_VGT_VTX_CNT_EN, 0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028B20_VGT_STRMOUT_BUFFER_EN, 0x00000000, 0xFFFFFFFF, NULL);

	r600_pipe_state_add_reg(rstate, R_02840C_VGT_MULTI_PRIM_IB_RESET_INDX, 0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028A84_VGT_PRIMITIVEID_EN, 0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028A94_VGT_MULTI_PRIM_IB_RESET_EN, 0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028AA0_VGT_INSTANCE_STEP_RATE_0, 0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028AA4_VGT_INSTANCE_STEP_RATE_1, 0x00000000, 0xFFFFFFFF, NULL);
	r600_context_pipe_state_set(&rctx->ctx, rstate);
}

void r600_pipe_shader_ps(struct pipe_context *ctx, struct r600_pipe_shader *shader)
{
	struct r600_pipe_state *rstate = &shader->rstate;
	struct r600_shader *rshader = &shader->shader;
	unsigned i, exports_ps, num_cout, spi_ps_in_control_0, spi_input_z, spi_ps_in_control_1, db_shader_control;
	int pos_index = -1, face_index = -1;

	rstate->nregs = 0;

	for (i = 0; i < rshader->ninput; i++) {
		if (rshader->input[i].name == TGSI_SEMANTIC_POSITION)
			pos_index = i;
		if (rshader->input[i].name == TGSI_SEMANTIC_FACE)
			face_index = i;
	}

	db_shader_control = 0;
	for (i = 0; i < rshader->noutput; i++) {
		if (rshader->output[i].name == TGSI_SEMANTIC_POSITION)
			db_shader_control |= S_02880C_Z_EXPORT_ENABLE(1);
		if (rshader->output[i].name == TGSI_SEMANTIC_STENCIL)
			db_shader_control |= S_02880C_STENCIL_REF_EXPORT_ENABLE(1);
	}
	if (rshader->uses_kill)
		db_shader_control |= S_02880C_KILL_ENABLE(1);

	exports_ps = 0;
	num_cout = 0;
	for (i = 0; i < rshader->noutput; i++) {
		if (rshader->output[i].name == TGSI_SEMANTIC_POSITION ||
		    rshader->output[i].name == TGSI_SEMANTIC_STENCIL)
			exports_ps |= 1;
		else if (rshader->output[i].name == TGSI_SEMANTIC_COLOR) {
			num_cout++;
		}
	}
	exports_ps |= S_028854_EXPORT_COLORS(num_cout);
	if (!exports_ps) {
		/* always at least export 1 component per pixel */
		exports_ps = 2;
	}

	spi_ps_in_control_0 = S_0286CC_NUM_INTERP(rshader->ninput) |
				S_0286CC_PERSP_GRADIENT_ENA(1);
	spi_input_z = 0;
	if (pos_index != -1) {
		spi_ps_in_control_0 |= (S_0286CC_POSITION_ENA(1) |
					S_0286CC_POSITION_CENTROID(rshader->input[pos_index].centroid) |
					S_0286CC_POSITION_ADDR(rshader->input[pos_index].gpr) |
					S_0286CC_BARYC_SAMPLE_CNTL(1));
		spi_input_z |= 1;
	}

	spi_ps_in_control_1 = 0;
	if (face_index != -1) {
		spi_ps_in_control_1 |= S_0286D0_FRONT_FACE_ENA(1) |
			S_0286D0_FRONT_FACE_ADDR(rshader->input[face_index].gpr);
	}

	r600_pipe_state_add_reg(rstate, R_0286CC_SPI_PS_IN_CONTROL_0, spi_ps_in_control_0, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_0286D0_SPI_PS_IN_CONTROL_1, spi_ps_in_control_1, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_0286D8_SPI_INPUT_Z, spi_input_z, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate,
				R_028840_SQ_PGM_START_PS,
				r600_bo_offset(shader->bo) >> 8, 0xFFFFFFFF, shader->bo);
	r600_pipe_state_add_reg(rstate,
				R_028850_SQ_PGM_RESOURCES_PS,
				S_028868_NUM_GPRS(rshader->bc.ngpr) |
				S_028868_STACK_SIZE(rshader->bc.nstack),
				0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate,
				R_028854_SQ_PGM_EXPORTS_PS,
				exports_ps, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate,
				R_0288CC_SQ_PGM_CF_OFFSET_PS,
				0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028808_CB_COLOR_CONTROL,
				S_028808_MULTIWRITE_ENABLE(!!rshader->fs_write_all),
				S_028808_MULTIWRITE_ENABLE(1),
				NULL);
	/* only set some bits here, the other bits are set in the dsa state */
	r600_pipe_state_add_reg(rstate, R_02880C_DB_SHADER_CONTROL,
				db_shader_control,
				S_02880C_Z_EXPORT_ENABLE(1) |
				S_02880C_STENCIL_REF_EXPORT_ENABLE(1) |
				S_02880C_KILL_ENABLE(1),
				NULL);

	r600_pipe_state_add_reg(rstate,
				R_03E200_SQ_LOOP_CONST_0, 0x01000FFF,
				0xFFFFFFFF, NULL);
}

void r600_pipe_shader_vs(struct pipe_context *ctx, struct r600_pipe_shader *shader)
{
	struct r600_pipe_state *rstate = &shader->rstate;
	struct r600_shader *rshader = &shader->shader;
	unsigned spi_vs_out_id[10];
	unsigned i, tmp;

	/* clear previous register */
	rstate->nregs = 0;

	/* so far never got proper semantic id from tgsi */
	/* FIXME better to move this in config things so they get emited
	 * only one time per cs
	 */
	for (i = 0; i < 10; i++) {
		spi_vs_out_id[i] = 0;
	}
	for (i = 0; i < 32; i++) {
		tmp = i << ((i & 3) * 8);
		spi_vs_out_id[i / 4] |= tmp;
	}
	for (i = 0; i < 10; i++) {
		r600_pipe_state_add_reg(rstate,
					R_028614_SPI_VS_OUT_ID_0 + i * 4,
					spi_vs_out_id[i], 0xFFFFFFFF, NULL);
	}

	r600_pipe_state_add_reg(rstate,
			R_0286C4_SPI_VS_OUT_CONFIG,
			S_0286C4_VS_EXPORT_COUNT(rshader->noutput - 2),
			0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate,
			R_028868_SQ_PGM_RESOURCES_VS,
			S_028868_NUM_GPRS(rshader->bc.ngpr) |
			S_028868_STACK_SIZE(rshader->bc.nstack),
			0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate,
			R_0288D0_SQ_PGM_CF_OFFSET_VS,
			0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate,
			R_028858_SQ_PGM_START_VS,
			r600_bo_offset(shader->bo) >> 8, 0xFFFFFFFF, shader->bo);

	r600_pipe_state_add_reg(rstate,
				R_03E200_SQ_LOOP_CONST_0 + (32 * 4), 0x01000FFF,
				0xFFFFFFFF, NULL);
}

void r600_fetch_shader(struct r600_vertex_element *ve)
{
	struct r600_pipe_state *rstate;

	rstate = &ve->rstate;
	rstate->id = R600_PIPE_STATE_FETCH_SHADER;
	rstate->nregs = 0;
	r600_pipe_state_add_reg(rstate, R_0288A4_SQ_PGM_RESOURCES_FS,
				0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_0288DC_SQ_PGM_CF_OFFSET_FS,
				0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_028894_SQ_PGM_START_FS,
				r600_bo_offset(ve->fetch_shader) >> 8,
				0xFFFFFFFF, ve->fetch_shader);
}

void *r600_create_db_flush_dsa(struct r600_pipe_context *rctx)
{
	struct pipe_depth_stencil_alpha_state dsa;
	struct r600_pipe_state *rstate;
	boolean quirk = false;

	if (rctx->family == CHIP_RV610 || rctx->family == CHIP_RV630 ||
		rctx->family == CHIP_RV620 || rctx->family == CHIP_RV635)
		quirk = true;

	memset(&dsa, 0, sizeof(dsa));

	if (quirk) {
		dsa.depth.enabled = 1;
		dsa.depth.func = PIPE_FUNC_LEQUAL;
		dsa.stencil[0].enabled = 1;
		dsa.stencil[0].func = PIPE_FUNC_ALWAYS;
		dsa.stencil[0].zpass_op = PIPE_STENCIL_OP_KEEP;
		dsa.stencil[0].zfail_op = PIPE_STENCIL_OP_INCR;
		dsa.stencil[0].writemask = 0xff;
	}

	rstate = rctx->context.create_depth_stencil_alpha_state(&rctx->context, &dsa);
	r600_pipe_state_add_reg(rstate,
				R_02880C_DB_SHADER_CONTROL,
				0x0,
				S_02880C_DUAL_EXPORT_ENABLE(1), NULL);
	r600_pipe_state_add_reg(rstate,
				R_028D0C_DB_RENDER_CONTROL,
				S_028D0C_DEPTH_COPY_ENABLE(1) |
				S_028D0C_STENCIL_COPY_ENABLE(1) |
				S_028D0C_COPY_CENTROID(1),
				S_028D0C_DEPTH_COPY_ENABLE(1) |
				S_028D0C_STENCIL_COPY_ENABLE(1) |
				S_028D0C_COPY_CENTROID(1), NULL);
	return rstate;
}

void r600_pipe_set_buffer_resource(struct r600_pipe_context *rctx,
				   struct r600_pipe_state *rstate,
				   struct r600_resource *rbuffer,
				   unsigned offset, unsigned stride)
{
	r600_pipe_state_add_reg(rstate, R_038000_RESOURCE0_WORD0,
				offset, 0xFFFFFFFF, rbuffer->bo);
	r600_pipe_state_add_reg(rstate, R_038004_RESOURCE0_WORD1,
				rbuffer->bo_size - offset - 1, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_038008_RESOURCE0_WORD2,
				S_038008_ENDIAN_SWAP(r600_endian_swap(32)) |
				S_038008_STRIDE(stride), 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_03800C_RESOURCE0_WORD3,
				0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_038010_RESOURCE0_WORD4,
				0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_038014_RESOURCE0_WORD5,
				0x00000000, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(rstate, R_038018_RESOURCE0_WORD6,
				0xC0000000, 0xFFFFFFFF, NULL);
}

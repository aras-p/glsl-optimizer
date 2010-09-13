/*
 * Copyright 2010 Jerome Glisse <glisse@freedesktop.org>
 *           2010 Red Hat Inc.
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
 *      Jerome Glisse
 *      Dave Airlie
 */
#include <util/u_inlines.h>
#include <util/u_format.h>
#include <util/u_memory.h>
#include <util/u_blitter.h>
#include "util/u_pack_color.h"
#include "r600_screen.h"
#include "r600_context.h"
#include "r600_resource.h"
#include "eg_state_inlines.h"
#include "evergreend.h"

#include "eg_states_inc.h"

static void eg_blend(struct r600_context *rctx, struct radeon_state *rstate, const struct pipe_blend_state *state)
{
	struct r600_screen *rscreen = rctx->screen;
	int i;

	radeon_state_init(rstate, rscreen->rw, R600_STATE_BLEND, 0, 0);
	rstate->states[EG_BLEND__CB_BLEND_RED] = fui(rctx->blend_color.color[0]);
	rstate->states[EG_BLEND__CB_BLEND_GREEN] = fui(rctx->blend_color.color[1]);
	rstate->states[EG_BLEND__CB_BLEND_BLUE] = fui(rctx->blend_color.color[2]);
	rstate->states[EG_BLEND__CB_BLEND_ALPHA] = fui(rctx->blend_color.color[3]);
	rstate->states[EG_BLEND__CB_BLEND0_CONTROL] = 0x00000000;
	rstate->states[EG_BLEND__CB_BLEND1_CONTROL] = 0x00000000;
	rstate->states[EG_BLEND__CB_BLEND2_CONTROL] = 0x00000000;
	rstate->states[EG_BLEND__CB_BLEND3_CONTROL] = 0x00000000;
	rstate->states[EG_BLEND__CB_BLEND4_CONTROL] = 0x00000000;
	rstate->states[EG_BLEND__CB_BLEND5_CONTROL] = 0x00000000;
	rstate->states[EG_BLEND__CB_BLEND6_CONTROL] = 0x00000000;
	rstate->states[EG_BLEND__CB_BLEND7_CONTROL] = 0x00000000;

	for (i = 0; i < 8; i++) {
		unsigned eqRGB = state->rt[i].rgb_func;
		unsigned srcRGB = state->rt[i].rgb_src_factor;
		unsigned dstRGB = state->rt[i].rgb_dst_factor;
		
		unsigned eqA = state->rt[i].alpha_func;
		unsigned srcA = state->rt[i].alpha_src_factor;
		unsigned dstA = state->rt[i].alpha_dst_factor;
		uint32_t bc = 0;

		if (!state->rt[i].blend_enable)
			continue;

		bc |= S_028780_BLEND_CONTROL_ENABLE(1);

		bc |= S_028780_COLOR_COMB_FCN(r600_translate_blend_function(eqRGB));
		bc |= S_028780_COLOR_SRCBLEND(r600_translate_blend_factor(srcRGB));
		bc |= S_028780_COLOR_DESTBLEND(r600_translate_blend_factor(dstRGB));

		if (srcA != srcRGB || dstA != dstRGB || eqA != eqRGB) {
			bc |= S_028780_SEPARATE_ALPHA_BLEND(1);
			bc |= S_028780_ALPHA_COMB_FCN(r600_translate_blend_function(eqA));
			bc |= S_028780_ALPHA_SRCBLEND(r600_translate_blend_factor(srcA));
			bc |= S_028780_ALPHA_DESTBLEND(r600_translate_blend_factor(dstA));
		}

		rstate->states[EG_BLEND__CB_BLEND0_CONTROL + i] = bc;
	}

	radeon_state_pm4(rstate);
}

static void eg_ucp(struct r600_context *rctx, struct radeon_state *rstate,
	    const struct pipe_clip_state *state)
{
	struct r600_screen *rscreen = rctx->screen;

	radeon_state_init(rstate, rscreen->rw, R600_STATE_UCP, 0, 0);

	for (int i = 0; i < state->nr; i++) {
		rstate->states[i * 4 + 0] = fui(state->ucp[i][0]);
		rstate->states[i * 4 + 1] = fui(state->ucp[i][1]);
		rstate->states[i * 4 + 2] = fui(state->ucp[i][2]);
		rstate->states[i * 4 + 3] = fui(state->ucp[i][3]);
	}
	radeon_state_pm4(rstate);
}

static void eg_cb(struct r600_context *rctx, struct radeon_state *rstate,
			const struct pipe_framebuffer_state *state, int cb)
{
	struct r600_screen *rscreen = rctx->screen;
	struct r600_resource_texture *rtex;
	struct r600_resource *rbuffer;
	unsigned level = state->cbufs[cb]->level;
	unsigned pitch, slice;
	unsigned color_info;
	unsigned format, swap, ntype;
	const struct util_format_description *desc;

	radeon_state_init(rstate, rscreen->rw, R600_STATE_CB0, cb, 0);
	rtex = (struct r600_resource_texture*)state->cbufs[cb]->texture;
	rbuffer = &rtex->resource;
	rstate->bo[0] = radeon_bo_incref(rscreen->rw, rbuffer->bo);
	rstate->placement[0] = RADEON_GEM_DOMAIN_VRAM;
	rstate->nbo = 1;
	pitch = (rtex->pitch[level] / rtex->bpt) / 8 - 1;
	slice = (rtex->pitch[level] / rtex->bpt) * state->cbufs[cb]->height / 64 - 1;

	ntype = 0;
	desc = util_format_description(rtex->resource.base.b.format);
	if (desc->colorspace == UTIL_FORMAT_COLORSPACE_SRGB)
		ntype = V_028C70_NUMBER_SRGB;

	format = r600_translate_colorformat(rtex->resource.base.b.format);
	swap = r600_translate_colorswap(rtex->resource.base.b.format);

	color_info = S_028C70_FORMAT(format) |
		S_028C70_COMP_SWAP(swap) |
		S_028C70_BLEND_CLAMP(1) |
		S_028C70_SOURCE_FORMAT(1) |
		S_028C70_NUMBER_TYPE(ntype);

	rstate->states[EG_CB__CB_COLOR0_BASE] = state->cbufs[cb]->offset >> 8;
	rstate->states[EG_CB__CB_COLOR0_INFO] = color_info;
	rstate->states[EG_CB__CB_COLOR0_PITCH] = S_028C64_PITCH_TILE_MAX(pitch);
	rstate->states[EG_CB__CB_COLOR0_SLICE] = S_028C68_SLICE_TILE_MAX(slice);
	rstate->states[EG_CB__CB_COLOR0_VIEW] = 0x00000000;
	rstate->states[EG_CB__CB_COLOR0_ATTRIB] = S_028C74_NON_DISP_TILING_ORDER(1);

	radeon_state_pm4(rstate);
}

static void eg_db(struct r600_context *rctx, struct radeon_state *rstate,
			const struct pipe_framebuffer_state *state)
{
	struct r600_screen *rscreen = rctx->screen;
	struct r600_resource_texture *rtex;
	struct r600_resource *rbuffer;
	unsigned level;
	unsigned pitch, slice, format;

	radeon_state_init(rstate, rscreen->rw, R600_STATE_DB, 0, 0);
	if (state->zsbuf == NULL)
		return;

	rtex = (struct r600_resource_texture*)state->zsbuf->texture;
	rtex->tilled = 1;
	rtex->array_mode = 2;
	rtex->tile_type = 1;
	rtex->depth = 1;
	rbuffer = &rtex->resource;

	rstate->bo[0] = radeon_bo_incref(rscreen->rw, rbuffer->bo);
	rstate->nbo = 1;
	rstate->placement[0] = RADEON_GEM_DOMAIN_VRAM;
	level = state->zsbuf->level;
	pitch = (rtex->pitch[level] / rtex->bpt) / 8 - 1;
	slice = (rtex->pitch[level] / rtex->bpt) * state->zsbuf->height / 64 - 1;
	format = r600_translate_dbformat(state->zsbuf->texture->format);
	rstate->states[EG_DB__DB_HTILE_DATA_BASE] = state->zsbuf->offset >> 8;
	rstate->states[EG_DB__DB_Z_READ_BASE] = state->zsbuf->offset >> 8;
	rstate->states[EG_DB__DB_Z_WRITE_BASE] = state->zsbuf->offset >> 8;
	rstate->states[EG_DB__DB_STENCIL_READ_BASE] = state->zsbuf->offset >> 8;
	rstate->states[EG_DB__DB_STENCIL_WRITE_BASE] = state->zsbuf->offset >> 8;
	rstate->states[EG_DB__DB_Z_INFO] = S_028040_ARRAY_MODE(rtex->array_mode) | S_028040_FORMAT(format);
	rstate->states[EG_DB__DB_DEPTH_VIEW] = 0x00000000;
	rstate->states[EG_DB__DB_DEPTH_SIZE] = S_028058_PITCH_TILE_MAX(pitch);
	rstate->states[EG_DB__DB_DEPTH_SLICE] = S_02805C_SLICE_TILE_MAX(slice);
	radeon_state_pm4(rstate);
}

static void eg_rasterizer(struct r600_context *rctx, struct radeon_state *rstate)
{
	const struct pipe_rasterizer_state *state = &rctx->rasterizer->state.rasterizer;
	const struct pipe_framebuffer_state *fb = &rctx->framebuffer->state.framebuffer;
	const struct pipe_clip_state *clip = NULL;
	struct r600_screen *rscreen = rctx->screen;
	float offset_units = 0, offset_scale = 0;
	char depth = 0;
	unsigned offset_db_fmt_cntl = 0;
	unsigned tmp;
	unsigned prov_vtx = 1;

	if (rctx->clip)
		clip = &rctx->clip->state.clip;
	if (fb->zsbuf) {
		offset_units = state->offset_units;
		offset_scale = state->offset_scale * 12.0f;
		switch (fb->zsbuf->texture->format) {
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
			R600_ERR("unsupported %d\n", fb->zsbuf->texture->format);
			return;
		}
	}
	offset_db_fmt_cntl |= S_028DF8_POLY_OFFSET_NEG_NUM_DB_BITS(depth);

	if (state->flatshade_first)
		prov_vtx = 0;

	rctx->flat_shade = state->flatshade;
	radeon_state_init(rstate, rscreen->rw, R600_STATE_RASTERIZER, 0, 0);
	rstate->states[EG_RASTERIZER__SPI_INTERP_CONTROL_0] = 0x00000000;
	if (rctx->flat_shade)
		rstate->states[EG_RASTERIZER__SPI_INTERP_CONTROL_0] |= S_0286D4_FLAT_SHADE_ENA(1);
	if (state->sprite_coord_enable) {
		rstate->states[EG_RASTERIZER__SPI_INTERP_CONTROL_0] |=
				S_0286D4_PNT_SPRITE_ENA(1) |
				S_0286D4_PNT_SPRITE_OVRD_X(2) |
				S_0286D4_PNT_SPRITE_OVRD_Y(3) |
				S_0286D4_PNT_SPRITE_OVRD_Z(0) |
				S_0286D4_PNT_SPRITE_OVRD_W(1);
		if (state->sprite_coord_mode != PIPE_SPRITE_COORD_UPPER_LEFT) {
			rstate->states[EG_RASTERIZER__SPI_INTERP_CONTROL_0] |=
					S_0286D4_PNT_SPRITE_TOP_1(1);
		}
	}
	rstate->states[EG_RASTERIZER__PA_CL_CLIP_CNTL] = 0;
	if (clip) {
		rstate->states[EG_RASTERIZER__PA_CL_CLIP_CNTL] = S_028810_PS_UCP_MODE(3) | ((1 << clip->nr) - 1);
		rstate->states[EG_RASTERIZER__PA_CL_CLIP_CNTL] |= S_028810_ZCLIP_NEAR_DISABLE(clip->depth_clamp);
		rstate->states[EG_RASTERIZER__PA_CL_CLIP_CNTL] |= S_028810_ZCLIP_FAR_DISABLE(clip->depth_clamp);
	}
	rstate->states[EG_RASTERIZER__PA_SU_SC_MODE_CNTL] =
		S_028814_PROVOKING_VTX_LAST(prov_vtx) |
		S_028814_CULL_FRONT((state->cull_face & PIPE_FACE_FRONT) ? 1 : 0) |
		S_028814_CULL_BACK((state->cull_face & PIPE_FACE_BACK) ? 1 : 0) |
		S_028814_FACE(!state->front_ccw) |
		S_028814_POLY_OFFSET_FRONT_ENABLE(state->offset_tri) |
		S_028814_POLY_OFFSET_BACK_ENABLE(state->offset_tri) |
		S_028814_POLY_OFFSET_PARA_ENABLE(state->offset_tri);
	rstate->states[EG_RASTERIZER__PA_CL_VS_OUT_CNTL] =
			S_02881C_USE_VTX_POINT_SIZE(state->point_size_per_vertex) |
			S_02881C_VS_OUT_MISC_VEC_ENA(state->point_size_per_vertex);
	rstate->states[EG_RASTERIZER__PA_CL_NANINF_CNTL] = 0x00000000;
	/* point size 12.4 fixed point */
	tmp = (unsigned)(state->point_size * 8.0);
	rstate->states[EG_RASTERIZER__PA_SU_POINT_SIZE] = S_028A00_HEIGHT(tmp) | S_028A00_WIDTH(tmp);
	rstate->states[EG_RASTERIZER__PA_SU_POINT_MINMAX] = 0x80000000;
	rstate->states[EG_RASTERIZER__PA_SU_LINE_CNTL] = 0x00000008;
	rstate->states[EG_RASTERIZER__PA_SU_VTX_CNTL] = 0x00000005;

	rstate->states[EG_RASTERIZER__PA_SC_MPASS_PS_CNTL] = 0x00000000;
	rstate->states[EG_RASTERIZER__PA_SC_LINE_CNTL] = 0x00000400;
	rstate->states[EG_RASTERIZER__PA_CL_GB_VERT_CLIP_ADJ] = 0x3F800000;
	rstate->states[EG_RASTERIZER__PA_CL_GB_VERT_DISC_ADJ] = 0x3F800000;
	rstate->states[EG_RASTERIZER__PA_CL_GB_HORZ_CLIP_ADJ] = 0x3F800000;
	rstate->states[EG_RASTERIZER__PA_CL_GB_HORZ_DISC_ADJ] = 0x3F800000;
	rstate->states[EG_RASTERIZER__PA_SU_POLY_OFFSET_DB_FMT_CNTL] = offset_db_fmt_cntl;
	rstate->states[EG_RASTERIZER__PA_SU_POLY_OFFSET_CLAMP] = 0x00000000;
	rstate->states[EG_RASTERIZER__PA_SU_POLY_OFFSET_FRONT_SCALE] = fui(offset_scale);
	rstate->states[EG_RASTERIZER__PA_SU_POLY_OFFSET_FRONT_OFFSET] = fui(offset_units);
	rstate->states[EG_RASTERIZER__PA_SU_POLY_OFFSET_BACK_SCALE] = fui(offset_scale);
	rstate->states[EG_RASTERIZER__PA_SU_POLY_OFFSET_BACK_OFFSET] = fui(offset_units);
	radeon_state_pm4(rstate);
}

static void eg_scissor(struct r600_context *rctx, struct radeon_state *rstate)
{
	const struct pipe_scissor_state *state = &rctx->scissor->state.scissor;
	const struct pipe_framebuffer_state *fb = &rctx->framebuffer->state.framebuffer;
	struct r600_screen *rscreen = rctx->screen;
	unsigned minx, maxx, miny, maxy;
	u32 tl, br;

	if (state == NULL) {
		minx = 0;
		miny = 0;
		maxx = fb->cbufs[0]->width;
		maxy = fb->cbufs[0]->height;
	} else {
		minx = state->minx;
		miny = state->miny;
		maxx = state->maxx;
		maxy = state->maxy;
	}
	tl = S_028240_TL_X(minx) | S_028240_TL_Y(miny);
	br = S_028244_BR_X(maxx) | S_028244_BR_Y(maxy);
	radeon_state_init(rstate, rscreen->rw, R600_STATE_SCISSOR, 0, 0);
	/* screen scissor has no WINDOW OFFSET */
	rstate->states[EG_SCISSOR__PA_SC_SCREEN_SCISSOR_TL] = tl;
	rstate->states[EG_SCISSOR__PA_SC_SCREEN_SCISSOR_BR] = br;
	rstate->states[EG_SCISSOR__PA_SC_WINDOW_OFFSET] = 0x00000000;
	rstate->states[EG_SCISSOR__PA_SC_WINDOW_SCISSOR_TL] = tl | S_028204_WINDOW_OFFSET_DISABLE(1);
	rstate->states[EG_SCISSOR__PA_SC_WINDOW_SCISSOR_BR] = br;
	rstate->states[EG_SCISSOR__PA_SC_CLIPRECT_RULE] = 0x0000FFFF;
	rstate->states[EG_SCISSOR__PA_SC_CLIPRECT_0_TL] = tl;
	rstate->states[EG_SCISSOR__PA_SC_CLIPRECT_0_BR] = br;
	rstate->states[EG_SCISSOR__PA_SC_CLIPRECT_1_TL] = tl;
	rstate->states[EG_SCISSOR__PA_SC_CLIPRECT_1_BR] = br;
	rstate->states[EG_SCISSOR__PA_SC_CLIPRECT_2_TL] = tl;
	rstate->states[EG_SCISSOR__PA_SC_CLIPRECT_2_BR] = br;
	rstate->states[EG_SCISSOR__PA_SC_CLIPRECT_3_TL] = tl;
	rstate->states[EG_SCISSOR__PA_SC_CLIPRECT_3_BR] = br;
	rstate->states[EG_SCISSOR__PA_SC_EDGERULE] = 0xAAAAAAAA;
	rstate->states[EG_SCISSOR__PA_SC_GENERIC_SCISSOR_TL] = tl | S_028240_WINDOW_OFFSET_DISABLE(1);
	rstate->states[EG_SCISSOR__PA_SC_GENERIC_SCISSOR_BR] = br;
	rstate->states[EG_SCISSOR__PA_SC_VPORT_SCISSOR_0_TL] = tl | S_028240_WINDOW_OFFSET_DISABLE(1);
	rstate->states[EG_SCISSOR__PA_SC_VPORT_SCISSOR_0_BR] = br;
	radeon_state_pm4(rstate);
}

static void eg_viewport(struct r600_context *rctx, struct radeon_state *rstate, const struct pipe_viewport_state *state)
{
	struct r600_screen *rscreen = rctx->screen;

	radeon_state_init(rstate, rscreen->rw, R600_STATE_VIEWPORT, 0, 0);
	rstate->states[EG_VIEWPORT__PA_SC_VPORT_ZMIN_0] = 0x00000000;
	rstate->states[EG_VIEWPORT__PA_SC_VPORT_ZMAX_0] = 0x3F800000;
	rstate->states[EG_VIEWPORT__PA_CL_VPORT_XSCALE_0] = fui(state->scale[0]);
	rstate->states[EG_VIEWPORT__PA_CL_VPORT_YSCALE_0] = fui(state->scale[1]);
	rstate->states[EG_VIEWPORT__PA_CL_VPORT_ZSCALE_0] = fui(state->scale[2]);
	rstate->states[EG_VIEWPORT__PA_CL_VPORT_XOFFSET_0] = fui(state->translate[0]);
	rstate->states[EG_VIEWPORT__PA_CL_VPORT_YOFFSET_0] = fui(state->translate[1]);
	rstate->states[EG_VIEWPORT__PA_CL_VPORT_ZOFFSET_0] = fui(state->translate[2]);
	rstate->states[EG_VIEWPORT__PA_CL_VTE_CNTL] = 0x0000043F;
	radeon_state_pm4(rstate);
}

static void eg_dsa(struct r600_context *rctx, struct radeon_state *rstate)
{
	const struct pipe_depth_stencil_alpha_state *state = &rctx->dsa->state.dsa;
	const struct pipe_stencil_ref *stencil_ref = &rctx->stencil_ref->state.stencil_ref;
	struct r600_screen *rscreen = rctx->screen;
	unsigned db_depth_control, alpha_test_control, alpha_ref, db_shader_control;
	unsigned stencil_ref_mask, stencil_ref_mask_bf, db_render_override, db_render_control;
	struct r600_shader *rshader;
	struct r600_query *rquery;
	boolean query_running;
	int i;

	if (rctx->ps_shader == NULL) {
		return;
	}
	radeon_state_init(rstate, rscreen->rw, R600_STATE_DSA, 0, 0);

	db_shader_control = 0x210;
	rshader = &rctx->ps_shader->shader;
	if (rshader->uses_kill)
		db_shader_control |= (1 << 6);
	for (i = 0; i < rshader->noutput; i++) {
		if (rshader->output[i].name == TGSI_SEMANTIC_POSITION)
			db_shader_control |= 1;
	}
	stencil_ref_mask = 0;
	stencil_ref_mask_bf = 0;
	db_depth_control = S_028800_Z_ENABLE(state->depth.enabled) |
		S_028800_Z_WRITE_ENABLE(state->depth.writemask) |
		S_028800_ZFUNC(state->depth.func);
	/* set stencil enable */

	if (state->stencil[0].enabled) {
		db_depth_control |= S_028800_STENCIL_ENABLE(1);
		db_depth_control |= S_028800_STENCILFUNC(r600_translate_ds_func(state->stencil[0].func));
		db_depth_control |= S_028800_STENCILFAIL(r600_translate_stencil_op(state->stencil[0].fail_op));
		db_depth_control |= S_028800_STENCILZPASS(r600_translate_stencil_op(state->stencil[0].zpass_op));
		db_depth_control |= S_028800_STENCILZFAIL(r600_translate_stencil_op(state->stencil[0].zfail_op));

		stencil_ref_mask = S_028430_STENCILMASK(state->stencil[0].valuemask) |
			S_028430_STENCILWRITEMASK(state->stencil[0].writemask);
		stencil_ref_mask |= S_028430_STENCILREF(stencil_ref->ref_value[0]);
		if (state->stencil[1].enabled) {
			db_depth_control |= S_028800_BACKFACE_ENABLE(1);
			db_depth_control |= S_028800_STENCILFUNC_BF(r600_translate_ds_func(state->stencil[1].func));
			db_depth_control |= S_028800_STENCILFAIL_BF(r600_translate_stencil_op(state->stencil[1].fail_op));
			db_depth_control |= S_028800_STENCILZPASS_BF(r600_translate_stencil_op(state->stencil[1].zpass_op));
			db_depth_control |= S_028800_STENCILZFAIL_BF(r600_translate_stencil_op(state->stencil[1].zfail_op));
			stencil_ref_mask_bf = S_028434_STENCILMASK_BF(state->stencil[1].valuemask) |
				S_028434_STENCILWRITEMASK_BF(state->stencil[1].writemask);
			stencil_ref_mask_bf |= S_028430_STENCILREF(stencil_ref->ref_value[1]);
		}
	}

	alpha_test_control = 0;
	alpha_ref = 0;
	if (state->alpha.enabled) {
		alpha_test_control = S_028410_ALPHA_FUNC(state->alpha.func);
		alpha_test_control |= S_028410_ALPHA_TEST_ENABLE(1);
		alpha_ref = fui(state->alpha.ref_value);
	}

	db_render_control = 0;
///	db_render_control = S_028D0C_STENCIL_COMPRESS_DISABLE(1) |
///		S_028D0C_DEPTH_COMPRESS_DISABLE(1);
	db_render_override = S_028D10_FORCE_HIZ_ENABLE(V_028D10_FORCE_DISABLE) |
		S_028D10_FORCE_HIS_ENABLE0(V_028D10_FORCE_DISABLE) |
		S_028D10_FORCE_HIS_ENABLE1(V_028D10_FORCE_DISABLE);

	query_running = false;

	LIST_FOR_EACH_ENTRY(rquery, &rctx->query_list, list) {
		if (rquery->state & R600_QUERY_STATE_STARTED) {
			query_running = true;
		}
	}

	if (query_running) {
		db_render_override |= S_028D10_NOOP_CULL_DISABLE(1);
		db_render_control |= S_028D0C_PERFECT_ZPASS_COUNTS(1);
	}

	rstate->states[EG_DSA__DB_STENCIL_CLEAR] = 0x00000000;
	rstate->states[EG_DSA__DB_DEPTH_CLEAR] = 0x3F800000;
	rstate->states[EG_DSA__SX_ALPHA_TEST_CONTROL] = alpha_test_control;
	rstate->states[EG_DSA__DB_STENCILREFMASK] = stencil_ref_mask;
	rstate->states[EG_DSA__DB_STENCILREFMASK_BF] = stencil_ref_mask_bf;
	rstate->states[EG_DSA__SX_ALPHA_REF] = alpha_ref;
	//	rstate->states[EG_DSA__SPI_FOG_FUNC_SCALE] = 0x00000000;
	//	rstate->states[EG_DSA__SPI_FOG_FUNC_BIAS] = 0x00000000;
	rstate->states[EG_DSA__SPI_FOG_CNTL] = 0x00000000;
	rstate->states[EG_DSA__DB_DEPTH_CONTROL] = db_depth_control;
	rstate->states[EG_DSA__DB_SHADER_CONTROL] = db_shader_control;
	rstate->states[EG_DSA__DB_RENDER_CONTROL] = db_render_control;
	rstate->states[EG_DSA__DB_RENDER_OVERRIDE] = db_render_override;
	  
	rstate->states[EG_DSA__DB_SRESULTS_COMPARE_STATE1] = 0x00000000;
	rstate->states[EG_DSA__DB_PRELOAD_CONTROL] = 0x00000000;
	rstate->states[EG_DSA__DB_ALPHA_TO_MASK] = 0x0000AA00;
	radeon_state_pm4(rstate);
}


static INLINE u32 S_FIXED(float value, u32 frac_bits)
{
	return value * (1 << frac_bits);
}

static void eg_sampler_border(struct r600_context *rctx, struct radeon_state *rstate,
				const struct pipe_sampler_state *state, unsigned id)
{
	struct r600_screen *rscreen = rctx->screen;
	union util_color uc;

	util_pack_color(state->border_color, PIPE_FORMAT_B8G8R8A8_UNORM, &uc);

	radeon_state_init(rstate, rscreen->rw, R600_STATE_SAMPLER_BORDER, id, R600_SHADER_PS);
	if (uc.ui) {
		rstate->states[EG_PS_SAMPLER_BORDER__TD_PS_SAMPLER0_BORDER_RED] = fui(state->border_color[0]);
		rstate->states[EG_PS_SAMPLER_BORDER__TD_PS_SAMPLER0_BORDER_GREEN] = fui(state->border_color[1]);
		rstate->states[EG_PS_SAMPLER_BORDER__TD_PS_SAMPLER0_BORDER_BLUE] = fui(state->border_color[2]);
		rstate->states[EG_PS_SAMPLER_BORDER__TD_PS_SAMPLER0_BORDER_ALPHA] = fui(state->border_color[3]);
	}
	radeon_state_pm4(rstate);
}

static void eg_sampler(struct r600_context *rctx, struct radeon_state *rstate,
			const struct pipe_sampler_state *state, unsigned id)
{
	struct r600_screen *rscreen = rctx->screen;
	union util_color uc;

	util_pack_color(state->border_color, PIPE_FORMAT_B8G8R8A8_UNORM, &uc);

	radeon_state_init(rstate, rscreen->rw, R600_STATE_SAMPLER, id, R600_SHADER_PS);
	rstate->states[EG_PS_SAMPLER__SQ_TEX_SAMPLER_WORD0_0] =
			S_03C000_CLAMP_X(r600_tex_wrap(state->wrap_s)) |
			S_03C000_CLAMP_Y(r600_tex_wrap(state->wrap_t)) |
			S_03C000_CLAMP_Z(r600_tex_wrap(state->wrap_r)) |
			S_03C000_XY_MAG_FILTER(r600_tex_filter(state->mag_img_filter)) |
			S_03C000_XY_MIN_FILTER(r600_tex_filter(state->min_img_filter)) |
			S_03C000_MIP_FILTER(r600_tex_mipfilter(state->min_mip_filter)) |
	                S_03C000_DEPTH_COMPARE_FUNCTION(r600_tex_compare(state->compare_func)) |
	                S_03C000_BORDER_COLOR_TYPE(uc.ui ? V_03C000_SQ_TEX_BORDER_COLOR_REGISTER : 0);
	/* FIXME LOD it depends on texture base level ... */
	rstate->states[EG_PS_SAMPLER__SQ_TEX_SAMPLER_WORD1_0] =
			S_03C004_MIN_LOD(S_FIXED(CLAMP(state->min_lod, 0, 15), 6)) |
		S_03C004_MAX_LOD(S_FIXED(CLAMP(state->max_lod, 0, 15), 6));

	rstate->states[EG_PS_SAMPLER__SQ_TEX_SAMPLER_WORD2_0] = 
		S_03C008_LOD_BIAS(S_FIXED(CLAMP(state->lod_bias, -16, 16), 6)) |
S_03C008_TYPE(1);
	radeon_state_pm4(rstate);

}


static void eg_resource(struct pipe_context *ctx, struct radeon_state *rstate,
			const struct pipe_sampler_view *view, unsigned id)
{
	struct r600_context *rctx = r600_context(ctx);
	struct r600_screen *rscreen = rctx->screen;
	const struct util_format_description *desc;
	struct r600_resource_texture *tmp;
	struct r600_resource *rbuffer;
	unsigned format;
	uint32_t word4 = 0, yuv_format = 0, pitch = 0;
	unsigned char swizzle[4];
	int r;

	rstate->cpm4 = 0;
	swizzle[0] = view->swizzle_r;
	swizzle[1] = view->swizzle_g;
	swizzle[2] = view->swizzle_b;
	swizzle[3] = view->swizzle_a;
	format = r600_translate_texformat(view->texture->format,
					  swizzle,
					  &word4, &yuv_format);
	if (format == ~0) {
		return;
	}
	desc = util_format_description(view->texture->format);
	if (desc == NULL) {
		R600_ERR("unknow format %d\n", view->texture->format);
		return;
	}
	radeon_state_init(rstate, rscreen->rw, R600_STATE_RESOURCE, id, R600_SHADER_PS);
	tmp = (struct r600_resource_texture*)view->texture;
	rbuffer = &tmp->resource;
	if (tmp->depth) {
		r = r600_texture_from_depth(ctx, tmp, view->first_level);
		if (r) {
			return;
		}
		rstate->bo[0] = radeon_bo_incref(rscreen->rw, tmp->uncompressed);
		rstate->bo[1] = radeon_bo_incref(rscreen->rw, tmp->uncompressed);
	} else {
		rstate->bo[0] = radeon_bo_incref(rscreen->rw, rbuffer->bo);
		rstate->bo[1] = radeon_bo_incref(rscreen->rw, rbuffer->bo);
	}
	rstate->nbo = 2;
	rstate->placement[0] = RADEON_GEM_DOMAIN_GTT;
	rstate->placement[1] = RADEON_GEM_DOMAIN_GTT;
	rstate->placement[2] = RADEON_GEM_DOMAIN_GTT;
	rstate->placement[3] = RADEON_GEM_DOMAIN_GTT;

	pitch = (tmp->pitch[0] / tmp->bpt);
	pitch = (pitch + 0x7) & ~0x7;

	/* FIXME properly handle first level != 0 */
	rstate->states[EG_PS_RESOURCE__RESOURCE0_WORD0] =
			S_030000_DIM(r600_tex_dim(view->texture->target)) |
			S_030000_PITCH((pitch / 8) - 1) |
			S_030000_TEX_WIDTH(view->texture->width0 - 1);
	rstate->states[EG_PS_RESOURCE__RESOURCE0_WORD1] =
		S_030004_TEX_HEIGHT(view->texture->height0 - 1) |
		S_030004_TEX_DEPTH(view->texture->depth0 - 1);
	rstate->states[EG_PS_RESOURCE__RESOURCE0_WORD2] = tmp->offset[0] >> 8;
	rstate->states[EG_PS_RESOURCE__RESOURCE0_WORD3] = tmp->offset[1] >> 8;
	rstate->states[EG_PS_RESOURCE__RESOURCE0_WORD4] =
		        word4 | 
			S_030010_NUM_FORMAT_ALL(V_030010_SQ_NUM_FORMAT_NORM) |
			S_030010_SRF_MODE_ALL(V_030010_SFR_MODE_NO_ZERO) |
			S_030010_REQUEST_SIZE(1) |
			S_030010_BASE_LEVEL(view->first_level);
	rstate->states[EG_PS_RESOURCE__RESOURCE0_WORD5] =
			S_030014_LAST_LEVEL(view->last_level) |
			S_030014_BASE_ARRAY(0) |
			S_030014_LAST_ARRAY(0);
	rstate->states[EG_PS_RESOURCE__RESOURCE0_WORD6] = 0;
	rstate->states[EG_PS_RESOURCE__RESOURCE0_WORD7] =
			S_03001C_DATA_FORMAT(format) |
			S_03001C_TYPE(V_03001C_SQ_TEX_VTX_VALID_TEXTURE);
	radeon_state_pm4(rstate);
}

static void eg_cb_cntl(struct r600_context *rctx, struct radeon_state *rstate)
{
	struct r600_screen *rscreen = rctx->screen;
	const struct pipe_blend_state *pbs = &rctx->blend->state.blend;
	int nr_cbufs = rctx->framebuffer->state.framebuffer.nr_cbufs;
	uint32_t color_control, target_mask, shader_mask;
	int i;

	target_mask = 0;
	shader_mask = 0;
	color_control = S_028808_MODE(1);

	for (i = 0; i < nr_cbufs; i++) {
		shader_mask |= 0xf << (i * 4);
	}

	if (pbs->logicop_enable) {
		color_control |= (pbs->logicop_func << 16) | (pbs->logicop_func << 20);
	} else {
		color_control |= (0xcc << 16);
	}

	if (pbs->independent_blend_enable) {
		for (i = 0; i < nr_cbufs; i++) {
			target_mask |= (pbs->rt[i].colormask << (4 * i));
		}
	} else {
		for (i = 0; i < nr_cbufs; i++) {
			target_mask |= (pbs->rt[0].colormask << (4 * i));
		}
	}
	radeon_state_init(rstate, rscreen->rw, R600_STATE_CB_CNTL, 0, 0);
	rstate->states[EG_CB_CNTL__CB_SHADER_MASK] = shader_mask;
	rstate->states[EG_CB_CNTL__CB_TARGET_MASK] = target_mask;
	rstate->states[EG_CB_CNTL__CB_COLOR_CONTROL] = color_control;
	rstate->states[EG_CB_CNTL__PA_SC_AA_CONFIG] = 0x00000000;
	rstate->states[EG_CB_CNTL__PA_SC_AA_SAMPLE_LOCS_MCTX] = 0x00000000;
	rstate->states[EG_CB_CNTL__PA_SC_AA_MASK] = 0xFFFFFFFF;
	radeon_state_pm4(rstate);
}


static void eg_init_config(struct r600_context *rctx)
{
	int ps_prio;
	int vs_prio;
	int gs_prio;
	int es_prio;
	int hs_prio, cs_prio, ls_prio;
	int num_ps_gprs;
	int num_vs_gprs;
	int num_gs_gprs;
	int num_es_gprs;
	int num_hs_gprs;
	int num_ls_gprs;
	int num_temp_gprs;
	int num_ps_threads;
	int num_vs_threads;
	int num_gs_threads;
	int num_es_threads;
	int num_hs_threads;
	int num_ls_threads;
	int num_ps_stack_entries;
	int num_vs_stack_entries;
	int num_gs_stack_entries;
	int num_es_stack_entries;
	int num_hs_stack_entries;
	int num_ls_stack_entries;
	enum radeon_family family;

	family = radeon_get_family(rctx->rw);
	ps_prio = 0;
	vs_prio = 1;
	gs_prio = 2;
	es_prio = 3;
	hs_prio = 0;
	ls_prio = 0;
	cs_prio = 0;

	switch (family) {
	case CHIP_CEDAR:
	default:
		num_ps_gprs = 93;
		num_vs_gprs = 46;
		num_temp_gprs = 4;
		num_gs_gprs = 31;
		num_es_gprs = 31;
		num_hs_gprs = 23;
		num_ls_gprs = 23;
		num_ps_threads = 96;
		num_vs_threads = 16;
		num_gs_threads = 16;
		num_es_threads = 16;
		num_hs_threads = 16;
		num_ls_threads = 16;
		num_ps_stack_entries = 42;
		num_vs_stack_entries = 42;
		num_gs_stack_entries = 42;
		num_es_stack_entries = 42;
		num_hs_stack_entries = 42;
		num_ls_stack_entries = 42;
		break;
	case CHIP_REDWOOD:
		num_ps_gprs = 93;
		num_vs_gprs = 46;
		num_temp_gprs = 4;
		num_gs_gprs = 31;
		num_es_gprs = 31;
		num_hs_gprs = 23;
		num_ls_gprs = 23;
		num_ps_threads = 128;
		num_vs_threads = 20;
		num_gs_threads = 20;
		num_es_threads = 20;
		num_hs_threads = 20;
		num_ls_threads = 20;
		num_ps_stack_entries = 42;
		num_vs_stack_entries = 42;
		num_gs_stack_entries = 42;
		num_es_stack_entries = 42;
		num_hs_stack_entries = 42;
		num_ls_stack_entries = 42;
		break;
	case CHIP_JUNIPER:
		num_ps_gprs = 93;
		num_vs_gprs = 46;
		num_temp_gprs = 4;
		num_gs_gprs = 31;
		num_es_gprs = 31;
		num_hs_gprs = 23;
		num_ls_gprs = 23;
		num_ps_threads = 128;
		num_vs_threads = 20;
		num_gs_threads = 20;
		num_es_threads = 20;
		num_hs_threads = 20;
		num_ls_threads = 20;
		num_ps_stack_entries = 85;
		num_vs_stack_entries = 85;
		num_gs_stack_entries = 85;
		num_es_stack_entries = 85;
		num_hs_stack_entries = 85;
		num_ls_stack_entries = 85;
		break;
	case CHIP_CYPRESS:
	case CHIP_HEMLOCK:
		num_ps_gprs = 93;
		num_vs_gprs = 46;
		num_temp_gprs = 4;
		num_gs_gprs = 31;
		num_es_gprs = 31;
		num_hs_gprs = 23;
		num_ls_gprs = 23;
		num_ps_threads = 128;
		num_vs_threads = 20;
		num_gs_threads = 20;
		num_es_threads = 20;
		num_hs_threads = 20;
		num_ls_threads = 20;
		num_ps_stack_entries = 85;
		num_vs_stack_entries = 85;
		num_gs_stack_entries = 85;
		num_es_stack_entries = 85;
		num_hs_stack_entries = 85;
		num_ls_stack_entries = 85;
		break;
	}

	radeon_state_init(&rctx->config, rctx->rw, R600_STATE_CONFIG, 0, 0);

	rctx->config.states[EG_CONFIG__SQ_CONFIG] = 0x00000000;
	switch (family) {
	case CHIP_CEDAR:
		break;
	default:
		rctx->config.states[EG_CONFIG__SQ_CONFIG] |= S_008C00_VC_ENABLE(1);
		break;
	}
	rctx->config.states[EG_CONFIG__SQ_CONFIG] |= S_008C00_EXPORT_SRC_C(1);
	rctx->config.states[EG_CONFIG__SQ_CONFIG] |= S_008C00_CS_PRIO(cs_prio);
	rctx->config.states[EG_CONFIG__SQ_CONFIG] |= S_008C00_LS_PRIO(ls_prio);
	rctx->config.states[EG_CONFIG__SQ_CONFIG] |= S_008C00_HS_PRIO(hs_prio);
	rctx->config.states[EG_CONFIG__SQ_CONFIG] |= S_008C00_PS_PRIO(ps_prio);
	rctx->config.states[EG_CONFIG__SQ_CONFIG] |= S_008C00_VS_PRIO(vs_prio);
	rctx->config.states[EG_CONFIG__SQ_CONFIG] |= S_008C00_GS_PRIO(gs_prio);
	rctx->config.states[EG_CONFIG__SQ_CONFIG] |= S_008C00_ES_PRIO(es_prio);

	rctx->config.states[EG_CONFIG__SQ_GPR_RESOURCE_MGMT_1] = 0;
	rctx->config.states[EG_CONFIG__SQ_GPR_RESOURCE_MGMT_1] |= S_008C04_NUM_PS_GPRS(num_ps_gprs);
	rctx->config.states[EG_CONFIG__SQ_GPR_RESOURCE_MGMT_1] |= S_008C04_NUM_VS_GPRS(num_vs_gprs);
	rctx->config.states[EG_CONFIG__SQ_GPR_RESOURCE_MGMT_1] |= S_008C04_NUM_CLAUSE_TEMP_GPRS(num_temp_gprs);

	rctx->config.states[EG_CONFIG__SQ_GPR_RESOURCE_MGMT_2] = 0;
	rctx->config.states[EG_CONFIG__SQ_GPR_RESOURCE_MGMT_2] |= S_008C08_NUM_GS_GPRS(num_gs_gprs);
	rctx->config.states[EG_CONFIG__SQ_GPR_RESOURCE_MGMT_2] |= S_008C08_NUM_ES_GPRS(num_es_gprs);

	rctx->config.states[EG_CONFIG__SQ_GPR_RESOURCE_MGMT_3] = 0;
	rctx->config.states[EG_CONFIG__SQ_GPR_RESOURCE_MGMT_3] |= S_008C0C_NUM_HS_GPRS(num_hs_gprs);
	rctx->config.states[EG_CONFIG__SQ_GPR_RESOURCE_MGMT_3] |= S_008C0C_NUM_LS_GPRS(num_ls_gprs);

	rctx->config.states[EG_CONFIG__SQ_THREAD_RESOURCE_MGMT_1] = 0;
	rctx->config.states[EG_CONFIG__SQ_THREAD_RESOURCE_MGMT_1] |= S_008C18_NUM_PS_THREADS(num_ps_threads);
	rctx->config.states[EG_CONFIG__SQ_THREAD_RESOURCE_MGMT_1] |= S_008C18_NUM_VS_THREADS(num_vs_threads);
	rctx->config.states[EG_CONFIG__SQ_THREAD_RESOURCE_MGMT_1] |= S_008C18_NUM_GS_THREADS(num_gs_threads);
	rctx->config.states[EG_CONFIG__SQ_THREAD_RESOURCE_MGMT_1] |= S_008C18_NUM_ES_THREADS(num_es_threads);

	rctx->config.states[EG_CONFIG__SQ_THREAD_RESOURCE_MGMT_2] = 0;
	rctx->config.states[EG_CONFIG__SQ_THREAD_RESOURCE_MGMT_2] |= S_008C1C_NUM_HS_THREADS(num_hs_threads);
	rctx->config.states[EG_CONFIG__SQ_THREAD_RESOURCE_MGMT_2] |= S_008C1C_NUM_LS_THREADS(num_ls_threads);

	rctx->config.states[EG_CONFIG__SQ_STACK_RESOURCE_MGMT_1] = 0;
	rctx->config.states[EG_CONFIG__SQ_STACK_RESOURCE_MGMT_1] |= S_008C20_NUM_PS_STACK_ENTRIES(num_ps_stack_entries);
	rctx->config.states[EG_CONFIG__SQ_STACK_RESOURCE_MGMT_1] |= S_008C20_NUM_VS_STACK_ENTRIES(num_vs_stack_entries);

	rctx->config.states[EG_CONFIG__SQ_STACK_RESOURCE_MGMT_2] = 0;
	rctx->config.states[EG_CONFIG__SQ_STACK_RESOURCE_MGMT_2] |= S_008C24_NUM_GS_STACK_ENTRIES(num_gs_stack_entries);
	rctx->config.states[EG_CONFIG__SQ_STACK_RESOURCE_MGMT_2] |= S_008C24_NUM_ES_STACK_ENTRIES(num_es_stack_entries);

	rctx->config.states[EG_CONFIG__SQ_STACK_RESOURCE_MGMT_3] = 0;
	rctx->config.states[EG_CONFIG__SQ_STACK_RESOURCE_MGMT_3] |= S_008C28_NUM_HS_STACK_ENTRIES(num_hs_stack_entries);
	rctx->config.states[EG_CONFIG__SQ_STACK_RESOURCE_MGMT_3] |= S_008C28_NUM_LS_STACK_ENTRIES(num_ls_stack_entries);

	rctx->config.states[EG_CONFIG__SPI_CONFIG_CNTL] = 0x00000000;
	rctx->config.states[EG_CONFIG__SPI_CONFIG_CNTL_1] = S_00913C_VTX_DONE_DELAY(4);

	rctx->config.states[EG_CONFIG__SX_MISC] = 0x00000000;

	rctx->config.states[EG_CONFIG__SQ_DYN_GPR_CNTL_PS_FLUSH_REQ] = 0x00000000;
	rctx->config.states[EG_CONFIG__PA_SC_MODE_CNTL_0] = 0x0;
	rctx->config.states[EG_CONFIG__PA_SC_MODE_CNTL_1] = 0x0;

	rctx->config.states[EG_CONFIG__SQ_ESGS_RING_ITEMSIZE] = 0x00000000;
	rctx->config.states[EG_CONFIG__SQ_GSVS_RING_ITEMSIZE] = 0x00000000;
	rctx->config.states[EG_CONFIG__SQ_ESTMP_RING_ITEMSIZE] = 0x00000000;
	rctx->config.states[EG_CONFIG__SQ_GSTMP_RING_ITEMSIZE] = 0x00000000;
	rctx->config.states[EG_CONFIG__SQ_VSTMP_RING_ITEMSIZE] = 0x00000000;
	rctx->config.states[EG_CONFIG__SQ_PSTMP_RING_ITEMSIZE] = 0x00000000;

	rctx->config.states[EG_CONFIG__SQ_GS_VERT_ITEMSIZE] = 0x00000000;
	rctx->config.states[EG_CONFIG__SQ_GS_VERT_ITEMSIZE_1] = 0x00000000;
	rctx->config.states[EG_CONFIG__SQ_GS_VERT_ITEMSIZE_2] = 0x00000000;
	rctx->config.states[EG_CONFIG__SQ_GS_VERT_ITEMSIZE_3] = 0x00000000;

	rctx->config.states[EG_CONFIG__VGT_OUTPUT_PATH_CNTL] = 0x00000000;
	rctx->config.states[EG_CONFIG__VGT_HOS_CNTL] = 0x00000000;
	rctx->config.states[EG_CONFIG__VGT_HOS_MAX_TESS_LEVEL] = 0x00000000;
	rctx->config.states[EG_CONFIG__VGT_HOS_MIN_TESS_LEVEL] = 0x00000000;
	rctx->config.states[EG_CONFIG__VGT_HOS_REUSE_DEPTH] = 0x00000000;
	rctx->config.states[EG_CONFIG__VGT_GROUP_PRIM_TYPE] = 0x00000000;
	rctx->config.states[EG_CONFIG__VGT_GROUP_FIRST_DECR] = 0x00000000;
	rctx->config.states[EG_CONFIG__VGT_GROUP_DECR] = 0x00000000;
	rctx->config.states[EG_CONFIG__VGT_GROUP_VECT_0_CNTL] = 0x00000000;
	rctx->config.states[EG_CONFIG__VGT_GROUP_VECT_1_CNTL] = 0x00000000;
	rctx->config.states[EG_CONFIG__VGT_GROUP_VECT_0_FMT_CNTL] = 0x00000000;
	rctx->config.states[EG_CONFIG__VGT_GROUP_VECT_1_FMT_CNTL] = 0x00000000;
	rctx->config.states[EG_CONFIG__VGT_GS_MODE] = 0x00000000;
	rctx->config.states[EG_CONFIG__VGT_STRMOUT_CONFIG] = 0x00000000;
	rctx->config.states[EG_CONFIG__VGT_STRMOUT_BUFFER_CONFIG] = 0x00000000;
	rctx->config.states[EG_CONFIG__VGT_REUSE_OFF] = 0x00000001;
	rctx->config.states[EG_CONFIG__VGT_VTX_CNT_EN] = 0x00000000;
//	rctx->config.states[EG_CONFIG__VGT_CACHE_INVALIDATION] = 0x2;
//	rctx->config.states[EG_CONFIG__VGT_GS_VERTEX_REUSE] = 0x16;
	rctx->config.states[EG_CONFIG__PA_CL_ENHANCE] = (3 << 1) | 1;
	
	radeon_state_pm4(&rctx->config);
}

static int eg_vs_resource(struct r600_context *rctx, int id, struct r600_resource *rbuffer, uint32_t offset,
			    uint32_t stride, uint32_t format)
{
	struct radeon_state *vs_resource = &rctx->vs_resource[id];
	struct r600_screen *rscreen = rctx->screen;

	radeon_state_init(vs_resource, rscreen->rw, R600_STATE_RESOURCE, id, R600_SHADER_VS);
	vs_resource->bo[0] = radeon_bo_incref(rscreen->rw, rbuffer->bo);
	vs_resource->nbo = 1;
	vs_resource->states[EG_PS_RESOURCE__RESOURCE0_WORD0] = offset;
	vs_resource->states[EG_PS_RESOURCE__RESOURCE0_WORD1] = rbuffer->bo->size - offset - 1;
	vs_resource->states[EG_PS_RESOURCE__RESOURCE0_WORD2] = S_030008_STRIDE(stride) |
		S_030008_DATA_FORMAT(format);
	vs_resource->states[EG_PS_RESOURCE__RESOURCE0_WORD3] = S_03000C_DST_SEL_X(V_03000C_SQ_SEL_X) |
		S_03000C_DST_SEL_Y(V_03000C_SQ_SEL_Y) |
		S_03000C_DST_SEL_Z(V_03000C_SQ_SEL_Z) |
		S_03000C_DST_SEL_W(V_03000C_SQ_SEL_W);
	  
	vs_resource->states[EG_PS_RESOURCE__RESOURCE0_WORD4] = 0x00000000;
	vs_resource->states[EG_PS_RESOURCE__RESOURCE0_WORD5] = 0x00000000;
	vs_resource->states[EG_PS_RESOURCE__RESOURCE0_WORD6] = 0x00000000;
	vs_resource->states[EG_PS_RESOURCE__RESOURCE0_WORD7] = 0xC0000000;
	vs_resource->placement[0] = RADEON_GEM_DOMAIN_GTT;
	vs_resource->placement[1] = RADEON_GEM_DOMAIN_GTT;
	return radeon_state_pm4(vs_resource);
}

static int eg_draw_vgt_init(struct r600_context *rctx, struct radeon_state *draw,
			    struct r600_resource *rbuffer,
			    uint32_t count, int vgt_draw_initiator)
{
	struct r600_screen *rscreen = rctx->screen;
	
	radeon_state_init(draw, rscreen->rw, R600_STATE_DRAW, 0, 0);
	draw->states[EG_DRAW__VGT_NUM_INDICES] = count;
	draw->states[EG_DRAW__VGT_DRAW_INITIATOR] = vgt_draw_initiator;
	if (rbuffer) {
		draw->bo[0] = radeon_bo_incref(rscreen->rw, rbuffer->bo);
		draw->placement[0] = RADEON_GEM_DOMAIN_GTT;
		draw->placement[1] = RADEON_GEM_DOMAIN_GTT;
		draw->nbo = 1;
	}
	return radeon_state_pm4(draw);
}

static int eg_draw_vgt_prim(struct r600_context *rctx, struct radeon_state *vgt,
			    uint32_t prim, uint32_t start, uint32_t vgt_dma_index_type)
{
	struct r600_screen *rscreen = rctx->screen;
	radeon_state_init(vgt, rscreen->rw, R600_STATE_VGT, 0, 0);
	vgt->states[EG_VGT__VGT_PRIMITIVE_TYPE] = prim;
	vgt->states[EG_VGT__VGT_MAX_VTX_INDX] = 0x00FFFFFF;
	vgt->states[EG_VGT__VGT_MIN_VTX_INDX] = 0x00000000;
	vgt->states[EG_VGT__VGT_INDX_OFFSET] = start;
	vgt->states[EG_VGT__VGT_DMA_INDEX_TYPE] = vgt_dma_index_type;
	vgt->states[EG_VGT__VGT_PRIMITIVEID_EN] = 0x00000000;
	vgt->states[EG_VGT__VGT_DMA_NUM_INSTANCES] = 0x00000001;
	vgt->states[EG_VGT__VGT_MULTI_PRIM_IB_RESET_EN] = 0x00000000;
	vgt->states[EG_VGT__VGT_INSTANCE_STEP_RATE_0] = 0x00000000;
	vgt->states[EG_VGT__VGT_INSTANCE_STEP_RATE_1] = 0x00000000;
	return radeon_state_pm4(vgt);
}


static int eg_ps_shader(struct r600_context *rctx, struct r600_context_state *rpshader,
			  struct radeon_state *state)
{
	struct r600_screen *rscreen = rctx->screen;
	const struct pipe_rasterizer_state *rasterizer;
	struct r600_shader *rshader = &rpshader->shader;
	unsigned i, tmp, exports_ps, num_cout;
	boolean have_pos = FALSE;

	rasterizer = &rctx->rasterizer->state.rasterizer;

	radeon_state_init(state, rscreen->rw, R600_STATE_SHADER, 0, R600_SHADER_PS);
	for (i = 0; i < rshader->ninput; i++) {
		tmp = S_028644_SEMANTIC(i);
		tmp |= S_028644_SEL_CENTROID(1);
		if (rshader->input[i].name == TGSI_SEMANTIC_POSITION)
			have_pos = TRUE;
		if (rshader->input[i].name == TGSI_SEMANTIC_COLOR ||
		    rshader->input[i].name == TGSI_SEMANTIC_BCOLOR ||
		    rshader->input[i].name == TGSI_SEMANTIC_POSITION) {
			tmp |= S_028644_FLAT_SHADE(rshader->flat_shade);
		}
		if (rasterizer->sprite_coord_enable & (1 << i)) {
			tmp |= S_028644_PT_SPRITE_TEX(1);
		}
		state->states[EG_PS_SHADER__SPI_PS_INPUT_CNTL_0 + i] = tmp;
	}

	exports_ps = 0;
	num_cout = 0;
	for (i = 0; i < rshader->noutput; i++) {
		if (rshader->output[i].name == TGSI_SEMANTIC_POSITION)
			exports_ps |= 1;
		else if (rshader->output[i].name == TGSI_SEMANTIC_COLOR) {
			exports_ps |= (1 << (num_cout+1));
			num_cout++;
		}
	}
	if (!exports_ps) {
		/* always at least export 1 component per pixel */
		exports_ps = 2;
	}
	state->states[EG_PS_SHADER__SPI_PS_IN_CONTROL_0] = S_0286CC_NUM_INTERP(rshader->ninput) |
							S_0286CC_PERSP_GRADIENT_ENA(1);
	if (have_pos) {
		state->states[EG_PS_SHADER__SPI_PS_IN_CONTROL_0] |=  S_0286CC_POSITION_ENA(1);
		state->states[EG_PS_SHADER__SPI_INPUT_Z] |= 1;
	}
	state->states[EG_PS_SHADER__SPI_PS_IN_CONTROL_1] = 0x00000000;
	state->states[EG_PS_SHADER__SQ_PGM_RESOURCES_PS] = S_028844_NUM_GPRS(rshader->bc.ngpr) | S_028844_PRIME_CACHE_ON_DRAW(1) |
		S_028844_STACK_SIZE(rshader->bc.nstack);
	state->states[EG_PS_SHADER__SQ_PGM_EXPORTS_PS] = exports_ps;
	state->states[EG_PS_SHADER__SPI_BARYC_CNTL] = S_0286E0_PERSP_CENTROID_ENA(1) |
	  S_0286E0_LINEAR_CENTROID_ENA(1); 
	state->bo[0] = radeon_bo_incref(rscreen->rw, rpshader->bo);
	state->nbo = 1;
	state->placement[0] = RADEON_GEM_DOMAIN_GTT;
	return radeon_state_pm4(state);
}

static int eg_vs_shader(struct r600_context *rctx, struct r600_context_state *rpshader,
			  struct radeon_state *state)
{
	struct r600_screen *rscreen = rctx->screen;	
	struct r600_shader *rshader = &rpshader->shader;
	unsigned i, tmp;

	radeon_state_init(state, rscreen->rw, R600_STATE_SHADER, 0, R600_SHADER_VS);
	for (i = 0; i < 10; i++) {
		state->states[EG_VS_SHADER__SPI_VS_OUT_ID_0 + i] = 0;
	}
	/* so far never got proper semantic id from tgsi */
	for (i = 0; i < 32; i++) {
		tmp = i << ((i & 3) * 8);
		state->states[EG_VS_SHADER__SPI_VS_OUT_ID_0 + i / 4] |= tmp;
	}
	state->states[EG_VS_SHADER__SPI_VS_OUT_CONFIG] = S_0286C4_VS_EXPORT_COUNT(rshader->noutput - 2);
	state->states[EG_VS_SHADER__SQ_PGM_RESOURCES_VS] = S_028860_NUM_GPRS(rshader->bc.ngpr) |
		S_028860_STACK_SIZE(rshader->bc.nstack);
	state->bo[0] = radeon_bo_incref(rscreen->rw, rpshader->bo);
	state->bo[1] = radeon_bo_incref(rscreen->rw, rpshader->bo);
	state->nbo = 2;
	state->placement[0] = RADEON_GEM_DOMAIN_GTT;
	state->placement[2] = RADEON_GEM_DOMAIN_GTT;
	return radeon_state_pm4(state);
}

struct r600_context_hw_state_vtbl eg_hw_state_vtbl = {
	.blend = eg_blend,
	.ucp = eg_ucp,
	.cb = eg_cb,
	.db = eg_db,
	.rasterizer = eg_rasterizer,
	.scissor = eg_scissor,
	.viewport = eg_viewport,
	.dsa = eg_dsa,
	.sampler_border = eg_sampler_border,
	.sampler = eg_sampler,
	.resource = eg_resource,
	.cb_cntl = eg_cb_cntl,
	.vs_resource = eg_vs_resource,
	.vgt_init = eg_draw_vgt_init,
	.vgt_prim = eg_draw_vgt_prim,
	.vs_shader = eg_vs_shader,
	.ps_shader = eg_ps_shader,
	.init_config = eg_init_config,
};

void eg_set_constant_buffer(struct pipe_context *ctx,
			    uint shader, uint index,
			    struct pipe_resource *buffer)
{
	struct r600_screen *rscreen = r600_screen(ctx->screen);
	struct r600_context *rctx = r600_context(ctx);
	unsigned nconstant = 0, type, shader_class, size;
	struct radeon_state *rstate, *rstates;
	struct r600_resource *rbuffer = (struct r600_resource*)buffer;

	type = R600_STATE_CBUF;

	switch (shader) {
	case PIPE_SHADER_VERTEX:
		shader_class = R600_SHADER_VS;
		rstates = rctx->vs_constant;
		break;
	case PIPE_SHADER_FRAGMENT:
		shader_class = R600_SHADER_PS;
		rstates = rctx->ps_constant;
		break;
	default:
		R600_ERR("unsupported %d\n", shader);
		return;
	}

	rstate = &rstates[0];

#define ALIGN_DIVUP(x, y) (((x) + (y) - 1) / (y))
	nconstant = buffer->width0 / 16;
	size = ALIGN_DIVUP(nconstant, 16);

	radeon_state_init(rstate, rscreen->rw, type, 0, shader_class);
	rstate->states[EG_VS_CBUF__ALU_CONST_BUFFER_SIZE_VS_0] = size;
	rstate->states[EG_VS_CBUF__ALU_CONST_CACHE_VS_0] = 0;

	rstate->bo[0] = radeon_bo_incref(rscreen->rw, rbuffer->bo);
	rstate->nbo = 1;
	rstate->placement[0] = RADEON_GEM_DOMAIN_VRAM;
	if (radeon_state_pm4(rstate))
		return;
	radeon_draw_bind(&rctx->draw, rstate);
}

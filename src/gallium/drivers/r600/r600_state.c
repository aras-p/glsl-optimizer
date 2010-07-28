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
 *
 * Authors:
 *      Jerome Glisse
 */
#include <stdio.h>
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_memory.h"
#include "r600_screen.h"
#include "r600_context.h"
#include "r600_resource.h"
#include "r600d.h"


static void r600_delete_state(struct pipe_context *ctx, void *state)
{
	struct radeon_state *rstate = state;

	radeon_state_decref(rstate);
}

static void *r600_create_blend_state(struct pipe_context *ctx,
					const struct pipe_blend_state *state)
{
	struct r600_screen *rscreen = r600_screen(ctx->screen);
	struct radeon_state *rstate;

	rstate = radeon_state(rscreen->rw, R600_BLEND_TYPE, R600_BLEND);
	if (rstate == NULL)
		return NULL;
	rstate->states[R600_BLEND__CB_BLEND_RED] = 0x00000000;
	rstate->states[R600_BLEND__CB_BLEND_GREEN] = 0x00000000;
	rstate->states[R600_BLEND__CB_BLEND_BLUE] = 0x00000000;
	rstate->states[R600_BLEND__CB_BLEND_ALPHA] = 0x00000000;
	rstate->states[R600_BLEND__CB_BLEND0_CONTROL] = 0x00010001;
	rstate->states[R600_BLEND__CB_BLEND1_CONTROL] = 0x00000000;
	rstate->states[R600_BLEND__CB_BLEND2_CONTROL] = 0x00000000;
	rstate->states[R600_BLEND__CB_BLEND3_CONTROL] = 0x00000000;
	rstate->states[R600_BLEND__CB_BLEND4_CONTROL] = 0x00000000;
	rstate->states[R600_BLEND__CB_BLEND5_CONTROL] = 0x00000000;
	rstate->states[R600_BLEND__CB_BLEND6_CONTROL] = 0x00000000;
	rstate->states[R600_BLEND__CB_BLEND7_CONTROL] = 0x00000000;
	rstate->states[R600_BLEND__CB_BLEND_CONTROL] = 0x00000000;
	if (radeon_state_pm4(rstate)) {
		radeon_state_decref(rstate);
		return NULL;
	}
	return rstate;
}

static void r600_bind_blend_state(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = r600_context(ctx);
	radeon_draw_set(rctx->draw, state);
}

static void r600_set_blend_color(struct pipe_context *ctx,
					const struct pipe_blend_color *color)
{
}

static void r600_set_clip_state(struct pipe_context *ctx,
				const struct pipe_clip_state *state)
{
}

static void r600_set_framebuffer_state(struct pipe_context *ctx,
					const struct pipe_framebuffer_state *state)
{
	struct r600_screen *rscreen = r600_screen(ctx->screen);
	struct r600_context *rctx = r600_context(ctx);
	struct r600_resource_texture *rtex;
	struct r600_resource *rbuffer;
	struct radeon_state *rstate;
	unsigned level = state->cbufs[0]->level;
	unsigned pitch, slice;

	rstate = radeon_state(rscreen->rw, R600_CB0_TYPE, R600_CB0);
	if (rstate == NULL)
		return;
	rtex = (struct r600_resource_texture*)state->cbufs[0]->texture;
	rbuffer = &rtex->resource;
	rstate->bo[0] = radeon_bo_incref(rscreen->rw, rbuffer->bo);
	rstate->bo[1] = radeon_bo_incref(rscreen->rw, rbuffer->bo);
	rstate->bo[2] = radeon_bo_incref(rscreen->rw, rbuffer->bo);
	rstate->placement[0] = RADEON_GEM_DOMAIN_GTT;
	rstate->placement[2] = RADEON_GEM_DOMAIN_GTT;
	rstate->placement[4] = RADEON_GEM_DOMAIN_GTT;
	rstate->nbo = 3;
	pitch = rtex->pitch[level] / 8 - 1;
	slice = rtex->pitch[level] * state->cbufs[0]->height / 64 - 1;
	rstate->states[R600_CB0__CB_COLOR0_BASE] = 0x00000000;
	rstate->states[R600_CB0__CB_COLOR0_INFO] = 0x08110068;
	rstate->states[R600_CB0__CB_COLOR0_SIZE] = S_028060_PITCH_TILE_MAX(pitch) |
						S_028060_SLICE_TILE_MAX(slice);
	rstate->states[R600_CB0__CB_COLOR0_VIEW] = 0x00000000;
	rstate->states[R600_CB0__CB_COLOR0_FRAG] = 0x00000000;
	rstate->states[R600_CB0__CB_COLOR0_TILE] = 0x00000000;
	rstate->states[R600_CB0__CB_COLOR0_MASK] = 0x00000000;
	if (radeon_state_pm4(rstate)) {
		radeon_state_decref(rstate);
		return;
	}
	radeon_draw_set_new(rctx->draw, rstate);
	rctx->db = radeon_state_decref(rctx->db);
	if(state->zsbuf) {
		rtex = (struct r600_resource_texture*)state->zsbuf->texture;
		rbuffer = &rtex->resource;
		rctx->db = radeon_state(rscreen->rw, R600_DB_TYPE, R600_DB);
		if(rctx->db == NULL)
		     return;
		rctx->db->bo[0] = radeon_bo_incref(rscreen->rw, rbuffer->bo);
		rctx->db->nbo = 1;
		rctx->db->placement[0] = RADEON_GEM_DOMAIN_VRAM;
		level = state->zsbuf->level;
		pitch = rtex->pitch[level] / 8 - 1;
		slice = rtex->pitch[level] * state->zsbuf->height / 64 - 1;

		rctx->db->states[R600_DB__DB_DEPTH_BASE] = 0x00000000;
		rctx->db->states[R600_DB__DB_DEPTH_INFO] = 0x00010006;
		rctx->db->states[R600_DB__DB_DEPTH_VIEW] = 0x00000000;
		rctx->db->states[R600_DB__DB_PREFETCH_LIMIT] = (state->zsbuf->height / 8) -1;
		rctx->db->states[R600_DB__DB_DEPTH_SIZE] = S_028000_PITCH_TILE_MAX(pitch) |
						S_028000_SLICE_TILE_MAX(slice);
	} else 
		rctx->db = NULL;
	rctx->fb_state = *state;
}

static void *r600_create_fs_state(struct pipe_context *ctx,
					const struct pipe_shader_state *shader)
{
	return r600_pipe_shader_create(ctx, shader->tokens);
}

static void r600_bind_fs_state(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = r600_context(ctx);

	rctx->ps_shader = state;
}

static void *r600_create_vs_state(struct pipe_context *ctx,
					const struct pipe_shader_state *shader)
{
	return r600_pipe_shader_create(ctx, shader->tokens);
}

static void r600_bind_vs_state(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = r600_context(ctx);

	rctx->vs_shader = state;
}

static void r600_set_polygon_stipple(struct pipe_context *ctx,
					 const struct pipe_poly_stipple *state)
{
}

static void *r600_create_rs_state(struct pipe_context *ctx,
					const struct pipe_rasterizer_state *state)
{
	struct r600_screen *rscreen = r600_screen(ctx->screen);
	struct r600_context *rctx = r600_context(ctx);
	struct radeon_state *rstate;

	rctx->flat_shade = state->flatshade;
	rctx->flat_shade = 0;
R600_ERR("flat shade with texture broke tex coord interp\n");
	rstate = radeon_state(rscreen->rw, R600_RASTERIZER_TYPE, R600_RASTERIZER);
	if (rstate == NULL)
		return NULL;
	rstate->states[R600_RASTERIZER__SPI_INTERP_CONTROL_0] = 0x00000001;
	rstate->states[R600_RASTERIZER__PA_CL_CLIP_CNTL] = 0x00000000;
	rstate->states[R600_RASTERIZER__PA_SU_SC_MODE_CNTL] = 0x00080000;
	rstate->states[R600_RASTERIZER__PA_CL_VS_OUT_CNTL] = 0x00000000;
	rstate->states[R600_RASTERIZER__PA_CL_NANINF_CNTL] = 0x00000000;
	rstate->states[R600_RASTERIZER__PA_SU_POINT_SIZE] = 0x00080008;
	rstate->states[R600_RASTERIZER__PA_SU_POINT_MINMAX] = 0x00000000;
	rstate->states[R600_RASTERIZER__PA_SU_LINE_CNTL] = 0x00000008;
	rstate->states[R600_RASTERIZER__PA_SC_LINE_STIPPLE] = 0x00000005;
	rstate->states[R600_RASTERIZER__PA_SC_MPASS_PS_CNTL] = 0x00000000;
	rstate->states[R600_RASTERIZER__PA_SC_LINE_CNTL] = 0x00000400;
	rstate->states[R600_RASTERIZER__PA_CL_GB_VERT_CLIP_ADJ] = 0x3F800000;
	rstate->states[R600_RASTERIZER__PA_CL_GB_VERT_DISC_ADJ] = 0x3F800000;
	rstate->states[R600_RASTERIZER__PA_CL_GB_HORZ_CLIP_ADJ] = 0x3F800000;
	rstate->states[R600_RASTERIZER__PA_CL_GB_HORZ_DISC_ADJ] = 0x3F800000;
	rstate->states[R600_RASTERIZER__PA_SU_POLY_OFFSET_DB_FMT_CNTL] = 0x00000000;
	rstate->states[R600_RASTERIZER__PA_SU_POLY_OFFSET_CLAMP] = 0x00000000;
	rstate->states[R600_RASTERIZER__PA_SU_POLY_OFFSET_FRONT_SCALE] = 0x00000000;
	rstate->states[R600_RASTERIZER__PA_SU_POLY_OFFSET_FRONT_OFFSET] = 0x00000000;
	rstate->states[R600_RASTERIZER__PA_SU_POLY_OFFSET_BACK_SCALE] = 0x00000000;
	rstate->states[R600_RASTERIZER__PA_SU_POLY_OFFSET_BACK_OFFSET] = 0x00000000;
	if (radeon_state_pm4(rstate)) {
		radeon_state_decref(rstate);
		return NULL;
	}
	return rstate;
}

static void r600_bind_rs_state(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = r600_context(ctx);
	radeon_draw_set(rctx->draw, state);
}

static inline unsigned r600_tex_wrap(unsigned wrap)
{
	switch (wrap) {
	default:
	case PIPE_TEX_WRAP_REPEAT:
		return V_03C000_SQ_TEX_WRAP;
	case PIPE_TEX_WRAP_CLAMP:
		return V_03C000_SQ_TEX_CLAMP_LAST_TEXEL;
	case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
		return V_03C000_SQ_TEX_CLAMP_HALF_BORDER;
	case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
		return V_03C000_SQ_TEX_CLAMP_BORDER;
	case PIPE_TEX_WRAP_MIRROR_REPEAT:
		return V_03C000_SQ_TEX_MIRROR;
	case PIPE_TEX_WRAP_MIRROR_CLAMP:
		return V_03C000_SQ_TEX_MIRROR_ONCE_LAST_TEXEL;
	case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
		return V_03C000_SQ_TEX_MIRROR_ONCE_HALF_BORDER;
	case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
		return V_03C000_SQ_TEX_MIRROR_ONCE_BORDER;
	}
}

static inline unsigned r600_tex_filter(unsigned filter)
{
	switch (filter) {
	default:
	case PIPE_TEX_FILTER_NEAREST:
		return V_03C000_SQ_TEX_XY_FILTER_POINT;
	case PIPE_TEX_FILTER_LINEAR:
		return V_03C000_SQ_TEX_XY_FILTER_BILINEAR;
	}
}

static inline unsigned r600_tex_mipfilter(unsigned filter)
{
	switch (filter) {
	case PIPE_TEX_MIPFILTER_NEAREST:
		return V_03C000_SQ_TEX_Z_FILTER_POINT;
	case PIPE_TEX_MIPFILTER_LINEAR:
		return V_03C000_SQ_TEX_Z_FILTER_LINEAR;
	default:
	case PIPE_TEX_MIPFILTER_NONE:
		return V_03C000_SQ_TEX_Z_FILTER_NONE;
	}
}

static inline unsigned r600_tex_compare(unsigned compare)
{
	switch (compare) {
	default:
	case PIPE_FUNC_NEVER:
		return V_03C000_SQ_TEX_DEPTH_COMPARE_NEVER;
	case PIPE_FUNC_LESS:
		return V_03C000_SQ_TEX_DEPTH_COMPARE_LESS;
	case PIPE_FUNC_EQUAL:
		return V_03C000_SQ_TEX_DEPTH_COMPARE_EQUAL;
	case PIPE_FUNC_LEQUAL:
		return V_03C000_SQ_TEX_DEPTH_COMPARE_LESSEQUAL;
	case PIPE_FUNC_GREATER:
		return V_03C000_SQ_TEX_DEPTH_COMPARE_GREATER;
	case PIPE_FUNC_NOTEQUAL:
		return V_03C000_SQ_TEX_DEPTH_COMPARE_NOTEQUAL;
	case PIPE_FUNC_GEQUAL:
		return V_03C000_SQ_TEX_DEPTH_COMPARE_GREATEREQUAL;
	case PIPE_FUNC_ALWAYS:
		return V_03C000_SQ_TEX_DEPTH_COMPARE_ALWAYS;
	}
}

static void *r600_create_sampler_state(struct pipe_context *ctx,
					const struct pipe_sampler_state *state)
{
	struct r600_screen *rscreen = r600_screen(ctx->screen);
	struct radeon_state *rstate;

	rstate = radeon_state(rscreen->rw, R600_PS_SAMPLER_TYPE, R600_PS_SAMPLER);
	if (rstate == NULL)
		return NULL;
	rstate->states[R600_PS_SAMPLER__SQ_TEX_SAMPLER_WORD0_0] =
			S_03C000_CLAMP_X(r600_tex_wrap(state->wrap_s)) |
			S_03C000_CLAMP_Y(r600_tex_wrap(state->wrap_t)) |
			S_03C000_CLAMP_Z(r600_tex_wrap(state->wrap_r)) |
			S_03C000_XY_MAG_FILTER(r600_tex_filter(state->mag_img_filter)) |
			S_03C000_XY_MIN_FILTER(r600_tex_filter(state->min_img_filter)) |
			S_03C000_MIP_FILTER(r600_tex_mipfilter(state->min_mip_filter)) |
			S_03C000_DEPTH_COMPARE_FUNCTION(r600_tex_compare(state->compare_func));
	/* FIXME LOD it depends on texture base level ... */
	rstate->states[R600_PS_SAMPLER__SQ_TEX_SAMPLER_WORD1_0] =
			S_03C004_MIN_LOD(0) |
			S_03C004_MAX_LOD(0) |
			S_03C004_LOD_BIAS(0);
	rstate->states[R600_PS_SAMPLER__SQ_TEX_SAMPLER_WORD2_0] = S_03C008_TYPE(1);
	if (radeon_state_pm4(rstate)) {
		radeon_state_decref(rstate);
		return NULL;
	}
	return rstate;
}

static void r600_bind_sampler_states(struct pipe_context *ctx,
					unsigned count, void **states)
{
	struct r600_context *rctx = r600_context(ctx);
	unsigned i;

	/* FIXME split VS/PS/GS sampler */
	for (i = 0; i < count; i++) {
		rctx->ps_sampler[i] = radeon_state_decref(rctx->ps_sampler[i]);
	}
	rctx->nps_sampler = count;
	for (i = 0; i < count; i++) {
		rctx->ps_sampler[i] = radeon_state_incref(states[i]);
		rctx->ps_sampler[i]->id = R600_PS_SAMPLER + i;
	}
}

static inline unsigned r600_tex_swizzle(unsigned swizzle)
{
	switch (swizzle) {
	case PIPE_SWIZZLE_RED:
		return V_038010_SQ_SEL_X;
	case PIPE_SWIZZLE_GREEN:
		return V_038010_SQ_SEL_Y;
	case PIPE_SWIZZLE_BLUE:
		return V_038010_SQ_SEL_Z;
	case PIPE_SWIZZLE_ALPHA:
		return V_038010_SQ_SEL_W;
	case PIPE_SWIZZLE_ZERO:
		return V_038010_SQ_SEL_0;
	default:
	case PIPE_SWIZZLE_ONE:
		return V_038010_SQ_SEL_1;
	}
}

static inline unsigned r600_format_type(unsigned format_type)
{
	switch (format_type) {
	default:
	case UTIL_FORMAT_TYPE_UNSIGNED:
		return V_038010_SQ_FORMAT_COMP_UNSIGNED;
	case UTIL_FORMAT_TYPE_SIGNED:
		return V_038010_SQ_FORMAT_COMP_SIGNED;
	case UTIL_FORMAT_TYPE_FIXED:
		return V_038010_SQ_FORMAT_COMP_UNSIGNED_BIASED;
	}
}

static inline unsigned r600_tex_dim(unsigned dim)
{
	switch (dim) {
	default:
	case PIPE_TEXTURE_1D:
		return V_038000_SQ_TEX_DIM_1D;
	case PIPE_TEXTURE_2D:
		return V_038000_SQ_TEX_DIM_2D;
	case PIPE_TEXTURE_3D:
		return V_038000_SQ_TEX_DIM_3D;
	case PIPE_TEXTURE_CUBE:
		return V_038000_SQ_TEX_DIM_CUBEMAP;
	}
}

static struct pipe_sampler_view *r600_create_sampler_view(struct pipe_context *ctx,
							  struct pipe_resource *texture,
							  const struct pipe_sampler_view *view)
{
	struct r600_screen *rscreen = r600_screen(ctx->screen);
	struct r600_texture_resource *rtexture;
	const struct util_format_description *desc;
	struct r600_resource_texture *tmp;
	struct r600_resource *rbuffer;
	unsigned format;

	if (r600_conv_pipe_format(texture->format, &format))
		return NULL;
	rtexture = CALLOC_STRUCT(r600_texture_resource);
	if (rtexture == NULL)
		return NULL;
	desc = util_format_description(texture->format);
	assert(desc == NULL);
	rtexture->state = radeon_state(rscreen->rw, R600_PS_RESOURCE_TYPE, R600_PS_RESOURCE);
	if (rtexture->state == NULL) {
		FREE(rtexture);
		return NULL;
	}
	rtexture->view = *view;
	rtexture->view.reference.count = 1;
	rtexture->view.texture = NULL;
	pipe_resource_reference(&rtexture->view.texture, texture);
	rtexture->view.context = ctx;

	tmp = (struct r600_resource_texture*)texture;
	rbuffer = &tmp->resource;
	rtexture->state->bo[0] = radeon_bo_incref(rscreen->rw, rbuffer->bo);
	rtexture->state->bo[1] = radeon_bo_incref(rscreen->rw, rbuffer->bo);
	rtexture->state->nbo = 2;
	rtexture->state->placement[0] = RADEON_GEM_DOMAIN_GTT;
	rtexture->state->placement[1] = RADEON_GEM_DOMAIN_GTT;
	rtexture->state->placement[2] = RADEON_GEM_DOMAIN_GTT;
	rtexture->state->placement[3] = RADEON_GEM_DOMAIN_GTT;

	/* FIXME properly handle first level != 0 */
	rtexture->state->states[R600_PS_RESOURCE__RESOURCE0_WORD0] =
			S_038000_DIM(r600_tex_dim(texture->target)) |
			S_038000_PITCH((tmp->pitch[0] / 8) - 1) |
			S_038000_TEX_WIDTH(texture->width0 - 1);
	rtexture->state->states[R600_PS_RESOURCE__RESOURCE0_WORD1] =
			S_038004_TEX_HEIGHT(texture->height0 - 1) |
			S_038004_TEX_DEPTH(texture->depth0 - 1) |
			S_038004_DATA_FORMAT(format);
	rtexture->state->states[R600_PS_RESOURCE__RESOURCE0_WORD2] = 0;
	rtexture->state->states[R600_PS_RESOURCE__RESOURCE0_WORD3] = tmp->offset[1] >> 8;
	rtexture->state->states[R600_PS_RESOURCE__RESOURCE0_WORD4] =
			S_038010_FORMAT_COMP_X(r600_format_type(UTIL_FORMAT_TYPE_UNSIGNED)) |
			S_038010_FORMAT_COMP_Y(r600_format_type(UTIL_FORMAT_TYPE_UNSIGNED)) |
			S_038010_FORMAT_COMP_Z(r600_format_type(UTIL_FORMAT_TYPE_UNSIGNED)) |
			S_038010_FORMAT_COMP_W(r600_format_type(UTIL_FORMAT_TYPE_UNSIGNED)) |
			S_038010_NUM_FORMAT_ALL(V_038010_SQ_NUM_FORMAT_NORM) |
			S_038010_SRF_MODE_ALL(V_038010_SFR_MODE_NO_ZERO) |
			S_038010_REQUEST_SIZE(1) |
			S_038010_DST_SEL_X(r600_tex_swizzle(view->swizzle_r)) |
			S_038010_DST_SEL_Y(r600_tex_swizzle(view->swizzle_g)) |
			S_038010_DST_SEL_Z(r600_tex_swizzle(view->swizzle_b)) |
			S_038010_DST_SEL_W(r600_tex_swizzle(view->swizzle_a)) |
			S_038010_BASE_LEVEL(view->first_level);
	rtexture->state->states[R600_PS_RESOURCE__RESOURCE0_WORD5] =
			S_038014_LAST_LEVEL(view->last_level) |
			S_038014_BASE_ARRAY(0) |
			S_038014_LAST_ARRAY(0);
	rtexture->state->states[R600_PS_RESOURCE__RESOURCE0_WORD6] =
			S_038018_TYPE(V_038010_SQ_TEX_VTX_VALID_TEXTURE);
	return &rtexture->view;
}

static void r600_sampler_view_destroy(struct pipe_context *ctx,
				      struct pipe_sampler_view *view)
{
	struct r600_texture_resource *texture;

	if (view == NULL)
		return;
	texture  = LIST_ENTRY(struct r600_texture_resource, view, view);
	radeon_state_decref(texture->state);
	FREE(texture);
}

static void r600_set_fragment_sampler_views(struct pipe_context *ctx,
					    unsigned count,
					    struct pipe_sampler_view **views)
{
	struct r600_texture_resource *rtexture;
	struct r600_context *rctx = r600_context(ctx);
	struct pipe_sampler_view *tmp;
	unsigned i, real_num_views = 0;

	if (views == NULL)
		return;

	for (i = 0; i < count; i++) {
		if (views[i])
			real_num_views++;
	}

	for (i = 0; i < rctx->nps_view; i++) {
		tmp = &rctx->ps_view[i]->view;
		pipe_sampler_view_reference(&tmp, NULL);
		rctx->ps_view[i] = NULL;
	}
	rctx->nps_view = real_num_views;
	for (i = 0; i < count; i++) {

		if (!views[i])
			continue;
		rtexture = LIST_ENTRY(struct r600_texture_resource, views[i], view);
		rctx->ps_view[i] = rtexture;
		tmp = NULL;
		pipe_sampler_view_reference(&tmp, views[i]);
		rtexture->state->id = R600_PS_RESOURCE + i;
	}
}

static void r600_set_vertex_sampler_views(struct pipe_context *ctx,
					  unsigned count,
					  struct pipe_sampler_view **views)
{
	struct r600_texture_resource *rtexture;
	struct r600_context *rctx = r600_context(ctx);
	struct pipe_sampler_view *tmp;
	unsigned i, real_num_views = 0;

	if (views == NULL)
		return;

	for (i = 0; i < count; i++) {
		if (views[i])
			real_num_views++;
	}
	for (i = 0; i < rctx->nvs_view; i++) {
		tmp = &rctx->vs_view[i]->view;
		pipe_sampler_view_reference(&tmp, NULL);
		rctx->vs_view[i] = NULL;
	}
	rctx->nvs_view = real_num_views;
	for (i = 0; i < count; i++) {
		if (!views[i])
			continue;
		rtexture = LIST_ENTRY(struct r600_texture_resource, views[i], view);
		rctx->vs_view[i] = rtexture;
		tmp = NULL;
		pipe_sampler_view_reference(&tmp, views[i]);
		rtexture->state->id = R600_VS_RESOURCE + i;
	}
}

static void r600_set_scissor_state(struct pipe_context *ctx,
					const struct pipe_scissor_state *state)
{
	struct r600_screen *rscreen = r600_screen(ctx->screen);
	struct r600_context *rctx = r600_context(ctx);
	struct radeon_state *rstate;
	u32 tl, br;

	tl = S_028240_TL_X(state->minx) | S_028240_TL_Y(state->miny) | S_028240_WINDOW_OFFSET_DISABLE(1);
	br = S_028244_BR_X(state->maxx) | S_028244_BR_Y(state->maxy);
	rstate = radeon_state(rscreen->rw, R600_SCISSOR_TYPE, R600_SCISSOR);
	if (rstate == NULL)
		return;
	rstate->states[R600_SCISSOR__PA_SC_SCREEN_SCISSOR_TL] = tl;
	rstate->states[R600_SCISSOR__PA_SC_SCREEN_SCISSOR_BR] = br;
	rstate->states[R600_SCISSOR__PA_SC_WINDOW_OFFSET] = 0x00000000;
	rstate->states[R600_SCISSOR__PA_SC_WINDOW_SCISSOR_TL] = tl;
	rstate->states[R600_SCISSOR__PA_SC_WINDOW_SCISSOR_BR] = br;
	rstate->states[R600_SCISSOR__PA_SC_CLIPRECT_RULE] = 0x0000FFFF;
	rstate->states[R600_SCISSOR__PA_SC_CLIPRECT_0_TL] = tl;
	rstate->states[R600_SCISSOR__PA_SC_CLIPRECT_0_BR] = br;
	rstate->states[R600_SCISSOR__PA_SC_CLIPRECT_1_TL] = tl;
	rstate->states[R600_SCISSOR__PA_SC_CLIPRECT_1_BR] = br;
	rstate->states[R600_SCISSOR__PA_SC_CLIPRECT_2_TL] = tl;
	rstate->states[R600_SCISSOR__PA_SC_CLIPRECT_2_BR] = br;
	rstate->states[R600_SCISSOR__PA_SC_CLIPRECT_3_TL] = tl;
	rstate->states[R600_SCISSOR__PA_SC_CLIPRECT_3_BR] = br;
	rstate->states[R600_SCISSOR__PA_SC_EDGERULE] = 0xAAAAAAAA;
	rstate->states[R600_SCISSOR__PA_SC_GENERIC_SCISSOR_TL] = tl;
	rstate->states[R600_SCISSOR__PA_SC_GENERIC_SCISSOR_BR] = br;
	rstate->states[R600_SCISSOR__PA_SC_VPORT_SCISSOR_0_TL] = tl;
	rstate->states[R600_SCISSOR__PA_SC_VPORT_SCISSOR_0_BR] = br;
	if (radeon_state_pm4(rstate)) {
		radeon_state_decref(rstate);
		return;
	}
	radeon_draw_set_new(rctx->draw, rstate);
}

static void r600_set_viewport_state(struct pipe_context *ctx,
					const struct pipe_viewport_state *state)
{
	struct r600_screen *rscreen = r600_screen(ctx->screen);
	struct r600_context *rctx = r600_context(ctx);
	struct radeon_state *rstate;

	rstate = radeon_state(rscreen->rw, R600_VIEWPORT_TYPE, R600_VIEWPORT);
	if (rstate == NULL)
		return;
	rstate->states[R600_VIEWPORT__PA_SC_VPORT_ZMIN_0] = 0x00000000;
	rstate->states[R600_VIEWPORT__PA_SC_VPORT_ZMAX_0] = 0x3F800000;
	rstate->states[R600_VIEWPORT__PA_CL_VPORT_XSCALE_0] = fui(state->scale[0]);
	rstate->states[R600_VIEWPORT__PA_CL_VPORT_YSCALE_0] = fui(state->scale[1]);
	rstate->states[R600_VIEWPORT__PA_CL_VPORT_ZSCALE_0] = fui(state->scale[2]);
	rstate->states[R600_VIEWPORT__PA_CL_VPORT_XOFFSET_0] = fui(state->translate[0]);
	rstate->states[R600_VIEWPORT__PA_CL_VPORT_YOFFSET_0] = fui(state->translate[1]);
	rstate->states[R600_VIEWPORT__PA_CL_VPORT_ZOFFSET_0] = fui(state->translate[2]);
	rstate->states[R600_VIEWPORT__PA_CL_VTE_CNTL] = 0x0000043F;
	if (radeon_state_pm4(rstate)) {
		radeon_state_decref(rstate);
		return;
	}
	radeon_draw_set_new(rctx->draw, rstate);
	rctx->viewport = *state;
}

static void r600_set_vertex_buffers(struct pipe_context *ctx,
					unsigned count,
					const struct pipe_vertex_buffer *buffers)
{
	struct r600_context *rctx = r600_context(ctx);

	memcpy(rctx->vertex_buffer, buffers, sizeof(struct pipe_vertex_buffer) * count);
	rctx->nvertex_buffer = count;
}


static void *r600_create_vertex_elements_state(struct pipe_context *ctx,
					       unsigned count,
					       const struct pipe_vertex_element *elements)
{
	struct r600_vertex_elements_state *v = CALLOC_STRUCT(r600_vertex_elements_state);

	assert(count < 32);
	v->count = count;
	memcpy(v->elements, elements, count * sizeof(struct pipe_vertex_element));
	return v;
}

static void r600_bind_vertex_elements_state(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = r600_context(ctx);
	struct r600_vertex_elements_state *v = (struct r600_vertex_elements_state*)state;

	rctx->vertex_elements = v;
}

static void r600_delete_vertex_elements_state(struct pipe_context *ctx, void *state)
{
	FREE(state);
}

static void *r600_create_dsa_state(struct pipe_context *ctx,
					const struct pipe_depth_stencil_alpha_state *state)
{
	struct r600_screen *rscreen = r600_screen(ctx->screen);
	struct radeon_state *rstate;
	unsigned db_depth_control;

	rstate = radeon_state(rscreen->rw, R600_DSA_TYPE, R600_DSA);
	if (rstate == NULL)
		return NULL;
	db_depth_control = 0x00700700 | S_028800_Z_ENABLE(state->depth.enabled) | S_028800_Z_WRITE_ENABLE(state->depth.writemask) | S_028800_ZFUNC(state->depth.func);
	
	rstate->states[R600_DSA__DB_STENCIL_CLEAR] = 0x00000000;
	rstate->states[R600_DSA__DB_DEPTH_CLEAR] = 0x3F800000;
	rstate->states[R600_DSA__SX_ALPHA_TEST_CONTROL] = 0x00000000;
	rstate->states[R600_DSA__DB_STENCILREFMASK] = 0xFFFFFF00;
	rstate->states[R600_DSA__DB_STENCILREFMASK_BF] = 0xFFFFFF00;
	rstate->states[R600_DSA__SX_ALPHA_REF] = 0x00000000;
	rstate->states[R600_DSA__SPI_FOG_FUNC_SCALE] = 0x00000000;
	rstate->states[R600_DSA__SPI_FOG_FUNC_BIAS] = 0x00000000;
	rstate->states[R600_DSA__SPI_FOG_CNTL] = 0x00000000;
	rstate->states[R600_DSA__DB_DEPTH_CONTROL] = db_depth_control;
	rstate->states[R600_DSA__DB_SHADER_CONTROL] = 0x00000210;
	rstate->states[R600_DSA__DB_RENDER_CONTROL] = 0x00000060;
	rstate->states[R600_DSA__DB_RENDER_OVERRIDE] = 0x0000002A;
	rstate->states[R600_DSA__DB_SRESULTS_COMPARE_STATE1] = 0x00000000;
	rstate->states[R600_DSA__DB_PRELOAD_CONTROL] = 0x00000000;
	rstate->states[R600_DSA__DB_ALPHA_TO_MASK] = 0x0000AA00;
	if (radeon_state_pm4(rstate)) {
		radeon_state_decref(rstate);
		return NULL;
	}
	return rstate;
}

static void r600_bind_dsa_state(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = r600_context(ctx);
	radeon_draw_set(rctx->draw, state);
}

static void r600_set_constant_buffer(struct pipe_context *ctx,
				     uint shader, uint index,
				     struct pipe_resource *buffer)
{
	struct r600_screen *rscreen = r600_screen(ctx->screen);
	struct r600_context *rctx = r600_context(ctx);
	unsigned nconstant = 0, i, type, id;
	struct radeon_state *rstate;
	struct pipe_transfer *transfer;
	u32 *ptr;

	switch (shader) {
	case PIPE_SHADER_VERTEX:
		id = R600_VS_CONSTANT;
		type = R600_VS_CONSTANT_TYPE;
		break;
	case PIPE_SHADER_FRAGMENT:
		id = R600_PS_CONSTANT;
		type = R600_PS_CONSTANT_TYPE;
		break;
	default:
		fprintf(stderr, "%s:%d unsupported %d\n", __func__, __LINE__, shader);
		return;
	}
	if (buffer && buffer->width0 > 0) {
		nconstant = buffer->width0 / 16;
		ptr = pipe_buffer_map(ctx, buffer, PIPE_TRANSFER_READ, &transfer);
		if (ptr == NULL)
			return;
		for (i = 0; i < nconstant; i++) {
			rstate = radeon_state(rscreen->rw, type, id + i);
			if (rstate == NULL)
				return;
			rstate->states[R600_PS_CONSTANT__SQ_ALU_CONSTANT0_0] = ptr[i * 4 + 0];
			rstate->states[R600_PS_CONSTANT__SQ_ALU_CONSTANT1_0] = ptr[i * 4 + 1];
			rstate->states[R600_PS_CONSTANT__SQ_ALU_CONSTANT2_0] = ptr[i * 4 + 2];
			rstate->states[R600_PS_CONSTANT__SQ_ALU_CONSTANT3_0] = ptr[i * 4 + 3];
			if (radeon_state_pm4(rstate))
				return;
			if (radeon_draw_set_new(rctx->draw, rstate))
				return;
		}
		pipe_buffer_unmap(ctx, buffer, transfer);
	}
}

static void r600_set_stencil_ref(struct pipe_context *ctx,
				const struct pipe_stencil_ref *sr)
{
	struct r600_context *rctx = r600_context(ctx);
	rctx->stencil_ref = *sr;
}

static void r600_set_sample_mask(struct pipe_context *pipe, unsigned sample_mask)
{
}

void r600_init_state_functions(struct r600_context *rctx)
{
	rctx->context.set_sample_mask = r600_set_sample_mask;
	rctx->context.create_blend_state = r600_create_blend_state;
	rctx->context.bind_blend_state = r600_bind_blend_state;
	rctx->context.delete_blend_state = r600_delete_state;
	rctx->context.set_blend_color = r600_set_blend_color;
	rctx->context.set_clip_state = r600_set_clip_state;
	rctx->context.set_constant_buffer = r600_set_constant_buffer;
	rctx->context.create_depth_stencil_alpha_state = r600_create_dsa_state;
	rctx->context.bind_depth_stencil_alpha_state = r600_bind_dsa_state;
	rctx->context.delete_depth_stencil_alpha_state = r600_delete_state;
	rctx->context.set_framebuffer_state = r600_set_framebuffer_state;
	rctx->context.create_fs_state = r600_create_fs_state;
	rctx->context.bind_fs_state = r600_bind_fs_state;
	rctx->context.delete_fs_state = r600_delete_state;
	rctx->context.set_polygon_stipple = r600_set_polygon_stipple;
	rctx->context.create_rasterizer_state = r600_create_rs_state;
	rctx->context.bind_rasterizer_state = r600_bind_rs_state;
	rctx->context.delete_rasterizer_state = r600_delete_state;
	rctx->context.create_sampler_state = r600_create_sampler_state;
	rctx->context.bind_fragment_sampler_states = r600_bind_sampler_states;
	rctx->context.bind_vertex_sampler_states = r600_bind_sampler_states;
	rctx->context.delete_sampler_state = r600_delete_state;
	rctx->context.create_sampler_view = r600_create_sampler_view;
	rctx->context.sampler_view_destroy = r600_sampler_view_destroy;
	rctx->context.set_fragment_sampler_views = r600_set_fragment_sampler_views;
	rctx->context.set_vertex_sampler_views = r600_set_vertex_sampler_views;
	rctx->context.set_scissor_state = r600_set_scissor_state;
	rctx->context.set_viewport_state = r600_set_viewport_state;
	rctx->context.set_vertex_buffers = r600_set_vertex_buffers;
	rctx->context.create_vertex_elements_state = r600_create_vertex_elements_state;
	rctx->context.bind_vertex_elements_state = r600_bind_vertex_elements_state;
	rctx->context.delete_vertex_elements_state = r600_delete_vertex_elements_state;
	rctx->context.create_vs_state = r600_create_vs_state;
	rctx->context.bind_vs_state = r600_bind_vs_state;
	rctx->context.delete_vs_state = r600_delete_state;
	rctx->context.set_stencil_ref = r600_set_stencil_ref;
}

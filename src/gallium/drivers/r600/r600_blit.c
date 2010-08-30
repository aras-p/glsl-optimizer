/*
 * Copyright 2009 Marek Olšák <maraeo@gmail.com>
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
 *      Marek Olšák
 */
#include <errno.h>
#include <pipe/p_screen.h>
#include <util/u_blitter.h>
#include <util/u_inlines.h>
#include <util/u_memory.h>
#include "util/u_surface.h"
#include "r600_screen.h"
#include "r600_context.h"
#include "r600d.h"

static void r600_blitter_save_states(struct pipe_context *ctx)
{
	struct r600_context *rctx = r600_context(ctx);

	util_blitter_save_blend(rctx->blitter, rctx->blend);
	util_blitter_save_depth_stencil_alpha(rctx->blitter, rctx->dsa);
	if (rctx->stencil_ref) {
		util_blitter_save_stencil_ref(rctx->blitter,
					&rctx->stencil_ref->state.stencil_ref);
	}
	util_blitter_save_rasterizer(rctx->blitter, rctx->rasterizer);
	util_blitter_save_fragment_shader(rctx->blitter, rctx->ps_shader);
	util_blitter_save_vertex_shader(rctx->blitter, rctx->vs_shader);
	util_blitter_save_vertex_elements(rctx->blitter, rctx->vertex_elements);
	if (rctx->viewport) {
		util_blitter_save_viewport(rctx->blitter, &rctx->viewport->state.viewport);
	}
	if (rctx->clip) {
		util_blitter_save_clip(rctx->blitter, &rctx->clip->state.clip);
	}
	util_blitter_save_vertex_buffers(rctx->blitter, rctx->nvertex_buffer,
					rctx->vertex_buffer);

	/* remove ptr so they don't get deleted */
	rctx->blend = NULL;
	rctx->clip = NULL;
	rctx->vs_shader = NULL;
	rctx->ps_shader = NULL;
	rctx->rasterizer = NULL;
	rctx->dsa = NULL;
	rctx->vertex_elements = NULL;

	/* suspend queries */
	r600_queries_suspend(ctx);
}

static void r600_clear(struct pipe_context *ctx, unsigned buffers,
			const float *rgba, double depth, unsigned stencil)
{
	struct r600_context *rctx = r600_context(ctx);
	struct pipe_framebuffer_state *fb = &rctx->framebuffer->state.framebuffer;

	r600_blitter_save_states(ctx);
	util_blitter_clear(rctx->blitter, fb->width, fb->height,
				fb->nr_cbufs, buffers, rgba, depth,
				stencil);
	/* resume queries */
	r600_queries_resume(ctx);
}

static void r600_clear_render_target(struct pipe_context *ctx,
				     struct pipe_surface *dst,
				     const float *rgba,
				     unsigned dstx, unsigned dsty,
				     unsigned width, unsigned height)
{
	struct r600_context *rctx = r600_context(ctx);
	struct pipe_framebuffer_state *fb = &rctx->framebuffer->state.framebuffer;

	r600_blitter_save_states(ctx);
	util_blitter_save_framebuffer(rctx->blitter, fb);

	util_blitter_clear_render_target(rctx->blitter, dst, rgba,
					 dstx, dsty, width, height);
	/* resume queries */
	r600_queries_resume(ctx);
}

static void r600_clear_depth_stencil(struct pipe_context *ctx,
				     struct pipe_surface *dst,
				     unsigned clear_flags,
				     double depth,
				     unsigned stencil,
				     unsigned dstx, unsigned dsty,
				     unsigned width, unsigned height)
{
	struct r600_context *rctx = r600_context(ctx);
	struct pipe_framebuffer_state *fb = &rctx->framebuffer->state.framebuffer;

	r600_blitter_save_states(ctx);
	util_blitter_save_framebuffer(rctx->blitter, fb);

	util_blitter_clear_depth_stencil(rctx->blitter, dst, clear_flags, depth, stencil,
					 dstx, dsty, width, height);
	/* resume queries */
	r600_queries_resume(ctx);
}


static void r600_resource_copy_region(struct pipe_context *ctx,
				      struct pipe_resource *dst,
				      struct pipe_subresource subdst,
				      unsigned dstx, unsigned dsty, unsigned dstz,
				      struct pipe_resource *src,
				      struct pipe_subresource subsrc,
				      unsigned srcx, unsigned srcy, unsigned srcz,
				      unsigned width, unsigned height)
{
	util_resource_copy_region(ctx, dst, subdst, dstx, dsty, dstz,
				  src, subsrc, srcx, srcy, srcz, width, height);
}

void r600_init_blit_functions(struct r600_context *rctx)
{
	rctx->context.clear = r600_clear;
	rctx->context.clear_render_target = r600_clear_render_target;
	rctx->context.clear_depth_stencil = r600_clear_depth_stencil;
	rctx->context.resource_copy_region = r600_resource_copy_region;
}


struct r600_blit_states {
	struct radeon_state	rasterizer;
	struct radeon_state	dsa;
	struct radeon_state	blend;
	struct radeon_state	cb_cntl;
	struct radeon_state	vgt;
	struct radeon_state	draw;
	struct radeon_state	vs_constant0;
	struct radeon_state	vs_constant1;
	struct radeon_state	vs_constant2;
	struct radeon_state	vs_constant3;
	struct radeon_state	ps_shader;
	struct radeon_state	vs_shader;
	struct radeon_state	vs_resource0;
	struct radeon_state	vs_resource1;
};

static int r600_blit_state_vs_resources(struct r600_screen *rscreen, struct r600_blit_states *bstates)
{
	struct radeon_state *rstate;
	struct radeon_bo *bo;
	u32 vbo[] = {
		0xBF800000, 0xBF800000, 0x3F800000, 0x3F800000,
		0x3F000000, 0x3F000000, 0x3F000000, 0x00000000,
		0x3F800000, 0xBF800000, 0x3F800000, 0x3F800000,
		0x3F000000, 0x3F000000, 0x3F000000, 0x00000000,
		0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000,
		0x3F000000, 0x3F000000, 0x3F000000, 0x00000000,
		0xBF800000, 0x3F800000, 0x3F800000, 0x3F800000,
		0x3F000000, 0x3F000000, 0x3F000000, 0x00000000
	};

	/* simple shader */
	bo = radeon_bo(rscreen->rw, 0, 128, 4096, NULL);
	if (bo == NULL) {
		return -ENOMEM;
	}
	if (radeon_bo_map(rscreen->rw, bo)) {
		radeon_bo_decref(rscreen->rw, bo);
		return -ENOMEM;
	}
	memcpy(bo->data, vbo, 128);
	radeon_bo_unmap(rscreen->rw, bo);

	rstate = &bstates->vs_resource0;
	radeon_state_init(rstate, rscreen->rw, R600_STATE_RESOURCE, 0, R600_SHADER_VS);

	/* set states (most default value are 0 and struct already
	 * initialized to 0, thus avoid resetting them)
	 */
	rstate->states[R600_VS_RESOURCE__RESOURCE160_WORD0] = 0x00000000;
	rstate->states[R600_VS_RESOURCE__RESOURCE160_WORD1] = 0x00000080;
	rstate->states[R600_VS_RESOURCE__RESOURCE160_WORD2] = 0x02302000;
	rstate->states[R600_VS_RESOURCE__RESOURCE160_WORD3] = 0x00000000;
	rstate->states[R600_VS_RESOURCE__RESOURCE160_WORD4] = 0x00000000;
	rstate->states[R600_VS_RESOURCE__RESOURCE160_WORD5] = 0x00000000;
	rstate->states[R600_VS_RESOURCE__RESOURCE160_WORD6] = 0xC0000000;
	rstate->bo[0] = bo;
	rstate->nbo = 1;
	rstate->placement[0] = RADEON_GEM_DOMAIN_GTT;
	if (radeon_state_pm4(rstate)) {
		radeon_state_fini(rstate);
		return -ENOMEM;
	}

	rstate = &bstates->vs_resource1;
	radeon_state_init(rstate, rscreen->rw, R600_STATE_RESOURCE, 1, R600_SHADER_VS);
	rstate->states[R600_VS_RESOURCE__RESOURCE160_WORD0] = 0x00000010;
	rstate->states[R600_VS_RESOURCE__RESOURCE160_WORD1] = 0x00000070;
	rstate->states[R600_VS_RESOURCE__RESOURCE160_WORD2] = 0x02302000;
	rstate->states[R600_VS_RESOURCE__RESOURCE160_WORD3] = 0x00000000;
	rstate->states[R600_VS_RESOURCE__RESOURCE160_WORD4] = 0x00000000;
	rstate->states[R600_VS_RESOURCE__RESOURCE160_WORD5] = 0x00000000;
	rstate->states[R600_VS_RESOURCE__RESOURCE160_WORD6] = 0xC0000000;
	rstate->bo[0] = radeon_bo_incref(rscreen->rw, bo);
	rstate->nbo = 1;
	rstate->placement[0] = RADEON_GEM_DOMAIN_GTT;
	if (radeon_state_pm4(rstate)) {
		radeon_state_fini(rstate);
		return -ENOMEM;
	}

	return 0;
}

static void r600_blit_state_vs_shader(struct r600_screen *rscreen, struct radeon_state *rstate)
{
	struct radeon_bo *bo;
	u32 shader_bc_r600[] = {
		0x00000004, 0x81000400,
		0x00000008, 0xA01C0000,
		0xC001A03C, 0x94000688,
		0xC0024000, 0x94200688,
		0x7C000000, 0x002D1001,
		0x00080000, 0x00000000,
		0x7C000100, 0x002D1002,
		0x00080000, 0x00000000,
		0x00000001, 0x00601910,
		0x00000401, 0x20601910,
		0x00000801, 0x40601910,
		0x80000C01, 0x60601910,
		0x00000002, 0x00801910,
		0x00000402, 0x20801910,
		0x00000802, 0x40801910,
		0x80000C02, 0x60801910
	};
	u32 shader_bc_r700[] = {
		0x00000004, 0x81000400,
		0x00000008, 0xA01C0000,
		0xC001A03C, 0x94000688,
		0xC0024000, 0x94200688,
		0x7C000000, 0x002D1001,
		0x00080000, 0x00000000,
		0x7C000100, 0x002D1002,
		0x00080000, 0x00000000,
		0x00000001, 0x00600C90,
		0x00000401, 0x20600C90,
		0x00000801, 0x40600C90,
		0x80000C01, 0x60600C90,
		0x00000002, 0x00800C90,
		0x00000402, 0x20800C90,
		0x00000802, 0x40800C90,
		0x80000C02, 0x60800C90
	};

	/* simple shader */
	bo = radeon_bo(rscreen->rw, 0, 128, 4096, NULL);
	if (bo == NULL) {
		return;
	}
	if (radeon_bo_map(rscreen->rw, bo)) {
		radeon_bo_decref(rscreen->rw, bo);
		return;
	}
	switch (rscreen->chip_class) {
	case R600:
		memcpy(bo->data, shader_bc_r600, 128);
		break;
	case R700:
		memcpy(bo->data, shader_bc_r700, 128);
		break;
	default:
		R600_ERR("unsupported chip family\n");
		radeon_bo_unmap(rscreen->rw, bo);
		radeon_bo_decref(rscreen->rw, bo);
		return;
	}
	radeon_bo_unmap(rscreen->rw, bo);

	radeon_state_init(rstate, rscreen->rw, R600_STATE_SHADER, 0, R600_SHADER_VS);

	/* set states (most default value are 0 and struct already
	 * initialized to 0, thus avoid resetting them)
	 */
	rstate->states[R600_VS_SHADER__SPI_VS_OUT_ID_0] = 0x03020100;
	rstate->states[R600_VS_SHADER__SPI_VS_OUT_ID_1] = 0x07060504;
	rstate->states[R600_VS_SHADER__SQ_PGM_RESOURCES_VS] = 0x00000005;

	rstate->bo[0] = bo;
	rstate->bo[1] = radeon_bo_incref(rscreen->rw, bo);
	rstate->nbo = 2;
	rstate->placement[0] = RADEON_GEM_DOMAIN_GTT;
	rstate->placement[2] = RADEON_GEM_DOMAIN_GTT;

	radeon_state_pm4(rstate);
}

static void r600_blit_state_ps_shader(struct r600_screen *rscreen, struct radeon_state *rstate)
{
	struct radeon_bo *bo;
	u32 shader_bc_r600[] = {
		0x00000002, 0xA00C0000,
		0xC0008000, 0x94200688,
		0x00000000, 0x00201910,
		0x00000400, 0x20201910,
		0x00000800, 0x40201910,
		0x80000C00, 0x60201910
	};
	u32 shader_bc_r700[] = {
		0x00000002, 0xA00C0000,
		0xC0008000, 0x94200688,
		0x00000000, 0x00200C90,
		0x00000400, 0x20200C90,
		0x00000800, 0x40200C90,
		0x80000C00, 0x60200C90
	};

	/* simple shader */
	bo = radeon_bo(rscreen->rw, 0, 128, 4096, NULL);
	if (bo == NULL) {
		radeon_bo_decref(rscreen->rw, bo);
		return;
	}
	if (radeon_bo_map(rscreen->rw, bo)) {
		return;
	}
	switch (rscreen->chip_class) {
	case R600:
		memcpy(bo->data, shader_bc_r600, 48);
		break;
	case R700:
		memcpy(bo->data, shader_bc_r700, 48);
		break;
	default:
		R600_ERR("unsupported chip family\n");
		radeon_bo_unmap(rscreen->rw, bo);
		radeon_bo_decref(rscreen->rw, bo);
		return;
	}
	radeon_bo_unmap(rscreen->rw, bo);

	radeon_state_init(rstate, rscreen->rw, R600_STATE_SHADER, 0, R600_SHADER_PS);

	/* set states (most default value are 0 and struct already
	 * initialized to 0, thus avoid resetting them)
	 */
	rstate->states[R600_PS_SHADER__SPI_PS_INPUT_CNTL_0] = 0x00000C00;
	rstate->states[R600_PS_SHADER__SPI_PS_IN_CONTROL_0] = 0x10000001;
	rstate->states[R600_PS_SHADER__SQ_PGM_EXPORTS_PS] = 0x00000002;
	rstate->states[R600_PS_SHADER__SQ_PGM_RESOURCES_PS] = 0x00000002;

	rstate->bo[0] = bo;
	rstate->nbo = 1;
	rstate->placement[0] = RADEON_GEM_DOMAIN_GTT;

	radeon_state_pm4(rstate);
}

static void r600_blit_state_vgt(struct r600_screen *rscreen, struct radeon_state *rstate)
{
	radeon_state_init(rstate, rscreen->rw, R600_STATE_VGT, 0, 0);

	/* set states (most default value are 0 and struct already
	 * initialized to 0, thus avoid resetting them)
	 */
	rstate->states[R600_VGT__VGT_DMA_NUM_INSTANCES] = 0x00000001;
	rstate->states[R600_VGT__VGT_MAX_VTX_INDX] = 0x00FFFFFF;
	rstate->states[R600_VGT__VGT_PRIMITIVE_TYPE] = 0x00000005;

	radeon_state_pm4(rstate);
}

static void r600_blit_state_draw(struct r600_screen *rscreen, struct radeon_state *rstate)
{
	radeon_state_init(rstate, rscreen->rw, R600_STATE_DRAW, 0, 0);

	/* set states (most default value are 0 and struct already
	 * initialized to 0, thus avoid resetting them)
	 */
	rstate->states[R600_DRAW__VGT_DRAW_INITIATOR] = 0x00000002;
	rstate->states[R600_DRAW__VGT_NUM_INDICES] = 0x00000004;

	radeon_state_pm4(rstate);
}

static void r600_blit_state_vs_constant(struct r600_screen *rscreen, struct radeon_state *rstate,
					unsigned id, float c0, float c1, float c2, float c3)
{
	radeon_state_init(rstate, rscreen->rw, R600_STATE_CONSTANT, id, R600_SHADER_VS);

	/* set states (most default value are 0 and struct already
	 * initialized to 0, thus avoid resetting them)
	 */
	rstate->states[R600_VS_CONSTANT__SQ_ALU_CONSTANT0_256] = fui(c0);
	rstate->states[R600_VS_CONSTANT__SQ_ALU_CONSTANT1_256] = fui(c1);
	rstate->states[R600_VS_CONSTANT__SQ_ALU_CONSTANT2_256] = fui(c2);
	rstate->states[R600_VS_CONSTANT__SQ_ALU_CONSTANT3_256] = fui(c3);

	radeon_state_pm4(rstate);
}

static void r600_blit_state_rasterizer(struct r600_screen *rscreen, struct radeon_state *rstate)
{
	radeon_state_init(rstate, rscreen->rw, R600_STATE_RASTERIZER, 0, 0);

	/* set states (most default value are 0 and struct already
	 * initialized to 0, thus avoid resetting them)
	 */
	rstate->states[R600_RASTERIZER__PA_CL_GB_HORZ_CLIP_ADJ] = 0x3F800000;
	rstate->states[R600_RASTERIZER__PA_CL_GB_HORZ_DISC_ADJ] = 0x3F800000;
	rstate->states[R600_RASTERIZER__PA_CL_GB_VERT_CLIP_ADJ] = 0x3F800000;
	rstate->states[R600_RASTERIZER__PA_CL_GB_VERT_DISC_ADJ] = 0x3F800000;
	rstate->states[R600_RASTERIZER__PA_SC_LINE_CNTL] = 0x00000400;
	rstate->states[R600_RASTERIZER__PA_SC_LINE_STIPPLE] = 0x00000005;
	rstate->states[R600_RASTERIZER__PA_SU_LINE_CNTL] = 0x00000008;
	rstate->states[R600_RASTERIZER__PA_SU_POINT_MINMAX] = 0x80000000;
	rstate->states[R600_RASTERIZER__PA_SU_SC_MODE_CNTL] = 0x00080004;
	rstate->states[R600_RASTERIZER__SPI_INTERP_CONTROL_0] = 0x00000001;

	radeon_state_pm4(rstate);
}

static void r600_blit_state_dsa(struct r600_screen *rscreen, struct radeon_state *rstate)
{
	radeon_state_init(rstate, rscreen->rw, R600_STATE_DSA, 0, 0);

	/* set states (most default value are 0 and struct already
	 * initialized to 0, thus avoid resetting them)
	 */
	rstate->states[R600_DSA__DB_ALPHA_TO_MASK] = 0x0000AA00;
	rstate->states[R600_DSA__DB_DEPTH_CLEAR] = 0x3F800000;
	rstate->states[R600_DSA__DB_RENDER_CONTROL] = 0x00000060;
	rstate->states[R600_DSA__DB_RENDER_OVERRIDE] = 0x0000002A;
	rstate->states[R600_DSA__DB_SHADER_CONTROL] = 0x00000210;

	radeon_state_pm4(rstate);
}

static void r600_blit_state_blend(struct r600_screen *rscreen, struct radeon_state *rstate)
{
	radeon_state_init(rstate, rscreen->rw, R600_STATE_BLEND, 0, 0);
	radeon_state_pm4(rstate);
}

static void r600_blit_state_cb_cntl(struct r600_screen *rscreen, struct radeon_state *rstate)
{
	radeon_state_init(rstate, rscreen->rw, R600_STATE_CB_CNTL, 0, 0);
	rstate->states[R600_CB_CNTL__CB_CLRCMP_CONTROL] = 0x01000000;
	rstate->states[R600_CB_CNTL__CB_CLRCMP_DST] = 0x000000FF;
	rstate->states[R600_CB_CNTL__CB_CLRCMP_MSK] = 0xFFFFFFFF;
	rstate->states[R600_CB_CNTL__CB_COLOR_CONTROL] = 0x00CC0080;
	rstate->states[R600_CB_CNTL__CB_SHADER_MASK] = 0x0000000F;
	rstate->states[R600_CB_CNTL__CB_TARGET_MASK] = 0x0000000F;
	rstate->states[R600_CB_CNTL__PA_SC_AA_MASK] = 0xFFFFFFFF;
	radeon_state_pm4(rstate);
}

static int r600_blit_states_init(struct pipe_context *ctx, struct r600_blit_states *bstates)
{
	struct r600_screen *rscreen = r600_screen(ctx->screen);

	r600_blit_state_ps_shader(rscreen, &bstates->ps_shader);
	r600_blit_state_vs_shader(rscreen, &bstates->vs_shader);
	r600_blit_state_vgt(rscreen, &bstates->vgt);
	r600_blit_state_draw(rscreen, &bstates->draw);
	r600_blit_state_vs_constant(rscreen, &bstates->vs_constant0, 0, 1.0, 0.0, 0.0, 0.0);
	r600_blit_state_vs_constant(rscreen, &bstates->vs_constant1, 1, 0.0, 1.0, 0.0, 0.0);
	r600_blit_state_vs_constant(rscreen, &bstates->vs_constant2, 2, 0.0, 0.0, -0.00199900055, 0.0);
	r600_blit_state_vs_constant(rscreen, &bstates->vs_constant3, 3, 0.0, 0.0, -0.99900049, 1.0);
	r600_blit_state_rasterizer(rscreen, &bstates->rasterizer);
	r600_blit_state_dsa(rscreen, &bstates->dsa);
	r600_blit_state_blend(rscreen, &bstates->blend);
	r600_blit_state_cb_cntl(rscreen, &bstates->cb_cntl);
	r600_blit_state_vs_resources(rscreen, bstates);
	return 0;
}

static void r600_blit_states_destroy(struct pipe_context *ctx, struct r600_blit_states *bstates)
{
	radeon_state_fini(&bstates->ps_shader);
	radeon_state_fini(&bstates->vs_shader);
	radeon_state_fini(&bstates->vs_resource0);
	radeon_state_fini(&bstates->vs_resource1);
}

int r600_blit_uncompress_depth(struct pipe_context *ctx, struct r600_resource_texture *rtexture, unsigned level)
{
	struct r600_screen *rscreen = r600_screen(ctx->screen);
	struct r600_context *rctx = r600_context(ctx);
	struct radeon_draw draw;
	struct r600_blit_states bstates;
	int r;

	r = r600_texture_scissor(ctx, rtexture, level);
	if (r) {
		return r;
	}
	r = r600_texture_cb(ctx, rtexture, 0, level);
	if (r) {
		return r;
	}
	r = r600_texture_db(ctx, rtexture, level);
	if (r) {
		return r;
	}
	r = r600_texture_viewport(ctx, rtexture, level);
	if (r) {
		return r;
	}

	r = r600_blit_states_init(ctx, &bstates);
	if (r) {
		return r;
	}
	bstates.dsa.states[R600_DSA__DB_RENDER_CONTROL] = 0x0000008C;
	bstates.cb_cntl.states[R600_CB_CNTL__CB_TARGET_MASK] = 0x00000001;
	/* force rebuild */
	bstates.dsa.cpm4 = bstates.cb_cntl.cpm4 = 0;
	if (radeon_state_pm4(&bstates.dsa)) {
		goto out;
	}
	if (radeon_state_pm4(&bstates.cb_cntl)) {
		goto out;
	}

	r = radeon_draw_init(&draw, rscreen->rw);
	if (r) {
		R600_ERR("failed creating draw for uncompressing textures\n");
		goto out;
	}

	radeon_draw_bind(&draw, &bstates.vs_shader);
	radeon_draw_bind(&draw, &bstates.ps_shader);
	radeon_draw_bind(&draw, &bstates.rasterizer);
	radeon_draw_bind(&draw, &bstates.dsa);
	radeon_draw_bind(&draw, &bstates.blend);
	radeon_draw_bind(&draw, &bstates.cb_cntl);
	radeon_draw_bind(&draw, &rctx->config);
	radeon_draw_bind(&draw, &bstates.vgt);
	radeon_draw_bind(&draw, &bstates.draw);
	radeon_draw_bind(&draw, &bstates.vs_resource0);
	radeon_draw_bind(&draw, &bstates.vs_resource1);
	radeon_draw_bind(&draw, &bstates.vs_constant0);
	radeon_draw_bind(&draw, &bstates.vs_constant1);
	radeon_draw_bind(&draw, &bstates.vs_constant2);
	radeon_draw_bind(&draw, &bstates.vs_constant3);
	radeon_draw_bind(&draw, &rtexture->viewport[level]);
	radeon_draw_bind(&draw, &rtexture->scissor[level]);
	radeon_draw_bind(&draw, &rtexture->cb[0][level]);
	radeon_draw_bind(&draw, &rtexture->db[level]);

	/* suspend queries */
	r600_queries_suspend(ctx);

	/* schedule draw*/
	r = radeon_ctx_set_draw(&rctx->ctx, &draw);
	if (r == -EBUSY) {
		r600_flush(ctx, 0, NULL);
		r = radeon_ctx_set_draw(&rctx->ctx, &draw);
	}
	if (r) {
		goto out;
	}

	/* resume queries */
	r600_queries_resume(ctx);

out:
	r600_blit_states_destroy(ctx, &bstates);
	return r;
}

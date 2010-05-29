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
#include <util/u_inlines.h>
#include <util/u_format.h>
#include <util/u_memory.h>
#include "r600_screen.h"
#include "r600_texture.h"
#include "r600_context.h"
#include "r600d.h"


static void r600_delete_state(struct pipe_context *ctx, void *state)
{
	struct radeon_state *rstate = state;

	radeon_state_decref(rstate);
}

static void *r600_create_blend_state(struct pipe_context *ctx,
					const struct pipe_blend_state *state)
{
	struct r600_screen *rscreen = (struct r600_screen*)ctx->screen;
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
	struct r600_context *rctx = (struct r600_context*)ctx;
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
	struct r600_screen *rscreen = (struct r600_screen*)ctx->screen;
	struct r600_context *rctx = (struct r600_context*)ctx;
	struct r600_texture *rtex;
	struct r600_buffer *rbuffer;
	struct radeon_state *rstate;
	unsigned level = state->cbufs[0]->level;
	unsigned pitch, slice;

	rstate = radeon_state(rscreen->rw, R600_CB0_TYPE, R600_CB0);
	if (rstate == NULL)
		return;
	rtex = (struct r600_texture*)state->cbufs[0]->texture;
	rbuffer = (struct r600_buffer*)rtex->buffer;
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
	rctx->db = radeon_state(rscreen->rw, R600_DB_TYPE, R600_DB);
	rctx->db->bo[0] = radeon_bo_incref(rscreen->rw, rstate->bo[0]);
	rctx->db->nbo = 1;
	rctx->db->placement[0] = RADEON_GEM_DOMAIN_GTT;
	rctx->fb_state = *state;
}

static void *r600_create_fs_state(struct pipe_context *ctx,
					const struct pipe_shader_state *shader)
{
	return r600_pipe_shader_create(ctx, C_PROGRAM_TYPE_FS, shader->tokens);
}

static void r600_bind_fs_state(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context*)ctx;

	rctx->ps_shader = state;
}

static void *r600_create_vs_state(struct pipe_context *ctx,
					const struct pipe_shader_state *shader)
{
	return r600_pipe_shader_create(ctx, C_PROGRAM_TYPE_VS, shader->tokens);
}

static void r600_bind_vs_state(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context*)ctx;

	rctx->vs_shader = state;
}

static void r600_set_polygon_stipple(struct pipe_context *ctx,
					 const struct pipe_poly_stipple *state)
{
}

static void *r600_create_rs_state(struct pipe_context *ctx,
					const struct pipe_rasterizer_state *state)
{
	struct r600_screen *rscreen = (struct r600_screen*)ctx->screen;
	struct r600_context *rctx = (struct r600_context*)ctx;
	struct radeon_state *rstate;

	rctx->flat_shade = state->flatshade;
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
	struct r600_context *rctx = (struct r600_context*)ctx;
	radeon_draw_set(rctx->draw, state);
}

static void *r600_create_sampler_state(struct pipe_context *ctx,
					const struct pipe_sampler_state *state)
{
	return NULL;
}

static void r600_bind_sampler_states(struct pipe_context *ctx,
					unsigned count, void **states)
{
}

static struct pipe_sampler_view *r600_create_sampler_view(struct pipe_context *ctx,
							  struct pipe_resource *texture,
							  const struct pipe_sampler_view *templ)
{
	struct pipe_sampler_view *view = CALLOC_STRUCT(pipe_sampler_view);

	*view = *templ;
	return view;
}

static void r600_sampler_view_destroy(struct pipe_context *ctx,
				      struct pipe_sampler_view *view)
{
	FREE(view);
}

static void r600_set_fragment_sampler_views(struct pipe_context *ctx,
					    unsigned count,
					    struct pipe_sampler_view **views)
{
}

static void r600_set_vertex_sampler_views(struct pipe_context *ctx,
					  unsigned count,
					  struct pipe_sampler_view **views)
{
}

static void r600_set_scissor_state(struct pipe_context *ctx,
					const struct pipe_scissor_state *state)
{
	struct r600_screen *rscreen = (struct r600_screen*)ctx->screen;
	struct r600_context *rctx = (struct r600_context*)ctx;
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
	struct r600_screen *rscreen = (struct r600_screen*)ctx->screen;
	struct r600_context *rctx = (struct r600_context*)ctx;
	struct radeon_state *rstate;

	rstate = radeon_state(rscreen->rw, R600_VIEWPORT_TYPE, R600_VIEWPORT);
	if (rstate == NULL)
		return;
	rstate->states[R600_VIEWPORT__PA_SC_VPORT_ZMIN_0] = 0x00000000;
	rstate->states[R600_VIEWPORT__PA_SC_VPORT_ZMAX_0] = 0x3F800000;
	rstate->states[R600_VIEWPORT__PA_CL_VPORT_XSCALE_0] = r600_float_to_u32(state->scale[0]);
	rstate->states[R600_VIEWPORT__PA_CL_VPORT_YSCALE_0] = r600_float_to_u32(state->scale[1]);
	rstate->states[R600_VIEWPORT__PA_CL_VPORT_ZSCALE_0] = r600_float_to_u32(state->scale[2]);
	rstate->states[R600_VIEWPORT__PA_CL_VPORT_XOFFSET_0] = r600_float_to_u32(state->translate[0]);
	rstate->states[R600_VIEWPORT__PA_CL_VPORT_YOFFSET_0] = r600_float_to_u32(state->translate[1]);
	rstate->states[R600_VIEWPORT__PA_CL_VPORT_ZOFFSET_0] = r600_float_to_u32(state->translate[2]);
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
	struct r600_context *rctx = (struct r600_context*)ctx;

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
	struct r600_context *rctx = (struct r600_context*)ctx;
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
	struct r600_screen *rscreen = (struct r600_screen*)ctx->screen;
	struct radeon_state *rstate;

	rstate = radeon_state(rscreen->rw, R600_DSA_TYPE, R600_DSA);
	if (rstate == NULL)
		return NULL;
	rstate->states[R600_DSA__DB_STENCIL_CLEAR] = 0x00000000;
	rstate->states[R600_DSA__DB_DEPTH_CLEAR] = 0x3F800000;
	rstate->states[R600_DSA__SX_ALPHA_TEST_CONTROL] = 0x00000000;
	rstate->states[R600_DSA__DB_STENCILREFMASK] = 0xFFFFFF00;
	rstate->states[R600_DSA__DB_STENCILREFMASK_BF] = 0xFFFFFF00;
	rstate->states[R600_DSA__SX_ALPHA_REF] = 0x00000000;
	rstate->states[R600_DSA__SPI_FOG_FUNC_SCALE] = 0x00000000;
	rstate->states[R600_DSA__SPI_FOG_FUNC_BIAS] = 0x00000000;
	rstate->states[R600_DSA__SPI_FOG_CNTL] = 0x00000000;
	rstate->states[R600_DSA__DB_DEPTH_CONTROL] = 0x00700700;
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
	struct r600_context *rctx = (struct r600_context*)ctx;
	radeon_draw_set(rctx->draw, state);
}

static void r600_set_constant_buffer(struct pipe_context *ctx,
				     uint shader, uint index,
				     struct pipe_resource *buffer)
{
	struct r600_screen *rscreen = (struct r600_screen*)ctx->screen;
	struct r600_context *rctx = (struct r600_context*)ctx;
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
	struct r600_context *rctx = (struct r600_context*)ctx;
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

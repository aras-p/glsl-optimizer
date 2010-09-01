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
#include <errno.h>
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_memory.h"
#include "r600_screen.h"
#include "r600_context.h"
#include "r600_resource.h"
#include "r600d.h"
#include "r600_state_inlines.h"

static void *r600_create_blend_state(struct pipe_context *ctx,
					const struct pipe_blend_state *state)
{
	struct r600_context *rctx = r600_context(ctx);

	return r600_context_state(rctx, pipe_blend_type, state);
}

static void *r600_create_dsa_state(struct pipe_context *ctx,
					const struct pipe_depth_stencil_alpha_state *state)
{
	struct r600_context *rctx = r600_context(ctx);

	return r600_context_state(rctx, pipe_dsa_type, state);
}

static void *r600_create_rs_state(struct pipe_context *ctx,
					const struct pipe_rasterizer_state *state)
{
	struct r600_context *rctx = r600_context(ctx);

	return r600_context_state(rctx, pipe_rasterizer_type, state);
}

static void *r600_create_sampler_state(struct pipe_context *ctx,
					const struct pipe_sampler_state *state)
{
	struct r600_context *rctx = r600_context(ctx);

	return r600_context_state(rctx, pipe_sampler_type, state);
}

static void r600_sampler_view_destroy(struct pipe_context *ctx,
				      struct pipe_sampler_view *state)
{
	struct r600_context_state *rstate = (struct r600_context_state *)state;

	r600_context_state_decref(rstate);
}

static struct pipe_sampler_view *r600_create_sampler_view(struct pipe_context *ctx,
							struct pipe_resource *texture,
							const struct pipe_sampler_view *state)
{
	struct r600_context *rctx = r600_context(ctx);
	struct r600_context_state *rstate;

	rstate = r600_context_state(rctx, pipe_sampler_type, state);
	pipe_reference(NULL, &texture->reference);
	rstate->state.sampler_view.texture = texture;
	rstate->state.sampler_view.reference.count = 1;
	rstate->state.sampler_view.context = ctx;
	return &rstate->state.sampler_view;
}

static void *r600_create_shader_state(struct pipe_context *ctx,
					const struct pipe_shader_state *state)
{
	struct r600_context *rctx = r600_context(ctx);

	return r600_context_state(rctx, pipe_shader_type, state);
}

static void *r600_create_vertex_elements(struct pipe_context *ctx,
				unsigned count,
				const struct pipe_vertex_element *elements)
{
	struct r600_vertex_element *v = CALLOC_STRUCT(r600_vertex_element);

	assert(count < 32);
	v->count = count;
	memcpy(v->elements, elements, count * sizeof(struct pipe_vertex_element));
	v->refcount = 1;
	return v;
}

static void r600_bind_state(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = r600_context(ctx);
	struct r600_context_state *rstate = (struct r600_context_state *)state;

	if (state == NULL)
		return;
	switch (rstate->type) {
	case pipe_rasterizer_type:
		rctx->rasterizer = r600_context_state_decref(rctx->rasterizer);
		rctx->rasterizer = r600_context_state_incref(rstate);
		break;
	case pipe_poly_stipple_type:
		rctx->poly_stipple = r600_context_state_decref(rctx->poly_stipple);
		rctx->poly_stipple = r600_context_state_incref(rstate);
		break;
	case pipe_scissor_type:
		rctx->scissor = r600_context_state_decref(rctx->scissor);
		rctx->scissor = r600_context_state_incref(rstate);
		break;
	case pipe_clip_type:
		rctx->clip = r600_context_state_decref(rctx->clip);
		rctx->clip = r600_context_state_incref(rstate);
		break;
	case pipe_depth_type:
		rctx->depth = r600_context_state_decref(rctx->depth);
		rctx->depth = r600_context_state_incref(rstate);
		break;
	case pipe_stencil_type:
		rctx->stencil = r600_context_state_decref(rctx->stencil);
		rctx->stencil = r600_context_state_incref(rstate);
		break;
	case pipe_alpha_type:
		rctx->alpha = r600_context_state_decref(rctx->alpha);
		rctx->alpha = r600_context_state_incref(rstate);
		break;
	case pipe_dsa_type:
		rctx->dsa = r600_context_state_decref(rctx->dsa);
		rctx->dsa = r600_context_state_incref(rstate);
		break;
	case pipe_blend_type:
		rctx->blend = r600_context_state_decref(rctx->blend);
		rctx->blend = r600_context_state_incref(rstate);
		break;
	case pipe_framebuffer_type:
		rctx->framebuffer = r600_context_state_decref(rctx->framebuffer);
		rctx->framebuffer = r600_context_state_incref(rstate);
		break;
	case pipe_stencil_ref_type:
		rctx->stencil_ref = r600_context_state_decref(rctx->stencil_ref);
		rctx->stencil_ref = r600_context_state_incref(rstate);
		break;
	case pipe_viewport_type:
		rctx->viewport = r600_context_state_decref(rctx->viewport);
		rctx->viewport = r600_context_state_incref(rstate);
		break;
	case pipe_shader_type:
	case pipe_sampler_type:
	case pipe_sampler_view_type:
	default:
		R600_ERR("invalid type %d\n", rstate->type);
		return;
	}
}

static void r600_bind_ps_shader(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = r600_context(ctx);
	struct r600_context_state *rstate = (struct r600_context_state *)state;

	rctx->ps_shader = r600_context_state_decref(rctx->ps_shader);
	rctx->ps_shader = r600_context_state_incref(rstate);
}

static void r600_bind_vs_shader(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = r600_context(ctx);
	struct r600_context_state *rstate = (struct r600_context_state *)state;

	rctx->vs_shader = r600_context_state_decref(rctx->vs_shader);
	rctx->vs_shader = r600_context_state_incref(rstate);
}

static void r600_delete_vertex_element(struct pipe_context *ctx, void *state)
{
	struct r600_vertex_element *v = (struct r600_vertex_element*)state;

	if (v == NULL)
		return;
	if (--v->refcount)
		return;
	free(v);
}

static void r600_bind_vertex_elements(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = r600_context(ctx);
	struct r600_vertex_element *v = (struct r600_vertex_element*)state;

	r600_delete_vertex_element(ctx, rctx->vertex_elements);
	rctx->vertex_elements = v;
	if (v) {
		v->refcount++;
	}
}

static void r600_bind_ps_sampler(struct pipe_context *ctx,
					unsigned count, void **states)
{
	struct r600_context *rctx = r600_context(ctx);
	struct r600_context_state *rstate;
	unsigned i;

	for (i = 0; i < rctx->ps_nsampler; i++) {
		rctx->ps_sampler[i] = r600_context_state_decref(rctx->ps_sampler[i]);
	}
	for (i = 0; i < count; i++) {
		rstate = (struct r600_context_state *)states[i];
		rctx->ps_sampler[i] = r600_context_state_incref(rstate);
	}
	rctx->ps_nsampler = count;
}

static void r600_bind_vs_sampler(struct pipe_context *ctx,
					unsigned count, void **states)
{
	struct r600_context *rctx = r600_context(ctx);
	struct r600_context_state *rstate;
	unsigned i;

	for (i = 0; i < rctx->vs_nsampler; i++) {
		rctx->vs_sampler[i] = r600_context_state_decref(rctx->vs_sampler[i]);
	}
	for (i = 0; i < count; i++) {
		rstate = (struct r600_context_state *)states[i];
		rctx->vs_sampler[i] = r600_context_state_incref(rstate);
	}
	rctx->vs_nsampler = count;
}

static void r600_delete_state(struct pipe_context *ctx, void *state)
{
	struct r600_context_state *rstate = (struct r600_context_state *)state;

	r600_context_state_decref(rstate);
}

static void r600_set_blend_color(struct pipe_context *ctx,
					const struct pipe_blend_color *color)
{
	struct r600_context *rctx = r600_context(ctx);

	rctx->blend_color = *color;
}

static void r600_set_clip_state(struct pipe_context *ctx,
				const struct pipe_clip_state *state)
{
	struct r600_context *rctx = r600_context(ctx);
	struct r600_context_state *rstate;

	rstate = r600_context_state(rctx, pipe_clip_type, state);
	r600_bind_state(ctx, rstate);
	/* refcount is taken care of this */
	r600_delete_state(ctx, rstate);
}

static void r600_set_constant_buffer(struct pipe_context *ctx,
					uint shader, uint index,
					struct pipe_resource *buffer)
{
	struct r600_screen *rscreen = r600_screen(ctx->screen);
	struct r600_context *rctx = r600_context(ctx);
	unsigned nconstant = 0, i, type, shader_class;
	struct radeon_state *rstate;
	struct pipe_transfer *transfer;
	u32 *ptr;

	type = R600_STATE_CONSTANT;

	switch (shader) {
	case PIPE_SHADER_VERTEX:
		shader_class = R600_SHADER_VS;
		break;
	case PIPE_SHADER_FRAGMENT:
		shader_class = R600_SHADER_PS;
		break;
	default:
		R600_ERR("unsupported %d\n", shader);
		return;
	}
	if (buffer && buffer->width0 > 0) {
		nconstant = buffer->width0 / 16;
		ptr = pipe_buffer_map(ctx, buffer, PIPE_TRANSFER_READ, &transfer);
		if (ptr == NULL)
			return;
		for (i = 0; i < nconstant; i++) {
			rstate = radeon_state_shader(rscreen->rw, type, i, shader_class);
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

static void r600_set_ps_sampler_view(struct pipe_context *ctx,
					unsigned count,
					struct pipe_sampler_view **views)
{
	struct r600_context *rctx = r600_context(ctx);
	struct r600_context_state *rstate;
	unsigned i;

	for (i = 0; i < rctx->ps_nsampler_view; i++) {
		rctx->ps_sampler_view[i] = r600_context_state_decref(rctx->ps_sampler_view[i]);
	}
	for (i = 0; i < count; i++) {
		rstate = (struct r600_context_state *)views[i];
		rctx->ps_sampler_view[i] = r600_context_state_incref(rstate);
	}
	rctx->ps_nsampler_view = count;
}

static void r600_set_vs_sampler_view(struct pipe_context *ctx,
					unsigned count,
					struct pipe_sampler_view **views)
{
	struct r600_context *rctx = r600_context(ctx);
	struct r600_context_state *rstate;
	unsigned i;

	for (i = 0; i < rctx->vs_nsampler_view; i++) {
		rctx->vs_sampler_view[i] = r600_context_state_decref(rctx->vs_sampler_view[i]);
	}
	for (i = 0; i < count; i++) {
		rstate = (struct r600_context_state *)views[i];
		rctx->vs_sampler_view[i] = r600_context_state_incref(rstate);
	}
	rctx->vs_nsampler_view = count;
}

static void r600_set_framebuffer_state(struct pipe_context *ctx,
					const struct pipe_framebuffer_state *state)
{
	struct r600_context *rctx = r600_context(ctx);
	struct r600_context_state *rstate;

	rstate = r600_context_state(rctx, pipe_framebuffer_type, state);
	r600_bind_state(ctx, rstate);
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
	struct r600_context *rctx = r600_context(ctx);
	struct r600_context_state *rstate;

	rstate = r600_context_state(rctx, pipe_scissor_type, state);
	r600_bind_state(ctx, rstate);
	/* refcount is taken care of this */
	r600_delete_state(ctx, rstate);
}

static void r600_set_stencil_ref(struct pipe_context *ctx,
				const struct pipe_stencil_ref *state)
{
	struct r600_context *rctx = r600_context(ctx);
	struct r600_context_state *rstate;

	rstate = r600_context_state(rctx, pipe_stencil_ref_type, state);
	r600_bind_state(ctx, rstate);
	/* refcount is taken care of this */
	r600_delete_state(ctx, rstate);
}

static void r600_set_vertex_buffers(struct pipe_context *ctx,
					unsigned count,
					const struct pipe_vertex_buffer *buffers)
{
	struct r600_context *rctx = r600_context(ctx);
	unsigned i;

	for (i = 0; i < rctx->nvertex_buffer; i++) {
		pipe_resource_reference(&rctx->vertex_buffer[i].buffer, NULL);
	}
	memcpy(rctx->vertex_buffer, buffers, sizeof(struct pipe_vertex_buffer) * count);
	for (i = 0; i < count; i++) {
		rctx->vertex_buffer[i].buffer = NULL;
		pipe_resource_reference(&rctx->vertex_buffer[i].buffer, buffers[i].buffer);
	}
	rctx->nvertex_buffer = count;
}

static void r600_set_index_buffer(struct pipe_context *ctx,
				  const struct pipe_index_buffer *ib)
{
	struct r600_context *rctx = r600_context(ctx);

	if (ib) {
		pipe_resource_reference(&rctx->index_buffer.buffer, ib->buffer);
		memcpy(&rctx->index_buffer, ib, sizeof(rctx->index_buffer));
	} else {
		pipe_resource_reference(&rctx->index_buffer.buffer, NULL);
		memset(&rctx->index_buffer, 0, sizeof(rctx->index_buffer));
	}

	/* TODO make this more like a state */
}

static void r600_set_viewport_state(struct pipe_context *ctx,
					const struct pipe_viewport_state *state)
{
	struct r600_context *rctx = r600_context(ctx);
	struct r600_context_state *rstate;

	rstate = r600_context_state(rctx, pipe_viewport_type, state);
	r600_bind_state(ctx, rstate);
	r600_delete_state(ctx, rstate);
}

void r600_init_state_functions(struct r600_context *rctx)
{
	rctx->context.create_blend_state = r600_create_blend_state;
	rctx->context.create_depth_stencil_alpha_state = r600_create_dsa_state;
	rctx->context.create_fs_state = r600_create_shader_state;
	rctx->context.create_rasterizer_state = r600_create_rs_state;
	rctx->context.create_sampler_state = r600_create_sampler_state;
	rctx->context.create_sampler_view = r600_create_sampler_view;
	rctx->context.create_vertex_elements_state = r600_create_vertex_elements;
	rctx->context.create_vs_state = r600_create_shader_state;
	rctx->context.bind_blend_state = r600_bind_state;
	rctx->context.bind_depth_stencil_alpha_state = r600_bind_state;
	rctx->context.bind_fragment_sampler_states = r600_bind_ps_sampler;
	rctx->context.bind_fs_state = r600_bind_ps_shader;
	rctx->context.bind_rasterizer_state = r600_bind_state;
	rctx->context.bind_vertex_elements_state = r600_bind_vertex_elements;
	rctx->context.bind_vertex_sampler_states = r600_bind_vs_sampler;
	rctx->context.bind_vs_state = r600_bind_vs_shader;
	rctx->context.delete_blend_state = r600_delete_state;
	rctx->context.delete_depth_stencil_alpha_state = r600_delete_state;
	rctx->context.delete_fs_state = r600_delete_state;
	rctx->context.delete_rasterizer_state = r600_delete_state;
	rctx->context.delete_sampler_state = r600_delete_state;
	rctx->context.delete_vertex_elements_state = r600_delete_vertex_element;
	rctx->context.delete_vs_state = r600_delete_state;
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
}

struct r600_context_state *r600_context_state_incref(struct r600_context_state *rstate)
{
	if (rstate == NULL)
		return NULL;
	rstate->refcount++;
	return rstate;
}

struct r600_context_state *r600_context_state_decref(struct r600_context_state *rstate)
{
	unsigned i;

	if (rstate == NULL)
		return NULL;
	if (--rstate->refcount)
		return NULL;
	switch (rstate->type) {
	case pipe_sampler_view_type:
		pipe_resource_reference(&rstate->state.sampler_view.texture, NULL);
		break;
	case pipe_framebuffer_type:
		for (i = 0; i < rstate->state.framebuffer.nr_cbufs; i++) {
			pipe_surface_reference(&rstate->state.framebuffer.cbufs[i], NULL);
		}
		pipe_surface_reference(&rstate->state.framebuffer.zsbuf, NULL);
		break;
	case pipe_viewport_type:
	case pipe_depth_type:
	case pipe_rasterizer_type:
	case pipe_poly_stipple_type:
	case pipe_scissor_type:
	case pipe_clip_type:
	case pipe_stencil_type:
	case pipe_alpha_type:
	case pipe_dsa_type:
	case pipe_blend_type:
	case pipe_stencil_ref_type:
	case pipe_shader_type:
	case pipe_sampler_type:
		break;
	default:
		R600_ERR("invalid type %d\n", rstate->type);
		return NULL;
	}
	radeon_state_decref(rstate->rstate);
	FREE(rstate);
	return NULL;
}

struct r600_context_state *r600_context_state(struct r600_context *rctx, unsigned type, const void *state)
{
	struct r600_context_state *rstate = CALLOC_STRUCT(r600_context_state);
	const union pipe_states *states = state;
	unsigned i;
	int r;

	if (rstate == NULL)
		return NULL;
	rstate->type = type;
	rstate->refcount = 1;

	switch (rstate->type) {
	case pipe_sampler_view_type:
		rstate->state.sampler_view = (*states).sampler_view;
		rstate->state.sampler_view.texture = NULL;
		break;
	case pipe_framebuffer_type:
		rstate->state.framebuffer = (*states).framebuffer;
		for (i = 0; i < rstate->state.framebuffer.nr_cbufs; i++) {
			pipe_surface_reference(&rstate->state.framebuffer.cbufs[i],
						(*states).framebuffer.cbufs[i]);
		}
		pipe_surface_reference(&rstate->state.framebuffer.zsbuf,
					(*states).framebuffer.zsbuf);
		break;
	case pipe_viewport_type:
		rstate->state.viewport = (*states).viewport;
		break;
	case pipe_depth_type:
		rstate->state.depth = (*states).depth;
		break;
	case pipe_rasterizer_type:
		rstate->state.rasterizer = (*states).rasterizer;
		break;
	case pipe_poly_stipple_type:
		rstate->state.poly_stipple = (*states).poly_stipple;
		break;
	case pipe_scissor_type:
		rstate->state.scissor = (*states).scissor;
		break;
	case pipe_clip_type:
		rstate->state.clip = (*states).clip;
		break;
	case pipe_stencil_type:
		rstate->state.stencil = (*states).stencil;
		break;
	case pipe_alpha_type:
		rstate->state.alpha = (*states).alpha;
		break;
	case pipe_dsa_type:
		rstate->state.dsa = (*states).dsa;
		break;
	case pipe_blend_type:
		rstate->state.blend = (*states).blend;
		break;
	case pipe_stencil_ref_type:
		rstate->state.stencil_ref = (*states).stencil_ref;
		break;
	case pipe_shader_type:
		rstate->state.shader = (*states).shader;
		r =  r600_pipe_shader_create(&rctx->context, rstate, rstate->state.shader.tokens);
		if (r) {
			r600_context_state_decref(rstate);
			return NULL;
		}
		break;
	case pipe_sampler_type:
		rstate->state.sampler = (*states).sampler;
		break;
	default:
		R600_ERR("invalid type %d\n", rstate->type);
		FREE(rstate);
		return NULL;
	}
	return rstate;
}

static struct radeon_state *r600_blend(struct r600_context *rctx)
{
	struct r600_screen *rscreen = rctx->screen;
	struct radeon_state *rstate;
	const struct pipe_blend_state *state = &rctx->blend->state.blend;
	int i;

	rstate = radeon_state(rscreen->rw, R600_STATE_BLEND, 0);
	if (rstate == NULL)
		return NULL;
	rstate->states[R600_BLEND__CB_BLEND_RED] = fui(rctx->blend_color.color[0]);
	rstate->states[R600_BLEND__CB_BLEND_GREEN] = fui(rctx->blend_color.color[1]);
	rstate->states[R600_BLEND__CB_BLEND_BLUE] = fui(rctx->blend_color.color[2]);
	rstate->states[R600_BLEND__CB_BLEND_ALPHA] = fui(rctx->blend_color.color[3]);
	rstate->states[R600_BLEND__CB_BLEND0_CONTROL] = 0x00000000;
	rstate->states[R600_BLEND__CB_BLEND1_CONTROL] = 0x00000000;
	rstate->states[R600_BLEND__CB_BLEND2_CONTROL] = 0x00000000;
	rstate->states[R600_BLEND__CB_BLEND3_CONTROL] = 0x00000000;
	rstate->states[R600_BLEND__CB_BLEND4_CONTROL] = 0x00000000;
	rstate->states[R600_BLEND__CB_BLEND5_CONTROL] = 0x00000000;
	rstate->states[R600_BLEND__CB_BLEND6_CONTROL] = 0x00000000;
	rstate->states[R600_BLEND__CB_BLEND7_CONTROL] = 0x00000000;
	rstate->states[R600_BLEND__CB_BLEND_CONTROL] = 0x00000000;

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

		bc |= S_028804_COLOR_COMB_FCN(r600_translate_blend_function(eqRGB));
		bc |= S_028804_COLOR_SRCBLEND(r600_translate_blend_factor(srcRGB));
		bc |= S_028804_COLOR_DESTBLEND(r600_translate_blend_factor(dstRGB));

		if (srcA != srcRGB || dstA != dstRGB || eqA != eqRGB) {
			bc |= S_028804_SEPARATE_ALPHA_BLEND(1);
			bc |= S_028804_ALPHA_COMB_FCN(r600_translate_blend_function(eqA));
			bc |= S_028804_ALPHA_SRCBLEND(r600_translate_blend_factor(srcA));
			bc |= S_028804_ALPHA_DESTBLEND(r600_translate_blend_factor(dstA));
		}

		rstate->states[R600_BLEND__CB_BLEND0_CONTROL + i] = bc;
		if (i == 0)
			rstate->states[R600_BLEND__CB_BLEND_CONTROL] = bc;
	}

	if (radeon_state_pm4(rstate)) {
		radeon_state_decref(rstate);
		return NULL;
	}
	return rstate;
}

static struct radeon_state *r600_ucp(struct r600_context *rctx, int clip)
{
	struct r600_screen *rscreen = rctx->screen;
	struct radeon_state *rstate;
	const struct pipe_clip_state *state = &rctx->clip->state.clip;

	rstate = radeon_state(rscreen->rw, R600_STATE_CLIP, clip);
	if (rstate == NULL)
		return NULL;

	rstate->states[R600_CLIP__PA_CL_UCP_X_0] = fui(state->ucp[clip][0]);
	rstate->states[R600_CLIP__PA_CL_UCP_Y_0] = fui(state->ucp[clip][1]);
	rstate->states[R600_CLIP__PA_CL_UCP_Z_0] = fui(state->ucp[clip][2]);
	rstate->states[R600_CLIP__PA_CL_UCP_W_0] = fui(state->ucp[clip][3]);

	if (radeon_state_pm4(rstate)) {
		radeon_state_decref(rstate);
		return NULL;
	}
	return rstate;

}

static struct radeon_state *r600_cb(struct r600_context *rctx, int cb)
{
	struct r600_screen *rscreen = rctx->screen;
	struct r600_resource_texture *rtex;
	struct r600_resource *rbuffer;
	struct radeon_state *rstate;
	const struct pipe_framebuffer_state *state = &rctx->framebuffer->state.framebuffer;
	unsigned level = state->cbufs[cb]->level;
	unsigned pitch, slice;
	unsigned color_info;
	unsigned format, swap, ntype;
	const struct util_format_description *desc;

	rstate = radeon_state(rscreen->rw, R600_STATE_CB0 + cb, 0);
	if (rstate == NULL)
		return NULL;
	rtex = (struct r600_resource_texture*)state->cbufs[cb]->texture;
	rbuffer = &rtex->resource;
	rstate->bo[0] = radeon_bo_incref(rscreen->rw, rbuffer->bo);
	rstate->bo[1] = radeon_bo_incref(rscreen->rw, rbuffer->bo);
	rstate->bo[2] = radeon_bo_incref(rscreen->rw, rbuffer->bo);
	rstate->placement[0] = RADEON_GEM_DOMAIN_GTT;
	rstate->placement[2] = RADEON_GEM_DOMAIN_GTT;
	rstate->placement[4] = RADEON_GEM_DOMAIN_GTT;
	rstate->nbo = 3;
	pitch = (rtex->pitch[level] / rtex->bpt) / 8 - 1;
	slice = (rtex->pitch[level] / rtex->bpt) * state->cbufs[cb]->height / 64 - 1;

	ntype = 0;
	desc = util_format_description(rtex->resource.base.b.format);
	if (desc->colorspace == UTIL_FORMAT_COLORSPACE_SRGB)
		ntype = V_0280A0_NUMBER_SRGB;

	format = r600_translate_colorformat(rtex->resource.base.b.format);
	swap = r600_translate_colorswap(rtex->resource.base.b.format);

	color_info = S_0280A0_FORMAT(format) |
		S_0280A0_COMP_SWAP(swap) |
		S_0280A0_BLEND_CLAMP(1) |
		S_0280A0_SOURCE_FORMAT(1) |
		S_0280A0_NUMBER_TYPE(ntype);

	rstate->states[R600_CB0__CB_COLOR0_BASE] = state->cbufs[cb]->offset >> 8;
	rstate->states[R600_CB0__CB_COLOR0_INFO] = color_info;
	rstate->states[R600_CB0__CB_COLOR0_SIZE] = S_028060_PITCH_TILE_MAX(pitch) |
						S_028060_SLICE_TILE_MAX(slice);
	rstate->states[R600_CB0__CB_COLOR0_VIEW] = 0x00000000;
	rstate->states[R600_CB0__CB_COLOR0_FRAG] = 0x00000000;
	rstate->states[R600_CB0__CB_COLOR0_TILE] = 0x00000000;
	rstate->states[R600_CB0__CB_COLOR0_MASK] = 0x00000000;
	if (radeon_state_pm4(rstate)) {
		radeon_state_decref(rstate);
		return NULL;
	}
	return rstate;
}

static struct radeon_state *r600_db(struct r600_context *rctx)
{
	struct r600_screen *rscreen = rctx->screen;
	struct r600_resource_texture *rtex;
	struct r600_resource *rbuffer;
	struct radeon_state *rstate;
	const struct pipe_framebuffer_state *state = &rctx->framebuffer->state.framebuffer;
	unsigned level;
	unsigned pitch, slice, format;

	if (state->zsbuf == NULL)
		return NULL;

	rstate = radeon_state(rscreen->rw, R600_STATE_DB, 0);
	if (rstate == NULL)
		return NULL;

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
	rstate->states[R600_DB__DB_DEPTH_BASE] = state->zsbuf->offset >> 8;
	rstate->states[R600_DB__DB_DEPTH_INFO] = S_028010_ARRAY_MODE(rtex->array_mode) |
					S_028010_FORMAT(format);
	rstate->states[R600_DB__DB_DEPTH_VIEW] = 0x00000000;
	rstate->states[R600_DB__DB_PREFETCH_LIMIT] = (state->zsbuf->height / 8) -1;
	rstate->states[R600_DB__DB_DEPTH_SIZE] = S_028000_PITCH_TILE_MAX(pitch) |
						S_028000_SLICE_TILE_MAX(slice);
	if (radeon_state_pm4(rstate)) {
		radeon_state_decref(rstate);
		return NULL;
	}
	return rstate;
}

static struct radeon_state *r600_rasterizer(struct r600_context *rctx)
{
	const struct pipe_rasterizer_state *state = &rctx->rasterizer->state.rasterizer;
	const struct pipe_framebuffer_state *fb = &rctx->framebuffer->state.framebuffer;
	const struct pipe_clip_state *clip = NULL;
	struct r600_screen *rscreen = rctx->screen;
	struct radeon_state *rstate;
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
			return NULL;
		}
	}
	offset_db_fmt_cntl |= S_028DF8_POLY_OFFSET_NEG_NUM_DB_BITS(depth);

	if (state->flatshade_first)
		prov_vtx = 0;

	rctx->flat_shade = state->flatshade;
	rstate = radeon_state(rscreen->rw, R600_STATE_RASTERIZER, 0);
	if (rstate == NULL)
		return NULL;
	rstate->states[R600_RASTERIZER__SPI_INTERP_CONTROL_0] = 0x00000001;
	if (state->sprite_coord_enable) {
		rstate->states[R600_RASTERIZER__SPI_INTERP_CONTROL_0] |=
				S_0286D4_PNT_SPRITE_ENA(1) |
				S_0286D4_PNT_SPRITE_OVRD_X(2) |
				S_0286D4_PNT_SPRITE_OVRD_Y(3) |
				S_0286D4_PNT_SPRITE_OVRD_Z(0) |
				S_0286D4_PNT_SPRITE_OVRD_W(1);
		if (state->sprite_coord_mode != PIPE_SPRITE_COORD_UPPER_LEFT) {
			rstate->states[R600_RASTERIZER__SPI_INTERP_CONTROL_0] |=
					S_0286D4_PNT_SPRITE_TOP_1(1);
		}
	}
	rstate->states[R600_RASTERIZER__PA_CL_CLIP_CNTL] = 0;
	if (clip) {
		rstate->states[R600_RASTERIZER__PA_CL_CLIP_CNTL] = S_028810_PS_UCP_MODE(3) | ((1 << clip->nr) - 1);
		rstate->states[R600_RASTERIZER__PA_CL_CLIP_CNTL] |= S_028810_ZCLIP_NEAR_DISABLE(clip->depth_clamp);
		rstate->states[R600_RASTERIZER__PA_CL_CLIP_CNTL] |= S_028810_ZCLIP_FAR_DISABLE(clip->depth_clamp);
	}
	rstate->states[R600_RASTERIZER__PA_SU_SC_MODE_CNTL] =
		S_028814_PROVOKING_VTX_LAST(prov_vtx) |
		S_028814_CULL_FRONT((state->cull_face & PIPE_FACE_FRONT) ? 1 : 0) |
		S_028814_CULL_BACK((state->cull_face & PIPE_FACE_BACK) ? 1 : 0) |
		S_028814_FACE(!state->front_ccw) |
		S_028814_POLY_OFFSET_FRONT_ENABLE(state->offset_tri) |
		S_028814_POLY_OFFSET_BACK_ENABLE(state->offset_tri) |
		S_028814_POLY_OFFSET_PARA_ENABLE(state->offset_tri);
	rstate->states[R600_RASTERIZER__PA_CL_VS_OUT_CNTL] =
			S_02881C_USE_VTX_POINT_SIZE(state->point_size_per_vertex) |
			S_02881C_VS_OUT_MISC_VEC_ENA(state->point_size_per_vertex);
	rstate->states[R600_RASTERIZER__PA_CL_NANINF_CNTL] = 0x00000000;
	/* point size 12.4 fixed point */
	tmp = (unsigned)(state->point_size * 8.0);
	rstate->states[R600_RASTERIZER__PA_SU_POINT_SIZE] = S_028A00_HEIGHT(tmp) | S_028A00_WIDTH(tmp);
	rstate->states[R600_RASTERIZER__PA_SU_POINT_MINMAX] = 0x80000000;
	rstate->states[R600_RASTERIZER__PA_SU_LINE_CNTL] = 0x00000008;
	rstate->states[R600_RASTERIZER__PA_SC_LINE_STIPPLE] = 0x00000005;
	rstate->states[R600_RASTERIZER__PA_SC_MPASS_PS_CNTL] = 0x00000000;
	rstate->states[R600_RASTERIZER__PA_SC_LINE_CNTL] = 0x00000400;
	rstate->states[R600_RASTERIZER__PA_CL_GB_VERT_CLIP_ADJ] = 0x3F800000;
	rstate->states[R600_RASTERIZER__PA_CL_GB_VERT_DISC_ADJ] = 0x3F800000;
	rstate->states[R600_RASTERIZER__PA_CL_GB_HORZ_CLIP_ADJ] = 0x3F800000;
	rstate->states[R600_RASTERIZER__PA_CL_GB_HORZ_DISC_ADJ] = 0x3F800000;
	rstate->states[R600_RASTERIZER__PA_SU_POLY_OFFSET_DB_FMT_CNTL] = offset_db_fmt_cntl;
	rstate->states[R600_RASTERIZER__PA_SU_POLY_OFFSET_CLAMP] = 0x00000000;
	rstate->states[R600_RASTERIZER__PA_SU_POLY_OFFSET_FRONT_SCALE] = fui(offset_scale);
	rstate->states[R600_RASTERIZER__PA_SU_POLY_OFFSET_FRONT_OFFSET] = fui(offset_units);
	rstate->states[R600_RASTERIZER__PA_SU_POLY_OFFSET_BACK_SCALE] = fui(offset_scale);
	rstate->states[R600_RASTERIZER__PA_SU_POLY_OFFSET_BACK_OFFSET] = fui(offset_units);
	if (radeon_state_pm4(rstate)) {
		radeon_state_decref(rstate);
		return NULL;
	}
	return rstate;
}

static struct radeon_state *r600_scissor(struct r600_context *rctx)
{
	const struct pipe_scissor_state *state = &rctx->scissor->state.scissor;
	const struct pipe_framebuffer_state *fb = &rctx->framebuffer->state.framebuffer;
	struct r600_screen *rscreen = rctx->screen;
	struct radeon_state *rstate;
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
	tl = S_028240_TL_X(minx) | S_028240_TL_Y(miny) | S_028240_WINDOW_OFFSET_DISABLE(1);
	br = S_028244_BR_X(maxx) | S_028244_BR_Y(maxy);
	rstate = radeon_state(rscreen->rw, R600_STATE_SCISSOR, 0);
	if (rstate == NULL)
		return NULL;
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
		return NULL;
	}
	return rstate;
}

static struct radeon_state *r600_viewport(struct r600_context *rctx)
{
	const struct pipe_viewport_state *state = &rctx->viewport->state.viewport;
	struct r600_screen *rscreen = rctx->screen;
	struct radeon_state *rstate;

	rstate = radeon_state(rscreen->rw, R600_STATE_VIEWPORT, 0);
	if (rstate == NULL)
		return NULL;
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
		return NULL;
	}
	return rstate;
}

static struct radeon_state *r600_dsa(struct r600_context *rctx)
{
	const struct pipe_depth_stencil_alpha_state *state = &rctx->dsa->state.dsa;
	const struct pipe_stencil_ref *stencil_ref = &rctx->stencil_ref->state.stencil_ref;
	struct r600_screen *rscreen = rctx->screen;
	unsigned db_depth_control, alpha_test_control, alpha_ref, db_shader_control;
	unsigned stencil_ref_mask, stencil_ref_mask_bf;
	struct r600_shader *rshader;
	struct radeon_state *rstate;
	int i;

	if (rctx->ps_shader == NULL) {
		return NULL;
	}
	rstate = radeon_state(rscreen->rw, R600_STATE_DSA, 0);
	if (rstate == NULL)
		return NULL;

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

	rstate->states[R600_DSA__DB_STENCIL_CLEAR] = 0x00000000;
	rstate->states[R600_DSA__DB_DEPTH_CLEAR] = 0x3F800000;
	rstate->states[R600_DSA__SX_ALPHA_TEST_CONTROL] = alpha_test_control;
	rstate->states[R600_DSA__DB_STENCILREFMASK] = stencil_ref_mask;
	rstate->states[R600_DSA__DB_STENCILREFMASK_BF] = stencil_ref_mask_bf;
	rstate->states[R600_DSA__SX_ALPHA_REF] = alpha_ref;
	rstate->states[R600_DSA__SPI_FOG_FUNC_SCALE] = 0x00000000;
	rstate->states[R600_DSA__SPI_FOG_FUNC_BIAS] = 0x00000000;
	rstate->states[R600_DSA__SPI_FOG_CNTL] = 0x00000000;
	rstate->states[R600_DSA__DB_DEPTH_CONTROL] = db_depth_control;
	rstate->states[R600_DSA__DB_SHADER_CONTROL] = db_shader_control;
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

static INLINE u32 S_FIXED(float value, u32 frac_bits)
{
	return value * (1 << frac_bits);
}

static struct radeon_state *r600_sampler(struct r600_context *rctx,
				const struct pipe_sampler_state *state,
				unsigned id)
{
	struct r600_screen *rscreen = rctx->screen;
	struct radeon_state *rstate;

	rstate = radeon_state_shader(rscreen->rw, R600_STATE_SAMPLER, id, R600_SHADER_PS);
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
			S_03C004_MIN_LOD(S_FIXED(CLAMP(state->min_lod, 0, 15), 6)) |
			S_03C004_MAX_LOD(S_FIXED(CLAMP(state->max_lod, 0, 15), 6)) |
			S_03C004_LOD_BIAS(S_FIXED(CLAMP(state->lod_bias, -16, 16), 6));
	rstate->states[R600_PS_SAMPLER__SQ_TEX_SAMPLER_WORD2_0] = S_03C008_TYPE(1);
	if (radeon_state_pm4(rstate)) {
		radeon_state_decref(rstate);
		return NULL;
	}
	return rstate;
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
	case PIPE_TEXTURE_RECT:
		return V_038000_SQ_TEX_DIM_2D;
	case PIPE_TEXTURE_3D:
		return V_038000_SQ_TEX_DIM_3D;
	case PIPE_TEXTURE_CUBE:
		return V_038000_SQ_TEX_DIM_CUBEMAP;
	}
}

static struct radeon_state *r600_resource(struct pipe_context *ctx,
					const struct pipe_sampler_view *view,
					unsigned id)
{
	struct r600_context *rctx = r600_context(ctx);
	struct r600_screen *rscreen = rctx->screen;
	const struct util_format_description *desc;
	struct r600_resource_texture *tmp;
	struct r600_resource *rbuffer;
	struct radeon_state *rstate;
	unsigned format;
	uint32_t word4 = 0, yuv_format = 0, pitch = 0;
	unsigned char swizzle[4], array_mode = 0, tile_type = 0;
	int r;

	swizzle[0] = view->swizzle_r;
	swizzle[1] = view->swizzle_g;
	swizzle[2] = view->swizzle_b;
	swizzle[3] = view->swizzle_a;
	format = r600_translate_texformat(view->texture->format,
					  swizzle,
					  &word4, &yuv_format);
	if (format == ~0)
		return NULL;
	desc = util_format_description(view->texture->format);
	if (desc == NULL) {
		R600_ERR("unknow format %d\n", view->texture->format);
		return NULL;
	}
	rstate = radeon_state_shader(rscreen->rw, R600_STATE_RESOURCE, id, R600_SHADER_PS);
	if (rstate == NULL) {
		return NULL;
	}
	tmp = (struct r600_resource_texture*)view->texture;
	rbuffer = &tmp->resource;
	if (tmp->depth) {
		r = r600_texture_from_depth(ctx, tmp, view->first_level);
		if (r) {
			return NULL;
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
	rstate->states[R600_PS_RESOURCE__RESOURCE0_WORD0] =
			S_038000_DIM(r600_tex_dim(view->texture->target)) |
			S_038000_TILE_MODE(array_mode) |
			S_038000_TILE_TYPE(tile_type) |
			S_038000_PITCH((pitch / 8) - 1) |
			S_038000_TEX_WIDTH(view->texture->width0 - 1);
	rstate->states[R600_PS_RESOURCE__RESOURCE0_WORD1] =
			S_038004_TEX_HEIGHT(view->texture->height0 - 1) |
			S_038004_TEX_DEPTH(view->texture->depth0 - 1) |
			S_038004_DATA_FORMAT(format);
	rstate->states[R600_PS_RESOURCE__RESOURCE0_WORD2] = tmp->offset[0] >> 8;
	rstate->states[R600_PS_RESOURCE__RESOURCE0_WORD3] = tmp->offset[1] >> 8;
	rstate->states[R600_PS_RESOURCE__RESOURCE0_WORD4] =
		        word4 | 
			S_038010_NUM_FORMAT_ALL(V_038010_SQ_NUM_FORMAT_NORM) |
			S_038010_SRF_MODE_ALL(V_038010_SFR_MODE_NO_ZERO) |
			S_038010_REQUEST_SIZE(1) |
			S_038010_BASE_LEVEL(view->first_level);
	rstate->states[R600_PS_RESOURCE__RESOURCE0_WORD5] =
			S_038014_LAST_LEVEL(view->last_level) |
			S_038014_BASE_ARRAY(0) |
			S_038014_LAST_ARRAY(0);
	rstate->states[R600_PS_RESOURCE__RESOURCE0_WORD6] =
			S_038018_TYPE(V_038010_SQ_TEX_VTX_VALID_TEXTURE);
	if (radeon_state_pm4(rstate)) {
		radeon_state_decref(rstate);
		return NULL;
	}
	return rstate;
}

static struct radeon_state *r600_cb_cntl(struct r600_context *rctx)
{
	struct r600_screen *rscreen = rctx->screen;
	struct radeon_state *rstate;
	const struct pipe_blend_state *pbs = &rctx->blend->state.blend;
	int nr_cbufs = rctx->framebuffer->state.framebuffer.nr_cbufs;
	uint32_t color_control, target_mask, shader_mask;
	int i;

	target_mask = 0;
	shader_mask = 0;
	color_control = S_028808_PER_MRT_BLEND(1);

	for (i = 0; i < nr_cbufs; i++) {
		shader_mask |= 0xf << (i * 4);
	}

	if (pbs->logicop_enable) {
		color_control |= (pbs->logicop_func) << 16;
	} else {
		color_control |= (0xcc << 16);
	}

	if (pbs->independent_blend_enable) {
		for (i = 0; i < nr_cbufs; i++) {
			if (pbs->rt[i].blend_enable) {
				color_control |= S_028808_TARGET_BLEND_ENABLE(1 << i);
			}
			target_mask |= (pbs->rt[i].colormask << (4 * i));
		}
	} else {
		for (i = 0; i < nr_cbufs; i++) {
			if (pbs->rt[0].blend_enable) {
				color_control |= S_028808_TARGET_BLEND_ENABLE(1 << i);
			}
			target_mask |= (pbs->rt[0].colormask << (4 * i));
		}
	}
	rstate = radeon_state(rscreen->rw, R600_STATE_CB_CNTL, 0);
	rstate->states[R600_CB_CNTL__CB_SHADER_MASK] = shader_mask;
	rstate->states[R600_CB_CNTL__CB_TARGET_MASK] = target_mask;
	rstate->states[R600_CB_CNTL__CB_COLOR_CONTROL] = color_control;
	rstate->states[R600_CB_CNTL__PA_SC_AA_CONFIG] = 0x00000000;
	rstate->states[R600_CB_CNTL__PA_SC_AA_SAMPLE_LOCS_MCTX] = 0x00000000;
	rstate->states[R600_CB_CNTL__PA_SC_AA_SAMPLE_LOCS_8S_WD1_MCTX] = 0x00000000;
	rstate->states[R600_CB_CNTL__CB_CLRCMP_CONTROL] = 0x01000000;
	rstate->states[R600_CB_CNTL__CB_CLRCMP_SRC] = 0x00000000;
	rstate->states[R600_CB_CNTL__CB_CLRCMP_DST] = 0x000000FF;
	rstate->states[R600_CB_CNTL__CB_CLRCMP_MSK] = 0xFFFFFFFF;
	rstate->states[R600_CB_CNTL__PA_SC_AA_MASK] = 0xFFFFFFFF;
	if (radeon_state_pm4(rstate)) {
		radeon_state_decref(rstate);
		return NULL;
	}
	return rstate;
}

int r600_context_hw_states(struct pipe_context *ctx)
{
	struct r600_context *rctx = r600_context(ctx);
	unsigned i;
	int r;
	int nr_cbufs = rctx->framebuffer->state.framebuffer.nr_cbufs;
	int ucp_nclip = 0;

	if (rctx->clip) 
		ucp_nclip = rctx->clip->state.clip.nr;

	/* free previous TODO determine what need to be updated, what
	 * doesn't
	 */
	//radeon_state_decref(rctx->hw_states.config);
	rctx->hw_states.cb_cntl = radeon_state_decref(rctx->hw_states.cb_cntl);
	rctx->hw_states.db = radeon_state_decref(rctx->hw_states.db);
	rctx->hw_states.rasterizer = radeon_state_decref(rctx->hw_states.rasterizer);
	rctx->hw_states.scissor = radeon_state_decref(rctx->hw_states.scissor);
	rctx->hw_states.dsa = radeon_state_decref(rctx->hw_states.dsa);
	rctx->hw_states.blend = radeon_state_decref(rctx->hw_states.blend);
	rctx->hw_states.viewport = radeon_state_decref(rctx->hw_states.viewport);
	for (i = 0; i < 8; i++) {
		rctx->hw_states.cb[i] = radeon_state_decref(rctx->hw_states.cb[i]);
	}
	for (i = 0; i < 6; i++) {
		rctx->hw_states.ucp[i] = radeon_state_decref(rctx->hw_states.ucp[i]);
	}
	for (i = 0; i < rctx->hw_states.ps_nresource; i++) {
		radeon_state_decref(rctx->hw_states.ps_resource[i]);
		rctx->hw_states.ps_resource[i] = NULL;
	}
	rctx->hw_states.ps_nresource = 0;
	for (i = 0; i < rctx->hw_states.ps_nsampler; i++) {
		radeon_state_decref(rctx->hw_states.ps_sampler[i]);
		rctx->hw_states.ps_sampler[i] = NULL;
	}
	rctx->hw_states.ps_nsampler = 0;

	/* build new states */
	rctx->hw_states.rasterizer = r600_rasterizer(rctx);
	rctx->hw_states.scissor = r600_scissor(rctx);
	rctx->hw_states.dsa = r600_dsa(rctx);
	rctx->hw_states.blend = r600_blend(rctx);
	rctx->hw_states.viewport = r600_viewport(rctx);
	for (i = 0; i < nr_cbufs; i++) {
		rctx->hw_states.cb[i] = r600_cb(rctx, i);
	}
	for (i = 0; i < ucp_nclip; i++) {
		rctx->hw_states.ucp[i] = r600_ucp(rctx, i);
	}
	rctx->hw_states.db = r600_db(rctx);
	rctx->hw_states.cb_cntl = r600_cb_cntl(rctx);

	for (i = 0; i < rctx->ps_nsampler; i++) {
		if (rctx->ps_sampler[i]) {
			rctx->hw_states.ps_sampler[i] = r600_sampler(rctx,
							&rctx->ps_sampler[i]->state.sampler,
							i);
		}
	}
	rctx->hw_states.ps_nsampler = rctx->ps_nsampler;
	for (i = 0; i < rctx->ps_nsampler_view; i++) {
		if (rctx->ps_sampler_view[i]) {
			rctx->hw_states.ps_resource[i] = r600_resource(ctx,
							&rctx->ps_sampler_view[i]->state.sampler_view,
							i);
		}
	}
	rctx->hw_states.ps_nresource = rctx->ps_nsampler_view;

	/* bind states */
	for (i = 0; i < ucp_nclip; i++) {
		r = radeon_draw_set(rctx->draw, rctx->hw_states.ucp[i]);
		if (r)
			return r;
	}
	r = radeon_draw_set(rctx->draw, rctx->hw_states.db);
	if (r)
		return r;
	r = radeon_draw_set(rctx->draw, rctx->hw_states.rasterizer);
	if (r)
		return r;
	r = radeon_draw_set(rctx->draw, rctx->hw_states.scissor);
	if (r)
		return r;
	r = radeon_draw_set(rctx->draw, rctx->hw_states.dsa);
	if (r)
		return r;
	r = radeon_draw_set(rctx->draw, rctx->hw_states.blend);
	if (r)
		return r;
	r = radeon_draw_set(rctx->draw, rctx->hw_states.viewport);
	if (r)
		return r;
	for (i = 0; i < nr_cbufs; i++) {
		r = radeon_draw_set(rctx->draw, rctx->hw_states.cb[i]);
		if (r)
			return r;
	}
	r = radeon_draw_set(rctx->draw, rctx->hw_states.config);
	if (r)
		return r;
	r = radeon_draw_set(rctx->draw, rctx->hw_states.cb_cntl);
	if (r)
		return r;
	for (i = 0; i < rctx->hw_states.ps_nresource; i++) {
		if (rctx->hw_states.ps_resource[i]) {
			r = radeon_draw_set(rctx->draw, rctx->hw_states.ps_resource[i]);
			if (r)
				return r;
		}
	}
	for (i = 0; i < rctx->hw_states.ps_nsampler; i++) {
		if (rctx->hw_states.ps_sampler[i]) {
			r = radeon_draw_set(rctx->draw, rctx->hw_states.ps_sampler[i]);
			if (r)
				return r;
		}
	}
	return 0;
}

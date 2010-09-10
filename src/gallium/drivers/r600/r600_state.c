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
#include "util/u_pack_color.h"
#include "r600_screen.h"
#include "r600_context.h"
#include "r600_resource.h"
#include "r600d.h"
#include "r600_state_inlines.h"

static struct r600_context_state *r600_new_context_state(unsigned type)
{
	struct r600_context_state *rstate = CALLOC_STRUCT(r600_context_state);
	if (rstate == NULL)
		return NULL;
	rstate->type = type;
	rstate->refcount = 1;
	return rstate;
}

static void *r600_create_blend_state(struct pipe_context *ctx,
					const struct pipe_blend_state *state)
{
	struct r600_context *rctx = r600_context(ctx);
	struct r600_context_state *rstate;

	rstate = r600_new_context_state(pipe_blend_type);
	rstate->state.blend = *state;
	rctx->vtbl->blend(rctx, &rstate->rstate[0], &rstate->state.blend);
	
	return rstate;
}

static void *r600_create_dsa_state(struct pipe_context *ctx,
				   const struct pipe_depth_stencil_alpha_state *state)
{
	struct r600_context_state *rstate;

	rstate = r600_new_context_state(pipe_dsa_type);
	rstate->state.dsa = *state;
	return rstate;
}

static void *r600_create_rs_state(struct pipe_context *ctx,
					const struct pipe_rasterizer_state *state)
{
	struct r600_context_state *rstate;

	rstate = r600_new_context_state(pipe_rasterizer_type);
	rstate->state.rasterizer = *state;
	return rstate;
}

static void *r600_create_sampler_state(struct pipe_context *ctx,
					const struct pipe_sampler_state *state)
{
	struct r600_context *rctx = r600_context(ctx);
	struct r600_context_state *rstate;

	rstate = r600_new_context_state(pipe_sampler_type);
	rstate->state.sampler = *state;
	rctx->vtbl->sampler(rctx, &rstate->rstate[0], &rstate->state.sampler, 0);
	rctx->vtbl->sampler_border(rctx, &rstate->rstate[1], &rstate->state.sampler, 0);
	return rstate;
}

static void r600_remove_sampler_view(struct r600_shader_sampler_states *sampler,
				     struct r600_context_state *rstate)
{
	int i, j;
	
	for (i = 0; i < sampler->nview; i++) {
		for (j = 0; j < rstate->nrstate; j++) {
			if (sampler->view[i] == &rstate->rstate[j])
				sampler->view[i] = NULL;
		}
	}
}
static void r600_sampler_view_destroy(struct pipe_context *ctx,
				      struct pipe_sampler_view *state)
{
	struct r600_context_state *rstate = (struct r600_context_state *)state;
	struct r600_context *rctx = r600_context(ctx);

	/* need to search list of vs/ps sampler views and remove it from any - uggh */
	r600_remove_sampler_view(&rctx->ps_sampler, rstate);
	r600_remove_sampler_view(&rctx->vs_sampler, rstate);
	r600_context_state_decref(rstate);
}

static struct pipe_sampler_view *r600_create_sampler_view(struct pipe_context *ctx,
							struct pipe_resource *texture,
							const struct pipe_sampler_view *state)
{
	struct r600_context_state *rstate;
	struct r600_context *rctx = r600_context(ctx);

	rstate = r600_new_context_state(pipe_sampler_view_type);
	rstate->state.sampler_view = *state;
	rstate->state.sampler_view.texture = NULL;
	pipe_reference(NULL, &texture->reference);
	rstate->state.sampler_view.texture = texture;
	rstate->state.sampler_view.reference.count = 1;
	rstate->state.sampler_view.context = ctx;
	rctx->vtbl->resource(ctx, &rstate->rstate[0], &rstate->state.sampler_view, 0);
	return &rstate->state.sampler_view;
}

static void r600_set_sampler_view(struct pipe_context *ctx,
				  unsigned count,
				  struct pipe_sampler_view **views,
				  struct r600_shader_sampler_states *sampler,
				  unsigned shader_id)
{
	struct r600_context *rctx = r600_context(ctx);
	struct r600_context_state *rstate;
	unsigned i;

	for (i = 0; i < sampler->nview; i++) {
		radeon_draw_unbind(&rctx->draw, sampler->view[i]);
	}

	for (i = 0; i < count; i++) {
		rstate = (struct r600_context_state *)views[i];
		if (rstate) {
			rstate->nrstate = 0;
		}
	}
	for (i = 0; i < count; i++) {
		rstate = (struct r600_context_state *)views[i];
		if (rstate) {
			if (rstate->nrstate >= R600_MAX_RSTATE)
				continue;
			if (rstate->nrstate) {
				memcpy(&rstate->rstate[rstate->nrstate], &rstate->rstate[0], sizeof(struct radeon_state));
			}
			radeon_state_convert(&rstate->rstate[rstate->nrstate], R600_STATE_RESOURCE, i, shader_id);
			sampler->view[i] = &rstate->rstate[rstate->nrstate];
			rstate->nrstate++;
		}
	}
	sampler->nview = count;
}

static void r600_set_ps_sampler_view(struct pipe_context *ctx,
					unsigned count,
					struct pipe_sampler_view **views)
{
	struct r600_context *rctx = r600_context(ctx);
	r600_set_sampler_view(ctx, count, views, &rctx->ps_sampler, R600_SHADER_PS);
}

static void r600_set_vs_sampler_view(struct pipe_context *ctx,
					unsigned count,
					struct pipe_sampler_view **views)
{
	struct r600_context *rctx = r600_context(ctx);
	r600_set_sampler_view(ctx, count, views, &rctx->vs_sampler, R600_SHADER_VS);
}

static void *r600_create_shader_state(struct pipe_context *ctx,
					const struct pipe_shader_state *state)
{
	struct r600_context *rctx = r600_context(ctx);
	struct r600_context_state *rstate;
	int r;

	rstate = r600_new_context_state(pipe_shader_type);
	rstate->state.shader = *state;
	r =  r600_pipe_shader_create(&rctx->context, rstate, rstate->state.shader.tokens);
	if (r) {
		r600_context_state_decref(rstate);
		return NULL;
	}
	return rstate;
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

static void r600_bind_rasterizer_state(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = r600_context(ctx);
	struct r600_context_state *rstate = (struct r600_context_state *)state;

	if (state == NULL)
		return;
	rctx->rasterizer = r600_context_state_decref(rctx->rasterizer);
	rctx->rasterizer = r600_context_state_incref(rstate);
}

static void r600_bind_blend_state(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = r600_context(ctx);
	struct r600_context_state *rstate = (struct r600_context_state *)state;

	if (state == NULL)
		return;
	rctx->blend = r600_context_state_decref(rctx->blend);
	rctx->blend = r600_context_state_incref(rstate);

}

static void r600_bind_dsa_state(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = r600_context(ctx);
	struct r600_context_state *rstate = (struct r600_context_state *)state;

	if (state == NULL)
		return;
	rctx->dsa = r600_context_state_decref(rctx->dsa);
	rctx->dsa = r600_context_state_incref(rstate);
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

static void r600_bind_sampler_shader(struct pipe_context *ctx,
				     unsigned count, void **states,
				     struct r600_shader_sampler_states *sampler, unsigned shader_id)
{
	struct r600_context *rctx = r600_context(ctx);
	struct r600_context_state *rstate;
	unsigned i;

	for (i = 0; i < sampler->nsampler; i++) {
		radeon_draw_unbind(&rctx->draw, sampler->sampler[i]);
	}
	for (i = 0; i < sampler->nborder; i++) {
		radeon_draw_unbind(&rctx->draw, sampler->border[i]);
	}
	for (i = 0; i < count; i++) {
		rstate = (struct r600_context_state *)states[i];
		if (rstate) {
			rstate->nrstate = 0;
		}
	}
	for (i = 0; i < count; i++) {
		rstate = (struct r600_context_state *)states[i];
		if (rstate) {
			if (rstate->nrstate >= R600_MAX_RSTATE)
				continue;
			if (rstate->nrstate) {
				memcpy(&rstate->rstate[rstate->nrstate], &rstate->rstate[0], sizeof(struct radeon_state));
				memcpy(&rstate->rstate[rstate->nrstate+1], &rstate->rstate[1], sizeof(struct radeon_state));
			}
			radeon_state_convert(&rstate->rstate[rstate->nrstate], R600_STATE_SAMPLER, i, shader_id);
			radeon_state_convert(&rstate->rstate[rstate->nrstate + 1], R600_STATE_SAMPLER_BORDER, i, shader_id);
			sampler->sampler[i] = &rstate->rstate[rstate->nrstate];
			sampler->border[i] = &rstate->rstate[rstate->nrstate + 1];
			rstate->nrstate += 2;
		}
	}
	sampler->nsampler = count;
	sampler->nborder = count;
}

static void r600_bind_ps_sampler(struct pipe_context *ctx,
					unsigned count, void **states)
{
	struct r600_context *rctx = r600_context(ctx);
	r600_bind_sampler_shader(ctx, count, states, &rctx->ps_sampler, R600_SHADER_PS);	
}

static void r600_bind_vs_sampler(struct pipe_context *ctx,
					unsigned count, void **states)
{
	struct r600_context *rctx = r600_context(ctx);
	r600_bind_sampler_shader(ctx, count, states, &rctx->vs_sampler, R600_SHADER_VS);	
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

	r600_context_state_decref(rctx->clip);

	rstate = r600_new_context_state(pipe_clip_type);
	rstate->state.clip = *state;
	rctx->vtbl->ucp(rctx, &rstate->rstate[0], &rstate->state.clip);
	rctx->clip = rstate;
}

static void r600_set_framebuffer_state(struct pipe_context *ctx,
					const struct pipe_framebuffer_state *state)
{
	struct r600_context *rctx = r600_context(ctx);
	struct r600_context_state *rstate;
	int i;

	r600_context_state_decref(rctx->framebuffer);

	rstate = r600_new_context_state(pipe_framebuffer_type);
	rstate->state.framebuffer = *state;
	for (i = 0; i < rstate->state.framebuffer.nr_cbufs; i++) {
		pipe_reference(NULL, &state->cbufs[i]->reference);
	}
	pipe_reference(NULL, &state->zsbuf->reference);
	rctx->framebuffer = rstate;
	for (i = 0; i < state->nr_cbufs; i++) {
		rctx->vtbl->cb(rctx, &rstate->rstate[i+1], state, i);
	}
	if (state->zsbuf) {
		rctx->vtbl->db(rctx, &rstate->rstate[0], state);
	}
	return;
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

	r600_context_state_decref(rctx->scissor);

	rstate = r600_new_context_state(pipe_scissor_type);
	rstate->state.scissor = *state;
	rctx->scissor = rstate;
}

static void r600_set_stencil_ref(struct pipe_context *ctx,
				const struct pipe_stencil_ref *state)
{
	struct r600_context *rctx = r600_context(ctx);
	struct r600_context_state *rstate;

	r600_context_state_decref(rctx->stencil_ref);

	rstate = r600_new_context_state(pipe_stencil_ref_type);
	rstate->state.stencil_ref = *state;
	rctx->stencil_ref = rstate;
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

	r600_context_state_decref(rctx->viewport);

	rstate = r600_new_context_state(pipe_viewport_type);
	rstate->state.viewport = *state;
	rctx->vtbl->viewport(rctx, &rstate->rstate[0], &rstate->state.viewport);
	rctx->viewport = rstate;
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
	rctx->context.bind_blend_state = r600_bind_blend_state;
	rctx->context.bind_depth_stencil_alpha_state = r600_bind_dsa_state;
	rctx->context.bind_fragment_sampler_states = r600_bind_ps_sampler;
	rctx->context.bind_fs_state = r600_bind_ps_shader;
	rctx->context.bind_rasterizer_state = r600_bind_rasterizer_state;
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

	if (rctx->screen->chip_class == EVERGREEN)
		rctx->context.set_constant_buffer = eg_set_constant_buffer;
	else if (rctx->screen->use_mem_constant)
		rctx->context.set_constant_buffer = r600_set_constant_buffer_mem;
	else
		rctx->context.set_constant_buffer = r600_set_constant_buffer_file;

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
	radeon_state_fini(&rstate->rstate[0]);
	FREE(rstate);
	return NULL;
}

static void r600_bind_shader_sampler(struct r600_context *rctx, struct r600_shader_sampler_states *sampler)
{
	int i;

	for (i = 0; i < sampler->nsampler; i++) {
		if (sampler->sampler[i])
			radeon_draw_bind(&rctx->draw, sampler->sampler[i]);
	}

	for (i = 0; i < sampler->nborder; i++) {
		if (sampler->border[i])
			radeon_draw_bind(&rctx->draw, sampler->border[i]);
	}

	for (i = 0; i < sampler->nview; i++) {
		if (sampler->view[i])
			radeon_draw_bind(&rctx->draw, sampler->view[i]);
	}
}


static int setup_cb_flush(struct r600_context *rctx, struct radeon_state *flush)
{
	struct r600_screen *rscreen = rctx->screen;
	struct r600_resource_texture *rtex;
	struct r600_resource *rbuffer;
	struct pipe_surface *surf;
	int i;

	radeon_state_init(flush, rscreen->rw, R600_STATE_CB_FLUSH, 0, 0);

	for (i = 0; i < rctx->framebuffer->state.framebuffer.nr_cbufs; i++) {
		surf = rctx->framebuffer->state.framebuffer.cbufs[i];
		
		rtex = (struct r600_resource_texture*)surf->texture;
		rbuffer = &rtex->resource;
		/* just need to the bo to the flush list */
		flush->bo[i] = radeon_bo_incref(rscreen->rw, rbuffer->bo);
		flush->placement[i] = RADEON_GEM_DOMAIN_VRAM;
	}
	flush->nbo = rctx->framebuffer->state.framebuffer.nr_cbufs;
	return radeon_state_pm4(flush);
}

static int setup_db_flush(struct r600_context *rctx, struct radeon_state *flush)
{
	struct r600_screen *rscreen = rctx->screen;
	struct r600_resource_texture *rtex;
	struct r600_resource *rbuffer;
	struct pipe_surface *surf;

	surf = rctx->framebuffer->state.framebuffer.zsbuf;

	if (!surf)
		return 0;
		
	radeon_state_init(flush, rscreen->rw, R600_STATE_DB_FLUSH, 0, 0);
	rtex = (struct r600_resource_texture*)surf->texture;
	rbuffer = &rtex->resource;
	/* just need to the bo to the flush list */
	flush->bo[0] = radeon_bo_incref(rscreen->rw, rbuffer->bo);
	flush->placement[0] = RADEON_GEM_DOMAIN_VRAM;

	flush->nbo = 1;
	return radeon_state_pm4(flush);
}

int r600_context_hw_states(struct pipe_context *ctx)
{
	struct r600_context *rctx = r600_context(ctx);
	unsigned i;
	
	/* build new states */
	rctx->vtbl->rasterizer(rctx, &rctx->hw_states.rasterizer);
	rctx->vtbl->scissor(rctx, &rctx->hw_states.scissor);
	rctx->vtbl->dsa(rctx, &rctx->hw_states.dsa);
	rctx->vtbl->cb_cntl(rctx, &rctx->hw_states.cb_cntl);
							       
	/* setup flushes */
	setup_db_flush(rctx, &rctx->hw_states.db_flush);
	setup_cb_flush(rctx, &rctx->hw_states.cb_flush);

	/* bind states */
	radeon_draw_bind(&rctx->draw, &rctx->config);

	radeon_draw_bind(&rctx->draw, &rctx->hw_states.rasterizer);
	radeon_draw_bind(&rctx->draw, &rctx->hw_states.scissor);
	radeon_draw_bind(&rctx->draw, &rctx->hw_states.dsa);
	radeon_draw_bind(&rctx->draw, &rctx->hw_states.cb_cntl);

	radeon_draw_bind(&rctx->draw, &rctx->hw_states.db_flush);
	radeon_draw_bind(&rctx->draw, &rctx->hw_states.cb_flush);

	radeon_draw_bind(&rctx->draw, &rctx->hw_states.db_flush);
	radeon_draw_bind(&rctx->draw, &rctx->hw_states.cb_flush);

	if (rctx->viewport) {
		radeon_draw_bind(&rctx->draw, &rctx->viewport->rstate[0]);
	}
	if (rctx->blend) {
		radeon_draw_bind(&rctx->draw, &rctx->blend->rstate[0]);
	}
	if (rctx->clip) {
		radeon_draw_bind(&rctx->draw, &rctx->clip->rstate[0]);
	}
	for (i = 0; i < rctx->framebuffer->state.framebuffer.nr_cbufs; i++) {
		radeon_draw_bind(&rctx->draw, &rctx->framebuffer->rstate[i+1]);
	}
	if (rctx->framebuffer->state.framebuffer.zsbuf) {
		radeon_draw_bind(&rctx->draw, &rctx->framebuffer->rstate[0]);
	}

	r600_bind_shader_sampler(rctx, &rctx->vs_sampler);
	r600_bind_shader_sampler(rctx, &rctx->ps_sampler);

	return 0;
}

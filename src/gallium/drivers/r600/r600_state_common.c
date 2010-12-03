/*
 * Copyright 2010 Red Hat Inc.
 *           2010 Jerome Glisse
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
 * Authors: Dave Airlie <airlied@redhat.com>
 *          Jerome Glisse <jglisse@redhat.com>
 */
#include <util/u_memory.h>
#include <util/u_format.h>
#include <pipebuffer/pb_buffer.h>
#include "r600_pipe.h"

/* common state between evergreen and r600 */
void r600_bind_blend_state(struct pipe_context *ctx, void *state)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct r600_pipe_blend *blend = (struct r600_pipe_blend *)state;
	struct r600_pipe_state *rstate;

	if (state == NULL)
		return;
	rstate = &blend->rstate;
	rctx->states[rstate->id] = rstate;
	rctx->cb_target_mask = blend->cb_target_mask;
	r600_context_pipe_state_set(&rctx->ctx, rstate);
}

void r600_bind_rs_state(struct pipe_context *ctx, void *state)
{
	struct r600_pipe_rasterizer *rs = (struct r600_pipe_rasterizer *)state;
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;

	if (state == NULL)
		return;

	rctx->flatshade = rs->flatshade;
	rctx->sprite_coord_enable = rs->sprite_coord_enable;
	rctx->rasterizer = rs;

	rctx->states[rs->rstate.id] = &rs->rstate;
	r600_context_pipe_state_set(&rctx->ctx, &rs->rstate);

	if (rctx->family >= CHIP_CEDAR) {
		evergreen_polygon_offset_update(rctx);
	} else {
		r600_polygon_offset_update(rctx);
	}
}

void r600_delete_rs_state(struct pipe_context *ctx, void *state)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct r600_pipe_rasterizer *rs = (struct r600_pipe_rasterizer *)state;

	if (rctx->rasterizer == rs) {
		rctx->rasterizer = NULL;
	}
	if (rctx->states[rs->rstate.id] == &rs->rstate) {
		rctx->states[rs->rstate.id] = NULL;
	}
	free(rs);
}

void r600_sampler_view_destroy(struct pipe_context *ctx,
			       struct pipe_sampler_view *state)
{
	struct r600_pipe_sampler_view *resource = (struct r600_pipe_sampler_view *)state;

	pipe_resource_reference(&state->texture, NULL);
	FREE(resource);
}

void r600_bind_state(struct pipe_context *ctx, void *state)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct r600_pipe_state *rstate = (struct r600_pipe_state *)state;

	if (state == NULL)
		return;
	rctx->states[rstate->id] = rstate;
	r600_context_pipe_state_set(&rctx->ctx, rstate);
}

void r600_delete_state(struct pipe_context *ctx, void *state)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct r600_pipe_state *rstate = (struct r600_pipe_state *)state;

	if (rctx->states[rstate->id] == rstate) {
		rctx->states[rstate->id] = NULL;
	}
	for (int i = 0; i < rstate->nregs; i++) {
		r600_bo_reference(rctx->radeon, &rstate->regs[i].bo, NULL);
	}
	free(rstate);
}

void r600_bind_vertex_elements(struct pipe_context *ctx, void *state)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct r600_vertex_element *v = (struct r600_vertex_element*)state;

	rctx->vertex_elements = v;
	if (v) {
//		rctx->vs_rebuild = TRUE;
	}
}

void r600_delete_vertex_element(struct pipe_context *ctx, void *state)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;

	FREE(state);

	if (rctx->vertex_elements == state)
		rctx->vertex_elements = NULL;
}


void r600_set_index_buffer(struct pipe_context *ctx,
			   const struct pipe_index_buffer *ib)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;

	if (ib) {
		pipe_resource_reference(&rctx->index_buffer.buffer, ib->buffer);
		memcpy(&rctx->index_buffer, ib, sizeof(rctx->index_buffer));
	} else {
		pipe_resource_reference(&rctx->index_buffer.buffer, NULL);
		memset(&rctx->index_buffer, 0, sizeof(rctx->index_buffer));
	}

	/* TODO make this more like a state */
}

void r600_set_vertex_buffers(struct pipe_context *ctx, unsigned count,
			     const struct pipe_vertex_buffer *buffers)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct pipe_vertex_buffer *vbo;
	unsigned max_index = (unsigned)-1;

	for (int i = 0; i < rctx->nvertex_buffer; i++) {
		pipe_resource_reference(&rctx->vertex_buffer[i].buffer, NULL);
	}
	memcpy(rctx->vertex_buffer, buffers, sizeof(struct pipe_vertex_buffer) * count);

	for (int i = 0; i < count; i++) {
		vbo = (struct pipe_vertex_buffer*)&buffers[i];

		rctx->vertex_buffer[i].buffer = NULL;
		if (r600_buffer_is_user_buffer(buffers[i].buffer))
			rctx->any_user_vbs = TRUE;
		pipe_resource_reference(&rctx->vertex_buffer[i].buffer, buffers[i].buffer);

		if (vbo->max_index == ~0) {
			if (!vbo->stride)
				vbo->max_index = 1;
			else
				vbo->max_index = (vbo->buffer->width0 - vbo->buffer_offset) / vbo->stride;
		}
		max_index = MIN2(vbo->max_index, max_index);
	}
	rctx->nvertex_buffer = count;
	rctx->vb_max_index = max_index;
}


#define FORMAT_REPLACE(what, withwhat) \
	case PIPE_FORMAT_##what: *format = PIPE_FORMAT_##withwhat; break

void *r600_create_vertex_elements(struct pipe_context *ctx,
				  unsigned count,
				  const struct pipe_vertex_element *elements)
{
	struct r600_vertex_element *v = CALLOC_STRUCT(r600_vertex_element);
	int i;
	enum pipe_format *format;

	assert(count < 32);
	if (!v)
		return NULL;

	v->count = count;
	memcpy(v->elements, elements, count * sizeof(struct pipe_vertex_element));

	for (i = 0; i < count; i++) {
		v->hw_format[i] = v->elements[i].src_format;
		format = &v->hw_format[i];

		switch (*format) {
		FORMAT_REPLACE(R64_FLOAT,           R32_FLOAT);
		FORMAT_REPLACE(R64G64_FLOAT,        R32G32_FLOAT);
		FORMAT_REPLACE(R64G64B64_FLOAT,     R32G32B32_FLOAT);
		FORMAT_REPLACE(R64G64B64A64_FLOAT,  R32G32B32A32_FLOAT);
		default:;
		}
		v->incompatible_layout =
			v->incompatible_layout ||
			v->elements[i].src_format != v->hw_format[i] ||
			v->elements[i].src_offset % 4 != 0;

		v->hw_format_size[i] = align(util_format_get_blocksize(v->hw_format[i]), 4);
	}

	return v;
}

void *r600_create_shader_state(struct pipe_context *ctx,
			       const struct pipe_shader_state *state)
{
	struct r600_pipe_shader *shader =  CALLOC_STRUCT(r600_pipe_shader);
	int r;

	r =  r600_pipe_shader_create(ctx, shader, state->tokens);
	if (r) {
		return NULL;
	}
	return shader;
}

void r600_bind_ps_shader(struct pipe_context *ctx, void *state)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;

	/* TODO delete old shader */
	rctx->ps_shader = (struct r600_pipe_shader *)state;
}

void r600_bind_vs_shader(struct pipe_context *ctx, void *state)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;

	/* TODO delete old shader */
	rctx->vs_shader = (struct r600_pipe_shader *)state;
}

void r600_delete_ps_shader(struct pipe_context *ctx, void *state)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct r600_pipe_shader *shader = (struct r600_pipe_shader *)state;

	if (rctx->ps_shader == shader) {
		rctx->ps_shader = NULL;
	}

	r600_pipe_shader_destroy(ctx, shader);
	free(shader);
}

void r600_delete_vs_shader(struct pipe_context *ctx, void *state)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct r600_pipe_shader *shader = (struct r600_pipe_shader *)state;

	if (rctx->vs_shader == shader) {
		rctx->vs_shader = NULL;
	}

	r600_pipe_shader_destroy(ctx, shader);
	free(shader);
}

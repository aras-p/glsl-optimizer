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
 *      Corbin Simpson
 */
#include <stdio.h>
#include <errno.h>
#include <pipe/p_screen.h>
#include <util/u_format.h>
#include <util/u_math.h>
#include <util/u_inlines.h>
#include <util/u_memory.h>
#include "radeon.h"
#include "r600_screen.h"
#include "r600_context.h"
#include "r600_resource.h"
#include "r600_state_inlines.h"

struct r600_draw {
	struct pipe_context	*ctx;
	struct radeon_state	draw;
	struct radeon_state	vgt;
	unsigned		mode;
	unsigned		start;
	unsigned		count;
	unsigned		index_size;
	struct pipe_resource	*index_buffer;
};

static int r600_draw_common(struct r600_draw *draw)
{
	struct r600_context *rctx = r600_context(draw->ctx);
	struct r600_screen *rscreen = rctx->screen;
	/* FIXME vs_resource */
	struct radeon_state *vs_resource;
	struct r600_resource *rbuffer;
	unsigned i, j, offset, format, prim;
	u32 vgt_dma_index_type, vgt_draw_initiator;
	struct pipe_vertex_buffer *vertex_buffer;
	int r;

	r = r600_context_hw_states(draw->ctx);
	if (r)
		return r;
	switch (draw->index_size) {
	case 2:
		vgt_draw_initiator = 0;
		vgt_dma_index_type = 0;
		break;
	case 4:
		vgt_draw_initiator = 0;
		vgt_dma_index_type = 1;
		break;
	case 0:
		vgt_draw_initiator = 2;
		vgt_dma_index_type = 0;
		break;
	default:
		fprintf(stderr, "%s %d unsupported index size %d\n", __func__, __LINE__, draw->index_size);
		return -EINVAL;
	}
	r = r600_conv_pipe_prim(draw->mode, &prim);
	if (r)
		return r;

	/* rebuild vertex shader if input format changed */
	r = r600_pipe_shader_update(draw->ctx, rctx->vs_shader);
	if (r)
		return r;
	r = r600_pipe_shader_update(draw->ctx, rctx->ps_shader);
	if (r)
		return r;
	radeon_draw_bind(&rctx->draw, &rctx->vs_shader->rstate[0]);
	radeon_draw_bind(&rctx->draw, &rctx->ps_shader->rstate[0]);

	for (i = 0 ; i < rctx->vs_nresource; i++) {
		radeon_state_fini(&rctx->vs_resource[i]);
	}
	for (i = 0 ; i < rctx->vertex_elements->count; i++) {
		vs_resource = &rctx->vs_resource[i];
		j = rctx->vertex_elements->elements[i].vertex_buffer_index;
		vertex_buffer = &rctx->vertex_buffer[j];
		rbuffer = (struct r600_resource*)vertex_buffer->buffer;
		offset = rctx->vertex_elements->elements[i].src_offset + vertex_buffer->buffer_offset;
		format = r600_translate_colorformat(rctx->vertex_elements->elements[i].src_format);
		
		rctx->vtbl->vs_resource(rctx, i, rbuffer, offset, vertex_buffer->stride, format);
		radeon_draw_bind(&rctx->draw, vs_resource);
	}
	rctx->vs_nresource = rctx->vertex_elements->count;
	/* FIXME start need to change winsys */
	rctx->vtbl->vgt_init(rctx, &draw->draw, (struct r600_resource *)draw->index_buffer,
			     draw->count, vgt_draw_initiator);
	radeon_draw_bind(&rctx->draw, &draw->draw);

	rctx->vtbl->vgt_prim(rctx, &draw->vgt, prim, draw->start, vgt_dma_index_type);
	radeon_draw_bind(&rctx->draw, &draw->vgt);

	r = radeon_ctx_set_draw(&rctx->ctx, &rctx->draw);
	if (r == -EBUSY) {
		r600_flush(draw->ctx, 0, NULL);
		r = radeon_ctx_set_draw(&rctx->ctx, &rctx->draw);
	}

	radeon_state_fini(&draw->draw);

	return r;
}

void r600_draw_vbo(struct pipe_context *ctx, const struct pipe_draw_info *info)
{
	struct r600_context *rctx = r600_context(ctx);
	struct r600_draw draw;
	int r;

	assert(info->index_bias == 0);

	memset(&draw, 0, sizeof(draw));

	draw.ctx = ctx;
	draw.mode = info->mode;
	draw.start = info->start;
	draw.count = info->count;
	if (info->indexed && rctx->index_buffer.buffer) {
		draw.index_size = rctx->index_buffer.index_size;
		draw.index_buffer = rctx->index_buffer.buffer;

		assert(rctx->index_buffer.offset %
				rctx->index_buffer.index_size == 0);
		draw.start += rctx->index_buffer.offset /
			rctx->index_buffer.index_size;
	}
	else {
		draw.index_size = 0;
		draw.index_buffer = NULL;
	}
	r = r600_draw_common(&draw);
	if (r)
	  fprintf(stderr,"draw common failed %d\n", r);
}

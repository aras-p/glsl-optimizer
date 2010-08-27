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
#include "r600_screen.h"
#include "r600_context.h"
#include "r600_resource.h"
#include "r600_state_inlines.h"

struct r600_draw {
	struct pipe_context	*ctx;
	struct radeon_state	*draw;
	struct radeon_state	*vgt;
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
	r = radeon_draw_set(rctx->draw, rctx->vs_shader->rstate);
	if (r)
		return r;
	r = radeon_draw_set(rctx->draw, rctx->ps_shader->rstate);
	if (r)
		return r;

	for (i = 0 ; i < rctx->vertex_elements->count; i++) {
		j = rctx->vertex_elements->elements[i].vertex_buffer_index;
		vertex_buffer = &rctx->vertex_buffer[j];
		rbuffer = (struct r600_resource*)vertex_buffer->buffer;
		offset = rctx->vertex_elements->elements[i].src_offset + vertex_buffer->buffer_offset;
		format = r600_translate_colorformat(rctx->vertex_elements->elements[i].src_format);
		vs_resource = radeon_state(rscreen->rw, R600_VS_RESOURCE_TYPE, R600_VS_RESOURCE + i);
		if (vs_resource == NULL)
			return -ENOMEM;
		vs_resource->bo[0] = radeon_bo_incref(rscreen->rw, rbuffer->bo);
		vs_resource->nbo = 1;
		vs_resource->states[R600_PS_RESOURCE__RESOURCE0_WORD0] = offset;
		vs_resource->states[R600_PS_RESOURCE__RESOURCE0_WORD1] = rbuffer->bo->size - offset;
		vs_resource->states[R600_PS_RESOURCE__RESOURCE0_WORD2] = S_038008_STRIDE(vertex_buffer->stride) |
								S_038008_DATA_FORMAT(format);
		vs_resource->states[R600_PS_RESOURCE__RESOURCE0_WORD3] = 0x00000000;
		vs_resource->states[R600_PS_RESOURCE__RESOURCE0_WORD4] = 0x00000000;
		vs_resource->states[R600_PS_RESOURCE__RESOURCE0_WORD5] = 0x00000000;
		vs_resource->states[R600_PS_RESOURCE__RESOURCE0_WORD6] = 0xC0000000;
		vs_resource->placement[0] = RADEON_GEM_DOMAIN_GTT;
		vs_resource->placement[1] = RADEON_GEM_DOMAIN_GTT;
		r = radeon_draw_set_new(rctx->draw, vs_resource);
		if (r)
			return r;
	}
	/* FIXME start need to change winsys */
	draw->draw = radeon_state(rscreen->rw, R600_DRAW_TYPE, R600_DRAW);
	if (draw->draw == NULL)
		return -ENOMEM;
	draw->draw->states[R600_DRAW__VGT_NUM_INDICES] = draw->count;
	draw->draw->states[R600_DRAW__VGT_DRAW_INITIATOR] = vgt_draw_initiator;
	if (draw->index_buffer) {
		rbuffer = (struct r600_resource*)draw->index_buffer;
		draw->draw->bo[0] = radeon_bo_incref(rscreen->rw, rbuffer->bo);
		draw->draw->placement[0] = RADEON_GEM_DOMAIN_GTT;
		draw->draw->placement[1] = RADEON_GEM_DOMAIN_GTT;
		draw->draw->nbo = 1;
	}
	r = radeon_draw_set_new(rctx->draw, draw->draw);
	if (r)
		return r;
	draw->vgt = radeon_state(rscreen->rw, R600_VGT_TYPE, R600_VGT);
	if (draw->vgt == NULL)
		return -ENOMEM;
	draw->vgt->states[R600_VGT__VGT_PRIMITIVE_TYPE] = prim;
	draw->vgt->states[R600_VGT__VGT_MAX_VTX_INDX] = 0x00FFFFFF;
	draw->vgt->states[R600_VGT__VGT_MIN_VTX_INDX] = 0x00000000;
	draw->vgt->states[R600_VGT__VGT_INDX_OFFSET] = draw->start;
	draw->vgt->states[R600_VGT__VGT_MULTI_PRIM_IB_RESET_INDX] = 0x00000000;
	draw->vgt->states[R600_VGT__VGT_DMA_INDEX_TYPE] = vgt_dma_index_type;
	draw->vgt->states[R600_VGT__VGT_PRIMITIVEID_EN] = 0x00000000;
	draw->vgt->states[R600_VGT__VGT_DMA_NUM_INSTANCES] = 0x00000001;
	draw->vgt->states[R600_VGT__VGT_MULTI_PRIM_IB_RESET_EN] = 0x00000000;
	draw->vgt->states[R600_VGT__VGT_INSTANCE_STEP_RATE_0] = 0x00000000;
	draw->vgt->states[R600_VGT__VGT_INSTANCE_STEP_RATE_1] = 0x00000000;
	r = radeon_draw_set_new(rctx->draw, draw->vgt);
	if (r)
		return r;
	/* FIXME */
	r = radeon_ctx_set_draw_new(rctx->ctx, rctx->draw);
	if (r == -EBUSY) {
		r600_flush(draw->ctx, 0, NULL);
		r = radeon_ctx_set_draw_new(rctx->ctx, rctx->draw);
	}
	if (r)
		return r;
	rctx->draw = radeon_draw_duplicate(rctx->draw);
	return 0;
}

void r600_draw_vbo(struct pipe_context *ctx, const struct pipe_draw_info *info)
{
	struct r600_context *rctx = r600_context(ctx);
	struct r600_draw draw;

	assert(info->index_bias == 0);

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
	r600_draw_common(&draw);
}

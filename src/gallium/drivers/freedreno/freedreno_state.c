/* -*- mode: C; c-file-style: "k&r"; tab-width 4; indent-tabs-mode: t; -*- */

/*
 * Copyright (C) 2012 Rob Clark <robclark@freedesktop.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *    Rob Clark <robclark@freedesktop.org>
 */

#include "pipe/p_state.h"
#include "util/u_string.h"
#include "util/u_memory.h"
#include "util/u_helpers.h"

#include "freedreno_state.h"
#include "freedreno_context.h"
#include "freedreno_resource.h"
#include "freedreno_texture.h"
#include "freedreno_gmem.h"
#include "freedreno_util.h"

/* All the generic state handling.. In case of CSO's that are specific
 * to the GPU version, when the bind and the delete are common they can
 * go in here.
 */

static void
fd_set_blend_color(struct pipe_context *pctx,
		const struct pipe_blend_color *blend_color)
{
	struct fd_context *ctx = fd_context(pctx);
	ctx->blend_color = *blend_color;
	ctx->dirty |= FD_DIRTY_BLEND_COLOR;
}

static void
fd_set_stencil_ref(struct pipe_context *pctx,
		const struct pipe_stencil_ref *stencil_ref)
{
	struct fd_context *ctx = fd_context(pctx);
	ctx->stencil_ref =* stencil_ref;
	ctx->dirty |= FD_DIRTY_STENCIL_REF;
}

static void
fd_set_clip_state(struct pipe_context *pctx,
		const struct pipe_clip_state *clip)
{
	DBG("TODO: ");
}

static void
fd_set_sample_mask(struct pipe_context *pctx, unsigned sample_mask)
{
	struct fd_context *ctx = fd_context(pctx);
	ctx->sample_mask = (uint16_t)sample_mask;
	ctx->dirty |= FD_DIRTY_SAMPLE_MASK;
}

/* notes from calim on #dri-devel:
 * index==0 will be non-UBO (ie. glUniformXYZ()) all packed together padded
 * out to vec4's
 * I should be able to consider that I own the user_ptr until the next
 * set_constant_buffer() call, at which point I don't really care about the
 * previous values.
 * index>0 will be UBO's.. well, I'll worry about that later
 */
static void
fd_set_constant_buffer(struct pipe_context *pctx, uint shader, uint index,
		struct pipe_constant_buffer *cb)
{
	struct fd_context *ctx = fd_context(pctx);
	struct fd_constbuf_stateobj *so = &ctx->constbuf[shader];

	/* Note that the state tracker can unbind constant buffers by
	 * passing NULL here.
	 */
	if (unlikely(!cb)) {
		so->enabled_mask &= ~(1 << index);
		so->dirty_mask &= ~(1 << index);
		pipe_resource_reference(&so->cb[index].buffer, NULL);
		return;
	}

	pipe_resource_reference(&so->cb[index].buffer, cb->buffer);
	so->cb[index].buffer_offset = cb->buffer_offset;
	so->cb[index].buffer_size   = cb->buffer_size;
	so->cb[index].user_buffer   = cb->user_buffer;

	so->enabled_mask |= 1 << index;
	so->dirty_mask |= 1 << index;
	ctx->dirty |= FD_DIRTY_CONSTBUF;
}

static void
fd_set_framebuffer_state(struct pipe_context *pctx,
		const struct pipe_framebuffer_state *framebuffer)
{
	struct fd_context *ctx = fd_context(pctx);
	struct pipe_framebuffer_state *cso = &ctx->framebuffer;
	unsigned i;

	DBG("%d: cbufs[0]=%p, zsbuf=%p", ctx->needs_flush,
			framebuffer->cbufs[0], framebuffer->zsbuf);

	fd_context_render(pctx);

	for (i = 0; i < framebuffer->nr_cbufs; i++)
		pipe_surface_reference(&cso->cbufs[i], framebuffer->cbufs[i]);
	for (; i < ctx->framebuffer.nr_cbufs; i++)
		pipe_surface_reference(&cso->cbufs[i], NULL);

	cso->nr_cbufs = framebuffer->nr_cbufs;

	if ((cso->width != framebuffer->width) ||
			(cso->height != framebuffer->height))
		ctx->needs_rb_fbd = true;

	cso->width = framebuffer->width;
	cso->height = framebuffer->height;

	pipe_surface_reference(&cso->zsbuf, framebuffer->zsbuf);

	ctx->dirty |= FD_DIRTY_FRAMEBUFFER;

	ctx->disabled_scissor.minx = 0;
	ctx->disabled_scissor.miny = 0;
	ctx->disabled_scissor.maxx = cso->width;
	ctx->disabled_scissor.maxy = cso->height;

	ctx->dirty |= FD_DIRTY_SCISSOR;
}

static void
fd_set_polygon_stipple(struct pipe_context *pctx,
		const struct pipe_poly_stipple *stipple)
{
	struct fd_context *ctx = fd_context(pctx);
	ctx->stipple = *stipple;
	ctx->dirty |= FD_DIRTY_STIPPLE;
}

static void
fd_set_scissor_states(struct pipe_context *pctx,
		unsigned start_slot,
		unsigned num_scissors,
		const struct pipe_scissor_state *scissor)
{
	struct fd_context *ctx = fd_context(pctx);

	ctx->scissor = *scissor;
	ctx->dirty |= FD_DIRTY_SCISSOR;
}

static void
fd_set_viewport_states(struct pipe_context *pctx,
		unsigned start_slot,
		unsigned num_viewports,
		const struct pipe_viewport_state *viewport)
{
	struct fd_context *ctx = fd_context(pctx);
	ctx->viewport = *viewport;
	ctx->dirty |= FD_DIRTY_VIEWPORT;
}

static void
fd_set_vertex_buffers(struct pipe_context *pctx,
		unsigned start_slot, unsigned count,
		const struct pipe_vertex_buffer *vb)
{
	struct fd_context *ctx = fd_context(pctx);
	struct fd_vertexbuf_stateobj *so = &ctx->vertexbuf;
	int i;

	/* on a2xx, pitch is encoded in the vtx fetch instruction, so
	 * we need to mark VTXSTATE as dirty as well to trigger patching
	 * and re-emitting the vtx shader:
	 */
	for (i = 0; i < count; i++) {
		bool new_enabled = vb && (vb[i].buffer || vb[i].user_buffer);
		bool old_enabled = so->vb[i].buffer || so->vb[i].user_buffer;
		uint32_t new_stride = vb ? vb[i].stride : 0;
		uint32_t old_stride = so->vb[i].stride;
		if ((new_enabled != old_enabled) || (new_stride != old_stride)) {
			ctx->dirty |= FD_DIRTY_VTXSTATE;
			break;
		}
	}

	util_set_vertex_buffers_mask(so->vb, &so->enabled_mask, vb, start_slot, count);
	so->count = util_last_bit(so->enabled_mask);

	ctx->dirty |= FD_DIRTY_VTXBUF;
}

static void
fd_set_index_buffer(struct pipe_context *pctx,
		const struct pipe_index_buffer *ib)
{
	struct fd_context *ctx = fd_context(pctx);

	if (ib) {
		pipe_resource_reference(&ctx->indexbuf.buffer, ib->buffer);
		ctx->indexbuf.index_size = ib->index_size;
		ctx->indexbuf.offset = ib->offset;
		ctx->indexbuf.user_buffer = ib->user_buffer;
	} else {
		pipe_resource_reference(&ctx->indexbuf.buffer, NULL);
	}

	ctx->dirty |= FD_DIRTY_INDEXBUF;
}

static void
fd_blend_state_bind(struct pipe_context *pctx, void *hwcso)
{
	struct fd_context *ctx = fd_context(pctx);
	ctx->blend = hwcso;
	ctx->dirty |= FD_DIRTY_BLEND;
}

static void
fd_blend_state_delete(struct pipe_context *pctx, void *hwcso)
{
	FREE(hwcso);
}

static void
fd_rasterizer_state_bind(struct pipe_context *pctx, void *hwcso)
{
	struct fd_context *ctx = fd_context(pctx);
	ctx->rasterizer = hwcso;
	ctx->dirty |= FD_DIRTY_RASTERIZER;
}

static void
fd_rasterizer_state_delete(struct pipe_context *pctx, void *hwcso)
{
	FREE(hwcso);
}

static void
fd_zsa_state_bind(struct pipe_context *pctx, void *hwcso)
{
	struct fd_context *ctx = fd_context(pctx);
	ctx->zsa = hwcso;
	ctx->dirty |= FD_DIRTY_ZSA;
}

static void
fd_zsa_state_delete(struct pipe_context *pctx, void *hwcso)
{
	FREE(hwcso);
}

static void *
fd_vertex_state_create(struct pipe_context *pctx, unsigned num_elements,
		const struct pipe_vertex_element *elements)
{
	struct fd_vertex_stateobj *so = CALLOC_STRUCT(fd_vertex_stateobj);

	if (!so)
		return NULL;

	memcpy(so->pipe, elements, sizeof(*elements) * num_elements);
	so->num_elements = num_elements;

	return so;
}

static void
fd_vertex_state_delete(struct pipe_context *pctx, void *hwcso)
{
	FREE(hwcso);
}

static void
fd_vertex_state_bind(struct pipe_context *pctx, void *hwcso)
{
	struct fd_context *ctx = fd_context(pctx);
	ctx->vtx = hwcso;
	ctx->dirty |= FD_DIRTY_VTXSTATE;
}

void
fd_state_init(struct pipe_context *pctx)
{
	pctx->set_blend_color = fd_set_blend_color;
	pctx->set_stencil_ref = fd_set_stencil_ref;
	pctx->set_clip_state = fd_set_clip_state;
	pctx->set_sample_mask = fd_set_sample_mask;
	pctx->set_constant_buffer = fd_set_constant_buffer;
	pctx->set_framebuffer_state = fd_set_framebuffer_state;
	pctx->set_polygon_stipple = fd_set_polygon_stipple;
	pctx->set_scissor_states = fd_set_scissor_states;
	pctx->set_viewport_states = fd_set_viewport_states;

	pctx->set_vertex_buffers = fd_set_vertex_buffers;
	pctx->set_index_buffer = fd_set_index_buffer;

	pctx->bind_blend_state = fd_blend_state_bind;
	pctx->delete_blend_state = fd_blend_state_delete;

	pctx->bind_rasterizer_state = fd_rasterizer_state_bind;
	pctx->delete_rasterizer_state = fd_rasterizer_state_delete;

	pctx->bind_depth_stencil_alpha_state = fd_zsa_state_bind;
	pctx->delete_depth_stencil_alpha_state = fd_zsa_state_delete;

	pctx->create_vertex_elements_state = fd_vertex_state_create;
	pctx->delete_vertex_elements_state = fd_vertex_state_delete;
	pctx->bind_vertex_elements_state = fd_vertex_state_bind;
}

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
#include "util/u_prim.h"

#include "freedreno_vbo.h"
#include "freedreno_context.h"
#include "freedreno_state.h"
#include "freedreno_zsa.h"
#include "freedreno_resource.h"
#include "freedreno_util.h"


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
	ctx->dirty |= FD_DIRTY_VTX;
}

static void
emit_cacheflush(struct fd_ringbuffer *ring)
{
	unsigned i;

	for (i = 0; i < 12; i++) {
		OUT_PKT3(ring, CP_EVENT_WRITE, 1);
		OUT_RING(ring, CACHE_FLUSH);
	}
}

static enum pc_di_primtype
mode2primtype(unsigned mode)
{
	switch (mode) {
	case PIPE_PRIM_POINTS:         return DI_PT_POINTLIST;
	case PIPE_PRIM_LINES:          return DI_PT_LINELIST;
	case PIPE_PRIM_LINE_STRIP:     return DI_PT_LINESTRIP;
	case PIPE_PRIM_TRIANGLES:      return DI_PT_TRILIST;
	case PIPE_PRIM_TRIANGLE_STRIP: return DI_PT_TRISTRIP;
	case PIPE_PRIM_TRIANGLE_FAN:   return DI_PT_TRIFAN;
	case PIPE_PRIM_QUADS:          return DI_PT_QUADLIST;
	case PIPE_PRIM_QUAD_STRIP:     return DI_PT_QUADSTRIP;
	case PIPE_PRIM_POLYGON:        return DI_PT_POLYGON;
	}
	DBG("unsupported mode: (%s) %d", u_prim_name(mode), mode);
	assert(0);
	return DI_PT_NONE;
}

static enum pc_di_index_size
size2indextype(unsigned index_size)
{
	switch (index_size) {
	case 1: return INDEX_SIZE_8_BIT;
	case 2: return INDEX_SIZE_16_BIT;
	case 4: return INDEX_SIZE_32_BIT;
	}
	DBG("unsupported index size: %d", index_size);
	assert(0);
	return INDEX_SIZE_IGN;
}

static void
emit_vertexbufs(struct fd_context *ctx, unsigned count)
{
	struct fd_vertex_stateobj *vtx = ctx->vtx;
	struct fd_vertexbuf_stateobj *vertexbuf = &ctx->vertexbuf;
	struct fd_vertex_buf bufs[PIPE_MAX_ATTRIBS];
	unsigned i;

	if (!vtx->num_elements)
		return;

	for (i = 0; i < vtx->num_elements; i++) {
		struct pipe_vertex_element *elem = &vtx->pipe[i];
		struct pipe_vertex_buffer *vb =
				&vertexbuf->vb[elem->vertex_buffer_index];
		bufs[i].offset = vb->buffer_offset;
		bufs[i].size = count * vb->stride;
		bufs[i].prsc = vb->buffer;
	}

	// NOTE I believe the 0x78 (or 0x9c in solid_vp) relates to the
	// CONST(20,0) (or CONST(26,0) in soliv_vp)

	fd_emit_vertex_bufs(ctx->ring, 0x78, bufs, vtx->num_elements);
}

static void
fd_draw_vbo(struct pipe_context *pctx, const struct pipe_draw_info *info)
{
	struct fd_context *ctx = fd_context(pctx);
	struct pipe_framebuffer_state *fb = &ctx->framebuffer;
	struct fd_ringbuffer *ring = ctx->ring;
	struct fd_bo *idx_bo = NULL;
	enum pc_di_index_size idx_type = INDEX_SIZE_IGN;
	enum pc_di_src_sel src_sel;
	uint32_t idx_size, idx_offset;
	unsigned buffers;

	/* if we supported transform feedback, we'd have to disable this: */
	if (((ctx->scissor.maxx - ctx->scissor.minx) *
			(ctx->scissor.maxy - ctx->scissor.miny)) == 0) {
		return;
	}

	ctx->needs_flush = true;

	if (info->indexed) {
		struct pipe_index_buffer *idx = &ctx->indexbuf;

		assert(!idx->user_buffer);

		idx_bo = fd_resource(idx->buffer)->bo;
		idx_type = size2indextype(idx->index_size);
		idx_size = idx->index_size * info->count;
		idx_offset = idx->offset;
		src_sel = DI_SRC_SEL_DMA;
	} else {
		idx_bo = NULL;
		idx_type = INDEX_SIZE_IGN;
		idx_size = 0;
		idx_offset = 0;
		src_sel = DI_SRC_SEL_AUTO_INDEX;
	}

	fd_resource(fb->cbufs[0]->texture)->dirty = true;

	/* figure out the buffers we need: */
	buffers = FD_BUFFER_COLOR;
	if (fd_depth_enabled(ctx->zsa)) {
		buffers |= FD_BUFFER_DEPTH;
		fd_resource(fb->zsbuf->texture)->dirty = true;
	}
	if (fd_stencil_enabled(ctx->zsa)) {
		buffers |= FD_BUFFER_STENCIL;
		fd_resource(fb->zsbuf->texture)->dirty = true;
	}

	/* any buffers that haven't been cleared, we need to restore: */
	ctx->restore |= buffers & (FD_BUFFER_ALL & ~ctx->cleared);
	/* and any buffers used, need to be resolved: */
	ctx->resolve |= buffers;

	fd_state_emit(pctx, ctx->dirty);

	emit_vertexbufs(ctx, info->count);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_VGT_INDX_OFFSET));
	OUT_RING(ring, info->start);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_VGT_VERTEX_REUSE_BLOCK_CNTL));
	OUT_RING(ring, 0x0000003b);

	OUT_PKT0(ring, REG_TC_CNTL_STATUS, 1);
	OUT_RING(ring, TC_CNTL_STATUS_L2_INVALIDATE);

	OUT_PKT3(ring, CP_WAIT_FOR_IDLE, 1);
	OUT_RING(ring, 0x0000000);

	OUT_PKT3(ring, CP_DRAW_INDX, info->indexed ? 5 : 3);
	OUT_RING(ring, 0x00000000);        /* viz query info. */
	OUT_RING(ring, DRAW(mode2primtype(info->mode),
			src_sel, idx_type, IGNORE_VISIBILITY));
	OUT_RING(ring, info->count);       /* NumIndices */
	if (info->indexed) {
		OUT_RELOC(ring, idx_bo, idx_offset, 0);
		OUT_RING (ring, idx_size);
	}

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_2010));
	OUT_RING(ring, 0x00000000);

	emit_cacheflush(ring);
}

void
fd_vbo_init(struct pipe_context *pctx)
{
	pctx->create_vertex_elements_state = fd_vertex_state_create;
	pctx->delete_vertex_elements_state = fd_vertex_state_delete;
	pctx->bind_vertex_elements_state = fd_vertex_state_bind;
	pctx->draw_vbo = fd_draw_vbo;
}

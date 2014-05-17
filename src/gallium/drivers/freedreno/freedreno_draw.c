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
#include "util/u_format.h"

#include "freedreno_draw.h"
#include "freedreno_context.h"
#include "freedreno_state.h"
#include "freedreno_resource.h"
#include "freedreno_query_hw.h"
#include "freedreno_util.h"


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

/* this is same for a2xx/a3xx, so split into helper: */
void
fd_draw_emit(struct fd_context *ctx, struct fd_ringbuffer *ring,
		enum pc_di_vis_cull_mode vismode,
		const struct pipe_draw_info *info)
{
	struct pipe_index_buffer *idx = &ctx->indexbuf;
	struct fd_bo *idx_bo = NULL;
	enum pc_di_index_size idx_type = INDEX_SIZE_IGN;
	enum pc_di_src_sel src_sel;
	uint32_t idx_size, idx_offset;

	if (info->indexed) {
		assert(!idx->user_buffer);

		idx_bo = fd_resource(idx->buffer)->bo;
		idx_type = size2indextype(idx->index_size);
		idx_size = idx->index_size * info->count;
		idx_offset = idx->offset + (info->start * idx->index_size);
		src_sel = DI_SRC_SEL_DMA;
	} else {
		idx_bo = NULL;
		idx_type = INDEX_SIZE_IGN;
		idx_size = 0;
		idx_offset = 0;
		src_sel = DI_SRC_SEL_AUTO_INDEX;
	}

	fd_draw(ctx, ring, ctx->primtypes[info->mode], vismode, src_sel,
			info->count, idx_type, idx_size, idx_offset, idx_bo);
}

static void
fd_draw_vbo(struct pipe_context *pctx, const struct pipe_draw_info *info)
{
	struct fd_context *ctx = fd_context(pctx);
	struct pipe_framebuffer_state *pfb = &ctx->framebuffer;
	struct pipe_scissor_state *scissor = fd_context_get_scissor(ctx);
	unsigned i, buffers = 0;

	/* if we supported transform feedback, we'd have to disable this: */
	if (((scissor->maxx - scissor->minx) *
			(scissor->maxy - scissor->miny)) == 0) {
		return;
	}

	/* emulate unsupported primitives: */
	if (!fd_supported_prim(ctx, info->mode)) {
		util_primconvert_save_index_buffer(ctx->primconvert, &ctx->indexbuf);
		util_primconvert_save_rasterizer_state(ctx->primconvert, ctx->rasterizer);
		util_primconvert_draw_vbo(ctx->primconvert, info);
		return;
	}

	ctx->needs_flush = true;

	/*
	 * Figure out the buffers/features we need:
	 */

	if (fd_depth_enabled(ctx)) {
		buffers |= FD_BUFFER_DEPTH;
		fd_resource(pfb->zsbuf->texture)->dirty = true;
		ctx->gmem_reason |= FD_GMEM_DEPTH_ENABLED;
	}

	if (fd_stencil_enabled(ctx)) {
		buffers |= FD_BUFFER_STENCIL;
		fd_resource(pfb->zsbuf->texture)->dirty = true;
		ctx->gmem_reason |= FD_GMEM_STENCIL_ENABLED;
	}

	if (fd_logicop_enabled(ctx))
		ctx->gmem_reason |= FD_GMEM_LOGICOP_ENABLED;

	for (i = 0; i < pfb->nr_cbufs; i++) {
		struct pipe_resource *surf;

		if (!pfb->cbufs[i])
			continue;

		surf = pfb->cbufs[i]->texture;

		fd_resource(surf)->dirty = true;
		buffers |= FD_BUFFER_COLOR;

		if (surf->nr_samples > 1)
			ctx->gmem_reason |= FD_GMEM_MSAA_ENABLED;

		if (fd_blend_enabled(ctx, i))
			ctx->gmem_reason |= FD_GMEM_BLEND_ENABLED;
	}

	ctx->num_draws++;

	ctx->stats.draw_calls++;
	ctx->stats.prims_emitted +=
		u_reduced_prims_for_vertices(info->mode, info->count);

	/* any buffers that haven't been cleared, we need to restore: */
	ctx->restore |= buffers & (FD_BUFFER_ALL & ~ctx->cleared);
	/* and any buffers used, need to be resolved: */
	ctx->resolve |= buffers;

	fd_hw_query_set_stage(ctx, ctx->ring, FD_STAGE_DRAW);
	ctx->draw(ctx, info);
}

/* TODO figure out how to make better use of existing state mechanism
 * for clear (and possibly gmem->mem / mem->gmem) so we can (a) keep
 * track of what state really actually changes, and (b) reduce the code
 * in the a2xx/a3xx parts.
 */

static void
fd_clear(struct pipe_context *pctx, unsigned buffers,
		const union pipe_color_union *color, double depth, unsigned stencil)
{
	struct fd_context *ctx = fd_context(pctx);
	struct pipe_framebuffer_state *pfb = &ctx->framebuffer;

	ctx->cleared |= buffers;
	ctx->resolve |= buffers;
	ctx->needs_flush = true;

	if (buffers & PIPE_CLEAR_COLOR)
		fd_resource(pfb->cbufs[0]->texture)->dirty = true;

	if (buffers & (PIPE_CLEAR_DEPTH | PIPE_CLEAR_STENCIL)) {
		fd_resource(pfb->zsbuf->texture)->dirty = true;
		ctx->gmem_reason |= FD_GMEM_CLEARS_DEPTH_STENCIL;
	}

	DBG("%x depth=%f, stencil=%u (%s/%s)", buffers, depth, stencil,
		util_format_short_name(pipe_surface_format(pfb->cbufs[0])),
		util_format_short_name(pipe_surface_format(pfb->zsbuf)));

	fd_hw_query_set_stage(ctx, ctx->ring, FD_STAGE_CLEAR);

	ctx->clear(ctx, buffers, color, depth, stencil);

	ctx->dirty |= FD_DIRTY_ZSA |
			FD_DIRTY_VIEWPORT |
			FD_DIRTY_RASTERIZER |
			FD_DIRTY_SAMPLE_MASK |
			FD_DIRTY_PROG |
			FD_DIRTY_CONSTBUF |
			FD_DIRTY_BLEND;

	if (fd_mesa_debug & FD_DBG_DCLEAR)
		ctx->dirty = 0xffffffff;
}

static void
fd_clear_render_target(struct pipe_context *pctx, struct pipe_surface *ps,
		const union pipe_color_union *color,
		unsigned x, unsigned y, unsigned w, unsigned h)
{
	DBG("TODO: x=%u, y=%u, w=%u, h=%u", x, y, w, h);
}

static void
fd_clear_depth_stencil(struct pipe_context *pctx, struct pipe_surface *ps,
		unsigned buffers, double depth, unsigned stencil,
		unsigned x, unsigned y, unsigned w, unsigned h)
{
	DBG("TODO: buffers=%u, depth=%f, stencil=%u, x=%u, y=%u, w=%u, h=%u",
			buffers, depth, stencil, x, y, w, h);
}

void
fd_draw_init(struct pipe_context *pctx)
{
	pctx->draw_vbo = fd_draw_vbo;
	pctx->clear = fd_clear;
	pctx->clear_render_target = fd_clear_render_target;
	pctx->clear_depth_stencil = fd_clear_depth_stencil;
}

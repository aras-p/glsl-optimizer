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

#ifndef FREEDRENO_CONTEXT_H_
#define FREEDRENO_CONTEXT_H_

#include "draw/draw_context.h"
#include "pipe/p_context.h"
#include "util/u_blitter.h"
#include "util/u_slab.h"
#include "util/u_string.h"

#include "freedreno_screen.h"

struct fd_vertex_stateobj;

struct fd_texture_stateobj {
	struct pipe_sampler_view *textures[PIPE_MAX_SAMPLERS];
	unsigned num_textures;
	struct pipe_sampler_state *samplers[PIPE_MAX_SAMPLERS];
	unsigned num_samplers;
	unsigned dirty_samplers;
};

struct fd_program_stateobj {
	void *vp, *fp;
	enum {
		FD_SHADER_DIRTY_VP = (1 << 0),
		FD_SHADER_DIRTY_FP = (1 << 1),
	} dirty;
	uint8_t num_exports;
	/* Indexed by semantic name or TGSI_SEMANTIC_COUNT + semantic index
	 * for TGSI_SEMANTIC_GENERIC.  Special vs exports (position and point-
	 * size) are not included in this
	 */
	uint8_t export_linkage[63];
};

struct fd_constbuf_stateobj {
	struct pipe_constant_buffer cb[PIPE_MAX_CONSTANT_BUFFERS];
	uint32_t enabled_mask;
	uint32_t dirty_mask;
};

struct fd_vertexbuf_stateobj {
	struct pipe_vertex_buffer vb[PIPE_MAX_ATTRIBS];
	unsigned count;
	uint32_t enabled_mask;
	uint32_t dirty_mask;
};

struct fd_vertex_stateobj {
	struct pipe_vertex_element pipe[PIPE_MAX_ATTRIBS];
	unsigned num_elements;
};

struct fd_gmem_stateobj {
	struct pipe_scissor_state scissor;
	uint cpp;
	uint16_t minx, miny;
	uint16_t bin_h, nbins_y;
	uint16_t bin_w, nbins_x;
	uint16_t width, height;
};

struct fd_context {
	struct pipe_context base;

	struct fd_screen *screen;
	struct blitter_context *blitter;

	struct util_slab_mempool transfer_pool;

	/* shaders used by clear, and gmem->mem blits: */
	struct fd_program_stateobj solid_prog; // TODO move to screen?

	/* shaders used by mem->gmem blits: */
	struct fd_program_stateobj blit_prog; // TODO move to screen?

	/* do we need to mem2gmem before rendering.  We don't, if for example,
	 * there was a glClear() that invalidated the entire previous buffer
	 * contents.  Keep track of which buffer(s) are cleared, or needs
	 * restore.  Masks of PIPE_CLEAR_*
	 */
	enum {
		/* align bitmask values w/ PIPE_CLEAR_*.. since that is convenient.. */
		FD_BUFFER_COLOR   = PIPE_CLEAR_COLOR,
		FD_BUFFER_DEPTH   = PIPE_CLEAR_DEPTH,
		FD_BUFFER_STENCIL = PIPE_CLEAR_STENCIL,
		FD_BUFFER_ALL     = FD_BUFFER_COLOR | FD_BUFFER_DEPTH | FD_BUFFER_STENCIL,
	} cleared, restore, resolve;

	bool needs_flush;

	/* To decide whether to render to system memory, keep track of the
	 * number of draws, and whether any of them require multisample,
	 * depth_test (or depth write), stencil_test, blending, and
	 * color_logic_Op (since those functions are disabled when by-
	 * passing GMEM.
	 */
	enum {
		FD_GMEM_CLEARS_DEPTH_STENCIL = 0x01,
		FD_GMEM_DEPTH_ENABLED        = 0x02,
		FD_GMEM_STENCIL_ENABLED      = 0x04,

		FD_GMEM_MSAA_ENABLED         = 0x08,
		FD_GMEM_BLEND_ENABLED        = 0x10,
		FD_GMEM_LOGICOP_ENABLED      = 0x20,
	} gmem_reason;
	unsigned num_draws;

	struct fd_ringbuffer *ring;
	struct fd_ringmarker *draw_start, *draw_end;

	struct pipe_scissor_state scissor;

	/* we don't have a disable/enable bit for scissor, so instead we keep
	 * a disabled-scissor state which matches the entire bound framebuffer
	 * and use that when scissor is not enabled.
	 */
	struct pipe_scissor_state disabled_scissor;

	/* Track the maximal bounds of the scissor of all the draws within a
	 * batch.  Used at the tile rendering step (fd_gmem_render_tiles(),
	 * mem2gmem/gmem2mem) to avoid needlessly moving data in/out of gmem.
	 */
	struct pipe_scissor_state max_scissor;

	/* Current gmem/tiling configuration.. gets updated on render_tiles()
	 * if out of date with current maximal-scissor/cpp:
	 */
	struct fd_gmem_stateobj gmem;

	/* which state objects need to be re-emit'd: */
	enum {
		FD_DIRTY_BLEND       = (1 <<  0),
		FD_DIRTY_RASTERIZER  = (1 <<  1),
		FD_DIRTY_ZSA         = (1 <<  2),
		FD_DIRTY_FRAGTEX     = (1 <<  3),
		FD_DIRTY_VERTTEX     = (1 <<  4),
		FD_DIRTY_TEXSTATE    = (1 <<  5),
		FD_DIRTY_PROG        = (1 <<  6),
		FD_DIRTY_BLEND_COLOR = (1 <<  7),
		FD_DIRTY_STENCIL_REF = (1 <<  8),
		FD_DIRTY_SAMPLE_MASK = (1 <<  9),
		FD_DIRTY_FRAMEBUFFER = (1 << 10),
		FD_DIRTY_STIPPLE     = (1 << 11),
		FD_DIRTY_VIEWPORT    = (1 << 12),
		FD_DIRTY_CONSTBUF    = (1 << 13),
		FD_DIRTY_VTXSTATE    = (1 << 14),
		FD_DIRTY_VTXBUF      = (1 << 15),
		FD_DIRTY_INDEXBUF    = (1 << 16),
		FD_DIRTY_SCISSOR     = (1 << 17),
	} dirty;

	struct pipe_blend_state *blend;
	struct pipe_rasterizer_state *rasterizer;
	struct pipe_depth_stencil_alpha_state *zsa;

	struct fd_texture_stateobj verttex, fragtex;

	struct fd_program_stateobj prog;

	struct fd_vertex_stateobj *vtx;

	struct pipe_blend_color blend_color;
	struct pipe_stencil_ref stencil_ref;
	unsigned sample_mask;
	struct pipe_framebuffer_state framebuffer;
	struct pipe_poly_stipple stipple;
	struct pipe_viewport_state viewport;
	struct fd_constbuf_stateobj constbuf[PIPE_SHADER_TYPES];
	struct fd_vertexbuf_stateobj vertexbuf;
	struct pipe_index_buffer indexbuf;

	/* GMEM/tile handling fxns: */
	void (*emit_tile_init)(struct fd_context *ctx);
	void (*emit_tile_prep)(struct fd_context *ctx, uint32_t xoff, uint32_t yoff,
			uint32_t bin_w, uint32_t bin_h);
	void (*emit_tile_mem2gmem)(struct fd_context *ctx, uint32_t xoff, uint32_t yoff,
			uint32_t bin_w, uint32_t bin_h);
	void (*emit_tile_renderprep)(struct fd_context *ctx, uint32_t xoff, uint32_t yoff,
			uint32_t bin_w, uint32_t bin_h);
	void (*emit_tile_gmem2mem)(struct fd_context *ctx, uint32_t xoff, uint32_t yoff,
			uint32_t bin_w, uint32_t bin_h);

	/* optional, for GMEM bypass: */
	void (*emit_sysmem_prep)(struct fd_context *ctx);

	/* draw: */
	void (*draw)(struct fd_context *pctx, const struct pipe_draw_info *info);
	void (*clear)(struct fd_context *ctx, unsigned buffers,
			const union pipe_color_union *color, double depth, unsigned stencil);
};

static INLINE struct fd_context *
fd_context(struct pipe_context *pctx)
{
	return (struct fd_context *)pctx;
}

static INLINE struct pipe_scissor_state *
fd_context_get_scissor(struct fd_context *ctx)
{
	if (ctx->rasterizer && ctx->rasterizer->scissor)
		return &ctx->scissor;
	return &ctx->disabled_scissor;
}

struct pipe_context * fd_context_init(struct fd_context *ctx,
		struct pipe_screen *pscreen, void *priv);

void fd_context_render(struct pipe_context *pctx);

void fd_context_destroy(struct pipe_context *pctx);

#endif /* FREEDRENO_CONTEXT_H_ */

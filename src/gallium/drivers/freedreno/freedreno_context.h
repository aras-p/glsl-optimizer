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
#include "indices/u_primconvert.h"
#include "util/u_blitter.h"
#include "util/u_double_list.h"
#include "util/u_slab.h"
#include "util/u_string.h"

#include "freedreno_screen.h"
#include "freedreno_gmem.h"
#include "freedreno_util.h"

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

/* Bitmask of stages in rendering that a particular query query is
 * active.  Queries will be automatically started/stopped (generating
 * additional fd_hw_sample_period's) on entrance/exit from stages that
 * are applicable to the query.
 *
 * NOTE: set the stage to NULL at end of IB to ensure no query is still
 * active.  Things aren't going to work out the way you want if a query
 * is active across IB's (or between tile IB and draw IB)
 */
enum fd_render_stage {
	FD_STAGE_NULL     = 0x00,
	FD_STAGE_DRAW     = 0x01,
	FD_STAGE_CLEAR    = 0x02,
	/* TODO before queries which include MEM2GMEM or GMEM2MEM will
	 * work we will need to call fd_hw_query_prepare() from somewhere
	 * appropriate so that queries in the tiling IB get backed with
	 * memory to write results to.
	 */
	FD_STAGE_MEM2GMEM = 0x04,
	FD_STAGE_GMEM2MEM = 0x08,
	/* used for driver internal draws (ie. util_blitter_blit()): */
	FD_STAGE_BLIT     = 0x10,
};

#define MAX_HW_SAMPLE_PROVIDERS 4
struct fd_hw_sample_provider;
struct fd_hw_sample;

struct fd_context {
	struct pipe_context base;

	struct fd_device *dev;
	struct fd_screen *screen;

	struct blitter_context *blitter;
	struct primconvert_context *primconvert;

	/* slab for pipe_transfer allocations: */
	struct util_slab_mempool transfer_pool;

	/* slabs for fd_hw_sample and fd_hw_sample_period allocations: */
	struct util_slab_mempool sample_pool;
	struct util_slab_mempool sample_period_pool;

	/* next sample offset.. incremented for each sample in the batch/
	 * submit, reset to zero on next submit.
	 */
	uint32_t next_sample_offset;

	/* sample-providers for hw queries: */
	const struct fd_hw_sample_provider *sample_providers[MAX_HW_SAMPLE_PROVIDERS];

	/* cached samples (in case multiple queries need to reference
	 * the same sample snapshot)
	 */
	struct fd_hw_sample *sample_cache[MAX_HW_SAMPLE_PROVIDERS];

	/* tracking for current stage, to know when to start/stop
	 * any active queries:
	 */
	enum fd_render_stage stage;

	/* list of active queries: */
	struct list_head active_queries;

	/* list of queries that are not active, but were active in the
	 * current submit:
	 */
	struct list_head current_queries;

	/* current query result bo and tile stride: */
	struct fd_bo *query_bo;
	uint32_t query_tile_stride;

	/* table with PIPE_PRIM_MAX entries mapping PIPE_PRIM_x to
	 * DI_PT_x value to use for draw initiator.  There are some
	 * slight differences between generation:
	 */
	const uint8_t *primtypes;
	uint32_t primtype_mask;

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
		FD_BUFFER_COLOR   = PIPE_CLEAR_COLOR0,
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
	unsigned num_draws;   /* number of draws in current batch */

	/* Stats/counters:
	 */
	struct {
		uint64_t prims_emitted;
		uint64_t draw_calls;
		uint64_t batch_total, batch_sysmem, batch_gmem, batch_restore;
	} stats;

	/* we can't really sanely deal with wraparound point in ringbuffer
	 * and because of the way tiling works we can't really flush at
	 * arbitrary points (without a big performance hit).  When we get
	 * too close to the end of the current ringbuffer, cycle to the next
	 * one (and wait for pending rendering from next rb to complete).
	 * We want the # of ringbuffers to be high enough that we don't
	 * normally have to wait before resetting to the start of the next
	 * rb.
	 */
	struct fd_ringbuffer *rings[8];
	unsigned rings_idx;

	/* normal draw/clear cmds: */
	struct fd_ringbuffer *ring;
	struct fd_ringmarker *draw_start, *draw_end;

	/* binning pass draw/clear cmds: */
	struct fd_ringbuffer *binning_ring;
	struct fd_ringmarker *binning_start, *binning_end;

	/* Keep track if WAIT_FOR_IDLE is needed for registers we need
	 * to update via RMW:
	 */
	bool needs_wfi;

	/* Do we need to re-emit RB_FRAME_BUFFER_DIMENSION?  At least on a3xx
	 * it is not a banked context register, so it needs a WFI to update.
	 * Keep track if it has actually changed, to avoid unneeded WFI.
	 * */
	bool needs_rb_fbd;

	/* Keep track of DRAW initiators that need to be patched up depending
	 * on whether we using binning or not:
	 */
	struct util_dynarray draw_patches;

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
	struct fd_vsc_pipe      pipe[8];
	struct fd_tile          tile[64];

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
	void (*emit_tile_prep)(struct fd_context *ctx, struct fd_tile *tile);
	void (*emit_tile_mem2gmem)(struct fd_context *ctx, struct fd_tile *tile);
	void (*emit_tile_renderprep)(struct fd_context *ctx, struct fd_tile *tile);
	void (*emit_tile_gmem2mem)(struct fd_context *ctx, struct fd_tile *tile);

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

static INLINE bool
fd_supported_prim(struct fd_context *ctx, unsigned prim)
{
	return (1 << prim) & ctx->primtype_mask;
}

static INLINE void
fd_reset_wfi(struct fd_context *ctx)
{
	ctx->needs_wfi = true;
}

/* emit a WAIT_FOR_IDLE only if needed, ie. if there has not already
 * been one since last draw:
 */
static inline void
fd_wfi(struct fd_context *ctx, struct fd_ringbuffer *ring)
{
	if (ctx->needs_wfi) {
		OUT_WFI(ring);
		ctx->needs_wfi = false;
	}
}

struct pipe_context * fd_context_init(struct fd_context *ctx,
		struct pipe_screen *pscreen, const uint8_t *primtypes,
		void *priv);

void fd_context_render(struct pipe_context *pctx);

void fd_context_destroy(struct pipe_context *pctx);

#endif /* FREEDRENO_CONTEXT_H_ */

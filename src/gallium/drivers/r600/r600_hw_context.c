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
#include "r600_pipe.h"
#include "r600d.h"
#include "util/u_memory.h"
#include <errno.h>
#include <unistd.h>


void r600_need_cs_space(struct r600_context *ctx, unsigned num_dw,
			boolean count_draw_in)
{

	if (!ctx->b.ws->cs_memory_below_limit(ctx->b.rings.gfx.cs, ctx->b.vram, ctx->b.gtt)) {
		ctx->b.gtt = 0;
		ctx->b.vram = 0;
		ctx->b.rings.gfx.flush(ctx, RADEON_FLUSH_ASYNC, NULL);
		return;
	}
	/* all will be accounted once relocation are emited */
	ctx->b.gtt = 0;
	ctx->b.vram = 0;

	/* The number of dwords we already used in the CS so far. */
	num_dw += ctx->b.rings.gfx.cs->cdw;

	if (count_draw_in) {
		unsigned i;

		/* The number of dwords all the dirty states would take. */
		for (i = 0; i < R600_NUM_ATOMS; i++) {
			if (ctx->atoms[i] && ctx->atoms[i]->dirty) {
				num_dw += ctx->atoms[i]->num_dw;
				if (ctx->screen->b.trace_bo) {
					num_dw += R600_TRACE_CS_DWORDS;
				}
			}
		}

		/* The upper-bound of how much space a draw command would take. */
		num_dw += R600_MAX_FLUSH_CS_DWORDS + R600_MAX_DRAW_CS_DWORDS;
		if (ctx->screen->b.trace_bo) {
			num_dw += R600_TRACE_CS_DWORDS;
		}
	}

	/* Count in queries_suspend. */
	num_dw += ctx->b.num_cs_dw_nontimer_queries_suspend;

	/* Count in streamout_end at the end of CS. */
	if (ctx->b.streamout.begin_emitted) {
		num_dw += ctx->b.streamout.num_dw_for_end;
	}

	/* Count in render_condition(NULL) at the end of CS. */
	if (ctx->b.predicate_drawing) {
		num_dw += 3;
	}

	/* SX_MISC */
	if (ctx->b.chip_class == R600) {
		num_dw += 3;
	}

	/* Count in framebuffer cache flushes at the end of CS. */
	num_dw += R600_MAX_FLUSH_CS_DWORDS;

	/* The fence at the end of CS. */
	num_dw += 10;

	/* Flush if there's not enough space. */
	if (num_dw > RADEON_MAX_CMDBUF_DWORDS) {
		ctx->b.rings.gfx.flush(ctx, RADEON_FLUSH_ASYNC, NULL);
	}
}

void r600_flush_emit(struct r600_context *rctx)
{
	struct radeon_winsys_cs *cs = rctx->b.rings.gfx.cs;
	unsigned cp_coher_cntl = 0;
	unsigned wait_until = 0;

	if (!rctx->b.flags) {
		return;
	}

	if (rctx->b.flags & R600_CONTEXT_WAIT_3D_IDLE) {
		wait_until |= S_008040_WAIT_3D_IDLE(1);
	}
	if (rctx->b.flags & R600_CONTEXT_WAIT_CP_DMA_IDLE) {
		wait_until |= S_008040_WAIT_CP_DMA_IDLE(1);
	}

	if (wait_until) {
		/* Use of WAIT_UNTIL is deprecated on Cayman+ */
		if (rctx->b.family >= CHIP_CAYMAN) {
			/* emit a PS partial flush on Cayman/TN */
			rctx->b.flags |= R600_CONTEXT_PS_PARTIAL_FLUSH;
		}
	}

	if (rctx->b.flags & R600_CONTEXT_PS_PARTIAL_FLUSH) {
		cs->buf[cs->cdw++] = PKT3(PKT3_EVENT_WRITE, 0, 0);
		cs->buf[cs->cdw++] = EVENT_TYPE(EVENT_TYPE_PS_PARTIAL_FLUSH) | EVENT_INDEX(4);
	}

	if (rctx->b.chip_class >= R700 &&
	    (rctx->b.flags & R600_CONTEXT_FLUSH_AND_INV_CB_META)) {
		cs->buf[cs->cdw++] = PKT3(PKT3_EVENT_WRITE, 0, 0);
		cs->buf[cs->cdw++] = EVENT_TYPE(EVENT_TYPE_FLUSH_AND_INV_CB_META) | EVENT_INDEX(0);
	}

	if (rctx->b.chip_class >= R700 &&
	    (rctx->b.flags & R600_CONTEXT_FLUSH_AND_INV_DB_META)) {
		cs->buf[cs->cdw++] = PKT3(PKT3_EVENT_WRITE, 0, 0);
		cs->buf[cs->cdw++] = EVENT_TYPE(EVENT_TYPE_FLUSH_AND_INV_DB_META) | EVENT_INDEX(0);

		/* Set FULL_CACHE_ENA for DB META flushes on r7xx and later.
		 *
		 * This hack predates use of FLUSH_AND_INV_DB_META, so it's
		 * unclear whether it's still needed or even whether it has
		 * any effect.
		 */
		cp_coher_cntl |= S_0085F0_FULL_CACHE_ENA(1);
	}

	if (rctx->b.flags & R600_CONTEXT_FLUSH_AND_INV) {
		cs->buf[cs->cdw++] = PKT3(PKT3_EVENT_WRITE, 0, 0);
		cs->buf[cs->cdw++] = EVENT_TYPE(EVENT_TYPE_CACHE_FLUSH_AND_INV_EVENT) | EVENT_INDEX(0);
	}

	if (rctx->b.flags & R600_CONTEXT_INV_CONST_CACHE) {
		/* Direct constant addressing uses the shader cache.
		 * Indirect contant addressing uses the vertex cache. */
		cp_coher_cntl |= S_0085F0_SH_ACTION_ENA(1) |
				 (rctx->has_vertex_cache ? S_0085F0_VC_ACTION_ENA(1)
							 : S_0085F0_TC_ACTION_ENA(1));
	}
	if (rctx->b.flags & R600_CONTEXT_INV_VERTEX_CACHE) {
		cp_coher_cntl |= rctx->has_vertex_cache ? S_0085F0_VC_ACTION_ENA(1)
							: S_0085F0_TC_ACTION_ENA(1);
	}
	if (rctx->b.flags & R600_CONTEXT_INV_TEX_CACHE) {
		/* Textures use the texture cache.
		 * Texture buffer objects use the vertex cache. */
		cp_coher_cntl |= S_0085F0_TC_ACTION_ENA(1) |
				 (rctx->has_vertex_cache ? S_0085F0_VC_ACTION_ENA(1) : 0);
	}

	/* Don't use the DB CP COHER logic on r6xx.
	 * There are hw bugs.
	 */
	if (rctx->b.chip_class >= R700 &&
	    (rctx->b.flags & R600_CONTEXT_FLUSH_AND_INV_DB)) {
		cp_coher_cntl |= S_0085F0_DB_ACTION_ENA(1) |
				S_0085F0_DB_DEST_BASE_ENA(1) |
				S_0085F0_SMX_ACTION_ENA(1);
	}

	/* Don't use the CB CP COHER logic on r6xx.
	 * There are hw bugs.
	 */
	if (rctx->b.chip_class >= R700 &&
	    (rctx->b.flags & R600_CONTEXT_FLUSH_AND_INV_CB)) {
		cp_coher_cntl |= S_0085F0_CB_ACTION_ENA(1) |
				S_0085F0_CB0_DEST_BASE_ENA(1) |
				S_0085F0_CB1_DEST_BASE_ENA(1) |
				S_0085F0_CB2_DEST_BASE_ENA(1) |
				S_0085F0_CB3_DEST_BASE_ENA(1) |
				S_0085F0_CB4_DEST_BASE_ENA(1) |
				S_0085F0_CB5_DEST_BASE_ENA(1) |
				S_0085F0_CB6_DEST_BASE_ENA(1) |
				S_0085F0_CB7_DEST_BASE_ENA(1) |
				S_0085F0_SMX_ACTION_ENA(1);
		if (rctx->b.chip_class >= EVERGREEN)
			cp_coher_cntl |= S_0085F0_CB8_DEST_BASE_ENA(1) |
					S_0085F0_CB9_DEST_BASE_ENA(1) |
					S_0085F0_CB10_DEST_BASE_ENA(1) |
					S_0085F0_CB11_DEST_BASE_ENA(1);
	}

	if (rctx->b.flags & R600_CONTEXT_STREAMOUT_FLUSH) {
		cp_coher_cntl |= S_0085F0_SO0_DEST_BASE_ENA(1) |
				S_0085F0_SO1_DEST_BASE_ENA(1) |
				S_0085F0_SO2_DEST_BASE_ENA(1) |
				S_0085F0_SO3_DEST_BASE_ENA(1) |
				S_0085F0_SMX_ACTION_ENA(1);
	}

	/* Workaround for buggy flushing on some R6xx chipsets. */
	if (rctx->b.flags & R600_CONTEXT_FLUSH_AND_INV &&
	    (rctx->b.family == CHIP_RV670 ||
	     rctx->b.family == CHIP_RS780 ||
	     rctx->b.family == CHIP_RS880)) {
		cp_coher_cntl |=  S_0085F0_CB1_DEST_BASE_ENA(1) |
				  S_0085F0_DEST_BASE_0_ENA(1);
	}

	if (cp_coher_cntl) {
		cs->buf[cs->cdw++] = PKT3(PKT3_SURFACE_SYNC, 3, 0);
		cs->buf[cs->cdw++] = cp_coher_cntl;   /* CP_COHER_CNTL */
		cs->buf[cs->cdw++] = 0xffffffff;      /* CP_COHER_SIZE */
		cs->buf[cs->cdw++] = 0;               /* CP_COHER_BASE */
		cs->buf[cs->cdw++] = 0x0000000A;      /* POLL_INTERVAL */
	}

	if (wait_until) {
		/* Use of WAIT_UNTIL is deprecated on Cayman+ */
		if (rctx->b.family < CHIP_CAYMAN) {
			/* wait for things to settle */
			r600_write_config_reg(cs, R_008040_WAIT_UNTIL, wait_until);
		}
	}

	/* everything is properly flushed */
	rctx->b.flags = 0;
}

void r600_context_gfx_flush(void *context, unsigned flags,
			    struct pipe_fence_handle **fence)
{
	struct r600_context *ctx = context;
	struct radeon_winsys_cs *cs = ctx->b.rings.gfx.cs;

	if (cs->cdw == ctx->b.initial_gfx_cs_size && !fence)
		return;

	ctx->b.rings.gfx.flushing = true;

	r600_preflush_suspend_features(&ctx->b);

	/* flush the framebuffer cache */
	ctx->b.flags |= R600_CONTEXT_FLUSH_AND_INV |
		      R600_CONTEXT_FLUSH_AND_INV_CB |
		      R600_CONTEXT_FLUSH_AND_INV_DB |
		      R600_CONTEXT_FLUSH_AND_INV_CB_META |
		      R600_CONTEXT_FLUSH_AND_INV_DB_META |
		      R600_CONTEXT_WAIT_3D_IDLE |
		      R600_CONTEXT_WAIT_CP_DMA_IDLE;

	r600_flush_emit(ctx);

	/* old kernels and userspace don't set SX_MISC, so we must reset it to 0 here */
	if (ctx->b.chip_class == R600) {
		r600_write_context_reg(cs, R_028350_SX_MISC, 0);
	}

	/* force to keep tiling flags */
	if (ctx->keep_tiling_flags) {
		flags |= RADEON_FLUSH_KEEP_TILING_FLAGS;
	}

	/* Flush the CS. */
	ctx->b.ws->cs_flush(cs, flags, fence, ctx->screen->b.cs_count++);
	ctx->b.rings.gfx.flushing = false;

	r600_begin_new_cs(ctx);
}

void r600_begin_new_cs(struct r600_context *ctx)
{
	unsigned shader;
	int i;
	ctx->b.flags = 0;
	ctx->b.gtt = 0;
	ctx->b.vram = 0;

	/* Begin a new CS. */
	r600_emit_command_buffer(ctx->b.rings.gfx.cs, &ctx->start_cs_cmd);

	/* Re-emit states. */
	ctx->alphatest_state.atom.dirty = true;
	ctx->blend_color.atom.dirty = true;
	ctx->cb_misc_state.atom.dirty = true;
	ctx->clip_misc_state.atom.dirty = true;
	ctx->clip_state.atom.dirty = true;
	ctx->db_misc_state.atom.dirty = true;
	ctx->db_state.atom.dirty = true;
	ctx->framebuffer.atom.dirty = true;
	ctx->pixel_shader.atom.dirty = true;
	ctx->poly_offset_state.atom.dirty = true;
	ctx->vgt_state.atom.dirty = true;
	ctx->sample_mask.atom.dirty = true;
	for (i = 0; i < 16; i++) {
		ctx->scissor[i].atom.dirty = true;
		ctx->viewport[i].atom.dirty = true;
	}
	ctx->config_state.atom.dirty = true;
	ctx->stencil_ref.atom.dirty = true;
	ctx->vertex_fetch_shader.atom.dirty = true;
	ctx->export_shader.atom.dirty = true;
	if (ctx->gs_shader) {
		ctx->geometry_shader.atom.dirty = true;
		ctx->shader_stages.atom.dirty = true;
		ctx->gs_rings.atom.dirty = true;
	}
	ctx->vertex_shader.atom.dirty = true;
	ctx->b.streamout.enable_atom.dirty = true;

	if (ctx->blend_state.cso)
		ctx->blend_state.atom.dirty = true;
	if (ctx->dsa_state.cso)
		ctx->dsa_state.atom.dirty = true;
	if (ctx->rasterizer_state.cso)
		ctx->rasterizer_state.atom.dirty = true;

	if (ctx->b.chip_class <= R700) {
		ctx->seamless_cube_map.atom.dirty = true;
	}

	ctx->vertex_buffer_state.dirty_mask = ctx->vertex_buffer_state.enabled_mask;
	r600_vertex_buffers_dirty(ctx);

	/* Re-emit shader resources. */
	for (shader = 0; shader < PIPE_SHADER_TYPES; shader++) {
		struct r600_constbuf_state *constbuf = &ctx->constbuf_state[shader];
		struct r600_textures_info *samplers = &ctx->samplers[shader];

		constbuf->dirty_mask = constbuf->enabled_mask;
		samplers->views.dirty_mask = samplers->views.enabled_mask;
		samplers->states.dirty_mask = samplers->states.enabled_mask;

		r600_constant_buffers_dirty(ctx, constbuf);
		r600_sampler_views_dirty(ctx, &samplers->views);
		r600_sampler_states_dirty(ctx, &samplers->states);
	}

	r600_postflush_resume_features(&ctx->b);

	/* Re-emit the draw state. */
	ctx->last_primitive_type = -1;
	ctx->last_start_instance = -1;

	ctx->b.initial_gfx_cs_size = ctx->b.rings.gfx.cs->cdw;
}

/* The max number of bytes to copy per packet. */
#define CP_DMA_MAX_BYTE_COUNT ((1 << 21) - 8)

void r600_cp_dma_copy_buffer(struct r600_context *rctx,
			     struct pipe_resource *dst, uint64_t dst_offset,
			     struct pipe_resource *src, uint64_t src_offset,
			     unsigned size)
{
	struct radeon_winsys_cs *cs = rctx->b.rings.gfx.cs;

	assert(size);
	assert(rctx->screen->b.has_cp_dma);

	/* Mark the buffer range of destination as valid (initialized),
	 * so that transfer_map knows it should wait for the GPU when mapping
	 * that range. */
	util_range_add(&r600_resource(dst)->valid_buffer_range, dst_offset,
		       dst_offset + size);

	dst_offset += r600_resource_va(&rctx->screen->b.b, dst);
	src_offset += r600_resource_va(&rctx->screen->b.b, src);

	/* Flush the caches where the resources are bound. */
	rctx->b.flags |= R600_CONTEXT_INV_CONST_CACHE |
			 R600_CONTEXT_INV_VERTEX_CACHE |
			 R600_CONTEXT_INV_TEX_CACHE |
			 R600_CONTEXT_FLUSH_AND_INV |
			 R600_CONTEXT_FLUSH_AND_INV_CB |
			 R600_CONTEXT_FLUSH_AND_INV_DB |
			 R600_CONTEXT_FLUSH_AND_INV_CB_META |
			 R600_CONTEXT_FLUSH_AND_INV_DB_META |
			 R600_CONTEXT_STREAMOUT_FLUSH |
			 R600_CONTEXT_WAIT_3D_IDLE;

	/* There are differences between R700 and EG in CP DMA,
	 * but we only use the common bits here. */
	while (size) {
		unsigned sync = 0;
		unsigned byte_count = MIN2(size, CP_DMA_MAX_BYTE_COUNT);
		unsigned src_reloc, dst_reloc;

		r600_need_cs_space(rctx, 10 + (rctx->b.flags ? R600_MAX_FLUSH_CS_DWORDS : 0), FALSE);

		/* Flush the caches for the first copy only. */
		if (rctx->b.flags) {
			r600_flush_emit(rctx);
		}

		/* Do the synchronization after the last copy, so that all data is written to memory. */
		if (size == byte_count) {
			sync = PKT3_CP_DMA_CP_SYNC;
		}

		/* This must be done after r600_need_cs_space. */
		src_reloc = r600_context_bo_reloc(&rctx->b, &rctx->b.rings.gfx, (struct r600_resource*)src,
						  RADEON_USAGE_READ, RADEON_PRIO_MIN);
		dst_reloc = r600_context_bo_reloc(&rctx->b, &rctx->b.rings.gfx, (struct r600_resource*)dst,
						  RADEON_USAGE_WRITE, RADEON_PRIO_MIN);

		radeon_emit(cs, PKT3(PKT3_CP_DMA, 4, 0));
		radeon_emit(cs, src_offset);	/* SRC_ADDR_LO [31:0] */
		radeon_emit(cs, sync | ((src_offset >> 32) & 0xff));		/* CP_SYNC [31] | SRC_ADDR_HI [7:0] */
		radeon_emit(cs, dst_offset);	/* DST_ADDR_LO [31:0] */
		radeon_emit(cs, (dst_offset >> 32) & 0xff);		/* DST_ADDR_HI [7:0] */
		radeon_emit(cs, byte_count);	/* COMMAND [29:22] | BYTE_COUNT [20:0] */

		radeon_emit(cs, PKT3(PKT3_NOP, 0, 0));
		radeon_emit(cs, src_reloc);
		radeon_emit(cs, PKT3(PKT3_NOP, 0, 0));
		radeon_emit(cs, dst_reloc);

		size -= byte_count;
		src_offset += byte_count;
		dst_offset += byte_count;
	}

	/* Invalidate the read caches. */
	rctx->b.flags |= R600_CONTEXT_INV_CONST_CACHE |
			 R600_CONTEXT_INV_VERTEX_CACHE |
			 R600_CONTEXT_INV_TEX_CACHE;
}

void r600_dma_copy_buffer(struct r600_context *rctx,
			  struct pipe_resource *dst,
			  struct pipe_resource *src,
			  uint64_t dst_offset,
			  uint64_t src_offset,
			  uint64_t size)
{
	struct radeon_winsys_cs *cs = rctx->b.rings.dma.cs;
	unsigned i, ncopy, csize;
	struct r600_resource *rdst = (struct r600_resource*)dst;
	struct r600_resource *rsrc = (struct r600_resource*)src;

	/* Mark the buffer range of destination as valid (initialized),
	 * so that transfer_map knows it should wait for the GPU when mapping
	 * that range. */
	util_range_add(&rdst->valid_buffer_range, dst_offset,
		       dst_offset + size);

	size >>= 2; /* convert to dwords */
	ncopy = (size / R600_DMA_COPY_MAX_SIZE_DW) + !!(size % R600_DMA_COPY_MAX_SIZE_DW);

	r600_need_dma_space(&rctx->b, ncopy * 5);
	for (i = 0; i < ncopy; i++) {
		csize = size < R600_DMA_COPY_MAX_SIZE_DW ? size : R600_DMA_COPY_MAX_SIZE_DW;
		/* emit reloc before writting cs so that cs is always in consistent state */
		r600_context_bo_reloc(&rctx->b, &rctx->b.rings.dma, rsrc, RADEON_USAGE_READ,
				      RADEON_PRIO_MIN);
		r600_context_bo_reloc(&rctx->b, &rctx->b.rings.dma, rdst, RADEON_USAGE_WRITE,
				      RADEON_PRIO_MIN);
		cs->buf[cs->cdw++] = DMA_PACKET(DMA_PACKET_COPY, 0, 0, csize);
		cs->buf[cs->cdw++] = dst_offset & 0xfffffffc;
		cs->buf[cs->cdw++] = src_offset & 0xfffffffc;
		cs->buf[cs->cdw++] = (dst_offset >> 32UL) & 0xff;
		cs->buf[cs->cdw++] = (src_offset >> 32UL) & 0xff;
		dst_offset += csize << 2;
		src_offset += csize << 2;
		size -= csize;
	}
}

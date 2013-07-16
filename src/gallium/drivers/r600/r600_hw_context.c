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

/* Get backends mask */
void r600_get_backend_mask(struct r600_context *ctx)
{
	struct radeon_winsys_cs *cs = ctx->rings.gfx.cs;
	struct r600_resource *buffer;
	uint32_t *results;
	unsigned num_backends = ctx->screen->info.r600_num_backends;
	unsigned i, mask = 0;
	uint64_t va;

	/* if backend_map query is supported by the kernel */
	if (ctx->screen->info.r600_backend_map_valid) {
		unsigned num_tile_pipes = ctx->screen->info.r600_num_tile_pipes;
		unsigned backend_map = ctx->screen->info.r600_backend_map;
		unsigned item_width, item_mask;

		if (ctx->chip_class >= EVERGREEN) {
			item_width = 4;
			item_mask = 0x7;
		} else {
			item_width = 2;
			item_mask = 0x3;
		}

		while(num_tile_pipes--) {
			i = backend_map & item_mask;
			mask |= (1<<i);
			backend_map >>= item_width;
		}
		if (mask != 0) {
			ctx->backend_mask = mask;
			return;
		}
	}

	/* otherwise backup path for older kernels */

	/* create buffer for event data */
	buffer = (struct r600_resource*)
		pipe_buffer_create(&ctx->screen->screen, PIPE_BIND_CUSTOM,
				   PIPE_USAGE_STAGING, ctx->max_db*16);
	if (!buffer)
		goto err;
	va = r600_resource_va(&ctx->screen->screen, (void*)buffer);

	/* initialize buffer with zeroes */
	results = r600_buffer_mmap_sync_with_rings(ctx, buffer, PIPE_TRANSFER_WRITE);
	if (results) {
		memset(results, 0, ctx->max_db * 4 * 4);
		ctx->ws->buffer_unmap(buffer->cs_buf);

		/* emit EVENT_WRITE for ZPASS_DONE */
		cs->buf[cs->cdw++] = PKT3(PKT3_EVENT_WRITE, 2, 0);
		cs->buf[cs->cdw++] = EVENT_TYPE(EVENT_TYPE_ZPASS_DONE) | EVENT_INDEX(1);
		cs->buf[cs->cdw++] = va;
		cs->buf[cs->cdw++] = (va >> 32UL) & 0xFF;

		cs->buf[cs->cdw++] = PKT3(PKT3_NOP, 0, 0);
		cs->buf[cs->cdw++] = r600_context_bo_reloc(ctx, &ctx->rings.gfx, buffer, RADEON_USAGE_WRITE);

		/* analyze results */
		results = r600_buffer_mmap_sync_with_rings(ctx, buffer, PIPE_TRANSFER_READ);
		if (results) {
			for(i = 0; i < ctx->max_db; i++) {
				/* at least highest bit will be set if backend is used */
				if (results[i*4 + 1])
					mask |= (1<<i);
			}
			ctx->ws->buffer_unmap(buffer->cs_buf);
		}
	}

	pipe_resource_reference((struct pipe_resource**)&buffer, NULL);

	if (mask != 0) {
		ctx->backend_mask = mask;
		return;
	}

err:
	/* fallback to old method - set num_backends lower bits to 1 */
	ctx->backend_mask = (~((uint32_t)0))>>(32-num_backends);
	return;
}

void r600_need_cs_space(struct r600_context *ctx, unsigned num_dw,
			boolean count_draw_in)
{
	if (!ctx->ws->cs_memory_below_limit(ctx->rings.gfx.cs, ctx->vram, ctx->gtt)) {
		ctx->gtt = 0;
		ctx->vram = 0;
		ctx->rings.gfx.flush(ctx, RADEON_FLUSH_ASYNC);
		return;
	}
	/* all will be accounted once relocation are emited */
	ctx->gtt = 0;
	ctx->vram = 0;

	/* The number of dwords we already used in the CS so far. */
	num_dw += ctx->rings.gfx.cs->cdw;

	if (count_draw_in) {
		unsigned i;

		/* The number of dwords all the dirty states would take. */
		for (i = 0; i < R600_NUM_ATOMS; i++) {
			if (ctx->atoms[i] && ctx->atoms[i]->dirty) {
				num_dw += ctx->atoms[i]->num_dw;
				if (ctx->screen->trace_bo) {
					num_dw += R600_TRACE_CS_DWORDS;
				}
			}
		}

		/* The upper-bound of how much space a draw command would take. */
		num_dw += R600_MAX_FLUSH_CS_DWORDS + R600_MAX_DRAW_CS_DWORDS;
		if (ctx->screen->trace_bo) {
			num_dw += R600_TRACE_CS_DWORDS;
		}
	}

	/* Count in queries_suspend. */
	num_dw += ctx->num_cs_dw_nontimer_queries_suspend;

	/* Count in streamout_end at the end of CS. */
	if (ctx->streamout.begin_emitted) {
		num_dw += ctx->streamout.num_dw_for_end;
	}

	/* Count in render_condition(NULL) at the end of CS. */
	if (ctx->predicate_drawing) {
		num_dw += 3;
	}

	/* SX_MISC */
	if (ctx->chip_class <= R700) {
		num_dw += 3;
	}

	/* Count in framebuffer cache flushes at the end of CS. */
	num_dw += R600_MAX_FLUSH_CS_DWORDS;

	/* The fence at the end of CS. */
	num_dw += 10;

	/* Flush if there's not enough space. */
	if (num_dw > RADEON_MAX_CMDBUF_DWORDS) {
		ctx->rings.gfx.flush(ctx, RADEON_FLUSH_ASYNC);
	}
}

void r600_flush_emit(struct r600_context *rctx)
{
	struct radeon_winsys_cs *cs = rctx->rings.gfx.cs;
	unsigned cp_coher_cntl = 0;
	unsigned wait_until = 0;

	if (!rctx->flags) {
		return;
	}

	if (rctx->flags & R600_CONTEXT_WAIT_3D_IDLE) {
		wait_until |= S_008040_WAIT_3D_IDLE(1);
	}
	if (rctx->flags & R600_CONTEXT_WAIT_CP_DMA_IDLE) {
		wait_until |= S_008040_WAIT_CP_DMA_IDLE(1);
	}

	if (wait_until) {
		/* Use of WAIT_UNTIL is deprecated on Cayman+ */
		if (rctx->family >= CHIP_CAYMAN) {
			/* emit a PS partial flush on Cayman/TN */
			rctx->flags |= R600_CONTEXT_PS_PARTIAL_FLUSH;
		}
	}

	if (rctx->flags & R600_CONTEXT_PS_PARTIAL_FLUSH) {
		cs->buf[cs->cdw++] = PKT3(PKT3_EVENT_WRITE, 0, 0);
		cs->buf[cs->cdw++] = EVENT_TYPE(EVENT_TYPE_PS_PARTIAL_FLUSH) | EVENT_INDEX(4);
	}

	if (rctx->chip_class >= R700 &&
	    (rctx->flags & R600_CONTEXT_FLUSH_AND_INV_CB_META)) {
		cs->buf[cs->cdw++] = PKT3(PKT3_EVENT_WRITE, 0, 0);
		cs->buf[cs->cdw++] = EVENT_TYPE(EVENT_TYPE_FLUSH_AND_INV_CB_META) | EVENT_INDEX(0);
	}

	if (rctx->chip_class >= R700 &&
	    (rctx->flags & R600_CONTEXT_FLUSH_AND_INV_DB_META)) {
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

	if (rctx->flags & R600_CONTEXT_FLUSH_AND_INV) {
		cs->buf[cs->cdw++] = PKT3(PKT3_EVENT_WRITE, 0, 0);
		cs->buf[cs->cdw++] = EVENT_TYPE(EVENT_TYPE_CACHE_FLUSH_AND_INV_EVENT) | EVENT_INDEX(0);
	}

	if (rctx->flags & R600_CONTEXT_INV_CONST_CACHE) {
		cp_coher_cntl |= S_0085F0_SH_ACTION_ENA(1);
	}
	if (rctx->flags & R600_CONTEXT_INV_VERTEX_CACHE) {
		cp_coher_cntl |= rctx->has_vertex_cache ? S_0085F0_VC_ACTION_ENA(1)
							: S_0085F0_TC_ACTION_ENA(1);
	}
	if (rctx->flags & R600_CONTEXT_INV_TEX_CACHE) {
		cp_coher_cntl |= S_0085F0_TC_ACTION_ENA(1);
	}

	/* Don't use the DB CP COHER logic on r6xx.
	 * There are hw bugs.
	 */
	if (rctx->chip_class >= R700 &&
	    (rctx->flags & R600_CONTEXT_FLUSH_AND_INV_DB)) {
		cp_coher_cntl |= S_0085F0_DB_ACTION_ENA(1) |
				S_0085F0_DB_DEST_BASE_ENA(1) |
				S_0085F0_SMX_ACTION_ENA(1);
	}

	/* Don't use the CB CP COHER logic on r6xx.
	 * There are hw bugs.
	 */
	if (rctx->chip_class >= R700 &&
	    (rctx->flags & R600_CONTEXT_FLUSH_AND_INV_CB)) {
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
		if (rctx->chip_class >= EVERGREEN)
			cp_coher_cntl |= S_0085F0_CB8_DEST_BASE_ENA(1) |
					S_0085F0_CB9_DEST_BASE_ENA(1) |
					S_0085F0_CB10_DEST_BASE_ENA(1) |
					S_0085F0_CB11_DEST_BASE_ENA(1);
	}

	if (rctx->flags & R600_CONTEXT_STREAMOUT_FLUSH) {
		cp_coher_cntl |= S_0085F0_SO0_DEST_BASE_ENA(1) |
				S_0085F0_SO1_DEST_BASE_ENA(1) |
				S_0085F0_SO2_DEST_BASE_ENA(1) |
				S_0085F0_SO3_DEST_BASE_ENA(1) |
				S_0085F0_SMX_ACTION_ENA(1);
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
		if (rctx->family < CHIP_CAYMAN) {
			/* wait for things to settle */
			r600_write_config_reg(cs, R_008040_WAIT_UNTIL, wait_until);
		}
	}

	/* everything is properly flushed */
	rctx->flags = 0;
}

void r600_context_flush(struct r600_context *ctx, unsigned flags)
{
	struct radeon_winsys_cs *cs = ctx->rings.gfx.cs;

	ctx->nontimer_queries_suspended = false;
	ctx->streamout.suspended = false;

	/* suspend queries */
	if (ctx->num_cs_dw_nontimer_queries_suspend) {
		r600_suspend_nontimer_queries(ctx);
		ctx->nontimer_queries_suspended = true;
	}

	if (ctx->streamout.begin_emitted) {
		r600_emit_streamout_end(ctx);
		ctx->streamout.suspended = true;
	}

	/* flush is needed to avoid lockups on some chips with user fences
	 * this will also flush the framebuffer cache
	 */
	ctx->flags |= R600_CONTEXT_FLUSH_AND_INV |
		      R600_CONTEXT_FLUSH_AND_INV_CB |
		      R600_CONTEXT_FLUSH_AND_INV_DB |
		      R600_CONTEXT_FLUSH_AND_INV_CB_META |
		      R600_CONTEXT_FLUSH_AND_INV_DB_META |
		      R600_CONTEXT_WAIT_3D_IDLE |
		      R600_CONTEXT_WAIT_CP_DMA_IDLE;

	r600_flush_emit(ctx);

	/* old kernels and userspace don't set SX_MISC, so we must reset it to 0 here */
	if (ctx->chip_class <= R700) {
		r600_write_context_reg(cs, R_028350_SX_MISC, 0);
	}

	/* force to keep tiling flags */
	if (ctx->keep_tiling_flags) {
		flags |= RADEON_FLUSH_KEEP_TILING_FLAGS;
	}

	/* Flush the CS. */
	ctx->ws->cs_flush(ctx->rings.gfx.cs, flags, ctx->screen->cs_count++);
}

void r600_begin_new_cs(struct r600_context *ctx)
{
	unsigned shader;

	ctx->flags = 0;
	ctx->gtt = 0;
	ctx->vram = 0;

	/* Begin a new CS. */
	r600_emit_command_buffer(ctx->rings.gfx.cs, &ctx->start_cs_cmd);

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
	ctx->scissor.atom.dirty = true;
	ctx->config_state.atom.dirty = true;
	ctx->stencil_ref.atom.dirty = true;
	ctx->vertex_fetch_shader.atom.dirty = true;
	ctx->vertex_shader.atom.dirty = true;
	ctx->viewport.atom.dirty = true;

	if (ctx->blend_state.cso)
		ctx->blend_state.atom.dirty = true;
	if (ctx->dsa_state.cso)
		ctx->dsa_state.atom.dirty = true;
	if (ctx->rasterizer_state.cso)
		ctx->rasterizer_state.atom.dirty = true;

	if (ctx->chip_class <= R700) {
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

	if (ctx->streamout.suspended) {
		ctx->streamout.append_bitmask = ctx->streamout.enabled_mask;
		r600_streamout_buffers_dirty(ctx);
	}

	/* resume queries */
	if (ctx->nontimer_queries_suspended) {
		r600_resume_nontimer_queries(ctx);
	}

	/* Re-emit the draw state. */
	ctx->last_primitive_type = -1;
	ctx->last_start_instance = -1;

	ctx->initial_gfx_cs_size = ctx->rings.gfx.cs->cdw;
}

void r600_context_emit_fence(struct r600_context *ctx, struct r600_resource *fence_bo, unsigned offset, unsigned value)
{
	struct radeon_winsys_cs *cs = ctx->rings.gfx.cs;
	uint64_t va;

	r600_need_cs_space(ctx, 10, FALSE);

	va = r600_resource_va(&ctx->screen->screen, (void*)fence_bo);
	va = va + (offset << 2);

	/* Use of WAIT_UNTIL is deprecated on Cayman+ */
	if (ctx->family >= CHIP_CAYMAN) {
		cs->buf[cs->cdw++] = PKT3(PKT3_EVENT_WRITE, 0, 0);
		cs->buf[cs->cdw++] = EVENT_TYPE(EVENT_TYPE_PS_PARTIAL_FLUSH) | EVENT_INDEX(4);
	} else {
		r600_write_config_reg(cs, R_008040_WAIT_UNTIL, S_008040_WAIT_3D_IDLE(1));
	}

	cs->buf[cs->cdw++] = PKT3(PKT3_EVENT_WRITE_EOP, 4, 0);
	cs->buf[cs->cdw++] = EVENT_TYPE(EVENT_TYPE_CACHE_FLUSH_AND_INV_TS_EVENT) | EVENT_INDEX(5);
	cs->buf[cs->cdw++] = va & 0xFFFFFFFFUL;       /* ADDRESS_LO */
	/* DATA_SEL | INT_EN | ADDRESS_HI */
	cs->buf[cs->cdw++] = (1 << 29) | (0 << 24) | ((va >> 32UL) & 0xFF);
	cs->buf[cs->cdw++] = value;                   /* DATA_LO */
	cs->buf[cs->cdw++] = 0;                       /* DATA_HI */
	cs->buf[cs->cdw++] = PKT3(PKT3_NOP, 0, 0);
	cs->buf[cs->cdw++] = r600_context_bo_reloc(ctx, &ctx->rings.gfx, fence_bo, RADEON_USAGE_WRITE);
}

static void r600_flush_vgt_streamout(struct r600_context *ctx)
{
	struct radeon_winsys_cs *cs = ctx->rings.gfx.cs;

	r600_write_config_reg(cs, R_008490_CP_STRMOUT_CNTL, 0);

	cs->buf[cs->cdw++] = PKT3(PKT3_EVENT_WRITE, 0, 0);
	cs->buf[cs->cdw++] = EVENT_TYPE(EVENT_TYPE_SO_VGTSTREAMOUT_FLUSH) | EVENT_INDEX(0);

	cs->buf[cs->cdw++] = PKT3(PKT3_WAIT_REG_MEM, 5, 0);
	cs->buf[cs->cdw++] = WAIT_REG_MEM_EQUAL; /* wait until the register is equal to the reference value */
	cs->buf[cs->cdw++] = R_008490_CP_STRMOUT_CNTL >> 2;  /* register */
	cs->buf[cs->cdw++] = 0;
	cs->buf[cs->cdw++] = S_008490_OFFSET_UPDATE_DONE(1); /* reference value */
	cs->buf[cs->cdw++] = S_008490_OFFSET_UPDATE_DONE(1); /* mask */
	cs->buf[cs->cdw++] = 4; /* poll interval */
}

static void r600_set_streamout_enable(struct r600_context *ctx, unsigned buffer_enable_bit)
{
	struct radeon_winsys_cs *cs = ctx->rings.gfx.cs;

	if (buffer_enable_bit) {
		r600_write_context_reg(cs, R_028AB0_VGT_STRMOUT_EN, S_028AB0_STREAMOUT(1));
		r600_write_context_reg(cs, R_028B20_VGT_STRMOUT_BUFFER_EN, buffer_enable_bit);
	} else {
		r600_write_context_reg(cs, R_028AB0_VGT_STRMOUT_EN, S_028AB0_STREAMOUT(0));
	}
}

void r600_emit_streamout_begin(struct r600_context *ctx, struct r600_atom *atom)
{
	struct radeon_winsys_cs *cs = ctx->rings.gfx.cs;
	struct r600_so_target **t = ctx->streamout.targets;
	unsigned *stride_in_dw = ctx->vs_shader->so.stride;
	unsigned i, update_flags = 0;
	uint64_t va;

	if (ctx->chip_class >= EVERGREEN) {
		evergreen_flush_vgt_streamout(ctx);
		evergreen_set_streamout_enable(ctx, ctx->streamout.enabled_mask);
	} else {
		r600_flush_vgt_streamout(ctx);
		r600_set_streamout_enable(ctx, ctx->streamout.enabled_mask);
	}

	for (i = 0; i < ctx->streamout.num_targets; i++) {
		if (t[i]) {
			t[i]->stride_in_dw = stride_in_dw[i];
			t[i]->so_index = i;
			va = r600_resource_va(&ctx->screen->screen,
					      (void*)t[i]->b.buffer);

			update_flags |= SURFACE_BASE_UPDATE_STRMOUT(i);

			r600_write_context_reg_seq(cs, R_028AD0_VGT_STRMOUT_BUFFER_SIZE_0 + 16*i, 3);
			r600_write_value(cs, (t[i]->b.buffer_offset +
					      t[i]->b.buffer_size) >> 2); /* BUFFER_SIZE (in DW) */
			r600_write_value(cs, stride_in_dw[i]);		  /* VTX_STRIDE (in DW) */
			r600_write_value(cs, va >> 8);			  /* BUFFER_BASE */

			cs->buf[cs->cdw++] = PKT3(PKT3_NOP, 0, 0);
			cs->buf[cs->cdw++] =
				r600_context_bo_reloc(ctx, &ctx->rings.gfx, r600_resource(t[i]->b.buffer),
						      RADEON_USAGE_WRITE);

			/* R7xx requires this packet after updating BUFFER_BASE.
			 * Without this, R7xx locks up. */
			if (ctx->family >= CHIP_RS780 && ctx->family <= CHIP_RV740) {
				cs->buf[cs->cdw++] = PKT3(PKT3_STRMOUT_BASE_UPDATE, 1, 0);
				cs->buf[cs->cdw++] = i;
				cs->buf[cs->cdw++] = va >> 8;

				cs->buf[cs->cdw++] = PKT3(PKT3_NOP, 0, 0);
				cs->buf[cs->cdw++] =
					r600_context_bo_reloc(ctx, &ctx->rings.gfx, r600_resource(t[i]->b.buffer),
							      RADEON_USAGE_WRITE);
			}

			if (ctx->streamout.append_bitmask & (1 << i)) {
				va = r600_resource_va(&ctx->screen->screen,
						      (void*)t[i]->buf_filled_size) + t[i]->buf_filled_size_offset;
				/* Append. */
				cs->buf[cs->cdw++] = PKT3(PKT3_STRMOUT_BUFFER_UPDATE, 4, 0);
				cs->buf[cs->cdw++] = STRMOUT_SELECT_BUFFER(i) |
							       STRMOUT_OFFSET_SOURCE(STRMOUT_OFFSET_FROM_MEM); /* control */
				cs->buf[cs->cdw++] = 0; /* unused */
				cs->buf[cs->cdw++] = 0; /* unused */
				cs->buf[cs->cdw++] = va & 0xFFFFFFFFUL; /* src address lo */
				cs->buf[cs->cdw++] = (va >> 32UL) & 0xFFUL; /* src address hi */

				cs->buf[cs->cdw++] = PKT3(PKT3_NOP, 0, 0);
				cs->buf[cs->cdw++] =
					r600_context_bo_reloc(ctx,  &ctx->rings.gfx, t[i]->buf_filled_size,
							      RADEON_USAGE_READ);
			} else {
				/* Start from the beginning. */
				cs->buf[cs->cdw++] = PKT3(PKT3_STRMOUT_BUFFER_UPDATE, 4, 0);
				cs->buf[cs->cdw++] = STRMOUT_SELECT_BUFFER(i) |
							       STRMOUT_OFFSET_SOURCE(STRMOUT_OFFSET_FROM_PACKET); /* control */
				cs->buf[cs->cdw++] = 0; /* unused */
				cs->buf[cs->cdw++] = 0; /* unused */
				cs->buf[cs->cdw++] = t[i]->b.buffer_offset >> 2; /* buffer offset in DW */
				cs->buf[cs->cdw++] = 0; /* unused */
			}
		}
	}

	if (ctx->family > CHIP_R600 && ctx->family < CHIP_RV770) {
		cs->buf[cs->cdw++] = PKT3(PKT3_SURFACE_BASE_UPDATE, 0, 0);
		cs->buf[cs->cdw++] = update_flags;
	}
	ctx->streamout.begin_emitted = true;
}

void r600_emit_streamout_end(struct r600_context *ctx)
{
	struct radeon_winsys_cs *cs = ctx->rings.gfx.cs;
	struct r600_so_target **t = ctx->streamout.targets;
	unsigned i;
	uint64_t va;

	if (ctx->chip_class >= EVERGREEN) {
		evergreen_flush_vgt_streamout(ctx);
	} else {
		r600_flush_vgt_streamout(ctx);
	}

	for (i = 0; i < ctx->streamout.num_targets; i++) {
		if (t[i]) {
			va = r600_resource_va(&ctx->screen->screen,
					      (void*)t[i]->buf_filled_size) + t[i]->buf_filled_size_offset;
			cs->buf[cs->cdw++] = PKT3(PKT3_STRMOUT_BUFFER_UPDATE, 4, 0);
			cs->buf[cs->cdw++] = STRMOUT_SELECT_BUFFER(i) |
						       STRMOUT_OFFSET_SOURCE(STRMOUT_OFFSET_NONE) |
						       STRMOUT_STORE_BUFFER_FILLED_SIZE; /* control */
			cs->buf[cs->cdw++] = va & 0xFFFFFFFFUL;     /* dst address lo */
			cs->buf[cs->cdw++] = (va >> 32UL) & 0xFFUL; /* dst address hi */
			cs->buf[cs->cdw++] = 0; /* unused */
			cs->buf[cs->cdw++] = 0; /* unused */

			cs->buf[cs->cdw++] = PKT3(PKT3_NOP, 0, 0);
			cs->buf[cs->cdw++] =
				r600_context_bo_reloc(ctx,  &ctx->rings.gfx, t[i]->buf_filled_size,
						      RADEON_USAGE_WRITE);
		}
	}

	if (ctx->chip_class >= EVERGREEN) {
		ctx->flags |= R600_CONTEXT_STREAMOUT_FLUSH;
		evergreen_set_streamout_enable(ctx, 0);
	} else {
		if (ctx->chip_class >= R700) {
			ctx->flags |= R600_CONTEXT_STREAMOUT_FLUSH;
		}
		r600_set_streamout_enable(ctx, 0);
	}
	ctx->flags |= R600_CONTEXT_WAIT_3D_IDLE | R600_CONTEXT_FLUSH_AND_INV;
	ctx->streamout.begin_emitted = false;
}

/* The max number of bytes to copy per packet. */
#define CP_DMA_MAX_BYTE_COUNT ((1 << 21) - 8)

void r600_cp_dma_copy_buffer(struct r600_context *rctx,
			     struct pipe_resource *dst, uint64_t dst_offset,
			     struct pipe_resource *src, uint64_t src_offset,
			     unsigned size)
{
	struct radeon_winsys_cs *cs = rctx->rings.gfx.cs;

	assert(size);
	assert(rctx->screen->has_cp_dma);

	dst_offset += r600_resource_va(&rctx->screen->screen, dst);
	src_offset += r600_resource_va(&rctx->screen->screen, src);

	/* Flush the caches where the resources are bound. */
	r600_flag_resource_cache_flush(rctx, src);
	r600_flag_resource_cache_flush(rctx, dst);
        rctx->flags |= R600_CONTEXT_WAIT_3D_IDLE;

	/* There are differences between R700 and EG in CP DMA,
	 * but we only use the common bits here. */
	while (size) {
		unsigned sync = 0;
		unsigned byte_count = MIN2(size, CP_DMA_MAX_BYTE_COUNT);
		unsigned src_reloc, dst_reloc;

		r600_need_cs_space(rctx, 10 + (rctx->flags ? R600_MAX_FLUSH_CS_DWORDS : 0), FALSE);

		/* Flush the caches for the first copy only. */
		if (rctx->flags) {
			r600_flush_emit(rctx);
		}

		/* Do the synchronization after the last copy, so that all data is written to memory. */
		if (size == byte_count) {
			sync = PKT3_CP_DMA_CP_SYNC;
		}

		/* This must be done after r600_need_cs_space. */
		src_reloc = r600_context_bo_reloc(rctx, &rctx->rings.gfx, (struct r600_resource*)src, RADEON_USAGE_READ);
		dst_reloc = r600_context_bo_reloc(rctx, &rctx->rings.gfx, (struct r600_resource*)dst, RADEON_USAGE_WRITE);

		r600_write_value(cs, PKT3(PKT3_CP_DMA, 4, 0));
		r600_write_value(cs, src_offset);	/* SRC_ADDR_LO [31:0] */
		r600_write_value(cs, sync | ((src_offset >> 32) & 0xff));		/* CP_SYNC [31] | SRC_ADDR_HI [7:0] */
		r600_write_value(cs, dst_offset);	/* DST_ADDR_LO [31:0] */
		r600_write_value(cs, (dst_offset >> 32) & 0xff);		/* DST_ADDR_HI [7:0] */
		r600_write_value(cs, byte_count);	/* COMMAND [29:22] | BYTE_COUNT [20:0] */

		r600_write_value(cs, PKT3(PKT3_NOP, 0, 0));
		r600_write_value(cs, src_reloc);
		r600_write_value(cs, PKT3(PKT3_NOP, 0, 0));
		r600_write_value(cs, dst_reloc);

		size -= byte_count;
		src_offset += byte_count;
		dst_offset += byte_count;
	}

	/* Flush the cache of the dst resource again in case the 3D engine
	 * has been prefetching it. */
	r600_flag_resource_cache_flush(rctx, dst);

	util_range_add(&r600_resource(dst)->valid_buffer_range, dst_offset,
		       dst_offset + size);
}

void r600_need_dma_space(struct r600_context *ctx, unsigned num_dw)
{
	/* The number of dwords we already used in the DMA so far. */
	num_dw += ctx->rings.dma.cs->cdw;
	/* Flush if there's not enough space. */
	if (num_dw > RADEON_MAX_CMDBUF_DWORDS) {
		ctx->rings.dma.flush(ctx, RADEON_FLUSH_ASYNC);
	}
}

void r600_dma_copy(struct r600_context *rctx,
		struct pipe_resource *dst,
		struct pipe_resource *src,
		uint64_t dst_offset,
		uint64_t src_offset,
		uint64_t size)
{
	struct radeon_winsys_cs *cs = rctx->rings.dma.cs;
	unsigned i, ncopy, csize, shift;
	struct r600_resource *rdst = (struct r600_resource*)dst;
	struct r600_resource *rsrc = (struct r600_resource*)src;

	/* make sure that the dma ring is only one active */
	rctx->rings.gfx.flush(rctx, RADEON_FLUSH_ASYNC);

	size >>= 2;
	shift = 2;
	ncopy = (size / 0xffff) + !!(size % 0xffff);

	r600_need_dma_space(rctx, ncopy * 5);
	for (i = 0; i < ncopy; i++) {
		csize = size < 0xffff ? size : 0xffff;
		/* emit reloc before writting cs so that cs is always in consistent state */
		r600_context_bo_reloc(rctx, &rctx->rings.dma, rsrc, RADEON_USAGE_READ);
		r600_context_bo_reloc(rctx, &rctx->rings.dma, rdst, RADEON_USAGE_WRITE);
		cs->buf[cs->cdw++] = DMA_PACKET(DMA_PACKET_COPY, 0, 0, csize);
		cs->buf[cs->cdw++] = dst_offset & 0xfffffffc;
		cs->buf[cs->cdw++] = src_offset & 0xfffffffc;
		cs->buf[cs->cdw++] = (dst_offset >> 32UL) & 0xff;
		cs->buf[cs->cdw++] = (src_offset >> 32UL) & 0xff;
		dst_offset += csize << shift;
		src_offset += csize << shift;
		size -= csize;
	}

	util_range_add(&rdst->valid_buffer_range, dst_offset,
		       dst_offset + size);
}

/* Flag the cache of the resource for it to be flushed later if the resource
 * is bound. Otherwise do nothing. Used for synchronization between engines.
 */
void r600_flag_resource_cache_flush(struct r600_context *rctx,
				    struct pipe_resource *res)
{
	/* Check vertex buffers. */
	uint32_t mask = rctx->vertex_buffer_state.enabled_mask;
	while (mask) {
		uint32_t i = u_bit_scan(&mask);
		if (rctx->vertex_buffer_state.vb[i].buffer == res) {
			rctx->flags |= R600_CONTEXT_INV_VERTEX_CACHE;
		}
	}

	/* Check vertex buffers for compute. */
	mask = rctx->cs_vertex_buffer_state.enabled_mask;
	while (mask) {
		uint32_t i = u_bit_scan(&mask);
		if (rctx->cs_vertex_buffer_state.vb[i].buffer == res) {
			rctx->flags |= R600_CONTEXT_INV_VERTEX_CACHE;
		}
	}

	/* Check constant buffers. */
	unsigned shader;
	for (shader = 0; shader < PIPE_SHADER_TYPES; shader++) {
		struct r600_constbuf_state *state = &rctx->constbuf_state[shader];
		uint32_t mask = state->enabled_mask;

		while (mask) {
			unsigned i = u_bit_scan(&mask);
			if (state->cb[i].buffer == res) {
				rctx->flags |= R600_CONTEXT_INV_CONST_CACHE;

				shader = PIPE_SHADER_TYPES; /* break the outer loop */
				break;
			}
		}
	}

	/* Check textures. */
	for (shader = 0; shader < PIPE_SHADER_TYPES; shader++) {
		struct r600_samplerview_state *state = &rctx->samplers[shader].views;
		uint32_t mask = state->enabled_mask;

		while (mask) {
			uint32_t i = u_bit_scan(&mask);
			if (&state->views[i]->tex_resource->b.b == res) {
				rctx->flags |= R600_CONTEXT_INV_TEX_CACHE;

				shader = PIPE_SHADER_TYPES; /* break the outer loop */
				break;
			}
		}
	}

	/* Check streamout buffers. */
	int i;
	for (i = 0; i < rctx->streamout.num_targets; i++) {
		if (rctx->streamout.targets[i]->b.buffer == res) {
			rctx->flags |= R600_CONTEXT_STREAMOUT_FLUSH |
				       R600_CONTEXT_FLUSH_AND_INV |
				       R600_CONTEXT_WAIT_3D_IDLE;
			break;
		}
	}

	/* Check colorbuffers. */
	for (i = 0; i < rctx->framebuffer.state.nr_cbufs; i++) {
		if (rctx->framebuffer.state.cbufs[i] &&
		    rctx->framebuffer.state.cbufs[i]->texture == res) {
			struct r600_texture *tex =
				(struct r600_texture*)rctx->framebuffer.state.cbufs[i]->texture;

			rctx->flags |= R600_CONTEXT_FLUSH_AND_INV_CB |
				       R600_CONTEXT_FLUSH_AND_INV |
				       R600_CONTEXT_WAIT_3D_IDLE;

			if (tex->cmask_size || tex->fmask_size) {
				rctx->flags |= R600_CONTEXT_FLUSH_AND_INV_CB_META;
			}
			break;
		}
	}

	/* Check a depth buffer. */
	if (rctx->framebuffer.state.zsbuf) {
		if (rctx->framebuffer.state.zsbuf->texture == res) {
			rctx->flags |= R600_CONTEXT_FLUSH_AND_INV_DB |
				       R600_CONTEXT_FLUSH_AND_INV |
				       R600_CONTEXT_WAIT_3D_IDLE;
		}

		struct r600_texture *tex =
			(struct r600_texture*)rctx->framebuffer.state.zsbuf->texture;
		if (tex && tex->htile && &tex->htile->b.b == res) {
			rctx->flags |= R600_CONTEXT_FLUSH_AND_INV_DB_META |
				       R600_CONTEXT_FLUSH_AND_INV |
				       R600_CONTEXT_WAIT_3D_IDLE;
		}
	}
}

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
#include "r600_hw_context_priv.h"
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

static void r600_init_block(struct r600_context *ctx,
			    struct r600_block *block,
			    const struct r600_reg *reg, int index, int nreg,
			    unsigned opcode, unsigned offset_base)
{
	int i = index;
	int j, n = nreg;

	/* initialize block */
	block->flags = 0;
	block->status |= R600_BLOCK_STATUS_DIRTY; /* dirty all blocks at start */
	block->start_offset = reg[i].offset;
	block->pm4[block->pm4_ndwords++] = PKT3(opcode, n, 0);
	block->pm4[block->pm4_ndwords++] = (block->start_offset - offset_base) >> 2;
	block->reg = &block->pm4[block->pm4_ndwords];
	block->pm4_ndwords += n;
	block->nreg = n;
	block->nreg_dirty = n;
	LIST_INITHEAD(&block->list);
	LIST_INITHEAD(&block->enable_list);

	for (j = 0; j < n; j++) {
		if (reg[i+j].flags & REG_FLAG_DIRTY_ALWAYS) {
			block->flags |= REG_FLAG_DIRTY_ALWAYS;
		}
		if (reg[i+j].flags & REG_FLAG_ENABLE_ALWAYS) {
			if (!(block->status & R600_BLOCK_STATUS_ENABLED)) {
				block->status |= R600_BLOCK_STATUS_ENABLED;
				LIST_ADDTAIL(&block->enable_list, &ctx->enable_list);
				LIST_ADDTAIL(&block->list,&ctx->dirty);
			}
		}
		if (reg[i+j].flags & REG_FLAG_FLUSH_CHANGE) {
			block->flags |= REG_FLAG_FLUSH_CHANGE;
		}

		if (reg[i+j].flags & REG_FLAG_NEED_BO) {
			block->nbo++;
			assert(block->nbo < R600_BLOCK_MAX_BO);
			block->pm4_bo_index[j] = block->nbo;
			block->pm4[block->pm4_ndwords++] = PKT3(PKT3_NOP, 0, 0);
			block->pm4[block->pm4_ndwords++] = 0x00000000;
			block->reloc[block->nbo].bo_pm4_index = block->pm4_ndwords - 1;
		}
	}
	/* check that we stay in limit */
	assert(block->pm4_ndwords < R600_BLOCK_MAX_REG);
}

int r600_context_add_block(struct r600_context *ctx, const struct r600_reg *reg, unsigned nreg,
			   unsigned opcode, unsigned offset_base)
{
	struct r600_block *block;
	struct r600_range *range;
	int offset;

	for (unsigned i = 0, n = 0; i < nreg; i += n) {
		/* ignore new block balise */
		if (reg[i].offset == GROUP_FORCE_NEW_BLOCK) {
			n = 1;
			continue;
		}

		/* register that need relocation are in their own group */
		/* find number of consecutive registers */
		n = 0;
		offset = reg[i].offset;
		while (reg[i + n].offset == offset) {
			n++;
			offset += 4;
			if ((n + i) >= nreg)
				break;
			if (n >= (R600_BLOCK_MAX_REG - 2))
				break;
		}

		/* allocate new block */
		block = calloc(1, sizeof(struct r600_block));
		if (block == NULL) {
			return -ENOMEM;
		}
		ctx->nblocks++;
		for (int j = 0; j < n; j++) {
			range = &ctx->range[CTX_RANGE_ID(reg[i + j].offset)];
			/* create block table if it doesn't exist */
			if (!range->blocks)
				range->blocks = calloc(1 << HASH_SHIFT, sizeof(void *));
			if (!range->blocks)
				return -1;

			range->blocks[CTX_BLOCK_ID(reg[i + j].offset)] = block;
		}

		r600_init_block(ctx, block, reg, i, n, opcode, offset_base);

	}
	return 0;
}

static const struct r600_reg r600_context_reg_list[] = {
	{R_028D24_DB_HTILE_SURFACE, 0, 0},
	{R_028614_SPI_VS_OUT_ID_0, 0, 0},
	{R_028618_SPI_VS_OUT_ID_1, 0, 0},
	{R_02861C_SPI_VS_OUT_ID_2, 0, 0},
	{R_028620_SPI_VS_OUT_ID_3, 0, 0},
	{R_028624_SPI_VS_OUT_ID_4, 0, 0},
	{R_028628_SPI_VS_OUT_ID_5, 0, 0},
	{R_02862C_SPI_VS_OUT_ID_6, 0, 0},
	{R_028630_SPI_VS_OUT_ID_7, 0, 0},
	{R_028634_SPI_VS_OUT_ID_8, 0, 0},
	{R_028638_SPI_VS_OUT_ID_9, 0, 0},
	{R_0286C4_SPI_VS_OUT_CONFIG, 0, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028858_SQ_PGM_START_VS, REG_FLAG_NEED_BO, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028868_SQ_PGM_RESOURCES_VS, 0, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_0288A4_SQ_PGM_RESOURCES_FS, 0, 0},
	{R_0288DC_SQ_PGM_CF_OFFSET_FS, 0, 0},
	{R_028644_SPI_PS_INPUT_CNTL_0, 0, 0},
	{R_028648_SPI_PS_INPUT_CNTL_1, 0, 0},
	{R_02864C_SPI_PS_INPUT_CNTL_2, 0, 0},
	{R_028650_SPI_PS_INPUT_CNTL_3, 0, 0},
	{R_028654_SPI_PS_INPUT_CNTL_4, 0, 0},
	{R_028658_SPI_PS_INPUT_CNTL_5, 0, 0},
	{R_02865C_SPI_PS_INPUT_CNTL_6, 0, 0},
	{R_028660_SPI_PS_INPUT_CNTL_7, 0, 0},
	{R_028664_SPI_PS_INPUT_CNTL_8, 0, 0},
	{R_028668_SPI_PS_INPUT_CNTL_9, 0, 0},
	{R_02866C_SPI_PS_INPUT_CNTL_10, 0, 0},
	{R_028670_SPI_PS_INPUT_CNTL_11, 0, 0},
	{R_028674_SPI_PS_INPUT_CNTL_12, 0, 0},
	{R_028678_SPI_PS_INPUT_CNTL_13, 0, 0},
	{R_02867C_SPI_PS_INPUT_CNTL_14, 0, 0},
	{R_028680_SPI_PS_INPUT_CNTL_15, 0, 0},
	{R_028684_SPI_PS_INPUT_CNTL_16, 0, 0},
	{R_028688_SPI_PS_INPUT_CNTL_17, 0, 0},
	{R_02868C_SPI_PS_INPUT_CNTL_18, 0, 0},
	{R_028690_SPI_PS_INPUT_CNTL_19, 0, 0},
	{R_028694_SPI_PS_INPUT_CNTL_20, 0, 0},
	{R_028698_SPI_PS_INPUT_CNTL_21, 0, 0},
	{R_02869C_SPI_PS_INPUT_CNTL_22, 0, 0},
	{R_0286A0_SPI_PS_INPUT_CNTL_23, 0, 0},
	{R_0286A4_SPI_PS_INPUT_CNTL_24, 0, 0},
	{R_0286A8_SPI_PS_INPUT_CNTL_25, 0, 0},
	{R_0286AC_SPI_PS_INPUT_CNTL_26, 0, 0},
	{R_0286B0_SPI_PS_INPUT_CNTL_27, 0, 0},
	{R_0286B4_SPI_PS_INPUT_CNTL_28, 0, 0},
	{R_0286B8_SPI_PS_INPUT_CNTL_29, 0, 0},
	{R_0286BC_SPI_PS_INPUT_CNTL_30, 0, 0},
	{R_0286C0_SPI_PS_INPUT_CNTL_31, 0, 0},
	{R_0286CC_SPI_PS_IN_CONTROL_0, 0, 0},
	{R_0286D0_SPI_PS_IN_CONTROL_1, 0, 0},
	{R_0286D8_SPI_INPUT_Z, 0, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028840_SQ_PGM_START_PS, REG_FLAG_NEED_BO, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028850_SQ_PGM_RESOURCES_PS, 0, 0},
	{R_028854_SQ_PGM_EXPORTS_PS, 0, 0},
};

/* initialize */
void r600_context_fini(struct r600_context *ctx)
{
	struct r600_block *block;
	struct r600_range *range;

	if (ctx->range) {
		for (int i = 0; i < NUM_RANGES; i++) {
			if (!ctx->range[i].blocks)
				continue;
			for (int j = 0; j < (1 << HASH_SHIFT); j++) {
				block = ctx->range[i].blocks[j];
				if (block) {
					for (int k = 0, offset = block->start_offset; k < block->nreg; k++, offset += 4) {
						range = &ctx->range[CTX_RANGE_ID(offset)];
						range->blocks[CTX_BLOCK_ID(offset)] = NULL;
					}
					for (int k = 1; k <= block->nbo; k++) {
						pipe_resource_reference((struct pipe_resource**)&block->reloc[k].bo, NULL);
					}
					free(block);
				}
			}
			free(ctx->range[i].blocks);
		}
	}
	free(ctx->blocks);
}

int r600_setup_block_table(struct r600_context *ctx)
{
	/* setup block table */
	int c = 0;
	ctx->blocks = calloc(ctx->nblocks, sizeof(void*));
	if (!ctx->blocks)
		return -ENOMEM;
	for (int i = 0; i < NUM_RANGES; i++) {
		if (!ctx->range[i].blocks)
			continue;
		for (int j = 0, add; j < (1 << HASH_SHIFT); j++) {
			if (!ctx->range[i].blocks[j])
				continue;

			add = 1;
			for (int k = 0; k < c; k++) {
				if (ctx->blocks[k] == ctx->range[i].blocks[j]) {
					add = 0;
					break;
				}
			}
			if (add) {
				assert(c < ctx->nblocks);
				ctx->blocks[c++] = ctx->range[i].blocks[j];
				j += (ctx->range[i].blocks[j]->nreg) - 1;
			}
		}
	}
	return 0;
}

int r600_context_init(struct r600_context *ctx)
{
	int r;

	/* add blocks */
	r = r600_context_add_block(ctx, r600_context_reg_list,
				   Elements(r600_context_reg_list), PKT3_SET_CONTEXT_REG, R600_CONTEXT_REG_OFFSET);
	if (r)
		goto out_err;

	r = r600_setup_block_table(ctx);
	if (r)
		goto out_err;

	ctx->max_db = 4;
	return 0;
out_err:
	r600_context_fini(ctx);
	return r;
}

void r600_need_cs_space(struct r600_context *ctx, unsigned num_dw,
			boolean count_draw_in)
{
	/* The number of dwords we already used in the CS so far. */
	num_dw += ctx->rings.gfx.cs->cdw;

	if (count_draw_in) {
		unsigned i;

		/* The number of dwords all the dirty states would take. */
		for (i = 0; i < R600_NUM_ATOMS; i++) {
			if (ctx->atoms[i] && ctx->atoms[i]->dirty) {
				num_dw += ctx->atoms[i]->num_dw;
#if R600_TRACE_CS
				if (ctx->screen->trace_bo) {
					num_dw += R600_TRACE_CS_DWORDS;
				}
#endif
			}
		}

		num_dw += ctx->pm4_dirty_cdwords;

		/* The upper-bound of how much space a draw command would take. */
		num_dw += R600_MAX_FLUSH_CS_DWORDS + R600_MAX_DRAW_CS_DWORDS;
#if R600_TRACE_CS
		if (ctx->screen->trace_bo) {
			num_dw += R600_TRACE_CS_DWORDS;
		}
#endif
	}

	/* Count in queries_suspend. */
	num_dw += ctx->num_cs_dw_nontimer_queries_suspend;

	/* Count in streamout_end at the end of CS. */
	num_dw += ctx->num_cs_dw_streamout_end;

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

void r600_context_dirty_block(struct r600_context *ctx,
			      struct r600_block *block,
			      int dirty, int index)
{
	if ((index + 1) > block->nreg_dirty)
		block->nreg_dirty = index + 1;

	if ((dirty != (block->status & R600_BLOCK_STATUS_DIRTY)) || !(block->status & R600_BLOCK_STATUS_ENABLED)) {
		block->status |= R600_BLOCK_STATUS_DIRTY;
		ctx->pm4_dirty_cdwords += block->pm4_ndwords;
		if (!(block->status & R600_BLOCK_STATUS_ENABLED)) {
			block->status |= R600_BLOCK_STATUS_ENABLED;
			LIST_ADDTAIL(&block->enable_list, &ctx->enable_list);
		}
		LIST_ADDTAIL(&block->list,&ctx->dirty);

		if (block->flags & REG_FLAG_FLUSH_CHANGE) {
			ctx->flags |= R600_CONTEXT_WAIT_3D_IDLE;
		}
	}
}

/**
 * If reg needs a reloc, this function will add it to its block's reloc list.
 * @return true if reg needs a reloc, false otherwise
 */
static bool r600_reg_set_block_reloc(struct r600_pipe_reg *reg)
{
	unsigned reloc_id;

	if (!reg->block->pm4_bo_index[reg->id]) {
		return false;
	}
	/* find relocation */
	reloc_id = reg->block->pm4_bo_index[reg->id];
	pipe_resource_reference(
		(struct pipe_resource**)&reg->block->reloc[reloc_id].bo,
		&reg->bo->b.b);
	reg->block->reloc[reloc_id].bo_usage = reg->bo_usage;
	return true;
}

/**
 * This function will emit all the registers in state directly to the command
 * stream allowing you to bypass the r600_context dirty list.
 *
 * This is used for dispatching compute shaders to avoid mixing compute and
 * 3D states in the context's dirty list.
 *
 * @param pkt_flags Should be either 0 or RADEON_CP_PACKET3_COMPUTE_MODE.  This
 * value will be passed on to r600_context_block_emit_dirty an or'd against
 * the PKT3 headers.
 */
void r600_context_pipe_state_emit(struct r600_context *ctx,
                          struct r600_pipe_state *state,
                          unsigned pkt_flags)
{
	unsigned i;

	/* Mark all blocks as dirty: 
	 * Since two registers can be in the same block, we need to make sure
	 * we mark all the blocks dirty before we emit any of them.  If we were
	 * to mark blocks dirty and emit them in the same loop, like this:
	 *
	 * foreach (reg in state->regs) {
	 *     mark_dirty(reg->block)
	 *     emit_block(reg->block)
	 * }
	 *
	 * Then if we have two registers in this state that are in the same
	 * block, we would end up emitting that block twice.
	 */
	for (i = 0; i < state->nregs; i++) {
		struct r600_pipe_reg *reg = &state->regs[i];
		/* Mark all the registers in the block as dirty */
		reg->block->nreg_dirty = reg->block->nreg;
		reg->block->status |= R600_BLOCK_STATUS_DIRTY;
		/* Update the reloc for this register if necessary. */
		r600_reg_set_block_reloc(reg);
	}

	/* Emit the registers writes */
	for (i = 0; i < state->nregs; i++) {
		struct r600_pipe_reg *reg = &state->regs[i];
		if (reg->block->status & R600_BLOCK_STATUS_DIRTY) {
			r600_context_block_emit_dirty(ctx, reg->block, pkt_flags);
		}
	}
}

void r600_context_pipe_state_set(struct r600_context *ctx, struct r600_pipe_state *state)
{
	struct r600_block *block;
	int dirty;
	for (int i = 0; i < state->nregs; i++) {
		unsigned id;
		struct r600_pipe_reg *reg = &state->regs[i];

		block = reg->block;
		id = reg->id;

		dirty = block->status & R600_BLOCK_STATUS_DIRTY;

		if (reg->value != block->reg[id]) {
			block->reg[id] = reg->value;
			dirty |= R600_BLOCK_STATUS_DIRTY;
		}
		if (block->flags & REG_FLAG_DIRTY_ALWAYS)
			dirty |= R600_BLOCK_STATUS_DIRTY;
		if (r600_reg_set_block_reloc(reg)) {
			/* always force dirty for relocs for now */
			dirty |= R600_BLOCK_STATUS_DIRTY;
		}

		if (dirty)
			r600_context_dirty_block(ctx, block, dirty, id);
	}
}

/**
 * @param pkt_flags should be set to RADEON_CP_PACKET3_COMPUTE_MODE if this
 * block will be used for compute shaders.
 */
void r600_context_block_emit_dirty(struct r600_context *ctx, struct r600_block *block,
	unsigned pkt_flags)
{
	struct radeon_winsys_cs *cs = ctx->rings.gfx.cs;
	int optional = block->nbo == 0 && !(block->flags & REG_FLAG_DIRTY_ALWAYS);
	int cp_dwords = block->pm4_ndwords, start_dword = 0;
	int new_dwords = 0;
	int nbo = block->nbo;

	if (block->nreg_dirty == 0 && optional) {
		goto out;
	}

	if (nbo) {
		for (int j = 0; j < block->nreg; j++) {
			if (block->pm4_bo_index[j]) {
				/* find relocation */
				struct r600_block_reloc *reloc = &block->reloc[block->pm4_bo_index[j]];
				if (reloc->bo) {
					block->pm4[reloc->bo_pm4_index] =
							r600_context_bo_reloc(ctx, &ctx->rings.gfx, reloc->bo, reloc->bo_usage);
				} else {
					block->pm4[reloc->bo_pm4_index] = 0;
				}
				nbo--;
				if (nbo == 0)
					break;

			}
		}
	}

	optional &= (block->nreg_dirty != block->nreg);
	if (optional) {
		new_dwords = block->nreg_dirty;
		start_dword = cs->cdw;
		cp_dwords = new_dwords + 2;
	}
	memcpy(&cs->buf[cs->cdw], block->pm4, cp_dwords * 4);

	/* We are applying the pkt_flags after copying the register block to
	 * the the command stream, because it is possible this block will be
	 * emitted with a different pkt_flags, and we don't want to store the
	 * pkt_flags in the block.
	 */
	cs->buf[cs->cdw] |= pkt_flags;
	cs->cdw += cp_dwords;

	if (optional) {
		uint32_t newword;

		newword = cs->buf[start_dword];
		newword &= PKT_COUNT_C;
		newword |= PKT_COUNT_S(new_dwords);
		cs->buf[start_dword] = newword;
	}
out:
	block->status ^= R600_BLOCK_STATUS_DIRTY;
	block->nreg_dirty = 0;
	LIST_DELINIT(&block->list);
}

void r600_flush_emit(struct r600_context *rctx)
{
	struct radeon_winsys_cs *cs = rctx->rings.gfx.cs;
	unsigned cp_coher_cntl = 0;
	unsigned wait_until = 0;
	unsigned emit_flush = 0;

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
			cs->buf[cs->cdw++] = PKT3(PKT3_EVENT_WRITE, 0, 0);
			cs->buf[cs->cdw++] = EVENT_TYPE(EVENT_TYPE_PS_PARTIAL_FLUSH) | EVENT_INDEX(4);
		}
	}

	if (rctx->chip_class >= R700 &&
	    (rctx->flags & R600_CONTEXT_FLUSH_AND_INV_CB_META)) {
		cs->buf[cs->cdw++] = PKT3(PKT3_EVENT_WRITE, 0, 0);
		cs->buf[cs->cdw++] = EVENT_TYPE(EVENT_TYPE_FLUSH_AND_INV_CB_META) | EVENT_INDEX(0);
	}

	if (rctx->flags & R600_CONTEXT_FLUSH_AND_INV) {
		cs->buf[cs->cdw++] = PKT3(PKT3_EVENT_WRITE, 0, 0);
		cs->buf[cs->cdw++] = EVENT_TYPE(EVENT_TYPE_CACHE_FLUSH_AND_INV_EVENT) | EVENT_INDEX(0);
		if (rctx->chip_class >= EVERGREEN) {
			cp_coher_cntl = S_0085F0_CB0_DEST_BASE_ENA(1) |
					S_0085F0_CB1_DEST_BASE_ENA(1) |
					S_0085F0_CB2_DEST_BASE_ENA(1) |
					S_0085F0_CB3_DEST_BASE_ENA(1) |
					S_0085F0_CB4_DEST_BASE_ENA(1) |
					S_0085F0_CB5_DEST_BASE_ENA(1) |
					S_0085F0_CB6_DEST_BASE_ENA(1) |
					S_0085F0_CB7_DEST_BASE_ENA(1) |
					S_0085F0_CB8_DEST_BASE_ENA(1) |
					S_0085F0_CB9_DEST_BASE_ENA(1) |
					S_0085F0_CB10_DEST_BASE_ENA(1) |
					S_0085F0_CB11_DEST_BASE_ENA(1) |
					S_0085F0_DB_DEST_BASE_ENA(1) |
					S_0085F0_TC_ACTION_ENA(1) |
					S_0085F0_CB_ACTION_ENA(1) |
					S_0085F0_DB_ACTION_ENA(1) |
					S_0085F0_SH_ACTION_ENA(1) |
					S_0085F0_SMX_ACTION_ENA(1) |
					S_0085F0_FULL_CACHE_ENA(1);
		} else {
			cp_coher_cntl = S_0085F0_SMX_ACTION_ENA(1) |
					S_0085F0_SH_ACTION_ENA(1) |
					S_0085F0_VC_ACTION_ENA(1) |
					S_0085F0_TC_ACTION_ENA(1) |
					S_0085F0_FULL_CACHE_ENA(1);
		}
	}

	if (rctx->flags & R600_CONTEXT_INVAL_READ_CACHES) {
		cp_coher_cntl |= S_0085F0_VC_ACTION_ENA(1) |
				S_0085F0_TC_ACTION_ENA(1) |
				S_0085F0_FULL_CACHE_ENA(1);
		emit_flush = 1;
	}

	if (rctx->family >= CHIP_RV770 && rctx->flags & R600_CONTEXT_STREAMOUT_FLUSH) {
		cp_coher_cntl |= S_0085F0_SO0_DEST_BASE_ENA(1) |
				S_0085F0_SO1_DEST_BASE_ENA(1) |
				S_0085F0_SO2_DEST_BASE_ENA(1) |
				S_0085F0_SO3_DEST_BASE_ENA(1) |
				S_0085F0_SMX_ACTION_ENA(1);
		emit_flush = 1;
	}

	if (emit_flush) {
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

	if (cs->cdw == ctx->start_cs_cmd.num_dw)
		return;

	ctx->nontimer_queries_suspended = false;
	ctx->streamout_suspended = false;

	/* suspend queries */
	if (ctx->num_cs_dw_nontimer_queries_suspend) {
		r600_suspend_nontimer_queries(ctx);
		ctx->nontimer_queries_suspended = true;
	}

	if (ctx->num_cs_dw_streamout_end) {
		r600_context_streamout_end(ctx);
		ctx->streamout_suspended = true;
	}

	/* flush is needed to avoid lockups on some chips with user fences
	 * this will also flush the framebuffer cache
	 */
	ctx->flags |= R600_CONTEXT_FLUSH_AND_INV |
		      R600_CONTEXT_FLUSH_AND_INV_CB_META |
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
#if R600_TRACE_CS
	if (ctx->screen->trace_bo) {
		struct r600_screen *rscreen = ctx->screen;
		unsigned i;

		for (i = 0; i < cs->cdw; i++) {
			fprintf(stderr, "[%4d] [%5d] 0x%08x\n", rscreen->cs_count, i, cs->buf[i]);
		}
		rscreen->cs_count++;
	}
#endif
	ctx->ws->cs_flush(ctx->rings.gfx.cs, flags);
#if R600_TRACE_CS
	if (ctx->screen->trace_bo) {
		struct r600_screen *rscreen = ctx->screen;
		unsigned i;

		for (i = 0; i < 10; i++) {
			usleep(5);
			if (!ctx->ws->buffer_is_busy(rscreen->trace_bo->buf, RADEON_USAGE_READWRITE)) {
				break;
			}
		}
		if (i == 10) {
			fprintf(stderr, "timeout on cs lockup likely happen at cs %d dw %d\n",
				rscreen->trace_ptr[1], rscreen->trace_ptr[0]);
		} else {
			fprintf(stderr, "cs %d executed in %dms\n", rscreen->trace_ptr[1], i * 5);
		}
	}
#endif
}

void r600_begin_new_cs(struct r600_context *ctx)
{
	struct r600_block *enable_block = NULL;
	unsigned shader;

	ctx->pm4_dirty_cdwords = 0;
	ctx->flags = 0;

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
	ctx->poly_offset_state.atom.dirty = true;
	ctx->vgt_state.atom.dirty = true;
	ctx->vgt2_state.atom.dirty = true;
	ctx->sample_mask.atom.dirty = true;
	ctx->scissor.atom.dirty = true;
	ctx->config_state.atom.dirty = true;
	ctx->stencil_ref.atom.dirty = true;
	ctx->vertex_fetch_shader.atom.dirty = true;
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

	if (ctx->streamout_suspended) {
		ctx->streamout_start = TRUE;
		ctx->streamout_append_bitmask = ~0;
	}

	/* resume queries */
	if (ctx->nontimer_queries_suspended) {
		r600_resume_nontimer_queries(ctx);
	}

	/* set all valid group as dirty so they get reemited on
	 * next draw command
	 */
	LIST_FOR_EACH_ENTRY(enable_block, &ctx->enable_list, enable_list) {
		if(!(enable_block->status & R600_BLOCK_STATUS_DIRTY)) {
			LIST_ADDTAIL(&enable_block->list,&ctx->dirty);
			enable_block->status |= R600_BLOCK_STATUS_DIRTY;
		}
		ctx->pm4_dirty_cdwords += enable_block->pm4_ndwords;
		enable_block->nreg_dirty = enable_block->nreg;
	}

	/* Re-emit the draw state. */
	ctx->last_primitive_type = -1;
	ctx->last_start_instance = -1;
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

void r600_context_streamout_begin(struct r600_context *ctx)
{
	struct radeon_winsys_cs *cs = ctx->rings.gfx.cs;
	struct r600_so_target **t = ctx->so_targets;
	unsigned *stride_in_dw = ctx->vs_shader->so.stride;
	unsigned buffer_en, i, update_flags = 0;
	uint64_t va;
	unsigned num_cs_dw_streamout_end;

	buffer_en = (ctx->num_so_targets >= 1 && t[0] ? 1 : 0) |
		    (ctx->num_so_targets >= 2 && t[1] ? 2 : 0) |
		    (ctx->num_so_targets >= 3 && t[2] ? 4 : 0) |
		    (ctx->num_so_targets >= 4 && t[3] ? 8 : 0);

	num_cs_dw_streamout_end =
		12 + /* flush_vgt_streamout */
		util_bitcount(buffer_en) * 8 + /* STRMOUT_BUFFER_UPDATE */
		3 /* set_streamout_enable(0) */;

	r600_need_cs_space(ctx,
			   12 + /* flush_vgt_streamout */
			   6 + /* set_streamout_enable */
			   util_bitcount(buffer_en) * 7 + /* SET_CONTEXT_REG */
			   (ctx->family >= CHIP_RS780 &&
			    ctx->family <= CHIP_RV740 ? util_bitcount(buffer_en) * 5 : 0) + /* STRMOUT_BASE_UPDATE */
			   util_bitcount(buffer_en & ctx->streamout_append_bitmask) * 8 + /* STRMOUT_BUFFER_UPDATE */
			   util_bitcount(buffer_en & ~ctx->streamout_append_bitmask) * 6 + /* STRMOUT_BUFFER_UPDATE */
			   (ctx->family > CHIP_R600 && ctx->family < CHIP_RS780 ? 2 : 0) + /* SURFACE_BASE_UPDATE */
			   num_cs_dw_streamout_end, TRUE);

	/* This must be set after r600_need_cs_space. */
	ctx->num_cs_dw_streamout_end = num_cs_dw_streamout_end;

	if (ctx->chip_class >= EVERGREEN) {
		evergreen_flush_vgt_streamout(ctx);
		evergreen_set_streamout_enable(ctx, buffer_en);
	} else {
		r600_flush_vgt_streamout(ctx);
		r600_set_streamout_enable(ctx, buffer_en);
	}

	for (i = 0; i < ctx->num_so_targets; i++) {
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

			if (ctx->streamout_append_bitmask & (1 << i)) {
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

	if (ctx->family > CHIP_R600 && ctx->family < CHIP_RS780) {
		cs->buf[cs->cdw++] = PKT3(PKT3_SURFACE_BASE_UPDATE, 0, 0);
		cs->buf[cs->cdw++] = update_flags;
	}
}

void r600_context_streamout_end(struct r600_context *ctx)
{
	struct radeon_winsys_cs *cs = ctx->rings.gfx.cs;
	struct r600_so_target **t = ctx->so_targets;
	unsigned i;
	uint64_t va;

	if (ctx->chip_class >= EVERGREEN) {
		evergreen_flush_vgt_streamout(ctx);
	} else {
		r600_flush_vgt_streamout(ctx);
	}

	for (i = 0; i < ctx->num_so_targets; i++) {
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
	ctx->num_cs_dw_streamout_end = 0;
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
	assert(rctx->chip_class != R600);

	/* CP DMA doesn't work on R600 (flushing seems to be unreliable). */
	if (rctx->chip_class == R600) {
		return;
	}

	dst_offset += r600_resource_va(&rctx->screen->screen, dst);
	src_offset += r600_resource_va(&rctx->screen->screen, src);

	/* We flush the caches, because we might read from or write
	 * to resources which are bound right now. */
	rctx->flags |= R600_CONTEXT_INVAL_READ_CACHES |
		       R600_CONTEXT_FLUSH_AND_INV |
		       R600_CONTEXT_FLUSH_AND_INV_CB_META |
		       R600_CONTEXT_STREAMOUT_FLUSH |
		       R600_CONTEXT_WAIT_3D_IDLE;

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
}

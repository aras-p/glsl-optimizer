/*
 * Copyright 2013 Advanced Micro Devices, Inc.
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
 * Authors: Marek Olšák <maraeo@gmail.com>
 *
 */

#include "r600_pipe_common.h"
#include "r600_cs.h"

#include "util/u_memory.h"

static struct pipe_stream_output_target *
r600_create_so_target(struct pipe_context *ctx,
		      struct pipe_resource *buffer,
		      unsigned buffer_offset,
		      unsigned buffer_size)
{
	struct r600_common_context *rctx = (struct r600_common_context *)ctx;
	struct r600_so_target *t;
	struct r600_resource *rbuffer = (struct r600_resource*)buffer;

	t = CALLOC_STRUCT(r600_so_target);
	if (!t) {
		return NULL;
	}

	u_suballocator_alloc(rctx->allocator_so_filled_size, 4,
			     &t->buf_filled_size_offset,
			     (struct pipe_resource**)&t->buf_filled_size);
	if (!t->buf_filled_size) {
		FREE(t);
		return NULL;
	}

	t->b.reference.count = 1;
	t->b.context = ctx;
	pipe_resource_reference(&t->b.buffer, buffer);
	t->b.buffer_offset = buffer_offset;
	t->b.buffer_size = buffer_size;

	util_range_add(&rbuffer->valid_buffer_range, buffer_offset,
		       buffer_offset + buffer_size);
	return &t->b;
}

static void r600_so_target_destroy(struct pipe_context *ctx,
				   struct pipe_stream_output_target *target)
{
	struct r600_so_target *t = (struct r600_so_target*)target;
	pipe_resource_reference(&t->b.buffer, NULL);
	pipe_resource_reference((struct pipe_resource**)&t->buf_filled_size, NULL);
	FREE(t);
}

void r600_streamout_buffers_dirty(struct r600_common_context *rctx)
{
	struct r600_atom *begin = &rctx->streamout.begin_atom;
	unsigned num_bufs = util_bitcount(rctx->streamout.enabled_mask);
	unsigned num_bufs_appended = util_bitcount(rctx->streamout.enabled_mask &
						   rctx->streamout.append_bitmask);

	rctx->streamout.num_dw_for_end =
		12 + /* flush_vgt_streamout */
		num_bufs * 8 + /* STRMOUT_BUFFER_UPDATE */
		3 /* set_streamout_enable(0) */;

	begin->num_dw = 12 + /* flush_vgt_streamout */
			6; /* set_streamout_enable */

	if (rctx->chip_class >= SI) {
		begin->num_dw += num_bufs * 4; /* SET_CONTEXT_REG */
	} else {
		begin->num_dw += num_bufs * 7; /* SET_CONTEXT_REG */

		if (rctx->family >= CHIP_RS780 && rctx->family <= CHIP_RV740)
			begin->num_dw += num_bufs * 5; /* STRMOUT_BASE_UPDATE */
	}

	begin->num_dw +=
		num_bufs_appended * 8 + /* STRMOUT_BUFFER_UPDATE */
		(num_bufs - num_bufs_appended) * 6 + /* STRMOUT_BUFFER_UPDATE */
		(rctx->family > CHIP_R600 && rctx->family < CHIP_RS780 ? 2 : 0) + /* SURFACE_BASE_UPDATE */
		rctx->streamout.num_dw_for_end;

	begin->dirty = true;
}

void r600_set_streamout_targets(struct pipe_context *ctx,
				unsigned num_targets,
				struct pipe_stream_output_target **targets,
				unsigned append_bitmask)
{
	struct r600_common_context *rctx = (struct r600_common_context *)ctx;
	unsigned i;

	/* Stop streamout. */
	if (rctx->streamout.num_targets && rctx->streamout.begin_emitted) {
		r600_emit_streamout_end(rctx);
	}

	/* Set the new targets. */
	for (i = 0; i < num_targets; i++) {
		pipe_so_target_reference((struct pipe_stream_output_target**)&rctx->streamout.targets[i], targets[i]);
		r600_context_add_resource_size(ctx, targets[i]->buffer);
	}
	for (; i < rctx->streamout.num_targets; i++) {
		pipe_so_target_reference((struct pipe_stream_output_target**)&rctx->streamout.targets[i], NULL);
	}

	rctx->streamout.enabled_mask = (num_targets >= 1 && targets[0] ? 1 : 0) |
				       (num_targets >= 2 && targets[1] ? 2 : 0) |
				       (num_targets >= 3 && targets[2] ? 4 : 0) |
				       (num_targets >= 4 && targets[3] ? 8 : 0);

	rctx->streamout.num_targets = num_targets;
	rctx->streamout.append_bitmask = append_bitmask;

	if (num_targets) {
		r600_streamout_buffers_dirty(rctx);
	} else {
		rctx->streamout.begin_atom.dirty = false;
	}
}

static void r600_flush_vgt_streamout(struct r600_common_context *rctx)
{
	struct radeon_winsys_cs *cs = rctx->rings.gfx.cs;
	unsigned reg_strmout_cntl;

	/* The register is at different places on different ASICs. */
	if (rctx->chip_class >= CIK) {
		reg_strmout_cntl = R_0300FC_CP_STRMOUT_CNTL;
	} else if (rctx->chip_class >= EVERGREEN) {
		reg_strmout_cntl = R_0084FC_CP_STRMOUT_CNTL;
	} else {
		reg_strmout_cntl = R_008490_CP_STRMOUT_CNTL;
	}

	if (rctx->chip_class >= CIK) {
		cik_write_uconfig_reg(cs, reg_strmout_cntl, 0);
	} else {
		r600_write_config_reg(cs, reg_strmout_cntl, 0);
	}

	radeon_emit(cs, PKT3(PKT3_EVENT_WRITE, 0, 0));
	radeon_emit(cs, EVENT_TYPE(EVENT_TYPE_SO_VGTSTREAMOUT_FLUSH) | EVENT_INDEX(0));

	radeon_emit(cs, PKT3(PKT3_WAIT_REG_MEM, 5, 0));
	radeon_emit(cs, WAIT_REG_MEM_EQUAL); /* wait until the register is equal to the reference value */
	radeon_emit(cs, reg_strmout_cntl >> 2);  /* register */
	radeon_emit(cs, 0);
	radeon_emit(cs, S_008490_OFFSET_UPDATE_DONE(1)); /* reference value */
	radeon_emit(cs, S_008490_OFFSET_UPDATE_DONE(1)); /* mask */
	radeon_emit(cs, 4); /* poll interval */
}

static void r600_set_streamout_enable(struct r600_common_context *rctx, unsigned buffer_enable_bit)
{
	struct radeon_winsys_cs *cs = rctx->rings.gfx.cs;

	if (buffer_enable_bit) {
		r600_write_context_reg(cs, R_028AB0_VGT_STRMOUT_EN, S_028AB0_STREAMOUT(1));
		r600_write_context_reg(cs, R_028B20_VGT_STRMOUT_BUFFER_EN, buffer_enable_bit);
	} else {
		r600_write_context_reg(cs, R_028AB0_VGT_STRMOUT_EN, S_028AB0_STREAMOUT(0));
	}
}

static void evergreen_set_streamout_enable(struct r600_common_context *rctx, unsigned buffer_enable_bit)
{
	struct radeon_winsys_cs *cs = rctx->rings.gfx.cs;

	if (buffer_enable_bit) {
		r600_write_context_reg_seq(cs, R_028B94_VGT_STRMOUT_CONFIG, 2);
		radeon_emit(cs, S_028B94_STREAMOUT_0_EN(1)); /* R_028B94_VGT_STRMOUT_CONFIG */
		radeon_emit(cs, S_028B98_STREAM_0_BUFFER_EN(buffer_enable_bit)); /* R_028B98_VGT_STRMOUT_BUFFER_CONFIG */
	} else {
		r600_write_context_reg(cs, R_028B94_VGT_STRMOUT_CONFIG, S_028B94_STREAMOUT_0_EN(0));
	}
}

static void r600_emit_reloc(struct r600_common_context *rctx,
			    struct r600_ring *ring, struct r600_resource *rbo,
			    enum radeon_bo_usage usage)
{
	struct radeon_winsys_cs *cs = ring->cs;
	bool has_vm = ((struct r600_common_screen*)rctx->b.screen)->info.r600_virtual_address;
	unsigned reloc = r600_context_bo_reloc(rctx, ring, rbo, usage);

	if (!has_vm) {
		radeon_emit(cs, PKT3(PKT3_NOP, 0, 0));
		radeon_emit(cs, reloc);
	}
}

static void r600_emit_streamout_begin(struct r600_common_context *rctx, struct r600_atom *atom)
{
	struct radeon_winsys_cs *cs = rctx->rings.gfx.cs;
	struct r600_so_target **t = rctx->streamout.targets;
	unsigned *stride_in_dw = rctx->streamout.stride_in_dw;
	unsigned i, update_flags = 0;

	r600_flush_vgt_streamout(rctx);

	if (rctx->chip_class >= EVERGREEN) {
		evergreen_set_streamout_enable(rctx, rctx->streamout.enabled_mask);
	} else {
		r600_set_streamout_enable(rctx, rctx->streamout.enabled_mask);
	}

	for (i = 0; i < rctx->streamout.num_targets; i++) {
		if (!t[i])
			continue;

		t[i]->stride_in_dw = stride_in_dw[i];

		if (rctx->chip_class >= SI) {
			/* SI binds streamout buffers as shader resources.
			 * VGT only counts primitives and tells the shader
			 * through SGPRs what to do. */
			r600_write_context_reg_seq(cs, R_028AD0_VGT_STRMOUT_BUFFER_SIZE_0 + 16*i, 2);
			radeon_emit(cs, (t[i]->b.buffer_offset +
					 t[i]->b.buffer_size) >> 2);	/* BUFFER_SIZE (in DW) */
			radeon_emit(cs, stride_in_dw[i]);		/* VTX_STRIDE (in DW) */
		} else {
			uint64_t va = r600_resource_va(rctx->b.screen,
						       (void*)t[i]->b.buffer);

			update_flags |= SURFACE_BASE_UPDATE_STRMOUT(i);

			r600_write_context_reg_seq(cs, R_028AD0_VGT_STRMOUT_BUFFER_SIZE_0 + 16*i, 3);
			radeon_emit(cs, (t[i]->b.buffer_offset +
					 t[i]->b.buffer_size) >> 2);	/* BUFFER_SIZE (in DW) */
			radeon_emit(cs, stride_in_dw[i]);		/* VTX_STRIDE (in DW) */
			radeon_emit(cs, va >> 8);			/* BUFFER_BASE */

			r600_emit_reloc(rctx, &rctx->rings.gfx, r600_resource(t[i]->b.buffer),
					RADEON_USAGE_WRITE);

			/* R7xx requires this packet after updating BUFFER_BASE.
			 * Without this, R7xx locks up. */
			if (rctx->family >= CHIP_RS780 && rctx->family <= CHIP_RV740) {
				radeon_emit(cs, PKT3(PKT3_STRMOUT_BASE_UPDATE, 1, 0));
				radeon_emit(cs, i);
				radeon_emit(cs, va >> 8);

				r600_emit_reloc(rctx, &rctx->rings.gfx, r600_resource(t[i]->b.buffer),
						RADEON_USAGE_WRITE);
			}
		}

		if (rctx->streamout.append_bitmask & (1 << i)) {
			uint64_t va = r600_resource_va(rctx->b.screen,
						       (void*)t[i]->buf_filled_size) +
				      t[i]->buf_filled_size_offset;

			/* Append. */
			radeon_emit(cs, PKT3(PKT3_STRMOUT_BUFFER_UPDATE, 4, 0));
			radeon_emit(cs, STRMOUT_SELECT_BUFFER(i) |
				    STRMOUT_OFFSET_SOURCE(STRMOUT_OFFSET_FROM_MEM)); /* control */
			radeon_emit(cs, 0); /* unused */
			radeon_emit(cs, 0); /* unused */
			radeon_emit(cs, va); /* src address lo */
			radeon_emit(cs, va >> 32); /* src address hi */

			r600_emit_reloc(rctx,  &rctx->rings.gfx, t[i]->buf_filled_size,
					RADEON_USAGE_READ);
		} else {
			/* Start from the beginning. */
			radeon_emit(cs, PKT3(PKT3_STRMOUT_BUFFER_UPDATE, 4, 0));
			radeon_emit(cs, STRMOUT_SELECT_BUFFER(i) |
				    STRMOUT_OFFSET_SOURCE(STRMOUT_OFFSET_FROM_PACKET)); /* control */
			radeon_emit(cs, 0); /* unused */
			radeon_emit(cs, 0); /* unused */
			radeon_emit(cs, t[i]->b.buffer_offset >> 2); /* buffer offset in DW */
			radeon_emit(cs, 0); /* unused */
		}
	}

	if (rctx->family > CHIP_R600 && rctx->family < CHIP_RV770) {
		radeon_emit(cs, PKT3(PKT3_SURFACE_BASE_UPDATE, 0, 0));
		radeon_emit(cs, update_flags);
	}
	rctx->streamout.begin_emitted = true;
}

void r600_emit_streamout_end(struct r600_common_context *rctx)
{
	struct radeon_winsys_cs *cs = rctx->rings.gfx.cs;
	struct r600_so_target **t = rctx->streamout.targets;
	unsigned i;
	uint64_t va;

	r600_flush_vgt_streamout(rctx);

	for (i = 0; i < rctx->streamout.num_targets; i++) {
		if (!t[i])
			continue;

		va = r600_resource_va(rctx->b.screen,
				      (void*)t[i]->buf_filled_size) + t[i]->buf_filled_size_offset;
		radeon_emit(cs, PKT3(PKT3_STRMOUT_BUFFER_UPDATE, 4, 0));
		radeon_emit(cs, STRMOUT_SELECT_BUFFER(i) |
			    STRMOUT_OFFSET_SOURCE(STRMOUT_OFFSET_NONE) |
			    STRMOUT_STORE_BUFFER_FILLED_SIZE); /* control */
		radeon_emit(cs, va);     /* dst address lo */
		radeon_emit(cs, va >> 32); /* dst address hi */
		radeon_emit(cs, 0); /* unused */
		radeon_emit(cs, 0); /* unused */

		r600_emit_reloc(rctx,  &rctx->rings.gfx, t[i]->buf_filled_size,
				RADEON_USAGE_WRITE);
	}

	if (rctx->chip_class >= EVERGREEN) {
		evergreen_set_streamout_enable(rctx, 0);
	} else {
		r600_set_streamout_enable(rctx, 0);
	}

	rctx->streamout.begin_emitted = false;

	if (rctx->chip_class >= R700) {
		rctx->flags |= R600_CONTEXT_STREAMOUT_FLUSH;
	} else {
		rctx->flags |= R600_CONTEXT_FLUSH_AND_INV;
	}
}

void r600_streamout_init(struct r600_common_context *rctx)
{
	rctx->b.create_stream_output_target = r600_create_so_target;
	rctx->b.stream_output_target_destroy = r600_so_target_destroy;
	rctx->streamout.begin_atom.emit = r600_emit_streamout_begin;
}

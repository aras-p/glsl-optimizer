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

static void r600_set_streamout_enable(struct r600_common_context *rctx, bool enable);

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

	if (!num_bufs)
		return;

	rctx->streamout.num_dw_for_end =
		12 + /* flush_vgt_streamout */
		num_bufs * 11; /* STRMOUT_BUFFER_UPDATE, BUFFER_SIZE */

	begin->num_dw = 12 + /* flush_vgt_streamout */
			3; /* VGT_STRMOUT_BUFFER_CONFIG */

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
		(rctx->family > CHIP_R600 && rctx->family < CHIP_RS780 ? 2 : 0); /* SURFACE_BASE_UPDATE */

	begin->dirty = true;

	r600_set_streamout_enable(rctx, true);
}

void r600_set_streamout_targets(struct pipe_context *ctx,
				unsigned num_targets,
				struct pipe_stream_output_target **targets,
				const unsigned *offsets)
{
	struct r600_common_context *rctx = (struct r600_common_context *)ctx;
	unsigned i;
        unsigned append_bitmask = 0;

	/* Stop streamout. */
	if (rctx->streamout.num_targets && rctx->streamout.begin_emitted) {
		r600_emit_streamout_end(rctx);
	}

	/* Set the new targets. */
	for (i = 0; i < num_targets; i++) {
		pipe_so_target_reference((struct pipe_stream_output_target**)&rctx->streamout.targets[i], targets[i]);
		r600_context_add_resource_size(ctx, targets[i]->buffer);
		if (offsets[i] == ((unsigned)-1))
			append_bitmask |=  1 << i;
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
		r600_set_streamout_enable(rctx, false);
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

static void r600_emit_streamout_begin(struct r600_common_context *rctx, struct r600_atom *atom)
{
	struct radeon_winsys_cs *cs = rctx->rings.gfx.cs;
	struct r600_so_target **t = rctx->streamout.targets;
	unsigned *stride_in_dw = rctx->streamout.stride_in_dw;
	unsigned i, update_flags = 0;

	r600_flush_vgt_streamout(rctx);

	r600_write_context_reg(cs, rctx->chip_class >= EVERGREEN ?
				       R_028B98_VGT_STRMOUT_BUFFER_CONFIG :
				       R_028B20_VGT_STRMOUT_BUFFER_EN,
			       rctx->streamout.enabled_mask);

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
					RADEON_USAGE_WRITE, RADEON_PRIO_SHADER_RESOURCE_RW);

			/* R7xx requires this packet after updating BUFFER_BASE.
			 * Without this, R7xx locks up. */
			if (rctx->family >= CHIP_RS780 && rctx->family <= CHIP_RV740) {
				radeon_emit(cs, PKT3(PKT3_STRMOUT_BASE_UPDATE, 1, 0));
				radeon_emit(cs, i);
				radeon_emit(cs, va >> 8);

				r600_emit_reloc(rctx, &rctx->rings.gfx, r600_resource(t[i]->b.buffer),
						RADEON_USAGE_WRITE, RADEON_PRIO_SHADER_RESOURCE_RW);
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
					RADEON_USAGE_READ, RADEON_PRIO_MIN);
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
				RADEON_USAGE_WRITE, RADEON_PRIO_MIN);

		/* Zero the buffer size. The counters (primitives generated,
		 * primitives emitted) may be enabled even if there is not
		 * buffer bound. This ensures that the primitives-emitted query
		 * won't increment. */
		r600_write_context_reg(cs, R_028AD0_VGT_STRMOUT_BUFFER_SIZE_0 + 16*i, 0);
	}

	rctx->streamout.begin_emitted = false;

	if (rctx->chip_class >= R700) {
		rctx->flags |= R600_CONTEXT_STREAMOUT_FLUSH;
	} else {
		rctx->flags |= R600_CONTEXT_FLUSH_AND_INV;
	}
}

/* STREAMOUT CONFIG DERIVED STATE
 *
 * Streamout must be enabled for the PRIMITIVES_GENERATED query to work.
 * The buffer mask is an independent state, so no writes occur if there
 * are no buffers bound.
 */

static bool r600_get_strmout_en(struct r600_common_context *rctx)
{
	return rctx->streamout.streamout_enabled ||
	       rctx->streamout.prims_gen_query_enabled;
}

static void r600_emit_streamout_enable(struct r600_common_context *rctx,
				       struct r600_atom *atom)
{
	r600_write_context_reg(rctx->rings.gfx.cs,
			       rctx->chip_class >= EVERGREEN ?
				       R_028B94_VGT_STRMOUT_CONFIG :
				       R_028AB0_VGT_STRMOUT_EN,
			       S_028B94_STREAMOUT_0_EN(r600_get_strmout_en(rctx)));
}

static void r600_set_streamout_enable(struct r600_common_context *rctx, bool enable)
{
	bool old_strmout_en = r600_get_strmout_en(rctx);

	rctx->streamout.streamout_enabled = enable;
	if (old_strmout_en != r600_get_strmout_en(rctx))
		rctx->streamout.enable_atom.dirty = true;
}

void r600_update_prims_generated_query_state(struct r600_common_context *rctx,
					     unsigned type, int diff)
{
	if (type == PIPE_QUERY_PRIMITIVES_GENERATED) {
		bool old_strmout_en = r600_get_strmout_en(rctx);

		rctx->streamout.num_prims_gen_queries += diff;
		assert(rctx->streamout.num_prims_gen_queries >= 0);

		rctx->streamout.prims_gen_query_enabled =
			rctx->streamout.num_prims_gen_queries != 0;

		if (old_strmout_en != r600_get_strmout_en(rctx))
			rctx->streamout.enable_atom.dirty = true;
	}
}

void r600_streamout_init(struct r600_common_context *rctx)
{
	rctx->b.create_stream_output_target = r600_create_so_target;
	rctx->b.stream_output_target_destroy = r600_so_target_destroy;
	rctx->streamout.begin_atom.emit = r600_emit_streamout_begin;
	rctx->streamout.enable_atom.emit = r600_emit_streamout_enable;
	rctx->streamout.enable_atom.num_dw = 3;
}

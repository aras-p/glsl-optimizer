/*
 * Copyright 2013 Advanced Micro Devices, Inc.
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
 *      Marek Olšák <marek.olsak@amd.com>
 */
#include "../radeon/r600_cs.h"
#include "radeonsi_pipe.h"
#include "radeonsi_resource.h"
#include "radeonsi_shader.h"

#include "util/u_memory.h"

#define SI_NUM_CONTEXTS 16

static uint32_t null_desc[8]; /* zeros */

/* Set this if you want the 3D engine to wait until CP DMA is done.
 * It should be set on the last CP DMA packet. */
#define R600_CP_DMA_SYNC	(1 << 0) /* R600+ */

/* Set this if the source data was used as a destination in a previous CP DMA
 * packet. It's for preventing a read-after-write (RAW) hazard between two
 * CP DMA packets. */
#define SI_CP_DMA_RAW_WAIT	(1 << 1) /* SI+ */

/* Emit a CP DMA packet to do a copy from one buffer to another.
 * The size must fit in bits [20:0].
 */
static void si_emit_cp_dma_copy_buffer(struct r600_context *rctx,
				       uint64_t dst_va, uint64_t src_va,
				       unsigned size, unsigned flags)
{
	struct radeon_winsys_cs *cs = rctx->b.rings.gfx.cs;
	uint32_t sync_flag = flags & R600_CP_DMA_SYNC ? PKT3_CP_DMA_CP_SYNC : 0;
	uint32_t raw_wait = flags & SI_CP_DMA_RAW_WAIT ? PKT3_CP_DMA_CMD_RAW_WAIT : 0;

	assert(size);
	assert((size & ((1<<21)-1)) == size);

	if (rctx->b.chip_class >= CIK) {
		radeon_emit(cs, PKT3(PKT3_DMA_DATA, 5, 0));
		radeon_emit(cs, sync_flag);		/* CP_SYNC [31] */
		radeon_emit(cs, src_va);		/* SRC_ADDR_LO [31:0] */
		radeon_emit(cs, src_va >> 32);		/* SRC_ADDR_HI [31:0] */
		radeon_emit(cs, dst_va);		/* DST_ADDR_LO [31:0] */
		radeon_emit(cs, dst_va >> 32);		/* DST_ADDR_HI [31:0] */
		radeon_emit(cs, size | raw_wait);	/* COMMAND [29:22] | BYTE_COUNT [20:0] */
	} else {
		radeon_emit(cs, PKT3(PKT3_CP_DMA, 4, 0));
		radeon_emit(cs, src_va);			/* SRC_ADDR_LO [31:0] */
		radeon_emit(cs, sync_flag | ((src_va >> 32) & 0xffff)); /* CP_SYNC [31] | SRC_ADDR_HI [15:0] */
		radeon_emit(cs, dst_va);			/* DST_ADDR_LO [31:0] */
		radeon_emit(cs, (dst_va >> 32) & 0xffff);	/* DST_ADDR_HI [15:0] */
		radeon_emit(cs, size | raw_wait);		/* COMMAND [29:22] | BYTE_COUNT [20:0] */
	}
}

/* Emit a CP DMA packet to clear a buffer. The size must fit in bits [20:0]. */
static void si_emit_cp_dma_clear_buffer(struct r600_context *rctx,
					uint64_t dst_va, unsigned size,
					uint32_t clear_value, unsigned flags)
{
	struct radeon_winsys_cs *cs = rctx->b.rings.gfx.cs;
	uint32_t sync_flag = flags & R600_CP_DMA_SYNC ? PKT3_CP_DMA_CP_SYNC : 0;
	uint32_t raw_wait = flags & SI_CP_DMA_RAW_WAIT ? PKT3_CP_DMA_CMD_RAW_WAIT : 0;

	assert(size);
	assert((size & ((1<<21)-1)) == size);

	if (rctx->b.chip_class >= CIK) {
		radeon_emit(cs, PKT3(PKT3_DMA_DATA, 5, 0));
		radeon_emit(cs, sync_flag | PKT3_CP_DMA_SRC_SEL(2)); /* CP_SYNC [31] | SRC_SEL[30:29] */
		radeon_emit(cs, clear_value);		/* DATA [31:0] */
		radeon_emit(cs, 0);
		radeon_emit(cs, dst_va);		/* DST_ADDR_LO [31:0] */
		radeon_emit(cs, dst_va >> 32);		/* DST_ADDR_HI [15:0] */
		radeon_emit(cs, size | raw_wait);	/* COMMAND [29:22] | BYTE_COUNT [20:0] */
	} else {
		radeon_emit(cs, PKT3(PKT3_CP_DMA, 4, 0));
		radeon_emit(cs, clear_value);		/* DATA [31:0] */
		radeon_emit(cs, sync_flag | PKT3_CP_DMA_SRC_SEL(2)); /* CP_SYNC [31] | SRC_SEL[30:29] */
		radeon_emit(cs, dst_va);			/* DST_ADDR_LO [31:0] */
		radeon_emit(cs, (dst_va >> 32) & 0xffff);	/* DST_ADDR_HI [15:0] */
		radeon_emit(cs, size | raw_wait);		/* COMMAND [29:22] | BYTE_COUNT [20:0] */
	}
}

static void si_init_descriptors(struct r600_context *rctx,
				struct si_descriptors *desc,
				unsigned shader_userdata_reg,
				unsigned element_dw_size,
				unsigned num_elements,
				void (*emit_func)(struct r600_context *ctx, struct r600_atom *state))
{
	uint64_t va;

	assert(num_elements <= sizeof(desc->enabled_mask)*8);
	assert(num_elements <= sizeof(desc->dirty_mask)*8);

	desc->atom.emit = (void*)emit_func;
	desc->shader_userdata_reg = shader_userdata_reg;
	desc->element_dw_size = element_dw_size;
	desc->num_elements = num_elements;
	desc->context_size = num_elements * element_dw_size * 4;

	desc->buffer = (struct r600_resource*)
		pipe_buffer_create(rctx->b.b.screen, PIPE_BIND_CUSTOM,
				   PIPE_USAGE_STATIC,
				   SI_NUM_CONTEXTS * desc->context_size);

	r600_context_bo_reloc(&rctx->b, &rctx->b.rings.gfx, desc->buffer, RADEON_USAGE_READWRITE);
	va = r600_resource_va(rctx->b.b.screen, &desc->buffer->b.b);

	/* We don't check for CS space here, because this should be called
	 * only once at context initialization. */
	si_emit_cp_dma_clear_buffer(rctx, va, desc->buffer->b.b.width0, 0,
				    R600_CP_DMA_SYNC);
}

static void si_release_descriptors(struct si_descriptors *desc)
{
	pipe_resource_reference((struct pipe_resource**)&desc->buffer, NULL);
}

static void si_update_descriptors(struct r600_context *rctx,
				  struct si_descriptors *desc)
{
	if (desc->dirty_mask) {
		desc->atom.num_dw =
			7 + /* copy */
			(4 + desc->element_dw_size) * util_bitcount(desc->dirty_mask) + /* update */
			4; /* pointer update */
		desc->atom.dirty = true;
		/* The descriptors are read with the K cache. */
		rctx->b.flags |= R600_CONTEXT_INV_CONST_CACHE;
	} else {
		desc->atom.dirty = false;
	}
}

static void si_emit_shader_pointer(struct r600_context *rctx,
				   struct si_descriptors *desc)
{
	struct radeon_winsys_cs *cs = rctx->b.rings.gfx.cs;
	uint64_t va = r600_resource_va(rctx->b.b.screen, &desc->buffer->b.b) +
		      desc->current_context_id * desc->context_size;

	radeon_emit(cs, PKT3(PKT3_SET_SH_REG, 2, 0));
	radeon_emit(cs, (desc->shader_userdata_reg - SI_SH_REG_OFFSET) >> 2);
	radeon_emit(cs, va);
	radeon_emit(cs, va >> 32);
}

static void si_emit_descriptors(struct r600_context *rctx,
				struct si_descriptors *desc,
				uint32_t **descriptors)
{
	struct radeon_winsys_cs *cs = rctx->b.rings.gfx.cs;
	uint64_t va_base;
	int packet_start;
	int packet_size = 0;
	int last_index = desc->num_elements; /* point to a non-existing element */
	unsigned dirty_mask = desc->dirty_mask;
	unsigned new_context_id = (desc->current_context_id + 1) % SI_NUM_CONTEXTS;

	assert(dirty_mask);

	va_base = r600_resource_va(rctx->b.b.screen, &desc->buffer->b.b);

	/* Copy the descriptors to a new context slot. */
	/* XXX Consider using TC or L2 for this copy on CIK. */
	si_emit_cp_dma_copy_buffer(rctx,
				   va_base + new_context_id * desc->context_size,
				   va_base + desc->current_context_id * desc->context_size,
				   desc->context_size, R600_CP_DMA_SYNC);

	va_base += new_context_id * desc->context_size;

	/* Update the descriptors.
	 * Updates of consecutive descriptors are merged to one WRITE_DATA packet.
	 *
	 * XXX When unbinding lots of resources, consider clearing the memory
	 *     with CP DMA instead of emitting zeros.
	 */
	while (dirty_mask) {
		int i = u_bit_scan(&dirty_mask);

		assert(i < desc->num_elements);

		if (last_index+1 == i && packet_size) {
			/* Append new data at the end of the last packet. */
			packet_size += desc->element_dw_size;
			cs->buf[packet_start] = PKT3(PKT3_WRITE_DATA, packet_size, 0);
		} else {
			/* Start a new packet. */
			uint64_t va = va_base + i * desc->element_dw_size * 4;

			packet_start = cs->cdw;
			packet_size = 2 + desc->element_dw_size;

			radeon_emit(cs, PKT3(PKT3_WRITE_DATA, packet_size, 0));
			radeon_emit(cs, PKT3_WRITE_DATA_DST_SEL(PKT3_WRITE_DATA_DST_SEL_TC_OR_L2) |
					     PKT3_WRITE_DATA_WR_CONFIRM |
					     PKT3_WRITE_DATA_ENGINE_SEL(PKT3_WRITE_DATA_ENGINE_SEL_ME));
			radeon_emit(cs, va & 0xFFFFFFFFUL);
			radeon_emit(cs, (va >> 32UL) & 0xFFFFFFFFUL);
		}

		radeon_emit_array(cs, descriptors[i], desc->element_dw_size);

		last_index = i;
	}

	desc->dirty_mask = 0;
	desc->current_context_id = new_context_id;

	/* Now update the shader userdata pointer. */
	si_emit_shader_pointer(rctx, desc);
}

static unsigned si_get_shader_user_data_base(unsigned shader)
{
	switch (shader) {
	case PIPE_SHADER_VERTEX:
		return R_00B130_SPI_SHADER_USER_DATA_VS_0;
	case PIPE_SHADER_GEOMETRY:
		return R_00B230_SPI_SHADER_USER_DATA_GS_0;
	case PIPE_SHADER_FRAGMENT:
		return R_00B030_SPI_SHADER_USER_DATA_PS_0;
	default:
		assert(0);
		return 0;
	}
}

/* SAMPLER VIEWS */

static void si_emit_sampler_views(struct r600_context *rctx, struct r600_atom *atom)
{
	struct si_sampler_views *views = (struct si_sampler_views*)atom;

	si_emit_descriptors(rctx, &views->desc, views->desc_data);
}

static void si_init_sampler_views(struct r600_context *rctx,
				  struct si_sampler_views *views,
				  unsigned shader)
{
	si_init_descriptors(rctx, &views->desc,
			    si_get_shader_user_data_base(shader) +
			    SI_SGPR_RESOURCE * 4,
			    8, NUM_SAMPLER_VIEWS, si_emit_sampler_views);
}

static void si_release_sampler_views(struct si_sampler_views *views)
{
	int i;

	for (i = 0; i < Elements(views->views); i++) {
		pipe_sampler_view_reference(&views->views[i], NULL);
	}
	si_release_descriptors(&views->desc);
}

static void si_sampler_views_begin_new_cs(struct r600_context *rctx,
					  struct si_sampler_views *views)
{
	unsigned mask = views->desc.enabled_mask;

	/* Add relocations to the CS. */
	while (mask) {
		int i = u_bit_scan(&mask);
		struct si_pipe_sampler_view *rview =
			(struct si_pipe_sampler_view*)views->views[i];

		r600_context_bo_reloc(&rctx->b, &rctx->b.rings.gfx, rview->resource, RADEON_USAGE_READ);
	}

	r600_context_bo_reloc(&rctx->b, &rctx->b.rings.gfx, views->desc.buffer, RADEON_USAGE_READWRITE);

	si_emit_shader_pointer(rctx, &views->desc);
}

void si_set_sampler_view(struct r600_context *rctx, unsigned shader,
			 unsigned slot, struct pipe_sampler_view *view,
			 unsigned *view_desc)
{
	struct si_sampler_views *views = &rctx->samplers[shader].views;

	if (views->views[slot] == view)
		return;

	if (view) {
		struct si_pipe_sampler_view *rview =
			(struct si_pipe_sampler_view*)view;

		r600_context_bo_reloc(&rctx->b, &rctx->b.rings.gfx, rview->resource, RADEON_USAGE_READ);

		pipe_sampler_view_reference(&views->views[slot], view);
		views->desc_data[slot] = view_desc;
		views->desc.enabled_mask |= 1 << slot;
	} else {
		pipe_sampler_view_reference(&views->views[slot], NULL);
		views->desc_data[slot] = null_desc;
		views->desc.enabled_mask &= ~(1 << slot);
	}

	views->desc.dirty_mask |= 1 << slot;
	si_update_descriptors(rctx, &views->desc);
}

/* BUFFER RESOURCES */

static void si_emit_buffer_resources(struct r600_context *rctx, struct r600_atom *atom)
{
	struct si_buffer_resources *buffers = (struct si_buffer_resources*)atom;

	si_emit_descriptors(rctx, &buffers->desc, buffers->desc_data);
}

static void si_init_buffer_resources(struct r600_context *rctx,
				     struct si_buffer_resources *buffers,
				     unsigned num_buffers, unsigned shader,
				     unsigned shader_userdata_index,
				     enum radeon_bo_usage shader_usage)
{
	int i;

	buffers->num_buffers = num_buffers;
	buffers->shader_usage = shader_usage;
	buffers->buffers = CALLOC(num_buffers, sizeof(struct pipe_resource*));
	buffers->desc_storage = CALLOC(num_buffers, sizeof(uint32_t) * 4);

	/* si_emit_descriptors only accepts an array of arrays.
	 * This adds such an array. */
	buffers->desc_data = CALLOC(num_buffers, sizeof(uint32_t*));
	for (i = 0; i < num_buffers; i++) {
		buffers->desc_data[i] = &buffers->desc_storage[i*4];
	}

	si_init_descriptors(rctx, &buffers->desc,
			    si_get_shader_user_data_base(shader) +
			    shader_userdata_index*4, 4, num_buffers,
			    si_emit_buffer_resources);
}

static void si_release_buffer_resources(struct si_buffer_resources *buffers)
{
	int i;

	for (i = 0; i < Elements(buffers->buffers); i++) {
		pipe_resource_reference(&buffers->buffers[i], NULL);
	}

	FREE(buffers->buffers);
	FREE(buffers->desc_storage);
	FREE(buffers->desc_data);
	si_release_descriptors(&buffers->desc);
}

static void si_buffer_resources_begin_new_cs(struct r600_context *rctx,
					     struct si_buffer_resources *buffers)
{
	unsigned mask = buffers->desc.enabled_mask;

	/* Add relocations to the CS. */
	while (mask) {
		int i = u_bit_scan(&mask);

		r600_context_bo_reloc(&rctx->b, &rctx->b.rings.gfx,
				      (struct r600_resource*)buffers->buffers[i],
				      buffers->shader_usage);
	}

	r600_context_bo_reloc(&rctx->b, &rctx->b.rings.gfx,
			      buffers->desc.buffer, RADEON_USAGE_READWRITE);

	si_emit_shader_pointer(rctx, &buffers->desc);
}

/* CONSTANT BUFFERS */

static void si_set_constant_buffer(struct pipe_context *ctx, uint shader, uint slot,
				   struct pipe_constant_buffer *input)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct si_buffer_resources *buffers = &rctx->const_buffers[shader];

	if (shader >= SI_NUM_SHADERS)
		return;

	assert(slot < buffers->num_buffers);
	pipe_resource_reference(&buffers->buffers[slot], NULL);

	/* CIK cannot unbind a constant buffer (S_BUFFER_LOAD is buggy
	 * with a NULL buffer). We need to use a dummy buffer instead. */
	if (rctx->b.chip_class == CIK &&
	    (!input || (!input->buffer && !input->user_buffer)))
		input = &rctx->null_const_buf;

	if (input && (input->buffer || input->user_buffer)) {
		struct pipe_resource *buffer = NULL;
		uint64_t va;

		/* Upload the user buffer if needed. */
		if (input->user_buffer) {
			unsigned buffer_offset;

			r600_upload_const_buffer(rctx,
						 (struct r600_resource**)&buffer, input->user_buffer,
						 input->buffer_size, &buffer_offset);
			va = r600_resource_va(ctx->screen, buffer) + buffer_offset;
		} else {
			pipe_resource_reference(&buffer, input->buffer);
			va = r600_resource_va(ctx->screen, buffer) + input->buffer_offset;
		}

		/* Set the descriptor. */
		uint32_t *desc = buffers->desc_data[slot];
		desc[0] = va;
		desc[1] = S_008F04_BASE_ADDRESS_HI(va >> 32) |
			  S_008F04_STRIDE(0);
		desc[2] = input->buffer_size;
		desc[3] = S_008F0C_DST_SEL_X(V_008F0C_SQ_SEL_X) |
			  S_008F0C_DST_SEL_Y(V_008F0C_SQ_SEL_Y) |
			  S_008F0C_DST_SEL_Z(V_008F0C_SQ_SEL_Z) |
			  S_008F0C_DST_SEL_W(V_008F0C_SQ_SEL_W) |
			  S_008F0C_NUM_FORMAT(V_008F0C_BUF_NUM_FORMAT_FLOAT) |
			  S_008F0C_DATA_FORMAT(V_008F0C_BUF_DATA_FORMAT_32);

		buffers->buffers[slot] = buffer;
		r600_context_bo_reloc(&rctx->b, &rctx->b.rings.gfx,
				      (struct r600_resource*)buffer, buffers->shader_usage);
		buffers->desc.enabled_mask |= 1 << slot;
	} else {
		/* Clear the descriptor. */
		memset(buffers->desc_data[slot], 0, sizeof(uint32_t) * 4);
		buffers->desc.enabled_mask &= ~(1 << slot);
	}

	buffers->desc.dirty_mask |= 1 << slot;
	si_update_descriptors(rctx, &buffers->desc);
}

/* STREAMOUT BUFFERS */

static void si_set_streamout_targets(struct pipe_context *ctx,
				     unsigned num_targets,
				     struct pipe_stream_output_target **targets,
				     unsigned append_bitmask)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct si_buffer_resources *buffers = &rctx->streamout_buffers;
	unsigned old_num_targets = rctx->b.streamout.num_targets;
	unsigned i;

	/* Streamout buffers must be bound in 2 places:
	 * 1) in VGT by setting the VGT_STRMOUT registers
	 * 2) as shader resources
	 */

	/* Set the VGT regs. */
	r600_set_streamout_targets(ctx, num_targets, targets, append_bitmask);

	/* Set the shader resources.*/
	for (i = 0; i < num_targets; i++) {
		if (targets[i]) {
			struct pipe_resource *buffer = targets[i]->buffer;
			uint64_t va = r600_resource_va(ctx->screen, buffer);

			/* Set the descriptor. */
			uint32_t *desc = buffers->desc_data[i];
			desc[0] = va;
			desc[1] = S_008F04_BASE_ADDRESS_HI(va >> 32);
			desc[2] = 0xffffffff;
			desc[3] = S_008F0C_DST_SEL_X(V_008F0C_SQ_SEL_X) |
				  S_008F0C_DST_SEL_Y(V_008F0C_SQ_SEL_Y) |
				  S_008F0C_DST_SEL_Z(V_008F0C_SQ_SEL_Z) |
				  S_008F0C_DST_SEL_W(V_008F0C_SQ_SEL_W);

			/* Set the resource. */
			pipe_resource_reference(&buffers->buffers[i], buffer);
			r600_context_bo_reloc(&rctx->b, &rctx->b.rings.gfx,
					      (struct r600_resource*)buffer,
					      buffers->shader_usage);
			buffers->desc.enabled_mask |= 1 << i;
		} else {
			/* Clear the descriptor and unset the resource. */
			memset(buffers->desc_data[i], 0, sizeof(uint32_t) * 4);
			pipe_resource_reference(&buffers->buffers[i], NULL);
			buffers->desc.enabled_mask &= ~(1 << i);
		}
		buffers->desc.dirty_mask |= 1 << i;
	}
	for (; i < old_num_targets; i++) {
		/* Clear the descriptor and unset the resource. */
		memset(buffers->desc_data[i], 0, sizeof(uint32_t) * 4);
		pipe_resource_reference(&buffers->buffers[i], NULL);
		buffers->desc.enabled_mask &= ~(1 << i);
		buffers->desc.dirty_mask |= 1 << i;
	}

	si_update_descriptors(rctx, &buffers->desc);
}

/* CP DMA */

/* The max number of bytes to copy per packet. */
#define CP_DMA_MAX_BYTE_COUNT ((1 << 21) - 8)

static void si_clear_buffer(struct pipe_context *ctx, struct pipe_resource *dst,
			    unsigned offset, unsigned size, unsigned value)
{
	struct r600_context *rctx = (struct r600_context*)ctx;

	if (!size)
		return;

	/* Fallback for unaligned clears. */
	if (offset % 4 != 0 || size % 4 != 0) {
		uint32_t *map = rctx->b.ws->buffer_map(r600_resource(dst)->cs_buf,
						       rctx->b.rings.gfx.cs,
						       PIPE_TRANSFER_WRITE);
		size /= 4;
		for (unsigned i = 0; i < size; i++)
			*map++ = value;

		util_range_add(&r600_resource(dst)->valid_buffer_range, offset,
			       offset + size);
		return;
	}

	uint64_t va = r600_resource_va(&rctx->screen->b.b, dst) + offset;

	/* Flush the caches where the resource is bound. */
	/* XXX only flush the caches where the buffer is bound. */
	rctx->b.flags |= R600_CONTEXT_INV_TEX_CACHE |
			 R600_CONTEXT_INV_CONST_CACHE |
			 R600_CONTEXT_FLUSH_AND_INV_CB |
			 R600_CONTEXT_FLUSH_AND_INV_DB |
			 R600_CONTEXT_FLUSH_AND_INV_CB_META |
			 R600_CONTEXT_FLUSH_AND_INV_DB_META;
	rctx->b.flags |= R600_CONTEXT_WAIT_3D_IDLE;

	while (size) {
		unsigned byte_count = MIN2(size, CP_DMA_MAX_BYTE_COUNT);
		unsigned dma_flags = 0;

		si_need_cs_space(rctx, 7 + (rctx->b.flags ? rctx->cache_flush.num_dw : 0),
				 FALSE);

		/* This must be done after need_cs_space. */
		r600_context_bo_reloc(&rctx->b, &rctx->b.rings.gfx,
				      (struct r600_resource*)dst, RADEON_USAGE_WRITE);

		/* Flush the caches for the first copy only.
		 * Also wait for the previous CP DMA operations. */
		if (rctx->b.flags) {
			si_emit_cache_flush(&rctx->b, NULL);
			dma_flags |= SI_CP_DMA_RAW_WAIT; /* same as WAIT_UNTIL=CP_DMA_IDLE */
		}

		/* Do the synchronization after the last copy, so that all data is written to memory. */
		if (size == byte_count)
			dma_flags |= R600_CP_DMA_SYNC;

		/* Emit the clear packet. */
		si_emit_cp_dma_clear_buffer(rctx, va, byte_count, value, dma_flags);

		size -= byte_count;
		va += byte_count;
	}

	/* Flush the caches again in case the 3D engine has been prefetching
	 * the resource. */
	/* XXX only flush the caches where the buffer is bound. */
	rctx->b.flags |= R600_CONTEXT_INV_TEX_CACHE |
			 R600_CONTEXT_INV_CONST_CACHE |
			 R600_CONTEXT_FLUSH_AND_INV_CB |
			 R600_CONTEXT_FLUSH_AND_INV_DB |
			 R600_CONTEXT_FLUSH_AND_INV_CB_META |
			 R600_CONTEXT_FLUSH_AND_INV_DB_META;

	util_range_add(&r600_resource(dst)->valid_buffer_range, offset,
		       offset + size);
}

/* INIT/DEINIT */

void si_init_all_descriptors(struct r600_context *rctx)
{
	int i;

	for (i = 0; i < SI_NUM_SHADERS; i++) {
		si_init_buffer_resources(rctx, &rctx->const_buffers[i],
					 NUM_CONST_BUFFERS, i, SI_SGPR_CONST,
					 RADEON_USAGE_READ);

		si_init_sampler_views(rctx, &rctx->samplers[i].views, i);

		rctx->atoms.const_buffers[i] = &rctx->const_buffers[i].desc.atom;
		rctx->atoms.sampler_views[i] = &rctx->samplers[i].views.desc.atom;
	}

	si_init_buffer_resources(rctx, &rctx->streamout_buffers, 4, PIPE_SHADER_VERTEX,
				 SI_SGPR_SO_BUFFER, RADEON_USAGE_WRITE);
	rctx->atoms.streamout_buffers = &rctx->streamout_buffers.desc.atom;

	/* Set pipe_context functions. */
	rctx->b.b.set_constant_buffer = si_set_constant_buffer;
	rctx->b.b.set_stream_output_targets = si_set_streamout_targets;
	rctx->b.clear_buffer = si_clear_buffer;
}

void si_release_all_descriptors(struct r600_context *rctx)
{
	int i;

	for (i = 0; i < SI_NUM_SHADERS; i++) {
		si_release_buffer_resources(&rctx->const_buffers[i]);
		si_release_sampler_views(&rctx->samplers[i].views);
	}
	si_release_buffer_resources(&rctx->streamout_buffers);
}

void si_all_descriptors_begin_new_cs(struct r600_context *rctx)
{
	int i;

	for (i = 0; i < SI_NUM_SHADERS; i++) {
		si_buffer_resources_begin_new_cs(rctx, &rctx->const_buffers[i]);
		si_sampler_views_begin_new_cs(rctx, &rctx->samplers[i].views);
	}
	si_buffer_resources_begin_new_cs(rctx, &rctx->streamout_buffers);
}

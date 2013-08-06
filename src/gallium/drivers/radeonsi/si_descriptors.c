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

#include "radeonsi_pipe.h"
#include "radeonsi_resource.h"
#include "radeonsi_shader.h"
#include "r600_hw_context_priv.h"

#include "util/u_memory.h"

#define SI_NUM_CONTEXTS 256

static const uint32_t null_desc[8]; /* zeros */

/* Set this if you want the 3D engine to wait until CP DMA is done.
 * It should be set on the last CP DMA packet. */
#define R600_CP_DMA_SYNC	(1 << 0) /* R600+ */

/* Set this if the source data was used as a destination in a previous CP DMA
 * packet. It's for preventing a read-after-write (RAW) hazard between two
 * CP DMA packets. */
#define SI_CP_DMA_RAW_WAIT	(1 << 1) /* SI+ */

/* Emit a CP DMA packet to do a copy from one buffer to another.
 * The size must fit in bits [20:0]. Notes:
 */
static void si_emit_cp_dma_copy_buffer(struct r600_context *rctx,
				       uint64_t dst_va, uint64_t src_va,
				       unsigned size, unsigned flags)
{
	struct radeon_winsys_cs *cs = rctx->cs;
	uint32_t sync_flag = flags & R600_CP_DMA_SYNC ? PKT3_CP_DMA_CP_SYNC : 0;
	uint32_t raw_wait = flags & SI_CP_DMA_RAW_WAIT ? PKT3_CP_DMA_CMD_RAW_WAIT : 0;

	assert(size);
	assert((size & ((1<<21)-1)) == size);

	if (rctx->chip_class >= CIK) {
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
	struct radeon_winsys_cs *cs = rctx->cs;
	uint32_t sync_flag = flags & R600_CP_DMA_SYNC ? PKT3_CP_DMA_CP_SYNC : 0;
	uint32_t raw_wait = flags & SI_CP_DMA_RAW_WAIT ? PKT3_CP_DMA_CMD_RAW_WAIT : 0;

	assert(size);
	assert((size & ((1<<21)-1)) == size);

	if (rctx->chip_class >= CIK) {
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
				void (*emit_func)(struct r600_context *ctx, struct si_atom *state))
{
	uint64_t va;

	desc->atom.emit = emit_func;
	desc->shader_userdata_reg = shader_userdata_reg;
	desc->element_dw_size = element_dw_size;
	desc->num_elements = num_elements;
	desc->context_size = num_elements * element_dw_size * 4;

	desc->buffer = (struct si_resource*)
		pipe_buffer_create(rctx->context.screen, PIPE_BIND_CUSTOM,
				   PIPE_USAGE_STATIC,
				   SI_NUM_CONTEXTS * desc->context_size);

	r600_context_bo_reloc(rctx, desc->buffer, RADEON_USAGE_READWRITE);
	va = r600_resource_va(rctx->context.screen, &desc->buffer->b.b);

	/* We don't check for CS space here, because this should be called
	 * only once at context initialization. */
	si_emit_cp_dma_clear_buffer(rctx, va, desc->buffer->b.b.width0, 0,
				    R600_CP_DMA_SYNC);
}

static void si_release_descriptors(struct si_descriptors *desc)
{
	pipe_resource_reference((struct pipe_resource**)&desc->buffer, NULL);
}

static void si_update_descriptors(struct si_descriptors *desc)
{
	if (desc->dirty_mask) {
		desc->atom.num_dw =
			7 + /* copy */
			(4 + desc->element_dw_size) * util_bitcount(desc->dirty_mask) + /* update */
			4; /* pointer update */
		desc->atom.dirty = true;
	} else {
		desc->atom.dirty = false;
	}
}

static void si_emit_shader_pointer(struct r600_context *rctx,
				   struct si_descriptors *desc)
{
	struct radeon_winsys_cs *cs = rctx->cs;
	uint64_t va = r600_resource_va(rctx->context.screen, &desc->buffer->b.b) +
		      desc->current_context_id * desc->context_size;

	radeon_emit(cs, PKT3(PKT3_SET_SH_REG, 2, 0));
	radeon_emit(cs, (desc->shader_userdata_reg - SI_SH_REG_OFFSET) >> 2);
	radeon_emit(cs, va);
	radeon_emit(cs, va >> 32);
}

static void si_emit_descriptors(struct r600_context *rctx,
				struct si_descriptors *desc,
				const uint32_t **descriptors)
{
	struct radeon_winsys_cs *cs = rctx->cs;
	uint64_t va_base;
	int packet_start;
	int packet_size = 0;
	int last_index = desc->num_elements; /* point to a non-existing element */
	unsigned dirty_mask = desc->dirty_mask;
	unsigned new_context_id = (desc->current_context_id + 1) % SI_NUM_CONTEXTS;

	assert(dirty_mask);

	va_base = r600_resource_va(rctx->context.screen, &desc->buffer->b.b);

	/* Copy the descriptors to a new context slot. */
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
			radeon_emit(cs, PKT3_WRITE_DATA_DST_SEL(PKT3_WRITE_DATA_DST_SEL_MEM_SYNC) |
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

static void si_emit_sampler_views(struct r600_context *rctx, struct si_atom *atom)
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
			    8, 16, si_emit_sampler_views);
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

		r600_context_bo_reloc(rctx, rview->resource, RADEON_USAGE_READ);
	}

	r600_context_bo_reloc(rctx, views->desc.buffer, RADEON_USAGE_READWRITE);

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

		r600_context_bo_reloc(rctx, rview->resource, RADEON_USAGE_READ);

		pipe_sampler_view_reference(&views->views[slot], view);
		views->desc_data[slot] = view_desc;
		views->desc.enabled_mask |= 1 << slot;
	} else {
		pipe_sampler_view_reference(&views->views[slot], NULL);
		views->desc_data[slot] = null_desc;
		views->desc.enabled_mask &= ~(1 << slot);
	}

	views->desc.dirty_mask |= 1 << slot;
	si_update_descriptors(&views->desc);
}

/* INIT/DEINIT */

void si_init_all_descriptors(struct r600_context *rctx)
{
	int i;

	for (i = 0; i < SI_NUM_SHADERS; i++) {
		si_init_sampler_views(rctx, &rctx->samplers[i].views, i);

		rctx->atoms.sampler_views[i] = &rctx->samplers[i].views.desc.atom;
	}
}

void si_release_all_descriptors(struct r600_context *rctx)
{
	int i;

	for (i = 0; i < SI_NUM_SHADERS; i++) {
		si_release_sampler_views(&rctx->samplers[i].views);
	}
}

void si_all_descriptors_begin_new_cs(struct r600_context *rctx)
{
	int i;

	for (i = 0; i < SI_NUM_SHADERS; i++) {
		si_sampler_views_begin_new_cs(rctx, &rctx->samplers[i].views);
	}
}

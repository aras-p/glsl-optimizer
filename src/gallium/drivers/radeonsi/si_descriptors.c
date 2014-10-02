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

/* Resource binding slots and sampler states (each described with 8 or 4 dwords)
 * live in memory on SI.
 *
 * This file is responsible for managing lists of resources and sampler states
 * in memory and binding them, which means updating those structures in memory.
 *
 * There is also code for updating shader pointers to resources and sampler
 * states. CP DMA functions are here too.
 */

#include "radeon/r600_cs.h"
#include "si_pipe.h"
#include "si_shader.h"
#include "sid.h"

#include "util/u_memory.h"
#include "util/u_upload_mgr.h"

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
static void si_emit_cp_dma_copy_buffer(struct si_context *sctx,
				       uint64_t dst_va, uint64_t src_va,
				       unsigned size, unsigned flags)
{
	struct radeon_winsys_cs *cs = sctx->b.rings.gfx.cs;
	uint32_t sync_flag = flags & R600_CP_DMA_SYNC ? PKT3_CP_DMA_CP_SYNC : 0;
	uint32_t raw_wait = flags & SI_CP_DMA_RAW_WAIT ? PKT3_CP_DMA_CMD_RAW_WAIT : 0;

	assert(size);
	assert((size & ((1<<21)-1)) == size);

	if (sctx->b.chip_class >= CIK) {
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
static void si_emit_cp_dma_clear_buffer(struct si_context *sctx,
					uint64_t dst_va, unsigned size,
					uint32_t clear_value, unsigned flags)
{
	struct radeon_winsys_cs *cs = sctx->b.rings.gfx.cs;
	uint32_t sync_flag = flags & R600_CP_DMA_SYNC ? PKT3_CP_DMA_CP_SYNC : 0;
	uint32_t raw_wait = flags & SI_CP_DMA_RAW_WAIT ? PKT3_CP_DMA_CMD_RAW_WAIT : 0;

	assert(size);
	assert((size & ((1<<21)-1)) == size);

	if (sctx->b.chip_class >= CIK) {
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

static void si_init_descriptors(struct si_context *sctx,
				struct si_descriptors *desc,
				unsigned shader_userdata_reg,
				unsigned element_dw_size,
				unsigned num_elements,
				void (*emit_func)(struct si_context *ctx, struct r600_atom *state))
{
	assert(num_elements <= sizeof(desc->enabled_mask)*8);
	assert(num_elements <= sizeof(desc->dirty_mask)*8);

	desc->atom.emit = (void*)emit_func;
	desc->shader_userdata_reg = shader_userdata_reg;
	desc->element_dw_size = element_dw_size;
	desc->num_elements = num_elements;
	desc->context_size = num_elements * element_dw_size * 4;

	desc->buffer = (struct r600_resource*)
		pipe_buffer_create(sctx->b.b.screen, PIPE_BIND_CUSTOM,
				   PIPE_USAGE_DEFAULT,
				   SI_NUM_CONTEXTS * desc->context_size);

	r600_context_bo_reloc(&sctx->b, &sctx->b.rings.gfx, desc->buffer,
			      RADEON_USAGE_READWRITE, RADEON_PRIO_SHADER_DATA);

	/* We don't check for CS space here, because this should be called
	 * only once at context initialization. */
	si_emit_cp_dma_clear_buffer(sctx, desc->buffer->gpu_address,
				    desc->buffer->b.b.width0, 0,
				    R600_CP_DMA_SYNC);
}

static void si_release_descriptors(struct si_descriptors *desc)
{
	pipe_resource_reference((struct pipe_resource**)&desc->buffer, NULL);
}

static void si_update_descriptors(struct si_context *sctx,
				  struct si_descriptors *desc)
{
	if (desc->dirty_mask) {
		desc->atom.num_dw =
			7 + /* copy */
			(4 + desc->element_dw_size) * util_bitcount(desc->dirty_mask) + /* update */
			4; /* pointer update */

		if (desc->shader_userdata_reg >= R_00B130_SPI_SHADER_USER_DATA_VS_0 &&
		    desc->shader_userdata_reg < R_00B230_SPI_SHADER_USER_DATA_GS_0)
			desc->atom.num_dw += 4; /* second pointer update */

		desc->atom.dirty = true;
		/* The descriptors are read with the K cache. */
		sctx->b.flags |= R600_CONTEXT_INV_CONST_CACHE;
	} else {
		desc->atom.dirty = false;
	}
}

static void si_emit_shader_pointer(struct si_context *sctx,
				   struct r600_atom *atom)
{
	struct si_descriptors *desc = (struct si_descriptors*)atom;
	struct radeon_winsys_cs *cs = sctx->b.rings.gfx.cs;
	uint64_t va = desc->buffer->gpu_address +
		      desc->current_context_id * desc->context_size +
		      desc->buffer_offset;

	radeon_emit(cs, PKT3(PKT3_SET_SH_REG, 2, 0));
	radeon_emit(cs, (desc->shader_userdata_reg - SI_SH_REG_OFFSET) >> 2);
	radeon_emit(cs, va);
	radeon_emit(cs, va >> 32);

	if (desc->shader_userdata_reg >= R_00B130_SPI_SHADER_USER_DATA_VS_0 &&
	    desc->shader_userdata_reg < R_00B230_SPI_SHADER_USER_DATA_GS_0) {
		radeon_emit(cs, PKT3(PKT3_SET_SH_REG, 2, 0));
		radeon_emit(cs, (desc->shader_userdata_reg +
				 (R_00B330_SPI_SHADER_USER_DATA_ES_0 -
				  R_00B130_SPI_SHADER_USER_DATA_VS_0) -
				 SI_SH_REG_OFFSET) >> 2);
		radeon_emit(cs, va);
		radeon_emit(cs, va >> 32);
	}
}

static void si_emit_descriptors(struct si_context *sctx,
				struct si_descriptors *desc,
				uint32_t **descriptors)
{
	struct radeon_winsys_cs *cs = sctx->b.rings.gfx.cs;
	uint64_t va_base;
	int packet_start;
	int packet_size = 0;
	int last_index = desc->num_elements; /* point to a non-existing element */
	unsigned dirty_mask = desc->dirty_mask;
	unsigned new_context_id = (desc->current_context_id + 1) % SI_NUM_CONTEXTS;

	assert(dirty_mask);

	va_base = desc->buffer->gpu_address;

	/* Copy the descriptors to a new context slot. */
	/* XXX Consider using TC or L2 for this copy on CIK. */
	si_emit_cp_dma_copy_buffer(sctx,
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
	si_emit_shader_pointer(sctx, &desc->atom);
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

static void si_emit_sampler_views(struct si_context *sctx, struct r600_atom *atom)
{
	struct si_sampler_views *views = (struct si_sampler_views*)atom;

	si_emit_descriptors(sctx, &views->desc, views->desc_data);
}

static void si_init_sampler_views(struct si_context *sctx,
				  struct si_sampler_views *views,
				  unsigned shader)
{
	si_init_descriptors(sctx, &views->desc,
			    si_get_shader_user_data_base(shader) +
			    SI_SGPR_RESOURCE * 4,
			    8, SI_NUM_SAMPLER_VIEWS, si_emit_sampler_views);
}

static void si_release_sampler_views(struct si_sampler_views *views)
{
	int i;

	for (i = 0; i < Elements(views->views); i++) {
		pipe_sampler_view_reference(&views->views[i], NULL);
	}
	si_release_descriptors(&views->desc);
}

static enum radeon_bo_priority si_get_resource_ro_priority(struct r600_resource *res)
{
	if (res->b.b.target == PIPE_BUFFER)
		return RADEON_PRIO_SHADER_BUFFER_RO;

	if (res->b.b.nr_samples > 1)
		return RADEON_PRIO_SHADER_TEXTURE_MSAA;

	return RADEON_PRIO_SHADER_TEXTURE_RO;
}

static void si_sampler_views_begin_new_cs(struct si_context *sctx,
					  struct si_sampler_views *views)
{
	unsigned mask = views->desc.enabled_mask;

	/* Add relocations to the CS. */
	while (mask) {
		int i = u_bit_scan(&mask);
		struct si_sampler_view *rview =
			(struct si_sampler_view*)views->views[i];

		r600_context_bo_reloc(&sctx->b, &sctx->b.rings.gfx,
				      rview->resource, RADEON_USAGE_READ,
				      si_get_resource_ro_priority(rview->resource));
	}

	r600_context_bo_reloc(&sctx->b, &sctx->b.rings.gfx, views->desc.buffer,
			      RADEON_USAGE_READWRITE, RADEON_PRIO_SHADER_DATA);

	si_emit_shader_pointer(sctx, &views->desc.atom);
}

static void si_set_sampler_view(struct si_context *sctx, unsigned shader,
				unsigned slot, struct pipe_sampler_view *view,
				unsigned *view_desc)
{
	struct si_sampler_views *views = &sctx->samplers[shader].views;

	if (views->views[slot] == view)
		return;

	if (view) {
		struct si_sampler_view *rview =
			(struct si_sampler_view*)view;

		r600_context_bo_reloc(&sctx->b, &sctx->b.rings.gfx,
				      rview->resource, RADEON_USAGE_READ,
				      si_get_resource_ro_priority(rview->resource));

		pipe_sampler_view_reference(&views->views[slot], view);
		views->desc_data[slot] = view_desc;
		views->desc.enabled_mask |= 1 << slot;
	} else {
		pipe_sampler_view_reference(&views->views[slot], NULL);
		views->desc_data[slot] = null_desc;
		views->desc.enabled_mask &= ~(1 << slot);
	}

	views->desc.dirty_mask |= 1 << slot;
}

static void si_set_sampler_views(struct pipe_context *ctx,
				 unsigned shader, unsigned start,
                                 unsigned count,
				 struct pipe_sampler_view **views)
{
	struct si_context *sctx = (struct si_context *)ctx;
	struct si_textures_info *samplers = &sctx->samplers[shader];
	struct si_sampler_view **rviews = (struct si_sampler_view **)views;
	int i;

	if (!count || shader >= SI_NUM_SHADERS)
		return;

	for (i = 0; i < count; i++) {
		unsigned slot = start + i;

		if (!views[i]) {
			samplers->depth_texture_mask &= ~(1 << slot);
			samplers->compressed_colortex_mask &= ~(1 << slot);
			si_set_sampler_view(sctx, shader, slot, NULL, NULL);
			si_set_sampler_view(sctx, shader, SI_FMASK_TEX_OFFSET + slot,
					    NULL, NULL);
			continue;
		}

		si_set_sampler_view(sctx, shader, slot, views[i], rviews[i]->state);

		if (views[i]->texture->target != PIPE_BUFFER) {
			struct r600_texture *rtex =
				(struct r600_texture*)views[i]->texture;

			if (rtex->is_depth && !rtex->is_flushing_texture) {
				samplers->depth_texture_mask |= 1 << slot;
			} else {
				samplers->depth_texture_mask &= ~(1 << slot);
			}
			if (rtex->cmask.size || rtex->fmask.size) {
				samplers->compressed_colortex_mask |= 1 << slot;
			} else {
				samplers->compressed_colortex_mask &= ~(1 << slot);
			}

			if (rtex->fmask.size) {
				si_set_sampler_view(sctx, shader, SI_FMASK_TEX_OFFSET + slot,
						    views[i], rviews[i]->fmask_state);
			} else {
				si_set_sampler_view(sctx, shader, SI_FMASK_TEX_OFFSET + slot,
						    NULL, NULL);
			}
		} else {
			samplers->depth_texture_mask &= ~(1 << slot);
			samplers->compressed_colortex_mask &= ~(1 << slot);
			si_set_sampler_view(sctx, shader, SI_FMASK_TEX_OFFSET + slot,
					    NULL, NULL);
		}
	}

	sctx->b.flags |= R600_CONTEXT_INV_TEX_CACHE;
	si_update_descriptors(sctx, &samplers->views.desc);
}

/* SAMPLER STATES */

static void si_emit_sampler_states(struct si_context *sctx, struct r600_atom *atom)
{
	struct si_sampler_states *states = (struct si_sampler_states*)atom;

	si_emit_descriptors(sctx, &states->desc, states->desc_data);
}

static void si_sampler_states_begin_new_cs(struct si_context *sctx,
					   struct si_sampler_states *states)
{
	r600_context_bo_reloc(&sctx->b, &sctx->b.rings.gfx, states->desc.buffer,
			      RADEON_USAGE_READWRITE, RADEON_PRIO_SHADER_DATA);
	si_emit_shader_pointer(sctx, &states->desc.atom);
}

void si_set_sampler_descriptors(struct si_context *sctx, unsigned shader,
				unsigned start, unsigned count, void **states)
{
	struct si_sampler_states *samplers = &sctx->samplers[shader].states;
	struct si_sampler_state **sstates = (struct si_sampler_state**)states;
	int i;

	if (start == 0)
		samplers->saved_states[0] = states[0];
	if (start == 1)
		samplers->saved_states[1] = states[0];
	else if (start == 0 && count >= 2)
		samplers->saved_states[1] = states[1];

	for (i = 0; i < count; i++) {
		unsigned slot = start + i;

		if (!sstates[i]) {
			samplers->desc.dirty_mask &= ~(1 << slot);
			continue;
		}

		samplers->desc_data[slot] = sstates[i]->val;
		samplers->desc.dirty_mask |= 1 << slot;
	}

	si_update_descriptors(sctx, &samplers->desc);
}

/* BUFFER RESOURCES */

static void si_emit_buffer_resources(struct si_context *sctx, struct r600_atom *atom)
{
	struct si_buffer_resources *buffers = (struct si_buffer_resources*)atom;

	si_emit_descriptors(sctx, &buffers->desc, buffers->desc_data);
}

static void si_init_buffer_resources(struct si_context *sctx,
				     struct si_buffer_resources *buffers,
				     unsigned num_buffers, unsigned shader,
				     unsigned shader_userdata_index,
				     enum radeon_bo_usage shader_usage,
				     enum radeon_bo_priority priority)
{
	int i;

	buffers->num_buffers = num_buffers;
	buffers->shader_usage = shader_usage;
	buffers->priority = priority;
	buffers->buffers = CALLOC(num_buffers, sizeof(struct pipe_resource*));
	buffers->desc_storage = CALLOC(num_buffers, sizeof(uint32_t) * 4);

	/* si_emit_descriptors only accepts an array of arrays.
	 * This adds such an array. */
	buffers->desc_data = CALLOC(num_buffers, sizeof(uint32_t*));
	for (i = 0; i < num_buffers; i++) {
		buffers->desc_data[i] = &buffers->desc_storage[i*4];
	}

	si_init_descriptors(sctx, &buffers->desc,
			    si_get_shader_user_data_base(shader) +
			    shader_userdata_index*4, 4, num_buffers,
			    si_emit_buffer_resources);
}

static void si_release_buffer_resources(struct si_buffer_resources *buffers)
{
	int i;

	for (i = 0; i < buffers->num_buffers; i++) {
		pipe_resource_reference(&buffers->buffers[i], NULL);
	}

	FREE(buffers->buffers);
	FREE(buffers->desc_storage);
	FREE(buffers->desc_data);
	si_release_descriptors(&buffers->desc);
}

static void si_buffer_resources_begin_new_cs(struct si_context *sctx,
					     struct si_buffer_resources *buffers)
{
	unsigned mask = buffers->desc.enabled_mask;

	/* Add relocations to the CS. */
	while (mask) {
		int i = u_bit_scan(&mask);

		r600_context_bo_reloc(&sctx->b, &sctx->b.rings.gfx,
				      (struct r600_resource*)buffers->buffers[i],
				      buffers->shader_usage, buffers->priority);
	}

	r600_context_bo_reloc(&sctx->b, &sctx->b.rings.gfx,
			      buffers->desc.buffer, RADEON_USAGE_READWRITE,
			      RADEON_PRIO_SHADER_DATA);

	si_emit_shader_pointer(sctx, &buffers->desc.atom);
}

/* VERTEX BUFFERS */

static void si_vertex_buffers_begin_new_cs(struct si_context *sctx)
{
	struct si_descriptors *desc = &sctx->vertex_buffers;
	int count = sctx->vertex_elements ? sctx->vertex_elements->count : 0;
	int i;

	for (i = 0; i < count; i++) {
		int vb = sctx->vertex_elements->elements[i].vertex_buffer_index;

		if (vb >= Elements(sctx->vertex_buffer))
			continue;
		if (!sctx->vertex_buffer[vb].buffer)
			continue;

		r600_context_bo_reloc(&sctx->b, &sctx->b.rings.gfx,
				      (struct r600_resource*)sctx->vertex_buffer[vb].buffer,
				      RADEON_USAGE_READ, RADEON_PRIO_SHADER_BUFFER_RO);
	}
	r600_context_bo_reloc(&sctx->b, &sctx->b.rings.gfx,
			      desc->buffer, RADEON_USAGE_READ,
			      RADEON_PRIO_SHADER_DATA);

	si_emit_shader_pointer(sctx, &desc->atom);
}

void si_update_vertex_buffers(struct si_context *sctx)
{
	struct si_descriptors *desc = &sctx->vertex_buffers;
	bool bound[SI_NUM_VERTEX_BUFFERS] = {};
	unsigned i, count = sctx->vertex_elements->count;
	uint64_t va;
	uint32_t *ptr;

	if (!count || !sctx->vertex_elements)
		return;

	/* Vertex buffer descriptors are the only ones which are uploaded
	 * directly through a staging buffer and don't go through
	 * the fine-grained upload path.
	 */
	u_upload_alloc(sctx->b.uploader, 0, count * 16, &desc->buffer_offset,
		       (struct pipe_resource**)&desc->buffer, (void**)&ptr);

	r600_context_bo_reloc(&sctx->b, &sctx->b.rings.gfx,
			      desc->buffer, RADEON_USAGE_READ,
			      RADEON_PRIO_SHADER_DATA);

	assert(count <= SI_NUM_VERTEX_BUFFERS);
	assert(desc->current_context_id == 0);

	for (i = 0; i < count; i++) {
		struct pipe_vertex_element *ve = &sctx->vertex_elements->elements[i];
		struct pipe_vertex_buffer *vb;
		struct r600_resource *rbuffer;
		unsigned offset;
		uint32_t *desc = &ptr[i*4];

		if (ve->vertex_buffer_index >= Elements(sctx->vertex_buffer)) {
			memset(desc, 0, 16);
			continue;
		}

		vb = &sctx->vertex_buffer[ve->vertex_buffer_index];
		rbuffer = (struct r600_resource*)vb->buffer;
		if (rbuffer == NULL) {
			memset(desc, 0, 16);
			continue;
		}

		offset = vb->buffer_offset + ve->src_offset;
		va = rbuffer->gpu_address + offset;

		/* Fill in T# buffer resource description */
		desc[0] = va & 0xFFFFFFFF;
		desc[1] = S_008F04_BASE_ADDRESS_HI(va >> 32) |
			  S_008F04_STRIDE(vb->stride);
		if (vb->stride)
			/* Round up by rounding down and adding 1 */
			desc[2] = (vb->buffer->width0 - offset -
				   sctx->vertex_elements->format_size[i]) /
				  vb->stride + 1;
		else
			desc[2] = vb->buffer->width0 - offset;

		desc[3] = sctx->vertex_elements->rsrc_word3[i];

		if (!bound[ve->vertex_buffer_index]) {
			r600_context_bo_reloc(&sctx->b, &sctx->b.rings.gfx,
					      (struct r600_resource*)vb->buffer,
					      RADEON_USAGE_READ, RADEON_PRIO_SHADER_BUFFER_RO);
			bound[ve->vertex_buffer_index] = true;
		}
	}

	desc->atom.num_dw = 8; /* update 2 shader pointers (VS+ES) */
	desc->atom.dirty = true;

	/* Don't flush the const cache. It would have a very negative effect
	 * on performance (confirmed by testing). New descriptors are always
	 * uploaded to a fresh new buffer, so I don't think flushing the const
	 * cache is needed. */
	sctx->b.flags |= R600_CONTEXT_INV_TEX_CACHE;
}


/* CONSTANT BUFFERS */

void si_upload_const_buffer(struct si_context *sctx, struct r600_resource **rbuffer,
			    const uint8_t *ptr, unsigned size, uint32_t *const_offset)
{
	void *tmp;

	u_upload_alloc(sctx->b.uploader, 0, size, const_offset,
		       (struct pipe_resource**)rbuffer, &tmp);
	util_memcpy_cpu_to_le32(tmp, ptr, size);
}

static void si_set_constant_buffer(struct pipe_context *ctx, uint shader, uint slot,
				   struct pipe_constant_buffer *input)
{
	struct si_context *sctx = (struct si_context *)ctx;
	struct si_buffer_resources *buffers = &sctx->const_buffers[shader];

	if (shader >= SI_NUM_SHADERS)
		return;

	assert(slot < buffers->num_buffers);
	pipe_resource_reference(&buffers->buffers[slot], NULL);

	/* CIK cannot unbind a constant buffer (S_BUFFER_LOAD is buggy
	 * with a NULL buffer). We need to use a dummy buffer instead. */
	if (sctx->b.chip_class == CIK &&
	    (!input || (!input->buffer && !input->user_buffer)))
		input = &sctx->null_const_buf;

	if (input && (input->buffer || input->user_buffer)) {
		struct pipe_resource *buffer = NULL;
		uint64_t va;

		/* Upload the user buffer if needed. */
		if (input->user_buffer) {
			unsigned buffer_offset;

			si_upload_const_buffer(sctx,
					       (struct r600_resource**)&buffer, input->user_buffer,
					       input->buffer_size, &buffer_offset);
			va = r600_resource(buffer)->gpu_address + buffer_offset;
		} else {
			pipe_resource_reference(&buffer, input->buffer);
			va = r600_resource(buffer)->gpu_address + input->buffer_offset;
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
		r600_context_bo_reloc(&sctx->b, &sctx->b.rings.gfx,
				      (struct r600_resource*)buffer,
				      buffers->shader_usage, buffers->priority);
		buffers->desc.enabled_mask |= 1 << slot;
	} else {
		/* Clear the descriptor. */
		memset(buffers->desc_data[slot], 0, sizeof(uint32_t) * 4);
		buffers->desc.enabled_mask &= ~(1 << slot);
	}

	buffers->desc.dirty_mask |= 1 << slot;
	si_update_descriptors(sctx, &buffers->desc);
}

/* RING BUFFERS */

void si_set_ring_buffer(struct pipe_context *ctx, uint shader, uint slot,
			struct pipe_resource *buffer,
			unsigned stride, unsigned num_records,
			bool add_tid, bool swizzle,
			unsigned element_size, unsigned index_stride)
{
	struct si_context *sctx = (struct si_context *)ctx;
	struct si_buffer_resources *buffers = &sctx->rw_buffers[shader];

	if (shader >= SI_NUM_SHADERS)
		return;

	/* The stride field in the resource descriptor has 14 bits */
	assert(stride < (1 << 14));

	assert(slot < buffers->num_buffers);
	pipe_resource_reference(&buffers->buffers[slot], NULL);

	if (buffer) {
		uint64_t va;

		va = r600_resource(buffer)->gpu_address;

		switch (element_size) {
		default:
			assert(!"Unsupported ring buffer element size");
		case 0:
		case 2:
			element_size = 0;
			break;
		case 4:
			element_size = 1;
			break;
		case 8:
			element_size = 2;
			break;
		case 16:
			element_size = 3;
			break;
		}

		switch (index_stride) {
		default:
			assert(!"Unsupported ring buffer index stride");
		case 0:
		case 8:
			index_stride = 0;
			break;
		case 16:
			index_stride = 1;
			break;
		case 32:
			index_stride = 2;
			break;
		case 64:
			index_stride = 3;
			break;
		}

		/* Set the descriptor. */
		uint32_t *desc = buffers->desc_data[slot];
		desc[0] = va;
		desc[1] = S_008F04_BASE_ADDRESS_HI(va >> 32) |
			  S_008F04_STRIDE(stride) |
			  S_008F04_SWIZZLE_ENABLE(swizzle);
		desc[2] = num_records;
		desc[3] = S_008F0C_DST_SEL_X(V_008F0C_SQ_SEL_X) |
			  S_008F0C_DST_SEL_Y(V_008F0C_SQ_SEL_Y) |
			  S_008F0C_DST_SEL_Z(V_008F0C_SQ_SEL_Z) |
			  S_008F0C_DST_SEL_W(V_008F0C_SQ_SEL_W) |
			  S_008F0C_NUM_FORMAT(V_008F0C_BUF_NUM_FORMAT_FLOAT) |
			  S_008F0C_DATA_FORMAT(V_008F0C_BUF_DATA_FORMAT_32) |
			  S_008F0C_ELEMENT_SIZE(element_size) |
			  S_008F0C_INDEX_STRIDE(index_stride) |
			  S_008F0C_ADD_TID_ENABLE(add_tid);

		pipe_resource_reference(&buffers->buffers[slot], buffer);
		r600_context_bo_reloc(&sctx->b, &sctx->b.rings.gfx,
				      (struct r600_resource*)buffer,
				      buffers->shader_usage, buffers->priority);
		buffers->desc.enabled_mask |= 1 << slot;
	} else {
		/* Clear the descriptor. */
		memset(buffers->desc_data[slot], 0, sizeof(uint32_t) * 4);
		buffers->desc.enabled_mask &= ~(1 << slot);
	}

	buffers->desc.dirty_mask |= 1 << slot;
	si_update_descriptors(sctx, &buffers->desc);
}

/* STREAMOUT BUFFERS */

static void si_set_streamout_targets(struct pipe_context *ctx,
				     unsigned num_targets,
				     struct pipe_stream_output_target **targets,
				     const unsigned *offsets)
{
	struct si_context *sctx = (struct si_context *)ctx;
	struct si_buffer_resources *buffers = &sctx->rw_buffers[PIPE_SHADER_VERTEX];
	unsigned old_num_targets = sctx->b.streamout.num_targets;
	unsigned i, bufidx;

	/* Streamout buffers must be bound in 2 places:
	 * 1) in VGT by setting the VGT_STRMOUT registers
	 * 2) as shader resources
	 */

	/* Set the VGT regs. */
	r600_set_streamout_targets(ctx, num_targets, targets, offsets);

	/* Set the shader resources.*/
	for (i = 0; i < num_targets; i++) {
		bufidx = SI_SO_BUF_OFFSET + i;

		if (targets[i]) {
			struct pipe_resource *buffer = targets[i]->buffer;
			uint64_t va = r600_resource(buffer)->gpu_address;

			/* Set the descriptor. */
			uint32_t *desc = buffers->desc_data[bufidx];
			desc[0] = va;
			desc[1] = S_008F04_BASE_ADDRESS_HI(va >> 32);
			desc[2] = 0xffffffff;
			desc[3] = S_008F0C_DST_SEL_X(V_008F0C_SQ_SEL_X) |
				  S_008F0C_DST_SEL_Y(V_008F0C_SQ_SEL_Y) |
				  S_008F0C_DST_SEL_Z(V_008F0C_SQ_SEL_Z) |
				  S_008F0C_DST_SEL_W(V_008F0C_SQ_SEL_W);

			/* Set the resource. */
			pipe_resource_reference(&buffers->buffers[bufidx],
						buffer);
			r600_context_bo_reloc(&sctx->b, &sctx->b.rings.gfx,
					      (struct r600_resource*)buffer,
					      buffers->shader_usage, buffers->priority);
			buffers->desc.enabled_mask |= 1 << bufidx;
		} else {
			/* Clear the descriptor and unset the resource. */
			memset(buffers->desc_data[bufidx], 0,
			       sizeof(uint32_t) * 4);
			pipe_resource_reference(&buffers->buffers[bufidx],
						NULL);
			buffers->desc.enabled_mask &= ~(1 << bufidx);
		}
		buffers->desc.dirty_mask |= 1 << bufidx;
	}
	for (; i < old_num_targets; i++) {
		bufidx = SI_SO_BUF_OFFSET + i;
		/* Clear the descriptor and unset the resource. */
		memset(buffers->desc_data[bufidx], 0, sizeof(uint32_t) * 4);
		pipe_resource_reference(&buffers->buffers[bufidx], NULL);
		buffers->desc.enabled_mask &= ~(1 << bufidx);
		buffers->desc.dirty_mask |= 1 << bufidx;
	}

	si_update_descriptors(sctx, &buffers->desc);
}

static void si_desc_reset_buffer_offset(struct pipe_context *ctx,
					uint32_t *desc, uint64_t old_buf_va,
					struct pipe_resource *new_buf)
{
	/* Retrieve the buffer offset from the descriptor. */
	uint64_t old_desc_va =
		desc[0] | ((uint64_t)G_008F04_BASE_ADDRESS_HI(desc[1]) << 32);

	assert(old_buf_va <= old_desc_va);
	uint64_t offset_within_buffer = old_desc_va - old_buf_va;

	/* Update the descriptor. */
	uint64_t va = r600_resource(new_buf)->gpu_address + offset_within_buffer;

	desc[0] = va;
	desc[1] = (desc[1] & C_008F04_BASE_ADDRESS_HI) |
		  S_008F04_BASE_ADDRESS_HI(va >> 32);
}

/* BUFFER DISCARD/INVALIDATION */

/* Reallocate a buffer a update all resource bindings where the buffer is
 * bound.
 *
 * This is used to avoid CPU-GPU synchronizations, because it makes the buffer
 * idle by discarding its contents. Apps usually tell us when to do this using
 * map_buffer flags, for example.
 */
static void si_invalidate_buffer(struct pipe_context *ctx, struct pipe_resource *buf)
{
	struct si_context *sctx = (struct si_context*)ctx;
	struct r600_resource *rbuffer = r600_resource(buf);
	unsigned i, shader, alignment = rbuffer->buf->alignment;
	uint64_t old_va = rbuffer->gpu_address;
	unsigned num_elems = sctx->vertex_elements ?
				       sctx->vertex_elements->count : 0;
	struct si_sampler_view *view;

	/* Reallocate the buffer in the same pipe_resource. */
	r600_init_resource(&sctx->screen->b, rbuffer, rbuffer->b.b.width0,
			   alignment, TRUE);

	/* We changed the buffer, now we need to bind it where the old one
	 * was bound. This consists of 2 things:
	 *   1) Updating the resource descriptor and dirtying it.
	 *   2) Adding a relocation to the CS, so that it's usable.
	 */

	/* Vertex buffers. */
	for (i = 0; i < num_elems; i++) {
		int vb = sctx->vertex_elements->elements[i].vertex_buffer_index;

		if (vb >= Elements(sctx->vertex_buffer))
			continue;
		if (!sctx->vertex_buffer[vb].buffer)
			continue;

		if (sctx->vertex_buffer[vb].buffer == buf) {
			sctx->vertex_buffers_dirty = true;
			break;
		}
	}

	/* Read/Write buffers. */
	for (shader = 0; shader < SI_NUM_SHADERS; shader++) {
		struct si_buffer_resources *buffers = &sctx->rw_buffers[shader];
		bool found = false;
		uint32_t mask = buffers->desc.enabled_mask;

		while (mask) {
			i = u_bit_scan(&mask);
			if (buffers->buffers[i] == buf) {
				si_desc_reset_buffer_offset(ctx, buffers->desc_data[i],
							    old_va, buf);

				r600_context_bo_reloc(&sctx->b, &sctx->b.rings.gfx,
						      rbuffer, buffers->shader_usage,
						      buffers->priority);

				buffers->desc.dirty_mask |= 1 << i;
				found = true;

				if (i >= SI_SO_BUF_OFFSET && shader == PIPE_SHADER_VERTEX) {
					/* Update the streamout state. */
					if (sctx->b.streamout.begin_emitted) {
						r600_emit_streamout_end(&sctx->b);
					}
					sctx->b.streamout.append_bitmask =
						sctx->b.streamout.enabled_mask;
					r600_streamout_buffers_dirty(&sctx->b);
				}
			}
		}
		if (found) {
			si_update_descriptors(sctx, &buffers->desc);
		}
	}

	/* Constant buffers. */
	for (shader = 0; shader < SI_NUM_SHADERS; shader++) {
		struct si_buffer_resources *buffers = &sctx->const_buffers[shader];
		bool found = false;
		uint32_t mask = buffers->desc.enabled_mask;

		while (mask) {
			unsigned i = u_bit_scan(&mask);
			if (buffers->buffers[i] == buf) {
				si_desc_reset_buffer_offset(ctx, buffers->desc_data[i],
							    old_va, buf);

				r600_context_bo_reloc(&sctx->b, &sctx->b.rings.gfx,
						      rbuffer, buffers->shader_usage,
						      buffers->priority);

				buffers->desc.dirty_mask |= 1 << i;
				found = true;
			}
		}
		if (found) {
			si_update_descriptors(sctx, &buffers->desc);
		}
	}

	/* Texture buffers - update virtual addresses in sampler view descriptors. */
	LIST_FOR_EACH_ENTRY(view, &sctx->b.texture_buffers, list) {
		if (view->base.texture == buf) {
			si_desc_reset_buffer_offset(ctx, view->state, old_va, buf);
		}
	}
	/* Texture buffers - update bindings. */
	for (shader = 0; shader < SI_NUM_SHADERS; shader++) {
		struct si_sampler_views *views = &sctx->samplers[shader].views;
		bool found = false;
		uint32_t mask = views->desc.enabled_mask;

		while (mask) {
			unsigned i = u_bit_scan(&mask);
			if (views->views[i]->texture == buf) {
				r600_context_bo_reloc(&sctx->b, &sctx->b.rings.gfx,
						      rbuffer, RADEON_USAGE_READ,
						      RADEON_PRIO_SHADER_BUFFER_RO);

				views->desc.dirty_mask |= 1 << i;
				found = true;
			}
		}
		if (found) {
			si_update_descriptors(sctx, &views->desc);
		}
	}
}

/* CP DMA */

/* The max number of bytes to copy per packet. */
#define CP_DMA_MAX_BYTE_COUNT ((1 << 21) - 8)

static void si_clear_buffer(struct pipe_context *ctx, struct pipe_resource *dst,
			    unsigned offset, unsigned size, unsigned value)
{
	struct si_context *sctx = (struct si_context*)ctx;

	if (!size)
		return;

	/* Mark the buffer range of destination as valid (initialized),
	 * so that transfer_map knows it should wait for the GPU when mapping
	 * that range. */
	util_range_add(&r600_resource(dst)->valid_buffer_range, offset,
		       offset + size);

	/* Fallback for unaligned clears. */
	if (offset % 4 != 0 || size % 4 != 0) {
		uint32_t *map = sctx->b.ws->buffer_map(r600_resource(dst)->cs_buf,
						       sctx->b.rings.gfx.cs,
						       PIPE_TRANSFER_WRITE);
		size /= 4;
		for (unsigned i = 0; i < size; i++)
			*map++ = value;
		return;
	}

	uint64_t va = r600_resource(dst)->gpu_address + offset;

	/* Flush the caches where the resource is bound. */
	/* XXX only flush the caches where the buffer is bound. */
	sctx->b.flags |= R600_CONTEXT_INV_TEX_CACHE |
			 R600_CONTEXT_INV_CONST_CACHE |
			 R600_CONTEXT_FLUSH_AND_INV_CB |
			 R600_CONTEXT_FLUSH_AND_INV_DB |
			 R600_CONTEXT_FLUSH_AND_INV_CB_META |
			 R600_CONTEXT_FLUSH_AND_INV_DB_META;
	sctx->b.flags |= R600_CONTEXT_WAIT_3D_IDLE;

	while (size) {
		unsigned byte_count = MIN2(size, CP_DMA_MAX_BYTE_COUNT);
		unsigned dma_flags = 0;

		si_need_cs_space(sctx, 7 + (sctx->b.flags ? sctx->cache_flush.num_dw : 0),
				 FALSE);

		/* This must be done after need_cs_space. */
		r600_context_bo_reloc(&sctx->b, &sctx->b.rings.gfx,
				      (struct r600_resource*)dst, RADEON_USAGE_WRITE,
				      RADEON_PRIO_MIN);

		/* Flush the caches for the first copy only.
		 * Also wait for the previous CP DMA operations. */
		if (sctx->b.flags) {
			si_emit_cache_flush(&sctx->b, NULL);
			dma_flags |= SI_CP_DMA_RAW_WAIT; /* same as WAIT_UNTIL=CP_DMA_IDLE */
		}

		/* Do the synchronization after the last copy, so that all data is written to memory. */
		if (size == byte_count)
			dma_flags |= R600_CP_DMA_SYNC;

		/* Emit the clear packet. */
		si_emit_cp_dma_clear_buffer(sctx, va, byte_count, value, dma_flags);

		size -= byte_count;
		va += byte_count;
	}

	/* Flush the caches again in case the 3D engine has been prefetching
	 * the resource. */
	/* XXX only flush the caches where the buffer is bound. */
	sctx->b.flags |= R600_CONTEXT_INV_TEX_CACHE |
			 R600_CONTEXT_INV_CONST_CACHE |
			 R600_CONTEXT_FLUSH_AND_INV_CB |
			 R600_CONTEXT_FLUSH_AND_INV_DB |
			 R600_CONTEXT_FLUSH_AND_INV_CB_META |
			 R600_CONTEXT_FLUSH_AND_INV_DB_META;
}

void si_copy_buffer(struct si_context *sctx,
		    struct pipe_resource *dst, struct pipe_resource *src,
		    uint64_t dst_offset, uint64_t src_offset, unsigned size)
{
	if (!size)
		return;

	/* Mark the buffer range of destination as valid (initialized),
	 * so that transfer_map knows it should wait for the GPU when mapping
	 * that range. */
	util_range_add(&r600_resource(dst)->valid_buffer_range, dst_offset,
		       dst_offset + size);

	dst_offset += r600_resource(dst)->gpu_address;
	src_offset += r600_resource(src)->gpu_address;

	/* Flush the caches where the resource is bound. */
	sctx->b.flags |= R600_CONTEXT_INV_TEX_CACHE |
			 R600_CONTEXT_INV_CONST_CACHE |
			 R600_CONTEXT_FLUSH_AND_INV_CB |
			 R600_CONTEXT_FLUSH_AND_INV_DB |
			 R600_CONTEXT_FLUSH_AND_INV_CB_META |
			 R600_CONTEXT_FLUSH_AND_INV_DB_META |
			 R600_CONTEXT_WAIT_3D_IDLE;

	while (size) {
		unsigned sync_flags = 0;
		unsigned byte_count = MIN2(size, CP_DMA_MAX_BYTE_COUNT);

		si_need_cs_space(sctx, 7 + (sctx->b.flags ? sctx->cache_flush.num_dw : 0), FALSE);

		/* Flush the caches for the first copy only. Also wait for old CP DMA packets to complete. */
		if (sctx->b.flags) {
			si_emit_cache_flush(&sctx->b, NULL);
			sync_flags |= SI_CP_DMA_RAW_WAIT;
		}

		/* Do the synchronization after the last copy, so that all data is written to memory. */
		if (size == byte_count) {
			sync_flags |= R600_CP_DMA_SYNC;
		}

		/* This must be done after r600_need_cs_space. */
		r600_context_bo_reloc(&sctx->b, &sctx->b.rings.gfx, (struct r600_resource*)src,
				      RADEON_USAGE_READ, RADEON_PRIO_MIN);
		r600_context_bo_reloc(&sctx->b, &sctx->b.rings.gfx, (struct r600_resource*)dst,
				      RADEON_USAGE_WRITE, RADEON_PRIO_MIN);

		si_emit_cp_dma_copy_buffer(sctx, dst_offset, src_offset, byte_count, sync_flags);

		size -= byte_count;
		src_offset += byte_count;
		dst_offset += byte_count;
	}

	sctx->b.flags |= R600_CONTEXT_INV_TEX_CACHE |
			 R600_CONTEXT_INV_CONST_CACHE |
			 R600_CONTEXT_FLUSH_AND_INV_CB |
			 R600_CONTEXT_FLUSH_AND_INV_DB |
			 R600_CONTEXT_FLUSH_AND_INV_CB_META |
			 R600_CONTEXT_FLUSH_AND_INV_DB_META;
}

/* INIT/DEINIT */

void si_init_all_descriptors(struct si_context *sctx)
{
	int i;

	for (i = 0; i < SI_NUM_SHADERS; i++) {
		si_init_buffer_resources(sctx, &sctx->const_buffers[i],
					 SI_NUM_CONST_BUFFERS, i, SI_SGPR_CONST,
					 RADEON_USAGE_READ, RADEON_PRIO_SHADER_BUFFER_RO);
		si_init_buffer_resources(sctx, &sctx->rw_buffers[i],
					 i == PIPE_SHADER_VERTEX ?
					 SI_NUM_RW_BUFFERS : SI_NUM_RING_BUFFERS,
					 i, SI_SGPR_RW_BUFFERS,
					 RADEON_USAGE_READWRITE, RADEON_PRIO_SHADER_RESOURCE_RW);

		si_init_sampler_views(sctx, &sctx->samplers[i].views, i);

		si_init_descriptors(sctx, &sctx->samplers[i].states.desc,
				    si_get_shader_user_data_base(i) + SI_SGPR_SAMPLER * 4,
				    4, SI_NUM_SAMPLER_STATES, si_emit_sampler_states);

		sctx->atoms.s.const_buffers[i] = &sctx->const_buffers[i].desc.atom;
		sctx->atoms.s.rw_buffers[i] = &sctx->rw_buffers[i].desc.atom;
		sctx->atoms.s.sampler_views[i] = &sctx->samplers[i].views.desc.atom;
		sctx->atoms.s.sampler_states[i] = &sctx->samplers[i].states.desc.atom;
	}

	si_init_descriptors(sctx, &sctx->vertex_buffers,
			    si_get_shader_user_data_base(PIPE_SHADER_VERTEX) +
			    SI_SGPR_VERTEX_BUFFER*4, 4, SI_NUM_VERTEX_BUFFERS,
			    si_emit_shader_pointer);
	sctx->atoms.s.vertex_buffers = &sctx->vertex_buffers.atom;

	/* Set pipe_context functions. */
	sctx->b.b.set_constant_buffer = si_set_constant_buffer;
	sctx->b.b.set_sampler_views = si_set_sampler_views;
	sctx->b.b.set_stream_output_targets = si_set_streamout_targets;
	sctx->b.clear_buffer = si_clear_buffer;
	sctx->b.invalidate_buffer = si_invalidate_buffer;
}

void si_release_all_descriptors(struct si_context *sctx)
{
	int i;

	for (i = 0; i < SI_NUM_SHADERS; i++) {
		si_release_buffer_resources(&sctx->const_buffers[i]);
		si_release_buffer_resources(&sctx->rw_buffers[i]);
		si_release_sampler_views(&sctx->samplers[i].views);
		si_release_descriptors(&sctx->samplers[i].states.desc);
	}
	si_release_descriptors(&sctx->vertex_buffers);
}

void si_all_descriptors_begin_new_cs(struct si_context *sctx)
{
	int i;

	for (i = 0; i < SI_NUM_SHADERS; i++) {
		si_buffer_resources_begin_new_cs(sctx, &sctx->const_buffers[i]);
		si_buffer_resources_begin_new_cs(sctx, &sctx->rw_buffers[i]);
		si_sampler_views_begin_new_cs(sctx, &sctx->samplers[i].views);
		si_sampler_states_begin_new_cs(sctx, &sctx->samplers[i].states);
	}
	si_vertex_buffers_begin_new_cs(sctx);
}

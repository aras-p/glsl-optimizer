/*
 * Copyright 2010 Red Hat Inc.
 *           2010 Jerome Glisse
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
 * Authors: Dave Airlie <airlied@redhat.com>
 *          Jerome Glisse <jglisse@redhat.com>
 */
#include <util/u_memory.h>
#include <util/u_format.h>
#include <pipebuffer/pb_buffer.h>
#include "pipe/p_shader_tokens.h"
#include "r600_pipe.h"
#include "r600d.h"

/* common state between evergreen and r600 */
void r600_bind_blend_state(struct pipe_context *ctx, void *state)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct r600_pipe_blend *blend = (struct r600_pipe_blend *)state;
	struct r600_pipe_state *rstate;

	if (state == NULL)
		return;
	rstate = &blend->rstate;
	rctx->states[rstate->id] = rstate;
	rctx->cb_target_mask = blend->cb_target_mask;
	r600_context_pipe_state_set(&rctx->ctx, rstate);
}

void r600_bind_rs_state(struct pipe_context *ctx, void *state)
{
	struct r600_pipe_rasterizer *rs = (struct r600_pipe_rasterizer *)state;
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;

	if (state == NULL)
		return;

	rctx->flatshade = rs->flatshade;
	rctx->sprite_coord_enable = rs->sprite_coord_enable;
	rctx->rasterizer = rs;

	rctx->states[rs->rstate.id] = &rs->rstate;
	r600_context_pipe_state_set(&rctx->ctx, &rs->rstate);

	if (rctx->family >= CHIP_CEDAR) {
		evergreen_polygon_offset_update(rctx);
	} else {
		r600_polygon_offset_update(rctx);
	}
}

void r600_delete_rs_state(struct pipe_context *ctx, void *state)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct r600_pipe_rasterizer *rs = (struct r600_pipe_rasterizer *)state;

	if (rctx->rasterizer == rs) {
		rctx->rasterizer = NULL;
	}
	if (rctx->states[rs->rstate.id] == &rs->rstate) {
		rctx->states[rs->rstate.id] = NULL;
	}
	free(rs);
}

void r600_sampler_view_destroy(struct pipe_context *ctx,
			       struct pipe_sampler_view *state)
{
	struct r600_pipe_sampler_view *resource = (struct r600_pipe_sampler_view *)state;

	pipe_resource_reference(&state->texture, NULL);
	FREE(resource);
}

void r600_bind_state(struct pipe_context *ctx, void *state)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct r600_pipe_state *rstate = (struct r600_pipe_state *)state;

	if (state == NULL)
		return;
	rctx->states[rstate->id] = rstate;
	r600_context_pipe_state_set(&rctx->ctx, rstate);
}

void r600_delete_state(struct pipe_context *ctx, void *state)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct r600_pipe_state *rstate = (struct r600_pipe_state *)state;

	if (rctx->states[rstate->id] == rstate) {
		rctx->states[rstate->id] = NULL;
	}
	for (int i = 0; i < rstate->nregs; i++) {
		r600_bo_reference(rctx->radeon, &rstate->regs[i].bo, NULL);
	}
	free(rstate);
}

void r600_bind_vertex_elements(struct pipe_context *ctx, void *state)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct r600_vertex_element *v = (struct r600_vertex_element*)state;

	rctx->vertex_elements = v;
	if (v) {
		rctx->states[v->rstate.id] = &v->rstate;
		r600_context_pipe_state_set(&rctx->ctx, &v->rstate);
	}
}

void r600_delete_vertex_element(struct pipe_context *ctx, void *state)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct r600_vertex_element *v = (struct r600_vertex_element*)state;

	if (rctx->states[v->rstate.id] == &v->rstate) {
		rctx->states[v->rstate.id] = NULL;
	}
	if (rctx->vertex_elements == state)
		rctx->vertex_elements = NULL;

	r600_bo_reference(rctx->radeon, &v->fetch_shader, NULL);
	FREE(state);
}


void r600_set_index_buffer(struct pipe_context *ctx,
			   const struct pipe_index_buffer *ib)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;

	if (ib) {
		pipe_resource_reference(&rctx->index_buffer.buffer, ib->buffer);
		memcpy(&rctx->index_buffer, ib, sizeof(rctx->index_buffer));
	} else {
		pipe_resource_reference(&rctx->index_buffer.buffer, NULL);
		memset(&rctx->index_buffer, 0, sizeof(rctx->index_buffer));
	}

	/* TODO make this more like a state */
}

void r600_set_vertex_buffers(struct pipe_context *ctx, unsigned count,
			     const struct pipe_vertex_buffer *buffers)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct pipe_vertex_buffer *vbo;
	unsigned max_index = ~0;
	int i;

	for (i = 0; i < count; i++) {
		vbo = (struct pipe_vertex_buffer*)&buffers[i];

		pipe_resource_reference(&rctx->vertex_buffer[i].buffer, vbo->buffer);
		pipe_resource_reference(&rctx->real_vertex_buffer[i], NULL);

		if (!vbo->buffer) {
			/* Zero states. */
			if (rctx->family >= CHIP_CEDAR) {
				evergreen_context_pipe_state_set_fs_resource(&rctx->ctx, NULL, i);
			} else {
				r600_context_pipe_state_set_fs_resource(&rctx->ctx, NULL, i);
			}
			continue;
		}

		if (r600_is_user_buffer(vbo->buffer)) {
			rctx->any_user_vbs = TRUE;
			continue;
		}

		pipe_resource_reference(&rctx->real_vertex_buffer[i], vbo->buffer);

		/* The stride of zero means we will be fetching only the first
		 * vertex, so don't care about max_index. */
		if (!vbo->stride) {
			continue;
		}

		/* Update the maximum index. */
		{
		    unsigned vbo_max_index =
			  (vbo->buffer->width0 - vbo->buffer_offset) / vbo->stride;
		    max_index = MIN2(max_index, vbo_max_index);
		}
	}

	for (; i < rctx->nreal_vertex_buffers; i++) {
		pipe_resource_reference(&rctx->vertex_buffer[i].buffer, NULL);
		pipe_resource_reference(&rctx->real_vertex_buffer[i], NULL);

		/* Zero states. */
		if (rctx->family >= CHIP_CEDAR) {
			evergreen_context_pipe_state_set_fs_resource(&rctx->ctx, NULL, i);
		} else {
			r600_context_pipe_state_set_fs_resource(&rctx->ctx, NULL, i);
		}
	}

	memcpy(rctx->vertex_buffer, buffers, sizeof(struct pipe_vertex_buffer) * count);

	rctx->nvertex_buffers = count;
	rctx->nreal_vertex_buffers = count;
	rctx->vb_max_index = max_index;
}


#define FORMAT_REPLACE(what, withwhat) \
	case PIPE_FORMAT_##what: *format = PIPE_FORMAT_##withwhat; break

void *r600_create_vertex_elements(struct pipe_context *ctx,
				  unsigned count,
				  const struct pipe_vertex_element *elements)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct r600_vertex_element *v = CALLOC_STRUCT(r600_vertex_element);
	enum pipe_format *format;
	int i;

	assert(count < 32);
	if (!v)
		return NULL;

	v->count = count;
	memcpy(v->elements, elements, count * sizeof(struct pipe_vertex_element));

	for (i = 0; i < count; i++) {
		v->hw_format[i] = v->elements[i].src_format;
		format = &v->hw_format[i];

		switch (*format) {
		FORMAT_REPLACE(R64_FLOAT,           R32_FLOAT);
		FORMAT_REPLACE(R64G64_FLOAT,        R32G32_FLOAT);
		FORMAT_REPLACE(R64G64B64_FLOAT,     R32G32B32_FLOAT);
		FORMAT_REPLACE(R64G64B64A64_FLOAT,  R32G32B32A32_FLOAT);

		/* r600 doesn't seem to support 32_*SCALED, these formats
		 * aren't in D3D10 either. */
		FORMAT_REPLACE(R32_UNORM,           R32_FLOAT);
		FORMAT_REPLACE(R32G32_UNORM,        R32G32_FLOAT);
		FORMAT_REPLACE(R32G32B32_UNORM,     R32G32B32_FLOAT);
		FORMAT_REPLACE(R32G32B32A32_UNORM,  R32G32B32A32_FLOAT);

		FORMAT_REPLACE(R32_USCALED,         R32_FLOAT);
		FORMAT_REPLACE(R32G32_USCALED,      R32G32_FLOAT);
		FORMAT_REPLACE(R32G32B32_USCALED,   R32G32B32_FLOAT);
		FORMAT_REPLACE(R32G32B32A32_USCALED,R32G32B32A32_FLOAT);

		FORMAT_REPLACE(R32_SNORM,           R32_FLOAT);
		FORMAT_REPLACE(R32G32_SNORM,        R32G32_FLOAT);
		FORMAT_REPLACE(R32G32B32_SNORM,     R32G32B32_FLOAT);
		FORMAT_REPLACE(R32G32B32A32_SNORM,  R32G32B32A32_FLOAT);

		FORMAT_REPLACE(R32_SSCALED,         R32_FLOAT);
		FORMAT_REPLACE(R32G32_SSCALED,      R32G32_FLOAT);
		FORMAT_REPLACE(R32G32B32_SSCALED,   R32G32B32_FLOAT);
		FORMAT_REPLACE(R32G32B32A32_SSCALED,R32G32B32A32_FLOAT);
		default:;
		}
		v->incompatible_layout =
			v->incompatible_layout ||
			v->elements[i].src_format != v->hw_format[i];

		v->hw_format_size[i] = align(util_format_get_blocksize(v->hw_format[i]), 4);
	}

	if (r600_vertex_elements_build_fetch_shader(rctx, v)) {
		FREE(v);
		return NULL;
	}

	return v;
}

void *r600_create_shader_state(struct pipe_context *ctx,
			       const struct pipe_shader_state *state)
{
	struct r600_pipe_shader *shader =  CALLOC_STRUCT(r600_pipe_shader);
	int r;

	r =  r600_pipe_shader_create(ctx, shader, state->tokens);
	if (r) {
		return NULL;
	}
	return shader;
}

void r600_bind_ps_shader(struct pipe_context *ctx, void *state)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;

	/* TODO delete old shader */
	rctx->ps_shader = (struct r600_pipe_shader *)state;
	if (state) {
		r600_context_pipe_state_set(&rctx->ctx, &rctx->ps_shader->rstate);
	}
}

void r600_bind_vs_shader(struct pipe_context *ctx, void *state)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;

	/* TODO delete old shader */
	rctx->vs_shader = (struct r600_pipe_shader *)state;
	if (state) {
		r600_context_pipe_state_set(&rctx->ctx, &rctx->vs_shader->rstate);
	}
}

void r600_delete_ps_shader(struct pipe_context *ctx, void *state)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct r600_pipe_shader *shader = (struct r600_pipe_shader *)state;

	if (rctx->ps_shader == shader) {
		rctx->ps_shader = NULL;
	}

	r600_pipe_shader_destroy(ctx, shader);
	free(shader);
}

void r600_delete_vs_shader(struct pipe_context *ctx, void *state)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct r600_pipe_shader *shader = (struct r600_pipe_shader *)state;

	if (rctx->vs_shader == shader) {
		rctx->vs_shader = NULL;
	}

	r600_pipe_shader_destroy(ctx, shader);
	free(shader);
}

/* FIXME optimize away spi update when it's not needed */
void r600_spi_update(struct r600_pipe_context *rctx)
{
	struct r600_pipe_shader *shader = rctx->ps_shader;
	struct r600_pipe_state rstate;
	struct r600_shader *rshader = &shader->shader;
	unsigned i, tmp;

	rstate.nregs = 0;
	for (i = 0; i < rshader->ninput; i++) {
		tmp = S_028644_SEMANTIC(r600_find_vs_semantic_index(&rctx->vs_shader->shader, rshader, i));

		if (rshader->input[i].name == TGSI_SEMANTIC_COLOR ||
		    rshader->input[i].name == TGSI_SEMANTIC_BCOLOR ||
		    rshader->input[i].name == TGSI_SEMANTIC_POSITION) {
			tmp |= S_028644_FLAT_SHADE(rctx->flatshade);
		}

		if (rshader->input[i].name == TGSI_SEMANTIC_GENERIC &&
		    rctx->sprite_coord_enable & (1 << rshader->input[i].sid)) {
			tmp |= S_028644_PT_SPRITE_TEX(1);
		}

                if (rctx->family < CHIP_CEDAR) {
                    if (rshader->input[i].centroid)
                            tmp |= S_028644_SEL_CENTROID(1);

                    if (rshader->input[i].interpolate == TGSI_INTERPOLATE_LINEAR)
                            tmp |= S_028644_SEL_LINEAR(1);
                }

		r600_pipe_state_add_reg(&rstate, R_028644_SPI_PS_INPUT_CNTL_0 + i * 4, tmp, 0xFFFFFFFF, NULL);
	}
	r600_context_pipe_state_set(&rctx->ctx, &rstate);
}

void r600_set_constant_buffer(struct pipe_context *ctx, uint shader, uint index,
			      struct pipe_resource *buffer)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct r600_resource_buffer *rbuffer = r600_buffer(buffer);
	uint32_t offset;

	/* Note that the state tracker can unbind constant buffers by
	 * passing NULL here.
	 */
	if (buffer == NULL) {
		return;
	}

	r600_upload_const_buffer(rctx, &rbuffer, &offset);

	switch (shader) {
	case PIPE_SHADER_VERTEX:
		rctx->vs_const_buffer.nregs = 0;
		r600_pipe_state_add_reg(&rctx->vs_const_buffer,
					R_028180_ALU_CONST_BUFFER_SIZE_VS_0,
					ALIGN_DIVUP(buffer->width0 >> 4, 16),
					0xFFFFFFFF, NULL);
		r600_pipe_state_add_reg(&rctx->vs_const_buffer,
					R_028980_ALU_CONST_CACHE_VS_0,
					(r600_bo_offset(rbuffer->r.bo) + offset) >> 8, 0xFFFFFFFF, rbuffer->r.bo);
		r600_context_pipe_state_set(&rctx->ctx, &rctx->vs_const_buffer);
		break;
	case PIPE_SHADER_FRAGMENT:
		rctx->ps_const_buffer.nregs = 0;
		r600_pipe_state_add_reg(&rctx->ps_const_buffer,
					R_028140_ALU_CONST_BUFFER_SIZE_PS_0,
					ALIGN_DIVUP(buffer->width0 >> 4, 16),
					0xFFFFFFFF, NULL);
		r600_pipe_state_add_reg(&rctx->ps_const_buffer,
					R_028940_ALU_CONST_CACHE_PS_0,
					(r600_bo_offset(rbuffer->r.bo) + offset) >> 8, 0xFFFFFFFF, rbuffer->r.bo);
		r600_context_pipe_state_set(&rctx->ctx, &rctx->ps_const_buffer);
		break;
	default:
		R600_ERR("unsupported %d\n", shader);
		return;
	}

	if (!rbuffer->user_buffer)
		pipe_resource_reference((struct pipe_resource**)&rbuffer, NULL);
}

static void r600_vertex_buffer_update(struct r600_pipe_context *rctx)
{
	struct r600_pipe_state *rstate;
	struct r600_resource *rbuffer;
	struct pipe_vertex_buffer *vertex_buffer;
	unsigned i, offset;

	if (rctx->vertex_elements->vbuffer_need_offset) {
		/* one resource per vertex elements */
		rctx->nvs_resource = rctx->vertex_elements->count;
	} else {
		/* bind vertex buffer once */
		rctx->nvs_resource = rctx->nreal_vertex_buffers;
	}

	for (i = 0 ; i < rctx->nvs_resource; i++) {
		rstate = &rctx->vs_resource[i];
		rstate->id = R600_PIPE_STATE_RESOURCE;
		rstate->nregs = 0;

		if (rctx->vertex_elements->vbuffer_need_offset) {
			/* one resource per vertex elements */
			unsigned vbuffer_index;
			vbuffer_index = rctx->vertex_elements->elements[i].vertex_buffer_index;
			vertex_buffer = &rctx->vertex_buffer[vbuffer_index];
			rbuffer = (struct r600_resource*)rctx->real_vertex_buffer[vbuffer_index];
			offset = rctx->vertex_elements->vbuffer_offset[i];
		} else {
			/* bind vertex buffer once */
			vertex_buffer = &rctx->vertex_buffer[i];
			rbuffer = (struct r600_resource*)rctx->real_vertex_buffer[i];
			offset = 0;
		}
		if (vertex_buffer == NULL || rbuffer == NULL)
			continue;
		offset += vertex_buffer->buffer_offset + r600_bo_offset(rbuffer->bo);

		if (rctx->family >= CHIP_CEDAR) {
			evergreen_pipe_add_vertex_attrib(rctx, rstate, i,
							 rbuffer, offset,
							 vertex_buffer->stride);
		} else {
			r600_pipe_add_vertex_attrib(rctx, rstate, i,
						    rbuffer, offset,
						    vertex_buffer->stride);
		}
	}
}

void r600_draw_vbo(struct pipe_context *ctx, const struct pipe_draw_info *info)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct r600_resource *rbuffer;
	u32 vgt_dma_index_type, vgt_draw_initiator, mask;
	struct r600_draw rdraw;
	struct r600_pipe_state vgt;
	struct r600_drawl draw = {};
	unsigned prim;

	if (rctx->vertex_elements->incompatible_layout) {
		r600_begin_vertex_translate(rctx);
	}

	if (rctx->any_user_vbs) {
		r600_upload_user_buffers(rctx, info->min_index, info->max_index);
	}

	r600_vertex_buffer_update(rctx);

	draw.info = *info;
	draw.ctx = ctx;
	if (info->indexed && rctx->index_buffer.buffer) {
		draw.info.start += rctx->index_buffer.offset / rctx->index_buffer.index_size;

		r600_translate_index_buffer(rctx, &rctx->index_buffer.buffer,
					    &rctx->index_buffer.index_size,
					    &draw.info.start,
					    info->count);

		draw.index_size = rctx->index_buffer.index_size;
		pipe_resource_reference(&draw.index_buffer, rctx->index_buffer.buffer);
		draw.index_buffer_offset = draw.info.start * draw.index_size;
		draw.info.start = 0;

		if (r600_is_user_buffer(draw.index_buffer)) {
			r600_upload_index_buffer(rctx, &draw);
		}
	} else {
		draw.info.index_bias = info->start;
	}

	switch (draw.index_size) {
	case 2:
		vgt_draw_initiator = 0;
		vgt_dma_index_type = 0;
		break;
	case 4:
		vgt_draw_initiator = 0;
		vgt_dma_index_type = 1;
		break;
	case 0:
		vgt_draw_initiator = 2;
		vgt_dma_index_type = 0;
		break;
	default:
		R600_ERR("unsupported index size %d\n", draw.index_size);
		return;
	}
	if (r600_conv_pipe_prim(draw.info.mode, &prim))
		return;
	if (unlikely(rctx->ps_shader == NULL)) {
		R600_ERR("missing vertex shader\n");
		return;
	}
	if (unlikely(rctx->vs_shader == NULL)) {
		R600_ERR("missing vertex shader\n");
		return;
	}
	/* there should be enough input */
	if (rctx->vertex_elements->count < rctx->vs_shader->shader.bc.nresource) {
		R600_ERR("%d resources provided, expecting %d\n",
			rctx->vertex_elements->count, rctx->vs_shader->shader.bc.nresource);
		return;
	}

	r600_spi_update(rctx);

	mask = 0;
	for (int i = 0; i < rctx->framebuffer.nr_cbufs; i++) {
		mask |= (0xF << (i * 4));
	}

	vgt.id = R600_PIPE_STATE_VGT;
	vgt.nregs = 0;
	r600_pipe_state_add_reg(&vgt, R_008958_VGT_PRIMITIVE_TYPE, prim, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(&vgt, R_028408_VGT_INDX_OFFSET, draw.info.index_bias, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(&vgt, R_028400_VGT_MAX_VTX_INDX, draw.info.max_index, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(&vgt, R_028404_VGT_MIN_VTX_INDX, draw.info.min_index, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(&vgt, R_028238_CB_TARGET_MASK, rctx->cb_target_mask & mask, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(&vgt, R_03CFF0_SQ_VTX_BASE_VTX_LOC, 0, 0xFFFFFFFF, NULL);
	r600_pipe_state_add_reg(&vgt, R_03CFF4_SQ_VTX_START_INST_LOC, 0, 0xFFFFFFFF, NULL);
	r600_context_pipe_state_set(&rctx->ctx, &vgt);

	rdraw.vgt_num_indices = draw.info.count;
	rdraw.vgt_num_instances = 1;
	rdraw.vgt_index_type = vgt_dma_index_type;
	rdraw.vgt_draw_initiator = vgt_draw_initiator;
	rdraw.indices = NULL;
	if (draw.index_buffer) {
		rbuffer = (struct r600_resource*)draw.index_buffer;
		rdraw.indices = rbuffer->bo;
		rdraw.indices_bo_offset = draw.index_buffer_offset;
	}

	if (rctx->family >= CHIP_CEDAR) {
		evergreen_context_draw(&rctx->ctx, &rdraw);
	} else {
		r600_context_draw(&rctx->ctx, &rdraw);
	}

	pipe_resource_reference(&draw.index_buffer, NULL);

	/* delete previous translated vertex elements */
	if (rctx->tran.new_velems) {
		r600_end_vertex_translate(rctx);
	}
}

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
#include "r600_formats.h"
#include "r600_pipe.h"
#include "r600d.h"

static int r600_conv_pipe_prim(unsigned pprim, unsigned *prim)
{
	static const int prim_conv[] = {
		V_008958_DI_PT_POINTLIST,
		V_008958_DI_PT_LINELIST,
		V_008958_DI_PT_LINELOOP,
		V_008958_DI_PT_LINESTRIP,
		V_008958_DI_PT_TRILIST,
		V_008958_DI_PT_TRISTRIP,
		V_008958_DI_PT_TRIFAN,
		V_008958_DI_PT_QUADLIST,
		V_008958_DI_PT_QUADSTRIP,
		V_008958_DI_PT_POLYGON,
		-1,
		-1,
		-1,
		-1
	};

	*prim = prim_conv[pprim];
	if (*prim == -1) {
		fprintf(stderr, "%s:%d unsupported %d\n", __func__, __LINE__, pprim);
		return -1;
	}
	return 0;
}

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

void r600_bind_dsa_state(struct pipe_context *ctx, void *state)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct r600_pipe_dsa *dsa = state;
	struct r600_pipe_state *rstate;

	if (state == NULL)
		return;
	rstate = &dsa->rstate;
	rctx->states[rstate->id] = rstate;
	rctx->alpha_ref = dsa->alpha_ref;
	rctx->alpha_ref_dirty = true;
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
		u_vbuf_mgr_bind_vertex_elements(rctx->vbuf_mgr, state,
						v->vmgr_elements);

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
	u_vbuf_mgr_destroy_vertex_elements(rctx->vbuf_mgr, v->vmgr_elements);
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
	int i;

	/* Zero states. */
	for (i = 0; i < count; i++) {
		if (!buffers[i].buffer) {
			if (rctx->family >= CHIP_CEDAR) {
				evergreen_context_pipe_state_set_fs_resource(&rctx->ctx, NULL, i);
			} else {
				r600_context_pipe_state_set_fs_resource(&rctx->ctx, NULL, i);
			}
		}
	}
	for (; i < rctx->vbuf_mgr->nr_real_vertex_buffers; i++) {
		if (rctx->family >= CHIP_CEDAR) {
			evergreen_context_pipe_state_set_fs_resource(&rctx->ctx, NULL, i);
		} else {
			r600_context_pipe_state_set_fs_resource(&rctx->ctx, NULL, i);
		}
	}

	u_vbuf_mgr_set_vertex_buffers(rctx->vbuf_mgr, count, buffers);
}

void *r600_create_vertex_elements(struct pipe_context *ctx,
				  unsigned count,
				  const struct pipe_vertex_element *elements)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct r600_vertex_element *v = CALLOC_STRUCT(r600_vertex_element);

	assert(count < 32);
	if (!v)
		return NULL;

	v->count = count;
	v->vmgr_elements =
		u_vbuf_mgr_create_vertex_elements(rctx->vbuf_mgr, count,
						  elements, v->elements);

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

static void r600_update_alpha_ref(struct r600_pipe_context *rctx)
{
	unsigned alpha_ref = rctx->alpha_ref;
	struct r600_pipe_state rstate;

	if (!rctx->alpha_ref_dirty)
		return;

	rstate.nregs = 0;
	if (rctx->export_16bpc)
		alpha_ref &= ~0x1FFF;
	r600_pipe_state_add_reg(&rstate, R_028438_SX_ALPHA_REF, alpha_ref, 0xFFFFFFFF, NULL);

	r600_context_pipe_state_set(&rctx->ctx, &rstate);
	rctx->alpha_ref_dirty = false;
}

/* FIXME optimize away spi update when it's not needed */
static void r600_spi_block_init(struct r600_pipe_context *rctx, struct r600_pipe_state *rstate)
{
	int i;
	rstate->nregs = 0;
	rstate->id = R600_PIPE_STATE_SPI;
	for (i = 0; i < 32; i++) {
		r600_pipe_state_add_reg(rstate, R_028644_SPI_PS_INPUT_CNTL_0 + i * 4, 0, 0xFFFFFFFF, NULL);
	}
}

static void r600_spi_update(struct r600_pipe_context *rctx, unsigned prim)
{
	struct r600_pipe_shader *shader = rctx->ps_shader;
	struct r600_pipe_state *rstate = &rctx->spi;
	struct r600_shader *rshader = &shader->shader;
	unsigned i, tmp;

	if (rctx->spi.id == 0)
		r600_spi_block_init(rctx, &rctx->spi);

	rstate->nregs = 0;
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

		r600_pipe_state_mod_reg(rstate, tmp);
	}

	r600_context_pipe_state_set(&rctx->ctx, rstate);
}

void r600_set_constant_buffer(struct pipe_context *ctx, uint shader, uint index,
			      struct pipe_resource *buffer)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct r600_resource_buffer *rbuffer = r600_buffer(buffer);
	struct r600_pipe_state *rstate;
	uint32_t offset;

	/* Note that the state tracker can unbind constant buffers by
	 * passing NULL here.
	 */
	if (buffer == NULL) {
		return;
	}

	r600_upload_const_buffer(rctx, &rbuffer, &offset);
	offset += r600_bo_offset(rbuffer->r.bo);

	switch (shader) {
	case PIPE_SHADER_VERTEX:
		rctx->vs_const_buffer.nregs = 0;
		r600_pipe_state_add_reg(&rctx->vs_const_buffer,
					R_028180_ALU_CONST_BUFFER_SIZE_VS_0,
					ALIGN_DIVUP(buffer->width0 >> 4, 16),
					0xFFFFFFFF, NULL);
		r600_pipe_state_add_reg(&rctx->vs_const_buffer,
					R_028980_ALU_CONST_CACHE_VS_0,
					offset >> 8, 0xFFFFFFFF, rbuffer->r.bo);
		r600_context_pipe_state_set(&rctx->ctx, &rctx->vs_const_buffer);

		rstate = &rctx->vs_const_buffer_resource[index];
		if (!rstate->id) {
			if (rctx->family >= CHIP_CEDAR) {
				evergreen_pipe_init_buffer_resource(rctx, rstate, &rbuffer->r, offset, 16);
			} else {
				r600_pipe_init_buffer_resource(rctx, rstate, &rbuffer->r, offset, 16);
			}
		}

		if (rctx->family >= CHIP_CEDAR) {
			evergreen_pipe_mod_buffer_resource(rstate, &rbuffer->r, offset, 16);
			evergreen_context_pipe_state_set_vs_resource(&rctx->ctx, rstate, index);
		} else {
			r600_pipe_mod_buffer_resource(rstate, &rbuffer->r, offset, 16);
			r600_context_pipe_state_set_vs_resource(&rctx->ctx, rstate, index);
		}
		break;
	case PIPE_SHADER_FRAGMENT:
		rctx->ps_const_buffer.nregs = 0;
		r600_pipe_state_add_reg(&rctx->ps_const_buffer,
					R_028140_ALU_CONST_BUFFER_SIZE_PS_0,
					ALIGN_DIVUP(buffer->width0 >> 4, 16),
					0xFFFFFFFF, NULL);
		r600_pipe_state_add_reg(&rctx->ps_const_buffer,
					R_028940_ALU_CONST_CACHE_PS_0,
					offset >> 8, 0xFFFFFFFF, rbuffer->r.bo);
		r600_context_pipe_state_set(&rctx->ctx, &rctx->ps_const_buffer);

		rstate = &rctx->ps_const_buffer_resource[index];
		if (!rstate->id) {
			if (rctx->family >= CHIP_CEDAR) {
				evergreen_pipe_init_buffer_resource(rctx, rstate, &rbuffer->r, offset, 16);
			} else {
				r600_pipe_init_buffer_resource(rctx, rstate, &rbuffer->r, offset, 16);
			}
		}
		if (rctx->family >= CHIP_CEDAR) {
			evergreen_pipe_mod_buffer_resource(rstate, &rbuffer->r, offset, 16);
			evergreen_context_pipe_state_set_ps_resource(&rctx->ctx, rstate, index);
		} else {
			r600_pipe_mod_buffer_resource(rstate, &rbuffer->r, offset, 16);
			r600_context_pipe_state_set_ps_resource(&rctx->ctx, rstate, index);
		}
		break;
	default:
		R600_ERR("unsupported %d\n", shader);
		return;
	}

	if (buffer != &rbuffer->r.b.b.b)
		pipe_resource_reference((struct pipe_resource**)&rbuffer, NULL);
}

static void r600_vertex_buffer_update(struct r600_pipe_context *rctx)
{
	struct r600_pipe_state *rstate;
	struct r600_resource *rbuffer;
	struct pipe_vertex_buffer *vertex_buffer;
	unsigned i, count, offset;

	if (rctx->vertex_elements->vbuffer_need_offset) {
		/* one resource per vertex elements */
		count = rctx->vertex_elements->count;
	} else {
		/* bind vertex buffer once */
		count = rctx->vbuf_mgr->nr_real_vertex_buffers;
	}

	for (i = 0 ; i < count; i++) {
		rstate = &rctx->fs_resource[i];

		if (rctx->vertex_elements->vbuffer_need_offset) {
			/* one resource per vertex elements */
			unsigned vbuffer_index;
			vbuffer_index = rctx->vertex_elements->elements[i].vertex_buffer_index;
			vertex_buffer = &rctx->vbuf_mgr->vertex_buffer[vbuffer_index];
			rbuffer = (struct r600_resource*)rctx->vbuf_mgr->real_vertex_buffer[vbuffer_index];
			offset = rctx->vertex_elements->vbuffer_offset[i];
		} else {
			/* bind vertex buffer once */
			vertex_buffer = &rctx->vbuf_mgr->vertex_buffer[i];
			rbuffer = (struct r600_resource*)rctx->vbuf_mgr->real_vertex_buffer[i];
			offset = 0;
		}
		if (vertex_buffer == NULL || rbuffer == NULL)
			continue;
		offset += vertex_buffer->buffer_offset + r600_bo_offset(rbuffer->bo);

		if (!rstate->id) {
			if (rctx->family >= CHIP_CEDAR) {
				evergreen_pipe_init_buffer_resource(rctx, rstate, rbuffer, offset, vertex_buffer->stride);
			} else {
				r600_pipe_init_buffer_resource(rctx, rstate, rbuffer, offset, vertex_buffer->stride);
			}
		}

		if (rctx->family >= CHIP_CEDAR) {
			evergreen_pipe_mod_buffer_resource(rstate, rbuffer, offset, vertex_buffer->stride);
			evergreen_context_pipe_state_set_fs_resource(&rctx->ctx, rstate, i);
		} else {
			r600_pipe_mod_buffer_resource(rstate, rbuffer, offset, vertex_buffer->stride);
			r600_context_pipe_state_set_fs_resource(&rctx->ctx, rstate, i);
		}
	}
}

void r600_draw_vbo(struct pipe_context *ctx, const struct pipe_draw_info *info)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct r600_resource *rbuffer;
	u32 vgt_dma_index_type, vgt_dma_swap_mode, vgt_draw_initiator, mask;
	struct r600_draw rdraw;
	struct r600_drawl draw = {};
	unsigned prim;

	r600_flush_depth_textures(rctx);
	u_vbuf_mgr_draw_begin(rctx->vbuf_mgr, info, NULL, NULL);
	r600_vertex_buffer_update(rctx);

	draw.info = *info;
	draw.ctx = ctx;
	if (info->indexed && rctx->index_buffer.buffer) {
		draw.info.start += rctx->index_buffer.offset / rctx->index_buffer.index_size;
		pipe_resource_reference(&draw.index_buffer, rctx->index_buffer.buffer);

		r600_translate_index_buffer(rctx, &draw.index_buffer,
					    &rctx->index_buffer.index_size,
					    &draw.info.start,
					    info->count);

		draw.index_size = rctx->index_buffer.index_size;
		draw.index_buffer_offset = draw.info.start * draw.index_size;
		draw.info.start = 0;

		if (u_vbuf_resource(draw.index_buffer)->user_ptr) {
			r600_upload_index_buffer(rctx, &draw);
		}
	} else {
		draw.info.index_bias = info->start;
	}

	vgt_dma_swap_mode = 0;
	switch (draw.index_size) {
	case 2:
		vgt_draw_initiator = 0;
		vgt_dma_index_type = 0;
		if (R600_BIG_ENDIAN) {
			vgt_dma_swap_mode = ENDIAN_8IN16;
		}
		break;
	case 4:
		vgt_draw_initiator = 0;
		vgt_dma_index_type = 1;
		if (R600_BIG_ENDIAN) {
			vgt_dma_swap_mode = ENDIAN_8IN32;
		}
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

	r600_update_alpha_ref(rctx);
	r600_spi_update(rctx, draw.info.mode);

	mask = 0;
	for (int i = 0; i < rctx->framebuffer.nr_cbufs; i++) {
		mask |= (0xF << (i * 4));
	}

	if (rctx->vgt.id != R600_PIPE_STATE_VGT) {
		rctx->vgt.id = R600_PIPE_STATE_VGT;
		rctx->vgt.nregs = 0;
		r600_pipe_state_add_reg(&rctx->vgt, R_008958_VGT_PRIMITIVE_TYPE, prim, 0xFFFFFFFF, NULL);
		r600_pipe_state_add_reg(&rctx->vgt, R_028238_CB_TARGET_MASK, rctx->cb_target_mask & mask, 0xFFFFFFFF, NULL);
		r600_pipe_state_add_reg(&rctx->vgt, R_028400_VGT_MAX_VTX_INDX, draw.info.max_index, 0xFFFFFFFF, NULL);
		r600_pipe_state_add_reg(&rctx->vgt, R_028404_VGT_MIN_VTX_INDX, draw.info.min_index, 0xFFFFFFFF, NULL);
		r600_pipe_state_add_reg(&rctx->vgt, R_028408_VGT_INDX_OFFSET, draw.info.index_bias, 0xFFFFFFFF, NULL);
		r600_pipe_state_add_reg(&rctx->vgt, R_03CFF0_SQ_VTX_BASE_VTX_LOC, 0, 0xFFFFFFFF, NULL);
		r600_pipe_state_add_reg(&rctx->vgt, R_03CFF4_SQ_VTX_START_INST_LOC, draw.info.start_instance, 0xFFFFFFFF, NULL);
		r600_pipe_state_add_reg(&rctx->vgt, R_028814_PA_SU_SC_MODE_CNTL,
					0,
					S_028814_PROVOKING_VTX_LAST(1), NULL);

	}

	rctx->vgt.nregs = 0;
	r600_pipe_state_mod_reg(&rctx->vgt, prim);
	r600_pipe_state_mod_reg(&rctx->vgt, rctx->cb_target_mask & mask);
	r600_pipe_state_mod_reg(&rctx->vgt, draw.info.max_index);
	r600_pipe_state_mod_reg(&rctx->vgt, draw.info.min_index);
	r600_pipe_state_mod_reg(&rctx->vgt, draw.info.index_bias);
	r600_pipe_state_mod_reg(&rctx->vgt, 0);
	r600_pipe_state_mod_reg(&rctx->vgt, draw.info.start_instance);
	if (draw.info.mode == PIPE_PRIM_QUADS || draw.info.mode == PIPE_PRIM_QUAD_STRIP || draw.info.mode == PIPE_PRIM_POLYGON) {
		r600_pipe_state_mod_reg(&rctx->vgt, S_028814_PROVOKING_VTX_LAST(1));
	}

	r600_context_pipe_state_set(&rctx->ctx, &rctx->vgt);

	rdraw.vgt_num_indices = draw.info.count;
	rdraw.vgt_num_instances = draw.info.instance_count;
	rdraw.vgt_index_type = vgt_dma_index_type | (vgt_dma_swap_mode << 2);
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

	if (rctx->framebuffer.zsbuf)
	{
		struct pipe_resource *tex = rctx->framebuffer.zsbuf->texture;
		((struct r600_resource_texture *)tex)->dirty_db = TRUE;
	}

	pipe_resource_reference(&draw.index_buffer, NULL);

	u_vbuf_mgr_draw_end(rctx->vbuf_mgr);
}

void _r600_pipe_state_add_reg(struct r600_context *ctx,
			      struct r600_pipe_state *state,
			      u32 offset, u32 value, u32 mask,
			      u32 range_id, u32 block_id,
			      struct r600_bo *bo)
{
	struct r600_range *range;
	struct r600_block *block;

	range = &ctx->range[range_id];
	block = range->blocks[block_id];
	state->regs[state->nregs].block = block;
	state->regs[state->nregs].id = (offset - block->start_offset) >> 2;

	state->regs[state->nregs].value = value;
	state->regs[state->nregs].mask = mask;
	state->regs[state->nregs].bo = bo;

	state->nregs++;
	assert(state->nregs < R600_BLOCK_MAX_REG);
}

void r600_pipe_state_add_reg_noblock(struct r600_pipe_state *state,
				     u32 offset, u32 value, u32 mask,
				     struct r600_bo *bo)
{
	state->regs[state->nregs].id = offset;
	state->regs[state->nregs].block = NULL;
	state->regs[state->nregs].value = value;
	state->regs[state->nregs].mask = mask;
	state->regs[state->nregs].bo = bo;

	state->nregs++;
	assert(state->nregs < R600_BLOCK_MAX_REG);
}

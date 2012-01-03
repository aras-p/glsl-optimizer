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
#include "util/u_blitter.h"
#include "util/u_memory.h"
#include "util/u_format.h"
#include "pipebuffer/pb_buffer.h"
#include "pipe/p_shader_tokens.h"
#include "tgsi/tgsi_parse.h"
#include "r600_formats.h"
#include "r600_pipe.h"
#include "r600d.h"

static bool r600_conv_pipe_prim(unsigned pprim, unsigned *prim)
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
		return false;
	}
	return true;
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

	rctx->clamp_vertex_color = rs->clamp_vertex_color;
	rctx->clamp_fragment_color = rs->clamp_fragment_color;

	rctx->sprite_coord_enable = rs->sprite_coord_enable;

	rctx->rasterizer = rs;

	rctx->states[rs->rstate.id] = &rs->rstate;
	r600_context_pipe_state_set(&rctx->ctx, &rs->rstate);

	if (rctx->chip_class >= EVERGREEN) {
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
		pipe_resource_reference((struct pipe_resource**)&rstate->regs[i].bo, NULL);
	}
	free(rstate);
}

void r600_bind_vertex_elements(struct pipe_context *ctx, void *state)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct r600_vertex_element *v = (struct r600_vertex_element*)state;

	rctx->vertex_elements = v;
	if (v) {
		u_vbuf_bind_vertex_elements(rctx->vbuf_mgr, state,
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

	pipe_resource_reference((struct pipe_resource**)&v->fetch_shader, NULL);
	u_vbuf_destroy_vertex_elements(rctx->vbuf_mgr, v->vmgr_elements);
	FREE(state);
}


void r600_set_index_buffer(struct pipe_context *ctx,
			   const struct pipe_index_buffer *ib)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;

	u_vbuf_set_index_buffer(rctx->vbuf_mgr, ib);
}

void r600_set_vertex_buffers(struct pipe_context *ctx, unsigned count,
			     const struct pipe_vertex_buffer *buffers)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	int i;

	/* Zero states. */
	for (i = 0; i < count; i++) {
		if (!buffers[i].buffer) {
			if (rctx->chip_class >= EVERGREEN) {
				evergreen_context_pipe_state_set_fs_resource(&rctx->ctx, NULL, i);
			} else {
				r600_context_pipe_state_set_fs_resource(&rctx->ctx, NULL, i);
			}
		}
	}
	for (; i < rctx->vbuf_mgr->nr_real_vertex_buffers; i++) {
		if (rctx->chip_class >= EVERGREEN) {
			evergreen_context_pipe_state_set_fs_resource(&rctx->ctx, NULL, i);
		} else {
			r600_context_pipe_state_set_fs_resource(&rctx->ctx, NULL, i);
		}
	}

	u_vbuf_set_vertex_buffers(rctx->vbuf_mgr, count, buffers);
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
		u_vbuf_create_vertex_elements(rctx->vbuf_mgr, count,
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
	struct r600_pipe_shader *shader = CALLOC_STRUCT(r600_pipe_shader);
	int r;

	shader->tokens = tgsi_dup_tokens(state->tokens);
	shader->so = state->stream_output;

	r =  r600_pipe_shader_create(ctx, shader);
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
	if (rctx->ps_shader && rctx->vs_shader) {
		r600_adjust_gprs(rctx);
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
	if (rctx->ps_shader && rctx->vs_shader) {
		r600_adjust_gprs(rctx);
	}
}

void r600_delete_ps_shader(struct pipe_context *ctx, void *state)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct r600_pipe_shader *shader = (struct r600_pipe_shader *)state;

	if (rctx->ps_shader == shader) {
		rctx->ps_shader = NULL;
	}

	free(shader->tokens);
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

	free(shader->tokens);
	r600_pipe_shader_destroy(ctx, shader);
	free(shader);
}

static void r600_update_alpha_ref(struct r600_pipe_context *rctx)
{
	unsigned alpha_ref;
	struct r600_pipe_state rstate;

	alpha_ref = rctx->alpha_ref;
	rstate.nregs = 0;
	if (rctx->export_16bpc)
		alpha_ref &= ~0x1FFF;
	r600_pipe_state_add_reg(&rstate, R_028438_SX_ALPHA_REF, alpha_ref, 0xFFFFFFFF, NULL, 0);

	r600_context_pipe_state_set(&rctx->ctx, &rstate);
	rctx->alpha_ref_dirty = false;
}

void r600_set_constant_buffer(struct pipe_context *ctx, uint shader, uint index,
			      struct pipe_resource *buffer)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct r600_resource *rbuffer = r600_resource(buffer);
	struct r600_pipe_resource_state *rstate;
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
					0xFFFFFFFF, NULL, 0);
		r600_pipe_state_add_reg(&rctx->vs_const_buffer,
					R_028980_ALU_CONST_CACHE_VS_0,
					offset >> 8, 0xFFFFFFFF, rbuffer, RADEON_USAGE_READ);
		r600_context_pipe_state_set(&rctx->ctx, &rctx->vs_const_buffer);

		rstate = &rctx->vs_const_buffer_resource[index];
		if (!rstate->id) {
			if (rctx->chip_class >= EVERGREEN) {
				evergreen_pipe_init_buffer_resource(rctx, rstate);
			} else {
				r600_pipe_init_buffer_resource(rctx, rstate);
			}
		}

		if (rctx->chip_class >= EVERGREEN) {
			evergreen_pipe_mod_buffer_resource(rstate, rbuffer, offset, 16, RADEON_USAGE_READ);
			evergreen_context_pipe_state_set_vs_resource(&rctx->ctx, rstate, index);
		} else {
			r600_pipe_mod_buffer_resource(rstate, rbuffer, offset, 16, RADEON_USAGE_READ);
			r600_context_pipe_state_set_vs_resource(&rctx->ctx, rstate, index);
		}
		break;
	case PIPE_SHADER_FRAGMENT:
		rctx->ps_const_buffer.nregs = 0;
		r600_pipe_state_add_reg(&rctx->ps_const_buffer,
					R_028140_ALU_CONST_BUFFER_SIZE_PS_0,
					ALIGN_DIVUP(buffer->width0 >> 4, 16),
					0xFFFFFFFF, NULL, 0);
		r600_pipe_state_add_reg(&rctx->ps_const_buffer,
					R_028940_ALU_CONST_CACHE_PS_0,
					offset >> 8, 0xFFFFFFFF, rbuffer, RADEON_USAGE_READ);
		r600_context_pipe_state_set(&rctx->ctx, &rctx->ps_const_buffer);

		rstate = &rctx->ps_const_buffer_resource[index];
		if (!rstate->id) {
			if (rctx->chip_class >= EVERGREEN) {
				evergreen_pipe_init_buffer_resource(rctx, rstate);
			} else {
				r600_pipe_init_buffer_resource(rctx, rstate);
			}
		}
		if (rctx->chip_class >= EVERGREEN) {
			evergreen_pipe_mod_buffer_resource(rstate, rbuffer, offset, 16, RADEON_USAGE_READ);
			evergreen_context_pipe_state_set_ps_resource(&rctx->ctx, rstate, index);
		} else {
			r600_pipe_mod_buffer_resource(rstate, rbuffer, offset, 16, RADEON_USAGE_READ);
			r600_context_pipe_state_set_ps_resource(&rctx->ctx, rstate, index);
		}
		break;
	default:
		R600_ERR("unsupported %d\n", shader);
		return;
	}

	if (buffer != &rbuffer->b.b.b)
		pipe_resource_reference((struct pipe_resource**)&rbuffer, NULL);
}

struct pipe_stream_output_target *
r600_create_so_target(struct pipe_context *ctx,
		      struct pipe_resource *buffer,
		      unsigned buffer_offset,
		      unsigned buffer_size)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct r600_so_target *t;
	void *ptr;

	t = CALLOC_STRUCT(r600_so_target);
	if (!t) {
		return NULL;
	}

	t->b.reference.count = 1;
	t->b.context = ctx;
	pipe_resource_reference(&t->b.buffer, buffer);
	t->b.buffer_offset = buffer_offset;
	t->b.buffer_size = buffer_size;

	t->filled_size = (struct r600_resource*)
		pipe_buffer_create(ctx->screen, PIPE_BIND_CUSTOM, PIPE_USAGE_STATIC, 4);
	ptr = rctx->ws->buffer_map(t->filled_size->buf, rctx->ctx.cs, PIPE_TRANSFER_WRITE);
	memset(ptr, 0, t->filled_size->buf->size);
	rctx->ws->buffer_unmap(t->filled_size->buf);

	return &t->b;
}

void r600_so_target_destroy(struct pipe_context *ctx,
			    struct pipe_stream_output_target *target)
{
	struct r600_so_target *t = (struct r600_so_target*)target;
	pipe_resource_reference(&t->b.buffer, NULL);
	pipe_resource_reference((struct pipe_resource**)&t->filled_size, NULL);
	FREE(t);
}

void r600_set_so_targets(struct pipe_context *ctx,
			 unsigned num_targets,
			 struct pipe_stream_output_target **targets,
			 unsigned append_bitmask)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	unsigned i;

	/* Stop streamout. */
	if (rctx->ctx.num_so_targets) {
		r600_context_streamout_end(&rctx->ctx);
	}

	/* Set the new targets. */
	for (i = 0; i < num_targets; i++) {
		pipe_so_target_reference((struct pipe_stream_output_target**)&rctx->ctx.so_targets[i], targets[i]);
	}
	for (; i < rctx->ctx.num_so_targets; i++) {
		pipe_so_target_reference((struct pipe_stream_output_target**)&rctx->ctx.so_targets[i], NULL);
	}

	rctx->ctx.num_so_targets = num_targets;
	rctx->ctx.streamout_start = num_targets != 0;
	rctx->ctx.streamout_append_bitmask = append_bitmask;
}

static void r600_vertex_buffer_update(struct r600_pipe_context *rctx)
{
	struct r600_pipe_resource_state *rstate;
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
			vertex_buffer = &rctx->vbuf_mgr->real_vertex_buffer[vbuffer_index];
			rbuffer = (struct r600_resource*)vertex_buffer->buffer;
			offset = rctx->vertex_elements->vbuffer_offset[i];
		} else {
			/* bind vertex buffer once */
			vertex_buffer = &rctx->vbuf_mgr->real_vertex_buffer[i];
			rbuffer = (struct r600_resource*)vertex_buffer->buffer;
			offset = 0;
		}
		if (vertex_buffer == NULL || rbuffer == NULL)
			continue;
		offset += vertex_buffer->buffer_offset;

		if (!rstate->id) {
			if (rctx->chip_class >= EVERGREEN) {
				evergreen_pipe_init_buffer_resource(rctx, rstate);
			} else {
				r600_pipe_init_buffer_resource(rctx, rstate);
			}
		}

		if (rctx->chip_class >= EVERGREEN) {
			evergreen_pipe_mod_buffer_resource(rstate, rbuffer, offset, vertex_buffer->stride, RADEON_USAGE_READ);
			evergreen_context_pipe_state_set_fs_resource(&rctx->ctx, rstate, i);
		} else {
			r600_pipe_mod_buffer_resource(rstate, rbuffer, offset, vertex_buffer->stride, RADEON_USAGE_READ);
			r600_context_pipe_state_set_fs_resource(&rctx->ctx, rstate, i);
		}
	}
}

static int r600_shader_rebuild(struct pipe_context * ctx, struct r600_pipe_shader * shader)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	int r;

	r600_pipe_shader_destroy(ctx, shader);
	r = r600_pipe_shader_create(ctx, shader);
	if (r) {
		return r;
	}
	r600_context_pipe_state_set(&rctx->ctx, &shader->rstate);

	return 0;
}

static void r600_update_derived_state(struct r600_pipe_context *rctx)
{
	struct pipe_context * ctx = (struct pipe_context*)rctx;

	if (!rctx->blitter->running) {
		if (rctx->have_depth_fb || rctx->have_depth_texture)
			r600_flush_depth_textures(rctx);
	}

	if (rctx->chip_class < EVERGREEN) {
		r600_update_sampler_states(rctx);
	}

	if (rctx->vs_shader->shader.clamp_color != rctx->clamp_vertex_color) {
		r600_shader_rebuild(&rctx->context, rctx->vs_shader);
	}

	if ((rctx->ps_shader->shader.clamp_color != rctx->clamp_fragment_color) ||
	    ((rctx->chip_class >= EVERGREEN) && rctx->ps_shader->shader.fs_write_all &&
	     (rctx->ps_shader->shader.nr_cbufs != rctx->nr_cbufs))) {
		r600_shader_rebuild(&rctx->context, rctx->ps_shader);
	}

	if (rctx->alpha_ref_dirty) {
		r600_update_alpha_ref(rctx);
	}

	if (rctx->ps_shader && rctx->sprite_coord_enable &&
		(rctx->ps_shader->sprite_coord_enable != rctx->sprite_coord_enable)) {

		if (rctx->chip_class >= EVERGREEN)
			evergreen_pipe_shader_ps(ctx, rctx->ps_shader);
		else
			r600_pipe_shader_ps(ctx, rctx->ps_shader);

		r600_context_pipe_state_set(&rctx->ctx, &rctx->ps_shader->rstate);
	}

}

void r600_draw_vbo(struct pipe_context *ctx, const struct pipe_draw_info *dinfo)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct pipe_draw_info info = *dinfo;
	struct r600_draw rdraw = {};
	struct pipe_index_buffer ib = {};
	unsigned prim, mask, ls_mask = 0;

	if ((!info.count && (info.indexed || !info.count_from_stream_output)) ||
	    (info.indexed && !rctx->vbuf_mgr->index_buffer.buffer) ||
	    !r600_conv_pipe_prim(info.mode, &prim)) {
		return;
	}

	if (!rctx->ps_shader || !rctx->vs_shader)
		return;

	r600_update_derived_state(rctx);

	u_vbuf_draw_begin(rctx->vbuf_mgr, &info);
	r600_vertex_buffer_update(rctx);

	rdraw.vgt_num_indices = info.count;
	rdraw.vgt_num_instances = info.instance_count;

	if (info.indexed) {
		/* Initialize the index buffer struct. */
		pipe_resource_reference(&ib.buffer, rctx->vbuf_mgr->index_buffer.buffer);
		ib.index_size = rctx->vbuf_mgr->index_buffer.index_size;
		ib.offset = rctx->vbuf_mgr->index_buffer.offset + info.start * ib.index_size;

		/* Translate or upload, if needed. */
		r600_translate_index_buffer(rctx, &ib, info.count);

		if (u_vbuf_resource(ib.buffer)->user_ptr) {
			r600_upload_index_buffer(rctx, &ib, info.count);
		}

		/* Initialize the r600_draw struct with index buffer info. */
		if (ib.index_size == 4) {
			rdraw.vgt_index_type = VGT_INDEX_32 |
				(R600_BIG_ENDIAN ? VGT_DMA_SWAP_32_BIT : 0);
		} else {
			rdraw.vgt_index_type = VGT_INDEX_16 |
				(R600_BIG_ENDIAN ? VGT_DMA_SWAP_16_BIT : 0);
		}
		rdraw.indices = (struct r600_resource*)ib.buffer;
		rdraw.indices_bo_offset = ib.offset;
		rdraw.vgt_draw_initiator = V_0287F0_DI_SRC_SEL_DMA;
	} else {
		info.index_bias = info.start;
		rdraw.vgt_draw_initiator = V_0287F0_DI_SRC_SEL_AUTO_INDEX;
		if (info.count_from_stream_output) {
			rdraw.vgt_draw_initiator |= S_0287F0_USE_OPAQUE(1);

			r600_context_draw_opaque_count(&rctx->ctx, (struct r600_so_target*)info.count_from_stream_output);
		}
	}

	rctx->ctx.vs_shader_so_strides = rctx->vs_shader->so_strides;

	mask = (1ULL << ((unsigned)rctx->framebuffer.nr_cbufs * 4)) - 1;

	if (rctx->vgt.id != R600_PIPE_STATE_VGT) {
		rctx->vgt.id = R600_PIPE_STATE_VGT;
		rctx->vgt.nregs = 0;
		r600_pipe_state_add_reg(&rctx->vgt, R_008958_VGT_PRIMITIVE_TYPE, prim, 0xFFFFFFFF, NULL, 0);
		r600_pipe_state_add_reg(&rctx->vgt, R_028238_CB_TARGET_MASK, rctx->cb_target_mask & mask, 0xFFFFFFFF, NULL, 0);
		r600_pipe_state_add_reg(&rctx->vgt, R_028400_VGT_MAX_VTX_INDX, ~0, 0xFFFFFFFF, NULL, 0);
		r600_pipe_state_add_reg(&rctx->vgt, R_028404_VGT_MIN_VTX_INDX, 0, 0xFFFFFFFF, NULL, 0);
		r600_pipe_state_add_reg(&rctx->vgt, R_028408_VGT_INDX_OFFSET, info.index_bias, 0xFFFFFFFF, NULL, 0);
		r600_pipe_state_add_reg(&rctx->vgt, R_02840C_VGT_MULTI_PRIM_IB_RESET_INDX, info.restart_index, 0xFFFFFFFF, NULL, 0);
		r600_pipe_state_add_reg(&rctx->vgt, R_028A94_VGT_MULTI_PRIM_IB_RESET_EN, info.primitive_restart, 0xFFFFFFFF, NULL, 0);
		r600_pipe_state_add_reg(&rctx->vgt, R_03CFF0_SQ_VTX_BASE_VTX_LOC, 0, 0xFFFFFFFF, NULL, 0);
		r600_pipe_state_add_reg(&rctx->vgt, R_03CFF4_SQ_VTX_START_INST_LOC, info.start_instance, 0xFFFFFFFF, NULL, 0);
		r600_pipe_state_add_reg(&rctx->vgt, R_028A0C_PA_SC_LINE_STIPPLE,
					0,
					S_028A0C_AUTO_RESET_CNTL(3), NULL, 0);
		r600_pipe_state_add_reg(&rctx->vgt, R_028814_PA_SU_SC_MODE_CNTL,
					0,
					S_028814_PROVOKING_VTX_LAST(1), NULL, 0);
	}

	rctx->vgt.nregs = 0;
	r600_pipe_state_mod_reg(&rctx->vgt, prim);
	r600_pipe_state_mod_reg(&rctx->vgt, rctx->cb_target_mask & mask);
	r600_pipe_state_mod_reg(&rctx->vgt, ~0);
	r600_pipe_state_mod_reg(&rctx->vgt, 0);
	r600_pipe_state_mod_reg(&rctx->vgt, info.index_bias);
	r600_pipe_state_mod_reg(&rctx->vgt, info.restart_index);
	r600_pipe_state_mod_reg(&rctx->vgt, info.primitive_restart);
	r600_pipe_state_mod_reg(&rctx->vgt, 0);
	r600_pipe_state_mod_reg(&rctx->vgt, info.start_instance);

	if (prim == V_008958_DI_PT_LINELIST)
		ls_mask = 1;
	else if (prim == V_008958_DI_PT_LINESTRIP) 
		ls_mask = 2;
	r600_pipe_state_mod_reg(&rctx->vgt, S_028A0C_AUTO_RESET_CNTL(ls_mask));

	if (info.mode == PIPE_PRIM_QUADS || info.mode == PIPE_PRIM_QUAD_STRIP || info.mode == PIPE_PRIM_POLYGON) {
		r600_pipe_state_mod_reg(&rctx->vgt, S_028814_PROVOKING_VTX_LAST(1));
	}

	r600_context_pipe_state_set(&rctx->ctx, &rctx->vgt);

	if (rctx->chip_class >= EVERGREEN) {
		evergreen_context_draw(&rctx->ctx, &rdraw);
	} else {
		r600_context_draw(&rctx->ctx, &rdraw);
	}

	if (rctx->framebuffer.zsbuf)
	{
		struct pipe_resource *tex = rctx->framebuffer.zsbuf->texture;
		((struct r600_resource_texture *)tex)->dirty_db = TRUE;
	}

	pipe_resource_reference(&ib.buffer, NULL);
	u_vbuf_draw_end(rctx->vbuf_mgr);
}

void _r600_pipe_state_add_reg(struct r600_context *ctx,
			      struct r600_pipe_state *state,
			      u32 offset, u32 value, u32 mask,
			      u32 range_id, u32 block_id,
			      struct r600_resource *bo,
			      enum radeon_bo_usage usage)
{
	struct r600_range *range;
	struct r600_block *block;

	if (bo) assert(usage);

	range = &ctx->range[range_id];
	block = range->blocks[block_id];
	state->regs[state->nregs].block = block;
	state->regs[state->nregs].id = (offset - block->start_offset) >> 2;

	state->regs[state->nregs].value = value;
	state->regs[state->nregs].mask = mask;
	state->regs[state->nregs].bo = bo;
	state->regs[state->nregs].bo_usage = usage;

	state->nregs++;
	assert(state->nregs < R600_BLOCK_MAX_REG);
}

void r600_pipe_state_add_reg_noblock(struct r600_pipe_state *state,
				     u32 offset, u32 value, u32 mask,
				     struct r600_resource *bo,
				     enum radeon_bo_usage usage)
{
	if (bo) assert(usage);

	state->regs[state->nregs].id = offset;
	state->regs[state->nregs].block = NULL;
	state->regs[state->nregs].value = value;
	state->regs[state->nregs].mask = mask;
	state->regs[state->nregs].bo = bo;
	state->regs[state->nregs].bo_usage = usage;

	state->nregs++;
	assert(state->nregs < R600_BLOCK_MAX_REG);
}

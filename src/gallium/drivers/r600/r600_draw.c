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
 *      Corbin Simpson
 */
#include <stdio.h>
#include <errno.h>
#include <pipe/p_screen.h>
#include <util/u_format.h>
#include <util/u_math.h>
#include <util/u_inlines.h>
#include <util/u_memory.h>
#include <util/u_index_modify.h>
#include "translate/translate_cache.h"
#include "translate/translate.h"
#include "radeon.h"
#include "r600_screen.h"
#include "r600_context.h"
#include "r600_resource.h"
#include "r600_state_inlines.h"

static void r600_begin_vertex_translate(struct r600_context *rctx)
{
	struct pipe_context *pipe = &rctx->context;
	struct translate_key key = {0};
	struct translate_element *te;
	unsigned tr_elem_index[PIPE_MAX_ATTRIBS] = {0};
	struct translate *tr;
	struct r600_vertex_element *ve = rctx->vertex_elements;
	boolean vb_translated[PIPE_MAX_ATTRIBS] = {0};
	void *vb_map[PIPE_MAX_ATTRIBS] = {0}, *out_map;
	struct pipe_transfer *vb_transfer[PIPE_MAX_ATTRIBS] = {0}, *out_transfer;
	struct pipe_resource *out_buffer;
	unsigned i, num_verts;

	/* Initialize the translate key, i.e. the recipe how vertices should be
	 * translated. */
	for (i = 0; i < ve->count; i++) {
		struct pipe_vertex_buffer *vb =
			&rctx->vertex_buffer[ve->elements[i].vertex_buffer_index];
		enum pipe_format output_format = ve->hw_format[i];
		unsigned output_format_size = ve->hw_format_size[i];

		/* Check for support. */
		if (ve->elements[i].src_format == ve->hw_format[i] &&
		    (vb->buffer_offset + ve->elements[i].src_offset) % 4 == 0 &&
		    vb->stride % 4 == 0) {
			continue;
		}

		/* Workaround for translate: output floats instead of halfs. */
		switch (output_format) {
		case PIPE_FORMAT_R16_FLOAT:
			output_format = PIPE_FORMAT_R32_FLOAT;
			output_format_size = 4;
			break;
		case PIPE_FORMAT_R16G16_FLOAT:
			output_format = PIPE_FORMAT_R32G32_FLOAT;
			output_format_size = 8;
			break;
		case PIPE_FORMAT_R16G16B16_FLOAT:
			output_format = PIPE_FORMAT_R32G32B32_FLOAT;
			output_format_size = 12;
			break;
		case PIPE_FORMAT_R16G16B16A16_FLOAT:
			output_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
			output_format_size = 16;
			break;
		default:;
		}

		/* Add this vertex element. */
		te = &key.element[key.nr_elements];
		/*te->type;
		  te->instance_divisor;*/
		te->input_buffer = ve->elements[i].vertex_buffer_index;
		te->input_format = ve->elements[i].src_format;
		te->input_offset = vb->buffer_offset + ve->elements[i].src_offset;
		te->output_format = output_format;
		te->output_offset = key.output_stride;

		key.output_stride += output_format_size;
		vb_translated[ve->elements[i].vertex_buffer_index] = TRUE;
		tr_elem_index[i] = key.nr_elements;
		key.nr_elements++;
	}

	/* Get a translate object. */
	tr = translate_cache_find(rctx->tran.translate_cache, &key);

	/* Map buffers we want to translate. */
	for (i = 0; i < rctx->nvertex_buffer; i++) {
		if (vb_translated[i]) {
			struct pipe_vertex_buffer *vb = &rctx->vertex_buffer[i];

			vb_map[i] = pipe_buffer_map(pipe, vb->buffer,
						    PIPE_TRANSFER_READ, &vb_transfer[i]);

			tr->set_buffer(tr, i, vb_map[i], vb->stride, vb->max_index);
		}
	}

	/* Create and map the output buffer. */
	num_verts = rctx->vb_max_index + 1;

	out_buffer = pipe_buffer_create(&rctx->screen->screen,
					PIPE_BIND_VERTEX_BUFFER,
					key.output_stride * num_verts);

	out_map = pipe_buffer_map(pipe, out_buffer, PIPE_TRANSFER_WRITE,
				  &out_transfer);

	/* Translate. */
	tr->run(tr, 0, num_verts, 0, out_map);

	/* Unmap all buffers. */
	for (i = 0; i < rctx->nvertex_buffer; i++) {
		if (vb_translated[i]) {
			pipe_buffer_unmap(pipe, rctx->vertex_buffer[i].buffer,
					  vb_transfer[i]);
		}
	}

	pipe_buffer_unmap(pipe, out_buffer, out_transfer);

	/* Setup the new vertex buffer in the first free slot. */
	for (i = 0; i < PIPE_MAX_ATTRIBS; i++) {
		struct pipe_vertex_buffer *vb = &rctx->vertex_buffer[i];

		if (!vb->buffer) {
			pipe_resource_reference(&vb->buffer, out_buffer);
			vb->buffer_offset = 0;
			vb->max_index = num_verts - 1;
			vb->stride = key.output_stride;
			rctx->tran.vb_slot = i;
			break;
		}
	}

	/* Save and replace vertex elements. */
	{
		struct pipe_vertex_element new_velems[PIPE_MAX_ATTRIBS];

		rctx->tran.saved_velems = rctx->vertex_elements;

		for (i = 0; i < ve->count; i++) {
			if (vb_translated[ve->elements[i].vertex_buffer_index]) {
				te = &key.element[tr_elem_index[i]];
				new_velems[i].instance_divisor = ve->elements[i].instance_divisor;
				new_velems[i].src_format = te->output_format;
				new_velems[i].src_offset = te->output_offset;
				new_velems[i].vertex_buffer_index = rctx->tran.vb_slot;
			} else {
				memcpy(&new_velems[i], &ve->elements[i],
				       sizeof(struct pipe_vertex_element));
			}
		}

		rctx->tran.new_velems =
			pipe->create_vertex_elements_state(pipe, ve->count, new_velems);
		pipe->bind_vertex_elements_state(pipe, rctx->tran.new_velems);
	}

	pipe_resource_reference(&out_buffer, NULL);
}

static void r600_end_vertex_translate(struct r600_context *rctx)
{
	struct pipe_context *pipe = &rctx->context;

	/* Restore vertex elements. */
	pipe->bind_vertex_elements_state(pipe, rctx->tran.saved_velems);
	pipe->delete_vertex_elements_state(pipe, rctx->tran.new_velems);

	/* Delete the now-unused VBO. */
	pipe_resource_reference(&rctx->vertex_buffer[rctx->tran.vb_slot].buffer,
				NULL);
}


static void r600_translate_index_buffer(struct r600_context *r600,
					struct pipe_resource **index_buffer,
					unsigned *index_size,
					unsigned *start, unsigned count)
{
    switch (*index_size) {
        case 1:
		util_shorten_ubyte_elts(&r600->context, index_buffer, 0, *start, count);
		*index_size = 2;
		*start = 0;
		break;

        case 2:
		if (*start % 2 != 0) {
			util_rebuild_ushort_elts(&r600->context, index_buffer, 0, *start, count);
			*start = 0;
		}
		break;
		
        case 4:
		break;
    }
}

static int r600_draw_common(struct r600_draw *draw)
{
	struct r600_context *rctx = r600_context(draw->ctx);
	/* FIXME vs_resource */
	struct radeon_state *vs_resource;
	struct r600_resource *rbuffer;
	unsigned i, j, offset, prim;
	u32 vgt_dma_index_type, vgt_draw_initiator;
	struct pipe_vertex_buffer *vertex_buffer;
	int r;

	r = r600_context_hw_states(draw->ctx);
	if (r)
		return r;
	switch (draw->index_size) {
	case 2:
		vgt_draw_initiator = S_0287F0_SOURCE_SELECT(V_0287F0_DI_SRC_SEL_DMA);
		vgt_dma_index_type = 0;
		break;
	case 4:
		vgt_draw_initiator = S_0287F0_SOURCE_SELECT(V_0287F0_DI_SRC_SEL_DMA);
		vgt_dma_index_type = 1;
		break;
	case 0:
		vgt_draw_initiator = S_0287F0_SOURCE_SELECT(V_0287F0_DI_SRC_SEL_AUTO_INDEX);
		vgt_dma_index_type = 0;
		break;
	default:
		fprintf(stderr, "%s %d unsupported index size %d\n", __func__, __LINE__, draw->index_size);
		return -EINVAL;
	}
	r = r600_conv_pipe_prim(draw->mode, &prim);
	if (r)
		return r;

	/* rebuild vertex shader if input format changed */
	r = r600_pipe_shader_update(draw->ctx, rctx->vs_shader);
	if (r)
		return r;
	r = r600_pipe_shader_update(draw->ctx, rctx->ps_shader);
	if (r)
		return r;
	radeon_draw_bind(&rctx->draw, &rctx->vs_shader->rstate[0]);
	radeon_draw_bind(&rctx->draw, &rctx->ps_shader->rstate[0]);

	for (i = 0 ; i < rctx->vs_nresource; i++) {
		radeon_state_fini(&rctx->vs_resource[i]);
	}
	for (i = 0 ; i < rctx->vertex_elements->count; i++) {
		vs_resource = &rctx->vs_resource[i];
		j = rctx->vertex_elements->elements[i].vertex_buffer_index;
		vertex_buffer = &rctx->vertex_buffer[j];
		rbuffer = (struct r600_resource*)vertex_buffer->buffer;
		offset = rctx->vertex_elements->elements[i].src_offset + vertex_buffer->buffer_offset;
		
		rctx->vtbl->vs_resource(rctx, i, rbuffer, offset, vertex_buffer->stride, rctx->vertex_elements->hw_format[i]);
		radeon_draw_bind(&rctx->draw, vs_resource);
	}
	rctx->vs_nresource = rctx->vertex_elements->count;
	/* FIXME start need to change winsys */
	rctx->vtbl->vgt_init(draw, vgt_draw_initiator);
	radeon_draw_bind(&rctx->draw, &draw->draw);

	rctx->vtbl->vgt_prim(draw, prim, vgt_dma_index_type);
	radeon_draw_bind(&rctx->draw, &draw->vgt);

	r = radeon_ctx_set_draw(rctx->ctx, &rctx->draw);
	if (r == -EBUSY) {
		r600_flush(draw->ctx, 0, NULL);
		r = radeon_ctx_set_draw(rctx->ctx, &rctx->draw);
	}

	radeon_state_fini(&draw->draw);

	return r;
}

void r600_draw_vbo(struct pipe_context *ctx, const struct pipe_draw_info *info)
{
	struct r600_context *rctx = r600_context(ctx);
	struct r600_draw draw;
	int r;
	boolean translate = FALSE;

	memset(&draw, 0, sizeof(draw));

	if (rctx->vertex_elements->incompatible_layout) {
		r600_begin_vertex_translate(rctx);
		translate = TRUE;
	}
	if (rctx->any_user_vbs) {
		r600_upload_user_buffers(rctx);
		rctx->any_user_vbs = FALSE;
	}

	draw.ctx = ctx;
	draw.mode = info->mode;
	draw.start = info->start;
	draw.count = info->count;
	if (info->indexed && rctx->index_buffer.buffer) {
		draw.start += rctx->index_buffer.offset / rctx->index_buffer.index_size;
		draw.min_index = info->min_index;
		draw.max_index = info->max_index;
		draw.index_bias = info->index_bias;

		r600_translate_index_buffer(rctx, &rctx->index_buffer.buffer,
					    &rctx->index_buffer.index_size,
					    &draw.start,
					    info->count);

		draw.index_size = rctx->index_buffer.index_size;
		pipe_resource_reference(&draw.index_buffer, rctx->index_buffer.buffer);
		draw.index_buffer_offset = draw.start * draw.index_size;
		draw.start = 0;
		r600_upload_index_buffer(rctx, &draw);
	}
	else {
		draw.index_size = 0;
		draw.index_buffer = NULL;
		draw.min_index = 0;
		draw.max_index = 0xffffff;
		draw.index_buffer_offset = 0;
		draw.index_bias = draw.start;
	}

	r = r600_draw_common(&draw);
	if (r)
	  fprintf(stderr,"draw common failed %d\n", r);

	if (translate) {
		r600_end_vertex_translate(rctx);
	}
	pipe_resource_reference(&draw.index_buffer, NULL);
}

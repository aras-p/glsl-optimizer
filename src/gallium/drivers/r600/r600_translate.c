/*
 * Copyright 2010 Red Hat Inc.
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
 */

#include "translate/translate_cache.h"
#include "translate/translate.h"
#include <pipebuffer/pb_buffer.h>
#include <util/u_index_modify.h>
#include "util/u_upload_mgr.h"
#include "r600_pipe.h"

void r600_begin_vertex_translate(struct r600_pipe_context *rctx,
                                 int min_index, int max_index)
{
	struct pipe_context *pipe = &rctx->context;
	struct translate_key key = {0};
	struct translate_element *te;
	unsigned tr_elem_index[PIPE_MAX_ATTRIBS] = {0};
	struct translate *tr;
	struct r600_vertex_element *ve = rctx->vertex_elements;
	boolean vb_translated[PIPE_MAX_ATTRIBS] = {0};
	uint8_t *vb_map[PIPE_MAX_ATTRIBS] = {0}, *out_map;
	struct pipe_transfer *vb_transfer[PIPE_MAX_ATTRIBS] = {0};
	struct pipe_resource *out_buffer = NULL;
	unsigned i, num_verts, out_offset;
	struct pipe_vertex_element new_velems[PIPE_MAX_ATTRIBS];
	boolean flushed;

	/* Initialize the translate key, i.e. the recipe how vertices should be
	 * translated. */
	for (i = 0; i < ve->count; i++) {
		enum pipe_format output_format = ve->hw_format[i];
		unsigned output_format_size = ve->hw_format_size[i];

		/* Check for support. */
		if (ve->elements[i].src_format == ve->hw_format[i]) {
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
		te->input_offset = ve->elements[i].src_offset;
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
	for (i = 0; i < rctx->nvertex_buffers; i++) {
		if (vb_translated[i]) {
			struct pipe_vertex_buffer *vb = &rctx->vertex_buffer[i];

			vb_map[i] = pipe_buffer_map(pipe, vb->buffer,
						    PIPE_TRANSFER_READ, &vb_transfer[i]);

			tr->set_buffer(tr, i,
				       vb_map[i] + vb->buffer_offset + vb->stride * min_index,
				       vb->stride, ~0);
		}
	}

	/* Create and map the output buffer. */
	num_verts = max_index + 1 - min_index;

	u_upload_alloc(rctx->upload_vb,
		       key.output_stride * min_index,
		       key.output_stride * num_verts,
		       &out_offset, &out_buffer, &flushed,
		       (void**)&out_map);

	out_offset -= key.output_stride * min_index;

	/* Translate. */
	tr->run(tr, 0, num_verts, 0, out_map);

	/* Unmap all buffers. */
	for (i = 0; i < rctx->nvertex_buffers; i++) {
		if (vb_translated[i]) {
			pipe_buffer_unmap(pipe, vb_transfer[i]);
		}
	}

	/* Find the first free slot. */
	rctx->tran.vb_slot = ~0;
	for (i = 0; i < PIPE_MAX_ATTRIBS; i++) {
		if (!rctx->vertex_buffer[i].buffer) {
			rctx->tran.vb_slot = i;

			if (i >= rctx->nvertex_buffers) {
				rctx->nreal_vertex_buffers = i+1;
			}
			break;
		}
	}

	if (rctx->tran.vb_slot != ~0) {
		/* Setup the new vertex buffer. */
		pipe_resource_reference(&rctx->real_vertex_buffer[rctx->tran.vb_slot], out_buffer);
		rctx->vertex_buffer[rctx->tran.vb_slot].buffer_offset = out_offset;
		rctx->vertex_buffer[rctx->tran.vb_slot].stride = key.output_stride;

		/* Setup new vertex elements. */
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

		rctx->tran.saved_velems = rctx->vertex_elements;
		rctx->tran.new_velems =
				pipe->create_vertex_elements_state(pipe, ve->count, new_velems);
		pipe->bind_vertex_elements_state(pipe, rctx->tran.new_velems);
	}

	pipe_resource_reference(&out_buffer, NULL);
}

void r600_end_vertex_translate(struct r600_pipe_context *rctx)
{
	struct pipe_context *pipe = &rctx->context;

	if (rctx->tran.new_velems == NULL) {
		return;
	}

	/* Restore vertex elements. */
	pipe->bind_vertex_elements_state(pipe, rctx->tran.saved_velems);
	rctx->tran.saved_velems = NULL;
	pipe->delete_vertex_elements_state(pipe, rctx->tran.new_velems);
	rctx->tran.new_velems = NULL;

	/* Delete the now-unused VBO. */
	pipe_resource_reference(&rctx->real_vertex_buffer[rctx->tran.vb_slot], NULL);
	rctx->nreal_vertex_buffers = rctx->nvertex_buffers;
}

void r600_translate_index_buffer(struct r600_pipe_context *r600,
				 struct pipe_resource **index_buffer,
				 unsigned *index_size,
				 unsigned *start, unsigned count)
{
	struct pipe_resource *out_buffer = NULL;
	unsigned out_offset;
	void *ptr;
	boolean flushed;

	switch (*index_size) {
	case 1:
		u_upload_alloc(r600->upload_vb, 0, count * 2,
			       &out_offset, &out_buffer, &flushed, &ptr);

		util_shorten_ubyte_elts_to_userptr(
				&r600->context, *index_buffer, 0, *start, count, ptr);

		pipe_resource_reference(index_buffer, out_buffer);
		*index_size = 2;
		*start = out_offset / 2;
		break;
	}
}

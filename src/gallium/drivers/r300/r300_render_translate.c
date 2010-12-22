/*
 * Copyright 2010 Marek Olšák <maraeo@gmail.com>
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
 * USE OR OTHER DEALINGS IN THE SOFTWARE. */

/**
 * The functions below translate vertex and index buffers to the layout
 * compatible with the hardware, so that all vertex and index fetches are
 * DWORD-aligned and all used vertex and index formats are supported.
 * For indices, an optional index offset is added to each index.
 */

#include "r300_context.h"
#include "translate/translate.h"
#include "util/u_index_modify.h"

void r300_begin_vertex_translate(struct r300_context *r300)
{
    struct pipe_context *pipe = &r300->context;
    struct translate_key key = {0};
    struct translate_element *te;
    unsigned tr_elem_index[PIPE_MAX_ATTRIBS] = {0};
    struct translate *tr;
    struct r300_vertex_element_state *ve = r300->velems;
    boolean vb_translated[PIPE_MAX_ATTRIBS] = {0};
    void *vb_map[PIPE_MAX_ATTRIBS] = {0}, *out_map;
    struct pipe_transfer *vb_transfer[PIPE_MAX_ATTRIBS] = {0}, *out_transfer;
    struct pipe_resource *out_buffer;
    unsigned i, num_verts;

    /* Initialize the translate key, i.e. the recipe how vertices should be
     * translated. */
    for (i = 0; i < ve->count; i++) {
        struct pipe_vertex_buffer *vb =
                &r300->vertex_buffer[ve->velem[i].vertex_buffer_index];
        enum pipe_format output_format = ve->hw_format[i];
        unsigned output_format_size = ve->hw_format_size[i];

        /* Check for support. */
        if (ve->velem[i].src_format == ve->hw_format[i] &&
            (vb->buffer_offset + ve->velem[i].src_offset) % 4 == 0 &&
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
        te->input_buffer = ve->velem[i].vertex_buffer_index;
        te->input_format = ve->velem[i].src_format;
        te->input_offset = vb->buffer_offset + ve->velem[i].src_offset;
        te->output_format = output_format;
        te->output_offset = key.output_stride;

        key.output_stride += output_format_size;
        vb_translated[ve->velem[i].vertex_buffer_index] = TRUE;
        tr_elem_index[i] = key.nr_elements;
        key.nr_elements++;
    }

    /* Get a translate object. */
    tr = translate_cache_find(r300->tran.translate_cache, &key);

    /* Map buffers we want to translate. */
    for (i = 0; i < r300->vertex_buffer_count; i++) {
        if (vb_translated[i]) {
            struct pipe_vertex_buffer *vb = &r300->vertex_buffer[i];

            vb_map[i] = pipe_buffer_map(pipe, vb->buffer,
                                        PIPE_TRANSFER_READ, &vb_transfer[i]);

            tr->set_buffer(tr, i, vb_map[i], vb->stride, vb->max_index);
        }
    }

    /* Create and map the output buffer. */
    num_verts = r300->vertex_buffer_max_index + 1;

    out_buffer = pipe_buffer_create(&r300->screen->screen,
                                    PIPE_BIND_VERTEX_BUFFER,
                                    key.output_stride * num_verts);

    out_map = pipe_buffer_map(pipe, out_buffer, PIPE_TRANSFER_WRITE,
                              &out_transfer);

    /* Translate. */
    tr->run(tr, 0, num_verts, 0, out_map);

    /* Unmap all buffers. */
    for (i = 0; i < r300->vertex_buffer_count; i++) {
        if (vb_translated[i]) {
            pipe_buffer_unmap(pipe, vb_transfer[i]);
        }
    }

    pipe_buffer_unmap(pipe, out_transfer);

    /* Setup the new vertex buffer in the first free slot. */
    for (i = 0; i < PIPE_MAX_ATTRIBS; i++) {
        struct pipe_vertex_buffer *vb = &r300->vertex_buffer[i];

        if (!vb->buffer) {
            pipe_resource_reference(&vb->buffer, out_buffer);
            vb->buffer_offset = 0;
            vb->max_index = num_verts - 1;
            vb->stride = key.output_stride;
            r300->tran.vb_slot = i;
            r300->validate_buffers = TRUE;
            break;
        }
    }

    /* Save and replace vertex elements. */
    {
        struct pipe_vertex_element new_velems[PIPE_MAX_ATTRIBS];

        r300->tran.saved_velems = r300->velems;

        for (i = 0; i < ve->count; i++) {
            if (vb_translated[ve->velem[i].vertex_buffer_index]) {
                te = &key.element[tr_elem_index[i]];
                new_velems[i].instance_divisor = ve->velem[i].instance_divisor;
                new_velems[i].src_format = te->output_format;
                new_velems[i].src_offset = te->output_offset;
                new_velems[i].vertex_buffer_index = r300->tran.vb_slot;
            } else {
                memcpy(&new_velems[i], &ve->velem[i],
                       sizeof(struct pipe_vertex_element));
            }
        }

        r300->tran.new_velems =
            pipe->create_vertex_elements_state(pipe, ve->count, new_velems);
        pipe->bind_vertex_elements_state(pipe, r300->tran.new_velems);
    }

    pipe_resource_reference(&out_buffer, NULL);
}

void r300_end_vertex_translate(struct r300_context *r300)
{
    struct pipe_context *pipe = &r300->context;

    /* Restore vertex elements. */
    pipe->bind_vertex_elements_state(pipe, r300->tran.saved_velems);
    pipe->delete_vertex_elements_state(pipe, r300->tran.new_velems);

    /* Delete the now-unused VBO. */
    pipe_resource_reference(&r300->vertex_buffer[r300->tran.vb_slot].buffer,
                            NULL);
}

void r300_translate_index_buffer(struct r300_context *r300,
                                 struct pipe_resource **index_buffer,
                                 unsigned *index_size, unsigned index_offset,
                                 unsigned *start, unsigned count)
{
    switch (*index_size) {
        case 1:
            util_shorten_ubyte_elts(&r300->context, index_buffer, index_offset, *start, count);
            *index_size = 2;
            *start = 0;
            r300->validate_buffers = TRUE;
            break;

        case 2:
            if (index_offset) {
                util_rebuild_ushort_elts(&r300->context, index_buffer, index_offset, *start, count);
                *start = 0;
                r300->validate_buffers = TRUE;
            }
            break;

        case 4:
            if (index_offset) {
                util_rebuild_uint_elts(&r300->context, index_buffer, index_offset, *start, count);
                *start = 0;
                r300->validate_buffers = TRUE;
            }
            break;
    }
}

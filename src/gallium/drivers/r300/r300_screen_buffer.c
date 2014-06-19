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
 * Authors: Dave Airlie
 */

#include <stdio.h>

#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_upload_mgr.h"
#include "util/u_math.h"

#include "r300_screen_buffer.h"

void r300_upload_index_buffer(struct r300_context *r300,
			      struct pipe_resource **index_buffer,
			      unsigned index_size, unsigned *start,
			      unsigned count, const uint8_t *ptr)
{
    unsigned index_offset;

    *index_buffer = NULL;

    u_upload_data(r300->uploader,
                  0, count * index_size,
                  ptr + (*start * index_size),
                  &index_offset,
                  index_buffer);

    *start = index_offset / index_size;
}

static void r300_buffer_destroy(struct pipe_screen *screen,
				struct pipe_resource *buf)
{
    struct r300_resource *rbuf = r300_resource(buf);

    align_free(rbuf->malloced_buffer);

    if (rbuf->buf)
        pb_reference(&rbuf->buf, NULL);

    FREE(rbuf);
}

static void *
r300_buffer_transfer_map( struct pipe_context *context,
                          struct pipe_resource *resource,
                          unsigned level,
                          unsigned usage,
                          const struct pipe_box *box,
                          struct pipe_transfer **ptransfer )
{
    struct r300_context *r300 = r300_context(context);
    struct radeon_winsys *rws = r300->screen->rws;
    struct r300_resource *rbuf = r300_resource(resource);
    struct pipe_transfer *transfer;
    uint8_t *map;

    transfer = util_slab_alloc(&r300->pool_transfers);
    transfer->resource = resource;
    transfer->level = level;
    transfer->usage = usage;
    transfer->box = *box;
    transfer->stride = 0;
    transfer->layer_stride = 0;

    if (rbuf->malloced_buffer) {
        *ptransfer = transfer;
        return rbuf->malloced_buffer + box->x;
    }

    if (usage & PIPE_TRANSFER_DISCARD_WHOLE_RESOURCE &&
        !(usage & PIPE_TRANSFER_UNSYNCHRONIZED)) {
        assert(usage & PIPE_TRANSFER_WRITE);

        /* Check if mapping this buffer would cause waiting for the GPU. */
        if (r300->rws->cs_is_buffer_referenced(r300->cs, rbuf->cs_buf, RADEON_USAGE_READWRITE) ||
            r300->rws->buffer_is_busy(rbuf->buf, RADEON_USAGE_READWRITE)) {
            unsigned i;
            struct pb_buffer *new_buf;

            /* Create a new one in the same pipe_resource. */
            new_buf = r300->rws->buffer_create(r300->rws, rbuf->b.b.width0,
                                               R300_BUFFER_ALIGNMENT, TRUE,
                                               rbuf->domain, 0);
            if (new_buf) {
                /* Discard the old buffer. */
                pb_reference(&rbuf->buf, NULL);
                rbuf->buf = new_buf;
                rbuf->cs_buf = r300->rws->buffer_get_cs_handle(rbuf->buf);

                /* We changed the buffer, now we need to bind it where the old one was bound. */
                for (i = 0; i < r300->nr_vertex_buffers; i++) {
                    if (r300->vertex_buffer[i].buffer == &rbuf->b.b) {
                        r300->vertex_arrays_dirty = TRUE;
                        break;
                    }
                }
            }
        }
    }

    /* Buffers are never used for write, therefore mapping for read can be
     * unsynchronized. */
    if (!(usage & PIPE_TRANSFER_WRITE)) {
       usage |= PIPE_TRANSFER_UNSYNCHRONIZED;
    }

    map = rws->buffer_map(rbuf->cs_buf, r300->cs, usage);

    if (map == NULL) {
        util_slab_free(&r300->pool_transfers, transfer);
        return NULL;
    }

    *ptransfer = transfer;
    return map + box->x;
}

static void r300_buffer_transfer_unmap( struct pipe_context *pipe,
                                        struct pipe_transfer *transfer )
{
    struct r300_context *r300 = r300_context(pipe);

    util_slab_free(&r300->pool_transfers, transfer);
}

static const struct u_resource_vtbl r300_buffer_vtbl =
{
   NULL,                               /* get_handle */
   r300_buffer_destroy,                /* resource_destroy */
   r300_buffer_transfer_map,           /* transfer_map */
   NULL,                               /* transfer_flush_region */
   r300_buffer_transfer_unmap,         /* transfer_unmap */
   NULL   /* transfer_inline_write */
};

struct pipe_resource *r300_buffer_create(struct pipe_screen *screen,
					 const struct pipe_resource *templ)
{
    struct r300_screen *r300screen = r300_screen(screen);
    struct r300_resource *rbuf;

    rbuf = MALLOC_STRUCT(r300_resource);

    rbuf->b.b = *templ;
    rbuf->b.vtbl = &r300_buffer_vtbl;
    pipe_reference_init(&rbuf->b.b.reference, 1);
    rbuf->b.b.screen = screen;
    rbuf->domain = RADEON_DOMAIN_GTT;
    rbuf->buf = NULL;
    rbuf->malloced_buffer = NULL;

    /* Allocate constant buffers and SWTCL vertex and index buffers in RAM.
     * Note that uploaded index buffers use the flag PIPE_BIND_CUSTOM, so that
     * we can distinguish them from user-created buffers.
     */
    if (templ->bind & PIPE_BIND_CONSTANT_BUFFER ||
        (!r300screen->caps.has_tcl && !(templ->bind & PIPE_BIND_CUSTOM))) {
        rbuf->malloced_buffer = align_malloc(templ->width0, 64);
        return &rbuf->b.b;
    }

    rbuf->buf =
        r300screen->rws->buffer_create(r300screen->rws, rbuf->b.b.width0,
                                       R300_BUFFER_ALIGNMENT, TRUE,
                                       rbuf->domain, 0);
    if (!rbuf->buf) {
        FREE(rbuf);
        return NULL;
    }

    rbuf->cs_buf =
        r300screen->rws->buffer_get_cs_handle(rbuf->buf);

    return &rbuf->b.b;
}

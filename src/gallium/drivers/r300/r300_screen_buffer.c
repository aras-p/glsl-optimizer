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
#include "r300_winsys.h"

unsigned r300_buffer_is_referenced(struct pipe_context *context,
				   struct pipe_resource *buf,
                                   enum r300_reference_domain domain)
{
    struct r300_context *r300 = r300_context(context);
    struct r300_buffer *rbuf = r300_buffer(buf);

    if (r300_is_user_buffer(buf))
 	return PIPE_UNREFERENCED;

    if (r300->rws->cs_is_buffer_referenced(r300->cs, rbuf->cs_buf, domain))
        return PIPE_REFERENCED_FOR_READ | PIPE_REFERENCED_FOR_WRITE;

    return PIPE_UNREFERENCED;
}

static unsigned r300_buffer_is_referenced_by_cs(struct pipe_context *context,
                                                struct pipe_resource *buf,
                                                unsigned level, int layer)
{
    return r300_buffer_is_referenced(context, buf, R300_REF_CS);
}

void r300_upload_index_buffer(struct r300_context *r300,
			      struct pipe_resource **index_buffer,
			      unsigned index_size, unsigned *start,
			      unsigned count)
{
    unsigned index_offset;
    uint8_t *ptr = r300_buffer(*index_buffer)->user_buffer;
    boolean flushed;

    *index_buffer = NULL;

    u_upload_data(r300->upload_ib,
                  0, count * index_size,
                  ptr + (*start * index_size),
                  &index_offset,
                  index_buffer, &flushed);

    *start = index_offset / index_size;

    if (flushed || !r300->upload_ib_validated) {
        r300->upload_ib_validated = FALSE;
        r300->validate_buffers = TRUE;
    }
}

void r300_upload_user_buffers(struct r300_context *r300,
                              int min_index, int max_index)
{
    int i, nr = r300->velems->count;
    unsigned count = max_index + 1 - min_index;
    boolean flushed;

    for (i = 0; i < nr; i++) {
        unsigned index = r300->velems->velem[i].vertex_buffer_index;
        struct pipe_vertex_buffer *vb = &r300->vertex_buffer[index];
        struct r300_buffer *userbuf = r300_buffer(vb->buffer);

        if (userbuf && userbuf->user_buffer) {
            unsigned first, size;

            if (vb->stride) {
                first = vb->stride * min_index;
                size = vb->stride * count;
            } else {
                first = 0;
                size = r300->velems->hw_format_size[i];
            }

            u_upload_data(r300->upload_vb, first, size,
                          userbuf->user_buffer + first,
                          &vb->buffer_offset,
                          &r300->valid_vertex_buffer[index],
                          &flushed);

            vb->buffer_offset -= first;

            r300->vertex_arrays_dirty = TRUE;

            if (flushed || !r300->upload_vb_validated) {
                r300->upload_vb_validated = FALSE;
                r300->validate_buffers = TRUE;
            }
        } else {
            assert(r300->valid_vertex_buffer[index]);
        }
    }
}

static void r300_buffer_destroy(struct pipe_screen *screen,
				struct pipe_resource *buf)
{
    struct r300_screen *r300screen = r300_screen(screen);
    struct r300_buffer *rbuf = r300_buffer(buf);
    struct r300_winsys_screen *rws = r300screen->rws;

    if (rbuf->constant_buffer)
        FREE(rbuf->constant_buffer);

    if (rbuf->buf)
        rws->buffer_reference(rws, &rbuf->buf, NULL);

    util_slab_free(&r300screen->pool_buffers, rbuf);
}

static struct pipe_transfer*
r300_buffer_get_transfer(struct pipe_context *context,
                         struct pipe_resource *resource,
                         unsigned level,
                         unsigned usage,
                         const struct pipe_box *box)
{
   struct r300_context *r300 = r300_context(context);
   struct pipe_transfer *transfer =
         util_slab_alloc(&r300->pool_transfers);

   transfer->resource = resource;
   transfer->level = level;
   transfer->usage = usage;
   transfer->box = *box;
   transfer->stride = 0;
   transfer->layer_stride = 0;
   transfer->data = NULL;

   /* Note strides are zero, this is ok for buffers, but not for
    * textures 2d & higher at least.
    */
   return transfer;
}

static void r300_buffer_transfer_destroy(struct pipe_context *pipe,
                                         struct pipe_transfer *transfer)
{
   struct r300_context *r300 = r300_context(pipe);
   util_slab_free(&r300->pool_transfers, transfer);
}

static void *
r300_buffer_transfer_map( struct pipe_context *pipe,
			  struct pipe_transfer *transfer )
{
    struct r300_context *r300 = r300_context(pipe);
    struct r300_screen *r300screen = r300_screen(pipe->screen);
    struct r300_winsys_screen *rws = r300screen->rws;
    struct r300_buffer *rbuf = r300_buffer(transfer->resource);
    uint8_t *map;

    if (rbuf->user_buffer)
        return (uint8_t *) rbuf->user_buffer + transfer->box.x;
    if (rbuf->constant_buffer)
        return (uint8_t *) rbuf->constant_buffer + transfer->box.x;

    map = rws->buffer_map(rws, rbuf->buf, r300->cs, transfer->usage);

    if (map == NULL)
        return NULL;

    return map + transfer->box.x;
}

static void r300_buffer_transfer_flush_region( struct pipe_context *pipe,
					       struct pipe_transfer *transfer,
					       const struct pipe_box *box)
{
    /* no-op */
}

static void r300_buffer_transfer_unmap( struct pipe_context *pipe,
			    struct pipe_transfer *transfer )
{
    struct r300_screen *r300screen = r300_screen(pipe->screen);
    struct r300_winsys_screen *rws = r300screen->rws;
    struct r300_buffer *rbuf = r300_buffer(transfer->resource);

    if (rbuf->buf) {
        rws->buffer_unmap(rws, rbuf->buf);
    }
}

static void r300_buffer_transfer_inline_write(struct pipe_context *pipe,
                                              struct pipe_resource *resource,
                                              unsigned level,
                                              unsigned usage,
                                              const struct pipe_box *box,
                                              const void *data,
                                              unsigned stride,
                                              unsigned layer_stride)
{
    struct r300_context *r300 = r300_context(pipe);
    struct r300_winsys_screen *rws = r300->screen->rws;
    struct r300_buffer *rbuf = r300_buffer(resource);
    uint8_t *map = NULL;

    if (rbuf->constant_buffer) {
        memcpy(rbuf->constant_buffer + box->x, data, box->width);
        return;
    }
    assert(rbuf->user_buffer == NULL);

    map = rws->buffer_map(rws, rbuf->buf, r300->cs,
                          PIPE_TRANSFER_WRITE | PIPE_TRANSFER_DISCARD | usage);

    memcpy(map + box->x, data, box->width);

    rws->buffer_unmap(rws, rbuf->buf);
}

struct u_resource_vtbl r300_buffer_vtbl =
{
   u_default_resource_get_handle,      /* get_handle */
   r300_buffer_destroy,                /* resource_destroy */
   r300_buffer_is_referenced_by_cs,    /* is_buffer_referenced */
   r300_buffer_get_transfer,           /* get_transfer */
   r300_buffer_transfer_destroy,       /* transfer_destroy */
   r300_buffer_transfer_map,           /* transfer_map */
   r300_buffer_transfer_flush_region,  /* transfer_flush_region */
   r300_buffer_transfer_unmap,         /* transfer_unmap */
   r300_buffer_transfer_inline_write   /* transfer_inline_write */
};

struct pipe_resource *r300_buffer_create(struct pipe_screen *screen,
					 const struct pipe_resource *templ)
{
    struct r300_screen *r300screen = r300_screen(screen);
    struct r300_buffer *rbuf;
    unsigned alignment = 16;

    rbuf = util_slab_alloc(&r300screen->pool_buffers);

    rbuf->magic = R300_BUFFER_MAGIC;

    rbuf->b.b = *templ;
    rbuf->b.vtbl = &r300_buffer_vtbl;
    pipe_reference_init(&rbuf->b.b.reference, 1);
    rbuf->b.b.screen = screen;
    rbuf->domain = R300_DOMAIN_GTT;
    rbuf->buf = NULL;
    rbuf->constant_buffer = NULL;
    rbuf->user_buffer = NULL;

    /* Alloc constant buffers in RAM. */
    if (templ->bind & PIPE_BIND_CONSTANT_BUFFER) {
        rbuf->constant_buffer = MALLOC(templ->width0);
        return &rbuf->b.b;
    }

    rbuf->buf =
        r300screen->rws->buffer_create(r300screen->rws,
                                       rbuf->b.b.width0, alignment,
                                       rbuf->b.b.bind, rbuf->b.b.usage,
                                       rbuf->domain);
    rbuf->cs_buf =
        r300screen->rws->buffer_get_cs_handle(r300screen->rws, rbuf->buf);

    if (!rbuf->buf) {
        util_slab_free(&r300screen->pool_buffers, rbuf);
        return NULL;
    }

    return &rbuf->b.b;
}

struct pipe_resource *r300_user_buffer_create(struct pipe_screen *screen,
					      void *ptr, unsigned size,
					      unsigned bind)
{
    struct r300_screen *r300screen = r300_screen(screen);
    struct r300_buffer *rbuf;

    rbuf = util_slab_alloc(&r300screen->pool_buffers);

    rbuf->magic = R300_BUFFER_MAGIC;

    pipe_reference_init(&rbuf->b.b.reference, 1);
    rbuf->b.vtbl = &r300_buffer_vtbl;
    rbuf->b.b.screen = screen;
    rbuf->b.b.target = PIPE_BUFFER;
    rbuf->b.b.format = PIPE_FORMAT_R8_UNORM;
    rbuf->b.b.usage = PIPE_USAGE_IMMUTABLE;
    rbuf->b.b.bind = bind;
    rbuf->b.b.width0 = ~0;
    rbuf->b.b.height0 = 1;
    rbuf->b.b.depth0 = 1;
    rbuf->b.b.array_size = 1;
    rbuf->b.b.flags = 0;
    rbuf->domain = R300_DOMAIN_GTT;
    rbuf->buf = NULL;
    rbuf->constant_buffer = NULL;
    rbuf->user_buffer = ptr;
    return &rbuf->b.b;
}

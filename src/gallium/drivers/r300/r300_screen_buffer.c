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
			      unsigned count, uint8_t *ptr)
{
    unsigned index_offset;
    boolean flushed;

    *index_buffer = NULL;

    u_upload_data(r300->vbuf_mgr->uploader,
                  0, count * index_size,
                  ptr + (*start * index_size),
                  &index_offset,
                  index_buffer, &flushed);

    *start = index_offset / index_size;
}

static void r300_buffer_destroy(struct pipe_screen *screen,
				struct pipe_resource *buf)
{
    struct r300_screen *r300screen = r300_screen(screen);
    struct r300_resource *rbuf = r300_resource(buf);

    if (rbuf->constant_buffer)
        FREE(rbuf->constant_buffer);

    if (rbuf->buf)
        pb_reference(&rbuf->buf, NULL);

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
    struct radeon_winsys *rws = r300screen->rws;
    struct r300_resource *rbuf = r300_resource(transfer->resource);
    uint8_t *map;

    if (rbuf->b.user_ptr)
        return (uint8_t *) rbuf->b.user_ptr + transfer->box.x;
    if (rbuf->constant_buffer)
        return (uint8_t *) rbuf->constant_buffer + transfer->box.x;

    map = rws->buffer_map(rbuf->buf, r300->cs, transfer->usage);

    if (map == NULL)
        return NULL;

    return map + transfer->box.x;
}

static void r300_buffer_transfer_unmap( struct pipe_context *pipe,
			    struct pipe_transfer *transfer )
{
    struct r300_screen *r300screen = r300_screen(pipe->screen);
    struct radeon_winsys *rws = r300screen->rws;
    struct r300_resource *rbuf = r300_resource(transfer->resource);

    if (rbuf->buf) {
        rws->buffer_unmap(rbuf->buf);
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
    struct radeon_winsys *rws = r300->screen->rws;
    struct r300_resource *rbuf = r300_resource(resource);
    uint8_t *map = NULL;

    if (rbuf->constant_buffer) {
        memcpy(rbuf->constant_buffer + box->x, data, box->width);
        return;
    }
    assert(rbuf->b.user_ptr == NULL);

    map = rws->buffer_map(rbuf->buf, r300->cs,
                          PIPE_TRANSFER_WRITE | PIPE_TRANSFER_DISCARD | usage);

    memcpy(map + box->x, data, box->width);

    rws->buffer_unmap(rbuf->buf);
}

static const struct u_resource_vtbl r300_buffer_vtbl =
{
   NULL,                               /* get_handle */
   r300_buffer_destroy,                /* resource_destroy */
   r300_buffer_get_transfer,           /* get_transfer */
   r300_buffer_transfer_destroy,       /* transfer_destroy */
   r300_buffer_transfer_map,           /* transfer_map */
   NULL,                               /* transfer_flush_region */
   r300_buffer_transfer_unmap,         /* transfer_unmap */
   r300_buffer_transfer_inline_write   /* transfer_inline_write */
};

struct pipe_resource *r300_buffer_create(struct pipe_screen *screen,
					 const struct pipe_resource *templ)
{
    struct r300_screen *r300screen = r300_screen(screen);
    struct r300_resource *rbuf;
    unsigned alignment = 16;

    rbuf = util_slab_alloc(&r300screen->pool_buffers);

    rbuf->b.b.b = *templ;
    rbuf->b.b.vtbl = &r300_buffer_vtbl;
    pipe_reference_init(&rbuf->b.b.b.reference, 1);
    rbuf->b.b.b.screen = screen;
    rbuf->b.user_ptr = NULL;
    rbuf->domain = RADEON_DOMAIN_GTT;
    rbuf->buf = NULL;
    rbuf->buf_size = templ->width0;
    rbuf->constant_buffer = NULL;

    /* Alloc constant buffers in RAM. */
    if (templ->bind & PIPE_BIND_CONSTANT_BUFFER) {
        rbuf->constant_buffer = MALLOC(templ->width0);
        return &rbuf->b.b.b;
    }

    rbuf->buf =
        r300screen->rws->buffer_create(r300screen->rws,
                                       rbuf->b.b.b.width0, alignment,
                                       rbuf->b.b.b.bind, rbuf->b.b.b.usage,
                                       rbuf->domain);
    if (!rbuf->buf) {
        util_slab_free(&r300screen->pool_buffers, rbuf);
        return NULL;
    }

    rbuf->cs_buf =
        r300screen->rws->buffer_get_cs_handle(rbuf->buf);

    return &rbuf->b.b.b;
}

struct pipe_resource *r300_user_buffer_create(struct pipe_screen *screen,
					      void *ptr, unsigned size,
					      unsigned bind)
{
    struct r300_screen *r300screen = r300_screen(screen);
    struct r300_resource *rbuf;

    rbuf = util_slab_alloc(&r300screen->pool_buffers);

    pipe_reference_init(&rbuf->b.b.b.reference, 1);
    rbuf->b.b.b.screen = screen;
    rbuf->b.b.b.target = PIPE_BUFFER;
    rbuf->b.b.b.format = PIPE_FORMAT_R8_UNORM;
    rbuf->b.b.b.usage = PIPE_USAGE_IMMUTABLE;
    rbuf->b.b.b.bind = bind;
    rbuf->b.b.b.width0 = ~0;
    rbuf->b.b.b.height0 = 1;
    rbuf->b.b.b.depth0 = 1;
    rbuf->b.b.b.array_size = 1;
    rbuf->b.b.b.flags = 0;
    rbuf->b.b.vtbl = &r300_buffer_vtbl;
    rbuf->b.user_ptr = ptr;
    rbuf->domain = RADEON_DOMAIN_GTT;
    rbuf->buf = NULL;
    rbuf->buf_size = size;
    rbuf->constant_buffer = NULL;
    return &rbuf->b.b.b;
}

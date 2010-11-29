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

    if (r300_buffer_is_user_buffer(buf))
 	return PIPE_UNREFERENCED;

    if (r300->rws->cs_is_buffer_referenced(r300->cs, rbuf->buf, domain))
        return PIPE_REFERENCED_FOR_READ | PIPE_REFERENCED_FOR_WRITE;

    return PIPE_UNREFERENCED;
}

static unsigned r300_buffer_is_referenced_by_cs(struct pipe_context *context,
                                                struct pipe_resource *buf,
                                                unsigned face, unsigned level)
{
    return r300_buffer_is_referenced(context, buf, R300_REF_CS);
}

/* External helper, not required to implent u_resource_vtbl:
 */
int r300_upload_index_buffer(struct r300_context *r300,
			     struct pipe_resource **index_buffer,
			     unsigned index_size,
			     unsigned start,
			     unsigned count,
			     unsigned *out_offset)
{
   struct pipe_resource *upload_buffer = NULL;
   unsigned index_offset = start * index_size;
   int ret = 0;

    if (r300_buffer_is_user_buffer(*index_buffer)) {
	ret = u_upload_buffer(r300->upload_ib,
			      index_offset,
			      count * index_size,
			      *index_buffer,
			      &index_offset,
			      &upload_buffer);
	if (ret) {
	    goto done;
	}
	*index_buffer = upload_buffer;
	*out_offset = index_offset / index_size;
    } else
        *out_offset = start;

 done:
    //    if (upload_buffer)
    //	pipe_resource_reference(&upload_buffer, NULL);
    return ret;
}

/* External helper, not required to implement u_resource_vtbl:
 */
int r300_upload_user_buffers(struct r300_context *r300)
{
    enum pipe_error ret = PIPE_OK;
    int i, nr;

    nr = r300->velems->count;

    for (i = 0; i < nr; i++) {
        struct pipe_vertex_buffer *vb =
            &r300->vertex_buffer[r300->velems->velem[i].vertex_buffer_index];

        if (r300_buffer_is_user_buffer(vb->buffer)) {
            struct pipe_resource *upload_buffer = NULL;
            unsigned offset = 0; /*vb->buffer_offset * 4;*/
            unsigned size = vb->buffer->width0;
            unsigned upload_offset;
            ret = u_upload_buffer(r300->upload_vb,
                                  offset, size,
                                  vb->buffer,
                                  &upload_offset, &upload_buffer);
            if (ret)
                return ret;

            pipe_resource_reference(&vb->buffer, NULL);
            vb->buffer = upload_buffer;
            vb->buffer_offset = upload_offset;
        }
    }
    return ret;
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
r300_default_get_transfer(struct pipe_context *context,
                          struct pipe_resource *resource,
                          struct pipe_subresource sr,
                          unsigned usage,
                          const struct pipe_box *box)
{
   struct r300_context *r300 = r300_context(context);
   struct pipe_transfer *transfer =
         util_slab_alloc(&r300->pool_transfers);

   transfer->resource = resource;
   transfer->sr = sr;
   transfer->usage = usage;
   transfer->box = *box;
   transfer->stride = 0;
   transfer->slice_stride = 0;
   transfer->data = NULL;

   /* Note strides are zero, this is ok for buffers, but not for
    * textures 2d & higher at least.
    */
   return transfer;
}

static void r300_default_transfer_destroy(struct pipe_context *pipe,
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
    boolean flush = FALSE;
    unsigned i;

    if (rbuf->user_buffer)
        return (uint8_t *) rbuf->user_buffer + transfer->box.x;
    if (rbuf->constant_buffer)
        return (uint8_t *) rbuf->constant_buffer + transfer->box.x;

    /* check if the mapping is to a range we already flushed */
    if (transfer->usage & PIPE_TRANSFER_DISCARD) {
	for (i = 0; i < rbuf->num_ranges; i++) {
	    if ((transfer->box.x >= rbuf->ranges[i].start) &&
		(transfer->box.x < rbuf->ranges[i].end))
		flush = TRUE;

	    if (flush) {
		/* unreference this hw buffer and allocate a new one */
		rws->buffer_reference(rws, &rbuf->buf, NULL);

		rbuf->num_ranges = 0;
                rbuf->buf =
                    r300screen->rws->buffer_create(r300screen->rws,
                                                   rbuf->b.b.width0, 16,
                                                   rbuf->b.b.bind,
                                                   rbuf->b.b.usage,
                                                   rbuf->domain);
		break;
	    }
	}
    }

    map = rws->buffer_map(rws, rbuf->buf, r300->cs, transfer->usage);

    if (map == NULL)
        return NULL;

    /* map_buffer() returned a pointer to the beginning of the buffer,
     * but transfers are expected to return a pointer to just the
     * region specified in the box.
     */
    return map + transfer->box.x;
}

static void r300_buffer_transfer_flush_region( struct pipe_context *pipe,
					       struct pipe_transfer *transfer,
					       const struct pipe_box *box)
{
    struct r300_buffer *rbuf = r300_buffer(transfer->resource);
    unsigned i;
    unsigned offset = transfer->box.x + box->x;
    unsigned length = box->width;

    assert(box->x + box->width <= transfer->box.width);

    if (rbuf->user_buffer)
	return;
    if (rbuf->constant_buffer)
        return;

    /* mark the range as used */
    for(i = 0; i < rbuf->num_ranges; ++i) {
	if(offset <= rbuf->ranges[i].end && rbuf->ranges[i].start <= (offset+box->width)) {
	    rbuf->ranges[i].start = MIN2(rbuf->ranges[i].start, offset);
	    rbuf->ranges[i].end   = MAX2(rbuf->ranges[i].end, (offset+length));
	    return;
	}
    }

    rbuf->ranges[rbuf->num_ranges].start = offset;
    rbuf->ranges[rbuf->num_ranges].end = offset+length;
    rbuf->num_ranges++;
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

struct u_resource_vtbl r300_buffer_vtbl = 
{
   u_default_resource_get_handle,      /* get_handle */
   r300_buffer_destroy,                /* resource_destroy */
   r300_buffer_is_referenced_by_cs,    /* is_buffer_referenced */
   r300_default_get_transfer,          /* get_transfer */
   r300_default_transfer_destroy,      /* transfer_destroy */
   r300_buffer_transfer_map,           /* transfer_map */
   r300_buffer_transfer_flush_region,  /* transfer_flush_region */
   r300_buffer_transfer_unmap,         /* transfer_unmap */
   u_default_transfer_inline_write     /* transfer_inline_write */
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
    rbuf->num_ranges = 0;
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

    if (!rbuf->buf) {
        util_slab_free(&r300screen->pool_buffers, rbuf);
        return NULL;
    }

    return &rbuf->b.b;
}

struct pipe_resource *r300_user_buffer_create(struct pipe_screen *screen,
					      void *ptr,
					      unsigned bytes,
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
    rbuf->b.b.width0 = bytes;
    rbuf->b.b.height0 = 1;
    rbuf->b.b.depth0 = 1;
    rbuf->b.b.flags = 0;
    rbuf->domain = R300_DOMAIN_GTT;
    rbuf->num_ranges = 0;
    rbuf->buf = NULL;
    rbuf->constant_buffer = NULL;
    rbuf->user_buffer = ptr;
    return &rbuf->b.b;
}

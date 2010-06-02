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

    if (r300->rws->is_buffer_referenced(r300->rws, rbuf->buf, domain))
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
			     unsigned count)
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
    }
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

    nr = r300->vertex_buffer_count;

    for (i = 0; i < nr; i++) {
	if (r300_buffer_is_user_buffer(r300->vertex_buffer[i].buffer)) {
	    struct pipe_resource *upload_buffer = NULL;
	    unsigned offset = 0; /*r300->vertex_buffer[i].buffer_offset * 4;*/
	    unsigned size = r300->vertex_buffer[i].buffer->width0;
	    unsigned upload_offset;
	    ret = u_upload_buffer(r300->upload_vb,
				  offset, size,
				  r300->vertex_buffer[i].buffer,
				  &upload_offset, &upload_buffer);
	    if (ret)
		return ret;

	    pipe_resource_reference(&r300->vertex_buffer[i].buffer, NULL);
	    r300->vertex_buffer[i].buffer = upload_buffer;
	    r300->vertex_buffer[i].buffer_offset = upload_offset;
	}
    }
    return ret;
}

static void r300_winsys_buffer_destroy(struct r300_screen *r300screen,
				       struct r300_buffer *rbuf)
{
    struct r300_winsys_screen *rws = r300screen->rws;

    if (rbuf->buf) {
	rws->buffer_reference(rws, &rbuf->buf, NULL);
	rbuf->buf = NULL;
    }
}

static void r300_buffer_destroy(struct pipe_screen *screen,
				struct pipe_resource *buf)
{
    struct r300_screen *r300screen = r300_screen(screen);
    struct r300_buffer *rbuf = r300_buffer(buf);

    r300_winsys_buffer_destroy(r300screen, rbuf);
    FREE(rbuf);
}

static void *
r300_buffer_transfer_map( struct pipe_context *pipe,
			  struct pipe_transfer *transfer )
{
    struct r300_screen *r300screen = r300_screen(pipe->screen);
    struct r300_winsys_screen *rws = r300screen->rws;
    struct r300_buffer *rbuf = r300_buffer(transfer->resource);
    uint8_t *map;
    boolean flush = FALSE;
    unsigned i;

    if (rbuf->user_buffer)
        return (uint8_t *) rbuf->user_buffer + transfer->box.x;

    if (rbuf->b.b.bind & PIPE_BIND_CONSTANT_BUFFER) {
	goto just_map;
    }

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
		rbuf->buf = r300screen->rws->buffer_create(r300screen->rws, 16,
						      rbuf->b.b.bind,
                                                      rbuf->domain,
						      rbuf->b.b.width0);
		break;
	    }
	}
    }
just_map:
    map = rws->buffer_map(rws, rbuf->buf, transfer->usage);

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

    if (rbuf->b.b.bind & PIPE_BIND_CONSTANT_BUFFER)
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
   r300_buffer_destroy,		     /* resource_destroy */
   r300_buffer_is_referenced_by_cs,  /* is_buffer_referenced */
   u_default_get_transfer,	     /* get_transfer */
   u_default_transfer_destroy,	     /* transfer_destroy */
   r300_buffer_transfer_map,	     /* transfer_map */
   r300_buffer_transfer_flush_region,  /* transfer_flush_region */
   r300_buffer_transfer_unmap,	     /* transfer_unmap */
   u_default_transfer_inline_write   /* transfer_inline_write */
};

struct pipe_resource *r300_buffer_create(struct pipe_screen *screen,
					 const struct pipe_resource *templ)
{
    struct r300_screen *r300screen = r300_screen(screen);
    struct r300_buffer *rbuf;
    unsigned alignment = 16;

    rbuf = CALLOC_STRUCT(r300_buffer);
    if (!rbuf)
	goto error1;

    rbuf->magic = R300_BUFFER_MAGIC;

    rbuf->b.b = *templ;
    rbuf->b.vtbl = &r300_buffer_vtbl;
    pipe_reference_init(&rbuf->b.b.reference, 1);
    rbuf->b.b.screen = screen;

    if (rbuf->b.b.bind & R300_BIND_OQBO)
        alignment = 4096;

    rbuf->domain = R300_DOMAIN_GTT;

    rbuf->buf = r300screen->rws->buffer_create(r300screen->rws,
					  alignment,
					  rbuf->b.b.bind,
                                          rbuf->domain,
					  rbuf->b.b.width0);

    if (!rbuf->buf)
	goto error2;

    return &rbuf->b.b;
error2:
    FREE(rbuf);
error1:
    return NULL;
}

struct pipe_resource *r300_user_buffer_create(struct pipe_screen *screen,
					      void *ptr,
					      unsigned bytes,
					      unsigned bind)
{
    struct r300_buffer *rbuf;

    rbuf = CALLOC_STRUCT(r300_buffer);
    if (!rbuf)
	goto no_rbuf;

    rbuf->magic = R300_BUFFER_MAGIC;

    pipe_reference_init(&rbuf->b.b.reference, 1);
    rbuf->b.vtbl = &r300_buffer_vtbl;
    rbuf->b.b.screen = screen;
    rbuf->b.b.format = PIPE_FORMAT_R8_UNORM;
    rbuf->b.b.usage = PIPE_USAGE_IMMUTABLE;
    rbuf->b.b.bind = bind;
    rbuf->b.b.width0 = bytes;
    rbuf->b.b.height0 = 1;
    rbuf->b.b.depth0 = 1;
    rbuf->domain = R300_DOMAIN_GTT;

    rbuf->user_buffer = ptr;
    return &rbuf->b.b;

no_rbuf:
    return NULL;
}

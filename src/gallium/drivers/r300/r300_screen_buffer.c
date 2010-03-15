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
#include "util/u_format.h"
#include "util/u_memory.h"
#include "util/u_upload_mgr.h"
#include "util/u_math.h"

#include "r300_screen_buffer.h"

#include "r300_winsys.h"

boolean r300_buffer_is_referenced(struct r300_context *r300,
				  struct pipe_buffer *buf)
{
    struct r300_buffer *rbuf = r300_buffer(buf);
    if (r300_buffer_is_user_buffer(buf))
	return FALSE;

    return r300->rws->is_buffer_referenced(r300->rws, rbuf->buf);
    
}
int r300_upload_index_buffer(struct r300_context *r300,
			     struct pipe_buffer **index_buffer,
			     unsigned index_size,
			     unsigned start,
			     unsigned count)
{
   struct pipe_buffer *upload_buffer = NULL;
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
    //	pipe_buffer_reference(&upload_buffer, NULL);
    return ret;
}

int r300_upload_user_buffers(struct r300_context *r300)
{
    enum pipe_error ret = PIPE_OK;
    int i, nr;

    nr = r300->vertex_buffer_count;

    for (i = 0; i < nr; i++) {

	if (r300_buffer_is_user_buffer(r300->vertex_buffer[i].buffer)) {
	    struct pipe_buffer *upload_buffer = NULL;
	    unsigned offset = 0; /*r300->vertex_buffer[i].buffer_offset * 4;*/
	    unsigned size = r300->vertex_buffer[i].buffer->size;
	    unsigned upload_offset;
	    ret = u_upload_buffer(r300->upload_vb,
				  offset, size,
				  r300->vertex_buffer[i].buffer,
				  &upload_offset, &upload_buffer);
	    if (ret)
		return ret;

	    pipe_buffer_reference(&r300->vertex_buffer[i].buffer, NULL);
	    r300->vertex_buffer[i].buffer = upload_buffer;
	    r300->vertex_buffer[i].buffer_offset = upload_offset;
	}
    }
    return ret;
}

static struct r300_winsys_buffer *
r300_winsys_buffer_create(struct r300_screen *r300screen,
			  unsigned alignment,
			  unsigned usage,
			  unsigned size)
{
    struct r300_winsys_screen *rws = r300screen->rws;
    struct r300_winsys_buffer *buf;

    buf = rws->buffer_create(rws, alignment, usage, size);
    return buf;
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

static struct pipe_buffer *r300_buffer_create(struct pipe_screen *screen,
					      unsigned alignment,
					      unsigned usage,
					      unsigned size)
{
    struct r300_screen *r300screen = r300_screen(screen);
    struct r300_buffer *rbuf;

    rbuf = CALLOC_STRUCT(r300_buffer);
    if (!rbuf)
	goto error1;

    rbuf->magic = R300_BUFFER_MAGIC;

    pipe_reference_init(&rbuf->base.reference, 1);
    rbuf->base.screen = screen;
    rbuf->base.alignment = alignment;
    rbuf->base.usage = usage;
    rbuf->base.size = size;

    rbuf->buf = r300_winsys_buffer_create(r300screen,
					  alignment,
					  usage,
					  size);

    if (!rbuf->buf)
	goto error2;

    return &rbuf->base;
error2:
    FREE(rbuf);
error1:
    return NULL;
}


static struct pipe_buffer *r300_user_buffer_create(struct pipe_screen *screen,
						   void *ptr,
						   unsigned bytes)
{
    struct r300_buffer *rbuf;

    rbuf = CALLOC_STRUCT(r300_buffer);
    if (!rbuf)
	goto no_rbuf;

    rbuf->magic = R300_BUFFER_MAGIC;

    pipe_reference_init(&rbuf->base.reference, 1);
    rbuf->base.screen = screen;
    rbuf->base.alignment = 1;
    rbuf->base.usage = 0;
    rbuf->base.size = bytes;

    rbuf->user_buffer = ptr;
    return &rbuf->base;

no_rbuf:
    return NULL;
}

static void r300_buffer_destroy(struct pipe_buffer *buf)
{
    struct r300_screen *r300screen = r300_screen(buf->screen);
    struct r300_buffer *rbuf = r300_buffer(buf);

    r300_winsys_buffer_destroy(r300screen, rbuf);
    FREE(rbuf);
}

static void *
r300_buffer_map_range(struct pipe_screen *screen,
		      struct pipe_buffer *buf,
		      unsigned offset, unsigned length,
		      unsigned usage )
{
    struct r300_screen *r300screen = r300_screen(screen);
    struct r300_winsys_screen *rws = r300screen->rws;
    struct r300_buffer *rbuf = r300_buffer(buf);
    void *map;
    int flush = 0;
    int i;

    if (rbuf->user_buffer)
	return rbuf->user_buffer;

    if (rbuf->base.usage & PIPE_BUFFER_USAGE_CONSTANT)
	goto just_map;

    /* check if the mapping is to a range we already flushed */
    if (usage & PIPE_BUFFER_USAGE_DISCARD) {
	for (i = 0; i < rbuf->num_ranges; i++) {

	    if ((offset >= rbuf->ranges[i].start) &&
		(offset < rbuf->ranges[i].end))
		flush = 1;
	    
	    if (flush) {
		/* unreference this hw buffer and allocate a new one */
		rws->buffer_reference(rws, &rbuf->buf, NULL);

		rbuf->num_ranges = 0;
		rbuf->map = NULL;
		rbuf->buf = r300_winsys_buffer_create(r300screen,
						      rbuf->base.alignment,
						      rbuf->base.usage,
						      rbuf->base.size);
		break;
	    }
	}
    }
just_map:
    map = rws->buffer_map(rws, rbuf->buf, usage | R300_USAGE_FLAG_DONT_SYNC);
   
    return map;
}

static void 
r300_buffer_flush_mapped_range( struct pipe_screen *screen,
				struct pipe_buffer *buf,
				unsigned offset,
				unsigned length )
{
    struct r300_buffer *rbuf = r300_buffer(buf);
    int i;

    if (rbuf->user_buffer)
	return;

    if (rbuf->base.usage & PIPE_BUFFER_USAGE_CONSTANT)
	return;

    /* mark the range as used */
    for(i = 0; i < rbuf->num_ranges; ++i) {
	if(offset <= rbuf->ranges[i].end && rbuf->ranges[i].start <= (offset+length)) {
	    rbuf->ranges[i].start = MIN2(rbuf->ranges[i].start, offset);
	    rbuf->ranges[i].end   = MAX2(rbuf->ranges[i].end, (offset+length));
	    return;
	}
    }

    rbuf->ranges[rbuf->num_ranges].start = offset;
    rbuf->ranges[rbuf->num_ranges].end = offset+length;
    rbuf->num_ranges++;
}

static void *
r300_buffer_map(struct pipe_screen *screen,
		struct pipe_buffer *buf,
		unsigned usage)
{
    struct r300_screen *r300screen = r300_screen(screen);
    struct r300_winsys_screen *rws = r300screen->rws;
    struct r300_buffer *rbuf = r300_buffer(buf);
    void *map;

   if (rbuf->user_buffer)
      return rbuf->user_buffer;

    map = rws->buffer_map(rws, rbuf->buf, usage);

    return map;
}

static void
r300_buffer_unmap(struct pipe_screen *screen,
		  struct pipe_buffer *buf)
{
    struct r300_screen *r300screen = r300_screen(screen);
    struct r300_winsys_screen *rws = r300screen->rws;
    struct r300_buffer *rbuf = r300_buffer(buf);

    if (rbuf->buf) {
        rws->buffer_unmap(rws, rbuf->buf);
    }
}

void r300_screen_init_buffer_functions(struct r300_screen *r300screen)
{
    r300screen->screen.buffer_create = r300_buffer_create;
    r300screen->screen.user_buffer_create = r300_user_buffer_create;
    r300screen->screen.buffer_map = r300_buffer_map;
    r300screen->screen.buffer_map_range = r300_buffer_map_range;
    r300screen->screen.buffer_flush_mapped_range = r300_buffer_flush_mapped_range;
    r300screen->screen.buffer_unmap = r300_buffer_unmap;
    r300screen->screen.buffer_destroy = r300_buffer_destroy;
}

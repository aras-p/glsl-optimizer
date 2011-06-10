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

#include <util/u_index_modify.h>
#include "util/u_inlines.h"
#include "util/u_upload_mgr.h"
#include "r600_pipe.h"


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
		u_upload_alloc(r600->vbuf_mgr->uploader, 0, count * 2,
			       &out_offset, &out_buffer, &flushed, &ptr);

		util_shorten_ubyte_elts_to_userptr(
				&r600->context, *index_buffer, 0, *start, count, ptr);

		pipe_resource_reference(index_buffer, out_buffer);
		pipe_resource_reference(&out_buffer, NULL);
		*index_size = 2;
		*start = out_offset / 2;
		break;
	}
}

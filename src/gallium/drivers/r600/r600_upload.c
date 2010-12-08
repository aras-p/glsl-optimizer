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
 *      Jerome Glisse <jglisse@redhat.com>
 */
#include <errno.h>
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "r600.h"
#include "r600_pipe.h"
#include "r600_resource.h"

struct r600_upload {
	struct r600_pipe_context	*rctx;
	struct r600_bo			*buffer;
	char				*ptr;
	unsigned			size;
	unsigned			default_size;
	unsigned			total_alloc_size;
	unsigned			offset;
	unsigned			alignment;
};

struct r600_upload *r600_upload_create(struct r600_pipe_context *rctx,
					unsigned default_size,
					unsigned alignment)
{
	struct r600_upload *upload = CALLOC_STRUCT(r600_upload);

	if (upload == NULL)
		return NULL;

	upload->rctx = rctx;
	upload->size = 0;
	upload->default_size = default_size;
	upload->alignment = alignment;
	upload->ptr = NULL;
	upload->buffer = NULL;
	upload->total_alloc_size = 0;

	return upload;
}

void r600_upload_flush(struct r600_upload *upload)
{
	if (upload->buffer) {
		r600_bo_reference(upload->rctx->radeon, &upload->buffer, NULL);
	}
	upload->default_size = MAX2(upload->total_alloc_size, upload->default_size);
	upload->total_alloc_size = 0;
	upload->size = 0;
	upload->ptr = NULL;
	upload->buffer = NULL;
}

void r600_upload_destroy(struct r600_upload *upload)
{
	r600_upload_flush(upload);
	FREE(upload);
}

int r600_upload_buffer(struct r600_upload *upload, unsigned offset,
			unsigned size, struct r600_resource_buffer *in_buffer,
			unsigned *out_offset, unsigned *out_size,
			struct r600_bo **out_buffer)
{
	unsigned alloc_size = align(size, upload->alignment);
	const void *in_ptr = NULL;

	if (upload->offset + alloc_size > upload->size) {
		if (upload->size) {
			r600_bo_reference(upload->rctx->radeon, &upload->buffer, NULL);
		}
		upload->size = align(MAX2(upload->default_size, alloc_size), 4096);
		upload->total_alloc_size += upload->size;
		upload->offset = 0;
		upload->buffer = r600_bo(upload->rctx->radeon, upload->size, 4096, PIPE_BIND_VERTEX_BUFFER, 0);
		if (upload->buffer == NULL) {
			return -ENOMEM;
		}
		upload->ptr = r600_bo_map(upload->rctx->radeon, upload->buffer, 0, NULL);
	}

	in_ptr = in_buffer->user_buffer;
	memcpy(upload->ptr + upload->offset, in_ptr + offset, size);
	*out_offset = upload->offset;
	*out_size = upload->size;
	*out_buffer = upload->buffer;
	upload->offset += alloc_size;

	return 0;
}

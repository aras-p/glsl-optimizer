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
 *      Jerome Glisse
 */
#define _FILE_OFFSET_BITS 64
#include "r600_priv.h"
#include "util/u_hash_table.h"
#include "util/u_memory.h"
#include "radeon_drm.h"
#include "xf86drm.h"
#include <sys/mman.h>
#include <errno.h>

#include "state_tracker/drm_driver.h"

struct radeon_bo *radeon_bo(struct radeon *radeon, unsigned handle,
			    unsigned size, unsigned alignment, unsigned bind,
			    unsigned initial_domain)
{
	struct radeon_bo *bo;
	struct winsys_handle whandle = {};
	whandle.handle = handle;

	bo = calloc(1, sizeof(*bo));
	if (bo == NULL) {
		return NULL;
	}
	pipe_reference_init(&bo->reference, 1);

	if (handle) {
		bo->buf = radeon->ws->buffer_from_handle(radeon->ws, &whandle, NULL, &size);
	} else {
		bo->buf = radeon->ws->buffer_create(radeon->ws, size, alignment, bind, initial_domain);
	}
	if (!bo->buf) {
		FREE(bo);
		return NULL;
	}
	bo->cs_buf = radeon->ws->buffer_get_cs_handle(bo->buf);
	bo->size = size;
	return bo;
}

static void radeon_bo_destroy(struct radeon *radeon, struct radeon_bo *bo)
{
	pb_reference(&bo->buf, NULL);
	FREE(bo);
}

void radeon_bo_reference(struct radeon *radeon,
			 struct radeon_bo **dst,
			 struct radeon_bo *src)
{
	struct radeon_bo *old = *dst;
	if (pipe_reference(&(*dst)->reference, &src->reference)) {
		radeon_bo_destroy(radeon, old);
	}
	*dst = src;
}

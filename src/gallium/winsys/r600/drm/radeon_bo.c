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

int radeon_bo_fixed_map(struct radeon *radeon, struct radeon_bo *bo)
{
	struct drm_radeon_gem_mmap args;
	void *ptr;
	int r;

	/* Zero out args to make valgrind happy */
	memset(&args, 0, sizeof(args));
	args.handle = bo->handle;
	args.offset = 0;
	args.size = (uint64_t)bo->size;
	r = drmCommandWriteRead(radeon->info.fd, DRM_RADEON_GEM_MMAP,
				&args, sizeof(args));
	if (r) {
		fprintf(stderr, "error mapping %p 0x%08X (error = %d)\n",
			bo, bo->handle, r);
		return r;
	}
	ptr = mmap(0, args.size, PROT_READ|PROT_WRITE, MAP_SHARED, radeon->info.fd, args.addr_ptr);
	if (ptr == MAP_FAILED) {
		fprintf(stderr, "%s failed to map bo\n", __func__);
		return -errno;
	}
	bo->data = ptr;

	bo->map_count++;
	return 0;
}

static void radeon_bo_fixed_unmap(struct radeon *radeon, struct radeon_bo *bo)
{
	if (bo->data) {
		munmap(bo->data, bo->size);
		bo->data = NULL;
	}
}

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
	bo->handle = radeon->ws->trans_get_buffer_handle(bo->buf);
	bo->size = size;
	return bo;
}

static void radeon_bo_destroy(struct radeon *radeon, struct radeon_bo *bo)
{
	radeon_bo_fixed_unmap(radeon, bo);
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

int radeon_bo_wait(struct radeon *radeon, struct radeon_bo *bo)
{
	struct drm_radeon_gem_wait_idle args;
	int ret;

	/* Zero out args to make valgrind happy */
	memset(&args, 0, sizeof(args));
	args.handle = bo->handle;
	do {
		ret = drmCommandWriteRead(radeon->info.fd, DRM_RADEON_GEM_WAIT_IDLE,
					&args, sizeof(args));
	} while (ret == -EBUSY);
	return ret;
}

int radeon_bo_busy(struct radeon *radeon, struct radeon_bo *bo, uint32_t *domain)
{
	struct drm_radeon_gem_busy args;
	int ret;

	memset(&args, 0, sizeof(args));
	args.handle = bo->handle;
	args.domain = 0;

	ret = drmCommandWriteRead(radeon->info.fd, DRM_RADEON_GEM_BUSY,
			&args, sizeof(args));

	*domain = args.domain;
	return ret;
}

int radeon_bo_get_tiling_flags(struct radeon *radeon,
			       struct radeon_bo *bo,
			       uint32_t *tiling_flags)
{
	struct drm_radeon_gem_get_tiling args = {};
	int ret;

	args.handle = bo->handle;
	ret = drmCommandWriteRead(radeon->info.fd, DRM_RADEON_GEM_GET_TILING,
				  &args, sizeof(args));
	if (ret)
		return ret;

	*tiling_flags = args.tiling_flags;
	return ret;
}

int radeon_bo_get_name(struct radeon *radeon,
		       struct radeon_bo *bo,
		       uint32_t *name)
{
	struct drm_gem_flink flink;
	int ret;

	flink.handle = bo->handle;
	ret = drmIoctl(radeon->info.fd, DRM_IOCTL_GEM_FLINK, &flink);
	if (ret)
		return ret;

	*name = flink.name;
	return ret;
}

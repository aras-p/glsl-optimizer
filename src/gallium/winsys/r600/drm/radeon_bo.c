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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <errno.h>
#include "r600_priv.h"
#include "xf86drm.h"
#include "radeon_drm.h"

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
	r = drmCommandWriteRead(radeon->fd, DRM_RADEON_GEM_MMAP,
				&args, sizeof(args));
	if (r) {
		fprintf(stderr, "error mapping %p 0x%08X (error = %d)\n",
			bo, bo->handle, r);
		return r;
	}
	ptr = mmap(0, args.size, PROT_READ|PROT_WRITE, MAP_SHARED, radeon->fd, args.addr_ptr);
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

struct radeon_bo *radeon_bo(struct radeon *radeon, unsigned handle,
			    unsigned size, unsigned alignment, unsigned initial_domain)
{
	struct radeon_bo *bo;
	int r;

	if (handle) {
		pipe_mutex_lock(radeon->bo_handles_mutex);
		bo = util_hash_table_get(radeon->bo_handles,
					 (void *)(uintptr_t)handle);
		if (bo) {
			struct radeon_bo *b = NULL;
			radeon_bo_reference(radeon, &b, bo);
			goto done;
		}
	}
	bo = calloc(1, sizeof(*bo));
	if (bo == NULL) {
		return NULL;
	}
	bo->size = size;
	bo->handle = handle;
	pipe_reference_init(&bo->reference, 1);
	bo->alignment = alignment;
	LIST_INITHEAD(&bo->fencedlist);

	if (handle) {
		struct drm_gem_open open_arg;

		memset(&open_arg, 0, sizeof(open_arg));
		open_arg.name = handle;
		r = drmIoctl(radeon->fd, DRM_IOCTL_GEM_OPEN, &open_arg);
		if (r != 0) {
			free(bo);
			return NULL;
		}
		bo->name = handle;
		bo->handle = open_arg.handle;
		bo->size = open_arg.size;
		bo->shared = TRUE;
	} else {
		struct drm_radeon_gem_create args = {};

		args.size = size;
		args.alignment = alignment;
		args.initial_domain = initial_domain;
		args.flags = 0;
		args.handle = 0;
		r = drmCommandWriteRead(radeon->fd, DRM_RADEON_GEM_CREATE,
					&args, sizeof(args));
		bo->handle = args.handle;
		if (r) {
			fprintf(stderr, "Failed to allocate :\n");
			fprintf(stderr, "   size      : %d bytes\n", size);
			fprintf(stderr, "   alignment : %d bytes\n", alignment);
			free(bo);
			return NULL;
		}
	}

	if (handle)
		util_hash_table_set(radeon->bo_handles, (void *)(uintptr_t)handle, bo);
done:
	if (handle)
		pipe_mutex_unlock(radeon->bo_handles_mutex);

	return bo;
}

static void radeon_bo_destroy(struct radeon *radeon, struct radeon_bo *bo)
{
	struct drm_gem_close args;

	if (bo->name) {
		pipe_mutex_lock(radeon->bo_handles_mutex);
		util_hash_table_remove(radeon->bo_handles,
				       (void *)(uintptr_t)bo->name);
		pipe_mutex_unlock(radeon->bo_handles_mutex);
	}
	LIST_DEL(&bo->fencedlist);
	radeon_bo_fixed_unmap(radeon, bo);
	memset(&args, 0, sizeof(args));
	args.handle = bo->handle;
	drmIoctl(radeon->fd, DRM_IOCTL_GEM_CLOSE, &args);
	memset(bo, 0, sizeof(struct radeon_bo));
	free(bo);
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

        if (!bo->shared) {
                if (!bo->fence)
			return 0;
		if (bo->fence <= *radeon->cfence) {
			LIST_DELINIT(&bo->fencedlist);
			bo->fence = 0;
			return 0;
		}
        }

	/* Zero out args to make valgrind happy */
	memset(&args, 0, sizeof(args));
	args.handle = bo->handle;
	do {
		ret = drmCommandWriteRead(radeon->fd, DRM_RADEON_GEM_WAIT_IDLE,
					&args, sizeof(args));
	} while (ret == -EBUSY);
	return ret;
}

int radeon_bo_busy(struct radeon *radeon, struct radeon_bo *bo, uint32_t *domain)
{
	struct drm_radeon_gem_busy args;
	int ret;

	if (!bo->shared) {
		if (!bo->fence)
			return 0;
		if (bo->fence <= *radeon->cfence) {
			LIST_DELINIT(&bo->fencedlist);
			bo->fence = 0;
			return 0;
		}
	}

	memset(&args, 0, sizeof(args));
	args.handle = bo->handle;
	args.domain = 0;

	ret = drmCommandWriteRead(radeon->fd, DRM_RADEON_GEM_BUSY,
			&args, sizeof(args));

	*domain = args.domain;
	return ret;
}

int radeon_bo_get_tiling_flags(struct radeon *radeon,
			       struct radeon_bo *bo,
			       uint32_t *tiling_flags,
			       uint32_t *pitch)
{
	struct drm_radeon_gem_get_tiling args = {};
	int ret;

	args.handle = bo->handle;
	ret = drmCommandWriteRead(radeon->fd, DRM_RADEON_GEM_GET_TILING,
				  &args, sizeof(args));
	if (ret)
		return ret;

	*tiling_flags = args.tiling_flags;
	*pitch = args.pitch;
	return ret;
}

int radeon_bo_get_name(struct radeon *radeon,
		       struct radeon_bo *bo,
		       uint32_t *name)
{
	struct drm_gem_flink flink;
	int ret;

	flink.handle = bo->handle;
	ret = drmIoctl(radeon->fd, DRM_IOCTL_GEM_FLINK, &flink);
	if (ret)
		return ret;

	*name = flink.name;
	return ret;
}

/*
 * Copyright 2007 Nouveau Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdint.h>
#include <stdlib.h>
#include <errno.h>

#include "nouveau_drmif.h"
#include "nouveau_dma.h"
#include "nouveau_local.h"

int
nouveau_bo_init(struct nouveau_device *userdev)
{
	return 0;
}

void
nouveau_bo_takedown(struct nouveau_device *userdev)
{
}

static int
nouveau_bo_realloc_gpu(struct nouveau_bo_priv *bo, uint32_t flags, int size)
{
	struct nouveau_device_priv *nv = nouveau_device(bo->base.device);
	int ret;

	if (bo->drm.size && bo->drm.size != size) {
		struct drm_nouveau_mem_free f;

		if (bo->map) {
			drmUnmap(bo->map, bo->drm.size);
			bo->map = NULL;
		}

		f.flags = bo->drm.flags;
		f.offset = bo->drm.offset;
		drmCommandWrite(nv->fd, DRM_NOUVEAU_MEM_FREE, &f, sizeof(f));

		bo->drm.size = 0;
	}

	if (size && !bo->drm.size) {
		if (flags) {
			bo->drm.flags = 0;
			if (flags & NOUVEAU_BO_VRAM)
				bo->drm.flags |= NOUVEAU_MEM_FB;
			if (flags & NOUVEAU_BO_GART)
				bo->drm.flags |= (NOUVEAU_MEM_AGP |
						  NOUVEAU_MEM_PCI);
			bo->drm.flags |= NOUVEAU_MEM_MAPPED;
		}

		bo->drm.size = size;
		
		ret = drmCommandWriteRead(nv->fd, DRM_NOUVEAU_MEM_ALLOC,
					  &bo->drm, sizeof(bo->drm));
		if (ret) {
			free(bo);
			return ret;
		}

		ret = drmMap(nv->fd, bo->drm.map_handle, bo->drm.size,
			     &bo->map);
		if (ret) {
			bo->map = NULL;
			nouveau_bo_del((void *)&bo);
			return ret;
		}
	}

	return 0;
}

int
nouveau_bo_new(struct nouveau_device *userdev, uint32_t flags, int align,
	       int size, struct nouveau_bo **userbo)
{
	struct nouveau_bo_priv *bo;
	int ret;

	if (!userdev || !userbo || *userbo)
		return -EINVAL;

	bo = calloc(1, sizeof(*bo));
	if (!bo)
		return -ENOMEM;
	bo->base.device = userdev;
	bo->drm.alignment = align;

	if (flags & NOUVEAU_BO_PIN) {
		ret = nouveau_bo_realloc_gpu(bo, flags, size);
		if (ret) {
			free(bo);
			return ret;
		}	
	} else {
		bo->sysmem = malloc(size);
		if (!bo->sysmem) {
			free(bo);
			return -ENOMEM;
		}
	}

	bo->base.size = size;
	bo->base.offset = bo->drm.offset;
	bo->base.handle = (unsigned long)bo;
	bo->refcount = 1;
	*userbo = &bo->base;
	return 0;
}

int
nouveau_bo_user(struct nouveau_device *userdev, void *ptr, int size,
		struct nouveau_bo **userbo)
{
	struct nouveau_bo_priv *bo;

	if (!userdev || !userbo || *userbo)
		return -EINVAL;

	bo = calloc(1, sizeof(*bo));
	if (!bo)
		return -ENOMEM;
	bo->base.device = userdev;
	
	bo->sysmem = ptr;
	bo->user = 1;

	bo->base.size = size;
	bo->base.offset = bo->drm.offset;
	bo->base.handle = (unsigned long)bo;
	bo->refcount = 1;
	*userbo = &bo->base;
	return 0;
}

int
nouveau_bo_ref(struct nouveau_device *userdev, uint64_t handle,
	       struct nouveau_bo **userbo)
{
	struct nouveau_bo_priv *bo = (void *)(unsigned long)handle;

	if (!userdev || !userbo || *userbo)
		return -EINVAL;

	bo->refcount++;
	*userbo = &bo->base;
	return 0;
}

int
nouveau_bo_resize(struct nouveau_bo *userbo, int size)
{
	struct nouveau_bo_priv *bo = nouveau_bo(userbo);
	int ret;

	if (!bo || bo->user)
		return -EINVAL;

	if (bo->sysmem) {
		bo->sysmem = realloc(bo->sysmem, size);
		if (!bo->sysmem)
			return -ENOMEM;
	} else {
		ret = nouveau_bo_realloc_gpu(bo, 0, size);
		if (ret)
			return ret;
	}

	bo->base.size = size;
	return 0;
}

void
nouveau_bo_del(struct nouveau_bo **userbo)
{
	struct nouveau_bo_priv *bo;

	if (!userbo || !*userbo)
		return;
	bo = nouveau_bo(*userbo);
	*userbo = NULL;

	if (--bo->refcount)
		return;

	if (bo->fence)
		nouveau_fence_wait(&bo->fence);

	nouveau_bo_realloc_gpu(bo, 0, 0);
	if (bo->sysmem && !bo->user)
		free(bo->sysmem);
	free(bo);
}

int
nouveau_bo_map(struct nouveau_bo *userbo, uint32_t flags)
{
	struct nouveau_bo_priv *bo = nouveau_bo(userbo);

	if (!bo)
		return -EINVAL;

	if (bo->sysmem)
		userbo->map = bo->sysmem;
	else
		userbo->map = bo->map;
	return 0;
}

void
nouveau_bo_unmap(struct nouveau_bo *userbo)
{
	userbo->map = NULL;
}

static int
nouveau_bo_upload(struct nouveau_bo_priv *bo)
{
	if (bo->fence)
		nouveau_fence_wait(&bo->fence);
	memcpy(bo->map, bo->sysmem, bo->drm.size);
	return 0;
}

int
nouveau_bo_validate(struct nouveau_channel *chan, struct nouveau_bo *bo,
		    struct nouveau_fence *fence, uint32_t flags)
{
	struct nouveau_bo_priv *nvbo = nouveau_bo(bo);

	if (!nvbo->drm.size) {
		nouveau_bo_realloc_gpu(nvbo, flags, nvbo->base.size);
		nouveau_bo_upload(nvbo);
	} else
	if (nvbo->user) {
		nouveau_bo_upload(nvbo);
	} else
	if (nvbo->base.map) {
		nouveau_bo_upload(nvbo);
		nvbo->sync_hack = 1;
	}

	if (!nvbo->user && !nvbo->base.map) {
		free(nvbo->sysmem);
		nvbo->sysmem = NULL;
	}

	if (nvbo->fence)
		nouveau_fence_del(&nvbo->fence);
	nouveau_fence_ref(fence, &nvbo->fence);

	nvbo->base.offset = nvbo->drm.offset;
	if (nvbo->drm.flags & NOUVEAU_MEM_AGP)
		nvbo->base.flags = NOUVEAU_BO_GART;
	else
		nvbo->base.flags = NOUVEAU_BO_VRAM;

	return 0;
}


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
nouveau_bo_init(struct nouveau_device *dev)
{
	return 0;
}

void
nouveau_bo_takedown(struct nouveau_device *dev)
{
}

static int
nouveau_bo_realloc_gpu(struct nouveau_bo_priv *nvbo, uint32_t flags, int size)
{
	struct nouveau_device_priv *nvdev = nouveau_device(nvbo->base.device);
	int ret;

	if (nvbo->drm.size && nvbo->drm.size != size) {
		struct drm_nouveau_mem_free f;

		if (nvbo->map) {
			drmUnmap(nvbo->map, nvbo->drm.size);
			nvbo->map = NULL;
		}

		f.flags = nvbo->drm.flags;
		f.offset = nvbo->drm.offset;
		drmCommandWrite(nvdev->fd, DRM_NOUVEAU_MEM_FREE, &f, sizeof(f));

		nvbo->drm.size = 0;
	}

	if (size && !nvbo->drm.size) {
		if (flags) {
			nvbo->drm.flags = 0;
			if (flags & NOUVEAU_BO_VRAM)
				nvbo->drm.flags |= NOUVEAU_MEM_FB;
			if (flags & NOUVEAU_BO_GART)
				nvbo->drm.flags |= (NOUVEAU_MEM_AGP |
						    NOUVEAU_MEM_PCI);
			nvbo->drm.flags |= NOUVEAU_MEM_MAPPED;
		}

		nvbo->drm.size = size;
		
		ret = drmCommandWriteRead(nvdev->fd, DRM_NOUVEAU_MEM_ALLOC,
					  &nvbo->drm, sizeof(nvbo->drm));
		if (ret) {
			free(nvbo);
			return ret;
		}

		ret = drmMap(nvdev->fd, nvbo->drm.map_handle, nvbo->drm.size,
			     &nvbo->map);
		if (ret) {
			nvbo->map = NULL;
			nouveau_bo_del((void *)&nvbo);
			return ret;
		}
	}

	return 0;
}

int
nouveau_bo_new(struct nouveau_device *dev, uint32_t flags, int align,
	       int size, struct nouveau_bo **bo)
{
	struct nouveau_bo_priv *nvbo;
	int ret;

	if (!dev || !bo || *bo)
		return -EINVAL;

	nvbo = calloc(1, sizeof(struct nouveau_bo_priv));
	if (!nvbo)
		return -ENOMEM;
	nvbo->base.device = dev;
	nvbo->drm.alignment = align;

	if (flags & NOUVEAU_BO_PIN) {
		ret = nouveau_bo_realloc_gpu(nvbo, flags, size);
		if (ret) {
			free(nvbo);
			return ret;
		}	
	} else {
		nvbo->sysmem = malloc(size);
		if (!nvbo->sysmem) {
			free(nvbo);
			return -ENOMEM;
		}
	}

	nvbo->base.size = size;
	nvbo->base.offset = nvbo->drm.offset;
	nvbo->base.handle = bo_to_ptr(nvbo);
	nvbo->refcount = 1;
	*bo = &nvbo->base;
	return 0;
}

int
nouveau_bo_user(struct nouveau_device *dev, void *ptr, int size,
		struct nouveau_bo **bo)
{
	struct nouveau_bo_priv *nvbo;

	if (!dev || !bo || *bo)
		return -EINVAL;

	nvbo = calloc(1, sizeof(*nvbo));
	if (!nvbo)
		return -ENOMEM;
	nvbo->base.device = dev;
	
	nvbo->sysmem = ptr;
	nvbo->user = 1;

	nvbo->base.size = size;
	nvbo->base.offset = nvbo->drm.offset;
	nvbo->base.handle = bo_to_ptr(nvbo);
	nvbo->refcount = 1;
	*bo = &nvbo->base;
	return 0;
}

int
nouveau_bo_ref(struct nouveau_device *dev, uint64_t handle,
	       struct nouveau_bo **bo)
{
	struct nouveau_bo_priv *nvbo = ptr_to_bo(handle);

	if (!dev || !bo || *bo)
		return -EINVAL;

	nvbo->refcount++;
	*bo = &nvbo->base;
	return 0;
}

int
nouveau_bo_resize(struct nouveau_bo *bo, int size)
{
	struct nouveau_bo_priv *nvbo = nouveau_bo(bo);
	int ret;

	if (!nvbo || nvbo->user)
		return -EINVAL;

	if (nvbo->sysmem) {
		nvbo->sysmem = realloc(nvbo->sysmem, size);
		if (!nvbo->sysmem)
			return -ENOMEM;
	} else {
		ret = nouveau_bo_realloc_gpu(nvbo, 0, size);
		if (ret)
			return ret;
	}

	nvbo->base.size = size;
	return 0;
}

void
nouveau_bo_del(struct nouveau_bo **bo)
{
	struct nouveau_bo_priv *nvbo;

	if (!bo || !*bo)
		return;
	nvbo = nouveau_bo(*bo);
	*bo = NULL;

	if (--nvbo->refcount)
		return;

	if (nvbo->fence)
		nouveau_fence_wait(&nvbo->fence);

	nouveau_bo_realloc_gpu(nvbo, 0, 0);
	if (nvbo->sysmem && !nvbo->user)
		free(nvbo->sysmem);
	free(nvbo);
}

int
nouveau_bo_map(struct nouveau_bo *bo, uint32_t flags)
{
	struct nouveau_bo_priv *nvbo = nouveau_bo(bo);

	if (!nvbo)
		return -EINVAL;

	if (nvbo->sysmem)
		bo->map = nvbo->sysmem;
	else
		bo->map = nvbo->map;
	return 0;
}

void
nouveau_bo_unmap(struct nouveau_bo *bo)
{
	bo->map = NULL;
}

static int
nouveau_bo_upload(struct nouveau_bo_priv *nvbo)
{
	if (nvbo->fence)
		nouveau_fence_wait(&nvbo->fence);
	memcpy(nvbo->map, nvbo->sysmem, nvbo->drm.size);
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
		if (!nvbo->user) {
			free(nvbo->sysmem);
			nvbo->sysmem = NULL;
		}
	} else
	if (nvbo->user) {
		nouveau_bo_upload(nvbo);
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


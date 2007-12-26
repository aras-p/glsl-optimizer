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
#include <assert.h>

#include "nouveau_drmif.h"
#include "nouveau_dma.h"
#include "nouveau_local.h"

static void
nouveau_mem_free(struct nouveau_device *dev, struct drm_nouveau_mem_alloc *ma,
		 void **map)
{
	struct nouveau_device_priv *nvdev = nouveau_device(dev);
	struct drm_nouveau_mem_free mf;

	if (map && *map) {
		drmUnmap(*map, ma->size);
		*map = NULL;
	}

	if (ma->size) {
		mf.offset = ma->offset;
		mf.flags = ma->flags;
		drmCommandWrite(nvdev->fd, DRM_NOUVEAU_MEM_FREE,
				&mf, sizeof(mf));
		ma->size = 0;
	}
}

static int
nouveau_mem_alloc(struct nouveau_device *dev, unsigned size, unsigned align,
		  uint32_t flags, struct drm_nouveau_mem_alloc *ma, void **map)
{
	struct nouveau_device_priv *nvdev = nouveau_device(dev);
	int ret;

	ma->alignment = align;
	ma->size = size;
	ma->flags = flags;
	if (map)
		ma->flags |= NOUVEAU_MEM_MAPPED;
	ret = drmCommandWriteRead(nvdev->fd, DRM_NOUVEAU_MEM_ALLOC, ma,
				  sizeof(struct drm_nouveau_mem_alloc));
	if (ret)
		return ret;

	if (map) {
		ret = drmMap(nvdev->fd, ma->map_handle, ma->size, map);
		if (ret) {
			*map = NULL;
			nouveau_mem_free(dev, ma, map);
			return ret;
		}
	}

	return 0;
}

static int
nouveau_bo_realloc_gpu(struct nouveau_bo_priv *nvbo, uint32_t flags, int size)
{
	int ret;

	if (nvbo->drm.size && nvbo->drm.size != size) {
		nouveau_mem_free(nvbo->base.device, &nvbo->drm, &nvbo->map);
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

		ret = nouveau_mem_alloc(nvbo->base.device, size,
					nvbo->drm.alignment, nvbo->drm.flags,
					&nvbo->drm, &nvbo->map);
		if (ret) {
			assert(0);
		}
	}

	return 0;
}

static void
nouveau_bo_tmp_del(void *priv)
{
	struct nouveau_resource *r = priv;

	nouveau_fence_ref(NULL, (struct nouveau_fence **)&r->priv);
	nouveau_resource_free(&r);
}

static struct nouveau_resource *
nouveau_bo_tmp(struct nouveau_channel *chan, unsigned size,
	       struct nouveau_fence *fence)
{
	struct nouveau_device_priv *nvdev = nouveau_device(chan->device);
	struct nouveau_resource *r = NULL;
	struct nouveau_fence *ref = NULL;

	if (fence)
		nouveau_fence_ref(fence, &ref);
	else
		nouveau_fence_new(chan, &ref);
	assert(ref);

	while (nouveau_resource_alloc(nvdev->sa_heap, size, ref, &r)) {
		nouveau_fence_flush(chan);
	}
	nouveau_fence_signal_cb(ref, nouveau_bo_tmp_del, r);

	return r;
}

int
nouveau_bo_init(struct nouveau_device *dev)
{
	struct nouveau_device_priv *nvdev = nouveau_device(dev);
	int ret;

	ret = nouveau_mem_alloc(dev, 128*1024, 0, NOUVEAU_MEM_AGP |
				NOUVEAU_MEM_PCI, &nvdev->sa, &nvdev->sa_map);
	if (ret)
		return ret;

	ret = nouveau_resource_init(&nvdev->sa_heap, 0, nvdev->sa.size);
	if (ret) {
		nouveau_mem_free(dev, &nvdev->sa, &nvdev->sa_map);
		return ret;
	}

	return 0;
}

void
nouveau_bo_takedown(struct nouveau_device *dev)
{
	struct nouveau_device_priv *nvdev = nouveau_device(dev);

	nouveau_mem_free(dev, &nvdev->sa, &nvdev->sa_map);
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

	if (flags & NOUVEAU_BO_WR)
		nouveau_fence_wait(&nvbo->fence);
	else
		nouveau_fence_wait(&nvbo->wr_fence);

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

static int
nouveau_bo_validate_user(struct nouveau_channel *chan, struct nouveau_bo *bo,
			 struct nouveau_fence *fence, uint32_t flags)
{
	struct nouveau_channel_priv *nvchan = nouveau_channel(chan);
	struct nouveau_device_priv *nvdev = nouveau_device(chan->device);
	struct nouveau_bo_priv *nvbo = nouveau_bo(bo);
	struct nouveau_resource *r;

	if (nvchan->user_charge + bo->size > nvdev->sa.size)
		return 1;
	nvchan->user_charge += bo->size;

	if (!(flags & NOUVEAU_BO_GART))
		return 1;

	r = nouveau_bo_tmp(chan, bo->size, fence);
	if (!r)
		return 1;

	memcpy(nvdev->sa_map + r->start, nvbo->sysmem, bo->size);

	nvbo->offset = nvdev->sa.offset + r->start;
	nvbo->flags = NOUVEAU_BO_GART;
	return 0;
}

static int
nouveau_bo_validate_bo(struct nouveau_channel *chan, struct nouveau_bo *bo,
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

	nvbo->offset = nvbo->drm.offset;
	if (nvbo->drm.flags & (NOUVEAU_MEM_AGP | NOUVEAU_MEM_PCI))
		nvbo->flags = NOUVEAU_BO_GART;
	else
		nvbo->flags = NOUVEAU_BO_VRAM;

	return 0;
}

int
nouveau_bo_validate(struct nouveau_channel *chan, struct nouveau_bo *bo,
		    struct nouveau_fence *fence, uint32_t flags)
{
	struct nouveau_bo_priv *nvbo = nouveau_bo(bo);
	int ret;

	assert(bo->map == NULL);

	if (nvbo->user) {
		ret = nouveau_bo_validate_user(chan, bo, fence, flags);
		if (ret) {
			ret = nouveau_bo_validate_bo(chan, bo, fence, flags);
			if (ret)
				return ret;
		}
	} else {
		ret = nouveau_bo_validate_bo(chan, bo, fence, flags);
		if (ret)
			return ret;
	}

	if (flags & NOUVEAU_BO_WR)
		nouveau_fence_ref(fence, &nvbo->wr_fence);
	nouveau_fence_ref(fence, &nvbo->fence);
	return 0;
}


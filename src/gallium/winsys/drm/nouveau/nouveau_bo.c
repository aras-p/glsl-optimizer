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

static void
nouveau_bo_tmp_del(void *priv)
{
	struct nouveau_resource *r = priv;

	nouveau_fence_ref(NULL, (struct nouveau_fence **)&r->priv);
	nouveau_resource_free(&r);
}

static unsigned
nouveau_bo_tmp_max(struct nouveau_device_priv *nvdev)
{
	struct nouveau_resource *r = nvdev->sa_heap;
	unsigned max = 0;

	while (r) {
		if (r->in_use && !nouveau_fence(r->priv)->emitted) {
			r = r->next;
			continue;
		}

		if (max < r->size)
			max = r->size;
		r = r->next;
	}

	return max;
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
		if (nouveau_bo_tmp_max(nvdev) < size) {
			nouveau_fence_ref(NULL, &ref);
			return NULL;
		}

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
	nvbo->base.size = size;
	nvbo->base.handle = bo_to_ptr(nvbo);
	nvbo->drm.alignment = align;
	nvbo->refcount = 1;

	if (flags & NOUVEAU_BO_TILED) {
		nvbo->tiled = 1;
		if (flags & NOUVEAU_BO_ZTILE)
			nvbo->tiled |= 2;
		flags &= ~NOUVEAU_BO_TILED;
	}

	ret = nouveau_bo_set_status(&nvbo->base, flags);
	if (ret) {
		free(nvbo);
		return ret;
	}

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

static void
nouveau_bo_del_cb(void *priv)
{
	struct nouveau_bo_priv *nvbo = priv;

	nouveau_fence_ref(NULL, &nvbo->fence);
	nouveau_mem_free(nvbo->base.device, &nvbo->drm, &nvbo->map);
	if (nvbo->sysmem && !nvbo->user)
		free(nvbo->sysmem);
	free(nvbo);
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

	if (nvbo->pending)
		nouveau_pushbuf_flush(nvbo->pending->channel, 0);

	if (nvbo->fence)
		nouveau_fence_signal_cb(nvbo->fence, nouveau_bo_del_cb, nvbo);
	else
		nouveau_bo_del_cb(nvbo);
}

int
nouveau_bo_map(struct nouveau_bo *bo, uint32_t flags)
{
	struct nouveau_bo_priv *nvbo = nouveau_bo(bo);

	if (!nvbo)
		return -EINVAL;

	if (nvbo->pending &&
	    (nvbo->pending->flags & NOUVEAU_BO_WR || flags & NOUVEAU_BO_WR)) {
		nouveau_pushbuf_flush(nvbo->pending->channel, 0);
	}

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

int
nouveau_bo_set_status(struct nouveau_bo *bo, uint32_t flags)
{
	struct nouveau_bo_priv *nvbo = nouveau_bo(bo);
	struct drm_nouveau_mem_alloc new;
	void *new_map = NULL, *new_sysmem = NULL;
	unsigned new_flags = 0, ret;

	assert(!bo->map);

	/* Check current memtype vs requested, if they match do nothing */
	if ((nvbo->drm.flags & NOUVEAU_MEM_FB) && (flags & NOUVEAU_BO_VRAM))
		return 0;
	if ((nvbo->drm.flags & (NOUVEAU_MEM_AGP | NOUVEAU_MEM_PCI)) &&
	    (flags & NOUVEAU_BO_GART))
		return 0;
	if (nvbo->drm.size == 0 && nvbo->sysmem && (flags & NOUVEAU_BO_LOCAL))
		return 0;

	memset(&new, 0x00, sizeof(new));

	/* Allocate new memory */
	if (flags & NOUVEAU_BO_VRAM)
		new_flags |= NOUVEAU_MEM_FB;
	else
	if (flags & NOUVEAU_BO_GART)
		new_flags |= (NOUVEAU_MEM_AGP | NOUVEAU_MEM_PCI);
	
	if (nvbo->tiled && flags) {
		new_flags |= NOUVEAU_MEM_TILE;
		if (nvbo->tiled & 2)
			new_flags |= NOUVEAU_MEM_TILE_ZETA;
	}

	if (new_flags) {
		ret = nouveau_mem_alloc(bo->device, bo->size,
					nvbo->drm.alignment, new_flags,
					&new, &new_map);
		if (ret)
			return ret;
	} else
	if (!nvbo->user) {
		new_sysmem = malloc(bo->size);
	}

	/* Copy old -> new */
	/*XXX: use M2MF */
	if (nvbo->sysmem || nvbo->map) {
		struct nouveau_pushbuf_bo *pbo = nvbo->pending;
		nvbo->pending = NULL;
		nouveau_bo_map(bo, NOUVEAU_BO_RD);
		memcpy(new_map, bo->map, bo->size);
		nouveau_bo_unmap(bo);
		nvbo->pending = pbo;
	}

	/* Free old memory */
	if (nvbo->fence)
		nouveau_fence_wait(&nvbo->fence);
	nouveau_mem_free(bo->device, &nvbo->drm, &nvbo->map);
	if (nvbo->sysmem && !nvbo->user)
		free(nvbo->sysmem);

	nvbo->drm = new;
	nvbo->map = new_map;
	if (!nvbo->user)
		nvbo->sysmem = new_sysmem;
	bo->flags = flags;
	bo->offset = nvbo->drm.offset;
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

	if (!(flags & NOUVEAU_BO_GART))
		return 1;

	r = nouveau_bo_tmp(chan, bo->size, fence);
	if (!r)
		return 1;
	nvchan->user_charge += bo->size;

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
	int ret;

	ret = nouveau_bo_set_status(bo, flags);
	if (ret) {
		nouveau_fence_flush(chan);

		ret = nouveau_bo_set_status(bo, flags);
		if (ret)
			return ret;
	}

	if (nvbo->user)
		nouveau_bo_upload(nvbo);

	nvbo->offset = nvbo->drm.offset;
	if (nvbo->drm.flags & (NOUVEAU_MEM_AGP | NOUVEAU_MEM_PCI))
		nvbo->flags = NOUVEAU_BO_GART;
	else
		nvbo->flags = NOUVEAU_BO_VRAM;

	return 0;
}

int
nouveau_bo_validate(struct nouveau_channel *chan, struct nouveau_bo *bo,
		    uint32_t flags)
{
	struct nouveau_bo_priv *nvbo = nouveau_bo(bo);
	struct nouveau_fence *fence = nouveau_pushbuf(chan->pushbuf)->fence;
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


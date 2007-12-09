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

#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include "nouveau_drmif.h"
#include "nouveau_dma.h"

int
nouveau_pushbuf_init(struct nouveau_channel *chan)
{
	struct nouveau_channel_priv *nvchan = nouveau_channel(chan);

	if (!nvchan)
		return -EINVAL;

	/* Everything except first 4KiB of the push buffer is managed by us */
	if (nouveau_resource_init(&nvchan->pb_heap,
				  nvchan->drm.cmdbuf_size - 4096))
		return -EINVAL;

	/* Shrink master ring to 4KiB */
	assert(nvchan->dma.cur <= (4096/4));
	nvchan->dma.max = (4096 / 4) - 2;
	nvchan->dma.free = nvchan->dma.max - nvchan->dma.cur;

	return 0;
}

/* This would be our TTM "superioctl" */
int
nouveau_pushbuf_flush(struct nouveau_channel *chan)
{
	struct nouveau_channel_priv *nvchan = nouveau_channel(chan);
	struct nouveau_pushbuf_priv *nvpb = nouveau_pushbuf(nvchan->pb_tail);
	struct nouveau_pushbuf_bo *pbbo;
	struct nouveau_fence *fence = NULL;
	int sync_hack = 0;
	int ret;

	if (!nvpb)
		goto out_realloc;

	if (nvpb->base.remaining == nvpb->res->size / 4)
		return 0;

	ret = nouveau_fence_new(chan, &fence);
	if (ret)
		return ret;

	/* Validate buffers + apply relocations */
	while ((pbbo = ptr_to_pbbo(nvpb->buffers))) {
		struct nouveau_pushbuf_reloc *r;
		struct nouveau_bo *bo = &ptr_to_bo(pbbo->handle)->base;

		ret = nouveau_bo_validate(chan, bo, fence, pbbo->flags);
		assert (ret == 0);

		sync_hack |= nouveau_bo(bo)->sync_hack;
		nouveau_bo(bo)->sync_hack = 0;

		while ((r = ptr_to_pbrel(pbbo->relocs))) {
			uint32_t push;

			if (r->flags & NOUVEAU_BO_LOW) {
				push = bo->offset + r->data;
			} else
			if (r->flags & NOUVEAU_BO_HIGH) {
				push = (bo->offset + r->data) >> 32;
			} else {
				push = r->data;
			}

			if (r->flags & NOUVEAU_BO_OR) {
				if (bo->flags & NOUVEAU_BO_VRAM)
					push |= r->vor;
				else
					push |= r->tor;
			}

			*r->ptr = push;
			pbbo->relocs = r->next;
			free(r);
		}

		nvpb->buffers = pbbo->next;
		free(pbbo);
	}
	nvpb->nr_buffers = 0;

	/* Emit JMP to indirect pushbuf */
	if (nvchan->dma.free < 1)
		WAIT_RING_CH(chan, 1);
	nvchan->dma.free -= 1;
#ifdef NOUVEAU_DMA_DEBUG
	nvchan->dma.push_free = 1;
#endif
	OUT_RING_CH(chan, 0x20000000 | (nvpb->res->start + 4096));

	/* Add JMP back to master pushbuf from indirect pushbuf */
	(*nvpb->base.cur++) =
		0x20000000 | ((nvchan->dma.cur << 2) + nvchan->dma.base);

	/* Fence */
	nvpb->fence = fence;
	nouveau_fence_emit(nvpb->fence);

	/* Kickoff */
	FIRE_RING_CH(chan);

	/* Allocate space for next push buffer */
out_realloc:
	nvpb = calloc(1, sizeof(struct nouveau_pushbuf_priv));
	if (!nvpb)
		return -ENOMEM;

	if (nouveau_resource_alloc(nvchan->pb_heap, 0x2000, NULL, &nvpb->res)) {
		struct nouveau_pushbuf_priv *e;
		int nr = 0;

		/* Update fences */
		nouveau_fence_flush(chan);

		/* Free any push buffers that have already been executed */
		e = nouveau_pushbuf(nvchan->pb_head);
		while (e && e->fence) {
			if (!e->fence || !nouveau_fence(e->fence)->signalled)
				break;
			nouveau_fence_del(&e->fence);
			nouveau_resource_free(&e->res);
			nr++;

			nvchan->pb_head = e->next;
			if (nvchan->pb_head == NULL)
				nvchan->pb_tail = NULL;
			free(e);
			e = nouveau_pushbuf(nvchan->pb_head);
		}

		/* We didn't free any buffers above.  As a last resort, busy
		 * wait on the oldest buffer becoming available.
		 */
		if (!nr) {
			e = nouveau_pushbuf(nvchan->pb_head);
			nouveau_fence_wait(&e->fence);
			nouveau_resource_free(&e->res);

			nvchan->pb_head = e->next;
			if (nvchan->pb_head == NULL)
				nvchan->pb_tail = NULL;
			free(e);
		}

		if (nouveau_resource_alloc(nvchan->pb_heap, 0x2000, nvpb,
					   &nvpb->res))
			assert(0);
	}

	nvpb->base.channel = chan;
	nvpb->base.remaining = nvpb->res->size / 4;
	nvpb->base.cur = &nvchan->pushbuf[(nvpb->res->start + 4096)/4];

	if (nvchan->pb_tail) {
		nouveau_pushbuf(nvchan->pb_tail)->next = &nvpb->base;
	} else {
		nvchan->pb_head = &nvpb->base;
	}
	nvchan->pb_tail = &nvpb->base;

	if (sync_hack) {
		struct nouveau_fence *f = NULL;
		nouveau_fence_ref(nvpb->fence, &f);
		nouveau_fence_wait(&f);
	}

	return 0;
}

static struct nouveau_pushbuf_bo *
nouveau_pushbuf_emit_buffer(struct nouveau_channel *chan, struct nouveau_bo *bo)
{
	struct nouveau_channel_priv *nvchan = nouveau_channel(chan);
	struct nouveau_pushbuf_priv *nvpb = nouveau_pushbuf(nvchan->pb_tail);
	struct nouveau_pushbuf_bo *pbbo = ptr_to_pbbo(nvpb->buffers);

	while (pbbo) {
		if (pbbo->handle == bo->handle)
			return pbbo;
		pbbo = ptr_to_pbbo(pbbo->next);
	}

	pbbo = malloc(sizeof(struct nouveau_pushbuf_bo));
	pbbo->next = nvpb->buffers;
	nvpb->buffers = pbbo_to_ptr(pbbo);
	nvpb->nr_buffers++;

	pbbo->handle = bo_to_ptr(bo);
	pbbo->flags = NOUVEAU_BO_VRAM | NOUVEAU_BO_GART;
	pbbo->relocs = 0;
	pbbo->nr_relocs = 0;
	return pbbo;
}

int
nouveau_pushbuf_emit_reloc(struct nouveau_channel *chan, void *ptr,
			   struct nouveau_bo *bo, uint32_t data, uint32_t flags,
			   uint32_t vor, uint32_t tor)
{
	struct nouveau_pushbuf_bo *pbbo;
	struct nouveau_pushbuf_reloc *r;

	if (!chan)
		return -EINVAL;

	pbbo = nouveau_pushbuf_emit_buffer(chan, bo);
	if (!pbbo)
		return -EFAULT;

	r = malloc(sizeof(struct nouveau_pushbuf_reloc));
	r->next = pbbo->relocs;
	pbbo->relocs = pbrel_to_ptr(r);
	pbbo->nr_relocs++;

	pbbo->flags |= (flags & NOUVEAU_BO_RDWR);
	pbbo->flags &= (flags | NOUVEAU_BO_RDWR);

	r->handle = bo_to_ptr(r);
	r->ptr = ptr;
	r->flags = flags;
	r->data = data;
	r->vor = vor;
	r->tor = tor;

	return 0;
}


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

#define PB_RSVD_DWORDS 2

static int
nouveau_pushbuf_space(struct nouveau_channel *chan)
{
	struct nouveau_channel_priv *nvchan = nouveau_channel(chan);
	struct nouveau_pushbuf_priv *nvpb;

	nvpb = calloc(1, sizeof(struct nouveau_pushbuf_priv));
	if (!nvpb)
		return -ENOMEM;

	while (nouveau_resource_alloc(nvchan->pb_heap, 0x2100, NULL,
				      &nvpb->res)) {
		nouveau_fence_flush(chan);
	}

	nvpb->base.channel = chan;
	nvpb->base.remaining = (nvpb->res->size / 4) - PB_RSVD_DWORDS;
	nvpb->base.cur = &nvchan->pushbuf[nvpb->res->start/4];
	nvchan->pb_tail = &nvpb->base;
	nvchan->base.pushbuf = nvchan->pb_tail;

	return 0;
}

int
nouveau_pushbuf_init(struct nouveau_channel *chan)
{
	struct nouveau_channel_priv *nvchan = nouveau_channel(chan);

	if (!nvchan)
		return -EINVAL;

	/* Everything except first 4KiB of the push buffer is managed by us */
	if (nouveau_resource_init(&nvchan->pb_heap, 4096,
				  nvchan->drm.cmdbuf_size - 4096))
		return -EINVAL;

	/* Shrink master ring to 4KiB */
	assert(nvchan->dma.cur <= (4096/4));
	nvchan->dma.max = (4096 / 4) - 2;
	nvchan->dma.free = nvchan->dma.max - nvchan->dma.cur;

	assert(!nouveau_pushbuf_space(chan));

	return 0;
}

static void
nouveau_pushbuf_fence_signalled(void *priv)
{
	struct nouveau_pushbuf_priv *nvpb = nouveau_pushbuf(priv);

	nouveau_fence_del(&nvpb->fence);
	nouveau_resource_free(&nvpb->res);
	free(nvpb);
}

/* This would be our TTM "superioctl" */
int
nouveau_pushbuf_flush(struct nouveau_channel *chan)
{
	struct nouveau_channel_priv *nvchan = nouveau_channel(chan);
	struct nouveau_pushbuf_priv *nvpb = nouveau_pushbuf(chan->pushbuf);
	struct nouveau_pushbuf_bo *pbbo;
	struct nouveau_fence *fence = NULL;
	int ret;

	if (nvpb->base.remaining == (nvpb->res->size / 4) - PB_RSVD_DWORDS)
		return 0;
	nvchan->pb_tail = NULL;

	ret = nouveau_fence_new(chan, &fence);
	if (ret)
		return ret;

	/* Validate buffers + apply relocations */
	while ((pbbo = ptr_to_pbbo(nvpb->buffers))) {
		struct nouveau_pushbuf_reloc *r;
		struct nouveau_bo *bo = &ptr_to_bo(pbbo->handle)->base;

		ret = nouveau_bo_validate(chan, bo, fence, pbbo->flags);
		assert (ret == 0);

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
	RING_SPACE_CH(chan, 1);
	OUT_RING_CH(chan, 0x20000000 | nvpb->res->start);

	/* Add JMP back to master pushbuf from indirect pushbuf */
	(*nvpb->base.cur++) =
		0x20000000 | ((nvchan->dma.cur << 2) + nvchan->dma.base);

	/* Fence */
	nvpb->fence = fence;
	nouveau_fence_signal_cb(nvpb->fence, nouveau_pushbuf_fence_signalled,
				nvpb);
	nouveau_fence_emit(nvpb->fence);

	/* Kickoff */
	FIRE_RING_CH(chan);

	/* Allocate space for next push buffer */
	assert(!nouveau_pushbuf_space(chan));

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

